/* $Id: CoinAlloc.cpp 1373 2011-01-03 23:57:44Z lou $ */
// Copyright (C) 2007, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include <cassert>
#include <cstdlib>
#include <new>
#include "CoinAlloc.hpp"

#if (COINUTILS_MEMPOOL_MAXPOOLED >= 0)

//=============================================================================

CoinMempool::CoinMempool(size_t entry) :
#if (COIN_MEMPOOL_SAVE_BLOCKHEADS==1)
  block_heads_(NULL),
  block_num_(0),
  max_block_num_(0),
#endif
  last_block_size_(0),
  first_free_(NULL),
  entry_size_(entry)
{
#if defined(COINUTILS_PTHREADS) && (COINUTILS_PTHREAD == 1)
  pthread_mutex_init(&mutex_, NULL);
#endif
  assert((entry_size_/COINUTILS_MEMPOOL_ALIGNMENT)*COINUTILS_MEMPOOL_ALIGNMENT
	 == entry_size_);
}

//=============================================================================

CoinMempool::~CoinMempool()
{
#if (COIN_MEMPOOL_SAVE_BLOCKHEADS==1)
  for (size_t i = 0; i < block_num_; ++i) {
    free(block_heads_[i]);
  }
#endif
#if defined(COINUTILS_PTHREADS) && (COINUTILS_PTHREAD == 1)
  pthread_mutex_destroy(&mutex_);
#endif
}

//==============================================================================

char* 
CoinMempool::alloc()
{
  lock_mutex();
  if (first_free_ == NULL) {
    unlock_mutex();
    char* block = allocate_new_block();
    lock_mutex();
#if (COIN_MEMPOOL_SAVE_BLOCKHEADS==1)
    // see if we can record another block head. If not, then resize
    // block_heads
    if (max_block_num_ == block_num_) {
      max_block_num_ = 2 * block_num_ + 10;
      char** old_block_heads = block_heads_;
      block_heads_ = (char**)malloc(max_block_num_ * sizeof(char*));
      CoinMemcpyN( old_block_heads,block_num_,block_heads_);
      free(old_block_heads);
    }
    // save the new block
    block_heads_[block_num_++] = block;
#endif
    // link in the new block
    *(char**)(block+((last_block_size_-1)*entry_size_)) = first_free_;
    first_free_ = block;
  }
  char* p = first_free_;
  first_free_ = *(char**)p;
  unlock_mutex();
  return p;
}

//=============================================================================

char*
CoinMempool::allocate_new_block()
{
  last_block_size_ = static_cast<int>(1.5 * last_block_size_ + 32);
  char* block = static_cast<char*>(std::malloc(last_block_size_*entry_size_));
  // link the entries in the new block together
  for (int i = last_block_size_-2; i >= 0; --i) {
    *(char**)(block+(i*entry_size_)) = block+((i+1)*entry_size_);
  }
  // terminate the linked list with a null pointer
  *(char**)(block+((last_block_size_-1)*entry_size_)) = NULL;
  return block;
}

//#############################################################################

CoinAlloc CoinAllocator;

CoinAlloc::CoinAlloc() :
  pool_(NULL),
  maxpooled_(COINUTILS_MEMPOOL_MAXPOOLED)
{
  const char* maxpooled = std::getenv("COINUTILS_MEMPOOL_MAXPOOLED");
  if (maxpooled) {
    maxpooled_ = std::atoi(maxpooled);
  }
  const size_t poolnum = maxpooled_ / COINUTILS_MEMPOOL_ALIGNMENT;
  maxpooled_ = poolnum * COINUTILS_MEMPOOL_ALIGNMENT;
  if (maxpooled_ > 0) {
    pool_ = (CoinMempool*)malloc(sizeof(CoinMempool)*poolnum);
    for (int i = poolnum-1; i >= 0; --i) {
      new (&pool_[i]) CoinMempool(i*COINUTILS_MEMPOOL_ALIGNMENT);
    }
  }
}

//#############################################################################

#if defined(COINUTILS_MEMPOOL_OVERRIDE_NEW) && (COINUTILS_MEMPOOL_OVERRIDE_NEW == 1)
void* operator new(std::size_t sz) throw (std::bad_alloc)
{ 
  return CoinAllocator.alloc(sz); 
}

void* operator new[](std::size_t sz) throw (std::bad_alloc)
{ 
  return CoinAllocator.alloc(sz); 
}

void operator delete(void* p) throw()
{ 
  CoinAllocator.dealloc(p); 
}
  
void operator delete[](void* p) throw()
{ 
  CoinAllocator.dealloc(p); 
}
  
void* operator new(std::size_t sz, const std::nothrow_t&) throw()
{
  void *p = NULL;
  try {
    p = CoinAllocator.alloc(sz);
  }
  catch (std::bad_alloc &ba) {
    return NULL;
  }
  return p;
}

void* operator new[](std::size_t sz, const std::nothrow_t&) throw()
{
  void *p = NULL;
  try {
    p = CoinAllocator.alloc(sz);
  }
  catch (std::bad_alloc &ba) {
    return NULL;
  }
  return p;
}

void operator delete(void* p, const std::nothrow_t&) throw()
{
  CoinAllocator.dealloc(p); 
}  

void operator delete[](void* p, const std::nothrow_t&) throw()
{
  CoinAllocator.dealloc(p); 
}  

#endif

#endif /*(COINUTILS_MEMPOOL_MAXPOOLED >= 0)*/
