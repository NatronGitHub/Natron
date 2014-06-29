//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
*contact: immarespond at gmail dot com
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

class Hash64;
class RenderTree;
class VideoEngine;
class RenderTree;
class Format;
class OverlaySupport;
class PluginMemory;
class BlockingBackgroundRender;

namespace Natron{

class Node;
class Image;
class ImageParams;
/**
 * @brief This is the base class for visual effects.
 * A live instance is always living throughout the lifetime of a Node and other copies are
 * created on demand when a render is needed.
 **/
class EffectInstance : public NamedKnobHolder
{
public:
    
    typedef std::map<EffectInstance*,RectI> RoIMap;
    
    typedef std::map<int, std::vector<RangeD> > FramesNeededMap;
    
    struct RenderRoIArgs
    {
        SequenceTime time; //< the time at which to render
        RenderScale scale; //< the scale at which to render
        unsigned int mipMapLevel; //< the mipmap level (redundant with the scale, stored here to avoid refetching it everytimes)
        int view; //< the view to render
        RectI roi; //< the region of interest (in pixel coordinates) , watch out OpenFX action getRegionsOfInterest expects canonical coords!
        bool isSequentialRender; //< is this render part of a sequential render (playback or render on disk) ?
        bool isRenderUserInteraction; // is this render due to user interaction ? (parameter tweek)
        bool byPassCache; //< turn-off the cache ability to look-up existing images ? (false when a refresh is forced)
        const RectI* preComputedRoD; //<  pre-computed region of definition for this effect to speed-up the call to renderRoi
        Natron::ImageComponents components; //< the requested image components
        Natron::ImageBitDepth bitdepth; //< the requested bit depth
        int channelForAlpha; //< if this is a mask this is from this channel that we will fetch the mask
        
        RenderRoIArgs() {}
        
        RenderRoIArgs(SequenceTime time_,
                      RenderScale scale_,
                      unsigned int mipMapLevel_,
                      int view_,
                      RectI roi_,
                      bool isSequentialRender_,
                      bool isRenderUserInteraction_,
                      bool byPassCache_,
                      const RectI* preComputedRoD_,
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
    
public:
    
    enum CachePolicy { ALWAYS_CACHE = 0 , NEVER_CACHE };
    
    /**
     * @brief Constructor used once for each node created. Its purpose is to create the "live instance".
     * You shouldn't do any heavy processing here nor lengthy initialization as the constructor is often
     * called just to be able to call a few virtuals fonctions.
     * The constructor is always called by the main thread of the application.
     **/
    explicit EffectInstance(boost::shared_ptr<Node> node);
    
    virtual ~EffectInstance();
    
    /**
     * @brief Returns a pointer to the node holding this effect.
     **/
    boost::shared_ptr<Node> getNode() const WARN_UNUSED_RETURN { return _node; }
    
    /**
     * @brief Returns the "real" hash of the node synchronized with the gui state
     **/
    U64 hash() const WARN_UNUSED_RETURN;
    
    /**
     * @brief Returns the hash the node had at the start of renderRoI. This will return the same value
     * at any time during the same render call. 
     * @returns This function returns true if case of success, false otherwise.
     **/
    bool getRenderHash(U64* hash) const WARN_UNUSED_RETURN;
    
    U64 knobsAge() const WARN_UNUSED_RETURN;
    
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
    const std::string& getName() const WARN_UNUSED_RETURN;
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
     * Cannot be called by another thread than the application's main thread.
     **/
    EffectInstance* input(int n) const WARN_UNUSED_RETURN;
  
    /**
     * @brief Returns input n. It might be NULL if the input is not connected.
     **/
    EffectInstance* input_other_thread(int n) const WARN_UNUSED_RETURN;
    
    /**
     * @brief Forwarded to the node holding the effect
     **/
    bool hasOutputConnected() const WARN_UNUSED_RETURN;
    
    /**
     * @brief Must return the plugin's major version.
     **/
    virtual int majorVersion() const WARN_UNUSED_RETURN = 0;
    
    /**
     * @brief Must return the plugin's minor version.
     **/
    virtual int minorVersion() const WARN_UNUSED_RETURN = 0;
    
    /**
     * @brief Is this node an input node ? An input node means
     * it has no input.
     **/
    virtual bool isGenerator() const WARN_UNUSED_RETURN { return false; }
    
    /**
     * @brief Is the node a reader ?
     **/
    virtual bool isReader() const WARN_UNUSED_RETURN { return false; }
    
    /**
     * @brief Is the node a writer ?
     **/
    virtual bool isWriter() const WARN_UNUSED_RETURN { return false; }
    
    /**
     * @brief Is this node an output node ? An output node means
     * it has no output.
     **/
    virtual bool isOutput() const WARN_UNUSED_RETURN { return false; }
    
    
    /**
     * @brief Returns true if the node is capable of generating
     * data and process data on the input as well
     **/
    virtual bool isGeneratorAndFilter() const WARN_UNUSED_RETURN { return false; }
    
    /**
     * @brief Is this node an OpenFX node?
     **/
    virtual bool isOpenFX() const WARN_UNUSED_RETURN { return false; }
    
    /**
     * @brief How many input can we have at most. (i.e: how many input arrows)
     **/
    virtual int maximumInputs() const WARN_UNUSED_RETURN = 0;
    
    /**
     * @brief Is inputNb optional ? In which case the render can be made without it.
     **/
    virtual bool isInputOptional(int inputNb) const WARN_UNUSED_RETURN = 0;
    
    /**
     * @brief Is inputNb a mask ? In which case the effect will have an additionnal mask parameter.
     **/
    virtual bool isInputMask(int /*inputNb*/) const WARN_UNUSED_RETURN { return false; };
    
    /**
     * @brief Is the input a roto brush ?
     **/
    virtual bool isInputRotoBrush(int /*inputNb*/) const WARN_UNUSED_RETURN { return false; }
    
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
    virtual Natron::ImagePremultiplication getOutputPremultiplication() const { return Natron::ImagePremultiplied; }
    
    /**
     * @brief Can be derived to give a more meaningful label to the input 'inputNb'
     **/
    virtual std::string inputLabel(int inputNb) const WARN_UNUSED_RETURN;
    
    /**
     * @brief Must be implemented to give the plugin internal id(i.e: net.sf.openfx:invertPlugin)
     **/
    virtual std::string pluginID() const WARN_UNUSED_RETURN = 0;
    
    /**
     * @brief Must be implemented to give the plugin a label that will be used by the graphical
     * user interface.
     **/
    virtual std::string pluginLabel() const WARN_UNUSED_RETURN = 0;
    
    /**
     * @brief The grouping of the plug-in. For instance Views/Stereo/MyStuff
     * Each string being one level of the grouping. The first one being the name
     * of one group that will appear in the user interface.
     **/
    virtual void pluginGrouping(std::list<std::string>* grouping) const = 0;
    
    /**
     * @brief Must be implemented to give a desription of the effect that this node does. This is typically
     * what you'll see displayed when the user clicks the '?' button on the node's panel in the user interface.
     **/
    virtual std::string description() const WARN_UNUSED_RETURN = 0;
    
    
    /**
     * @bried Returns the effect render order preferences: 
     * EFFECT_NOT_SEQUENTIAL: The effect does not need to be run in a sequential order
     * EFFECT_ONLY_SEQUENTIAL: The effect can only be run in a sequential order (i.e like the background render would do)
     * EFFECT_PREFER_SEQUENTIAL: This indicates that the effect would work better by rendering sequential. This is merely
     * a hint to Natron but for now we just consider it as EFFECT_NOT_SEQUENTIAL.
     **/
    virtual Natron::SequentialPreference getSequentialPreference() const { return Natron::EFFECT_NOT_SEQUENTIAL; }
    
    
    /**
     * @brief Renders the image at the given time,scale and for the given view & render window.
     * @param hashUsed, if not null, it will be set to the hash of this effect used to render.
     **/
    boost::shared_ptr<Image> renderRoI(const RenderRoIArgs& args,U64* hashUsed = NULL) WARN_UNUSED_RETURN;
    
    /**
     * @brief Same as renderRoI(SequenceTime,RenderScale,int,RectI,bool) but takes in parameter
     * the outputImage where to render instead. This is used by the Viewer which already did
     * a cache look-up for optimization purposes.
     **/
    void renderRoI(SequenceTime time,const RenderScale& scale,unsigned int mipMapLevel,
                   int view,const RectI& renderWindow,
                   const boost::shared_ptr<const ImageParams>& cachedImgParams,
                   const boost::shared_ptr<Image>& image,
                   const boost::shared_ptr<Image>& downscaledImage,
                   bool isSequentialRender,
                   bool isRenderMadeInResponseToUserInteraction,
                   bool byPassCache,
                   U64 nodeHash);
    
    /**
     * @breif Don't override this one, override onKnobValueChanged instead.
     **/
    virtual void onKnobValueChanged_public(KnobI* k,Natron::ValueChangedReason reason) OVERRIDE FINAL;

protected:
    /**
     * @brief Must fill the image 'output' for the region of interest 'roi' at the given time and
     * at the given scale.
     * Pre-condition: render() has been called for all inputs so the portion of the image contained
     * in output corresponding to the roi is valid.
     * Note that this function can be called concurrently for the same output image but with different
     * rois, depending on the threading-affinity of the plug-in.
     **/
    virtual Natron::Status render(SequenceTime /*time*/, RenderScale /*scale*/, const RectI& /*roi*/, int /*view*/,
                                  bool /*isSequentialRender*/,bool /*isRenderResponseToUserInteraction*/,
                                  boost::shared_ptr<Natron::Image> /*output*/) WARN_UNUSED_RETURN { return Natron::StatOK; }
    
public:
    
     Natron::Status render_public(SequenceTime time, RenderScale scale, const RectI& roi, int view,
                                      bool isSequentialRender,bool isRenderResponseToUserInteraction,
                                      boost::shared_ptr<Natron::Image> output) WARN_UNUSED_RETURN;
    
protected:
    /**
     * @brief Can be overloaded to indicates whether the effect is an identity, i.e it doesn't produce
     * any change in output.
     * @param time The time of interest
     * @param scale The scale of interest
     * @param roi The region of interest
     * @param view The view we 're interested in
     * @param inputTime[out] the input time to which this plugin is identity of
     * @param inputNb[out] the input number of the effect that is identity of.
     * The special value of -2 indicates that the plugin is identity of itself at another time
     **/
    virtual bool isIdentity(SequenceTime /*time*/,RenderScale /*scale*/,const RectI& /*roi*/,
                            int /*view*/,SequenceTime* /*inputTime*/,int* /*inputNb*/) WARN_UNUSED_RETURN { return false; }
    
public:
    
    bool isIdentity_public(SequenceTime time,RenderScale scale,const RectI& roi,
                           int view,SequenceTime* inputTime,int* inputNb) WARN_UNUSED_RETURN;
    
    enum RenderSafety{UNSAFE = 0,INSTANCE_SAFE = 1,FULLY_SAFE = 2,FULLY_SAFE_FRAME = 3};
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
    virtual bool shouldRenderedDataBePersistent() const WARN_UNUSED_RETURN {return false;}
    
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
     */
    boost::shared_ptr<Image> getImage(int inputNb,
                                      SequenceTime time,
                                      RenderScale scale,
                                      int view,
                                      const RectD *optionalBounds,
                                      Natron::ImageComponents comp,
                                      Natron::ImageBitDepth depth,
                                      bool dontUpscale) WARN_UNUSED_RETURN;
    
protected:
    
    /**
     * @brief Can be derived to get the region that the plugin is capable of filling.
     * This is meaningful for plugins that generate images or transform images.
     * By default it returns in rod the union of all inputs RoD and StatReplyDefault is returned.
     * @param isProjectFormat[out] If set to true, then rod is taken to be equal to the current project format.
     * In case of failure the plugin should return StatFailed.
     **/
    virtual Natron::Status getRegionOfDefinition(SequenceTime time,const RenderScale& scale,int view,RectI* rod) WARN_UNUSED_RETURN;
    
    /**
     * @brief If the rod is infinite, returns the union of all connected inputs. If there's no input this returns the
     * project format.
     * @returns true if the rod is set to the project format.
     **/
    bool ifInfiniteApplyHeuristic(SequenceTime time,const RenderScale& scale, int view,RectI* rod) const;
    
    /**
     * @brief Can be derived to indicate for each input node what is the region of interest
     * of the node at time 'time' and render scale 'scale' given a render window.
     * For exemple a blur plugin would specify what it needs
     * from inputs in order to do a blur taking into account the size of the blurring kernel.
     * By default, it returns renderWindow for each input.
     **/
    virtual RoIMap getRegionOfInterest(SequenceTime time,RenderScale scale,const RectI& outputRoD,
                                       const RectI& renderWindow,int view) WARN_UNUSED_RETURN;
    
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
    
    Natron::Status getRegionOfDefinition_public(SequenceTime time,const RenderScale& scale,int view,
                                         RectI* rod,bool* isProjectFormat) WARN_UNUSED_RETURN;
    
    RoIMap getRegionOfInterest_public(SequenceTime time,RenderScale scale,
                                      const RectI& outputRoD,
                                      const RectI& renderWindow,int view) WARN_UNUSED_RETURN;
    
    FramesNeededMap getFramesNeeded_public(SequenceTime time) WARN_UNUSED_RETURN;
    
    void getFrameRange_public(SequenceTime *first,SequenceTime *last);
    
    /**
     * @brief Override to initialize the overlay interact. It is called only on the 
     * live instance.
     **/
    virtual void initializeOverlayInteract(){}
    
   
    virtual void setCurrentViewportForOverlays(OverlaySupport* /*viewport*/) {}

    /**
     * @brief Overload this and return true if your operator should dislay a preview image by default.
     **/
    virtual bool makePreviewByDefault() const WARN_UNUSED_RETURN {return false;}

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
    virtual void beginKnobsValuesChanged(Natron::ValueChangedReason /*reason*/) OVERRIDE {}
    
    /**
     * @brief Used to bracket a series of call to onKnobValueChanged(...) in case many complex changes are done
     * at once. If not called, onKnobValueChanged() will call automatically bracket its call be a begin/end
     * but this can lead to worse performance. You can overload this to make all changes to params at once.
     **/
    virtual void endKnobsValuesChanged(Natron::ValueChangedReason /*reason*/) OVERRIDE {}
    
    
    /**
     * @brief Can be overloaded to clear any cache the plugin might be
     * handling on his side.
     **/
    virtual void purgeCaches(){};
    
    void clearLastRenderedImage();
    
    /**
     * @brief Can be overloaded to indicate whether a plug-in wants to cache
     * a frame rendered or not.
     **/
    virtual CachePolicy getCachePolicy(SequenceTime /*time*/) const { return ALWAYS_CACHE; }
    
    /**
     * @brief When called, if the node holding this effect  is connected to any
     * viewer, it will render again the current frame. By default you don't have to
     * call this yourself as this is called automatically on a knob value changed,
     * but this is provided as a way to force things.
     **/
    void requestRender() { evaluate(NULL,true); }

    
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
    bool message(Natron::MessageType type,const std::string& content) const;
    
    /**
     * @brief Use this function to post a persistent message to the user. It will be displayed on the
     * node's graphical interface and on any connected viewer. The message can be of 3 types...
     * INFORMATION_MESSAGE : you just want to inform the user about something.
     * WARNING_MESSAGE : you want to inform the user that something important happened.
     * ERROR_MESSAGE : you want to inform the user an error occured.
     * @param content The message you want to pass.
     **/
    void setPersistentMessage(Natron::MessageType type,const std::string& content);
    
    /**
     * @brief Clears any message posted previously by setPersistentMessage.
     **/
    void clearPersistentMessage();
    
    /**
     * @brief Does this effect supports tiling ?
     **/
    virtual bool supportsTiles() const { return false; }
    
    /**
     * @brief Does this effect supports rendering at a different scale than 1 ?
     **/
    virtual bool supportsRenderScale() const { return false; }
    
    /**
     * @brief If this effect is a reader then the file path corresponding to the input images path will be fed
     * with the content of files. Note that an exception is thrown if the file knob does not support image sequences
     * but you attempt to feed-in several files.
     **/
    void setInputFilesForReader(const std::vector<std::string>& files);
    
    /**
     * @brief If this effect is a writer then the file path corresponding to the output images path will be fed
     * with the content of pattern.
     **/
    void setOutputFilesForWriter(const std::string& pattern);
    
    /**
     * @brief Constructs a new memory holder, with nBytes allocated. If the allocation failed, bad_alloc is thrown
     **/
    PluginMemory* newMemoryInstance(size_t nBytes) WARN_UNUSED_RETURN;
    
    /// used to count the memory used by a plugin
    /// Don't call these, they're called by PluginMemory automatically
    void registerPluginMemory(size_t nBytes);
    void unregisterPluginMemory(size_t nBytes);
    
    /**
     * @brief Called right away when the user first opens the settings panel of the node.
     * This is called after each params had its default value set.
     **/
    virtual void beginEditKnobs() {}
    
    virtual std::vector<std::string> supportedFileFormats() const { return std::vector<std::string>(); }
    
    /**
     * @brief Called everytimes an input connection is changed
     **/
    virtual void onInputChanged(int /*inputNo*/) {}
    
    
    /**
     * @brief Same as onInputChanged but called once for many changes.
     **/
    virtual void onMultipleInputsChanged() {}
    
    /**
     * @brief Returns the current frame this effect is rendering depending 
     * on the state of the renderer. If no writer is actively rendering this
     * node then the timeline current time is returned, otherwise the writer's
     * current frame is returned. This function uses thread storage to determine
     * exactly what writer is actively calling this function.
     **/
    int getCurrentFrameRecursive() const;
    
protected:
    
 
    /**
     * @brief Called whenever a param changes. It calls the virtual
     * portion paramChangedByUser(...) and brackets the call by a begin/end if it was
     * not done already.
     **/
    virtual void knobChanged(KnobI* /*k*/, Natron::ValueChangedReason /*reason*/,const RectI& /*rod*/) {}
    
    
    virtual Natron::Status beginSequenceRender(SequenceTime /*first*/,SequenceTime /*last*/,
                                     SequenceTime /*step*/,bool /*interactive*/,RenderScale /*scale*/,
                                     bool /*isSequentialRender*/,bool /*isRenderResponseToUserInteraction*/,
                                               int /*view*/) { return Natron::StatOK; }
    
    virtual Natron::Status endSequenceRender(SequenceTime /*first*/,SequenceTime /*last*/,
                                   SequenceTime /*step*/,bool /*interactive*/,RenderScale /*scale*/,
                                   bool /*isSequentialRender*/,bool /*isRenderResponseToUserInteraction*/,
                                             int /*view*/) { return Natron::StatOK;}
public:
    
    virtual void onKnobValueChanged(KnobI* k, Natron::ValueChangedReason reason) OVERRIDE FINAL;
    
    
    Natron::Status beginSequenceRender_public(SequenceTime first,SequenceTime last,
                                     SequenceTime step,bool interactive,RenderScale scale,
                                     bool isSequentialRender,bool isRenderResponseToUserInteraction,
                                    int view);
    
    Natron::Status endSequenceRender_public(SequenceTime first,SequenceTime last,
                                   SequenceTime step,bool interactive,RenderScale scale,
                                   bool isSequentialRender,bool isRenderResponseToUserInteraction,
                                  int view);
    
    
    void drawOverlay_public(double scaleX,double scaleY);
    
    bool onOverlayPenDown_public(double scaleX,double scaleY,const QPointF& viewportPos, const QPointF& pos) WARN_UNUSED_RETURN;
    
    bool onOverlayPenMotion_public(double scaleX,double scaleY,const QPointF& viewportPos, const QPointF& pos) WARN_UNUSED_RETURN ;
    
    bool onOverlayPenUp_public(double scaleX,double scaleY,const QPointF& viewportPos, const QPointF& pos) WARN_UNUSED_RETURN;
    
    bool onOverlayKeyDown_public(double scaleX,double scaleY,Natron::Key key,Natron::KeyboardModifiers modifiers) WARN_UNUSED_RETURN ;
    
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
    virtual void initializeKnobs() OVERRIDE {};
    
   
    
    /**
     * @brief This function is provided for means to copy more data than just the knobs from the live instance
     * to the render clones.
     **/
    virtual void cloneExtras(){}
    
    /* @brief Overlay support:
     * Just overload this function in your operator.
     * No need to include any OpenGL related header.
     * The coordinate space is  defined by the displayWindow
     * (i.e: (0,0) = bottomLeft and  width() and height() being
     * respectivly the width and height of the frame.)
     */
    virtual void drawOverlay(double /*scaleX*/,double /*scaleY*/,const RectI& /*outputRoD*/){}
    
    virtual bool onOverlayPenDown(double /*scaleX*/,double /*scaleY*/,
                                  const QPointF& /*viewportPos*/, const QPointF& /*pos*/
                                  ,const RectI& /*outputRoD*/) WARN_UNUSED_RETURN { return false; }
    
    virtual bool onOverlayPenMotion(double /*scaleX*/,double /*scaleY*/,
                                    const QPointF& /*viewportPos*/, const QPointF& /*pos*/
                                    ,const RectI& /*outputRoD*/) WARN_UNUSED_RETURN { return false; }
    
    virtual bool onOverlayPenUp(double /*scaleX*/,double /*scaleY*/,
                                const QPointF& /*viewportPos*/, const QPointF& /*pos*/
                                ,const RectI& /*outputRoD*/) WARN_UNUSED_RETURN { return false; }
    
    virtual bool onOverlayKeyDown(double /*scaleX*/,double /*scaleY*/,
                                  Natron::Key /*key*/,Natron::KeyboardModifiers /*modifiers*/
                                  ,const RectI& /*outputRoD*/) WARN_UNUSED_RETURN { return false; }
    
    virtual bool onOverlayKeyUp(double /*scaleX*/,double /*scaleY*/,
                                Natron::Key /*key*/,Natron::KeyboardModifiers /*modifiers*/
                                ,const RectI& /*outputRoD*/) WARN_UNUSED_RETURN { return false; }
    
    virtual bool onOverlayKeyRepeat(double /*scaleX*/,double /*scaleY*/,
                                    Natron::Key /*key*/,Natron::KeyboardModifiers /*modifiers*/
                                    ,const RectI& /*outputRoD*/) WARN_UNUSED_RETURN { return false; }
    
    virtual bool onOverlayFocusGained(double /*scaleX*/,double /*scaleY*/
                                      ,const RectI& /*outputRoD*/) WARN_UNUSED_RETURN { return false; }
    
    virtual bool onOverlayFocusLost(double /*scaleX*/,double /*scaleY*/
                                    ,const RectI& /*outputRoD*/) WARN_UNUSED_RETURN { return false; }
    
    /**
     * @brief Retrieves the current time, the view rendered by the attached viewer , the mipmaplevel of the attached
     * viewer and the rod of the output.
     **/
    void getClipThreadStorageData(SequenceTime& time,int &view,unsigned int& mipMapLevel,RectI& outputRoD) ;
    
    boost::shared_ptr<Node> _node; //< the node holding this effect

private:
    struct Implementation;
    boost::scoped_ptr<Implementation> _imp; // PIMPL: hide implementation details
    
    struct RenderArgs;
    
    
    enum RenderRoIStatus {
        eImageAlreadyRendered = 0, // there was nothing left to render
        eImageRendered, // we rendered what was missing
        eImageRenderFailed // render failed
    };
    /**
     * @brief The internal of renderRoI, mainly it calls render and handles the thread safety of the effect.
     * @returns True if the render call succeeded, false otherwise.
     **/
    RenderRoIStatus renderRoIInternal(SequenceTime time,const RenderScale& scale,unsigned int mipMapLevel,
                                      int view,const RectI& renderWindow, //< renderWindow in pixel coordinates
                                      const boost::shared_ptr<const ImageParams>& cachedImgParams,
                                      const boost::shared_ptr<Image>& image,
                                      const boost::shared_ptr<Image>& downscaledImage,
                                      bool isSequentialRender,
                                      bool isRenderMadeInResponseToUserInteraction,
                                      bool byPassCache,
                                      U64 nodeHash,
                                      int channelForAlpha);
    
    /**
     * @brief Must be implemented to evaluate a value change
     * made to a knob(e.g: force a new render).
     * @param knob[in] The knob whose value changed.
     **/
    void evaluate(KnobI* knob,bool isSignificant) OVERRIDE;

    
    virtual void onSlaveStateChanged(bool isSlave,KnobHolder* master) OVERRIDE FINAL;

    
    
    Natron::Status tiledRenderingFunctor(const RenderArgs& args,
                                         const RectI& roi,
                                         boost::shared_ptr<Natron::Image> downscaledOutput,
                                         boost::shared_ptr<Natron::Image> fullScaleOutput,
                                         boost::shared_ptr<Natron::Image> downscaledMappedOutput,
                                         boost::shared_ptr<Natron::Image> fullScaleMappedOutput);
    
    /**
     * @brief Returns the index of the input if inputEffect is a valid input connected to this effect, otherwise returns -1.
     **/
    int getInputNumber(Natron::EffectInstance* inputEffect) const;
    
};
    
    
/**
* @brief This object locks an image for writing (and waits until it can lock it) and releases
* the lock when it is destroyed.
**/
class OutputImageLocker {
        
    Natron::Node* n;
    boost::shared_ptr<Natron::Image> img;
public:
        
    OutputImageLocker(Natron::Node* node,const boost::shared_ptr<Natron::Image>& image);
        
    ~OutputImageLocker();
    
};
    

/**
 * @typedef Any plug-in should have a static function called BuildEffect with the following signature.
 * It is used to build a new instance of an effect. Basically it should just call the constructor.
 **/
typedef Natron::EffectInstance* (*EffectBuilder)(boost::shared_ptr<Node>);


class OutputEffectInstance : public Natron::EffectInstance {

    

    boost::shared_ptr<VideoEngine> _videoEngine;
    SequenceTime _writerCurrentFrame;/*!< for writers only: indicates the current frame
                             It avoids snchronizing all viewers in the app to the render*/
    SequenceTime _writerFirstFrame;
    SequenceTime _writerLastFrame;
    bool _doingFullSequenceRender;
    mutable QMutex* _outputEffectDataLock;
    
    BlockingBackgroundRender* _renderController; //< pointer to a blocking renderer
public:

    OutputEffectInstance(boost::shared_ptr<Node> node);
    
    virtual ~OutputEffectInstance();

    virtual bool isOutput() const { return true; }

    boost::shared_ptr<VideoEngine> getVideoEngine() const {return _videoEngine;}
    
    /**
     * @brief Starts rendering of all the sequence available, from start to end.
     * This function is meant to be called for on-disk renderer only (i.e: not viewers).
     **/
    void renderFullSequence(BlockingBackgroundRender* renderController);
    
    void notifyRenderFinished();

    void updateTreeAndRender();

    void refreshAndContinueRender(bool forcePreview);

    bool ifInfiniteclipRectToProjectDefault(RectI* rod) const;

    /**
     * @brief Returns the frame number this effect is currently rendering.
     * Note that this function can be used only for Writers or OpenFX writers,
     * it doesn't work with the Viewer.
     **/
    int getCurrentFrame() const ;
    
    void setCurrentFrame(int f);
    
    int getFirstFrame() const ;
    
    void setFirstFrame(int f);
    
    int getLastFrame() const;
    
    void setLastFrame(int f) ;

    void setDoingFullSequenceRender(bool b);
    
    bool isDoingFullSequenceRender() const;
    
};


} // Natron
#endif // NATRON_ENGINE_EFFECTINSTANCE_H_
