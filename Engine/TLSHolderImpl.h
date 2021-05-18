/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2021 The Natron developers
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

#ifndef Engine_TLSHolderImpl_h
#define Engine_TLSHolderImpl_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include "TLSHolder.h"

#include "Engine/AppManager.h"
#include "Engine/EffectInstance.h"

NATRON_NAMESPACE_ENTER

//Template specialization for EffectInstance::EffectTLSData:
//We do this  for the following reasons:
//We may be here in 2 cases: either in a thread from the multi-thread suite or from a thread that just got spawned
//from the host-frame threading (executing tiledRenderingFunctor).
//A multi-thread suite thread is not allowed by OpenFX to call clipGetImage, which does not require us to apply TLS
//on OfxClipInstance and also RenderArgs in EffectInstance. But a multi-thread suite thread may call the abort() function
//which needs the ParallelRenderArgs set on the EffectInstance.
//Similarly a host-frame threading thread is spawned at a time where the spawner thread only has the ParallelRenderArgs
//set on the TLS, so just copy this instead of the whole TLS.

template <>
EffectInstance::EffectTLSDataPtr
TLSHolder<EffectInstance::EffectTLSData>::copyAndReturnNewTLS(const QThread* fromThread,
                                                              const QThread* toThread) const
{
    QWriteLocker k(&perThreadDataMutex);
    ThreadDataMap::iterator found = perThreadData.find(fromThread);

    if ( found == perThreadData.end() ) {
        ///No TLS for fromThread
        return EffectInstance::EffectTLSDataPtr();
    }

    ThreadData data;
    //Copy constructor
    data.value = boost::make_shared<EffectInstance::EffectTLSData>( *(found->second.value) );
    perThreadData[toThread] = data;

    return data.value;
}

template <>
void
TLSHolder<EffectInstance::EffectTLSData>::copyTLS(const QThread* fromThread,
                                                  const QThread* toThread) const
{
    EffectInstance::EffectTLSDataPtr tlsDataPtr = copyAndReturnNewTLS(fromThread, toThread);

    Q_UNUSED(tlsDataPtr);
}

template <typename T>
void
TLSHolder<T>::copyTLS(const QThread* fromThread,
                      const QThread* toThread) const
{
    Q_UNUSED(fromThread);
    Q_UNUSED(toThread);
}

template <typename T>
boost::shared_ptr<T>
TLSHolder<T>::copyAndReturnNewTLS(const QThread* fromThread,
                                  const QThread* toThread) const
{
    Q_UNUSED(fromThread);
    Q_UNUSED(toThread);

    return boost::shared_ptr<T>();
}

template <typename T>
bool
TLSHolder<T>::canCleanupPerThreadData(const QThread* curThread) const
{
    QReadLocker k(&perThreadDataMutex);

    if ( perThreadData.empty() ) {
        return true; // cleanup would return true
    }
    typename ThreadDataMap::iterator found = perThreadData.find(curThread);
    if ( found != perThreadData.end() ) {
        return true; // cleanup would do something
    }

    return false;
}

template <typename T>
bool
TLSHolder<T>::cleanupPerThreadData(const QThread* curThread) const
{
    QWriteLocker k(&perThreadDataMutex);

    typename ThreadDataMap::iterator found = perThreadData.find(curThread);
    if ( found != perThreadData.end() ) {
        perThreadData.erase(found);
    }

    return perThreadData.empty();
}

template <typename T>
boost::shared_ptr<T>
TLSHolder<T>::getTLSData() const
{
    QThread* curThread  = QThread::currentThread();

    //This thread might be registered by a spawner thread, copy the TLS and attempt to find the TLS for this holder.
    boost::shared_ptr<T> ret = appPTR->getAppTLS()->copyTLSFromSpawnerThread<T>(this, curThread);

    if (ret) {
        return ret;
    }


    //Attempt to find an object in the map. It will be there if we already called getOrCreateTLSData() for this thread
    {
        QReadLocker k(&perThreadDataMutex);
        const ThreadDataMap& perThreadDataCRef = perThreadData; // take a const ref, since it's a read lock
        typename ThreadDataMap::const_iterator found = perThreadDataCRef.find(curThread);
        if ( found != perThreadDataCRef.end() ) {
            ret = found->second.value;
        }
    }

    return ret;
}

template <typename T>
boost::shared_ptr<T>
TLSHolder<T>::getOrCreateTLSData() const
{
    QThread* curThread  = QThread::currentThread();

    //This thread might be registered by a spawner thread, copy the TLS and attempt to find the TLS for this holder.
    boost::shared_ptr<T> ret = appPTR->getAppTLS()->copyTLSFromSpawnerThread<T>(this, curThread);

    if (ret) {
        return ret;
    }

    //Attempt to find an object in the map. It will be there if we already called getOrCreateTLSData() for this thread
    //Note that if present, this call is extremely fast as we do not block other threads
    {
        QReadLocker k(&perThreadDataMutex);
        const ThreadDataMap& perThreadDataCRef = perThreadData; // take a const ref, since it's a read lock
        typename ThreadDataMap::const_iterator found = perThreadDataCRef.find(curThread);
        if ( found != perThreadDataCRef.end() ) {
            assert(found->second.value);

            return found->second.value;
        }
    }

    //getOrCreateTLSData() has never been called on the thread, lookup the TLS
    ThreadData data;
    TLSHolderBaseConstPtr thisShared = shared_from_this();
    appPTR->getAppTLS()->registerTLSHolder(thisShared);
    data.value = boost::make_shared<T>();
    {
        QWriteLocker k(&perThreadDataMutex);
        perThreadData.insert( std::make_pair(curThread, data) );
    }
    assert(data.value);

    return data.value;
}

template <typename T>
boost::shared_ptr<T>
AppTLS::copyTLSFromSpawnerThread(const TLSHolderBase* holder,
                                 const QThread* curThread)
{
    //3 cases where this function returns NULL:
    // 1) No spawner thread registered
    // 2) T is not a ParallelRenderArgs (see comments above copyAndReturnNewTLS explicit template instantiation
    // 3) The spawner thread did not have TLS but was marked in the spawn map...
    // Either way: return a new object


    // first pass with a read lock to exit early without taking the write lock
    {
        QReadLocker k(&_spawnsMutex);
        const ThreadSpawnMap& spawnsCRef = _spawns; // take a const ref, since it's a read lock
        ThreadSpawnMap::const_iterator foundSpawned = spawnsCRef.find(curThread);
        if ( foundSpawned == spawnsCRef.end() ) {
            //This is not a spawned thread and it did not have TLS already
            return boost::shared_ptr<T>();
        }
    }
    const QThread* foundThread = 0;
    // Find first under a read lock to enable better concurrency
    {
        QReadLocker k(&_spawnsMutex);
        ThreadSpawnMap::iterator foundSpawned = _spawns.find(curThread);
        if ( foundSpawned == _spawns.end() ) {
            //This is not a spawned thread and it did not have TLS already
            return boost::shared_ptr<T>();
        }
        foundThread = foundSpawned->second;
    }

    if (foundThread) {
        QWriteLocker k(&_spawnsMutex);
        ThreadSpawnMap::iterator foundSpawned = _spawns.find(curThread);
        if ( foundSpawned == _spawns.end() ) {
            //This is not a spawned thread and it did not have TLS already
            return boost::shared_ptr<T>();
        }
        foundThread = foundSpawned->second;
        //Erase the thread from the spawn map
        _spawns.erase(foundSpawned);
    }
    {
        QWriteLocker k(&_objectMutex);
        boost::shared_ptr<T> retval = copyTLSFromSpawnerThreadInternal<T>(holder, curThread, foundThread);


        return retval;
    }
} // AppTLS::copyTLSFromSpawnerThread

template <typename T>
boost::shared_ptr<T>
AppTLS::copyTLSFromSpawnerThreadInternal(const TLSHolderBase* holder,
                                         const QThread* curThread,
                                         const QThread* spawnerThread)
{
    //Private - should be locked
    assert( !_objectMutex.tryLockForWrite() );

    boost::shared_ptr<T> tls;

    //This is a spawned thread and the first time we need the TLS for this thread, copy the whole TLS on all objects
    for (TLSObjects::iterator it = _object->objects.begin();
         it != _object->objects.end(); ++it) {
        TLSHolderBaseConstPtr p = (*it).lock();
        if (p) {
            const TLSHolder<T>* foundHolder = 0;
            if (p.get() == holder) {
                //This is the object from which it has been required to create the TLS data, copy
                //the TLS data from the spawner thread and mark it to 'tls'.
                foundHolder = dynamic_cast<const TLSHolder<T>*>( p.get() );
                if (foundHolder) {
                    tls = foundHolder->copyAndReturnNewTLS(spawnerThread, curThread);
                }
            }

            if (!foundHolder) {
                //Copy anyway
                p->copyTLS(spawnerThread, curThread);
            }
        }
    }

    return tls;
}

NATRON_NAMESPACE_EXIT

#endif // Engine_TLSHolderImpl_h
