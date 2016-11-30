/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
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

#include "Engine/AppInstance.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/ViewIdx.h"


NATRON_NAMESPACE_ENTER;

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
    , renderWindowPixel()
    , time(0)
    , view(0)
    , validArgs(false)
    , inputImages()
    , outputPlanes()
    , outputPlaneBeingRendered()
    , firstFrame(0)
    , lastFrame(0)
    , transformRedirections()
{
}

EffectInstance::RenderArgs::RenderArgs(const RenderArgs & o)
    : rod(o.rod)
    , renderWindowPixel(o.renderWindowPixel)
    , time(o.time)
    , view(o.view)
    , validArgs(o.validArgs)
    , inputImages(o.inputImages)
    , outputPlanes(o.outputPlanes)
    , outputPlaneBeingRendered(o.outputPlaneBeingRendered)
    , firstFrame(o.firstFrame)
    , lastFrame(o.lastFrame)
    , transformRedirections(o.transformRedirections)
{
}

EffectInstance::Implementation::Implementation(EffectInstance* publicInterface)
    : _publicInterface(publicInterface)
    , tlsData( new TLSHolder<EffectTLSData>() )
    , duringInteractActionMutex()
    , duringInteractAction(false)
    , pluginMemoryChunksMutex()
    , pluginMemoryChunks()
    , supportsRenderScale(eSupportsMaybe)
    , actionsCache(new ActionsCache(appPTR->getHardwareIdealThreadCount() * 2))
#if NATRON_ENABLE_TRIMAP
    , imagesBeingRenderedMutex()
    , imagesBeingRendered()
#endif
    , componentsAvailableMutex()
    , componentsAvailableDirty(true)
    , outputComponentsAvailable()
    , metadatasMutex()
    , metadatas()
    , runningClipPreferences(false)
    , overlaysViewport()
    , attachedContextsMutex(QMutex::Recursive)
    , attachedContexts()
    , mainInstance()
    , isDoingInstanceSafeRender(false)
    , renderClonesMutex()
    , renderClonesPool()
{
}

EffectInstance::Implementation::Implementation(const Implementation& other)
: _publicInterface()
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
, componentsAvailableMutex()
, componentsAvailableDirty(other.componentsAvailableDirty)
, outputComponentsAvailable(other.outputComponentsAvailable)
, metadatasMutex()
, metadatas(other.metadatas)
, runningClipPreferences(other.runningClipPreferences)
, overlaysViewport(other.overlaysViewport)
, attachedContextsMutex(QMutex::Recursive)
, attachedContexts()
, mainInstance(other._publicInterface->shared_from_this())
, isDoingInstanceSafeRender(false)
, renderClonesMutex()
, renderClonesPool()
{

}

void
EffectInstance::Implementation::setDuringInteractAction(bool b)
{
    QWriteLocker l(&duringInteractActionMutex);

    duringInteractAction = b;
}

#if NATRON_ENABLE_TRIMAP
void
EffectInstance::Implementation::markImageAsBeingRendered(const ImagePtr & img)
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
                                                                            const ImagePtr & img)
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
EffectInstance::Implementation::unmarkImageAsBeingRendered(const ImagePtr & img,
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

#endif \
    // if NATRON_ENABLE_TRIMAP


EffectInstance::Implementation::ScopedRenderArgs::ScopedRenderArgs(const EffectDataTLSPtr& tlsData,
                                                                   const RectD & rod,
                                                                   const RectI & renderWindow,
                                                                   double time,
                                                                   ViewIdx view,
                                                                   const ComponentsNeededMapPtr& compsNeeded,
                                                                   const EffectInstance::InputImagesMap& inputImages,
                                                                   int firstFrame,
                                                                   int lastFrame)
    : tlsData(tlsData)
{
    tlsData->currentRenderArgs.rod = rod;
    tlsData->currentRenderArgs.renderWindowPixel = renderWindow;
    tlsData->currentRenderArgs.time = time;
    tlsData->currentRenderArgs.view = view;
    tlsData->currentRenderArgs.compsNeeded = compsNeeded;
    tlsData->currentRenderArgs.inputImages.insert( inputImages.begin(), inputImages.end() );
    tlsData->currentRenderArgs.firstFrame = firstFrame;
    tlsData->currentRenderArgs.lastFrame = lastFrame;

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




NATRON_NAMESPACE_EXIT;


