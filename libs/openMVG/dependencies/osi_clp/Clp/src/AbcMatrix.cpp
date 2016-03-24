/* $Id$ */
// Copyright (C) 2002, International Business Machines
// Corporation and others, Copyright (C) 2012, FasterCoin.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).



#include <cstdio>
#include "CoinPragma.hpp"
#include "CoinIndexedVector.hpp"
#include "CoinHelperFunctions.hpp"
#include "CoinAbcHelperFunctions.hpp"
#include "AbcSimplexFactorization.hpp"
#include "AbcPrimalColumnDantzig.hpp"
#include "AbcPrimalColumnSteepest.hpp"
#include "CoinTime.hpp"

#include "AbcSimplex.hpp"
#include "AbcSimplexDual.hpp"
// at end to get min/max!
#include "AbcMatrix.hpp"
#include "ClpMessage.hpp"
#ifdef INTEL_MKL
#include "mkl_spblas.h"
#endif
#if ABC_INSTRUMENT>1
extern int abcPricing[20];
extern int abcPricingDense[20];
#endif
//=============================================================================

//#############################################################################
// Constructors / Destructor / Assignment
//#############################################################################

//-------------------------------------------------------------------
// Default Constructor
//-------------------------------------------------------------------
AbcMatrix::AbcMatrix ()
  : matrix_(NULL),
    model_(NULL),
    rowStart_(NULL),
    element_(NULL),
    column_(NULL),
    numberColumnBlocks_(0),
    numberRowBlocks_(0),
#ifdef COUNT_COPY
    countRealColumn_(NULL),
    countStartLarge_(NULL),
    countRow_(NULL),
    countElement_(NULL),
    smallestCount_(0),
    largestCount_(0),
#endif
    startFraction_(0.0),
    endFraction_(1.0),
    savedBestDj_(0.0),
    originalWanted_(0),
    currentWanted_(0),
    savedBestSequence_(-1),
    minimumObjectsScan_(-1),
    minimumGoodReducedCosts_(-1)
{
}

//-------------------------------------------------------------------
// Copy constructor
//-------------------------------------------------------------------
AbcMatrix::AbcMatrix (const AbcMatrix & rhs)
{
#ifndef COIN_SPARSE_MATRIX
  matrix_ = new CoinPackedMatrix(*(rhs.matrix_), -1, -1);
#else
  matrix_ = new CoinPackedMatrix(*(rhs.matrix_), -0, -0);
#endif
  model_=rhs.model_;
  rowStart_ = NULL;
  element_ = NULL;
  column_ = NULL;
#ifdef COUNT_COPY
  countRealColumn_ = NULL;
  countStartLarge_ = NULL;
  countRow_ = NULL;
  countElement_ = NULL;
#endif
  numberColumnBlocks_ = rhs.numberColumnBlocks_;
  CoinAbcMemcpy(startColumnBlock_,rhs.startColumnBlock_,numberColumnBlocks_+1);
  numberRowBlocks_ = rhs.numberRowBlocks_;
  if (numberRowBlocks_) {
    assert (model_);
    int numberRows=model_->numberRows();
    int numberElements = matrix_->getNumElements();
    memcpy(blockStart_,rhs.blockStart_,sizeof(blockStart_));
    rowStart_=CoinCopyOfArray(rhs.rowStart_,numberRows*(numberRowBlocks_+2));
    element_=CoinCopyOfArray(rhs.element_,numberElements);
    column_=CoinCopyOfArray(rhs.column_,numberElements);
#ifdef COUNT_COPY
    smallestCount_ = rhs.smallestCount_;
    largestCount_ = rhs.largestCount_;
    int numberColumns=model_->numberColumns();
    countRealColumn_=CoinCopyOfArray(rhs.countRealColumn_,numberColumns);
    memcpy(countStart_,rhs.countStart_,reinterpret_cast<char *>(&countRealColumn_)-
	   reinterpret_cast<char *>(countStart_));
    int numberLarge = numberColumns-countStart_[MAX_COUNT];
    countStartLarge_=CoinCopyOfArray(rhs.countStartLarge_,numberLarge+1);
    numberElements=countStartLarge_[numberLarge];
    countElement_=CoinCopyOfArray(rhs.countElement_,numberElements);
    countRow_=CoinCopyOfArray(rhs.countRow_,numberElements);
#endif
  }
}

AbcMatrix::AbcMatrix (const CoinPackedMatrix & rhs)
{
#ifndef COIN_SPARSE_MATRIX
  matrix_ = new CoinPackedMatrix(rhs, -1, -1);
#else
  matrix_ = new CoinPackedMatrix(rhs, -0, -0);
#endif
  matrix_->cleanMatrix();
  model_=NULL;
  rowStart_ = NULL;
  element_ = NULL;
  column_ = NULL;
#ifdef COUNT_COPY
  countRealColumn_ = NULL;
  countStartLarge_ = NULL;
  countRow_ = NULL;
  countElement_ = NULL;
  smallestCount_ = 0;
  largestCount_ = 0;
#endif
  numberColumnBlocks_=1;
  startColumnBlock_[0]=0;
  startColumnBlock_[1]=0;
  numberRowBlocks_ = 0;
  startFraction_ = 0;
  endFraction_ = 1.0;
  savedBestDj_ = 0;
  originalWanted_ = 0;
  currentWanted_ = 0;
  savedBestSequence_ = -1;
  minimumObjectsScan_ = -1;
  minimumGoodReducedCosts_ = -1;
}

//-------------------------------------------------------------------
// Destructor
//-------------------------------------------------------------------
AbcMatrix::~AbcMatrix ()
{
  delete matrix_;
  delete [] rowStart_;
  delete [] element_;
  delete [] column_;
#ifdef COUNT_COPY
  delete [] countRealColumn_;
  delete [] countStartLarge_;
  delete [] countRow_;
  delete [] countElement_;
#endif
}

//----------------------------------------------------------------
// Assignment operator
//-------------------------------------------------------------------
AbcMatrix &
AbcMatrix::operator=(const AbcMatrix& rhs)
{
  if (this != &rhs) {
    delete matrix_;
#ifndef COIN_SPARSE_MATRIX
    matrix_ = new CoinPackedMatrix(*(rhs.matrix_));
#else
    matrix_ = new CoinPackedMatrix(*(rhs.matrix_), -0, -0);
#endif
    model_=rhs.model_;
    delete [] rowStart_;
    delete [] element_;
    delete [] column_;
#ifdef COUNT_COPY
    delete [] countRealColumn_;
    delete [] countStartLarge_;
    delete [] countRow_;
    delete [] countElement_;
#endif
    rowStart_ = NULL;
    element_ = NULL;
    column_ = NULL;
#ifdef COUNT_COPY
    countRealColumn_ = NULL;
    countStartLarge_ = NULL;
    countRow_ = NULL;
    countElement_ = NULL;
#endif
    numberColumnBlocks_ = rhs.numberColumnBlocks_;
    CoinAbcMemcpy(startColumnBlock_,rhs.startColumnBlock_,numberColumnBlocks_+1);
    numberRowBlocks_ = rhs.numberRowBlocks_;
    if (numberRowBlocks_) {
      assert (model_);
      int numberRows=model_->numberRows();
      int numberElements = matrix_->getNumElements();
      memcpy(blockStart_,rhs.blockStart_,sizeof(blockStart_));
      rowStart_=CoinCopyOfArray(rhs.rowStart_,numberRows*(numberRowBlocks_+2));
      element_=CoinCopyOfArray(rhs.element_,numberElements);
      column_=CoinCopyOfArray(rhs.column_,numberElements);
#ifdef COUNT_COPY
      smallestCount_ = rhs.smallestCount_;
      largestCount_ = rhs.largestCount_;
      int numberColumns=model_->numberColumns();
      countRealColumn_=CoinCopyOfArray(rhs.countRealColumn_,numberColumns);
      memcpy(countStart_,rhs.countStart_,reinterpret_cast<char *>(&countRealColumn_)-
	     reinterpret_cast<char *>(countStart_));
      int numberLarge = numberColumns-countStart_[MAX_COUNT];
      countStartLarge_=CoinCopyOfArray(rhs.countStartLarge_,numberLarge+1);
      numberElements=countStartLarge_[numberLarge];
      countElement_=CoinCopyOfArray(rhs.countElement_,numberElements);
      countRow_=CoinCopyOfArray(rhs.countRow_,numberElements);
#endif
    }
    startFraction_ = rhs.startFraction_;
    endFraction_ = rhs.endFraction_;
    savedBestDj_ = rhs.savedBestDj_;
    originalWanted_ = rhs.originalWanted_;
    currentWanted_ = rhs.currentWanted_;
    savedBestSequence_ = rhs.savedBestSequence_;
    minimumObjectsScan_ = rhs.minimumObjectsScan_;
    minimumGoodReducedCosts_ = rhs.minimumGoodReducedCosts_;
  }
  return *this;
}
// Sets model
void 
AbcMatrix::setModel(AbcSimplex * model)
{
  model_=model;
  int numberColumns=model_->numberColumns();
  bool needExtension=numberColumns>matrix_->getNumCols();
  if (needExtension) {
    CoinBigIndex lastElement = matrix_->getNumElements();
    matrix_->reserve(numberColumns,lastElement,true);
    CoinBigIndex * columnStart = matrix_->getMutableVectorStarts();
    for (int i=numberColumns;i>=0;i--) {
      if (columnStart[i]==0)
	columnStart[i]=lastElement;
      else
	break;
    }
    assert (lastElement==columnStart[numberColumns]);
  }
}
/* Returns a new matrix in reverse order without gaps */
CoinPackedMatrix *
AbcMatrix::reverseOrderedCopy() const
{
  CoinPackedMatrix * matrix = new CoinPackedMatrix();
  matrix->setExtraGap(0.0);
  matrix->setExtraMajor(0.0);
  matrix->reverseOrderedCopyOf(*matrix_);
  return matrix;
}
/// returns number of elements in column part of basis,
CoinBigIndex
AbcMatrix::countBasis( const int * whichColumn,
		       int & numberColumnBasic)
{
  const int *  COIN_RESTRICT columnLength = matrix_->getVectorLengths();
  CoinBigIndex numberElements = 0;
  int numberRows=model_->numberRows();
  // just count - can be over so ignore zero problem
  for (int i = 0; i < numberColumnBasic; i++) {
    int iColumn = whichColumn[i]-numberRows;
    numberElements += columnLength[iColumn];
  }
  return numberElements;
}
void
AbcMatrix::fillBasis(const int * COIN_RESTRICT whichColumn,
		     int & numberColumnBasic,
		     int * COIN_RESTRICT indexRowU,
		     int * COIN_RESTRICT start,
		     int * COIN_RESTRICT rowCount,
		     int * COIN_RESTRICT columnCount,
		     CoinFactorizationDouble * COIN_RESTRICT elementU)
{
  const int * COIN_RESTRICT columnLength = matrix_->getVectorLengths();
  CoinBigIndex numberElements = start[0];
  // fill
  const CoinBigIndex * COIN_RESTRICT columnStart = matrix_->getVectorStarts();
  const int * COIN_RESTRICT row = matrix_->getIndices();
  const double * COIN_RESTRICT elementByColumn = matrix_->getElements();
  int numberRows=model_->numberRows();
  for (int i = 0; i < numberColumnBasic; i++) {
    int iColumn = whichColumn[i]-numberRows;
    int length = columnLength[iColumn];
    CoinBigIndex startThis = columnStart[iColumn];
    columnCount[i] = length;
    CoinBigIndex endThis = startThis + length;
    for (CoinBigIndex j = startThis; j < endThis; j++) {
      int iRow = row[j];
      indexRowU[numberElements] = iRow;
      rowCount[iRow]++;
      assert (elementByColumn[j]);
      elementU[numberElements++] = elementByColumn[j];
    }
    start[i+1] = numberElements;
  }
}
#ifdef ABC_LONG_FACTORIZATION
void
AbcMatrix::fillBasis(const int * COIN_RESTRICT whichColumn,
		     int & numberColumnBasic,
		     int * COIN_RESTRICT indexRowU,
		     int * COIN_RESTRICT start,
		     int * COIN_RESTRICT rowCount,
		     int * COIN_RESTRICT columnCount,
		     long double * COIN_RESTRICT elementU)
{
  const int * COIN_RESTRICT columnLength = matrix_->getVectorLengths();
  CoinBigIndex numberElements = start[0];
  // fill
  const CoinBigIndex * COIN_RESTRICT columnStart = matrix_->getVectorStarts();
  const int * COIN_RESTRICT row = matrix_->getIndices();
  const double * COIN_RESTRICT elementByColumn = matrix_->getElements();
  int numberRows=model_->numberRows();
  for (int i = 0; i < numberColumnBasic; i++) {
    int iColumn = whichColumn[i]-numberRows;
    int length = columnLength[iColumn];
    CoinBigIndex startThis = columnStart[iColumn];
    columnCount[i] = length;
    CoinBigIndex endThis = startThis + length;
    for (CoinBigIndex j = startThis; j < endThis; j++) {
      int iRow = row[j];
      indexRowU[numberElements] = iRow;
      rowCount[iRow]++;
      assert (elementByColumn[j]);
      elementU[numberElements++] = elementByColumn[j];
    }
    start[i+1] = numberElements;
  }
}
#endif  
#if 0 
/// Move largest in column to beginning
void 
AbcMatrix::moveLargestToStart()
{
  // get matrix data pointers
  int *  COIN_RESTRICT row = matrix_->getMutableIndices();
  const CoinBigIndex *  COIN_RESTRICT columnStart = matrix_->getVectorStarts();
  double *  COIN_RESTRICT elementByColumn = matrix_->getMutableElements();
  int numberColumns=model_->numberColumns();
  CoinBigIndex start = 0;
  for (int iColumn = 0; iColumn < numberColumns; iColumn++) {
    CoinBigIndex end = columnStart[iColumn+1];
    double largest=0.0;
    int position=-1;
    for (CoinBigIndex j = start; j < end; j++) {
      double value = fabs(elementByColumn[j]);
      if (value>largest) {
	largest=value;
	position=j;
      }
    }
    assert (position>=0); // ? empty column
    if (position>start) {
      double value=elementByColumn[start];
      elementByColumn[start]=elementByColumn[position];
      elementByColumn[position]=value;
      int iRow=row[start];
      row[start]=row[position];
      row[position]=iRow;
    }
    start=end;
  }
}
#endif
// Creates row copy
void 
AbcMatrix::createRowCopy()
{
#if ABC_PARALLEL
  if (model_->parallelMode()==0)
#endif
    numberRowBlocks_=1;
#if ABC_PARALLEL
  else
    numberRowBlocks_=CoinMin(NUMBER_ROW_BLOCKS,model_->numberCpus());
#endif
  int maximumRows=model_->maximumAbcNumberRows();
  int numberRows=model_->numberRows();
  int numberColumns=model_->numberColumns();
  int numberElements = matrix_->getNumElements();
  assert (!rowStart_);
  char * whichBlock_=new char [numberColumns];
  rowStart_=new CoinBigIndex[numberRows*(numberRowBlocks_+2)];
  element_ = new double [numberElements];
  column_ = new int [numberElements];
  const CoinBigIndex *  COIN_RESTRICT columnStart = matrix_->getVectorStarts();
  memset(blockStart_,0,sizeof(blockStart_));
  int ecount[10];
  assert (numberRowBlocks_<16);
  CoinAbcMemset0(ecount,10);
  // allocate to blocks (put a bit less in first as will be dealing with slacks) LATER
  CoinBigIndex start=0;
  int block=0;
  CoinBigIndex work=(2*numberColumns+matrix_->getNumElements()+numberRowBlocks_-1)/numberRowBlocks_;
  CoinBigIndex thisWork=work;
  for (int iColumn=0;iColumn<numberColumns;iColumn++) {
    CoinBigIndex end=columnStart[iColumn+1];
    assert (block>=0&&block<numberRowBlocks_);
    whichBlock_[iColumn]=static_cast<char>(block);
    thisWork -= 2+end-start;
    ecount[block]+=end-start;
    start=end;
    blockStart_[block]++;
    if (thisWork<=0) {
      block++;
      thisWork=work;
    }
  }
#if 0
  printf("Blocks ");
  for (int i=0;i<numberRowBlocks_;i++)
    printf("(%d %d) ",blockStart_[i],ecount[i]);
  printf("\n");
#endif
  start=0;
  for (int i=0;i<numberRowBlocks_;i++) {
    int next=start+blockStart_[i];
    blockStart_[i]=start;
    start=next;
  }
  blockStart_[numberRowBlocks_]=start;
  assert (start==numberColumns);
  const int *  COIN_RESTRICT row = matrix_->getIndices();
  const double * COIN_RESTRICT elementByColumn = matrix_->getElements();
  // counts
  CoinAbcMemset0(rowStart_,numberRows*(numberRowBlocks_+2));
  int * COIN_RESTRICT last = rowStart_+numberRows*(numberRowBlocks_+1);
  for (int iColumn=0;iColumn<numberColumns;iColumn++) {
    int block=whichBlock_[iColumn];
    blockStart_[block]++;
    int base=(block+1)*numberRows;
    for (CoinBigIndex j=columnStart[iColumn];j<columnStart[iColumn+1];j++) {
      int iRow=row[j];
      rowStart_[base+iRow]++;
      last[iRow]++;
    }
  }
  // back to real starts
  for (int iBlock=numberRowBlocks_-1;iBlock>=0;iBlock--) 
    blockStart_[iBlock+1]=blockStart_[iBlock];
  blockStart_[0]=0;
  CoinBigIndex put=0;
  for (int iRow=1;iRow<numberRows;iRow++) {
    int n=last[iRow-1];
    last[iRow-1]=put;
    put += n;
    rowStart_[iRow]=put;
  }
  int n=last[numberRows-1];
  last[numberRows-1]=put;
  put += n;
  assert (put==numberElements);
  //last[numberRows-1]=put;
  // starts
  int base=0;
  for (int iBlock=0;iBlock<numberRowBlocks_;iBlock++) {
    for (int iRow=0;iRow<numberRows;iRow++) {
      rowStart_[base+numberRows+iRow]+=rowStart_[base+iRow];
    }
    base+= numberRows;
  }
  for (int iColumn=0;iColumn<numberColumns;iColumn++) {
    int block=whichBlock_[iColumn];
    int where=blockStart_[block];
    blockStart_[block]++;
    int base=block*numberRows;
    for (CoinBigIndex j=columnStart[iColumn];j<columnStart[iColumn+1];j++) {
      int iRow=row[j];
      CoinBigIndex put=rowStart_[base+iRow];
      rowStart_[base+iRow]++;
      column_[put]=where+maximumRows;
      element_[put]=elementByColumn[j];
    }
  }
  // back to real starts etc
  base=numberRows*numberRowBlocks_;
  for (int iBlock=numberRowBlocks_-1;iBlock>=0;iBlock--)  {
    blockStart_[iBlock+1]=blockStart_[iBlock]+maximumRows;
    CoinAbcMemcpy(rowStart_+base,rowStart_+base-numberRows,numberRows);
    base -= numberRows;
  }
  blockStart_[0]=0;//maximumRows;
  delete [] whichBlock_;
  // and move 
  CoinAbcMemcpy(rowStart_,last,numberRows);
  // All in useful
  CoinAbcMemcpy(rowStart_+(numberRowBlocks_+1)*numberRows,
		rowStart_+(numberRowBlocks_)*numberRows,numberRows);
#ifdef COUNT_COPY
  // now blocked by element count
  countRealColumn_=new int [numberColumns];
  int counts [2*MAX_COUNT];
  memset(counts,0,sizeof(counts));
  //memset(countFirst_,0,sizeof(countFirst_));
  int numberLarge=0;
  for (int iColumn=0;iColumn<numberColumns;iColumn++) {
    int n=columnStart[iColumn+1]-columnStart[iColumn];
    if (n<MAX_COUNT)
      counts[n]++;
    else
      numberLarge++;
  }
  CoinBigIndex numberExtra=3; // for alignment
#define SMALL_COUNT 1
  for (int i=0;i<MAX_COUNT;i++) {
    int n=counts[i];
    if (n>=SMALL_COUNT) {
      n &= 3;
      int extra=(4-n)&3;
      numberExtra+= i*extra;
    } else {
      // treat as large
      numberLarge+=n;
    }
  }
  countElement_= new double [numberElements+numberExtra];
  countRow_=new int [numberElements+numberExtra];
  countStartLarge_ = new CoinBigIndex [numberLarge+1];
  countStartLarge_[numberLarge]=numberElements+numberExtra;
  //return;
  CoinInt64 xx = reinterpret_cast<CoinInt64>(countElement_);
  int iBottom = static_cast<int>(xx & 31);
  int offset = iBottom>>3;
  CoinBigIndex firstElementLarge=0;
  if (offset)
    firstElementLarge=4-offset;
  //countStart_[0]=firstElementLarge;
  int positionLarge=0;
  smallestCount_=0;
  largestCount_=0;
  for (int i=0;i<MAX_COUNT;i++) {
    int n=counts[i];
    countFirst_[i]=positionLarge;
    countStart_[i]=firstElementLarge;
    if (n>=SMALL_COUNT) {
      counts[i+MAX_COUNT]=1;
      if (smallestCount_==0)
	smallestCount_=i;
      largestCount_=i;
      positionLarge+=n;
      firstElementLarge+=n*i;
      n &= 3;
      int extra=(4-n)&3;
      firstElementLarge+= i*extra;
    }
    counts[i]=0;
  }
  largestCount_++;
  countFirst_[MAX_COUNT]=positionLarge;
  countStart_[MAX_COUNT]=firstElementLarge;
  numberLarge=0;
  for (int iColumn=0;iColumn<numberColumns;iColumn++) {
    CoinBigIndex start=columnStart[iColumn];
    int n=columnStart[iColumn+1]-start;
    CoinBigIndex put;
    int position;
    if (n<MAX_COUNT&&counts[MAX_COUNT+n]!=0) {
      int iWhich=counts[n];
      position = countFirst_[n]+iWhich;
      int iBlock4=iWhich&(~3);
      iWhich &= 3;
      put = countStart_[n]+iBlock4*n;
      put += iWhich;
      counts[n]++;
      for (int i=0;i<n;i++) {
	countRow_[put]=row[start+i];
	countElement_[put]=elementByColumn[start+i];
	put+=4;
      }
    } else {
      position = positionLarge+numberLarge;
      put=firstElementLarge;
      countStartLarge_[numberLarge]=put;
      firstElementLarge+=n;
      numberLarge++;
      CoinAbcMemcpy(countRow_+put,row+start,n);
      CoinAbcMemcpy(countElement_+put,elementByColumn+start,n);
    }
    countRealColumn_[position]=iColumn+maximumRows;
  }
  countStartLarge_[numberLarge]=firstElementLarge;
  assert (firstElementLarge<=numberElements+numberExtra);
#endif
}
// Make all useful
void 
AbcMatrix::makeAllUseful(CoinIndexedVector & /*spare*/)
{
  int numberRows=model_->numberRows();
  CoinBigIndex * COIN_RESTRICT rowEnd = rowStart_+numberRows;
  const CoinBigIndex * COIN_RESTRICT rowReallyEnd = rowStart_+2*numberRows;
  for (int iRow=0;iRow<numberRows;iRow++) {
    rowEnd[iRow]=rowReallyEnd[iRow];
  }
}
#define SQRT_ARRAY
// Creates scales for column copy (rowCopy in model may be modified)
void
AbcMatrix::scale(int numberAlreadyScaled)
{
  int numberRows=model_->numberRows();
  bool inBranchAndBound=(model_->specialOptions(),0x1000000)!=0;
  bool doScaling=numberAlreadyScaled>=0;
  if (!doScaling)
    numberAlreadyScaled=0;
  if (numberAlreadyScaled==numberRows)
    return; // no need to do anything
  int numberColumns=model_->numberColumns();
  double *  COIN_RESTRICT rowScale=model_->rowScale2();
  double *  COIN_RESTRICT inverseRowScale=model_->inverseRowScale2();
  double *  COIN_RESTRICT columnScale=model_->columnScale2();
  double *  COIN_RESTRICT inverseColumnScale=model_->inverseColumnScale2();
  // we are going to mark bits we are interested in
  int whichArray=model_->getAvailableArrayPublic();
  char *  COIN_RESTRICT usefulColumn = reinterpret_cast<char *>(model_->usefulArray(whichArray)->getIndices());
  memset(usefulColumn,1,numberColumns);
  const double *  COIN_RESTRICT rowLower = model_->rowLower();
  const double *  COIN_RESTRICT rowUpper = model_->rowUpper();
  const double *  COIN_RESTRICT columnLower = model_->columnLower();
  const double *  COIN_RESTRICT columnUpper = model_->columnUpper();
  //#define LEAVE_FIXED
  // mark empty and fixed columns
  // get matrix data pointers
  const int *  COIN_RESTRICT row = matrix_->getIndices();
  const CoinBigIndex *  COIN_RESTRICT columnStart = matrix_->getVectorStarts();
  double *  COIN_RESTRICT elementByColumn = matrix_->getMutableElements();
  CoinPackedMatrix *  COIN_RESTRICT rowCopy = reverseOrderedCopy();
  const int * column = rowCopy->getIndices();
  const CoinBigIndex *  COIN_RESTRICT rowStart = rowCopy->getVectorStarts();
  const double *  COIN_RESTRICT element = rowCopy->getElements();
  assert (numberAlreadyScaled>=0&&numberAlreadyScaled<numberRows);
#ifndef LEAVE_FIXED
  for (int iColumn = 0; iColumn < numberColumns; iColumn++) {
    if (columnUpper[iColumn] ==	columnLower[iColumn])
      usefulColumn[iColumn]=0;
  }
#endif
  double overallLargest = -1.0e-20;
  double overallSmallest = 1.0e20;
  if (!numberAlreadyScaled) {
    // may be redundant
    CoinFillN(rowScale,numberRows,1.0);
    CoinFillN(columnScale,numberColumns,1.0);
    CoinBigIndex start = 0;
    for (int iColumn = 0; iColumn < numberColumns; iColumn++) {
      CoinBigIndex end = columnStart[iColumn+1];
      if (usefulColumn[iColumn]) {
	if (end>start) {
	  for (CoinBigIndex j = start; j < end; j++) {
	    double value = fabs(elementByColumn[j]);
	    overallLargest = CoinMax(overallLargest, value);
	    overallSmallest = CoinMin(overallSmallest, value);
	  }
	} else {
	  usefulColumn[iColumn]=0;
	}
      }
      start=end;
    }
  } else {
    CoinBigIndex start = rowStart[numberAlreadyScaled];
    for (int iRow = numberAlreadyScaled; iRow < numberRows; iRow++) {
      rowScale[iRow]=1.0;
      CoinBigIndex end = rowStart[iRow+1];
      for (CoinBigIndex j = start; j < end; j++) {
	int iColumn=column[j];
	if (usefulColumn[iColumn]) {
	  double value = fabs(elementByColumn[j])*columnScale[iColumn];
	  overallLargest = CoinMax(overallLargest, value);
	  overallSmallest = CoinMin(overallSmallest, value);
	}
      }
    }
  }
  if ((overallSmallest >= 0.5 && overallLargest <= 2.0)||!doScaling) {
    //printf("no scaling\n");
    delete rowCopy;
    model_->clearArraysPublic(whichArray);
    CoinFillN(inverseRowScale+numberAlreadyScaled,numberRows-numberAlreadyScaled,1.0);
    if (!numberAlreadyScaled) 
      CoinFillN(inverseColumnScale,numberColumns,1.0);
    //moveLargestToStart();
    return ;
  }
  // need to scale
  double largest;
  double smallest;
  int scalingMethod=model_->scalingFlag();
  if (scalingMethod == 4) {
    // As auto
    scalingMethod = 3;
  } else if (scalingMethod == 5) {
    // As geometric
    scalingMethod = 2;
  }
  double savedOverallRatio = 0.0;
  double tolerance = 5.0 * model_->primalTolerance();
  bool finished = false;
  // if scalingMethod 3 then may change
  bool extraDetails = (model_->logLevel() > 2);
  bool secondTime=false;
  while (!finished) {
    int numberPass = !numberAlreadyScaled ? 3 : 1;
    overallLargest = -1.0e-20;
    overallSmallest = 1.0e20;
    if (!secondTime) {
      secondTime=true;
    } else {
      CoinFillN ( rowScale, numberRows, 1.0);
      CoinFillN ( columnScale, numberColumns, 1.0);
    } 
    if (scalingMethod == 1 || scalingMethod == 3) {
      // Maximum in each row
      for (int iRow = numberAlreadyScaled; iRow < numberRows; iRow++) {
	largest = 1.0e-10;
	for (CoinBigIndex j = rowStart[iRow]; j < rowStart[iRow+1]; j++) {
	  int iColumn = column[j];
	  if (usefulColumn[iColumn]) {
	    double value = fabs(element[j]);
	    largest = CoinMax(largest, value);
	    assert (largest < 1.0e40);
	  }
	}
	rowScale[iRow] = 1.0 / largest;
#ifdef COIN_DEVELOP
	if (extraDetails) {
	  overallLargest = CoinMax(overallLargest, largest);
	  overallSmallest = CoinMin(overallSmallest, largest);
	}
#endif
      }
    } else {
      while (numberPass) {
	overallLargest = 0.0;
	overallSmallest = 1.0e50;
	numberPass--;
	// Geometric mean on row scales
	for (int iRow = numberAlreadyScaled; iRow < numberRows; iRow++) {
	  largest = 1.0e-50;
	  smallest = 1.0e50;
	  for (CoinBigIndex j = rowStart[iRow]; j < rowStart[iRow+1]; j++) {
	    int iColumn = column[j];
	    if (usefulColumn[iColumn]) {
	      double value = fabs(element[j]);
	      value *= columnScale[iColumn];
	      largest = CoinMax(largest, value);
	      smallest = CoinMin(smallest, value);
	    }
	  }
#ifdef SQRT_ARRAY
	  rowScale[iRow] = smallest * largest;
#else
	  rowScale[iRow] = 1.0 / sqrt(smallest * largest);
#endif
	  if (extraDetails) {
	    overallLargest = CoinMax(largest * rowScale[iRow], overallLargest);
	    overallSmallest = CoinMin(smallest * rowScale[iRow], overallSmallest);
	  }
	}
	if (model_->scalingFlag() == 5)
	  break; // just scale rows
#ifdef SQRT_ARRAY
	CoinAbcInverseSqrts(rowScale, numberRows);
#endif
	if (!inBranchAndBound)
	model_->messageHandler()->message(CLP_PACKEDSCALE_WHILE, *model_->messagesPointer())
	  << overallSmallest
	  << overallLargest
	  << CoinMessageEol;
	// skip last column round
	if (numberPass == 1)
	  break;
	// Geometric mean on column scales
	for (int iColumn = 0; iColumn < numberColumns; iColumn++) {
	  if (usefulColumn[iColumn]) {
	    largest = 1.0e-50;
	    smallest = 1.0e50;
	    for (CoinBigIndex j = columnStart[iColumn];
		 j < columnStart[iColumn+1]; j++) {
	      int iRow = row[j];
	      double value = fabs(elementByColumn[j]);
	      value *= rowScale[iRow];
	      largest = CoinMax(largest, value);
	      smallest = CoinMin(smallest, value);
	    }
#ifdef SQRT_ARRAY
	    columnScale[iColumn] = smallest * largest;
#else
	    columnScale[iColumn] = 1.0 / sqrt(smallest * largest);
#endif
	  }
	}
#ifdef SQRT_ARRAY
	CoinAbcInverseSqrts(columnScale, numberColumns);
#endif
      }
    }
    // If ranges will make horrid then scale
    for (int iRow = numberAlreadyScaled; iRow < numberRows; iRow++) {
      double difference = rowUpper[iRow] - rowLower[iRow];
      double scaledDifference = difference * rowScale[iRow];
      if (scaledDifference > tolerance && scaledDifference < 1.0e-4) {
	// make gap larger
	rowScale[iRow] *= 1.0e-4 / scaledDifference;
	rowScale[iRow] = CoinMax(1.0e-10, CoinMin(1.0e10, rowScale[iRow]));
	//printf("Row %d difference %g scaled diff %g => %g\n",iRow,difference,
	// scaledDifference,difference*rowScale[iRow]);
      }
    }
    // final pass to scale columns so largest is reasonable
    // See what smallest will be if largest is 1.0
    if (model_->scalingFlag() != 5) {
      overallSmallest = 1.0e50;
      for (int iColumn = 0; iColumn < numberColumns; iColumn++) {
	if (usefulColumn[iColumn]) {
	  largest = 1.0e-20;
	  smallest = 1.0e50;
	  for (CoinBigIndex j = columnStart[iColumn];j < columnStart[iColumn+1]; j++) {
	    int iRow = row[j];
	    double value = fabs(elementByColumn[j] * rowScale[iRow]);
	    largest = CoinMax(largest, value);
	    smallest = CoinMin(smallest, value);
	  }
	  if (overallSmallest * largest > smallest)
	    overallSmallest = smallest / largest;
	}
      }
    }
    if (scalingMethod == 1 || scalingMethod == 2) {
      finished = true;
    } else if (savedOverallRatio == 0.0 && scalingMethod != 4) {
      savedOverallRatio = overallSmallest;
      scalingMethod = 4;
    } else {
      assert (scalingMethod == 4);
      if (overallSmallest > 2.0 * savedOverallRatio) {
	finished = true; // geometric was better
	if (model_->scalingFlag() == 4) {
	  // If in Branch and bound change
	  if ((model_->specialOptions() & 1024) != 0) {
	    model_->scaling(2);
	  }
	}
      } else {
	scalingMethod = 1; // redo equilibrium
	if (model_->scalingFlag() == 4) {
	  // If in Branch and bound change
	  if ((model_->specialOptions() & 1024) != 0) {
	    model_->scaling(1);
	  }
	}
      }
#if 0
      if (extraDetails) {
	if (finished)
	  printf("equilibrium ratio %g, geometric ratio %g , geo chosen\n",
		 savedOverallRatio, overallSmallest);
	else
	  printf("equilibrium ratio %g, geometric ratio %g , equi chosen\n",
		 savedOverallRatio, overallSmallest);
      }
#endif
    }
  }
  //#define RANDOMIZE
#ifdef RANDOMIZE
  // randomize by up to 10%
  for (int iRow = numberAlreadyScaled; iRow < numberRows; iRow++) {
    double value = 0.5 - randomNumberGenerator_.randomDouble(); //between -0.5 to + 0.5
    rowScale[iRow] *= (1.0 + 0.1 * value);
  }
#endif
  overallLargest = 1.0;
  if (overallSmallest < 1.0e-1)
    overallLargest = 1.0 / sqrt(overallSmallest);
  overallLargest = CoinMin(100.0, overallLargest);
  overallSmallest = 1.0e50;
  char *  COIN_RESTRICT usedRow = reinterpret_cast<char *>(inverseRowScale);
  memset(usedRow, 0, numberRows);
  //printf("scaling %d\n",model_->scalingFlag());
  if (model_->scalingFlag() != 5) {
    for (int iColumn = 0; iColumn < numberColumns; iColumn++) {
      if (usefulColumn[iColumn]) {
	largest = 1.0e-20;
	smallest = 1.0e50;
	for (CoinBigIndex j = columnStart[iColumn];j < columnStart[iColumn+1]; j++) {
	  int iRow = row[j];
	  usedRow[iRow] = 1;
	  double value = fabs(elementByColumn[j] * rowScale[iRow]);
	  largest = CoinMax(largest, value);
	  smallest = CoinMin(smallest, value);
	}
	columnScale[iColumn] = overallLargest / largest;
	//columnScale[iColumn]=CoinMax(1.0e-10,CoinMin(1.0e10,columnScale[iColumn]));
#ifdef RANDOMIZE
	if (!numberAlreadyScaled) {
	  double value = 0.5 - randomNumberGenerator_.randomDouble(); //between -0.5 to + 0.5
	  columnScale[iColumn] *= (1.0 + 0.1 * value);
	}
#endif
	double difference = columnUpper[iColumn] - columnLower[iColumn];
	if (difference < 1.0e-5 * columnScale[iColumn]) {
	  // make gap larger
	  columnScale[iColumn] = difference / 1.0e-5;
	  //printf("Column %d difference %g scaled diff %g => %g\n",iColumn,difference,
	  // scaledDifference,difference*columnScale[iColumn]);
	}
	double value = smallest * columnScale[iColumn];
	if (overallSmallest > value)
	  overallSmallest = value;
	//overallSmallest = CoinMin(overallSmallest,smallest*columnScale[iColumn]);
      } else {
	assert(columnScale[iColumn] == 1.0);
	//columnScale[iColumn]=1.0;
      }
    }
    for (int iRow = numberAlreadyScaled; iRow < numberRows; iRow++) {
      if (!usedRow[iRow]) {
	rowScale[iRow] = 1.0;
      }
    }
  }
  if (!inBranchAndBound)
  model_->messageHandler()->message(CLP_PACKEDSCALE_FINAL, *model_->messagesPointer())
    << overallSmallest
    << overallLargest
    << CoinMessageEol;
  if (overallSmallest < 1.0e-13) {
    // Change factorization zero tolerance
    double newTolerance = CoinMax(1.0e-15 * (overallSmallest / 1.0e-13),
				  1.0e-18);
    if (model_->factorization()->zeroTolerance() > newTolerance)
      model_->factorization()->zeroTolerance(newTolerance);
    newTolerance = CoinMax(overallSmallest * 0.5, 1.0e-18);
    model_->setZeroTolerance(newTolerance);
#ifndef NDEBUG
    assert (newTolerance<0.0); // just so we can fix
#endif
  }
  // make copy (could do faster by using previous values)
  // could just do partial
  CoinAbcReciprocal(inverseRowScale+numberAlreadyScaled,numberRows-numberAlreadyScaled,
		rowScale+numberAlreadyScaled);
  if (!numberAlreadyScaled) 
    CoinAbcReciprocal(inverseColumnScale,numberColumns,columnScale);
  // Do scaled copy //NO and move largest to start
  CoinBigIndex start = 0;
  for (int iColumn = 0; iColumn < numberColumns; iColumn++) {
    double scale=columnScale[iColumn];
    CoinBigIndex end = columnStart[iColumn+1];
    for (CoinBigIndex j = start; j < end; j++) {
      double value=elementByColumn[j];
      int iRow = row[j];
      if (iRow>=numberAlreadyScaled) {
	value*= scale*rowScale[iRow];
	elementByColumn[j]= value;
      }
    }
    start=end;
  }
  delete rowCopy;
#if 0
  if (model_->rowCopy()) {
    // need to replace row by row
    CoinPackedMatrix * rowCopy = NULL;
    //static_cast< AbcMatrix*>(model_->rowCopy());
    double * element = rowCopy->getMutableElements();
    const int * column = rowCopy->getIndices();
    const CoinBigIndex * rowStart = rowCopy->getVectorStarts();
    // scale row copy
    for (iRow = 0; iRow < numberRows; iRow++) {
      CoinBigIndex j;
      double scale = rowScale[iRow];
      double * elementsInThisRow = element + rowStart[iRow];
      const int * columnsInThisRow = column + rowStart[iRow];
      int number = rowStart[iRow+1] - rowStart[iRow];
      assert (number <= numberColumns);
      for (j = 0; j < number; j++) {
	int iColumn = columnsInThisRow[j];
	elementsInThisRow[j] *= scale * columnScale[iColumn];
      }
    }
  }
#endif
  model_->clearArraysPublic(whichArray);
}
/* Return <code>y + A * scalar *x</code> in <code>y</code>.
   @pre <code>x</code> must be of size <code>numColumns()</code>
   @pre <code>y</code> must be of size <code>numRows()</code> */
//scaled versions
void
AbcMatrix::timesModifyExcludingSlacks(double scalar,
				      const double * x, double * y) const
{
  int numberTotal = model_->numberTotal();
  int maximumRows = model_->maximumAbcNumberRows();
  // get matrix data pointers
  const int *  COIN_RESTRICT row = matrix_->getIndices();
  const CoinBigIndex *  COIN_RESTRICT columnStart = matrix_->getVectorStarts()-maximumRows;
  const double *  COIN_RESTRICT elementByColumn = matrix_->getElements();
  for (int iColumn = maximumRows; iColumn < numberTotal; iColumn++) {
    double value = x[iColumn];
    if (value) {
      CoinBigIndex start = columnStart[iColumn];
      CoinBigIndex end = columnStart[iColumn+1];
      value *= scalar;
      for (CoinBigIndex j = start; j < end; j++) {
	int iRow = row[j];
	y[iRow] += value * elementByColumn[j];
      }
    }
  }
}
/* Return <code>y + A * scalar(+-1) *x</code> in <code>y</code>.
   @pre <code>x</code> must be of size <code>numColumns()+numRows()</code>
   @pre <code>y</code> must be of size <code>numRows()</code> */
void 
AbcMatrix::timesModifyIncludingSlacks(double scalar,
				      const double * x, double * y) const
{
  int numberRows=model_->numberRows();
  int numberTotal = model_->numberTotal();
  int maximumRows = model_->maximumAbcNumberRows();
  // makes no sense for x==y??
  assert (x!=y);
  // For now just by column and assumes already scaled (use reallyScale)
  // get matrix data pointers
  const int * COIN_RESTRICT row = matrix_->getIndices();
  const CoinBigIndex * COIN_RESTRICT columnStart = matrix_->getVectorStarts()-maximumRows;
  const double * COIN_RESTRICT elementByColumn = matrix_->getElements();
  if (scalar==1.0) {
    // add
    if (x!=y) {
      for (int i=0;i<numberRows;i++) 
	y[i]+=x[i];
    } else {
      for (int i=0;i<numberRows;i++) 
	y[i]+=y[i];
    }
    for (int iColumn = maximumRows; iColumn < numberTotal; iColumn++) {
      double value = x[iColumn];
      if (value) {
	CoinBigIndex start = columnStart[iColumn];
	CoinBigIndex next = columnStart[iColumn+1];
	for (CoinBigIndex j = start; j < next; j++) {
	  int jRow = row[j];
	  y[jRow]+=value*elementByColumn[j];
	}
      }
    }
  } else {
    // subtract
    if (x!=y) {
      for (int i=0;i<numberRows;i++) 
	y[i]-=x[i];
    } else {
      for (int i=0;i<numberRows;i++) 
	y[i]=0.0;
    }
    for (int iColumn = maximumRows; iColumn < numberTotal; iColumn++) {
      double value = x[iColumn];
      if (value) {
	CoinBigIndex start = columnStart[iColumn];
	CoinBigIndex next = columnStart[iColumn+1];
	for (CoinBigIndex j = start; j < next; j++) {
	  int jRow = row[j];
	  y[jRow]-=value*elementByColumn[j];
	}
      }
    }
  }
}
/* Return A * scalar(+-1) *x</code> in <code>y</code>.
   @pre <code>x</code> must be of size <code>numColumns()+numRows()</code>
   @pre <code>y</code> must be of size <code>numRows()</code> */
void 
AbcMatrix::timesIncludingSlacks(double scalar,
				const double * x, double * y) const
{
  int numberRows=model_->numberRows();
  int numberTotal = model_->numberTotal();
  int maximumRows = model_->maximumAbcNumberRows();
  // For now just by column and assumes already scaled (use reallyScale)
  // get matrix data pointers
  const int * COIN_RESTRICT row = matrix_->getIndices();
  const CoinBigIndex * COIN_RESTRICT columnStart = matrix_->getVectorStarts()-maximumRows;
  const double * COIN_RESTRICT elementByColumn = matrix_->getElements();
  if (scalar==1.0) {
    // add
    if (x!=y) {
      for (int i=0;i<numberRows;i++) 
	y[i]=x[i];
    } 
    for (int iColumn = maximumRows; iColumn < numberTotal; iColumn++) {
      double value = x[iColumn];
      if (value) {
	CoinBigIndex start = columnStart[iColumn];
	CoinBigIndex next = columnStart[iColumn+1];
	for (CoinBigIndex j = start; j < next; j++) {
	  int jRow = row[j];
	  y[jRow]+=value*elementByColumn[j];
	}
      }
    }
  } else {
    // subtract
    if (x!=y) {
      for (int i=0;i<numberRows;i++) 
	y[i]=-x[i];
    } else {
      for (int i=0;i<numberRows;i++) 
	y[i]=-y[i];
    }
    for (int iColumn = maximumRows; iColumn < numberTotal; iColumn++) {
      double value = x[iColumn];
      if (value) {
	CoinBigIndex start = columnStart[iColumn];
	CoinBigIndex next = columnStart[iColumn+1];
	for (CoinBigIndex j = start; j < next; j++) {
	  int jRow = row[j];
	  y[jRow]-=value*elementByColumn[j];
	}
      }
    }
  }
}
extern double parallelDual4(AbcSimplexDual * );
static double firstPass(AbcSimplex * model,int iBlock,
		 double upperTheta, int & freeSequence,
		 CoinPartitionedVector & tableauRow,
		 CoinPartitionedVector & candidateList) 
{
  int *  COIN_RESTRICT index = tableauRow.getIndices();
  double *  COIN_RESTRICT array = tableauRow.denseVector();
  double *  COIN_RESTRICT arrayCandidate=candidateList.denseVector();
  int *  COIN_RESTRICT indexCandidate = candidateList.getIndices();
  const double *  COIN_RESTRICT abcDj = model->djRegion();
  const unsigned char *  COIN_RESTRICT internalStatus = model->internalStatus();
  // do first pass to get possibles
  double bestPossible = 0.0;
  // We can also see if infeasible or pivoting on free
  double tentativeTheta = 1.0e25; // try with this much smaller as guess
  double acceptablePivot = model->currentAcceptablePivot();
  double dualT=-model->currentDualTolerance();
  // fixed will have been taken out by now
  const double multiplier[] = { 1.0, -1.0};
  freeSequence=-1;
  int firstIn=model->abcMatrix()->blockStart(iBlock);
  int numberNonZero=tableauRow.getNumElements(iBlock)+firstIn;
  int numberRemaining=firstIn;
  //first=tableauRow.getNumElements();
  // could pass in last and if numberNonZero==last-firstIn scan as well
  if (model->ordinaryVariables()) {
    for (int i = firstIn; i < numberNonZero; i++) {
      int iSequence = index[i];
      double tableauValue=array[i];
      unsigned char iStatus=internalStatus[iSequence]&7;
      double mult = multiplier[iStatus];
      double alpha = tableauValue * mult;
      double oldValue = abcDj[iSequence] * mult;
      double value = oldValue - tentativeTheta * alpha;
      if (value < dualT) {
	bestPossible = CoinMax(bestPossible, alpha);
	value = oldValue - upperTheta * alpha;
	if (value < dualT && alpha >= acceptablePivot) {
	  upperTheta = (oldValue - dualT) / alpha;
	}
	// add to list
	arrayCandidate[numberRemaining] = alpha;
	indexCandidate[numberRemaining++] = iSequence;
      }
    }
  } else {
    double badFree = 0.0;
    double freeAlpha = model->currentAcceptablePivot();
    int freeSequenceIn=model->freeSequenceIn();
    double currentDualTolerance = model->currentDualTolerance();
    for (int i = firstIn; i < numberNonZero; i++) {
      int iSequence = index[i];
      double tableauValue=array[i];
      unsigned char iStatus=internalStatus[iSequence]&7;
      if ((iStatus&4)==0) {
	double mult = multiplier[iStatus];
	double alpha = tableauValue * mult;
	double oldValue = abcDj[iSequence] * mult;
	double value = oldValue - tentativeTheta * alpha;
	if (value < dualT) {
	  bestPossible = CoinMax(bestPossible, alpha);
	  value = oldValue - upperTheta * alpha;
	  if (value < dualT && alpha >= acceptablePivot) {
	    upperTheta = (oldValue - dualT) / alpha;
	  }
	  // add to list
	  arrayCandidate[numberRemaining] = alpha;
	  indexCandidate[numberRemaining++] = iSequence;
	}
      } else {
	bool keep;
	bestPossible = CoinMax(bestPossible, fabs(tableauValue));
	double oldValue = abcDj[iSequence];
	// If free has to be very large - should come in via dualRow
	//if (getInternalStatus(iSequence+addSequence)==isFree&&fabs(tableauValue)<1.0e-3)
	//break;
	if (oldValue > currentDualTolerance) {
	  keep = true;
	} else if (oldValue < -currentDualTolerance) {
	  keep = true;
	} else {
	  if (fabs(tableauValue) > CoinMax(10.0 * acceptablePivot, 1.0e-5)) {
	    keep = true;
	  } else {
	    keep = false;
	    badFree = CoinMax(badFree, fabs(tableauValue));
	  }
	}
	if (keep) {
#ifdef PAN
	  if (model->fakeSuperBasic(iSequence)>=0) {
#endif
	    if (iSequence==freeSequenceIn)
	      tableauValue=COIN_DBL_MAX;
	    // free - choose largest
	    if (fabs(tableauValue) > fabs(freeAlpha)) {
	      freeAlpha = tableauValue;
	      freeSequence = iSequence;
	    }
#ifdef PAN
	  }
#endif
	}
      }
    }
  }
  //firstInX=numberNonZero-firstIn;
  //lastInX=-1;//numberRemaining-lastInX;
  tableauRow.setNumElementsPartition(iBlock,numberNonZero-firstIn);
  candidateList.setNumElementsPartition(iBlock,numberRemaining-firstIn);
  return upperTheta;
}
// gets sorted tableau row and a possible value of theta
double 
AbcMatrix::dualColumn1Row(int iBlock, double upperTheta, int & freeSequence,
			  const CoinIndexedVector & update,
			  CoinPartitionedVector & tableauRow,
			  CoinPartitionedVector & candidateList) const
{
  int maximumRows = model_->maximumAbcNumberRows();
  int number=update.getNumElements();
  const double *  COIN_RESTRICT pi=update.denseVector();
  const int *  COIN_RESTRICT piIndex = update.getIndices();
  const CoinBigIndex * COIN_RESTRICT rowStart = rowStart_;
  int numberRows=model_->numberRows();
  const CoinBigIndex * COIN_RESTRICT rowEnd = rowStart+numberRows*numberRowBlocks_;
  // count down
  int nColumns;
  int firstIn=blockStart_[iBlock];
  int first=firstIn;
  if (!first)
    first=maximumRows;
  int last=blockStart_[iBlock+1];
  nColumns=last-first;
  int target=nColumns;
  rowStart += iBlock*numberRows;
  rowEnd = rowStart+numberRows;
  for (int i=0;i<number;i++) {
    int iRow=piIndex[i];
    CoinBigIndex end = rowEnd[iRow];
    target -= end-rowStart[iRow];
    if (target<0)
      break;
  }
  if (target>0) {
    //printf("going to few %d ops %d\n",number,nColumns-target);
    return dualColumn1RowFew(iBlock, upperTheta, freeSequence,
			  update, tableauRow, candidateList);
  }
  const unsigned char *  COIN_RESTRICT internalStatus = model_->internalStatus();
  int *  COIN_RESTRICT index = tableauRow.getIndices();
  double *  COIN_RESTRICT array = tableauRow.denseVector();
  const double * COIN_RESTRICT element = element_;
  const int * COIN_RESTRICT column = column_;
  for (int i=0;i<number;i++) {
    int iRow=piIndex[i];
    double piValue = pi[iRow];
    CoinBigIndex end = rowEnd[iRow];
    for (CoinBigIndex j=rowStart[iRow];j<end;j++) {
      int iColumn = column[j];
      assert (iColumn>=first&&iColumn<last);
      double value = element[j];
      array[iColumn]+= piValue*value;
    }
  }
  int numberNonZero;
  int numberRemaining;
  if (iBlock==0) {
#if 1
    upperTheta=static_cast<AbcSimplexDual *>(model_)->dualColumn1A();
    numberNonZero=tableauRow.getNumElements(0);
    numberRemaining=candidateList.getNumElements(0);
#else
    numberNonZero=0;
    for (int i=0;i<number;i++) {
      int iRow=piIndex[i];
      unsigned char type=internalStatus[iRow];
      if ((type&7)<6) {
	index[numberNonZero]=iRow;
	double piValue=pi[iRow];
	array[numberNonZero++]=piValue;
      }
    }
    numberRemaining=0;
#endif
  } else {
    numberNonZero=firstIn;
    numberRemaining=firstIn;
  }
  double *  COIN_RESTRICT arrayCandidate=candidateList.denseVector();
  int *  COIN_RESTRICT indexCandidate = candidateList.getIndices();
  //printf("first %d last %d firstIn %d lastIn %d\n",
  //	 first,last,firstIn,lastIn);
  const double *  COIN_RESTRICT abcDj = model_->djRegion();
  // do first pass to get possibles
  double bestPossible = 0.0;
  // We can also see if infeasible or pivoting on free
  double tentativeTheta = 1.0e25; // try with this much smaller as guess
  double acceptablePivot = model_->currentAcceptablePivot();
  double dualT=-model_->currentDualTolerance();
  const double multiplier[] = { 1.0, -1.0};
  double zeroTolerance = model_->zeroTolerance();
  freeSequence=-1;
  if (model_->ordinaryVariables()) {
    for (int iSequence = first; iSequence < last; iSequence++) {
      double tableauValue=array[iSequence];
      if (tableauValue) {
	array[iSequence]=0.0;
	if (fabs(tableauValue)>zeroTolerance) {
	  unsigned char iStatus=internalStatus[iSequence]&7;
	  if (iStatus<4) {
	    index[numberNonZero]=iSequence;
	    array[numberNonZero++]=tableauValue;
	    double mult = multiplier[iStatus];
	    double alpha = tableauValue * mult;
	    double oldValue = abcDj[iSequence] * mult;
	    double value = oldValue - tentativeTheta * alpha;
	    if (value < dualT) {
	      bestPossible = CoinMax(bestPossible, alpha);
	      value = oldValue - upperTheta * alpha;
	      if (value < dualT && alpha >= acceptablePivot) {
		upperTheta = (oldValue - dualT) / alpha;
	      }
	      // add to list
	      arrayCandidate[numberRemaining] = alpha;
	      indexCandidate[numberRemaining++] = iSequence;
	    }
	  }
	}
      }
    }
  } else {
    double badFree = 0.0;
    double freeAlpha = model_->currentAcceptablePivot();
    int freeSequenceIn=model_->freeSequenceIn();
    //printf("block %d freeSequence %d acceptable %g\n",iBlock,freeSequenceIn,freeAlpha);
    double currentDualTolerance = model_->currentDualTolerance();
    for (int iSequence = first; iSequence < last; iSequence++) {
      double tableauValue=array[iSequence];
      if (tableauValue) {
	array[iSequence]=0.0;
	if (fabs(tableauValue)>zeroTolerance) {
	  unsigned char iStatus=internalStatus[iSequence]&7;
	  if (iStatus<6) {
	    if ((iStatus&4)==0) {
	      index[numberNonZero]=iSequence;
	      array[numberNonZero++]=tableauValue;
	      double mult = multiplier[iStatus];
	      double alpha = tableauValue * mult;
	      double oldValue = abcDj[iSequence] * mult;
	      double value = oldValue - tentativeTheta * alpha;
	      if (value < dualT) {
		bestPossible = CoinMax(bestPossible, alpha);
		value = oldValue - upperTheta * alpha;
		if (value < dualT && alpha >= acceptablePivot) {
		  upperTheta = (oldValue - dualT) / alpha;
		}
		// add to list
		arrayCandidate[numberRemaining] = alpha;
		indexCandidate[numberRemaining++] = iSequence;
	      }
	    } else {
	      bool keep;
	      index[numberNonZero]=iSequence;
	      array[numberNonZero++]=tableauValue;
	      bestPossible = CoinMax(bestPossible, fabs(tableauValue));
	      double oldValue = abcDj[iSequence];
	      // If free has to be very large - should come in via dualRow
	      //if (getInternalStatus(iSequence+addSequence)==isFree&&fabs(tableauValue)<1.0e-3)
	      //break;
	      // may be fake super basic
	      if (oldValue > currentDualTolerance) {
		keep = true;
	      } else if (oldValue < -currentDualTolerance) {
		keep = true;
	      } else {
		if (fabs(tableauValue) > CoinMax(10.0 * acceptablePivot, 1.0e-5)) {
		  keep = true;
		} else {
		  keep = false;
		  badFree = CoinMax(badFree, fabs(tableauValue));
		}
	      }
#if 0
	      if (iSequence==freeSequenceIn)
		assert (keep);
#endif
	      if (keep) {
#ifdef PAN
		if (model_->fakeSuperBasic(iSequence)>=0) {
#endif
		  if (iSequence==freeSequenceIn)
		    tableauValue=COIN_DBL_MAX;
		  // free - choose largest
		  if (fabs(tableauValue) > fabs(freeAlpha)) {
		    freeAlpha = tableauValue;
		    freeSequence = iSequence;
		  }
#ifdef PAN
		}
#endif
	      }
	    }
	  }
	}
      }
    }
  }
#if 0
  if (model_->freeSequenceIn()>=first&&model_->freeSequenceIn()<last)
    assert (freeSequence==model_->freeSequenceIn());
  extern int xxInfo[6][8];
  xxInfo[0][iBlock]=first;
  xxInfo[1][iBlock]=last;
  xxInfo[2][iBlock]=firstIn;
  xxInfo[3][iBlock]=numberNonZero-firstIn;
  xxInfo[4][iBlock]=numberRemaining-firstIn;
#endif
  tableauRow.setNumElementsPartition(iBlock,numberNonZero-firstIn);
  candidateList.setNumElementsPartition(iBlock,numberRemaining-firstIn);
  return upperTheta;
}
// gets sorted tableau row and a possible value of theta
double 
AbcMatrix::dualColumn1Row2(double upperTheta, int & freeSequence,
			   const CoinIndexedVector & update,
			   CoinPartitionedVector & tableauRow,
			   CoinPartitionedVector & candidateList) const
{
  //int first=model_->maximumAbcNumberRows();
  assert(update.getNumElements()==2);
  const double *  COIN_RESTRICT pi=update.denseVector();
  const int *  COIN_RESTRICT piIndex = update.getIndices();
  int *  COIN_RESTRICT index = tableauRow.getIndices();
  double *  COIN_RESTRICT array = tableauRow.denseVector();
  const CoinBigIndex * COIN_RESTRICT rowStart = rowStart_;
  int numberRows=model_->numberRows();
  const CoinBigIndex * COIN_RESTRICT rowEnd = rowStart+numberRows*numberRowBlocks_;
  const double * COIN_RESTRICT element = element_;
  const int * COIN_RESTRICT column = column_;
  int iRow0 = piIndex[0];
  int iRow1 = piIndex[1];
  CoinBigIndex end0 = rowEnd[iRow0];
  CoinBigIndex end1 = rowEnd[iRow1];
  if (end0-rowStart[iRow0]>end1-rowStart[iRow1]) {
    int temp=iRow0;
    iRow0=iRow1;
    iRow1=temp;
  }
  CoinBigIndex start = rowStart[iRow0];
  CoinBigIndex end = rowEnd[iRow0];
  double piValue = pi[iRow0];
  double *  COIN_RESTRICT arrayCandidate=candidateList.denseVector();
  int numberNonZero;
  numberNonZero=tableauRow.getNumElements(0);
  int n=numberNonZero;
  for (CoinBigIndex j=start;j<end;j++) {
    int iSequence = column[j];
    double value = element[j];
    arrayCandidate[iSequence]= piValue*value; 
    index[n++]=iSequence;
  }
  start = rowStart[iRow1];
  end = rowEnd[iRow1];
  piValue = pi[iRow1];
  for (CoinBigIndex j=start;j<end;j++) {
    int iSequence = column[j];
    double value = element[j];
    value *= piValue;
    if (!arrayCandidate[iSequence]) {
      arrayCandidate[iSequence]= value; 
      index[n++]=iSequence;
    } else {	
      arrayCandidate[iSequence] += value; 
    }
  }
  // pack down
  const unsigned char *  COIN_RESTRICT internalStatus = model_->internalStatus();
  double zeroTolerance = model_->zeroTolerance();
  while (numberNonZero<n) {
    int iSequence=index[numberNonZero];
    double value=arrayCandidate[iSequence];
    arrayCandidate[iSequence]=0.0;
    unsigned char iStatus=internalStatus[iSequence]&7;
    if (fabs(value)>zeroTolerance&&iStatus<6) {
      index[numberNonZero]=iSequence;
      array[numberNonZero++]=value;
    } else {
      // kill
      n--;
      index[numberNonZero]=index[n];
    }
  }
  tableauRow.setNumElementsPartition(0,numberNonZero);
  return firstPass(model_,0, upperTheta, freeSequence,
		   tableauRow,
		   candidateList) ;
}
//static int ixxxxxx=1;
// gets sorted tableau row and a possible value of theta
double 
AbcMatrix::dualColumn1RowFew(int iBlock, double upperTheta, int & freeSequence,
			   const CoinIndexedVector & update,
			   CoinPartitionedVector & tableauRow,
			   CoinPartitionedVector & candidateList) const
{
  //int first=model_->maximumAbcNumberRows();
  int number = update.getNumElements();
  const double *  COIN_RESTRICT pi=update.denseVector();
  const int *  COIN_RESTRICT piIndex = update.getIndices();
  int *  COIN_RESTRICT index = tableauRow.getIndices();
  double *  COIN_RESTRICT array = tableauRow.denseVector();
  const CoinBigIndex * COIN_RESTRICT rowStart = rowStart_;
  int numberRows=model_->numberRows();
  const CoinBigIndex * COIN_RESTRICT rowEnd = rowStart+numberRows*numberRowBlocks_;
  const double * COIN_RESTRICT element = element_;
  const int * COIN_RESTRICT column = column_;
  double *  COIN_RESTRICT arrayCandidate=candidateList.denseVector();
  int numberNonZero;
  assert (iBlock>=0); 
  const unsigned char *  COIN_RESTRICT internalStatus = model_->internalStatus();
  int firstIn=blockStart_[iBlock];
  if (iBlock==0) {
    numberNonZero=0;
    for (int i=0;i<number;i++) {
      int iRow=piIndex[i];
      unsigned char type=internalStatus[iRow];
      if ((type&7)<6) {
	index[numberNonZero]=iRow;
	double piValue=pi[iRow];
	array[numberNonZero++]=piValue;
      }
    }
  } else {
    numberNonZero=firstIn;
  }
  int n=numberNonZero;
  rowStart += iBlock*numberRows;
  rowEnd = rowStart+numberRows;
  for (int i=0;i<number;i++) {
    int iRow=piIndex[i];
    double piValue = pi[iRow];
    CoinBigIndex end = rowEnd[iRow];
    for (CoinBigIndex j=rowStart[iRow];j<end;j++) {
      int iColumn = column[j];
      double value = element[j]*piValue;
      double oldValue=arrayCandidate[iColumn];
      value += oldValue;
      if (!oldValue) {
	arrayCandidate[iColumn]= value; 
	index[n++]=iColumn;
      } else if (value) {	
	arrayCandidate[iColumn] = value; 
      } else {
	arrayCandidate[iColumn] = COIN_INDEXED_REALLY_TINY_ELEMENT;
      }
    }
  }
  // pack down
  double zeroTolerance = model_->zeroTolerance();
  while (numberNonZero<n) {
    int iSequence=index[numberNonZero];
    double value=arrayCandidate[iSequence];
    arrayCandidate[iSequence]=0.0;
    unsigned char iStatus=internalStatus[iSequence]&7;
    if (fabs(value)>zeroTolerance&&iStatus<6) {
      index[numberNonZero]=iSequence;
      array[numberNonZero++]=value;
    } else {
      // kill
      n--;
      index[numberNonZero]=index[n];
    }
  }
  tableauRow.setNumElementsPartition(iBlock,numberNonZero-firstIn);
  upperTheta = firstPass(model_,iBlock, upperTheta, freeSequence,
		   tableauRow,
		   candidateList) ;
  return upperTheta;
}
// gets sorted tableau row and a possible value of theta
double 
AbcMatrix::dualColumn1Row1(double upperTheta, int & freeSequence,
			   const CoinIndexedVector & update,
			   CoinPartitionedVector & tableauRow,
			   CoinPartitionedVector & candidateList) const
{
  assert(update.getNumElements()==1);
  int iRow = update.getIndices()[0];
  double piValue = update.denseVector()[iRow];
  int *  COIN_RESTRICT index = tableauRow.getIndices();
  double *  COIN_RESTRICT array = tableauRow.denseVector();
  const CoinBigIndex * COIN_RESTRICT rowStart = rowStart_;
  int numberRows=model_->numberRows();
  const CoinBigIndex * COIN_RESTRICT rowEnd = rowStart+numberRows*numberRowBlocks_;
  CoinBigIndex start = rowStart[iRow];
  CoinBigIndex end = rowEnd[iRow];
  const double * COIN_RESTRICT element = element_;
  const int * COIN_RESTRICT column = column_;
  int numberNonZero;
  int numberRemaining;
  numberNonZero=tableauRow.getNumElements(0);
  numberRemaining = candidateList.getNumElements(0);
  double *  COIN_RESTRICT arrayCandidate=candidateList.denseVector();
  int *  COIN_RESTRICT indexCandidate = candidateList.getIndices();
  const double *  COIN_RESTRICT abcDj = model_->djRegion();
  const unsigned char *  COIN_RESTRICT internalStatus = model_->internalStatus();
  // do first pass to get possibles
  double bestPossible = 0.0;
  // We can also see if infeasible or pivoting on free
  double tentativeTheta = 1.0e25; // try with this much smaller as guess
  double acceptablePivot = model_->currentAcceptablePivot();
  double dualT=-model_->currentDualTolerance();
  const double multiplier[] = { 1.0, -1.0};
  double zeroTolerance = model_->zeroTolerance();
  freeSequence=-1;
  if (model_->ordinaryVariables()) {
    for (CoinBigIndex j=start;j<end;j++) {
      int iSequence = column[j];
      double value = element[j];
      double tableauValue = piValue*value;
      if (fabs(tableauValue)>zeroTolerance) {
	unsigned char iStatus=internalStatus[iSequence]&7;
	if (iStatus<4) {
	  index[numberNonZero]=iSequence;
	  array[numberNonZero++]=tableauValue;
	  double mult = multiplier[iStatus];
	  double alpha = tableauValue * mult;
	  double oldValue = abcDj[iSequence] * mult;
	  double value = oldValue - tentativeTheta * alpha;
	  if (value < dualT) {
	    bestPossible = CoinMax(bestPossible, alpha);
	    value = oldValue - upperTheta * alpha;
	    if (value < dualT && alpha >= acceptablePivot) {
	      upperTheta = (oldValue - dualT) / alpha;
	    }
	    // add to list
	    arrayCandidate[numberRemaining] = alpha;
	    indexCandidate[numberRemaining++] = iSequence;
	  }
	}
      }
    }
  } else {
    double badFree = 0.0;
    double freeAlpha = model_->currentAcceptablePivot();
    int freeSequenceIn=model_->freeSequenceIn();
    double currentDualTolerance = model_->currentDualTolerance();
    for (CoinBigIndex j=start;j<end;j++) {
      int iSequence = column[j];
      double value = element[j];
      double tableauValue = piValue*value;
      if (fabs(tableauValue)>zeroTolerance) {
	unsigned char iStatus=internalStatus[iSequence]&7;
	if (iStatus<6) {
	  if ((iStatus&4)==0) {
	    index[numberNonZero]=iSequence;
	    array[numberNonZero++]=tableauValue;
	    double mult = multiplier[iStatus];
	    double alpha = tableauValue * mult;
	    double oldValue = abcDj[iSequence] * mult;
	    double value = oldValue - tentativeTheta * alpha;
	    if (value < dualT) {
	      bestPossible = CoinMax(bestPossible, alpha);
	      value = oldValue - upperTheta * alpha;
	      if (value < dualT && alpha >= acceptablePivot) {
		upperTheta = (oldValue - dualT) / alpha;
	      }
	      // add to list
	      arrayCandidate[numberRemaining] = alpha;
	      indexCandidate[numberRemaining++] = iSequence;
	    }
	  } else {
	    bool keep;
	    index[numberNonZero]=iSequence;
	    array[numberNonZero++]=tableauValue;
	    bestPossible = CoinMax(bestPossible, fabs(tableauValue));
	    double oldValue = abcDj[iSequence];
	    // If free has to be very large - should come in via dualRow
	    //if (getInternalStatus(iSequence+addSequence)==isFree&&fabs(tableauValue)<1.0e-3)
	    //break;
	    if (oldValue > currentDualTolerance) {
	      keep = true;
	    } else if (oldValue < -currentDualTolerance) {
	      keep = true;
	    } else {
	      if (fabs(tableauValue) > CoinMax(10.0 * acceptablePivot, 1.0e-5)) {
		keep = true;
	      } else {
		keep = false;
		badFree = CoinMax(badFree, fabs(tableauValue));
	      }
	    }
	    if (keep) {
#ifdef PAN
	      if (model_->fakeSuperBasic(iSequence)>=0) {
#endif
		if (iSequence==freeSequenceIn)
		  tableauValue=COIN_DBL_MAX;
		// free - choose largest
		if (fabs(tableauValue) > fabs(freeAlpha)) {
		  freeAlpha = tableauValue;
		  freeSequence = iSequence;
		}
#ifdef PAN
	      }
#endif
	    }
	  }
	}
      }
    }
  }
  tableauRow.setNumElementsPartition(0,numberNonZero);
  candidateList.setNumElementsPartition(0,numberRemaining);
  return upperTheta;
}
//#define PARALLEL2
#ifdef PARALLEL2
#undef cilk_for 
#undef cilk_spawn
#undef cilk_sync
#include <cilk/cilk.h>
#include <cilk/cilk_api.h>
#endif
#if 0
static void compact(int numberBlocks,CoinIndexedVector * vector,const int * starts,const int * lengths)
{
  int numberNonZeroIn=vector->getNumElements();
  int * index = vector->getIndices();
  double * array = vector->denseVector();
  CoinAbcCompact(numberBlocks,numberNonZeroIn,
		 array,starts,lengths); 
  int numberNonZero = CoinAbcCompact(numberBlocks,numberNonZeroIn,
				     index,starts,lengths); 
  vector->setNumElements(numberNonZero);
}
static void compactBoth(int numberBlocks,CoinIndexedVector * vector1,CoinIndexedVector * vector2,
			const int * starts,const int * lengths1, const int * lengths2)
{
  cilk_spawn compact(numberBlocks,vector1,starts,lengths1);
  compact(numberBlocks,vector2,starts,lengths2);
  cilk_sync;
}
#endif
void
AbcMatrix::rebalance() const
{
  int maximumRows = model_->maximumAbcNumberRows();
  int numberTotal = model_->numberTotal();
  /* rebalance
     For non-vector version
     each basic etc column is 1
     each real column is 5+2*nel
     each basic slack is 0
     each real slack is 3
   */
#if ABC_PARALLEL
  int howOften=CoinMax(model_->factorization()->maximumPivots(),200);
  if ((model_->numberIterations()%howOften)==0||!startColumnBlock_[1]) {
    int numberCpus=model_->numberCpus();
    if (numberCpus>1) {
      const CoinBigIndex * COIN_RESTRICT columnStart = matrix_->getVectorStarts()-maximumRows;
      const unsigned char *  COIN_RESTRICT internalStatus = model_->internalStatus();
      int numberRows=model_->numberRows();
      int total=0;
      for (int iSequence=0;iSequence<numberRows;iSequence++) {
	unsigned char iStatus=internalStatus[iSequence]&7;
	if (iStatus<4) 
	  total+=3;
      }
      int totalSlacks=total;
      CoinBigIndex end = columnStart[maximumRows];
      for (int iSequence=maximumRows;iSequence<numberTotal;iSequence++) {
	CoinBigIndex start=end;
	end = columnStart[iSequence+1];
	unsigned char iStatus=internalStatus[iSequence]&7;
	if (iStatus<4) 
	  total+=5+2*(end-start);
	else
	  total+=1;
      }
      int chunk = (total+numberCpus-1)/numberCpus;
      total=totalSlacks;
      int iCpu=0;
      startColumnBlock_[0]=0;
      end = columnStart[maximumRows];
      for (int iSequence=maximumRows;iSequence<numberTotal;iSequence++) {
	CoinBigIndex start=end;
	end = columnStart[iSequence+1];
	unsigned char iStatus=internalStatus[iSequence]&7;
	if (iStatus<4) 
	  total+=5+2*(end-start);
	else
	  total+=1;
	if (total>chunk) {
	  iCpu++;
	  total=0;
	  startColumnBlock_[iCpu]=iSequence;
	}
      }
      assert(iCpu<numberCpus);
      iCpu++;
      for (;iCpu<=numberCpus;iCpu++)
	startColumnBlock_[iCpu]=numberTotal;
      numberColumnBlocks_=numberCpus;
    } else {
      numberColumnBlocks_=1;
      startColumnBlock_[0]=0;
      startColumnBlock_[1]=numberTotal;
    }
  }
#else
  startColumnBlock_[0]=0;
  startColumnBlock_[1]=numberTotal;
#endif
}
double 
AbcMatrix::dualColumn1(const CoinIndexedVector & update,
				  CoinPartitionedVector & tableauRow,CoinPartitionedVector & candidateList) const
{
  int numberTotal = model_->numberTotal();
  // rebalance
  rebalance();
  tableauRow.setPackedMode(true);
  candidateList.setPackedMode(true);
  int number=update.getNumElements();
  double upperTheta;
  if (rowStart_&&number<3) {
#if ABC_INSTRUMENT>1
    {
      int n=update.getNumElements();
      abcPricing[n<19 ? n : 19]++;
    }
#endif
    // always do serially
    // do slacks first
    int starts[2];
    starts[0]=0;
    starts[1]=numberTotal;
    tableauRow.setPartitions(1,starts);
    candidateList.setPartitions(1,starts);
    upperTheta = static_cast<AbcSimplexDual *>(model_)->dualColumn1A();
    //assert (upperTheta>-100*model_->dualTolerance()||model_->sequenceIn()>=0||model_->lastFirstFree()>=0);
    int freeSequence=-1;
    // worth using row copy
    assert (number);
    if (number==2) {
      upperTheta=dualColumn1Row2(upperTheta,freeSequence,update,tableauRow,candidateList);
    } else {
      upperTheta=dualColumn1Row1(upperTheta,freeSequence,update,tableauRow,candidateList);
    }
    if (freeSequence>=0) {
      int numberNonZero=tableauRow.getNumElements(0);
      const int *  COIN_RESTRICT index = tableauRow.getIndices();
      const double *  COIN_RESTRICT array = tableauRow.denseVector();
      // search for free coming in
      double freeAlpha=0.0;
      int bestSequence=model_->sequenceIn();
      if (bestSequence>=0)
	freeAlpha=model_->alpha();
      index = tableauRow.getIndices();
      array = tableauRow.denseVector();
      // free variable - search 
      for (int k=0;k<numberNonZero;k++) {
	if (freeSequence==index[k]) {
	  if (fabs(freeAlpha)<fabs(array[k])) {
	    freeAlpha=array[k];
	  }
	  break;
	}
      }
      if (model_->sequenceIn()<0||fabs(freeAlpha)>fabs(model_->alpha())) {
	double oldValue = model_->djRegion()[freeSequence];
	model_->setSequenceIn(freeSequence);
	model_->setAlpha(freeAlpha);
	model_->setTheta(oldValue / freeAlpha);
      }
    }
  } else {
    // three or more
    // need to do better job on dividing up (but wait until vector or by row)
    upperTheta = parallelDual4(static_cast<AbcSimplexDual *>(model_));
  }
  //tableauRow.compact();
  //candidateList.compact();
#if 0 //ndef NDEBUG
  model_->checkArrays();
#endif
  candidateList.computeNumberElements();
  int numberRemaining=candidateList.getNumElements();
  if (!numberRemaining&&model_->sequenceIn()<0) {
    return COIN_DBL_MAX; // Looks infeasible
  } else {
    return upperTheta;
  }
}
#define _mm256_broadcast_sd(x) static_cast<__m256d> (__builtin_ia32_vbroadcastsd256 (x))
#define _mm256_load_pd(x) *(__m256d *)(x)
#define _mm256_store_pd (s, x)  *((__m256d *)s)=x
void
AbcMatrix::dualColumn1Part(int iBlock, int & sequenceIn, double & upperThetaResult,
			   const CoinIndexedVector & update,
			   CoinPartitionedVector & tableauRow,CoinPartitionedVector & candidateList) const
{
  double upperTheta=upperThetaResult;
#if 0
  double time0=CoinCpuTime();
#endif
  int maximumRows = model_->maximumAbcNumberRows();
  int firstIn=startColumnBlock_[iBlock];
  int last = startColumnBlock_[iBlock+1];
  int numberNonZero;
  int numberRemaining;
  int first;
  if (firstIn==0) {
    upperTheta=static_cast<AbcSimplexDual *>(model_)->dualColumn1A();
    numberNonZero=tableauRow.getNumElements(0);
    numberRemaining = candidateList.getNumElements(0);
    first=maximumRows;
  } else {
    first=firstIn;
    numberNonZero=first;
    numberRemaining=first;
  }
  sequenceIn=-1;
  // get matrix data pointers
  const int * COIN_RESTRICT row = matrix_->getIndices();
  const CoinBigIndex * COIN_RESTRICT columnStart = matrix_->getVectorStarts()-maximumRows;
  const double * COIN_RESTRICT elementByColumn = matrix_->getElements();
  const double *  COIN_RESTRICT pi=update.denseVector();
  //const int *  COIN_RESTRICT piIndex = update.getIndices();
  int *  COIN_RESTRICT index = tableauRow.getIndices();
  double *  COIN_RESTRICT array = tableauRow.denseVector();
  // pivot elements
  double *  COIN_RESTRICT arrayCandidate=candidateList.denseVector();
  // indices
  int *  COIN_RESTRICT indexCandidate = candidateList.getIndices();
  const double *  COIN_RESTRICT abcDj = model_->djRegion();
  const unsigned char *  COIN_RESTRICT internalStatus = model_->internalStatus();
  // do first pass to get possibles
  double bestPossible = 0.0;
  // We can also see if infeasible or pivoting on free
  double tentativeTheta = 1.0e25; // try with this much smaller as guess
  double acceptablePivot = model_->currentAcceptablePivot();
  double dualT=-model_->currentDualTolerance();
  const double multiplier[] = { 1.0, -1.0};
  double zeroTolerance = model_->zeroTolerance();
  if (model_->ordinaryVariables()) {
#ifdef COUNT_COPY
    if (first>maximumRows||last<model_->numberTotal()||false) {
#endif
#if 1
    for (int iSequence = first; iSequence < last; iSequence++) {
      unsigned char iStatus=internalStatus[iSequence]&7;
      if (iStatus<4) {
	CoinBigIndex start = columnStart[iSequence];
	CoinBigIndex next = columnStart[iSequence+1];
	double tableauValue = 0.0;
	for (CoinBigIndex j = start; j < next; j++) {
	  int jRow = row[j];
	  tableauValue += pi[jRow] * elementByColumn[j];
	}
	if (fabs(tableauValue)>zeroTolerance) {
#else
    cilk_for (int iSequence = first; iSequence < last; iSequence++) {
      unsigned char iStatus=internalStatus[iSequence]&7;
      if (iStatus<4) {
	CoinBigIndex start = columnStart[iSequence];
	CoinBigIndex next = columnStart[iSequence+1];
	double tableauValue = 0.0;
	for (CoinBigIndex j = start; j < next; j++) {
	  int jRow = row[j];
	  tableauValue += pi[jRow] * elementByColumn[j];
	}
	array[iSequence]=tableauValue;
      }
    }
    //printf("time %.6g\n",CoinCpuTime()-time0);
    for (int iSequence = first; iSequence < last; iSequence++) {
      double tableauValue=array[iSequence];
      if (tableauValue) {
	array[iSequence]=0.0;
	if (fabs(tableauValue)>zeroTolerance) {
	  unsigned char iStatus=internalStatus[iSequence]&7;
#endif
	  index[numberNonZero]=iSequence;
	  array[numberNonZero++]=tableauValue;
	  double mult = multiplier[iStatus];
	  double alpha = tableauValue * mult;
	  double oldValue = abcDj[iSequence] * mult;
	  double value = oldValue - tentativeTheta * alpha;
	  if (value < dualT) {
	    bestPossible = CoinMax(bestPossible, alpha);
	    value = oldValue - upperTheta * alpha;
	    if (value < dualT && alpha >= acceptablePivot) {
	      upperTheta = (oldValue - dualT) / alpha;
	    }
	    // add to list
	    arrayCandidate[numberRemaining] = alpha;
	    indexCandidate[numberRemaining++] = iSequence;
	  }
	}
      }
    }
#ifdef COUNT_COPY
    } else {
      const double * COIN_RESTRICT element = countElement_;
      const int * COIN_RESTRICT row = countRow_;
      double temp[4] __attribute__ ((aligned (32)));
      memset(temp,0,sizeof(temp));
      for (int iCount=smallestCount_;iCount<largestCount_;iCount++) {
	int iStart=countFirst_[iCount];
	int number=countFirst_[iCount+1]-iStart;
	if (!number)
	  continue;
	const int * COIN_RESTRICT blockRow = row+countStart_[iCount];
	const double * COIN_RESTRICT blockElement = element+countStart_[iCount];
	const int * COIN_RESTRICT sequence = countRealColumn_+iStart;
	// really need to sort and swap to avoid tests
	int numberBlocks=number>>2;
	number &= 3;
	for (int iBlock=0;iBlock<numberBlocks;iBlock++) {
	  double tableauValue0=0.0;
	  double tableauValue1=0.0;
	  double tableauValue2=0.0;
	  double tableauValue3=0.0;
	  for (int j=0;j<iCount;j++) {
	    tableauValue0 += pi[blockRow[0]]*blockElement[0];
	    tableauValue1 += pi[blockRow[1]]*blockElement[1];
	    tableauValue2 += pi[blockRow[2]]*blockElement[2];
	    tableauValue3 += pi[blockRow[3]]*blockElement[3];
	    blockRow+=4;
	    blockElement+=4;
	  }
	  if (fabs(tableauValue0)>zeroTolerance) {
	    int iSequence=sequence[0];
	    unsigned char iStatus=internalStatus[iSequence]&7;
	    if (iStatus<4) {
	      index[numberNonZero]=iSequence;
	      array[numberNonZero++]=tableauValue0;
	      double mult = multiplier[iStatus];
	      double alpha = tableauValue0 * mult;
	      double oldValue = abcDj[iSequence] * mult;
	      double value = oldValue - tentativeTheta * alpha;
	      if (value < dualT) {
		bestPossible = CoinMax(bestPossible, alpha);
		value = oldValue - upperTheta * alpha;
		if (value < dualT && alpha >= acceptablePivot) {
		  upperTheta = (oldValue - dualT) / alpha;
		}
		// add to list
		arrayCandidate[numberRemaining] = alpha;
		indexCandidate[numberRemaining++] = iSequence;
	      }
	    }
	  }
	  if (fabs(tableauValue1)>zeroTolerance) {
	    int iSequence=sequence[1];
	    unsigned char iStatus=internalStatus[iSequence]&7;
	    if (iStatus<4) {
	      index[numberNonZero]=iSequence;
	      array[numberNonZero++]=tableauValue1;
	      double mult = multiplier[iStatus];
	      double alpha = tableauValue1 * mult;
	      double oldValue = abcDj[iSequence] * mult;
	      double value = oldValue - tentativeTheta * alpha;
	      if (value < dualT) {
		bestPossible = CoinMax(bestPossible, alpha);
		value = oldValue - upperTheta * alpha;
		if (value < dualT && alpha >= acceptablePivot) {
		  upperTheta = (oldValue - dualT) / alpha;
		}
		// add to list
		arrayCandidate[numberRemaining] = alpha;
		indexCandidate[numberRemaining++] = iSequence;
	      }
	    }
	  }
	  if (fabs(tableauValue2)>zeroTolerance) {
	    int iSequence=sequence[2];
	    unsigned char iStatus=internalStatus[iSequence]&7;
	    if (iStatus<4) {
	      index[numberNonZero]=iSequence;
	      array[numberNonZero++]=tableauValue2;
	      double mult = multiplier[iStatus];
	      double alpha = tableauValue2 * mult;
	      double oldValue = abcDj[iSequence] * mult;
	      double value = oldValue - tentativeTheta * alpha;
	      if (value < dualT) {
		bestPossible = CoinMax(bestPossible, alpha);
		value = oldValue - upperTheta * alpha;
		if (value < dualT && alpha >= acceptablePivot) {
		  upperTheta = (oldValue - dualT) / alpha;
		}
		// add to list
		arrayCandidate[numberRemaining] = alpha;
		indexCandidate[numberRemaining++] = iSequence;
	      }
	    }
	  }
	  if (fabs(tableauValue3)>zeroTolerance) {
	    int iSequence=sequence[3];
	    unsigned char iStatus=internalStatus[iSequence]&7;
	    if (iStatus<4) {
	      index[numberNonZero]=iSequence;
	      array[numberNonZero++]=tableauValue3;
	      double mult = multiplier[iStatus];
	      double alpha = tableauValue3 * mult;
	      double oldValue = abcDj[iSequence] * mult;
	      double value = oldValue - tentativeTheta * alpha;
	      if (value < dualT) {
		bestPossible = CoinMax(bestPossible, alpha);
		value = oldValue - upperTheta * alpha;
		if (value < dualT && alpha >= acceptablePivot) {
		  upperTheta = (oldValue - dualT) / alpha;
		}
		// add to list
		arrayCandidate[numberRemaining] = alpha;
		indexCandidate[numberRemaining++] = iSequence;
	      }
	    }
	  }
	  sequence+=4;
	}
	for (int i=0;i<number;i++) {
	  int iSequence=sequence[i];
	  unsigned char iStatus=internalStatus[iSequence]&7;
	  if (iStatus<4) {
	    double tableauValue=0.0;
	    for (int j=0;j<iCount;j++) {
	      int iRow=blockRow[4*j];
	      tableauValue += pi[iRow]*blockElement[4*j];
	    }
	    if (fabs(tableauValue)>zeroTolerance) {
	      index[numberNonZero]=iSequence;
	      array[numberNonZero++]=tableauValue;
	      double mult = multiplier[iStatus];
	      double alpha = tableauValue * mult;
	      double oldValue = abcDj[iSequence] * mult;
	      double value = oldValue - tentativeTheta * alpha;
	      if (value < dualT) {
		bestPossible = CoinMax(bestPossible, alpha);
		value = oldValue - upperTheta * alpha;
		if (value < dualT && alpha >= acceptablePivot) {
		  upperTheta = (oldValue - dualT) / alpha;
		}
		// add to list
		arrayCandidate[numberRemaining] = alpha;
		indexCandidate[numberRemaining++] = iSequence;
	      }
	    }
	  }
	  blockRow++;
	  blockElement++;
	}
      }
      int numberColumns=model_->numberColumns();
      // large ones
      const CoinBigIndex * COIN_RESTRICT largeStart = countStartLarge_-countFirst_[MAX_COUNT];
      for (int i=countFirst_[MAX_COUNT];i<numberColumns;i++) {
	int iSequence=countRealColumn_[i];
	unsigned char iStatus=internalStatus[iSequence]&7;
	if (iStatus<4) {
	  CoinBigIndex start = largeStart[i];
	  CoinBigIndex next = largeStart[i+1];
	  double tableauValue = 0.0;
	  for (CoinBigIndex j = start; j < next; j++) {
	    int jRow = row[j];
	    tableauValue += pi[jRow] * element[j];
	  }
	  if (fabs(tableauValue)>zeroTolerance) {
	    index[numberNonZero]=iSequence;
	    array[numberNonZero++]=tableauValue;
	    double mult = multiplier[iStatus];
	    double alpha = tableauValue * mult;
	    double oldValue = abcDj[iSequence] * mult;
	    double value = oldValue - tentativeTheta * alpha;
	    if (value < dualT) {
	      bestPossible = CoinMax(bestPossible, alpha);
	      value = oldValue - upperTheta * alpha;
	      if (value < dualT && alpha >= acceptablePivot) {
		upperTheta = (oldValue - dualT) / alpha;
	      }
	      // add to list
	      arrayCandidate[numberRemaining] = alpha;
	      indexCandidate[numberRemaining++] = iSequence;
	    }
	  }
	}
      }
    }
#endif
  } else {
    double badFree = 0.0;
    double freePivot = model_->currentAcceptablePivot();
    int freeSequenceIn=model_->freeSequenceIn();
    double currentDualTolerance = model_->currentDualTolerance();
    for (int iSequence = first; iSequence < last; iSequence++) {
      unsigned char iStatus=internalStatus[iSequence]&7;
      if (iStatus<6) {
	CoinBigIndex start = columnStart[iSequence];
	CoinBigIndex next = columnStart[iSequence+1];
	double tableauValue = 0.0;
	for (CoinBigIndex j = start; j < next; j++) {
	  int jRow = row[j];
	  tableauValue += pi[jRow] * elementByColumn[j];
	}
	if (fabs(tableauValue)>zeroTolerance) {
	  if ((iStatus&4)==0) {
	    index[numberNonZero]=iSequence;
	    array[numberNonZero++]=tableauValue;
	    double mult = multiplier[iStatus];
	    double alpha = tableauValue * mult;
	    double oldValue = abcDj[iSequence] * mult;
	    double value = oldValue - tentativeTheta * alpha;
	    if (value < dualT) {
	      bestPossible = CoinMax(bestPossible, alpha);
	      value = oldValue - upperTheta * alpha;
	      if (value < dualT && alpha >= acceptablePivot) {
		upperTheta = (oldValue - dualT) / alpha;
	      }
	      // add to list
	      arrayCandidate[numberRemaining] = alpha;
	      indexCandidate[numberRemaining++] = iSequence;
	    }
	  } else {
	    bool keep;
	    index[numberNonZero]=iSequence;
	    array[numberNonZero++]=tableauValue;
	    bestPossible = CoinMax(bestPossible, fabs(tableauValue));
	    double oldValue = abcDj[iSequence];
	    // If free has to be very large - should come in via dualRow
	    //if (getInternalStatus(iSequence+addSequence)==isFree&&fabs(tableauValue)<1.0e-3)
	    //break;
	    if (oldValue > currentDualTolerance) {
	      keep = true;
	    } else if (oldValue < -currentDualTolerance) {
	      keep = true;
	    } else {
	      if (fabs(tableauValue) > CoinMax(10.0 * acceptablePivot, 1.0e-5)) {
		keep = true;
	      } else {
		keep = false;
		badFree = CoinMax(badFree, fabs(tableauValue));
	      }
	    }
	    if (keep) {
#ifdef PAN
	      if (model_->fakeSuperBasic(iSequence)>=0) {
#endif
		if (iSequence==freeSequenceIn)
		  tableauValue=COIN_DBL_MAX;
		// free - choose largest
		if (fabs(tableauValue) > fabs(freePivot)) {
		  freePivot = tableauValue;
		  sequenceIn = iSequence;
		}
#ifdef PAN
	      }
#endif
	    }
	  }
	}
      }
    }
  } 
  // adjust
  numberNonZero -= firstIn;
  numberRemaining -= firstIn;
  tableauRow.setNumElementsPartition(iBlock,numberNonZero);
  candidateList.setNumElementsPartition(iBlock,numberRemaining);
  if (!numberRemaining&&model_->sequenceIn()<0) {
  
    upperThetaResult=COIN_DBL_MAX; // Looks infeasible
  } else {
    upperThetaResult=upperTheta;
  }
}
// gets tableau row - returns number of slacks in block 
int 
AbcMatrix::primalColumnRow(int iBlock,bool doByRow,const CoinIndexedVector & updates,
			   CoinPartitionedVector & tableauRow) const
{
  int maximumRows = model_->maximumAbcNumberRows();
  int first=tableauRow.startPartition(iBlock);
  int last=tableauRow.startPartition(iBlock+1);
  if (doByRow) { 
    assert(first==blockStart_[iBlock]);
    assert(last==blockStart_[iBlock+1]);
  } else {
    assert(first==startColumnBlock_[iBlock]);
    assert(last==startColumnBlock_[iBlock+1]);
  }
  int numberSlacks=updates.getNumElements();
  int numberNonZero=0;
  const double *  COIN_RESTRICT pi=updates.denseVector();
  const int *  COIN_RESTRICT piIndex = updates.getIndices();
  int *  COIN_RESTRICT index = tableauRow.getIndices()+first;
  double *  COIN_RESTRICT array = tableauRow.denseVector()+first;
  const unsigned char *  COIN_RESTRICT internalStatus = model_->internalStatus();
  if (!iBlock) {
    numberNonZero=numberSlacks;
    for (int i=0;i<numberSlacks;i++) {
      int iRow=piIndex[i];
      index[i]=iRow;
      array[i]=pi[iRow]; // ? what if small or basic
    }
    first=maximumRows;
  }
  double zeroTolerance = model_->zeroTolerance();
  if (doByRow) {
    int numberRows=model_->numberRows();
    const CoinBigIndex * COIN_RESTRICT rowStart = rowStart_+iBlock*numberRows;
    const CoinBigIndex * COIN_RESTRICT rowEnd = rowStart+numberRows;
    const double * COIN_RESTRICT element = element_;
    const int * COIN_RESTRICT column = column_;
    if (numberSlacks>1) {
      double *  COIN_RESTRICT arrayTemp = tableauRow.denseVector();
      for (int i=0;i<numberSlacks;i++) {
	int iRow=piIndex[i];
	double piValue = pi[iRow];
	CoinBigIndex end = rowEnd[iRow];
	for (CoinBigIndex j=rowStart[iRow];j<end;j++) {
	  int iColumn = column[j];
	  arrayTemp[iColumn] += element[j]*piValue;
	}
      }
      for (int iSequence=first;iSequence<last;iSequence++) {
	double tableauValue = arrayTemp[iSequence];
	if (tableauValue) {
	  arrayTemp[iSequence]=0.0;
	  if (fabs(tableauValue)>zeroTolerance) {
	    unsigned char iStatus=internalStatus[iSequence]&7;
	    if (iStatus<6) {
	      index[numberNonZero]=iSequence;
	      array[numberNonZero++]=tableauValue;
	    }
	  }
	}
      }
    } else {
      int iRow=piIndex[0];
      double piValue = pi[iRow];
      CoinBigIndex end = rowEnd[iRow];
      for (CoinBigIndex j=rowStart[iRow];j<end;j++) {
	int iSequence = column[j];
	double tableauValue = element[j]*piValue;
	if (fabs(tableauValue)>zeroTolerance) {
	  unsigned char iStatus=internalStatus[iSequence]&7;
	  if (iStatus<6) {
	    index[numberNonZero]=iSequence;
	    array[numberNonZero++]=tableauValue;
	  }
	}
      }
    }
  } else {
    // get matrix data pointers
    const int * COIN_RESTRICT row = matrix_->getIndices();
    const CoinBigIndex * COIN_RESTRICT columnStart = matrix_->getVectorStarts()-maximumRows;
    const double * COIN_RESTRICT elementByColumn = matrix_->getElements();
    const double *  COIN_RESTRICT pi=updates.denseVector();
    for (int iSequence = first; iSequence < last; iSequence++) {
      unsigned char iStatus=internalStatus[iSequence]&7;
      if (iStatus<6) {
	CoinBigIndex start = columnStart[iSequence];
	CoinBigIndex next = columnStart[iSequence+1];
	double tableauValue = 0.0;
	for (CoinBigIndex j = start; j < next; j++) {
	  int jRow = row[j];
	  tableauValue += pi[jRow] * elementByColumn[j];
	}
	if (fabs(tableauValue)>zeroTolerance) {
	  index[numberNonZero]=iSequence;
	  array[numberNonZero++]=tableauValue;
	}
      }
    }
  }
  tableauRow.setNumElementsPartition(iBlock,numberNonZero);
  return numberSlacks;
}
// Get sequenceIn when Dantzig
 int 
AbcMatrix::pivotColumnDantzig(const CoinIndexedVector & updates,
			      CoinPartitionedVector & spare) const
 {
   int maximumRows = model_->maximumAbcNumberRows();
   // rebalance
   rebalance();
   spare.setPackedMode(true);
   bool useRowCopy=false;
   if (rowStart_) {
     // see if worth using row copy
     int number=updates.getNumElements();
     assert (number);
     useRowCopy = (number<<2)<maximumRows;
   }
   const int * starts;
   if (useRowCopy)
     starts=blockStart();
   else
     starts=startColumnBlock();
#if ABC_PARALLEL
#define NUMBER_BLOCKS NUMBER_ROW_BLOCKS
  int numberBlocks=CoinMin(NUMBER_BLOCKS,model_->numberCpus());
#else
#define NUMBER_BLOCKS 1
  int numberBlocks=NUMBER_BLOCKS;
#endif
#if ABC_PARALLEL
  if (model_->parallelMode()==0)
    numberBlocks=1;
#endif
   spare.setPartitions(numberBlocks,starts);
   int which[NUMBER_BLOCKS];
   double best[NUMBER_BLOCKS];
   for (int i=0;i<numberBlocks-1;i++) 
     which[i]=cilk_spawn pivotColumnDantzig(i,useRowCopy,updates,spare,best[i]); 
   which[numberBlocks-1]=pivotColumnDantzig(numberBlocks-1,useRowCopy,updates,
						       spare,best[numberBlocks-1]);
   cilk_sync;
   int bestSequence=-1;
   double bestValue=model_->dualTolerance();
   for (int i=0;i<numberBlocks;i++) {
     if (best[i]>bestValue) {
       bestValue=best[i];
       bestSequence=which[i];
     }
   }
   return bestSequence;
}
 // Get sequenceIn when Dantzig (One block)
 int 
   AbcMatrix::pivotColumnDantzig(int iBlock, bool doByRow,const CoinIndexedVector & updates,
				 CoinPartitionedVector & spare,
			      double & bestValue) const
 {
   // could rewrite to do more directly
   int numberSlacks=primalColumnRow(iBlock,doByRow,updates,spare);
   double * COIN_RESTRICT reducedCost = model_->djRegion();
   int first=spare.startPartition(iBlock);
   int last=spare.startPartition(iBlock+1);
   int *  COIN_RESTRICT index = spare.getIndices()+first;
   double *  COIN_RESTRICT array = spare.denseVector()+first;
   int numberNonZero=spare.getNumElements(iBlock);
   int bestSequence=-1;
   double bestDj=model_->dualTolerance();
   double bestFreeDj = model_->dualTolerance();
   int bestFreeSequence = -1;
   // redo LOTS so signs for slacks and columns same
   if (!first) {
     first = model_->maximumAbcNumberRows();
     for (int i=0;i<numberSlacks;i++) {
       int iSequence = index[i];
       double value = reducedCost[iSequence];
       value += array[i];
       array[i] = 0.0;
       reducedCost[iSequence] = value;
     }
#ifndef CLP_PRIMAL_SLACK_MULTIPLIER 
#define CLP_PRIMAL_SLACK_MULTIPLIER 1.0
#endif
     int numberRows=model_->numberRows();
     for (int iSequence=0 ; iSequence < numberRows; iSequence++) {
       // check flagged variable
       if (!model_->flagged(iSequence)) {
	 double value = reducedCost[iSequence] * CLP_PRIMAL_SLACK_MULTIPLIER;
	 AbcSimplex::Status status = model_->getInternalStatus(iSequence);
	 
	 switch(status) {
	   
	 case AbcSimplex::basic:
	 case AbcSimplex::isFixed:
	   break;
	 case AbcSimplex::isFree:
	 case AbcSimplex::superBasic:
	   if (fabs(value) > bestFreeDj) {
	     bestFreeDj = fabs(value);
	     bestFreeSequence = iSequence;
	   }
	   break;
	 case AbcSimplex::atUpperBound:
	   if (value > bestDj) {
	     bestDj = value;
	     bestSequence = iSequence;
	   }
	   break;
	 case AbcSimplex::atLowerBound:
	   if (value < -bestDj) {
	     bestDj = -value;
	     bestSequence = iSequence;
	   }
	 }
       }
     }
   }
   for (int i=numberSlacks;i<numberNonZero;i++) {
     int iSequence = index[i];
     double value = reducedCost[iSequence];
     value += array[i];
     array[i] = 0.0;
     reducedCost[iSequence] = value;
   }
   for (int iSequence=first ; iSequence < last; iSequence++) {
     // check flagged variable
     if (!model_->flagged(iSequence)) {
       double value = reducedCost[iSequence];
       AbcSimplex::Status status = model_->getInternalStatus(iSequence);
       
       switch(status) {
	 
       case AbcSimplex::basic:
       case AbcSimplex::isFixed:
	 break;
       case AbcSimplex::isFree:
       case AbcSimplex::superBasic:
	 if (fabs(value) > bestFreeDj) {
	   bestFreeDj = fabs(value);
	   bestFreeSequence = iSequence;
	 }
	 break;
       case AbcSimplex::atUpperBound:
	 if (value > bestDj) {
	   bestDj = value;
	   bestSequence = iSequence;
	 }
	 break;
       case AbcSimplex::atLowerBound:
	 if (value < -bestDj) {
	   bestDj = -value;
	   bestSequence = iSequence;
	 }
       }
     }
   }
   spare.setNumElementsPartition(iBlock,0);
   // bias towards free
   if (bestFreeSequence >= 0 && bestFreeDj > 0.1 * bestDj) {
     bestSequence = bestFreeSequence;
     bestDj=10.0*bestFreeDj;
   }
   bestValue=bestDj;
   return bestSequence;
 } 
// gets tableau row and dj row - returns number of slacks in block 
 int 
   AbcMatrix::primalColumnRowAndDjs(int iBlock,const CoinIndexedVector & updateTableau,
				    const CoinIndexedVector & updateDjs,
				    CoinPartitionedVector & tableauRow) const
{
  int maximumRows = model_->maximumAbcNumberRows();
  int first=tableauRow.startPartition(iBlock);
  int last=tableauRow.startPartition(iBlock+1);
  assert(first==startColumnBlock_[iBlock]);
  assert(last==startColumnBlock_[iBlock+1]);
  int numberSlacks=updateTableau.getNumElements();
  int numberNonZero=0;
  const double *  COIN_RESTRICT piTableau=updateTableau.denseVector();
  const double *  COIN_RESTRICT pi=updateDjs.denseVector();
  int *  COIN_RESTRICT index = tableauRow.getIndices()+first;
  double *  COIN_RESTRICT array = tableauRow.denseVector()+first;
  const unsigned char *  COIN_RESTRICT internalStatus = model_->internalStatus();
  double * COIN_RESTRICT reducedCost = model_->djRegion();
  if (!iBlock) {
    const int *  COIN_RESTRICT piIndexTableau = updateTableau.getIndices();
    numberNonZero=numberSlacks;
    for (int i=0;i<numberSlacks;i++) {
      int iRow=piIndexTableau[i];
      index[i]=iRow;
      array[i]=piTableau[iRow]; // ? what if small or basic
    }
    const int *  COIN_RESTRICT piIndex = updateDjs.getIndices();
    int number=updateDjs.getNumElements();
    for (int i=0;i<number;i++) {
      int iRow=piIndex[i];
      reducedCost[iRow] -= pi[iRow]; // sign?
    }
    first=maximumRows;
  }
  double zeroTolerance = model_->zeroTolerance();
  // get matrix data pointers
  const int * COIN_RESTRICT row = matrix_->getIndices();
  const CoinBigIndex * COIN_RESTRICT columnStart = matrix_->getVectorStarts()-maximumRows;
  const double * COIN_RESTRICT elementByColumn = matrix_->getElements();
  for (int iSequence = first; iSequence < last; iSequence++) {
    unsigned char iStatus=internalStatus[iSequence]&7;
    if (iStatus<6) {
      CoinBigIndex start = columnStart[iSequence];
      CoinBigIndex next = columnStart[iSequence+1];
      double tableauValue = 0.0;
      double djValue=reducedCost[iSequence];
      for (CoinBigIndex j = start; j < next; j++) {
	int jRow = row[j];
	tableauValue += piTableau[jRow] * elementByColumn[j];
	djValue -= pi[jRow] * elementByColumn[j]; // sign?
      }
      reducedCost[iSequence]=djValue;
      if (fabs(tableauValue)>zeroTolerance) {
	index[numberNonZero]=iSequence;
	array[numberNonZero++]=tableauValue;
      }
    }
  }
  tableauRow.setNumElementsPartition(iBlock,numberNonZero);
  return numberSlacks;
}
/* does steepest edge double or triple update
   If scaleFactor!=0 then use with tableau row to update djs
   otherwise use updateForDjs
*/
int
AbcMatrix::primalColumnDouble(int iBlock,CoinPartitionedVector & updateForTableauRow,
			      CoinPartitionedVector & updateForDjs,
			      const CoinIndexedVector & updateForWeights,
			      CoinPartitionedVector & spareColumn1,
			      double * infeasibilities, 
			      double referenceIn, double devex,
			      // Array for exact devex to say what is in reference framework
			      unsigned int * reference,
			      double * weights, double scaleFactor) const
{
  int maximumRows = model_->maximumAbcNumberRows();
  int first=startColumnBlock_[iBlock];
  int last=startColumnBlock_[iBlock+1];
  const double *  COIN_RESTRICT piTableau=updateForTableauRow.denseVector();
  const double *  COIN_RESTRICT pi=updateForDjs.denseVector();
  const double *  COIN_RESTRICT piWeights=updateForWeights.denseVector();
  const unsigned char *  COIN_RESTRICT internalStatus = model_->internalStatus();
  double * COIN_RESTRICT reducedCost = model_->djRegion();
  double tolerance = model_->currentDualTolerance();
  // use spareColumn to track new infeasibilities
  int * COIN_RESTRICT newInfeas = spareColumn1.getIndices()+first;
  int numberNewInfeas=0;
  // we can't really trust infeasibilities if there is dual error
  // this coding has to mimic coding in checkDualSolution
  double error = CoinMin(1.0e-2, model_->largestDualError());
  // allow tolerance at least slightly bigger than standard
  tolerance = tolerance +  error;
  double mult[2]={1.0,-1.0};
  bool updateWeights=devex!=0.0;
#define isReference(i) (((reference[i>>5] >> (i & 31)) & 1) != 0)
  // Scale factor is other way round so tableau row is 1.0* while dj update is scaleFactor*
  if (!iBlock) {
    int numberSlacks=updateForTableauRow.getNumElements();
    const int *  COIN_RESTRICT piIndexTableau = updateForTableauRow.getIndices();
    if (!scaleFactor) {
      const int *  COIN_RESTRICT piIndex = updateForDjs.getIndices();
      int number=updateForDjs.getNumElements();
      for (int i=0;i<number;i++) {
	int iRow=piIndex[i];
	int iStatus=internalStatus[iRow]&7;
	double value=reducedCost[iRow];
	value += pi[iRow];
	if (iStatus<4) {
	  reducedCost[iRow]=value;
	  value *= mult[iStatus];
	  if (value<0.0) {
	    if (!infeasibilities[iRow])
	      newInfeas[numberNewInfeas++]=iRow;
#ifdef CLP_PRIMAL_SLACK_MULTIPLIER 
	    infeasibilities[iRow]=value*value*CLP_PRIMAL_SLACK_MULTIPLIER;
#else
	    infeasibilities[iRow]=value*value;
#endif
	  } else {
	    if (infeasibilities[iRow])
	      infeasibilities[iRow]=COIN_INDEXED_REALLY_TINY_ELEMENT;
	  }
	}
      }
    } else if (scaleFactor!=COIN_DBL_MAX) {
      for (int i=0;i<numberSlacks;i++) {
	int iRow=piIndexTableau[i];
	int iStatus=internalStatus[iRow]&7;
	if (iStatus<4) {
	  double value=reducedCost[iRow];
	  value += scaleFactor*piTableau[iRow]; // sign?
	  reducedCost[iRow]=value;
	  value *= mult[iStatus];
	  if (value<0.0) {
	    if (!infeasibilities[iRow])
	      newInfeas[numberNewInfeas++]=iRow;
#ifdef CLP_PRIMAL_SLACK_MULTIPLIER 
	    infeasibilities[iRow]=value*value*CLP_PRIMAL_SLACK_MULTIPLIER;
#else
	    infeasibilities[iRow]=value*value;
#endif
	  } else {
	    if (infeasibilities[iRow])
	      infeasibilities[iRow]=COIN_INDEXED_REALLY_TINY_ELEMENT;
	  }
	}
      }
    }
    if (updateWeights) {
      for (int i=0;i<numberSlacks;i++) {
	int iRow=piIndexTableau[i];
	double modification=piWeights[iRow];
	double thisWeight = weights[iRow];
	double pivot = piTableau[iRow];
	double pivotSquared = pivot * pivot;
	thisWeight += pivotSquared * devex - pivot * modification;
	if (thisWeight < DEVEX_TRY_NORM) {
	  if (referenceIn < 0.0) {
	    // steepest
	    thisWeight = CoinMax(DEVEX_TRY_NORM, DEVEX_ADD_ONE + pivotSquared);
	  } else {
	    // exact
	    thisWeight = referenceIn * pivotSquared;
	    if (isReference(iRow))
	      thisWeight += 1.0;
	    thisWeight = CoinMax(thisWeight, DEVEX_TRY_NORM);
	  }
	}
	weights[iRow] = thisWeight;
      }
    }
    first=maximumRows;
  }
  double zeroTolerance = model_->zeroTolerance();
  double freeTolerance = FREE_ACCEPT * tolerance;;
  // get matrix data pointers
  const int * COIN_RESTRICT row = matrix_->getIndices();
  const CoinBigIndex * COIN_RESTRICT columnStart = matrix_->getVectorStarts()-maximumRows;
  const double * COIN_RESTRICT elementByColumn = matrix_->getElements();
  bool updateDjs=scaleFactor!=COIN_DBL_MAX;
  for (int iSequence = first; iSequence < last; iSequence++) {
    unsigned char iStatus=internalStatus[iSequence]&7;
    if (iStatus<6) {
      CoinBigIndex start = columnStart[iSequence];
      CoinBigIndex next = columnStart[iSequence+1];
      double tableauValue = 0.0;
      double djValue=reducedCost[iSequence];
      if (!scaleFactor) {
	for (CoinBigIndex j = start; j < next; j++) {
	  int jRow = row[j];
	  tableauValue += piTableau[jRow] * elementByColumn[j];
	  djValue += pi[jRow] * elementByColumn[j];
	}
      } else {
	for (CoinBigIndex j = start; j < next; j++) {
	  int jRow = row[j];
	  tableauValue += piTableau[jRow] * elementByColumn[j];
	}
	djValue += tableauValue*scaleFactor; // sign?
      }
      if (updateDjs) {
	reducedCost[iSequence]=djValue;
	if (iStatus<4) {
	  djValue *= mult[iStatus];
	  if (djValue<0.0) {
	    if (!infeasibilities[iSequence])
	      newInfeas[numberNewInfeas++]=iSequence;
	    infeasibilities[iSequence]=djValue*djValue;
	  } else {
	    if (infeasibilities[iSequence])
	      infeasibilities[iSequence]=COIN_INDEXED_REALLY_TINY_ELEMENT;
	  }
	} else {
	  if (fabs(djValue) > freeTolerance) {
	    // we are going to bias towards free (but only if reasonable)
	    djValue *= FREE_BIAS;
	    if (!infeasibilities[iSequence])
	      newInfeas[numberNewInfeas++]=iSequence;
	    // store square in list
	    infeasibilities[iSequence] = djValue * djValue;
	  } else {
	    if (infeasibilities[iSequence])
	      infeasibilities[iSequence]=COIN_INDEXED_REALLY_TINY_ELEMENT;
	  }
	}
      }
      if (fabs(tableauValue)>zeroTolerance&&updateWeights) {
	double modification=0.0;
	for (CoinBigIndex j = start; j < next; j++) {
	  int jRow = row[j];
	  modification += piWeights[jRow] * elementByColumn[j];
	}
	double thisWeight = weights[iSequence];
	double pivot = tableauValue;
	double pivotSquared = pivot * pivot;
	thisWeight += pivotSquared * devex - pivot * modification;
	if (thisWeight < DEVEX_TRY_NORM) {
	  if (referenceIn < 0.0) {
	    // steepest
	    thisWeight = CoinMax(DEVEX_TRY_NORM, DEVEX_ADD_ONE + pivotSquared);
	  } else {
	    // exact
	    thisWeight = referenceIn * pivotSquared;
	    if (isReference(iSequence))
	      thisWeight += 1.0;
	    thisWeight = CoinMax(thisWeight, DEVEX_TRY_NORM);
	  }
	}
	weights[iSequence] = thisWeight;
      }
    }
  }
  spareColumn1.setTempNumElementsPartition(iBlock,numberNewInfeas);
  int bestSequence=-1;
#if 0
  if (!iBlock)
    first=0;
  // not at present - maybe later
  double bestDj=tolerance*tolerance;
  for (int iSequence=first;iSequence<last;iSequence++) {
    if (infeasibilities[iSequence]>bestDj*weights[iSequence]) {
      int iStatus=internalStatus[iSequence]&7;
      assert (iStatus<6);
      bestSequence=iSequence;
      bestDj=infeasibilities[iSequence]/weights[iSequence];
    }
  }
#endif
  return bestSequence;
}
/* does steepest edge double or triple update
   If scaleFactor!=0 then use with tableau row to update djs
   otherwise use updateForDjs
*/
int
AbcMatrix::primalColumnSparseDouble(int iBlock,CoinPartitionedVector & updateForTableauRow,
			      CoinPartitionedVector & updateForDjs,
			      const CoinIndexedVector & updateForWeights,
			      CoinPartitionedVector & spareColumn1,
			      double * infeasibilities, 
			      double referenceIn, double devex,
			      // Array for exact devex to say what is in reference framework
			      unsigned int * reference,
			      double * weights, double scaleFactor) const
{
  int maximumRows = model_->maximumAbcNumberRows();
  int first=blockStart_[iBlock];
  //int last=blockStart_[iBlock+1];
  const double *  COIN_RESTRICT piTableau=updateForTableauRow.denseVector();
  //const double *  COIN_RESTRICT pi=updateForDjs.denseVector();
  const double *  COIN_RESTRICT piWeights=updateForWeights.denseVector();
  const unsigned char *  COIN_RESTRICT internalStatus = model_->internalStatus();
  double * COIN_RESTRICT reducedCost = model_->djRegion();
  double tolerance = model_->currentDualTolerance();
  // use spareColumn to track new infeasibilities
  int * COIN_RESTRICT newInfeas = spareColumn1.getIndices()+first;
  int numberNewInfeas=0;
  // we can't really trust infeasibilities if there is dual error
  // this coding has to mimic coding in checkDualSolution
  double error = CoinMin(1.0e-2, model_->largestDualError());
  // allow tolerance at least slightly bigger than standard
  tolerance = tolerance +  error;
  double mult[2]={1.0,-1.0};
  bool updateWeights=devex!=0.0;
  int numberSlacks=updateForTableauRow.getNumElements();
  const int *  COIN_RESTRICT piIndexTableau = updateForTableauRow.getIndices();
#define isReference(i) (((reference[i>>5] >> (i & 31)) & 1) != 0)
  // Scale factor is other way round so tableau row is 1.0* while dj update is scaleFactor*
  assert (scaleFactor);
  if (!iBlock) {
    if (scaleFactor!=COIN_DBL_MAX) {
      for (int i=0;i<numberSlacks;i++) {
	int iRow=piIndexTableau[i];
	int iStatus=internalStatus[iRow]&7;
	if (iStatus<4) {
	  double value=reducedCost[iRow];
	  value += scaleFactor*piTableau[iRow]; // sign?
	  reducedCost[iRow]=value;
	  value *= mult[iStatus];
	  if (value<0.0) {
	    if (!infeasibilities[iRow])
	      newInfeas[numberNewInfeas++]=iRow;
#ifdef CLP_PRIMAL_SLACK_MULTIPLIER 
	    infeasibilities[iRow]=value*value*CLP_PRIMAL_SLACK_MULTIPLIER;
#else
	    infeasibilities[iRow]=value*value;
#endif
	  } else {
	    if (infeasibilities[iRow])
	      infeasibilities[iRow]=COIN_INDEXED_REALLY_TINY_ELEMENT;
	  }
	}
      }
    }
    if (updateWeights) {
      for (int i=0;i<numberSlacks;i++) {
	int iRow=piIndexTableau[i];
	double modification=piWeights[iRow];
	double thisWeight = weights[iRow];
	double pivot = piTableau[iRow];
	double pivotSquared = pivot * pivot;
	thisWeight += pivotSquared * devex - pivot * modification;
	if (thisWeight < DEVEX_TRY_NORM) {
	  if (referenceIn < 0.0) {
	    // steepest
	    thisWeight = CoinMax(DEVEX_TRY_NORM, DEVEX_ADD_ONE + pivotSquared);
	  } else {
	    // exact
	    thisWeight = referenceIn * pivotSquared;
	    if (isReference(iRow))
	      thisWeight += 1.0;
	    thisWeight = CoinMax(thisWeight, DEVEX_TRY_NORM);
	  }
	}
	weights[iRow] = thisWeight;
      }
    }
    first=maximumRows;
  }
  double zeroTolerance = model_->zeroTolerance();
  double freeTolerance = FREE_ACCEPT * tolerance;;
  // get matrix data pointers
  const int * COIN_RESTRICT row = matrix_->getIndices();
  const CoinBigIndex * COIN_RESTRICT columnStart = matrix_->getVectorStarts()-maximumRows;
  const double * COIN_RESTRICT elementByColumn = matrix_->getElements();
  // get tableau row
  int *  COIN_RESTRICT index = updateForTableauRow.getIndices()+first;
  double *  COIN_RESTRICT array = updateForTableauRow.denseVector();
  int numberRows=model_->numberRows();
  const CoinBigIndex * COIN_RESTRICT rowStart = rowStart_+iBlock*numberRows;
  const CoinBigIndex * COIN_RESTRICT rowEnd = rowStart+numberRows;
  const double * COIN_RESTRICT element = element_;
  const int * COIN_RESTRICT column = column_;
  int numberInRow=0;
  if (numberSlacks>2) {
    for (int i=0;i<numberSlacks;i++) {
      int iRow=piIndexTableau[i];
      double piValue = piTableau[iRow];
      CoinBigIndex end = rowEnd[iRow];
      for (CoinBigIndex j=rowStart[iRow];j<end;j++) {
	int iSequence = column[j];
	double value = element[j]*piValue;
	double oldValue=array[iSequence];
	value += oldValue;
	if (!oldValue) {
	  array[iSequence]= value; 
	  index[numberInRow++]=iSequence;
	} else if (value) {	
	  array[iSequence] = value; 
	} else {
	  array[iSequence] = COIN_INDEXED_REALLY_TINY_ELEMENT;
	}
      }
    }
  } else if (numberSlacks==2) {
    int iRow0 = piIndexTableau[0];
    int iRow1 = piIndexTableau[1];
    CoinBigIndex end0 = rowEnd[iRow0];
    CoinBigIndex end1 = rowEnd[iRow1];
    if (end0-rowStart[iRow0]>end1-rowStart[iRow1]) {
      int temp=iRow0;
      iRow0=iRow1;
      iRow1=temp;
    }
    CoinBigIndex start = rowStart[iRow0];
    CoinBigIndex end = rowEnd[iRow0];
    double piValue = piTableau[iRow0];
    for (CoinBigIndex j=start;j<end;j++) {
      int iSequence = column[j];
      double value = element[j];
      array[iSequence]= piValue*value; 
      index[numberInRow++]=iSequence;
    }
    start = rowStart[iRow1];
    end = rowEnd[iRow1];
    piValue = piTableau[iRow1];
    for (CoinBigIndex j=start;j<end;j++) {
      int iSequence = column[j];
      double value = element[j];
      value *= piValue;
      if (!array[iSequence]) {
	array[iSequence]= value; 
	index[numberInRow++]=iSequence;
      } else {	
	value += array[iSequence];
	if (value)
	  array[iSequence]= value; 
	else
	  array[iSequence] = COIN_INDEXED_REALLY_TINY_ELEMENT;
      }
    }
  } else {
    int iRow0 = piIndexTableau[0];
    CoinBigIndex start = rowStart[iRow0];
    CoinBigIndex end = rowEnd[iRow0];
    double piValue = piTableau[iRow0];
    for (CoinBigIndex j=start;j<end;j++) {
      int iSequence = column[j];
      double value = element[j];
      array[iSequence]= piValue*value; 
      index[numberInRow++]=iSequence;
    }
  }
  bool updateDjs=scaleFactor!=COIN_DBL_MAX;
  for (int iLook =0 ;iLook<numberInRow;iLook++) {
    int iSequence=index[iLook];
    unsigned char iStatus=internalStatus[iSequence]&7;
    if (iStatus<6) {
      double tableauValue = array[iSequence];
      array[iSequence]=0.0;
      double djValue=reducedCost[iSequence];
      djValue += tableauValue*scaleFactor; // sign?
      if (updateDjs) {
	reducedCost[iSequence]=djValue;
	if (iStatus<4) {
	  djValue *= mult[iStatus];
	  if (djValue<0.0) {
	    if (!infeasibilities[iSequence])
	      newInfeas[numberNewInfeas++]=iSequence;
	    infeasibilities[iSequence]=djValue*djValue;
	  } else {
	    if (infeasibilities[iSequence])
	      infeasibilities[iSequence]=COIN_INDEXED_REALLY_TINY_ELEMENT;
	  }
	} else {
	  if (fabs(djValue) > freeTolerance) {
	    // we are going to bias towards free (but only if reasonable)
	    djValue *= FREE_BIAS;
	    if (!infeasibilities[iSequence])
	      newInfeas[numberNewInfeas++]=iSequence;
	    // store square in list
	    infeasibilities[iSequence] = djValue * djValue;
	  } else {
	    if (infeasibilities[iSequence])
	      infeasibilities[iSequence]=COIN_INDEXED_REALLY_TINY_ELEMENT;
	  }
	}
      }
      if (fabs(tableauValue)>zeroTolerance&&updateWeights) {
	CoinBigIndex start = columnStart[iSequence];
	CoinBigIndex next = columnStart[iSequence+1];
	double modification=0.0;
	for (CoinBigIndex j = start; j < next; j++) {
	  int jRow = row[j];
	  modification += piWeights[jRow] * elementByColumn[j];
	}
	double thisWeight = weights[iSequence];
	double pivot = tableauValue;
	double pivotSquared = pivot * pivot;
	thisWeight += pivotSquared * devex - pivot * modification;
	if (thisWeight < DEVEX_TRY_NORM) {
	  if (referenceIn < 0.0) {
	    // steepest
	    thisWeight = CoinMax(DEVEX_TRY_NORM, DEVEX_ADD_ONE + pivotSquared);
	  } else {
	    // exact
	    thisWeight = referenceIn * pivotSquared;
	    if (isReference(iSequence))
	      thisWeight += 1.0;
	    thisWeight = CoinMax(thisWeight, DEVEX_TRY_NORM);
	  }
	}
	weights[iSequence] = thisWeight;
      }
    } else {
      array[iSequence]=0.0;
    }
  }
  spareColumn1.setTempNumElementsPartition(iBlock,numberNewInfeas);
  int bestSequence=-1;
#if 0
  if (!iBlock)
    first=0;
  // not at present - maybe later
  double bestDj=tolerance*tolerance;
  for (int iSequence=first;iSequence<last;iSequence++) {
    if (infeasibilities[iSequence]>bestDj*weights[iSequence]) {
      int iStatus=internalStatus[iSequence]&7;
      assert (iStatus<6);
      bestSequence=iSequence;
      bestDj=infeasibilities[iSequence]/weights[iSequence];
    }
  }
#endif
  return bestSequence;
}
#if 0
/* Chooses best weighted dj
 */
int 
AbcMatrix::chooseBestDj(int iBlock,const CoinIndexedVector & infeasibilities, 
			const double * weights) const
{
  return 0;
}
#endif
/* does steepest edge double or triple update
   If scaleFactor!=0 then use with tableau row to update djs
   otherwise use updateForDjs
   Returns best
*/
int
AbcMatrix::primalColumnDouble(CoinPartitionedVector & updateForTableauRow,
			      CoinPartitionedVector & updateForDjs,
			      const CoinIndexedVector & updateForWeights,
			      CoinPartitionedVector & spareColumn1,
			      CoinIndexedVector & infeasible, 
			      double referenceIn, double devex,
			      // Array for exact devex to say what is in reference framework
			      unsigned int * reference,
			      double * weights, double scaleFactor) const
{
  //int maximumRows = model_->maximumAbcNumberRows();
   // rebalance
   rebalance();
#ifdef PRICE_IN_ABC_MATRIX
   int which[NUMBER_BLOCKS];
#endif
   double * infeasibilities=infeasible.denseVector();
   int bestSequence=-1;
   // see if worth using row copy
   int maximumRows=model_->maximumAbcNumberRows();
   int number=updateForTableauRow.getNumElements();
   assert (number);
   bool useRowCopy = (gotRowCopy()&&(number<<2)<maximumRows);
   //useRowCopy=false;
   if (!scaleFactor)
     useRowCopy=false; // look again later
   const int * starts;
   int numberBlocks;
   if (useRowCopy) {
     starts=blockStart_;
     numberBlocks=numberRowBlocks_;
   } else {
     starts=startColumnBlock_;
     numberBlocks=numberColumnBlocks_;
   }
   if (useRowCopy) {
     for (int i=0;i<numberBlocks;i++) 
#ifdef PRICE_IN_ABC_MATRIX
       which[i]=
#endif
	 cilk_spawn primalColumnSparseDouble(i,updateForTableauRow,updateForDjs,updateForWeights,
				       spareColumn1,
				       infeasibilities,referenceIn,devex,reference,weights,scaleFactor);
     cilk_sync;
   } else {
     for (int i=0;i<numberBlocks;i++) 
#ifdef PRICE_IN_ABC_MATRIX
       which[i]=
#endif
	 cilk_spawn primalColumnDouble(i,updateForTableauRow,updateForDjs,updateForWeights,
				       spareColumn1,
				       infeasibilities,referenceIn,devex,reference,weights,scaleFactor);
     cilk_sync;
   }
#ifdef PRICE_IN_ABC_MATRIX
   double bestValue=model_->dualTolerance();
   int sequenceIn[8]={-1,-1,-1,-1,-1,-1,-1,-1};
#endif
   assert (numberColumnBlocks_<=8);
#ifdef PRICE_IN_ABC_MATRIX
   double weightedDj[8];
   int nPossible=0;
#endif
   //const double * reducedCost = model_->djRegion();
   // use spareColumn to track new infeasibilities
   int numberInfeas=infeasible.getNumElements();
   int * COIN_RESTRICT infeas = infeasible.getIndices();
   const int * COIN_RESTRICT newInfeasAll = spareColumn1.getIndices();
   for (int i=0;i<numberColumnBlocks_;i++) {
     const int * COIN_RESTRICT newInfeas = newInfeasAll+starts[i];
     int numberNewInfeas=spareColumn1.getNumElements(i);
     spareColumn1.setTempNumElementsPartition(i,0);
     for (int j=0;j<numberNewInfeas;j++)
       infeas[numberInfeas++]=newInfeas[j];
#ifdef PRICE_IN_ABC_MATRIX
     if (which[i]>=0) {
       int iSequence=which[i];
       double value=fabs(infeasibilities[iSequence]/weights[iSequence]);
       if (value>bestValue) {
	 bestValue=value;
	 bestSequence=which[i];
       }
       weightedDj[nPossible]=-value;
       sequenceIn[nPossible++]=iSequence;
     }
#endif
   }
   infeasible.setNumElements(numberInfeas);
#ifdef PRICE_IN_ABC_MATRIX
   CoinSort_2(weightedDj,weightedDj+nPossible,sequenceIn);
   //model_->setMultipleSequenceIn(sequenceIn);
#endif
   return bestSequence;

}
// Partial pricing
void
AbcMatrix::partialPricing(double startFraction, double endFraction,
			  int & bestSequence, int & numberWanted)
{
  numberWanted = currentWanted_;
  int maximumRows = model_->maximumAbcNumberRows();
  // get matrix data pointers
  const int * COIN_RESTRICT row = matrix_->getIndices();
  const CoinBigIndex * COIN_RESTRICT columnStart = matrix_->getVectorStarts()-maximumRows;
  const double * COIN_RESTRICT elementByColumn = matrix_->getElements();
  int numberColumns=model_->numberColumns();
  int start = static_cast<int> (startFraction * numberColumns);
  int end = CoinMin(static_cast<int> (endFraction * numberColumns + 1), numberColumns);
  // adjust
  start += maximumRows;
  end += maximumRows;
  double tolerance = model_->currentDualTolerance();
  double * reducedCost = model_->djRegion();
  const double * duals = model_->dualRowSolution();
  const double * cost = model_->costRegion();
  double bestDj;
  if (bestSequence >= 0)
    bestDj = fabs(reducedCost[bestSequence]);
  else
    bestDj = tolerance;
  int sequenceOut = model_->sequenceOut();
  int saveSequence = bestSequence;
  int lastScan = minimumObjectsScan_ < 0 ? end : start + minimumObjectsScan_;
  int minNeg = minimumGoodReducedCosts_ == -1 ? numberWanted : minimumGoodReducedCosts_;
  for (int iSequence = start; iSequence < end; iSequence++) {
    if (iSequence != sequenceOut) {
      double value;
      AbcSimplex::Status status = model_->getInternalStatus(iSequence);
      switch(status) {
      case AbcSimplex::basic:
      case AbcSimplex::isFixed:
	break;
      case AbcSimplex::isFree:
      case AbcSimplex::superBasic:
	value = cost[iSequence];
	for (CoinBigIndex j = columnStart[iSequence];
	     j < columnStart[iSequence+1]; j++) {
	  int jRow = row[j];
	  value -= duals[jRow] * elementByColumn[j];
	}
	value = fabs(value);
	if (value > FREE_ACCEPT * tolerance) {
	  numberWanted--;
	  // we are going to bias towards free (but only if reasonable)
	  value *= FREE_BIAS;
	  if (value > bestDj) {
	    // check flagged variable and correct dj
	    if (!model_->flagged(iSequence)) {
	      bestDj = value;
	      bestSequence = iSequence;
	    } else {
	      // just to make sure we don't exit before got something
	      numberWanted++;
	    }
	  }
	}
	break;
      case AbcSimplex::atUpperBound:
	value = cost[iSequence];
	// scaled
	for (CoinBigIndex j = columnStart[iSequence];
	     j < columnStart[iSequence+1]; j++) {
	  int jRow = row[j];
	  value -= duals[jRow] * elementByColumn[j];
	}
	if (value > tolerance) {
	  numberWanted--;
	  if (value > bestDj) {
	    // check flagged variable and correct dj
	    if (!model_->flagged(iSequence)) {
	      bestDj = value;
	      bestSequence = iSequence;
	    } else {
	      // just to make sure we don't exit before got something
	      numberWanted++;
	    }
	  }
	}
	break;
      case AbcSimplex::atLowerBound:
	value = cost[iSequence];
	for (CoinBigIndex j = columnStart[iSequence];
	     j < columnStart[iSequence+1]; j++) {
	  int jRow = row[j];
	  value -= duals[jRow] * elementByColumn[j];
	}
	value = -value;
	if (value > tolerance) {
	  numberWanted--;
	  if (value > bestDj) {
	    // check flagged variable and correct dj
	    if (!model_->flagged(iSequence)) {
	      bestDj = value;
	      bestSequence = iSequence;
	    } else {
	      // just to make sure we don't exit before got something
	      numberWanted++;
	    }
	  }
	}
	break;
      }
    }
    if (numberWanted + minNeg < originalWanted_ && iSequence > lastScan) {
      // give up
      break;
    }
    if (!numberWanted)
      break;
  }
  if (bestSequence != saveSequence) {
    // recompute dj
    double value = cost[bestSequence];
    for (CoinBigIndex j = columnStart[bestSequence];
	 j < columnStart[bestSequence+1]; j++) {
      int jRow = row[j];
      value -= duals[jRow] * elementByColumn[j];
    }
    reducedCost[bestSequence] = value;
    savedBestSequence_ = bestSequence;
    savedBestDj_ = reducedCost[savedBestSequence_];
  }
  currentWanted_ = numberWanted;
}
/* Return <code>x *A</code> in <code>z</code> but
   just for indices Already in z.
   Note - z always packed mode */
void 
AbcMatrix::subsetTransposeTimes(const CoinIndexedVector & x,
				CoinIndexedVector & z) const
{
  int maximumRows = model_->maximumAbcNumberRows();
  // get matrix data pointers
  const int * COIN_RESTRICT row = matrix_->getIndices();
  const CoinBigIndex * COIN_RESTRICT columnStart = matrix_->getVectorStarts()-maximumRows;
  const double * COIN_RESTRICT elementByColumn = matrix_->getElements();
  const double *  COIN_RESTRICT other=x.denseVector();
  int numberNonZero = z.getNumElements();
  int *  COIN_RESTRICT index = z.getIndices();
  double *  COIN_RESTRICT array = z.denseVector();
  int numberRows=model_->numberRows();
  for (int i=0;i<numberNonZero;i++) {
    int iSequence=index[i];
    if (iSequence>=numberRows) {
      CoinBigIndex start = columnStart[iSequence];
      CoinBigIndex next = columnStart[iSequence+1];
      double value = 0.0;
      for (CoinBigIndex j = start; j < next; j++) {
	int jRow = row[j];
	value -= other[jRow] * elementByColumn[j];
      }
      array[i]=value;
    } else { 
      array[i]=-other[iSequence];
    }
  }
}
// Return <code>-x *A</code> in <code>z</code> 
void 
AbcMatrix::transposeTimes(const CoinIndexedVector & x,
				CoinIndexedVector & z) const
{
  int maximumRows = model_->maximumAbcNumberRows();
  // get matrix data pointers
  const int * COIN_RESTRICT row = matrix_->getIndices();
  const CoinBigIndex * COIN_RESTRICT columnStart = matrix_->getVectorStarts()-maximumRows;
  const double * COIN_RESTRICT elementByColumn = matrix_->getElements();
  const double *  COIN_RESTRICT other=x.denseVector();
  int numberNonZero = 0;
  int *  COIN_RESTRICT index = z.getIndices();
  double *  COIN_RESTRICT array = z.denseVector();
  int numberColumns=model_->numberColumns();
  double zeroTolerance=model_->zeroTolerance();
  for (int iSequence=maximumRows;iSequence<maximumRows+numberColumns;iSequence++) {
    CoinBigIndex start = columnStart[iSequence];
    CoinBigIndex next = columnStart[iSequence+1];
    double value = 0.0;
    for (CoinBigIndex j = start; j < next; j++) {
      int jRow = row[j];
      value -= other[jRow] * elementByColumn[j];
    }
    if (fabs(value)>zeroTolerance) {
      // TEMP
      array[iSequence-maximumRows]=value;
      index[numberNonZero++]=iSequence-maximumRows;
    }
  }
  z.setNumElements(numberNonZero);
}
/* Return A * scalar(+-1) *x + y</code> in <code>y</code>.
   @pre <code>x</code> must be of size <code>numRows()</code>
   @pre <code>y</code> must be of size <code>numRows()+numColumns()</code> */
void 
AbcMatrix::transposeTimesNonBasic(double scalar,
				  const double * pi, double * y) const
{
  int numberTotal = model_->numberTotal();
  int maximumRows = model_->maximumAbcNumberRows();
  assert (scalar==-1.0);
  // get matrix data pointers
  const int * COIN_RESTRICT row = matrix_->getIndices();
  const CoinBigIndex * COIN_RESTRICT columnStart = matrix_->getVectorStarts()-maximumRows;
  const double * COIN_RESTRICT elementByColumn = matrix_->getElements();
  int numberRows=model_->numberRows();
  const unsigned char *  COIN_RESTRICT status=model_->internalStatus();
  for (int i=0;i<numberRows;i++) {
    y[i]+=scalar*pi[i];
  }
  for (int iColumn = maximumRows; iColumn < numberTotal;iColumn++) {
    unsigned char type=status[iColumn]&7;
    if (type<6) {
      CoinBigIndex start = columnStart[iColumn];
      CoinBigIndex next = columnStart[iColumn+1];
      double value = 0.0;
      for (CoinBigIndex j = start; j < next; j++) {
	int jRow = row[j];
	value += scalar*pi[jRow] * elementByColumn[j];
      }
      y[iColumn]+=value;
    } else {
      y[iColumn]=0.0; // may not be but .....
    }
  }
}
/* Return y - A * x</code> in <code>y</code>.
   @pre <code>x</code> must be of size <code>numRows()</code>
   @pre <code>y</code> must be of size <code>numRows()+numColumns()</code> */
void 
AbcMatrix::transposeTimesAll(const double * pi, double * y) const
{
  int numberTotal = model_->numberTotal();
  int maximumRows = model_->maximumAbcNumberRows();
  // get matrix data pointers
  const int * COIN_RESTRICT row = matrix_->getIndices();
  const CoinBigIndex * COIN_RESTRICT columnStart = matrix_->getVectorStarts()-maximumRows;
  const double * COIN_RESTRICT elementByColumn = matrix_->getElements();
  int numberRows=model_->numberRows();
  for (int i=0;i<numberRows;i++) {
    y[i]-=pi[i];
  }
  for (int iColumn = maximumRows; iColumn < numberTotal;iColumn++) {
    CoinBigIndex start = columnStart[iColumn];
    CoinBigIndex next = columnStart[iColumn+1];
    double value = 0.0;
    for (CoinBigIndex j = start; j < next; j++) {
      int jRow = row[j];
      value += pi[jRow] * elementByColumn[j];
    }
    y[iColumn]-=value;
  }
}
/* Return y  + A * scalar(+-1) *x</code> in <code>y</code>.
   @pre <code>x</code> must be of size <code>numRows()</code>
   @pre <code>y</code> must be of size <code>numRows()</code> */
void 
AbcMatrix::transposeTimesBasic(double scalar,
			       const double * pi, double * y) const
{
  assert (scalar==-1.0);
  int numberRows=model_->numberRows();
  //AbcMemset0(y,numberRows);
  // get matrix data pointers
  const int * COIN_RESTRICT row = matrix_->getIndices();
  const CoinBigIndex * COIN_RESTRICT columnStart = matrix_->getVectorStarts()-model_->maximumAbcNumberRows();
  const double * COIN_RESTRICT elementByColumn = matrix_->getElements();
  const int *  COIN_RESTRICT pivotVariable = model_->pivotVariable();
  for (int i=0;i<numberRows;i++) {
    int iSequence=pivotVariable[i];
    double value;
    if (iSequence>=numberRows) {
      CoinBigIndex start = columnStart[iSequence];
      CoinBigIndex next = columnStart[iSequence+1];
      value = 0.0;
      for (CoinBigIndex j = start; j < next; j++) {
	int jRow = row[j];
	value += pi[jRow] * elementByColumn[j];
      }
    } else {
      value=pi[iSequence];
    }
    y[i]+=scalar*value;
  }
}
// Adds multiple of a column (or slack) into an CoinIndexedvector
void 
AbcMatrix::add(CoinIndexedVector & rowArray, int iSequence, double multiplier) const 
{
  int maximumRows=model_->maximumAbcNumberRows();
  if (iSequence>=maximumRows) {
    const int * COIN_RESTRICT row = matrix_->getIndices();
    iSequence -= maximumRows;
    const CoinBigIndex * COIN_RESTRICT columnStart = matrix_->getVectorStarts();
    const double * COIN_RESTRICT elementByColumn = matrix_->getElements();
    CoinBigIndex start = columnStart[iSequence];
    CoinBigIndex next = columnStart[iSequence+1];
    for (CoinBigIndex j = start; j < next; j++) {
      int jRow = row[j];
      rowArray.quickAdd(jRow,elementByColumn[j]*multiplier);
    }
  } else {
    rowArray.quickAdd(iSequence,multiplier);
  } 
}
/* Unpacks a column into an CoinIndexedVector
 */
void 
AbcMatrix::unpack(CoinIndexedVector & usefulArray,
		  int sequence) const 
{
  usefulArray.clear();
  int maximumRows=model_->maximumAbcNumberRows();
  if (sequence < maximumRows) {
    //slack
    usefulArray.insert(sequence , 1.0);
  } else {
    // column
    const CoinBigIndex *  COIN_RESTRICT columnStart = matrix()->getVectorStarts()-maximumRows;
    CoinBigIndex start=columnStart[sequence];
    CoinBigIndex end=columnStart[sequence+1];
    const int *  COIN_RESTRICT row = matrix()->getIndices()+start;
    const double *  COIN_RESTRICT elementByColumn = matrix()->getElements()+start;
    int *  COIN_RESTRICT index = usefulArray.getIndices();
    double *  COIN_RESTRICT array = usefulArray.denseVector();
    int numberNonZero=end-start;
    for (int j=0; j<numberNonZero;j++) {
      int iRow=row[j];
      index[j]=iRow;
      array[iRow]=elementByColumn[j];
    }
    usefulArray.setNumElements(numberNonZero);
  }
}
// Row starts
CoinBigIndex * 
AbcMatrix::rowStart() const
{
  return rowStart_;
}
// Row ends
CoinBigIndex * 
AbcMatrix::rowEnd() const
{
  //if (numberRowBlocks_<2) {
  //return rowStart_+1;
  //} else {
    return rowStart_+model_->numberRows()*(numberRowBlocks_+1);
    //}
}
// Row elements
double * 
AbcMatrix::rowElements() const
{
  return element_;
}
// Row columnss
CoinSimplexInt * 
AbcMatrix::rowColumns() const
{
  return column_;
}
