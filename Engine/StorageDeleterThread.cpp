/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
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

#include "StorageDeleterThread.h"

#include <list>

#include <QMutex>
#include <QWaitCondition>

#ifdef DEBUG
#include "Global/FloatingPointExceptions.h"
#endif
#include "Engine/AppManager.h"
#include "Engine/Cache.h"
#include "Engine/ImageStorage.h"

NATRON_NAMESPACE_ENTER

struct StorageDeleterThreadPrivate
{
    mutable QMutex entriesQueueMutex;
    std::list<ImageStorageBasePtr> entriesQueue;
    int cacheEvictChecksRequest;
    QWaitCondition noworkCond;
    QMutex mustQuitMutex;
    QWaitCondition mustQuitCond;
    bool mustQuit;

    StorageDeleterThreadPrivate()
    : entriesQueueMutex()
    , entriesQueue()
    , cacheEvictChecksRequest(0)
    , noworkCond()
    , mustQuitMutex()
    , mustQuitCond()
    , mustQuit(false)
    {

    }
};



StorageDeleterThread::StorageDeleterThread()
: QThread()
, _imp(new StorageDeleterThreadPrivate())
{
    setObjectName( QString::fromUtf8("StorageDeleter") );
}

StorageDeleterThread::~StorageDeleterThread()
{
    
}

void
StorageDeleterThread::appendToQueue(const ImageStorageBasePtr& entry)
{
    std::list<ImageStorageBasePtr> tmpList;
    tmpList.push_back(entry);
    appendToQueue(tmpList);
}

void
StorageDeleterThread::appendToQueue(const std::list<ImageStorageBasePtr> & entriesToDelete)
{
    if ( entriesToDelete.empty() ) {
        return;
    }

    {
        QMutexLocker k(&_imp->entriesQueueMutex);
        _imp->entriesQueue.insert( _imp->entriesQueue.begin(), entriesToDelete.begin(), entriesToDelete.end() );
    }
    if ( !isRunning() ) {
        start();
    } else {
        QMutexLocker k(&_imp->entriesQueueMutex);
        _imp->noworkCond.wakeOne();
    }
}

void
StorageDeleterThread::checkCachesMemory()
{
    {
        QMutexLocker k(&_imp->entriesQueueMutex);
        ++_imp->cacheEvictChecksRequest;
    }
    if ( !isRunning() ) {
        start();
    } else {
        QMutexLocker k(&_imp->entriesQueueMutex);
        _imp->noworkCond.wakeOne();
    }
}

void
StorageDeleterThread::quitThread()
{
    if ( !isRunning() ) {
        return;
    }
    QMutexLocker k(&_imp->mustQuitMutex);
    assert(!_imp->mustQuit);
    _imp->mustQuit = true;

    {
        QMutexLocker k2(&_imp->entriesQueueMutex);
        _imp->entriesQueue.push_back( ImageStorageBasePtr() );
        _imp->noworkCond.wakeOne();
    }
    while (_imp->mustQuit) {
        _imp->mustQuitCond.wait(&_imp->mustQuitMutex);
    }
    wait();
}

bool
StorageDeleterThread::isWorking() const
{
    QMutexLocker k(&_imp->entriesQueueMutex);

    return !_imp->entriesQueue.empty();
}

void
StorageDeleterThread::run()
{
#ifdef DEBUG
    boost_adaptbx::floating_point::exception_trapping trap(boost_adaptbx::floating_point::exception_trapping::division_by_zero |
                                                           boost_adaptbx::floating_point::exception_trapping::invalid |
                                                           boost_adaptbx::floating_point::exception_trapping::overflow);
#endif
    for (;;) {
        bool quit;
        {
            QMutexLocker k(&_imp->mustQuitMutex);
            quit = _imp->mustQuit;
        }

        {
            ImageStorageBasePtr front;
            int evictRequest = 0;
            {
                QMutexLocker k(&_imp->entriesQueueMutex);
                if ( quit && _imp->entriesQueue.empty() ) {
                    _imp->entriesQueueMutex.unlock();
                    QMutexLocker k(&_imp->mustQuitMutex);
                    assert(_imp->mustQuit);
                    _imp->mustQuit = false;
                    _imp->mustQuitCond.wakeOne();

                    return;
                }
                while ( _imp->entriesQueue.empty() && _imp->cacheEvictChecksRequest <= 0) {
                    _imp->noworkCond.wait(&_imp->entriesQueueMutex);
                }

                if (!_imp->entriesQueue.empty() ) {
                    front = _imp->entriesQueue.front();
                    _imp->entriesQueue.pop_front();
                }
                if (_imp->cacheEvictChecksRequest > 0) {
                    evictRequest = _imp->cacheEvictChecksRequest;
                    _imp->cacheEvictChecksRequest = 0;
                }
            }
            if (front) {
                // if we are the last owner using this buffer, remove it
                //if (front.use_count() == 1) {
                    front->deallocateMemory();
                //}
            } else if (evictRequest > 0) {
                appPTR->getGeneralPurposeCache()->evictLRUEntries(0);
                appPTR->getTileCache()->evictLRUEntries(0);
            }
     
        } // front. After this scope, the image is guarenteed to be freed
      
    }
} // run
#if 0

struct StorageAllocatorThreadPrivate
{
    QMutex mustQuitMutex;
    QWaitCondition mustQuitCond;
    bool mustQuit;
    QMutex checkStorageMutex;
    QWaitCondition checkStorageCond;

    StorageAllocatorThreadPrivate()
    : mustQuitMutex()
    , mustQuitCond()
    , mustQuit(false)
    {

    }
};


StorageAllocatorThread::StorageAllocatorThread()
: QThread()
, _imp(new StorageAllocatorThreadPrivate())
{
    setObjectName( QString::fromUtf8("StorageAllocator") );
}

StorageAllocatorThread::~StorageAllocatorThread()
{

}

void
StorageAllocatorThread::quitThread()
{
    if ( !isRunning() ) {
        return;
    }
    QMutexLocker k(&_imp->mustQuitMutex);
    assert(!_imp->mustQuit);
    _imp->mustQuit = true;

    {

    }
    while (_imp->mustQuit) {
        _imp->mustQuitCond.wait(&_imp->mustQuitMutex);
    }
    wait();
}

void
StorageAllocatorThread::requestTilesStorageAllocation()
{

}

bool
StorageAllocatorThread::isWorking() const
{

}

void
StorageAllocatorThread::run()
{
#ifdef DEBUG
    boost_adaptbx::floating_point::exception_trapping trap(boost_adaptbx::floating_point::exception_trapping::division_by_zero |
                                                           boost_adaptbx::floating_point::exception_trapping::invalid |
                                                           boost_adaptbx::floating_point::exception_trapping::overflow);
#endif
    for (;;) {
        bool quit;
        {
            QMutexLocker k(&_imp->mustQuitMutex);
            quit = _imp->mustQuit;
        }
        if (quit) {
            return;
        }




    }
} // run
#endif

NATRON_NAMESPACE_EXIT
