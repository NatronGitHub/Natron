/* $Id$ */
// Copyright (C) 2002, International Business Machines
// Corporation and others, Copyright (C) 2012, FasterCoin.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include "CoinPragma.hpp"
#include "AbcSimplex.hpp"
#include "ClpMessage.hpp"
#include "AbcDualRowSteepest.hpp"
#include "CoinIndexedVector.hpp"
#include "AbcSimplexFactorization.hpp"
#include "CoinHelperFunctions.hpp"
#include "CoinAbcHelperFunctions.hpp"
#include <cstdio>
//#############################################################################
// Constructors / Destructor / Assignment
//#############################################################################
//#define CLP_DEBUG 2
//-------------------------------------------------------------------
// Default Constructor
//-------------------------------------------------------------------
AbcDualRowSteepest::AbcDualRowSteepest (int mode)
  : AbcDualRowPivot(),
    norm_(0.0),
    factorizationRatio_(10.0),
    state_(-1),
    mode_(mode),
    persistence_(normal),
    weights_(NULL),
    infeasible_(NULL),
    savedWeights_(NULL)
{
  type_ = 2 + 64 * mode;
  //printf("XX %x AbcDualRowSteepest constructor\n",this);
}

//-------------------------------------------------------------------
// Copy constructor
//-------------------------------------------------------------------
AbcDualRowSteepest::AbcDualRowSteepest (const AbcDualRowSteepest & rhs)
  : AbcDualRowPivot(rhs)
{
  //printf("XX %x AbcDualRowSteepest constructor from %x\n",this,&rhs);
  norm_ = rhs.norm_;
  factorizationRatio_ = rhs.factorizationRatio_;
  state_ = rhs.state_;
  mode_ = rhs.mode_;
  persistence_ = rhs.persistence_;
  model_ = rhs.model_;
  if ((model_ && model_->whatsChanged() & 1) != 0) {
    int number = model_->numberRows();
    if (rhs.savedWeights_)
      number = CoinMin(number, rhs.savedWeights_->capacity());
    if (rhs.infeasible_) {
      infeasible_ = new CoinIndexedVector(rhs.infeasible_);
    } else {
      infeasible_ = NULL;
    }
    if (rhs.weights_) {
      weights_ = new CoinIndexedVector(rhs.weights_);
    } else {
      weights_ = NULL;
    }
    if (rhs.savedWeights_) {
      savedWeights_ = new CoinIndexedVector(rhs.savedWeights_);
    } else {
      savedWeights_ = NULL;
    }
  } else {
    infeasible_ = NULL;
    weights_ = NULL;
    savedWeights_ = NULL;
  }
}

//-------------------------------------------------------------------
// Destructor
//-------------------------------------------------------------------
AbcDualRowSteepest::~AbcDualRowSteepest ()
{
  //printf("XX %x AbcDualRowSteepest destructor\n",this);
  delete weights_;
  delete infeasible_;
  delete savedWeights_;
}

//----------------------------------------------------------------
// Assignment operator
//-------------------------------------------------------------------
AbcDualRowSteepest &
AbcDualRowSteepest::operator=(const AbcDualRowSteepest& rhs)
{
  if (this != &rhs) {
    AbcDualRowPivot::operator=(rhs);
    norm_ = rhs.norm_;
    factorizationRatio_ = rhs.factorizationRatio_;
    state_ = rhs.state_;
    mode_ = rhs.mode_;
    persistence_ = rhs.persistence_;
    model_ = rhs.model_;
    delete weights_;
    delete infeasible_;
    delete savedWeights_;
    assert(model_);
    int number = model_->numberRows();
    if (rhs.savedWeights_)
      number = CoinMin(number, rhs.savedWeights_->capacity());
    if (rhs.infeasible_ != NULL) {
      infeasible_ = new CoinIndexedVector(rhs.infeasible_);
    } else {
      infeasible_ = NULL;
    }
    if (rhs.weights_ != NULL) {
      weights_ = new CoinIndexedVector(rhs.weights_);
    } else {
      weights_ = NULL;
    }
    if (rhs.savedWeights_ != NULL) {
      savedWeights_ = new CoinIndexedVector(rhs.savedWeights_);
    } else {
      savedWeights_ = NULL;
    }
  }
  return *this;
}
// Fill most values
void
AbcDualRowSteepest::fill(const AbcDualRowSteepest& rhs)
{
  norm_ = rhs.norm_;
  factorizationRatio_ = rhs.factorizationRatio_;
  state_ = rhs.state_;
  mode_ = rhs.mode_;
  persistence_ = rhs.persistence_;
  assert (model_->numberRows() == rhs.model_->numberRows());
  model_ = rhs.model_;
  assert(model_);
  int number = model_->numberRows();
  if (rhs.savedWeights_)
    number = CoinMin(number, rhs.savedWeights_->capacity());
  if (rhs.infeasible_ != NULL) {
    if (!infeasible_)
      infeasible_ = new CoinIndexedVector(rhs.infeasible_);
    else
      *infeasible_ = *rhs.infeasible_;
  } else {
    delete infeasible_;
    infeasible_ = NULL;
  }
  if (rhs.weights_ != NULL) {
    if (!weights_)
      weights_ = new CoinIndexedVector(rhs.weights_);
  } else {
    delete weights_;
    weights_ = NULL;
  }
  if (rhs.savedWeights_ != NULL) {
    if (!savedWeights_)
      savedWeights_ = new CoinIndexedVector(rhs.savedWeights_);
    else
      *savedWeights_ = *rhs.savedWeights_;
  } else {
    delete savedWeights_;
    savedWeights_ = NULL;
  }
}
#ifdef CHECK_NUMBER_WANTED
static int xTimes=0;
static int xWanted=0;
#endif
#if ABC_PARALLEL==2
#define DO_REDUCE 2
#ifdef DO_REDUCE
#if DO_REDUCE==1
#include <cilk/reducer_max.h>
static void choose(int & chosenRow,double & largest, int n,
		   const int * index, const double * infeas,
		   const double * weights,double tolerance)
{
  cilk::reducer_max_index<int,double> maximumIndex(chosenRow,largest);
#pragma cilk_grainsize=128
  cilk_for (int i = 0; i < n; i++) {
    int iRow = index[i];
    double value = infeas[iRow];
    if (value > tolerance) {
      double thisWeight = CoinMin(weights[iRow], 1.0e50);
      maximumIndex.calc_max(iRow,value/thisWeight);
    }
  }
  chosenRow=maximumIndex.get_index();
  largest=maximumIndex.get_value();
}
#else
static void choose(AbcDualRowSteepest * steepest,
		   int & chosenRowSave,double & largestSave, int first, int last,
		   double tolerance)
{
  if (last-first>256) {
    int mid=(last+first)>>1;
    int chosenRow2=chosenRowSave;
    double largest2=largestSave;
    cilk_spawn choose(steepest,chosenRow2,largest2, first, mid,
		      tolerance);
    choose(steepest,chosenRowSave,largestSave, mid, last,
	   tolerance);
    cilk_sync;
    if (largest2>largestSave) {
      largestSave=largest2;
      chosenRowSave=chosenRow2;
    }
  } else {
    const int * index=steepest->infeasible()->getIndices();
     const double * infeas=steepest->infeasible()->denseVector();
    const double * weights=steepest->weights()->denseVector();
    double largest=largestSave;
    int chosenRow=chosenRowSave;
    if ((steepest->model()->stateOfProblem()&VALUES_PASS2)==0) {
      for (int i = first; i < last; i++) {
	int iRow = index[i];
	double value = infeas[iRow];
	if (value > tolerance) {
	  double thisWeight = CoinMin(weights[iRow], 1.0e50);
	  if (value>largest*thisWeight) {
	    largest=value/thisWeight;
	    chosenRow=iRow;
	  }
	}
      }
    } else {
      const double * fakeDjs = steepest->model()->fakeDjs();
      const int * pivotVariable = steepest->model()->pivotVariable();
      for (int i = first; i < last; i++) {
	int iRow = index[i];
	double value = infeas[iRow];
	if (value > tolerance) {
	  int iSequence=pivotVariable[iRow];
	  value *=(fabs(fakeDjs[iSequence])+1.0e-6);
	  double thisWeight = CoinMin(weights[iRow], 1.0e50);
	  /*
	    Ideas
	    always use fake
	    use fake until chosen
	    if successful would move djs to basic region?
	    how to switch off if cilking - easiest at saveWeights
	   */
	  if (value>largest*thisWeight) {
	    largest=value/thisWeight;
	    chosenRow=iRow;
	  }
	}
      }
    }
    chosenRowSave=chosenRow;
    largestSave=largest;
  }
}
static void choose2(AbcDualRowSteepest * steepest,
		   int & chosenRowSave,double & largestSave, int first, int last,
		   double tolerance)
{
  if (last-first>256) {
    int mid=(last+first)>>1;
    int chosenRow2=chosenRowSave;
    double largest2=largestSave;
    cilk_spawn choose2(steepest,chosenRow2,largest2, first, mid,
		      tolerance);
    choose2(steepest,chosenRowSave,largestSave, mid, last,
	   tolerance);
    cilk_sync;
    if (largest2>largestSave) {
      largestSave=largest2;
      chosenRowSave=chosenRow2;
    }
  } else {
    const int * index=steepest->infeasible()->getIndices();
    const double * infeas=steepest->infeasible()->denseVector();
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
#endif
#endif
// Returns pivot row, -1 if none
int
AbcDualRowSteepest::pivotRow()
{
  assert(model_);
  double *  COIN_RESTRICT infeas = infeasible_->denseVector();
  int *  COIN_RESTRICT index = infeasible_->getIndices();
  int number = infeasible_->getNumElements();
  int lastPivotRow = model_->lastPivotRow();
  assert (lastPivotRow < model_->numberRows());
  double tolerance = model_->currentPrimalTolerance();
  // we can't really trust infeasibilities if there is primal error
  // this coding has to mimic coding in checkPrimalSolution
  double error = CoinMin(1.0e-2, model_->largestPrimalError());
  // allow tolerance at least slightly bigger than standard
  tolerance = tolerance +  error;
  // But cap
  tolerance = CoinMin(1000.0, tolerance);
  tolerance *= tolerance; // as we are using squares
  bool toleranceChanged = false;
  const double *  COIN_RESTRICT solutionBasic = model_->solutionBasic();
  const double *  COIN_RESTRICT lowerBasic = model_->lowerBasic();
  const double *  COIN_RESTRICT upperBasic = model_->upperBasic();
  // do last pivot row one here
  if (lastPivotRow >= 0 && lastPivotRow < model_->numberRows()) {
    double value = solutionBasic[lastPivotRow];
    double lower = lowerBasic[lastPivotRow];
    double upper = upperBasic[lastPivotRow];
    if (value > upper + tolerance) {
      value -= upper;
      value *= value;
      // store square in list
      if (infeas[lastPivotRow])
	infeas[lastPivotRow] = value; // already there
      else
	infeasible_->quickAdd(lastPivotRow, value);
    } else if (value < lower - tolerance) {
      value -= lower;
      value *= value;
      // store square in list
      if (infeas[lastPivotRow])
	infeas[lastPivotRow] = value; // already there
      else
	infeasible_->add(lastPivotRow, value);
    } else {
      // feasible - was it infeasible - if so set tiny
      if (infeas[lastPivotRow])
	infeas[lastPivotRow] = COIN_INDEXED_REALLY_TINY_ELEMENT;
    }
    number = infeasible_->getNumElements();
  }
  if(model_->numberIterations() < model_->lastBadIteration() + 200) {
    // we can't really trust infeasibilities if there is dual error
    if (model_->largestDualError() > model_->largestPrimalError()) {
      tolerance *= CoinMin(model_->largestDualError() / model_->largestPrimalError(), 1000.0);
      toleranceChanged = true;
    }
  }
  int numberWanted;
  if (mode_ < 2 ) {
    numberWanted = number + 1;
  } else {
    if (model_->factorization()->pivots()==0) {
      int numberElements = model_->factorization()->numberElements();
      int numberRows = model_->numberRows();
#if 0
      int numberSlacks = model_->factorization()->numberSlacks();
      factorizationRatio_ = static_cast<double> (numberElements+1) /
	static_cast<double> (numberRows+1-numberSlacks);
#else
      factorizationRatio_ = static_cast<double> (numberElements) /
	static_cast<double> (numberRows);
#endif
      //printf("%d iterations, numberElements %d ratio %g\n",
      //     model_->numberIterations(),numberElements,factorizationRatio_);
      // Bias so less likely to go to steepest if large
      double weights[]={1.0,2.0,5.0};
      double modifier;
      if (numberRows<100000)
	modifier=weights[0];
      else if (numberRows<200000)
	modifier=weights[1];
      else
	modifier=weights[2];
      if (mode_==2&&factorizationRatio_>modifier) {
	// switch from dantzig to steepest
	mode_=3;
	saveWeights(model_,5);
      }
    }
#if 0
    // look at this and modify
    numberWanted = number+1;
    if (factorizationRatio_ < 1.0) {
      numberWanted = CoinMax(2000, numberRows / 20);
#if 0
    } else if (factorizationRatio_ > 10.0) {
      double ratio = number * (factorizationRatio_ / 80.0);
      if (ratio > number)
	numberWanted = number + 1;
      else
	numberWanted = CoinMax(2000, static_cast<int> (ratio));
#else
    } else {
      //double multiplier=CoinMin(factorizationRatio_*0.1,1.0)
#endif
    }
#else
    numberWanted = CoinMax(2000, number / 8);
    if (factorizationRatio_ < 1.5) {
      numberWanted = CoinMax(2000, number / 20);
    } else if (factorizationRatio_ > 5.0) {
      numberWanted = number + 1;
    }
#endif
    numberWanted=CoinMin(numberWanted,number+1);
  }
  if (model_->largestPrimalError() > 1.0e-3)
    numberWanted = number + 1; // be safe
#ifdef CHECK_NUMBER_WANTED
  xWanted+=numberWanted;
  xTimes++;
  if ((xTimes%1000)==0)
    printf("time %d average wanted %g\n",xTimes,static_cast<double>(xWanted)/xTimes);
#endif
  int iPass;
  // Setup two passes
  int start[4];
  start[1] = number;
  start[2] = 0;
  double dstart = static_cast<double> (number) * model_->randomNumberGenerator()->randomDouble();
  start[0] = static_cast<int> (dstart);
  start[3] = start[0];
  //double largestWeight=0.0;
  //double smallestWeight=1.0e100;
  const double * weights = weights_->denseVector();
  const int * pivotVariable = model_->pivotVariable();
  double savePivotWeight=weights_->denseVector()[lastPivotRow];
  weights_->denseVector()[lastPivotRow] *= 1.0e10;
  double largest = 0.0;
  int chosenRow = -1;
  int saveNumberWanted=numberWanted;
#ifdef DO_REDUCE
  bool doReduce=true;
  int lastChosen=-1;
  double lastLargest=0.0;
#endif
  for (iPass = 0; iPass < 2; iPass++) {
    int endThis = start[2*iPass+1];
    int startThis=start[2*iPass];
    while (startThis<endThis) {
      int end = CoinMin(endThis,startThis+numberWanted);
#ifdef DO_REDUCE 
      if (doReduce) {
#if DO_REDUCE==1
	choose(chosenRow,largest,end-startThis,
	       index+startThis,infeas,weights,tolerance);
#else
	if (mode_!=2)
	  choose(this,chosenRow,largest,startThis,end,tolerance);
	else
	  choose2(this,chosenRow,largest,startThis,end,tolerance);
#endif
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
	if (value > tolerance) {
	  //#define OUT_EQ
#ifdef OUT_EQ
	  if (upper[iRow] == lower[iRow])
	    value *= 2.0;
#endif
	  double thisWeight = CoinMin(weights[iRow], 1.0e50);
	  //largestWeight = CoinMax(largestWeight,weight);
	  //smallestWeight = CoinMin(smallestWeight,weight);
	  //double dubious = dubiousWeights_[iRow];
	  //weight *= dubious;
	  //if (value>2.0*largest*weight||(value>0.5*largest*weight&&value*largestWeight>dubious*largest*weight)) {
	  if (value > largest * thisWeight) {
	    // make last pivot row last resort choice
	    //if (iRow == lastPivotRow) {
	    //if (value * 1.0e-10 < largest * thisWeight)
	    //  continue;
	    //else
	    //  value *= 1.0e-10;
	    //}
	    if (!model_->flagged(pivotVariable[iRow])) {
	      //#define CLP_DEBUG 3
#ifdef CLP_DEBUG
	      double value2 = 0.0;
	      if (solutionBasic[iRow] > upperBasic[iRow] + tolerance)
		value2 = solutionBasic[iRow] - upperBasic[iRow];
	      else if (solutionBasic[iRow] < lowerBasic[iRow] - tolerance)
		value2 = solutionBasic[iRow] - lowerBasic[iRow];
	      assert(fabs(value2 * value2 - infeas[iRow]) < 1.0e-8 * CoinMin(value2 * value2, infeas[iRow]));
#endif
	      if (solutionBasic[iRow] > upperBasic[iRow] + tolerance ||
		  solutionBasic[iRow] < lowerBasic[iRow] - tolerance) {
		chosenRow = iRow;
		largest = value / thisWeight;
		//largestWeight = dubious;
	      }
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
  weights_->denseVector()[lastPivotRow]=savePivotWeight;
  //printf("smallest %g largest %g\n",smallestWeight,largestWeight);
  if (chosenRow < 0 && toleranceChanged) {
    // won't line up with checkPrimalSolution - do again
    double saveError = model_->largestDualError();
    model_->setLargestDualError(0.0);
    // can't loop
    chosenRow = pivotRow();
    model_->setLargestDualError(saveError);
  }
  if (chosenRow < 0 && lastPivotRow < 0) {
    int nLeft = 0;
    for (int i = 0; i < number; i++) {
      int iRow = index[i];
      if (fabs(infeas[iRow]) > 1.0e-50) {
	index[nLeft++] = iRow;
      } else {
	infeas[iRow] = 0.0;
      }
    }
    infeasible_->setNumElements(nLeft);
    model_->setNumberPrimalInfeasibilities(nLeft);
  }
  if (true&&(model_->stateOfProblem()&VALUES_PASS2)!=0) {
    if (chosenRow>=0) {
      double * fakeDjs = model_->fakeDjs();
      //const int * pivotVariable = model_->pivotVariable();
      fakeDjs[pivotVariable[chosenRow]]=0.0;
    }
  }
  return chosenRow;
}
//#define PRINT_WEIGHTS_FREQUENCY
#ifdef PRINT_WEIGHTS_FREQUENCY
int xweights1=0;
int xweights2=0;
int xweights3=0;
#endif
/* Updates weights and returns pivot alpha.
   Also does FT update */
double
AbcDualRowSteepest::updateWeights(CoinIndexedVector & input,
                                  CoinIndexedVector & updateColumn)
{
   double *  COIN_RESTRICT weights = weights_->denseVector();
#if CLP_DEBUG>2
  // Very expensive debug
  {
    int numberRows = model_->numberRows();
    CoinIndexedVector temp;
    temp.reserve(numberRows);
    double * array = temp.denseVector();
    int * which = temp.getIndices();
    for (int i = 0; i < numberRows; i++) {
      double value = 0.0;
      array[i] = 1.0;
      which[0] = i;
      temp.setNumElements(1);
      model_->factorization()->updateColumnTranspose(temp);
      int number = temp.getNumElements();
      for (int j = 0; j < number; j++) {
	int iRow = which[j];
	value += array[iRow] * array[iRow];
	array[iRow] = 0.0;
      }
      temp.setNumElements(0);
      double w = CoinMax(weights[i], value) * .1;
      if (fabs(weights[i] - value) > w) {
	printf("%d old %g, true %g\n", i, weights[i], value);
	weights[i] = value; // to reduce printout
      }
    }
  }
#endif
  double alpha = 0.0;
  double norm = 0.0;
  int i;
  double *  COIN_RESTRICT work2 = input.denseVector();
  int numberNonZero = input.getNumElements();
  int *  COIN_RESTRICT which = input.getIndices();
  
  //compute norm
  for (int i = 0; i < numberNonZero; i ++ ) {
    int iRow = which[i];
    double value = work2[iRow];
    norm += value * value;
  }
  // Do FT update
  if (true) {
    model_->factorization()->updateTwoColumnsFT(updateColumn,input);
#if CLP_DEBUG>3
    printf("REGION after %d els\n", input.getNumElements());
    input.print();
#endif
  } else {
    // Leave as old way
    model_->factorization()->updateColumnFT(updateColumn);
    model_->factorization()->updateColumn(input);
  }
  //#undef CLP_DEBUG
  numberNonZero = input.getNumElements();
  int pivotRow = model_->lastPivotRow();
  assert (model_->pivotRow()==model_->lastPivotRow());
#ifdef CLP_DEBUG
  if ( model_->logLevel (  ) > 4  &&
       fabs(norm - weights[pivotRow]) > 1.0e-2 * (1.0 + norm))
    printf("on row %d, true weight %g, old %g\n",
	   pivotRow, sqrt(norm), sqrt(weights[pivotRow]));
#endif
  // could re-initialize here (could be expensive)
  norm /= model_->alpha() * model_->alpha();
  assert(model_->alpha());
  assert(norm);
  double multiplier = 2.0 / model_->alpha();
#ifdef PRINT_WEIGHTS_FREQUENCY
  xweights1++;
  if ((xweights1+xweights2+xweights3)%100==0) {
    printf("ZZ iteration %d sum weights %d - %d %d %d\n",
	   model_->numberIterations(),xweights1+xweights2+xweights3,xweights1,xweights2,xweights3);
    assert(abs(model_->numberIterations()-(xweights1+xweights2+xweights3))<=1);
  }
#endif
  // look at updated column
  double *  COIN_RESTRICT work = updateColumn.denseVector();
  numberNonZero = updateColumn.getNumElements();
  which = updateColumn.getIndices();
  
  // pivot element
  alpha=work[pivotRow];
  for (i = 0; i < numberNonZero; i++) {
    int iRow = which[i];
    double theta = work[iRow];
    double devex = weights[iRow];
    //work3[iRow] = devex; // save old
    //which3[nSave++] = iRow;
    double value = work2[iRow];
    devex +=  theta * (theta * norm + value * multiplier);
    if (devex < DEVEX_TRY_NORM)
      devex = DEVEX_TRY_NORM;
    weights[iRow] = devex;
  }
  if (norm < DEVEX_TRY_NORM)
    norm = DEVEX_TRY_NORM;
  // Try this to make less likely will happen again and stop cycling
  //norm *= 1.02;
  weights[pivotRow] = norm;
  input.clear();
#ifdef CLP_DEBUG
  input.checkClear();
#endif
#ifdef CLP_DEBUG
  input.checkClear();
#endif
  return alpha;
}
double 
AbcDualRowSteepest::updateWeights1(CoinIndexedVector & input,CoinIndexedVector & updateColumn)
{
  if (mode_==2) {
    abort();
    // no update
    // Do FT update
    model_->factorization()->updateColumnFT(updateColumn);
    // pivot element
    double alpha = 0.0;
    // look at updated column
    double * work = updateColumn.denseVector();
    int pivotRow = model_->lastPivotRow();
    assert (pivotRow == model_->pivotRow());
    
    assert (!updateColumn.packedMode());
    alpha = work[pivotRow];
    return alpha;
  }
#if CLP_DEBUG>2
  // Very expensive debug
  {
    double * weights = weights_->denseVector();
    int numberRows = model_->numberRows();
    const int * pivotVariable = model_->pivotVariable();
    CoinIndexedVector temp;
    temp.reserve(numberRows);
    double * array = temp.denseVector();
    int * which = temp.getIndices();
    for (int i = 0; i < numberRows; i++) {
      double value = 0.0;
      array[i] = 1.0;
      which[0] = i;
      temp.setNumElements(1);
      model_->factorization()->updateColumnTranspose(temp);
      int number = temp.getNumElements();
      for (int j = 0; j < number; j++) {
	int iRow = which[j];
	value += array[iRow] * array[iRow];
	array[iRow] = 0.0;
      }
      temp.setNumElements(0);
      double w = CoinMax(weights[i], value) * .1;
      if (fabs(weights[i] - value) > w) {
	printf("XX row %d (variable %d) old %g, true %g\n", i, pivotVariable[i],
	       weights[i], value);
	weights[i] = value; // to reduce printout
      }
    }
  }
#endif
  norm_ = 0.0;
  double *  COIN_RESTRICT work2 = input.denseVector();
  int numberNonZero = input.getNumElements();
  int *  COIN_RESTRICT which = input.getIndices();
  
  //compute norm
  for (int i = 0; i < numberNonZero; i ++ ) {
    int iRow = which[i];
    double value = work2[iRow];
    norm_ += value * value;
  }
  // Do FT update
  if (true) {
    model_->factorization()->updateTwoColumnsFT(updateColumn,input);
#if CLP_DEBUG>3
    printf("REGION after %d els\n", input.getNumElements());
    input.print();
#endif
  } else {
    // Leave as old way
    model_->factorization()->updateColumnFT(updateColumn);
    model_->factorization()->updateWeights(input);
  }
  // pivot element
  assert (model_->pivotRow()==model_->lastPivotRow());
  return updateColumn.denseVector()[model_->lastPivotRow()];
}
void AbcDualRowSteepest::updateWeightsOnly(CoinIndexedVector & input)
{
  if (mode_==2)
    return;
  norm_ = 0.0;
  double *  COIN_RESTRICT work2 = input.denseVector();
  int numberNonZero = input.getNumElements();
  int *  COIN_RESTRICT which = input.getIndices();
  
  //compute norm
  for (int i = 0; i < numberNonZero; i ++ ) {
    int iRow = which[i];
    double value = work2[iRow];
    norm_ += value * value;
  }
  model_->factorization()->updateWeights(input);
}
// Actually updates weights
void AbcDualRowSteepest::updateWeights2(CoinIndexedVector & input,CoinIndexedVector & updateColumn)
{
  if (mode_==2)
    return;
  double *  COIN_RESTRICT weights = weights_->denseVector();
  const double *  COIN_RESTRICT work2 = input.denseVector();
  int pivotRow = model_->lastPivotRow();
  assert (model_->pivotRow()==model_->lastPivotRow());
#ifdef CLP_DEBUG
  if ( model_->logLevel (  ) > 4  &&
       fabs(norm_ - weights[pivotRow]) > 1.0e-2 * (1.0 + norm_))
    printf("on row %d, true weight %g, old %g\n",
	   pivotRow, sqrt(norm_), sqrt(weights[pivotRow]));
#endif
  // pivot element
  double alpha=model_->alpha();
  // could re-initialize here (could be expensive)
  norm_ /= alpha * alpha;
  assert(alpha);
  assert(norm_);
  double multiplier = 2.0 / alpha;
  // look at updated column
  const double *  COIN_RESTRICT work = updateColumn.denseVector();
  int numberNonZero = updateColumn.getNumElements();
  const int * which = updateColumn.getIndices();
  double multiplier2=1.0;
#ifdef PRINT_WEIGHTS_FREQUENCY
  xweights2++;
  if ((xweights1+xweights2+xweights3)%100==0) {
    printf("ZZ iteration %d sum weights %d - %d %d %d\n",
	   model_->numberIterations(),xweights1+xweights2+xweights3,xweights1,xweights2,xweights3);
    assert(abs(model_->numberIterations()-(xweights1+xweights2+xweights3))<=1);
  }
#endif
  if (model_->directionOut()<0) {
#ifdef ABC_DEBUG
    printf("Minus out in %d %d, out %d %d\n",model_->sequenceIn(),model_->directionIn(),
	   model_->sequenceOut(),model_->directionOut());
#endif
    multiplier2=-1.0;
  }
  for (int i = 0; i < numberNonZero; i++) {
    int iRow = which[i];
    double theta = multiplier2*work[iRow];
    double devex = weights[iRow];
    double value = work2[iRow];
    devex +=  theta * (theta * norm_ + value * multiplier);
    if (devex < DEVEX_TRY_NORM)
      devex = DEVEX_TRY_NORM;
    weights[iRow] = devex;
  }
  if (norm_ < DEVEX_TRY_NORM)
    norm_ = DEVEX_TRY_NORM;
  // Try this to make less likely will happen again and stop cycling
  //norm_ *= 1.02;
  weights[pivotRow] = norm_;
  input.clear();
#ifdef CLP_DEBUG
  input.checkClear();
#endif
}

/* Updates primal solution (and maybe list of candidates)
   Uses input vector which it deletes
   Computes change in objective function
*/
void
AbcDualRowSteepest::updatePrimalSolution(
					 CoinIndexedVector & primalUpdate,
					 double primalRatio)
{
  double * COIN_RESTRICT work = primalUpdate.denseVector();
  int number = primalUpdate.getNumElements();
  int * COIN_RESTRICT which = primalUpdate.getIndices();
  double tolerance = model_->currentPrimalTolerance();
  double * COIN_RESTRICT infeas = infeasible_->denseVector();
  double * COIN_RESTRICT solutionBasic = model_->solutionBasic();
  const double * COIN_RESTRICT lowerBasic = model_->lowerBasic();
  const double * COIN_RESTRICT upperBasic = model_->upperBasic();
  assert (!primalUpdate.packedMode());
  for (int i = 0; i < number; i++) {
    int iRow = which[i];
    double value = solutionBasic[iRow];
    double change = primalRatio * work[iRow];
    // no need to keep for steepest edge
    work[iRow] = 0.0;
    value -= change;
    double lower = lowerBasic[iRow];
    double upper = upperBasic[iRow];
    solutionBasic[iRow] = value;
    if (value < lower - tolerance) {
      value -= lower;
      value *= value;
#ifdef CLP_DUAL_FIXED_COLUMN_MULTIPLIER
      if (lower == upper)
	value *= CLP_DUAL_FIXED_COLUMN_MULTIPLIER; // bias towards taking out fixed variables
#endif
      // store square in list
      if (infeas[iRow])
	infeas[iRow] = value; // already there
      else
	infeasible_->quickAdd(iRow, value);
    } else if (value > upper + tolerance) {
      value -= upper;
      value *= value;
#ifdef CLP_DUAL_FIXED_COLUMN_MULTIPLIER
      if (lower == upper)
	value *= CLP_DUAL_FIXED_COLUMN_MULTIPLIER; // bias towards taking out fixed variables
#endif
      // store square in list
      if (infeas[iRow])
	infeas[iRow] = value; // already there
      else
	infeasible_->quickAdd(iRow, value);
    } else {
      // feasible - was it infeasible - if so set tiny
      if (infeas[iRow])
	infeas[iRow] = COIN_INDEXED_REALLY_TINY_ELEMENT;
    }
  }
  // Do pivot row
  int iRow = model_->lastPivotRow();
  assert (model_->pivotRow()==model_->lastPivotRow());
  // feasible - was it infeasible - if so set tiny
  //assert (infeas[iRow]);
  if (infeas[iRow])
    infeas[iRow] = COIN_INDEXED_REALLY_TINY_ELEMENT;
  primalUpdate.setNumElements(0);
}
#if ABC_PARALLEL==2
static void update(int first, int last,
		   const int * COIN_RESTRICT which, double * COIN_RESTRICT work, 
		   const double * COIN_RESTRICT work2, double * COIN_RESTRICT weights,
		   const double * COIN_RESTRICT lowerBasic,double * COIN_RESTRICT solutionBasic,
		   const double * COIN_RESTRICT upperBasic,
		   double multiplier,double multiplier2,
		   double norm,double theta,double tolerance)
{
  if (last-first>256) {
    int mid=(last+first)>>1;
    cilk_spawn update(first,mid,which,work,work2,weights,lowerBasic,solutionBasic,
		      upperBasic,multiplier,multiplier2,norm,theta,tolerance);
    update(mid,last,which,work,work2,weights,lowerBasic,solutionBasic,
	   upperBasic,multiplier,multiplier2,norm,theta,tolerance);
    cilk_sync;
  } else {
    for (int i = first; i < last; i++) {
      int iRow = which[i];
      double updateValue = work[iRow];
      double thetaDevex = multiplier2*updateValue;
      double devex = weights[iRow];
      double valueDevex = work2[iRow];
      devex +=  thetaDevex * (thetaDevex * norm + valueDevex * multiplier);
      if (devex < DEVEX_TRY_NORM)
	devex = DEVEX_TRY_NORM;
      weights[iRow] = devex;
      double value = solutionBasic[iRow];
      double change = theta * updateValue;
      value -= change;
      double lower = lowerBasic[iRow];
      double upper = upperBasic[iRow];
      solutionBasic[iRow] = value;
      if (value < lower - tolerance) {
	value -= lower;
	value *= value;
#ifdef CLP_DUAL_FIXED_COLUMN_MULTIPLIER
	if (lower == upper)
	  value *= CLP_DUAL_FIXED_COLUMN_MULTIPLIER; // bias towards taking out fixed variables
#endif
      } else if (value > upper + tolerance) {
	value -= upper;
	value *= value;
#ifdef CLP_DUAL_FIXED_COLUMN_MULTIPLIER
	if (lower == upper)
	  value *= CLP_DUAL_FIXED_COLUMN_MULTIPLIER; // bias towards taking out fixed variables
#endif
      } else {
	// feasible
	value=0.0;
      }
      // store square
      work[iRow]=value;
    }
  }
}
#endif
void 
AbcDualRowSteepest::updatePrimalSolutionAndWeights(CoinIndexedVector & weightsVector,
				    CoinIndexedVector & primalUpdate,
						   double theta)
{
  //printf("updatePrimal iteration %d - weightsVector has %d, primalUpdate has %d\n",
  //	 model_->numberIterations(),weightsVector.getNumElements(),primalUpdate.getNumElements());
  double *  COIN_RESTRICT weights = weights_->denseVector();
  double *  COIN_RESTRICT work2 = weightsVector.denseVector();
  int pivotRow = model_->lastPivotRow();
  assert (model_->pivotRow()==model_->lastPivotRow());
  double * COIN_RESTRICT work = primalUpdate.denseVector();
  int numberNonZero = primalUpdate.getNumElements();
  int * COIN_RESTRICT which = primalUpdate.getIndices();
  double tolerance = model_->currentPrimalTolerance();
  double * COIN_RESTRICT infeas = infeasible_->denseVector();
  double * COIN_RESTRICT solutionBasic = model_->solutionBasic();
  const double * COIN_RESTRICT lowerBasic = model_->lowerBasic();
  const double * COIN_RESTRICT upperBasic = model_->upperBasic();
  assert (!primalUpdate.packedMode());
#ifdef CLP_DEBUG
  if ( model_->logLevel (  ) > 4  &&
       fabs(norm_ - weights[pivotRow]) > 1.0e-2 * (1.0 + norm_))
    printf("on row %d, true weight %g, old %g\n",
	   pivotRow, sqrt(norm_), sqrt(weights[pivotRow]));
#endif
  // pivot element
  double alpha=model_->alpha();
  // could re-initialize here (could be expensive)
  norm_ /= alpha * alpha;
  assert(alpha);
  assert(norm_);
  double multiplier = 2.0 / alpha;
#ifdef PRINT_WEIGHTS_FREQUENCY
  // only weights3 is used? - slightly faster to just update weights if going to invert
  xweights3++;
  if ((xweights1+xweights2+xweights3)%1000==0) {
    printf("ZZ iteration %d sum weights %d - %d %d %d\n",
	   model_->numberIterations(),xweights1+xweights2+xweights3,xweights1,xweights2,xweights3);
    assert(abs(model_->numberIterations()-(xweights1+xweights2+xweights3))<=1);
  }
#endif
  // look at updated column
  double multiplier2=1.0;
  if (model_->directionOut()<0) {
#ifdef ABC_DEBUG
    printf("Minus out in %d %d, out %d %d\n",model_->sequenceIn(),model_->directionIn(),
	   model_->sequenceOut(),model_->directionOut());
#endif
    multiplier2=-1.0;
  }
#if ABC_PARALLEL==2
  update(0,numberNonZero,which,work,work2,weights,
	 lowerBasic,solutionBasic,upperBasic,
	 multiplier,multiplier2,norm_,theta,tolerance);
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
    double thetaDevex = multiplier2*updateValue;
    double devex = weights[iRow];
    double valueDevex = work2[iRow];
    devex +=  thetaDevex * (thetaDevex * norm_ + valueDevex * multiplier);
    if (devex < DEVEX_TRY_NORM)
      devex = DEVEX_TRY_NORM;
    weights[iRow] = devex;
    double value = solutionBasic[iRow];
    double change = theta * updateValue;
    value -= change;
    double lower = lowerBasic[iRow];
    double upper = upperBasic[iRow];
    solutionBasic[iRow] = value;
    if (value < lower - tolerance) {
      value -= lower;
      value *= value;
#ifdef CLP_DUAL_FIXED_COLUMN_MULTIPLIER
      if (lower == upper)
	value *= CLP_DUAL_FIXED_COLUMN_MULTIPLIER; // bias towards taking out fixed variables
#endif
      // store square in list
      if (infeas[iRow])
	infeas[iRow] = value; // already there
      else
	infeasible_->quickAdd(iRow, value);
    } else if (value > upper + tolerance) {
      value -= upper;
      value *= value;
#ifdef CLP_DUAL_FIXED_COLUMN_MULTIPLIER
      if (lower == upper)
	value *= CLP_DUAL_FIXED_COLUMN_MULTIPLIER; // bias towards taking out fixed variables
#endif
      // store square in list
      if (infeas[iRow])
	infeas[iRow] = value; // already there
      else
	infeasible_->quickAdd(iRow, value);
    } else {
      // feasible - was it infeasible - if so set tiny
      if (infeas[iRow])
	infeas[iRow] = COIN_INDEXED_REALLY_TINY_ELEMENT;
    }
  }
#endif
  weightsVector.clear();
  primalUpdate.setNumElements(0);
#ifdef CLP_DEBUG
  weightsVector.checkClear();
#endif
  // Do pivot row
  if (norm_ < DEVEX_TRY_NORM)
    norm_ = DEVEX_TRY_NORM;
  // Try this to make less likely will happen again and stop cycling
  //norm_ *= 1.02;
  weights[pivotRow] = norm_;
  // feasible - was it infeasible - if so set tiny
  //assert (infeas[iRow]);
  if (infeas[pivotRow])
    infeas[pivotRow] = COIN_INDEXED_REALLY_TINY_ELEMENT;
  // see 1/2 above primalUpdate.setNumElements(0);
}
extern void parallelDual5(AbcSimplexFactorization * factorization,
			 CoinIndexedVector ** whichVector,
			 int numberCpu,
			 int whichCpu,
			 double * weights);
/* Saves any weights round factorization as pivot rows may change
   1) before factorization
   2) after factorization
   3) just redo infeasibilities
   4) restore weights
*/
void
AbcDualRowSteepest::saveWeights(AbcSimplex * model, int mode)
{
  model_ = model;
  int numberRows = model_->numberRows();
  // faster to save indices in array numberTotal in length
  const int * pivotVariable = model_->pivotVariable();
  double * weights = weights_ ? weights_->denseVector() : NULL;
  // see if we need to switch
  //if (mode_==2&&mode==2) {
    // switch
    //mode=5;
  //}
  if (mode == 1) {
    if(weights_) {
      // Check if size has changed
      if (infeasible_->capacity() == numberRows) {
	// change from row numbers to sequence numbers
	CoinAbcMemcpy(weights_->getIndices(),pivotVariable,numberRows);
	state_ = 1;
      } else {
	// size has changed - clear everything
	delete weights_;
	weights_ = NULL;
	delete infeasible_;
	infeasible_ = NULL;
	delete savedWeights_;
	savedWeights_ = NULL;
	state_ = -1;
      }
    }
  } else if (mode !=3) {
    // restore
    if (!weights_ || state_ == -1 || mode == 5) {
      // see if crash basis???
      int numberBasicSlacks=0;
      bool initializeNorms=false;
      for (int i=0;i<numberRows;i++) {
	if (model_->getInternalStatus(i)==AbcSimplex::basic)
	  numberBasicSlacks++;
      }
      if (numberBasicSlacks<numberRows&&!model_->numberIterations()) {
	char line[100];
	if (numberBasicSlacks>numberRows-(numberRows>>1)&&mode_==3&&
	    (model_->moreSpecialOptions()&256)==0) {
	  sprintf(line,"Steep initial basis has %d structurals out of %d - initializing norms\n",numberRows-numberBasicSlacks,numberRows);
	  initializeNorms=true;
	} else {
	  sprintf(line,"Steep initial basis has %d structurals out of %d - too many\n",numberRows-numberBasicSlacks,numberRows);
	}
	model_->messageHandler()->message(CLP_GENERAL2,*model_->messagesPointer())
	  << line << CoinMessageEol;
      }
      // initialize weights
      delete weights_;
      delete savedWeights_;
      delete infeasible_;
      weights_ = new CoinIndexedVector();
      weights_->reserve(numberRows);
      savedWeights_ = new CoinIndexedVector();
      // allow for dense multiple of 8!
      savedWeights_->reserve(numberRows+((initializeNorms) ? 8 : 0));
      infeasible_ = new CoinIndexedVector();
      infeasible_->reserve(numberRows);
      weights = weights_->denseVector();
      if (mode == 5||(mode_!=1&&!initializeNorms)) {
	// initialize to 1.0 (can we do better?)
	for (int i = 0; i < numberRows; i++) {
	  weights[i] = 1.0;
	}
      } else {
	double *  COIN_RESTRICT array = savedWeights_->denseVector();
	int *  COIN_RESTRICT which = savedWeights_->getIndices();
	// Later look at factorization to see which can be skipped
	// also use threads to parallelize
#if 0
	// use infeasible to collect row counts
	int * COIN_RESTRICT counts = infeasible_->getIndices();
	CoinAbcMemset0(counts,numberRows);
	// get matrix data pointers
	int maximumRows = model_->maximumAbcNumberRows();
	const CoinBigIndex * columnStart = model_->abcMatrix()->getPackedMatrix()->getVectorStarts()-maximumRows;
	//const int * columnLength = model_->abcMatrix()->getPackedMatrix()->getVectorLengths()-maximumRows;
	const int * row = model_->abcMatrix()->getPackedMatrix()->getIndices();
	const int * pivotVariable = model_->pivotVariable();
	for (int i = 0; i < numberRows; i++) {
	  int iSequence = pivotVariable[i];
	  if (iSequence>=numberRows) {
	    for (CoinBigIndex j=columnStart[iSequence];j<columnStart[iSequence+1];j++) {
	      int iRow=row[j];
	      counts[iRow]++;
	    }
	  }
	}
	int nSlack=0;
#endif
	//#undef ABC_PARALLEL
	// for now just with cilk
#if ABC_PARALLEL==2
	if (model_->parallelMode()==0) {
#endif	
	for (int i = 0; i < numberRows; i++) {
	  double value = 0.0;
	  array[i] = 1.0;
	  which[0] = i;
	  savedWeights_->setNumElements(1);
	  model_->factorization()->updateColumnTranspose(*savedWeights_);
	  int number = savedWeights_->getNumElements();
	  for (int j = 0; j < number; j++) {
	    int k= which[j];
	    value += array[k] * array[k];
	    array[k] = 0.0;
	  }
#if 0
	  if (!counts[i]) {
	    assert (value==1.0);
	    nSlack++;
	  }
#endif
	  savedWeights_->setNumElements(0);
	  weights[i] = value;
	}
#if ABC_PARALLEL==2
	} else {
	  // parallel
	  // get arrays
	  int numberCpuMinusOne=CoinMin(model_->parallelMode(),3);
	  int which[3];
	  CoinIndexedVector * whichVector[4];
	  for (int i=0;i<numberCpuMinusOne;i++) {
	    which[i]=model_->getAvailableArray();
	    whichVector[i]=model_->usefulArray(which[i]);
	    assert(!whichVector[i]->packedMode());
	  }
	  whichVector[numberCpuMinusOne]=savedWeights_;
	  parallelDual5(model_->factorization(),whichVector,numberCpuMinusOne+1,numberCpuMinusOne,weights);
	  for (int i=0;i<numberCpuMinusOne;i++) 
	    model_->clearArrays(which[i]);
	}
#endif	
#if 0
	printf("Steep %d can be skipped\n",nSlack);
#endif
      }
    } else {
      int useArray=model_->getAvailableArrayPublic();
      CoinIndexedVector * usefulArray = model_->usefulArray(useArray);
      int *  COIN_RESTRICT which = usefulArray->getIndices();
      double *  COIN_RESTRICT element = usefulArray->denseVector();
      if (mode!=4) {
	CoinAbcMemcpy(which,weights_->getIndices(),numberRows);
	CoinAbcScatterTo(weights_->denseVector(),element,which,numberRows);
      } else {
	// restore weights
	CoinAbcMemcpy(which,savedWeights_->getIndices(),numberRows);
	CoinAbcScatterTo(savedWeights_->denseVector(),element,which,numberRows);
      }
      for (int i = 0; i < numberRows; i++) {
	int iPivot = pivotVariable[i];
	double thisWeight=element[iPivot];
	if (thisWeight) {
	  if (thisWeight < DEVEX_TRY_NORM)
	    thisWeight = DEVEX_TRY_NORM; // may need to check more
	  weights[i] = thisWeight;
	} else {
	  // odd
	  weights[i] = 1.0;
	}
      }
      if (0) {
	double w[1000];
	int s[1000];
	assert (numberRows<=1000);
	memcpy(w,weights,numberRows*sizeof(double));
	memcpy(s,pivotVariable,numberRows*sizeof(int));
	CoinSort_2(s,s+numberRows,w);
	printf("Weights===========\n");
	for (int i=0;i<numberRows;i++) {
	  printf("%d seq %d weight %g\n",i,s[i],w[i]);
	}
	printf("End weights===========\n");
      }
      usefulArray->setNumElements(numberRows);
      usefulArray->clear();
      // won't be done in parallel
      model_->clearArraysPublic(useArray);
    }
    state_ = 0;
    CoinAbcMemcpy(savedWeights_->denseVector(),weights_->denseVector(),numberRows);
    CoinAbcMemcpy(savedWeights_->getIndices(),pivotVariable,numberRows);
  }
  if (mode >= 2) {
    infeasible_->clear();
    double tolerance = model_->currentPrimalTolerance();
    const double *  COIN_RESTRICT solutionBasic = model_->solutionBasic();
    const double *  COIN_RESTRICT lowerBasic = model_->lowerBasic();
    const double *  COIN_RESTRICT upperBasic = model_->upperBasic();
    if ((model_->stateOfProblem()&VALUES_PASS2)==0) {
      for (int iRow = 0; iRow < numberRows; iRow++) {
	double value = solutionBasic[iRow];
	double lower = lowerBasic[iRow];
	double upper = upperBasic[iRow];
	if (value < lower - tolerance) {
	  value -= lower;
	  value *= value;
#ifdef CLP_DUAL_FIXED_COLUMN_MULTIPLIER
	  if (lower == upper)
	    value *= CLP_DUAL_FIXED_COLUMN_MULTIPLIER; // bias towards taking out fixed variables
#endif
	  // store square in list
	  infeasible_->quickAdd(iRow, value);
	} else if (value > upper + tolerance) {
	  value -= upper;
	  value *= value;
#ifdef CLP_DUAL_FIXED_COLUMN_MULTIPLIER
	  if (lower == upper)
	    value *= CLP_DUAL_FIXED_COLUMN_MULTIPLIER; // bias towards taking out fixed variables
#endif
	  // store square in list
	infeasible_->quickAdd(iRow, value);
	}
      }
    } else {
      // check values pass
      const double * fakeDjs = model_->fakeDjs();
      int numberFake=0;
      for (int iRow = 0; iRow < numberRows; iRow++) {
	double value = solutionBasic[iRow];
	double lower = lowerBasic[iRow];
	double upper = upperBasic[iRow];
	if (value < lower - tolerance) {
	  value -= lower;
	} else if (value > upper + tolerance) {
	  value -= upper;
	} else {
	  value=0.0;
	}
	if (value) {
	  value *= value;
#ifdef CLP_DUAL_FIXED_COLUMN_MULTIPLIER
	  if (lower == upper)
	    value *= CLP_DUAL_FIXED_COLUMN_MULTIPLIER; // bias towards taking out fixed variables
#endif
	  // store square in list
	  infeasible_->quickAdd(iRow, value);
	  int iSequence=pivotVariable[iRow];
	  double fakeDj=fakeDjs[iSequence];
	  if (fabs(fakeDj)>10.0*tolerance) 
	    numberFake++;
	}
      }
      if (numberFake)
	printf("%d fake basic djs\n",numberFake);
      else
	model_->setStateOfProblem(model_->stateOfProblem()&~VALUES_PASS2);
    }
  }
}
// Recompute infeasibilities
void 
AbcDualRowSteepest::recomputeInfeasibilities()
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
      value *= value;
#ifdef CLP_DUAL_FIXED_COLUMN_MULTIPLIER
      if (lower == upper)
	value *= CLP_DUAL_FIXED_COLUMN_MULTIPLIER; // bias towards taking out fixed variables
#endif
      // store square in list
      infeasible_->quickAdd(iRow, value);
    } else if (value > upper + tolerance) {
      value -= upper;
      value *= value;
#ifdef CLP_DUAL_FIXED_COLUMN_MULTIPLIER
      if (lower == upper)
	value *= CLP_DUAL_FIXED_COLUMN_MULTIPLIER; // bias towards taking out fixed variables
#endif
      // store square in list
      infeasible_->quickAdd(iRow, value);
    }
  }
}
//-------------------------------------------------------------------
// Clone
//-------------------------------------------------------------------
AbcDualRowPivot * AbcDualRowSteepest::clone(bool CopyData) const
{
  if (CopyData) {
    return new AbcDualRowSteepest(*this);
  } else {
    return new AbcDualRowSteepest();
  }
}
// Gets rid of all arrays
void
AbcDualRowSteepest::clearArrays()
{
  if (persistence_ == normal) {
    delete weights_;
    weights_ = NULL;
    delete infeasible_;
    infeasible_ = NULL;
    delete savedWeights_;
    savedWeights_ = NULL;
  }
  state_ = -1;
}
// Returns true if would not find any row
bool
AbcDualRowSteepest::looksOptimal() const
{
  int iRow;
  double tolerance = model_->currentPrimalTolerance();
  // we can't really trust infeasibilities if there is primal error
  // this coding has to mimic coding in checkPrimalSolution
  double error = CoinMin(1.0e-2, model_->largestPrimalError());
  // allow tolerance at least slightly bigger than standard
  tolerance = tolerance +  error;
  // But cap
  tolerance = CoinMin(1000.0, tolerance);
  int numberRows = model_->numberRows();
  int numberInfeasible = 0;
  const double *  COIN_RESTRICT solutionBasic = model_->solutionBasic();
  const double *  COIN_RESTRICT lowerBasic = model_->lowerBasic();
  const double *  COIN_RESTRICT upperBasic = model_->upperBasic();
  for (iRow = 0; iRow < numberRows; iRow++) {
    double value = solutionBasic[iRow];
    double lower = lowerBasic[iRow];
    double upper = upperBasic[iRow];
    if (value < lower - tolerance) {
      numberInfeasible++;
    } else if (value > upper + tolerance) {
      numberInfeasible++;
    }
  }
  return (numberInfeasible == 0);
}
