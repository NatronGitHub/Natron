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

#include <QtCore/QDebug>
#include <QByteArray>
#include <QReadWriteLock>
#include <QPointF>

#include "Global/Macros.h"

#include <ofxhPluginCache.h>
#include <ofxhPluginAPICache.h>
#include <ofxhImageEffect.h>
#include <ofxhImageEffectAPI.h>
#include <ofxhHost.h>

#include <tuttle/ofxReadWrite.h>


#include "Engine/AppManager.h"
#include "Engine/OfxParamInstance.h"
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
#include "Engine/NodeSerialization.h"
#include "Engine/Node.h"

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

OfxEffectInstance::OfxEffectInstance(boost::shared_ptr<Natron::Node> node)
    : AbstractOfxEffectInstance(node)
    , effect_()
    , _isOutput(false)
    , _penDown(false)
    , _overlayInteract(0)
    , _initialized(false)
    , _renderButton()
    , _renderSafety(EffectInstance::UNSAFE)
    , _wasRenderSafetySet(false)
    , _renderSafetyLock(new QReadWriteLock)
{
    QObject::connect(this, SIGNAL(syncPrivateDataRequested()), this, SLOT(onSyncPrivateDataRequested()));
}

void OfxEffectInstance::createOfxImageEffectInstance(OFX::Host::ImageEffect::ImageEffectPlugin* plugin,
                                                     const std::string& context,const NodeSerialization* serialization){
    /*Replicate of the code in OFX::Host::ImageEffect::ImageEffectPlugin::createInstance.
     We need to pass more parameters to the constructor . That means we cannot
     create it in the virtual function newInstance. Thus we create it before
     instanciating the OfxImageEffect. The problem is that calling OFX::Host::ImageEffect::ImageEffectPlugin::createInstance
     creates the OfxImageEffect and calls populate(). populate() will actually create all OfxClipInstance and OfxParamInstance.
     All these subclasses need a valid pointer to an this. Hence we need to set the pointer to this in
     OfxImageEffect BEFORE calling populate().
     */
    
    ///Only called from the main thread.
    assert(QThread::currentThread() == qApp->thread());
    
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
        effect_->setOfxEffectInstancePointer(dynamic_cast<OfxEffectInstance*>(this));
        notifyProjectBeginKnobsValuesChanged(Natron::PROJECT_LOADING);
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
        
        ///before calling the createInstanceAction
        
        if (serialization && !serialization->isNull()) {
            getNode()->loadKnobs(*serialization);
        }
        
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
    delete _renderSafetyLock;
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
    OfxPluginEntryPoint *overlayEntryPoint = effect_->getOverlayInteractMainEntry();
    if(overlayEntryPoint){
        _overlayInteract = new OfxOverlayInteract(*effect_,8,true);
        _overlayInteract->createInstanceAction();
        getApp()->redrawAllViewers();
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

/// the smallest RectI enclosing the given RectD
static void ofxRectDToEnclosingRectI(const OfxRectD& ofxrect,RectI* box)
{
    // safely convert to OfxRectI, avoiding overflows
    int xmin = (int)std::max((double)kOfxFlagInfiniteMin, std::floor(ofxrect.x1));
    int ymin = (int)std::max((double)kOfxFlagInfiniteMin, std::floor(ofxrect.y1));
    int xmax = (int)std::min((double)kOfxFlagInfiniteMax, std::ceil(ofxrect.x2));
    int ymax = (int)std::min((double)kOfxFlagInfiniteMax, std::ceil(ofxrect.y2));
    box->set(xmin, ymin, xmax, ymax);
}

/// the largest RectI enclosed in the given RectD
static void ofxRectDToEnclosedRectI(const OfxRectD& ofxrect,RectI* box)
{
    // safely convert to OfxRectI, avoiding overflows
    int xmin = (int)std::max((double)kOfxFlagInfiniteMin, std::ceil(ofxrect.x1));
    int ymin = (int)std::max((double)kOfxFlagInfiniteMin, std::ceil(ofxrect.y1));
    int xmax = (int)std::min((double)kOfxFlagInfiniteMax, std::floor(ofxrect.x2));
    int ymax = (int)std::min((double)kOfxFlagInfiniteMax, std::floor(ofxrect.y2));
    box->set(xmin, ymin, xmax, ymax);
}

bool OfxEffectInstance::ifInfiniteclipRectToProjectDefault(OfxRectD* rod) const{
    /*If the rod is infinite clip it to the project's default*/

    Format projectDefault;
    getRenderFormat(&projectDefault);
    /// FIXME: before removing the assert() (I know you are tempted) please explain (here: document!) if the format rectangle can be empty and in what situation(s)
    assert(!projectDefault.isNull());
    // BE CAREFUL:
    // std::numeric_limits<int>::infinity() does not exist (check std::numeric_limits<int>::has_infinity)
    bool isProjectFormat = false;
    if (rod->x1 == kOfxFlagInfiniteMin || rod->x1 == -std::numeric_limits<double>::infinity()) {
        rod->x1 = projectDefault.left();
        isProjectFormat = true;
    }
    if (rod->y1 == kOfxFlagInfiniteMin || rod->y1 == -std::numeric_limits<double>::infinity()) {
        rod->y1 = projectDefault.bottom();
        isProjectFormat = true;
    }
    if (rod->x2== kOfxFlagInfiniteMax || rod->x2 == std::numeric_limits<double>::infinity()) {
        rod->x2 = projectDefault.right();
        isProjectFormat = true;
    }
    if (rod->y2 == kOfxFlagInfiniteMax || rod->y2  == std::numeric_limits<double>::infinity()) {
        rod->y2 = projectDefault.top();
        isProjectFormat = true;
    }
    return isProjectFormat;
}


void OfxEffectInstance::onInputChanged(int inputNo) {
    OfxClipInstance* clip = getClipCorrespondingToInput(inputNo);
    assert(clip);
    double time = effect_->getFrameRecursive();
    RenderScale s;
    s.x = s.y = 1.;
    effect_->beginInstanceChangedAction(kOfxChangeUserEdited);
    effect_->clipInstanceChangedAction(clip->getName(), kOfxChangeUserEdited, time, s);
    effect_->endInstanceChangedAction(kOfxChangeUserEdited);
    
}

void OfxEffectInstance::onMultipleInputsChanged() {
    
    ///Recursive action, must not call assertActionIsNotRecursive()
    incrementRecursionLevel();
    effect_->runGetClipPrefsConditionally();
    decrementRecursionLevel();
}

std::vector<std::string> OfxEffectInstance::supportedFileFormats() const {

    int formatsCount = effect_->getDescriptor().getProps().getDimension(kTuttleOfxImageEffectPropSupportedExtensions);
    std::vector<std::string> formats(formatsCount);
    for (int k = 0; k < formatsCount; ++k) {
        formats[k] = effect_->getDescriptor().getProps().getStringProperty(kTuttleOfxImageEffectPropSupportedExtensions,k);
        std::transform(formats[k].begin(), formats[k].end(), formats[k].begin(), ::tolower);
    }
    return formats;
}

Natron::Status OfxEffectInstance::getRegionOfDefinition(SequenceTime time,const RenderScale& scale,RectI* rod,bool* isProjectFormat){
    if(!_initialized){
        return Natron::StatFailed;
    }

    assert(effect_);
    
    effectInstance()->setClipsRenderScale(scale);

    OfxRectD ofxRod;
    OfxStatus stat = effect_->getRegionOfDefinitionAction(time, (OfxPointD)scale, ofxRod);
    if (stat!= kOfxStatOK && stat != kOfxStatReplyDefault) {
        return StatFailed;
    }
    *isProjectFormat = ifInfiniteclipRectToProjectDefault(&ofxRod);
    ofxRectDToEnclosedRectI(ofxRod,rod);
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


EffectInstance::RoIMap OfxEffectInstance::getRegionOfInterest(SequenceTime time,RenderScale scale,const RectI& renderWindow,int view) {
    
    std::map<OFX::Host::ImageEffect::ClipInstance*,OfxRectD> inputRois;
    EffectInstance::RoIMap ret;
    if(!_initialized){
        return ret;
    }
    
    ///before calling getRoIaction set the relevant infos on the clips
    effectInstance()->setClipsRenderScale(scale);
    effectInstance()->setClipsView(view);
    
    OfxStatus stat = effect_->getRegionOfInterestAction((OfxTime)time, scale, rectToOfxRect2D(renderWindow), inputRois);
    
    if(stat != kOfxStatOK && stat != kOfxStatReplyDefault) {
        Natron::errorDialog(getNode()->getName_mt_safe(), "Failed to specify the region of interest from inputs.");
    }
    if (stat != kOfxStatReplyDefault) {
        for(std::map<OFX::Host::ImageEffect::ClipInstance*,OfxRectD>::iterator it = inputRois.begin();it!= inputRois.end();++it){
            EffectInstance* inputNode = dynamic_cast<OfxClipInstance*>(it->first)->getAssociatedNode();
            if (inputNode && inputNode != this) {
                RectI inputRoi;
                ofxRectDToEnclosingRectI(it->second, &inputRoi);
                ret.insert(std::make_pair(inputNode,inputRoi));
            }
        }
    } else if (stat == kOfxStatReplyDefault) {
        for (int i = 0; i < effectInstance()->getNClips(); ++i) {
            EffectInstance* inputNode = dynamic_cast<OfxClipInstance*>(effectInstance()->getNthClip(i))->getAssociatedNode();
            if (inputNode && inputNode != this) {
                ret.insert(std::make_pair(inputNode, renderWindow));
            }
        }
    }
    return ret;
}

Natron::EffectInstance::FramesNeededMap OfxEffectInstance::getFramesNeeded(SequenceTime time) {
    EffectInstance::FramesNeededMap ret;
    if(!_initialized){
        return ret;
    }
    OFX::Host::ImageEffect::RangeMap inputRanges;
    assert(effect_);
    
    OfxStatus stat = effect_->getFrameNeededAction((OfxTime)time, inputRanges);
    if(stat != kOfxStatOK && stat != kOfxStatReplyDefault) {
        Natron::errorDialog(getName(), "Failed to specify the frame ranges needed from inputs.");
    } else if (stat == kOfxStatOK) {
        for (OFX::Host::ImageEffect::RangeMap::iterator it = inputRanges.begin(); it!=inputRanges.end(); ++it) {
            int inputNb = dynamic_cast<OfxClipInstance*>(it->first)->getInputNb();
            if (inputNb != -1) {
                ret.insert(std::make_pair(inputNb,it->second));
            }
        }
    } else if(stat == kOfxStatReplyDefault) {
        return Natron::EffectInstance::getFramesNeeded(time);
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
    // Edit: Also add the 'writer' context as we need the getTimeDomain action to be able to find out the frame range to render.
    OfxStatus st = kOfxStatReplyDefault;
    if (effect_->getContext() == kOfxImageEffectContextGeneral ||
        effect_->getContext() == kOfxImageEffectContextReader ||
        effect_->getContext() == kOfxImageEffectContextWriter ||
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


void OfxEffectInstance::beginSequenceRender(SequenceTime first,SequenceTime last,
                                            SequenceTime step,bool interactive,RenderScale scale,
                                            bool isSequentialRender,bool isRenderResponseToUserInteraction,int view) {
    OfxStatus stat = effectInstance()->beginRenderAction(first, last, step, interactive, scale,isSequentialRender,isRenderResponseToUserInteraction,view);
    assert(stat == kOfxStatOK || stat == kOfxStatReplyDefault);
    
}

void OfxEffectInstance::endSequenceRender(SequenceTime first,SequenceTime last,
                                          SequenceTime step,bool interactive,RenderScale scale,
                                          bool isSequentialRender,bool isRenderResponseToUserInteraction,int view) {
    OfxStatus stat = effectInstance()->endRenderAction(first, last, step, interactive, scale,isSequentialRender,isRenderResponseToUserInteraction,view);
    assert(stat == kOfxStatOK || stat == kOfxStatReplyDefault);
    
}

Natron::Status OfxEffectInstance::render(SequenceTime time,RenderScale scale,
                                         const RectI& roi,int view,
                                         bool isSequentialRender,bool isRenderResponseToUserInteraction,
                                         boost::shared_ptr<Natron::Image> /*output*/){
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
    ///before calling render, set the render scale thread storage for each clip
    effectInstance()->setClipsRenderScale(scale);
    stat = effect_->renderAction((OfxTime)time, field, ofxRoI, scale,isSequentialRender,isRenderResponseToUserInteraction,view, viewsCount);
    if (stat != kOfxStatOK) {
        return StatFailed;
    } else {
        return StatOK;
    }
}

EffectInstance::RenderSafety OfxEffectInstance::renderThreadSafety() const {
    
    {
        QReadLocker readL(_renderSafetyLock);
        if (_wasRenderSafetySet) {
            return _renderSafety;
        }
    }
    {
        QWriteLocker writeL(_renderSafetyLock);
        const std::string& safety = effect_->getRenderThreadSafety();
        if (safety == kOfxImageEffectRenderUnsafe) {
            _renderSafety =  EffectInstance::UNSAFE;
        } else if (safety == kOfxImageEffectRenderInstanceSafe) {
            _renderSafety = EffectInstance::INSTANCE_SAFE;
        } else if (safety == kOfxImageEffectRenderFullySafe) {
            if (effect_->getHostFrameThreading()) {
                _renderSafety =  EffectInstance::FULLY_SAFE_FRAME;
            } else {
                _renderSafety =  EffectInstance::FULLY_SAFE;
            }
        } else {
            qDebug() << "Unknown thread safety level: " << safety.c_str();
            _renderSafety =  EffectInstance::UNSAFE;
        }
        _wasRenderSafetySet = true;
        return _renderSafety;
    }
    
}

bool OfxEffectInstance::makePreviewByDefault() const { return isGenerator();}

const std::string& OfxEffectInstance::getShortLabel() const {
    return effectInstance()->getShortLabel();
}

void OfxEffectInstance::initializeOverlayInteract() {
    tryInitializeOverlayInteracts();
}


void OfxEffectInstance::drawOverlay(double /*scaleX*/,double /*scaleY*/){
    if(!_initialized){
        return;
    }
    if(_overlayInteract){
        OfxPointD rs;
        rs.x = 1.;//scaleX;
        rs.y = 1.;//scaleY;
        OfxTime time = effect_->getFrameRecursive();
        _overlayInteract->drawAction(time, rs);
    }
}

void OfxEffectInstance::setCurrentViewportForOverlays(OverlaySupport* viewport) {
    if (_overlayInteract) {
        _overlayInteract->setCallingViewport(viewport);
    }
}

bool OfxEffectInstance::onOverlayPenDown(double /*scaleX*/,double /*scaleY*/,const QPointF& viewportPos,const QPointF& pos){
    if(!_initialized){
        return false;
    }
    if(_overlayInteract){
        OfxPointD rs;
        rs.x = 1.;//scaleX;
        rs.y = 1.;//scaleY;
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

bool OfxEffectInstance::onOverlayPenMotion(double /*scaleX*/,double /*scaleY*/,const QPointF& viewportPos,const QPointF& pos){
    if(!_initialized){
        return false;
    }
    if(_overlayInteract){
        OfxPointD rs;
        rs.x = 1.;//scaleX;
        rs.y = 1.;//scaleY;
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


bool OfxEffectInstance::onOverlayPenUp(double /*scaleX*/,double /*scaleY*/,const QPointF& viewportPos,const QPointF& pos){
    if(!_initialized){
        return false;
    }
    if(_overlayInteract){
        OfxPointD rs;
        rs.x = 1.;//scaleX;
        rs.y = 1.;//scaleY;
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

bool OfxEffectInstance::onOverlayKeyDown(double /*scaleX*/,double /*scaleY*/,Natron::Key key,Natron::KeyboardModifiers /*modifiers*/){
    if(!_initialized){
        return false;;
    }
    if(_overlayInteract){
        OfxPointD rs;
        rs.x = 1.;//scaleX;
        rs.y = 1.;//scaleY;
        OfxTime time = effect_->getFrameRecursive();
        QByteArray keyStr;
        OfxStatus stat = _overlayInteract->keyDownAction(time, rs, (int)key, keyStr.data());
        if (stat == kOfxStatOK) {
            return true;
        }
    }
    return false;
}

bool OfxEffectInstance::onOverlayKeyUp(double /*scaleX*/,double /*scaleY*/,Natron::Key key,Natron::KeyboardModifiers /* modifiers*/){
    if(!_initialized){
        return false;
    }
    if(_overlayInteract){
        OfxPointD rs;
        rs.x = 1.;//scaleX;
        rs.y = 1.;//scaleY;
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

bool OfxEffectInstance::onOverlayKeyRepeat(double /*scaleX*/,double /*scaleY*/,Natron::Key key,Natron::KeyboardModifiers /*modifiers*/){
    if(!_initialized){
        return false;
    }
    if(_overlayInteract){
        OfxPointD rs;
        rs.x = 1.;//scaleX;
        rs.y = 1.;//scaleY;
        OfxTime time = effect_->getFrameRecursive();
        QByteArray keyStr;
        OfxStatus stat = _overlayInteract->keyRepeatAction(time, rs, (int)key, keyStr.data());
        if (stat == kOfxStatOK) {
            return true;
        }
    }
    return false;
}

bool OfxEffectInstance::onOverlayFocusGained(double /*scaleX*/,double /*scaleY*/){
    if(!_initialized){
        return false;
    }
    if(_overlayInteract){
        OfxPointD rs;
        rs.x = 1.;//scaleX;
        rs.y = 1.;//scaleY;
        OfxTime time = effect_->getFrameRecursive();
        OfxStatus stat = _overlayInteract->gainFocusAction(time, rs);
        assert(stat == kOfxStatOK || stat == kOfxStatReplyDefault);
        if (stat == kOfxStatOK) {
            return true;
        }
    }
    return false;
}

bool OfxEffectInstance::onOverlayFocusLost(double /*scaleX*/,double /*scaleY*/){
    if(!_initialized){
        return false;
    }
    if(_overlayInteract){
        OfxPointD rs;
        rs.x = 1.;//scaleX;
        rs.y = 1.;//scaleY;
        OfxTime time = effect_->getFrameRecursive();
        OfxStatus stat = _overlayInteract->loseFocusAction(time, rs);
        assert(stat == kOfxStatOK || stat == kOfxStatReplyDefault);
        if (stat == kOfxStatOK) {
            return true;
        }
    }
    return false;
}


void OfxEffectInstance::onKnobValueChanged(KnobI* k,Natron::ValueChangedReason reason){
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
        case Natron::PROJECT_LOADING:
        default:
            break;
    }

    if (stat != kOfxStatOK && stat != kOfxStatReplyDefault) {
        QString err(QString(getNode()->getName_mt_safe().c_str()) + ": An error occured while changing parameter " +
                    k->getDescription().c_str());
        appPTR->writeToOfxLog_mt_safe(err);
        return;
    }
    
    if (effect_->isClipPreferencesSlaveParam(k->getName())) {
        ///Recursive action, must not call assertActionIsNotRecursive()
        incrementRecursionLevel();
        effect_->runGetClipPrefsConditionally();
        decrementRecursionLevel();
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
        case Natron::PROJECT_LOADING:
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
        case Natron::PROJECT_LOADING:
        default:
            break;
    }
    assert(stat == kOfxStatOK || stat == kOfxStatReplyDefault);

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
    OFX::Host::ImageEffect::ClipInstance* outputClip =  effectInstance()->getClip(kOfxImageEffectOutputClipName);
    if (!outputClip) {
        return false;
    }
    return effectInstance()->supportsTiles() && outputClip->supportsTiles();
}

bool OfxEffectInstance::supportsRenderScale() const
{
    return effectInstance()->supportsMultiResolution();
}

void OfxEffectInstance::beginEditKnobs() {
    effectInstance()->beginInstanceEditAction();
}

void OfxEffectInstance::onSyncPrivateDataRequested()
{
    ///Can only be called in the main thread
    assert(QThread::currentThread() == qApp->thread());
    effectInstance()->syncPrivateDataAction();
}
