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

//#include "Engine/OfxHost.h" // for OfxHost::vmessage
#include "Engine/OfxParamInstance.h"
#include "Engine/Row.h"
#include "Engine/OfxClipInstance.h"
#include "Engine/OfxImageEffectInstance.h"
#include "Engine/Model.h"
#include "Writers/Writer.h"
#include "Global/AppManager.h"
#include "Engine/ViewerNode.h"
#include "Engine/VideoEngine.h"
#include "Gui/NodeGui.h"
using namespace std;
using namespace Powiter;

namespace {
    ChannelSet ofxComponentsToPowiterChannels(const std::string& comp) {
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
}

OfxNode::OfxNode(OFX::Host::ImageEffect::ImageEffectPlugin* plugin,
                 OFX::Host::ImageEffect::Descriptor         &other,
                 const std::string  &context,
                 Model* model)
: OutputNode(model)
, _tabKnob(0)
, _lastKnobLayoutWithNoNewLine(0)
, _isOutput(false)
, _preview(NULL)
, _canHavePreview(false)
, effect_(new OfxImageEffectInstance(this,plugin,other,context,false))
{
    
}

OfxNode::~OfxNode() {
    delete effect_;
}

ChannelSet OfxNode::supportedComponents() {
    // FIXME: should be const...
    const OFX::Host::ImageEffect::ClipInstance* clip = effectInstance()->getClip("Output");
    const vector<string>& suppComponents = clip->getSupportedComponents();
    ChannelSet supportedComp;
    for (vector<string>::const_iterator it = suppComponents.begin(); it!= suppComponents.end(); ++it) {
        supportedComp += ofxComponentsToPowiterChannels(*it);
    }
    return supportedComp;
}

bool OfxNode::isOutputNode() const {
    return _isOutput;
}

bool OfxNode::isInputNode() const {
    if(effectInstance()->getContext() == kOfxImageEffectContextGenerator)
        return true;
    return false;
}

std::string OfxNode::className() { // should be const
    std::string label = getShortLabel();
    if(label.empty()){
        label = effectInstance()->getLabel();
    }
    if (label!="Viewer") {
        return label;
    }else{
        std::string grouping = effectInstance()->getDescriptor().getPluginGrouping();
        std::vector<std::string> groups = ofxExtractAllPartsOfGrouping(grouping);
        return groups[0] + effectInstance()->getLongLabel();
    }
}

std::string OfxNode::setInputLabel(int inputNb) {
    
    MappedInputV copy = inputClipsCopyWithoutOutput();
    if(inputNb < (int)copy.size()){
        return copy[copy.size()-1-inputNb]->getShortLabel();
    }else{
        return Node::setInputLabel(inputNb);
    }
}
OfxNode::MappedInputV OfxNode::inputClipsCopyWithoutOutput() const {
    const std::vector<OFX::Host::ImageEffect::ClipDescriptor*>& clips = effectInstance()->getDescriptor().getClipsByOrder();
    MappedInputV copy;
    for (U32 i = 0; i < clips.size(); ++i) {
        if(clips[i]->getShortLabel() != "Output"){
            copy.push_back(clips[i]);
            // cout << "Clip[" << i << "] = " << clips[i]->getShortLabel() << endl;
        }
    }
    return copy;
}

int OfxNode::maximumInputs() const {
    if(isInputNode()){
        return 0;
    }else{
        int totalClips = effectInstance()->getDescriptor().getClips().size();
        return totalClips-1;
    }
    
}

int OfxNode::minimumInputs() const {
    typedef std::map<std::string, OFX::Host::ImageEffect::ClipDescriptor*>  ClipsMap;
    const ClipsMap& clips = effectInstance()->getDescriptor().getClips();
    int minimalCount = 0;
    for (ClipsMap::const_iterator it = clips.begin(); it!=clips.end(); ++it) {
        if(!it->second->isOptional()){
            ++minimalCount;
        }
    }
    return minimalCount-1;// -1 because we counted the "output" clip
}
bool OfxNode::isInputOptional(int inpubNb) const {
    MappedInputV inputs = inputClipsCopyWithoutOutput();
    return inputs[inputs.size()-1-inpubNb]->isOptional();
}
bool OfxNode::_validate(bool forReal){
    _firstTime = true;
    _timeline.setFirstFrame(info().firstFrame());
    _timeline.setLastFrame(info().lastFrame());
    
    /*Checking if all mandatory inputs are connected!*/
    MappedInputV ofxInputs = inputClipsCopyWithoutOutput();
    for (U32 i = 0; i < ofxInputs.size(); ++i) {
        if (!ofxInputs[i]->isOptional() && !input(ofxInputs.size()-1-i)) {
            return false;
        }
    }
    
    if (isInputNode()) {
        OFX::Host::ImageEffect::ClipInstance* clip = effectInstance()->getClip("Output");
        /*if forReal is true we need to pass down the tree all the infos generated
         besides just the frame range.*/
        if (forReal) {
            OfxPointD rS;
            rS.x = rS.y = 1.0;
            OfxRectD rod;
            //This function calculates the merge of the inputs RoD.
            effectInstance()->getRegionOfDefinitionAction(1.0, rS, rod);
            double pa = clip->getAspectRatio();
            //  cout << "pa : " << pa << endl;
            //we just set the displayWindow/dataWindow rather than merge it
            _info.set_displayWindow(Format(rod.x1, rod.y1, rod.x2, rod.y2, "",pa));
            _info.set_dataWindow(Box2D(rod.x1, rod.y1, rod.x2, rod.y2));
            _info.set_rgbMode(true);
            _info.set_ydirection(1);
            string comp = clip->getUnmappedComponents();
            _info.set_channels(ofxComponentsToPowiterChannels(comp));
            
        }
        
    }
    //iterate over param and find if there's an unvalid param
    // e.g: an empty filename
    const map<string,OFX::Host::Param::Instance*>& params = effect_->getParams();
    for (map<string,OFX::Host::Param::Instance*>::const_iterator it = params.begin(); it!=params.end(); ++it) {
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
    const Format& dispW = info().displayWindow();
    renderW.x1 = dispW.x();
    renderW.x2 = dispW.right();
    renderW.y1 = dispW.y();
    renderW.y2 = dispW.top();
    OfxPointD renderScale;
    effectInstance()->getRenderScaleRecursive(renderScale.x, renderScale.y);
    {
        QMutexLocker g(&_lock);
        if(_firstTime){
            _firstTime = false;
            effectInstance()->renderAction(0,kOfxImageFieldNone,renderW, renderScale);

        }
    }
    //the input clips and output clip are filled at this point, we need to copy the output image
    //into the rows, each thread handle a specific row again (the mutex locker is dead).

    OfxImage* img = dynamic_cast<OfxImage*>(effectInstance()->getClip("Output")->getImage(0.0,NULL));
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


void OfxNode::onInstanceChangedAction(const QString& str){
    double frame  = effectInstance()->getFrameRecursive();
    OfxPointD renderScale;
    effectInstance()->getRenderScaleRecursive(renderScale.x, renderScale.y);
    effectInstance()->beginInstanceChangedAction(kOfxChangeUserEdited);
    effectInstance()->paramInstanceChangedAction(str.toStdString(),kOfxChangeUserEdited,frame,renderScale);
    effectInstance()->endInstanceChangedAction(kOfxChangeUserEdited);
    
    if(isOutputNode())
        getExecutingEngine()->changeDAGAndStartEngine(this);
    else{
        getExecutingEngine()->seekRandomFrame(getExecutingEngine()->getCurrentDAG().getOutput()->getTimeLine().currentFrame());
    }
}


void OfxNode::computePreviewImage(){
    if(effectInstance()->getContext() != kOfxImageEffectContextGenerator){
        return;
    }
    //iterate over param and find if there's an unvalid param
    // e.g: an empty filename
    const map<string,OFX::Host::Param::Instance*>& params = effect_->getParams();
    for (map<string,OFX::Host::Param::Instance*>::const_iterator it = params.begin(); it!=params.end(); ++it) {
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
    effectInstance()->getRegionOfDefinitionAction(1.0, rS, rod);
    OfxRectI renderW;
    renderW.x1 = rod.x1;
    renderW.x2 = rod.x2;
    renderW.y1 = rod.y1;
    renderW.y2 = rod.y2;
    Box2D oldBox(info().dataWindow());
    _info.set_dataWindow(Box2D(rod.x1, rod.y1, rod.x2, rod.y2));
    OfxPointD renderScale;
    renderScale.x = renderScale.y = 1.;
    effectInstance()->beginRenderAction(0, 25, 1, true, renderScale);
    effectInstance()->renderAction(0,kOfxImageFieldNone,renderW, renderScale);
    OfxImage* img = dynamic_cast<OfxImage*>(effectInstance()->getClip("Output")->getImage(0.0,NULL));
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
    _info.set_dataWindow(oldBox);
    effectInstance()->endRenderAction(0, 25, 1, true, renderScale);

    
    _preview = ret;
    _nodeGUI->updatePreviewImageForReader();
}

/*group is a string as such:
 Toto/Superplugins/blabla
 This functions extracts the all parts of such a grouping, e.g in this case
 it would return [Toto,Superplugins,blabla].*/
std::vector<std::string> ofxExtractAllPartsOfGrouping(const std::string& group) {
    std::vector<std::string> out;
    QString str(group.c_str());
    int pos = 0;
    while(pos < str.size()){
        std::string newPart;
        while(pos < str.size() && str.at(pos) != QChar('/') && str.at(pos) != QChar('\\')){
            newPart.append(1,str.at(pos).toLatin1());
            ++pos;
        }
        ++pos;
        out.push_back(newPart);
    }
    return out;
}

const std::string& OfxNode::getShortLabel() const {
    return effect_->getShortLabel();
}

const std::string& OfxNode::getPluginGrouping() const {
    return effect_->getPluginGrouping();
}

