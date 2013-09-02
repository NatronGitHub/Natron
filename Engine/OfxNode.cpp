//  Powiter
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */

#include "OfxNode.h"

#include <locale>

#include "Engine/OfxParamInstance.h"
#include "Engine/Row.h"
#include "Engine/OfxClipInstance.h"
#include "Engine/Model.h"
#include "Writers/Writer.h"
#include "Global/Controler.h"
#include "Engine/ViewerNode.h"
#include "Engine/VideoEngine.h"
#include "Gui/Timeline.h"
#include "Gui/ViewerTab.h"
#include "Gui/NodeGui.h"
using namespace std;
using namespace Powiter;

OfxNode::OfxNode(OFX::Host::ImageEffect::ImageEffectPlugin* plugin,
                 OFX::Host::ImageEffect::Descriptor         &other,
                 const std::string  &context):
OutputNode(),
OFX::Host::ImageEffect::Instance(plugin,other,context,false),
_tabKnob(0),
_lastKnobLayoutWithNoNewLine(0),
_isOutput(false),
_currentFrame(-1),
_preview(NULL),
_canHavePreview(false)
{
    
}



OFX::Host::ImageEffect::ClipInstance* OfxNode::newClipInstance(OFX::Host::ImageEffect::Instance* ,
                                                               OFX::Host::ImageEffect::ClipDescriptor* descriptor,
                                                               int index){
    return new OfxClipInstance(index,this,descriptor);
}
ChannelSet OfxNode::ofxComponentsToPowiterChannels(const std::string& comp){
    ChannelSet out;
    if(comp == kOfxImageComponentAlpha){
        out += Channel_alpha;
    }else if(comp == kOfxImageComponentRGB){
        out += Mask_RGB;
    }else if(comp == kOfxImageComponentRGBA){
        out += Mask_RGBA;
    }else if(comp == kOfxImageComponentYUVA){
        out += Mask_RGBA;
    }
    return out;
}

ChannelSet OfxNode::supportedComponents(){
    OFX::Host::ImageEffect::ClipInstance* clip = getClip("Output");
    const vector<string>& suppComponents = clip->getSupportedComponents();
    ChannelSet supportedComp;
    for (vector<string>::const_iterator it = suppComponents.begin(); it!= suppComponents.end(); ++it) {
        supportedComp += OfxNode::ofxComponentsToPowiterChannels(*it);
    }
    return supportedComp;
}

bool OfxNode::isOutputNode(){
    return _isOutput;
}

bool OfxNode::isInputNode(){
    if(getContext() == kOfxImageEffectContextGenerator)
        return true;
    return false;
}
const std::string OfxNode::className(){
    std::string label = getShortLabel();
    if(label.empty()){
        label = getLabel();
    }
    if (label!="Viewer") {
        return label;
    }else{
        std::string grouping = getDescriptor().getPluginGrouping();
        std::vector<std::string> groups = Model::extractAllPartsOfGrouping(grouping);
        return groups[0]+getLongLabel();
    }
}

std::string OfxNode::setInputLabel(int inputNb){
    
    MappedInputV copy = inputClipsCopyWithoutOutput();
    if(inputNb < (int)copy.size()){
        return copy[copy.size()-1-inputNb]->getShortLabel();
    }else{
        return Node::setInputLabel(inputNb);
    }
}
OfxNode::MappedInputV OfxNode::inputClipsCopyWithoutOutput(){
    const std::vector<OFX::Host::ImageEffect::ClipDescriptor*>& clips = getDescriptor().getClipsByOrder();
    MappedInputV copy;
    for (U32 i = 0; i < clips.size(); ++i) {
        if(clips[i]->getShortLabel() != "Output"){
            copy.push_back(clips[i]);
            // cout << "Clip[" << i << "] = " << clips[i]->getShortLabel() << endl;
        }
    }
    return copy;
}

int OfxNode::maximumInputs(){
    if(isInputNode()){
        return 0;
    }else{
        int totalClips = getDescriptor().getClips().size();
        return totalClips-1;
    }
    
}

int OfxNode::minimumInputs(){
    typedef std::map<std::string, OFX::Host::ImageEffect::ClipDescriptor*>  ClipsMap;
    const ClipsMap& clips = getDescriptor().getClips();
    int minimalCount = 0;
    for (ClipsMap::const_iterator it = clips.begin(); it!=clips.end(); ++it) {
        if(!it->second->isOptional()){
            ++minimalCount;
        }
    }
    return minimalCount-1;// -1 because we counted the "output" clip
}
bool OfxNode::isInputOptional(int inpubNb){
    MappedInputV inputs = inputClipsCopyWithoutOutput();
    return inputs[inputs.size()-1-inpubNb]->isOptional();
}
bool OfxNode::_validate(bool forReal){
    _firstTime = true;
    _frameRange.first = _info->firstFrame();
    _frameRange.second = _info->lastFrame();
    if (isInputNode()) {
        OFX::Host::ImageEffect::ClipInstance* clip = getClip("Output");
        /*if forReal is true we need to pass down the tree all the infos generated
         besides just the frame range.*/
        if (forReal) {
            OfxPointD rS;
            rS.x = rS.y = 1.0;
            OfxRectD rod;
            //This function calculates the merge of the inputs RoD.
            getRegionOfDefinitionAction(1.0, rS, rod);
            double pa = clip->getAspectRatio();
            //  cout << "pa : " << pa << endl;
            //we just set the displayWindow/dataWindow rather than merge it
            _info->setDisplayWindow(Format(rod.x1, rod.y1, rod.x2, rod.y2, "",pa));
            _info->set(rod.x1, rod.y1, rod.x2, rod.y2);
            _info->rgbMode(true);
            _info->setYdirection(1);
            string comp = clip->getUnmappedComponents();
            _info->setChannels(ofxComponentsToPowiterChannels(comp));
            
        }
        
    }
    //iterate over param and find if there's an unvalid param
    // e.g: an empty filename
    for (map<string,OFX::Host::Param::Instance*>::iterator it = _params.begin(); it!=_params.end(); ++it) {
        if(it->second->getType() == kOfxParamTypeString){
            OfxStringInstance* param = dynamic_cast<OfxStringInstance*>(it->second);
            assert(param);
            if(!param->isValid()){
                return false;
            }
        }
    }
    return true;
    
}
void OfxNode::engine(int y,int ,int ,ChannelSet channels ,Row* out){
    OfxRectI renderW;
    const Format& dispW = _info->getDisplayWindow();
    renderW.x1 = dispW.x();
    renderW.x2 = dispW.right();
    renderW.y1 = dispW.y();
    renderW.y2 = dispW.top();
    OfxPointD renderScale;
    getRenderScaleRecursive(renderScale.x, renderScale.y);
    {
        QMutexLocker g(&_lock);
        if(_firstTime){
            _firstTime = false;
            renderAction(0,kOfxImageFieldNone,renderW, renderScale);
            
        }
    }
    //the input clips and output clip are filled at this point, we need to copy the output image
    //into the rows, each thread handle a specific row again (the mutex locker is dead).
    
    OfxImage* img = dynamic_cast<OfxImage*>(getClip("Output")->getImage(0.0,NULL));
    if(img->bitDepth() == OfxImage::eBitDepthUByte)
    {
        const OfxRGBAColourB* srcPixels = img->pixelB(out->offset(), y);
        foreachChannels(chan, channels){
            float* writeable = out->writable(chan);
            if(writeable){
                ofxPackedBufferToRowPlane<OfxRGBAColourB>(chan, srcPixels, out->right()-out->offset(), writeable+out->offset());
            }
        }
    }else if(img->bitDepth() == OfxImage::eBitDepthUShort)
    {
        const OfxRGBAColourS* srcPixels = img->pixelS(out->offset(), y);
        foreachChannels(chan, channels){
            float* writeable = out->writable(chan);
            if(writeable){
                ofxPackedBufferToRowPlane<OfxRGBAColourS>(chan, srcPixels, out->right()-out->offset(), writeable+out->offset());
            }
        }
    }else if(img->bitDepth() == OfxImage::eBitDepthFloat)
    {
        const OfxRGBAColourF* srcPixels = img->pixelF(out->offset(), y);
        foreachChannels(chan, channels){
            float* writeable = out->writable(chan);
            if(writeable){
                ofxPackedBufferToRowPlane<OfxRGBAColourF>(chan, srcPixels, out->right()-out->offset(), writeable+out->offset());
            }
        }
    }
}

// make a parameter instance
OFX::Host::Param::Instance* OfxNode::newParam(const std::string& oldName, OFX::Host::Param::Descriptor& descriptor)
{
    std::string name =  descriptor.getShortLabel();
    std::string prep = name.substr(0,1);
    std::locale loc;
    name[0] = std::toupper(prep.at(0),loc);
    if(descriptor.getType()==kOfxParamTypeInteger){
        OfxIntegerInstance* ret = new OfxIntegerInstance(this,name,descriptor);
        if(ret){
            string parent = ret->getProperties().getStringProperty(kOfxParamPropParent);
            map<string,OFX::Host::Param::Instance*>::const_iterator it = _params.find(parent);
            if(it != _params.end()){
                if(it->second->getType() == kOfxParamTypeGroup){
                    OfxGroupInstance* group = dynamic_cast<OfxGroupInstance*>(it->second);
                    group->addKnob(ret->getKnob());
                }
            }
            _params.insert(make_pair(oldName,ret));
        }
        return ret;
        
    }else if(descriptor.getType()==kOfxParamTypeDouble){
        OfxDoubleInstance*  ret = new OfxDoubleInstance(this,name,descriptor);
        if(ret){
            string parent = ret->getProperties().getStringProperty(kOfxParamPropParent);
            map<string,OFX::Host::Param::Instance*>::const_iterator it = _params.find(parent);
            if(it != _params.end()){
                if(it->second->getType() == kOfxParamTypeGroup){
                    OfxGroupInstance* group = dynamic_cast<OfxGroupInstance*>(it->second);
                    group->addKnob(ret->getKnob());
                }
            }
            _params.insert(make_pair(oldName,ret));
        }
        return ret;
        
    }else if(descriptor.getType()==kOfxParamTypeBoolean){
        OfxBooleanInstance* ret = new OfxBooleanInstance(this,name,descriptor);
        if(ret){
            string parent = ret->getProperties().getStringProperty(kOfxParamPropParent);
            map<string,OFX::Host::Param::Instance*>::const_iterator it = _params.find(parent);
            if(it != _params.end()){
                if(it->second->getType() == kOfxParamTypeGroup){
                    OfxGroupInstance* group = dynamic_cast<OfxGroupInstance*>(it->second);
                    group->addKnob(ret->getKnob());
                }
            }
            _params.insert(make_pair(oldName,ret));
        }
        return ret;
        
    }else if(descriptor.getType()==kOfxParamTypeChoice){
        OfxChoiceInstance* ret = new OfxChoiceInstance(this,name,descriptor);
        if(ret){
            string parent = ret->getProperties().getStringProperty(kOfxParamPropParent);
            map<string,OFX::Host::Param::Instance*>::const_iterator it = _params.find(parent);
            if(it != _params.end()){
                if(it->second->getType() == kOfxParamTypeGroup){
                    OfxGroupInstance* group = dynamic_cast<OfxGroupInstance*>(it->second);
                    group->addKnob(ret->getKnob());
                }
            }
            _params.insert(make_pair(oldName,ret));
        }
        return ret;
        
    }else if(descriptor.getType()==kOfxParamTypeRGBA){
        OfxRGBAInstance* ret = new OfxRGBAInstance(this,name,descriptor);
        if(ret){
            string parent = ret->getProperties().getStringProperty(kOfxParamPropParent);
            map<string,OFX::Host::Param::Instance*>::const_iterator it = _params.find(parent);
            if(it != _params.end()){
                if(it->second->getType() == kOfxParamTypeGroup){
                    OfxGroupInstance* group = dynamic_cast<OfxGroupInstance*>(it->second);
                    group->addKnob(ret->getKnob());
                }
            }
            _params.insert(make_pair(oldName,ret));
        }
        return ret;
        
    }else if(descriptor.getType()==kOfxParamTypeRGB){
        OfxRGBInstance* ret = new OfxRGBInstance(this,name,descriptor);
        if(ret){
            string parent = ret->getProperties().getStringProperty(kOfxParamPropParent);
            map<string,OFX::Host::Param::Instance*>::const_iterator it = _params.find(parent);
            if(it != _params.end()){
                if(it->second->getType() == kOfxParamTypeGroup){
                    OfxGroupInstance* group = dynamic_cast<OfxGroupInstance*>(it->second);
                    group->addKnob(ret->getKnob());
                }
            }
            _params.insert(make_pair(oldName,ret));
        }
        return ret;
        
    }else if(descriptor.getType()==kOfxParamTypeDouble2D){
        OfxDouble2DInstance* ret = new OfxDouble2DInstance(this,name,descriptor);
        if(ret){
            string parent = ret->getProperties().getStringProperty(kOfxParamPropParent);
            map<string,OFX::Host::Param::Instance*>::const_iterator it = _params.find(parent);
            if(it != _params.end()){
                if(it->second->getType() == kOfxParamTypeGroup){
                    OfxGroupInstance* group = dynamic_cast<OfxGroupInstance*>(it->second);
                    group->addKnob(ret->getKnob());
                }
            }
            _params.insert(make_pair(oldName,ret));
        }
        return ret;
        
    }else if(descriptor.getType()==kOfxParamTypeInteger2D){
        OfxInteger2DInstance* ret = new OfxInteger2DInstance(this,name,descriptor);
        if(ret){
            string parent = ret->getProperties().getStringProperty(kOfxParamPropParent);
            map<string,OFX::Host::Param::Instance*>::const_iterator it = _params.find(parent);
            if(it != _params.end()){
                if(it->second->getType() == kOfxParamTypeGroup){
                    OfxGroupInstance* group = dynamic_cast<OfxGroupInstance*>(it->second);
                    group->addKnob(ret->getKnob());
                }
            }
            _params.insert(make_pair(oldName,ret));
        }
        return ret;
        
    }else if(descriptor.getType()==kOfxParamTypePushButton){
        OfxPushButtonInstance* ret = new OfxPushButtonInstance(this,name,descriptor);
        if(ret){
            string parent = ret->getProperties().getStringProperty(kOfxParamPropParent);
            map<string,OFX::Host::Param::Instance*>::const_iterator it = _params.find(parent);
            if(it != _params.end()){
                if(it->second->getType() == kOfxParamTypeGroup){
                    OfxGroupInstance* group = dynamic_cast<OfxGroupInstance*>(it->second);
                    group->addKnob(ret->getKnob());
                }
            }
            _params.insert(make_pair(oldName,ret));
        }
        return ret;
        
    }else if(descriptor.getType()==kOfxParamTypeGroup){
        OfxGroupInstance* ret = new OfxGroupInstance(this,name,descriptor);
        if(ret){
            string parent = ret->getProperties().getStringProperty(kOfxParamPropParent);
            map<string,OFX::Host::Param::Instance*>::const_iterator it = _params.find(parent);
            if(it != _params.end()){
                if(it->second->getType() == kOfxParamTypeGroup){
                    OfxGroupInstance* group = dynamic_cast<OfxGroupInstance*>(it->second);
                    group->addKnob(ret->getKnob());
                }
            }
            _params.insert(make_pair(oldName,ret));
        }
        return ret;
        
    }else if(descriptor.getType()==kOfxParamTypePage){
        OFX::Host::Param::PageInstance* ret = new OFX::Host::Param::PageInstance(descriptor,this);
        return ret;
    }else if(descriptor.getType()==kOfxParamTypeString){
        OfxStringInstance* ret = new OfxStringInstance(this,name,descriptor);if(ret){
            string parent = ret->getProperties().getStringProperty(kOfxParamPropParent);
            map<string,OFX::Host::Param::Instance*>::const_iterator it = _params.find(parent);
            if(it != _params.end()){
                if(it->second->getType() == kOfxParamTypeGroup){
                    OfxGroupInstance* group = dynamic_cast<OfxGroupInstance*>(it->second);
                    group->addKnob(ret->getKnob());
                }
            }
            _params.insert(make_pair(oldName,ret));
        }
        return ret;
    }else{
        return 0;
    }
}

void OfxNode::onInstanceChangedAction(const QString& str){
    double frame  = getFrameRecursive();
    OfxPointD renderScale;
    getRenderScaleRecursive(renderScale.x, renderScale.y);
    beginInstanceChangedAction(kOfxChangeUserEdited);
    paramInstanceChangedAction(str.toStdString(),kOfxChangeUserEdited,frame,renderScale);
    endInstanceChangedAction(kOfxChangeUserEdited);
    
    ctrlPTR->getModel()->getVideoEngine()->changeDAGAndStartEngine(this);
}

OfxStatus OfxNode::editBegin(const std::string& /*name*/)
{
    return kOfxStatErrMissingHostFeature;
}

OfxStatus OfxNode::editEnd(){
    return kOfxStatErrMissingHostFeature;
}

/// Start doing progress.
void  OfxNode::progressStart(const std::string& /*message*/)
{
}

/// finish yer progress
void  OfxNode::progressEnd()
{
}

/// set the progress to some level of completion, returns
/// false if you should abandon processing, true to continue
bool  OfxNode::progressUpdate(double /*t*/)
{
    return true;
}


/// get the current time on the timeline. This is not necessarily the same
/// time as being passed to an action (eg render)
double  OfxNode::timeLineGetTime()
{
    if(!ctrlPTR->getModel()->getVideoEngine())
        return -1.;
    const VideoEngine::DAG& dag = ctrlPTR->getModel()->getVideoEngine()->getCurrentDAG();
    if(!dag.getOutput()){
        return -1.;
    }
    if(dag.isOutputAnOpenFXNode()){
        return dag.outputAsOpenFXNode()->currentFrame();
    }else{
        if(dag.isOutputAViewer()){
            return currentViewer->getUiContext()->frameSeeker->currentFrame();
            
        }else{
            return dag.outputAsWriter()->currentFrame();
        }
    }
}

/// set the timeline to a specific time
void  OfxNode::timeLineGotoTime(double t)
{
    const VideoEngine::DAG& dag = ctrlPTR->getModel()->getVideoEngine()->getCurrentDAG();
    if(!dag.getOutput()){
        return;
    }

    if(dag.isOutputAnOpenFXNode()){
        return dag.outputAsOpenFXNode()->setCurrentFrame(t);
    }else{
        if(dag.isOutputAViewer()){
            currentViewer->getUiContext()->frameSeeker->seek_notSlot(t);
        }else{
            return dag.outputAsWriter()->setCurrentFrame(t);
        }
    }
}


/// get the first and last times available on the effect's timeline
void  OfxNode::timeLineGetBounds(double &t1, double &t2)
{
   

    const VideoEngine::DAG& dag = ctrlPTR->getModel()->getVideoEngine()->getCurrentDAG();
    if(!dag.getOutput()){
        t1 = -1.;
        t2 = -1.;
        return;
    }
    if(dag.isOutputAnOpenFXNode()){
        t1 = dag.outputAsOpenFXNode()->firstFrame();
        t2 = dag.outputAsOpenFXNode()->lastFrame();

    }else{
        if(dag.isOutputAViewer()){
            t1 = currentViewer->getUiContext()->frameSeeker->firstFrame();
            t2 = currentViewer->getUiContext()->frameSeeker->lastFrame();
        }else{
            t1 = dag.outputAsWriter()->firstFrame();
            t2 = dag.outputAsWriter()->lastFrame();
        }
    }
}

OfxStatus OfxNode::vmessage(const char* type,
                            const char* id,
                            const char* format,
                            va_list args) {
    return ctrlPTR->getModel()->vmessage(type, id, format, args);
}
void OfxNode::computePreviewImage(){
    if(getContext() != kOfxImageEffectContextGenerator){
        return;
    }
    //iterate over param and find if there's an unvalid param
    // e.g: an empty filename
    for (map<string,OFX::Host::Param::Instance*>::iterator it = _params.begin(); it!=_params.end(); ++it) {
        if(it->second->getType() == kOfxParamTypeString){
            OfxStringInstance* param = dynamic_cast<OfxStringInstance*>(it->second);
            assert(param);
            if(!param->isValid()){
                return;
            }
        }
    }

    if(_preview){
        delete _preview;
        _preview = 0;
    }
    
    QImage* ret = new QImage(64,64,QImage::Format_ARGB32);
    OfxPointD rS;
    rS.x = rS.y = 1.0;
    OfxRectD rod;
    //This function calculates the merge of the inputs RoD.
    getRegionOfDefinitionAction(1.0, rS, rod);
    OfxRectI renderW;
    renderW.x1 = rod.x1;
    renderW.x2 = rod.x2;
    renderW.y1 = rod.y1;
    renderW.y2 = rod.y2;
    Box2D oldBox(_info->getDataWindow());
    _info->set(rod.x1, rod.y1, rod.x2, rod.y2);
    OfxPointD renderScale;
    renderScale.x = renderScale.y = 1.;
    beginRenderAction(0, 25, 1, true, renderScale);
    renderAction(0,kOfxImageFieldNone,renderW, renderScale);
    OfxImage* img = dynamic_cast<OfxImage*>(getClip("Output")->getImage(0.0,NULL));
    OfxRectI bounds = img->getBounds();
    int w = (bounds.x2-bounds.x1) < 64 ? (bounds.x2-bounds.x1) : 64;
    int h = (bounds.y2-bounds.y1) < 64 ? (bounds.y2-bounds.y1) : 64;
    float zoomFactor = (float)h/(float)(bounds.y2-bounds.y1);
    for (int i = 0; i < h; ++i) {
        float y = (float)i*1.f/zoomFactor;
        int nearest;
        (y-floor(y) < ceil(y) - y) ? nearest = floor(y) : nearest = ceil(y);
        OfxRGBAColourF* src_pixels = img->pixelF(0,bounds.y2 -1 - nearest);
        QRgb *dst_pixels = (QRgb *) ret->scanLine(i);
        assert(src_pixels);
        for(int j = 0 ; j < w ; ++j) {
            float x = (float)j*1.f/zoomFactor;
            int nearestX;
            (x-floor(x) < ceil(x) - x) ? nearestX = floor(x) : nearestX = ceil(x);
            OfxRGBAColourF p = src_pixels[nearestX];
            dst_pixels[j] = qRgba(p.r*255, p.g*255, p.b*255,p.a ? p.a*255 : 255);
        }
    }
    _info->set(oldBox);
    endRenderAction(0, 25, 1, true, renderScale);

    
    _preview = ret;
    _nodeGUI->updatePreviewImageForReader();
}


