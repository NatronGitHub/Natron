//  Powiter
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

#include "Engine/OfxNode.h"
#include "Engine/OfxClipInstance.h"
#include "Engine/OfxParamInstance.h"
#include "Engine/VideoEngine.h"


using namespace std;
using namespace Powiter;

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
    for (std::map<std::string,OFX::Host::Param::Instance*>::iterator it = _parentingMap.begin();
         it != _parentingMap.end();
         ++it) {
        delete it->second;
    }
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
    assert(plugin == this);
    return new OfxClipInstance(node(),node()->effectInstance(),index, descriptor);
}

OfxStatus OfxImageEffectInstance::vmessage(const char* type,
                                           const char* /*id*/,
                                           const char* format,
                                           va_list args) {
    // FIXME: this is really GUI stuff, and should be handled by signal/slot
    assert(type);
    assert(format);
    bool isQuestion = false;
    const char *prefix = "Message : ";
    if (strcmp(type, kOfxMessageLog) == 0) {
        prefix = "Log : ";
    }
    else if(strcmp(type, kOfxMessageFatal) == 0 ||
            strcmp(type, kOfxMessageError) == 0) {
        prefix = "Error : ";
    }
    else if(strcmp(type, kOfxMessageQuestion) == 0) {
        prefix = "Question : ";
        isQuestion = true;
    }

    // Just dump our message to stdout, should be done with a proper
    // UI in a full ap, and post a dialogue for yes/no questions.
    fputs(prefix, stdout);
    vprintf(format, args);
    printf("\n");

    if(isQuestion) {
        /// cant do this properly inour example, as we need to raise a dialogue to ask a question, so just return yes
        return kOfxStatReplyYes;
    }
    else {
        return kOfxStatOK;
    }
}


// The size of the current project in canonical coordinates.
// The size of a project is a sub set of the kOfxImageEffectPropProjectExtent. For example a
// project may be a PAL SD project, but only be a letter-box within that. The project size is
// the size of this sub window.
void OfxImageEffectInstance::getProjectSize(double& xSize, double& ySize) const {
    assert(node());
    xSize = node()->width();
    ySize = node()->height();
}

// The offset of the current project in canonical coordinates.
// The offset is related to the kOfxImageEffectPropProjectSize and is the offset from the origin
// of the project 'subwindow'. For example for a PAL SD project that is in letterbox form, the
// project offset is the offset to the bottom left hand corner of the letter box. The project
// offset is in canonical coordinates.
void OfxImageEffectInstance::getProjectOffset(double& xOffset, double& yOffset) const {
    assert(node());
    xOffset = node()->info().dataWindow().left();
    yOffset = node()->info().dataWindow().bottom();
}

// The extent of the current project in canonical coordinates.
// The extent is the size of the 'output' for the current project. See ProjectCoordinateSystems
// for more infomation on the project extent. The extent is in canonical coordinates and only
// returns the top right position, as the extent is always rooted at 0,0. For example a PAL SD
// project would have an extent of 768, 576.
void OfxImageEffectInstance::getProjectExtent(double& xSize, double& ySize) const {
    assert(node());
    xSize = node()->info().dataWindow().width();
    ySize = node()->info().dataWindow().height();
}

// The pixel aspect ratio of the current project
double OfxImageEffectInstance::getProjectPixelAspectRatio() const {
    assert(node());
    return node()->info().displayWindow().pixel_aspect();
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
    return 0.0;
}

/// This is called whenever a param is changed by the plugin so that
/// the recursive instanceChangedAction will be fed the correct
/// renderScale
void OfxImageEffectInstance::getRenderScaleRecursive(double &x, double &y) const {
    assert(node());
    x = y = 1.0;
}

// make a parameter instance
OFX::Host::Param::Instance* OfxImageEffectInstance::newParam(const std::string& oldName, OFX::Host::Param::Descriptor& descriptor)
{
    std::string name =  descriptor.getShortLabel();
    std::string prep = name.substr(0,1);
    std::locale loc;
    name[0] = std::toupper(prep.at(0),loc);
    if(descriptor.getType()==kOfxParamTypeInteger){
        OfxIntegerInstance* ret = new OfxIntegerInstance(node(),name,descriptor);
        if(ret){
            string parent = ret->getProperties().getStringProperty(kOfxParamPropParent);
            map<string,OFX::Host::Param::Instance*>::const_iterator it = _parentingMap.find(parent);
            if(it != _parentingMap.end()){
                if(it->second->getType() == kOfxParamTypeGroup){
                    OfxGroupInstance* group = dynamic_cast<OfxGroupInstance*>(it->second);
                    group->addKnob(ret->getKnob());
                }
            }
            _parentingMap.insert(make_pair(oldName,ret));
        }
        return ret;

    }else if(descriptor.getType()==kOfxParamTypeDouble){
        OfxDoubleInstance*  ret = new OfxDoubleInstance(node(),name,descriptor);
        if(ret){
            string parent = ret->getProperties().getStringProperty(kOfxParamPropParent);
            map<string,OFX::Host::Param::Instance*>::const_iterator it = _parentingMap.find(parent);
            if(it != _parentingMap.end()){
                if(it->second->getType() == kOfxParamTypeGroup){
                    OfxGroupInstance* group = dynamic_cast<OfxGroupInstance*>(it->second);
                    group->addKnob(ret->getKnob());
                }
            }
            _parentingMap.insert(make_pair(oldName,ret));
        }
        return ret;

    }else if(descriptor.getType()==kOfxParamTypeBoolean){
        OfxBooleanInstance* ret = new OfxBooleanInstance(node(),name,descriptor);
        if(ret){
            string parent = ret->getProperties().getStringProperty(kOfxParamPropParent);
            map<string,OFX::Host::Param::Instance*>::const_iterator it = _parentingMap.find(parent);
            if(it != _parentingMap.end()){
                if(it->second->getType() == kOfxParamTypeGroup){
                    OfxGroupInstance* group = dynamic_cast<OfxGroupInstance*>(it->second);
                    group->addKnob(ret->getKnob());
                }
            }
            _parentingMap.insert(make_pair(oldName,ret));
        }
        return ret;

    }else if(descriptor.getType()==kOfxParamTypeChoice){
        OfxChoiceInstance* ret = new OfxChoiceInstance(node(),name,descriptor);
        if(ret){
            string parent = ret->getProperties().getStringProperty(kOfxParamPropParent);
            map<string,OFX::Host::Param::Instance*>::const_iterator it = _parentingMap.find(parent);
            if(it != _parentingMap.end()){
                if(it->second->getType() == kOfxParamTypeGroup){
                    OfxGroupInstance* group = dynamic_cast<OfxGroupInstance*>(it->second);
                    group->addKnob(ret->getKnob());
                }
            }
            _parentingMap.insert(make_pair(oldName,ret));
        }
        return ret;

    }else if(descriptor.getType()==kOfxParamTypeRGBA){
        OfxRGBAInstance* ret = new OfxRGBAInstance(node(),name,descriptor);
        if(ret){
            string parent = ret->getProperties().getStringProperty(kOfxParamPropParent);
            map<string,OFX::Host::Param::Instance*>::const_iterator it = _parentingMap.find(parent);
            if(it != _parentingMap.end()){
                if(it->second->getType() == kOfxParamTypeGroup){
                    OfxGroupInstance* group = dynamic_cast<OfxGroupInstance*>(it->second);
                    group->addKnob(ret->getKnob());
                }
            }
            _parentingMap.insert(make_pair(oldName,ret));
        }
        return ret;

    }else if(descriptor.getType()==kOfxParamTypeRGB){
        OfxRGBInstance* ret = new OfxRGBInstance(node(),name,descriptor);
        if(ret){
            string parent = ret->getProperties().getStringProperty(kOfxParamPropParent);
            map<string,OFX::Host::Param::Instance*>::const_iterator it = _parentingMap.find(parent);
            if(it != _parentingMap.end()){
                if(it->second->getType() == kOfxParamTypeGroup){
                    OfxGroupInstance* group = dynamic_cast<OfxGroupInstance*>(it->second);
                    group->addKnob(ret->getKnob());
                }
            }
            _parentingMap.insert(make_pair(oldName,ret));
        }
        return ret;

    }else if(descriptor.getType()==kOfxParamTypeDouble2D){
        OfxDouble2DInstance* ret = new OfxDouble2DInstance(node(),name,descriptor);
        if(ret){
            string parent = ret->getProperties().getStringProperty(kOfxParamPropParent);
            map<string,OFX::Host::Param::Instance*>::const_iterator it = _parentingMap.find(parent);
            if(it != _parentingMap.end()){
                if(it->second->getType() == kOfxParamTypeGroup){
                    OfxGroupInstance* group = dynamic_cast<OfxGroupInstance*>(it->second);
                    group->addKnob(ret->getKnob());
                }
            }
            _parentingMap.insert(make_pair(oldName,ret));
        }
        return ret;

    }else if(descriptor.getType()==kOfxParamTypeInteger2D){
        OfxInteger2DInstance* ret = new OfxInteger2DInstance(node(),name,descriptor);
        if(ret){
            string parent = ret->getProperties().getStringProperty(kOfxParamPropParent);
            map<string,OFX::Host::Param::Instance*>::const_iterator it = _parentingMap.find(parent);
            if(it != _parentingMap.end()){
                if(it->second->getType() == kOfxParamTypeGroup){
                    OfxGroupInstance* group = dynamic_cast<OfxGroupInstance*>(it->second);
                    group->addKnob(ret->getKnob());
                }
            }
            _parentingMap.insert(make_pair(oldName,ret));
        }
        return ret;

    }else if(descriptor.getType()==kOfxParamTypePushButton){
        OfxPushButtonInstance* ret = new OfxPushButtonInstance(node(),name,descriptor);
        if(ret){
            string parent = ret->getProperties().getStringProperty(kOfxParamPropParent);
            map<string,OFX::Host::Param::Instance*>::const_iterator it = _parentingMap.find(parent);
            if(it != _parentingMap.end()){
                if(it->second->getType() == kOfxParamTypeGroup){
                    OfxGroupInstance* group = dynamic_cast<OfxGroupInstance*>(it->second);
                    group->addKnob(ret->getKnob());
                }
            }
            _parentingMap.insert(make_pair(oldName,ret));
        }
        return ret;

    }else if(descriptor.getType()==kOfxParamTypeGroup){
        OfxGroupInstance* ret = new OfxGroupInstance(node(),name,descriptor);
        if(ret){
            string parent = ret->getProperties().getStringProperty(kOfxParamPropParent);
            map<string,OFX::Host::Param::Instance*>::const_iterator it = _parentingMap.find(parent);
            if(it != _parentingMap.end()){
                if(it->second->getType() == kOfxParamTypeGroup){
                    OfxGroupInstance* group = dynamic_cast<OfxGroupInstance*>(it->second);
                    group->addKnob(ret->getKnob());
                }
            }
            _parentingMap.insert(make_pair(oldName,ret));
        }
        return ret;

    }else if(descriptor.getType()==kOfxParamTypePage){
        OFX::Host::Param::PageInstance* ret = new OFX::Host::Param::PageInstance(descriptor,this);
        return ret;
    }else if(descriptor.getType()==kOfxParamTypeString){
        OfxStringInstance* ret = new OfxStringInstance(node(),name,descriptor);if(ret){
            string parent = ret->getProperties().getStringProperty(kOfxParamPropParent);
            map<string,OFX::Host::Param::Instance*>::const_iterator it = _parentingMap.find(parent);
            if(it != _parentingMap.end()){
                if(it->second->getType() == kOfxParamTypeGroup){
                    OfxGroupInstance* group = dynamic_cast<OfxGroupInstance*>(it->second);
                    group->addKnob(ret->getKnob());
                }
            }
            _parentingMap.insert(make_pair(oldName,ret));
        }
        return ret;
    }else{
        return 0;
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
void OfxImageEffectInstance::progressStart(const std::string& /*message*/) {
}

/// finish yer progress
void OfxImageEffectInstance::progressEnd() {
}

/// set the progress to some level of completion, returns
/// false if you should abandon processing, true to continue
bool OfxImageEffectInstance::progressUpdate(double /*t*/) {
    return true;
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
    VideoEngine* engine = node()->getExecutingEngine();
    if(!engine)
        return -1.;
    const VideoEngine::DAG& dag = engine->getCurrentDAG();
    return dag.getOutput()->getTimeLine().currentFrame();
}


/// set the timeline to a specific time
void OfxImageEffectInstance::timeLineGotoTime(double t) {
    // FIXME-seeabove: disconnect timeline handling from GUI
    const VideoEngine::DAG& dag = node()->getExecutingEngine()->getCurrentDAG();
    if(!dag.getOutput()){
        return;
    }
    // FIXME: wrong Temporal Coordinates!!!
    // See http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#id476301
    // - the effect may not begin at frame 0
    // - it depends on the input and output FPS of the effect 
    return dag.getOutput()->getTimeLine().seek((int)t);
    
}


/// get the first and last times available on the effect's timeline
void OfxImageEffectInstance::timeLineGetBounds(double &t1, double &t2) {
    // FIXME-seeabove: disconnect timeline handling from GUI
    const VideoEngine::DAG& dag = node()->getExecutingEngine()->getCurrentDAG();
    if(!dag.getOutput()){
        t1 = -1.;
        t2 = -1.;
        return;
    }
    t1 = dag.getOutput()->getTimeLine().firstFrame();
    t2 = dag.getOutput()->getTimeLine().lastFrame();

}


// override this to make processing abort, return 1 to abort processing
int OfxImageEffectInstance::abort() {
    VideoEngine* v = node()->getExecutingEngine();
    if(v){
        return (int)v->hasBeenAborted();
    }
    return 0;
}
