/* $Id$ */
// Copyright (C) 2008, International Business Machines
// Corporation and others, Copyright (C) 2012, FasterCoin.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include "CoinUtilsConfig.h"
#include "CoinPragma.hpp"

#include <cassert>
#include <cstdio>

#include "CoinAbcDenseFactorization.hpp"
#include "CoinAbcCommonFactorization.hpp"
#include "CoinIndexedVector.hpp"
#include "CoinHelperFunctions.hpp"
#include "CoinPackedMatrix.hpp"
#include "CoinFinite.hpp"
//:class CoinAbcDenseFactorization.  Deals with Factorization and Updates
//  CoinAbcDenseFactorization.  Constructor
CoinAbcDenseFactorization::CoinAbcDenseFactorization (  )
  : CoinAbcAnyFactorization()
{
  gutsOfInitialize();
}

/// Copy constructor 
CoinAbcDenseFactorization::CoinAbcDenseFactorization ( const CoinAbcDenseFactorization &other)
  : CoinAbcAnyFactorization(other)
{
  gutsOfInitialize();
  gutsOfCopy(other);
}
// Clone
CoinAbcAnyFactorization * 
CoinAbcDenseFactorization::clone() const 
{
  return new CoinAbcDenseFactorization(*this);
}
/// The real work of constructors etc
void CoinAbcDenseFactorization::gutsOfDestructor()
{
  delete [] elements_;
  delete [] pivotRow_;
  delete [] workArea_;
  elements_ = NULL;
  pivotRow_ = NULL;
  workArea_ = NULL;
  numberRows_ = 0;
  numberGoodU_ = 0;
  status_ = -1;
  maximumRows_=0;
#if ABC_PARALLEL==2
  parallelMode_=0;
#endif
  maximumSpace_=0;
  maximumRowsAdjusted_=0;
  solveMode_=0;
}
void CoinAbcDenseFactorization::gutsOfInitialize()
{
  pivotTolerance_ = 1.0e-1;
  minimumPivotTolerance_ = 1.0e-1;
  zeroTolerance_ = 1.0e-13;
  areaFactor_=1.0;
  numberDense_=0;
  maximumPivots_=200;
  relaxCheck_=1.0;
  numberRows_ = 0;
  numberGoodU_ = 0;
  status_ = -1;
  numberSlacks_ = 0;
  numberPivots_ = 0;
  maximumRows_=0;
#if ABC_PARALLEL==2
  parallelMode_=0;
#endif
  maximumSpace_=0;
  maximumRowsAdjusted_=0;
  elements_ = NULL;
  pivotRow_ = NULL;
  workArea_ = NULL;
  solveMode_=0;
}
//  ~CoinAbcDenseFactorization.  Destructor
CoinAbcDenseFactorization::~CoinAbcDenseFactorization (  )
{
  gutsOfDestructor();
}
//  =
CoinAbcDenseFactorization & CoinAbcDenseFactorization::operator = ( const CoinAbcDenseFactorization & other ) {
  if (this != &other) {    
    gutsOfDestructor();
    gutsOfInitialize();
    gutsOfCopy(other);
  }
  return *this;
}
#define WORK_MULT 2
void CoinAbcDenseFactorization::gutsOfCopy(const CoinAbcDenseFactorization &other)
{
  pivotTolerance_ = other.pivotTolerance_;
  minimumPivotTolerance_ = other.minimumPivotTolerance_;
  zeroTolerance_ = other.zeroTolerance_;
  areaFactor_=other.areaFactor_;
  numberDense_=other.numberDense_;
  relaxCheck_ = other.relaxCheck_;
  numberRows_ = other.numberRows_;
  maximumRows_ = other.maximumRows_;
#if ABC_PARALLEL==2
  parallelMode_=other.parallelMode_;
#endif
  maximumSpace_ = other.maximumSpace_;
  maximumRowsAdjusted_ = other.maximumRowsAdjusted_;
  solveMode_ = other.solveMode_;
  numberGoodU_ = other.numberGoodU_;
  maximumPivots_ = other.maximumPivots_;
  numberPivots_ = other.numberPivots_;
  factorElements_ = other.factorElements_;
  status_ = other.status_;
  numberSlacks_ = other.numberSlacks_;
  if (other.pivotRow_) {
    pivotRow_ = new int [2*maximumRowsAdjusted_+maximumPivots_];
    CoinMemcpyN(other.pivotRow_,(2*maximumRowsAdjusted_+numberPivots_),pivotRow_);
    elements_ = new CoinFactorizationDouble [maximumSpace_];
    CoinMemcpyN(other.elements_,(maximumRowsAdjusted_+numberPivots_)*maximumRowsAdjusted_,elements_);
    workArea_ = new CoinFactorizationDouble [maximumRowsAdjusted_*WORK_MULT];
    CoinZeroN(workArea_,maximumRowsAdjusted_*WORK_MULT);
  } else {
    elements_ = NULL;
    pivotRow_ = NULL;
    workArea_ = NULL;
  }
}

//  getAreas.  Gets space for a factorization
//called by constructors
void
CoinAbcDenseFactorization::getAreas ( int numberOfRows,
				      int /*numberOfColumns*/,
			 CoinBigIndex ,
			 CoinBigIndex  )
{

  numberRows_ = numberOfRows;
  numberDense_=numberRows_;
  if ((numberRows_&(BLOCKING8-1))!=0)
    numberDense_ += (BLOCKING8-(numberRows_&(BLOCKING8-1)));
  CoinBigIndex size = numberDense_*(2*numberDense_+CoinMax(maximumPivots_,(numberDense_+1)>>1));
  if (size>maximumSpace_) {
    delete [] elements_;
    elements_ = new CoinFactorizationDouble [size];
    maximumSpace_ = size;
  }
  if (numberRows_>maximumRows_) {
    maximumRows_ = numberRows_;
    maximumRowsAdjusted_ = maximumRows_;
    if ((maximumRows_&(BLOCKING8-1))!=0)
      maximumRowsAdjusted_ += (BLOCKING8-(maximumRows_&(BLOCKING8-1)));
    delete [] pivotRow_;
    delete [] workArea_;
    pivotRow_ = new int [2*maximumRowsAdjusted_+maximumPivots_];
    workArea_ = new CoinFactorizationDouble [maximumRowsAdjusted_*WORK_MULT];
  }
}

//  preProcess.  
void
CoinAbcDenseFactorization::preProcess ()
{
  CoinBigIndex put = numberDense_*numberDense_;
  CoinFactorizationDouble * COIN_RESTRICT area = elements_+maximumSpace_-put;
  CoinAbcMemset0(area,put);
  int *indexRow = reinterpret_cast<int *> (elements_+numberRows_*numberRows_);
  CoinBigIndex * starts = reinterpret_cast<CoinBigIndex *> (pivotRow_); 
  CoinFactorizationDouble * COIN_RESTRICT column = area;
  //if (solveMode_==10)
  //solveMode_=1;
  if ((solveMode_%10)!=0) {
    for (int i=0;i<numberRows_;i++) {
      for (CoinBigIndex j=starts[i];j<starts[i+1];j++) {
	int iRow = indexRow[j];
	int iBlock=iRow>>3;
	iRow=iRow&(BLOCKING8-1);
	column[iRow+BLOCKING8X8*iBlock]=elements_[j];
      }
      if ((i&(BLOCKING8-1))!=(BLOCKING8-1))
	column += BLOCKING8;
      else
	column += numberDense_*BLOCKING8-(BLOCKING8-1)*BLOCKING8;
    }
    for (int i=numberRows_;i<numberDense_;i++) {
      int iRow = i;
      int iBlock=iRow>>3;
      iRow=iRow&(BLOCKING8-1);
      column[iRow+BLOCKING8X8*iBlock]=1.0;
      if ((i&(BLOCKING8-1))!=(BLOCKING8-1))
	column += BLOCKING8;
      //else
      //column += numberDense_*BLOCKING8-(BLOCKING8-1)*BLOCKING8;
    }
  } else {
    // first or bad
    for (int i=0;i<numberRows_;i++) {
      for (CoinBigIndex j=starts[i];j<starts[i+1];j++) {
	int iRow = indexRow[j];
	column[iRow]=elements_[j];
      }
      column += numberDense_;
    }
    for (int i=numberRows_;i<numberDense_;i++) {
      column[i]=1.0;
      column += numberDense_;
    }
  }
}

//Does factorization
int
CoinAbcDenseFactorization::factor (AbcSimplex * /*model*/)
{
  numberPivots_=0;
  // ? take out 
  //printf("Debug non lapack dense factor\n");
  //solveMode_&=~1;
  CoinBigIndex put = numberDense_*numberDense_;
  CoinFactorizationDouble * COIN_RESTRICT area = elements_+maximumSpace_-put;
  if ((solveMode_%10)!=0) {
    // save last start
    CoinBigIndex lastStart=pivotRow_[numberRows_];
    status_=CoinAbcDgetrf(numberDense_,numberDense_,area,numberDense_,pivotRow_+numberDense_
#if ABC_PARALLEL==2
			  ,parallelMode_
#endif
			  );
    // need to check size of pivots
    if(!status_) {
      // OK
      solveMode_=1+10*(solveMode_/10);
      numberGoodU_=numberRows_;
      CoinZeroN(workArea_,2*numberRows_);
#if 0 //ndef NDEBUG
      const CoinFactorizationDouble * column = area;
      double smallest=COIN_DBL_MAX;
      for (int i=0;i<numberRows_;i++) {
	if (fabs(column[i])<smallest)
	  smallest = fabs(column[i]);
	column += numberRows_;
      }
      if (smallest<1.0e-8)
	printf("small el %g\n",smallest);
#endif
      return 0;
    } else {
      solveMode_=10*(solveMode_/10);
      // redo matrix
      // last start
      pivotRow_[numberRows_]=lastStart;
      preProcess();
    }
  }
  status_=0;
  for (int j=0;j<numberRows_;j++) {
    pivotRow_[j+numberDense_]=j;
  }
  CoinFactorizationDouble * elements = area;
  numberGoodU_=0;
  for (int i=0;i<numberRows_;i++) {
    int iRow = -1;
    // Find largest
    double largest=zeroTolerance_;
    for (int j=i;j<numberRows_;j++) {
      double value = fabs(elements[j]);
      if (value>largest) {
	largest=value;
	iRow=j;
      }
    }
    if (iRow>=0) {
      if (iRow!=i) {
	// swap
	assert (iRow>i);
	CoinFactorizationDouble * elementsA = area;
	for (int k=0;k<=i;k++) {
	  // swap
	  CoinFactorizationDouble value = elementsA[i];
	  elementsA[i]=elementsA[iRow];
	  elementsA[iRow]=value;
	  elementsA += numberDense_;
	}
	int iPivot = pivotRow_[i+numberDense_];
	pivotRow_[i+numberDense_]=pivotRow_[iRow+numberDense_];
	pivotRow_[iRow+numberDense_]=iPivot;
      }
      CoinFactorizationDouble pivotValue = 1.0/elements[i];
      elements[i]=pivotValue;
      for (int j=i+1;j<numberRows_;j++) {
	elements[j] *= pivotValue;
      }
      // Update rest
      CoinFactorizationDouble * elementsA = elements;
      for (int k=i+1;k<numberRows_;k++) {
	elementsA += numberDense_;
	// swap
	if (iRow!=i) {
	  CoinFactorizationDouble value = elementsA[i];
	  elementsA[i]=elementsA[iRow];
	  elementsA[iRow]=value;
	}
	CoinFactorizationDouble value = elementsA[i];
	for (int j=i+1;j<numberRows_;j++) {
	  elementsA[j] -= value * elements[j];
	}
      }
    } else {
      status_=-1;
      break;
    }
    numberGoodU_++;
    elements += numberDense_;
  }
  for (int j=0;j<numberRows_;j++) {
    int k = pivotRow_[j+numberDense_];
    pivotRow_[k]=j;
  }
  //assert (status_);
  //if (!status_)
  //solveMode_=10; // for next time
  //for (int j=0;j<numberRows_;j++) {
  //int k = pivotRow_[j+numberDense_];
  //pivotRow_[k]=j;
  //}
  return status_;
}
// Makes a non-singular basis by replacing variables
void 
CoinAbcDenseFactorization::makeNonSingular(int * sequence)
{
  // Replace bad ones by correct slack
  int * workArea = reinterpret_cast<int *> (workArea_);
  int i;
  for ( i=0;i<numberRows_;i++) 
    workArea[i]=-1;
  for ( i=0;i<numberGoodU_;i++) {
    int iOriginal = pivotRow_[i+numberDense_];
    workArea[iOriginal]=i;
    //workArea[i]=iOriginal;
  }
  int lastRow=-1;
  for ( i=0;i<numberRows_;i++) {
    if (workArea[i]==-1) {
      lastRow=i;
      break;
    }
  }
  assert (lastRow>=0);
  for ( i=numberGoodU_;i<numberRows_;i++) {
    assert (lastRow<numberRows_);
    // Put slack in basis
    sequence[i]=lastRow;
    lastRow++;
    for (;lastRow<numberRows_;lastRow++) {
      if (workArea[lastRow]==-1)
	break;
    }
  }
}
//#define DENSE_PERMUTE
// Does post processing on valid factorization - putting variables on correct rows
void 
CoinAbcDenseFactorization::postProcess(const int * sequence, int * pivotVariable)
{
  if ((solveMode_%10)==0) {
    for (int i=0;i<numberRows_;i++) {
      int k = sequence[i];
#ifdef DENSE_PERMUTE
      pivotVariable[pivotRow_[i+numberDense_]]=k;
#else
      //pivotVariable[pivotRow_[i]]=k;
      //pivotVariable[pivotRow_[i]]=k;
      pivotVariable[i]=k;
      k=pivotRow_[i];
      pivotRow_[i] = pivotRow_[i+numberDense_];
      pivotRow_[i+numberDense_]=k;
#endif
    }
  } else {
    // lapack
    for (int i=0;i<numberRows_;i++) {
      int k = sequence[i];
      pivotVariable[i]=k;
    }
  }
}
/* Replaces one Column to basis,
   returns 0=OK, 1=Probably OK, 2=singular, 3=no room
   partial update already in U */
int 
CoinAbcDenseFactorization::replaceColumn ( CoinIndexedVector * regionSparse,
					int pivotRow,
					double pivotCheck ,
					bool /*skipBtranU*/,
				       double /*acceptablePivot*/)
{
  if (numberPivots_==maximumPivots_)
    return 3;
  CoinFactorizationDouble * elements = elements_ + numberDense_*numberPivots_;
  double *region = regionSparse->denseVector (  );
  int *regionIndex = regionSparse->getIndices (  );
  int numberNonZero = regionSparse->getNumElements (  );
  int i;
  memset(elements,0,numberRows_*sizeof(CoinFactorizationDouble));
  CoinFactorizationDouble pivotValue = pivotCheck;
  if (fabs(pivotValue)<zeroTolerance_)
    return 2;
  pivotValue = 1.0/pivotValue;
  if ((solveMode_%10)==0) {
    if (regionSparse->packedMode()) {
      for (i=0;i<numberNonZero;i++) {
	int iRow = regionIndex[i];
	double value = region[i];
#ifdef DENSE_PERMUTE
	iRow = pivotRow_[iRow]; // permute
#endif
	elements[iRow] = value;;
      }
    } else {
      // not packed! - from user pivot?
      for (i=0;i<numberNonZero;i++) {
	int iRow = regionIndex[i];
	double value = region[iRow];
#ifdef DENSE_PERMUTE
	iRow = pivotRow_[iRow]; // permute
#endif
	elements[iRow] = value;;
      }
    }
    int realPivotRow = pivotRow_[pivotRow];
    elements[realPivotRow]=pivotValue;
    pivotRow_[2*numberDense_+numberPivots_]=realPivotRow;
  } else {
    // lapack
    if (regionSparse->packedMode()) {
      for (i=0;i<numberNonZero;i++) {
	int iRow = regionIndex[i];
	double value = region[i];
	elements[iRow] = value;;
      }
    } else {
      // not packed! - from user pivot?
      for (i=0;i<numberNonZero;i++) {
	int iRow = regionIndex[i];
	double value = region[iRow];
	elements[iRow] = value;;
      }
    }
    elements[pivotRow]=pivotValue;
    pivotRow_[2*numberDense_+numberPivots_]=pivotRow;
  }
  numberPivots_++;
  return 0;
}
/* Checks if can replace one Column to basis,
   returns 0=OK, 1=Probably OK, 2=singular, 3=no room, 5 max pivots */
int 
CoinAbcDenseFactorization::checkReplacePart2 ( int pivotRow,
					       double btranAlpha, 
					       double ftranAlpha, 
#ifdef ABC_LONG_FACTORIZATION 
					       long
#endif
					       double ftAlpha,
					       double acceptablePivot)
{
  if (numberPivots_==maximumPivots_)
    return 3;
  if (fabs(ftranAlpha)<zeroTolerance_)
    return 2;
  return 0;
}
/* Replaces one Column to basis,
   partial update already in U */
void 
CoinAbcDenseFactorization::replaceColumnPart3 ( const AbcSimplex * model,
					   CoinIndexedVector * regionSparse,
					   CoinIndexedVector * tableauColumn,
					   int pivotRow,
#ifdef ABC_LONG_FACTORIZATION 
						long
#endif
					   double alpha )
{
  CoinFactorizationDouble * elements = elements_ + numberDense_*numberPivots_;
  double *region = tableauColumn->denseVector (  );
  int *regionIndex = tableauColumn->getIndices (  );
  int numberNonZero = tableauColumn->getNumElements (  );
  int i;
  memset(elements,0,numberRows_*sizeof(CoinFactorizationDouble));
  double pivotValue = 1.0/alpha;
  if ((solveMode_%10)==0) {
    if (tableauColumn->packedMode()) {
      for (i=0;i<numberNonZero;i++) {
	int iRow = regionIndex[i];
	double value = region[i];
#ifdef DENSE_PERMUTE
	iRow = pivotRow_[iRow]; // permute
#endif
	elements[iRow] = value;;
      }
    } else {
      // not packed! - from user pivot?
      for (i=0;i<numberNonZero;i++) {
	int iRow = regionIndex[i];
	double value = region[iRow];
#ifdef DENSE_PERMUTE
	iRow = pivotRow_[iRow]; // permute
#endif
	elements[iRow] = value;;
      }
    }
    int realPivotRow = pivotRow_[pivotRow];
    elements[realPivotRow]=pivotValue;
    pivotRow_[2*numberDense_+numberPivots_]=realPivotRow;
  } else {
    // lapack
    if (tableauColumn->packedMode()) {
      for (i=0;i<numberNonZero;i++) {
	int iRow = regionIndex[i];
	double value = region[i];
	elements[iRow] = value;;
      }
    } else {
      // not packed! - from user pivot?
      for (i=0;i<numberNonZero;i++) {
	int iRow = regionIndex[i];
	double value = region[iRow];
	elements[iRow] = value;;
      }
    }
    elements[pivotRow]=pivotValue;
    pivotRow_[2*numberDense_+numberPivots_]=pivotRow;
  }
  numberPivots_++;
}
static void 
CoinAbcDlaswp1(double * COIN_RESTRICT a,int m, int * ipiv)
{
  for (int i=0;i<m;i++) {
    int ip=ipiv[i];
    if (ip!=i) {
      double temp=a[i];
      a[i]=a[ip];
      a[ip]=temp;
    }
  }
}
static void 
CoinAbcDlaswp1Backwards(double * COIN_RESTRICT a,int m, int * ipiv)
{
  for (int i=m-1;i>=0;i--) {
    int ip=ipiv[i];
    if (ip!=i) {
      double temp=a[i];
      a[i]=a[ip];
      a[ip]=temp;
    }
  }
}
/* This version has same effect as above with FTUpdate==false
   so number returned is always >=0 */
int 
CoinAbcDenseFactorization::updateColumn (CoinIndexedVector & regionSparse) const
{
  double *region = regionSparse.denseVector (  );
  CoinBigIndex put = numberDense_*numberDense_;
  CoinFactorizationDouble * COIN_RESTRICT area = elements_+maximumSpace_-put;
  CoinFactorizationDouble * COIN_RESTRICT elements = area;
  if ((solveMode_%10)==0) {
    // base factorization L
    for (int i=0;i<numberRows_;i++) {
      double value = region[i];
      for (int j=i+1;j<numberRows_;j++) {
	region[j] -= value*elements[j];
      }
      elements += numberDense_;
    }
    elements = area+numberRows_*numberDense_;
    // base factorization U
    for (int i=numberRows_-1;i>=0;i--) {
      elements -= numberDense_;
      CoinFactorizationDouble value = region[i]*elements[i];
      region[i] = value;
      for (int j=0;j<i;j++) {
	region[j] -= value*elements[j];
      }
    }
  } else {
    CoinAbcDlaswp1(region,numberRows_,pivotRow_+numberDense_);
    CoinAbcDgetrs('N',numberDense_,area,region);
  }
  // now updates
  elements = elements_;
  for (int i=0;i<numberPivots_;i++) {
    int iPivot = pivotRow_[i+2*numberDense_];
    CoinFactorizationDouble value = region[iPivot]*elements[iPivot];
    for (int j=0;j<numberRows_;j++) {
      region[j] -= value*elements[j];
    }
    region[iPivot] = value;
    elements += numberDense_;
  }
  regionSparse.setNumElements(0);
  regionSparse.scan(0,numberRows_);
  return 0;
}

/* Updates one column for dual steepest edge weights (FTRAN) */
void 
CoinAbcDenseFactorization::updateWeights ( CoinIndexedVector & regionSparse) const
{
  updateColumn(regionSparse);
}

int 
CoinAbcDenseFactorization::updateTwoColumnsFT(CoinIndexedVector & regionSparse2,
					      CoinIndexedVector & regionSparse3)
{
  updateColumn(regionSparse2);
  updateColumn(regionSparse3);
  return 0;
}

/* Updates one column (BTRAN) from regionSparse2
   regionSparse starts as zero and is zero at end 
   Note - if regionSparse2 packed on input - will be packed on output
*/
int 
CoinAbcDenseFactorization::updateColumnTranspose ( CoinIndexedVector & regionSparse2) const
{
  double *region = regionSparse2.denseVector (  );
  CoinFactorizationDouble * elements = elements_+numberDense_*numberPivots_;
  // updates
  for (int i=numberPivots_-1;i>=0;i--) {
    elements -= numberDense_;
    int iPivot = pivotRow_[i+2*numberDense_];
    CoinFactorizationDouble value = region[iPivot]; //*elements[iPivot];
    for (int j=0;j<iPivot;j++) {
      value -= region[j]*elements[j];
    }
    for (int j=iPivot+1;j<numberRows_;j++) {
      value -= region[j]*elements[j];
    }
    region[iPivot] = value*elements[iPivot];
  }
  CoinBigIndex put = numberDense_*numberDense_;
  CoinFactorizationDouble * COIN_RESTRICT area = elements_+maximumSpace_-put;
  if ((solveMode_%10)==0) {
    // base factorization U
    elements = area;
    for (int i=0;i<numberRows_;i++) {
      //CoinFactorizationDouble value = region[i]*elements[i];
      CoinFactorizationDouble value = region[i];
      for (int j=0;j<i;j++) {
	value -= region[j]*elements[j];
      }
      //region[i] = value;
      region[i] = value*elements[i];
      elements += numberDense_;
    }
    // base factorization L
    elements = area+numberDense_*numberRows_;
    for (int i=numberRows_-1;i>=0;i--) {
      elements -= numberDense_;
      CoinFactorizationDouble value = region[i];
      for (int j=i+1;j<numberRows_;j++) {
	value -= region[j]*elements[j];
      }
      region[i] = value;
    }
  } else {
    CoinAbcDgetrs('T',numberDense_,area,region);
    CoinAbcDlaswp1Backwards(region,numberRows_,pivotRow_+numberDense_);
  }
  regionSparse2.setNumElements(0);
  regionSparse2.scan(0,numberRows_);
  return 0;
}
/* Updates one column (FTRAN) */
void 
CoinAbcAnyFactorization::updateColumnCpu ( CoinIndexedVector & regionSparse,int whichCpu) const
{ updateColumn(regionSparse);}
/* Updates one column (BTRAN) */
void 
CoinAbcAnyFactorization::updateColumnTransposeCpu ( CoinIndexedVector & regionSparse,int whichCpu) const
{ updateColumnTranspose(regionSparse);}
// Default constructor
CoinAbcAnyFactorization::CoinAbcAnyFactorization (  )
   :  pivotTolerance_(1.0e-1),
      minimumPivotTolerance_(1.0e-1),
      zeroTolerance_(1.0e-13),
      relaxCheck_(1.0),
      factorElements_(0),
      numberRows_(0),
      numberGoodU_(0),
      maximumPivots_(200),
      numberPivots_(0),
      status_(-1),
      solveMode_(0)
{
}
// Copy constructor 
CoinAbcAnyFactorization::CoinAbcAnyFactorization ( const CoinAbcAnyFactorization &other)
   :  pivotTolerance_(other.pivotTolerance_),
      minimumPivotTolerance_(other.minimumPivotTolerance_),
  zeroTolerance_(other.zeroTolerance_),
  relaxCheck_(other.relaxCheck_),
  factorElements_(other.factorElements_),
  numberRows_(other.numberRows_),
  numberGoodU_(other.numberGoodU_),
  maximumPivots_(other.maximumPivots_),
  numberPivots_(other.numberPivots_),
      status_(other.status_),
      solveMode_(other.solveMode_)
{
}
// Destructor
CoinAbcAnyFactorization::~CoinAbcAnyFactorization (  )
{
}
// = copy
CoinAbcAnyFactorization & CoinAbcAnyFactorization::operator = ( const CoinAbcAnyFactorization & other )
{
  if (this != &other) {    
    pivotTolerance_ = other.pivotTolerance_;
    minimumPivotTolerance_ = other.minimumPivotTolerance_;
    zeroTolerance_ = other.zeroTolerance_;
    relaxCheck_ = other.relaxCheck_;
    factorElements_ = other.factorElements_;
    numberRows_ = other.numberRows_;
    numberGoodU_ = other.numberGoodU_;
    maximumPivots_ = other.maximumPivots_;
    numberPivots_ = other.numberPivots_;
    status_ = other.status_;
    solveMode_ = other.solveMode_;
  }
  return *this;
}
void CoinAbcAnyFactorization::pivotTolerance (  double value )
{
  if (value>0.0&&value<=1.0) {
    pivotTolerance_=CoinMax(value,minimumPivotTolerance_);
  }
}
void CoinAbcAnyFactorization::minimumPivotTolerance (  double value )
{
  if (value>0.0&&value<=1.0) {
    minimumPivotTolerance_=value;
  }
}
void CoinAbcAnyFactorization::zeroTolerance (  double value )
{
  if (value>0.0&&value<1.0) {
    zeroTolerance_=value;
  }
}
void 
CoinAbcAnyFactorization::maximumPivots (  int value )
{
  if (value>maximumPivots_) {
    delete [] pivotRow_;
    int n=maximumRows_;
    if ((n&(BLOCKING8-1))!=0)
    n += (BLOCKING8-(n&(BLOCKING8-1)));
    pivotRow_ = new int[2*n+value];
  }
  maximumPivots_ = value;
}
// Number of entries in each row
int * 
CoinAbcAnyFactorization::numberInRow() const
{ return reinterpret_cast<int *> (workArea_);}
// Number of entries in each column
int * 
CoinAbcAnyFactorization::numberInColumn() const
{ return (reinterpret_cast<int *> (workArea_))+numberRows_;}
// Returns array to put basis starts in
CoinBigIndex * 
CoinAbcAnyFactorization::starts() const
{ return reinterpret_cast<CoinBigIndex *> (pivotRow_);}
// Returns array to put basis elements in
CoinFactorizationDouble * 
CoinAbcAnyFactorization::elements() const
{ return elements_;}
// Returns pivot row 
int * 
CoinAbcAnyFactorization::pivotRow() const
{ return pivotRow_;}
// Returns work area
CoinFactorizationDouble * 
CoinAbcAnyFactorization::workArea() const
{ return workArea_;}
// Returns int work area
int * 
CoinAbcAnyFactorization::intWorkArea() const
{ return reinterpret_cast<int *> (workArea_);}
// Returns permute back
int * 
CoinAbcAnyFactorization::permuteBack() const
{ return pivotRow_+numberRows_;}
// Returns pivot column
int * 
CoinAbcAnyFactorization::pivotColumn() const
{ return permute();}
// Returns true if wants tableauColumn in replaceColumn
bool
CoinAbcAnyFactorization::wantsTableauColumn() const
{ return true;}
/* Useful information for factorization
   0 - iteration number
   whereFrom is 0 for factorize and 1 for replaceColumn
*/
void 
CoinAbcAnyFactorization::setUsefulInformation(const int * ,int )
{ }
