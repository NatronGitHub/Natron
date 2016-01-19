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

#ifndef Engine_TLSHolderImpl_h
#define Engine_TLSHolderImpl_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "TLSHolder.h"

#include "Engine/AppManager.h"
#include "Engine/EffectInstance.h"

NATRON_NAMESPACE_ENTER;

//Template specialization for Natron::EffectInstance::EffectTLSData:
//We do this  for the following reasons:
//We may be here in 2 cases: either in a thread from the multi-thread suite or from a thread that just got spawned
//from the host-frame threading (executing tiledRenderingFunctor).
//A multi-thread suite thread is not allowed by OpenFX to call clipGetImage, which does not require us to apply TLS
//on OfxClipInstance and also RenderArgs in EffectInstance. But a multi-thread suite thread may call the abort() function
//which needs the ParallelRenderArgs set on the EffectInstance.
//Similarly a host-frame threading thread is spawned at a time where the spawner thread only has the ParallelRenderArgs
//set on the TLS, so just copy this instead of the whole TLS.

template <>
boost::shared_ptr<Natron::EffectInstance::EffectTLSData>
TLSHolder<Natron::EffectInstance::EffectTLSData>::copyAndReturnNewTLS(const QThread* fromThread, const QThread* toThread) const
{
    QWriteLocker k(&perThreadDataMutex);
    ThreadDataMap::iterator found = perThreadData.find(fromThread);
    if (found == perThreadData.end()) {
        ///No TLS for fromThread
        return boost::shared_ptr<Natron::EffectInstance::EffectTLSData>();
    }
    
    ThreadData data;
    //Copy constructor
    data.value.reset(new Natron::EffectInstance::EffectTLSData(*(found->second.value)));
    perThreadData[toThread] = data;
    return data.value;
    
}

template <>
void
TLSHolder<Natron::EffectInstance::EffectTLSData>::copyTLS(const QThread* fromThread, const QThread* toThread) const
{
    (void)copyAndReturnNewTLS(fromThread, toThread);
}

template <typename T>
void
TLSHolder<T>::copyTLS(const QThread* /*fromThread*/, const QThread* /*toThread*/) const
{
    
}

template <typename T>
boost::shared_ptr<T>
TLSHolder<T>::copyAndReturnNewTLS(const QThread* /*fromThread*/, const QThread* /*toThread*/) const
{
    return boost::shared_ptr<T>();
}



template <typename T>
bool
TLSHolder<T>::cleanupPerThreadData(const QThread* curThread) const
{
    QWriteLocker k(&perThreadDataMutex);
    typename ThreadDataMap::iterator found = perThreadData.find(curThread);
    if (found != perThreadData.end()) {
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
    boost::shared_ptr<const TLSHolderBase> thisShared = shared_from_this();
    boost::shared_ptr<T> ret = appPTR->getAppTLS()->copyTLSFromSpawnerThread<T>(thisShared, curThread);
    if (ret) {
        return ret;
    }
    
    
    //Attempt to find an object in the map. It will be there if we already called getOrCreateTLSData() for this thread
    {
        QReadLocker k(&perThreadDataMutex);
        typename ThreadDataMap::iterator found = perThreadData.find(curThread);
        if (found != perThreadData.end()) {
            return found->second.value;
        }
    }
    
    return boost::shared_ptr<T>();
}

template <typename T>
boost::shared_ptr<T>
TLSHolder<T>::getOrCreateTLSData() const
{
    QThread* curThread  = QThread::currentThread();
    
    //This thread might be registered by a spawner thread, copy the TLS and attempt to find the TLS for this holder.
    boost::shared_ptr<const TLSHolderBase> thisShared = shared_from_this();
    boost::shared_ptr<T> ret = appPTR->getAppTLS()->copyTLSFromSpawnerThread<T>(thisShared, curThread);
    if (ret) {
        return ret;
    }
    
    //Attempt to find an object in the map. It will be there if we already called getOrCreateTLSData() for this thread
    //Note that if present, this call is extremely fast as we do not block other threads
    {
        QReadLocker k(&perThreadDataMutex);
        typename ThreadDataMap::iterator found = perThreadData.find(curThread);
        if (found != perThreadData.end()) {
            assert(found->second.value);
            return found->second.value;
        }
    }
    
    //getOrCreateTLSData() has never been called on the thread, lookup the TLS
    ThreadData data;
    appPTR->getAppTLS()->registerTLSHolder(thisShared);
    data.value.reset(new T);
    {
        QWriteLocker k(&perThreadDataMutex);
        perThreadData.insert(std::make_pair(curThread,data));
    }
    assert(data.value);
    return data.value;
}


template <typename T>
boost::shared_ptr<T>
AppTLS::copyTLSFromSpawnerThread(const boost::shared_ptr<const TLSHolderBase>& holder,
                                 const QThread* curThread)
{
    //3 cases where this function returns NULL:
    // 1) No spawner thread registered
    // 2) T is not a ParallelRenderArgs (see comments above copyAndReturnNewTLS explicit template instanciation
    // 3) The spawner thread did not have TLS but was marked in the spawn map...
    // Either way: return a new object

    
    ThreadSpawnMap::iterator foundSpawned;
    {
        QReadLocker k(&_objectMutex);
        foundSpawned = _spawns.find(curThread);
        if (foundSpawned == _spawns.end()) {
            //This is not a spawned thread and it did not have TLS already
            return boost::shared_ptr<T>();
        }
    }
    {
        QWriteLocker k(&_objectMutex);
        return copyTLSFromSpawnerThreadInternal<T>(holder, curThread, foundSpawned);
    }
}

template <typename T>
boost::shared_ptr<T>
AppTLS::copyTLSFromSpawnerThreadInternal(const boost::shared_ptr<const TLSHolderBase>& holder,
                                              const QThread* curThread,
                                         ThreadSpawnMap::iterator foundSpawned)
{
    //Private - should be locked
    assert(!_objectMutex.tryLockForWrite());
    
    boost::shared_ptr<T> tls;
    
    //This is a spawned thread and the first time we need the TLS for this thread, copy the whole TLS on all objects
    for (TLSObjects::iterator it = _object->objects.begin();
         it!=_object->objects.end(); ++it) {
        boost::shared_ptr<const TLSHolderBase> p = (*it).lock();
        if (p) {
            const TLSHolder<T>* foundHolder = 0;
            if (p.get() == holder.get()) {
                //This is the object from which it has been required to create the TLS data, copy
                //the TLS data from the spawner thread and mark it to 'tls'.
                foundHolder = dynamic_cast<const TLSHolder<T>*>(p.get());
                if (foundHolder) {
                    tls = foundHolder->copyAndReturnNewTLS(foundSpawned->second, curThread);
                }
            }
            
            if (!foundHolder) {
                //Copy anyway
                p->copyTLS(foundSpawned->second, curThread);
            }
        }
    }
    
    
    //Erase the thread from the spawn map
    _spawns.erase(foundSpawned);
    return tls;

}

NATRON_NAMESPACE_EXIT;

#endif // Engine_TLSHolderImpl_h
