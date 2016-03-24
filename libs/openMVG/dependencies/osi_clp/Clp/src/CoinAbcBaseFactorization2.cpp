/* $Id$ */
// Copyright (C) 2002, International Business Machines
// Corporation and others, Copyright (C) 2012, FasterCoin.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).
#ifdef ABC_JUST_ONE_FACTORIZATION
#include "CoinAbcCommonFactorization.hpp"
#define CoinAbcTypeFactorization CoinAbcBaseFactorization
#define ABC_SMALL -2
#include "CoinAbcBaseFactorization.hpp"
#endif
#ifdef CoinAbcTypeFactorization

#include "CoinIndexedVector.hpp"
#include "CoinHelperFunctions.hpp"
#include "CoinFinite.hpp"
//  factorSparse.  Does sparse phase of factorization
//return code is <0 error, 0= finished
CoinSimplexInt
CoinAbcTypeFactorization::factorSparse (  )
{
  CoinSimplexInt * COIN_RESTRICT indexRow = indexRowUAddress_;
  CoinSimplexInt * COIN_RESTRICT indexColumn = indexColumnUAddress_;
  CoinFactorizationDouble * COIN_RESTRICT element = elementUAddress_;
  workArea_.conditionalNew(numberRows_);
  workAreaAddress_=workArea_.array();
  CoinFactorizationDouble *  COIN_RESTRICT workArea = workAreaAddress_;
  double initialLargest = preProcess3();
  // adjust for test
  initialLargest=1.0e-10/initialLargest;
#if ABC_NORMAL_DEBUG>0
  double largestPivot=1.0;
  double smallestPivot=1.0;
#endif
  CoinSimplexInt status = 0;
  //do slacks first
  CoinSimplexInt *  COIN_RESTRICT numberInRow = numberInRowAddress_;
  CoinSimplexInt *  COIN_RESTRICT numberInColumn = numberInColumnAddress_;
  //CoinSimplexInt * numberInColumnPlus = numberInColumnPlusAddress_;
  CoinBigIndex *  COIN_RESTRICT startColumnU = startColumnUAddress_;
  //CoinBigIndex *  COIN_RESTRICT startColumnL = startColumnLAddress_;
  CoinZeroN ( workArea, numberRows_ );
  CoinSimplexInt * COIN_RESTRICT nextCount = this->nextCountAddress_;
  CoinSimplexInt * COIN_RESTRICT firstCount = this->firstCountAddress_;
  CoinBigIndex * COIN_RESTRICT startRow = startRowUAddress_;
  CoinBigIndex * COIN_RESTRICT startColumn = startColumnU;
#if 0
	{
	  int checkEls=0;
	  for (CoinSimplexInt iColumn=0;iColumn<numberRows_;iColumn++) {
	    if (numberInColumn[iColumn]) { 
	      checkEls+=numberInColumn[iColumn];
	    }
	  }
	  assert (checkEls==totalElements_);
	}
#endif
  // Put column singletons first - (if false)
  //separateLinks();
  // while just singletons - do singleton rows first
  CoinSimplexInt *  COIN_RESTRICT pivotColumn = pivotColumnAddress_;
  // allow a bit more on tolerance
  double tolerance=0.5*pivotTolerance_;
  CoinSimplexInt *  COIN_RESTRICT nextRow = nextRowAddress_;
  CoinSimplexInt *  COIN_RESTRICT lastRow = lastRowAddress_;
  CoinBigIndex *  COIN_RESTRICT startColumnL = startColumnLAddress_;
  CoinFactorizationDouble *  COIN_RESTRICT elementL = elementLAddress_;
  CoinSimplexInt *  COIN_RESTRICT indexRowL = indexRowLAddress_;
  CoinSimplexInt *  COIN_RESTRICT numberInColumnPlus = numberInColumnPlusAddress_;
  CoinFactorizationDouble * COIN_RESTRICT pivotRegion = pivotRegionAddress_;
  startColumnL[numberGoodL_] = lengthL_; //for luck and first time
#ifdef ABC_USE_FUNCTION_POINTERS
  CoinSimplexInt *  COIN_RESTRICT nextColumn = nextColumnAddress_;
  CoinSimplexInt *  COIN_RESTRICT lastColumn = lastColumnAddress_;
  scatterStruct * COIN_RESTRICT scatter = scatterUColumn();
#if ABC_USE_FUNCTION_POINTERS
  extern scatterUpdate AbcScatterLowSubtract[9];
  extern scatterUpdate AbcScatterHighSubtract[4];
#endif
  CoinFactorizationDouble *  COIN_RESTRICT elementUColumnPlus = elementUColumnPlusAddress_;
#endif
#if CONVERTROW>1
  CoinBigIndex * COIN_RESTRICT convertRowToColumn = convertRowToColumnUAddress_;
  CoinBigIndex * COIN_RESTRICT convertColumnToRow = convertColumnToRowUAddress_;
#endif
  //#define PRINT_INVERT_STATS
#ifdef PRINT_INVERT_STATS
  int numberGoodUX=numberGoodU_;
  int numberTakenOutOfUSpace=0;
#endif
  CoinSimplexInt look = firstCount[1];
  while (look>=0) {
    checkLinks(1);
    CoinSimplexInt iPivotRow;
    CoinSimplexInt iPivotColumn=-1;
#ifdef SMALL_PERMUTE
    int realPivotRow;
    int realPivotColumn=-1;
#endif
    CoinFactorizationDouble pivotMultiplier=0.0;
    if ( look < numberRows_ ) {
      assert (numberInRow[look]==1);
      iPivotRow = look;
      CoinBigIndex start = startRow[iPivotRow];
      iPivotColumn = indexColumn[start];
#ifdef SMALL_PERMUTE
      realPivotRow=fromSmallToBigRow_[iPivotRow];
      realPivotColumn=fromSmallToBigColumn_[iPivotColumn];
      //printf("singleton row realPivotRow %d realPivotColumn %d\n",
      //     realPivotRow,realPivotColumn);
#endif
      assert (numberInColumn[iPivotColumn]>0);
      CoinBigIndex startC = startColumn[iPivotColumn];
      double minimumValue = fabs(element[startC]*tolerance );
#if CONVERTROW<2
      CoinBigIndex where = startC;
      while ( indexRow[where] != iPivotRow ) {
	where++;
      }
#else
      CoinBigIndex where = convertRowToColumn[start];
#endif
      assert ( where < startC+numberInColumn[iPivotColumn]);
      CoinFactorizationDouble value = element[where];
      // size shouldn't matter as would be taken if matrix flipped but ...
      if ( fabs(value) > minimumValue ) {
	CoinSimplexInt numberDoColumn=numberInColumn[iPivotColumn];
	totalElements_ -= numberDoColumn;
	// L is always big enough here
	//store column in L, compress in U and take column out
	CoinBigIndex l = lengthL_;
	lengthL_ += numberDoColumn-1;
	pivotMultiplier = 1.0/value;
	CoinBigIndex endC = startC + numberDoColumn;
	for (CoinBigIndex i = startC; i < endC; i++ ) {
	  if (i!=where) {
	    CoinSimplexInt iRow = indexRow[i];
#ifdef SMALL_PERMUTE
	    indexRowL[l] = fromSmallToBigRow_[iRow];
#else
	    indexRowL[l] = iRow;
#endif
	    elementL[l] = element[i] * pivotMultiplier;
	    l++;
	    //take out of row list
	    CoinBigIndex startR = startRow[iRow];
	    CoinSimplexInt iNumberInRow = numberInRow[iRow];
	    CoinBigIndex endR = startR + iNumberInRow;
#if CONVERTROW<2
	    CoinBigIndex whereRow = startR;
	    while ( indexColumn[whereRow] != iPivotColumn ) 
	      whereRow++;
	    assert ( whereRow < endR );
#else
	    CoinBigIndex whereRow = convertColumnToRow[i];
#endif
	    int iColumn = indexColumn[endR-1];
	    indexColumn[whereRow] = iColumn;
#if CONVERTROW>1
	    CoinBigIndex whereColumnEntry = convertRowToColumn[endR-1];
            convertRowToColumn[whereRow] = whereColumnEntry;
	    convertColumnToRow[whereColumnEntry] = whereRow;
#endif
	    iNumberInRow--;
	    numberInRow[iRow] = iNumberInRow;
	    modifyLink ( iRow, iNumberInRow );
	    checkLinks();
	  }
	}
      } else {
	//printf("Rejecting as element %g largest %g\n",value,element[startC]);
      }
    } else {
      iPivotColumn = look - numberRows_;
      assert ( numberInColumn[iPivotColumn] == 1 );
      {
	CoinBigIndex startC = startColumn[iPivotColumn];
	iPivotRow = indexRow[startC];
#ifdef SMALL_PERMUTE
	realPivotRow=fromSmallToBigRow_[iPivotRow];
	realPivotColumn=fromSmallToBigColumn_[iPivotColumn];
	int realPivotRow;
	realPivotRow=fromSmallToBigRow_[iPivotRow];
	indexRow[startC]=realPivotRow;
	//printf("singleton column realPivotRow %d realPivotColumn %d\n",
	//   realPivotRow,realPivotColumn);
#endif
	//store pivot columns (so can easily compress)
	assert (numberInColumn[iPivotColumn]==1);
	CoinSimplexInt numberDoRow = numberInRow[iPivotRow];
	int numberDoColumn = numberInColumn[iPivotColumn] - 1;
	totalElements_ -= ( numberDoRow + numberDoColumn );
	CoinBigIndex startR = startRow[iPivotRow];
	CoinBigIndex endR = startR + numberDoRow ;
	//clean up counts
	pivotMultiplier = 1.0/element[startC];
	// would I be better off doing other way first??
	for (CoinBigIndex i = startR; i < endR; i++ ) {
	  CoinSimplexInt iColumn = indexColumn[i];
	  if (iColumn==iPivotColumn)
	    continue;
	  assert ( numberInColumn[iColumn] );
	  CoinSimplexInt number = numberInColumn[iColumn] - 1;
	  //modify linked list
	  modifyLink ( iColumn + numberRows_, number );
	  CoinBigIndex start = startColumn[iColumn];
	  //move pivot row element
	  if ( number ) {
#if CONVERTROW<2
	    CoinBigIndex pivot = start;
	    CoinSimplexInt iRow = indexRow[pivot];
	    while ( iRow != iPivotRow ) {
	      pivot++;
	      iRow = indexRow[pivot];
	    }
#else
	    CoinBigIndex pivot= convertRowToColumn[i];
#endif
	    assert (pivot < startColumn[iColumn] +
		    numberInColumn[iColumn]);
	    if ( pivot != start ) {
	      //move largest one up
	      CoinFactorizationDouble value = element[start];
	      int iRow = indexRow[start];
	      element[start] = element[pivot];
	      indexRow[start] = indexRow[pivot];
	      element[pivot] = element[start + 1];
	      indexRow[pivot] = indexRow[start + 1];
#if CONVERTROW>1
	      CoinBigIndex whereRowEntry = convertColumnToRow[start+1];
	      CoinBigIndex whereRowEntry2 = convertColumnToRow[start];
	      convertRowToColumn[whereRowEntry] = pivot;
	      convertColumnToRow[pivot] = whereRowEntry;
	      convertRowToColumn[whereRowEntry2] = start+1;
	      convertColumnToRow[start+1] = whereRowEntry2;
#endif
	      element[start + 1] = value;
	      indexRow[start + 1] = iRow;
	    } else {
	      //find new largest element
	      CoinSimplexInt iRowSave = indexRow[start + 1];
	      CoinFactorizationDouble valueSave = element[start + 1];
	      CoinFactorizationDouble valueLargest = fabs ( valueSave );
	      CoinBigIndex end = start + numberInColumn[iColumn];
	      CoinBigIndex largest = start + 1;
	      for (CoinBigIndex k = start + 2; k < end; k++ ) {
		CoinFactorizationDouble value = element[k];
		CoinFactorizationDouble valueAbs = fabs ( value );
		if ( valueAbs > valueLargest ) {
		  valueLargest = valueAbs;
		  largest = k;
		}
	      }
	      indexRow[start + 1] = indexRow[largest];
	      element[start + 1] = element[largest];
#if CONVERTROW>1
	      CoinBigIndex whereRowEntry = convertColumnToRow[largest];
	      CoinBigIndex whereRowEntry2 = convertColumnToRow[start+1];
	      convertRowToColumn[whereRowEntry] = start+1;
	      convertColumnToRow[start+1] = whereRowEntry;
	      convertRowToColumn[whereRowEntry2] = largest;
	      convertColumnToRow[largest] = whereRowEntry2;
#endif
	      indexRow[largest] = iRowSave;
	      element[largest] = valueSave;
	    }
	  }
	  //clean up counts
	  numberInColumn[iColumn]--;
	  numberInColumnPlus[iColumn]++;
#ifdef SMALL_PERMUTE
	  indexRow[start]=realPivotRow;
#endif
	  startColumn[iColumn]++;
	}
      }
    }
    if (pivotMultiplier) {
      numberGoodL_++;
      startColumnL[numberGoodL_] = lengthL_;
      //take out this bit of indexColumnU
      CoinSimplexInt next = nextRow[iPivotRow];
      CoinSimplexInt last = lastRow[iPivotRow];
      nextRow[last] = next;
      lastRow[next] = last;
      lastRow[iPivotRow] =-2; //mark
#ifdef SMALL_PERMUTE
      // mark
      fromSmallToBigRow_[iPivotRow]=-1;
      fromSmallToBigColumn_[iPivotColumn]=-1;
      permuteAddress_[realPivotRow]=numberGoodU_;
#else
      permuteAddress_[iPivotRow]=numberGoodU_;
#endif
#ifdef ABC_USE_FUNCTION_POINTERS
      int number = numberInColumnPlus[iPivotColumn];
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
	scatter[iPivotRow].functionPointer=AbcScatterLowSubtract[number];
      } else {
	scatter[iPivotRow].functionPointer=AbcScatterHighSubtract[number&3];
      }
#endif
      scatter[iPivotRow].offset=lastEntryByColumnUPlus_;
      scatter[iPivotRow].number=number;
#endif
      CoinBigIndex startC = startColumn[iPivotColumn]-number;
      for (int i=startC;i<startC+number;i++)
	elementUColumnPlus[lastEntryByColumnUPlus_++]=element[i]*pivotMultiplier;
      CoinAbcMemcpy(reinterpret_cast<CoinSimplexInt *>(elementUColumnPlus+lastEntryByColumnUPlus_),indexRow+startC,number);
      lastEntryByColumnUPlus_ += (number+1)>>1;
#endif
      numberInColumn[iPivotColumn] = 0;
      numberInRow[iPivotRow] = 0;
      //modify linked list for pivots
      deleteLink ( iPivotRow );
      deleteLink ( iPivotColumn + numberRows_ );
    checkLinks();
#ifdef SMALL_PERMUTE
      assert (permuteAddress_[realPivotRow]==numberGoodU_);
      pivotColumn[numberGoodU_] = realPivotColumn;
#else
      assert (permuteAddress_[iPivotRow]==numberGoodU_);
      pivotColumn[numberGoodU_] = iPivotColumn;
#endif
      pivotRegion[numberGoodU_] = pivotMultiplier;
      numberGoodU_++;
    } else {
      //take out for moment
      modifyLink ( iPivotRow, numberRows_ + 1 );
    }
    look=-1; //nextCount[look];
    if (look<0) {
      // start again
      look = firstCount[1];
    }
    assert (iPivotColumn>=0);
#ifdef ABC_USE_FUNCTION_POINTERS
    if (!numberInColumn[iPivotColumn]) {
      int iNext=nextColumn[iPivotColumn];
      int iLast=lastColumn[iPivotColumn];
      assert (iLast>=0);
#ifdef PRINT_INVERT_STATS
      numberTakenOutOfUSpace++;
#endif
      lastColumn[iNext]=iLast;
      nextColumn[iLast]=iNext;
    }
#endif
  }
  // put back all rejected
  look = firstCount[numberRows_+1];
  while (look>=0) {
#ifndef NDEBUG
    if ( look < numberRows_ ) {
      assert (numberInRow[look]==1);
    } else {
      int iColumn = look - numberRows_;
      assert ( numberInColumn[iColumn] == 1 );
    }
#endif
    int nextLook=nextCount[look];
    modifyLink ( look, 1 );
    look=nextLook;
  }
#ifdef SMALL_PERMUTE
  if (numberGoodU_<numberRows_&&numberGoodU_>numberSlacks_+(numberRows_>>3)) {
    CoinSimplexInt * COIN_RESTRICT fromBigToSmallRow=reinterpret_cast<CoinSimplexInt *>(workArea_.array());
    CoinSimplexInt * COIN_RESTRICT fromBigToSmallColumn=fromBigToSmallRow+numberRows_;
    int nSmall=0;
    for (int i=0;i<numberRowsSmall_;i++) {
      int realRow=fromSmallToBigRow_[i];
      if (realRow<0) {
	fromBigToSmallRow[i]=-1;
      } else {
	// in
	fromBigToSmallRow[i]=nSmall;
	numberInRow[nSmall]=numberInRow[i];
	startRow[nSmall]=startRow[i];
	fromSmallToBigRow_[nSmall++]=realRow;
      }
    }
    nSmall=0;
    for (int i=0;i<numberRowsSmall_;i++) {
      int realColumn=fromSmallToBigColumn_[i];
      if (realColumn<0) {
	fromBigToSmallColumn[i]=-1;
      } else {
	// in
	fromBigToSmallColumn[i]=nSmall;
	numberInColumn[nSmall]=numberInColumn[i];
	numberInColumnPlus[nSmall]=numberInColumnPlus[i];
	startColumn[nSmall]=startColumn[i];
	fromSmallToBigColumn_[nSmall++]=realColumn;
      }
    }
    CoinAbcMemset0(numberInRow+nSmall,numberRowsSmall_-nSmall);
    CoinAbcMemset0(numberInColumn+nSmall,numberRowsSmall_-nSmall);
    // indices
    for (int i=0;i<numberRowsSmall_;i++) {
      CoinBigIndex start=startRow[i];
      CoinBigIndex end=start+numberInRow[i];
      for (CoinBigIndex j=start;j<end;j++) {
	indexColumn[j]=fromBigToSmallColumn[indexColumn[j]];
      }
    }
    for (int i=0;i<numberRowsSmall_;i++) {
      CoinBigIndex start=startColumn[i];
      CoinBigIndex end=start+numberInColumn[i];
      for (CoinBigIndex j=start;j<end;j++) {
	indexRow[j]=fromBigToSmallRow[indexRow[j]];
      }
    }
    // find area somewhere
    int * temp = fromSmallToBigColumn_+nSmall;
    CoinSimplexInt * nextFake=temp;
    //CoinSimplexInt lastFake=temp+numberRows_;
    CoinAbcMemcpy(nextFake,nextRow,numberRowsSmall_);
    nextFake[numberRows_]=nextRow[numberRows_];
    //CoinAbcMemcpy(lastFake,lastRow,numberRowsSmall_);
    // nextRow has end at numberRows_
    CoinSimplexInt lastBigRow=numberRows_;
    CoinSimplexInt lastSmallRow=numberRows_;
    for (CoinSimplexInt i=0;i<nSmall;i++) {
      CoinSimplexInt bigRow=nextFake[lastBigRow];
      assert (bigRow>=0&&bigRow<numberRowsSmall_);
      CoinSimplexInt smallRow=fromBigToSmallRow[bigRow];
      nextRow[lastSmallRow]=smallRow;
      lastRow[smallRow]=lastSmallRow;
      lastBigRow=bigRow;
      lastSmallRow=smallRow;
    }
    assert(nextFake[lastBigRow]==numberRows_);
    nextRow[lastSmallRow]=numberRows_;
    lastRow[numberRows_]=lastSmallRow;
    // nextColumn has end at maximumRowsExtra_
    CoinAbcMemcpy(nextFake,nextColumn,numberRowsSmall_);
    nextFake[maximumRowsExtra_]=nextColumn[maximumRowsExtra_];
    CoinSimplexInt lastBigColumn=maximumRowsExtra_;
    CoinSimplexInt lastSmallColumn=maximumRowsExtra_;
    for (CoinSimplexInt i=0;i<nSmall;i++) {
      CoinSimplexInt bigColumn=nextFake[lastBigColumn];
      assert (bigColumn>=0&&bigColumn<numberRowsSmall_);
      CoinSimplexInt smallColumn=fromBigToSmallColumn[bigColumn];
      nextColumn[lastSmallColumn]=smallColumn;
      lastColumn[smallColumn]=lastSmallColumn;
      lastBigColumn=bigColumn;
      lastSmallColumn=smallColumn;
    }
    assert(nextFake[lastBigColumn]==maximumRowsExtra_);
    nextColumn[lastSmallColumn]=maximumRowsExtra_;
    lastColumn[maximumRowsExtra_]=lastSmallColumn;
    // now equal counts (could redo - but for now get exact copy)
    CoinSimplexInt * COIN_RESTRICT lastCount = this->lastCountAddress_;
    CoinAbcMemcpy(temp,nextCount,numberRows_+numberRowsSmall_);
    for (int iCount=0;iCount<=nSmall;iCount++) {
      CoinSimplexInt nextBig=firstCount[iCount];
      if (nextBig>=0) {
	//CoinSimplexInt next=firstCount[iCount];
	CoinSimplexInt nextSmall;
	if (nextBig<numberRows_) 
	  nextSmall=fromBigToSmallRow[nextBig];
	else
	  nextSmall=fromBigToSmallColumn[nextBig-numberRows_]+numberRows_;
	firstCount[iCount]=nextSmall;
	CoinSimplexInt * where = &lastCount[nextSmall];
	while (nextBig>=0) {
	  CoinSimplexInt thisSmall=nextSmall;
	  nextBig=temp[nextBig];
	  if (nextBig>=0) {
	    if (nextBig<numberRows_) 
	      nextSmall=fromBigToSmallRow[nextBig];
	    else
	      nextSmall=fromBigToSmallColumn[nextBig-numberRows_]+numberRows_;
	    lastCount[nextSmall]=thisSmall;
	    nextCount[thisSmall]=nextSmall;
	  } else {
	    nextCount[thisSmall]=nextBig;
	  }
	}
	assert (nextSmall>=0);
	// fill in odd one
	*where=iCount-numberRows_-2;
      }
    }
    //printf("%d rows small1 %d small2 %d\n",numberRows_,numberRowsSmall_,nSmall);
    numberRowsSmall_=nSmall;
    //exit(0);
  }
#endif
  // Put .hpp functions in separate file for profiling
#ifdef PRINT_INVERT_STATS
  int numberGoodUY=numberGoodU_;
  int numberElsLeftX=0;
  for (int i=0;i<numberRows_;i++)
    numberElsLeftX+= numberInColumn[i];
  int saveN1X=totalElements_;
#endif
#if ABC_DENSE_CODE
  // when to go dense
  //CoinSimplexInt denseThreshold=abs(denseThreshold_);
#endif
  //get space for bit work area
  CoinBigIndex workSize = 1000;
  workArea2_.conditionalNew(workSize);
  workArea2Address_=workArea2_.array();
  CoinSimplexUnsignedInt *  COIN_RESTRICT workArea2 = workArea2Address_;

  //set markRow so no rows updated
  //set markRow so no rows updated
  CoinSimplexInt *  COIN_RESTRICT markRow = markRow_.array();
  CoinFillN ( markRow, numberRowsSmall_, LARGE_UNSET);
  CoinZeroN ( workArea, numberRowsSmall_ );
  CoinFactorizationDouble pivotTolerance = pivotTolerance_;
  CoinSimplexInt numberTrials = numberTrials_;
  CoinSimplexInt numberRows = numberRows_;
  // while just singletons - do singleton rows first
  CoinSimplexInt count = 1;
  startColumnL[numberGoodL_] = lengthL_; //for luck and first time
  numberRowsLeft_=numberRows_-numberGoodU_;
  while ( count <= numberRowsLeft_ ) {
    CoinBigIndex minimumCount = COIN_INT_MAX;
    CoinFactorizationDouble minimumCost = COIN_DBL_MAX;

    CoinSimplexInt iPivotRow = -1;
    CoinSimplexInt iPivotColumn = -1;
    CoinBigIndex pivotRowPosition = -1;
    CoinBigIndex pivotColumnPosition = -1;
    CoinSimplexInt look = firstCount[count];
#if 0
    if (numberRowsSmall_==2744&&!numberInColumn[1919]) {
      int look2=look;
      while (look2>=0) {
	if (look2==numberRows_+1919) {
	  printf("*** 1919 on list of count %d\n",count);
	  abort();
	}
        look2 = nextCount[look2];
      }
    }
#endif
    //printf("At count %d look %d\n",count,look);
    CoinSimplexInt trials = 0;
    //CoinSimplexInt *  COIN_RESTRICT pivotColumn = pivotColumnAddress_;
    while ( look >= 0 ) {
      checkLinks(1);
      if ( look < numberRows_ ) {
        CoinSimplexInt iRow = look;
        look = nextCount[look];
        bool rejected = false;
        CoinBigIndex start = startRow[iRow];
        CoinBigIndex end = start + count;
        
        CoinBigIndex i;
        for ( i = start; i < end; i++ ) {
          CoinSimplexInt iColumn = indexColumn[i];
          assert (numberInColumn[iColumn]>0);
          CoinFactorizationDouble cost = ( count - 1 ) * numberInColumn[iColumn] + 0.1;
          
          if ( cost < minimumCost ) {
            CoinBigIndex where = startColumn[iColumn];
            double minimumValue = element[where];
            
            minimumValue = fabs ( minimumValue ) * pivotTolerance;
	    if (count==1)
	      minimumValue=CoinMin(minimumValue,0.999999);
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
            } else if ( iPivotRow == -1 && count > 1) {
              rejected = true;
            }
          }
        }
        trials++;
        if ( iPivotRow >= 0 && (trials >= numberTrials||minimumCount<count) ) {
          look = -1;
          break;
        }
        if ( rejected ) {
          //take out for moment
          //eligible when row changes
          modifyLink ( iRow, numberRows_ + 1 );
        }
      } else {
        CoinSimplexInt iColumn = look - numberRows;
        
#if 0
        if ( numberInColumn[iColumn] != count ) {
	  printf("column %d has %d elements but in count list of %d\n",
		 iColumn,numberInColumn[iColumn],count);
	  abort();
	}
#endif
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
            CoinSimplexInt iRow = indexRow[i];
            CoinSimplexInt nInRow = numberInRow[iRow];
            assert (nInRow>0);
            CoinFactorizationDouble cost = ( count - 1 ) * nInRow;
            
            if ( cost < minimumCost ) {
              minimumCost = cost;
              minimumCount = nInRow;
              iPivotRow = iRow;
              pivotRowPosition = i;
              iPivotColumn = iColumn;
              assert (iPivotRow>=0&&iPivotColumn>=0);
              pivotColumnPosition = -1;
            }
          }
        }
        trials++;
        if ( iPivotRow >= 0 && (trials >= numberTrials||minimumCount<count) ) {
          look = -1;
          break;
        }
      }
    }				/* endwhile */
    if (iPivotRow>=0) {
      assert (iPivotRow<numberRows_);
      CoinSimplexInt numberDoRow = numberInRow[iPivotRow] - 1;
      CoinSimplexInt numberDoColumn = numberInColumn[iPivotColumn] - 1;
      
	//printf("realPivotRow %d (%d elements) realPivotColumn %d (%d elements)\n",
	//   fromSmallToBigRow[iPivotRow],numberDoRow+1,fromSmallToBigColumn[iPivotColumn],numberDoColumn+1);
      //      printf("nGoodU %d pivRow %d (num %d) pivCol %d (num %d) - %d elements left\n",
      //   numberGoodU_,iPivotRow,numberDoRow,iPivotColumn,numberDoColumn,totalElements_);
#if 0 //ndef NDEBUG
      static int testXXXX=0;
      testXXXX++;
      if ((testXXXX%100)==0)
	printf("check consistent totalElements_\n");
      int nn=0;
      for (int i=0;i<numberRowsSmall_;i++) {
	int n=numberInColumn[i];
	if (n) {
	  int start=startColumn[i];
	  double largest=fabs(element[start]);
	  for (int j=1;j<n;j++) {
	    assert (fabs(element[start+j])<=largest);
	  }
	  nn+=n;
	}
      }
      assert (nn==totalElements_);
#endif
      totalElements_ -= ( numberDoRow + numberDoColumn + 1 );
#if 0
      if (numberInColumn[5887]==0&&numberRowsSmall_>5887) {
	int start=startRow[1181];
	int end=start+numberInRow[1181];
	for (int i=start;i<end;i++)
	  assert(indexColumn[i]!=5887);
      }
#endif
      if ( numberDoColumn > 0 ) {
	if ( numberDoRow > 0 ) {
	  if ( numberDoColumn > 1 ) {
	    //  if (1) {
	    //need to adjust more for cache and SMP
	    //allow at least 4 extra
	    CoinSimplexInt increment = numberDoColumn + 1 + 4;
            
	    if ( increment & 15 ) {
	      increment = increment & ( ~15 );
	      increment += 16;
	    }
	    CoinSimplexInt increment2 =
	      
	      ( increment + COINFACTORIZATION_BITS_PER_INT - 1 ) >> COINFACTORIZATION_SHIFT_PER_INT;
	    CoinBigIndex size = increment2 * numberDoRow;
            
	    if ( size > workSize ) {
	      workSize = size;
	      workArea2_.conditionalNew(workSize);
	      workArea2Address_=workArea2_.array();
	      workArea2 = workArea2Address_;
	    }
	    //branch out to best pivot routine 
#define ABC_PARALLEL_PIVOT
#ifndef ABC_USE_FUNCTION_POINTERS
#undef ABC_PARALLEL_PIVOT
#endif
#ifdef ABC_PARALLEL_PIVOT
	    //if (numberRowsSmall_==7202&&numberGoodU_==15609) {
	    //printf("NumberGoodU %d\n",numberGoodU_);
	    //}
	    if (numberDoRow<20)
	      status = pivot ( iPivotRow, iPivotColumn,
				  pivotRowPosition, pivotColumnPosition,
				  workArea, workArea2, 
				  increment2,  markRow );
	    else
	      status = pivot ( iPivotRow, iPivotColumn,
				  pivotRowPosition, pivotColumnPosition,
				  markRow );
#else
	    status = pivot ( iPivotRow, iPivotColumn,
				pivotRowPosition, pivotColumnPosition,
				workArea, workArea2, 
				increment2,  markRow );
#endif
#if 0
	    for (int i=0;i<numberRowsSmall_;i++)
	      assert (markRow[i]==LARGE_UNSET);
#endif
	    //#define CHECK_SIZE
#ifdef CHECK_SIZE
	    {
	      double largestU=0.0;
	      int iU=-1;
	      int jU=-1;
	      for (int i=0;i<numberRowsSmall_;i++) {
		CoinBigIndex start = startColumn[i];
		CoinBigIndex end = start + numberInColumn[i];
		for (int j=start;j<end;j++) {
		  if (fabs(element[j])>largestU) {
		    iU=i;
		    jU=j;
		    largestU=fabs(element[j]);
		  }
		}
	      }
	      printf("%d largest el %g at i=%d,j=%d start %d end %d count %d\n",numberGoodU_,element[jU],
		     iU,jU,startColumn[iU],startColumn[iU]+numberInColumn[iU],count);
	    }
#endif
#undef CHECK_SIZE
    checkLinks();
	    if ( status < 0) {
	      count=numberRows_+1;
	      break;
	    }
	  } else {
	    if ( !pivotOneOtherRow ( iPivotRow, iPivotColumn ) ) {
	      status = -99;
	      count=numberRows_+1;
	      break;
	    }
#ifdef CHECK_SIZE
	    {
	      double largestU=0.0;
	      int iU=-1;
	      int jU=-1;
	      for (int i=0;i<numberRowsSmall_;i++) {
		CoinBigIndex start = startColumn[i];
		CoinBigIndex end = start + numberInColumn[i];
		for (int j=start;j<end;j++) {
		  if (fabs(element[j])>largestU) {
		    iU=i;
		    jU=j;
		    largestU=fabs(element[j]);
		  }
		}
	      }
	      printf("B%d largest el %g at i=%d,j=%d\n",numberGoodU_,element[jU],
		     iU,jU);
	    }
#endif
    checkLinks();
	  }
	} else {
	  assert (!numberDoRow);
    checkLinks();
	  if ( !pivotRowSingleton ( iPivotRow, iPivotColumn ) ) {
	    status = -99;
	    count=numberRows_+1;
	    break;
	  }
#ifdef CHECK_SIZE
	    {
	      double largestU=0.0;
	      int iU=-1;
	      int jU=-1;
	      for (int i=0;i<numberRowsSmall_;i++) {
		CoinBigIndex start = startColumn[i];
		CoinBigIndex end = start + numberInColumn[i];
		for (int j=start;j<end;j++) {
		  if (fabs(element[j])>largestU) {
		    iU=i;
		    jU=j;
		    largestU=fabs(element[j]);
		  }
		}
	      }
	      printf("C%d largest el %g at i=%d,j=%d\n",numberGoodU_,element[jU],
		     iU,jU);
	    }
#endif
    checkLinks();
	}
      } else {
	assert (!numberDoColumn);
	pivotColumnSingleton ( iPivotRow, iPivotColumn );
#ifdef CHECK_SIZE
	    {
	      double largestU=0.0;
	      int iU=-1;
	      int jU=-1;
	      for (int i=0;i<numberRowsSmall_;i++) {
		CoinBigIndex start = startColumn[i];
		CoinBigIndex end = start + numberInColumn[i];
		for (int j=start;j<end;j++) {
		  if (fabs(element[j])>largestU) {
		    iU=i;
		    jU=j;
		    largestU=fabs(element[j]);
		  }
		}
	      }
	      printf("D%d largest el %g at i=%d,j=%d\n",numberGoodU_,element[jU],
		     iU,jU);
	    }
#endif
    checkLinks();
      }
      double pivotValue=fabs(pivotRegion[numberGoodU_]);
#if ABC_NORMAL_DEBUG>0
      largestPivot=CoinMax(largestPivot,pivotValue);
      smallestPivot=CoinMin(smallestPivot,pivotValue);
#endif
      if (pivotValue<initialLargest) {
	if (pivotTolerance_<0.95) {
	  pivotTolerance_= CoinMin(1.1*pivotTolerance_,0.99);
#if ABC_NORMAL_DEBUG>0
	  printf("PPPivot value of %g increasing pivot tolerance to %g\n",
		 pivotValue,pivotTolerance_);
#endif
	  status=-97;
	  break;
	}
      }
      afterPivot(iPivotRow,iPivotColumn);
      assert (numberGoodU_<=numberRows_);
      assert(startRow[numberRows_]==lengthAreaU_);
#if 0
      static int ixxxxxx=0;
      {
	int nLeft=0;
	for (int i=0;i<numberRows_;i++) {
	  if (permuteAddress_[i]<0) {
	    nLeft++;
	  }
	}
	assert (nLeft==numberRowsLeft_);
	if (nLeft!=numberRowsLeft_) {
	  printf("mismatch nleft %d numberRowsleft_ %d\n",nLeft,numberRowsLeft_);
	  abort();
	}
      }
      if (numberRowsSmall_==-262/*2791*/) {
	bool bad=false;
#if 0
	int cols[2800];
	for (int i=0;i<2791;i++)
	  cols[i]=-1;
	int n=numberInRow[1714];
	int start = startRow[1714];
	int end=start+n;
	for (int i=start;i<end;i++) {
	  int iColumn=indexColumn[i];
	  if (iColumn<0||iColumn>=2744) {
	    printf("Bad column %d at %d\n",iColumn,i);
	    bad=true;
	  } else if (cols[iColumn]>=0) {
	    printf("Duplicate column %d at %d was at %d\n",iColumn,i,cols[iColumn]);
	    bad=true;
	  } else {
	    cols[iColumn]=i;
	  }
	}
#else
	{
	  int n=numberInRow[1160];
	  int start = startRow[1160];
	  int end=start+n;
	  for (int i=start;i<end;i++) {
	    int iColumn=indexColumn[i];
	    if (iColumn==2111&&!numberInColumn[2111]) {
	      printf("Bad column %d at %d\n",iColumn,i);
	      abort();
	    }
	  }
	}
#endif
	ixxxxxx++;
	printf("pivrow %d pivcol %d count in 2111 %d in row 2111 %d xxx %d ngood %d\n",iPivotRow,
	       iPivotColumn,numberInColumn[2111],numberInRow[2111],ixxxxxx,numberGoodU_);
	//printf("pivrow %d pivcol %d numberGoodU_ %d xxx %d\n",iPivotRow,
	//     iPivotColumn,numberGoodU_,ixxxxxx);
	if (bad)
	  abort();
#if 0
	if (numberInRow[1714]>numberRows_-numberGoodU_) {
	  printf("numberGoodU %d nrow-ngood %d\n",numberGoodU_,numberRows_-numberGoodU_);
	  abort();
	}
	if (ixxxxxx==2198||ixxxxxx==1347) {
	  printf("Trouble ahead\n");
	}
#else
	if (ixxxxxx==1756) {
	  printf("Trouble ahead\n");
	}
#endif
      }
#endif
#if ABC_DENSE_CODE
      if (!status) {
	status=wantToGoDense();
      }
#endif
      if (status)
	break;
      // start at 1 again
      count = 1;
    } else {
      //end of this - onto next
      count++;
    } 
  }				/* endwhile */
#if ABC_NORMAL_DEBUG>0
#if ABC_SMALL<2
  int lenU=2*(lastEntryByColumnUPlus_/3);
  printf("largest initial element %g, smallestPivot %g largest %g (%d dense rows) - %d in L, %d in U\n",
	 1.0e-10/initialLargest,smallestPivot,largestPivot,numberRows_-numberGoodU_,
	 lengthL_,lenU);
#endif
#endif
  workArea2_.conditionalDelete() ;
  workArea2Address_=NULL;
#ifdef PRINT_INVERT_STATS
  int numberDense=numberRows_-numberGoodU_;
  printf("XX %d rows, %d in bump (%d slacks,%d triangular), % d dense - startels %d (%d) now %d - taken out of uspace (triang) %d\n",
	 numberRows_,numberRows_-numberGoodUY,numberGoodUX,numberGoodUY-numberGoodUX,
	 numberDense,numberElsLeftX,saveN1X,totalElements_,numberTakenOutOfUSpace);
#endif
  return status;
}
#if ABC_DENSE_CODE
//:method factorDense.  Does dense phase of factorization
//return code is <0 error, 0= finished
CoinSimplexInt CoinAbcTypeFactorization::factorDense()
{
  CoinSimplexInt status=0;
  numberDense_=numberRows_-numberGoodU_;
  if (sizeof(CoinBigIndex)==4&&numberDense_>=(2<<15)) {
    abort();
  } 
  CoinBigIndex full;
  full = numberDense_*numberDense_;
  totalElements_=full;
  // Add coding to align an object
#if ABC_DENSE_CODE==1
  leadingDimension_=((numberDense_>>4)+1)<<4;
#else
  leadingDimension_=numberDense_;
#endif
  CoinBigIndex newSize=(leadingDimension_+FACTOR_CPU)*numberDense_;
  newSize += (numberDense_+1)/(sizeof(CoinFactorizationDouble)/sizeof(CoinSimplexInt));
#if 1 
  newSize += 2*((numberDense_+3)/(sizeof(CoinFactorizationDouble)/sizeof(short)));
  newSize += ((numberRows_+3)/(sizeof(CoinFactorizationDouble)/sizeof(short)));
  // so we can align on 256 byte
  newSize+=32;
  //newSize += (numberDense_+1)/(sizeof(CoinFactorizationDouble)/sizeof(CoinSimplexInt));
#endif
#ifdef SMALL_PERMUTE
  // densePermute has fromSmallToBig!!!
  CoinSimplexInt * COIN_RESTRICT fromSmallToBigRow=reinterpret_cast<CoinSimplexInt *>(workArea_.array());
  CoinSimplexInt * COIN_RESTRICT fromSmallToBigColumn = fromSmallToBigRow+numberRowsSmall_;
  CoinAbcMemcpy(fromSmallToBigRow,fromSmallToBigRow_,numberRowsSmall_);
  CoinAbcMemcpy(fromSmallToBigColumn,fromSmallToBigColumn_,numberRowsSmall_);
#endif
  denseArea_.conditionalDelete();
  denseArea_.conditionalNew( newSize );
  denseAreaAddress_=denseArea_.array();
  CoinInt64 xx = reinterpret_cast<CoinInt64>(denseAreaAddress_);
  int iBottom = static_cast<int>(xx & 63);
  int offset = (256-iBottom)>>3;
  denseAreaAddress_ += offset;
  CoinFactorizationDouble *  COIN_RESTRICT denseArea = denseAreaAddress_;
  CoinZeroN(denseArea,newSize-32);
  CoinSimplexInt *  COIN_RESTRICT densePermute=
    reinterpret_cast<CoinSimplexInt *>(denseArea+(leadingDimension_+FACTOR_CPU)*numberDense_);
#if ABC_DENSE_CODE<0
  CoinSimplexInt *  COIN_RESTRICT indexRowU = indexRowUAddress_;
  CoinSimplexInt *  COIN_RESTRICT numberInColumnPlus = numberInColumnPlusAddress_;
#endif
  //mark row lookup using lastRow
  CoinSimplexInt i;
  CoinSimplexInt *  COIN_RESTRICT lastRow = lastRowAddress_;
  CoinSimplexInt *  COIN_RESTRICT numberInColumn = numberInColumnAddress_;
#if 0
  char xxx[17000];
  assert (numberRows_<=17000);
  memset(xxx,0,numberRows_);
  int nLeft=0;
  for (i=0;i<numberRows_;i++) {
    if (permuteAddress_[i]>=0) {
      xxx[i]=1;
    } else {
      nLeft++;
    }
  }
  printf("%d rows left\n",nLeft);
  bool bad=false;
  for (i=0;i<numberRowsSmall_;i++) {
    int iBigRow=fromSmallToBigRow[i];
    if (!xxx[iBigRow])
      printf("Row %d (big %d) in dense\n",i,iBigRow);
    if(iBigRow<0) abort();
    if (lastRow[i]>=0) {
      lastRow[i]=0;
      printf("dense row %d translates to %d permute %d\n",i,iBigRow,permuteAddress_[iBigRow]);
    } else {
      if (permuteAddress_[iBigRow]<0)
      printf("sparse row %d translates to %d permute %d\n",i,iBigRow,permuteAddress_[iBigRow]);
      if (xxx[iBigRow]!=1) bad=true;
      xxx[iBigRow]=0;
    }
  }
  if (bad)
    abort();
#else
  for (i=0;i<numberRowsSmall_;i++) {
    if (lastRow[i]>=0) {
      lastRow[i]=0;
    }
  }
#endif
  CoinSimplexInt *  COIN_RESTRICT indexRow = indexRowUAddress_;
  CoinFactorizationDouble *  COIN_RESTRICT element = elementUAddress_;
  CoinSimplexInt which=0;
  for (i=0;i<numberRowsSmall_;i++) {
    if (!lastRow[i]) {
      lastRow[i]=which;
#ifdef SMALL_PERMUTE
      int realRow=fromSmallToBigRow[i];
      //if (xxx[realRow]!=0) abort();
      //xxx[realRow]=-1;
      permuteAddress_[realRow]=numberGoodU_+which;
      densePermute[which]=realRow;
#else	
      permuteAddress_[i]=numberGoodU_+which;
      densePermute[which]=i;
#endif
      which++;
    }
  } 
  //for (i=0;i<numberRowsSmall_;i++) {
  //int realRow=fromSmallToBigRow[i];
  //if(xxx[realRow]>0) abort();
  //}
  //for L part
  CoinBigIndex *  COIN_RESTRICT startColumnL = startColumnLAddress_;
#if ABC_DENSE_CODE<0
  CoinFactorizationDouble *  COIN_RESTRICT elementL = elementLAddress_;
  CoinSimplexInt *  COIN_RESTRICT indexRowL = indexRowLAddress_;
#endif
  CoinBigIndex endL=startColumnL[numberGoodL_];
  //take out of U
  CoinFactorizationDouble *  COIN_RESTRICT column = denseArea;
  CoinSimplexInt rowsDone=0;
  CoinSimplexInt iColumn=0;
  CoinSimplexInt *  COIN_RESTRICT pivotColumn = pivotColumnAddress_;
  CoinFactorizationDouble *  COIN_RESTRICT pivotRegion = pivotRegionAddress_;
  CoinBigIndex * startColumnU = startColumnUAddress_;
#ifdef ABC_USE_FUNCTION_POINTERS
  scatterStruct * COIN_RESTRICT scatter = scatterUColumn();
#if ABC_USE_FUNCTION_POINTERS
  extern scatterUpdate AbcScatterLowSubtract[9];
  extern scatterUpdate AbcScatterHighSubtract[4];
#endif
  CoinFactorizationDouble *  COIN_RESTRICT elementUColumnPlus = elementUColumnPlusAddress_;
  CoinSimplexInt *  COIN_RESTRICT numberInColumnPlus = numberInColumnPlusAddress_;
#endif
#if ABC_DENSE_CODE==2
  int nDense=0;
#endif
  for (iColumn=0;iColumn<numberRowsSmall_;iColumn++) {
    if (numberInColumn[iColumn]) {
#if ABC_DENSE_CODE==2
      nDense++;
#endif
      //move
      CoinBigIndex start = startColumnU[iColumn];
      CoinSimplexInt number = numberInColumn[iColumn];
      CoinBigIndex end = start+number;
      for (CoinBigIndex i=start;i<end;i++) {
        CoinSimplexInt iRow=indexRow[i];
        iRow=lastRow[iRow];
	assert (iRow>=0&&iRow<numberDense_);
#if ABC_DENSE_CODE!=2
        column[iRow]=element[i];
#else
#if BLOCKING8==8
	int iBlock=iRow>>3;
#elif BLOCKING8==4
	int iBlock=iRow>>2;
#elif BLOCKING8==2
	int iBlock=iRow>>1;
#else
	abort();
#endif
	iRow=iRow&(BLOCKING8-1);
	column[iRow+BLOCKING8X8*iBlock]=element[i];
#endif
      } /* endfor */
#if ABC_DENSE_CODE!=2
      column+=leadingDimension_;
#else
      if ((nDense&(BLOCKING8-1))!=0)
	column += BLOCKING8;
      else
	column += leadingDimension_*BLOCKING8-(BLOCKING8-1)*BLOCKING8;
#endif
      while (lastRow[rowsDone]<0) {
        rowsDone++;
      } /* endwhile */
#ifdef ABC_USE_FUNCTION_POINTERS
      int numberIn = numberInColumnPlus[iColumn];
#ifdef SMALL_PERMUTE
      int realRowsDone=fromSmallToBigRow[rowsDone];
#if ABC_USE_FUNCTION_POINTERS
      if (numberIn<9) {
	scatter[realRowsDone].functionPointer=AbcScatterLowSubtract[numberIn];
      } else {
	scatter[realRowsDone].functionPointer=AbcScatterHighSubtract[numberIn&3];
      }
#endif
      scatter[realRowsDone].offset=lastEntryByColumnUPlus_;
      scatter[realRowsDone].number=numberIn;
#else
#if ABC_USE_FUNCTION_POINTERS
      if (numberIn<9) {
	scatter[rowsDone].functionPointer=AbcScatterLowSubtract[numberIn];
      } else {
	scatter[rowsDone].functionPointer=AbcScatterHighSubtract[numberIn&3];
      }
#endif
      scatter[rowsDone].offset=lastEntryByColumnUPlus_;
      scatter[rowsDone].number=numberIn;
#endif
      CoinBigIndex startC = start-numberIn;
      for (int i=startC;i<startC+numberIn;i++)
	elementUColumnPlus[lastEntryByColumnUPlus_++]=element[i];
      CoinAbcMemcpy(reinterpret_cast<CoinSimplexInt *>(elementUColumnPlus+lastEntryByColumnUPlus_),indexRow+startC,numberIn);
      lastEntryByColumnUPlus_ += (numberIn+1)>>1;
#endif
#ifdef SMALL_PERMUTE
      permuteAddress_[realRowsDone]=numberGoodU_;
      pivotColumn[numberGoodU_]=fromSmallToBigColumn[iColumn];
#else
      permuteAddress_[rowsDone]=numberGoodU_;
      pivotColumn[numberGoodU_]=iColumn;
#endif
      rowsDone++;
      startColumnL[numberGoodU_+1]=endL;
      numberInColumn[iColumn]=0;
      pivotRegion[numberGoodU_]=1.0;
      numberGoodU_++;
    } 
  } 
#if ABC_DENSE_CODE>0
  //printf("Going dense at %d\n",numberDense_); 
  if (denseThreshold_>0) {
    if(numberGoodU_!=numberRows_)
      return -1; // something odd
    numberGoodL_=numberRows_;
    //now factorize
    //dgef(denseArea,&numberDense_,&numberDense_,densePermute);
#if ABC_DENSE_CODE!=2
#ifndef CLAPACK
    CoinSimplexInt info;
    F77_FUNC(dgetrf,DGETRF)(&numberDense_,&numberDense_,denseArea,&leadingDimension_,densePermute,
			    &info);
    
    // need to check size of pivots
    if(info)
      status = -1;
#else
    status = clapack_dgetrf(CblasColMajor, numberDense_,numberDense_,
			    denseArea, leadingDimension_,densePermute);
    assert (!status);
#endif
#else
    status=CoinAbcDgetrf(numberDense_,numberDense_,denseArea,numberDense_,densePermute
#if ABC_PARALLEL==2
			  ,parallelMode_
#endif
);
    if (status) {
      status=-1;
      printf("singular in dense\n");
    }
#endif
    return status;
  } 
#else
  numberGoodU_ = numberRows_-numberDense_;
  CoinSimplexInt base = numberGoodU_;
  CoinSimplexInt iDense;
  CoinSimplexInt numberToDo=numberDense_;
  denseThreshold_=-abs(denseThreshold_); //0;
  CoinSimplexInt *  COIN_RESTRICT nextColumn = nextColumnAddress_;
  const CoinSimplexInt *  COIN_RESTRICT pivotColumnConst = pivotColumnAddress_;
  // make sure we have enough space in L and U
  for (iDense=0;iDense<numberToDo;iDense++) {
    //how much space have we got
    iColumn = pivotColumnConst[base+iDense];
    CoinSimplexInt next = nextColumn[iColumn];
    CoinSimplexInt numberInPivotColumn = iDense;
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
  CoinFactorizationDouble * COIN_RESTRICT elementU = elementUAddress_;
  CoinSimplexInt *  COIN_RESTRICT ipiv = densePermute-numberDense_;
  int returnCode=CoinAbcDgetrf(numberDense_,numberDense_,denseArea,numberDense_,ipiv);
  if (!returnCode) {
    CoinSimplexDouble *  COIN_RESTRICT element = denseArea;
    for (int iDense=0;iDense<numberToDo;iDense++) {
      int pivotRow=ipiv[iDense];
      // get original row
      CoinSimplexInt originalRow = densePermute[pivotRow];
      // swap
      densePermute[pivotRow]=densePermute[iDense];
      densePermute[iDense] = originalRow;
    }
    for (int iDense=0;iDense<numberToDo;iDense++) {
      int iColumn = pivotColumnConst[base+iDense];
      // get original row
      CoinSimplexInt originalRow = densePermute[iDense];
      // do nextRow
      lastRow[originalRow] = -2; //mark
      permuteAddress_[originalRow]=numberGoodU_;
      CoinFactorizationDouble pivotMultiplier = element[iDense];
      //printf("pivotMultiplier %g\n",pivotMultiplier);
      pivotRegion[numberGoodU_] = pivotMultiplier;
      // Do L
      CoinBigIndex l = lengthL_;
      startColumnL[numberGoodL_] = l;	//for luck and first time
      for (int iRow=iDense+1;iRow<numberDense_;iRow++) {
	CoinFactorizationDouble value = element[iRow];
	if (!TEST_LESS_THAN_TOLERANCE(value)) {
	  //printf("L %d row %d value %g\n",l,densePermute[iRow],value);
	  indexRowL[l] = densePermute[iRow];
	  elementL[l++] = value;
	}
      }
      numberGoodL_++;
      lengthL_ = l;
      startColumnL[numberGoodL_] = l;
      // update U column
      CoinBigIndex start = startColumnU[iColumn];
      for (int iRow=0;iRow<iDense;iRow++) {
	if (!TEST_LESS_THAN_TOLERANCE(element[iRow])) {
	  //printf("U %d row %d value %g\n",start,densePermute[iRow],element[iRow]);
	  indexRowU[start] = densePermute[iRow];
	  elementU[start++] = element[iRow];
	}
      }
      element += numberDense_;
      numberInColumn[iColumn]=0;
      numberInColumnPlus[iColumn] += start-startColumnU[iColumn];
      startColumnU[iColumn]=start;
      numberGoodU_++;
    }
  } else {
    return -1;
  }
  numberDense_=0;
#endif
  return status;
}
#endif
#if 0
// Separate out links with same row/column count
void 
CoinAbcTypeFactorization::separateLinks()
{
  const CoinSimplexInt count=1;
  CoinSimplexInt * COIN_RESTRICT nextCount = this->nextCountAddress_;
  CoinSimplexInt * COIN_RESTRICT firstCount = this->firstCountAddress_;
  CoinSimplexInt * COIN_RESTRICT lastCount = this->lastCountAddress_;
  CoinSimplexInt next = firstCount[count];
  CoinSimplexInt firstRow=-1;
  CoinSimplexInt firstColumn=-1;
  CoinSimplexInt lastRow=-1;
  CoinSimplexInt lastColumn=-1;
  while(next>=0) {
    CoinSimplexInt next2=nextCount[next];
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
  if (firstRow>=0) {
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
#endif
#endif
