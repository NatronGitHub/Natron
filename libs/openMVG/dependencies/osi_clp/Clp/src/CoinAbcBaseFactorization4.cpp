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
#if ABC_SMALL<2
//  getColumnSpaceIterateR.  Gets space for one extra R element in Column
//may have to do compression  (returns true)
//also moves existing vector
bool
CoinAbcTypeFactorization::getColumnSpaceIterateR ( CoinSimplexInt iColumn, CoinFactorizationDouble value,
					   CoinSimplexInt iRow)
{
  CoinFactorizationDouble * COIN_RESTRICT elementR = elementRAddress_ + lengthAreaR_;
  CoinSimplexInt * COIN_RESTRICT indexRowR = indexRowRAddress_ + lengthAreaR_;
  CoinBigIndex * COIN_RESTRICT startR = startColumnRAddress_+maximumPivots_+1;
  CoinSimplexInt * COIN_RESTRICT numberInColumnPlus = numberInColumnPlusAddress_;
  CoinSimplexInt number = numberInColumnPlus[iColumn];
  //*** modify so sees if can go in
  //see if it can go in at end
  CoinSimplexInt * COIN_RESTRICT nextColumn = nextColumnAddress_;
  CoinSimplexInt * COIN_RESTRICT lastColumn = lastColumnAddress_;
  if (lengthAreaR_-startR[maximumRowsExtra_]<number+1) {
    //compression
    CoinSimplexInt jColumn = nextColumn[maximumRowsExtra_];
    CoinBigIndex put = 0;
    while ( jColumn != maximumRowsExtra_ ) {
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
    startR[maximumRowsExtra_]=put;
  }
  // Still may not be room (as iColumn was still in)
  if (lengthAreaR_-startR[maximumRowsExtra_]<number+1) 
    return false;

  CoinSimplexInt next = nextColumn[iColumn];
  CoinSimplexInt last = lastColumn[iColumn];
  //out
  nextColumn[last] = next;
  lastColumn[next] = last;

  CoinBigIndex put = startR[maximumRowsExtra_];
  //in at end
  last = lastColumn[maximumRowsExtra_];
  nextColumn[last] = iColumn;
  lastColumn[maximumRowsExtra_] = iColumn;
  lastColumn[iColumn] = last;
  nextColumn[iColumn] = maximumRowsExtra_;
  
  //move
  CoinBigIndex get = startR[iColumn];
  startR[iColumn] = put;
  CoinSimplexInt i = 0;
  for (i=0 ; i < number; i ++ ) {
    elementR[put]= elementR[get];
    indexRowR[put++] = indexRowR[get++];
  }
  //insert
  elementR[put]=value;
  indexRowR[put++]=iRow;
  numberInColumnPlus[iColumn]++;
  //add 4 for luck
  startR[maximumRowsExtra_] = CoinMin(static_cast<CoinBigIndex> (put + 4) ,lengthAreaR_);
  return true;
}
#endif
CoinSimplexInt CoinAbcTypeFactorization::checkPivot(CoinSimplexDouble saveFromU,
				 CoinSimplexDouble oldPivot) const
{
  CoinSimplexInt status;
  if ( fabs ( saveFromU ) > 1.0e-8 ) {
    CoinFactorizationDouble checkTolerance;
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
#define INLINE_IT
#define INLINE_IT2
#define INLINE_IT3
#define UNROLL 0
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
  switch(number) {
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
#if 0
/* Checks if can replace one Column to basis,
   returns 0=OK, 1=Probably OK, 2=singular, 3=no room, 5 max pivots
   Fills in region for use later
   partial update already in U */
int 
CoinAbcTypeFactorization::checkReplace ( const AbcSimplex * model,
					 CoinIndexedVector * regionSparse,
					 int pivotRow,
					 CoinSimplexDouble &pivotCheck,
					 double acceptablePivot)
{
  if ( lengthR_+numberRows_ >= lengthAreaR_ ) {
    //not enough room
    return 3;
  }       
#ifdef ABC_USE_FUNCTION_POINTERS
  scatterStruct * COIN_RESTRICT scatter = scatterUColumn();
  CoinFactorizationDouble *  COIN_RESTRICT elementUColumnPlus = elementUColumnPlusAddress_;
  CoinBigIndex startU = scatter[numberRows_].offset;
  CoinSimplexInt numberInColumnU2 = scatter[numberRows_].number;
  CoinFactorizationDouble * COIN_RESTRICT elementU2 = elementUColumnPlus+startU;
  CoinSimplexInt * COIN_RESTRICT indexU2 = reinterpret_cast<CoinSimplexInt *>(elementU2+numberInColumnU2);
#else
  CoinBigIndex * COIN_RESTRICT startColumnU = startColumnUAddress_;
  CoinSimplexInt * COIN_RESTRICT numberInColumn = numberInColumnAddress_;
  CoinFactorizationDouble * COIN_RESTRICT elementU = elementUAddress_;
  CoinBigIndex startU = startColumnU[numberRows_];
  CoinSimplexInt * COIN_RESTRICT indexU2 = &indexRowUAddress_[startU];
  CoinFactorizationDouble * COIN_RESTRICT elementU2 = &elementUAddress_[startU];
  CoinSimplexInt numberInColumnU2 = numberInColumn[numberRows_];
#endif
  if ( lengthU_+numberInColumnU2 >= lengthAreaU_ ) {
    //not enough room
    return 3;
  }
#if ABC_SMALL<2
  CoinSimplexInt * COIN_RESTRICT numberInRowU = numberInRowAddress_;
#endif
  //zeroed out region
  CoinFactorizationDouble * COIN_RESTRICT region = denseVector(regionSparse);
  CoinFactorizationDouble * COIN_RESTRICT pivotRegion = pivotRegionAddress_;
  CoinFactorizationDouble oldPivot = pivotRegion[pivotRow];
  // for accuracy check
  pivotCheck = pivotCheck / oldPivot;

#ifndef ABC_USE_FUNCTION_POINTERS
  CoinBigIndex saveStart = startColumnU[pivotRow];
  CoinBigIndex saveEnd = saveStart + numberInColumn[pivotRow];
#endif
  //get entries in row (pivot not stored)
  CoinSimplexInt numberNonZero = 0;
#if ABC_SMALL<2
  CoinSimplexInt * COIN_RESTRICT indexColumnU = indexColumnUAddress_;
#endif
#if CONVERTROW
  CoinBigIndex * COIN_RESTRICT convertRowToColumn = convertRowToColumnUAddress_;
#if CONVERTROW>2
  CoinBigIndex * COIN_RESTRICT convertColumnToRow = convertColumnToRowUAddress_;
#endif
#endif
#if ABC_SMALL<2
  CoinFactorizationDouble * COIN_RESTRICT elementRowU = elementRowUAddress_;
  CoinBigIndex * COIN_RESTRICT startRowU = startRowUAddress_;
#endif
  CoinSimplexInt * COIN_RESTRICT regionIndex = regionSparse->getIndices (  );
  CoinSimplexInt status=0;
#ifndef ABC_USE_FUNCTION_POINTERS
  CoinSimplexInt *  COIN_RESTRICT indexRowU = indexRowUAddress_;
#endif

#ifndef NDEBUG
#if CONVERTROW
#define CONVERTDEBUG
#endif
#endif
  int nInRow;
#if ABC_SMALL<2
  CoinBigIndex start=0;
  CoinBigIndex end=0;
  if (gotUCopy()) {
    start=startRowU[pivotRow];
    nInRow=numberInRowU[pivotRow];
    end= start + nInRow;
    CoinSimplexInt smallestIndex=pivotLinkedForwardsAddress_[-1];
    CoinSimplexInt *  COIN_RESTRICT pivotRowForward = pivotColumnAddress_;
    if (nInRow<10) {
      CoinSimplexInt smallest=numberRowsExtra_;
      for (CoinBigIndex i = start; i < end ; i ++ ) {
	CoinSimplexInt jColumn = indexColumnU[i];
	if (pivotRowForward[jColumn]<smallest) {
	  smallest=pivotRowForward[jColumn];
	  smallestIndex=jColumn;
	}
#ifndef ABC_USE_FUNCTION_POINTERS
#ifdef CONVERTDEBUG
	CoinBigIndex j = convertRowToColumn[i]+startColumnU[jColumn];
	assert (fabs(elementU[j]-elementRowU[i])<1.0e-4);
#endif      
	region[jColumn] = elementRowU[i];
#else
#ifdef CONVERTDEBUG
	CoinBigIndex j = convertRowToColumn[i];
	CoinFactorizationDouble * COIN_RESTRICT area = elementUColumnPlus+scatter[jColumn].offset;
	assert (fabs(area[j]-elementRowU[i])<1.0e-4);
#endif      
	region[jColumn] = elementRowU[i];
#endif
	regionIndex[numberNonZero++] = jColumn;
      }
    } else {
      for (CoinBigIndex i = start; i < end ; i ++ ) {
	CoinSimplexInt jColumn = indexColumnU[i];
#ifdef CONVERTDEBUG
#ifdef ABC_USE_FUNCTION_POINTERS
	CoinBigIndex j = convertRowToColumn[i];
	CoinFactorizationDouble * COIN_RESTRICT area = elementUColumnPlus+scatter[jColumn].offset;
	assert (fabs(area[j]-elementRowU[i])<1.0e-4);
#else
	CoinBigIndex j = convertRowToColumn[i]+startColumnU[jColumn];
	assert (fabs(elementU[j]-elementRowU[i])<1.0e-4);
#endif
#endif      
	region[jColumn] = elementRowU[i];
	regionIndex[numberNonZero++] = jColumn;
      }
    }
    //do BTRAN - finding first one to use
    regionSparse->setNumElements ( numberNonZero );
    if (numberNonZero) {
      assert (smallestIndex>=0);
#ifndef NDEBUG
      const CoinSimplexInt *  COIN_RESTRICT pivotLinked = pivotLinkedForwardsAddress_;
      CoinSimplexInt jRow=pivotLinked[-1];
      if (jRow!=smallestIndex) {
	while (jRow>=0) {
	  CoinFactorizationDouble pivotValue = region[jRow];
	  if (pivotValue)
	    break;
	  jRow=pivotLinked[jRow];
	}
	assert (jRow==smallestIndex);
      }
#endif
      //smallestIndex=pivotLinkedForwardsAddress_[-1];
      updateColumnTransposeU ( regionSparse, smallestIndex 
#if ABC_SMALL<2
		  ,reinterpret_cast<CoinAbcStatistics &>(btranCountInput_)
#endif
#if ABC_PARALLEL
		    ,0
#endif
);
    }
  } else {
#endif
#if ABC_SMALL>=0
    // No row copy check space
    CoinSimplexInt * COIN_RESTRICT indexRowR = indexRowRAddress_;
    CoinFactorizationDouble *  COIN_RESTRICT elementR = elementRAddress_;
    CoinBigIndex COIN_RESTRICT * deletedPosition = reinterpret_cast<CoinBigIndex *>(elementR+lengthR_);
    CoinSimplexInt COIN_RESTRICT * deletedColumns = reinterpret_cast<CoinSimplexInt *>(indexRowR+lengthR_);
    nInRow=replaceColumnU(regionSparse,deletedPosition,deletedColumns,pivotRow);
#endif
#if ABC_SMALL<2
  }
#endif
  numberNonZero = regionSparse->getNumElements (  );
  //printf("replace nels %d\n",numberNonZero);
  CoinFactorizationDouble saveFromU = 0.0;

  for (CoinBigIndex i = 0; i < numberInColumnU2; i++ ) {
    CoinSimplexInt iRow = indexU2[i];
    if ( iRow != pivotRow ) {
      saveFromU -= elementU2[i] * region[iRow];
    } else {
      saveFromU += elementU2[i];
    }
  }       
  //check accuracy
  status = checkPivot(saveFromU,pivotCheck);
  if (status==1&&!numberPivots_) {
    printf("check status ok\n");
    status=2;
  }
  pivotCheck=saveFromU;
  return status;
}
//  replaceColumn.  Replaces one Column to basis
//      returns 0=OK, 1=Probably OK, 2=singular, 3=no room
//partial update already in U
CoinSimplexInt
CoinAbcTypeFactorization::replaceColumn ( CoinIndexedVector * regionSparse,
                                 CoinSimplexInt pivotRow,
				  CoinSimplexDouble pivotCheck ,
				  bool skipBtranU,
				   CoinSimplexDouble )
{
  assert (numberU_<=numberRowsExtra_);
  
#ifndef ALWAYS_SKIP_BTRAN
  //return at once if too many iterations
  if ( numberRowsExtra_ >= maximumRowsExtra_ ) {
    return 5;
  }       
  if ( lengthAreaU_ < lastEntryByColumnU_ ) {
    return 3;
  }
#endif  
#ifndef ABC_USE_FUNCTION_POINTERS
  CoinBigIndex * COIN_RESTRICT startColumnU = startColumnUAddress_;
  CoinSimplexInt * COIN_RESTRICT numberInColumn = numberInColumnAddress_;
  CoinFactorizationDouble * COIN_RESTRICT elementU = elementUAddress_;
#endif
  CoinSimplexInt * COIN_RESTRICT indexRowR = indexRowRAddress_;
#if ABC_SMALL<2
  CoinSimplexInt * COIN_RESTRICT numberInRowU = numberInRowAddress_;
#endif
#if ABC_SMALL<2
  CoinSimplexInt * COIN_RESTRICT numberInColumnPlus = numberInColumnPlusAddress_;
#endif
  CoinSimplexInt *  COIN_RESTRICT permuteLookup = pivotColumnAddress_;
  CoinFactorizationDouble * COIN_RESTRICT region = denseVector(regionSparse);
  
  //take out old pivot column
  
#ifndef ABC_USE_FUNCTION_POINTERS
  totalElements_ -= numberInColumn[pivotRow];
#else
  scatterStruct * COIN_RESTRICT scatter = scatterUColumn();
  extern scatterUpdate AbcScatterLowSubtract[9];
  extern scatterUpdate AbcScatterHighSubtract[4];
  CoinFactorizationDouble *  COIN_RESTRICT elementUColumnPlus = elementUColumnPlusAddress_;
  CoinBigIndex startU = scatter[numberRows_].offset;
  totalElements_ -= scatter[pivotRow].number;
  CoinBigIndex saveStart = scatter[pivotRow].offset;
  CoinBigIndex saveEnd = scatter[pivotRow].number;
  scatter[pivotRow].offset=startU;
  CoinSimplexInt numberInColumnU2 = scatter[numberRows_].number;
  CoinFactorizationDouble * COIN_RESTRICT elementU2 = elementUColumnPlus+startU;
  CoinSimplexInt * COIN_RESTRICT indexU2 = reinterpret_cast<CoinSimplexInt *>(elementU2+numberInColumnU2);
#endif
#ifndef ALWAYS_SKIP_BTRAN

#ifndef ABC_USE_FUNCTION_POINTERS
  CoinBigIndex saveStart = startColumnU[pivotRow];
  CoinBigIndex saveEnd = saveStart + numberInColumn[pivotRow];
#endif
#endif
  //get entries in row (pivot not stored)
  CoinSimplexInt numberNonZero = 0;
#if ABC_SMALL<2
  CoinSimplexInt * COIN_RESTRICT indexColumnU = indexColumnUAddress_;
#endif
#if CONVERTROW
  CoinBigIndex * COIN_RESTRICT convertRowToColumn = convertRowToColumnUAddress_;
#if CONVERTROW>2
  CoinBigIndex * COIN_RESTRICT convertColumnToRow = convertColumnToRowUAddress_;
#endif
#endif
#if ABC_SMALL<2
  CoinFactorizationDouble * COIN_RESTRICT elementRowU = elementRowUAddress_;
  CoinBigIndex * COIN_RESTRICT startRowU = startRowUAddress_;
#endif
  CoinSimplexInt * COIN_RESTRICT regionIndex = regionSparse->getIndices (  );
  CoinFactorizationDouble pivotValue = 1.0;
  CoinSimplexInt status=0;
#ifndef ABC_USE_FUNCTION_POINTERS
  CoinSimplexInt *  COIN_RESTRICT indexRowU = indexRowUAddress_;
#endif
  CoinFactorizationDouble *  COIN_RESTRICT elementR = elementRAddress_;

#ifndef NDEBUG
#if CONVERTROW
#define CONVERTDEBUG
#endif
#endif
  int nInRow;
#if ABC_SMALL<2
  CoinBigIndex start=0;
  CoinBigIndex end=0;
  if (gotUCopy()) {
    start=startRowU[pivotRow];
    nInRow=numberInRowU[pivotRow];
    end= start + nInRow;
  }
#endif
  CoinFactorizationDouble * COIN_RESTRICT pivotRegion = pivotRegionAddress_;
#ifndef ALWAYS_SKIP_BTRAN
  if (!skipBtranU) {
  CoinFactorizationDouble oldPivot = pivotRegion[pivotRow];
  // for accuracy check
  pivotCheck = pivotCheck / oldPivot;
#if ABC_SMALL<2
  if (gotUCopy()) {
    CoinSimplexInt smallestIndex=pivotLinkedForwardsAddress_[-1];
    CoinSimplexInt *  COIN_RESTRICT pivotRowForward = pivotColumnAddress_;
    if (nInRow<10) {
      CoinSimplexInt smallest=numberRowsExtra_;
      for (CoinBigIndex i = start; i < end ; i ++ ) {
	CoinSimplexInt jColumn = indexColumnU[i];
	if (pivotRowForward[jColumn]<smallest) {
	  smallest=pivotRowForward[jColumn];
	  smallestIndex=jColumn;
	}
#ifndef ABC_USE_FUNCTION_POINTERS
#ifdef CONVERTDEBUG
	CoinBigIndex j = convertRowToColumn[i]+startColumnU[jColumn];
	assert (fabs(elementU[j]-elementRowU[i])<1.0e-4);
#endif      
	region[jColumn] = elementRowU[i];
#else
#ifdef CONVERTDEBUG
	CoinBigIndex j = convertRowToColumn[i];
	CoinFactorizationDouble * COIN_RESTRICT area = elementUColumnPlus+scatter[jColumn].offset;
	assert (fabs(area[j]-elementRowU[i])<1.0e-4);
#endif      
	region[jColumn] = elementRowU[i];
#endif
	regionIndex[numberNonZero++] = jColumn;
      }
    } else {
      for (CoinBigIndex i = start; i < end ; i ++ ) {
	CoinSimplexInt jColumn = indexColumnU[i];
#ifdef CONVERTDEBUG
#ifdef ABC_USE_FUNCTION_POINTERS
	CoinBigIndex j = convertRowToColumn[i];
	CoinFactorizationDouble * COIN_RESTRICT area = elementUColumnPlus+scatter[jColumn].offset;
	assert (fabs(area[j]-elementRowU[i])<1.0e-4);
#else
	CoinBigIndex j = convertRowToColumn[i]+startColumnU[jColumn];
	assert (fabs(elementU[j]-elementRowU[i])<1.0e-4);
#endif
#endif      
	region[jColumn] = elementRowU[i];
	regionIndex[numberNonZero++] = jColumn;
      }
    }
    //do BTRAN - finding first one to use
    regionSparse->setNumElements ( numberNonZero );
    if (numberNonZero) {
      assert (smallestIndex>=0);
#ifndef NDEBUG
      const CoinSimplexInt *  COIN_RESTRICT pivotLinked = pivotLinkedForwardsAddress_;
      CoinSimplexInt jRow=pivotLinked[-1];
      if (jRow!=smallestIndex) {
	while (jRow>=0) {
	  CoinFactorizationDouble pivotValue = region[jRow];
	  if (pivotValue)
	    break;
	  jRow=pivotLinked[jRow];
	}
	assert (jRow==smallestIndex);
      }
#endif
      //smallestIndex=pivotLinkedForwardsAddress_[-1];
      updateColumnTransposeU ( regionSparse, smallestIndex 
#if ABC_SMALL<2
		  ,reinterpret_cast<CoinAbcStatistics &>(btranCountInput_)
#endif
#if ABC_PARALLEL
		    ,0
#endif
);
    }
  } else {
#endif
#if ABC_SMALL>=0
    // No row copy check space
    if ( lengthR_ + numberRows_>= lengthAreaR_ ) {
      //not enough room
      return 3;
    }       
    CoinBigIndex COIN_RESTRICT * deletedPosition = reinterpret_cast<CoinBigIndex *>(elementR+lengthR_);
    CoinSimplexInt COIN_RESTRICT * deletedColumns = reinterpret_cast<CoinSimplexInt *>(indexRowR+lengthR_);
    nInRow=replaceColumnU(regionSparse,deletedPosition,deletedColumns,pivotRow);
#endif
#if ABC_SMALL<2
  }
#endif
  }
#endif
  numberNonZero = regionSparse->getNumElements (  );
  CoinFactorizationDouble saveFromU = 0.0;

#ifndef ABC_USE_FUNCTION_POINTERS
  CoinBigIndex startU = startColumnU[numberRows_];
  startColumnU[pivotRow]=startU;
  CoinSimplexInt * COIN_RESTRICT indexU2 = &indexRowUAddress_[startU];
  CoinFactorizationDouble * COIN_RESTRICT elementU2 = &elementUAddress_[startU];
  CoinSimplexInt numberInColumnU2 = numberInColumn[numberRows_];
#else
//CoinSimplexInt numberInColumnU2 = scatter[numberRows_].number;
#endif
  instrument_do("CoinAbcFactorizationReplaceColumn",2*numberRows_+4*numberInColumnU2);
#ifndef ALWAYS_SKIP_BTRAN
  if (!skipBtranU) {
  for (CoinBigIndex i = 0; i < numberInColumnU2; i++ ) {
    CoinSimplexInt iRow = indexU2[i];
    if ( iRow != pivotRow ) {
      saveFromU -= elementU2[i] * region[iRow];
    } else {
      saveFromU += elementU2[i];
    }
  }       
  //check accuracy
  status = checkPivot(saveFromU,pivotCheck);
  } else {
    status=0;
    saveFromU=pivotCheck;
  }
  //regionSparse->print();
#endif
  if (status>1||(status==1&&!numberPivots_)) {
    // restore some things
    //pivotRegion[pivotRow] = oldPivot;
    CoinSimplexInt number = saveEnd-saveStart;
    totalElements_ += number;
    //numberInColumn[pivotRow]=number;
    regionSparse->clear();
    return status;
  } else {
    pivotValue = 1.0 / saveFromU;
    // do what we would have done by now
#if ABC_SMALL<2
    if (gotUCopy()) {
      for (CoinBigIndex i = start; i < end ; i ++ ) {
	CoinSimplexInt jColumn = indexColumnU[i];
#ifndef ABC_USE_FUNCTION_POINTERS
	CoinBigIndex startColumn = startColumnU[jColumn];
#if CONVERTROW
	CoinBigIndex j = convertRowToColumn[i];
#else
	CoinBigIndex j;
	int number = numberInColumn[jColumn];
	for (j=0;j<number;j++) {
	  if (indexRowU[j+startColumn]==pivotRow)
	    break;
	}
	assert (j<number);
	//assert (j==convertRowToColumn[i]); // temp
#endif
#ifdef CONVERTDEBUG
	assert (fabs(elementU[j+startColumn]-elementRowU[i])<1.0e-4);
#endif      
#else
	int number = scatter[jColumn].number;
	CoinSimplexInt k = number-1;
	scatter[jColumn].number=k;
	CoinFactorizationDouble * COIN_RESTRICT area = elementUColumnPlus+scatter[jColumn].offset;
	CoinSimplexInt * COIN_RESTRICT indices = reinterpret_cast<CoinSimplexInt *>(area+k+1);
#if CONVERTROW
	CoinSimplexInt j = convertRowToColumn[i];
#else
	CoinSimplexInt j;
	for (j=0;j<number;j++) {
	  if (indices[j]==pivotRow)
	    break;
	}
	assert (j<number);
	//assert (j==convertRowToColumn[i]); // temp
#endif
#ifdef CONVERTDEBUG
	assert (fabs(area[j]-elementRowU[i])<1.0e-4);
#endif      
#endif
	// swap
#ifndef ABC_USE_FUNCTION_POINTERS
	CoinSimplexInt k = numberInColumn[jColumn]-1;
	numberInColumn[jColumn]=k;
	CoinBigIndex k2 = k+startColumn;
	int kRow2=indexRowU[k2];
	indexRowU[j+startColumn]=kRow2;
	CoinFactorizationDouble value2=elementU[k2];
	elementU[j+startColumn]=value2;
#else
	int kRow2=indices[k];
	CoinFactorizationDouble value2=area[k];
#if ABC_USE_FUNCTION_POINTERS
	if (k<9) {
	  scatter[jColumn].functionPointer=AbcScatterLowSubtract[k];
	} else {
	  scatter[jColumn].functionPointer=AbcScatterHighSubtract[k&3];
	}
#endif
	// later more complicated swap to avoid move
	indices[j]=kRow2;
	area[j]=value2;
	// move 
	indices-=2;
	for (int i=0;i<k;i++)
	  indices[i]=indices[i+2];
#endif
	// move in row copy (slow - as other remark (but shouldn't need to))
	CoinBigIndex start2 = startRowU[kRow2];
	int n=numberInRowU[kRow2];
	CoinBigIndex end2 = start2 + n;
#ifndef NDEBUG
	bool found=false;
#endif
	for (CoinBigIndex i2 = start2; i2 < end2 ; i2 ++ ) {
	  CoinSimplexInt iColumn2 = indexColumnU[i2];
	  if (jColumn==iColumn2) {
#if CONVERTROW
	    convertRowToColumn[i2] = j;
#endif
#ifndef NDEBUG
	    found=true;
#endif
	    break;
	  }
	}
#ifndef NDEBUG
	assert(found);
#endif
      }
    } else {
#endif
#if ABC_SMALL>=0
      // no row copy
      CoinBigIndex COIN_RESTRICT * deletedPosition = reinterpret_cast<CoinBigIndex *>(elementR+lengthR_);
      CoinSimplexInt COIN_RESTRICT * deletedColumns = reinterpret_cast<CoinSimplexInt *>(indexRowR+lengthR_);
      for (CoinSimplexInt i = 0; i < nInRow ; i ++ ) {
	CoinBigIndex j = deletedPosition[i];
	CoinSimplexInt jColumn = deletedColumns[i];
	// swap
	CoinSimplexInt k = numberInColumn[jColumn]-1;
	numberInColumn[jColumn]=k;
	CoinBigIndex k2 = k+startColumnU[jColumn];
	int kRow2=indexRowU[k2];
	indexRowU[j]=kRow2;
	CoinFactorizationDouble value2=elementU[k2];
	elementU[j]=value2;
      }
#endif
#if ABC_SMALL<2
    }
#endif
  }
#if ABC_SMALL<2
  if (gotUCopy()) {
    // Now zero out column of U
    //take out old pivot column
#ifdef ABC_USE_FUNCTION_POINTERS
    CoinFactorizationDouble * COIN_RESTRICT elementU = elementUColumnPlus+saveStart;
    saveStart=0;
    CoinSimplexInt * COIN_RESTRICT indexRowU = reinterpret_cast<CoinSimplexInt *>(elementU+saveEnd);
#endif
    for (CoinBigIndex i = saveStart; i < saveEnd ; i ++ ) {
      //elementU[i] = 0.0;
      // If too slow then reverse meaning of convertRowToColumn and use
      int jRow=indexRowU[i];
      CoinBigIndex start = startRowU[jRow];
      CoinBigIndex end = start + numberInRowU[jRow];
      for (CoinBigIndex j = start; j < end ; j ++ ) {
	CoinSimplexInt jColumn = indexColumnU[j];
	if (jColumn==pivotRow) {
	  // swap
	  numberInRowU[jRow]--;
	  elementRowU[j]=elementRowU[end-1];
#if CONVERTROW
	  convertRowToColumn[j]=convertRowToColumn[end-1];
#endif
	  indexColumnU[j]=indexColumnU[end-1];
	  break;
	}
      }
    }    
  }
#endif   
  //zero out pivot Row (before or after?)
  //add to R
  CoinBigIndex * COIN_RESTRICT startColumnR = startColumnRAddress_;
  CoinBigIndex putR = lengthR_;
  CoinSimplexInt number = numberR_;
  
  startColumnR[number] = putR;  //for luck and first time
  number++;
  //assert (startColumnR+number-firstCountAddress_<( CoinMax(5*numberRows_,2*numberRows_+2*maximumPivots_)+2));
  startColumnR[number] = putR + numberNonZero;
  numberR_ = number;
  lengthR_ = putR + numberNonZero;
  totalElements_ += numberNonZero;
  if ( lengthR_ >= lengthAreaR_ ) {
    //not enough room
    regionSparse->clear();
    return 3;
  }       
  if ( lengthU_+numberInColumnU2 >= lengthAreaU_ ) {
    //not enough room
    regionSparse->clear();
    return 3;
  }

#if ABC_SMALL<2
  CoinSimplexInt *  COIN_RESTRICT nextRow=NULL;
  CoinSimplexInt *  COIN_RESTRICT lastRow=NULL;
  CoinSimplexInt next;
  CoinSimplexInt last;
  if (gotUCopy()) {
    //take out row
    nextRow = nextRowAddress_;
    lastRow = lastRowAddress_;
    next = nextRow[pivotRow];
    last = lastRow[pivotRow];
    
    nextRow[last] = next;
    lastRow[next] = last;
    numberInRowU[pivotRow]=0;
  }
#endif  
  //put in pivot
  //add row counts
  
  int n = 0;
  for (CoinSimplexInt i = 0; i < numberInColumnU2; i++ ) {
    CoinSimplexInt iRow = indexU2[i];
    CoinFactorizationDouble value=elementU2[i];
    assert (value);
    if ( !TEST_LESS_THAN_TOLERANCE(value ) ) {
      if ( iRow != pivotRow ) {
	//modify by pivot
	CoinFactorizationDouble value2 = value*pivotValue;
	indexU2[n]=iRow;
	elementU2[n++] = value2;
#if ABC_SMALL<2
	if (gotUCopy()) {
	  CoinSimplexInt next = nextRow[iRow];
	  CoinSimplexInt iNumberInRow = numberInRowU[iRow];
	  CoinBigIndex space;
	  CoinBigIndex put = startRowU[iRow] + iNumberInRow;
	  space = startRowU[next] - put;
	  if ( space <= 0 ) {
	    getRowSpaceIterate ( iRow, iNumberInRow + 4 );
	    put = startRowU[iRow] + iNumberInRow;
	  } else if (next==numberRows_) {
	    lastEntryByRowU_=put+1;
	  }     
	  indexColumnU[put] = pivotRow;
#if CONVERTROW
	  convertRowToColumn[put] = i;
#endif
	  elementRowU[put] = value2;
	  numberInRowU[iRow] = iNumberInRow + 1;
	}
#endif
      } else {
	//swap and save
	indexU2[i]=indexU2[numberInColumnU2-1];
	elementU2[i] = elementU2[numberInColumnU2-1];
	numberInColumnU2--;
	i--;
      }
    } else {
      // swap
      indexU2[i]=indexU2[numberInColumnU2-1];
      elementU2[i] = elementU2[numberInColumnU2-1];
      numberInColumnU2--;
      i--;
    }       
  }       
  numberInColumnU2=n;
  //do permute
  permuteAddress_[numberRowsExtra_] = pivotRow;
  //and for safety
  permuteAddress_[numberRowsExtra_ + 1] = 0;

  //pivotColumnAddress_[pivotRow] = numberRowsExtra_;
  
  numberU_++;

#if ABC_SMALL<2
  if (gotUCopy()) {
    //in at end
    last = lastRow[numberRows_];
    nextRow[last] = pivotRow; //numberRowsExtra_;
    lastRow[numberRows_] = pivotRow; //numberRowsExtra_;
    lastRow[pivotRow] = last;
    nextRow[pivotRow] = numberRows_;
    startRowU[pivotRow] = lastEntryByRowU_;
  }
#endif
#ifndef ABC_USE_FUNCTION_POINTERS
  numberInColumn[pivotRow]=numberInColumnU2;
#else
#if ABC_USE_FUNCTION_POINTERS
  if (numberInColumnU2<9) {
    scatter[pivotRow].functionPointer=AbcScatterLowSubtract[numberInColumnU2];
  } else {
    scatter[pivotRow].functionPointer=AbcScatterHighSubtract[numberInColumnU2&3];
  }
#endif
  assert(scatter[pivotRow].offset==lastEntryByColumnUPlus_);
  //CoinFactorizationDouble * COIN_RESTRICT area = elementUColumnPlus+lastEntryByColumnUPlus_;
  //CoinSimplexInt * COIN_RESTRICT indices = reinterpret_cast<CoinSimplexInt *>(area+numberInColumnU2);
  if (numberInColumnU2<scatter[numberRows_].number) {
    int offset=2*(scatter[numberRows_].number-numberInColumnU2);
    indexU2 -= offset;
    //assert(indexU2-reinterpret_cast<int*>(area)>=0);
    for (int i=0;i<numberInColumnU2;i++)
      indexU2[i]=indexU2[i+offset];
  }
  scatter[pivotRow].number=numberInColumnU2;
  //CoinAbcMemcpy(indices,indexU2,numberInColumnU2);
  //CoinAbcMemcpy(area,elementU2,numberInColumnU2);
  lastEntryByColumnUPlus_ += (3*numberInColumnU2+1)>>1;
#endif
  totalElements_ += numberInColumnU2;
  lengthU_ += numberInColumnU2;
#if ABC_SMALL<2
  CoinSimplexInt * COIN_RESTRICT nextColumn = nextColumnAddress_;
  CoinSimplexInt * COIN_RESTRICT lastColumn = lastColumnAddress_;
  if (gotRCopy()) {
    //column in at beginning (as empty)
    next = nextColumn[maximumRowsExtra_];
    lastColumn[next] = numberRowsExtra_;
    nextColumn[maximumRowsExtra_] = numberRowsExtra_;
    nextColumn[numberRowsExtra_] = next;
    lastColumn[numberRowsExtra_] = maximumRowsExtra_;
  }
#endif
#if 0
  {
    int k=-1;
    for (int i=0;i<numberRows_;i++) {
      k=CoinMax(k,startRowU[i]+numberInRowU[i]);
    }
    assert (k==lastEntryByRowU_);
  }
#endif
  //pivotRegion[numberRowsExtra_] = pivotValue;
  pivotRegion[pivotRow] = pivotValue;
#ifndef ABC_USE_FUNCTION_POINTERS
  maximumU_ = CoinMax(maximumU_,startU+numberInColumnU2);
#else
  maximumU_ = CoinMax(maximumU_,lastEntryByColumnUPlus_);
#endif
  permuteLookup[pivotRow]=numberRowsExtra_;
  permuteLookup[numberRowsExtra_]=pivotRow;
  //pivotColumnAddress_[pivotRow]=numberRowsExtra_;
  //pivotColumnAddress_[numberRowsExtra_]=pivotRow;
  assert (pivotRow<numberRows_);
  // out of chain
  CoinSimplexInt iLast=pivotLinkedBackwardsAddress_[pivotRow];
  CoinSimplexInt iNext=pivotLinkedForwardsAddress_[pivotRow];
  assert (pivotRow==pivotLinkedForwardsAddress_[iLast]);
  assert (pivotRow==pivotLinkedBackwardsAddress_[iNext]);
  pivotLinkedForwardsAddress_[iLast]=iNext;
  pivotLinkedBackwardsAddress_[iNext]=iLast;
  if (pivotRow==lastSlack_) {
    lastSlack_ = iLast;
  }
  iLast=pivotLinkedBackwardsAddress_[numberRows_];
  assert (numberRows_==pivotLinkedForwardsAddress_[iLast]);
  pivotLinkedForwardsAddress_[iLast]=pivotRow;
  pivotLinkedBackwardsAddress_[pivotRow]=iLast;
  pivotLinkedBackwardsAddress_[numberRows_]=pivotRow;
  pivotLinkedForwardsAddress_[pivotRow]=numberRows_;
  assert (numberRows_>pivotLinkedForwardsAddress_[-1]);
  assert (pivotLinkedBackwardsAddress_[numberRows_]>=0);
  numberRowsExtra_++;
  numberGoodU_++;
  numberPivots_++;
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
#if ABC_SMALL<2
  if (gotRCopy()&&status<2) {
    //if (numberInColumnPlus&&status<2) {
    // we are going to put another copy of R in R
    CoinFactorizationDouble * COIN_RESTRICT elementRR = elementRAddress_ + lengthAreaR_;
    CoinSimplexInt * COIN_RESTRICT indexRowRR = indexRowRAddress_ + lengthAreaR_;
    CoinBigIndex * COIN_RESTRICT startRR = startColumnRAddress_+maximumPivots_+1;
    CoinSimplexInt pivotRow = numberRowsExtra_-1;
    for (CoinBigIndex i = 0; i < numberNonZero; i++ ) {
      CoinSimplexInt jRow = regionIndex[i];
      CoinFactorizationDouble value=region[jRow];
      elementR[putR] = value;
      indexRowR[putR] = jRow;
      putR++;
      region[jRow]=0.0;
      int iRow = permuteLookup[jRow];
      next = nextColumn[iRow];
      CoinBigIndex space;
      if (next!=maximumRowsExtra_)
	space = startRR[next]-startRR[iRow];
      else
	space = lengthAreaR_-startRR[iRow];
      CoinSimplexInt numberInR = numberInColumnPlus[iRow];
      if (space>numberInR) {
	// there is space
	CoinBigIndex  put=startRR[iRow]+numberInR;
	numberInColumnPlus[iRow]=numberInR+1;
	indexRowRR[put]=pivotRow;
	elementRR[put]=value;
	//add 4 for luck
	if (next==maximumRowsExtra_)
	  startRR[maximumRowsExtra_] = CoinMin(static_cast<CoinBigIndex> (put + 4) ,lengthAreaR_);
      } else {
	// no space - do we shuffle?
	if (!getColumnSpaceIterateR(iRow,value,pivotRow)) {
	  // printf("Need more space for R\n");
	  numberInColumnPlus_.conditionalDelete();
	  numberInColumnPlusAddress_=NULL;
	  setNoGotRCopy();
	  regionSparse->clear();
#if ABC_SMALL<0
	  status=3;
#endif
	  break;
	}
      }
    }       
  } else {
#endif
    for (CoinBigIndex i = 0; i < numberNonZero; i++ ) {
      CoinSimplexInt iRow = regionIndex[i];
      elementR[putR] = region[iRow];
      region[iRow]=0.0;
      indexRowR[putR] = iRow;
      putR++;
    }       
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
    printf("=============================================start\n");
    for (int iRow=0;iRow<numberRows_;iRow++) {
      int number=scatter[iRow].number;
      CoinBigIndex offset = scatter[iRow].offset;
      CoinFactorizationDouble * COIN_RESTRICT area = elementUColumnPlus+offset;
      CoinSimplexInt * COIN_RESTRICT indices = reinterpret_cast<CoinSimplexInt *>(area+number);
      CoinSimplexInt * COIN_RESTRICT indices2 = indexRow+startColumnU[iRow];
      CoinFactorizationDouble * COIN_RESTRICT area2 = element+startColumnU[iRow];
      //assert (number==numberInColumn[iRow]);
#if ABC_USE_FUNCTION_POINTERS
      if (number<9)
	assert (scatter[iRow].functionPointer==AbcScatterLowSubtract[number]);
      else
	assert (scatter[iRow].functionPointer==AbcScatterHighSubtract[number&3]);
#endif
      if (number)
	printf("Row %d %d els\n",iRow,number);
      int n=0;
      for (int i=0;i<number;i++) {
	printf("(%d,%g) ",indices[i],area[i]);
	n++;
	if (n==10) {
	  n=0;
	  printf("\n");
	}
      }
      if (n)
	printf("\n");
#if 0
      for (int i=0;i<number;i++) {
	assert (indices[i]==indices2[i]);
	assert (area[i]==area2[i]);
      }
#endif
    }
    printf("=============================================end\n");
  }
#endif
#endif
  regionSparse->setNumElements(0);
  //for (int i=0;i<numberRows_;i++) 
  //assert(permuteLookupAddress_[i]==pivotColumnAddress_[i]);
  return status;
}
#endif
/* Checks if can replace one Column to basis,
   returns update alpha
   Fills in region for use later
   partial update already in U */
#ifdef ABC_LONG_FACTORIZATION 
  long
#endif
double
CoinAbcTypeFactorization::checkReplacePart1 (CoinIndexedVector * regionSparse,
					      int pivotRow)
{
#ifdef ABC_USE_FUNCTION_POINTERS
  scatterStruct * COIN_RESTRICT scatter = scatterUColumn();
  CoinFactorizationDouble *  COIN_RESTRICT elementUColumnPlus = elementUColumnPlusAddress_;
  CoinBigIndex startU = scatter[numberRows_].offset;
  CoinSimplexInt numberInColumnU2 = scatter[numberRows_].number;
  CoinFactorizationDouble * COIN_RESTRICT elementU2 = elementUColumnPlus+startU;
  //CoinSimplexInt * COIN_RESTRICT indexU2 = reinterpret_cast<CoinSimplexInt *>(elementU2+numberInColumnU2);
#else
  CoinBigIndex * COIN_RESTRICT startColumnU = startColumnUAddress_;
  CoinSimplexInt * COIN_RESTRICT numberInColumn = numberInColumnAddress_;
  CoinFactorizationDouble * COIN_RESTRICT elementU = elementUAddress_;
  CoinBigIndex startU = startColumnU[numberRows_];
  CoinSimplexInt * COIN_RESTRICT indexU2 = &indexRowUAddress_[startU];
  CoinFactorizationDouble * COIN_RESTRICT elementU2 = &elementUAddress_[startU];
  CoinSimplexInt numberInColumnU2 = numberInColumn[numberRows_];
#endif
#if ABC_SMALL<2
  CoinSimplexInt * COIN_RESTRICT numberInRowU = numberInRowAddress_;
#endif
  //zeroed out region
  toLongArray(regionSparse,3);
  CoinFactorizationDouble * COIN_RESTRICT region = denseVector(regionSparse);
  //CoinFactorizationDouble * COIN_RESTRICT pivotRegion = pivotRegionAddress_;

#ifndef ABC_USE_FUNCTION_POINTERS
  CoinBigIndex saveStart = startColumnU[pivotRow];
  CoinBigIndex saveEnd = saveStart + numberInColumn[pivotRow];
#endif
  //get entries in row (pivot not stored)
  CoinSimplexInt numberNonZero = 0;
#if ABC_SMALL<2
  CoinSimplexInt * COIN_RESTRICT indexColumnU = indexColumnUAddress_;
#endif
#if CONVERTROW
#ifndef NDEBUG
#define CONVERTDEBUG
#endif
#ifdef CONVERTDEBUG
  CoinBigIndex * COIN_RESTRICT convertRowToColumn = convertRowToColumnUAddress_;
#endif
#if CONVERTROW>2
  CoinBigIndex * COIN_RESTRICT convertColumnToRow = convertColumnToRowUAddress_;
#endif
#endif
#if ABC_SMALL<2
  CoinFactorizationDouble * COIN_RESTRICT elementRowU = elementRowUAddress_;
  CoinBigIndex * COIN_RESTRICT startRowU = startRowUAddress_;
#endif
  CoinSimplexInt * COIN_RESTRICT regionIndex = regionSparse->getIndices (  );
  //CoinSimplexInt status=0;
#ifndef ABC_USE_FUNCTION_POINTERS
  CoinSimplexInt *  COIN_RESTRICT indexRowU = indexRowUAddress_;
#endif

  int nInRow;
#if ABC_SMALL<2
  CoinBigIndex start=0;
  CoinBigIndex end=0;
  if (gotUCopy()) {
    start=startRowU[pivotRow];
    nInRow=numberInRowU[pivotRow];
    end= start + nInRow;
    CoinSimplexInt smallestIndex=pivotLinkedForwardsAddress_[-1];
    CoinSimplexInt *  COIN_RESTRICT pivotRowForward = pivotColumnAddress_;
    if (nInRow<10) {
      CoinSimplexInt smallest=numberRowsExtra_;
      for (CoinBigIndex i = start; i < end ; i ++ ) {
	CoinSimplexInt jColumn = indexColumnU[i];
	if (pivotRowForward[jColumn]<smallest) {
	  smallest=pivotRowForward[jColumn];
	  smallestIndex=jColumn;
	}
#ifndef ABC_USE_FUNCTION_POINTERS
#ifdef CONVERTDEBUG
	CoinBigIndex j = convertRowToColumn[i]+startColumnU[jColumn];
	assert (fabs(elementU[j]-elementRowU[i])<1.0e-4);
#endif      
	region[jColumn] = elementRowU[i];
#else
#ifdef CONVERTDEBUG
	CoinBigIndex j = convertRowToColumn[i];
	CoinFactorizationDouble * COIN_RESTRICT area = elementUColumnPlus+scatter[jColumn].offset;
	assert (fabs(area[j]-elementRowU[i])<1.0e-4);
#endif      
	region[jColumn] = elementRowU[i];
#endif
	regionIndex[numberNonZero++] = jColumn;
      }
    } else {
      for (CoinBigIndex i = start; i < end ; i ++ ) {
	CoinSimplexInt jColumn = indexColumnU[i];
#ifdef CONVERTDEBUG
#ifdef ABC_USE_FUNCTION_POINTERS
	CoinBigIndex j = convertRowToColumn[i];
	CoinFactorizationDouble * COIN_RESTRICT area = elementUColumnPlus+scatter[jColumn].offset;
	assert (fabs(area[j]-elementRowU[i])<1.0e-4);
#else
	CoinBigIndex j = convertRowToColumn[i]+startColumnU[jColumn];
	assert (fabs(elementU[j]-elementRowU[i])<1.0e-4);
#endif
#endif      
	region[jColumn] = elementRowU[i];
	regionIndex[numberNonZero++] = jColumn;
      }
    }
    //do BTRAN - finding first one to use
    regionSparse->setNumElements ( numberNonZero );
    if (numberNonZero) {
      assert (smallestIndex>=0);
#ifndef NDEBUG
      const CoinSimplexInt *  COIN_RESTRICT pivotLinked = pivotLinkedForwardsAddress_;
      CoinSimplexInt jRow=pivotLinked[-1];
      if (jRow!=smallestIndex) {
	while (jRow>=0) {
	  CoinFactorizationDouble pivotValue = region[jRow];
	  if (pivotValue)
	    break;
	  jRow=pivotLinked[jRow];
	}
	assert (jRow==smallestIndex);
      }
#endif
#if ABC_PARALLEL==0
      //smallestIndex=pivotLinkedForwardsAddress_[-1];
      updateColumnTransposeU ( regionSparse, smallestIndex 
#if ABC_SMALL<2
	      ,reinterpret_cast<CoinAbcStatistics &>(btranCountInput_)
#endif
			       );
#else
      assert (FACTOR_CPU>3);
      updateColumnTransposeU ( regionSparse, smallestIndex 
#if ABC_SMALL<2
	      ,reinterpret_cast<CoinAbcStatistics &>(btranCountInput_)
#endif
			       ,3
			       );
#endif
    }
  } else {
#endif
#if ABC_SMALL>=0
    // No row copy check space
    CoinSimplexInt * COIN_RESTRICT indexRowR = indexRowRAddress_;
    CoinFactorizationDouble *  COIN_RESTRICT elementR = elementRAddress_;
    CoinBigIndex COIN_RESTRICT * deletedPosition = reinterpret_cast<CoinBigIndex *>(elementR+lengthR_);
    CoinSimplexInt COIN_RESTRICT * deletedColumns = reinterpret_cast<CoinSimplexInt *>(indexRowR+lengthR_);
    nInRow=replaceColumnU(regionSparse,deletedPosition,deletedColumns,pivotRow);
#endif
#if ABC_SMALL<2
  }
#endif
  if ( lengthR_+numberRows_ >= lengthAreaR_ ) {
    //not enough room
    return 0.0;
  }       
#ifdef ABC_USE_FUNCTION_POINTERS
  CoinSimplexInt * COIN_RESTRICT indexU2 = reinterpret_cast<CoinSimplexInt *>(elementU2+numberInColumnU2);
#endif
  if ( lengthU_+numberInColumnU2 >= lengthAreaU_ ) {
    //not enough room
    return 0.0;
  }
  //CoinSimplexDouble * COIN_RESTRICT region = regionSparse->denseVector (  );
  //CoinSimplexInt * COIN_RESTRICT regionIndex = regionSparse->getIndices (  );
  //CoinSimplexInt status=0;

  //int numberNonZero = regionSparse->getNumElements (  );
  //printf("replace nels %d\n",numberNonZero);
  CoinFactorizationDouble saveFromU = 0.0;

  for (CoinBigIndex i = 0; i < numberInColumnU2; i++ ) {
    CoinSimplexInt iRow = indexU2[i];
    if ( iRow != pivotRow ) {
      saveFromU -= elementU2[i] * region[iRow];
    } else {
      saveFromU += elementU2[i];
    }
  }       
  return saveFromU;
}
/* Checks if can replace one Column to basis,
   returns update alpha
   Fills in region for use later
   partial update in vector */
#ifdef ABC_LONG_FACTORIZATION 
  long
#endif
double
CoinAbcTypeFactorization::checkReplacePart1 (CoinIndexedVector * regionSparse,
					     CoinIndexedVector * partialUpdate,
					      int pivotRow)
{
#ifdef ABC_USE_FUNCTION_POINTERS
  scatterStruct * COIN_RESTRICT scatter = scatterUColumn();
  CoinFactorizationDouble *  COIN_RESTRICT elementUColumnPlus = elementUColumnPlusAddress_;
  CoinBigIndex startU = scatter[numberRows_].offset;
#else
  CoinBigIndex * COIN_RESTRICT startColumnU = startColumnUAddress_;
  CoinSimplexInt * COIN_RESTRICT numberInColumn = numberInColumnAddress_;
  CoinFactorizationDouble * COIN_RESTRICT elementU = elementUAddress_;
#endif
  CoinSimplexInt * COIN_RESTRICT indexU2 = partialUpdate->getIndices();
  CoinFactorizationDouble * COIN_RESTRICT elementU2 = denseVector(partialUpdate);
  CoinSimplexInt numberInColumnU2 = partialUpdate->getNumElements();
#if ABC_SMALL<2
  CoinSimplexInt * COIN_RESTRICT numberInRowU = numberInRowAddress_;
#endif
  //zeroed out region
  toLongArray(regionSparse,3);
  CoinFactorizationDouble * COIN_RESTRICT region = denseVector(regionSparse);
  //CoinFactorizationDouble * COIN_RESTRICT pivotRegion = pivotRegionAddress_;

#ifndef ABC_USE_FUNCTION_POINTERS
  CoinBigIndex saveStart = startColumnU[pivotRow];
  CoinBigIndex saveEnd = saveStart + numberInColumn[pivotRow];
#endif
  //get entries in row (pivot not stored)
  CoinSimplexInt numberNonZero = 0;
#if ABC_SMALL<2
  CoinSimplexInt * COIN_RESTRICT indexColumnU = indexColumnUAddress_;
#endif
#if CONVERTROW
#ifndef NDEBUG
#define CONVERTDEBUG
#endif
#ifdef CONVERTDEBUG
  CoinBigIndex * COIN_RESTRICT convertRowToColumn = convertRowToColumnUAddress_;
#endif
#if CONVERTROW>2
  CoinBigIndex * COIN_RESTRICT convertColumnToRow = convertColumnToRowUAddress_;
#endif
#endif
#if ABC_SMALL<2
  CoinFactorizationDouble * COIN_RESTRICT elementRowU = elementRowUAddress_;
  CoinBigIndex * COIN_RESTRICT startRowU = startRowUAddress_;
#endif
  CoinSimplexInt * COIN_RESTRICT regionIndex = regionSparse->getIndices (  );
  //CoinSimplexInt status=0;
#ifndef ABC_USE_FUNCTION_POINTERS
  CoinSimplexInt *  COIN_RESTRICT indexRowU = indexRowUAddress_;
#endif

  int nInRow;
#if ABC_SMALL<2
  CoinBigIndex start=0;
  CoinBigIndex end=0;
  if (gotUCopy()) {
    start=startRowU[pivotRow];
    nInRow=numberInRowU[pivotRow];
    end= start + nInRow;
    CoinSimplexInt smallestIndex=pivotLinkedForwardsAddress_[-1];
    CoinSimplexInt *  COIN_RESTRICT pivotRowForward = pivotColumnAddress_;
    if (nInRow<10) {
      CoinSimplexInt smallest=numberRowsExtra_;
      for (CoinBigIndex i = start; i < end ; i ++ ) {
	CoinSimplexInt jColumn = indexColumnU[i];
	if (pivotRowForward[jColumn]<smallest) {
	  smallest=pivotRowForward[jColumn];
	  smallestIndex=jColumn;
	}
#ifndef ABC_USE_FUNCTION_POINTERS
#ifdef CONVERTDEBUG
	CoinBigIndex j = convertRowToColumn[i]+startColumnU[jColumn];
	assert (fabs(elementU[j]-elementRowU[i])<1.0e-4);
#endif      
	region[jColumn] = elementRowU[i];
#else
#ifdef CONVERTDEBUG
	CoinBigIndex j = convertRowToColumn[i];
	CoinFactorizationDouble * COIN_RESTRICT area = elementUColumnPlus+scatter[jColumn].offset;
	assert (fabs(area[j]-elementRowU[i])<1.0e-4);
#endif      
	region[jColumn] = elementRowU[i];
#endif
	regionIndex[numberNonZero++] = jColumn;
      }
    } else {
      for (CoinBigIndex i = start; i < end ; i ++ ) {
	CoinSimplexInt jColumn = indexColumnU[i];
#ifdef CONVERTDEBUG
#ifdef ABC_USE_FUNCTION_POINTERS
	CoinBigIndex j = convertRowToColumn[i];
	CoinFactorizationDouble * COIN_RESTRICT area = elementUColumnPlus+scatter[jColumn].offset;
	assert (fabs(area[j]-elementRowU[i])<1.0e-4);
#else
	CoinBigIndex j = convertRowToColumn[i]+startColumnU[jColumn];
	assert (fabs(elementU[j]-elementRowU[i])<1.0e-4);
#endif
#endif      
	region[jColumn] = elementRowU[i];
	regionIndex[numberNonZero++] = jColumn;
      }
    }
    //do BTRAN - finding first one to use
    regionSparse->setNumElements ( numberNonZero );
    if (numberNonZero) {
      assert (smallestIndex>=0);
#ifndef NDEBUG
      const CoinSimplexInt *  COIN_RESTRICT pivotLinked = pivotLinkedForwardsAddress_;
      CoinSimplexInt jRow=pivotLinked[-1];
      if (jRow!=smallestIndex) {
	while (jRow>=0) {
	  CoinFactorizationDouble pivotValue = region[jRow];
	  if (pivotValue)
	    break;
	  jRow=pivotLinked[jRow];
	}
	assert (jRow==smallestIndex);
      }
#endif
#if ABC_PARALLEL==0
      //smallestIndex=pivotLinkedForwardsAddress_[-1];
      updateColumnTransposeU ( regionSparse, smallestIndex 
#if ABC_SMALL<2
	      ,reinterpret_cast<CoinAbcStatistics &>(btranCountInput_)
#endif
			       );
#else
      assert (FACTOR_CPU>3);
      updateColumnTransposeU ( regionSparse, smallestIndex 
#if ABC_SMALL<2
	      ,reinterpret_cast<CoinAbcStatistics &>(btranCountInput_)
#endif
			       ,3
			       );
#endif
    }
  } else {
#endif
#if ABC_SMALL>=0
    // No row copy check space
    CoinSimplexInt * COIN_RESTRICT indexRowR = indexRowRAddress_;
    CoinFactorizationDouble *  COIN_RESTRICT elementR = elementRAddress_;
    CoinBigIndex COIN_RESTRICT * deletedPosition = reinterpret_cast<CoinBigIndex *>(elementR+lengthR_);
    CoinSimplexInt COIN_RESTRICT * deletedColumns = reinterpret_cast<CoinSimplexInt *>(indexRowR+lengthR_);
    nInRow=replaceColumnU(regionSparse,deletedPosition,deletedColumns,pivotRow);
#endif
#if ABC_SMALL<2
  }
#endif
  if ( lengthR_+numberRows_ >= lengthAreaR_ ) {
    //not enough room
    return 0.0;
  }       
  if ( lengthU_+numberInColumnU2 >= lengthAreaU_ ) {
    //not enough room
    partialUpdate->clear();
    return 0.0;
  }
  //CoinSimplexDouble * COIN_RESTRICT region = regionSparse->denseVector (  );
  //CoinSimplexInt * COIN_RESTRICT regionIndex = regionSparse->getIndices (  );
  //CoinSimplexInt status=0;

  //int numberNonZero = regionSparse->getNumElements (  );
  //printf("replace nels %d\n",numberNonZero);
  CoinFactorizationDouble saveFromU = 0.0;

  for (CoinBigIndex i = 0; i < numberInColumnU2; i++ ) {
    CoinSimplexInt iRow = indexU2[i];
    if ( iRow != pivotRow ) {
      saveFromU -= elementU2[iRow] * region[iRow];
    } else {
      saveFromU += elementU2[iRow];
    }
  }       
  return saveFromU;
}
#ifdef MOVE_REPLACE_PART1A
/* Checks if can replace one Column in basis,
   returns update alpha
   Fills in region for use later
   partial update already in U */
void
CoinAbcTypeFactorization::checkReplacePart1a (CoinIndexedVector * regionSparse,
					      int pivotRow)
{
#ifdef ABC_USE_FUNCTION_POINTERS
  scatterStruct * COIN_RESTRICT scatter = scatterUColumn();
  CoinFactorizationDouble *  COIN_RESTRICT elementUColumnPlus = elementUColumnPlusAddress_;
  CoinBigIndex startU = scatter[numberRows_].offset;
  CoinSimplexInt numberInColumnU2 = scatter[numberRows_].number;
  CoinFactorizationDouble * COIN_RESTRICT elementU2 = elementUColumnPlus+startU;
  //CoinSimplexInt * COIN_RESTRICT indexU2 = reinterpret_cast<CoinSimplexInt *>(elementU2+numberInColumnU2);
#else
  CoinBigIndex * COIN_RESTRICT startColumnU = startColumnUAddress_;
  CoinSimplexInt * COIN_RESTRICT numberInColumn = numberInColumnAddress_;
  CoinFactorizationDouble * COIN_RESTRICT elementU = elementUAddress_;
  CoinBigIndex startU = startColumnU[numberRows_];
  CoinSimplexInt * COIN_RESTRICT indexU2 = &indexRowUAddress_[startU];
  CoinFactorizationDouble * COIN_RESTRICT elementU2 = &elementUAddress_[startU];
  CoinSimplexInt numberInColumnU2 = numberInColumn[numberRows_];
#endif
#if ABC_SMALL<2
  CoinSimplexInt * COIN_RESTRICT numberInRowU = numberInRowAddress_;
#endif
  //zeroed out region
  CoinFactorizationDouble * COIN_RESTRICT region = denseVector(regionSparse);
  //CoinFactorizationDouble * COIN_RESTRICT pivotRegion = pivotRegionAddress_;

#ifndef ABC_USE_FUNCTION_POINTERS
  CoinBigIndex saveStart = startColumnU[pivotRow];
  CoinBigIndex saveEnd = saveStart + numberInColumn[pivotRow];
#endif
  //get entries in row (pivot not stored)
  CoinSimplexInt numberNonZero = 0;
#if ABC_SMALL<2
  CoinSimplexInt * COIN_RESTRICT indexColumnU = indexColumnUAddress_;
#endif
#if CONVERTROW
#ifndef NDEBUG
#define CONVERTDEBUG
#endif
#ifdef CONVERTDEBUG
  CoinBigIndex * COIN_RESTRICT convertRowToColumn = convertRowToColumnUAddress_;
#endif
#if CONVERTROW>2
  CoinBigIndex * COIN_RESTRICT convertColumnToRow = convertColumnToRowUAddress_;
#endif
#endif
#if ABC_SMALL<2
  CoinFactorizationDouble * COIN_RESTRICT elementRowU = elementRowUAddress_;
  CoinBigIndex * COIN_RESTRICT startRowU = startRowUAddress_;
#endif
  CoinSimplexInt * COIN_RESTRICT regionIndex = regionSparse->getIndices (  );
  //CoinSimplexInt status=0;
#ifndef ABC_USE_FUNCTION_POINTERS
  CoinSimplexInt *  COIN_RESTRICT indexRowU = indexRowUAddress_;
#endif

  int nInRow;
#if ABC_SMALL<2
  CoinBigIndex start=0;
  CoinBigIndex end=0;
  if (gotUCopy()) {
    start=startRowU[pivotRow];
    nInRow=numberInRowU[pivotRow];
    end= start + nInRow;
    CoinSimplexInt smallestIndex=pivotLinkedForwardsAddress_[-1];
    CoinSimplexInt *  COIN_RESTRICT pivotRowForward = pivotColumnAddress_;
    if (nInRow<10) {
      CoinSimplexInt smallest=numberRowsExtra_;
      for (CoinBigIndex i = start; i < end ; i ++ ) {
	CoinSimplexInt jColumn = indexColumnU[i];
	if (pivotRowForward[jColumn]<smallest) {
	  smallest=pivotRowForward[jColumn];
	  smallestIndex=jColumn;
	}
#ifndef ABC_USE_FUNCTION_POINTERS
#ifdef CONVERTDEBUG
	CoinBigIndex j = convertRowToColumn[i]+startColumnU[jColumn];
	assert (fabs(elementU[j]-elementRowU[i])<1.0e-4);
#endif      
	region[jColumn] = elementRowU[i];
#else
#ifdef CONVERTDEBUG
	CoinBigIndex j = convertRowToColumn[i];
	CoinFactorizationDouble * COIN_RESTRICT area = elementUColumnPlus+scatter[jColumn].offset;
	assert (fabs(area[j]-elementRowU[i])<1.0e-4);
#endif      
	region[jColumn] = elementRowU[i];
#endif
	regionIndex[numberNonZero++] = jColumn;
      }
    } else {
      for (CoinBigIndex i = start; i < end ; i ++ ) {
	CoinSimplexInt jColumn = indexColumnU[i];
#ifdef CONVERTDEBUG
#ifdef ABC_USE_FUNCTION_POINTERS
	CoinBigIndex j = convertRowToColumn[i];
	CoinFactorizationDouble * COIN_RESTRICT area = elementUColumnPlus+scatter[jColumn].offset;
	assert (fabs(area[j]-elementRowU[i])<1.0e-4);
#else
	CoinBigIndex j = convertRowToColumn[i]+startColumnU[jColumn];
	assert (fabs(elementU[j]-elementRowU[i])<1.0e-4);
#endif
#endif      
	region[jColumn] = elementRowU[i];
	regionIndex[numberNonZero++] = jColumn;
      }
    }
    //do BTRAN - finding first one to use
    regionSparse->setNumElements ( numberNonZero );
    if (numberNonZero) {
      assert (smallestIndex>=0);
#ifndef NDEBUG
      const CoinSimplexInt *  COIN_RESTRICT pivotLinked = pivotLinkedForwardsAddress_;
      CoinSimplexInt jRow=pivotLinked[-1];
      if (jRow!=smallestIndex) {
	while (jRow>=0) {
	  CoinFactorizationDouble pivotValue = region[jRow];
	  if (pivotValue)
	    break;
	  jRow=pivotLinked[jRow];
	}
	assert (jRow==smallestIndex);
      }
#endif
#if ABC_PARALLEL==0
      //smallestIndex=pivotLinkedForwardsAddress_[-1];
      updateColumnTransposeU ( regionSparse, smallestIndex 
#if ABC_SMALL<2
	      ,reinterpret_cast<CoinAbcStatistics &>(btranCountInput_)
#endif
			       );
#else
      assert (FACTOR_CPU>3);
      updateColumnTransposeU ( regionSparse, smallestIndex 
#if ABC_SMALL<2
	      ,reinterpret_cast<CoinAbcStatistics &>(btranCountInput_)
#endif
			       ,3
			       );
#endif
    }
  } else {
#endif
#if ABC_SMALL>=0
    // No row copy check space
    CoinSimplexInt * COIN_RESTRICT indexRowR = indexRowRAddress_;
    CoinFactorizationDouble *  COIN_RESTRICT elementR = elementRAddress_;
    CoinBigIndex COIN_RESTRICT * deletedPosition = reinterpret_cast<CoinBigIndex *>(elementR+lengthR_);
    CoinSimplexInt COIN_RESTRICT * deletedColumns = reinterpret_cast<CoinSimplexInt *>(indexRowR+lengthR_);
    nInRow=replaceColumnU(regionSparse,deletedPosition,deletedColumns,pivotRow);
#endif
#if ABC_SMALL<2
  }
#endif
}
#ifdef ABC_LONG_FACTORIZATION 
  long
#endif
double 
CoinAbcTypeFactorization::checkReplacePart1b (CoinIndexedVector * regionSparse,
					      int pivotRow)
{
  if ( lengthR_+numberRows_ >= lengthAreaR_ ) {
    //not enough room
    return 0.0;
  }       
#ifdef ABC_USE_FUNCTION_POINTERS
  scatterStruct * COIN_RESTRICT scatter = scatterUColumn();
  CoinFactorizationDouble *  COIN_RESTRICT elementUColumnPlus = elementUColumnPlusAddress_;
  CoinBigIndex startU = scatter[numberRows_].offset;
  CoinSimplexInt numberInColumnU2 = scatter[numberRows_].number;
  CoinFactorizationDouble * COIN_RESTRICT elementU2 = elementUColumnPlus+startU;
  CoinSimplexInt * COIN_RESTRICT indexU2 = reinterpret_cast<CoinSimplexInt *>(elementU2+numberInColumnU2);
#else
  CoinBigIndex * COIN_RESTRICT startColumnU = startColumnUAddress_;
  CoinSimplexInt * COIN_RESTRICT numberInColumn = numberInColumnAddress_;
  CoinFactorizationDouble * COIN_RESTRICT elementU = elementUAddress_;
  CoinBigIndex startU = startColumnU[numberRows_];
  CoinSimplexInt * COIN_RESTRICT indexU2 = &indexRowUAddress_[startU];
  CoinFactorizationDouble * COIN_RESTRICT elementU2 = &elementUAddress_[startU];
  CoinSimplexInt numberInColumnU2 = numberInColumn[numberRows_];
#endif
  if ( lengthU_+numberInColumnU2 >= lengthAreaU_ ) {
    //not enough room
    return 0.0;
  }
  CoinFactorizationDouble * COIN_RESTRICT region = denseVector(regionSparse);
  //CoinSimplexInt * COIN_RESTRICT regionIndex = regionSparse->getIndices (  );
  //CoinSimplexInt status=0;

  //int numberNonZero = regionSparse->getNumElements (  );
  //printf("replace nels %d\n",numberNonZero);
  CoinFactorizationDouble saveFromU = 0.0;

  for (CoinBigIndex i = 0; i < numberInColumnU2; i++ ) {
    CoinSimplexInt iRow = indexU2[i];
    if ( iRow != pivotRow ) {
      saveFromU -= elementU2[i] * region[iRow];
    } else {
      saveFromU += elementU2[i];
    }
  }       
  return saveFromU;
}
#endif
#include "AbcSimplex.hpp"
/* Checks if can replace one Column to basis,
   returns 0=OK, 1=Probably OK, 2=singular, 3=no room, 5 max pivots */
int 
CoinAbcTypeFactorization::checkReplacePart2 ( int pivotRow,
					      CoinSimplexDouble /*btranAlpha*/, CoinSimplexDouble ftranAlpha, 
#ifdef ABC_LONG_FACTORIZATION 
					      long
#endif
					      double ftAlpha,
					      double /*acceptablePivot*/)
{
  if ( lengthR_+numberRows_ >= lengthAreaR_ ) {
    //not enough room
    return 3;
  }       
  CoinFactorizationDouble * COIN_RESTRICT pivotRegion = pivotRegionAddress_;
  CoinFactorizationDouble oldPivot = pivotRegion[pivotRow];
  // for accuracy check
  CoinFactorizationDouble pivotCheck = ftranAlpha / oldPivot;
  //check accuracy
  int status = checkPivot(ftAlpha,pivotCheck);
  if (status==1&&!numberPivots_) {
    printf("check status ok\n");
    status=2;
  }
  return status;
}
/* Replaces one Column to basis,
   partial update already in U */
void 
CoinAbcTypeFactorization::replaceColumnPart3 ( const AbcSimplex * /*model*/,
					  CoinIndexedVector * regionSparse,
					       CoinIndexedVector * /*tableauColumn*/,
					  int pivotRow,
#ifdef ABC_LONG_FACTORIZATION 
					       long
#endif
					  double alpha )
{
  assert (numberU_<=numberRowsExtra_);
#ifndef ABC_USE_FUNCTION_POINTERS
  CoinBigIndex * COIN_RESTRICT startColumnU = startColumnUAddress_;
  CoinSimplexInt * COIN_RESTRICT numberInColumn = numberInColumnAddress_;
  CoinFactorizationDouble * COIN_RESTRICT elementU = elementUAddress_;
#endif
  CoinSimplexInt * COIN_RESTRICT indexRowR = indexRowRAddress_;
#if ABC_SMALL<2
  CoinSimplexInt * COIN_RESTRICT numberInRowU = numberInRowAddress_;
#endif
#if ABC_SMALL<2
  CoinSimplexInt * COIN_RESTRICT numberInColumnPlus = numberInColumnPlusAddress_;
#endif
  CoinSimplexInt *  COIN_RESTRICT permuteLookup = pivotColumnAddress_;
  CoinFactorizationDouble * COIN_RESTRICT region = denseVector(regionSparse);
  
  //take out old pivot column
  
#ifndef ABC_USE_FUNCTION_POINTERS
  totalElements_ -= numberInColumn[pivotRow];
#else
  scatterStruct * COIN_RESTRICT scatter = scatterUColumn();
#if ABC_USE_FUNCTION_POINTERS
  extern scatterUpdate AbcScatterLowSubtract[9];
  extern scatterUpdate AbcScatterHighSubtract[4];
#endif
  CoinFactorizationDouble *  COIN_RESTRICT elementUColumnPlus = elementUColumnPlusAddress_;
  CoinBigIndex startU = scatter[numberRows_].offset;
  totalElements_ -= scatter[pivotRow].number;
  CoinBigIndex saveStart = scatter[pivotRow].offset;
  CoinBigIndex saveEnd = scatter[pivotRow].number;
  scatter[pivotRow].offset=startU;
  CoinSimplexInt numberInColumnU2 = scatter[numberRows_].number;
  CoinFactorizationDouble * COIN_RESTRICT elementU2 = elementUColumnPlus+startU;
  CoinSimplexInt * COIN_RESTRICT indexU2 = reinterpret_cast<CoinSimplexInt *>(elementU2+numberInColumnU2);
#endif
  //get entries in row (pivot not stored)
  CoinSimplexInt numberNonZero = 0;
#if ABC_SMALL<2
  CoinSimplexInt * COIN_RESTRICT indexColumnU = indexColumnUAddress_;
#endif
#if CONVERTROW
  CoinBigIndex * COIN_RESTRICT convertRowToColumn = convertRowToColumnUAddress_;
#if CONVERTROW>2
  CoinBigIndex * COIN_RESTRICT convertColumnToRow = convertColumnToRowUAddress_;
#endif
#endif
#if ABC_SMALL<2
  CoinFactorizationDouble * COIN_RESTRICT elementRowU = elementRowUAddress_;
  CoinBigIndex * COIN_RESTRICT startRowU = startRowUAddress_;
#endif
  CoinSimplexInt * COIN_RESTRICT regionIndex = regionSparse->getIndices (  );
  CoinFactorizationDouble pivotValue = 1.0;
  CoinSimplexInt status=0;
#ifndef ABC_USE_FUNCTION_POINTERS
  CoinSimplexInt *  COIN_RESTRICT indexRowU = indexRowUAddress_;
#endif
  CoinFactorizationDouble *  COIN_RESTRICT elementR = elementRAddress_;

#ifndef NDEBUG
#if CONVERTROW
#define CONVERTDEBUG
#endif
#endif
  int nInRow=9999999; // ? what if ABC_SMALL==0 or ABC_SMALL==1
#if ABC_SMALL<2
  CoinBigIndex start=0;
  CoinBigIndex end=0;
  if (gotUCopy()) {
    start=startRowU[pivotRow];
    nInRow=numberInRowU[pivotRow];
    end= start + nInRow;
  }
#endif
  CoinFactorizationDouble * COIN_RESTRICT pivotRegion = pivotRegionAddress_;
  numberNonZero = regionSparse->getNumElements (  );
  //CoinFactorizationDouble saveFromU = 0.0;

#ifndef ABC_USE_FUNCTION_POINTERS
  CoinBigIndex startU = startColumnU[numberRows_];
  startColumnU[pivotRow]=startU;
  CoinSimplexInt * COIN_RESTRICT indexU2 = &indexRowUAddress_[startU];
  CoinFactorizationDouble * COIN_RESTRICT elementU2 = &elementUAddress_[startU];
  CoinSimplexInt numberInColumnU2 = numberInColumn[numberRows_];
#else
//CoinSimplexInt numberInColumnU2 = scatter[numberRows_].number;
#endif
  instrument_do("CoinAbcFactorizationReplaceColumn",2*numberRows_+4*numberInColumnU2);
  pivotValue = 1.0 / alpha;
#if ABC_SMALL<2
  if (gotUCopy()) {
    for (CoinBigIndex i = start; i < end ; i ++ ) {
      CoinSimplexInt jColumn = indexColumnU[i];
#ifndef ABC_USE_FUNCTION_POINTERS
      CoinBigIndex startColumn = startColumnU[jColumn];
#if CONVERTROW
      CoinBigIndex j = convertRowToColumn[i];
#else
      CoinBigIndex j;
      int number = numberInColumn[jColumn];
      for (j=0;j<number;j++) {
	if (indexRowU[j+startColumn]==pivotRow)
	  break;
      }
      assert (j<number);
      //assert (j==convertRowToColumn[i]); // temp
#endif
#ifdef CONVERTDEBUG
      assert (fabs(elementU[j+startColumn]-elementRowU[i])<1.0e-4);
#endif      
#else
      int number = scatter[jColumn].number;
      CoinSimplexInt k = number-1;
      scatter[jColumn].number=k;
      CoinFactorizationDouble * COIN_RESTRICT area = elementUColumnPlus+scatter[jColumn].offset;
      CoinSimplexInt * COIN_RESTRICT indices = reinterpret_cast<CoinSimplexInt *>(area+k+1);
#if CONVERTROW
      CoinSimplexInt j = convertRowToColumn[i];
#else
      CoinSimplexInt j;
      for (j=0;j<number;j++) {
	if (indices[j]==pivotRow)
	  break;
      }
      assert (j<number);
      //assert (j==convertRowToColumn[i]); // temp
#endif
#ifdef CONVERTDEBUG
      assert (fabs(area[j]-elementRowU[i])<1.0e-4);
#endif      
#endif
      // swap
#ifndef ABC_USE_FUNCTION_POINTERS
      CoinSimplexInt k = numberInColumn[jColumn]-1;
      numberInColumn[jColumn]=k;
      CoinBigIndex k2 = k+startColumn;
      int kRow2=indexRowU[k2];
      indexRowU[j+startColumn]=kRow2;
      CoinFactorizationDouble value2=elementU[k2];
      elementU[j+startColumn]=value2;
#else
      int kRow2=indices[k];
      CoinFactorizationDouble value2=area[k];
#if ABC_USE_FUNCTION_POINTERS
      if (k<9) {
	scatter[jColumn].functionPointer=AbcScatterLowSubtract[k];
      } else {
	scatter[jColumn].functionPointer=AbcScatterHighSubtract[k&3];
      }
#endif
      // later more complicated swap to avoid move (don't think we can!)
      indices[j]=kRow2;
      area[j]=value2;
      // move 
      indices-=2;
      for (int i=0;i<k;i++)
	indices[i]=indices[i+2];
#endif
      // move in row copy (slow - as other remark (but shouldn't need to))
      CoinBigIndex start2 = startRowU[kRow2];
      int n=numberInRowU[kRow2];
      CoinBigIndex end2 = start2 + n;
#ifndef NDEBUG
      bool found=false;
#endif
      for (CoinBigIndex i2 = start2; i2 < end2 ; i2 ++ ) {
	CoinSimplexInt iColumn2 = indexColumnU[i2];
	if (jColumn==iColumn2) {
#if CONVERTROW
	  convertRowToColumn[i2] = j;
#endif
#ifndef NDEBUG
	  found=true;
#endif
	  break;
	}
      }
#ifndef NDEBUG
      assert(found);
#endif
    }
  } else {
#endif
#if ABC_SMALL>=0
    assert(nInRow!=9999999); // ? what if ABC_SMALL==0 or ABC_SMALL==1
    // no row copy
    CoinBigIndex COIN_RESTRICT * deletedPosition = reinterpret_cast<CoinBigIndex *>(elementR+lengthR_);
    CoinSimplexInt COIN_RESTRICT * deletedColumns = reinterpret_cast<CoinSimplexInt *>(indexRowR+lengthR_);
    for (CoinSimplexInt i = 0; i < nInRow ; i ++ ) {
      CoinBigIndex j = deletedPosition[i];
      CoinSimplexInt jColumn = deletedColumns[i];
      // swap
      CoinSimplexInt k = numberInColumn[jColumn]-1;
      numberInColumn[jColumn]=k;
      CoinBigIndex k2 = k+startColumnU[jColumn];
      int kRow2=indexRowU[k2];
      indexRowU[j]=kRow2;
      CoinFactorizationDouble value2=elementU[k2];
      elementU[j]=value2;
    }
#endif
#if ABC_SMALL<2
  }
#endif
 #if ABC_SMALL<2
  if (gotUCopy()) {
    // Now zero out column of U
    //take out old pivot column
#ifdef ABC_USE_FUNCTION_POINTERS
    CoinFactorizationDouble * COIN_RESTRICT elementU = elementUColumnPlus+saveStart;
    saveStart=0;
    CoinSimplexInt * COIN_RESTRICT indexRowU = reinterpret_cast<CoinSimplexInt *>(elementU+saveEnd);
#endif
    for (CoinBigIndex i = saveStart; i < saveEnd ; i ++ ) {
      //elementU[i] = 0.0;
      // If too slow then reverse meaning of convertRowToColumn and use
      int jRow=indexRowU[i];
      CoinBigIndex start = startRowU[jRow];
      CoinBigIndex end = start + numberInRowU[jRow];
      for (CoinBigIndex j = start; j < end ; j ++ ) {
	CoinSimplexInt jColumn = indexColumnU[j];
	if (jColumn==pivotRow) {
	  // swap
	  numberInRowU[jRow]--;
	  elementRowU[j]=elementRowU[end-1];
#if CONVERTROW
	  convertRowToColumn[j]=convertRowToColumn[end-1];
#endif
	  indexColumnU[j]=indexColumnU[end-1];
	  break;
	}
      }
    }    
  }
#endif   
  //zero out pivot Row (before or after?)
  //add to R
  CoinBigIndex * COIN_RESTRICT startColumnR = startColumnRAddress_;
  CoinBigIndex putR = lengthR_;
  CoinSimplexInt number = numberR_;
  
  startColumnR[number] = putR;  //for luck and first time
  number++;
  //assert (startColumnR+number-firstCountAddress_<( CoinMax(5*numberRows_,2*numberRows_+2*maximumPivots_)+2));
  startColumnR[number] = putR + numberNonZero;
  numberR_ = number;
  lengthR_ = putR + numberNonZero;
  totalElements_ += numberNonZero;

#if ABC_SMALL<2
  CoinSimplexInt *  COIN_RESTRICT nextRow=NULL;
  CoinSimplexInt *  COIN_RESTRICT lastRow=NULL;
  CoinSimplexInt next;
  CoinSimplexInt last;
  if (gotUCopy()) {
    //take out row
    nextRow = nextRowAddress_;
    lastRow = lastRowAddress_;
    next = nextRow[pivotRow];
    last = lastRow[pivotRow];
    
    nextRow[last] = next;
    lastRow[next] = last;
    numberInRowU[pivotRow]=0;
  }
#endif  
  //put in pivot
  //add row counts
  
  int n = 0;
  for (CoinSimplexInt i = 0; i < numberInColumnU2; i++ ) {
    CoinSimplexInt iRow = indexU2[i];
    CoinFactorizationDouble value=elementU2[i];
    assert (value);
    if ( !TEST_LESS_THAN_TOLERANCE(value ) ) {
      if ( iRow != pivotRow ) {
	//modify by pivot
	CoinFactorizationDouble value2 = value*pivotValue;
	indexU2[n]=iRow;
	elementU2[n++] = value2;
#if ABC_SMALL<2
	if (gotUCopy()) {
	  CoinSimplexInt next = nextRow[iRow];
	  CoinSimplexInt iNumberInRow = numberInRowU[iRow];
	  CoinBigIndex space;
	  CoinBigIndex put = startRowU[iRow] + iNumberInRow;
	  space = startRowU[next] - put;
	  if ( space <= 0 ) {
	    getRowSpaceIterate ( iRow, iNumberInRow + 4 );
	    put = startRowU[iRow] + iNumberInRow;
	  } else if (next==numberRows_) {
	    lastEntryByRowU_=put+1;
	  }     
	  indexColumnU[put] = pivotRow;
#if CONVERTROW
	  convertRowToColumn[put] = i;
#endif
	  elementRowU[put] = value2;
	  numberInRowU[iRow] = iNumberInRow + 1;
	}
#endif
      } else {
	//swap and save
	indexU2[i]=indexU2[numberInColumnU2-1];
	elementU2[i] = elementU2[numberInColumnU2-1];
	numberInColumnU2--;
	i--;
      }
    } else {
      // swap
      indexU2[i]=indexU2[numberInColumnU2-1];
      elementU2[i] = elementU2[numberInColumnU2-1];
      numberInColumnU2--;
      i--;
    }       
  }       
  numberInColumnU2=n;
  //do permute
  permuteAddress_[numberRowsExtra_] = pivotRow;
  //and for safety
  permuteAddress_[numberRowsExtra_ + 1] = 0;

  //pivotColumnAddress_[pivotRow] = numberRowsExtra_;
  
  numberU_++;

#if ABC_SMALL<2
  if (gotUCopy()) {
    //in at end
    last = lastRow[numberRows_];
    nextRow[last] = pivotRow; //numberRowsExtra_;
    lastRow[numberRows_] = pivotRow; //numberRowsExtra_;
    lastRow[pivotRow] = last;
    nextRow[pivotRow] = numberRows_;
    startRowU[pivotRow] = lastEntryByRowU_;
  }
#endif
#ifndef ABC_USE_FUNCTION_POINTERS
  numberInColumn[pivotRow]=numberInColumnU2;
#else
#if ABC_USE_FUNCTION_POINTERS
  if (numberInColumnU2<9) {
    scatter[pivotRow].functionPointer=AbcScatterLowSubtract[numberInColumnU2];
  } else {
    scatter[pivotRow].functionPointer=AbcScatterHighSubtract[numberInColumnU2&3];
  }
#endif
  assert(scatter[pivotRow].offset==lastEntryByColumnUPlus_);
  //CoinFactorizationDouble * COIN_RESTRICT area = elementUColumnPlus+lastEntryByColumnUPlus_;
  //CoinSimplexInt * COIN_RESTRICT indices = reinterpret_cast<CoinSimplexInt *>(area+numberInColumnU2);
  if (numberInColumnU2<scatter[numberRows_].number) {
    int offset=2*(scatter[numberRows_].number-numberInColumnU2);
    indexU2 -= offset;
    //assert(indexU2-reinterpret_cast<int*>(area)>=0);
    for (int i=0;i<numberInColumnU2;i++)
      indexU2[i]=indexU2[i+offset];
  }
  scatter[pivotRow].number=numberInColumnU2;
  //CoinAbcMemcpy(indices,indexU2,numberInColumnU2);
  //CoinAbcMemcpy(area,elementU2,numberInColumnU2);
  lastEntryByColumnUPlus_ += (3*numberInColumnU2+1)>>1;
#endif
  totalElements_ += numberInColumnU2;
  lengthU_ += numberInColumnU2;
#if ABC_SMALL<2
  CoinSimplexInt * COIN_RESTRICT nextColumn = nextColumnAddress_;
  CoinSimplexInt * COIN_RESTRICT lastColumn = lastColumnAddress_;
  if (gotRCopy()) {
    //column in at beginning (as empty)
    next = nextColumn[maximumRowsExtra_];
    lastColumn[next] = numberRowsExtra_;
    nextColumn[maximumRowsExtra_] = numberRowsExtra_;
    nextColumn[numberRowsExtra_] = next;
    lastColumn[numberRowsExtra_] = maximumRowsExtra_;
  }
#endif
#if 0
  {
    int k=-1;
    for (int i=0;i<numberRows_;i++) {
      k=CoinMax(k,startRowU[i]+numberInRowU[i]);
    }
    assert (k==lastEntryByRowU_);
  }
#endif
  //pivotRegion[numberRowsExtra_] = pivotValue;
  pivotRegion[pivotRow] = pivotValue;
#ifndef ABC_USE_FUNCTION_POINTERS
  maximumU_ = CoinMax(maximumU_,startU+numberInColumnU2);
#else
  maximumU_ = CoinMax(maximumU_,lastEntryByColumnUPlus_);
#endif
  permuteLookup[pivotRow]=numberRowsExtra_;
  permuteLookup[numberRowsExtra_]=pivotRow;
  //pivotColumnAddress_[pivotRow]=numberRowsExtra_;
  //pivotColumnAddress_[numberRowsExtra_]=pivotRow;
  assert (pivotRow<numberRows_);
  // out of chain
  CoinSimplexInt iLast=pivotLinkedBackwardsAddress_[pivotRow];
  CoinSimplexInt iNext=pivotLinkedForwardsAddress_[pivotRow];
  assert (pivotRow==pivotLinkedForwardsAddress_[iLast]);
  assert (pivotRow==pivotLinkedBackwardsAddress_[iNext]);
  pivotLinkedForwardsAddress_[iLast]=iNext;
  pivotLinkedBackwardsAddress_[iNext]=iLast;
  if (pivotRow==lastSlack_) {
    lastSlack_ = iLast;
  }
  iLast=pivotLinkedBackwardsAddress_[numberRows_];
  assert (numberRows_==pivotLinkedForwardsAddress_[iLast]);
  pivotLinkedForwardsAddress_[iLast]=pivotRow;
  pivotLinkedBackwardsAddress_[pivotRow]=iLast;
  pivotLinkedBackwardsAddress_[numberRows_]=pivotRow;
  pivotLinkedForwardsAddress_[pivotRow]=numberRows_;
  assert (numberRows_>pivotLinkedForwardsAddress_[-1]);
  assert (pivotLinkedBackwardsAddress_[numberRows_]>=0);
  numberRowsExtra_++;
  numberGoodU_++;
  numberPivots_++;
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
#if ABC_SMALL<2
  if (gotRCopy()&&status<2) {
    //if (numberInColumnPlus&&status<2) {
    // we are going to put another copy of R in R
    CoinFactorizationDouble * COIN_RESTRICT elementRR = elementRAddress_ + lengthAreaR_;
    CoinSimplexInt * COIN_RESTRICT indexRowRR = indexRowRAddress_ + lengthAreaR_;
    CoinBigIndex * COIN_RESTRICT startRR = startColumnRAddress_+maximumPivots_+1;
    CoinSimplexInt pivotRow = numberRowsExtra_-1;
    for (CoinBigIndex i = 0; i < numberNonZero; i++ ) {
      CoinSimplexInt jRow = regionIndex[i];
      CoinFactorizationDouble value=region[jRow];
      elementR[putR] = value;
      indexRowR[putR] = jRow;
      putR++;
      region[jRow]=0.0;
      int iRow = permuteLookup[jRow];
      next = nextColumn[iRow];
      CoinBigIndex space;
      if (next!=maximumRowsExtra_)
	space = startRR[next]-startRR[iRow];
      else
	space = lengthAreaR_-startRR[iRow];
      CoinSimplexInt numberInR = numberInColumnPlus[iRow];
      if (space>numberInR) {
	// there is space
	CoinBigIndex  put=startRR[iRow]+numberInR;
	numberInColumnPlus[iRow]=numberInR+1;
	indexRowRR[put]=pivotRow;
	elementRR[put]=value;
	//add 4 for luck
	if (next==maximumRowsExtra_)
	  startRR[maximumRowsExtra_] = CoinMin(static_cast<CoinBigIndex> (put + 4) ,lengthAreaR_);
      } else {
	// no space - do we shuffle?
	if (!getColumnSpaceIterateR(iRow,value,pivotRow)) {
	  // printf("Need more space for R\n");
	  numberInColumnPlus_.conditionalDelete();
	  numberInColumnPlusAddress_=NULL;
	  setNoGotRCopy();
	  regionSparse->clear();
#if ABC_SMALL<0
	  abort();
	  status=3;
#endif
	  break;
	}
      }
    }       
  } else {
#endif
    for (CoinBigIndex i = 0; i < numberNonZero; i++ ) {
      CoinSimplexInt iRow = regionIndex[i];
      elementR[putR] = region[iRow];
      region[iRow]=0.0;
      indexRowR[putR] = iRow;
      putR++;
    }       
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
    printf("=============================================start\n");
    for (int iRow=0;iRow<numberRows_;iRow++) {
      int number=scatter[iRow].number;
      CoinBigIndex offset = scatter[iRow].offset;
      CoinFactorizationDouble * COIN_RESTRICT area = elementUColumnPlus+offset;
      CoinSimplexInt * COIN_RESTRICT indices = reinterpret_cast<CoinSimplexInt *>(area+number);
      CoinSimplexInt * COIN_RESTRICT indices2 = indexRow+startColumnU[iRow];
      CoinFactorizationDouble * COIN_RESTRICT area2 = element+startColumnU[iRow];
      //assert (number==numberInColumn[iRow]);
#if ABC_USE_FUNCTION_POINTERS
      if (number<9)
	assert (scatter[iRow].functionPointer==AbcScatterLowSubtract[number]);
      else
	assert (scatter[iRow].functionPointer==AbcScatterHighSubtract[number&3]);
#endif
      if (number)
	printf("Row %d %d els\n",iRow,number);
      int n=0;
      for (int i=0;i<number;i++) {
	printf("(%d,%g) ",indices[i],area[i]);
	n++;
	if (n==10) {
	  n=0;
	  printf("\n");
	}
      }
      if (n)
	printf("\n");
#if 0
      for (int i=0;i<number;i++) {
	assert (indices[i]==indices2[i]);
	assert (area[i]==area2[i]);
      }
#endif
    }
    printf("=============================================end\n");
  }
#endif
#endif
  regionSparse->setNumElements(0);
  fromLongArray(regionSparse);
#if 0
  static int arrayF[6]={1,1,1,1,1,1};
  static int arrayB[6]={1,1,1,1,1,1};
  static int arrayF1[6]={1,1,1,1,1,1};
  static int arrayB1[6]={1,1,1,1,1,1};
  static int itry=0;
  {
    const CoinSimplexInt *  COIN_RESTRICT back = firstCountAddress_+numberRows_;
    const CoinSimplexInt *  COIN_RESTRICT pivotColumn = pivotColumnAddress_; // no
    const CoinSimplexInt *  COIN_RESTRICT permute = permuteAddress_;
    const CoinSimplexInt *  COIN_RESTRICT pivotLOrder = this->pivotLOrderAddress_;
    const CoinSimplexInt *  COIN_RESTRICT pivotLinkedForwards = this->pivotLinkedForwardsAddress_;//no
    const CoinSimplexInt *  COIN_RESTRICT pivotLinkedBackwards = this->pivotLinkedBackwardsAddress_;//no
    itry++;
    int testit[6];
    for (int i=0;i<6;i++) 
      testit[i]=1;
    for (int i=0;i<numberRows_;i++) {
      if (back[i]!=i)
	testit[0]=0;
      if (pivotColumn[i]!=i)
	testit[1]=0;
      if (permute[i]!=i)
	testit[2]=0;
      if (pivotLOrder[i]!=i)
	testit[3]=0;
      if (pivotLinkedForwards[i]!=i)
	testit[4]=0;
      if (pivotLinkedBackwards[i]!=i)
	testit[5]=0;
    }
    for (int i=0;i<6;i++) {
      if (!testit[i])
	arrayF[i]=0;
    }
    for (int i=0;i<6;i++) 
      testit[i]=1;
    for (int i=0;i<numberRows_;i++) {
      if (back[i]!=(numberRows_-1-i))
	testit[0]=0;
      if (pivotColumn[i]!=(numberRows_-1-i))
	testit[1]=0;
      if (permute[i]!=(numberRows_-1-i))
	testit[2]=0;
      if (pivotLOrder[i]!=(numberRows_-1-i))
	testit[3]=0;
      if (pivotLinkedForwards[i]!=(numberRows_-1-i))
	testit[4]=0;
      if (pivotLinkedBackwards[i]!=(numberRows_-1-i))
	testit[5]=0;
    }
    for (int i=0;i<6;i++) {
      if (!testit[i])
	arrayB[i]=0;
    }
    for (int i=0;i<6;i++) 
      testit[i]=1;
    for (int i=0;i<numberRows_-1;i++) {
      if (back[i]!=(i+1))
	testit[0]=0;
      if (pivotColumn[i]!=(i+1))
	testit[1]=0;
      if (permute[i]!=(i+1))
	testit[2]=0;
      if (pivotLOrder[i]!=(i+1))
	testit[3]=0;
      if (pivotLinkedForwards[i]!=(i+1))
	testit[4]=0;
      if (pivotLinkedBackwards[i]!=(i+1))
	testit[5]=0;
    }
    for (int i=0;i<6;i++) {
      if (!testit[i])
	arrayF1[i]=0;
    }
    for (int i=0;i<6;i++) 
      testit[i]=1;
    for (int i=0;i<numberRows_-1;i++) {
      if (back[i]!=pivotLinkedBackwards[i-1])
	testit[0]=0;
      if (pivotColumn[i]!=(numberRows_-i-2))
	testit[1]=0;
      if (permute[i]!=(numberRows_-i-2))
	testit[2]=0;
      if (pivotLOrder[i]!=(numberRows_-i-2))
	testit[3]=0;
      if (pivotLinkedForwards[i]!=(numberRows_-i-2))
	testit[4]=0;
      if (pivotLinkedBackwards[i]!=i-1)
	testit[5]=0;
    }
    for (int i=0;i<6;i++) {
      if (!testit[i])
	arrayB1[i]=0;
    }
    if ((itry%1000)==0) {
      for (int i=0;i<6;i++) {
	if (arrayF[i])
	  printf("arrayF[%d] still active\n",i);
	if (arrayB[i])
	  printf("arrayB[%d] still active\n",i);
	if (arrayF1[i])
	  printf("arrayF1[%d] still active\n",i);
	if (arrayB1[i])
	  printf("arrayB1[%d] still active\n",i);
      }
    }
  }
#endif 
}
/* Replaces one Column to basis,
   partial update in vector */
void 
CoinAbcTypeFactorization::replaceColumnPart3 ( const AbcSimplex * /*model*/,
					  CoinIndexedVector * regionSparse,
					       CoinIndexedVector * /*tableauColumn*/,
					       CoinIndexedVector * partialUpdate,
					  int pivotRow,
#ifdef ABC_LONG_FACTORIZATION 
					       long
#endif
					  double alpha )
{
  assert (numberU_<=numberRowsExtra_);
#ifndef ABC_USE_FUNCTION_POINTERS
  CoinBigIndex * COIN_RESTRICT startColumnU = startColumnUAddress_;
  CoinSimplexInt * COIN_RESTRICT numberInColumn = numberInColumnAddress_;
  CoinFactorizationDouble * COIN_RESTRICT elementU = elementUAddress_;
#endif
  CoinSimplexInt * COIN_RESTRICT indexRowR = indexRowRAddress_;
#if ABC_SMALL<2
  CoinSimplexInt * COIN_RESTRICT numberInRowU = numberInRowAddress_;
#endif
#if ABC_SMALL<2
  CoinSimplexInt * COIN_RESTRICT numberInColumnPlus = numberInColumnPlusAddress_;
#endif
  CoinSimplexInt *  COIN_RESTRICT permuteLookup = pivotColumnAddress_;
  CoinFactorizationDouble * COIN_RESTRICT region = denseVector(regionSparse);
  
  CoinSimplexInt numberInColumnU2 = partialUpdate->getNumElements();
  //take out old pivot column
  
#ifndef ABC_USE_FUNCTION_POINTERS
  totalElements_ -= numberInColumn[pivotRow];
  CoinBigIndex startU = startColumnU[numberRows_];
  startColumnU[pivotRow]=startU;
  CoinSimplexInt * COIN_RESTRICT indexU2 = &indexRowUAddress_[startU];
  CoinFactorizationDouble * COIN_RESTRICT elementU2 = &elementUAddress_[startU];
  numberInColumn[numberRows_]=numberInColumnU2;
#else
  scatterStruct * COIN_RESTRICT scatter = scatterUColumn();
#if ABC_USE_FUNCTION_POINTERS
  extern scatterUpdate AbcScatterLowSubtract[9];
  extern scatterUpdate AbcScatterHighSubtract[4];
#endif
  CoinFactorizationDouble *  COIN_RESTRICT elementUColumnPlus = elementUColumnPlusAddress_;
  CoinBigIndex startU = lastEntryByColumnUPlus_;
  totalElements_ -= scatter[pivotRow].number;
  CoinBigIndex saveStart = scatter[pivotRow].offset;
  CoinBigIndex saveEnd = scatter[pivotRow].number;
  scatter[pivotRow].offset=startU;

  scatter[numberRows_].number=numberInColumnU2;
  CoinFactorizationDouble * COIN_RESTRICT elementU2 = elementUColumnPlus+startU;
  CoinSimplexInt * COIN_RESTRICT indexU2 = reinterpret_cast<CoinSimplexInt *>(elementU2+numberInColumnU2);
#endif
  // move
  CoinSimplexInt * COIN_RESTRICT indexU2Save = partialUpdate->getIndices();
  CoinFactorizationDouble * COIN_RESTRICT elementU2Save = denseVector(partialUpdate);
  memcpy(indexU2,indexU2Save,numberInColumnU2*sizeof(CoinSimplexInt));
  for (int i=0;i<numberInColumnU2;i++) {
    int iRow=indexU2[i];
    elementU2[i]=elementU2Save[iRow];
    elementU2Save[iRow]=0.0;
  }
  //memcpy(elementU2,elementU2Save,numberInColumnU2*sizeof(double));
  //memset(elementU2Save,0,numberInColumnU2*sizeof(double));
  partialUpdate->setNumElements(0);
  //get entries in row (pivot not stored)
  CoinSimplexInt numberNonZero = 0;
#if ABC_SMALL<2
  CoinSimplexInt * COIN_RESTRICT indexColumnU = indexColumnUAddress_;
#endif
#if CONVERTROW
  CoinBigIndex * COIN_RESTRICT convertRowToColumn = convertRowToColumnUAddress_;
#if CONVERTROW>2
  CoinBigIndex * COIN_RESTRICT convertColumnToRow = convertColumnToRowUAddress_;
#endif
#endif
#if ABC_SMALL<2
  CoinFactorizationDouble * COIN_RESTRICT elementRowU = elementRowUAddress_;
  CoinBigIndex * COIN_RESTRICT startRowU = startRowUAddress_;
#endif
  CoinSimplexInt * COIN_RESTRICT regionIndex = regionSparse->getIndices (  );
  CoinFactorizationDouble pivotValue = 1.0;
  CoinSimplexInt status=0;
#ifndef ABC_USE_FUNCTION_POINTERS
  CoinSimplexInt *  COIN_RESTRICT indexRowU = indexRowUAddress_;
#endif
  CoinFactorizationDouble *  COIN_RESTRICT elementR = elementRAddress_;

#ifndef NDEBUG
#if CONVERTROW
#define CONVERTDEBUG
#endif
#endif
  int nInRow=9999999; // ? what if ABC_SMALL==0 or ABC_SMALL==1
#if ABC_SMALL<2
  CoinBigIndex start=0;
  CoinBigIndex end=0;
  if (gotUCopy()) {
    start=startRowU[pivotRow];
    nInRow=numberInRowU[pivotRow];
    end= start + nInRow;
  }
#endif
  CoinFactorizationDouble * COIN_RESTRICT pivotRegion = pivotRegionAddress_;
  numberNonZero = regionSparse->getNumElements (  );
  //CoinFactorizationDouble saveFromU = 0.0;

  instrument_do("CoinAbcFactorizationReplaceColumn",2*numberRows_+4*numberInColumnU2);
  pivotValue = 1.0 / alpha;
#if ABC_SMALL<2
  if (gotUCopy()) {
    for (CoinBigIndex i = start; i < end ; i ++ ) {
      CoinSimplexInt jColumn = indexColumnU[i];
#ifndef ABC_USE_FUNCTION_POINTERS
      CoinBigIndex startColumn = startColumnU[jColumn];
#if CONVERTROW
      CoinBigIndex j = convertRowToColumn[i];
#else
      CoinBigIndex j;
      int number = numberInColumn[jColumn];
      for (j=0;j<number;j++) {
	if (indexRowU[j+startColumn]==pivotRow)
	  break;
      }
      assert (j<number);
      //assert (j==convertRowToColumn[i]); // temp
#endif
#ifdef CONVERTDEBUG
      assert (fabs(elementU[j+startColumn]-elementRowU[i])<1.0e-4);
#endif      
#else
      int number = scatter[jColumn].number;
      CoinSimplexInt k = number-1;
      scatter[jColumn].number=k;
      CoinFactorizationDouble * COIN_RESTRICT area = elementUColumnPlus+scatter[jColumn].offset;
      CoinSimplexInt * COIN_RESTRICT indices = reinterpret_cast<CoinSimplexInt *>(area+k+1);
#if CONVERTROW
      CoinSimplexInt j = convertRowToColumn[i];
#else
      CoinSimplexInt j;
      for (j=0;j<number;j++) {
	if (indices[j]==pivotRow)
	  break;
      }
      assert (j<number);
      //assert (j==convertRowToColumn[i]); // temp
#endif
#ifdef CONVERTDEBUG
      assert (fabs(area[j]-elementRowU[i])<1.0e-4);
#endif      
#endif
      // swap
#ifndef ABC_USE_FUNCTION_POINTERS
      CoinSimplexInt k = numberInColumn[jColumn]-1;
      numberInColumn[jColumn]=k;
      CoinBigIndex k2 = k+startColumn;
      int kRow2=indexRowU[k2];
      indexRowU[j+startColumn]=kRow2;
      CoinFactorizationDouble value2=elementU[k2];
      elementU[j+startColumn]=value2;
#else
      int kRow2=indices[k];
      CoinFactorizationDouble value2=area[k];
#if ABC_USE_FUNCTION_POINTERS
      if (k<9) {
	scatter[jColumn].functionPointer=AbcScatterLowSubtract[k];
      } else {
	scatter[jColumn].functionPointer=AbcScatterHighSubtract[k&3];
      }
#endif
      // later more complicated swap to avoid move (don't think we can!)
      indices[j]=kRow2;
      area[j]=value2;
      // move 
      indices-=2;
      for (int i=0;i<k;i++)
	indices[i]=indices[i+2];
#endif
      // move in row copy (slow - as other remark (but shouldn't need to))
      CoinBigIndex start2 = startRowU[kRow2];
      int n=numberInRowU[kRow2];
      CoinBigIndex end2 = start2 + n;
#ifndef NDEBUG
      bool found=false;
#endif
      for (CoinBigIndex i2 = start2; i2 < end2 ; i2 ++ ) {
	CoinSimplexInt iColumn2 = indexColumnU[i2];
	if (jColumn==iColumn2) {
#if CONVERTROW
	  convertRowToColumn[i2] = j;
#endif
#ifndef NDEBUG
	  found=true;
#endif
	  break;
	}
      }
#ifndef NDEBUG
      assert(found);
#endif
    }
  } else {
#endif
#if ABC_SMALL>=0
    assert(nInRow!=9999999); // ? what if ABC_SMALL==0 or ABC_SMALL==1
    // no row copy
    CoinBigIndex COIN_RESTRICT * deletedPosition = reinterpret_cast<CoinBigIndex *>(elementR+lengthR_);
    CoinSimplexInt COIN_RESTRICT * deletedColumns = reinterpret_cast<CoinSimplexInt *>(indexRowR+lengthR_);
    for (CoinSimplexInt i = 0; i < nInRow ; i ++ ) {
      CoinBigIndex j = deletedPosition[i];
      CoinSimplexInt jColumn = deletedColumns[i];
      // swap
      CoinSimplexInt k = numberInColumn[jColumn]-1;
      numberInColumn[jColumn]=k;
      CoinBigIndex k2 = k+startColumnU[jColumn];
      int kRow2=indexRowU[k2];
      indexRowU[j]=kRow2;
      CoinFactorizationDouble value2=elementU[k2];
      elementU[j]=value2;
    }
#endif
#if ABC_SMALL<2
  }
#endif
 #if ABC_SMALL<2
  if (gotUCopy()) {
    // Now zero out column of U
    //take out old pivot column
#ifdef ABC_USE_FUNCTION_POINTERS
    CoinFactorizationDouble * COIN_RESTRICT elementU = elementUColumnPlus+saveStart;
    saveStart=0;
    CoinSimplexInt * COIN_RESTRICT indexRowU = reinterpret_cast<CoinSimplexInt *>(elementU+saveEnd);
#endif
    for (CoinBigIndex i = saveStart; i < saveEnd ; i ++ ) {
      //elementU[i] = 0.0;
      // If too slow then reverse meaning of convertRowToColumn and use
      int jRow=indexRowU[i];
      CoinBigIndex start = startRowU[jRow];
      CoinBigIndex end = start + numberInRowU[jRow];
      for (CoinBigIndex j = start; j < end ; j ++ ) {
	CoinSimplexInt jColumn = indexColumnU[j];
	if (jColumn==pivotRow) {
	  // swap
	  numberInRowU[jRow]--;
	  elementRowU[j]=elementRowU[end-1];
#if CONVERTROW
	  convertRowToColumn[j]=convertRowToColumn[end-1];
#endif
	  indexColumnU[j]=indexColumnU[end-1];
	  break;
	}
      }
    }    
  }
#endif   
  //zero out pivot Row (before or after?)
  //add to R
  CoinBigIndex * COIN_RESTRICT startColumnR = startColumnRAddress_;
  CoinBigIndex putR = lengthR_;
  CoinSimplexInt number = numberR_;
  
  startColumnR[number] = putR;  //for luck and first time
  number++;
  //assert (startColumnR+number-firstCountAddress_<( CoinMax(5*numberRows_,2*numberRows_+2*maximumPivots_)+2));
  startColumnR[number] = putR + numberNonZero;
  numberR_ = number;
  lengthR_ = putR + numberNonZero;
  totalElements_ += numberNonZero;

#if ABC_SMALL<2
  CoinSimplexInt *  COIN_RESTRICT nextRow=NULL;
  CoinSimplexInt *  COIN_RESTRICT lastRow=NULL;
  CoinSimplexInt next;
  CoinSimplexInt last;
  if (gotUCopy()) {
    //take out row
    nextRow = nextRowAddress_;
    lastRow = lastRowAddress_;
    next = nextRow[pivotRow];
    last = lastRow[pivotRow];
    
    nextRow[last] = next;
    lastRow[next] = last;
    numberInRowU[pivotRow]=0;
  }
#endif  
  //put in pivot
  //add row counts
  
  int n = 0;
  for (CoinSimplexInt i = 0; i < numberInColumnU2; i++ ) {
    CoinSimplexInt iRow = indexU2[i];
    CoinFactorizationDouble value=elementU2[i];
    assert (value);
    if ( !TEST_LESS_THAN_TOLERANCE(value ) ) {
      if ( iRow != pivotRow ) {
	//modify by pivot
	CoinFactorizationDouble value2 = value*pivotValue;
	indexU2[n]=iRow;
	elementU2[n++] = value2;
#if ABC_SMALL<2
	if (gotUCopy()) {
	  CoinSimplexInt next = nextRow[iRow];
	  CoinSimplexInt iNumberInRow = numberInRowU[iRow];
	  CoinBigIndex space;
	  CoinBigIndex put = startRowU[iRow] + iNumberInRow;
	  space = startRowU[next] - put;
	  if ( space <= 0 ) {
	    getRowSpaceIterate ( iRow, iNumberInRow + 4 );
	    put = startRowU[iRow] + iNumberInRow;
	  } else if (next==numberRows_) {
	    lastEntryByRowU_=put+1;
	  }     
	  indexColumnU[put] = pivotRow;
#if CONVERTROW
	  convertRowToColumn[put] = i;
#endif
	  elementRowU[put] = value2;
	  numberInRowU[iRow] = iNumberInRow + 1;
	}
#endif
      } else {
	//swap and save
	indexU2[i]=indexU2[numberInColumnU2-1];
	elementU2[i] = elementU2[numberInColumnU2-1];
	numberInColumnU2--;
	i--;
      }
    } else {
      // swap
      indexU2[i]=indexU2[numberInColumnU2-1];
      elementU2[i] = elementU2[numberInColumnU2-1];
      numberInColumnU2--;
      i--;
    }       
  }       
  numberInColumnU2=n;
  //do permute
  permuteAddress_[numberRowsExtra_] = pivotRow;
  //and for safety
  permuteAddress_[numberRowsExtra_ + 1] = 0;

  //pivotColumnAddress_[pivotRow] = numberRowsExtra_;
  
  numberU_++;

#if ABC_SMALL<2
  if (gotUCopy()) {
    //in at end
    last = lastRow[numberRows_];
    nextRow[last] = pivotRow; //numberRowsExtra_;
    lastRow[numberRows_] = pivotRow; //numberRowsExtra_;
    lastRow[pivotRow] = last;
    nextRow[pivotRow] = numberRows_;
    startRowU[pivotRow] = lastEntryByRowU_;
  }
#endif
#ifndef ABC_USE_FUNCTION_POINTERS
  numberInColumn[pivotRow]=numberInColumnU2;
#else
#if ABC_USE_FUNCTION_POINTERS
  if (numberInColumnU2<9) {
    scatter[pivotRow].functionPointer=AbcScatterLowSubtract[numberInColumnU2];
  } else {
    scatter[pivotRow].functionPointer=AbcScatterHighSubtract[numberInColumnU2&3];
  }
#endif
  assert(scatter[pivotRow].offset==lastEntryByColumnUPlus_);
  //CoinFactorizationDouble * COIN_RESTRICT area = elementUColumnPlus+lastEntryByColumnUPlus_;
  //CoinSimplexInt * COIN_RESTRICT indices = reinterpret_cast<CoinSimplexInt *>(area+numberInColumnU2);
  if (numberInColumnU2<scatter[numberRows_].number) {
    int offset=2*(scatter[numberRows_].number-numberInColumnU2);
    indexU2 -= offset;
    //assert(indexU2-reinterpret_cast<int*>(area)>=0);
    for (int i=0;i<numberInColumnU2;i++)
      indexU2[i]=indexU2[i+offset];
  }
  scatter[pivotRow].number=numberInColumnU2;
  //CoinAbcMemcpy(indices,indexU2,numberInColumnU2);
  //CoinAbcMemcpy(area,elementU2,numberInColumnU2);
  lastEntryByColumnUPlus_ += (3*numberInColumnU2+1)>>1;
#endif
  totalElements_ += numberInColumnU2;
  lengthU_ += numberInColumnU2;
#if ABC_SMALL<2
  CoinSimplexInt * COIN_RESTRICT nextColumn = nextColumnAddress_;
  CoinSimplexInt * COIN_RESTRICT lastColumn = lastColumnAddress_;
  if (gotRCopy()) {
    //column in at beginning (as empty)
    next = nextColumn[maximumRowsExtra_];
    lastColumn[next] = numberRowsExtra_;
    nextColumn[maximumRowsExtra_] = numberRowsExtra_;
    nextColumn[numberRowsExtra_] = next;
    lastColumn[numberRowsExtra_] = maximumRowsExtra_;
  }
#endif
#if 0
  {
    int k=-1;
    for (int i=0;i<numberRows_;i++) {
      k=CoinMax(k,startRowU[i]+numberInRowU[i]);
    }
    assert (k==lastEntryByRowU_);
  }
#endif
  //pivotRegion[numberRowsExtra_] = pivotValue;
  pivotRegion[pivotRow] = pivotValue;
#ifndef ABC_USE_FUNCTION_POINTERS
  maximumU_ = CoinMax(maximumU_,startU+numberInColumnU2);
#else
  maximumU_ = CoinMax(maximumU_,lastEntryByColumnUPlus_);
#endif
  permuteLookup[pivotRow]=numberRowsExtra_;
  permuteLookup[numberRowsExtra_]=pivotRow;
  //pivotColumnAddress_[pivotRow]=numberRowsExtra_;
  //pivotColumnAddress_[numberRowsExtra_]=pivotRow;
  assert (pivotRow<numberRows_);
  // out of chain
  CoinSimplexInt iLast=pivotLinkedBackwardsAddress_[pivotRow];
  CoinSimplexInt iNext=pivotLinkedForwardsAddress_[pivotRow];
  assert (pivotRow==pivotLinkedForwardsAddress_[iLast]);
  assert (pivotRow==pivotLinkedBackwardsAddress_[iNext]);
  pivotLinkedForwardsAddress_[iLast]=iNext;
  pivotLinkedBackwardsAddress_[iNext]=iLast;
  if (pivotRow==lastSlack_) {
    lastSlack_ = iLast;
  }
  iLast=pivotLinkedBackwardsAddress_[numberRows_];
  assert (numberRows_==pivotLinkedForwardsAddress_[iLast]);
  pivotLinkedForwardsAddress_[iLast]=pivotRow;
  pivotLinkedBackwardsAddress_[pivotRow]=iLast;
  pivotLinkedBackwardsAddress_[numberRows_]=pivotRow;
  pivotLinkedForwardsAddress_[pivotRow]=numberRows_;
  assert (numberRows_>pivotLinkedForwardsAddress_[-1]);
  assert (pivotLinkedBackwardsAddress_[numberRows_]>=0);
  numberRowsExtra_++;
  numberGoodU_++;
  numberPivots_++;
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
#if ABC_SMALL<2
  if (gotRCopy()&&status<2) {
    //if (numberInColumnPlus&&status<2) {
    // we are going to put another copy of R in R
    CoinFactorizationDouble * COIN_RESTRICT elementRR = elementRAddress_ + lengthAreaR_;
    CoinSimplexInt * COIN_RESTRICT indexRowRR = indexRowRAddress_ + lengthAreaR_;
    CoinBigIndex * COIN_RESTRICT startRR = startColumnRAddress_+maximumPivots_+1;
    CoinSimplexInt pivotRow = numberRowsExtra_-1;
    for (CoinBigIndex i = 0; i < numberNonZero; i++ ) {
      CoinSimplexInt jRow = regionIndex[i];
      CoinFactorizationDouble value=region[jRow];
      elementR[putR] = value;
      indexRowR[putR] = jRow;
      putR++;
      region[jRow]=0.0;
      int iRow = permuteLookup[jRow];
      next = nextColumn[iRow];
      CoinBigIndex space;
      if (next!=maximumRowsExtra_)
	space = startRR[next]-startRR[iRow];
      else
	space = lengthAreaR_-startRR[iRow];
      CoinSimplexInt numberInR = numberInColumnPlus[iRow];
      if (space>numberInR) {
	// there is space
	CoinBigIndex  put=startRR[iRow]+numberInR;
	numberInColumnPlus[iRow]=numberInR+1;
	indexRowRR[put]=pivotRow;
	elementRR[put]=value;
	//add 4 for luck
	if (next==maximumRowsExtra_)
	  startRR[maximumRowsExtra_] = CoinMin(static_cast<CoinBigIndex> (put + 4) ,lengthAreaR_);
      } else {
	// no space - do we shuffle?
	if (!getColumnSpaceIterateR(iRow,value,pivotRow)) {
	  // printf("Need more space for R\n");
	  numberInColumnPlus_.conditionalDelete();
	  numberInColumnPlusAddress_=NULL;
	  setNoGotRCopy();
	  regionSparse->clear();
#if ABC_SMALL<0
	  abort();
	  status=3;
#endif
	  break;
	}
      }
    }       
  } else {
#endif
    for (CoinBigIndex i = 0; i < numberNonZero; i++ ) {
      CoinSimplexInt iRow = regionIndex[i];
      elementR[putR] = region[iRow];
      region[iRow]=0.0;
      indexRowR[putR] = iRow;
      putR++;
    }       
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
    printf("=============================================start\n");
    for (int iRow=0;iRow<numberRows_;iRow++) {
      int number=scatter[iRow].number;
      CoinBigIndex offset = scatter[iRow].offset;
      CoinFactorizationDouble * COIN_RESTRICT area = elementUColumnPlus+offset;
      CoinSimplexInt * COIN_RESTRICT indices = reinterpret_cast<CoinSimplexInt *>(area+number);
      CoinSimplexInt * COIN_RESTRICT indices2 = indexRow+startColumnU[iRow];
      CoinFactorizationDouble * COIN_RESTRICT area2 = element+startColumnU[iRow];
      //assert (number==numberInColumn[iRow]);
#if ABC_USE_FUNCTION_POINTERS
      if (number<9)
	assert (scatter[iRow].functionPointer==AbcScatterLowSubtract[number]);
      else
	assert (scatter[iRow].functionPointer==AbcScatterHighSubtract[number&3]);
#endif
      if (number)
	printf("Row %d %d els\n",iRow,number);
      int n=0;
      for (int i=0;i<number;i++) {
	printf("(%d,%g) ",indices[i],area[i]);
	n++;
	if (n==10) {
	  n=0;
	  printf("\n");
	}
      }
      if (n)
	printf("\n");
#if 0
      for (int i=0;i<number;i++) {
	assert (indices[i]==indices2[i]);
	assert (area[i]==area2[i]);
      }
#endif
    }
    printf("=============================================end\n");
  }
#endif
#endif
  regionSparse->setNumElements(0);
}
#if ABC_SMALL>=0
/* Combines BtranU and store which elements are to be deleted
 */
int 
CoinAbcTypeFactorization::replaceColumnU ( CoinIndexedVector * regionSparse,
				   CoinBigIndex * deletedPosition,
				   CoinSimplexInt * deletedColumns,
				   CoinSimplexInt pivotRow)
{
  instrument_start("CoinAbcFactorizationReplaceColumnU",numberRows_);
  CoinSimplexDouble * COIN_RESTRICT region = regionSparse->denseVector();
  int * COIN_RESTRICT regionIndex = regionSparse->getIndices();
  const CoinBigIndex COIN_RESTRICT *startColumn = startColumnUAddress_;
  const CoinSimplexInt COIN_RESTRICT *indexRow = indexRowUAddress_;
  CoinFactorizationDouble COIN_RESTRICT *element = elementUAddress_;
  int numberNonZero = 0;
  const CoinSimplexInt COIN_RESTRICT *numberInColumn = numberInColumnAddress_;
  int nPut=0;
  const CoinSimplexInt *  COIN_RESTRICT pivotLinked = pivotLinkedForwardsAddress_;
  CoinSimplexInt jRow=pivotLinked[-1];
  while (jRow!=numberRows_) {
    assert (!region[jRow]);
    CoinFactorizationDouble pivotValue = 0.0;
    instrument_add(numberInColumn[jRow]);
    for (CoinBigIndex  j= startColumn[jRow] ; 
	 j < startColumn[jRow]+numberInColumn[jRow]; j++ ) {
      int iRow = indexRow[j];
      CoinFactorizationDouble value = element[j];
      if (iRow!=pivotRow) {
	pivotValue -= value * region[iRow];
      } else {
	assert (!region[iRow]);
	pivotValue += value;
	deletedColumns[nPut]=jRow;
	deletedPosition[nPut++]=j;
      }
    }       
    if ( !TEST_LESS_THAN_TOLERANCE_REGISTER( pivotValue ) ) {
      regionIndex[numberNonZero++] = jRow;
      region[jRow] = pivotValue;
    } else {
      region[jRow] = 0;
    }
    jRow=pivotLinked[jRow];
  }
  regionSparse->setNumElements(numberNonZero);
  instrument_end();
  return nPut;
}
/* Updates part of column transpose (BTRANU) by column
   assumes index is sorted i.e. region is correct */
void 
CoinAbcTypeFactorization::updateColumnTransposeUByColumn ( CoinIndexedVector * regionSparse,
						   CoinSimplexInt smallestIndex) const
{
  instrument_start("CoinAbcFactorizationUpdateTransposeUByColumn",numberRows_);
  CoinSimplexDouble * COIN_RESTRICT region = regionSparse->denseVector();
  const CoinBigIndex COIN_RESTRICT *startColumn = startColumnUAddress_;
  const CoinSimplexInt COIN_RESTRICT *indexRow = indexRowUAddress_;
  const CoinFactorizationDouble COIN_RESTRICT *element = elementUAddress_;
#if ABC_SMALL<3
  int numberNonZero = 0;
  int * COIN_RESTRICT regionIndex = regionSparse->getIndices();
#endif
  const CoinSimplexInt COIN_RESTRICT *numberInColumn = numberInColumnAddress_;
  //const CoinFactorizationDouble COIN_RESTRICT *pivotRegion = pivotRegionAddress_;
  const CoinSimplexInt *  COIN_RESTRICT pivotLinked = pivotLinkedForwardsAddress_;
  CoinSimplexInt jRow=smallestIndex;
  while (jRow!=numberRows_) {
    CoinFactorizationDouble pivotValue = region[jRow];
    CoinBigIndex start=startColumn[jRow];
    instrument_add(numberInColumn[jRow]);
#ifndef INLINE_IT2
    for (CoinBigIndex  j= start ; j < start+numberInColumn[jRow]; j++ ) {
      int iRow = indexRow[j];
      CoinFactorizationDouble value = element[j];
      pivotValue -= value * region[iRow];
    }
#else
    pivotValue+=CoinAbcGatherUpdate(numberInColumn[jRow],element+start,indexRow+start,region);
#endif       
#if ABC_SMALL<3
    if ( !TEST_LESS_THAN_TOLERANCE_REGISTER( pivotValue )  ) {
      regionIndex[numberNonZero++] = jRow;
      region[jRow] = pivotValue;
    } else {
      region[jRow] = 0;
    }
#else
      region[jRow] = pivotValue;
#endif       
    jRow=pivotLinked[jRow];
  }
#if ABC_SMALL<3
  regionSparse->setNumElements(numberNonZero);
#endif
  instrument_end();
}
#endif
//  updateColumnTranspose.  Updates one column transpose (BTRAN)
CoinSimplexInt
CoinAbcTypeFactorization::updateColumnTranspose ( CoinIndexedVector & regionSparse) const
{
  toLongArray(&regionSparse,0);
  CoinFactorizationDouble * COIN_RESTRICT region = denseVector(regionSparse);
  CoinSimplexInt * COIN_RESTRICT regionIndex = regionSparse.getIndices();
  CoinSimplexInt numberNonZero = regionSparse.getNumElements();
  const CoinFactorizationDouble * COIN_RESTRICT pivotRegion = pivotRegionAddress_;

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
#if 0
  static int xxxxxx=0;
  xxxxxx++;
  if (xxxxxx%20==0) {
    int smallestK=-1;
    int largestK=-1;
    //const CoinBigIndex COIN_RESTRICT *startColumn = startColumnUAddress_;
    CoinSimplexInt *  COIN_RESTRICT pivotRowForward = pivotColumnAddress_;
    const CoinSimplexInt COIN_RESTRICT *numberInColumn = numberInColumnAddress_;
    for (CoinSimplexInt k = numberRows_-1 ; k >= 0; k-- ) {
      CoinSimplexInt i=pivotRowForward[k];
      if (numberInColumn[i]) {
	if (largestK<0)
	  largestK=k;
	smallestK=k;
      }
    }
    printf("UByCol %d slacks largestK %d smallestK %d\n",numberSlacks_,largestK,smallestK);
    //const CoinBigIndex * COIN_RESTRICT startRow = startRowUAddress_;
    const CoinSimplexInt * COIN_RESTRICT numberInRow = numberInRowAddress_;
    smallestK=-1;
    largestK=-1;
    for (CoinSimplexInt k = numberRows_-1 ; k >= 0; k-- ) {
      CoinSimplexInt i=pivotRowForward[k];
      if (numberInRow[i]) {
	if (largestK<0)
	  largestK=k;
	smallestK=k;
      }
    }
    printf("and ByRow largestK %d smallestK %d\n",largestK,smallestK);
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
		    ,0
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
				      ,0
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
  fromLongArray(static_cast<int>(0));
  return numberNonZero;
}
#if ABC_SMALL<2

/* Updates part of column transpose (BTRANU) when densish,
   assumes index is sorted i.e. region is correct */
void 
CoinAbcTypeFactorization::updateColumnTransposeUDensish 
                        ( CoinIndexedVector * regionSparse,
			  CoinSimplexInt smallestIndex) const
{
  CoinFactorizationDouble * COIN_RESTRICT region = denseVector(regionSparse);
  CoinSimplexInt numberNonZero = regionSparse->getNumElements (  );
  
  CoinSimplexInt * COIN_RESTRICT regionIndex = regionSparse->getIndices (  );
  
  const CoinBigIndex * COIN_RESTRICT startRow = startRowUAddress_;
  const CoinSimplexInt * COIN_RESTRICT indexColumn = indexColumnUAddress_;
  const CoinSimplexInt * COIN_RESTRICT numberInRow = numberInRowAddress_;
  
  const CoinFactorizationDouble * COIN_RESTRICT elementRowU = elementRowUAddress_;
  
  numberNonZero = 0;
  const CoinSimplexInt *  COIN_RESTRICT pivotLinked = pivotLinkedForwardsAddress_;
  CoinSimplexInt jRow=smallestIndex ; //pivotLinked[-1];
  instrument_start("CoinAbcFactorizationUpdateTransposeUDensish",numberRows_);
  while (jRow>=0) {
    if (TEST_DOUBLE_NONZERO(region[jRow])) {
      CoinFactorizationDouble pivotValue = region[jRow];
      if ( !TEST_LESS_THAN_UPDATE_TOLERANCE(pivotValue )  ) {
	CoinSimplexInt numberIn = numberInRow[jRow];
	instrument_add(numberIn);
	if (TEST_INT_NONZERO(numberIn)) {
	  CoinBigIndex start = startRow[jRow];
	  CoinBigIndex end = start + numberIn;
#ifndef INLINE_IT
	  for (CoinBigIndex j = start ; j < end; j ++ ) {
	    CoinSimplexInt iRow = indexColumn[j];
	    CoinFactorizationDouble value = elementRowU[j];
	    assert (value);
	    // i<iRow
	    region[iRow] -=  value * pivotValue;
	  }
#else
	  CoinAbcScatterUpdate(end-start,pivotValue,elementRowU+start,indexColumn+start,region);
#endif    
	} 
	regionIndex[numberNonZero++] = jRow;
      } else {
	region[jRow] = 0.0;
      }       
    }
    jRow=pivotLinked[jRow];
  }
  //set counts
  regionSparse->setNumElements ( numberNonZero );
  instrument_end();
}
/* Updates part of column transpose (BTRANU) when sparse,
   assumes index is sorted i.e. region is correct */
void 
CoinAbcTypeFactorization::updateColumnTransposeUSparse ( 
							CoinIndexedVector * regionSparse
#if ABC_PARALLEL
						,int whichSparse
#endif
							 ) const
{
  CoinFactorizationDouble * COIN_RESTRICT region = denseVector(regionSparse);
  CoinSimplexInt numberNonZero = regionSparse->getNumElements (  );
  
  CoinSimplexInt * COIN_RESTRICT regionIndex = regionSparse->getIndices (  );
  const CoinBigIndex * COIN_RESTRICT startRow = startRowUAddress_;
  const CoinSimplexInt * COIN_RESTRICT indexColumn = indexColumnUAddress_;
  const CoinSimplexInt * COIN_RESTRICT numberInRow = numberInRowAddress_;
  
  const CoinFactorizationDouble * COIN_RESTRICT elementRowU = elementRowUAddress_;
  
  // use sparse_ as temporary area
  // mark known to be zero
  //printf("PP 0_ %d %s\n",__LINE__,__FILE__);
  CoinAbcStack * COIN_RESTRICT stackList = reinterpret_cast<CoinAbcStack *>(sparseAddress_);
  CoinSimplexInt * COIN_RESTRICT list = listAddress_;
  CoinCheckZero * COIN_RESTRICT mark = markRowAddress_;
#if ABC_PARALLEL
  //printf("PP %d %d %s\n",whichSparse,__LINE__,__FILE__);
  if (whichSparse) {
    //printf("alternative sparse\n");
    int addAmount=whichSparse*sizeSparseArray_;
    stackList=reinterpret_cast<CoinAbcStack *>(reinterpret_cast<char *>(stackList)+addAmount);
    list=reinterpret_cast<CoinSimplexInt *>(reinterpret_cast<char *>(list)+addAmount);
    mark=reinterpret_cast<CoinCheckZero *>(reinterpret_cast<char *>(mark)+addAmount);
  }
#endif
  CoinSimplexInt nList;
#if 0 //ndef NDEBUG
  for (CoinSimplexInt i=0;i<numberRows_;i++) {
#if 0
    assert (!mark[i]);
#else
    if (mark[i]) {
      printf("HHHHHHHHHHelp %d %d\n",i,mark[i]);
      usleep(10000000);
    }
#endif
  }
#endif
  nList=0;
  for (CoinSimplexInt k=0;k<numberNonZero;k++) {
    CoinSimplexInt kPivot=regionIndex[k];
    stackList[0].stack=kPivot;
    CoinSimplexInt jPivot=kPivot;
    CoinBigIndex start=startRow[jPivot];
    stackList[0].next=start+numberInRow[jPivot]-1;
    stackList[0].start=start;
    CoinSimplexInt nStack=1;
    while (nStack) {
      /* take off stack */
      kPivot=stackList[--nStack].stack;
      if (mark[kPivot]!=1) {
#if 0
	CoinBigIndex j=stackList[nStack].next;
	CoinBigIndex startThis=stackList[nStack].start;
	for (;j>=startThis;j--) {
	  CoinSimplexInt kPivot3=indexColumn[j--];
	  if (!mark[kPivot3]) {
	    kPivot=kPivot3;
	    break;
	  }
	}
	if (j>=startThis) {
	  /* put back on stack */
	  stackList[nStack++].next =j;
	  /* and new one */
	  CoinBigIndex start=startRow[kPivot];
	  j=start+numberInRow[kPivot]-1;
	  stackList[nStack].stack=kPivot;
	  stackList[nStack].start=start;
	  mark[kPivot]=2;
	  stackList[nStack++].next=j;
#else
	CoinBigIndex j=stackList[nStack].next;
	if (j>=stackList[nStack].start) {
	  CoinSimplexInt kPivot3=indexColumn[j--];
	  /* put back on stack */
	  stackList[nStack++].next =j;
	  if (!mark[kPivot3]) {
	    /* and new one */
	    CoinBigIndex start=startRow[kPivot3];
	    j=start+numberInRow[kPivot3]-1;
	    stackList[nStack].stack=kPivot3;
	    stackList[nStack].start=start;
	    mark[kPivot3]=2;
	    stackList[nStack++].next=j;
	  }
#endif
	} else {
	  // finished
	  list[nList++]=kPivot;
	  mark[kPivot]=1;
	}
      }
    }
  }
  instrument_start("CoinAbcFactorizationUpdateTransposeUSparse",2*(numberNonZero+nList));
  numberNonZero=0;
  for (CoinSimplexInt i=nList-1;i>=0;i--) {
    CoinSimplexInt iPivot = list[i];
    assert (iPivot>=0&&iPivot<numberRows_);
    mark[iPivot]=0;
    CoinFactorizationDouble pivotValue = region[iPivot];
    if ( !TEST_LESS_THAN_UPDATE_TOLERANCE(pivotValue )  ) {
      CoinSimplexInt numberIn = numberInRow[iPivot];
      instrument_add(numberIn);
      if (TEST_INT_NONZERO(numberIn)) {
	CoinBigIndex start = startRow[iPivot];
#ifndef INLINE_IT
	CoinBigIndex end = start + numberIn;
	for (CoinBigIndex j=start ; j < end; j ++ ) {
	  CoinSimplexInt iRow = indexColumn[j];
	  CoinFactorizationDouble value = elementRowU[j];
	  region[iRow] -= value * pivotValue;
	}
#else
	CoinAbcScatterUpdate(numberIn,pivotValue,elementRowU+start,indexColumn+start,region);
#endif  
      }   
      regionIndex[numberNonZero++] = iPivot;
    } else {
      region[iPivot] = 0.0;
    }       
  }       
  //set counts
  regionSparse->setNumElements ( numberNonZero );
  instrument_end_and_adjust(1.3);
}
#endif
//  updateColumnTransposeU.  Updates part of column transpose (BTRANU)
//assumes index is sorted i.e. region is correct
//does not sort by sign
void
CoinAbcTypeFactorization::updateColumnTransposeU ( CoinIndexedVector * regionSparse,
						   CoinSimplexInt smallestIndex
#if ABC_SMALL<2
		       , CoinAbcStatistics & statistics
#endif
#if ABC_PARALLEL
		    ,int whichCpu
#endif
						   ) const
{
#if CILK_CONFLICT>0
#if ABC_PARALLEL
  // for conflicts
#if CILK_CONFLICT>1
  printf("file %s line %d which %d\n",__FILE__,__LINE__,whichCpu);
#endif
  abc_assert((cilk_conflict&(1<<whichCpu))==0);
  cilk_conflict |= (1<<whichCpu);
#else
  abc_assert((cilk_conflict&(1<<0))==0);
  cilk_conflict |= (1<<0);
#endif
#endif
#if ABC_SMALL<2
  CoinSimplexInt number = regionSparse->getNumElements (  );
  if (factorizationStatistics()) {
    statistics.numberCounts_++;
    statistics.countInput_ += static_cast<CoinSimplexDouble> (number);
  }
  CoinSimplexInt goSparse;
  // Guess at number at end
  if (gotUCopy()) {
    if (gotSparse()) {
      assert (statistics.averageAfterU_);
      CoinSimplexInt newNumber = static_cast<CoinSimplexInt> (number*statistics.averageAfterU_*twiddleBtranFactor1());
      if (newNumber< sparseThreshold_)
	goSparse = 2;
      else
	goSparse = 0;
    } else {
      goSparse=0;
    }
#if ABC_SMALL>=0
  } else {
    goSparse=-1;
#endif
  }
  //CoinIndexedVector temp(*regionSparse);
  switch (goSparse) {
#if ABC_SMALL>=0
  case -1: // no row copy
    updateColumnTransposeUByColumn(regionSparse,smallestIndex);
    break;
#endif
  case 0: // densish
    updateColumnTransposeUDensish(regionSparse,smallestIndex);
    break;
  case 2: // sparse
    {
      updateColumnTransposeUSparse(regionSparse
#if ABC_PARALLEL
				      ,whichCpu
#endif
				   );
    }
    break;
  }
  if (factorizationStatistics()) 
    statistics.countAfterU_ += static_cast<CoinSimplexDouble> (regionSparse->getNumElements());
#else
  updateColumnTransposeUByColumn(regionSparse,smallestIndex);
#endif
#if CILK_CONFLICT>0
#if ABC_PARALLEL
  // for conflicts
  abc_assert((cilk_conflict&(1<<whichCpu))!=0);
  cilk_conflict &= ~(1<<whichCpu);
#else
  abc_assert((cilk_conflict&(1<<0))!=0);
  cilk_conflict &= ~(1<<0);
#endif
#endif
}

/*  updateColumnTransposeLDensish.  
    Updates part of column transpose (BTRANL) dense by column */
void
CoinAbcTypeFactorization::updateColumnTransposeLDensish 
     ( CoinIndexedVector * regionSparse ) const
{
  CoinFactorizationDouble * COIN_RESTRICT region = denseVector(regionSparse);
  CoinSimplexInt base;
  CoinSimplexInt first = -1;
  const CoinSimplexInt * COIN_RESTRICT pivotLOrder = pivotLOrderAddress_;
  //const CoinSimplexInt * COIN_RESTRICT pivotLBackwardOrder = permuteAddress_;
  
#if ABC_SMALL<3
  CoinSimplexInt * COIN_RESTRICT regionIndex = regionSparse->getIndices (  );
  CoinSimplexInt numberNonZero=0;
#endif
  //scan
  for (first=numberRows_-1;first>=0;first--) {
#ifndef ABC_ORDERED_FACTORIZATION
    if (region[pivotLOrder[first]]) 
      break;
#else
    if (region[first]) 
      break;
#endif
  }
  instrument_start("CoinAbcFactorizationUpdateTransposeLDensish",first);
  if ( first >= 0 ) {
    base = baseL_;
    const CoinBigIndex * COIN_RESTRICT startColumn = startColumnLAddress_;
    const CoinSimplexInt * COIN_RESTRICT indexRow = indexRowLAddress_;
    const CoinFactorizationDouble * COIN_RESTRICT element = elementLAddress_;
    CoinSimplexInt last = baseL_ + numberL_;
    
    if ( first >= last ) {
      first = last - 1;
    } 
    CoinBigIndex end = startColumn[first+1];
    for (CoinSimplexInt k = first ; k >= base; k-- ) {
#ifndef ABC_ORDERED_FACTORIZATION
      CoinSimplexInt i=pivotLOrder[k];
#else
      CoinSimplexInt i=k;
#endif
      CoinFactorizationDouble pivotValue = region[i];
#ifndef INLINE_IT2
      CoinBigIndex j=end-1;
      end=startColumn[k];
      instrument_add(j-end);
      for (  ; j >= end; j-- ) {
	CoinSimplexInt iRow = indexRow[j];
	CoinFactorizationDouble value = element[j];
	pivotValue -= value * region[iRow];
      }       
#else
      CoinBigIndex start=startColumn[k];
      instrument_add(end-start);
      pivotValue+=CoinAbcGatherUpdate(end-start,element+start,indexRow+start,region);
      end=start;
#endif
#if ABC_SMALL<3
      if ( !TEST_LESS_THAN_TOLERANCE_REGISTER(pivotValue )  ) {
	region[i] = pivotValue;
	regionIndex[numberNonZero++] = i;
      } else { 
	region[i] = 0.0;
      }
#else
	region[i] = pivotValue;
#endif       
    }       
#if ABC_SMALL<3
    //may have stopped early
    if ( first < base ) {
      base = first + 1;
    }
    
    for (CoinSimplexInt k = base-1 ; k >= 0; k-- ) {
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
#endif
  } 
#if ABC_SMALL<3
  //set counts
  regionSparse->setNumElements ( numberNonZero );
#endif
  instrument_end();
}
#if ABC_SMALL<2
/*  updateColumnTransposeLByRow. 
    Updates part of column transpose (BTRANL) densish but by row */
void
CoinAbcTypeFactorization::updateColumnTransposeLByRow 
    ( CoinIndexedVector * regionSparse ) const
{
  CoinFactorizationDouble * COIN_RESTRICT region = denseVector(regionSparse);
  CoinSimplexInt * COIN_RESTRICT regionIndex = regionSparse->getIndices (  );
  CoinSimplexInt numberNonZero;
  
  // use row copy of L
  const CoinFactorizationDouble *  COIN_RESTRICT element = elementByRowLAddress_;
  const CoinBigIndex *  COIN_RESTRICT startRow = startRowLAddress_;
  const CoinSimplexInt *  COIN_RESTRICT column = indexColumnLAddress_;
  const CoinSimplexInt * COIN_RESTRICT pivotLOrder = pivotLOrderAddress_;
  numberNonZero=0;
  instrument_start("CoinAbcFactorizationUpdateTransposeLByRow",numberRows_+numberSlacks_);
  // down to slacks
  for (CoinSimplexInt k = numberRows_-1 ; k >= numberSlacks_; k-- ) {
#ifndef ABC_ORDERED_FACTORIZATION
    CoinSimplexInt i=pivotLOrder[k];
#else
    CoinSimplexInt i=k;
#endif
    if (TEST_DOUBLE_NONZERO(region[i])) {
      CoinFactorizationDouble pivotValue = region[i];
      if ( !TEST_LESS_THAN_TOLERANCE(pivotValue )  ) {
	regionIndex[numberNonZero++] = i;
	CoinBigIndex start=startRow[i];
	CoinBigIndex end=startRow[i+1];
	instrument_add(end-start);
	if (TEST_INT_NONZERO(end-start)) {
#ifndef INLINE_IT
	  for (CoinBigIndex j = end-1;j >= start; j--) {
	    CoinSimplexInt iRow = column[j];
	    CoinFactorizationDouble value = element[j];
	    region[iRow] -= pivotValue*value;
	  }
#else
	  CoinAbcScatterUpdate(end-start,pivotValue,element+start,column+start,region);
#endif
	}
      } else {
	region[i] = 0.0;
      }     
    }
  }
  for (CoinSimplexInt k = numberSlacks_-1 ; k >= 0; k-- ) {
#ifndef ABC_ORDERED_FACTORIZATION
    CoinSimplexInt i=pivotLOrder[k];
#else
    CoinSimplexInt i=k;
#endif
    CoinExponent expValue=ABC_EXPONENT(region[i]);
    if (expValue) {
      if (!TEST_EXPONENT_LESS_THAN_TOLERANCE(expValue)) {
	regionIndex[numberNonZero++]=i;
      } else {
	region[i] = 0.0;
      }     
    }
  }
  //set counts
  regionSparse->setNumElements ( numberNonZero );
  instrument_end();
}
/*  updateColumnTransposeLSparse. 
    Updates part of column transpose (BTRANL) sparse */
void
CoinAbcTypeFactorization::updateColumnTransposeLSparse ( CoinIndexedVector * regionSparse 
#if ABC_PARALLEL
						,int whichSparse
#endif
							 ) const
{
  CoinFactorizationDouble * COIN_RESTRICT region = denseVector(regionSparse);
  CoinSimplexInt * COIN_RESTRICT regionIndex = regionSparse->getIndices (  );
  CoinSimplexInt numberNonZero = regionSparse->getNumElements (  );
  
  // use row copy of L
  const CoinFactorizationDouble *  COIN_RESTRICT element = elementByRowLAddress_;
  const CoinBigIndex *  COIN_RESTRICT startRow = startRowLAddress_;
  const CoinSimplexInt *  COIN_RESTRICT column = indexColumnLAddress_;
  // use sparse_ as temporary area
  // mark known to be zero
  //printf("PP 0_ %d %s\n",__LINE__,__FILE__);
  CoinAbcStack * COIN_RESTRICT stackList = reinterpret_cast<CoinAbcStack *>(sparseAddress_);
  CoinSimplexInt * COIN_RESTRICT list = listAddress_;
  CoinCheckZero * COIN_RESTRICT mark = markRowAddress_;
#if ABC_PARALLEL
  //printf("PP %d %d %s\n",whichSparse,__LINE__,__FILE__);
  if (whichSparse) {
    int addAmount=whichSparse*sizeSparseArray_;
    //printf("alternative sparse\n");
    stackList=reinterpret_cast<CoinAbcStack *>(reinterpret_cast<char *>(stackList)+addAmount);
    list=reinterpret_cast<CoinSimplexInt *>(reinterpret_cast<char *>(list)+addAmount);
    mark=reinterpret_cast<CoinCheckZero *>(reinterpret_cast<char *>(mark)+addAmount);
  }
#endif
#ifndef NDEBUG
  for (CoinSimplexInt i=0;i<numberRows_;i++) {
#if 0
    assert (!mark[i]);
#else
    if (mark[i]) {
      printf("HHHHHHHHHHelp2 %d %d\n",i,mark[i]);
      usleep(10000000);
    }
#endif
  }
#endif
  CoinSimplexInt nList;
  CoinSimplexInt number = numberNonZero;
  nList=0;
  for (CoinSimplexInt k=0;k<number;k++) {
    CoinSimplexInt kPivot=regionIndex[k];
    //CoinSimplexInt iPivot=pivotLBackwardOrder[kPivot];
    if(!mark[kPivot]&&region[kPivot]) {
      stackList[0].stack=kPivot;
      stackList[0].start=startRow[kPivot];
      CoinBigIndex j=startRow[kPivot+1]-1;
      CoinSimplexInt nStack=0;
      while (nStack>=0) {
	/* take off stack */
	if (j>=stackList[nStack].start) {
	  CoinSimplexInt jPivot=column[j--];
	  /* put back on stack */
	  stackList[nStack].next =j;
	  if (!mark[jPivot]) {
	    /* and new one */
	    kPivot=jPivot;
	    //iPivot=pivotLBackwardOrder[kPivot];
	    j = startRow[kPivot+1]-1;
	    stackList[++nStack].stack=kPivot;
	    mark[kPivot]=1;
	    stackList[nStack].start=startRow[kPivot];
	    stackList[nStack].next=j;
	  }
	} else {
	  /* finished so mark */
	  list[nList++]=kPivot;
	  mark[kPivot]=1;
	  --nStack;
	  if (nStack>=0) {
	    kPivot=stackList[nStack].stack;
	    j=stackList[nStack].next;
	  }
	}
      }
    }
  }
  instrument_start("CoinAbcFactorizationUpdateTransposeLSparse",2*(numberNonZero+nList));
  numberNonZero=0;
  for (CoinSimplexInt i=nList-1;i>=0;i--) {
    CoinSimplexInt iPivot = list[i];
    //CoinSimplexInt kPivot=pivotLOrder[iPivot];
    mark[iPivot]=0;
    CoinFactorizationDouble pivotValue = region[iPivot];
    if ( !TEST_LESS_THAN_TOLERANCE(pivotValue )  ) {
      regionIndex[numberNonZero++] = iPivot;
      CoinBigIndex start=startRow[iPivot];
      CoinBigIndex end=startRow[iPivot+1];
      instrument_add(end-start);
#ifndef INLINE_IT
      CoinBigIndex j;
      for ( j = start; j < end; j ++ ) {
	CoinSimplexInt iRow = column[j];
	CoinFactorizationDouble value = element[j];
	region[iRow] -= value * pivotValue;
      }
#else
      CoinAbcScatterUpdate(end-start,pivotValue,element+start,column+start,region);
#endif
    } else {
      region[iPivot]=0.0;
    }
  }
  //set counts
  regionSparse->setNumElements ( numberNonZero );
  instrument_end_and_adjust(1.3);
}
#endif
//  updateColumnTransposeL.  Updates part of column transpose (BTRANL)
void
CoinAbcTypeFactorization::updateColumnTransposeL ( CoinIndexedVector * regionSparse 
#if ABC_SMALL<2
		       , CoinAbcStatistics & statistics
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
#if ABC_SMALL<3
  CoinSimplexInt number = regionSparse->getNumElements (  );
#endif
  if (!numberL_
#if ABC_SMALL<4
      &&!numberDense_
#endif
      ) {
#if ABC_SMALL<3
    if (number>=numberRows_) {
      // need scan
      regionSparse->setNumElements(0);
      scan(regionSparse);
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
    return;
  }
#if ABC_SMALL<2
  CoinSimplexInt goSparse;
  // Guess at number at end
  // we may need to rethink on dense
  if (gotLCopy()) {
    assert (statistics.averageAfterL_);
    CoinSimplexInt newNumber = static_cast<CoinSimplexInt> (number*statistics.averageAfterL_*twiddleBtranFactor1());
    if (newNumber< sparseThreshold_)
      goSparse = 2;
    else if (2*newNumber< numberRows_)
      goSparse = 0;
    else
      goSparse = -1;
  } else {
    goSparse=-1;
  }
#endif
#if ABC_SMALL<4
#if ABC_DENSE_CODE>0
  if (numberDense_) {
    //take off list
    CoinSimplexInt lastSparse = numberRows_-numberDense_;
    CoinFactorizationDouble * COIN_RESTRICT region = denseVector(regionSparse);
    const CoinSimplexInt * COIN_RESTRICT pivotLOrder = pivotLOrderAddress_;
    CoinFactorizationDouble *  COIN_RESTRICT denseArea = denseAreaAddress_;
    CoinFactorizationDouble * COIN_RESTRICT denseRegion = 
      denseArea+leadingDimension_*numberDense_;
    CoinSimplexInt *  COIN_RESTRICT densePermute=
      reinterpret_cast<CoinSimplexInt *>(denseRegion+FACTOR_CPU*numberDense_);
#if ABC_PARALLEL
    //printf("PP %d %d %s\n",whichSparse,__LINE__,__FILE__);
    if (whichSparse)
      denseRegion+=whichSparse*numberDense_;
#endif
    CoinFactorizationDouble * COIN_RESTRICT densePut = 
      denseRegion-lastSparse;
    CoinZeroN(denseRegion,numberDense_);
    bool doDense=false;
#if ABC_SMALL<3
    CoinSimplexInt number = regionSparse->getNumElements();
    CoinSimplexInt * COIN_RESTRICT regionIndex = regionSparse->getIndices (  );
    const CoinSimplexInt * COIN_RESTRICT pivotLBackwardOrder = permuteAddress_;
    CoinSimplexInt i=0;
#if ABC_SMALL<0
    assert (number<=numberRows_);
#else
    if (number<=numberRows_) {
#endif
      while (i<number) {
	CoinSimplexInt iRow = regionIndex[i];
#ifndef ABC_ORDERED_FACTORIZATION
	CoinSimplexInt jRow = pivotLBackwardOrder[iRow];
#else
	CoinSimplexInt jRow=iRow;
#endif
	if (jRow>=lastSparse) {
	  doDense=true;
	  regionIndex[i] = regionIndex[--number];
	  densePut[jRow]=region[iRow];
	  region[iRow]=0.0;
	} else {
	  i++;
	}
      }
#endif
#if ABC_SMALL>=0
#if ABC_SMALL<3
    } else {
#endif
      for (CoinSimplexInt jRow=lastSparse;jRow<numberRows_;jRow++) {
	CoinSimplexInt iRow = pivotLOrder[jRow];
	if (region[iRow]) {
	  doDense=true;
	  densePut[jRow]=region[iRow];
	  region[iRow]=0.0;
	}
      }
#if ABC_SMALL<3
      // numbers are all wrong - do a scan
      regionSparse->setNumElements(0);
      scan(regionSparse);
      number=regionSparse->getNumElements();
    }
#endif
#endif
    if (doDense) {
      instrument_do("CoinAbcFactorizationDenseTranspose",0.25*numberDense_*numberDense_);
#if ABC_DENSE_CODE==1
#ifndef CLAPACK
      char trans = 'T';
      CoinSimplexInt ione=1;
      CoinSimplexInt info;
      F77_FUNC(dgetrs,DGETRS)(&trans,&numberDense_,&ione,denseArea,&leadingDimension_,
			      densePermute,denseRegion,&numberDense_,&info,1);
#else
      clapack_dgetrs(CblasColMajor, CblasTrans,numberDense_,1,
		     denseArea, leadingDimension_,densePermute,denseRegion,numberDense_);
#endif
#elif ABC_DENSE_CODE==2
      CoinAbcDgetrs('T',numberDense_,denseArea,denseRegion);
      short * COIN_RESTRICT forBtran = reinterpret_cast<short *>(densePermute+numberDense_)+numberDense_-lastSparse;
      pivotLOrder += lastSparse; // adjust
#endif
      for (CoinSimplexInt i=lastSparse;i<numberRows_;i++) {
	CoinSimplexDouble value = densePut[i];
	if (value) {
	  if (!TEST_LESS_THAN_TOLERANCE(value)) {
#if ABC_DENSE_CODE!=2
	    CoinSimplexInt iRow=pivotLOrder[i];
#else
	    CoinSimplexInt iRow=pivotLOrder[forBtran[i]];
#endif
	    region[iRow]=value;
#if ABC_SMALL<3
	    regionIndex[number++] = iRow;
#endif
	  }
	}
      }
#if ABC_SMALL<3
      regionSparse->setNumElements(number);
#endif
    } 
  } 
  //printRegion(*regionSparse,"After BtranL");
#endif
#endif
#if ABC_SMALL<2
  switch (goSparse) {
  case -1: // No row copy
    updateColumnTransposeLDensish(regionSparse);
    break;
  case 0: // densish but by row
    updateColumnTransposeLByRow(regionSparse);
    break;
  case 2: // sparse
    updateColumnTransposeLSparse(regionSparse
#if ABC_PARALLEL
				      ,whichSparse
#endif
				 );
    break;
  }
#else
  updateColumnTransposeLDensish(regionSparse);
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
#if ABC_SMALL>=0
// Updates part of column transpose (BTRANR) when dense
void 
CoinAbcTypeFactorization::updateColumnTransposeRDensish( CoinIndexedVector * regionSparse ) const
{
  const CoinBigIndex *  COIN_RESTRICT startColumn = startColumnRAddress_-numberRows_;
  CoinFactorizationDouble * COIN_RESTRICT region = denseVector(regionSparse);
#if ABC_SMALL<3
#ifdef ABC_DEBUG
  regionSparse->checkClean();
#endif
#endif
  instrument_start("CoinAbcFactorizationUpdateTransposeRDensish",numberRows_);
  CoinSimplexInt last = numberRowsExtra_-1;
  const CoinSimplexInt * COIN_RESTRICT indexRow = indexRowRAddress_;
  const CoinFactorizationDouble * COIN_RESTRICT element = elementRAddress_;
  const CoinSimplexInt *  COIN_RESTRICT pivotRowBack = pivotColumnAddress_;
  for (CoinSimplexInt i = last ; i >= numberRows_; i-- ) {
    CoinSimplexInt putRow=pivotRowBack[i];
    CoinFactorizationDouble pivotValue = region[putRow];
    if ( pivotValue ) {
      CoinBigIndex start=startColumn[i];
      CoinBigIndex end=startColumn[i+1];
      instrument_add(end-start);
#ifndef INLINE_IT
      for (CoinBigIndex j = start; j < end; j++ ) {
	CoinFactorizationDouble value = element[j];
	CoinSimplexInt iRow = indexRow[j];
	assert (iRow<numberRows_);
	region[iRow] -= value * pivotValue;
      }
#else
      CoinAbcScatterUpdate(end-start,pivotValue,element+start,indexRow+start,region);
#endif
      region[putRow] = pivotValue;
    }
  }
  instrument_end();
}
#endif
#if ABC_SMALL<2
// Updates part of column transpose (BTRANR) when sparse
void 
CoinAbcTypeFactorization::updateColumnTransposeRSparse 
( CoinIndexedVector * regionSparse ) const
{
  CoinFactorizationDouble * COIN_RESTRICT region = denseVector(regionSparse);
  CoinSimplexInt * COIN_RESTRICT regionIndex = regionSparse->getIndices (  );
  CoinSimplexInt numberNonZero = regionSparse->getNumElements (  );
  instrument_start("CoinAbcFactorizationUpdateTransposeRSparse",numberRows_);

#if ABC_SMALL<3
#ifdef ABC_DEBUG
  regionSparse->checkClean();
#endif
#endif
  CoinSimplexInt last = numberRowsExtra_-1;
  
  const CoinSimplexInt * COIN_RESTRICT indexRow = indexRowRAddress_;
  const CoinFactorizationDouble * COIN_RESTRICT element = elementRAddress_;
  const CoinBigIndex *  COIN_RESTRICT startColumn = startColumnRAddress_-numberRows_;
  //move using lookup
  const CoinSimplexInt *  COIN_RESTRICT pivotRowBack = pivotColumnAddress_;
  // still need to do in correct order (for now)
  for (CoinSimplexInt i = last ; i >= numberRows_; i-- ) {
    CoinSimplexInt putRow = pivotRowBack[i];
    CoinFactorizationDouble pivotValue = region[putRow];
    if ( pivotValue ) {
      instrument_add(startColumn[i+1]-startColumn[i]);
      for (CoinBigIndex j = startColumn[i]; j < startColumn[i+1]; j++ ) {
	CoinFactorizationDouble value = element[j];
	CoinSimplexInt iRow = indexRow[j];
	assert (iRow<numberRows_);
	bool oldValue =  (TEST_DOUBLE_REALLY_NONZERO(region[iRow]));
	CoinFactorizationDouble newValue = region[iRow] - value * pivotValue;
	if (oldValue) {
	  if (newValue) 
	    region[iRow]=newValue;
	  else
	    region[iRow]=COIN_INDEXED_REALLY_TINY_ELEMENT;
	} else if (!TEST_LESS_THAN_TOLERANCE_REGISTER(newValue)) {
	  region[iRow] = newValue;
	  regionIndex[numberNonZero++]=iRow;
	}
      }
      region[putRow] = pivotValue;
    }
  }
  instrument_end();
  regionSparse->setNumElements(numberNonZero);
}
#endif
//  updateColumnTransposeR.  Updates part of column (FTRANR)
void
CoinAbcTypeFactorization::updateColumnTransposeR ( CoinIndexedVector * regionSparse 
#if ABC_SMALL<2
						   , CoinAbcStatistics & statistics
#endif
						   ) const
{
  if (numberRowsExtra_==numberRows_)
    return;
#if ABC_SMALL<3
  CoinSimplexInt numberNonZero = regionSparse->getNumElements (  );
  if (numberNonZero) {
#endif
#if ABC_SMALL<3
    const CoinBigIndex *  COIN_RESTRICT startColumn = startColumnRAddress_-numberRows_;
#endif
    // Size of R
    instrument_do("CoinAbcFactorizationTransposeR",startColumnRAddress_[numberR_]);
#if ABC_SMALL<2
    if (startColumn[numberRowsExtra_]) {
#if ABC_SMALL>=0
      if (numberNonZero < ((sparseThreshold_<<2)*twiddleBtranFactor2())
	  ||(!numberL_&&gotLCopy()&&gotRCopy())) {
#endif
	updateColumnTransposeRSparse ( regionSparse );
	if (factorizationStatistics()) 
	  statistics.countAfterR_ += regionSparse->getNumElements();
#if ABC_SMALL>=0
      } else {
	updateColumnTransposeRDensish ( regionSparse );
	// we have lost indices
	// make sure won't try and go sparse again
	if (factorizationStatistics()) 
	  statistics.countAfterR_ += CoinMin((numberNonZero<<1),numberRows_);
	// temp +2 (was +1)
	regionSparse->setNumElements (numberRows_+2);
      }
#endif
    } else {
      if (factorizationStatistics()) 
	statistics.countAfterR_ += numberNonZero;
    }
#else
    updateColumnTransposeRDensish ( regionSparse );
#if ABC_SMALL<3
    // we have lost indices
    // make sure won't try and go sparse again
    // temp +2 (was +1)
    regionSparse->setNumElements (numberRows_+2);
#endif
#endif
#if ABC_SMALL<3
  }
#endif
}
// Update partial Ftran by R update
void 
CoinAbcTypeFactorization::updatePartialUpdate(CoinIndexedVector & partialUpdate)
{
  CoinSimplexDouble * COIN_RESTRICT region = partialUpdate.denseVector (  );
  CoinSimplexInt * COIN_RESTRICT regionIndex = partialUpdate.getIndices (  );
  CoinSimplexInt numberNonZero = partialUpdate.getNumElements (  );
  const CoinSimplexInt *  COIN_RESTRICT indexRow = indexRowRAddress_;
  const CoinFactorizationDouble *  COIN_RESTRICT element = elementRAddress_;
  const CoinSimplexInt * pivotRowBack = pivotColumnAddress_;
  const CoinBigIndex *  COIN_RESTRICT startColumn = startColumnRAddress_-numberRows_;
  CoinBigIndex start = startColumn[numberRowsExtra_-1];
  CoinBigIndex end = startColumn[numberRowsExtra_];
  CoinSimplexInt iRow =pivotRowBack[numberRowsExtra_-1];
  CoinFactorizationDouble pivotValue = region[iRow];
  for (CoinBigIndex j = start; j < end; j ++ ) {
    CoinFactorizationDouble value = element[j];
    CoinSimplexInt jRow = indexRow[j];
    value *= region[jRow];
    pivotValue -= value;
  }
  if ( !TEST_LESS_THAN_TOLERANCE_REGISTER(pivotValue ) ) {
    if (!region[iRow])
      regionIndex[numberNonZero++] = iRow;
    region[iRow] = pivotValue;
  } else {
    if (region[iRow])
      region[iRow] = COIN_INDEXED_REALLY_TINY_ELEMENT;
  }
  partialUpdate.setNumElements(numberNonZero);
}
#if FACTORIZATION_STATISTICS
#ifdef ABC_JUST_ONE_FACTORIZATION
#define FACTORS_HERE
#endif
#ifdef FACTORS_HERE
double ftranTwiddleFactor1X=1.0;
double ftranTwiddleFactor2X=1.0;
double ftranFTTwiddleFactor1X=1.0;
double ftranFTTwiddleFactor2X=1.0;
double btranTwiddleFactor1X=1.0;
double btranTwiddleFactor2X=1.0;
double ftranFullTwiddleFactor1X=1.0;
double ftranFullTwiddleFactor2X=1.0;
double btranFullTwiddleFactor1X=1.0;
double btranFullTwiddleFactor2X=1.0;
double denseThresholdX=31;
double twoThresholdX=1000;
double minRowsSparse=300;
double largeRowsSparse=10000;
double mediumRowsDivider=6;
double mediumRowsMinCount=500;
double largeRowsCount=1000;
#else
extern double ftranTwiddleFactor1X;
extern double ftranTwiddleFactor2X;
extern double ftranFTTwiddleFactor1X;
extern double ftranFTTwiddleFactor2X;
extern double btranTwiddleFactor1X;
extern double btranTwiddleFactor2X;
extern double ftranFullTwiddleFactor1X;
extern double ftranFullTwiddleFactor2X;
extern double btranFullTwiddleFactor1X;
extern double btranFullTwiddleFactor2X;
extern double denseThresholdX;
extern double twoThresholdX;
extern double minRowsSparse;
extern double largeRowsSparse;
extern double mediumRowsDivider;
extern double mediumRowsMinCount;
extern double largeRowsCount;
#endif
#else
#define minRowsSparse 300
#define largeRowsSparse 10000
#define mediumRowsDivider 6
#define mediumRowsMinCount 500
#define largeRowsCount 1000
#endif
//  makes a row copy of L
void
CoinAbcTypeFactorization::goSparse2 ( )
{
#if ABC_SMALL<2
#if ABC_SMALL>=0
  if (numberRows_>minRowsSparse) {
#endif
    if (numberRows_<largeRowsSparse) {
      sparseThreshold_=CoinMin(numberRows_/mediumRowsDivider,mediumRowsMinCount);
    } else {
      sparseThreshold_=CoinMax(largeRowsCount,numberRows_>>3);
    }
#if FACTORIZATION_STATISTICS
    ftranTwiddleFactor1_=ftranTwiddleFactor1X;
    ftranTwiddleFactor2_=ftranTwiddleFactor2X;
    ftranFTTwiddleFactor1_=ftranFTTwiddleFactor1X;
    ftranFTTwiddleFactor2_=ftranFTTwiddleFactor2X;
    btranTwiddleFactor1_=btranTwiddleFactor1X;
    btranTwiddleFactor2_=btranTwiddleFactor2X;
    ftranFullTwiddleFactor1_=ftranFullTwiddleFactor1X;
    ftranFullTwiddleFactor2_=ftranFullTwiddleFactor2X;
    btranFullTwiddleFactor1_=btranFullTwiddleFactor1X;
    btranFullTwiddleFactor2_=btranFullTwiddleFactor2X;
#endif
    setYesGotLCopy();
    setYesGotUCopy();
    setYesGotSparse();
    // allow for stack, list, next, starts and char map of mark
    CoinSimplexInt nRowIndex = (numberRows_+CoinSizeofAsInt(CoinSimplexInt)-1)/
      CoinSizeofAsInt(char);
    CoinSimplexInt nInBig = static_cast<CoinSimplexInt>(sizeof(CoinBigIndex)/sizeof(CoinSimplexInt));
    assert (nInBig>=1);
    sizeSparseArray_ = (2+2*nInBig)*numberRows_ + nRowIndex ;
    sparse_.conditionalNew(FACTOR_CPU*sizeSparseArray_);
    sparseAddress_ = sparse_.array();
    // zero out mark
    CoinAbcStack * stackList = reinterpret_cast<CoinAbcStack *>(sparseAddress_);
    listAddress_ = reinterpret_cast<CoinSimplexInt *>(stackList+numberRows_);
    markRowAddress_ = reinterpret_cast<CoinCheckZero *> (listAddress_ + numberRows_);
    assert(reinterpret_cast<CoinCheckZero *>(sparseAddress_+(2+2*nInBig)*numberRows_)==markRowAddress_);
    CoinAbcMemset0(markRowAddress_,nRowIndex);
#if ABC_PARALLEL
    //printf("PP 0__ %d %s\n",__LINE__,__FILE__);
    // convert to bytes
    sizeSparseArray_*=sizeof(CoinSimplexInt);
    char * mark=reinterpret_cast<char *>(reinterpret_cast<char *>(markRowAddress_)+sizeSparseArray_);
    for (int i=1;i<FACTOR_CPU;i++) {
      CoinAbcMemset0(mark,nRowIndex);
      mark=reinterpret_cast<char *>(reinterpret_cast<char *>(mark)+sizeSparseArray_);
    }
#endif
    elementByRowL_.conditionalDelete();
    indexColumnL_.conditionalDelete();
    startRowL_.conditionalNew(numberRows_+1);
    if (lengthAreaL_) {
      elementByRowL_.conditionalNew(lengthAreaL_);
      indexColumnL_.conditionalNew(lengthAreaL_);
    }
    elementByRowLAddress_=elementByRowL_.array();
    indexColumnLAddress_=indexColumnL_.array();
    startRowLAddress_=startRowL_.array();
    // counts
    CoinBigIndex * COIN_RESTRICT startRowL = startRowLAddress_;
    CoinZeroN(startRowL,numberRows_);
    const CoinBigIndex * startColumnL = startColumnLAddress_;
    CoinFactorizationDouble * COIN_RESTRICT elementL = elementLAddress_;
    const CoinSimplexInt * indexRowL = indexRowLAddress_;
    for (CoinSimplexInt i=baseL_;i<baseL_+numberL_;i++) {
      for (CoinBigIndex j=startColumnL[i];j<startColumnL[i+1];j++) {
	CoinSimplexInt iRow = indexRowL[j];
	startRowL[iRow]++;
      }
    }
    const CoinSimplexInt * COIN_RESTRICT pivotLOrder = pivotLOrderAddress_;
    // convert count to lasts
    CoinBigIndex count=0;
    for (CoinSimplexInt i=0;i<numberRows_;i++) {
      CoinSimplexInt numberInRow=startRowL[i];
      count += numberInRow;
      startRowL[i]=count;
    }
    startRowL[numberRows_]=count;
    // now insert
    CoinFactorizationDouble * COIN_RESTRICT elementByRowL = elementByRowLAddress_;
    CoinSimplexInt * COIN_RESTRICT indexColumnL = indexColumnLAddress_;
    for (CoinSimplexInt i=baseL_+numberL_-1;i>=baseL_;i--) {
#ifndef ABC_ORDERED_FACTORIZATION
      CoinSimplexInt iPivot=pivotLOrder[i];
#else
      CoinSimplexInt iPivot=i;
#endif
      for (CoinBigIndex j=startColumnL[i];j<startColumnL[i+1];j++) {
	CoinSimplexInt iRow = indexRowL[j];
	CoinBigIndex start = startRowL[iRow]-1;
	startRowL[iRow]=start;
	elementByRowL[start]=elementL[j];
	indexColumnL[start]=iPivot;
      }
    }
#if ABC_SMALL>=0
  } else {
    sparseThreshold_=0;
    setNoGotLCopy();
    //setYesGotUCopy();
    setNoGotUCopy();
    setNoGotSparse();
  }
#endif
#endif
}

//  set sparse threshold
void
CoinAbcTypeFactorization::sparseThreshold ( CoinSimplexInt /*value*/ ) 
{
  return;
#if 0
  if (value>0&&sparseThreshold_) {
    sparseThreshold_=value;
  } else if (!value&&sparseThreshold_) {
    // delete copy
    sparseThreshold_=0;
    elementByRowL_.conditionalDelete();
    startRowL_.conditionalDelete();
    indexColumnL_.conditionalDelete();
    sparse_.conditionalDelete();
  } else if (value>0&&!sparseThreshold_) {
    if (value>1)
      sparseThreshold_=value;
    else
      sparseThreshold_=0;
    goSparse();
  }
#endif
}
void CoinAbcTypeFactorization::maximumPivots (  CoinSimplexInt value )
{
  if (value>0) {
    if (numberRows_)
      maximumMaximumPivots_=CoinMax(maximumMaximumPivots_,value);
    else
      maximumMaximumPivots_=value;
    maximumPivots_=value;
  }
  //printf("max %d maxmax %d\n",maximumPivots_,maximumMaximumPivots_);
}
void CoinAbcTypeFactorization::messageLevel (  CoinSimplexInt value )
{
  if (value>0&&value<16) {
    messageLevel_=value;
  }
}
// Reset all sparsity etc statistics
void CoinAbcTypeFactorization::resetStatistics()
{
#if ABC_SMALL<2
  setStatistics(true);

  /// Below are all to collect
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

  /// We can roll over factorizations
  numberFtranCounts_=0;
  numberFtranFTCounts_=0;
  numberBtranCounts_=0;
  numberFtranFullCounts_=0;
  numberFtranFullCounts_=0;

  /// While these are averages collected over last 
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
// See if worth going sparse
void 
CoinAbcTypeFactorization::checkSparse()
{
#if ABC_SMALL<2
  // See if worth going sparse and when
  if (numberFtranCounts_>50) {
    ftranCountInput_= CoinMax(ftranCountInput_,1.0);
    ftranAverageAfterL_ = CoinMax(ftranCountAfterL_/ftranCountInput_,INITIAL_AVERAGE2);
    ftranAverageAfterR_ = CoinMax(ftranCountAfterR_/ftranCountAfterL_,INITIAL_AVERAGE2);
    ftranAverageAfterU_ = CoinMax(ftranCountAfterU_/ftranCountAfterR_,INITIAL_AVERAGE2);
    ftranFTCountInput_= CoinMax(ftranFTCountInput_,INITIAL_AVERAGE2);
    ftranFTAverageAfterL_ = CoinMax(ftranFTCountAfterL_/ftranFTCountInput_,INITIAL_AVERAGE2);
    ftranFTAverageAfterR_ = CoinMax(ftranFTCountAfterR_/ftranFTCountAfterL_,INITIAL_AVERAGE2);
    ftranFTAverageAfterU_ = CoinMax(ftranFTCountAfterU_/ftranFTCountAfterR_,INITIAL_AVERAGE2);
    if (btranCountInput_&&btranCountAfterU_&&btranCountAfterR_) {
      btranAverageAfterU_ = CoinMax(btranCountAfterU_/btranCountInput_,INITIAL_AVERAGE2);
      btranAverageAfterR_ = CoinMax(btranCountAfterR_/btranCountAfterU_,INITIAL_AVERAGE2);
      btranAverageAfterL_ = CoinMax(btranCountAfterL_/btranCountAfterR_,INITIAL_AVERAGE2);
    } else {
      // we have not done any useful btrans (values pass?)
      btranAverageAfterU_ = INITIAL_AVERAGE2;
      btranAverageAfterR_ = INITIAL_AVERAGE2;
      btranAverageAfterL_ = INITIAL_AVERAGE2;
    }
    ftranFullCountInput_= CoinMax(ftranFullCountInput_,1.0);
    ftranFullAverageAfterL_ = CoinMax(ftranFullCountAfterL_/ftranFullCountInput_,INITIAL_AVERAGE2);
    ftranFullAverageAfterR_ = CoinMax(ftranFullCountAfterR_/ftranFullCountAfterL_,INITIAL_AVERAGE2);
    ftranFullAverageAfterU_ = CoinMax(ftranFullCountAfterU_/ftranFullCountAfterR_,INITIAL_AVERAGE2);
    btranFullCountInput_= CoinMax(btranFullCountInput_,1.0);
    btranFullAverageAfterL_ = CoinMax(btranFullCountAfterL_/btranFullCountInput_,INITIAL_AVERAGE2);
    btranFullAverageAfterR_ = CoinMax(btranFullCountAfterR_/btranFullCountAfterL_,INITIAL_AVERAGE2);
    btranFullAverageAfterU_ = CoinMax(btranFullCountAfterU_/btranFullCountAfterR_,INITIAL_AVERAGE2);
  }
  // scale back
  
  ftranCountInput_ *= AVERAGE_SCALE_BACK;
  ftranCountAfterL_ *= AVERAGE_SCALE_BACK;
  ftranCountAfterR_ *= AVERAGE_SCALE_BACK;
  ftranCountAfterU_ *= AVERAGE_SCALE_BACK;
  ftranFTCountInput_ *= AVERAGE_SCALE_BACK;
  ftranFTCountAfterL_ *= AVERAGE_SCALE_BACK;
  ftranFTCountAfterR_ *= AVERAGE_SCALE_BACK;
  ftranFTCountAfterU_ *= AVERAGE_SCALE_BACK;
  btranCountInput_ *= AVERAGE_SCALE_BACK;
  btranCountAfterU_ *= AVERAGE_SCALE_BACK;
  btranCountAfterR_ *= AVERAGE_SCALE_BACK;
  btranCountAfterL_ *= AVERAGE_SCALE_BACK;
  ftranFullCountInput_ *= AVERAGE_SCALE_BACK;
  ftranFullCountAfterL_ *= AVERAGE_SCALE_BACK;
  ftranFullCountAfterR_ *= AVERAGE_SCALE_BACK;
  ftranFullCountAfterU_ *= AVERAGE_SCALE_BACK;
  btranFullCountInput_ *= AVERAGE_SCALE_BACK;
  btranFullCountAfterL_ *= AVERAGE_SCALE_BACK;
  btranFullCountAfterR_ *= AVERAGE_SCALE_BACK;
  btranFullCountAfterU_ *= AVERAGE_SCALE_BACK;
#endif
}
// Condition number - product of pivots after factorization
CoinSimplexDouble 
CoinAbcTypeFactorization::conditionNumber() const
{
  CoinSimplexDouble condition = 1.0;
  const CoinFactorizationDouble * pivotRegion = pivotRegionAddress_;
  for (CoinSimplexInt i=0;i<numberRows_;i++) {
    condition *= pivotRegion[i];
  }
  condition = CoinMax(fabs(condition),1.0e-50);
  return 1.0/condition;
}
#ifndef NDEBUG
void 
CoinAbcTypeFactorization::checkMarkArrays() const
{
  CoinCheckZero * COIN_RESTRICT mark = markRowAddress_;
  for (CoinSimplexInt i=0;i<numberRows_;i++) {
    assert (!mark[i]);
  }
#if ABC_PARALLEL
  mark=reinterpret_cast<CoinCheckZero *>(reinterpret_cast<char *>(mark)+sizeSparseArray_);
  for (CoinSimplexInt i=0;i<numberRows_;i++) {
    assert (!mark[i]);
  }
  for (int i=1;i<FACTOR_CPU;i++) {
    for (CoinSimplexInt i=0;i<numberRows_;i++) {
      assert (!mark[i]);
    }
    mark=reinterpret_cast<CoinCheckZero *>(reinterpret_cast<char *>(mark)+sizeSparseArray_);
  }
#endif
}
#endif
#endif
