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
#include <QImage>
#if QT_VERSION < 0x050000
CLANG_DIAG_OFF(unused-private-field);
#include <QtGui/qmime.h>
CLANG_DIAG_ON(unused-private-field);
#endif
#include <QKeyEvent>

#include "Global/AppManager.h"

#include "Engine/OfxParamInstance.h"
#include "Engine/Row.h"
#include "Engine/OfxClipInstance.h"
#include "Engine/OfxImageEffectInstance.h"
#include "Engine/OfxOverlayInteract.h"
#include "Engine/Model.h"
#include "Engine/ViewerNode.h"
#include "Engine/VideoEngine.h"

#include "Writers/Writer.h"

#include <ofxhPluginCache.h>
#include <ofxhPluginAPICache.h>
#include <ofxhImageEffect.h>
#include <ofxhImageEffectAPI.h>
#include <ofxhHost.h>

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

OfxNode::OfxNode(Model* model,
                 OFX::Host::ImageEffect::ImageEffectPlugin* plugin,
                 const std::string& context)
: OutputNode(model)
, _tabKnob(0)
, _lastKnobLayoutWithNoNewLine(0)
, _firstTimeMutex()
, _firstTime(false)
, _isOutput(false)
, _preview(NULL)
, _canHavePreview(false)
, effect_(NULL)
, _overlayInteract(NULL)
{
    /*Replicate of the code in OFX::Host::ImageEffect::ImageEffectPlugin::createInstance.
     We need to pass more parameters to the constructor of OfxNode. That means we cannot
     create the OfxNode in the virtual function newInstance. Thus we create before it before
     instanciating the OfxImageEffect. The problem is that calling OFX::Host::ImageEffect::ImageEffectPlugin::createInstance
     creates the OfxImageEffect and calls populate(). populate() will actually create all OfxClipInstance and OfxParamInstance.
     All these subclasses need a valid pointer to an OfxNode. Hence we need to set the pointer to the OfxNode in 
     OfxImageEffect BEFORE calling populate(). 
     */
    assert(plugin);
    OFX::Host::PluginHandle* ph = plugin->getPluginHandle();
    (void)ph;
    OFX::Host::ImageEffect::Descriptor* desc = NULL;
    try {
        desc = plugin->getContext(context);
    } catch (const std::exception &e) {
        cout << "Error: Caught exception while creating OfxNode: " << e.what() << std::endl;
        throw;
    }
    if (desc) {
        try {
            effect_ = new Powiter::OfxImageEffectInstance(plugin,*desc,context,false);
            assert(effect_);
            effect_->setOfxNodePointer(this);
            OfxStatus stat = effect_->populate();
            assert(stat == kOfxStatOK); // Prop Tester crashes here
        } catch (const std::exception &e) {
            cout << "Error: Caught exception while creating OfxImageEffectInstance: " << e.what() << std::endl;
            throw;
        }
    }
    if(context == kOfxImageEffectContextGenerator){
        _canHavePreview = true;
    }
    
    
}

OfxNode::~OfxNode() {
    // An interact instance must always be associated with an effect instance. So it gets created after an effect and destroyed before one.
    if(_overlayInteract)
        delete _overlayInteract;
    delete effect_;
}

void OfxNode::tryInitializeOverlayInteracts(){
    /*create overlay instance if any*/
    OfxPluginEntryPoint *overlayEntryPoint = effect_->getOverlayInteractMainEntry();
    if(overlayEntryPoint){
        _overlayInteract = new OfxOverlayInteract(*effect_,8,true);
        _overlayInteract->createInstanceAction();
    }
}

ChannelSet OfxNode::supportedComponents() {
    // FIXME: should be const...
    assert(effectInstance());
    const OFX::Host::ImageEffect::ClipInstance* clip = effectInstance()->getClip(kOfxImageEffectOutputClipName);
    assert(clip);
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
    assert(effectInstance());
    if(effectInstance()->getContext() == kOfxImageEffectContextGenerator)
        return true;
    return false;
}

std::string OfxNode::className() { // should be const
    assert(effectInstance());
    std::string label = getShortLabel();
    if(label.empty()){
        label = effectInstance()->getLabel();
    }
    if (label!="Viewer") {
        return label;
    }else{
        QString grouping = effectInstance()->getDescriptor().getPluginGrouping().c_str();
        QStringList groups = ofxExtractAllPartsOfGrouping(grouping);
        return groups[0].toStdString() + effectInstance()->getLongLabel();
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
    assert(effectInstance());
    const std::vector<OFX::Host::ImageEffect::ClipDescriptor*>& clips = effectInstance()->getDescriptor().getClipsByOrder();
    MappedInputV copy;
    for (U32 i = 0; i < clips.size(); ++i) {
        assert(clips[i]);
        if(clips[i]->getShortLabel() != kOfxImageEffectOutputClipName){
            copy.push_back(clips[i]);
            // cout << "Clip[" << i << "] = " << clips[i]->getShortLabel() << endl;
        }
    }
    return copy;
}

int OfxNode::maximumInputs() const {
    if(isInputNode()){
        return 0;
    } else {
        assert(effectInstance());
        int totalClips = effectInstance()->getDescriptor().getClips().size();
        return totalClips-1;
    }
    
}

int OfxNode::minimumInputs() const {
    assert(effectInstance());
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
bool OfxNode::_validate(bool doFullWork){
    QMutexLocker g(&_firstTimeMutex);
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
        assert(effectInstance());
        OFX::Host::ImageEffect::ClipInstance* clip = effectInstance()->getClip(kOfxImageEffectOutputClipName);
        assert(clip);
        /*if doFullWork is true we need to pass down the tree all the infos generated
         besides just the frame range.*/
        if (doFullWork) {
            OfxPointD rS;
            rS.x = rS.y = 1.0;
            OfxRectD rod;
            //This function calculates the merge of the inputs RoD.
            OfxStatus stat = effectInstance()->getRegionOfDefinitionAction(1.0, rS, rod);
            assert(stat == kOfxStatOK);
            const Format& format = getModel()->getApp()->getProjectFormat();
            if(rod.x1 == kOfxFlagInfiniteMin){
                rod.x1 = format.left();
            }
            if(rod.x2 == kOfxFlagInfiniteMax){
                rod.x2 = format.right();
            }
            if(rod.y1 == kOfxFlagInfiniteMin){
                rod.y1 = format.bottom();
            }
            if(rod.y2 == kOfxFlagInfiniteMax){
                rod.y2 = format.top();
            }
            int xmin = (int)std::floor(rod.x1);
            int ymin = (int)std::floor(rod.y1);
            int xmax = (int)std::ceil(rod.x2);
            int ymax = (int)std::ceil(rod.y2);
            double pa = clip->getAspectRatio();
            //  cout << "pa : " << pa << endl;
            //we just set the displayWindow/dataWindow rather than merge it
            _info.set_displayWindow(Format(xmin, ymin, xmax, ymax, "",pa));
            _info.set_dataWindow(Box2D(xmin, ymin, xmax, ymax));
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


void OfxNode::openFilesForAllFileParams(){
    const map<string,OFX::Host::Param::Instance*>& params = effect_->getParams();
    for (map<string,OFX::Host::Param::Instance*>::const_iterator it = params.begin(); it!=params.end(); ++it) {
        if(it->second->getType() == kOfxParamTypeString){
            OfxStringInstance* param = dynamic_cast<OfxStringInstance*>(it->second);
            assert(param);
            param->ifFileKnobPopDialog();
        }
    }

}

void OfxNode::engine(int y, int /*offset*/, int /*range*/, ChannelSet channels, Row* out) {
    OfxRectI renderW;
    const Format& dispW = info().displayWindow();
    assert(effectInstance());
    renderW.x1 = dispW.left();
    renderW.x2 = dispW.right();
    renderW.y1 = dispW.bottom();
    renderW.y2 = dispW.top();
    OfxPointD renderScale;
    effectInstance()->getRenderScaleRecursive(renderScale.x, renderScale.y);
    {
        QMutexLocker g(&_firstTimeMutex);
        if(_firstTime){
            _firstTime = false;
            OfxStatus stat = effectInstance()->renderAction(0,kOfxImageFieldNone,renderW, renderScale);
            assert(stat == kOfxStatOK);

        }
    }
    //the input clips and output clip are filled at this point, we need to copy the output image
    //into the rows, each thread handle a specific row again (the mutex locker is dead).

    OFX::Host::ImageEffect::ClipInstance* clip = effectInstance()->getClip(kOfxImageEffectOutputClipName);
    assert(clip);
    const OfxImage* img = dynamic_cast<OfxImage*>(clip->getImage(0.0,NULL));
    assert(img);
    if(img->bitDepth() == OfxImage::eBitDepthUByte)
    {
        const OfxRGBAColourB* srcPixels = img->pixelB(out->offset(), y);
        assert(srcPixels);
        foreachChannels(chan, channels){
            float* writeable = out->writable(chan);
            if(writeable){
                ofxPackedBufferToRowPlane<OfxRGBAColourB>(chan, srcPixels, out->right()-out->offset(), writeable+out->offset());
            }
        }
    }else if(img->bitDepth() == OfxImage::eBitDepthUShort)
    {
        const OfxRGBAColourS* srcPixels = img->pixelS(out->offset(), y);
        assert(srcPixels);
        foreachChannels(chan, channels){
            float* writeable = out->writable(chan);
            if(writeable){
                ofxPackedBufferToRowPlane<OfxRGBAColourS>(chan, srcPixels, out->right()-out->offset(), writeable+out->offset());
            }
        }
    }else if(img->bitDepth() == OfxImage::eBitDepthFloat)
    {
        const OfxRGBAColourF* srcPixels = img->pixelF(out->offset(), y);
        assert(srcPixels);
        foreachChannels(chan, channels){
            float* writeable = out->writable(chan);
            if(writeable){
                ofxPackedBufferToRowPlane<OfxRGBAColourF>(chan, srcPixels, out->right()-out->offset(), writeable+out->offset());
            }
        }
    }
}

//
//void OfxNode::onInstanceChangedAction(const QString& str){
//    double frame  = effectInstance()->getFrameRecursive();
//    OfxPointD renderScale;
//    effectInstance()->getRenderScaleRecursive(renderScale.x, renderScale.y);
//    effectInstance()->beginInstanceChangedAction(kOfxChangeUserEdited);
//    effectInstance()->paramInstanceChangedAction(str.toStdString(),kOfxChangeUserEdited,frame,renderScale);
//    effectInstance()->endInstanceChangedAction(kOfxChangeUserEdited);
//    
//    if(isOutputNode())
//        getExecutingEngine()->changeDAGAndStartEngine(this);
//    else{
//        getExecutingEngine()->seekRandomFrame(getExecutingEngine()->getCurrentDAG().getOutput()->getTimeLine().currentFrame());
//    }
//}


void OfxNode::computePreviewImage(){
    assert(effectInstance());
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
    assert(ret);
    OfxPointD rS;
    rS.x = rS.y = 1.0;
    OfxRectD rod;
    //This function calculates the merge of the inputs RoD.
    OfxStatus stat = effectInstance()->beginRenderAction(0, 25, 1, true, rS);
    assert(stat == kOfxStatOK || stat == kOfxStatReplyDefault);
    stat = effectInstance()->getRegionOfDefinitionAction(1.0, rS, rod);
    assert(stat == kOfxStatOK);
    const Format& format = getModel()->getApp()->getProjectFormat();
    if(rod.x1 == kOfxFlagInfiniteMin){
        rod.x1 = format.left();
    }
    if(rod.x2 == kOfxFlagInfiniteMax){
        rod.x2 = format.right();
    }
    if(rod.y1 == kOfxFlagInfiniteMin){
        rod.y1 = format.bottom();
    }
    if(rod.y2 == kOfxFlagInfiniteMax){
        rod.y2 = format.top();
    }
    OfxRectI renderW;
    renderW.x1 = (int)std::floor(rod.x1);
    renderW.y1 = (int)std::floor(rod.y1);
    renderW.x2 = (int)std::ceil(rod.x2);
    renderW.y2 = (int)std::ceil(rod.y2);
    Box2D oldBox(info().dataWindow());
    _info.set_dataWindow(Box2D(renderW.x1, renderW.y1, renderW.x2, renderW.y2));
    stat = effectInstance()->renderAction(0,kOfxImageFieldNone,renderW, rS);
    assert(stat == kOfxStatOK);
    OFX::Host::ImageEffect::ClipInstance* outputClip = effectInstance()->getClip(kOfxImageEffectOutputClipName);
    assert(outputClip);
    const OfxImage* img = dynamic_cast<OfxImage*>(outputClip->getImage(0.0,NULL));
    assert(img);
    OfxRectI bounds = img->getBounds();
    int w = std::min(bounds.x2-bounds.x1, 64);
    int h = std::min(bounds.y2-bounds.y1, 64);
    double zoomFactor = (double)h/(bounds.y2-bounds.y1);
    for (int i = 0; i < h; ++i) {
        double y = i / zoomFactor;
        int nearestY = (int)(y+0.5);
        OfxRGBAColourF* src_pixels = img->pixelF(0,bounds.y2 -1 - nearestY);
        QRgb *dst_pixels = (QRgb *) ret->scanLine(i);
        assert(src_pixels);
        for(int j = 0 ; j < w ; ++j) {
            double x = j / zoomFactor;
            int nearestX = (int)(x+0.5);
            OfxRGBAColourF p = src_pixels[nearestX];
            uchar r = (uchar)std::min(p.r*256., 255.);
            uchar g = (uchar)std::min(p.g*256., 255.);
            uchar b = (uchar)std::min(p.b*256., 255.);
            uchar a = (p.a != 0.) ? (uchar)std::min(p.a*256., 255.) : 255;
            dst_pixels[j] = qRgba(r, g, b, a);
        }
    }
    _info.set_dataWindow(oldBox);
    stat = effectInstance()->endRenderAction(0, 25, 1, true, rS);
    assert(stat == kOfxStatOK || stat == kOfxStatReplyDefault);
    
    _preview = ret;
    notifyGuiPreviewChanged();
}

/*group is a string as such:
 Toto/Superplugins/blabla
 This functions extracts the all parts of such a grouping, e.g in this case
 it would return [Toto,Superplugins,blabla].*/
QStringList ofxExtractAllPartsOfGrouping(const QString& str) {
    QStringList out;
    int pos = 0;
    while(pos < str.size()){
        QString newPart;
        while(pos < str.size() && str.at(pos) != QChar('/') && str.at(pos) != QChar('\\')){
            newPart.append(str.at(pos));
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
void OfxNode::swapBuffersOfAttachedViewer(){
    ViewerNode* n = hasViewerConnected(this);
    if(n){
        n->swapBuffers();
    }
}

void OfxNode::redrawInteractOnAttachedViewer(){
    ViewerNode* n = hasViewerConnected(this);
    if(n){
        n->redrawViewer();
    }
}

void OfxNode::pixelScaleOfAttachedViewer(double &x,double &y){
    ViewerNode* n = hasViewerConnected(this);
    if(n){
        n->pixelScale(x, y);
    }
}

void OfxNode::viewportSizeOfAttachedViewer(double &w,double &h){
    ViewerNode* n = hasViewerConnected(this);
    if(n){
        n->viewportSize(w, h);
    }
}
void OfxNode::backgroundColorOfAttachedViewer(double &r,double &g,double &b){
    ViewerNode* n = hasViewerConnected(this);
    if(n){
        n->backgroundColor(r, g, b);
    }
}

void OfxNode::drawOverlay(){
    if(_overlayInteract){
        OfxPointD rs;
        rs.x = rs.y = 1.;
        _overlayInteract->drawAction(1.0, rs);
    }
}


bool OfxNode::onOverlayPenDown(const QPointF& viewportPos,const QPointF& pos){
    if(_overlayInteract){
        OfxPointD rs;
        rs.x = rs.y = 1.;
        OfxPointD penPos;
        penPos.x = pos.x();
        penPos.y = pos.y();
        OfxPointI penPosViewport;
        penPosViewport.x = viewportPos.x();
        penPosViewport.y = viewportPos.y();
        
        OfxStatus st = _overlayInteract->penDownAction(1.0, rs, penPos, penPosViewport, 1.);
        if(st == kOfxStatOK){
            return true;
        }
    }
    return false;
}

bool OfxNode::onOverlayPenMotion(const QPointF& viewportPos,const QPointF& pos){
    if(_overlayInteract){
        OfxPointD rs;
        rs.x = rs.y = 1.;
        OfxPointD penPos;
        penPos.x = pos.x();
        penPos.y = pos.y();
        OfxPointI penPosViewport;
        penPosViewport.x = viewportPos.x();
        penPosViewport.y = viewportPos.y();
        
        OfxStatus st = _overlayInteract->penMotionAction(1.0, rs, penPos, penPosViewport, 1.);
        if(st == kOfxStatOK){
            return true;
        }
    }
    return false;
}


bool OfxNode::onOverlayPenUp(const QPointF& viewportPos,const QPointF& pos){
    if(_overlayInteract){
        OfxPointD rs;
        rs.x = rs.y = 1.;
        OfxPointD penPos;
        penPos.x = pos.x();
        penPos.y = pos.y();
        OfxPointI penPosViewport;
        penPosViewport.x = viewportPos.x();
        penPosViewport.y = viewportPos.y();
        
        OfxStatus st= _overlayInteract->penUpAction(1.0, rs, penPos, penPosViewport, 1.);
        if(st == kOfxStatOK){
            return true;
        }
    }
    return false;
}

void OfxNode::onOverlayKeyDown(QKeyEvent* e){
    if(_overlayInteract){
        OfxPointD rs;
        rs.x = rs.y = 1.;
        _overlayInteract->keyDownAction(1., rs, e->nativeVirtualKey(), e->text().toLatin1().data());
    }
}

void OfxNode::onOverlayKeyUp(QKeyEvent* e){
    if(_overlayInteract){
        OfxPointD rs;
        rs.x = rs.y = 1.;
        _overlayInteract->keyUpAction(1., rs, e->nativeVirtualKey(), e->text().toLatin1().data());
    }
}

void OfxNode::onOverlayKeyRepeat(QKeyEvent* e){
    if(_overlayInteract){
        OfxPointD rs;
        rs.x = rs.y = 1.;
        _overlayInteract->keyRepeatAction(1., rs, e->nativeVirtualKey(), e->text().toLatin1().data());
    }
}

void OfxNode::onOverlayFocusGained(){
    if(_overlayInteract){
        OfxPointD rs;
        rs.x = rs.y = 1.;
        _overlayInteract->gainFocusAction(1., rs);
    }
}

void OfxNode::onOverlayFocusLost(){
    if(_overlayInteract){
        OfxPointD rs;
        rs.x = rs.y = 1.;
        _overlayInteract->loseFocusAction(1., rs);
    }
}

const QString OfxNode::getRandomFrameName(int f) const{
    if (effectInstance()->getContext() == kOfxImageEffectContextGenerator) {
        const map<string,OFX::Host::Param::Instance*>& params = effect_->getParams();
        for (map<string,OFX::Host::Param::Instance*>::const_iterator it = params.begin(); it!=params.end(); ++it) {
            if(it->second->getType() == kOfxParamTypeString){
                OfxStringInstance* param = dynamic_cast<OfxStringInstance*>(it->second);
                assert(param);
                return param->getRandomFrameName(f);
            }
        }
    }
    return "";
}
