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
#include "Engine/ThreadPool.h"

// An effect may not use more than this amount of threads
#define NATRON_MULTI_THREAD_SUITE_MAX_NUM_CPU 4


NATRON_NAMESPACE_ENTER;

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

    MultiThreadPrivate()
    : threadsData()
    {

    }
};

NATRON_NAMESPACE_ANONYMOUS_ENTER

// Using QtConcurrent doesn't work with The Foundry Furnace plug-ins because they expect fresh threads
// to be created. As QtConcurrent's thread-pool recycles thread, it seems to make Furnace crash.
// We think this is because Furnace must keep an internal thread-local state that becomes then dirty
// if we re-use the same thread.

static StatusEnum
threadFunctionWrapper(MultiThreadPrivate* imp,
                      MultiThread::ThreadFunctor func,
                      unsigned int threadIndex,
                      unsigned int threadMax,
                      QThread* spawnerThread,
                      void *customArg,
                      const EffectInstancePtr& effect,
                      const TreeRenderNodeArgsPtr& renderArgs)
{
    assert(threadIndex < threadMax);

    QThread* spawnedThread = QThread::currentThread();

    MultiThreadThreadData& spawnedThreadData = imp->threadsData[spawnedThread];
    spawnedThreadData.indices.push_back(threadIndex);

    // If we launched the functor in a new thread,
    // this thread doesn't have any TLS set.
    // However some functions in the OpenFX API might require it.
    // We flag on the application that this thread is spawned from
    // another thread and whenever it will try to access the TLS
    // it will copy the required data.
    if (spawnedThread != spawnerThread) {
        appPTR->getAppTLS()->softCopy(spawnerThread, spawnedThread);
    }

    StatusEnum ret = eStatusOK;
    try {
        ret = func(threadIndex, threadMax, customArg, effect, renderArgs);
    } catch (const std::bad_alloc & ba) {
        ret =  eStatusOutOfMemory;
    } catch (...) {
        ret =  eStatusFailed;
    }

    // Reset back the index otherwise it could mess up the indices if the same thread is re-used
    spawnedThreadData.indices.pop_back();

    // If we used TLS on this thread, clean it up.
    if (spawnedThread != spawnerThread) {
        appPTR->getAppTLS()->cleanupTLSForThread();
    }

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
                        QThread* spawnerThread,
                        void *customArg,
                        const EffectInstancePtr& effect,
                        const TreeRenderNodeArgsPtr& renderArgs,
                        StatusEnum *stat)
    : QThread()
    , AbortableThread(this)
    , _imp(imp)
    , _func(func)
    , _threadIndex(threadIndex)
    , _threadMax(threadMax)
    , _spawnerThread(spawnerThread)
    , _customArg(customArg)
    , _effect(effect)
    , _renderArgs(renderArgs)
    , _stat(stat)
    {
        setThreadName("Multi-thread suite");
    }

private:

    virtual void run() OVERRIDE FINAL
    {
        assert(_threadIndex < _threadMax);

        MultiThreadThreadData& spawnedThreadData = _imp->threadsData[this];
        spawnedThreadData.indices.push_back(_threadIndex);


        // This thread doesn't have any TLS set.
        // However some functions in the OpenFX API might require it.
        // We flag on the application that this thread is spawned from
        // another thread and whenever it will try to access the TLS
        // it will copy the required data.

        appPTR->getAppTLS()->softCopy(_spawnerThread, this);

        assert(*_stat == eStatusFailed);
        try {
            _func(_threadIndex, _threadMax, _customArg, _effect, _renderArgs);
            *_stat = eStatusOK;
        } catch (const std::bad_alloc & ba) {
            *_stat = eStatusOutOfMemory;
        } catch (...) {
        }

        // Reset back the index otherwise it could mess up the indexes if the same thread is re-used
        spawnedThreadData.indices.pop_back();

        // If we used TLS on this thread, clean it up.
        appPTR->getAppTLS()->cleanupTLSForThread();
    }

private:
    MultiThreadPrivate* _imp;
    MultiThread::ThreadFunctor *_func;
    unsigned int _threadIndex;
    unsigned int _threadMax;
    QThread* _spawnerThread;
    void *_customArg;
    EffectInstancePtr _effect;
    TreeRenderNodeArgsPtr _renderArgs;
    StatusEnum *_stat;
};

NATRON_NAMESPACE_ANONYMOUS_EXIT


MultiThread::MultiThread()
: _imp(new MultiThreadPrivate())
{

}

MultiThread::~MultiThread()
{

}


StatusEnum
MultiThread::launchThreads(ThreadFunctor func, unsigned int nThreads, void *customArg, const EffectInstancePtr& effect, const TreeRenderNodeArgsPtr& renderArgs)
{
    if (!func) {
        return eStatusFailed;
    }

    unsigned int maxConcurrentThread = getNCPUsAvailable();


    // from the documentation:
    // "nThreads can be more than the value returned by multiThreadNumCPUs, however
    // the threads will be limitted to the number of CPUs returned by multiThreadNumCPUs."

    if ( (nThreads == 1) || (maxConcurrentThread <= 1) ) {
        // If user wants multiple calls but we only have 1 thread, call the function sequentially
        // multiple times.
        try {
            for (unsigned int i = 0; i < nThreads; ++i) {
                StatusEnum stat = func(i, nThreads, customArg, effect, renderArgs);
                if (stat != eStatusOK) {
                    return stat;
                }
            }

            return eStatusOK;
        } catch (...) {
            return eStatusFailed;
        }
    }

    QThread* spawnerThread = QThread::currentThread();

    // We have 2 back-ends for multi-threading: either a thread pool or threads creating on the fly.
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

        std::vector<unsigned int> threadIndexes(nThreads);
        for (unsigned int i = 0; i < nThreads; ++i) {
            threadIndexes[i] = i;
        }

        // If the current thread is a thread-pool thread, make it also do an iteration instead
        // of waiting for other threads
        bool isThreadPoolThread = isRunningInThreadPoolThread();
        if (isThreadPoolThread) {
            threadIndexes.pop_back();
        }

        // DON'T set the maximum thread count: this is a global application setting, and see the documentation excerpt above
        // QThreadPool::globalInstance()->setMaxThreadCount(nThreads);

        QFuture<StatusEnum> future = QtConcurrent::mapped( threadIndexes, boost::bind(threadFunctionWrapper, imp, func, _1, nThreads, spawnerThread, customArg, effect, renderArgs) );

        // Do one iteration in this thread
        if (isThreadPoolThread) {
            StatusEnum stat = threadFunctionWrapper(imp, func, nThreads - 1, nThreads, spawnerThread, customArg, effect, renderArgs);
            if (stat != eStatusOK) {
                // This thread failed, wait for other threads and exit
                future.waitForFinished();
                return stat;
            }
        }

        // Wait for other threads
        future.waitForFinished();

        // DON'T reset back to the original value the maximum thread count
        // QThreadPool::globalInstance()->setMaxThreadCount(QThread::idealThreadCount());

        for (QFuture<StatusEnum>::const_iterator it = future.begin(); it != future.end(); ++it) {
            StatusEnum stat = *it;
            if (stat != eStatusOK) {
                return stat;
            }
        }

    } else { // !useThreadPool

        QVector<StatusEnum> status(nThreads); // vector for the return status of each thread
        status.fill(eStatusFailed); // by default, a thread fails
        {
            // at most maxConcurrentThread should be running at the same time
            QVector<NonThreadPoolThread*> threads(nThreads);
            for (unsigned int i = 0; i < nThreads; ++i) {
                threads[i] = new NonThreadPoolThread(imp, func, i, nThreads, spawnerThread, customArg, effect, renderArgs, &status[i]);
            }
            unsigned int i = 0; // index of next thread to launch
            unsigned int running = 0; // number of running threads
            unsigned int j = 0; // index of first running thread. all threads before this one are finished running
            while (j < nThreads) {
                // have no more than maxConcurrentThread threads launched at the same time
                int threadsStarted = 0;
                while (i < nThreads && running < maxConcurrentThread) {
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
        for (QVector<StatusEnum>::const_iterator it = status.begin(); it != status.end(); ++it) {
            StatusEnum stat = *it;
            if (stat != eStatusOK) {
                return stat;
            }
        }
    } // useThreadPool

    return eStatusOK;
} // multiThread


unsigned int
MultiThread::getNCPUsAvailable()
{
    // activeThreadCount may be negative (for example if releaseThread() is called)
    int activeThreadsCount = QThreadPool::globalInstance()->activeThreadCount();

    // If we are running in the thread pool already, count this thread as available
    if (isRunningInThreadPoolThread()) {
        --activeThreadsCount;
    }

    // Clamp to 0
    activeThreadsCount = std::max( 0, activeThreadsCount);

    // better than QThread::idealThreadCount(), because it can be set by a global preference
    const int maxThreadsCount = QThreadPool::globalInstance()->maxThreadCount();
    assert(maxThreadsCount >= 0);

    int ret = std::max(1, maxThreadsCount - activeThreadsCount);
    return ret;
} // getNCPUsAvailable

StatusEnum
MultiThread::getCurrentThreadIndex(unsigned int *threadIndex)
{
    QThread* thisThread = QThread::currentThread();
    if (!thisThread) {
        return eStatusFailed;
    }

    // Get the global multi-thread handler data
    MultiThreadPrivate* imp = appPTR->getMultiThreadHandler()->_imp.get();

    PerThreadMultiThreadDataMap::const_iterator foundThread = imp->threadsData.find(thisThread);
    if (foundThread == imp->threadsData.end()) {
        return eStatusFailed;
    }
    if (foundThread->second.indices.empty()) {
        return eStatusFailed;
    }
    *threadIndex = foundThread->second.indices.back();
    return eStatusOK;
}

bool
MultiThread::isCurrentThreadSpawnedThread()
{
    unsigned int index;
    StatusEnum stat = getCurrentThreadIndex(&index);
    (void)index;
    return stat == eStatusOK;
}

MultiThreadProcessorBase::MultiThreadProcessorBase(const EffectInstancePtr& effect, const TreeRenderNodeArgsPtr& renderArgs)
: _effect(effect)
, _renderArgs(renderArgs)
{

}

MultiThreadProcessorBase::~MultiThreadProcessorBase()
{

}

StatusEnum
MultiThreadProcessorBase::staticMultiThreadFunction(unsigned int threadIndex,
                                                    unsigned int threadMax,
                                                    void *customArg,
                                                    const EffectInstancePtr& effect,
                                                    const TreeRenderNodeArgsPtr& renderArgs)
{
    MultiThreadProcessorBase* processor = (MultiThreadProcessorBase*)customArg;
    return processor->multiThreadFunction(threadIndex, threadMax, effect, renderArgs);
}

StatusEnum
MultiThreadProcessorBase::launchThreads(unsigned int nCPUs)
{
    // if 0, use all the CPUs we can
    if (nCPUs == 0) {
        nCPUs = MultiThread::getNCPUsAvailable();
    }

    // if 1 cpu, don't bother with the threading
    if (nCPUs == 1) {
        return multiThreadFunction(0, 1, _effect, _renderArgs);
    } else {
        // OK do it
        StatusEnum stat = MultiThread::launchThreads(staticMultiThreadFunction, nCPUs, (void*)this /*customArgs*/, _effect, _renderArgs);
        return stat;
    }
}

ImageMultiThreadProcessorBase::ImageMultiThreadProcessorBase(const EffectInstancePtr& effect,
                                                             const TreeRenderNodeArgsPtr& renderArgs)
: MultiThreadProcessorBase(effect, renderArgs)
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

StatusEnum
ImageMultiThreadProcessorBase::multiThreadFunction(unsigned int threadID,
                                                   unsigned int nThreads,
                                                   const EffectInstancePtr& effect,
                                                   const TreeRenderNodeArgsPtr& renderArgs)
{
    // Each threads get a rectangular portion but full scan-lines
    RectI win = _renderWindow;
    getThreadRange(threadID, nThreads, _renderWindow.y1, _renderWindow.y2, &win.y1, &win.y2);

    if ( (win.y2 - win.y1) > 0 ) {
        // and render that thread on each
        return multiThreadProcessImages(win, effect, renderArgs);
    }
    return eStatusOK;
}


StatusEnum
ImageMultiThreadProcessorBase::process()
{

    // make sure there are at least 4096 pixels per CPU and at least 1 line par CPU
    unsigned int nCPUs = ( std::min(_renderWindow.x2 - _renderWindow.x1, 4096) *
                          (_renderWindow.y2 - _renderWindow.y1) ) / 4096;

    // make sure the number of CPUs is valid (and use at least 1 CPU)
    nCPUs = std::max(1u, std::min( nCPUs, MultiThread::getNCPUsAvailable())) ;

    // call the base multi threading code
    return launchThreads(nCPUs);

}

NATRON_NAMESPACE_EXIT;
