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
    if (comp == kOfxImageComponentAlpha) {
        out += Channel_alpha;
    } else if (comp == kOfxImageComponentRGB) {
        out += Mask_RGB;
    } else if (comp == kOfxImageComponentRGBA) {
        out += Mask_RGBA;
    } else if (comp == kOfxImageComponentYUVA) {
        out += Mask_RGBA;
    }
    return out;
}
}
#endif

namespace  {
    
    /**
     * @class This class is helpful to set thread-storage data on the clips of an effect
     * When destroyed, it is removed from the clips, ensuring they are removed.
     * It is to be instantiated right before calling the action that will need the per thread-storage
     * This way even if exceptions are thrown, clip thread-storage will be purged.
     **/
    class ClipsThreadStorageSetter
    {
    public:
        ClipsThreadStorageSetter(OfxImageEffectInstance* effect,
                                 bool skipDiscarding, //< this is in case a recursive action is called
                                 bool setMipMapLevel,
                                 unsigned int mipMapLevel,
                                 bool setRenderedImage,
                                 const boost::shared_ptr<Natron::Image>& renderedImage,
                                 bool setView,
                                 int view,
                                 bool setOutputRoD,
                                 const RectD& rod, //!< output RoD in canonical coordinates
                                 bool setFrameRange,
                                 double first,double last)
        : effect(effect)
        , skipDiscarding(skipDiscarding)
        , mipmaplvlSet(setMipMapLevel)
        , renderedImageSet(setRenderedImage)
        , viewSet(setView)
        , outputRoDSet(setOutputRoD)
        , frameRangeSet(setFrameRange)
        {
            if (setMipMapLevel) {
                effect->setClipsMipMapLevel(mipMapLevel);
            }
            if (setRenderedImage) {
                effect->setClipsRenderedImage(renderedImage);
            }
            if (setView) {
                effect->setClipsView(view);
            }
            if (setOutputRoD) {
                effect->setClipsOutputRoD(rod);
            }
            if (setFrameRange) {
                effect->setClipsFrameRange(first, last);
            }
        }
        
        ~ClipsThreadStorageSetter()
        {
            if (!skipDiscarding) {
                if (mipmaplvlSet) {
                    effect->discardClipsMipMapLevel();
                }
                if (renderedImageSet) {
                    effect->discardClipsImage();
                }
                if (viewSet) {
                    effect->discardClipsView();
                }
                if (outputRoDSet) {
                    effect->discardClipsOutputRoD();
                }
                if (frameRangeSet) {
                    effect->discardClipsFrameRange();
                }
            }
        }
    private:
        OfxImageEffectInstance* effect;
        bool skipDiscarding;

        bool mipmaplvlSet;
        bool renderedImageSet;
        bool viewSet;
        bool outputRoDSet;
        bool frameRangeSet;
    };
}

OfxEffectInstance::OfxEffectInstance(boost::shared_ptr<Natron::Node> node)
    : AbstractOfxEffectInstance(node)
    , effect_()
    , _natronPluginID()
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

void
OfxEffectInstance::createOfxImageEffectInstance(OFX::Host::ImageEffect::ImageEffectPlugin* plugin,
                                                const std::string& context,const NodeSerialization* serialization)
{
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
        
        _natronPluginID = generateImageEffectClassName(effect_->getPlugin()->getIdentifier(),
                                                       effect_->getPlugin()->getVersionMajor(),
                                                       effect_->getPlugin()->getVersionMinor(),
                                                       effect_->getDescriptor().getShortLabel(),
                                                       effect_->getDescriptor().getLabel(),
                                                       effect_->getDescriptor().getLongLabel(),
                                                       effect_->getDescriptor().getPluginGrouping());

        
        blockEvaluation();

        OfxStatus stat = effect_->populate();
        
        initializeContextDependentParams();
        
        effect_->addParamsToTheirParents();

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
        unblockEvaluation();
        
        if (stat != kOfxStatOK && stat != kOfxStatReplyDefault) {
            throw std::runtime_error("Could not create effect instance for plugin");
        }

        
        if (!effect_->getClipPreferences()) {
           qDebug() << "The plugin failed in the getClipPreferencesAction.";
        }
#pragma message WARN("FIXME: Check here that bitdepth and components given by getClipPreferences are supported by the effect")
        // FIXME: Check here that bitdepth and components given by getClipPreferences are supported by the effect.
        // If we don't, the following assert will crash at the beginning of EffectInstance::renderRoIInternal():
        // assert(isSupportedBitDepth(outputDepth) && isSupportedComponent(-1, outputComponents));
        // If a component/bitdepth is not supported (this is probably a plugin bug), use the closest one, but don't crash Natron.

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

OfxEffectInstance::~OfxEffectInstance()
{
    if (_overlayInteract) {
        delete _overlayInteract;
    }
    
    delete effect_;
    delete _renderSafetyLock;
}



void
OfxEffectInstance::initializeContextDependentParams()
{
    if (isWriter()) {
        _renderButton = Natron::createKnob<Button_Knob>(this, "Render");
        _renderButton->setHintToolTip("Starts rendering the specified frame range.");
        _renderButton->setAsRenderButton();
    }
}

std::string
OfxEffectInstance::description() const
{
    if (effectInstance()) {
        return effectInstance()->getProps().getStringProperty(kOfxPropPluginDescription);
    } else {
        return "";
    }
}

void
OfxEffectInstance::tryInitializeOverlayInteracts()
{
    /*create overlay instance if any*/
    OfxPluginEntryPoint *overlayEntryPoint = effect_->getOverlayInteractMainEntry();
    if (overlayEntryPoint) {
        _overlayInteract = new OfxOverlayInteract(*effect_,8,true);
        RenderScale s;
        effectInstance()->getRenderScaleRecursive(s.x, s.y);
        
        {
            ClipsThreadStorageSetter clipSetter(effectInstance(),
                                     false,
                                     true, //< setMipMapLevel ?
                                     Image::getLevelFromScale(s.x),
                                     false, //< setRenderedImage ?
                                     boost::shared_ptr<Image>(),
                                     true, //< setView ?
                                     0,
                                     false, //< setOutputRoD ?
                                     RectD(),
                                     false,
                                     0,0);
            
            
            _overlayInteract->createInstanceAction();
            
        }
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

bool
OfxEffectInstance::isOutput() const
{
    return _isOutput;
}

bool
OfxEffectInstance::isGenerator() const
{
    assert(effectInstance());
    const std::set<std::string>& contexts = effectInstance()->getPlugin()->getContexts();
    std::set<std::string>::const_iterator foundGenerator = contexts.find(kOfxImageEffectContextGenerator);
    std::set<std::string>::const_iterator foundReader = contexts.find(kOfxImageEffectContextReader);
    if (foundGenerator != contexts.end() || foundReader!= contexts.end())
        return true;
    return false;
}

bool
OfxEffectInstance::isReader() const
{
    assert(effectInstance());
    const std::set<std::string>& contexts = effectInstance()->getPlugin()->getContexts();
    std::set<std::string>::const_iterator foundReader = contexts.find(kOfxImageEffectContextReader);
    if (foundReader != contexts.end()) {
        return true;
    }
    return false;
}

bool
OfxEffectInstance::isWriter() const
{
    assert(effectInstance());
    const std::set<std::string>& contexts = effectInstance()->getPlugin()->getContexts();
    std::set<std::string>::const_iterator foundWriter = contexts.find(kOfxImageEffectContextWriter);
    if (foundWriter != contexts.end()) {
        return true;
    }
    return false;
}

bool
OfxEffectInstance::isGeneratorAndFilter() const
{
    const std::set<std::string>& contexts = effectInstance()->getPlugin()->getContexts();
    std::set<std::string>::const_iterator foundGenerator = contexts.find(kOfxImageEffectContextGenerator);
    std::set<std::string>::const_iterator foundGeneral = contexts.find(kOfxImageEffectContextGeneral);
    return foundGenerator!=contexts.end() && foundGeneral!=contexts.end();
}


/*group is a string as such:
 Toto/Superplugins/blabla
 This functions extracts the all parts of such a grouping, e.g in this case
 it would return [Toto,Superplugins,blabla].*/
static
QStringList ofxExtractAllPartsOfGrouping(const QString& pluginIdentifier, int /*versionMajor*/, int /*versionMinor*/, const QString& /*pluginLabel*/, const QString& str)
{
    QString s(str);
    s.replace(QChar('\\'),QChar('/'));

    QStringList out;
    if (pluginIdentifier.startsWith("com.genarts.sapphire.") || s.startsWith("Sapphire ") || str.startsWith(" Sapphire ")) {
        out.push_back("Sapphire");
    } else if (pluginIdentifier.startsWith("com.genarts.monsters.") ||s.startsWith("Monsters ") || str.startsWith(" Monsters ")) {
        out.push_back("Monsters");
    } else if (pluginIdentifier == "uk.co.thefoundry.keylight.keylight") {
        s = PLUGIN_GROUP_KEYER;
    } else if (pluginIdentifier == "uk.co.thefoundry.noisetools.denoise") {
        s = PLUGIN_GROUP_FILTER;
    } else if ((pluginIdentifier == "tuttle.anisotropicdiffusion") ||
               (pluginIdentifier == "tuttle.anisotropictensors") ||
               (pluginIdentifier == "tuttle.blur") ||
               (pluginIdentifier == "tuttle.floodfill") ||
               (pluginIdentifier == "tuttle.localmaxima") ||
               (pluginIdentifier == "tuttle.nlmdenoiser") ||
               (pluginIdentifier == "tuttle.sobel") ||
               (pluginIdentifier == "tuttle.thinning")) {
        s = PLUGIN_GROUP_FILTER;
    } else if ((pluginIdentifier == "tuttle.bitdepth") ||
               (pluginIdentifier == "tuttle.colorgradation") ||
               (pluginIdentifier == "tuttle.colorsuppress") ||
               (pluginIdentifier == "tuttle.colortransfer") ||
               (pluginIdentifier == "tuttle.ctl") ||
               (pluginIdentifier == "tuttle.gamma") ||
               (pluginIdentifier == "tuttle.invert") ||
               (pluginIdentifier == "tuttle.normalize")) {
        s = PLUGIN_GROUP_COLOR;
    } else if ((pluginIdentifier == "tuttle.ocio.colorspace") ||
               (pluginIdentifier == "tuttle.ocio.lut")) {
        out.push_back(PLUGIN_GROUP_COLOR);
        s = "OCIO";
    } else if ((pluginIdentifier == "tuttle.histogramkeyer") ||
               (pluginIdentifier == "tuttle.idkeyer")) {
        s = PLUGIN_GROUP_KEYER;
    } else if ((pluginIdentifier == "tuttle.avreader") ||
               (pluginIdentifier == "tuttle.avwriter") ||
               (pluginIdentifier == "tuttle.dpxwriter") ||
               (pluginIdentifier == "tuttle.exrreader") ||
               (pluginIdentifier == "tuttle.exrwriter") ||
               (pluginIdentifier == "tuttle.imagemagickreader") ||
               (pluginIdentifier == "tuttle.jpeg2000reader") ||
               (pluginIdentifier == "tuttle.jpeg2000writer") ||
               (pluginIdentifier == "tuttle.jpegreader") ||
               (pluginIdentifier == "tuttle.jpegwriter") ||
               (pluginIdentifier == "tuttle.oiioreader") ||
               (pluginIdentifier == "tuttle.oiiowriter") ||
               (pluginIdentifier == "tuttle.pngreader") ||
               (pluginIdentifier == "tuttle.pngwriter") ||
               (pluginIdentifier == "tuttle.rawreader") ||
               (pluginIdentifier == "tuttle.turbojpegreader") ||
               (pluginIdentifier == "tuttle.turbojpegwriter")) {
        out.push_back(PLUGIN_GROUP_IMAGE);
        if (pluginIdentifier.endsWith("reader")) {
            s = PLUGIN_GROUP_IMAGE_READERS;
        } else {
            s = PLUGIN_GROUP_IMAGE_WRITERS;
        }
    } else if ((pluginIdentifier == "tuttle.checkerboard") ||
               (pluginIdentifier == "tuttle.colorbars") ||
               (pluginIdentifier == "tuttle.colorcube") ||
               (pluginIdentifier == "tuttle.colorwheel") ||
               (pluginIdentifier == "tuttle.constant") ||
               (pluginIdentifier == "tuttle.inputbuffer") ||
               (pluginIdentifier == "tuttle.outputbuffer") ||
               (pluginIdentifier == "tuttle.ramp") ||
               (pluginIdentifier == "tuttle.seexpr")) {
        s = PLUGIN_GROUP_IMAGE;
    } else if ((pluginIdentifier == "tuttle.text")) {
        s = PLUGIN_GROUP_PAINT;
    } else if ((pluginIdentifier == "tuttle.component") ||
               (pluginIdentifier == "tuttle.merge")) {
        s = PLUGIN_GROUP_MERGE;
    } else if ((pluginIdentifier == "tuttle.crop") ||
               (pluginIdentifier == "tuttle.flip") ||
               (pluginIdentifier == "tuttle.lensdistort") ||
               (pluginIdentifier == "tuttle.pinning") ||
               (pluginIdentifier == "tuttle.pushpixel") ||
               (pluginIdentifier == "tuttle.resize") ||
               (pluginIdentifier == "tuttle.swscale")) {
        s = PLUGIN_GROUP_TRANSFORM;
    } else if ((pluginIdentifier == "tuttle.mathoperator")) {
        out.push_back(PLUGIN_GROUP_COLOR);
        s = "Math";
    } else if ((pluginIdentifier == "tuttle.timeshift")) {
        s = PLUGIN_GROUP_TIME;
    }
    /*
     (pluginIdentifier == "tuttle.diff") ||
     (pluginIdentifier == "tuttle.dummy") ||
     (pluginIdentifier == "tuttle.histogram") ||
     (pluginIdentifier == "tuttle.imagestatistics") ||
     (pluginIdentifier == "tuttle.viewer") ||
     */
    return out + s.split('/');
}

QStringList
AbstractOfxEffectInstance::getPluginGrouping(const std::string& pluginIdentifier, int versionMajor, int versionMinor, const std::string& pluginLabel, const std::string& grouping)
{
    //printf("%s,%s\n",pluginLabel.c_str(),grouping.c_str());
    return ofxExtractAllPartsOfGrouping(pluginIdentifier.c_str(), versionMajor, versionMinor, pluginLabel.c_str(),grouping.c_str());
}

std::string
AbstractOfxEffectInstance::getPluginLabel(const std::string& shortLabel,
                                          const std::string& label,
                                          const std::string& longLabel)
{
    std::string labelToUse = label;
    if (labelToUse.empty()) {
        labelToUse = shortLabel;
    }
    if (labelToUse.empty()) {
        labelToUse = longLabel;
    }
    return labelToUse;
}

std::string
AbstractOfxEffectInstance::generateImageEffectClassName(const std::string& pluginIdentifier,
                                                        int versionMajor,
                                                        int versionMinor,
                                                        const std::string& shortLabel,
                                                        const std::string& label,
                                                        const std::string& longLabel,
                                                        const std::string& grouping)
{
    std::string labelToUse = getPluginLabel(shortLabel, label, longLabel);
    QStringList groups = getPluginGrouping(pluginIdentifier, versionMajor, versionMinor, labelToUse, grouping);

    if (labelToUse == "Viewer") { // we don't want a plugin to have the same name as our viewer
        labelToUse =  groups[0].toStdString() + longLabel;
    }
    if (groups.size() >= 1) {
        labelToUse.append("  [");
        labelToUse.append(groups[0].toStdString());
        labelToUse.append("]");
    }
    
    return labelToUse;
}

std::string
OfxEffectInstance::pluginID() const
{
    return _natronPluginID;
}

std::string
OfxEffectInstance::pluginLabel() const
{
    assert(effect_);
    return getPluginLabel( effect_->getDescriptor().getShortLabel(),effect_->getDescriptor().getLabel(),effect_->getDescriptor().getLongLabel());
}

void
OfxEffectInstance::pluginGrouping(std::list<std::string>* grouping) const
{
    std::string groupStr = effectInstance()->getPluginGrouping();
    std::string label = pluginLabel();
    const OFX::Host::ImageEffect::ImageEffectPlugin *p = effectInstance()->getPlugin();
    QStringList groups = ofxExtractAllPartsOfGrouping(p->getIdentifier().c_str(), p->getVersionMajor(), p->getVersionMinor(), label.c_str(), groupStr.c_str());
    for (int i = 0; i < groups.size(); ++i) {
        grouping->push_back(groups[i].toStdString());
    }
}

std::string
OfxEffectInstance::inputLabel(int inputNb) const
{
    
    MappedInputV copy = inputClipsCopyWithoutOutput();
    if (inputNb < (int)copy.size()) {
        return copy[copy.size()-1-inputNb]->getShortLabel();
    } else {
        return EffectInstance::inputLabel(inputNb);
    }
}

OfxEffectInstance::MappedInputV
OfxEffectInstance::inputClipsCopyWithoutOutput() const
{
    assert(effectInstance());
    const std::vector<OFX::Host::ImageEffect::ClipDescriptor*>& clips = effectInstance()->getDescriptor().getClipsByOrder();
    MappedInputV copy;
    for (U32 i = 0; i < clips.size(); ++i) {
        assert(clips[i]);
        if (clips[i]->getShortLabel() != kOfxImageEffectOutputClipName) {
            copy.push_back(clips[i]);
            // cout << "Clip[" << i << "] = " << clips[i]->getShortLabel() << endl;
        }
    }
    return copy;
}

OfxClipInstance*
OfxEffectInstance::getClipCorrespondingToInput(int inputNo) const
{
    OfxEffectInstance::MappedInputV clips = inputClipsCopyWithoutOutput();
    assert(inputNo < (int)clips.size());
    OFX::Host::ImageEffect::ClipInstance* clip = effect_->getClip(clips[clips.size() - 1 - inputNo]->getName());
    assert(clip);
    return dynamic_cast<OfxClipInstance*>(clip);
}

int
OfxEffectInstance::maximumInputs() const
{
    const std::string& context = effectInstance()->getContext();
    if (context == kOfxImageEffectContextReader ||
        context == kOfxImageEffectContextGenerator) {
        return 0;
    } else {
        assert(effectInstance());
        int totalClips = effectInstance()->getDescriptor().getClips().size();
        return totalClips > 0  ?  totalClips-1 : 0;
    }
    
}

bool
OfxEffectInstance::isInputOptional(int inputNb) const
{
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

bool
OfxEffectInstance::isInputMask(int inputNb) const
{
    MappedInputV inputs = inputClipsCopyWithoutOutput();
    assert(inputNb < (int)inputs.size());
    return inputs[inputs.size()-1-inputNb]->isMask();
}

bool
OfxEffectInstance::isInputRotoBrush(int inputNb) const
{
    MappedInputV inputs = inputClipsCopyWithoutOutput();
    assert(inputNb < (int)inputs.size());
    
    ///Maybe too crude ? Not like many plug-ins use the paint context except Natron's roto node.
    return inputs[inputs.size()-1-inputNb]->getName() == "Roto" && getNode()->isRotoNode();
}


void
OfxEffectInstance::onInputChanged(int inputNo)
{
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
        checkClipPrefs(time,s,kOfxChangeUserEdited);
    }
}

void
OfxEffectInstance::checkClipPrefs(double time,const RenderScale& scale, const std::string& reason)
{
    assert(QThread::currentThread() == qApp->thread());
    
    effect_->runGetClipPrefsConditionally();
    const std::string& outputClipDepth = effect_->getClip(kOfxImageEffectOutputClipName)->getPixelDepth();

    QString bitDepthWarning("This nodes converts higher bit depths images from its inputs to work. As "
                            "a result of this process, the quality of the images is degraded. The following conversions are done: \n");
    bool setBitDepthWarning = false;
    
    ///We remap all the input clips components to be the same as the output clip, except for the masks.
    OFX::Host::ImageEffect::ClipInstance* outputClip = effectInstance()->getClip(kOfxImageEffectOutputClipName);
    assert(outputClip);
    std::string outputComponents = outputClip->getComponents();
    
    effect_->beginInstanceChangedAction(reason);

    for (int i = 0; i < maximumInputs() ; ++i) {
        OfxEffectInstance* instance = dynamic_cast<OfxEffectInstance*>(input(i));
        OfxClipInstance* clip = getClipCorrespondingToInput(i);

        if (instance) {
            OFX::Host::ImageEffect::ClipInstance* inputOutputClip = instance->effectInstance()->getClip(kOfxImageEffectOutputClipName);

            if (clip->isSupportedComponent(outputComponents)) {
                ///we only take into account non mask clips for the most components
                if (!clip->isMask()) {
                    clip->setComponents(outputComponents);
                }
            }
            
            ///Try to remap the clip's bitdepth to be the same as the output depth of the input node
            const std::string & input_outputDepth = inputOutputClip->getPixelDepth();
            Natron::ImageBitDepth input_outputNatronDepth = OfxClipInstance::ofxDepthToNatronDepth(input_outputDepth);
            Natron::ImageBitDepth outputClipDepthNatron = OfxClipInstance::ofxDepthToNatronDepth(outputClipDepth);
            
            if (isSupportedBitDepth(input_outputNatronDepth)) {
                bool depthsDifferent = input_outputNatronDepth != outputClipDepthNatron;
                if ((effect_->supportsMultipleClipDepths() && depthsDifferent) || !depthsDifferent) {
                    clip->setPixelDepth(input_outputDepth);
                }
            } else if (Image::isBitDepthConversionLossy(input_outputNatronDepth, outputClipDepthNatron)) {
                bitDepthWarning.append(instance->getName().c_str());
                bitDepthWarning.append(" (" + QString(Image::getDepthString(input_outputNatronDepth).c_str()) + ")");
                bitDepthWarning.append(" ----> ");
                bitDepthWarning.append(getName().c_str());
                bitDepthWarning.append(" (" + QString(Image::getDepthString(outputClipDepthNatron).c_str()) + ")");
                bitDepthWarning.append('\n');
                setBitDepthWarning = true;
            }
            effect_->clipInstanceChangedAction(outputClip->getName(), reason, time, scale);
        }
    }
    effect_->endInstanceChangedAction(reason);

    getNode()->toggleBitDepthWarning(setBitDepthWarning, bitDepthWarning);
}

void
OfxEffectInstance::onMultipleInputsChanged()
{
    ///Recursive action, must not call assertActionIsNotRecursive()
    incrementRecursionLevel();
    effect_->runGetClipPrefsConditionally();
    decrementRecursionLevel();
}

std::vector<std::string>
OfxEffectInstance::supportedFileFormats() const
{
    int formatsCount = effect_->getDescriptor().getProps().getDimension(kTuttleOfxImageEffectPropSupportedExtensions);
    std::vector<std::string> formats(formatsCount);
    for (int k = 0; k < formatsCount; ++k) {
        formats[k] = effect_->getDescriptor().getProps().getStringProperty(kTuttleOfxImageEffectPropSupportedExtensions,k);
        std::transform(formats[k].begin(), formats[k].end(), formats[k].begin(), ::tolower);
    }
    return formats;
}

Natron::Status
OfxEffectInstance::getRegionOfDefinition(SequenceTime time,const RenderScale& scale,int view, RectD* rod)
{
    if (!_initialized) {
        return Natron::StatFailed;
    }

    assert(effect_);
    
    unsigned int mipMapLevel = Natron::Image::getLevelFromScale(scale.x);
    
    OfxPointD scaleOne;
    scaleOne.x = scaleOne.y = 1.;
    
    bool useScaleOne = !supportsRenderScale();
    OfxRectD ofxRod;
    OfxStatus stat;
    
    {
        bool skipDiscarding = false;
        if (getRecursionLevel() > 1) {
            qDebug() << "getRegionOfDefinition cannot be called recursively as an action. Please check this.";
            skipDiscarding = true;
        }
        ClipsThreadStorageSetter clipSetter(effectInstance(),
                                 skipDiscarding,
                                 true, //< setMipMapLevel ?
                                 mipMapLevel,
                                 false, //< setRenderedImage ?
                                 boost::shared_ptr<Image>(),
                                 true, //< setView ?
                                 view,
                                 false, //< setOutputRoD ?
                                 RectD(),
                                 false, //< setFrameRange ?
                                 0,0);

        stat = effect_->getRegionOfDefinitionAction(time, useScaleOne ? scaleOne : (OfxPointD)scale, ofxRod);
        *rod = RectD::fromOfxRectD(ofxRod);
    }
    
    if (stat!= kOfxStatOK && stat != kOfxStatReplyDefault) {
        return StatFailed;
    }

    ///If the rod is 1 pixel, determine if it was because one clip was unconnected or this is really a
    ///1 pixel large image
    if (ofxRod.x2 == 1. && ofxRod.y2 == 1. && ofxRod.x1 == 0. && ofxRod.y1 == 0.) {
        int maxInputs = maximumInputs();
        for (int i = 0; i < maxInputs; ++i) {
            OfxClipInstance* clip = getClipCorrespondingToInput(i);
            if (clip && !clip->getConnected() && !clip->isOptional() && !clip->isMask()) {
                ///this is a mandatory source clip and it is not connected, return statfailed
                return StatFailed;
            }
        }
    }
    
    return StatOK;
    
    // OFX::Host::ImageEffect::ClipInstance* clip = effectInstance()->getClip(kOfxImageEffectOutputClipName);
    //assert(clip);
    //double pa = clip->getAspectRatio();
}

RectD
OfxEffectInstance::calcDefaultRegionOfDefinition(SequenceTime time,
                                                 const RenderScale& scale) const
{
    if (!_initialized) {
        throw std::runtime_error("OfxEffectInstance not initialized");
    }
    OfxRectD rod = effect_->calcDefaultRegionOfDefinition(time, (OfxPointD)scale);
    return RectD(rod.x1, rod.y1, rod.x2, rod.y2);
}

static void
rectToOfxRectD(const RectD& b, OfxRectD *out)
{
    out->x1 = b.left();
    out->x2 = b.right();
    out->y1 = b.bottom();
    out->y2 = b.top();
}


EffectInstance::RoIMap
OfxEffectInstance::getRegionsOfInterest(SequenceTime time,
                                        const RenderScale& scale,
                                        const RectD& outputRoD,
                                        const RectD& renderWindow, //!< the region to be rendered in the output image, in Canonical Coordinates
                                        int view)
{
    std::map<OFX::Host::ImageEffect::ClipInstance*,OfxRectD> inputRois;
    EffectInstance::RoIMap ret;
    if (!_initialized) {
        return ret;
    }
    assert(outputRoD.x2 >= outputRoD.x1 && outputRoD.y2 >= outputRoD.y1);
    assert(renderWindow.x2 >= renderWindow.x1 && renderWindow.y2 >= renderWindow.y1);

    unsigned int mipMapLevel = Natron::Image::getLevelFromScale(scale.x);
    OfxPointD scaleOne;
    scaleOne.x = scaleOne.y = 1.;
    
    bool useScaleOne = !supportsRenderScale();
    OfxStatus stat;
    
    ///before calling getRoIaction set the relevant infos on the clips
  
    
    {
        bool skipDiscarding = false;
        if (getRecursionLevel() > 1) {
            qDebug() << "getRegionsOfInterest cannot be called recursively as an action. Please check this.";
            skipDiscarding = true;
        }
        ClipsThreadStorageSetter clipSetter(effectInstance(),
                                 skipDiscarding,
                                 true, //< setMipMapLevel ?
                                 mipMapLevel,
                                 false, //< setRenderedImage ?
                                 boost::shared_ptr<Image>(),
                                 true, //< setView ?
                                 view,
                                 true, //< setOutputRoD ?
                                 outputRoD,
                                 false,
                                 0,0); //< setFrameRange ?
        OfxRectD roi;
        rectToOfxRectD(renderWindow, &roi);
        stat = effect_->getRegionOfInterestAction((OfxTime)time, useScaleOne ? scaleOne : scale,
                                                  roi, inputRois);
        
    }
    

    if (stat != kOfxStatOK && stat != kOfxStatReplyDefault) {
        appPTR->writeToOfxLog_mt_safe(QString(getNode()->getName_mt_safe().c_str()) + "Failed to specify the region of interest from inputs.");
    }
    if (stat != kOfxStatReplyDefault) {
        for(std::map<OFX::Host::ImageEffect::ClipInstance*,OfxRectD>::iterator it = inputRois.begin();it!= inputRois.end();++it) {
            EffectInstance* inputNode = dynamic_cast<OfxClipInstance*>(it->first)->getAssociatedNode();
            RectD inputRoi; // input RoI in canonical coordinates
            inputRoi.x1 = it->second.x1;
            inputRoi.x2 = it->second.x2;
            inputRoi.y1 = it->second.y1;
            inputRoi.y2 = it->second.y2;

            ///The RoI might be infinite if the getRoI action of the plug-in doesn't do anything and the input effect has an
            ///infinite rod.
            ifInfiniteclipRectToProjectDefault(&inputRoi);
            ret.insert(std::make_pair(inputNode,inputRoi));
        }
    } else if (stat == kOfxStatReplyDefault) {
        for (int i = 0; i < effectInstance()->getNClips(); ++i) {
            EffectInstance* inputNode = dynamic_cast<OfxClipInstance*>(effectInstance()->getNthClip(i))->getAssociatedNode();
            ret.insert(std::make_pair(inputNode, renderWindow));
        }
    }
    return ret;
}

Natron::EffectInstance::FramesNeededMap
OfxEffectInstance::getFramesNeeded(SequenceTime time)
{
    EffectInstance::FramesNeededMap ret;
    if (!_initialized) {
        return ret;
    }
    OFX::Host::ImageEffect::RangeMap inputRanges;
    assert(effect_);
    
    OfxStatus stat = effect_->getFrameNeededAction((OfxTime)time, inputRanges);
    if (stat != kOfxStatOK && stat != kOfxStatReplyDefault) {
        Natron::errorDialog(getName(), QObject::tr("Failed to specify the frame ranges needed from inputs.").toStdString());
    } else if (stat == kOfxStatOK) {
        for (OFX::Host::ImageEffect::RangeMap::iterator it = inputRanges.begin(); it!=inputRanges.end(); ++it) {
            int inputNb = dynamic_cast<OfxClipInstance*>(it->first)->getInputNb();
            if (inputNb != -1) {
                ret.insert(std::make_pair(inputNb,it->second));
            }
        }
    } else if (stat == kOfxStatReplyDefault) {
        return Natron::EffectInstance::getFramesNeeded(time);
    }
    return ret;
}

void
OfxEffectInstance::getFrameRange(SequenceTime *first,
                                 SequenceTime *last)
{
    if (!_initialized) {
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
    } else if (st == kOfxStatReplyDefault) {
        //The default is...
        int nthClip = effect_->getNClips();
        if (nthClip == 0) {
            //infinite if there are no non optional input clips.
            *first = INT_MIN;
            *last = INT_MAX;
        } else {
            //the union of all the frame ranges of the non optional input clips.
            bool firstValidInput = true;
            *first = INT_MIN;
            *last = INT_MAX;
            
            int inputsCount = maximumInputs();
            
            ///Uncommented the isOptional() introduces a bugs with Genarts Monster plug-ins when 2 generators
            ///are connected in the pipeline. They must rely on the time domain to maintain an internal state and apparantly
            ///not taking optional inputs into accounts messes it up.
            for (int i = 0; i< inputsCount; ++i) {
                
                //if (!isInputOptional(i)) {
                EffectInstance* inputEffect = input_other_thread(i);
                if (inputEffect) {
                    SequenceTime f,l;
                    inputEffect->getFrameRange_public(&f, &l);
                    if (!firstValidInput) {
                        if (f < *first && f != INT_MIN) {
                            *first = f;
                        }
                        if (l > *last && l != INT_MAX) {
                            *last = l;
                        }
                    } else {
                        firstValidInput = false;
                        *first = f;
                        *last = l;
                    }
                }
                // }
                
            }
        }
    }
    
}

bool
OfxEffectInstance::isIdentity(SequenceTime time,
                              const RenderScale& scale,
                              const RectI& roi,
                              int view,
                              SequenceTime* inputTime,
                              int* inputNb)
{
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
    OfxStatus stat ;
    
    {
        bool skipDiscarding = false;
        if (getRecursionLevel() > 1) {
            qDebug() << "isIdenity cannot be called recursively as an action. Please check this.";
            skipDiscarding = true;
        }
        ClipsThreadStorageSetter clipSetter(effectInstance(),
                                            skipDiscarding,
                                            true, //< setMipMapLevel ?
                                            mipmapLevel,
                                            false, //< setRenderedImage ?
                                            boost::shared_ptr<Image>(),
                                            true, //< setView ?
                                            view,
                                            false, //< setOutputRoD ?
                                            RectD(),
                                            false,
                                            0,0); //< setFrameRange ?
        
        stat = effect_->isIdentityAction(inputTimeOfx,field,ofxRoI,useScaleOne ? scaleOne : scale,inputclip);
    }

    if (stat == kOfxStatOK) {
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
    } else {
        return false;
    }
}


Natron::Status
OfxEffectInstance::beginSequenceRender(SequenceTime first,
                                       SequenceTime last,
                                       SequenceTime step,
                                       bool interactive,
                                       const RenderScale& scale,
                                       bool isSequentialRender,
                                       bool isRenderResponseToUserInteraction,
                                       int view)
{
    OfxPointD scaleOne;
    scaleOne.x = scaleOne.y = 1.;
    bool useScaleOne = !supportsRenderScale();
    
    unsigned int mipmapLevel = Image::getLevelFromScale(scale.x);
    
    OfxStatus stat ;
    
    {
        bool skipDiscarding = false;
        if (getRecursionLevel() > 1) {
            qDebug() << "beginRenderAction cannot be called recursively as an action. Please check this.";
            skipDiscarding = true;
        }
        ClipsThreadStorageSetter clipSetter(effectInstance(),
                                            skipDiscarding,
                                            true, //< setMipMapLevel ?
                                            mipmapLevel,
                                            false, //< setRenderedImage ?
                                            boost::shared_ptr<Image>(),
                                            true, //< setView ?
                                            view,
                                            false, //< setOutputRoD ?
                                            RectD(),
                                            false,
                                            0,0); //< setFrameRange ?
        
        stat = effectInstance()->beginRenderAction(first, last, step, interactive,useScaleOne ? scaleOne :  scale,isSequentialRender,isRenderResponseToUserInteraction,view);
    }

    if (stat != kOfxStatOK && stat != kOfxStatReplyDefault) {
        return StatFailed;
    }
    return StatOK;
}

Natron::Status
OfxEffectInstance::endSequenceRender(SequenceTime first,
                                     SequenceTime last,
                                     SequenceTime step,
                                     bool interactive,
                                     const RenderScale& scale,
                                     bool isSequentialRender,
                                     bool isRenderResponseToUserInteraction,
                                     int view)
{
    OfxPointD scaleOne;
    scaleOne.x = scaleOne.y = 1.;
    bool useScaleOne = !supportsRenderScale();
    
    unsigned int mipmapLevel = Image::getLevelFromScale(scale.x);
    
    OfxStatus stat ;
    
    {
        bool skipDiscarding = false;
        if (getRecursionLevel() > 1) {
            qDebug() << "endRenderAction cannot be called recursively as an action. Please check this.";
            skipDiscarding = true;
        }
        ClipsThreadStorageSetter clipSetter(effectInstance(),
                                            skipDiscarding,
                                            true, //< setMipMapLevel ?
                                            mipmapLevel,
                                            false, //< setRenderedImage ?
                                            boost::shared_ptr<Image>(),
                                            true, //< setView ?
                                            view,
                                            false, //< setOutputRoD ?
                                            RectD(),
                                            false,
                                            0,0); //< setFrameRange ?
        
        stat = effectInstance()->endRenderAction(first, last, step, interactive,useScaleOne ? scaleOne : scale,isSequentialRender,isRenderResponseToUserInteraction,view);
    }

    if (stat != kOfxStatOK && stat != kOfxStatReplyDefault) {
        return StatFailed;
    }
    return StatOK;
}

Natron::Status
OfxEffectInstance::render(SequenceTime time,
                          const RenderScale& scale,
                          const RectI& roi,
                          int view,
                          bool isSequentialRender,
                          bool isRenderResponseToUserInteraction,
                          boost::shared_ptr<Natron::Image> output)
{
    if (!_initialized) {
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
    ///This is passed to the render action to plug-ins that don't support render scale
    RenderScale scaleOne;
    scaleOne.x = scaleOne.y = 1.;
    
    bool useScaleOne = !supportsRenderScale();

    {
        bool skipDiscarding = false;
        if (getRecursionLevel() > 1) {
            qDebug() << "renderAction cannot be called recursively as an action. Please check this.";
            skipDiscarding = true;
        }
        ClipsThreadStorageSetter clipSetter(effectInstance(),
                                 skipDiscarding,
                                 true, //< setMipMapLevel ?
                                 mipMapLevel,
                                 true, //< setRenderedImage ?
                                 output,
                                 true, //< setView ?
                                 view,
                                 true, //< setOutputRoD ?
                                 output->getRoD(),
                                 false, //< setFrameRange ?
                                 0,0);
        
        stat = effect_->renderAction((OfxTime)time,
                                     field,
                                     ofxRoI,
                                     useScaleOne ? scaleOne : scale,
                                     isSequentialRender,
                                     isRenderResponseToUserInteraction,
                                     view,
                                     viewsCount);
    }

    if (stat != kOfxStatOK) {
        return StatFailed;
    } else {
        return StatOK;
    }
}

EffectInstance::RenderSafety
OfxEffectInstance::renderThreadSafety() const
{
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

bool
OfxEffectInstance::makePreviewByDefault() const
{
    return isGenerator();
}

const std::string&
OfxEffectInstance::getShortLabel() const
{
    return effectInstance()->getShortLabel();
}

void
OfxEffectInstance::initializeOverlayInteract()
{
    tryInitializeOverlayInteracts();
}


void
OfxEffectInstance::drawOverlay(double /*scaleX*/,
                               double /*scaleY*/,
                               const RectD& rod) //!< effect RoD in canonical coordinates
{
    if (!_initialized) {
        return;
    }
    if (_overlayInteract) {
        OfxPointD rs;
        rs.x = 1.;//scaleX;
        rs.y = 1.;//scaleY;
        OfxTime time = effect_->getFrameRecursive();

        if (getRecursionLevel() == 1) {
            int view = getCurrentViewRecursive();

            bool skipDiscarding = false;
            if (getRecursionLevel() > 1) {
                ///This happens sometimes because of dialogs popping from the request of a plug-in (inside an action)
                ///and making the mainwindow loose focus, hence forcing a new paint event
                //qDebug() << "drawAction cannot be called recursively as an action. Please check this.";
                skipDiscarding = true;
            }
            ClipsThreadStorageSetter clipSetter(effectInstance(),
                                     skipDiscarding,
                                     true, //< setMipMapLevel ?
                                     0,
                                     false, //< setRenderedImage ?
                                     boost::shared_ptr<Image>(),
                                     true, //< setView ?
                                     view,
                                     true, //< setOutputRoD ?
                                     rod,
                                     false,
                                     0,0); //< setFrameRange ?
            
            _overlayInteract->drawAction(time, rs);
        } else {
            _overlayInteract->drawAction(time, rs);
        }
        
    }
}

void
OfxEffectInstance::setCurrentViewportForOverlays(OverlaySupport* viewport)
{
    if (_overlayInteract) {
        _overlayInteract->setCallingViewport(viewport);
    }
}

bool
OfxEffectInstance::onOverlayPenDown(double /*scaleX*/,
                                    double /*scaleY*/,
                                    const QPointF& viewportPos,
                                    const QPointF& pos,
                                    const RectD& rod) //!< effect RoD in canonical coordinates
{
    if (!_initialized) {
        return false;
    }
    if (_overlayInteract) {
        OfxPointD rs;
        rs.x = 1.;//scaleX;
        rs.y = 1.;//scaleY;
        OfxPointD penPos;
        penPos.x = pos.x();
        penPos.y = pos.y();
        OfxPointI penPosViewport;
        penPosViewport.x = viewportPos.x();
        penPosViewport.y = viewportPos.y();
        
        OfxTime time = getCurrentFrameRecursive();
        
       int view = getCurrentViewRecursive();
        
        ClipsThreadStorageSetter clipSetter(effectInstance(),
                                 false,
                                 true, //< setMipMapLevel ?
                                 0,
                                 false, //< setRenderedImage ?
                                 boost::shared_ptr<Image>(),
                                 true, //< setView ?
                                 view,
                                 true, //< setOutputRoD ?
                                 rod,
                                 false,
                                 0,0); //< setFrameRange ?
        

        OfxStatus stat = _overlayInteract->penDownAction(time, rs, penPos, penPosViewport, 1.);
 
        
        if (stat == kOfxStatOK) {
            _penDown = true;
            return true;
        }
    }
    return false;
}

bool
OfxEffectInstance::onOverlayPenMotion(double /*scaleX*/,
                                      double /*scaleY*/,
                                      const QPointF& viewportPos,
                                      const QPointF& pos,
                                      const RectD& rod) //!< effect RoD in canonical coordinates
{
    if (!_initialized) {
        return false;
    }
    if (_overlayInteract) {
        OfxPointD rs;
        rs.x = 1.;//scaleX;
        rs.y = 1.;//scaleY;
        OfxPointD penPos;
        penPos.x = pos.x();
        penPos.y = pos.y();
        OfxPointI penPosViewport;
        penPosViewport.x = viewportPos.x();
        penPosViewport.y = viewportPos.y();
        OfxTime time = getCurrentFrameRecursive();
        OfxStatus stat;
        if (getRecursionLevel() == 1) {
            int view = getCurrentViewRecursive();

            ClipsThreadStorageSetter clipSetter(effectInstance(),
                                     false,
                                     true, //< setMipMapLevel ?
                                     0,
                                     false, //< setRenderedImage ?
                                     boost::shared_ptr<Image>(),
                                     true, //< setView ?
                                     view,
                                     true, //< setOutputRoD ?
                                     rod,
                                     false,
                                     0,0); //< setFrameRange ?
            stat = _overlayInteract->penMotionAction(time, rs, penPos, penPosViewport, 1.);

        } else {
            stat = _overlayInteract->penMotionAction(time, rs, penPos, penPosViewport, 1.);
        }

        if (stat == kOfxStatOK) {
            return true;
        }
    }
    return false;
}


bool
OfxEffectInstance::onOverlayPenUp(double /*scaleX*/,
                                  double /*scaleY*/,
                                  const QPointF& viewportPos,
                                  const QPointF& pos,
                                  const RectD& rod) //!< effect RoD in canonical coordinates
{
    if (!_initialized) {
        return false;
    }
    if (_overlayInteract) {
        OfxPointD rs;
        rs.x = 1.;//scaleX;
        rs.y = 1.;//scaleY;
        OfxPointD penPos;
        penPos.x = pos.x();
        penPos.y = pos.y();
        OfxPointI penPosViewport;
        penPosViewport.x = viewportPos.x();
        penPosViewport.y = viewportPos.y();
        OfxTime time = getCurrentFrameRecursive();
        
        int view = getCurrentViewRecursive();
        
        ClipsThreadStorageSetter clipSetter(effectInstance(),
                                 false,
                                 true, //< setMipMapLevel ?
                                 0,
                                 false, //< setRenderedImage ?
                                 boost::shared_ptr<Image>(),
                                 true, //< setView ?
                                 view,
                                 true, //< setOutputRoD ?
                                 rod,
                                 false,
                                 0,0); //< setFrameRange ?
        OfxStatus stat = _overlayInteract->penUpAction(time, rs, penPos, penPosViewport, 1.);
        
        if (stat == kOfxStatOK) {
            _penDown = false;
            return true;
        }
    }
    return false;
}

bool
OfxEffectInstance::onOverlayKeyDown(double /*scaleX*/,
                                    double /*scaleY*/,
                                    Natron::Key key,
                                    Natron::KeyboardModifiers /*modifiers*/,
                                    const RectD& rod) //!< effect RoD in canonical coordinates
{
    if (!_initialized) {
        return false;;
    }
    if (_overlayInteract) {
        OfxPointD rs;
        rs.x = 1.;//scaleX;
        rs.y = 1.;//scaleY;
        OfxTime time = getCurrentFrameRecursive();
        QByteArray keyStr;
        
        int view = getCurrentViewRecursive();
        ClipsThreadStorageSetter clipSetter(effectInstance(),
                                 false,
                                 true, //< setMipMapLevel ?
                                 0,
                                 false, //< setRenderedImage ?
                                 boost::shared_ptr<Image>(),
                                 true, //< setView ?
                                 view,
                                 true, //< setOutputRoD ?
                                 rod,
                                 false,
                                 0,0); //< setFrameRange ?
        OfxStatus stat = _overlayInteract->keyDownAction(time, rs, (int)key, keyStr.data());

        if (stat == kOfxStatOK) {
            return true;
        }
    }
    return false;
}

bool
OfxEffectInstance::onOverlayKeyUp(double /*scaleX*/,
                                  double /*scaleY*/,
                                  Natron::Key key,
                                  Natron::KeyboardModifiers /* modifiers*/,
                                  const RectD& rod) //!< effect RoD in canonical coordinates
{
    if (!_initialized) {
        return false;
    }
    if (_overlayInteract) {
        OfxPointD rs;
        rs.x = 1.;//scaleX;
        rs.y = 1.;//scaleY;
        OfxTime time = getCurrentFrameRecursive();
        QByteArray keyStr;
        
        int view = getCurrentViewRecursive();
        ClipsThreadStorageSetter clipSetter(effectInstance(),
                                 false,
                                 true, //< setMipMapLevel ?
                                 0,
                                 false, //< setRenderedImage ?
                                 boost::shared_ptr<Image>(),
                                 true, //< setView ?
                                 view,
                                 true, //< setOutputRoD ?
                                 rod,
                                 false,
                                 0,0); //< setFrameRange ?
        OfxStatus stat = _overlayInteract->keyUpAction(time, rs, (int)key, keyStr.data());
      
        assert(stat == kOfxStatOK || stat == kOfxStatReplyDefault);
        if (stat == kOfxStatOK) {
            return true;
        };
    }
    return false;
}

bool
OfxEffectInstance::onOverlayKeyRepeat(double /*scaleX*/,
                                      double /*scaleY*/,
                                      Natron::Key key,
                                      Natron::KeyboardModifiers /*modifiers*/,
                                      const RectD& rod) //!< effect RoD in canonical coordinates
{
    if (!_initialized) {
        return false;
    }
    if (_overlayInteract) {
        OfxPointD rs;
        rs.x = 1.;//scaleX;
        rs.y = 1.;//scaleY;
        OfxTime time = getCurrentFrameRecursive();
        QByteArray keyStr;
        
        int view = getCurrentViewRecursive();
        ClipsThreadStorageSetter clipSetter(effectInstance(),
                                 false,
                                 true, //< setMipMapLevel ?
                                 0,
                                 false, //< setRenderedImage ?
                                 boost::shared_ptr<Image>(),
                                 true, //< setView ?
                                 view,
                                 true, //< setOutputRoD ?
                                 rod,
                                 false,
                                 0,0); //< setFrameRange ?
        OfxStatus stat = _overlayInteract->keyRepeatAction(time, rs, (int)key, keyStr.data());
      
        if (stat == kOfxStatOK) {
            return true;
        }
    }
    return false;
}

bool
OfxEffectInstance::onOverlayFocusGained(double /*scaleX*/,
                                        double /*scaleY*/,
                                        const RectD& rod) //!< effect RoD in canonical coordinates
{
    if (!_initialized) {
        return false;
    }
    if (_overlayInteract) {
        OfxPointD rs;
        rs.x = 1.;//scaleX;
        rs.y = 1.;//scaleY;
        OfxTime time = getCurrentFrameRecursive();
        OfxStatus stat;
        
        if (getRecursionLevel() == 1) {
            int view = getCurrentViewRecursive();
            ClipsThreadStorageSetter clipSetter(effectInstance(),
                                     false,
                                     true, //< setMipMapLevel ?
                                     0,
                                     false, //< setRenderedImage ?
                                     boost::shared_ptr<Image>(),
                                     true, //< setView ?
                                     view,
                                     true, //< setOutputRoD ?
                                     rod,
                                     false,
                                     0,0); //< setFrameRange ?
            stat = _overlayInteract->gainFocusAction(time, rs);
        } else {
            stat = _overlayInteract->gainFocusAction(time, rs);
        }
        assert(stat == kOfxStatOK || stat == kOfxStatReplyDefault);
        if (stat == kOfxStatOK) {
            return true;
        }
    }
    return false;
}

bool
OfxEffectInstance::onOverlayFocusLost(double /*scaleX*/,
                                      double /*scaleY*/,
                                      const RectD& rod) //!< effect RoD in canonical coordinates
{
    if (!_initialized) {
        return false;
    }
    if (_overlayInteract) {
        OfxPointD rs;
        rs.x = 1.;//scaleX;
        rs.y = 1.;//scaleY;
        OfxTime time = getCurrentFrameRecursive();
        OfxStatus stat;
        if (getRecursionLevel() == 1) {
            int view = getCurrentViewRecursive();
            ClipsThreadStorageSetter clipSetter(effectInstance(),
                                     false,
                                     true, //< setMipMapLevel ?
                                     0,
                                     false, //< setRenderedImage ?
                                     boost::shared_ptr<Image>(),
                                     true, //< setView ?
                                     view,
                                     true, //< setOutputRoD ?
                                     rod,
                                     false,
                                     0,0); //< setFrameRange ?
            
            stat = _overlayInteract->loseFocusAction(time, rs);
        } else {
            stat = _overlayInteract->loseFocusAction(time, rs);
        }
        
        assert(stat == kOfxStatOK || stat == kOfxStatReplyDefault);
        if (stat == kOfxStatOK) {
            return true;
        }
    }
    return false;
}

bool
OfxEffectInstance::hasOverlay() const
{
    return _overlayInteract != NULL;
}

static std::string
natronValueChangedReasonToOfxValueChangedReason(Natron::ValueChangedReason reason)
{
    switch (reason) {
        case Natron::USER_EDITED:
            return kOfxChangeUserEdited;
        case Natron::PLUGIN_EDITED:
            return kOfxChangePluginEdited;
        case Natron::TIME_CHANGED:
            return kOfxChangeTime;
        default:
            assert(false); // all Natron reasons should be processed
            return "";
    }
}

void
OfxEffectInstance::knobChanged(KnobI* k,
                               Natron::ValueChangedReason reason,
                               const RectD& rod, //!< effect RoD in canonical coordinates
                               int view,
                               SequenceTime time)
{
    if (!_initialized) {
        return;
    }
    
    ///If the param changed is a button and the node is disabled don't do anything which might
    ///trigger an analysis
    if (reason == USER_EDITED && dynamic_cast<Button_Knob*>(k) && _node->isNodeDisabled()) {
        return;
    }
      
    if (_renderButton && k == _renderButton.get()) {
        
        ///don't do anything since it is handled upstream
        return;
    }
    
    if (reason == SLAVE_REFRESH || reason == RESTORE_DEFAULT) {
        reason = PLUGIN_EDITED;
    }
    std::string ofxReason = natronValueChangedReasonToOfxValueChangedReason(reason);
    assert(!ofxReason.empty()); // crashes when resetting to defaults
    OfxPointD renderScale;
    effect_->getRenderScaleRecursive(renderScale.x, renderScale.y);
    OfxStatus stat = kOfxStatOK;

    if (getRecursionLevel() == 1) {
        
        ClipsThreadStorageSetter clipSetter(effectInstance(),
                                 false,
                                 true, //< setMipMapLevel ?
                                 0,
                                 false, //< setRenderedImage ?
                                 boost::shared_ptr<Image>(),
                                 true, //< setView ?
                                 view,
                                 true, //< setOutputRoD ?
                                 rod,
                                 false,
                                 0,0); //< setFrameRange ?
        stat = effectInstance()->paramInstanceChangedAction(k->getName(), ofxReason,(OfxTime)time,renderScale);
    } else {
        stat = effectInstance()->paramInstanceChangedAction(k->getName(), ofxReason,(OfxTime)time,renderScale);
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
    if (_overlayInteract) {
        std::vector<std::string> params;
        _overlayInteract->getSlaveToParam(params);
        for (U32 i = 0; i < params.size(); ++i) {
            if (params[i] == k->getName()) {
                stat = _overlayInteract->redraw();
                assert(stat == kOfxStatOK || stat == kOfxStatReplyDefault);
            }
        }
    }


}

void
OfxEffectInstance::beginKnobsValuesChanged(Natron::ValueChangedReason reason)
{
    if (!_initialized) {
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

void
OfxEffectInstance::endKnobsValuesChanged(Natron::ValueChangedReason reason)
{
    if (!_initialized) {
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


void
OfxEffectInstance::purgeCaches()
{
    // The kOfxActionPurgeCaches is an action that may be passed to a plug-in instance from time to time in low memory situations. Instances recieving this action should destroy any data structures they may have and release the associated memory, they can later reconstruct this from the effect's parameter set and associated information. http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#kOfxActionPurgeCaches
    OfxStatus stat =  effect_->purgeCachesAction();
    assert(stat == kOfxStatOK || stat == kOfxStatReplyDefault);
    // The kOfxActionSyncPrivateData action is called when a plugin should synchronise any private data structures to its parameter set. This generally occurs when an effect is about to be saved or copied, but it could occur in other situations as well. http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#kOfxActionSyncPrivateData
    stat =  effect_->syncPrivateDataAction();
    assert(stat == kOfxStatOK || stat == kOfxStatReplyDefault);
}

int
OfxEffectInstance::majorVersion() const
{
    return effectInstance()->getPlugin()->getVersionMajor();
}

int
OfxEffectInstance::minorVersion() const
{
    return effectInstance()->getPlugin()->getVersionMinor();
}

bool
OfxEffectInstance::supportsTiles() const
{
    OFX::Host::ImageEffect::ClipInstance* outputClip =  effectInstance()->getClip(kOfxImageEffectOutputClipName);
    if (!outputClip) {
        return false;
    }
    return effectInstance()->supportsTiles() && outputClip->supportsTiles();
}

bool
OfxEffectInstance::supportsMultiResolution() const
{
    return effectInstance()->supportsMultiResolution();
}

bool
OfxEffectInstance::supportsRenderScale() const
{
    // Most readers don't support multiresolution, including Tuttle readers -
    // which crash on an assert in copy_and_convert_pixels( avSrcView, this->_dstView );
    return effect_->getContext() != kOfxImageEffectContextReader;}

void
OfxEffectInstance::beginEditKnobs()
{
    effectInstance()->beginInstanceEditAction();
}

void
OfxEffectInstance::onSyncPrivateDataRequested()
{
    ///Can only be called in the main thread
    assert(QThread::currentThread() == qApp->thread());
    effectInstance()->syncPrivateDataAction();
}



void
OfxEffectInstance::addAcceptedComponents(int inputNb,
                                         std::list<Natron::ImageComponents>* comps)
{
    if (inputNb >= 0) {
        OfxClipInstance* clip = getClipCorrespondingToInput(inputNb);
        assert(clip);
        const std::vector<std::string>& supportedComps = clip->getSupportedComponents();
        for (U32 i = 0; i < supportedComps.size(); ++i) {
            try {
                comps->push_back(OfxClipInstance::ofxComponentsToNatronComponents(supportedComps[i]));
            } catch (const std::runtime_error &e) {
                // ignore unsupported components
            }
        }
    } else {
        assert(inputNb == -1);
        OfxClipInstance* clip = dynamic_cast<OfxClipInstance*>(effectInstance()->getClip(kOfxImageEffectOutputClipName));
        assert(clip);
        const std::vector<std::string>& supportedComps = clip->getSupportedComponents();
        for (U32 i = 0; i < supportedComps.size(); ++i) {
            try {
                comps->push_back(OfxClipInstance::ofxComponentsToNatronComponents(supportedComps[i]));
            } catch (const std::runtime_error &e) {
                // ignore unsupported components
            }
        }
    }

}

void
OfxEffectInstance::addSupportedBitDepth(std::list<Natron::ImageBitDepth>* depths) const
{
    const OFX::Host::Property::Set& prop = effectInstance()->getPlugin()->getDescriptor().getParamSetProps();
    int dim = prop.getDimension(kOfxImageEffectPropSupportedPixelDepths);
    for (int i = 0; i < dim ; ++i) {
        const std::string& depth = prop.getStringProperty(kOfxImageEffectPropSupportedPixelDepths,i);
        try {
            depths->push_back(OfxClipInstance::ofxDepthToNatronDepth(depth));
        } catch (const std::runtime_error &e) {
            // ignore unsupported bitdepth
        }
    }
}

void
OfxEffectInstance::getPreferredDepthAndComponents(int inputNb,Natron::ImageComponents* comp,
                                                  Natron::ImageBitDepth* depth) const
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

Natron::SequentialPreference
OfxEffectInstance::getSequentialPreference() const
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

Natron::ImagePremultiplication
OfxEffectInstance::ofxPremultToNatronPremult(const std::string& str)
{
    if (str == kOfxImagePreMultiplied) {
        return Natron::ImagePremultiplied;
    } else if (str== kOfxImageUnPreMultiplied) {
        return Natron::ImageUnPremultiplied;
    } else {
        return Natron::ImageOpaque;
    }
}

Natron::ImagePremultiplication
OfxEffectInstance::getOutputPremultiplication() const
{
    return ofxPremultToNatronPremult(ofxGetOutputPremultiplication());
}

const std::string&
OfxEffectInstance::ofxGetOutputPremultiplication() const
{
    static const std::string v(kOfxImagePreMultiplied);
    
    OFX::Host::ImageEffect::ClipInstance* clip = effectInstance()->getClip(kOfxImageEffectOutputClipName);
    assert(clip);
    const std::string& premult = effectInstance()->getOutputPreMultiplication();
    ///if the output has something, use it, otherwise default to premultiplied
    if (!premult.empty()) {
        return premult;
    } else {
        return v;
    }
}
