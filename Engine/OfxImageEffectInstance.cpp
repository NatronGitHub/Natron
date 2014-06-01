//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
//
//  Created by Frédéric Devernay on 03/09/13.
//
//

#include "OfxImageEffectInstance.h"

#include <cassert>
#include <string>
#include <map>
#include <locale>

#include <QDebug>

//ofx extension
#include <nuke/fnPublicOfxExtensions.h>

//for parametric params properties
#include <ofxParametricParam.h>

#include "Engine/OfxEffectInstance.h"
#include "Engine/OfxClipInstance.h"
#include "Engine/OfxParamInstance.h"
#include "Engine/VideoEngine.h"
#include "Engine/TimeLine.h"
#include "Engine/Knob.h"
#include "Engine/KnobTypes.h"
#include "Engine/KnobFactory.h"
#include "Engine/OfxMemory.h"
#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/Format.h"
#include "Engine/Node.h"
#include "Global/MemoryInfo.h"
#include "Engine/ViewerInstance.h"

using namespace Natron;

OfxImageEffectInstance::OfxImageEffectInstance(OFX::Host::ImageEffect::ImageEffectPlugin* plugin,
                                               OFX::Host::ImageEffect::Descriptor& desc,
                                               const std::string& context,
                                               bool interactive)
: OFX::Host::ImageEffect::Instance(plugin, desc, context, interactive)
, _node(NULL)
, _parentingMap()
{
}

OfxImageEffectInstance::~OfxImageEffectInstance()
{

}

const std::string& OfxImageEffectInstance::getDefaultOutputFielding() const {
    static const std::string v(kOfxImageFieldNone);
    return v;
}

/// make a clip
OFX::Host::ImageEffect::ClipInstance* OfxImageEffectInstance::newClipInstance(OFX::Host::ImageEffect::Instance* plugin,
                                                                              OFX::Host::ImageEffect::ClipDescriptor* descriptor,
                                                                              int index) {
    (void)plugin;
    return new OfxClipInstance(node(),this,index, descriptor);
}

OfxStatus OfxImageEffectInstance::setPersistentMessage(const char* type,
                               const char* /*id*/,
                               const char* format,
                               va_list args){
    assert(type);
    assert(format);
    char buf[10000];
    sprintf(buf, format,args);
    std::string message(buf);

    if(strcmp(type, kOfxMessageError) == 0) {
        
        _node->setPersistentMessage(Natron::ERROR_MESSAGE, message);
        
    }else if(strcmp(type, kOfxMessageWarning)){
        
        _node->setPersistentMessage(Natron::WARNING_MESSAGE, message);
        
    }else if(strcmp(type, kOfxMessageMessage)){
        
        _node->setPersistentMessage(Natron::INFO_MESSAGE, message);
        
    }
    return kOfxStatOK;
}

OfxStatus OfxImageEffectInstance::clearPersistentMessage(){
    _node->clearPersistentMessage();
    return kOfxStatOK;
}
OfxStatus OfxImageEffectInstance::vmessage(const char* msgtype,
                                           const char* /*id*/,
                                           const char* format,
                                           va_list args)
{
    assert(msgtype);
    assert(format);
    char buf[10000];
    sprintf(buf, format,args);
    std::string message(buf);
    std::string type(msgtype);

    if (type == kOfxMessageLog) {
        appPTR->writeToOfxLog_mt_safe(message.c_str());
    } else if (type == kOfxMessageFatal || type == kOfxMessageError) {
        _node->message(Natron::ERROR_MESSAGE, message);
    } else if (type == kOfxMessageWarning) {
        _node->message(Natron::WARNING_MESSAGE, message);
    } else if (type == kOfxMessageMessage) {
        _node->message(Natron::INFO_MESSAGE, message);
    } else if (type == kOfxMessageQuestion) {
        if (_node->message(Natron::QUESTION_MESSAGE, message)) {
            return kOfxStatReplyYes;
        } else {
            return kOfxStatReplyNo;
        }
    }
    return kOfxStatReplyDefault;
}

// The size of the current project in canonical coordinates.
// The size of a project is a sub set of the kOfxImageEffectPropProjectExtent. For example a
// project may be a PAL SD project, but only be a letter-box within that. The project size is
// the size of this sub window.
void OfxImageEffectInstance::getProjectSize(double& xSize, double& ySize) const
{
    Format f;
    _node->getRenderFormat(&f);
    xSize = f.width();
    ySize = f.height();

}

// The offset of the current project in canonical coordinates.
// The offset is related to the kOfxImageEffectPropProjectSize and is the offset from the origin
// of the project 'subwindow'. For example for a PAL SD project that is in letterbox form, the
// project offset is the offset to the bottom left hand corner of the letter box. The project
// offset is in canonical coordinates.
void OfxImageEffectInstance::getProjectOffset(double& xOffset, double& yOffset) const
{
    Format f;
    _node->getRenderFormat(&f);
    xOffset = f.left();
    yOffset = f.bottom();
}

// The extent of the current project in canonical coordinates.
// The extent is the size of the 'output' for the current project. See ProjectCoordinateSystems
// for more infomation on the project extent. The extent is in canonical coordinates and only
// returns the top right position, as the extent is always rooted at 0,0. For example a PAL SD
// project would have an extent of 768, 576.
void OfxImageEffectInstance::getProjectExtent(double& xSize, double& ySize) const
{
    Format f;
    _node->getRenderFormat(&f);
    xSize = f.right();
    ySize = f.top();
}

// The pixel aspect ratio of the current project
double OfxImageEffectInstance::getProjectPixelAspectRatio() const
{
    assert(_node);
    Format f;
    _node->getRenderFormat(&f);
    return f.getPixelAspect();
}

// The duration of the effect
// This contains the duration of the plug-in effect, in frames.
double OfxImageEffectInstance::getEffectDuration() const {
    assert(node());
    return 1.0;
}

// For an instance, this is the frame rate of the project the effect is in.
double OfxImageEffectInstance::getFrameRate() const {
    assert(node());
    return 25.0;
}

/// This is called whenever a param is changed by the plugin so that
/// the recursive instanceChangedAction will be fed the correct frame
double OfxImageEffectInstance::getFrameRecursive() const {
    assert(node());
    return node()->getApp()->getTimeLine()->currentFrame();
}

/// This is called whenever a param is changed by the plugin so that
/// the recursive instanceChangedAction will be fed the correct
/// renderScale
void OfxImageEffectInstance::getRenderScaleRecursive(double &x, double &y) const {
    assert(node());
    std::list<ViewerInstance*> attachedViewers;
    node()->getNode()->hasViewersConnected(&attachedViewers);
    ///get the render scale of the 1st viewer
    if (!attachedViewers.empty()) {
        ViewerInstance* first = attachedViewers.front();
        int mipMapLevel = first->getMipMapLevel();
        x = Natron::Image::getScaleFromMipMapLevel((unsigned int)mipMapLevel);
        y = x;
    } else {
        x = 1.;
        y = 1.;
    }
}

void OfxImageEffectInstance::getViewRecursive(int& view) const
{
    assert(node());
    std::list<ViewerInstance*> attachedViewers;
    node()->getNode()->hasViewersConnected(&attachedViewers);
    ///get the render scale of the 1st viewer
    if (!attachedViewers.empty()) {
        ViewerInstance* first = attachedViewers.front();
        view = first->getCurrentView();
    } else {
        view = 0;
    }
}

// make a parameter instance
OFX::Host::Param::Instance *OfxImageEffectInstance::newParam(const std::string &paramName, OFX::Host::Param::Descriptor &descriptor)
{
    // note: the order for parameter types is the same as in ofxParam.h
    OFX::Host::Param::Instance* instance = NULL;
    boost::shared_ptr<KnobI> knob;

    bool paramShouldBePersistant = true;
    if (descriptor.getType() == kOfxParamTypeInteger) {
        OfxIntegerInstance *ret = new OfxIntegerInstance(node(), descriptor);
        knob = ret->getKnob();
        boost::shared_ptr<KnobSignalSlotHandler> handler = dynamic_cast<KnobHelper*>(knob.get())->getSignalSlotHandler();
        QObject::connect(handler.get(),SIGNAL(animationLevelChanged(int)),ret,SLOT(onKnobAnimationLevelChanged(int)));
        instance = ret;

    } else if (descriptor.getType() == kOfxParamTypeDouble) {
        OfxDoubleInstance  *ret = new OfxDoubleInstance(node(), descriptor);
        knob = ret->getKnob();
        boost::shared_ptr<KnobSignalSlotHandler> handler = dynamic_cast<KnobHelper*>(knob.get())->getSignalSlotHandler();
        QObject::connect(handler.get(),SIGNAL(animationLevelChanged(int)),ret,SLOT(onKnobAnimationLevelChanged(int)));
        instance = ret;

    } else if (descriptor.getType() == kOfxParamTypeBoolean) {
        OfxBooleanInstance *ret = new OfxBooleanInstance(node(), descriptor);
        knob = ret->getKnob();
        boost::shared_ptr<KnobSignalSlotHandler> handler = dynamic_cast<KnobHelper*>(knob.get())->getSignalSlotHandler();
        QObject::connect(handler.get(),SIGNAL(animationLevelChanged(int)),ret,SLOT(onKnobAnimationLevelChanged(int)));
        instance = ret;

    } else if (descriptor.getType() == kOfxParamTypeChoice) {
        OfxChoiceInstance *ret = new OfxChoiceInstance(node(), descriptor);
        knob = ret->getKnob();
        boost::shared_ptr<KnobSignalSlotHandler> handler = dynamic_cast<KnobHelper*>(knob.get())->getSignalSlotHandler();
        QObject::connect(handler.get(),SIGNAL(animationLevelChanged(int)),ret,SLOT(onKnobAnimationLevelChanged(int)));
        instance = ret;

    } else if (descriptor.getType() == kOfxParamTypeRGBA) {
        OfxRGBAInstance *ret = new OfxRGBAInstance(node(), descriptor);
        knob = ret->getKnob();
        boost::shared_ptr<KnobSignalSlotHandler> handler = dynamic_cast<KnobHelper*>(knob.get())->getSignalSlotHandler();
        QObject::connect(handler.get(),SIGNAL(animationLevelChanged(int)),ret,SLOT(onKnobAnimationLevelChanged(int)));
        instance = ret;

    } else if (descriptor.getType() == kOfxParamTypeRGB) {
        OfxRGBInstance *ret = new OfxRGBInstance(node(), descriptor);
        knob = ret->getKnob();
        boost::shared_ptr<KnobSignalSlotHandler> handler = dynamic_cast<KnobHelper*>(knob.get())->getSignalSlotHandler();
        QObject::connect(handler.get(),SIGNAL(animationLevelChanged(int)),ret,SLOT(onKnobAnimationLevelChanged(int)));
        instance = ret;

    } else if (descriptor.getType() == kOfxParamTypeDouble2D) {
        OfxDouble2DInstance *ret = new OfxDouble2DInstance(node(), descriptor);
        knob = ret->getKnob();
        boost::shared_ptr<KnobSignalSlotHandler> handler = dynamic_cast<KnobHelper*>(knob.get())->getSignalSlotHandler();
        QObject::connect(handler.get(),SIGNAL(animationLevelChanged(int)),ret,SLOT(onKnobAnimationLevelChanged(int)));
        instance = ret;

    } else if (descriptor.getType() == kOfxParamTypeInteger2D) {
        OfxInteger2DInstance *ret = new OfxInteger2DInstance(node(), descriptor);
        knob = ret->getKnob();
        boost::shared_ptr<KnobSignalSlotHandler> handler = dynamic_cast<KnobHelper*>(knob.get())->getSignalSlotHandler();
        QObject::connect(handler.get(),SIGNAL(animationLevelChanged(int)),ret,SLOT(onKnobAnimationLevelChanged(int)));
        instance = ret;

    } else if (descriptor.getType() == kOfxParamTypeDouble3D) {
        OfxDouble3DInstance *ret = new OfxDouble3DInstance(node(), descriptor);
        knob = ret->getKnob();
        boost::shared_ptr<KnobSignalSlotHandler> handler = dynamic_cast<KnobHelper*>(knob.get())->getSignalSlotHandler();
        QObject::connect(handler.get(),SIGNAL(animationLevelChanged(int)),ret,SLOT(onKnobAnimationLevelChanged(int)));
        instance = ret;

    } else if (descriptor.getType() == kOfxParamTypeInteger3D) {
        OfxInteger3DInstance *ret = new OfxInteger3DInstance(node(), descriptor);
        knob = ret->getKnob();
        boost::shared_ptr<KnobSignalSlotHandler> handler = dynamic_cast<KnobHelper*>(knob.get())->getSignalSlotHandler();
        QObject::connect(handler.get(),SIGNAL(animationLevelChanged(int)),ret,SLOT(onKnobAnimationLevelChanged(int)));
        instance = ret;

    } else if (descriptor.getType() == kOfxParamTypeString) {
        OfxStringInstance *ret = new OfxStringInstance(node(), descriptor);
        knob = ret->getKnob();
        boost::shared_ptr<KnobSignalSlotHandler> handler = dynamic_cast<KnobHelper*>(knob.get())->getSignalSlotHandler();
        QObject::connect(handler.get(),SIGNAL(animationLevelChanged(int)),ret,SLOT(onKnobAnimationLevelChanged(int)));
        instance = ret;

    } else if (descriptor.getType() == kOfxParamTypeCustom) {
        /*
         http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#kOfxParamTypeCustom

         Custom parameters contain null terminated char * C strings, and may animate. They are designed to provide plugins with a way of storing data that is too complicated or impossible to store in a set of ordinary parameters.

        If a custom parameter animates, it must set its kOfxParamPropCustomInterpCallbackV1 property, which points to a OfxCustomParamInterpFuncV1 function. This function is used to interpolate keyframes in custom params.

        Custom parameters have no interface by default. However,

        * if they animate, the host's animation sheet/editor should present a keyframe/curve representation to allow positioning of keys and control of interpolation. The 'normal' (ie: paged or hierarchical) interface should not show any gui.
        * if the custom param sets its kOfxParamPropInteractV1 property, this should be used by the host in any normal (ie: paged or hierarchical) interface for the parameter.

        Custom parameters are mandatory, as they are simply ASCII C strings. However, animation of custom parameters an support for an in editor interact is optional.
         */
        //throw std::runtime_error(std::string("Parameter ") + paramName + std::string(" has unsupported OFX type ") + descriptor.getType());
        OfxCustomInstance *ret = new OfxCustomInstance(node(), descriptor);
        knob = ret->getKnob();
        boost::shared_ptr<KnobSignalSlotHandler> handler = dynamic_cast<KnobHelper*>(knob.get())->getSignalSlotHandler();
        QObject::connect(handler.get(),SIGNAL(animationLevelChanged(int)),ret,SLOT(onKnobAnimationLevelChanged(int)));
        instance = ret;

    } else if (descriptor.getType() == kOfxParamTypeGroup) {
        OfxGroupInstance *ret = new OfxGroupInstance(node(), descriptor);
        knob = ret->getKnob();
        instance = ret;
        paramShouldBePersistant = false;
    } else if (descriptor.getType() == kOfxParamTypePage) {
        OfxPageInstance* ret = new OfxPageInstance(node(),descriptor);
        knob = ret->getKnob();
        instance = ret;
        paramShouldBePersistant = false;
    } else if (descriptor.getType() == kOfxParamTypePushButton) {
        OfxPushButtonInstance *ret = new OfxPushButtonInstance(node(), descriptor);
        knob = ret->getKnob();
        instance = ret;
        paramShouldBePersistant = false;
    } else if (descriptor.getType() == kOfxParamTypeParametric) {
        OfxParametricInstance* ret = new OfxParametricInstance(node(), descriptor);
        OfxStatus stat = ret->defaultInitializeAllCurves(descriptor);
        if(stat == kOfxStatFailed){
            throw std::runtime_error("The parameter failed to create curves from their default\n"
                                     "initialized by the plugin.");
        }
        knob = ret->getKnob();
        instance = ret;
    }
    

    if (!instance) {
        throw std::runtime_error(std::string("Parameter ") + paramName + std::string(" has unknown OFX type ") + descriptor.getType());
    }

    std::string parent = instance->getProperties().getStringProperty(kOfxParamPropParent);
    if(!parent.empty()){
        _parentingMap.insert(make_pair(instance,parent));
    }
    
    knob->setName(paramName);
    knob->setEvaluateOnChange(descriptor.getEvaluateOnChange());
    
    bool persistant = descriptor.getIsPersistant();
    if (!paramShouldBePersistant) {
        persistant = false;
    }
    knob->setIsPersistant(persistant);
    knob->setAnimationEnabled(descriptor.getCanAnimate());
    knob->setSecret(descriptor.getSecret());
    knob->setAllDimensionsEnabled(descriptor.getEnabled());
    knob->setHintToolTip(descriptor.getHint());
    knob->setCanUndo(descriptor.getCanUndo());
    knob->setSpacingBetweenItems(descriptor.getProperties().getIntProperty(kOfxParamPropLayoutPadWidth));
    int layoutHint = descriptor.getProperties().getIntProperty(kOfxParamPropLayoutHint);
    if (layoutHint == 1) {
        (void)Natron::createKnob<Separator_Knob>(node(), knob->getDescription());
    }else if(layoutHint == 2){
        knob->turnOffNewLine();
    }
    
    
    return instance;

}

void OfxImageEffectInstance::addParamsToTheirParents(){
    const std::list<OFX::Host::Param::Instance*>& params = getParamList();
    //for each params find their parents if any and add to the parent this param's knob
    for (std::list<OFX::Host::Param::Instance*>::const_iterator it = params.begin(); it!=params.end(); ++it) {
        
        OfxPageInstance* isPage = dynamic_cast<OfxPageInstance*>(*it);
        if (isPage) {
            isPage->populatePage();
        } else {
            
            std::map<OFX::Host::Param::Instance*,std::string>::const_iterator found = _parentingMap.find(*it);
            
            //the param has no parent
            if(found == _parentingMap.end()){
                continue;
            }
            
            assert(!found->second.empty());
            
            //find the parent by name
            const std::map<std::string, OFX::Host::Param::Instance*>& paramsMap = getParams();
            std::map<std::string, OFX::Host::Param::Instance*>::const_iterator foundParent = paramsMap.find(found->second);
            
            //the parent must exist!
            assert(foundParent != paramsMap.end());
            
            //add the param's knob to the parent
            OfxParamToKnob* knobHolder = dynamic_cast<OfxParamToKnob*>(found->first);
            assert(knobHolder);
            dynamic_cast<OfxGroupInstance*>(foundParent->second)->addKnob(knobHolder->getKnob());
        }
    }
    
    
    
}

/** @brief Used to group any parameter changes for undo/redo purposes

 \arg paramSet   the parameter set in which this is happening
 \arg name       label to attach to any undo/redo string UTF8
 
 If a plugin calls paramSetValue/paramSetValueAtTime on one or more parameters, either from custom GUI interaction
 or some analysis of imagery etc.. this is used to indicate the start of a set of a parameter
 changes that should be considered part of a single undo/redo block.
 
 See also OfxParameterSuiteV1::paramEditEnd
 
 \return
 - ::kOfxStatOK       - all was OK
 - ::kOfxStatErrBadHandle  - if the instance handle was invalid
 
 */
OfxStatus OfxImageEffectInstance::editBegin(const std::string& /*name*/) {
    return kOfxStatErrMissingHostFeature;
}

/// Triggered when the plug-in calls OfxParameterSuiteV1::paramEditEnd
///
/// Client host code needs to implement this
OfxStatus OfxImageEffectInstance::editEnd(){
    return kOfxStatErrMissingHostFeature;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// overridden for Progress::ProgressI

/// Start doing progress.
void OfxImageEffectInstance::progressStart(const std::string& message) {
    _node->getApp()->startProgress(_node, message);
}

/// finish yer progress
void OfxImageEffectInstance::progressEnd() {
    _node->getApp()->endProgress(_node);
}

/** @brief Indicate how much of the processing task has been completed and reports on any abort status.
 
 \arg \e effectInstance - the instance of the plugin this progress bar is
 associated with. It cannot be NULL.
 \arg \e progress - a number between 0.0 and 1.0 indicating what proportion of the current task has been processed.
 \returns false if you should abandon processing, true to continue
*/
bool OfxImageEffectInstance::progressUpdate(double t) {
    return _node->getApp()->progressUpdate(_node, t);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// overridden for TimeLine::TimeLineI

/// get the current time on the timeline. This is not necessarily the same
/// time as being passed to an action (eg render)
double OfxImageEffectInstance::timeLineGetTime() {
    
    return _node->getApp()->getTimeLine()->currentFrame();
}


/// set the timeline to a specific time
void OfxImageEffectInstance::timeLineGotoTime(double t) {
    
    _node->getApp()->getTimeLine()->seekFrame((int)t,NULL);
    
}


/// get the first and last times available on the effect's timeline
void OfxImageEffectInstance::timeLineGetBounds(double &t1, double &t2) {

    t1 = _node->getApp()->getTimeLine()->leftBound();
    t2 = _node->getApp()->getTimeLine()->rightBound();
}


// override this to make processing abort, return 1 to abort processing
int OfxImageEffectInstance::abort() {
    return (int)node()->aborted();
}

OFX::Host::Memory::Instance* OfxImageEffectInstance::newMemoryInstance(size_t nBytes) {
    OfxMemory* ret = new OfxMemory(_node);
    bool allocated = ret->alloc(nBytes);
    if (!ret->getPtr() || !allocated) {
        Natron::errorDialog("Out of memory", node()->getNode()->getName_mt_safe() + " failed to allocate memory (" + printAsRAM(nBytes).toStdString() + ").");
    }
    return ret;
}

void OfxImageEffectInstance::setClipsMipMapLevel(unsigned int mipMapLevel)
{
    for(std::map<std::string, OFX::Host::ImageEffect::ClipInstance*>::iterator it = _clips.begin(); it != _clips.end(); ++it) {
        dynamic_cast<OfxClipInstance*>(it->second)->setMipMapLevel(mipMapLevel);
    } 
}

void OfxImageEffectInstance::setClipsView(int view)
{
    for(std::map<std::string, OFX::Host::ImageEffect::ClipInstance*>::iterator it = _clips.begin(); it != _clips.end(); ++it) {
        it->second->setView(view);
    }
}

void OfxImageEffectInstance::discardClipsMipMapLevel()
{
    for(std::map<std::string, OFX::Host::ImageEffect::ClipInstance*>::iterator it = _clips.begin(); it != _clips.end(); ++it) {
        dynamic_cast<OfxClipInstance*>(it->second)->discardMipMapLevel();
    }

}

void OfxImageEffectInstance::discardClipsView()
{
    for(std::map<std::string, OFX::Host::ImageEffect::ClipInstance*>::iterator it = _clips.begin(); it != _clips.end(); ++it) {
        dynamic_cast<OfxClipInstance*>(it->second)->discardView();
    }

}

void OfxImageEffectInstance::setClipsRenderedImage(const boost::shared_ptr<Natron::Image>& image)
{
    for(std::map<std::string, OFX::Host::ImageEffect::ClipInstance*>::iterator it = _clips.begin(); it != _clips.end(); ++it) {
        dynamic_cast<OfxClipInstance*>(it->second)->setRenderedImage(image);
    }
}

void OfxImageEffectInstance::discardClipsImage()
{
    for(std::map<std::string, OFX::Host::ImageEffect::ClipInstance*>::iterator it = _clips.begin(); it != _clips.end(); ++it) {
        dynamic_cast<OfxClipInstance*>(it->second)->discardRenderedImage();
    }
}

void OfxImageEffectInstance::setClipsHash(U64 hash)
{
    for(std::map<std::string, OFX::Host::ImageEffect::ClipInstance*>::iterator it = _clips.begin(); it != _clips.end(); ++it) {
        dynamic_cast<OfxClipInstance*>(it->second)->setAttachedNodeHash(hash);
    }
}

void OfxImageEffectInstance::discardClipsHash()
{
    for(std::map<std::string, OFX::Host::ImageEffect::ClipInstance*>::iterator it = _clips.begin(); it != _clips.end(); ++it) {
        dynamic_cast<OfxClipInstance*>(it->second)->discardAttachedNodeHash();
    }
}