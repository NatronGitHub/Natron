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

#include "Engine/Knob.h" // for KnobHolder

class RectI;
class Hash64;
class RenderTree;
class VideoEngine;
class RenderTree;
class Format;
namespace Natron{

class Node;
class Image;


/**
 * @brief This is the base class for visual effects.
 * A live instance is always living throughout the lifetime of a Node and other copies are
 * created on demand when a render is needed.
 **/
class EffectInstance : public KnobHolder
{
public:
    typedef std::vector<EffectInstance*> Inputs;
    
    typedef std::map<EffectInstance*,RectI> RoIMap;
public:
    
    
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

    bool isLiveInstance() const WARN_UNUSED_RETURN;
    
    /**
     * @brief  Used once for each "render instance". It makes a full clone of the live instance.
     * This instance will be "read-only": modifying values will have no impact on the GUI.
     **/
    void clone(SequenceTime time);
    
    /**
     * @brief  Used once for each "render instance". It makes a full clone of the other instance.
     * This instance will be "read-only": modifying values will have no impact on the GUI.
     **/
    U64 computeHash(SequenceTime time,const std::vector<U64>& inputsHashs);
    
    const Hash64& hash() const WARN_UNUSED_RETURN;
            
    bool isHashValid() const WARN_UNUSED_RETURN;
    
    int hashAge() const WARN_UNUSED_RETURN;
    
    const Inputs& getInputs() const WARN_UNUSED_RETURN;
    
    boost::shared_ptr<Knob> getKnobByDescription(const std::string& desc) const WARN_UNUSED_RETURN;

    void setMarkedByTopologicalSort(bool marked) const;

    bool isMarkedByTopologicalSort() const WARN_UNUSED_RETURN;

    /**
     * @brief Forwarded to the node's name
     **/
    const std::string& getName() const WARN_UNUSED_RETURN;

    /**
     * @brief Forwarded to the node's render format
     **/
    const Format& getRenderFormat() const WARN_UNUSED_RETURN;
    
    /**
     * @brief Forwarded to the node's render views count
     **/
    int getRenderViewsCount() const WARN_UNUSED_RETURN;
    
    /**
     * @brief Returns input n. It might be NULL if the input is not connected.
     **/
    EffectInstance* input(int n) const WARN_UNUSED_RETURN;
    
    /**
     * @brief Forwarded to the node holding the effect
     **/
    bool hasOutputConnected() const WARN_UNUSED_RETURN;
    
    /**
     * @brief Is this node an input node ? An input node means
     * it has no input.
     **/
    virtual bool isGenerator() const WARN_UNUSED_RETURN { return false; }
    
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
                                                     bool byPassCache = false) WARN_UNUSED_RETURN;

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
    virtual Natron::Status render(SequenceTime /*time*/, RenderScale /*scale*/, const RectI& /*roi*/, int /*view*/, boost::shared_ptr<Natron::Image> /*output*/) WARN_UNUSED_RETURN { return Natron::StatOK; }
    
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
     * @brief This is the place to initialise any per frame specific data or flag.
     * If the return value is StatOK or StatReplyDefault the render will continue, otherwise
     * the render will stop.
     * If the status is StatFailed a message should be posted by the plugin.
     **/
    virtual Natron::Status preProcessFrame(SequenceTime /*time*/) WARN_UNUSED_RETURN { return Natron::StatReplyDefault; }
    
    
    /**
     * @brief Can be derived to get the region that the plugin is capable of filling.
     * This is meaningful for plugins that generate images or transform images.
     * By default it returns in rod the union of all inputs RoD and StatReplyDefault is returned.
     * In case of failure the plugin should return StatFailed.
     **/
    virtual Natron::Status getRegionOfDefinition(SequenceTime time,RectI* rod) WARN_UNUSED_RETURN;
    
    
    /**
     * @brief Can be derived to indicate for each input node what is the region of interest
     * of the node at time 'time' and render scale 'scale' given a render window.
     * For exemple a blur plugin would specify what it needs
     * from inputs in order to do a blur taking into account the size of the blurring kernel.
     * By default, it returns renderWindow for each input.
     **/
    virtual RoIMap getRegionOfInterest(SequenceTime time,RenderScale scale,const RectI& renderWindow) WARN_UNUSED_RETURN;

    /**
     * @brief Can be derived to get the frame range wherein the plugin is capable of producing frames.
     * By default it merges the frame range of the inputs.
     * In case of failure the plugin should return StatFailed.
     **/
    virtual void getFrameRange(SequenceTime *first,SequenceTime *last);
    
    
    /* @brief Overlay support:
     * Just overload this function in your operator.
     * No need to include any OpenGL related header.
     * The coordinate space is  defined by the displayWindow
     * (i.e: (0,0) = bottomLeft and  width() and height() being
     * respectivly the width and height of the frame.)
     */
    virtual void drawOverlay(){}
    
    virtual bool onOverlayPenDown(const QPointF& /*viewportPos*/, const QPointF& /*pos*/) { return false; }
    
    virtual bool onOverlayPenMotion(const QPointF& /*viewportPos*/, const QPointF& /*pos*/) { return false; }
    
    virtual bool onOverlayPenUp(const QPointF& /*viewportPos*/, const QPointF& /*pos*/) { return false; }
    
    virtual void onOverlayKeyDown(QKeyEvent* /*e*/) {}
    
    virtual void onOverlayKeyUp(QKeyEvent* /*e*/) {}
    
    virtual void onOverlayKeyRepeat(QKeyEvent* /*e*/) {}
    
    virtual void onOverlayFocusGained(){}
    
    virtual void onOverlayFocusLost(){}
    
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
     * @brief Must be implemented to initialize any knob using the
     * KnobFactory.
     **/
    virtual void initializeKnobs() OVERRIDE {};
    
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
     * @brief When called, if the node holding this effect  is connected to any
     * viewer, it will render again the current frame. By default you don't have to
     * call this yourself as this is called automatically on a knob value changed,
     * but this is provided as a way to force things.
     **/
    void requestRender() { evaluate(NULL,true); }
    
    /**
     * @brief Abort any ongoing rendering that uses this effect.
     **/
    void abortRendering();
    
    /**
     * @brief Call this whenever the frame range for this operator changed (i.e: through reading a file sequence for instance).
     * This in turns updates the timeline of the project.
     **/
    void notifyFrameRangeChanged(int first,int last);
    
    /**
     * @brief This function is called by the GUI on node creation if it has any file knob.
     **/
    void openFilesForAllFileKnobs();
    
    /**
     * @brief This function is called by the node holding this effect. Never call this yourself.
     **/
    void setAsRenderClone();
    
    /**
     * @brief This function is called by the node holding this effect. Never call this yourself.
     **/
    void updateInputs(RenderTree* tree);
    
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
    
protected:
    /**
     * @brief This function is provided for means to copy more data than just the knobs from the live instance
     * to the render clones.
     **/
    virtual void cloneExtras(){}
    
private:
    Node* _node; //< the node holding this effect
    struct Implementation;
    boost::scoped_ptr<Implementation> _imp; // PIMPL: hide implementation details
    
    struct RenderArgs;


    /**
     * @brief Must be implemented to evaluate a value change
     * made to a knob(e.g: force a new render).
     * @param knob[in] The knob whose value changed.
     **/
    void evaluate(Knob* knob,bool isSignificant) OVERRIDE;


    
    
    Natron::Status tiledRenderingFunctor(const RenderArgs& args,
                               const RectI& roi,
                               boost::shared_ptr<Natron::Image> output);
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
public:

    OutputEffectInstance(Node* node);

    virtual ~OutputEffectInstance(){}

    virtual bool isOutput() const { return true; }

    boost::shared_ptr<VideoEngine> getVideoEngine() const {return _videoEngine;}
    
    /**
     * @brief Starts rendering of all the sequence available, from start to end.
     * This function is meant to be called for on-disk renderer only (i.e: not viewers).
     **/
    void renderFullSequence();

    void updateTreeAndRender(bool initViewer = false);

    void refreshAndContinueRender(bool initViewer = false);

    void ifInfiniteclipRectToProjectDefault(RectI* rod) const;

    /**
     * @brief Returns the frame number this effect is currently rendering.
     * Note that this function can be used only for Writers or OpenFX writers,
     * it doesn't work with the Viewer.
     **/
    int getCurrentFrame() const { return _writerCurrentFrame; }
    
    void setCurrentFrame(int f) { _writerCurrentFrame = f; }
    
    int getFirstFrame() const { return _writerFirstFrame; }
    
    void setFirstFrame(int f) {  _writerFirstFrame = f;}
    
    int getLastFrame() const { return _writerLastFrame; }
    
    void setLastFrame(int f) {  _writerLastFrame = f;}

    
    
};


} // Natron
#endif // NATRON_ENGINE_EFFECTINSTANCE_H_
