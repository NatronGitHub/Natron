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

#ifndef Engine_StorageDeleterThread_h
#define Engine_StorageDeleterThread_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <list>

#include <QtCore/QThread>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#endif

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

/**
 * @brief The point of this thread is to delete the content of the list in a separate thread so the thread calling
 * get() doesn't wait for all the entries to be deleted (which can be expensive for large images)
 **/
struct StorageDeleterThreadPrivate;
class StorageDeleterThread
: public QThread
{
    

public:

    StorageDeleterThread();

    virtual ~StorageDeleterThread();

    void appendToQueue(const ImageStorageBasePtr& entry);

    void appendToQueue(const std::list<ImageStorageBasePtr> & entriesToDelete);

    void checkCachesMemory();

    void quitThread();

    bool isWorking() const;

private:

    virtual void run() OVERRIDE FINAL;

    boost::scoped_ptr<StorageDeleterThreadPrivate> _imp;
};

#if 0
/**
 * @brief The point of this thread is to monitor the cache tiles storage and create storage when needed.
 **/
struct StorageAllocatorThreadPrivate;
class StorageAllocatorThread
: public QThread
{


public:

    StorageAllocatorThread();

    virtual ~StorageAllocatorThread();

    /**
     * @brief To be called when a Cache bucket no longer has sufficient free tiles. 
     * This call is blocking and returns once all buckets are guaranteed to contain at least 1 free tile. 
     **/
    void requestTilesStorageAllocation();

    void quitThread();

    bool isWorking() const;

private:

    virtual void run() OVERRIDE FINAL;

    boost::scoped_ptr<StorageAllocatorThreadPrivate> _imp;
};
#endif 


NATRON_NAMESPACE_EXIT

#endif // Engine_StorageDeleterThread_h
