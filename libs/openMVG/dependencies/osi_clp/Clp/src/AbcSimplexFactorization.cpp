/* $Id$ */
// Copyright (C) 2002, International Business Machines
// Corporation and others, Copyright (C) 2012, FasterCoin.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).
#define USE_DENSE_FAC -1
#define USE_SMALL_FAC 200
#define USE_LONG_FAC 10000
#include "CoinPragma.hpp"
#include "AbcSimplexFactorization.hpp"
#include "ClpFactorization.hpp"
#include "ClpMessage.hpp"
#include "CoinAbcCommon.hpp"
#include "CoinHelperFunctions.hpp"
#include "CoinIndexedVector.hpp"
#include "AbcSimplex.hpp"
#include "AbcSimplexDual.hpp"
#include "AbcMatrix.hpp"
#include "CoinAbcFactorization.hpp"
//#include "CoinDenseFactorization.hpp"
#ifdef ABC_JUST_ONE_FACTORIZATION
#define CoinAbcFactorization CoinAbcBaseFactorization
#define CoinAbcSmallFactorization CoinAbcBaseFactorization
#define CoinAbcLongFactorization CoinAbcBaseFactorization
#define CoinAbcOrderedFactorization CoinAbcBaseFactorization
#endif
#ifndef ABC_LONG_FACTORIZATION
#undef CoinAbcLongFactorization
#define CoinAbcLongFactorization CoinAbcOrderedFactorization
#endif
#ifdef ABC_TEMPORARY_FACTORIZATION
#undef CoinAbcSmallFactorization
#define CoinAbcSmallFactorization CoinAbcOrderedFactorization
#endif
//-------------------------------------------------------------------
// Default Constructor
//-------------------------------------------------------------------
AbcSimplexFactorization::AbcSimplexFactorization (int /*numberRows*/)
{
  model_=NULL;
  coinAbcFactorization_ = new CoinAbcFactorization();
  forceB_ = 0;
  goDenseThreshold_ = USE_DENSE_FAC;
  goSmallThreshold_ = USE_SMALL_FAC;
  goLongThreshold_ = USE_LONG_FAC;
  numberSlacks_=0;
}

//-------------------------------------------------------------------
// Copy constructor
//-------------------------------------------------------------------
AbcSimplexFactorization::AbcSimplexFactorization (const AbcSimplexFactorization & rhs,
						  int denseIfSmaller)
{
  forceB_ = rhs.forceB_;
  goDenseThreshold_ = rhs.goDenseThreshold_;
  goSmallThreshold_ = rhs.goSmallThreshold_;
  goLongThreshold_ = rhs.goLongThreshold_;
  numberSlacks_=rhs.numberSlacks_;
  int goDense = 0;
  model_=rhs.model_;
  if (denseIfSmaller > 0 && denseIfSmaller <= goDenseThreshold_) {
    CoinAbcDenseFactorization * denseR =
      dynamic_cast<CoinAbcDenseFactorization *>(rhs.coinAbcFactorization_);
    if (!denseR)
      goDense = 1;
  }
  if (denseIfSmaller > 0 && !rhs.coinAbcFactorization_) {
    if (denseIfSmaller <= goDenseThreshold_)
      goDense = 1;
    else if (denseIfSmaller <= goSmallThreshold_)
      goDense = 2;
    else if (denseIfSmaller >= goLongThreshold_)
      goDense = 3;
  } else if (denseIfSmaller < 0) {
    if (-denseIfSmaller <= goDenseThreshold_)
      goDense = 1;
    else if (-denseIfSmaller <= goSmallThreshold_)
      goDense = 2;
    else if (-denseIfSmaller >= goLongThreshold_)
      goDense = 3;
  }
  if (rhs.coinAbcFactorization_ && (denseIfSmaller >= 0 || !goDense))
    coinAbcFactorization_ = rhs.coinAbcFactorization_->clone();
  else
    coinAbcFactorization_ = NULL;
  if (goDense) {
    delete coinAbcFactorization_;
    if (goDense == 1)
      coinAbcFactorization_ = new CoinAbcDenseFactorization();
    else if (goDense == 2)
      coinAbcFactorization_ = new CoinAbcSmallFactorization();
    else if (goDense == 3)
      coinAbcFactorization_ = new CoinAbcLongFactorization();
    else
      coinAbcFactorization_ = new CoinAbcFactorization();
    assert (coinAbcFactorization_);
    coinAbcFactorization_->maximumPivots(rhs.coinAbcFactorization_->maximumPivots());
    coinAbcFactorization_->pivotTolerance(rhs.coinAbcFactorization_->pivotTolerance());
    coinAbcFactorization_->zeroTolerance(rhs.coinAbcFactorization_->zeroTolerance());
  }
}
//-------------------------------------------------------------------
// Destructor
//-------------------------------------------------------------------
AbcSimplexFactorization::~AbcSimplexFactorization ()
{
  delete coinAbcFactorization_;
}

//----------------------------------------------------------------
// Assignment operator
//-------------------------------------------------------------------
AbcSimplexFactorization &
AbcSimplexFactorization::operator=(const AbcSimplexFactorization& rhs)
{
  if (this != &rhs) {
    forceB_ = rhs.forceB_;
    model_=rhs.model_;
    goDenseThreshold_ = rhs.goDenseThreshold_;
    goSmallThreshold_ = rhs.goSmallThreshold_;
    goLongThreshold_ = rhs.goLongThreshold_;
    numberSlacks_=rhs.numberSlacks_;
    
    if (rhs.coinAbcFactorization_) {
      delete coinAbcFactorization_;
      coinAbcFactorization_ = rhs.coinAbcFactorization_->clone();
    }
  }
  return *this;
}
// Go over to dense code
void
AbcSimplexFactorization::goDenseOrSmall(int numberRows)
{
  if (!forceB_) {
    delete coinAbcFactorization_;
    if (numberRows <= goDenseThreshold_) {
      coinAbcFactorization_ = new CoinAbcDenseFactorization();
    } else if (numberRows <= goSmallThreshold_) {
      coinAbcFactorization_ = new CoinAbcSmallFactorization();
    } else if (numberRows >= goLongThreshold_) {
      coinAbcFactorization_ = new CoinAbcLongFactorization();
    } else {
      coinAbcFactorization_ = new CoinAbcFactorization();
    }
  }
}
// If nonzero force use of 1,dense 2,small 3,long
void
AbcSimplexFactorization::forceOtherFactorization(int which)
{
  delete coinAbcFactorization_;
  forceB_ = 0;
  coinAbcFactorization_ = NULL;
  if (which > 0 && which < 6) {
    forceB_ = which;
    switch (which) {
    case 1:
      coinAbcFactorization_ = new CoinAbcDenseFactorization();
      goDenseThreshold_ = COIN_INT_MAX;
      break;
    case 2:
    case 4:
      coinAbcFactorization_ = new CoinAbcSmallFactorization();
      goSmallThreshold_ = COIN_INT_MAX;
      break;
    case 3:
    case 5:
      coinAbcFactorization_ = new CoinAbcLongFactorization();
      goLongThreshold_ = 0;
      break;
    }
  } else {
    coinAbcFactorization_ = new CoinAbcFactorization();
  }
}
// Synchronize stuff
void 
AbcSimplexFactorization::synchronize(const ClpFactorization * otherFactorization,const AbcSimplex * model)
{
  goDenseThreshold_=otherFactorization->goDenseThreshold();
  goSmallThreshold_=otherFactorization->goSmallThreshold();
  goLongThreshold_=otherFactorization->goOslThreshold();
  //forceOtherFactorization(otherFactorization->typeOfFactorization());
  goDenseOrSmall(model->numberRows());
  maximumPivots(static_cast<int>(otherFactorization->maximumPivots()*1.2));
}
int
AbcSimplexFactorization::factorize ( AbcSimplex * model,
				     int solveType, bool valuesPass)
{
  model_= model;
  AbcMatrix * matrix = model->abcMatrix();
  int numberRows = model->numberRows();
  if (!numberRows)
    return 0;
  bool anyChanged = false;
  coinAbcFactorization_->setStatus(-99);
  const int *  COIN_RESTRICT pivotVariable = model->pivotVariable();
  //returns 0 -okay, -1 singular, -2 too many in basis */
  // allow dense
  int solveMode = coinAbcFactorization_->solveMode()&1;
  if (model->numberIterations()>model->baseIteration())
    solveMode += 8;
  else
    solveMode = 1; // try dense
  if (valuesPass)
    solveMode += 4;
  coinAbcFactorization_->setSolveMode(solveMode);
  while (status() < -98) {
    
    int i;
    int numberBasic = 0;
    // Move pivot variables across if they look good
    int *  COIN_RESTRICT pivotTemp = model->usefulArray(0)->getIndices();
#ifndef NDEBUG
    model_->checkArrays();
#endif
    assert (!model->usefulArray(0)->getNumElements());
    // Seems to prefer things in order so quickest
    // way is to go though like this
    for (i = 0; i < numberRows; i++) {
      if (pivotVariable[i]<numberRows)
	pivotTemp[numberBasic++] = pivotVariable[i];
    }
    numberSlacks_ = numberBasic;
    /* Put column basic variables into pivotVariable
    */
    for (i = 0; i < numberRows; i++) {
      if (pivotVariable[i]>=numberRows)
	pivotTemp[numberBasic++] = pivotVariable[i];
    }
    CoinBigIndex numberElements = numberSlacks_;
    
    // compute how much in basis
    int numberColumnBasic = numberBasic - numberSlacks_;
    
    numberElements += matrix->countBasis(pivotTemp + numberSlacks_,
					 numberColumnBasic);
    //printf("Basis has %d slacks - size %d\n",numberSlacks_,numberElements);
    // Not needed for dense
    numberElements = 3 * numberBasic + 3 * numberElements + 20000;
    int numberIterations = model->numberIterations();
    coinAbcFactorization_->setUsefulInformation(&numberIterations, 0);
    coinAbcFactorization_->getAreas ( numberRows,
				    numberSlacks_ + numberColumnBasic, numberElements,
				    2 * numberElements );
#if 0
    if (!model->numberIterations())
      printf("do I need destructor etc in getAreas?\n");
#endif
    // Fill in counts so we can skip part of preProcess
    // This is NOT needed for dense but would be needed for later versions
    CoinFactorizationDouble *  COIN_RESTRICT elementU;
    int *  COIN_RESTRICT indexRowU;
    CoinBigIndex *  COIN_RESTRICT startColumnU;
    int *  COIN_RESTRICT numberInRow;
    int *  COIN_RESTRICT numberInColumn;
    elementU = coinAbcFactorization_->elements();
    indexRowU = coinAbcFactorization_->indices();
    startColumnU = coinAbcFactorization_->starts();
#define slackValue 1.0
    numberInRow = coinAbcFactorization_->numberInRow();
    numberInColumn = coinAbcFactorization_->numberInColumn();
    coinAbcFactorization_->setNumberSlacks(numberSlacks_);
    CoinZeroN ( numberInRow, numberRows  );
    CoinZeroN ( numberInColumn, numberRows );
    for (i = 0; i < numberSlacks_; i++) {
      int iRow = pivotTemp[i];
      indexRowU[i] = iRow;
      startColumnU[i] = i;
      elementU[i] = slackValue;
      numberInRow[iRow] = 1;
      numberInColumn[i] = 1;
    }
    startColumnU[numberSlacks_] = numberSlacks_;
    // can change for gub so redo
    numberColumnBasic = numberRows - numberSlacks_;
    matrix->fillBasis(pivotTemp + numberSlacks_,
		      numberColumnBasic,
		      indexRowU,
		      startColumnU + numberSlacks_,
		      numberInRow,
		      numberInColumn + numberSlacks_,
		      elementU);
    numberElements = startColumnU[numberRows-1]
      + numberInColumn[numberRows-1];
    coinAbcFactorization_->preProcess ( );
    coinAbcFactorization_->factor (model);
#if 0
    if (model_->numberIterations()==23) {
      CoinAbcFactorization * factor = dynamic_cast<CoinAbcFactorization *>(coinAbcFactorization_);
      if (factor)
	factor->show_self();
    }
#endif
    if (coinAbcFactorization_->status() == -1 &&
	(coinAbcFactorization_->solveMode() & 1) != 0) {
      int solveMode = coinAbcFactorization_->solveMode();
      solveMode --; // so bottom will be 0
      coinAbcFactorization_->setSolveMode(solveMode);
      coinAbcFactorization_->setStatus(-99);
    }
    if (coinAbcFactorization_->status() == -99)
      continue;
    // If we get here status is 0 or -1
    if (coinAbcFactorization_->status() == 0 && numberBasic == numberRows) {
      coinAbcFactorization_->postProcess(pivotTemp, model->pivotVariable());
      model_->moveToBasic();
    } else if (solveType == 0 || solveType == 2/*||solveType==1*/) {
      // Change pivotTemp to be correct list
      anyChanged = true;
      coinAbcFactorization_->makeNonSingular(pivotTemp);
      const double *  COIN_RESTRICT lowerArray = model->lowerRegion();
      const double *  COIN_RESTRICT upperArray = model->upperRegion();
      double *  COIN_RESTRICT solution = model->solutionRegion();
      //int * pivotVariable=model_->pivotVariable();
      //int * fromExternal=model_->fromExternal();
      int numberTotal=model_->numberTotal();
      //can use external status_
      unsigned char *  COIN_RESTRICT statusArray = model_->statusArray();
      CoinAbcMemset0(statusArray,numberTotal);
      for (int iRow=0;iRow<numberRows;iRow++) {
	int iPivot=pivotVariable[iRow];
	//if (iPivot!=pivotTemp[iRow])
	//printf("row %d bad pivot %d good %d\n",iRow,iPivot,pivotTemp[iRow]);
	statusArray[iPivot]=1;
      }
      for (int iRow=0;iRow<numberRows;iRow++) {
	int iPivot=pivotTemp[iRow];
	statusArray[iPivot]|=2;
      }
      int jPivot=0;
      double largeValue = model->largeValue();
      for (int iRow=0;iRow<numberRows;iRow++) {
	int iPivot=pivotVariable[iRow];
	if (statusArray[iPivot]==1) {
	  // clean
	  while (statusArray[jPivot]!=2) {
	    jPivot++;
	  }
	  statusArray[iPivot]=0;
	  statusArray[jPivot]=0;
#ifndef NDEBUG
	  printf("On Row %d replacing %d by %d\n",iRow,iPivot,jPivot);
#endif
	  AbcSimplex::Status thisStatus;
	  if (!valuesPass) {
	    double lower = lowerArray[iPivot];
	    double upper = upperArray[iPivot];
	    double value = solution[iPivot];
	    if (lower > -largeValue || upper < largeValue) {
	      if (lower!=upper) {
		if (fabs(value - lower) < fabs(value - upper)) {
		  thisStatus=AbcSimplex::atLowerBound;
		  solution[iPivot] = lower;
		} else {
		  thisStatus= AbcSimplex::atUpperBound;
		  solution[iPivot] = upper;
		}
	      } else {
		thisStatus= AbcSimplex::isFixed;
		solution[iPivot] = upper;
	      }
	    } else {
	      thisStatus=AbcSimplex::isFree;
	    }
	  } else {
	    thisStatus=AbcSimplex::superBasic;
	  }
	  model_->setInternalStatus(iPivot,thisStatus);
	  model_->setInternalStatus(jPivot,AbcSimplex::basic);
	  // swap (solution will be wrong - but that doesn't matter as basic)
	  model_->swap(iRow,jPivot);
	}
      }
      CoinAbcMemcpy(model_->pivotVariable(),pivotTemp,numberRows);
#ifndef NDEBUG
      model_->checkConsistentPivots();
#endif
      // signal repeat
      coinAbcFactorization_->pivotTolerance(0.999);
      coinAbcFactorization_->setStatus(-99);
    }
  }
  //coinAbcFactorization_->setSolveMode(solveMode|1);
  if ( anyChanged && model->algorithm() < 0 && solveType > 0) {
    double dummyCost;
    static_cast<AbcSimplexDual *> (model)->changeBounds(3,dummyCost);
  }
  return coinAbcFactorization_->status();
}
#if 0
/* Checks if can replace one Column to basis,
   returns 0=OK, 1=Probably OK, 2=singular, 3=no room, 5 max pivots
   Fills in region for use later
   partial update already in U */
int 
AbcSimplexFactorization::checkReplace ( const AbcSimplex * model,
					CoinIndexedVector * regionSparse,
					int pivotRow,
					double &pivotCheck,
					double acceptablePivot)
{
  if (pivots()==maximumPivots())
    return 5;
  else 
    return coinAbcFactorization_->checkReplace(model,regionSparse,pivotRow,pivotCheck,acceptablePivot);
}
/* Replaces one Column in basis,
   returns 0=OK, 1=Probably OK, 2=singular, 3=no room
   If skipBtranU is false will do btran part
   partial update already in U */
int
AbcSimplexFactorization::replaceColumn ( const AbcSimplex * model,
					 CoinIndexedVector * regionSparse,
					 CoinIndexedVector * tableauColumn,
					 int pivotRow,
					 double pivotCheck ,
					 bool skipBtranU,
					 double acceptablePivot)
{
  bool tab = coinAbcFactorization_->wantsTableauColumn();
  int tempInfo[1];
  tempInfo[0] = model->numberIterations();
  coinAbcFactorization_->setUsefulInformation(tempInfo, 1);
  int returnCode =
    coinAbcFactorization_->replaceColumn(tab ? tableauColumn : regionSparse,
				       pivotRow,
				       pivotCheck,
				       skipBtranU,
				       acceptablePivot);
  return returnCode;
}
#endif
/* Replaces one Column to basis,
   partial update already in U */
void 
AbcSimplexFactorization::replaceColumnPart3 ( const AbcSimplex * model,
					      CoinIndexedVector * regionSparse,
					      CoinIndexedVector * tableauColumn,
					      int pivotRow,
#ifdef ABC_LONG_FACTORIZATION 
					      long
#endif
					      double alpha )
{
  bool tab = coinAbcFactorization_->wantsTableauColumn();
  int tempInfo[1];
  tempInfo[0] = model->numberIterations();
  coinAbcFactorization_->setUsefulInformation(tempInfo, 1);
  if (tab)
    coinAbcFactorization_->replaceColumnPart3(model,NULL,tableauColumn,
				       pivotRow,
					      tableauColumn->denseVector()[pivotRow]);
  else
    coinAbcFactorization_->replaceColumnPart3(model,regionSparse,NULL,
					      pivotRow,
					      alpha);
}
/* Replaces one Column to basis,
   partial update in vector */
void 
AbcSimplexFactorization::replaceColumnPart3 ( const AbcSimplex * model,
					      CoinIndexedVector * regionSparse,
					      CoinIndexedVector * tableauColumn,
					      CoinIndexedVector * partialUpdate,
					      int pivotRow,
#ifdef ABC_LONG_FACTORIZATION 
					      long
#endif
					      double alpha )
{
  bool tab = coinAbcFactorization_->wantsTableauColumn();
  int tempInfo[1];
  tempInfo[0] = model->numberIterations();
  coinAbcFactorization_->setUsefulInformation(tempInfo, 1);
  if (tab)
    coinAbcFactorization_->replaceColumnPart3(model,NULL,tableauColumn,
					      pivotRow,
					      tableauColumn->denseVector()[pivotRow]);
  else
    coinAbcFactorization_->replaceColumnPart3(model,regionSparse,NULL,partialUpdate,
					      pivotRow,
					      alpha);
}
#if 0
/* Updates one column (FTRAN) from region2
   number returned is negative if no room
   region1 starts as zero and is zero at end */
int
AbcSimplexFactorization::updateColumnFT ( CoinIndexedVector * regionSparse,
					  CoinIndexedVector * regionSparse2)
{
  if (!numberRows())
    return 0;
  int returnCode;
  returnCode = coinAbcFactorization_->updateColumnFT(regionSparse,
						   regionSparse2);
  return returnCode;
}
/* Updates one column (FTRAN) from region2
   number returned is negative if no room
   region1 starts as zero and is zero at end */
int
AbcSimplexFactorization::updateColumn ( CoinIndexedVector * regionSparse,
					CoinIndexedVector * regionSparse2) const
{
  if (!numberRows())
    return 0;
  int returnCode;
  returnCode = coinAbcFactorization_->updateColumn(regionSparse,
						 regionSparse2,true);
  return returnCode;
}
/* Updates one column (FTRAN) from region2
   Tries to do FT update
   number returned is negative if no room.
   Also updates region3
   region1 starts as zero and is zero at end */
int
AbcSimplexFactorization::updateTwoColumnsFT ( CoinIndexedVector * regionSparse1,
					      CoinIndexedVector * regionSparse2,
					      CoinIndexedVector * regionSparse3)
{
  if (!numberRows())
    return 0;
  int returnCode = 0;
  returnCode = 
    coinAbcFactorization_->updateTwoColumnsFT(
					    regionSparse1,
					    regionSparse2,
					    regionSparse3,
					    true);
  return returnCode;
}
/* Updates one column (BTRAN) from region2
   region1 starts as zero and is zero at end */
int
AbcSimplexFactorization::updateColumnTranspose ( CoinIndexedVector * regionSparse,
						 CoinIndexedVector * regionSparse2) const
{
  if (!numberRows())
    return 0;
  int returnCode;
  returnCode = coinAbcFactorization_->updateColumnTranspose(regionSparse,
							  regionSparse2);
  return returnCode;
}
/* Updates one column for dual steepest edge weights (FTRAN) */
void 
AbcSimplexFactorization::updateWeights ( CoinIndexedVector & regionSparse) const
{
  // NOTE either switch off sparse or pass in a sparseArray_ so can go parallel
  // may be best to use inner product approach
  static double fraction[2]={0.0,0.0};
  static int times=0;
  times++;
  fraction[0] += static_cast<double>(regionSparse.getNumElements())/
    (static_cast<double>(model_->numberRows())+1.0);
  updateColumn(regionSparse);
  fraction[1] += static_cast<double>(regionSparse.getNumElements())/
    (static_cast<double>(model_->numberRows())+1.0);
  if ((times%1000)==0)
    printf("Average density %g before then %g\n",
	   (100.0*fraction[0])/static_cast<double>(times),
	   (100.0*fraction[1])/static_cast<double>(times));
}
#endif
/* makes a row copy of L for speed and to allow very sparse problems */
void
AbcSimplexFactorization::goSparse()
{
  abort();
  coinAbcFactorization_->goSparse();
}
// Set tolerances to safer of existing and given
void
AbcSimplexFactorization::saferTolerances (  double zeroValue,
					    double pivotValue)
{
  double newValue1;
  // better to have small tolerance even if slower
  if (zeroValue > 0.0)
    newValue1 = zeroValue;
  else
    newValue1 = -zeroTolerance() * zeroValue; 
  newValue1 = CoinMin(zeroTolerance(),newValue1);
  if (newValue1>1.0e-15)
    zeroTolerance(newValue1);
  double  newValue2;
  // better to have large tolerance even if slower
  if (pivotValue > 0.0)
    newValue2 = pivotValue;
  else
    newValue2 = -pivotTolerance() * pivotValue;
  newValue2 =CoinMin(CoinMax(pivotTolerance(), newValue2), 0.999);
  if (newValue2>pivotTolerance()) {
    pivotTolerance(newValue2);
    char line[100];
    sprintf(line,"new zero tolerance %g new pivot tolerance %g",
	    zeroTolerance(),pivotTolerance());
    model_->messageHandler()->message(CLP_GENERAL2,*model_->messagesPointer())
      << line << CoinMessageEol;
  }
}
// Sets factorization
void
AbcSimplexFactorization::setFactorization(AbcSimplexFactorization & rhs)
{
  AbcSimplexFactorization::operator=(rhs);
}
