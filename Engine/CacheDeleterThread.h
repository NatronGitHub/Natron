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


#ifndef NATRON_ENGINE_CACHEDELETERTHREAD_H
#define NATRON_ENGINE_CACHEDELETERTHREAD_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <list>

#include <QThread>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#endif

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER;

/**
 * @brief The point of this thread is to delete the content of the list in a separate thread so the thread calling
 * get() doesn't wait for all the entries to be deleted (which can be expensive for large images)
 **/
struct CacheDeleterThreadPrivate;
class CacheDeleterThread
: public QThread
{
    

public:

    CacheDeleterThread();

    virtual ~CacheDeleterThread();


    void appendToQueue(const std::list<CacheEntryBasePtr> & entriesToDelete);

    void quitThread();

    bool isWorking() const;

private:

    virtual void run() OVERRIDE FINAL;

    boost::scoped_ptr<CacheDeleterThreadPrivate> _imp;
};


/**
 * @brief The point of this thread is to remove entries that we are sure are no longer needed
 * e.g: they may have a hash that can no longer be produced
 **/
struct CacheCleanerThreadPrivate;
class CacheCleanerThread
: public QThread
{

public:

    CacheCleanerThread(const CachePtr& cache);

    virtual ~CacheCleanerThread();

    void appendToQueue(const std::string& pluginID);

    void quitThread();

    bool isWorking() const;

private:

    virtual void run() OVERRIDE FINAL;

    boost::scoped_ptr<CacheCleanerThreadPrivate> _imp;

};



NATRON_NAMESPACE_EXIT;

#endif // NATRON_ENGINE_CACHEDELETERTHREAD_H
