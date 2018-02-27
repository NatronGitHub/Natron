/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#include "EffectInstancePrivate.h"

#include <cassert>
#include <stdexcept>
#include <sstream> // stringstream

#include "Engine/AppInstance.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/ViewIdx.h"


NATRON_NAMESPACE_ENTER

ActionsCache::ActionsCacheInstance::ActionsCacheInstance()
    : _hash(0)
    , _timeDomain()
    , _timeDomainSet(false)
    , _identityCache()
    , _rodCache()
    , _framesNeededCache()
{
}

std::list<ActionsCache::ActionsCacheInstance>::iterator
ActionsCache::createActionCacheInternal(U64 newHash)
{
    if (_instances.size() >= _maxInstances) {
        _instances.pop_front();
    }
    ActionsCacheInstance cache;
    cache._hash = newHash;

    return _instances.insert(_instances.end(), cache);
}

ActionsCache::ActionsCacheInstance &
ActionsCache::getOrCreateActionCache(U64 newHash)
{
    std::list<ActionsCacheInstance>::iterator found = _instances.end();

    for (std::list<ActionsCacheInstance>::iterator it = _instances.begin(); it != _instances.end(); ++it) {
        if (it->_hash == newHash) {
            found = it;
            break;
        }
    }
    if ( found == _instances.end() ) {
        found = createActionCacheInternal(newHash);
    }
    assert( found != _instances.end() );

    return *found;
}

ActionsCache::ActionsCache(int maxAvailableHashes)
    : _cacheMutex()
    , _instances()
    , _maxInstances( (std::size_t)maxAvailableHashes )
{
}

void
ActionsCache::clearAll()
{
    QMutexLocker l(&_cacheMutex);

    _instances.clear();
}

void
ActionsCache::invalidateAll(U64 newHash)
{
    QMutexLocker l(&_cacheMutex);

    createActionCacheInternal(newHash);
}

bool
ActionsCache::getIdentityResult(U64 hash,
                                double time,
                                ViewIdx view,
                                int* inputNbIdentity,
                                ViewIdx *inputView,
                                double* identityTime)
{
    QMutexLocker l(&_cacheMutex);

    for (std::list<ActionsCacheInstance>::iterator it = _instances.begin(); it != _instances.end(); ++it) {
        if (it->_hash == hash) {
            ActionKey key;
            key.time = time;
            key.view = view;
            key.mipMapLevel = 0;

            IdentityCacheMap::const_iterator found = it->_identityCache.find(key);
            if ( found != it->_identityCache.end() ) {
                *inputNbIdentity = found->second.inputIdentityNb;
                *identityTime = found->second.inputIdentityTime;
                *inputView = found->second.inputView;

                return true;
            }

            return false;
        }
    }

    return false;
}

void
ActionsCache::setIdentityResult(U64 hash,
                                double time,
                                ViewIdx view,
                                int inputNbIdentity,
                                ViewIdx inputView,
                                double identityTime)
{
    QMutexLocker l(&_cacheMutex);
    ActionsCacheInstance & cache = getOrCreateActionCache(hash);
    ActionKey key;

    key.time = time;
    key.view = view;
    key.mipMapLevel = 0;

    IdentityResults & v = cache._identityCache[key];
    v.inputIdentityNb = inputNbIdentity;
    v.inputIdentityTime = identityTime;
    v.inputView = inputView;
}

bool
ActionsCache::getComponentsNeededResults(U64 hash, double time, ViewIdx view, EffectInstance::ComponentsNeededMap* neededComps, std::bitset<4> *processChannels, bool *processAll,
                                         std::list<ImagePlaneDesc> *passThroughPlanes, int* passThroughInputNb, ViewIdx *passThroughView, double* passThroughTime)
{
    QMutexLocker l(&_cacheMutex);

    for (std::list<ActionsCacheInstance>::iterator it = _instances.begin(); it != _instances.end(); ++it) {
        if (it->_hash == hash) {
            ActionKey key;
            key.time = time;
            key.view = view;
            key.mipMapLevel = 0;

            ComponentsNeededCacheMap::const_iterator found = it->_componentsNeededCache.find(key);
            if ( found != it->_componentsNeededCache.end() ) {
                *passThroughInputNb = found->second.passThroughInputNb;
                *passThroughTime = found->second.passThroughTime;
                *passThroughView = found->second.passThroughView;
                *neededComps = found->second.neededComps;
                *processChannels = found->second.processChannels;
                *processAll = found->second.processAll;
                *passThroughPlanes = found->second.passThroughPlanes;
                return true;
            }

            return false;
        }
    }
    
    return false;
}

void
ActionsCache::setComponentsNeededResults(U64 hash, double time, ViewIdx view, const EffectInstance::ComponentsNeededMap& neededComps,
                                         std::bitset<4> processChannels,
                                         bool processAll,
                                         const std::list<ImagePlaneDesc>& passThroughPlanes, int passThroughInputNb, ViewIdx passThroughView, double passThroughTime)
{
    QMutexLocker l(&_cacheMutex);
    ActionsCacheInstance & cache = getOrCreateActionCache(hash);
    ActionKey key;

    key.time = time;
    key.view = view;
    key.mipMapLevel = 0;

    ComponentsNeededResults & v = cache._componentsNeededCache[key];
    v.neededComps = neededComps;
    v.passThroughTime = passThroughTime;
    v.passThroughView = passThroughView;
    v.passThroughInputNb = passThroughInputNb;
    v.processChannels = processChannels;
    v.processAll = processAll;
    v.passThroughPlanes = passThroughPlanes;

}

bool
ActionsCache::getRoDResult(U64 hash,
                           double time,
                           ViewIdx view,
                           unsigned int mipMapLevel,
                           RectD* rod)
{
    QMutexLocker l(&_cacheMutex);

    for (std::list<ActionsCacheInstance>::iterator it = _instances.begin(); it != _instances.end(); ++it) {
        if (it->_hash == hash) {
            ActionKey key;
            key.time = time;
            key.view = view;
            key.mipMapLevel = mipMapLevel;

            RoDCacheMap::const_iterator found = it->_rodCache.find(key);
            if ( found != it->_rodCache.end() ) {
                *rod = found->second;

                return true;
            }

            return false;
        }
    }

    return false;
}

void
ActionsCache::setRoDResult(U64 hash,
                           double time,
                           ViewIdx view,
                           unsigned int mipMapLevel,
                           const RectD & rod)
{
    QMutexLocker l(&_cacheMutex);
    ActionsCacheInstance & cache = getOrCreateActionCache(hash);
    ActionKey key;

    key.time = time;
    key.view = view;
    key.mipMapLevel = mipMapLevel;

    cache._rodCache[key] = rod;
}

bool
ActionsCache::getFramesNeededResult(U64 hash,
                                    double time,
                                    ViewIdx view,
                                    unsigned int mipMapLevel,
                                    FramesNeededMap* framesNeeded)
{
    QMutexLocker l(&_cacheMutex);

    for (std::list<ActionsCacheInstance>::iterator it = _instances.begin(); it != _instances.end(); ++it) {
        if (it->_hash == hash) {
            ActionKey key;
            key.time = time;
            key.view = view;
            key.mipMapLevel = mipMapLevel;

            FramesNeededCacheMap::const_iterator found = it->_framesNeededCache.find(key);
            if ( found != it->_framesNeededCache.end() ) {
                *framesNeeded = found->second;

                return true;
            }

            return false;
        }
    }

    return false;
}

void
ActionsCache::setFramesNeededResult(U64 hash,
                                    double time,
                                    ViewIdx view,
                                    unsigned int mipMapLevel,
                                    const FramesNeededMap & framesNeeded)
{
    QMutexLocker l(&_cacheMutex);
    ActionsCacheInstance & cache = getOrCreateActionCache(hash);
    ActionKey key;

    key.time = time;
    key.view = view;
    key.mipMapLevel = mipMapLevel;

    cache._framesNeededCache[key] = framesNeeded;
}

bool
ActionsCache::getTimeDomainResult(U64 hash,
                                  double *first,
                                  double* last)
{
    QMutexLocker l(&_cacheMutex);

    for (std::list<ActionsCacheInstance>::iterator it = _instances.begin(); it != _instances.end(); ++it) {
        if ( (it->_hash == hash) && it->_timeDomainSet ) {
            *first = it->_timeDomain.min;
            *last = it->_timeDomain.max;

            return true;
        }
    }

    return false;
}

void
ActionsCache::setTimeDomainResult(U64 hash,
                                  double first,
                                  double last)
{
    QMutexLocker l(&_cacheMutex);
    ActionsCacheInstance & cache = getOrCreateActionCache(hash);

    cache._timeDomainSet = true;
    cache._timeDomain.min = first;
    cache._timeDomain.max = last;
}

EffectInstance::RenderArgs::RenderArgs()
    : rod()
    , regionOfInterestResults()
    , renderWindowPixel()
    , time(0)
    , view(0)
    , validArgs(false)
    , isIdentity(false)
    , identityTime(0)
    , identityInput()
    , inputImages()
    , outputPlanes()
    , outputPlaneBeingRendered()
    , firstFrame(0)
    , lastFrame(0)
    , transformRedirections()
    , isDoingOpenGLRender(false)
{
}

EffectInstance::RenderArgs::RenderArgs(const RenderArgs & o)
    : rod(o.rod)
    , regionOfInterestResults(o.regionOfInterestResults)
    , renderWindowPixel(o.renderWindowPixel)
    , time(o.time)
    , view(o.view)
    , validArgs(o.validArgs)
    , isIdentity(o.isIdentity)
    , identityTime(o.identityTime)
    , identityInput(o.identityInput)
    , inputImages(o.inputImages)
    , outputPlanes(o.outputPlanes)
    , outputPlaneBeingRendered(o.outputPlaneBeingRendered)
    , firstFrame(o.firstFrame)
    , lastFrame(o.lastFrame)
    , transformRedirections(o.transformRedirections)
    , isDoingOpenGLRender(o.isDoingOpenGLRender)
{
}

EffectInstance::Implementation::Implementation(EffectInstance* publicInterface)
    : _publicInterface(publicInterface)
    , tlsData()
    , duringInteractActionMutex()
    , duringInteractAction(false)
    , pluginMemoryChunksMutex()
    , pluginMemoryChunks()
    , supportsRenderScale(eSupportsMaybe)
    , actionsCache()
#if NATRON_ENABLE_TRIMAP
    , imagesBeingRenderedMutex()
    , imagesBeingRendered()
#endif
    , overlaySlaves()
    , metadataMutex()
    , metadata()
    , runningClipPreferences(false)
    , overlaysViewport(0)
    , attachedContextsMutex(QMutex::Recursive)
    , attachedContexts()
    , mainInstance(0)
    , isDoingInstanceSafeRender(false)
    , renderClonesMutex()
    , renderClonesPool()
    , mustSyncPrivateData(false)
{
    tlsData = boost::make_shared<TLSHolder<EffectTLSData> >();
    actionsCache = boost::make_shared<ActionsCache>(appPTR->getHardwareIdealThreadCount() * 2);
}

EffectInstance::Implementation::Implementation(const Implementation& other)
: _publicInterface(0)
, tlsData(other.tlsData)
, duringInteractActionMutex()
, duringInteractAction(other.duringInteractAction)
, pluginMemoryChunksMutex()
, pluginMemoryChunks()
, supportsRenderScale(other.supportsRenderScale)
, actionsCache(other.actionsCache)
#if NATRON_ENABLE_TRIMAP
, imagesBeingRenderedMutex()
, imagesBeingRendered()
#endif
, overlaySlaves(other.overlaySlaves)
, metadataMutex()
, metadata(other.metadata)
, runningClipPreferences(other.runningClipPreferences)
, overlaysViewport(other.overlaysViewport)
, attachedContextsMutex(QMutex::Recursive)
, attachedContexts()
, mainInstance(other._publicInterface)
, isDoingInstanceSafeRender(false)
, renderClonesMutex()
, renderClonesPool()
{

}

void
EffectInstance::Implementation::runChangedParamCallback(KnobI* k,
                                                        bool userEdited,
                                                        const std::string & callback)
{
    std::vector<std::string> args;
    std::string error;

    if ( !k || (k->getName() == "onParamChanged") ) {
        return;
    }
    try {
        NATRON_PYTHON_NAMESPACE::getFunctionArguments(callback, &error, &args);
    } catch (const std::exception& e) {
        _publicInterface->getApp()->appendToScriptEditor( tr("Failed to run onParamChanged callback: %1").arg( QString::fromUtf8( e.what() ) ).toStdString() );

        return;
    }

    if ( !error.empty() ) {
        _publicInterface->getApp()->appendToScriptEditor( tr("Failed to run onParamChanged callback: %1").arg( QString::fromUtf8( error.c_str() ) ).toStdString() );

        return;
    }

    std::string signatureError;
    signatureError.append( tr("The param changed callback supports the following signature(s):").toStdString() );
    signatureError.append("\n- callback(thisParam,thisNode,thisGroup,app,userEdited)");
    if (args.size() != 5) {
        _publicInterface->getApp()->appendToScriptEditor( tr("Failed to run onParamChanged callback: %1").arg( QString::fromUtf8( signatureError.c_str() ) ).toStdString() );

        return;
    }

    if ( ( (args[0] != "thisParam") || (args[1] != "thisNode") || (args[2] != "thisGroup") || (args[3] != "app") || (args[4] != "userEdited") ) ) {
        _publicInterface->getApp()->appendToScriptEditor( tr("Failed to run onParamChanged callback: %1").arg( QString::fromUtf8( signatureError.c_str() ) ).toStdString() );

        return;
    }

    std::string appID = _publicInterface->getApp()->getAppIDString();

    assert(k);
    std::string thisNodeVar = appID + ".";
    thisNodeVar.append( _publicInterface->getNode()->getFullyQualifiedName() );

    boost::shared_ptr<NodeCollection> collection = _publicInterface->getNode()->getGroup();
    assert(collection);
    if (!collection) {
        return;
    }

    std::string thisGroupVar;
    NodeGroup* isParentGrp = dynamic_cast<NodeGroup*>( collection.get() );
    if (isParentGrp) {
        std::string nodeName = isParentGrp->getNode()->getFullyQualifiedName();
        std::string nodeFullName = appID + "." + nodeName;
        thisGroupVar = nodeFullName;
    } else {
        thisGroupVar = appID;
    }

    bool alreadyDefined = false;
    PyObject* nodeObj = NATRON_PYTHON_NAMESPACE::getAttrRecursive(thisNodeVar, NATRON_PYTHON_NAMESPACE::getMainModule(), &alreadyDefined);
    if (!nodeObj || !alreadyDefined) {
        return;
    }

    if (!PyObject_HasAttrString( nodeObj, k->getName().c_str() ) ) {
        return;
    }

    std::stringstream ss;
    ss << callback << "(" << thisNodeVar << "." << k->getName() << "," << thisNodeVar << "," << thisGroupVar << "," << appID
       << ",";
    if (userEdited) {
        ss << "True";
    } else {
        ss << "False";
    }
    ss << ")\n";

    std::string script = ss.str();
    std::string err;
    std::string output;
    if ( !NATRON_PYTHON_NAMESPACE::interpretPythonScript(script, &err, &output) ) {
        _publicInterface->getApp()->appendToScriptEditor( tr("Failed to execute onParamChanged callback: %1").arg( QString::fromUtf8( err.c_str() ) ).toStdString() );
    } else {
        if ( !output.empty() ) {
            _publicInterface->getApp()->appendToScriptEditor(output);
        }
    }
} // EffectInstance::Implementation::runChangedParamCallback

void
EffectInstance::Implementation::setDuringInteractAction(bool b)
{
    QWriteLocker l(&duringInteractActionMutex);

    duringInteractAction = b;
}

#if NATRON_ENABLE_TRIMAP
void
EffectInstance::Implementation::markImageAsBeingRendered(const boost::shared_ptr<Image> & img, const RectI& roi, std::list<RectI>* restToRender, bool *renderedElsewhere)
{
    if ( !img->usesBitMap() ) {
        return;
    }

    QMutexLocker k(&imagesBeingRenderedMutex);
    IBRMap::iterator found = imagesBeingRendered.find(img);
    if ( found != imagesBeingRendered.end() ) {
        IBRPtr ibr = found->second;
        k.unlock(); // imagesBeingRenderedMutex
        QMutexLocker k2(&ibr->lock);
        ++(ibr->refCount);
        img->getRestToRender_trimap(roi, *restToRender, renderedElsewhere);
        for (std::list<RectI>::const_iterator it = restToRender->begin(); it!=restToRender->end();++it) {
            img->markForRendering(*it);
        }
    } else {
        IBRPtr ibr = boost::make_shared<Implementation::ImageBeingRendered>();
        ++ibr->refCount;
        std::pair<IBRMap::iterator, bool> ok = imagesBeingRendered.insert( std::make_pair(img, ibr) );
        assert(ok.second);
        k.unlock(); // imagesBeingRenderedMutex
        QMutexLocker k2(&ibr->lock);
        img->getRestToRender_trimap(roi, *restToRender, renderedElsewhere);
        for (std::list<RectI>::const_iterator it = restToRender->begin(); it!=restToRender->end();++it) {
            img->markForRendering(*it);
        }
    }
}

bool
EffectInstance::Implementation::waitForImageBeingRenderedElsewhere(const RectI & roi,
                                                                            const boost::shared_ptr<Image> & img)
{
    if ( !img->usesBitMap() ) {
        return true;
    }
    IBRPtr ibr;
    QMutexLocker k(&imagesBeingRenderedMutex);
    IBRMap::iterator found = imagesBeingRendered.find(img);
    if( found != imagesBeingRendered.end() ) {
        ibr = found->second;
    }
    if (!ibr) {
        return true;
    }
    k.unlock(); // imagesBeingRenderedMutex
    std::list<RectI> restToRender;
    bool isBeingRenderedElseWhere = false;
    img->getRestToRender_trimap(roi, restToRender, &isBeingRenderedElseWhere);

    bool ab = _publicInterface->aborted();

    QMutexLocker kk(&ibr->lock);
    while (!ab && isBeingRenderedElseWhere && !ibr->failed && ibr->refCount > 1) {
        ibr->cond.wait(&ibr->lock, 50);
        restToRender.clear();
        isBeingRenderedElseWhere = false;
        img->getRestToRender_trimap(roi, restToRender, &isBeingRenderedElseWhere);
        ab = _publicInterface->aborted();
    }
    ///Everything should be rendered now unless we are aborted
    return restToRender.empty() && !ibr->failed && !ab;
}

void
EffectInstance::Implementation::unmarkImageAsBeingRendered(const boost::shared_ptr<Image> & img,
                                                           const std::list<RectI>& rects,
                                                           bool renderFailed)
{
    if ( !img->usesBitMap() ) {
        return;
    }
    IBRPtr ibr;
    QMutexLocker k(&imagesBeingRenderedMutex);
    IBRMap::iterator found = imagesBeingRendered.find(img);
    if( found != imagesBeingRendered.end() ) {
        ibr = found->second;
    }
    if (!ibr) {
        return;
    }
    k.unlock(); // imagesBeingRenderedMutex
    QMutexLocker kk(&ibr->lock);
    if (renderFailed) {
        ibr->failed = true;
    }
    for (std::list<RectI>::const_iterator it = rects.begin(); it!=rects.end();++it) {
        if (renderFailed) {
            img->clearBitmap(*it);
        } else {
            img->markForRendered(*it);
        }
    }

    --ibr->refCount;
    ibr->cond.wakeAll();
    if (!ibr->refCount) {
        kk.unlock(); // < unlock before erase which is going to delete the lock
        k.relock(); // imagesBeingRenderedMutex
        IBRMap::iterator found = imagesBeingRendered.find(img);
        if( found != imagesBeingRendered.end() ) {
            imagesBeingRendered.erase(found);
        }
    }
}

#endif \
    // if NATRON_ENABLE_TRIMAP


EffectInstance::Implementation::ScopedRenderArgs::ScopedRenderArgs(const EffectDataTLSPtr& tlsData,
                                                                   const RectD & rod,
                                                                   const RectI & renderWindow,
                                                                   double time,
                                                                   ViewIdx view,
                                                                   bool isIdentity,
                                                                   double identityTime,
                                                                   const EffectInstPtr& identityInput,
                                                                   const boost::shared_ptr<ComponentsNeededMap>& compsNeeded,
                                                                   const EffectInstance::InputImagesMap& inputImages,
                                                                   const RoIMap & roiMap,
                                                                   int firstFrame,
                                                                   int lastFrame,
                                                                   bool isDoingOpenGLRender)
    : tlsData(tlsData)
{
    tlsData->currentRenderArgs.rod = rod;
    tlsData->currentRenderArgs.renderWindowPixel = renderWindow;
    tlsData->currentRenderArgs.time = time;
    tlsData->currentRenderArgs.view = view;
    tlsData->currentRenderArgs.isIdentity = isIdentity;
    tlsData->currentRenderArgs.identityTime = identityTime;
    tlsData->currentRenderArgs.identityInput = identityInput;
    tlsData->currentRenderArgs.compsNeeded = compsNeeded;
    tlsData->currentRenderArgs.inputImages.insert( inputImages.begin(), inputImages.end() );
    tlsData->currentRenderArgs.regionOfInterestResults = roiMap;
    tlsData->currentRenderArgs.firstFrame = firstFrame;
    tlsData->currentRenderArgs.lastFrame = lastFrame;
    tlsData->currentRenderArgs.isDoingOpenGLRender = isDoingOpenGLRender;

    tlsData->currentRenderArgs.validArgs = true;
}

EffectInstance::Implementation::ScopedRenderArgs::ScopedRenderArgs(const EffectDataTLSPtr& tlsData,
                                                                   const EffectDataTLSPtr& otherThreadData)
    : tlsData(tlsData)
{
    tlsData->currentRenderArgs = otherThreadData->currentRenderArgs;
}

EffectInstance::Implementation::ScopedRenderArgs::~ScopedRenderArgs()
{
    assert(tlsData);
    tlsData->currentRenderArgs.outputPlanes.clear();
    tlsData->currentRenderArgs.inputImages.clear();
    tlsData->currentRenderArgs.validArgs = false;
}

void
EffectInstance::Implementation::addInputImageTempPointer(int inputNb,
                                                         const boost::shared_ptr<Image> & img)
{
    EffectDataTLSPtr tls = tlsData->getTLSData();

    if (!tls) {
        return;
    }
    tls->currentRenderArgs.inputImages[inputNb].push_back(img);
}

void
EffectInstance::Implementation::clearInputImagePointers()
{
    EffectDataTLSPtr tls = tlsData->getTLSData();

    if (!tls) {
        return;
    }
    tls->currentRenderArgs.inputImages.clear();
}

NATRON_NAMESPACE_EXIT


