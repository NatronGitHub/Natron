/* $Id: CoinFactorization1.cpp 1581 2013-04-06 12:48:50Z stefan $ */
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#if defined(_MSC_VER)
// Turn off compiler warning about long names
#  pragma warning(disable:4786)
#endif

#include "CoinUtilsConfig.h"

#include <cassert>
#include "CoinFactorization.hpp"
#include "CoinIndexedVector.hpp"
#include "CoinHelperFunctions.hpp"
#include "CoinPackedMatrix.hpp"
#include "CoinFinite.hpp"
#include <stdio.h>
//:class CoinFactorization.  Deals with Factorization and Updates
//  CoinFactorization.  Constructor
CoinFactorization::CoinFactorization (  )
{
  persistenceFlag_=0;
  gutsOfInitialize(7);
}

/// Copy constructor 
CoinFactorization::CoinFactorization ( const CoinFactorization &other)
{
  persistenceFlag_=0;
  gutsOfInitialize(3);
  persistenceFlag_=other.persistenceFlag_;
  gutsOfCopy(other);
}
/// The real work of constructors etc
/// Really really delete if type 2
void CoinFactorization::gutsOfDestructor(int type)
{
  delete [] denseArea_;
  delete [] densePermute_;
  if (type==2) {
    elementU_.switchOff();
    startRowU_.switchOff();
    convertRowToColumnU_.switchOff();
    indexRowU_.switchOff();
    indexColumnU_.switchOff();
    startColumnU_.switchOff();
    elementL_.switchOff();
    indexRowL_.switchOff();
    startColumnL_.switchOff();
    startColumnR_.switchOff();
    numberInRow_.switchOff();
    numberInColumn_.switchOff();
    numberInColumnPlus_.switchOff();
    pivotColumn_.switchOff();
    pivotColumnBack_.switchOff();
    firstCount_.switchOff();
    nextCount_.switchOff();
    lastCount_.switchOff();
    permute_.switchOff();
    permuteBack_.switchOff();
    nextColumn_.switchOff();
    lastColumn_.switchOff();
    nextRow_.switchOff();
    lastRow_.switchOff();
    saveColumn_.switchOff();
    markRow_.switchOff();
    pivotRowL_.switchOff();
    pivotRegion_.switchOff();
    elementByRowL_.switchOff();
    startRowL_.switchOff();
    indexColumnL_.switchOff();
    sparse_.switchOff();
    workArea_.switchOff();
    workArea2_.switchOff();
  }
  elementU_.conditionalDelete();
  startRowU_.conditionalDelete();
  convertRowToColumnU_.conditionalDelete();
  indexRowU_.conditionalDelete();
  indexColumnU_.conditionalDelete();
  startColumnU_.conditionalDelete();
  elementL_.conditionalDelete();
  indexRowL_.conditionalDelete();
  startColumnL_.conditionalDelete();
  startColumnR_.conditionalDelete();
  numberInRow_.conditionalDelete();
  numberInColumn_.conditionalDelete();
  numberInColumnPlus_.conditionalDelete();
  pivotColumn_.conditionalDelete();
  pivotColumnBack_.conditionalDelete();
  firstCount_.conditionalDelete();
  nextCount_.conditionalDelete();
  lastCount_.conditionalDelete();
  permute_.conditionalDelete();
  permuteBack_.conditionalDelete();
  nextColumn_.conditionalDelete();
  lastColumn_.conditionalDelete();
  nextRow_.conditionalDelete();
  lastRow_.conditionalDelete();
  saveColumn_.conditionalDelete();
  markRow_.conditionalDelete();
  pivotRowL_.conditionalDelete();
  pivotRegion_.conditionalDelete();
  elementByRowL_.conditionalDelete();
  startRowL_.conditionalDelete();
  indexColumnL_.conditionalDelete();
  sparse_.conditionalDelete();
  workArea_.conditionalDelete();
  workArea2_.conditionalDelete();
  numberCompressions_ = 0;
  biggerDimension_ = 0;
  numberRows_ = 0;
  numberRowsExtra_ = 0;
  maximumRowsExtra_ = 0;
  numberColumns_ = 0;
  numberColumnsExtra_ = 0;
  maximumColumnsExtra_ = 0;
  numberGoodU_ = 0;
  numberGoodL_ = 0;
  totalElements_ = 0;
  factorElements_ = 0;
  status_ = -1;
  numberSlacks_ = 0;
  numberU_ = 0;
  maximumU_=0;
  lengthU_ = 0;
  lengthAreaU_ = 0;
  numberL_ = 0;
  baseL_ = 0;
  lengthL_ = 0;
  lengthAreaL_ = 0;
  numberR_ = 0;
  lengthR_ = 0;
  lengthAreaR_ = 0;
  denseArea_=NULL;
  densePermute_=NULL;
  // next two offsets but this makes cleaner
  elementR_=NULL;
  indexRowR_=NULL;
  numberDense_=0;
  //persistenceFlag_=0;
  ////denseThreshold_=0;
  
}
// type - 1 bit tolerances etc, 2 rest
void CoinFactorization::gutsOfInitialize(int type)
{
  if ((type&2)!=0) {
    numberCompressions_ = 0;
    biggerDimension_ = 0;
    numberRows_ = 0;
    numberRowsExtra_ = 0;
    maximumRowsExtra_ = 0;
    numberColumns_ = 0;
    numberColumnsExtra_ = 0;
    maximumColumnsExtra_ = 0;
    numberGoodU_ = 0;
    numberGoodL_ = 0;
    totalElements_ = 0;
    factorElements_ = 0;
    status_ = -1;
    numberPivots_ = 0;
    numberSlacks_ = 0;
    numberU_ = 0;
    maximumU_=0;
    lengthU_ = 0;
    lengthAreaU_ = 0;
    numberL_ = 0;
    baseL_ = 0;
    lengthL_ = 0;
    lengthAreaL_ = 0;
    numberR_ = 0;
    lengthR_ = 0;
    lengthAreaR_ = 0;
    elementR_ = NULL;
    indexRowR_ = NULL;
    // always switch off sparse
    sparseThreshold_=0;
    sparseThreshold2_= 0;
    denseArea_ = NULL;
    densePermute_=NULL;
    numberDense_=0;
    if (!persistenceFlag_) {
      workArea_=CoinFactorizationDoubleArrayWithLength();
      workArea2_=CoinUnsignedIntArrayWithLength();
      pivotColumn_=CoinIntArrayWithLength();
    }
  }
  // after 2 because of persistenceFlag_
  if ((type&1)!=0) {
    areaFactor_ = 0.0;
    pivotTolerance_ = 1.0e-1;
    zeroTolerance_ = 1.0e-13;
#ifndef COIN_FAST_CODE
    slackValue_ = -1.0;
#endif
    messageLevel_=0;
    maximumPivots_=200;
    numberTrials_ = 4;
    relaxCheck_=1.0;
#if DENSE_CODE==1
    denseThreshold_=31;
    denseThreshold_=71;
#else
    denseThreshold_=0;
#endif
    biasLU_=2;
    doForrestTomlin_=true;
    persistenceFlag_=0;
  }
  if ((type&4)!=0) {
    // we need to get 1 element arrays for any with length n+1 !!
    startColumnL_.conditionalNew (1);
    startColumnR_.conditionalNew( 1 );
    startRowU_.conditionalNew(1);
    numberInRow_.conditionalNew(1);
    nextRow_.conditionalNew(1);
    lastRow_.conditionalNew( 1 );
    pivotRegion_.conditionalNew(1);
    permuteBack_.conditionalNew(1);
    permute_.conditionalNew(1);
    pivotColumnBack_.conditionalNew(1);
    startColumnU_.conditionalNew( 1 );
    numberInColumn_.conditionalNew(1);
    numberInColumnPlus_.conditionalNew(1);
    pivotColumn_.conditionalNew(1);
    nextColumn_.conditionalNew(1);
    lastColumn_.conditionalNew(1);
    collectStatistics_=false;
    
    // Below are all to collect
    ftranCountInput_=0.0;
    ftranCountAfterL_=0.0;
    ftranCountAfterR_=0.0;
    ftranCountAfterU_=0.0;
    btranCountInput_=0.0;
    btranCountAfterU_=0.0;
    btranCountAfterR_=0.0;
    btranCountAfterL_=0.0;
    
    // We can roll over factorizations
    numberFtranCounts_=0;
    numberBtranCounts_=0;
    
    // While these are averages collected over last 
    ftranAverageAfterL_=0;
    ftranAverageAfterR_=0;
    ftranAverageAfterU_=0;
    btranAverageAfterU_=0;
    btranAverageAfterR_=0;
    btranAverageAfterL_=0; 
#ifdef ZEROFAULT
    startColumnL_.array()[0] = 0;
    startColumnR_.array()[0] = 0;
    startRowU_.array()[0] = 0;
    numberInRow_.array()[0] = 0;
    nextRow_.array()[0] = 0;
    lastRow_.array()[0] = 0;
    pivotRegion_.array()[0] = 0.0;
    permuteBack_.array()[0] = 0;
    permute_.array()[0] = 0;
    pivotColumnBack_.array()[0] = 0;
    startColumnU_.array()[0] = 0;
    numberInColumn_.array()[0] = 0;
    numberInColumnPlus_.array()[0] = 0;
    pivotColumn_.array()[0] = 0;
    nextColumn_.array()[0] = 0;
    lastColumn_.array()[0] = 0;
#endif
  }
}
//Part of LP
int CoinFactorization::factorize (
				 const CoinPackedMatrix & matrix,
				 int rowIsBasic[],
				 int columnIsBasic[],
				 double areaFactor )
{
  // maybe for speed will be better to leave as many regions as possible
  gutsOfDestructor();
  gutsOfInitialize(2);
  // ? is this correct
  //if (biasLU_==2)
  //biasLU_=3;
  if (areaFactor)
    areaFactor_ = areaFactor;
  const int * row = matrix.getIndices();
  const CoinBigIndex * columnStart = matrix.getVectorStarts();
  const int * columnLength = matrix.getVectorLengths(); 
  const double * element = matrix.getElements();
  int numberRows=matrix.getNumRows();
  int numberColumns=matrix.getNumCols();
  int numberBasic = 0;
  CoinBigIndex numberElements=0;
  int numberRowBasic=0;

  // compute how much in basis

  int i;

  for (i=0;i<numberRows;i++) {
    if (rowIsBasic[i]>=0)
      numberRowBasic++;
  }

  numberBasic = numberRowBasic;

  for (i=0;i<numberColumns;i++) {
    if (columnIsBasic[i]>=0) {
      numberBasic++;
      numberElements += columnLength[i];
    }
  }
  if ( numberBasic > numberRows ) {
    return -2; // say too many in basis
  }
  numberElements = 3 * numberBasic + 3 * numberElements + 20000;
  getAreas ( numberRows, numberBasic, numberElements,
	     2 * numberElements );
  //fill
  //copy
  numberBasic=0;
  numberElements=0;
  int * indexColumnU = indexColumnU_.array();
  int * indexRowU = indexRowU_.array();
  CoinFactorizationDouble * elementU = elementU_.array();
  for (i=0;i<numberRows;i++) {
    if (rowIsBasic[i]>=0) {
      indexRowU[numberElements]=i;
      indexColumnU[numberElements]=numberBasic;
      elementU[numberElements++]=slackValue_;
      numberBasic++;
    }
  }
  for (i=0;i<numberColumns;i++) {
    if (columnIsBasic[i]>=0) {
      CoinBigIndex j;
      for (j=columnStart[i];j<columnStart[i]+columnLength[i];j++) {
	indexRowU[numberElements]=row[j];
	indexColumnU[numberElements]=numberBasic;
	elementU[numberElements++]=element[j];
      }
      numberBasic++;
    }
  }
  lengthU_ = numberElements;
  maximumU_ = numberElements;

  preProcess ( 0 );
  factor (  );
  numberBasic=0;
  if (status_ == 0) {
    int * permuteBack = permuteBack_.array();
    int * back = pivotColumnBack();
    for (i=0;i<numberRows;i++) {
      if (rowIsBasic[i]>=0) {
	rowIsBasic[i]=permuteBack[back[numberBasic++]];
      }
    }
    for (i=0;i<numberColumns;i++) {
      if (columnIsBasic[i]>=0) {
	columnIsBasic[i]=permuteBack[back[numberBasic++]];
      }
    }
    // Set up permutation vector
    // these arrays start off as copies of permute
    // (and we could use permute_ instead of pivotColumn (not back though))
    CoinMemcpyN ( permute_.array(), numberRows_ , pivotColumn_.array()  );
    CoinMemcpyN ( permuteBack_.array(), numberRows_ , pivotColumnBack()  );
  } else if (status_ == -1) {
    const int * pivotColumn = pivotColumn_.array();
    // mark as basic or non basic
    for (i=0;i<numberRows_;i++) {
      if (rowIsBasic[i]>=0) {
	if (pivotColumn[numberBasic]>=0) 
	  rowIsBasic[i]=pivotColumn[numberBasic];
	else 
	  rowIsBasic[i]=-1;
	numberBasic++;
      }
    }
    for (i=0;i<numberColumns;i++) {
      if (columnIsBasic[i]>=0) {
	if (pivotColumn[numberBasic]>=0) 
	  columnIsBasic[i]=pivotColumn[numberBasic];
	 else 
	  columnIsBasic[i]=-1;
	numberBasic++;
      }
    }
  }

  return status_;
}
//Given as triplets
int CoinFactorization::factorize (
			     int numberOfRows,
			     int numberOfColumns,
			     CoinBigIndex numberOfElements,
			     CoinBigIndex maximumL,
			     CoinBigIndex maximumU,
			     const int indicesRow[],
			     const int indicesColumn[],
			     const double elements[] ,
			     int permutation[],
			     double areaFactor)
{
  gutsOfDestructor();
  gutsOfInitialize(2);
  if (areaFactor)
    areaFactor_ = areaFactor;
  getAreas ( numberOfRows, numberOfColumns, maximumL, maximumU );
  //copy
  CoinMemcpyN ( indicesRow, numberOfElements, indexRowU_.array() );
  CoinMemcpyN ( indicesColumn, numberOfElements, indexColumnU_.array() );
  int i;
  CoinFactorizationDouble * elementU = elementU_.array();
  for (i=0;i<numberOfElements;i++)
    elementU[i] = elements[i];
  lengthU_ = numberOfElements;
  maximumU_ = numberOfElements;
  preProcess ( 0 );
  factor (  );
  //say which column is pivoting on which row
  if (status_ == 0) {
    int * permuteBack = permuteBack_.array();
    int * back = pivotColumnBack();
    // permute so slacks on own rows etc
    for (i=0;i<numberOfColumns;i++) {
      permutation[i]=permuteBack[back[i]];
    }
    // Set up permutation vector
    // these arrays start off as copies of permute
    // (and we could use permute_ instead of pivotColumn (not back though))
    CoinMemcpyN ( permute_.array(), numberRows_ , pivotColumn_.array()  );
    CoinMemcpyN ( permuteBack_.array(), numberRows_ , pivotColumnBack()  );
  } else if (status_ == -1) {
    const int * pivotColumn = pivotColumn_.array();
    // mark as basic or non basic
    for (i=0;i<numberOfColumns;i++) {
      if (pivotColumn[i]>=0) {
	permutation[i]=pivotColumn[i];
      } else {
	permutation[i]=-1;
      }
    }
  }

  return status_;
}
/* Two part version for flexibility
   This part creates arrays for user to fill.
   maximumL is guessed maximum size of L part of
   final factorization, maximumU of U part.  These are multiplied by
   areaFactor which can be computed by user or internally.  
   returns 0 -okay, -99 memory */
int 
CoinFactorization::factorizePart1 ( int numberOfRows,
				   int ,
				   CoinBigIndex numberOfElements,
				   int * indicesRow[],
				   int * indicesColumn[],
				   CoinFactorizationDouble * elements[],
				   double areaFactor)
{
  // maybe for speed will be better to leave as many regions as possible
  gutsOfDestructor();
  gutsOfInitialize(2);
  if (areaFactor)
    areaFactor_ = areaFactor;
  CoinBigIndex numberElements = 3 * numberOfRows + 3 * numberOfElements + 20000;
  getAreas ( numberOfRows, numberOfRows, numberElements,
	     2 * numberElements );
  // need to trap memory for -99 code
  *indicesRow = indexRowU_.array() ;
  *indicesColumn = indexColumnU_.array() ;
  *elements = elementU_.array() ;
  lengthU_ = numberOfElements;
  maximumU_ = numberElements;
  return 0;
}
/* This is part two of factorization
   Arrays belong to factorization and were returned by part 1
   If status okay, permutation has pivot rows.
   If status is singular, then basic variables have +1 and ones thrown out have -COIN_INT_MAX
   to say thrown out.
   returns 0 -okay, -1 singular, -99 memory */
int 
CoinFactorization::factorizePart2 (int permutation[],int exactNumberElements)
{
  lengthU_ = exactNumberElements;
  preProcess ( 0 );
  factor (  );
  //say which column is pivoting on which row
  int i;
  int * permuteBack = permuteBack_.array();
  int * back = pivotColumnBack();
  // permute so slacks on own rows etc
  for (i=0;i<numberColumns_;i++) {
    permutation[i]=permuteBack[back[i]];
  }
  if (status_ == 0) {
    // Set up permutation vector
    // these arrays start off as copies of permute
    // (and we could use permute_ instead of pivotColumn (not back though))
    CoinMemcpyN ( permute_.array(), numberRows_ , pivotColumn_.array()  );
    CoinMemcpyN ( permuteBack_.array(), numberRows_ , pivotColumnBack()  );
  } else if (status_ == -1) {
    const int * pivotColumn = pivotColumn_.array();
    // mark as basic or non basic
    for (i=0;i<numberColumns_;i++) {
      if (pivotColumn[i]>=0) {
	permutation[i]=pivotColumn[i];
      } else {
	permutation[i]=-1;
      }
    }
  }

  return status_;
}

//  ~CoinFactorization.  Destructor
CoinFactorization::~CoinFactorization (  )
{
  gutsOfDestructor(2);
}

//  show_self.  Debug show object
void
CoinFactorization::show_self (  ) const
{
  int i;

  const int * pivotColumn = pivotColumn_.array();
  for ( i = 0; i < numberRows_; i++ ) {
    std::cout << "r " << i << " " << pivotColumn[i];
    if (pivotColumnBack()) std::cout<< " " << pivotColumnBack()[i];
    std::cout<< " " << permute_.array()[i];
    if (permuteBack_.array()) std::cout<< " " << permuteBack_.array()[i];
    std::cout<< " " << pivotRegion_.array()[i];
    std::cout << std::endl;
  }
  for ( i = 0; i < numberRows_; i++ ) {
    std::cout << "u " << i << " " << numberInColumn_.array()[i] << std::endl;
    int j;
    CoinSort_2(indexRowU_.array()+startColumnU_.array()[i],
	       indexRowU_.array()+startColumnU_.array()[i]+numberInColumn_.array()[i],
	       elementU_.array()+startColumnU_.array()[i]);
    for ( j = startColumnU_.array()[i]; j < startColumnU_.array()[i] + numberInColumn_.array()[i];
	  j++ ) {
      assert (indexRowU_.array()[j]>=0&&indexRowU_.array()[j]<numberRows_);
      assert (elementU_.array()[j]>-1.0e50&&elementU_.array()[j]<1.0e50);
      std::cout << indexRowU_.array()[j] << " " << elementU_.array()[j] << std::endl;
    }
  }
  for ( i = 0; i < numberRows_; i++ ) {
    std::cout << "l " << i << " " << startColumnL_.array()[i + 1] -
      startColumnL_.array()[i] << std::endl;
    CoinSort_2(indexRowL_.array()+startColumnL_.array()[i],
	       indexRowL_.array()+startColumnL_.array()[i+1],
	       elementL_.array()+startColumnL_.array()[i]);
    int j;
    for ( j = startColumnL_.array()[i]; j < startColumnL_.array()[i + 1]; j++ ) {
      std::cout << indexRowL_.array()[j] << " " << elementL_.array()[j] << std::endl;
    }
  }

}
//  sort so can compare
void
CoinFactorization::sort (  ) const
{
  int i;

  for ( i = 0; i < numberRows_; i++ ) {
    CoinSort_2(indexRowU_.array()+startColumnU_.array()[i],
	       indexRowU_.array()+startColumnU_.array()[i]+numberInColumn_.array()[i],
	       elementU_.array()+startColumnU_.array()[i]);
  }
  for ( i = 0; i < numberRows_; i++ ) {
    CoinSort_2(indexRowL_.array()+startColumnL_.array()[i],
	       indexRowL_.array()+startColumnL_.array()[i+1],
	       elementL_.array()+startColumnL_.array()[i]);
  }

}

//  getAreas.  Gets space for a factorization
//called by constructors
void
CoinFactorization::getAreas ( int numberOfRows,
			 int numberOfColumns,
			 CoinBigIndex maximumL,
			 CoinBigIndex maximumU )
{
  
  numberRows_ = numberOfRows;
  numberColumns_ = numberOfColumns;
  maximumRowsExtra_ = numberRows_ + maximumPivots_;
  numberRowsExtra_ = numberRows_;
  maximumColumnsExtra_ = numberColumns_ + maximumPivots_;
  numberColumnsExtra_ = numberColumns_;
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
  elementU_.conditionalNew( lengthAreaU_ );
  indexRowU_.conditionalNew( lengthAreaU_ );
  indexColumnU_.conditionalNew( lengthAreaU_ );
  elementL_.conditionalNew( lengthAreaL_ );
  indexRowL_.conditionalNew( lengthAreaL_ );
  if (persistenceFlag_) {
    // But we can use all we have if bigger
    int length;
    length = CoinMin(elementU_.getSize(),indexRowU_.getSize());
    if (length>lengthAreaU_) {
      lengthAreaU_=length;
      assert (indexColumnU_.getSize()==indexRowU_.getSize());
    }
    length = CoinMin(elementL_.getSize(),indexRowL_.getSize());
    if (length>lengthAreaL_) {
      lengthAreaL_=length;
    }
  }
  startColumnL_.conditionalNew( numberRows_ + 1 );
  startColumnL_.array()[0] = 0;
  startRowU_.conditionalNew( maximumRowsExtra_ + 1);
  // make sure this is valid
  startRowU_.array()[maximumRowsExtra_]=0;
  numberInRow_.conditionalNew( maximumRowsExtra_ + 1 );
  markRow_.conditionalNew( numberRows_ );
  pivotRowL_.conditionalNew( numberRows_ + 1 );
  nextRow_.conditionalNew( maximumRowsExtra_ + 1 );
  lastRow_.conditionalNew( maximumRowsExtra_ + 1 );
  permute_.conditionalNew( maximumRowsExtra_ + 1 );
  pivotRegion_.conditionalNew( maximumRowsExtra_ + 1 );
#ifdef ZEROFAULT
  memset(elementU_.array(),'a',lengthAreaU_*sizeof(CoinFactorizationDouble));
  memset(indexRowU_.array(),'b',lengthAreaU_*sizeof(int));
  memset(indexColumnU_.array(),'c',lengthAreaU_*sizeof(int));
  memset(elementL_.array(),'d',lengthAreaL_*sizeof(CoinFactorizationDouble));
  memset(indexRowL_.array(),'e',lengthAreaL_*sizeof(int));
  memset(startColumnL_.array()+1,'f',numberRows_*sizeof(CoinBigIndex));
  memset(startRowU_.array(),'g',maximumRowsExtra_*sizeof(CoinBigIndex));
  memset(numberInRow_.array(),'h',(maximumRowsExtra_+1)*sizeof(int));
  memset(markRow_.array(),'i',numberRows_*sizeof(int));
  memset(pivotRowL_.array(),'j',(numberRows_+1)*sizeof(int));
  memset(nextRow_.array(),'k',(maximumRowsExtra_+1)*sizeof(int));
  memset(lastRow_.array(),'l',(maximumRowsExtra_+1)*sizeof(int));
  memset(permute_.array(),'l',(maximumRowsExtra_+1)*sizeof(int));
  memset(pivotRegion_.array(),'m',(maximumRowsExtra_+1)*sizeof(CoinFactorizationDouble));
#endif
  startColumnU_.conditionalNew( maximumColumnsExtra_ + 1 );
  numberInColumn_.conditionalNew( maximumColumnsExtra_ + 1 );
  numberInColumnPlus_.conditionalNew( maximumColumnsExtra_ + 1 );
  pivotColumn_.conditionalNew( maximumColumnsExtra_ + 1 );
  nextColumn_.conditionalNew( maximumColumnsExtra_ + 1 );
  lastColumn_.conditionalNew( maximumColumnsExtra_ + 1 );
  saveColumn_.conditionalNew( numberColumns_);
#ifdef ZEROFAULT
  memset(startColumnU_.array(),'a',(maximumColumnsExtra_+1)*sizeof(CoinBigIndex));
  memset(numberInColumn_.array(),'b',(maximumColumnsExtra_+1)*sizeof(int));
  memset(numberInColumnPlus_.array(),'c',(maximumColumnsExtra_+1)*sizeof(int));
  memset(pivotColumn_.array(),'d',(maximumColumnsExtra_+1)*sizeof(int));
  memset(nextColumn_.array(),'e',(maximumColumnsExtra_+1)*sizeof(int));
  memset(lastColumn_.array(),'f',(maximumColumnsExtra_+1)*sizeof(int));
#endif
  if ( numberRows_ + numberColumns_ ) {
    if ( numberRows_ > numberColumns_ ) {
      biggerDimension_ = numberRows_;
    } else {
      biggerDimension_ = numberColumns_;
    }
    firstCount_.conditionalNew( CoinMax(biggerDimension_ + 2, maximumRowsExtra_+1) );
    nextCount_.conditionalNew( numberRows_ + numberColumns_ );
    lastCount_.conditionalNew( numberRows_ + numberColumns_ );
#ifdef ZEROFAULT
    memset(firstCount_.array(),'g',(biggerDimension_ + 2 )*sizeof(int));
    memset(nextCount_.array(),'h',(numberRows_+numberColumns_)*sizeof(int));
    memset(lastCount_.array(),'i',(numberRows_+numberColumns_)*sizeof(int));
#endif
  } else {
    firstCount_.conditionalNew( 2 );
    nextCount_.conditionalNew( 0 );
    lastCount_.conditionalNew( 0 );
#ifdef ZEROFAULT
    memset(firstCount_.array(),'g', 2 *sizeof(int));
#endif
    biggerDimension_ = 0;
  }
}

//  preProcess.  PreProcesses raw triplet data
//state is 0 - triplets, 1 - some counts etc , 2 - ..
void
CoinFactorization::preProcess ( int state,
			   int )
{
  int *indexRow = indexRowU_.array();
  int *indexColumn = indexColumnU_.array();
  CoinFactorizationDouble *element = elementU_.array();
  CoinBigIndex numberElements = lengthU_;
  int *numberInRow = numberInRow_.array();
  int *numberInColumn = numberInColumn_.array();
  int *numberInColumnPlus = numberInColumnPlus_.array();
  CoinBigIndex *startRow = startRowU_.array();
  CoinBigIndex *startColumn = startColumnU_.array();
  int numberRows = numberRows_;
  int numberColumns = numberColumns_;

  if (state<4)
    totalElements_ = numberElements;
  //state falls through to next state
  switch ( state ) {
  case 0:			//counts
    {
      CoinZeroN ( numberInRow, numberRows + 1 );
      CoinZeroN ( numberInColumn, maximumColumnsExtra_ + 1 );
      CoinBigIndex i;
      for ( i = 0; i < numberElements; i++ ) {
	int iRow = indexRow[i];
	int iColumn = indexColumn[i];

	numberInRow[iRow]++;
	numberInColumn[iColumn]++;
      }
    }
  case -1:			//sort
  case 1:			//sort
    {
      CoinBigIndex i, k;

      i = 0;
      int iColumn;
      for ( iColumn = 0; iColumn < numberColumns; iColumn++ ) {
	//position after end of Column
	i += numberInColumn[iColumn];
	startColumn[iColumn] = i;
      }
      for ( k = numberElements - 1; k >= 0; k-- ) {
	int iColumn = indexColumn[k];

	if ( iColumn >= 0 ) {
	  CoinFactorizationDouble value = element[k];
	  int iRow = indexRow[k];

	  indexColumn[k] = -1;
	  while ( true ) {
	    CoinBigIndex iLook = startColumn[iColumn] - 1;

	    startColumn[iColumn] = iLook;
	    CoinFactorizationDouble valueSave = element[iLook];
	    int iColumnSave = indexColumn[iLook];
	    int iRowSave = indexRow[iLook];

	    element[iLook] = value;
	    indexRow[iLook] = iRow;
	    indexColumn[iLook] = -1;
	    if ( iColumnSave >= 0 ) {
	      iColumn = iColumnSave;
	      value = valueSave;
	      iRow = iRowSave;
	    } else {
	      break;
	    }
	  }			/* endwhile */
	}
      }
    }
  case 2:			//move largest in column to beginning
    //and do row part
    {
      CoinBigIndex i, k;

      i = 0;
      int iRow;
      for ( iRow = 0; iRow < numberRows; iRow++ ) {
	startRow[iRow] = i;
	i += numberInRow[iRow];
      }
      CoinZeroN ( numberInRow, numberRows );
      int iColumn;
      for ( iColumn = 0; iColumn < numberColumns; iColumn++ ) {
	int number = numberInColumn[iColumn];

	if ( number ) {
	  CoinBigIndex first = startColumn[iColumn];
	  CoinBigIndex largest = first;
	  int iRowSave = indexRow[first];
	  CoinFactorizationDouble valueSave = element[first];
	  double valueLargest = fabs ( valueSave );
	  int iLook = numberInRow[iRowSave];

	  numberInRow[iRowSave] = iLook + 1;
	  indexColumn[startRow[iRowSave] + iLook] = iColumn;
	  for ( k = first + 1; k < first + number; k++ ) {
	    int iRow = indexRow[k];
	    int iLook = numberInRow[iRow];

	    numberInRow[iRow] = iLook + 1;
	    indexColumn[startRow[iRow] + iLook] = iColumn;
	    CoinFactorizationDouble value = element[k];
	    double valueAbs = fabs ( value );

	    if ( valueAbs > valueLargest ) {
	      valueLargest = valueAbs;
	      largest = k;
	    }
	  }
	  indexRow[first] = indexRow[largest];
	  element[first] = element[largest];
	  indexRow[largest] = iRowSave;
	  element[largest] = valueSave;
	}
      }
    }
  case 3:			//links and initialize pivots
    {
      int *lastRow = lastRow_.array();
      int *nextRow = nextRow_.array();
      int *lastColumn = lastColumn_.array();
      int *nextColumn = nextColumn_.array();

      CoinFillN ( firstCount_.array(), biggerDimension_ + 2, -1 );
      CoinFillN ( pivotColumn_.array(), numberColumns_, -1 );
      CoinZeroN ( numberInColumnPlus, maximumColumnsExtra_ + 1 );
      int iRow;
      for ( iRow = 0; iRow < numberRows; iRow++ ) {
	lastRow[iRow] = iRow - 1;
	nextRow[iRow] = iRow + 1;
	int number = numberInRow[iRow];

	addLink ( iRow, number );
      }
      lastRow[maximumRowsExtra_] = numberRows - 1;
      nextRow[maximumRowsExtra_] = 0;
      lastRow[0] = maximumRowsExtra_;
      nextRow[numberRows - 1] = maximumRowsExtra_;
      startRow[maximumRowsExtra_] = numberElements;
      int iColumn;
      for ( iColumn = 0; iColumn < numberColumns; iColumn++ ) {
	lastColumn[iColumn] = iColumn - 1;
	nextColumn[iColumn] = iColumn + 1;
	int number = numberInColumn[iColumn];

	addLink ( iColumn + numberRows, number );
      }
      lastColumn[maximumColumnsExtra_] = numberColumns - 1;
      nextColumn[maximumColumnsExtra_] = 0;
      lastColumn[0] = maximumColumnsExtra_;
      if (numberColumns)
	nextColumn[numberColumns - 1] = maximumColumnsExtra_;
      startColumn[maximumColumnsExtra_] = numberElements;
    }
    break;
  case 4:			//move largest in column to beginning
    {
      CoinBigIndex i, k;
      CoinFactorizationDouble * pivotRegion = pivotRegion_.array();
      int iColumn;
      int iRow;
      for ( iRow = 0; iRow < numberRows; iRow++ ) {
	if( numberInRow[iRow]>=0) {
	  // zero count
	  numberInRow[iRow]=0;
	} else {
	  // empty
	  //numberInRow[iRow]=-1; already that
	}
      }
      //CoinZeroN ( numberInColumnPlus, maximumColumnsExtra_ + 1 );
      for ( iColumn = 0; iColumn < numberColumns; iColumn++ ) {
	int number = numberInColumn[iColumn];

	if ( number ) {
	  // use pivotRegion and startRow for remaining elements
	  CoinBigIndex first = startColumn[iColumn];
	  CoinBigIndex largest = -1;
	  
	  double valueLargest = -1.0;
	  int nOther=0;
	  k = first;
	  CoinBigIndex end = first+number;
	  for (  ; k < end; k++ ) {
	    int iRow = indexRow[k];
	    assert (iRow<numberRows_);
	    CoinFactorizationDouble value = element[k];
	    if (numberInRow[iRow]>=0) {
	      numberInRow[iRow]++;
	      double valueAbs = fabs ( value );
	      if ( valueAbs > valueLargest ) {
		valueLargest = valueAbs;
		largest = nOther;
	      }
	      startRow[nOther]=iRow;
	      pivotRegion[nOther++]=value;
	    } else {
	      indexRow[first] = iRow;
	      element[first++] = value;
	    }
	  }
	  numberInColumnPlus[iColumn]=first-startColumn[iColumn];
	  startColumn[iColumn]=first;
	  //largest
	  if (largest>=0) {
	    indexRow[first] = startRow[largest];
	    element[first++] = pivotRegion[largest];
	  }
	  for (k=0;k<nOther;k++) {
	    if (k!=largest) {
	      indexRow[first] = startRow[k];
	      element[first++] = pivotRegion[k];
	    }
	  }
	  numberInColumn[iColumn]=first-startColumn[iColumn];
	}
      }
      //and do row part
      i = 0;
      for ( iRow = 0; iRow < numberRows; iRow++ ) {
	startRow[iRow] = i;
	int n=numberInRow[iRow];
	if (n>0) {
	  numberInRow[iRow]=0;
	  i += n;
	}
      }
      for ( iColumn = 0; iColumn < numberColumns; iColumn++ ) {
	int number = numberInColumn[iColumn];

	if ( number ) {
	  CoinBigIndex first = startColumn[iColumn];
	  for ( k = first; k < first + number; k++ ) {
	    int iRow = indexRow[k];
	    int iLook = numberInRow[iRow];

	    numberInRow[iRow] = iLook + 1;
	    indexColumn[startRow[iRow] + iLook] = iColumn;
	  }
	}
      }
    }
    // modified 3
    {
      //set markRow so no rows updated
      //CoinFillN ( markRow_.array(), numberRows_, -1 );
      int *lastColumn = lastColumn_.array();
      int *nextColumn = nextColumn_.array();
      CoinFactorizationDouble * pivotRegion = pivotRegion_.array();

      int iRow;
      int numberGood=0;
      startColumnL_.array()[0] = 0;	//for luck
      for ( iRow = 0; iRow < numberRows; iRow++ ) {
	int number = numberInRow[iRow];
	if (number<0) {
	  numberInRow[iRow]=0;
	  pivotRegion[numberGood++]=slackValue_;
	}
      }
      int iColumn;
      for ( iColumn = 0 ; iColumn < numberColumns_; iColumn++ ) {
	lastColumn[iColumn] = iColumn - 1;
	nextColumn[iColumn] = iColumn + 1;
	int number = numberInColumn[iColumn];
	deleteLink(iColumn+numberRows);
	addLink ( iColumn + numberRows, number );
      }
      lastColumn[maximumColumnsExtra_] = numberColumns - 1;
      nextColumn[maximumColumnsExtra_] = 0;
      lastColumn[0] = maximumColumnsExtra_;
      if (numberColumns)
	nextColumn[numberColumns - 1] = maximumColumnsExtra_;
      startColumn[maximumColumnsExtra_] = numberElements;
    }
  }				/* endswitch */
}

//Does most of factorization
int
CoinFactorization::factor (  )
{
  int * lastColumn = lastColumn_.array();
  int * lastRow = lastRow_.array();
  //sparse
  status_ = factorSparse (  );
  switch ( status_ ) {
  case 0:			//finished
    totalElements_ = 0;
    {
      int * pivotColumn = pivotColumn_.array();
      if ( numberGoodU_ < numberRows_ ) {
	int i, k;
	// Clean out unset nextRow
	int * nextRow = nextRow_.array();
	//int nSing =0;
	k=nextRow[maximumRowsExtra_];
	while (k!=maximumRowsExtra_ && k>=0) {
	  int iRow = k;
	  k=nextRow[k];
	  //nSing++;
	  nextRow[iRow]=-1;
	}
	//assert (nSing);
	//printf("%d singularities - good %d rows %d\n",nSing,
	//     numberGoodU_,numberRows_);
	// Now nextRow has -1 or sequence into numberGoodU_;
	int * permuteA = permute_.array();
	for ( i = 0; i < numberRows_; i++ ) {
	  int iGood= nextRow[i];
	  if (iGood>=0)
	    permuteA[iGood]=i;
	}

	// swap arrays
	permute_.swap(nextRow_);
	int * permute = permute_.array();
	for ( i = 0; i < numberRows_; i++ ) {
	  lastRow[i] = -1;
	}
	for ( i = 0; i < numberColumns_; i++ ) {
	  lastColumn[i] = -1;
	}
	for ( i = 0; i < numberGoodU_; i++ ) {
	  int goodRow = permuteA[i];	//valid pivot row
	  int goodColumn = pivotColumn[i];

	  lastRow[goodRow] = goodColumn;	//will now have -1 or column sequence
	  lastColumn[goodColumn] = goodRow;	//will now have -1 or row sequence
	}
	nextRow_.conditionalDelete();
	k = 0;
	//copy back and count
	for ( i = 0; i < numberRows_; i++ ) {
	  permute[i] = lastRow[i];
	  if ( permute[i] < 0 ) {
	    //std::cout << i << " " <<permute[i] << std::endl;
	  } else {
	    k++;
	  }
	}
	for ( i = 0; i < numberColumns_; i++ ) {
	  pivotColumn[i] = lastColumn[i];
	}
	if ((messageLevel_&4)!=0) 
	  std::cout <<"Factorization has "<<numberRows_-k
		    <<" singularities"<<std::endl;
	status_ = -1;
      }
    }
    break;
    // dense
  case 2:
    status_=factorDense();
    if(!status_) 
      break;
  default:
    //singular ? or some error
    if ((messageLevel_&4)!=0) 
      std::cout << "Error " << status_ << std::endl;
    break;
  }				/* endswitch */
  //clean up
  if ( !status_ ) {
    if ( (messageLevel_ & 16)&&numberCompressions_)
      std::cout<<"        Factorization did "<<numberCompressions_
	       <<" compressions"<<std::endl;
    if ( numberCompressions_ > 10 ) {
      areaFactor_ *= 1.1;
    }
    numberCompressions_=0;
    cleanup (  );
  }
  return status_;
}


//  pivotRowSingleton.  Does one pivot on Row Singleton in factorization
bool
CoinFactorization::pivotRowSingleton ( int pivotRow,
				  int pivotColumn )
{
  //store pivot columns (so can easily compress)
  CoinBigIndex * startColumnU = startColumnU_.array();
  CoinBigIndex startColumn = startColumnU[pivotColumn];
  int * numberInRow = numberInRow_.array();
  int * numberInColumn = numberInColumn_.array();
  int numberDoColumn = numberInColumn[pivotColumn] - 1;
  CoinBigIndex endColumn = startColumn + numberDoColumn + 1;
  CoinBigIndex pivotRowPosition = startColumn;
  int * indexRowU = indexRowU_.array();
  int iRow = indexRowU[pivotRowPosition];
  CoinBigIndex * startRowU = startRowU_.array();
  int * nextRow = nextRow_.array();
  int * lastRow = lastRow_.array();

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
  CoinBigIndex * startColumnL = startColumnL_.array();
  CoinFactorizationDouble * elementL = elementL_.array();
  int * indexRowL = indexRowL_.array();
  startColumnL[numberGoodL_] = l;	//for luck and first time
  numberGoodL_++;
  startColumnL[numberGoodL_] = l + numberDoColumn;
  lengthL_ += numberDoColumn;
  CoinFactorizationDouble *elementU = elementU_.array();
  CoinFactorizationDouble pivotElement = elementU[pivotRowPosition];
  CoinFactorizationDouble pivotMultiplier = 1.0 / pivotElement;

  pivotRegion_.array()[numberGoodU_] = pivotMultiplier;
  CoinBigIndex i;

  int * indexColumnU = indexColumnU_.array();
  for ( i = startColumn; i < pivotRowPosition; i++ ) {
    int iRow = indexRowU[i];

    indexRowL[l] = iRow;
    elementL[l] = elementU[i] * pivotMultiplier;
    l++;
    //take out of row list
    CoinBigIndex start = startRowU[iRow];
    int iNumberInRow = numberInRow[iRow];
    CoinBigIndex end = start + iNumberInRow;
    CoinBigIndex where = start;

    while ( indexColumnU[where] != pivotColumn ) {
      where++;
    }				/* endwhile */
    assert ( where < end );
    indexColumnU[where] = indexColumnU[end - 1];
    iNumberInRow--;
    numberInRow[iRow] = iNumberInRow;
    deleteLink ( iRow );
    addLink ( iRow, iNumberInRow );
  }
  for ( i = pivotRowPosition + 1; i < endColumn; i++ ) {
    int iRow = indexRowU[i];

    indexRowL[l] = iRow;
    elementL[l] = elementU[i] * pivotMultiplier;
    l++;
    //take out of row list
    CoinBigIndex start = startRowU[iRow];
    int iNumberInRow = numberInRow[iRow];
    CoinBigIndex end = start + iNumberInRow;
    CoinBigIndex where = start;

    while ( indexColumnU[where] != pivotColumn ) {
      where++;
    }				/* endwhile */
    assert ( where < end );
    indexColumnU[where] = indexColumnU[end - 1];
    iNumberInRow--;
    numberInRow[iRow] = iNumberInRow;
    deleteLink ( iRow );
    addLink ( iRow, iNumberInRow );
  }
  numberInColumn[pivotColumn] = 0;
  //modify linked list for pivots
  numberInRow[pivotRow] = 0;
  deleteLink ( pivotRow );
  deleteLink ( pivotColumn + numberRows_ );
  //take out this bit of indexColumnU
  int next = nextRow[pivotRow];
  int last = lastRow[pivotRow];

  nextRow[last] = next;
  lastRow[next] = last;
  lastRow[pivotRow] =-2;
  nextRow[pivotRow] = numberGoodU_;	//use for permute
  return true;
}

//  pivotColumnSingleton.  Does one pivot on Column Singleton in factorization
bool
CoinFactorization::pivotColumnSingleton ( int pivotRow,
				     int pivotColumn )
{
  int * numberInRow = numberInRow_.array();
  int * numberInColumn = numberInColumn_.array();
  int * numberInColumnPlus = numberInColumnPlus_.array();
  //store pivot columns (so can easily compress)
  int numberDoRow = numberInRow[pivotRow] - 1;
  CoinBigIndex * startColumnU = startColumnU_.array();
  CoinBigIndex startColumn = startColumnU[pivotColumn];
  int put = 0;
  CoinBigIndex * startRowU = startRowU_.array();
  CoinBigIndex startRow = startRowU[pivotRow];
  CoinBigIndex endRow = startRow + numberDoRow + 1;
  int * indexColumnU = indexColumnU_.array();
  int * indexRowU = indexRowU_.array();
  int * saveColumn = saveColumn_.array();
  CoinBigIndex i;

  for ( i = startRow; i < endRow; i++ ) {
    int iColumn = indexColumnU[i];

    if ( iColumn != pivotColumn ) {
      saveColumn[put++] = iColumn;
    }
  }
  int * nextRow = nextRow_.array();
  int * lastRow = lastRow_.array();
  //take out this bit of indexColumnU
  int next = nextRow[pivotRow];
  int last = lastRow[pivotRow];

  nextRow[last] = next;
  lastRow[next] = last;
  nextRow[pivotRow] = numberGoodU_;	//use for permute
  lastRow[pivotRow] =-2; //mark
  //clean up counts
  CoinFactorizationDouble *elementU = elementU_.array();
  CoinFactorizationDouble pivotElement = elementU[startColumn];

  pivotRegion_.array()[numberGoodU_] = 1.0 / pivotElement;
  numberInColumn[pivotColumn] = 0;
  //totalElements_ --;
  //numberInColumnPlus[pivotColumn]++;
  //move pivot row in other columns to safe zone
  for ( i = 0; i < numberDoRow; i++ ) {
    int iColumn = saveColumn[i];

    if ( numberInColumn[iColumn] ) {
      int number = numberInColumn[iColumn] - 1;

      //modify linked list
      deleteLink ( iColumn + numberRows_ );
      addLink ( iColumn + numberRows_, number );
      //move pivot row element
      if ( number ) {
	CoinBigIndex start = startColumnU[iColumn];
	CoinBigIndex pivot = start;
	int iRow = indexRowU[pivot];
	while ( iRow != pivotRow ) {
	  pivot++;
	  iRow = indexRowU[pivot];
	}
        assert (pivot < startColumnU[iColumn] +
                numberInColumn[iColumn]);
	if ( pivot != start ) {
	  //move largest one up
	  CoinFactorizationDouble value = elementU[start];

	  iRow = indexRowU[start];
	  elementU[start] = elementU[pivot];
	  indexRowU[start] = indexRowU[pivot];
	  elementU[pivot] = elementU[start + 1];
	  indexRowU[pivot] = indexRowU[start + 1];
	  elementU[start + 1] = value;
	  indexRowU[start + 1] = iRow;
	} else {
	  //find new largest element
	  int iRowSave = indexRowU[start + 1];
	  CoinFactorizationDouble valueSave = elementU[start + 1];
	  double valueLargest = fabs ( valueSave );
	  CoinBigIndex end = start + numberInColumn[iColumn];
	  CoinBigIndex largest = start + 1;

	  CoinBigIndex k;
	  for ( k = start + 2; k < end; k++ ) {
	    CoinFactorizationDouble value = elementU[k];
	    double valueAbs = fabs ( value );

	    if ( valueAbs > valueLargest ) {
	      valueLargest = valueAbs;
	      largest = k;
	    }
	  }
	  indexRowU[start + 1] = indexRowU[largest];
	  elementU[start + 1] = elementU[largest];
	  indexRowU[largest] = iRowSave;
	  elementU[largest] = valueSave;
	}
      }
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

  CoinBigIndex * startColumnL = startColumnL_.array();
  startColumnL[numberGoodL_] = l;	//for luck and first time
  numberGoodL_++;
  startColumnL[numberGoodL_] = l;
  return true;
}


//  getColumnSpace.  Gets space for one Column with given length
//may have to do compression  (returns true)
//also moves existing vector
bool
CoinFactorization::getColumnSpace ( int iColumn,
			       int extraNeeded )
{
  int * numberInColumn = numberInColumn_.array();
  int * numberInColumnPlus = numberInColumnPlus_.array();
  int * nextColumn = nextColumn_.array();
  int * lastColumn = lastColumn_.array();
  int number = numberInColumnPlus[iColumn] +
    numberInColumn[iColumn];
  CoinBigIndex * startColumnU = startColumnU_.array();
  CoinBigIndex space = lengthAreaU_ - startColumnU[maximumColumnsExtra_];
  CoinFactorizationDouble *elementU = elementU_.array();
  int * indexRowU = indexRowU_.array();

  if ( space < extraNeeded + number + 4 ) {
    //compression
    int iColumn = nextColumn[maximumColumnsExtra_];
    CoinBigIndex put = 0;

    while ( iColumn != maximumColumnsExtra_ ) {
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
      CoinBigIndex i;
      for ( i = get; i < getEnd; i++ ) {
	indexRowU[put] = indexRowU[i];
	elementU[put] = elementU[i];
	put++;
      }
      iColumn = nextColumn[iColumn];
    }				/* endwhile */
    numberCompressions_++;
    startColumnU[maximumColumnsExtra_] = put;
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
  CoinBigIndex put = startColumnU[maximumColumnsExtra_];
  int next = nextColumn[iColumn];
  int last = lastColumn[iColumn];

  if ( extraNeeded || next != maximumColumnsExtra_ ) {
    //out
    nextColumn[last] = next;
    lastColumn[next] = last;
    //in at end
    last = lastColumn[maximumColumnsExtra_];
    nextColumn[last] = iColumn;
    lastColumn[maximumColumnsExtra_] = iColumn;
    lastColumn[iColumn] = last;
    nextColumn[iColumn] = maximumColumnsExtra_;
    //move
    CoinBigIndex get = startColumnU[iColumn]
      - numberInColumnPlus[iColumn];

    startColumnU[iColumn] = put + numberInColumnPlus[iColumn];
    if ( number < 50 ) {
      int *indexRow = indexRowU;
      CoinFactorizationDouble *element = elementU;
      int i = 0;

      if ( ( number & 1 ) != 0 ) {
	element[put] = element[get];
	indexRow[put] = indexRow[get];
	i = 1;
      }
      for ( ; i < number; i += 2 ) {
	CoinFactorizationDouble value0 = element[get + i];
	CoinFactorizationDouble value1 = element[get + i + 1];
	int index0 = indexRow[get + i];
	int index1 = indexRow[get + i + 1];

	element[put + i] = value0;
	element[put + i + 1] = value1;
	indexRow[put + i] = index0;
	indexRow[put + i + 1] = index1;
      }
    } else {
      CoinMemcpyN ( &indexRowU[get], number, &indexRowU[put] );
      CoinMemcpyN ( &elementU[get], number, &elementU[put] );
    }
    put += number;
    get += number;
    //add 2 for luck
    startColumnU[maximumColumnsExtra_] = put + extraNeeded + 2;
    if (startColumnU[maximumColumnsExtra_]>lengthAreaU_) {
      // get more memory
#ifdef CLP_DEVELOP
      printf("put %d, needed %d, start %d, length %d\n",
	     put,extraNeeded,startColumnU[maximumColumnsExtra_],
	     lengthAreaU_);
#endif
      return false;
    }
  } else {
    //take off space
    startColumnU[maximumColumnsExtra_] = startColumnU[last] +
      numberInColumn[last];
  }
  return true;
}

//  getRowSpace.  Gets space for one Row with given length
//may have to do compression  (returns true)
//also moves existing vector
bool
CoinFactorization::getRowSpace ( int iRow,
			    int extraNeeded )
{
  int * numberInRow = numberInRow_.array();
  int number = numberInRow[iRow];
  CoinBigIndex * startRowU = startRowU_.array();
  CoinBigIndex space = lengthAreaU_ - startRowU[maximumRowsExtra_];
  int * nextRow = nextRow_.array();
  int * lastRow = lastRow_.array();
  int * indexColumnU = indexColumnU_.array();

  if ( space < extraNeeded + number + 2 ) {
    //compression
    int iRow = nextRow[maximumRowsExtra_];
    CoinBigIndex put = 0;

    while ( iRow != maximumRowsExtra_ ) {
      //move
      CoinBigIndex get = startRowU[iRow];
      CoinBigIndex getEnd = startRowU[iRow] + numberInRow[iRow];

      startRowU[iRow] = put;
      CoinBigIndex i;
      for ( i = get; i < getEnd; i++ ) {
	indexColumnU[put] = indexColumnU[i];
	put++;
      }
      iRow = nextRow[iRow];
    }				/* endwhile */
    numberCompressions_++;
    startRowU[maximumRowsExtra_] = put;
    space = lengthAreaU_ - put;
    if ( space < extraNeeded + number + 2 ) {
      //need more space
      //if we can allocate bigger then do so and copy
      //if not then return so code can start again
      status_ = -99;
      return false;;
    }
  }
  CoinBigIndex put = startRowU[maximumRowsExtra_];
  int next = nextRow[iRow];
  int last = lastRow[iRow];

  //out
  nextRow[last] = next;
  lastRow[next] = last;
  //in at end
  last = lastRow[maximumRowsExtra_];
  nextRow[last] = iRow;
  lastRow[maximumRowsExtra_] = iRow;
  lastRow[iRow] = last;
  nextRow[iRow] = maximumRowsExtra_;
  //move
  CoinBigIndex get = startRowU[iRow];

  startRowU[iRow] = put;
  while ( number ) {
    number--;
    indexColumnU[put] = indexColumnU[get];
    put++;
    get++;
  }				/* endwhile */
  //add 4 for luck
  startRowU[maximumRowsExtra_] = put + extraNeeded + 4;
  return true;
}

#if COIN_ONE_ETA_COPY
/* Reorders U so contiguous and in order (if there is space)
   Returns true if it could */
bool 
CoinFactorization::reorderU()
{
#if 1
  return false;
#else
  if (numberRows_!=numberColumns_)
    return false;
  CoinBigIndex * startColumnU = startColumnU_.array();
  int * numberInColumn = numberInColumn_.array();
  int * numberInColumnPlus = numberInColumnPlus_.array();
  int iColumn;
  CoinBigIndex put = 0;
  for (iColumn =0;iColumn<numberRows_;iColumn++) 
    put += numberInColumnPlus[iColumn];
  CoinBigIndex space = lengthAreaU_ - startColumnU[maximumColumnsExtra_];
  if (space<put) {
    //printf("Space %d out of %d - needed %d\n",
    //   space,lengthAreaU_,put);
    return false;
  }
  int *indexRowU = indexRowU_.array();
  CoinFactorizationDouble *elementU = elementU_.array();
  int * pivotColumn = pivotColumn_.array();
  put = startColumnU[maximumColumnsExtra_];
  for (int jColumn =0;jColumn<numberRows_;jColumn++) {
    iColumn = pivotColumn[jColumn];
    int n = numberInColumnPlus[iColumn];
    CoinBigIndex getEnd = startColumnU[iColumn];
    CoinBigIndex get = getEnd - n;
    startColumnU[iColumn] = put;
    numberInColumn[jColumn]=n;
    CoinBigIndex i;
    for ( i = get; i < getEnd; i++ ) {
      indexRowU[put] = indexRowU[i];
      elementU[put] = elementU[i];
      put++;
    }
  }
  // and pack down
  put = 0;
  for (int jColumn =0;jColumn<numberRows_;jColumn++) {
    iColumn = pivotColumn[jColumn];
    int n = numberInColumn[jColumn];
    CoinBigIndex get = startColumnU[iColumn];
    CoinBigIndex getEnd = get+n;
    CoinBigIndex i;
    for ( i = get; i < getEnd; i++ ) {
      indexRowU[put] = indexRowU[i];
      elementU[put] = elementU[i];
      put++;
    }
  }
  put=0;
  for (iColumn =0;iColumn<numberRows_;iColumn++) { 
    int n = numberInColumn[iColumn];
    startColumnU[iColumn]=put;
    put += n;
    //numberInColumnPlus[iColumn]=n;
    //numberInColumn[iColumn]=0; // necessary?
    //pivotColumn[iColumn]=iColumn;
  }
#if 0
  // reset nextColumn - probably not necessary
  int * nextColumn = nextColumn_.array();
  nextColumn[maximumColumnsExtra_]=0;
  nextColumn[numberRows_-1] = maximumColumnsExtra_;
  for (iColumn=0;iColumn<numberRows_-1;iColumn++)
    nextColumn[iColumn]=iColumn+1;
  // swap arrays
  numberInColumn_.swap(numberInColumnPlus_);
#endif
  //return false;
  return true;
#endif
}
#endif
//  cleanup.  End of factorization
void
CoinFactorization::cleanup (  )
{
#if COIN_ONE_ETA_COPY
  bool compressDone = reorderU();
  if (!compressDone) {
    getColumnSpace ( 0, COIN_INT_MAX >> 1 );	//compress
    // swap arrays
    numberInColumn_.swap(numberInColumnPlus_);
  }
#else
  getColumnSpace ( 0, COIN_INT_MAX >> 1 );	//compress
  // swap arrays
    numberInColumn_.swap(numberInColumnPlus_);
#endif
  CoinBigIndex * startColumnU = startColumnU_.array();
  CoinBigIndex lastU = startColumnU[maximumColumnsExtra_];

  //free some memory here
  saveColumn_.conditionalDelete();
  markRow_.conditionalDelete() ;
  //firstCount_.conditionalDelete() ;
  nextCount_.conditionalDelete() ;
  lastCount_.conditionalDelete() ;
  int * numberInRow = numberInRow_.array();
  int * numberInColumn = numberInColumn_.array();
  int * numberInColumnPlus = numberInColumnPlus_.array();
  //make column starts OK
  //for best cache behavior get in order (last pivot at bottom of space)
  //that will need thinking about
  //use nextRow for permutation  (as that is what it is)
  int i;

#ifndef NDEBUG
  {
    if (numberGoodU_<numberRows_)
      abort();
    char * mark = new char[numberRows_];
    memset(mark,0,numberRows_);
    int * array;
    array = nextRow_.array();
    for ( i = 0; i < numberRows_; i++ ) {
      int k = array[i];
      if(k<0||k>=numberRows_)
	printf("Bad a %d %d\n",i,k);
      assert(k>=0&&k<numberRows_);
      if(mark[k]==1)
	printf("Bad a %d %d\n",i,k);
      mark[k]=1;
    }
    for ( i = 0; i < numberRows_; i++ ) {
      assert(mark[i]==1);
      if(mark[i]!=1)
	printf("Bad b %d\n",i);
    }
    delete [] mark;
  }
#endif
  // swap arrays
  permute_.swap(nextRow_);
  //safety feature
  int * permute = permute_.array();
  permute[numberRows_] = 0;
  permuteBack_.conditionalNew( maximumRowsExtra_ + 1);
  int * permuteBack = permuteBack_.array();
#ifdef ZEROFAULT
  memset(permuteBack_.array(),'w',(maximumRowsExtra_+1)*sizeof(int));
#endif
  for ( i = 0; i < numberRows_; i++ ) {
    int iRow = permute[i];

    permuteBack[iRow] = i;
  }
  //redo nextRow_

#ifndef NDEBUG
  for ( i = 0; i < numberRows_; i++ ) {
    assert (permute[i]>=0&&permute[i]<numberRows_);
    assert (permuteBack[i]>=0&&permuteBack[i]<numberRows_);
  }
#endif
#if COIN_ONE_ETA_COPY
  if (!compressDone) {
#endif
    // Redo total elements
    totalElements_=0;
    for ( i = 0; i < numberColumns_; i++ ) {
      int number = numberInColumn[i];
      totalElements_ += number;
      startColumnU[i] -= number;
    }
#if COIN_ONE_ETA_COPY
  }
#endif
  int numberU = 0;

  pivotColumnBack_.conditionalNew( maximumRowsExtra_ + 1);
#ifdef ZEROFAULT
  memset(pivotColumnBack(),'q',(maximumRowsExtra_+1)*sizeof(int));
#endif
  const int * pivotColumn = pivotColumn_.array();
  int * pivotColumnB = pivotColumnBack();
  int *indexColumnU = indexColumnU_.array();
  CoinBigIndex *startColumn = startColumnU;
  int *indexRowU = indexRowU_.array();
  CoinFactorizationDouble *elementU = elementU_.array();
#if COIN_ONE_ETA_COPY
  if (!compressDone) {
#endif
    for ( i = 0; i < numberColumns_; i++ ) {
      int iColumn = pivotColumn[i];
      
      pivotColumnB[iColumn] = i;
      if ( iColumn >= 0 ) {
	//wanted
	if ( numberU != iColumn ) {
	  numberInColumnPlus[iColumn] = numberU;
	} else {
	  numberInColumnPlus[iColumn] = -1;	//already in correct place
	}
	numberU++;
      }
    }
    for ( i = 0; i < numberColumns_; i++ ) {
      int number = numberInColumn[i];	//always 0?
      int where = numberInColumnPlus[i];
      
      numberInColumnPlus[i] = -1;
      CoinBigIndex start = startColumnU[i];
      
      while ( where >= 0 ) {
	//put where it should be
	int numberNext = numberInColumn[where];	//always 0?
	int whereNext = numberInColumnPlus[where];
	CoinBigIndex startNext = startColumnU[where];
	
	numberInColumn[where] = number;
	numberInColumnPlus[where] = -1;
	startColumnU[where] = start;
	number = numberNext;
	where = whereNext;
	start = startNext;
      }				/* endwhile */
    }
    //sort - using indexColumn
    CoinFillN ( indexColumnU_.array(), lastU, -1 );
    CoinBigIndex k = 0;
    
    for ( i = numberSlacks_; i < numberRows_; i++ ) {
      CoinBigIndex start = startColumn[i];
      CoinBigIndex end = start + numberInColumn[i];
      
      CoinBigIndex j;
      for ( j = start; j < end; j++ ) {
	indexColumnU[j] = k++;
      }
    }
    for ( i = numberSlacks_; i < numberRows_; i++ ) {
      CoinBigIndex start = startColumn[i];
      CoinBigIndex end = start + numberInColumn[i];
      
      CoinBigIndex j;
      for ( j = start; j < end; j++ ) {
	CoinBigIndex k = indexColumnU[j];
	int iRow = indexRowU[j];
	CoinFactorizationDouble element = elementU[j];
	
	while ( k != -1 ) {
	  CoinBigIndex kNext = indexColumnU[k];
	  int iRowNext = indexRowU[k];
	  CoinFactorizationDouble elementNext = elementU[k];
	  
	  indexColumnU[k] = -1;
	  indexRowU[k] = iRow;
	  elementU[k] = element;
	  k = kNext;
	  iRow = iRowNext;
	  element = elementNext;
	}				/* endwhile */
      }
    }
    CoinZeroN ( startColumnU, numberSlacks_ );
    k = 0;
    for ( i = numberSlacks_; i < numberRows_; i++ ) {
      startColumnU[i] = k;
      k += numberInColumn[i];
    }
    maximumU_=k;
#if COIN_ONE_ETA_COPY
  } else {
    // U already OK
    for ( i = 0; i < numberColumns_; i++ ) {
      int iColumn = pivotColumn[i];
      pivotColumnB[iColumn] = i;
    }
    maximumU_=startColumnU[numberRows_-1]+
      numberInColumn[numberRows_-1];
    numberU=numberRows_;
  }
#endif
  if ( (messageLevel_ & 8)) {
    std::cout<<"        length of U "<<totalElements_<<", length of L "<<lengthL_;
    if (numberDense_)
      std::cout<<" plus "<<numberDense_*numberDense_<<" from "<<numberDense_<<" dense rows";
    std::cout<<std::endl;
  }
  // and add L and dense
  totalElements_ += numberDense_*numberDense_+lengthL_;
  int * nextColumn = nextColumn_.array();
  int * lastColumn = lastColumn_.array();
  // See whether to have extra copy of R
  if (maximumU_>10*numberRows_||numberRows_<200) {
    // NO
    numberInColumnPlus_.conditionalDelete() ;
  } else {
    for ( i = 0; i < numberColumns_; i++ ) {
      lastColumn[i] = i - 1;
      nextColumn[i] = i + 1;
      numberInColumnPlus[i]=0;
    }
    nextColumn[numberColumns_ - 1] = maximumColumnsExtra_;
    lastColumn[maximumColumnsExtra_] = numberColumns_ - 1;
    nextColumn[maximumColumnsExtra_] = 0;
    lastColumn[0] = maximumColumnsExtra_;
  }
  numberU_ = numberU;
  numberGoodU_ = numberU;
  numberL_ = numberGoodL_;
#if COIN_DEBUG
  for ( i = 0; i < numberRows_; i++ ) {
    if ( permute[i] < 0 ) {
      std::cout << i << std::endl;
      abort (  );
    }
  }
#endif
  CoinFactorizationDouble * pivotRegion = pivotRegion_.array();
  for ( i = numberSlacks_; i < numberU; i++ ) {
    CoinBigIndex start = startColumnU[i];
    CoinBigIndex end = start + numberInColumn[i];

    totalElements_ += numberInColumn[i];
    CoinBigIndex j;

    for ( j = start; j < end; j++ ) {
      int iRow = indexRowU[j];
      iRow = permute[iRow];
      indexRowU[j] = iRow;
      numberInRow[iRow]++;
    }
  }
#if COIN_ONE_ETA_COPY
  if (numberRows_>=COIN_ONE_ETA_COPY) {
#endif
    //space for cross reference
    convertRowToColumnU_.conditionalNew( lengthAreaU_ );
    CoinBigIndex *convertRowToColumn = convertRowToColumnU_.array();
    CoinBigIndex j = 0;
    CoinBigIndex *startRow = startRowU_.array();
    
    int iRow;
    for ( iRow = 0; iRow < numberRows_; iRow++ ) {
      startRow[iRow] = j;
      j += numberInRow[iRow];
    }
    CoinBigIndex numberInU = j;
    
    CoinZeroN ( numberInRow_.array(), numberRows_ );
    
    for ( i = numberSlacks_; i < numberRows_; i++ ) {
      CoinBigIndex start = startColumnU[i];
      CoinBigIndex end = start + numberInColumn[i];
      
      CoinFactorizationDouble pivotValue = pivotRegion[i];

      CoinBigIndex j;
      for ( j = start; j < end; j++ ) {
	int iRow = indexRowU[j];
	int iLook = numberInRow[iRow];
	
	numberInRow[iRow] = iLook + 1;
	CoinBigIndex k = startRow[iRow] + iLook;
	
	indexColumnU[k] = i;
	convertRowToColumn[k] = j;
	//multiply by pivot
	elementU[j] *= pivotValue;
      }
    }
    int * nextRow = nextRow_.array();
    int * lastRow = lastRow_.array();
    for ( j = 0; j < numberRows_; j++ ) {
      lastRow[j] = j - 1;
      nextRow[j] = j + 1;
    }
    nextRow[numberRows_ - 1] = maximumRowsExtra_;
    lastRow[maximumRowsExtra_] = numberRows_ - 1;
    nextRow[maximumRowsExtra_] = 0;
    lastRow[0] = maximumRowsExtra_;
    startRow[maximumRowsExtra_] = numberInU;
#if COIN_ONE_ETA_COPY
  } else {
    // no row copy
    for ( i = numberSlacks_; i < numberU; i++ ) {
      CoinBigIndex start = startColumnU[i];
      CoinBigIndex end = start + numberInColumn[i];
      
      CoinBigIndex j;
      CoinFactorizationDouble pivotValue = pivotRegion[i];
      
      for ( j = start; j < end; j++ ) {
	//multiply by pivot
	elementU[j] *= pivotValue;
      }
    }
  }
#endif

  int firstReal = numberRows_;

  CoinBigIndex * startColumnL = startColumnL_.array();
  int * indexRowL = indexRowL_.array();
  for ( i = numberRows_ - 1; i >= 0; i-- ) {
    CoinBigIndex start = startColumnL[i];
    CoinBigIndex end = startColumnL[i + 1];

    totalElements_ += end - start;
    if ( end > start ) {
      firstReal = i;
      CoinBigIndex j;
      for ( j = start; j < end; j++ ) {
	int iRow = indexRowL[j];
	iRow = permute[iRow];
	assert (iRow>firstReal);
	indexRowL[j] = iRow;
      }
    }
  }
  baseL_ = firstReal;
  numberL_ -= firstReal;
  factorElements_ = totalElements_;
  //can delete pivotRowL_ as not used
  pivotRowL_.conditionalDelete() ;
  //use L for R if room
  CoinBigIndex space = lengthAreaL_ - lengthL_;
  CoinBigIndex spaceUsed = lengthL_ + lengthU_;

  int needed = ( spaceUsed + numberRows_ - 1 ) / numberRows_;

  needed = needed * 2 * maximumPivots_;
  if ( needed < 2 * numberRows_ ) {
    needed = 2 * numberRows_;
  }
  if (numberInColumnPlus_.array()) {
    // Need double the space for R
    space = space/2;
    startColumnR_.conditionalNew( maximumPivots_ + 1 + maximumColumnsExtra_ + 1 );
    CoinBigIndex * startR = startColumnR_.array() + maximumPivots_+1;
    CoinZeroN (startR,(maximumColumnsExtra_+1));
  } else {
    startColumnR_.conditionalNew(maximumPivots_ + 1 );
  }
#ifdef ZEROFAULT
  memset(startColumnR_.array(),'z',(maximumPivots_ + 1)*sizeof(CoinBigIndex));
#endif
  if ( space >= needed ) {
    lengthR_ = 0;
    lengthAreaR_ = space;
    elementR_ = elementL_.array() + lengthL_;
    indexRowR_ = indexRowL_.array() + lengthL_;
  } else {
    lengthR_ = 0;
    lengthAreaR_ = space;
    elementR_ = elementL_.array() + lengthL_;
    indexRowR_ = indexRowL_.array() + lengthL_;
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
CoinFactorization::adjustedAreaFactor() const
{
  double factor = areaFactor_;
  if (numberDense_&&areaFactor_>1.0) {
    double dense = numberDense_;
    dense *= dense;
    double withoutDense = totalElements_ - dense +1.0;
    factor *= 1.0 +dense/withoutDense;
  }
  return factor;
}

//  checkConsistency.  Checks that row and column copies look OK
void
CoinFactorization::checkConsistency (  )
{
  bool bad = false;

  int iRow;
  CoinBigIndex * startRowU = startRowU_.array();
  int * numberInRow = numberInRow_.array();
  int * numberInColumn = numberInColumn_.array();
  int * indexColumnU = indexColumnU_.array();
  int * indexRowU = indexRowU_.array();
  CoinBigIndex * startColumnU = startColumnU_.array();
  for ( iRow = 0; iRow < numberRows_; iRow++ ) {
    if ( numberInRow[iRow] ) {
      CoinBigIndex startRow = startRowU[iRow];
      CoinBigIndex endRow = startRow + numberInRow[iRow];

      CoinBigIndex j;
      for ( j = startRow; j < endRow; j++ ) {
	int iColumn = indexColumnU[j];
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
  int iColumn;
  for ( iColumn = 0; iColumn < numberColumns_; iColumn++ ) {
    if ( numberInColumn[iColumn] ) {
      CoinBigIndex startColumn = startColumnU[iColumn];
      CoinBigIndex endColumn = startColumn + numberInColumn[iColumn];

      CoinBigIndex j;
      for ( j = startColumn; j < endColumn; j++ ) {
	int iRow = indexRowU[j];
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
CoinFactorization::pivotOneOtherRow ( int pivotRow,
					   int pivotColumn )
{
  int * numberInRow = numberInRow_.array();
  int * numberInColumn = numberInColumn_.array();
  int * numberInColumnPlus = numberInColumnPlus_.array();
  int numberInPivotRow = numberInRow[pivotRow] - 1;
  CoinBigIndex * startRowU = startRowU_.array();
  CoinBigIndex * startColumnU = startColumnU_.array();
  CoinBigIndex startColumn = startColumnU[pivotColumn];
  CoinBigIndex startRow = startRowU[pivotRow];
  CoinBigIndex endRow = startRow + numberInPivotRow + 1;

  //take out this bit of indexColumnU
  int * nextRow = nextRow_.array();
  int * lastRow = lastRow_.array();
  int next = nextRow[pivotRow];
  int last = lastRow[pivotRow];

  nextRow[last] = next;
  lastRow[next] = last;
  nextRow[pivotRow] = numberGoodU_;	//use for permute
  lastRow[pivotRow] = -2;
  numberInRow[pivotRow] = 0;
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
  CoinBigIndex * startColumnL = startColumnL_.array();
  CoinFactorizationDouble * elementL = elementL_.array();
  int * indexRowL = indexRowL_.array();
  startColumnL[numberGoodL_] = l;	//for luck and first time
  numberGoodL_++;
  startColumnL[numberGoodL_] = l + 1;
  lengthL_++;
  CoinFactorizationDouble pivotElement;
  CoinFactorizationDouble otherMultiplier;
  int otherRow;
  int * saveColumn = saveColumn_.array();
  CoinFactorizationDouble *elementU = elementU_.array();
  int * indexRowU = indexRowU_.array();

  if ( indexRowU[startColumn] == pivotRow ) {
    pivotElement = elementU[startColumn];
    otherMultiplier = elementU[startColumn + 1];
    otherRow = indexRowU[startColumn + 1];
  } else {
    pivotElement = elementU[startColumn + 1];
    otherMultiplier = elementU[startColumn];
    otherRow = indexRowU[startColumn];
  }
  int numberSave = numberInRow[otherRow];
  CoinFactorizationDouble pivotMultiplier = 1.0 / pivotElement;

  CoinFactorizationDouble * pivotRegion = pivotRegion_.array();
  pivotRegion[numberGoodU_] = pivotMultiplier;
  numberInColumn[pivotColumn] = 0;
  otherMultiplier = otherMultiplier * pivotMultiplier;
  indexRowL[l] = otherRow;
  elementL[l] = otherMultiplier;
  //take out of row list
  CoinBigIndex start = startRowU[otherRow];
  CoinBigIndex end = start + numberSave;
  CoinBigIndex where = start;
  int * indexColumnU = indexColumnU_.array();

  while ( indexColumnU[where] != pivotColumn ) {
    where++;
  }				/* endwhile */
  assert ( where < end );
  end--;
  indexColumnU[where] = indexColumnU[end];
  int numberAdded = 0;
  int numberDeleted = 0;

  //pack down and move to work
  int j;
  const int * nextCount = nextCount_.array();
  int * nextColumn = nextColumn_.array();

  for ( j = startRow; j < endRow; j++ ) {
    int iColumn = indexColumnU[j];

    if ( iColumn != pivotColumn ) {
      CoinBigIndex startColumn = startColumnU[iColumn];
      CoinBigIndex endColumn = startColumn + numberInColumn[iColumn];
      int iRow = indexRowU[startColumn];
      CoinFactorizationDouble value = elementU[startColumn];
      double largest;
      bool foundOther = false;

      //leave room for pivot
      CoinBigIndex put = startColumn + 1;
      CoinBigIndex positionLargest = -1;
      CoinFactorizationDouble thisPivotValue = 0.0;
      CoinFactorizationDouble otherElement = 0.0;
      CoinFactorizationDouble nextValue = elementU[put];;
      int nextIRow = indexRowU[put];

      //compress column and find largest not updated
      if ( iRow != pivotRow ) {
	if ( iRow != otherRow ) {
	  largest = fabs ( value );
	  elementU[put] = value;
	  indexRowU[put] = iRow;
	  positionLargest = put;
	  put++;
	  CoinBigIndex i;
	  for ( i = startColumn + 1; i < endColumn; i++ ) {
	    iRow = nextIRow;
	    value = nextValue;
#ifdef ZEROFAULT
	    // doesn't matter reading uninitialized but annoys checking
	    if ( i + 1 < endColumn ) {
#endif
	      nextIRow = indexRowU[i + 1];
	      nextValue = elementU[i + 1];
#ifdef ZEROFAULT
	    }
#endif
	    if ( iRow != pivotRow ) {
	      if ( iRow != otherRow ) {
		//keep
		indexRowU[put] = iRow;
		elementU[put] = value;;
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
#ifdef ZEROFAULT
	    // doesn't matter reading uninitialized but annoys checking
	    if ( i + 1 < endColumn ) {
#endif
	      nextIRow = indexRowU[i + 1];
	      nextValue = elementU[i + 1];
#ifdef ZEROFAULT
	    }
#endif
	    if ( iRow != pivotRow ) {
	      //keep
	      indexRowU[put] = iRow;
	      elementU[put] = value;;
	      double absValue = fabs ( value );

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
#ifdef ZEROFAULT
	  // doesn't matter reading uninitialized but annoys checking
	  if ( i + 1 < endColumn ) {
#endif
	    nextIRow = indexRowU[i + 1];
	    nextValue = elementU[i + 1];
#ifdef ZEROFAULT
	  }
#endif
	  if ( iRow != otherRow ) {
	    //keep
	    indexRowU[put] = iRow;
	    elementU[put] = value;;
	    double absValue = fabs ( value );

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
      indexRowU[startColumn] = pivotRow;
      //clean up counts
      startColumn++;
      numberInColumn[iColumn] = put - startColumn;
      numberInColumnPlus[iColumn]++;
      startColumnU[iColumn]++;
      otherElement = otherElement - thisPivotValue * otherMultiplier;
      double absValue = fabs ( otherElement );

      if ( absValue > zeroTolerance_ ) {
	if ( !foundOther ) {
	  //have we space
	  saveColumn[numberAdded++] = iColumn;
	  int next = nextColumn[iColumn];
	  CoinBigIndex space;

	  space = startColumnU[next] - put - numberInColumnPlus[next];
	  if ( space <= 0 ) {
	    //getColumnSpace also moves fixed part
	    int number = numberInColumn[iColumn];

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
	  end--;
	  indexColumnU[where] = indexColumnU[end];
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
      }
      //linked list for column
      if ( nextCount[iColumn + numberRows_] != -2 ) {
	//modify linked list
	deleteLink ( iColumn + numberRows_ );
	addLink ( iColumn + numberRows_, numberInColumn[iColumn] );
      }
    }
  }
  //get space for row list
  next = nextRow[otherRow];
  CoinBigIndex space;

  space = startRowU[next] - end;
  totalElements_ += numberAdded - numberDeleted;
  int number = numberAdded + ( end - start );

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
    deleteLink ( otherRow );
    addLink ( otherRow, number );
  }
  for ( j = 0; j < numberAdded; j++ ) {
    indexColumnU[end++] = saveColumn[j];
  }
  //modify linked list for pivots
  deleteLink ( pivotRow );
  deleteLink ( pivotColumn + numberRows_ );
  return true;
}
void 
CoinFactorization::setPersistenceFlag(int flag)
{ 
  persistenceFlag_=flag;
  workArea_.setPersistence(flag,maximumRowsExtra_+1);
  workArea2_.setPersistence(flag,maximumRowsExtra_+1);
  pivotColumn_.setPersistence(flag,maximumColumnsExtra_+1);
  permute_.setPersistence(flag,maximumRowsExtra_+1);
  pivotColumnBack_.setPersistence(flag,maximumRowsExtra_+1);
  permuteBack_.setPersistence(flag,maximumRowsExtra_+1);
  nextRow_.setPersistence(flag,maximumRowsExtra_+1);
  startRowU_.setPersistence(flag,maximumRowsExtra_+1);
  numberInRow_.setPersistence(flag,maximumRowsExtra_+1);
  numberInColumn_.setPersistence(flag,maximumColumnsExtra_+1);
  numberInColumnPlus_.setPersistence(flag,maximumColumnsExtra_+1);
  firstCount_.setPersistence(flag,CoinMax(biggerDimension_+2,maximumRowsExtra_+1));
  nextCount_.setPersistence(flag,numberRows_+numberColumns_);
  lastCount_.setPersistence(flag,numberRows_+numberColumns_);
  nextColumn_.setPersistence(flag,maximumColumnsExtra_+1);
  lastColumn_.setPersistence(flag,maximumColumnsExtra_+1);
  lastRow_.setPersistence(flag,maximumRowsExtra_+1);
  markRow_.setPersistence(flag,numberRows_);
  saveColumn_.setPersistence(flag,numberColumns_);
  indexColumnU_.setPersistence(flag,lengthAreaU_);
  pivotRowL_.setPersistence(flag,numberRows_+1);
  pivotRegion_.setPersistence(flag,maximumRowsExtra_+1);
  elementU_.setPersistence(flag,lengthAreaU_);
  indexRowU_.setPersistence(flag,lengthAreaU_);
  startColumnU_.setPersistence(flag,maximumColumnsExtra_+1);
  convertRowToColumnU_.setPersistence( flag, lengthAreaU_ );
  elementL_.setPersistence( flag, lengthAreaL_ );
  indexRowL_.setPersistence( flag, lengthAreaL_ );
  startColumnL_.setPersistence( flag, numberRows_ + 1 );
  startColumnR_.setPersistence( flag,  maximumPivots_ + 1 + maximumColumnsExtra_ + 1 );
  startRowL_.setPersistence( flag,0 );
  indexColumnL_.setPersistence( flag, 0 );
  elementByRowL_.setPersistence( flag, 0 );
  sparse_.setPersistence( flag, 0 );
}
// Delete all stuff
void 
CoinFactorization::almostDestructor()
{
  gutsOfDestructor(2);
}
