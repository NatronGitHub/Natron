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


#include "ofxnode.h"
#include "Core/ofxparaminstance.h"
#include "Core/row.h"
#include "Core/ofxclipinstance.h"

using namespace std;
using namespace Powiter;
OfxNode::OfxNode(OFX::Host::ImageEffect::ImageEffectPlugin* plugin,
                 OFX::Host::ImageEffect::Descriptor         &other,
                 const std::string  &context):
Node(),
OFX::Host::ImageEffect::Instance(plugin,other,context,false)
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
    for (vector<string>::const_iterator it = suppComponents.begin(); it!= suppComponents.end(); it++) {
        supportedComp += OfxNode::ofxComponentsToPowiterChannels(*it);
    }
    return supportedComp;
}

bool OfxNode::isOutputNode(){
    return false;
}

bool OfxNode::isInputNode(){
    if(getContext() == kOfxImageEffectContextGenerator)
        return true;
    return false;
}

std::string OfxNode::setInputLabel(int inputNb){
    const std::vector<OFX::Host::ImageEffect::ClipDescriptor*>& clips = getDescriptor().getClipsByOrder();
    if(inputNb < (int)clips.size()){
        return clips[inputNb]->getShortLabel();
    }else{
        return Node::setInputLabel(inputNb);
    }
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
    const ClipsMap clips = getDescriptor().getClips();
    int minimalCount = 0;
    for (ClipsMap::const_iterator it = clips.begin(); it!=clips.end(); it++) {
        if(!it->second->isOptional()){
            minimalCount++;
        }
    }
    return minimalCount;
}
void OfxNode::_validate(bool){
    _firstTime = true;
    if (isInputNode()) {
        OfxPointD rS;
        rS.x = rS.y = 1.0;
        OfxRectD rod;
        getRegionOfDefinitionAction(1.0, rS, rod);
        _info->setDisplayWindow(Format(rod.x1, rod.y1, rod.x2, rod.y2, ""));
        _info->set(rod.x1, rod.y1, rod.x2, rod.y2);
        _info->rgbMode(true);
        _info->setYdirection(1);
        OFX::Host::ImageEffect::ClipInstance* clip = getClip("Source");
        string comp = clip->getUnmappedComponents();
        _info->setChannels(ofxComponentsToPowiterChannels(comp));
    }
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
        const OfxRGBAColourB* srcPixels = img->pixelB(0, y);
        foreachChannels(chan, channels){
            float* writeable = out->writable(chan);
            if(writeable){
                ofxPackedBufferToRowPlane<OfxRGBAColourB>(chan, srcPixels, out->right()-out->offset(), writeable+out->offset());
            }
        }
    }else if(img->bitDepth() == OfxImage::eBitDepthUShort)
    {
        const OfxRGBAColourS* srcPixels = img->pixelS(0, y);
        foreachChannels(chan, channels){
            float* writeable = out->writable(chan);
            if(writeable){
                ofxPackedBufferToRowPlane<OfxRGBAColourS>(chan, srcPixels, out->right()-out->offset(), writeable+out->offset());
            }
        }
    }else if(img->bitDepth() == OfxImage::eBitDepthFloat)
    {
        const OfxRGBAColourF* srcPixels = img->pixelF(0, y);
        foreachChannels(chan, channels){
            float* writeable = out->writable(chan);
            if(writeable){
                ofxPackedBufferToRowPlane<OfxRGBAColourF>(chan, srcPixels, out->right()-out->offset(), writeable+out->offset());
            }
        }
    }
}

// make a parameter instance
OFX::Host::Param::Instance* OfxNode::newParam(const std::string& name, OFX::Host::Param::Descriptor& descriptor)
{
    if(descriptor.getType()==kOfxParamTypeInteger)
        return new OfxIntegerInstance(this,name,descriptor);
    else if(descriptor.getType()==kOfxParamTypeDouble)
        return new OfxDoubleInstance(this,name,descriptor);
    else if(descriptor.getType()==kOfxParamTypeBoolean)
        return new OfxBooleanInstance(this,name,descriptor);
    else if(descriptor.getType()==kOfxParamTypeChoice)
        return new OfxChoiceInstance(this,name,descriptor);
    else if(descriptor.getType()==kOfxParamTypeRGBA)
        return new OfxRGBAInstance(this,name,descriptor);
    else if(descriptor.getType()==kOfxParamTypeRGB)
        return new OfxRGBInstance(this,name,descriptor);
    else if(descriptor.getType()==kOfxParamTypeDouble2D)
        return new OfxDouble2DInstance(this,name,descriptor);
    else if(descriptor.getType()==kOfxParamTypeInteger2D)
        return new OfxInteger2DInstance(this,name,descriptor);
    else if(descriptor.getType()==kOfxParamTypePushButton)
        return new OfxPushButtonInstance(this,name,descriptor);
    else if(descriptor.getType()==kOfxParamTypeGroup)
        return new OFX::Host::Param::GroupInstance(descriptor,this);
    else if(descriptor.getType()==kOfxParamTypePage)
        return new OFX::Host::Param::PageInstance(descriptor,this);
    else
        return 0;
}

void OfxNode::onInstanceChangedAction(const QString& str){
    double frame  = getFrameRecursive();
    OfxPointD renderScale;
    getRenderScaleRecursive(renderScale.x, renderScale.y);
    
    beginInstanceChangedAction(kOfxChangeUserEdited);
    paramInstanceChangedAction(str.toStdString(),kOfxChangeUserEdited,frame,renderScale);
    endInstanceChangedAction(kOfxChangeUserEdited);
}

OfxStatus OfxNode::editBegin(const std::string& name)
{
    return kOfxStatErrMissingHostFeature;
}

OfxStatus OfxNode::editEnd(){
    return kOfxStatErrMissingHostFeature;
}

/// Start doing progress.
void  OfxNode::progressStart(const std::string &message)
{
}

/// finish yer progress
void  OfxNode::progressEnd()
{
}

/// set the progress to some level of completion, returns
/// false if you should abandon processing, true to continue
bool  OfxNode::progressUpdate(double t)
{
    return true;
}


/// get the current time on the timeline. This is not necessarily the same
/// time as being passed to an action (eg render)
double  OfxNode::timeLineGetTime()
{
    return 0;
}

/// set the timeline to a specific time
void  OfxNode::timeLineGotoTime(double t)
{
}

/// get the first and last times available on the effect's timeline
void  OfxNode::timeLineGetBounds(double &t1, double &t2)
{
    t1 = 1;
    t2 = 1;
}
