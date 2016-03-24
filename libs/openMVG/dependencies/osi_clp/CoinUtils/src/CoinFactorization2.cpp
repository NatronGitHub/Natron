/* $Id: CoinFactorization2.cpp 1416 2011-04-17 09:57:29Z stefan $ */
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#if defined(_MSC_VER)
// Turn off compiler warning about long names
#  pragma warning(disable:4786)
#endif

#include "CoinUtilsConfig.h"

#include <cassert>
#include <cfloat>
#include <stdio.h>
#include "CoinFactorization.hpp"
#include "CoinIndexedVector.hpp"
#include "CoinHelperFunctions.hpp"
#include "CoinFinite.hpp"
#if DENSE_CODE==1
// using simple lapack interface
extern "C" 
{
  /** LAPACK Fortran subroutine DGETRF. */
  void F77_FUNC(dgetrf,DGETRF)(ipfint * m, ipfint *n,
                               double *A, ipfint *ldA,
                               ipfint * ipiv, ipfint *info);
}
#endif
#ifndef NDEBUG
static int counter1=0;
#endif
//  factorSparse.  Does sparse phase of factorization
//return code is <0 error, 0= finished
int
CoinFactorization::factorSparse (  )
{
  int larger;

  if ( numberRows_ < numberColumns_ ) {
    larger = numberColumns_;
  } else {
    larger = numberRows_;
  }
  int returnCode;
#define LARGELIMIT 65530
#define SMALL_SET 65531
#define SMALL_UNSET (SMALL_SET+1)
#define LARGE_SET COIN_INT_MAX-10
#define LARGE_UNSET (LARGE_SET+1)
  if ( larger < LARGELIMIT )
    returnCode = factorSparseSmall();
  else
    returnCode = factorSparseLarge();
  return returnCode;
}
//  factorSparse.  Does sparse phase of factorization
//return code is <0 error, 0= finished
int
CoinFactorization::factorSparseSmall (  )
{
  int *indexRow = indexRowU_.array();
  int *indexColumn = indexColumnU_.array();
  CoinFactorizationDouble *element = elementU_.array();
  int count = 1;
  workArea_.conditionalNew(numberRows_);
  CoinFactorizationDouble * workArea = workArea_.array();
#ifndef NDEBUG
  counter1++;
#endif
  // when to go dense
  int denseThreshold=denseThreshold_;

  CoinZeroN ( workArea, numberRows_ );
  //get space for bit work area
  CoinBigIndex workSize = 1000;
  workArea2_.conditionalNew(workSize);
  unsigned int * workArea2 = workArea2_.array();

  //set markRow so no rows updated
  unsigned short * markRow = reinterpret_cast<unsigned short *> (markRow_.array());
  CoinFillN (  markRow, numberRows_, static_cast<unsigned short> (SMALL_UNSET));
  int status = 0;
  //do slacks first
  int * numberInRow = numberInRow_.array();
  int * numberInColumn = numberInColumn_.array();
  int * numberInColumnPlus = numberInColumnPlus_.array();
  CoinBigIndex * startColumnU = startColumnU_.array();
  CoinBigIndex * startColumnL = startColumnL_.array();
  if (biasLU_<3&&numberColumns_==numberRows_) {
    int iPivotColumn;
    int * pivotColumn = pivotColumn_.array();
    int * nextRow = nextRow_.array();
    int * lastRow = lastRow_.array();
    for ( iPivotColumn = 0; iPivotColumn < numberColumns_;
	  iPivotColumn++ ) {
      if ( numberInColumn[iPivotColumn] == 1 ) {
	CoinBigIndex start = startColumnU[iPivotColumn];
	CoinFactorizationDouble value = element[start];
	if ( value == slackValue_ && numberInColumnPlus[iPivotColumn] == 0 ) {
	  // treat as slack
	  int iRow = indexRow[start];
	  // but only if row not marked
	  if (numberInRow[iRow]>0) {
	    totalElements_ -= numberInRow[iRow];
	    //take out this bit of indexColumnU
	    int next = nextRow[iRow];
	    int last = lastRow[iRow];
	    
	    nextRow[last] = next;
	    lastRow[next] = last;
	    nextRow[iRow] = numberGoodU_;	//use for permute
	    lastRow[iRow] = -2; //mark
	    //modify linked list for pivots
	    deleteLink ( iRow );
	    numberInRow[iRow]=-1;
	    numberInColumn[iPivotColumn]=0;
	    numberGoodL_++;
	    startColumnL[numberGoodL_] = 0;
	    pivotColumn[numberGoodU_] = iPivotColumn;
	    numberGoodU_++;
	  }
	}
      }
    }
    // redo
    preProcess(4);
    CoinFillN (  markRow, numberRows_, static_cast<unsigned short> (SMALL_UNSET));
  }
  numberSlacks_ = numberGoodU_;
  int *nextCount = nextCount_.array();
  int *firstCount = firstCount_.array();
  CoinBigIndex *startRow = startRowU_.array();
  CoinBigIndex *startColumn = startColumnU;
  //#define UGLY_COIN_FACTOR_CODING
#ifdef UGLY_COIN_FACTOR_CODING
  CoinFactorizationDouble *elementL = elementL_.array();
  int *indexRowL = indexRowL_.array();
  int *saveColumn = saveColumn_.array();
  int *nextRow = nextRow_.array();
  int *lastRow = lastRow_.array() ;
#endif
  double pivotTolerance = pivotTolerance_;
  int numberTrials = numberTrials_;
  int numberRows = numberRows_;
  // Put column singletons first - (if false)
  separateLinks(1,(biasLU_>1));
#ifndef NDEBUG
  int counter2=0;
#endif
  while ( count <= biggerDimension_ ) {
#ifndef NDEBUG
    counter2++;
    int badRow=-1;
    if (counter1==-1&&counter2>=0) {
      // check counts consistent
      for (int iCount=1;iCount<numberRows_;iCount++) {
        int look = firstCount[iCount];
        while ( look >= 0 ) {
          if ( look < numberRows_ ) {
            int iRow = look;
            if (iRow==badRow)
              printf("row count for row %d is %d\n",iCount,iRow);
            if ( numberInRow[iRow] != iCount ) {
              printf("failed debug on %d entry to factorSparse and %d try\n",
                     counter1,counter2);
              printf("row %d - count %d number %d\n",iRow,iCount,numberInRow[iRow]);
              abort();
            }
            look = nextCount[look];
          } else {
            int iColumn = look - numberRows;
            if ( numberInColumn[iColumn] != iCount ) {
              printf("failed debug on %d entry to factorSparse and %d try\n",
                     counter1,counter2);
              printf("column %d - count %d number %d\n",iColumn,iCount,numberInColumn[iColumn]);
              abort();
            }
            look = nextCount[look];
          }
        }
      }
    }
#endif
    CoinBigIndex minimumCount = COIN_INT_MAX;
    double minimumCost = COIN_DBL_MAX;

    int iPivotRow = -1;
    int iPivotColumn = -1;
    int pivotRowPosition = -1;
    int pivotColumnPosition = -1;
    int look = firstCount[count];
    int trials = 0;
    int * pivotColumn = pivotColumn_.array();

    if ( count == 1 && firstCount[1] >= 0 &&!biasLU_) {
      //do column singletons first to put more in U
      while ( look >= 0 ) {
        if ( look < numberRows_ ) {
          look = nextCount[look];
        } else {
          int iColumn = look - numberRows_;
          
          assert ( numberInColumn[iColumn] == count );
          CoinBigIndex start = startColumnU[iColumn];
          int iRow = indexRow[start];
          
          iPivotRow = iRow;
          pivotRowPosition = start;
          iPivotColumn = iColumn;
          assert (iPivotRow>=0&&iPivotColumn>=0);
          pivotColumnPosition = -1;
          look = -1;
          break;
        }
      }			/* endwhile */
      if ( iPivotRow < 0 ) {
        //back to singletons
        look = firstCount[1];
      }
    }
    while ( look >= 0 ) {
      if ( look < numberRows_ ) {
        int iRow = look;
#ifndef NDEBUG        
        if ( numberInRow[iRow] != count ) {
          printf("failed on %d entry to factorSparse and %d try\n",
                 counter1,counter2);
          printf("row %d - count %d number %d\n",iRow,count,numberInRow[iRow]);
          abort();
        }
#endif
        look = nextCount[look];
        bool rejected = false;
        CoinBigIndex start = startRow[iRow];
        CoinBigIndex end = start + count;
        
        CoinBigIndex i;
        for ( i = start; i < end; i++ ) {
          int iColumn = indexColumn[i];
          assert (numberInColumn[iColumn]>0);
          double cost = ( count - 1 ) * numberInColumn[iColumn];
          
          if ( cost < minimumCost ) {
            CoinBigIndex where = startColumn[iColumn];
            double minimumValue = element[where];
            
            minimumValue = fabs ( minimumValue ) * pivotTolerance;
            while ( indexRow[where] != iRow ) {
              where++;
            }			/* endwhile */
            assert ( where < startColumn[iColumn] +
                     numberInColumn[iColumn]);
            CoinFactorizationDouble value = element[where];
            
            value = fabs ( value );
            if ( value >= minimumValue ) {
              minimumCost = cost;
              minimumCount = numberInColumn[iColumn];
              iPivotRow = iRow;
              pivotRowPosition = -1;
              iPivotColumn = iColumn;
              assert (iPivotRow>=0&&iPivotColumn>=0);
              pivotColumnPosition = i;
              rejected=false;
              if ( minimumCount < count ) {
                look = -1;
                break;
              }
            } else if ( iPivotRow == -1 ) {
              rejected = true;
            }
          }
        }
        trials++;
        if ( trials >= numberTrials && iPivotRow >= 0 ) {
          look = -1;
          break;
        }
        if ( rejected ) {
          //take out for moment
          //eligible when row changes
          deleteLink ( iRow );
          addLink ( iRow, biggerDimension_ + 1 );
        }
      } else {
        int iColumn = look - numberRows;
        
        assert ( numberInColumn[iColumn] == count );
        look = nextCount[look];
        CoinBigIndex start = startColumn[iColumn];
        CoinBigIndex end = start + numberInColumn[iColumn];
        CoinFactorizationDouble minimumValue = element[start];
        
        minimumValue = fabs ( minimumValue ) * pivotTolerance;
        CoinBigIndex i;
        for ( i = start; i < end; i++ ) {
          CoinFactorizationDouble value = element[i];
          
          value = fabs ( value );
          if ( value >= minimumValue ) {
            int iRow = indexRow[i];
            int nInRow = numberInRow[iRow];
            assert (nInRow>0);
            double cost = ( count - 1 ) * nInRow;
            
            if ( cost < minimumCost ) {
              minimumCost = cost;
              minimumCount = nInRow;
              iPivotRow = iRow;
              pivotRowPosition = i;
              iPivotColumn = iColumn;
              assert (iPivotRow>=0&&iPivotColumn>=0);
              pivotColumnPosition = -1;
              if ( minimumCount <= count + 1 ) {
                look = -1;
                break;
              }
            }
          }
        }
        trials++;
        if ( trials >= numberTrials && iPivotRow >= 0 ) {
          look = -1;
          break;
        }
      }
    }				/* endwhile */
    if (iPivotRow>=0) {
      assert (iPivotRow<numberRows_);
      int numberDoRow = numberInRow[iPivotRow] - 1;
      int numberDoColumn = numberInColumn[iPivotColumn] - 1;
      
      totalElements_ -= ( numberDoRow + numberDoColumn + 1 );
      if ( numberDoColumn > 0 ) {
	if ( numberDoRow > 0 ) {
	  if ( numberDoColumn > 1 ) {
	    //  if (1) {
	    //need to adjust more for cache and SMP
	    //allow at least 4 extra
	    int increment = numberDoColumn + 1 + 4;
            
	    if ( increment & 15 ) {
	      increment = increment & ( ~15 );
	      increment += 16;
	    }
	    int increment2 =
	      
	      ( increment + COINFACTORIZATION_BITS_PER_INT - 1 ) >> COINFACTORIZATION_SHIFT_PER_INT;
	    CoinBigIndex size = increment2 * numberDoRow;
            
	    if ( size > workSize ) {
	      workSize = size;
	      workArea2_.conditionalNew(workSize);
	      workArea2 = workArea2_.array();
	    }
	    bool goodPivot;
#ifndef UGLY_COIN_FACTOR_CODING
	    //branch out to best pivot routine 
	    goodPivot = pivot ( iPivotRow, iPivotColumn,
				pivotRowPosition, pivotColumnPosition,
				workArea, workArea2, 
				increment2,  markRow ,
				SMALL_SET);
#else
#define FAC_SET SMALL_SET
#include "CoinFactorization.hpp"
#undef FAC_SET
#undef UGLY_COIN_FACTOR_CODING
#endif
	    if ( !goodPivot ) {
	      status = -99;
	      count=biggerDimension_+1;
	      break;
	    }
	  } else {
	    if ( !pivotOneOtherRow ( iPivotRow, iPivotColumn ) ) {
	      status = -99;
	      count=biggerDimension_+1;
	      break;
	    }
	  }
	} else {
	  assert (!numberDoRow);
	  if ( !pivotRowSingleton ( iPivotRow, iPivotColumn ) ) {
	    status = -99;
	    count=biggerDimension_+1;
	    break;
	  }
	}
      } else {
	assert (!numberDoColumn);
	if ( !pivotColumnSingleton ( iPivotRow, iPivotColumn ) ) {
	  status = -99;
	  count=biggerDimension_+1;
	  break;
	}
      }
      assert (nextRow_.array()[iPivotRow]==numberGoodU_);
      pivotColumn[numberGoodU_] = iPivotColumn;
      numberGoodU_++;
      // This should not need to be trapped here - but be safe
      if (numberGoodU_==numberRows_) 
	count=biggerDimension_+1;
#if COIN_DEBUG==2
      checkConsistency (  );
#endif
#if 0
      // Even if no dense code may be better to use naive dense
      if (!denseThreshold_&&totalElements_>1000) {
        int leftRows=numberRows_-numberGoodU_;
        double full = leftRows;
        full *= full;
        assert (full>=0.0);
        double leftElements = totalElements_;
        double ratio;
        if (leftRows>2000)
          ratio=4.0;
        else 
          ratio=3.0;
        if (ratio*leftElements>full) 
          denseThreshold=1;
      }
#endif
      if (denseThreshold) {
        // see whether to go dense 
        int leftRows=numberRows_-numberGoodU_;
        double full = leftRows;
        full *= full;
        assert (full>=0.0);
        double leftElements = totalElements_;
        //if (leftRows==100)
        //printf("at 100 %d elements\n",totalElements_);
        double ratio;
        if (leftRows>2000)
          ratio=4.0;
        else if (leftRows>800)
          ratio=3.0;
        else if (leftRows>256)
          ratio=2.0;
        else
          ratio=1.5;
        if ((ratio*leftElements>full&&leftRows>denseThreshold_)) {
          //return to do dense
          if (status!=0)
            break;
          int check=0;
          for (int iColumn=0;iColumn<numberColumns_;iColumn++) {
            if (numberInColumn[iColumn]) 
              check++;
          }
          if (check!=leftRows&&denseThreshold_) {
            //printf("** mismatch %d columns left, %d rows\n",check,leftRows);
            denseThreshold=0;
          } else {
            status=2;
            if ((messageLevel_&4)!=0) 
              std::cout<<"      Went dense at "<<leftRows<<" rows "<<
                totalElements_<<" "<<full<<" "<<leftElements<<std::endl;
            if (!denseThreshold_)
              denseThreshold_=-check; // say how many
            break;
          }
        }
      }
      // start at 1 again
      count = 1;
    } else {
      //end of this - onto next
      count++;
    } 
  }				/* endwhile */
  workArea_.conditionalDelete() ;
  workArea2_.conditionalDelete() ;
  return status;
}

//:method factorDense.  Does dense phase of factorization
//return code is <0 error, 0= finished
int CoinFactorization::factorDense()
{
  int status=0;
  numberDense_=numberRows_-numberGoodU_;
  if (sizeof(CoinBigIndex)==4&&numberDense_>=2<<15) {
    abort();
  } 
  CoinBigIndex full;
  if (denseThreshold_>0) 
    full = numberDense_*numberDense_;
  else
    full = - denseThreshold_*numberDense_;
  totalElements_=full;
  denseArea_= new double [full];
  CoinZeroN(denseArea_,full);
  densePermute_= new int [numberDense_];
  int * indexRowU = indexRowU_.array();
  //mark row lookup using lastRow
  int i;
  int * nextRow = nextRow_.array();
  int * lastRow = lastRow_.array();
  int * numberInColumn = numberInColumn_.array();
  int * numberInColumnPlus = numberInColumnPlus_.array();
  for (i=0;i<numberRows_;i++) {
    if (lastRow[i]>=0)
      lastRow[i]=0;
  }
  int * indexRow = indexRowU_.array();
  CoinFactorizationDouble * element = elementU_.array();
  int which=0;
  for (i=0;i<numberRows_;i++) {
    if (!lastRow[i]) {
      lastRow[i]=which;
      nextRow[i]=numberGoodU_+which;
      densePermute_[which]=i;
      which++;
    }
  } 
  //for L part
  CoinBigIndex * startColumnL = startColumnL_.array();
  CoinFactorizationDouble * elementL = elementL_.array();
  int * indexRowL = indexRowL_.array();
  CoinBigIndex endL=startColumnL[numberGoodL_];
  //take out of U
  double * column = denseArea_;
  int rowsDone=0;
  int iColumn=0;
  int * pivotColumn = pivotColumn_.array();
  CoinFactorizationDouble * pivotRegion = pivotRegion_.array();
  CoinBigIndex * startColumnU = startColumnU_.array();
  for (iColumn=0;iColumn<numberColumns_;iColumn++) {
    if (numberInColumn[iColumn]) {
      //move
      CoinBigIndex start = startColumnU[iColumn];
      int number = numberInColumn[iColumn];
      CoinBigIndex end = start+number;
      for (CoinBigIndex i=start;i<end;i++) {
        int iRow=indexRow[i];
        iRow=lastRow[iRow];
	assert (iRow>=0&&iRow<numberDense_);
        column[iRow]=element[i];
      } /* endfor */
      column+=numberDense_;
      while (lastRow[rowsDone]<0) {
        rowsDone++;
      } /* endwhile */
      nextRow[rowsDone]=numberGoodU_;
      rowsDone++;
      startColumnL[numberGoodU_+1]=endL;
      numberInColumn[iColumn]=0;
      pivotColumn[numberGoodU_]=iColumn;
      pivotRegion[numberGoodU_]=1.0;
      numberGoodU_++;
    } 
  } 
#ifdef DENSE_CODE
  if (denseThreshold_>0) {
    assert(numberGoodU_==numberRows_);
    numberGoodL_=numberRows_;
    //now factorize
    //dgef(denseArea_,&numberDense_,&numberDense_,densePermute_);
    int info;
    F77_FUNC(dgetrf,DGETRF)(&numberDense_,&numberDense_,denseArea_,&numberDense_,densePermute_,
			    &info);
    // need to check size of pivots
    if(info)
      status = -1;
    return status;
  } 
#endif
  numberGoodU_ = numberRows_-numberDense_;
  int base = numberGoodU_;
  int iDense;
  int numberToDo=-denseThreshold_;
  denseThreshold_=0;
  double tolerance = zeroTolerance_;
  tolerance = 1.0e-30;
  int * nextColumn = nextColumn_.array();
  const int * pivotColumnConst = pivotColumn_.array();
  // make sure we have enough space in L and U
  for (iDense=0;iDense<numberToDo;iDense++) {
    //how much space have we got
    iColumn = pivotColumnConst[base+iDense];
    int next = nextColumn[iColumn];
    int numberInPivotColumn = iDense;
    CoinBigIndex space = startColumnU[next] 
      - startColumnU[iColumn]
      - numberInColumnPlus[next];
    //assume no zero elements
    if ( numberInPivotColumn > space ) {
      //getColumnSpace also moves fixed part
      if ( !getColumnSpace ( iColumn, numberInPivotColumn ) ) {
	return -99;
      }
    }
    // set so further moves will work
    numberInColumn[iColumn]=numberInPivotColumn;
  }
  // Fill in ?
  for (iColumn=numberGoodU_+numberToDo;iColumn<numberRows_;iColumn++) {
    nextRow[iColumn]=iColumn;
    startColumnL[iColumn+1]=endL;
    pivotRegion[iColumn]=1.0;
  } 
  if ( lengthL_ + full*0.5 > lengthAreaL_ ) {
    //need more memory
    if ((messageLevel_&4)!=0) 
      std::cout << "more memory needed in middle of invert" << std::endl;
    return -99;
  }
  CoinFactorizationDouble *elementU = elementU_.array();
  for (iDense=0;iDense<numberToDo;iDense++) {
    int iRow;
    int jDense;
    int pivotRow=-1;
    double * element = denseArea_+iDense*numberDense_;
    CoinFactorizationDouble largest = 1.0e-12;
    for (iRow=iDense;iRow<numberDense_;iRow++) {
      if (fabs(element[iRow])>largest) {
	largest = fabs(element[iRow]);
	pivotRow = iRow;
      }
    }
    if ( pivotRow >= 0 ) {
      iColumn = pivotColumnConst[base+iDense];
      CoinFactorizationDouble pivotElement=element[pivotRow];
      // get original row
      int originalRow = densePermute_[pivotRow];
      // do nextRow
      nextRow[originalRow] = numberGoodU_;
      lastRow[originalRow] = -2; //mark
      // swap
      densePermute_[pivotRow]=densePermute_[iDense];
      densePermute_[iDense] = originalRow;
      for (jDense=iDense;jDense<numberDense_;jDense++) {
	CoinFactorizationDouble value = element[iDense];
	element[iDense] = element[pivotRow];
	element[pivotRow] = value;
	element += numberDense_;
      }
      CoinFactorizationDouble pivotMultiplier = 1.0 / pivotElement;
      //printf("pivotMultiplier %g\n",pivotMultiplier);
      pivotRegion[numberGoodU_] = pivotMultiplier;
      // Do L
      element = denseArea_+iDense*numberDense_;
      CoinBigIndex l = lengthL_;
      startColumnL[numberGoodL_] = l;	//for luck and first time
      for (iRow=iDense+1;iRow<numberDense_;iRow++) {
	CoinFactorizationDouble value = element[iRow]*pivotMultiplier;
	element[iRow] = value;
	if (fabs(value)>tolerance) {
	  indexRowL[l] = densePermute_[iRow];
	  elementL[l++] = value;
	}
      }
      numberGoodL_++;
      lengthL_ = l;
      startColumnL[numberGoodL_] = l;
      // update U column
      CoinBigIndex start = startColumnU[iColumn];
      for (iRow=0;iRow<iDense;iRow++) {
	if (fabs(element[iRow])>tolerance) {
	  indexRowU[start] = densePermute_[iRow];
	  elementU[start++] = element[iRow];
	}
      }
      numberInColumn[iColumn]=0;
      numberInColumnPlus[iColumn] += start-startColumnU[iColumn];
      startColumnU[iColumn]=start;
      // update other columns
      double * element2 = element+numberDense_;
      for (jDense=iDense+1;jDense<numberToDo;jDense++) {
	CoinFactorizationDouble value = element2[iDense];
	for (iRow=iDense+1;iRow<numberDense_;iRow++) {
	  //double oldValue=element2[iRow];
	  element2[iRow] -= value*element[iRow];
	  //if (oldValue&&!element2[iRow]) {
          //printf("Updated element for column %d, row %d old %g",
          //   pivotColumnConst[base+jDense],densePermute_[iRow],oldValue);
          //printf(" new %g\n",element2[iRow]);
	  //}
	}
	element2 += numberDense_;
      }
      numberGoodU_++;
    } else {
      return -1;
    }
  }
  // free area (could use L?)
  delete [] denseArea_;
  denseArea_ = NULL;
  // check if can use another array for densePermute_
  delete [] densePermute_;
  densePermute_ = NULL;
  numberDense_=0;
  return status;
}
// Separate out links with same row/column count
void 
CoinFactorization::separateLinks(int count,bool rowsFirst)
{
  int *nextCount = nextCount_.array();
  int *firstCount = firstCount_.array();
  int *lastCount = lastCount_.array();
  int next = firstCount[count];
  int firstRow=-1;
  int firstColumn=-1;
  int lastRow=-1;
  int lastColumn=-1;
  while(next>=0) {
    int next2=nextCount[next];
    if (next>=numberRows_) {
      nextCount[next]=-1;
      // Column
      if (firstColumn>=0) {
	lastCount[next]=lastColumn;
	nextCount[lastColumn]=next;
      } else {
	lastCount[next]= -2 - count;
	firstColumn=next;
      }
      lastColumn=next;
    } else {
      // Row
      if (firstRow>=0) {
	lastCount[next]=lastRow;
	nextCount[lastRow]=next;
      } else {
	lastCount[next]= -2 - count;
	firstRow=next;
      }
      lastRow=next;
    }
    next=next2;
  }
  if (rowsFirst&&firstRow>=0) {
    firstCount[count]=firstRow;
    nextCount[lastRow]=firstColumn;
    if (firstColumn>=0)
      lastCount[firstColumn]=lastRow;
  } else if (firstRow<0) {
    firstCount[count]=firstColumn;
  } else if (firstColumn>=0) {
    firstCount[count]=firstColumn;
    nextCount[lastColumn]=firstRow;
    if (firstRow>=0)
      lastCount[firstRow]=lastColumn;
  } 
}
// Debug - save on file
int
CoinFactorization::saveFactorization (const char * file  ) const
{
  FILE * fp = fopen(file,"wb");
  if (fp) {
    // Save so we can pick up scalars
    const char * first = reinterpret_cast<const char *> ( &pivotTolerance_);
    const char * last = reinterpret_cast<const char *> ( &biasLU_);
    // increment
    last += sizeof(int);
    if (fwrite(first,last-first,1,fp)!=1)
      return 1;
    // Now arrays
    if (CoinToFile(elementU_.array(),lengthAreaU_ , fp ))
      return 1;
    if (CoinToFile(indexRowU_.array(),lengthAreaU_ , fp ))
      return 1;
    if (CoinToFile(indexColumnU_.array(),lengthAreaU_ , fp ))
      return 1;
    if (CoinToFile(convertRowToColumnU_.array(),lengthAreaU_ , fp ))
      return 1;
    if (CoinToFile(elementByRowL_.array(),lengthAreaL_ , fp ))
      return 1;
    if (CoinToFile(indexColumnL_.array(),lengthAreaL_ , fp ))
      return 1;
    if (CoinToFile(startRowL_.array() , numberRows_+1, fp ))
      return 1;
    if (CoinToFile(elementL_.array(),lengthAreaL_ , fp ))
      return 1;
    if (CoinToFile(indexRowL_.array(),lengthAreaL_ , fp ))
      return 1;
    if (CoinToFile(startColumnL_.array(),numberRows_ + 1 , fp ))
      return 1;
    if (CoinToFile(markRow_.array(),numberRows_  , fp))
      return 1;
    if (CoinToFile(saveColumn_.array(),numberColumns_  , fp))
      return 1;
    if (CoinToFile(startColumnR_.array() , maximumPivots_ + 1 , fp ))
      return 1;
    if (CoinToFile(startRowU_.array(),maximumRowsExtra_ + 1 , fp ))
      return 1;
    if (CoinToFile(numberInRow_.array(),maximumRowsExtra_ + 1 , fp ))
      return 1;
    if (CoinToFile(nextRow_.array(),maximumRowsExtra_ + 1 , fp ))
      return 1;
    if (CoinToFile(lastRow_.array(),maximumRowsExtra_ + 1 , fp ))
      return 1;
    if (CoinToFile(pivotRegion_.array(),maximumRowsExtra_ + 1 , fp ))
      return 1;
    if (CoinToFile(permuteBack_.array(),maximumRowsExtra_ + 1 , fp ))
      return 1;
    if (CoinToFile(permute_.array(),maximumRowsExtra_ + 1 , fp ))
      return 1;
    if (CoinToFile(pivotColumnBack_.array(),maximumRowsExtra_ + 1 , fp ))
      return 1;
    if (CoinToFile(startColumnU_.array(),maximumColumnsExtra_ + 1 , fp ))
      return 1;
    if (CoinToFile(numberInColumn_.array(),maximumColumnsExtra_ + 1 , fp ))
      return 1;
    if (CoinToFile(numberInColumnPlus_.array(),maximumColumnsExtra_ + 1 , fp ))
      return 1;
    if (CoinToFile(firstCount_.array(),biggerDimension_ + 2 , fp ))
      return 1;
    if (CoinToFile(nextCount_.array(),numberRows_ + numberColumns_ , fp ))
      return 1;
    if (CoinToFile(lastCount_.array(),numberRows_ + numberColumns_ , fp ))
      return 1;
    if (CoinToFile(pivotRowL_.array(),numberRows_ + 1 , fp ))
      return 1;
    if (CoinToFile(pivotColumn_.array(),maximumColumnsExtra_ + 1 , fp ))
      return 1;
    if (CoinToFile(nextColumn_.array(),maximumColumnsExtra_ + 1 , fp ))
      return 1;
    if (CoinToFile(lastColumn_.array(),maximumColumnsExtra_ + 1 , fp ))
      return 1;
    if (CoinToFile(denseArea_ , numberDense_*numberDense_, fp ))
      return 1;
    if (CoinToFile(densePermute_ , numberDense_, fp ))
      return 1;
    fclose(fp);
  }
  return 0;
}
// Debug - restore from file
int 
CoinFactorization::restoreFactorization (const char * file , bool factorIt ) 
{
  FILE * fp = fopen(file,"rb");
  if (fp) {
    // Get rid of current
    gutsOfDestructor();
    CoinBigIndex newSize=0; // for checking - should be same
    // Restore so we can pick up scalars
    char * first = reinterpret_cast<char *> ( &pivotTolerance_);
    char * last = reinterpret_cast<char *> ( &biasLU_);
    // increment
    last += sizeof(int);
    if (fread(first,last-first,1,fp)!=1)
      return 1;
    CoinBigIndex space = lengthAreaL_ - lengthL_;
    // Now arrays
    CoinFactorizationDouble *elementU = elementU_.array();
    if (CoinFromFile(elementU,lengthAreaU_ , fp, newSize )==1)
      return 1;
    assert (newSize==lengthAreaU_);
    int * indexRowU = indexRowU_.array();
    if (CoinFromFile(indexRowU,lengthAreaU_ , fp, newSize )==1)
      return 1;
    assert (newSize==lengthAreaU_);
    int * indexColumnU = indexColumnU_.array();
    if (CoinFromFile(indexColumnU,lengthAreaU_ , fp, newSize )==1)
      return 1;
    assert (newSize==lengthAreaU_);
    CoinBigIndex *convertRowToColumnU = convertRowToColumnU_.array();
    if (CoinFromFile(convertRowToColumnU,lengthAreaU_ , fp, newSize )==1)
      return 1;
    assert (newSize==lengthAreaU_||(newSize==0&&!convertRowToColumnU_.array()));
    CoinFactorizationDouble * elementByRowL = elementByRowL_.array();
    if (CoinFromFile(elementByRowL,lengthAreaL_ , fp, newSize )==1)
      return 1;
    assert (newSize==lengthAreaL_||(newSize==0&&!elementByRowL_.array()));
    int * indexColumnL = indexColumnL_.array();
    if (CoinFromFile(indexColumnL,lengthAreaL_ , fp, newSize )==1)
      return 1;
    assert (newSize==lengthAreaL_||(newSize==0&&!indexColumnL_.array()));
    CoinBigIndex * startRowL = startRowL_.array();
    if (CoinFromFile(startRowL , numberRows_+1, fp, newSize )==1)
      return 1;
    assert (newSize==numberRows_+1||(newSize==0&&!startRowL_.array()));
    CoinFactorizationDouble * elementL = elementL_.array();
    if (CoinFromFile(elementL,lengthAreaL_ , fp, newSize )==1)
      return 1;
    assert (newSize==lengthAreaL_);
    int * indexRowL = indexRowL_.array();
    if (CoinFromFile(indexRowL,lengthAreaL_ , fp, newSize )==1)
      return 1;
    assert (newSize==lengthAreaL_);
    CoinBigIndex * startColumnL = startColumnL_.array();
    if (CoinFromFile(startColumnL,numberRows_ + 1 , fp, newSize )==1)
      return 1;
    assert (newSize==numberRows_+1);
    int * markRow = markRow_.array();
    if (CoinFromFile(markRow,numberRows_  , fp, newSize )==1)
      return 1;
    assert (newSize==numberRows_);
    int * saveColumn = saveColumn_.array();
    if (CoinFromFile(saveColumn,numberColumns_  , fp, newSize )==1)
      return 1;
    assert (newSize==numberColumns_);
    CoinBigIndex * startColumnR = startColumnR_.array();
    if (CoinFromFile(startColumnR , maximumPivots_ + 1 , fp, newSize )==1)
      return 1;
    assert (newSize==maximumPivots_+1||(newSize==0&&!startColumnR_.array()));
    CoinBigIndex * startRowU = startRowU_.array();
    if (CoinFromFile(startRowU,maximumRowsExtra_ + 1 , fp, newSize )==1)
      return 1;
    assert (newSize==maximumRowsExtra_+1||(newSize==0&&!startRowU_.array()));
    int * numberInRow = numberInRow_.array();
    if (CoinFromFile(numberInRow,maximumRowsExtra_ + 1 , fp, newSize )==1)
      return 1;
    assert (newSize==maximumRowsExtra_+1);
    int * nextRow = nextRow_.array();
    if (CoinFromFile(nextRow,maximumRowsExtra_ + 1 , fp, newSize )==1)
      return 1;
    assert (newSize==maximumRowsExtra_+1);
    int * lastRow = lastRow_.array();
    if (CoinFromFile(lastRow,maximumRowsExtra_ + 1 , fp, newSize )==1)
      return 1;
    assert (newSize==maximumRowsExtra_+1);
    CoinFactorizationDouble * pivotRegion = pivotRegion_.array();
    if (CoinFromFile(pivotRegion,maximumRowsExtra_ + 1 , fp, newSize )==1)
      return 1;
    assert (newSize==maximumRowsExtra_+1);
    int * permuteBack = permuteBack_.array();
    if (CoinFromFile(permuteBack,maximumRowsExtra_ + 1 , fp, newSize )==1)
      return 1;
    assert (newSize==maximumRowsExtra_+1||(newSize==0&&!permuteBack_.array()));
    int * permute = permute_.array();
    if (CoinFromFile(permute,maximumRowsExtra_ + 1 , fp, newSize )==1)
      return 1;
    assert (newSize==maximumRowsExtra_+1||(newSize==0&&!permute_.array()));
    int * pivotColumnBack = pivotColumnBack_.array();
    if (CoinFromFile(pivotColumnBack,maximumRowsExtra_ + 1 , fp, newSize )==1)
      return 1;
    assert (newSize==maximumRowsExtra_+1||(newSize==0&&!pivotColumnBack_.array()));
    CoinBigIndex * startColumnU = startColumnU_.array();
    if (CoinFromFile(startColumnU,maximumColumnsExtra_ + 1 , fp, newSize )==1)
      return 1;
    assert (newSize==maximumColumnsExtra_+1);
    int * numberInColumn = numberInColumn_.array();
    if (CoinFromFile(numberInColumn,maximumColumnsExtra_ + 1 , fp, newSize )==1)
      return 1;
    assert (newSize==maximumColumnsExtra_+1);
    int * numberInColumnPlus = numberInColumnPlus_.array();
    if (CoinFromFile(numberInColumnPlus,maximumColumnsExtra_ + 1 , fp, newSize )==1)
      return 1;
    assert (newSize==maximumColumnsExtra_+1);
    int *firstCount = firstCount_.array();
    if (CoinFromFile(firstCount,biggerDimension_ + 2 , fp, newSize )==1)
      return 1;
    assert (newSize==biggerDimension_+2);
    int *nextCount = nextCount_.array();
    if (CoinFromFile(nextCount,numberRows_ + numberColumns_ , fp, newSize )==1)
      return 1;
    assert (newSize==numberRows_+numberColumns_);
    int *lastCount = lastCount_.array();
    if (CoinFromFile(lastCount,numberRows_ + numberColumns_ , fp, newSize )==1)
      return 1;
    assert (newSize==numberRows_+numberColumns_);
    int * pivotRowL = pivotRowL_.array();
    if (CoinFromFile(pivotRowL,numberRows_ + 1 , fp, newSize )==1)
      return 1;
    assert (newSize==numberRows_+1);
    int * pivotColumn = pivotColumn_.array(); 
    if (CoinFromFile(pivotColumn,maximumColumnsExtra_ + 1 , fp, newSize )==1)
      return 1;
    assert (newSize==maximumColumnsExtra_+1);
    int * nextColumn = nextColumn_.array();
    if (CoinFromFile(nextColumn,maximumColumnsExtra_ + 1 , fp, newSize )==1)
      return 1;
    assert (newSize==maximumColumnsExtra_+1);
    int * lastColumn = lastColumn_.array();
    if (CoinFromFile(lastColumn,maximumColumnsExtra_ + 1 , fp, newSize )==1)
      return 1;
    assert (newSize==maximumColumnsExtra_+1);
    if (CoinFromFile(denseArea_ , numberDense_*numberDense_, fp, newSize )==1)
      return 1;
    assert (newSize==numberDense_*numberDense_);
    if (CoinFromFile(densePermute_ , numberDense_, fp, newSize )==1)
      return 1;
    assert (newSize==numberDense_);
    lengthAreaR_ = space;
    elementR_ = elementL_.array() + lengthL_;
    indexRowR_ = indexRowL_.array() + lengthL_;
    fclose(fp);
    if (factorIt) {
      if (biasLU_>=3||numberRows_!=numberColumns_)
        preProcess ( 2 );
      else
        preProcess ( 3 ); // no row copy
      factor (  );
    }
  }
  return 0;
}
//  factorSparse.  Does sparse phase of factorization
//return code is <0 error, 0= finished
int
CoinFactorization::factorSparseLarge (  )
{
  int *indexRow = indexRowU_.array();
  int *indexColumn = indexColumnU_.array();
  CoinFactorizationDouble *element = elementU_.array();
  int count = 1;
  workArea_.conditionalNew(numberRows_);
  CoinFactorizationDouble * workArea = workArea_.array();
#ifndef NDEBUG
  counter1++;
#endif
  // when to go dense
  int denseThreshold=denseThreshold_;

  CoinZeroN ( workArea, numberRows_ );
  //get space for bit work area
  CoinBigIndex workSize = 1000;
  workArea2_.conditionalNew(workSize);
  unsigned int * workArea2 = workArea2_.array();

  //set markRow so no rows updated
  int * markRow = markRow_.array();
  CoinFillN ( markRow, numberRows_, COIN_INT_MAX-10+1);
  int status = 0;
  //do slacks first
  int * numberInRow = numberInRow_.array();
  int * numberInColumn = numberInColumn_.array();
  int * numberInColumnPlus = numberInColumnPlus_.array();
  CoinBigIndex * startColumnU = startColumnU_.array();
  CoinBigIndex * startColumnL = startColumnL_.array();
  if (biasLU_<3&&numberColumns_==numberRows_) {
    int iPivotColumn;
    int * pivotColumn = pivotColumn_.array();
    int * nextRow = nextRow_.array();
    int * lastRow = lastRow_.array();
    for ( iPivotColumn = 0; iPivotColumn < numberColumns_;
	  iPivotColumn++ ) {
      if ( numberInColumn[iPivotColumn] == 1 ) {
	CoinBigIndex start = startColumnU[iPivotColumn];
	CoinFactorizationDouble value = element[start];
	if ( value == slackValue_ && numberInColumnPlus[iPivotColumn] == 0 ) {
	  // treat as slack
	  int iRow = indexRow[start];
	  // but only if row not marked
	  if (numberInRow[iRow]>0) {
	    totalElements_ -= numberInRow[iRow];
	    //take out this bit of indexColumnU
	    int next = nextRow[iRow];
	    int last = lastRow[iRow];
	    
	    nextRow[last] = next;
	    lastRow[next] = last;
	    nextRow[iRow] = numberGoodU_;	//use for permute
	    lastRow[iRow] = -2; //mark
	    //modify linked list for pivots
	    deleteLink ( iRow );
	    numberInRow[iRow]=-1;
	    numberInColumn[iPivotColumn]=0;
	    numberGoodL_++;
	    startColumnL[numberGoodL_] = 0;
	    pivotColumn[numberGoodU_] = iPivotColumn;
	    numberGoodU_++;
	  }
	}
      }
    }
    // redo
    preProcess(4);
    CoinFillN ( markRow, numberRows_, COIN_INT_MAX-10+1);
  }
  numberSlacks_ = numberGoodU_;
  int *nextCount = nextCount_.array();
  int *firstCount = firstCount_.array();
  CoinBigIndex *startRow = startRowU_.array();
  CoinBigIndex *startColumn = startColumnU;
  //double *elementL = elementL_.array();
  //int *indexRowL = indexRowL_.array();
  //int *saveColumn = saveColumn_.array();
  //int *nextRow = nextRow_.array();
  //int *lastRow = lastRow_.array() ;
  double pivotTolerance = pivotTolerance_;
  int numberTrials = numberTrials_;
  int numberRows = numberRows_;
  // Put column singletons first - (if false)
  separateLinks(1,(biasLU_>1));
#ifndef NDEBUG
  int counter2=0;
#endif
  while ( count <= biggerDimension_ ) {
#ifndef NDEBUG
    counter2++;
    int badRow=-1;
    if (counter1==-1&&counter2>=0) {
      // check counts consistent
      for (int iCount=1;iCount<numberRows_;iCount++) {
        int look = firstCount[iCount];
        while ( look >= 0 ) {
          if ( look < numberRows_ ) {
            int iRow = look;
            if (iRow==badRow)
              printf("row count for row %d is %d\n",iCount,iRow);
            if ( numberInRow[iRow] != iCount ) {
              printf("failed debug on %d entry to factorSparse and %d try\n",
                     counter1,counter2);
              printf("row %d - count %d number %d\n",iRow,iCount,numberInRow[iRow]);
              abort();
            }
            look = nextCount[look];
          } else {
            int iColumn = look - numberRows;
            if ( numberInColumn[iColumn] != iCount ) {
              printf("failed debug on %d entry to factorSparse and %d try\n",
                     counter1,counter2);
              printf("column %d - count %d number %d\n",iColumn,iCount,numberInColumn[iColumn]);
              abort();
            }
            look = nextCount[look];
          }
        }
      }
    }
#endif
    CoinBigIndex minimumCount = COIN_INT_MAX;
    double minimumCost = COIN_DBL_MAX;

    int iPivotRow = -1;
    int iPivotColumn = -1;
    int pivotRowPosition = -1;
    int pivotColumnPosition = -1;
    int look = firstCount[count];
    int trials = 0;
    int * pivotColumn = pivotColumn_.array();

    if ( count == 1 && firstCount[1] >= 0 &&!biasLU_) {
      //do column singletons first to put more in U
      while ( look >= 0 ) {
        if ( look < numberRows_ ) {
          look = nextCount[look];
        } else {
          int iColumn = look - numberRows_;
          
          assert ( numberInColumn[iColumn] == count );
          CoinBigIndex start = startColumnU[iColumn];
          int iRow = indexRow[start];
          
          iPivotRow = iRow;
          pivotRowPosition = start;
          iPivotColumn = iColumn;
          assert (iPivotRow>=0&&iPivotColumn>=0);
          pivotColumnPosition = -1;
          look = -1;
          break;
        }
      }			/* endwhile */
      if ( iPivotRow < 0 ) {
        //back to singletons
        look = firstCount[1];
      }
    }
    while ( look >= 0 ) {
      if ( look < numberRows_ ) {
        int iRow = look;
#ifndef NDEBUG        
        if ( numberInRow[iRow] != count ) {
          printf("failed on %d entry to factorSparse and %d try\n",
                 counter1,counter2);
          printf("row %d - count %d number %d\n",iRow,count,numberInRow[iRow]);
          abort();
        }
#endif
        look = nextCount[look];
        bool rejected = false;
        CoinBigIndex start = startRow[iRow];
        CoinBigIndex end = start + count;
        
        CoinBigIndex i;
        for ( i = start; i < end; i++ ) {
          int iColumn = indexColumn[i];
          assert (numberInColumn[iColumn]>0);
          double cost = ( count - 1 ) * numberInColumn[iColumn];
          
          if ( cost < minimumCost ) {
            CoinBigIndex where = startColumn[iColumn];
            CoinFactorizationDouble minimumValue = element[where];
            
            minimumValue = fabs ( minimumValue ) * pivotTolerance;
            while ( indexRow[where] != iRow ) {
              where++;
            }			/* endwhile */
            assert ( where < startColumn[iColumn] +
                     numberInColumn[iColumn]);
            CoinFactorizationDouble value = element[where];
            
            value = fabs ( value );
            if ( value >= minimumValue ) {
              minimumCost = cost;
              minimumCount = numberInColumn[iColumn];
              iPivotRow = iRow;
              pivotRowPosition = -1;
              iPivotColumn = iColumn;
              assert (iPivotRow>=0&&iPivotColumn>=0);
              pivotColumnPosition = i;
              rejected=false;
              if ( minimumCount < count ) {
                look = -1;
                break;
              }
            } else if ( iPivotRow == -1 ) {
              rejected = true;
            }
          }
        }
        trials++;
        if ( trials >= numberTrials && iPivotRow >= 0 ) {
          look = -1;
          break;
        }
        if ( rejected ) {
          //take out for moment
          //eligible when row changes
          deleteLink ( iRow );
          addLink ( iRow, biggerDimension_ + 1 );
        }
      } else {
        int iColumn = look - numberRows;
        
        assert ( numberInColumn[iColumn] == count );
        look = nextCount[look];
        CoinBigIndex start = startColumn[iColumn];
        CoinBigIndex end = start + numberInColumn[iColumn];
        CoinFactorizationDouble minimumValue = element[start];
        
        minimumValue = fabs ( minimumValue ) * pivotTolerance;
        CoinBigIndex i;
        for ( i = start; i < end; i++ ) {
          CoinFactorizationDouble value = element[i];
          
          value = fabs ( value );
          if ( value >= minimumValue ) {
            int iRow = indexRow[i];
            int nInRow = numberInRow[iRow];
            assert (nInRow>0);
            double cost = ( count - 1 ) * nInRow;
            
            if ( cost < minimumCost ) {
              minimumCost = cost;
              minimumCount = nInRow;
              iPivotRow = iRow;
              pivotRowPosition = i;
              iPivotColumn = iColumn;
              assert (iPivotRow>=0&&iPivotColumn>=0);
              pivotColumnPosition = -1;
              if ( minimumCount <= count + 1 ) {
                look = -1;
                break;
              }
            }
          }
        }
        trials++;
        if ( trials >= numberTrials && iPivotRow >= 0 ) {
          look = -1;
          break;
        }
      }
    }				/* endwhile */
    if (iPivotRow>=0) {
      if ( iPivotRow >= 0 ) {
        int numberDoRow = numberInRow[iPivotRow] - 1;
        int numberDoColumn = numberInColumn[iPivotColumn] - 1;
        
        totalElements_ -= ( numberDoRow + numberDoColumn + 1 );
        if ( numberDoColumn > 0 ) {
          if ( numberDoRow > 0 ) {
            if ( numberDoColumn > 1 ) {
              //  if (1) {
              //need to adjust more for cache and SMP
              //allow at least 4 extra
              int increment = numberDoColumn + 1 + 4;
              
              if ( increment & 15 ) {
                increment = increment & ( ~15 );
                increment += 16;
              }
              int increment2 =
                
                ( increment + COINFACTORIZATION_BITS_PER_INT - 1 ) >> COINFACTORIZATION_SHIFT_PER_INT;
              CoinBigIndex size = increment2 * numberDoRow;
              
              if ( size > workSize ) {
                workSize = size;
		workArea2_.conditionalNew(workSize);
		workArea2 = workArea2_.array();
              }
              bool goodPivot;
              
	      //might be able to do better by permuting
#ifndef UGLY_COIN_FACTOR_CODING
	      //branch out to best pivot routine 
	      goodPivot = pivot ( iPivotRow, iPivotColumn,
				  pivotRowPosition, pivotColumnPosition,
				  workArea, workArea2, 
				  increment2, markRow ,
				  LARGE_SET);
#else
#define FAC_SET LARGE_SET
#include "CoinFactorization.hpp"
#undef FAC_SET
#endif
              if ( !goodPivot ) {
                status = -99;
                count=biggerDimension_+1;
                break;
              }
            } else {
              if ( !pivotOneOtherRow ( iPivotRow, iPivotColumn ) ) {
                status = -99;
                count=biggerDimension_+1;
                break;
              }
            }
          } else {
            assert (!numberDoRow);
            if ( !pivotRowSingleton ( iPivotRow, iPivotColumn ) ) {
              status = -99;
              count=biggerDimension_+1;
              break;
            }
          }
        } else {
          assert (!numberDoColumn);
          if ( !pivotColumnSingleton ( iPivotRow, iPivotColumn ) ) {
            status = -99;
            count=biggerDimension_+1;
            break;
          }
        }
	assert (nextRow_.array()[iPivotRow]==numberGoodU_);
        pivotColumn[numberGoodU_] = iPivotColumn;
        numberGoodU_++;
        // This should not need to be trapped here - but be safe
        if (numberGoodU_==numberRows_) 
          count=biggerDimension_+1;
      }
#if COIN_DEBUG==2
      checkConsistency (  );
#endif
#if 0
      // Even if no dense code may be better to use naive dense
      if (!denseThreshold_&&totalElements_>1000) {
        int leftRows=numberRows_-numberGoodU_;
        double full = leftRows;
        full *= full;
        assert (full>=0.0);
        double leftElements = totalElements_;
        double ratio;
        if (leftRows>2000)
          ratio=4.0;
        else 
          ratio=3.0;
        if (ratio*leftElements>full) 
          denseThreshold=1;
      }
#endif
      if (denseThreshold) {
        // see whether to go dense 
        int leftRows=numberRows_-numberGoodU_;
        double full = leftRows;
        full *= full;
        assert (full>=0.0);
        double leftElements = totalElements_;
        //if (leftRows==100)
        //printf("at 100 %d elements\n",totalElements_);
        double ratio;
        if (leftRows>2000)
          ratio=4.0;
        else if (leftRows>800)
          ratio=3.0;
        else if (leftRows>256)
          ratio=2.0;
        else
          ratio=1.5;
        if ((ratio*leftElements>full&&leftRows>denseThreshold_)) {
          //return to do dense
          if (status!=0)
            break;
          int check=0;
          for (int iColumn=0;iColumn<numberColumns_;iColumn++) {
            if (numberInColumn[iColumn]) 
              check++;
          }
          if (check!=leftRows&&denseThreshold_) {
            //printf("** mismatch %d columns left, %d rows\n",check,leftRows);
            denseThreshold=0;
          } else {
            status=2;
            if ((messageLevel_&4)!=0) 
              std::cout<<"      Went dense at "<<leftRows<<" rows "<<
                totalElements_<<" "<<full<<" "<<leftElements<<std::endl;
            if (!denseThreshold_)
              denseThreshold_=-check; // say how many
            break;
          }
        }
      }
      // start at 1 again
      count = 1;
    } else {
      //end of this - onto next
      count++;
    } 
  }				/* endwhile */
  workArea_.conditionalDelete() ;
  workArea2_.conditionalDelete() ;
  return status;
}
