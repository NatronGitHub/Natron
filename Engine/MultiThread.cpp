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

#include "MultiThread.h"

#include <map>
#include <list>

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
CLANG_DIAG_OFF(deprecated-register) //'register' storage class specifier is deprecated
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QMutex>
#include <QtCore/QThreadPool>
#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QThread>
#include <QtCore/QReadWriteLock>
#include <QtCore/QAtomicInt>
#include <QtCore/QThreadStorage>
#include <QtConcurrentMap> // QtCore on Qt4, QtConcurrent on Qt5
CLANG_DIAG_ON(deprecated-register)
CLANG_DIAG_ON(uninitialized)

GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
// /usr/local/include/boost/bind/arg.hpp:37:9: warning: unused typedef 'boost_static_assert_typedef_37' [-Wunused-local-typedef]
#include <boost/bind.hpp>
#include <boost/algorithm/string/predicate.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON

#include "Engine/AppManager.h"
#include "Engine/EffectInstance.h"
#include "Engine/Node.h"
#include "Engine/Settings.h"
#include "Engine/TLSHolder.h"
#include "Engine/TreeRenderQueueManager.h"
#include "Engine/EffectInstanceTLSData.h"
#include "Engine/ThreadPool.h"

#define NATRON_USE_MULTITHREAD_V2

NATRON_NAMESPACE_ENTER

struct MultiThreadThreadData
{
    // Index of the thread. This is a list so that the launchThread functino
    // may be used recursively.
    std::list<unsigned int> indices;
};

typedef std::map<QThread*, MultiThreadThreadData> PerThreadMultiThreadDataMap;

struct MultiThreadPrivate
{

    PerThreadMultiThreadDataMap threadsData;
    mutable QReadWriteLock threadsDataMutex;

    MultiThreadPrivate()
    : threadsData()
    , threadsDataMutex()
    {
    }

    void pushThreadIndex(QThread* thread, unsigned int index)
    {
        QWriteLocker k(&threadsDataMutex);
        MultiThreadThreadData& data = threadsData[thread];
        data.indices.push_back(index);
    }
    void popThreadIndex(QThread* thread)
    {
        QWriteLocker k(&threadsDataMutex);
        PerThreadMultiThreadDataMap::iterator foundThread = threadsData.find(thread);
        assert(foundThread != threadsData.end());
        if (foundThread != threadsData.end()) {
            assert(!foundThread->second.indices.empty());
            foundThread->second.indices.pop_back();
            if (foundThread->second.indices.empty()) {
                threadsData.erase(foundThread);
            }
        }
    }

    bool getThreadIndex(QThread* thread, unsigned int* index) const
    {
        QReadLocker k(&threadsDataMutex);
        PerThreadMultiThreadDataMap::const_iterator foundThread = threadsData.find(thread);
        if (foundThread == threadsData.end()) {
            return false;
        }
        if (foundThread->second.indices.empty()) {
            return false;
        }
        *index = foundThread->second.indices.back();
        return true;
    }
};

NATRON_NAMESPACE_ANONYMOUS_ENTER

static void copyOFXRenderTLS(const EffectInstancePtr& effect,
                             QThread* spawnerThread,
                             boost::scoped_ptr<RenderActionArgsSetter_RAII> &renderActionData)
{
    EffectInstanceTLSDataPtr tlsSpawnerThread = effect->getTLSObjectForThread(spawnerThread);
    EffectInstanceTLSDataPtr tlsSpawnedThread = effect->getOrCreateTLSObject();
    if (!tlsSpawnerThread || !tlsSpawnedThread) {
        return;
    }


    // Copy the current render action args: this will enable threads from the multi-thread suite to
    // correctly know that they are in a render action.
    TimeValue time;
    ViewIdx view;
    RenderScale proxyScale;
    unsigned int mipMapLevel;
    RectI renderWindow;
    std::map<ImagePlaneDesc, ImagePtr> outputPlanes;
    if (tlsSpawnerThread->getCurrentRenderActionArgs(&time, &view, &proxyScale, &mipMapLevel, &renderWindow, &outputPlanes)) {
        renderActionData.reset(new RenderActionArgsSetter_RAII(tlsSpawnedThread, time, view, proxyScale, mipMapLevel, renderWindow, outputPlanes));
    }

}

// Using QtConcurrent doesn't work with The Foundry Furnace plug-ins because they expect fresh threads
// to be created. As QtConcurrent's thread-pool recycles thread, it seems to make Furnace crash.
// We think this is because Furnace must keep an internal thread-local state that becomes then dirty
// if we re-use the same thread.

static ActionRetCodeEnum
threadFunctionWrapper(MultiThreadPrivate* imp,
                      MultiThread::ThreadFunctor func,
                      unsigned int threadIndex,
                      unsigned int threadMax,
                      QThread* spawnerThread,
                      const EffectInstancePtr& effect,
                      void *customArg)
{
    assert(threadIndex < threadMax);

    QThread* spawnedThread = QThread::currentThread();

    imp->pushThreadIndex(spawnedThread, threadIndex);

    // If we launched the functor in a new thread,
    // this thread doesn't have any TLS set.
    // However some functions in the OpenFX API might require it.
    boost::scoped_ptr<RenderActionArgsSetter_RAII> renderActionData;
    if (spawnedThread != spawnerThread && effect) {
        copyOFXRenderTLS(effect, spawnerThread , renderActionData);
    }

    ActionRetCodeEnum ret = eActionStatusOK;
    try {
        ret = func(threadIndex, threadMax, customArg);
    } catch (const std::bad_alloc & ba) {
        ret =  eActionStatusOutOfMemory;
    } catch (...) {
        ret =  eActionStatusFailed;
    }

    // Reset back the index otherwise it could mess up the indices if the same thread is re-used
    imp->popThreadIndex(spawnedThread);

    return ret;
} // threadFunctionWrapper

class NonThreadPoolThread
: public QThread
, public AbortableThread
{
public:
    NonThreadPoolThread(MultiThreadPrivate* imp,
                        MultiThread::ThreadFunctor func,
                        unsigned int threadIndex,
                        unsigned int threadMax,
                        void *customArg,
                        QThread* spawnerThread,
                        const EffectInstancePtr& effect,
                        ActionRetCodeEnum *stat)
    : QThread()
    , AbortableThread(this)
    , _imp(imp)
    , _func(func)
    , _threadIndex(threadIndex)
    , _threadMax(threadMax)
    , _customArg(customArg)
    , _spawnerThread(spawnerThread)
    , _effect(effect)
    , _stat(stat)
    {
        setThreadName("Multi-thread suite");
    }

private:

    virtual void run() OVERRIDE FINAL
    {
        assert(_threadIndex < _threadMax);

        _imp->pushThreadIndex(this, _threadIndex);

        // If we launched the functor in a new thread,
        // this thread doesn't have any TLS set.
        // However some functions in the OpenFX API might require it.
        boost::scoped_ptr<RenderActionArgsSetter_RAII> renderActionData;
        if (_effect) {
            copyOFXRenderTLS(_effect, _spawnerThread, renderActionData);
        }


        assert(*_stat == eActionStatusFailed);
        try {
            _func(_threadIndex, _threadMax, _customArg);
            *_stat = eActionStatusOK;
        } catch (const std::bad_alloc & ba) {
            *_stat = eActionStatusOutOfMemory;
        } catch (...) {
        }

        // Reset back the index otherwise it could mess up the indexes if the same thread is re-used
        _imp->popThreadIndex(this);
    }

private:
    MultiThreadPrivate* _imp;
    MultiThread::ThreadFunctor *_func;
    unsigned int _threadIndex;
    unsigned int _threadMax;
    void *_customArg;
    QThread* _spawnerThread;
    EffectInstancePtr _effect;
    ActionRetCodeEnum *_stat;
};

NATRON_NAMESPACE_ANONYMOUS_EXIT


MultiThread::MultiThread()
: _imp(new MultiThreadPrivate())
{

}

MultiThread::~MultiThread()
{

}


struct MultiThreadFuturePrivate
{
    boost::scoped_ptr<QFuture<ActionRetCodeEnum> > future;
    std::vector<unsigned int> threadIndices;
    ActionRetCodeEnum status;

    MultiThreadFuturePrivate(ActionRetCodeEnum initialStatus)
    : future()
    , threadIndices()
    , status(initialStatus)
    {

    }
};

MultiThreadFuture::MultiThreadFuture(ActionRetCodeEnum initialStatus)
: _imp(new MultiThreadFuturePrivate(initialStatus))
{

}

ActionRetCodeEnum
MultiThreadFuture::waitForFinished()
{
    if (_imp->future) {
        ReleaseTPThread_RAII releaser;

        _imp->future->waitForFinished();

        if (isFailureRetCode(_imp->status)) {
            return _imp->status;
        }
        for (QFuture<ActionRetCodeEnum>::const_iterator it = _imp->future->begin(); it != _imp->future->end(); ++it) {
            ActionRetCodeEnum stat = *it;
            if (isFailureRetCode(stat)) {
                return stat;
            }
        }
    }
    return _imp->status;
}

MultiThreadFuture::~MultiThreadFuture()
{

}

// A high priority render is the one that in the front of the render queue of the manager
static bool isHighPriorityRender(const EffectInstancePtr& effect)
{
    if (!effect) {
        return true;
    }
    TreeRenderPtr currentRender = effect->getCurrentRender();
    if (!currentRender) {
        return true;
    }
    int renderIndex, nRenders;
    appPTR->getTasksQueueManager()->getRenderIndex(currentRender, &renderIndex, &nRenders);
    if (renderIndex == -1 || nRenders == 1) {
        return true;
    }
    assert(renderIndex < nRenders);
    return renderIndex == 0;
}

void
MultiThread::launchThreadsInThreadPool(ThreadFunctor func, void *customArg, unsigned int startTaskIndex, unsigned int nTasks, const EffectInstancePtr& effect, const MultiThreadFuturePtr& ret)
{
    QThread* spawnerThread = QThread::currentThread();

    // Get the global multi-thread handler data
    MultiThreadPrivate* imp = appPTR->getMultiThreadHandler()->_imp.get();

    // If the current thread is a thread-pool thread, make it also do an iteration instead
    // of waiting for other threads
    bool isThreadPoolThread = isRunningInThreadPoolThread();

    unsigned int mappedNTasks = (isThreadPoolThread) ? nTasks - 1 : nTasks;

    if (mappedNTasks > 0 && startTaskIndex < mappedNTasks) {
        unsigned int nTasksRemaining = mappedNTasks - startTaskIndex;
        ret->_imp->threadIndices.resize(nTasksRemaining);
        {
            unsigned int index = 0;
            for (unsigned int i = startTaskIndex; i < mappedNTasks; ++i, ++index) {
                ret->_imp->threadIndices[index] = i;
            }
        }

        // DON'T set the maximum thread count: this is a global application setting
        ret->_imp->future.reset(new QFuture<ActionRetCodeEnum>);
        *ret->_imp->future = QtConcurrent::mapped( ret->_imp->threadIndices, boost::bind(threadFunctionWrapper, imp, func, _1, nTasks, spawnerThread, effect, customArg) );

    }
    
    // Do one iteration in this thread
    if (isThreadPoolThread) {
        ActionRetCodeEnum stat = threadFunctionWrapper(imp, func, nTasks - 1, nTasks, spawnerThread, effect, customArg);
        if (isFailureRetCode(stat)) {
            // This thread failed, wait for other threads and exit
            ret->_imp->status = stat;
            if (ret->_imp->future) {
                ret->_imp->future->waitForFinished();
                ret->_imp->future.reset();
            }
        }
    }
} // launchThreadsInThreadPool

MultiThreadFuturePtr
MultiThread::launchThreadsInternal_v2(ThreadFunctor func, unsigned int bestNThreads, void *customArg, const EffectInstancePtr& effect)
{
    MultiThreadFuturePtr ret(new MultiThreadFuture);
    if (!func) {
        ret->_imp->status = eActionStatusFailed;
        return ret;
    }

    QThread* spawnerThread = QThread::currentThread();


    const unsigned int maxThreads = (unsigned int)appPTR->getMaxThreadCount();
    bestNThreads = std::max(1u, std::min(maxThreads, bestNThreads));

    // Get the global multi-thread handler data
    MultiThreadPrivate* imp = appPTR->getMultiThreadHandler()->_imp.get();


    unsigned int nTasksProcessed = 0;
    while (nTasksProcessed < bestNThreads) {
        unsigned int nAvailCPUs = getNCPUsAvailable(effect);
        if (nAvailCPUs > 1 && isHighPriorityRender(effect)) {
            launchThreadsInThreadPool(func, customArg, nTasksProcessed, bestNThreads, effect, ret);
            break;
        } else {
            ActionRetCodeEnum stat = threadFunctionWrapper(imp, func, nTasksProcessed, bestNThreads, spawnerThread, effect, customArg);
            if (isFailureRetCode(stat)) {
                // This thread failed, wait for other threads and exit
                ret->_imp->status = stat;
                return ret;
            }
            ++nTasksProcessed;
        }
    }

    return ret;
} // launchThreadsInternal_v2


MultiThreadFuturePtr
MultiThread::launchThreadsInternal(MultiThread::ThreadFunctor func, unsigned int nDesiredThreadsArg, void *customArg, const EffectInstancePtr& effect)
{
    MultiThreadFuturePtr ret(new MultiThreadFuture);
    if (!func) {
        ret->_imp->status = eActionStatusFailed;
        return ret;
    }

    const unsigned int nThreadsAvailable = MultiThread::getNCPUsAvailable(effect);
    assert(nThreadsAvailable > 0);

    if (nDesiredThreadsArg == 0) {
        nDesiredThreadsArg = nThreadsAvailable;
    }



    // from the documentation:
    // "nThreads can be more than the value returned by multiThreadNumCPUs, however
    // the threads will be limitted to the number of CPUs returned by multiThreadNumCPUs."

    if (/* (nDesiredThreads == 1) || */(nThreadsAvailable <= 1) ) {
        // If user wants multiple calls but we only have 1 thread, call the function sequentially
        // multiple times.
        try {
            for (unsigned int i = 0; i < nDesiredThreadsArg; ++i) {
                ActionRetCodeEnum stat = func(i, nDesiredThreadsArg, customArg);
                if (isFailureRetCode(stat)) {
                    ret->_imp->status = stat;
                    return ret;
                }
            }

            ret->_imp->status = eActionStatusOK;
            return ret;
        } catch (...) {
            ret->_imp->status = eActionStatusFailed;
            return ret;
        }
    }

    QThread* spawnerThread = QThread::currentThread();

    // We have 2 back-ends for multi-threading: either a thread pool or threads created on the fly.
    // The first method is to be preferred but can be proven to not work properly with some plug-ins that
    // require thread local storage such as The Foundry Furnace plug-ins.
    bool useThreadPool = true;
    if (effect) {
        NodePtr node = effect->getNode();
        if (node) {
            if (boost::starts_with(node->getPluginID(), "uk.co.thefoundry.furnace")) {
                useThreadPool = false;
            }
        }
    }

    // Get the global multi-thread handler data
    MultiThreadPrivate* imp = appPTR->getMultiThreadHandler()->_imp.get();

    if (useThreadPool) {

        // If the current thread is a thread-pool thread, make it also do an iteration instead
        // of waiting for other threads
        bool isThreadPoolThread = isRunningInThreadPoolThread();

        unsigned int nMappedElements = isThreadPoolThread ? nDesiredThreadsArg - 1 : nDesiredThreadsArg;
        ret->_imp->threadIndices.resize(nMappedElements);
        for (unsigned int i = 0; i < nMappedElements; ++i) {
            ret->_imp->threadIndices[i] = i;
        }

        // DON'T set the maximum thread count: this is a global application setting, and see the documentation excerpt above
        // QThreadPool::globalInstance()->setMaxThreadCount(nThreads);

        if (nMappedElements > 0) {
            ret->_imp->future.reset(new QFuture<ActionRetCodeEnum>);
            *ret->_imp->future = QtConcurrent::mapped( ret->_imp->threadIndices, boost::bind(threadFunctionWrapper, imp, func, _1, nDesiredThreadsArg, spawnerThread, effect, customArg) );
        }

        // Do one iteration in this thread
        if (isThreadPoolThread) {
            ActionRetCodeEnum stat = threadFunctionWrapper(imp, func, nDesiredThreadsArg - 1, nDesiredThreadsArg, spawnerThread, effect, customArg);
            if (isFailureRetCode(stat)) {
                // This thread failed, wait for other threads and exit
                ret->_imp->status = stat;
                if (ret->_imp->future) {
                    ret->_imp->future->waitForFinished();
                    ret->_imp->future.reset();
                }
                return ret;
            }
        }


    } else { // !useThreadPool

        QVector<ActionRetCodeEnum> status(nDesiredThreadsArg); // vector for the return status of each thread
        status.fill(eActionStatusFailed); // by default, a thread fails
        {
            // at most maxConcurrentThread should be running at the same time
            QVector<NonThreadPoolThread*> threads(nDesiredThreadsArg);
            for (unsigned int i = 0; i < nDesiredThreadsArg; ++i) {
                threads[i] = new NonThreadPoolThread(imp, func, i, nDesiredThreadsArg, customArg, spawnerThread, effect, &status[i]);
            }
            unsigned int i = 0; // index of next thread to launch
            unsigned int running = 0; // number of running threads
            unsigned int j = 0; // index of first running thread. all threads before this one are finished running
            while (j < nDesiredThreadsArg) {
                // have no more than maxConcurrentThread threads launched at the same time
                int threadsStarted = 0;
                while (i < nDesiredThreadsArg && running < nDesiredThreadsArg) {
                    threads[i]->start();
                    ++i;
                    ++running;
                    ++threadsStarted;
                }


                // now we've got at most maxConcurrentThread running. wait for each thread and launch a new one
                threads[j]->wait();
                assert( !threads[j]->isRunning() );
                assert( threads[j]->isFinished() );
                delete threads[j];
                ++j;
                --running;

            }
            assert(running == 0);
        }
        // check the return status of each thread, return the first error found
        for (QVector<ActionRetCodeEnum>::const_iterator it = status.begin(); it != status.end(); ++it) {
            ActionRetCodeEnum stat = *it;
            if (isFailureRetCode(stat)) {
                ret->_imp->status = stat;
                return ret;
            }
        }
    } // useThreadPool

    return ret;
} // launchThreadsInternal

ActionRetCodeEnum
MultiThread::launchThreadsBlocking(ThreadFunctor func, unsigned int nThreads, void *customArg, const EffectInstancePtr& effect)
{
#ifdef NATRON_USE_MULTITHREAD_V2
    MultiThreadFuturePtr ret = launchThreadsInternal_v2(func, nThreads, customArg, effect);
    return ret->waitForFinished();
#else
    MultiThreadFuturePtr ret = launchThreadsInternal(func, nThreads, customArg, effect);
    return ret->waitForFinished();
#endif
}

MultiThreadFuturePtr
MultiThread::launchThreadsNonBlocking(ThreadFunctor func, unsigned int nThreads, void *customArg, const EffectInstancePtr& effect)
{
#ifdef NATRON_USE_MULTITHREAD_V2
    return launchThreadsInternal_v2(func, nThreads, customArg, effect);
#else
    return launchThreadsInternal(func, nThreads, customArg, effect);
#endif
}


unsigned int
MultiThread::getNCPUsAvailable(const EffectInstancePtr& /*effect*/)
{

    // activeThreadCount may be negative (for example if releaseThread() is called)
    int activeThreadsCount = QThreadPool::globalInstance()->activeThreadCount();

    // If we are running in the thread pool already, count this thread as available
    if (isRunningInThreadPoolThread()) {
        --activeThreadsCount;
    }

    // Clamp to 0
    activeThreadsCount = std::max( 0, activeThreadsCount);

    // maxThreadCount() is set by the setting the preferences
    const int maxThreadsCount = std::max(1,QThreadPool::globalInstance()->maxThreadCount());

    int ret = std::max(1, maxThreadsCount - activeThreadsCount);
    return ret;
#if 0
    // If the effect is currently rendering and it is the first render in the TreeRenderQueueManager priority list
    // allocate it more CPU than other renders.
    if (!effect || ret <= 1) {
        return ret;
    }
    TreeRenderPtr currentRender = effect->getCurrentRender();
    if (!currentRender) {
        return ret;
    }
    int renderIndex, nRenders;
    appPTR->getTasksQueueManager()->getRenderIndex(currentRender, &renderIndex, &nRenders);
    if (renderIndex == -1 || nRenders == 1) {
        // If there's a single ongoing render or it is not registered in the manager, return the actual num cpu
        return ret;
    }
    assert(renderIndex < nRenders);

    // This is the number of physical cores excluding logical cores (not counting hyperthreading).
    const int nPhysicalThreadCount = appPTR->getPhysicalThreadCount();

    // If the current available num CPU is below the physical cores, allocate everything to the highest priority render
    if (ret <= nPhysicalThreadCount) {
        if (renderIndex == 0) {
            return ret;
        } else {
            return 1;
        }
    } else {
        if (renderIndex == 0) {
            return nPhysicalThreadCount;
        } else {
            double perThreadRet = (ret - nPhysicalThreadCount) / (double)(nRenders - 1);
            perThreadRet = std::max(1.,std::floor(perThreadRet + 0.5));
            return perThreadRet;
        }
    }
#endif
} // getNCPUsAvailable

ActionRetCodeEnum
MultiThread::getCurrentThreadIndex(unsigned int *threadIndex)
{
    QThread* thisThread = QThread::currentThread();
    if (!thisThread) {
        return eActionStatusFailed;
    }

    // Get the global multi-thread handler data
    MultiThreadPrivate* imp = appPTR->getMultiThreadHandler()->_imp.get();
    if (!imp->getThreadIndex(thisThread, threadIndex)) {
        return eActionStatusFailed;
    }
    return eActionStatusOK;
}

bool
MultiThread::isCurrentThreadSpawnedThread()
{
    unsigned int index;
    ActionRetCodeEnum stat = getCurrentThreadIndex(&index);
    (void)index;
    return stat == eActionStatusOK;
}

MultiThreadProcessorBase::MultiThreadProcessorBase(const EffectInstancePtr& effect)
:  _effect(effect)
{

}

MultiThreadProcessorBase::~MultiThreadProcessorBase()
{

}

ActionRetCodeEnum
MultiThreadProcessorBase::staticMultiThreadFunction(unsigned int threadIndex,
                                                    unsigned int threadMax,
                                                    void *customArg)
{
    MultiThreadProcessorBase* processor = (MultiThreadProcessorBase*)customArg;
    return processor->multiThreadFunction(threadIndex, threadMax);
}

ActionRetCodeEnum
MultiThreadProcessorBase::launchThreadsBlocking(unsigned int nCPUs)
{

#ifdef NATRON_USE_MULTITHREAD_V2
    if (nCPUs == 0) {
        nCPUs = appPTR->getMaxThreadCount();
    }
    ActionRetCodeEnum stat = MultiThread::launchThreadsBlocking(staticMultiThreadFunction, nCPUs, (void*)this /*customArgs*/, _effect);
    return stat;

#else
    // if 0, use all the CPUs we can
    if (nCPUs == 0) {
        nCPUs = MultiThread::getNCPUsAvailable(_effect);
    }

    // if 1 cpu, don't bother with the threading
    if (nCPUs == 1) {
        return multiThreadFunction(0, 1);
    } else {
        // OK do it
        ActionRetCodeEnum stat = MultiThread::launchThreadsBlocking(staticMultiThreadFunction, nCPUs, (void*)this /*customArgs*/, _effect);
        return stat;
    }
#endif
}

MultiThreadFuturePtr
MultiThreadProcessorBase::launchThreadsNonBlocking(unsigned int nCPUs)
{
#ifdef NATRON_USE_MULTITHREAD_V2
    if (nCPUs == 0) {
        nCPUs = appPTR->getMaxThreadCount();
    }
    MultiThreadFuturePtr ret = MultiThread::launchThreadsNonBlocking(staticMultiThreadFunction, nCPUs, (void*)this /*customArgs*/, _effect);
    return ret;

#else
    // if 0, use all the CPUs we can
    if (nCPUs == 0) {
        nCPUs = MultiThread::getNCPUsAvailable(_effect);
    }

    // if 1 cpu, don't bother with the threading
    if (nCPUs == 1) {
        ActionRetCodeEnum stat = multiThreadFunction(0, 1);
        MultiThreadFuturePtr ret(new MultiThreadFuture(stat));
        return ret;
    } else {
        // OK do it
        MultiThreadFuturePtr ret = MultiThread::launchThreadsNonBlocking(staticMultiThreadFunction, nCPUs, (void*)this /*customArgs*/, _effect);
        return ret;
    }
#endif
}

ImageMultiThreadProcessorBase::ImageMultiThreadProcessorBase(const EffectInstancePtr& effect)
: MultiThreadProcessorBase(effect)
{

}

ImageMultiThreadProcessorBase::~ImageMultiThreadProcessorBase()
{

}

void
ImageMultiThreadProcessorBase::setRenderWindow(const RectI& renderWindow)
{
    _renderWindow = renderWindow;
}


void
ImageMultiThreadProcessorBase::getThreadRange(unsigned int threadID, unsigned int nThreads, int ibegin, int iend, int* ibegin_range, int* iend_range)
{
    unsigned int di = iend - ibegin;
    // the following is equivalent to std::ceil(di/(double)nThreads); but doesn't require <cmath>
    unsigned int r = (di + nThreads - 1) / nThreads;

    if (r == 0) {
        // there are more threads than lines to process
        r = 1;
    }
    if ((int)threadID * r >= di) {
        // empty range
        *ibegin_range = *iend_range = iend;
        return;
    }
    *ibegin_range = ibegin + threadID * r;
    unsigned int step = (threadID + 1) * r;
    *iend_range = ibegin + (step < di ? step : di);
}

ActionRetCodeEnum
ImageMultiThreadProcessorBase::multiThreadFunction(unsigned int threadID,
                                                   unsigned int nThreads)
{
    // Each threads get a rectangular portion but full scan-lines
    RectI win = _renderWindow;
    getThreadRange(threadID, nThreads, _renderWindow.y1, _renderWindow.y2, &win.y1, &win.y2);

    if ( (win.y2 - win.y1) > 0 ) {
        // and render that thread on each
        return multiThreadProcessImages(win);
    }
    return eActionStatusOK;
}


ActionRetCodeEnum
ImageMultiThreadProcessorBase::process()
{

    // make sure there are at least 4096 pixels per CPU and at least 1 line par CPU
    unsigned int nCPUs = ( std::min(_renderWindow.x2 - _renderWindow.x1, 4096) *
                          (_renderWindow.y2 - _renderWindow.y1) ) / 4096;

#ifndef NATRON_USE_MULTITHREAD_V2
    // make sure the number of CPUs is valid (and use at least 1 CPU)
    nCPUs = std::max(1u, std::min( nCPUs, MultiThread::getNCPUsAvailable(_effect))) ;
#endif

    // call the base multi threading code
    return launchThreadsBlocking(nCPUs);

}

NATRON_NAMESPACE_EXIT
