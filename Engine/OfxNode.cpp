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
#include <limits>
#include <QtGui/QImage>
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
#include "Engine/TimeLine.h"

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
:OutputNode(model)
, _tabKnob(0)
, _lastKnobLayoutWithNoNewLine(0)
, _overlayInteract()
, _penDown(false)
, effect_()
, _firstTime(false)
, _isOutput(false)
{
    /*Replicate of the code in OFX::Host::ImageEffect::ImageEffectPlugin::createInstance.
     We need to pass more parameters to the constructor of OfxNode. That means we cannot
     create the OfxNode in the virtual function newInstance. Thus we create before it before
     instanciating the OfxImageEffect. The problem is that calling OFX::Host::ImageEffect::ImageEffectPlugin::createInstance
     creates the OfxImageEffect and calls populate(). populate() will actually create all OfxClipInstance and OfxParamInstance.
     All these subclasses need a valid pointer to an OfxNode. Hence we need to set the pointer to the OfxNode in
     OfxImageEffect BEFORE calling populate().
     */
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
}
OfxNode::~OfxNode(){
    delete effect_;
}



std::string OfxNode::description() const {
    if(effectInstance()){
        return effectInstance()->getProps().getStringProperty(kOfxPropPluginDescription);
    }else{
        return "";
    }
}

void OfxNode::tryInitializeOverlayInteracts(){
    /*create overlay instance if any*/
    OfxPluginEntryPoint *overlayEntryPoint = effect_->getOverlayInteractMainEntry();
    if(overlayEntryPoint){
        _overlayInteract.reset(new OfxOverlayInteract(*effect_,8,true));
        _overlayInteract->createInstanceAction();
    }
}

ChannelSet OfxNode::supportedComponents() const {
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

static std::string getEffectInstanceLabel(const Powiter::OfxImageEffectInstance* effect){
    std::string label = effect->getLabel();
    if(label.empty()){
        label = effect->getShortLabel();
    }
    if(label.empty()){
        label = effect->getLongLabel();
    }
    return label;
}

std::string OfxNode::className() const { // should be const
    std::string label = getEffectInstanceLabel(effectInstance());
    if (label!="Viewer") {
        return label;
    }else{
        const QStringList groups = getPluginGrouping();
        return groups[0].toStdString() + effectInstance()->getLongLabel();
    }
}

std::string OfxNode::setInputLabel(int inputNb) const {
    
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

void ofxRectDToBox2D(const OfxRectD& ofxrect,Box2D* box){
    int xmin = (int)std::floor(ofxrect.x1);
    int ymin = (int)std::floor(ofxrect.y1);
    int xmax = (int)std::ceil(ofxrect.x2);
    int ymax = (int)std::ceil(ofxrect.y2);
    box->set_left(xmin);
    box->set_right(xmax);
    box->set_bottom(ymin);
    box->set_top(ymax);
}

Powiter::Status OfxNode::getRegionOfDefinition(SequenceTime time,Box2D* rod){
    assert(effect_);
    OfxPointD rS;
    rS.x = rS.y = 1.0;
    OfxRectD ofxRod;
    OfxStatus stat = effect_->getRegionOfDefinitionAction(time, rS, ofxRod);
    assert(stat == kOfxStatOK || stat == kOfxStatReplyDefault);
    assert(!(ofxRod.x1 ==  0. && ofxRod.x2 == 0. && ofxRod.y1 == 0. && ofxRod.y2 == 0.));
    ofxRectDToBox2D(ofxRod,rod);
    return (Powiter::Status)stat; // compatible enums
    
    // OFX::Host::ImageEffect::ClipInstance* clip = effectInstance()->getClip(kOfxImageEffectOutputClipName);
    //assert(clip);
    //double pa = clip->getAspectRatio();
}
void OfxNode::getFrameRange(SequenceTime *first,SequenceTime *last){
    OFX::Host::ImageEffect::ClipInstance* clip = effectInstance()->getClip(kOfxImageEffectSimpleSourceClipName);
    assert(clip);
    double f,l;
    clip->getFrameRange(f, l);
    *first = (SequenceTime)f;
    *last = (SequenceTime)l;
}

Powiter::Status OfxNode::preProcessFrame(SequenceTime /*time*/){
    _firstTime = true;
    /*Checking if all mandatory inputs are connected!*/
    MappedInputV ofxInputs = inputClipsCopyWithoutOutput();
    for (U32 i = 0; i < ofxInputs.size(); ++i) {
        if (!ofxInputs[i]->isOptional() && !input(ofxInputs.size()-1-i)) {
            return StatFailed;
        }
    }
    //iterate over param and find if there's an unvalid param
    // e.g: an empty filename
    const map<string,OFX::Host::Param::Instance*>& params = effectInstance()->getParams();
    for (map<string,OFX::Host::Param::Instance*>::const_iterator it = params.begin(); it!=params.end(); ++it) {
        if(it->second->getType() == kOfxParamTypeString){
            OfxStringInstance* param = dynamic_cast<OfxStringInstance*>(it->second);
            assert(param);
            if(!param->isValid()){
                return StatFailed;
            }
        }
    }
    return StatOK;
}

void OfxNode::openFilesForAllFileParams(){
    const map<string,OFX::Host::Param::Instance*>& params = effectInstance()->getParams();
    for (map<string,OFX::Host::Param::Instance*>::const_iterator it = params.begin(); it!=params.end(); ++it) {
        if(it->second->getType() == kOfxParamTypeString){
            OfxStringInstance* param = dynamic_cast<OfxStringInstance*>(it->second);
            assert(param);
            param->ifFileKnobPopDialog();
        }
    }

}

void OfxNode::render(SequenceTime time,Row* out) {
    const ChannelSet& channels = out->channels();
    int y = out->y();
    OfxRectI renderW;
    assert(effectInstance());
    OFX::Host::ImageEffect::ClipInstance* clip = effectInstance()->getClip(kOfxImageEffectOutputClipName);
    assert(clip);
    
    {
        QMutexLocker l(&_firstTimeMutex);
        if(_firstTime){
            _firstTime = false;
            Box2D rod;
            getRegionOfDefinition(time, &rod);
            renderW.x1 = rod.left();
            renderW.x2 = rod.right();
            renderW.y1 = rod.bottom();
            renderW.y2 = rod.top();
            OfxPointD renderScale;
            OfxStatus stat = effectInstance()->renderAction((OfxTime)time, kOfxImageFieldNone, renderW, renderScale);
            assert(stat == kOfxStatOK);
        }
    }
    const OfxImage* img = dynamic_cast<OfxImage*>(clip->getImage(0.0,NULL));
    assert(img);
    if(img->bitDepth() == OfxImage::eBitDepthUByte)
    {
        const OfxRGBAColourB* srcPixels = img->pixelB(out->left(), y);
        assert(srcPixels);
        foreachChannels(chan, channels){
            float* writable = out->begin(chan);
            if(writable){
                ofxPackedBufferToRowPlane<OfxRGBAColourB>(chan, srcPixels, out->right()-out->left(), writable);
            }
        }
    }else if(img->bitDepth() == OfxImage::eBitDepthUShort)
    {
        const OfxRGBAColourS* srcPixels = img->pixelS(out->left(), y);
        assert(srcPixels);
        foreachChannels(chan, channels){
            float* writable = out->begin(chan);
            if(writable){
                ofxPackedBufferToRowPlane<OfxRGBAColourS>(chan, srcPixels, out->right()-out->left(), writable);
            }
        }
    }else if(img->bitDepth() == OfxImage::eBitDepthFloat)
    {
        const OfxRGBAColourF* srcPixels = img->pixelF(out->left(), y);
        assert(srcPixels);
        foreachChannels(chan, channels){
            float* writable = out->begin(chan);
            if(writable){
                ofxPackedBufferToRowPlane<OfxRGBAColourF>(chan, srcPixels, out->right()-out->left(), writable);
            }
        }
    }
}

void OfxNode::onInstanceChanged(const std::string& paramName){
   
    OfxStatus stat;
    stat = effectInstance()->beginInstanceChangedAction(kOfxChangeUserEdited);
    assert(stat == kOfxStatOK || stat == kOfxStatReplyDefault);
    OfxPointD renderScale;
    renderScale.x = renderScale.y = 1.0;
    stat = effectInstance()->paramInstanceChangedAction(paramName, kOfxChangeUserEdited, 1.0,renderScale);
    // note: DON'T remove the following assert()s, unless you replace them with proper error feedback.
    assert(stat == kOfxStatOK || stat == kOfxStatReplyDefault);
    stat = effectInstance()->endInstanceChangedAction(kOfxChangeUserEdited);
    assert(stat == kOfxStatOK || stat == kOfxStatReplyDefault);
}

bool OfxNode::canMakePreviewImage() const { return effectInstance()->getContext() == kOfxImageEffectContextGenerator; }

QImage OfxNode::getPreview(int /*width*/, int /*height*/){
    return _preview;
}

void OfxNode::computePreviewImage(int width,int height){
    assert(effectInstance());
    if(effectInstance()->getContext() != kOfxImageEffectContextGenerator){
        return;
    }
    //iterate over param and find if there's an unvalid param
    // e.g: an empty filename
    const map<string,OFX::Host::Param::Instance*>& params = effectInstance()->getParams();
    for (map<string,OFX::Host::Param::Instance*>::const_iterator it = params.begin(); it!=params.end(); ++it) {
        if(it->second->getType() == kOfxParamTypeString){
            OfxStringInstance* param = dynamic_cast<OfxStringInstance*>(it->second);
            assert(param);
            if(!param->isValid()){
                return;
            }
        }
    }

    _preview = QImage(width, height, QImage::Format_ARGB32);
    OfxPointD rS;
    rS.x = rS.y = 1.0;
    OfxRectD rod;
    //This function calculates the merge of the inputs RoD.
    OfxStatus stat = effectInstance()->beginRenderAction(0, 25, 1, true, rS);
    assert(stat == kOfxStatOK || stat == kOfxStatReplyDefault);
    stat = effectInstance()->getRegionOfDefinitionAction(1.0, rS, rod);
    assert(stat == kOfxStatOK || stat == kOfxStatReplyDefault);
    const Format& format = getProjectDefaultFormat();
    if (stat == kOfxStatReplyDefault) {
        rod.x1 = format.left();
        rod.x2 = format.right();
        rod.y1 = format.bottom();
        rod.y2 = format.top();
    }
    if (rod.x1 == kOfxFlagInfiniteMin) {
        rod.x1 = format.left();
    }
    if (rod.x2 == kOfxFlagInfiniteMax) {
        rod.x2 = format.right();
    }
    if (rod.y1 == kOfxFlagInfiniteMin) {
        rod.y1 = format.bottom();
    }
    if (rod.y2 == kOfxFlagInfiniteMax) {
        rod.y2 = format.top();
    }
    assert(!(rod.x1 ==  0. && rod.x2 == 0. && rod.y1 == 0. && rod.y2 == 0.));
    OfxRectI renderW;
    renderW.x1 = (int)std::floor(rod.x1);
    renderW.y1 = (int)std::floor(rod.y1);
    renderW.x2 = (int)std::ceil(rod.x2);
    renderW.y2 = (int)std::ceil(rod.y2);
    stat = effectInstance()->renderAction(0,kOfxImageFieldNone,renderW, rS);
    assert(stat == kOfxStatOK);
    OFX::Host::ImageEffect::ClipInstance* outputClip = effectInstance()->getClip(kOfxImageEffectOutputClipName);
    assert(outputClip);
    const OfxImage* img = dynamic_cast<OfxImage*>(outputClip->getImage(0.0,NULL));
    assert(img);
    OfxRectI bounds = img->getBounds();
    int w = std::min(bounds.x2-bounds.x1, width);
    int h = std::min(bounds.y2-bounds.y1, height);
    double yZoomFactor = (double)h/(bounds.y2-bounds.y1);
    double xZoomFactor = (double)w/(bounds.x2 - bounds.x1);
    for (int i = 0; i < h; ++i) {
        double y = i / yZoomFactor;
        int nearestY = (int)(y+0.5);
        OfxRGBAColourF* src_pixels = img->pixelF(0,bounds.y2 -1 - nearestY);
        QRgb *dst_pixels = (QRgb *) _preview.scanLine(i);
        assert(src_pixels);
        for(int j = 0 ; j < w ; ++j) {
            double x = (double)j / xZoomFactor;
            int nearestX = (int)(x+0.5);
            OfxRGBAColourF p = src_pixels[nearestX];
            uchar r = (uchar)std::min(p.r*256., 255.);
            uchar g = (uchar)std::min(p.g*256., 255.);
            uchar b = (uchar)std::min(p.b*256., 255.);
            uchar a = (p.a != 0.) ? (uchar)std::min(p.a*256., 255.) : 255;
            dst_pixels[j] = qRgba(r, g, b, a);
        }
    }
    stat = effectInstance()->endRenderAction(0, 25, 1, true, rS);
    assert(stat == kOfxStatOK || stat == kOfxStatReplyDefault);
    
    notifyGuiPreviewChanged();
    
}

/*group is a string as such:
 Toto/Superplugins/blabla
 This functions extracts the all parts of such a grouping, e.g in this case
 it would return [Toto,Superplugins,blabla].*/
QStringList ofxExtractAllPartsOfGrouping(const QString& str,const QString& bundlePath) {
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
    if(!bundlePath.isEmpty()){
        int lastDotPos = bundlePath.lastIndexOf('.');
        QString bundleName = bundlePath.left(lastDotPos);
        lastDotPos = bundleName.lastIndexOf('.');
        bundleName = bundleName.left(lastDotPos);
        lastDotPos = bundleName.lastIndexOf('/');
        if(lastDotPos == -1){
            lastDotPos = bundleName.lastIndexOf('\\');
        }
        if(lastDotPos != -1){
            QString toRemove = bundleName.left(lastDotPos+1);
            bundleName = bundleName.remove(toRemove);
            if(out.size() == 1)
                out.push_front(bundleName);
        }
        
    }
    return out;
}

const std::string& OfxNode::getShortLabel() const {
    return effectInstance()->getShortLabel();
}

const QStringList OfxNode::getPluginGrouping() const {
    int pluginCount = effectInstance()->getPlugin()->getBinary()->getNPlugins();
    QString bundlePath;
    bundlePath = pluginCount  > 1 ? effectInstance()->getPlugin()->getBinary()->getBundlePath().c_str() : "";
    QString grouping = effectInstance()->getDescriptor().getPluginGrouping().c_str();
    QStringList groups = ofxExtractAllPartsOfGrouping(grouping,bundlePath);
    return groups;
}
void OfxNode::swapBuffersOfAttachedViewer(){
    ViewerNode* n = hasViewerConnected();
    if(n){
        n->swapBuffers();
    }
}

void OfxNode::redrawInteractOnAttachedViewer(){
    ViewerNode* n = hasViewerConnected();
    if(n){
        n->redrawViewer();
    }
}

void OfxNode::pixelScaleOfAttachedViewer(double &x,double &y){
    ViewerNode* n = hasViewerConnected();
    if(n){
        n->pixelScale(x, y);
    }
}

void OfxNode::viewportSizeOfAttachedViewer(double &w,double &h){
    ViewerNode* n = hasViewerConnected();
    if(n){
        n->viewportSize(w, h);
    }
}
void OfxNode::backgroundColorOfAttachedViewer(double &r,double &g,double &b){
    ViewerNode* n = hasViewerConnected();
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
        
        OfxStatus stat = _overlayInteract->penDownAction(1.0, rs, penPos, penPosViewport, 1.);
        //assert(stat == kOfxStatOK || stat == kOfxStatReplyDefault);
        if (stat == kOfxStatOK) {
            _penDown = true;
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
        
        OfxStatus stat = _overlayInteract->penMotionAction(1.0, rs, penPos, penPosViewport, 1.);
        // assert(stat == kOfxStatOK || stat == kOfxStatReplyDefault);
        if (stat == kOfxStatOK) {
            if(_penDown){
                ViewerNode* v = hasViewerConnected();
                if(v){
                    v->refreshAndContinueRender();
                }
            }
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
        
        OfxStatus stat = _overlayInteract->penUpAction(1.0, rs, penPos, penPosViewport, 1.);
        //assert(stat == kOfxStatOK || stat == kOfxStatReplyDefault);
        if (stat == kOfxStatOK) {
            _penDown = false;
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
        ViewerNode* v = hasViewerConnected();
        if(v){
            v->refreshAndContinueRender();

        }
    }
    
}

void OfxNode::onOverlayKeyUp(QKeyEvent* e){
    if(_overlayInteract){
        OfxPointD rs;
        rs.x = rs.y = 1.;
        OfxStatus stat = _overlayInteract->keyUpAction(1., rs, e->nativeVirtualKey(), e->text().toLatin1().data());
        assert(stat == kOfxStatOK || stat == kOfxStatReplyDefault);
        if (stat == kOfxStatOK) {
            ViewerNode* v = hasViewerConnected();
            if (v) {
                v->refreshAndContinueRender();
            }
        }
    }
}

void OfxNode::onOverlayKeyRepeat(QKeyEvent* e){
    if(_overlayInteract){
        OfxPointD rs;
        rs.x = rs.y = 1.;
        _overlayInteract->keyRepeatAction(1., rs, e->nativeVirtualKey(), e->text().toLatin1().data());
        ViewerNode* v = hasViewerConnected();
        if(v){
            v->refreshAndContinueRender();

        }
    }
}

void OfxNode::onOverlayFocusGained(){
    if(_overlayInteract){
        OfxPointD rs;
        rs.x = rs.y = 1.;
        OfxStatus stat = _overlayInteract->gainFocusAction(1., rs);
        assert(stat == kOfxStatOK || stat == kOfxStatReplyDefault);
        if (stat == kOfxStatOK) {
            ViewerNode* v = hasViewerConnected();
            if (v) {
                v->refreshAndContinueRender();
            }
        }
    }
}

void OfxNode::onOverlayFocusLost(){
    if(_overlayInteract){
        OfxPointD rs;
        rs.x = rs.y = 1.;
        OfxStatus stat = _overlayInteract->loseFocusAction(1., rs);
        assert(stat == kOfxStatOK || stat == kOfxStatReplyDefault);
        if (stat == kOfxStatOK) {
            ViewerNode* v = hasViewerConnected();
            if (v) {
                v->refreshAndContinueRender();
            }
        }
    }
}
void OfxNode::onFrameRangeChanged(int first,int last){
    _frameRange.first = first;
    _frameRange.second = last;
}
