/* $Id: CoinDenseFactorization.cpp 1417 2011-04-17 15:05:57Z forrest $ */
// Copyright (C) 2008, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include "CoinUtilsConfig.h"
#include "CoinPragma.hpp"

#include <cassert>
#include <cstdio>

#include "CoinDenseFactorization.hpp"
#include "CoinIndexedVector.hpp"
#include "CoinHelperFunctions.hpp"
#include "CoinPackedMatrix.hpp"
#include "CoinFinite.hpp"
#if COIN_BIG_DOUBLE==1
#undef DENSE_CODE
#endif
#ifdef DENSE_CODE
// using simple lapack interface
extern "C" 
{
  /** LAPACK Fortran subroutine DGETRF. */
  void F77_FUNC(dgetrf,DGETRF)(ipfint * m, ipfint *n,
                               double *A, ipfint *ldA,
                               ipfint * ipiv, ipfint *info);
  /** LAPACK Fortran subroutine DGETRS. */
  void F77_FUNC(dgetrs,DGETRS)(char *trans, cipfint *n,
                               cipfint *nrhs, const double *A, cipfint *ldA,
                               cipfint * ipiv, double *B, cipfint *ldB, ipfint *info,
			       int trans_len);
}
#endif
//:class CoinDenseFactorization.  Deals with Factorization and Updates
//  CoinDenseFactorization.  Constructor
CoinDenseFactorization::CoinDenseFactorization (  )
  : CoinOtherFactorization()
{
  gutsOfInitialize();
}

/// Copy constructor 
CoinDenseFactorization::CoinDenseFactorization ( const CoinDenseFactorization &other)
  : CoinOtherFactorization(other)
{
  gutsOfInitialize();
  gutsOfCopy(other);
}
// Clone
CoinOtherFactorization * 
CoinDenseFactorization::clone() const 
{
  return new CoinDenseFactorization(*this);
}
/// The real work of constructors etc
void CoinDenseFactorization::gutsOfDestructor()
{
  delete [] elements_;
  delete [] pivotRow_;
  delete [] workArea_;
  elements_ = NULL;
  pivotRow_ = NULL;
  workArea_ = NULL;
  numberRows_ = 0;
  numberColumns_ = 0;
  numberGoodU_ = 0;
  status_ = -1;
  maximumRows_=0;
  maximumSpace_=0;
  solveMode_=0;
}
void CoinDenseFactorization::gutsOfInitialize()
{
  pivotTolerance_ = 1.0e-1;
  zeroTolerance_ = 1.0e-13;
#ifndef COIN_FAST_CODE
  slackValue_ = -1.0;
#endif
  maximumPivots_=200;
  relaxCheck_=1.0;
  numberRows_ = 0;
  numberColumns_ = 0;
  numberGoodU_ = 0;
  status_ = -1;
  numberPivots_ = 0;
  maximumRows_=0;
  maximumSpace_=0;
  elements_ = NULL;
  pivotRow_ = NULL;
  workArea_ = NULL;
  solveMode_=0;
}
//  ~CoinDenseFactorization.  Destructor
CoinDenseFactorization::~CoinDenseFactorization (  )
{
  gutsOfDestructor();
}
//  =
CoinDenseFactorization & CoinDenseFactorization::operator = ( const CoinDenseFactorization & other ) {
  if (this != &other) {    
    gutsOfDestructor();
    gutsOfInitialize();
    gutsOfCopy(other);
  }
  return *this;
}
#ifdef DENSE_CODE
#define WORK_MULT 2
#else
#define WORK_MULT 2
#endif
void CoinDenseFactorization::gutsOfCopy(const CoinDenseFactorization &other)
{
  pivotTolerance_ = other.pivotTolerance_;
  zeroTolerance_ = other.zeroTolerance_;
#ifndef COIN_FAST_CODE
  slackValue_ = other.slackValue_;
#endif
  relaxCheck_ = other.relaxCheck_;
  numberRows_ = other.numberRows_;
  numberColumns_ = other.numberColumns_;
  maximumRows_ = other.maximumRows_;
  maximumSpace_ = other.maximumSpace_;
  solveMode_ = other.solveMode_;
  numberGoodU_ = other.numberGoodU_;
  maximumPivots_ = other.maximumPivots_;
  numberPivots_ = other.numberPivots_;
  factorElements_ = other.factorElements_;
  status_ = other.status_;
  if (other.pivotRow_) {
    pivotRow_ = new int [2*maximumRows_+maximumPivots_];
    CoinMemcpyN(other.pivotRow_,(2*maximumRows_+numberPivots_),pivotRow_);
    elements_ = new CoinFactorizationDouble [maximumSpace_];
    CoinMemcpyN(other.elements_,(maximumRows_+numberPivots_)*maximumRows_,elements_);
    workArea_ = new CoinFactorizationDouble [maximumRows_*WORK_MULT];
    CoinZeroN(workArea_,maximumRows_*WORK_MULT);
  } else {
    elements_ = NULL;
    pivotRow_ = NULL;
    workArea_ = NULL;
  }
}

//  getAreas.  Gets space for a factorization
//called by constructors
void
CoinDenseFactorization::getAreas ( int numberOfRows,
			 int numberOfColumns,
			 CoinBigIndex ,
			 CoinBigIndex  )
{

  numberRows_ = numberOfRows;
  numberColumns_ = numberOfColumns;
  CoinBigIndex size = numberRows_*(numberRows_+CoinMax(maximumPivots_,(numberRows_+1)>>1));
  if (size>maximumSpace_) {
    delete [] elements_;
    elements_ = new CoinFactorizationDouble [size];
    maximumSpace_ = size;
  }
  if (numberRows_>maximumRows_) {
    maximumRows_ = numberRows_;
    delete [] pivotRow_;
    delete [] workArea_;
    pivotRow_ = new int [2*maximumRows_+maximumPivots_];
    workArea_ = new CoinFactorizationDouble [maximumRows_*WORK_MULT];
  }
}

//  preProcess.  
void
CoinDenseFactorization::preProcess ()
{
  // could do better than this but this only a demo
  CoinBigIndex put = numberRows_*numberRows_;
  int *indexRow = reinterpret_cast<int *> (elements_+put);
  CoinBigIndex * starts = reinterpret_cast<CoinBigIndex *> (pivotRow_); 
  put = numberRows_*numberColumns_;
  for (int i=numberColumns_-1;i>=0;i--) {
    put -= numberRows_;
    memset(workArea_,0,numberRows_*sizeof(CoinFactorizationDouble));
    assert (starts[i]<=put);
    for (CoinBigIndex j=starts[i];j<starts[i+1];j++) {
      int iRow = indexRow[j];
      workArea_[iRow] = elements_[j];
    }
    // move to correct position
    CoinMemcpyN(workArea_,numberRows_,elements_+put);
  }
}

//Does factorization
int
CoinDenseFactorization::factor ( )
{
  numberPivots_=0;
  status_= 0;
#ifdef DENSE_CODE
  if (numberRows_==numberColumns_&&(solveMode_%10)!=0) {
    int info;
    F77_FUNC(dgetrf,DGETRF)(&numberRows_,&numberRows_,
			    elements_,&numberRows_,pivotRow_,
			    &info);
    // need to check size of pivots
    if(!info) {
      // OK
      solveMode_=1+10*(solveMode_/10);
      numberGoodU_=numberRows_;
      CoinZeroN(workArea_,2*numberRows_);
#if 0 //ndef NDEBUG
      const CoinFactorizationDouble * column = elements_;
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
    }
  }
#endif
  for (int j=0;j<numberRows_;j++) {
    pivotRow_[j+numberRows_]=j;
  }
  CoinFactorizationDouble * elements = elements_;
  numberGoodU_=0;
  for (int i=0;i<numberColumns_;i++) {
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
	CoinFactorizationDouble * elementsA = elements_;
	for (int k=0;k<=i;k++) {
	  // swap
	  CoinFactorizationDouble value = elementsA[i];
	  elementsA[i]=elementsA[iRow];
	  elementsA[iRow]=value;
	  elementsA += numberRows_;
	}
	int iPivot = pivotRow_[i+numberRows_];
	pivotRow_[i+numberRows_]=pivotRow_[iRow+numberRows_];
	pivotRow_[iRow+numberRows_]=iPivot;
      }
      CoinFactorizationDouble pivotValue = 1.0/elements[i];
      elements[i]=pivotValue;
      for (int j=i+1;j<numberRows_;j++) {
	elements[j] *= pivotValue;
      }
      // Update rest
      CoinFactorizationDouble * elementsA = elements;
      for (int k=i+1;k<numberColumns_;k++) {
	elementsA += numberRows_;
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
    elements += numberRows_;
  }
  for (int j=0;j<numberRows_;j++) {
    int k = pivotRow_[j+numberRows_];
    pivotRow_[k]=j;
  }
  return status_;
}
// Makes a non-singular basis by replacing variables
void 
CoinDenseFactorization::makeNonSingular(int * sequence, int numberColumns)
{
  // Replace bad ones by correct slack
  int * workArea = reinterpret_cast<int *> (workArea_);
  int i;
  for ( i=0;i<numberRows_;i++) 
    workArea[i]=-1;
  for ( i=0;i<numberGoodU_;i++) {
    int iOriginal = pivotRow_[i+numberRows_];
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
    sequence[i]=lastRow+numberColumns;
    lastRow++;
    for (;lastRow<numberRows_;lastRow++) {
      if (workArea[lastRow]==-1)
	break;
    }
  }
}
#define DENSE_PERMUTE
// Does post processing on valid factorization - putting variables on correct rows
void 
CoinDenseFactorization::postProcess(const int * sequence, int * pivotVariable)
{
#ifdef DENSE_CODE
  if ((solveMode_%10)==0) {
#endif
    for (int i=0;i<numberRows_;i++) {
      int k = sequence[i];
#ifdef DENSE_PERMUTE
      pivotVariable[pivotRow_[i+numberRows_]]=k;
#else
      //pivotVariable[pivotRow_[i]]=k;
      //pivotVariable[pivotRow_[i]]=k;
      pivotVariable[i]=k;
      k=pivotRow_[i];
      pivotRow_[i] = pivotRow_[i+numberRows_];
      pivotRow_[i+numberRows_]=k;
#endif
    }
#ifdef DENSE_CODE
  } else {
    // lapack
    for (int i=0;i<numberRows_;i++) {
      int k = sequence[i];
      pivotVariable[i]=k;
    }
  }
#endif
}
/* Replaces one Column to basis,
   returns 0=OK, 1=Probably OK, 2=singular, 3=no room
   If checkBeforeModifying is true will do all accuracy checks
   before modifying factorization.  Whether to set this depends on
   speed considerations.  You could just do this on first iteration
   after factorization and thereafter re-factorize
   partial update already in U */
int 
CoinDenseFactorization::replaceColumn ( CoinIndexedVector * regionSparse,
					int pivotRow,
					double pivotCheck ,
					bool /*checkBeforeModifying*/,
				       double /*acceptablePivot*/)
{
  if (numberPivots_==maximumPivots_)
    return 3;
  CoinFactorizationDouble * elements = elements_ + numberRows_*(numberColumns_+numberPivots_);
  double *region = regionSparse->denseVector (  );
  int *regionIndex = regionSparse->getIndices (  );
  int numberNonZero = regionSparse->getNumElements (  );
  int i;
  memset(elements,0,numberRows_*sizeof(CoinFactorizationDouble));
  CoinFactorizationDouble pivotValue = pivotCheck;
  if (fabs(pivotValue)<zeroTolerance_)
    return 2;
  pivotValue = 1.0/pivotValue;
#ifdef DENSE_CODE
  if ((solveMode_%10)==0) {
#endif
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
    pivotRow_[2*numberRows_+numberPivots_]=realPivotRow;
#ifdef DENSE_CODE
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
    pivotRow_[2*numberRows_+numberPivots_]=pivotRow;
  }
#endif
  numberPivots_++;
  return 0;
}
/* This version has same effect as above with FTUpdate==false
   so number returned is always >=0 */
int 
CoinDenseFactorization::updateColumn ( CoinIndexedVector * regionSparse,
				       CoinIndexedVector * regionSparse2,
				       bool noPermute) const
{
  assert (numberRows_==numberColumns_);
  double *region2 = regionSparse2->denseVector (  );
  int *regionIndex = regionSparse2->getIndices (  );
  int numberNonZero = regionSparse2->getNumElements (  );
  double *region = regionSparse->denseVector (  );
#ifdef DENSE_CODE
  if ((solveMode_%10)==0) {
#endif
    if (!regionSparse2->packedMode()) {
      if (!noPermute) {
	for (int j=0;j<numberRows_;j++) {
	  int iRow = pivotRow_[j+numberRows_];
	  region[j]=region2[iRow];
	  region2[iRow]=0.0;
	}
      } else {
	// can't due to check mode assert (regionSparse==regionSparse2);
	region = regionSparse2->denseVector (  );
      }
    } else {
      // packed mode
      assert (!noPermute);
      for (int j=0;j<numberNonZero;j++) {
	int jRow = regionIndex[j];
	int iRow = pivotRow_[jRow];
	region[iRow]=region2[j];
	region2[j]=0.0;
      }
    }
#ifdef DENSE_CODE
  } else {
    // lapack
    if (!regionSparse2->packedMode()) {
      if (!noPermute) {
	for (int j=0;j<numberRows_;j++) {
	  region[j]=region2[j];
	  region2[j]=0.0;
	}
      } else {
	// can't due to check mode assert (regionSparse==regionSparse2);
	region = regionSparse2->denseVector (  );
      }
    } else {
      // packed mode
      assert (!noPermute);
      for (int j=0;j<numberNonZero;j++) {
	int jRow = regionIndex[j];
	region[jRow]=region2[j];
	region2[j]=0.0;
      }
    }
  }
#endif
  int i;
  CoinFactorizationDouble * elements = elements_;
#ifdef DENSE_CODE
  if ((solveMode_%10)==0) {
#endif
    // base factorization L
    for (i=0;i<numberColumns_;i++) {
      double value = region[i];
      for (int j=i+1;j<numberRows_;j++) {
	region[j] -= value*elements[j];
      }
      elements += numberRows_;
    }
    elements = elements_+numberRows_*numberRows_;
    // base factorization U
    for (i=numberColumns_-1;i>=0;i--) {
      elements -= numberRows_;
      CoinFactorizationDouble value = region[i]*elements[i];
      region[i] = value;
      for (int j=0;j<i;j++) {
	region[j] -= value*elements[j];
      }
    }
#ifdef DENSE_CODE
  } else {
    char trans = 'N';
    int ione=1;
    int info;
    F77_FUNC(dgetrs,DGETRS)(&trans,&numberRows_,&ione,elements_,&numberRows_,
			      pivotRow_,region,&numberRows_,&info,1);
  }
#endif
  // now updates
  elements = elements_+numberRows_*numberRows_;
  for (i=0;i<numberPivots_;i++) {
    int iPivot = pivotRow_[i+2*numberRows_];
    CoinFactorizationDouble value = region[iPivot]*elements[iPivot];
    for (int j=0;j<numberRows_;j++) {
      region[j] -= value*elements[j];
    }
    region[iPivot] = value;
    elements += numberRows_;
  }
  // permute back and get nonzeros
  numberNonZero=0;
#ifdef DENSE_CODE
  if ((solveMode_%10)==0) {
#endif
    if (!noPermute) {
      if (!regionSparse2->packedMode()) {
	for (int j=0;j<numberRows_;j++) {
#ifdef DENSE_PERMUTE
	  int iRow = pivotRow_[j];
#else
	  int iRow=j;
#endif
	  double value = region[iRow];
	  region[iRow]=0.0;
	  if (fabs(value)>zeroTolerance_) {
	    region2[j] = value;
	    regionIndex[numberNonZero++]=j;
	  }
	}
      } else {
	// packed mode
	for (int j=0;j<numberRows_;j++) {
#ifdef DENSE_PERMUTE
	  int iRow = pivotRow_[j];
#else
	  int iRow=j;
#endif
	  double value = region[iRow];
	  region[iRow]=0.0;
	  if (fabs(value)>zeroTolerance_) {
	    region2[numberNonZero] = value;
	    regionIndex[numberNonZero++]=j;
	  }
	}
      }
    } else {
      for (int j=0;j<numberRows_;j++) {
	double value = region[j];
	if (fabs(value)>zeroTolerance_) {
	  regionIndex[numberNonZero++]=j;
	} else {
	  region[j]=0.0;
	}
      }
    }
#ifdef DENSE_CODE
  } else {
    // lapack
    if (!noPermute) {
      if (!regionSparse2->packedMode()) {
	for (int j=0;j<numberRows_;j++) {
	  double value = region[j];
	  region[j]=0.0;
	  if (fabs(value)>zeroTolerance_) {
	    region2[j] = value;
	    regionIndex[numberNonZero++]=j;
	  }
	}
      } else {
	// packed mode
	for (int j=0;j<numberRows_;j++) {
	  double value = region[j];
	  region[j]=0.0;
	  if (fabs(value)>zeroTolerance_) {
	    region2[numberNonZero] = value;
	    regionIndex[numberNonZero++]=j;
	  }
	}
      }
    } else {
      for (int j=0;j<numberRows_;j++) {
	double value = region[j];
	if (fabs(value)>zeroTolerance_) {
	  regionIndex[numberNonZero++]=j;
	} else {
	  region[j]=0.0;
	}
      }
    }
  }
#endif
  regionSparse2->setNumElements(numberNonZero);
  return 0;
}


int 
CoinDenseFactorization::updateTwoColumnsFT(CoinIndexedVector * regionSparse1,
					  CoinIndexedVector * regionSparse2,
					  CoinIndexedVector * regionSparse3,
					   bool /*noPermute*/)
{
#ifdef DENSE_CODE
#if 0
  CoinIndexedVector s2(*regionSparse2);
  CoinIndexedVector s3(*regionSparse3);
  updateColumn(regionSparse1,&s2);
  updateColumn(regionSparse1,&s3);
#endif
  if ((solveMode_%10)==0) {
#endif
    updateColumn(regionSparse1,regionSparse2);
    updateColumn(regionSparse1,regionSparse3);
#ifdef DENSE_CODE
  } else {
    // lapack
    assert (numberRows_==numberColumns_);
    double *region2 = regionSparse2->denseVector (  );
    int *regionIndex2 = regionSparse2->getIndices (  );
    int numberNonZero2 = regionSparse2->getNumElements (  );
    CoinFactorizationDouble * regionW2 = workArea_;
    if (!regionSparse2->packedMode()) {
      for (int j=0;j<numberRows_;j++) {
	regionW2[j]=region2[j];
	region2[j]=0.0;
      }
    } else {
      // packed mode
      for (int j=0;j<numberNonZero2;j++) {
	int jRow = regionIndex2[j];
	regionW2[jRow]=region2[j];
	region2[j]=0.0;
      }
    }
    double *region3 = regionSparse3->denseVector (  );
    int *regionIndex3 = regionSparse3->getIndices (  );
    int numberNonZero3 = regionSparse3->getNumElements (  );
    CoinFactorizationDouble *regionW3 = workArea_+numberRows_;
    if (!regionSparse3->packedMode()) {
      for (int j=0;j<numberRows_;j++) {
	regionW3[j]=region3[j];
	region3[j]=0.0;
      }
    } else {
      // packed mode
      for (int j=0;j<numberNonZero3;j++) {
	int jRow = regionIndex3[j];
	regionW3[jRow]=region3[j];
	region3[j]=0.0;
      }
    }
    int i;
    CoinFactorizationDouble * elements = elements_;
    char trans = 'N';
    int itwo=2;
    int info;
    F77_FUNC(dgetrs,DGETRS)(&trans,&numberRows_,&itwo,elements_,&numberRows_,
			    pivotRow_,workArea_,&numberRows_,&info,1);
    // now updates
    elements = elements_+numberRows_*numberRows_;
    for (i=0;i<numberPivots_;i++) {
      int iPivot = pivotRow_[i+2*numberRows_];
      CoinFactorizationDouble value2 = regionW2[iPivot]*elements[iPivot];
      CoinFactorizationDouble value3 = regionW3[iPivot]*elements[iPivot];
      for (int j=0;j<numberRows_;j++) {
	regionW2[j] -= value2*elements[j];
	regionW3[j] -= value3*elements[j];
      }
      regionW2[iPivot] = value2;
      regionW3[iPivot] = value3;
      elements += numberRows_;
    }
    // permute back and get nonzeros
    numberNonZero2=0;
    if (!regionSparse2->packedMode()) {
      for (int j=0;j<numberRows_;j++) {
	double value = regionW2[j];
	regionW2[j]=0.0;
	if (fabs(value)>zeroTolerance_) {
	  region2[j] = value;
	  regionIndex2[numberNonZero2++]=j;
	}
      }
    } else {
      // packed mode
      for (int j=0;j<numberRows_;j++) {
	double value = regionW2[j];
	regionW2[j]=0.0;
	if (fabs(value)>zeroTolerance_) {
	  region2[numberNonZero2] = value;
	  regionIndex2[numberNonZero2++]=j;
	}
      }
    }
    regionSparse2->setNumElements(numberNonZero2);
    numberNonZero3=0;
    if (!regionSparse3->packedMode()) {
      for (int j=0;j<numberRows_;j++) {
	double value = regionW3[j];
	regionW3[j]=0.0;
	if (fabs(value)>zeroTolerance_) {
	  region3[j] = value;
	  regionIndex3[numberNonZero3++]=j;
	}
      }
    } else {
      // packed mode
      for (int j=0;j<numberRows_;j++) {
	double value = regionW3[j];
	regionW3[j]=0.0;
	if (fabs(value)>zeroTolerance_) {
	  region3[numberNonZero3] = value;
	  regionIndex3[numberNonZero3++]=j;
	}
      }
    }
    regionSparse3->setNumElements(numberNonZero3);
#if 0
    printf("Good2==\n");
    s2.print();
    printf("Bad2==\n");
    regionSparse2->print();
    printf("======\n");
    printf("Good3==\n");
    s3.print();
    printf("Bad3==\n");
    regionSparse3->print();
    printf("======\n");
#endif
  }
#endif
  return 0;
}

/* Updates one column (BTRAN) from regionSparse2
   regionSparse starts as zero and is zero at end 
   Note - if regionSparse2 packed on input - will be packed on output
*/
int 
CoinDenseFactorization::updateColumnTranspose ( CoinIndexedVector * regionSparse,
						CoinIndexedVector * regionSparse2) const
{
  assert (numberRows_==numberColumns_);
  double *region2 = regionSparse2->denseVector (  );
  int *regionIndex = regionSparse2->getIndices (  );
  int numberNonZero = regionSparse2->getNumElements (  );
  double *region = regionSparse->denseVector (  );
#ifdef DENSE_CODE
  if ((solveMode_%10)==0) {
#endif
    if (!regionSparse2->packedMode()) {
      for (int j=0;j<numberRows_;j++) {
#ifdef DENSE_PERMUTE
	int iRow = pivotRow_[j];
#else
	int iRow=j;
#endif
	region[iRow]=region2[j];
	region2[j]=0.0;
      }
    } else {
      for (int j=0;j<numberNonZero;j++) {
	int jRow = regionIndex[j];
#ifdef DENSE_PERMUTE
	int iRow = pivotRow_[jRow];
#else
	int iRow=jRow;
#endif
	region[iRow]=region2[j];
	region2[j]=0.0;
      }
    }
#ifdef DENSE_CODE
  } else {
    // lapack
    if (!regionSparse2->packedMode()) {
      for (int j=0;j<numberRows_;j++) {
	region[j]=region2[j];
	region2[j]=0.0;
      }
    } else {
      for (int j=0;j<numberNonZero;j++) {
	int jRow = regionIndex[j];
	region[jRow]=region2[j];
	region2[j]=0.0;
      }
    }
  }
#endif
  int i;
  CoinFactorizationDouble * elements = elements_+numberRows_*(numberRows_+numberPivots_);
  // updates
  for (i=numberPivots_-1;i>=0;i--) {
    elements -= numberRows_;
    int iPivot = pivotRow_[i+2*numberRows_];
    CoinFactorizationDouble value = region[iPivot]; //*elements[iPivot];
    for (int j=0;j<iPivot;j++) {
      value -= region[j]*elements[j];
    }
    for (int j=iPivot+1;j<numberRows_;j++) {
      value -= region[j]*elements[j];
    }
    region[iPivot] = value*elements[iPivot];
  }
#ifdef DENSE_CODE
  if ((solveMode_%10)==0) {
#endif
    // base factorization U
    elements = elements_;
    for (i=0;i<numberColumns_;i++) {
      //CoinFactorizationDouble value = region[i]*elements[i];
      CoinFactorizationDouble value = region[i];
      for (int j=0;j<i;j++) {
	value -= region[j]*elements[j];
      }
      //region[i] = value;
      region[i] = value*elements[i];
      elements += numberRows_;
    }
    // base factorization L
    elements = elements_+numberRows_*numberRows_;
    for (i=numberColumns_-1;i>=0;i--) {
      elements -= numberRows_;
      CoinFactorizationDouble value = region[i];
      for (int j=i+1;j<numberRows_;j++) {
	value -= region[j]*elements[j];
      }
      region[i] = value;
    }
#ifdef DENSE_CODE
  } else {
    char trans = 'T';
    int ione=1;
    int info;
    F77_FUNC(dgetrs,DGETRS)(&trans,&numberRows_,&ione,elements_,&numberRows_,
			      pivotRow_,region,&numberRows_,&info,1);
  }
#endif
  // permute back and get nonzeros
  numberNonZero=0;
#ifdef DENSE_CODE
  if ((solveMode_%10)==0) {
#endif
    if (!regionSparse2->packedMode()) {
      for (int j=0;j<numberRows_;j++) {
	int iRow = pivotRow_[j+numberRows_];
	double value = region[j];
	region[j]=0.0;
	if (fabs(value)>zeroTolerance_) {
	  region2[iRow] = value;
	  regionIndex[numberNonZero++]=iRow;
	}
      }
    } else {
      for (int j=0;j<numberRows_;j++) {
	int iRow = pivotRow_[j+numberRows_];
	double value = region[j];
	region[j]=0.0;
	if (fabs(value)>zeroTolerance_) {
	  region2[numberNonZero] = value;
	  regionIndex[numberNonZero++]=iRow;
	}
      }
    }
#ifdef DENSE_CODE
  } else {
    // lapack
    if (!regionSparse2->packedMode()) {
      for (int j=0;j<numberRows_;j++) {
	double value = region[j];
	region[j]=0.0;
	if (fabs(value)>zeroTolerance_) {
	  region2[j] = value;
	  regionIndex[numberNonZero++]=j;
	}
      }
    } else {
      for (int j=0;j<numberRows_;j++) {
	double value = region[j];
	region[j]=0.0;
	if (fabs(value)>zeroTolerance_) {
	  region2[numberNonZero] = value;
	  regionIndex[numberNonZero++]=j;
	}
      }
    }
  }
#endif
  regionSparse2->setNumElements(numberNonZero);
  return 0;
}
// Default constructor
CoinOtherFactorization::CoinOtherFactorization (  )
   :  pivotTolerance_(1.0e-1),
      zeroTolerance_(1.0e-13),
#ifndef COIN_FAST_CODE
      slackValue_(-1.0),
#endif
      relaxCheck_(1.0),
      factorElements_(0),
      numberRows_(0),
      numberColumns_(0),
      numberGoodU_(0),
      maximumPivots_(200),
      numberPivots_(0),
      status_(-1),
      solveMode_(0)
{
}
// Copy constructor 
CoinOtherFactorization::CoinOtherFactorization ( const CoinOtherFactorization &other)
   :  pivotTolerance_(other.pivotTolerance_),
  zeroTolerance_(other.zeroTolerance_),
#ifndef COIN_FAST_CODE
  slackValue_(other.slackValue_),
#endif
  relaxCheck_(other.relaxCheck_),
  factorElements_(other.factorElements_),
  numberRows_(other.numberRows_),
  numberColumns_(other.numberColumns_),
  numberGoodU_(other.numberGoodU_),
  maximumPivots_(other.maximumPivots_),
  numberPivots_(other.numberPivots_),
      status_(other.status_),
      solveMode_(other.solveMode_)
{
}
// Destructor
CoinOtherFactorization::~CoinOtherFactorization (  )
{
}
// = copy
CoinOtherFactorization & CoinOtherFactorization::operator = ( const CoinOtherFactorization & other )
{
  if (this != &other) {    
    pivotTolerance_ = other.pivotTolerance_;
    zeroTolerance_ = other.zeroTolerance_;
#ifndef COIN_FAST_CODE
    slackValue_ = other.slackValue_;
#endif
    relaxCheck_ = other.relaxCheck_;
    factorElements_ = other.factorElements_;
    numberRows_ = other.numberRows_;
    numberColumns_ = other.numberColumns_;
    numberGoodU_ = other.numberGoodU_;
    maximumPivots_ = other.maximumPivots_;
    numberPivots_ = other.numberPivots_;
    status_ = other.status_;
    solveMode_ = other.solveMode_;
  }
  return *this;
}
void CoinOtherFactorization::pivotTolerance (  double value )
{
  if (value>0.0&&value<=1.0) {
    pivotTolerance_=value;
  }
}
void CoinOtherFactorization::zeroTolerance (  double value )
{
  if (value>0.0&&value<1.0) {
    zeroTolerance_=value;
  }
}
#ifndef COIN_FAST_CODE
void CoinOtherFactorization::slackValue (  double value )
{
  if (value>=0.0) {
    slackValue_=1.0;
  } else {
    slackValue_=-1.0;
  }
}
#endif
void 
CoinOtherFactorization::maximumPivots (  int value )
{
  if (value>maximumPivots_) {
    delete [] pivotRow_;
    pivotRow_ = new int[2*maximumRows_+value];
  }
  maximumPivots_ = value;
}
// Number of entries in each row
int * 
CoinOtherFactorization::numberInRow() const
{ return reinterpret_cast<int *> (workArea_);}
// Number of entries in each column
int * 
CoinOtherFactorization::numberInColumn() const
{ return (reinterpret_cast<int *> (workArea_))+numberRows_;}
// Returns array to put basis starts in
CoinBigIndex * 
CoinOtherFactorization::starts() const
{ return reinterpret_cast<CoinBigIndex *> (pivotRow_);}
// Returns array to put basis elements in
CoinFactorizationDouble * 
CoinOtherFactorization::elements() const
{ return elements_;}
// Returns pivot row 
int * 
CoinOtherFactorization::pivotRow() const
{ return pivotRow_;}
// Returns work area
CoinFactorizationDouble * 
CoinOtherFactorization::workArea() const
{ return workArea_;}
// Returns int work area
int * 
CoinOtherFactorization::intWorkArea() const
{ return reinterpret_cast<int *> (workArea_);}
// Returns permute back
int * 
CoinOtherFactorization::permuteBack() const
{ return pivotRow_+numberRows_;}
// Returns true if wants tableauColumn in replaceColumn
bool
CoinOtherFactorization::wantsTableauColumn() const
{ return true;}
/* Useful information for factorization
   0 - iteration number
   whereFrom is 0 for factorize and 1 for replaceColumn
*/
void 
CoinOtherFactorization::setUsefulInformation(const int * ,int )
{ }
