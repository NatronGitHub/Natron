/* $Id$ */
// Copyright (C) 2002, International Business Machines
// Corporation and others, Copyright (C) 2012, FasterCoin.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).


#include "CoinPragma.hpp"
#include "AbcSimplex.hpp"
#include "AbcDualRowDantzig.hpp"
#include "AbcSimplexFactorization.hpp"
#include "CoinIndexedVector.hpp"
#include "CoinHelperFunctions.hpp"
#ifndef CLP_DUAL_COLUMN_MULTIPLIER
#define CLP_DUAL_COLUMN_MULTIPLIER 1.01
#endif

//#############################################################################
// Constructors / Destructor / Assignment
//#############################################################################

//-------------------------------------------------------------------
// Default Constructor
//-------------------------------------------------------------------
AbcDualRowDantzig::AbcDualRowDantzig ()
  : AbcDualRowPivot(),
    infeasible_(NULL)
{
  type_ = 1;
}

//-------------------------------------------------------------------
// Copy constructor
//-------------------------------------------------------------------
AbcDualRowDantzig::AbcDualRowDantzig (const AbcDualRowDantzig & rhs)
  : AbcDualRowPivot(rhs),
    infeasible_(NULL)
{
  model_ = rhs.model_;
  if ((model_ && model_->whatsChanged() & 1) != 0) {
    if (rhs.infeasible_) {
      infeasible_ = new CoinIndexedVector(rhs.infeasible_);
    } else {
      infeasible_ = NULL;
    }
  }  
}

//-------------------------------------------------------------------
// Destructor
//-------------------------------------------------------------------
AbcDualRowDantzig::~AbcDualRowDantzig ()
{
  delete infeasible_;
}

//----------------------------------------------------------------
// Assignment operator
//-------------------------------------------------------------------
AbcDualRowDantzig &
AbcDualRowDantzig::operator=(const AbcDualRowDantzig& rhs)
{
  if (this != &rhs) {
    AbcDualRowPivot::operator=(rhs);
    delete infeasible_;
    if (rhs.infeasible_ != NULL) {
      infeasible_ = new CoinIndexedVector(rhs.infeasible_);
    } else {
      infeasible_ = NULL;
    }
  }
  return *this;
}

/* 
   1) before factorization
   2) after factorization
   3) just redo infeasibilities
   4) restore weights
*/
void
AbcDualRowDantzig::saveWeights(AbcSimplex * model, int mode)
{
  model_ = model;
  int numberRows = model_->numberRows();
  if (mode == 1) {
    // Check if size has changed
    if (infeasible_&&infeasible_->capacity() != numberRows) {
      // size has changed - clear everything
      delete infeasible_;
      infeasible_ = NULL;
    }
  } else if (mode !=3 && !infeasible_) {
      infeasible_ = new CoinIndexedVector();
      infeasible_->reserve(numberRows);
  }
  if (mode >= 2) {
    recomputeInfeasibilities();
  }
}
// Recompute infeasibilities
void 
AbcDualRowDantzig::recomputeInfeasibilities()
{
  int numberRows = model_->numberRows();
  infeasible_->clear();
  double tolerance = model_->currentPrimalTolerance();
  const double *  COIN_RESTRICT solutionBasic = model_->solutionBasic();
  const double *  COIN_RESTRICT lowerBasic = model_->lowerBasic();
  const double *  COIN_RESTRICT upperBasic = model_->upperBasic();
  for (int iRow = 0; iRow < numberRows; iRow++) {
    double value = solutionBasic[iRow];
    double lower = lowerBasic[iRow];
    double upper = upperBasic[iRow];
    if (value < lower - tolerance) {
      value -= lower;
#ifdef CLP_DUAL_FIXED_COLUMN_MULTIPLIER
      if (lower == upper)
	value *= CLP_DUAL_FIXED_COLUMN_MULTIPLIER; // bias towards taking out fixed variables
#endif
      // store in list
      infeasible_->quickAdd(iRow, fabs(value));
    } else if (value > upper + tolerance) {
      value -= upper;
#ifdef CLP_DUAL_FIXED_COLUMN_MULTIPLIER
      if (lower == upper)
	value *= CLP_DUAL_FIXED_COLUMN_MULTIPLIER; // bias towards taking out fixed variables
#endif
      // store in list
      infeasible_->quickAdd(iRow, fabs(value));
    }
  }
}
#if ABC_PARALLEL==2
static void choose(CoinIndexedVector * infeasible,
		   int & chosenRowSave,double & largestSave, int first, int last,
		   double tolerance)
{
  if (last-first>256) {
    int mid=(last+first)>>1;
    int chosenRow2=chosenRowSave;
    double largest2=largestSave;
    cilk_spawn choose(infeasible,chosenRow2,largest2, first, mid,
		      tolerance);
    choose(infeasible,chosenRowSave,largestSave, mid, last,
	   tolerance);
    cilk_sync;
    if (largest2>largestSave) {
      largestSave=largest2;
      chosenRowSave=chosenRow2;
    }
  } else {
    const int * index=infeasible->getIndices();
    const double * infeas=infeasible->denseVector();
    double largest=largestSave;
    int chosenRow=chosenRowSave;
    for (int i = first; i < last; i++) {
      int iRow = index[i];
      double value = infeas[iRow];
      if (value > largest) {
	largest=value;
	chosenRow=iRow;
      }
    }
    chosenRowSave=chosenRow;
    largestSave=largest;
  }
}
#endif
// Returns pivot row, -1 if none
int
AbcDualRowDantzig::pivotRow()
{
  assert(model_);
  double *  COIN_RESTRICT infeas = infeasible_->denseVector();
  int *  COIN_RESTRICT index = infeasible_->getIndices();
  int number = infeasible_->getNumElements();
  double tolerance = model_->currentPrimalTolerance();
  // we can't really trust infeasibilities if there is primal error
  if (model_->largestPrimalError() > 1.0e-8)
    tolerance *= model_->largestPrimalError() / 1.0e-8;
  int numberRows = model_->numberRows();
  const double *  COIN_RESTRICT solutionBasic=model_->solutionBasic();
  const double *  COIN_RESTRICT lowerBasic=model_->lowerBasic();
  const double *  COIN_RESTRICT upperBasic=model_->upperBasic();
  const int * pivotVariable = model_->pivotVariable();
  // code so has list of infeasibilities (like steepest)
  int numberWanted=CoinMax(2000,numberRows>>4);
  numberWanted=CoinMax(numberWanted,number>>2);
  if (model_->largestPrimalError() > 1.0e-3)
    numberWanted = number + 1; // be safe
  // Setup two passes
  int start[4];
  start[1] = number;
  start[2] = 0;
  double dstart = static_cast<double> (number) * model_->randomNumberGenerator()->randomDouble();
  start[0] = static_cast<int> (dstart);
  start[3] = start[0];
  double largest = tolerance;
  int chosenRow = -1;
  int saveNumberWanted=numberWanted;
#ifdef DO_REDUCE
  bool doReduce=true;
  int lastChosen=-1;
  double lastLargest=0.0;
#endif
  for (int iPass = 0; iPass < 2; iPass++) {
    int endThis = start[2*iPass+1];
    int startThis=start[2*iPass];
    while (startThis<endThis) {
      int end = CoinMin(endThis,startThis+numberWanted);
#ifdef DO_REDUCE 
      if (doReduce) {
	choose(infeasible,chosenRow,largest,startThis,end,tolerance);
	if (chosenRow!=lastChosen) {
	  assert (chosenRow>=0);
	  if (model_->flagged(pivotVariable[chosenRow])||
	      (solutionBasic[chosenRow] <= upperBasic[chosenRow] + tolerance &&
	       solutionBasic[chosenRow] >= lowerBasic[chosenRow] - tolerance)) {
	    doReduce=false;
	    chosenRow=lastChosen;
	    largest=lastLargest;
	  } else {
	    lastChosen=chosenRow;
	    lastLargest=largest;
	  }
	}
      }
      if (!doReduce) {
#endif
      for (int i = startThis; i < end; i++) {
	int iRow = index[i];
	double value = infeas[iRow];
	if (value > largest) {
	  if (!model_->flagged(pivotVariable[iRow])) {
	    if (solutionBasic[iRow] > upperBasic[iRow] + tolerance ||
		solutionBasic[iRow] < lowerBasic[iRow] - tolerance) {
	      chosenRow = iRow;
	      largest = value;
	    }
	  }
	}
      }
#ifdef DO_REDUCE
      }
#endif
      numberWanted-=(end-startThis);
      if (!numberWanted) {
	if(chosenRow>=0)
	  break;
	else
	  numberWanted=(saveNumberWanted+1)>>1;
      }
      startThis=end;
    }
    if (!numberWanted) {
      if(chosenRow>=0)
	break;
      else
	numberWanted=(saveNumberWanted+1)>>1;
    }
  }
  return chosenRow;
}
// FT update and returns pivot alpha
double
AbcDualRowDantzig::updateWeights(CoinIndexedVector & input,CoinIndexedVector & updatedColumn)
{
  // Do FT update
  model_->factorization()->updateColumnFT(updatedColumn);
  // pivot element
  double alpha = 0.0;
  // look at updated column
  double * work = updatedColumn.denseVector();
  int pivotRow = model_->lastPivotRow();
  assert (pivotRow == model_->pivotRow());
  
  assert (!updatedColumn.packedMode());
  alpha = work[pivotRow];
  return alpha;
}
double 
AbcDualRowDantzig::updateWeights1(CoinIndexedVector & input,CoinIndexedVector & updateColumn)
{
  return updateWeights(input,updateColumn);
}
#if ABC_PARALLEL==2
static void update(int first, int last,
		   const int * COIN_RESTRICT which, double * COIN_RESTRICT work, 
		   const double * COIN_RESTRICT lowerBasic,double * COIN_RESTRICT solutionBasic,
		   const double * COIN_RESTRICT upperBasic,double theta,double tolerance)
{
  if (last-first>256) {
    int mid=(last+first)>>1;
    cilk_spawn update(first,mid,which,work,lowerBasic,solutionBasic,
		      upperBasic,theta,tolerance);
    update(mid,last,which,work,lowerBasic,solutionBasic,
	   upperBasic,theta,tolerance);
    cilk_sync;
  } else {
    for (int i = first; i < last; i++) {
      int iRow = which[i];
      double updateValue = work[iRow];
      double value = solutionBasic[iRow];
      double change = theta * updateValue;
      value -= change;
      double lower = lowerBasic[iRow];
      double upper = upperBasic[iRow];
      solutionBasic[iRow] = value;
      if (value < lower - tolerance) {
	value -= lower;
#ifdef CLP_DUAL_FIXED_COLUMN_MULTIPLIER
	if (lower == upper)
	  value *= CLP_DUAL_FIXED_COLUMN_MULTIPLIER; // bias towards taking out fixed variables
#endif
      } else if (value > upper + tolerance) {
	value -= upper;
#ifdef CLP_DUAL_FIXED_COLUMN_MULTIPLIER
	if (lower == upper)
	  value *= CLP_DUAL_FIXED_COLUMN_MULTIPLIER; // bias towards taking out fixed variables
#endif
      } else {
	// feasible
	value=0.0;
      }
      // store
      work[iRow]=fabs(value);
    }
  }
}
#endif
/* Updates primal solution (and maybe list of candidates)
   Uses input vector which it deletes
   Computes change in objective function
*/
void
AbcDualRowDantzig::updatePrimalSolution(CoinIndexedVector & primalUpdate,
                                        double theta)
{
  double *  COIN_RESTRICT work = primalUpdate.denseVector();
  int numberNonZero = primalUpdate.getNumElements();
  int * which = primalUpdate.getIndices();
  double tolerance = model_->currentPrimalTolerance();
  double * COIN_RESTRICT infeas = infeasible_->denseVector();
  double * COIN_RESTRICT solutionBasic = model_->solutionBasic();
  const double * COIN_RESTRICT lowerBasic = model_->lowerBasic();
  const double * COIN_RESTRICT upperBasic = model_->upperBasic();
  assert (!primalUpdate.packedMode());
#if 0 //ABC_PARALLEL==2
  update(0,numberNonZero,which,work,
	 lowerBasic,solutionBasic,upperBasic,
	 theta,tolerance);
  for (int i = 0; i < numberNonZero; i++) {
    int iRow = which[i];
    double infeasibility=work[iRow];
    work[iRow]=0.0;
    if (infeasibility) {
      if (infeas[iRow])
	infeas[iRow] = infeasibility; // already there
      else
	infeasible_->quickAdd(iRow, infeasibility);
    } else {
      // feasible - was it infeasible - if so set tiny
      if (infeas[iRow])
	infeas[iRow] = COIN_INDEXED_REALLY_TINY_ELEMENT;
    }
  }
#else
  for (int i = 0; i < numberNonZero; i++) {
    int iRow = which[i];
    double updateValue = work[iRow];
    work[iRow]=0.0;
    double value = solutionBasic[iRow];
    double change = theta * updateValue;
    value -= change;
    double lower = lowerBasic[iRow];
    double upper = upperBasic[iRow];
    solutionBasic[iRow] = value;
    if (value < lower - tolerance) {
      value -= lower;
#ifdef CLP_DUAL_FIXED_COLUMN_MULTIPLIER
      if (lower == upper)
	value *= CLP_DUAL_FIXED_COLUMN_MULTIPLIER; // bias towards taking out fixed variables
#endif
      // store in list
      if (infeas[iRow])
	infeas[iRow] = fabs(value); // already there
      else
	infeasible_->quickAdd(iRow, fabs(value));
    } else if (value > upper + tolerance) {
      value -= upper;
#ifdef CLP_DUAL_FIXED_COLUMN_MULTIPLIER
      if (lower == upper)
	value *= CLP_DUAL_FIXED_COLUMN_MULTIPLIER; // bias towards taking out fixed variables
#endif
      // store in list
      if (infeas[iRow])
	infeas[iRow] = fabs(value); // already there
      else
	infeasible_->quickAdd(iRow, fabs(value));
    } else {
      // feasible - was it infeasible - if so set tiny
      if (infeas[iRow])
	infeas[iRow] = COIN_INDEXED_REALLY_TINY_ELEMENT;
    }
  }
#endif
  primalUpdate.setNumElements(0);
}
//-------------------------------------------------------------------
// Clone
//-------------------------------------------------------------------
AbcDualRowPivot * AbcDualRowDantzig::clone(bool CopyData) const
{
  if (CopyData) {
    return new AbcDualRowDantzig(*this);
  } else {
    return new AbcDualRowDantzig();
  }
}
