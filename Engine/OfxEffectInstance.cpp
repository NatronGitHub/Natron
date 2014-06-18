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
        
        getNode()->createRotoContextConditionnally();
        
        ///before calling the createInstanceAction, load values
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
                && !clips[i]->isOptional() && !clips[i]->isMask()) {
                appPTR->writeToOfxLog_mt_safe(QString(plugin->getDescriptor().getLabel().c_str())
                                              + "RGBA components not supported by OFX plugin in context " + QString(context.c_str()));
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
        effectInstance()->setClipsHash(hash());
        RenderScale s;
        effectInstance()->getRenderScaleRecursive(s.x, s.y);
        effectInstance()->setClipsMipMapLevel(Natron::Image::getLevelFromScale(s.x));
        effectInstance()->setClipsView(0);
        _overlayInteract->createInstanceAction();
        effectInstance()->discardClipsHash();
        effectInstance()->discardClipsMipMapLevel();
        effectInstance()->discardClipsView();
        getApp()->redrawAllViewers();
    }
    ///for each param, if it has a valid custom interact, create it
    const std::list<OFX::Host::Param::Instance*>& params = effectInstance()->getParamList();
    for (std::list<OFX::Host::Param::Instance*>::const_iterator it = params.begin(); it!=params.end(); ++it) {
        OfxParamToKnob* paramToKnob = dynamic_cast<OfxParamToKnob*>(*it);
        assert(paramToKnob);
        OFX::Host::Interact::Descriptor& interactDesc = paramToKnob->getInteractDesc();
        if (interactDesc.getState() == OFX::Host::Interact::eDescribed) {
            boost::shared_ptr<KnobI> knob = paramToKnob->getKnob();
            boost::shared_ptr<OfxParamOverlayInteract> overlay(new OfxParamOverlayInteract(knob.get(),interactDesc,
                                                                                           effectInstance()->getHandle()));
            
            overlay->createInstanceAction();
            knob->setCustomInteract(overlay);
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
    OFX::Host::ImageEffect::ClipInstance* clip = effect_->getClip(clips[clips.size() - 1 - inputNo]->getName());
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
    assert(inputNb < (int)inputs.size());
    if (inputs[inputs.size()-1-inputNb]->isOptional()) {
        return true;
    } else {
        if (isInputRotoBrush(inputNb)) {
            return true;
        }
    }
    return false;
}

bool OfxEffectInstance::isInputMask(int inputNb) const
{
    MappedInputV inputs = inputClipsCopyWithoutOutput();
    assert(inputNb < (int)inputs.size());
    return inputs[inputs.size()-1-inputNb]->isMask();
}

bool OfxEffectInstance::isInputRotoBrush(int inputNb) const
{
    MappedInputV inputs = inputClipsCopyWithoutOutput();
    assert(inputNb < (int)inputs.size());
    
    ///Maybe too crude ? Not like many plug-ins use the paint context except Natron's roto node.
    return inputs[inputs.size()-1-inputNb]->getName() == "Brush" && getNode()->isRotoNode();
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

bool OfxEffectInstance::ifInfiniteApplyHeuristic(OfxTime time,const RenderScale& scale, int view,OfxRectD* rod) const{
    /*If the rod is infinite clip it to the project's default*/

    Format projectDefault;
    getRenderFormat(&projectDefault);
    /// FIXME: before removing the assert() (I know you are tempted) please explain (here: document!) if the format rectangle can be empty and in what situation(s)
    assert(!projectDefault.isNull());
    
    bool x1Infinite = rod->x1 == kOfxFlagInfiniteMin || rod->x1 == -std::numeric_limits<double>::infinity();
    bool y1Infinite = rod->y1 == kOfxFlagInfiniteMin || rod->y1 == -std::numeric_limits<double>::infinity();
    bool x2Infinite = rod->x2== kOfxFlagInfiniteMax || rod->x2 == std::numeric_limits<double>::infinity();
    bool y2Infinite = rod->y2 == kOfxFlagInfiniteMax || rod->y2  == std::numeric_limits<double>::infinity();
    
    ///Get the union of the inputs.
    RectI inputsUnion;
    for (int i = 0; i < maximumInputs(); ++i) {
        Natron::EffectInstance* input = input_other_thread(i);
        if (input) {
            RectI inputRod;
            bool isProjectFormat;
            Status st = input->getRegionOfDefinition_public(time,scale,view, &inputRod,&isProjectFormat);
            if (st != StatFailed) {
                if (i == 0) {
                    inputsUnion = inputRod;
                } else {
                    inputsUnion.merge(inputRod);
                }
            }
        }
    }

    ///If infinite : clip to inputsUnion if not null, otherwise to project default
    
    // BE CAREFUL:
    // std::numeric_limits<int>::infinity() does not exist (check std::numeric_limits<int>::has_infinity)
    bool isProjectFormat = false;
    if (x1Infinite) {
        if (!inputsUnion.isNull()) {
            rod->x1 = inputsUnion.x1;
        } else {
            rod->x1 = projectDefault.left();
            isProjectFormat = true;
        }
    }
    if (y1Infinite) {
        if (!inputsUnion.isNull()) {
            rod->y1 = inputsUnion.y1;
        } else {
            rod->y1 = projectDefault.bottom();
            isProjectFormat = true;
        }
    }
    if (x2Infinite) {
        if (!inputsUnion.isNull()) {
            rod->x2 = inputsUnion.x2;
        } else {
            rod->x2 = projectDefault.right();
            isProjectFormat = true;
        }
    }
    if (y2Infinite) {
        if (!inputsUnion.isNull()) {
            rod->y2 = inputsUnion.y2;
        } else {
            rod->y2 = projectDefault.top();
            isProjectFormat = true;
        }
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
    
    ///if all non optional clips are connected, call getClipPrefs
    ///The clip preferences action is never called until all non optional clips have been attached to the plugin.
    if (effect_->areAllNonOptionalClipsConnected()) {
        checkClipPrefs(time,s);
    }
}

void OfxEffectInstance::checkClipPrefs(double time,const RenderScale& scale)
{
    assert(QThread::currentThread() == qApp->thread());
    
    effect_->runGetClipPrefsConditionally();
    const std::string& outputClipDepth = effect_->getClip(kOfxImageEffectOutputClipName)->getPixelDepth();

    QString bitDepthWarning("This nodes converts higher bit depths images from its inputs to work. As "
                            "a result of this process, the quality of the images is degraded. The following conversions are done: \n");
    bool setBitDepthWarning = false;
    
    ///for all inputs we run getClipPrefs too on their output clip
    for (int i = 0; i < maximumInputs() ; ++i) {
        OfxEffectInstance* instance = dynamic_cast<OfxEffectInstance*>(input(i));
        OfxClipInstance* clip = getClipCorrespondingToInput(i);
        ///pointer might be null if it is not an OpenFX plug-in.
        if (instance) {
            instance->effectInstance()->beginInstanceChangedAction(kOfxChangeUserEdited);
            instance->effectInstance()->clipInstanceChangedAction(kOfxImageEffectOutputClipName, kOfxChangeUserEdited, time, scale);
            instance->effectInstance()->endInstanceChangedAction(kOfxChangeUserEdited);
            instance->effectInstance()->runGetClipPrefsConditionally();
            
            OFX::Host::ImageEffect::ClipInstance* inputOutputClip = instance->effectInstance()->getClip(kOfxImageEffectOutputClipName);
            const std::string& input_outputClipComps = inputOutputClip->getComponents();
            if (clip->isSupportedComponent(input_outputClipComps)) {
                clip->setComponents(input_outputClipComps);
            }
            
            const std::string & input_outputDepth = inputOutputClip->getPixelDepth();
            Natron::ImageBitDepth input_outputNatronDepth = OfxClipInstance::ofxDepthToNatronDepth(input_outputDepth);
            Natron::ImageBitDepth outputClipDepthNatron = OfxClipInstance::ofxDepthToNatronDepth(outputClipDepth);
            
            if (isSupportedBitDepth(input_outputNatronDepth)) {
                bool depthsDifferent = input_outputNatronDepth != outputClipDepthNatron;
                if ((effect_->supportsMultipleClipDepths() && depthsDifferent) || !depthsDifferent) {
                    clip->setPixelDepth(input_outputDepth);
                }
            } else {
                
                if (Image::isBitDepthConversionLossy(input_outputNatronDepth, outputClipDepthNatron)) {
                    bitDepthWarning.append(instance->getName().c_str());
                    bitDepthWarning.append(" (" + QString(Image::getDepthString(input_outputNatronDepth).c_str()) + ")");
                    bitDepthWarning.append(" ----> ");
                    bitDepthWarning.append(getName().c_str());
                    bitDepthWarning.append(" (" + QString(Image::getDepthString(outputClipDepthNatron).c_str()) + ")");
                    bitDepthWarning.append('\n');
                    setBitDepthWarning = true;
                }
            }


            ///validate with an instance changed action
            effect_->beginInstanceChangedAction(kOfxChangeUserEdited);
            effect_->clipInstanceChangedAction(clip->getName(), kOfxChangeUserEdited, time, scale);
            effect_->endInstanceChangedAction(kOfxChangeUserEdited);
        }
    }
    getNode()->toggleBitDepthWarning(setBitDepthWarning, bitDepthWarning);
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

Natron::Status OfxEffectInstance::getRegionOfDefinition(SequenceTime time,const RenderScale& scale,int view,RectI* rod,
                                                        bool* isProjectFormat){
    if(!_initialized){
        return Natron::StatFailed;
    }

    assert(effect_);
    
    unsigned int mipMapLevel = Natron::Image::getLevelFromScale(scale.x);
    effectInstance()->setClipsMipMapLevel(mipMapLevel);
    effectInstance()->setClipsView(view);

    OfxPointD scaleOne;
    scaleOne.x = scaleOne.y = 1.;
    
    bool useScaleOne = !supportsRenderScale();
    
    OfxRectD ofxRod;
    OfxStatus stat = effect_->getRegionOfDefinitionAction(time, useScaleOne ? scaleOne : (OfxPointD)scale, ofxRod);
    
    if (getRecursionLevel() <= 1) {
        effectInstance()->discardClipsMipMapLevel();
        effectInstance()->discardClipsView();
    } else {
        qDebug() << "getRegionOfDefinition cannot be called recursively as an action. Please check this.";
    }
    
    if (stat!= kOfxStatOK && stat != kOfxStatReplyDefault) {
        return StatFailed;
    }
    
    *isProjectFormat = ifInfiniteApplyHeuristic(time,scale,view,&ofxRod);
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


EffectInstance::RoIMap OfxEffectInstance::getRegionOfInterest(SequenceTime time,RenderScale scale,
                                                              const RectI& renderWindow,int view,U64 nodeHash) {
    
    std::map<OFX::Host::ImageEffect::ClipInstance*,OfxRectD> inputRois;
    EffectInstance::RoIMap ret;
    if(!_initialized){
        return ret;
    }
    
    unsigned int mipMapLevel = Natron::Image::getLevelFromScale(scale.x);
    ///before calling getRoIaction set the relevant infos on the clips
    effectInstance()->setClipsMipMapLevel(mipMapLevel);
    effectInstance()->setClipsView(view);
    effectInstance()->setClipsHash(nodeHash);
    
    OfxPointD scaleOne;
    scaleOne.x = scaleOne.y = 1.;
    
    bool useScaleOne = !supportsRenderScale();
    
    OfxStatus stat = effect_->getRegionOfInterestAction((OfxTime)time, useScaleOne ? scaleOne : scale,
                                                        rectToOfxRect2D(renderWindow), inputRois);
    
    if (getRecursionLevel() <= 1) {
        effectInstance()->discardClipsMipMapLevel();
        effectInstance()->discardClipsView();  
        effectInstance()->discardClipsHash();
    } else {
        qDebug() << "getRegionsOfInterest cannot be called recursively as an action. Please check this.";
    }
    
    if(stat != kOfxStatOK && stat != kOfxStatReplyDefault) {
        Natron::errorDialog(getNode()->getName_mt_safe(), "Failed to specify the region of interest from inputs.");
    }
    if (stat != kOfxStatReplyDefault) {
        for(std::map<OFX::Host::ImageEffect::ClipInstance*,OfxRectD>::iterator it = inputRois.begin();it!= inputRois.end();++it){
             EffectInstance* inputNode = dynamic_cast<OfxClipInstance*>(it->first)->getAssociatedNode();
            //if (inputNode && inputNode != this) {
                RectI inputRoi;
                ofxRectDToEnclosingRectI(it->second, &inputRoi);
                ret.insert(std::make_pair(inputNode,inputRoi));
            // }
        }
    } else if (stat == kOfxStatReplyDefault) {
        for (int i = 0; i < effectInstance()->getNClips(); ++i) {
            EffectInstance* inputNode = dynamic_cast<OfxClipInstance*>(effectInstance()->getNthClip(i))->getAssociatedNode();
            // if (inputNode && inputNode != this) {
                ret.insert(std::make_pair(inputNode, renderWindow));
            //}
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
                if (!clip->isOutput() && !clip->isOptional() && (clip->getName() != "Brush" || !getNode()->isRotoNode())) {
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
                                int view,SequenceTime* inputTime,int* inputNb) {
    OfxRectI ofxRoI;
    ofxRoI.x1 = roi.left();
    ofxRoI.x2 = roi.right();
    ofxRoI.y1 = roi.bottom();
    ofxRoI.y2 = roi.top();
    const std::string field = kOfxImageFieldNone; // TODO: support interlaced data
    std::string inputclip;
    OfxTime inputTimeOfx = time;
    
    OfxPointD scaleOne;
    scaleOne.x = scaleOne.y = 1.;
    
    bool useScaleOne = !supportsRenderScale();
    
    unsigned int mipmapLevel = Image::getLevelFromScale(scale.x);
    effectInstance()->setClipsMipMapLevel(mipmapLevel);
    effectInstance()->setClipsView(view);
    
    OfxStatus stat = effect_->isIdentityAction(inputTimeOfx,field,ofxRoI,useScaleOne ? scaleOne : scale,inputclip);
    
    effectInstance()->discardClipsView();
    effectInstance()->discardClipsMipMapLevel();
    
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
        
        if (natronClip->isOutput()) {
            *inputNb = -2;
        } else {
            *inputNb = natronClip->getInputNb();
        }
        return true;
    }else{
        return false;
    }
}


void OfxEffectInstance::beginSequenceRender(SequenceTime first,SequenceTime last,
                                            SequenceTime step,bool interactive,RenderScale scale,
                                            bool isSequentialRender,bool isRenderResponseToUserInteraction,int view) {
    OfxPointD scaleOne;
    scaleOne.x = scaleOne.y = 1.;
    bool useScaleOne = !supportsRenderScale();
    
    OfxStatus stat = effectInstance()->beginRenderAction(first, last, step, interactive,useScaleOne ? scaleOne :  scale,isSequentialRender,isRenderResponseToUserInteraction,view);
    assert(stat == kOfxStatOK || stat == kOfxStatReplyDefault);
    
}

void OfxEffectInstance::endSequenceRender(SequenceTime first,SequenceTime last,
                                          SequenceTime step,bool interactive,RenderScale scale,
                                          bool isSequentialRender,bool isRenderResponseToUserInteraction,int view) {
    OfxPointD scaleOne;
    scaleOne.x = scaleOne.y = 1.;
    bool useScaleOne = !supportsRenderScale();
    OfxStatus stat = effectInstance()->endRenderAction(first, last, step, interactive,useScaleOne ? scaleOne : scale,isSequentialRender,isRenderResponseToUserInteraction,view);
    assert(stat == kOfxStatOK || stat == kOfxStatReplyDefault);
    
}

Natron::Status OfxEffectInstance::render(SequenceTime time,RenderScale scale,
                                         const RectI& roi,int view,
                                         bool isSequentialRender,bool isRenderResponseToUserInteraction,
                                         boost::shared_ptr<Natron::Image> output){
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
    unsigned int mipMapLevel = Natron::Image::getLevelFromScale(scale.x);
    effectInstance()->setClipsMipMapLevel(mipMapLevel);
    effectInstance()->setClipsRenderedImage(output);
    
    ///This is passed to the render action to plug-ins that don't support render scale
    RenderScale scaleOne;
    scaleOne.x = scaleOne.y = 1.;
    
    bool useScaleOne = !supportsRenderScale();
    
    stat = effect_->renderAction((OfxTime)time, field, ofxRoI,useScaleOne ? scaleOne : scale,
                                 isSequentialRender,isRenderResponseToUserInteraction,view, viewsCount);
    
    effectInstance()->discardClipsMipMapLevel();
    effectInstance()->discardClipsView();
    effectInstance()->discardClipsImage();
    
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
        int view;
        effectInstance()->getViewRecursive(view);
        effectInstance()->setClipsView(view);
        effectInstance()->setClipsMipMapLevel(0);
        effectInstance()->setClipsHash(hash());
        _overlayInteract->drawAction(time, rs);
        effectInstance()->discardClipsView();
        effectInstance()->discardClipsHash();
        effectInstance()->discardClipsMipMapLevel();
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
        
        int view;
        effectInstance()->getViewRecursive(view);
        effectInstance()->setClipsView(view);
        effectInstance()->setClipsMipMapLevel(0);
        effectInstance()->setClipsHash(hash());
        
        OfxStatus stat = _overlayInteract->penDownAction(time, rs, penPos, penPosViewport, 1.);
        
        effectInstance()->discardClipsView();
        effectInstance()->discardClipsHash();
        effectInstance()->discardClipsMipMapLevel();
        
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
        int view;
        effectInstance()->getViewRecursive(view);
        effectInstance()->setClipsView(view);
        effectInstance()->setClipsMipMapLevel(0);
        effectInstance()->setClipsHash(hash());
        
        OfxStatus stat = _overlayInteract->penMotionAction(time, rs, penPos, penPosViewport, 1.);
        
        effectInstance()->discardClipsView();
        effectInstance()->discardClipsHash();
        effectInstance()->discardClipsMipMapLevel();
        
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
        
        int view;
        effectInstance()->getViewRecursive(view);
        effectInstance()->setClipsView(view);
        effectInstance()->setClipsMipMapLevel(0);
        effectInstance()->setClipsHash(hash());
        
        OfxStatus stat = _overlayInteract->penUpAction(time, rs, penPos, penPosViewport, 1.);
        
        effectInstance()->discardClipsView();
        effectInstance()->discardClipsHash();
        effectInstance()->discardClipsMipMapLevel();
        
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
        
        int view;
        effectInstance()->getViewRecursive(view);
        effectInstance()->setClipsView(view);
        effectInstance()->setClipsMipMapLevel(0);
        effectInstance()->setClipsHash(hash());
        OfxStatus stat = _overlayInteract->keyDownAction(time, rs, (int)key, keyStr.data());
        
        effectInstance()->discardClipsView();
        effectInstance()->discardClipsHash();
        effectInstance()->discardClipsMipMapLevel();
        
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
        
        int view;
        effectInstance()->getViewRecursive(view);
        effectInstance()->setClipsView(view);
        effectInstance()->setClipsMipMapLevel(0);
        effectInstance()->setClipsHash(hash());
        
        OfxStatus stat = _overlayInteract->keyUpAction(time, rs, (int)key, keyStr.data());
        
        effectInstance()->discardClipsView();
        effectInstance()->discardClipsHash();
        effectInstance()->discardClipsMipMapLevel();
        
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
        
        int view;
        effectInstance()->getViewRecursive(view);
        effectInstance()->setClipsView(view);
        effectInstance()->setClipsMipMapLevel(0);
        effectInstance()->setClipsHash(hash());
        
        OfxStatus stat = _overlayInteract->keyRepeatAction(time, rs, (int)key, keyStr.data());
        
        effectInstance()->discardClipsView();
        effectInstance()->discardClipsHash();
        effectInstance()->discardClipsMipMapLevel();
        
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
        
        int view;
        effectInstance()->getViewRecursive(view);
        effectInstance()->setClipsView(view);
        effectInstance()->setClipsMipMapLevel(0);
        effectInstance()->setClipsHash(hash());
        
        OfxStatus stat = _overlayInteract->gainFocusAction(time, rs);
        
        effectInstance()->discardClipsView();
        effectInstance()->discardClipsHash();
        effectInstance()->discardClipsMipMapLevel();
        
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
        
        int view;
        effectInstance()->getViewRecursive(view);
        effectInstance()->setClipsView(view);
        effectInstance()->setClipsMipMapLevel(0);
        effectInstance()->setClipsHash(hash());
        
        OfxStatus stat = _overlayInteract->loseFocusAction(time, rs);
        
        effectInstance()->discardClipsView();
        effectInstance()->discardClipsHash();
        effectInstance()->discardClipsMipMapLevel();
        
        assert(stat == kOfxStatOK || stat == kOfxStatReplyDefault);
        if (stat == kOfxStatOK) {
            return true;
        }
    }
    return false;
}


void OfxEffectInstance::knobChanged(KnobI* k,Natron::ValueChangedReason reason){
    if(!_initialized){
        return;
    }
      
    if (_renderButton && k == _renderButton.get()) {
        
        ///don't do anything since it is handled upstream
        return;
    }
    
    OfxPointD renderScale;
    effect_->getRenderScaleRecursive(renderScale.x, renderScale.y);
    unsigned int mipMapLevel = Natron::Image::getLevelFromScale(renderScale.x);
    int view;
    effectInstance()->getViewRecursive(view);
    
    if (getRecursionLevel() == 1) {
        effectInstance()->setClipsMipMapLevel(mipMapLevel);
        effectInstance()->setClipsView(view);
        effectInstance()->setClipsHash(hash());
    }
    
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

    if (getRecursionLevel() == 1) {
        effectInstance()->discardClipsHash();
        effectInstance()->discardClipsMipMapLevel();
        effectInstance()->discardClipsView();
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



void OfxEffectInstance::addAcceptedComponents(int inputNb,std::list<Natron::ImageComponents>* comps)
{
    if (inputNb >= 0) {
        OfxClipInstance* clip = getClipCorrespondingToInput(inputNb);
        assert(clip);
        const std::vector<std::string>& supportedComps = clip->getSupportedComponents();
        for (U32 i = 0; i < supportedComps.size(); ++i) {
            comps->push_back(OfxClipInstance::ofxComponentsToNatronComponents(supportedComps[i]));
        }
    } else {
        assert(inputNb == -1);
        OfxClipInstance* clip = dynamic_cast<OfxClipInstance*>(effectInstance()->getClip(kOfxImageEffectOutputClipName));
        assert(clip);
        const std::vector<std::string>& supportedComps = clip->getSupportedComponents();
        for (U32 i = 0; i < supportedComps.size(); ++i) {
            comps->push_back(OfxClipInstance::ofxComponentsToNatronComponents(supportedComps[i]));
        }
    }

}

static Natron::ImageBitDepth ofxBitDepthToNatron(const std::string& bitDepth)
{
    if (bitDepth == kOfxBitDepthFloat) {
        return Natron::IMAGE_FLOAT;
    } else if (bitDepth == kOfxBitDepthByte) {
        return Natron::IMAGE_BYTE;
    } else {
        return Natron::IMAGE_SHORT;
    }
}

void OfxEffectInstance::addSupportedBitDepth(std::list<Natron::ImageBitDepth>* depths) const
{
    const OFX::Host::Property::Set& prop = effectInstance()->getPlugin()->getDescriptor().getParamSetProps();
    int dim = prop.getDimension(kOfxImageEffectPropSupportedPixelDepths);
    for (int i = 0; i < dim ; ++i) {
        depths->push_back(ofxBitDepthToNatron(prop.getStringProperty(kOfxImageEffectPropSupportedPixelDepths,i)));
    }
}

void OfxEffectInstance::getPreferredDepthAndComponents(int inputNb,Natron::ImageComponents* comp,Natron::ImageBitDepth* depth) const
{
    OfxClipInstance* clip;
    if (inputNb == -1) {
        clip = dynamic_cast<OfxClipInstance*>(effect_->getClip(kOfxImageEffectOutputClipName));
    } else {
        clip = getClipCorrespondingToInput(inputNb);
    }
    assert(clip);
    *comp = OfxClipInstance::ofxComponentsToNatronComponents(clip->getComponents());
    *depth = OfxClipInstance::ofxDepthToNatronDepth(clip->getPixelDepth());
}

Natron::SequentialPreference OfxEffectInstance::getSequentialPreference() const
{
    int sequential = effect_->getPlugin()->getDescriptor().getProps().getIntProperty(kOfxImageEffectInstancePropSequentialRender);
    switch (sequential) {
        case 0:
            return Natron::EFFECT_NOT_SEQUENTIAL;
        case 1:
            return Natron::EFFECT_ONLY_SEQUENTIAL;
        case 2:
            return Natron::EFFECT_PREFER_SEQUENTIAL;
        default:
            return Natron::EFFECT_NOT_SEQUENTIAL;
            break;
    }
}

