/* $Id$ */
// Copyright (C) 2002, International Business Machines
// Corporation and others, Copyright (C) 2012, FasterCoin.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).
#ifdef ABC_JUST_ONE_FACTORIZATION
#include "CoinAbcCommonFactorization.hpp"
#define CoinAbcTypeFactorization CoinAbcBaseFactorization
#define ABC_SMALL -1
#include "CoinAbcBaseFactorization.hpp"
#endif
#ifdef CoinAbcTypeFactorization

#include "CoinIndexedVector.hpp"
#include "CoinHelperFunctions.hpp"
#include "CoinAbcHelperFunctions.hpp"
#if CILK_CONFLICT>0
// for conflicts
extern int cilk_conflict;
#endif
//:class CoinAbcTypeFactorization.  Deals with Factorization and Updates

/* Updates one column for dual steepest edge weights (FTRAN) */
void 
CoinAbcTypeFactorization::updateWeights ( CoinIndexedVector & regionSparse) const
{
  toLongArray(&regionSparse,1);
#ifdef ABC_ORDERED_FACTORIZATION
  // Permute in for Ftran
  permuteInForFtran(regionSparse);
#endif
  //  ******* L
  updateColumnL ( &regionSparse
#if ABC_SMALL<2
		  ,reinterpret_cast<CoinAbcStatistics &>(ftranCountInput_)
#endif
#if ABC_PARALLEL
		  // can re-use 0 which would have been used for btran
		  ,0
#endif
		  );
  //row bits here
  updateColumnR ( &regionSparse 
#if ABC_SMALL<2
		  ,reinterpret_cast<CoinAbcStatistics &>(ftranCountInput_) 
#endif
#if ABC_PARALLEL
		  ,0
#endif
		  );
  //update counts
  //  ******* U
  updateColumnU ( &regionSparse
#if ABC_SMALL<2
		  , reinterpret_cast<CoinAbcStatistics &>(ftranCountInput_)
#endif
#if ABC_PARALLEL
		  ,0
#endif
		  );
#if ABC_DEBUG
  regionSparse.checkClean();
#endif
}
CoinSimplexInt CoinAbcTypeFactorization::updateColumn ( CoinIndexedVector & regionSparse)
  const
{
  toLongArray(&regionSparse,0);
#ifdef ABC_ORDERED_FACTORIZATION
  // Permute in for Ftran
  permuteInForFtran(regionSparse);
#endif
  //  ******* L
  updateColumnL ( &regionSparse
#if ABC_SMALL<2
		  ,reinterpret_cast<CoinAbcStatistics &>(ftranCountInput_)
#endif
		  );
  //row bits here
  updateColumnR ( &regionSparse 
#if ABC_SMALL<2
		  ,reinterpret_cast<CoinAbcStatistics &>(ftranCountInput_) 
#endif
		  );
  //update counts
  //  ******* U
  updateColumnU ( &regionSparse
#if ABC_SMALL<2
		  , reinterpret_cast<CoinAbcStatistics &>(ftranCountInput_)
#endif
		  );
#if ABC_DEBUG
  regionSparse.checkClean();
#endif
  return regionSparse.getNumElements();
}
/* Updates one full column (FTRAN) */
void 
CoinAbcTypeFactorization::updateFullColumn ( CoinIndexedVector & regionSparse) const
{
#ifndef ABC_ORDERED_FACTORIZATION
  regionSparse.setNumElements(0);
  regionSparse.scan(0,numberRows_);
#else
  permuteInForFtran(regionSparse,true);
#endif
  if (regionSparse.getNumElements()) {
    toLongArray(&regionSparse,1);
    //  ******* L
    updateColumnL ( &regionSparse
#if ABC_SMALL<2
	      ,reinterpret_cast<CoinAbcStatistics &>(ftranFullCountInput_)
#endif
#if ABC_PARALLEL
		    ,1
#endif
		    );
    //row bits here
    updateColumnR ( &regionSparse 
#if ABC_SMALL<2
	      ,reinterpret_cast<CoinAbcStatistics &>(ftranFullCountInput_) 
#endif
#if ABC_PARALLEL
		    ,1
#endif
		    );
    //update counts
    //  ******* U
    updateColumnU ( &regionSparse
#if ABC_SMALL<2
	      , reinterpret_cast<CoinAbcStatistics &>(ftranFullCountInput_)
#endif
#if ABC_PARALLEL
		    ,1
#endif
		    );
    fromLongArray(1);
#if ABC_DEBUG
    regionSparse.checkClean();
#endif
  }
}
// move stuff like this into CoinAbcHelperFunctions.hpp
inline void multiplyIndexed(CoinSimplexInt number,
			    const CoinFactorizationDouble *  COIN_RESTRICT multiplier,
			    const CoinSimplexInt *  COIN_RESTRICT thisIndex,
			    CoinFactorizationDouble *  COIN_RESTRICT region)
{
  for (CoinSimplexInt i = 0; i < number; i ++ ) {
    CoinSimplexInt iRow = thisIndex[i];
    CoinSimplexDouble value = region[iRow];
    value *= multiplier[iRow];
    region[iRow] = value;
  }
}
/* Updates one full column (BTRAN) */
void 
CoinAbcTypeFactorization::updateFullColumnTranspose ( CoinIndexedVector & regionSparse) const
{
  int numberNonZero=0;
  // Should pass in statistics
  toLongArray(&regionSparse,0);
  CoinFactorizationDouble * COIN_RESTRICT region = denseVector(regionSparse);
  CoinSimplexInt * COIN_RESTRICT regionIndex = regionSparse.getIndices();
#ifndef ABC_ORDERED_FACTORIZATION
  const CoinFactorizationDouble * COIN_RESTRICT pivotRegion = pivotRegionAddress_;
  for (int iRow=0;iRow<numberRows_;iRow++) {
    double value = region[iRow];
    if (value) {
      region[iRow] = value*pivotRegion[iRow];
      regionIndex[numberNonZero++]=iRow;
    }
  }
  regionSparse.setNumElements(numberNonZero);
  if (!numberNonZero)
    return;
  //regionSparse.setNumElements(0);
  //regionSparse.scan(0,numberRows_);
#else
  permuteInForBtranAndMultiply(regionSparse,true);
  if (!regionSparse.getNumElements()) {
    permuteOutForBtran(regionSparse);
    return;
  }
#endif

  //  ******* U
  // Apply pivot region - could be combined for speed
  // Can only combine/move inside vector for sparse
  CoinSimplexInt smallestIndex=pivotLinkedForwardsAddress_[-1];
#if ABC_SMALL<2
  // copy of code inside transposeU
  bool goSparse=false;
#else
#define goSparse false
#endif
#if ABC_SMALL<2
  // Guess at number at end
  if (gotUCopy()) {
    assert (btranFullAverageAfterU_);
    CoinSimplexInt newNumber = 
      static_cast<CoinSimplexInt> (numberNonZero*btranFullAverageAfterU_*twiddleBtranFullFactor1());
    if (newNumber< sparseThreshold_)
      goSparse = true;
  }
#endif
  if (numberNonZero<40&&(numberNonZero<<4)<numberRows_&&!goSparse) {
    CoinSimplexInt *  COIN_RESTRICT pivotRowForward = pivotColumnAddress_;
    CoinSimplexInt smallest=numberRowsExtra_;
    for (CoinSimplexInt j = 0; j < numberNonZero; j++ ) {
      CoinSimplexInt iRow = regionIndex[j];
      if (pivotRowForward[iRow]<smallest) {
	smallest=pivotRowForward[iRow];
	smallestIndex=iRow;
      }
    }
  }
  updateColumnTransposeU ( &regionSparse,smallestIndex
#if ABC_SMALL<2
		  ,reinterpret_cast<CoinAbcStatistics &>(btranFullCountInput_)
#endif
#if ABC_PARALLEL
		    ,0
#endif
			   );
  //row bits here
  updateColumnTransposeR ( &regionSparse 
#if ABC_SMALL<2
	      ,reinterpret_cast<CoinAbcStatistics &>(btranFullCountInput_)
#endif
							 );
  //  ******* L
  updateColumnTransposeL ( &regionSparse 
#if ABC_SMALL<2
		  ,reinterpret_cast<CoinAbcStatistics &>(btranFullCountInput_)
#endif
#if ABC_PARALLEL
				      ,0
#endif
			   );
  fromLongArray(0);
#if ABC_SMALL<3
#ifdef ABC_ORDERED_FACTORIZATION
  permuteOutForBtran(regionSparse);
#endif
#if ABC_DEBUG
  regionSparse.checkClean();
#endif
#else
  numberNonZero=0;
  for (CoinSimplexInt i=0;i<numberRows_;i++) {
    CoinExponent expValue=ABC_EXPONENT(region[i]);
    if (expValue) {
      if (!TEST_EXPONENT_LESS_THAN_TOLERANCE(expValue)) {
	regionIndex[numberNonZero++]=i;
      } else {
	region[i]=0.0;
      }
    }
  }
  regionSparse.setNumElements(numberNonZero);
#endif
}
//  updateColumnL.  Updates part of column (FTRANL)
void
CoinAbcTypeFactorization::updateColumnL ( CoinIndexedVector * regionSparse
#if ABC_SMALL<2
				      ,CoinAbcStatistics & statistics
#endif
#if ABC_PARALLEL
				      ,int whichSparse
#endif
				      ) const
{
#if CILK_CONFLICT>0
#if ABC_PARALLEL
  // for conflicts
#if CILK_CONFLICT>1
  printf("file %s line %d which %d\n",__FILE__,__LINE__,whichSparse);
#endif
  abc_assert((cilk_conflict&(1<<whichSparse))==0);
  cilk_conflict |= (1<<whichSparse);
#else
  abc_assert((cilk_conflict&(1<<0))==0);
  cilk_conflict |= (1<<0);
#endif
#endif
#if ABC_SMALL<2
  CoinSimplexInt number = regionSparse->getNumElements (  );
  if (factorizationStatistics()) {
    statistics.numberCounts_++;
    statistics.countInput_ += number;
  }
#endif
  if (numberL_) {
#if ABC_SMALL<2
    int goSparse;
    // Guess at number at end
    if (gotSparse()) {
      double average = statistics.averageAfterL_*twiddleFactor1S();
      assert (average);
      CoinSimplexInt newNumber = static_cast<CoinSimplexInt> (number*average);
      if (newNumber< sparseThreshold_&&(numberL_<<2)>newNumber*1.0*twiddleFactor2S())
	goSparse = 1;
      else if (3*newNumber < numberRows_)
	goSparse = 0;
      else
	goSparse = -1;
    } else {
      goSparse=0;
    } //if(goSparse==1) goSparse=0;
    if (!goSparse) {
      // densish
      updateColumnLDensish(regionSparse);
    } else if (goSparse<0) {
      // densish
      updateColumnLDense(regionSparse);
    } else {
      // sparse
      updateColumnLSparse(regionSparse
#if ABC_PARALLEL
			  ,whichSparse
#endif
			  );
    }
#else
    updateColumnLDensish(regionSparse);
#endif
  }
#if ABC_SMALL<4
#if ABC_DENSE_CODE>0
  if (numberDense_) {
    instrument_do("CoinAbcFactorizationDense",0.3*numberDense_*numberDense_);
    //take off list
    CoinSimplexInt lastSparse = numberRows_-numberDense_;
    CoinFactorizationDouble * COIN_RESTRICT region = denseVector (regionSparse);
    CoinFactorizationDouble *  COIN_RESTRICT denseArea = denseAreaAddress_;
    CoinFactorizationDouble * COIN_RESTRICT denseRegion = 
      denseArea+leadingDimension_*numberDense_;
    CoinSimplexInt *  COIN_RESTRICT densePermute=
      reinterpret_cast<CoinSimplexInt *>(denseRegion+FACTOR_CPU*numberDense_);
    //for (int i=0;i<numberDense_;i++) {
      //printf("perm %d %d\n",i,densePermute[i]);
      //assert (densePermute[i]>=0&&densePermute[i]<numberRows_);
    //} 
#if ABC_PARALLEL
    if (whichSparse)
      denseRegion+=whichSparse*numberDense_;
    //printf("PP %d %d %s region %x\n",whichSparse,__LINE__,__FILE__,denseRegion);
#endif
    CoinFactorizationDouble * COIN_RESTRICT densePut = 
      denseRegion-lastSparse;
    CoinZeroN(denseRegion,numberDense_);
    bool doDense=false;
#if ABC_SMALL<3
#if ABC_DENSE_CODE==2
	      //short * COIN_RESTRICT forFtran = reinterpret_cast<short *>(densePermute+numberDense_)-lastSparse;
    short * COIN_RESTRICT forFtran2 = reinterpret_cast<short *>(densePermute+numberDense_)+2*numberDense_;
#else
    const CoinSimplexInt * COIN_RESTRICT pivotLBackwardOrder = permuteAddress_;
#endif
    CoinSimplexInt number = regionSparse->getNumElements();
    CoinSimplexInt * COIN_RESTRICT regionIndex = regionSparse->getIndices();
    CoinSimplexInt i=0;
    while (i<number) {
      CoinSimplexInt iRow = regionIndex[i];
#if ABC_DENSE_CODE==2
      int kRow = forFtran2[iRow];
      if (kRow!=-1) {
	doDense=true;
	regionIndex[i] = regionIndex[--number];
	denseRegion[kRow]=region[iRow];
#else
      CoinSimplexInt jRow = pivotLBackwardOrder[iRow];
      if (jRow>=lastSparse) {
	doDense=true;
	regionIndex[i] = regionIndex[--number];
	densePut[jRow]=region[iRow];
#endif
	region[iRow]=0.0;
      } else {
	i++;
      }
    }
#else
    CoinSimplexInt * COIN_RESTRICT forFtran = densePermute+numberDense_-lastSparse;
    const CoinSimplexInt * COIN_RESTRICT pivotLOrder = pivotLOrderAddress_;
    for (CoinSimplexInt jRow=lastSparse;jRow<numberRows_;jRow++) {
      CoinSimplexInt iRow = pivotLOrder[jRow];
      if (region[iRow]) {
	doDense=true;
	//densePut[jRow]=region[iRow];
#if ABC_DENSE_CODE==2
	int kRow=forFtran[jRow];
	denseRegion[kRow]=region[iRow];
#endif
	region[iRow]=0.0;
      }
    }
#endif
    if (doDense) {
#if ABC_DENSE_CODE==1
#ifndef CLAPACK
      char trans = 'N';
      CoinSimplexInt ione=1;
      CoinSimplexInt info;
      F77_FUNC(dgetrs,DGETRS)(&trans,&numberDense_,&ione,denseArea,&leadingDimension_,
			      densePermute,denseRegion,&numberDense_,&info,1);
#else
      clapack_dgetrs(CblasColMajor, CblasNoTrans,numberDense_,1,
		     denseArea, leadingDimension_,densePermute,denseRegion,numberDense_);
#endif
#elif ABC_DENSE_CODE==2
      CoinAbcDgetrs('N',numberDense_,denseArea,denseRegion);
#endif
      const CoinSimplexInt * COIN_RESTRICT pivotLOrder = pivotLOrderAddress_;
      for (CoinSimplexInt i=lastSparse;i<numberRows_;i++) {
	if (!TEST_LESS_THAN_TOLERANCE(densePut[i])) {
#ifndef ABC_ORDERED_FACTORIZATION
	  CoinSimplexInt iRow=pivotLOrder[i];
#else
	  CoinSimplexInt iRow=i;
#endif
	  region[iRow]=densePut[i];
#if ABC_SMALL<3
	  regionIndex[number++] = iRow;
#endif
	}
      }
#if ABC_SMALL<3
      regionSparse->setNumElements(number);
#endif
    }
  }
  //printRegion(*regionSparse,"After FtranL");
#endif
#endif
#if ABC_SMALL<2
  if (factorizationStatistics()) 
    statistics.countAfterL_ += regionSparse->getNumElements();
#endif
#if CILK_CONFLICT>0
#if ABC_PARALLEL
  // for conflicts
  abc_assert((cilk_conflict&(1<<whichSparse))!=0);
  cilk_conflict &= ~(1<<whichSparse);
#else
  abc_assert((cilk_conflict&(1<<0))!=0);
  cilk_conflict &= ~(1<<0);
#endif
#endif
}
#define UNROLL 2
#define INLINE_IT
//#define INLINE_IT2
inline void scatterUpdateInline(CoinSimplexInt number,
			  CoinFactorizationDouble pivotValue,
			  const CoinFactorizationDouble *  COIN_RESTRICT thisElement,
			  const CoinSimplexInt *  COIN_RESTRICT thisIndex,
			  CoinFactorizationDouble *  COIN_RESTRICT region)
{
#if UNROLL==0
  for (CoinBigIndex j=number-1 ; j >=0; j-- ) {
    CoinSimplexInt iRow = thisIndex[j];
    CoinFactorizationDouble regionValue = region[iRow];
    CoinFactorizationDouble value = thisElement[j];
    assert (value);
    region[iRow] = regionValue - value * pivotValue;
  }
#elif UNROLL==1
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
#elif UNROLL==2
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
inline CoinFactorizationDouble gatherUpdate(CoinSimplexInt number,
			  const CoinFactorizationDouble *  COIN_RESTRICT thisElement,
			  const CoinSimplexInt *  COIN_RESTRICT thisIndex,
			  CoinFactorizationDouble *  COIN_RESTRICT region)
{
  CoinFactorizationDouble pivotValue=0.0;
  for (CoinBigIndex j = 0; j < number; j ++ ) {
    CoinFactorizationDouble value = thisElement[j];
    CoinSimplexInt jRow = thisIndex[j];
    value *= region[jRow];
    pivotValue -= value;
  }
  return pivotValue;
}
// Updates part of column (FTRANL) when densish
void 
CoinAbcTypeFactorization::updateColumnLDensish ( CoinIndexedVector * regionSparse) const
{
  CoinSimplexInt * COIN_RESTRICT regionIndex=regionSparse->getIndices();
  CoinFactorizationDouble * COIN_RESTRICT region = denseVector(regionSparse);
  CoinSimplexInt number = regionSparse->getNumElements (  );
#if ABC_SMALL<3
  CoinSimplexInt numberNonZero = 0;
#endif
  
  const CoinBigIndex * COIN_RESTRICT startColumn = startColumnLAddress_;
  const CoinSimplexInt * COIN_RESTRICT indexRow = indexRowLAddress_;
  const CoinFactorizationDouble * COIN_RESTRICT element = elementLAddress_;
  const CoinSimplexInt * COIN_RESTRICT pivotLOrder = pivotLOrderAddress_;
  const CoinSimplexInt * COIN_RESTRICT pivotLBackwardOrder = permuteAddress_;
  CoinSimplexInt last = numberRows_;
  assert ( last == baseL_ + numberL_);
#if ABC_DENSE_CODE>0
  //can take out last bit of sparse L as empty
  last -= numberDense_;
#endif
  CoinSimplexInt smallestIndex = numberRowsExtra_;
  // do easy ones
  for (CoinSimplexInt k=0;k<number;k++) {
    CoinSimplexInt iPivot=regionIndex[k];
#ifndef ABC_ORDERED_FACTORIZATION
    CoinSimplexInt jPivot=pivotLBackwardOrder[iPivot];
#else
    CoinSimplexInt jPivot=iPivot;
#endif
#if ABC_SMALL<3
    if (jPivot>=baseL_)
      smallestIndex = CoinMin(jPivot,smallestIndex);
    else
      regionIndex[numberNonZero++]=iPivot;
#else
    if (jPivot>=baseL_)
      smallestIndex = CoinMin(jPivot,smallestIndex);
#endif      
  }
  instrument_start("CoinAbcFactorizationUpdateLDensish",number+(last-smallestIndex));
  // now others
  for (CoinSimplexInt k = smallestIndex; k < last; k++ ) {
#if 0
    for (int j=0;j<numberRows_;j+=10) {
      for (int jj=j;jj<CoinMin(j+10,numberRows_);jj++)
	printf("%g ",region[jj]);
      printf("\n");
    }
#endif
#ifndef ABC_ORDERED_FACTORIZATION
    CoinSimplexInt i=pivotLOrder[k];
#else
    CoinSimplexInt i=k;
#endif
    CoinExponent expValue=ABC_EXPONENT(region[i]);
    if (expValue) {
      if (!TEST_EXPONENT_LESS_THAN_TOLERANCE(expValue)) {
	CoinBigIndex start = startColumn[k];
	CoinBigIndex end = startColumn[k + 1];
	instrument_add(end-start);
	if (TEST_INT_NONZERO(end-start)) {
	  CoinFactorizationDouble pivotValue = region[i];
#ifndef INLINE_IT
	  for (CoinBigIndex j = start; j < end; j ++ ) {
	    CoinSimplexInt iRow = indexRow[j];
	    CoinFactorizationDouble result = region[iRow];
	    CoinFactorizationDouble value = element[j];
	    region[iRow] = result - value * pivotValue;
	  }     
#else
	  CoinAbcScatterUpdate(end-start,pivotValue,element+start,indexRow+start,region);
#endif
	}
#if ABC_SMALL<3
	regionIndex[numberNonZero++] = i;
      } else {
	region[i] = 0.0;
#endif
      }       
    }
  }     
  // and dense
#if ABC_SMALL<3
  for (CoinSimplexInt k = last; k < numberRows_; k++ ) {
#ifndef ABC_ORDERED_FACTORIZATION
    CoinSimplexInt i=pivotLOrder[k];
#else
    CoinSimplexInt i=k;
#endif
    CoinExponent expValue=ABC_EXPONENT(region[i]);
    if (expValue) {
      if (!TEST_EXPONENT_LESS_THAN_TOLERANCE(expValue)) {
	regionIndex[numberNonZero++] = i;
      } else {
	region[i] = 0.0;
      }       
    }
  }     
  regionSparse->setNumElements ( numberNonZero );
#endif
  instrument_end();
}
// Updates part of column (FTRANL) when dense
void 
CoinAbcTypeFactorization::updateColumnLDense ( CoinIndexedVector * regionSparse) const
{
  CoinSimplexInt * COIN_RESTRICT regionIndex=regionSparse->getIndices();
  CoinFactorizationDouble * COIN_RESTRICT region = denseVector(regionSparse);
  CoinSimplexInt number = regionSparse->getNumElements (  );
#if ABC_SMALL<3
  CoinSimplexInt numberNonZero = 0;
#endif
  
  const CoinBigIndex * COIN_RESTRICT startColumn = startColumnLAddress_;
  const CoinSimplexInt * COIN_RESTRICT indexRow = indexRowLAddress_;
  const CoinFactorizationDouble * COIN_RESTRICT element = elementLAddress_;
  const CoinSimplexInt * COIN_RESTRICT pivotLOrder = pivotLOrderAddress_;
  const CoinSimplexInt * COIN_RESTRICT pivotLBackwardOrder = permuteAddress_;
  CoinSimplexInt last = numberRows_;
  assert ( last == baseL_ + numberL_);
#if ABC_DENSE_CODE>0
  //can take out last bit of sparse L as empty
  last -= numberDense_;
#endif
  CoinSimplexInt smallestIndex = numberRowsExtra_;
  // do easy ones
  for (CoinSimplexInt k=0;k<number;k++) {
    CoinSimplexInt iPivot=regionIndex[k];
#ifndef ABC_ORDERED_FACTORIZATION
    CoinSimplexInt jPivot=pivotLBackwardOrder[iPivot];
#else
    CoinSimplexInt jPivot=iPivot;
#endif
#if ABC_SMALL<3
    if (jPivot>=baseL_)
      smallestIndex = CoinMin(jPivot,smallestIndex);
    else
      regionIndex[numberNonZero++]=iPivot;
#else
    if (jPivot>=baseL_)
      smallestIndex = CoinMin(jPivot,smallestIndex);
#endif      
  }
  instrument_start("CoinAbcFactorizationUpdateLDensish",number+(last-smallestIndex));
  // now others
  for (CoinSimplexInt k = smallestIndex; k < last; k++ ) {
#ifndef ABC_ORDERED_FACTORIZATION
    CoinSimplexInt i=pivotLOrder[k];
#else
    CoinSimplexInt i=k;
#endif
    CoinExponent expValue=ABC_EXPONENT(region[i]);
    if (expValue) {
      if (!TEST_EXPONENT_LESS_THAN_TOLERANCE(expValue)) {
	CoinBigIndex start = startColumn[k];
	CoinBigIndex end = startColumn[k + 1];
	instrument_add(end-start);
	if (TEST_INT_NONZERO(end-start)) {
	  CoinFactorizationDouble pivotValue = region[i];
#ifndef INLINE_IT
	  for (CoinBigIndex j = start; j < end; j ++ ) {
	    CoinSimplexInt iRow = indexRow[j];
	    CoinFactorizationDouble result = region[iRow];
	    CoinFactorizationDouble value = element[j];
	    region[iRow] = result - value * pivotValue;
	  }     
#else
	  CoinAbcScatterUpdate(end-start,pivotValue,element+start,indexRow+start,region);
#endif
	}
#if ABC_SMALL<3
	regionIndex[numberNonZero++] = i;
      } else {
	region[i] = 0.0;
#endif
      }       
    }
  }     
  // and dense
#if ABC_SMALL<3
  for (CoinSimplexInt k = last; k < numberRows_; k++ ) {
#ifndef ABC_ORDERED_FACTORIZATION
    CoinSimplexInt i=pivotLOrder[k];
#else
    CoinSimplexInt i=k;
#endif
    CoinExponent expValue=ABC_EXPONENT(region[i]);
    if (expValue) {
      if (!TEST_EXPONENT_LESS_THAN_TOLERANCE(expValue)) {
	regionIndex[numberNonZero++] = i;
      } else {
	region[i] = 0.0;
      }       
    }
  }     
  regionSparse->setNumElements ( numberNonZero );
#endif
  instrument_end();
}
#if ABC_SMALL<2
// Updates part of column (FTRANL) when sparse
void 
CoinAbcTypeFactorization::updateColumnLSparse ( CoinIndexedVector * regionSparse
#if ABC_PARALLEL
						,int whichSparse
#endif
						) const
{
  CoinSimplexInt * COIN_RESTRICT regionIndex=regionSparse->getIndices();
  CoinFactorizationDouble * COIN_RESTRICT region = denseVector(regionSparse);
  CoinSimplexInt number = regionSparse->getNumElements (  );
  CoinSimplexInt numberNonZero;
  
  numberNonZero = 0;
  
  const CoinBigIndex * COIN_RESTRICT startColumn = startColumnLAddress_;
  const CoinSimplexInt * COIN_RESTRICT indexRow = indexRowLAddress_;
  const CoinFactorizationDouble * COIN_RESTRICT element = elementLAddress_;
  const CoinSimplexInt * COIN_RESTRICT pivotLBackwardOrder = permuteAddress_;
  CoinSimplexInt nList;
  // use sparse_ as temporary area
  // need to redo if this fails (just means using CoinAbcStack to compute sizes)
  assert (sizeof(CoinSimplexInt)==sizeof(CoinBigIndex));
  CoinAbcStack * COIN_RESTRICT stackList = reinterpret_cast<CoinAbcStack *>(sparseAddress_);
  CoinSimplexInt * COIN_RESTRICT list = listAddress_;
#define DO_CHAR1 1
#if DO_CHAR1 // CHAR
  CoinCheckZero * COIN_RESTRICT mark = markRowAddress_;
#else
  //BIT
  CoinSimplexUnsignedInt * COIN_RESTRICT mark = reinterpret_cast<CoinSimplexUnsignedInt *> 
    (markRowAddress_);
#endif
#if ABC_PARALLEL
  //printf("PP %d %d %s\n",whichSparse,__LINE__,__FILE__);
  if (whichSparse) {
    //printf("alternative sparse\n");
    int addAmount=whichSparse*sizeSparseArray_;
    stackList=reinterpret_cast<CoinAbcStack *>(reinterpret_cast<char *>(stackList)+addAmount);
    list=reinterpret_cast<CoinSimplexInt *>(reinterpret_cast<char *>(list)+addAmount);
#if DO_CHAR1 // CHAR
    mark=reinterpret_cast<CoinCheckZero *>(reinterpret_cast<char *>(mark)+addAmount);
#else
    mark=reinterpret_cast<CoinSimplexUnsignedInt *>(reinterpret_cast<char *>(mark)+addAmount);
#endif
  }
#endif
  // mark known to be zero
#ifdef COIN_DEBUG
#if DO_CHAR1 // CHAR
  for (CoinSimplexInt i=0;i<maximumRowsExtra_;i++) {
    assert (!mark[i]);
  }
#else
  //BIT
  for (CoinSimplexInt i=0;i<numberRows_>>COINFACTORIZATION_SHIFT_PER_INT;i++) {
    assert (!mark[i]);
  }
#endif
#endif
  nList=0;
  for (CoinSimplexInt k=0;k<number;k++) {
    CoinSimplexInt kPivot=regionIndex[k];
#ifndef ABC_ORDERED_FACTORIZATION
    CoinSimplexInt iPivot=pivotLBackwardOrder[kPivot];
#else
    CoinSimplexInt iPivot=kPivot;
#endif
    if (iPivot>=baseL_) {
#if DO_CHAR1 // CHAR
      CoinCheckZero mark_k = mark[kPivot];
#else
      CoinSimplexUnsignedInt wordK = kPivot >> COINFACTORIZATION_SHIFT_PER_INT;
      CoinSimplexUnsignedInt bitK = (kPivot & COINFACTORIZATION_MASK_PER_INT);
      CoinSimplexUnsignedInt mark_k=(mark[wordK]>>bitK)&1;
#endif
      if(!mark_k) {
	CoinBigIndex j=startColumn[iPivot+1]-1;
	stackList[0].stack=kPivot;
	CoinBigIndex start=startColumn[iPivot];
	stackList[0].next=j;
	stackList[0].start=start;
        CoinSimplexInt nStack=0;
	while (nStack>=0) {
	  /* take off stack */
#ifndef ABC_ORDERED_FACTORIZATION
	  iPivot=pivotLBackwardOrder[kPivot]; // put startColumn on stackstuff
#else
	  iPivot=kPivot;
#endif
#if 1 //0 // what went wrong??
	  CoinBigIndex startThis=startColumn[iPivot];
	  for (;j>=startThis;j--) {
	    CoinSimplexInt jPivot=indexRow[j];
#if DO_CHAR1 // CHAR
	    CoinCheckZero mark_j = mark[jPivot];
#else
	    CoinSimplexUnsignedInt word0 = jPivot >> COINFACTORIZATION_SHIFT_PER_INT;
	    CoinSimplexUnsignedInt bit0 = (jPivot & COINFACTORIZATION_MASK_PER_INT);
	    CoinSimplexUnsignedInt mark_j=(mark[word0]>>bit0)&1;
#endif
	    if (!mark_j) {
#if DO_CHAR1 // CHAR
	      mark[jPivot]=1;
#else
	      mark[word0]|=(1<<bit0);
#endif
	      /* and new one */
	      kPivot=jPivot;
	      break;
	    }
	  }
	  if (j>=startThis) {
	    /* put back on stack */
	    stackList[nStack].next =j-1;
#ifndef ABC_ORDERED_FACTORIZATION
	    iPivot=pivotLBackwardOrder[kPivot];
#else
	    iPivot=kPivot;
#endif
	    j = startColumn[iPivot+1]-1;
	    nStack++;
	    stackList[nStack].stack=kPivot;
	    assert (kPivot<numberRowsExtra_);
	    CoinBigIndex start=startColumn[iPivot];
	    stackList[nStack].next=j;
	    stackList[nStack].start=start;
#else
	  if (j>=startColumn[iPivot]/*stackList[nStack].start*/) {
	    CoinSimplexInt jPivot=indexRow[j--];
	    /* put back on stack */
	    stackList[nStack].next =j;
#if DO_CHAR1 // CHAR
	    CoinCheckZero mark_j = mark[jPivot];
#else
	    CoinSimplexUnsignedInt word0 = jPivot >> COINFACTORIZATION_SHIFT_PER_INT;
	    CoinSimplexUnsignedInt bit0 = (jPivot & COINFACTORIZATION_MASK_PER_INT);
	    CoinSimplexUnsignedInt mark_j=(mark[word0]>>bit0)&1;
#endif
	    if (!mark_j) {
	      /* and new one */
	      kPivot=jPivot;
#ifndef ABC_ORDERED_FACTORIZATION
	      iPivot=pivotLBackwardOrder[kPivot];
#else
	      iPivot=kPivot;
#endif
	      j = startColumn[iPivot+1]-1;
	      nStack++;
	      stackList[nStack].stack=kPivot;
	      assert (kPivot<numberRowsExtra_);
	      CoinBigIndex start=startColumn[iPivot];
	      stackList[nStack].next=j;
	      stackList[nStack].start=start;
#if DO_CHAR1 // CHAR
	      mark[jPivot]=1;
#else
	      mark[word0]|=(1<<bit0);
#endif
	    }
#endif
	  } else {
	    /* finished so mark */
	    list[nList++]=kPivot;
#if DO_CHAR1 // CHAR
	    mark[kPivot]=1;
#else
	    CoinSimplexUnsignedInt wordB = kPivot >> COINFACTORIZATION_SHIFT_PER_INT;
	    CoinSimplexUnsignedInt bitB = (kPivot & COINFACTORIZATION_MASK_PER_INT);
	    mark[wordB]|=(1<<bitB);
#endif
	    --nStack;
	    if (nStack>=0) {
	      kPivot=stackList[nStack].stack;
#ifndef ABC_ORDERED_FACTORIZATION
	      iPivot=pivotLBackwardOrder[kPivot];
#else
	      iPivot=kPivot;
#endif
	      assert (kPivot<numberRowsExtra_);
	      j=stackList[nStack].next;
	    }
	  }
	}
      }
    } else {
      if (!TEST_LESS_THAN_TOLERANCE(region[kPivot])) {
	// just put on list
	regionIndex[numberNonZero++]=kPivot;
      } else {
	region[kPivot]=0.0;
      }
    }
  }
#if 0
  CoinSimplexDouble temp[20000];
  {
    memcpy(temp,region,numberRows_*sizeof(CoinSimplexDouble));
    for (CoinSimplexInt k = 0; k < numberRows_; k++ ) {
      CoinSimplexInt i=pivotLOrder[k];
      CoinFactorizationDouble pivotValue = temp[i];
    
      if ( !TEST_LESS_THAN_TOLERANCE(pivotValue) ) {
	CoinBigIndex start = startColumn[k];
	CoinBigIndex end = startColumn[k + 1];
	for (CoinBigIndex j = start; j < end; j ++ ) {
	  CoinSimplexInt iRow = indexRow[j];
	  CoinFactorizationDouble result = temp[iRow];
	  CoinFactorizationDouble value = element[j];
	  
	  temp[iRow] = result - value * pivotValue;
	}     
      } else {
	temp[i] = 0.0;
      }       
    }     
  }
#endif
  instrument_start("CoinAbcFactorizationUpdateLSparse",number+nList);
#if 0 //ndef NDEBUG
  char * test = new char[numberRows_];
  memset(test,0,numberRows_);
  for (int i=0;i<numberRows_;i++) {
    if (region[i])
      test[i]=-1;
  }
  for (CoinSimplexInt i=nList-1;i>=0;i--) {
    CoinSimplexInt iPivot = list[i];
    test[iPivot]=1;
    CoinSimplexInt kPivot=pivotLBackwardOrder[iPivot];
    CoinBigIndex start=startColumn[kPivot];
    CoinBigIndex end=startColumn[kPivot+1];
    for (CoinBigIndex j = start;j < end; j ++ ) {
      CoinSimplexInt iRow = indexRow[j];
      assert (test[iRow]<=0);
    }
  }
  delete [] test;
#endif
  for (CoinSimplexInt i=nList-1;i>=0;i--) {
    CoinSimplexInt iPivot = list[i];
#ifndef ABC_ORDERED_FACTORIZATION
    CoinSimplexInt kPivot=pivotLBackwardOrder[iPivot];
#else
    CoinSimplexInt kPivot=iPivot;
#endif
#if DO_CHAR1 // CHAR
    mark[iPivot]=0;
#else
    CoinSimplexUnsignedInt word0 = iPivot >> COINFACTORIZATION_SHIFT_PER_INT;
    //CoinSimplexUnsignedInt bit0 = (iPivot & COINFACTORIZATION_MASK_PER_INT);
    //mark[word0]&=(~(1<<bit0));
    mark[word0]=0;
#endif
    if ( !TEST_LESS_THAN_TOLERANCE( region[iPivot] ) ) {
      CoinFactorizationDouble pivotValue = region[iPivot];
      regionIndex[numberNonZero++]=iPivot;
      CoinBigIndex start=startColumn[kPivot];
      CoinBigIndex end=startColumn[kPivot+1];
      instrument_add(end-start);
      if (TEST_INT_NONZERO(end-start)) {
#ifndef INLINE_IT
	for (CoinBigIndex j = start;j < end; j ++ ) {
	  CoinSimplexInt iRow = indexRow[j];
	  CoinFactorizationDouble value = element[j];
	  region[iRow] -= value * pivotValue;
	}
#else
	CoinAbcScatterUpdate(end-start,pivotValue,element+start,indexRow+start,region);
#endif
      }
    } else {
      region[iPivot]=0.0;
    }
  }
#if 0
  {
    for (CoinSimplexInt k = 0; k < numberRows_; k++ ) {
      assert (fabs(region[k]-temp[k])<1.0e-1);
    }
  }
#endif
  regionSparse->setNumElements ( numberNonZero );
  instrument_end_and_adjust(1.3);
}
#endif
#if FACTORIZATION_STATISTICS
extern double twoThresholdX;
#endif
CoinSimplexInt 
CoinAbcTypeFactorization::updateTwoColumnsFT ( CoinIndexedVector & regionFTX,
				       CoinIndexedVector & regionOtherX)
{
  CoinIndexedVector * regionFT = &regionFTX;
  CoinIndexedVector * regionOther = &regionOtherX;
#if ABC_DEBUG
  regionFT->checkClean();
  regionOther->checkClean();
#endif
  toLongArray(regionFT,0);
  toLongArray(regionOther,1);
#ifdef ABC_ORDERED_FACTORIZATION
  // Permute in for Ftran
  permuteInForFtran(regionFTX);
  permuteInForFtran(regionOtherX);
#endif
  CoinSimplexInt numberNonZero=regionOther->getNumElements();
  CoinSimplexInt numberNonZeroFT=regionFT->getNumElements();
  //  ******* L
  updateColumnL ( regionFT
#if ABC_SMALL<2
		  ,reinterpret_cast<CoinAbcStatistics &>(ftranFTCountInput_)
#endif
		  );
  updateColumnL ( regionOther
#if ABC_SMALL<2
		  ,reinterpret_cast<CoinAbcStatistics &>(ftranCountInput_)
#endif
		  );
  //row bits here
  updateColumnR ( regionFT 
#if ABC_SMALL<2
		  ,reinterpret_cast<CoinAbcStatistics &>(ftranFTCountInput_)
#endif
		  );
  updateColumnR ( regionOther 
#if ABC_SMALL<2
		  ,reinterpret_cast<CoinAbcStatistics &>(ftranCountInput_)
#endif
		  );
  bool doFT=storeFT(regionFT);
  //update counts
  //  ******* U - see if densish
#if ABC_SMALL<2
  // Guess at number at end
  CoinSimplexInt goSparse=0;
  if (gotSparse()) {
    CoinSimplexInt numberNonZero = (regionOther->getNumElements (  )+
			 regionFT->getNumElements())>>1;
    double average = 0.25*(ftranAverageAfterL_*twiddleFtranFactor1() 
			   + ftranFTAverageAfterL_*twiddleFtranFTFactor1());
    assert (average);
    CoinSimplexInt newNumber = static_cast<CoinSimplexInt> (numberNonZero*average);
    if (newNumber< sparseThreshold_)
      goSparse = 2;
  }
#if FACTORIZATION_STATISTICS
  int twoThreshold=twoThresholdX;
#else
#define twoThreshold 1000
#endif
#else
#define goSparse false
#define twoThreshold 1000
#endif
  if (!goSparse&&numberRows_<twoThreshold) {
    CoinFactorizationDouble * COIN_RESTRICT arrayFT = denseVector (regionFT );
    CoinSimplexInt * COIN_RESTRICT indexFT = regionFT->getIndices();
    CoinFactorizationDouble * COIN_RESTRICT arrayUpdate = denseVector (regionOther);
    CoinSimplexInt * COIN_RESTRICT indexUpdate = regionOther->getIndices();
    updateTwoColumnsUDensish(numberNonZeroFT,arrayFT,indexFT,
			     numberNonZero,arrayUpdate,indexUpdate);
    regionFT->setNumElements ( numberNonZeroFT );
    regionOther->setNumElements ( numberNonZero );
  } else {
    // sparse 
    updateColumnU ( regionFT
#if ABC_SMALL<2
		    , reinterpret_cast<CoinAbcStatistics &>(ftranFTCountInput_)
#endif
		    );
    numberNonZeroFT=regionFT->getNumElements();
    updateColumnU ( regionOther
#if ABC_SMALL<2
		    , reinterpret_cast<CoinAbcStatistics &>(ftranCountInput_)
#endif
		    );
    numberNonZero=regionOther->getNumElements();
  }
  fromLongArray(0);
  fromLongArray(1);
#if ABC_DEBUG
  regionFT->checkClean();
  regionOther->checkClean();
#endif
  if ( doFT ) 
    return numberNonZeroFT;
  else 
    return -numberNonZeroFT;
}
// Updates part of 2 columns (FTRANU) real work
void
CoinAbcTypeFactorization::updateTwoColumnsUDensish (
					     CoinSimplexInt & numberNonZero1,
					     CoinFactorizationDouble * COIN_RESTRICT region1, 
					     CoinSimplexInt * COIN_RESTRICT index1,
					     CoinSimplexInt & numberNonZero2,
					     CoinFactorizationDouble * COIN_RESTRICT region2, 
					     CoinSimplexInt * COIN_RESTRICT index2) const
{
#ifdef ABC_USE_FUNCTION_POINTERS
  scatterStruct * COIN_RESTRICT scatterPointer = scatterUColumn();
  CoinFactorizationDouble *  COIN_RESTRICT elementUColumnPlus = elementUColumnPlusAddress_;
#else
  const CoinSimplexInt * COIN_RESTRICT numberInColumn = numberInColumnAddress_;
  const CoinBigIndex * COIN_RESTRICT startColumn = startColumnUAddress_;
  const CoinSimplexInt * COIN_RESTRICT indexRow = indexRowUAddress_;
  const CoinFactorizationDouble * COIN_RESTRICT element = elementUAddress_;
#endif
  CoinSimplexInt numberNonZeroA = 0;
  CoinSimplexInt numberNonZeroB = 0;
  const CoinFactorizationDouble * COIN_RESTRICT pivotRegion = pivotRegionAddress_;
  const CoinSimplexInt *  COIN_RESTRICT pivotLinked = pivotLinkedBackwardsAddress_;
  CoinSimplexInt jRow=pivotLinked[numberRows_];
  instrument_start("CoinAbcFactorizationUpdateTwoUDense",2*numberRows_);
#if 1 //def DONT_USE_SLACKS
  while (jRow!=-1/*lastSlack_*/) {
#else
    // would need extra code
  while (jRow!=lastSlack_) {
#endif
    bool nonZero2 = (TEST_DOUBLE_NONZERO(region2[jRow]));
    bool nonZero1 = (TEST_DOUBLE_NONZERO(region1[jRow]));
#ifndef ABC_USE_FUNCTION_POINTERS
    int numberIn=numberInColumn[jRow];
#else
    scatterStruct & COIN_RESTRICT scatter = scatterPointer[jRow];
    CoinSimplexInt numberIn = scatter.number;
#endif
    if (nonZero1||nonZero2) {
#ifndef ABC_USE_FUNCTION_POINTERS
      CoinBigIndex start = startColumn[jRow];
      const CoinFactorizationDouble * COIN_RESTRICT thisElement = element+start;
      const CoinSimplexInt * COIN_RESTRICT thisIndex = indexRow+start;
#else
      const CoinFactorizationDouble * COIN_RESTRICT thisElement = elementUColumnPlus+scatter.offset;
      const CoinSimplexInt * COIN_RESTRICT thisIndex = reinterpret_cast<const CoinSimplexInt *>(thisElement+numberIn);
#endif
      CoinFactorizationDouble pivotValue2 = region2[jRow];
      CoinFactorizationDouble pivotMult = pivotRegion[jRow];
      assert (pivotMult);
      CoinFactorizationDouble pivotValue2a = pivotValue2 * pivotMult;
      bool do2 = !TEST_LESS_THAN_TOLERANCE_REGISTER( pivotValue2 );
      region2[jRow] = 0.0;
      CoinFactorizationDouble pivotValue1 = region1[jRow];
      region1[jRow] = 0.0;
      CoinFactorizationDouble pivotValue1a = pivotValue1 * pivotMult;
      bool do1 = !TEST_LESS_THAN_TOLERANCE_REGISTER( pivotValue1 );
      if (do2) {
	if ( !TEST_LESS_THAN_TOLERANCE_REGISTER( pivotValue2a ) ) {
	  region2[jRow]=pivotValue2a;
	  index2[numberNonZeroB++]=jRow;
	}
      }
      if (do1) {
	if ( !TEST_LESS_THAN_TOLERANCE_REGISTER( pivotValue1a ) ) {
	  region1[jRow]=pivotValue1a;
	  index1[numberNonZeroA++]=jRow;
	}
      }
      instrument_add(numberIn);
      if (numberIn) { 
	if (do2) {
	  if (!do1) {
	    // just region 2
	    for (CoinBigIndex j=numberIn-1 ; j >=0; j-- ) {
	      CoinSimplexInt iRow = thisIndex[j];
	      CoinFactorizationDouble value = thisElement[j];
	      assert (value);
#ifdef NO_LOAD
	      region2[iRow] -= value * pivotValue2;
#else
	      CoinFactorizationDouble regionValue2 = region2[iRow];
	      region2[iRow] = regionValue2 - value * pivotValue2;
#endif
	    }
	  } else {
	    // both
	    instrument_add(numberIn);
	    for (CoinBigIndex j=numberIn-1 ; j >=0; j-- ) {
	      CoinSimplexInt iRow = thisIndex[j];
	      CoinFactorizationDouble value = thisElement[j];
#ifdef NO_LOAD
	      region1[iRow] -= value * pivotValue1;
	      region2[iRow] -= value * pivotValue2;
#else
	      CoinFactorizationDouble regionValue1 = region1[iRow];
	      CoinFactorizationDouble regionValue2 = region2[iRow];
	      region1[iRow] = regionValue1 - value * pivotValue1;
	      region2[iRow] = regionValue2 - value * pivotValue2;
#endif
	    }
	  }
	} else if (do1 ) {
	  // just region 1
	  for (CoinBigIndex j=numberIn-1 ; j >=0; j-- ) {
	    CoinSimplexInt iRow = thisIndex[j];
	    CoinFactorizationDouble value = thisElement[j];
	    assert (value);
#ifdef NO_LOAD
	    region1[iRow] -= value * pivotValue1;
#else
	    CoinFactorizationDouble regionValue1 = region1[iRow];
	    region1[iRow] = regionValue1 - value * pivotValue1;
#endif
	  }
	}
      } else {
	// no elements in column
      }
    }
    jRow=pivotLinked[jRow];
  }
  numberNonZero1=numberNonZeroA;
  numberNonZero2=numberNonZeroB;
#if ABC_SMALL<2
  if (factorizationStatistics()) {
    ftranFTCountAfterU_ += numberNonZeroA;
    ftranCountAfterU_ += numberNonZeroB;
  }
#endif
  instrument_end();
}
//  updateColumnU.  Updates part of column (FTRANU)
void
CoinAbcTypeFactorization::updateColumnU ( CoinIndexedVector * regionSparse
#if ABC_SMALL<2
					  ,CoinAbcStatistics & statistics
#endif
#if ABC_PARALLEL
					  ,int whichSparse
#endif
					  ) const
{
#if CILK_CONFLICT>0
#if ABC_PARALLEL
  // for conflicts
#if CILK_CONFLICT>1
  printf("file %s line %d which %d\n",__FILE__,__LINE__,whichSparse);
#endif
  abc_assert((cilk_conflict&(1<<whichSparse))==0);
  cilk_conflict |= (1<<whichSparse);
#else
  abc_assert((cilk_conflict&(1<<0))==0);
  cilk_conflict |= (1<<0);
#endif
#endif
#if ABC_SMALL<2
  CoinSimplexInt numberNonZero = regionSparse->getNumElements (  );
  int goSparse;
  // Guess at number at end
  if (gotSparse()) { 
    double average = statistics.averageAfterU_*twiddleFactor1S();
    assert (average);
    CoinSimplexInt newNumber = static_cast<CoinSimplexInt> (numberNonZero*average);
    if (newNumber< sparseThreshold_)
      goSparse = 1;
    else if (numberNonZero*3<numberRows_)
      goSparse = 0;
    else
      goSparse = -1;
  } else {
    goSparse=0;
  }
#endif
  if (!goSparse) {
    // densish
    updateColumnUDensish(regionSparse);
#if ABC_SMALL<2
  } else if (goSparse<0) {
    // dense
    updateColumnUDense(regionSparse);
  } else {
    // sparse
    updateColumnUSparse(regionSparse
#if ABC_PARALLEL
			,whichSparse
#endif
			);
#endif
  }
#if ABC_SMALL<2
  if (factorizationStatistics()) {
    statistics.countAfterU_ += regionSparse->getNumElements (  );
  }
#endif
#if CILK_CONFLICT>0
#if ABC_PARALLEL
  // for conflicts
  abc_assert((cilk_conflict&(1<<whichSparse))!=0);
  cilk_conflict &= ~(1<<whichSparse);
#else
  abc_assert((cilk_conflict&(1<<0))!=0);
  cilk_conflict &= ~(1<<0);
#endif
#endif
}
//#define COUNT_U
#ifdef COUNT_U
static int nUDense[12]={0,0,0,0,0,0,0,0,0,0,0,0};
static int nUSparse[12]={0,0,0,0,0,0,0,0,0,0,0,0};
#endif
//  updateColumnU.  Updates part of column (FTRANU)
// Updates part of column (FTRANU) real work
void
CoinAbcTypeFactorization::updateColumnUDensish (  CoinIndexedVector * regionSparse ) const
{
  instrument_start("CoinAbcFactorizationUpdateUDensish",2*numberRows_);
  CoinSimplexInt * COIN_RESTRICT regionIndex = regionSparse->getIndices();
  CoinFactorizationDouble * COIN_RESTRICT region = denseVector(regionSparse);
  CoinSimplexInt numberNonZero = 0;
  const CoinFactorizationDouble * COIN_RESTRICT pivotRegion = pivotRegionAddress_;
#ifdef ABC_USE_FUNCTION_POINTERS
  scatterStruct * COIN_RESTRICT scatterPointer = scatterUColumn();
  CoinFactorizationDouble *  COIN_RESTRICT elementUColumnPlus = elementUColumnPlusAddress_;
#else
  const CoinSimplexInt * COIN_RESTRICT numberInColumn = numberInColumnAddress_;
  const CoinBigIndex * COIN_RESTRICT startColumn = startColumnUAddress_;
  const CoinSimplexInt * COIN_RESTRICT indexRow = indexRowUAddress_;
  const CoinFactorizationDouble * COIN_RESTRICT element = elementUAddress_;
#endif
  
  const CoinSimplexInt *  COIN_RESTRICT pivotLinked = pivotLinkedBackwardsAddress_;
  CoinSimplexInt jRow=pivotLinked[numberRows_];
#define ETAS_1 2
#define TEST_BEFORE
#ifdef TEST_BEFORE
  CoinExponent expValue=ABC_EXPONENT(region[jRow]);
#endif
#ifdef DONT_USE_SLACKS
  while (jRow!=-1/*lastSlack_*/) {
#else
  while (jRow!=lastSlack_) {
#endif
#ifndef TEST_BEFORE
    CoinExponent expValue=ABC_EXPONENT(region[jRow]);
#endif
    if (expValue) {
      if (!TEST_EXPONENT_LESS_THAN_UPDATE_TOLERANCE(expValue)) {
	CoinFactorizationDouble pivotValue = region[jRow];
#if ETAS_1>1
	CoinFactorizationDouble pivotValue2 = pivotValue*pivotRegion[jRow];
#endif
#ifndef ABC_USE_FUNCTION_POINTERS
	int number=numberInColumn[jRow];
#else
	scatterStruct & COIN_RESTRICT scatter = scatterPointer[jRow];
	CoinSimplexInt number = scatter.number;
#endif
	instrument_add(number);
	if (TEST_INT_NONZERO(number)) {
#ifdef COUNT_U
	  {
	    int k=numberInColumn[jRow];
	    if (k>10)
	      k=11;
	    nUDense[k]++;
	    int kk=0;
	    for (int k=0;k<12;k++)
	      kk+=nUDense[k];
	    if (kk%1000000==0) {
	      printf("ZZ");
	      for (int k=0;k<12;k++)
		printf(" %d",nUDense[k]);
	      printf("\n");
	    }
	  }
#endif
#ifndef ABC_USE_FUNCTION_POINTERS
	  CoinBigIndex start = startColumn[jRow];
#ifndef INLINE_IT
	  const CoinFactorizationDouble *  COIN_RESTRICT thisElement = element+start;
	  const CoinSimplexInt *  COIN_RESTRICT thisIndex = indexRow+start;
	  for (CoinBigIndex j=number-1 ; j >=0; j-- ) {
	    CoinSimplexInt iRow = thisIndex[j];
	    CoinFactorizationDouble regionValue = region[iRow];
	    CoinFactorizationDouble value = thisElement[j];
	    assert (value);
	    region[iRow] = regionValue - value * pivotValue;
	  }
#else
	  CoinAbcScatterUpdate(number,pivotValue,element+start,indexRow+start,region);
#endif
#else
	  CoinBigIndex start = scatter.offset;
#if ABC_USE_FUNCTION_POINTERS
	  (*(scatter.functionPointer))(number,pivotValue,elementUColumnPlus+start,region);
#else
	  CoinAbcScatterUpdate(number,pivotValue,elementUColumnPlus+start,region);
#endif
#endif
	}
	CoinSimplexInt kRow=jRow;
	jRow=pivotLinked[jRow];
#ifdef TEST_BEFORE
	expValue=ABC_EXPONENT(region[jRow]);
#endif
#if ETAS_1<2
	pivotValue *= pivotRegion[kRow];
	if ( !TEST_LESS_THAN_TOLERANCE_REGISTER( pivotValue ) ) {
	  region[kRow]=pivotValue;
	  regionIndex[numberNonZero++]=kRow;
	} else {
	  region[kRow]=0.0;
	}
#else
	if ( !TEST_LESS_THAN_TOLERANCE_REGISTER( pivotValue2 ) ) {
	  region[kRow]=pivotValue2;
	  regionIndex[numberNonZero++]=kRow;
	} else {
	  region[kRow]=0.0;
	}
#endif
      } else {
	CoinSimplexInt kRow=jRow;
	jRow=pivotLinked[jRow];
#ifdef TEST_BEFORE
	expValue=ABC_EXPONENT(region[jRow]);
#endif
	region[kRow]=0.0;
      }
    } else {
      jRow=pivotLinked[jRow];
#ifdef TEST_BEFORE
      expValue=ABC_EXPONENT(region[jRow]);
#endif
    }
  }
#ifndef DONT_USE_SLACKS
  while (jRow!=-1) {
#ifndef TEST_BEFORE
    CoinExponent expValue=ABC_EXPONENT(region[jRow]);
#endif
    if (expValue) {
      if (!TEST_EXPONENT_LESS_THAN_TOLERANCE(expValue)) {
#if SLACK_VALUE==-1
	CoinFactorizationDouble pivotValue = region[jRow];
	assert (pivotRegion[jRow]==SLACK_VALUE);
	region[jRow]=-pivotValue;
#endif
	regionIndex[numberNonZero++]=jRow;
      } else {
	region[jRow] = 0.0;
      }
    }
    jRow=pivotLinked[jRow];
#ifdef TEST_BEFORE
    expValue=ABC_EXPONENT(region[jRow]);
#endif
  }
#endif
  regionSparse->setNumElements ( numberNonZero );
  instrument_end();
}
//  updateColumnU.  Updates part of column (FTRANU)
// Updates part of column (FTRANU) real work
void
CoinAbcTypeFactorization::updateColumnUDense (  CoinIndexedVector * regionSparse ) const
{
  instrument_start("CoinAbcFactorizationUpdateUDensish",2*numberRows_);
  CoinSimplexInt * COIN_RESTRICT regionIndex = regionSparse->getIndices();
  CoinFactorizationDouble * COIN_RESTRICT region = denseVector(regionSparse);
  const CoinSimplexInt * COIN_RESTRICT numberInColumn = numberInColumnAddress_;
  CoinSimplexInt numberNonZero = 0;
  const CoinFactorizationDouble * COIN_RESTRICT pivotRegion = pivotRegionAddress_;
  
#ifdef ABC_USE_FUNCTION_POINTERS
  scatterStruct * COIN_RESTRICT scatterPointer = scatterUColumn();
  CoinFactorizationDouble *  COIN_RESTRICT elementUColumnPlus = elementUColumnPlusAddress_;
#else
  const CoinBigIndex * COIN_RESTRICT startColumn = startColumnUAddress_;
  const CoinSimplexInt * COIN_RESTRICT indexRow = indexRowUAddress_;
  const CoinFactorizationDouble * COIN_RESTRICT element = elementUAddress_;
#endif
  const CoinSimplexInt *  COIN_RESTRICT pivotLinked = pivotLinkedBackwardsAddress_;
  CoinSimplexInt jRow=pivotLinked[numberRows_];
#define ETAS_1 2
#define TEST_BEFORE
#ifdef TEST_BEFORE
  CoinExponent expValue=ABC_EXPONENT(region[jRow]);
#endif
  //int ixxxxxx=0;
#ifdef DONT_USE_SLACKS
  while (jRow!=-1/*lastSlack_*/) {
#else
  while (jRow!=lastSlack_) {
#endif
#if 0
    double largest=1.0;
    int iLargest=-1;
    ixxxxxx++;
    for (int i=0;i<numberRows_;i++) {
      if (fabs(region[i])>largest) {
	largest=fabs(region[i]);
	iLargest=i;
      }
    }
    if (iLargest>=0)
      printf("largest %g on row %d after %d\n",largest,iLargest,ixxxxxx);
#endif
#ifndef TEST_BEFORE
    CoinExponent expValue=ABC_EXPONENT(region[jRow]);
#endif
    if (expValue) {
      if (!TEST_EXPONENT_LESS_THAN_UPDATE_TOLERANCE(expValue)) {
	CoinFactorizationDouble pivotValue = region[jRow];
#if ETAS_1>1
	CoinFactorizationDouble pivotValue2 = pivotValue*pivotRegion[jRow];
#endif
#ifndef ABC_USE_FUNCTION_POINTERS
	int number=numberInColumn[jRow];
#else
	scatterStruct & COIN_RESTRICT scatter = scatterPointer[jRow];
	CoinSimplexInt number = scatter.number;
#endif
	instrument_add(number);
	if (TEST_INT_NONZERO(number)) {
#ifdef COUNT_U
	  {
	    int k=numberInColumn[jRow];
	    if (k>10)
	      k=11;
	    nUDense[k]++;
	    int kk=0;
	    for (int k=0;k<12;k++)
	      kk+=nUDense[k];
	    if (kk%1000000==0) {
	      printf("ZZ");
	      for (int k=0;k<12;k++)
		printf(" %d",nUDense[k]);
	      printf("\n");
	    }
	  }
#endif
#ifndef ABC_USE_FUNCTION_POINTERS
	  CoinBigIndex start = startColumn[jRow];
#ifndef INLINE_IT
	  const CoinFactorizationDouble *  COIN_RESTRICT thisElement = element+start;
	  const CoinSimplexInt *  COIN_RESTRICT thisIndex = indexRow+start;
	  for (CoinBigIndex j=number-1 ; j >=0; j-- ) {
	    CoinSimplexInt iRow = thisIndex[j];
	    CoinFactorizationDouble regionValue = region[iRow];
	    CoinFactorizationDouble value = thisElement[j];
	    assert (value);
	    region[iRow] = regionValue - value * pivotValue;
	  }
#else
	  CoinAbcScatterUpdate(number,pivotValue,element+start,indexRow+start,region);
#endif
#else
	  CoinBigIndex start = scatter.offset;
#if ABC_USE_FUNCTION_POINTERS
	  (*(scatter.functionPointer))(number,pivotValue,elementUColumnPlus+start,region);
#else
	  CoinAbcScatterUpdate(number,pivotValue,elementUColumnPlus+start,region);
#endif
#endif
	}
	CoinSimplexInt kRow=jRow;
	jRow=pivotLinked[jRow];
#ifdef TEST_BEFORE
	expValue=ABC_EXPONENT(region[jRow]);
#endif
#if ETAS_1<2
	pivotValue *= pivotRegion[kRow];
	if ( !TEST_LESS_THAN_TOLERANCE_REGISTER( pivotValue ) ) {
	  region[kRow]=pivotValue;
	  regionIndex[numberNonZero++]=kRow;
	} else {
	  region[kRow]=0.0;
	}
#else
	if ( !TEST_LESS_THAN_TOLERANCE_REGISTER( pivotValue2 ) ) {
	  region[kRow]=pivotValue2;
	  regionIndex[numberNonZero++]=kRow;
	} else {
	  region[kRow]=0.0;
	}
#endif
      } else {
	CoinSimplexInt kRow=jRow;
	jRow=pivotLinked[jRow];
#ifdef TEST_BEFORE
	expValue=ABC_EXPONENT(region[jRow]);
#endif
	region[kRow]=0.0;
      }
    } else {
      jRow=pivotLinked[jRow];
#ifdef TEST_BEFORE
      expValue=ABC_EXPONENT(region[jRow]);
#endif
    }
  }
#ifndef DONT_USE_SLACKS
  while (jRow!=-1) {
#ifndef TEST_BEFORE
    CoinExponent expValue=ABC_EXPONENT(region[jRow]);
#endif
    if (expValue) {
      if (!TEST_EXPONENT_LESS_THAN_TOLERANCE(expValue)) {
#if SLACK_VALUE==-1
	CoinFactorizationDouble pivotValue = region[jRow];
	assert (pivotRegion[jRow]==SLACK_VALUE);
	region[jRow]=-pivotValue;
#endif
	regionIndex[numberNonZero++]=jRow;
      } else {
	region[jRow] = 0.0;
      }
    }
    jRow=pivotLinked[jRow];
#ifdef TEST_BEFORE
    expValue=ABC_EXPONENT(region[jRow]);
#endif
  }
#endif
  regionSparse->setNumElements ( numberNonZero );
  instrument_end();
}
#if ABC_SMALL<2
//  updateColumnU.  Updates part of column (FTRANU)
/*
  Since everything is in order I should be able to do a better job of
  marking stuff - think.  Also as L is static maybe I can do something
  better there (I know I could if I marked the depth of every element
  but that would lead to other inefficiencies.
*/
void
CoinAbcTypeFactorization::updateColumnUSparse ( CoinIndexedVector * regionSparse
#if ABC_PARALLEL
						,int whichSparse
#endif
						) const
{
  CoinSimplexInt * COIN_RESTRICT regionIndex = regionSparse->getIndices();
  CoinFactorizationDouble * COIN_RESTRICT region = denseVector(regionSparse);
  CoinSimplexInt numberNonZero = regionSparse->getNumElements (  );
  //const CoinBigIndex * COIN_RESTRICT startColumn = startColumnUAddress_;
  //const CoinSimplexInt * COIN_RESTRICT numberInColumn = numberInColumnAddress_;
  //const CoinFactorizationDouble * COIN_RESTRICT element = elementUAddress_;
  const CoinFactorizationDouble * COIN_RESTRICT pivotRegion = pivotRegionAddress_;
  // use sparse_ as temporary area
  // mark known to be zero
#define COINFACTORIZATION_SHIFT_PER_INT2 (COINFACTORIZATION_SHIFT_PER_INT-1)
#define COINFACTORIZATION_MASK_PER_INT2 (COINFACTORIZATION_MASK_PER_INT>>1)
  // need to redo if this fails (just means using CoinAbcStack to compute sizes)
  assert (sizeof(CoinSimplexInt)==sizeof(CoinBigIndex));
  CoinAbcStack * COIN_RESTRICT stackList = reinterpret_cast<CoinAbcStack *>(sparseAddress_);
  CoinSimplexInt * COIN_RESTRICT list = listAddress_;
#ifndef ABC_USE_FUNCTION_POINTERS
  const CoinSimplexInt * COIN_RESTRICT indexRow = indexRowUAddress_;
#else
  scatterStruct * COIN_RESTRICT scatterPointer = scatterUColumn();
  const CoinFactorizationDouble *  COIN_RESTRICT elementUColumnPlus = elementUColumnPlusAddress_;
  const CoinSimplexInt *  COIN_RESTRICT indexRow = reinterpret_cast<const CoinSimplexInt *>(elementUColumnPlusAddress_);
#endif
  //#define DO_CHAR2 1
#if DO_CHAR2 // CHAR
  CoinCheckZero * COIN_RESTRICT mark = markRowAddress_;
#else
  //BIT
  CoinSimplexUnsignedInt * COIN_RESTRICT mark = reinterpret_cast<CoinSimplexUnsignedInt *> 
    (markRowAddress_);
#endif
#if ABC_PARALLEL
  //printf("PP %d %d %s\n",whichSparse,__LINE__,__FILE__);
  if (whichSparse) {
    int addAmount=whichSparse*sizeSparseArray_;
    stackList=reinterpret_cast<CoinAbcStack *>(reinterpret_cast<char *>(stackList)+addAmount);
    list=reinterpret_cast<CoinSimplexInt *>(reinterpret_cast<char *>(list)+addAmount);
#if DO_CHAR2 // CHAR
    mark=reinterpret_cast<CoinCheckZero *>(reinterpret_cast<char *>(mark)+addAmount);
#else
    mark=reinterpret_cast<CoinSimplexUnsignedInt *>(reinterpret_cast<char *>(mark)+addAmount);
#endif
  }
#endif
#ifdef COIN_DEBUG
#if DO_CHAR2 // CHAR
  for (CoinSimplexInt i=0;i<maximumRowsExtra_;i++) {
    assert (!mark[i]);
  }
#else
  //BIT
  for (CoinSimplexInt i=0;i<numberRows_>>COINFACTORIZATION_SHIFT_PER_INT;i++) {
    assert (!mark[i]);
  }
#endif
#endif
  CoinSimplexInt nList = 0;
  // move slacks to end of stack list
  int * COIN_RESTRICT putLast = list+numberRows_;
  int * COIN_RESTRICT put = putLast;
  for (CoinSimplexInt i=0;i<numberNonZero;i++) {
    CoinSimplexInt kPivot=regionIndex[i];
#if DO_CHAR2 // CHAR
    CoinCheckZero mark_B = mark[kPivot];
#else
    CoinSimplexUnsignedInt wordB = kPivot >> COINFACTORIZATION_SHIFT_PER_INT2;
    CoinSimplexUnsignedInt bitB = (kPivot & COINFACTORIZATION_MASK_PER_INT2)<<1;
    CoinSimplexUnsignedInt mark_B=(mark[wordB]>>bitB)&3;
#endif
    if (!mark_B) {
#if DO_CHAR2 // CHAR
      mark[kPivot]=1;
#else
      mark[wordB]|=(1<<bitB);
#endif
#ifndef ABC_USE_FUNCTION_POINTERS
      CoinBigIndex start=startColumn[kPivot];
      CoinSimplexInt number=numberInColumn[kPivot];
#else
      scatterStruct & COIN_RESTRICT scatter = scatterPointer[kPivot];
      CoinSimplexInt number = scatter.number;
      CoinBigIndex start = (scatter.offset+number)<<1;
#endif
      stackList[0].stack=kPivot;
      stackList[0].next=start+number;
      stackList[0].start=start;
      CoinSimplexInt nStack=0;
      while (nStack>=0) {
	/* take off stack */
	CoinBigIndex j=stackList[nStack].next-1;
	CoinBigIndex start=stackList[nStack].start;
#if DO_CHAR2==0 // CHAR
	CoinSimplexUnsignedInt word0;
	CoinSimplexUnsignedInt bit0;
#endif
	CoinSimplexInt jPivot;
	for (;j>=start;j--) {
	  jPivot=indexRow[j];
#if DO_CHAR2 // CHAR
	  CoinCheckZero mark_j = mark[jPivot];
#else
	  word0 = jPivot >> COINFACTORIZATION_SHIFT_PER_INT2;
	  bit0 = (jPivot & COINFACTORIZATION_MASK_PER_INT2)<<1;
	  CoinSimplexUnsignedInt mark_j=(mark[word0]>>bit0)&3;
#endif
	  if (!mark_j) 
	    break;
	}
	if (j>=start) {
	  /* put back on stack */
	  stackList[nStack].next =j;
	  /* and new one */
#ifndef ABC_USE_FUNCTION_POINTERS
	  CoinBigIndex start=startColumn[jPivot];
	  CoinSimplexInt number=numberInColumn[jPivot];
#else
	  scatterStruct & COIN_RESTRICT scatter = scatterPointer[jPivot];
	  CoinSimplexInt number = scatter.number;
	  CoinBigIndex start = (scatter.offset+number)<<1;
#endif
	  if (number) {
	    nStack++;
	    stackList[nStack].stack=jPivot;
	    stackList[nStack].next=start+number;
	    stackList[nStack].start=start;
#if DO_CHAR2 // CHAR
	    mark[jPivot]=1;
#else
	    mark[word0]|=(1<<bit0);
#endif
	  } else {
	    // can do at once
#ifndef NDEBUG
#if DO_CHAR2 // CHAR
	    CoinCheckZero mark_j = mark[jPivot];
#else
	    CoinSimplexUnsignedInt mark_j=(mark[word0]>>bit0)&3;
#endif
	    assert (mark_j!=3);
#endif
#if ABC_SMALL<2
	    if (!start) {
	      // slack - put at end
	      --put;
	      *put=jPivot;
	      assert(pivotRegion[jPivot]==1.0);
	    } else {
	      list[nList++]=jPivot;
	    }
#else
	    list[nList++]=jPivot;
#endif
#if DO_CHAR2 // CHAR
	    mark[jPivot]=3;
#else
	    mark[word0]|=(3<<bit0);
#endif
	  }
	} else {
	  /* finished so mark */
	  CoinSimplexInt kPivot=stackList[nStack].stack;
#if DO_CHAR2 // CHAR
	  CoinCheckZero mark_B = mark[kPivot];
#else
	  CoinSimplexUnsignedInt wordB = kPivot >> COINFACTORIZATION_SHIFT_PER_INT2;
	  CoinSimplexUnsignedInt bitB = (kPivot & COINFACTORIZATION_MASK_PER_INT2)<<1;
	  CoinSimplexUnsignedInt mark_B=(mark[wordB]>>bitB)&3;
#endif
	  assert (mark_B!=3);
	  //if (mark_B!=3) {
	    list[nList++]=kPivot;
#if DO_CHAR2 // CHAR
	    mark[kPivot]=3;
#else
	    mark[wordB]|=(3<<bitB);
#endif
	    //}
	  --nStack;
	}
      }
    }
  }
  instrument_start("CoinAbcFactorizationUpdateUSparse",numberNonZero+2*nList);
  numberNonZero=0;
  list[-1]=-1;
  //assert (nList);
  for (CoinSimplexInt i=nList-1;i>=0;i--) {
    CoinSimplexInt iPivot = list[i];
    CoinExponent expValue=ABC_EXPONENT(region[iPivot]);
#if DO_CHAR2 // CHAR
    mark[iPivot]=0;
#else
    CoinSimplexInt word0 = iPivot >> COINFACTORIZATION_SHIFT_PER_INT2;
    mark[word0]=0;
#endif
    if (expValue) {
      if (!TEST_EXPONENT_LESS_THAN_UPDATE_TOLERANCE(expValue)) {
	CoinFactorizationDouble pivotValue = region[iPivot];
#if ETAS_1>1
	CoinFactorizationDouble pivotValue2 = pivotValue*pivotRegion[iPivot];
#endif
#ifndef ABC_USE_FUNCTION_POINTERS
	CoinSimplexInt number = numberInColumn[iPivot];
#else
	scatterStruct & COIN_RESTRICT scatter = scatterPointer[iPivot];
	CoinSimplexInt number = scatter.number;
#endif
	if (TEST_INT_NONZERO(number)) {
#ifdef COUNT_U
	  {
	    int k=numberInColumn[iPivot];
	    if (k>10)
	      k=11;
	    nUSparse[k]++;
	    int kk=0;
	    for (int k=0;k<12;k++)
	      kk+=nUSparse[k];
	    if (kk%1000000==0) {
	      printf("ZZsparse");
	      for (int k=0;k<12;k++)
		printf(" %d",nUSparse[k]);
	      printf("\n");
	    }
	  }
#endif
#ifndef ABC_USE_FUNCTION_POINTERS
	  CoinBigIndex start = startColumn[iPivot];
#else
	  //CoinBigIndex start = startColumn[iPivot];
	  CoinBigIndex start = scatter.offset;
#endif
	  instrument_add(number);
#ifndef INLINE_IT
	  CoinBigIndex j;
	  for ( j = start; j < start+number; j++ ) {
	    CoinFactorizationDouble value = element[j];
	    assert (value);
	    CoinSimplexInt iRow = indexRow[j];
	    region[iRow] -=  value * pivotValue;
	}
#else
#ifdef ABC_USE_FUNCTION_POINTERS
#if ABC_USE_FUNCTION_POINTERS
	  (*(scatter.functionPointer))(number,pivotValue,elementUColumnPlus+start,region);
#else
	  CoinAbcScatterUpdate(number,pivotValue,elementUColumnPlus+start,region);
#endif
#else
	  CoinAbcScatterUpdate(number,pivotValue,element+start,indexRow+start,region);
#endif
#endif
	}
#if ETAS_1<2
	pivotValue *= pivotRegion[iPivot];
	if ( !TEST_LESS_THAN_TOLERANCE_REGISTER( pivotValue ) ) {
	  region[iPivot]=pivotValue;
	  regionIndex[numberNonZero++]=iPivot;
	}
#else
	if ( !TEST_LESS_THAN_TOLERANCE_REGISTER( pivotValue2 ) ) {
	  region[iPivot]=pivotValue2;
	  regionIndex[numberNonZero++]=iPivot;
	} else {
	  region[iPivot]=0.0;
	}
#endif
      } else {
	region[iPivot]=0.0;
      }
    }
  }
#if ABC_SMALL<2
  // slacks
  for (;put<putLast;put++) {
    int iPivot = *put;
    CoinExponent expValue=ABC_EXPONENT(region[iPivot]);
#if DO_CHAR2 // CHAR
    mark[iPivot]=0;
#else
    CoinSimplexInt word0 = iPivot >> COINFACTORIZATION_SHIFT_PER_INT2;
    mark[word0]=0;
#endif
    if (expValue) {
      if (!TEST_EXPONENT_LESS_THAN_UPDATE_TOLERANCE(expValue)) {
	CoinFactorizationDouble pivotValue = region[iPivot];
	if ( !TEST_LESS_THAN_TOLERANCE_REGISTER( pivotValue ) ) {
	  region[iPivot]=pivotValue;
	  regionIndex[numberNonZero++]=iPivot;
	}
      } else {
	region[iPivot]=0.0;
      }
    }
  }
#endif
#ifdef COIN_DEBUG
  for (CoinSimplexInt i=0;i<maximumRowsExtra_>>COINFACTORIZATION_SHIFT_PER_INT2;i++) {
    assert (!mark[i]);
  }
#endif
  regionSparse->setNumElements ( numberNonZero );
  instrument_end_and_adjust(1.3);
}
#endif
// Store update after doing L and R - retuns false if no room
bool 
CoinAbcTypeFactorization::storeFT(
#if ABC_SMALL<3
			      const 
#endif
			      CoinIndexedVector * updateFT)
{
#if ABC_SMALL<3
  CoinSimplexInt numberNonZero = updateFT->getNumElements (  );
#else
  CoinSimplexInt numberNonZero = numberRows_;
#endif
#ifndef ABC_USE_FUNCTION_POINTERS
  if (lengthAreaU_>=lastEntryByColumnU_+numberNonZero) {
#else
    if (lengthAreaUPlus_>=lastEntryByColumnUPlus_+(3*numberNonZero+1)/2) {
    scatterStruct & scatter = scatterUColumn()[numberRows_];
    CoinFactorizationDouble *  COIN_RESTRICT elementUColumnPlus = elementUColumnPlusAddress_;
#endif
#if ABC_SMALL<3
    const CoinFactorizationDouble * COIN_RESTRICT region = denseVector(updateFT);
    const CoinSimplexInt * COIN_RESTRICT regionIndex = updateFT->getIndices();
#else
    CoinSimplexDouble * COIN_RESTRICT region = updateFT->denseVector (  );
#endif
#ifndef ABC_USE_FUNCTION_POINTERS
    CoinBigIndex * COIN_RESTRICT startColumnU = startColumnUAddress_;
    //we are going to save at end of U
    startColumnU[numberRows_] = lastEntryByColumnU_; 
    CoinSimplexInt * COIN_RESTRICT putIndex = indexRowUAddress_ + lastEntryByColumnU_;
    CoinFactorizationDouble * COIN_RESTRICT putElement = elementUAddress_ + lastEntryByColumnU_;
#else
    scatter.offset=lastEntryByColumnUPlus_;
    CoinFactorizationDouble * COIN_RESTRICT putElement = elementUColumnPlus + lastEntryByColumnUPlus_;
    CoinSimplexInt * COIN_RESTRICT putIndex = reinterpret_cast<CoinSimplexInt *>(putElement+numberNonZero);
#endif
#if ABC_SMALL<3
    for (CoinSimplexInt i=0;i<numberNonZero;i++) {
      CoinSimplexInt indexValue = regionIndex[i];
      CoinSimplexDouble value = region[indexValue];
      putElement[i]=value;
      putIndex[i]=indexValue;
    }
#else
    numberNonZero=0;
    for (CoinSimplexInt i=0;i<numberRows_;i++) {
      CoinExponent expValue=ABC_EXPONENT(region[i]);
      if (expValue) {
	if (!TEST_EXPONENT_LESS_THAN_TOLERANCE(expValue)) {
	  putElement[numberNonZero]=region[i];
	  putIndex[numberNonZero++]=i;
	} else {
	  region[i]=0.0;
	}
      }
    }
#endif
#ifndef ABC_USE_FUNCTION_POINTERS
    numberInColumnAddress_[numberRows_] = numberNonZero;
    lastEntryByColumnU_ += numberNonZero;
#else
    scatter.number=numberNonZero;
#endif
    return true;
  } else {
    return false;
  }
}
CoinSimplexInt CoinAbcTypeFactorization::updateColumnFT ( CoinIndexedVector & regionSparseX)
{
  CoinIndexedVector * regionSparse = &regionSparseX;
  CoinSimplexInt numberNonZero=regionSparse->getNumElements();
  toLongArray(regionSparse,0);
#ifdef ABC_ORDERED_FACTORIZATION
  // Permute in for Ftran
  permuteInForFtran(regionSparseX);
#endif
  //  ******* L
  //printf("a\n");
  //regionSparse->checkClean();
  updateColumnL ( regionSparse
#if ABC_SMALL<2
	      ,reinterpret_cast<CoinAbcStatistics &>(ftranFTCountInput_)
#endif
		  );
  //printf("b\n");
  //regionSparse->checkClean();
  //row bits here
  updateColumnR ( regionSparse 
#if ABC_SMALL<2
	      ,reinterpret_cast<CoinAbcStatistics &>(ftranFTCountInput_)
#endif
);

  //printf("c\n");
  //regionSparse->checkClean();
  bool doFT=storeFT(regionSparse);
  //printf("d\n");
  //regionSparse->checkClean();
  //update counts
  //  ******* U
  updateColumnU ( regionSparse
#if ABC_SMALL<2
	      , reinterpret_cast<CoinAbcStatistics &>(ftranFTCountInput_)
#endif
		  );
  //printf("e\n");
#if ABC_DEBUG
  regionSparse->checkClean();
#endif
  numberNonZero=regionSparse->getNumElements();
  // will be negative if no room
  if ( doFT ) 
    return numberNonZero;
  else
    return -numberNonZero;
}
void CoinAbcTypeFactorization::updateColumnFT ( CoinIndexedVector & regionSparseX,
						CoinIndexedVector & partialUpdate,
						int whichSparse)
{
  CoinIndexedVector * regionSparse = &regionSparseX;
  CoinSimplexInt numberNonZero=regionSparse->getNumElements();
  toLongArray(regionSparse,whichSparse);
#ifdef ABC_ORDERED_FACTORIZATION
  // Permute in for Ftran
  permuteInForFtran(regionSparseX);
#endif
  //  ******* L
  //printf("a\n");
  //regionSparse->checkClean();
  updateColumnL ( regionSparse
#if ABC_SMALL<2
	      ,reinterpret_cast<CoinAbcStatistics &>(ftranFTCountInput_)
#endif
#if ABC_PARALLEL
					  ,whichSparse
#endif
		  );
  //printf("b\n");
  //regionSparse->checkClean();
  //row bits here
  updateColumnR ( regionSparse 
#if ABC_SMALL<2
	      ,reinterpret_cast<CoinAbcStatistics &>(ftranFTCountInput_)
#endif
#if ABC_PARALLEL
					  ,whichSparse
#endif
);
  numberNonZero = regionSparse->getNumElements (  );
  CoinSimplexInt * COIN_RESTRICT indexSave=partialUpdate.getIndices();
  CoinSimplexDouble * COIN_RESTRICT regionSave=partialUpdate.denseVector();
  partialUpdate.setNumElements(numberNonZero);
  memcpy(indexSave,regionSparse->getIndices(),numberNonZero*sizeof(CoinSimplexInt));
  partialUpdate.setPackedMode(false);
  CoinFactorizationDouble * COIN_RESTRICT region=denseVector(regionSparse);
  for (int i=0;i<numberNonZero;i++) {
    int iRow=indexSave[i];
    regionSave[iRow]=region[iRow];
  }
  //  ******* U
  updateColumnU ( regionSparse
#if ABC_SMALL<2
	      , reinterpret_cast<CoinAbcStatistics &>(ftranFTCountInput_)
#endif
#if ABC_PARALLEL
					  ,whichSparse
#endif
		  );
  //printf("e\n");
#if ABC_DEBUG
  regionSparse->checkClean();
#endif
}
 int 
  CoinAbcTypeFactorization::updateColumnFTPart1 ( CoinIndexedVector & regionSparse) 
{
  toLongArray(&regionSparse,0);
#ifdef ABC_ORDERED_FACTORIZATION
  // Permute in for Ftran
  permuteInForFtran(regionSparse);
#endif
  //CoinSimplexInt numberNonZero=regionSparse.getNumElements();
  //  ******* L
  //regionSparse.checkClean();
  updateColumnL ( &regionSparse
#if ABC_SMALL<2
	      ,reinterpret_cast<CoinAbcStatistics &>(ftranFTCountInput_)
#endif
#if ABC_PARALLEL
					  ,2
#endif
		  );
  //regionSparse.checkClean();
  //row bits here
  updateColumnR ( &regionSparse 
#if ABC_SMALL<2
	      ,reinterpret_cast<CoinAbcStatistics &>(ftranFTCountInput_)
#endif
#if ABC_PARALLEL
					  ,2
#endif
);

  //regionSparse.checkClean();
  bool doFT=storeFT(&regionSparse);
  // will be negative if no room
  if ( doFT ) 
    return 1;
  else
    return -1;
}
 void
CoinAbcTypeFactorization::updateColumnFTPart2 ( CoinIndexedVector & regionSparse) 
{
  toLongArray(&regionSparse,0);
  //CoinSimplexInt numberNonZero=regionSparse.getNumElements();
  //  ******* U
  updateColumnU ( &regionSparse
#if ABC_SMALL<2
	      , reinterpret_cast<CoinAbcStatistics &>(ftranFTCountInput_)
#endif
#if ABC_PARALLEL
					  ,2
#endif
		  );
#if ABC_DEBUG
  regionSparse.checkClean();
#endif
}
/* Updates one column (FTRAN) */
void 
CoinAbcTypeFactorization::updateColumnCpu ( CoinIndexedVector & regionSparse,int whichCpu) const
{
  toLongArray(&regionSparse,whichCpu);
#ifdef ABC_ORDERED_FACTORIZATION
  // Permute in for Ftran
  permuteInForFtran(regionSparse);
#endif
  //  ******* L
  updateColumnL ( &regionSparse
#if ABC_SMALL<2
		  ,reinterpret_cast<CoinAbcStatistics &>(ftranCountInput_)
#endif
#if ABC_PARALLEL
		  ,whichCpu
#endif
		  );
  //row bits here
  updateColumnR ( &regionSparse 
#if ABC_SMALL<2
		  ,reinterpret_cast<CoinAbcStatistics &>(ftranCountInput_) 
#endif
#if ABC_PARALLEL
		  ,whichCpu
#endif
		  );
  //update counts
  //  ******* U
  updateColumnU ( &regionSparse
#if ABC_SMALL<2
		  , reinterpret_cast<CoinAbcStatistics &>(ftranCountInput_)
#endif
#if ABC_PARALLEL
		  ,whichCpu
#endif
		  );
  fromLongArray(whichCpu);
#if ABC_DEBUG
  regionSparse.checkClean();
#endif
}
/* Updates one column (BTRAN) */
 void 
CoinAbcTypeFactorization::updateColumnTransposeCpu ( CoinIndexedVector & regionSparse,int whichCpu) const
 {
  toLongArray(&regionSparse,whichCpu);
     
  CoinSimplexDouble * COIN_RESTRICT region = regionSparse.denseVector();
  CoinSimplexInt * COIN_RESTRICT regionIndex = regionSparse.getIndices();
  CoinSimplexInt numberNonZero = regionSparse.getNumElements();
  //#if COIN_BIG_DOUBLE==1
  //const CoinFactorizationDouble * COIN_RESTRICT pivotRegion = pivotRegion_.array()+1;
  //#else
  const CoinFactorizationDouble * COIN_RESTRICT pivotRegion = pivotRegionAddress_;
  //#endif

#ifndef ABC_ORDERED_FACTORIZATION
  // can I move this
#ifndef INLINE_IT3
  for (CoinSimplexInt i = 0; i < numberNonZero; i ++ ) {
    CoinSimplexInt iRow = regionIndex[i];
    CoinSimplexDouble value = region[iRow];
    value *= pivotRegion[iRow];
    region[iRow] = value;
  }
#else
  multiplyIndexed(numberNonZero,pivotRegion,
		  regionIndex,region);
#endif
#else
  // Permute in for Btran
  permuteInForBtranAndMultiply(regionSparse);
#endif
  //  ******* U
  // Apply pivot region - could be combined for speed
  // Can only combine/move inside vector for sparse
  CoinSimplexInt smallestIndex=pivotLinkedForwardsAddress_[-1];
#if ABC_SMALL<2
  // copy of code inside transposeU
  bool goSparse=false;
#else
#define goSparse false
#endif
#if ABC_SMALL<2
  // Guess at number at end
  if (gotUCopy()) {
    assert (btranAverageAfterU_);
    CoinSimplexInt newNumber = 
      static_cast<CoinSimplexInt> (numberNonZero*btranAverageAfterU_*twiddleBtranFactor1());
    if (newNumber< sparseThreshold_)
      goSparse = true;
  }
#endif
  if (numberNonZero<40&&(numberNonZero<<4)<numberRows_&&!goSparse) {
    CoinSimplexInt *  COIN_RESTRICT pivotRowForward = pivotColumnAddress_;
    CoinSimplexInt smallest=numberRowsExtra_;
    for (CoinSimplexInt j = 0; j < numberNonZero; j++ ) {
      CoinSimplexInt iRow = regionIndex[j];
      if (pivotRowForward[iRow]<smallest) {
	smallest=pivotRowForward[iRow];
	smallestIndex=iRow;
      }
    }
  }
  updateColumnTransposeU ( &regionSparse,smallestIndex 
#if ABC_SMALL<2
		  ,reinterpret_cast<CoinAbcStatistics &>(btranCountInput_)
#endif
#if ABC_PARALLEL
		  ,whichCpu
#endif
			   );
  //row bits here
  updateColumnTransposeR ( &regionSparse 
#if ABC_SMALL<2
		  ,reinterpret_cast<CoinAbcStatistics &>(btranCountInput_)
#endif
			   );
  //  ******* L
  updateColumnTransposeL ( &regionSparse 
#if ABC_SMALL<2
		  ,reinterpret_cast<CoinAbcStatistics &>(btranCountInput_)
#endif
#if ABC_PARALLEL
			   , whichCpu
#endif
			   );
#if ABC_SMALL<3
#ifdef ABC_ORDERED_FACTORIZATION
  // Permute out for Btran
  permuteOutForBtran(regionSparse);
#endif
#if ABC_DEBUG
  regionSparse.checkClean();
#endif
  numberNonZero = regionSparse.getNumElements (  );
#else
  numberNonZero=0;
  for (CoinSimplexInt i=0;i<numberRows_;i++) {
    CoinExponent expValue=ABC_EXPONENT(region[i]);
    if (expValue) {
      if (!TEST_EXPONENT_LESS_THAN_TOLERANCE(expValue)) {
	regionIndex[numberNonZero++]=i;
      } else {
	region[i]=0.0;
      }
    }
  }
  regionSparse.setNumElements(numberNonZero);
#endif
 }
#if ABC_SMALL<2
//  getRowSpaceIterate.  Gets space for one Row with given length
//may have to do compression  (returns true)
//also moves existing vector
bool
CoinAbcTypeFactorization::getRowSpaceIterate ( CoinSimplexInt iRow,
                                      CoinSimplexInt extraNeeded )
{
  const CoinSimplexInt *  COIN_RESTRICT numberInRow = numberInRowAddress_;
  CoinSimplexInt number = numberInRow[iRow];
  CoinBigIndex * COIN_RESTRICT startRow = startRowUAddress_;
  CoinSimplexInt * COIN_RESTRICT indexColumnU = indexColumnUAddress_;
#if CONVERTROW
  CoinBigIndex * COIN_RESTRICT convertRowToColumn = convertRowToColumnUAddress_;
#if CONVERTROW>2
  CoinBigIndex * COIN_RESTRICT convertColumnToRow = convertColumnToRowUAddress_;
#endif
#endif
  CoinFactorizationDouble * COIN_RESTRICT elementRowU = elementRowUAddress_;
  CoinBigIndex space = lengthAreaU_ - lastEntryByRowU_;
  CoinSimplexInt * COIN_RESTRICT nextRow = nextRowAddress_;
  CoinSimplexInt * COIN_RESTRICT lastRow = lastRowAddress_;
  if ( space < extraNeeded + number + 2 ) {
    //compression
    CoinSimplexInt iRow = nextRow[numberRows_];
    CoinBigIndex put = 0;
    while ( iRow != numberRows_ ) {
      //move
      CoinBigIndex get = startRow[iRow];
      CoinBigIndex getEnd = startRow[iRow] + numberInRow[iRow];
      
      startRow[iRow] = put;
      CoinBigIndex i;
      for ( i = get; i < getEnd; i++ ) {
	indexColumnU[put] = indexColumnU[i];
#if CONVERTROW
	convertRowToColumn[put] = convertRowToColumn[i];
#if CONVERTROW>2
	convertColumnToRow[i] = convertColumnToRow[put];
#endif
#endif
	elementRowU[put] = elementRowU[i];
	put++;
      }       
      iRow = nextRow[iRow];
    }       /* endwhile */
    numberCompressions_++;
    lastEntryByRowU_ = put;
    space = lengthAreaU_ - put;
    if ( space < extraNeeded + number + 2 ) {
      //need more space
      //if we can allocate bigger then do so and copy
      //if not then return so code can start again
      status_ = -99;
      return false;    }       
  }       
  CoinBigIndex put = lastEntryByRowU_;
  CoinSimplexInt next = nextRow[iRow];
  CoinSimplexInt last = lastRow[iRow];
  
  //out
  nextRow[last] = next;
  lastRow[next] = last;
  //in at end
  last = lastRow[numberRows_];
  nextRow[last] = iRow;
  lastRow[numberRows_] = iRow;
  lastRow[iRow] = last;
  nextRow[iRow] = numberRows_;
  //move
  CoinBigIndex get = startRow[iRow];
  startRow[iRow] = put;
  while ( number ) {
    number--;
    indexColumnU[put] = indexColumnU[get];
#if CONVERTROW
    convertRowToColumn[put] = convertRowToColumn[get];
#if CONVERTROW>2
    convertColumnToRow[get] = convertColumnToRow[put];
#endif
#endif
    elementRowU[put] = elementRowU[get];
    put++;
    get++;
  }       /* endwhile */
  //add four for luck
  lastEntryByRowU_ = put + extraNeeded + 4;
  return true;
}
#endif
void 
CoinAbcTypeFactorization::printRegion(const CoinIndexedVector & vector,const char * where) const
{
  //return;
  CoinSimplexInt numberNonZero = vector.getNumElements (  );
  //CoinSimplexInt * COIN_RESTRICT regionIndex = vector.getIndices (  );
  const CoinSimplexDouble * COIN_RESTRICT region = vector.denseVector (  );
  int n=0;
  for (int i=0;i<numberRows_;i++) {
    if (region[i])
      n++;
  }
  printf("==== %d nonzeros (%d in count) %s ====\n",n,numberNonZero,where);
  for (int i=0;i<numberRows_;i++) {
    if (region[i])
      printf("%d %g\n",i,region[i]);
  }
  printf("====            %s ====\n",where);
}
void 
CoinAbcTypeFactorization::unpack ( CoinIndexedVector * regionFrom,
			   CoinIndexedVector * regionTo) const
{
  // move 
  CoinSimplexInt * COIN_RESTRICT regionIndex = regionTo->getIndices (  );
  CoinSimplexInt numberNonZero = regionFrom->getNumElements();
  CoinSimplexInt * COIN_RESTRICT index = regionFrom->getIndices();
  CoinSimplexDouble * COIN_RESTRICT region = regionTo->denseVector();
  CoinSimplexDouble * COIN_RESTRICT array = regionFrom->denseVector();
  for (CoinSimplexInt j = 0; j < numberNonZero; j ++ ) {
    CoinSimplexInt iRow = index[j];
    CoinSimplexDouble value = array[j];
    array[j]=0.0;
    region[iRow] = value;
    regionIndex[j] = iRow;
  }
  regionTo->setNumElements(numberNonZero);
}
void 
CoinAbcTypeFactorization::pack ( CoinIndexedVector * regionFrom,
			 CoinIndexedVector * regionTo) const
{
  // move 
  CoinSimplexInt * COIN_RESTRICT regionIndex = regionFrom->getIndices (  );
  CoinSimplexInt numberNonZero = regionFrom->getNumElements();
  CoinSimplexInt * COIN_RESTRICT index = regionTo->getIndices();
  CoinSimplexDouble * COIN_RESTRICT region = regionFrom->denseVector();
  CoinSimplexDouble * COIN_RESTRICT array = regionTo->denseVector();
  for (CoinSimplexInt j = 0; j < numberNonZero; j ++ ) {
    CoinSimplexInt iRow = regionIndex[j];
    CoinSimplexDouble value = region[iRow];
    region[iRow]=0.0;
    array[j] = value;
    index[j] = iRow;
  }
  regionTo->setNumElements(numberNonZero);
}
// Set up addresses from arrays
void 
CoinAbcTypeFactorization::doAddresses()
{
  pivotColumnAddress_ = pivotColumn_.array();
  permuteAddress_ = permute_.array();
  pivotRegionAddress_ = pivotRegion_.array()+1;
  elementUAddress_ = elementU_.array();
  indexRowUAddress_ = indexRowU_.array();
  numberInColumnAddress_ = numberInColumn_.array();
  numberInColumnPlusAddress_ = numberInColumnPlus_.array();
#ifdef ABC_USE_FUNCTION_POINTERS
  scatterPointersUColumnAddress_ = reinterpret_cast<scatterStruct *>(scatterUColumn_.array());
#endif
  startColumnUAddress_ = startColumnU_.array();
#if CONVERTROW
  convertRowToColumnUAddress_ = convertRowToColumnU_.array();
#if CONVERTROW>1
  convertColumnToRowUAddress_ = convertColumnToRowU_.array();
#endif
#endif
#if ABC_SMALL<2
  elementRowUAddress_ = elementRowU_.array();
#endif
  startRowUAddress_ = startRowU_.array();
  numberInRowAddress_ = numberInRow_.array();
  indexColumnUAddress_ = indexColumnU_.array();
  firstCountAddress_ = firstCount_.array();
  nextCountAddress_ = nextCount();
  lastCountAddress_ = lastCount();
  nextColumnAddress_ = nextColumn_.array();
  lastColumnAddress_ = lastColumn_.array();
  nextRowAddress_ = nextRow_.array();
  lastRowAddress_ = lastRow_.array();
  saveColumnAddress_ = saveColumn_.array();
  //saveColumnAddress2_ = saveColumn_.array()+numberRows_;
  elementLAddress_ = elementL_.array();
  indexRowLAddress_ = indexRowL_.array();
  startColumnLAddress_ = startColumnL_.array();
#if ABC_SMALL<2
  startRowLAddress_ = startRowL_.array();
#endif
  pivotLinkedBackwardsAddress_ = pivotLinkedBackwards();
  pivotLinkedForwardsAddress_ = pivotLinkedForwards();
  pivotLOrderAddress_ = pivotLOrder();
  startColumnRAddress_ = startColumnR();
  // next two are recomputed cleanup
  elementRAddress_ = elementL_.array() + lengthL_;
  indexRowRAddress_ = indexRowL_.array() + lengthL_;
#if ABC_SMALL<2
  indexColumnLAddress_ = indexColumnL_.array();
  elementByRowLAddress_ = elementByRowL_.array();
#endif
#if ABC_DENSE_CODE
  denseAreaAddress_ = denseArea_.array();
#endif
  workAreaAddress_ = workArea_.array();
  workArea2Address_ = workArea2_.array();
#if ABC_SMALL<2
  sparseAddress_ = sparse_.array();
  CoinAbcStack * stackList = reinterpret_cast<CoinAbcStack *>(sparseAddress_);
  listAddress_ = reinterpret_cast<CoinSimplexInt *>(stackList+numberRows_);
  markRowAddress_ = reinterpret_cast<CoinCheckZero *> (listAddress_ + numberRows_);
#endif
}
#endif
