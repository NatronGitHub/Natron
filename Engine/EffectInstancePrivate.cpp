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




NATRON_NAMESPACE_EXIT;


