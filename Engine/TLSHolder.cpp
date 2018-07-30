/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
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
#ifndef NATRON_TLS_DISABLE_COPY
    , _spawnsMutex()
    , _spawns()
#endif
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

#ifndef NATRON_TLS_DISABLE_COPY
void
AppTLS::copyTLS(QThread* fromThread,
                QThread* toThread)
{
    if ( (fromThread == toThread) || !fromThread || !toThread ) {
        return;
    }

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

    QWriteLocker k(&_spawnsMutex);
    _spawns[toThread] = fromThread;
}
#endif
void
AppTLS::cleanupTLSForThread()
{
    QThread* curThread = QThread::currentThread();


    // Clean-up any thread data on the TLSHolder

#ifndef NATRON_TLS_DISABLE_COPY
    // If this thread was spawned, but TLS not used, do not bother to clean-up
    {
        // Try to find first with a read-lock so we still allow other threads to run
        bool threadIsInSpawnList = false;
        {
            QReadLocker l(&_spawnsMutex);
            ThreadSpawnMap::iterator foundSpawned = _spawns.find(curThread);
            if ( foundSpawned != _spawns.end() ) {
                threadIsInSpawnList = true;
            }
        }

        if (threadIsInSpawnList) {
            // The thread is in the spawns list, lock out other threads and remove it from the list
            QWriteLocker l(&_spawnsMutex);
            ThreadSpawnMap::iterator foundSpawned = _spawns.find(curThread);
            if ( foundSpawned != _spawns.end() ) {
                _spawns.erase(foundSpawned);

                return;
            }
        }
    }
#endif

    // First determine if this thread has objects to clean. Do it under
    // a read locker so we don't lock out other threads
    std::list<boost::shared_ptr<const TLSHolderBase> > objectsToClean;
    {
        // Cycle through each TLS object that had TLS set on it
        // and check if there's data for this thread.
        // Note that the list may be extremely long (several thousands of items) so avoid copying it
        // since we don't lock out other threads.
        QReadLocker k (&_objectMutex);
        const TLSObjects& objectsCRef = _object->objects;
        for (TLSObjects::iterator it = objectsCRef.begin();
             it != objectsCRef.end();
             ++it) {
            TLSHolderBaseConstPtr p = (*it).lock();
            if (p) {
                if ( p->canCleanupPerThreadData(curThread) ) {
                    // This object has data for this thread, add it to the clean up list
                    objectsToClean.push_back(p);
                }
            }
        }
    }

    // Clean objects that need it
    // We can only do so under the write-lock, which will lock out other threads
    // Note that the _object->objects set may be composed of multiple thousands of items
    // and this operation is expensive.
    // However since we are in the clean-up stage for this thread
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

template class TLSHolder<EffectInstanceTLSData>;
template class TLSHolder<OfxHost::OfxHostTLSData>;
template class TLSHolder<AppManager::PythonTLSData>;
template class TLSHolder<Project::ProjectTLSData>;
template class TLSHolder<OfxClipInstance::ClipTLSData>;
template class TLSHolder<OfxParamToKnob::OfxParamTLSData>;

NATRON_NAMESPACE_EXIT

