#ifndef _ofxMemory_h_
#define _ofxMemory_h_

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


#ifdef __cplusplus
extern "C" {
#endif

#define kOfxMemorySuite "OfxMemorySuite"

/** @brief The OFX suite that implements general purpose memory management.

Use this suite for ordinary memory management functions, where you would normally use malloc/free or new/delete on ordinary objects.

For images, you should use the memory allocation functions in the image effect suite, as many hosts have specific image memory pools.

\note C++ plugin developers will need to redefine new and delete as skins ontop of this suite.
 */
typedef struct OfxMemorySuiteV1 {
  /** @brief Allocate memory.
      
  \arg handle	- effect instance to assosciate with this memory allocation, or NULL.
  \arg nBytes        - the number of bytes to allocate
  \arg allocatedData - a pointer to the return value. Allocated memory will be alligned for any use.

  This function has the host allocate memory using its own memory resources
  and returns that to the plugin.

  @returns
  - ::kOfxStatOK the memory was sucessfully allocated
  - ::kOfxStatErrMemory the request could not be met and no memory was allocated

  */   
  OfxStatus (*memoryAlloc)(void *handle, 
			   size_t nBytes,
			   void **allocatedData);
	
  /** @brief Frees memory.
      
  \arg allocatedData - pointer to memory previously returned by OfxMemorySuiteV1::memoryAlloc

  This function frees any memory that was previously allocated via OfxMemorySuiteV1::memoryAlloc.

  @returns
  - ::kOfxStatOK the memory was sucessfully freed
  - ::kOfxStatErrBadHandle \e allocatedData was not a valid pointer returned by OfxMemorySuiteV1::memoryAlloc

  */   
  OfxStatus (*memoryFree)(void *allocatedData);
 } OfxMemorySuiteV1;


/** @file ofxMemory.h
    This file contains the API for general purpose memory allocation from a host.
*/



#ifdef __cplusplus
}
#endif

#endif
