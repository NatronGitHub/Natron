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
#include <stdexcept>
#include <QByteArray>
#include <QPointF>

#include "Global/Macros.h"

#include <ofxhPluginCache.h>
#include <ofxhPluginAPICache.h>
#include <ofxhImageEffect.h>
#include <ofxhImageEffectAPI.h>
#include <ofxhHost.h>

#include "Engine/AppManager.h"
#include "Engine/OfxParamInstance.h"
#include "Engine/Row.h"
#include "Engine/OfxClipInstance.h"
#include "Engine/OfxImageEffectInstance.h"
#include "Engine/OfxOverlayInteract.h"
#include "Engine/ViewerInstance.h"
#include "Engine/VideoEngine.h"
#include "Engine/TimeLine.h"
#include "Engine/Project.h"
#include "Engine/KnobFile.h"
#include "Engine/KnobTypes.h"
#include "Engine/AppInstance.h"

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
    : AbstractOfxEffectInstance(node)
    , effect_()
    , _isOutput(false)
    , _penDown(false)
    , _overlayInteract(0)
    , _initialized(false)
    , _renderButton()
{
    if(node && !node->getLiveInstance()){
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
    
    if (context == kOfxImageEffectContextWriter) {
        setAsOutputNode();
    }
    
    OFX::Host::PluginHandle* ph = plugin->getPluginHandle();
    assert(ph->getOfxPlugin());
    assert(ph->getOfxPlugin()->mainEntry);
    (void)ph;
    OFX::Host::ImageEffect::Descriptor* desc = NULL;
    desc = plugin->getContext(context);
    if (!desc) {
        throw std::runtime_error(std::string("Failed to get description for OFX plugin in context ") + context);
    }
    

    try {
        effect_ = new Natron::OfxImageEffectInstance(plugin,*desc,context,false);
        assert(effect_);
        effect_->setOfxEffectInstancePointer(this);
        notifyProjectBeginKnobsValuesChanged(Natron::OTHER_REASON);
        OfxStatus stat = effect_->populate();
        
        initializeContextDependentParams();
        
        effect_->addParamsToTheirParents();
        notifyProjectEndKnobsValuesChanged();

        if (stat != kOfxStatOK) {
            throw std::runtime_error("Error while populating the Ofx image effect");
        }
        assert(effect_->getPlugin());
        assert(effect_->getPlugin()->getPluginHandle());
        assert(effect_->getPlugin()->getPluginHandle()->getOfxPlugin());
        assert(effect_->getPlugin()->getPluginHandle()->getOfxPlugin()->mainEntry);
        stat = effect_->createInstanceAction();
        if(stat != kOfxStatOK && stat != kOfxStatReplyDefault){
            throw std::runtime_error("Could not create effect instance for plugin");
        }
        if (!effect_->getClipPreferences()) {
           qDebug() << "The plugin failed in the getClipPreferencesAction.";
        }
        
        // check that the plugin supports kOfxImageComponentRGBA for all the clips
        const std::vector<OFX::Host::ImageEffect::ClipDescriptor*>& clips = effectInstance()->getDescriptor().getClipsByOrder();
        for (U32 i = 0; i < clips.size(); ++i) {
            if (clips[i]->getProps().findStringPropValueIndex(kOfxImageEffectPropSupportedComponents, kOfxImageComponentRGBA) == -1
                && !clips[i]->isOptional()) {
                throw std::runtime_error(std::string("RGBA components not supported by OFX plugin in context ") + context);
            }
        }
    

    } catch (const std::exception& e) {
        qDebug() << "Error: Caught exception while creating OfxImageEffectInstance" << ": " << e.what();
        throw;
    } catch (...) {
        qDebug() << "Error: Caught exception while creating OfxImageEffectInstance";
        throw;
    }
    _initialized = true;
}

OfxEffectInstance::~OfxEffectInstance(){
    
    if(_overlayInteract){
        delete _overlayInteract;
    }
    
    delete effect_;

}



void OfxEffectInstance::initializeContextDependentParams() {
    
    if (isWriter()) {
        _renderButton = Natron::createKnob<Button_Knob>(this, "Render");
        _renderButton->setHintToolTip("Starts rendering the specified frame range.");
        _renderButton->setAsRenderButton();
    }
    
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
            _overlayInteract = new OfxOverlayInteract(*effect_,8,true);
            _overlayInteract->createInstanceAction();
            getApp()->redrawAllViewers();
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
    std::set<std::string>::const_iterator foundReader = contexts.find(kOfxImageEffectContextReader);
    if(foundGenerator != contexts.end() || foundReader!= contexts.end())
        return true;
    return false;
}

bool OfxEffectInstance::isReader() const {
    assert(effectInstance());
    const std::set<std::string>& contexts = effectInstance()->getPlugin()->getContexts();
    std::set<std::string>::const_iterator foundReader = contexts.find(kOfxImageEffectContextReader);
    if (foundReader != contexts.end()) {
        return true;
    }
    return false;
}

bool OfxEffectInstance::isWriter() const {
    assert(effectInstance());
    const std::set<std::string>& contexts = effectInstance()->getPlugin()->getContexts();
    std::set<std::string>::const_iterator foundWriter = contexts.find(kOfxImageEffectContextWriter);
    if (foundWriter != contexts.end()) {
        return true;
    }
    return false;
}

bool OfxEffectInstance::isGeneratorAndFilter() const {
    const std::set<std::string>& contexts = effectInstance()->getPlugin()->getContexts();
    std::set<std::string>::const_iterator foundGenerator = contexts.find(kOfxImageEffectContextGenerator);
    std::set<std::string>::const_iterator foundGeneral = contexts.find(kOfxImageEffectContextGeneral);
    return foundGenerator!=contexts.end() && foundGeneral!=contexts.end();
}


/*group is a string as such:
 Toto/Superplugins/blabla
 This functions extracts the all parts of such a grouping, e.g in this case
 it would return [Toto,Superplugins,blabla].*/
QStringList ofxExtractAllPartsOfGrouping(const QString& pluginLabel,const QString& str) {
    QStringList out;
    if (str.startsWith("Sapphire ") || str.startsWith(" Sapphire ")) {
        out.push_back("Sapphire");
    }else if (str.startsWith("Monsters ") || str.startsWith(" Monsters ")) {
        out.push_back("Monsters");
    }
    int pos = 0;
    while(pos < str.size()){
        QString newPart;
        while(pos < str.size() && str.at(pos) != QChar('/') && str.at(pos) != QChar('\\')){
            newPart.append(str.at(pos));
            ++pos;
        }
        ++pos;
        if(newPart != pluginLabel){
            out.push_back(newPart);
        }
    }
    return out;
}

QStringList AbstractOfxEffectInstance::getPluginGrouping(const std::string& pluginLabel,const std::string& grouping){
    return  ofxExtractAllPartsOfGrouping(pluginLabel.c_str(),grouping.c_str());
}
std::string AbstractOfxEffectInstance::getPluginLabel(const std::string& shortLabel,
                                      const std::string& label,
                                      const std::string& longLabel) {
    std::string labelToUse = label;
    if(labelToUse.empty()){
        labelToUse = shortLabel;
    }
    if(labelToUse.empty()){
        labelToUse = longLabel;
    }
    return labelToUse;
}

std::string AbstractOfxEffectInstance::generateImageEffectClassName(const std::string& shortLabel,
                                                            const std::string& label,
                                                            const std::string& longLabel,
                                                            const std::string& grouping){
    std::string labelToUse = getPluginLabel(shortLabel,label,longLabel);
    QStringList groups = getPluginGrouping(labelToUse,grouping);

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

std::string OfxEffectInstance::pluginID() const {
    assert(effect_);
    return generateImageEffectClassName(effect_->getDescriptor().getShortLabel(),
                                        effect_->getDescriptor().getLabel(),
                                        effect_->getDescriptor().getLongLabel(),
                                        effect_->getDescriptor().getPluginGrouping());
}

std::string OfxEffectInstance::pluginLabel() const {
    assert(effect_);
    return getPluginLabel( effect_->getDescriptor().getShortLabel(),effect_->getDescriptor().getLabel(),effect_->getDescriptor().getLongLabel());
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

OfxClipInstance* OfxEffectInstance::getClipCorrespondingToInput(int inputNo) const {
    OfxEffectInstance::MappedInputV clips = inputClipsCopyWithoutOutput();
    assert(inputNo < (int)clips.size());
    OFX::Host::ImageEffect::ClipInstance* clip = effect_->getClip(clips[inputNo]->getName());
    assert(clip);
    return dynamic_cast<OfxClipInstance*>(clip);
}

int OfxEffectInstance::maximumInputs() const {
    const std::string& context = effectInstance()->getContext();
    if(context == kOfxImageEffectContextReader ||
       context == kOfxImageEffectContextGenerator){
        return 0;
    } else {
        assert(effectInstance());
        int totalClips = effectInstance()->getDescriptor().getClips().size();
        return totalClips > 0  ?  totalClips-1 : 0;
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


void OfxEffectInstance::onInputChanged(int inputNo) {
    OfxClipInstance* clip = getClipCorrespondingToInput(inputNo);
    assert(clip);
    double time = effect_->getFrameRecursive();
    RenderScale s;
    s.x = s.y = 1.;
    effect_->clipInstanceChangedAction(clip->getName(), kOfxChangeUserEdited, time, s);
    
}

void OfxEffectInstance::onMultipleInputsChanged() {
    effect_->runGetClipPrefsConditionally();
}

Natron::Status OfxEffectInstance::getRegionOfDefinition(SequenceTime time,RectI* rod){
    if(!_initialized){
        return Natron::StatFailed;
    }

    assert(effect_);
    
//    
//    if (isWriter()) {
//        rod->set(getRenderFormat());
//        return StatReplyDefault;
//    }
    
    OfxPointD rS;
    rS.x = rS.y = 1.0;
    OfxRectD ofxRod;
    OfxStatus stat = effect_->getRegionOfDefinitionAction(time, rS, ofxRod);
    if((stat!= kOfxStatOK && stat != kOfxStatReplyDefault) ||
            (ofxRod.x1 ==  0. && ofxRod.x2 == 0. && ofxRod.y1 == 0. && ofxRod.y2 == 0.))
        return StatFailed;
    ifInfiniteclipRectToProjectDefault(&ofxRod);
    ofxRectDToRectI(ofxRod,rod);
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
    if(!_initialized){
        return ret;
    }
    OfxStatus stat = effect_->getRegionOfInterestAction((OfxTime)time, scale, rectToOfxRect2D(renderWindow), inputRois);
    if(stat != kOfxStatOK && stat != kOfxStatReplyDefault)
        return ret;
    for(std::map<OFX::Host::ImageEffect::ClipInstance*,OfxRectD>::iterator it = inputRois.begin();it!= inputRois.end();++it){
        EffectInstance* inputNode = dynamic_cast<OfxClipInstance*>(it->first)->getAssociatedNode();
        if(inputNode && inputNode != this){
            RectI inputRoi;
            ofxRectDToRectI(it->second, &inputRoi);
            ret.insert(std::make_pair(inputNode,inputRoi));
        }
    }
    return ret;
}


void OfxEffectInstance::getFrameRange(SequenceTime *first,SequenceTime *last){
    if(!_initialized){
        return;
    }
    OfxRangeD range;
    // getTimeDomain should only be called on the 'general', 'reader' or 'generator' contexts.
    //  see http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#kOfxImageEffectActionGetTimeDomain"
    OfxStatus st = kOfxStatReplyDefault;
    if (effect_->getContext() == kOfxImageEffectContextGeneral ||
        effect_->getContext() == kOfxImageEffectContextReader ||
        effect_->getContext() == kOfxImageEffectContextGenerator) {
        st = effect_->getTimeDomainAction(range);
    }
    if (st == kOfxStatOK) {
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
            bool firstValidClip = true;
            *first = INT_MIN;
            *last = INT_MAX;
            for (int i = 0; i < nthClip ; ++i) {
                OFX::Host::ImageEffect::ClipInstance* clip = effect_->getNthClip(i);
                assert(clip);
                if (!clip->isOutput() && !clip->isOptional()) {
                    double f,l;
                    clip->getFrameRange(f, l);
                    if (!firstValidClip) {
                        if (f < *first) {
                            *first = f;
                        }
                        if (l > *last) {
                            *last = l;
                        }
                    } else {
                        firstValidClip = false;
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
    
    return StatOK;
}

bool OfxEffectInstance::isIdentity(SequenceTime time,RenderScale scale,const RectI& roi,
                                int /*view*/,SequenceTime* inputTime,int* inputNb) {
    OfxRectI ofxRoI;
    ofxRoI.x1 = roi.left();
    ofxRoI.x2 = roi.right();
    ofxRoI.y1 = roi.bottom();
    ofxRoI.y2 = roi.top();
    const std::string field = kOfxImageFieldNone; // TODO: support interlaced data
    std::string inputclip;
    OfxTime inputTimeOfx = time;
    OfxStatus stat = effect_->isIdentityAction(inputTimeOfx,field,ofxRoI,scale,inputclip);
    if(stat == kOfxStatOK){
        OFX::Host::ImageEffect::ClipInstance* clip = effect_->getClip(inputclip);
        if (!clip) {
            // this is a plugin-side error, don't crash
            qDebug() << "Error in OfxEffectInstance::render(): kOfxImageEffectActionIsIdentity returned an unknown clip: " << inputclip.c_str();
            return StatFailed;
        }
        OfxClipInstance* natronClip = dynamic_cast<OfxClipInstance*>(clip);
        assert(natronClip);
        *inputTime = inputTimeOfx;
        *inputNb = natronClip->getInputNb();
        return true;
    }else{
        return false;
    }
}

Natron::Status OfxEffectInstance::render(SequenceTime time,RenderScale scale,
                                         const RectI& roi,int view,boost::shared_ptr<Natron::Image> /*output*/){
    if(!_initialized){
        return Natron::StatFailed;
    }
    OfxRectI ofxRoI;
    ofxRoI.x1 = roi.left();
    ofxRoI.x2 = roi.right();
    ofxRoI.y1 = roi.bottom();
    ofxRoI.y2 = roi.top();
    int viewsCount = getApp()->getProject()->getProjectViewsCount();
    OfxStatus stat;
    const std::string field = kOfxImageFieldNone; // TODO: support interlaced data
    stat = effect_->renderAction((OfxTime)time, field, ofxRoI, scale, view, viewsCount);
    if (stat != kOfxStatOK) {
        return StatFailed;
    } else {
        return StatOK;
    }
}

EffectInstance::RenderSafety OfxEffectInstance::renderThreadSafety() const{
    const std::string& safety = effect_->getRenderThreadSafety();
    // TODO: cache this result
    if (safety == kOfxImageEffectRenderUnsafe) {
        return EffectInstance::UNSAFE;
    }else if(safety == kOfxImageEffectRenderInstanceSafe) {
        return EffectInstance::INSTANCE_SAFE;
    }else if(safety == kOfxImageEffectRenderFullySafe) {
        if (effect_->getHostFrameThreading()) {
            return EffectInstance::FULLY_SAFE_FRAME;
        } else {
            return EffectInstance::FULLY_SAFE;
        }
    }
    qDebug() << "Unknown thread safety level: " << safety.c_str();
    return EffectInstance::UNSAFE;
}

bool OfxEffectInstance::makePreviewByDefault() const { return isGenerator() || isGeneratorAndFilter(); }



const std::string& OfxEffectInstance::getShortLabel() const {
    return effectInstance()->getShortLabel();
}

void OfxEffectInstance::initializeOverlayInteract() {
    tryInitializeOverlayInteracts();
}


void OfxEffectInstance::drawOverlay(){
    if(!_initialized){
        return;
    }
    if(_overlayInteract){
        OfxPointD rs;
        rs.x = rs.y = 1.;
        OfxTime time = effect_->getFrameRecursive();
        _overlayInteract->drawAction(time, rs);
    }
}

void OfxEffectInstance::setCurrentViewportForOverlays(OverlaySupport* viewport) {
    if (_overlayInteract) {
        _overlayInteract->setCallingViewport(viewport);
    }
}

bool OfxEffectInstance::onOverlayPenDown(const QPointF& viewportPos,const QPointF& pos){
    if(!_initialized){
        return false;
    }
    if(_overlayInteract){
        OfxPointD rs;
        rs.x = rs.y = 1.;
        OfxPointD penPos;
        penPos.x = pos.x();
        penPos.y = pos.y();
        OfxPointI penPosViewport;
        penPosViewport.x = viewportPos.x();
        penPosViewport.y = viewportPos.y();
        
        OfxTime time = effect_->getFrameRecursive();
        OfxStatus stat = _overlayInteract->penDownAction(time, rs, penPos, penPosViewport, 1.);
        if (stat == kOfxStatOK) {
            _penDown = true;
            return true;
        }
    }
    return false;
}

bool OfxEffectInstance::onOverlayPenMotion(const QPointF& viewportPos,const QPointF& pos){
    if(!_initialized){
        return false;
    }
    if(_overlayInteract){
        OfxPointD rs;
        rs.x = rs.y = 1.;
        OfxPointD penPos;
        penPos.x = pos.x();
        penPos.y = pos.y();
        OfxPointI penPosViewport;
        penPosViewport.x = viewportPos.x();
        penPosViewport.y = viewportPos.y();
        OfxTime time = effect_->getFrameRecursive();
        OfxStatus stat = _overlayInteract->penMotionAction(time, rs, penPos, penPosViewport, 1.);
        if (stat == kOfxStatOK) {
            return true;
        }
    }
    return false;
}


bool OfxEffectInstance::onOverlayPenUp(const QPointF& viewportPos,const QPointF& pos){
    if(!_initialized){
        return false;
    }
    if(_overlayInteract){
        OfxPointD rs;
        rs.x = rs.y = 1.;
        OfxPointD penPos;
        penPos.x = pos.x();
        penPos.y = pos.y();
        OfxPointI penPosViewport;
        penPosViewport.x = viewportPos.x();
        penPosViewport.y = viewportPos.y();
        OfxTime time = effect_->getFrameRecursive();
        OfxStatus stat = _overlayInteract->penUpAction(time, rs, penPos, penPosViewport, 1.);
        if (stat == kOfxStatOK) {
            _penDown = false;
            return true;
        }
    }
    return false;
}

bool OfxEffectInstance::onOverlayKeyDown(Natron::Key key,Natron::KeyboardModifiers /*modifiers*/){
    if(!_initialized){
        return false;;
    }
    if(_overlayInteract){
        OfxPointD rs;
        rs.x = rs.y = 1.;
        OfxTime time = effect_->getFrameRecursive();
        QByteArray keyStr;
        OfxStatus stat = _overlayInteract->keyDownAction(time, rs, (int)key, keyStr.data());
        if (stat == kOfxStatOK) {
            return true;
        }
    }
    return false;
}

bool OfxEffectInstance::onOverlayKeyUp(Natron::Key key,Natron::KeyboardModifiers /* modifiers*/){
    if(!_initialized){
        return false;
    }
    if(_overlayInteract){
        OfxPointD rs;
        rs.x = rs.y = 1.;
        OfxTime time = effect_->getFrameRecursive();
        QByteArray keyStr;
        OfxStatus stat = _overlayInteract->keyUpAction(time, rs, (int)key, keyStr.data());
        assert(stat == kOfxStatOK || stat == kOfxStatReplyDefault);
        if (stat == kOfxStatOK) {
            return true;
        };
    }
    return false;
}

bool OfxEffectInstance::onOverlayKeyRepeat(Natron::Key key,Natron::KeyboardModifiers /*modifiers*/){
    if(!_initialized){
        return false;
    }
    if(_overlayInteract){
        OfxPointD rs;
        rs.x = rs.y = 1.;
        OfxTime time = effect_->getFrameRecursive();
        QByteArray keyStr;
        OfxStatus stat = _overlayInteract->keyRepeatAction(time, rs, (int)key, keyStr.data());
        if (stat == kOfxStatOK) {
            return true;
        }
    }
    return false;
}

bool OfxEffectInstance::onOverlayFocusGained(){
    if(!_initialized){
        return false;
    }
    if(_overlayInteract){
        OfxPointD rs;
        rs.x = rs.y = 1.;
        OfxTime time = effect_->getFrameRecursive();
        OfxStatus stat = _overlayInteract->gainFocusAction(time, rs);
        assert(stat == kOfxStatOK || stat == kOfxStatReplyDefault);
        if (stat == kOfxStatOK) {
            return true;
        }
    }
    return false;
}

bool OfxEffectInstance::onOverlayFocusLost(){
    if(!_initialized){
        return false;
    }
    if(_overlayInteract){
        OfxPointD rs;
        rs.x = rs.y = 1.;
        OfxTime time = effect_->getFrameRecursive();
        OfxStatus stat = _overlayInteract->loseFocusAction(time, rs);
        assert(stat == kOfxStatOK || stat == kOfxStatReplyDefault);
        if (stat == kOfxStatOK) {
            return true;
        }
    }
    return false;
}


void OfxEffectInstance::onKnobValueChanged(Knob* k,Natron::ValueChangedReason reason){
    if(!_initialized){
        return;
    }
    
    if (_renderButton && k == _renderButton.get()) {
        
        ///don't do anything since it is handled upstream
        return;
    }
    
    OfxPointD renderScale;
    effect_->getRenderScaleRecursive(renderScale.x, renderScale.y);
    OfxTime time = effect_->getFrameRecursive();
    OfxStatus stat = kOfxStatOK;
    switch (reason) {
        case Natron::USER_EDITED:
            stat = effectInstance()->paramInstanceChangedAction(k->getName(), kOfxChangeUserEdited,time,renderScale);
            break;
        case Natron::TIME_CHANGED:
            stat = effectInstance()->paramInstanceChangedAction(k->getName(), kOfxChangeTime,time,renderScale);
            break;
        case Natron::PLUGIN_EDITED:
            stat = effectInstance()->paramInstanceChangedAction(k->getName(), kOfxChangePluginEdited,time,renderScale);
            break;
        case Natron::OTHER_REASON:
        default:
            break;
    }

    if (stat != kOfxStatOK && stat != kOfxStatReplyDefault) {
        Natron::errorDialog(getName(), "An error occured while changing a parameter.");
        return;
    }
    
    if (effect_->isClipPreferencesSlaveParam(k->getName())) {
        effect_->runGetClipPrefsConditionally();
    }
    if(_overlayInteract){
        std::vector<std::string> params;
        _overlayInteract->getSlaveToParam(params);
        for (U32 i = 0; i < params.size(); ++i) {
            if(params[i] == k->getDescription()){
                stat = _overlayInteract->redraw();
                assert(stat == kOfxStatOK || stat == kOfxStatReplyDefault);
            }
        }
    }
   

}

void OfxEffectInstance::beginKnobsValuesChanged(Natron::ValueChangedReason reason) {
    if(!_initialized){
        return;
    }
    OfxStatus stat = kOfxStatOK;
    switch (reason) {
        case Natron::USER_EDITED:
            stat = effectInstance()->beginInstanceChangedAction(kOfxChangeUserEdited);
            break;
        case Natron::TIME_CHANGED:
            stat = effectInstance()->beginInstanceChangedAction(kOfxChangeTime);
            break;
        case Natron::PLUGIN_EDITED:
            stat = effectInstance()->beginInstanceChangedAction(kOfxChangePluginEdited);
            break;
        case Natron::OTHER_REASON:
        default:
            break;
    }
    assert(stat == kOfxStatOK || stat == kOfxStatReplyDefault);

}

void OfxEffectInstance::endKnobsValuesChanged(Natron::ValueChangedReason reason){
    if(!_initialized){
        return;
    }
    OfxStatus stat = kOfxStatOK;
    switch (reason) {
        case Natron::USER_EDITED:
            stat = effectInstance()->endInstanceChangedAction(kOfxChangeUserEdited);
            break;
        case Natron::TIME_CHANGED:
            stat = effectInstance()->endInstanceChangedAction(kOfxChangeTime);
            break;
        case Natron::PLUGIN_EDITED:
            stat = effectInstance()->endInstanceChangedAction(kOfxChangePluginEdited);
            break;
        case Natron::OTHER_REASON:
        default:
            break;
    }
    assert(stat == kOfxStatOK || stat == kOfxStatReplyDefault);
    //effectInstance()->syncPrivateDataAction();

}


void OfxEffectInstance::purgeCaches(){
    // The kOfxActionPurgeCaches is an action that may be passed to a plug-in instance from time to time in low memory situations. Instances recieving this action should destroy any data structures they may have and release the associated memory, they can later reconstruct this from the effect's parameter set and associated information. http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#kOfxActionPurgeCaches
    OfxStatus stat =  effect_->purgeCachesAction();
    assert(stat == kOfxStatOK || stat == kOfxStatReplyDefault);
    // The kOfxActionSyncPrivateData action is called when a plugin should synchronise any private data structures to its parameter set. This generally occurs when an effect is about to be saved or copied, but it could occur in other situations as well. http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#kOfxActionSyncPrivateData
    stat =  effect_->syncPrivateDataAction();
    assert(stat == kOfxStatOK || stat == kOfxStatReplyDefault);
}

int OfxEffectInstance::majorVersion() const {
    return effectInstance()->getPlugin()->getVersionMajor();
}

int OfxEffectInstance::minorVersion() const {
    return effectInstance()->getPlugin()->getVersionMinor();
}

bool OfxEffectInstance::supportsTiles() const {
    return effectInstance()->supportsTiles() && effectInstance()->getClip(kOfxImageEffectOutputClipName)->supportsTiles();
}

void OfxEffectInstance::beginEditKnobs() {
    effectInstance()->beginInstanceEditAction();
}