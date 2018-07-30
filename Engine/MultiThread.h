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

#ifndef NATRON_ENGINE_MULTITHREAD_H
#define NATRON_ENGINE_MULTITHREAD_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#endif

#include "Global/GlobalDefines.h"
#include "Engine/RectI.h"

#include "Engine/EngineFwd.h"


NATRON_NAMESPACE_ENTER

struct MultiThreadFuturePrivate;
class MultiThreadFuture
{

public:

    MultiThreadFuture(ActionRetCodeEnum initialStatus = eActionStatusOK);

    ~MultiThreadFuture();

    ActionRetCodeEnum waitForFinished();

private:

    friend class MultiThread;

    boost::scoped_ptr<MultiThreadFuturePrivate> _imp;
};

typedef boost::shared_ptr<MultiThreadFuture> MultiThreadFuturePtr;

struct MultiThreadPrivate;
class MultiThread
{
public:


    /** @brief The function type to passed to the multi threading routines

     @param threadIndex unique index of this thread, will be between 0 and threadMax
     @param threadMax to total number of threads executing this function
     @param customArg the argument passed into multiThread
     @param renderArgs A pointer to the current render data for the node launching threads
     This may be used to call the aborted() function of the effect to cancel
     processing quickly

     A function of this type is passed to MultiThread::launchThreads to be launched in multiple threads.
     */
    typedef ActionRetCodeEnum (ThreadFunctor)(unsigned int threadIndex, unsigned int threadMax, void *customArg);
    
    MultiThread();

    ~MultiThread();

    /*
     * @brief Function to spawn SMP threads.
     * This function will spawn nThreads separate threads of computation (typically one per CPU) 
     * to allow something to perform symmetric multi processing. 
     * Each thread will call 'func' passing in the index of the thread and the number of threads actually launched.
     * This function will not return until all the spawned threads have returned.
     * It is up to the host how it waits for all the threads to return (busy wait, blocking, whatever).
     *
     * @param nThreads can be more than the value returned by getNCPUsAvailable,
     * however the threads will be limitted to the number of CPUs returned by getNCPUsAvailable.
     *
     * @param customArg The arguments to passed to the function
     *
     * This function cannot be called recursively.
     * Note that the thread indexes are from 0 to nThreads - 1.
     * http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#OfxMultiThreadSuiteV1_multiThread
     */
    static ActionRetCodeEnum launchThreadsBlocking(ThreadFunctor func, unsigned int nThreads, void *customArg, const EffectInstancePtr& effect);

    /**
     * @brief Same as launchThreadsBlocking except that this functions returns a future object, on which
     **/
    static MultiThreadFuturePtr launchThreadsNonBlocking(ThreadFunctor func, unsigned int nThreads, void *customArg, const EffectInstancePtr& effect);

private:

    static void launchThreadsInThreadPool(ThreadFunctor func, void *customArg, unsigned int startTaskIndex, unsigned int nTasks, const EffectInstancePtr& effect, const MultiThreadFuturePtr& ret);

    static MultiThreadFuturePtr launchThreadsInternal_v2(ThreadFunctor func, unsigned int bestNThreads, void *customArg, const EffectInstancePtr& effect);

    static MultiThreadFuturePtr launchThreadsInternal(ThreadFunctor func, unsigned int nThreads, void *customArg, const EffectInstancePtr& effect);

public:

    /**
     * @brief Function which indicates the number of CPUs available for SMP processing
     * This value may be less than the actual number of CPUs on a machine, as the host may reserve other CPUs for itself.
     *http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#OfxMultiThreadSuiteV1_multiThreadNumCPUs
     **/
    static unsigned int getNCPUsAvailable(const EffectInstancePtr& effect);

    /**
     * @brief Function which indicates the index of the current thread.
     * This function returns the thread index, which is the same as the threadIndex argument passed to the ThreadFunctor.
     * If there are no threads currently spawned, then this function will set threadIndex to 0.
     *
     * http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#OfxMultiThreadSuiteV1_multiThreadIndex
     *
     * Note that the thread indexes are from 0 to nThreads-1, so a return value of 0 does not mean that it's not a spawned thread
     * (use isCurrentThreadSpawnedThread() to check if it's a spawned thread)
     **/
    static ActionRetCodeEnum getCurrentThreadIndex(unsigned int *threadIndex);

    /** 
     * @brief Function to enquire if the calling thread was spawned by launchThreads
     * http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#OfxMultiThreadSuiteV1_multiThreadIsSpawnedThread
     **/
    static bool isCurrentThreadSpawnedThread();

   private:

    boost::scoped_ptr<MultiThreadPrivate> _imp;
};

/**
 * @brief Base class to multi-thread a function, makes use of the global application MultiThread object.
 **/
class MultiThreadProcessorBase
{
protected:

    EffectInstancePtr _effect;

public:

    MultiThreadProcessorBase(const EffectInstancePtr& renderArgs);

    virtual ~MultiThreadProcessorBase();

protected:

    /**
     * @brief function that will be called in each thread.
     * ID is from 0..nThreads-1 nThreads are the number of threads it is being run over */
    virtual ActionRetCodeEnum multiThreadFunction(unsigned int threadID,
                                                  unsigned int nThreads) = 0;

public:
    
    /** @brief Call this to kick off multi threading
     *  @param nCPUs The number of threads to use at most to process this function
     *  If nCPUs is 0, the maximum allowable number of CPUs will be used.
     */
    virtual ActionRetCodeEnum launchThreadsBlocking(unsigned int nCPUs = 0);
    virtual MultiThreadFuturePtr launchThreadsNonBlocking(unsigned int nCPUs = 0);

private:

    // the function passed to launchThread
    static ActionRetCodeEnum staticMultiThreadFunction(unsigned int threadIndex, unsigned int threadMax, void *customArg);
    
};

class ImageMultiThreadProcessorBase : public MultiThreadProcessorBase
{
    RectI _renderWindow;

public:

    ImageMultiThreadProcessorBase(const EffectInstancePtr& effect);

    virtual ~ImageMultiThreadProcessorBase();

    /**
     * @brief Set the render window that will be used in process().
     * Derived class should also set other required data members before calling process()
     **/
    void setRenderWindow(const RectI& renderWindow);

    /**
     * @brief Launch the threads and render. This is a simple wrapper over launchThreads()
     * which set the appropriate number of threads given the render window
     **/
    virtual ActionRetCodeEnum process();

    /**
     * @brief Utility function to compute the subrange of a given thread
     **/
    static void getThreadRange(unsigned int threadID, unsigned int nThreads, int ibegin, int iend, int* ibegin_range, int* iend_range);

protected:

    /**
     * @brief The function that will be called by each thread concurrently and that should process the image.
     * @param renderWindow The rectangle of pixels to process.
     *
     * Note that this function should use the renderData parameter to check periodically if the render
     * has been aborted.
     **/
    virtual ActionRetCodeEnum multiThreadProcessImages(const RectI& renderWindow) = 0;

private:



    virtual ActionRetCodeEnum multiThreadFunction(unsigned int threadID, unsigned int nThreads) OVERRIDE FINAL;


};

NATRON_NAMESPACE_EXIT

#endif // NATRON_ENGINE_MULTITHREAD_H
