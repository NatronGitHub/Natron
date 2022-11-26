/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2022 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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
#include <cassert>
#include <stdexcept>

#include <QtCore/QDebug>
#include <QtCore/QByteArray>
#include <QtCore/QReadWriteLock>
#include <QtCore/QPointF>

// ofxhPropertySuite.h:565:37: warning: 'this' pointer cannot be null in well-defined C++ code; comparison may be assumed to always evaluate to true [-Wtautological-undefined-compare]
// clang-format off
CLANG_DIAG_OFF(unknown-pragmas)
CLANG_DIAG_OFF(tautological-undefined-compare) // appeared in clang 3.5
#include <ofxhPluginCache.h>
#include <ofxhPluginAPICache.h>
#include <ofxhImageEffect.h>
#include <ofxhImageEffectAPI.h>
#include <ofxOpenGLRender.h>
#include <ofxhHost.h>
CLANG_DIAG_ON(tautological-undefined-compare)
CLANG_DIAG_ON(unknown-pragmas)
// clang-format on

#include <tuttle/ofxReadWrite.h>
#include <ofxNatron.h>
#include <nuke/fnOfxExtensions.h>

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/KnobFile.h"
#include "Engine/KnobTypes.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/Node.h"
#include "Engine/NodeSerialization.h"
#include "Engine/NodeMetadata.h"
#include "Engine/OfxClipInstance.h"
#include "Engine/OfxImageEffectInstance.h"
#include "Engine/OfxOverlayInteract.h"
#include "Engine/OfxParamInstance.h"
#include "Engine/Project.h"
#include "Engine/ReadNode.h"
#include "Engine/RotoLayer.h"
#include "Engine/TimeLine.h"
#include "Engine/Transform.h"
#include "Engine/UndoCommand.h"
#include "Engine/ViewIdx.h"
#include "Engine/ViewerInstance.h"
#include "Engine/WriteNode.h"
#ifdef DEBUG
#include "Engine/TLSHolder.h"
#endif


NATRON_NAMESPACE_ENTER

using std::cout; using std::endl; using std::string;


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
                             ViewIdx view,
                             unsigned mipmapLevel)
        : effect(effect)
    {
        const std::map<std::string, OFX::Host::ImageEffect::ClipInstance*>& clips = effect->getClips();

        for (std::map<std::string, OFX::Host::ImageEffect::ClipInstance*>::const_iterator it = clips.begin(); it != clips.end(); ++it) {
            OfxClipInstance* clip = dynamic_cast<OfxClipInstance*>(it->second);
            assert(clip);
            if (clip) {
                clip->setClipTLS( view, mipmapLevel, ImagePlaneDesc::getNoneComponents() );
            }
        }
    }

    virtual ~ClipsThreadStorageSetter()
    {
        const std::map<std::string, OFX::Host::ImageEffect::ClipInstance*>& clips = effect->getClips();

        for (std::map<std::string, OFX::Host::ImageEffect::ClipInstance*>::const_iterator it = clips.begin(); it != clips.end(); ++it) {
            OfxClipInstance* clip = dynamic_cast<OfxClipInstance*>(it->second);
            assert(clip);
            if (clip) {
                clip->invalidateClipTLS();
            }
        }
    }

private:
    OfxImageEffectInstance* effect;
};

class RenderThreadStorageSetter
{
public:

    RenderThreadStorageSetter(OfxImageEffectInstance* effect,
                              ViewIdx view,
                              unsigned int mipmapLevel,
                              const ImagePlaneDesc& currentPlane,
                              const EffectInstance::InputImagesMap& inputImages)
        : effect(effect)
    {
        const std::map<std::string, OFX::Host::ImageEffect::ClipInstance*>& clips = effect->getClips();

        for (std::map<std::string, OFX::Host::ImageEffect::ClipInstance*>::const_iterator it = clips.begin(); it != clips.end(); ++it) {
            OfxClipInstance* clip = dynamic_cast<OfxClipInstance*>(it->second);
            assert(clip);
            if (clip) {
                if ( clip->isOutput() ) {
                    clip->setClipTLS(view, mipmapLevel, currentPlane);
                } else {
                    int inputNb = clip->getInputNb();
                    EffectInstance::InputImagesMap::const_iterator foundClip = inputImages.find(inputNb);

                    if ( ( foundClip != inputImages.end() ) && !foundClip->second.empty() ) {
                        const ImagePtr& img = foundClip->second.front();
                        assert(img);
                        clip->setClipTLS( view, mipmapLevel, img->getComponents() );
                    } else {
                        clip->setClipTLS( view, mipmapLevel, ImagePlaneDesc::getNoneComponents() );
                    }
                }
            }
        }
    }

    virtual ~RenderThreadStorageSetter()
    {
        const std::map<std::string, OFX::Host::ImageEffect::ClipInstance*>& clips = effect->getClips();

        for (std::map<std::string, OFX::Host::ImageEffect::ClipInstance*>::const_iterator it = clips.begin(); it != clips.end(); ++it) {
            OfxClipInstance* clip = dynamic_cast<OfxClipInstance*>(it->second);
            assert(clip);
            if (clip) {
                clip->invalidateClipTLS();
            }
        }
    }

private:
    OfxImageEffectInstance* effect;
};
} // anon namespace

struct OfxEffectInstancePrivate
{
    std::unique_ptr<OfxImageEffectInstance> effect;
    std::string natronPluginID; //< small cache to avoid calls to generateImageEffectClassName
    std::unique_ptr<OfxOverlayInteract> overlayInteract; // ptr to the overlay interact if any
    KnobStringWPtr cursorKnob; // secret knob for ofx effects so they can set the cursor
    KnobIntWPtr selectionRectangleStateKnob;
    KnobStringWPtr undoRedoTextKnob;
    KnobBoolWPtr undoRedoStateKnob;
    mutable QReadWriteLock preferencesLock;
    mutable QReadWriteLock renderSafetyLock;
    mutable RenderSafetyEnum renderSafety;
    mutable bool wasRenderSafetySet;
    ContextEnum context;
    struct ClipsInfo
    {
        ClipsInfo()
            : optional(false)
            , mask(false)
            , rotoBrush(false)
            , clip(NULL)
            , label()
            , hint()
            , visible(true)
        {
        }

        bool optional;
        bool mask;
        bool rotoBrush;
        OfxClipInstance* clip;
        std::string label;
        std::string hint;
        bool visible;
    };

    std::vector<ClipsInfo> clipsInfos;
    OfxClipInstance* outputClip;
    int nbSourceClips;
    SequentialPreferenceEnum sequentialPref;
    mutable QMutex supportsConcurrentGLRendersMutex;
    bool supportsConcurrentGLRenders;
    bool isOutput; //if the OfxNode can output a file somehow
    bool penDown; // true when the overlay trapped a penDow action
    bool created; // true after the call to createInstance
    bool initialized; //true when the image effect instance has been created and populated

    /*
       Some OpenFX do not handle render scale properly when it comes to overlay interacts.
       We try to keep a blacklist of these and call overlay actions with render scale = 1  in that
       case
     */
    bool overlaysCanHandleRenderScale;
    bool supportsMultipleClipPARs;
    bool supportsMultipleClipDepths;
    bool doesTemporalAccess;
    bool multiplanar;

    OfxEffectInstancePrivate()
        : effect()
        , natronPluginID()
        , overlayInteract()
        , cursorKnob()
        , selectionRectangleStateKnob()
        , undoRedoTextKnob()
        , undoRedoStateKnob()
        , preferencesLock(QReadWriteLock::Recursive)
        , renderSafetyLock()
        , renderSafety(eRenderSafetyUnsafe)
        , wasRenderSafetySet(false)
        , context(eContextNone)
        , clipsInfos()
        , outputClip(0)
        , nbSourceClips(0)
        , sequentialPref(eSequentialPreferenceNotSequential)
        , supportsConcurrentGLRendersMutex()
        , supportsConcurrentGLRenders(false)
        , isOutput(false)
        , penDown(false)
        , created(false)
        , initialized(false)
        , overlaysCanHandleRenderScale(true)
        , supportsMultipleClipPARs(false)
        , supportsMultipleClipDepths(false)
        , doesTemporalAccess(false)
        , multiplanar(false)
    {
    }

    OfxEffectInstancePrivate(const OfxEffectInstancePrivate& other)
        : effect()
        , natronPluginID(other.natronPluginID)
        , overlayInteract()
        , preferencesLock(QReadWriteLock::Recursive)
        , renderSafetyLock()
        , renderSafety(other.renderSafety)
        , wasRenderSafetySet(other.wasRenderSafetySet)
        , context(other.context)
        , clipsInfos(other.clipsInfos)
        , outputClip(other.outputClip)
        , nbSourceClips(other.nbSourceClips)
        , sequentialPref(other.sequentialPref)
        , supportsConcurrentGLRendersMutex()
        , supportsConcurrentGLRenders(other.supportsConcurrentGLRenders)
        , isOutput(other.isOutput)
        , penDown(other.penDown)
        , created(other.created)
        , initialized(other.initialized)
        , overlaysCanHandleRenderScale(other.overlaysCanHandleRenderScale)
        , supportsMultipleClipPARs(other.supportsMultipleClipPARs)
        , supportsMultipleClipDepths(other.supportsMultipleClipDepths)
        , doesTemporalAccess(other.doesTemporalAccess)
        , multiplanar(other.multiplanar)
    {
    }
};

OfxEffectInstance::OfxEffectInstance(NodePtr node)
    : AbstractOfxEffectInstance(node)
    , _imp( new OfxEffectInstancePrivate() )
{
    QObject::connect( this, SIGNAL(syncPrivateDataRequested()), this, SLOT(onSyncPrivateDataRequested()) );
}

OfxEffectInstance::OfxEffectInstance(const OfxEffectInstance& other)
    : AbstractOfxEffectInstance(other)
    , _imp( new OfxEffectInstancePrivate(*other._imp) )
{
    QObject::connect( this, SIGNAL(syncPrivateDataRequested()), this, SLOT(onSyncPrivateDataRequested()) );
}

OfxImageEffectInstance*
OfxEffectInstance::effectInstance()
{
    return _imp->effect.get();
}

const OfxImageEffectInstance*
OfxEffectInstance::effectInstance() const
{
    return _imp->effect.get();
}

bool
OfxEffectInstance::isCreated() const
{
    return _imp->created;
}

bool
OfxEffectInstance::isInitialized() const
{
    return _imp->initialized;
}

void
OfxEffectInstance::createOfxImageEffectInstance(OFX::Host::ImageEffect::ImageEffectPlugin* plugin,
                                                OFX::Host::ImageEffect::Descriptor* desc,
                                                ContextEnum context,
                                                const NodeSerialization* serialization,
                                                const CreateNodeArgs& args
#ifndef NATRON_ENABLE_IO_META_NODES
                                                ,
                                                bool allowFileDialogs,
                                                bool *hasUsedFileDialog
#endif
                                                )
{
    /*Replicate of the code in OFX::Host::ImageEffect::ImageEffectPlugin::createInstance.
       We need to pass more parameters to the constructor . That means we cannot
       create it in the virtual function newInstance. Thus we create it before
       instantiating the OfxImageEffect. The problem is that calling OFX::Host::ImageEffect::ImageEffectPlugin::createInstance
       creates the OfxImageEffect and calls populate(). populate() will actually create all OfxClipInstance and OfxParamInstance.
       All these subclasses need a valid pointer to an this. Hence we need to set the pointer to this in
       OfxImageEffect BEFORE calling populate().
     */

    ///Only called from the main thread.
    assert( QThread::currentThread() == qApp->thread() );
    assert(plugin && desc && context != eContextNone);


    _imp->context = context;

    if (context == eContextWriter) {
        _imp->isOutput = true;
    }
    if (context == eContextWriter || context == eContextReader) {
        // Writers don't support render scale (full-resolution images are written to disk)
        // Readers don't support render scale otherwise each mipmap level would require a file decoding
        setSupportsRenderScaleMaybe(eSupportsNo);
    }

    std::string images;

    try {
        _imp->effect.reset( new OfxImageEffectInstance(plugin, *desc, mapContextToString(context), false) );
        assert(_imp->effect);

        OfxEffectInstancePtr thisShared = std::dynamic_pointer_cast<OfxEffectInstance>( shared_from_this() );
        _imp->effect->setOfxEffectInstance(thisShared);

        _imp->natronPluginID = plugin->getIdentifier();

        OfxEffectInstance::MappedInputV clips = inputClipsCopyWithoutOutput();
        _imp->nbSourceClips = (int)clips.size();

        _imp->clipsInfos.resize( clips.size() );
        for (unsigned i = 0; i < clips.size(); ++i) {
            OfxEffectInstancePrivate::ClipsInfo info;
            info.optional = clips[i]->isOptional() || info.rotoBrush;
            info.mask = clips[i]->isMask();
            info.rotoBrush = clips[i]->getName() == CLIP_OFX_ROTO && getNode()->isRotoNode();
            info.clip = NULL;
            // label, hint, visible are set below
            _imp->clipsInfos[i] = info;
        }

        getNode()->refreshAcceptedBitDepths();
        _imp->supportsMultipleClipPARs = _imp->effect->supportsMultipleClipPARs();
        _imp->supportsMultipleClipDepths = _imp->effect->supportsMultipleClipDepths();
        _imp->doesTemporalAccess = _imp->effect->temporalAccess();
        _imp->multiplanar = _imp->effect->isMultiPlanar();
        int sequential = _imp->effect->getPlugin()->getDescriptor().getProps().getIntProperty(kOfxImageEffectInstancePropSequentialRender);
        switch (sequential) {
        case 0:
            _imp->sequentialPref = eSequentialPreferenceNotSequential;
            break;
        case 1:
            _imp->sequentialPref = eSequentialPreferenceOnlySequential;
            break;
        case 2:
            _imp->sequentialPref = eSequentialPreferencePreferSequential;
            break;
        default:
            _imp->sequentialPref =  eSequentialPreferenceNotSequential;
            break;
        }

        beginChanges();
        OfxStatus stat;
        {
            SET_CAN_SET_VALUE(true);

            ///Create clips & parameters
            stat = _imp->effect->populate();


            for (unsigned i = 0; i < clips.size(); ++i) {
                _imp->clipsInfos[i].clip = dynamic_cast<OfxClipInstance*>( _imp->effect->getClip( clips[i]->getName() ) );
                _imp->clipsInfos[i].label = _imp->clipsInfos[i].clip->getLabel();
                _imp->clipsInfos[i].hint = _imp->clipsInfos[i].clip->getHint();
                _imp->clipsInfos[i].visible = !_imp->clipsInfos[i].clip->isSecret();
                assert(_imp->clipsInfos[i].clip);
            }

            _imp->outputClip = dynamic_cast<OfxClipInstance*>( _imp->effect->getClip(kOfxImageEffectOutputClipName) );
            assert(_imp->outputClip);

            _imp->effect->addParamsToTheirParents();

            int nPages = _imp->effect->getDescriptor().getProps().getDimension(kOfxPluginPropParamPageOrder);
            std::list<std::string> pagesOrder;
            for (int i = 0; i < nPages; ++i) {
                const std::string& pageName = _imp->effect->getDescriptor().getProps().getStringProperty(kOfxPluginPropParamPageOrder, i);
                pagesOrder.push_back(pageName);
            }
            if ( !pagesOrder.empty() ) {
                getNode()->setPagesOrder(pagesOrder);
            }

            if (stat != kOfxStatOK) {
                throw std::runtime_error("Error while populating the Ofx image effect");
            }
            assert( _imp->effect->getPlugin() );
            assert( _imp->effect->getPlugin()->getPluginHandle() );
            assert( _imp->effect->getPlugin()->getPluginHandle()->getOfxPlugin() );
            assert(_imp->effect->getPlugin()->getPluginHandle()->getOfxPlugin()->mainEntry);

            getNode()->createRotoContextConditionnally();

            getNode()->initializeInputs();
            getNode()->initializeKnobs(serialization != 0);

            {
                KnobIPtr foundCursorKnob = getKnobByName(kNatronOfxParamCursorName);
                if (foundCursorKnob) {
                    KnobStringPtr isStringKnob = std::dynamic_pointer_cast<KnobString>(foundCursorKnob);
                    _imp->cursorKnob = isStringKnob;
                }
            }
            
            {

                KnobIPtr foundSelKnob = getKnobByName(kNatronOfxImageEffectSelectionRectangle);
                if (foundSelKnob) {
                    KnobIntPtr isIntKnob = std::dynamic_pointer_cast<KnobInt>(foundSelKnob);
                    _imp->selectionRectangleStateKnob = isIntKnob;
                }
            }
            {
                KnobIPtr foundTextKnob = getKnobByName(kNatronOfxParamUndoRedoText);
                if (foundTextKnob) {
                    KnobStringPtr isStringKnob = std::dynamic_pointer_cast<KnobString>(foundTextKnob);
                    _imp->undoRedoTextKnob = isStringKnob;
                }
            }
            {
                KnobIPtr foundUndoRedoKnob = getKnobByName(kNatronOfxParamUndoRedoState);
                if (foundUndoRedoKnob) {
                    KnobBoolPtr isBool = std::dynamic_pointer_cast<KnobBool>(foundUndoRedoKnob);
                    _imp->undoRedoStateKnob = isBool;
                }
            }
            ///before calling the createInstanceAction, load values
            if ( serialization && !serialization->isNull() ) {
                getNode()->loadKnobs(*serialization);
            }

            getNode()->setValuesFromSerialization(args);
            

#ifndef NATRON_ENABLE_IO_META_NODES
            //////////////////////////////////////////////////////
            ///////For READERS & WRITERS only we open an image file dialog
            if ( !getApp()->isCreatingPythonGroup() && allowFileDialogs && isReader() && !( serialization && !serialization->isNull() ) && paramValues.empty() ) {
                images = getApp()->openImageFileDialog();
            } else if ( !getApp()->isCreatingPythonGroup() && allowFileDialogs && isWriter() && !( serialization && !serialization->isNull() )  && paramValues.empty() ) {
                images = getApp()->saveImageFileDialog();
            }
            if ( !images.empty() ) {
                *hasUsedFileDialog = true;
                KnobSerializationPtr defaultFile = createDefaultValueForParam(kOfxImageEffectFileParamName, images);
                CreateNodeArgs::DefaultValuesList list;
                list.push_back(defaultFile);

                std::string canonicalFilename = images;
                getApp()->getProject()->canonicalizePath(canonicalFilename);
                int firstFrame, lastFrame;
                Node::getOriginalFrameRangeForReader(getPluginID(), canonicalFilename, &firstFrame, &lastFrame);
                list.push_back( createDefaultValueForParam(kReaderParamNameOriginalFrameRange, firstFrame, lastFrame) );
                getNode()->setValuesFromSerialization(list);
            }
            //////////////////////////////////////////////////////
#endif

            ///Set default metadata since the OpenFX plug-in may fetch images in its constructor
            setDefaultMetadata();

            {
                ///Take the preferences lock so that it cannot be modified throughout the action.
                QReadLocker preferencesLocker(&_imp->preferencesLock);
                stat = _imp->effect->createInstanceAction();
            }
            _imp->created = true;
        } // SET_CAN_SET_VALUE(true);


        if ( (stat != kOfxStatOK) && (stat != kOfxStatReplyDefault) ) {
            QString message;
            int type;
            NodePtr messageContainer = getNode();
#ifdef NATRON_ENABLE_IO_META_NODES
            NodePtr ioContainer = messageContainer->getIOContainer();
            if (ioContainer) {
                messageContainer = ioContainer;
            }
#endif
            messageContainer->getPersistentMessage(&message, &type);
            if (message.isEmpty()) {
                throw std::runtime_error("Could not create effect instance for plugin");
            } else {
                throw std::runtime_error(message.toStdString());
            }
        }

        OfxPointD scaleOne;
        scaleOne.x = 1.;
        scaleOne.y = 1.;
        // Try to set renderscale support at plugin creation.
        // This is not always possible (e.g. if a param has a wrong value).
        if (supportsRenderScaleMaybe() == eSupportsMaybe) {
            // does the effect support renderscale?
            double first = std::numeric_limits<int>::min(), last = std::numeric_limits<int>::max();
            getFrameRange(&first, &last);
            if ( (first == std::numeric_limits<int>::min()) || (last == std::numeric_limits<int>::max()) ) {
                first = last = getApp()->getTimeLine()->currentFrame();
            }
            ClipsThreadStorageSetter clipSetter(effectInstance(),
                                                ViewIdx(0),
                                                0);
            double time = first;
            OfxRectD rod;
            OfxStatus rodstat = _imp->effect->getRegionOfDefinitionAction(time, scaleOne, 0, rod);
            if ( (rodstat == kOfxStatOK) || (rodstat == kOfxStatReplyDefault) ) {
                OfxPointD scale;
                scale.x = 0.5;
                scale.y = 0.5;
                rodstat = _imp->effect->getRegionOfDefinitionAction(time, scale, 0, rod);
                if ( (rodstat == kOfxStatOK) || (rodstat == kOfxStatReplyDefault) ) {
                    setSupportsRenderScaleMaybe(eSupportsYes);
                } else {
                    setSupportsRenderScaleMaybe(eSupportsNo);
                }
            }
        }


        if ( isReader() && serialization && !serialization->isNull() ) {
            getNode()->refreshCreatedViews(true /*silent*/);
        }
    } catch (const std::exception & e) {
        qDebug() << "Error: Caught exception while creating OfxImageEffectInstance" << ": " << e.what();
        _imp->effect.reset();
        throw;
    } catch (...) {
        qDebug() << "Error: Caught exception while creating OfxImageEffectInstance";
        _imp->effect.reset();
        throw;
    }

    _imp->initialized = true;

    endChanges();
} // createOfxImageEffectInstance

OfxEffectInstance::~OfxEffectInstance()
{
    _imp->overlayInteract.reset();
    if (_imp->effect) {
        _imp->effect->destroyInstanceAction();
    }
}

EffectInstancePtr
OfxEffectInstance::createRenderClone()
{
    OfxEffectInstancePtr clone( new OfxEffectInstance(*this) );

    clone->_imp->effect.reset( new OfxImageEffectInstance(*_imp->effect) );
    assert(clone->_imp->effect);
    clone->_imp->effect->setOfxEffectInstance(clone);

    OfxStatus stat;
    {
        ///Take the preferences lock so that it cannot be modified throughout the action.
        QReadLocker preferencesLocker(&clone->_imp->preferencesLock);
        stat = clone->_imp->effect->createInstanceAction();
    }

    if ( (stat != kOfxStatOK) && (stat != kOfxStatReplyDefault) ) {
        // Failed to create clone...
        return EffectInstancePtr();
    }


    return clone;
}

bool
OfxEffectInstance::isEffectCreated() const
{
    return _imp->created;
}

bool
OfxEffectInstance::isPluginDescriptionInMarkdown() const
{
    assert(_imp->context != eContextNone);
    if ( effectInstance() ) {
        return effectInstance()->getProps().getIntProperty(kNatronOfxPropDescriptionIsMarkdown);
    } else {
        return false;
    }
}

std::string
OfxEffectInstance::getPluginDescription() const
{
    assert(_imp->context != eContextNone);
    if ( effectInstance() ) {
        return effectInstance()->getProps().getStringProperty(kOfxPropPluginDescription);
    } else {
        return "";
    }
}

void
OfxEffectInstance::tryInitializeOverlayInteracts()
{
    assert(_imp->context != eContextNone);
    if (_imp->overlayInteract) {
        // already created
        return;
    }

    QString pluginID = QString::fromUtf8( getPluginID().c_str() );
    /*
       Currently genarts plug-ins do not handle render scale properly for overlays
     */
    if ( pluginID.startsWith( QString::fromUtf8("com.genarts.") ) ) {
        _imp->overlaysCanHandleRenderScale = false;
    }

    /*create overlay instance if any*/
    assert(_imp->effect);
    OfxPluginEntryPoint *overlayEntryPoint = _imp->effect->getOverlayInteractMainEntry();
    if (overlayEntryPoint) {
        _imp->overlayInteract.reset( new OfxOverlayInteract(*_imp->effect, 8, true) );
        double sx, sy;
        effectInstance()->getRenderScaleRecursive(sx, sy);
        RenderScale s(sx, sy);

        {
            ClipsThreadStorageSetter clipSetter(effectInstance(),
                                                ViewIdx(0),
                                                0);


            {
                SET_CAN_SET_VALUE(true);
                ///Take the preferences lock so that it cannot be modified throughout the action.
                QReadLocker preferencesLocker(&_imp->preferencesLock);
                _imp->overlayInteract->createInstanceAction();
            }
        }

        ///Fetch all parameters that are overlay slave
        std::vector<std::string> slaveParams;
        _imp->overlayInteract->getSlaveToParam(slaveParams);
        for (U32 i = 0; i < slaveParams.size(); ++i) {
            KnobIPtr param;
            const std::vector<KnobIPtr> & knobs = getKnobs();
            for (std::vector<KnobIPtr>::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
                if ( (*it)->getOriginalName() == slaveParams[i] ) {
                    param = *it;
                    break;
                }
            }
            if (!param) {
                qDebug() << "OfxEffectInstance::tryInitializeOverlayInteracts(): slaveToParam " << slaveParams[i].c_str() << " not available";
            } else {
                addOverlaySlaveParam(param);
            }
        }

        //For multi-instances, redraw is already taken care of by the GUI
        if ( !getNode()->getParentMultiInstance() ) {
            getApp()->redrawAllViewers();
        }
    }


    ///for each param, if it has a valid custom interact, create it
    const std::list<OFX::Host::Param::Instance*> & params = effectInstance()->getParamList();
    for (std::list<OFX::Host::Param::Instance*>::const_iterator it = params.begin(); it != params.end(); ++it) {
        OfxParamToKnob* paramToKnob = dynamic_cast<OfxParamToKnob*>(*it);
        assert(paramToKnob);
        if (!paramToKnob) {
            continue;
        }

        OfxPluginEntryPoint* interactEntryPoint = paramToKnob->getCustomOverlayInteractEntryPoint(*it);
        if (!interactEntryPoint) {
            continue;
        }
        KnobIPtr knob = paramToKnob->getKnob();
        const OFX::Host::Property::PropSpec* interactDescProps = OfxImageEffectInstance::getOfxParamOverlayInteractDescProps();
        OFX::Host::Interact::Descriptor &interactDesc = paramToKnob->getInteractDesc();
        interactDesc.getProperties().addProperties(interactDescProps);
        interactDesc.setEntryPoint(interactEntryPoint);
#pragma message WARN("FIXME: bitdepth and hasalpha are probably wrong")
        interactDesc.describe(/*bitdepthPerComponent=*/ 8, /*hasAlpha=*/ false);
        OfxParamOverlayInteractPtr overlayInteract( new OfxParamOverlayInteract( knob.get(), interactDesc, effectInstance()->getHandle()) );
        knob->setCustomInteract(overlayInteract);
        overlayInteract->createInstanceAction();
    }
} // OfxEffectInstance::tryInitializeOverlayInteracts

void
OfxEffectInstance::setInteractColourPicker(const OfxRGBAColourD& color, bool setColor, bool hasColor)
{
    if (!_imp->overlayInteract) {
        return;
    }

    if (!_imp->overlayInteract->isColorPickerRequired()) {
        return;
    }
    if (!hasColor) {
        _imp->overlayInteract->setHasColorPicker(false);
    } else {
        if (setColor) {
            _imp->overlayInteract->setLastColorPickerColor(color);
        }
        _imp->overlayInteract->setHasColorPicker(true);
    }

    ignore_result(_imp->overlayInteract->redraw());
}

bool
OfxEffectInstance::isOutput() const
{
    assert(_imp->context != eContextNone);

    return _imp->isOutput;
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
    assert(_imp->context != eContextNone);

    return _imp->context == eContextReader;
}

bool
OfxEffectInstance::isVideoReader() const
{
    return isReader() && ReadNode::isVideoReader( getPluginID() );
}


bool
OfxEffectInstance::isVideoWriter() const
{
    return isWriter() && WriteNode::isVideoWriter( getPluginID() );
}

bool
OfxEffectInstance::isWriter() const
{
    assert(_imp->context != eContextNone);

    return _imp->context == eContextWriter;
}

bool
OfxEffectInstance::isTrackerNodePlugin() const
{
    assert(_imp->context != eContextNone);

    return _imp->context == eContextTracker;
}

bool
OfxEffectInstance::isFilter() const
{
    assert(_imp->context != eContextNone);
    const std::set<std::string> & contexts = effectInstance()->getPlugin()->getContexts();
    bool foundGeneral = contexts.find(kOfxImageEffectContextGeneral) != contexts.end();
    bool foundFilter = contexts.find(kOfxImageEffectContextFilter) != contexts.end();

    return foundFilter || (foundGeneral && getNInputs() > 0);
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
    std::string stdIdentifier = pluginIdentifier.toStdString();

    s.replace( QLatin1Char('\\'), QLatin1Char('/') );

    QStringList out;
    if ( ( pluginIdentifier.startsWith( QString::fromUtf8("com.genarts.sapphire.") ) || s.startsWith( QString::fromUtf8("Sapphire ") ) || s.startsWith( QString::fromUtf8(" Sapphire ") ) ) &&
         !s.startsWith( QString::fromUtf8("Sapphire/") ) ) {
        out.push_back( QString::fromUtf8("Sapphire") );
    } else if ( ( pluginIdentifier.startsWith( QString::fromUtf8("com.genarts.monsters.") ) || s.startsWith( QString::fromUtf8("Monsters ") ) || s.startsWith( QString::fromUtf8(" Monsters ") ) ) &&
                !s.startsWith( QString::fromUtf8("Monsters/") ) ) {
        out.push_back( QString::fromUtf8("Monsters") );
    } else if ( ( pluginIdentifier == QString::fromUtf8("uk.co.thefoundry.keylight.keylight") ) ||
                ( pluginIdentifier == QString::fromUtf8("jp.co.ise.imagica:PrimattePlugin") ) ) {
        s = QString::fromUtf8(PLUGIN_GROUP_KEYER);
    } else if ( ( pluginIdentifier == QString::fromUtf8("uk.co.thefoundry.noisetools.denoise") ) ||
                pluginIdentifier.startsWith( QString::fromUtf8("com.rubbermonkey:FilmConvert") ) ) {
        s = QString::fromUtf8(PLUGIN_GROUP_FILTER);
    } else if ( pluginIdentifier.startsWith( QString::fromUtf8("com.NewBlue.Titler") ) ) {
        s = QString::fromUtf8(PLUGIN_GROUP_PAINT);
    } else if ( pluginIdentifier.startsWith( QString::fromUtf8("com.FXHOME.HitFilm") ) ) {
        // HitFilm uses grouping such as "HitFilm - Keying - Matte Enhancement"
        s.replace( QString::fromUtf8(" - "), QString::fromUtf8("/") );
    } else if ( pluginIdentifier.startsWith( QString::fromUtf8("com.redgiantsoftware.Universe") ) && s.startsWith( QString::fromUtf8("Universe ") ) ) {
        // Red Giant Universe uses grouping such as "Universe Blur"
        out.push_back( QString::fromUtf8("Universe") );
    } else if ( pluginIdentifier.startsWith( QString::fromUtf8("com.NewBlue.") ) && s.startsWith( QString::fromUtf8("NewBlue ") ) ) {
        // NewBlueFX uses grouping such as "NewBlue Elements"
        out.push_back( QString::fromUtf8("NewBlue") );
    } else if ( (stdIdentifier == "tuttle.avreader") ||
                (stdIdentifier == "tuttle.avwriter") ||
                (stdIdentifier == "tuttle.dpxwriter") ||
                (stdIdentifier == "tuttle.exrreader") ||
                (stdIdentifier == "tuttle.exrwriter") ||
                (stdIdentifier == "tuttle.imagemagickreader") ||
                (stdIdentifier == "tuttle.jpeg2000reader") ||
                (stdIdentifier == "tuttle.jpeg2000writer") ||
                (stdIdentifier == "tuttle.jpegreader") ||
                (stdIdentifier == "tuttle.jpegwriter") ||
                (stdIdentifier == "tuttle.oiioreader") ||
                (stdIdentifier == "tuttle.oiiowriter") ||
                (stdIdentifier == "tuttle.pngreader") ||
                (stdIdentifier == "tuttle.pngwriter") ||
                (stdIdentifier == "tuttle.rawreader") ||
                (stdIdentifier == "tuttle.turbojpegreader") ||
                (stdIdentifier == "tuttle.turbojpegwriter") ) {
        out.push_back( QString::fromUtf8(PLUGIN_GROUP_IMAGE) );
        if ( pluginIdentifier.endsWith( QString::fromUtf8("reader") ) ) {
            s = QString::fromUtf8(PLUGIN_GROUP_IMAGE_READERS);
        } else {
            s = QString::fromUtf8(PLUGIN_GROUP_IMAGE_WRITERS);
        }
    } else if ( (stdIdentifier == "tuttle.checkerboard") ||
                (stdIdentifier == "tuttle.colorbars") ||
                (stdIdentifier == "tuttle.colorcube") || // TuttleColorCube
                (stdIdentifier == "tuttle.colorgradient") ||
                (stdIdentifier == "tuttle.colorwheel") ||
                (stdIdentifier == "tuttle.constant") ||
                (stdIdentifier == "tuttle.inputbuffer") ||
                (stdIdentifier == "tuttle.outputbuffer") ||
                (stdIdentifier == "tuttle.ramp") ||
                (stdIdentifier == "tuttle.seexpr") ) {
        s = QString::fromUtf8(PLUGIN_GROUP_IMAGE);
    } else if ( (stdIdentifier == "tuttle.bitdepth") ||
                (stdIdentifier == "tuttle.colorgradation") ||
                (stdIdentifier == "tuttle.colorspace") ||
                (stdIdentifier == "tuttle.colorsuppress") ||
                (stdIdentifier == "tuttle.colortransfer") ||
                (stdIdentifier == "tuttle.colortransform") ||
                (stdIdentifier == "tuttle.ctl") ||
                (stdIdentifier == "tuttle.invert") ||
                (stdIdentifier == "tuttle.lut") ||
                (stdIdentifier == "tuttle.normalize") ) {
        s = QString::fromUtf8(PLUGIN_GROUP_COLOR);
    } else if ( (stdIdentifier == "tuttle.ocio.colorspace") ||
                (stdIdentifier == "tuttle.ocio.lut") ) {
        out.push_back( QString::fromUtf8(PLUGIN_GROUP_COLOR) );
        s = QString::fromUtf8("OCIO");
    } else if ( (stdIdentifier == "tuttle.gamma") ||
                (stdIdentifier == "tuttle.mathoperator") ) {
        out.push_back( QString::fromUtf8(PLUGIN_GROUP_COLOR) );
        s = QString::fromUtf8("Math");
    } else if ( (stdIdentifier == "tuttle.channelshuffle") ) {
        s = QString::fromUtf8(PLUGIN_GROUP_CHANNEL);
    } else if ( (stdIdentifier == "tuttle.component") ||
                (stdIdentifier == "tuttle.fade") ||
                (stdIdentifier == "tuttle.merge") ) {
        s = QString::fromUtf8(PLUGIN_GROUP_MERGE);
    } else if ( (stdIdentifier == "tuttle.anisotropicdiffusion") ||
                (stdIdentifier == "tuttle.anisotropictensors") ||
                (stdIdentifier == "tuttle.blur") ||
                (stdIdentifier == "tuttle.convolution") ||
                (stdIdentifier == "tuttle.floodfill") ||
                (stdIdentifier == "tuttle.localmaxima") ||
                (stdIdentifier == "tuttle.nlmdenoiser") ||
                (stdIdentifier == "tuttle.sobel") ||
                (stdIdentifier == "tuttle.thinning") ) {
        s = QString::fromUtf8(PLUGIN_GROUP_FILTER);
    } else if ( (stdIdentifier == "tuttle.crop") ||
                (stdIdentifier == "tuttle.flip") ||
                (stdIdentifier == "tuttle.lensdistort") ||
                (stdIdentifier == "tuttle.move2d") ||
                (stdIdentifier == "tuttle.pinning") ||
                (stdIdentifier == "tuttle.pushpixel") ||
                (stdIdentifier == "tuttle.resize") ||
                (stdIdentifier == "tuttle.swscale") ||
                (stdIdentifier == "tuttle.warp") ) {
        s = QString::fromUtf8(PLUGIN_GROUP_TRANSFORM);
    } else if ( (stdIdentifier == "tuttle.timeshift") ) {
        s = QString::fromUtf8(PLUGIN_GROUP_TIME);
    } else if ( (stdIdentifier == "tuttle.text") ) {
        s = QString::fromUtf8(PLUGIN_GROUP_PAINT);
    } else if ( (stdIdentifier == "tuttle.basickeyer") ||
                (stdIdentifier == "tuttle.colorspacekeyer") ||
                (stdIdentifier == "tuttle.histogramkeyer") ||
                (stdIdentifier == "tuttle.idkeyer") ) {
        s = QString::fromUtf8(PLUGIN_GROUP_KEYER);
    } else if ( (stdIdentifier == "tuttle.colorCube") || // TuttleColorCubeViewer
                (stdIdentifier == "tuttle.colorcubeviewer") ||
                (stdIdentifier == "tuttle.diff") ||
                (stdIdentifier == "tuttle.dummy") ||
                (stdIdentifier == "tuttle.histogram") ||
                (stdIdentifier == "tuttle.imagestatistics") ) {
        s = QString::fromUtf8(PLUGIN_GROUP_OTHER);
    } else if ( (stdIdentifier == "tuttle.debugimageeffectapi") ) {
        out.push_back( QString::fromUtf8(PLUGIN_GROUP_OTHER) );
        s = QString::fromUtf8("Test");
    }

    // The following plugins are pretty much useless for use within Natron, keep them in the Tuttle group:

    /*
       (stdIdentifier == "tuttle.print") ||
       (stdIdentifier == "tuttle.viewer") ||
     */
    return out + s.split( QLatin1Char('/') );
} // ofxExtractAllPartsOfGrouping

QStringList
AbstractOfxEffectInstance::makePluginGrouping(const std::string & pluginIdentifier,
                                              int versionMajor,
                                              int versionMinor,
                                              const std::string & pluginLabel,
                                              const std::string & grouping)
{
    //printf("%s,%s\n",pluginLabel.c_str(),grouping.c_str());
    return ofxExtractAllPartsOfGrouping( QString::fromUtf8( pluginIdentifier.c_str() ), versionMajor, versionMinor, QString::fromUtf8( pluginLabel.c_str() ), QString::fromUtf8( grouping.c_str() ) );
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
    assert(_imp->context != eContextNone);

    return _imp->natronPluginID;
}

std::string
OfxEffectInstance::getPluginLabel() const
{
    assert(_imp->context != eContextNone);
    assert(_imp->effect);

    return makePluginLabel( _imp->effect->getDescriptor().getShortLabel(),
                            _imp->effect->getDescriptor().getLabel(),
                            _imp->effect->getDescriptor().getLongLabel() );
}

void
OfxEffectInstance::getPluginGrouping(std::list<std::string>* grouping) const
{
    assert(_imp->context != eContextNone);
    std::string groupStr = effectInstance()->getPluginGrouping();
    std::string label = getPluginLabel();
    const OFX::Host::ImageEffect::ImageEffectPlugin *p = effectInstance()->getPlugin();
    QStringList groups = ofxExtractAllPartsOfGrouping( QString::fromUtf8( p->getIdentifier().c_str() ), p->getVersionMajor(), p->getVersionMinor(), QString::fromUtf8( label.c_str() ), QString::fromUtf8( groupStr.c_str() ) );
    Q_FOREACH(const QString &group, groups) {
        grouping->push_back( group.toStdString() );
    }
}

void
OfxEffectInstance::onClipLabelChanged(int inputNb, const std::string& label)
{
    assert(inputNb >= 0 && inputNb < (int)_imp->clipsInfos.size());
    _imp->clipsInfos[inputNb].label = label;
    getNode()->setInputLabel(inputNb, label);
}

void
OfxEffectInstance::onClipHintChanged(int inputNb, const std::string& hint)
{
    assert(inputNb >= 0 && inputNb < (int)_imp->clipsInfos.size());
    _imp->clipsInfos[inputNb].hint = hint;
    getNode()->setInputHint(inputNb, hint);
}

void
OfxEffectInstance::onClipSecretChanged(int inputNb, bool isSecret)
{
    assert(inputNb >= 0 && inputNb < (int)_imp->clipsInfos.size());
    _imp->clipsInfos[inputNb].visible = !isSecret;
    getNode()->setInputVisible(inputNb, !isSecret);
}

std::string
OfxEffectInstance::getInputLabel(int inputNb) const
{
    assert(_imp->context != eContextNone);
    assert( inputNb >= 0 &&  inputNb < (int)_imp->clipsInfos.size() );
    if (_imp->context != eContextReader) {
        return _imp->clipsInfos[inputNb].clip->getShortLabel();
    } else {
        return NATRON_READER_INPUT_NAME;
    }
}

std::string
OfxEffectInstance::getInputHint(int inputNb) const
{
    assert(_imp->context != eContextNone);
    assert( inputNb >= 0 &&  inputNb < (int)_imp->clipsInfos.size() );
    if (_imp->context != eContextReader) {
        return _imp->clipsInfos[inputNb].clip->getHint();
    } else {
        return NATRON_READER_INPUT_NAME;
    }
}

OfxEffectInstance::MappedInputV
OfxEffectInstance::inputClipsCopyWithoutOutput() const
{
    assert(_imp->context != eContextNone);
    assert( effectInstance() );
    const std::vector<OFX::Host::ImageEffect::ClipDescriptor*> & clips = effectInstance()->getDescriptor().getClipsByOrder();
    MappedInputV copy;
    for (U32 i = 0; i < clips.size(); ++i) {
        assert(clips[i]);
        if ( !clips[i]->isOutput() ) {
            copy.push_back(clips[i]);
        }
    }

    return copy;
}

OfxClipInstance*
OfxEffectInstance::getClipCorrespondingToInput(int inputNo) const
{
    assert(_imp->context != eContextNone);
    assert( inputNo < (int)_imp->clipsInfos.size() );

    return _imp->clipsInfos[inputNo].clip;
}

int
OfxEffectInstance::getNInputs() const
{
    assert(_imp->context != eContextNone);

    //const std::string & context = effectInstance()->getContext();
    return _imp->nbSourceClips;
}

bool
OfxEffectInstance::isInputOptional(int inputNb) const
{
    assert(_imp->context != eContextNone);
    assert( inputNb >= 0 && inputNb < (int)_imp->clipsInfos.size() );

    return _imp->clipsInfos[inputNb].optional;
}

bool
OfxEffectInstance::isInputMask(int inputNb) const
{
    assert(_imp->context != eContextNone);
    assert( inputNb >= 0 && inputNb < (int)_imp->clipsInfos.size() );

    return _imp->clipsInfos[inputNb].mask;
}

bool
OfxEffectInstance::isInputRotoBrush(int inputNb) const
{
    assert(_imp->context != eContextNone);
    assert( inputNb >= 0 && inputNb < (int)_imp->clipsInfos.size() );

    return _imp->clipsInfos[inputNb].rotoBrush;
}

int
OfxEffectInstance::getRotoBrushInputIndex() const
{
    assert(_imp->context != eContextNone);
    for (std::size_t i = 0; i < _imp->clipsInfos.size(); ++i) {
        if (_imp->clipsInfos[i].rotoBrush) {
            return (int)i;
        }
    }

    return -1;
}

void
OfxEffectInstance::onInputChanged(int inputNo)
{
    assert(_imp->context != eContextNone);
    OfxClipInstance* clip = getClipCorrespondingToInput(inputNo);
    assert(clip);
    double time = getApp()->getTimeLine()->currentFrame();
    RenderScale s(1.);


    {
        RECURSIVE_ACTION();
        REPORT_CURRENT_THREAD_ACTION( kOfxActionInstanceChanged, getNode() );
        SET_CAN_SET_VALUE(true);
        ClipsThreadStorageSetter clipSetter(effectInstance(),
                                            ViewIdx(0),
                                            0);
        assert(_imp->effect);

        _imp->effect->beginInstanceChangedAction(kOfxChangeUserEdited);
        _imp->effect->clipInstanceChangedAction(clip->getName(), kOfxChangeUserEdited, time, s);
        _imp->effect->endInstanceChangedAction(kOfxChangeUserEdited);
    }
}

/** @brief map a std::string to a context */
ContextEnum
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
OfxEffectInstance::mapContextToString(ContextEnum ctx)
{
    switch (ctx) {
    case eContextGenerator:

        return kOfxImageEffectContextGenerator;
    case eContextFilter:

        return kOfxImageEffectContextFilter;
    case eContextTransition:

        return kOfxImageEffectContextTransition;
    case eContextPaint:

        return kOfxImageEffectContextPaint;
    case eContextGeneral:

        return kOfxImageEffectContextGeneral;
    case eContextRetimer:

        return kOfxImageEffectContextRetimer;
    case eContextReader:

        return kOfxImageEffectContextReader;
    case eContextWriter:

        return kOfxImageEffectContextWriter;
    case eContextTracker:

        return kNatronOfxImageEffectContextTracker;
    case eContextNone:
    default:
        break;
    }

    return std::string();
}

void
OfxEffectInstance::onMetadataRefreshed(const NodeMetadata& metadata)
{
    //////////////// Actually push to the clips the preferences and set the flags on the effect, protected by a write lock.

    {
        QWriteLocker l(&_imp->preferencesLock);
        if (!_imp->effect) {
            return;
        }
        const std::map<std::string, OFX::Host::ImageEffect::ClipInstance*>& clips = _imp->effect->getClips();
        for (std::map<std::string, OFX::Host::ImageEffect::ClipInstance*>::const_iterator it = clips.begin()
             ; it != clips.end(); ++it) {
            OfxClipInstance* clip = dynamic_cast<OfxClipInstance*>(it->second);
            assert(clip);
            if (!clip) {
                continue;
            }
            int inputNb = clip->getInputNb();

            std::string ofxClipComponentStr;
            std::string componentsType = metadata.getComponentsType(inputNb);
            int nComps = metadata.getNComps(inputNb);
            ImagePlaneDesc natronPlane = ImagePlaneDesc::mapNCompsToColorPlane(nComps);
            if (componentsType == kNatronColorPlaneID) {
                ofxClipComponentStr = ImagePlaneDesc::mapPlaneToOFXComponentsTypeString(natronPlane);
            } else if (componentsType == kNatronDisparityComponentsLabel) {
                ofxClipComponentStr = kFnOfxImageComponentStereoDisparity;
            } else if (componentsType == kNatronMotionComponentsLabel) {
                ofxClipComponentStr = kFnOfxImageComponentMotionVectors;
            } else {
                ofxClipComponentStr = ImagePlaneDesc::mapPlaneToOFXComponentsTypeString(natronPlane);
            }


            clip->setComponents(ofxClipComponentStr);
            clip->setPixelDepth( OfxClipInstance::natronsDepthToOfxDepth( metadata.getBitDepth(inputNb) ) );
            clip->setAspectRatio( metadata.getPixelAspectRatio(inputNb) );
        }

        effectInstance()->updatePreferences_safe( metadata.getOutputFrameRate(),
                                                  OfxClipInstance::natronsFieldingToOfxFielding( metadata.getOutputFielding() ),
                                                  OfxClipInstance::natronsPremultToOfxPremult( metadata.getOutputPremult() ),
                                                  metadata.getIsContinuous(),
                                                  metadata.getIsFrameVarying() );
    }
}

StatusEnum
OfxEffectInstance::getPreferredMetadata(NodeMetadata& metadata)
{
    if (!_imp->created || !_imp->effect) {
        return eStatusFailed;
    }
    assert(_imp->context != eContextNone);
    assert( QThread::currentThread() == qApp->thread() );

    StatusEnum stat;
    ////////////////////////////////////////////////////////////////
    ///////////////////////////////////
    ////////////////  Get plug-in render preferences
    {
        RECURSIVE_ACTION();
        /*
           We allow parameters values changes within the getClipPreference action because some parameters may only be updated
           reliably in this action. For example a choice parameter tracking all available components upstream in each clip needs
           to be refreshed in getClipPreferences since it may change due to a param change in a plug-in upstream.
           It is then up to the plug-in to avoid infinite recursions in getClipPreference.
         */
        SET_CAN_SET_VALUE(true);

        ///First push the "default" meta-datas to the clips so that they get proper values
        onMetadataRefreshed(metadata);

        ///It has been overridden and no data is actually set on the clip, everything will be set into the
        ///metadata object
        stat = _imp->effect->getClipPreferences_safe(metadata);
    }

    return stat;
}

StatusEnum
OfxEffectInstance::getRegionOfDefinition(U64 /*hash*/,
                                         double time,
                                         const RenderScale & scale,
                                         ViewIdx view,
                                         RectD* rod)
{
    assert(_imp->context != eContextNone);
    if (!_imp->initialized) {
        return eStatusFailed;
    }

    assert(_imp->effect);

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
        ClipsThreadStorageSetter clipSetter(effectInstance(),
                                            view,
                                            mipMapLevel);

        assert(_imp->effect);
        if (getRecursionLevel() > 1) {
            stat = _imp->effect->getRegionOfDefinitionAction(time, scale, view, ofxRod);
        } else {
            ///Take the preferences lock so that it cannot be modified throughout the action.
            QReadLocker preferencesLocker(&_imp->preferencesLock);
            stat = _imp->effect->getRegionOfDefinitionAction(time, scale, view, ofxRod);
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
                        assert(_imp->effect);

                        if (getRecursionLevel() > 1) {
                            stat = _imp->effect->getRegionOfDefinitionAction(time, halfScale, view, tmpRod);
                        } else {
                            ///Take the preferences lock so that it cannot be modified throughout the action.
                            QReadLocker preferencesLocker(&_imp->preferencesLock);
                            stat = _imp->effect->getRegionOfDefinitionAction(time, halfScale, view, tmpRod);
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
                        assert(_imp->effect);

                        if (getRecursionLevel() > 1) {
                            stat = _imp->effect->getRegionOfDefinitionAction(time, scaleOne, view, tmpRod);
                        } else {
                            ///Take the preferences lock so that it cannot be modified throughout the action.
                            QReadLocker preferencesLocker(&_imp->preferencesLock);
                            stat = _imp->effect->getRegionOfDefinitionAction(time, scaleOne, view, tmpRod);
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
//            calcDefaultRegionOfDefinition(hash, time, scale, view, rod);
//
//            return eStatusReplyDefault;
//        }
    }


    ///If the rod is 1 pixel, determine if it was because one clip was unconnected or this is really a
    ///1 pixel large image
    if ( (ofxRod.x2 == 1.) && (ofxRod.y2 == 1.) && (ofxRod.x1 == 0.) && (ofxRod.y1 == 0.) ) {
        int maxInputs = getNInputs();
        for (int i = 0; i < maxInputs; ++i) {
            OfxClipInstance* clip = getClipCorrespondingToInput(i);
            if ( clip && !clip->getConnected() && !clip->getIsOptional() && !clip->getIsMask() ) {
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
                                                 const RenderScale & scale,
                                                 ViewIdx view,
                                                 RectD *rod)
{
    assert(_imp->context != eContextNone);
    if (!_imp->initialized) {
        throw std::runtime_error("OfxEffectInstance not initialized");
    }

    unsigned int mipMapLevel = Image::getLevelFromScale(scale.x);
    OfxRectD ofxRod;

    {
        SET_CAN_SET_VALUE(false);
        assert(_imp->effect);

        ///Take the preferences lock so that it cannot be modified throughout the action.
        if (getRecursionLevel() == 0) {
            ClipsThreadStorageSetter clipSetter(effectInstance(),
                                                view,
                                                mipMapLevel);


            // from http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#kOfxImageEffectActionGetRegionOfDefinition
            // generator context - defaults to the project window,
            // filter and paint contexts - defaults to the RoD of the 'Source' input clip at the given time,
            // transition context - defaults to the union of the RoDs of the 'SourceFrom' and 'SourceTo' input clips at the given time,
            // general context - defaults to the union of the RoDs of all the effect non optional input clips at the given time, if none exist, then it is the project window
            // retimer context - defaults to the union of the RoD of the 'Source' input clip at the frame directly preceding the value of the 'SourceTime' double parameter and the frame directly after it

            // the following ofxh function does the job
            QReadLocker preferencesLocker(&_imp->preferencesLock);
            ofxRod = _imp->effect->calcDefaultRegionOfDefinition(time, scale, view);
        } else {
            ofxRod = _imp->effect->calcDefaultRegionOfDefinition(time, scale, view);
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
                                        ViewIdx view,
                                        RoIMap* ret)
{
    assert(_imp->context != eContextNone);
    std::map<OFX::Host::ImageEffect::ClipInstance*, OfxRectD> inputRois;
    if (!_imp->initialized) {
        return;
    }
    assert(outputRoD.x2 >= outputRoD.x1 && outputRoD.y2 >= outputRoD.y1);
    Q_UNUSED(outputRoD);
    assert(renderWindow.x2 >= renderWindow.x1 && renderWindow.y2 >= renderWindow.y1);

    assert((supportsRenderScaleMaybe() != eSupportsNo) || (scale.x == 1. && scale.y == 1.));

    OfxStatus stat;

    ///before calling getRoIaction set the relevant info on the clips
    unsigned int mipMapLevel = Image::getLevelFromScale(scale.x);
    {
        SET_CAN_SET_VALUE(false);

        ClipsThreadStorageSetter clipSetter(effectInstance(),
                                            view,
                                            mipMapLevel);
        OfxRectD roi;
        rectToOfxRectD(renderWindow, &roi);

        ///Take the preferences lock so that it cannot be modified throughout the action.
        QReadLocker preferencesLocker(&_imp->preferencesLock);
        assert(_imp->effect);
        stat = _imp->effect->getRegionOfInterestAction( (OfxTime)time, scale, view,
                                                        roi, inputRois );
    }


    if ( (stat != kOfxStatOK) && (stat != kOfxStatReplyDefault) ) {
        throw std::runtime_error("getRegionsOfInterest action failed");
    }

    //Default behaviour already handled in getRegionOfInterestAction

    for (std::map<OFX::Host::ImageEffect::ClipInstance*, OfxRectD>::iterator it = inputRois.begin(); it != inputRois.end(); ++it) {
        OfxClipInstance* clip = dynamic_cast<OfxClipInstance*>(it->first);
        assert(clip);
        if (clip) {
            EffectInstancePtr inputNode = clip->getAssociatedNode();
            RectD inputRoi; // input RoI in canonical coordinates
            inputRoi.x1 = it->second.x1;
            inputRoi.x2 = it->second.x2;
            inputRoi.y1 = it->second.y1;
            inputRoi.y2 = it->second.y2;

            if ( inputRoi.isNull() ) {
                continue;
            }

            ///The RoI might be infinite if the getRoI action of the plug-in doesn't do anything and the input effect has an
            ///infinite rod.
            ifInfiniteclipRectToProjectDefault(&inputRoi);
            //if (!inputRoi.isNull()) {
            ret->insert( std::make_pair(inputNode, inputRoi) );
            //}
        }
    }
} // getRegionsOfInterest

FramesNeededMap
OfxEffectInstance::getFramesNeeded(double time,
                                   ViewIdx view)
{
    assert(_imp->context != eContextNone);
    FramesNeededMap ret;
    if (!_imp->initialized) {
        return ret;
    }
    assert(_imp->effect);
    OfxStatus stat;

    if ( isViewAware() ) {
        OFX::Host::ImageEffect::ViewsRangeMap inputRanges;
        {
            SET_CAN_SET_VALUE(false);
            assert(_imp->effect);

            ///Take the preferences lock so that it cannot be modified throughout the action.
            QReadLocker preferencesLocker(&_imp->preferencesLock);
            stat = _imp->effect->getFrameViewsNeeded( (OfxTime)time, view, inputRanges );
        }

        if ( (stat != kOfxStatOK) && (stat != kOfxStatReplyDefault) ) {
            throw std::runtime_error("getFrameViewsNeeded action failed");
        } else {
            for (OFX::Host::ImageEffect::ViewsRangeMap::iterator it = inputRanges.begin(); it != inputRanges.end(); ++it) {
                OfxClipInstance* clip = dynamic_cast<OfxClipInstance*>(it->first);
                assert(clip);
                if (clip) {
                    int inputNb = clip->getInputNb();
                    if (inputNb != -1) {
                        // convert HostSupport's std::map<int, std::vector<OfxRangeD> > to  Natron's FrameRangesMap
                        FrameRangesMap frameRanges;
                        const std::map<int, std::vector<OfxRangeD> >& ofxRanges = it->second;
                        for (std::map<int, std::vector<OfxRangeD> >::const_iterator itr = ofxRanges.begin(); itr != ofxRanges.end(); ++itr) {
                            frameRanges[ViewIdx(itr->first)] = itr->second;
                        }
                        ret.insert( std::make_pair(inputNb, frameRanges) );
                    }
                }
            }
        }
    } else {
        OFX::Host::ImageEffect::RangeMap inputRanges;
        {
            SET_CAN_SET_VALUE(false);
            assert(_imp->effect);

            ///Take the preferences lock so that it cannot be modified throughout the action.
            QReadLocker preferencesLocker(&_imp->preferencesLock);
            stat = _imp->effect->getFrameNeededAction( (OfxTime)time, inputRanges );
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
                        FrameRangesMap viewRangeMap;
                        viewRangeMap.insert( std::make_pair(view, it->second) );
                        ret.insert( std::make_pair(inputNb, viewRangeMap) );
                    }
                }
            }
        }
    }

    //Default is already handled by HostSupport
//    if (stat == kOfxStatReplyDefault) {
//        return EffectInstance::getFramesNeeded(time,view);

//    }
    return ret;
} // OfxEffectInstance::getFramesNeeded

void
OfxEffectInstance::getFrameRange(double *first,
                                 double *last)
{
    assert(_imp->context != eContextNone);
    if (!_imp->initialized) {
        return;
    }
    OfxRangeD range;
    // getTimeDomain should only be called on the 'general', 'reader' or 'generator' contexts.
    //  see http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#kOfxImageEffectActionGetTimeDomain"
    // Edit: Also add the 'writer' context as we need the getTimeDomain action to be able to find out the frame range to render.
    OfxStatus st = kOfxStatReplyDefault;
    if ( (_imp->context == eContextGeneral) ||
         ( _imp->context == eContextReader) ||
         ( _imp->context == eContextWriter) ||
         ( _imp->context == eContextGenerator) ) {
        SET_CAN_SET_VALUE(false);
        assert(_imp->effect);

        ///Take the preferences lock so that it cannot be modified throughout the action.
        QReadLocker preferencesLocker(&_imp->preferencesLock);
        st = _imp->effect->getTimeDomainAction(range);
    }
    if (st == kOfxStatOK) {
        *first = range.min;
        *last = range.max;
    } else if (st == kOfxStatReplyDefault) {
        assert(_imp->effect);
        //The default is...
        int nthClip = _imp->effect->getNClips();
        if (nthClip == 0) {
            //infinite if there are no non optional input clips.
            *first = std::numeric_limits<int>::min();
            *last = std::numeric_limits<int>::max();
        } else {
            //the union of all the frame ranges of the non optional input clips.
            bool firstValidInput = true;
            *first = std::numeric_limits<int>::min();
            *last = std::numeric_limits<int>::max();

            int inputsCount = getNInputs();

            ///Uncommented the isOptional() introduces a bugs with Genarts Monster plug-ins when 2 generators
            ///are connected in the pipeline. They must rely on the time domain to maintain an internal state and apparently
            ///not taking optional inputs into accounts messes it up.
            for (int i = 0; i < inputsCount; ++i) {
                //if (!isInputOptional(i)) {
                EffectInstancePtr inputEffect = getInput(i);
                if (inputEffect) {
                    double f, l;
                    inputEffect->getFrameRange_public(inputEffect->getRenderHash(), &f, &l);
                    if (!firstValidInput) {
                        if ( (f < *first) && (f != std::numeric_limits<int>::min()) ) {
                            *first = f;
                        }
                        if ( (l > *last) && (l != std::numeric_limits<int>::max()) ) {
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
                              ViewIdx view,
                              double* inputTime,
                              ViewIdx* inputView,
                              int* inputNb)
{
    *inputView = view;
    if (!_imp->created) {
        *inputNb = -1;
        *inputTime = 0;

        return false;
    }

    assert(_imp->context != eContextNone);
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
        SET_CAN_SET_VALUE(false);


        ClipsThreadStorageSetter clipSetter(effectInstance(),
                                            view,
                                            mipMapLevel);
        OfxRectI ofxRoI;
        ofxRoI.x1 = renderWindow.left();
        ofxRoI.x2 = renderWindow.right();
        ofxRoI.y1 = renderWindow.bottom();
        ofxRoI.y2 = renderWindow.top();

        assert(_imp->effect);

        int identityView = view;
        string identityPlane = kFnOfxImagePlaneColour;
        if (getRecursionLevel() > 1) {
            stat = _imp->effect->isIdentityAction(inputTimeOfx, field, ofxRoI, scale, identityView, identityPlane, inputclip);
        } else {
            ///Take the preferences lock so that it cannot be modified throughout the action.
            QReadLocker preferencesLocker(&_imp->preferencesLock);
            stat = _imp->effect->isIdentityAction(inputTimeOfx, field, ofxRoI, scale, identityView, identityPlane, inputclip);
        }
        if (identityView != view || identityPlane != kFnOfxImagePlaneColour) {
#pragma message WARN("can Natron RB2-multiplane2 handle isIdentity across views and planes?")
            // Natron 2 cannot handle isIdentity across planes
            stat = kOfxStatOK;
        }
    }

    assert(_imp->effect);
    if (stat == kOfxStatOK) {
        OFX::Host::ImageEffect::ClipInstance* clip = _imp->effect->getClip(inputclip);
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

class OfxGLContextEffectData
    : public EffectInstance::OpenGLContextEffectData
{
    void* _dataHandle;

public:

    OfxGLContextEffectData()
        : EffectInstance::OpenGLContextEffectData()
        , _dataHandle(0)
    {
    }

    void setDataHandle(void *dataHandle)
    {
        _dataHandle = dataHandle;
    }

    void* getDataHandle() const
    {
        return _dataHandle;
    }

    virtual ~OfxGLContextEffectData() {}
};

typedef std::shared_ptr<OfxGLContextEffectData> OfxGLContextEffectDataPtr;

StatusEnum
OfxEffectInstance::beginSequenceRender(double first,
                                       double last,
                                       double step,
                                       bool interactive,
                                       const RenderScale & scale,
                                       bool isSequentialRender,
                                       bool isRenderResponseToUserInteraction,
                                       bool draftMode,
                                       ViewIdx view,
                                       bool isOpenGLRender,
                                       const EffectInstance::OpenGLContextEffectDataPtr& glContextData)
{
    {
        bool scaleIsOne = (scale.x == 1. && scale.y == 1.);
        assert( !( (supportsRenderScaleMaybe() == eSupportsNo) && !scaleIsOne ) );
        Q_UNUSED(scaleIsOne);
    }

    OfxStatus stat;
    unsigned int mipMapLevel = Image::getLevelFromScale(scale.x);
    {
        ClipsThreadStorageSetter clipSetter(effectInstance(),
                                            view,
                                            mipMapLevel);

        SET_CAN_SET_VALUE(false);

        OfxGLContextEffectData* isOfxGLData = dynamic_cast<OfxGLContextEffectData*>( glContextData.get() );
        void* oglData = isOfxGLData ? isOfxGLData->getDataHandle() : 0;

        ///Take the preferences lock so that it cannot be modified throughout the action.
        QReadLocker preferencesLocker(&_imp->preferencesLock);
        stat = effectInstance()->beginRenderAction(first, last, step,
                                                   interactive, scale,
                                                   isSequentialRender, isRenderResponseToUserInteraction,
                                                   isOpenGLRender, oglData, draftMode, view);
    }

    if ( (stat != kOfxStatOK) && (stat != kOfxStatReplyDefault) ) {
        return eStatusFailed;
    }

    return eStatusOK;
}

StatusEnum
OfxEffectInstance::endSequenceRender(double first,
                                     double last,
                                     double step,
                                     bool interactive,
                                     const RenderScale & scale,
                                     bool isSequentialRender,
                                     bool isRenderResponseToUserInteraction,
                                     bool draftMode,
                                     ViewIdx view,
                                     bool isOpenGLRender,
                                     const EffectInstance::OpenGLContextEffectDataPtr& glContextData)
{
    {
        bool scaleIsOne = (scale.x == 1. && scale.y == 1.);
        assert( !( (supportsRenderScaleMaybe() == eSupportsNo) && !scaleIsOne ) );
        Q_UNUSED(scaleIsOne);
    }

    OfxStatus stat;
    unsigned int mipMapLevel = Image::getLevelFromScale(scale.x);
    {
        ClipsThreadStorageSetter clipSetter(effectInstance(),
                                            view,
                                            mipMapLevel);
        SET_CAN_SET_VALUE(false);

        OfxGLContextEffectData* isOfxGLData = dynamic_cast<OfxGLContextEffectData*>( glContextData.get() );
        void* oglData = isOfxGLData ? isOfxGLData->getDataHandle() : 0;

        ///Take the preferences lock so that it cannot be modified throughout the action.
        QReadLocker preferencesLocker(&_imp->preferencesLock);
        stat = effectInstance()->endRenderAction(first, last, step,
                                                 interactive, scale,
                                                 isSequentialRender, isRenderResponseToUserInteraction,
                                                 isOpenGLRender, oglData, draftMode, view);
    }

    if ( (stat != kOfxStatOK) && (stat != kOfxStatReplyDefault) ) {
        return eStatusFailed;
    }

    return eStatusOK;
}

StatusEnum
OfxEffectInstance::render(const RenderActionArgs& args)
{
    if (!_imp->initialized) {
        return eStatusFailed;
    }

    assert( !args.outputPlanes.empty() );

    const std::pair<ImagePlaneDesc, ImagePtr>& firstPlane = args.outputPlanes.front();
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
    for (std::list<std::pair<ImagePlaneDesc, ImagePtr> >::const_iterator it = args.outputPlanes.begin();
         it != args.outputPlanes.end(); ++it) {
        if (!multiPlanar) {
            // When not multi-planar, the components of the image will be the colorplane
            ofxPlanes.push_back(ImagePlaneDesc::mapPlaneToOFXPlaneString(it->second->getComponents()));
        } else {
            ofxPlanes.push_back(ImagePlaneDesc::mapPlaneToOFXPlaneString(it->first));
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

        if ( !supportsTiles() && !isDuringPaintStrokeCreationThreadLocal() ) {
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
            // Commented-out: Some Furnace plug-ins from The Foundry (e.g F_Steadiness) are not supporting multi-resolution but actually produce an output
            // with a RoD different from the input
            /* assert(dstRod.x1 == 0);
               assert(dstRod.y1 == 0);*/
        }
    }
# endif // DEBUG
    {
        SET_CAN_SET_VALUE(false);
        assert(_imp->effect);

        RenderThreadStorageSetter clipSetter(effectInstance(),
                                             args.view,
                                             Image::getLevelFromScale(args.originalScale.x),
                                             firstPlane.first,
                                             args.inputImages);
        OfxGLContextEffectData* isOfxGLData = dynamic_cast<OfxGLContextEffectData*>( args.glContextData.get() );
        void* oglData = isOfxGLData ? isOfxGLData->getDataHandle() : 0;

        ///Take the preferences lock so that it cannot be modified throughout the action.
        QReadLocker preferencesLocker(&_imp->preferencesLock);
        stat = _imp->effect->renderAction( (OfxTime)args.time,
                                           field,
                                           ofxRoI,
                                           args.mappedScale,
                                           args.isSequentialRender,
                                           args.isRenderResponseToUserInteraction,
                                           args.useOpenGL,
                                           oglData,
                                           args.draftMode,
                                           args.view,
                                           viewsCount,
                                           ofxPlanes );
    }

    if (stat != kOfxStatOK) {
        if ( !getNode()->hasPersistentMessage() ) {
            QString err;
            if (stat == kOfxStatErrImageFormat) {
                err = tr("Bad image format was supplied by %1.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) );
                setPersistentMessage( eMessageTypeError, err.toStdString() );
            } else if (stat == kOfxStatErrMemory) {
                err = tr("Out of memory!");
                setPersistentMessage( eMessageTypeError, err.toStdString() );
            } else {
                QString existingMessage;
                int type;
                getNode()->getPersistentMessage(&existingMessage, &type);
                if (existingMessage.isEmpty()) {
                    err = tr("Unknown failure reason.");
                }
            }

        }

        return eStatusFailed;
    } else {
        return eStatusOK;
    }
} // render

bool
OfxEffectInstance::supportsMultipleClipPARs() const
{
    return _imp->supportsMultipleClipPARs;
}

bool
OfxEffectInstance::supportsMultipleClipDepths() const
{
    return _imp->supportsMultipleClipDepths;
}

RenderSafetyEnum
OfxEffectInstance::renderThreadSafety() const
{
    if (!_imp->effect) {
        return eRenderSafetyUnsafe;
    }
    {
        QReadLocker readL(&_imp->renderSafetyLock);
        if (_imp->wasRenderSafetySet) {
            return _imp->renderSafety;
        }
    }
    {
        QWriteLocker writeL(&_imp->renderSafetyLock);
        assert(_imp->effect);

        const std::string & safety = _imp->effect->getRenderThreadSafety();
        if (safety == kOfxImageEffectRenderUnsafe) {
            _imp->renderSafety =  eRenderSafetyUnsafe;
        } else if (safety == kOfxImageEffectRenderInstanceSafe) {
            _imp->renderSafety = eRenderSafetyInstanceSafe;
        } else if (safety == kOfxImageEffectRenderFullySafe) {
            if ( _imp->effect->getHostFrameThreading() ) {
                _imp->renderSafety =  eRenderSafetyFullySafeFrame;
            } else {
                _imp->renderSafety =  eRenderSafetyFullySafe;
            }
        } else {
            qDebug() << "Unknown thread safety level: " << safety.c_str();
            _imp->renderSafety =  eRenderSafetyUnsafe;
        }
        _imp->wasRenderSafetySet = true;

        return _imp->renderSafety;
    }
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

bool
OfxEffectInstance::canHandleRenderScaleForOverlays() const
{
    return _imp->overlaysCanHandleRenderScale;
}

void
OfxEffectInstance::drawOverlay(double time,
                               const RenderScale & renderScale,
                               ViewIdx view)
{
    if (!_imp->initialized) {
        return;
    }
    if (_imp->overlayInteract) {
        SET_CAN_SET_VALUE(false);
        _imp->overlayInteract->drawAction(time, renderScale, view, _imp->overlayInteract->hasColorPicker() ? &_imp->overlayInteract->getLastColorPickerColor() : /*colourPicker=*/0);
    }
}

void
OfxEffectInstance::setCurrentViewportForOverlays(OverlaySupport* viewport)
{
    if (_imp->overlayInteract) {
        _imp->overlayInteract->setCallingViewport(viewport);
    }
}

bool
OfxEffectInstance::onOverlayPenDown(double time,
                                    const RenderScale & renderScale,
                                    ViewIdx view,
                                    const QPointF & viewportPos,
                                    const QPointF & pos,
                                    double pressure,
                                    double /*timestamp*/,
                                    PenType /*pen*/)
{
    if (!_imp->initialized) {
        return false;
    }
    if (_imp->overlayInteract) {
        OfxPointD penPos;
        penPos.x = pos.x();
        penPos.y = pos.y();
        OfxPointI penPosViewport;
        penPosViewport.x = viewportPos.x();
        penPosViewport.y = viewportPos.y();

        SET_CAN_SET_VALUE(true);

        OfxStatus stat = _imp->overlayInteract->penDownAction(time, renderScale, view, _imp->overlayInteract->hasColorPicker() ? &_imp->overlayInteract->getLastColorPickerColor() : /*colourPicker=*/0, penPos, penPosViewport, pressure);


        if ( (getRecursionLevel() == 1) && checkIfOverlayRedrawNeeded() ) {
            OfxStatus redrawstat = _imp->overlayInteract->redraw();
            assert(redrawstat == kOfxStatOK || redrawstat == kOfxStatReplyDefault);
            Q_UNUSED(redrawstat);
        }

        if (stat == kOfxStatOK) {
            _imp->penDown = true;

            return true;
        }
    }

    return false;
}

bool
OfxEffectInstance::onOverlayPenMotion(double time,
                                      const RenderScale & renderScale,
                                      ViewIdx view,
                                      const QPointF & viewportPos,
                                      const QPointF & pos,
                                      double pressure,
                                      double /*timestamp*/)
{
    if (!_imp->initialized) {
        return false;
    }
    if (_imp->overlayInteract) {
        OfxPointD penPos;
        penPos.x = pos.x();
        penPos.y = pos.y();
        OfxPointI penPosViewport;
        penPosViewport.x = viewportPos.x();
        penPosViewport.y = viewportPos.y();
        OfxStatus stat;

        SET_CAN_SET_VALUE(true);
        stat = _imp->overlayInteract->penMotionAction(time, renderScale, view, _imp->overlayInteract->hasColorPicker() ? &_imp->overlayInteract->getLastColorPickerColor() : /*colourPicker=*/0, penPos, penPosViewport, pressure);

        if ( (getRecursionLevel() == 1) && checkIfOverlayRedrawNeeded() ) {
            stat = _imp->overlayInteract->redraw();
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
                                  const RenderScale & renderScale,
                                  ViewIdx view,
                                  const QPointF & viewportPos,
                                  const QPointF & pos,
                                  double pressure,
                                  double /*timestamp*/)
{
    if (!_imp->initialized) {
        return false;
    }
    if (_imp->overlayInteract) {
        OfxPointD penPos;
        penPos.x = pos.x();
        penPos.y = pos.y();
        OfxPointI penPosViewport;
        penPosViewport.x = viewportPos.x();
        penPosViewport.y = viewportPos.y();

        SET_CAN_SET_VALUE(true);
        OfxStatus stat = _imp->overlayInteract->penUpAction(time, renderScale, view, _imp->overlayInteract->hasColorPicker() ? &_imp->overlayInteract->getLastColorPickerColor() : /*colourPicker=*/0, penPos, penPosViewport, pressure);

        if ( (getRecursionLevel() == 1) && checkIfOverlayRedrawNeeded() ) {
            stat = _imp->overlayInteract->redraw();
            assert(stat == kOfxStatOK || stat == kOfxStatReplyDefault);
        }

        if (stat == kOfxStatOK) {
            _imp->penDown = false;

            return true;
        }
    }

    return false;
}

bool
OfxEffectInstance::onOverlayKeyDown(double time,
                                    const RenderScale & renderScale,
                                    ViewIdx view,
                                    Key key,
                                    KeyboardModifiers /*modifiers*/)
{
    if (!_imp->initialized) {
        return false;;
    }
    if (_imp->overlayInteract) {
        QByteArray keyStr;
        SET_CAN_SET_VALUE(true);
        OfxStatus stat = _imp->overlayInteract->keyDownAction( time, renderScale, view,_imp->overlayInteract->hasColorPicker() ? &_imp->overlayInteract->getLastColorPickerColor() : /*colourPicker=*/0, (int)key, keyStr.data() );

        if ( (getRecursionLevel() == 1) && checkIfOverlayRedrawNeeded() ) {
            stat = _imp->overlayInteract->redraw();
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
                                  const RenderScale & renderScale,
                                  ViewIdx view,
                                  Key key,
                                  KeyboardModifiers /* modifiers*/)
{
    if (!_imp->initialized) {
        return false;
    }
    if (_imp->overlayInteract) {
        QByteArray keyStr;
        SET_CAN_SET_VALUE(true);
        OfxStatus stat = _imp->overlayInteract->keyUpAction( time, renderScale, view, _imp->overlayInteract->hasColorPicker() ? &_imp->overlayInteract->getLastColorPickerColor() : /*colourPicker=*/0, (int)key, keyStr.data() );

        if ( (getRecursionLevel() == 1) && checkIfOverlayRedrawNeeded() ) {
            stat = _imp->overlayInteract->redraw();
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
                                      const RenderScale & renderScale,
                                      ViewIdx view,
                                      Key key,
                                      KeyboardModifiers /*modifiers*/)
{
    if (!_imp->initialized) {
        return false;
    }
    if (_imp->overlayInteract) {
        QByteArray keyStr;

        SET_CAN_SET_VALUE(true);
        OfxStatus stat = _imp->overlayInteract->keyRepeatAction( time, renderScale, view, _imp->overlayInteract->hasColorPicker() ? &_imp->overlayInteract->getLastColorPickerColor() : /*colourPicker=*/0, (int)key, keyStr.data() );

        if ( (getRecursionLevel() == 1) && checkIfOverlayRedrawNeeded() ) {
            stat = _imp->overlayInteract->redraw();
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
                                        const RenderScale & renderScale,
                                        ViewIdx view)
{
    if (!_imp->initialized) {
        return false;
    }
    if (_imp->overlayInteract) {
        OfxStatus stat;
        SET_CAN_SET_VALUE(true);
        stat = _imp->overlayInteract->gainFocusAction(time, renderScale, view, _imp->overlayInteract->hasColorPicker() ? &_imp->overlayInteract->getLastColorPickerColor() : /*colourPicker=*/0);
        if (stat == kOfxStatOK) {
            return true;
        }
    }

    return false;
}

bool
OfxEffectInstance::onOverlayFocusLost(double time,
                                      const RenderScale & renderScale,
                                      ViewIdx view)
{
    if (!_imp->initialized) {
        return false;
    }
    if (_imp->overlayInteract) {
        OfxStatus stat;
        SET_CAN_SET_VALUE(true);
        stat = _imp->overlayInteract->loseFocusAction(time, renderScale, view, _imp->overlayInteract->hasColorPicker() ? &_imp->overlayInteract->getLastColorPickerColor() : /*colourPicker=*/0);
        if (stat == kOfxStatOK) {
            return true;
        }
    }

    return false;
}

bool
OfxEffectInstance::hasOverlay() const
{
    return _imp->overlayInteract != NULL;
}

void
OfxEffectInstance::redrawOverlayInteract()
{
    assert(_imp->overlayInteract);
    OfxStatus st = _imp->overlayInteract->redraw();
    Q_UNUSED(st);
}

std::string
OfxEffectInstance::natronValueChangedReasonToOfxValueChangedReason(ValueChangedReasonEnum reason)
{
    switch (reason) {
    case eValueChangedReasonUserEdited:
    case eValueChangedReasonNatronGuiEdited:
    case eValueChangedReasonSlaveRefresh:
    case eValueChangedReasonRestoreDefault:
    case eValueChangedReasonNatronInternalEdited:

        return kOfxChangeUserEdited;
    case eValueChangedReasonPluginEdited:

        return kOfxChangePluginEdited;
    case eValueChangedReasonTimeChanged:

        return kOfxChangeTime;
    default:
        assert(false);         // all Natron reasons should be processed
        return "";
    }
}

class OfxUndoCommand : public UndoCommand
{
    KnobStringWPtr _textKnob;
    KnobBoolWPtr _stateKnob;
public:

    OfxUndoCommand(const KnobStringPtr& textKnob, const KnobBoolPtr &stateKnob)
    : _textKnob(textKnob)
    , _stateKnob(stateKnob)
    {
        setText(textKnob->getValue());
        stateKnob->blockValueChanges();
        stateKnob->setValue(true);
        stateKnob->unblockValueChanges();
    }

    virtual ~OfxUndoCommand()
    {

    }

    /**
     * @brief Called to redo the action
     **/
    virtual void redo() OVERRIDE FINAL
    {
        KnobBoolPtr state = _stateKnob.lock();
        bool currentValue = state->getValue();
        assert(!currentValue);
        state->setValue(true);
        if (currentValue) {
            state->evaluateValueChange(0, 0, ViewSpec(0), eValueChangedReasonUserEdited);
        }
    }

    /**
     * @brief Called to undo the action
     **/
    virtual void undo() OVERRIDE FINAL
    {
        KnobBoolPtr state = _stateKnob.lock();
        bool currentValue = state->getValue();
        assert(currentValue);
        state->setValue(false);
        if (!currentValue) {
            state->evaluateValueChange(0, 0, ViewSpec(0), eValueChangedReasonUserEdited);
        }
    }
    

};

bool
OfxEffectInstance::knobChanged(KnobI* k,
                               ValueChangedReasonEnum reason,
                               ViewSpec view,
                               double time,
                               bool /*originatedFromMainThread*/)
{
    assert(k);
    if (!k) {
        throw std::logic_error(__func__);
    }
    if (!_imp->initialized) {
        return false;
    }

    {
        // Handle cursor knob
        KnobStringPtr cursorKnob = _imp->cursorKnob.lock();
        if (k == cursorKnob.get()) {
            CursorEnum c;
            std::string cursorStr = cursorKnob->getValue();
            if (OfxImageEffectInstance::ofxCursorToNatronCursor(cursorStr, &c)) {
                setCurrentCursor(c);
            } else {
                setCurrentCursor(QString::fromUtf8(cursorStr.c_str()));
            }
            return true;
        }
        KnobStringPtr undoRedoText = _imp->undoRedoTextKnob.lock();
        if (k == undoRedoText.get()) {
            KnobBoolPtr undoRedoState = _imp->undoRedoStateKnob.lock();
            assert(undoRedoState);

            if (undoRedoState && reason == eValueChangedReasonPluginEdited) {
                UndoCommandPtr cmd = std::make_shared<OfxUndoCommand>(undoRedoText, undoRedoState);
                pushUndoCommand(cmd);
                return true;
            }
        }

    }
    std::string ofxReason = natronValueChangedReasonToOfxValueChangedReason(reason);
    assert( !ofxReason.empty() ); // crashes when resetting to defaults
    RenderScale renderScale  = getOverlayInteractRenderScale();
    OfxStatus stat = kOfxStatOK;
    int recursionLevel = getRecursionLevel();

    OfxImageEffectInstance* effect = effectInstance();
    assert(effect);
    if (!effect) {
        throw std::logic_error(__func__);
    }
    if (recursionLevel == 1) {
        SET_CAN_SET_VALUE(true);
        // assert(!view.isAll() && !view.isCurrent());
        ViewIdx v = ( view.isAll() || view.isCurrent() ) ? ViewIdx(0) : ViewIdx(view);
        ClipsThreadStorageSetter clipSetter( effect,
                                             v,
                                             Image::getLevelFromScale(renderScale.x) );

        ///This action as all the overlay interacts actions can trigger recursive actions, such as
        ///getClipPreferences() so we don't take the clips preferences lock for read here otherwise we would
        ///create a deadlock. This code then assumes that the instance changed action of the plug-in doesn't require
        ///the clip preferences to stay the same throughout the action.
        stat = effect->paramInstanceChangedAction(k->getOriginalName(), ofxReason, (OfxTime)time, renderScale);
    } else {
        ///This action as all the overlay interacts actions can trigger recursive actions, such as
        ///getClipPreferences() so we don't take the clips preferences lock for read here otherwise we would
        ///create a deadlock. This code then assumes that the instance changed action of the plug-in doesn't require
        ///the clip preferences to stay the same throughout the action.
        stat = effect->paramInstanceChangedAction(k->getOriginalName(), ofxReason, (OfxTime)time, renderScale);
    }

    if ( (stat != kOfxStatOK) && (stat != kOfxStatReplyDefault) ) {
        return false;
    }
    Q_UNUSED(stat);

    return true;
} // knobChanged

void
OfxEffectInstance::beginKnobsValuesChanged(ValueChangedReasonEnum reason)
{
    if (!_imp->initialized) {
        return;
    }

    SET_CAN_SET_VALUE(true);
    ///This action as all the overlay interacts actions can trigger recursive actions, such as
    ///getClipPreferences() so we don't take the clips preferences lock for read here otherwise we would
    ///create a deadlock. This code then assumes that the instance changed action of the plug-in doesn't require
    ///the clip preferences to stay the same throughout the action.
    ignore_result( effectInstance()->beginInstanceChangedAction( natronValueChangedReasonToOfxValueChangedReason(reason) ) );
}

void
OfxEffectInstance::endKnobsValuesChanged(ValueChangedReasonEnum reason)
{
    if (!_imp->initialized) {
        return;
    }

    SET_CAN_SET_VALUE(true);
    ///This action as all the overlay interacts actions can trigger recursive actions, such as
    ///getClipPreferences() so we don't take the clips preferences lock for read here otherwise we would
    ///create a deadlock. This code then assumes that the instance changed action of the plug-in doesn't require
    ///the clip preferences to stay the same throughout the action.
    ignore_result( effectInstance()->endInstanceChangedAction( natronValueChangedReasonToOfxValueChangedReason(reason) ) );
}

void
OfxEffectInstance::purgeCaches()
{
    // The kOfxActionPurgeCaches is an action that may be passed to a plug-in instance from time to time in low memory situations. Instances receiving this action should destroy any data structures they may have and release the associated memory, they can later reconstruct this from the effect's parameter set and associated information. http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#kOfxActionPurgeCaches
    OfxStatus stat;
    {
        SET_CAN_SET_VALUE(false);
        assert(_imp->effect);

        ///Take the preferences lock so that it cannot be modified throughout the action.
        QReadLocker preferencesLocker(&_imp->preferencesLock);
        assert(_imp->effect);
        stat =  _imp->effect->purgeCachesAction();

        assert(stat == kOfxStatOK || stat == kOfxStatReplyDefault);
    }
    // The kOfxActionSyncPrivateData action is called when a plugin should synchronise any private data structures to its parameter set. This generally occurs when an effect is about to be saved or copied, but it could occur in other situations as well. http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#kOfxActionSyncPrivateData

    {
        RECURSIVE_ACTION();
        SET_CAN_SET_VALUE(true);
        assert(_imp->effect);

        ///This action as all the overlay interacts actions can trigger recursive actions, such as
        ///getClipPreferences() so we don't take the clips preferences lock for read here otherwise we would
        ///create a deadlock. This code then assumes that the instance changed action of the plug-in doesn't require
        ///the clip preferences to stay the same throughout the action.
        stat =  _imp->effect->syncPrivateDataAction();
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
    if (!effectInstance()) {
        return false;
    }
    // This is a dynamic property since OFX 1.4, so get the prop from the instance, not the descriptor.
    // The descriptor may have it set to false for backward compatibility with hosts that do not support
    // this dynamic property.
    if ( !effectInstance()->supportsTiles() ) {
        return false;
    }

    OFX::Host::ImageEffect::ClipInstance* outputClip =  effectInstance()->getClip(kOfxImageEffectOutputClipName);

    return outputClip->supportsTiles();
}

PluginOpenGLRenderSupport
OfxEffectInstance::supportsOpenGLRender() const
{
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

void
OfxEffectInstance::onEnableOpenGLKnobValueChanged(bool activated)
{
    const Plugin* p = getNode()->getPlugin();
    if (p->getPluginOpenGLRenderSupport() == ePluginOpenGLRenderSupportYes) {
        // The property may only change if the plug-in has the property set to yes on the descriptor
        if (activated) {
            effectInstance()->getProps().setStringProperty(kOfxImageEffectPropOpenGLRenderSupported, "true");
        } else {
            effectInstance()->getProps().setStringProperty(kOfxImageEffectPropOpenGLRenderSupported, "false");
        }
    }
}

bool
OfxEffectInstance::supportsMultiResolution() const
{
    // first, check the descriptor, then the instance
    if (!effectInstance()) {
        return false;
    }
    return effectInstance()->getDescriptor().supportsMultiResolution() && effectInstance()->supportsMultiResolution();
}

void
OfxEffectInstance::beginEditKnobs()
{
    ///Take the preferences lock so that it cannot be modified throughout the action.
    QReadLocker preferencesLocker(&_imp->preferencesLock);

    effectInstance()->beginInstanceEditAction();
}

void
OfxEffectInstance::syncPrivateData_other_thread()
{
    if ( getApp()->isShowingDialog() ) {
        /*
           We may enter a situation where a plug-in called EffectInstance::message to show a dialog
           and would block the main thread until the user would click OK but Qt would request a paintGL() on the viewer
           because of focus changes. This would end-up in the interact draw action being called whilst the message() function
           did not yet return and may in some plug-ins cause deadlocks (happens in all Genarts Sapphire plug-ins).
         */
        return;
    }
    Q_EMIT syncPrivateDataRequested();
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
                                         std::list<ImagePlaneDesc>* comps)
{
    if (inputNb >= 0) {
        OfxClipInstance* clip = getClipCorrespondingToInput(inputNb);
        assert(clip);
        const std::vector<std::string> & supportedComps = clip->getSupportedComponents();
        for (U32 i = 0; i < supportedComps.size(); ++i) {
            try {
                ImagePlaneDesc comp, pairedComp;
                ImagePlaneDesc::mapOFXComponentsTypeStringToPlanes(supportedComps[i], &comp, &pairedComp);
                comps->push_back(ImagePlaneDesc::mapNCompsToColorPlane(comp.getNumComponents()));
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
                ImagePlaneDesc comp, pairedComp;
                ImagePlaneDesc::mapOFXComponentsTypeStringToPlanes(supportedComps[i], &comp, &pairedComp);
                comps->push_back(ImagePlaneDesc::mapNCompsToColorPlane(comp.getNumComponents()));
            } catch (const std::runtime_error &e) {
                // ignore unsupported components
            }
        }
    }
}

void
OfxEffectInstance::addSupportedBitDepth(std::list<ImageBitDepthEnum>* depths) const
{
    const OFX::Host::Property::Set & prop = effectInstance()->getPlugin()->getDescriptor().getParamSetProps();
    int dim = prop.getDimension(kOfxImageEffectPropSupportedPixelDepths);

    for (int i = 0; i < dim; ++i) {
        const std::string & depth = prop.getStringProperty(kOfxImageEffectPropSupportedPixelDepths, i);
        // ignore unsupported bitdepth
        ImageBitDepthEnum bitDepth = OfxClipInstance::ofxDepthToNatronDepth(depth, false);
        if (bitDepth != eImageBitDepthNone) {
            depths->push_back(bitDepth);
        }
    }
}

void
OfxEffectInstance::getComponentsNeededAndProduced(double time,
                                                  ViewIdx view,
                                                  EffectInstance::ComponentsNeededMap* comps,
                                                  double* passThroughTime,
                                                  int* passThroughView,
                                                  int* passThroughInputNb)
{
    OfxStatus stat;
    {
        SET_CAN_SET_VALUE(false);


        ClipsThreadStorageSetter clipSetter(effectInstance(),
                                            view,
                                            0);
        OFX::Host::ImageEffect::ComponentsMap compMap;
        OFX::Host::ImageEffect::ClipInstance* ptClip = 0;
        OfxTime ptTime;
        stat = effectInstance()->getClipComponentsAction( (OfxTime)time, view, compMap, ptClip, ptTime, *passThroughView );
        if (stat != kOfxStatFailed) {
            *passThroughInputNb = -1;
            if (ptClip) {
                OfxClipInstance* clip = dynamic_cast<OfxClipInstance*>(ptClip);
                if (clip) {
                    *passThroughInputNb = clip->getInputNb();
                }
            }
            *passThroughTime = (SequenceTime)ptTime;

            for (OFX::Host::ImageEffect::ComponentsMap::iterator it = compMap.begin(); it != compMap.end(); ++it) {
                OfxClipInstance* clip = dynamic_cast<OfxClipInstance*>(it->first);
                assert(clip);
                if (clip) {
                    int index = clip->getInputNb();
                    std::list<ImagePlaneDesc>& compNeeded = (*comps)[index];
                    for (std::list<std::string>::iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {

                        ImagePlaneDesc plane;
                        if ((*it2) == kFnOfxImagePlaneColour) {
                            plane = ImagePlaneDesc::mapNCompsToColorPlane(getMetadataNComps(index));
                        } else {
                            plane = ImagePlaneDesc::mapOFXPlaneStringToPlane(*it2);
                        }
                        if (plane.getNumComponents() > 0) {
                            compNeeded.push_back(plane);
                        }
                    }
                }
            }
        }
    } // ClipsThreadStorageSetter
}

bool
OfxEffectInstance::isMultiPlanar() const
{
    return _imp->multiplanar;
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
    return effectInstance() ? effectInstance()->isViewAware() : false;
}

EffectInstance::ViewInvarianceLevel
OfxEffectInstance::isViewInvariant() const
{
    int inv = effectInstance() ? effectInstance()->getViewInvariance() : 0;

    if (inv == 0) {
        return eViewInvarianceAllViewsVariant;
    } else if (inv == 1) {
        return eViewInvarianceOnlyPassThroughPlanesVariant;
    } else {
        assert(inv == 2);

        return eViewInvarianceAllViewsInvariant;
    }
}

SequentialPreferenceEnum
OfxEffectInstance::getSequentialPreference() const
{
    return _imp->sequentialPref;
}

const std::string &
OfxEffectInstance::ofxGetOutputPremultiplication() const
{
    return OfxClipInstance::natronsPremultToOfxPremult( getPremult() );
}

bool
OfxEffectInstance::getCanTransform() const
{   //use OFX_EXTENSIONS_NUKE
    if (!effectInstance()) {
        return false;
    }
    return effectInstance()->canTransform();
}

bool
OfxEffectInstance::getInputsHoldingTransform(std::list<int>* inputs) const
{
    return effectInstance()->getInputsHoldingTransform(inputs);
}

StatusEnum
OfxEffectInstance::getTransform(double time,
                                const RenderScale & renderScale, //< the plug-in accepted scale
                                bool draftRender,
                                ViewIdx view,
                                EffectInstancePtr* inputToTransform,
                                Transform::Matrix3x3* transform)
{
    const std::string field = kOfxImageFieldNone; // TODO: support interlaced data
    std::string clipName;
    double tmpTransform[9];
    OfxStatus stat;
    {
        SET_CAN_SET_VALUE(false);


        ClipsThreadStorageSetter clipSetter( effectInstance(),
                                             view,
                                             Image::getLevelFromScale(renderScale.x) );

        try {
            stat = effectInstance()->getTransformAction( (OfxTime)time, field, renderScale, draftRender, view, clipName, tmpTransform );
        } catch (...) {
            return eStatusFailed;
        }

        if (stat == kOfxStatReplyDefault) {
            return eStatusReplyDefault;
        } else if (stat == kOfxStatFailed) {
            return eStatusFailed;
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
        return eStatusFailed;
    }
    *inputToTransform = natronClip->getAssociatedNode();
    if (!*inputToTransform) {
        return eStatusFailed;
    }

    return eStatusOK;
}

bool
OfxEffectInstance::doesTemporalClipAccess() const
{
    // first, check the descriptor, then the instance
    return _imp->doesTemporalAccess;
}

bool
OfxEffectInstance::isHostChannelSelectorSupported(bool* defaultR,
                                                  bool* defaultG,
                                                  bool* defaultB,
                                                  bool* defaultA) const
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
        qDebug() << getScriptName_mt_safe().c_str() << "Invalid value given to property" << kNatronOfxImageEffectPropChannelSelector << "defaulting to RGBA checked";
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
    for (std::size_t i = 0; i < _imp->clipsInfos.size(); ++i) {
        if (_imp->clipsInfos[i].clip == clip) {
            return (int)i;
        }
    }
    if (clip == _imp->outputClip) {
        return -1;
    }

    return 0;
}

void
OfxEffectInstance::onScriptNameChanged(const std::string& fullyQualifiedName)
{
    if (!_imp->effect) {
        return;
    }
    std::size_t foundLastDot = fullyQualifiedName.find_last_of('.');
    std::string scriptname, groupprefix, appID;
    if (foundLastDot == std::string::npos) {
        // No prefix
        scriptname = fullyQualifiedName;
    } else {
        if ( (foundLastDot + 1) < fullyQualifiedName.size() ) {
            scriptname = fullyQualifiedName.substr(foundLastDot + 1);
        }
        groupprefix = fullyQualifiedName.substr(0, foundLastDot);
    }
    appID = getApp()->getAppIDString();
    assert(_imp->effect);
    if ( NATRON_PYTHON_NAMESPACE::isKeyword(scriptname) ) {
        throw std::runtime_error(scriptname + " is a Python keyword");
    }

    _imp->effect->getProps().setStringProperty(kNatronOfxImageEffectPropProjectId, appID);
    _imp->effect->getProps().setStringProperty(kNatronOfxImageEffectPropGroupId, groupprefix);
    _imp->effect->getProps().setStringProperty(kNatronOfxImageEffectPropInstanceId, scriptname);
}

bool
OfxEffectInstance::supportsConcurrentOpenGLRenders() const
{
    // By default OpenFX OpenGL render suite does not support concurrent OpenGL renders.
    QMutexLocker k(&_imp->supportsConcurrentGLRendersMutex);

    return _imp->supportsConcurrentGLRenders;
}

StatusEnum
OfxEffectInstance::attachOpenGLContext(EffectInstance::OpenGLContextEffectDataPtr* data)
{
    OfxGLContextEffectDataPtr ofxData( new OfxGLContextEffectData() );

    *data = ofxData;
    void* ofxGLData = 0;
    OfxStatus stat = effectInstance()->contextAttachedAction(ofxGLData);

    // If the plug-in use the Natron property kNatronOfxImageEffectPropOpenGLContextData, that means it can handle
    // concurrent OpenGL renders.
    if (ofxGLData) {
        ofxData->setDataHandle(ofxGLData);
        QMutexLocker k(&_imp->supportsConcurrentGLRendersMutex);
        if (!_imp->supportsConcurrentGLRenders) {
            _imp->supportsConcurrentGLRenders = true;
        }
    }
    if (stat == kOfxStatFailed) {
        return eStatusFailed;
    } else if (stat == kOfxStatErrMemory) {
        return eStatusOutOfMemory;
    } else if (stat == kOfxStatReplyDefault) {
        return eStatusReplyDefault;
    } else {
        return eStatusOK;
    }
}

StatusEnum
OfxEffectInstance::dettachOpenGLContext(const EffectInstance::OpenGLContextEffectDataPtr& data)
{
    OfxGLContextEffectData* isOfxData = dynamic_cast<OfxGLContextEffectData*>( data.get() );
    void* ofxGLData = isOfxData ? isOfxData->getDataHandle() : 0;
    OfxStatus stat = effectInstance()->contextDetachedAction(ofxGLData);

    if (isOfxData) {
        // the context data can not be used anymore, reset it.
        isOfxData->setDataHandle(NULL);
    }
    if (stat == kOfxStatFailed) {
        return eStatusFailed;
    } else if (stat == kOfxStatErrMemory) {
        return eStatusOutOfMemory;
    } else if (stat == kOfxStatReplyDefault) {
        return eStatusReplyDefault;
    } else {
        return eStatusOK;
    }
}

void
OfxEffectInstance::onInteractViewportSelectionCleared()
{
    KnobIntPtr k = _imp->selectionRectangleStateKnob.lock();
    if (!k) {
        return;
    }
    double propV[4] = {0, 0, 0, 0};
    effectInstance()->getProps().setDoublePropertyN(kNatronOfxImageEffectSelectionRectangle, propV, 4);

    k->setValue(0);
}


void
OfxEffectInstance::onInteractViewportSelectionUpdated(const RectD& rectangle, bool onRelease)
{
    KnobIntPtr k = _imp->selectionRectangleStateKnob.lock();
    if (!k) {
        return;
    }
    double propV[4] = {rectangle.x1, rectangle.y1, rectangle.x2, rectangle.y2};
    effectInstance()->getProps().setDoublePropertyN(kNatronOfxImageEffectSelectionRectangle, propV, 4);
    k->setValue(onRelease ? 2 : 1);
}

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_OfxEffectInstance.cpp"
