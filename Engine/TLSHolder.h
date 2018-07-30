/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://natrongithub.github.io/>,
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

#ifndef NATRON_ENGINE_TLSHOLDER_H
#define NATRON_ENGINE_TLSHOLDER_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <list>
#include <map>
#include <vector>
#include <string>
#include <set>

#include "Global/Macros.h"
#include "Global/GlobalDefines.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#endif

#include <QtCore/QReadWriteLock>
#include <QMutex>
#include <QtCore/QThread>

#include "Engine/EngineFwd.h"


#define NATRON_TLS_DISABLE_COPY


NATRON_NAMESPACE_ENTER


///This must be stored as a shared_ptr
class TLSHolderBase
    : public boost::enable_shared_from_this<TLSHolderBase>
{
    friend class AppTLS;

public:
    // TODO: enable_shared_from_this
    // constructors should be privatized in any class that derives from boost::enable_shared_from_this<>

    TLSHolderBase() {}

public:
    virtual ~TLSHolderBase() {}

protected:

    /**
     * @brief Returns true if cleanupPerThreadData would do anything OR would return true.
     * It does not return the same value as cleanupPerThreadData, since cleanupPerThreadData
     * may do something and return false.
     * This is much faster than cleanupPerThreadData as it does not take any write lock.
     **/
    virtual bool canCleanupPerThreadData(const QThread* curThread) const = 0;

    /**
     * @brief Must clean-up any data stored for the given thread 'curThread'
     * @returns True if this object no longer holds any per-thread data
     **/
    virtual bool cleanupPerThreadData(const QThread* curThread) const = 0;

#ifndef NATRON_TLS_DISABLE_COPY
    /**
     * @brief Copy all the TLS from fromThread to toThread
     **/
    virtual void copyTLS(const QThread* fromThread, const QThread* toThread) const = 0;
#endif
};


/**
 * @brief Stores globally to the application any thread-local storage object so that it gets
 * destroyed when all threads are shutdown.
 **/
class AppTLS
{
    //This is the object in the QThreadStorage, it is duplicated on every thread

    typedef std::set<TLSHolderBaseConstWPtr> TLSObjects;
    struct GLobalTLSObject
    {
        TLSObjects objects;
    };

    typedef boost::shared_ptr<GLobalTLSObject> GLobalTLSObjectPtr;

    //<spawned thread, spawner thread>
    typedef std::map<const QThread*, const QThread*> ThreadSpawnMap;

public:

    AppTLS();

    virtual ~AppTLS();

    /**
     * @brief Registers the holder as using TLS.
     **/
    void registerTLSHolder(const TLSHolderBaseConstPtr& holder);

#ifndef NATRON_TLS_DISABLE_COPY

    /**
     * @brief Copy all the TLS from fromThread to toThread
     **/
    void copyTLS(QThread* fromThread, QThread* toThread);

    /**
     * @brief This function registers fromThread as a thread who spawned toThread.
     * The first time attempting to call getOrCreateTLSData() for toThread, it will
     * call copyTLS() first before returning the TLS value.
     * This is to ensure that threads that "may" need TLS do not always copy the TLS
     * if it is not needed.
     * Note that when calling softCopy,  fromThread may not already have
     * the TLS that may be required for the copy to happen, in which case a new value will
     * be constructed.
     **/
    void softCopy(QThread* fromThread, QThread* toThread);

    /**
     * @brief Same as copyTLS() except that if a spawner thread was register for curThread beforehand
     * with softCopy() then the TLS will be copied from the spawner thread.
     * This function also returns the TLS for the given holder for convenience.
     **/
    template <typename T>
    boost::shared_ptr<T> copyTLSFromSpawnerThread(const TLSHolderBase* holder,
                                                  const QThread* curThread);

#endif
    /**
     * @brief Should be called by any thread using TLS when done to cleanup its TLS
     **/
    void cleanupTLSForThread();

private:

    template <typename T>
    boost::shared_ptr<T> copyTLSFromSpawnerThreadInternal(const TLSHolderBase* holder,
                                                          const QThread* curThread,
                                                          const QThread* spawnerThread);


    //This is the "TLS" object: it stores a set of all TLSHolder's who used the TLS to clean it up afterwards
    mutable QReadWriteLock _objectMutex;
    GLobalTLSObjectPtr _object;

#ifndef NATRON_TLS_DISABLE_COPY
    //if a thread is a spawned thread, then copy the tls from the spawner thread instead
    //of creating a new object and no longer mark it as spawned
    mutable QReadWriteLock _spawnsMutex;
    ThreadSpawnMap _spawns;
#endif
};


/**
 * @brief Use this class if you need to hold TLS data on an object.
 * @param T is the data type held in the thread local storage.
 **/
template <typename T>
class TLSHolder
: public TLSHolderBase
{
    friend class AppTLS;

    struct ThreadData
    {
        boost::shared_ptr<T> value;
    };

    typedef std::map<const QThread*, ThreadData> ThreadDataMap;

public:

    TLSHolder()
    : TLSHolderBase() {}

    virtual ~TLSHolder() {}

    // Returns tls data for the current thread
    boost::shared_ptr<T> getTLSData() const;

    // Warning this does not hold any promise on the thread safety of the returned object
    // if the thread in parameter is different than the current thread.
    boost::shared_ptr<T> getTLSDataForThread(QThread* thread) const;

    // Get or create tls data for the current thread
    boost::shared_ptr<T> getOrCreateTLSData() const;

private:

    virtual bool canCleanupPerThreadData(const QThread* curThread) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool cleanupPerThreadData(const QThread* curThread) const OVERRIDE FINAL WARN_UNUSED_RETURN;
#ifndef NATRON_TLS_DISABLE_COPY
    virtual void copyTLS(const QThread* fromThread, const QThread* toThread) const OVERRIDE FINAL;
    boost::shared_ptr<T> copyAndReturnNewTLS(const QThread* fromThread, const QThread* toThread) const WARN_UNUSED_RETURN;
#endif

    //Store a cache on the object to be faster than using the getOrCreate... function from AppTLS
    mutable QReadWriteLock perThreadDataMutex;
    mutable ThreadDataMap perThreadData;
};

NATRON_NAMESPACE_EXIT

#endif // NATRON_ENGINE_TLSHOLDER_H
