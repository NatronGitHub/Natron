/* $Id$ */
// Copyright (C) 2000, International Business Machines
// Corporation and others, Copyright (C) 2012, FasterCoin.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include "ClpConfig.h"

#include "CoinPragma.hpp"
#include <math.h>
//#define ABC_DEBUG 2

#if SLIM_CLP==2
#define SLIM_NOIO
#endif
#include "CoinHelperFunctions.hpp"
#include "CoinFloatEqual.hpp"
#include "ClpSimplex.hpp"
#include "AbcSimplex.hpp"
#include "AbcSimplexDual.hpp"
#include "AbcSimplexFactorization.hpp"
#include "AbcNonLinearCost.hpp"
#include "CoinAbcCommon.hpp"
#include "AbcMatrix.hpp"
#include "CoinIndexedVector.hpp"
#include "AbcDualRowDantzig.hpp"
#include "AbcDualRowSteepest.hpp"
#include "AbcPrimalColumnDantzig.hpp"
#include "AbcPrimalColumnSteepest.hpp"
#include "ClpMessage.hpp"
#include "ClpEventHandler.hpp"
#include "ClpLinearObjective.hpp"
#include "CoinAbcHelperFunctions.hpp"
#include "CoinModel.hpp"
#include "CoinLpIO.hpp"
#include <cfloat>

#include <string>
#include <stdio.h>
#include <iostream>
//#############################################################################
AbcSimplex::AbcSimplex (bool emptyMessages) :
  
  ClpSimplex(emptyMessages)
{
  gutsOfDelete(0);
  gutsOfInitialize(0,0,true);
  assert (maximumAbcNumberRows_>=0);
  //printf("XX %x AbcSimplex constructor\n",this);
}

//-----------------------------------------------------------------------------

AbcSimplex::~AbcSimplex ()
{
  //printf("XX %x AbcSimplex destructor\n",this);
  gutsOfDelete(1);
}
// Copy constructor.
AbcSimplex::AbcSimplex(const AbcSimplex &rhs) :
  ClpSimplex(rhs)
{
  //printf("XX %x AbcSimplex constructor from %x\n",this,&rhs);
  gutsOfDelete(0);
  gutsOfInitialize(numberRows_,numberColumns_,false);
  gutsOfCopy(rhs);
  assert (maximumAbcNumberRows_>=0);
}
#include "ClpDualRowSteepest.hpp"
#include "ClpPrimalColumnSteepest.hpp"
#include "ClpFactorization.hpp"
// Copy constructor from model
AbcSimplex::AbcSimplex(const ClpSimplex &rhs) :
  ClpSimplex(rhs)
{
  //printf("XX %x AbcSimplex constructor from ClpSimplex\n",this);
  gutsOfDelete(0);
  gutsOfInitialize(numberRows_,numberColumns_,true);
  gutsOfResize(numberRows_,numberColumns_);
  // Set up row/column selection and factorization type
  ClpDualRowSteepest * pivot = dynamic_cast<ClpDualRowSteepest *>(rhs.dualRowPivot());
  if (pivot) {
    AbcDualRowSteepest steep(pivot->mode());
    setDualRowPivotAlgorithm(steep);
  } else {
    AbcDualRowDantzig dantzig;
    setDualRowPivotAlgorithm(dantzig);
  }
  ClpPrimalColumnSteepest * pivotColumn = dynamic_cast<ClpPrimalColumnSteepest *>(rhs.primalColumnPivot());
  if (pivotColumn) {
    AbcPrimalColumnSteepest steep(pivotColumn->mode());
    setPrimalColumnPivotAlgorithm(steep);
  } else {
    AbcPrimalColumnDantzig dantzig;
    setPrimalColumnPivotAlgorithm(dantzig);
  }
  //if (rhs.factorization()->)
  //factorization_->forceOtherFactorization();
  factorization()->synchronize(rhs.factorization(),this);
  //factorization_->setGoDenseThreshold(rhs.factorization()->goDenseThreshold());
  //factorization_->setGoSmallThreshold(rhs.factorization()->goSmallThreshold());
  translate(DO_SCALE_AND_MATRIX|DO_BASIS_AND_ORDER|DO_STATUS|DO_SOLUTION);
  assert (maximumAbcNumberRows_>=0);
}
// Assignment operator. This copies the data
AbcSimplex &
AbcSimplex::operator=(const AbcSimplex & rhs)
{
  if (this != &rhs) {
    gutsOfDelete(1);
    ClpSimplex::operator=(rhs);
    gutsOfCopy(rhs);
    assert (maximumAbcNumberRows_>=0);
  }
  return *this;
}
// fills in perturbationSaved_ from start with 0.5+random
void 
AbcSimplex::fillPerturbation(int start, int number)
{
  double * array = perturbationSaved_+start;
  for (int i=0;i<number;i++) 
    array[i] = 0.5+0.5*randomNumberGenerator_.randomDouble();
}
// Sets up all extra pointers
void 
AbcSimplex::setupPointers(int maxRows,int maxColumns)
{
  int numberTotal=maxRows+maxColumns;
  scaleToExternal_ = scaleFromExternal_+numberTotal;
  tempArray_=offset_+numberTotal;
  lowerSaved_=abcLower_+numberTotal;
  upperSaved_=abcUpper_+numberTotal;
  costSaved_=abcCost_+numberTotal;
  solutionSaved_=abcSolution_+numberTotal;
  djSaved_=abcDj_+numberTotal;
  internalStatusSaved_= internalStatus_+numberTotal;
  perturbationSaved_=abcPerturbation_+numberTotal;
  offsetRhs_=tempArray_+numberTotal;
  lowerBasic_=lowerSaved_+numberTotal;
  upperBasic_=upperSaved_+numberTotal;
  costBasic_=costSaved_+2*numberTotal;
  solutionBasic_=solutionSaved_+numberTotal;
  djBasic_=djSaved_+numberTotal;
  perturbationBasic_=perturbationSaved_+numberTotal;
  columnUseScale_ = scaleToExternal_+maxRows;
  inverseColumnUseScale_ = scaleFromExternal_+maxRows;
}
// Copies all saved versions to working versions and may do something for perturbation
void 
AbcSimplex::copyFromSaved(int which)
{
  if ((which&1)!=0)
    CoinAbcMemcpy(abcSolution_,solutionSaved_,maximumNumberTotal_);
  if ((which&2)!=0)
    CoinAbcMemcpy(abcCost_,costSaved_,maximumNumberTotal_);
  if ((which&4)!=0)
    CoinAbcMemcpy(abcLower_,lowerSaved_,maximumNumberTotal_);
  if ((which&8)!=0)
    CoinAbcMemcpy(abcUpper_,upperSaved_,maximumNumberTotal_);
  if ((which&16)!=0)
    CoinAbcMemcpy(abcDj_,djSaved_,maximumNumberTotal_);
  if ((which&32)!=0) {
    CoinAbcMemcpy(abcLower_,abcPerturbation_,numberTotal_);
    CoinAbcMemcpy(abcUpper_,abcPerturbation_+numberTotal_,numberTotal_);
  }
}
void
AbcSimplex::gutsOfCopy(const AbcSimplex & rhs)
{
#ifdef ABC_INHERIT
  abcSimplex_=this;
#endif
  assert (numberRows_ == rhs.numberRows_);
  assert (numberColumns_ == rhs.numberColumns_);
  numberTotal_ = rhs.numberTotal_;
  maximumNumberTotal_ = rhs.maximumNumberTotal_;
  // special options here to be safe
  specialOptions_=rhs.specialOptions_;
  //assert (maximumInternalRows_ >= numberRows_);
  //assert (maximumInternalColumns_ >= numberColumns_);
  // Copy all scalars
  CoinAbcMemcpy(reinterpret_cast<int *>(&sumNonBasicCosts_),
	    reinterpret_cast<const int *>(&rhs.sumNonBasicCosts_),
	    (&swappedAlgorithm_-reinterpret_cast<int *>(&sumNonBasicCosts_))+1);
  // could add 2 so can go off end
  int sizeArray=2*maximumNumberTotal_+maximumAbcNumberRows_;
  internalStatus_ = ClpCopyOfArray(rhs.internalStatus_,sizeArray+maximumNumberTotal_);
  abcLower_ = ClpCopyOfArray(rhs.abcLower_, sizeArray);
  abcUpper_ = ClpCopyOfArray(rhs.abcUpper_, sizeArray);
  abcCost_ = ClpCopyOfArray(rhs.abcCost_, sizeArray+maximumNumberTotal_);
  abcDj_ = ClpCopyOfArray(rhs.abcDj_, sizeArray);

  abcSolution_ = ClpCopyOfArray(rhs.abcSolution_, sizeArray+maximumNumberTotal_);
  abcPerturbation_ = ClpCopyOfArray(rhs.abcPerturbation_,sizeArray);
  abcPivotVariable_ = ClpCopyOfArray(rhs.abcPivotVariable_,maximumAbcNumberRows_);
  //fromExternal_ = ClpCopyOfArray(rhs.fromExternal_,sizeArray);
  //toExternal_ = ClpCopyOfArray(rhs.toExternal_,sizeArray);
  scaleFromExternal_ = ClpCopyOfArray(rhs.scaleFromExternal_,sizeArray);
  offset_ = ClpCopyOfArray(rhs.offset_,sizeArray);
  setupPointers(maximumAbcNumberRows_,maximumAbcNumberColumns_);
  if (rhs.abcMatrix_)
    abcMatrix_ = new AbcMatrix(*rhs.abcMatrix_);
  else
    abcMatrix_ = NULL;
  for (int i = 0; i < ABC_NUMBER_USEFUL; i++) {
    usefulArray_[i] = rhs.usefulArray_[i];
  }
  if (rhs.abcFactorization_) {
    setFactorization(*rhs.abcFactorization_);
  } else {
    delete abcFactorization_;
    abcFactorization_ = NULL;
  }
#ifdef EARLY_FACTORIZE
  delete abcEarlyFactorization_;
  if (rhs.abcEarlyFactorization_) {
    abcEarlyFactorization_ = new AbcSimplexFactorization(*rhs.abcEarlyFactorization_);
  } else {
    abcEarlyFactorization_ = NULL;
  }
#endif
  abcDualRowPivot_ = rhs.abcDualRowPivot_->clone(true);
  abcDualRowPivot_->setModel(this);
  abcPrimalColumnPivot_ = rhs.abcPrimalColumnPivot_->clone(true);
  abcPrimalColumnPivot_->setModel(this);
  if (rhs.abcBaseModel_) {
    abcBaseModel_ = new AbcSimplex(*rhs.abcBaseModel_);
  } else {
    abcBaseModel_ = NULL;
  }
  if (rhs.clpModel_) {
    clpModel_ = new ClpSimplex(*rhs.clpModel_);
  } else {
    clpModel_ = NULL;
  }
  abcProgress_ = rhs.abcProgress_;
  solveType_ = rhs.solveType_;
}
// type == 0 nullify, 1 also delete
void
AbcSimplex::gutsOfDelete(int type)
{
  if (type) {
    delete [] abcLower_;
    delete [] abcUpper_;
    delete [] abcCost_;
    delete [] abcDj_;
    delete [] abcSolution_;
    //delete [] fromExternal_ ;
    //delete [] toExternal_ ;
    delete [] scaleFromExternal_ ;
    //delete [] scaleToExternal_ ;
    delete [] offset_ ;
    delete [] internalStatus_ ;
    delete [] abcPerturbation_ ;
    delete abcMatrix_ ;
    delete abcFactorization_;
#ifdef EARLY_FACTORIZE
    delete abcEarlyFactorization_;
#endif
    delete [] abcPivotVariable_;
    delete abcDualRowPivot_;
    delete abcPrimalColumnPivot_;
    delete abcBaseModel_;
    delete clpModel_;
    delete abcNonLinearCost_;
  }
  CoinAbcMemset0(reinterpret_cast<char *>(&scaleToExternal_),
	     reinterpret_cast<char *>(&usefulArray_[0])-reinterpret_cast<char *>(&scaleToExternal_));
}
template <class T> T *
newArray(T * /*nullArray*/, int size)
{
  if (size) {
    T * arrayNew = new T[size];
#ifndef NDEBUG
    memset(arrayNew,'A',(size)*sizeof(T));
#endif
    return arrayNew;
  } else {
    return NULL;
  }
}
template <class T> T *
resizeArray( T * array, int oldSize1, int oldSize2, int newSize1, int newSize2,int extra)
{
  if ((array||!oldSize1)&&(newSize1!=oldSize1||newSize2!=oldSize2)) {
    int newTotal=newSize1+newSize2;
    int oldTotal=oldSize1+oldSize2;
    T * arrayNew;
    if (newSize1>oldSize1||newSize2>oldSize2) {
      arrayNew = new T[2*newTotal+newSize1+extra];
#ifndef NDEBUG
      memset(arrayNew,'A',(2*newTotal+newSize1+extra)*sizeof(T));
#endif
      CoinAbcMemcpy(arrayNew,array,oldSize1);
      CoinAbcMemcpy(arrayNew+newSize1,array+oldSize1,oldSize2);
      // and second half
      CoinAbcMemcpy(arrayNew+newTotal,array,oldSize1+oldTotal);
      CoinAbcMemcpy(arrayNew+newSize1+newTotal,array+oldSize1+oldTotal,oldSize2);
      delete [] array;
    } else {
      arrayNew=array;
      for (int i=0;i<newSize2;i++)
	array[newSize1+i]=array[oldSize1+i];
      for (int i=0;i<newSize1;i++)
	array[newTotal+i]=array[oldTotal+i];
      for (int i=0;i<newSize2;i++)
	array[newSize1+newTotal+i]=array[oldSize1+oldTotal+i];
    }
    return arrayNew;
  } else {
    return array;
  }
}
// Initializes arrays
void 
AbcSimplex::gutsOfInitialize(int numberRows,int numberColumns,bool doMore)
{
#ifdef ABC_INHERIT
  abcSimplex_=this;
#endif
  // Zero all
  CoinAbcMemset0(&sumNonBasicCosts_,(reinterpret_cast<double *>(&usefulArray_[0])-&sumNonBasicCosts_));
  zeroTolerance_ = 1.0e-13;
  bestObjectiveValue_ = -COIN_DBL_MAX;
  primalToleranceToGetOptimal_ = -1.0;
  primalTolerance_ = 1.0e-6;
  //dualTolerance_ = 1.0e-6;
  alphaAccuracy_ = -1.0;
  upperIn_ = -COIN_DBL_MAX;
  lowerOut_ = -1;
  valueOut_ = -1;
  upperOut_ = -1;
  dualOut_ = -1;
  acceptablePivot_ = 1.0e-8;
  //dualBound_=1.0e9;
  sequenceIn_ = -1;
  directionIn_ = -1;
  sequenceOut_ = -1;
  directionOut_ = -1;
  pivotRow_ = -1;
  lastGoodIteration_ = -100;
  numberPrimalInfeasibilities_ = 100;
  numberRefinements_=1;
  changeMade_ = 1;
  forceFactorization_ = -1;
  if (perturbation_<50||(perturbation_>60&&perturbation_<100))
    perturbation_ = 50;
  lastBadIteration_ = -999999;
  lastFlaggedIteration_ = -999999; // doesn't seem to be used
  firstFree_ = -1;
  incomingInfeasibility_ = 1.0;
  allowedInfeasibility_ = 10.0;
  solveType_ = 1; // say simplex based life form
  //specialOptions_|=65536;
  //ClpModel::startPermanentArrays();
  maximumInternalRows_ =0;
  maximumInternalColumns_ =0;
  numberRows_=numberRows;
  numberColumns_=numberColumns;
  numberTotal_=numberRows_+numberColumns_;
  maximumAbcNumberRows_=numberRows;
  maximumAbcNumberColumns_=numberColumns;
  maximumNumberTotal_=numberTotal_;
  int sizeArray=2*maximumNumberTotal_+maximumAbcNumberRows_;
  if (doMore) {
    // say Steepest pricing
    abcDualRowPivot_ = new AbcDualRowSteepest();
    abcPrimalColumnPivot_ = new AbcPrimalColumnSteepest();
    internalStatus_ = newArray(reinterpret_cast<unsigned char *>(NULL),
			       sizeArray+maximumNumberTotal_);
    abcLower_ = newArray(reinterpret_cast<double *>(NULL),sizeArray);
    abcUpper_ = newArray(reinterpret_cast<double *>(NULL),sizeArray);
    abcCost_ = newArray(reinterpret_cast<double *>(NULL),sizeArray+maximumNumberTotal_);
    abcDj_ = newArray(reinterpret_cast<double *>(NULL),sizeArray);
    abcSolution_ = newArray(reinterpret_cast<double *>(NULL),sizeArray+maximumNumberTotal_);
    //fromExternal_ = newArray(reinterpret_cast<int *>(NULL),sizeArray);
    //toExternal_ = newArray(reinterpret_cast<int *>(NULL),sizeArray);
    scaleFromExternal_ = newArray(reinterpret_cast<double *>(NULL),sizeArray);
    offset_ = newArray(reinterpret_cast<double *>(NULL),sizeArray);
    abcPerturbation_ = newArray(reinterpret_cast<double *>(NULL),sizeArray);
    abcPivotVariable_ = newArray(reinterpret_cast<int *>(NULL),maximumAbcNumberRows_);
    // Fill perturbation array
    setupPointers(maximumAbcNumberRows_,maximumAbcNumberColumns_);
    fillPerturbation(0,maximumNumberTotal_);
  }
  // get an empty factorization so we can set tolerances etc
  getEmptyFactorization();
  for (int i=0;i<ABC_NUMBER_USEFUL;i++) 
    usefulArray_[i].reserve(CoinMax(CoinMax(numberTotal_,maximumAbcNumberRows_+200),2*numberRows_));
  //savedStatus_ = internalStatus_+maximumNumberTotal_;
  //startPermanentArrays();
}
// resizes arrays
void 
AbcSimplex::gutsOfResize(int numberRows,int numberColumns)
{
  if (!abcSolution_) {
    numberRows_=0;
    numberColumns_=0;
    numberTotal_=0;
    maximumAbcNumberRows_=0;
    maximumAbcNumberColumns_=0;
    maximumNumberTotal_=0;
  }
  if (numberRows==numberRows_&&numberColumns==numberColumns_)
    return;
  // can do on state bit
  int newSize1=CoinMax(numberRows,maximumAbcNumberRows_);
  if ((stateOfProblem_&ADD_A_BIT)!=0&&numberRows>maximumAbcNumberRows_)
    newSize1=CoinMax(numberRows,maximumAbcNumberRows_+CoinMin(100,numberRows_/10));
  int newSize2=CoinMax(numberColumns,maximumAbcNumberColumns_);
  numberRows_=numberRows;
  numberColumns_=numberColumns;
  numberTotal_=numberRows_+numberColumns_;
  //fromExternal_ = resizeArray(fromExternal_,maximumAbcNumberRows_,maximumAbcNumberColumns_,newSize1,newSize2,1);
  //toExternal_ = resizeArray(toExternal_,maximumAbcNumberRows_,maximumAbcNumberColumns_,newSize1,newSize2,1,0);
  scaleFromExternal_ = resizeArray(scaleFromExternal_,maximumAbcNumberRows_,maximumAbcNumberColumns_,newSize1,newSize2,0);
  //scaleToExternal_ = resizeArray(scaleToExternal_,maximumAbcNumberRows_,maximumAbcNumberColumns_,newSize1,newSize2,1,0);
  internalStatus_ = resizeArray(internalStatus_,maximumAbcNumberRows_,
				maximumAbcNumberColumns_,
				newSize1,newSize2,numberTotal_);
  abcLower_ = resizeArray(abcLower_,maximumAbcNumberRows_,maximumAbcNumberColumns_,newSize1,newSize2,0);
  abcUpper_ = resizeArray(abcUpper_,maximumAbcNumberRows_,maximumAbcNumberColumns_,newSize1,newSize2,0);
  abcCost_ = resizeArray(abcCost_,maximumAbcNumberRows_,maximumAbcNumberColumns_,newSize1,newSize2,numberTotal_);
  abcDj_ = resizeArray(abcDj_,maximumAbcNumberRows_,maximumAbcNumberColumns_,newSize1,newSize2,0);
  abcSolution_ = resizeArray(abcSolution_,maximumAbcNumberRows_,maximumAbcNumberColumns_,newSize1,newSize2,numberTotal_);
  abcPerturbation_ = resizeArray(abcPerturbation_,maximumAbcNumberRows_,maximumAbcNumberColumns_,newSize1,newSize2,0);
  offset_ = resizeArray(offset_,maximumAbcNumberRows_,maximumAbcNumberColumns_,newSize1,newSize2,0);
  setupPointers(newSize1,newSize2);
  // Fill gaps in perturbation array
  fillPerturbation(maximumAbcNumberRows_,newSize1-maximumAbcNumberRows_);
  fillPerturbation(newSize1+maximumAbcNumberColumns_,newSize2-maximumAbcNumberColumns_);
  // Clean gap
  //CoinIotaN(fromExternal_+maximumAbcNumberRows_,newSize1-maximumAbcNumberRows_,maximumAbcNumberRows_);
  //CoinIotaN(toExternal_+maximumAbcNumberRows_,newSize1-maximumAbcNumberRows_,maximumAbcNumberRows_);
  CoinFillN(scaleFromExternal_+maximumAbcNumberRows_,newSize1-maximumAbcNumberRows_,1.0);
  CoinFillN(scaleToExternal_+maximumAbcNumberRows_,newSize1-maximumAbcNumberRows_,1.0);
  CoinFillN(internalStatusSaved_+maximumAbcNumberRows_,newSize1-maximumAbcNumberRows_,static_cast<unsigned char>(1));
  CoinFillN(lowerSaved_+maximumAbcNumberRows_,newSize1-maximumAbcNumberRows_,-COIN_DBL_MAX);
  CoinFillN(upperSaved_+maximumAbcNumberRows_,newSize1-maximumAbcNumberRows_,COIN_DBL_MAX);
  CoinFillN(costSaved_+maximumAbcNumberRows_,newSize1-maximumAbcNumberRows_,0.0);
  CoinFillN(djSaved_+maximumAbcNumberRows_,newSize1-maximumAbcNumberRows_,0.0);
  CoinFillN(solutionSaved_+maximumAbcNumberRows_,newSize1-maximumAbcNumberRows_,0.0);
  CoinFillN(offset_+maximumAbcNumberRows_,newSize1-maximumAbcNumberRows_,0.0);
  maximumAbcNumberRows_=newSize1;
  maximumAbcNumberColumns_=newSize2;
  maximumNumberTotal_=newSize1+newSize2;
  delete [] abcPivotVariable_;
  abcPivotVariable_ = new int[maximumAbcNumberRows_];
  for (int i=0;i<ABC_NUMBER_USEFUL;i++) 
    usefulArray_[i].reserve(CoinMax(numberTotal_,maximumAbcNumberRows_+200));
}
void
AbcSimplex::translate(int type)
{
  // clear bottom bits
  stateOfProblem_ &= ~(VALUES_PASS-1);
  if ((type&DO_SCALE_AND_MATRIX)!=0) {
    //stateOfProblem_ |= DO_SCALE_AND_MATRIX;
    stateOfProblem_ |= DO_SCALE_AND_MATRIX;
    delete abcMatrix_;
    abcMatrix_=new AbcMatrix(*matrix());
    abcMatrix_->setModel(this);
    abcMatrix_->scale(scalingFlag_ ? 0 : -1);
  }
  if ((type&DO_STATUS)!=0&&(type&DO_BASIS_AND_ORDER)==0) {
    // from Clp enum to Abc enum (and bound flip)
    unsigned char lookupToAbcSlack[6]={4,6,0/*1*/,1/*0*/,5,7};
    unsigned char *  COIN_RESTRICT statusAbc=internalStatus_;
    const unsigned char *  COIN_RESTRICT statusClp=status_;
    int i;
    for (i=numberRows_-1;i>=0;i--) {
      unsigned char status=statusClp[i]&7;
      bool basicClp=status==1;
      bool basicAbc=(statusAbc[i]&7)==4;
      if (basicClp==basicAbc)
	statusAbc[i]=lookupToAbcSlack[status];
      else
	break;
    }
    if (!i) {
      // from Clp enum to Abc enum
      unsigned char lookupToAbc[6]={4,6,1,0,5,7};
      statusAbc+=maximumAbcNumberRows_;
      statusClp+=maximumAbcNumberRows_;
      for (i=numberColumns_-1;i>=0;i--) {
	unsigned char status=statusClp[i]&7;
	bool basicClp=status==1;
	bool basicAbc=(statusAbc[i]&7)==4;
	if (basicClp==basicAbc)
	  statusAbc[i]=lookupToAbc[status];
	else
	  break;
      }
      if (i)
	type |=DO_BASIS_AND_ORDER;
    } else {
      type |=DO_BASIS_AND_ORDER;
    }
    stateOfProblem_ |= DO_STATUS;
  }
  if ((type&DO_BASIS_AND_ORDER)!=0) {
    permuteIn();
    permuteBasis();
    stateOfProblem_ |= DO_BASIS_AND_ORDER;
    type &= ~DO_SOLUTION;
  }
  if ((type&DO_SOLUTION)!=0) {
    permuteBasis();
    stateOfProblem_ |= DO_SOLUTION;
  }
  if ((type&DO_JUST_BOUNDS)!=0) {
    stateOfProblem_ |= DO_JUST_BOUNDS;
  }
  if (!type) {
    // just move stuff down
    CoinAbcMemcpy(abcLower_,abcLower_+maximumNumberTotal_,numberTotal_);
    CoinAbcMemcpy(abcUpper_,abcUpper_+maximumNumberTotal_,numberTotal_);
    CoinAbcMemcpy(abcCost_,abcCost_+maximumNumberTotal_,numberTotal_);
  }
}
/* Sets dual values pass djs using unscaled duals
   type 1 - values pass
   type 2 - just use as infeasibility weights 
   type 3 - as 2 but crash
*/
void 
AbcSimplex::setupDualValuesPass(const double * fakeDuals,
				const double * fakePrimals,
				int type)
{
  // allslack basis
  memset(internalStatus_,6,numberRows_);
  // temp
  if (type==1) {
    bool allEqual=true;
    for (int i=0;i<numberRows_;i++) {
      if (rowUpper_[i]>rowLower_[i]) {
	allEqual=false;
	break;
      }
    }
    if (allEqual) {
      // just modify costs
      transposeTimes(-1.0,fakeDuals,objective());
      return;
    }
  }
  // compute unscaled djs
  CoinAbcMemcpy(djSaved_+maximumAbcNumberRows_,objective(),numberColumns_);
  matrix_->transposeTimes(-1.0,fakeDuals,djSaved_+maximumAbcNumberRows_);
  // save fake solution
  assert (solution_);
  //solution_ = new double[numberTotal_];
  CoinAbcMemset0(solution_,numberRows_);
  CoinAbcMemcpy(solution_+maximumAbcNumberRows_,fakePrimals,numberColumns_);
  // adjust
  for (int iSequence=maximumAbcNumberRows_;iSequence<numberTotal_;iSequence++)
    solution_[iSequence]-=offset_[iSequence];
  matrix_->times(-1.0,solution_+maximumAbcNumberRows_,solution_);
  double direction = optimizationDirection_;
  const double *  COIN_RESTRICT rowScale=scaleFromExternal_;
  const double *  COIN_RESTRICT inverseRowScale=scaleToExternal_;
  for (int iRow=0;iRow<numberRows_;iRow++) {
    djSaved_[iRow]=direction*fakeDuals[iRow]*inverseRowScale[iRow];
    solution_[iRow] *= rowScale[iRow];
  }
  const double *  COIN_RESTRICT columnScale=scaleToExternal_;
  for (int iColumn=maximumAbcNumberRows_;iColumn<numberTotal_;iColumn++)
    djSaved_[iColumn]*=direction*columnScale[iColumn];
  // Set solution values
  const double * lower = abcLower_+maximumAbcNumberRows_;
  const double * upper = abcUpper_+maximumAbcNumberRows_;
  double * solution = abcSolution_+maximumAbcNumberRows_;
  const double * djs = djSaved_+maximumAbcNumberRows_;
  const double * inverseColumnScale=scaleFromExternal_+maximumAbcNumberRows_;
  for (int iColumn=0;iColumn<numberColumns_;iColumn++) {
    double thisValue=fakePrimals[iColumn];
    double thisLower=columnLower_[iColumn];
    double thisUpper=columnUpper_[iColumn];
    double thisDj=djs[iColumn];
    solution_[iColumn+maximumAbcNumberRows_]=solution_[iColumn+maximumAbcNumberRows_]*
      inverseColumnScale[iColumn];
    if (thisLower>-1.0e30) {
      if (thisUpper<1.0e30) {
	double gapUp=thisUpper-thisValue;
	double gapDown=thisValue-thisLower;
	bool goUp;
	if (gapUp>gapDown&&thisDj>-dualTolerance_) {
	  goUp=false;
	} else if (gapUp<gapDown&&thisDj<dualTolerance_) {
	  goUp=true;
	} else {
	  // wild guess
	  double badUp;
	  double badDown;
	  if (gapUp>gapDown) {
	    badUp=gapUp*dualTolerance_;
	    badDown=-gapDown*thisDj;
	  } else {
	    badUp=gapUp*thisDj;
	    badDown=gapDown*dualTolerance_;
	  }
	  goUp=badDown>badUp;
	}
	if (goUp) {
	  solution[iColumn]=upper[iColumn];
	  setInternalStatus(iColumn+maximumAbcNumberRows_,atUpperBound);
	  setStatus(iColumn,ClpSimplex::atUpperBound);
	} else {
	  solution[iColumn]=lower[iColumn];
	  setInternalStatus(iColumn+maximumAbcNumberRows_,atLowerBound);
	  setStatus(iColumn,ClpSimplex::atLowerBound);
	}
      } else {
	solution[iColumn]=lower[iColumn];
	setInternalStatus(iColumn+maximumAbcNumberRows_,atLowerBound);
	setStatus(iColumn,ClpSimplex::atLowerBound);
      }
    } else if (thisUpper<1.0e30) {
      solution[iColumn]=upper[iColumn];
      setInternalStatus(iColumn+maximumAbcNumberRows_,atUpperBound);
      setStatus(iColumn,ClpSimplex::atUpperBound);
    } else {
      // free
      solution[iColumn]=thisValue*inverseColumnScale[iColumn];
      setInternalStatus(iColumn+maximumAbcNumberRows_,isFree);
      setStatus(iColumn,ClpSimplex::isFree);
    }
  }
  switch (type) {
  case 1:
    stateOfProblem_ |= VALUES_PASS;
    break;
  case 2:
    stateOfProblem_ |= VALUES_PASS2;
    delete [] solution_;
    solution_=NULL;
    break;
  case 3:
    stateOfProblem_ |= VALUES_PASS2;
    break;
  }
}
//#############################################################################
int
AbcSimplex::computePrimals(CoinIndexedVector * arrayVector, CoinIndexedVector * previousVector)
{
  
  arrayVector->clear();
  previousVector->clear();
  // accumulate non basic stuff
  double *  COIN_RESTRICT array = arrayVector->denseVector();
  CoinAbcScatterZeroTo(abcSolution_,abcPivotVariable_,numberRows_);
  abcMatrix_->timesIncludingSlacks(-1.0, abcSolution_, array);
#if 0
  static int xxxxxx=0;
  if (xxxxxx==0)
  for (int i=0;i<numberRows_;i++)
    printf("%d %.18g\n",i,array[i]);
#endif
  //arrayVector->scan(0,numberRows_,zeroTolerance_);
  // Ftran adjusted RHS and iterate to improve accuracy
  double lastError = COIN_DBL_MAX;
  CoinIndexedVector * thisVector = arrayVector;
  CoinIndexedVector * lastVector = previousVector;
  //if (arrayVector->getNumElements())
#if 0
  double largest=0.0;
  int iLargest=-1;
  for (int i=0;i<numberRows_;i++) {
    if (fabs(array[i])>largest) {
      largest=array[i];
      iLargest=i;
    }
  }
  printf("largest %g at row %d\n",array[iLargest],iLargest);
#endif
  abcFactorization_->updateFullColumn(*thisVector);
#if 0
  largest=0.0;
  iLargest=-1;
  for (int i=0;i<numberRows_;i++) {
    if (fabs(array[i])>largest) {
      largest=array[i];
      iLargest=i;
    }
  }
  printf("after largest %g at row %d\n",array[iLargest],iLargest);
#endif
#if 0
  if (xxxxxx==0)
  for (int i=0;i<numberRows_;i++)
    printf("xx %d %.19g\n",i,array[i]);
  if (numberIterations_>300)
  exit(0);
#endif
  int numberRefinements=0;
  for (int iRefine = 0; iRefine < numberRefinements_ + 1; iRefine++) {
    int numberIn = thisVector->getNumElements();
    const int *  COIN_RESTRICT indexIn = thisVector->getIndices();
    const double *  COIN_RESTRICT arrayIn = thisVector->denseVector();
    CoinAbcScatterToList(arrayIn,abcSolution_,indexIn,abcPivotVariable_,numberIn);
    // check Ax == b  (for all)
    abcMatrix_->timesIncludingSlacks(-1.0, abcSolution_, solutionBasic_);
#if 0
    if (xxxxxx==0)
      for (int i=0;i<numberTotal_;i++)
	printf("sol %d %.19g\n",i,abcSolution_[i]);
    if (xxxxxx==0)
      for (int i=0;i<numberRows_;i++)
	printf("basic %d %.19g\n",i,solutionBasic_[i]);
#endif
    double multiplier = 131072.0;
    largestPrimalError_ = CoinAbcMaximumAbsElementAndScale(solutionBasic_,multiplier,numberRows_);
    double maxValue=0.0;
    for (int i=0;i<numberRows_;i++) {
      double value=fabs(solutionBasic_[i]);
      if (value>maxValue) {
#if 0
	if (xxxxxx==0)
	  printf("larger %.19gg at row %d\n",value,i);
	maxValue=value;
#endif
      }
    }
    if (largestPrimalError_ >= lastError) {
      // restore
      CoinIndexedVector * temp = thisVector;
      thisVector = lastVector;
      lastVector = temp;
      //goodSolution = false;
      break;
    }
    if (iRefine < numberRefinements_ && largestPrimalError_ > 1.0e-10) {
      // try and make better
      numberRefinements++;
      // save this
      CoinIndexedVector * temp = thisVector;
      thisVector = lastVector;
      lastVector = temp;
      int *  COIN_RESTRICT indexOut = thisVector->getIndices();
      int number = 0;
      array = thisVector->denseVector();
      thisVector->clear();
      for (int iRow = 0; iRow < numberRows_; iRow++) {
	double value = solutionBasic_[iRow];
	if (value) {
	  array[iRow] = value;
	  indexOut[number++] = iRow;
	  solutionBasic_[iRow] = 0.0;
	}
      }
      thisVector->setNumElements(number);
      lastError = largestPrimalError_;
      abcFactorization_->updateFullColumn(*thisVector);
      double * previous = lastVector->denseVector();
      number = 0;
      multiplier=1.0/multiplier;
      for (int iRow = 0; iRow < numberRows_; iRow++) {
	double value = previous[iRow] + multiplier * array[iRow];
	if (value) {
	  array[iRow] = value;
	  indexOut[number++] = iRow;
	} else {
	  array[iRow] = 0.0;
	}
      }
      thisVector->setNumElements(number);
    } else {
      break;
    }
  }
  
  // solution as accurate as we are going to get
  //if (!goodSolution) {
  CoinAbcMemcpy(solutionBasic_,thisVector->denseVector(),numberRows_);
  CoinAbcScatterTo(solutionBasic_,abcSolution_,abcPivotVariable_,numberRows_);
  arrayVector->clear();
  previousVector->clear();
  return numberRefinements;
}
// Moves basic stuff to basic area
void 
AbcSimplex::moveToBasic(int which)
{
  if ((which&1)!=0)
    CoinAbcGatherFrom(abcSolution_,solutionBasic_,abcPivotVariable_,numberRows_);
  if ((which&2)!=0)
    CoinAbcGatherFrom(abcCost_,costBasic_,abcPivotVariable_,numberRows_);
  if ((which&4)!=0)
    CoinAbcGatherFrom(abcLower_,lowerBasic_,abcPivotVariable_,numberRows_);
  if ((which&8)!=0)
    CoinAbcGatherFrom(abcUpper_,upperBasic_,abcPivotVariable_,numberRows_);
}
// now dual side
int
AbcSimplex::computeDuals(double * givenDjs, CoinIndexedVector * arrayVector, CoinIndexedVector * previousVector)
{
  double *  COIN_RESTRICT array = arrayVector->denseVector();
  int *  COIN_RESTRICT index = arrayVector->getIndices();
  int number = 0;
  if (!givenDjs) {
    for (int iRow = 0; iRow < numberRows_; iRow++) {
      double value = costBasic_[iRow];
      if (value) {
	array[iRow] = value;
	index[number++] = iRow;
      }
    }
  } else {
    // dual values pass - djs may not be zero
    for (int iRow = 0; iRow < numberRows_; iRow++) {
      int iPivot = abcPivotVariable_[iRow];
      // make sure zero if done
      if (!pivoted(iPivot))
	givenDjs[iPivot] = 0.0;
      double value = abcCost_[iPivot] - givenDjs[iPivot];
      if (value) {
	array[iRow] = value;
	index[number++] = iRow;
      }
    }
  }
  arrayVector->setNumElements(number);
  // Btran basic costs and get as accurate as possible
  double lastError = COIN_DBL_MAX;
  CoinIndexedVector * thisVector = arrayVector;
  CoinIndexedVector * lastVector = previousVector;
  abcFactorization_->updateFullColumnTranspose(*thisVector);
  
  int numberRefinements=0;
  for (int iRefine = 0; iRefine < numberRefinements_+1; iRefine++) {
    // check basic reduced costs zero
    // put reduced cost of basic into djBasic_
    CoinAbcMemcpy(djBasic_,costBasic_,numberRows_);
    abcMatrix_->transposeTimesBasic(-1.0,thisVector->denseVector(),djBasic_);
    largestDualError_ = CoinAbcMaximumAbsElement(djBasic_,numberRows_);
    if (largestDualError_ >= lastError) {
      // restore
      CoinIndexedVector * temp = thisVector;
      thisVector = lastVector;
      lastVector = temp;
      break;
    }
    if (iRefine < numberRefinements_ && largestDualError_ > 1.0e-10
	&& !givenDjs) {
      numberRefinements++;
      // try and make better
      // save this
      CoinIndexedVector * temp = thisVector;
      thisVector = lastVector;
      lastVector = temp;
      array = thisVector->denseVector();
      double multiplier = 131072.0;
      //array=djBasic_*multiplier
      CoinAbcMultiplyAdd(djBasic_,numberRows_,multiplier,array,0.0);
      lastError = largestDualError_;
      abcFactorization_->updateFullColumnTranspose( *thisVector);
#if ABC_DEBUG
      thisVector->checkClean();
#endif
      multiplier = 1.0 / multiplier;
      double *  COIN_RESTRICT previous = lastVector->denseVector();
      // array = array*multiplier+previous
      CoinAbcMultiplyAdd(previous,numberRows_,1.0,array,multiplier);
    } else {
      break;
    }
  }
  // now look at dual solution
  CoinAbcMemcpy(dual_,thisVector->denseVector(),numberRows_);
  CoinAbcMemset0(thisVector->denseVector(),numberRows_);
  thisVector->setNumElements(0);
  if (numberRefinements) {
    CoinAbcMemset0(lastVector->denseVector(),numberRows_);
    lastVector->setNumElements(0);
  }
  CoinAbcMemcpy(abcDj_,abcCost_,numberTotal_);
  abcMatrix_->transposeTimesNonBasic(-1.0, dual_,abcDj_);
  // If necessary - override results
  if (givenDjs) {
    // restore accurate duals
    CoinMemcpyN(abcDj_, numberTotal_, givenDjs);
  }
  //arrayVector->clear();
  //previousVector->clear();
#if ABC_DEBUG
  arrayVector->checkClean();
  previousVector->checkClean();
#endif
  return numberRefinements;
}

/* Factorizes using current basis.
   solveType - 1 iterating, 0 initial, -1 external
   - 2 then iterating but can throw out of basis
   If 10 added then in primal values pass
   Return codes are as from AbcSimplexFactorization unless initial factorization
   when total number of singularities is returned.
   Special case is numberRows_+1 -> all slack basis.
*/
int AbcSimplex::internalFactorize ( int solveType)
{
  assert (status_);
  
  bool valuesPass = false;
  if (solveType >= 10) {
    valuesPass = true;
    solveType -= 10;
  }
#if 0
  // Make sure everything is clean
  for (int iSequence = 0; iSequence < numberTotal_; iSequence++) {
    if(getInternalStatus(iSequence) == isFixed) {
      // double check fixed
      assert (abcUpper_[iSequence] == abcLower_[iSequence]);
      assert (fabs(abcSolution_[iSequence]-abcLower_[iSequence])<100.0*primalTolerance_);
    } else if (getInternalStatus(iSequence) == isFree) {
      assert (abcUpper_[iSequence] == COIN_DBL_MAX && abcLower_[iSequence]==-COIN_DBL_MAX);
    } else if (getInternalStatus(iSequence) == atLowerBound) {
      assert (fabs(abcSolution_[iSequence]-abcLower_[iSequence])<1000.0*primalTolerance_);
    } else if (getInternalStatus(iSequence) == atUpperBound) {
      assert (fabs(abcSolution_[iSequence]-abcUpper_[iSequence])<1000.0*primalTolerance_);
    } else if (getInternalStatus(iSequence) == superBasic) {
      assert (!valuesPass);
    }
  }
#endif
#if 0 //ndef NDEBUG
  // Make sure everything is clean
  double sumOutside=0.0;
  int numberOutside=0;
  //double sumOutsideLarge=0.0;
  int numberOutsideLarge=0;
  double sumInside=0.0;
  int numberInside=0;
  //double sumInsideLarge=0.0;
  int numberInsideLarge=0;
  char rowcol[] = {'R', 'C'};
  for (int iSequence = 0; iSequence < numberTotal_; iSequence++) {
    if(getInternalStatus(iSequence) == isFixed) {
      // double check fixed
      assert (abcUpper_[iSequence] == abcLower_[iSequence]);
      assert (fabs(abcSolution_[iSequence]-abcLower_[iSequence])<primalTolerance_);
    } else if (getInternalStatus(iSequence) == isFree) {
      assert (abcUpper_[iSequence] == COIN_DBL_MAX && abcLower_[iSequence]==-COIN_DBL_MAX);
    } else if (getInternalStatus(iSequence) == atLowerBound) {
      assert (fabs(abcSolution_[iSequence]-abcLower_[iSequence])<1000.0*primalTolerance_);
      if (abcSolution_[iSequence]<abcLower_[iSequence]) {
	numberOutside++;
#if ABC_NORMAL_DEBUG>1
	if (handler_->logLevel()==63)
	  printf("%c%d below by %g\n",rowcol[isColumn(iSequence)],sequenceWithin(iSequence),
		 abcLower_[iSequence]-abcSolution_[iSequence]);
#endif
	sumOutside-=abcSolution_[iSequence]-abcLower_[iSequence];
	if (abcSolution_[iSequence]<abcLower_[iSequence]-primalTolerance_) 
	  numberOutsideLarge++;
      } else if (abcSolution_[iSequence]>abcLower_[iSequence]) {
	numberInside++;
	sumInside+=abcSolution_[iSequence]-abcLower_[iSequence];
	if (abcSolution_[iSequence]>abcLower_[iSequence]+primalTolerance_) 
	  numberInsideLarge++;
      }
    } else if (getInternalStatus(iSequence) == atUpperBound) {
      assert (fabs(abcSolution_[iSequence]-abcUpper_[iSequence])<1000.0*primalTolerance_);
      if (abcSolution_[iSequence]>abcUpper_[iSequence]) {
	numberOutside++;
#if ABC_NORMAL_DEBUG>0
	if (handler_->logLevel()==63)
	  printf("%c%d above by %g\n",rowcol[isColumn(iSequence)],sequenceWithin(iSequence),
		 -(abcUpper_[iSequence]-abcSolution_[iSequence]));
#endif
	sumOutside+=abcSolution_[iSequence]-abcUpper_[iSequence];
	if (abcSolution_[iSequence]>abcUpper_[iSequence]+primalTolerance_) 
	  numberOutsideLarge++;
      } else if (abcSolution_[iSequence]<abcUpper_[iSequence]) {
	numberInside++;
	sumInside-=abcSolution_[iSequence]-abcUpper_[iSequence];
	if (abcSolution_[iSequence]<abcUpper_[iSequence]-primalTolerance_) 
	  numberInsideLarge++;
      }
    } else if (getInternalStatus(iSequence) == superBasic) {
      //assert (!valuesPass);
    }
  }
#if ABC_NORMAL_DEBUG>0
  if (numberInside+numberOutside)
    printf("%d outside summing to %g (%d large), %d inside summing to %g (%d large)\n",
	   numberOutside,sumOutside,numberOutsideLarge,
	   numberInside,sumInside,numberInsideLarge);
#endif
#endif
  for (int iSequence = 0; iSequence < numberTotal_; iSequence++) {
    AbcSimplex::Status status=getInternalStatus(iSequence);
    if (status!= basic&&status!=isFixed&&abcUpper_[iSequence] == abcLower_[iSequence])
      setInternalStatus(iSequence,isFixed);
  }
  if (numberIterations_==baseIteration_&&!valuesPass) {
    double largeValue = this->largeValue();
    double * COIN_RESTRICT solution = abcSolution_;
    for (int iSequence = 0; iSequence < numberTotal_; iSequence++) {
      AbcSimplex::Status status=getInternalStatus(iSequence);
      if (status== superBasic) {
	double lower = abcLower_[iSequence];
	double upper = abcUpper_[iSequence];
	double value = solution[iSequence];
	AbcSimplex::Status thisStatus=isFree;
	if (lower > -largeValue || upper < largeValue) {
	  if (lower!=upper) {
	    if (fabs(value - lower) < fabs(value - upper)) {
	      thisStatus=AbcSimplex::atLowerBound;
	      solution[iSequence] = lower;
	    } else {
	      thisStatus= AbcSimplex::atUpperBound;
	      solution[iSequence] = upper;
	    }
	  } else {
	    thisStatus= AbcSimplex::isFixed;
	    solution[iSequence] = upper;
	  }
	  setInternalStatus(iSequence,thisStatus);
	}
      }
    }
  }
  int status = abcFactorization_->factorize(this, solveType, valuesPass);
  if (status) {
    handler_->message(CLP_SIMPLEX_BADFACTOR, messages_)
      << status
      << CoinMessageEol;
    return -1;
  } else if (!solveType) {
    // Initial basis - return number of singularities
    int numberSlacks = 0;
    for (int iRow = 0; iRow < numberRows_; iRow++) {
      if (getInternalStatus(iRow) == basic)
	numberSlacks++;
    }
    status = CoinMax(numberSlacks - numberRows_, 0);
    if (status)
      printf("%d singularities\n",status);
    // special case if all slack
    if (numberSlacks == numberRows_) {
      status = numberRows_ + 1;
    }
  }
  return status;
}
// Sets objectiveValue_ from rawObjectiveValue_
void 
AbcSimplex::setClpSimplexObjectiveValue()
{
  objectiveValue_ = rawObjectiveValue_/(objectiveScale_ * rhsScale_);
  objectiveValue_ += objectiveOffset_;
}
/*
  This does basis housekeeping and does values for in/out variables.
  Can also decide to re-factorize
*/
int
AbcSimplex::housekeeping()
{
#define DELAYED_UPDATE
#ifdef DELAYED_UPDATE
  if (algorithm_<0) {
    // modify dualout
    dualOut_ /= alpha_;
    dualOut_ *= -directionOut_;
    //double oldValue = valueIn_;
    if (directionIn_ == -1) {
      // as if from upper bound
      valueIn_ = upperIn_ + dualOut_;
#if 0 //def ABC_DEBUG
      printf("In from upper dualout_ %g movement %g (old %g) valueIn_ %g using movement %g\n",
	     dualOut_,movement_,movementOld,valueIn_,upperIn_+movement_);
#endif
    } else {
      // as if from lower bound
      valueIn_ = lowerIn_ + dualOut_;
#if 0 //def ABC_DEBUG
      printf("In from lower dualout_ %g movement %g (old %g) valueIn_ %g using movement %g\n",
	     dualOut_,movement_,movementOld,valueIn_,lowerIn_+movement_);
#endif
    }
    // outgoing
    if (directionOut_ > 0) {
      valueOut_ = lowerOut_;
    } else {
      valueOut_ = upperOut_;
    }
#if ABC_NORMAL_DEBUG>3
    if (rawObjectiveValue_ < oldobj - 1.0e-5 && (handler_->logLevel() & 16))
      printf("obj backwards %g %g\n", rawObjectiveValue_, oldobj);
#endif
  }
#endif
#if 0 //ndef NDEBUG
  {
    int numberFlagged=0;
    for (int iRow = 0; iRow < numberRows_; iRow++) {
      int iPivot = abcPivotVariable_[iRow];
      if (flagged(iPivot)) 
	numberFlagged++;
    }
    assert (numberFlagged==numberFlagged_);
  }
#endif
  // save value of incoming and outgoing
  numberIterations_++;
  changeMade_++; // something has happened
  // incoming variable
  if (handler_->logLevel() > 7) {
    //if (handler_->detail(CLP_SIMPLEX_HOUSE1,messages_)<100) {
    handler_->message(CLP_SIMPLEX_HOUSE1, messages_)
      << directionOut_
      << directionIn_ << theta_
      << dualOut_ << dualIn_ << alpha_
      << CoinMessageEol;
    if (getInternalStatus(sequenceIn_) == isFree) {
      handler_->message(CLP_SIMPLEX_FREEIN, messages_)
	<< sequenceIn_
	<< CoinMessageEol;
    }
  }
  // change of incoming
  char rowcol[] = {'R', 'C'};
  if (abcUpper_[sequenceIn_] > 1.0e20 && abcLower_[sequenceIn_] < -1.0e20)
    progressFlag_ |= 2; // making real progress
#ifndef DELAYED_UPDATE
  abcSolution_[sequenceIn_] = valueIn_;
#endif
  if (abcUpper_[sequenceOut_] - abcLower_[sequenceOut_] < 1.0e-12)
    progressFlag_ |= 1; // making real progress
  if (sequenceIn_ != sequenceOut_) {
    if (alphaAccuracy_ > 0.0) {
      double value = fabs(alpha_);
      if (value > 1.0)
	alphaAccuracy_ *= value;
      else
	alphaAccuracy_ /= value;
    }
    setInternalStatus(sequenceIn_, basic);
    if (abcUpper_[sequenceOut_] - abcLower_[sequenceOut_] > 0) {
      // As Nonlinear costs may have moved bounds (to more feasible)
      // Redo using value
      if (fabs(valueOut_ - abcLower_[sequenceOut_]) < fabs(valueOut_ - abcUpper_[sequenceOut_])) {
	// going to lower
	setInternalStatus(sequenceOut_, atLowerBound);
      } else {
	// going to upper
	setInternalStatus(sequenceOut_, atUpperBound);
      }
    } else {
      // fixed
      setInternalStatus(sequenceOut_, isFixed);
    }
#ifndef DELAYED_UPDATE
    abcSolution_[sequenceOut_] = valueOut_;
#endif
#if PARTITION_ROW_COPY==1
    // move in row copy
    abcMatrix_->inOutUseful(sequenceIn_,sequenceOut_);
#endif
  } else {
    //if (objective_->type()<2)
    //assert (fabs(theta_)>1.0e-13);
    // flip from bound to bound
    // As Nonlinear costs may have moved bounds (to more feasible)
    // Redo using value
    if (fabs(valueIn_ - abcLower_[sequenceIn_]) < fabs(valueIn_ - abcUpper_[sequenceIn_])) {
      // as if from upper bound
      setInternalStatus(sequenceIn_, atLowerBound);
    } else {
      // as if from lower bound
      setInternalStatus(sequenceIn_, atUpperBound);
    }
  }
  setClpSimplexObjectiveValue();
  if (handler_->logLevel() > 7) {
    //if (handler_->detail(CLP_SIMPLEX_HOUSE2,messages_)<100) {
    handler_->message(CLP_SIMPLEX_HOUSE2, messages_)
      << numberIterations_ << objectiveValue()
      << rowcol[isColumn(sequenceIn_)] << sequenceWithin(sequenceIn_)
      << rowcol[isColumn(sequenceOut_)] << sequenceWithin(sequenceOut_);
    handler_->printing(algorithm_ < 0) << dualOut_ << theta_;
    handler_->printing(algorithm_ > 0) << dualIn_ << theta_;
    handler_->message() << CoinMessageEol;
  }
#if 0
  if (numberIterations_ > 10000)
    printf(" it %d %g %c%d %c%d\n"
	   , numberIterations_, objectiveValue()
	   , rowcol[isColumn(sequenceIn_)], sequenceWithin(sequenceIn_)
	   , rowcol[isColumn(sequenceOut_)], sequenceWithin(sequenceOut_));
#endif
  if (hitMaximumIterations())
    return 2;
  // check for small cycles
  int in = sequenceIn_;
  int out = sequenceOut_;
  int cycle = abcProgress_.cycle(in, out,
			      directionIn_, directionOut_);
  if (cycle > 0 ) {
#if ABC_NORMAL_DEBUG>0
    if (handler_->logLevel() >= 63)
      printf("Cycle of %d\n", cycle);
#endif
    // reset
    abcProgress_.startCheck();
    double random = randomNumberGenerator_.randomDouble();
    int extra = static_cast<int> (9.999 * random);
    int off[] = {1, 1, 1, 1, 2, 2, 2, 3, 3, 4};
    if (abcFactorization_->pivots() > cycle) {
      forceFactorization_ = CoinMax(1, cycle - off[extra]);
    } else {
      // need to reject something
      int iSequence;
      if (algorithm_<0) {
	iSequence = sequenceIn_;
      } else {
	/* should be better if don't reject incoming
	 as it is in basis */
	iSequence = sequenceOut_;
	// so can't happen immediately again
	sequenceOut_=-1;
      }
      char x = isColumn(iSequence) ? 'C' : 'R';
      if (handler_->logLevel() >= 63)
	handler_->message(CLP_SIMPLEX_FLAG, messages_)
	  << x << sequenceWithin(iSequence)
	  << CoinMessageEol;
      setFlagged(iSequence);
      //printf("flagging %d\n",iSequence);
    }
    return 1;
  }
  int invertNow=0;
  // only time to re-factorize if one before real time
  // this is so user won't be surprised that maximumPivots has exact meaning
  int numberPivots = abcFactorization_->pivots();
  if (algorithm_<0)
    numberPivots++; // allow for update not done
  int maximumPivots = abcFactorization_->maximumPivots();
  int numberDense = 0; //abcFactorization_->numberDense();
  bool dontInvert = ((specialOptions_ & 16384) != 0 && numberIterations_ * 3 >
		     2 * maximumIterations());
  if (numberPivots == maximumPivots ||
      maximumPivots < 2) {
    // If dense then increase
    if (maximumPivots > 100 && numberDense > 1.5 * maximumPivots) {
      abcFactorization_->maximumPivots(numberDense);
    }
    //printf("ZZ maxpivots %d its %d\n",numberPivots,maximumPivots);
    return 1;
  } else if ((abcFactorization_->timeToRefactorize() && !dontInvert)
	     ||invertNow) {
    //printf("ZZ time invertNow %s its %d\n",invertNow ? "yes":"no",numberPivots);
    return 1;
  } else if (forceFactorization_ > 0 &&
#ifndef DELAYED_UPDATE
	     abcFactorization_->pivots() 
#else
	     abcFactorization_->pivots()+1
#endif 
	     >= forceFactorization_) {
    // relax
    forceFactorization_ = (3 + 5 * forceFactorization_) / 4;
    if (forceFactorization_ > abcFactorization_->maximumPivots())
      forceFactorization_ = -1; //off
    //printf("ZZ forceFactor %d its %d\n",forceFactorization_,numberPivots);
    return 1;
  } else if (numberIterations_ > 1000 + 10 * (numberRows_ + (numberColumns_ >> 2))) {
    // A bit worried problem may be cycling - lets factorize at random intervals for a short period
    int numberTooManyIterations = numberIterations_ - 10 * (numberRows_ + (numberColumns_ >> 2));
    double random = randomNumberGenerator_.randomDouble();
    int window = numberTooManyIterations%5000;
    if (window<2*maximumPivots)
      random = 0.2*random+0.8; // randomly re-factorize but not too soon
    else
      random=1.0; // switch off if not in window of opportunity
    int maxNumber = (forceFactorization_ < 0) ? maximumPivots : CoinMin(forceFactorization_, maximumPivots);
    if (abcFactorization_->pivots() >= random * maxNumber) {
      //printf("ZZ cycling a %d\n",numberPivots);
      return 1;
    } else if (numberIterations_ > 1000000 + 10 * (numberRows_ + (numberColumns_ >> 2)) &&
	       numberIterations_ < 1000010 + 10 * (numberRows_ + (numberColumns_ >> 2))) {
      //printf("ZZ cycling b %d\n",numberPivots);
      return 1;
    } else {
      // carry on iterating
      return 0;
    }
  } else {
    // carry on iterating
    return 0;
  }
}
// Swaps primal stuff 
void 
AbcSimplex::swapPrimalStuff()
{
  if (sequenceOut_<0)
    return;
  assert (sequenceIn_>=0);
  abcSolution_[sequenceOut_] = valueOut_;
  abcSolution_[sequenceIn_] = valueIn_;
  solutionBasic_[pivotRow_]=valueIn_;
  if (algorithm_<0) {
    // and set bounds correctly
    static_cast<AbcSimplexDual *> (this)->originalBound(sequenceIn_);
    static_cast<AbcSimplexDual *> (this)->changeBound(sequenceOut_);
  } else {
#if ABC_NORMAL_DEBUG>2
    if (handler_->logLevel()==63)
      printf("Code swapPrimalStuff for primal\n");
#endif
  }
  if (pivotRow_>=0) { // may be flip in primal
    lowerBasic_[pivotRow_]=abcLower_[sequenceIn_];
    upperBasic_[pivotRow_]=abcUpper_[sequenceIn_];
    costBasic_[pivotRow_]=abcCost_[sequenceIn_];
    abcPivotVariable_[pivotRow_] = sequenceIn_;
  }
}
// Swaps dual stuff
void 
AbcSimplex::swapDualStuff(int lastSequenceOut,int lastDirectionOut)
{
  // outgoing
  // set dj to zero unless values pass
  if (lastSequenceOut>=0) {
    if ((stateOfProblem_&VALUES_PASS)==0) {
      if (lastDirectionOut > 0) {
	abcDj_[lastSequenceOut] = theta_;
      } else {
	abcDj_[lastSequenceOut] = -theta_;
      }
    } else {
      if (lastDirectionOut > 0) {
	abcDj_[lastSequenceOut] -= theta_;
      } else {
	abcDj_[lastSequenceOut] += theta_;
      }
    }
  }
  if (sequenceIn_>=0) {
    abcDj_[sequenceIn_]=0.0;
    //costBasic_[pivotRow_]=abcCost_[sequenceIn_];
  }
}
static void solveMany(int number,ClpSimplex ** simplex)
{
  for (int i=0;i<number-1;i++)
    cilk_spawn simplex[i]->dual(0);
  simplex[number-1]->dual(0);
  cilk_sync;
}
void 
AbcSimplex::crash (int type)
{
  int i;
  for (i=0;i<numberRows_;i++) {
    if (getInternalStatus(i)!=basic)
      break;
  }
  if (i<numberRows_)
    return;
  // all slack
  if (type==3) {
    // decomposition crash
    if (!abcMatrix_->gotRowCopy())
      abcMatrix_->createRowCopy();
    //const double * element = abcMatrix_->getPackedMatrix()->getElements();
    const CoinBigIndex * columnStart = abcMatrix_->getPackedMatrix()->getVectorStarts();
    const int * row = abcMatrix_->getPackedMatrix()->getIndices();
    //const double * elementByRow = abcMatrix_->rowElements();
    const CoinBigIndex * rowStart = abcMatrix_->rowStart();
    const CoinBigIndex * rowEnd = abcMatrix_->rowEnd();
    const int * column = abcMatrix_->rowColumns();
    int * blockStart = usefulArray_[0].getIndices();
    int * columnBlock = blockStart+numberRows_;
    int * nextColumn = usefulArray_[1].getIndices();
    int * blockCount = nextColumn+numberColumns_;
    int direction[2]={-1,1};
    int bestBreak=-1;
    double bestValue=0.0;
    int iPass=0;
    int halfway=(numberRows_+1)/2;
    int firstMaster=-1;
    int lastMaster=-2;
    int numberBlocks=0;
    int largestRows=0;
    int largestColumns=0;
    int numberEmpty=0;
    int numberMaster=0;
    int numberEmptyColumns=0;
    int numberMasterColumns=0;
    while (iPass<2) {
      int increment=direction[iPass];
      int start= increment>0 ? 0 : numberRows_-1;
      int stop=increment>0 ? numberRows_ : -1;
      numberBlocks=0;
      int thisBestBreak=-1;
      double thisBestValue=COIN_DBL_MAX;
      int numberRowsDone=0;
      int numberMarkedColumns=0;
      int maximumBlockSize=0;
      for (int i=0;i<numberRows_;i++) { 
	blockStart[i]=-1;
	blockCount[i]=0;
      }
      for (int i=0;i<numberColumns_;i++) {
	columnBlock[i]=-1;
	nextColumn[i]=-1;
      }
      for (int iRow=start;iRow!=stop;iRow+=increment) {
	int iBlock = -1;
	for (CoinBigIndex j=rowStart[iRow];j<rowEnd[iRow];j++) {
	  int iColumn=column[j]-maximumAbcNumberRows_;
	  int whichColumnBlock=columnBlock[iColumn];
	  if (whichColumnBlock>=0) {
	    // column marked
	    if (iBlock<0) {
	      // put row in that block
	      iBlock=whichColumnBlock;
	    } else if (iBlock!=whichColumnBlock) {
	      // merge
	      blockCount[iBlock]+=blockCount[whichColumnBlock];
	      blockCount[whichColumnBlock]=0;
	      int jColumn=blockStart[whichColumnBlock];
#ifndef NDEBUG
	      int iiColumn=iColumn;
#endif
	      while (jColumn>=0) {
		assert (columnBlock[jColumn]==whichColumnBlock);
		columnBlock[jColumn]=iBlock;
#ifndef NDEBUG
		if (jColumn==iiColumn)
		  iiColumn=-1;
#endif
		iColumn=jColumn;
		jColumn=nextColumn[jColumn];
	      }
#ifndef NDEBUG
	      assert (iiColumn==-1);
#endif
	      nextColumn[iColumn]=blockStart[iBlock];
	      blockStart[iBlock]=blockStart[whichColumnBlock];
	      blockStart[whichColumnBlock]=-1;
	    }
	  }
	}
	int n=numberMarkedColumns;
	if (iBlock<0) {
	  //new block
	  if (rowEnd[iRow]>rowStart[iRow]) {
	    numberBlocks++;
	    iBlock=numberBlocks;
	    int jColumn=column[rowStart[iRow]]-maximumAbcNumberRows_;
	    columnBlock[jColumn]=iBlock;
	    blockStart[iBlock]=jColumn;
	    numberMarkedColumns++;
	    for (CoinBigIndex j=rowStart[iRow]+1;j<rowEnd[iRow];j++) {
	      int iColumn=column[j]-maximumAbcNumberRows_;
	      columnBlock[iColumn]=iBlock;
	      numberMarkedColumns++;
	      nextColumn[jColumn]=iColumn;
	      jColumn=iColumn;
	    }
	    blockCount[iBlock]=numberMarkedColumns-n;
	  } else {
	    // empty
	  }
	} else {
	  // put in existing block
	  int jColumn=blockStart[iBlock];
	  for (CoinBigIndex j=rowStart[iRow];j<rowEnd[iRow];j++) {
	    int iColumn=column[j]-maximumAbcNumberRows_;
	    assert (columnBlock[iColumn]<0||columnBlock[iColumn]==iBlock);
	    if (columnBlock[iColumn]<0) {
	      columnBlock[iColumn]=iBlock;
	      numberMarkedColumns++;
	      nextColumn[iColumn]=jColumn;
	      jColumn=iColumn;
	    }
	  }
	  blockStart[iBlock]=jColumn;
	  blockCount[iBlock]+=numberMarkedColumns-n;
	}
	if (iBlock>=0)
	  maximumBlockSize=CoinMax(maximumBlockSize,blockCount[iBlock]);
	numberRowsDone++;
	if (thisBestValue*numberRowsDone > maximumBlockSize&&numberRowsDone>halfway) { 
	  thisBestBreak=iRow;
	  thisBestValue=static_cast<double>(maximumBlockSize)/
	    static_cast<double>(numberRowsDone);
	}
      }
      if (thisBestBreak==stop)
	thisBestValue=COIN_DBL_MAX;
      iPass++;
      if (iPass==1) {
	bestBreak=thisBestBreak;
	bestValue=thisBestValue;
      } else {
	if (bestValue<thisBestValue) {
	  firstMaster=0;
	  lastMaster=bestBreak;
	} else {
	  firstMaster=thisBestBreak; // ? +1
	  lastMaster=numberRows_;
	}
      }
    }
    bool useful=false;
    if (firstMaster<lastMaster) {
      for (int i=0;i<numberRows_;i++) 
	blockStart[i]=-1;
      for (int i=firstMaster;i<lastMaster;i++)
	blockStart[i]=-2;
      for (int i=0;i<numberColumns_;i++)
	columnBlock[i]=-1;
      int firstRow=0;
      numberBlocks=-1;
      while (true) {
	for (;firstRow<numberRows_;firstRow++) {
	  if (blockStart[firstRow]==-1)
	    break;
	}
	if (firstRow==numberRows_)
	  break;
	int nRows=0;
	numberBlocks++;
	int numberStack=1;
	blockCount[0] = firstRow;
	while (numberStack) {
	  int iRow=blockCount[--numberStack];
	  for (CoinBigIndex j=rowStart[iRow];j<rowEnd[iRow];j++) {
	    int iColumn=column[j]-maximumAbcNumberRows_;
	    int iBlock=columnBlock[iColumn];
	    if (iBlock<0) {
	      columnBlock[iColumn]=numberBlocks;
	      for (CoinBigIndex k=columnStart[iColumn];
		   k<columnStart[iColumn+1];k++) {
		int jRow=row[k];
		int rowBlock=blockStart[jRow];
		if (rowBlock==-1) {
		  nRows++;
		  blockStart[jRow]=numberBlocks;
		  blockCount[numberStack++]=jRow;
		}
	      }
	    }
	  }
	}
	if (!nRows) {
	  // empty!!
	  numberBlocks--;
	}
	firstRow++;
      }
      // adjust 
      numberBlocks++;
      for (int i=0;i<numberBlocks;i++) {
	blockCount[i]=0;
	nextColumn[i]=0;
      }
      for (int iRow = 0; iRow < numberRows_; iRow++) {
	int iBlock=blockStart[iRow];
	if (iBlock>=0) {
	  blockCount[iBlock]++;
	} else {
	  if (iBlock==-2)
	    numberMaster++;
	  else
	    numberEmpty++;
	}
      }
      for (int iColumn = 0; iColumn < numberColumns_; iColumn++) {
	int iBlock=columnBlock[iColumn];
	if (iBlock>=0) {
	  nextColumn[iBlock]++;
	} else {
	  if (columnStart[iColumn+1]>columnStart[iColumn])
	    numberMasterColumns++;
	  else
	    numberEmptyColumns++;
	}
      }
      for (int i=0;i<numberBlocks;i++) {
	if (blockCount[i]+nextColumn[i]>largestRows+largestColumns) {
	  largestRows=blockCount[i];
	  largestColumns=nextColumn[i];
	}
      }
      useful=true;
      if (numberMaster>halfway||largestRows*3>numberRows_)
	useful=false;
    }
    if (useful) {
#if ABC_NORMAL_DEBUG>0
      if (logLevel()>=2)
	printf("%d master rows %d <= < %d\n",lastMaster-firstMaster,
	       firstMaster,lastMaster);
      printf("%s %d blocks (largest %d,%d), %d master rows (%d empty) out of %d, %d master columns (%d empty) out of %d\n",
	     useful ? "**Useful" : "NoGood",
	     numberBlocks,largestRows,largestColumns,numberMaster,numberEmpty,numberRows_,
	     numberMasterColumns,numberEmptyColumns,numberColumns_);
      if (logLevel()>=2) {
	for (int i=0;i<numberBlocks;i++) 
	  printf("Block %d has %d rows and %d columns\n",
	       i,blockCount[i],nextColumn[i]);
      }
#endif
#define NUMBER_DW_BLOCKS 20
      int minSize1=(numberRows_-numberMaster+NUMBER_DW_BLOCKS-1)/NUMBER_DW_BLOCKS;
      int minSize2=(numberRows_-numberMaster+2*NUMBER_DW_BLOCKS-1)/(2*NUMBER_DW_BLOCKS);
      int * backRow = usefulArray_[1].getIndices();
      // first sort
      for (int i=0;i<numberBlocks;i++) {
	backRow[i]=-(3*blockCount[i]+0*nextColumn[i]);
	nextColumn[i]=i;
      }
      CoinSort_2(backRow,backRow+numberBlocks,nextColumn);
      // keep if >minSize2 or sum >minSize1;
      int n=0;
      for (int i=0;i<numberBlocks;i++) {
	int originalBlock=nextColumn[i];
	if (blockCount[originalBlock]<minSize2)
	  break;
	n++;
      }
      int size=minSize1;
      for (int i=n;i<numberBlocks;i++) {
	int originalBlock=nextColumn[i];
	size-=blockCount[originalBlock];
	if (size>0&&i<numberBlocks-1) {
	  blockCount[originalBlock]=-1;
	} else {
	  size=minSize1;
	  n++;
	}
      }
      int n2=numberBlocks;
      numberBlocks=n;
      for (int i=n2-1;i>=0;i--) {
	int originalBlock=nextColumn[i];
	if (blockCount[originalBlock]>0)
	  n--;
	blockCount[originalBlock]=n;
      }
      assert (!n);
      for (int iRow = 0; iRow < numberRows_; iRow++) {
	int iBlock=blockStart[iRow];
	if (iBlock>=0) 
	  blockStart[iRow]=blockCount[iBlock];
      }
      for (int iColumn = 0; iColumn < numberColumns_; iColumn++) {
	int iBlock=columnBlock[iColumn];
	if (iBlock>=0) 
	  columnBlock[iColumn]=blockCount[iBlock];
      }
      // stick to Clp for now
      ClpSimplex ** simplex = new ClpSimplex * [numberBlocks]; 
      for (int iBlock=0;iBlock<numberBlocks;iBlock++) {
	int nRow=0;
	int nColumn=0;
	for (int iRow=0;iRow<numberRows_;iRow++) {
	  if (blockStart[iRow]==iBlock) 
	    blockCount[nRow++]=iRow;
	}
	for (int iColumn=0;iColumn<numberColumns_;iColumn++) {
	  if (columnBlock[iColumn]==iBlock) 
	    nextColumn[nColumn++]=iColumn;
	}
	simplex[iBlock]=new ClpSimplex(this,nRow,blockCount,nColumn,nextColumn);
	simplex[iBlock]->setSpecialOptions(simplex[iBlock]->specialOptions()&(~65536));
	if (logLevel()<2)
	  simplex[iBlock]->setLogLevel(0);
      }
      solveMany(numberBlocks,simplex);
      int numberBasic=numberMaster;
      int numberStructurals=0;
      for (int i=0;i<numberMaster;i++)
	abcPivotVariable_[i]=i+firstMaster;
      for (int iBlock=0;iBlock<numberBlocks;iBlock++) {
	int nRow=0;
	int nColumn=0;
	// from Clp enum to Abc enum (and bound flip)
	unsigned char lookupToAbcSlack[6]={4,6,0/*1*/,1/*0*/,5,7};
	unsigned char *  COIN_RESTRICT getStatus = simplex[iBlock]->statusArray()+
	  simplex[iBlock]->numberColumns();
	double *  COIN_RESTRICT solutionSaved=solutionSaved_;
	double *  COIN_RESTRICT lowerSaved=lowerSaved_;
	double *  COIN_RESTRICT upperSaved=upperSaved_;
	for (int iRow=0;iRow<numberRows_;iRow++) {
	  if (blockStart[iRow]==iBlock) {
	    unsigned char status=getStatus[nRow++]&7;
	    AbcSimplex::Status abcStatus=static_cast<AbcSimplex::Status>(lookupToAbcSlack[status]);
	    if (status!=ClpSimplex::basic) {
	      double lowerValue=lowerSaved[iRow];
	      double upperValue=upperSaved[iRow];
	      if (lowerValue==-COIN_DBL_MAX) {
		if(upperValue==COIN_DBL_MAX) {
		  // free
		  abcStatus=isFree;
		} else {
		  abcStatus=atUpperBound;
		}
	      } else if (upperValue==COIN_DBL_MAX) {
		abcStatus=atLowerBound;
	      } else if (lowerValue==upperValue) {
		abcStatus=isFixed;
	      }
	      switch (abcStatus) {
	      case isFixed:
	      case atLowerBound:
		solutionSaved[iRow]=lowerValue;
		break;
	      case atUpperBound:
		solutionSaved[iRow]=upperValue;
		break;
	      default:
		break;
	      }
	    } else {
	      // basic
	      abcPivotVariable_[numberBasic++]=iRow;
	    }
	    internalStatus_[iRow]=abcStatus;
	  }
	}
	// from Clp enum to Abc enum
	unsigned char lookupToAbc[6]={4,6,1,0,5,7};
	unsigned char *  COIN_RESTRICT putStatus=internalStatus_+maximumAbcNumberRows_;
	getStatus = simplex[iBlock]->statusArray();
	solutionSaved+= maximumAbcNumberRows_;
	lowerSaved+= maximumAbcNumberRows_;
	upperSaved+= maximumAbcNumberRows_;
	int numberSaved=numberBasic;
	for (int iColumn=0;iColumn<numberColumns_;iColumn++) {
	  if (columnBlock[iColumn]==iBlock) {
	    unsigned char status=getStatus[nColumn++]&7;
	    AbcSimplex::Status abcStatus=static_cast<AbcSimplex::Status>(lookupToAbc[status]);
	    if (status!=ClpSimplex::basic) {
	      double lowerValue=lowerSaved[iColumn];
	      double upperValue=upperSaved[iColumn];
	      if (lowerValue==-COIN_DBL_MAX) {
		if(upperValue==COIN_DBL_MAX) {
		  // free
		  abcStatus=isFree;
		} else {
		  abcStatus=atUpperBound;
		}
	      } else if (upperValue==COIN_DBL_MAX) {
		abcStatus=atLowerBound;
	      } else if (lowerValue==upperValue) {
		abcStatus=isFixed;
	      } else if (abcStatus==isFree) {
		abcStatus=superBasic;
	      }
	      switch (abcStatus) {
	      case isFixed:
	      case atLowerBound:
		solutionSaved[iColumn]=lowerValue;
		break;
	      case atUpperBound:
		solutionSaved[iColumn]=upperValue;
		break;
	    default:
	      break;
	      }
	    } else {
	      // basic
	      if (numberBasic<numberRows_) 
		abcPivotVariable_[numberBasic++]=iColumn+maximumAbcNumberRows_;
	      else
		abcStatus=superBasic;
	    }
	    putStatus[iColumn]=abcStatus;
	  }
	}
	numberStructurals+=numberBasic-numberSaved;
	delete simplex[iBlock];
      }
#if ABC_NORMAL_DEBUG>0
      printf("%d structurals put into basis\n",numberStructurals);
#endif
      if (numberBasic<numberRows_) {
	for (int iRow=0;iRow<numberRows_;iRow++) {
	  AbcSimplex::Status status=getInternalStatus(iRow);
	  if (status!=AbcSimplex::basic) {
	    abcPivotVariable_[numberBasic++]=iRow;
	    setInternalStatus(iRow,basic);
	    if (numberBasic==numberRows_)
	      break;
	  }
	}
      }
      assert (numberBasic==numberRows_);
#if 0
      int n2=0;
      for (int i=0;i<numberTotal_;i++) {
	if (getInternalStatus(i)==basic)
	  n2++;
      }
      assert (n2==numberRows_);
      std::sort(abcPivotVariable_,abcPivotVariable_+numberRows_);
      n2=-1;
      for (int i=0;i<numberRows_;i++) {
	assert (abcPivotVariable_[i]>n2);
	n2=abcPivotVariable_[i];
      }
#endif
      delete [] simplex;
      return;
    } else {
      // try another
      type=2;
    }
  }
  if (type==1) {
    const double * linearObjective = abcCost_+maximumAbcNumberRows_;
    double gamma=0.0;
    int numberTotal=numberRows_+numberColumns_;
    double * q = new double [numberTotal];
    double * v = q+numberColumns_;
    int * which = new int [numberTotal+3*numberRows_];
    int * ii = which+numberColumns_;
    int * r = ii+numberRows_;
    int * pivoted = r+numberRows_;
    for (int iColumn = 0; iColumn < numberColumns_; iColumn++) {
      gamma=CoinMax(gamma,linearObjective[iColumn]);
    }
    for (int iRow = 0; iRow < numberRows_; iRow++) {
      double lowerBound = abcLower_[iRow];
      double upperBound = abcUpper_[iRow];
      pivoted[iRow]=-1;
      ii[iRow]=0;
      r[iRow]=0;
      v[iRow]=COIN_DBL_MAX;
      if (lowerBound==upperBound)
	continue;
      if (lowerBound<=0.0&&upperBound>=0.0) {
	pivoted[iRow]=iRow;
	ii[iRow]=1;
	r[iRow]=1;
      }
    }
    int nPossible=0;
    int lastPossible=0;
    double cMaxDiv;
    if (gamma)
      cMaxDiv=1.0/(1000.0*gamma);
    else
      cMaxDiv=1.0;
    const double * columnLower = abcLower_+maximumAbcNumberRows_;
    const double * columnUpper = abcUpper_+maximumAbcNumberRows_;
    for (int iPass=0;iPass<3;iPass++) {
      for (int iColumn = 0; iColumn < numberColumns_; iColumn++) {
	double lowerBound = columnLower[iColumn];
	double upperBound = columnUpper[iColumn];
	if (lowerBound==upperBound)
	  continue;
	double qValue; 
	if (lowerBound > -1.0e20 ) {
	  if (upperBound < 1.0e20) {
	    // both
	    qValue=lowerBound-upperBound;
	    if (iPass!=2)
	      qValue=COIN_DBL_MAX;
	  } else {
	    // just lower
	    qValue=lowerBound;
	    if (iPass!=1)
	      qValue=COIN_DBL_MAX;
	  }
	} else {
	  if (upperBound < 1.0e20) {
	    // just upper
	    qValue=-upperBound;
	    if (iPass!=1)
	      qValue=COIN_DBL_MAX;
	  } else {
	    // free
	    qValue=0.0;
	    if (iPass!=0)
	      qValue=COIN_DBL_MAX;
	  }
	}
	if (qValue!=COIN_DBL_MAX) {
	  which[nPossible]=iColumn;
	  q[nPossible++]=qValue+linearObjective[iColumn]*cMaxDiv;;
	}
      }
      CoinSort_2(q+lastPossible,q+nPossible,which+lastPossible);
      lastPossible=nPossible;
    }
    const double * element = abcMatrix_->getPackedMatrix()->getElements();
    const CoinBigIndex * columnStart = abcMatrix_->getPackedMatrix()->getVectorStarts();
    //const int * columnLength = abcMatrix_->getPackedMatrix()->getVectorLengths();
    const int * row = abcMatrix_->getPackedMatrix()->getIndices();
    int nPut=0;
    for (int i=0;i<nPossible;i++) {
      int iColumn=which[i];
      double maxAlpha=0.0;
      int kRow=-1;
      double alternativeAlpha=0.0;
      int jRow=-1;
      bool canTake=true;
      for (CoinBigIndex j=columnStart[iColumn];j<columnStart[iColumn+1];j++) {
	int iRow=row[j];
	double alpha=fabs(element[j]);
	if (alpha>0.01*v[iRow]) {
	  canTake=false;
	} else if (!ii[iRow]&&alpha>alternativeAlpha) {
	  alternativeAlpha=alpha;
	  jRow=iRow;
	}
	if (!r[iRow]&&alpha>maxAlpha) {
	  maxAlpha=alpha;
	  kRow=iRow;
	}
      }
      // only really works if scaled
      if (maxAlpha>0.99) {
	pivoted[kRow]=iColumn+maximumAbcNumberRows_;
	v[kRow]=maxAlpha;
	ii[kRow]=1;
	for (CoinBigIndex j=columnStart[iColumn];j<columnStart[iColumn+1];j++) {
	  int iRow=row[j];
	  r[iRow]++;
	}
	nPut++;
      } else if (canTake&&jRow>=0) {
	pivoted[jRow]=iColumn+maximumAbcNumberRows_;
	v[jRow]=maxAlpha;
	ii[jRow]=1;
	for (CoinBigIndex j=columnStart[iColumn];j<columnStart[iColumn+1];j++) {
	  int iRow=row[j];
	  r[iRow]++;
	}
	nPut++;
      }
    }
    for (int iRow=0;iRow<numberRows_;iRow++) {
      int iSequence=pivoted[iRow];
      if (iSequence>=0&&iSequence<numberColumns_) {
	abcPivotVariable_[iRow]=iSequence;
	if (fabs(abcLower_[iRow])<fabs(abcUpper_[iRow])) {
	  setInternalStatus(iRow,atLowerBound);
	  abcSolution_[iRow]=abcLower_[iRow];
	  solutionSaved_[iRow]=abcLower_[iRow];
	} else {
	  setInternalStatus(iRow,atUpperBound);
	  abcSolution_[iRow]=abcUpper_[iRow];
	  solutionSaved_[iRow]=abcUpper_[iRow];
	}
	setInternalStatus(iSequence,basic);
      }
    }
#if ABC_NORMAL_DEBUG>0
    printf("%d put into basis\n",nPut);
#endif
    delete [] q;
    delete [] which;
  } else if (type==2) {
    //return;
    int numberBad=0;
    CoinAbcMemcpy(abcDj_,abcCost_,numberTotal_);
    // Work on savedSolution and current
    int iVector=getAvailableArray();
#define ALLOW_BAD_DJS
#ifdef ALLOW_BAD_DJS
    double * modDj = usefulArray_[iVector].denseVector();
#endif
    for (int iSequence=maximumAbcNumberRows_;iSequence<numberTotal_;iSequence++) {
      double dj=abcDj_[iSequence];
      modDj[iSequence]=0.0;
      if (getInternalStatus(iSequence)==atLowerBound) {
	if (dj<-dualTolerance_) {
	  double costThru=-dj*(abcUpper_[iSequence]-abcLower_[iSequence]);
	  if (costThru<dualBound_) {
	    // flip
	    setInternalStatus(iSequence,atUpperBound);
	    solutionSaved_[iSequence]=abcUpper_[iSequence];
	    abcSolution_[iSequence]=abcUpper_[iSequence];
	  } else {
	    numberBad++;
#ifdef ALLOW_BAD_DJS
	    modDj[iSequence]=dj;
	    dj=0.0;
#else
	    break;
#endif
	  }
	} else {
	  dj=CoinMax(dj,0.0);
	}
      } else if (getInternalStatus(iSequence)==atLowerBound) {
	if (dj>dualTolerance_) {
	  double costThru=dj*(abcUpper_[iSequence]-abcLower_[iSequence]);
	  if (costThru<dualBound_) {
	    assert (abcUpper_[iSequence]-abcLower_[iSequence]<1.0e10);
	    // flip
	    setInternalStatus(iSequence,atLowerBound);
	    solutionSaved_[iSequence]=abcLower_[iSequence];
	    abcSolution_[iSequence]=abcLower_[iSequence];
	  } else {
	    numberBad++;
#ifdef ALLOW_BAD_DJS
	    modDj[iSequence]=dj;
	    dj=0.0;
#else
	    break;
#endif
	  }
	} else {
	  dj=CoinMin(dj,0.0);
	}
      } else {
	if (fabs(dj)<dualTolerance_) {
	  dj=0.0;
	} else {
	  numberBad++;
#ifdef ALLOW_BAD_DJS
	  modDj[iSequence]=dj;
	  dj=0.0;
#else
	  break;
#endif
	}
      }
      abcDj_[iSequence]=dj;
    }
#ifndef ALLOW_BAD_DJS
    if (numberBad) {
      //CoinAbcMemset0(modDj+maximumAbcNumberRows_,numberColumns_);
      return ;
    }
#endif
    if (!abcMatrix_->gotRowCopy())
      abcMatrix_->createRowCopy();
    //const double * element = abcMatrix_->getPackedMatrix()->getElements();
    const CoinBigIndex * columnStart = abcMatrix_->getPackedMatrix()->getVectorStarts()-maximumAbcNumberRows_;
    const int * row = abcMatrix_->getPackedMatrix()->getIndices();
    const double * elementByRow = abcMatrix_->rowElements();
    const CoinBigIndex * rowStart = abcMatrix_->rowStart();
    const CoinBigIndex * rowEnd = abcMatrix_->rowEnd();
    const int * column = abcMatrix_->rowColumns();
    CoinAbcMemset0(solutionBasic_,numberRows_);
    CoinAbcMemcpy(lowerBasic_,abcLower_,numberRows_);
    CoinAbcMemcpy(upperBasic_,abcUpper_,numberRows_);
    abcMatrix_->timesIncludingSlacks(-1.0,solutionSaved_,solutionBasic_);
    const double multiplier[] = { 1.0, -1.0};
    int nBasic=0;
    int * index=usefulArray_[iVector].getIndices();
    int iVector2=getAvailableArray();
    int * index2=usefulArray_[iVector2].getIndices();
    double * sort =usefulArray_[iVector2].denseVector();
    int average = CoinMax(5,rowEnd[numberRows_-1]/(8*numberRows_));
    int nPossible=0;
    if (numberRows_>10000) {
      // allow more
      for (int iRow=0;iRow<numberRows_;iRow++) {
	double solutionValue=solutionBasic_[iRow];
	double infeasibility=0.0;
	double lowerValue=lowerBasic_[iRow];
	double upperValue=upperBasic_[iRow];
	if (solutionValue<lowerValue-primalTolerance_) {
	  infeasibility=-(lowerValue-solutionValue);
	} else if (solutionValue>upperValue+primalTolerance_) {
	  infeasibility=upperValue-solutionValue;
	}
	int length=rowEnd[iRow]-rowStart[iRow];
	if (infeasibility) 
	  index2[nPossible++]=length;
      }
      std::sort(index2,index2+nPossible);
      // see how much we need to get numberRows/10 or nPossible/3
      average=CoinMax(average,index2[CoinMin(numberRows_/10,nPossible/3)]);
      nPossible=0;
    }
    for (int iRow=0;iRow<numberRows_;iRow++) {
      double solutionValue=solutionBasic_[iRow];
      double infeasibility=0.0;
      double lowerValue=lowerBasic_[iRow];
      double upperValue=upperBasic_[iRow];
      if (solutionValue<lowerValue-primalTolerance_) {
	infeasibility=-(lowerValue-solutionValue);
      } else if (solutionValue>upperValue+primalTolerance_) {
	infeasibility=upperValue-solutionValue;
      }
      int length=rowEnd[iRow]-rowStart[iRow];
      if (infeasibility&&length<average) {
	index2[nPossible]=iRow;
	sort[nPossible++]=1.0e5*infeasibility-iRow;
	//sort[nPossible++]=1.0e9*length-iRow;//infeasibility;
      }
    }
    CoinSort_2(sort,sort+nPossible,index2);
    for (int iWhich=0;iWhich<nPossible;iWhich++) {
      int iRow = index2[iWhich];
      sort[iWhich]=0.0;
      if (abcDj_[iRow])
	continue; // marked as invalid
      double solutionValue=solutionBasic_[iRow];
      double infeasibility=0.0;
      double lowerValue=lowerBasic_[iRow];
      double upperValue=upperBasic_[iRow];
      if (solutionValue<lowerValue-primalTolerance_) {
	infeasibility=lowerValue-solutionValue;
      } else if (solutionValue>upperValue+primalTolerance_) {
	infeasibility=upperValue-solutionValue;
      }
      assert (infeasibility);
      double direction = infeasibility >0 ? 1.0 : -1.0;
      infeasibility *= direction;
      int whichColumn=-1;
      double upperTheta=1.0e30;
      int whichColumn2=-1;
      double upperTheta2=1.0e30;
      double costThru=0.0;
      int nThru=0;
      for (CoinBigIndex j=rowStart[iRow];j<rowEnd[iRow];j++) {
	int iSequence=column[j];
	assert (iSequence>=maximumAbcNumberRows_);
	double dj = abcDj_[iSequence];
	double tableauValue=-elementByRow[j]*direction;
	unsigned char iStatus=internalStatus_[iSequence]&7;
	if ((iStatus&4)==0) {
	  double mult = multiplier[iStatus];
	  double alpha = tableauValue * mult;
	  double oldValue = dj * mult;
	  assert (oldValue>-1.0e-2);
	  double value = oldValue - upperTheta * alpha;
	  if (value < 0.0) {
	    upperTheta2=upperTheta;
	    whichColumn2=whichColumn;
	    costThru=alpha*(abcUpper_[iSequence]-abcLower_[iSequence]);
	    nThru=0;
	    upperTheta = oldValue / alpha;
	    whichColumn=iSequence;
	  } else if (oldValue-upperTheta2*alpha<0.0) {
	    costThru+=alpha*(abcUpper_[iSequence]-abcLower_[iSequence]);
	    index[nThru++]=iSequence;
	  }
	} else if (iStatus<6) {
	  upperTheta=-1.0;
	  upperTheta2=elementByRow[j];
	  whichColumn=iSequence;
	  break;
	} 
      }
      if (whichColumn<0)
	continue;
      if (upperTheta!=-1.0) {
	assert (upperTheta>=0.0);
	if (costThru<infeasibility&&whichColumn2>=0) {
	  index[nThru++]=whichColumn;
	  for (int i=0;i<nThru;i++) {
	    int iSequence=index[i];
	    assert (abcUpper_[iSequence]-abcLower_[iSequence]<1.0e10);
	    // flip and use previous
	    if (getInternalStatus(iSequence)==atLowerBound) {
	      setInternalStatus(iSequence,atUpperBound);
	      solutionSaved_[iSequence]=abcUpper_[iSequence];
	      abcSolution_[iSequence]=abcUpper_[iSequence];
	    } else {
	      setInternalStatus(iSequence,atLowerBound);
	      solutionSaved_[iSequence]=abcLower_[iSequence];
	      abcSolution_[iSequence]=abcLower_[iSequence];
	    }
	  }
	  whichColumn=whichColumn2;
	  upperTheta=upperTheta2;
	}
      } else {
	// free coming in
	upperTheta=(abcDj_[whichColumn]*direction)/upperTheta2;
      }
      // update djs
      upperTheta *= -direction;
      for (CoinBigIndex j=rowStart[iRow];j<rowEnd[iRow];j++) {
	int iSequence=column[j];
	double dj = abcDj_[iSequence];
	double tableauValue=elementByRow[j];
	dj -= upperTheta*tableauValue;
	unsigned char iStatus=internalStatus_[iSequence]&7;
	if ((iStatus&4)==0) {
	  if (!iStatus) {
	    assert (dj>-1.0e-2);
	    dj = CoinMax(dj,0.0);
#ifdef ALLOW_BAD_DJS
	    if (numberBad&&modDj[iSequence]) {
	      double bad=modDj[iSequence];
	      if (dj+bad>=0.0) {
		numberBad--;
		modDj[iSequence]=0.0;
		dj += bad;
	      } else {
		modDj[iSequence]+=dj;
		dj =0.0;
	      }
	    }
#endif
	  } else {
	    assert (dj<1.0e-2);
	    dj = CoinMin(dj,0.0);
#ifdef ALLOW_BAD_DJS
	    if (numberBad&&modDj[iSequence]) {
	      double bad=modDj[iSequence];
	      if (dj+bad<=0.0) {
		numberBad--;
		modDj[iSequence]=0.0;
		dj += bad;
	      } else {
		modDj[iSequence]+=dj;
		dj =0.0;
	      }
	    }
#endif
	  }
	} else if (iStatus<6) {
	  assert (fabs(dj)<1.0e-4);
	  dj = 0.0;
	}
	abcDj_[iSequence]=dj;
      }
      // do basis
      if (direction>0.0) {
	if (upperBasic_[iRow]>lowerBasic_[iRow]) 
	  setInternalStatus(iRow,atLowerBound);
	else
	  setInternalStatus(iRow,isFixed);
	solutionSaved_[iRow]=abcLower_[iRow];
	abcSolution_[iRow]=abcLower_[iRow];
      } else {
	if (upperBasic_[iRow]>lowerBasic_[iRow]) 
	  setInternalStatus(iRow,atUpperBound);
	else
	  setInternalStatus(iRow,isFixed);
	solutionSaved_[iRow]=abcUpper_[iRow];
	abcSolution_[iRow]=abcUpper_[iRow];
      }
      setInternalStatus(whichColumn,basic);
      abcPivotVariable_[iRow]=whichColumn;
      nBasic++;
      // mark rows
      for (CoinBigIndex j=columnStart[whichColumn];j<columnStart[whichColumn+1];j++) {
	int jRow=row[j];
	abcDj_[jRow]=1.0;
      }
    }
#ifdef ALLOW_BAD_DJS
    CoinAbcMemset0(modDj+maximumAbcNumberRows_,numberColumns_);
#endif
    setAvailableArray(iVector);
    setAvailableArray(iVector2);
#if ABC_NORMAL_DEBUG>0
    printf("dual crash put %d in basis\n",nBasic);
#endif
  } else {
    assert ((stateOfProblem_&VALUES_PASS2)!=0);
    // The idea is to put as many likely variables into basis as possible
    int n=0;
    int iVector=getAvailableArray();
    int * index=usefulArray_[iVector].getIndices();
    double * array = usefulArray_[iVector].denseVector();
    int iVector2=getAvailableArray();
    int * index2=usefulArray_[iVector].getIndices();
    for (int iSequence=0;iSequence<numberTotal_;iSequence++) {
      double dj = djSaved_[iSequence];
      double value = solution_[iSequence];
      double lower = abcLower_[iSequence];
      double upper = abcUpper_[iSequence];
      double gapUp=CoinMin(1.0e3,upper-value);
      assert (gapUp>=-1.0e-3);
      gapUp=CoinMax(gapUp,0.0);
      double gapDown=CoinMin(1.0e3,value-lower);
      assert (gapDown>=-1.0e-3);
      gapDown=CoinMax(gapDown,0.0);
      double measure = (CoinMin(gapUp,gapDown)+1.0e-6)/(fabs(dj)+1.0e-6);
      if (gapUp<primalTolerance_*10.0&&dj<dualTolerance_) {
	// set to ub
	setInternalStatus(iSequence,atUpperBound);
	solutionSaved_[iSequence]=abcUpper_[iSequence];
	abcSolution_[iSequence]=abcUpper_[iSequence];
      } else if (gapDown<primalTolerance_*10.0&&dj>-dualTolerance_) {
	// set to lb
	setInternalStatus(iSequence,atLowerBound);
	solutionSaved_[iSequence]=abcLower_[iSequence];
	abcSolution_[iSequence]=abcLower_[iSequence];
      } else if (upper>lower) {
	// set to nearest
	if (gapUp<gapDown) {
	  // set to ub
	  setInternalStatus(iSequence,atUpperBound);
	  solutionSaved_[iSequence]=abcUpper_[iSequence];
	  abcSolution_[iSequence]=abcUpper_[iSequence];
	} else {
	  // set to lb
	  setInternalStatus(iSequence,atLowerBound);
	  solutionSaved_[iSequence]=abcLower_[iSequence];
	  abcSolution_[iSequence]=abcLower_[iSequence];
	}
	array[n]=-measure;
	index[n++]=iSequence;
      }
    }
    // set slacks basic
    memset(internalStatus_,6,numberRows_);
    CoinSort_2(solution_,solution_+n,index);
    CoinAbcMemset0(array,n);
    for (int i=0;i<numberRows_;i++)
      index2[i]=0;
    // first put all possible slacks in
    int n2=0;
    for (int i=0;i<n;i++) {
      int iSequence=index[i];
      if (iSequence<numberRows_) {
	index2[iSequence]=numberRows_;
      } else {
	index[n2++]=iSequence;
      }
    }
    n=n2;
    int numberIn=0;
    const CoinBigIndex * columnStart = abcMatrix_->getPackedMatrix()->getVectorStarts()-
      maximumAbcNumberRows_;
    //const int * columnLength = abcMatrix_->getPackedMatrix()->getVectorLengths();
    const int * row = abcMatrix_->getPackedMatrix()->getIndices();
    if (!abcMatrix_->gotRowCopy())
      abcMatrix_->createRowCopy();
    //const CoinBigIndex * rowStart = abcMatrix_->rowStart();
    //const CoinBigIndex * rowEnd = abcMatrix_->rowEnd();
    for (int i=0;i<n;i++) {
      int iSequence=index[i];
      int bestRow=-1;
      int bestCount=numberRows_+1;
      for (CoinBigIndex j=columnStart[iSequence];j<columnStart[iSequence+1];j++) {
	int iRow=row[j];
    if (!abcMatrix_->gotRowCopy())
      abcMatrix_->createRowCopy();
    const CoinBigIndex * rowStart = abcMatrix_->rowStart();
    const CoinBigIndex * rowEnd = abcMatrix_->rowEnd();
	if (!index2[iRow]) {
	  int length=rowEnd[iRow]-rowStart[iRow];
	  if (length<bestCount) {
	    bestCount=length;
	    bestRow=iRow;
	  }
	}
      }
      if (bestRow>=0) {
	numberIn++;
	for (CoinBigIndex j=columnStart[iSequence];j<columnStart[iSequence+1];j++) {
	  int iRow=row[j];
	  index2[iRow]++;
	}
	setInternalStatus(iSequence,basic);
	abcPivotVariable_[bestRow]=iSequence;
	double dj = djSaved_[bestRow];
	double value = solution_[bestRow];
	double lower = abcLower_[bestRow];
	double upper = abcUpper_[bestRow];
	double gapUp=CoinMax(CoinMin(1.0e3,upper-value),0.0);
	double gapDown=CoinMax(CoinMin(1.0e3,value-lower),0.0);
	//double measure = (CoinMin(gapUp,gapDown)+1.0e-6)/(fabs(dj)+1.0e-6);
	if (gapUp<primalTolerance_*10.0&&dj<dualTolerance_) {
	  // set to ub
	  setInternalStatus(bestRow,atUpperBound);
	  solutionSaved_[bestRow]=abcUpper_[bestRow];
	  abcSolution_[bestRow]=abcUpper_[bestRow];
	} else if (gapDown<primalTolerance_*10.0&&dj>-dualTolerance_) {
	  // set to lb
	  setInternalStatus(bestRow,atLowerBound);
	  solutionSaved_[bestRow]=abcLower_[bestRow];
	  abcSolution_[bestRow]=abcLower_[bestRow];
	} else if (upper>lower) {
	  // set to nearest
	  if (gapUp<gapDown) {
	    // set to ub
	    setInternalStatus(bestRow,atUpperBound);
	    solutionSaved_[bestRow]=abcUpper_[bestRow];
	    abcSolution_[bestRow]=abcUpper_[bestRow];
	  } else {
	    // set to lb
	    setInternalStatus(bestRow,atLowerBound);
	    solutionSaved_[bestRow]=abcLower_[bestRow];
	    abcSolution_[bestRow]=abcLower_[bestRow];
	  }
	}
      }
    }
#if ABC_NORMAL_DEBUG>0
    printf("Idiot crash put %d in basis\n",numberIn);
#endif
    setAvailableArray(iVector);
    setAvailableArray(iVector2);
    delete [] solution_;
    solution_=NULL;
  }
}
/* Puts more stuff in basis
   1 bit set - do even if basis exists
   2 bit set - don't bother staying triangular
*/
void 
AbcSimplex::putStuffInBasis(int type)
{
  int i;
  for (i=0;i<numberRows_;i++) {
    if (getInternalStatus(i)!=basic)
      break;
  }
  if (i<numberRows_&&(type&1)==0)
    return;
  int iVector=getAvailableArray();
  // Column part is mustColumnIn
  int * COIN_RESTRICT mustRowOut = usefulArray_[iVector].getIndices();
  if (!abcMatrix_->gotRowCopy())
    abcMatrix_->createRowCopy();
  const double * elementByRow = abcMatrix_->rowElements();
  const CoinBigIndex * rowStart = abcMatrix_->rowStart();
  const CoinBigIndex * rowEnd = abcMatrix_->rowEnd();
  const int * column = abcMatrix_->rowColumns();
  for (int i=0;i<numberTotal_;i++)
    mustRowOut[i]=-1;
  int nPossible=0;
  // find equality rows with effective nonzero rhs
  for (int iRow=0;iRow<numberRows_;iRow++) {
    if (abcUpper_[iRow]>abcLower_[iRow]||getInternalStatus(iRow)!=basic) {
      mustRowOut[iRow]=-2;
      continue;
    }
    int chooseColumn[2]={-1,-1};
    for (CoinBigIndex j=rowStart[iRow];j<rowEnd[iRow];j++) {
      int iColumn=column[j];
      if (elementByRow[j]>0.0) {
	if (chooseColumn[0]==-1)
	  chooseColumn[0]=iColumn;
	else
	  chooseColumn[0]=-2;
      } else {
	if (chooseColumn[1]==-1)
	  chooseColumn[1]=iColumn;
	else
	  chooseColumn[1]=-2;
      }
    }
    for (int iTry=0;iTry<2;iTry++) {
      int jColumn=chooseColumn[iTry];
      if (jColumn>=0&&getInternalStatus(jColumn)!=basic) {
	// see if has to be basic
	double lowerValue=-abcUpper_[iRow]; // check swap
	double upperValue=-abcLower_[iRow];
	int lowerInf=0;
	int upperInf=0;
	double alpha=0.0;
	for (CoinBigIndex j=rowStart[iRow];j<rowEnd[iRow];j++) {
	  int iColumn=column[j];
	  if (iColumn!=jColumn) {
	    if (abcLower_[iColumn]>-1.0e20)
	      lowerValue -= abcLower_[iColumn]*elementByRow[j];
	    else
	      lowerInf ++;
	    if (abcUpper_[iColumn]<1.0e20)
	      upperValue -= abcUpper_[iColumn]*elementByRow[j];
	    else
	      upperInf ++;
	  } else {
	    alpha=elementByRow[j];
	  }
	}
	// find values column must lie between (signs again)
	if (upperInf)
	  upperValue=COIN_DBL_MAX;
	else
	  upperValue /=alpha; 
	if (lowerInf)
	  lowerValue=-COIN_DBL_MAX;
	else
	  lowerValue /=alpha;
	if (iTry) {
	  // swap
	  double temp=lowerValue;
	  lowerValue=upperValue;
	  upperValue=temp;
	}
	if (lowerValue>abcLower_[jColumn]+10.0*primalTolerance_&&
	    upperValue<abcUpper_[jColumn]-10.0*primalTolerance_) {
	  nPossible++;
	  if (mustRowOut[jColumn]>=0) {
	    // choose one ???
	    //printf("Column %d already selected on row %d now on %d\n",
	    //	   jColumn,mustRowOut[jColumn],iRow);
	    continue;
	  }
	  mustRowOut[jColumn]=iRow;
	  mustRowOut[iRow]=jColumn;
	}
      }
    }
  }
  if (nPossible) {
#if ABC_NORMAL_DEBUG>0
    printf("%d possible candidates\n",nPossible);
#endif
    if ((type&2)==0) {
      // triangular
      int iVector2=getAvailableArray();
      int * COIN_RESTRICT counts = usefulArray_[iVector2].getIndices();
      CoinAbcMemset0(counts,numberRows_);
      for (int iRow=0;iRow<numberRows_;iRow++) {
	int n=0;
	for (CoinBigIndex j=rowStart[iRow];j<rowEnd[iRow];j++) {
	  int iColumn=column[j];
	  if (getInternalStatus(iColumn)==basic)
	    n++;
	}
	counts[iRow]=n;
      }
      const CoinBigIndex * columnStart = abcMatrix_->getPackedMatrix()->getVectorStarts()
	-maximumAbcNumberRows_;
      const int * row = abcMatrix_->getPackedMatrix()->getIndices();
      for (int iRow=0;iRow<numberRows_;iRow++) {
	if (!counts[iRow]) {
	  int iColumn=mustRowOut[iRow];
	  if (iColumn>=0) {
	    setInternalStatus(iRow,isFixed);
	    solutionSaved_[iRow]=abcLower_[iRow];
	    setInternalStatus(iColumn,basic);
	    for (CoinBigIndex j=columnStart[iColumn];j<columnStart[iColumn+1];j++) {
	      int jRow=row[j];
	      counts[jRow]++;
	    }
	  }
	}
      }
      setAvailableArray(iVector2);
    } else {
      // all
      for (int iRow=0;iRow<numberRows_;iRow++) {
	int iColumn=mustRowOut[iRow];
	if (iColumn>=0) {
	  setInternalStatus(iRow,isFixed);
	  solutionSaved_[iRow]=abcLower_[iRow];
	  setInternalStatus(iColumn,basic);
	}
      }
    }
    // redo pivot array
    int numberBasic=0;
    for (int iSequence=0;iSequence<numberTotal_;iSequence++) {
      if (getInternalStatus(iSequence)==basic) 
	abcPivotVariable_[numberBasic++]=iSequence;
    }
    assert (numberBasic==numberRows_);
  }
  setAvailableArray(iVector);
}
// Computes nonbasic cost and total cost
void 
AbcSimplex::computeObjective ()
{
  sumNonBasicCosts_ = 0.0;
  rawObjectiveValue_ = 0.0;
  for (int iSequence = 0; iSequence < numberTotal_; iSequence++) {
    double value = abcSolution_[iSequence]*abcCost_[iSequence];
    rawObjectiveValue_ += value;
    if (getInternalStatus(iSequence)!=basic)
      sumNonBasicCosts_ += value;
  }
  setClpSimplexObjectiveValue();
}
// This sets largest infeasibility and most infeasible
void
AbcSimplex::checkPrimalSolution(bool justBasic)
{
  rawObjectiveValue_ = CoinAbcInnerProduct(costBasic_,numberRows_,solutionBasic_);
  //rawObjectiveValue_ += sumNonBasicCosts_;  
  setClpSimplexObjectiveValue();
  // now look at primal solution
  sumPrimalInfeasibilities_ = 0.0;
  numberPrimalInfeasibilities_ = 0;
  double primalTolerance = primalTolerance_;
  double relaxedTolerance = primalTolerance_;
  // we can't really trust infeasibilities if there is primal error
  double error = CoinMin(1.0e-2, largestPrimalError_);
  // allow tolerance at least slightly bigger than standard
  relaxedTolerance = relaxedTolerance +  error;
  sumOfRelaxedPrimalInfeasibilities_ = 0.0;
  const double *  COIN_RESTRICT lowerBasic = lowerBasic_;
  const double *  COIN_RESTRICT upperBasic = upperBasic_;
  const double *  COIN_RESTRICT solutionBasic = solutionBasic_;
  if (justBasic) {
    for (int iRow = 0; iRow < numberRows_; iRow++) {
      double infeasibility = 0.0;
      if (solutionBasic[iRow] > upperBasic[iRow]) {
	infeasibility = solutionBasic[iRow] - upperBasic[iRow];
      } else if (solutionBasic[iRow] < lowerBasic[iRow]) {
	infeasibility = lowerBasic[iRow] - solutionBasic[iRow];
      }
      if (infeasibility > primalTolerance) {
	sumPrimalInfeasibilities_ += infeasibility - primalTolerance_;
	if (infeasibility > relaxedTolerance)
	  sumOfRelaxedPrimalInfeasibilities_ += infeasibility - relaxedTolerance;
	numberPrimalInfeasibilities_ ++;
      }
    }
  } else {
    for (int iSequence = 0; iSequence < numberTotal_; iSequence++) {
      double infeasibility = 0.0;
      if (abcSolution_[iSequence] > abcUpper_[iSequence]) {
	infeasibility = abcSolution_[iSequence] - abcUpper_[iSequence];
      } else if (abcSolution_[iSequence] < abcLower_[iSequence]) {
	infeasibility = abcLower_[iSequence] - abcSolution_[iSequence];
      }
      if (infeasibility > primalTolerance) {
	//assert (getInternalStatus(iSequence)==basic);
	sumPrimalInfeasibilities_ += infeasibility - primalTolerance_;
	if (infeasibility > relaxedTolerance)
	  sumOfRelaxedPrimalInfeasibilities_ += infeasibility - relaxedTolerance;
	numberPrimalInfeasibilities_ ++;
      }
    }
  }
}
void
AbcSimplex::checkDualSolution()
{
  
  sumDualInfeasibilities_ = 0.0;
  numberDualInfeasibilities_ = 0;
  int firstFreePrimal = -1;
  int firstFreeDual = -1;
  int numberSuperBasicWithDj = 0;
  bestPossibleImprovement_ = 0.0;
  // we can't really trust infeasibilities if there is dual error
  double error = CoinMin(1.0e-2, largestDualError_);
  // allow tolerance at least slightly bigger than standard
  double relaxedTolerance = currentDualTolerance_ +  error;
  // allow bigger tolerance for possible improvement
  double possTolerance = 5.0 * relaxedTolerance;
  sumOfRelaxedDualInfeasibilities_ = 0.0;
  int numberNeedToMove=0;
  for (int iSequence = 0; iSequence < numberTotal_; iSequence++) {
    if (getInternalStatus(iSequence) != basic && !flagged(iSequence)) {
      // not basic
      double distanceUp = abcUpper_[iSequence] - abcSolution_[iSequence];
      double distanceDown = abcSolution_[iSequence] - abcLower_[iSequence];
      double value = abcDj_[iSequence];
      if (distanceUp > primalTolerance_) {
	// Check if "free"
	if (distanceDown <= primalTolerance_) {
	  // should not be negative
	  if (value < 0.0) {
	    value = - value;
	    if (value > currentDualTolerance_) {
	      sumDualInfeasibilities_ += value - currentDualTolerance_;
	      if (value > possTolerance)
		bestPossibleImprovement_ += CoinMin(distanceUp, 1.0e10) * value;
	      if (value > relaxedTolerance)
		sumOfRelaxedDualInfeasibilities_ += value - relaxedTolerance;
	      numberDualInfeasibilities_ ++;
	    }
	  }
	} else {
	  // free
	  value=fabs(value);
	  if (value > 1.0 * relaxedTolerance) {
	    numberSuperBasicWithDj++;
	    if (firstFreeDual < 0)
	      firstFreeDual = iSequence;
	  }
	  if (value > currentDualTolerance_||getInternalStatus(iSequence)!=AbcSimplex::isFree) {
	    numberNeedToMove++;
	    if (firstFreePrimal < 0)
	      firstFreePrimal = iSequence;
	    sumDualInfeasibilities_ += value - currentDualTolerance_;
	    if (value > possTolerance)
	      bestPossibleImprovement_ += CoinMin(distanceUp, 1.0e10) * value;
	    if (value > relaxedTolerance)
	      sumOfRelaxedDualInfeasibilities_ += value - relaxedTolerance;
	    numberDualInfeasibilities_ ++;
	  }
	}
      } else if (distanceDown > primalTolerance_) {
	// should not be positive
	if (value > 0.0) {
	  if (value > currentDualTolerance_) {
	    sumDualInfeasibilities_ += value - currentDualTolerance_;
	    if (value > possTolerance)
	      bestPossibleImprovement_ += value * CoinMin(distanceDown, 1.0e10);
	    if (value > relaxedTolerance)
	      sumOfRelaxedDualInfeasibilities_ += value - relaxedTolerance;
	    numberDualInfeasibilities_ ++;
	  }
	}
      }
    }
  }
  numberDualInfeasibilitiesWithoutFree_ = numberDualInfeasibilities_-numberSuperBasicWithDj;
  if (algorithm_ < 0 && firstFreeDual >= 0) {
    // dual
    firstFree_ = firstFreeDual;
  } else if (numberSuperBasicWithDj||numberNeedToMove) {
    //(abcProgress_.lastIterationNumber(0) <= 0)) {
    firstFree_ = firstFreePrimal;
    if (!sumDualInfeasibilities_)
      sumDualInfeasibilities_=1.0e-8;
  }
}
/* This sets sum and number of infeasibilities (Dual and Primal) */
void
AbcSimplex::checkBothSolutions()
{
    // old way
  checkPrimalSolution(true);
  checkDualSolution();
}
/*
  Unpacks one column of the matrix into indexed array
*/
void 
AbcSimplex::unpack(CoinIndexedVector & rowArray, int sequence) const
{ 
  abcMatrix_->unpack(rowArray,sequence);
}
// Sets row pivot choice algorithm in dual
void
AbcSimplex::setDualRowPivotAlgorithm(AbcDualRowPivot & choice)
{
  delete abcDualRowPivot_;
  abcDualRowPivot_ = choice.clone(true);
  abcDualRowPivot_->setModel(this);
}
// Sets column pivot choice algorithm in primal
void
AbcSimplex::setPrimalColumnPivotAlgorithm(AbcPrimalColumnPivot & choice)
{
  delete abcPrimalColumnPivot_;
  abcPrimalColumnPivot_ = choice.clone(true);
  abcPrimalColumnPivot_->setModel(this);
}
void
AbcSimplex::setFactorization( AbcSimplexFactorization & factorization)
{
  if (abcFactorization_)
    abcFactorization_->setFactorization(factorization);
  else
    abcFactorization_ = new AbcSimplexFactorization(factorization,
					  numberRows_);
}

// Swaps factorization
AbcSimplexFactorization *
AbcSimplex::swapFactorization( AbcSimplexFactorization * factorization)
{
  AbcSimplexFactorization * swap = abcFactorization_;
  abcFactorization_ = factorization;
  return swap;
}

/* Tightens primal bounds to make dual faster.  Unless
   fixed, bounds are slightly looser than they could be.
   This is to make dual go faster and is probably not needed
   with a presolve.  Returns non-zero if problem infeasible
*/
int
AbcSimplex::tightenPrimalBounds()
{
  int tightenType=1;
  if (maximumIterations()>=1000000&&maximumIterations()<1000010)
    tightenType=maximumIterations()-1000000;
  if (!tightenType)
    return 0;
  if (integerType_) {
#if ABC_NORMAL_DEBUG>0
    printf("Redo tighten to relax by 1 for integers (and don't be shocked by infeasibility)\n");
#endif
    return 0;
  }
  // This needs to work on scaled matrix - then replace   
  // Get a row copy in standard format
  CoinPackedMatrix copy;
  copy.setExtraGap(0.0);
  copy.setExtraMajor(0.0);
  copy.reverseOrderedCopyOf(*abcMatrix_->matrix());
  // get matrix data pointers
  const int *  COIN_RESTRICT column = copy.getIndices();
  const CoinBigIndex *  COIN_RESTRICT rowStart = copy.getVectorStarts();
  const int *  COIN_RESTRICT rowLength = copy.getVectorLengths();
  const double *  COIN_RESTRICT element = copy.getElements();
  int numberChanged = 1, iPass = 0;
  double large = largeValue(); // treat bounds > this as infinite
#ifndef NDEBUG
  double large2 = 1.0e10 * large;
#endif
  int numberInfeasible = 0;
  int totalTightened = 0;
  
  double tolerance = primalTolerance();
  
  
  // A copy of bounds is up at top
  translate(0); // move down
  
#define MAXPASS 10
  
  // Loop round seeing if we can tighten bounds
  // Would be faster to have a stack of possible rows
  // and we put altered rows back on stack
  int numberCheck = -1;
  // temp swap signs so can use existing code
  double *  COIN_RESTRICT rowLower=abcLower_;
  double *  COIN_RESTRICT rowUpper=abcUpper_;
  for (int iRow=0;iRow<numberRows_;iRow++) {
    double value=-rowLower[iRow];
    rowLower[iRow]=-rowUpper[iRow];
    rowUpper[iRow]=value;
  }
  // Keep lower and upper for rows
  int whichArray[5];
  for (int i=0;i<5;i++)
    whichArray[i]=getAvailableArray();
  double * COIN_RESTRICT minRhs = usefulArray_[whichArray[0]].denseVector();
  double * COIN_RESTRICT maxRhs = usefulArray_[whichArray[1]].denseVector();
  int * COIN_RESTRICT changedList = usefulArray_[whichArray[0]].getIndices(); 
  int * COIN_RESTRICT changedListNext = usefulArray_[whichArray[1]].getIndices(); 
  char * COIN_RESTRICT changed = reinterpret_cast<char*>(usefulArray_[whichArray[2]].getIndices()); 
  // -1 no infinite, -2 more than one infinite >=0 column
  int * COIN_RESTRICT whichInfiniteDown = usefulArray_[whichArray[3]].getIndices(); 
  int * COIN_RESTRICT whichInfiniteUp = usefulArray_[whichArray[3]].getIndices(); 
  int numberToDoNext=0;
  for (int iRow=0;iRow<numberRows_;iRow++) {
    if (rowLower[iRow] > -large || rowUpper[iRow] < large) {
      changedListNext[numberToDoNext++]=iRow;
      whichInfiniteDown[iRow]=-1;
      whichInfiniteUp[iRow]=-1;
    } else {
      minRhs[iRow]=-COIN_DBL_MAX;
      maxRhs[iRow]=COIN_DBL_MAX;
      whichInfiniteDown[iRow]=-2;
      whichInfiniteUp[iRow]=-2;
    }
  }
  const int * COIN_RESTRICT row = abcMatrix_->matrix()->getIndices();
  const CoinBigIndex * COIN_RESTRICT columnStart = abcMatrix_->matrix()->getVectorStarts();
  const double * COIN_RESTRICT elementByColumn = abcMatrix_->matrix()->getElements();
  
  double *  COIN_RESTRICT columnLower=abcLower_+maximumAbcNumberRows_;
  double *  COIN_RESTRICT columnUpper=abcUpper_+maximumAbcNumberRows_;
  while(numberChanged > numberCheck) {
    int numberToDo=numberToDoNext;
    numberToDoNext=0;
    int * COIN_RESTRICT temp=changedList;
    changedList=changedListNext;
    changedListNext=temp;
    CoinAbcMemset0(changed,numberRows_);
    
    numberChanged = 0; // Bounds tightened this pass
    
    if (iPass == MAXPASS) break;
    iPass++;
    for (int k=0;k<numberToDo;k++) {
      int iRow=changedList[k];
      // possible row
      int infiniteUpper = 0;
      int infiniteLower = 0;
      int firstInfiniteUpper=-1;
      int firstInfiniteLower=-1;
      double maximumUp = 0.0;
      double maximumDown = 0.0;
      double newBound;
      CoinBigIndex rStart = rowStart[iRow];
      CoinBigIndex rEnd = rowStart[iRow] + rowLength[iRow];
      CoinBigIndex j;
      // Compute possible lower and upper ranges
      
      for (j = rStart; j < rEnd; ++j) {
	double value = element[j];
	int iColumn = column[j];
	if (value > 0.0) {
	  if (columnUpper[iColumn] >= large) {
	    firstInfiniteUpper=iColumn;
	    ++infiniteUpper;
	  } else {
	    maximumUp += columnUpper[iColumn] * value;
	  }
	  if (columnLower[iColumn] <= -large) {
	    firstInfiniteLower=iColumn;
	    ++infiniteLower;
	  } else {
	    maximumDown += columnLower[iColumn] * value;
	  }
	} else if (value < 0.0) {
	  if (columnUpper[iColumn] >= large) {
	    firstInfiniteLower=iColumn;
	    ++infiniteLower;
	  } else {
	    maximumDown += columnUpper[iColumn] * value;
	  }
	  if (columnLower[iColumn] <= -large) {
	    firstInfiniteUpper=iColumn;
	    ++infiniteUpper;
	  } else {
	    maximumUp += columnLower[iColumn] * value;
	  }
	}
      }
      // Build in a margin of error
      maximumUp += 1.0e-8 * fabs(maximumUp);
      maximumDown -= 1.0e-8 * fabs(maximumDown);
      double maxUp = maximumUp + infiniteUpper * 1.0e31;
      double maxDown = maximumDown - infiniteLower * 1.0e31;
      minRhs[iRow]=infiniteLower ? -COIN_DBL_MAX : maximumDown;
      maxRhs[iRow]=infiniteUpper ? COIN_DBL_MAX : maximumUp;
      if (infiniteLower==0)
	whichInfiniteDown[iRow]=-1;
      else if (infiniteLower==1)
	whichInfiniteDown[iRow]=firstInfiniteLower;
      else
	whichInfiniteDown[iRow]=-2;
      if (infiniteUpper==0)
	whichInfiniteUp[iRow]=-1;
      else if (infiniteUpper==1)
	whichInfiniteUp[iRow]=firstInfiniteUpper;
      else
	whichInfiniteUp[iRow]=-2;
      if (maxUp <= rowUpper[iRow] + tolerance &&
	  maxDown >= rowLower[iRow] - tolerance) {
	
	// Row is redundant - make totally free
	// NO - can't do this for postsolve
	// rowLower[iRow]=-COIN_DBL_MAX;
	// rowUpper[iRow]=COIN_DBL_MAX;
	//printf("Redundant row in presolveX %d\n",iRow);
	
      } else {
	if (maxUp < rowLower[iRow] - 100.0 * tolerance ||
	    maxDown > rowUpper[iRow] + 100.0 * tolerance) {
	  // problem is infeasible - exit at once
	  numberInfeasible++;
	  break;
	}
	double lower = rowLower[iRow];
	double upper = rowUpper[iRow];
	for (j = rStart; j < rEnd; ++j) {
	  double value = element[j];
	  int iColumn = column[j];
	  double nowLower = columnLower[iColumn];
	  double nowUpper = columnUpper[iColumn];
	  if (value > 0.0) {
	    // positive value
	    if (lower > -large) {
	      if (!infiniteUpper) {
		assert(nowUpper < large2);
		newBound = nowUpper +
		  (lower - maximumUp) / value;
		// relax if original was large
		if (fabs(maximumUp) > 1.0e8)
		  newBound -= 1.0e-12 * fabs(maximumUp);
	      } else if (infiniteUpper == 1 && nowUpper > large) {
		newBound = (lower - maximumUp) / value;
		// relax if original was large
		if (fabs(maximumUp) > 1.0e8)
		  newBound -= 1.0e-12 * fabs(maximumUp);
	      } else {
		newBound = -COIN_DBL_MAX;
	      }
	      if (newBound > nowLower + 1.0e-12 && newBound > -large) {
		// Tighten the lower bound
		numberChanged++;
		// check infeasible (relaxed)
		if (nowUpper < newBound) {
		  if (nowUpper - newBound <
		      -100.0 * tolerance)
		    numberInfeasible++;
		  else
		    newBound = nowUpper;
		}
		columnLower[iColumn] = newBound;
		for (CoinBigIndex j1=columnStart[iColumn];j1<columnStart[iColumn+1];j1++) {
		  int jRow=row[j1];
		  if (!changed[jRow]) {
		    changed[jRow]=1;
		    changedListNext[numberToDoNext++]=jRow;
		  }
		}
		// adjust
		double now;
		if (nowLower < -large) {
		  now = 0.0;
		  infiniteLower--;
		} else {
		  now = nowLower;
		}
		maximumDown += (newBound - now) * value;
		nowLower = newBound;
	      }
	    }
	    if (upper < large) {
	      if (!infiniteLower) {
		assert(nowLower > - large2);
		newBound = nowLower +
		  (upper - maximumDown) / value;
		// relax if original was large
		if (fabs(maximumDown) > 1.0e8)
		  newBound += 1.0e-12 * fabs(maximumDown);
	      } else if (infiniteLower == 1 && nowLower < -large) {
		newBound =   (upper - maximumDown) / value;
		// relax if original was large
		if (fabs(maximumDown) > 1.0e8)
		  newBound += 1.0e-12 * fabs(maximumDown);
	      } else {
		newBound = COIN_DBL_MAX;
	      }
	      if (newBound < nowUpper - 1.0e-12 && newBound < large) {
		// Tighten the upper bound
		numberChanged++;
		// check infeasible (relaxed)
		if (nowLower > newBound) {
		  if (newBound - nowLower <
		      -100.0 * tolerance)
		    numberInfeasible++;
		  else
		    newBound = nowLower;
		}
		columnUpper[iColumn] = newBound;
		for (CoinBigIndex j1=columnStart[iColumn];j1<columnStart[iColumn+1];j1++) {
		  int jRow=row[j1];
		  if (!changed[jRow]) {
		    changed[jRow]=1;
		    changedListNext[numberToDoNext++]=jRow;
		  }
		}
		// adjust
		double now;
		if (nowUpper > large) {
		  now = 0.0;
		  infiniteUpper--;
		} else {
		  now = nowUpper;
		}
		maximumUp += (newBound - now) * value;
		nowUpper = newBound;
	      }
	    }
	  } else {
	    // negative value
	    if (lower > -large) {
	      if (!infiniteUpper) {
		assert(nowLower < large2);
		newBound = nowLower +
		  (lower - maximumUp) / value;
		// relax if original was large
		if (fabs(maximumUp) > 1.0e8)
		  newBound += 1.0e-12 * fabs(maximumUp);
	      } else if (infiniteUpper == 1 && nowLower < -large) {
		newBound = (lower - maximumUp) / value;
		// relax if original was large
		if (fabs(maximumUp) > 1.0e8)
		  newBound += 1.0e-12 * fabs(maximumUp);
	      } else {
		newBound = COIN_DBL_MAX;
	      }
	      if (newBound < nowUpper - 1.0e-12 && newBound < large) {
		// Tighten the upper bound
		numberChanged++;
		// check infeasible (relaxed)
		if (nowLower > newBound) {
		  if (newBound - nowLower <
		      -100.0 * tolerance)
		    numberInfeasible++;
		  else
		    newBound = nowLower;
		}
		columnUpper[iColumn] = newBound;
		for (CoinBigIndex j1=columnStart[iColumn];j1<columnStart[iColumn+1];j1++) {
		  int jRow=row[j1];
		  if (!changed[jRow]) {
		    changed[jRow]=1;
		    changedListNext[numberToDoNext++]=jRow;
		  }
		}
		// adjust
		double now;
		if (nowUpper > large) {
		  now = 0.0;
		  infiniteLower--;
		} else {
		  now = nowUpper;
		}
		maximumDown += (newBound - now) * value;
		nowUpper = newBound;
	      }
	    }
	    if (upper < large) {
	      if (!infiniteLower) {
		assert(nowUpper < large2);
		newBound = nowUpper +
		  (upper - maximumDown) / value;
		// relax if original was large
		if (fabs(maximumDown) > 1.0e8)
		  newBound -= 1.0e-12 * fabs(maximumDown);
	      } else if (infiniteLower == 1 && nowUpper > large) {
		newBound =   (upper - maximumDown) / value;
		// relax if original was large
		if (fabs(maximumDown) > 1.0e8)
		  newBound -= 1.0e-12 * fabs(maximumDown);
	      } else {
		newBound = -COIN_DBL_MAX;
	      }
	      if (newBound > nowLower + 1.0e-12 && newBound > -large) {
		// Tighten the lower bound
		numberChanged++;
		// check infeasible (relaxed)
		if (nowUpper < newBound) {
		  if (nowUpper - newBound <
		      -100.0 * tolerance)
		    numberInfeasible++;
		  else
		    newBound = nowUpper;
		}
		columnLower[iColumn] = newBound;
		for (CoinBigIndex j1=columnStart[iColumn];j1<columnStart[iColumn+1];j1++) {
		  int jRow=row[j1];
		  if (!changed[jRow]) {
		    changed[jRow]=1;
		    changedListNext[numberToDoNext++]=jRow;
		  }
		}
		// adjust
		double now;
		if (nowLower < -large) {
		  now = 0.0;
		  infiniteUpper--;
		} else {
		  now = nowLower;
		}
		maximumUp += (newBound - now) * value;
		nowLower = newBound;
	      }
	    }
	  }
	}
      }
    }
#if 0
    // see if we can tighten doubletons with infinite bounds
    if (iPass==1) {
      const double * COIN_RESTRICT elementByColumn = abcMatrix_->matrix()->getElements();
      for (int iColumn=0;iColumn<numberColumns_;iColumn++) {
	CoinBigIndex start = columnStart[iColumn];
	if (start+2==columnStart[iColumn+1]&&
	  (columnLower[iColumn]<-1.0e30||columnUpper[iColumn])) {
	  int iRow0=row[start];
	  int iRow1=row[start+1];
	  printf("col %d row0 %d el0 %g whichDown0 %d whichUp0 %d row1 %d el1 %g whichDown1 %d whichUp1 %d\n",iColumn,
		 iRow0,elementByColumn[start],whichInfiniteDown[iRow0],whichInfiniteUp[iRow0],
		 iRow1,elementByColumn[start+1],whichInfiniteDown[iRow1],whichInfiniteUp[iRow1]);
	}
      }
    }
#endif
    totalTightened += numberChanged;
    if (iPass == 1)
      numberCheck = numberChanged >> 4;
    if (numberInfeasible) break;
  }
  const double *  COIN_RESTRICT saveLower = abcLower_+maximumNumberTotal_+maximumAbcNumberRows_;
  const double *  COIN_RESTRICT saveUpper = abcUpper_+maximumNumberTotal_+maximumAbcNumberRows_;
  if (!numberInfeasible) {
    handler_->message(CLP_SIMPLEX_BOUNDTIGHTEN, messages_)
      << totalTightened
      << CoinMessageEol;
    int numberLowerChanged=0;
    int numberUpperChanged=0;
    if (!integerType_) {
    // Set bounds slightly loose
    // tighten rows as well
      //#define PRINT_TIGHTEN_PROGRESS 0
    for (int iRow = 0; iRow < numberRows_; iRow++) {
      assert (maxRhs[iRow]>=rowLower[iRow]);
      assert (minRhs[iRow]<=rowUpper[iRow]);
      rowLower[iRow]=CoinMax(rowLower[iRow],minRhs[iRow]);
      rowUpper[iRow]=CoinMin(rowUpper[iRow],maxRhs[iRow]);
#if PRINT_TIGHTEN_PROGRESS>3
      if (handler_->logLevel()>5)
	printf("Row %d min %g max %g lower %g upper %g\n",
	       iRow,minRhs[iRow],maxRhs[iRow],rowLower[iRow],rowUpper[iRow]);
#endif
    }
#if 0
    double useTolerance = 1.0e-4;
    double multiplier = 100.0;
    // To do this we need to compute min and max JUST for no costs?
    const double * COIN_RESTRICT cost = abcCost_+maximumAbcNumberRows_;
    for (int iColumn = 0; iColumn < numberColumns_; iColumn++) {
      if (saveUpper[iColumn] > saveLower[iColumn] + useTolerance) {
	double awayFromLower=0.0;
	double awayFromUpper=0.0;
	//double gap=columnUpper[iColumn]-columnLower[iColumn];
	for (CoinBigIndex j=columnStart[iColumn];j<columnStart[iColumn+1];j++) {
	  int iRow=row[j];
	  double value=elementByColumn[j];
#if PRINT_TIGHTEN_PROGRESS>3
	  if (handler_->logLevel()>5)
	    printf("row %d element %g\n",iRow,value);
#endif
	  if (value>0.0) {
	    if (minRhs[iRow]+value*awayFromLower<rowLower[iRow]) {
	      if (minRhs[iRow]>-1.0e10&&awayFromLower<1.0e10)
		awayFromLower = CoinMax(awayFromLower,(rowLower[iRow]-minRhs[iRow])/value);
	      else
		awayFromLower=COIN_DBL_MAX;
	    }
	    if (maxRhs[iRow]-value*awayFromUpper>rowUpper[iRow]) {
	      if (maxRhs[iRow]<1.0e10&&awayFromUpper<1.0e10)
		awayFromUpper = CoinMax(awayFromUpper,(maxRhs[iRow]-rowUpper[iRow])/value);
	      else
		awayFromUpper=COIN_DBL_MAX;
	    }
	  } else {
	    if (maxRhs[iRow]+value*awayFromLower>rowUpper[iRow]) {
	      if (maxRhs[iRow]<1.0e10&&awayFromLower<1.0e10)
		awayFromLower = CoinMax(awayFromLower,(rowUpper[iRow]-maxRhs[iRow])/value);
	      else
		awayFromLower=COIN_DBL_MAX;
	    }
	    if (minRhs[iRow]-value*awayFromUpper<rowLower[iRow]) {
	      if (minRhs[iRow]>-1.0e10&&awayFromUpper<1.0e10)
		awayFromUpper = CoinMin(awayFromUpper,(minRhs[iRow]-rowLower[iRow])/value);
	      else
		awayFromUpper=COIN_DBL_MAX;
	    }
	  }
	}
	// Might have to go as high as
	double upper = CoinMin(columnLower[iColumn]+awayFromLower,columnUpper[iColumn]);
	// and as low as
	double lower = CoinMax(columnUpper[iColumn]-awayFromUpper,columnLower[iColumn]);
	// but be sensible
	double gap=0.999*(CoinMin(columnUpper[iColumn]-columnLower[iColumn],1.0e10));
	if (awayFromLower>gap||awayFromUpper>gap) {
	  if (fabs(columnUpper[iColumn]-upper)>1.0e-5) {
#if PRINT_TIGHTEN_PROGRESS>1
	    printf("YYchange upper bound on %d from %g (original %g) to %g (awayFromLower %g) - cost %g\n",iColumn,
		   columnUpper[iColumn],saveUpper[iColumn],upper,awayFromLower,cost[iColumn]);
#endif
	    upper=columnUpper[iColumn];
	  }
	  if (fabs(columnLower[iColumn]-lower)>1.0e-5) {
#if PRINT_TIGHTEN_PROGRESS>1
	    printf("YYchange lower bound on %d from %g (original %g) to %g (awayFromUpper %g) - cost %g\n",iColumn,
		   columnLower[iColumn],saveLower[iColumn],lower,awayFromUpper,cost[iColumn]);
#endif
	    lower=columnLower[iColumn];
	  }
	}
	if (lower>upper) {
#if PRINT_TIGHTEN_PROGRESS
	  printf("XXchange upper bound on %d from %g (original %g) to %g (awayFromLower %g) - cost %g\n",iColumn,
		 columnUpper[iColumn],saveUpper[iColumn],upper,awayFromLower,cost[iColumn]);
	  printf("XXchange lower bound on %d from %g (original %g) to %g (awayFromUpper %g) - cost %g\n",iColumn,
		 columnLower[iColumn],saveLower[iColumn],lower,awayFromUpper,cost[iColumn]);
#endif
	  lower=columnLower[iColumn];
	  upper=columnUpper[iColumn];
	}
	//upper=CoinMax(upper,0.0);
	//upper=CoinMax(upper,0.0);
	if (cost[iColumn]>=0.0&&awayFromLower<1.0e10&&columnLower[iColumn]>-1.0e10) {
	  // doesn't want to be too positive
	  if (fabs(columnUpper[iColumn]-upper)>1.0e-5) {
#if PRINT_TIGHTEN_PROGRESS>2
	    if (handler_->logLevel()>1)
	      printf("change upper bound on %d from %g (original %g) to %g (awayFromLower %g) - cost %g\n",iColumn,
		     columnUpper[iColumn],saveUpper[iColumn],upper,awayFromLower,cost[iColumn]);
#endif
	    double difference=columnUpper[iColumn]-upper;
	    for (CoinBigIndex j=columnStart[iColumn];j<columnStart[iColumn+1];j++) {
	      int iRow=row[j];
	      double value=elementByColumn[j];
	      if (value>0.0) {
		assert (difference*value>=0.0);
		maxRhs[iRow]-=difference*value;
	      } else {
		assert (difference*value<=0.0);
		minRhs[iRow]-=difference*value;
	      }
	    }
	    columnUpper[iColumn]=upper;
	    numberUpperChanged++;
	  }
	}
	if (cost[iColumn]<=0.0&&awayFromUpper<1.0e10&&columnUpper[iColumn]<1.0e10) {
	  // doesn't want to be too negative
	  if (fabs(columnLower[iColumn]-lower)>1.0e-5) {
#if PRINT_TIGHTEN_PROGRESS>2
	    if (handler_->logLevel()>1)
	      printf("change lower bound on %d from %g (original %g) to %g (awayFromUpper %g) - cost %g\n",iColumn,
		     columnLower[iColumn],saveLower[iColumn],lower,awayFromUpper,cost[iColumn]);
#endif
	    double difference=columnLower[iColumn]-lower;
	    for (CoinBigIndex j=columnStart[iColumn];j<columnStart[iColumn+1];j++) {
	      int iRow=row[j];
	      double value=elementByColumn[j];
	      if (value>0.0) {
		assert (difference*value<=0.0);
		minRhs[iRow]-=difference*value;
	      } else {
		assert (difference*value>=0.0);
		maxRhs[iRow]-=difference*value;
	      }
	    }
	    columnLower[iColumn]=lower;
	    numberLowerChanged++;
	  }
	}
	// Make large bounds stay infinite
	if (saveUpper[iColumn] > 1.0e30 && columnUpper[iColumn] > 1.0e10) {
	  columnUpper[iColumn] = COIN_DBL_MAX;
	}
	if (saveLower[iColumn] < -1.0e30 && columnLower[iColumn] < -1.0e10) {
	  columnLower[iColumn] = -COIN_DBL_MAX;
	}
	if (columnUpper[iColumn] - columnLower[iColumn] < useTolerance + 1.0e-8) {
	  // relax enough so will have correct dj
	  columnLower[iColumn] = CoinMax(saveLower[iColumn],
					  columnLower[iColumn] - multiplier * useTolerance);
	  columnUpper[iColumn] = CoinMin(saveUpper[iColumn],
					  columnUpper[iColumn] + multiplier * useTolerance);
	} else {
	  if (columnUpper[iColumn] < saveUpper[iColumn]) {
	    // relax a bit
	    columnUpper[iColumn] = CoinMin(columnUpper[iColumn] + multiplier * useTolerance,
					    saveUpper[iColumn]);
	  }
	  if (columnLower[iColumn] > saveLower[iColumn]) {
	    // relax a bit
	    columnLower[iColumn] = CoinMax(columnLower[iColumn] - multiplier * useTolerance,
					    saveLower[iColumn]);
	  }
	}
	if (0) {
	  // debug
	  // tighten rows as well
	  for (int iRow = 0; iRow < numberRows_; iRow++) {
	    if (rowLower[iRow] > -large || rowUpper[iRow] < large) {
	      // possible row
	      int infiniteUpper = 0;
	      int infiniteLower = 0;
	      double maximumUp = 0.0;
	      double maximumDown = 0.0;
	      CoinBigIndex rStart = rowStart[iRow];
	      CoinBigIndex rEnd = rowStart[iRow] + rowLength[iRow];
	      CoinBigIndex j;
	      // Compute possible lower and upper ranges
	      for (j = rStart; j < rEnd; ++j) {
		double value = element[j];
		int iColumn = column[j];
		if (value > 0.0) {
		  if (columnUpper[iColumn] >= large) {
		    ++infiniteUpper;
		  } else {
		    maximumUp += columnUpper[iColumn] * value;
		  }
		  if (columnLower[iColumn] <= -large) {
		    ++infiniteLower;
		  } else {
		    maximumDown += columnLower[iColumn] * value;
		  }
		} else if (value < 0.0) {
		  if (columnUpper[iColumn] >= large) {
		    ++infiniteLower;
		  } else {
		    maximumDown += columnUpper[iColumn] * value;
		  }
		  if (columnLower[iColumn] <= -large) {
		    ++infiniteUpper;
		  } else {
		    maximumUp += columnLower[iColumn] * value;
		  }
		}
	      }
	      // Build in a margin of error
	      double maxUp = maximumUp+1.0e-8 * fabs(maximumUp);
	      double maxDown = maximumDown-1.0e-8 * fabs(maximumDown);
	      double rLower=rowLower[iRow];
	      double rUpper=rowUpper[iRow];
	      if (!infiniteUpper&&maxUp < rUpper - tolerance) {
		if (rUpper!=COIN_DBL_MAX||maximumUp<1.0e5) 
		  rUpper=maximumUp;
	      }
	      if (!infiniteLower&&maxDown > rLower + tolerance) {
		if (rLower!=-COIN_DBL_MAX||maximumDown>-1.0e5) 
		  rLower=maximumDown;
	      }
	      if (infiniteUpper)
		maxUp=COIN_DBL_MAX;
	      if (fabs(maxUp-maxRhs[iRow])>1.0e-3*(1.0+fabs(maxUp)))
		printf("bad Maxup row %d maxUp %g maxRhs %g\n",iRow,maxUp,maxRhs[iRow]);
	      maxRhs[iRow]=maxUp;
	      if (infiniteLower)
		maxDown=-COIN_DBL_MAX;
	      if (fabs(maxDown-minRhs[iRow])>1.0e-3*(1.0+fabs(maxDown)))
		printf("bad Maxdown row %d maxDown %g minRhs %g\n",iRow,maxDown,minRhs[iRow]);
	      minRhs[iRow]=maxDown;
	      assert(rLower<=rUpper);
	    }
	  }
	    // end debug
	}
      }
    }
    if (tightenType>1) {
      // relax
      for (int iColumn = 0; iColumn < numberColumns_; iColumn++) {
	// relax enough so will have correct dj
	double lower=saveLower[iColumn];
	double upper=saveUpper[iColumn];
	columnLower[iColumn] = CoinMax(lower,columnLower[iColumn] - multiplier);
	columnUpper[iColumn] = CoinMin(upper,columnUpper[iColumn] + multiplier);
      }
    }
#endif
    } else {
#if ABC_NORMAL_DEBUG>0
      printf("Redo tighten to relax by 1 for integers\n");
#endif
    }
#if ABC_NORMAL_DEBUG>0
    printf("Tighten - phase1 %d bounds changed, phase2 %d lower, %d upper\n",
	   totalTightened,numberLowerChanged,numberUpperChanged);
#endif
    if (integerType_) {
      const double * columnScale=scaleToExternal_+maximumAbcNumberRows_;
      const double * inverseColumnScale=scaleFromExternal_+maximumAbcNumberRows_;
      for (int iColumn = 0; iColumn < numberColumns_; iColumn++) {
	if (integerType_[iColumn]) {
	  double value;
	  double lower = columnLower[iColumn]*inverseColumnScale[iColumn];
	  double upper = columnUpper[iColumn]*inverseColumnScale[iColumn];
	  value = floor(lower + 0.5);
	  if (fabs(value - lower) > primalTolerance_)
	    value = ceil(lower);
	  columnLower_[iColumn] = value;
	  columnLower[iColumn]=value*columnScale[iColumn];
	  value = floor(upper + 0.5);
	  if (fabs(value - upper) > primalTolerance_)
	    value = floor(upper);
	  columnUpper_[iColumn] = value;
	  columnUpper[iColumn]=value*columnScale[iColumn];
	  if (columnLower_[iColumn] > columnUpper_[iColumn])
	    numberInfeasible++;
	}
      }
      if (numberInfeasible) {
	handler_->message(CLP_SIMPLEX_INFEASIBILITIES, messages_)
	  << numberInfeasible
	  << CoinMessageEol;
	// restore column bounds
	CoinMemcpyN(saveLower, numberColumns_, columnLower_);
	CoinMemcpyN(saveUpper, numberColumns_, columnUpper_);
      }
    }
  } else {
    handler_->message(CLP_SIMPLEX_INFEASIBILITIES, messages_)
      << numberInfeasible
      << CoinMessageEol;
    // restore column bounds
    CoinMemcpyN(saveLower, numberColumns_, columnLower_);
    CoinMemcpyN(saveUpper, numberColumns_, columnUpper_);
  }
  if (!numberInfeasible) {
    // tighten rows as well
    for (int iRow = 0; iRow < numberRows_; iRow++) {
      if (rowLower[iRow] > -large || rowUpper[iRow] < large) {
	// possible row
	int infiniteUpper = 0;
	int infiniteLower = 0;
	double maximumUp = 0.0;
	double maximumDown = 0.0;
	CoinBigIndex rStart = rowStart[iRow];
	CoinBigIndex rEnd = rowStart[iRow] + rowLength[iRow];
	CoinBigIndex j;
	// Compute possible lower and upper ranges
	for (j = rStart; j < rEnd; ++j) {
	  double value = element[j];
	  int iColumn = column[j];
	  if (value > 0.0) {
	    if (columnUpper[iColumn] >= large) {
	      ++infiniteUpper;
	    } else {
	      maximumUp += columnUpper[iColumn] * value;
	    }
	    if (columnLower[iColumn] <= -large) {
	      ++infiniteLower;
	    } else {
	      maximumDown += columnLower[iColumn] * value;
	    }
	  } else if (value < 0.0) {
	    if (columnUpper[iColumn] >= large) {
	      ++infiniteLower;
	    } else {
	      maximumDown += columnUpper[iColumn] * value;
	    }
	    if (columnLower[iColumn] <= -large) {
	      ++infiniteUpper;
	    } else {
	      maximumUp += columnLower[iColumn] * value;
	    }
	  }
	}
	// Build in a margin of error
	double maxUp = maximumUp+1.0e-8 * fabs(maximumUp);
	double maxDown = maximumDown-1.0e-8 * fabs(maximumDown);
	if (!infiniteUpper&&maxUp < rowUpper[iRow] - tolerance) {
	  if (rowUpper[iRow]!=COIN_DBL_MAX||maximumUp<1.0e5) 
	    rowUpper[iRow]=maximumUp;
	}
	if (!infiniteLower&&maxDown > rowLower[iRow] + tolerance) {
	  if (rowLower[iRow]!=-COIN_DBL_MAX||maximumDown>-1.0e5) 
	    rowLower[iRow]=maximumDown;
	}
	assert(rowLower[iRow]<=rowUpper[iRow]);
	if(rowUpper[iRow]-rowLower[iRow]<1.0e-12) {
	  if (fabs(rowLower[iRow])>fabs(rowUpper[iRow]))
	    rowLower[iRow]=rowUpper[iRow];
	  else
	    rowUpper[iRow]=rowLower[iRow];
	}
      }
    }
    // flip back
    for (int iRow=0;iRow<numberRows_;iRow++) {
      double value=-rowLower[iRow];
      rowLower[iRow]=-rowUpper[iRow];
      rowUpper[iRow]=value;
    }
    // move back - relaxed unless integer
    if (!integerType_) {
      double * COIN_RESTRICT lowerSaved = abcLower_+maximumNumberTotal_; 
      double * COIN_RESTRICT upperSaved = abcUpper_+maximumNumberTotal_; 
      double tolerance=2.0*primalTolerance_;
      for (int i=0;i<numberTotal_;i++) {
	double lower=abcLower_[i];
	double upper=abcUpper_[i];
	if (lower!=upper) {
	  lower=CoinMax(lower-tolerance,lowerSaved[i]);
	  upper=CoinMin(upper+tolerance,upperSaved[i]);
	}
	abcLower_[i]=lower;
	lowerSaved[i]=lower;
	abcUpper_[i]=upper;
	upperSaved[i]=upper;
      }
    } else {
      CoinAbcMemcpy(abcLower_+maximumNumberTotal_,abcLower_,numberTotal_);
      CoinAbcMemcpy(abcUpper_+maximumNumberTotal_,abcUpper_,numberTotal_);
    }
#if 0
    for (int i=0;i<numberTotal_;i++) {
      if (abcSolution_[i+maximumNumberTotal_]>abcUpper_[i+maximumNumberTotal_]+1.0e-5)
	printf ("above %d %g %g\n",i,
		abcSolution_[i+maximumNumberTotal_],abcUpper_[i+maximumNumberTotal_]);
      if (abcSolution_[i+maximumNumberTotal_]<abcLower_[i+maximumNumberTotal_]-1.0e-5)
	printf ("below %d %g %g\n",i,
		abcSolution_[i+maximumNumberTotal_],abcLower_[i+maximumNumberTotal_]);
    }
#endif
    permuteBasis();
    // move solution
    CoinAbcMemcpy(abcSolution_,solutionSaved_,numberTotal_);
  }
  CoinAbcMemset0(minRhs,numberRows_);
  CoinAbcMemset0(maxRhs,numberRows_);
  for (int i=0;i<5;i++)
    setAvailableArray(whichArray[i]);
  return (numberInfeasible);
}
// dual
#include "AbcSimplexDual.hpp"
#ifdef ABC_INHERIT
// Returns 0 if dual can be skipped
int 
ClpSimplex::doAbcDual()
{
  if ((abcState_&CLP_ABC_WANTED)==0) {
    return 1; // not wanted
  } else {
    assert (abcSimplex_);
    int crashState=(abcState_>>8)&7;
    if (crashState&&crashState<4) {
      switch (crashState) {
      case 1:
	crash(1000.0,1);
	break;
      case 2:
	crash(abcSimplex_->dualBound(),0);
	break;
      case 3:
	crash(0.0,3);
	break;
      }
    }
    // this and abcSimplex_ are approximately same pointer
    if ((abcState_&CLP_ABC_FULL_DONE)==0) {
      abcSimplex_->gutsOfResize(numberRows_,numberColumns_);
      abcSimplex_->translate(DO_SCALE_AND_MATRIX|DO_BASIS_AND_ORDER|DO_STATUS|DO_SOLUTION);
      //abcSimplex_->permuteIn();
      int maximumPivotsAbc=CoinMin(abcSimplex_->factorization()->maximumPivots(),numberColumns_+200);
      abcSimplex_->factorization()->maximumPivots(maximumPivotsAbc);
      if (numberColumns_)
	abcSimplex_->tightenPrimalBounds();
    }
    if ((abcSimplex_->stateOfProblem()&VALUES_PASS2)!=0) {
      if (solution_)
	crashState=6;
    }
    if (crashState&&crashState>3) {
      abcSimplex_->crash(crashState-2);
    }
    if (crashState&&crashState<4) {
      abcSimplex_->putStuffInBasis(1+2);
    }
    int returnCode = abcSimplex_->doAbcDual();
    //set to force crossover test returnCode=1;
    abcSimplex_->permuteOut(ROW_PRIMAL_OK|ROW_DUAL_OK|COLUMN_PRIMAL_OK|COLUMN_DUAL_OK|ALL_STATUS_OK);
    if (returnCode) {
      // fix as this model is all messed up e.g. scaling
      scalingFlag_ = abs(scalingFlag_);
      //delete [] rowScale_;
      //delete [] columnScale_;
      rowScale_ = NULL;
      columnScale_ = NULL;
#if 0
      delete [] savedRowScale_;
      delete [] savedColumnScale_;
      savedRowScale_ = NULL;
      savedColumnScale_ = NULL;
      inverseRowScale_ = NULL;
      inverseColumnScale_ = NULL;
#endif
      if (perturbation_==50)
	perturbation_=51;
      //perturbation_=100;
#if ABC_NORMAL_DEBUG>0
      printf("Bad exit with status of %d\n",problemStatus_);
#endif
      //problemStatus_=10;
      int status=problemStatus_;
      //problemStatus_=-1;
      if (status!=10) {
	dual(); // Do ClpSimplexDual
     } else {
	moreSpecialOptions_ |= 256;
	primal(1);
      }
    } else {
      // double check objective
      //printf("%g %g\n",objectiveValue(),abcSimplex_->objectiveValue());
      perturbation_=100;
      moreSpecialOptions_ |= 256;
      //primal(1);
    }
    return returnCode;
  }
}
#endif
#include "AbcSimplexPrimal.hpp"
// Do dual (return 1 if cleanup needed)
int 
AbcSimplex::doAbcDual()
{
  if (objective_) {
    objective_->setActivated(0);
  } else {
    // create dummy stuff
    assert (!numberColumns_);
    if (!numberRows_)
      problemStatus_ = 0; // say optimal
    return 0;
  }
  /*  Note use of "down casting".  The only class the user sees is AbcSimplex.
      Classes AbcSimplexDual, AbcSimplexPrimal, (AbcSimplexNonlinear)
      and AbcSimplexOther all exist and inherit from AbcSimplex but have no
      additional data and have no destructor or (non-default) constructor.
      
      This is to stop classes becoming too unwieldy and so I (JJHF) can use e.g. "perturb"
      in primal and dual.
      
      As far as I can see this is perfectly safe.
  */
  algorithm_=-1;
  baseIteration_=numberIterations_;
  if (!abcMatrix_->gotRowCopy())
    abcMatrix_->createRowCopy();
#ifdef EARLY_FACTORIZE
  if (maximumIterations()>1000000&&maximumIterations()<1000999) {
    numberEarly_=maximumIterations()-1000000;
#if ABC_NORMAL_DEBUG>0
    printf("Setting numberEarly_ to %d\n",numberEarly_);
#endif
    numberEarly_+=numberEarly_<<16;
  }
#endif
  minimumThetaMovement_=1.0e-10;
  if (fabs(infeasibilityCost_-1.0e10)<1.1e9
      &&fabs(infeasibilityCost_-1.0e10)>1.0e5&&false) {
    minimumThetaMovement_=1.0e-3/fabs(infeasibilityCost_-1.0e10);
    printf("trying minimum theta movement of %g\n",minimumThetaMovement_);
    infeasibilityCost_=1.0e10;
  }
  int savePerturbation=perturbation_;
  static_cast<AbcSimplexDual *> (this)->dual();
  minimumThetaMovement_=1.0e-10;
  if ((specialOptions_&2048)!=0 && problemStatus_==10 && !numberPrimalInfeasibilities_
      && sumDualInfeasibilities_ < 1000.0* dualTolerance_)
    problemStatus_ = 0; // ignore
  onStopped(); // set secondary status if stopped
  if (problemStatus_==1&&numberFlagged_) {
    problemStatus_=10;
    static_cast<AbcSimplexPrimal *> (this)->unflag();
  }
  if (perturbation_<101) {
    //printf("Perturbed djs?\n");
    double * save=abcCost_;
    abcCost_=costSaved_;
    computeInternalObjectiveValue();
    abcCost_=save;
  }
  int totalNumberIterations=numberIterations_;
  if (problemStatus_ == 10 && (moreSpecialOptions_&32768)!=0 &&sumDualInfeasibilities_ < 0.1) {
    problemStatus_=0;
  }
#ifndef TRY_ABC_GUS
  if (problemStatus_==10) {
    int savePerturbation=perturbation_;
    perturbation_=100;
    copyFromSaved(2);
    minimumThetaMovement_=1.0e-12;
    baseIteration_=numberIterations_;
    static_cast<AbcSimplexPrimal *> (this)->primal(0);
    totalNumberIterations+=numberIterations_-baseIteration_;
    perturbation_=savePerturbation;
  }
#else
  int iPass=0;
  while (problemStatus_==10&&minimumThetaMovement_>0.99999e-12) {
    iPass++;
    if (initialSumInfeasibilities_>=0.0) {
      if (savePerturbation>=100) {
	perturbation_=100;
      } else {
	if (savePerturbation==50)
	  perturbation_=CoinMax(51,HEAVY_PERTURBATION-4-iPass); //smaller
	else
	  perturbation_=CoinMax(51,savePerturbation-1-iPass); //smaller
      }
    } else {
      perturbation_=savePerturbation;
      abcFactorization_->setPivots(100000); // force factorization
      initialSumInfeasibilities_=1.23456789e-5;
      // clean pivots
      int numberBasic=0;
      int * pivot=pivotVariable();
      for (int i=0;i<numberTotal_;i++) {
	if (getInternalStatus(i)==basic)
	  pivot[numberBasic++]=i;
      }
      assert (numberBasic==numberRows_);
    }
    copyFromSaved(14);
    // Say second call
    if (iPass>3)
      moreSpecialOptions_ |= 256;
    if (iPass>2)
      perturbation_=101;
    baseIteration_=numberIterations_;
    static_cast<AbcSimplexPrimal *> (this)->primal(0);
    totalNumberIterations+=numberIterations_-baseIteration_;
    if (problemStatus_==10) {
      // reduce 
      minimumThetaMovement_*=1.0e-1;
      copyFromSaved(14);
      baseIteration_=numberIterations_;
      static_cast<AbcSimplexDual *> (this)->dual();
      totalNumberIterations+=numberIterations_-baseIteration_;
    }
    perturbation_=savePerturbation;
  }
#endif
  numberIterations_=totalNumberIterations;
  return (problemStatus_<0||problemStatus_>3) ? 1 : 0;
}
int AbcSimplex::dual ()
{
  if (!abcMatrix_->gotRowCopy())
    abcMatrix_->createRowCopy();
  assert (!numberFlagged_);
  baseIteration_=numberIterations_;
  //double savedPivotTolerance = abcFactorization_->pivotTolerance();
  if (objective_) {
    objective_->setActivated(0);
  } else {
    // create dummy stuff
    assert (!numberColumns_);
    if (!numberRows_)
      problemStatus_ = 0; // say optimal
    return 0;
  }
  /*  Note use of "down casting".  The only class the user sees is AbcSimplex.
      Classes AbcSimplexDual, AbcSimplexPrimal, (AbcSimplexNonlinear)
      and AbcSimplexOther all exist and inherit from AbcSimplex but have no
      additional data and have no destructor or (non-default) constructor.
      
      This is to stop classes becoming too unwieldy and so I (JJF) can use e.g. "perturb"
      in primal and dual.
      
      As far as I can see this is perfectly safe.
  */
  minimumThetaMovement_=1.0e-12;
  int returnCode = static_cast<AbcSimplexDual *> (this)->dual();
  if ((specialOptions_ & 2048) != 0 && problemStatus_ == 10 && !numberPrimalInfeasibilities_
      && sumDualInfeasibilities_ < 1000.0 * dualTolerance_ && perturbation_ >= 100)
    problemStatus_ = 0; // ignore
  if (problemStatus_ == 10) {
#if 1 //ABC_NORMAL_DEBUG>0
    printf("return and use ClpSimplexPrimal\n");
    abort();
#else
    //printf("Cleaning up with primal\n");
    //lastAlgorithm=1;
    int savePerturbation = perturbation_;
    int saveLog = handler_->logLevel();
    //handler_->setLogLevel(63);
    perturbation_ = 101;
    bool denseFactorization = initialDenseFactorization();
    // It will be safe to allow dense
    setInitialDenseFactorization(true);
    // Allow for catastrophe
    int saveMax = intParam_[ClpMaxNumIteration];
    if (numberIterations_) {
      // normal
      if (intParam_[ClpMaxNumIteration] > 100000 + numberIterations_)
	intParam_[ClpMaxNumIteration]
	  = numberIterations_ + 1000 + 2 * numberRows_ + numberColumns_;
    } else {
      // Not normal allow more
      baseIteration_ += 2 * (numberRows_ + numberColumns_);
    }
    // check which algorithms allowed
    int dummy;
    if (problemStatus_ == 10 && saveObjective == objective_)
      startFinishOptions |= 2;
    baseIteration_ = numberIterations_;
    // Say second call
    moreSpecialOptions_ |= 256;
    minimumThetaMovement_=1.0e-12;
    returnCode = static_cast<AbcSimplexPrimal *> (this)->primal(1, startFinishOptions);
    // Say not second call
    moreSpecialOptions_ &= ~256;
    baseIteration_ = 0;
    if (saveObjective != objective_) {
      // We changed objective to see if infeasible
      delete objective_;
      objective_ = saveObjective;
      if (!problemStatus_) {
	// carry on
	returnCode = static_cast<AbcSimplexPrimal *> (this)->primal(1, startFinishOptions);
      }
    }
    if (problemStatus_ == 3 && numberIterations_ < saveMax) {
      // flatten solution and try again
      for (int iSequence = 0; iSequence < numberTotal_; iSequence++) {
	if (getInternalStatus(iSequence) != basic) {
	  setInternalStatus(iSequence, superBasic);
	  // but put to bound if close
	  if (fabs(rowActivity_[iSequence] - rowLower_[iSequence])
	      <= primalTolerance_) {
	    abcSolution_[iSequence] = abcLower_[iSequence];
	    setInternalStatus(iSequence, atLowerBound);
	  } else if (fabs(rowActivity_[iSequence] - rowUpper_[iSequence])
		     <= primalTolerance_) {
	    abcSolution_[iSequence] = abcUpper_[iSequence];
	    setInternalStatus(iSequence, atUpperBound);
	  }
	}
      }
      problemStatus_ = -1;
      intParam_[ClpMaxNumIteration] = CoinMin(numberIterations_ + 1000 +
					      2 * numberRows_ + numberColumns_, saveMax);
      perturbation_ = savePerturbation;
      baseIteration_ = numberIterations_;
      // Say second call
      moreSpecialOptions_ |= 256;
      returnCode = static_cast<AbcSimplexPrimal *> (this)->primal(0, startFinishOptions);
      // Say not second call
      moreSpecialOptions_ &= ~256;
      baseIteration_ = 0;
      computeObjectiveValue();
      // can't rely on djs either
      memset(reducedCost_, 0, numberColumns_ * sizeof(double));
    }
    intParam_[ClpMaxNumIteration] = saveMax;
    
    setInitialDenseFactorization(denseFactorization);
    perturbation_ = savePerturbation;
    if (problemStatus_ == 10) {
      if (!numberPrimalInfeasibilities_)
	problemStatus_ = 0;
      else
	problemStatus_ = 4;
    }
    handler_->setLogLevel(saveLog);
#endif
  }
  onStopped(); // set secondary status if stopped
  return returnCode;
}
#ifdef ABC_INHERIT
// primal
// Returns 0 if primal can be skipped
int 
ClpSimplex::doAbcPrimal(int ifValuesPass)
{
  if ((abcState_&CLP_ABC_WANTED)==0) {
    return 1; // not wanted
  } else {
    assert (abcSimplex_);
    if (ifValuesPass) 
      abcSimplex_->setStateOfProblem(abcSimplex_->stateOfProblem()|VALUES_PASS);
    int crashState=(abcState_>>8)&7;
    if (crashState&&crashState<4) {
      switch (crashState) {
      case 1:
	crash(1000.0,1);
	break;
      case 2:
	crash(abcSimplex_->dualBound(),0);
	break;
      case 3:
	crash(0.0,3);
	break;
      }
    }
    // this and abcSimplex_ are approximately same pointer
    if ((abcState_&CLP_ABC_FULL_DONE)==0) {
      abcSimplex_->gutsOfResize(numberRows_,numberColumns_);
      abcSimplex_->translate(DO_SCALE_AND_MATRIX|DO_BASIS_AND_ORDER|DO_STATUS|DO_SOLUTION);
      //abcSimplex_->permuteIn();
      int maximumPivotsAbc=CoinMin(abcSimplex_->factorization()->maximumPivots(),numberColumns_+200);
      abcSimplex_->factorization()->maximumPivots(maximumPivotsAbc);
      abcSimplex_->copyFromSaved(1);
    }
    if ((abcSimplex_->stateOfProblem()&VALUES_PASS2)!=0) {
      if (solution_)
	crashState=6;
    }
    if (crashState&&crashState>3) {
      abcSimplex_->crash(crashState-2);
    }
    if (crashState&&crashState<4) {
      abcSimplex_->putStuffInBasis(1+2);
    }
    int returnCode = abcSimplex_->doAbcPrimal(ifValuesPass);
    //set to force crossover test returnCode=1;
    abcSimplex_->permuteOut(ROW_PRIMAL_OK|ROW_DUAL_OK|COLUMN_PRIMAL_OK|COLUMN_DUAL_OK|ALL_STATUS_OK);
    if (returnCode) {
      // fix as this model is all messed up e.g. scaling
     scalingFlag_ = abs(scalingFlag_);
     //delete [] rowScale_;
     //delete [] columnScale_;
     rowScale_ = NULL;
     columnScale_ = NULL;
#if 0
     delete [] savedRowScale_;
     delete [] savedColumnScale_;
     savedRowScale_ = NULL;
     savedColumnScale_ = NULL;
     inverseRowScale_ = NULL;
     inverseColumnScale_ = NULL;
#endif
     if (perturbation_==50)
       perturbation_=51;
     //perturbation_=100;
#if ABC_NORMAL_DEBUG>0
     if (problemStatus_)
       printf("Bad exit with status of %d\n",problemStatus_);
#endif
     //problemStatus_=10;
     int status=problemStatus_;
     // should not be using
     for (int i = 0; i < 6; i++) {
       if (rowArray_[i])
	 rowArray_[i]->clear();
       if (columnArray_[i])
	 columnArray_[i]->clear();
     }
     //problemStatus_=-1;
     if (status!=3&&status!=0) {
       if (status!=10) {
	 primal(); // Do ClpSimplexPrimal
       } else {
	 moreSpecialOptions_ |= 256;
	 dual();
       }
     }
    } else {
      // double check objective
      //printf("%g %g\n",objectiveValue(),abcSimplex_->objectiveValue());
    }
    numberIterations_=abcSimplex_->numberIterations();
    return returnCode;
  }
}
#endif
// Do primal (return 1 if cleanup needed)
int 
AbcSimplex::doAbcPrimal(int ifValuesPass)
{
  if (objective_) {
    objective_->setActivated(0);
  } else {
    // create dummy stuff
    assert (!numberColumns_);
    if (!numberRows_)
      problemStatus_ = 0; // say optimal
    return 0;
  }
  /*  Note use of "down casting".  The only class the user sees is AbcSimplex.
      Classes AbcSimplexDual, AbcSimplexPrimal, (AbcSimplexNonlinear)
      and AbcSimplexOther all exist and inherit from AbcSimplex but have no
      additional data and have no destructor or (non-default) constructor.
      
      This is to stop classes becoming too unwieldy and so I (JJHF) can use e.g. "perturb"
      in primal and dual.
      
      As far as I can see this is perfectly safe.
  */
  algorithm_=-1;
  int savePerturbation=perturbation_;
  baseIteration_=numberIterations_;
  if (!abcMatrix_->gotRowCopy())
    abcMatrix_->createRowCopy();
#ifdef EARLY_FACTORIZE
  if (maximumIterations()>1000000&&maximumIterations()<1000999) {
    numberEarly_=maximumIterations()-1000000;
#if ABC_NORMAL_DEBUG>0
    printf("Setting numberEarly_ to %d\n",numberEarly_);
#endif
    numberEarly_+=numberEarly_<<16;
  }
#endif
  // add values pass options
  minimumThetaMovement_=1.0e-12;
  if ((specialOptions_&8192)!=0)
    minimumThetaMovement_=1.0e-15;
  int returnCode=static_cast<AbcSimplexPrimal *> (this)->primal(ifValuesPass);
  stateOfProblem_ &= ~VALUES_PASS;
#ifndef TRY_ABC_GUS
  if (returnCode==10) {
    copyFromSaved(14);
    int savePerturbation=perturbation_;
    perturbation_=51;
    minimumThetaMovement_=1.0e-12;
    returnCode=static_cast<AbcSimplexDual *> (this)->dual();
    perturbation_=savePerturbation;
  }
#else
  minimumThetaMovement_=1.0e-12;
  int iPass=0;
  while (problemStatus_==10&&minimumThetaMovement_>1.0e-15) {
    iPass++;
    if (minimumThetaMovement_==1.0e-12)
      perturbation_=CoinMin(savePerturbation,55);
    else
      perturbation_=100;
    copyFromSaved(14);
    // reduce ?
    // Say second call
    // Say second call
    if (iPass>3)
      moreSpecialOptions_ |= 256;
    if (iPass>2)
      perturbation_=101;
    baseIteration_=numberIterations_;
    if ((specialOptions_&8192)==0)
      static_cast<AbcSimplexDual *> (this)->dual();
    baseIteration_=numberIterations_;
    if (problemStatus_==10) {
      if (initialSumInfeasibilities_>=0.0) {
	if (savePerturbation>=100) {
	  perturbation_=100;
	} else {
	  if (savePerturbation==50)
	    perturbation_=CoinMax(51,HEAVY_PERTURBATION-4-iPass); //smaller
	  else
	    perturbation_=CoinMax(51,savePerturbation-1-iPass); //smaller
	}
      } else {
	specialOptions_ |= 8192; // stop going to dual
	perturbation_=savePerturbation;
	abcFactorization_->setPivots(100000); // force factorization
	initialSumInfeasibilities_=1.23456789e-5;
	// clean pivots
	int numberBasic=0;
	int * pivot=pivotVariable();
	for (int i=0;i<numberTotal_;i++) {
	  if (getInternalStatus(i)==basic)
	    pivot[numberBasic++]=i;
	}
	assert (numberBasic==numberRows_);
      }
      if (iPass>2)
	perturbation_=100;
      copyFromSaved(14);
      baseIteration_=numberIterations_;
      static_cast<AbcSimplexPrimal *> (this)->primal(0);
    }
    minimumThetaMovement_*=0.5;
    perturbation_=savePerturbation;
  }
#endif
  return returnCode;
}
/* Sets up all slack basis and resets solution to
   as it was after initial load or readMps */
void AbcSimplex::allSlackBasis()
{
  // set column status to one nearest zero
  CoinFillN(internalStatus_,numberRows_,static_cast<unsigned char>(basic));
  const double *  COIN_RESTRICT lower = abcLower_;
  const double *  COIN_RESTRICT upper = abcUpper_;
  double *  COIN_RESTRICT solution = abcSolution_;
  // But set value to zero if lb <0.0 and ub>0.0
  for (int i = maximumAbcNumberRows_; i < numberTotal_; i++) {
    if (lower[i] >= 0.0) {
      solution[i] = lower[i];
      setInternalStatus(i, atLowerBound);
    } else if (upper[i] <= 0.0) {
      solution[i] = upper[i];
      setInternalStatus(i, atUpperBound);
    } else if (lower[i] < -1.0e20 && upper[i] > 1.0e20) {
      // free
      solution[i] = 0.0;
      setInternalStatus(i, isFree);
    } else if (fabs(lower[i]) < fabs(upper[i])) {
      solution[i] = 0.0;
      setInternalStatus(i, atLowerBound);
    } else {
      solution[i] = 0.0;
      setInternalStatus(i, atUpperBound);
    }
  }
}
#if 0 //ndef SLIM_NOIO
// Read an mps file from the given filename
int
AbcSimplex::readMps(const char *filename,
                    bool keepNames,
                    bool ignoreErrors)
{
  int status = ClpSimplex::readMps(filename, keepNames, ignoreErrors);
  ClpSimplex * thisModel=this;
  gutsOfResize(thisModel->numberRows(),thisModel->numberColumns());
  translate(DO_SCALE_AND_MATRIX|DO_BASIS_AND_ORDER|DO_STATUS|DO_SOLUTION);
  return status;
}
// Read GMPL files from the given filenames
int
AbcSimplex::readGMPL(const char *filename, const char * dataName,
                     bool keepNames)
{
  int status = ClpSimplex::readGMPL(filename, dataName, keepNames);
  translate(DO_SCALE_AND_MATRIX|DO_BASIS_AND_ORDER|DO_STATUS|DO_SOLUTION);
  return status;
}
// Read file in LP format from file with name filename.
int
AbcSimplex::readLp(const char *filename, const double epsilon )
{
  FILE *fp = fopen(filename, "r");
  
  if(!fp) {
    printf("### ERROR: AbcSimplex::readLp():  Unable to open file %s for reading\n",
	   filename);
    return(1);
  }
  CoinLpIO m;
  m.readLp(fp, epsilon);
  fclose(fp);
  
  // set problem name
  setStrParam(ClpProbName, m.getProblemName());
  // no errors
  loadProblem(*m.getMatrixByRow(), m.getColLower(), m.getColUpper(),
	      m.getObjCoefficients(), m.getRowLower(), m.getRowUpper());
  
  if (m.integerColumns()) {
    integerType_ = new char[numberColumns_];
    CoinMemcpyN(m.integerColumns(), numberColumns_, integerType_);
  } else {
    integerType_ = NULL;
  }
  translate(DO_SCALE_AND_MATRIX|DO_BASIS_AND_ORDER|DO_STATUS|DO_SOLUTION);
  unsigned int maxLength = 0;
  int iRow;
  rowNames_ = std::vector<std::string> ();
  columnNames_ = std::vector<std::string> ();
  rowNames_.reserve(numberRows_);
  for (iRow = 0; iRow < numberRows_; iRow++) {
    const char * name = m.rowName(iRow);
    if (name) {
      maxLength = CoinMax(maxLength, static_cast<unsigned int> (strlen(name)));
      rowNames_.push_back(name);
    } else {
      rowNames_.push_back("");
    }
  }
  
  int iColumn;
  columnNames_.reserve(numberColumns_);
  for (iColumn = 0; iColumn < numberColumns_; iColumn++) {
    const char * name = m.columnName(iColumn);
    if (name) {
      maxLength = CoinMax(maxLength, static_cast<unsigned int> (strlen(name)));
      columnNames_.push_back(name);
    } else {
      columnNames_.push_back("");
    }
  }
  lengthNames_ = static_cast<int> (maxLength);
  
  return 0;
}
#endif
// Factorization frequency
int
AbcSimplex::factorizationFrequency() const
{
  if (abcFactorization_)
    return abcFactorization_->maximumPivots();
  else
    return -1;
}
void
AbcSimplex::setFactorizationFrequency(int value)
{
  if (abcFactorization_)
    abcFactorization_->maximumPivots(value);
}

/* Get a clean factorization - i.e. throw out singularities
   may do more later */
int
AbcSimplex::cleanFactorization(int ifValuesPass)
{
  int status = internalFactorize(ifValuesPass ? 10 : 0);
  if (status < 0) {
    return 1; // some error
  } else {
    firstFree_=0;
    return 0;
  }
}
// Save data
ClpDataSave
AbcSimplex::saveData()
{
  ClpDataSave saved;
  saved.dualBound_ = dualBound_;
  saved.infeasibilityCost_ = infeasibilityCost_;
  saved.pivotTolerance_ = abcFactorization_->pivotTolerance();
  saved.zeroFactorizationTolerance_ = abcFactorization_->zeroTolerance();
  saved.zeroSimplexTolerance_ = zeroTolerance_;
  saved.perturbation_ = perturbation_;
  saved.forceFactorization_ = forceFactorization_;
  saved.acceptablePivot_ = acceptablePivot_;
  saved.objectiveScale_ = objectiveScale_;
  // Progress indicator
  abcProgress_.fillFromModel (this);
  return saved;
}
// Restore data
void
AbcSimplex::restoreData(ClpDataSave saved)
{
  //abcFactorization_->sparseThreshold(saved.sparseThreshold_);
  abcFactorization_->pivotTolerance(saved.pivotTolerance_);
  abcFactorization_->zeroTolerance(saved.zeroFactorizationTolerance_);
  zeroTolerance_ = saved.zeroSimplexTolerance_;
  perturbation_ = saved.perturbation_;
  infeasibilityCost_ = saved.infeasibilityCost_;
  dualBound_ = saved.dualBound_;
  forceFactorization_ = saved.forceFactorization_;
  objectiveScale_ = saved.objectiveScale_;
  acceptablePivot_ = saved.acceptablePivot_;
}
// To flag a variable
void
AbcSimplex::setFlagged( int sequence)
{
  assert (sequence>=0&&sequence<numberTotal_);
  if (sequence>=0) {
    internalStatus_[sequence] |= 64;
    // use for real disaster lastFlaggedIteration_ = numberIterations_;
    numberFlagged_++;
  }
}
#ifndef NDEBUG
// For errors to make sure print to screen
// only called in debug mode
static void indexError(int index,
                       std::string methodName)
{
  std::cerr << "Illegal index " << index << " in AbcSimplex::" << methodName << std::endl;
  throw CoinError("Illegal index", methodName, "AbcSimplex");
}
#endif
// After modifying first copy refreshes second copy and marks as updated
void 
AbcSimplex::refreshCosts()
{
  whatsChanged_ &= ~OBJECTIVE_SAME;
  CoinAbcMemcpy(abcCost_,costSaved_,numberTotal_);
}
void 
AbcSimplex::refreshLower(unsigned int type)
{
  if (type)
    whatsChanged_ &= type;
  CoinAbcMemcpy(abcLower_,lowerSaved_,numberTotal_);
}
void 
AbcSimplex::refreshUpper(unsigned int type)
{
  if (type)
    whatsChanged_ &= type;
  CoinAbcMemcpy(abcUpper_,upperSaved_,numberTotal_);
}
/* Set an objective function coefficient */
void
AbcSimplex::setObjectiveCoefficient( int elementIndex, double elementValue )
{
#ifndef NDEBUG
  if (elementIndex < 0 || elementIndex >= numberColumns_) {
    indexError(elementIndex, "setObjectiveCoefficient");
  }
#endif
  if (objective()[elementIndex] != elementValue) {
    objective()[elementIndex] = elementValue;
    // work arrays exist - update as well
    whatsChanged_ &= ~OBJECTIVE_SAME;
    double direction = optimizationDirection_ * objectiveScale_;
    costSaved_[elementIndex+maximumAbcNumberRows_] = direction * elementValue
      * columnUseScale_[elementIndex+maximumAbcNumberRows_];
  }
}
/* Set a single row lower bound<br>
   Use -DBL_MAX for -infinity. */
void
AbcSimplex::setRowLower( int elementIndex, double elementValue )
{
#ifndef NDEBUG
  int n = numberRows_;
  if (elementIndex < 0 || elementIndex >= n) {
    indexError(elementIndex, "setRowLower");
  }
#endif
  if (elementValue < -1.0e27)
    elementValue = -COIN_DBL_MAX;
  if (rowLower_[elementIndex] != elementValue) {
    rowLower_[elementIndex] = elementValue;
    // work arrays exist - update as well
    whatsChanged_ &= ~ROW_LOWER_SAME;
    if (rowLower_[elementIndex] == -COIN_DBL_MAX) {
      lowerSaved_[elementIndex] = -COIN_DBL_MAX;
    } else {
      lowerSaved_[elementIndex] = elementValue * rhsScale_
	* rowUseScale_[elementIndex];
    }
  }
}

/* Set a single row upper bound<br>
   Use DBL_MAX for infinity. */
void
AbcSimplex::setRowUpper( int elementIndex, double elementValue )
{
#ifndef NDEBUG
  int n = numberRows_;
  if (elementIndex < 0 || elementIndex >= n) {
    indexError(elementIndex, "setRowUpper");
  }
#endif
  if (elementValue > 1.0e27)
    elementValue = COIN_DBL_MAX;
  if (rowUpper_[elementIndex] != elementValue) {
    rowUpper_[elementIndex] = elementValue;
    // work arrays exist - update as well
    whatsChanged_ &= ~ROW_UPPER_SAME;
    if (rowUpper_[elementIndex] == COIN_DBL_MAX) {
      upperSaved_[elementIndex] = COIN_DBL_MAX;
    } else {
      upperSaved_[elementIndex] = elementValue * rhsScale_
	* rowUseScale_[elementIndex];
    }
  }
}

/* Set a single row lower and upper bound */
void
AbcSimplex::setRowBounds( int elementIndex,
                          double lowerValue, double upperValue )
{
#ifndef NDEBUG
  int n = numberRows_;
  if (elementIndex < 0 || elementIndex >= n) {
    indexError(elementIndex, "setRowBounds");
  }
#endif
  if (lowerValue < -1.0e27)
    lowerValue = -COIN_DBL_MAX;
  if (upperValue > 1.0e27)
    upperValue = COIN_DBL_MAX;
  //CoinAssert (upperValue>=lowerValue);
  if (rowLower_[elementIndex] != lowerValue) {
    rowLower_[elementIndex] = lowerValue;
    // work arrays exist - update as well
    whatsChanged_ &= ~ROW_LOWER_SAME;
    if (rowLower_[elementIndex] == -COIN_DBL_MAX) {
      lowerSaved_[elementIndex] = -COIN_DBL_MAX;
    } else {
      lowerSaved_[elementIndex] = lowerValue * rhsScale_
	* rowUseScale_[elementIndex];
    }
  }
  if (rowUpper_[elementIndex] != upperValue) {
    rowUpper_[elementIndex] = upperValue;
    // work arrays exist - update as well
    whatsChanged_ &= ~ROW_UPPER_SAME;
    if (rowUpper_[elementIndex] == COIN_DBL_MAX) {
      upperSaved_[elementIndex] = COIN_DBL_MAX;
    } else {
      upperSaved_[elementIndex] = upperValue * rhsScale_
	* rowUseScale_[elementIndex];
    }
  }
}
void AbcSimplex::setRowSetBounds(const int* indexFirst,
                                 const int* indexLast,
                                 const double* boundList)
{
#ifndef NDEBUG
  int n = numberRows_;
#endif
  int numberChanged = 0;
  const int * saveFirst = indexFirst;
  while (indexFirst != indexLast) {
    const int iRow = *indexFirst++;
#ifndef NDEBUG
    if (iRow < 0 || iRow >= n) {
      indexError(iRow, "setRowSetBounds");
    }
#endif
    double lowerValue = *boundList++;
    double upperValue = *boundList++;
    if (lowerValue < -1.0e27)
      lowerValue = -COIN_DBL_MAX;
    if (upperValue > 1.0e27)
      upperValue = COIN_DBL_MAX;
    //CoinAssert (upperValue>=lowerValue);
    if (rowLower_[iRow] != lowerValue) {
      rowLower_[iRow] = lowerValue;
      whatsChanged_ &= ~ROW_LOWER_SAME;
      numberChanged++;
    }
    if (rowUpper_[iRow] != upperValue) {
      rowUpper_[iRow] = upperValue;
      whatsChanged_ &= ~ROW_UPPER_SAME;
      numberChanged++;
    }
  }
  if (numberChanged) {
    indexFirst = saveFirst;
    while (indexFirst != indexLast) {
      const int iRow = *indexFirst++;
      if (rowLower_[iRow] == -COIN_DBL_MAX) {
	lowerSaved_[iRow] = -COIN_DBL_MAX;
      } else {
	lowerSaved_[iRow] = rowLower_[iRow] * rhsScale_
	  * rowUseScale_[iRow];
      }
      if (rowUpper_[iRow] == COIN_DBL_MAX) {
	upperSaved_[iRow] = COIN_DBL_MAX;
      } else {
	upperSaved_[iRow] = rowUpper_[iRow] * rhsScale_
	  * rowUseScale_[iRow];
      }
    }
  }
}
//-----------------------------------------------------------------------------
/* Set a single column lower bound<br>
   Use -DBL_MAX for -infinity. */
void
AbcSimplex::setColumnLower( int elementIndex, double elementValue )
{
#ifndef NDEBUG
  int n = numberColumns_;
  if (elementIndex < 0 || elementIndex >= n) {
    indexError(elementIndex, "setColumnLower");
  }
#endif
  if (elementValue < -1.0e27)
    elementValue = -COIN_DBL_MAX;
  if (columnLower_[elementIndex] != elementValue) {
    columnLower_[elementIndex] = elementValue;
    // work arrays exist - update as well
    whatsChanged_ &= ~COLUMN_LOWER_SAME;
    double value;
    if (columnLower_[elementIndex] == -COIN_DBL_MAX) {
      value = -COIN_DBL_MAX;
    } else {
      value = elementValue * rhsScale_
	* inverseColumnUseScale_[elementIndex];
    }
    lowerSaved_[elementIndex+maximumAbcNumberRows_] = value;
  }
}

/* Set a single column upper bound<br>
   Use DBL_MAX for infinity. */
void
AbcSimplex::setColumnUpper( int elementIndex, double elementValue )
{
#ifndef NDEBUG
  int n = numberColumns_;
  if (elementIndex < 0 || elementIndex >= n) {
    indexError(elementIndex, "setColumnUpper");
  }
#endif
  if (elementValue > 1.0e27)
    elementValue = COIN_DBL_MAX;
  if (columnUpper_[elementIndex] != elementValue) {
    columnUpper_[elementIndex] = elementValue;
    // work arrays exist - update as well
    whatsChanged_ &= ~COLUMN_UPPER_SAME;
    double value;
    if (columnUpper_[elementIndex] == COIN_DBL_MAX) {
      value = COIN_DBL_MAX;
    } else {
      value = elementValue * rhsScale_
	* inverseColumnUseScale_[elementIndex];
    }
    upperSaved_[elementIndex+maximumAbcNumberRows_] = value;
  }
}

/* Set a single column lower and upper bound */
void
AbcSimplex::setColumnBounds( int elementIndex,
                             double lowerValue, double upperValue )
{
#ifndef NDEBUG
  int n = numberColumns_;
  if (elementIndex < 0 || elementIndex >= n) {
    indexError(elementIndex, "setColumnBounds");
  }
#endif
  if (lowerValue < -1.0e27)
    lowerValue = -COIN_DBL_MAX;
  if (columnLower_[elementIndex] != lowerValue) {
    columnLower_[elementIndex] = lowerValue;
    // work arrays exist - update as well
    whatsChanged_ &= ~COLUMN_LOWER_SAME;
    if (columnLower_[elementIndex] == -COIN_DBL_MAX) {
      lowerSaved_[elementIndex+maximumAbcNumberRows_] = -COIN_DBL_MAX;
    } else {
      lowerSaved_[elementIndex+maximumAbcNumberRows_] = lowerValue * rhsScale_
	* inverseColumnUseScale_[elementIndex];
    }
  }
  if (upperValue > 1.0e27)
    upperValue = COIN_DBL_MAX;
  if (columnUpper_[elementIndex] != upperValue) {
    columnUpper_[elementIndex] = upperValue;
    // work arrays exist - update as well
    whatsChanged_ &= ~COLUMN_UPPER_SAME;
    if (columnUpper_[elementIndex] == COIN_DBL_MAX) {
      upperSaved_[elementIndex+maximumAbcNumberRows_] = COIN_DBL_MAX;
    } else {
      upperSaved_[elementIndex+maximumAbcNumberRows_] = upperValue * rhsScale_
	* inverseColumnUseScale_[elementIndex];
    }
  }
}
void AbcSimplex::setColumnSetBounds(const int* indexFirst,
                                    const int* indexLast,
                                    const double* boundList)
{
#ifndef NDEBUG
  int n = numberColumns_;
#endif
  int numberChanged = 0;
  const int * saveFirst = indexFirst;
  while (indexFirst != indexLast) {
    const int iColumn = *indexFirst++;
#ifndef NDEBUG
    if (iColumn < 0 || iColumn >= n) {
      indexError(iColumn, "setColumnSetBounds");
    }
#endif
    double lowerValue = *boundList++;
    double upperValue = *boundList++;
    if (lowerValue < -1.0e27)
      lowerValue = -COIN_DBL_MAX;
    if (upperValue > 1.0e27)
      upperValue = COIN_DBL_MAX;
    //CoinAssert (upperValue>=lowerValue);
    if (columnLower_[iColumn] != lowerValue) {
      columnLower_[iColumn] = lowerValue;
      whatsChanged_ &= ~COLUMN_LOWER_SAME;
      numberChanged++;
    }
    if (columnUpper_[iColumn] != upperValue) {
      columnUpper_[iColumn] = upperValue;
      whatsChanged_ &= ~COLUMN_UPPER_SAME;
      numberChanged++;
    }
  }
  if (numberChanged) {
    indexFirst = saveFirst;
    while (indexFirst != indexLast) {
      const int iColumn = *indexFirst++;
      if (columnLower_[iColumn] == -COIN_DBL_MAX) {
	lowerSaved_[iColumn+maximumAbcNumberRows_] = -COIN_DBL_MAX;
      } else {
	lowerSaved_[iColumn+maximumAbcNumberRows_] = columnLower_[iColumn] * rhsScale_
	  * inverseColumnUseScale_[iColumn];
      }
      if (columnUpper_[iColumn] == COIN_DBL_MAX) {
	upperSaved_[iColumn+maximumAbcNumberRows_] = COIN_DBL_MAX;
      } else {
	upperSaved_[iColumn+maximumAbcNumberRows_] = columnUpper_[iColumn] * rhsScale_
	  * inverseColumnUseScale_[iColumn];
      }
    }
  }
}
#ifndef SLIM_CLP
#include "CoinWarmStartBasis.hpp"

// Returns a basis (to be deleted by user)
CoinWarmStartBasis *
AbcSimplex::getBasis() const
{
  CoinWarmStartBasis * basis = new CoinWarmStartBasis();
  basis->setSize(numberColumns_, numberRows_);
  
  unsigned char lookup[8]={2,3,255,255,0,0,1,3};
  for (int iRow = 0; iRow < numberRows_; iRow++) {
    int iStatus = getInternalStatus(iRow);
    iStatus = lookup[iStatus];
    basis->setArtifStatus(iRow, static_cast<CoinWarmStartBasis::Status> (iStatus));
  }
  for (int iColumn = 0; iColumn < numberColumns_; iColumn++) {
    int iStatus = getInternalStatus(iColumn+maximumAbcNumberRows_);
    iStatus = lookup[iStatus];
    basis->setStructStatus(iColumn, static_cast<CoinWarmStartBasis::Status> (iStatus));
  }
  return basis;
}
#endif
// Compute objective value from solution
void
AbcSimplex::computeObjectiveValue(bool useInternalArrays)
{
  const double * obj = objective();
  if (!useInternalArrays) {
    objectiveValue_ = 0.0;
    for (int iSequence = 0; iSequence < numberColumns_; iSequence++) {
      double value = columnActivity_[iSequence];
      objectiveValue_ += value * obj[iSequence];
    }
    // But remember direction as we are using external objective
    objectiveValue_ *= optimizationDirection_;
  } else {
    rawObjectiveValue_ = 0.0;
    for (int iSequence = maximumAbcNumberRows_; iSequence < numberTotal_; iSequence++) {
      double value = abcSolution_[iSequence];
      rawObjectiveValue_ += value * abcCost_[iSequence];
    }
    setClpSimplexObjectiveValue();
  }
}
// Compute minimization objective value from internal solution
double
AbcSimplex::computeInternalObjectiveValue()
{
  rawObjectiveValue_ = 0.0;
  for (int iSequence = maximumAbcNumberRows_; iSequence < numberTotal_; iSequence++) {
    double value = abcSolution_[iSequence];
    rawObjectiveValue_ += value * abcCost_[iSequence];
  }
  setClpSimplexObjectiveValue();
  return objectiveValue_-dblParam_[ClpObjOffset];
}
// If user left factorization frequency then compute
void
AbcSimplex::defaultFactorizationFrequency()
{
  if (factorizationFrequency() == 200) {
    // User did not touch preset
    const int cutoff1 = 10000;
    const int cutoff2 = 100000;
    const int base = 75;
    const int freq0 = 50;
    const int freq1 = 150;
    const int maximum = 10000;
    int frequency;
    if (numberRows_ < cutoff1)
      frequency = base + numberRows_ / freq0;
    else 
      frequency = base + cutoff1 / freq0 + (numberRows_ - cutoff1) / freq1;
    setFactorizationFrequency(CoinMin(maximum, frequency));
  }
}
// Gets clean and emptyish factorization
AbcSimplexFactorization *
AbcSimplex::getEmptyFactorization()
{
  if ((specialOptions_ & 65536) == 0) {
    assert (!abcFactorization_);
    abcFactorization_ = new AbcSimplexFactorization();
  } else if (!abcFactorization_) {
    abcFactorization_ = new AbcSimplexFactorization();
  }
  return abcFactorization_;
}
// Resizes rim part of model
void
AbcSimplex::resize (int newNumberRows, int newNumberColumns)
{
  assert (maximumAbcNumberRows_>=0);
  // save
  int numberRows=numberRows_;
  int numberColumns=numberColumns_;
  ClpSimplex::resize(newNumberRows, newNumberColumns);
  // back out
  numberRows_=numberRows;
  numberColumns_=numberColumns;
  gutsOfResize(newNumberRows,newNumberColumns);
}
// Return true if the objective limit test can be relied upon
bool
AbcSimplex::isObjectiveLimitTestValid() const
{
  if (problemStatus_ == 0) {
    return true;
  } else if (problemStatus_ == 1) {
    // ok if dual
    return (algorithm_ < 0);
  } else if (problemStatus_ == 2) {
    // ok if primal
    return (algorithm_ > 0);
  } else {
    return false;
  }
}
/*
  Permutes in from ClpModel data - assumes scale factors done
  and AbcMatrix exists but is in original order (including slacks)
  For now just add basicArray at end
  ==
  But could partition into
  normal (i.e. reasonable lower/upper)
  abnormal - free, odd bounds
  fixed
  ==
  sets a valid pivotVariable
  Slacks always shifted by offset
  Fixed variables always shifted by offset
  Recode to allow row objective so can use pi from idiot etc
*/
void 
AbcSimplex::permuteIn()
{
  // for now only if maximumAbcNumberRows_==numberRows_
  //assert(maximumAbcNumberRows_==numberRows_);
  numberTotal_=maximumAbcNumberRows_+numberColumns_;
  double direction = optimizationDirection_;
  // all this could use cilk
  // move primal stuff to end
  const double *  COIN_RESTRICT rowScale=scaleFromExternal_;
  const double *  COIN_RESTRICT inverseRowScale=scaleToExternal_;
  const double *  COIN_RESTRICT columnScale=scaleToExternal_+maximumAbcNumberRows_;
  const double *  COIN_RESTRICT inverseColumnScale=scaleFromExternal_+maximumAbcNumberRows_;
  double *  COIN_RESTRICT offsetRhs=offsetRhs_;
  double *  COIN_RESTRICT saveLower=lowerSaved_+maximumAbcNumberRows_;
  double *  COIN_RESTRICT saveUpper=upperSaved_+maximumAbcNumberRows_;
  double *  COIN_RESTRICT saveCost=costSaved_+maximumAbcNumberRows_;
  double *  COIN_RESTRICT saveSolution=solutionSaved_+maximumAbcNumberRows_;
  CoinAbcMemset0(offsetRhs,maximumAbcNumberRows_);
  const double *  COIN_RESTRICT objective=this->objective();
  objectiveOffset_=0.0;
  double *  COIN_RESTRICT offset=offset_+maximumAbcNumberRows_;
  const int * COIN_RESTRICT row = abcMatrix_->matrix()->getIndices();
  const CoinBigIndex * COIN_RESTRICT columnStart = abcMatrix_->matrix()->getVectorStarts();
  const double * COIN_RESTRICT elementByColumn = abcMatrix_->matrix()->getElements();
  largestGap_=1.0e-12;
  for (int iColumn=0;iColumn<numberColumns_;iColumn++) {
    double scale=inverseColumnScale[iColumn];
    double lowerValue=columnLower_[iColumn];
    double upperValue=columnUpper_[iColumn];
    double thisOffset=0.0;
    double lowerValue2 = fabs(lowerValue);
    double upperValue2 = fabs(upperValue);
    if (CoinMin(lowerValue2,upperValue2)<1000.0) {
      // move to zero
      if (lowerValue2>upperValue2) 
	thisOffset=upperValue;
      else
	thisOffset=lowerValue;
    }
    offset[iColumn]=thisOffset;
    objectiveOffset_ += thisOffset*objective[iColumn]*optimizationDirection_;
    double scaledOffset = thisOffset*scale;
    for (CoinBigIndex j=columnStart[iColumn];j<columnStart[iColumn+1];j++) {
      int iRow=row[j];
      offsetRhs[iRow]+= scaledOffset*elementByColumn[j];
    }
    lowerValue-=thisOffset;
    if (lowerValue>-1.0e30) 
      lowerValue*=scale;
    saveLower[iColumn]=lowerValue;
    upperValue-=thisOffset;
    if (upperValue<1.0e30) 
      upperValue*=scale;
    saveUpper[iColumn]=upperValue;
    largestGap_=CoinMax(largestGap_,upperValue-lowerValue);
    saveSolution[iColumn]=scale*(columnActivity_[iColumn]-thisOffset);
    saveCost[iColumn]=objective[iColumn]*direction*columnScale[iColumn];
  }
  CoinAbcMemset0(offset_,maximumAbcNumberRows_);
  saveLower-=maximumAbcNumberRows_;
  saveUpper-=maximumAbcNumberRows_;
  saveCost-=maximumAbcNumberRows_;
  saveSolution-=maximumAbcNumberRows_;
  for (int iRow=0;iRow<numberRows_;iRow++) {
    // note switch of lower to upper
    double upperValue=-rowLower_[iRow];
    double lowerValue=-rowUpper_[iRow];
    double thisOffset=offsetRhs_[iRow];
    double scale=rowScale[iRow];
    if (lowerValue>-1.0e30) 
      lowerValue=lowerValue*scale+thisOffset;
    saveLower[iRow]=lowerValue;
    if (upperValue<1.0e30) 
      upperValue=upperValue*scale+thisOffset;
    saveUpper[iRow]=upperValue;
    largestGap_=CoinMax(largestGap_,upperValue-lowerValue);
    saveCost[iRow]=0.0;
    dual_[iRow]*=direction*inverseRowScale[iRow];
    saveSolution[iRow]=0.0; // not necessary
  }
  dualBound_=CoinMin(dualBound_,largestGap_);
  // Compute rhsScale_ and objectiveScale_
  double minValue=COIN_DBL_MAX;
  double maxValue=0.0;
  CoinAbcMinMaxAbsNormalValues(costSaved_+maximumAbcNumberRows_,numberTotal_-maximumAbcNumberRows_,minValue,maxValue);
  // scale to 1000.0 ?
  if (minValue&&false) {
    objectiveScale_= 1000.0/sqrt(minValue*maxValue);
    objectiveScale_=CoinMin(1.0,1000.0/maxValue);
#ifndef NDEBUG
    double smallestNormal=COIN_DBL_MAX;
    double smallestAny=COIN_DBL_MAX;
    double largestAny=0.0;
    for (int i=0;i<numberTotal_;i++) {
      double value=fabs(costSaved_[i]);
      if (value) {
	if (value>1.0e-8)
	  smallestNormal=CoinMin(smallestNormal,value);
	smallestAny=CoinMin(smallestAny,value);
	largestAny=CoinMax(largestAny,value);
      }
    }
    printf("objectiveScale_ %g min_used %g (min_reasonable %g, min_any %g) max_used %g (max_any %g)\n",
	   objectiveScale_,minValue,smallestNormal,smallestAny,maxValue,largestAny);
#endif
  } else {
    //objectiveScale_=1.0;
  }
  CoinAbcScale(costSaved_,objectiveScale_,numberTotal_);
  minValue=COIN_DBL_MAX;
  maxValue=0.0;
  CoinAbcMinMaxAbsNormalValues(lowerSaved_,numberTotal_,minValue,maxValue);
  CoinAbcMinMaxAbsNormalValues(upperSaved_,numberTotal_,minValue,maxValue);
  // scale to 100.0 ?
  if (minValue&&false) {
    rhsScale_= 100.0/sqrt(minValue*maxValue);
#ifndef NDEBUG
    double smallestNormal=COIN_DBL_MAX;
    double smallestAny=COIN_DBL_MAX;
    double largestAny=0.0;
    for (int i=0;i<numberTotal_;i++) {
      double value=fabs(lowerSaved_[i]);
      if (value&&value!=COIN_DBL_MAX) {
	if (value>1.0e-8)
	  smallestNormal=CoinMin(smallestNormal,value);
	smallestAny=CoinMin(smallestAny,value);
	largestAny=CoinMax(largestAny,value);
      }
    }
    for (int i=0;i<numberTotal_;i++) {
      double value=fabs(upperSaved_[i]);
      if (value&&value!=COIN_DBL_MAX) {
	if (value>1.0e-8)
	  smallestNormal=CoinMin(smallestNormal,value);
	smallestAny=CoinMin(smallestAny,value);
	largestAny=CoinMax(largestAny,value);
      }
    }
    printf("rhsScale_ %g min_used %g (min_reasonable %g, min_any %g) max_used %g (max_any %g)\n",
	   rhsScale_,minValue,smallestNormal,smallestAny,maxValue,largestAny);
#endif
  } else {
    rhsScale_=1.0;
  }
  CoinAbcScaleNormalValues(lowerSaved_,rhsScale_,1.0e-13,numberTotal_);
  CoinAbcScaleNormalValues(upperSaved_,rhsScale_,1.0e-13,numberTotal_);
  // copy
  CoinAbcMemcpy(abcLower_,abcLower_+maximumNumberTotal_,numberTotal_);
  CoinAbcMemcpy(abcUpper_,abcUpper_+maximumNumberTotal_,numberTotal_);
  CoinAbcMemcpy(abcSolution_,abcSolution_+maximumNumberTotal_,numberTotal_);
  CoinAbcMemcpy(abcCost_,abcCost_+maximumNumberTotal_,numberTotal_);
}
void 
AbcSimplex::permuteBasis()
{
  assert (abcPivotVariable_||(!numberRows_&&!numberColumns_));
  int numberBasic=0;
  // from Clp enum to Abc enum (and bound flip)
  unsigned char lookupToAbcSlack[6]={4,6,0/*1*/,1/*0*/,5,7};
  unsigned char *  COIN_RESTRICT getStatus = status_+numberColumns_;
  double *  COIN_RESTRICT solutionSaved=solutionSaved_;
  double *  COIN_RESTRICT lowerSaved=lowerSaved_;
  double *  COIN_RESTRICT upperSaved=upperSaved_;
  bool ordinaryVariables=true;
  bool valuesPass=(stateOfProblem_&VALUES_PASS)!=0;
  if (valuesPass) {
    // get solution
    CoinAbcMemset0(abcSolution_,numberRows_);
    abcMatrix_->timesIncludingSlacks(-1.0,abcSolution_,abcSolution_);
    //double * temp = new double[numberRows_];
    //memset(temp,0,numberRows_*sizeof(double));
    //matrix_->times(1.0,columnActivity_,temp);
    CoinAbcMemcpy(solutionSaved_,abcSolution_,numberRows_);
    int n=0;
    for (int i=0;i<numberTotal_;i++) {
      if (solutionSaved_[i]>upperSaved_[i]+1.0e-5)
	n++;
      else if (solutionSaved_[i]<lowerSaved_[i]-1.0e-5)
	n++;
    }
#if ABC_NORMAL_DEBUG>0
    if (n)
      printf("%d infeasibilities\n",n);
#endif
  }
  // dual at present does not like superbasic
  for (int iRow=0;iRow<numberRows_;iRow++) {
    unsigned char status=getStatus[iRow]&7;
    AbcSimplex::Status abcStatus=static_cast<AbcSimplex::Status>(lookupToAbcSlack[status]);
    if (status!=ClpSimplex::basic) {
      double lowerValue=lowerSaved[iRow];
      double upperValue=upperSaved[iRow];
      if (lowerValue==-COIN_DBL_MAX) {
	if(upperValue==COIN_DBL_MAX) {
	  // free
	  abcStatus=isFree;
	  ordinaryVariables=false;
	} else {
	  abcStatus=atUpperBound;
	}
      } else if (upperValue==COIN_DBL_MAX) {
	abcStatus=atLowerBound;
      } else if (lowerValue==upperValue) {
	abcStatus=isFixed;
      } else if (abcStatus==isFixed) {
	double value=solutionSaved[iRow];
	if (value-lowerValue<upperValue-value)
	  abcStatus=atLowerBound;
	else
	  abcStatus=atUpperBound;
      }
      if (valuesPass) {
	double value=solutionSaved[iRow];
	if (value>lowerValue+primalTolerance_&&value<upperValue-primalTolerance_&&
	    (abcStatus==atLowerBound||abcStatus==atUpperBound))
	    abcStatus=superBasic;
      }
      switch (abcStatus) {
      case isFixed:
      case atLowerBound:
	solutionSaved[iRow]=lowerValue;
	break;
      case atUpperBound:
	solutionSaved[iRow]=upperValue;
	break;
      default:
	ordinaryVariables=false;
	break;
      }
    } else {
      // basic
      abcPivotVariable_[numberBasic++]=iRow;
    }
    internalStatus_[iRow]=abcStatus;
  }
  // from Clp enum to Abc enum
  unsigned char lookupToAbc[6]={4,6,1,0,5,7};
  unsigned char *  COIN_RESTRICT putStatus=internalStatus_+maximumAbcNumberRows_;
  getStatus = status_;
  solutionSaved+= maximumAbcNumberRows_;
  lowerSaved+= maximumAbcNumberRows_;
  upperSaved+= maximumAbcNumberRows_;
  for (int iColumn=0;iColumn<numberColumns_;iColumn++) {
    unsigned char status=getStatus[iColumn]&7;
    AbcSimplex::Status abcStatus=static_cast<AbcSimplex::Status>(lookupToAbc[status]);
    if (status!=ClpSimplex::basic) {
      double lowerValue=lowerSaved[iColumn];
      double upperValue=upperSaved[iColumn];
      if (lowerValue==-COIN_DBL_MAX) {
	if(upperValue==COIN_DBL_MAX) {
	  // free
	  abcStatus=isFree;
	  ordinaryVariables=false;
	} else {
	  abcStatus=atUpperBound;
	}
      } else if (upperValue==COIN_DBL_MAX) {
	abcStatus=atLowerBound;
      } else if (lowerValue==upperValue) {
	abcStatus=isFixed;
      } else if (abcStatus==isFixed) {
	double value=solutionSaved[iColumn];
	if (value-lowerValue<upperValue-value)
	  abcStatus=atLowerBound;
	else
	  abcStatus=atUpperBound;
      } else if (abcStatus==isFree) {
	abcStatus=superBasic;
      }
      if (valuesPass&&(abcStatus==atLowerBound||abcStatus==atUpperBound)) {
	double value=solutionSaved[iColumn];
	if (value>lowerValue+primalTolerance_) {
	  if(value<upperValue-primalTolerance_) {
	    abcStatus=superBasic;
	  } else {
	    abcStatus=atUpperBound;
	  }
	} else {
	  abcStatus=atLowerBound;
	}
      }
      switch (abcStatus) {
      case isFixed:
      case atLowerBound:
	solutionSaved[iColumn]=lowerValue;
	break;
      case atUpperBound:
	solutionSaved[iColumn]=upperValue;
	break;
      default:
	ordinaryVariables=false;
	break;
      }
    } else {
      // basic
      if (numberBasic<numberRows_) 
	abcPivotVariable_[numberBasic++]=iColumn+maximumAbcNumberRows_;
      else
	abcStatus=superBasic;
    }
    putStatus[iColumn]=abcStatus;
  }
  ordinaryVariables_ =  ordinaryVariables ? 1 : 0;
  if (numberBasic<numberRows_) {
    for (int iRow=0;iRow<numberRows_;iRow++) {
      AbcSimplex::Status status=getInternalStatus(iRow);
      if (status!=AbcSimplex::basic) {
	abcPivotVariable_[numberBasic++]=iRow;
	setInternalStatus(iRow,basic);
	if (numberBasic==numberRows_)
	  break;
      }
    }
  }
  // copy
  CoinAbcMemcpy(internalStatus_+maximumNumberTotal_,internalStatus_,numberTotal_);
}
// Permutes out - bit settings same as stateOfProblem
void 
AbcSimplex::permuteOut(int whatsWanted)
{
  assert (whatsWanted);
  if ((whatsWanted&ALL_STATUS_OK)!=0&&(stateOfProblem_&ALL_STATUS_OK)==0) {
    stateOfProblem_ |= ALL_STATUS_OK;
    // from Abc enum to Clp enum (and bound flip)
    unsigned char lookupToClpSlack[8]={2,3,255,255,0,0,1,5};
    unsigned char *  COIN_RESTRICT putStatus = status_+numberColumns_;
    const unsigned char *  COIN_RESTRICT getStatus=internalStatus_;
    for (int iRow=0;iRow<numberRows_;iRow++) {
      putStatus[iRow]=lookupToClpSlack[getStatus[iRow]&7];
    }
    // from Abc enum to Clp enum
    unsigned char lookupToClp[8]={3,2,255,255,0,0,1,5};
    putStatus = status_;
    getStatus=internalStatus_+maximumAbcNumberRows_;
    for (int iColumn=0;iColumn<numberColumns_;iColumn++) {
      putStatus[iColumn]=lookupToClp[getStatus[iColumn]&7];
    }
  }
  const double *  COIN_RESTRICT rowScale=scaleFromExternal_;
  const double *  COIN_RESTRICT inverseRowScale=scaleToExternal_;
  const double *  COIN_RESTRICT columnScale=scaleToExternal_; // allow for offset in loop +maximumAbcNumberRows_;
  const double *  COIN_RESTRICT inverseColumnScale=scaleFromExternal_; // allow for offset in loop +maximumAbcNumberRows_;
  double scaleC = 1.0 / objectiveScale_;
  double scaleR = 1.0 / rhsScale_;
  int numberPrimalScaled = 0;
  int numberPrimalUnscaled = 0;
  int numberDualScaled = 0;
  int numberDualUnscaled = 0;
  bool filledInSolution=false;
  if ((whatsWanted&ROW_PRIMAL_OK)!=0&&(stateOfProblem_&ROW_PRIMAL_OK)==0) {
    stateOfProblem_ |= ROW_PRIMAL_OK;
    if (!filledInSolution) {
      filledInSolution=true;
      CoinAbcScatterTo(solutionBasic_,abcSolution_,abcPivotVariable_,numberRows_);
      CoinAbcScatterZeroTo(abcDj_,abcPivotVariable_,numberRows_);
    }
    // Collect infeasibilities
    double *  COIN_RESTRICT putSolution = rowActivity_;
    const double *  COIN_RESTRICT rowLower =rowLower_;
    const double *  COIN_RESTRICT rowUpper =rowUpper_;
    const double *  COIN_RESTRICT abcLower =abcLower_;
    const double *  COIN_RESTRICT abcUpper =abcUpper_;
    const double *  COIN_RESTRICT abcSolution =abcSolution_;
    const double *  COIN_RESTRICT offsetRhs =offsetRhs_;
    for (int i = 0; i < numberRows_; i++) {
      double scaleFactor = inverseRowScale[i];
      double valueScaled = abcSolution[i];
      double lowerScaled = abcLower[i];
      double upperScaled = abcUpper[i];
      if (lowerScaled > -1.0e20 || upperScaled < 1.0e20) {
	if (valueScaled < lowerScaled - primalTolerance_ ||
	    valueScaled > upperScaled + primalTolerance_)
	  numberPrimalScaled++;
	else
	  upperOut_ = CoinMax(upperOut_, CoinMin(valueScaled - lowerScaled, upperScaled - valueScaled));
      }
      double value = (offsetRhs[i]-valueScaled * scaleR) * scaleFactor;
      putSolution[i]=value;
      if (value < rowLower[i] - primalTolerance_)
	numberPrimalUnscaled++;
      else if (value > rowUpper[i] + primalTolerance_)
	numberPrimalUnscaled++;
    }
  }
  if ((whatsWanted&COLUMN_PRIMAL_OK)!=0&&(stateOfProblem_&COLUMN_PRIMAL_OK)==0) {
    stateOfProblem_ |= COLUMN_PRIMAL_OK;
    // Collect infeasibilities
    if (!filledInSolution) {
      filledInSolution=true;
      CoinAbcScatterTo(solutionBasic_,abcSolution_,abcPivotVariable_,numberRows_);
      CoinAbcScatterZeroTo(abcDj_,abcPivotVariable_,numberRows_);
    }
    // Collect infeasibilities
    double *  COIN_RESTRICT putSolution = columnActivity_-maximumAbcNumberRows_;
    const double *  COIN_RESTRICT columnLower=columnLower_-maximumAbcNumberRows_;
    const double *  COIN_RESTRICT columnUpper=columnUpper_-maximumAbcNumberRows_;
    for (int i = maximumAbcNumberRows_; i < numberTotal_; i++) {
      double scaleFactor = columnScale[i];
      double valueScaled = abcSolution_[i];
      double lowerScaled = abcLower_[i];
      double upperScaled = abcUpper_[i];
      if (lowerScaled > -1.0e20 || upperScaled < 1.0e20) {
	if (valueScaled < lowerScaled - primalTolerance_ ||
	    valueScaled > upperScaled + primalTolerance_)
	  numberPrimalScaled++;
	else
	  upperOut_ = CoinMax(upperOut_, CoinMin(valueScaled - lowerScaled, upperScaled - valueScaled));
      }
      double value = (valueScaled * scaleR) * scaleFactor+offset_[i];
      putSolution[i]=value;
      if (value < columnLower[i] - primalTolerance_)
	numberPrimalUnscaled++;
      else if (value > columnUpper[i] + primalTolerance_)
	numberPrimalUnscaled++;
    }
  }
  if ((whatsWanted&COLUMN_DUAL_OK)!=0&&(stateOfProblem_&COLUMN_DUAL_OK)==0) {
    // Get fixed djs
    CoinAbcMemcpy(abcDj_,abcCost_,numberTotal_);
    abcMatrix_->transposeTimesAll( dual_,abcDj_);
    stateOfProblem_ |= COLUMN_DUAL_OK;
    // Collect infeasibilities
    double *  COIN_RESTRICT putDj=reducedCost_-maximumAbcNumberRows_;
    for (int i = maximumAbcNumberRows_; i < numberTotal_; i++) {
      double scaleFactor = inverseColumnScale[i];
      double valueDual = abcDj_[i];
      double value=abcSolution_[i];
      bool aboveLower= value>abcLower_[i]+primalTolerance_;
      bool belowUpper= value<abcUpper_[i]-primalTolerance_;
      if (aboveLower&& valueDual > dualTolerance_)
	numberDualScaled++;
      if (belowUpper && valueDual < -dualTolerance_)
	numberDualScaled++;
      valueDual *= scaleFactor * scaleC;
      putDj[i]=valueDual;
      if (aboveLower&& valueDual > dualTolerance_)
	numberDualUnscaled++;
      if (belowUpper && valueDual < -dualTolerance_)
	numberDualUnscaled++;
    }
  }
  if ((whatsWanted&ROW_DUAL_OK)!=0&&(stateOfProblem_&ROW_DUAL_OK)==0) {
    stateOfProblem_ |= ROW_DUAL_OK;
    // Collect infeasibilities
    for (int i = 0; i < numberRows_; i++) {
      double scaleFactor = rowScale[i];
      double valueDual = abcDj_[i]; // ? +- and direction
      double value=abcSolution_[i];
      bool aboveLower= value>abcLower_[i]+primalTolerance_;
      bool belowUpper= value<abcUpper_[i]-primalTolerance_;
      if (aboveLower&& valueDual > dualTolerance_)
	numberDualScaled++;
      if (belowUpper && valueDual < -dualTolerance_)
	numberDualScaled++;
      valueDual *= scaleFactor * scaleC;
      dual_[i]=-(valueDual-abcCost_[i]); // sign
      if (aboveLower&& valueDual > dualTolerance_)
	numberDualUnscaled++;
      if (belowUpper && valueDual < -dualTolerance_)
	numberDualUnscaled++;
    }
  }
  // do after djs
  if (!problemStatus_ && (!secondaryStatus_||secondaryStatus_==2||secondaryStatus_==3)) {
    // See if we need to set secondary status
    if (numberPrimalUnscaled) {
      if (numberDualUnscaled||secondaryStatus_==3)
	secondaryStatus_ = 4;
      else
	secondaryStatus_ = 2;
    } else if (numberDualUnscaled) {
      if (secondaryStatus_==0)
	secondaryStatus_ = 3;
      else
	secondaryStatus_ = 4;
    }
  }
  if (problemStatus_ == 2) {
    for (int i = 0; i < numberColumns_; i++) {
      ray_[i] *= columnScale[i];
    }
  } else if (problemStatus_ == 1 && ray_) {
    for (int i = 0; i < numberRows_; i++) {
      ray_[i] *= rowScale[i];
    }
  }
}
#if ABC_DEBUG>1
// For debug - moves solution back to external and computes stuff
void 
AbcSimplex::checkMoveBack(bool checkDuals)
{
  stateOfProblem_ &= ~(ROW_PRIMAL_OK|ROW_DUAL_OK|COLUMN_PRIMAL_OK|COLUMN_DUAL_OK|ALL_STATUS_OK);
  permuteOut(ROW_PRIMAL_OK|ROW_DUAL_OK|COLUMN_PRIMAL_OK|COLUMN_DUAL_OK|ALL_STATUS_OK);
  ClpSimplex::computeObjectiveValue(false);
#if ABC_NORMAL_DEBUG>0
  printf("Check objective %g\n",objectiveValue()-objectiveOffset_);
#endif
  double * region = new double [numberRows_+numberColumns_];
  CoinAbcMemset0(region,numberRows_);
  ClpModel::times(1.0,columnActivity_,region);
  int numberInf;
  double sumInf;
  numberInf=0;
  sumInf=0.0;
  for (int i=0;i<numberColumns_;i++) {
    if (columnActivity_[i]>columnUpper_[i]+1.0e-7) {
      numberInf++;
      sumInf+=columnActivity_[i]-columnUpper_[i];
    } else if (columnActivity_[i]<columnLower_[i]-1.0e-7) {
      numberInf++;
      sumInf-=columnActivity_[i]-columnLower_[i];
    }
  }
#if ABC_NORMAL_DEBUG>0
  if (numberInf)
    printf("Check column infeasibilities %d sum %g\n",numberInf,sumInf);
#endif
  numberInf=0;
  sumInf=0.0;
  for (int i=0;i<numberRows_;i++) {
    if (rowActivity_[i]>rowUpper_[i]+1.0e-7) {
      numberInf++;
      sumInf+=rowActivity_[i]-rowUpper_[i];
    } else if (rowActivity_[i]<rowLower_[i]-1.0e-7) {
      numberInf++;
      sumInf-=rowActivity_[i]-rowLower_[i];
    }
  }
#if ABC_NORMAL_DEBUG>0
  if (numberInf)
    printf("Check row infeasibilities %d sum %g\n",numberInf,sumInf);
#endif
  CoinAbcMemcpy(region,objective(),numberColumns_);
  ClpModel::transposeTimes(-1.0,dual_,region);
  numberInf=0;
  sumInf=0.0;
  for (int i=0;i<numberColumns_;i++) {
    if (columnUpper_[i]>columnLower_[i]) {
      if (getColumnStatus(i)==ClpSimplex::atLowerBound) {
	if (region[i]<-1.0e-7) {
	  numberInf++;
	  sumInf-=region[i];
	}
      } else if (getColumnStatus(i)==ClpSimplex::atUpperBound) {
	if (region[i]>1.0e-7) {
	  numberInf++;
	  sumInf+=region[i];
	}
      }
    }
  }
#if ABC_NORMAL_DEBUG>0
  if (numberInf)
    printf("Check column dj infeasibilities %d sum %g\n",numberInf,sumInf);
#endif
  if (checkDuals) {
    numberInf=0;
    sumInf=0.0;
    for (int i=0;i<numberRows_;i++) {
      if (rowUpper_[i]>rowLower_[i]) {
	if (getRowStatus(i)==ClpSimplex::atLowerBound) {
	  if (dual_[i]<-1.0e-7) {
	    numberInf++;
	    sumInf-=region[i];
	  }
	} else if (getRowStatus(i)==ClpSimplex::atUpperBound) {
	  if (dual_[i]>1.0e-7) {
	    numberInf++;
	    sumInf+=region[i];
	  }
	}
      }
    }
#if ABC_NORMAL_DEBUG>0
    if (numberInf)
      printf("Check row dual infeasibilities %d sum %g\n",numberInf,sumInf);
#endif
  }
  delete [] region;
}
#if 0
void xxxxxx(const char * where) {
  printf("xxxxx %s\n",where);
}
#endif
// For debug - checks solutionBasic
void 
AbcSimplex::checkSolutionBasic() const
{
  //work space
  int whichArray[2];
  for (int i=0;i<2;i++)
    whichArray[i]=getAvailableArray();
  CoinIndexedVector * arrayVector = &usefulArray_[whichArray[0]];
  double * solution = usefulArray_[whichArray[1]].denseVector();
  CoinAbcMemcpy(solution,abcSolution_,numberTotal_);
  // accumulate non basic stuff
  double * array = arrayVector->denseVector();
  CoinAbcScatterZeroTo(solution,abcPivotVariable_,numberRows_);
  abcMatrix_->timesIncludingSlacks(-1.0, solution, array);
  //arrayVector->scan(0,numberRows_,zeroTolerance_);
  // Ftran adjusted RHS
  //if (arrayVector->getNumElements())
  abcFactorization_->updateFullColumn(*arrayVector);
  CoinAbcScatterTo(array,solution,abcPivotVariable_,numberRows_);
  double largestDifference=0.0;
  int whichDifference=-1;
  for (int i=0;i<numberRows_;i++) {
    double difference = fabs(solutionBasic_[i]-array[i]);
    if (difference>1.0e-5&&numberRows_<100)
      printf("solutionBasic difference is %g on row %d solutionBasic_ %g computed %g\n",
	   difference,i,solutionBasic_[i],array[i]);
    if (difference>largestDifference) {
      largestDifference=difference;
      whichDifference=i;
    }
  }
  if (largestDifference>1.0e-9)
    printf("Debug largest solutionBasic difference is %g on row %d solutionBasic_ %g computed %g\n",
	   largestDifference,whichDifference,solutionBasic_[whichDifference],array[whichDifference]);
    arrayVector->clear();
  CoinAbcMemset0(solution,numberTotal_);
  for (int i=0;i<2;i++)
    setAvailableArray(whichArray[i]);
}

// For debug - summarizes dj situation
void 
AbcSimplex::checkDjs(int type) const
{
  if (type) {
    //work space
    int whichArrays[2];
    for (int i=0;i<2;i++)
      whichArrays[i]=getAvailableArray();
    
    CoinIndexedVector * arrayVector = &usefulArray_[whichArrays[0]];
    double * array = arrayVector->denseVector();
    int * index = arrayVector->getIndices();
    int number = 0;
    for (int iRow = 0; iRow < numberRows_; iRow++) {
      double value = costBasic_[iRow];
      if (value) {
	array[iRow] = value;
	index[number++] = iRow;
      }
    }
    arrayVector->setNumElements(number);
    // Btran basic costs 
    abcFactorization_->updateFullColumnTranspose(*arrayVector);
    double largestDifference=0.0;
    int whichDifference=-1;
    if (type==2) {
      for (int i=0;i<numberRows_;i++) {
	double difference = fabs(dual_[i]-array[i]);
	if (difference>largestDifference) {
	  largestDifference=difference;
	  whichDifference=i;
	}
      }
      if (largestDifference>1.0e-9)
	printf("Debug largest dual difference is %g on row %d dual_ %g computed %g\n",
	       largestDifference,whichDifference,dual_[whichDifference],array[whichDifference]);
    }
    // now look at djs
    double * djs=usefulArray_[whichArrays[1]].denseVector();
    CoinAbcMemcpy(djs,abcCost_,numberTotal_);
    abcMatrix_->transposeTimesNonBasic(-1.0, array,djs);
    largestDifference=0.0;
    whichDifference=-1;
    for (int i=0;i<numberTotal_;i++) {
      Status status = getInternalStatus(i);
      double difference = fabs(abcDj_[i]-djs[i]);
      switch(status) {
      case basic:
	// probably not an error
	break;
      case AbcSimplex::isFixed:
	break;
      case isFree:
      case superBasic:
      case atUpperBound:
      case atLowerBound:
	if (difference>1.0e-5&&numberTotal_<200) 
	  printf("Debug dj difference is %g on column %d abcDj_ %g computed %g\n",
		 difference,i,abcDj_[i],djs[i]);
	if (difference>largestDifference) {
	  largestDifference=difference;
	  whichDifference=i;
	}
	break;
      }
    }
    if (largestDifference>1.0e-9)
      printf("Debug largest dj difference is %g on column %d abcDj_ %g computed %g\n",
	     largestDifference,whichDifference,abcDj_[whichDifference],djs[whichDifference]);
    CoinAbcMemset0(djs,numberTotal_);
    arrayVector->clear();
    for (int i=0;i<2;i++)
      setAvailableArray(whichArrays[i]);
  }
  int state[8]={0,0,0,0,0,0,0,0};
  int badT[8]={0,0,0,0,0,0,0,0};
  //int badP[8]={0,0,0,0,0,0,0,0};
  int numberInfeasibilities=0;
  for (int i=0;i<numberTotal_;i++) {
    int iStatus = internalStatus_[i] & 7;
    state[iStatus]++;
    Status status = getInternalStatus(i);
    double djValue=abcDj_[i];
    double value=abcSolution_[i];
    double lowerValue=abcLower_[i];
    double upperValue=abcUpper_[i];
    //double perturbationValue=abcPerturbation_[i];
    switch(status) {
    case basic:
      // probably not an error
      if(fabs(djValue)>currentDualTolerance_)
	badT[iStatus]++;
      //if(fabs(djValue)>perturbationValue)
      //badP[iStatus]++;
      break;
    case AbcSimplex::isFixed:
      break;
    case isFree:
    case superBasic:
      if(fabs(djValue)>currentDualTolerance_)
	badT[iStatus]++;
      //if(fabs(djValue)>perturbationValue)
      //badP[iStatus]++;
      break;
    case atUpperBound:
      if (fabs(value - upperValue) > primalTolerance_)
	numberInfeasibilities++;
      if(djValue>currentDualTolerance_)
	badT[iStatus]++;
      //if(djValue>perturbationValue)
      //badP[iStatus]++;
      break;
    case atLowerBound:
      if (fabs(value - lowerValue) > primalTolerance_)
	numberInfeasibilities++;
      if(-djValue>currentDualTolerance_)
	badT[iStatus]++;
      //if(-djValue>perturbationValue)
      //badP[iStatus]++;
      break;
    }
  }
  if (numberInfeasibilities)
    printf("Debug %d variables away from bound when should be\n",numberInfeasibilities);
  int numberBad=0;
  for (int i=0;i<8;i++) {
    numberBad += badT[i]/*+badP[i]*/;
  }
  if (numberBad) {
    const char * type[]={"atLowerBound",
			 "atUpperBound",
			 "??",
			 "??",
			 "isFree",
			 "superBasic",
			 "basic",
			 "isFixed"};
    for (int i=0;i<8;i++) {
      if (state[i])
	printf("Debug - %s %d total %d bad with tolerance %d bad with perturbation\n",
	       type[i],state[i],badT[i],0/*badP[i]*/);
    }
  }
}
#else
// For debug - moves solution back to external and computes stuff
void 
AbcSimplex::checkMoveBack(bool )
{
}
// For debug - checks solutionBasic
void 
AbcSimplex::checkSolutionBasic() const
{
}

// For debug - summarizes dj situation
void 
AbcSimplex::checkDjs(int) const
{
}
#endif
#ifndef EARLY_FACTORIZE
#define ABC_NUMBER_USEFUL_NORMAL ABC_NUMBER_USEFUL
#else
#define ABC_NUMBER_USEFUL_NORMAL ABC_NUMBER_USEFUL-1
#endif
static double * elAddress[ABC_NUMBER_USEFUL_NORMAL];
// For debug - prints summary of arrays which are out of kilter
void 
AbcSimplex::checkArrays(int ignoreEmpty) const
{
  if (!numberIterations_||!elAddress[0]) {
    for (int i=0;i<ABC_NUMBER_USEFUL_NORMAL;i++) 
      elAddress[i]=usefulArray_[i].denseVector();
  } else {
    if(elAddress[0]!=usefulArray_[0].denseVector()) {
      printf("elAddress not zero and does not match??\n");
      for (int i=0;i<ABC_NUMBER_USEFUL_NORMAL;i++) 
	elAddress[i]=usefulArray_[i].denseVector();
    }
    for (int i=0;i<ABC_NUMBER_USEFUL_NORMAL;i++) 
      assert(elAddress[i]==usefulArray_[i].denseVector());
  }
  for (int i=0;i<ABC_NUMBER_USEFUL_NORMAL;i++) {
    int check=1<<i;
    if (usefulArray_[i].getNumElements()) {
      if ((stateOfProblem_&check)==0)
	printf("Array %d has %d elements but says it is available!\n",
	       i,usefulArray_[i].getNumElements());
      if (usefulArray_[i].getNumPartitions())
	usefulArray_[i].checkClean();
      else
	usefulArray_[i].CoinIndexedVector::checkClean();
    } else {
      if ((stateOfProblem_&check)!=0&&(ignoreEmpty&check)==0)
	printf("Array %d has no elements but says it is not available\n",i);
      if (usefulArray_[i].getNumPartitions())
	usefulArray_[i].checkClear();
      else
	usefulArray_[i].CoinIndexedVector::checkClear();
    }
  }
}
#include "CoinAbcCommon.hpp"
#if 0
#include "ClpFactorization.hpp"
// Loads tolerances etc
void 
ClpSimplex::loadTolerancesEtc(const AbcTolerancesEtc & data)
{
  zeroTolerance_ = data.zeroTolerance_;
  primalToleranceToGetOptimal_ = data.primalToleranceToGetOptimal_;
  largeValue_ = data.largeValue_;
  alphaAccuracy_ = data.alphaAccuracy_;
  dualBound_ = data.dualBound_;
  dualTolerance_ = data.dualTolerance_;
  primalTolerance_ = data.primalTolerance_;
  infeasibilityCost_ = data.infeasibilityCost_;
  incomingInfeasibility_ = data.incomingInfeasibility_;
  allowedInfeasibility_ = data.allowedInfeasibility_;
  baseIteration_ = data.baseIteration_;
  numberRefinements_ = data.numberRefinements_;
  forceFactorization_ = data.forceFactorization_;
  perturbation_ = data.perturbation_;
  dontFactorizePivots_ = data.dontFactorizePivots_;
  /// For factorization
  abcFactorization_->maximumPivots(data.maximumPivots_);
}
// Loads tolerances etc
void 
ClpSimplex::unloadTolerancesEtc(AbcTolerancesEtc & data)
{
  data.zeroTolerance_ = zeroTolerance_;
  data.primalToleranceToGetOptimal_ = primalToleranceToGetOptimal_;
  data.largeValue_ = largeValue_;
  data.alphaAccuracy_ = alphaAccuracy_;
  data.dualBound_ = dualBound_;
  data.dualTolerance_ = dualTolerance_;
  data.primalTolerance_ = primalTolerance_;
  data.infeasibilityCost_ = infeasibilityCost_;
  data.incomingInfeasibility_ = incomingInfeasibility_;
  data.allowedInfeasibility_ = allowedInfeasibility_;
  data.baseIteration_ = baseIteration_;
  data.numberRefinements_ = numberRefinements_;
  data.forceFactorization_ = forceFactorization_;
  data.perturbation_ = perturbation_;
  data.dontFactorizePivots_ = dontFactorizePivots_;
  /// For factorization
  data.maximumPivots_ = abcFactorization_->maximumPivots();
}
// Loads tolerances etc
void 
AbcSimplex::loadTolerancesEtc(const AbcTolerancesEtc & data)
{
  zeroTolerance_ = data.zeroTolerance_;
  primalToleranceToGetOptimal_ = data.primalToleranceToGetOptimal_;
  largeValue_ = data.largeValue_;
  alphaAccuracy_ = data.alphaAccuracy_;
  dualBound_ = data.dualBound_;
  dualTolerance_ = data.dualTolerance_;
  primalTolerance_ = data.primalTolerance_;
  infeasibilityCost_ = data.infeasibilityCost_;
  incomingInfeasibility_ = data.incomingInfeasibility_;
  allowedInfeasibility_ = data.allowedInfeasibility_;
  baseIteration_ = data.baseIteration_;
  numberRefinements_ = data.numberRefinements_;
  forceFactorization_ = data.forceFactorization_;
  perturbation_ = data.perturbation_;
  dontFactorizePivots_ = data.dontFactorizePivots_;
  /// For factorization
  abcFactorization_->maximumPivots(data.maximumPivots_);
}
// Loads tolerances etc
void 
AbcSimplex::unloadTolerancesEtc(AbcTolerancesEtc & data)
{
  data.zeroTolerance_ = zeroTolerance_;
  data.primalToleranceToGetOptimal_ = primalToleranceToGetOptimal_;
  data.largeValue_ = largeValue_;
  data.alphaAccuracy_ = alphaAccuracy_;
  data.dualBound_ = dualBound_;
  data.dualTolerance_ = dualTolerance_;
  data.primalTolerance_ = primalTolerance_;
  data.infeasibilityCost_ = infeasibilityCost_;
  data.incomingInfeasibility_ = incomingInfeasibility_;
  data.allowedInfeasibility_ = allowedInfeasibility_;
  data.baseIteration_ = baseIteration_;
  data.numberRefinements_ = numberRefinements_;
  data.forceFactorization_ = forceFactorization_;
  data.perturbation_ = perturbation_;
  data.dontFactorizePivots_ = dontFactorizePivots_;
  /// For factorization
  data.maximumPivots_ = abcFactorization_->maximumPivots();
}
#endif
// Swaps two variables (for now just updates basic list) and sets status
void 
AbcSimplex::swap(int pivotRow,int nonBasicPosition,Status newStatus)
{
  // set status
  setInternalStatus(nonBasicPosition,newStatus);
  solutionBasic_[pivotRow]=abcSolution_[nonBasicPosition];
  lowerBasic_[pivotRow]=abcLower_[nonBasicPosition];
  upperBasic_[pivotRow]=abcUpper_[nonBasicPosition];
  costBasic_[pivotRow]=abcCost_[nonBasicPosition];
}
// Swaps two variables (for now just updates basic list)
void 
AbcSimplex::swap(int pivotRow,int nonBasicPosition)
{
  solutionBasic_[pivotRow]=abcSolution_[nonBasicPosition];
  lowerBasic_[pivotRow]=lowerSaved_[nonBasicPosition];
  upperBasic_[pivotRow]=upperSaved_[nonBasicPosition];
  costBasic_[pivotRow]=abcCost_[nonBasicPosition];
}
// Move status and solution to ClpSimplex
void 
AbcSimplex::moveStatusToClp(ClpSimplex * clpModel)
{
  assert (clpModel);
  if (algorithm_<0)
    clpModel->setObjectiveValue(clpObjectiveValue());
  else
    clpModel->setObjectiveValue(objectiveValue());
  clpModel->setProblemStatus(problemStatus_);
  clpModel->setSecondaryStatus(secondaryStatus_);
  clpModel->setNumberIterations(numberIterations_);
  clpModel->setSumDualInfeasibilities(sumDualInfeasibilities_);
  clpModel->setSumOfRelaxedDualInfeasibilities(sumOfRelaxedDualInfeasibilities_);
  clpModel->setNumberDualInfeasibilities(numberDualInfeasibilities_);
  clpModel->setSumPrimalInfeasibilities(sumPrimalInfeasibilities_);
  clpModel->setSumOfRelaxedPrimalInfeasibilities(sumOfRelaxedPrimalInfeasibilities_);
  clpModel->setNumberPrimalInfeasibilities(numberPrimalInfeasibilities_);
  permuteOut(ROW_PRIMAL_OK|ROW_DUAL_OK|COLUMN_PRIMAL_OK|COLUMN_DUAL_OK|ALL_STATUS_OK);
  CoinAbcMemcpy(clpModel->primalColumnSolution(),primalColumnSolution(),numberColumns_);
  CoinAbcMemcpy(clpModel->dualColumnSolution(),dualColumnSolution(),numberColumns_);
  CoinAbcMemcpy(clpModel->primalRowSolution(),primalRowSolution(),numberRows_);
  CoinAbcMemcpy(clpModel->dualRowSolution(),dualRowSolution(),numberRows_);
  CoinAbcMemcpy(clpModel->statusArray(),statusArray(),numberTotal_);
}
// Move status and solution from ClpSimplex
void 
AbcSimplex::moveStatusFromClp(ClpSimplex * clpModel)
{
  assert (clpModel);
  problemStatus_=clpModel->problemStatus();
  secondaryStatus_=clpModel->secondaryStatus();
  numberIterations_=clpModel->numberIterations();
  sumDualInfeasibilities_ = clpModel->sumDualInfeasibilities();
  sumOfRelaxedDualInfeasibilities_ = clpModel->sumOfRelaxedDualInfeasibilities();
  numberDualInfeasibilities_ = clpModel->numberDualInfeasibilities();
  sumPrimalInfeasibilities_ = clpModel->sumPrimalInfeasibilities();
  sumOfRelaxedPrimalInfeasibilities_ = clpModel->sumOfRelaxedPrimalInfeasibilities();
  numberPrimalInfeasibilities_ = clpModel->numberPrimalInfeasibilities();
  CoinAbcMemcpy(primalColumnSolution(),clpModel->primalColumnSolution(),numberColumns_);
  CoinAbcMemcpy(dualColumnSolution(),clpModel->dualColumnSolution(),numberColumns_);
  CoinAbcMemcpy(primalRowSolution(),clpModel->primalRowSolution(),numberRows_);
  CoinAbcMemcpy(dualRowSolution(),clpModel->dualRowSolution(),numberRows_);
  CoinAbcMemcpy(statusArray(),clpModel->statusArray(),numberTotal_);
  translate(DO_SCALE_AND_MATRIX|DO_BASIS_AND_ORDER|DO_STATUS|DO_SOLUTION);
}
// Clears an array and says available (-1 does all)
void 
AbcSimplex::clearArrays(int which)
{
  if (which>=0) {
    if (usefulArray_[which].getNumElements())
      usefulArray_[which].clear();
    int check=1<<which;
    stateOfProblem_&= ~check;
  } else if (which==-1) {
    for (int i=0;i<ABC_NUMBER_USEFUL_NORMAL;i++) {
      if (usefulArray_[i].getNumElements()) {
	usefulArray_[i].clear();
	usefulArray_[i].clearAndReset();
      }
    }
    stateOfProblem_&= ~255;
  } else {
    for (int i=0;i<ABC_NUMBER_USEFUL_NORMAL;i++) {
      usefulArray_[i].clearAndReset();
    }
    stateOfProblem_&= ~255;
  }
}
// Clears an array and says available
void 
AbcSimplex::clearArrays(CoinPartitionedVector * which)
{
  int check=1;
  int i;
  for (i=0;i<ABC_NUMBER_USEFUL_NORMAL;i++) {
    if (&usefulArray_[i]==which) {
      usefulArray_[i].clear();
      stateOfProblem_&= ~check;
      break;
    }
    check=check<<1;
  }
  assert (i<ABC_NUMBER_USEFUL_NORMAL);
}
// set multiple sequence in
void 
AbcSimplex::setMultipleSequenceIn(int sequenceIn[4])
{
  memcpy(multipleSequenceIn_,sequenceIn,sizeof(multipleSequenceIn_));
}
// Returns first available empty array (and sets flag)
int 
AbcSimplex::getAvailableArray() const
{
  int which;
  int status=1;
  for (which=0;which<ABC_NUMBER_USEFUL_NORMAL;which++) {
    if ((stateOfProblem_&status)==0)
      break;
    status=status<<1;
  }
  assert (which<ABC_NUMBER_USEFUL_NORMAL);
  assert (!usefulArray_[which].getNumElements());
  stateOfProblem_ |= status;
  return which;
}
bool 
AbcSimplex::atFakeBound(int sequence) const
{
  FakeBound status=getFakeBound(sequence);
  bool atBound=false;
  switch (status) {
  case noFake:
    break;
  case lowerFake:
    atBound = (getInternalStatus(sequence)==atLowerBound);
    break;
  case upperFake:
    atBound = (getInternalStatus(sequence)==atUpperBound);
    break;
  case bothFake:
    atBound=true;
    break;
  }
  return atBound;
}
AbcSimplexProgress::AbcSimplexProgress () :
  ClpSimplexProgress()
{
}

//-----------------------------------------------------------------------------

AbcSimplexProgress::~AbcSimplexProgress ()
{
}
// Copy constructor from model
AbcSimplexProgress::AbcSimplexProgress(ClpSimplex * rhs) :
  ClpSimplexProgress(rhs)
{
}
// Assignment operator. This copies the data
AbcSimplexProgress &
AbcSimplexProgress::operator=(const AbcSimplexProgress & rhs)
{
  if (this != &rhs) {
    ClpSimplexProgress::operator=(rhs);
  }
  return *this;
}
int
AbcSimplexProgress::looping()
{
  if (!model_)
    return -1;
#ifdef ABC_INHERIT
  AbcSimplex * model = model_->abcSimplex();
#else
  AbcSimplex * model;
  return -1;
#endif
  double objective;
  if (model_->algorithm() < 0) {
    objective = model_->rawObjectiveValue();
    objective -= model_->bestPossibleImprovement();
  } else {
    objective = model->abcNonLinearCost()->feasibleReportCost();
  }
  double infeasibility;
  double realInfeasibility = 0.0;
  int numberInfeasibilities;
  int iterationNumber = model->numberIterations();
  //numberTimesFlagged_ = 0;
  if (model->algorithm() < 0) {
    // dual
    infeasibility = model->sumPrimalInfeasibilities();
    numberInfeasibilities = model->numberPrimalInfeasibilities();
  } else {
    //primal
    infeasibility = model->sumDualInfeasibilities();
    realInfeasibility = model->abcNonLinearCost()->sumInfeasibilities();
    numberInfeasibilities = model->numberDualInfeasibilities();
  }
  int i;
  int numberMatched = 0;
  int matched = 0;
  int nsame = 0;
  for (i = 0; i < CLP_PROGRESS; i++) {
    bool matchedOnObjective = objective == objective_[i];
    bool matchedOnInfeasibility = infeasibility == infeasibility_[i];
    bool matchedOnInfeasibilities =
      (numberInfeasibilities == numberInfeasibilities_[i]);
    
    if (matchedOnObjective && matchedOnInfeasibility && matchedOnInfeasibilities) {
      matched |= (1 << i);
      // Check not same iteration
      if (iterationNumber != iterationNumber_[i]) {
	numberMatched++;
#if ABC_NORMAL_DEBUG>0
	// here mainly to get over compiler bug?
	if (model->messageHandler()->logLevel() > 10)
	  printf("%d %d %d %d %d loop check\n", i, numberMatched,
		 matchedOnObjective, matchedOnInfeasibility,
		 matchedOnInfeasibilities);
#endif
      } else {
	// stuck but code should notice
	nsame++;
      }
    }
    if (i) {
      objective_[i-1] = objective_[i];
      infeasibility_[i-1] = infeasibility_[i];
      realInfeasibility_[i-1] = realInfeasibility_[i];
      numberInfeasibilities_[i-1] = numberInfeasibilities_[i];
      iterationNumber_[i-1] = iterationNumber_[i];
    }
  }
  objective_[CLP_PROGRESS-1] = objective;
  infeasibility_[CLP_PROGRESS-1] = infeasibility;
  realInfeasibility_[CLP_PROGRESS-1] = realInfeasibility;
  numberInfeasibilities_[CLP_PROGRESS-1] = numberInfeasibilities;
  iterationNumber_[CLP_PROGRESS-1] = iterationNumber;
  if (nsame == CLP_PROGRESS)
    numberMatched = CLP_PROGRESS; // really stuck
  if (model->progressFlag())
    numberMatched = 0;
  numberTimes_++;
  if (numberTimes_ < 10)
    numberMatched = 0;
  // skip if just last time as may be checking something
  if (matched == (1 << (CLP_PROGRESS - 1)))
    numberMatched = 0;
  if (numberMatched) {
    model->messageHandler()->message(CLP_POSSIBLELOOP, model->messages())
      << numberMatched
      << matched
      << numberTimes_
      << CoinMessageEol;
    printf("loop detected %d times out of %d\n",numberBadTimes_,numberTimes_);
    numberBadTimes_++;
    if (numberBadTimes_ < 10) {
      // make factorize every iteration
      model->forceFactorization(1);
      if (numberBadTimes_ < 2) {
	startCheck(); // clear other loop check
	if (model->algorithm() < 0) {
	  // dual - change tolerance
	  model->setCurrentDualTolerance(model->currentDualTolerance() * 1.05);
	  // if infeasible increase dual bound
	  if (model->currentDualBound() < 1.0e17) {
	    model->setDualBound(model->currentDualBound() * 1.1);
	    static_cast<AbcSimplexDual *>(model)->bounceTolerances(100);
	  }
	} else {
	  abort();
	  // primal - change tolerance
	  if (numberBadTimes_ > 3)
	    model->setCurrentPrimalTolerance(model->currentPrimalTolerance() * 1.05);
	  // if infeasible increase infeasibility cost
	  //if (model->nonLinearCost()->numberInfeasibilities() &&
	  //        model->infeasibilityCost() < 1.0e17) {
	  //   model->setInfeasibilityCost(model->infeasibilityCost() * 1.1);
	  //}
	}
      } else {
	// flag
	int iSequence;
	if (model->algorithm() < 0) {
	  // dual
	  if (model->currentDualBound() > 1.0e14)
	    model->setDualBound(1.0e14);
	  iSequence = in_[CLP_CYCLE-1];
	} else {
	  // primal
	  if (model->infeasibilityCost() > 1.0e14)
	    model->setInfeasibilityCost(1.0e14);
	  iSequence = out_[CLP_CYCLE-1];
	}
	if (iSequence >= 0) {
	  char x = model->isColumn(iSequence) ? 'C' : 'R';
	  if (model->messageHandler()->logLevel() >= 63)
	    model->messageHandler()->message(CLP_SIMPLEX_FLAG, model->messages())
	      << x << model->sequenceWithin(iSequence)
	      << CoinMessageEol;
	  // if Gub then needs to be sequenceIn_
	  int save = model->sequenceIn();
	  model->setSequenceIn(iSequence);
	  model->setFlagged(iSequence);
	  model->setLastBadIteration(model->numberIterations());
	  model->setSequenceIn(save);
	  //printf("flagging %d from loop\n",iSequence);
	  startCheck();
	} else {
	  // Give up
#if ABC_NORMAL_DEBUG>0
	  if (model->messageHandler()->logLevel() >= 63)
	    printf("***** All flagged?\n");
#endif
	  return 4;
	}
	// reset
	numberBadTimes_ = 2;
      }
      return -2;
    } else {
      // look at solution and maybe declare victory
      if (infeasibility < 1.0e-4) {
	return 0;
      } else {
	model->messageHandler()->message(CLP_LOOP, model->messages())
	  << CoinMessageEol;
#ifndef NDEBUG
	printf("debug loop AbcSimplex A\n");
	abort();
#endif
	return 3;
      }
    }
  }
  return -1;
}
#if 0
void
AbcSimplex::loadProblem (  const CoinPackedMatrix& matrix,
                           const double* collb, const double* colub,
                           const double* obj,
                           const double* rowlb, const double* rowub,
                           const double * rowObjective)
{
  ClpSimplex::loadProblem(matrix, collb, colub, obj, rowlb, rowub,
			rowObjective);
  translate(DO_SCALE_AND_MATRIX|DO_BASIS_AND_ORDER|DO_STATUS|DO_SOLUTION);
}
#endif
// For debug - check pivotVariable consistent
void 
AbcSimplex::checkConsistentPivots() const
{
  unsigned char * copyStatus = CoinCopyOfArray(internalStatus_,numberTotal_);
  int nBad=0;
  for (int i=0;i<numberRows_;i++) {
    int k=abcPivotVariable_[i];
    if (k>=0&&k<numberTotal_) {
      if ((copyStatus[k]&7)!=6) {
	nBad++;
	printf("%d pivoting on row %d is not basic - status %d\n",k,i,copyStatus[k]&7);
      }
      copyStatus[k]=16;
    } else {
      nBad++;
      printf("%d pivoting on row %d is bad\n",k,i);
    }
  }
  for (int i=0;i<numberTotal_;i++) {
    if ((copyStatus[i]&7)==6) {
      nBad++;
      printf("%d without pivot is basic\n",i);
    }
  }
  assert (!nBad);
  delete [] copyStatus;
}
// Print stuff
void 
AbcSimplex::printStuff() const
{
  if (numberIterations_!=2821)
  return;
  printf("XXStart\n");
  for (int i=0;i<this->numberTotal();i++) {
    if (this->getInternalStatus(i)!=AbcSimplex::basic)
      printf("%d status %d primal %g dual %g lb %g ub %g\n",
	     i,this->getInternalStatus(i),this->solutionRegion()[i],
	     this->djRegion()[i],this->lowerRegion()[i],this->upperRegion()[i]);
  }
  for (int i=0;i<this->numberRows();i++) {
    printf("%d %g <= %g <= %g -pivot %d cost %g\n",
	   i,this->lowerBasic()[i],this->solutionBasic()[i],
	   this->upperBasic()[i],this->pivotVariable()[i],this->costBasic()[i]);
  }
  printf("XXend\n");
}
#if ABC_PARALLEL==1
// so thread can find out which one it is 
int 
AbcSimplex::whichThread() const
{
  pthread_t thisThread=pthread_self();
  int whichThread;
  for (whichThread=0;whichThread<NUMBER_THREADS;whichThread++) {
    if (pthread_equal(thisThread,abcThread_[whichThread]))
      break;
  }
  assert (whichThread<NUMBER_THREADS);
  return whichThread;
}
#endif
