/* $Id: CoinFactorization4.cpp 1553 2012-10-30 17:13:11Z forrest $ */
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#if defined(_MSC_VER)
// Turn off compiler warning about long names
#  pragma warning(disable:4786)
#endif

#include "CoinUtilsConfig.h"

#include <cassert>
#include <cstdio>

#include "CoinFactorization.hpp"
#include "CoinIndexedVector.hpp"
#include "CoinHelperFunctions.hpp"
#include <stdio.h>
#include <iostream>
#if DENSE_CODE==1 
// using simple lapack interface
extern "C" 
{
  /** LAPACK Fortran subroutine DGETRS. */
  void F77_FUNC(dgetrs,DGETRS)(char *trans, cipfint *n,
                               cipfint *nrhs, const double *A, cipfint *ldA,
                               cipfint * ipiv, double *B, cipfint *ldB, ipfint *info,
			       int trans_len);
}
#endif
// For semi-sparse
#define BITS_PER_CHECK 8
#define CHECK_SHIFT 3
typedef unsigned char CoinCheckZero;

//:class CoinFactorization.  Deals with Factorization and Updates


//  getRowSpaceIterate.  Gets space for one Row with given length
//may have to do compression  (returns true)
//also moves existing vector
bool
CoinFactorization::getRowSpaceIterate ( int iRow,
                                      int extraNeeded )
{
  const int * numberInRow = numberInRow_.array();
  int number = numberInRow[iRow];
  CoinBigIndex * COIN_RESTRICT startRow = startRowU_.array();
  int * COIN_RESTRICT indexColumn = indexColumnU_.array();
  CoinBigIndex * COIN_RESTRICT convertRowToColumn = convertRowToColumnU_.array();
  CoinBigIndex space = lengthAreaU_ - startRow[maximumRowsExtra_];
  int * COIN_RESTRICT nextRow = nextRow_.array();
  int * COIN_RESTRICT lastRow = lastRow_.array();
  if ( space < extraNeeded + number + 2 ) {
    //compression
    int iRow = nextRow[maximumRowsExtra_];
    CoinBigIndex put = 0;
    while ( iRow != maximumRowsExtra_ ) {
      //move
      CoinBigIndex get = startRow[iRow];
      CoinBigIndex getEnd = startRow[iRow] + numberInRow[iRow];
      
      startRow[iRow] = put;
      CoinBigIndex i;
      for ( i = get; i < getEnd; i++ ) {
	indexColumn[put] = indexColumn[i];
	convertRowToColumn[put] = convertRowToColumn[i];
	put++;
      }       
      iRow = nextRow[iRow];
    }       /* endwhile */
    numberCompressions_++;
    startRow[maximumRowsExtra_] = put;
    space = lengthAreaU_ - put;
    if ( space < extraNeeded + number + 2 ) {
      //need more space
      //if we can allocate bigger then do so and copy
      //if not then return so code can start again
      status_ = -99;
      return false;    }       
  }       
  CoinBigIndex put = startRow[maximumRowsExtra_];
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
  CoinBigIndex get = startRow[iRow];
  
  int * indexColumnU = indexColumnU_.array();
  startRow[iRow] = put;
  while ( number ) {
    number--;
    indexColumnU[put] = indexColumnU[get];
    convertRowToColumn[put] = convertRowToColumn[get];
    put++;
    get++;
  }       /* endwhile */
  //add four for luck
  startRow[maximumRowsExtra_] = put + extraNeeded + 4;
  return true;
}

//  getColumnSpaceIterateR.  Gets space for one extra R element in Column
//may have to do compression  (returns true)
//also moves existing vector
bool
CoinFactorization::getColumnSpaceIterateR ( int iColumn, double value,
					   int iRow)
{
  CoinFactorizationDouble * COIN_RESTRICT elementR = elementR_ + lengthAreaR_;
  int * COIN_RESTRICT indexRowR = indexRowR_ + lengthAreaR_;
  CoinBigIndex * COIN_RESTRICT startR = startColumnR_.array()+maximumPivots_+1;
  int * COIN_RESTRICT numberInColumnPlus = numberInColumnPlus_.array();
  int number = numberInColumnPlus[iColumn];
  //*** modify so sees if can go in
  //see if it can go in at end
  int * COIN_RESTRICT nextColumn = nextColumn_.array();
  int * COIN_RESTRICT lastColumn = lastColumn_.array();
  if (lengthAreaR_-startR[maximumColumnsExtra_]<number+1) {
    //compression
    int jColumn = nextColumn[maximumColumnsExtra_];
    CoinBigIndex put = 0;
    while ( jColumn != maximumColumnsExtra_ ) {
      //move
      CoinBigIndex get;
      CoinBigIndex getEnd;

      get = startR[jColumn];
      getEnd = get + numberInColumnPlus[jColumn];
      startR[jColumn] = put;
      CoinBigIndex i;
      for ( i = get; i < getEnd; i++ ) {
	indexRowR[put] = indexRowR[i];
	elementR[put] = elementR[i];
	put++;
      }
      jColumn = nextColumn[jColumn];
    }
    numberCompressions_++;
    startR[maximumColumnsExtra_]=put;
  }
  // Still may not be room (as iColumn was still in)
  if (lengthAreaR_-startR[maximumColumnsExtra_]<number+1) 
    return false;

  int next = nextColumn[iColumn];
  int last = lastColumn[iColumn];

  //out
  nextColumn[last] = next;
  lastColumn[next] = last;

  CoinBigIndex put = startR[maximumColumnsExtra_];
  //in at end
  last = lastColumn[maximumColumnsExtra_];
  nextColumn[last] = iColumn;
  lastColumn[maximumColumnsExtra_] = iColumn;
  lastColumn[iColumn] = last;
  nextColumn[iColumn] = maximumColumnsExtra_;
  
  //move
  CoinBigIndex get = startR[iColumn];
  startR[iColumn] = put;
  int i = 0;
  for (i=0 ; i < number; i ++ ) {
    elementR[put]= elementR[get];
    indexRowR[put++] = indexRowR[get++];
  }
  //insert
  elementR[put]=value;
  indexRowR[put++]=iRow;
  numberInColumnPlus[iColumn]++;
  //add 4 for luck
  startR[maximumColumnsExtra_] = CoinMin(static_cast<CoinBigIndex> (put + 4) ,lengthAreaR_);
  return true;
}
int CoinFactorization::checkPivot(double saveFromU,
				 double oldPivot) const
{
  int status;
  if ( fabs ( saveFromU ) > 1.0e-8 ) {
    double checkTolerance;
    
    if ( numberRowsExtra_ < numberRows_ + 2 ) {
      checkTolerance = 1.0e-5;
    } else if ( numberRowsExtra_ < numberRows_ + 10 ) {
      checkTolerance = 1.0e-6;
    } else if ( numberRowsExtra_ < numberRows_ + 50 ) {
      checkTolerance = 1.0e-8;
    } else {
      checkTolerance = 1.0e-10;
    }       
    checkTolerance *= relaxCheck_;
    if ( fabs ( 1.0 - fabs ( saveFromU / oldPivot ) ) < checkTolerance ) {
      status = 0;
    } else {
#if COIN_DEBUG
      std::cout <<"inaccurate pivot "<< oldPivot << " " 
		<< saveFromU << std::endl;
#endif
      if ( fabs ( fabs ( oldPivot ) - fabs ( saveFromU ) ) < 1.0e-12 ||
        fabs ( 1.0 - fabs ( saveFromU / oldPivot ) ) < 1.0e-8 ) {
        status = 1;
      } else {
        status = 2;
      }       
    }       
  } else {
    //error
    status = 2;
#if COIN_DEBUG
    std::cout <<"inaccurate pivot "<< saveFromU / oldPivot 
	      << " " << saveFromU << std::endl;
#endif
  } 
  return status;
}
//  replaceColumn.  Replaces one Column to basis
//      returns 0=OK, 1=Probably OK, 2=singular, 3=no room
//partial update already in U
int
CoinFactorization::replaceColumn ( CoinIndexedVector * regionSparse,
                                 int pivotRow,
				  double pivotCheck ,
				   bool checkBeforeModifying,
				   double )
{
  assert (numberU_<=numberRowsExtra_);
  CoinBigIndex * COIN_RESTRICT startColumnU = startColumnU_.array();
  CoinBigIndex * COIN_RESTRICT startColumn;
  int * COIN_RESTRICT indexRow;
  CoinFactorizationDouble * COIN_RESTRICT element;
  
  //return at once if too many iterations
  if ( numberColumnsExtra_ >= maximumColumnsExtra_ ) {
    return 5;
  }       
  if ( lengthAreaU_ < startColumnU[maximumColumnsExtra_] ) {
    return 3;
  }   
  
  int * COIN_RESTRICT numberInRow = numberInRow_.array();
  int * COIN_RESTRICT numberInColumn = numberInColumn_.array();
  int * COIN_RESTRICT numberInColumnPlus = numberInColumnPlus_.array();
  int realPivotRow;
  realPivotRow = pivotColumn_.array()[pivotRow];
  //zeroed out region
  double * COIN_RESTRICT region = regionSparse->denseVector (  );
  
  element = elementU_.array();
  //take out old pivot column

  // If we have done no pivots then always check before modification
  if (!numberPivots_)
    checkBeforeModifying=true;
  
  totalElements_ -= numberInColumn[realPivotRow];
  CoinFactorizationDouble * COIN_RESTRICT pivotRegion = pivotRegion_.array();
  CoinFactorizationDouble oldPivot = pivotRegion[realPivotRow];
  // for accuracy check
  pivotCheck = pivotCheck / oldPivot;
#if COIN_DEBUG>1
  int checkNumber=1000000;
  //if (numberL_) checkNumber=-1;
  if (numberR_>=checkNumber) {
    printf("pivot row %d, check %g - alpha region:\n",
      realPivotRow,pivotCheck);
      /*int i;
      for (i=0;i<numberRows_;i++) {
      if (pivotRegion[i])
      printf("%d %g\n",i,pivotRegion[i]);
  }*/
  }   
#endif
  pivotRegion[realPivotRow] = 0.0;

  CoinBigIndex saveEnd = startColumnU[realPivotRow]
                         + numberInColumn[realPivotRow];
  // not necessary at present - but take no chances for future
  numberInColumn[realPivotRow] = 0;
  //get entries in row (pivot not stored)
  int numberNonZero = 0;
  int * COIN_RESTRICT indexColumn = indexColumnU_.array();
  CoinBigIndex * COIN_RESTRICT convertRowToColumn = convertRowToColumnU_.array();
  int * COIN_RESTRICT regionIndex = regionSparse->getIndices (  );
  CoinBigIndex * COIN_RESTRICT startRow = startRowU_.array();
  CoinBigIndex start=0;
  CoinBigIndex end=0;
#if COIN_DEBUG>1
  if (numberR_>=checkNumber) 
    printf("Before btranu\n");
#endif

#if COIN_ONE_ETA_COPY
  if (convertRowToColumn) {
#endif
    start = startRow[realPivotRow];
    end = start + numberInRow[realPivotRow];
    
    int smallestIndex=numberRowsExtra_;
    if (!checkBeforeModifying) {
      for (CoinBigIndex i = start; i < end ; i ++ ) {
	int iColumn = indexColumn[i];
	assert (iColumn<numberRowsExtra_);
	smallestIndex = CoinMin(smallestIndex,iColumn);
	CoinBigIndex j = convertRowToColumn[i];
	
	region[iColumn] = element[j];
#if COIN_DEBUG>1
	if (numberR_>=checkNumber) 
	  printf("%d %g\n",iColumn,region[iColumn]);
#endif
	element[j] = 0.0;
	regionIndex[numberNonZero++] = iColumn;
      }
    } else {
      for (CoinBigIndex i = start; i < end ; i ++ ) {
	int iColumn = indexColumn[i];
	smallestIndex = CoinMin(smallestIndex,iColumn);
	CoinBigIndex j = convertRowToColumn[i];
	
	region[iColumn] = element[j];
#if COIN_DEBUG>1
	if (numberR_>=checkNumber) 
	  printf("%d %g\n",iColumn,region[iColumn]);
#endif
	regionIndex[numberNonZero++] = iColumn;
      }
    }       
    //do BTRAN - finding first one to use
    regionSparse->setNumElements ( numberNonZero );
    updateColumnTransposeU ( regionSparse, smallestIndex );
#if COIN_ONE_ETA_COPY
  } else {
    // use R to save where elements are
    CoinBigIndex * saveWhere = NULL;
    if (checkBeforeModifying) {
      if ( lengthR_ + maximumRowsExtra_ +1>= lengthAreaR_ ) {
	//not enough room
	return 3;
      }       
      saveWhere = indexRowR_+lengthR_;
    }
    replaceColumnU(regionSparse,saveWhere,
		   realPivotRow);
  }
#endif
  numberNonZero = regionSparse->getNumElements (  );
  CoinFactorizationDouble saveFromU = 0.0;

  CoinBigIndex startU = startColumnU[numberColumnsExtra_];
  int * COIN_RESTRICT indexU = &indexRowU_.array()[startU];
  CoinFactorizationDouble * COIN_RESTRICT elementU = &elementU_.array()[startU];
  

  // Do accuracy test here if caller is paranoid
  if (checkBeforeModifying) {
    double tolerance = zeroTolerance_;
    int number = numberInColumn[numberColumnsExtra_];
  
    for (CoinBigIndex i = 0; i < number; i++ ) {
      int iRow = indexU[i];
      //if (numberCompressions_==99&&lengthU_==278)
      //printf("row %d saveFromU %g element %g region %g\n",
      //       iRow,saveFromU,elementU[i],region[iRow]);
      if ( fabs ( elementU[i] ) > tolerance ) {
	if ( iRow != realPivotRow ) {
	  saveFromU -= elementU[i] * region[iRow];
	} else {
	  saveFromU += elementU[i];
	}       
      }       
    }       
    //check accuracy
    int status = checkPivot(saveFromU,pivotCheck);
    if (status) {
      // restore some things
      pivotRegion[realPivotRow] = oldPivot;
      number = saveEnd-startColumnU[realPivotRow];
      totalElements_ += number;
      numberInColumn[realPivotRow]=number;
      regionSparse->clear();
      return status;
#if COIN_ONE_ETA_COPY
    } else if (convertRowToColumn) {
#else
    } else {
#endif
      // do what we would have done by now
      for (CoinBigIndex i = start; i < end ; i ++ ) {
	CoinBigIndex j = convertRowToColumn[i];
	element[j] = 0.0;
      }
#if COIN_ONE_ETA_COPY
    } else {
      // delete elements
      // used R to save where elements are
      CoinBigIndex * saveWhere = indexRowR_+lengthR_;
      CoinFactorizationDouble *element = elementU_.array();
      int n = saveWhere[0];
      for (int i=0;i<n;i++) {
	CoinBigIndex where = saveWhere[i+1];
	element[where]=0.0;
      }
      //printf("deleting els\n");
#endif
    }
  }
  // Now zero out column of U
  //take out old pivot column
  for (CoinBigIndex i = startColumnU[realPivotRow]; i < saveEnd ; i ++ ) {
    element[i] = 0.0;
  }       
  //zero out pivot Row (before or after?)
  //add to R
  startColumn = startColumnR_.array();
  indexRow = indexRowR_;
  element = elementR_;
  CoinBigIndex l = lengthR_;
  int number = numberR_;
  
  startColumn[number] = l;  //for luck and first time
  number++;
  startColumn[number] = l + numberNonZero;
  numberR_ = number;
  lengthR_ = l + numberNonZero;
  totalElements_ += numberNonZero;
  if ( lengthR_ >= lengthAreaR_ ) {
    //not enough room
    regionSparse->clear();
    return 3;
  }       
#if COIN_DEBUG>1
  if (numberR_>=checkNumber) 
    printf("After btranu\n");
#endif
  for (CoinBigIndex i = 0; i < numberNonZero; i++ ) {
    int iRow = regionIndex[i];
#if COIN_DEBUG>1
    if (numberR_>=checkNumber) 
      printf("%d %g\n",iRow,region[iRow]);
#endif
    
    indexRow[l] = iRow;
    element[l] = region[iRow];
    l++;
  }       
  int * nextRow;
  int * lastRow;
  int next;
  int last;
#if COIN_ONE_ETA_COPY
  if (convertRowToColumn) {
#endif
    //take out row
    nextRow = nextRow_.array();
    lastRow = lastRow_.array();
    next = nextRow[realPivotRow];
    last = lastRow[realPivotRow];
    
    nextRow[last] = next;
    lastRow[next] = last;
    numberInRow[realPivotRow]=0;
#if COIN_DEBUG
    nextRow[realPivotRow] = 777777;
    lastRow[realPivotRow] = 777777;
#endif
#if COIN_ONE_ETA_COPY
  }
#endif
  //do permute
  permute_.array()[numberRowsExtra_] = realPivotRow;
  // and other way
  permuteBack_.array()[realPivotRow] = numberRowsExtra_;
  permuteBack_.array()[numberRowsExtra_] = -1;;
  //and for safety
  permute_.array()[numberRowsExtra_ + 1] = 0;

  pivotColumn_.array()[pivotRow] = numberRowsExtra_;
  pivotColumnBack()[numberRowsExtra_] = pivotRow;
  startColumn = startColumnU;
  indexRow = indexRowU_.array();
  element = elementU_.array();

  numberU_++;
  number = numberInColumn[numberColumnsExtra_];

  totalElements_ += number;
  lengthU_ += number;
  if ( lengthU_ >= lengthAreaU_ ) {
    //not enough room
    regionSparse->clear();
    return 3;
  }
       
  saveFromU = 0.0;
  
  //put in pivot
  //add row counts

#if COIN_DEBUG>1
  if (numberR_>=checkNumber) 
    printf("On U\n");
#endif
#if COIN_ONE_ETA_COPY
  if (convertRowToColumn) {
#endif
    for (CoinBigIndex i = 0; i < number; i++ ) {
      int iRow = indexU[i];
#if COIN_DEBUG>1
      if (numberR_>=checkNumber) 
	printf("%d %g\n",iRow,elementU[i]);
#endif
      
      //assert ( fabs ( elementU[i] ) > zeroTolerance_ );
      if ( iRow != realPivotRow ) {
	int next = nextRow[iRow];
	int iNumberInRow = numberInRow[iRow];
	CoinBigIndex space;
	CoinBigIndex put = startRow[iRow] + iNumberInRow;
	
	space = startRow[next] - put;
	if ( space <= 0 ) {
	  getRowSpaceIterate ( iRow, iNumberInRow + 4 );
	  put = startRow[iRow] + iNumberInRow;
	}     
	indexColumn[put] = numberColumnsExtra_;
	convertRowToColumn[put] = i + startU;
	numberInRow[iRow] = iNumberInRow + 1;
	saveFromU = saveFromU - elementU[i] * region[iRow];
      } else {
	//zero out and save
	saveFromU += elementU[i];
	elementU[i] = 0.0;
      }       
    }       
    //in at end
    last = lastRow[maximumRowsExtra_];
    nextRow[last] = numberRowsExtra_;
    lastRow[maximumRowsExtra_] = numberRowsExtra_;
    lastRow[numberRowsExtra_] = last;
    nextRow[numberRowsExtra_] = maximumRowsExtra_;
    startRow[numberRowsExtra_] = startRow[maximumRowsExtra_];
    numberInRow[numberRowsExtra_] = 0;
#if COIN_ONE_ETA_COPY
  } else {
    //abort();
    for (CoinBigIndex i = 0; i < number; i++ ) {
      int iRow = indexU[i];
#if COIN_DEBUG>1
      if (numberR_>=checkNumber) 
	printf("%d %g\n",iRow,elementU[i]);
#endif
      
      if ( fabs ( elementU[i] ) > tolerance ) {
	if ( iRow != realPivotRow ) {
	  saveFromU = saveFromU - elementU[i] * region[iRow];
	} else {
	  //zero out and save
	  saveFromU += elementU[i];
	  elementU[i] = 0.0;
	}       
      } else {
	elementU[i] = 0.0;
      }       
    }       
  }
#endif
  //column in at beginning (as empty)
  int * COIN_RESTRICT nextColumn = nextColumn_.array();
  int * COIN_RESTRICT lastColumn = lastColumn_.array();
  next = nextColumn[maximumColumnsExtra_];
  lastColumn[next] = numberColumnsExtra_;
  nextColumn[maximumColumnsExtra_] = numberColumnsExtra_;
  nextColumn[numberColumnsExtra_] = next;
  lastColumn[numberColumnsExtra_] = maximumColumnsExtra_;
  //check accuracy - but not if already checked (optimization problem)
  int status =  (checkBeforeModifying) ? 0 : checkPivot(saveFromU,pivotCheck);

  if (status!=2) {
  
    CoinFactorizationDouble pivotValue = 1.0 / saveFromU;
    
    pivotRegion[numberRowsExtra_] = pivotValue;
    //modify by pivot
    for (CoinBigIndex i = 0; i < number; i++ ) {
      elementU[i] *= pivotValue;
    }       
    maximumU_ = CoinMax(maximumU_,startU+number);
    numberRowsExtra_++;
    numberColumnsExtra_++;
    numberGoodU_++;
    numberPivots_++;
  }       
  if ( numberRowsExtra_ > numberRows_ + 50 ) {
    CoinBigIndex extra = factorElements_ >> 1;
    
    if ( numberRowsExtra_ > numberRows_ + 100 + numberRows_ / 500 ) {
      if ( extra < 2 * numberRows_ ) {
        extra = 2 * numberRows_;
      }       
    } else {
      if ( extra < 5 * numberRows_ ) {
        extra = 5 * numberRows_;
      }       
    }       
    CoinBigIndex added = totalElements_ - factorElements_;
    
    if ( added > extra && added > ( factorElements_ ) << 1 && !status 
	 && 3*totalElements_ > 2*(lengthAreaU_+lengthAreaL_)) {
      status = 3;
      if ( messageLevel_ & 4 ) {
        std::cout << "Factorization has "<< totalElements_
          << ", basis had " << factorElements_ <<std::endl;
      }
    }       
  }
  if (numberInColumnPlus&&status<2) {
    // we are going to put another copy of R in R
    CoinFactorizationDouble * COIN_RESTRICT elementR = elementR_ + lengthAreaR_;
    int * COIN_RESTRICT indexRowR = indexRowR_ + lengthAreaR_;
    CoinBigIndex * COIN_RESTRICT startR = startColumnR_.array()+maximumPivots_+1;
    int pivotRow = numberRowsExtra_-1;
    for (CoinBigIndex i = 0; i < numberNonZero; i++ ) {
      int iRow = regionIndex[i];
      assert (pivotRow>iRow);
      next = nextColumn[iRow];
      CoinBigIndex space;
      if (next!=maximumColumnsExtra_)
	space = startR[next]-startR[iRow];
      else
	space = lengthAreaR_-startR[iRow];
      int numberInR = numberInColumnPlus[iRow];
      if (space>numberInR) {
	// there is space
	CoinBigIndex  put=startR[iRow]+numberInR;
	numberInColumnPlus[iRow]=numberInR+1;
	indexRowR[put]=pivotRow;
	elementR[put]=region[iRow];
	//add 4 for luck
	if (next==maximumColumnsExtra_)
	  startR[maximumColumnsExtra_] = CoinMin(static_cast<CoinBigIndex> (put + 4) ,lengthAreaR_);
      } else {
	// no space - do we shuffle?
	if (!getColumnSpaceIterateR(iRow,region[iRow],pivotRow)) {
	  // printf("Need more space for R\n");
	  numberInColumnPlus_.conditionalDelete();
	  regionSparse->clear();
	  break;
	}
      }
      region[iRow]=0.0;
    }       
    regionSparse->setNumElements(0);
  } else {
    regionSparse->clear();
  }
  return status;
}

//  updateColumnTranspose.  Updates one column transpose (BTRAN)
int
CoinFactorization::updateColumnTranspose ( CoinIndexedVector * regionSparse,
                                          CoinIndexedVector * regionSparse2 ) 
  const
{
  //zero region
  regionSparse->clear (  );
  double * COIN_RESTRICT region = regionSparse->denseVector (  );
  double * COIN_RESTRICT vector = regionSparse2->denseVector();
  int * COIN_RESTRICT index = regionSparse2->getIndices();
  int numberNonZero = regionSparse2->getNumElements();
  const int * pivotColumn = pivotColumn_.array();
  
  //move indices into index array
  int * COIN_RESTRICT regionIndex = regionSparse->getIndices (  );
  bool packed = regionSparse2->packedMode();
  if (packed) {
    for (int i = 0; i < numberNonZero; i ++ ) {
      int iRow = index[i];
      double value = vector[i];
      iRow=pivotColumn[iRow];
      vector[i]=0.0;
      region[iRow] = value;
      regionIndex[i] = iRow;
    }
  } else {
    for (int i = 0; i < numberNonZero; i ++ ) {
      int iRow = index[i];
      double value = vector[iRow];
      vector[iRow]=0.0;
      iRow=pivotColumn[iRow];
      region[iRow] = value;
      regionIndex[i] = iRow;
    }
  }
  regionSparse->setNumElements ( numberNonZero );
  if (collectStatistics_) {
    numberBtranCounts_++;
    btranCountInput_ += static_cast<double> (numberNonZero);
  }
  if (!doForrestTomlin_) {
    // Do PFI before everything else
    updateColumnTransposePFI(regionSparse);
    numberNonZero = regionSparse->getNumElements();
  }
  //  ******* U
  // Apply pivot region - could be combined for speed
  CoinFactorizationDouble * COIN_RESTRICT pivotRegion = pivotRegion_.array();
  
  int smallestIndex=numberRowsExtra_;
  for (int j = 0; j < numberNonZero; j++ ) {
    int iRow = regionIndex[j];
    smallestIndex = CoinMin(smallestIndex,iRow);
    region[iRow] *= pivotRegion[iRow];
  }
  updateColumnTransposeU ( regionSparse,smallestIndex );
  if (collectStatistics_) 
    btranCountAfterU_ += static_cast<double> (regionSparse->getNumElements());
  //permute extra
  //row bits here
  updateColumnTransposeR ( regionSparse );
  //  ******* L
  updateColumnTransposeL ( regionSparse );
  numberNonZero = regionSparse->getNumElements (  );
  if (collectStatistics_) 
    btranCountAfterL_ += static_cast<double> (numberNonZero);
  const int * permuteBack = pivotColumnBack();
  int number=0;
  if (packed) {
    for (int i=0;i<numberNonZero;i++) {
      int iRow=regionIndex[i];
      double value = region[iRow];
      region[iRow]=0.0;
      //if (fabs(value)>zeroTolerance_) {
	iRow=permuteBack[iRow];
	vector[number]=value;
	index[number++]=iRow;
	//}
    }
  } else {
    for (int i=0;i<numberNonZero;i++) {
      int iRow=regionIndex[i];
      double value = region[iRow];
      region[iRow]=0.0;
      //if (fabs(value)>zeroTolerance_) {
	iRow=permuteBack[iRow];
	vector[iRow]=value;
	index[number++]=iRow;
	//}
    }
  }
  regionSparse->setNumElements(0);
  regionSparse2->setNumElements(number);
#ifdef COIN_DEBUG
  for (i=0;i<numberRowsExtra_;i++) {
    assert (!region[i]);
  }
#endif
  return number;
}

/* Updates part of column transpose (BTRANU) when densish,
   assumes index is sorted i.e. region is correct */
void 
CoinFactorization::updateColumnTransposeUDensish 
                        ( CoinIndexedVector * regionSparse,
			  int smallestIndex) const
{
  double * COIN_RESTRICT region = regionSparse->denseVector (  );
  int numberNonZero = regionSparse->getNumElements (  );
  double tolerance = zeroTolerance_;
  
  int * COIN_RESTRICT regionIndex = regionSparse->getIndices (  );
  
  const CoinBigIndex *startRow = startRowU_.array();
  
  const CoinBigIndex *convertRowToColumn = convertRowToColumnU_.array();
  const int *indexColumn = indexColumnU_.array();
  
  const CoinFactorizationDouble * element = elementU_.array();
  int last = numberU_;
  
  const int *numberInRow = numberInRow_.array();
  numberNonZero = 0;
  for (int i=smallestIndex ; i < last; i++ ) {
    CoinFactorizationDouble pivotValue = region[i];
    if ( fabs ( pivotValue ) > tolerance ) {
      CoinBigIndex start = startRow[i];
      int numberIn = numberInRow[i];
      CoinBigIndex end = start + numberIn;
      for (CoinBigIndex j = start ; j < end; j ++ ) {
	int iRow = indexColumn[j];
	CoinBigIndex getElement = convertRowToColumn[j];
	CoinFactorizationDouble value = element[getElement];
	region[iRow] -=  value * pivotValue;
      }     
      regionIndex[numberNonZero++] = i;
    } else {
      region[i] = 0.0;
    }       
  }
  //set counts
  regionSparse->setNumElements ( numberNonZero );
}
/* Updates part of column transpose (BTRANU) when sparsish,
      assumes index is sorted i.e. region is correct */
void 
CoinFactorization::updateColumnTransposeUSparsish 
                        ( CoinIndexedVector * regionSparse,
			  int smallestIndex) const
{
  double * COIN_RESTRICT region = regionSparse->denseVector (  );
  int numberNonZero = regionSparse->getNumElements (  );
  double tolerance = zeroTolerance_;
  
  int * COIN_RESTRICT regionIndex = regionSparse->getIndices (  );
  
  const CoinBigIndex *startRow = startRowU_.array();
  
  const CoinBigIndex *convertRowToColumn = convertRowToColumnU_.array();
  const int *indexColumn = indexColumnU_.array();
  
  const CoinFactorizationDouble * element = elementU_.array();
  int last = numberU_;
  
  const int *numberInRow = numberInRow_.array();
  
  // mark known to be zero
  int nInBig = sizeof(CoinBigIndex)/sizeof(int);
  CoinCheckZero * COIN_RESTRICT mark = reinterpret_cast<CoinCheckZero *> (sparse_.array()+(2+nInBig)*maximumRowsExtra_);

  for (int i=0;i<numberNonZero;i++) {
    int iPivot=regionIndex[i];
    int iWord = iPivot>>CHECK_SHIFT;
    int iBit = iPivot-(iWord<<CHECK_SHIFT);
    if (mark[iWord]) {
      mark[iWord] = static_cast<CoinCheckZero>(mark[iWord] | (1<<iBit));
    } else {
      mark[iWord] = static_cast<CoinCheckZero>(1<<iBit);
    }
  }

  numberNonZero = 0;
  // Find convenient power of 2
  smallestIndex = smallestIndex >> CHECK_SHIFT;
  int kLast = last>>CHECK_SHIFT;
  // do in chunks

  for (int k=smallestIndex;k<kLast;k++) {
    unsigned int iMark = mark[k];
    if (iMark) {
      // something in chunk - do all (as imark may change)
      int i = k<<CHECK_SHIFT;
      int iLast = i+BITS_PER_CHECK;
      for ( ; i < iLast; i++ ) {
	CoinFactorizationDouble pivotValue = region[i];
	if ( fabs ( pivotValue ) > tolerance ) {
	  CoinBigIndex start = startRow[i];
	  int numberIn = numberInRow[i];
	  CoinBigIndex end = start + numberIn;
	  for (CoinBigIndex j = start ; j < end; j ++ ) {
	    int iRow = indexColumn[j];
	    CoinBigIndex getElement = convertRowToColumn[j];
	    CoinFactorizationDouble value = element[getElement];
	    int iWord = iRow>>CHECK_SHIFT;
	    int iBit = iRow-(iWord<<CHECK_SHIFT);
	    if (mark[iWord]) {
	      mark[iWord] = static_cast<CoinCheckZero>(mark[iWord] | (1<<iBit));
	    } else {
	      mark[iWord] = static_cast<CoinCheckZero>(1<<iBit);
	    }
	    region[iRow] -=  value * pivotValue;
	  }     
	  regionIndex[numberNonZero++] = i;
	} else {
	  region[i] = 0.0;
	}       
      }
      mark[k]=0;
    }
  }
  mark[kLast]=0;
  for (int i = kLast<<CHECK_SHIFT; i < last; i++ ) {
    CoinFactorizationDouble pivotValue = region[i];
    if ( fabs ( pivotValue ) > tolerance ) {
      CoinBigIndex start = startRow[i];
      int numberIn = numberInRow[i];
      CoinBigIndex end = start + numberIn;
      for (CoinBigIndex j = start ; j < end; j ++ ) {
	int iRow = indexColumn[j];
	CoinBigIndex getElement = convertRowToColumn[j];
	CoinFactorizationDouble value = element[getElement];

	region[iRow] -=  value * pivotValue;
      }     
      regionIndex[numberNonZero++] = i;
    } else {
      region[i] = 0.0;
    }       
  }
#ifdef COIN_DEBUG
  for (int i=0;i<maximumRowsExtra_;i++) {
    assert (!mark[i]);
  }
#endif
  //set counts
  regionSparse->setNumElements ( numberNonZero );
}
/* Updates part of column transpose (BTRANU) when sparse,
   assumes index is sorted i.e. region is correct */
void 
CoinFactorization::updateColumnTransposeUSparse ( 
		   CoinIndexedVector * regionSparse) const
{
  double * COIN_RESTRICT region = regionSparse->denseVector (  );
  int numberNonZero = regionSparse->getNumElements (  );
  double tolerance = zeroTolerance_;
  
  int * COIN_RESTRICT regionIndex = regionSparse->getIndices (  );
  const CoinBigIndex *startRow = startRowU_.array();
  
  const CoinBigIndex *convertRowToColumn = convertRowToColumnU_.array();
  const int *indexColumn = indexColumnU_.array();
  
  const CoinFactorizationDouble * element = elementU_.array();
  
  const int *numberInRow = numberInRow_.array();
  
  // use sparse_ as temporary area
  // mark known to be zero
  int * COIN_RESTRICT stack = sparse_.array();  /* pivot */
  int * COIN_RESTRICT list = stack + maximumRowsExtra_;  /* final list */
  CoinBigIndex * COIN_RESTRICT next = reinterpret_cast<CoinBigIndex *> (list + maximumRowsExtra_);  /* jnext */
  char * COIN_RESTRICT mark = reinterpret_cast<char *> (next + maximumRowsExtra_);
  int nList;
#ifdef COIN_DEBUG
  for (int i=0;i<maximumRowsExtra_;i++) {
    assert (!mark[i]);
  }
#endif
#if 0
  {
    int i;
    for (i=0;i<numberRowsExtra_;i++) {
      CoinBigIndex krs = startRow[i];
      CoinBigIndex kre = krs + numberInRow[i];
      CoinBigIndex k;
      for (k=krs;k<kre;k++)
	assert (indexColumn[k]>i);
    }
  }
#endif
  nList=0;
  for (int k=0;k<numberNonZero;k++) {
    int kPivot=regionIndex[k];
    stack[0]=kPivot;
    CoinBigIndex j=startRow[kPivot]+numberInRow[kPivot]-1;
    next[0]=j;
    int nStack=1;
    while (nStack) {
      /* take off stack */
      kPivot=stack[--nStack];
      if (mark[kPivot]!=1) {
	j=next[nStack];
	if (j>=startRow[kPivot]) {
	  kPivot=indexColumn[j--];
	  /* put back on stack */
	  next[nStack++] =j;
	  if (!mark[kPivot]) {
	    /* and new one */
	    j=startRow[kPivot]+numberInRow[kPivot]-1;
	    stack[nStack]=kPivot;
	    mark[kPivot]=2;
	    next[nStack++]=j;
	  }
	} else {
	  // finished
	  list[nList++]=kPivot;
	  mark[kPivot]=1;
	}
      }
    }
  }
  numberNonZero=0;
  for (int i=nList-1;i>=0;i--) {
    int iPivot = list[i];
    mark[iPivot]=0;
    CoinFactorizationDouble pivotValue = region[iPivot];
    if ( fabs ( pivotValue ) > tolerance ) {
      CoinBigIndex start = startRow[iPivot];
      int numberIn = numberInRow[iPivot];
      CoinBigIndex end = start + numberIn;
      for (CoinBigIndex j=start ; j < end; j ++ ) {
	int iRow = indexColumn[j];
	CoinBigIndex getElement = convertRowToColumn[j];
	CoinFactorizationDouble value = element[getElement];
	region[iRow] -= value * pivotValue;
      }     
      regionIndex[numberNonZero++] = iPivot;
    } else {
      region[iPivot] = 0.0;
    }       
  }       
  //set counts
  regionSparse->setNumElements ( numberNonZero );
}
//  updateColumnTransposeU.  Updates part of column transpose (BTRANU)
//assumes index is sorted i.e. region is correct
//does not sort by sign
void
CoinFactorization::updateColumnTransposeU ( CoinIndexedVector * regionSparse,
					    int smallestIndex) const
{
#if COIN_ONE_ETA_COPY
  CoinBigIndex *convertRowToColumn = convertRowToColumnU_.array();
  if (!convertRowToColumn) {
    //abort();
    updateColumnTransposeUByColumn(regionSparse,smallestIndex);
    return;
  }
#endif
  int number = regionSparse->getNumElements (  );
  int goSparse;
  // Guess at number at end
  if (sparseThreshold_>0) {
    if (btranAverageAfterU_) {
      int newNumber = static_cast<int> (number*btranAverageAfterU_);
      if (newNumber< sparseThreshold_)
	goSparse = 2;
      else if (newNumber< sparseThreshold2_)
	goSparse = 1;
      else
	goSparse = 0;
    } else {
      if (number<sparseThreshold_) 
	goSparse = 2;
      else
	goSparse = 0;
    }
  } else {
    goSparse=0;
  }
  switch (goSparse) {
  case 0: // densish
    updateColumnTransposeUDensish(regionSparse,smallestIndex);
    break;
  case 1: // middling
    updateColumnTransposeUSparsish(regionSparse,smallestIndex);
    break;
  case 2: // sparse
    updateColumnTransposeUSparse(regionSparse);
    break;
  }
}

/*  updateColumnTransposeLDensish.  
    Updates part of column transpose (BTRANL) dense by column */
void
CoinFactorization::updateColumnTransposeLDensish 
     ( CoinIndexedVector * regionSparse ) const
{
  double * COIN_RESTRICT region = regionSparse->denseVector (  );
  int * COIN_RESTRICT regionIndex = regionSparse->getIndices (  );
  int numberNonZero;
  double tolerance = zeroTolerance_;
  int base;
  int first = -1;
  
  numberNonZero=0;
  //scan
  for (first=numberRows_-1;first>=0;first--) {
    if (region[first]) 
      break;
  }
  if ( first >= 0 ) {
    base = baseL_;
    const CoinBigIndex * COIN_RESTRICT startColumn = startColumnL_.array();
    const int * COIN_RESTRICT indexRow = indexRowL_.array();
    const CoinFactorizationDouble * COIN_RESTRICT element = elementL_.array();
    int last = baseL_ + numberL_;
    
    if ( first >= last ) {
      first = last - 1;
    }       
    for (int i = first ; i >= base; i-- ) {
      CoinBigIndex j;
      CoinFactorizationDouble pivotValue = region[i];
      for ( j= startColumn[i] ; j < startColumn[i+1]; j++ ) {
	int iRow = indexRow[j];
	CoinFactorizationDouble value = element[j];
	pivotValue -= value * region[iRow];
      }       
      if ( fabs ( pivotValue ) > tolerance ) {
	region[i] = pivotValue;
	regionIndex[numberNonZero++] = i;
      } else { 
	region[i] = 0.0;
      }       
    }       
    //may have stopped early
    if ( first < base ) {
      base = first + 1;
    }
    if (base > 5) {
      int i=base-1;
      CoinFactorizationDouble pivotValue=region[i];
      bool store = fabs(pivotValue) > tolerance;
      for (; i > 0; i-- ) {
	bool oldStore = store;
	CoinFactorizationDouble oldValue = pivotValue;
	pivotValue = region[i-1];
	store = fabs ( pivotValue ) > tolerance ;
	if (!oldStore) {
	  region[i] = 0.0;
	} else {
	  region[i] = oldValue;
	  regionIndex[numberNonZero++] = i;
	}       
      }     
      if (store) {
	region[0] = pivotValue;
	regionIndex[numberNonZero++] = 0;
      } else {
	region[0] = 0.0;
      }       
    } else {
      for (int i = base -1 ; i >= 0; i-- ) {
	CoinFactorizationDouble pivotValue = region[i];
	if ( fabs ( pivotValue ) > tolerance ) {
	  region[i] = pivotValue;
	  regionIndex[numberNonZero++] = i;
	} else {
	  region[i] = 0.0;
	}       
      }     
    }
  } 
  //set counts
  regionSparse->setNumElements ( numberNonZero );
}
/*  updateColumnTransposeLByRow. 
    Updates part of column transpose (BTRANL) densish but by row */
void
CoinFactorization::updateColumnTransposeLByRow 
    ( CoinIndexedVector * regionSparse ) const
{
  double * COIN_RESTRICT region = regionSparse->denseVector (  );
  int * COIN_RESTRICT regionIndex = regionSparse->getIndices (  );
  int numberNonZero;
  double tolerance = zeroTolerance_;
  int first = -1;
  
  // use row copy of L
  const CoinFactorizationDouble * element = elementByRowL_.array();
  const CoinBigIndex * startRow = startRowL_.array();
  const int * column = indexColumnL_.array();
  for (first=numberRows_-1;first>=0;first--) {
    if (region[first]) 
      break;
  }
  numberNonZero=0;
  for (int i=first;i>=0;i--) {
    CoinFactorizationDouble pivotValue = region[i];
    if ( fabs ( pivotValue ) > tolerance ) {
      regionIndex[numberNonZero++] = i;
      CoinBigIndex j;
      for (j = startRow[i + 1]-1;j >= startRow[i]; j--) {
	int iRow = column[j];
	CoinFactorizationDouble value = element[j];
	region[iRow] -= pivotValue*value;
      }
    } else {
      region[i] = 0.0;
    }     
  }
  //set counts
  regionSparse->setNumElements ( numberNonZero );
}
// Updates part of column transpose (BTRANL) when sparsish by row
void
CoinFactorization::updateColumnTransposeLSparsish 
    ( CoinIndexedVector * regionSparse ) const
{
  double * COIN_RESTRICT region = regionSparse->denseVector (  );
  int * COIN_RESTRICT regionIndex = regionSparse->getIndices (  );
  int numberNonZero = regionSparse->getNumElements();
  double tolerance = zeroTolerance_;
  
  // use row copy of L
  const CoinFactorizationDouble * element = elementByRowL_.array();
  const CoinBigIndex * startRow = startRowL_.array();
  const int * column = indexColumnL_.array();
  // mark known to be zero
  int nInBig = sizeof(CoinBigIndex)/sizeof(int);
  CoinCheckZero * COIN_RESTRICT mark = reinterpret_cast<CoinCheckZero *> (sparse_.array()+(2+nInBig)*maximumRowsExtra_);
  for (int i=0;i<numberNonZero;i++) {
    int iPivot=regionIndex[i];
    int iWord = iPivot>>CHECK_SHIFT;
    int iBit = iPivot-(iWord<<CHECK_SHIFT);
    if (mark[iWord]) {
      mark[iWord] = static_cast<CoinCheckZero>(mark[iWord] | (1<<iBit));
    } else {
      mark[iWord] = static_cast<CoinCheckZero>(1<<iBit);
    }
  }
  numberNonZero = 0;
  // First do down to convenient power of 2
  int jLast = (numberRows_-1)>>CHECK_SHIFT;
  jLast = (jLast<<CHECK_SHIFT);
  for (int i=numberRows_-1;i>=jLast;i--) {
    CoinFactorizationDouble pivotValue = region[i];
    if ( fabs ( pivotValue ) > tolerance ) {
      regionIndex[numberNonZero++] = i;
      CoinBigIndex j;
      for (j = startRow[i + 1]-1;j >= startRow[i]; j--) {
	int iRow = column[j];
	CoinFactorizationDouble value = element[j];
	int iWord = iRow>>CHECK_SHIFT;
	int iBit = iRow-(iWord<<CHECK_SHIFT);
	if (mark[iWord]) {
	  mark[iWord] = static_cast<CoinCheckZero>(mark[iWord] | (1<<iBit));
	} else {
	  mark[iWord] = static_cast<CoinCheckZero>(1<<iBit);
	}
	region[iRow] -= pivotValue*value;
      }
    } else {
      region[i] = 0.0;
    }     
  }
  // and in chunks
  jLast = jLast>>CHECK_SHIFT;
  mark[jLast]=0;
  for (int k=jLast-1;k>=0;k--) {
    unsigned int iMark = mark[k];
    if (iMark) {
      // something in chunk - do all (as imark may change)
      int iLast = k<<CHECK_SHIFT;
      for (int i = iLast+BITS_PER_CHECK-1; i >= iLast; i-- ) {
	CoinFactorizationDouble pivotValue = region[i];
	if ( fabs ( pivotValue ) > tolerance ) {
	  regionIndex[numberNonZero++] = i;
	  CoinBigIndex j;
	  for (j = startRow[i + 1]-1;j >= startRow[i]; j--) {
	    int iRow = column[j];
	    CoinFactorizationDouble value = element[j];
	    int iWord = iRow>>CHECK_SHIFT;
	    int iBit = iRow-(iWord<<CHECK_SHIFT);
	    if (mark[iWord]) {
	      mark[iWord] = static_cast<CoinCheckZero>(mark[iWord] | (1<<iBit));
	    } else {
	      mark[iWord] = static_cast<CoinCheckZero>(1<<iBit);
	    }
	    region[iRow] -= pivotValue*value;
	  }
	} else {
	  region[i] = 0.0;
	}     
      }
      mark[k]=0;
    }
  }
#ifdef COIN_DEBUG
  for (int i=0;i<maximumRowsExtra_;i++) {
    assert (!mark[i]);
  }
#endif
  //set counts
  regionSparse->setNumElements ( numberNonZero );
}
/*  updateColumnTransposeLSparse. 
    Updates part of column transpose (BTRANL) sparse */
void
CoinFactorization::updateColumnTransposeLSparse 
    ( CoinIndexedVector * regionSparse ) const
{
  double * COIN_RESTRICT region = regionSparse->denseVector (  );
  int * COIN_RESTRICT regionIndex = regionSparse->getIndices (  );
  int numberNonZero = regionSparse->getNumElements (  );
  double tolerance = zeroTolerance_;
  
  // use row copy of L
  const CoinFactorizationDouble * element = elementByRowL_.array();
  const CoinBigIndex * startRow = startRowL_.array();
  const int * column = indexColumnL_.array();
  // use sparse_ as temporary area
  // mark known to be zero
  int * COIN_RESTRICT stack = sparse_.array();  /* pivot */
  int * COIN_RESTRICT list = stack + maximumRowsExtra_;  /* final list */
  CoinBigIndex * COIN_RESTRICT next = reinterpret_cast<CoinBigIndex *> (list + maximumRowsExtra_);  /* jnext */
  char * COIN_RESTRICT mark = reinterpret_cast<char *> (next + maximumRowsExtra_);
  int nList;
  int number = numberNonZero;
#ifdef COIN_DEBUG
  for (i=0;i<maximumRowsExtra_;i++) {
    assert (!mark[i]);
  }
#endif
  nList=0;
  for (int k=0;k<number;k++) {
    int kPivot=regionIndex[k];
    if(!mark[kPivot]&&region[kPivot]) {
      stack[0]=kPivot;
      CoinBigIndex j=startRow[kPivot+1]-1;
      int nStack=0;
      while (nStack>=0) {
	/* take off stack */
	if (j>=startRow[kPivot]) {
	  int jPivot=column[j--];
	  /* put back on stack */
	  next[nStack] =j;
	  if (!mark[jPivot]) {
	    /* and new one */
	    kPivot=jPivot;
	    j = startRow[kPivot+1]-1;
	    stack[++nStack]=kPivot;
	    mark[kPivot]=1;
	    next[nStack]=j;
	  }
	} else {
	  /* finished so mark */
	  list[nList++]=kPivot;
	  mark[kPivot]=1;
	  --nStack;
	  if (nStack>=0) {
	    kPivot=stack[nStack];
	    j=next[nStack];
	  }
	}
      }
    }
  }
  numberNonZero=0;
  for (int i=nList-1;i>=0;i--) {
    int iPivot = list[i];
    mark[iPivot]=0;
    CoinFactorizationDouble pivotValue = region[iPivot];
    if ( fabs ( pivotValue ) > tolerance ) {
      regionIndex[numberNonZero++] = iPivot;
      CoinBigIndex j;
      for ( j = startRow[iPivot]; j < startRow[iPivot+1]; j ++ ) {
	int iRow = column[j];
	CoinFactorizationDouble value = element[j];
	region[iRow] -= value * pivotValue;
      }
    } else {
      region[iPivot]=0.0;
    }
  }
  //set counts
  regionSparse->setNumElements ( numberNonZero );
}
//  updateColumnTransposeL.  Updates part of column transpose (BTRANL)
void
CoinFactorization::updateColumnTransposeL ( CoinIndexedVector * regionSparse ) const
{
  int number = regionSparse->getNumElements (  );
  if (!numberL_&&!numberDense_) {
    if (sparse_.array()||number<numberRows_)
      return;
  }
  int goSparse;
  // Guess at number at end
  // we may need to rethink on dense
  if (sparseThreshold_>0) {
    if (btranAverageAfterL_) {
      int newNumber = static_cast<int> (number*btranAverageAfterL_);
      if (newNumber< sparseThreshold_)
	goSparse = 2;
      else if (newNumber< sparseThreshold2_)
	goSparse = 1;
      else
	goSparse = 0;
    } else {
      if (number<sparseThreshold_) 
	goSparse = 2;
      else
	goSparse = 0;
    }
  } else {
    goSparse=-1;
  }
#ifdef DENSE_CODE
  if (numberDense_) {
    //take off list
    int lastSparse = numberRows_-numberDense_;
    int number = regionSparse->getNumElements();
    double * COIN_RESTRICT region = regionSparse->denseVector (  );
    int * COIN_RESTRICT regionIndex = regionSparse->getIndices (  );
    int i=0;
    bool doDense=false;
    if (number<=numberRows_) {
      while (i<number) {
	int iRow = regionIndex[i];
	if (iRow>=lastSparse) {
	  doDense=true;
	  regionIndex[i] = regionIndex[--number];
	} else {
	  i++;
	}
      }
    } else {
      for (i=numberRows_-1;i>=lastSparse;i--) {
	if (region[i]) {
	  doDense=true;
          // numbers are all wrong - do a scan
          regionSparse->setNumElements(0);
          regionSparse->scan(0,lastSparse,zeroTolerance_);
          number=regionSparse->getNumElements();
	  break;
	}
      }
      if (sparseThreshold_)
	goSparse=0;
      else
	goSparse=-1;
    }
    if (doDense) {
      regionSparse->setNumElements(number);
      char trans = 'T';
      int ione=1;
      int info;
      F77_FUNC(dgetrs,DGETRS)(&trans,&numberDense_,&ione,denseArea_,&numberDense_,
			      densePermute_,region+lastSparse,&numberDense_,&info,1);
      //and scan again
      if (goSparse>0||!numberL_)
	regionSparse->scan(lastSparse,numberRows_,zeroTolerance_);
    } 
    if (!numberL_) {
      // could be odd combination of sparse/dense
      if (number>numberRows_) {
	regionSparse->setNumElements(0);
	regionSparse->scan(0,numberRows_,zeroTolerance_);
      }
      return;
    }
  } 
#endif
  if (goSparse>0&&regionSparse->getNumElements()>numberRows_)
    goSparse=0;
  switch (goSparse) {
  case -1: // No row copy
    updateColumnTransposeLDensish(regionSparse);
    break;
  case 0: // densish but by row
    updateColumnTransposeLByRow(regionSparse);
    break;
  case 1: // middling(and by row)
    updateColumnTransposeLSparsish(regionSparse);
    break;
  case 2: // sparse
    updateColumnTransposeLSparse(regionSparse);
    break;
  }
}
#if COIN_ONE_ETA_COPY
/* Combines BtranU and delete elements
   If deleted is NULL then delete elements
   otherwise store where elements are
*/
void 
CoinFactorization::replaceColumnU ( CoinIndexedVector * regionSparse,
				    CoinBigIndex * deleted,
				    int internalPivotRow)
{
  double * COIN_RESTRICT region = regionSparse->denseVector();
  int * COIN_RESTRICT regionIndex = regionSparse->getIndices();
  double tolerance = zeroTolerance_;
  const CoinBigIndex *startColumn = startColumnU_.array();
  const int *indexRow = indexRowU_.array();
  CoinFactorizationDouble *element = elementU_.array();
  int numberNonZero = 0;
  const int *numberInColumn = numberInColumn_.array();
  //const CoinFactorizationDouble *pivotRegion = pivotRegion_.array();
  bool deleteNow=true;
  if (deleted) {
    deleteNow = false;
    deleted ++;
  }
  int nPut=0;
  for (int i = CoinMax(numberSlacks_,internalPivotRow) ;
       i < numberU_; i++ ) {
    assert (!region[i]);
    CoinFactorizationDouble pivotValue = 0.0; //region[i]*pivotRegion[i];
    //printf("Epv %g reg %g pr %g\n",
    //   pivotValue,region[i],pivotRegion[i]);
    //pivotValue = region[i];
    for (CoinBigIndex  j= startColumn[i] ; 
	 j < startColumn[i]+numberInColumn[i]; j++ ) {
      int iRow = indexRow[j];
      CoinFactorizationDouble value = element[j];
      if (iRow!=internalPivotRow) {
	pivotValue -= value * region[iRow];
      } else {
	assert (!region[iRow]);
	pivotValue += value;
	if (deleteNow)
	  element[j]=0.0;
	else
	  deleted[nPut++]=j;
      }
    }       
    if ( fabs ( pivotValue ) > tolerance ) {
      regionIndex[numberNonZero++] = i;
      region[i] = pivotValue;
    } else {
      region[i] = 0;
    }       
  }
  if (!deleteNow) {
    deleted--;
    deleted[0]=nPut;
  }
  regionSparse->setNumElements(numberNonZero);
}
/* Updates part of column transpose (BTRANU) by column
   assumes index is sorted i.e. region is correct */
void 
CoinFactorization::updateColumnTransposeUByColumn ( CoinIndexedVector * regionSparse,
						    int smallestIndex) const
{
  //CoinIndexedVector temp = *regionSparse;
  //updateColumnTransposeUDensish(&temp,smallestIndex);
  double * COIN_RESTRICT region = regionSparse->denseVector();
  int * COIN_RESTRICT regionIndex = regionSparse->getIndices();
  double tolerance = zeroTolerance_;
  const CoinBigIndex *startColumn = startColumnU_.array();
  const int *indexRow = indexRowU_.array();
  const CoinFactorizationDouble *element = elementU_.array();
  int numberNonZero = 0;
  const int *numberInColumn = numberInColumn_.array();
  const CoinFactorizationDouble *pivotRegion = pivotRegion_.array();
  
  for (int i = smallestIndex;i<numberSlacks_; i++) {
    double value = region[i];
    if ( value ) {
      //region[i]=-value;
      regionIndex[numberNonZero]=i;
      if ( fabs(value) > tolerance ) 
	numberNonZero++;
      else 
	region[i]=0.0;
    }
  }
  for (int i = CoinMax(numberSlacks_,smallestIndex) ;
       i < numberU_; i++ ) {
    CoinFactorizationDouble pivotValue = region[i]*pivotRegion[i];
    //printf("pv %g reg %g pr %g\n",
    //   pivotValue,region[i],pivotRegion[i]);
    pivotValue = region[i];
    for (CoinBigIndex  j= startColumn[i] ; 
	 j < startColumn[i]+numberInColumn[i]; j++ ) {
      int iRow = indexRow[j];
      CoinFactorizationDouble value = element[j];
      pivotValue -= value * region[iRow];
    }       
    if ( fabs ( pivotValue ) > tolerance ) {
      regionIndex[numberNonZero++] = i;
      region[i] = pivotValue;
    } else {
      region[i] = 0;
    }       
  }
  regionSparse->setNumElements(numberNonZero);
  //double * region2 = temp.denseVector();
  //for (i=0;i<maximumRowsExtra_;i++) {
  //assert(fabs(region[i]-region2[i])<1.0e-4);
  //}
}
#endif
// Updates part of column transpose (BTRANR) when dense
void 
CoinFactorization::updateColumnTransposeRDensish 
( CoinIndexedVector * regionSparse ) const
{
  double * COIN_RESTRICT region = regionSparse->denseVector (  );

#ifdef COIN_DEBUG
  regionSparse->checkClean();
#endif
  int last = numberRowsExtra_-1;
  
  
  const int *indexRow = indexRowR_;
  const CoinFactorizationDouble *element = elementR_;
  const CoinBigIndex * startColumn = startColumnR_.array()-numberRows_;
  //move using permute_ (stored in inverse fashion)
  const int * permute = permute_.array();
  
  for (int i = last ; i >= numberRows_; i-- ) {
    int putRow = permute[i];
    CoinFactorizationDouble pivotValue = region[i];
    //zero out  old permuted
    region[i] = 0.0;
    if ( pivotValue ) {
      for (CoinBigIndex j = startColumn[i]; j < startColumn[i+1]; j++ ) {
	CoinFactorizationDouble value = element[j];
	int iRow = indexRow[j];
	region[iRow] -= value * pivotValue;
      }
      region[putRow] = pivotValue;
      //putRow must have been zero before so put on list ??
      //but can't catch up so will have to do L from end
      //unless we update lookBtran in replaceColumn - yes
    }
  }
}
// Updates part of column transpose (BTRANR) when sparse
void 
CoinFactorization::updateColumnTransposeRSparse 
( CoinIndexedVector * regionSparse ) const
{
  double * COIN_RESTRICT region = regionSparse->denseVector (  );
  int * COIN_RESTRICT regionIndex = regionSparse->getIndices (  );
  int numberNonZero = regionSparse->getNumElements (  );
  double tolerance = zeroTolerance_;

#ifdef COIN_DEBUG
  regionSparse->checkClean();
#endif
  int last = numberRowsExtra_-1;
  
  
  const int *indexRow = indexRowR_;
  const CoinFactorizationDouble *element = elementR_;
  const CoinBigIndex * startColumn = startColumnR_.array()-numberRows_;
  //move using permute_ (stored in inverse fashion)
  const int * permute = permute_.array();
    
  // we can use sparse_ as temporary array
  int * COIN_RESTRICT spare = sparse_.array();
  for (int i=0;i<numberNonZero;i++) {
    spare[regionIndex[i]]=i;
  }
  // still need to do in correct order (for now)
  for (int i = last ; i >= numberRows_; i-- ) {
    int putRow = permute[i];
    assert (putRow<=i);
    CoinFactorizationDouble pivotValue = region[i];
    //zero out  old permuted
    region[i] = 0.0;
    if ( pivotValue ) {
      for (CoinBigIndex j = startColumn[i]; j < startColumn[i+1]; j++ ) {
	CoinFactorizationDouble value = element[j];
	int iRow = indexRow[j];
	CoinFactorizationDouble oldValue = region[iRow];
	CoinFactorizationDouble newValue = oldValue - value * pivotValue;
	if (oldValue) {
	  if (newValue) 
	    region[iRow]=newValue;
	  else
	    region[iRow]=COIN_INDEXED_REALLY_TINY_ELEMENT;
	} else if (fabs(newValue)>tolerance) {
	  region[iRow] = newValue;
	  spare[iRow]=numberNonZero;
	  regionIndex[numberNonZero++]=iRow;
	}
      }
      region[putRow] = pivotValue;
      // modify list
      int position=spare[i];
      regionIndex[position]=putRow;
      spare[putRow]=position;
    }
  }
  regionSparse->setNumElements(numberNonZero);
}

//  updateColumnTransposeR.  Updates part of column (FTRANR)
void
CoinFactorization::updateColumnTransposeR ( CoinIndexedVector * regionSparse ) const
{
  if (numberRowsExtra_==numberRows_)
    return;
  int numberNonZero = regionSparse->getNumElements (  );

  if (numberNonZero) {
    if (numberNonZero < (sparseThreshold_<<2)||(!numberL_&&sparse_.array())) {
      updateColumnTransposeRSparse ( regionSparse );
      if (collectStatistics_) 
	btranCountAfterR_ += regionSparse->getNumElements();
    } else {
      updateColumnTransposeRDensish ( regionSparse );
      // we have lost indices
      // make sure won't try and go sparse again
      if (collectStatistics_) 
	btranCountAfterR_ += CoinMin((numberNonZero<<1),numberRows_);
      regionSparse->setNumElements (numberRows_+1);
    }
  }
}
//  makes a row copy of L
void
CoinFactorization::goSparse ( )
{
  if (!sparseThreshold_) {
    if (numberRows_>300) {
      if (numberRows_<10000) {
	sparseThreshold_=CoinMin(numberRows_/6,500);
	//sparseThreshold2_=sparseThreshold_;
      } else {
	sparseThreshold_=1000;
	//sparseThreshold2_=sparseThreshold_;
      }
      sparseThreshold2_=numberRows_>>2;
    } else {
      sparseThreshold_=0;
      sparseThreshold2_=0;
    }
  } else {
    if (!sparseThreshold_&&numberRows_>400) {
      sparseThreshold_=CoinMin((numberRows_-300)/9,1000);
    }
    sparseThreshold2_=sparseThreshold_;
  }
  if (!sparseThreshold_)
    return;
  // allow for stack, list, next and char map of mark
  int nRowIndex = (maximumRowsExtra_+CoinSizeofAsInt(int)-1)/
    CoinSizeofAsInt(char);
  int nInBig = static_cast<int>(sizeof(CoinBigIndex)/sizeof(int));
  assert (nInBig>=1);
  sparse_.conditionalNew( (2+nInBig)*maximumRowsExtra_ + nRowIndex );
  // zero out mark
  memset(sparse_.array()+(2+nInBig)*maximumRowsExtra_,
         0,maximumRowsExtra_*sizeof(char));
  elementByRowL_.conditionalDelete();
  indexColumnL_.conditionalDelete();
  startRowL_.conditionalNew(numberRows_+1);
  if (lengthAreaL_) {
    elementByRowL_.conditionalNew(lengthAreaL_);
    indexColumnL_.conditionalNew(lengthAreaL_);
  }
  // counts
  CoinBigIndex * COIN_RESTRICT startRowL = startRowL_.array();
  CoinZeroN(startRowL,numberRows_);
  const CoinBigIndex * startColumnL = startColumnL_.array();
  CoinFactorizationDouble * COIN_RESTRICT elementL = elementL_.array();
  const int * indexRowL = indexRowL_.array();
  for (int i=baseL_;i<baseL_+numberL_;i++) {
    for (CoinBigIndex j=startColumnL[i];j<startColumnL[i+1];j++) {
      int iRow = indexRowL[j];
      startRowL[iRow]++;
    }
  }
  // convert count to lasts
  CoinBigIndex count=0;
  for (int i=0;i<numberRows_;i++) {
    int numberInRow=startRowL[i];
    count += numberInRow;
    startRowL[i]=count;
  }
  startRowL[numberRows_]=count;
  // now insert
  CoinFactorizationDouble * COIN_RESTRICT elementByRowL = elementByRowL_.array();
  int * COIN_RESTRICT indexColumnL = indexColumnL_.array();
  for (int i=baseL_+numberL_-1;i>=baseL_;i--) {
    for (CoinBigIndex j=startColumnL[i];j<startColumnL[i+1];j++) {
      int iRow = indexRowL[j];
      CoinBigIndex start = startRowL[iRow]-1;
      startRowL[iRow]=start;
      elementByRowL[start]=elementL[j];
      indexColumnL[start]=i;
    }
  }
}

//  set sparse threshold
void
CoinFactorization::sparseThreshold ( int value ) 
{
  if (value>0&&sparseThreshold_) {
    sparseThreshold_=value;
    sparseThreshold2_= sparseThreshold_;
  } else if (!value&&sparseThreshold_) {
    // delete copy
    sparseThreshold_=0;
    sparseThreshold2_= 0;
    elementByRowL_.conditionalDelete();
    startRowL_.conditionalDelete();
    indexColumnL_.conditionalDelete();
    sparse_.conditionalDelete();
  } else if (value>0&&!sparseThreshold_) {
    if (value>1)
      sparseThreshold_=value;
    else
      sparseThreshold_=0;
    sparseThreshold2_= sparseThreshold_;
    goSparse();
  }
}
void CoinFactorization::maximumPivots (  int value )
{
  if (value>0) {
    maximumPivots_=value;
  }
}
void CoinFactorization::messageLevel (  int value )
{
  if (value>0&&value<16) {
    messageLevel_=value;
  }
}
void CoinFactorization::pivotTolerance (  double value )
{
  if (value>0.0&&value<=1.0) {
    pivotTolerance_=value;
  }
}
void CoinFactorization::zeroTolerance (  double value )
{
  if (value>0.0&&value<1.0) {
    zeroTolerance_=value;
  }
}
#ifndef COIN_FAST_CODE
void CoinFactorization::slackValue (  double value )
{
  if (value>=0.0) {
    slackValue_=1.0;
  } else {
    slackValue_=-1.0;
  }
}
#endif
// Reset all sparsity etc statistics
void CoinFactorization::resetStatistics()
{
  collectStatistics_=false;

  /// Below are all to collect
  ftranCountInput_=0.0;
  ftranCountAfterL_=0.0;
  ftranCountAfterR_=0.0;
  ftranCountAfterU_=0.0;
  btranCountInput_=0.0;
  btranCountAfterU_=0.0;
  btranCountAfterR_=0.0;
  btranCountAfterL_=0.0;

  /// We can roll over factorizations
  numberFtranCounts_=0;
  numberBtranCounts_=0;

  /// While these are averages collected over last 
  ftranAverageAfterL_=0.0;
  ftranAverageAfterR_=0.0;
  ftranAverageAfterU_=0.0;
  btranAverageAfterU_=0.0;
  btranAverageAfterR_=0.0;
  btranAverageAfterL_=0.0; 
}
/*  getColumnSpaceIterate.  Gets space for one extra U element in Column
    may have to do compression  (returns true)
    also moves existing vector.
    Returns -1 if no memory or where element was put
    Used by replaceRow (turns off R version) */
CoinBigIndex 
CoinFactorization::getColumnSpaceIterate ( int iColumn, double value,
					   int iRow)
{
  if (numberInColumnPlus_.array()) {
    numberInColumnPlus_.conditionalDelete();
  }
  int * COIN_RESTRICT numberInRow = numberInRow_.array();
  int * COIN_RESTRICT numberInColumn = numberInColumn_.array();
  int * COIN_RESTRICT nextColumn = nextColumn_.array();
  int * COIN_RESTRICT lastColumn = lastColumn_.array();
  int number = numberInColumn[iColumn];
  int iNext=nextColumn[iColumn];
  CoinBigIndex * COIN_RESTRICT startColumnU = startColumnU_.array();
  CoinBigIndex * COIN_RESTRICT startRowU = startRowU_.array();
  CoinBigIndex space = startColumnU[iNext]-startColumnU[iColumn];
  CoinBigIndex put;
  CoinBigIndex * COIN_RESTRICT convertRowToColumnU = convertRowToColumnU_.array();
  int * COIN_RESTRICT indexColumnU = indexColumnU_.array();
  CoinFactorizationDouble * COIN_RESTRICT elementU = elementU_.array();
  int * COIN_RESTRICT indexRowU = indexRowU_.array();
  if ( space < number + 1 ) {
    //see if it can go in at end
    if (lengthAreaU_-startColumnU[maximumColumnsExtra_]<number+1) {
      //compression
      int jColumn = nextColumn[maximumColumnsExtra_];
      CoinBigIndex put = 0;
      while ( jColumn != maximumColumnsExtra_ ) {
	//move
	CoinBigIndex get;
	CoinBigIndex getEnd;

	get = startColumnU[jColumn];
	getEnd = get + numberInColumn[jColumn];
	startColumnU[jColumn] = put;
	CoinBigIndex i;
	for ( i = get; i < getEnd; i++ ) {
	  CoinFactorizationDouble value = elementU[i];
	  if (value) {
	    indexRowU[put] = indexRowU[i];
	    elementU[put] = value;
	    put++;
	  } else {
	    numberInColumn[jColumn]--;
	  }
	}
	jColumn = nextColumn[jColumn];
      }
      numberCompressions_++;
      startColumnU[maximumColumnsExtra_]=put;
      //space for cross reference
      CoinBigIndex *convertRowToColumn = convertRowToColumnU_.array();
      CoinBigIndex j = 0;
      CoinBigIndex *startRow = startRowU_.array();
      
      int iRow;
      for ( iRow = 0; iRow < numberRowsExtra_; iRow++ ) {
	startRow[iRow] = j;
	j += numberInRow[iRow];
      }
      factorElements_=j;
      
      CoinZeroN ( numberInRow, numberRowsExtra_ );
      int i;
      for ( i = 0; i < numberRowsExtra_; i++ ) {
	CoinBigIndex start = startColumnU[i];
	CoinBigIndex end = start + numberInColumn[i];

	CoinBigIndex j;
	for ( j = start; j < end; j++ ) {
	  int iRow = indexRowU[j];
	  int iLook = numberInRow[iRow];
	  
	  numberInRow[iRow] = iLook + 1;
	  CoinBigIndex k = startRow[iRow] + iLook;
	  
	  indexColumnU[k] = i;
	  convertRowToColumn[k] = j;
	}
      }
    }
    // Still may not be room (as iColumn was still in)
    if (lengthAreaU_-startColumnU[maximumColumnsExtra_]>=number+1) {
      int next = nextColumn[iColumn];
      int last = lastColumn[iColumn];
      
      //out
      nextColumn[last] = next;
      lastColumn[next] = last;
      
      put = startColumnU[maximumColumnsExtra_];
      //in at end
      last = lastColumn[maximumColumnsExtra_];
      nextColumn[last] = iColumn;
      lastColumn[maximumColumnsExtra_] = iColumn;
      lastColumn[iColumn] = last;
      nextColumn[iColumn] = maximumColumnsExtra_;
      
      //move
      CoinBigIndex get = startColumnU[iColumn];
      startColumnU[iColumn] = put;
      int i = 0;
      for (i=0 ; i < number; i ++ ) {
	CoinFactorizationDouble value = elementU[get];
	int iRow=indexRowU[get++];
	if (value) {
	  elementU[put]= value;
	  int n=numberInRow[iRow];
	  CoinBigIndex start = startRowU[iRow];
	  CoinBigIndex j;
	  for (j=start;j<start+n;j++) {
	    if (indexColumnU[j]==iColumn) {
	      convertRowToColumnU[j]=put;
	      break;
	    }
	  }
	  assert (j<start+n);
	  indexRowU[put++] = iRow;
	} else {
	  assert (!numberInRow[iRow]);
	  numberInColumn[iColumn]--;
	}
      }
      //insert
      int n=numberInRow[iRow];
      CoinBigIndex start = startRowU[iRow];
      CoinBigIndex j;
      for (j=start;j<start+n;j++) {
	if (indexColumnU[j]==iColumn) {
	  convertRowToColumnU[j]=put;
	  break;
	}
      }
      assert (j<start+n);
      elementU[put]=value;
      indexRowU[put]=iRow;
      numberInColumn[iColumn]++;
      //add 4 for luck
      startColumnU[maximumColumnsExtra_] = CoinMin(static_cast<CoinBigIndex> (put + 4) ,lengthAreaU_);
    } else {
      // no room
      put=-1;
    }
  } else {
    // just slot in
    put=startColumnU[iColumn]+numberInColumn[iColumn];
    int n=numberInRow[iRow];
    CoinBigIndex start = startRowU[iRow];
    CoinBigIndex j;
    for (j=start;j<start+n;j++) {
      if (indexColumnU[j]==iColumn) {
	convertRowToColumnU[j]=put;
	break;
      }
    }
    assert (j<start+n);
    elementU[put]=value;
    indexRowU[put]=iRow;
    numberInColumn[iColumn]++;
  }
  return put;
}
/* Replaces one Row in basis,
   At present assumes just a singleton on row is in basis
   returns 0=OK, 1=Probably OK, 2=singular, 3 no space */
int 
CoinFactorization::replaceRow ( int whichRow, int iNumberInRow,
				const int indicesColumn[], const double elements[] )
{
  if (!iNumberInRow)
    return 0;
  int next = nextRow_.array()[whichRow];
  int * numberInRow = numberInRow_.array();
#ifndef NDEBUG
  const int * numberInColumn = numberInColumn_.array();
#endif
  int numberNow = numberInRow[whichRow];
  const CoinBigIndex * startRowU = startRowU_.array();
  CoinFactorizationDouble * pivotRegion = pivotRegion_.array();
  CoinBigIndex start = startRowU[whichRow];
  CoinFactorizationDouble * elementU = elementU_.array();
  CoinBigIndex *convertRowToColumnU = convertRowToColumnU_.array();
  if (numberNow&&numberNow<100) {
    int ind[100];
    CoinMemcpyN(indexColumnU_.array()+start,numberNow,ind);
    int i;
    for (i=0;i<iNumberInRow;i++) {
      int jColumn=indicesColumn[i];
      int k;
      for (k=0;k<numberNow;k++) {
	if (ind[k]==jColumn) {
	  ind[k]=-1;
	  break;
	}
      }
      if (k==numberNow) {
	printf("new column %d not in current\n",jColumn);
      } else {
	k=convertRowToColumnU[start+k];
	CoinFactorizationDouble oldValue = elementU[k];
	CoinFactorizationDouble newValue = elements[i]*pivotRegion[jColumn];
	if (fabs(oldValue-newValue)>1.0e-3)
	  printf("column %d, old value %g new %g (el %g, piv %g)\n",jColumn,oldValue,
		 newValue,elements[i],pivotRegion[jColumn]);
      }
    }
    for (i=0;i<numberNow;i++) {
      if (ind[i]>=0)
	printf("current column %d not in new\n",ind[i]);
    }
    assert (numberNow==iNumberInRow);
  }
  assert (numberInColumn[whichRow]==0);
  assert (pivotRegion[whichRow]==1.0);
  CoinBigIndex space;
  int returnCode=0;
      
  space = startRowU[next] - (start+iNumberInRow);
  if ( space < 0 ) {
    if (!getRowSpaceIterate ( whichRow, iNumberInRow)) 
      returnCode=3;
  }
  //return 0;
  if (!returnCode) {
    int * indexColumnU = indexColumnU_.array();
    numberInRow[whichRow]=iNumberInRow;
    start = startRowU[whichRow];
    int i;
    for (i=0;i<iNumberInRow;i++) {
      int iColumn = indicesColumn[i];
      indexColumnU[start+i]=iColumn;
      assert (iColumn>whichRow);
      CoinFactorizationDouble value  = elements[i]*pivotRegion[iColumn];
#if 0
      int k;
      bool found=false;
      for (k=startColumnU[iColumn];k<startColumnU[iColumn]+numberInColumn[iColumn];k++) {
	if (indexRowU[k]==whichRow) {
	  assert (fabs(elementU[k]-value)<1.0e-3);
	  found=true;
	  break;
	}
      }
#if 0
      assert (found);
#else
      if (found) {
	int number = numberInColumn[iColumn]-1;
	numberInColumn[iColumn]=number;
	CoinBigIndex j=startColumnU[iColumn]+number;
	if (k<j) {
	  int iRow=indexRowU[j];
	  indexRowU[k]=iRow;
	  elementU[k]=elementU[j];
	  int n=numberInRow[iRow];
	  CoinBigIndex start = startRowU[iRow];
	  for (j=start;j<start+n;j++) {
	    if (indexColumnU[j]==iColumn) {
	      convertRowToColumnU[j]=k;
	      break;
	    }
	  }
	  assert (j<start+n);
	}
      }
      found=false;
#endif
      if (!found) {
#endif
	CoinBigIndex iWhere = getColumnSpaceIterate(iColumn,value,whichRow);
	if (iWhere>=0) {
	  convertRowToColumnU[start+i] = iWhere;
	} else {
	  returnCode=3;
	  break;
	}
#if 0
      } else {
	convertRowToColumnU[start+i] = k;
      }
#endif
    }
  }       
  return returnCode;
}
// Takes out all entries for given rows
void 
CoinFactorization::emptyRows(int numberToEmpty, const int which[])
{
  int i;
  int * delRow = new int [maximumRowsExtra_];
  int * indexRowU = indexRowU_.array();
#ifndef NDEBUG
  CoinFactorizationDouble * pivotRegion = pivotRegion_.array();
#endif
  for (i=0;i<maximumRowsExtra_;i++)
    delRow[i]=0;
  int * numberInRow = numberInRow_.array();
  int * numberInColumn = numberInColumn_.array();
  CoinFactorizationDouble * elementU = elementU_.array();
  const CoinBigIndex * startColumnU = startColumnU_.array();
  for (i=0;i<numberToEmpty;i++) {
    int iRow = which[i];
    delRow[iRow]=1;
    assert (numberInColumn[iRow]==0);
    assert (pivotRegion[iRow]==1.0);
    numberInRow[iRow]=0;
  }
  for (i=0;i<numberU_;i++) {
    CoinBigIndex k;
    CoinBigIndex j=startColumnU[i];
    for (k=startColumnU[i];k<startColumnU[i]+numberInColumn[i];k++) {
      int iRow=indexRowU[k];
      if (!delRow[iRow]) {
	indexRowU[j]=indexRowU[k];
	elementU[j++]=elementU[k];
      }
    }
    numberInColumn[i] = j-startColumnU[i];
  }
  delete [] delRow;
  //space for cross reference
  CoinBigIndex *convertRowToColumn = convertRowToColumnU_.array();
  CoinBigIndex j = 0;
  CoinBigIndex *startRow = startRowU_.array();

  int iRow;
  for ( iRow = 0; iRow < numberRows_; iRow++ ) {
    startRow[iRow] = j;
    j += numberInRow[iRow];
  }
  factorElements_=j;

  CoinZeroN ( numberInRow, numberRows_ );

  int * indexColumnU = indexColumnU_.array();
  for ( i = 0; i < numberRows_; i++ ) {
    CoinBigIndex start = startColumnU[i];
    CoinBigIndex end = start + numberInColumn[i];

    CoinBigIndex j;
    for ( j = start; j < end; j++ ) {
      int iRow = indexRowU[j];
      int iLook = numberInRow[iRow];

      numberInRow[iRow] = iLook + 1;
      CoinBigIndex k = startRow[iRow] + iLook;

      indexColumnU[k] = i;
      convertRowToColumn[k] = j;
    }
  }
}
// Updates part of column PFI (FTRAN)
void 
CoinFactorization::updateColumnPFI ( CoinIndexedVector * regionSparse) const
{
  double *region = regionSparse->denseVector (  );
  int * regionIndex = regionSparse->getIndices();
  double tolerance = zeroTolerance_;
  const CoinBigIndex *startColumn = startColumnU_.array()+numberRows_;
  const int *indexRow = indexRowU_.array();
  const CoinFactorizationDouble *element = elementU_.array();
  int numberNonZero = regionSparse->getNumElements();
  int i;
#ifdef COIN_DEBUG
  for (i=0;i<numberNonZero;i++) {
    int iRow=regionIndex[i];
    assert (iRow>=0&&iRow<numberRows_);
    assert (region[iRow]);
  }
#endif
  const CoinFactorizationDouble *pivotRegion = pivotRegion_.array()+numberRows_;
  const int *pivotColumn = pivotColumn_.array()+numberRows_;

  for (i = 0 ; i <numberPivots_; i++ ) {
    int pivotRow=pivotColumn[i];
    CoinFactorizationDouble pivotValue = region[pivotRow];
    if (pivotValue) {
      if ( fabs ( pivotValue ) > tolerance ) {
	for (CoinBigIndex  j= startColumn[i] ; j < startColumn[i+1]; j++ ) {
	  int iRow = indexRow[j];
	  CoinFactorizationDouble oldValue = region[iRow];
	  CoinFactorizationDouble value = oldValue - pivotValue*element[j];
	  if (!oldValue) {
	    if (fabs(value)>tolerance) {
	      region[iRow]=value;
	      regionIndex[numberNonZero++]=iRow;
	    }
	  } else {
	    if (fabs(value)>tolerance) {
	      region[iRow]=value;
	    } else {
	      region[iRow]=COIN_INDEXED_REALLY_TINY_ELEMENT;
	    }
	  }
	}       
	pivotValue *= pivotRegion[i];
	region[pivotRow]=pivotValue;
      } else if (pivotValue) {
	region[pivotRow]=COIN_INDEXED_REALLY_TINY_ELEMENT;
      }
    }
  }
  regionSparse->setNumElements ( numberNonZero );
}
// Updates part of column transpose PFI (BTRAN),
    
void 
CoinFactorization::updateColumnTransposePFI ( CoinIndexedVector * regionSparse) const
{
  double *region = regionSparse->denseVector (  );
  int numberNonZero = regionSparse->getNumElements();
  int *index = regionSparse->getIndices (  );
  int i;
#ifdef COIN_DEBUG
  for (i=0;i<numberNonZero;i++) {
    int iRow=index[i];
    assert (iRow>=0&&iRow<numberRows_);
    assert (region[iRow]);
  }
#endif
  const int * pivotColumn = pivotColumn_.array()+numberRows_;
  const CoinFactorizationDouble *pivotRegion = pivotRegion_.array()+numberRows_;
  double tolerance = zeroTolerance_;
  
  const CoinBigIndex *startColumn = startColumnU_.array()+numberRows_;
  const int *indexRow = indexRowU_.array();
  const CoinFactorizationDouble *element = elementU_.array();

  for (i=numberPivots_-1 ; i>=0; i-- ) {
    int pivotRow = pivotColumn[i];
    CoinFactorizationDouble pivotValue = region[pivotRow]*pivotRegion[i];
    for (CoinBigIndex  j= startColumn[i] ; j < startColumn[i+1]; j++ ) {
      int iRow = indexRow[j];
      CoinFactorizationDouble value = element[j];
      pivotValue -= value * region[iRow];
    }       
    //pivotValue *= pivotRegion[i];
    if ( fabs ( pivotValue ) > tolerance ) {
      if (!region[pivotRow])
	index[numberNonZero++] = pivotRow;
      region[pivotRow] = pivotValue;
    } else {
      if (region[pivotRow])
	region[pivotRow] = COIN_INDEXED_REALLY_TINY_ELEMENT;
    }       
  }       
  //set counts
  regionSparse->setNumElements ( numberNonZero );
}
/* Replaces one Column to basis for PFI
   returns 0=OK, 1=Probably OK, 2=singular, 3=no room
   Not sure what checkBeforeModifying means for PFI.
*/
int 
CoinFactorization::replaceColumnPFI ( CoinIndexedVector * regionSparse,
				      int pivotRow,
				      double alpha)
{
  CoinBigIndex *startColumn=startColumnU_.array()+numberRows_;
  int *indexRow=indexRowU_.array();
  CoinFactorizationDouble *element=elementU_.array();
  CoinFactorizationDouble * pivotRegion = pivotRegion_.array()+numberRows_;
  // This has incoming column
  const double *region = regionSparse->denseVector (  );
  const int * index = regionSparse->getIndices();
  int numberNonZero = regionSparse->getNumElements();

  int iColumn = numberPivots_;

  if (!iColumn) 
    startColumn[0]=startColumn[maximumColumnsExtra_];
  CoinBigIndex start = startColumn[iColumn];
  
  //return at once if too many iterations
  if ( numberPivots_ >= maximumPivots_ ) {
    return 5;
  }       
  if ( lengthAreaU_ - ( start + numberNonZero ) < 0) {
    return 3;
  }   
  
  int i;
  if (numberPivots_) {
    if (fabs(alpha)<1.0e-5) {
      if (fabs(alpha)<1.0e-7)
	return 2;
      else
	return 1;
    }
  } else {
    if (fabs(alpha)<1.0e-8)
      return 2;
  }
  CoinFactorizationDouble pivotValue = 1.0/alpha;
  pivotRegion[iColumn]=pivotValue;
  double tolerance = zeroTolerance_;
  const int * pivotColumn = pivotColumn_.array();
  // Operations done before permute back
  if (regionSparse->packedMode()) {
    for ( i = 0; i < numberNonZero; i++ ) {
      int iRow = index[i];
      if (iRow!=pivotRow) {
	if ( fabs ( region[i] ) > tolerance ) {
	  indexRow[start]=pivotColumn[iRow];
	  element[start++]=region[i]*pivotValue;
	}
      }
    }    
  } else {
    for ( i = 0; i < numberNonZero; i++ ) {
      int iRow = index[i];
      if (iRow!=pivotRow) {
	if ( fabs ( region[iRow] ) > tolerance ) {
	  indexRow[start]=pivotColumn[iRow];
	  element[start++]=region[iRow]*pivotValue;
	}
      }
    }    
  }   
  numberPivots_++;
  numberNonZero=start-startColumn[iColumn];
  startColumn[numberPivots_]=start;
  totalElements_ += numberNonZero;
  int * pivotColumn2 = pivotColumn_.array()+numberRows_;
  pivotColumn2[iColumn]=pivotColumn[pivotRow];
  return 0;
}
//  =
CoinFactorization & CoinFactorization::operator = ( const CoinFactorization & other ) {
  if (this != &other) {    
    gutsOfDestructor(2);
    gutsOfInitialize(3);
    persistenceFlag_=other.persistenceFlag_;
    gutsOfCopy(other);
  }
  return *this;
}
void CoinFactorization::gutsOfCopy(const CoinFactorization &other)
{
  elementU_.allocate(other.elementU_, other.lengthAreaU_ *CoinSizeofAsInt(CoinFactorizationDouble));
  indexRowU_.allocate(other.indexRowU_, other.lengthAreaU_*CoinSizeofAsInt(int) );
  elementL_.allocate(other.elementL_, other.lengthAreaL_*CoinSizeofAsInt(CoinFactorizationDouble) );
  indexRowL_.allocate(other.indexRowL_, other.lengthAreaL_*CoinSizeofAsInt(int) );
  startColumnL_.allocate(other.startColumnL_,(other.numberRows_ + 1)*CoinSizeofAsInt(CoinBigIndex) );
  int extraSpace;
  if (other.numberInColumnPlus_.array()) {
    extraSpace = other.maximumPivots_ + 1 + other.maximumColumnsExtra_ + 1;
  } else {
    extraSpace = other.maximumPivots_ + 1 ;
  }
  startColumnR_.allocate(other.startColumnR_,extraSpace*CoinSizeofAsInt(CoinBigIndex));
  pivotRegion_.allocate(other.pivotRegion_, (other.maximumRowsExtra_ + 1 )*CoinSizeofAsInt(CoinFactorizationDouble));
  permuteBack_.allocate(other.permuteBack_,(other.maximumRowsExtra_ + 1)*CoinSizeofAsInt(int));
  permute_.allocate(other.permute_,(other.maximumRowsExtra_ + 1)*CoinSizeofAsInt(int));
  pivotColumnBack_.allocate(other.pivotColumnBack_,(other.maximumRowsExtra_ + 1)*CoinSizeofAsInt(int));
  firstCount_.allocate(other.firstCount_,(other.maximumRowsExtra_ + 1)*CoinSizeofAsInt(int));
  startColumnU_.allocate(other.startColumnU_, (other.maximumColumnsExtra_ + 1 )*CoinSizeofAsInt(CoinBigIndex));
  numberInColumn_.allocate(other.numberInColumn_, (other.maximumColumnsExtra_ + 1 )*CoinSizeofAsInt(int));
  pivotColumn_.allocate(other.pivotColumn_,(other.maximumColumnsExtra_ + 1)*CoinSizeofAsInt(int));
  nextColumn_.allocate(other.nextColumn_, (other.maximumColumnsExtra_ + 1 )*CoinSizeofAsInt(int));
  lastColumn_.allocate(other.lastColumn_, (other.maximumColumnsExtra_ + 1 )*CoinSizeofAsInt(int));
  indexColumnU_.allocate(other.indexColumnU_, other.lengthAreaU_*CoinSizeofAsInt(int) );
  nextRow_.allocate(other.nextRow_,(other.maximumRowsExtra_ + 1)*CoinSizeofAsInt(int));
  lastRow_.allocate( other.lastRow_,(other.maximumRowsExtra_ + 1 )*CoinSizeofAsInt(int));
  const CoinBigIndex * convertUOther = other.convertRowToColumnU_.array();
#if COIN_ONE_ETA_COPY
  if (convertUOther) {
#endif
    convertRowToColumnU_.allocate(other.convertRowToColumnU_, other.lengthAreaU_*CoinSizeofAsInt(CoinBigIndex) );
    startRowU_.allocate(other.startRowU_,(other.maximumRowsExtra_ + 1)*CoinSizeofAsInt(CoinBigIndex));
    numberInRow_.allocate(other.numberInRow_, (other.maximumRowsExtra_ + 1 )*CoinSizeofAsInt(int));
#if COIN_ONE_ETA_COPY
  }
#endif
  if (other.sparseThreshold_) {
    elementByRowL_.allocate(other.elementByRowL_, other.lengthAreaL_ );
    indexColumnL_.allocate(other.indexColumnL_, other.lengthAreaL_ );
    startRowL_.allocate(other.startRowL_,other.numberRows_+1);
  }
  numberTrials_ = other.numberTrials_;
  biggerDimension_ = other.biggerDimension_;
  relaxCheck_ = other.relaxCheck_;
  numberSlacks_ = other.numberSlacks_;
  numberU_ = other.numberU_;
  maximumU_=other.maximumU_;
  lengthU_ = other.lengthU_;
  lengthAreaU_ = other.lengthAreaU_;
  numberL_ = other.numberL_;
  baseL_ = other.baseL_;
  lengthL_ = other.lengthL_;
  lengthAreaL_ = other.lengthAreaL_;
  numberR_ = other.numberR_;
  lengthR_ = other.lengthR_;
  lengthAreaR_ = other.lengthAreaR_;
  pivotTolerance_ = other.pivotTolerance_;
  zeroTolerance_ = other.zeroTolerance_;
#ifndef COIN_FAST_CODE
  slackValue_ = other.slackValue_;
#endif
  areaFactor_ = other.areaFactor_;
  numberRows_ = other.numberRows_;
  numberRowsExtra_ = other.numberRowsExtra_;
  maximumRowsExtra_ = other.maximumRowsExtra_;
  numberColumns_ = other.numberColumns_;
  numberColumnsExtra_ = other.numberColumnsExtra_;
  maximumColumnsExtra_ = other.maximumColumnsExtra_;
  maximumPivots_=other.maximumPivots_;
  numberGoodU_ = other.numberGoodU_;
  numberGoodL_ = other.numberGoodL_;
  numberPivots_ = other.numberPivots_;
  messageLevel_ = other.messageLevel_;
  totalElements_ = other.totalElements_;
  factorElements_ = other.factorElements_;
  status_ = other.status_;
  doForrestTomlin_ = other.doForrestTomlin_;
  collectStatistics_=other.collectStatistics_;
  ftranCountInput_=other.ftranCountInput_;
  ftranCountAfterL_=other.ftranCountAfterL_;
  ftranCountAfterR_=other.ftranCountAfterR_;
  ftranCountAfterU_=other.ftranCountAfterU_;
  btranCountInput_=other.btranCountInput_;
  btranCountAfterU_=other.btranCountAfterU_;
  btranCountAfterR_=other.btranCountAfterR_;
  btranCountAfterL_=other.btranCountAfterL_;
  numberFtranCounts_=other.numberFtranCounts_;
  numberBtranCounts_=other.numberBtranCounts_;
  ftranAverageAfterL_=other.ftranAverageAfterL_;
  ftranAverageAfterR_=other.ftranAverageAfterR_;
  ftranAverageAfterU_=other.ftranAverageAfterU_;
  btranAverageAfterU_=other.btranAverageAfterU_;
  btranAverageAfterR_=other.btranAverageAfterR_;
  btranAverageAfterL_=other.btranAverageAfterL_; 
  biasLU_=other.biasLU_;
  sparseThreshold_=other.sparseThreshold_;
  sparseThreshold2_=other.sparseThreshold2_;
  CoinBigIndex space = lengthAreaL_ - lengthL_;

  numberDense_ = other.numberDense_;
  denseThreshold_=other.denseThreshold_;
  if (numberDense_) {
    denseArea_ = new double [numberDense_*numberDense_];
    CoinMemcpyN(other.denseArea_,
	   numberDense_*numberDense_,denseArea_);
    densePermute_ = new int [numberDense_];
    CoinMemcpyN(other.densePermute_,
	   numberDense_,densePermute_);
  }

  lengthAreaR_ = space;
  elementR_ = elementL_.array() + lengthL_;
  indexRowR_ = indexRowL_.array() + lengthL_;
  workArea_ = other.workArea_;
  workArea2_ = other.workArea2_;
  //now copy everything
  //assuming numberRowsExtra==numberColumnsExtra
  if (numberRowsExtra_) {
    if (convertUOther) {
      CoinMemcpyN ( other.startRowU_.array(), numberRowsExtra_ + 1, startRowU_.array() );
      CoinMemcpyN ( other.numberInRow_.array(), numberRowsExtra_ + 1, numberInRow_.array() );
      startRowU_.array()[maximumRowsExtra_] = other.startRowU_.array()[maximumRowsExtra_];
    }
    CoinMemcpyN ( other.pivotRegion_.array(), numberRowsExtra_ , pivotRegion_.array() );
    CoinMemcpyN ( other.permuteBack_.array(), numberRowsExtra_ + 1, permuteBack_.array() );
    CoinMemcpyN ( other.permute_.array(), numberRowsExtra_ + 1, permute_.array() );
    CoinMemcpyN ( other.pivotColumnBack_.array(), numberRowsExtra_ + 1, pivotColumnBack_.array() );
    CoinMemcpyN ( other.firstCount_.array(), numberRowsExtra_ + 1, firstCount_.array() );
    CoinMemcpyN ( other.startColumnU_.array(), numberRowsExtra_ + 1, startColumnU_.array() );
    CoinMemcpyN ( other.numberInColumn_.array(), numberRowsExtra_ + 1, numberInColumn_.array() );
    CoinMemcpyN ( other.pivotColumn_.array(), numberRowsExtra_ + 1, pivotColumn_.array() );
    CoinMemcpyN ( other.nextColumn_.array(), numberRowsExtra_ + 1, nextColumn_.array() );
    CoinMemcpyN ( other.lastColumn_.array(), numberRowsExtra_ + 1, lastColumn_.array() );
    CoinMemcpyN ( other.startColumnR_.array() , numberRowsExtra_ - numberColumns_ + 1,
			startColumnR_.array() );  
    //extra one at end
    startColumnU_.array()[maximumColumnsExtra_] =
      other.startColumnU_.array()[maximumColumnsExtra_];
    nextColumn_.array()[maximumColumnsExtra_] = other.nextColumn_.array()[maximumColumnsExtra_];
    lastColumn_.array()[maximumColumnsExtra_] = other.lastColumn_.array()[maximumColumnsExtra_];
    CoinMemcpyN ( other.nextRow_.array(), numberRowsExtra_ + 1, nextRow_.array() );
    CoinMemcpyN ( other.lastRow_.array(), numberRowsExtra_ + 1, lastRow_.array() );
    nextRow_.array()[maximumRowsExtra_] = other.nextRow_.array()[maximumRowsExtra_];
    lastRow_.array()[maximumRowsExtra_] = other.lastRow_.array()[maximumRowsExtra_];
  }
  CoinMemcpyN ( other.elementR_, lengthR_, elementR_ );
  CoinMemcpyN ( other.indexRowR_, lengthR_, indexRowR_ );
  //row and column copies of U
  /* as elements of U may have been zeroed but column counts zero
     copy all elements */
  const CoinBigIndex * startColumnU = startColumnU_.array();
  const int * numberInColumn = numberInColumn_.array();
#ifndef NDEBUG
  int maxU=0;
  for (int iRow = 0; iRow < numberRowsExtra_; iRow++ ) {
    CoinBigIndex start = startColumnU[iRow];
    int numberIn = numberInColumn[iRow];
    maxU = CoinMax(maxU,start+numberIn);
  }
  assert (maximumU_>=maxU);
#endif
  CoinMemcpyN ( other.elementU_.array() , maximumU_, elementU_.array() );
#if COIN_ONE_ETA_COPY
  if (convertUOther) {
#endif
    const int * indexColumnUOther = other.indexColumnU_.array();
    CoinBigIndex * convertU = convertRowToColumnU_.array();
    int * indexColumnU = indexColumnU_.array();
    const CoinBigIndex * startRowU = startRowU_.array();
    const int * numberInRow = numberInRow_.array();
    for (int iRow = 0; iRow < numberRowsExtra_; iRow++ ) {
      //row
      CoinBigIndex start = startRowU[iRow];
      int numberIn = numberInRow[iRow];
      
      CoinMemcpyN ( indexColumnUOther + start, numberIn, indexColumnU + start );
      CoinMemcpyN (convertUOther + start , numberIn,   convertU + start );
    }
#if COIN_ONE_ETA_COPY
  }
#endif
  const int * indexRowUOther = other.indexRowU_.array();
  int * indexRowU = indexRowU_.array();
  for (int iRow = 0; iRow < numberRowsExtra_; iRow++ ) {
    //column
    CoinBigIndex start = startColumnU[iRow];
    int numberIn = numberInColumn[iRow];
    CoinMemcpyN ( indexRowUOther + start, numberIn, indexRowU + start );
  }
  // L is contiguous
  if (numberRows_)
    CoinMemcpyN ( other.startColumnL_.array(), numberRows_ + 1, startColumnL_.array() );
  CoinMemcpyN ( other.elementL_.array(), lengthL_, elementL_.array() );
  CoinMemcpyN ( other.indexRowL_.array(), lengthL_, indexRowL_.array() );
  if (other.sparseThreshold_) {
    goSparse();
  }
}
// See if worth going sparse
void 
CoinFactorization::checkSparse()
{
  // See if worth going sparse and when
  if (numberFtranCounts_>100) {
    ftranCountInput_= CoinMax(ftranCountInput_,1.0);
    ftranAverageAfterL_ = CoinMax(ftranCountAfterL_/ftranCountInput_,1.0);
    ftranAverageAfterR_ = CoinMax(ftranCountAfterR_/ftranCountAfterL_,1.0);
    ftranAverageAfterU_ = CoinMax(ftranCountAfterU_/ftranCountAfterR_,1.0);
    if (btranCountInput_&&btranCountAfterU_&&btranCountAfterR_) {
      btranAverageAfterU_ = CoinMax(btranCountAfterU_/btranCountInput_,1.0);
      btranAverageAfterR_ = CoinMax(btranCountAfterR_/btranCountAfterU_,1.0);
      btranAverageAfterL_ = CoinMax(btranCountAfterL_/btranCountAfterR_,1.0);
    } else {
      // we have not done any useful btrans (values pass?)
      btranAverageAfterU_ = 1.0;
      btranAverageAfterR_ = 1.0;
      btranAverageAfterL_ = 1.0;
    }
  }
  // scale back
  
  ftranCountInput_ *= 0.8;
  ftranCountAfterL_ *= 0.8;
  ftranCountAfterR_ *= 0.8;
  ftranCountAfterU_ *= 0.8;
  btranCountInput_ *= 0.8;
  btranCountAfterU_ *= 0.8;
  btranCountAfterR_ *= 0.8;
  btranCountAfterL_ *= 0.8;
}
// Condition number - product of pivots after factorization
double 
CoinFactorization::conditionNumber() const
{
  double condition = 1.0;
  const CoinFactorizationDouble * pivotRegion = pivotRegion_.array();
  for (int i=0;i<numberRows_;i++) {
    condition *= pivotRegion[i];
  }
  condition = CoinMax(fabs(condition),1.0e-50);
  return 1.0/condition;
}
#ifdef COIN_DEVELOP
extern double ncall_DZ;
extern double nrow_DZ;
extern double nslack_DZ;
extern double nU_DZ;
extern double nnz_DZ;
extern double nDone_DZ;
extern double ncall_SZ;
extern double nrow_SZ;
extern double nslack_SZ;
extern double nU_SZ;
extern double nnz_SZ;
extern double nDone_SZ;
void print_fac_stats()
{
  double mult = ncall_DZ ? 1.0/ncall_DZ : 1.0;
  printf("UDen called %g times, average rows %g, average slacks %g, average (U-S) %g average nnz in %g average ops %g\n",
	 ncall_DZ,mult*nrow_DZ,mult*nslack_DZ,mult*(nU_DZ-nslack_DZ),mult*nnz_DZ,mult*nDone_DZ);
  ncall_DZ=0.0;
  nrow_DZ=0.0;
  nslack_DZ=0.0;
  nU_DZ=0.0;
  nnz_DZ=0.0;
  nDone_DZ=0.0;
  mult = ncall_SZ ? 1.0/ncall_SZ : 1.0;
  printf("USpars called %g times, average rows %g, average slacks %g, average (U-S) %g average nnz in %g average ops %g\n",
	 ncall_SZ,mult*nrow_SZ,mult*nslack_SZ,mult*(nU_SZ-nslack_SZ),mult*nnz_SZ,mult*nDone_SZ);
  ncall_SZ=0.0;
  nrow_SZ=0.0;
  nslack_SZ=0.0;
  nU_SZ=0.0;
  nnz_SZ=0.0;
  nDone_SZ=0.0;
}
#endif
