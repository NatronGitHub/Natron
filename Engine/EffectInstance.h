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

#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>

#include "Global/GlobalDefines.h"
#include "Global/KeySymbols.h"
#include "Engine/Knob.h" // for KnobHolder

class RectI;
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
class EffectInstance : public KnobHolder
{
public:
    
    typedef std::map<EffectInstance*,RectI> RoIMap;
    
    typedef std::map<int, std::vector<RangeD> > FramesNeededMap;
    
public:
    
    enum CachePolicy { ALWAYS_CACHE = 0 , NEVER_CACHE };
    
    /**
     * @brief Constructor used once for each node created. Its purpose is to create the "live instance".
     * You shouldn't do any heavy processing here nor lengthy initialization as the constructor is often
     * called just to be able to call a few virtuals fonctions.
     * The constructor is always called by the main thread of the application.
     **/
    explicit EffectInstance(Node* node);
    
    virtual ~EffectInstance();
    
    /**
     * @brief Returns a pointer to the node holding this effect.
     **/
    Node* getNode() const WARN_UNUSED_RETURN { return _node; }
    
    U64 hash() const WARN_UNUSED_RETURN;
    
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
     * @brief Must be implemented to give a desription of the effect that this node does. This is typically
     * what you'll see displayed when the user clicks the '?' button on the node's panel in the user interface.
     **/
    virtual std::string description() const WARN_UNUSED_RETURN = 0;
    
    /**
     * @brief Renders the image at the given time,scale and for the given view & render window.
     * Pre-condition: preProcess must have been called.
     **/
    boost::shared_ptr<Image> renderRoI(SequenceTime time,RenderScale scale,
                                       int view,const RectI& renderWindow,
                                       bool isSequentialRender,
                                       bool isRenderMadeInResponseToUserInteraction,
                                       bool byPassCache = false,const RectI* preComputedRoD = NULL) WARN_UNUSED_RETURN;
    
    /**
     * @brief Same as renderRoI(SequenceTime,RenderScale,int,RectI,bool) but takes in parameter
     * the outputImage where to render instead. This is used by the Viewer which already did
     * a cache look-up for optimization purposes.
     **/
    void renderRoI(SequenceTime time,RenderScale scale,
                   int view,const RectI& renderWindow,
                   const boost::shared_ptr<const ImageParams>& cachedImgParams,
                   const boost::shared_ptr<Image>& image,
                   bool isSequentialRender,
                   bool isRenderMadeInResponseToUserInteraction,
                   bool byPassCache = false);


    /**
     * @brief Returns a pointer to the image being rendered currently by renderRoI if any.
     **/
    boost::shared_ptr<Natron::Image> getImageBeingRendered(SequenceTime time,int view) const WARN_UNUSED_RETURN;
    
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
    
    /**
     * @brief Can be overloaded to indicates whether the effect is an identity, i.e it doesn't produce
     * any change in output.
     * @param time The time of interest
     * @param scale The scale of interest
     * @param roi The region of interest
     * @param view The view we 're interested in
     * @param inputTime[out] the input time to which this plugin is identity of
     * @param inputNb[out] the input number of the effect that is identity of
     **/
    virtual bool isIdentity(SequenceTime /*time*/,RenderScale /*scale*/,const RectI& /*roi*/,
                            int /*view*/,SequenceTime* /*inputTime*/,int* /*inputNb*/) WARN_UNUSED_RETURN { return false; }
    
    enum RenderSafety{UNSAFE = 0,INSTANCE_SAFE = 1,FULLY_SAFE = 2,FULLY_SAFE_FRAME = 3};
    /**
     * @brief Indicates how many simultaneous renders the plugin can deal with.
     * RenderSafety::UNSAFE - indicating that only a single 'render' call can be made at any time amoung all instances,
     * RenderSafety::INSTANCE_SAFE - indicating that any instance can have a single 'render' call at any one time,
     * RenderSafety::FULLY_SAFE - indicating that any instance of a plugin can have multiple renders running simultaneously
     
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
     */
    boost::shared_ptr<Image> getImage(int inputNb,SequenceTime time,RenderScale scale,int view) WARN_UNUSED_RETURN;
    
    /**
     * @brief Can be derived to get the region that the plugin is capable of filling.
     * This is meaningful for plugins that generate images or transform images.
     * By default it returns in rod the union of all inputs RoD and StatReplyDefault is returned.
     * @param isProjectFormat[out] If set to true, then rod is taken to be equal to the current project format.
     * In case of failure the plugin should return StatFailed.
     **/
    virtual Natron::Status getRegionOfDefinition(SequenceTime time,RectI* rod,bool* isProjectFormat) WARN_UNUSED_RETURN;
    
    
    /**
     * @brief Can be derived to indicate for each input node what is the region of interest
     * of the node at time 'time' and render scale 'scale' given a render window.
     * For exemple a blur plugin would specify what it needs
     * from inputs in order to do a blur taking into account the size of the blurring kernel.
     * By default, it returns renderWindow for each input.
     **/
    virtual RoIMap getRegionOfInterest(SequenceTime time,RenderScale scale,const RectI& renderWindow) WARN_UNUSED_RETURN;
    
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
    
    /**
     * @brief Override to initialize the overlay interact. It is called only on the 
     * live instance.
     **/
    virtual void initializeOverlayInteract(){}
    
    /* @brief Overlay support:
     * Just overload this function in your operator.
     * No need to include any OpenGL related header.
     * The coordinate space is  defined by the displayWindow
     * (i.e: (0,0) = bottomLeft and  width() and height() being
     * respectivly the width and height of the frame.)
     */
    virtual void drawOverlay(){}
    
    virtual bool onOverlayPenDown(const QPointF& /*viewportPos*/, const QPointF& /*pos*/) WARN_UNUSED_RETURN { return false; }
    
    virtual bool onOverlayPenMotion(const QPointF& /*viewportPos*/, const QPointF& /*pos*/) WARN_UNUSED_RETURN { return false; }
    
    virtual bool onOverlayPenUp(const QPointF& /*viewportPos*/, const QPointF& /*pos*/) WARN_UNUSED_RETURN { return false; }
    
    virtual bool onOverlayKeyDown(Natron::Key /*key*/,Natron::KeyboardModifiers /*modifiers*/) WARN_UNUSED_RETURN { return false; }
    
    virtual bool onOverlayKeyUp(Natron::Key /*key*/,Natron::KeyboardModifiers /*modifiers*/) WARN_UNUSED_RETURN { return false; }
    
    virtual bool onOverlayKeyRepeat(Natron::Key /*key*/,Natron::KeyboardModifiers /*modifiers*/) WARN_UNUSED_RETURN { return false; }
    
    virtual bool onOverlayFocusGained() WARN_UNUSED_RETURN { return false; }
    
    virtual bool onOverlayFocusLost() WARN_UNUSED_RETURN { return false; }
    
    virtual void setCurrentViewportForOverlays(OverlaySupport* /*viewport*/) {}

    /**
     * @brief Overload this and return true if your operator should dislay a preview image by default.
     **/
    virtual bool makePreviewByDefault() const WARN_UNUSED_RETURN {return false;}
    
    bool isPreviewEnabled() const WARN_UNUSED_RETURN;
    
    /**
     * @brief Called by the GUI when the user wants to activate preview image for this effect.
     **/
    void togglePreview();
    
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
     * @brief Called whenever a param changes. It calls the virtual
     * portion paramChangedByUser(...) and brackets the call by a begin/end if it was
     * not done already.
     **/
    virtual void onKnobValueChanged(Knob* /*k*/, Natron::ValueChangedReason /*reason*/) OVERRIDE {}
    
    /**
     * @brief Can be overloaded to clear any cache the plugin might be
     * handling on his side.
     **/
    virtual void purgeCaches(){};
    
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
     * @brief If this effect is a reader then the file path corresponding to the input images path will be fed
     * with the content of files. Note that an exception is thrown if the file knob does not support image sequences
     * but you attempt to feed-in several files.
     **/
    void setInputFilesForReader(const QStringList& files);
    
    /**
     * @brief If this effect is a writer then the file path corresponding to the output images path will be fed
     * with the content of pattern.
     **/
    void setOutputFilesForWriter(const QString& pattern);
    
    /**
     * @brief Constructs a new memory holder, with nBytes allocated. If the allocation failed, bad_alloc is thrown
     **/
    PluginMemory* newMemoryInstance(size_t nBytes) WARN_UNUSED_RETURN;
    
    /// used to count the memory used by a plugin
    /// Don't call these, they're called by PluginMemory automatically
    void registerPluginMemory(size_t nBytes);
    void unregisterPluginMemory(size_t nBytes);
    
    /**
     * @brief Called everytimes an input connection is changed
     **/
    virtual void onInputChanged(int /*inputNo*/) {}
    
    
    /**
     * @brief Same as onInputChanged but called once for many changes.
     **/
    virtual void onMultipleInputsChanged() {}
    
    /**
     * @brief Called right away when the user first opens the settings panel of the node.
     * This is called after each params had its default value set.
     **/
    virtual void beginEditKnobs() {}
    
    virtual std::vector<std::string> supportedFileFormats() const { return std::vector<std::string>(); }
    
    virtual void beginSequenceRender(SequenceTime /*first*/,SequenceTime /*last*/,
                                     SequenceTime /*step*/,bool /*interactive*/,RenderScale /*scale*/,
                                     bool /*isSequentialRender*/,bool /*isRenderResponseToUserInteraction*/,
                                     int /*view*/) {}
    
    virtual void endSequenceRender(SequenceTime /*first*/,SequenceTime /*last*/,
                                   SequenceTime /*step*/,bool /*interactive*/,RenderScale /*scale*/,
                                   bool /*isSequentialRender*/,bool /*isRenderResponseToUserInteraction*/,
                                   int /*view*/) {}
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
    
    Node* const _node; //< the node holding this effect

private:
    struct Implementation;
    boost::scoped_ptr<Implementation> _imp; // PIMPL: hide implementation details
    
    struct RenderArgs;
    
    /**
     * @brief The internal of renderRoI, mainly it calls render and handles the thread safety of the effect.
     * @returns True if the render call succeeded, false otherwise.
     **/
    bool renderRoIInternal(SequenceTime time,RenderScale scale,
                           int view,const RectI& renderWindow,
                           const boost::shared_ptr<const ImageParams>& cachedImgParams,
                           const boost::shared_ptr<Image>& image,
                           bool isSequentialRender,
                           bool isRenderMadeInResponseToUserInteraction,
                           bool byPassCache);
    
    /**
     * @brief Must be implemented to evaluate a value change
     * made to a knob(e.g: force a new render).
     * @param knob[in] The knob whose value changed.
     **/
    void evaluate(Knob* knob,bool isSignificant) OVERRIDE;

    
    virtual void onSlaveStateChanged(bool isSlave,KnobHolder* master) OVERRIDE FINAL;

    
    
    Natron::Status tiledRenderingFunctor(const RenderArgs& args,
                                         const RectI& roi,
                                         boost::shared_ptr<Natron::Image> output);
    
    /**
     * @brief Returns the index of the input if inputEffect is a valid input connected to this effect, otherwise returns -1.
     **/
    int getInputNumber(Natron::EffectInstance* inputEffect) const;
    
};

/**
 * @typedef Any plug-in should have a static function called BuildEffect with the following signature.
 * It is used to build a new instance of an effect. Basically it should just call the constructor.
 **/
typedef Natron::EffectInstance* (*EffectBuilder)(Natron::Node*);


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

    OutputEffectInstance(Node* node);

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
