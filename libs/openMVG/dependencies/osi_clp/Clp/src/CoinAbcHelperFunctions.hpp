/* $Id$ */
// Copyright (C) 2003, International Business Machines
// Corporation and others, Copyright (C) 2012, FasterCoin.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef CoinAbcHelperFunctions_H
#define CoinAbcHelperFunctions_H

#include "ClpConfig.h"
#ifdef HAVE_CMATH
# include <cmath>
#else
# ifdef HAVE_MATH_H
#  include <math.h>
# else
# include <cmath>
# endif
#endif
#include "CoinAbcCommon.hpp"
#ifndef abc_assert
#define abc_assert(condition)							\
  { if (!condition) {printf("abc_assert in %s at line %d - %s is false\n", \
			    __FILE__, __LINE__, __STRING(condition)); abort();} }
#endif
// cilk_for granularity.
#define CILK_FOR_GRAINSIZE 128
//#define AVX2 2
#if AVX2==1
#include "emmintrin.h"
#elif AVX2==2
#include <immintrin.h>
#elif AVX2==3
#include "avx2intrin.h"
#endif
//#define __AVX__ 1
//#define __AVX2__ 1
/**
    Note (JJF) I have added some operations on arrays even though they may
    duplicate CoinDenseVector.

*/

#define UNROLL_SCATTER 2
#define INLINE_SCATTER 1
#if INLINE_SCATTER==0
void CoinAbcScatterUpdate(int number,CoinFactorizationDouble pivotValue,
			  const CoinFactorizationDouble *  COIN_RESTRICT thisElement,
			  const int *  COIN_RESTRICT thisIndex,
			  CoinFactorizationDouble * COIN_RESTRICT region);
#else
void ABC_INLINE inline CoinAbcScatterUpdate(int number,CoinFactorizationDouble pivotValue,
					    const CoinFactorizationDouble *  COIN_RESTRICT thisElement,
					    const int *  COIN_RESTRICT thisIndex,
					    CoinFactorizationDouble * COIN_RESTRICT region)
{
#if UNROLL_SCATTER==0
  for (CoinBigIndex j=number-1 ; j >=0; j-- ) {
    CoinSimplexInt iRow = thisIndex[j];
    CoinFactorizationDouble regionValue = region[iRow];
    CoinFactorizationDouble value = thisElement[j];
    assert (value);
    region[iRow] = regionValue - value * pivotValue;
  }
#elif UNROLL_SCATTER==1
  if ((number&1)!=0) {
    number--;
    CoinSimplexInt iRow = thisIndex[number];
    CoinFactorizationDouble regionValue = region[iRow];
    CoinFactorizationDouble value = thisElement[number];
    region[iRow] = regionValue - value * pivotValue;
  }
  for (CoinBigIndex j=number-1 ; j >=0; j-=2 ) {
    CoinSimplexInt iRow0 = thisIndex[j];
    CoinSimplexInt iRow1 = thisIndex[j-1];
    CoinFactorizationDouble regionValue0 = region[iRow0];
    CoinFactorizationDouble regionValue1 = region[iRow1];
    region[iRow0] = regionValue0 - thisElement[j] * pivotValue;
    region[iRow1] = regionValue1 - thisElement[j-1] * pivotValue;
  }
#elif UNROLL_SCATTER==2
  if ((number&1)!=0) {
    number--;
    CoinSimplexInt iRow = thisIndex[number];
    CoinFactorizationDouble regionValue = region[iRow];
    CoinFactorizationDouble value = thisElement[number]; 
    region[iRow] = regionValue - value * pivotValue;
  }
  if ((number&2)!=0) {
    CoinSimplexInt iRow0 = thisIndex[number-1];
    CoinFactorizationDouble regionValue0 = region[iRow0];
    CoinFactorizationDouble value0 = thisElement[number-1]; 
    CoinSimplexInt iRow1 = thisIndex[number-2];
    CoinFactorizationDouble regionValue1 = region[iRow1];
    CoinFactorizationDouble value1 = thisElement[number-2]; 
    region[iRow0] = regionValue0 - value0 * pivotValue;
    region[iRow1] = regionValue1 - value1 * pivotValue;
    number-=2;
  }
#pragma cilk_grainsize=CILK_FOR_GRAINSIZE
  for (CoinBigIndex j=number-1 ; j >=0; j-=4 ) {
    CoinSimplexInt iRow0 = thisIndex[j];
    CoinSimplexInt iRow1 = thisIndex[j-1];
    CoinFactorizationDouble regionValue0 = region[iRow0];
    CoinFactorizationDouble regionValue1 = region[iRow1];
    region[iRow0] = regionValue0 - thisElement[j] * pivotValue;
    region[iRow1] = regionValue1 - thisElement[j-1] * pivotValue;
    CoinSimplexInt iRow2 = thisIndex[j-2];
    CoinSimplexInt iRow3 = thisIndex[j-3];
    CoinFactorizationDouble regionValue2 = region[iRow2];
    CoinFactorizationDouble regionValue3 = region[iRow3];
    region[iRow2] = regionValue2 - thisElement[j-2] * pivotValue;
    region[iRow3] = regionValue3 - thisElement[j-3] * pivotValue;
  }
#elif UNROLL_SCATTER==3
  CoinSimplexInt iRow0;
  CoinSimplexInt iRow1;
  CoinFactorizationDouble regionValue0;
  CoinFactorizationDouble regionValue1;
  switch(static_cast<unsigned int>(number)) {
  case 0:
    break;
  case 1:
    iRow0 = thisIndex[0];
    regionValue0 = region[iRow0];
    region[iRow0] = regionValue0 - thisElement[0] * pivotValue;
    break;
  case 2:
    iRow0 = thisIndex[0];
    iRow1 = thisIndex[1];
    regionValue0 = region[iRow0];
    regionValue1 = region[iRow1];
    region[iRow0] = regionValue0 - thisElement[0] * pivotValue;
    region[iRow1] = regionValue1 - thisElement[1] * pivotValue;
    break;
  case 3:
    iRow0 = thisIndex[0];
    iRow1 = thisIndex[1];
    regionValue0 = region[iRow0];
    regionValue1 = region[iRow1];
    region[iRow0] = regionValue0 - thisElement[0] * pivotValue;
    region[iRow1] = regionValue1 - thisElement[1] * pivotValue;
    iRow0 = thisIndex[2];
    regionValue0 = region[iRow0];
    region[iRow0] = regionValue0 - thisElement[2] * pivotValue;
    break;
  case 4:
    iRow0 = thisIndex[0];
    iRow1 = thisIndex[1];
    regionValue0 = region[iRow0];
    regionValue1 = region[iRow1];
    region[iRow0] = regionValue0 - thisElement[0] * pivotValue;
    region[iRow1] = regionValue1 - thisElement[1] * pivotValue;
    iRow0 = thisIndex[2];
    iRow1 = thisIndex[3];
    regionValue0 = region[iRow0];
    regionValue1 = region[iRow1];
    region[iRow0] = regionValue0 - thisElement[2] * pivotValue;
    region[iRow1] = regionValue1 - thisElement[3] * pivotValue;
    break;
  case 5:
    iRow0 = thisIndex[0];
    iRow1 = thisIndex[1];
    regionValue0 = region[iRow0];
    regionValue1 = region[iRow1];
    region[iRow0] = regionValue0 - thisElement[0] * pivotValue;
    region[iRow1] = regionValue1 - thisElement[1] * pivotValue;
    iRow0 = thisIndex[2];
    iRow1 = thisIndex[3];
    regionValue0 = region[iRow0];
    regionValue1 = region[iRow1];
    region[iRow0] = regionValue0 - thisElement[2] * pivotValue;
    region[iRow1] = regionValue1 - thisElement[3] * pivotValue;
    iRow0 = thisIndex[4];
    regionValue0 = region[iRow0];
    region[iRow0] = regionValue0 - thisElement[4] * pivotValue;
    break;
  case 6:
    iRow0 = thisIndex[0];
    iRow1 = thisIndex[1];
    regionValue0 = region[iRow0];
    regionValue1 = region[iRow1];
    region[iRow0] = regionValue0 - thisElement[0] * pivotValue;
    region[iRow1] = regionValue1 - thisElement[1] * pivotValue;
    iRow0 = thisIndex[2];
    iRow1 = thisIndex[3];
    regionValue0 = region[iRow0];
    regionValue1 = region[iRow1];
    region[iRow0] = regionValue0 - thisElement[2] * pivotValue;
    region[iRow1] = regionValue1 - thisElement[3] * pivotValue;
    iRow0 = thisIndex[4];
    iRow1 = thisIndex[5];
    regionValue0 = region[iRow0];
    regionValue1 = region[iRow1];
    region[iRow0] = regionValue0 - thisElement[4] * pivotValue;
    region[iRow1] = regionValue1 - thisElement[5] * pivotValue;
    break;
  case 7:
    iRow0 = thisIndex[0];
    iRow1 = thisIndex[1];
    regionValue0 = region[iRow0];
    regionValue1 = region[iRow1];
    region[iRow0] = regionValue0 - thisElement[0] * pivotValue;
    region[iRow1] = regionValue1 - thisElement[1] * pivotValue;
    iRow0 = thisIndex[2];
    iRow1 = thisIndex[3];
    regionValue0 = region[iRow0];
    regionValue1 = region[iRow1];
    region[iRow0] = regionValue0 - thisElement[2] * pivotValue;
    region[iRow1] = regionValue1 - thisElement[3] * pivotValue;
    iRow0 = thisIndex[4];
    iRow1 = thisIndex[5];
    regionValue0 = region[iRow0];
    regionValue1 = region[iRow1];
    region[iRow0] = regionValue0 - thisElement[4] * pivotValue;
    region[iRow1] = regionValue1 - thisElement[5] * pivotValue;
    iRow0 = thisIndex[6];
    regionValue0 = region[iRow0];
    region[iRow0] = regionValue0 - thisElement[6] * pivotValue;
    break;
  case 8:
    iRow0 = thisIndex[0];
    iRow1 = thisIndex[1];
    regionValue0 = region[iRow0];
    regionValue1 = region[iRow1];
    region[iRow0] = regionValue0 - thisElement[0] * pivotValue;
    region[iRow1] = regionValue1 - thisElement[1] * pivotValue;
    iRow0 = thisIndex[2];
    iRow1 = thisIndex[3];
    regionValue0 = region[iRow0];
    regionValue1 = region[iRow1];
    region[iRow0] = regionValue0 - thisElement[2] * pivotValue;
    region[iRow1] = regionValue1 - thisElement[3] * pivotValue;
    iRow0 = thisIndex[4];
    iRow1 = thisIndex[5];
    regionValue0 = region[iRow0];
    regionValue1 = region[iRow1];
    region[iRow0] = regionValue0 - thisElement[4] * pivotValue;
    region[iRow1] = regionValue1 - thisElement[5] * pivotValue;
    iRow0 = thisIndex[6];
    iRow1 = thisIndex[7];
    regionValue0 = region[iRow0];
    regionValue1 = region[iRow1];
    region[iRow0] = regionValue0 - thisElement[6] * pivotValue;
    region[iRow1] = regionValue1 - thisElement[7] * pivotValue;
    break;
  default:
    if ((number&1)!=0) {
      number--;
      CoinSimplexInt iRow = thisIndex[number];
      CoinFactorizationDouble regionValue = region[iRow];
      CoinFactorizationDouble value = thisElement[number];
      region[iRow] = regionValue - value * pivotValue;
    }
    for (CoinBigIndex j=number-1 ; j >=0; j-=2 ) {
      CoinSimplexInt iRow0 = thisIndex[j];
      CoinSimplexInt iRow1 = thisIndex[j-1];
      CoinFactorizationDouble regionValue0 = region[iRow0];
      CoinFactorizationDouble regionValue1 = region[iRow1];
      region[iRow0] = regionValue0 - thisElement[j] * pivotValue;
      region[iRow1] = regionValue1 - thisElement[j-1] * pivotValue;
    }
    break;
  }
#endif
}
void ABC_INLINE inline CoinAbcScatterUpdate(int number,CoinFactorizationDouble pivotValue,
					    const CoinFactorizationDouble *  COIN_RESTRICT thisElement,
					    CoinFactorizationDouble * COIN_RESTRICT region)
{
#if UNROLL_SCATTER==0
  const int * COIN_RESTRICT thisIndex = reinterpret_cast<const int *>(thisElement+number);
  for (CoinBigIndex j=number-1 ; j >=0; j-- ) {
    CoinSimplexInt iRow = thisIndex[j];
    CoinFactorizationDouble regionValue = region[iRow];
    CoinFactorizationDouble value = thisElement[j];
    assert (value);
    region[iRow] = regionValue - value * pivotValue;
  }
#elif UNROLL_SCATTER==1
  const int * COIN_RESTRICT thisIndex = reinterpret_cast<const int *>(thisElement+number);
  if ((number&1)!=0) {
    number--;
    CoinSimplexInt iRow = thisIndex[number];
    CoinFactorizationDouble regionValue = region[iRow];
    CoinFactorizationDouble value = thisElement[number];
    region[iRow] = regionValue - value * pivotValue;
  }
  for (CoinBigIndex j=number-1 ; j >=0; j-=2 ) {
    CoinSimplexInt iRow0 = thisIndex[j];
    CoinSimplexInt iRow1 = thisIndex[j-1];
    CoinFactorizationDouble regionValue0 = region[iRow0];
    CoinFactorizationDouble regionValue1 = region[iRow1];
    region[iRow0] = regionValue0 - thisElement[j] * pivotValue;
    region[iRow1] = regionValue1 - thisElement[j-1] * pivotValue;
  }
#elif UNROLL_SCATTER==2
  const int * COIN_RESTRICT thisIndex = reinterpret_cast<const int *>(thisElement+number);
  if ((number&1)!=0) {
    number--;
    CoinSimplexInt iRow = thisIndex[number];
    CoinFactorizationDouble regionValue = region[iRow];
    CoinFactorizationDouble value = thisElement[number]; 
    region[iRow] = regionValue - value * pivotValue;
  }
  if ((number&2)!=0) {
    CoinSimplexInt iRow0 = thisIndex[number-1];
    CoinFactorizationDouble regionValue0 = region[iRow0];
    CoinFactorizationDouble value0 = thisElement[number-1]; 
    CoinSimplexInt iRow1 = thisIndex[number-2];
    CoinFactorizationDouble regionValue1 = region[iRow1];
    CoinFactorizationDouble value1 = thisElement[number-2]; 
    region[iRow0] = regionValue0 - value0 * pivotValue;
    region[iRow1] = regionValue1 - value1 * pivotValue;
    number-=2;
  }
#if AVX2==22
  CoinFactorizationDouble temp[4] __attribute__ ((aligned (32)));
  __m256d pv = _mm256_broadcast_sd(&pivotValue);
  for (CoinBigIndex j=number-1 ; j >=0; j-=4 ) {
    __m256d elements=_mm256_loadu_pd(thisElement+j-3);
    CoinSimplexInt iRow0 = thisIndex[j-3];
    CoinSimplexInt iRow1 = thisIndex[j-2];
    CoinSimplexInt iRow2 = thisIndex[j-1];
    CoinSimplexInt iRow3 = thisIndex[j-0];
    temp[0] = region[iRow0];
    temp[1] = region[iRow1];
    temp[2] = region[iRow2];
    temp[3] = region[iRow3];
    __m256d t0=_mm256_load_pd(temp);
    t0 -= pv*elements;
    _mm256_store_pd (temp, t0);
    region[iRow0] = temp[0];
    region[iRow1] = temp[1];
    region[iRow2] = temp[2];
    region[iRow3] = temp[3];
  }
#else
#pragma cilk_grainsize=CILK_FOR_GRAINSIZE
  for (CoinBigIndex j=number-1 ; j >=0; j-=4 ) {
    CoinSimplexInt iRow0 = thisIndex[j];
    CoinSimplexInt iRow1 = thisIndex[j-1];
    CoinFactorizationDouble regionValue0 = region[iRow0];
    CoinFactorizationDouble regionValue1 = region[iRow1];
    region[iRow0] = regionValue0 - thisElement[j] * pivotValue;
    region[iRow1] = regionValue1 - thisElement[j-1] * pivotValue;
    CoinSimplexInt iRow2 = thisIndex[j-2];
    CoinSimplexInt iRow3 = thisIndex[j-3];
    CoinFactorizationDouble regionValue2 = region[iRow2];
    CoinFactorizationDouble regionValue3 = region[iRow3];
    region[iRow2] = regionValue2 - thisElement[j-2] * pivotValue;
    region[iRow3] = regionValue3 - thisElement[j-3] * pivotValue;
  }
#endif
#elif UNROLL_SCATTER==3
  const int * COIN_RESTRICT thisIndex = reinterpret_cast<const int *>(thisElement+number);
  CoinSimplexInt iRow0;
  CoinSimplexInt iRow1;
  CoinFactorizationDouble regionValue0;
  CoinFactorizationDouble regionValue1;
  switch(static_cast<unsigned int>(number)) {
  case 0:
    break;
  case 1:
    iRow0 = thisIndex[0];
    regionValue0 = region[iRow0];
    region[iRow0] = regionValue0 - thisElement[0] * pivotValue;
    break;
  case 2:
    iRow0 = thisIndex[0];
    iRow1 = thisIndex[1];
    regionValue0 = region[iRow0];
    regionValue1 = region[iRow1];
    region[iRow0] = regionValue0 - thisElement[0] * pivotValue;
    region[iRow1] = regionValue1 - thisElement[1] * pivotValue;
    break;
  case 3:
    iRow0 = thisIndex[0];
    iRow1 = thisIndex[1];
    regionValue0 = region[iRow0];
    regionValue1 = region[iRow1];
    region[iRow0] = regionValue0 - thisElement[0] * pivotValue;
    region[iRow1] = regionValue1 - thisElement[1] * pivotValue;
    iRow0 = thisIndex[2];
    regionValue0 = region[iRow0];
    region[iRow0] = regionValue0 - thisElement[2] * pivotValue;
    break;
  case 4:
    iRow0 = thisIndex[0];
    iRow1 = thisIndex[1];
    regionValue0 = region[iRow0];
    regionValue1 = region[iRow1];
    region[iRow0] = regionValue0 - thisElement[0] * pivotValue;
    region[iRow1] = regionValue1 - thisElement[1] * pivotValue;
    iRow0 = thisIndex[2];
    iRow1 = thisIndex[3];
    regionValue0 = region[iRow0];
    regionValue1 = region[iRow1];
    region[iRow0] = regionValue0 - thisElement[2] * pivotValue;
    region[iRow1] = regionValue1 - thisElement[3] * pivotValue;
    break;
  case 5:
    iRow0 = thisIndex[0];
    iRow1 = thisIndex[1];
    regionValue0 = region[iRow0];
    regionValue1 = region[iRow1];
    region[iRow0] = regionValue0 - thisElement[0] * pivotValue;
    region[iRow1] = regionValue1 - thisElement[1] * pivotValue;
    iRow0 = thisIndex[2];
    iRow1 = thisIndex[3];
    regionValue0 = region[iRow0];
    regionValue1 = region[iRow1];
    region[iRow0] = regionValue0 - thisElement[2] * pivotValue;
    region[iRow1] = regionValue1 - thisElement[3] * pivotValue;
    iRow0 = thisIndex[4];
    regionValue0 = region[iRow0];
    region[iRow0] = regionValue0 - thisElement[4] * pivotValue;
    break;
  case 6:
    iRow0 = thisIndex[0];
    iRow1 = thisIndex[1];
    regionValue0 = region[iRow0];
    regionValue1 = region[iRow1];
    region[iRow0] = regionValue0 - thisElement[0] * pivotValue;
    region[iRow1] = regionValue1 - thisElement[1] * pivotValue;
    iRow0 = thisIndex[2];
    iRow1 = thisIndex[3];
    regionValue0 = region[iRow0];
    regionValue1 = region[iRow1];
    region[iRow0] = regionValue0 - thisElement[2] * pivotValue;
    region[iRow1] = regionValue1 - thisElement[3] * pivotValue;
    iRow0 = thisIndex[4];
    iRow1 = thisIndex[5];
    regionValue0 = region[iRow0];
    regionValue1 = region[iRow1];
    region[iRow0] = regionValue0 - thisElement[4] * pivotValue;
    region[iRow1] = regionValue1 - thisElement[5] * pivotValue;
    break;
  case 7:
    iRow0 = thisIndex[0];
    iRow1 = thisIndex[1];
    regionValue0 = region[iRow0];
    regionValue1 = region[iRow1];
    region[iRow0] = regionValue0 - thisElement[0] * pivotValue;
    region[iRow1] = regionValue1 - thisElement[1] * pivotValue;
    iRow0 = thisIndex[2];
    iRow1 = thisIndex[3];
    regionValue0 = region[iRow0];
    regionValue1 = region[iRow1];
    region[iRow0] = regionValue0 - thisElement[2] * pivotValue;
    region[iRow1] = regionValue1 - thisElement[3] * pivotValue;
    iRow0 = thisIndex[4];
    iRow1 = thisIndex[5];
    regionValue0 = region[iRow0];
    regionValue1 = region[iRow1];
    region[iRow0] = regionValue0 - thisElement[4] * pivotValue;
    region[iRow1] = regionValue1 - thisElement[5] * pivotValue;
    iRow0 = thisIndex[6];
    regionValue0 = region[iRow0];
    region[iRow0] = regionValue0 - thisElement[6] * pivotValue;
    break;
  case 8:
    iRow0 = thisIndex[0];
    iRow1 = thisIndex[1];
    regionValue0 = region[iRow0];
    regionValue1 = region[iRow1];
    region[iRow0] = regionValue0 - thisElement[0] * pivotValue;
    region[iRow1] = regionValue1 - thisElement[1] * pivotValue;
    iRow0 = thisIndex[2];
    iRow1 = thisIndex[3];
    regionValue0 = region[iRow0];
    regionValue1 = region[iRow1];
    region[iRow0] = regionValue0 - thisElement[2] * pivotValue;
    region[iRow1] = regionValue1 - thisElement[3] * pivotValue;
    iRow0 = thisIndex[4];
    iRow1 = thisIndex[5];
    regionValue0 = region[iRow0];
    regionValue1 = region[iRow1];
    region[iRow0] = regionValue0 - thisElement[4] * pivotValue;
    region[iRow1] = regionValue1 - thisElement[5] * pivotValue;
    iRow0 = thisIndex[6];
    iRow1 = thisIndex[7];
    regionValue0 = region[iRow0];
    regionValue1 = region[iRow1];
    region[iRow0] = regionValue0 - thisElement[6] * pivotValue;
    region[iRow1] = regionValue1 - thisElement[7] * pivotValue;
    break;
  default:
    if ((number&1)!=0) {
      number--;
      CoinSimplexInt iRow = thisIndex[number];
      CoinFactorizationDouble regionValue = region[iRow];
      CoinFactorizationDouble value = thisElement[number];
      region[iRow] = regionValue - value * pivotValue;
    }
    for (CoinBigIndex j=number-1 ; j >=0; j-=2 ) {
      CoinSimplexInt iRow0 = thisIndex[j];
      CoinSimplexInt iRow1 = thisIndex[j-1];
      CoinFactorizationDouble regionValue0 = region[iRow0];
      CoinFactorizationDouble regionValue1 = region[iRow1];
      region[iRow0] = regionValue0 - thisElement[j] * pivotValue;
      region[iRow1] = regionValue1 - thisElement[j-1] * pivotValue;
    }
    break;
  }
#endif
}
#endif
//#define COIN_PREFETCH
#ifdef COIN_PREFETCH
#if 1
#define coin_prefetch(mem)						\
  __asm__ __volatile__ ("prefetchnta %0" : : "m" (*(reinterpret_cast<char *>(mem))))
#define coin_prefetch_const(mem)					\
  __asm__ __volatile__ ("prefetchnta %0" : : "m" (*(reinterpret_cast<const char *>(mem))))
#else
#define coin_prefetch(mem)						\
  __asm__ __volatile__ ("prefetch %0" : : "m" (*(reinterpret_cast<char *>(mem))))
#define coin_prefetch_const(mem)					\
  __asm__ __volatile__ ("prefetch %0" : : "m" (*(reinterpret_cast<const char *>(mem))))
#endif
#else
// dummy
#define coin_prefetch(mem)
#define coin_prefetch_const(mem)
#endif
#define NEW_CHUNK_SIZE 4
#define NEW_CHUNK_SIZE_INCREMENT (NEW_CHUNK_SIZE+NEW_CHUNK_SIZE/2);
#define NEW_CHUNK_SIZE_OFFSET (NEW_CHUNK_SIZE/2)
// leaf, pure, nothrow and hot give warnings 
// fastcall and sseregparm give wrong results
//#define SCATTER_ATTRIBUTE __attribute__ ((leaf,fastcall,pure,sseregparm,nothrow,hot))
#define SCATTER_ATTRIBUTE 
typedef void (*scatterUpdate) (int,CoinFactorizationDouble,const CoinFactorizationDouble *, double *) SCATTER_ATTRIBUTE ;
typedef struct {
  scatterUpdate functionPointer;
  CoinBigIndex offset;
  int number;
} scatterStruct;
void CoinAbcScatterUpdate0(int numberIn, CoinFactorizationDouble multiplier,
			  const CoinFactorizationDouble *  COIN_RESTRICT thisElement,
			  CoinFactorizationDouble * COIN_RESTRICT region)  SCATTER_ATTRIBUTE ;
void CoinAbcScatterUpdate1(int numberIn, CoinFactorizationDouble multiplier,
			  const CoinFactorizationDouble *  COIN_RESTRICT thisElement,
			   CoinFactorizationDouble * COIN_RESTRICT region)  SCATTER_ATTRIBUTE ;
void CoinAbcScatterUpdate2(int numberIn, CoinFactorizationDouble multiplier,
			  const CoinFactorizationDouble *  COIN_RESTRICT thisElement,
			  CoinFactorizationDouble * COIN_RESTRICT region) SCATTER_ATTRIBUTE ;
void CoinAbcScatterUpdate3(int numberIn, CoinFactorizationDouble multiplier,
			  const CoinFactorizationDouble *  COIN_RESTRICT thisElement,
			  CoinFactorizationDouble * COIN_RESTRICT region) SCATTER_ATTRIBUTE ;
void CoinAbcScatterUpdate4(int numberIn, CoinFactorizationDouble multiplier,
			  const CoinFactorizationDouble *  COIN_RESTRICT thisElement,
			  CoinFactorizationDouble * COIN_RESTRICT region) SCATTER_ATTRIBUTE ;
void CoinAbcScatterUpdate5(int numberIn, CoinFactorizationDouble multiplier,
			  const CoinFactorizationDouble *  COIN_RESTRICT thisElement,
			  CoinFactorizationDouble * COIN_RESTRICT region) SCATTER_ATTRIBUTE ;
void CoinAbcScatterUpdate6(int numberIn, CoinFactorizationDouble multiplier,
			  const CoinFactorizationDouble *  COIN_RESTRICT thisElement,
			  CoinFactorizationDouble * COIN_RESTRICT region) SCATTER_ATTRIBUTE ;
void CoinAbcScatterUpdate7(int numberIn, CoinFactorizationDouble multiplier,
			  const CoinFactorizationDouble *  COIN_RESTRICT thisElement,
			  CoinFactorizationDouble * COIN_RESTRICT region) SCATTER_ATTRIBUTE ;
void CoinAbcScatterUpdate8(int numberIn, CoinFactorizationDouble multiplier,
			  const CoinFactorizationDouble *  COIN_RESTRICT thisElement,
			  CoinFactorizationDouble * COIN_RESTRICT region) SCATTER_ATTRIBUTE ;
void CoinAbcScatterUpdate4N(int numberIn, CoinFactorizationDouble multiplier,
			  const CoinFactorizationDouble *  COIN_RESTRICT thisElement,
			  CoinFactorizationDouble * COIN_RESTRICT region) SCATTER_ATTRIBUTE ;
void CoinAbcScatterUpdate4NPlus1(int numberIn, CoinFactorizationDouble multiplier,
			  const CoinFactorizationDouble *  COIN_RESTRICT thisElement,
			  CoinFactorizationDouble * COIN_RESTRICT region) SCATTER_ATTRIBUTE ;
void CoinAbcScatterUpdate4NPlus2(int numberIn, CoinFactorizationDouble multiplier,
			  const CoinFactorizationDouble *  COIN_RESTRICT thisElement,
			  CoinFactorizationDouble * COIN_RESTRICT region) SCATTER_ATTRIBUTE ;
void CoinAbcScatterUpdate4NPlus3(int numberIn, CoinFactorizationDouble multiplier,
			  const CoinFactorizationDouble *  COIN_RESTRICT thisElement,
			  CoinFactorizationDouble * COIN_RESTRICT region) SCATTER_ATTRIBUTE ;
void CoinAbcScatterUpdate1Subtract(int numberIn, CoinFactorizationDouble multiplier,
			  const CoinFactorizationDouble *  COIN_RESTRICT thisElement,
			  CoinFactorizationDouble * COIN_RESTRICT region) SCATTER_ATTRIBUTE ;
void CoinAbcScatterUpdate2Subtract(int numberIn, CoinFactorizationDouble multiplier,
			  const CoinFactorizationDouble *  COIN_RESTRICT thisElement,
			  CoinFactorizationDouble * COIN_RESTRICT region) SCATTER_ATTRIBUTE ;
void CoinAbcScatterUpdate3Subtract(int numberIn, CoinFactorizationDouble multiplier,
			  const CoinFactorizationDouble *  COIN_RESTRICT thisElement,
			  CoinFactorizationDouble * COIN_RESTRICT region) SCATTER_ATTRIBUTE ;
void CoinAbcScatterUpdate4Subtract(int numberIn, CoinFactorizationDouble multiplier,
			  const CoinFactorizationDouble *  COIN_RESTRICT thisElement,
			  CoinFactorizationDouble * COIN_RESTRICT region) SCATTER_ATTRIBUTE ;
void CoinAbcScatterUpdate5Subtract(int numberIn, CoinFactorizationDouble multiplier,
			  const CoinFactorizationDouble *  COIN_RESTRICT thisElement,
			  CoinFactorizationDouble * COIN_RESTRICT region) SCATTER_ATTRIBUTE ;
void CoinAbcScatterUpdate6Subtract(int numberIn, CoinFactorizationDouble multiplier,
			  const CoinFactorizationDouble *  COIN_RESTRICT thisElement,
			  CoinFactorizationDouble * COIN_RESTRICT region) SCATTER_ATTRIBUTE ;
void CoinAbcScatterUpdate7Subtract(int numberIn, CoinFactorizationDouble multiplier,
			  const CoinFactorizationDouble *  COIN_RESTRICT thisElement,
			  CoinFactorizationDouble * COIN_RESTRICT region) SCATTER_ATTRIBUTE ;
void CoinAbcScatterUpdate8Subtract(int numberIn, CoinFactorizationDouble multiplier,
			  const CoinFactorizationDouble *  COIN_RESTRICT thisElement,
			  CoinFactorizationDouble * COIN_RESTRICT region) SCATTER_ATTRIBUTE ;
void CoinAbcScatterUpdate4NSubtract(int numberIn, CoinFactorizationDouble multiplier,
			  const CoinFactorizationDouble *  COIN_RESTRICT thisElement,
			  CoinFactorizationDouble * COIN_RESTRICT region) SCATTER_ATTRIBUTE ;
void CoinAbcScatterUpdate4NPlus1Subtract(int numberIn, CoinFactorizationDouble multiplier,
			  const CoinFactorizationDouble *  COIN_RESTRICT thisElement,
			  CoinFactorizationDouble * COIN_RESTRICT region) SCATTER_ATTRIBUTE ;
void CoinAbcScatterUpdate4NPlus2Subtract(int numberIn, CoinFactorizationDouble multiplier,
			  const CoinFactorizationDouble *  COIN_RESTRICT thisElement,
			  CoinFactorizationDouble * COIN_RESTRICT region) SCATTER_ATTRIBUTE ;
void CoinAbcScatterUpdate4NPlus3Subtract(int numberIn, CoinFactorizationDouble multiplier,
			  const CoinFactorizationDouble *  COIN_RESTRICT thisElement,
			  CoinFactorizationDouble * COIN_RESTRICT region) SCATTER_ATTRIBUTE ;
void CoinAbcScatterUpdate1Add(int numberIn, CoinFactorizationDouble multiplier,
			  const CoinFactorizationDouble *  COIN_RESTRICT thisElement,
			  CoinFactorizationDouble * COIN_RESTRICT region) SCATTER_ATTRIBUTE ;
void CoinAbcScatterUpdate2Add(int numberIn, CoinFactorizationDouble multiplier,
			  const CoinFactorizationDouble *  COIN_RESTRICT thisElement,
			  CoinFactorizationDouble * COIN_RESTRICT region) SCATTER_ATTRIBUTE ;
void CoinAbcScatterUpdate3Add(int numberIn, CoinFactorizationDouble multiplier,
			  const CoinFactorizationDouble *  COIN_RESTRICT thisElement,
			  CoinFactorizationDouble * COIN_RESTRICT region) SCATTER_ATTRIBUTE ;
void CoinAbcScatterUpdate4Add(int numberIn, CoinFactorizationDouble multiplier,
			  const CoinFactorizationDouble *  COIN_RESTRICT thisElement,
			  CoinFactorizationDouble * COIN_RESTRICT region) SCATTER_ATTRIBUTE ;
void CoinAbcScatterUpdate5Add(int numberIn, CoinFactorizationDouble multiplier,
			  const CoinFactorizationDouble *  COIN_RESTRICT thisElement,
			  CoinFactorizationDouble * COIN_RESTRICT region) SCATTER_ATTRIBUTE ;
void CoinAbcScatterUpdate6Add(int numberIn, CoinFactorizationDouble multiplier,
			  const CoinFactorizationDouble *  COIN_RESTRICT thisElement,
			  CoinFactorizationDouble * COIN_RESTRICT region) SCATTER_ATTRIBUTE ;
void CoinAbcScatterUpdate7Add(int numberIn, CoinFactorizationDouble multiplier,
			  const CoinFactorizationDouble *  COIN_RESTRICT thisElement,
			  CoinFactorizationDouble * COIN_RESTRICT region) SCATTER_ATTRIBUTE ;
void CoinAbcScatterUpdate8Add(int numberIn, CoinFactorizationDouble multiplier,
			  const CoinFactorizationDouble *  COIN_RESTRICT thisElement,
			  CoinFactorizationDouble * COIN_RESTRICT region) SCATTER_ATTRIBUTE ;
void CoinAbcScatterUpdate4NAdd(int numberIn, CoinFactorizationDouble multiplier,
			  const CoinFactorizationDouble *  COIN_RESTRICT thisElement,
			  CoinFactorizationDouble * COIN_RESTRICT region) SCATTER_ATTRIBUTE ;
void CoinAbcScatterUpdate4NPlus1Add(int numberIn, CoinFactorizationDouble multiplier,
			  const CoinFactorizationDouble *  COIN_RESTRICT thisElement,
			  CoinFactorizationDouble * COIN_RESTRICT region) SCATTER_ATTRIBUTE ;
void CoinAbcScatterUpdate4NPlus2Add(int numberIn, CoinFactorizationDouble multiplier,
			  const CoinFactorizationDouble *  COIN_RESTRICT thisElement,
			  CoinFactorizationDouble * COIN_RESTRICT region) SCATTER_ATTRIBUTE ;
void CoinAbcScatterUpdate4NPlus3Add(int numberIn, CoinFactorizationDouble multiplier,
			  const CoinFactorizationDouble *  COIN_RESTRICT thisElement,
			  CoinFactorizationDouble * COIN_RESTRICT region) SCATTER_ATTRIBUTE ;
#if INLINE_SCATTER==0
void CoinAbcScatterUpdate(int number,CoinFactorizationDouble pivotValue,
			  const CoinFactorizationDouble *  COIN_RESTRICT thisElement,
			  const int *  COIN_RESTRICT thisIndex,
			  CoinFactorizationDouble * COIN_RESTRICT region,
			  double * COIN_RESTRICT work);
#else
#if 0
void ABC_INLINE inline CoinAbcScatterUpdate(int number,CoinFactorizationDouble pivotValue,
					    const CoinFactorizationDouble *  COIN_RESTRICT thisElement,
					    const int *  COIN_RESTRICT thisIndex,
					    CoinFactorizationDouble * COIN_RESTRICT region,
					    double * COIN_RESTRICT /*work*/)
{
#if UNROLL_SCATTER==0
  for (CoinBigIndex j=number-1 ; j >=0; j-- ) {
    CoinSimplexInt iRow = thisIndex[j];
    CoinFactorizationDouble regionValue = region[iRow];
    CoinFactorizationDouble value = thisElement[j];
    assert (value);
    region[iRow] = regionValue - value * pivotValue;
  }
#elif UNROLL_SCATTER==1
  if ((number&1)!=0) {
    CoinSimplexInt iRow = thisIndex[0];
    thisIndex++;
    CoinFactorizationDouble regionValue = region[iRow];
    CoinFactorizationDouble value = thisElement[0];
    thisElement++;
    region[iRow] = regionValue - value * pivotValue;
  }
  number = number>>1;
  CoinFactorizationDouble work2[4];
  for ( ; number !=0; number-- ) {
    CoinSimplexInt iRow0 = thisIndex[0];
    CoinSimplexInt iRow1 = thisIndex[1];
    work2[0] = region[iRow0];
    work2[1] = region[iRow1];
#if 0
    work2[2] = region[iRow0];
    work2[3] = region[iRow1];
    //__v4df b = __builtin_ia32_maskloadpd256(work2);
    __v4df b = __builtin_ia32_loadupd256(work2);
    //__v4df b = _mm256_load_pd(work2);
#endif
    work2[0] -= thisElement[0] * pivotValue;
    work2[1] -= thisElement[1] * pivotValue;
    region[iRow0] = work2[0];
    region[iRow1] = work2[1];
    thisIndex+=2;
    thisElement+=2;
  }
#endif
}
#endif
#endif
#define UNROLL_GATHER 0
#define INLINE_GATHER 1
#if INLINE_GATHER==0
CoinFactorizationDouble CoinAbcGatherUpdate(CoinSimplexInt number,
			  const CoinFactorizationDouble *  COIN_RESTRICT thisElement,
			  const int *  COIN_RESTRICT thisIndex,
			   CoinFactorizationDouble * COIN_RESTRICT region);
#else
CoinFactorizationDouble ABC_INLINE inline CoinAbcGatherUpdate(CoinSimplexInt number,
					     const CoinFactorizationDouble *  COIN_RESTRICT thisElement,
					     const int *  COIN_RESTRICT thisIndex,
					     CoinFactorizationDouble * COIN_RESTRICT region)
{
#if UNROLL_GATHER==0
  CoinFactorizationDouble pivotValue=0.0;
  for (CoinBigIndex j = 0; j < number; j ++ ) {
    CoinFactorizationDouble value = thisElement[j];
    CoinSimplexInt jRow = thisIndex[j];
    value *= region[jRow];
    pivotValue -= value;
  }
  return pivotValue;
#else
#error code
#endif
}
#endif
#define UNROLL_MULTIPLY_INDEXED 0
#define INLINE_MULTIPLY_INDEXED 0
#if INLINE_MULTIPLY_INDEXED==0
void CoinAbcMultiplyIndexed(int number,
			    const double *  COIN_RESTRICT multiplier,
			    const int *  COIN_RESTRICT thisIndex,
			    CoinFactorizationDouble * COIN_RESTRICT region);
void CoinAbcMultiplyIndexed(int number,
			    const long double *  COIN_RESTRICT multiplier,
			    const int *  COIN_RESTRICT thisIndex,
			    long double * COIN_RESTRICT region);
#else
void ABC_INLINE inline CoinAbcMultiplyIndexed(int number,
			    const double *  COIN_RESTRICT multiplier,
			    const int *  COIN_RESTRICT thisIndex,
			    CoinFactorizationDouble * COIN_RESTRICT region)
{
}
#endif
double CoinAbcMaximumAbsElement(const double * region, int size);
void CoinAbcMinMaxAbsElement(const double * region, int size,double & minimum , double & maximum);
void CoinAbcMinMaxAbsNormalValues(const double * region, int size,double & minimum , double & maximum);
void CoinAbcScale(double * region, double multiplier,int size);
void CoinAbcScaleNormalValues(double * region, double multiplier,double killIfLessThanThis,int size);
/// maximum fabs(region[i]) and then region[i]*=multiplier
double CoinAbcMaximumAbsElementAndScale(double * region, double multiplier,int size);
void CoinAbcSetElements(double * region, int size, double value);
void CoinAbcMultiplyAdd(const double * region1, int size, double multiplier1,
                 double * regionChanged, double multiplier2);
double CoinAbcInnerProduct(const double * region1, int size, const double * region2);
void CoinAbcGetNorms(const double * region, int size, double & norm1, double & norm2);
/// regionTo[index[i]]=regionFrom[i]
void CoinAbcScatterTo(const double * regionFrom, double * regionTo, const int * index,int number);
/// regionTo[i]=regionFrom[index[i]]
void CoinAbcGatherFrom(const double * regionFrom, double * regionTo, const int * index,int number);
/// regionTo[index[i]]=0.0
void CoinAbcScatterZeroTo(double * regionTo, const int * index,int number);
/// regionTo[indexScatter[indexList[i]]]=regionFrom[indexList[i]]
void CoinAbcScatterToList(const double * regionFrom, double * regionTo, 
		   const int * indexList, const int * indexScatter ,int number);
/// array[i]=1.0/sqrt(array[i])
void CoinAbcInverseSqrts(double * array, int n);
void CoinAbcReciprocal(double * array, int n, const double *input);
void CoinAbcMemcpyLong(double * array,const double * arrayFrom,int size);
void CoinAbcMemcpyLong(int * array,const int * arrayFrom,int size);
void CoinAbcMemcpyLong(unsigned char * array,const unsigned char * arrayFrom,int size);
void CoinAbcMemset0Long(double * array,int size);
void CoinAbcMemset0Long(int * array,int size);
void CoinAbcMemset0Long(unsigned char * array,int size);
void CoinAbcMemmove(double * array,const double * arrayFrom,int size);
void CoinAbcMemmove(int * array,const int * arrayFrom,int size);
void CoinAbcMemmove(unsigned char * array,const unsigned char * arrayFrom,int size);
/// This moves down and zeroes out end
void CoinAbcMemmoveAndZero(double * array,double * arrayFrom,int size);
/// This compacts several sections and zeroes out end (returns number)
int CoinAbcCompact(int numberSections,int alreadyDone,double * array,const int * starts, const int * lengths); 
/// This compacts several sections (returns number)
int CoinAbcCompact(int numberSections,int alreadyDone,int * array,const int * starts, const int * lengths); 
#endif
#if ABC_CREATE_SCATTER_FUNCTION
SCATTER_ATTRIBUTE void functionName(ScatterUpdate1)(int numberIn, CoinFactorizationDouble multiplier,
			  const CoinFactorizationDouble *  COIN_RESTRICT element,
			  CoinFactorizationDouble * COIN_RESTRICT region)
{
#ifndef NDEBUG
  assert (numberIn==1);
#endif
  const int * COIN_RESTRICT thisColumn = reinterpret_cast<const int *>(element+1);
  int iColumn0=thisColumn[0];
  double value0=region[iColumn0];
  value0 OPERATION multiplier*element[0];
  region[iColumn0]=value0;
}
SCATTER_ATTRIBUTE void functionName(ScatterUpdate2)(int numberIn, CoinFactorizationDouble multiplier,
			  const CoinFactorizationDouble *  COIN_RESTRICT element,
			  CoinFactorizationDouble * COIN_RESTRICT region)
{
#ifndef NDEBUG
  assert (numberIn==2);
#endif
  const int * COIN_RESTRICT thisColumn = reinterpret_cast<const int *>(element+2);
#if NEW_CHUNK_SIZE==2
  int nFull=2&(~(NEW_CHUNK_SIZE-1));
  for (int j=0;j<nFull;j+=NEW_CHUNK_SIZE) {
    coin_prefetch(element+NEW_CHUNK_SIZE_INCREMENT);
    int iColumn0=thisColumn[0];
    int iColumn1=thisColumn[1];
    CoinFactorizationDouble value0=region[iColumn0];
    CoinFactorizationDouble value1=region[iColumn1];
    value0 OPERATION multiplier*element[0+NEW_CHUNK_SIZE_OFFSET];
    value1 OPERATION multiplier*element[1+NEW_CHUNK_SIZE_OFFSET];
    region[iColumn0]=value0;
    region[iColumn1]=value1;
    element+=NEW_CHUNK_SIZE_INCREMENT;
    thisColumn = reinterpret_cast<const int *>(element);
  }
#endif
#if NEW_CHUNK_SIZE==4
  int iColumn0=thisColumn[0];
  int iColumn1=thisColumn[1];
  CoinFactorizationDouble value0=region[iColumn0];
  CoinFactorizationDouble value1=region[iColumn1];
  value0 OPERATION multiplier*element[0];
  value1 OPERATION multiplier*element[1];
  region[iColumn0]=value0;
  region[iColumn1]=value1;
#endif
}
SCATTER_ATTRIBUTE void functionName(ScatterUpdate3)(int numberIn, CoinFactorizationDouble multiplier,
			  const CoinFactorizationDouble *  COIN_RESTRICT element,
			  CoinFactorizationDouble * COIN_RESTRICT region)
{
#ifndef NDEBUG
  assert (numberIn==3);
#endif
  const int * COIN_RESTRICT thisColumn = reinterpret_cast<const int *>(element+3);
#if AVX2==1
  double temp[2];
#endif
#if NEW_CHUNK_SIZE==2
  int nFull=3&(~(NEW_CHUNK_SIZE-1));
  for (int j=0;j<nFull;j+=NEW_CHUNK_SIZE) {
    //coin_prefetch_const(element+NEW_CHUNK_SIZE_INCREMENT);
    int iColumn0=thisColumn[0];
    int iColumn1=thisColumn[1];
    CoinFactorizationDouble value0=region[iColumn0];
    CoinFactorizationDouble value1=region[iColumn1];
    value0 OPERATION multiplier*element[0];
    value1 OPERATION multiplier*element[1];
    region[iColumn0]=value0;
    region[iColumn1]=value1;
    element+=NEW_CHUNK_SIZE;
    thisColumn+ = NEW_CHUNK_SIZE;
  }
#endif
#if NEW_CHUNK_SIZE==2
  int iColumn0=thisColumn[0];
  double value0=region[iColumn0];
  value0 OPERATION multiplier*element[0];
  region[iColumn0]=value0;
#else
  int iColumn0=thisColumn[0];
  int iColumn1=thisColumn[1];
  int iColumn2=thisColumn[2];
#if AVX2==1
  __v2df bb;
  double value2=region[iColumn2];
  value2 OPERATION multiplier*element[2];
  set_const_v2df(bb,multiplier);
  temp[0]=region[iColumn0];
  temp[1]=region[iColumn1];
  region[iColumn2]=value2;
  __v2df v0 = __builtin_ia32_loadupd (temp);
  __v2df a = __builtin_ia32_loadupd (element);
  a *= bb;
  v0 OPERATION a;
  __builtin_ia32_storeupd (temp, v0);
  region[iColumn0]=temp[0];
  region[iColumn1]=temp[1];
#else
  double value0=region[iColumn0];
  double value1=region[iColumn1];
  double value2=region[iColumn2];
  value0 OPERATION multiplier*element[0];
  value1 OPERATION multiplier*element[1];
  value2 OPERATION multiplier*element[2];
  region[iColumn0]=value0;
  region[iColumn1]=value1;
  region[iColumn2]=value2;
#endif
#endif
}
SCATTER_ATTRIBUTE void functionName(ScatterUpdate4)(int numberIn, CoinFactorizationDouble multiplier,
			   const CoinFactorizationDouble *  COIN_RESTRICT element,
			   CoinFactorizationDouble * COIN_RESTRICT region)
{
#ifndef NDEBUG
  assert (numberIn==4);
#endif
  const int * COIN_RESTRICT thisColumn = reinterpret_cast<const int *>(element+4);
  int nFull=4&(~(NEW_CHUNK_SIZE-1));
#if AVX2==1
  double temp[4];
#endif
  for (int j=0;j<nFull;j+=NEW_CHUNK_SIZE) {
    //coin_prefetch_const(element+NEW_CHUNK_SIZE_INCREMENT);
#if NEW_CHUNK_SIZE==2
    int iColumn0=thisColumn[0];
    int iColumn1=thisColumn[1];
    double value0=region[iColumn0];
    double value1=region[iColumn1];
    value0 OPERATION multiplier*element[0];
    value1 OPERATION multiplier*element[1];
    region[iColumn0]=value0;
    region[iColumn1]=value1;
#elif NEW_CHUNK_SIZE==4
    int iColumn0=thisColumn[0];
    int iColumn1=thisColumn[1];
    int iColumn2=thisColumn[2];
    int iColumn3=thisColumn[3];
#if AVX2==1
    __v2df bb;
    set_const_v2df(bb,multiplier);
    temp[0]=region[iColumn0];
    temp[1]=region[iColumn1];
    temp[2]=region[iColumn2];
    temp[3]=region[iColumn3];
    __v2df v0 = __builtin_ia32_loadupd (temp);
    __v2df v1 = __builtin_ia32_loadupd (temp+2);
    __v2df a = __builtin_ia32_loadupd (element);
    a *= bb;
    v0 OPERATION a;
    a = __builtin_ia32_loadupd (element+2);
    a *= bb;
    v1 OPERATION a;
    __builtin_ia32_storeupd (temp, v0);
    __builtin_ia32_storeupd (temp+2, v1);
    region[iColumn0]=temp[0];
    region[iColumn1]=temp[1];
    region[iColumn2]=temp[2];
    region[iColumn3]=temp[3];
#else
    double value0=region[iColumn0];
    double value1=region[iColumn1];
    double value2=region[iColumn2];
    double value3=region[iColumn3];
    value0 OPERATION multiplier*element[0];
    value1 OPERATION multiplier*element[1];
    value2 OPERATION multiplier*element[2];
    value3 OPERATION multiplier*element[3];
    region[iColumn0]=value0;
    region[iColumn1]=value1;
    region[iColumn2]=value2;
    region[iColumn3]=value3;
#endif
#else
    abort();
#endif
    element+=NEW_CHUNK_SIZE;
    thisColumn += NEW_CHUNK_SIZE;
  }
}
SCATTER_ATTRIBUTE void functionName(ScatterUpdate5)(int numberIn, CoinFactorizationDouble multiplier,
			   const CoinFactorizationDouble *  COIN_RESTRICT element,
			   CoinFactorizationDouble * COIN_RESTRICT region)
{
#ifndef NDEBUG
  assert (numberIn==5);
#endif
  const int * COIN_RESTRICT thisColumn = reinterpret_cast<const int *>(element+5);
  int nFull=5&(~(NEW_CHUNK_SIZE-1));
#if AVX2==1
  double temp[4];
#endif
  for (int j=0;j<nFull;j+=NEW_CHUNK_SIZE) {
    //coin_prefetch_const(element+NEW_CHUNK_SIZE_INCREMENT);
#if NEW_CHUNK_SIZE==2
    int iColumn0=thisColumn[0];
    int iColumn1=thisColumn[1];
    double value0=region[iColumn0];
    double value1=region[iColumn1];
    value0 OPERATION multiplier*element[0];
    value1 OPERATION multiplier*element[1];
    region[iColumn0]=value0;
    region[iColumn1]=value1;
#elif NEW_CHUNK_SIZE==4
    int iColumn0=thisColumn[0];
    int iColumn1=thisColumn[1];
    int iColumn2=thisColumn[2];
    int iColumn3=thisColumn[3];
#if AVX2==1
    __v2df bb;
    set_const_v2df(bb,multiplier);
    temp[0]=region[iColumn0];
    temp[1]=region[iColumn1];
    temp[2]=region[iColumn2];
    temp[3]=region[iColumn3];
    __v2df v0 = __builtin_ia32_loadupd (temp);
    __v2df v1 = __builtin_ia32_loadupd (temp+2);
    __v2df a = __builtin_ia32_loadupd (element);
    a *= bb;
    v0 OPERATION a;
    a = __builtin_ia32_loadupd (element+2);
    a *= bb;
    v1 OPERATION a;
    __builtin_ia32_storeupd (temp, v0);
    __builtin_ia32_storeupd (temp+2, v1);
    region[iColumn0]=temp[0];
    region[iColumn1]=temp[1];
    region[iColumn2]=temp[2];
    region[iColumn3]=temp[3];
#else
    double value0=region[iColumn0];
    double value1=region[iColumn1];
    double value2=region[iColumn2];
    double value3=region[iColumn3];
    value0 OPERATION multiplier*element[0];
    value1 OPERATION multiplier*element[1];
    value2 OPERATION multiplier*element[2];
    value3 OPERATION multiplier*element[3];
    region[iColumn0]=value0;
    region[iColumn1]=value1;
    region[iColumn2]=value2;
    region[iColumn3]=value3;
#endif
#else
    abort();
#endif
    element+=NEW_CHUNK_SIZE;
    thisColumn += NEW_CHUNK_SIZE;
  }
  int iColumn0=thisColumn[0];
  double value0=region[iColumn0];
  value0 OPERATION multiplier*element[0];
  region[iColumn0]=value0;
}
SCATTER_ATTRIBUTE void functionName(ScatterUpdate6)(int numberIn, CoinFactorizationDouble multiplier,
			   const CoinFactorizationDouble *  COIN_RESTRICT element,
			   CoinFactorizationDouble * COIN_RESTRICT region)
{
#ifndef NDEBUG
  assert (numberIn==6);
#endif
  const int * COIN_RESTRICT thisColumn = reinterpret_cast<const int *>(element+6);
  int nFull=6&(~(NEW_CHUNK_SIZE-1));
#if AVX2==1
  double temp[4];
#endif
  for (int j=0;j<nFull;j+=NEW_CHUNK_SIZE) {
    coin_prefetch_const(element+6);
#if NEW_CHUNK_SIZE==2
    int iColumn0=thisColumn[0];
    int iColumn1=thisColumn[1];
    double value0=region[iColumn0];
    double value1=region[iColumn1];
    value0 OPERATION multiplier*element[0];
    value1 OPERATION multiplier*element[1];
    region[iColumn0]=value0;
    region[iColumn1]=value1;
#elif NEW_CHUNK_SIZE==4
    int iColumn0=thisColumn[0];
    int iColumn1=thisColumn[1];
    int iColumn2=thisColumn[2];
    int iColumn3=thisColumn[3];
#if AVX2==1
    __v2df bb;
    set_const_v2df(bb,multiplier);
    temp[0]=region[iColumn0];
    temp[1]=region[iColumn1];
    temp[2]=region[iColumn2];
    temp[3]=region[iColumn3];
    __v2df v0 = __builtin_ia32_loadupd (temp);
    __v2df v1 = __builtin_ia32_loadupd (temp+2);
    __v2df a = __builtin_ia32_loadupd (element);
    a *= bb;
    v0 OPERATION a;
    a = __builtin_ia32_loadupd (element+2);
    a *= bb;
    v1 OPERATION a;
    __builtin_ia32_storeupd (temp, v0);
    __builtin_ia32_storeupd (temp+2, v1);
    region[iColumn0]=temp[0];
    region[iColumn1]=temp[1];
    region[iColumn2]=temp[2];
    region[iColumn3]=temp[3];
#else
    double value0=region[iColumn0];
    double value1=region[iColumn1];
    double value2=region[iColumn2];
    double value3=region[iColumn3];
    value0 OPERATION multiplier*element[0];
    value1 OPERATION multiplier*element[1];
    value2 OPERATION multiplier*element[2];
    value3 OPERATION multiplier*element[3];
    region[iColumn0]=value0;
    region[iColumn1]=value1;
    region[iColumn2]=value2;
    region[iColumn3]=value3;
#endif
#else
    abort();
#endif
    element+=NEW_CHUNK_SIZE;
    thisColumn += NEW_CHUNK_SIZE;
  }
#if NEW_CHUNK_SIZE==4
  int iColumn0=thisColumn[0];
  int iColumn1=thisColumn[1];
  double value0=region[iColumn0];
  double value1=region[iColumn1];
  value0 OPERATION multiplier*element[0];
  value1 OPERATION multiplier*element[1];
  region[iColumn0]=value0;
  region[iColumn1]=value1;
#endif
}
SCATTER_ATTRIBUTE void functionName(ScatterUpdate7)(int numberIn, CoinFactorizationDouble multiplier,
			   const CoinFactorizationDouble *  COIN_RESTRICT element,
			   CoinFactorizationDouble * COIN_RESTRICT region)
{
#ifndef NDEBUG
  assert (numberIn==7);
#endif
  const int * COIN_RESTRICT thisColumn = reinterpret_cast<const int *>(element+7);
  int nFull=7&(~(NEW_CHUNK_SIZE-1));
#if AVX2==1
  double temp[4];
#endif
  for (int j=0;j<nFull;j+=NEW_CHUNK_SIZE) {
    coin_prefetch_const(element+6);
#if NEW_CHUNK_SIZE==2
    int iColumn0=thisColumn[0];
    int iColumn1=thisColumn[1];
    double value0=region[iColumn0];
    double value1=region[iColumn1];
    value0 OPERATION multiplier*element[0];
    value1 OPERATION multiplier*element[1];
    region[iColumn0]=value0;
    region[iColumn1]=value1;
#elif NEW_CHUNK_SIZE==4
    int iColumn0=thisColumn[0];
    int iColumn1=thisColumn[1];
    int iColumn2=thisColumn[2];
    int iColumn3=thisColumn[3];
#if AVX2==1
    __v2df bb;
    set_const_v2df(bb,multiplier);
    temp[0]=region[iColumn0];
    temp[1]=region[iColumn1];
    temp[2]=region[iColumn2];
    temp[3]=region[iColumn3];
    __v2df v0 = __builtin_ia32_loadupd (temp);
    __v2df v1 = __builtin_ia32_loadupd (temp+2);
    __v2df a = __builtin_ia32_loadupd (element);
    a *= bb;
    v0 OPERATION a;
    a = __builtin_ia32_loadupd (element+2);
    a *= bb;
    v1 OPERATION a;
    __builtin_ia32_storeupd (temp, v0);
    __builtin_ia32_storeupd (temp+2, v1);
    region[iColumn0]=temp[0];
    region[iColumn1]=temp[1];
    region[iColumn2]=temp[2];
    region[iColumn3]=temp[3];
#else
    double value0=region[iColumn0];
    double value1=region[iColumn1];
    double value2=region[iColumn2];
    double value3=region[iColumn3];
    value0 OPERATION multiplier*element[0];
    value1 OPERATION multiplier*element[1];
    value2 OPERATION multiplier*element[2];
    value3 OPERATION multiplier*element[3];
    region[iColumn0]=value0;
    region[iColumn1]=value1;
    region[iColumn2]=value2;
    region[iColumn3]=value3;
#endif
#else
    abort();
#endif
    element+=NEW_CHUNK_SIZE;
    thisColumn += NEW_CHUNK_SIZE;
  }
#if NEW_CHUNK_SIZE==2
  int iColumn0=thisColumn[0];
  double value0=region[iColumn0];
  value0 OPERATION multiplier*element[0];
  region[iColumn0]=value0;
#else
  int iColumn0=thisColumn[0];
  int iColumn1=thisColumn[1];
  int iColumn2=thisColumn[2];
  double value0=region[iColumn0];
  double value1=region[iColumn1];
  double value2=region[iColumn2];
  value0 OPERATION multiplier*element[0];
  value1 OPERATION multiplier*element[1];
  value2 OPERATION multiplier*element[2];
  region[iColumn0]=value0;
  region[iColumn1]=value1;
  region[iColumn2]=value2;
#endif
}
SCATTER_ATTRIBUTE void functionName(ScatterUpdate8)(int numberIn, CoinFactorizationDouble multiplier,
			   const CoinFactorizationDouble *  COIN_RESTRICT element,
			   CoinFactorizationDouble * COIN_RESTRICT region)
{
#ifndef NDEBUG
  assert (numberIn==8);
#endif
  const int * COIN_RESTRICT thisColumn = reinterpret_cast<const int *>(element+8);
  int nFull=8&(~(NEW_CHUNK_SIZE-1));
#if AVX2==1
  double temp[4];
#endif
  for (int j=0;j<nFull;j+=NEW_CHUNK_SIZE) {
    coin_prefetch_const(element+6);
#if NEW_CHUNK_SIZE==2
    int iColumn0=thisColumn[0];
    int iColumn1=thisColumn[1];
    double value0=region[iColumn0];
    double value1=region[iColumn1];
    value0 OPERATION multiplier*element[0];
    value1 OPERATION multiplier*element[1];
    region[iColumn0]=value0;
    region[iColumn1]=value1;
#elif NEW_CHUNK_SIZE==4
    int iColumn0=thisColumn[0];
    int iColumn1=thisColumn[1];
    int iColumn2=thisColumn[2];
    int iColumn3=thisColumn[3];
#if AVX2==1
    __v2df bb;
    set_const_v2df(bb,multiplier);
    temp[0]=region[iColumn0];
    temp[1]=region[iColumn1];
    temp[2]=region[iColumn2];
    temp[3]=region[iColumn3];
    __v2df v0 = __builtin_ia32_loadupd (temp);
    __v2df v1 = __builtin_ia32_loadupd (temp+2);
    __v2df a = __builtin_ia32_loadupd (element);
    a *= bb;
    v0 OPERATION a;
    a = __builtin_ia32_loadupd (element+2);
    a *= bb;
    v1 OPERATION a;
    __builtin_ia32_storeupd (temp, v0);
    __builtin_ia32_storeupd (temp+2, v1);
    region[iColumn0]=temp[0];
    region[iColumn1]=temp[1];
    region[iColumn2]=temp[2];
    region[iColumn3]=temp[3];
#else
    double value0=region[iColumn0];
    double value1=region[iColumn1];
    double value2=region[iColumn2];
    double value3=region[iColumn3];
    value0 OPERATION multiplier*element[0];
    value1 OPERATION multiplier*element[1];
    value2 OPERATION multiplier*element[2];
    value3 OPERATION multiplier*element[3];
    region[iColumn0]=value0;
    region[iColumn1]=value1;
    region[iColumn2]=value2;
    region[iColumn3]=value3;
#endif
#else
    abort();
#endif
    element+=NEW_CHUNK_SIZE;
    thisColumn += NEW_CHUNK_SIZE;
  }
}
SCATTER_ATTRIBUTE void functionName(ScatterUpdate4N)(int numberIn, CoinFactorizationDouble multiplier,
			    const CoinFactorizationDouble *  COIN_RESTRICT element,
			    CoinFactorizationDouble * COIN_RESTRICT region)
{
  assert ((numberIn&3)==0);
  const int * COIN_RESTRICT thisColumn = reinterpret_cast<const int *>(element+numberIn);
  int nFull=numberIn&(~(NEW_CHUNK_SIZE-1));
#if AVX2==1
  double temp[4];
#endif
  for (int j=0;j<nFull;j+=NEW_CHUNK_SIZE) {
    coin_prefetch_const(element+16);
    coin_prefetch_const(thisColumn+32);
#if NEW_CHUNK_SIZE==2
    int iColumn0=thisColumn[0];
    int iColumn1=thisColumn[1];
    double value0=region[iColumn0];
    double value1=region[iColumn1];
    value0 OPERATION multiplier*element[0];
    value1 OPERATION multiplier*element[1];
    region[iColumn0]=value0;
    region[iColumn1]=value1;
#elif NEW_CHUNK_SIZE==4
    int iColumn0=thisColumn[0];
    int iColumn1=thisColumn[1];
    int iColumn2=thisColumn[2];
    int iColumn3=thisColumn[3];
#if AVX2==1
    __v2df bb;
    set_const_v2df(bb,multiplier);
    temp[0]=region[iColumn0];
    temp[1]=region[iColumn1];
    temp[2]=region[iColumn2];
    temp[3]=region[iColumn3];
    __v2df v0 = __builtin_ia32_loadupd (temp);
    __v2df v1 = __builtin_ia32_loadupd (temp+2);
    __v2df a = __builtin_ia32_loadupd (element);
    a *= bb;
    v0 OPERATION a;
    a = __builtin_ia32_loadupd (element+2);
    a *= bb;
    v1 OPERATION a;
    __builtin_ia32_storeupd (temp, v0);
    __builtin_ia32_storeupd (temp+2, v1);
    region[iColumn0]=temp[0];
    region[iColumn1]=temp[1];
    region[iColumn2]=temp[2];
    region[iColumn3]=temp[3];
#else
    double value0=region[iColumn0];
    double value1=region[iColumn1];
    double value2=region[iColumn2];
    double value3=region[iColumn3];
    value0 OPERATION multiplier*element[0];
    value1 OPERATION multiplier*element[1];
    value2 OPERATION multiplier*element[2];
    value3 OPERATION multiplier*element[3];
    region[iColumn0]=value0;
    region[iColumn1]=value1;
    region[iColumn2]=value2;
    region[iColumn3]=value3;
#endif
#else
    abort();
#endif
    element+=NEW_CHUNK_SIZE;
    thisColumn += NEW_CHUNK_SIZE;
  }
}
SCATTER_ATTRIBUTE void functionName(ScatterUpdate4NPlus1)(int numberIn, CoinFactorizationDouble multiplier,
				 const CoinFactorizationDouble *  COIN_RESTRICT element,
				 CoinFactorizationDouble * COIN_RESTRICT region)
{
  assert ((numberIn&3)==1);
  const int * COIN_RESTRICT thisColumn = reinterpret_cast<const int *>(element+numberIn);
  int nFull=numberIn&(~(NEW_CHUNK_SIZE-1));
#if AVX2==1
  double temp[4];
#endif
  for (int j=0;j<nFull;j+=NEW_CHUNK_SIZE) {
    coin_prefetch_const(element+16);
    coin_prefetch_const(thisColumn+32);
#if NEW_CHUNK_SIZE==2
    int iColumn0=thisColumn[0];
    int iColumn1=thisColumn[1];
    double value0=region[iColumn0];
    double value1=region[iColumn1];
    value0 OPERATION multiplier*element[0];
    value1 OPERATION multiplier*element[1];
    region[iColumn0]=value0;
    region[iColumn1]=value1;
#elif NEW_CHUNK_SIZE==4
    int iColumn0=thisColumn[0];
    int iColumn1=thisColumn[1];
    int iColumn2=thisColumn[2];
    int iColumn3=thisColumn[3];
#if AVX2==1
    __v2df bb;
    set_const_v2df(bb,multiplier);
    temp[0]=region[iColumn0];
    temp[1]=region[iColumn1];
    temp[2]=region[iColumn2];
    temp[3]=region[iColumn3];
    __v2df v0 = __builtin_ia32_loadupd (temp);
    __v2df v1 = __builtin_ia32_loadupd (temp+2);
    __v2df a = __builtin_ia32_loadupd (element);
    a *= bb;
    v0 OPERATION a;
    a = __builtin_ia32_loadupd (element+2);
    a *= bb;
    v1 OPERATION a;
    __builtin_ia32_storeupd (temp, v0);
    __builtin_ia32_storeupd (temp+2, v1);
    region[iColumn0]=temp[0];
    region[iColumn1]=temp[1];
    region[iColumn2]=temp[2];
    region[iColumn3]=temp[3];
#else
    double value0=region[iColumn0];
    double value1=region[iColumn1];
    double value2=region[iColumn2];
    double value3=region[iColumn3];
    value0 OPERATION multiplier*element[0];
    value1 OPERATION multiplier*element[1];
    value2 OPERATION multiplier*element[2];
    value3 OPERATION multiplier*element[3];
    region[iColumn0]=value0;
    region[iColumn1]=value1;
    region[iColumn2]=value2;
    region[iColumn3]=value3;
#endif
#else
    abort();
#endif
    element+=NEW_CHUNK_SIZE;
    thisColumn += NEW_CHUNK_SIZE;
  }
  int iColumn0=thisColumn[0];
  double value0=region[iColumn0];
  value0 OPERATION multiplier*element[0];
  region[iColumn0]=value0;
}
SCATTER_ATTRIBUTE void functionName(ScatterUpdate4NPlus2)(int numberIn, CoinFactorizationDouble multiplier,
				 const CoinFactorizationDouble *  COIN_RESTRICT element,
				 CoinFactorizationDouble * COIN_RESTRICT region)
{
  assert ((numberIn&3)==2);
  const int * COIN_RESTRICT thisColumn = reinterpret_cast<const int *>(element+numberIn);
  int nFull=numberIn&(~(NEW_CHUNK_SIZE-1));
#if AVX2==1
  double temp[4];
#endif
  for (int j=0;j<nFull;j+=NEW_CHUNK_SIZE) {
    coin_prefetch_const(element+16);
    coin_prefetch_const(thisColumn+32);
#if NEW_CHUNK_SIZE==2
    int iColumn0=thisColumn[0];
    int iColumn1=thisColumn[1];
    double value0=region[iColumn0];
    double value1=region[iColumn1];
    value0 OPERATION multiplier*element[0];
    value1 OPERATION multiplier*element[1];
    region[iColumn0]=value0;
    region[iColumn1]=value1;
#elif NEW_CHUNK_SIZE==4
    int iColumn0=thisColumn[0];
    int iColumn1=thisColumn[1];
    int iColumn2=thisColumn[2];
    int iColumn3=thisColumn[3];
#if AVX2==1
    __v2df bb;
    set_const_v2df(bb,multiplier);
    temp[0]=region[iColumn0];
    temp[1]=region[iColumn1];
    temp[2]=region[iColumn2];
    temp[3]=region[iColumn3];
    __v2df v0 = __builtin_ia32_loadupd (temp);
    __v2df v1 = __builtin_ia32_loadupd (temp+2);
    __v2df a = __builtin_ia32_loadupd (element);
    a *= bb;
    v0 OPERATION a;
    a = __builtin_ia32_loadupd (element+2);
    a *= bb;
    v1 OPERATION a;
    __builtin_ia32_storeupd (temp, v0);
    __builtin_ia32_storeupd (temp+2, v1);
    region[iColumn0]=temp[0];
    region[iColumn1]=temp[1];
    region[iColumn2]=temp[2];
    region[iColumn3]=temp[3];
#else
    double value0=region[iColumn0];
    double value1=region[iColumn1];
    double value2=region[iColumn2];
    double value3=region[iColumn3];
    value0 OPERATION multiplier*element[0];
    value1 OPERATION multiplier*element[1];
    value2 OPERATION multiplier*element[2];
    value3 OPERATION multiplier*element[3];
    region[iColumn0]=value0;
    region[iColumn1]=value1;
    region[iColumn2]=value2;
    region[iColumn3]=value3;
#endif
#else
    abort();
#endif
    element+=NEW_CHUNK_SIZE;
    thisColumn += NEW_CHUNK_SIZE;
  }
#if NEW_CHUNK_SIZE==4
  int iColumn0=thisColumn[0];
  int iColumn1=thisColumn[1];
  double value0=region[iColumn0];
  double value1=region[iColumn1];
  value0 OPERATION multiplier*element[0];
  value1 OPERATION multiplier*element[1];
  region[iColumn0]=value0;
  region[iColumn1]=value1;
#endif
}
SCATTER_ATTRIBUTE void functionName(ScatterUpdate4NPlus3)(int numberIn, CoinFactorizationDouble multiplier,
				 const CoinFactorizationDouble *  COIN_RESTRICT element,
				 CoinFactorizationDouble * COIN_RESTRICT region)
{
  assert ((numberIn&3)==3);
  const int * COIN_RESTRICT thisColumn = reinterpret_cast<const int *>(element+numberIn);
  int nFull=numberIn&(~(NEW_CHUNK_SIZE-1));
#if AVX2==1
  double temp[4];
#endif
  for (int j=0;j<nFull;j+=NEW_CHUNK_SIZE) {
    coin_prefetch_const(element+16);
    coin_prefetch_const(thisColumn+32);
#if NEW_CHUNK_SIZE==2
    int iColumn0=thisColumn[0];
    int iColumn1=thisColumn[1];
    double value0=region[iColumn0];
    double value1=region[iColumn1];
    value0 OPERATION multiplier*element[0];
    value1 OPERATION multiplier*element[1];
    region[iColumn0]=value0;
    region[iColumn1]=value1;
#elif NEW_CHUNK_SIZE==4
    int iColumn0=thisColumn[0];
    int iColumn1=thisColumn[1];
    int iColumn2=thisColumn[2];
    int iColumn3=thisColumn[3];
#if AVX2==1
    __v2df bb;
    set_const_v2df(bb,multiplier);
    temp[0]=region[iColumn0];
    temp[1]=region[iColumn1];
    temp[2]=region[iColumn2];
    temp[3]=region[iColumn3];
    __v2df v0 = __builtin_ia32_loadupd (temp);
    __v2df v1 = __builtin_ia32_loadupd (temp+2);
    __v2df a = __builtin_ia32_loadupd (element);
    a *= bb;
    v0 OPERATION a;
    a = __builtin_ia32_loadupd (element+2);
    a *= bb;
    v1 OPERATION a;
    __builtin_ia32_storeupd (temp, v0);
    __builtin_ia32_storeupd (temp+2, v1);
    region[iColumn0]=temp[0];
    region[iColumn1]=temp[1];
    region[iColumn2]=temp[2];
    region[iColumn3]=temp[3];
#else
    double value0=region[iColumn0];
    double value1=region[iColumn1];
    double value2=region[iColumn2];
    double value3=region[iColumn3];
    value0 OPERATION multiplier*element[0];
    value1 OPERATION multiplier*element[1];
    value2 OPERATION multiplier*element[2];
    value3 OPERATION multiplier*element[3];
    region[iColumn0]=value0;
    region[iColumn1]=value1;
    region[iColumn2]=value2;
    region[iColumn3]=value3;
#endif
#else
    abort();
#endif
    element+=NEW_CHUNK_SIZE;
    thisColumn += NEW_CHUNK_SIZE;
  }
#if NEW_CHUNK_SIZE==2
  int iColumn0=thisColumn[0];
  double value0=region[iColumn0];
  value0 OPERATION multiplier*element[0];
  region[iColumn0]=value0;
#else
  int iColumn0=thisColumn[0];
  int iColumn1=thisColumn[1];
  int iColumn2=thisColumn[2];
  double value0=region[iColumn0];
  double value1=region[iColumn1];
  double value2=region[iColumn2];
  value0 OPERATION multiplier*element[0];
  value1 OPERATION multiplier*element[1];
  value2 OPERATION multiplier*element[2];
  region[iColumn0]=value0;
  region[iColumn1]=value1;
  region[iColumn2]=value2;
#endif
}
#endif
