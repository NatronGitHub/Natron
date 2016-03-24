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
#if ABC_NORMAL_DEBUG>0
// for conflicts
extern int cilk_conflict;
#endif
#define UNROLL 1
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
#undef INLINE_IT
//  updateColumnR.  Updates part of column (FTRANR)
void
CoinAbcTypeFactorization::updateColumnR ( CoinIndexedVector * regionSparse 
#if ABC_SMALL<2
				      , CoinAbcStatistics & statistics
#endif
#if ABC_PARALLEL
					  ,int whichSparse
#endif
				      ) const
{

  if ( numberR_ ) {
    CoinSimplexDouble * COIN_RESTRICT region = regionSparse->denseVector (  );
#if ABC_SMALL<3
    CoinSimplexInt * COIN_RESTRICT regionIndex = regionSparse->getIndices (  );
    CoinSimplexInt numberNonZero = regionSparse->getNumElements (  );
#endif
    
    const CoinBigIndex *  COIN_RESTRICT startColumn = startColumnRAddress_-numberRows_;
    const CoinSimplexInt *  COIN_RESTRICT indexRow = indexRowRAddress_;
    CoinSimplexInt * COIN_RESTRICT indexRowR2 = indexRowRAddress_+lengthAreaR_;
    if (gotRCopy()) 
      indexRowR2 += lengthAreaR_;
    const CoinFactorizationDouble *  COIN_RESTRICT element = elementRAddress_;
    const CoinSimplexInt *  COIN_RESTRICT permute = permuteAddress_;
    instrument_do("CoinAbcFactorizationUpdateR",sizeR);
#if ABC_SMALL<2
    // Size of R
    CoinSimplexDouble sizeR=startColumnRAddress_[numberR_];
    // Work out very dubious idea of what would be fastest
    // Average
    CoinSimplexDouble averageR = sizeR/(static_cast<CoinSimplexDouble> (numberRowsExtra_));
    // weights (relative to actual work)
    CoinSimplexDouble setMark = 0.1; // setting mark
    CoinSimplexDouble test1= 1.0; // starting ftran (without testPivot)
    CoinSimplexDouble testPivot = 2.0; // Seeing if zero etc
    CoinSimplexDouble startDot=2.0; // For starting dot product version
    // For final scan
    CoinSimplexDouble final = numberNonZero*1.0;
    CoinSimplexDouble methodTime0;
    // For first type
    methodTime0 = numberPivots_ * (testPivot + ((static_cast<double> (numberNonZero))/(static_cast<double> (numberRows_))
						* averageR));
    methodTime0 += numberNonZero *(test1 + averageR);
    methodTime0 += (numberNonZero+numberPivots_)*setMark;
    // third
    CoinSimplexDouble methodTime2 = sizeR + numberPivots_*startDot + numberNonZero*final;
    // switch off if necessary
    CoinSimplexInt method=0;
    if (!gotRCopy()||!gotSparse()||methodTime2<methodTime0) {
      method=2;
    } 
    /* 
       IF we have row copy of R then always store in old way -
       otherwise unpermuted way.
    */
#endif
#if ABC_SMALL<2
    const CoinSimplexInt * numberInColumnPlus = numberInColumnPlusAddress_;
#endif
    const CoinSimplexInt * pivotRowBack = pivotColumnAddress_;
#if ABC_SMALL<2
    switch (method) {
    case 0:
      {
	instrument_start("CoinAbcFactorizationUpdateRSparse1",numberRows_);
	// mark known to be zero
	CoinCheckZero * COIN_RESTRICT mark = markRowAddress_;
#if ABC_PARALLEL
	if (whichSparse) {
	  mark=reinterpret_cast<CoinCheckZero *>(reinterpret_cast<char *>(mark)+whichSparse*sizeSparseArray_);
	}
#endif
	// mark all rows which will be permuted
	for (CoinSimplexInt i = numberRows_; i < numberRowsExtra_; i++ ) {
	  CoinSimplexInt iRow = permute[i];
	  mark[iRow]=1;
	}
	// we have another copy of R in R
	const CoinFactorizationDouble *  COIN_RESTRICT elementR = elementRAddress_ + lengthAreaR_;
	const CoinSimplexInt *  COIN_RESTRICT indexRowR = indexRowRAddress_ + lengthAreaR_;
	const CoinBigIndex *  COIN_RESTRICT startR = startColumnRAddress_+maximumPivots_+1;
	// For current list order does not matter as
	// only affects end
	CoinSimplexInt newNumber=0;
	for (CoinSimplexInt i = 0; i < numberNonZero; i++ ) {
	  CoinSimplexInt iRow = regionIndex[i];
	  CoinFactorizationDouble pivotValue = region[iRow];
	  assert (region[iRow]);
	  if (!mark[iRow]) {
	    regionIndex[newNumber++]=iRow;
	  }
	  CoinSimplexInt kRow=permute[iRow];
	  CoinSimplexInt number = numberInColumnPlus[kRow];
	  instrument_add(number);
	  if (TEST_INT_NONZERO(number)) {
	    CoinBigIndex start=startR[kRow];
#ifndef INLINE_IT
	    CoinBigIndex end = start+number;
	    for (CoinBigIndex j = start; j < end; j ++ ) {
	      CoinFactorizationDouble value = elementR[j];
	      CoinSimplexInt jRow = indexRowR[j];
	      region[jRow] -= pivotValue*value;
	    }
#else
	    CoinAbcScatterUpdate(number,pivotValue,elementR+start,indexRowR+start,region);
#endif
	  }
	}
	instrument_start("CoinAbcFactorizationUpdateRSparse2",numberRows_);
	numberNonZero = newNumber;
	for (CoinSimplexInt i = numberRows_; i < numberRowsExtra_; i++ ) {
	  //move using permute_ (stored in inverse fashion)
	  CoinSimplexInt iRow = permute[i];
	  CoinFactorizationDouble pivotValue = region[iRow]+region[i];
	  //zero out pre-permuted
	  region[iRow] = 0.0;
	  region[i]=0.0;
	  if ( !TEST_LESS_THAN_TOLERANCE_REGISTER(pivotValue )  ) {
	    int jRow=pivotRowBack[i];
	    assert (!region[jRow]);
	    region[jRow]=pivotValue;
	    CoinSimplexInt number = numberInColumnPlus[i];
	    instrument_add(number);
	    if (TEST_INT_NONZERO(number)) {
	      CoinBigIndex start=startR[i];
#ifndef INLINE_IT
	      CoinBigIndex end = start+number;
	      for (CoinBigIndex j = start; j < end; j ++ ) {
		CoinFactorizationDouble value = elementR[j];
		CoinSimplexInt jRow = indexRowR[j];
		region[jRow] -= pivotValue*value;
	      }
#else
	      CoinAbcScatterUpdate(number,pivotValue,elementR+start,indexRowR+start,region);
#endif
	    }
	  }
	}
	instrument_end();
	for (CoinSimplexInt i = numberRows_; i < numberRowsExtra_; i++ ) {
	  CoinSimplexInt iRow = permute[i];
	  assert (iRow<numberRows_);
	  if (mark[iRow]) {
	    mark[iRow]=0;
	    CoinExponent expValue=ABC_EXPONENT(region[iRow]);
	    if (expValue) {
	      if (!TEST_EXPONENT_LESS_THAN_TOLERANCE(expValue)) {
		regionIndex[numberNonZero++]=iRow;
	      } else {
		region[iRow]=0.0;
	      }
	    }
	  }
	}
      }
      break;
    case 2:
#endif
      {
	instrument_start("CoinAbcFactorizationUpdateRDense",numberRows_);
	CoinBigIndex start = startColumn[numberRows_];
	for (CoinSimplexInt i = numberRows_; i < numberRowsExtra_; i++ ) {
	  //move using permute_ (stored in inverse fashion)
	  CoinBigIndex end = startColumn[i+1];
	  CoinSimplexInt iRow = pivotRowBack[i];
	  assert (iRow<numberRows_);
	  CoinFactorizationDouble pivotValue = region[iRow];
	  instrument_add(end-start);
	  if (TEST_INT_NONZERO(end-start)) {
#ifndef INLINE_IT2
	    for (CoinBigIndex j = start; j < end; j ++ ) {
	      CoinFactorizationDouble value = element[j];
	      CoinSimplexInt jRow = indexRow[j];
	      value *= region[jRow];
	      pivotValue -= value;
	    }
#else
	    pivotValue+=CoinAbcGatherUpdate(end-start,element+start,indexRow+start,region);
#endif
	    start=end;
	  }
#if ABC_SMALL<3
	  if ( !TEST_LESS_THAN_TOLERANCE_REGISTER(pivotValue ) ) {
	    if (!region[iRow])
	      regionIndex[numberNonZero++] = iRow;
	    region[iRow] = pivotValue;
	  } else {
	    if (region[iRow])
	      region[iRow] = COIN_INDEXED_REALLY_TINY_ELEMENT;
	  }
#else
	    region[iRow] = pivotValue;
#endif
	}
	instrument_end();
#if ABC_SMALL<3
	// pack down
	CoinSimplexInt n=numberNonZero;
	numberNonZero=0;
	for (CoinSimplexInt i=0;i<n;i++) {
	  CoinSimplexInt indexValue = regionIndex[i];
	  CoinExponent expValue=ABC_EXPONENT(region[indexValue]);
	  if (expValue) {
	    if (!TEST_EXPONENT_LESS_THAN_TOLERANCE(expValue)) {
	      regionIndex[numberNonZero++]=indexValue; // needed? is region used
	    } else {
	      region[indexValue]=0.0;
	    }
	  }
	}
#endif
      }
#if ABC_SMALL<2
      break;
    }
#endif
#if ABC_SMALL<3
    //set counts
    regionSparse->setNumElements ( numberNonZero );
#endif
  }
#if ABC_SMALL<2
  if (factorizationStatistics()) 
    statistics.countAfterR_ += regionSparse->getNumElements();
#endif
}
#ifdef EARLY_FACTORIZE
#include "AbcSimplex.hpp"
#include "AbcMatrix.hpp"
// Returns -2 if can't, -1 if singular, -99 memory, 0 OK
int 
CoinAbcTypeFactorization::factorize (AbcSimplex * model, CoinIndexedVector & stuff)
{
  AbcMatrix * matrix = model->abcMatrix();
  setStatus(-99);
  int * COIN_RESTRICT pivotTemp = stuff.getIndices();
  int numberColumnBasic = stuff.getNumElements();
  // compute how much in basis
  int numberSlacks = numberRows_ - numberColumnBasic;
  
  CoinBigIndex numberElements = numberSlacks+matrix->countBasis(pivotTemp + numberSlacks,
					 numberColumnBasic);
  // Not needed for dense
  numberElements = 3 * numberRows_ + 3 * numberElements + 20000;
  getAreas ( numberRows_, numberSlacks + numberColumnBasic, numberElements,
	     2 * numberElements );
  numberSlacks_=numberSlacks;
  // Fill in counts so we can skip part of preProcess
  // This is NOT needed for dense but would be needed for later versions
  CoinFactorizationDouble *  COIN_RESTRICT elementU;
  int *  COIN_RESTRICT indexRowU;
  CoinBigIndex *  COIN_RESTRICT startColumnU;
  int *  COIN_RESTRICT numberInRow;
  int *  COIN_RESTRICT numberInColumn;
  elementU = this->elements();
  indexRowU = this->indices();
  startColumnU = this->starts();
  numberInRow = this->numberInRow();
  numberInColumn = this->numberInColumn();
  CoinZeroN ( numberInRow, numberRows_+1  );
  CoinZeroN ( numberInColumn, numberRows_ );
  for (int i = 0; i < numberSlacks_; i++) {
    int iRow = pivotTemp[i];
    indexRowU[i] = iRow;
    startColumnU[i] = i;
    elementU[i] = 1.0;
    numberInRow[iRow] = 1;
    numberInColumn[i] = 1;
  }
  startColumnU[numberSlacks_] = numberSlacks_;
  matrix->fillBasis(pivotTemp + numberSlacks_,
		    numberColumnBasic,
		    indexRowU,
		    startColumnU + numberSlacks_,
		    numberInRow,
		    numberInColumn + numberSlacks_,
		    elementU);
  numberElements = startColumnU[numberRows_-1]
    + numberInColumn[numberRows_-1];
  preProcess ( );
  factor (model);
  if (status() == 0 ) {
    // Put sequence numbers in workArea
    int * savePivots = reinterpret_cast<int *>(workAreaAddress_);
    CoinAbcMemcpy(savePivots,pivotTemp,numberRows_);
    postProcess(pivotTemp, savePivots);
    return 0;
  } else {
    return -2;
  }
}
// 0 success, -1 can't +1 accuracy problems
int 
CoinAbcTypeFactorization::replaceColumns ( const AbcSimplex * model,
					   CoinIndexedVector & stuff,
					   int firstPivot,int lastPivot,
					   bool cleanUp)
{
  // Could skip some if goes in and out (only if easy as then no alpha check)
  // Put sequence numbers in workArea
  int * savePivots = reinterpret_cast<int *>(workAreaAddress_);
  const int * pivotIndices = stuff.getIndices();
  CoinSimplexDouble * pivotValues = stuff.denseVector();
  int savePivot = stuff.capacity();
  savePivot--;
  savePivot -= 2*firstPivot;
  int numberDo=lastPivot-firstPivot;
  // Say clear
  stuff.setNumElements(0);
  bool badUpdates=false;
  for (int iPivot=0;iPivot<numberDo;iPivot++) {
    int sequenceOut = pivotIndices[--savePivot];
    CoinSimplexDouble alpha=pivotValues[savePivot];
    int sequenceIn = pivotIndices[--savePivot];
    CoinSimplexDouble btranAlpha=pivotValues[savePivot];
    int pivotRow;
    for (pivotRow=0;pivotRow<numberRows_;pivotRow++) {
      if (sequenceOut==savePivots[pivotRow])
	break;
    }
    assert (pivotRow<numberRows_);
    savePivots[pivotRow]=sequenceIn;
    model->unpack(stuff,sequenceIn);
    updateColumnFTPart1(stuff);
    stuff.clear();
    //checkReplacePart1a(&stuff,pivotRow);
    CoinSimplexDouble ftAlpha=checkReplacePart1(&stuff,pivotRow);
    // may need better check
    if (checkReplacePart2(pivotRow,btranAlpha,alpha,ftAlpha)>1) {
      badUpdates=true;
      printf("Inaccuracy ? btranAlpha %g ftranAlpha %g ftAlpha %g\n",
	     btranAlpha,alpha,ftAlpha);
      break;
    }
    replaceColumnPart3(model,&stuff,NULL,pivotRow,ftAlpha);
  }
  int flag;
  if (!badUpdates) {
    flag=1;
    if (cleanUp) {
      CoinAbcMemcpy(model->pivotVariable(),savePivots,numberRows_);
    }
  } else {
    flag=-1;
  }
  stuff.setNumElements(flag);
  return flag >0 ? 0 : flag;
}
#endif
#ifdef ABC_ORDERED_FACTORIZATION
// Permute in for Ftran
void 
CoinAbcTypeFactorization::permuteInForFtran(CoinIndexedVector & regionSparse,
					    bool full) const
{
  int numberNonZero=regionSparse.getNumElements();
  const int * COIN_RESTRICT permuteIn = permuteAddress_+maximumRowsExtra_+1;
  double * COIN_RESTRICT region = regionSparse.denseVector();
  double * COIN_RESTRICT tempRegion = region+numberRows_;
  int * COIN_RESTRICT index = regionSparse.getIndices();
  if ((numberNonZero<<1)>numberRows_||full) {
    CoinAbcMemcpy(tempRegion,region,numberRows_);
    CoinAbcMemset0(region,numberRows_);
    numberNonZero=0;
    for (int i=0;i<numberRows_;i++) {
      double value=tempRegion[i];
      if (value) {
	tempRegion[i]=0.0;
	int iRow=permuteIn[i];
	region[iRow]=value;
	index[numberNonZero++]=iRow;
      }
    }
    regionSparse.setNumElements(numberNonZero);
  } else {
    for (int i=0;i<numberNonZero;i++) {
      int iRow=index[i];
      double value=region[iRow];
      region[iRow]=0.0;
      iRow=permuteIn[iRow];
      tempRegion[iRow]=value;
      index[i]=iRow;
    }
    // and back
    for (int i=0;i<numberNonZero;i++) {
      int iRow=index[i];
      double value=tempRegion[iRow];
      tempRegion[iRow]=0.0;
      region[iRow]=value;
    }
  }
}
// Permute in for Btran and multiply by pivot Region
void 
CoinAbcTypeFactorization::permuteInForBtranAndMultiply(CoinIndexedVector & regionSparse,
						       bool full) const
{
  int numberNonZero=regionSparse.getNumElements();
  double * COIN_RESTRICT region = regionSparse.denseVector();
  double * COIN_RESTRICT tempRegion = region+numberRows_;
  int * COIN_RESTRICT index = regionSparse.getIndices();
  const CoinFactorizationDouble * COIN_RESTRICT pivotRegion = pivotRegionAddress_;
  if (full) {
    numberNonZero=0;
    for (int iRow=0;iRow<numberRows_;iRow++) {
      double value = region[iRow];
      if (value) {
	region[iRow]=0.0;
	tempRegion[iRow] = value*pivotRegion[iRow];
	index[numberNonZero++]=iRow;
      }
    }
    regionSparse.setNumElements(numberNonZero);
  } else {
    for (int i=0;i<numberNonZero;i++) {
      int iRow=index[i];
      double value = region[iRow];
      region[iRow]=0.0;
      tempRegion[iRow] = value*pivotRegion[iRow];
    }
  }
  regionSparse.setDenseVector(tempRegion);
#if 0
  if (numberNonZero)
    printf("BtranIn vector %p region %p regionTemp %p\n",
	 &regionSparse,region,tempRegion);
  else
    printf("BtranIn no els! vector %p region %p regionTemp %p\n",
	 &regionSparse,region,tempRegion);
#endif
}
// Permute out for Btran
void 
CoinAbcTypeFactorization::permuteOutForBtran(CoinIndexedVector & regionSparse) const
{
  int numberNonZero=regionSparse.getNumElements();
  const int * COIN_RESTRICT permuteIn = permuteAddress_+maximumRowsExtra_+1;
  const int * COIN_RESTRICT permuteOut = permuteIn+numberRows_;
  CoinFactorizationDouble * COIN_RESTRICT tempRegion = denseVector(regionSparse);
  CoinFactorizationDouble * COIN_RESTRICT region = tempRegion-numberRows_;
  CoinSimplexInt * COIN_RESTRICT index = regionSparse.getIndices();
  regionSparse.setDenseVector(region);
  //printf("BtranOut vector %p region %p regionTemp %p\n",
  //	 &regionSparse,region,tempRegion);
  for (int i=0;i<numberNonZero;i++) {
    int iRow=index[i];
    double value=tempRegion[iRow];
    tempRegion[iRow]=0.0;
    iRow=permuteOut[iRow];
    region[iRow]=value;
    index[i]=iRow;
  }
}
#endif
#if COIN_BIG_DOUBLE==1
// To a work array and associate vector
void 
CoinAbcTypeFactorization::toLongArray(CoinIndexedVector * vector,int which) const
{
  assert (which>=0&&which<FACTOR_CPU);
  assert (!associatedVector_[which]);
  associatedVector_[which]=vector;
  CoinSimplexDouble * COIN_RESTRICT region = vector->denseVector (  );
  CoinSimplexInt * COIN_RESTRICT regionIndex = vector->getIndices (  );
  CoinSimplexInt numberNonZero = vector->getNumElements (  );
  long double * COIN_RESTRICT longRegion = longArray_[which].array();
  assert (!vector->packedMode());
  // could check all of longRegion for zero but this should trap most errors
  for (int i=0;i<numberNonZero;i++) {
    int iRow=regionIndex[i];
    assert (!longRegion[iRow]);
    longRegion[iRow]=region[iRow];
    region[iRow]=0.0;
  }
}
// From a work array and dis-associate vector
void 
CoinAbcTypeFactorization::fromLongArray(CoinIndexedVector * vector) const
{
  int which;
  for (which=0;which<FACTOR_CPU;which++) 
    if (vector==associatedVector_[which])
      break;
  assert (which<FACTOR_CPU);
  associatedVector_[which]=NULL;
  CoinSimplexDouble * COIN_RESTRICT region = vector->denseVector (  );
  CoinSimplexInt * COIN_RESTRICT regionIndex = vector->getIndices (  );
  CoinSimplexInt numberNonZero = vector->getNumElements (  );
  long double * COIN_RESTRICT longRegion = longArray_[which].array();
  assert (!vector->packedMode());
  // could check all of region for zero but this should trap most errors
  for (int i=0;i<numberNonZero;i++) {
    int iRow=regionIndex[i];
    assert (!region[iRow]);
    region[iRow]=longRegion[iRow];
    longRegion[iRow]=0.0;
  }
}
// From a work array and dis-associate vector
void 
CoinAbcTypeFactorization::fromLongArray(int which) const
{
  assert (which<FACTOR_CPU);
  CoinIndexedVector * vector = associatedVector_[which];
  associatedVector_[which]=NULL;
  CoinSimplexDouble * COIN_RESTRICT region = vector->denseVector (  );
  CoinSimplexInt * COIN_RESTRICT regionIndex = vector->getIndices (  );
  CoinSimplexInt numberNonZero = vector->getNumElements (  );
  long double * COIN_RESTRICT longRegion = longArray_[which].array();
  assert (!vector->packedMode());
  // could check all of region for zero but this should trap most errors
  for (int i=0;i<numberNonZero;i++) {
    int iRow=regionIndex[i];
    assert (!region[iRow]);
    region[iRow]=longRegion[iRow];
    longRegion[iRow]=0.0;
  }
}
// Returns long double * associated with vector
long double * 
CoinAbcTypeFactorization::denseVector(CoinIndexedVector * vector) const
{
  int which;
  for (which=0;which<FACTOR_CPU;which++) 
    if (vector==associatedVector_[which])
      break;
  assert (which<FACTOR_CPU);
  return longArray_[which].array();
}
// Returns long double * associated with vector
long double * 
CoinAbcTypeFactorization::denseVector(CoinIndexedVector & vector) const
{
  int which;
  for (which=0;which<FACTOR_CPU;which++) 
    if (&vector==associatedVector_[which])
      break;
  assert (which<FACTOR_CPU);
  return longArray_[which].array();
}
// Returns long double * associated with vector
const long double * 
CoinAbcTypeFactorization::denseVector(const CoinIndexedVector * vector) const
{
  int which;
  for (which=0;which<FACTOR_CPU;which++) 
    if (vector==associatedVector_[which])
      break;
  assert (which<FACTOR_CPU);
  return longArray_[which].array();
}
void
CoinAbcTypeFactorization::scan(CoinIndexedVector * vector) const
{
  int numberNonZero=0;
  int * COIN_RESTRICT index = vector->getIndices();
  CoinFactorizationDouble * COIN_RESTRICT region = denseVector(vector);
  for (int i=0;i<numberRows_;i++) {
    if (fabs(region[i])>zeroTolerance_)
      index[numberNonZero++]=i;
    else
      region[i]=0.0;
  }
  vector->setNumElements(numberNonZero);
}
// Clear all hidden arrays
void 
CoinAbcTypeFactorization::clearHiddenArrays()
{
  for (int which=0;which<FACTOR_CPU;which++) {
    if (associatedVector_[which]) {
      CoinIndexedVector * vector = associatedVector_[which];
      associatedVector_[which]=NULL;
      CoinFactorizationDouble * COIN_RESTRICT region = longArray_[which].array();
      CoinSimplexInt * COIN_RESTRICT regionIndex = vector->getIndices (  );
      CoinSimplexInt numberNonZero = vector->getNumElements (  );
    }
  }
}
#endif
#endif
