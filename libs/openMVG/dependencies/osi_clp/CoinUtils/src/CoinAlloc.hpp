/* $Id: CoinAlloc.hpp 1438 2011-06-09 18:14:12Z stefan $ */
// Copyright (C) 2007, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef CoinAlloc_hpp
#define CoinAlloc_hpp

#include "CoinUtilsConfig.h"
#include <cstdlib>

#if !defined(COINUTILS_MEMPOOL_MAXPOOLED)
#  define COINUTILS_MEMPOOL_MAXPOOLED -1
#endif

#if (COINUTILS_MEMPOOL_MAXPOOLED >= 0)

#ifndef COINUTILS_MEMPOOL_ALIGNMENT
#define COINUTILS_MEMPOOL_ALIGNMENT 16
#endif

/* Note:
   This memory pool implementation assumes that sizeof(size_t) and
   sizeof(void*) are both <= COINUTILS_MEMPOOL_ALIGNMENT.
   Choosing an alignment of 4 will cause segfault on 64-bit platforms and may
   lead to bad performance on 32-bit platforms. So 8 is a mnimum recommended
   alignment. Probably 16 does not waste too much space either and may be even
   better for performance. One must play with it.
*/

//#############################################################################

#if (COINUTILS_MEMPOOL_ALIGNMENT == 16)
static const std::size_t CoinAllocPtrShift = 4;
static const std::size_t CoinAllocRoundMask = ~((std::size_t)15);
#elif (COINUTILS_MEMPOOL_ALIGNMENT == 8)
static const std::size_t CoinAllocPtrShift = 3;
static const std::size_t CoinAllocRoundMask = ~((std::size_t)7);
#else
#error "COINUTILS_MEMPOOL_ALIGNMENT must be defined as 8 or 16 (or this code needs to be changed :-)"
#endif

//#############################################################################

#ifndef COIN_MEMPOOL_SAVE_BLOCKHEADS
#  define COIN_MEMPOOL_SAVE_BLOCKHEADS 0
#endif

//#############################################################################

class CoinMempool 
{
private:
#if (COIN_MEMPOOL_SAVE_BLOCKHEADS == 1)
   char** block_heads;
   std::size_t block_num;
   std::size_t max_block_num;
#endif
#if defined(COINUTILS_PTHREADS) && (COINUTILS_PTHREAD == 1)
  pthread_mutex_t mutex_;
#endif
  int last_block_size_;
  char* first_free_;
  const std::size_t entry_size_;

private:
  CoinMempool(const CoinMempool&);
  CoinMempool& operator=(const CoinMempool&);

private:
  char* allocate_new_block();
  inline void lock_mutex() {
#if defined(COINUTILS_PTHREADS) && (COINUTILS_PTHREAD == 1)
    pthread_mutex_lock(&mutex_);
#endif
  }
  inline void unlock_mutex() {
#if defined(COINUTILS_PTHREADS) && (COINUTILS_PTHREAD == 1)
    pthread_mutex_unlock(&mutex_);
#endif
  }

public:
  CoinMempool(std::size_t size = 0);
  ~CoinMempool();

  char* alloc();
  inline void dealloc(char *p) 
  {
    char** pp = (char**)p;
    lock_mutex();
    *pp = first_free_;
    first_free_ = p;
    unlock_mutex();
  }
};

//#############################################################################

/** A memory pool allocator.

    If a request arrives for allocating \c n bytes then it is first
    rounded up to the nearest multiple of \c sizeof(void*) (this is \c
    n_roundup), then one more \c sizeof(void*) is added to this
    number. If the result is no more than maxpooled_ then
    the appropriate pool is used to get a chunk of memory, if not,
    then malloc is used. In either case, the size of the allocated
    chunk is written into the first \c sizeof(void*) bytes and a
    pointer pointing afterwards is returned.
*/

class CoinAlloc
{
private:
  CoinMempool* pool_;
  int maxpooled_;
public:
  CoinAlloc();
  ~CoinAlloc() {}

  inline void* alloc(const std::size_t n)
  {
    if (maxpooled_ <= 0) {
      return std::malloc(n);
    }
    char *p = NULL;
    const std::size_t to_alloc =
      ((n+COINUTILS_MEMPOOL_ALIGNMENT-1) & CoinAllocRoundMask) +
      COINUTILS_MEMPOOL_ALIGNMENT;
    CoinMempool* pool = NULL;
    if (maxpooled_ > 0 && to_alloc >= (size_t)maxpooled_) {
      p = static_cast<char*>(std::malloc(to_alloc));
      if (p == NULL) throw std::bad_alloc();
    } else {
      pool = pool_ + (to_alloc >> CoinAllocPtrShift);
      p = pool->alloc();
    }
    *((CoinMempool**)p) = pool;
    return static_cast<void*>(p+COINUTILS_MEMPOOL_ALIGNMENT);
  }

  inline void dealloc(void* p)
  {
    if (maxpooled_ <= 0) {
      std::free(p);
      return;
    }
    if (p) {
      char* base = static_cast<char*>(p)-COINUTILS_MEMPOOL_ALIGNMENT;
      CoinMempool* pool = *((CoinMempool**)base);
      if (!pool) {
	std::free(base);
      } else {
	pool->dealloc(base);
      }
    }
  }
};

extern CoinAlloc CoinAllocator;

//#############################################################################

#if defined(COINUTILS_MEMPOOL_OVERRIDE_NEW) && (COINUTILS_MEMPOOL_OVERRIDE_NEW == 1)
void* operator new(std::size_t size) throw (std::bad_alloc);
void* operator new[](std::size_t) throw (std::bad_alloc);
void operator delete(void*) throw();
void operator delete[](void*) throw();
void* operator new(std::size_t, const std::nothrow_t&) throw();
void* operator new[](std::size_t, const std::nothrow_t&) throw();
void operator delete(void*, const std::nothrow_t&) throw();
void operator delete[](void*, const std::nothrow_t&) throw();
#endif

#endif /*(COINUTILS_MEMPOOL_MAXPOOLED >= 0)*/
#endif
