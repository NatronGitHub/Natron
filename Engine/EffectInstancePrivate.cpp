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

#include "EffectInstancePrivate.h"

#include "Engine/AppInstance.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h"


using namespace Natron;


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
        , _maxInstances((std::size_t)maxAvailableHashes)
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
                                int view,
                                unsigned int mipMapLevel,
                                int* inputNbIdentity,
                                double* identityTime)
{
    QMutexLocker l(&_cacheMutex);

    for (std::list<ActionsCacheInstance>::iterator it = _instances.begin(); it != _instances.end(); ++it) {
        if (it->_hash == hash) {
            ActionKey key;
            key.time = time;
            key.view = view;
            key.mipMapLevel = mipMapLevel;

            IdentityCacheMap::const_iterator found = it->_identityCache.find(key);
            if ( found != it->_identityCache.end() ) {
                *inputNbIdentity = found->second.inputIdentityNb;
                *identityTime = found->second.inputIdentityTime;

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
                                int view,
                                unsigned int mipMapLevel,
                                int inputNbIdentity,
                                double identityTime)
{
    QMutexLocker l(&_cacheMutex);
    ActionsCacheInstance & cache = getOrCreateActionCache(hash);
    ActionKey key;

    key.time = time;
    key.view = view;
    key.mipMapLevel = mipMapLevel;

    IdentityResults & v = cache._identityCache[key];
    v.inputIdentityNb = inputNbIdentity;
    v.inputIdentityTime = identityTime;
}


bool
ActionsCache::getRoDResult(U64 hash,
                           double time,
                           int view,
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
                           int view,
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
                                    int view,
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
                                    int view,
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
    : _rod()
    , _regionOfInterestResults()
    , _renderWindowPixel()
    , _time(0)
    , _view(0)
    , _validArgs(false)
    , _isIdentity(false)
    , _identityTime(0)
    , _identityInputNb(-1)
    , _outputPlanes()
    , _outputPlaneBeingRendered()
    , _firstFrame(0)
    , _lastFrame(0)
{
}


EffectInstance::RenderArgs::RenderArgs(const RenderArgs & o)
    : _rod(o._rod)
    , _regionOfInterestResults(o._regionOfInterestResults)
    , _renderWindowPixel(o._renderWindowPixel)
    , _time(o._time)
    , _view(o._view)
    , _validArgs(o._validArgs)
    , _isIdentity(o._isIdentity)
    , _identityTime(o._identityTime)
    , _identityInputNb(o._identityInputNb)
    , _outputPlanes(o._outputPlanes)
    , _outputPlaneBeingRendered(o._outputPlaneBeingRendered)
    , _firstFrame(o._firstFrame)
    , _lastFrame(o._lastFrame)
{
}


EffectInstance::Implementation::Implementation(EffectInstance* publicInterface)
    : _publicInterface(publicInterface)
    , renderArgs()
    , frameRenderArgs()
    , beginEndRenderCount()
    , inputImages()
    , duringInteractActionMutex()
    , duringInteractAction(false)
    , pluginMemoryChunksMutex()
    , pluginMemoryChunks()
    , supportsRenderScale(eSupportsMaybe)
    , actionsCache(appPTR->getHardwareIdealThreadCount() * 2)
#if NATRON_ENABLE_TRIMAP
    , imagesBeingRenderedMutex()
    , imagesBeingRendered()
#endif
    , componentsAvailableMutex()
    , componentsAvailableDirty(true)
    , outputComponentsAvailable()
    , defaultClipPreferencesDataMutex()
    , clipPrefsData()
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
        Natron::getFunctionArguments(callback, &error, &args);
    } catch (const std::exception& e) {
        _publicInterface->getApp()->appendToScriptEditor(std::string("Failed to run onParamChanged callback: ")
                                                         + e.what());
        return;
    }
    if ( !error.empty() ) {
        _publicInterface->getApp()->appendToScriptEditor("Failed to run onParamChanged callback: " + error);

        return;
    }

    std::string signatureError;
    signatureError.append("The param changed callback supports the following signature(s):\n");
    signatureError.append("- callback(thisParam,thisNode,thisGroup,app,userEdited)");
    if (args.size() != 5) {
        _publicInterface->getApp()->appendToScriptEditor("Failed to run onParamChanged callback: " + signatureError);

        return;
    }

    if ( ( (args[0] != "thisParam") || (args[1] != "thisNode") || (args[2] != "thisGroup") || (args[3] != "app") || (args[4] != "userEdited") ) ) {
        _publicInterface->getApp()->appendToScriptEditor("Failed to run onParamChanged callback: " + signatureError);

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
    if ( !Natron::interpretPythonScript(script, &err, &output) ) {
        _publicInterface->getApp()->appendToScriptEditor(QObject::tr("Failed to execute onParamChanged callback: ").toStdString() + err);
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
EffectInstance::Implementation::markImageAsBeingRendered(const boost::shared_ptr<Natron::Image> & img)
{
    if ( !img->usesBitMap() ) {
        return;
    }
    QMutexLocker k(&imagesBeingRenderedMutex);
    IBRMap::iterator found = imagesBeingRendered.find(img);
    if ( found != imagesBeingRendered.end() ) {
        ++(found->second->refCount);
    } else {
        IBRPtr ibr(new Implementation::ImageBeingRendered);
        ++ibr->refCount;
        imagesBeingRendered.insert( std::make_pair(img, ibr) );
    }
}


bool
EffectInstance::Implementation::waitForImageBeingRenderedElsewhereAndUnmark(const RectI & roi,
                                                                            const boost::shared_ptr<Natron::Image> & img)
{
    if ( !img->usesBitMap() ) {
        return true;
    }
    IBRPtr ibr;
    {
        QMutexLocker k(&imagesBeingRenderedMutex);
        IBRMap::iterator found = imagesBeingRendered.find(img);
        assert( found != imagesBeingRendered.end() );
        ibr = found->second;
    }
    std::list<RectI> restToRender;
    bool isBeingRenderedElseWhere = false;
    img->getRestToRender_trimap(roi, restToRender, &isBeingRenderedElseWhere);

    bool ab = _publicInterface->aborted();
    {
        QMutexLocker kk(&ibr->lock);
        while (!ab && isBeingRenderedElseWhere && !ibr->renderFailed && ibr->refCount > 1) {
            ibr->cond.wait(&ibr->lock);
            isBeingRenderedElseWhere = false;
            img->getRestToRender_trimap(roi, restToRender, &isBeingRenderedElseWhere);
            ab = _publicInterface->aborted();
        }
    }

    ///Everything should be rendered now.
    bool hasFailed;
    {
        QMutexLocker k(&imagesBeingRenderedMutex);
        IBRMap::iterator found = imagesBeingRendered.find(img);
        assert( found != imagesBeingRendered.end() );

        QMutexLocker kk(&ibr->lock);
        hasFailed = ab || ibr->renderFailed || isBeingRenderedElseWhere;

        // If it crashes here, that is probably because ibr->refCount == 1, but in this case isBeingRenderedElseWhere should be false.
        // If this assert is triggered, please investigate, this is a serious bug in the trimap system.
        assert(ab || !isBeingRenderedElseWhere || ibr->renderFailed);
        --ibr->refCount;
        found->second->cond.wakeAll();
        if ( ( found != imagesBeingRendered.end() ) && !ibr->refCount ) {
            imagesBeingRendered.erase(found);
        }
    }

    return !hasFailed;
}


void
EffectInstance::Implementation::unmarkImageAsBeingRendered(const boost::shared_ptr<Natron::Image> & img,
                                                           bool renderFailed)
{
    if ( !img->usesBitMap() ) {
        return;
    }
    QMutexLocker k(&imagesBeingRenderedMutex);
    IBRMap::iterator found = imagesBeingRendered.find(img);
    assert( found != imagesBeingRendered.end() );

    QMutexLocker kk(&found->second->lock);
    if (renderFailed) {
        found->second->renderFailed = true;
    }
    found->second->cond.wakeAll();
    --found->second->refCount;
    if (!found->second->refCount) {
        kk.unlock(); // < unlock before erase which is going to delete the lock
        imagesBeingRendered.erase(found);
    }
}

#endif // if NATRON_ENABLE_TRIMAP



EffectInstance::Implementation::ScopedRenderArgs::ScopedRenderArgs(ThreadStorage<RenderArgs>* dst)
    : localData( &dst->localData() )
    , _dst(dst)
{
    assert(_dst);
}


EffectInstance::Implementation::ScopedRenderArgs::ScopedRenderArgs(ThreadStorage<RenderArgs>* dst,
                                                                   const RenderArgs & a)
    : localData( &dst->localData() )
    , _dst(dst)
{
    *localData = a;
    localData->_validArgs = true;
}


EffectInstance::Implementation::ScopedRenderArgs::~ScopedRenderArgs()
{
    assert( _dst->hasLocalData() );
    localData->_outputPlanes.clear();
    localData->_validArgs = false;
}


EffectInstance::RenderArgs &
EffectInstance::Implementation::ScopedRenderArgs::getLocalData()
{
    return *localData;
}


///Setup the first pass on thread-local storage.
///RoIMap and frame range are separated because those actions might need
///the thread-storage set up in the first pass to work
void
EffectInstance::Implementation::ScopedRenderArgs::setArgs_firstPass(const RectD & rod,
                                                                    const RectI & renderWindow,
                                                                    double time,
                                                                    int view,
                                                                    bool isIdentity,
                                                                    double identityTime,
                                                                    int inputNbIdentity,
                                                                    const boost::shared_ptr<EffectInstance::ComponentsNeededMap> & compsNeeded)
{
    localData->_rod = rod;
    localData->_renderWindowPixel = renderWindow;
    localData->_time = time;
    localData->_view = view;
    localData->_isIdentity = isIdentity;
    localData->_identityTime = identityTime;
    localData->_identityInputNb = inputNbIdentity;
    localData->_compsNeeded = compsNeeded;
    localData->_validArgs = true;
}


void
EffectInstance::Implementation::ScopedRenderArgs::setArgs_secondPass(const RoIMap & roiMap,
                                                                     int firstFrame,
                                                                     int lastFrame)
{
    localData->_regionOfInterestResults = roiMap;
    localData->_firstFrame = firstFrame;
    localData->_lastFrame = lastFrame;
    localData->_validArgs = true;
}


void
EffectInstance::Implementation::addInputImageTempPointer(int inputNb,
                                                         const boost::shared_ptr<Natron::Image> & img)
{
    InputImagesMap & tls = inputImages.localData();

    tls[inputNb].push_back(img);
}


void
EffectInstance::Implementation::clearInputImagePointers()
{
    if ( inputImages.hasLocalData() ) {
        inputImages.localData().clear();
    }
}


InputImagesHolder_RAII::InputImagesHolder_RAII(const EffectInstance::InputImagesMap & imgs,
                                               ThreadStorage< EffectInstance::InputImagesMap>* storage)
    : storage(storage)
    , localData(0)
{
    if ( !imgs.empty() ) {
        localData = &storage->localData();
        localData->insert( imgs.begin(), imgs.end() );
    } else {
        this->storage = 0;
    }
}


InputImagesHolder_RAII::~InputImagesHolder_RAII()
{
    if (storage) {
        assert(localData);
        localData->clear();
    }
}
