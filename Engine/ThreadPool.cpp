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

#include "ThreadPool.h"
#include <QAtomicInt>

NATRON_NAMESPACE_ENTER;


// We patched Qt to be able to derive QThreadPool to control the threads that are spawned to improve performances
// of the EffectInstance::aborted() function
#ifdef QT_CUSTOM_THREADPOOL

struct AbortableThreadPrivate
{
    bool isRenderResponseToUserInteraction;
    AbortableRenderInfoPtr abortInfo;
    EffectInstWPtr treeRoot;
    bool abortInfoValid;

    AbortableThreadPrivate()
        : isRenderResponseToUserInteraction(false)
        , abortInfo()
        , treeRoot()
        , abortInfoValid(false)
    {
    }
};

AbortableThread::AbortableThread()
    : _imp( new AbortableThreadPrivate() )
{
}

AbortableThread::~AbortableThread()
{
}

void
AbortableThread::setAbortInfo(bool isRenderResponseToUserInteraction,
                              const AbortableRenderInfoPtr& abortInfo,
                              const EffectInstPtr& treeRoot)
{
    _imp->isRenderResponseToUserInteraction = isRenderResponseToUserInteraction;
    _imp->abortInfo = abortInfo;
    _imp->treeRoot = treeRoot;
    _imp->abortInfoValid = true;
}

void
AbortableThread::clearAbortInfo()
{
    _imp->abortInfo.reset();
    _imp->treeRoot.reset();
    _imp->abortInfoValid = false;
}

bool
AbortableThread::getAbortInfo(bool* isRenderResponseToUserInteraction,
                              AbortableRenderInfoPtr* abortInfo,
                              EffectInstPtr* treeRoot) const
{
    if (!_imp->abortInfoValid) {
        return false;
    }
    *isRenderResponseToUserInteraction = _imp->isRenderResponseToUserInteraction;
    *abortInfo = _imp->abortInfo;
    *treeRoot = _imp->treeRoot.lock();

    return true;
}

namespace {
class ThreadPoolThread
    : public QThreadPoolThread, public AbortableThread
{
public:

    ThreadPoolThread()
        : QThreadPoolThread()
    {
    }

    virtual ~ThreadPoolThread() {}
};
} // anon

ThreadPool::ThreadPool()
    : QThreadPool()
{
}

ThreadPool::~ThreadPool()
{
}

QThreadPoolThread*
ThreadPool::createThreadPoolThread() const
{
    ThreadPoolThread* ret = new ThreadPoolThread();

    ret->setObjectName( QLatin1String("Global Thread (Pooled)") );

    return ret;
}

#endif // ifdef QT_CUSTOM_THREADPOOL

NATRON_NAMESPACE_EXIT;

