//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#ifndef NATRON_ENGINE_EFFECTINSTANCE_H_
#define NATRON_ENGINE_EFFECTINSTANCE_H_
#include <list>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>

#include "Global/GlobalDefines.h"
#include "Global/KeySymbols.h"
#include "Engine/Knob.h" // for KnobHolder
#include "Engine/Rect.h"
#include "Engine/ImageLocker.h"

class Hash64;
class RenderTree;
class VideoEngine;
class RenderTree;
class Format;
class OverlaySupport;
class PluginMemory;
class BlockingBackgroundRender;

namespace Natron {
class Node;
class Image;
class ImageParams;
/**
 * @brief This is the base class for visual effects.
 * A live instance is always living throughout the lifetime of a Node and other copies are
 * created on demand when a render is needed.
 **/
class EffectInstance
    : public NamedKnobHolder
    , public LockManagerI<Natron::Image>
{
public:

    typedef std::map<EffectInstance*,RectD> RoIMap; // RoIs are in canonical coordinates
    typedef std::map<int, std::vector<RangeD> > FramesNeededMap;

    struct RenderRoIArgs
    {
        SequenceTime time; //< the time at which to render
        RenderScale scale; //< the scale at which to render
        unsigned int mipMapLevel; //< the mipmap level (redundant with the scale, stored here to avoid refetching it everytimes)
        int view; //< the view to render
        RectI roi; //< the renderWindow (in pixel coordinates) , watch out OpenFX action getRegionsOfInterest expects canonical coords!
        bool isSequentialRender; //< is this render part of a sequential render (playback or render on disk) ?
        bool isRenderUserInteraction; // is this render due to user interaction ? (parameter tweek)
        bool byPassCache; //< turn-off the cache ability to look-up existing images ? (false when a refresh is forced)
        RectD preComputedRoD; //<  pre-computed region of definition in canonical coordinates for this effect to speed-up the call to renderRoi
        Natron::ImageComponents components; //< the requested image components
        Natron::ImageBitDepth bitdepth; //< the requested bit depth
        int channelForAlpha; //< if this is a mask this is from this channel that we will fetch the mask

        RenderRoIArgs()
        {
        }

        RenderRoIArgs(SequenceTime time_,
                      const RenderScale & scale_,
                      unsigned int mipMapLevel_,
                      int view_,
                      const RectI & roi_,
                      bool isSequentialRender_,
                      bool isRenderUserInteraction_,
                      bool byPassCache_,
                      const RectD & preComputedRoD_,
                      Natron::ImageComponents components_,
                      Natron::ImageBitDepth bitdepth_,
                      int channelForAlpha_ = 3)
            : time(time_)
              , scale(scale_)
              , mipMapLevel(mipMapLevel_)
              , view(view_)
              , roi(roi_)
              , isSequentialRender(isSequentialRender_)
              , isRenderUserInteraction(isRenderUserInteraction_)
              , byPassCache(byPassCache_)
              , preComputedRoD(preComputedRoD_)
              , components(components_)
              , bitdepth(bitdepth_)
              , channelForAlpha(channelForAlpha_)
        {
        }
    };

    enum SupportsEnum
    {
        eSupportsMaybe = -1,
        eSupportsNo = 0,
        eSupportsYes = 1
    };

public:


    /**
     * @brief Constructor used once for each node created. Its purpose is to create the "live instance".
     * You shouldn't do any heavy processing here nor lengthy initialization as the constructor is often
     * called just to be able to call a few virtuals fonctions.
     * The constructor is always called by the main thread of the application.
     **/
    explicit EffectInstance(boost::shared_ptr<Natron::Node> node);

    virtual ~EffectInstance();

    /**
     * @brief Returns true once the effect has been fully initialized and is ready to have its actions called apart from
     * the createInstanceAction
     **/
    virtual bool isEffectCreated() const { return true; }
    
    
    /**
     * @brief Returns a pointer to the node holding this effect.
     **/
    boost::shared_ptr<Natron::Node> getNode() const WARN_UNUSED_RETURN
    {
        return _node;
    }

    /**
     * @brief Returns the "real" hash of the node synchronized with the gui state
     **/
    U64 getHash() const WARN_UNUSED_RETURN;

    /**
     * @brief Returns the hash the node had at the start of renderRoI. This will return the same value
     * at any time during the same render call.
     * @returns This function returns true if case of success, false otherwise.
     **/
    U64 getRenderHash() const WARN_UNUSED_RETURN;

    U64 getKnobsAge() const WARN_UNUSED_RETURN;

    /**
     * @brief Set the knobs age of this node to be 'age'. Note that this can be called
     * for 2 reasons:
     * - loading a project
     * - If this node is a clone and the master node changed its hash.
     **/
    void setKnobsAge(U64 age);

    /**
     * @brief Forwarded to the node's name
     **/
    const std::string & getName() const WARN_UNUSED_RETURN;
    virtual std::string getName_mt_safe() const OVERRIDE FINAL WARN_UNUSED_RETURN;

    /**
     * @brief Forwarded to the node's render format
     **/
    void getRenderFormat(Format *f) const;

    /**
     * @brief Forwarded to the node's render views count
     **/
    int getRenderViewsCount() const WARN_UNUSED_RETURN;

    /**
     * @brief Returns input n. It might be NULL if the input is not connected.
     * MT-Safe
     **/
    EffectInstance* getInput(int n) const WARN_UNUSED_RETURN;

    /**
     * @brief Forwarded to the node holding the effect
     **/
    bool hasOutputConnected() const WARN_UNUSED_RETURN;

    /**
     * @brief Must return the plugin's major version.
     **/
    virtual int getMajorVersion() const WARN_UNUSED_RETURN = 0;

    /**
     * @brief Must return the plugin's minor version.
     **/
    virtual int getMinorVersion() const WARN_UNUSED_RETURN = 0;

    /**
     * @brief Is this node an input node ? An input node means
     * it has no input.
     **/
    virtual bool isGenerator() const WARN_UNUSED_RETURN
    {
        return false;
    }

    /**
     * @brief Is the node a reader ?
     **/
    virtual bool isReader() const WARN_UNUSED_RETURN
    {
        return false;
    }

    /**
     * @brief Is the node a writer ?
     **/
    virtual bool isWriter() const WARN_UNUSED_RETURN
    {
        return false;
    }

    /**
     * @brief Is this node an output node ? An output node means
     * it has no output.
     **/
    virtual bool isOutput() const WARN_UNUSED_RETURN
    {
        return false;
    }

    /**
     * @brief Returns true if the node is capable of generating
     * data and process data on the input as well
     **/
    virtual bool isGeneratorAndFilter() const WARN_UNUSED_RETURN
    {
        return false;
    }

    /**
     * @brief Is this node an OpenFX node?
     **/
    virtual bool isOpenFX() const WARN_UNUSED_RETURN
    {
        return false;
    }

    /**
     * @brief How many input can we have at most. (i.e: how many input arrows)
     **/
    virtual int getMaxInputCount() const WARN_UNUSED_RETURN = 0;

    /**
     * @brief Is inputNb optional ? In which case the render can be made without it.
     **/
    virtual bool isInputOptional(int inputNb) const WARN_UNUSED_RETURN = 0;

    /**
     * @brief Is inputNb a mask ? In which case the effect will have an additionnal mask parameter.
     **/
    virtual bool isInputMask(int /*inputNb*/) const WARN_UNUSED_RETURN
    {
        return false;
    };

    /**
     * @brief Is the input a roto brush ?
     **/
    virtual bool isInputRotoBrush(int /*inputNb*/) const WARN_UNUSED_RETURN
    {
        return false;
    }

    virtual int getRotoBrushInputIndex() const WARN_UNUSED_RETURN
    {
        return -1;
    }
    
    /**
     * @brief Returns the index of the channel to use to produce the mask.
     * None = -1
     * R = 0
     * G = 1
     * B = 2
     * A = 3
     **/
    int getMaskChannel(int inputNb) const;

    /**
     * @brief Returns whether masking is enabled or not
     **/
    bool isMaskEnabled(int inputNb) const;

    /**
     * @brief Routine called after the creation of an effect. This function must
     * fill for the given input what image components we can feed it with.
     * This function is also called to specify what image components this effect can output.
     * In that case inputNb equals -1.
     **/
    virtual void addAcceptedComponents(int inputNb,std::list<Natron::ImageComponents>* comps) = 0;
    virtual void addSupportedBitDepth(std::list<Natron::ImageBitDepth>* depths) const = 0;

    /**
     * @brief Must return the deepest bit depth that this plug-in can support.
     * If 32 float is supported then return Natron::IMAGE_FLOAT, otherwise
     * return IMAGE_SHORT if 16 bits is supported, and as a last resort, return
     * IMAGE_BYTE. At least one must be returned.
     **/
    Natron::ImageBitDepth getBitDepth() const;

    bool isSupportedBitDepth(Natron::ImageBitDepth depth) const;

    /**
     * @brief Returns true if the given input supports the given components. If inputNb equals -1
     * then this function will check whether the effect can produce the given components.
     **/
    bool isSupportedComponent(int inputNb,Natron::ImageComponents comp) const;

    /**
     * @brief Returns the most appropriate components that can be supported by the inputNb.
     * If inputNb equals -1 then this function will check the output components.
     **/
    Natron::ImageComponents findClosestSupportedComponents(int inputNb,Natron::ImageComponents comp) const WARN_UNUSED_RETURN;

    /**
     * @brief Returns the preferred depth and components for the given input.
     * If inputNb equals -1 then this function will check the output components.
     **/
    virtual void getPreferredDepthAndComponents(int inputNb,Natron::ImageComponents* comp,Natron::ImageBitDepth* depth) const;

    /**
     * @brief Override to get the preffered premultiplication flag for the output image
     **/
    virtual Natron::ImagePremultiplication getOutputPremultiplication() const
    {
        return Natron::ImagePremultiplied;
    }

    /**
     * @brief Can be derived to give a more meaningful label to the input 'inputNb'
     **/
    virtual std::string getInputLabel(int inputNb) const WARN_UNUSED_RETURN;

    /**
     * @brief Must be implemented to give the plugin internal id(i.e: net.sf.openfx:invertPlugin)
     **/
    virtual std::string getPluginID() const WARN_UNUSED_RETURN = 0;

    /**
     * @brief Must be implemented to give the plugin a label that will be used by the graphical
     * user interface.
     **/
    virtual std::string getPluginLabel() const WARN_UNUSED_RETURN = 0;

    /**
     * @brief The grouping of the plug-in. For instance Views/Stereo/MyStuff
     * Each string being one level of the grouping. The first one being the name
     * of one group that will appear in the user interface.
     **/
    virtual void getPluginGrouping(std::list<std::string>* grouping) const = 0;

    /**
     * @brief Must be implemented to give a desription of the effect that this node does. This is typically
     * what you'll see displayed when the user clicks the '?' button on the node's panel in the user interface.
     **/
    virtual std::string getDescription() const WARN_UNUSED_RETURN = 0;


    /**
     * @bried Returns the effect render order preferences:
     * EFFECT_NOT_SEQUENTIAL: The effect does not need to be run in a sequential order
     * EFFECT_ONLY_SEQUENTIAL: The effect can only be run in a sequential order (i.e like the background render would do)
     * EFFECT_PREFER_SEQUENTIAL: This indicates that the effect would work better by rendering sequential. This is merely
     * a hint to Natron but for now we just consider it as EFFECT_NOT_SEQUENTIAL.
     **/
    virtual Natron::SequentialPreference getSequentialPreference() const
    {
        return Natron::EFFECT_NOT_SEQUENTIAL;
    }

    /**
     * @brief Renders the image at the given time,scale and for the given view & render window.
     * @param args See the definition of the class for comments on each argument.
     * @param hashUsed, if not null, it will be set to the hash of this effect used by this function to render.
     * This is useful if we want to retrieve results from the cache with this exact hash key afterwards because the
     * real hash key of the effect might have changed due to user interaction already.
     **/
    boost::shared_ptr<Image> renderRoI(const RenderRoIArgs & args,U64* hashUsed = NULL) WARN_UNUSED_RETURN;

    /**
     * @brief Same as renderRoI(...) but takes in parameter
     * the outputImage where to render instead. This is used by the Viewer which already did
     * a cache look-up for optimization purposes.
     **/
    void renderRoI(SequenceTime time,
                   const RenderScale & scale,
                   unsigned int mipMapLevel,
                   int view,
                   const RectI & renderWindow,
                   const RectD & rod, //!< effect rod in canonical coords
                   const boost::shared_ptr<ImageParams> & cachedImgParams,
                   const boost::shared_ptr<Image> & image,
                   const boost::shared_ptr<Image> & downscaledImage,
                   bool isSequentialRender,
                   bool isRenderMadeInResponseToUserInteraction,
                   bool byPassCache,
                   U64 nodeHash);

    /**
     * @breif Don't override this one, override onKnobValueChanged instead.
     **/
    virtual void onKnobValueChanged_public(KnobI* k,Natron::ValueChangedReason reason,SequenceTime time) OVERRIDE FINAL;

    /**
     * @brief Returns a pointer to the first non disabled upstream node.
     * When cycling through the tree, we prefer non optional inputs and we span inputs
     * from last to first.
     * If this not is not disabled, it will return a pointer to this.
     **/
    Natron::EffectInstance* getNearestNonDisabled() const;
    
    /**
     * @brief Same as getNearestNonDisabled except that it looks for the nearest non identity node.
     * This function calls the action isIdentity and getRegionOfDefinition and can be expensive!
     **/
    Natron::EffectInstance* getNearestNonIdentity(int time);

    /**
     * @brief This is purely for the OfxEffectInstance derived class, but passed here for the sake of abstraction
     **/
    virtual void checkOFXClipPreferences(double /*time*/,
                                         const RenderScale & /*scale*/,
                                         const std::string & /*reason*/,
                                        bool /*forceGetClipPrefAction*/) {}
    
    /**
     * @brief Returns the output aspect ratio to render with
     **/
    virtual double getPreferredAspectRatio() const { return 1.; }

    virtual void lock(const boost::shared_ptr<Natron::Image>& entry) OVERRIDE FINAL;
    virtual void unlock(const boost::shared_ptr<Natron::Image>& entry) OVERRIDE FINAL ;

protected:
    /**
     * @brief Must fill the image 'output' for the region of interest 'roi' at the given time and
     * at the given scale.
     * Pre-condition: render() has been called for all inputs so the portion of the image contained
     * in output corresponding to the roi is valid.
     * Note that this function can be called concurrently for the same output image but with different
     * rois, depending on the threading-affinity of the plug-in.
     **/
    virtual Natron::Status render(SequenceTime /*time*/,
                                  const RenderScale & /*scale*/,
                                  const RectI & /*roi*/,
                                  int /*view*/,
                                  bool /*isSequentialRender*/,
                                  bool /*isRenderResponseToUserInteraction*/,
                                  boost::shared_ptr<Natron::Image> /*output*/) WARN_UNUSED_RETURN
    {
        return Natron::StatOK;
    }

public:

    Natron::Status render_public(SequenceTime time,
                                 const RenderScale & scale,
                                 const RectI & roi,
                                 int view,
                                 bool isSequentialRender,
                                 bool isRenderResponseToUserInteraction,
                                 boost::shared_ptr<Natron::Image> output) WARN_UNUSED_RETURN;

protected:
    /**
     * @brief Can be overloaded to indicates whether the effect is an identity, i.e it doesn't produce
     * any change in output.
     * @param time The time of interest
     * @param scale The scale of interest
     * @param rod The image region of definition, in canonical coordinates
     * @param view The view we 're interested in
     * @param inputTime[out] the input time to which this plugin is identity of
     * @param inputNb[out] the input number of the effect that is identity of.
     * The special value of -2 indicates that the plugin is identity of itself at another time
     **/
    virtual bool isIdentity(SequenceTime /*time*/,
                            const RenderScale & /*scale*/,
                            const RectD & /*rod*/,
                            int /*view*/,
                            SequenceTime* /*inputTime*/,
                            int* /*inputNb*/) WARN_UNUSED_RETURN
    {
        return false;
    }

public:

    bool isIdentity_public(U64 hash,
                           SequenceTime time,
                           const RenderScale & scale,
                           const RectD & rod, //!< image rod in canonical coordinates
                           int view,SequenceTime* inputTime,
                           int* inputNb) WARN_UNUSED_RETURN;
    enum RenderSafety
    {
        UNSAFE = 0,
        INSTANCE_SAFE = 1,
        FULLY_SAFE = 2,
        FULLY_SAFE_FRAME = 3,
    };

    /**
     * @brief Indicates how many simultaneous renders the plugin can deal with.
     * RenderSafety::UNSAFE - indicating that only a single 'render' call can be made at any time amoung all instances,
     * RenderSafety::INSTANCE_SAFE - indicating that any instance can have a single 'render' call at any one time,
     * RenderSafety::FULLY_SAFE - indicating that any instance of a plugin can have multiple renders running simultaneously
     * RenderSafety::FULLY_SAFE_FRAME - Same as FULLY_SAFE but the plug-in also flagged  kOfxImageEffectPluginPropHostFrameThreading to true.
     **/
    virtual RenderSafety renderThreadSafety() const WARN_UNUSED_RETURN = 0;

    /**
     * @brief Can be derived to indicate that the data rendered by the plug-in is expensive
     * and should be stored in a persistent manner such as on disk.
     **/
    virtual bool shouldRenderedDataBePersistent() const WARN_UNUSED_RETURN
    {
        return false;
    }

    /*@brief The derived class should query this to abort any long process
       in the engine function.*/
    bool aborted() const WARN_UNUSED_RETURN;

    /**
     * @brief Called externally when the rendering is aborted. You should never
     * call this yourself.
     **/
    void setAborted(bool b);

    /** @brief Returns the image computed by the input 'inputNb' at the given time and scale for the given view.
     * @param dontUpscale If the image is retrieved is downscaled but the plug-in doesn't support the user of
     * downscaled images by default we upscale the image. If dontUpscale is true then we don't do this upscaling.
     *
     * @param roiPixel If non NULL will be set to the render window used to render the image, that is, either the
     * region of interest of this effect on the input effect we want to render or the optionalBounds if set, but
     * converted to pixel coordinates
     */
    boost::shared_ptr<Image> getImage(int inputNb,
                                      const SequenceTime time,
                                      const RenderScale & scale,
                                      const int view,
                                      const RectD *optionalBounds, //!< optional region in canonical coordinates
                                      const Natron::ImageComponents comp,
                                      const Natron::ImageBitDepth depth,
                                      const bool dontUpscale,
                                      RectI* roiPixel) WARN_UNUSED_RETURN;
    virtual void aboutToRestoreDefaultValues() OVERRIDE FINAL;

protected:


    /**
     * @brief Can be derived to get the region that the plugin is capable of filling.
     * This is meaningful for plugins that generate images or transform images.
     * By default it returns in rod the union of all inputs RoD and StatReplyDefault is returned.
     * @param isProjectFormat[out] If set to true, then rod is taken to be equal to the current project format.
     * In case of failure the plugin should return StatFailed.
     * @returns StatOK, StatReplyDefault, or StatFailed. rod is set except if return value is StatOK or StatReplyDefault.
     **/
    virtual Natron::Status getRegionOfDefinition(U64 hash,SequenceTime time, const RenderScale & scale, int view, RectD* rod) WARN_UNUSED_RETURN;
    virtual void calcDefaultRegionOfDefinition(U64 hash,SequenceTime time,int view, const RenderScale & scale, RectD *rod) ;

    /**
     * @brief If the instance rod is infinite, returns the union of all connected inputs. If there's no input this returns the
     * project format.
     * @returns true if the rod is set to the project format.
     **/
    bool ifInfiniteApplyHeuristic(U64 hash,SequenceTime time, const RenderScale & scale, int view, RectD* rod) ;

    /**
     * @brief Can be derived to indicate for each input node what is the region of interest
     * of the node at time 'time' and render scale 'scale' given a render window.
     * For exemple a blur plugin would specify what it needs
     * from inputs in order to do a blur taking into account the size of the blurring kernel.
     * By default, it returns renderWindow for each input.
     **/
    virtual RoIMap getRegionsOfInterest(SequenceTime time,
                                        const RenderScale & scale,
                                        const RectD & outputRoD, //!< the RoD of the effect, in canonical coordinates
                                        const RectD & renderWindow, //!< the region to be rendered in the output image, in Canonical Coordinates
                                        int view) WARN_UNUSED_RETURN;

    /**
     * @brief Can be derived to indicate for each input node what is the frame range(s) (which can be discontinuous)
     * that this effects need in order to render the frame at the given time.
     **/
    virtual FramesNeededMap getFramesNeeded(SequenceTime time) WARN_UNUSED_RETURN;


    /**
     * @brief Can be derived to get the frame range wherein the plugin is capable of producing frames.
     * By default it merges the frame range of the inputs.
     * In case of failure the plugin should return StatFailed.
     **/
    virtual void getFrameRange(SequenceTime *first,SequenceTime *last);

public:

    Natron::Status getRegionOfDefinition_public(U64 hash,
                                                SequenceTime time,
                                                const RenderScale & scale,
                                                int view,
                                                RectD* rod,
                                                bool* isProjectFormat) WARN_UNUSED_RETURN;

    RoIMap getRegionsOfInterest_public(SequenceTime time,
                                       const RenderScale & scale,
                                       const RectD & outputRoD,
                                       const RectD & renderWindow, //!< the region to be rendered in the output image, in Canonical Coordinates
                                       int view) WARN_UNUSED_RETURN;

    FramesNeededMap getFramesNeeded_public(SequenceTime time) WARN_UNUSED_RETURN;

    void getFrameRange_public(U64 hash,SequenceTime *first,SequenceTime *last);

    /**
     * @brief Override to initialize the overlay interact. It is called only on the
     * live instance.
     **/
    virtual void initializeOverlayInteract()
    {
    }

    virtual void setCurrentViewportForOverlays(OverlaySupport* /*viewport*/)
    {
    }

    /**
     * @brief Overload this and return true if your operator should dislay a preview image by default.
     **/
    virtual bool makePreviewByDefault() const WARN_UNUSED_RETURN
    {
        return false;
    }

    /**
     * @brief Called on generator effects upon creation if they have an image input file field.
     **/
    void openImageFileKnob();


    /**
     * @brief
     * You must call this in order to notify the GUI of any change (add/delete) for knobs not made during
     * initializeKnobs().
     * For example you may want to remove some knobs in response to a value changed of another knob.
     * This is something that OpenFX does not provide but we make it possible for Natron plugins.
     * - To properly delete a knob just call the destructor of the knob.
     * - To properly delete
     **/
    void createKnobDynamically();


    /**
     * @brief Used to bracket a series of call to onKnobValueChanged(...) in case many complex changes are done
     * at once. If not called, onKnobValueChanged() will call automatically bracket its call be a begin/end
     * but this can lead to worse performance. You can overload this to make all changes to params at once.
     **/
    virtual void beginKnobsValuesChanged(Natron::ValueChangedReason /*reason*/) OVERRIDE
    {
    }

    /**
     * @brief Used to bracket a series of call to onKnobValueChanged(...) in case many complex changes are done
     * at once. If not called, onKnobValueChanged() will call automatically bracket its call be a begin/end
     * but this can lead to worse performance. You can overload this to make all changes to params at once.
     **/
    virtual void endKnobsValuesChanged(Natron::ValueChangedReason /*reason*/) OVERRIDE
    {
    }

    /**
     * @brief Can be overloaded to clear any cache the plugin might be
     * handling on his side.
     **/
    virtual void purgeCaches()
    {
    };

    void clearLastRenderedImage();

    /**
     * @brief Use this function to post a transient message to the user. It will be displayed using
     * a dialog. The message can be of 4 types...
     * INFORMATION_MESSAGE : you just want to inform the user about something.
     * WARNING_MESSAGE : you want to inform the user that something important happened.
     * ERROR_MESSAGE : you want to inform the user an error occured.
     * QUESTION_MESSAGE : you want to ask the user about something.
     * The function will return true always except for a message of type QUESTION_MESSAGE, in which
     * case the function may return false if the user pressed the 'No' button.
     * @param content The message you want to pass.
     **/
    bool message(Natron::MessageType type,const std::string & content) const;

    /**
     * @brief Use this function to post a persistent message to the user. It will be displayed on the
     * node's graphical interface and on any connected viewer. The message can be of 3 types...
     * INFORMATION_MESSAGE : you just want to inform the user about something.
     * WARNING_MESSAGE : you want to inform the user that something important happened.
     * ERROR_MESSAGE : you want to inform the user an error occured.
     * @param content The message you want to pass.
     **/
    void setPersistentMessage(Natron::MessageType type,const std::string & content);

    /**
     * @brief Clears any message posted previously by setPersistentMessage.
     **/
    void clearPersistentMessage();

    /**
     * @brief Does this effect supports tiling ?
     * http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#kOfxImageEffectPropSupportsTiles
     * If a clip or plugin does not support tiled images, then the host should supply
     * full RoD images to the effect whenever it fetches one.
     **/
    virtual bool supportsTiles() const
    {
        return false;
    }

    /**
     * @brief Does this effect supports multiresolution ?
     * http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#kOfxImageEffectPropSupportsMultiResolution
     * Multiple resolution images mean...
     * input and output images can be of any size
     * input and output images can be offset from the origin
     **/
    virtual bool supportsMultiResolution() const
    {
        return false;
    }

    /**
     * @brief Does this effect supports rendering at a different scale than 1 ?
     * There is no OFX property for this purpose. The only solution found for OFX is that if a render
     * or isIdentity with renderscale != 1 fails, the host retries with renderscale = 1 (and upscaled images).
     * If the renderScale support was not set, this throws an exception.
     **/
    bool supportsRenderScale() const;

    SupportsEnum supportsRenderScaleMaybe() const;

    /// should be set during effect initialization, but may also be set by the first getRegionOfDefinition with scale != 1 that succeeds
    void setSupportsRenderScaleMaybe(EffectInstance::SupportsEnum s) const;

    /**
     * @brief If this effect is a writer then the file path corresponding to the output images path will be fed
     * with the content of pattern.
     **/
    void setOutputFilesForWriter(const std::string & pattern);

    /**
     * @brief Constructs a new memory holder, with nBytes allocated. If the allocation failed, bad_alloc is thrown
     **/
    PluginMemory* newMemoryInstance(size_t nBytes) WARN_UNUSED_RETURN;

    /// used to count the memory used by a plugin
    /// Don't call these, they're called by PluginMemory automatically
    void registerPluginMemory(size_t nBytes);
    void unregisterPluginMemory(size_t nBytes);

    void addPluginMemoryPointer(PluginMemory* mem);
    void removePluginMemoryPointer(PluginMemory* mem);

    void clearPluginMemoryChunks();

    /**
     * @brief Called right away when the user first opens the settings panel of the node.
     * This is called after each params had its default value set.
     **/
    virtual void beginEditKnobs()
    {
    }

    virtual std::vector<std::string> supportedFileFormats() const
    {
        return std::vector<std::string>();
    }

    /**
     * @brief Called everytimes an input connection is changed
     **/
    virtual void onInputChanged(int /*inputNo*/)
    {
    }

    /**
     * @brief Same as onInputChanged but called once for many changes.
     **/
    virtual void onMultipleInputsChanged()
    {
    }

    /**
     * @brief Returns the current frame this effect is rendering depending
     * on the state of the renderer. If it is not actively rendering this
     * node then returns the timeline current time, otherwise the ongoing render
     * current frame is returned. This function uses thread storage to determine
     * exactly what writer is actively calling this function.
     *
     * WARNING: This is MUCH MORE expensive than calling getApp()->getTimeLine()->currentFrame()
     * so use with caution when you know you're on a render thread and during an action.
     **/
    int getThreadLocalRenderTime() const;


    /**
     * @brief If the plug-in calls timelineGoTo and we're during a render/instance changed action,
     * then all the knobs will retrieve the current time as being the one in the last render args thread-storage.
     * This function is here to update the last render args thread storage.
     **/
    void updateThreadLocalRenderTime(int time);
    
    /**
     * @brief If the caller thread is currently rendering an image, it will return a pointer to it
     * otherwise it will return NULL.
     * This function also returns the current renderWindow that is being rendered on that image
     * To be called exclusively on a render thread.
     *
     * WARNING: This call isexpensive and this function should not be called many times.
     **/
    bool getThreadLocalRenderedImage(boost::shared_ptr<Natron::Image>* image,RectI* renderWindow) const;

    /**
     * @brief Called when the associated node's hash has changed.
     * This is always called on the main-thread.
     **/
    void onNodeHashChanged(U64 hash);
    
protected:


    /**
     * @brief Called whenever a param changes. It calls the virtual
     * portion paramChangedByUser(...) and brackets the call by a begin/end if it was
     * not done already.
     **/
    virtual void knobChanged(KnobI* /*k*/,
                             Natron::ValueChangedReason /*reason*/,
                             int /*view*/,
                             SequenceTime /*time*/)
    {
    }

    virtual Natron::Status beginSequenceRender(SequenceTime /*first*/,
                                               SequenceTime /*last*/,
                                               SequenceTime /*step*/,
                                               bool /*interactive*/,
                                               const RenderScale & /*scale*/,
                                               bool /*isSequentialRender*/,
                                               bool /*isRenderResponseToUserInteraction*/,
                                               int /*view*/)
    {
        return Natron::StatOK;
    }

    virtual Natron::Status endSequenceRender(SequenceTime /*first*/,
                                             SequenceTime /*last*/,
                                             SequenceTime /*step*/,
                                             bool /*interactive*/,
                                             const RenderScale & /*scale*/,
                                             bool /*isSequentialRender*/,
                                             bool /*isRenderResponseToUserInteraction*/,
                                             int /*view*/)
    {
        return Natron::StatOK;
    }

public:

    ///Doesn't do anything, instead we overriden onKnobValueChanged_public
    virtual void onKnobValueChanged(KnobI* k, Natron::ValueChangedReason reason,SequenceTime time) OVERRIDE FINAL;
    Natron::Status beginSequenceRender_public(SequenceTime first, SequenceTime last,
                                              SequenceTime step, bool interactive, const RenderScale & scale,
                                              bool isSequentialRender, bool isRenderResponseToUserInteraction,
                                              int view);
    Natron::Status endSequenceRender_public(SequenceTime first, SequenceTime last,
                                            SequenceTime step, bool interactive, const RenderScale & scale,
                                            bool isSequentialRender, bool isRenderResponseToUserInteraction,
                                            int view);


    void drawOverlay_public(double scaleX,double scaleY);

    bool onOverlayPenDown_public(double scaleX,double scaleY,const QPointF & viewportPos, const QPointF & pos) WARN_UNUSED_RETURN;

    bool onOverlayPenMotion_public(double scaleX,double scaleY,const QPointF & viewportPos, const QPointF & pos) WARN_UNUSED_RETURN;

    bool onOverlayPenUp_public(double scaleX,double scaleY,const QPointF & viewportPos, const QPointF & pos) WARN_UNUSED_RETURN;

    bool onOverlayKeyDown_public(double scaleX,double scaleY,Natron::Key key,Natron::KeyboardModifiers modifiers) WARN_UNUSED_RETURN;

    bool onOverlayKeyUp_public(double scaleX,double scaleY,Natron::Key key,Natron::KeyboardModifiers modifiers) WARN_UNUSED_RETURN;

    bool onOverlayKeyRepeat_public(double scaleX,double scaleY,Natron::Key key,Natron::KeyboardModifiers modifiers) WARN_UNUSED_RETURN;

    bool onOverlayFocusGained_public(double scaleX,double scaleY) WARN_UNUSED_RETURN;

    bool onOverlayFocusLost_public(double scaleX,double scaleY) WARN_UNUSED_RETURN;

    bool isDoingInteractAction() const WARN_UNUSED_RETURN;

protected:

    /**
     * @brief Must be implemented to initialize any knob using the
     * KnobFactory.
     **/
    virtual void initializeKnobs() OVERRIDE
    {
    };


    /**
     * @brief This function is provided for means to copy more data than just the knobs from the live instance
     * to the render clones.
     **/
    virtual void cloneExtras()
    {
    }

    /* @brief Overlay support:
     * Just overload this function in your operator.
     * No need to include any OpenGL related header.
     * The coordinate space is  defined by the displayWindow
     * (i.e: (0,0) = bottomLeft and  width() and height() being
     * respectivly the width and height of the frame.)
     */
    virtual bool hasOverlay() const
    {
        return false;
    }

    virtual void drawOverlay(double /*scaleX*/,
                             double /*scaleY*/)
    {
    }

    virtual bool onOverlayPenDown(double /*scaleX*/,
                                  double /*scaleY*/,
                                  const QPointF & /*viewportPos*/,
                                  const QPointF & /*pos*/) WARN_UNUSED_RETURN
    {
        return false;
    }

    virtual bool onOverlayPenMotion(double /*scaleX*/,
                                    double /*scaleY*/,
                                    const QPointF & /*viewportPos*/,
                                    const QPointF & /*pos*/) WARN_UNUSED_RETURN
    {
        return false;
    }

    virtual bool onOverlayPenUp(double /*scaleX*/,
                                double /*scaleY*/,
                                const QPointF & /*viewportPos*/,
                                const QPointF & /*pos*/) WARN_UNUSED_RETURN
    {
        return false;
    }

    virtual bool onOverlayKeyDown(double /*scaleX*/,
                                  double /*scaleY*/,
                                  Natron::Key /*key*/,
                                  Natron::KeyboardModifiers /*modifiers*/) WARN_UNUSED_RETURN
    {
        return false;
    }

    virtual bool onOverlayKeyUp(double /*scaleX*/,
                                double /*scaleY*/,
                                Natron::Key /*key*/,
                                Natron::KeyboardModifiers /*modifiers*/) WARN_UNUSED_RETURN
    {
        return false;
    }

    virtual bool onOverlayKeyRepeat(double /*scaleX*/,
                                    double /*scaleY*/,
                                    Natron::Key /*key*/,
                                    Natron::KeyboardModifiers /*modifiers*/) WARN_UNUSED_RETURN
    {
        return false;
    }

    virtual bool onOverlayFocusGained(double /*scaleX*/,
                                      double /*scaleY*/) WARN_UNUSED_RETURN
    {
        return false;
    }

    virtual bool onOverlayFocusLost(double /*scaleX*/,
                                    double /*scaleY*/) WARN_UNUSED_RETURN
    {
        return false;
    }
   
    
    boost::shared_ptr<Node> _node; //< the node holding this effect

private:
    struct Implementation;
    boost::scoped_ptr<Implementation> _imp; // PIMPL: hide implementation details
    struct RenderArgs;
    enum RenderRoIStatus
    {
        eImageAlreadyRendered = 0, // there was nothing left to render
        eImageRendered, // we rendered what was missing
        eImageRenderFailed // render failed
    };

    /**
     * @brief The internal of renderRoI, mainly it calls render and handles the thread safety of the effect.
     * @param time The time at which to render
     * @param scale The scale at which to render
     * @param mipMapLevel Redundant with scale
     * @param view The view on which to render
     * @param renderWindow The rectangle to render of the image, in pixel coordinates (downscaled then)
     * @param cachedImgParams The parameters of the image to render as they are in the cache.
     * @param image This is the "full-scale" image, if the effect does support the render scale, then
     * image and downscaledImage are pointing to the SAME image.
     * @param downscaledImage If the effect doesn't support the render scale, then this is a pointer to the
     * downscaled image. If the effect doesn't support render scale then it will render in the "image" parameter
     * and then downscale the results into downscaledImage.
     * @param isSequentialRender True when the render is sequential
     * @param isRenderMadeInResponseToUserInteraction True when the render is made due to user interaction
     * @param byPassCache Cache look-ups have been already handled by renderRoI(...) but we pass it here because
     * we need to call renderRoI() on the input effects with this parameter too.
     * @param nodeHash The hash of the node used to render. This might no longer be equal to the value returned by
     * getHash() because the user might have changed something in the project (parameters...links..)
     * @param channelForAlpha This is passed here so that we can remember it later when converting the mask
     * which channel we wanted for the alpha channel.
     * @param renderFullScaleThenDownscale means that rendering should be done at full resolution and then
     * downscaled, because the plugin does not support render scale.
     * @returns True if the render call succeeded, false otherwise.
     **/
    RenderRoIStatus renderRoIInternal(SequenceTime time,
                                      const RenderScale & scale,
                                      unsigned int mipMapLevel,
                                      int view,
                                      const RectI & renderWindow, //< renderWindow in pixel coordinates
                                      const RectD & rod, //!< rod in canonical coordinates
                                      const boost::shared_ptr<ImageParams> & cachedImgParams,
                                      const boost::shared_ptr<Image> & image,
                                      const boost::shared_ptr<Image> & downscaledImage,
                                      bool isSequentialRender,
                                      bool isRenderMadeInResponseToUserInteraction,
                                      bool byPassCache,
                                      U64 nodeHash,
                                      int channelForAlpha,
                                      bool renderFullScaleThenDownscale);

    /**
     * @brief Must be implemented to evaluate a value change
     * made to a knob(e.g: force a new render).
     * @param knob[in] The knob whose value changed.
     **/
    void evaluate(KnobI* knob,bool isSignificant,Natron::ValueChangedReason reason) OVERRIDE;


    virtual void onAllKnobsSlaved(bool isSlave,KnobHolder* master) OVERRIDE FINAL;
    virtual void onKnobSlaved(const boost::shared_ptr<KnobI> & knob,int dimension,bool isSlave,KnobHolder* master) OVERRIDE FINAL;


    ///These are the image passed to the plug-in to render
    /// - fullscaleMappedImage is the fullscale image remapped to what the plugin can support (components/bitdepth)
    /// - downscaledMappedImage is the downscaled image remapped to what the plugin can support (components/bitdepth wise)
    /// - fullscaleMappedImage is pointing to "image" if the plug-in does support the renderscale, meaning we don't use it.
    /// - Similarily downscaledMappedImage is pointing to "downscaledImage" if the plug-in doesn't support the render scale.
    ///
    /// - renderMappedImage is what is given to the plug-in to render the image into,it is mapped to an image that the plug-in
    ///can render onto (good scale, good components, good bitdepth)
    ///
    /// These are the possible scenarios:
    /// - 1) Plugin doesn't need remapping and doesn't need downscaling
    ///    * We render in downscaledImage always, all image pointers point to it.
    /// - 2) Plugin doesn't need remapping but needs downscaling (doesn't support the renderscale)
    ///    * We render in fullScaleImage, fullscaleMappedImage points to it and then we downscale into downscaledImage.
    ///    * renderMappedImage points to fullScaleImage
    /// - 3) Plugin needs remapping (doesn't support requested components or bitdepth) but doesn't need downscaling
    ///    * renderMappedImage points to downscaledMappedImage
    ///    * We render in downscaledMappedImage and then convert back to downscaledImage with requested comps/bitdepth
    /// - 4) Plugin needs remapping and downscaling
    ///    * renderMappedImage points to fullScaleMappedImage
    ///    * We render in fullScaledMappedImage, then convert into "image" and then downscale into downscaledImage.
    Natron::Status tiledRenderingFunctor(const RenderArgs & args,
                                         bool renderFullScaleThenDownscale,
                                         const RectI & roi,
                                         const boost::shared_ptr<Natron::Image> & downscaledImage,
                                         const boost::shared_ptr<Natron::Image> & fullScaleImage,
                                         const boost::shared_ptr<Natron::Image> & downscaledMappedImage,
                                         const boost::shared_ptr<Natron::Image> & fullScaleMappedImage,
                                         const boost::shared_ptr<Natron::Image> & renderMappedImage);

    /**
     * @brief Returns the index of the input if inputEffect is a valid input connected to this effect, otherwise returns -1.
     **/
    int getInputNumber(Natron::EffectInstance* inputEffect) const;
};





/**
 * @typedef Any plug-in should have a static function called BuildEffect with the following signature.
 * It is used to build a new instance of an effect. Basically it should just call the constructor.
 **/
typedef Natron::EffectInstance* (*EffectBuilder)(boost::shared_ptr<Node>);


class OutputEffectInstance
    : public Natron::EffectInstance
{
    boost::shared_ptr<VideoEngine> _videoEngine;
    SequenceTime _writerCurrentFrame; /*!< for writers only: indicates the current frame
                                         It avoids snchronizing all viewers in the app to the render*/
    SequenceTime _writerFirstFrame;
    SequenceTime _writerLastFrame;
    bool _doingFullSequenceRender;
    mutable QMutex* _outputEffectDataLock;
    BlockingBackgroundRender* _renderController; //< pointer to a blocking renderer

public:

    OutputEffectInstance(boost::shared_ptr<Node> node);

    virtual ~OutputEffectInstance();

    virtual bool isOutput() const
    {
        return true;
    }

    boost::shared_ptr<VideoEngine> getVideoEngine() const
    {
        return _videoEngine;
    }

    /**
     * @brief Starts rendering of all the sequence available, from start to end.
     * This function is meant to be called for on-disk renderer only (i.e: not viewers).
     **/
    void renderFullSequence(BlockingBackgroundRender* renderController);

    void notifyRenderFinished();

    void updateTreeAndRender();

    void refreshAndContinueRender(bool forcePreview, bool abortRender);

    bool ifInfiniteclipRectToProjectDefault(RectD* rod) const;

    /**
     * @brief Returns the frame number this effect is currently rendering.
     * Note that this function can be used only for Writers or OpenFX writers,
     * it doesn't work with the Viewer.
     **/
    int getCurrentFrame() const;

    void setCurrentFrame(int f);

    int getFirstFrame() const;

    void setFirstFrame(int f);

    int getLastFrame() const;

    void setLastFrame(int f);

    void setDoingFullSequenceRender(bool b);

    bool isDoingFullSequenceRender() const;
};
} // Natron
#endif // NATRON_ENGINE_EFFECTINSTANCE_H_
