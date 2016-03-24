/* $Id$ */
// Copyright (C) 2003, International Business Machines
// Corporation and others, Copyright (C) 2012, FasterCoin.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include <cfloat>
#include <cstdlib>
#include <cmath>
#include "CoinHelperFunctions.hpp"
#include "CoinAbcHelperFunctions.hpp"
#include "CoinTypes.hpp"
#include "CoinFinite.hpp"
#include "CoinAbcCommon.hpp"
//#define AVX2 1
#if AVX2==1
#define set_const_v2df(bb,b) {double * temp=reinterpret_cast<double*>(&bb);temp[0]=b;temp[1]=b;}       
#endif
#if INLINE_SCATTER==0
void 
CoinAbcScatterUpdate(int number,CoinFactorizationDouble pivotValue,
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
#endif
#if INLINE_GATHER==0
CoinFactorizationDouble 
CoinAbcGatherUpdate(CoinSimplexInt number,
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
#if INLINE_MULTIPLY_ADD==0
void 
CoinAbcMultiplyIndexed(int number,
		       const CoinFactorizationDouble *  COIN_RESTRICT multiplier,
		       const int *  COIN_RESTRICT thisIndex,
		       CoinSimplexDouble * COIN_RESTRICT region)
{
#ifndef INTEL_COMPILER
// was #pragma simd
#endif
#if UNROLL_MULTIPLY_ADD==0
  for (CoinSimplexInt i = 0; i < number; i ++ ) {
    CoinSimplexInt iRow = thisIndex[i];
    CoinSimplexDouble value = region[iRow];
    value *= multiplier[iRow];
    region[iRow] = value;
  }
#else
#error code
#endif
}
void 
CoinAbcMultiplyIndexed(int number,
		       const long double *  COIN_RESTRICT multiplier,
		       const int *  COIN_RESTRICT thisIndex,
		       long double * COIN_RESTRICT region)
{
#ifndef INTEL_COMPILER
// was #pragma simd
#endif
#if UNROLL_MULTIPLY_ADD==0
  for (CoinSimplexInt i = 0; i < number; i ++ ) {
    CoinSimplexInt iRow = thisIndex[i];
    long double value = region[iRow];
    value *= multiplier[iRow];
    region[iRow] = value;
  }
#else
#error code
#endif
}
#endif
double
CoinAbcMaximumAbsElement(const double * region, int sizeIn)
{
  int size=sizeIn;
     double maxValue = 0.0;
     //printf("a\n");
#ifndef INTEL_COMPILER
// was #pragma simd
#endif
     /*cilk_*/ for (int i = 0; i < size; i++)
          maxValue = CoinMax(maxValue, fabs(region[i]));
     return maxValue;
}
void 
CoinAbcMinMaxAbsElement(const double * region, int sizeIn,double & minimum , double & maximum)
{
  double minValue=minimum;
  double maxValue=maximum;
  int size=sizeIn;
  //printf("b\n");
#ifndef INTEL_COMPILER
// was #pragma simd
#endif
     /*cilk_*/ for (int i=0;i<size;i++) {
    double value=fabs(region[i]);
    minValue=CoinMin(value,minValue);
    maxValue=CoinMax(value,maxValue);
  }
  minimum=minValue;
  maximum=maxValue;
}
void 
CoinAbcMinMaxAbsNormalValues(const double * region, int sizeIn,double & minimum , double & maximum)
{
  int size=sizeIn;
  double minValue=minimum;
  double maxValue=maximum;
#define NORMAL_LOW_VALUE 1.0e-8
#define NORMAL_HIGH_VALUE 1.0e8
  //printf("c\n");
#ifndef INTEL_COMPILER
// was #pragma simd
#endif
     /*cilk_*/ for (int i=0;i<size;i++) {
    double value=fabs(region[i]);
    if (value>NORMAL_LOW_VALUE&&value<NORMAL_HIGH_VALUE) {
      minValue=CoinMin(value,minValue);
      maxValue=CoinMax(value,maxValue);
    }
  }
  minimum=minValue;
  maximum=maxValue;
}
void 
CoinAbcScale(double * region, double multiplier,int sizeIn)
{
  int size=sizeIn;
  // used printf("d\n");
#ifndef INTEL_COMPILER
// was #pragma simd
#endif
#pragma cilk_grainsize=CILK_FOR_GRAINSIZE
  cilk_for (int i=0;i<size;i++) {
    region[i] *= multiplier;
  }
}
void 
CoinAbcScaleNormalValues(double * region, double multiplier,double killIfLessThanThis,int sizeIn)
{
  int size=sizeIn;
     // used printf("e\n");
#ifndef INTEL_COMPILER
// was #pragma simd
#endif
#pragma cilk_grainsize=CILK_FOR_GRAINSIZE
  cilk_for (int i=0;i<size;i++) {
    double value=fabs(region[i]);
    if (value>killIfLessThanThis) {
      if (value!=COIN_DBL_MAX) {
	value *= multiplier;
	if (value>killIfLessThanThis)
	  region[i] *= multiplier;
	else
	  region[i]=0.0;
      }
    } else {
      region[i]=0.0;
    }
  }
}
// maximum fabs(region[i]) and then region[i]*=multiplier
double 
CoinAbcMaximumAbsElementAndScale(double * region, double multiplier,int sizeIn)
{
  double maxValue=0.0;
  int size=sizeIn;
  //printf("f\n");
#ifndef INTEL_COMPILER
// was #pragma simd
#endif
     /*cilk_*/ for (int i=0;i<size;i++) {
    double value=fabs(region[i]);
    region[i] *= multiplier;
    maxValue=CoinMax(value,maxValue);
  }
  return maxValue;
}
void
CoinAbcSetElements(double * region, int sizeIn, double value)
{
  int size=sizeIn;
     printf("g\n");
#ifndef INTEL_COMPILER
// was #pragma simd
#endif
#pragma cilk_grainsize=CILK_FOR_GRAINSIZE
     cilk_for (int i = 0; i < size; i++)
          region[i] = value;
}
void
CoinAbcMultiplyAdd(const double * region1, int sizeIn, double multiplier1,
            double * regionChanged, double multiplier2)
{
  //printf("h\n");
  int size=sizeIn;
     if (multiplier1 == 1.0) {
          if (multiplier2 == 1.0) {
#ifndef INTEL_COMPILER
// was #pragma simd
#endif
               /*cilk_*/ for (int i = 0; i < size; i++)
                    regionChanged[i] = region1[i] + regionChanged[i];
          } else if (multiplier2 == -1.0) {
#ifndef INTEL_COMPILER
// was #pragma simd
#endif
               /*cilk_*/ for (int i = 0; i < size; i++)
                    regionChanged[i] = region1[i] - regionChanged[i];
          } else if (multiplier2 == 0.0) {
#ifndef INTEL_COMPILER
// was #pragma simd
#endif
               /*cilk_*/ for (int i = 0; i < size; i++)
                    regionChanged[i] = region1[i] ;
          } else {
#ifndef INTEL_COMPILER
// was #pragma simd
#endif
               /*cilk_*/ for (int i = 0; i < size; i++)
                    regionChanged[i] = region1[i] + multiplier2 * regionChanged[i];
          }
     } else if (multiplier1 == -1.0) {
          if (multiplier2 == 1.0) {
#ifndef INTEL_COMPILER
// was #pragma simd
#endif
               /*cilk_*/ for (int i = 0; i < size; i++)
                    regionChanged[i] = -region1[i] + regionChanged[i];
          } else if (multiplier2 == -1.0) {
#ifndef INTEL_COMPILER
// was #pragma simd
#endif
               /*cilk_*/ for (int i = 0; i < size; i++)
                    regionChanged[i] = -region1[i] - regionChanged[i];
          } else if (multiplier2 == 0.0) {
#ifndef INTEL_COMPILER
// was #pragma simd
#endif
               /*cilk_*/ for (int i = 0; i < size; i++)
                    regionChanged[i] = -region1[i] ;
          } else {
#ifndef INTEL_COMPILER
// was #pragma simd
#endif
               /*cilk_*/ for (int i = 0; i < size; i++)
                    regionChanged[i] = -region1[i] + multiplier2 * regionChanged[i];
          }
     } else if (multiplier1 == 0.0) {
          if (multiplier2 == 1.0) {
               // nothing to do
          } else if (multiplier2 == -1.0) {
#ifndef INTEL_COMPILER
// was #pragma simd
#endif
               /*cilk_*/ for (int i = 0; i < size; i++)
                    regionChanged[i] =  -regionChanged[i];
          } else if (multiplier2 == 0.0) {
#ifndef INTEL_COMPILER
// was #pragma simd
#endif
               /*cilk_*/ for (int i = 0; i < size; i++)
                    regionChanged[i] =  0.0;
          } else {
#ifndef INTEL_COMPILER
// was #pragma simd
#endif
               /*cilk_*/ for (int i = 0; i < size; i++)
                    regionChanged[i] =  multiplier2 * regionChanged[i];
          }
     } else {
          if (multiplier2 == 1.0) {
#ifndef INTEL_COMPILER
// was #pragma simd
#endif
               /*cilk_*/ for (int i = 0; i < size; i++)
                    regionChanged[i] = multiplier1 * region1[i] + regionChanged[i];
          } else if (multiplier2 == -1.0) {
#ifndef INTEL_COMPILER
// was #pragma simd
#endif
               /*cilk_*/ for (int i = 0; i < size; i++)
                    regionChanged[i] = multiplier1 * region1[i] - regionChanged[i];
          } else if (multiplier2 == 0.0) {
#ifndef INTEL_COMPILER
// was #pragma simd
#endif
               /*cilk_*/ for (int i = 0; i < size; i++)
                    regionChanged[i] = multiplier1 * region1[i] ;
          } else {
#ifndef INTEL_COMPILER
// was #pragma simd
#endif
               /*cilk_*/ for (int i = 0; i < size; i++)
                    regionChanged[i] = multiplier1 * region1[i] + multiplier2 * regionChanged[i];
          }
     }
}
double
CoinAbcInnerProduct(const double * region1, int sizeIn, const double * region2)
{
  int size=sizeIn;
  //printf("i\n");
     double value = 0.0;
#ifndef INTEL_COMPILER
// was #pragma simd
#endif
     /*cilk_*/ for (int i = 0; i < size; i++)
          value += region1[i] * region2[i];
     return value;
}
void
CoinAbcGetNorms(const double * region, int sizeIn, double & norm1, double & norm2)
{
  int size=sizeIn;
  //printf("j\n");
     norm1 = 0.0;
     norm2 = 0.0;
#ifndef INTEL_COMPILER
// was #pragma simd
#endif
     /*cilk_*/ for (int i = 0; i < size; i++) {
          norm2 += region[i] * region[i];
          norm1 = CoinMax(norm1, fabs(region[i]));
     }
}
// regionTo[index[i]]=regionFrom[i]
void 
CoinAbcScatterTo(const double * regionFrom, double * regionTo, const int * index,int numberIn)
{
  int number=numberIn;
     // used printf("k\n");
#ifndef INTEL_COMPILER
// was #pragma simd
#endif
#pragma cilk_grainsize=CILK_FOR_GRAINSIZE
  cilk_for (int i=0;i<number;i++) {
    int k=index[i];
    regionTo[k]=regionFrom[i];
  }
}
// regionTo[i]=regionFrom[index[i]]
void 
CoinAbcGatherFrom(const double * regionFrom, double * regionTo, const int * index,int numberIn)
{
  int number=numberIn;
     // used printf("l\n");
#ifndef INTEL_COMPILER
// was #pragma simd
#endif
#pragma cilk_grainsize=CILK_FOR_GRAINSIZE
  cilk_for (int i=0;i<number;i++) {
    int k=index[i];
    regionTo[i]=regionFrom[k];
  }
}
// regionTo[index[i]]=0.0
void 
CoinAbcScatterZeroTo(double * regionTo, const int * index,int numberIn)
{
  int number=numberIn;
     // used printf("m\n");
#ifndef INTEL_COMPILER
// was #pragma simd
#endif
#pragma cilk_grainsize=CILK_FOR_GRAINSIZE
  cilk_for (int i=0;i<number;i++) {
    int k=index[i];
    regionTo[k]=0.0;
  }
}
// regionTo[indexScatter[indexList[i]]]=regionFrom[indexList[i]]
void 
CoinAbcScatterToList(const double * regionFrom, double * regionTo, 
		   const int * indexList, const int * indexScatter ,int numberIn)
{
  int number=numberIn;
  //printf("n\n");
#ifndef INTEL_COMPILER
// was #pragma simd
#endif
     /*cilk_*/ for (int i=0;i<number;i++) {
    int j=indexList[i];
    double value=regionFrom[j];
    int k=indexScatter[j];
    regionTo[k]=value;
  }
}
void 
CoinAbcInverseSqrts(double * array, int nIn)
{
  int n=nIn;
     // used printf("o\n");
#ifndef INTEL_COMPILER
// was #pragma simd
#endif
#pragma cilk_grainsize=CILK_FOR_GRAINSIZE
  cilk_for (int i = 0; i < n; i++)
    array[i] = 1.0 / sqrt(array[i]);
}
void 
CoinAbcReciprocal(double * array, int nIn, const double *input)
{
  int n=nIn;
     // used printf("p\n");
#ifndef INTEL_COMPILER
// was #pragma simd
#endif
#pragma cilk_grainsize=CILK_FOR_GRAINSIZE
  cilk_for (int i = 0; i < n; i++)
    array[i] = 1.0 / input[i];
}
void 
CoinAbcMemcpyLong(double * array,const double * arrayFrom,int size)
{
  memcpy(array,arrayFrom,size*sizeof(double));
}
void 
CoinAbcMemcpyLong(int * array,const int * arrayFrom,int size)
{
  memcpy(array,arrayFrom,size*sizeof(int));
}
void 
CoinAbcMemcpyLong(unsigned char * array,const unsigned char * arrayFrom,int size)
{
  memcpy(array,arrayFrom,size*sizeof(unsigned char));
}
void 
CoinAbcMemset0Long(double * array,int size)
{
  memset(array,0,size*sizeof(double));
}
void 
CoinAbcMemset0Long(int * array,int size)
{
  memset(array,0,size*sizeof(int));
}
void 
CoinAbcMemset0Long(unsigned char * array,int size)
{
  memset(array,0,size*sizeof(unsigned char));
}
void 
CoinAbcMemmove(double * array,const double * arrayFrom,int size)
{
  memmove(array,arrayFrom,size*sizeof(double));
}
void CoinAbcMemmove(int * array,const int * arrayFrom,int size)
{
  memmove(array,arrayFrom,size*sizeof(int));
}
void CoinAbcMemmove(unsigned char * array,const unsigned char * arrayFrom,int size)
{
  memmove(array,arrayFrom,size*sizeof(unsigned char));
}
// This moves down and zeroes out end
void CoinAbcMemmoveAndZero(double * array,double * arrayFrom,int size)
{
  assert(arrayFrom-array>=0);
  memmove(array,arrayFrom,size*sizeof(double));
  double * endArray = array+size;
  double * endArrayFrom = arrayFrom+size;
  double * startZero = CoinMax(arrayFrom,endArray);
  memset(startZero,0,(endArrayFrom-startZero)*sizeof(double));
}
// This compacts several sections and zeroes out end
int CoinAbcCompact(int numberSections,int alreadyDone,double * array,const int * starts, const int * lengths)
{
  int totalLength=alreadyDone;
  for (int i=0;i<numberSections;i++) {
    totalLength += lengths[i];
  }
  for (int i=0;i<numberSections;i++) {
    int start=starts[i];
    int length=lengths[i];
    int end=start+length;
    memmove(array+alreadyDone,array+start,length*sizeof(double));
    if (end>totalLength) {
      start=CoinMax(start,totalLength);
      memset(array+start,0,(end-start)*sizeof(double));
    }
    alreadyDone += length;
  }
  return totalLength;
} 
// This compacts several sections
int CoinAbcCompact(int numberSections,int alreadyDone,int * array,const int * starts, const int * lengths)
{
  for (int i=0;i<numberSections;i++) {
    memmove(array+alreadyDone,array+starts[i],lengths[i]*sizeof(int));
    alreadyDone += lengths[i];
  }
  return alreadyDone;
}
void CoinAbcScatterUpdate0(int number,CoinFactorizationDouble /*pivotValue*/,
			   const CoinFactorizationDouble *  COIN_RESTRICT /*thisElement*/,
			   const int *  COIN_RESTRICT /*thisIndex*/,
			   CoinFactorizationDouble *  COIN_RESTRICT /*region*/)
{
  assert (number==0);
}
void CoinAbcScatterUpdate1(int number,CoinFactorizationDouble pivotValue,
			  const CoinFactorizationDouble *  COIN_RESTRICT thisElement,
			  const int *  COIN_RESTRICT thisIndex,
			  CoinFactorizationDouble * COIN_RESTRICT region)
{
  assert (number==1);
  int iRow0 = thisIndex[0];
  CoinFactorizationDouble regionValue0 = region[iRow0];
  region[iRow0] = regionValue0 - thisElement[0] * pivotValue;
}
void CoinAbcScatterUpdate2(int number,CoinFactorizationDouble pivotValue,
			  const CoinFactorizationDouble *  COIN_RESTRICT thisElement,
			  const int *  COIN_RESTRICT thisIndex,
			  CoinFactorizationDouble * COIN_RESTRICT region)
{
  assert (number==2);
  int iRow0 = thisIndex[0];
  int iRow1 = thisIndex[1];
  CoinFactorizationDouble regionValue0 = region[iRow0];
  CoinFactorizationDouble regionValue1 = region[iRow1];
  region[iRow0] = regionValue0 - thisElement[0] * pivotValue;
  region[iRow1] = regionValue1 - thisElement[1] * pivotValue;
}
void CoinAbcScatterUpdate3(int number,CoinFactorizationDouble pivotValue,
			  const CoinFactorizationDouble *  COIN_RESTRICT thisElement,
			  const int *  COIN_RESTRICT thisIndex,
			  CoinFactorizationDouble * COIN_RESTRICT region)
{
  assert (number==3);
  int iRow0 = thisIndex[0];
  int iRow1 = thisIndex[1];
  CoinFactorizationDouble regionValue0 = region[iRow0];
  CoinFactorizationDouble regionValue1 = region[iRow1];
  region[iRow0] = regionValue0 - thisElement[0] * pivotValue;
  region[iRow1] = regionValue1 - thisElement[1] * pivotValue;
  iRow0 = thisIndex[2];
  regionValue0 = region[iRow0];
  region[iRow0] = regionValue0 - thisElement[2] * pivotValue;
}
void CoinAbcScatterUpdate4(int number,CoinFactorizationDouble pivotValue,
			  const CoinFactorizationDouble *  COIN_RESTRICT thisElement,
			  const int *  COIN_RESTRICT thisIndex,
			  CoinFactorizationDouble * COIN_RESTRICT region)
{
  assert (number==4);
  int iRow0 = thisIndex[0];
  int iRow1 = thisIndex[1];
  CoinFactorizationDouble regionValue0 = region[iRow0];
  CoinFactorizationDouble regionValue1 = region[iRow1];
  region[iRow0] = regionValue0 - thisElement[0] * pivotValue;
  region[iRow1] = regionValue1 - thisElement[1] * pivotValue;
  iRow0 = thisIndex[2];
  iRow1 = thisIndex[3];
  regionValue0 = region[iRow0];
  regionValue1 = region[iRow1];
  region[iRow0] = regionValue0 - thisElement[2] * pivotValue;
  region[iRow1] = regionValue1 - thisElement[3] * pivotValue;
}
void CoinAbcScatterUpdate5(int number,CoinFactorizationDouble pivotValue,
			  const CoinFactorizationDouble *  COIN_RESTRICT thisElement,
			  const int *  COIN_RESTRICT thisIndex,
			  CoinFactorizationDouble * COIN_RESTRICT region)
{
  assert (number==5);
  int iRow0 = thisIndex[0];
  int iRow1 = thisIndex[1];
  CoinFactorizationDouble regionValue0 = region[iRow0];
  CoinFactorizationDouble regionValue1 = region[iRow1];
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
}
void CoinAbcScatterUpdate6(int number,CoinFactorizationDouble pivotValue,
			  const CoinFactorizationDouble *  COIN_RESTRICT thisElement,
			  const int *  COIN_RESTRICT thisIndex,
			  CoinFactorizationDouble * COIN_RESTRICT region)
{
  assert (number==6);
  int iRow0 = thisIndex[0];
  int iRow1 = thisIndex[1];
  CoinFactorizationDouble regionValue0 = region[iRow0];
  CoinFactorizationDouble regionValue1 = region[iRow1];
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
}
void CoinAbcScatterUpdate7(int number,CoinFactorizationDouble pivotValue,
			  const CoinFactorizationDouble *  COIN_RESTRICT thisElement,
			  const int *  COIN_RESTRICT thisIndex,
			  CoinFactorizationDouble * COIN_RESTRICT region)
{
  assert (number==7);
  int iRow0 = thisIndex[0];
  int iRow1 = thisIndex[1];
  CoinFactorizationDouble regionValue0 = region[iRow0];
  CoinFactorizationDouble regionValue1 = region[iRow1];
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
}
void CoinAbcScatterUpdate8(int number,CoinFactorizationDouble pivotValue,
			  const CoinFactorizationDouble *  COIN_RESTRICT thisElement,
			  const int *  COIN_RESTRICT thisIndex,
			  CoinFactorizationDouble * COIN_RESTRICT region)
{
  assert (number==8);
  int iRow0 = thisIndex[0];
  int iRow1 = thisIndex[1];
  CoinFactorizationDouble regionValue0 = region[iRow0];
  CoinFactorizationDouble regionValue1 = region[iRow1];
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
}
void CoinAbcScatterUpdate4N(int number,CoinFactorizationDouble pivotValue,
			  const CoinFactorizationDouble *  COIN_RESTRICT thisElement,
			  const int *  COIN_RESTRICT thisIndex,
			  CoinFactorizationDouble * COIN_RESTRICT region)
{
  assert ((number&3)==0);
  for (CoinBigIndex j=number-1 ; j >=0; j-=4 ) {
    CoinSimplexInt iRow0 = thisIndex[j];
    CoinSimplexInt iRow1 = thisIndex[j-1];
    CoinFactorizationDouble regionValue0 = region[iRow0];
    CoinFactorizationDouble regionValue1 = region[iRow1];
    region[iRow0] = regionValue0 - thisElement[j] * pivotValue;
    region[iRow1] = regionValue1 - thisElement[j-1] * pivotValue;
    iRow0 = thisIndex[j-2];
    iRow1 = thisIndex[j-3];
    regionValue0 = region[iRow0];
    regionValue1 = region[iRow1];
    region[iRow0] = regionValue0 - thisElement[j-2] * pivotValue;
    region[iRow1] = regionValue1 - thisElement[j-3] * pivotValue;
  }
}
void CoinAbcScatterUpdate4NPlus1(int number,CoinFactorizationDouble pivotValue,
			  const CoinFactorizationDouble *  COIN_RESTRICT thisElement,
			  const int *  COIN_RESTRICT thisIndex,
			  CoinFactorizationDouble * COIN_RESTRICT region)
{
  assert ((number&3)==1);
  int iRow0 = thisIndex[number-1];
  CoinFactorizationDouble regionValue0 = region[iRow0];
  region[iRow0] = regionValue0 - thisElement[number-1] * pivotValue;
  number -= 1;
  for (CoinBigIndex j=number-1 ; j >=0; j-=4 ) {
    CoinSimplexInt iRow0 = thisIndex[j];
    CoinSimplexInt iRow1 = thisIndex[j-1];
    CoinFactorizationDouble regionValue0 = region[iRow0];
    CoinFactorizationDouble regionValue1 = region[iRow1];
    region[iRow0] = regionValue0 - thisElement[j] * pivotValue;
    region[iRow1] = regionValue1 - thisElement[j-1] * pivotValue;
    iRow0 = thisIndex[j-2];
    iRow1 = thisIndex[j-3];
    regionValue0 = region[iRow0];
    regionValue1 = region[iRow1];
    region[iRow0] = regionValue0 - thisElement[j-2] * pivotValue;
    region[iRow1] = regionValue1 - thisElement[j-3] * pivotValue;
  }
}
void CoinAbcScatterUpdate4NPlus2(int number,CoinFactorizationDouble pivotValue,
			  const CoinFactorizationDouble *  COIN_RESTRICT thisElement,
			  const int *  COIN_RESTRICT thisIndex,
			  CoinFactorizationDouble * COIN_RESTRICT region)
{
  assert ((number&3)==2);
  int iRow0 = thisIndex[number-1];
  int iRow1 = thisIndex[number-2];
  CoinFactorizationDouble regionValue0 = region[iRow0];
  CoinFactorizationDouble regionValue1 = region[iRow1];
  region[iRow0] = regionValue0 - thisElement[number-1] * pivotValue;
  region[iRow1] = regionValue1 - thisElement[number-2] * pivotValue;
  number -= 2;
  for (CoinBigIndex j=number-1 ; j >=0; j-=4 ) {
    CoinSimplexInt iRow0 = thisIndex[j];
    CoinSimplexInt iRow1 = thisIndex[j-1];
    CoinFactorizationDouble regionValue0 = region[iRow0];
    CoinFactorizationDouble regionValue1 = region[iRow1];
    region[iRow0] = regionValue0 - thisElement[j] * pivotValue;
    region[iRow1] = regionValue1 - thisElement[j-1] * pivotValue;
    iRow0 = thisIndex[j-2];
    iRow1 = thisIndex[j-3];
    regionValue0 = region[iRow0];
    regionValue1 = region[iRow1];
    region[iRow0] = regionValue0 - thisElement[j-2] * pivotValue;
    region[iRow1] = regionValue1 - thisElement[j-3] * pivotValue;
  }
}
void CoinAbcScatterUpdate4NPlus3(int number,CoinFactorizationDouble pivotValue,
			  const CoinFactorizationDouble *  COIN_RESTRICT thisElement,
			  const int *  COIN_RESTRICT thisIndex,
			  CoinFactorizationDouble * COIN_RESTRICT region)
{
  assert ((number&3)==3);
  int iRow0 = thisIndex[number-1];
  int iRow1 = thisIndex[number-2];
  CoinFactorizationDouble regionValue0 = region[iRow0];
  CoinFactorizationDouble regionValue1 = region[iRow1];
  region[iRow0] = regionValue0 - thisElement[number-1] * pivotValue;
  region[iRow1] = regionValue1 - thisElement[number-2] * pivotValue;
  iRow0 = thisIndex[number-3];
  regionValue0 = region[iRow0];
  region[iRow0] = regionValue0 - thisElement[number-3] * pivotValue;
  number -= 3;
  for (CoinBigIndex j=number-1 ; j >=0; j-=4 ) {
    CoinSimplexInt iRow0 = thisIndex[j];
    CoinSimplexInt iRow1 = thisIndex[j-1];
    CoinFactorizationDouble regionValue0 = region[iRow0];
    CoinFactorizationDouble regionValue1 = region[iRow1];
    region[iRow0] = regionValue0 - thisElement[j] * pivotValue;
    region[iRow1] = regionValue1 - thisElement[j-1] * pivotValue;
    iRow0 = thisIndex[j-2];
    iRow1 = thisIndex[j-3];
    regionValue0 = region[iRow0];
    regionValue1 = region[iRow1];
    region[iRow0] = regionValue0 - thisElement[j-2] * pivotValue;
    region[iRow1] = regionValue1 - thisElement[j-3] * pivotValue;
  }
}
void CoinAbcScatterUpdate0(int numberIn, CoinFactorizationDouble /*multiplier*/,
			   const CoinFactorizationDouble *  COIN_RESTRICT element,
			   CoinFactorizationDouble *  COIN_RESTRICT /*region*/)
{
#ifndef NDEBUG
  assert (numberIn==0);
#endif
}
// start alternatives
#if 0
void CoinAbcScatterUpdate1(int numberIn, CoinFactorizationDouble multiplier,
			  const CoinFactorizationDouble *  COIN_RESTRICT element,
			  CoinFactorizationDouble * COIN_RESTRICT region)
{
#ifndef NDEBUG
  assert (numberIn==1);
#endif
  const int * COIN_RESTRICT thisColumn = reinterpret_cast<const int *>(element);
  element++;
  int iColumn0=thisColumn[0];
  CoinFactorizationDouble value0=region[iColumn0];
  value0 += multiplier*element[0];
  region[iColumn0]=value0;
}
void CoinAbcScatterUpdate2(int numberIn, CoinFactorizationDouble multiplier,
			  const CoinFactorizationDouble *  COIN_RESTRICT element,
			  CoinFactorizationDouble * COIN_RESTRICT region)
{
#ifndef NDEBUG
  assert (numberIn==2);
#endif
  const int * COIN_RESTRICT thisColumn = reinterpret_cast<const int *>(element);
#if NEW_CHUNK_SIZE==2
  int nFull=2&(~(NEW_CHUNK_SIZE-1));
  for (int j=0;j<nFull;j+=NEW_CHUNK_SIZE) {
    coin_prefetch(element+NEW_CHUNK_SIZE_INCREMENT);
    int iColumn0=thisColumn[0];
    int iColumn1=thisColumn[1];
    CoinFactorizationDouble value0=region[iColumn0];
    CoinFactorizationDouble value1=region[iColumn1];
    value0 += multiplier*element[0+NEW_CHUNK_SIZE_OFFSET];
    value1 += multiplier*element[1+NEW_CHUNK_SIZE_OFFSET];
    region[iColumn0]=value0;
    region[iColumn1]=value1;
    element+=NEW_CHUNK_SIZE_INCREMENT;
    thisColumn = reinterpret_cast<const int *>(element);
  }
#endif
#if NEW_CHUNK_SIZE==4
  element++;
  int iColumn0=thisColumn[0];
  int iColumn1=thisColumn[1];
  CoinFactorizationDouble value0=region[iColumn0];
  CoinFactorizationDouble value1=region[iColumn1];
  value0 += multiplier*element[0];
  value1 += multiplier*element[1];
  region[iColumn0]=value0;
  region[iColumn1]=value1;
#endif
}
void CoinAbcScatterUpdate3(int numberIn, CoinFactorizationDouble multiplier,
			  const CoinFactorizationDouble *  COIN_RESTRICT element,
			  CoinFactorizationDouble * COIN_RESTRICT region)
{
#ifndef NDEBUG
  assert (numberIn==3);
#endif
  const int * COIN_RESTRICT thisColumn = reinterpret_cast<const int *>(element);
#if NEW_CHUNK_SIZE==2
  int nFull=3&(~(NEW_CHUNK_SIZE-1));
  for (int j=0;j<nFull;j+=NEW_CHUNK_SIZE) {
    coin_prefetch(element+NEW_CHUNK_SIZE_INCREMENT);
    int iColumn0=thisColumn[0];
    int iColumn1=thisColumn[1];
    CoinFactorizationDouble value0=region[iColumn0];
    CoinFactorizationDouble value1=region[iColumn1];
    value0 += multiplier*element[0+NEW_CHUNK_SIZE_OFFSET];
    value1 += multiplier*element[1+NEW_CHUNK_SIZE_OFFSET];
    region[iColumn0]=value0;
    region[iColumn1]=value1;
    element+=NEW_CHUNK_SIZE_INCREMENT;
    thisColumn = reinterpret_cast<const int *>(element);
  }
#endif
#if NEW_CHUNK_SIZE==2
  element++;
  int iColumn0=thisColumn[0];
  CoinFactorizationDouble value0=region[iColumn0];
  value0 += multiplier*element[0];
  region[iColumn0]=value0;
#else
  element+=2;
  int iColumn0=thisColumn[0];
  int iColumn1=thisColumn[1];
  int iColumn2=thisColumn[2];
  CoinFactorizationDouble value0=region[iColumn0];
  CoinFactorizationDouble value1=region[iColumn1];
  CoinFactorizationDouble value2=region[iColumn2];
  value0 += multiplier*element[0];
  value1 += multiplier*element[1];
  value2 += multiplier*element[2];
  region[iColumn0]=value0;
  region[iColumn1]=value1;
  region[iColumn2]=value2;
#endif
}
void CoinAbcScatterUpdate4(int numberIn, CoinFactorizationDouble multiplier,
			  const CoinFactorizationDouble *  COIN_RESTRICT element,
			  CoinFactorizationDouble * COIN_RESTRICT region)
{
#ifndef NDEBUG
  assert (numberIn==4);
#endif
  const int * COIN_RESTRICT thisColumn = reinterpret_cast<const int *>(element);
  int nFull=4&(~(NEW_CHUNK_SIZE-1));
  for (int j=0;j<nFull;j+=NEW_CHUNK_SIZE) {
    //coin_prefetch(element+NEW_CHUNK_SIZE_INCREMENT);
#if NEW_CHUNK_SIZE==2
    int iColumn0=thisColumn[0];
    int iColumn1=thisColumn[1];
    CoinFactorizationDouble value0=region[iColumn0];
    CoinFactorizationDouble value1=region[iColumn1];
    value0 += multiplier*element[0+NEW_CHUNK_SIZE_OFFSET];
    value1 += multiplier*element[1+NEW_CHUNK_SIZE_OFFSET];
    region[iColumn0]=value0;
    region[iColumn1]=value1;
#elif NEW_CHUNK_SIZE==4
    int iColumn0=thisColumn[0];
    int iColumn1=thisColumn[1];
    int iColumn2=thisColumn[2];
    int iColumn3=thisColumn[3];
    CoinFactorizationDouble value0=region[iColumn0];
    CoinFactorizationDouble value1=region[iColumn1];
    CoinFactorizationDouble value2=region[iColumn2];
    CoinFactorizationDouble value3=region[iColumn3];
    value0 += multiplier*element[0+NEW_CHUNK_SIZE_OFFSET];
    value1 += multiplier*element[1+NEW_CHUNK_SIZE_OFFSET];
    value2 += multiplier*element[2+NEW_CHUNK_SIZE_OFFSET];
    value3 += multiplier*element[3+NEW_CHUNK_SIZE_OFFSET];
    region[iColumn0]=value0;
    region[iColumn1]=value1;
    region[iColumn2]=value2;
    region[iColumn3]=value3;
#else
    abort();
#endif
    element+=NEW_CHUNK_SIZE_INCREMENT;
    thisColumn = reinterpret_cast<const int *>(element);
  }
}
void CoinAbcScatterUpdate5(int numberIn, CoinFactorizationDouble multiplier,
			  const CoinFactorizationDouble *  COIN_RESTRICT element,
			  CoinFactorizationDouble * COIN_RESTRICT region)
{
#ifndef NDEBUG
  assert (numberIn==5);
#endif
  const int * COIN_RESTRICT thisColumn = reinterpret_cast<const int *>(element);
  int nFull=5&(~(NEW_CHUNK_SIZE-1));
  for (int j=0;j<nFull;j+=NEW_CHUNK_SIZE) {
    coin_prefetch_const(element+6);
#if NEW_CHUNK_SIZE==2
    int iColumn0=thisColumn[0];
    int iColumn1=thisColumn[1];
    CoinFactorizationDouble value0=region[iColumn0];
    CoinFactorizationDouble value1=region[iColumn1];
    value0 += multiplier*element[0+NEW_CHUNK_SIZE_OFFSET];
    value1 += multiplier*element[1+NEW_CHUNK_SIZE_OFFSET];
    region[iColumn0]=value0;
    region[iColumn1]=value1;
#elif NEW_CHUNK_SIZE==4
    int iColumn0=thisColumn[0];
    int iColumn1=thisColumn[1];
    int iColumn2=thisColumn[2];
    int iColumn3=thisColumn[3];
    CoinFactorizationDouble value0=region[iColumn0];
    CoinFactorizationDouble value1=region[iColumn1];
    CoinFactorizationDouble value2=region[iColumn2];
    CoinFactorizationDouble value3=region[iColumn3];
    value0 += multiplier*element[0+NEW_CHUNK_SIZE_OFFSET];
    value1 += multiplier*element[1+NEW_CHUNK_SIZE_OFFSET];
    value2 += multiplier*element[2+NEW_CHUNK_SIZE_OFFSET];
    value3 += multiplier*element[3+NEW_CHUNK_SIZE_OFFSET];
    region[iColumn0]=value0;
    region[iColumn1]=value1;
    region[iColumn2]=value2;
    region[iColumn3]=value3;
#else
    abort();
#endif
    element+=NEW_CHUNK_SIZE_INCREMENT;
    thisColumn = reinterpret_cast<const int *>(element);
  }
  element++;
  int iColumn0=thisColumn[0];
  CoinFactorizationDouble value0=region[iColumn0];
  value0 += multiplier*element[0];
  region[iColumn0]=value0;
}
void CoinAbcScatterUpdate6(int numberIn, CoinFactorizationDouble multiplier,
			  const CoinFactorizationDouble *  COIN_RESTRICT element,
			  CoinFactorizationDouble * COIN_RESTRICT region)
{
#ifndef NDEBUG
  assert (numberIn==6);
#endif
  const int * COIN_RESTRICT thisColumn = reinterpret_cast<const int *>(element);
  int nFull=6&(~(NEW_CHUNK_SIZE-1));
  for (int j=0;j<nFull;j+=NEW_CHUNK_SIZE) {
    coin_prefetch_const(element+6);
#if NEW_CHUNK_SIZE==2
    int iColumn0=thisColumn[0];
    int iColumn1=thisColumn[1];
    CoinFactorizationDouble value0=region[iColumn0];
    CoinFactorizationDouble value1=region[iColumn1];
    value0 += multiplier*element[0+NEW_CHUNK_SIZE_OFFSET];
    value1 += multiplier*element[1+NEW_CHUNK_SIZE_OFFSET];
    region[iColumn0]=value0;
    region[iColumn1]=value1;
#elif NEW_CHUNK_SIZE==4
    int iColumn0=thisColumn[0];
    int iColumn1=thisColumn[1];
    int iColumn2=thisColumn[2];
    int iColumn3=thisColumn[3];
    CoinFactorizationDouble value0=region[iColumn0];
    CoinFactorizationDouble value1=region[iColumn1];
    CoinFactorizationDouble value2=region[iColumn2];
    CoinFactorizationDouble value3=region[iColumn3];
    value0 += multiplier*element[0+NEW_CHUNK_SIZE_OFFSET];
    value1 += multiplier*element[1+NEW_CHUNK_SIZE_OFFSET];
    value2 += multiplier*element[2+NEW_CHUNK_SIZE_OFFSET];
    value3 += multiplier*element[3+NEW_CHUNK_SIZE_OFFSET];
    region[iColumn0]=value0;
    region[iColumn1]=value1;
    region[iColumn2]=value2;
    region[iColumn3]=value3;
#else
    abort();
#endif
    element+=NEW_CHUNK_SIZE_INCREMENT;
    thisColumn = reinterpret_cast<const int *>(element);
  }
#if NEW_CHUNK_SIZE==4
  element++;
  int iColumn0=thisColumn[0];
  int iColumn1=thisColumn[1];
  CoinFactorizationDouble value0=region[iColumn0];
  CoinFactorizationDouble value1=region[iColumn1];
  value0 += multiplier*element[0];
  value1 += multiplier*element[1];
  region[iColumn0]=value0;
  region[iColumn1]=value1;
#endif
}
void CoinAbcScatterUpdate7(int numberIn, CoinFactorizationDouble multiplier,
			  const CoinFactorizationDouble *  COIN_RESTRICT element,
			  CoinFactorizationDouble * COIN_RESTRICT region)
{
#ifndef NDEBUG
  assert (numberIn==7);
#endif
  const int * COIN_RESTRICT thisColumn = reinterpret_cast<const int *>(element);
  int nFull=7&(~(NEW_CHUNK_SIZE-1));
  for (int j=0;j<nFull;j+=NEW_CHUNK_SIZE) {
    coin_prefetch_const(element+6);
#if NEW_CHUNK_SIZE==2
    int iColumn0=thisColumn[0];
    int iColumn1=thisColumn[1];
    CoinFactorizationDouble value0=region[iColumn0];
    CoinFactorizationDouble value1=region[iColumn1];
    value0 += multiplier*element[0+NEW_CHUNK_SIZE_OFFSET];
    value1 += multiplier*element[1+NEW_CHUNK_SIZE_OFFSET];
    region[iColumn0]=value0;
    region[iColumn1]=value1;
#elif NEW_CHUNK_SIZE==4
    int iColumn0=thisColumn[0];
    int iColumn1=thisColumn[1];
    int iColumn2=thisColumn[2];
    int iColumn3=thisColumn[3];
    CoinFactorizationDouble value0=region[iColumn0];
    CoinFactorizationDouble value1=region[iColumn1];
    CoinFactorizationDouble value2=region[iColumn2];
    CoinFactorizationDouble value3=region[iColumn3];
    value0 += multiplier*element[0+NEW_CHUNK_SIZE_OFFSET];
    value1 += multiplier*element[1+NEW_CHUNK_SIZE_OFFSET];
    value2 += multiplier*element[2+NEW_CHUNK_SIZE_OFFSET];
    value3 += multiplier*element[3+NEW_CHUNK_SIZE_OFFSET];
    region[iColumn0]=value0;
    region[iColumn1]=value1;
    region[iColumn2]=value2;
    region[iColumn3]=value3;
#else
    abort();
#endif
    element+=NEW_CHUNK_SIZE_INCREMENT;
    thisColumn = reinterpret_cast<const int *>(element);
  }
#if NEW_CHUNK_SIZE==2
  element++;
  int iColumn0=thisColumn[0];
  CoinFactorizationDouble value0=region[iColumn0];
  value0 += multiplier*element[0];
  region[iColumn0]=value0;
#else
  element+=2;
  int iColumn0=thisColumn[0];
  int iColumn1=thisColumn[1];
  int iColumn2=thisColumn[2];
  CoinFactorizationDouble value0=region[iColumn0];
  CoinFactorizationDouble value1=region[iColumn1];
  CoinFactorizationDouble value2=region[iColumn2];
  value0 += multiplier*element[0];
  value1 += multiplier*element[1];
  value2 += multiplier*element[2];
  region[iColumn0]=value0;
  region[iColumn1]=value1;
  region[iColumn2]=value2;
#endif
}
void CoinAbcScatterUpdate8(int numberIn, CoinFactorizationDouble multiplier,
			  const CoinFactorizationDouble *  COIN_RESTRICT element,
			  CoinFactorizationDouble * COIN_RESTRICT region)
{
#ifndef NDEBUG
  assert (numberIn==8);
#endif
  const int * COIN_RESTRICT thisColumn = reinterpret_cast<const int *>(element);
  int nFull=8&(~(NEW_CHUNK_SIZE-1));
  for (int j=0;j<nFull;j+=NEW_CHUNK_SIZE) {
    coin_prefetch_const(element+6);
#if NEW_CHUNK_SIZE==2
    int iColumn0=thisColumn[0];
    int iColumn1=thisColumn[1];
    CoinFactorizationDouble value0=region[iColumn0];
    CoinFactorizationDouble value1=region[iColumn1];
    value0 += multiplier*element[0+NEW_CHUNK_SIZE_OFFSET];
    value1 += multiplier*element[1+NEW_CHUNK_SIZE_OFFSET];
    region[iColumn0]=value0;
    region[iColumn1]=value1;
#elif NEW_CHUNK_SIZE==4
    int iColumn0=thisColumn[0];
    int iColumn1=thisColumn[1];
    int iColumn2=thisColumn[2];
    int iColumn3=thisColumn[3];
    CoinFactorizationDouble value0=region[iColumn0];
    CoinFactorizationDouble value1=region[iColumn1];
    CoinFactorizationDouble value2=region[iColumn2];
    CoinFactorizationDouble value3=region[iColumn3];
    value0 += multiplier*element[0+NEW_CHUNK_SIZE_OFFSET];
    value1 += multiplier*element[1+NEW_CHUNK_SIZE_OFFSET];
    value2 += multiplier*element[2+NEW_CHUNK_SIZE_OFFSET];
    value3 += multiplier*element[3+NEW_CHUNK_SIZE_OFFSET];
    region[iColumn0]=value0;
    region[iColumn1]=value1;
    region[iColumn2]=value2;
    region[iColumn3]=value3;
#else
    abort();
#endif
    element+=NEW_CHUNK_SIZE_INCREMENT;
    thisColumn = reinterpret_cast<const int *>(element);
  }
}
void CoinAbcScatterUpdate4N(int numberIn, CoinFactorizationDouble multiplier,
				 const CoinFactorizationDouble *  COIN_RESTRICT element,
				 CoinFactorizationDouble * COIN_RESTRICT region)
{
  assert ((numberIn&3)==0);
  const int * COIN_RESTRICT thisColumn = reinterpret_cast<const int *>(element);
  int nFull=numberIn&(~(NEW_CHUNK_SIZE-1));
  for (int j=0;j<nFull;j+=NEW_CHUNK_SIZE) {
    coin_prefetch_const(element+6);
#if NEW_CHUNK_SIZE==2
    int iColumn0=thisColumn[0];
    int iColumn1=thisColumn[1];
    CoinFactorizationDouble value0=region[iColumn0];
    CoinFactorizationDouble value1=region[iColumn1];
    value0 += multiplier*element[0+NEW_CHUNK_SIZE_OFFSET];
    value1 += multiplier*element[1+NEW_CHUNK_SIZE_OFFSET];
    region[iColumn0]=value0;
    region[iColumn1]=value1;
#elif NEW_CHUNK_SIZE==4
    int iColumn0=thisColumn[0];
    int iColumn1=thisColumn[1];
    int iColumn2=thisColumn[2];
    int iColumn3=thisColumn[3];
    CoinFactorizationDouble value0=region[iColumn0];
    CoinFactorizationDouble value1=region[iColumn1];
    CoinFactorizationDouble value2=region[iColumn2];
    CoinFactorizationDouble value3=region[iColumn3];
    value0 += multiplier*element[0+NEW_CHUNK_SIZE_OFFSET];
    value1 += multiplier*element[1+NEW_CHUNK_SIZE_OFFSET];
    value2 += multiplier*element[2+NEW_CHUNK_SIZE_OFFSET];
    value3 += multiplier*element[3+NEW_CHUNK_SIZE_OFFSET];
    region[iColumn0]=value0;
    region[iColumn1]=value1;
    region[iColumn2]=value2;
    region[iColumn3]=value3;
#else
    abort();
#endif
    element+=NEW_CHUNK_SIZE_INCREMENT;
    thisColumn = reinterpret_cast<const int *>(element);
  }
}
void CoinAbcScatterUpdate4NPlus1(int numberIn, CoinFactorizationDouble multiplier,
				 const CoinFactorizationDouble *  COIN_RESTRICT element,
				 CoinFactorizationDouble * COIN_RESTRICT region)
{
  assert ((numberIn&3)==1);
  const int * COIN_RESTRICT thisColumn = reinterpret_cast<const int *>(element);
  int nFull=numberIn&(~(NEW_CHUNK_SIZE-1));
  for (int j=0;j<nFull;j+=NEW_CHUNK_SIZE) {
    coin_prefetch_const(element+6);
#if NEW_CHUNK_SIZE==2
    int iColumn0=thisColumn[0];
    int iColumn1=thisColumn[1];
    CoinFactorizationDouble value0=region[iColumn0];
    CoinFactorizationDouble value1=region[iColumn1];
    value0 += multiplier*element[0+NEW_CHUNK_SIZE_OFFSET];
    value1 += multiplier*element[1+NEW_CHUNK_SIZE_OFFSET];
    region[iColumn0]=value0;
    region[iColumn1]=value1;
#elif NEW_CHUNK_SIZE==4
    int iColumn0=thisColumn[0];
    int iColumn1=thisColumn[1];
    int iColumn2=thisColumn[2];
    int iColumn3=thisColumn[3];
    CoinFactorizationDouble value0=region[iColumn0];
    CoinFactorizationDouble value1=region[iColumn1];
    CoinFactorizationDouble value2=region[iColumn2];
    CoinFactorizationDouble value3=region[iColumn3];
    value0 += multiplier*element[0+NEW_CHUNK_SIZE_OFFSET];
    value1 += multiplier*element[1+NEW_CHUNK_SIZE_OFFSET];
    value2 += multiplier*element[2+NEW_CHUNK_SIZE_OFFSET];
    value3 += multiplier*element[3+NEW_CHUNK_SIZE_OFFSET];
    region[iColumn0]=value0;
    region[iColumn1]=value1;
    region[iColumn2]=value2;
    region[iColumn3]=value3;
#else
    abort();
#endif
    element+=NEW_CHUNK_SIZE_INCREMENT;
    thisColumn = reinterpret_cast<const int *>(element);
  }
  element++;
  int iColumn0=thisColumn[0];
  CoinFactorizationDouble value0=region[iColumn0];
  value0 += multiplier*element[0];
  region[iColumn0]=value0;
}
void CoinAbcScatterUpdate4NPlus2(int numberIn, CoinFactorizationDouble multiplier,
				 const CoinFactorizationDouble *  COIN_RESTRICT element,
				 CoinFactorizationDouble * COIN_RESTRICT region)
{
  assert ((numberIn&3)==2);
  const int * COIN_RESTRICT thisColumn = reinterpret_cast<const int *>(element);
  int nFull=numberIn&(~(NEW_CHUNK_SIZE-1));
  for (int j=0;j<nFull;j+=NEW_CHUNK_SIZE) {
    coin_prefetch_const(element+6);
#if NEW_CHUNK_SIZE==2
    int iColumn0=thisColumn[0];
    int iColumn1=thisColumn[1];
    CoinFactorizationDouble value0=region[iColumn0];
    CoinFactorizationDouble value1=region[iColumn1];
    value0 += multiplier*element[0+NEW_CHUNK_SIZE_OFFSET];
    value1 += multiplier*element[1+NEW_CHUNK_SIZE_OFFSET];
    region[iColumn0]=value0;
    region[iColumn1]=value1;
#elif NEW_CHUNK_SIZE==4
    int iColumn0=thisColumn[0];
    int iColumn1=thisColumn[1];
    int iColumn2=thisColumn[2];
    int iColumn3=thisColumn[3];
    CoinFactorizationDouble value0=region[iColumn0];
    CoinFactorizationDouble value1=region[iColumn1];
    CoinFactorizationDouble value2=region[iColumn2];
    CoinFactorizationDouble value3=region[iColumn3];
    value0 += multiplier*element[0+NEW_CHUNK_SIZE_OFFSET];
    value1 += multiplier*element[1+NEW_CHUNK_SIZE_OFFSET];
    value2 += multiplier*element[2+NEW_CHUNK_SIZE_OFFSET];
    value3 += multiplier*element[3+NEW_CHUNK_SIZE_OFFSET];
    region[iColumn0]=value0;
    region[iColumn1]=value1;
    region[iColumn2]=value2;
    region[iColumn3]=value3;
#else
    abort();
#endif
    element+=NEW_CHUNK_SIZE_INCREMENT;
    thisColumn = reinterpret_cast<const int *>(element);
  }
#if NEW_CHUNK_SIZE==4
  element++;
  int iColumn0=thisColumn[0];
  int iColumn1=thisColumn[1];
  CoinFactorizationDouble value0=region[iColumn0];
  CoinFactorizationDouble value1=region[iColumn1];
  value0 += multiplier*element[0];
  value1 += multiplier*element[1];
  region[iColumn0]=value0;
  region[iColumn1]=value1;
#endif
}
void CoinAbcScatterUpdate4NPlus3(int numberIn, CoinFactorizationDouble multiplier,
				 const CoinFactorizationDouble *  COIN_RESTRICT element,
				 CoinFactorizationDouble * COIN_RESTRICT region)
{
  assert ((numberIn&3)==3);
  const int * COIN_RESTRICT thisColumn = reinterpret_cast<const int *>(element);
  int nFull=numberIn&(~(NEW_CHUNK_SIZE-1));
  for (int j=0;j<nFull;j+=NEW_CHUNK_SIZE) {
    coin_prefetch_const(element+6);
#if NEW_CHUNK_SIZE==2
    int iColumn0=thisColumn[0];
    int iColumn1=thisColumn[1];
    CoinFactorizationDouble value0=region[iColumn0];
    CoinFactorizationDouble value1=region[iColumn1];
    value0 += multiplier*element[0+NEW_CHUNK_SIZE_OFFSET];
    value1 += multiplier*element[1+NEW_CHUNK_SIZE_OFFSET];
    region[iColumn0]=value0;
    region[iColumn1]=value1;
#elif NEW_CHUNK_SIZE==4
    int iColumn0=thisColumn[0];
    int iColumn1=thisColumn[1];
    int iColumn2=thisColumn[2];
    int iColumn3=thisColumn[3];
    CoinFactorizationDouble value0=region[iColumn0];
    CoinFactorizationDouble value1=region[iColumn1];
    CoinFactorizationDouble value2=region[iColumn2];
    CoinFactorizationDouble value3=region[iColumn3];
    value0 += multiplier*element[0+NEW_CHUNK_SIZE_OFFSET];
    value1 += multiplier*element[1+NEW_CHUNK_SIZE_OFFSET];
    value2 += multiplier*element[2+NEW_CHUNK_SIZE_OFFSET];
    value3 += multiplier*element[3+NEW_CHUNK_SIZE_OFFSET];
    region[iColumn0]=value0;
    region[iColumn1]=value1;
    region[iColumn2]=value2;
    region[iColumn3]=value3;
#else
    abort();
#endif
    element+=NEW_CHUNK_SIZE_INCREMENT;
    thisColumn = reinterpret_cast<const int *>(element);
  }
#if NEW_CHUNK_SIZE==2
  element++;
  int iColumn0=thisColumn[0];
  CoinFactorizationDouble value0=region[iColumn0];
  value0 += multiplier*element[0];
  region[iColumn0]=value0;
#else
  element+=2;
  int iColumn0=thisColumn[0];
  int iColumn1=thisColumn[1];
  int iColumn2=thisColumn[2];
  CoinFactorizationDouble value0=region[iColumn0];
  CoinFactorizationDouble value1=region[iColumn1];
  CoinFactorizationDouble value2=region[iColumn2];
  value0 += multiplier*element[0];
  value1 += multiplier*element[1];
  value2 += multiplier*element[2];
  region[iColumn0]=value0;
  region[iColumn1]=value1;
  region[iColumn2]=value2;
#endif
}
#else
// alternative
#define CACHE_LINE_SIZE 16
#define OPERATION +=
#define functionName(zname) CoinAbc##zname
#define ABC_CREATE_SCATTER_FUNCTION 1
#include "CoinAbcHelperFunctions.hpp"
#undef OPERATION
#undef functionName
#define OPERATION -=
#define functionName(zname) CoinAbc##zname##Subtract
#include "CoinAbcHelperFunctions.hpp"
#undef OPERATION
#undef functionName
#define OPERATION +=
#define functionName(zname) CoinAbc##zname##Add
#include "CoinAbcHelperFunctions.hpp"
scatterUpdate AbcScatterLow[]={
  &CoinAbcScatterUpdate0,
  &CoinAbcScatterUpdate1,
  &CoinAbcScatterUpdate2,
  &CoinAbcScatterUpdate3,
  &CoinAbcScatterUpdate4,
  &CoinAbcScatterUpdate5,
  &CoinAbcScatterUpdate6,
  &CoinAbcScatterUpdate7,
  &CoinAbcScatterUpdate8};
scatterUpdate AbcScatterHigh[]={
  &CoinAbcScatterUpdate4N,
  &CoinAbcScatterUpdate4NPlus1,
  &CoinAbcScatterUpdate4NPlus2,
  &CoinAbcScatterUpdate4NPlus3};
scatterUpdate AbcScatterLowSubtract[]={
  &CoinAbcScatterUpdate0,
  &CoinAbcScatterUpdate1Subtract,
  &CoinAbcScatterUpdate2Subtract,
  &CoinAbcScatterUpdate3Subtract,
  &CoinAbcScatterUpdate4Subtract,
  &CoinAbcScatterUpdate5Subtract,
  &CoinAbcScatterUpdate6Subtract,
  &CoinAbcScatterUpdate7Subtract,
  &CoinAbcScatterUpdate8Subtract};
scatterUpdate AbcScatterHighSubtract[]={
  &CoinAbcScatterUpdate4NSubtract,
  &CoinAbcScatterUpdate4NPlus1Subtract,
  &CoinAbcScatterUpdate4NPlus2Subtract,
  &CoinAbcScatterUpdate4NPlus3Subtract};
scatterUpdate AbcScatterLowAdd[]={
  &CoinAbcScatterUpdate0,
  &CoinAbcScatterUpdate1Add,
  &CoinAbcScatterUpdate2Add,
  &CoinAbcScatterUpdate3Add,
  &CoinAbcScatterUpdate4Add,
  &CoinAbcScatterUpdate5Add,
  &CoinAbcScatterUpdate6Add,
  &CoinAbcScatterUpdate7Add,
  &CoinAbcScatterUpdate8Add};
scatterUpdate AbcScatterHighAdd[]={
  &CoinAbcScatterUpdate4NAdd,
  &CoinAbcScatterUpdate4NPlus1Add,
  &CoinAbcScatterUpdate4NPlus2Add,
  &CoinAbcScatterUpdate4NPlus3Add};
#endif
#include "CoinPragma.hpp"

#include <math.h>

#include <cfloat>
#include <cassert>
#include <string>
#include <stdio.h>
#include <iostream>
#include "CoinAbcCommonFactorization.hpp"
#if 1
#if AVX2==1
#include "emmintrin.h"
#elif AVX2==2
#include <immintrin.h>
#elif AVX2==3
#include "avx2intrin.h"
#endif
#endif
// cilk seems a bit fragile
#define CILK_FRAGILE 0
#if CILK_FRAGILE>1
#undef cilk_spawn
#undef cilk_sync
#define cilk_spawn
#define cilk_sync
#define ONWARD 0
#elif CILK_FRAGILE==1
#define ONWARD 0
#else
#define ONWARD 1 
#endif
#define BLOCKING1 8 // factorization strip
#define BLOCKING2 8 // dgemm recursive
#define BLOCKING3 32 // dgemm parallel
void CoinAbcDgemm(int m, int n, int k, double * COIN_RESTRICT a,int lda,
			  double * COIN_RESTRICT b,double * COIN_RESTRICT c
#if ABC_PARALLEL==2
			  ,int parallelMode
#endif
)
{
  assert ((m&(BLOCKING8-1))==0&&(n&(BLOCKING8-1))==0&&(k&(BLOCKING8-1))==0);
  /* entry for column j and row i (when multiple of BLOCKING8)
     is at aBlocked+j*m+i*BLOCKING8
  */
  // k is always smallish 
  if (m<=BLOCKING8&&n<=BLOCKING8) {
    assert (m==BLOCKING8&&n==BLOCKING8&&k==BLOCKING8);
    double * COIN_RESTRICT aBase2=a;
    double * COIN_RESTRICT bBase2=b;
    double * COIN_RESTRICT cBase2=c;
    for (int j=0;j<BLOCKING8;j++) {
      double * COIN_RESTRICT aBase=aBase2;
#if AVX2!=2
#if 1
      double c0=cBase2[0];
      double c1=cBase2[1];
      double c2=cBase2[2];
      double c3=cBase2[3];
      double c4=cBase2[4];
      double c5=cBase2[5];
      double c6=cBase2[6];
      double c7=cBase2[7];
      for (int l=0;l<BLOCKING8;l++) {
	double bValue = bBase2[l];
	if (bValue) {
	  c0 -= bValue*aBase[0];
	  c1 -= bValue*aBase[1];
	  c2 -= bValue*aBase[2];
	  c3 -= bValue*aBase[3];
	  c4 -= bValue*aBase[4];
	  c5 -= bValue*aBase[5];
	  c6 -= bValue*aBase[6];
	  c7 -= bValue*aBase[7];
	}
	aBase += BLOCKING8;
      }
      cBase2[0]=c0;
      cBase2[1]=c1;
      cBase2[2]=c2;
      cBase2[3]=c3;
      cBase2[4]=c4;
      cBase2[5]=c5;
      cBase2[6]=c6;
      cBase2[7]=c7;
#else
      for (int l=0;l<BLOCKING8;l++) {
	double bValue = bBase2[l];
	if (bValue) {
	  for (int i=0;i<BLOCKING8;i++) {
	    cBase2[i] -= bValue*aBase[i];
	  }
	}
	aBase += BLOCKING8;
      }
#endif
#else
      //__m256d c0=_mm256_load_pd(cBase2);
      __m256d c0=*reinterpret_cast<__m256d *>(cBase2);
      //__m256d c1=_mm256_load_pd(cBase2+4);
      __m256d c1=*reinterpret_cast<__m256d *>(cBase2+4);
      for (int l=0;l<BLOCKING8;l++) {
	//__m256d bb = _mm256_broadcast_sd(bBase2+l);
	__m256d bb = static_cast<__m256d> (__builtin_ia32_vbroadcastsd256 (bBase2+l));
	//__m256d a0 = _mm256_load_pd(aBase);
	__m256d a0=*reinterpret_cast<__m256d *>(aBase);
	//__m256d a1 = _mm256_load_pd(aBase+4);
	__m256d a1=*reinterpret_cast<__m256d *>(aBase+4);
	c0 -= bb*a0;
	c1 -= bb*a1;
	aBase += BLOCKING8;
      }
      //_mm256_store_pd (cBase2, c0);
      *reinterpret_cast<__m256d *>(cBase2)=c0;
      //_mm256_store_pd (cBase2+4, c1);
      *reinterpret_cast<__m256d *>(cBase2+4)=c1;
#endif
      bBase2 += BLOCKING8;
      cBase2 += BLOCKING8;
    }
  } else if (m>n) {
    // make sure mNew1 multiple of BLOCKING8
#if BLOCKING8==8
    int mNew1=((m+15)>>4)<<3;
#elif BLOCKING8==4
    int mNew1=((m+7)>>3)<<2;
#elif BLOCKING8==2
    int mNew1=((m+3)>>2)<<1;
#else
    abort();
#endif
    assert (mNew1>0&&m-mNew1>0);
#if ABC_PARALLEL==2
    if (mNew1<=BLOCKING3||!parallelMode) {
#endif
      //printf("splitMa mNew1 %d\n",mNew1);
      CoinAbcDgemm(mNew1,n,k,a,lda,b,c
#if ABC_PARALLEL==2
			  ,0
#endif
);
      //printf("splitMb mNew1 %d\n",mNew1);
      CoinAbcDgemm(m-mNew1,n,k,a+mNew1*BLOCKING8,lda,b,c+mNew1*BLOCKING8
#if ABC_PARALLEL==2
			  ,0
#endif
);
#if ABC_PARALLEL==2
    } else {
      //printf("splitMa mNew1 %d\n",mNew1);
      cilk_spawn CoinAbcDgemm(mNew1,n,k,a,lda,b,c,ONWARD);
      //printf("splitMb mNew1 %d\n",mNew1);
      CoinAbcDgemm(m-mNew1,n,k,a+mNew1*BLOCKING8,lda,b,c+mNew1*BLOCKING8,ONWARD);
      cilk_sync;
    }
#endif
  } else {
    // make sure nNew1 multiple of BLOCKING8
#if BLOCKING8==8
    int nNew1=((n+15)>>4)<<3;
#elif BLOCKING8==4
    int nNew1=((n+7)>>3)<<2;
#elif BLOCKING8==2
    int nNew1=((n+3)>>2)<<1;
#else
    abort();
#endif
    assert (nNew1>0&&n-nNew1>0);
#if ABC_PARALLEL==2
    if (nNew1<=BLOCKING3||!parallelMode) {
#endif
      //printf("splitNa nNew1 %d\n",nNew1);
      CoinAbcDgemm(m,nNew1,k,a,lda,b,c
#if ABC_PARALLEL==2
			  ,0
#endif
		    );
      //printf("splitNb nNew1 %d\n",nNew1);
      CoinAbcDgemm(m,n-nNew1,k,a,lda,b+lda*nNew1,c+lda*nNew1
#if ABC_PARALLEL==2
			  ,0
#endif
		    );
#if ABC_PARALLEL==2
    } else {
      //printf("splitNa nNew1 %d\n",nNew1);
      cilk_spawn CoinAbcDgemm(m,nNew1,k,a,lda,b,c,ONWARD);
      //printf("splitNb nNew1 %d\n",nNew1);
      CoinAbcDgemm(m,n-nNew1,k,a,lda,b+lda*nNew1,c+lda*nNew1,ONWARD);
      cilk_sync;
    }
#endif
  }
}
#ifdef ABC_LONG_FACTORIZATION
// Start long double version
void CoinAbcDgemm(int m, int n, int k, long double * COIN_RESTRICT a,int lda,
			  long double * COIN_RESTRICT b,long double * COIN_RESTRICT c
#if ABC_PARALLEL==2
			  ,int parallelMode
#endif
)
{
  assert ((m&(BLOCKING8-1))==0&&(n&(BLOCKING8-1))==0&&(k&(BLOCKING8-1))==0);
  /* entry for column j and row i (when multiple of BLOCKING8)
     is at aBlocked+j*m+i*BLOCKING8
  */
  // k is always smallish 
  if (m<=BLOCKING8&&n<=BLOCKING8) {
    assert (m==BLOCKING8&&n==BLOCKING8&&k==BLOCKING8);
    long double * COIN_RESTRICT aBase2=a;
    long double * COIN_RESTRICT bBase2=b;
    long double * COIN_RESTRICT cBase2=c;
    for (int j=0;j<BLOCKING8;j++) {
      long double * COIN_RESTRICT aBase=aBase2;
#if AVX2!=2
#if 1
      long double c0=cBase2[0];
      long double c1=cBase2[1];
      long double c2=cBase2[2];
      long double c3=cBase2[3];
      long double c4=cBase2[4];
      long double c5=cBase2[5];
      long double c6=cBase2[6];
      long double c7=cBase2[7];
      for (int l=0;l<BLOCKING8;l++) {
	long double bValue = bBase2[l];
	if (bValue) {
	  c0 -= bValue*aBase[0];
	  c1 -= bValue*aBase[1];
	  c2 -= bValue*aBase[2];
	  c3 -= bValue*aBase[3];
	  c4 -= bValue*aBase[4];
	  c5 -= bValue*aBase[5];
	  c6 -= bValue*aBase[6];
	  c7 -= bValue*aBase[7];
	}
	aBase += BLOCKING8;
      }
      cBase2[0]=c0;
      cBase2[1]=c1;
      cBase2[2]=c2;
      cBase2[3]=c3;
      cBase2[4]=c4;
      cBase2[5]=c5;
      cBase2[6]=c6;
      cBase2[7]=c7;
#else
      for (int l=0;l<BLOCKING8;l++) {
	long double bValue = bBase2[l];
	if (bValue) {
	  for (int i=0;i<BLOCKING8;i++) {
	    cBase2[i] -= bValue*aBase[i];
	  }
	}
	aBase += BLOCKING8;
      }
#endif
#else
      //__m256d c0=_mm256_load_pd(cBase2);
      __m256d c0=*reinterpret_cast<__m256d *>(cBase2);
      //__m256d c1=_mm256_load_pd(cBase2+4);
      __m256d c1=*reinterpret_cast<__m256d *>(cBase2+4);
      for (int l=0;l<BLOCKING8;l++) {
	//__m256d bb = _mm256_broadcast_sd(bBase2+l);
	__m256d bb = static_cast<__m256d> (__builtin_ia32_vbroadcastsd256 (bBase2+l));
	//__m256d a0 = _mm256_load_pd(aBase);
	__m256d a0=*reinterpret_cast<__m256d *>(aBase);
	//__m256d a1 = _mm256_load_pd(aBase+4);
	__m256d a1=*reinterpret_cast<__m256d *>(aBase+4);
	c0 -= bb*a0;
	c1 -= bb*a1;
	aBase += BLOCKING8;
      }
      //_mm256_store_pd (cBase2, c0);
      *reinterpret_cast<__m256d *>(cBase2)=c0;
      //_mm256_store_pd (cBase2+4, c1);
      *reinterpret_cast<__m256d *>(cBase2+4)=c1;
#endif
      bBase2 += BLOCKING8;
      cBase2 += BLOCKING8;
    }
  } else if (m>n) {
    // make sure mNew1 multiple of BLOCKING8
#if BLOCKING8==8
    int mNew1=((m+15)>>4)<<3;
#elif BLOCKING8==4
    int mNew1=((m+7)>>3)<<2;
#elif BLOCKING8==2
    int mNew1=((m+3)>>2)<<1;
#else
    abort();
#endif
    assert (mNew1>0&&m-mNew1>0);
#if ABC_PARALLEL==2
    if (mNew1<=BLOCKING3||!parallelMode) {
#endif
      //printf("splitMa mNew1 %d\n",mNew1);
      CoinAbcDgemm(mNew1,n,k,a,lda,b,c
#if ABC_PARALLEL==2
			  ,0
#endif
);
      //printf("splitMb mNew1 %d\n",mNew1);
      CoinAbcDgemm(m-mNew1,n,k,a+mNew1*BLOCKING8,lda,b,c+mNew1*BLOCKING8
#if ABC_PARALLEL==2
			  ,0
#endif
);
#if ABC_PARALLEL==2
    } else {
      //printf("splitMa mNew1 %d\n",mNew1);
      cilk_spawn CoinAbcDgemm(mNew1,n,k,a,lda,b,c,ONWARD);
      //printf("splitMb mNew1 %d\n",mNew1);
      CoinAbcDgemm(m-mNew1,n,k,a+mNew1*BLOCKING8,lda,b,c+mNew1*BLOCKING8,ONWARD);
      cilk_sync;
    }
#endif
  } else {
    // make sure nNew1 multiple of BLOCKING8
#if BLOCKING8==8
    int nNew1=((n+15)>>4)<<3;
#elif BLOCKING8==4
    int nNew1=((n+7)>>3)<<2;
#elif BLOCKING8==2
    int nNew1=((n+3)>>2)<<1;
#else
    abort();
#endif
    assert (nNew1>0&&n-nNew1>0);
#if ABC_PARALLEL==2
    if (nNew1<=BLOCKING3||!parallelMode) {
#endif
      //printf("splitNa nNew1 %d\n",nNew1);
      CoinAbcDgemm(m,nNew1,k,a,lda,b,c
#if ABC_PARALLEL==2
			  ,0
#endif
		    );
      //printf("splitNb nNew1 %d\n",nNew1);
      CoinAbcDgemm(m,n-nNew1,k,a,lda,b+lda*nNew1,c+lda*nNew1
#if ABC_PARALLEL==2
			  ,0
#endif
		    );
#if ABC_PARALLEL==2
    } else {
      //printf("splitNa nNew1 %d\n",nNew1);
      cilk_spawn CoinAbcDgemm(m,nNew1,k,a,lda,b,c,ONWARD);
      //printf("splitNb nNew1 %d\n",nNew1);
      CoinAbcDgemm(m,n-nNew1,k,a,lda,b+lda*nNew1,c+lda*nNew1,ONWARD);
      cilk_sync;
    }
#endif
  }
}
#endif
// From Intel site
// get AVX intrinsics  
#include <immintrin.h>  
// get CPUID capability  
//#include <intrin.h>  
#define cpuid(func,ax,bx,cx,dx)\
	__asm__ __volatile__ ("cpuid":\
	"=a" (ax), "=b" (bx), "=c" (cx), "=d" (dx) : "a" (func));  
// written for clarity, not conciseness  
#define OSXSAVEFlag (1UL<<27)  
#define AVXFlag     ((1UL<<28)|OSXSAVEFlag)  
#define VAESFlag    ((1UL<<25)|AVXFlag|OSXSAVEFlag)  
#define FMAFlag     ((1UL<<12)|AVXFlag|OSXSAVEFlag)  
#define CLMULFlag   ((1UL<< 1)|AVXFlag|OSXSAVEFlag)  
   
bool DetectFeature(unsigned int feature)  
{  
  int CPUInfo[4]; //, InfoType=1, ECX = 1;  
  //__cpuidex(CPUInfo, 1, 1);       // read the desired CPUID format  
  cpuid(1,CPUInfo[0],CPUInfo[1],CPUInfo[2],CPUInfo[3]);
  unsigned int ECX = CPUInfo[2];  // the output of CPUID in the ECX register.   
  if ((ECX & feature) != feature) // Missing feature   
    return false;   
#if 0
  long int val = _xgetbv(0);       // read XFEATURE_ENABLED_MASK register  
  if ((val&6) != 6)               // check OS has enabled both XMM and YMM support.  
    return false;   
#endif
  return true;  
} 
