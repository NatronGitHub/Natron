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
#include "CoinPackedMatrix.hpp"
#include "CoinFinite.hpp"
#define _mm256_broadcast_sd(x) static_cast<__m256d> (__builtin_ia32_vbroadcastsd256 (x))
#define _mm256_load_pd(x) *(__m256d *)(x)
#define _mm256_store_pd (s, x)  *((__m256d *)s)=x
//:class CoinAbcTypeFactorization.  Deals with Factorization and Updates
//  CoinAbcTypeFactorization.  Constructor
CoinAbcTypeFactorization::CoinAbcTypeFactorization (  )
  : CoinAbcAnyFactorization()
{
  gutsOfInitialize(7);
}

/// Copy constructor 
CoinAbcTypeFactorization::CoinAbcTypeFactorization ( const CoinAbcTypeFactorization &other)
  : CoinAbcAnyFactorization(other)
{
  gutsOfInitialize(3);
  gutsOfCopy(other);
}
/// Clone
CoinAbcAnyFactorization * 
CoinAbcTypeFactorization::clone() const 
{
  return new CoinAbcTypeFactorization(*this);
}
/// Copy constructor 
CoinAbcTypeFactorization::CoinAbcTypeFactorization ( const CoinFactorization & /*other*/)
  : CoinAbcAnyFactorization()
{
  gutsOfInitialize(3);
  //gutsOfCopy(other);
  abort();
}
/// The real work of constructors etc
void CoinAbcTypeFactorization::gutsOfDestructor(CoinSimplexInt )
{
  numberCompressions_ = 0;
  numberRows_ = 0;
  numberRowsExtra_ = 0;
  maximumRowsExtra_ = 0;
  numberGoodU_ = 0;
  numberGoodL_ = 0;
  totalElements_ = 0;
  factorElements_ = 0;
  status_ = -1;
  numberSlacks_ = 0;
  lastSlack_ = 0;
  numberU_ = 0;
  maximumU_=0;
  lengthU_ = 0;
  lastEntryByColumnU_=0;
  lastEntryByRowU_=0;
  lengthAreaU_ = 0;
  numberL_ = 0;
  baseL_ = 0;
  lengthL_ = 0;
  lengthAreaL_ = 0;
  numberR_ = 0;
  lengthR_ = 0;
  lengthAreaR_ = 0;
#if ABC_SMALL<4
  numberDense_=0;
#endif  
}
#if FACTORIZATION_STATISTICS
extern double denseThresholdX;
#endif
// type - 1 bit tolerances etc, 2 rest
void CoinAbcTypeFactorization::gutsOfInitialize(CoinSimplexInt type)
{
  if ((type&2)!=0) {
    numberCompressions_ = 0;
    numberRows_ = 0;
    numberRowsExtra_ = 0;
    maximumRowsExtra_ = 0;
    numberGoodU_ = 0;
    numberGoodL_ = 0;
    totalElements_ = 0;
    factorElements_ = 0;
    status_ = -1;
    numberPivots_ = 0;
    numberSlacks_ = 0;
    lastSlack_ = 0;
    numberU_ = 0;
    maximumU_=0;
    lengthU_ = 0;
    lastEntryByColumnU_=0;
    lastEntryByRowU_=0;
    lengthAreaU_ = 0;
#ifdef ABC_USE_FUNCTION_POINTERS
    lengthAreaUPlus_ = 0;
#endif
    numberL_ = 0;
    baseL_ = 0;
    lengthL_ = 0;
    lengthAreaL_ = 0;
    numberR_ = 0;
    lengthR_ = 0;
    lengthAreaR_ = 0;
    elementRAddress_ = NULL;
    indexRowRAddress_ = NULL;
#if ABC_SMALL<2
    // always switch off sparse
    sparseThreshold_=0;
#endif
    state_= 0;
    // Maximum rows (ever)
    maximumRows_=0;
    // Rows first time nonzero
    initialNumberRows_=0;
    // Maximum maximum pivots
    maximumMaximumPivots_=0;
#if ABC_SMALL<4
    numberDense_=0;
#endif
  }
  // after 2 
  if ((type&1)!=0) {
    areaFactor_ = 0.0;
    pivotTolerance_ = 1.0e-1;
    numberDense_=0;
#ifndef USE_FIXED_ZERO_TOLERANCE
    zeroTolerance_ = 1.0e-13;
#else
    zeroTolerance_ = pow(0.5,43);
#endif
    messageLevel_=0;
    maximumPivots_=200;
    maximumMaximumPivots_=200;
    numberTrials_ = 4;
    relaxCheck_=1.0;
#if ABC_SMALL<4
#if ABC_DENSE_CODE>0
    denseThreshold_=4;//16; //31;
    //denseThreshold_=71;
#else
    denseThreshold_=-16;
#endif
#if FACTORIZATION_STATISTICS
    denseThreshold_=denseThresholdX;
#endif
    //denseThreshold_=0; // temp (? ABC_PARALLEL)
#endif
  }
#if ABC_SMALL<4
  //denseThreshold_=10000;
#endif
  if ((type&4)!=0) {
#if COIN_BIG_DOUBLE==1
    for (int i=0;i<FACTOR_CPU;i++) {
      longArray_[i].switchOn(4);
      associatedVector_[i]=NULL;
    }
#endif
    pivotColumn_.switchOn();
    permute_.switchOn();
    startRowU_.switchOn();
    numberInRow_.switchOn();
    numberInColumn_.switchOn();
    numberInColumnPlus_.switchOn();
#ifdef ABC_USE_FUNCTION_POINTERS
    scatterUColumn_.switchOn();
#endif
    firstCount_.switchOn();
    nextColumn_.switchOn();
    lastColumn_.switchOn();
    nextRow_.switchOn();
    lastRow_.switchOn();
    saveColumn_.switchOn();
    markRow_.switchOn();
    indexColumnU_.switchOn();
    pivotRegion_.switchOn();
    elementU_.switchOn();
    indexRowU_.switchOn();
    startColumnU_.switchOn();
#if CONVERTROW
    convertRowToColumnU_.switchOn();
#if CONVERTROW>1
    convertColumnToRowU_.switchOn();
#endif
#endif
#if ABC_SMALL<2
    elementRowU_.switchOn();
#endif
    elementL_.switchOn();
    indexRowL_.switchOn();
    startColumnL_.switchOn();
#if ABC_SMALL<4
    denseArea_.switchOn(7);
#endif
    workArea_.switchOn();
    workArea2_.switchOn();
#if ABC_SMALL<2
    startRowL_.switchOn();
    indexColumnL_.switchOn();
    elementByRowL_.switchOn();
    sparse_.switchOn();
    
    // Below are all to collect
    ftranCountInput_=0.0;
    ftranCountAfterL_=0.0;
    ftranCountAfterR_=0.0;
    ftranCountAfterU_=0.0;
    ftranFTCountInput_=0.0;
    ftranFTCountAfterL_=0.0;
    ftranFTCountAfterR_=0.0;
    ftranFTCountAfterU_=0.0;
    btranCountInput_=0.0;
    btranCountAfterU_=0.0;
    btranCountAfterR_=0.0;
    btranCountAfterL_=0.0;
    ftranFullCountInput_=0.0;
    ftranFullCountAfterL_=0.0;
    ftranFullCountAfterR_=0.0;
    ftranFullCountAfterU_=0.0;
    btranFullCountInput_=0.0;
    btranFullCountAfterL_=0.0;
    btranFullCountAfterR_=0.0;
    btranFullCountAfterU_=0.0;
#if FACTORIZATION_STATISTICS
    ftranTwiddleFactor1_=1.0;
    ftranFTTwiddleFactor1_=1.0;
    btranTwiddleFactor1_=1.0;
    ftranFullTwiddleFactor1_=1.0;
    btranFullTwiddleFactor1_=1.0;
    ftranTwiddleFactor2_=1.0;
    ftranFTTwiddleFactor2_=1.0;
    btranTwiddleFactor2_=1.0;
    ftranFullTwiddleFactor2_=1.0;
    btranFullTwiddleFactor2_=1.0;
#endif    
    // We can roll over factorizations
    numberFtranCounts_=0;
    numberFtranFTCounts_=0;
    numberBtranCounts_=0;
    numberFtranFullCounts_=0;
    numberFtranFullCounts_=0;
    
    // While these are averages collected over last 
    ftranAverageAfterL_=INITIAL_AVERAGE;
    ftranAverageAfterR_=INITIAL_AVERAGE;
    ftranAverageAfterU_=INITIAL_AVERAGE;
    ftranFTAverageAfterL_=INITIAL_AVERAGE;
    ftranFTAverageAfterR_=INITIAL_AVERAGE;
    ftranFTAverageAfterU_=INITIAL_AVERAGE;
    btranAverageAfterU_=INITIAL_AVERAGE;
    btranAverageAfterR_=INITIAL_AVERAGE;
    btranAverageAfterL_=INITIAL_AVERAGE; 
    ftranFullAverageAfterL_=INITIAL_AVERAGE;
    ftranFullAverageAfterR_=INITIAL_AVERAGE;
    ftranFullAverageAfterU_=INITIAL_AVERAGE;
    btranFullAverageAfterL_=INITIAL_AVERAGE;
    btranFullAverageAfterR_=INITIAL_AVERAGE;
    btranFullAverageAfterU_=INITIAL_AVERAGE;
#endif
  }
}

//  ~CoinAbcTypeFactorization.  Destructor
CoinAbcTypeFactorization::~CoinAbcTypeFactorization (  )
{
  gutsOfDestructor();
}
//  =
CoinAbcTypeFactorization & CoinAbcTypeFactorization::operator = ( const CoinAbcTypeFactorization & other ) {
  if (this != &other) {    
    gutsOfDestructor();
    CoinAbcAnyFactorization::operator=(other);
    gutsOfInitialize(3);
    gutsOfCopy(other);
  }
  return *this;
}
void CoinAbcTypeFactorization::gutsOfCopy(const CoinAbcTypeFactorization &other)
{
#ifdef ABC_USE_FUNCTION_POINTERS
  elementU_.allocate(other.elementU_, other.lengthAreaUPlus_ *CoinSizeofAsInt(CoinFactorizationDouble));
#else
  elementU_.allocate(other.elementU_, other.lengthAreaU_ *CoinSizeofAsInt(CoinFactorizationDouble));
#endif
  indexRowU_.allocate(other.indexRowU_, (other.lengthAreaU_+1)*CoinSizeofAsInt(CoinSimplexInt) );
  elementL_.allocate(other.elementL_, other.lengthAreaL_*CoinSizeofAsInt(CoinFactorizationDouble) );
  indexRowL_.allocate(other.indexRowL_, other.lengthAreaL_*CoinSizeofAsInt(CoinSimplexInt) );
  startColumnL_.allocate(other.startColumnL_,(other.numberRows_ + 1)*CoinSizeofAsInt(CoinBigIndex) );
  pivotRegion_.allocate(other.pivotRegion_, (other.numberRows_+2 )*CoinSizeofAsInt(CoinFactorizationDouble));
#ifndef ABC_ORDERED_FACTORIZATION
  permute_.allocate(other.permute_,(other.maximumRowsExtra_ + 1)*CoinSizeofAsInt(CoinSimplexInt));
#else
  //temp
  permute_.allocate(other.permute_,(other.maximumRowsExtra_+2*numberRows_ + 1)*CoinSizeofAsInt(CoinSimplexInt));
#endif
  firstCount_.allocate(other.firstCount_,
		       ( CoinMax(5*numberRows_,4*numberRows_+2*maximumPivots_+4)+2)
		       *CoinSizeofAsInt(CoinSimplexInt));
  nextCountAddress_=nextCount();
  lastCountAddress_=lastCount();
  startColumnU_.allocate(other.startColumnU_, (other.numberRows_ + 1 )*CoinSizeofAsInt(CoinBigIndex));
  numberInColumn_.allocate(other.numberInColumn_, (other.numberRows_ + 1 )*CoinSizeofAsInt(CoinSimplexInt));
#if COIN_BIG_DOUBLE==1
  for (int i=0;i<FACTOR_CPU;i++)
    longArray_[i].allocate(other.longArray_[i],(other.maximumRowsExtra_ + 1)*CoinSizeofAsInt(long double));
#endif
  pivotColumn_.allocate(other.pivotColumn_,(other.maximumRowsExtra_ + 1)*CoinSizeofAsInt(CoinSimplexInt));
  nextColumn_.allocate(other.nextColumn_, (other.maximumRowsExtra_ + 1 )*CoinSizeofAsInt(CoinSimplexInt));
  lastColumn_.allocate(other.lastColumn_, (other.maximumRowsExtra_ + 1 )*CoinSizeofAsInt(CoinSimplexInt));
  nextRow_.allocate(other.nextRow_, (other.numberRows_ + 1 )*CoinSizeofAsInt(CoinSimplexInt));
  lastRow_.allocate(other.lastRow_, (other.numberRows_ + 1 )*CoinSizeofAsInt(CoinSimplexInt));
  indexColumnU_.allocate(other.indexColumnU_, (other.lengthAreaU_+1)*CoinSizeofAsInt(CoinSimplexInt) );
#if CONVERTROW
  convertRowToColumnU_.allocate(other.convertRowToColumnU_, other.lengthAreaU_*CoinSizeofAsInt(CoinBigIndex) );
  const CoinBigIndex * convertUOther = other.convertRowToColumnUAddress_;
#if CONVERTROW>1
  convertColumnToRowU_.allocate(other.convertColumnToRowU_, other.lengthAreaU_*CoinSizeofAsInt(CoinBigIndex) );
#if CONVERTROW>2
  const CoinBigIndex * convertUOther2 = other.convertColumnToRowUAddress_;
#endif
#endif
#endif
#if ABC_SMALL<2
  const CoinFactorizationDouble * elementRowUOther = other.elementRowUAddress_;
#if COIN_ONE_ETA_COPY
  if (elementRowUOther) {
#endif
#ifdef ABC_USE_FUNCTION_POINTERS
    elementRowU_.allocate(other.elementRowU_, other.lengthAreaUPlus_*CoinSizeofAsInt(CoinFactorizationDouble) );
#else
    elementRowU_.allocate(other.elementRowU_, other.lengthAreaU_*CoinSizeofAsInt(CoinFactorizationDouble) );
#endif
    startRowU_.allocate(other.startRowU_,(other.numberRows_ + 1)*CoinSizeofAsInt(CoinBigIndex));
    numberInRow_.allocate(other.numberInRow_, (other.numberRows_ + 1 )*CoinSizeofAsInt(CoinSimplexInt));
#if COIN_ONE_ETA_COPY
  }
#endif
  if (other.sparseThreshold_) {
    elementByRowL_.allocate(other.elementByRowL_, other.lengthAreaL_ );
    indexColumnL_.allocate(other.indexColumnL_, other.lengthAreaL_ );
    startRowL_.allocate(other.startRowL_,other.numberRows_+1);
  }
#endif
  numberTrials_ = other.numberTrials_;
  relaxCheck_ = other.relaxCheck_;
  numberSlacks_ = other.numberSlacks_;
  lastSlack_ = other.lastSlack_;
  numberU_ = other.numberU_;
  maximumU_=other.maximumU_;
  lengthU_ = other.lengthU_;
  lastEntryByColumnU_=other.lastEntryByColumnU_;
  lastEntryByRowU_=other.lastEntryByRowU_;
  lengthAreaU_ = other.lengthAreaU_;
#ifdef ABC_USE_FUNCTION_POINTERS
  lengthAreaUPlus_ = other.lengthAreaUPlus_;
#endif
  numberL_ = other.numberL_;
  baseL_ = other.baseL_;
  lengthL_ = other.lengthL_;
  lengthAreaL_ = other.lengthAreaL_;
  numberR_ = other.numberR_;
  lengthR_ = other.lengthR_;
  lengthAreaR_ = other.lengthAreaR_;
  pivotTolerance_ = other.pivotTolerance_;
  zeroTolerance_ = other.zeroTolerance_;
  areaFactor_ = other.areaFactor_;
  numberRows_ = other.numberRows_;
  numberRowsExtra_ = other.numberRowsExtra_;
  maximumRowsExtra_ = other.maximumRowsExtra_;
  maximumPivots_=other.maximumPivots_;
  numberGoodU_ = other.numberGoodU_;
  numberGoodL_ = other.numberGoodL_;
  numberPivots_ = other.numberPivots_;
  messageLevel_ = other.messageLevel_;
  totalElements_ = other.totalElements_;
  factorElements_ = other.factorElements_;
  status_ = other.status_;
#if ABC_SMALL<2
  //doForrestTomlin_ = other.doForrestTomlin_;
  ftranCountInput_=other.ftranCountInput_;
  ftranCountAfterL_=other.ftranCountAfterL_;
  ftranCountAfterR_=other.ftranCountAfterR_;
  ftranCountAfterU_=other.ftranCountAfterU_;
  ftranFTCountInput_=other.ftranFTCountInput_;
  ftranFTCountAfterL_=other.ftranFTCountAfterL_;
  ftranFTCountAfterR_=other.ftranFTCountAfterR_;
  ftranFTCountAfterU_=other.ftranFTCountAfterU_;
  btranCountInput_=other.btranCountInput_;
  btranCountAfterU_=other.btranCountAfterU_;
  btranCountAfterR_=other.btranCountAfterR_;
  btranCountAfterL_=other.btranCountAfterL_;
  ftranFullCountInput_=other.ftranFullCountInput_;
  ftranFullCountAfterL_=other.ftranFullCountAfterL_;
  ftranFullCountAfterR_=other.ftranFullCountAfterR_;
  ftranFullCountAfterU_=other.ftranFullCountAfterU_;
  btranFullCountInput_=other.btranFullCountInput_;
  btranFullCountAfterL_=other.btranFullCountAfterL_;
  btranFullCountAfterR_=other.btranFullCountAfterR_;
  btranFullCountAfterU_=other.btranFullCountAfterU_;
  numberFtranCounts_=other.numberFtranCounts_;
  numberFtranFTCounts_=other.numberFtranFTCounts_;
  numberBtranCounts_=other.numberBtranCounts_;
  numberFtranFullCounts_=other.numberFtranFullCounts_;
  numberFtranFullCounts_=other.numberFtranFullCounts_;
  ftranAverageAfterL_=other.ftranAverageAfterL_;
  ftranAverageAfterR_=other.ftranAverageAfterR_;
  ftranAverageAfterU_=other.ftranAverageAfterU_;
  ftranFTAverageAfterL_=other.ftranFTAverageAfterL_;
  ftranFTAverageAfterR_=other.ftranFTAverageAfterR_;
  ftranFTAverageAfterU_=other.ftranFTAverageAfterU_;
  btranAverageAfterU_=other.btranAverageAfterU_;
  btranAverageAfterR_=other.btranAverageAfterR_;
  btranAverageAfterL_=other.btranAverageAfterL_; 
  ftranFullAverageAfterL_=other.ftranFullAverageAfterL_;
  ftranFullAverageAfterR_=other.ftranFullAverageAfterR_;
  ftranFullAverageAfterU_=other.ftranFullAverageAfterU_;
  btranFullAverageAfterL_=other.btranFullAverageAfterL_;
  btranFullAverageAfterR_=other.btranFullAverageAfterR_;
  btranFullAverageAfterU_=other.btranFullAverageAfterU_;
#if FACTORIZATION_STATISTICS
  ftranTwiddleFactor1_=other.ftranTwiddleFactor1_;
  ftranFTTwiddleFactor1_=other.ftranFTTwiddleFactor1_;
  btranTwiddleFactor1_=other.btranTwiddleFactor1_;
  ftranFullTwiddleFactor1_=other.ftranFullTwiddleFactor1_;
  btranFullTwiddleFactor1_=other.btranFullTwiddleFactor1_;
  ftranTwiddleFactor2_=other.ftranTwiddleFactor2_;
  ftranFTTwiddleFactor2_=other.ftranFTTwiddleFactor2_;
  btranTwiddleFactor2_=other.btranTwiddleFactor2_;
  ftranFullTwiddleFactor2_=other.ftranFullTwiddleFactor2_;
  btranFullTwiddleFactor2_=other.btranFullTwiddleFactor2_;
#endif    
  sparseThreshold_=other.sparseThreshold_;
#endif
  state_=other.state_;
  // Maximum rows (ever)
  maximumRows_=other.maximumRows_;
  // Rows first time nonzero
  initialNumberRows_=other.initialNumberRows_;
  // Maximum maximum pivots
  maximumMaximumPivots_=other.maximumMaximumPivots_;
  CoinBigIndex space = lengthAreaL_ - lengthL_;

#if ABC_SMALL<4
  numberDense_ = other.numberDense_;
  denseThreshold_=other.denseThreshold_;
#endif
  lengthAreaR_ = space;
  elementRAddress_ = elementL_.array() + lengthL_;
  indexRowRAddress_ = indexRowL_.array() + lengthL_;
  workArea_ = other.workArea_;
  workArea2_ = other.workArea2_;
  //now copy everything
  //assuming numberRowsExtra==numberColumnsExtra
  if (numberRowsExtra_) {
    if (startRowU_.array()) {
      CoinAbcMemcpy( startRowU_.array() , other.startRowU_.array(), numberRows_ + 1);
      CoinAbcMemcpy( numberInRow_.array() , other.numberInRow_.array(), numberRows_ + 1);
      //startRowU_.array()[maximumRowsExtra_] = other.startRowU_.array()[maximumRowsExtra_];
    }
    CoinAbcMemcpy( pivotRegion_.array() , other.pivotRegion_.array(), numberRows_+2 );
#ifndef ABC_ORDERED_FACTORIZATION
    CoinAbcMemcpy( permute_.array() , other.permute_.array(), numberRowsExtra_ + 1);
#else
  //temp
    CoinAbcMemcpy( permute_.array() , other.permute_.array(), maximumRowsExtra_+2*numberRows_ + 1);
#endif
    CoinAbcMemcpy( firstCount_.array() , other.firstCount_.array(),CoinMax(5*numberRows_,4*numberRows_+2*maximumPivots_+4)+2);
    CoinAbcMemcpy( startColumnU_.array() , other.startColumnU_.array(), numberRows_ + 1);
    CoinAbcMemcpy( numberInColumn_.array() , other.numberInColumn_.array(), numberRows_ + 1);
    CoinAbcMemcpy( pivotColumn_.array() , other.pivotColumn_.array(), numberRowsExtra_ + 1);
    CoinAbcMemcpy( nextColumn_.array() , other.nextColumn_.array(), numberRowsExtra_ + 1);
    CoinAbcMemcpy( lastColumn_.array() , other.lastColumn_.array(), numberRowsExtra_ + 1);
    CoinAbcMemcpy (startColumnR() , other.startColumnR() , numberRowsExtra_ - numberRows_ + 1);
			  
    //extra one at end
    //startColumnU_.array()[maximumRowsExtra_] =
    //other.startColumnU_.array()[maximumRowsExtra_];
    nextColumn_.array()[maximumRowsExtra_] = other.nextColumn_.array()[maximumRowsExtra_];
    lastColumn_.array()[maximumRowsExtra_] = other.lastColumn_.array()[maximumRowsExtra_];
    CoinAbcMemcpy( nextRow_.array() , other.nextRow_.array(), numberRows_ + 1);
    CoinAbcMemcpy( lastRow_.array() , other.lastRow_.array(), numberRows_ + 1);
    nextRow_.array()[numberRows_] = other.nextRow_.array()[numberRows_];
    lastRow_.array()[numberRows_] = other.lastRow_.array()[numberRows_];
  }
  CoinAbcMemcpy( elementRAddress_ , other.elementRAddress_, lengthR_);
  CoinAbcMemcpy( indexRowRAddress_ , other.indexRowRAddress_, lengthR_);
  //row and column copies of U
  /* as elements of U may have been zeroed but column counts zero
     copy all elements */
  const CoinBigIndex *  COIN_RESTRICT startColumnU = startColumnU_.array();
  const CoinSimplexInt *  COIN_RESTRICT numberInColumn = numberInColumn_.array();
#ifndef NDEBUG
  CoinSimplexInt maxU=0;
  for (CoinSimplexInt iRow = 0; iRow < numberRows_; iRow++ ) {
    CoinBigIndex start = startColumnU[iRow];
    CoinSimplexInt numberIn = numberInColumn[iRow];
    maxU = CoinMax(maxU,start+numberIn);
  }
  //assert (maximumU_>=maxU);
#endif
  CoinAbcMemcpy( elementU_.array() , other.elementU_.array() , maximumU_);
#if ABC_SMALL<2
#if COIN_ONE_ETA_COPY
  if (elementRowUOther) {
#endif
    const CoinSimplexInt *  COIN_RESTRICT indexColumnUOther = other.indexColumnU_.array();
#if CONVERTROW
    CoinBigIndex *  COIN_RESTRICT convertU = convertRowToColumnU_.array();
#if CONVERTROW>2
    CoinBigIndex *  COIN_RESTRICT convertU2 = convertColumnToRowU_.array();
#endif
#endif
    CoinFactorizationDouble *  COIN_RESTRICT elementRowU = elementRowU_.array();
    CoinSimplexInt *  COIN_RESTRICT indexColumnU = indexColumnU_.array();
    const CoinBigIndex *  COIN_RESTRICT startRowU = startRowU_.array();
    const CoinSimplexInt *  COIN_RESTRICT numberInRow = numberInRow_.array();
    for (CoinSimplexInt iRow = 0; iRow < numberRows_; iRow++ ) {
      //row
      CoinBigIndex start = startRowU[iRow];
      CoinSimplexInt numberIn = numberInRow[iRow];
      
      CoinAbcMemcpy( indexColumnU + start , indexColumnUOther + start, numberIn);
#if CONVERTROW
      CoinAbcMemcpy(   convertU + start ,convertUOther + start , numberIn);
#if CONVERTROW>2
      CoinAbcMemcpy(   convertU2 + start ,convertUOther2 + start , numberIn);
#endif
#endif
      CoinAbcMemcpy(   elementRowU + start ,elementRowUOther + start , numberIn);
    }
#if COIN_ONE_ETA_COPY
  }
#endif
#endif
  const CoinSimplexInt *  COIN_RESTRICT indexRowUOther = other.indexRowU_.array();
  CoinSimplexInt *  COIN_RESTRICT indexRowU = indexRowU_.array();
  for (CoinSimplexInt iRow = 0; iRow < numberRows_; iRow++ ) {
    //column
    CoinBigIndex start = startColumnU[iRow];
    CoinSimplexInt numberIn = numberInColumn[iRow];
    if (numberIn>0)
      CoinAbcMemcpy( indexRowU + start , indexRowUOther + start, numberIn);
  }
  // L is contiguous
  if (numberRows_)
    CoinAbcMemcpy( startColumnL_.array() , other.startColumnL_.array(), numberRows_ + 1);
  CoinAbcMemcpy( elementL_.array() , other.elementL_.array(), lengthL_);
  CoinAbcMemcpy( indexRowL_.array() , other.indexRowL_.array(), lengthL_);
#if ABC_SMALL<2
  if (other.sparseThreshold_) {
    goSparse();
  }
#endif
  doAddresses();
}

//  getAreas.  Gets space for a factorization
//called by constructors
void
CoinAbcTypeFactorization::getAreas ( CoinSimplexInt numberOfRows,
			     CoinSimplexInt /*numberOfColumns*/,
			 CoinBigIndex maximumL,
			 CoinBigIndex maximumU )
{
  gutsOfInitialize(2);
  
  numberRows_ = numberOfRows;
  numberRowsSmall_ = numberOfRows;
  maximumRows_ = CoinMax(maximumRows_,numberRows_);
  maximumRowsExtra_ = numberRows_ + maximumPivots_;
  numberRowsExtra_ = numberRows_;
  lengthAreaU_ = maximumU;
  lengthAreaL_ = maximumL;
  if ( !areaFactor_ ) {
    areaFactor_ = 1.0;
  }
  if ( areaFactor_ != 1.0 ) {
    if ((messageLevel_&16)!=0) 
      printf("Increasing factorization areas by %g\n",areaFactor_);
    lengthAreaU_ =  static_cast<CoinBigIndex> (areaFactor_*lengthAreaU_);
    lengthAreaL_ =  static_cast<CoinBigIndex> (areaFactor_*lengthAreaL_);
  }
#ifdef ABC_USE_FUNCTION_POINTERS
  lengthAreaUPlus_ = (lengthAreaU_*3)/2+maximumRowsExtra_ ;
  elementU_.conditionalNew( lengthAreaUPlus_ );
#else
  elementU_.conditionalNew( lengthAreaU_ );
#endif
  indexRowU_.conditionalNew( lengthAreaU_+1 );
  indexColumnU_.conditionalNew( lengthAreaU_+1 );
  elementL_.conditionalNew( lengthAreaL_ );
  indexRowL_.conditionalNew( lengthAreaL_ );
  // But we can use all we have if bigger
  CoinBigIndex length;
  length = CoinMin(elementU_.getSize(),indexRowU_.getSize());
  if (length>lengthAreaU_) {
    lengthAreaU_=length;
    assert (indexColumnU_.getSize()==indexRowU_.getSize());
  }
  length = CoinMin(elementL_.getSize(),indexRowL_.getSize());
  if (length>lengthAreaL_) {
    lengthAreaL_=length;
  }
  startColumnL_.conditionalNew( numberRows_ + 1 );
  startColumnL_.array()[0] = 0;
  startRowU_.conditionalNew( maximumRowsExtra_ + 1);
  // make sure this is valid (Needed for row links????)
  startRowU_.array()[maximumRowsExtra_]=0; 
  lastEntryByRowU_ = 0;
  numberInRow_.conditionalNew( maximumRowsExtra_ + 1 );
  markRow_.conditionalNew( numberRows_ );
  nextRow_.conditionalNew( maximumRowsExtra_ + 1 );
  lastRow_.conditionalNew( maximumRowsExtra_ + 1 );
#ifndef ABC_ORDERED_FACTORIZATION
  permute_.conditionalNew( maximumRowsExtra_ + 1 );
#else
  //temp
  permute_.conditionalNew( maximumRowsExtra_+2*numberRows_ + 1 );
#endif
  permuteAddress_ = permute_.array();
  pivotRegion_.conditionalNew( maximumRows_ + 2 );
  startColumnU_.conditionalNew( maximumRowsExtra_ + 1 );
  numberInColumn_.conditionalNew( maximumRowsExtra_ + 1 );
  numberInColumnPlus_.conditionalNew( maximumRowsExtra_ + 1 );
#ifdef ABC_USE_FUNCTION_POINTERS
  scatterUColumn_.conditionalNew(sizeof(scatterStruct), maximumRowsExtra_ + 1);
  elementRowU_.conditionalNew( lengthAreaUPlus_ );
  elementRowUAddress_=elementRowU_.array();
  firstZeroed_=0;
#elif ABC_SMALL<2
  elementRowU_.conditionalNew( lengthAreaU_ );
  elementRowUAddress_=elementRowU_.array();
#endif
#if COIN_BIG_DOUBLE==1
  for (int i=0;i<FACTOR_CPU;i++) {
      longArray_[i].conditionalNew( maximumRowsExtra_ + 1 );
      longArray_[i].clear();
  }
#endif
  pivotColumn_.conditionalNew( maximumRowsExtra_ + 1 );
  nextColumn_.conditionalNew( maximumRowsExtra_ + 1 );
  lastColumn_.conditionalNew( maximumRowsExtra_ + 1 );
  saveColumn_.conditionalNew( /*2*  */ numberRows_);
  assert (sizeof(CoinSimplexInt)==sizeof(CoinBigIndex)); // would need to redo
  firstCount_.conditionalNew( CoinMax(5*numberRows_,4*numberRows_+2*maximumPivots_+4)+2 );
#if CONVERTROW
  //space for cross reference
  convertRowToColumnU_.conditionalNew( lengthAreaU_ );
  convertRowToColumnUAddress_=convertRowToColumnU_.array();
#if CONVERTROW>1
  convertColumnToRowU_.conditionalNew( lengthAreaU_ );
  convertColumnToRowUAddress_=convertColumnToRowU_.array();
#endif
#endif
#ifdef SMALL_PERMUTE
  assert (sizeof(CoinFactorizationDouble)>=2*sizeof(int));
  denseArea_.conditionalNew(numberRows_+maximumRowsExtra_+2);
  fromSmallToBigRow_=reinterpret_cast<CoinSimplexInt *>(denseArea_.array());
  fromSmallToBigColumn_=fromSmallToBigRow_+numberRows_;
#endif
  doAddresses();
#if ABC_SMALL<2
  // temp - use as marker for valid row/column lookup
  sparseThreshold_=0;
#endif
}

#include "AbcSimplex.hpp"
#include "ClpMessage.hpp"

//Does most of factorization
CoinSimplexInt
CoinAbcTypeFactorization::factor (AbcSimplex * model)
{
#if ABC_DENSE_CODE
  int saveDense = denseThreshold_;
  if ((solveMode_&1)==0)
    denseThreshold_=0;
#endif
  //sparse
  status_ = factorSparse (  );
  switch ( status_ ) {
  case 0:			//finished
    totalElements_ = 0;
    if ( numberGoodU_ < numberRows_ ) 
      status_ = -1; 
    break; 
#if ABC_DENSE_CODE
#if ABC_SMALL<4
    // dense
  case 2: 
    status_=factorDense();
    if(!status_) 
      break;
#endif
#endif
  default:
    //singular ? or some error
    if ((messageLevel_&4)!=0) 
      std::cout << "Error " << status_ << std::endl;
    if (status_==-99) {
      if (numberRows_-numberGoodU_<1000) {
	areaFactor_ *= 1.5;
      } else {
	double denseArea=(numberRows_-numberGoodU_)*(numberRows_-numberGoodU_);
	if (denseArea>1.5*lengthAreaU_) {
	  areaFactor_ *= CoinMin(2.5,denseArea/1.5);
	} else {
	  areaFactor_ *= 1.5;
	}
      }
      if (model->logLevel()>1) {
	char line[100];
	sprintf(line,"need more memory lengthArea %d number %d done %d areaFactor %g",
		lengthAreaL_+lengthAreaU_,lengthU_+lengthL_,numberGoodU_,areaFactor_);
	model->messageHandler()->message(CLP_GENERAL2,*model->messagesPointer())
	  << line << CoinMessageEol;
      }
    } else if (status_==-97) {
      // just pivot tolerance issue
      status_=-99;
      char line[100];
      sprintf(line,"Bad pivot values - increasing pivot tolerance to %g",
	      pivotTolerance_);
      model->messageHandler()->message(CLP_GENERAL2,*model->messagesPointer())
	<< line << CoinMessageEol;
    }
    break;
  }				/* endswitch */
  //clean up
  if ( !status_ ) {
    if ( (messageLevel_ & 16)&&numberCompressions_)
      std::cout<<"        Factorization did "<<numberCompressions_
	       <<" compressions"<<std::endl;
    if ( numberCompressions_ > 10 ) {
      // but not more than 5 times final
      if (lengthAreaL_+lengthAreaU_<10*(lengthU_+lengthL_)) {
	areaFactor_ *= 1.1;
	if (model->logLevel()>1) {
	  char line[100];
	  sprintf(line,"%d factorization compressions, lengthArea %d number %d new areaFactor %g",
		  numberCompressions_,lengthAreaL_+lengthAreaU_,lengthU_+lengthL_,areaFactor_);
	  model->messageHandler()->message(CLP_GENERAL2,*model->messagesPointer())
	    << line << CoinMessageEol;
	}
      }
    }
    numberCompressions_=0;
    cleanup (  );
  }
  numberPivots_=0;
#if ABC_DENSE_CODE
  denseThreshold_=saveDense;
#endif
  return status_;
}
#ifdef ABC_USE_FUNCTION_POINTERS 
#define DENSE_TRY
#ifdef DENSE_TRY
static void pivotStartup(int first, int last, int numberInPivotColumn, int lengthArea,int giveUp,
			 CoinFactorizationDouble * COIN_RESTRICT eArea,const int * COIN_RESTRICT saveColumn,
			 const int * COIN_RESTRICT startColumnU,int * COIN_RESTRICT tempColumnCount,
			 const CoinFactorizationDouble * COIN_RESTRICT elementU, 
			 const int * COIN_RESTRICT numberInColumn,
			 const int * COIN_RESTRICT indexRowU)
{
  if (last-first>giveUp) {
    int mid=(last+first)>>1;
    cilk_spawn pivotStartup(first,mid,numberInPivotColumn,lengthArea,giveUp,
			    eArea,saveColumn,startColumnU,tempColumnCount,
			    elementU,numberInColumn,indexRowU);
    pivotStartup(mid,last,numberInPivotColumn,lengthArea,giveUp,
		 eArea,saveColumn,startColumnU,tempColumnCount,
		 elementU,numberInColumn,indexRowU);
    cilk_sync;
  } else {
    CoinFactorizationDouble * COIN_RESTRICT area =eArea+first*lengthArea;
    for (int jColumn = first; jColumn < last; jColumn++ ) {
      CoinSimplexInt iColumn = saveColumn[jColumn];
      CoinBigIndex startColumn = startColumnU[iColumn];
      int numberNow=tempColumnCount[jColumn];
      int start=startColumn+numberNow;
      int end=startColumn+numberInColumn[iColumn];
      CoinFactorizationDouble thisPivotValue = elementU[startColumn-1];
      for (CoinBigIndex j=start;j<end;j++) {
	CoinFactorizationDouble value=elementU[j];
	int iPut=indexRowU[j];
	assert (iPut<lengthArea);
	area[iPut]=value;
      }
      area[numberInPivotColumn]=thisPivotValue;
      area+=lengthArea;
    }
  }
}
static void pivotWhile(int first, int last, int numberInPivotColumn,int lengthArea,int giveUp,
		       CoinFactorizationDouble * COIN_RESTRICT eArea,const CoinFactorizationDouble * COIN_RESTRICT multipliersL)
{
  if (last-first>giveUp) {
    int mid=(last+first)>>1;
    cilk_spawn pivotWhile(first,mid,numberInPivotColumn,lengthArea,giveUp,
			  eArea,multipliersL);
    pivotWhile(mid,last,numberInPivotColumn,lengthArea,giveUp,
	       eArea,multipliersL);
    cilk_sync;
  } else {
    CoinFactorizationDouble * COIN_RESTRICT area =eArea+first*lengthArea;
    int nDo=last-first;
#if AVX2==0
    for (int jColumn = 0; jColumn < nDo; jColumn++ ) {
      CoinFactorizationDouble thisPivotValue = area[numberInPivotColumn];
      area[numberInPivotColumn]=0.0;
      for (CoinSimplexInt j = 0; j < numberInPivotColumn; j++ ) {
	area[j] -= thisPivotValue * multipliersL[j];
      }
      area+=lengthArea;
    }
#else
    int n=(numberInPivotColumn+3)&(~3);
    // for compiler error
    CoinFactorizationDouble * multipliers = const_cast<CoinFactorizationDouble *>(multipliersL);
    for (int jColumn = 0; jColumn < nDo; jColumn++ ) {
      //CoinFactorizationDouble thisPivotValue = area[numberInPivotColumn];
      __m256d pivot = _mm256_broadcast_sd(area+numberInPivotColumn);
      area[numberInPivotColumn]=0.0;
      for (CoinSimplexInt j = 0; j < n; j+=4 ) {
	__m256d a=_mm256_load_pd(area+j);
	__m256d m=_mm256_load_pd(multipliers+j);
	a -= pivot*m;
	*reinterpret_cast<__m256d *>(area+j)=a; //_mm256_store_pd ((area+j), a);
	//area[j] -= thisPivotValue * multipliersL[j];
	//area[j+1] -= thisPivotValue * multipliersL[j+1];
	//area[j+2] -= thisPivotValue * multipliersL[j+2];
	//area[j+3] -= thisPivotValue * multipliersL[j+3];
      }
      area+=lengthArea;
    }
#endif
  }
}
static void pivotSomeAfter(int first, int last, int numberInPivotColumn,int lengthArea,int giveUp,
		      CoinFactorizationDouble * COIN_RESTRICT eArea,const int * COIN_RESTRICT saveColumn,
		      const int * COIN_RESTRICT startColumnU,int * COIN_RESTRICT tempColumnCount,
		      CoinFactorizationDouble * COIN_RESTRICT elementU, int * COIN_RESTRICT numberInColumn,
		      int * COIN_RESTRICT indexRowU, unsigned int * aBits,
		      const int * COIN_RESTRICT indexL,
		      const CoinFactorizationDouble * COIN_RESTRICT multipliersL, double tolerance)
{
  if (last-first>giveUp) {
    int mid=(last+first)>>1;
    cilk_spawn pivotSomeAfter(first,mid,numberInPivotColumn,lengthArea,giveUp,
	    eArea,saveColumn,startColumnU,tempColumnCount,
	    elementU,numberInColumn,indexRowU,aBits,indexL,
	    multipliersL,tolerance);
    pivotSomeAfter(mid,last,numberInPivotColumn,lengthArea,giveUp,
	    eArea,saveColumn,startColumnU,tempColumnCount,
	    elementU,numberInColumn,indexRowU,aBits,indexL,
	    multipliersL,tolerance);
    cilk_sync;
  } else {
  int intsPerColumn = (lengthArea+31)>>5;
  CoinFactorizationDouble * COIN_RESTRICT area =eArea+first*lengthArea;
  for (int jColumn = first; jColumn < last; jColumn++ ) {
    CoinSimplexInt iColumn = saveColumn[jColumn];
    CoinBigIndex startColumn = startColumnU[iColumn];
    int numberNow=tempColumnCount[jColumn];
    CoinFactorizationDouble thisPivotValue = area[numberInPivotColumn];
    area[numberInPivotColumn]=0.0;
    int n=0;
    int positionLargest=-1;
    double largest=numberNow ? fabs(elementU[startColumn]) : 0.0;
    unsigned int * aBitsThis = aBits+jColumn*intsPerColumn;
#if AVX2==0
    for (CoinSimplexInt j = 0; j < numberInPivotColumn; j++ ) {
      CoinFactorizationDouble value = area[j] - thisPivotValue * multipliersL[j];
      CoinSimplexDouble absValue = fabs ( value );
      area[j]=0.0;
      if ( absValue > tolerance ) {
	area[n] = value;
	if (absValue>largest) {
	  largest=absValue;
	  positionLargest=n;
	}
	n++;
      } else {
	// say zero
	int word = j>>5;
	int unsetBit=j-(word<<5);
	aBitsThis[word]&= ~(1<<unsetBit);
      }
    }
#else
    // for compiler error
    CoinFactorizationDouble * multipliers = const_cast<CoinFactorizationDouble *>(multipliersL);
    int nDo=(numberInPivotColumn+3)&(~3);
    //double temp[4] __attribute__ ((aligned (32)));
    __m256d pivot = _mm256_broadcast_sd(&thisPivotValue);
    for (CoinSimplexInt j = 0; j < nDo; j+=4 ) {
      __m256d a=_mm256_load_pd(area+j);
      __m256d m=_mm256_load_pd(multipliers+j);
      a -= pivot*m;
      *reinterpret_cast<__m256d *>(area+j)=a; //_mm256_store_pd ((area+j), a);
    }
    for (CoinSimplexInt j = 0; j < numberInPivotColumn; j++ ) {
      CoinFactorizationDouble value = area[j];
      CoinSimplexDouble absValue = fabs ( value );
      area[j]=0.0;
      if ( absValue > tolerance ) {
	area[n] = value;
	if (absValue>largest) {
	  largest=absValue;
	  positionLargest=n;
	}
	n++;
      } else {
	// say zero
	int word = j>>5;
	int unsetBit=j-(word<<5);
	aBitsThis[word]&= ~(1<<unsetBit);
      }
    }
#endif
    int put = startColumn;
    if (positionLargest>=0) 
      positionLargest+=put+numberNow;
    put+=numberNow;
    int iA=0;
    for (int jj=0;jj<numberInPivotColumn;jj+=32) {
      unsigned int aThis=*aBitsThis;
      aBitsThis++; 
      for (int i=jj;i<CoinMin(jj+32,numberInPivotColumn);i++) {
	if ((aThis&1)!=0) {
	  indexRowU[put]=indexL[i];
	  elementU[put++]=area[iA];
	  area[iA++]=0.0;
	}
	aThis=aThis>>1;
      }
    }
    int numberNeeded=put-startColumn;
    if (positionLargest>=0) {
      //swap first and largest
      CoinBigIndex start=startColumn;
      int firstIndex=indexRowU[start];
      CoinFactorizationDouble firstValue=elementU[start];
      indexRowU[start]=indexRowU[positionLargest];
      elementU[start]=elementU[positionLargest];
      indexRowU[positionLargest]=firstIndex;
      elementU[positionLargest]=firstValue;
    }
    tempColumnCount[jColumn]=numberNeeded-numberInColumn[iColumn];
    assert (numberNeeded>=0);
    numberInColumn[iColumn]=numberNeeded;
    area += lengthArea;
  }
  }
}
#endif
static void pivotSome(int first, int last, int numberInPivotColumn,int lengthArea,int giveUp,
		      CoinFactorizationDouble * COIN_RESTRICT eArea,const int * COIN_RESTRICT saveColumn,
		      const int * COIN_RESTRICT startColumnU,int * COIN_RESTRICT tempColumnCount,
		      CoinFactorizationDouble * COIN_RESTRICT elementU, int * COIN_RESTRICT numberInColumn,
		      int * COIN_RESTRICT indexRowU, unsigned int * aBits,
		      const int * COIN_RESTRICT indexL,
		      const CoinFactorizationDouble * COIN_RESTRICT multipliersL, double tolerance)
{
  if (last-first>giveUp) {
    int mid=(last+first)>>1;
    cilk_spawn pivotSome(first,mid,numberInPivotColumn,lengthArea,giveUp,
	    eArea,saveColumn,startColumnU,tempColumnCount,
	    elementU,numberInColumn,indexRowU,aBits,indexL,
	    multipliersL,tolerance);
    pivotSome(mid,last,numberInPivotColumn,lengthArea,giveUp,
	    eArea,saveColumn,startColumnU,tempColumnCount,
	    elementU,numberInColumn,indexRowU,aBits,indexL,
	    multipliersL,tolerance);
    cilk_sync;
  } else {
    int intsPerColumn = (lengthArea+31)>>5;
    CoinFactorizationDouble * COIN_RESTRICT area =eArea+first*lengthArea;
    for (int jColumn = first; jColumn < last; jColumn++ ) {
      CoinSimplexInt iColumn = saveColumn[jColumn];
      CoinBigIndex startColumn = startColumnU[iColumn];
      int numberNow=tempColumnCount[jColumn];
      CoinFactorizationDouble thisPivotValue = elementU[startColumn-1];
      int start=startColumn+numberNow;
      int end=startColumn+numberInColumn[iColumn];
      for (CoinBigIndex j=start;j<end;j++) {
	CoinFactorizationDouble value=elementU[j];
	int iPut=indexRowU[j];
	assert (iPut<lengthArea);
	area[iPut]=value;
      }
      int n=0;
      int positionLargest=-1;
      double largest=numberNow ? fabs(elementU[startColumn]) : 0.0;
      unsigned int * aBitsThis = aBits+jColumn*intsPerColumn;
#if AVX2==0
      for (CoinSimplexInt j = 0; j < numberInPivotColumn; j++ ) {
	CoinFactorizationDouble value = area[j] - thisPivotValue * multipliersL[j];
	CoinSimplexDouble absValue = fabs ( value );
	area[j]=0.0;
	if ( absValue > tolerance ) {
	  area[n] = value;
	  if (absValue>largest) {
	    largest=absValue;
	    positionLargest=n;
	  }
	  n++;
	} else {
	  // say zero
	  int word = j>>5;
	  int unsetBit=j-(word<<5);
	  aBitsThis[word]&= ~(1<<unsetBit);
	}
      }
#else
      int nDo=(numberInPivotColumn+3)&(~3);
      // for compiler error
      CoinFactorizationDouble * multipliers = const_cast<CoinFactorizationDouble *>(multipliersL);
      //double temp[4] __attribute__ ((aligned (32)));
      __m256d pivot = _mm256_broadcast_sd(&thisPivotValue);
      for (CoinSimplexInt j = 0; j < nDo; j+=4 ) {
	__m256d a=_mm256_load_pd(area+j);
	__m256d m=_mm256_load_pd(multipliers+j);
	a -= pivot*m;
	*reinterpret_cast<__m256d *>(area+j)=a; //_mm256_store_pd ((area+j), a);
      }
      for (CoinSimplexInt j = 0; j < numberInPivotColumn; j++ ) {
	CoinFactorizationDouble value = area[j];
	CoinSimplexDouble absValue = fabs ( value );
	area[j]=0.0;
	if ( absValue > tolerance ) {
	  area[n] = value;
	  if (absValue>largest) {
	    largest=absValue;
	    positionLargest=n;
	  }
	  n++;
	} else {
	  // say zero
	  int word = j>>5;
	  int unsetBit=j-(word<<5);
	  aBitsThis[word]&= ~(1<<unsetBit);
	}
      }
#endif
      int put = startColumn;
      if (positionLargest>=0) 
	positionLargest+=put+numberNow;
      put+=numberNow;
      int iA=0;
      for (int jj=0;jj<numberInPivotColumn;jj+=32) {
	unsigned int aThis=*aBitsThis;
	aBitsThis++; 
	for (int i=jj;i<CoinMin(jj+32,numberInPivotColumn);i++) {
	  if ((aThis&1)!=0) {
	    indexRowU[put]=indexL[i];
	    elementU[put++]=area[iA];
	    area[iA++]=0.0;
	  }
	  aThis=aThis>>1;
	}
      }
      int numberNeeded=put-startColumn;
      if (positionLargest>=0) {
	//swap first and largest
	CoinBigIndex start=startColumn;
	int firstIndex=indexRowU[start];
	CoinFactorizationDouble firstValue=elementU[start];
	indexRowU[start]=indexRowU[positionLargest];	elementU[start]=elementU[positionLargest];
	indexRowU[positionLargest]=firstIndex;
	elementU[positionLargest]=firstValue;
      }
      tempColumnCount[jColumn]=numberNeeded-numberInColumn[iColumn];
      assert (numberNeeded>=0);
      numberInColumn[iColumn]=numberNeeded;
    }
  }
}
int
CoinAbcTypeFactorization::pivot ( CoinSimplexInt & pivotRow,
				  CoinSimplexInt & pivotColumn,
				  CoinBigIndex pivotRowPosition,
				  CoinBigIndex pivotColumnPosition,
				  int * COIN_RESTRICT markRow)
{
  CoinSimplexInt * COIN_RESTRICT indexColumnU = indexColumnU_.array();
  CoinBigIndex * COIN_RESTRICT startColumnU = startColumnU_.array();
  CoinSimplexInt * COIN_RESTRICT numberInColumn = numberInColumn_.array();
  CoinFactorizationDouble * COIN_RESTRICT elementU = elementU_.array();
  CoinSimplexInt * COIN_RESTRICT indexRowU = indexRowU_.array();
  CoinBigIndex * COIN_RESTRICT startRowU = startRowU_.array();
  CoinSimplexInt * COIN_RESTRICT numberInRow = numberInRow_.array();
  CoinFactorizationDouble * COIN_RESTRICT elementL = elementL_.array();
  CoinSimplexInt * COIN_RESTRICT indexRowL = indexRowL_.array();
  CoinSimplexInt * COIN_RESTRICT saveColumn = saveColumn_.array();
  CoinSimplexInt * COIN_RESTRICT nextRow = nextRow_.array();
  CoinSimplexInt * COIN_RESTRICT lastRow = lastRow_.array() ;
  int realPivotRow=fromSmallToBigRow_[pivotRow];
  //int realPivotColumn=fromSmallToBigColumn[pivotColumn];
  //store pivot columns (so can easily compress)
  CoinSimplexInt numberInPivotRow = numberInRow[pivotRow] - 1;
  CoinBigIndex startColumn = startColumnU[pivotColumn];
  CoinSimplexInt numberInPivotColumn = numberInColumn[pivotColumn] - 1;
  // temp
  CoinBigIndex endColumn = startColumn + numberInPivotColumn + 1;
  CoinBigIndex put = 0;
  CoinBigIndex startRow = startRowU[pivotRow];
  CoinBigIndex endRow = startRow + numberInPivotRow + 1;
#if ABC_SMALL<2
  // temp - switch off marker for valid row/column lookup
  sparseThreshold_=-1;
#endif

  if ( pivotColumnPosition < 0 ) {
    for ( pivotColumnPosition = startRow; pivotColumnPosition < endRow; pivotColumnPosition++ ) {
      CoinSimplexInt iColumn = indexColumnU[pivotColumnPosition];
      if ( iColumn != pivotColumn ) {
	saveColumn[put++] = iColumn;
      } else {
        break;
      }
    }
  } else {
    for (CoinBigIndex i = startRow ; i < pivotColumnPosition ; i++ ) {
      saveColumn[put++] = indexColumnU[i];
    }
  }
  assert (pivotColumnPosition<endRow);
  assert (indexColumnU[pivotColumnPosition]==pivotColumn);
  pivotColumnPosition++;
  for ( ; pivotColumnPosition < endRow; pivotColumnPosition++ ) {
    saveColumn[put++] = indexColumnU[pivotColumnPosition];
  }
  //take out this bit of indexColumnU
  CoinSimplexInt next = nextRow[pivotRow];
  CoinSimplexInt last = lastRow[pivotRow];

  nextRow[last] = next;
  lastRow[next] = last;
#ifdef SMALL_PERMUTE
  permuteAddress_[realPivotRow] = numberGoodU_;	//use for permute
#else
  permuteAddress_[pivotRow] = numberGoodU_;	//use for permute
#endif
  lastRow[pivotRow] = -2;
  numberInRow[pivotRow] = 0;
  //store column in L, compress in U and take column out
  CoinBigIndex l = lengthL_;

  if ( l + numberInPivotColumn > lengthAreaL_ ) {
    //need more memory
    if ((messageLevel_&4)!=0) 
      printf("more memory needed in middle of invert\n");
    return -99;
  }
  //l+=currentAreaL_->elementByColumn-elementL;
  CoinBigIndex lSave = l;
  CoinSimplexInt saveGoodL=numberGoodL_;
  CoinBigIndex *  COIN_RESTRICT startColumnL = startColumnL_.array();
  startColumnL[numberGoodL_] = l;	//for luck and first time
  numberGoodL_++;
  if ( pivotRowPosition < 0 ) {
    for ( pivotRowPosition = startColumn; pivotRowPosition < endColumn; pivotRowPosition++ ) {
      CoinSimplexInt iRow = indexRowU[pivotRowPosition];
      if ( iRow == pivotRow ) 
	break;
    }
  }
  assert (pivotRowPosition<endColumn);
  assert (indexRowU[pivotRowPosition]==pivotRow);
  CoinFactorizationDouble pivotElement = elementU[pivotRowPosition];
  CoinFactorizationDouble pivotMultiplier = 1.0 / pivotElement;
  endColumn--;
  indexRowU[pivotRowPosition]=indexRowU[endColumn];
  elementU[pivotRowPosition]=elementU[endColumn];
  double tolerance=zeroTolerance_;
  numberInPivotColumn=endColumn-startColumn;
  pivotRegionAddress_[numberGoodU_] = pivotMultiplier;
  markRow[pivotRow] = LARGE_SET;
  //compress pivot column (move pivot to front including saved)
  numberInColumn[pivotColumn] = 0;
  // modify so eArea aligned on 256 bytes boundary
  // also allow for numberInPivotColumn rounded to multiple of four
  CoinBigIndex added = numberInPivotRow * (numberInPivotColumn+4);
  CoinBigIndex spaceZeroed=added+((numberRowsSmall_+3)>>2);
  spaceZeroed=CoinMax(spaceZeroed,firstZeroed_);
  int maxBoth=CoinMax(numberInPivotRow,numberInPivotColumn);
  CoinBigIndex spaceOther=numberInPivotRow+maxBoth+numberInPivotRow*((numberInPivotColumn+31)>>5);
  // allow for new multipliersL
  spaceOther += 2*numberInPivotColumn+8; // 32 byte alignment wanted
  CoinBigIndex spaceNeeded=spaceZeroed+((spaceOther+1)>>1)+32;
#ifdef INT_IS_8
  abort();
#endif
  if (spaceNeeded>=lengthAreaUPlus_-lastEntryByColumnUPlus_) 
    return -99;
  // see if need to zero out
  CoinFactorizationDouble * COIN_RESTRICT eArea = elementUColumnPlusAddress_+lengthAreaUPlus_-spaceZeroed;
  CoinInt64 xx = reinterpret_cast<CoinInt64>(eArea);
  int iBottom = static_cast<int>(xx & 255);
  int offset = iBottom>>3;
  eArea -= offset;
  spaceZeroed=(elementUColumnPlusAddress_+lengthAreaUPlus_)-eArea;
  char * COIN_RESTRICT mark = reinterpret_cast<char *>(eArea+added);
  CoinFactorizationDouble * COIN_RESTRICT multipliersL = eArea-numberInPivotColumn;
  // adjust
  xx = reinterpret_cast<CoinInt64>(multipliersL);
  iBottom = static_cast<int>(xx & 31);
  offset = iBottom>>3;
  multipliersL -= offset;
  int * COIN_RESTRICT tempColumnCount = reinterpret_cast<int *>(multipliersL)-numberInPivotRow;
  int * COIN_RESTRICT tempCount = tempColumnCount-maxBoth;
  int * COIN_RESTRICT others = tempCount-numberInPivotRow;
  for (CoinBigIndex i = startColumn; i < endColumn; i++ ) {
    CoinSimplexInt iRow = indexRowU[i];
    //double value=elementU[i]*pivotMultiplier;
    markRow[iRow] = l - lSave;
    indexRowL[l] = iRow;
    multipliersL[l-lSave] = elementU[i];
    l++;
  }
  startColumnL[numberGoodL_] = l;
  lengthL_ = l;
  //use end of L for temporary space BUT AVX
  CoinSimplexInt * COIN_RESTRICT indexL = &indexRowL[lSave];
  //CoinFactorizationDouble * COIN_RESTRICT multipliersL = &elementL[lSave];
  for (int i=0;i<numberInPivotColumn;i++) 
    multipliersL[i] *= pivotMultiplier;
  // could make multiple of 4 for AVX (then adjust previous computation)
  int lengthArea=((numberInPivotColumn+4)>>2)<<2;
  // zero out rest
  memset(multipliersL+numberInPivotColumn,0,(lengthArea-numberInPivotColumn)*sizeof(CoinFactorizationDouble));
  int intsPerColumn = (lengthArea+31)>>5;
  unsigned int * COIN_RESTRICT aBits = reinterpret_cast<unsigned int *>(others-intsPerColumn*numberInPivotRow);
  memset(aBits,0xff,intsPerColumn*4*numberInPivotRow);
  if (spaceZeroed>firstZeroed_) {
    CoinAbcMemset0(eArea,spaceZeroed-firstZeroed_);
    firstZeroed_=spaceZeroed;
  }
#ifndef NDEBUG
  for (int i=0;i<added;i++)
    assert (!eArea[i]);
  for (int i=0;i<firstZeroed_;i++)
    assert (!elementUColumnPlusAddress_[lengthAreaUPlus_-firstZeroed_+i]);
  for (int i=0;i<numberRowsSmall_;i++)
    assert (!mark[i]);
#endif
  // could be 2 if dense
  int status=0;
  CoinSimplexInt *  COIN_RESTRICT numberInColumnPlus = numberInColumnPlus_.array();
  for (int jColumn = 0; jColumn < numberInPivotRow; jColumn++ ) {
    CoinSimplexInt iColumn = saveColumn[jColumn];
    mark[iColumn]=1;
  }
  // can go parallel
  // make two versions (or use other way for one) and choose serial if small added?
  CoinSimplexInt *  COIN_RESTRICT nextColumn = nextColumn_.array();
#define ABC_PARALLEL_PIVOT
#ifdef ABC_PARALLEL_PIVOT
#define cilk_for_query cilk_for
#else
#define cilk_for_query for
#endif
#if 0
  for (int i=0;i<numberRowsSmall_;i++)
    assert (numberInColumn[i]>=0);
#endif
  cilk_for_query (int jColumn = 0; jColumn < numberInPivotRow; jColumn++ ) {
    CoinSimplexInt iColumn = saveColumn[jColumn];
    CoinBigIndex startColumn = startColumnU[iColumn];
    CoinBigIndex endColumn = startColumn + numberInColumn[iColumn];
    CoinFactorizationDouble thisPivotValue = 0.0;
    CoinBigIndex put = startColumn;
    CoinBigIndex positionLargest = -1;
    double largest=0.0;
    CoinBigIndex position = startColumn;
    int iRow = indexRowU[position];
    CoinFactorizationDouble value = elementU[position];
    CoinSimplexInt mark = markRow[iRow];
    CoinBigIndex end=endColumn-1;
    while ( position < end) {
      if ( mark == LARGE_UNSET ) {
	//keep
	if (fabs(value)>largest) {
	  positionLargest=put;
	  largest=fabs(value);
	}
	indexRowU[put] = iRow;
	elementU[put++] = value;
      } else if ( mark != LARGE_SET ) {
	//will be updated - move to end
	CoinFactorizationDouble value2=value;
	int mark2=mark;
	iRow = indexRowU[end];
	value = elementU[end];
	mark = markRow[iRow];
	indexRowU[end]=mark2;
	elementU[end]=value2;
	end--;
	continue;
      } else {
	thisPivotValue = value;
      }
      position++;
      iRow = indexRowU[position];
      value = elementU[position];
      mark = markRow[iRow];
    }
    // now last in column
    if ( mark == LARGE_UNSET ) {
      //keep
      if (fabs(value)>largest) {
	positionLargest=put;
	largest=fabs(value);
      }
      indexRowU[put] = iRow;
      elementU[put++] = value;
    } else if ( mark != LARGE_SET ) {
      //will be updated - move to end
      indexRowU[end]=mark;
      elementU[end]=value;
    } else {
      thisPivotValue = value;
    }
    //swap largest
    if (positionLargest>=0) {
      double largestValue=elementU[positionLargest];
      int largestIndex=indexRowU[positionLargest];
      if (positionLargest!=startColumn+1&&put>startColumn+1) {
	elementU[positionLargest]=elementU[startColumn+1];
	indexRowU[positionLargest]=indexRowU[startColumn+1];
	elementU[startColumn+1] = largestValue;
	indexRowU[startColumn+1] = largestIndex;
      }
      int firstIndex=indexRowU[startColumn];
      CoinFactorizationDouble firstValue=elementU[startColumn];
      // move first to end
      elementU[put] = firstValue;
      indexRowU[put++] = firstIndex;
    } else {
      put++;
    }
    elementU[startColumn] = thisPivotValue;
    indexRowU[startColumn] = realPivotRow;
    startColumn++;
    startColumnU[iColumn]=startColumn;
    numberInColumnPlus[iColumn]++;
    int numberNow=put-startColumn;
    tempColumnCount[jColumn]=numberNow;
    numberInColumn[iColumn]--;
#ifndef NDEBUG
	for (int i=startColumn+numberNow;i<startColumn+numberInColumn[iColumn];i++)
	  assert (indexRowU[i]<numberInPivotColumn);
#endif
  }
  //int tempxx[1000];
  //memcpy(tempxx,tempColumnCount,numberInPivotRow*sizeof(int));
#if 0
  for (int i=0;i<numberRowsSmall_;i++)
    assert (numberInColumn[i]>=0);
#endif
  //parallel
  mark[pivotColumn]=1;
  cilk_for_query (int jRow = 0; jRow < numberInPivotColumn; jRow++ ) {
    CoinSimplexInt iRow = indexL[jRow];
    CoinBigIndex startRow = startRowU[iRow];
    CoinBigIndex endRow = startRow + numberInRow[iRow];
    CoinBigIndex put=startRow;
    // move earlier but set numberInRow to final
    for (CoinBigIndex i = startRow ; i < endRow; i++ ) {
      int iColumn = indexColumnU[i];
      if (!mark[iColumn]) 
	indexColumnU[put++]=iColumn;
    }
    numberInRow[iRow]=put-startRow;
  }
  mark[pivotColumn]=0;
#ifdef DENSE_TRY
  int numberZeroOther=0;
  for (int jRow = 0; jRow < numberInPivotColumn; jRow++ ) {
    CoinSimplexInt iRow = indexL[jRow];
    if (numberInRow[iRow]) {
      //printf("Cantdo INrow %d incolumn %d nother %d\n",
      //     numberInPivotRow,numberInPivotColumn,numberZeroOther);
      numberZeroOther=-1;
      break;
    }
  }
  if (!numberZeroOther) {
    for (int jColumn = 0; jColumn < numberInPivotRow; jColumn++ ) {
      if (!tempColumnCount[jColumn])
	others[numberZeroOther++]=jColumn;
    }
  }
#endif
  // serial
  for (int jColumn = 0; jColumn < numberInPivotRow; jColumn++ ) {
    CoinSimplexInt iColumn = saveColumn[jColumn];
    CoinBigIndex startColumn = startColumnU[iColumn];
    int numberNow=tempColumnCount[jColumn];
    int numberNeeded=numberNow+numberInPivotColumn;
    tempCount[jColumn]=numberInColumn[iColumn];
    //how much space have we got
    CoinSimplexInt next = nextColumn[iColumn];
    CoinBigIndex space = startColumnU[next] - startColumn - numberInColumnPlus[next];
    // do moves serially but fill in in parallel!
    if ( numberNeeded > space ) {
      //getColumnSpace also moves fixed part
      if ( !getColumnSpace ( iColumn, numberNeeded ) ) {
	return -99;
      }
    }
    numberInColumn[iColumn]=numberNeeded;
  }
  // move counts back
  for (int jColumn = 0; jColumn < numberInPivotRow; jColumn++ ) {
    CoinSimplexInt iColumn = saveColumn[jColumn];
    numberInColumn[iColumn]=tempCount[jColumn];
  }
  // giveUp should be more sophisticated and also vary in next three
#if ABC_PARALLEL==2
  int giveUp=CoinMax(numberInPivotRow/(parallelMode_+1+(parallelMode_>>1)),16);
#else
  int giveUp=numberInPivotRow;
#endif
#ifdef DENSE_TRY
  // If we keep full area - then can try doing many pivots in this function
  // Go round if there are any others just in this block
  bool inAreaAlready=false;
  if (numberZeroOther>4) {
    //printf("Cando INrow %d incolumn %d nother %d\n",
    //	   numberInPivotRow,numberInPivotColumn,numberZeroOther);
    int numberRowsTest=(numberRowsLeft_-numberZeroOther+0)&~7; 
    if (numberRowsLeft_-numberZeroOther<=numberRowsTest) {
      //see if would go dense
      int numberSubtracted=0;
      // would have to be very big for parallel
      for (int jColumn = 0; jColumn < numberInPivotRow; jColumn++ ) {
	CoinSimplexInt iColumn = saveColumn[jColumn];
	numberSubtracted += numberInColumn[iColumn]-tempColumnCount[jColumn];
      }
      // could do formula? - work backwards
      CoinBigIndex saveTotalElements=totalElements_;
      int saveNumberRowsLeft=numberRowsLeft_;
      int numberOut=numberRowsLeft_-numberRowsTest-1;
      numberRowsLeft_=numberRowsTest;
      CoinBigIndex total= totalElements_-numberSubtracted;
      int bestGoDense=-1;
      for (;numberRowsLeft_<saveNumberRowsLeft;numberRowsLeft_+=8) {
	totalElements_ = total + (numberInPivotColumn-numberOut)*(numberInPivotRow-numberOut);
	int goDense=wantToGoDense();
	if (goDense==2)
	  bestGoDense=numberRowsLeft_;
	else
	  break;
	numberOut-=8;
      }
      totalElements_=saveTotalElements;
      numberRowsLeft_=saveNumberRowsLeft;
      // see if we need to stop early
      if (bestGoDense>numberRowsLeft_-numberZeroOther)
	numberZeroOther=numberRowsLeft_-bestGoDense;;
    }
  } else {
    numberZeroOther=-1;
  }
  if (numberZeroOther>0) {
    // do all except last
    pivotStartup(0,numberInPivotRow,numberInPivotColumn,lengthArea,giveUp,
		 eArea,saveColumn,startColumnU,tempColumnCount,
		 elementU,numberInColumn,indexRowU);
    assert (tempColumnCount[0]>=0);
    for (int iTry=numberZeroOther-1;iTry>=0;iTry--) {
      pivotWhile(0,numberInPivotRow,numberInPivotColumn,lengthArea,giveUp,
		 eArea,multipliersL);
    assert (tempColumnCount[0]>=0);
      // find new pivot row
      int jPivotColumn=others[iTry];
      CoinFactorizationDouble * COIN_RESTRICT area =eArea+jPivotColumn*lengthArea;
      double largest=tolerance;
      int jPivotRow=-1;
      for (CoinSimplexInt j = 0; j < numberInPivotColumn; j++ ) {
	CoinFactorizationDouble value = area[j];
	CoinSimplexDouble absValue = fabs ( value );
	if (absValue>largest) {
	  largest=absValue;
	  jPivotRow=j;
	}
      }
      if (jPivotRow>=0) {
	markRow[pivotRow] = LARGE_UNSET;
	//modify linked list for pivots
	deleteLink ( pivotRow );
	deleteLink ( pivotColumn + numberRows_ );
	// put away last
	afterPivot(pivotRow,pivotColumn);
	if (reinterpret_cast<unsigned int *>(elementUColumnPlusAddress_+lastEntryByColumnUPlus_)>aBits) {
#ifndef NDEBUG
	  printf("? dense\n");
#endif
	  return -99;
	}
	// new ones
	pivotRow=indexL[jPivotRow];
	pivotColumn=saveColumn[jPivotColumn];
	// now do stuff for pivot
	realPivotRow=fromSmallToBigRow_[pivotRow];
	//int realPivotColumn=fromSmallToBigColumn[pivotColumn];
	//store pivot columns (so can easily compress)
	CoinBigIndex startColumn = startColumnU[pivotColumn];
	CoinBigIndex endColumn = startColumn + tempColumnCount[jPivotColumn];
	CoinSimplexInt next = nextRow[pivotRow];
	CoinSimplexInt last = lastRow[pivotRow];

	nextRow[last] = next;
	lastRow[next] = last;
#ifdef SMALL_PERMUTE
	permuteAddress_[realPivotRow] = numberGoodU_;	//use for permute
#else
	permuteAddress_[pivotRow] = numberGoodU_;	//use for permute
#endif
	lastRow[pivotRow] = -2;
	numberInRow[pivotRow] = 0;
    assert (tempColumnCount[0]>=0);
	//store column in L, compress in U and take column out
	CoinBigIndex l = lengthL_;
	if ( l + numberInPivotColumn > lengthAreaL_ ) {
	  //need more memory
	  if ((messageLevel_&4)!=0) 
	    printf("more memory needed in middle of invert\n");
	  return -99;
	}
	//CoinBigIndex *  COIN_RESTRICT startColumnL = startColumnL_.array();
	numberGoodL_++;
	// move to L and move last
	numberInPivotColumn--;
    assert (tempColumnCount[0]>=0);
	if (jPivotRow!=numberInPivotColumn) {
	  // swap
	  for (int jColumn = 0; jColumn < numberInPivotRow; jColumn++ ) {
	    CoinFactorizationDouble * COIN_RESTRICT area =eArea+jColumn*lengthArea;
	    CoinFactorizationDouble thisValue = area[jPivotRow];
	    area[jPivotRow]=area[numberInPivotColumn];
	    area[numberInPivotColumn]=thisValue;
	  }
	  // do indexL in last one
	  int tempI=indexL[numberInPivotColumn];
	  CoinFactorizationDouble tempD=multipliersL[numberInPivotColumn];
	  indexL[numberInPivotColumn]=indexL[jPivotRow];
	  multipliersL[numberInPivotColumn]=multipliersL[jPivotRow];
	  indexL[jPivotRow]=tempI;
	  multipliersL[jPivotRow]=tempD;
	  jPivotRow=numberInPivotColumn;
	}
    assert (tempColumnCount[0]>=0);
	numberInPivotRow--;
	CoinFactorizationDouble pivotElement = area[numberInPivotColumn];
	area[numberInPivotColumn]=0.0;
	// put last one in now
	CoinFactorizationDouble * COIN_RESTRICT areaLast =eArea+numberInPivotRow*lengthArea;
	// swap columns
	saveColumn[jPivotColumn]=saveColumn[numberInPivotRow];
	saveColumn[numberInPivotRow]=pivotColumn;
	//int temp=tempColumnCount[jPivotColumn];
	tempColumnCount[jPivotColumn]=tempColumnCount[numberInPivotRow];
	totalElements_-=numberInColumn[pivotColumn];
	mark[pivotColumn]=0;
    assert (tempColumnCount[0]>=0);
	for (int jRow=0;jRow<numberInPivotColumn+1;jRow++) {
	  CoinFactorizationDouble temp=areaLast[jRow];
	  areaLast[jRow]=area[jRow];
	  area[jRow]=temp;
	}
	areaLast[numberInPivotColumn]=0.0;
	// swap rows in area and save
	for (int jColumn = 0; jColumn < numberInPivotRow; jColumn++ ) {
	  CoinFactorizationDouble * COIN_RESTRICT area =eArea+jColumn*lengthArea;
	  CoinFactorizationDouble thisPivotValue = area[numberInPivotColumn];
    assert (tempColumnCount[0]>=0);
	  area[numberInPivotColumn]=thisPivotValue;
	  if (fabs(thisPivotValue)>tolerance) {
	    CoinSimplexInt iColumn = saveColumn[jColumn];
	    CoinBigIndex startColumn = startColumnU[iColumn];
	    int nKeep=tempColumnCount[jColumn];
	    if (nKeep) {
	      if (nKeep>1) {
		// need to move one to end
		elementU[startColumn+nKeep]=elementU[startColumn+1];
		indexRowU[startColumn+nKeep]=indexRowU[startColumn+1];
	      }
	      // move largest
	      elementU[startColumn+1]=elementU[startColumn];
	      indexRowU[startColumn+1]=indexRowU[startColumn];
	    }
	    elementU[startColumn] = thisPivotValue;
	    indexRowU[startColumn] = realPivotRow;
	    startColumn++;
	    startColumnU[iColumn]=startColumn;
	    numberInColumnPlus[iColumn]++;
	    //numberInColumn[iColumn]--;
	  }
	}
    assert (tempColumnCount[0]>=0);
	// put away last (but remember count is one out)
	CoinAbcMemcpy(elementL+lSave,multipliersL,numberInPivotColumn+1);
	lSave=l;
	for (int i=0;i<numberInPivotColumn;i++) {
	  CoinFactorizationDouble value=areaLast[i];
	  areaLast[i]=0.0;
	  int iRow=indexL[i];
	  indexRowL[l] = iRow;
	  assert (iRow>=0);
	  multipliersL[l-lSave] = value;
	  l++;
	}
	startColumnL[numberGoodL_] = l;
	lengthL_ = l;
	CoinFactorizationDouble pivotMultiplier = 1.0 / pivotElement;
	pivotRegionAddress_[numberGoodU_] = pivotMultiplier;
	numberInColumn[pivotColumn] = 0;
	indexL = &indexRowL[lSave];
	//adjust
	for (CoinSimplexInt j = 0; j < numberInPivotColumn; j++ ) {
	  multipliersL[j] *= pivotMultiplier;
	}
	// zero out last
	multipliersL[numberInPivotColumn]=0.0;
      } else {
	// singular
	// give up
	break;
      }
    }
    assert (tempColumnCount[0]>=0);
#if 0
    // zero out extra bits
    int add1=numberInPivotColumn>>5;
    unsigned int * aBitsThis = aBits;
    int left=numberInPivotColumn-(add1<<5);
    unsigned int mark1 = (1<<(left-1)-1);

    int jColumn;
    for (jColumn = 0; jColumn < numberInPivotRow; jColumn++ ) {
      memset(aBitsThis+add1,0,(intsPerColumn-add1)*sizeof(int));
      aBitsThis += intsPerColumn;
    }
    for (; jColumn < numberInPivotRow+numberZeroOther; jColumn++ ) {
      memset(aBitsThis,0,intsPerColumn*sizeof(int));
      aBitsThis += intsPerColumn;
    }
#endif
    // last bit
    inAreaAlready=true;
  }
#endif 
#if 0
  for (int i=0;i<numberRowsSmall_;i++)
    assert (numberInColumn[i]>=0);
#endif
  //if (added<1000)
  //giveUp=CoinMax(); //??
#ifdef DENSE_TRY
  if (!inAreaAlready) {
#endif
    pivotSome(0,numberInPivotRow,numberInPivotColumn,lengthArea,giveUp,
	      eArea,saveColumn,startColumnU,tempColumnCount,
	      elementU,numberInColumn,indexRowU,aBits,indexL,
	      multipliersL,tolerance);
#ifdef DENSE_TRY
  } else {
    pivotSomeAfter(0,numberInPivotRow,numberInPivotColumn,lengthArea,giveUp,
	      eArea,saveColumn,startColumnU,tempColumnCount,
	      elementU,numberInColumn,indexRowU,aBits,indexL,
	      multipliersL,tolerance);
  }
#endif
#if 0
  for (int i=0;i<numberRowsSmall_;i++)
    assert (numberInColumn[i]>=0);
#endif
  //serial
  for (int jRow = 0; jRow < numberInPivotColumn; jRow++ ) {
    CoinSimplexInt iRow = indexL[jRow];
    CoinSimplexInt next = nextRow[iRow];
    CoinBigIndex space = startRowU[next] - startRowU[iRow];
    // assume full
    int numberNeeded = numberInPivotRow+numberInRow[iRow];
    tempCount[jRow]=numberInRow[iRow];
    numberInRow[iRow]=numberNeeded;
    if ( space < numberNeeded ) {
      if ( !getRowSpace ( iRow, numberNeeded ) ) {
	return -99;
      }
    }
    lastEntryByRowU_ = CoinMax(startRowU[iRow]+numberNeeded,lastEntryByRowU_);
    // in case I got it wrong!
    if (lastEntryByRowU_>lengthAreaU_) {
      return -99;
    }
  }
  // move counts back
  for (int jRow = 0; jRow < numberInPivotColumn; jRow++ ) {
    CoinSimplexInt iRow = indexL[jRow];
    numberInRow[iRow]=tempCount[jRow];
  }
  // parallel
  cilk_for_query (int jRow = 0; jRow < numberInPivotColumn; jRow++ ) {
    CoinSimplexInt iRow = indexL[jRow];
    CoinBigIndex startRow = startRowU[iRow];
    CoinBigIndex put = startRow + numberInRow[iRow];
    int iWord = jRow>>5;
    unsigned int setBit=1<<(jRow&31);
    unsigned int * aBitsThis = aBits+iWord;
    for (int i=0;i<numberInPivotRow;i++) {
      if ((aBitsThis[0]&setBit)!=0) {
	indexColumnU[put++]=saveColumn[i];
      }
      aBitsThis += intsPerColumn;
    }
    markRow[iRow] = LARGE_UNSET;
    numberInRow[iRow] = put-startRow;
  }
  //memset(aBits,0x00,intsPerColumn*4*(numberInPivotRow+numberZeroOther));
  added=0;
  //int addedX=0;
  for (int jColumn = 0; jColumn < numberInPivotRow; jColumn++ ) {
    CoinSimplexInt iColumn = saveColumn[jColumn];
    for (int i=startColumnU[iColumn];i<startColumnU[iColumn]+numberInColumn[iColumn];i++)
      assert (indexRowU[i]>=0&&indexRowU[i]<numberRowsSmall_);
    mark[iColumn]=0;
    added += tempColumnCount[jColumn];
    modifyLink ( iColumn + numberRows_, numberInColumn[iColumn] );
    //addedX += numberInColumn[iColumn]-tempxx[jColumn];
  }
#ifndef NDEBUG
  for (int i=0;i<added;i++)
    assert (!eArea[i]);
  for (int i=0;i<firstZeroed_;i++)
    assert (!elementUColumnPlusAddress_[lengthAreaUPlus_-firstZeroed_+i]);
  for (int i=0;i<numberRowsSmall_;i++)
    assert (!mark[i]);
#endif
  //double ratio=((double)addedX)/((double)(numberInPivotRow*numberInPivotColumn)); 
#if 0 //def DENSE_TRY
  if (numberZeroOther>10000)
    printf("INrow %d incolumn %d nzero %d\n",
	   numberInPivotRow,numberInPivotColumn,numberZeroOther);
#endif
  //printf("INrow %d incolumn %d nzero %d ratio %g\n",
  //	 numberInPivotRow,numberInPivotColumn,numberZeroOther,ratio);
  for (int i=0;i<numberInPivotColumn;i++) {
    int iRow=indexL[i];
    modifyLink(iRow,numberInRow[iRow]);
  }
  CoinAbcMemcpy(elementL+lSave,multipliersL,numberInPivotColumn);
#ifdef DENSE_TRY
  int iLBad=saveGoodL;
  put=startColumnL[iLBad];
  for (int iL=iLBad;iL<numberGoodL_;iL++) {
    CoinBigIndex start=startColumnL[iL];
    startColumnL[iL]=put;
    for (CoinBigIndex j=start;j<startColumnL[iL+1];j++) {
      if (elementL[j]) {
	elementL[put]=elementL[j];
	indexRowL[put++]=fromSmallToBigRow_[indexRowL[j]];
      }
    }
  }
  startColumnL[numberGoodL_]=put;
  lengthL_=put;
#else
  // redo L indices
  for (CoinBigIndex j=lSave;j<startColumnL[numberGoodL_];j++) {
    //assert (fabs(elementL[j])>=tolerance) ;
    int iRow=indexRowL[j];
    indexRowL[j] = fromSmallToBigRow_[iRow];
  }
#endif
  markRow[pivotRow] = LARGE_UNSET;
  //modify linked list for pivots
  deleteLink ( pivotRow );
  deleteLink ( pivotColumn + numberRows_ );
  totalElements_ += added;
  return status;
}
#endif
// nonparallel pivot
int
CoinAbcTypeFactorization::pivot ( CoinSimplexInt pivotRow,
				  CoinSimplexInt pivotColumn,
				  CoinBigIndex pivotRowPosition,
				  CoinBigIndex pivotColumnPosition,
				  CoinFactorizationDouble * COIN_RESTRICT work,
				  CoinSimplexUnsignedInt * COIN_RESTRICT workArea2,
				  CoinSimplexInt increment2,
				  int * COIN_RESTRICT markRow)
{
  CoinSimplexInt * COIN_RESTRICT indexColumnU = indexColumnU_.array();
  CoinBigIndex * COIN_RESTRICT startColumnU = startColumnU_.array();
  CoinSimplexInt * COIN_RESTRICT numberInColumn = numberInColumn_.array();
  CoinFactorizationDouble * COIN_RESTRICT elementU = elementU_.array();
  CoinSimplexInt * COIN_RESTRICT indexRowU = indexRowU_.array();
  CoinBigIndex * COIN_RESTRICT startRowU = startRowU_.array();
  CoinSimplexInt * COIN_RESTRICT numberInRow = numberInRow_.array();
  CoinFactorizationDouble * COIN_RESTRICT elementL = elementL_.array();
  CoinSimplexInt * COIN_RESTRICT indexRowL = indexRowL_.array();
  CoinSimplexInt * COIN_RESTRICT saveColumn = saveColumn_.array();
  CoinSimplexInt * COIN_RESTRICT nextRow = nextRow_.array();
  CoinSimplexInt * COIN_RESTRICT lastRow = lastRow_.array() ;
#ifdef SMALL_PERMUTE
  int realPivotRow=fromSmallToBigRow_[pivotRow];
  //int realPivotColumn=fromSmallToBigColumn[pivotColumn];
#endif
  //store pivot columns (so can easily compress)
  CoinSimplexInt numberInPivotRow = numberInRow[pivotRow] - 1;
  CoinBigIndex startColumn = startColumnU[pivotColumn];
  CoinSimplexInt numberInPivotColumn = numberInColumn[pivotColumn] - 1;
  // temp
  CoinBigIndex endColumn = startColumn + numberInPivotColumn + 1;
  CoinBigIndex put = 0;
  CoinBigIndex startRow = startRowU[pivotRow];
  CoinBigIndex endRow = startRow + numberInPivotRow + 1;
#if ABC_SMALL<2
  // temp - switch off marker for valid row/column lookup
  sparseThreshold_=-1;
#endif

  if ( pivotColumnPosition < 0 ) {
    for ( pivotColumnPosition = startRow; pivotColumnPosition < endRow; pivotColumnPosition++ ) {
      CoinSimplexInt iColumn = indexColumnU[pivotColumnPosition];
      if ( iColumn != pivotColumn ) {
	saveColumn[put++] = iColumn;
      } else {
#if BOTH_WAYS
#endif
        break;
      }
    }
  } else {
    for (CoinBigIndex i = startRow ; i < pivotColumnPosition ; i++ ) {
      saveColumn[put++] = indexColumnU[i];
    }
  }
  assert (pivotColumnPosition<endRow);
  assert (indexColumnU[pivotColumnPosition]==pivotColumn);
  pivotColumnPosition++;
  for ( ; pivotColumnPosition < endRow; pivotColumnPosition++ ) {
    saveColumn[put++] = indexColumnU[pivotColumnPosition];
  }
  //take out this bit of indexColumnU
  CoinSimplexInt next = nextRow[pivotRow];
  CoinSimplexInt last = lastRow[pivotRow];

  nextRow[last] = next;
  lastRow[next] = last;
#ifdef SMALL_PERMUTE
  permuteAddress_[realPivotRow] = numberGoodU_;	//use for permute
#else
  permuteAddress_[pivotRow] = numberGoodU_;	//use for permute
#endif
  lastRow[pivotRow] = -2;
  numberInRow[pivotRow] = 0;
  //store column in L, compress in U and take column out
  CoinBigIndex l = lengthL_;

  if ( l + numberInPivotColumn > lengthAreaL_ ) {
    //need more memory
    if ((messageLevel_&4)!=0) 
      printf("more memory needed in middle of invert\n");
    return -99;
  }
  //l+=currentAreaL_->elementByColumn-elementL;
  CoinBigIndex lSave = l;

  CoinBigIndex *  COIN_RESTRICT startColumnL = startColumnL_.array();
  startColumnL[numberGoodL_] = l;	//for luck and first time
  numberGoodL_++;
  startColumnL[numberGoodL_] = l + numberInPivotColumn;
  lengthL_ += numberInPivotColumn;
  if ( pivotRowPosition < 0 ) {
    for ( pivotRowPosition = startColumn; pivotRowPosition < endColumn; pivotRowPosition++ ) {
      CoinSimplexInt iRow = indexRowU[pivotRowPosition];
      if ( iRow != pivotRow ) {
	indexRowL[l] = iRow;
	elementL[l] = elementU[pivotRowPosition];
	markRow[iRow] = l - lSave;
	l++;
	//take out of row list
	CoinBigIndex start = startRowU[iRow];
	CoinBigIndex end = start + numberInRow[iRow];
	CoinBigIndex where = start;

	while ( indexColumnU[where] != pivotColumn ) {
	  where++;
	}			/* endwhile */
#if DEBUG_COIN
	if ( where >= end ) {
	  abort (  );
	}
#endif
#if BOTH_WAYS
#endif
	indexColumnU[where] = indexColumnU[end - 1];
	numberInRow[iRow]--;
      } else {
	break;
      }
    }
  } else {
    CoinBigIndex i;

    for ( i = startColumn; i < pivotRowPosition; i++ ) {
      CoinSimplexInt iRow = indexRowU[i];

      markRow[iRow] = l - lSave;
      indexRowL[l] = iRow;
      elementL[l] = elementU[i];
      l++;
      //take out of row list
      CoinBigIndex start = startRowU[iRow];
      CoinBigIndex end = start + numberInRow[iRow];
      CoinBigIndex where = start;

      while ( indexColumnU[where] != pivotColumn ) {
	where++;
      }				/* endwhile */
#if DEBUG_COIN
      if ( where >= end ) {
	abort (  );
      }
#endif
#if BOTH_WAYS
#endif
      indexColumnU[where] = indexColumnU[end - 1];
      numberInRow[iRow]--;
      assert (numberInRow[iRow]>=0);
    }
  }
  assert (pivotRowPosition<endColumn);
  assert (indexRowU[pivotRowPosition]==pivotRow);
  CoinFactorizationDouble pivotElement = elementU[pivotRowPosition];
  CoinFactorizationDouble pivotMultiplier = 1.0 / pivotElement;

  pivotRegionAddress_[numberGoodU_] = pivotMultiplier;
  pivotRowPosition++;
  for ( ; pivotRowPosition < endColumn; pivotRowPosition++ ) {
    CoinSimplexInt iRow = indexRowU[pivotRowPosition];
    
    markRow[iRow] = l - lSave;
    indexRowL[l] = iRow;
    elementL[l] = elementU[pivotRowPosition];
    l++;
    //take out of row list
    CoinBigIndex start = startRowU[iRow];
    CoinBigIndex end = start + numberInRow[iRow];
    CoinBigIndex where = start;
    // could vectorize?
    while ( indexColumnU[where] != pivotColumn ) {
      where++;
    }				/* endwhile */
#if DEBUG_COIN
    if ( where >= end ) {
      abort (  );
    }
#endif
#if BOTH_WAYS
#endif
    indexColumnU[where] = indexColumnU[end - 1];
    numberInRow[iRow]--;
    assert (numberInRow[iRow]>=0);
  }
  markRow[pivotRow] = LARGE_SET;
  //compress pivot column (move pivot to front including saved)
  numberInColumn[pivotColumn] = 0;
  //use end of L for temporary space
  CoinSimplexInt * COIN_RESTRICT indexL = &indexRowL[lSave];
  CoinFactorizationDouble * COIN_RESTRICT multipliersL = &elementL[lSave];

  //adjust
  CoinSimplexInt j;

  for ( j = 0; j < numberInPivotColumn; j++ ) {
    multipliersL[j] *= pivotMultiplier;
  }
  CoinBigIndex added = numberInPivotRow * numberInPivotColumn;
  //zero out fill
  CoinBigIndex iErase;
  for ( iErase = 0; iErase < increment2 * numberInPivotRow;
	iErase++ ) {
    workArea2[iErase] = 0;
  }
  CoinSimplexUnsignedInt * COIN_RESTRICT temp2 = workArea2;
  CoinSimplexInt *  COIN_RESTRICT nextColumn = nextColumn_.array();

  //pack down and move to work
  CoinSimplexInt jColumn;
  for ( jColumn = 0; jColumn < numberInPivotRow; jColumn++ ) {
    CoinSimplexInt iColumn = saveColumn[jColumn];
    CoinBigIndex startColumn = startColumnU[iColumn];
    CoinBigIndex endColumn = startColumn + numberInColumn[iColumn];
    CoinSimplexInt iRow = indexRowU[startColumn];
    CoinFactorizationDouble value = elementU[startColumn];
    CoinSimplexDouble largest;
    CoinBigIndex put = startColumn;
    CoinBigIndex positionLargest = -1;
    CoinFactorizationDouble thisPivotValue = 0.0;

    //compress column and find largest not updated
    bool checkLargest;
    CoinSimplexInt mark = markRow[iRow];

    if ( mark == LARGE_UNSET ) {
      largest = fabs ( value );
      positionLargest = put;
      put++;
      checkLargest = false;
    } else {
      //need to find largest
      largest = 0.0;
      checkLargest = true;
      if ( mark != LARGE_SET ) {
	//will be updated
	work[mark] = value;
	CoinSimplexInt word = mark >> COINFACTORIZATION_SHIFT_PER_INT;
	CoinSimplexInt bit = mark & COINFACTORIZATION_MASK_PER_INT;

	temp2[word] = temp2[word] | ( 1 << bit );	//say already in counts
	added--;
      } else {
	thisPivotValue = value;
      }
    }
    CoinBigIndex i;
    for ( i = startColumn + 1; i < endColumn; i++ ) {
      iRow = indexRowU[i];
      value = elementU[i];
      CoinSimplexInt mark = markRow[iRow];

      if ( mark == LARGE_UNSET ) {
	//keep
	indexRowU[put] = iRow;
#if BOTH_WAYS
#endif
	elementU[put] = value;
	if ( checkLargest ) {
	  CoinSimplexDouble absValue = fabs ( value );

	  if ( absValue > largest ) {
	    largest = absValue;
	    positionLargest = put;
	  }
	}
	put++;
      } else if ( mark != LARGE_SET ) {
	//will be updated
	work[mark] = value;
	CoinSimplexInt word = mark >> COINFACTORIZATION_SHIFT_PER_INT;
	CoinSimplexInt bit = mark & COINFACTORIZATION_MASK_PER_INT;

	temp2[word] = temp2[word] | ( 1 << bit );	//say already in counts
	added--;
      } else {
	thisPivotValue = value;
      }
    }
    //slot in pivot
    elementU[put] = elementU[startColumn];
    indexRowU[put] = indexRowU[startColumn];
#if BOTH_WAYS
#endif
    if ( positionLargest == startColumn ) {
      positionLargest = put;	//follow if was largest
    }
    put++;
    elementU[startColumn] = thisPivotValue;
#ifdef SMALL_PERMUTE
    indexRowU[startColumn] = realPivotRow;
#else
    indexRowU[startColumn] = pivotRow;
#endif
#if BOTH_WAYS
#endif
    //clean up counts
    startColumn++;
    numberInColumn[iColumn] = put - startColumn;
    CoinSimplexInt *  COIN_RESTRICT numberInColumnPlus = numberInColumnPlus_.array();
    numberInColumnPlus[iColumn]++;
    startColumnU[iColumn]++;
    //how much space have we got
    CoinSimplexInt next = nextColumn[iColumn];
    CoinBigIndex space;

    space = startColumnU[next] - put - numberInColumnPlus[next];
    //assume no zero elements
    if ( numberInPivotColumn > space ) {
      //getColumnSpace also moves fixed part
      if ( !getColumnSpace ( iColumn, numberInPivotColumn ) ) {
	return -99;
      }
      //redo starts
      positionLargest = positionLargest + startColumnU[iColumn] - startColumn;
      startColumn = startColumnU[iColumn];
      put = startColumn + numberInColumn[iColumn];
    }
    CoinSimplexDouble tolerance = zeroTolerance_;

    //CoinSimplexInt * COIN_RESTRICT nextCount = this->nextCount();
    for ( j = 0; j < numberInPivotColumn; j++ ) {
      value = work[j] - thisPivotValue * multipliersL[j];
      CoinSimplexDouble absValue = fabs ( value );

      if ( absValue > tolerance ) {
	work[j] = 0.0;
	assert (put<lengthAreaU_); 
	elementU[put] = value;
	indexRowU[put] = indexL[j];
#if BOTH_WAYS
#endif
	if ( absValue > largest ) {
	  largest = absValue;
	  positionLargest = put;
	}
	put++;
      } else {
	work[j] = 0.0;
	added--;
	CoinSimplexInt word = j >> COINFACTORIZATION_SHIFT_PER_INT;
	CoinSimplexInt bit = j & COINFACTORIZATION_MASK_PER_INT;

	if ( temp2[word] & ( 1 << bit ) ) {
	  //take out of row list
	  iRow = indexL[j];
	  CoinBigIndex start = startRowU[iRow];
	  CoinBigIndex end = start + numberInRow[iRow];
	  CoinBigIndex where = start;

	  while ( indexColumnU[where] != iColumn ) {
	    where++;
	  }			/* endwhile */
#if DEBUG_COIN
	  if ( where >= end ) {
	    abort (  );
	  }
#endif
#if BOTH_WAYS
#endif
	  indexColumnU[where] = indexColumnU[end - 1];
	  numberInRow[iRow]--;
	} else {
	  //make sure won't be added
	  CoinSimplexInt word = j >> COINFACTORIZATION_SHIFT_PER_INT;
	  CoinSimplexInt bit = j & COINFACTORIZATION_MASK_PER_INT;

	  temp2[word] = temp2[word] | ( 1 << bit );	//say already in counts
	}
      }
    }
    numberInColumn[iColumn] = put - startColumn;
    //move largest
    if ( positionLargest >= 0 ) {
      value = elementU[positionLargest];
      iRow = indexRowU[positionLargest];
      elementU[positionLargest] = elementU[startColumn];
      indexRowU[positionLargest] = indexRowU[startColumn];
      elementU[startColumn] = value;
      indexRowU[startColumn] = iRow;
#if BOTH_WAYS
#endif
    }
    //linked list for column
    //if ( nextCount[iColumn + numberRows_] != -2 ) {
      //modify linked list
      //deleteLink ( iColumn + numberRows_ );
      //modifyLink ( iColumn + numberRows_, numberInColumn[iColumn] );
      //}
    temp2 += increment2;
  }
  //get space for row list
  CoinSimplexUnsignedInt * COIN_RESTRICT putBase = workArea2;
  CoinSimplexInt bigLoops = numberInPivotColumn >> COINFACTORIZATION_SHIFT_PER_INT;
  CoinSimplexInt i = 0;

  // do linked lists and update counts
  while ( bigLoops ) {
    bigLoops--;
    CoinSimplexInt bit;
    for ( bit = 0; bit < COINFACTORIZATION_BITS_PER_INT; i++, bit++ ) {
      CoinSimplexUnsignedInt *putThis = putBase;
      CoinSimplexInt iRow = indexL[i];

      //get space
      CoinSimplexInt number = 0;
      CoinSimplexInt jColumn;

      for ( jColumn = 0; jColumn < numberInPivotRow; jColumn++ ) {
	CoinSimplexUnsignedInt test = *putThis;

	putThis += increment2;
	test = 1 - ( ( test >> bit ) & 1 );
	number += test;
      }
      CoinSimplexInt next = nextRow[iRow];
      CoinBigIndex space;

      space = startRowU[next] - startRowU[iRow];
      number += numberInRow[iRow];
      if ( space < number ) {
	if ( !getRowSpace ( iRow, number ) ) {
	  return -99;
	}
      }
      // now do
      putThis = putBase;
      next = nextRow[iRow];
      number = numberInRow[iRow];
      CoinBigIndex end = startRowU[iRow] + number;
      CoinSimplexInt saveIndex = indexColumnU[startRowU[next]];

      //add in
      for ( jColumn = 0; jColumn < numberInPivotRow; jColumn++ ) {
	CoinSimplexUnsignedInt test = *putThis;

	putThis += increment2;
	test = 1 - ( ( test >> bit ) & 1 );
	indexColumnU[end] = saveColumn[jColumn];
#if BOTH_WAYS
#endif
	end += test;
      }
      lastEntryByRowU_ = CoinMax(end,lastEntryByRowU_);
      assert (lastEntryByRowU_<=lengthAreaU_);
      //put back next one in case zapped
      indexColumnU[startRowU[next]] = saveIndex;
#if BOTH_WAYS
#endif
      markRow[iRow] = LARGE_UNSET;
      number = end - startRowU[iRow];
      numberInRow[iRow] = number;
      //deleteLink ( iRow );
      //modifyLink ( iRow, number );
    }
    putBase++;
  }				/* endwhile */
  CoinSimplexInt bit;

  for ( bit = 0; i < numberInPivotColumn; i++, bit++ ) {
    CoinSimplexUnsignedInt *putThis = putBase;
    CoinSimplexInt iRow = indexL[i];

    //get space
    CoinSimplexInt number = 0;
    CoinSimplexInt jColumn;

    for ( jColumn = 0; jColumn < numberInPivotRow; jColumn++ ) {
      CoinSimplexUnsignedInt test = *putThis;

      putThis += increment2;
      test = 1 - ( ( test >> bit ) & 1 );
      number += test;
    }
    CoinSimplexInt next = nextRow[iRow];
    CoinBigIndex space;

    space = startRowU[next] - startRowU[iRow];
    number += numberInRow[iRow];
    if ( space < number ) {
      if ( !getRowSpace ( iRow, number ) ) {
	return -99;
      }
    }
    // now do
    putThis = putBase;
    next = nextRow[iRow];
    number = numberInRow[iRow];
    CoinBigIndex end = startRowU[iRow] + number;
    CoinSimplexInt saveIndex;

    saveIndex = indexColumnU[startRowU[next]];
    //if (numberRowsSmall_==2791&&iRow==1160)
    //printf("start %d end %d\n",startRowU[iRow],end);
    //add in
    for ( jColumn = 0; jColumn < numberInPivotRow; jColumn++ ) {
      CoinSimplexUnsignedInt test = *putThis;

      putThis += increment2;
      test = 1 - ( ( test >> bit ) & 1 );

      indexColumnU[end] = saveColumn[jColumn];
#if BOTH_WAYS
#endif
      end += test;
      //if (numberRowsSmall_==2791&&iRow==1160)
      //printf("start %d end %d test %d col %d\n",startRowU[iRow],end,test,saveColumn[jColumn]);
    }
    indexColumnU[startRowU[next]] = saveIndex;
    lastEntryByRowU_ = CoinMax(end,lastEntryByRowU_);
    assert (lastEntryByRowU_<=lengthAreaU_);
#if BOTH_WAYS
#endif
    markRow[iRow] = LARGE_UNSET;
    number = end - startRowU[iRow];
    numberInRow[iRow] = number;
    //deleteLink ( iRow );
    //modifyLink ( iRow, number );
  }
  for ( jColumn = 0; jColumn < numberInPivotRow; jColumn++ ) {
    CoinSimplexInt iColumn = saveColumn[jColumn];
    modifyLink ( iColumn + numberRows_, numberInColumn[iColumn] );
  }
  for (int i=0;i<numberInPivotColumn;i++) {
    int iRow=indexL[i];
    modifyLink(iRow,numberInRow[iRow]);
  }
#ifdef SMALL_PERMUTE
  // redo L indices
  for (CoinBigIndex j=lSave;j<startColumnL[numberGoodL_];j++) {
    int iRow=indexRowL[j];
    indexRowL[j] = fromSmallToBigRow_[iRow];
  }
#endif
  markRow[pivotRow] = LARGE_UNSET;
  //modify linked list for pivots
  deleteLink ( pivotRow );
  deleteLink ( pivotColumn + numberRows_ );
  totalElements_ += added;
  return 0;
}
// After pivoting
void
CoinAbcTypeFactorization::afterPivot( CoinSimplexInt pivotRow,
				      CoinSimplexInt pivotColumn )
{
#ifdef SMALL_PERMUTE
  int realPivotRow=fromSmallToBigRow_[pivotRow];
  int realPivotColumn=fromSmallToBigColumn_[pivotColumn];
  assert (permuteAddress_[realPivotRow]==numberGoodU_);
  pivotColumnAddress_[numberGoodU_] = realPivotColumn;
#else
  assert (permuteAddress_[pivotRow]==numberGoodU_);
  pivotColumnAddress_[numberGoodU_] = pivotColumn;
#endif
#ifdef ABC_USE_FUNCTION_POINTERS
  int number = numberInColumnPlusAddress_[pivotColumn];
  scatterStruct * COIN_RESTRICT scatter = scatterUColumn();
#ifdef SMALL_PERMUTE
#if ABC_USE_FUNCTION_POINTERS
  if (number<9) {
    scatter[realPivotRow].functionPointer=AbcScatterLowSubtract[number];
  } else {
    scatter[realPivotRow].functionPointer=AbcScatterHighSubtract[number&3];
  }
#endif
  scatter[realPivotRow].offset=lastEntryByColumnUPlus_;
  scatter[realPivotRow].number=number;
#else
#if ABC_USE_FUNCTION_POINTERS
  if (number<9) {
    scatter[pivotRow].functionPointer=AbcScatterLowSubtract[number];
  } else {
    scatter[pivotRow].functionPointer=AbcScatterHighSubtract[number&3];
  }
#endif
  scatter[pivotRow].offset=lastEntryByColumnUPlus_;
  scatter[pivotRow].number=number;
#endif
  // if scatter could move from inside pivot etc rather than indexRow
  CoinSimplexInt * COIN_RESTRICT indexRow = indexRowUAddress_;
  CoinFactorizationDouble * COIN_RESTRICT element = elementUAddress_;
  CoinBigIndex *  COIN_RESTRICT startColumnU = startColumnUAddress_;
  CoinBigIndex startC = startColumnU[pivotColumn]-number;
  CoinFactorizationDouble pivotMultiplier = pivotRegionAddress_[numberGoodU_];
  for (int i=startC;i<startC+number;i++)
    elementUColumnPlusAddress_[lastEntryByColumnUPlus_++]=element[i]*pivotMultiplier;
  CoinAbcMemcpy(reinterpret_cast<CoinSimplexInt *>(elementUColumnPlusAddress_+lastEntryByColumnUPlus_),indexRow+startC,number);
  lastEntryByColumnUPlus_ += (number+1)>>1;
#ifdef ABC_USE_FUNCTION_POINTERS
  if (!numberInColumnAddress_[pivotColumn]) {
    int iNext=nextColumnAddress_[pivotColumn];
    int iLast=lastColumnAddress_[pivotColumn];
    lastColumnAddress_[iNext]=iLast;
    assert (iLast>=0);
    nextColumnAddress_[iLast]=iNext;
  }
#endif
#endif
  numberGoodU_++;
  numberRowsLeft_--;
#if COIN_DEBUG==2
  checkConsistency (  );
#endif
}
#if ABC_DENSE_CODE
// After pivoting - returns true if need to go dense
int CoinAbcTypeFactorization::wantToGoDense()
{
  int status=0;
  if (denseThreshold_) {
    // see whether to go dense 
    // only if exact multiple
    if ((numberRowsLeft_&7)==0) {
      CoinSimplexDouble full = numberRowsLeft_;
      full *= full;
      assert (full>=0.0);
      CoinSimplexDouble leftElements = totalElements_;
      //if (numberRowsLeft_==100)
      //printf("at 100 %d elements\n",totalElements_);
      CoinSimplexDouble ratio;
      if (numberRowsLeft_>2000)
	ratio=4.5;
      else if (numberRowsLeft_>800)
	ratio=3.0;//3.5;
      else if (numberRowsLeft_>256)
	ratio=2.0;//2.5;
      //else if (numberRowsLeft_==8)
      //ratio=200.0;
      else
	ratio=1.5;
      //ratio *=0.75;
      if ((ratio*leftElements>full&&numberRowsLeft_>abs(denseThreshold_))) {
#if 0 //ndef NDEBUG
	//return to do dense
	CoinSimplexInt check=0;
	for (CoinSimplexInt iColumn=0;iColumn<numberRowsSmall_;iColumn++) {
	  if (numberInColumnAddress_[iColumn]) 
	    check++;
	}
	if (check!=numberRowsLeft_) {
	  printf("** mismatch %d columns left, %d rows\n",check,numberRowsLeft_);
	  //denseThreshold_=0;
	} else {
	  status=2;
	  if ((messageLevel_&4)!=0) 
	    std::cout<<"      Went dense at "<<numberRowsLeft_<<" rows "<<
	      totalElements_<<" "<<full<<" "<<leftElements<<std::endl;
	}
#else
	status=2;
#if 0 //ABC_NORMAL_DEBUG>1
	std::cout<<"      Went dense at "<<numberRowsLeft_<<" rows "<<
	  totalElements_<<" "<<full<<" "<<leftElements<<std::endl;
#endif
#endif
      }
    }
  }
  return status;
}
#endif
//  pivotRowSingleton.  Does one pivot on Row Singleton in factorization
bool
CoinAbcTypeFactorization::pivotRowSingleton ( CoinSimplexInt pivotRow,
				  CoinSimplexInt pivotColumn )
{
  //store pivot columns (so can easily compress)
  CoinBigIndex *  COIN_RESTRICT startColumnU = startColumnUAddress_;
  CoinBigIndex  COIN_RESTRICT startColumn = startColumnU[pivotColumn];
  CoinSimplexInt *  COIN_RESTRICT numberInRow = numberInRowAddress_;
  CoinSimplexInt *  COIN_RESTRICT numberInColumn = numberInColumnAddress_;
  CoinSimplexInt numberDoColumn = numberInColumn[pivotColumn] - 1;
  CoinBigIndex endColumn = startColumn + numberDoColumn + 1;
  CoinBigIndex pivotRowPosition = startColumn;
  CoinSimplexInt *  COIN_RESTRICT indexRowU = indexRowUAddress_;
  CoinSimplexInt iRow = indexRowU[pivotRowPosition];
  CoinBigIndex *  COIN_RESTRICT startRowU = startRowUAddress_;
  CoinSimplexInt *  COIN_RESTRICT nextRow = nextRowAddress_;
  CoinSimplexInt *  COIN_RESTRICT lastRow = lastRowAddress_;

  while ( iRow != pivotRow ) {
    pivotRowPosition++;
    iRow = indexRowU[pivotRowPosition];
  }				/* endwhile */
  assert ( pivotRowPosition < endColumn );
  //store column in L, compress in U and take column out
  CoinBigIndex l = lengthL_;

  if ( l + numberDoColumn > lengthAreaL_ ) {
    //need more memory
    if ((messageLevel_&4)!=0) 
      std::cout << "more memory needed in middle of invert" << std::endl;
    return false;
  }
  CoinBigIndex *  COIN_RESTRICT startColumnL = startColumnLAddress_;
  CoinFactorizationDouble *  COIN_RESTRICT elementL = elementLAddress_;
  CoinSimplexInt *  COIN_RESTRICT indexRowL = indexRowLAddress_;
  startColumnL[numberGoodL_] = l;	//for luck and first time
  numberGoodL_++;
  startColumnL[numberGoodL_] = l + numberDoColumn;
  lengthL_ += numberDoColumn;
  CoinFactorizationDouble * COIN_RESTRICT elementU = elementUAddress_;
  CoinFactorizationDouble pivotElement = elementU[pivotRowPosition];
  CoinFactorizationDouble pivotMultiplier = 1.0 / pivotElement;

  pivotRegionAddress_[numberGoodU_] = pivotMultiplier;
  CoinBigIndex i;

  CoinSimplexInt *  COIN_RESTRICT indexColumnU = indexColumnUAddress_;
#ifdef SMALL_PERMUTE
  int realPivotRow=fromSmallToBigRow_[pivotRow];
#endif
  for ( i = startColumn; i < pivotRowPosition; i++ ) {
    CoinSimplexInt iRow = indexRowU[i];

#ifdef SMALL_PERMUTE
    indexRowL[l] = fromSmallToBigRow_[iRow];
#else
    indexRowL[l] = iRow;
#endif
    elementL[l] = elementU[i] * pivotMultiplier;
    l++;
    //take out of row list
    CoinBigIndex start = startRowU[iRow];
    CoinSimplexInt iNumberInRow = numberInRow[iRow];
    CoinBigIndex end = start + iNumberInRow;
    CoinBigIndex where = start;

    while ( indexColumnU[where] != pivotColumn ) {
      where++;
    }				/* endwhile */
    assert ( where < end );
#if BOTH_WAYS
#endif
    indexColumnU[where] = indexColumnU[end - 1];
    iNumberInRow--;
    numberInRow[iRow] = iNumberInRow;
    modifyLink ( iRow, iNumberInRow );
  }
  for ( i = pivotRowPosition + 1; i < endColumn; i++ ) {
    CoinSimplexInt iRow = indexRowU[i];

#ifdef SMALL_PERMUTE
    indexRowL[l] = fromSmallToBigRow_[iRow];
#else
    indexRowL[l] = iRow;
#endif
    elementL[l] = elementU[i] * pivotMultiplier;
    l++;
    //take out of row list
    CoinBigIndex start = startRowU[iRow];
    CoinSimplexInt iNumberInRow = numberInRow[iRow];
    CoinBigIndex end = start + iNumberInRow;
    CoinBigIndex where = start;

    while ( indexColumnU[where] != pivotColumn ) {
      where++;
    }				/* endwhile */
    assert ( where < end );
#if BOTH_WAYS
#endif
    indexColumnU[where] = indexColumnU[end - 1];
    iNumberInRow--;
    numberInRow[iRow] = iNumberInRow;
    modifyLink ( iRow, iNumberInRow );
  }
  numberInColumn[pivotColumn] = 0;
  //modify linked list for pivots
  numberInRow[pivotRow] = 0;
  deleteLink ( pivotRow );
  deleteLink ( pivotColumn + numberRows_ );
  //take out this bit of indexColumnU
  CoinSimplexInt next = nextRow[pivotRow];
  CoinSimplexInt last = lastRow[pivotRow];

  nextRow[last] = next;
  lastRow[next] = last;
  lastRow[pivotRow] =-2;
#ifdef SMALL_PERMUTE
  permuteAddress_[realPivotRow]=numberGoodU_;
#else
  permuteAddress_[pivotRow]=numberGoodU_;
#endif
  return true;
}

//  pivotColumnSingleton.  Does one pivot on Column Singleton in factorization
void
CoinAbcTypeFactorization::pivotColumnSingleton ( CoinSimplexInt pivotRow,
				     CoinSimplexInt pivotColumn )
{
  CoinSimplexInt *  COIN_RESTRICT numberInRow = numberInRowAddress_;
  CoinSimplexInt *  COIN_RESTRICT numberInColumn = numberInColumnAddress_;
  CoinSimplexInt *  COIN_RESTRICT numberInColumnPlus = numberInColumnPlusAddress_;
  //store pivot columns (so can easily compress)
  CoinSimplexInt numberDoRow = numberInRow[pivotRow] - 1;
  CoinBigIndex *  COIN_RESTRICT startColumnU = startColumnUAddress_;
  CoinBigIndex startColumn = startColumnU[pivotColumn];
  CoinBigIndex *  COIN_RESTRICT startRowU = startRowUAddress_;
  CoinBigIndex startRow = startRowU[pivotRow];
  CoinBigIndex endRow = startRow + numberDoRow + 1;
  CoinSimplexInt *  COIN_RESTRICT nextRow = nextRowAddress_;
  CoinSimplexInt *  COIN_RESTRICT lastRow = lastRowAddress_;
  //take out this bit of indexColumnU
  CoinSimplexInt next = nextRow[pivotRow];
  CoinSimplexInt last = lastRow[pivotRow];

  nextRow[last] = next;
  lastRow[next] = last;
  lastRow[pivotRow] =-2; //mark
#ifdef SMALL_PERMUTE
  int realPivotRow=fromSmallToBigRow_[pivotRow];
  permuteAddress_[realPivotRow]=numberGoodU_;
#else
  permuteAddress_[pivotRow]=numberGoodU_;
#endif
  //clean up counts
  CoinFactorizationDouble * COIN_RESTRICT elementU = elementUAddress_;
  CoinFactorizationDouble pivotElement = elementU[startColumn];

  pivotRegionAddress_[numberGoodU_] = 1.0 / pivotElement;
  numberInColumn[pivotColumn] = 0;
  CoinSimplexInt *  COIN_RESTRICT indexColumnU = indexColumnUAddress_;
  CoinSimplexInt *  COIN_RESTRICT indexRowU = indexRowUAddress_;
  //CoinSimplexInt *  COIN_RESTRICT saveColumn = saveColumnAddress_;
  CoinBigIndex i;
  // would I be better off doing other way first??
  for ( i = startRow; i < endRow; i++ ) {
    CoinSimplexInt iColumn = indexColumnU[i];
    if ( numberInColumn[iColumn] ) {
      CoinSimplexInt number = numberInColumn[iColumn] - 1;
      //modify linked list
      modifyLink ( iColumn + numberRows_, number );
      //move pivot row element
#ifdef SMALL_PERMUTE
      CoinBigIndex start = startColumnU[iColumn];
#endif
      if ( number ) {
#ifndef SMALL_PERMUTE
	CoinBigIndex start = startColumnU[iColumn];
#endif
	CoinBigIndex pivot = start;
	CoinSimplexInt iRow = indexRowU[pivot];
	while ( iRow != pivotRow ) {
	  pivot++;
	  iRow = indexRowU[pivot];
	}
        assert (pivot < startColumnU[iColumn] +
                numberInColumn[iColumn]);
#if BOTH_WAYS
#endif
	if ( pivot != start ) {
	  //move largest one up
	  CoinFactorizationDouble value = elementU[start];

	  iRow = indexRowU[start];
	  elementU[start] = elementU[pivot];
	  assert (indexRowU[pivot]==pivotRow);
	  indexRowU[start] = indexRowU[pivot];
	  elementU[pivot] = elementU[start + 1];
	  indexRowU[pivot] = indexRowU[start + 1];
	  elementU[start + 1] = value;
	  indexRowU[start + 1] = iRow;
#if BOTH_WAYS
#endif
	} else {
	  //find new largest element
	  CoinSimplexInt iRowSave = indexRowU[start + 1];
	  CoinFactorizationDouble valueSave = elementU[start + 1];
	  CoinFactorizationDouble valueLargest = fabs ( valueSave );
	  CoinBigIndex end = start + numberInColumn[iColumn];
	  CoinBigIndex largest = start + 1;

	  CoinBigIndex k;
	  for ( k = start + 2; k < end; k++ ) {
	    CoinFactorizationDouble value = elementU[k];
	    CoinFactorizationDouble valueAbs = fabs ( value );

	    if ( valueAbs > valueLargest ) {
	      valueLargest = valueAbs;
	      largest = k;
	    }
	  }
	  indexRowU[start + 1] = indexRowU[largest];
	  elementU[start + 1] = elementU[largest];
	  indexRowU[largest] = iRowSave;
	  elementU[largest] = valueSave;
#if BOTH_WAYS
#endif
	}
      }
#ifdef SMALL_PERMUTE
      indexRowU[start]=realPivotRow;
#endif
      //clean up counts
      numberInColumn[iColumn]--;
      numberInColumnPlus[iColumn]++;
      startColumnU[iColumn]++;
      //totalElements_--;
    }
  }
  //modify linked list for pivots
  deleteLink ( pivotRow );
  deleteLink ( pivotColumn + numberRows_ );
  numberInRow[pivotRow] = 0;
  //put in dummy pivot in L
  CoinBigIndex l = lengthL_;

  CoinBigIndex * startColumnL = startColumnLAddress_;
  startColumnL[numberGoodL_] = l;	//for luck and first time
  numberGoodL_++;
  startColumnL[numberGoodL_] = l;
}


//  getColumnSpace.  Gets space for one Column with given length
//may have to do compression  (returns true)
//also moves existing vector
bool
CoinAbcTypeFactorization::getColumnSpace ( CoinSimplexInt iColumn,
			       CoinSimplexInt extraNeeded )
{
  CoinSimplexInt *  COIN_RESTRICT numberInColumn = numberInColumnAddress_;
  CoinSimplexInt *  COIN_RESTRICT numberInColumnPlus = numberInColumnPlusAddress_;
  CoinSimplexInt *  COIN_RESTRICT nextColumn = nextColumnAddress_;
  CoinSimplexInt *  COIN_RESTRICT lastColumn = lastColumnAddress_;
  CoinSimplexInt number = numberInColumnPlus[iColumn] +
    numberInColumn[iColumn];
  CoinBigIndex *  COIN_RESTRICT startColumnU = startColumnUAddress_;
  CoinBigIndex space = lengthAreaU_ - lastEntryByColumnU_;
  CoinFactorizationDouble * COIN_RESTRICT elementU = elementUAddress_;
  CoinSimplexInt *  COIN_RESTRICT indexRowU = indexRowUAddress_;

  if ( space < extraNeeded + number + 4 ) {
    //compression
    CoinSimplexInt iColumn = nextColumn[maximumRowsExtra_];
    CoinBigIndex put = 0;

    while ( iColumn != maximumRowsExtra_ ) {
      //move
      CoinBigIndex get;
      CoinBigIndex getEnd;

      if ( startColumnU[iColumn] >= 0 ) {
	get = startColumnU[iColumn]
	  - numberInColumnPlus[iColumn];
	getEnd = startColumnU[iColumn] + numberInColumn[iColumn];
	startColumnU[iColumn] = put + numberInColumnPlus[iColumn];
      } else {
	get = -startColumnU[iColumn];
	getEnd = get + numberInColumn[iColumn];
	startColumnU[iColumn] = -put;
      }
      assert (put<=get);
      for (CoinBigIndex i = get; i < getEnd; i++ ) {
	indexRowU[put] = indexRowU[i];
	elementU[put] = elementU[i];
#if BOTH_WAYS
#endif
	put++;
      }
      iColumn = nextColumn[iColumn];
    }				/* endwhile */
    numberCompressions_++;
    lastEntryByColumnU_ = put;
    space = lengthAreaU_ - put;
    if ( extraNeeded == COIN_INT_MAX >> 1 ) {
      return true;
    }
    if ( space < extraNeeded + number + 2 ) {
      //need more space
      //if we can allocate bigger then do so and copy
      //if not then return so code can start again
      status_ = -99;
      return false;
    }
  }
  CoinBigIndex put = lastEntryByColumnU_;
  CoinSimplexInt next = nextColumn[iColumn];
  CoinSimplexInt last = lastColumn[iColumn];

  if ( extraNeeded || next != maximumRowsExtra_ ) {
    //out
    nextColumn[last] = next;
    lastColumn[next] = last;
    //in at end
    last = lastColumn[maximumRowsExtra_];
    nextColumn[last] = iColumn;
    lastColumn[maximumRowsExtra_] = iColumn;
    lastColumn[iColumn] = last;
    nextColumn[iColumn] = maximumRowsExtra_;
    //move
    CoinBigIndex get = startColumnU[iColumn]
      - numberInColumnPlus[iColumn];

    startColumnU[iColumn] = put + numberInColumnPlus[iColumn];
    CoinSimplexInt *indexRow = indexRowU;
    CoinFactorizationDouble *element = elementU;
    CoinSimplexInt i = 0;
    
    if ( ( number & 1 ) != 0 ) {
      element[put] = element[get];
      indexRow[put] = indexRow[get];
#if BOTH_WAYS
#endif
      i = 1;
    }
    for ( ; i < number; i += 2 ) {
      CoinFactorizationDouble value0 = element[get + i];
      CoinFactorizationDouble value1 = element[get + i + 1];
      CoinSimplexInt index0 = indexRow[get + i];
      CoinSimplexInt index1 = indexRow[get + i + 1];
      
      element[put + i] = value0;
      element[put + i + 1] = value1;
      indexRow[put + i] = index0;
      indexRow[put + i + 1] = index1;
#if BOTH_WAYS
#endif
    }
    put += number;
    get += number;
    //add 2 for luck
    lastEntryByColumnU_ = put + extraNeeded + 2;
    if (lastEntryByColumnU_>lengthAreaU_) {
      // get more memory
#ifdef CLP_DEVELOP
      printf("put %d, needed %d, start %d, length %d\n",
	     put,extraNeeded,lastEntryByColumnU_,
	     lengthAreaU_);
#endif
      return false;
    }
  } else {
    //take off space
    lastEntryByColumnU_ = startColumnU[last] + numberInColumn[last];
  }
  startColumnU[maximumRowsExtra_]=lastEntryByColumnU_;
  return true;
}

//  getRowSpace.  Gets space for one Row with given length
//may have to do compression  (returns true)
//also moves existing vector
bool
CoinAbcTypeFactorization::getRowSpace ( CoinSimplexInt iRow,
			    CoinSimplexInt extraNeeded )
{
  CoinSimplexInt *  COIN_RESTRICT numberInRow = numberInRowAddress_;
  CoinSimplexInt number = numberInRow[iRow];
  CoinBigIndex *  COIN_RESTRICT startRowU = startRowUAddress_;
  CoinBigIndex space = lengthAreaU_ - lastEntryByRowU_;
  CoinSimplexInt *  COIN_RESTRICT nextRow = nextRowAddress_;
  CoinSimplexInt *  COIN_RESTRICT lastRow = lastRowAddress_;
  CoinSimplexInt *  COIN_RESTRICT indexColumnU = indexColumnUAddress_;

  if ( space < extraNeeded + number + 2 ) {
    //compression
    CoinSimplexInt iRow = nextRow[numberRows_];
    CoinBigIndex put = 0;

    while ( iRow != numberRows_ ) {
      //move
      CoinBigIndex get = startRowU[iRow];
      CoinBigIndex getEnd = startRowU[iRow] + numberInRow[iRow];

      startRowU[iRow] = put;
      if (put>get) {
	//need more space
	//if we can allocate bigger then do so and copy
	//if not then return so code can start again
	status_ = -99;
	return false;;
      }
      CoinBigIndex i;
      for ( i = get; i < getEnd; i++ ) {
	indexColumnU[put] = indexColumnU[i];
#if BOTH_WAYS
#endif
	put++;
      }
      iRow = nextRow[iRow];
    }				/* endwhile */
    numberCompressions_++;
    lastEntryByRowU_ = put;
    space = lengthAreaU_ - put;
    if ( space < extraNeeded + number + 2 ) {
      //need more space
      //if we can allocate bigger then do so and copy
      //if not then return so code can start again
      status_ = -99;
      return false;;
    }
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
  CoinBigIndex get = startRowU[iRow];

  startRowU[iRow] = put;
  while ( number ) {
    number--;
    indexColumnU[put] = indexColumnU[get];
#if BOTH_WAYS
#endif
    put++;
    get++;
  }				/* endwhile */
  //add 4 for luck
  lastEntryByRowU_ = put + extraNeeded + 4;
  return true;
}

//  cleanup.  End of factorization
void
CoinAbcTypeFactorization::cleanup (  )
{
#ifdef ABC_USE_FUNCTION_POINTERS
#define PRINT_LEVEL 0
  if (PRINT_LEVEL){
    printf("----------Start---------\n");
#if PRINT_LEVEL<2
    double smallestP=COIN_DBL_MAX;
    int iPs=-1;
    double largestP=0.0;
    int iP=-1;
    double largestL=0.0;
    int iL=-1;
    double largestU=0.0;
    int iU=-1;
#endif
    for (int i=0;i<numberRows_;i++) {
#if PRINT_LEVEL>1
      printf("%d permute %d pivotColumn %d pivotRegion %g\n",i,permuteAddress_[i],
	     pivotColumnAddress_[i],pivotRegionAddress_[i]);
#else
      if (fabs(pivotRegionAddress_[i])>largestP) {
	iP=i;
	largestP=fabs(pivotRegionAddress_[i]);
      }
      if (fabs(pivotRegionAddress_[i])<smallestP) {
	iPs=i;
	smallestP=fabs(pivotRegionAddress_[i]);
      }
#endif
    }
    scatterStruct * COIN_RESTRICT scatterPointer = scatterUColumn();
    CoinFactorizationDouble *  COIN_RESTRICT elementUColumnPlus = elementUColumnPlusAddress_;
    for (int i=0;i<numberRows_;i++) {
      scatterStruct & COIN_RESTRICT scatter = scatterPointer[i];
      CoinSimplexInt number = scatter.number;
#if PRINT_LEVEL>1
      printf("pivot %d offset %d number %d\n",i,scatter.offset,number);
#endif
      const CoinFactorizationDouble * COIN_RESTRICT area = elementUColumnPlus+scatter.offset;
      const CoinBigIndex * COIN_RESTRICT indices = reinterpret_cast<const CoinBigIndex *>(area+number);
#if PRINT_LEVEL>1
      for (int j1 = 0; j1 < number; j1+=5 ) {
	for (int j = j1; j < CoinMin(number,j1+5); j++ ) 
	  printf("(%d,%g) ",indices[j],area[j]);
	printf("\n");
      }
#else
      for (int j = 0; j < number; j++ ) 
	if (fabs(area[j])>largestU) {
	  iU=i;
	  largestU=fabs(area[j]);
	}
#endif
    }
    printf("----------LLL---------\n");
    CoinBigIndex *  COIN_RESTRICT startColumnL = startColumnLAddress_;
    CoinSimplexInt *  COIN_RESTRICT indexRowL = indexRowLAddress_;
    CoinFactorizationDouble *  COIN_RESTRICT elementL = elementLAddress_;
    for (int i=0;i<numberRows_;i++) {
      int number=startColumnL[i+1]-startColumnL[i];
      if (number) {
#if PRINT_LEVEL>1
	int * indices = indexRowL+startColumnL[i];
	printf("%d has %d elements\n",i,number);
	for (int j1 = 0; j1 < number; j1+=5 ) {
	  for (int j = j1; j < CoinMin(number,j1+5); j++ ) 
	    printf("(%d,%g) ",indices[j],elementL[j+startColumnL[i]]);
	  printf("\n");
	}
#else
	for (int j = 0; j < number; j++ ) {
	  if (fabs(elementL[j+startColumnL[i]])>largestL) {
	    iL=i;
	    largestL=fabs(elementL[j+startColumnL[i]]);
	  }
	}
#endif
      }
    }
#if PRINT_LEVEL<2
    printf("smallest pivot %g at %d, largest %g at %d - largestL %g at %d largestU %g at %d\n",smallestP,iPs,largestP,iP,largestL,iL,largestU,iU);
#endif
    printf("----------End---------\n");
  }
#endif
#ifdef ABC_ORDERED_FACTORIZATION
  {
    //int sizeL=0;
    //int sizeU=0;
    CoinSimplexInt *  COIN_RESTRICT permuteA = permuteAddress_+maximumRowsExtra_+1;
    CoinSimplexInt *  COIN_RESTRICT permuteB = permuteA + numberRows_;
    // one may be correct
    for (int i=0;i<numberRows_;i++) {
      int iPermute=permuteAddress_[i];
      permuteA[i]=iPermute;
      permuteB[iPermute]=i;
      permuteAddress_[i]=i;
    }
    scatterStruct * COIN_RESTRICT scatterPointer = scatterUColumn();
    CoinFactorizationDouble *  COIN_RESTRICT elementUColumnPlus = elementUColumnPlusAddress_;
    for (int i=0;i<numberRows_;i++) {
      scatterStruct & COIN_RESTRICT scatter = scatterPointer[i];
      CoinSimplexInt number = scatter.number;
      //sizeU+=number;
      CoinFactorizationDouble * COIN_RESTRICT area = elementUColumnPlus+scatter.offset;
      CoinBigIndex * COIN_RESTRICT indices = reinterpret_cast<CoinBigIndex *>(area+number);
      for (int j = 0; j < number; j++ ) {
	int iRow=indices[j];
	indices[j]=permuteA[iRow];
      }
    }
    CoinBigIndex *  COIN_RESTRICT startColumnL = startColumnLAddress_;
    CoinSimplexInt *  COIN_RESTRICT indexRowL = indexRowLAddress_;
    CoinFactorizationDouble *  COIN_RESTRICT elementL = elementLAddress_;
    for (int i=0;i<numberRows_;i++) {
      int number=startColumnL[i+1]-startColumnL[i];
      if (number) {
	//sizeL+=number;
	int * indices = indexRowL+startColumnL[i];
	for (int j = 0; j < number; j++ ) {
	  int iRow=indices[j];
	  indices[j]=permuteA[iRow];
	}
      }
    }
    //printf("Ordered says %d L %d U\n",sizeL,sizeU);
  }
#elif 0
#ifdef ABC_USE_FUNCTION_POINTERS
  {
    int sizeL=0;
    int sizeU=0;
    scatterStruct * COIN_RESTRICT scatterPointer = scatterUColumn();
    for (int i=0;i<numberRows_;i++) {
      scatterStruct & COIN_RESTRICT scatter = scatterPointer[i];
      CoinSimplexInt number = scatter.number;
      sizeU+=number;
    }
    CoinBigIndex *  COIN_RESTRICT startColumnL = startColumnLAddress_;
    for (int i=0;i<numberRows_;i++) {
      int number=startColumnL[i+1]-startColumnL[i];
      if (number) 
	sizeL+=number;
    }
    printf("Unordered says %d L %d U\n",sizeL,sizeU);
  }
#endif
#endif
  //free some memory here
  markRow_.conditionalDelete() ;
  nextRowAddress_ = nextRow_.array();
  //safety feature
  CoinSimplexInt *  COIN_RESTRICT permute = permuteAddress_;
  permute[numberRows_] = 0;
#ifndef NDEBUG
  {
    if (numberGoodU_<numberRows_)
      abort();
    char * mark = new char[numberRows_];
    memset(mark,0,numberRows_);
    for (int i = 0; i < numberRows_; i++ ) {
      assert (permute[i]>=0&&permute[i]<numberRows_);
      CoinSimplexInt k = permute[i];
      if(k<0||k>=numberRows_)
	printf("Bad a %d %d\n",i,k);
      assert(k>=0&&k<numberRows_);
      if(mark[k]==1)
	printf("Bad a %d %d\n",i,k);
      mark[k]=1;
    }
    for (int i = 0; i < numberRows_; i++ ) {
      assert(mark[i]==1);
      if(mark[i]!=1)
	printf("Bad b %d\n",i);
    }
    delete [] mark;
  }
#endif
  CoinSimplexInt *  COIN_RESTRICT numberInColumnPlus = numberInColumnPlusAddress_;
  const CoinSimplexInt *  COIN_RESTRICT pivotColumn = pivotColumnAddress_;
  CoinSimplexInt *  COIN_RESTRICT pivotLOrder = firstCountAddress_;//tttt;//this->pivotLOrder();
  CoinBigIndex *  COIN_RESTRICT startColumnU = startColumnUAddress_;
  bool cleanCopy=false;
#if ABC_SMALL<2
  assert (numberGoodU_==numberRows_);
  if (numberGoodU_==numberRows_) {
#ifndef ABC_USE_FUNCTION_POINTERS
    const CoinSimplexInt * COIN_RESTRICT numberInColumn = numberInColumnPlusAddress_;
    CoinFactorizationDouble * elementU = elementUAddress_;
    CoinFactorizationDouble * elementU2 = elementRowUAddress_;
    CoinSimplexInt * COIN_RESTRICT indexRowU = indexRowUAddress_;  
    CoinSimplexInt * COIN_RESTRICT indexRowU2 = indexColumnUAddress_;  
    // so that just slacks will have start of 0
    CoinBigIndex size=1;
    for (int i=0;i<numberSlacks_;i++) {
      CoinSimplexInt iNext=pivotColumn[i];
      startColumnU[iNext]=0;
      assert (!numberInColumn[iNext]);
    }
    for (int i=numberSlacks_;i<numberRows_;i++) {
      CoinSimplexInt iNext=pivotColumn[i];
      CoinSimplexInt number = numberInColumn[iNext];
      CoinBigIndex start = startColumnU[iNext]-number;
      startColumnU[iNext]=size;
      CoinAbcMemcpy(elementU2+size,elementU+start,number);
      CoinAbcMemcpy(indexRowU2+size,indexRowU+start,number);
      size+=number;
    }
    // swap arrays
    elementU_.swap(elementRowU_);
    elementUAddress_ = elementU_.array();
    elementRowUAddress_ = elementRowU_.array();
    indexColumnU_.swap(indexRowU_);
    indexRowUAddress_ = indexRowU_.array();
    indexColumnUAddress_ = indexColumnU_.array();
    // Redo total elements
    totalElements_=size;
#else
    // swap arrays
    elementU_.swap(elementRowU_);
    elementUAddress_ = elementU_.array();
    elementRowUAddress_ = elementRowU_.array();
    totalElements_=lastEntryByColumnUPlus_;
#endif
    cleanCopy=true;
  }
#endif
#ifndef ABC_USE_FUNCTION_POINTERS
  if (!cleanCopy) {
    getColumnSpace ( 0, COIN_INT_MAX >> 1 );	//compress
    // Redo total elements
    totalElements_=0;
    for (int i = 0; i < numberRows_; i++ ) {
      CoinSimplexInt number = numberInColumnPlus[i];
      totalElements_ += number;
      startColumnU[i] -= number;
    }
  }
#endif
  // swap arrays
  numberInColumn_.swap(numberInColumnPlus_);
  numberInColumnAddress_ = numberInColumn_.array();
  numberInColumnPlusAddress_ = numberInColumnPlus_.array();
  CoinSimplexInt *  COIN_RESTRICT numberInRow = numberInRowAddress_;
  CoinSimplexInt *  COIN_RESTRICT numberInColumn = numberInColumnAddress_;
#if ABC_SMALL<2
  numberInColumnPlus = numberInColumnPlusAddress_;
#endif
  //make column starts OK
  //for best cache behavior get in order (last pivot at bottom of space)
  //that will need thinking about

  lastEntryByRowU_ = totalElements_;

  CoinSimplexInt *  COIN_RESTRICT back = firstCountAddress_+numberRows_;
  CoinBigIndex * COIN_RESTRICT startRow = startRowUAddress_;
#ifndef ABC_USE_FUNCTION_POINTERS
  maximumU_=startColumnU[numberRows_-1]+numberInColumn[numberRows_-1];
#else
  maximumU_=lastEntryByColumnUPlus_;
#endif
#ifndef ABC_ORDERED_FACTORIZATION
  CoinFactorizationDouble *  COIN_RESTRICT workArea = workAreaAddress_;
  CoinFactorizationDouble *  COIN_RESTRICT pivotRegion = pivotRegionAddress_;
#else
  CoinSimplexInt *  COIN_RESTRICT permuteA = permuteAddress_+maximumRowsExtra_+1;
  // use work area for scatter
  scatterStruct * COIN_RESTRICT scatterPointer = scatterUColumn();
  scatterStruct * COIN_RESTRICT scatterPointerTemp = reinterpret_cast<scatterStruct *>(workAreaAddress_);
#endif
  for (CoinSimplexInt i=0;i<numberRows_;i++) {
#ifndef ABC_ORDERED_FACTORIZATION
    CoinSimplexInt pivotOrder=permute[i];
    CoinSimplexInt columnOrder=pivotColumn[pivotOrder];
    workArea[i]=pivotRegion[pivotOrder];
    back[columnOrder] = i;
#else
    CoinSimplexInt pivotOrder=i;
    CoinSimplexInt columnOrder=pivotColumn[pivotOrder];
    int pivotOrder2=permuteA[i];
    scatterPointerTemp[pivotOrder2]=scatterPointer[i];
    back[columnOrder] = i;
#endif
#ifndef ABC_USE_FUNCTION_POINTERS
    startRow[i]=startColumnU[columnOrder];
    numberInRow[i]=numberInColumn[columnOrder];
#endif
    pivotLOrder[pivotOrder]=i;
  }
#ifndef ABC_ORDERED_FACTORIZATION
  CoinAbcMemcpy(pivotRegion,workArea,numberRows_); // could swap
#else
  CoinAbcMemcpy(scatterPointer,scatterPointerTemp,numberRows_);
#endif
#ifndef EARLY_FACTORIZE
  workArea_.conditionalDelete() ;
  workAreaAddress_=NULL;
#endif
#ifndef ABC_USE_FUNCTION_POINTERS
  //memcpy(this->pivotLOrder(),pivotLOrder,numberRows_*sizeof(int));
  // swap later
  CoinAbcMemcpy(startColumnU,startRow,numberRows_);
  CoinAbcMemcpy(numberInColumn,numberInRow,numberRows_);
#endif
#if ABC_DENSE_CODE==2
  if (numberDense_) {
    assert (numberDense_<30000);
    CoinFactorizationDouble *  COIN_RESTRICT denseArea = denseAreaAddress_;
    CoinFactorizationDouble * COIN_RESTRICT denseRegion = 
      denseArea+leadingDimension_*numberDense_;
    CoinSimplexInt *  COIN_RESTRICT densePermute=
      reinterpret_cast<CoinSimplexInt *>(denseRegion+FACTOR_CPU*numberDense_);
    short * COIN_RESTRICT forFtran = reinterpret_cast<short *>(densePermute+numberDense_);
    short * COIN_RESTRICT forBtran = forFtran+numberDense_;
    short * COIN_RESTRICT forFtran2 = forBtran+numberDense_;
    // maybe could do inside dgetrf
    //CoinAbcMemcpy(forBtran,pivotLOrder+numberRows_-numberDense_,numberDense_);
    //CoinAbcMemcpy(forFtran,pivotLOrder+numberRows_-numberDense_,numberDense_);
    for (int i=0;i<numberDense_;i++) {
      forBtran[i]=static_cast<short>(i);
      forFtran[i]=static_cast<short>(i);
    }
    for (int i=0;i<numberDense_;i++) {
      int ip=densePermute[i];
      short temp=forBtran[i];
      forBtran[i]=forBtran[ip];
      forBtran[ip]=temp;
    }
    for (int i=numberDense_-1;i>=0;i--) {
      int ip=densePermute[i];
      short temp=forFtran[i];
      forFtran[i]=forFtran[ip];
      forFtran[ip]=temp;
    }
#if ABC_SMALL<3
    const CoinSimplexInt * COIN_RESTRICT pivotLBackwardOrder = permuteAddress_;
    int lastSparse=numberRows_-numberDense_;
    forFtran -= lastSparse; // adjust
    CoinFillN(forFtran2,numberRows_,static_cast<short>(-1));
    for (int i=0;i<numberRows_;i++) {
      int jRow=pivotLBackwardOrder[i];
      if (jRow>=lastSparse) {
	short kRow = static_cast<short>(forFtran[jRow]);
	forFtran2[i]=kRow;
      }
    }
#endif 
  }
#endif
  CoinAbcMemset0(numberInRow,numberRows_+1);
  if ( (messageLevel_ & 8)) {
    std::cout<<"        length of U "<<totalElements_<<", length of L "<<lengthL_;
#if ABC_SMALL<4
    if (numberDense_)
      std::cout<<" plus "<<numberDense_*numberDense_<<" from "<<numberDense_<<" dense rows";
#endif
    std::cout<<std::endl;
  }
#if ABC_SMALL<4
  // and add L and dense
  totalElements_ += numberDense_*numberDense_+lengthL_;
#endif
#if ABC_SMALL<2
  CoinSimplexInt *  COIN_RESTRICT nextColumn = nextColumnAddress_;
  CoinSimplexInt *  COIN_RESTRICT lastColumn = lastColumnAddress_;
  //goSparse2();
  // See whether to have extra copy of R
#if ABC_SMALL>=0
  if (maximumU_>10*numberRows_||numberRows_<200) {
    // NO
    numberInColumnPlus_.conditionalDelete() ;
    numberInColumnPlusAddress_=NULL;
    setNoGotRCopy();
  } else {
#endif
    setYesGotRCopy();
    for (int i = 0; i < numberRows_; i++ ) {
      lastColumn[i] = i - 1;
      nextColumn[i] = i + 1;
      numberInColumnPlus[i]=0;
#if ABC_SMALL>=0
    }
#endif
    nextColumn[numberRows_ - 1] = maximumRowsExtra_;
    lastColumn[maximumRowsExtra_] = numberRows_ - 1;
    nextColumn[maximumRowsExtra_] = 0;
    lastColumn[0] = maximumRowsExtra_;
  }
#endif
  numberU_ = numberGoodU_;
  numberL_ = numberGoodL_;

  CoinSimplexInt firstReal = numberSlacks_;

  CoinBigIndex *  COIN_RESTRICT startColumnL = startColumnLAddress_;
  // We should know how many in L
  //CoinSimplexInt *  COIN_RESTRICT indexRowL = indexRowLAddress_;
  for (firstReal = numberSlacks_; firstReal<numberRows_; firstReal++ ) {
    if (startColumnL[firstReal+1])
      break;
  }
  totalElements_ += startColumnL[numberRows_];
  baseL_ = firstReal;
  numberL_ -= firstReal;
  factorElements_ = totalElements_;
  //use L for R if room
  CoinBigIndex space = lengthAreaL_ - lengthL_;
  CoinBigIndex spaceUsed = lengthL_ + lengthU_;

  CoinSimplexInt needed = ( spaceUsed + numberRows_ - 1 ) / numberRows_;

  needed = needed * 2 * maximumPivots_;
  if ( needed < 2 * numberRows_ ) {
    needed = 2 * numberRows_;
  }
  if (gotRCopy()) {
    //if (numberInColumnPlusAddress_) {
    // Need double the space for R
    space = space/2;
  }
  elementRAddress_ = elementLAddress_ + lengthL_;
  indexRowRAddress_ = indexRowLAddress_ + lengthL_;
  if ( space >= needed ) {
    lengthR_ = 0;
    lengthAreaR_ = space;
  } else {
    lengthR_ = 0;
    lengthAreaR_ = space;
    if ((messageLevel_&4))
      std::cout<<"Factorization may need some increasing area space"
	       <<std::endl;
    if ( areaFactor_ ) {
      areaFactor_ *= 1.1;
    } else {
      areaFactor_ = 1.1;
    }
  }
  numberR_ = 0;
}
// Returns areaFactor but adjusted for dense
double 
CoinAbcTypeFactorization::adjustedAreaFactor() const
{
  double factor = areaFactor_;
#if ABC_SMALL<4
  if (numberDense_&&areaFactor_>1.0) {
    double dense = numberDense_;
    dense *= dense;
    double withoutDense = totalElements_ - dense +1.0;
    factor *= 1.0 +dense/withoutDense;
  }
#endif
  return factor;
}

//  checkConsistency.  Checks that row and column copies look OK
void
CoinAbcTypeFactorization::checkConsistency (  )
{
  bool bad = false;

  CoinSimplexInt iRow;
  CoinBigIndex * startRowU = startRowUAddress_;
  CoinSimplexInt * numberInRow = numberInRowAddress_;
  CoinSimplexInt * numberInColumn = numberInColumnAddress_;
  CoinSimplexInt * indexColumnU = indexColumnUAddress_;
  CoinSimplexInt * indexRowU = indexRowUAddress_;
  CoinBigIndex * startColumnU = startColumnUAddress_;
  for ( iRow = 0; iRow < numberRows_; iRow++ ) {
    if ( numberInRow[iRow] ) {
      CoinBigIndex startRow = startRowU[iRow];
      CoinBigIndex endRow = startRow + numberInRow[iRow];

      CoinBigIndex j;
      for ( j = startRow; j < endRow; j++ ) {
	CoinSimplexInt iColumn = indexColumnU[j];
	CoinBigIndex startColumn = startColumnU[iColumn];
	CoinBigIndex endColumn = startColumn + numberInColumn[iColumn];
	bool found = false;

	CoinBigIndex k;
	for ( k = startColumn; k < endColumn; k++ ) {
	  if ( indexRowU[k] == iRow ) {
	    found = true;
	    break;
	  }
	}
	if ( !found ) {
	  bad = true;
	  std::cout << "row " << iRow << " column " << iColumn << " Rows" << std::endl;
	}
      }
    }
  }
  CoinSimplexInt iColumn;
  for ( iColumn = 0; iColumn < numberRows_; iColumn++ ) {
    if ( numberInColumn[iColumn] ) {
      CoinBigIndex startColumn = startColumnU[iColumn];
      CoinBigIndex endColumn = startColumn + numberInColumn[iColumn];

      CoinBigIndex j;
      for ( j = startColumn; j < endColumn; j++ ) {
	CoinSimplexInt iRow = indexRowU[j];
	CoinBigIndex startRow = startRowU[iRow];
	CoinBigIndex endRow = startRow + numberInRow[iRow];
	bool found = false;

	CoinBigIndex k;
	for (  k = startRow; k < endRow; k++ ) {
	  if ( indexColumnU[k] == iColumn ) {
	    found = true;
	    break;
	  }
	}
	if ( !found ) {
	  bad = true;
	  std::cout << "row " << iRow << " column " << iColumn << " Columns" <<
	    std::endl;
	}
      }
    }
  }
  if ( bad ) {
    abort (  );
  }
}
  //  pivotOneOtherRow.  When just one other row so faster
bool 
CoinAbcTypeFactorization::pivotOneOtherRow ( CoinSimplexInt pivotRow,
					   CoinSimplexInt pivotColumn )
{
  CoinSimplexInt *  COIN_RESTRICT numberInRow = numberInRowAddress_;
  CoinSimplexInt *  COIN_RESTRICT numberInColumn = numberInColumnAddress_;
  CoinSimplexInt *  COIN_RESTRICT numberInColumnPlus = numberInColumnPlusAddress_;
  CoinSimplexInt  COIN_RESTRICT numberInPivotRow = numberInRow[pivotRow] - 1;
  CoinBigIndex *  COIN_RESTRICT startRowU = startRowUAddress_;
  CoinBigIndex *  COIN_RESTRICT startColumnU = startColumnUAddress_;
  CoinBigIndex startColumn = startColumnU[pivotColumn];
  CoinBigIndex startRow = startRowU[pivotRow];
  CoinBigIndex endRow = startRow + numberInPivotRow + 1;

  //take out this bit of indexColumnU
  CoinSimplexInt *  COIN_RESTRICT nextRow = nextRowAddress_;
  CoinSimplexInt *  COIN_RESTRICT lastRow = lastRowAddress_;
  CoinSimplexInt next = nextRow[pivotRow];
  CoinSimplexInt last = lastRow[pivotRow];

  nextRow[last] = next;
  lastRow[next] = last;
  lastRow[pivotRow] = -2;
#ifdef SMALL_PERMUTE
  int realPivotRow=fromSmallToBigRow_[pivotRow];
  //int realPivotColumn=fromSmallToBigColumn[pivotColumn];
#endif
#ifdef SMALL_PERMUTE
  permuteAddress_[realPivotRow]=numberGoodU_;
#else
  permuteAddress_[pivotRow]=numberGoodU_;
#endif
  numberInRow[pivotRow] = 0;
  #if ABC_SMALL<2
// temp - switch off marker for valid row/column lookup
  sparseThreshold_=-2;
#endif
  //store column in L, compress in U and take column out
  CoinBigIndex l = lengthL_;

  if ( l + 1 > lengthAreaL_ ) {
    //need more memory
    if ((messageLevel_&4)!=0) 
      std::cout << "more memory needed in middle of invert" << std::endl;
    return false;
  }
  //l+=currentAreaL_->elementByColumn-elementL_;
  //CoinBigIndex lSave=l;
  CoinBigIndex *  COIN_RESTRICT startColumnL = startColumnLAddress_;
  CoinFactorizationDouble *  COIN_RESTRICT elementL = elementLAddress_;
  CoinSimplexInt *  COIN_RESTRICT indexRowL = indexRowLAddress_;
  startColumnL[numberGoodL_] = l;	//for luck and first time
  numberGoodL_++;
  startColumnL[numberGoodL_] = l + 1;
  lengthL_++;
  CoinFactorizationDouble pivotElement;
  CoinFactorizationDouble otherMultiplier;
  CoinSimplexInt otherRow;
  CoinSimplexInt *  COIN_RESTRICT saveColumn = saveColumnAddress_;
  CoinFactorizationDouble * COIN_RESTRICT elementU = elementUAddress_;
  CoinSimplexInt *  COIN_RESTRICT indexRowU = indexRowUAddress_;

  if ( indexRowU[startColumn] == pivotRow ) {
    pivotElement = elementU[startColumn];
    otherMultiplier = elementU[startColumn + 1];
    otherRow = indexRowU[startColumn + 1];
  } else {
    pivotElement = elementU[startColumn + 1];
    otherMultiplier = elementU[startColumn];
    otherRow = indexRowU[startColumn];
  }
  CoinSimplexInt numberSave = numberInRow[otherRow];
  CoinFactorizationDouble pivotMultiplier = 1.0 / pivotElement;

  CoinFactorizationDouble *  COIN_RESTRICT pivotRegion = pivotRegionAddress_;
  pivotRegion[numberGoodU_] = pivotMultiplier;
  numberInColumn[pivotColumn] = 0;
  otherMultiplier = otherMultiplier * pivotMultiplier;
#ifdef SMALL_PERMUTE
  indexRowL[l] = fromSmallToBigRow_[otherRow];
#else
  indexRowL[l] = otherRow;
#endif
  elementL[l] = otherMultiplier;
  //take out of row list
  CoinBigIndex start = startRowU[otherRow];
  CoinBigIndex end = start + numberSave;
  CoinBigIndex where = start;
  CoinSimplexInt *  COIN_RESTRICT indexColumnU = indexColumnUAddress_;

  while ( indexColumnU[where] != pivotColumn ) {
    where++;
  }				/* endwhile */
  assert ( where < end );
#if BOTH_WAYS
#endif
  end--;
  indexColumnU[where] = indexColumnU[end];
  CoinSimplexInt numberAdded = 0;
  CoinSimplexInt numberDeleted = 0;

  //pack down and move to work
  CoinSimplexInt j;
  const CoinSimplexInt *  COIN_RESTRICT nextCount = this->nextCountAddress_;
  CoinSimplexInt *  COIN_RESTRICT nextColumn = nextColumnAddress_;
  for ( j = startRow; j < endRow; j++ ) {
    CoinSimplexInt iColumn = indexColumnU[j];

    if ( iColumn != pivotColumn ) {
      CoinBigIndex startColumn = startColumnU[iColumn];
      CoinSimplexInt numberInColumnIn=numberInColumn[iColumn];
      CoinBigIndex endColumn = startColumn + numberInColumnIn;
      CoinSimplexInt iRow = indexRowU[startColumn];
      CoinFactorizationDouble value = elementU[startColumn];
      CoinFactorizationDouble largest;
      bool foundOther = false;

      //leave room for pivot
      CoinBigIndex put = startColumn + 1;
      CoinBigIndex positionLargest = -1;
      CoinFactorizationDouble thisPivotValue = 0.0;
      CoinFactorizationDouble otherElement = 0.0;
      CoinFactorizationDouble nextValue = elementU[put];;
      CoinSimplexInt nextIRow = indexRowU[put];

      //compress column and find largest not updated
      if ( iRow != pivotRow ) {
	if ( iRow != otherRow ) {
	  largest = fabs ( value );
	  elementU[put] = value;
	  indexRowU[put] = iRow;
	  positionLargest = put;
#if BOTH_WAYS
#endif
	  put++;
	  CoinBigIndex i;
	  for ( i = startColumn + 1; i < endColumn; i++ ) {
	    iRow = nextIRow;
	    value = nextValue;
	    nextIRow = indexRowU[i + 1];
	    nextValue = elementU[i + 1];
	    if ( iRow != pivotRow ) {
	      if ( iRow != otherRow ) {
		//keep
		indexRowU[put] = iRow;
		elementU[put] = value;;
#if BOTH_WAYS
#endif
		put++;
	      } else {
		otherElement = value;
		foundOther = true;
	      }
	    } else {
	      thisPivotValue = value;
	    }
	  }
	} else {
	  otherElement = value;
	  foundOther = true;
	  //need to find largest
	  largest = 0.0;
	  CoinBigIndex i;
	  for ( i = startColumn + 1; i < endColumn; i++ ) {
	    iRow = nextIRow;
	    value = nextValue;
	    nextIRow = indexRowU[i + 1];
	    nextValue = elementU[i + 1];
	    if ( iRow != pivotRow ) {
	      //keep
	      indexRowU[put] = iRow;
	      elementU[put] = value;;
#if BOTH_WAYS
#endif
	      CoinFactorizationDouble absValue = fabs ( value );

	      if ( absValue > largest ) {
		largest = absValue;
		positionLargest = put;
	      }
	      put++;
	    } else {
	      thisPivotValue = value;
	    }
	  }
	}
      } else {
	//need to find largest
	largest = 0.0;
	thisPivotValue = value;
	CoinBigIndex i;
	for ( i = startColumn + 1; i < endColumn; i++ ) {
	  iRow = nextIRow;
	  value = nextValue;
	  nextIRow = indexRowU[i + 1];
	  nextValue = elementU[i + 1];
	  if ( iRow != otherRow ) {
	    //keep
	    indexRowU[put] = iRow;
	    elementU[put] = value;;
#if BOTH_WAYS
#endif
	    CoinFactorizationDouble absValue = fabs ( value );

	    if ( absValue > largest ) {
	      largest = absValue;
	      positionLargest = put;
	    }
	    put++;
	  } else {
	    otherElement = value;
	    foundOther = true;
	  }
	}
      }
      //slot in pivot
      elementU[startColumn] = thisPivotValue;
#ifdef SMALL_PERMUTE
      indexRowU[startColumn] = realPivotRow;
#else
      indexRowU[startColumn] = pivotRow;
#endif
      //clean up counts
      startColumn++;
      numberInColumn[iColumn] = put - startColumn;
      numberInColumnPlus[iColumn]++;
      startColumnU[iColumn]++;
      otherElement = otherElement - thisPivotValue * otherMultiplier;
      CoinFactorizationDouble absValue = fabs ( otherElement );

      if ( !TEST_LESS_THAN_TOLERANCE_REGISTER(absValue) ) {
	if ( !foundOther ) {
	  //have we space
	  saveColumn[numberAdded++] = iColumn;
	  CoinSimplexInt next = nextColumn[iColumn];
	  CoinBigIndex space;

	  space = startColumnU[next] - put - numberInColumnPlus[next];
	  if ( space <= 0 ) {
	    //getColumnSpace also moves fixed part
	    CoinSimplexInt number = numberInColumn[iColumn];

	    if ( !getColumnSpace ( iColumn, number + 1 ) ) {
	      return false;
	    }
	    //redo starts
	    positionLargest =
	      positionLargest + startColumnU[iColumn] - startColumn;
	    startColumn = startColumnU[iColumn];
	    put = startColumn + number;
	  }
	}
	elementU[put] = otherElement;
	indexRowU[put] = otherRow;
#if BOTH_WAYS
#endif
	if ( absValue > largest ) {
	  largest = absValue;
	  positionLargest = put;
	}
	put++;
      } else {
	if ( foundOther ) {
	  numberDeleted++;
	  //take out of row list
	  CoinBigIndex where = start;

	  while ( indexColumnU[where] != iColumn ) {
	    where++;
	  }			/* endwhile */
	  assert ( where < end );
#if BOTH_WAYS
#endif
	  end--;
	  indexColumnU[where] = indexColumnU[end];
	}
      }
      CoinSimplexInt numberInColumnOut = put - startColumn;
      numberInColumn[iColumn] = numberInColumnOut;
      //move largest
      if ( positionLargest >= 0 ) {
	value = elementU[positionLargest];
	iRow = indexRowU[positionLargest];
	elementU[positionLargest] = elementU[startColumn];
	indexRowU[positionLargest] = indexRowU[startColumn];
	elementU[startColumn] = value;
	indexRowU[startColumn] = iRow;
#if BOTH_WAYS
#endif
      }
      //linked list for column
      if ( numberInColumnIn!=numberInColumnOut&&nextCount[iColumn + numberRows_] != -2 ) {
	//modify linked list
	modifyLink ( iColumn + numberRows_, numberInColumnOut );
      }
    }
  }
  //get space for row list
  next = nextRow[otherRow];
  CoinBigIndex space;

  space = startRowU[next] - end;
  totalElements_ += numberAdded - numberDeleted;
  CoinSimplexInt number = numberAdded + ( end - start );

  if ( space < numberAdded ) {
    numberInRow[otherRow] = end - start;
    if ( !getRowSpace ( otherRow, number ) ) {
      return false;
    }
    end = startRowU[otherRow] + end - start;
  }
  // do linked lists and update counts
  numberInRow[otherRow] = number;
  if ( number != numberSave ) {
    modifyLink ( otherRow, number );
  }
  for ( j = 0; j < numberAdded; j++ ) {
#if BOTH_WAYS
#endif
    indexColumnU[end++] = saveColumn[j];
  }
  lastEntryByRowU_ = CoinMax(end,lastEntryByRowU_);
  assert (lastEntryByRowU_<=lengthAreaU_);
  //modify linked list for pivots
  deleteLink ( pivotRow );
  deleteLink ( pivotColumn + numberRows_ );
  return true;
}
// Delete all stuff
void 
CoinAbcTypeFactorization::almostDestructor()
{
  gutsOfDestructor(1);
}
// PreProcesses column ordered copy of basis
void 
CoinAbcTypeFactorization::preProcess ( )
{
  CoinBigIndex numberElements = startColumnUAddress_[numberRows_-1]
    + numberInColumnAddress_[numberRows_-1];
  setNumberElementsU(numberElements);
}
// Return largest
double 
CoinAbcTypeFactorization::preProcess3 ( )
{
  CoinSimplexInt * COIN_RESTRICT numberInRow = numberInRowAddress_;
  CoinSimplexInt * COIN_RESTRICT numberInColumn = numberInColumnAddress_;
  CoinSimplexInt * COIN_RESTRICT numberInColumnPlus = numberInColumnPlusAddress_;
  CoinBigIndex * COIN_RESTRICT startRow = startRowUAddress_;
  CoinBigIndex * COIN_RESTRICT startColumn = startColumnUAddress_;

  totalElements_ = lengthU_;
  //links and initialize pivots
  CoinZeroN ( numberInColumn+numberRows_, maximumRowsExtra_ + 1 - numberRows_);
  CoinSimplexInt * COIN_RESTRICT lastRow = lastRowAddress_;
  CoinSimplexInt * COIN_RESTRICT nextRow = nextRowAddress_;
  
  CoinFillN ( pivotColumnAddress_, numberRows_, -1 );
  CoinZeroN ( numberInColumnPlus, maximumRowsExtra_ + 1 );
  CoinZeroN ( lastRow, numberRows_ );
  startColumn[maximumRowsExtra_] = lengthU_;
  lastEntryByColumnU_=lengthU_;
  lastEntryByRowU_=lengthU_;
  
  //do slacks first
  //CoinBigIndex * startColumnU = startColumnUAddress_;
  CoinBigIndex *  COIN_RESTRICT startColumnL = startColumnLAddress_;
  CoinSimplexInt * COIN_RESTRICT indexRow = indexRowUAddress_;
  //CoinSimplexInt * COIN_RESTRICT indexColumn = indexColumnUAddress_;
  CoinFactorizationDouble * COIN_RESTRICT element = elementUAddress_;
  CoinFactorizationDouble *  COIN_RESTRICT pivotRegion = pivotRegionAddress_;
  CoinSimplexInt *  COIN_RESTRICT pivotColumn = pivotColumnAddress_;
  //CoinFillN(pivotRegion,numberSlacks_,static_cast<double>(SLACK_VALUE));
  // **************** TEMP *********************
  CoinFillN(pivotRegion,numberSlacks_,static_cast<CoinFactorizationDouble>(1.0));
  CoinZeroN(startColumnL,numberSlacks_+1);
  CoinZeroN(numberInColumn,numberSlacks_);
  CoinZeroN(numberInColumnPlus,numberSlacks_);
#ifdef ABC_USE_FUNCTION_POINTERS
  // Use row copy
  elementUColumnPlusAddress_ = elementRowU_.array();
  scatterStruct * COIN_RESTRICT scatter = scatterUColumn();
#if ABC_USE_FUNCTION_POINTERS
  extern scatterUpdate AbcScatterLowSubtract[9];
#endif
  lastEntryByColumnUPlus_=1;
  //for (int i=0;i<numberRows_;i++) {
  //scatter[iRow].functionPointer=AbcScatterLow[0];
  //scatter[iRow].offset=1;
  //scatter[iRow].number=0;
  //}
  //extern scatterUpdate AbcScatterHigh[4];
#endif
  CoinSimplexInt * COIN_RESTRICT permute = permuteAddress_;
  CoinFillN(permute,numberRows_,-1);
  for (CoinSimplexInt iSlack = 0; iSlack < numberSlacks_;iSlack++ ) {
    // slack
    CoinSimplexInt iRow = indexRow[iSlack];
    lastRow[iRow] =-2;
    //nextRow[iRow] = numberGoodU_;	//use for permute
    permute[iRow]=iSlack; 
    numberInRow[iRow]=0;
    pivotColumn[iSlack] = iSlack;
#ifdef ABC_USE_FUNCTION_POINTERS
#if ABC_USE_FUNCTION_POINTERS
    scatter[iRow].functionPointer=AbcScatterLowSubtract[0];
#endif
    scatter[iRow].offset=0;
    scatter[iRow].number=0;
#endif
  }
  totalElements_ -= numberSlacks_;
  numberGoodU_=numberSlacks_;
  numberGoodL_=numberSlacks_;
#if CONVERTROW>1
  CoinBigIndex * COIN_RESTRICT convertRowToColumn = convertRowToColumnUAddress_;
  CoinBigIndex * COIN_RESTRICT convertColumnToRow = convertColumnToRowUAddress_;
#endif
#ifdef SMALL_PERMUTE
  CoinSimplexInt * COIN_RESTRICT fromBigToSmallRow=reinterpret_cast<CoinSimplexInt *>(saveColumn_.array());
  CoinSimplexInt * COIN_RESTRICT indexColumn = indexColumnUAddress_;
  //move largest in column to beginning
  // If we always compress then can combine
  CoinSimplexInt *  COIN_RESTRICT tempIndex = firstCountAddress_; //saveColumnAddress_;
  CoinFactorizationDouble *  COIN_RESTRICT workArea = workAreaAddress_;
  // Compress
  int nSmall=0;
  // fromBig is only used at setup time
  for (int i=0;i<numberRows_;i++) {
    if (permute[i]>=0) {
      // out
      fromBigToSmallRow[i]=-1;
    } else {
      // in
      fromBigToSmallRow[i]=nSmall;
      numberInRow[nSmall]=numberInRow[i];
      lastRow[nSmall]=lastRow[i];
      fromSmallToBigRow_[nSmall++]=i;
    }
  }
  nSmall=0;
  for (CoinSimplexInt iColumn = numberSlacks_; iColumn < numberRows_; iColumn++ ) {
    CoinSimplexInt number = numberInColumn[iColumn];
    // use workArea and tempIndex for remaining elements
    CoinBigIndex first = startColumn[iColumn];
    int saveFirst=first;
    fromSmallToBigColumn_[nSmall]=iColumn;
    CoinBigIndex largest = -1;
    CoinFactorizationDouble valueLargest = -1.0;
    CoinSimplexInt nOther=0;
    CoinBigIndex end = first+number;
    for (CoinBigIndex k = first ; k < end; k++ ) {
      CoinSimplexInt iRow = indexRow[k];
      assert (iRow<numberRowsSmall_);
      CoinFactorizationDouble value = element[k];
      if (permute[iRow]<0) {
	CoinFactorizationDouble valueAbs = fabs ( value );
	if ( valueAbs > valueLargest ) {
	  valueLargest = valueAbs;
	  largest = nOther;
	}
	iRow=fromBigToSmallRow[iRow];
	assert (iRow>=0);
	tempIndex[nOther]=iRow;
	workArea[nOther++]=value;
      } else {
	indexRow[first] = iRow;
	element[first++] = value;
      }
    }
    CoinSimplexInt nMoved = first-saveFirst;
    numberInColumnPlus[nSmall]=nMoved;
    totalElements_ -= nMoved;
    startColumn[nSmall]=first;
    //largest
    if (largest>=0) {
      indexRow[first] = tempIndex[largest];
      element[first++] = workArea[largest];
    }
    for (CoinBigIndex k=0;k<nOther;k++) {
      if (k!=largest) {
	indexRow[first] = tempIndex[k];
	element[first++] = workArea[k];
      }
    }
    numberInColumn[nSmall]=first-startColumn[nSmall];
    nSmall++;
  }
  numberRowsSmall_=nSmall;
  CoinAbcMemset0(numberInRow+nSmall,numberRows_-nSmall);
  CoinAbcMemset0(numberInColumn+nSmall,numberRows_-nSmall);
#endif
  //row part
  CoinBigIndex i = 0;
  for (CoinSimplexInt iRow = 0; iRow < numberRowsSmall_; iRow++ ) {
    startRow[iRow] = i;
    CoinSimplexInt n=numberInRow[iRow];
#ifndef SMALL_PERMUTE
    numberInRow[iRow]=0;
#endif
    i += n;
  }
  startRow[numberRows_]=lengthAreaU_;//i;
  double overallLargest=1.0;
#ifndef SMALL_PERMUTE
  CoinSimplexInt * COIN_RESTRICT indexColumn = indexColumnUAddress_;
  //move largest in column to beginning
  CoinSimplexInt *  COIN_RESTRICT tempIndex = firstCountAddress_; //saveColumnAddress_;
  CoinFactorizationDouble *  COIN_RESTRICT workArea = workAreaAddress_;
  for (CoinSimplexInt iColumn = numberSlacks_; iColumn < numberRows_; iColumn++ ) {
    CoinSimplexInt number = numberInColumn[iColumn];
    // use workArea and tempIndex for remaining elements
    CoinBigIndex first = startColumn[iColumn];
    CoinBigIndex largest = -1;
    CoinFactorizationDouble valueLargest = -1.0;
    CoinSimplexInt nOther=0;
    CoinBigIndex end = first+number;
    for (CoinBigIndex k = first ; k < end; k++ ) {
      CoinSimplexInt iRow = indexRow[k];
      assert (iRow<numberRowsSmall_);
      CoinFactorizationDouble value = element[k];
      if (!lastRow[iRow]) {
	numberInRow[iRow]++;
	CoinFactorizationDouble valueAbs = fabs ( value );
	if ( valueAbs > valueLargest ) {
	  valueLargest = valueAbs;
	  largest = nOther;
	}
	tempIndex[nOther]=iRow;
	workArea[nOther++]=value;
#if CONVERTROW<2
	CoinSimplexInt iLook = startRow[iRow];
	startRow[iRow] = iLook + 1;
	indexColumn[iLook] = iColumn;
#endif
#if BOTH_WAYS
#endif
      } else {
	indexRow[first] = iRow;
#if BOTH_WAYS
#endif
	element[first++] = value;
      }
    }
    overallLargest=CoinMax(overallLargest,valueLargest);
    CoinSimplexInt nMoved = first-startColumn[iColumn];
    numberInColumnPlus[iColumn]=nMoved;
    totalElements_ -= nMoved;
    startColumn[iColumn]=first;
    //largest
    if (largest>=0) {
#if CONVERTROW>1
      int iRow=tempIndex[largest];
      indexRow[first] = iRow;
      CoinSimplexInt iLook = startRow[iRow];
      startRow[iRow] = iLook + 1;
      indexColumn[iLook] = iColumn;
      convertRowToColumn[iLook]=first;
      convertColumnToRow[first]=iLook;
#else
      indexRow[first] = tempIndex[largest];
#endif
#if BOTH_WAYS
#endif
      element[first++] = workArea[largest];
    }
    for (CoinBigIndex k=0;k<nOther;k++) {
      if (k!=largest) {
#if CONVERTROW>1
      int iRow=tempIndex[k];
      indexRow[first] = iRow;
      CoinSimplexInt iLook = startRow[iRow];
      startRow[iRow] = iLook + 1;
      indexColumn[iLook] = iColumn;
      convertRowToColumn[iLook]=first;
      convertColumnToRow[first]=iLook;
#else
      indexRow[first] = tempIndex[k];
#endif
#if BOTH_WAYS
#endif
	element[first++] = workArea[k];
      }
    }
    numberInColumn[iColumn]=first-startColumn[iColumn];
  }
#else
#if CONVERTROW>1
  for (CoinSimplexInt iColumn = 0; iColumn < numberRowsSmall_; iColumn++ ) {
    CoinSimplexInt number = numberInColumn[iColumn];
    CoinBigIndex first = startColumn[iColumn];
    CoinBigIndex end = first+number;
    for (CoinBigIndex k = first ; k < end; k++ ) {
      CoinSimplexInt iRow = indexRow[k];
      CoinSimplexInt iLook = startRow[iRow];
      startRow[iRow] = iLook + 1;
      indexColumn[iLook] = iColumn;
      convertRowToColumn[iLook]=k;
      convertColumnToRow[k]=iLook;
    }
  }
#endif
#endif
  for (CoinSimplexInt iRow=numberRowsSmall_-1;iRow>0;iRow--) 
    startRow[iRow]=startRow[iRow-1];
  startRow[0]=0;
  // Do links (do backwards to get rows first?)
  CoinSimplexInt * firstCount=tempIndex;
  for (int i=0;i<numberRows_+2;i++) {
    firstCount[i]=i-numberRows_-2;
    //assert (&nextCount_[firstCount[i]]==firstCount+i);
  }
  startColumnLAddress_[0] = 0;	//for luck
  CoinSimplexInt *lastColumn = lastColumnAddress_;
  CoinSimplexInt *nextColumn = nextColumnAddress_;
#ifdef SMALL_PERMUTE
  // ? do I need to do slacks (and if so can I do faster)
  for (CoinSimplexInt iColumn = 0; iColumn <numberRowsSmall_; iColumn++ ) {
    lastColumn[iColumn] = iColumn - 1;
    nextColumn[iColumn] = iColumn + 1;
    CoinSimplexInt number = numberInColumn[iColumn];
    addLink ( iColumn + numberRows_, number );
  }
  lastColumn[maximumRowsExtra_] = numberRowsSmall_ - 1;
  nextColumn[maximumRowsExtra_] = 0;
  lastColumn[0] = maximumRowsExtra_;
  if (numberRowsSmall_)
    nextColumn[numberRowsSmall_ - 1] = maximumRowsExtra_;
#else
  lastColumn[maximumRowsExtra_] = numberRows_ - 1;
  for (CoinSimplexInt iColumn = 0; iColumn <numberRows_; iColumn++ ) {
    lastColumn[iColumn] = iColumn - 1;
    nextColumn[iColumn] = iColumn + 1;
    CoinSimplexInt number = numberInColumn[iColumn];
    addLink ( iColumn + numberRows_, number );
  }
  nextColumn[maximumRowsExtra_] = 0;
  lastColumn[0] = maximumRowsExtra_;
  if (numberRows_)
    nextColumn[numberRows_ - 1] = maximumRowsExtra_;
#endif
  startColumn[maximumRowsExtra_] = lengthU_;
  lastEntryByColumnU_=lengthU_;
  CoinSimplexInt jRow=numberRows_;
  for (CoinSimplexInt iRow = 0; iRow < numberRowsSmall_; iRow++ ) {
    CoinSimplexInt number = numberInRow[iRow];
    if (!lastRow[iRow]) {
      lastRow[iRow]=jRow;
      nextRow[jRow]=iRow;
      jRow=iRow;
      addLink ( iRow, number );
    }
  }
  // Is this why startRow[max] must be 0
  lastRow[numberRows_] = jRow;
  nextRow[jRow] = numberRows_;
  //checkLinks(999);
  return overallLargest;
}
// Does post processing on valid factorization - putting variables on correct rows
void 
CoinAbcTypeFactorization::postProcess(const CoinSimplexInt * sequence, CoinSimplexInt * pivotVariable)
{
  const CoinSimplexInt *  COIN_RESTRICT back = firstCountAddress_+numberRows_;
  if (pivotVariable) {
#ifndef NDEBUG
    CoinFillN(pivotVariable, numberRows_, -1);
#endif
    // Redo pivot order
    for (CoinSimplexInt i = 0; i < numberRows_; i++) {
      CoinSimplexInt k = sequence[i];
      CoinSimplexInt j = back[i];
      assert (pivotVariable[j] == -1);
      pivotVariable[j] = k;
    }
  }
  // Set up permutation vector
  // these arrays start off as copies of permute
  // (and we could use permute_ instead of pivotColumn (not back though))
  CoinAbcMemcpy( pivotColumnAddress_  , permuteAddress_, numberRows_ );
  // See if worth going sparse and when
  checkSparse();
  // Set pivotRegion in case we go off end
  pivotRegionAddress_[-1]=0.0;
  pivotRegionAddress_[numberRows_]=0.0;
  const CoinSimplexInt *  COIN_RESTRICT pivotLOrder = this->pivotLOrderAddress_;
  CoinSimplexInt *  COIN_RESTRICT pivotLinkedForwards = this->pivotLinkedForwardsAddress_;
  CoinSimplexInt *  COIN_RESTRICT pivotLinkedBackwards = this->pivotLinkedBackwardsAddress_;
  CoinSimplexInt iThis=-1;
  for (int i=0;i<numberSlacks_;i++) {
#ifndef ABC_ORDERED_FACTORIZATION
    CoinSimplexInt iNext=pivotLOrder[i];
#else
    CoinSimplexInt iNext=i;
#endif
    pivotLinkedForwards[iThis]=iNext;
    iThis=iNext;
  }
  lastSlack_ = iThis;
  for (int i=numberSlacks_;i<numberRows_;i++) {
#ifndef ABC_ORDERED_FACTORIZATION
    CoinSimplexInt iNext=pivotLOrder[i];
#else
    CoinSimplexInt iNext=i;
#endif
    pivotLinkedForwards[iThis]=iNext;
    iThis=iNext;
  }
  pivotLinkedForwards[iThis]=numberRows_;
  pivotLinkedForwards[numberRows_]=-1;
  iThis=numberRows_;
  for (int i=numberRows_-1;i>=0;i--) {
#ifndef ABC_ORDERED_FACTORIZATION
    CoinSimplexInt iNext=pivotLOrder[i];
#else
    CoinSimplexInt iNext=i;
#endif
    pivotLinkedBackwards[iThis]=iNext;
    iThis=iNext;
  }
  pivotLinkedBackwards[iThis]=-1;
  pivotLinkedBackwards[-1]=-2;
#if 0 //ndef NDEBUG
  {
    char * test = new char[numberRows_];
    memset(test,0,numberRows_);
    CoinBigIndex *  COIN_RESTRICT startColumnL = startColumnLAddress_;
    CoinSimplexInt *  COIN_RESTRICT indexRowL = indexRowLAddress_;
    const CoinSimplexInt * COIN_RESTRICT pivotLOrder = pivotLOrderAddress_;
    //CoinSimplexInt *  COIN_RESTRICT pivotLOrder = firstCountAddress_;
    //const CoinSimplexInt * COIN_RESTRICT pivotLBackwardOrder = permuteAddress_;
    for (int i=0;i<numberRows_;i++) {
      //CoinSimplexInt pivotOrder=permuteAddress_[i];
#ifndef ABC_ORDERED_FACTORIZATION
      CoinSimplexInt iRow=pivotLOrder[i];
#else
      CoinSimplexInt iRow=i;
#endif
      assert(iRow>=0&&iRow<numberRows_);
      test[iRow]=1;
      CoinBigIndex start=startColumnL[i];
      CoinBigIndex end=startColumnL[i+1];
      for (CoinBigIndex j = start;j < end; j ++ ) {
	CoinSimplexInt jRow = indexRowL[j];
	assert (test[jRow]==0);
      }
    }
    delete [] test;
  }
#endif
  //memcpy(pivotColumnAddress_,pivotLOrder,numberRows_*sizeof(CoinSimplexInt));
  if (gotRCopy()) {
    //if (numberInColumnPlusAddress_) {
    CoinBigIndex *  COIN_RESTRICT startR = startColumnRAddress_ + maximumPivots_+1;
    // do I need to zero
    assert(startR+maximumRowsExtra_+1<=firstCountAddress_+CoinMax(5*numberRows_,4*numberRows_+2*maximumPivots_+4)+2 );
    CoinZeroN (startR,(maximumRowsExtra_+1));
  }
  goSparse2();
#ifndef ABC_USE_FUNCTION_POINTERS
  CoinSimplexInt *  COIN_RESTRICT numberInColumn = numberInColumnAddress_;
  CoinBigIndex * COIN_RESTRICT startColumnU = startColumnUAddress_;
  CoinFactorizationDouble * COIN_RESTRICT elementU = elementUAddress_;
  CoinFactorizationDouble * COIN_RESTRICT pivotRegion = pivotRegionAddress_;
#else
  scatterStruct * COIN_RESTRICT scatterPointer = scatterUColumn();
  CoinFactorizationDouble *  COIN_RESTRICT elementUColumnPlus = elementUColumnPlusAddress_;
#endif
  //lastEntryByRowU_ = totalElements_;
#if ABC_SMALL<2
  CoinSimplexInt * COIN_RESTRICT indexRowU = indexRowUAddress_;
  if (gotUCopy()) {
    CoinSimplexInt *  COIN_RESTRICT numberInRow = numberInRowAddress_;
    CoinBigIndex *  COIN_RESTRICT startRow = startRowUAddress_;
    CoinSimplexInt *  COIN_RESTRICT nextRow = nextRowAddress_;
    CoinSimplexInt *  COIN_RESTRICT lastRow = lastRowAddress_;
    CoinSimplexInt * COIN_RESTRICT indexColumnU = indexColumnUAddress_;
#ifndef ABC_USE_FUNCTION_POINTERS
    for ( CoinSimplexInt i = 0; i < numberU_; i++ ) {
      CoinBigIndex start = startColumnU[i];
      CoinBigIndex end = start + numberInColumn[i];
      CoinBigIndex j;
      // only needed if row copy
      for ( j = start; j < end; j++ ) {
	CoinSimplexInt iRow = indexRowU[j];
	numberInRow[iRow]++;
      }
    }
#else
    for ( CoinSimplexInt i = 0; i < numberU_; i++ ) {
      scatterStruct & COIN_RESTRICT scatter = scatterPointer[i];
      CoinSimplexInt number = scatter.number;
      const CoinFactorizationDouble * COIN_RESTRICT area = elementUColumnPlus+scatter.offset;
      const CoinBigIndex * COIN_RESTRICT indices = reinterpret_cast<const CoinBigIndex *>(area+number);
      for (int j = 0; j < number; j++ ) {
	CoinSimplexInt iRow = indices[j];
	assert (iRow>=0&&iRow<numberRows_);
	numberInRow[iRow]++;
      }
    }
#endif
    CoinFactorizationDouble * COIN_RESTRICT elementRow = elementRowUAddress_;
    CoinBigIndex j = 0;
    
    CoinSimplexInt iRow;
    for ( iRow = 0; iRow < numberRows_; iRow++ ) {
      startRow[iRow] = j;
      j += numberInRow[iRow];
    }
    lastEntryByRowU_ = j;
    
#if CONVERTROW
    CoinBigIndex * COIN_RESTRICT convertRowToColumn = convertRowToColumnUAddress_;
#if CONVERTROW>2
    CoinBigIndex * COIN_RESTRICT convertColumnToRow = convertColumnToRowUAddress_;
#endif
#endif
#ifndef ABC_USE_FUNCTION_POINTERS
    CoinZeroN ( numberInRow, numberRows_+1 );
    for (CoinSimplexInt i = 0; i < numberRows_; i++ ) {
      CoinBigIndex start = startColumnU[i];
      CoinBigIndex end = start + numberInColumn[i];
      CoinFactorizationDouble pivotValue = pivotRegion[i];
      CoinBigIndex j;
      for ( j = start; j < end; j++ ) {
	CoinSimplexInt iRow = indexRowU[j];
	CoinSimplexInt iLook = numberInRow[iRow];
	numberInRow[iRow] = iLook + 1;
	CoinBigIndex k = startRow[iRow] + iLook;
	indexColumnU[k] = i;
#if CONVERTROW
	convertRowToColumn[k] = j-start;
#if CONVERTROW>2
	convertColumnToRow[j] = k;
#endif
#endif
	//multiply by pivot
	elementU[j] *= pivotValue;
	elementRow[k] = elementU[j];
      }
    }
#else
    for (CoinSimplexInt i = 0; i < numberRows_; i++ ) {
      scatterStruct & COIN_RESTRICT scatter = scatterPointer[i];
      CoinSimplexInt number = scatter.number;
      const CoinFactorizationDouble * COIN_RESTRICT area = elementUColumnPlus+scatter.offset;
      const CoinBigIndex * COIN_RESTRICT indices = reinterpret_cast<const CoinBigIndex *>(area+number);
      for (int j = 0; j < number; j++ ) {
	CoinSimplexInt iRow = indices[j];
	CoinBigIndex k = startRow[iRow];
	startRow[iRow]=k+1;
	indexColumnU[k] = i;
#if CONVERTROW
	convertRowToColumn[k] = j;
#if CONVERTROW>2
	convertColumnToRow[j+startColumnU[i]] = k;
	abort(); // fix
#endif
#endif
	//CoinFactorizationDouble pivotValue = pivotRegion[i];
	//multiply by pivot
	//elementU[j+startColumnU[i]] *= pivotValue;
	elementRow[k] = area[j];
      }
    }
#endif
    //CoinSimplexInt *  COIN_RESTRICT nextRow = nextRowAddress_;
    //CoinSimplexInt *  COIN_RESTRICT lastRow = lastRowAddress_;
    for (int j = numberRows_-1; j >=0; j-- ) {
      lastRow[j] = j - 1;
      nextRow[j] = j + 1;
#ifdef ABC_USE_FUNCTION_POINTERS
      startRow[j+1]=startRow[j];
#endif
    }
    startRow[0]=0;
    nextRow[numberRows_ - 1] = numberRows_;
    lastRow[numberRows_] = numberRows_ - 1;
    nextRow[numberRows_] = 0;
    lastRow[0] = numberRows_;
    //memcpy(firstCountAddress_+numberRows_+1,pivotLinkedBackwards-1,(numberRows_+2)*sizeof(int));
  } else {
#endif
#ifndef ABC_USE_FUNCTION_POINTERS
    for (CoinSimplexInt i = 0; i < numberRows_; i++ ) {
      CoinBigIndex start = startColumnU[i];
      CoinBigIndex end = start + numberInColumn[i];
      CoinFactorizationDouble pivotValue = pivotRegion[i];
      CoinBigIndex j;
      for ( j = start; j < end; j++ ) {
	//multiply by pivot
	elementU[j] *= pivotValue;
      }
    }
#endif
#if ABC_SMALL<2
  }
#endif
#ifdef ABC_USE_FUNCTION_POINTERS
#if 0
  {
    CoinFactorizationDouble *  COIN_RESTRICT elementUColumnPlus = elementUColumnPlusAddress_;
    CoinSimplexInt * COIN_RESTRICT indexRow = indexRowUAddress_;
    CoinFactorizationDouble * COIN_RESTRICT element = elementUAddress_;
    CoinSimplexInt *  COIN_RESTRICT numberInColumn = numberInColumnAddress_;
    CoinBigIndex *  COIN_RESTRICT startColumnU = startColumnUAddress_;
    scatterStruct * COIN_RESTRICT scatter = scatterUColumn();
    extern scatterUpdate AbcScatterLowSubtract[9];
    extern scatterUpdate AbcScatterHighSubtract[4];
    for (int iRow=0;iRow<numberRows_;iRow++) {
      int number=scatter[iRow].number;
      CoinBigIndex offset = scatter[iRow].offset;
      CoinFactorizationDouble * COIN_RESTRICT area = elementUColumnPlus+offset;
      CoinSimplexInt * COIN_RESTRICT indices = reinterpret_cast<CoinSimplexInt *>(area+number);
      CoinSimplexInt * COIN_RESTRICT indices2 = indexRow+startColumnU[iRow];
      CoinFactorizationDouble * COIN_RESTRICT area2 = element+startColumnU[iRow];
      assert (number==numberInColumn[iRow]);
      if (number<9)
	assert (scatter[iRow].functionPointer==AbcScatterLowSubtract[number]);
      else
	assert (scatter[iRow].functionPointer==AbcScatterHighSubtract[number&3]);
      for (int i=0;i<number;i++) {
	assert (indices[i]==indices2[i]);
	assert (area[i]==area2[i]);
      }
    }
  }
#endif
#endif
}
// Makes a non-singular basis by replacing variables
void 
CoinAbcTypeFactorization::makeNonSingular(CoinSimplexInt *  COIN_RESTRICT sequence)
{
  // Replace bad ones by correct slack
  CoinSimplexInt *  COIN_RESTRICT workArea = indexRowUAddress_;
  for (CoinSimplexInt i=0;i<numberRows_;i++) 
    workArea[i]=-1;
  const CoinSimplexInt *  COIN_RESTRICT pivotColumn = pivotColumnAddress_;
  const CoinSimplexInt *  COIN_RESTRICT permute = permuteAddress_;
  for ( CoinSimplexInt i=0;i<numberGoodU_;i++) {
    CoinSimplexInt iOriginal = pivotColumn[i];
    assert (iOriginal>=0);
    workArea[iOriginal]=i;
  }
  CoinSimplexInt iRow=0;
  for (CoinSimplexInt i=0;i<numberRows_;i++) {
    if (workArea[i]==-1) {
      while (permute[iRow]>=0)
	iRow++;
      assert (iRow<numberRows_);
      // Put slack in basis
      sequence[i]=iRow;
      iRow++;
    }
  }
}

//  show_self.  Debug show object
void
CoinAbcTypeFactorization::show_self (  ) const
{
  CoinSimplexInt i;

  const CoinSimplexInt * pivotColumn = pivotColumnAddress_;
  for ( i = 0; i < numberRows_; i++ ) {
    std::cout << "r " << i << " " << pivotColumn[i];
    std::cout<< " " << permuteAddress_[i];
    std::cout<< " " << pivotRegionAddress_[i];
    std::cout << std::endl;
  }
  for ( i = 0; i < numberRows_; i++ ) {
    std::cout << "u " << i << " " << numberInColumnAddress_[i] << std::endl;
    CoinSimplexInt j;
    CoinSort_2(indexRowUAddress_+startColumnUAddress_[i],
	       indexRowUAddress_+startColumnUAddress_[i]+numberInColumnAddress_[i],
	       elementUAddress_+startColumnUAddress_[i]);
    for ( j = startColumnUAddress_[i]; j < startColumnUAddress_[i] + numberInColumnAddress_[i];
	  j++ ) {
      assert (indexRowUAddress_[j]>=0&&indexRowUAddress_[j]<numberRows_);
      assert (elementUAddress_[j]>-1.0e50&&elementUAddress_[j]<1.0e50);
      std::cout << indexRowUAddress_[j] << " " << elementUAddress_[j] << std::endl;
    }
  }
  for ( i = 0; i < numberRows_; i++ ) {
    std::cout << "l " << i << " " << startColumnLAddress_[i + 1] -
      startColumnLAddress_[i] << std::endl;
    CoinSort_2(indexRowLAddress_+startColumnLAddress_[i],
	       indexRowLAddress_+startColumnLAddress_[i+1],
	       elementLAddress_+startColumnLAddress_[i]);
    CoinSimplexInt j;
    for ( j = startColumnLAddress_[i]; j < startColumnLAddress_[i + 1]; j++ ) {
      std::cout << indexRowLAddress_[j] << " " << elementLAddress_[j] << std::endl;
    }
  }

}
//  sort so can compare
void
CoinAbcTypeFactorization::sort (  ) const
{
  CoinSimplexInt i;

  for ( i = 0; i < numberRows_; i++ ) {
    CoinSort_2(indexRowUAddress_+startColumnUAddress_[i],
	       indexRowUAddress_+startColumnUAddress_[i]+numberInColumnAddress_[i],
	       elementUAddress_+startColumnUAddress_[i]);
  }
  for ( i = 0; i < numberRows_; i++ ) {
    CoinSort_2(indexRowLAddress_+startColumnLAddress_[i],
	       indexRowLAddress_+startColumnLAddress_[i+1],
	       elementLAddress_+startColumnLAddress_[i]);
  }

}
#ifdef CHECK_LINKS
//static int ixxxxxx=0;
void
CoinAbcTypeFactorization::checkLinks(int type)
{
  //if (type<1)
  return;
#if 0 //ABC_SMALL<2
  if (numberRowsSmall_!=5698)
    return;
  ixxxxxx++;
  if (fromSmallToBigRow_[3195]==3581) {
    if (permuteAddress_[3581]>=0) {
      assert (numberInRowAddress_[3195]==0);
      CoinSimplexInt * numberInColumn = numberInColumnAddress_;
      CoinSimplexInt * COIN_RESTRICT indexRow = indexRowUAddress_;
      CoinBigIndex *  COIN_RESTRICT startColumn = startColumnUAddress_;
      for (int i=startColumn[1523];i<startColumn[1523]+numberInColumn[1523];i++)
	assert (indexRow[i]!=3195);
    }
    CoinSimplexInt *  COIN_RESTRICT numberInColumn = numberInColumnAddress_;
    CoinSimplexInt *  COIN_RESTRICT numberInColumnPlus = numberInColumnPlusAddress_;
    CoinSimplexInt *  COIN_RESTRICT nextColumn = nextColumnAddress_;
    CoinBigIndex *  COIN_RESTRICT startColumnU = startColumnUAddress_;
    CoinSimplexInt iColumn = nextColumn[maximumRowsExtra_];
    //CoinBigIndex put = startColumnU[iColumn] - numberInColumnPlus[iColumn];
    //CoinBigIndex putEnd = startColumnU[iColumn] + numberInColumn[iColumn];
    CoinBigIndex putEnd = -1;
    while ( iColumn != maximumRowsExtra_ ) {
      CoinBigIndex get;
      CoinBigIndex getEnd;
      get = startColumnU[iColumn] - numberInColumnPlus[iColumn];
      getEnd = startColumnU[iColumn] + numberInColumn[iColumn];
      assert (putEnd<=get);
      putEnd=getEnd;
      iColumn = nextColumn[iColumn];
    }
  }
  if (ixxxxxx>=7615900) {
    printf("trouble ahead\n");
  }
  return;
  if (ixxxxxx<406245)
    return;
#endif
  CoinSimplexInt * COIN_RESTRICT nextCount = this->nextCount();
  CoinSimplexInt * COIN_RESTRICT firstCount = this->firstCount();
  CoinSimplexInt * COIN_RESTRICT lastCount = this->lastCount();
  CoinSimplexInt * numberInColumn = numberInColumnAddress_;
  CoinSimplexInt * numberInRow = numberInRowAddress_;
  if (type) {
    int nBad=0;
    CoinSimplexInt * COIN_RESTRICT indexRow = indexRowUAddress_;
    CoinSimplexInt * COIN_RESTRICT indexColumn = indexColumnUAddress_;
    CoinBigIndex *  COIN_RESTRICT startColumn = startColumnUAddress_;
    CoinBigIndex * COIN_RESTRICT startRow = startRowUAddress_;
    for (int i=0;i<numberRowsSmall_;i++) {
      assert (startRow[i]+numberInRow[i]<=lastEntryByRowU_);
    }
    return;
    CoinSimplexInt * COIN_RESTRICT nextRow = nextRow_.array();
    CoinSimplexInt * COIN_RESTRICT lastRow = lastRow_.array() ;
    int * counts = new int [3*numberRows_+2];
    int * list1=counts+numberRows_;
    int * list2=list1+numberRows_+1;
    {
      int lastStart=-1;
      int iRow=nextRow[numberRows_];
      memset(list1,0,numberRowsSmall_*sizeof(int));
      while (iRow>=0&&iRow<numberRowsSmall_) {
	int iStart=startRow[iRow];
	assert (iStart>=lastStart);
	lastStart=iStart;
	list1[iRow]=1;
	iRow=nextRow[iRow];
      }
      for (int i=0;i<numberRowsSmall_;i++) { 
	assert (list1[i]||!numberInRow[i]);
	assert (!list1[i]||numberInRow[i]);
      }
      memset(list1,0,numberRowsSmall_*sizeof(int));
#if 1
      CoinSimplexInt * COIN_RESTRICT nextColumn = nextColumn_.array();
      CoinSimplexInt * COIN_RESTRICT lastColumn = lastColumn_.array() ;
      lastStart=-1;
      int iColumn=nextColumn[maximumRowsExtra_];
      int nDone=0;
      while (iColumn>=0&&iColumn<numberRowsSmall_) {
	nDone++;
	int iStart=startColumn[iColumn];
	assert (iStart>=lastStart);
	lastStart=iStart;
	list1[iColumn]=1;
	iColumn=nextColumn[iColumn];
      }
      int nEnd=0;
      for (int i=0;i<numberRowsSmall_;i++) {
	if (numberInColumn[i]&&nEnd==maximumRowsExtra_)
	  nEnd++;
      }
      if (nEnd>1) {
	for (int i=0;i<numberRowsSmall_;i++) {
	  if (numberInColumn[i]&&nEnd==maximumRowsExtra_)
	    printf("%d points to end\n",i);
	}
	assert (nEnd<=1);
      }
      //printf("rows %d goodU %d done %d iColumn %d\n",numberRowsSmall_,numberGoodU_,nDone,iColumn);
#endif
#if 0
      for (int i=0;i<numberRowsSmall_;i++) {
	assert (list1[i]||!numberInColumn[i]);
	assert (!list1[i]||numberInColumn[i]);
	//assert (list1[i]||!numberInColumn[fromSmallToBigColumn[i]]);
	//assert (!list1[i]||numberInColumn[fromSmallToBigColumn[i]]);
      }
#endif
    }
    CoinAbcMemset0(counts,numberRows_);
    for (int iColumn=0;iColumn<numberRows_;iColumn++) {
      for (CoinBigIndex j=startColumn[iColumn];j<startColumn[iColumn]+numberInColumn[iColumn];j++) {
	int iRow=indexRow[j];
	counts[iRow]++;
      }
    }
    int nTotal1=0;
    for (int i=0;i<numberRows_;i++) {
      if (numberInRow[i]!=counts[i]) {
	printf("Row %d numberInRow %d counts %d\n",i,numberInRow[i],counts[i]);
	int n1=0;
	int n2=0;
	for (int j=startRow[i];j<startRow[i]+numberInRow[i];j++)
	  list1[n1++]=indexColumn[j];
	for (int iColumn=0;iColumn<numberRows_;iColumn++) {
	  for (CoinBigIndex j=startColumn[iColumn];j<startColumn[iColumn]+numberInColumn[iColumn];j++) {
	    int iRow=indexRow[j];
	    if (i==iRow)
	      list2[n2++]=iColumn;
	  }
	}
	std::sort(list1,list1+n1);
	std::sort(list2,list2+n2);
	list1[n1]=numberRows_;
	list2[n2]=numberRows_;
	n1=1;
	n2=1;
	int iLook1=list1[0];
	int iLook2=list2[0];
	int xx=0;
	while(iLook1!=numberRows_||iLook2!=numberRows_) {
	  if (iLook1==iLook2) {
	    printf(" (rc %d)",iLook1);
	    iLook1=list1[n1++];
	    iLook2=list2[n2++];
	  } else if (iLook1<iLook2) {
	    printf(" (r %d)",iLook1);
	    iLook1=list1[n1++];
	  } else {
	    printf(" (c %d)",iLook2);
	    iLook2=list2[n2++];
	  }
	  xx++;
	  if (xx==10) {
	    printf("\n");
	    xx=0;
	  }
	}
	if (xx)
	  printf("\n");
	nBad++;
      }
      nTotal1+=counts[i];
    }
    CoinAbcMemset0(counts,numberRows_);
    int nTotal2=0;
    for (int iRow=0;iRow<numberRows_;iRow++) {
      for (CoinBigIndex j=startRow[iRow];j<startRow[iRow]+numberInRow[iRow];j++) {
	int iColumn=indexColumn[j];
	counts[iColumn]++;
      }
    }
    for (int i=0;i<numberRows_;i++) {
      if (numberInColumn[i]!=counts[i]) {
	printf("Column %d numberInColumn %d counts %d\n",i,numberInColumn[i],counts[i]);
	int n1=0;
	int n2=0;
	for (int j=startColumn[i];j<startColumn[i]+numberInColumn[i];j++)
	  list1[n1++]=indexRow[j];
	for (int iRow=0;iRow<numberRows_;iRow++) {
	  for (CoinBigIndex j=startRow[iRow];j<startRow[iRow]+numberInRow[iRow];j++) {
	    int iColumn=indexColumn[j];
	    if (i==iColumn)
	      list2[n2++]=iRow;
	  }
	}
	std::sort(list1,list1+n1);
	std::sort(list2,list2+n2);
	list1[n1]=numberRows_;
	list2[n2]=numberRows_;
	n1=1;
	n2=1;
	int iLook1=list1[0];
	int iLook2=list2[0];
	int xx=0;
	while(iLook1!=numberRows_||iLook2!=numberRows_) {
	  if (iLook1==iLook2) {
	    printf(" (rc %d)",iLook1);
	    iLook1=list1[n1++];
	    iLook2=list2[n2++];
	  } else if (iLook1<iLook2) {
	    printf(" (c %d)",iLook1);
	    iLook1=list1[n1++];
	  } else {
	    printf(" (r %d)",iLook2);
	    iLook2=list2[n2++];
	  }
	  xx++;
	  if (xx==10) {
	    printf("\n");
	    xx=0;
	  }
	}
	if (xx)
	  printf("\n");
	nBad++;
      }
      nTotal2+=counts[i];
    }
    delete [] counts;
    assert (!nBad);
  }
  char * mark = new char [2*numberRows_];
  memset(mark,0,2*numberRows_);
  for (int iCount=0;iCount<numberRowsSmall_+1;iCount++) {
    int next=firstCount[iCount];
#define VERBOSE 0
#if VERBOSE
    int count=0;
#endif
    while (next>=0) {
#if VERBOSE
      count++;
      if (iCount<=VERBOSE) {
	if (next<numberRows_)
#ifdef SMALL_PERMUTE
	  printf("R%d\n",fromSmallToBigRow_[next]);
#else
	  printf("R%d\n",next);
#endif
	else
	  printf("C%d\n",next-numberRows_);
      }
#endif
      assert (!mark[next]);
      mark[next]=1;
      if (next<numberRows_)
	assert (numberInRow[next]==iCount);
      else
	assert (numberInColumn[next-numberRows_]==iCount);
      int next2=next;
      assert (next!=nextCount[next]);
      next=nextCount[next];
      if (next>=0)
	assert (next2==lastCount[next]);
    }
#if VERBOSE
    if (count)
      printf("%d items have count %d\n",count,iCount); 
#endif
  }
  delete [] mark;
}
#endif
#endif
