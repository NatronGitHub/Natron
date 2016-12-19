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

#include "CacheDeleterThread.h"

#include <list>

#include <QMutex>
#include <QWaitCondition>

#include "Engine/Cache.h"

NATRON_NAMESPACE_ENTER;

struct CacheDeleterThreadPrivate
{
    mutable QMutex entriesQueueMutex;
    std::list<CacheEntryBasePtr> entriesQueue;
    QWaitCondition entriesQueueNotEmptyCond;
    QMutex mustQuitMutex;
    QWaitCondition mustQuitCond;
    bool mustQuit;

    CacheDeleterThreadPrivate()
    : entriesQueueMutex()
    , entriesQueue()
    , entriesQueueNotEmptyCond()
    , mustQuitMutex()
    , mustQuitCond()
    , mustQuit(false)
    {

    }
};



CacheDeleterThread::CacheDeleterThread()
: QThread()
, _imp(new CacheDeleterThreadPrivate())
{
    setObjectName( QString::fromUtf8("CacheDeleter") );
}

CacheDeleterThread::~CacheDeleterThread()
{
    
}



void
CacheDeleterThread::appendToQueue(const std::list<CacheEntryBasePtr> & entriesToDelete)
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
        _imp->entriesQueueNotEmptyCond.wakeOne();
    }
}

void
CacheDeleterThread::quitThread()
{
    if ( !isRunning() ) {
        return;
    }
    QMutexLocker k(&_imp->mustQuitMutex);
    assert(!_imp->mustQuit);
    _imp->mustQuit = true;

    {
        QMutexLocker k2(&_imp->entriesQueueMutex);
        _imp->entriesQueue.push_back( CacheEntryBasePtr() );
        _imp->entriesQueueNotEmptyCond.wakeOne();
    }
    while (_imp->mustQuit) {
        _imp->mustQuitCond.wait(&_imp->mustQuitMutex);
    }
}

bool
CacheDeleterThread::isWorking() const
{
    QMutexLocker k(&_imp->entriesQueueMutex);

    return !_imp->entriesQueue.empty();
}

void
CacheDeleterThread::run()
{
    for (;;) {
        bool quit;
        {
            QMutexLocker k(&_imp->mustQuitMutex);
            quit = _imp->mustQuit;
        }

        {
            CacheEntryBasePtr front;
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
                while ( _imp->entriesQueue.empty() ) {
                    _imp->entriesQueueNotEmptyCond.wait(&_imp->entriesQueueMutex);
                }

                assert( !_imp->entriesQueue.empty() );
                front = _imp->entriesQueue.front();
                _imp->entriesQueue.pop_front();
            }
            if (front) {
                front->scheduleForDestruction();
            }
        } // front. After this scope, the image is guarenteed to be freed
      
    }
} // run

struct CleanRequest
{
    std::string pluginID;
};

struct CacheCleanerThreadPrivate
{
    mutable QMutex requestQueueMutex;

    std::list<CleanRequest> requestsQueues;
    QWaitCondition requestsQueueNotEmptyCond;
    CacheWPtr cache;
    QMutex mustQuitMutex;
    QWaitCondition mustQuitCond;
    bool mustQuit;

    CacheCleanerThreadPrivate(const CachePtr& cache)
    : requestQueueMutex()
    , requestsQueues()
    , requestsQueueNotEmptyCond()
    , cache(cache)
    , mustQuitMutex()
    , mustQuitCond()
    , mustQuit(false)
    {
        
    }
};

CacheCleanerThread::CacheCleanerThread(const CachePtr& cache)
: QThread()
, _imp(new CacheCleanerThreadPrivate(cache))
{
    setObjectName( QString::fromUtf8("CacheCleaner") );
}

CacheCleanerThread::~CacheCleanerThread()
{
}

void
CacheCleanerThread::appendToQueue(const std::string& pluginID)
{
    {
        QMutexLocker k(&_imp->requestQueueMutex);
        CleanRequest r;
        r.pluginID = pluginID;
        _imp->requestsQueues.push_back(r);
    }
    if ( !isRunning() ) {
        start();
    } else {
        QMutexLocker k(&_imp->requestQueueMutex);
        _imp->requestsQueueNotEmptyCond.wakeOne();
    }
}

void
CacheCleanerThread::quitThread()
{
    if ( !isRunning() ) {
        return;
    }
    QMutexLocker k(&_imp->mustQuitMutex);
    assert(!_imp->mustQuit);
    _imp->mustQuit = true;

    {
        QMutexLocker k2(&_imp->requestQueueMutex);
        CleanRequest r;
        _imp->requestsQueues.push_back(r);
        _imp->requestsQueueNotEmptyCond.wakeOne();
    }
    while (_imp->mustQuit) {
        _imp->mustQuitCond.wait(&_imp->mustQuitMutex);
    }
}

bool
CacheCleanerThread::isWorking() const
{
    QMutexLocker k(&_imp->requestQueueMutex);

    return !_imp->requestsQueues.empty();
}

void
CacheCleanerThread::run()
{
    for (;;) {
        bool quit;
        {
            QMutexLocker k(&_imp->mustQuitMutex);
            quit = _imp->mustQuit;
        }

        {
            CleanRequest front;
            {
                QMutexLocker k(&_imp->requestQueueMutex);
                if ( quit && _imp->requestsQueues.empty() ) {
                    _imp->requestQueueMutex.unlock();
                    QMutexLocker k(&_imp->mustQuitMutex);
                    assert(_imp->mustQuit);
                    _imp->mustQuit = false;
                    _imp->mustQuitCond.wakeOne();

                    return;
                }
                while ( _imp->requestsQueues.empty() ) {
                    _imp->requestsQueueNotEmptyCond.wait(&_imp->requestQueueMutex);
                }

                assert( !_imp->requestsQueues.empty() );
                front = _imp->requestsQueues.front();
                _imp->requestsQueues.pop_front();
            }
            CachePtr c = _imp->cache.lock();
            if (c) {
                c->removeAllEntriesForPluginPrivate(front.pluginID, 0);
            }

        }
    }
} // run

NATRON_NAMESPACE_EXIT;
