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

#include "TLSHolder.h"
#include "TLSHolderImpl.h"

#include <cassert>
#include <stdexcept>

#include "Engine/OfxClipInstance.h"
#include "Engine/OfxHost.h"
#include "Engine/OfxParamInstance.h"
#include "Engine/Project.h"
#include "Engine/ThreadPool.h"

#include <QtCore/QWaitCondition>
#include <QtCore/QThread>
#include <QtCore/QDebug>

NATRON_NAMESPACE_ENTER


AppTLS::AppTLS()
    : _objectMutex()
    , _object( new GLobalTLSObject() )
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
        QWriteLocker k(&_objectMutex);
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

    QReadLocker k(&_objectMutex);
    const TLSObjects& objectsCRef = _object->objects; // take a const ref, since it's a read lock
    for (TLSObjects::const_iterator it = objectsCRef.begin();
         it != objectsCRef.end(); ++it) {
        TLSHolderBaseConstPtr p = (*it).lock();
        if (p) {
            p->copyTLS(fromThread, toThread);
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

    QWriteLocker k(&_spawnsMutex);
    _spawns[toThread] = fromThread;
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
        QWriteLocker l(&_spawnsMutex);

        //This thread was spawned, but TLS not used, do not bother to clean-up
        ThreadSpawnMap::iterator foundSpawned = _spawns.find(curThread);
        if ( foundSpawned != _spawns.end() ) {
            _spawns.erase(foundSpawned);

            return;
        }
    }
    std::list<TLSHolderBaseConstPtr> objectsToClean;
    {
        QReadLocker k (&_objectMutex);
        const TLSObjects& objectsCRef = _object->objects;
        for (TLSObjects::iterator it = objectsCRef.begin();
             it != objectsCRef.end();
             ++it) {
            TLSHolderBaseConstPtr p = (*it).lock();
            if (p) {
                if ( p->canCleanupPerThreadData(curThread) ) {
                    objectsToClean.push_back(p);
                }
            }
        }
    }
    if ( !objectsToClean.empty() ) {
#if 1
        // version from 1a0712b
        // should be OK, since the bug in 1a0712b was in canCleanupPerThreadData
        QWriteLocker k (&_objectMutex);
        for (std::list<TLSHolderBaseConstPtr>::iterator it = objectsToClean.begin();
             it != objectsToClean.end();
             ++it) {
            if ( (*it)->cleanupPerThreadData(curThread) ) {
                TLSObjects::iterator found = _object->objects.find(*it);
                if ( found != _object->objects.end() ) {
                    _object->objects.erase(found);
                }
            }
        }
#else
        // original version
        TLSObjects newObjects;
        QWriteLocker k (&_objectMutex);
        for (TLSObjects::iterator it = _object->objects.begin();
             it != _object->objects.end(); ++it) {
            TLSHolderBaseConstPtr p = (*it).lock();
            if (p) {
                if ( !p->cleanupPerThreadData(curThread) ) {
                    //The TLSHolder still has TLS on it for another thread and is still alive,
                    //then leave it in the set
                    newObjects.insert(p);
                }
            }
        }
        _object->objects = newObjects;
#endif
    }
} // AppTLS::cleanupTLSForThread

template class TLSHolder<EffectInstance::EffectTLSData>;
template class TLSHolder<NATRON_NAMESPACE::OfxHost::OfxHostTLSData>;
template class TLSHolder<KnobHelper::KnobTLSData>;
template class TLSHolder<Project::ProjectTLSData>;
template class TLSHolder<OfxClipInstance::ClipTLSData>;
template class TLSHolder<OfxParamToKnob::OfxParamTLSData>;

NATRON_NAMESPACE_EXIT

