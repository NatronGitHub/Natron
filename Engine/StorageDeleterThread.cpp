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

#include "StorageDeleterThread.h"

#include <list>

#include <QMutex>
#include <QWaitCondition>

#include "Engine/Cache.h"

NATRON_NAMESPACE_ENTER;

struct StorageDeleterThreadPrivate
{
    mutable QMutex entriesQueueMutex;
    std::list<ImageStorageBasePtr> entriesQueue;
    QWaitCondition entriesQueueNotEmptyCond;
    QMutex mustQuitMutex;
    QWaitCondition mustQuitCond;
    bool mustQuit;

    StorageDeleterThreadPrivate()
    : entriesQueueMutex()
    , entriesQueue()
    , entriesQueueNotEmptyCond()
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
        _imp->entriesQueueNotEmptyCond.wakeOne();
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
        _imp->entriesQueueNotEmptyCond.wakeOne();
    }
    while (_imp->mustQuit) {
        _imp->mustQuitCond.wait(&_imp->mustQuitMutex);
    }
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
    for (;;) {
        bool quit;
        {
            QMutexLocker k(&_imp->mustQuitMutex);
            quit = _imp->mustQuit;
        }

        {
            ImageStorageBasePtr front;
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
     
        } // front. After this scope, the image is guarenteed to be freed
      
    }
} // run

NATRON_NAMESPACE_EXIT;
