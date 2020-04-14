#ifndef _ofxMultiThread_h_
#define _ofxMultiThread_h_

/*
Software License :

Copyright (c) 2003-2009, The Open Effects Association Ltd. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.
    * Neither the name The Open Effects Association Ltd, nor the names of its 
      contributors may be used to endorse or promote products derived from this
      software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#include "ofxCore.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @file ofxMultiThread.h

    This file contains the Host Suite for threading 
*/

#define kOfxMultiThreadSuite "OfxMultiThreadSuite"

/** @brief Mutex blind data handle
 */
typedef struct OfxMutex *OfxMutexHandle;

/** @brief The function type to passed to the multi threading routines 

    \arg \e threadIndex unique index of this thread, will be between 0 and threadMax
    \arg \e threadMax to total number of threads executing this function
    \arg \e customArg the argument passed into multiThread

A function of this type is passed to OfxMultiThreadSuiteV1::multiThread to be launched in multiple threads.
 */
typedef void (OfxThreadFunctionV1)(unsigned int threadIndex,
				   unsigned int threadMax,
				   void *customArg);

/** @brief OFX suite that provides simple SMP style multi-processing
 */
typedef struct OfxMultiThreadSuiteV1 {
  /**@brief Function to spawn SMP threads

  \arg func The function to call in each thread.
  \arg nThreads The number of threads to launch
  \arg customArg The paramter to pass to customArg of func in each thread.

  This function will spawn nThreads separate threads of computation (typically one per CPU) 
  to allow something to perform symmetric multi processing. Each thread will call 'func' passing
  in the index of the thread and the number of threads actually launched.

  multiThread will not return until all the spawned threads have returned. It is up to the host
  how it waits for all the threads to return (busy wait, blocking, whatever).

  \e nThreads can be more than the value returned by multiThreadNumCPUs, however the threads will
  be limitted to the number of CPUs returned by multiThreadNumCPUs.

  This function cannot be called recursively.

  @returns
  - ::kOfxStatOK, the function func has executed and returned sucessfully
  - ::kOfxStatFailed, the threading function failed to launch
  - ::kOfxStatErrExists, failed in an attempt to call multiThread recursively,

  */
  OfxStatus (*multiThread)(OfxThreadFunctionV1 func,
			   unsigned int nThreads,
			   void *customArg);
			  
  /**@brief Function which indicates the number of CPUs available for SMP processing

  \arg nCPUs pointer to an integer where the result is returned
     
  This value may be less than the actual number of CPUs on a machine, as the host may reserve other CPUs for itself.

  @returns
  - ::kOfxStatOK, all was OK and the maximum number of threads is in nThreads.
  - ::kOfxStatFailed, the function failed to get the number of CPUs 
  */
  OfxStatus (*multiThreadNumCPUs)(unsigned int *nCPUs);

  /**@brief Function which indicates the index of the current thread

  \arg threadIndex  pointer to an integer where the result is returned

  This function returns the thread index, which is the same as the \e threadIndex argument passed to the ::OfxThreadFunctionV1.

  If there are no threads currently spawned, then this function will set threadIndex to 0

  @returns
  - ::kOfxStatOK, all was OK and the maximum number of threads is in nThreads.
  - ::kOfxStatFailed, the function failed to return an index
  */
  OfxStatus (*multiThreadIndex)(unsigned int *threadIndex);

  /**@brief Function to enquire if the calling thread was spawned by multiThread

  @returns
  - 0 if the thread is not one spawned by multiThread
  - 1 if the thread was spawned by multiThread
  */
  int (*multiThreadIsSpawnedThread)(void);

  /** @brief Create a mutex

  \arg mutex - where the new handle is returned
  \arg count - initial lock count on the mutex. This can be negative.

  Creates a new mutex with lockCount locks on the mutex intially set.    

  @returns
  - kOfxStatOK - mutex is now valid and ready to go
  */
  OfxStatus (*mutexCreate)(OfxMutexHandle *mutex, int lockCount);

  /** @brief Destroy a mutex
      
  Destroys a mutex intially created by mutexCreate.
  
  @returns
  - kOfxStatOK - if it destroyed the mutex
  - kOfxStatErrBadHandle - if the handle was bad
  */
  OfxStatus (*mutexDestroy)(const OfxMutexHandle mutex);

  /** @brief Blocking lock on the mutex

  This trys to lock a mutex and blocks the thread it is in until the lock suceeds. 

  A sucessful lock causes the mutex's lock count to be increased by one and to block any other calls to lock the mutex until it is unlocked.
  
  @returns
  - kOfxStatOK - if it got the lock
  - kOfxStatErrBadHandle - if the handle was bad
  */
  OfxStatus (*mutexLock)(const OfxMutexHandle mutex);

  /** @brief Unlock the mutex

  This  unlocks a mutex. Unlocking a mutex decreases its lock count by one.
  
  @returns
  - kOfxStatOK if it released the lock
  - kOfxStatErrBadHandle if the handle was bad
  */
  OfxStatus (*mutexUnLock)(const OfxMutexHandle mutex);

  /** @brief Non blocking attempt to lock the mutex

  This attempts to lock a mutex, if it cannot, it returns and says so, rather than blocking.

  A sucessful lock causes the mutex's lock count to be increased by one, if the lock did not suceed, the call returns immediately and the lock count remains unchanged.

  @returns
  - kOfxStatOK - if it got the lock
  - kOfxStatFailed - if it did not get the lock
  - kOfxStatErrBadHandle - if the handle was bad
  */
  OfxStatus (*mutexTryLock)(const OfxMutexHandle mutex);

 } OfxMultiThreadSuiteV1;

#ifdef __cplusplus
}
#endif


#endif
