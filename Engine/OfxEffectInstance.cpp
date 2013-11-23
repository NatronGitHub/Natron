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

#include "OfxEffectInstance.h"

#include <locale>
#include <limits>
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
#include "Engine/ViewerInstance.h"
#include "Engine/VideoEngine.h"
#include "Engine/TimeLine.h"
#include "Engine/Project.h"

#include "Writers/Writer.h"

#include <ofxhPluginCache.h>
#include <ofxhPluginAPICache.h>
#include <ofxhImageEffect.h>
#include <ofxhImageEffectAPI.h>
#include <ofxhHost.h>

using namespace Natron;
using std::cout; using std::endl;
#if 0
namespace {
ChannelSet ofxComponentsToNatronChannels(const std::string& comp) {
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
#endif

OfxEffectInstance::OfxEffectInstance(Natron::Node* node)
    :Natron::OutputEffectInstance(node)
    , effect_()
    , _isOutput(false)
    , _penDown(false)
    , _overlayInteract()
    , _tabKnob(0)
    , _lastKnobLayoutWithNoNewLine(0)
{
    if(!node->getLiveInstance()){
        node->setLiveInstance(this);
    }
}

void OfxEffectInstance::createOfxImageEffectInstance(OFX::Host::ImageEffect::ImageEffectPlugin* plugin,
                                                     const std::string& context){
    /*Replicate of the code in OFX::Host::ImageEffect::ImageEffectPlugin::createInstance.
     We need to pass more parameters to the constructor . That means we cannot
     create it in the virtual function newInstance. Thus we create it before
     instanciating the OfxImageEffect. The problem is that calling OFX::Host::ImageEffect::ImageEffectPlugin::createInstance
     creates the OfxImageEffect and calls populate(). populate() will actually create all OfxClipInstance and OfxParamInstance.
     All these subclasses need a valid pointer to an this. Hence we need to set the pointer to this in
     OfxImageEffect BEFORE calling populate().
     */
    OFX::Host::PluginHandle* ph = plugin->getPluginHandle();
    (void)ph;
    OFX::Host::ImageEffect::Descriptor* desc = NULL;
    try {
        desc = plugin->getContext(context);
    } catch (const std::exception &e) {
        throw e;
    }
    if (desc) {
        try {
            effect_ = new Natron::OfxImageEffectInstance(plugin,*desc,context,false);
            assert(effect_);
            effect_->setOfxEffectInstancePointer(this);
            OfxStatus stat = effect_->populate();

            if (stat != kOfxStatOK) {
                throw std::runtime_error("Error while populating the Ofx image effect");
            }
        } catch (const std::exception &e) {
            cout << "Error: Caught exception while creating OfxImageEffectInstance: " << e.what() << std::endl;
            throw;
        }
    }
}

OfxEffectInstance::~OfxEffectInstance(){
    // delete effect_;
}



std::string OfxEffectInstance::description() const {
    if(effectInstance()){
        return effectInstance()->getProps().getStringProperty(kOfxPropPluginDescription);
    }else{
        return "";
    }
}

void OfxEffectInstance::tryInitializeOverlayInteracts(){
    /*create overlay instance if any*/
    if(isLiveInstance()){
        OfxPluginEntryPoint *overlayEntryPoint = effect_->getOverlayInteractMainEntry();
        if(overlayEntryPoint){
            _overlayInteract.reset(new OfxOverlayInteract(*effect_,8,true));
            _overlayInteract->createInstanceAction();
        }
    }
}

bool OfxEffectInstance::isOutput() const {
    return _isOutput;
}

bool OfxEffectInstance::isGenerator() const {
    assert(effectInstance());
    const std::set<std::string>& contexts = effectInstance()->getPlugin()->getContexts();
    std::set<std::string>::const_iterator foundGenerator = contexts.find(kOfxImageEffectContextGenerator);
    if(foundGenerator != contexts.end())
        return true;
    return false;
}

bool OfxEffectInstance::isGeneratorAndFilter() const {
    const std::set<std::string>& contexts = effectInstance()->getPlugin()->getContexts();
    std::set<std::string>::const_iterator foundGenerator = contexts.find(kOfxImageEffectContextGenerator);
    std::set<std::string>::const_iterator foundGeneral = contexts.find(kOfxImageEffectContextGenerator);
    return foundGenerator!=contexts.end() && foundGeneral!=contexts.end();
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

QStringList OfxEffectInstance::getPluginGrouping(const std::string& bundlePath,int pluginsCount,const std::string& grouping){
    std::string bundlePathToUse;
    bundlePathToUse = pluginsCount  > 1 ? bundlePath : "";
    return  ofxExtractAllPartsOfGrouping(grouping.c_str(),bundlePathToUse.c_str());
}

std::string OfxEffectInstance::generateImageEffectClassName(const std::string& shortLabel,
                                         const std::string& label,
                                         const std::string& longLabel,
                                         int pluginsCount,
                                         const std::string& bundlePath,
                                         const std::string& grouping){
    std::string labelToUse = label;
    if(labelToUse.empty()){
        labelToUse = shortLabel;
    }
    if(labelToUse.empty()){
        labelToUse = longLabel;
    }
    QStringList groups = getPluginGrouping(bundlePath,pluginsCount,grouping);

    if(labelToUse == "Viewer"){ // we don't want a plugin to have the same name as our viewer
        labelToUse =  groups[0].toStdString() + longLabel;
    }
    if (groups.size() >= 1) {
        labelToUse.append("  [");
        labelToUse.append(groups[0].toStdString());
        labelToUse.append("]");
    }
    
    return labelToUse;
}

std::string OfxEffectInstance::className() const {
    return generateImageEffectClassName(effect_->getShortLabel(),
                                        effect_->getLabel(),
                                        effect_->getLongLabel(),
                                        effect_->getPlugin()->getBinary()->getNPlugins(),
                                        effect_->getPlugin()->getBinary()->getBundlePath(),
                                        effect_->getPluginGrouping());
}

std::string OfxEffectInstance::inputLabel(int inputNb) const {
    
    MappedInputV copy = inputClipsCopyWithoutOutput();
    if(inputNb < (int)copy.size()){
        return copy[copy.size()-1-inputNb]->getShortLabel();
    }else{
        return EffectInstance::inputLabel(inputNb);
    }
}
OfxEffectInstance::MappedInputV OfxEffectInstance::inputClipsCopyWithoutOutput() const {
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

int OfxEffectInstance::maximumInputs() const {
    if(isGenerator() && !isGeneratorAndFilter()){
        return 0;
    } else {
        assert(effectInstance());
        int totalClips = effectInstance()->getDescriptor().getClips().size();
        return totalClips-1;
    }
    
}

bool OfxEffectInstance::isInputOptional(int inputNb) const {
    MappedInputV inputs = inputClipsCopyWithoutOutput();
    return inputs[inputs.size()-1-inputNb]->isOptional();
}

void ofxRectDToRectI(const OfxRectD& ofxrect,RectI* box){
    int xmin = (int)std::floor(ofxrect.x1);
    int ymin = (int)std::floor(ofxrect.y1);
    int xmax = (int)std::ceil(ofxrect.x2);
    int ymax = (int)std::ceil(ofxrect.y2);
    box->set_left(xmin);
    box->set_right(xmax);
    box->set_bottom(ymin);
    box->set_top(ymax);
}

void OfxEffectInstance::ifInfiniteclipRectToProjectDefault(OfxRectD* rod) const{
    /*If the rod is infinite clip it to the project's default*/
    const Format& projectDefault = getRenderFormat();
    // BE CAREFUL:
    // std::numeric_limits<int>::infinity() does not exist (check std::numeric_limits<int>::has_infinity)
    if (rod->x1 == kOfxFlagInfiniteMin || rod->x1 == -std::numeric_limits<double>::infinity()) {
        rod->x1 = projectDefault.left();
    }
    if (rod->y1 == kOfxFlagInfiniteMin || rod->y1 == -std::numeric_limits<double>::infinity()) {
        rod->y1 = projectDefault.bottom();
    }
    if (rod->x2== kOfxFlagInfiniteMax || rod->x2 == std::numeric_limits<double>::infinity()) {
       rod->x2 = projectDefault.right();
    }
    if (rod->y2 == kOfxFlagInfiniteMax || rod->y2  == std::numeric_limits<double>::infinity()) {
        rod->y2 = projectDefault.top();
    }
    
}

Natron::Status OfxEffectInstance::getRegionOfDefinition(SequenceTime time,RectI* rod){
    assert(effect_);
    OfxPointD rS;
    rS.x = rS.y = 1.0;
    OfxRectD ofxRod;
    OfxStatus stat = effect_->getRegionOfDefinitionAction(time, rS, ofxRod);
    if((stat!= kOfxStatOK && stat != kOfxStatReplyDefault) ||
            (ofxRod.x1 ==  0. && ofxRod.x2 == 0. && ofxRod.y1 == 0. && ofxRod.y2 == 0.))
        return StatFailed;
    ifInfiniteclipRectToProjectDefault(&ofxRod);
    ofxRectDToRectI(ofxRod,rod);
    if(isGenerator()){
        getApp()->getProject()->lock();
        if(getApp()->getProject()->shouldAutoSetProjectFormat()){
            getApp()->getProject()->setAutoSetProjectFormat(false);
            Format dispW;
            dispW.set(*rod);
            getApp()->getProject()->setProjectDefaultFormat(dispW);
        }else{
            Format dispW;
            dispW.set(*rod);

            getApp()->tryAddProjectFormat(dispW);
        }
        getApp()->getProject()->unlock();
    }
    return StatOK;
    
    // OFX::Host::ImageEffect::ClipInstance* clip = effectInstance()->getClip(kOfxImageEffectOutputClipName);
    //assert(clip);
    //double pa = clip->getAspectRatio();
}

OfxRectD rectToOfxRect2D(const RectI b){
    OfxRectD out;
    out.x1 = b.left();
    out.x2 = b.right();
    out.y1 = b.bottom();
    out.y2 = b.top();
    return out;
}


EffectInstance::RoIMap OfxEffectInstance::getRegionOfInterest(SequenceTime time,RenderScale scale,const RectI& renderWindow) {
    std::map<OFX::Host::ImageEffect::ClipInstance*,OfxRectD> inputRois;
    EffectInstance::RoIMap ret;
    OfxStatus stat = effect_->getRegionOfInterestAction((OfxTime)time, scale, rectToOfxRect2D(renderWindow), inputRois);
    if(stat != kOfxStatOK && stat != kOfxStatReplyDefault)
        return ret;
    for(std::map<OFX::Host::ImageEffect::ClipInstance*,OfxRectD>::iterator it = inputRois.begin();it!= inputRois.end();++it){
        EffectInstance* inputNode = dynamic_cast<OfxClipInstance*>(it->first)->getAssociatedNode();
        if(inputNode){
            RectI inputRoi;
            ofxRectDToRectI(it->second, &inputRoi);
            ret.insert(std::make_pair(inputNode,inputRoi));
        }
    }
    return ret;
}


void OfxEffectInstance::getFrameRange(SequenceTime *first,SequenceTime *last){
    OfxRangeD range;
    OfxStatus st = effect_->getTimeDomainAction(range);
    if(st == kOfxStatOK){
        *first = (SequenceTime)range.min;
        *last = (SequenceTime)range.max;
    }else if(st == kOfxStatReplyDefault){
        //The default is...
        int nthClip = effect_->getNClips();
        if(nthClip == 0){
            //infinite if there are no non optional input clips.
            *first = INT_MIN;
            *last = INT_MAX;
        }else{
            //the union of all the frame ranges of the non optional input clips.
            for (int i = 0; i < nthClip ; ++i) {
                OFX::Host::ImageEffect::ClipInstance* clip = effect_->getNthClip(i);
                assert(clip);
                if (!clip->isOutput()) {
                    double f,l;
                    clip->getFrameRange(f, l);
                    if (i != 0) {
                        if (f < *first) {
                            *first = f;
                        }
                        if (l > *last) {
                            *last = l;
                        }
                    } else {
                        *first = f;
                        *last = l;
                    }
                }
            }
        }
    }
}
    

Natron::Status OfxEffectInstance::preProcessFrame(SequenceTime /*time*/){
    //if(!isGenerator() && !isGeneratorAndFilter()){
        /*Checking if all mandatory inputs are connected!*/
        MappedInputV ofxInputs = inputClipsCopyWithoutOutput();
        for (U32 i = 0; i < ofxInputs.size(); ++i) {
            if (!ofxInputs[i]->isOptional() && !input(ofxInputs.size()-1-i)) {
                return StatFailed;
            }
        }
    //  }
    //iterate over param and find if there's an unvalid param
    // e.g: an empty filename
    const std::map<std::string,OFX::Host::Param::Instance*>& params = effectInstance()->getParams();
    for (std::map<std::string,OFX::Host::Param::Instance*>::const_iterator it = params.begin(); it!=params.end(); ++it) {
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

Natron::Status OfxEffectInstance::render(SequenceTime time,RenderScale scale,
                                          const RectI& roi,int view,boost::shared_ptr<Natron::Image>/* output*/){
    OfxRectI ofxRoI;
    ofxRoI.x1 = roi.left();
    ofxRoI.x2 = roi.right();
    ofxRoI.y1 = roi.bottom();
    ofxRoI.y2 = roi.top();
    int viewsCount = getApp()->getCurrentProjectViewsCount();
    OfxStatus st = effect_->renderAction((OfxTime)time,kOfxImageFieldNone,ofxRoI,scale,view,viewsCount);
    if(st != kOfxStatOK){
        return StatFailed;
    }else{
        return StatOK;
    }
}

EffectInstance::RenderSafety OfxEffectInstance::renderThreadSafety() const{
    if(!effect_->getHostFrameThreading()){
        return EffectInstance::INSTANCE_SAFE;
    }
    
    const std::string& safety = effect_->getRenderThreadSafety();
    if (safety == kOfxImageEffectRenderUnsafe) {
        return EffectInstance::UNSAFE;
    }else if(safety == kOfxImageEffectRenderInstanceSafe){
        return EffectInstance::INSTANCE_SAFE;
    }else{
        return EffectInstance::FULLY_SAFE;
    }
    
}

bool OfxEffectInstance::makePreviewByDefault() const { return isGenerator() || isGeneratorAndFilter(); }



const std::string& OfxEffectInstance::getShortLabel() const {
    return effectInstance()->getShortLabel();
}


void OfxEffectInstance::swapBuffersOfAttachedViewer(){
    std::list<ViewerInstance*> viewers;
    getNode()->hasViewersConnected(&viewers);
    for(std::list<ViewerInstance*>::iterator it = viewers.begin();it!=viewers.end();++it){
        (*it)->swapBuffers();
    }
}

void OfxEffectInstance::redrawInteractOnAttachedViewer(){
    std::list<ViewerInstance*> viewers;
    getNode()->hasViewersConnected(&viewers);
    for(std::list<ViewerInstance*>::iterator it = viewers.begin();it!=viewers.end();++it){
        (*it)->redrawViewer();
    }

}

void OfxEffectInstance::pixelScaleOfAttachedViewer(double &x,double &y){
    std::list<ViewerInstance*> viewers;
    getNode()->hasViewersConnected(&viewers);
    for(std::list<ViewerInstance*>::iterator it = viewers.begin();it!=viewers.end();++it){
        (*it)->pixelScale(x, y);
    }
}

void OfxEffectInstance::viewportSizeOfAttachedViewer(double &w,double &h){
    std::list<ViewerInstance*> viewers;
    getNode()->hasViewersConnected(&viewers);
    for(std::list<ViewerInstance*>::iterator it = viewers.begin();it!=viewers.end();++it){
        (*it)->viewportSize(w, h);
    }
}
void OfxEffectInstance::backgroundColorOfAttachedViewer(double &r,double &g,double &b){
    std::list<ViewerInstance*> viewers;
    getNode()->hasViewersConnected(&viewers);
    for(std::list<ViewerInstance*>::iterator it = viewers.begin();it!=viewers.end();++it){
        (*it)->backgroundColor(r, g, b);
    }
    
}



void OfxEffectInstance::drawOverlay(){
    if(_overlayInteract){
        OfxPointD rs;
        rs.x = rs.y = 1.;
        _overlayInteract->drawAction(1.0, rs);
    }
}


bool OfxEffectInstance::onOverlayPenDown(const QPointF& viewportPos,const QPointF& pos){
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
        if (stat == kOfxStatOK) {
            _penDown = true;
            return true;
        }
    }
    return false;
}

bool OfxEffectInstance::onOverlayPenMotion(const QPointF& viewportPos,const QPointF& pos){
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
        if (stat == kOfxStatOK) {
            return true;
        }
    }
    return false;
}


bool OfxEffectInstance::onOverlayPenUp(const QPointF& viewportPos,const QPointF& pos){
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
        if (stat == kOfxStatOK) {
            _penDown = false;
            return true;
        }
    }
    return false;
}

void OfxEffectInstance::onOverlayKeyDown(QKeyEvent* e){
    if(_overlayInteract){
        OfxPointD rs;
        rs.x = rs.y = 1.;
        OfxStatus stat = _overlayInteract->keyDownAction(1., rs, e->nativeVirtualKey(), e->text().toLatin1().data());
        if (stat == kOfxStatOK) {
            //requestRender();
        }
    }
    
}

void OfxEffectInstance::onOverlayKeyUp(QKeyEvent* e){
    if(_overlayInteract){
        OfxPointD rs;
        rs.x = rs.y = 1.;
        OfxStatus stat = _overlayInteract->keyUpAction(1., rs, e->nativeVirtualKey(), e->text().toLatin1().data());
        assert(stat == kOfxStatOK || stat == kOfxStatReplyDefault);
        if (stat == kOfxStatOK) {
            //requestRender();
        };
    }
}

void OfxEffectInstance::onOverlayKeyRepeat(QKeyEvent* e){
    if(_overlayInteract){
        OfxPointD rs;
        rs.x = rs.y = 1.;
        OfxStatus stat = _overlayInteract->keyRepeatAction(1., rs, e->nativeVirtualKey(), e->text().toLatin1().data());
        if (stat == kOfxStatOK) {
            //requestRender();
        }
    }
}

void OfxEffectInstance::onOverlayFocusGained(){
    if(_overlayInteract){
        OfxPointD rs;
        rs.x = rs.y = 1.;
        OfxStatus stat = _overlayInteract->gainFocusAction(1., rs);
        assert(stat == kOfxStatOK || stat == kOfxStatReplyDefault);
        if (stat == kOfxStatOK) {
            //requestRender();
        }
    }
}

void OfxEffectInstance::onOverlayFocusLost(){
    if(_overlayInteract){
        OfxPointD rs;
        rs.x = rs.y = 1.;
        OfxStatus stat = _overlayInteract->loseFocusAction(1., rs);
        assert(stat == kOfxStatOK || stat == kOfxStatReplyDefault);
        if (stat == kOfxStatOK) {
            //requestRender();
        }
    }
}


void OfxEffectInstance::onKnobValueChanged(Knob* k,Knob::ValueChangedReason reason){
    if(reason == Knob::USER_EDITED){
        OfxPointD renderScale;
        effect_->getRenderScaleRecursive(renderScale.x, renderScale.y);
        OfxTime time = effect_->getFrameRecursive();
        OfxStatus stat = effectInstance()->paramInstanceChangedAction(k->getName(), kOfxChangeUserEdited,time,renderScale);
        // note: DON'T remove the following assert()s, unless you replace them with proper error feedback.
        assert(stat == kOfxStatOK || stat == kOfxStatReplyDefault);
    }
}

void OfxEffectInstance::beginKnobsValuesChanged(Knob::ValueChangedReason reason) {
    if(reason == Knob::USER_EDITED){
        OfxStatus stat = effectInstance()->beginInstanceChangedAction(kOfxChangeUserEdited);
        assert(stat == kOfxStatOK || stat == kOfxStatReplyDefault);
    }
}

void OfxEffectInstance::endKnobsValuesChanged(Knob::ValueChangedReason reason){
    if(reason == Knob::USER_EDITED){
        OfxStatus stat = effectInstance()->endInstanceChangedAction(kOfxChangeUserEdited);
        assert(stat == kOfxStatOK || stat == kOfxStatReplyDefault);
    }
}

std::string OfxEffectInstance::getOutputFileName() const{
    const std::vector<boost::shared_ptr<Knob> >& knobs = getKnobs();
    for (U32 i = 0; i < knobs.size(); ++i) {
        if (knobs[i]->typeName() == "OutputFile") {
            boost::shared_ptr<OutputFile_Knob> knob = boost::dynamic_pointer_cast<OutputFile_Knob>(knobs[i]);
            return knob->getValue<QString>().toStdString();
        }
    }
    return "";
}
