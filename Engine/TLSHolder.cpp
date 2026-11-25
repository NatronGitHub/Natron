/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2023 The Natron developers
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

#include "TLSHolder.h"
#include "TLSHolderImpl.h"

#include <cassert>
#include <stdexcept>

#include "Engine/OfxClipInstance.h"
#include "Engine/OfxHost.h"
#include "Engine/OfxParamInstance.h"
#include "Engine/Project.h"
#include "Engine/ThreadPool.h"

#include <QWaitCondition>
#include <QThread>
#include <QDebug>

NATRON_NAMESPACE_ENTER


AppTLS::AppTLS()
    : _objectMutex()
    , _object( new GlobalTLSObject() )
    , _spawnsMutex()
    , _spawns()
{
}

AppTLS::~AppTLS()
{
}

void
AppTLS::registerTLSHolder(const TLSHolderBaseConstPtr& holder)
{
    //This must be the first time for this thread that we reach here since cleanupTLSForThread() was made, otherwise
    //the TLS data should always be available on the TLSHolder
    {
        std::unique_lock<std::shared_mutex> k(_objectMutex);
        //The insert call might not succeed because another thread inserted this holder in the set already, that's fine
        _object->objects.insert(holder);
    }
}

static void
copyAbortInfo(QThread* fromThread,
              QThread* toThread)
{
    AbortableThread* fromAbortable = dynamic_cast<AbortableThread*>(fromThread);
    AbortableThread* toAbortable = dynamic_cast<AbortableThread*>(toThread);

    if (fromAbortable && toAbortable) {
        bool isRenderResponseToUserInteraction;
        AbortableRenderInfoPtr abortInfo;
        EffectInstancePtr treeRoot;
        fromAbortable->getAbortInfo(&isRenderResponseToUserInteraction, &abortInfo, &treeRoot);
        toAbortable->setAbortInfo(isRenderResponseToUserInteraction, abortInfo, treeRoot);
    }
}

void
AppTLS::copyTLS(QThread* fromThread,
                QThread* toThread)
{
    if ( (fromThread == toThread) || !fromThread || !toThread ) {
        return;
    }

    copyAbortInfo(fromThread, toThread);

    // The holders are released after _objectMutex (declared before the lock): one of them may be the last
    // reference to its holder, and destroying a holder may destroy effects (see cleanupTLSForThread()).
    std::vector<TLSHolderBaseConstPtr> holders;
    std::shared_lock<std::shared_mutex> k(_objectMutex);
    const TLSObjects& objectsCRef = _object->objects; // take a const ref, since it's a read lock
    holders.reserve( objectsCRef.size() );
    for (TLSObjects::const_iterator it = objectsCRef.begin();
         it != objectsCRef.end(); ++it) {
        TLSHolderBaseConstPtr p = (*it).lock();
        if (p) {
            p->copyTLS(fromThread, toThread);
            holders.push_back( std::move(p) );
        }
    }
}

void
AppTLS::softCopy(QThread* fromThread,
                 QThread* toThread)
{
    if ( (fromThread == toThread) || !fromThread || !toThread ) {
        return;
    }

    copyAbortInfo(fromThread, toThread);

    std::unique_lock<std::shared_mutex> k(_spawnsMutex);
    _spawns[uintptr_t(toThread)] = fromThread;
}

void
AppTLS::cleanupTLSForThread()
{
    QThread* curThread = QThread::currentThread();
    AbortableThread* isAbortableThread = dynamic_cast<AbortableThread*>(curThread);

    if (isAbortableThread) {
        isAbortableThread->clearAbortInfo();
    }

    //Cleanup any cached data on the TLSHolder
    {
        std::unique_lock<std::shared_mutex> l(_spawnsMutex);

        //This thread was spawned, but TLS not used, do not bother to clean-up
        ThreadSpawnMap::iterator foundSpawned = _spawns.find(uintptr_t(curThread));
        if ( foundSpawned != _spawns.end() ) {
            _spawns.erase(foundSpawned);

            return;
        }
    }
    std::list<TLSHolderBaseConstPtr> objectsToClean;
    {
        // the holders that need no cleanup are released after _objectMutex (see copyTLS())
        std::vector<TLSHolderBaseConstPtr> otherHolders;
        std::shared_lock<std::shared_mutex> k(_objectMutex);
        const TLSObjects& objectsCRef = _object->objects;
        otherHolders.reserve( objectsCRef.size() );
        for (TLSObjects::iterator it = objectsCRef.begin();
             it != objectsCRef.end();
             ++it) {
            TLSHolderBaseConstPtr p = (*it).lock();
            if (p) {
                if ( p->canCleanupPerThreadData(curThread) ) {
                    objectsToClean.push_back( std::move(p) );
                } else {
                    otherHolders.push_back( std::move(p) );
                }
            }
        }
    }
    if ( !objectsToClean.empty() ) {
        // The removed data is destroyed after _objectMutex is released (declared before the lock, and
        // reserved so that no allocation can fail under the lock): it may hold the last references to effects
        // of deleted nodes, and destroying them runs plug-in actions that register thread-local storage, which
        // takes _objectMutex (a self-deadlock, #1653). Those actions may create thread-local data for this
        // thread again (e.g. OfxHost's, with empty stacks): this cleanup doesn't remove it.
        std::vector<std::shared_ptr<const void>> removedData;
        removedData.reserve( objectsToClean.size() );
        std::unique_lock<std::shared_mutex> k(_objectMutex);
        for (std::list<TLSHolderBaseConstPtr>::iterator it = objectsToClean.begin();
             it != objectsToClean.end();
             ++it) {
            TLSHolderBase::CleanupResult cleanup = (*it)->cleanupPerThreadData(curThread);
            if (cleanup.removedData) {
                removedData.push_back( std::move(cleanup.removedData) ); // no allocation: reserved
            }
            if (cleanup.holderEmpty) {
                TLSObjects::iterator found = _object->objects.find(*it);
                if ( found != _object->objects.end() ) {
                    _object->objects.erase(found);
                }
            }
        }
    }
} // AppTLS::cleanupTLSForThread

template class TLSHolder<EffectInstance::EffectTLSData>;
template class TLSHolder<NATRON_NAMESPACE::OfxHost::OfxHostTLSData>;
template class TLSHolder<KnobHelper::KnobTLSData>;
template class TLSHolder<Project::ProjectTLSData>;
template class TLSHolder<OfxClipInstance::ClipTLSData>;
template class TLSHolder<OfxParamToKnob::OfxParamTLSData>;

NATRON_NAMESPACE_EXIT

