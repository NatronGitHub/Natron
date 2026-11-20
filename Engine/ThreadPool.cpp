/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2023 The Natron developers
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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "ThreadPool.h"

#include <algorithm> // min
#include <cstddef>
#include <limits>
#include <string>
#include <sstream> // stringstream

#ifndef __NATRON_WIN32__
#include <pthread.h>
#endif

#include <QAtomicInt>
#include <QMutex>
#include <QThread>
#include <QThreadPool>

#include "Engine/AbortableRenderInfo.h"
#include "Engine/Node.h"

NATRON_NAMESPACE_ENTER


struct AbortableThreadPrivate
{
    QThread* thread;
    std::string threadName;
    mutable QMutex abortInfoMutex;
    bool isRenderResponseToUserInteraction;
    AbortableRenderInfoWPtr abortInfo;
    EffectInstanceWPtr treeRoot;
    bool abortInfoValid;
    std::string currentActionName;
    NodeWPtr currentActionNode;

    AbortableThreadPrivate(QThread* thread)
        : thread(thread)
        , threadName()
        , abortInfoMutex()
        , isRenderResponseToUserInteraction(false)
        , abortInfo()
        , treeRoot()
        , abortInfoValid(false)
        , currentActionName()
        , currentActionNode()
    {
    }
};

unsigned int
getRenderThreadStackSize()
{
    static const unsigned int stackSize = []() {
        std::size_t size = NATRON_RENDER_THREAD_STACK_SIZE;
#ifndef __NATRON_WIN32__
        // The default stack size of new threads (on Linux, RLIMIT_STACK, i.e. `ulimit -s`)
        pthread_attr_t attr;
        if (pthread_attr_init(&attr) == 0) {
            std::size_t defaultSize = 0;
            if ( (pthread_attr_getstacksize(&attr, &defaultSize) == 0) && (defaultSize > size) ) {
                size = defaultSize;
            }
            pthread_attr_destroy(&attr);
        }
#endif
        return (unsigned int)std::min<std::size_t>( size, std::numeric_limits<unsigned int>::max() );
    }();

    return stackSize;
}

AbortableThread::AbortableThread(QThread* thread)
    : _imp( new AbortableThreadPrivate(thread) )
{
    // Every thread that renders derives from AbortableThread (#1434)
    assert( !thread->isRunning() );
    thread->setStackSize( getRenderThreadStackSize() );
}

AbortableThread::~AbortableThread()
{
}

void
AbortableThread::setThreadName(const std::string& threadName)
{
    std::stringstream ss;

    ss << threadName << " (" << this << ")";
    _imp->threadName = ss.str();
    _imp->thread->setObjectName( QString::fromUtf8( _imp->threadName.c_str() ) );
}

const std::string&
AbortableThread::getThreadName() const
{
    return _imp->threadName;
}

void
AbortableThread::setCurrentActionInfos(const std::string& actionName,
                                       const NodePtr& node)
{
    assert(QThread::currentThread() == _imp->thread);

    QMutexLocker k(&_imp->abortInfoMutex);
    _imp->currentActionName = actionName;
    _imp->currentActionNode = node;
}

void
AbortableThread::getCurrentActionInfos(std::string* actionName,
                                       NodePtr* node) const
{
    QMutexLocker k(&_imp->abortInfoMutex);

    *actionName = _imp->currentActionName;
    *node = _imp->currentActionNode.lock();
}

void
AbortableThread::killThread()
{
    _imp->thread->terminate();
}

QThread*
AbortableThread::getThread() const
{
    return _imp->thread;
}

void
AbortableThread::setAbortInfo(bool isRenderResponseToUserInteraction,
                              const AbortableRenderInfoPtr& abortInfo,
                              const EffectInstancePtr& treeRoot)
{
    {
        QMutexLocker k(&_imp->abortInfoMutex);
        _imp->isRenderResponseToUserInteraction = isRenderResponseToUserInteraction;
        _imp->abortInfo = abortInfo;
        _imp->treeRoot = treeRoot;
        _imp->abortInfoValid = true;
    }
    if (abortInfo) {
        abortInfo->registerThreadForRender(this);
    }
}

void
AbortableThread::clearAbortInfo()
{
    AbortableRenderInfoPtr abortInfo;
    {
        QMutexLocker k(&_imp->abortInfoMutex);
        abortInfo = _imp->abortInfo.lock();
        _imp->abortInfo.reset();
        _imp->treeRoot.reset();
        _imp->abortInfoValid = false;
    }

    if (abortInfo) {
        abortInfo->unregisterThreadForRender(this);
    }
}

bool
AbortableThread::getAbortInfo(bool* isRenderResponseToUserInteraction,
                              AbortableRenderInfoPtr* abortInfo,
                              EffectInstancePtr* treeRoot) const
{
    QMutexLocker k(&_imp->abortInfoMutex);

    if (!_imp->abortInfoValid) {
        return false;
    }
    *isRenderResponseToUserInteraction = _imp->isRenderResponseToUserInteraction;
    *abortInfo = _imp->abortInfo.lock();
    *treeRoot = _imp->treeRoot.lock();

    return true;
}

// We patched Qt to be able to derive QThreadPool to control the threads that are spawned to improve performances
// of the EffectInstance::aborted() function
#ifdef QT_CUSTOM_THREADPOOL

NATRON_NAMESPACE_ANONYMOUS_ENTER

class ThreadPoolThread
    : public QThreadPoolThread
      , public AbortableThread
{
public:

    ThreadPoolThread()
        : QThreadPoolThread()
        , AbortableThread(this)
    {
    }

    virtual bool isThreadPoolThread() const { return true; }

    virtual ~ThreadPoolThread() {}
};

NATRON_NAMESPACE_ANONYMOUS_EXIT


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

    ret->setThreadName("Global Thread (Pooled)");

    return ret;
}

#endif // ifdef QT_CUSTOM_THREADPOOL

NATRON_NAMESPACE_EXIT

