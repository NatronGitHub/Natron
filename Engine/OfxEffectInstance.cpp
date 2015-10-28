/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
 *
 * Natron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Natron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 * ***** END LICENSE BLOCK ***** */

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "OfxEffectInstance.h"

#include <locale>
#include <limits>
#include <stdexcept>

#include <QtCore/QDebug>
#include <QByteArray>
#include <QReadWriteLock>
#include <QPointF>


#include "Global/Macros.h"

#include "Global/Macros.h"
// ofxhPropertySuite.h:565:37: warning: 'this' pointer cannot be null in well-defined C++ code; comparison may be assumed to always evaluate to true [-Wtautological-undefined-compare]
CLANG_DIAG_OFF(unknown-pragmas)
CLANG_DIAG_OFF(tautological-undefined-compare) // appeared in clang 3.5
#include <ofxhPluginCache.h>
CLANG_DIAG_ON(tautological-undefined-compare)
CLANG_DIAG_ON(unknown-pragmas)
#include <ofxhPluginAPICache.h>
CLANG_DIAG_OFF(unknown-pragmas)
CLANG_DIAG_OFF(tautological-undefined-compare)
#include <ofxhImageEffect.h>
CLANG_DIAG_ON(tautological-undefined-compare)
CLANG_DIAG_ON(unknown-pragmas)
#include <ofxhImageEffectAPI.h>
#include <ofxOpenGLRender.h>
#include <ofxhHost.h>

#include <tuttle/ofxReadWrite.h>
#include "ofxNatron.h"
#include <nuke/fnOfxExtensions.h>

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/KnobFile.h"
#include "Engine/KnobTypes.h"
#include "Engine/Node.h"
#include "Engine/NodeSerialization.h"
#include "Engine/OfxClipInstance.h"
#include "Engine/OfxImageEffectInstance.h"
#include "Engine/OfxOverlayInteract.h"
#include "Engine/OfxParamInstance.h"
#include "Engine/Project.h"
#include "Engine/RotoLayer.h"
#include "Engine/TimeLine.h"
#include "Engine/Transform.h"
#include "Engine/ViewerInstance.h"

#define READER_INPUT_NAME "Sync"

using namespace Natron;
using std::cout; using std::endl;

namespace  {
/**
 * @class This class is helpful to set thread-storage data on the clips of an effect
 * When destroyed, it is removed from the clips, ensuring they are removed.
 * It is to be instantiated right before calling the action that will need the per thread-storage
 * This way even if exceptions are thrown, clip thread-storage will be purged.
 *
 * All the info set on clip thread-storage are "cached" data that might be needed by a call of the OpenFX API which would
 * otherwise require a recursive action call, which is forbidden by the specification.
 * The more you pass parameters, the safer you are that the plug-in will not attempt recursive action calls but the more expensive
 * it is.
 **/
class ClipsThreadStorageSetter
{
public:
    ClipsThreadStorageSetter(OfxImageEffectInstance* effect,
                             bool skipDiscarding,     //< this is in case a recursive action is called
                             bool setView,
                             int view,
                             bool setMipmapLevel,
                             unsigned int mipMapLevel)
    : effect(effect)
    , skipDiscarding(skipDiscarding)
    , viewSet(setView)
    , mipMapLevelSet(setMipmapLevel)
    {

        if (setView) {
            effect->setClipsView(view);
        }
        if (setMipmapLevel) {
            effect->setClipsMipMapLevel(mipMapLevel);
        }
    }

    virtual ~ClipsThreadStorageSetter()
    {
        if (!skipDiscarding) {

            if (viewSet) {
                effect->discardClipsView();
            }
            if (mipMapLevelSet) {
                effect->discardClipsMipMapLevel();
            }
            
        }
    }

protected:
    OfxImageEffectInstance* effect;
    bool skipDiscarding;
    bool viewSet;
    bool mipMapLevelSet;
};
    
class RenderThreadStorageSetter : public ClipsThreadStorageSetter {
public:
    
    RenderThreadStorageSetter(OfxEffectInstance* effect,
                              bool skipDiscarding,     //< this is in case a recursive action is called
                              bool setView,
                              int view,
                              bool setMipmapLevel,
                              unsigned int mipMapLevel,
                              bool setPlane,
                              const Natron::ImageComponents& currentPlane,
                              const EffectInstance::InputImagesMap& inputImages)
    : ClipsThreadStorageSetter(effect->effectInstance(),skipDiscarding,setView, view, setMipmapLevel, mipMapLevel)
    , planeSet(setPlane)
    {
        OfxImageEffectInstance* instance = effect->effectInstance();
        
        if (setPlane) {
            instance->setClipsPlaneBeingRendered(currentPlane);
            for (EffectInstance::InputImagesMap::const_iterator it = inputImages.begin(); it != inputImages.end(); ++it) {
                if (!it->second.empty()) {
                    const ImagePtr& img = it->second.front();
                    assert(img);
                    instance->setInputClipPlane(it->first, true, img->getComponents());
                } else {
                    instance->setInputClipPlane(it->first, false, ImageComponents::getNoneComponents());
                }
            }
        }
        
    }
    
    virtual ~RenderThreadStorageSetter() {
        if (planeSet) {
            effect->discardClipsPlaneBeingRendered();
        }
        if (!skipDiscarding) {
            //Make sure that the images being rendered TLS is being cleared otherwise it will crash
            OFX::Host::ImageEffect::ClipInstance* ofxClip  = effect->getClip(kOfxImageEffectOutputClipName);
            assert(ofxClip);
            OfxClipInstance* clip = dynamic_cast<OfxClipInstance*>(ofxClip);
            assert(clip);
            clip->clearOfxImagesTLS();
        }
    }
    
private:
    bool planeSet;
};
}

OfxEffectInstance::OfxEffectInstance(boost::shared_ptr<Natron::Node> node)
: AbstractOfxEffectInstance(node)
, _effect()
, _natronPluginID()
, _isOutput(false)
, _penDown(false)
, _overlayInteract(0)
, _overlaySlaves()
, _created(false)
, _initialized(false)
, _renderButton()
, _renderSafety(Natron::eRenderSafetyUnsafe)
, _wasRenderSafetySet(false)
, _renderSafetyLock(new QReadWriteLock)
, _context(eContextNone)
, _preferencesLock(new QReadWriteLock(QReadWriteLock::Recursive))
#ifdef DEBUG
, _canSetValue()
#endif
, _nbSourceClips(0)
, _clipsInfos()
, _outputClip(0)
{
    QObject::connect( this, SIGNAL( syncPrivateDataRequested() ), this, SLOT( onSyncPrivateDataRequested() ) );
}

void
OfxEffectInstance::createOfxImageEffectInstance(OFX::Host::ImageEffect::ImageEffectPlugin* plugin,
                                                OFX::Host::ImageEffect::Descriptor* desc,
                                                Natron::ContextEnum context,
                                                const NodeSerialization* serialization,
                                                 const std::list<boost::shared_ptr<KnobSerialization> >& paramValues,
                                                bool allowFileDialogs,
                                                bool disableRenderScaleSupport,
                                                bool *hasUsedFileDialog)
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
    assert( QThread::currentThread() == qApp->thread() );
    assert(plugin && desc && context != eContextNone);
    
    
    *hasUsedFileDialog = false;
    
    _context = context;

    
    if (disableRenderScaleSupport || context == eContextWriter) {
        setAsOutputNode();
        // Writers don't support render scale (full-resolution images are written to disk)
        setSupportsRenderScaleMaybe(eSupportsNo);
    }
    
    
    if (context == eContextReader) {
        // Tuttle readers don't support render scale as of 11/8/2014, but may crash (at least in debug configuration).
        // TuttleAVReader crashes on an assert in copy_and_convert_pixels( avSrcView, this->_dstView );
        std::string prefix("tuttle.");
        if ( !plugin->getIdentifier().compare(0, prefix.size(), prefix) ) {
            setSupportsRenderScaleMaybe(eSupportsNo);
        }
    }

    std::string images;

    try {
        _effect = new Natron::OfxImageEffectInstance(plugin,*desc,mapContextToString(context),false);
        assert(_effect);
        _effect->setOfxEffectInstance( dynamic_cast<OfxEffectInstance*>(this) );

        _natronPluginID = plugin->getIdentifier();
        
        OfxEffectInstance::MappedInputV clips = inputClipsCopyWithoutOutput();
        _nbSourceClips = (int)clips.size();
        
        _clipsInfos.resize(clips.size());
        for (int i = 0; i < (int)clips.size(); ++i) {
            ClipsInfo info;
            info.rotoBrush = clips[i]->getName() == CLIP_OFX_ROTO && getNode()->isRotoNode();
            info.optional = clips[i]->isOptional() || info.rotoBrush;
            info.mask = clips[i]->isMask();
            info.clip = NULL;
            _clipsInfos[i] = info;
        }
        
        

        beginChanges();
        OfxStatus stat;
        {
            SET_CAN_SET_VALUE(true);
            
            stat = _effect->populate();
            
            
            for (int i = 0; i < (int)clips.size(); ++i) {
                _clipsInfos[i].clip = dynamic_cast<OfxClipInstance*>(_effect->getClip(clips[i]->getName()));
                assert(_clipsInfos[i].clip);
            }
            
            _outputClip = dynamic_cast<OfxClipInstance*>(_effect->getClip(kOfxImageEffectOutputClipName));
            assert(_outputClip);
            
            initializeContextDependentParams();
            
            _effect->addParamsToTheirParents();
            
            if (stat != kOfxStatOK) {
                throw std::runtime_error("Error while populating the Ofx image effect");
            }
            assert( _effect->getPlugin() );
            assert( _effect->getPlugin()->getPluginHandle() );
            assert( _effect->getPlugin()->getPluginHandle()->getOfxPlugin() );
            assert(_effect->getPlugin()->getPluginHandle()->getOfxPlugin()->mainEntry);
            
            getNode()->createRotoContextConditionnally();
            
            getNode()->initializeInputs();
            getNode()->initializeKnobs(disableRenderScaleSupport ? 1 : 0 );
            
            ///before calling the createInstanceAction, load values
            if ( serialization && !serialization->isNull() ) {
                getNode()->loadKnobs(*serialization);
            }
            
            if (!paramValues.empty()) {
                getNode()->setValuesFromSerialization(paramValues);
            }
            
            //////////////////////////////////////////////////////
            ///////For READERS & WRITERS only we open an image file dialog
            if (!getApp()->isCreatingPythonGroup() && allowFileDialogs && isReader() && !(serialization && !serialization->isNull()) && paramValues.empty()) {
                images = getApp()->openImageFileDialog();
            } else if (!getApp()->isCreatingPythonGroup() && allowFileDialogs && isWriter() && !(serialization && !serialization->isNull())  && paramValues.empty()) {
                images = getApp()->saveImageFileDialog();
            }
            if (!images.empty()) {
                *hasUsedFileDialog = true;
                boost::shared_ptr<KnobSerialization> defaultFile = createDefaultValueForParam(kOfxImageEffectFileParamName, images);
                CreateNodeArgs::DefaultValuesList list;
                list.push_back(defaultFile);
                getNode()->setValuesFromSerialization(list);
            }
            //////////////////////////////////////////////////////
            
            
            {
                ///Take the preferences lock so that it cannot be modified throughout the action.
                QReadLocker preferencesLocker(_preferencesLock);
                stat = _effect->createInstanceAction();
            }
            _created = true;
            
            
        } // SET_CAN_SET_VALUE(true);
        
        
        if ( (stat != kOfxStatOK) && (stat != kOfxStatReplyDefault) ) {
            throw std::runtime_error("Could not create effect instance for plugin");
        }
        
        OfxPointD scaleOne;
        scaleOne.x = 1.;
        scaleOne.y = 1.;
        // Try to set renderscale support at plugin creation.
        // This is not always possible (e.g. if a param has a wrong value).
        if (supportsRenderScaleMaybe() == eSupportsMaybe) {
            // does the effect support renderscale?
            double first = INT_MIN,last = INT_MAX;
            getFrameRange(&first, &last);
            if (first == INT_MIN || last == INT_MAX) {
                first = last = getApp()->getTimeLine()->currentFrame();
            }
            ClipsThreadStorageSetter clipSetter(effectInstance(),
                                                false,
                                                true, //< setView ?
                                                0,
                                                true,
                                                0);
            double time = first;
            
            OfxRectD rod;
            OfxStatus rodstat = _effect->getRegionOfDefinitionAction(time, scaleOne, 0, rod);
            if ( (rodstat == kOfxStatOK) || (rodstat == kOfxStatReplyDefault) ) {
                OfxPointD scale;
                scale.x = 0.5;
                scale.y = 0.5;
                rodstat = _effect->getRegionOfDefinitionAction(time, scale, 0, rod);
                if ( (rodstat == kOfxStatOK) || (rodstat == kOfxStatReplyDefault) ) {
                    setSupportsRenderScaleMaybe(eSupportsYes);
                } else {
                    setSupportsRenderScaleMaybe(eSupportsNo);
                }
            }
            
        }
        
        
        
        // Check here that bitdepth and components given by getClipPreferences are supported by the effect.
        // If we don't, the following assert will crash at the beginning of EffectInstance::renderRoIInternal():
        // assert(isSupportedBitDepth(outputDepth) && isSupportedComponent(-1, outputComponents));
        // If a component/bitdepth is not supported (this is probably a plugin bug), use the closest one, but don't crash Natron.
        //checkOFXClipPreferences_public(getApp()->getTimeLine()->currentFrame(), scaleOne, kOfxChangeUserEdited,true, false);
        

    } catch (const std::exception & e) {
        qDebug() << "Error: Caught exception while creating OfxImageEffectInstance" << ": " << e.what();
        throw;
    } catch (...) {
        qDebug() << "Error: Caught exception while creating OfxImageEffectInstance";
        throw;
    }

    _initialized = true;
    
  
    endChanges();
    
} // createOfxImageEffectInstance

OfxEffectInstance::~OfxEffectInstance()
{
    delete _overlayInteract;
    delete _effect;
    delete _renderSafetyLock;
    delete _preferencesLock;
}

bool
OfxEffectInstance::isEffectCreated() const
{
    return _created;
}

void
OfxEffectInstance::initializeContextDependentParams()
{
    assert(_context != eContextNone);
    if ( isWriter() ) {
        _renderButton = Natron::createKnob<KnobButton>(this, "Render");
        _renderButton->setHintToolTip("Starts rendering the specified frame range.");
        _renderButton->setAsRenderButton();
    }
}

std::string
OfxEffectInstance::getDescription() const
{
    assert(_context != eContextNone);
    if ( effectInstance() ) {
        return effectInstance()->getProps().getStringProperty(kOfxPropPluginDescription);
    } else {
        return "";
    }
}

void
OfxEffectInstance::tryInitializeOverlayInteracts()
{
    assert(_context != eContextNone);
    if (_overlayInteract) {
        // already created
        return;
    }
    /*create overlay instance if any*/
    OfxPluginEntryPoint *overlayEntryPoint = _effect->getOverlayInteractMainEntry();
    if (overlayEntryPoint) {
        _overlayInteract = new OfxOverlayInteract(*_effect,8,true);
        RenderScale s;
        effectInstance()->getRenderScaleRecursive(s.x, s.y);

        {
            ClipsThreadStorageSetter clipSetter(effectInstance(),
                                                false,
                                                true, //< setView ?
                                                0,
                                                true,
                                                0);


            {
                SET_CAN_SET_VALUE(true);
                ///Take the preferences lock so that it cannot be modified throughout the action.
                QReadLocker preferencesLocker(_preferencesLock);
                _overlayInteract->createInstanceAction();
            }
        }
        
        ///Fetch all parameters that are overlay slave
        std::vector<std::string> slaveParams;
        _overlayInteract->getSlaveToParam(slaveParams);
        for (U32 i = 0; i < slaveParams.size(); ++i) {
            boost::shared_ptr<KnobI> param ;
            const std::vector< boost::shared_ptr<KnobI> > & knobs = getKnobs();
            for (std::vector< boost::shared_ptr<KnobI> >::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
                if ((*it)->getOriginalName() == slaveParams[i]) {
                    param = *it;
                    break;
                    
                }
            }
            if (!param) {
                qDebug() << "OfxEffectInstance::tryInitializeOverlayInteracts(): slaveToParam " << slaveParams[i].c_str() << " not available";
            } else {
                _overlaySlaves.push_back((void*)param.get());
            }
        }
        
        //For multi-instances, redraw is already taken care of by the GUI
        if (!getNode()->getParentMultiInstance()) {
            getApp()->redrawAllViewers();
        }
    }
    
    
    
    ///for each param, if it has a valid custom interact, create it
    const std::list<OFX::Host::Param::Instance*> & params = effectInstance()->getParamList();
    for (std::list<OFX::Host::Param::Instance*>::const_iterator it = params.begin(); it != params.end(); ++it) {
        OfxParamToKnob* paramToKnob = dynamic_cast<OfxParamToKnob*>(*it);
        assert(paramToKnob);
        OFX::Host::Interact::Descriptor & interactDesc = paramToKnob->getInteractDesc();
        if (interactDesc.getState() == OFX::Host::Interact::eDescribed) {
            boost::shared_ptr<KnobI> knob = paramToKnob->getKnob();
            boost::shared_ptr<OfxParamOverlayInteract> overlay( new OfxParamOverlayInteract( knob.get(),interactDesc,
                                                                                             effectInstance()->getHandle() ) );

            {
                SET_CAN_SET_VALUE(true);
                ///Take the preferences lock so that it cannot be modified throughout the action.
                QReadLocker preferencesLocker(_preferencesLock);
                overlay->createInstanceAction();
            }
            knob->setCustomInteract(overlay);
        }
    }
}

bool
OfxEffectInstance::isOutput() const
{
    assert(_context != eContextNone);

    return _isOutput;
}

bool
OfxEffectInstance::isGenerator() const
{
#if 1
    /*
     * This is to deal with effects that can be both filters and generators (e.g: like constant or S_Zap)
     * Some plug-ins unfortunately do not behave exactly the same in these 2 contexts and we want them to behave
     * as a general context. So we just look for the presence of the generator context to determine if the plug-in
     * is really a generator or not.
     */
    
    assert( effectInstance() );
    const std::set<std::string> & contexts = effectInstance()->getPlugin()->getContexts();
    std::set<std::string>::const_iterator foundGenerator = contexts.find(kOfxImageEffectContextGenerator);
    std::set<std::string>::const_iterator foundReader = contexts.find(kOfxImageEffectContextReader);
    if ( ( foundGenerator != contexts.end() ) || ( foundReader != contexts.end() ) ) {
        return true;
    }

    return false;
#else
    assert(_context != eContextNone);

    return _context == eContextGenerator || _context == eContextReader;
#endif
}

bool
OfxEffectInstance::isReader() const
{
#if 0
    assert( effectInstance() );
    const std::set<std::string> & contexts = effectInstance()->getPlugin()->getContexts();
    std::set<std::string>::const_iterator foundReader = contexts.find(kOfxImageEffectContextReader);
    if ( foundReader != contexts.end() ) {
        return true;
    }

    return false;
#else
    assert(_context != eContextNone);

    return _context == eContextReader;
#endif
}

bool
OfxEffectInstance::isWriter() const
{
#if 0
    assert(_context != eContextNone);
    assert( effectInstance() );
    const std::set<std::string> & contexts = effectInstance()->getPlugin()->getContexts();
    std::set<std::string>::const_iterator foundWriter = contexts.find(kOfxImageEffectContextWriter);
    if ( foundWriter != contexts.end() ) {
        return true;
    }

    return false;
#else
    assert(_context != eContextNone);

    return _context == eContextWriter;
#endif
}

bool
OfxEffectInstance::isTrackerNode() const
{
    assert(_context != eContextNone);
    return _context == eContextTracker;
}

bool
OfxEffectInstance::isGeneratorAndFilter() const
{
    assert(_context != eContextNone);
    const std::set<std::string> & contexts = effectInstance()->getPlugin()->getContexts();
    std::set<std::string>::const_iterator foundGenerator = contexts.find(kOfxImageEffectContextGenerator);
    std::set<std::string>::const_iterator foundGeneral = contexts.find(kOfxImageEffectContextGeneral);

    return foundGenerator != contexts.end() && foundGeneral != contexts.end();
}

/*group is a string as such:
   Toto/Superplugins/blabla
   This functions extracts the all parts of such a grouping, e.g in this case
   it would return [Toto,Superplugins,blabla].*/
static
QStringList
ofxExtractAllPartsOfGrouping(const QString & pluginIdentifier,
                             int /*versionMajor*/,
                             int /*versionMinor*/,
                             const QString & /*pluginLabel*/,
                             const QString & str)
{
    QString s(str);

    s.replace( QChar('\\'),QChar('/') );

    QStringList out;
    if ((pluginIdentifier.startsWith("com.genarts.sapphire.") || s.startsWith("Sapphire ") || s.startsWith(" Sapphire ")) &&
        !s.startsWith("Sapphire/")) {
        out.push_back("Sapphire");

    } else if ((pluginIdentifier.startsWith("com.genarts.monsters.") || s.startsWith("Monsters ") || s.startsWith(" Monsters ")) &&
               !s.startsWith("Monsters/")) {
        out.push_back("Monsters");

    } else if ((pluginIdentifier == "uk.co.thefoundry.keylight.keylight") ||
               (pluginIdentifier == "jp.co.ise.imagica:PrimattePlugin")) {
        s = PLUGIN_GROUP_KEYER;

    } else if ((pluginIdentifier == "uk.co.thefoundry.noisetools.denoise") ||
               pluginIdentifier.startsWith("com.rubbermonkey:FilmConvert")) {
        s = PLUGIN_GROUP_FILTER;

    } else if (pluginIdentifier.startsWith("com.NewBlue.Titler")) {
        s = PLUGIN_GROUP_PAINT;

    } else if (pluginIdentifier.startsWith("com.FXHOME.HitFilm")) {
        // HitFilm uses grouping such as "HitFilm - Generate"
        s.replace("HitFilm - ", "HitFilm/");

    } else if (pluginIdentifier.startsWith("com.redgiantsoftware.Universe")) {
        // Red Giant Universe uses grouping such as "Universe Blur"
        s.replace("Universe ", "Universe/");

    } else if ( (pluginIdentifier == "tuttle.avreader") ||
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
               (pluginIdentifier == "tuttle.turbojpegwriter") ) {
        out.push_back(PLUGIN_GROUP_IMAGE);
        if ( pluginIdentifier.endsWith("reader") ) {
            s = PLUGIN_GROUP_IMAGE_READERS;
        } else {
            s = PLUGIN_GROUP_IMAGE_WRITERS;
        }

    } else if ( (pluginIdentifier == "tuttle.checkerboard") ||
               (pluginIdentifier == "tuttle.colorbars") ||
               (pluginIdentifier == "tuttle.colorcube") || // TuttleColorCube
               (pluginIdentifier == "tuttle.colorgradient") ||
               (pluginIdentifier == "tuttle.colorwheel") ||
               (pluginIdentifier == "tuttle.constant") ||
               (pluginIdentifier == "tuttle.inputbuffer") ||
               (pluginIdentifier == "tuttle.outputbuffer") ||
               (pluginIdentifier == "tuttle.ramp") ||
               (pluginIdentifier == "tuttle.seexpr") ) {
        s = PLUGIN_GROUP_IMAGE;

    } else if ( (pluginIdentifier == "tuttle.bitdepth") ||
               (pluginIdentifier == "tuttle.colorgradation") ||
               (pluginIdentifier == "tuttle.colorspace") ||
               (pluginIdentifier == "tuttle.colorsuppress") ||
               (pluginIdentifier == "tuttle.colortransfer") ||
               (pluginIdentifier == "tuttle.colortransform") ||
               (pluginIdentifier == "tuttle.ctl") ||
               (pluginIdentifier == "tuttle.invert") ||
               (pluginIdentifier == "tuttle.lut") ||
               (pluginIdentifier == "tuttle.normalize") ) {
        s = PLUGIN_GROUP_COLOR;

    } else if ( (pluginIdentifier == "tuttle.ocio.colorspace") ||
               (pluginIdentifier == "tuttle.ocio.lut") ) {
        out.push_back(PLUGIN_GROUP_COLOR);
        s = "OCIO";

    } else if ( (pluginIdentifier == "tuttle.gamma") ||
               (pluginIdentifier == "tuttle.mathoperator") ) {
        out.push_back(PLUGIN_GROUP_COLOR);
        s = "Math";

    } else if ( (pluginIdentifier == "tuttle.channelshuffle") ) {
        s = PLUGIN_GROUP_CHANNEL;

    } else if ( (pluginIdentifier == "tuttle.component") ||
               (pluginIdentifier == "tuttle.fade") ||
               (pluginIdentifier == "tuttle.merge") ) {
        s = PLUGIN_GROUP_MERGE;

    } else if ( (pluginIdentifier == "tuttle.anisotropicdiffusion") ||
                (pluginIdentifier == "tuttle.anisotropictensors") ||
                (pluginIdentifier == "tuttle.blur") ||
                (pluginIdentifier == "tuttle.convolution") ||
                (pluginIdentifier == "tuttle.floodfill") ||
                (pluginIdentifier == "tuttle.localmaxima") ||
                (pluginIdentifier == "tuttle.nlmdenoiser") ||
                (pluginIdentifier == "tuttle.sobel") ||
                (pluginIdentifier == "tuttle.thinning") ) {
        s = PLUGIN_GROUP_FILTER;

    } else if ( (pluginIdentifier == "tuttle.crop") ||
               (pluginIdentifier == "tuttle.flip") ||
               (pluginIdentifier == "tuttle.lensdistort") ||
               (pluginIdentifier == "tuttle.move2d") ||
               (pluginIdentifier == "tuttle.pinning") ||
               (pluginIdentifier == "tuttle.pushpixel") ||
               (pluginIdentifier == "tuttle.resize") ||
               (pluginIdentifier == "tuttle.swscale") ||
               (pluginIdentifier == "tuttle.warp") ) {
        s = PLUGIN_GROUP_TRANSFORM;

    } else if ( (pluginIdentifier == "tuttle.timeshift") ) {
        s = PLUGIN_GROUP_TIME;

    } else if ( (pluginIdentifier == "tuttle.text") ) {
        s = PLUGIN_GROUP_PAINT;

    } else if ( (pluginIdentifier == "tuttle.basickeyer") ||
                (pluginIdentifier == "tuttle.colorspacekeyer") ||
                (pluginIdentifier == "tuttle.histogramkeyer") ||
                (pluginIdentifier == "tuttle.idkeyer") ) {
        s = PLUGIN_GROUP_KEYER;

    } else if ( (pluginIdentifier == "tuttle.colorCube") || // TuttleColorCubeViewer
               (pluginIdentifier == "tuttle.colorcubeviewer") ||
               (pluginIdentifier == "tuttle.diff") ||
               (pluginIdentifier == "tuttle.dummy") ||
               (pluginIdentifier == "tuttle.histogram") ||
               (pluginIdentifier == "tuttle.imagestatistics") ) {
        s = PLUGIN_GROUP_OTHER;
        
    } else if ( (pluginIdentifier == "tuttle.debugimageeffectapi") ) {
        out.push_back(PLUGIN_GROUP_OTHER);
        s = "Test";
    }

    // The following plugins are pretty much useless for use within Natron, keep them in the Tuttle group:
    /*
       (pluginIdentifier == "tuttle.print") ||
       (pluginIdentifier == "tuttle.viewer") ||
     */
    return out + s.split('/');
} // ofxExtractAllPartsOfGrouping

QStringList
AbstractOfxEffectInstance::makePluginGrouping(const std::string & pluginIdentifier,
                                              int versionMajor,
                                              int versionMinor,
                                              const std::string & pluginLabel,
                                              const std::string & grouping)
{
    //printf("%s,%s\n",pluginLabel.c_str(),grouping.c_str());
    return ofxExtractAllPartsOfGrouping( pluginIdentifier.c_str(), versionMajor, versionMinor, pluginLabel.c_str(),grouping.c_str() );
}

std::string
AbstractOfxEffectInstance::makePluginLabel(const std::string & shortLabel,
                                           const std::string & label,
                                           const std::string & longLabel)
{
    std::string labelToUse = label;

    if ( labelToUse.empty() ) {
        labelToUse = shortLabel;
    }
    if ( labelToUse.empty() ) {
        labelToUse = longLabel;
    }

    return labelToUse;
}


std::string
OfxEffectInstance::getPluginID() const
{
    assert(_context != eContextNone);

    return _natronPluginID;
}

std::string
OfxEffectInstance::getPluginLabel() const
{
    assert(_context != eContextNone);
    assert(_effect);

    return makePluginLabel( _effect->getDescriptor().getShortLabel(),
                            _effect->getDescriptor().getLabel(),
                            _effect->getDescriptor().getLongLabel() );
}

void
OfxEffectInstance::getPluginGrouping(std::list<std::string>* grouping) const
{
    assert(_context != eContextNone);
    std::string groupStr = effectInstance()->getPluginGrouping();
    std::string label = getPluginLabel();
    const OFX::Host::ImageEffect::ImageEffectPlugin *p = effectInstance()->getPlugin();
    QStringList groups = ofxExtractAllPartsOfGrouping( p->getIdentifier().c_str(), p->getVersionMajor(), p->getVersionMinor(), label.c_str(), groupStr.c_str() );
    for (int i = 0; i < groups.size(); ++i) {
        grouping->push_back( groups[i].toStdString() );
    }
}

std::string
OfxEffectInstance::getInputLabel(int inputNb) const
{
    assert(_context != eContextNone);
    assert(inputNb >= 0 &&  inputNb < (int)_clipsInfos.size());
    if (_context != eContextReader) {
        return _clipsInfos[inputNb].clip->getShortLabel();
    } else {
        return READER_INPUT_NAME;
    }
}

OfxEffectInstance::MappedInputV
OfxEffectInstance::inputClipsCopyWithoutOutput() const
{
    assert(_context != eContextNone);
    assert( effectInstance() );
    const std::vector<OFX::Host::ImageEffect::ClipDescriptor*> & clips = effectInstance()->getDescriptor().getClipsByOrder();
    MappedInputV copy;
    for (U32 i = 0; i < clips.size(); ++i) {
        assert(clips[i]);
        if (!clips[i]->isOutput()) {
            copy.push_back(clips[i]);
        }
    }

    return copy;
}

OfxClipInstance*
OfxEffectInstance::getClipCorrespondingToInput(int inputNo) const
{
    assert(_context != eContextNone);
    assert( inputNo < (int)_clipsInfos.size() );
    return _clipsInfos[inputNo].clip;
}

int
OfxEffectInstance::getMaxInputCount() const
{
    assert(_context != eContextNone);
    //const std::string & context = effectInstance()->getContext();
    return _nbSourceClips;
}

bool
OfxEffectInstance::isInputOptional(int inputNb) const
{
    assert(_context != eContextNone);
    assert(inputNb >= 0 && inputNb < (int)_clipsInfos.size());
    return _clipsInfos[inputNb].optional;
}

bool
OfxEffectInstance::isInputMask(int inputNb) const
{
    assert(_context != eContextNone);
    assert(inputNb >= 0 && inputNb < (int)_clipsInfos.size());
    return _clipsInfos[inputNb].mask;
}

bool
OfxEffectInstance::isInputRotoBrush(int inputNb) const
{
    assert(_context != eContextNone);
    assert(inputNb >= 0 && inputNb < (int)_clipsInfos.size());
    return _clipsInfos[inputNb].rotoBrush;
}

int
OfxEffectInstance::getRotoBrushInputIndex() const
{
    assert(_context != eContextNone);
    for (std::size_t i = 0; i < _clipsInfos.size(); ++i) {
        if (_clipsInfos[i].rotoBrush) {
            return (int)i;
        }
    }
    return -1;
}

void
OfxEffectInstance::onInputChanged(int inputNo)
{
    
    
    assert(_context != eContextNone);
    OfxClipInstance* clip = getClipCorrespondingToInput(inputNo);
    assert(clip);
    double time = getApp()->getTimeLine()->currentFrame();
    RenderScale s;
    s.x = s.y = 1.;
    
    
    EffectPointerThreadProperty_RAII propHolder_raii(this);
   
    {
        RECURSIVE_ACTION();
        SET_CAN_SET_VALUE(true);
        ClipsThreadStorageSetter clipSetter(effectInstance(),
                                            false,
                                            true, //< setView ?
                                            0 /*view*/,
                                            true, //< setmipmaplevel?
                                            0);

        _effect->beginInstanceChangedAction(kOfxChangeUserEdited);
        _effect->clipInstanceChangedAction(clip->getName(), kOfxChangeUserEdited, time, s);
        _effect->endInstanceChangedAction(kOfxChangeUserEdited);
    }

}

/** @brief map a std::string to a context */
Natron::ContextEnum
OfxEffectInstance::mapToContextEnum(const std::string &s)
{
    if (s == kOfxImageEffectContextGenerator) {
        return eContextGenerator;
    }
    if (s == kOfxImageEffectContextFilter) {
        return eContextFilter;
    }
    if (s == kOfxImageEffectContextTransition) {
        return eContextTransition;
    }
    if (s == kOfxImageEffectContextPaint) {
        return eContextPaint;
    }
    if (s == kOfxImageEffectContextGeneral) {
        return eContextGeneral;
    }
    if (s == kOfxImageEffectContextRetimer) {
        return eContextRetimer;
    }
    if (s == kOfxImageEffectContextReader) {
        return eContextReader;
    }
    if (s == kOfxImageEffectContextWriter) {
        return eContextWriter;
    }
    if (s == kNatronOfxImageEffectContextTracker) {
        return eContextTracker;
    }
    qDebug() << "OfxEffectInstance::mapToContextEnum: Unknown image effect context '" << s.c_str() << "'";
    throw std::invalid_argument(s);
}

std::string
OfxEffectInstance::mapContextToString(Natron::ContextEnum ctx)
{
    switch (ctx) {
        case Natron::eContextGenerator:
            return kOfxImageEffectContextGenerator;
        case Natron::eContextFilter:
            return kOfxImageEffectContextFilter;
        case Natron::eContextTransition:
            return kOfxImageEffectContextTransition;
        case Natron::eContextPaint:
            return kOfxImageEffectContextPaint;
        case Natron::eContextGeneral:
            return kOfxImageEffectContextGeneral;
        case Natron::eContextRetimer:
            return kOfxImageEffectContextRetimer;
        case Natron::eContextReader:
            return kOfxImageEffectContextReader;
        case Natron::eContextWriter:
            return kOfxImageEffectContextWriter;
        case Natron::eContextTracker:
            return kNatronOfxImageEffectContextTracker;
        case Natron::eContextNone:
        default:
            break;
    }
    return std::string();
}

/**
 * @brief The purpose of this function is to allow Natron to modify slightly the values returned in the getClipPreferencesAction
 * by the plugin so that we can minimize the amount of Natron::Image::convertToFormat calls.
 **/
static void
clipPrefsProxy(OfxEffectInstance* self,
               double time,
               std::map<OfxClipInstance*,OfxImageEffectInstance::ClipPrefs>& clipPrefs,
               OfxImageEffectInstance::EffectPrefs& effectPrefs,
               std::list<OfxClipInstance*>& changedClips)
{
    ///We remap all the input clips components to be the same as the output clip, except for the masks.
    OfxClipInstance* outputClip = dynamic_cast<OfxClipInstance*>(self->effectInstance()->getClip(kOfxImageEffectOutputClipName));
    assert(outputClip);
    std::map<OfxClipInstance*,OfxImageEffectInstance::ClipPrefs>::iterator foundOutputPrefs = clipPrefs.find(outputClip);
    assert(foundOutputPrefs != clipPrefs.end());

    
    
    std::string outputClipDepth = foundOutputPrefs->second.bitdepth;
    
    //Might be needed for bugged plugins
    if (outputClipDepth.empty()) {
        outputClipDepth = self->effectInstance()->bestSupportedDepth(kOfxBitDepthFloat);
    }
    Natron::ImageBitDepthEnum outputClipDepthNatron = OfxClipInstance::ofxDepthToNatronDepth(outputClipDepth);
    
    ///Set a warning on the node if the bitdepth conversion from one of the input clip to the output clip is lossy
    QString bitDepthWarning("This nodes converts higher bit depths images from its inputs to work. As "
                            "a result of this process, the quality of the images is degraded. The following conversions are done: \n");
    bool setBitDepthWarning = false;
    
    bool outputModified = false;
    
    if (!self->isSupportedBitDepth(OfxClipInstance::ofxDepthToNatronDepth(outputClipDepth))) {
        outputClipDepth = self->effectInstance()->bestSupportedDepth(kOfxBitDepthFloat);
        outputClipDepthNatron = OfxClipInstance::ofxDepthToNatronDepth(outputClipDepth);
        foundOutputPrefs->second.bitdepth = outputClipDepth;
        outputModified = true;
    }
    
    double outputAspectRatio = foundOutputPrefs->second.par;
    
    
    ///output clip doesn't support components just remap it, this is probably a plug-in bug.
    if (!outputClip->isSupportedComponent(foundOutputPrefs->second.components)) {
        foundOutputPrefs->second.components = outputClip->findSupportedComp(kOfxImageComponentRGBA);
        outputModified = true;
    }
    
    /// Remap Disparity and Motion vectors to generic XY if possible
    if ((foundOutputPrefs->second.components == kFnOfxImageComponentMotionVectors ||
         foundOutputPrefs->second.components == kFnOfxImageComponentStereoDisparity) &&
        outputClip->isSupportedComponent(kNatronOfxImageComponentXY)) {
        foundOutputPrefs->second.components = kNatronOfxImageComponentXY;
        outputModified = true;
    }

    
    ///Adjust output premultiplication if needed
    if (foundOutputPrefs->second.components == kOfxImageComponentRGB) {
        effectPrefs.premult = kOfxImageOpaque;
    } else if (foundOutputPrefs->second.components == kOfxImageComponentAlpha) {
        effectPrefs.premult = kOfxImagePreMultiplied;
    }
    
    
    int maxInputs = self->getMaxInputCount();
    
    for (int i = 0; i < maxInputs; ++i) {
        EffectInstance* inputEffect = self->getInput(i);
        if (inputEffect) {
            inputEffect = inputEffect->getNearestNonIdentity(time);
        }
        OfxEffectInstance* instance = dynamic_cast<OfxEffectInstance*>(inputEffect);
        OfxClipInstance* clip = self->getClipCorrespondingToInput(i);
        
        bool hasChanged = false;

        std::map<OfxClipInstance*,OfxImageEffectInstance::ClipPrefs>::iterator foundClipPrefs = clipPrefs.find(clip);
        assert(foundClipPrefs != clipPrefs.end());
        
 
        if (instance) {
            
            

            
            ///This is the output clip of the input node
            OFX::Host::ImageEffect::ClipInstance* inputOutputClip = instance->effectInstance()->getClip(kOfxImageEffectOutputClipName);

            ///Try to remap the clip's bitdepth to be the same as
            const std::string & input_outputDepth = inputOutputClip->getPixelDepth();
            Natron::ImageBitDepthEnum input_outputNatronDepth = OfxClipInstance::ofxDepthToNatronDepth(input_outputDepth);
            
            ///If supported, set the clip's bitdepth to be the same as the output depth of the input node
            if ( self->isSupportedBitDepth(input_outputNatronDepth) ) {
                bool depthsDifferent = input_outputNatronDepth != outputClipDepthNatron;
                if (self->effectInstance()->supportsMultipleClipDepths() && depthsDifferent) {
                    foundClipPrefs->second.bitdepth = input_outputDepth;
                    hasChanged = true;
                }
            }
            ///Otherwise if the bit-depth conversion will be lossy, warn the user
            else if ( Image::isBitDepthConversionLossy(input_outputNatronDepth, outputClipDepthNatron) ) {
                bitDepthWarning.append( instance->getNode()->getLabel_mt_safe().c_str() );
                bitDepthWarning.append(" (" + QString( Image::getDepthString(input_outputNatronDepth).c_str() ) + ")");
                bitDepthWarning.append(" ----> ");
                bitDepthWarning.append( self->getNode()->getLabel_mt_safe().c_str() );
                bitDepthWarning.append(" (" + QString( Image::getDepthString(outputClipDepthNatron).c_str() ) + ")");
                bitDepthWarning.append('\n');
                setBitDepthWarning = true;
            }
            
            if (!self->effectInstance()->supportsMultipleClipPARs() && foundClipPrefs->second.par != outputAspectRatio && foundClipPrefs->first->getConnected()) {
                qDebug() << self->getScriptName_mt_safe().c_str() << ": An input clip ("<< foundClipPrefs->first->getName().c_str()
                << ") has a pixel aspect ratio (" << foundClipPrefs->second.par
                << ") different than the output clip (" << outputAspectRatio << ") but it doesn't support multiple clips PAR. "
                << "This should have been handled earlier before connecting the nodes, @see Node::canConnectInput.";
            }
        } // if(instance)
        if (hasChanged) {
            changedClips.push_back(clip);
        }
    }
    
    if (outputModified) {
        changedClips.push_back(outputClip);
    }
    
    self->getNode()->toggleBitDepthWarning(setBitDepthWarning, bitDepthWarning);
    
} //endCheckOFXClipPreferences



bool
OfxEffectInstance::checkOFXClipPreferences(double time,
                                           const RenderScale & scale,
                                           const std::string & reason,
                                           bool forceGetClipPrefAction)
{
    
    if (!_created) {
        return false;
    }
    assert(_context != eContextNone);
    assert( QThread::currentThread() == qApp->thread() );
    
    ////////////////////////////////////////////////////////////////
    ///////////////////////////////////
    //////////////// STEP 1 : Get plug-in render preferences
    std::map<OfxClipInstance*,OfxImageEffectInstance::ClipPrefs> clipsPrefs;
    OfxImageEffectInstance::EffectPrefs effectPrefs;
    {
        RECURSIVE_ACTION();
        /*
         We allow parameters values changes within the getClipPreference action because some parameters may only be updated
         reliably in this action. For example a choice parameter tracking all available components upstream in each clip needs
         to be refreshed in getClipPreferences since it may change due to a param change in a plug-in upstream.
         It is then up to the plug-in to avoid infinite recursions in getClipPreference.
         */
        SET_CAN_SET_VALUE(true);
        
        ///Take the preferences lock so that it cannot be modified throughout the action.
        QWriteLocker preferencesLocker(_preferencesLock);
        if (forceGetClipPrefAction) {
            if (!_effect->getClipPreferences_safe(clipsPrefs,effectPrefs)) {
                return false;
            }
        } else {
            if (_effect->areClipPrefsDirty()) {
                if (!_effect->getClipPreferences_safe(clipsPrefs, effectPrefs)) {
                    return false;
                }
            } else {
                return false;
            }
        }
    }
    
    
    ////////////////////////////////////////////////////////////////
    ////////////////////////////////
    //////////////// STEP 2: Apply a proxy, i.e: modify the preferences so it requires a minimum pixel shuffling
    std::list<OfxClipInstance*> modifiedClips;
    
    clipPrefsProxy(this,time,clipsPrefs,effectPrefs,modifiedClips);
    
    bool changed = false;
    
    ////////////////////////////////////////////////////////////////
    ////////////////////////////////
    //////////////// STEP 3: Actually push to the clips the preferences and set the flags on the effect, protected by a write lock.
    
    {
        QWriteLocker l(_preferencesLock);
        for (std::map<OfxClipInstance*,OfxImageEffectInstance::ClipPrefs>::const_iterator it = clipsPrefs.begin(); it != clipsPrefs.end(); ++it) {
            if (it->first->getComponents() != it->second.components) {
                it->first->setComponents(it->second.components);
                changed = true;
            }
            if (it->second.bitdepth != it->first->getPixelDepth()) {
                it->first->setPixelDepth(it->second.bitdepth);
                changed = true;
            }
            if (it->second.par != it->first->getAspectRatio()) {
                it->first->setAspectRatio(it->second.par);
                changed = true;
            }
        }
        
        changed |= effectInstance()->updatePreferences_safe(effectPrefs.frameRate, effectPrefs.fielding, effectPrefs.premult,
                                                 effectPrefs.continuous, effectPrefs.frameVarying);
    }
    
    
    ////////////////////////////////////////////////////////////////
    ////////////////////////////////
    //////////////// STEP 4: If our proxy remapping changed some clips preferences, notifying the plug-in of the clips which changed
    if (!getApp()->getProject()->isLoadingProject() && !getApp()->isCreatingPythonGroup()) {
        RECURSIVE_ACTION();
        SET_CAN_SET_VALUE(true);
        if (!modifiedClips.empty()) {
            effectInstance()->beginInstanceChangedAction(reason);
        }
        for (std::list<OfxClipInstance*>::iterator it = modifiedClips.begin(); it != modifiedClips.end(); ++it) {
            effectInstance()->clipInstanceChangedAction((*it)->getName(), reason, time, scale);
        }
        if (!modifiedClips.empty()) {
            effectInstance()->endInstanceChangedAction(reason);
        }
    }
    
    return changed;
} // checkOFXClipPreferences


std::vector<std::string>
OfxEffectInstance::supportedFileFormats() const
{
    assert(_context != eContextNone);
    int formatsCount = _effect->getDescriptor().getProps().getDimension(kTuttleOfxImageEffectPropSupportedExtensions);
    std::vector<std::string> formats(formatsCount);
    for (int k = 0; k < formatsCount; ++k) {
        formats[k] = _effect->getDescriptor().getProps().getStringProperty(kTuttleOfxImageEffectPropSupportedExtensions,k);
        std::transform(formats[k].begin(), formats[k].end(), formats[k].begin(), ::tolower);
    }

    return formats;
}

Natron::StatusEnum
OfxEffectInstance::getRegionOfDefinition(U64 /*hash*/,
                                         double time,
                                         const RenderScale & scale,
                                         int view,
                                         RectD* rod)
{
    assert(_context != eContextNone);
    if (!_initialized) {
        return Natron::eStatusFailed;
    }

    assert(_effect);

    unsigned int mipMapLevel = Image::getLevelFromScale(scale.x);

    // getRegionOfDefinition may be the first action with renderscale called on any effect.
    // it may have to check for render scale support.
    SupportsEnum supportsRS = supportsRenderScaleMaybe();
    bool scaleIsOne = (scale.x == 1. && scale.y == 1.);
    if ( (supportsRS == eSupportsNo) && !scaleIsOne ) {
        qDebug() << "getRegionOfDefinition called with render scale != 1, but effect does not support render scale!";

        return eStatusFailed;
    }

    OfxRectD ofxRod;
    OfxStatus stat;
    
    {
        bool skipDiscarding = false;
        if (getRecursionLevel() > 1) {
            skipDiscarding = true;
        }
        
        
        ClipsThreadStorageSetter clipSetter(effectInstance(),
                                            skipDiscarding,
                                            true, //< setView ?
                                            view,
                                            true, //< set mipmaplevel?
                                            mipMapLevel);
        
        {
            if (getRecursionLevel() > 1) {
                stat = _effect->getRegionOfDefinitionAction(time, scale, view, ofxRod);
            } else {
                ///Take the preferences lock so that it cannot be modified throughout the action.
                QReadLocker preferencesLocker(_preferencesLock);
                stat = _effect->getRegionOfDefinitionAction(time, scale, view, ofxRod);
            }
        }
        if (supportsRS == eSupportsMaybe) {
            OfxRectD tmpRod;
            if ( (stat == kOfxStatOK) || (stat == kOfxStatReplyDefault) ) {
                if (!scaleIsOne) {
                    // we got at least one success with RS != 1
                    setSupportsRenderScaleMaybe(eSupportsYes);
                } else {
                    //try with scale != 1
                    // maybe the effect does not support renderscale
                    // try again with scale one
                    OfxPointD halfScale;
                    halfScale.x = halfScale.y = .5;
                    
                    {
                        SET_CAN_SET_VALUE(false);
                        
                        if (getRecursionLevel() > 1) {
                            stat = _effect->getRegionOfDefinitionAction(time, halfScale, view, tmpRod);
                        } else {
                            ///Take the preferences lock so that it cannot be modified throughout the action.
                            QReadLocker preferencesLocker(_preferencesLock);
                            stat = _effect->getRegionOfDefinitionAction(time, halfScale, view, tmpRod);
                        }
                    }
                    if ( (stat == kOfxStatOK) || (stat == kOfxStatReplyDefault) ) {
                        setSupportsRenderScaleMaybe(eSupportsYes);
                    } else {
                        setSupportsRenderScaleMaybe(eSupportsNo);
                    }
                }
            } else if (stat == kOfxStatFailed) {
                
                if (scaleIsOne) {
                    //scale one failed, we can't say anything
                     return eStatusFailed;
                } else {
                    // maybe the effect does not support renderscale
                    // try again with scale one
                    OfxPointD scaleOne;
                    scaleOne.x = scaleOne.y = 1.;
                    
                    {
                        SET_CAN_SET_VALUE(false);
                        
                        if (getRecursionLevel() > 1) {
                            stat = _effect->getRegionOfDefinitionAction(time, scaleOne, view, tmpRod);
                        } else {
                            ///Take the preferences lock so that it cannot be modified throughout the action.
                            QReadLocker preferencesLocker(_preferencesLock);
                            stat = _effect->getRegionOfDefinitionAction(time, scaleOne, view, tmpRod);
                        }
                    }
                    
                    if ( (stat == kOfxStatOK) || (stat == kOfxStatReplyDefault) ) {
                        // we got success with scale = 1, which means it doesn't support renderscale after all
                        setSupportsRenderScaleMaybe(eSupportsNo);
                    } else {
                        // if both actions failed, we can't say anything
                        return eStatusFailed;
                    }
                }
                
            }
        }
        if ( (stat != kOfxStatOK) && (stat != kOfxStatReplyDefault) ) {
            return eStatusFailed;
        }

        /// This code is not needed since getRegionOfDefinitionAction in HostSupport does it for us
        /// plus it is horribly slow (don't know why)
//        if (stat == kOfxStatReplyDefault) {
//            calcDefaultRegionOfDefinition(hash,time,view, scale, rod);
//
//            return eStatusReplyDefault;
//        }
    }


    ///If the rod is 1 pixel, determine if it was because one clip was unconnected or this is really a
    ///1 pixel large image
    if ( (ofxRod.x2 == 1.) && (ofxRod.y2 == 1.) && (ofxRod.x1 == 0.) && (ofxRod.y1 == 0.) ) {
        int maxInputs = getMaxInputCount();
        for (int i = 0; i < maxInputs; ++i) {
            OfxClipInstance* clip = getClipCorrespondingToInput(i);
            if ( clip && !clip->getConnected() && !clip->isOptional() && !clip->isMask() ) {
                ///this is a mandatory source clip and it is not connected, return statfailed
                return eStatusFailed;
            }
        }
    }

    RectD::ofxRectDToRectD(ofxRod, rod);
    return eStatusOK;

    // OFX::Host::ImageEffect::ClipInstance* clip = effectInstance()->getClip(kOfxImageEffectOutputClipName);
    //assert(clip);
    //double pa = clip->getAspectRatio();
} // getRegionOfDefinition

void
OfxEffectInstance::calcDefaultRegionOfDefinition(U64 /*hash*/,
                                                 double time,
                                                 int view,
                                                 const RenderScale & scale,
                                                 RectD *rod)
{
    assert(_context != eContextNone);
    if (!_initialized) {
        throw std::runtime_error("OfxEffectInstance not initialized");
    }
    
    bool skipDiscarding = false;
    if (getRecursionLevel() > 1) {
        skipDiscarding = true;
    }
    unsigned int mipMapLevel = Image::getLevelFromScale(scale.x);
    OfxRectD ofxRod;
    
    {
        SET_CAN_SET_VALUE(false);
        
        ///Take the preferences lock so that it cannot be modified throughout the action.
        if (getRecursionLevel() == 0) {
            ClipsThreadStorageSetter clipSetter(effectInstance(),
                                                skipDiscarding,
                                                true, //< setView ?
                                                view,
                                                true, //< set mipmaplevel?
                                                mipMapLevel);
            
            
            // from http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#kOfxImageEffectActionGetRegionOfDefinition
            // generator context - defaults to the project window,
            // filter and paint contexts - defaults to the RoD of the 'Source' input clip at the given time,
            // transition context - defaults to the union of the RoDs of the 'SourceFrom' and 'SourceTo' input clips at the given time,
            // general context - defaults to the union of the RoDs of all the effect non optional input clips at the given time, if none exist, then it is the project window
            // retimer context - defaults to the union of the RoD of the 'Source' input clip at the frame directly preceding the value of the 'SourceTime' double parameter and the frame directly after it
            
            // the following ofxh function does the job
            QReadLocker preferencesLocker(_preferencesLock);
            ofxRod = _effect->calcDefaultRegionOfDefinition(time, (OfxPointD)scale);
        } else {
            ofxRod = _effect->calcDefaultRegionOfDefinition(time, (OfxPointD)scale);
        }
    }
    rod->x1 = ofxRod.x1;
    rod->x2 = ofxRod.x2;
    rod->y1 = ofxRod.y1;
    rod->y2 = ofxRod.y2;
}

static void
rectToOfxRectD(const RectD & b,
               OfxRectD *out)
{
    out->x1 = b.left();
    out->x2 = b.right();
    out->y1 = b.bottom();
    out->y2 = b.top();
}

void
OfxEffectInstance::getRegionsOfInterest(double time,
                                        const RenderScale & scale,
                                        const RectD & outputRoD,
                                        const RectD & renderWindow, //!< the region to be rendered in the output image, in Canonical Coordinates
                                        int view,
                                        RoIMap* ret)
{
    assert(_context != eContextNone);
    std::map<OFX::Host::ImageEffect::ClipInstance*,OfxRectD> inputRois;
    if (!_initialized) {
        return;
    }
    assert(outputRoD.x2 >= outputRoD.x1 && outputRoD.y2 >= outputRoD.y1);
    Q_UNUSED(outputRoD);
    assert(renderWindow.x2 >= renderWindow.x1 && renderWindow.y2 >= renderWindow.y1);

    {
        bool scaleIsOne = (scale.x == 1. && scale.y == 1.);
        assert( !( (supportsRenderScaleMaybe() == eSupportsNo) && !scaleIsOne ) );
        Q_UNUSED(scaleIsOne);
    }

    OfxStatus stat;

    ///before calling getRoIaction set the relevant info on the clips

    unsigned int mipMapLevel = Image::getLevelFromScale(scale.x);
    {
        SET_CAN_SET_VALUE(false);

        bool skipDiscarding = false;
        if (getRecursionLevel() > 1) {
            // getRegionsOfInterest may be called recursively as a result of calling fetchImage() from an action
            skipDiscarding = true;
        }
        ClipsThreadStorageSetter clipSetter(effectInstance(),
                                            skipDiscarding,
                                            true, //< setView ?
                                            view,
                                            true,
                                            mipMapLevel);
        OfxRectD roi;
        rectToOfxRectD(renderWindow, &roi);
        
        ///Take the preferences lock so that it cannot be modified throughout the action.
        QReadLocker preferencesLocker(_preferencesLock);
        stat = _effect->getRegionOfInterestAction( (OfxTime)time, scale, view,
                                                   roi, inputRois );
    }


    if ( (stat != kOfxStatOK) && (stat != kOfxStatReplyDefault) ) {
        throw std::runtime_error("getRegionsOfInterest action failed");
    }
    
    //Default behaviour already handled in getRegionOfInterestAction
    
    for (std::map<OFX::Host::ImageEffect::ClipInstance*,OfxRectD>::iterator it = inputRois.begin(); it != inputRois.end(); ++it) {
        OfxClipInstance* clip = dynamic_cast<OfxClipInstance*>(it->first);
        assert(clip);
        if (clip) {
            EffectInstance* inputNode = clip->getAssociatedNode();
            RectD inputRoi; // input RoI in canonical coordinates
            inputRoi.x1 = it->second.x1;
            inputRoi.x2 = it->second.x2;
            inputRoi.y1 = it->second.y1;
            inputRoi.y2 = it->second.y2;
            
            if (inputRoi.isNull()) {
                continue;
            }
            
            ///The RoI might be infinite if the getRoI action of the plug-in doesn't do anything and the input effect has an
            ///infinite rod.
            ifInfiniteclipRectToProjectDefault(&inputRoi);
            //if (!inputRoi.isNull()) {
            ret->insert( std::make_pair(inputNode,inputRoi) );
            //}
        }
    }
    
    
    
} // getRegionsOfInterest

FramesNeededMap
OfxEffectInstance::getFramesNeeded(double time, int view)
{
    assert(_context != eContextNone);
    FramesNeededMap ret;
    if (!_initialized) {
        return ret;
    }
    assert(_effect);
    OfxStatus stat;
    
    if (isViewAware()) {
        
        OFX::Host::ImageEffect::ViewsRangeMap inputRanges;
        {
            SET_CAN_SET_VALUE(false);
            
            ///Take the preferences lock so that it cannot be modified throughout the action.
            QReadLocker preferencesLocker(_preferencesLock);
            stat = _effect->getFrameViewsNeeded( (OfxTime)time, view, inputRanges );
        }
        
        if ( (stat != kOfxStatOK) && (stat != kOfxStatReplyDefault) ) {
            throw std::runtime_error("getFrameViewsNeeded action failed");
        } else if (stat == kOfxStatOK) {
            for (OFX::Host::ImageEffect::ViewsRangeMap::iterator it = inputRanges.begin(); it != inputRanges.end(); ++it) {
                OfxClipInstance* clip = dynamic_cast<OfxClipInstance*>(it->first);
                assert(clip);
                if (clip) {
                    int inputNb = clip->getInputNb();
                    if (inputNb != -1) {
                        ret.insert( std::make_pair(inputNb,it->second) );
                    }
                }
            }
        }

    } else {
        OFX::Host::ImageEffect::RangeMap inputRanges;
        {
            SET_CAN_SET_VALUE(false);
            
            ///Take the preferences lock so that it cannot be modified throughout the action.
            QReadLocker preferencesLocker(_preferencesLock);
            stat = _effect->getFrameNeededAction( (OfxTime)time, inputRanges );
        }
        if ( (stat != kOfxStatOK) && (stat != kOfxStatReplyDefault) ) {
            throw std::runtime_error("getFramesNeeded action failed");
        } else {
            for (OFX::Host::ImageEffect::RangeMap::iterator it = inputRanges.begin(); it != inputRanges.end(); ++it) {
                OfxClipInstance* clip = dynamic_cast<OfxClipInstance*>(it->first);
                assert(clip);
                if (clip) {
                    int inputNb = clip->getInputNb();
                    if (inputNb != -1) {
                        std::map<int, std::vector<OfxRangeD> > viewRangeMap;
                        viewRangeMap.insert(std::make_pair(view, it->second));
                        ret.insert(std::make_pair(inputNb, viewRangeMap));
                    }
                }
            }
        }
    }
    
    //Default is already handled by HostSupport
//    if (stat == kOfxStatReplyDefault) {
//        return Natron::EffectInstance::getFramesNeeded(time,view);
//    }
    return ret;
}

void
OfxEffectInstance::getFrameRange(double *first,
                                 double *last)
{
    assert(_context != eContextNone);
    if (!_initialized) {
        return;
    }
    OfxRangeD range;
    // getTimeDomain should only be called on the 'general', 'reader' or 'generator' contexts.
    //  see http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#kOfxImageEffectActionGetTimeDomain"
    // Edit: Also add the 'writer' context as we need the getTimeDomain action to be able to find out the frame range to render.
    OfxStatus st = kOfxStatReplyDefault;
    if ( (_context == eContextGeneral) ||
         ( _context == eContextReader) ||
         ( _context == eContextWriter) ||
         ( _context == eContextGenerator) ) {
        
        SET_CAN_SET_VALUE(false);
        ///Take the preferences lock so that it cannot be modified throughout the action.
        QReadLocker preferencesLocker(_preferencesLock);
        st = _effect->getTimeDomainAction(range);
    }
    if (st == kOfxStatOK) {
        *first = range.min;
        *last = range.max;
    } else if (st == kOfxStatReplyDefault) {
        //The default is...
        int nthClip = _effect->getNClips();
        if (nthClip == 0) {
            //infinite if there are no non optional input clips.
            *first = INT_MIN;
            *last = INT_MAX;
        } else {
            //the union of all the frame ranges of the non optional input clips.
            bool firstValidInput = true;
            *first = INT_MIN;
            *last = INT_MAX;

            int inputsCount = getMaxInputCount();

            ///Uncommented the isOptional() introduces a bugs with Genarts Monster plug-ins when 2 generators
            ///are connected in the pipeline. They must rely on the time domain to maintain an internal state and apparantly
            ///not taking optional inputs into accounts messes it up.
            for (int i = 0; i < inputsCount; ++i) {
                //if (!isInputOptional(i)) {
                EffectInstance* inputEffect = getInput(i);
                if (inputEffect) {
                    double f,l;
                    inputEffect->getFrameRange_public(inputEffect->getRenderHash(),&f, &l);
                    if (!firstValidInput) {
                        if ( (f < *first) && (f != INT_MIN) ) {
                            *first = f;
                        }
                        if ( (l > *last) && (l != INT_MAX) ) {
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
} // getFrameRange

bool
OfxEffectInstance::isIdentity(double time,
                              const RenderScale & scale,
                              const RectI & renderWindow,
                              int view,
                              double* inputTime,
                              int* inputNb)
{
    if (!_created) {
        *inputNb = -1;
        *inputTime = 0;
        return false;
    }
    
    assert(_context != eContextNone);
    const std::string field = kOfxImageFieldNone; // TODO: support interlaced data
    std::string inputclip;
    OfxTime inputTimeOfx = time;


    // isIdentity may be the first action with renderscale called on any effect.
    // it may have to check for render scale support.
    SupportsEnum supportsRS = supportsRenderScaleMaybe();
    bool scaleIsOne = (scale.x == 1. && scale.y == 1.);
    if ( (supportsRS == eSupportsNo) && !scaleIsOne ) {
        qDebug() << "isIdentity called with render scale != 1, but effect does not support render scale!";
        assert(false);
        throw std::logic_error("isIdentity called with render scale != 1, but effect does not support render scale!");
    }
    
    unsigned int mipMapLevel = Image::getLevelFromScale(scale.x);
    OfxStatus stat;
    
    {
        bool skipDiscarding = false;
        
        if (getRecursionLevel() > 1) {
            
//#ifdef DEBUG
//            if (QThread::currentThread() != qApp->thread()) {
//                qDebug() << "isIdentity cannot be called recursively as an action. Please check this.";
//            }
//#endif
            skipDiscarding = true;
        }
        
        SET_CAN_SET_VALUE(false);

        
        ClipsThreadStorageSetter clipSetter(effectInstance(),
                                            skipDiscarding,
                                            true, //< setView ?
                                            view,
                                            true,
                                            mipMapLevel);
        
        OfxRectI ofxRoI;
        ofxRoI.x1 = renderWindow.left();
        ofxRoI.x2 = renderWindow.right();
        ofxRoI.y1 = renderWindow.bottom();
        ofxRoI.y2 = renderWindow.top();
        
        {
            if (getRecursionLevel() > 1) {
                stat = _effect->isIdentityAction(inputTimeOfx, field, ofxRoI, scale, view, inputclip);
            } else {
                ///Take the preferences lock so that it cannot be modified throughout the action.
                QReadLocker preferencesLocker(_preferencesLock);
                stat = _effect->isIdentityAction(inputTimeOfx, field, ofxRoI, scale, view, inputclip);
            }
        }
       
    }

    if (stat == kOfxStatOK) {
        OFX::Host::ImageEffect::ClipInstance* clip = _effect->getClip(inputclip);
        if (!clip) {
            // this is a plugin-side error, don't crash
            qDebug() << "Error in OfxEffectInstance::render(): kOfxImageEffectActionIsIdentity returned an unknown clip: " << inputclip.c_str();

            return false;
        }
        OfxClipInstance* natronClip = dynamic_cast<OfxClipInstance*>(clip);
        assert(natronClip);
        if (!natronClip) {
            // coverity[dead_error_line]
            qDebug() << "Error in OfxEffectInstance::render(): kOfxImageEffectActionIsIdentity returned an unknown clip: " << inputclip.c_str();

            return false;
        }
        *inputTime = inputTimeOfx;

        if ( natronClip->isOutput() ) {
            *inputNb = -2;
        } else {
            *inputNb = natronClip->getInputNb();
        }

        return true;
    } else if (stat == kOfxStatReplyDefault) {
        return false;
    }
    return false; //< may fail if getRegionOfDefinition has failed in the plug-in code
    //throw std::runtime_error("isIdentity failed");
} // isIdentity

Natron::StatusEnum
OfxEffectInstance::beginSequenceRender(double first,
                                       double last,
                                       double step,
                                       bool interactive,
                                       const RenderScale & scale,
                                       bool isSequentialRender,
                                       bool isRenderResponseToUserInteraction,
                                       bool draftMode,
                                       int view)
{
    {
        bool scaleIsOne = (scale.x == 1. && scale.y == 1.);
        assert( !( (supportsRenderScaleMaybe() == eSupportsNo) && !scaleIsOne ) );
        Q_UNUSED(scaleIsOne);
    }

    OfxStatus stat;
    unsigned int mipMapLevel = Image::getLevelFromScale(scale.x);
    {
        bool skipDiscarding = false;
        if (getRecursionLevel() > 1) {
            qDebug() << "beginRenderAction cannot be called recursively as an action. Please check this.";
            skipDiscarding = true;
        }
        ClipsThreadStorageSetter clipSetter(effectInstance(),
                                            skipDiscarding,
                                            true, //< setView ?
                                            view,
                                            true,
                                            mipMapLevel);

        SET_CAN_SET_VALUE(false);

        
        ///Take the preferences lock so that it cannot be modified throughout the action.
        QReadLocker preferencesLocker(_preferencesLock);
        stat = effectInstance()->beginRenderAction(first, last, step,
                                                   interactive, scale,
                                                   isSequentialRender, isRenderResponseToUserInteraction,
                                                   /*openGLRender=*/false, draftMode, view);
    }

    if ( (stat != kOfxStatOK) && (stat != kOfxStatReplyDefault) ) {
        return eStatusFailed;
    }

    return eStatusOK;
}

Natron::StatusEnum
OfxEffectInstance::endSequenceRender(double first,
                                     double last,
                                     double step,
                                     bool interactive,
                                     const RenderScale & scale,
                                     bool isSequentialRender,
                                     bool isRenderResponseToUserInteraction,
                                     bool draftMode,
                                     int view)
{
    {
        bool scaleIsOne = (scale.x == 1. && scale.y == 1.);
        assert( !( (supportsRenderScaleMaybe() == eSupportsNo) && !scaleIsOne ) );
        Q_UNUSED(scaleIsOne);
    }

    OfxStatus stat;
    unsigned int mipMapLevel = Image::getLevelFromScale(scale.x);
    {
        bool skipDiscarding = false;
        if (getRecursionLevel() > 1) {
            qDebug() << "endRenderAction cannot be called recursively as an action. Please check this.";
            skipDiscarding = true;
        }
        ClipsThreadStorageSetter clipSetter(effectInstance(),
                                            skipDiscarding,
                                            true, //< setView ?
                                            view,
                                            true,
                                            mipMapLevel);
        SET_CAN_SET_VALUE(false);

        
        ///Take the preferences lock so that it cannot be modified throughout the action.
        QReadLocker preferencesLocker(_preferencesLock);
        stat = effectInstance()->endRenderAction(first, last, step,
                                                 interactive, scale,
                                                 isSequentialRender, isRenderResponseToUserInteraction,
                                                 /*openGLRender=*/false, draftMode, view);
    }

    if ( (stat != kOfxStatOK) && (stat != kOfxStatReplyDefault) ) {
        return eStatusFailed;
    }

    return eStatusOK;
}

Natron::StatusEnum
OfxEffectInstance::render(const RenderActionArgs& args)
{
    if (!_initialized) {
        return Natron::eStatusFailed;
    }

    assert(!args.outputPlanes.empty());
    
    const std::pair<ImageComponents,ImagePtr>& firstPlane = args.outputPlanes.front();
    
    OfxRectI ofxRoI;
    ofxRoI.x1 = args.roi.left();
    ofxRoI.x2 = args.roi.right();
    ofxRoI.y1 = args.roi.bottom();
    ofxRoI.y2 = args.roi.top();
    int viewsCount = getApp()->getProject()->getProjectViewsCount();
    OfxStatus stat;
    const std::string field = kOfxImageFieldNone; // TODO: support interlaced data
    
    bool multiPlanar = isMultiPlanar();
    
    std::list<std::string> ofxPlanes;
    for (std::list<std::pair<ImageComponents,boost::shared_ptr<Natron::Image> > >::const_iterator it = args.outputPlanes.begin();
         it!=args.outputPlanes.end(); ++it) {
        if (!multiPlanar) {
            ofxPlanes.push_back(OfxClipInstance::natronsPlaneToOfxPlane(it->second->getComponents()));
        } else {
            ofxPlanes.push_back(OfxClipInstance::natronsPlaneToOfxPlane(it->first));
        }
    }
    
    ///before calling render, set the render scale thread storage for each clip
# ifdef DEBUG
    {
        // check the dimensions of output images
        const RectI & dstBounds = firstPlane.second->getBounds();
        const RectD & dstRodCanonical = firstPlane.second->getRoD();
        RectI dstRod;
        dstRodCanonical.toPixelEnclosing(args.mappedScale, firstPlane.second->getPixelAspectRatio(), &dstRod);

        if ( !supportsTiles() && !isDuringPaintStrokeCreationThreadLocal()) {
            // http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#kOfxImageEffectPropSupportsTiles
            //  If a clip or plugin does not support tiled images, then the host should supply full RoD images to the effect whenever it fetches one.
            assert(dstRod.x1 == dstBounds.x1);
            assert(dstRod.x2 == dstBounds.x2);
            assert(dstRod.y1 == dstBounds.y1);
            assert(dstRod.y2 == dstBounds.y2);
        }
        if ( !supportsMultiResolution() ) {
            // http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#kOfxImageEffectPropSupportsMultiResolution
            //   Multiple resolution images mean...
            //    input and output images can be of any size
            //    input and output images can be offset from the origin
            assert(dstRod.x1 == 0);
            assert(dstRod.y1 == 0);
        }
    }
# endif // DEBUG
    {
        bool skipDiscarding = false;
        if (getRecursionLevel() > 1) {
            qDebug() << "renderAction cannot be called recursively as an action. Please check this.";
            skipDiscarding = true;
        }
        
        SET_CAN_SET_VALUE(false);
        
        
        RenderThreadStorageSetter clipSetter(this,
                                             skipDiscarding,
                                             true, //< setView ?
                                             args.view,
                                             true,//< set mipmaplevel ?
                                             Natron::Image::getLevelFromScale(args.originalScale.x),
                                             !isMultiPlanar(),
                                             firstPlane.first,
                                             args.inputImages);

        
        ///Take the preferences lock so that it cannot be modified throughout the action.
        QReadLocker preferencesLocker(_preferencesLock);
        stat = _effect->renderAction( (OfxTime)args.time,
                                     field,
                                     ofxRoI,
                                     args.mappedScale,
                                     args.isSequentialRender,
                                     args.isRenderResponseToUserInteraction,
                                     /*openGLRender=*/false,
                                     args.draftMode,
                                     args.view,
                                     viewsCount,
                                     ofxPlanes);
        
        
    }
    
    if (stat != kOfxStatOK) {
        
        if (!getNode()->hasPersistentMessage()) {
            QString err;
            err.append(QObject::tr("Render failed: "));
            if (stat == kOfxStatErrImageFormat) {
                err.append(QObject::tr("Bad image format was supplied by "));
                err.append(NATRON_APPLICATION_NAME);
            } else if (stat == kOfxStatErrMemory) {
                err.append(QObject::tr("Out of memory!"));
            } else {
                err.append(QObject::tr("Unknown failure reason"));
            }
            setPersistentMessage(Natron::eMessageTypeError, err.toStdString());
        }
        return eStatusFailed;
    } else {
        return eStatusOK;
    }
} // render

bool
OfxEffectInstance::supportsMultipleClipsPAR() const
{
    return _effect->supportsMultipleClipPARs();
}

Natron::RenderSafetyEnum
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
        const std::string & safety = _effect->getRenderThreadSafety();
        if (safety == kOfxImageEffectRenderUnsafe) {
            _renderSafety =  Natron::eRenderSafetyUnsafe;
        } else if (safety == kOfxImageEffectRenderInstanceSafe) {
            _renderSafety = Natron::eRenderSafetyInstanceSafe;
        } else if (safety == kOfxImageEffectRenderFullySafe) {
            if ( _effect->getHostFrameThreading() ) {
                _renderSafety =  Natron::eRenderSafetyFullySafeFrame;
            } else {
                _renderSafety =  Natron::eRenderSafetyFullySafe;
            }
        } else {
            qDebug() << "Unknown thread safety level: " << safety.c_str();
            _renderSafety =  Natron::eRenderSafetyUnsafe;
        }
        _wasRenderSafetySet = true;

        return _renderSafety;
    }
}

bool
OfxEffectInstance::makePreviewByDefault() const
{
    return isReader();
}

const std::string &
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
OfxEffectInstance::drawOverlay(double time,
                               double scaleX,
                               double scaleY)
{
    if (!_initialized) {
        return;
    }
    if (_overlayInteract) {
        OfxPointD rs;
        rs.x = scaleX;
        rs.y = scaleY;

        SET_CAN_SET_VALUE(false);
        _overlayInteract->drawAction(time, rs);
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
OfxEffectInstance::onOverlayPenDown(double time,
                                    double scaleX,
                                    double scaleY,
                                    const QPointF & viewportPos,
                                    const QPointF & pos,
                                    double pressure)
{
    if (!_initialized) {
        return false;
    }
    if (_overlayInteract) {
        OfxPointD rs;
        rs.x = scaleX;
        rs.y = scaleY;
        OfxPointD penPos;
        penPos.x = pos.x();
        penPos.y = pos.y();
        OfxPointI penPosViewport;
        penPosViewport.x = viewportPos.x();
        penPosViewport.y = viewportPos.y();

        SET_CAN_SET_VALUE(true);

        OfxStatus stat = _overlayInteract->penDownAction(time, rs, penPos, penPosViewport, pressure);
        

        if (getRecursionLevel() == 1 && checkIfOverlayRedrawNeeded()) {
            OfxStatus redrawstat = _overlayInteract->redraw();
            assert(redrawstat == kOfxStatOK || redrawstat == kOfxStatReplyDefault);
            Q_UNUSED(redrawstat);
        }

        if (stat == kOfxStatOK) {
            _penDown = true;

            return true;
        }
    }

    return false;
}

bool
OfxEffectInstance::onOverlayPenMotion(double time,
                                      double scaleX,
                                      double scaleY,
                                      const QPointF & viewportPos,
                                      const QPointF & pos,
                                      double pressure)
{
    if (!_initialized) {
        return false;
    }
    if (_overlayInteract) {
        OfxPointD rs;
        rs.x = scaleX;
        rs.y = scaleY;
        OfxPointD penPos;
        penPos.x = pos.x();
        penPos.y = pos.y();
        OfxPointI penPosViewport;
        penPosViewport.x = viewportPos.x();
        penPosViewport.y = viewportPos.y();
        OfxStatus stat;

        SET_CAN_SET_VALUE(true);
        stat = _overlayInteract->penMotionAction(time, rs, penPos, penPosViewport, pressure);
        
        if (getRecursionLevel() == 1 && checkIfOverlayRedrawNeeded()) {
            stat = _overlayInteract->redraw();
            assert(stat == kOfxStatOK || stat == kOfxStatReplyDefault);
        }

        if (stat == kOfxStatOK) {
            return true;
        }
    }

    return false;
}

bool
OfxEffectInstance::onOverlayPenUp(double time,
                                  double scaleX,
                                  double scaleY,
                                  const QPointF & viewportPos,
                                  const QPointF & pos,
                                  double pressure)
{
    if (!_initialized) {
        return false;
    }
    if (_overlayInteract) {
        OfxPointD rs;
        rs.x = scaleX;
        rs.y = scaleY;
        OfxPointD penPos;
        penPos.x = pos.x();
        penPos.y = pos.y();
        OfxPointI penPosViewport;
        penPosViewport.x = viewportPos.x();
        penPosViewport.y = viewportPos.y();

        SET_CAN_SET_VALUE(true);
        OfxStatus stat = _overlayInteract->penUpAction(time, rs, penPos, penPosViewport, pressure);

        if (getRecursionLevel() == 1 && checkIfOverlayRedrawNeeded()) {
            stat = _overlayInteract->redraw();
            assert(stat == kOfxStatOK || stat == kOfxStatReplyDefault);
        }
        
        if (stat == kOfxStatOK) {
            _penDown = false;

            return true;
        }
    }

    return false;
}

bool
OfxEffectInstance::onOverlayKeyDown(double time,
                                    double scaleX,
                                    double scaleY,
                                    Natron::Key key,
                                    Natron::KeyboardModifiers /*modifiers*/)
{
    if (!_initialized) {
        return false;;
    }
    if (_overlayInteract) {
        OfxPointD rs;
        rs.x = scaleX;
        rs.y = scaleY;
        QByteArray keyStr;
        SET_CAN_SET_VALUE(true);
        OfxStatus stat = _overlayInteract->keyDownAction( time, rs, (int)key, keyStr.data() );

        if (getRecursionLevel() == 1 && checkIfOverlayRedrawNeeded()) {
            stat = _overlayInteract->redraw();
            assert(stat == kOfxStatOK || stat == kOfxStatReplyDefault);
        }
        
        if (stat == kOfxStatOK) {
            return true;
        }
    }

    return false;
}

bool
OfxEffectInstance::onOverlayKeyUp(double time,
                                  double scaleX,
                                  double scaleY,
                                  Natron::Key key,
                                  Natron::KeyboardModifiers /* modifiers*/)
{
    if (!_initialized) {
        return false;
    }
    if (_overlayInteract) {
        OfxPointD rs;
        rs.x = scaleX;
        rs.y = scaleY;
        QByteArray keyStr;
        SET_CAN_SET_VALUE(true);
        OfxStatus stat = _overlayInteract->keyUpAction( time, rs, (int)key, keyStr.data() );

        if (getRecursionLevel() == 1 && checkIfOverlayRedrawNeeded()) {
            stat = _overlayInteract->redraw();
            assert(stat == kOfxStatOK || stat == kOfxStatReplyDefault);
        }
        
        //assert(stat == kOfxStatOK || stat == kOfxStatReplyDefault);
        if (stat == kOfxStatOK) {
            return true;
        }
        ;
    }

    return false;
}

bool
OfxEffectInstance::onOverlayKeyRepeat(double time,
                                      double scaleX,
                                      double scaleY,
                                      Natron::Key key,
                                      Natron::KeyboardModifiers /*modifiers*/)
{
    if (!_initialized) {
        return false;
    }
    if (_overlayInteract) {
        OfxPointD rs;
        rs.x = scaleX;
        rs.y = scaleY;
        QByteArray keyStr;

        SET_CAN_SET_VALUE(true);
        OfxStatus stat = _overlayInteract->keyRepeatAction( time, rs, (int)key, keyStr.data() );

        if (getRecursionLevel() == 1 && checkIfOverlayRedrawNeeded()) {
            stat = _overlayInteract->redraw();
            assert(stat == kOfxStatOK || stat == kOfxStatReplyDefault);
        }
        
        if (stat == kOfxStatOK) {
            return true;
        }
    }

    return false;
}

bool
OfxEffectInstance::onOverlayFocusGained(double time,
                                        double scaleX,
                                        double scaleY)
{
    if (!_initialized) {
        return false;
    }
    if (_overlayInteract) {
        OfxPointD rs;
        rs.x = scaleX;
        rs.y = scaleY;
        OfxStatus stat;
        SET_CAN_SET_VALUE(true);
        stat = _overlayInteract->gainFocusAction(time, rs);
        if (stat == kOfxStatOK) {
            return true;
        }
    }

    return false;
}

bool
OfxEffectInstance::onOverlayFocusLost(double time,
                                      double scaleX,
                                      double scaleY)
{
    if (!_initialized) {
        return false;
    }
    if (_overlayInteract) {
        OfxPointD rs;
        rs.x = scaleX;
        rs.y = scaleY;
        OfxStatus stat;
        SET_CAN_SET_VALUE(true);
        stat = _overlayInteract->loseFocusAction(time, rs);
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

std::string
OfxEffectInstance::natronValueChangedReasonToOfxValueChangedReason(Natron::ValueChangedReasonEnum reason)
{
    switch (reason) {
        case Natron::eValueChangedReasonUserEdited:
        case Natron::eValueChangedReasonNatronGuiEdited:
        case Natron::eValueChangedReasonSlaveRefresh:
        case Natron::eValueChangedReasonRestoreDefault:
            return kOfxChangeUserEdited;
        case Natron::eValueChangedReasonPluginEdited:
        case Natron::eValueChangedReasonNatronInternalEdited:
            return kOfxChangePluginEdited;
        case Natron::eValueChangedReasonTimeChanged:
            return kOfxChangeTime;
        default:
            assert(false);     // all Natron reasons should be processed
            return "";
    }
}

void
OfxEffectInstance::knobChanged(KnobI* k,
                               Natron::ValueChangedReasonEnum reason,
                               int view,
                               SequenceTime time,
                               bool originatedFromMainThread)
{
    if (!_initialized) {
        return;
    }

    ///If the param changed is a button and the node is disabled don't do anything which might
    ///trigger an analysis
    if ( (reason == eValueChangedReasonUserEdited) && dynamic_cast<KnobButton*>(k) && getNode()->isNodeDisabled() ) {
        return;
    }

    if ( _renderButton && ( k == _renderButton.get() ) ) {
        ///don't do anything since it is handled upstream
        return;
    }


    std::string ofxReason = natronValueChangedReasonToOfxValueChangedReason(reason);
    assert( !ofxReason.empty() ); // crashes when resetting to defaults
    OfxPointD renderScale;
    if (isDoingInteractAction() && _overlayInteract) {
        OverlaySupport* lastInteract = _overlayInteract->getLastCallingViewport();
        assert(lastInteract);
        unsigned int mmLevel = lastInteract->getCurrentRenderScale();
        renderScale.x = renderScale.y = 1 << mmLevel;
    } else {
        renderScale.x = renderScale.y = 1;
    }
    OfxStatus stat = kOfxStatOK;
    
    int recursionLevel = getRecursionLevel();

    if (recursionLevel == 1) {
        SET_CAN_SET_VALUE(true);
        ClipsThreadStorageSetter clipSetter(effectInstance(),
                                            false,
                                            true, //< setView ?
                                            view,
                                            true, //< setmipmaplevel?
                                            0);
        
        ///This action as all the overlay interacts actions can trigger recursive actions, such as
        ///getClipPreferences() so we don't take the clips preferences lock for read here otherwise we would
        ///create a deadlock. This code then assumes that the instance changed action of the plug-in doesn't require
        ///the clip preferences to stay the same throughout the action.
        stat = effectInstance()->paramInstanceChangedAction(k->getOriginalName(), ofxReason,(OfxTime)time,renderScale);
    } else {
        ///This action as all the overlay interacts actions can trigger recursive actions, such as
        ///getClipPreferences() so we don't take the clips preferences lock for read here otherwise we would
        ///create a deadlock. This code then assumes that the instance changed action of the plug-in doesn't require
        ///the clip preferences to stay the same throughout the action.
        stat = effectInstance()->paramInstanceChangedAction(k->getOriginalName(), ofxReason,(OfxTime)time,renderScale);
    }
    
    if ( (stat != kOfxStatOK) && (stat != kOfxStatReplyDefault) ) {
        return;
    }
    
    if (QThread::currentThread() == qApp->thread() &&
        originatedFromMainThread) { //< change didnt occur in main-thread in the first, palce don't attempt to draw the overlay
        
        ///Run the following only in the main-thread

        if ( _effect->isClipPreferencesSlaveParam( k->getOriginalName() ) ) {
            RECURSIVE_ACTION();
            checkOFXClipPreferences_public(time, renderScale, ofxReason,true, true);
        }
        if (_overlayInteract && getNode()->shouldDrawOverlay() && !getNode()->hasDefaultOverlayForParam(k)) {
            // Some plugins (e.g. by digital film tools) forget to set kOfxInteractPropSlaveToParam.
            // Most hosts trigger a redraw if the plugin has an active overlay.
            //if (std::find(_overlaySlaves.begin(), _overlaySlaves.end(), (void*)k) != _overlaySlaves.end()) {
            incrementRedrawNeededCounter();
            //}

            if (recursionLevel == 1 && checkIfOverlayRedrawNeeded()) {
                stat = _overlayInteract->redraw();
                assert(stat == kOfxStatOK || stat == kOfxStatReplyDefault);
            }
        }
    }
} // knobChanged

void
OfxEffectInstance::beginKnobsValuesChanged(Natron::ValueChangedReasonEnum reason)
{
    if (!_initialized) {
        return;
    }
    
    RECURSIVE_ACTION();
    SET_CAN_SET_VALUE(true);
    ///This action as all the overlay interacts actions can trigger recursive actions, such as
    ///getClipPreferences() so we don't take the clips preferences lock for read here otherwise we would
    ///create a deadlock. This code then assumes that the instance changed action of the plug-in doesn't require
    ///the clip preferences to stay the same throughout the action.
    ignore_result(effectInstance()->beginInstanceChangedAction(natronValueChangedReasonToOfxValueChangedReason(reason)));
}

void
OfxEffectInstance::endKnobsValuesChanged(Natron::ValueChangedReasonEnum reason)
{
    if (!_initialized) {
        return;
    }
    
    RECURSIVE_ACTION();
    SET_CAN_SET_VALUE(true);
    ///This action as all the overlay interacts actions can trigger recursive actions, such as
    ///getClipPreferences() so we don't take the clips preferences lock for read here otherwise we would
    ///create a deadlock. This code then assumes that the instance changed action of the plug-in doesn't require
    ///the clip preferences to stay the same throughout the action.
    ignore_result(effectInstance()->endInstanceChangedAction(natronValueChangedReasonToOfxValueChangedReason(reason)));

}

void
OfxEffectInstance::purgeCaches()
{
    // The kOfxActionPurgeCaches is an action that may be passed to a plug-in instance from time to time in low memory situations. Instances recieving this action should destroy any data structures they may have and release the associated memory, they can later reconstruct this from the effect's parameter set and associated information. http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#kOfxActionPurgeCaches
    OfxStatus stat;
    {
        SET_CAN_SET_VALUE(false);
        ///Take the preferences lock so that it cannot be modified throughout the action.
        QReadLocker preferencesLocker(_preferencesLock);
        stat =  _effect->purgeCachesAction();
        
        assert(stat == kOfxStatOK || stat == kOfxStatReplyDefault);
        
    }
    // The kOfxActionSyncPrivateData action is called when a plugin should synchronise any private data structures to its parameter set. This generally occurs when an effect is about to be saved or copied, but it could occur in other situations as well. http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#kOfxActionSyncPrivateData
    
    {
        RECURSIVE_ACTION();
        SET_CAN_SET_VALUE(true);
        ///This action as all the overlay interacts actions can trigger recursive actions, such as
        ///getClipPreferences() so we don't take the clips preferences lock for read here otherwise we would
        ///create a deadlock. This code then assumes that the instance changed action of the plug-in doesn't require
        ///the clip preferences to stay the same throughout the action.
        stat =  _effect->syncPrivateDataAction();
        assert(stat == kOfxStatOK || stat == kOfxStatReplyDefault);
        
    }
    Q_UNUSED(stat);
}

int
OfxEffectInstance::getMajorVersion() const
{
    return effectInstance()->getPlugin()->getVersionMajor();
}

int
OfxEffectInstance::getMinorVersion() const
{
    return effectInstance()->getPlugin()->getVersionMinor();
}

bool
OfxEffectInstance::supportsRenderQuality() const
{
    return effectInstance()->supportsRenderQuality();
}

bool
OfxEffectInstance::supportsTiles() const
{
    // first, check the descriptor, then the instance
    if (!effectInstance()->getDescriptor().supportsTiles() || !effectInstance()->supportsTiles()) {
        return false;
    }

    OFX::Host::ImageEffect::ClipInstance* outputClip =  effectInstance()->getClip(kOfxImageEffectOutputClipName);

    return outputClip->supportsTiles();
}

Natron::PluginOpenGLRenderSupport
OfxEffectInstance::supportsOpenGLRender() const
{
    // first, check the descriptor
    {
        const std::string& str = effectInstance()->getDescriptor().getProps().getStringProperty(kOfxImageEffectPropOpenGLRenderSupported);
        if (str == "false") {
            return ePluginOpenGLRenderSupportNone;
        }
    }
    // then, check the instance
    const std::string& str = effectInstance()->getProps().getStringProperty(kOfxImageEffectPropOpenGLRenderSupported);
    if (str == "false") {
        return ePluginOpenGLRenderSupportNone;
    } else if (str == "true") {
        return ePluginOpenGLRenderSupportYes;
    } else {
        assert(str == "needed");
        return ePluginOpenGLRenderSupportNeeded;
    }
}

bool
OfxEffectInstance::supportsMultiResolution() const
{
    // first, check the descriptor, then the instance
    return effectInstance()->getDescriptor().supportsMultiResolution() && effectInstance()->supportsMultiResolution();
}

void
OfxEffectInstance::beginEditKnobs()
{
    ///Take the preferences lock so that it cannot be modified throughout the action.
    QReadLocker preferencesLocker(_preferencesLock);
    effectInstance()->beginInstanceEditAction();
}

void
OfxEffectInstance::onSyncPrivateDataRequested()
{
    ///Can only be called in the main thread
    assert( QThread::currentThread() == qApp->thread() );
    
    RECURSIVE_ACTION();
    
    ///This action as all the overlay interacts actions can trigger recursive actions, such as
    ///getClipPreferences() so we don't take the clips preferences lock for read here otherwise we would
    ///create a deadlock. This code then assumes that the instance changed action of the plug-in doesn't require
    ///the clip preferences to stay the same throughout the action.
    SET_CAN_SET_VALUE(true);
    effectInstance()->syncPrivateDataAction();
}

void
OfxEffectInstance::addAcceptedComponents(int inputNb,
                                         std::list<Natron::ImageComponents>* comps)
{
    if (inputNb >= 0) {
        OfxClipInstance* clip = getClipCorrespondingToInput(inputNb);
        assert(clip);
        const std::vector<std::string> & supportedComps = clip->getSupportedComponents();
        for (U32 i = 0; i < supportedComps.size(); ++i) {
            try {
                std::list<Natron::ImageComponents> ofxComp = OfxClipInstance::ofxComponentsToNatronComponents(supportedComps[i]);
                comps->insert(comps->end(), ofxComp.begin(), ofxComp.end());
            } catch (const std::runtime_error &e) {
                // ignore unsupported components
            }
        }
    } else {
        assert(inputNb == -1);
        OfxClipInstance* clip = dynamic_cast<OfxClipInstance*>( effectInstance()->getClip(kOfxImageEffectOutputClipName) );
        assert(clip);
        const std::vector<std::string> & supportedComps = clip->getSupportedComponents();
        for (U32 i = 0; i < supportedComps.size(); ++i) {
            try {
                std::list<Natron::ImageComponents> ofxComp = OfxClipInstance::ofxComponentsToNatronComponents(supportedComps[i]);
                comps->insert(comps->end(), ofxComp.begin(), ofxComp.end());
            } catch (const std::runtime_error &e) {
                // ignore unsupported components
            }
        }
    }
}

void
OfxEffectInstance::addSupportedBitDepth(std::list<Natron::ImageBitDepthEnum>* depths) const
{
    const OFX::Host::Property::Set & prop = effectInstance()->getPlugin()->getDescriptor().getParamSetProps();
    int dim = prop.getDimension(kOfxImageEffectPropSupportedPixelDepths);

    for (int i = 0; i < dim; ++i) {
        const std::string & depth = prop.getStringProperty(kOfxImageEffectPropSupportedPixelDepths,i);
        try {
            depths->push_back( OfxClipInstance::ofxDepthToNatronDepth(depth) );
        } catch (const std::runtime_error &e) {
            // ignore unsupported bitdepth
        }
    }
}


void
OfxEffectInstance::getPreferredDepthAndComponents(int inputNb,
                                                  std::list<Natron::ImageComponents>* comp,
                                                  Natron::ImageBitDepthEnum* depth) const
{
    OfxClipInstance* clip;

    if (inputNb == -1) {
        clip = dynamic_cast<OfxClipInstance*>( _effect->getClip(kOfxImageEffectOutputClipName) );
    } else {
        clip = getClipCorrespondingToInput(inputNb);
    }
    assert(clip);
    
    if (getRecursionLevel() > 0) {
        ///Someone took the read  (all actions) or write (getClipPreferences action)lock already
        *comp = OfxClipInstance::ofxComponentsToNatronComponents( clip->getComponents() );
        *depth = OfxClipInstance::ofxDepthToNatronDepth( clip->getPixelDepth() );
    } else {
        ///Take the preferences lock to be sure we're not writing them
        QReadLocker l(_preferencesLock);
        *comp = OfxClipInstance::ofxComponentsToNatronComponents( clip->getComponents() );
        *depth = OfxClipInstance::ofxDepthToNatronDepth( clip->getPixelDepth() );
    }
}

void
OfxEffectInstance::getComponentsNeededAndProduced(double time, int view,
                                            ComponentsNeededMap* comps,
                                            SequenceTime* passThroughTime,
                                            int* passThroughView,
                                            boost::shared_ptr<Natron::Node>* passThroughInput) 
{
    OfxStatus stat ;
    {
        bool skipDiscarding = false;
        if (getRecursionLevel() > 1) {
            skipDiscarding = true;
        }
        
        SET_CAN_SET_VALUE(false);
        
        
        ClipsThreadStorageSetter clipSetter(effectInstance(),
                                            skipDiscarding,
                                            true, //< setView ?
                                            view,
                                            false,//< set mipmaplevel ?
                                            0);
        
        
        OFX::Host::ImageEffect::ComponentsMap compMap;
        OFX::Host::ImageEffect::ClipInstance* ptClip = 0;
        OfxTime ptTime;
        stat = effectInstance()->getClipComponentsAction((OfxTime)time, view, compMap, ptClip, ptTime, *passThroughView);
        if (stat != kOfxStatFailed) {
            
            if (ptClip) {
                OfxClipInstance* clip = dynamic_cast<OfxClipInstance*>(ptClip);
                if (clip && clip->getAssociatedNode()) {
                    *passThroughInput = clip->getAssociatedNode()->getNode();
                }
            }
            *passThroughTime = (SequenceTime)ptTime;
            
            for (OFX::Host::ImageEffect::ComponentsMap::iterator it = compMap.begin(); it != compMap.end(); ++it) {
                
                OfxClipInstance* clip = dynamic_cast<OfxClipInstance*>(it->first);
                assert(clip);
                int index = clip->getInputNb();
                
                std::vector<Natron::ImageComponents> compNeeded;
                for (std::list<std::string>::iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
                    std::list<Natron::ImageComponents> ofxComp = OfxClipInstance::ofxComponentsToNatronComponents(*it2);
                    compNeeded.insert(compNeeded.end(), ofxComp.begin(), ofxComp.end());
                }
                comps->insert(std::make_pair(index, compNeeded));
            }
        }
        
    } // ClipsThreadStorageSetter
    

}

bool
OfxEffectInstance::isMultiPlanar() const
{
    return effectInstance()->isMultiPlanar();
}

EffectInstance::PassThroughEnum
OfxEffectInstance::isPassThroughForNonRenderedPlanes() const
{
    OFX::Host::ImageEffect::Base::OfxPassThroughLevelEnum pt = effectInstance()->getPassThroughForNonRenderedPlanes();
    switch (pt) {
        case OFX::Host::ImageEffect::Base::ePassThroughLevelEnumBlockAllNonRenderedPlanes:
            return EffectInstance::ePassThroughBlockNonRenderedPlanes;
        case OFX::Host::ImageEffect::Base::ePassThroughLevelEnumPassThroughAllNonRenderedPlanes:
            return EffectInstance::ePassThroughPassThroughNonRenderedPlanes;
        case OFX::Host::ImageEffect::Base::ePassThroughLevelEnumRenderAllRequestedPlanes:
            return EffectInstance::ePassThroughRenderAllRequestedPlanes;
    }
    return EffectInstance::ePassThroughBlockNonRenderedPlanes;
}

bool
OfxEffectInstance::isViewAware() const
{
    return effectInstance()->isViewAware();
}

EffectInstance::ViewInvarianceLevel
OfxEffectInstance::isViewInvariant() const
{
    int inv = effectInstance()->getViewInvariance();
    if (inv == 0) {
        return eViewInvarianceAllViewsVariant;
    } else if (inv == 1) {
        return eViewInvarianceOnlyPassThroughPlanesVariant;
    } else {
        assert(inv == 2);
        return eViewInvarianceAllViewsInvariant;
    }
 }

Natron::SequentialPreferenceEnum
OfxEffectInstance::getSequentialPreference() const
{
    int sequential = _effect->getPlugin()->getDescriptor().getProps().getIntProperty(kOfxImageEffectInstancePropSequentialRender);

    switch (sequential) {
    case 0:

        return Natron::eSequentialPreferenceNotSequential;
    case 1:

        return Natron::eSequentialPreferenceOnlySequential;
    case 2:

        return Natron::eSequentialPreferencePreferSequential;
    default:

        return Natron::eSequentialPreferenceNotSequential;
        break;
    }
}

Natron::ImagePremultiplicationEnum
OfxEffectInstance::getOutputPremultiplication() const
{
    const std::string & str = ofxGetOutputPremultiplication();

    if (str == kOfxImagePreMultiplied) {
        return Natron::eImagePremultiplicationPremultiplied;
    } else if (str == kOfxImageUnPreMultiplied) {
        return Natron::eImagePremultiplicationUnPremultiplied;
    } else {
        return Natron::eImagePremultiplicationOpaque;
    }
}

const std::string &
OfxEffectInstance::ofxGetOutputPremultiplication() const
{
    static const std::string v(kOfxImagePreMultiplied);
    assert(effectInstance()->getClip(kOfxImageEffectOutputClipName));
    
    if (getRecursionLevel() > 0) {
        const std::string & premult = effectInstance()->getOutputPreMultiplication();
        ///if the output has something, use it, otherwise default to premultiplied
        if ( !premult.empty() ) {
            return premult;
        } else {
            return v;
        }
    } else {
        ///Take the preferences lock to be sure we're not writing them
        QReadLocker l(_preferencesLock);
        const std::string & premult = effectInstance()->getOutputPreMultiplication();
        ///if the output has something, use it, otherwise default to premultiplied
        if ( !premult.empty() ) {
            return premult;
        } else {
            return v;
        }
    }
}


double
OfxEffectInstance::getPreferredAspectRatio() const
{
    OFX::Host::ImageEffect::ClipInstance* clip = effectInstance()->getClip(kOfxImageEffectOutputClipName);
    assert(clip);
    
    if (getRecursionLevel() > 0) {
        return clip->getAspectRatio();
    } else {
        ///Take the preferences lock to be sure we're not writing them
        QReadLocker l(_preferencesLock);
        return clip->getAspectRatio();

    }
}

double
OfxEffectInstance::getPreferredFrameRate() const
{
    OFX::Host::ImageEffect::ClipInstance* clip = effectInstance()->getClip(kOfxImageEffectOutputClipName);
    assert(clip);
    
    if (getRecursionLevel() > 0) {
        return clip->getFrameRate();
    } else {
        ///Take the preferences lock to be sure we're not writing them
        QReadLocker l(_preferencesLock);
        return clip->getFrameRate();
        
    }
}


bool
OfxEffectInstance::getCanTransform() const
{   //use OFX_EXTENSIONS_NUKE
    return effectInstance()->canTransform();
}

bool
OfxEffectInstance::getInputsHoldingTransform(std::list<int>* inputs) const
{
    return effectInstance()->getInputsHoldingTransform(inputs);
}

Natron::StatusEnum
OfxEffectInstance::getTransform(double time,
                                const RenderScale& renderScale, //< the plug-in accepted scale
                                int view,
                                Natron::EffectInstance** inputToTransform,
                                Transform::Matrix3x3* transform)
{
    assert(getCanTransform());
    const std::string field = kOfxImageFieldNone; // TODO: support interlaced data
    
    std::string clipName;
    double tmpTransform[9];
    
    OfxStatus stat ;
    {
        bool skipDiscarding = false;
        if (getRecursionLevel() > 1) {
            skipDiscarding = true;
        }
        SET_CAN_SET_VALUE(false);
        
        
        ClipsThreadStorageSetter clipSetter(effectInstance(),
                                            skipDiscarding,
                                            true, //< setView ?
                                            view,
                                            true,//< set mipmaplevel ?
                                            Natron::Image::getLevelFromScale(renderScale.x));
        
        
        stat = effectInstance()->getTransformAction((OfxTime)time, field, renderScale, view, clipName, tmpTransform);
        if (stat == kOfxStatReplyDefault) {
            return Natron::eStatusReplyDefault;
        } else if (stat == kOfxStatFailed) {
            return Natron::eStatusFailed;
        }

    }

    
    assert(stat == kOfxStatOK);
    
    transform->a = tmpTransform[0]; transform->b = tmpTransform[1]; transform->c = tmpTransform[2];
    transform->d = tmpTransform[3]; transform->e = tmpTransform[4]; transform->f = tmpTransform[5];
    transform->g = tmpTransform[6]; transform->h = tmpTransform[7]; transform->i = tmpTransform[8];
    
    
    
    OFX::Host::ImageEffect::ClipInstance* clip = effectInstance()->getClip(clipName);
    assert(clip);
    OfxClipInstance* natronClip = dynamic_cast<OfxClipInstance*>(clip);
    if (!natronClip) {
        return Natron::eStatusFailed;
    }
    *inputToTransform = natronClip->getAssociatedNode();
    if (!*inputToTransform) {
        return Natron::eStatusFailed;
    }
    return Natron::eStatusOK;
}

void
OfxEffectInstance::rerouteInputAndSetTransform(const InputMatrixMap& inputTransforms)
{
    for (InputMatrixMap::const_iterator it = inputTransforms.begin(); it != inputTransforms.end(); ++it) {
        OfxClipInstance* clip = getClipCorrespondingToInput(it->first);
        assert(clip);
        clip->setTransformAndReRouteInput(*it->second.cat, it->second.newInputEffect, it->second.newInputNbToFetchFrom);
    }
    
}

void
OfxEffectInstance::clearTransform(int inputNb)
{
    OfxClipInstance* clip = getClipCorrespondingToInput(inputNb);
    assert(clip);
    clip->clearTransform();
}


bool
OfxEffectInstance::isFrameVarying() const
{
    // only check the instance (frameVarying is set by clip prefs)
    return effectInstance()->isFrameVarying();
}

bool
OfxEffectInstance::doesTemporalClipAccess() const
{
    // first, check the descriptor, then the instance
    return effectInstance()->getDescriptor().temporalAccess() && effectInstance()->temporalAccess();
}

bool
OfxEffectInstance::isHostChannelSelectorSupported(bool* defaultR,bool* defaultG, bool* defaultB, bool* defaultA) const
{
    std::string defaultChannels = effectInstance()->getProps().getStringProperty(kNatronOfxImageEffectPropChannelSelector);
    if (defaultChannels == kOfxImageComponentNone) {
        return false;
    }
    if (defaultChannels == kOfxImageComponentRGBA) {
        *defaultR = *defaultG = *defaultB = *defaultA = true;
    } else if (defaultChannels == kOfxImageComponentRGB) {
        *defaultR = *defaultG = *defaultB = true;
        *defaultA = false;
    } else if (defaultChannels == kOfxImageComponentAlpha) {
        *defaultR = *defaultG = *defaultB = false;
        *defaultA = true;
    } else {
        qDebug() << getScriptName_mt_safe().c_str() <<"Invalid value given to property" << kNatronOfxImageEffectPropChannelSelector << "defaulting to RGBA checked";
        *defaultR = *defaultG = *defaultB = *defaultA = true;
    }
    return true;
}

bool
OfxEffectInstance::isHostMaskingEnabled() const
{
    return effectInstance()->isHostMaskingEnabled();
}

bool
OfxEffectInstance::isHostMixingEnabled() const
{
    return effectInstance()->isHostMixingEnabled();
}

int
OfxEffectInstance::getClipInputNumber(const OfxClipInstance* clip) const
{
    
    for (std::size_t i = 0; i < _clipsInfos.size(); ++i) {
        if (_clipsInfos[i].clip == clip) {
            return (int)i;
        }
    }
    if (clip == _outputClip) {
        return -1;
    }
    return 0;
}
