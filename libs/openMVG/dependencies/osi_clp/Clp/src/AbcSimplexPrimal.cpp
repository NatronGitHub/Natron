/* $Id$ */
// Copyright (C) 2002, International Business Machines
// Corporation and others, Copyright (C) 2012, FasterCoin.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

/* Notes on implementation of primal simplex algorithm.
   
   When primal feasible(A):
   
   If dual feasible, we are optimal.  Otherwise choose an infeasible
   basic variable to enter basis from a bound (B).  We now need to find an
   outgoing variable which will leave problem primal feasible so we get
   the column of the tableau corresponding to the incoming variable
   (with the correct sign depending if variable will go up or down).
   
   We now perform a ratio test to determine which outgoing variable will
   preserve primal feasibility (C).  If no variable found then problem
   is unbounded (in primal sense).  If there is a variable, we then
   perform pivot and repeat.  Trivial?
   
   -------------------------------------------
   
   A) How do we get primal feasible?  All variables have fake costs
   outside their feasible region so it is trivial to declare problem
   feasible.  OSL did not have a phase 1/phase 2 approach but
   instead effectively put an extra cost on infeasible basic variables
   I am taking the same approach here, although it is generalized
   to allow for non-linear costs and dual information.
   
   In OSL, this weight was changed heuristically, here at present
   it is only increased if problem looks finished.  If problem is
   feasible I check for unboundedness.  If not unbounded we
   could play with going into dual.  As long as weights increase
   any algorithm would be finite.
   
   B) Which incoming variable to choose is a virtual base class.
   For difficult problems steepest edge is preferred while for
   very easy (large) problems we will need partial scan.
   
   C) Sounds easy, but this is hardest part of algorithm.
   1) Instead of stopping at first choice, we may be able
   to allow that variable to go through bound and if objective
   still improving choose again.  These mini iterations can
   increase speed by orders of magnitude but we may need to
   go to more of a bucket choice of variable rather than looking
   at them one by one (for speed).
   2) Accuracy.  Basic infeasibilities may be less than
   tolerance.  Pivoting on these makes objective go backwards.
   OSL modified cost so a zero move was made, Gill et al
   modified so a strictly positive move was made.
   The two problems are that re-factorizations can
   change rinfeasibilities above and below tolerances and that when
   finished we need to reset costs and try again.
   3) Degeneracy.  Gill et al helps but may not be enough.  We
   may need more.  Also it can improve speed a lot if we perturb
   the rhs and bounds significantly.
   
   References:
   Forrest and Goldfarb, Steepest-edge simplex algorithms for
   linear programming - Mathematical Programming 1992
   Forrest and Tomlin, Implementing the simplex method for
   the Optimization Subroutine Library - IBM Systems Journal 1992
   Gill, Murray, Saunders, Wright A Practical Anti-Cycling
   Procedure for Linear and Nonlinear Programming SOL report 1988
   
   
   TODO:
   
   a) Better recovery procedures.  At present I never check on forward
   progress.  There is checkpoint/restart with reducing
   re-factorization frequency, but this is only on singular
   factorizations.
   b) Fast methods for large easy problems (and also the option for
   the code to automatically choose which method).
   c) We need to be able to stop in various ways for OSI - this
   is fairly easy.
   
*/

#include "CoinPragma.hpp"

#include <math.h>

#include "CoinHelperFunctions.hpp"
#include "CoinAbcHelperFunctions.hpp"
#include "AbcSimplexPrimal.hpp"
#include "AbcSimplexFactorization.hpp"
#include "AbcNonLinearCost.hpp"
#include "CoinPackedMatrix.hpp"
#include "CoinIndexedVector.hpp"
#include "AbcPrimalColumnPivot.hpp"
#include "ClpMessage.hpp"
#include "ClpEventHandler.hpp"
#include <cfloat>
#include <cassert>
#include <string>
#include <stdio.h>
#include <iostream>
#ifdef CLP_USER_DRIVEN1
/* Returns true if variable sequenceOut can leave basis when
   model->sequenceIn() enters.
   This function may be entered several times for each sequenceOut.  
   The first time realAlpha will be positive if going to lower bound
   and negative if going to upper bound (scaled bounds in lower,upper) - then will be zero.
   currentValue is distance to bound.
   currentTheta is current theta.
   alpha is fabs(pivot element).
   Variable will change theta if currentValue - currentTheta*alpha < 0.0
*/
bool userChoiceValid1(const AbcSimplex * model,
		      int sequenceOut,
		      double currentValue,
		      double currentTheta,
		      double alpha,
		      double realAlpha);
/* This returns true if chosen in/out pair valid.
   The main thing to check would be variable flipping bounds may be
   OK.  This would be signaled by reasonable theta_ and valueOut_.
   If you return false sequenceIn_ will be flagged as ineligible.
*/
bool userChoiceValid2(const AbcSimplex * model);
/* If a good pivot then you may wish to unflag some variables.
 */
void userChoiceWasGood(AbcSimplex * model);
#endif
#ifdef TRY_SPLIT_VALUES_PASS
static double valuesChunk=-10.0;
static double valuesRatio=0.1;
static int valuesStop=-1;
static int keepValuesPass=-1;
static int doOrdinaryVariables=-1;
#endif
// primal
int AbcSimplexPrimal::primal (int ifValuesPass , int /*startFinishOptions*/)
{
  
  /*
    Method
    
    It tries to be a single phase approach with a weight of 1.0 being
    given to getting optimal and a weight of infeasibilityCost_ being
    given to getting primal feasible.  In this version I have tried to
    be clever in a stupid way.  The idea of fake bounds in dual
    seems to work so the primal analogue would be that of getting
    bounds on reduced costs (by a presolve approach) and using
    these for being above or below feasible region.  I decided to waste
    memory and keep these explicitly.  This allows for non-linear
    costs!
    
    The code is designed to take advantage of sparsity so arrays are
    seldom zeroed out from scratch or gone over in their entirety.
    The only exception is a full scan to find incoming variable for
    Dantzig row choice.  For steepest edge we keep an updated list
    of dual infeasibilities (actually squares).
    On easy problems we don't need full scan - just
    pick first reasonable.
    
    One problem is how to tackle degeneracy and accuracy.  At present
    I am using the modification of costs which I put in OSL and which was
    extended by Gill et al.  I am still not sure of the exact details.
    
    The flow of primal is three while loops as follows:
    
    while (not finished) {
    
    while (not clean solution) {
    
    Factorize and/or clean up solution by changing bounds so
    primal feasible.  If looks finished check fake primal bounds.
    Repeat until status is iterating (-1) or finished (0,1,2)
    
    }
    
    while (status==-1) {
    
    Iterate until no pivot in or out or time to re-factorize.
    
    Flow is:
    
    choose pivot column (incoming variable).  if none then
    we are primal feasible so looks as if done but we need to
    break and check bounds etc.
    
    Get pivot column in tableau
    
    Choose outgoing row.  If we don't find one then we look
    primal unbounded so break and check bounds etc.  (Also the
    pivot tolerance is larger after any iterations so that may be
    reason)
    
    If we do find outgoing row, we may have to adjust costs to
    keep going forwards (anti-degeneracy).  Check pivot will be stable
    and if unstable throw away iteration and break to re-factorize.
    If minor error re-factorize after iteration.
    
    Update everything (this may involve changing bounds on
    variables to stay primal feasible.
    
    }
    
    }
    
    At present we never check we are going forwards.  I overdid that in
    OSL so will try and make a last resort.
    
    Needs partial scan pivot in option.
    
    May need other anti-degeneracy measures, especially if we try and use
    loose tolerances as a way to solve in fewer iterations.
    
    I like idea of dynamic scaling.  This gives opportunity to decouple
    different implications of scaling for accuracy, iteration count and
    feasibility tolerance.
    
  */
  
  algorithm_ = +1;
  moreSpecialOptions_ &= ~16; // clear check replaceColumn accuracy
#if ABC_PARALLEL>0
  if (parallelMode()) {
    // extra regions
    // for moment allow for ordered factorization
    int length=2*numberRows_+abcFactorization_->maximumPivots();
    for (int i = 0; i < 6; i++) {
      delete rowArray_[i];
      rowArray_[i] = new CoinIndexedVector();
      rowArray_[i]->reserve(length);
      delete columnArray_[i];
      columnArray_[i] = new CoinIndexedVector();
      columnArray_[i]->reserve(length);
    }
  }
#endif
  // save data
  ClpDataSave data = saveData();
  dualTolerance_ = dblParam_[ClpDualTolerance];
  primalTolerance_ = dblParam_[ClpPrimalTolerance];
  currentDualTolerance_=dualTolerance_;
  //nextCleanNonBasicIteration_=baseIteration_+numberRows_;
  // Save so can see if doing after dual
  int initialStatus = problemStatus_;
  int initialIterations = numberIterations_;
  int initialNegDjs = -1;
  // copy bounds to perturbation
  CoinAbcMemcpy(abcPerturbation_,abcLower_,numberTotal_);
  CoinAbcMemcpy(abcPerturbation_+numberTotal_,abcUpper_,numberTotal_);
#if 0
  if (numberRows_>80000&&numberRows_<90000) {
    FILE * fp = fopen("save.stuff", "rb");
    if (fp) {
      fread(internalStatus_,sizeof(char),numberTotal_,fp);
      fread(abcSolution_,sizeof(double),numberTotal_,fp);
    } else {
      printf("can't open save.stuff\n");
      abort();
    }
    fclose(fp);
  }
#endif
  // initialize - maybe values pass and algorithm_ is +1
  /// Initial coding here
  if (nonLinearCost_ == NULL && algorithm_ > 0) {
    // get a valid nonlinear cost function
    abcNonLinearCost_ = new AbcNonLinearCost(this);
  }
  statusOfProblemInPrimal(0);
  /*if (!startup(ifValuesPass))*/ {
    
    // Set average theta
    abcNonLinearCost_->setAverageTheta(1.0e3);
    
    // Say no pivot has occurred (for steepest edge and updates)
    pivotRow_ = -2;
    
    // This says whether to restore things etc
    int factorType = 0;
    if (problemStatus_ < 0 && perturbation_ < 100 && !ifValuesPass) {
      perturb(0);
      if (perturbation_!=100) {
	// Can't get here if values pass
	assert (!ifValuesPass);
	gutsOfPrimalSolution(3);
	if (handler_->logLevel() > 2) {
	  handler_->message(CLP_SIMPLEX_STATUS, messages_)
	    << numberIterations_ << objectiveValue();
	  handler_->printing(sumPrimalInfeasibilities_ > 0.0)
	    << sumPrimalInfeasibilities_ << numberPrimalInfeasibilities_;
	  handler_->printing(sumDualInfeasibilities_ > 0.0)
	    << sumDualInfeasibilities_ << numberDualInfeasibilities_;
	  handler_->printing(numberDualInfeasibilitiesWithoutFree_
			     < numberDualInfeasibilities_)
	    << numberDualInfeasibilitiesWithoutFree_;
	  handler_->message() << CoinMessageEol;
	}
      }
    }
    // Start check for cycles
    abcProgress_.fillFromModel(this);
    abcProgress_.startCheck();
    double pivotTolerance=abcFactorization_->pivotTolerance();
    /*
      Status of problem:
      0 - optimal
      1 - infeasible
      2 - unbounded
      -1 - iterating
      -2 - factorization wanted
      -3 - redo checking without factorization
      -4 - looks infeasible
      -5 - looks unbounded
    */
    while (problemStatus_ < 0) {
      // clear all arrays
      clearArrays(-1);
      // If getting nowhere - why not give it a kick
#if 1
      if (perturbation_ < 101 && numberIterations_ ==6078/*> 2 * (numberRows_ + numberColumns_)*/ && (specialOptions_ & 4) == 0
	  && initialStatus != 10) {
	perturb(1);
      }
#endif
      // If we have done no iterations - special
      if (lastGoodIteration_ == numberIterations_ && factorType)
	factorType = 3;
      if(pivotTolerance<abcFactorization_->pivotTolerance()) {
	// force factorization
	pivotTolerance = abcFactorization_->pivotTolerance();
	factorType=5;
      }
      
      // may factorize, checks if problem finished
      if (factorType)
	statusOfProblemInPrimal(factorType);
      if (problemStatus_==10 && (moreSpecialOptions_ & 2048) != 0) {
	problemStatus_=0;
      }
      if (initialStatus == 10) {
	initialStatus=-1;
	// cleanup phase
	if(initialIterations != numberIterations_) {
	  if (numberDualInfeasibilities_ > 10000 && numberDualInfeasibilities_ > 10 * initialNegDjs) {
	    // getting worse - try perturbing
	    if (perturbation_ < 101 && (specialOptions_ & 4) == 0) {
	      perturb(1);
	      statusOfProblemInPrimal(factorType);
	    }
	  }
	} else {
	  // save number of negative djs
	  if (!numberPrimalInfeasibilities_)
	    initialNegDjs = numberDualInfeasibilities_;
	  // make sure weight won't be changed
	  if (infeasibilityCost_ == 1.0e10)
	    infeasibilityCost_ = 1.000001e10;
	}
      }
      
      // Say good factorization
      factorType = 1;
      
      // Say no pivot has occurred (for steepest edge and updates)
      pivotRow_ = -2;
      
      // exit if victory declared
      if (problemStatus_ >= 0) {
#ifdef CLP_USER_DRIVEN
	int status = 
	  eventHandler_->event(ClpEventHandler::endInPrimal);
	if (status>=0&&status<10) {
	  // carry on
	  problemStatus_=-1;
	  if (status==0)
	    continue; // re-factorize
	} else if (status>=10) {
	  problemStatus_=status-10;
	  break;
	} else {
	  break;
	}
#else
	break;
#endif
      }
      
      // test for maximum iterations
      if (hitMaximumIterations() || (ifValuesPass == 2 && firstFree_ < 0)) {
	problemStatus_ = 3;
	break;
      }
      
      if (firstFree_ < 0) {
	if (ifValuesPass) {
	  // end of values pass
	  ifValuesPass = 0;
	  int status = eventHandler_->event(ClpEventHandler::endOfValuesPass);
	  if (status >= 0) {
	    problemStatus_ = 5;
	    secondaryStatus_ = ClpEventHandler::endOfValuesPass;
	    break;
	  }
	  //#define FEB_TRY
#if 1 //def FEB_TRY
	  if (perturbation_ < 100
#ifdef TRY_SPLIT_VALUES_PASS
	      &&valuesStop<0
#endif
	      )
	    perturb(0);
#endif
	}
      }
      // Check event
      {
	int status = eventHandler_->event(ClpEventHandler::endOfFactorization);
	if (status >= 0) {
	  problemStatus_ = 5;
	  secondaryStatus_ = ClpEventHandler::endOfFactorization;
	  break;
	}
      }
      // Iterate
      whileIterating(ifValuesPass ? 1 : 0);
    }
  }
  // if infeasible get real values
  //printf("XXXXY final cost %g\n",infeasibilityCost_);
  abcProgress_.initialWeight_ = 0.0;
  if (problemStatus_ == 1 && secondaryStatus_ != 6) {
    infeasibilityCost_ = 0.0;
    copyFromSaved();
    delete abcNonLinearCost_;
    abcNonLinearCost_ = new AbcNonLinearCost(this);
    abcNonLinearCost_->checkInfeasibilities(0.0);
    sumPrimalInfeasibilities_ = abcNonLinearCost_->sumInfeasibilities();
    numberPrimalInfeasibilities_ = abcNonLinearCost_->numberInfeasibilities();
    // and get good feasible duals
    gutsOfPrimalSolution(1);
  }
  // clean up
  unflag();
  abcProgress_.clearTimesFlagged();
#if ABC_PARALLEL>0
  if (parallelMode()) {
    for (int i = 0; i < 6; i++) {
      delete rowArray_[i];
      rowArray_[i] = NULL;
      delete columnArray_[i];
      columnArray_[i] = NULL;
    }
  }
#endif
  //finish(startFinishOptions);
  restoreData(data);
  setObjectiveValue(abcNonLinearCost_->feasibleReportCost()+objectiveOffset_);
  // clean up
  if (problemStatus_==10)
    abcNonLinearCost_->checkInfeasibilities(0.0);
  delete abcNonLinearCost_;
  abcNonLinearCost_=NULL;
#if 0
  if (numberRows_>80000&&numberRows_<90000) {
    FILE * fp = fopen("save.stuff", "wb");
    if (fp) {
      fwrite(internalStatus_,sizeof(char),numberTotal_,fp);
      fwrite(abcSolution_,sizeof(double),numberTotal_,fp);
    } else {
      printf("can't open save.stuff\n");
      abort();
    }
    fclose(fp);
  }
#endif
  return problemStatus_;
}
/*
  Reasons to come out:
  -1 iterations etc
  -2 inaccuracy
  -3 slight inaccuracy (and done iterations)
  -4 end of values pass and done iterations
  +0 looks optimal (might be infeasible - but we will investigate)
  +2 looks unbounded
  +3 max iterations
*/
int
AbcSimplexPrimal::whileIterating(int valuesOption)
{
#if 1
#define DELAYED_UPDATE
  arrayForBtran_=0;
  //setUsedArray(arrayForBtran_);
  arrayForFtran_=1;
  setUsedArray(arrayForFtran_);
  arrayForFlipBounds_=2;
  setUsedArray(arrayForFlipBounds_);
  arrayForTableauRow_=3;
  setUsedArray(arrayForTableauRow_);
  //arrayForDualColumn_=4;
  //setUsedArray(arrayForDualColumn_);
#if ABC_PARALLEL<2
  arrayForReplaceColumn_=4; //4;
#else
  arrayForReplaceColumn_=6; //4;
  setUsedArray(arrayForReplaceColumn_);
#endif
  //arrayForFlipRhs_=5;
  //setUsedArray(arrayForFlipRhs_);
#endif
  // Say if values pass
  int ifValuesPass = (firstFree_ >= 0) ? 1 : 0;
  int returnCode = -1;
  int superBasicType = 1;
  if (valuesOption > 1)
    superBasicType = 3;
  int numberStartingInfeasibilities=abcNonLinearCost_->numberInfeasibilities();
  // status stays at -1 while iterating, >=0 finished, -2 to invert
  // status -3 to go to top without an invert
  int numberFlaggedStart =abcProgress_.timesFlagged();
  while (problemStatus_ == -1) {
    if (!ifValuesPass) {
      // choose column to come in
      // can use pivotRow_ to update weights
      // pass in list of cost changes so can do row updates (rowArray_[1])
      // NOTE rowArray_[0] is used by computeDuals which is a
      // slow way of getting duals but might be used
      int saveSequence=sequenceIn_;
      primalColumn(&usefulArray_[arrayForFtran_], &usefulArray_[arrayForTableauRow_],
		   &usefulArray_[arrayForFlipBounds_]);
      if (saveSequence>=0&&saveSequence!=sequenceOut_) {
	if (getInternalStatus(saveSequence)==basic)
	  abcDj_[saveSequence]=0.0;
      }
#if ABC_NORMAL_DEBUG>0
      if (handler_->logLevel()==63) {
	for (int i=0;i<numberTotal_;i++) {
	  if (fabs(abcDj_[i])>1.0e2)
	    assert (getInternalStatus(i)!=basic);
	}
	double largestCost=0.0;
	double largestDj=0.0;
	double largestGoodDj=0.0;
	int iLargest=-1;
	int numberInfeasibilities=0;
	double sum=0.0;
	for (int i=0;i<numberTotal_;i++) {
	  if (getInternalStatus(i)==isFixed)
	    continue;
	  largestCost=CoinMax(largestCost,fabs(abcCost_[i]));
	  double dj=abcDj_[i];
	  if (getInternalStatus(i)==atLowerBound)
	    dj=-dj;
	  largestDj=CoinMax(largestDj,fabs(dj));
	  if(largestGoodDj<dj) {
	    largestGoodDj=dj;
	    iLargest=i;
	  }
	  if (getInternalStatus(i)==basic) {
	    if (abcSolution_[i]>abcUpper_[i]+primalTolerance_) {
	      numberInfeasibilities++;
	      sum += abcSolution_[i]-abcUpper_[i]-primalTolerance_;
	    } else if (abcSolution_[i]<abcLower_[i]-primalTolerance_) {
	      numberInfeasibilities++;
	      sum -= abcSolution_[i]-abcLower_[i]+primalTolerance_;
	    }
	  }
	}
	char xx;
	int kLargest;
	if (iLargest>=numberRows_) {
	  xx='C';
	  kLargest=iLargest-numberRows_;
	} else {
	  xx='R';
	  kLargest=iLargest;
	}
	printf("largest cost %g, largest dj %g best %g (%d==%c%d) - %d infeasible (sum %g) nonlininf %d\n",
	       largestCost,largestDj,largestGoodDj,iLargest,xx,kLargest,numberInfeasibilities,sum,
	       abcNonLinearCost_->numberInfeasibilities());
	assert (getInternalStatus(iLargest)!=basic);
      }
#endif
    } else {
      // in values pass
      for (int i=0;i<4;i++)
	multipleSequenceIn_[i]=-1;
      int sequenceIn = nextSuperBasic(superBasicType, &usefulArray_[arrayForFlipBounds_]);
      if (valuesOption > 1)
	superBasicType = 2;
      if (sequenceIn < 0) {
	// end of values pass - initialize weights etc
	sequenceIn_=-1;
	handler_->message(CLP_END_VALUES_PASS, messages_)
	  << numberIterations_;
	stateOfProblem_ &= ~VALUES_PASS;
#ifdef TRY_SPLIT_VALUES_PASS
	valuesStop=numberIterations_+doOrdinaryVariables;
#endif
	abcPrimalColumnPivot_->saveWeights(this, 5);
	problemStatus_ = -2; // factorize now
	pivotRow_ = -1; // say no weights update
	returnCode = -4;
	// Clean up
	for (int i = 0; i < numberTotal_; i++) {
	  if (getInternalStatus(i) == atLowerBound || getInternalStatus(i) == isFixed)
	    abcSolution_[i] = abcLower_[i];
	  else if (getInternalStatus(i) == atUpperBound)
	    abcSolution_[i] = abcUpper_[i];
	}
	break;
      } else {
	// normal
	sequenceIn_ = sequenceIn;
	valueIn_ = abcSolution_[sequenceIn_];
	lowerIn_ = abcLower_[sequenceIn_];
	upperIn_ = abcUpper_[sequenceIn_];
	dualIn_ = abcDj_[sequenceIn_];
	// see if any more
	if (maximumIterations()==100000) {
	  multipleSequenceIn_[0]=sequenceIn;
	  for (int i=1;i<4;i++) {
	    int sequenceIn = nextSuperBasic(superBasicType, &usefulArray_[arrayForFlipBounds_]);
	    if (sequenceIn>=0)
	      multipleSequenceIn_[i]=sequenceIn;
	    else
	      break;
	  }
	}
      }
    }
    pivotRow_ = -1;
    sequenceOut_ = -1;
    usefulArray_[arrayForFtran_].clear();
    if (sequenceIn_ >= 0) {
      // we found a pivot column
      assert (!flagged(sequenceIn_));
      //#define MULTIPLE_PRICE
      // do second half of iteration
      if (multipleSequenceIn_[1]==-1||maximumIterations()!=100000) {
	returnCode = pivotResult(ifValuesPass);
      } else {
	if (multipleSequenceIn_[0]<0)
	  multipleSequenceIn_[0]=sequenceIn_;
	returnCode = pivotResult4(ifValuesPass);
#ifdef MULTIPLE_PRICE
	if (sequenceIn_>=0)
	  returnCode = pivotResult(ifValuesPass);
#endif
      }
      if(numberStartingInfeasibilities&&!abcNonLinearCost_->numberInfeasibilities()) {
	//if (abcFactorization_->pivots()>200&&numberIterations_>2*(numberRows_+numberColumns_))
	if (abcFactorization_->pivots()>2&&numberIterations_>(numberRows_+numberColumns_)&&(stateOfProblem_&PESSIMISTIC)!=0)
	  returnCode=-2; // refactorize - maybe just after n iterations
      }
      if (returnCode < -1 && returnCode > -5) {
	problemStatus_ = -2; //
      } else if (returnCode == -5) {
	if (abcProgress_.timesFlagged()>10+numberFlaggedStart)
	  problemStatus_ =-2;
	if ((moreSpecialOptions_ & 16) == 0 && abcFactorization_->pivots()) {
	  moreSpecialOptions_ |= 16;
	  problemStatus_ = -2;
	}
	// otherwise something flagged - continue;
      } else if (returnCode == 2) {
	problemStatus_ = -5; // looks unbounded
      } else if (returnCode == 4) {
	problemStatus_ = -2; // looks unbounded but has iterated
      } else if (returnCode != -1) {
	assert(returnCode == 3);
	if (problemStatus_ != 5)
	  problemStatus_ = 3;
      }
    } else {
      // no pivot column
#if ABC_NORMAL_DEBUG>3
      if (handler_->logLevel() & 32)
	printf("** no column pivot\n");
#endif
      if (abcNonLinearCost_->numberInfeasibilities())
	problemStatus_ = -4; // might be infeasible
      // Force to re-factorize early next time
      int numberPivots = abcFactorization_->pivots();
      returnCode = 0;
#ifdef CLP_USER_DRIVEN
      // If large number of pivots trap later?
      if (problemStatus_==-1 && numberPivots<13000) {
	int status = eventHandler_->event(ClpEventHandler::noCandidateInPrimal);
	if (status>=0&&status<10) {
	  // carry on
	  problemStatus_=-1;
	  if (status==0) 
	    break;
	} else if (status>=10) {
	  problemStatus_=status-10;
	  break;
	} else {
	  forceFactorization_ = CoinMin(forceFactorization_, (numberPivots + 1) >> 1);
	  break;
	}
      }
#else
      forceFactorization_ = CoinMin(forceFactorization_, (numberPivots + 1) >> 1);
      break;
#endif
    }
  }
  if (valuesOption > 1)
    usefulArray_[arrayForFlipBounds_].setNumElements(0);
  return returnCode;
}
/* Checks if finished.  Updates status */
void
AbcSimplexPrimal::statusOfProblemInPrimal(int type)
{
  int saveFirstFree = firstFree_;
  // number of pivots done
  int numberPivots = abcFactorization_->pivots();
#if 0
  printf("statusOf %d pivots type %d pstatus %d forceFac %d dont %d\n",
  	 numberPivots,type,problemStatus_,forceFactorization_,dontFactorizePivots_);
#endif
  //int saveType=type;
  if (type>=4) {
    // force factorization
    type &= 3;
    numberPivots=9999999;
  }
#ifndef TRY_ABC_GUS
  bool doFactorization =  (type != 3&&(numberPivots>dontFactorizePivots_||numberIterations_==baseIteration_||problemStatus_==10));
#else
  bool doFactorization =  (type != 3&&(numberPivots>dontFactorizePivots_||numberIterations_==baseIteration_));
#endif
  bool doWeights = doFactorization||problemStatus_==10;
  if (type == 2) {
    // trouble - restore solution
    restoreGoodStatus(1);
    forceFactorization_ = 1; // a bit drastic but ..
    pivotRow_ = -1; // say no weights update
    changeMade_++; // say change made
  }
  int tentativeStatus = problemStatus_;
  double lastSumInfeasibility = COIN_DBL_MAX;
  int lastNumberInfeasibility = 1;
#ifndef CLP_CAUTION
#define CLP_CAUTION 1
#endif
#if CLP_CAUTION
  double lastAverageInfeasibility = sumDualInfeasibilities_ /
    static_cast<double>(numberDualInfeasibilities_ + 1);
#endif
  if (numberIterations_&&type) {
    lastSumInfeasibility = abcNonLinearCost_->sumInfeasibilities();
    lastNumberInfeasibility = abcNonLinearCost_->numberInfeasibilities();
  } else {
    lastAverageInfeasibility=1.0e10;
  }
  bool ifValuesPass=(stateOfProblem_&VALUES_PASS)!=0;
  bool takenAction=false;
  double sumInfeasibility=0.0;
  if (problemStatus_ > -3 || problemStatus_ == -4) {
    // factorize
    // later on we will need to recover from singularities
    // also we could skip if first time
    // do weights
    // This may save pivotRow_ for use
    if (doFactorization)
      abcPrimalColumnPivot_->saveWeights(this, 1);
    
    if (!type) {
      // be optimistic
      stateOfProblem_ &= ~PESSIMISTIC;
      // but use 0.1
      double newTolerance = CoinMax(0.1, saveData_.pivotTolerance_);
      abcFactorization_->pivotTolerance(newTolerance);
    }
    if (doFactorization) {
      // is factorization okay?
      int solveType = ifValuesPass ? 11 :1;
      if (!type)
	solveType++;
      int factorStatus = internalFactorize(solveType);
      if (factorStatus) {
	if (type != 1 || largestPrimalError_ > 1.0e3
	    || largestDualError_ > 1.0e3) {
#if ABC_NORMAL_DEBUG>0
	  printf("Bad initial basis\n");
#endif
	  internalFactorize(2);
	} else {
	  // no - restore previous basis
	  // Keep any flagged variables
	  for (int i = 0; i < numberTotal_; i++) {
	    if (flagged(i))
	      internalStatusSaved_[i] |= 64; //say flagged
	  }
	  restoreGoodStatus(1);
	  if (numberPivots <= 1) {
	    // throw out something
	    if (sequenceIn_ >= 0 && getInternalStatus(sequenceIn_) != basic) {
	      setFlagged(sequenceIn_);
	    } else if (sequenceOut_ >= 0 && getInternalStatus(sequenceOut_) != basic) {
	      setFlagged(sequenceOut_);
	    }
	    abcProgress_.incrementTimesFlagged();
	    double newTolerance = CoinMax(0.5 + 0.499 * randomNumberGenerator_.randomDouble(), abcFactorization_->pivotTolerance());
	    abcFactorization_->pivotTolerance(newTolerance);
	  } else {
	    // Go to safe
	    abcFactorization_->pivotTolerance(0.99);
	  }
	  forceFactorization_ = 1; // a bit drastic but ..
	  type = 2;
	  abcNonLinearCost_->checkInfeasibilities();
	  if (internalFactorize(2) != 0) {
	    largestPrimalError_ = 1.0e4; // force other type
	  }
	}
	changeMade_++; // say change made
      }
    }
    if (problemStatus_ != -4)
      problemStatus_ = -3;
    // at this stage status is -3 or -5 if looks unbounded
    // get primal and dual solutions
    // put back original costs and then check
    // createRim(4); // costs do not change
    if (ifValuesPass&&numberIterations_==baseIteration_) {
      abcNonLinearCost_->checkInfeasibilities(primalTolerance_);
      lastSumInfeasibility = abcNonLinearCost_->largestInfeasibility();
    }
#ifdef CLP_USER_DRIVEN
    int status = eventHandler_->event(ClpEventHandler::goodFactorization);
    if (status >= 0) {
      lastSumInfeasibility = COIN_DBL_MAX;
    }
#endif
    if (ifValuesPass&&numberIterations_==baseIteration_) {
      double * save = new double[numberRows_];
      for (int iRow = 0; iRow < numberRows_; iRow++) {
	int iPivot = abcPivotVariable_[iRow];
	save[iRow] = abcSolution_[iPivot];
      }
      int numberOut = 1;
      while (numberOut) {
	gutsOfPrimalSolution(2);
	double badInfeasibility = abcNonLinearCost_->largestInfeasibility();
	numberOut = 0;
	// But may be very large rhs etc
	double useError = CoinMin(largestPrimalError_,
				  1.0e5 / CoinAbcMaximumAbsElement(abcSolution_, numberTotal_));
	if ((lastSumInfeasibility < incomingInfeasibility_ || badInfeasibility >
	     (CoinMax(10.0, 100.0 * lastSumInfeasibility)))
	    && (badInfeasibility > 10.0 ||useError > 1.0e-3)) {
	  //printf("Original largest infeas %g, now %g, primalError %g\n",
	  //     lastSumInfeasibility,abcNonLinearCost_->largestInfeasibility(),
	  //     largestPrimalError_);
	  // throw out up to 1000 structurals
	  int * sort = new int[numberRows_];
	  // first put back solution and store difference
	  for (int iRow = 0; iRow < numberRows_; iRow++) {
	    int iPivot = abcPivotVariable_[iRow];
	    double difference = fabs(abcSolution_[iPivot] - save[iRow]);
	    abcSolution_[iPivot] = save[iRow];
	    save[iRow] = difference;
	  }
	  abcNonLinearCost_->checkInfeasibilities(primalTolerance_);
	  printf("Largest infeasibility %g\n",abcNonLinearCost_->largestInfeasibility());
	  int numberBasic = 0;
	  for (int iRow = 0; iRow < numberRows_; iRow++) {
	    int iPivot = abcPivotVariable_[iRow];
	    if (iPivot >= numberRows_) {
	      // column
	      double difference = save[iRow];
	      if (difference > 1.0e-4) {
		sort[numberOut] = iRow;
		save[numberOut++] = -difference;
		if (getInternalStatus(iPivot) == basic)
		  numberBasic++;
	      }
	    }
	  }
	  if (!numberBasic) {
	    //printf("no errors on basic - going to all slack - numberOut %d\n",numberOut);
	    // allow
	    numberOut = 0;
	  }
	  CoinSort_2(save, save + numberOut, sort);
	  numberOut = CoinMin(1000, numberOut);
	  // for now bring in any slack
	  int jRow=0;
	  for (int i = 0; i < numberOut; i++) {
	    int iRow = sort[i];
	    int iColumn = abcPivotVariable_[iRow];
	    assert (getInternalStatus(iColumn)==basic);
	    setInternalStatus(iColumn, superBasic);
	    while (getInternalStatus(jRow)==basic)
	      jRow++;
	    setInternalStatus(jRow, basic);
	    abcPivotVariable_[iRow] = jRow;
	    if (fabs(abcSolution_[iColumn]) > 1.0e10) {
	      if (abcUpper_[iColumn] < 0.0) {
		abcSolution_[iColumn] = abcUpper_[iColumn];
	      } else if (abcLower_[iColumn] > 0.0) {
		abcSolution_[iColumn] = abcLower_[iColumn];
	      } else {
		abcSolution_[iColumn] = 0.0;
	      }
	    }
	  }
	  delete [] sort;
	}
	if (numberOut) {
	  int factorStatus = internalFactorize(12);
	  assert (!factorStatus);
	}
      }
      delete [] save;
    }
    gutsOfPrimalSolution(3);
    sumInfeasibility =  abcNonLinearCost_->sumInfeasibilities();
    int reason2 = 0;
#if CLP_CAUTION
#if CLP_CAUTION==2
    double test2 = 1.0e5;
#else
    double test2 = 1.0e-1;
#endif
    if (!lastSumInfeasibility && sumInfeasibility>1.0e3*primalTolerance_ &&
	lastAverageInfeasibility < test2 && numberPivots > 10&&!ifValuesPass)
      reason2 = 3;
    if (lastSumInfeasibility < 1.0e-6 && sumInfeasibility > 1.0e-3 &&
	numberPivots > 10&&!ifValuesPass)
      reason2 = 4;
#endif
    if ((sumInfeasibility > 1.0e7 && sumInfeasibility > 100.0 * lastSumInfeasibility
	 && abcFactorization_->pivotTolerance() < 0.11) ||
	(largestPrimalError_ > 1.0e10 && largestDualError_ > 1.0e10))
      reason2 = 2;
    if (reason2) {
      takenAction=true;
      problemStatus_ = tentativeStatus;
      doFactorization = true;
      if (numberPivots) {
	// go back
	// trouble - restore solution
	restoreGoodStatus(1);
	sequenceIn_=-1;
	sequenceOut_=-1;
	abcNonLinearCost_->checkInfeasibilities();
	if (reason2 < 3) {
	  // Go to safe
	  abcFactorization_->pivotTolerance(CoinMin(0.99, 1.01 * abcFactorization_->pivotTolerance()));
	  forceFactorization_ = 1; // a bit drastic but ..
	} else if (forceFactorization_ < 0) {
	  forceFactorization_ = CoinMin(numberPivots / 4, 100);
	} else {
	  forceFactorization_ = CoinMin(forceFactorization_,
					CoinMax(3, numberPivots / 4));
	}
	pivotRow_ = -1; // say no weights update
	changeMade_++; // say change made
	if (numberPivots == 1) {
	  // throw out something
	  if (sequenceIn_ >= 0 && getInternalStatus(sequenceIn_) != basic) {
	    setFlagged(sequenceIn_);
	  } else if (sequenceOut_ >= 0 && getInternalStatus(sequenceOut_) != basic) {
	    setFlagged(sequenceOut_);
	  }
	  abcProgress_.incrementTimesFlagged();
	}
	type = 2; // so will restore weights
	if (internalFactorize(2) != 0) {
	  largestPrimalError_ = 1.0e4; // force other type
	}
	numberPivots = 0;
	gutsOfPrimalSolution(3);
	sumInfeasibility =  abcNonLinearCost_->sumInfeasibilities();
      }
    }
  }
  // Double check reduced costs if no action
  if (abcProgress_.lastIterationNumber(0) == numberIterations_) {
    if (abcPrimalColumnPivot_->looksOptimal()) {
      numberDualInfeasibilities_ = 0;
      sumDualInfeasibilities_ = 0.0;
    }
  }
  // If in primal and small dj give up
  if ((specialOptions_ & 1024) != 0 && !numberPrimalInfeasibilities_ && numberDualInfeasibilities_) {
    double average = sumDualInfeasibilities_ / (static_cast<double> (numberDualInfeasibilities_));
    if (numberIterations_ > 300 && average < 1.0e-4) {
      numberDualInfeasibilities_ = 0;
      sumDualInfeasibilities_ = 0.0;
    }
  }
  // Check if looping
  int loop;
  ifValuesPass=firstFree_>=0;
  if (type != 2 && !ifValuesPass)
    loop = abcProgress_.looping();
  else
    loop = -1;
  if ((moreSpecialOptions_ & 2048) != 0 && !numberPrimalInfeasibilities_ && numberDualInfeasibilities_) {
    double average = sumDualInfeasibilities_ / (static_cast<double> (numberDualInfeasibilities_));
    if (abcProgress_.lastIterationNumber(2)==numberIterations_&&
	((abcProgress_.timesFlagged()>2&&average < 1.0e-1)||
	 abcProgress_.timesFlagged()>20)) {
      numberDualInfeasibilities_ = 0;
      sumDualInfeasibilities_ = 0.0;
      problemStatus_=3;
      loop=0;
    }
  }
  if (loop >= 0) {
    if (!problemStatus_) {
      // declaring victory
      numberPrimalInfeasibilities_ = 0;
      sumPrimalInfeasibilities_ = 0.0;
      // say been feasible
      abcState_ |= CLP_ABC_BEEN_FEASIBLE;
    } else {
      problemStatus_ = loop; //exit if in loop
      problemStatus_ = 10; // instead - try other algorithm
      numberPrimalInfeasibilities_ = abcNonLinearCost_->numberInfeasibilities();
    }
    problemStatus_ = 10; // instead - try other algorithm
    return ;
  } else if (loop < -1) {
    // Is it time for drastic measures
    if (abcNonLinearCost_->numberInfeasibilities() && abcProgress_.badTimes() > 5 &&
	abcProgress_.oddState() < 10 && abcProgress_.oddState() >= 0) {
      abcProgress_.newOddState();
      abcNonLinearCost_->zapCosts();
    }
    // something may have changed
    gutsOfPrimalSolution(3);
  }
  // If progress then reset costs
  if (loop == -1 && !abcNonLinearCost_->numberInfeasibilities() && abcProgress_.oddState() < 0) {
    copyFromSaved(); // costs back
    delete abcNonLinearCost_;
    abcNonLinearCost_ = new AbcNonLinearCost(this);
    abcProgress_.endOddState();
    gutsOfPrimalSolution(3);
  }
  abcProgress_.modifyObjective(abcNonLinearCost_->feasibleReportCost());
  if (!lastNumberInfeasibility && sumInfeasibility && numberPivots > 1&&!ifValuesPass&&
      !takenAction&&
      abcProgress_.lastObjective(0)>=abcProgress_.lastObjective(CLP_PROGRESS-1)) {
    // first increase minimumThetaMovement_;
    // be more careful
    //abcFactorization_->saferTolerances (-0.99, -1.002);
#if ABC_NORMAL_DEBUG>0
    if (handler_->logLevel() == 63)
      printf("thought feasible but sum is %g force %d minimum theta %g\n", sumInfeasibility, 
	     forceFactorization_,minimumThetaMovement_);
#endif
    stateOfProblem_ |= PESSIMISTIC;
    if (minimumThetaMovement_<1.0e-10) {
      minimumThetaMovement_ *= 1.1;
    } else if (0) {
      int maxFactor = abcFactorization_->maximumPivots();
      if (maxFactor > 10) {
	if (forceFactorization_ < 0)
	  forceFactorization_ = maxFactor;
	forceFactorization_ = CoinMax(1, (forceFactorization_ >> 2));
	if (handler_->logLevel() == 63)
	  printf("Reducing factorization frequency to %d\n", forceFactorization_);
      }
    }
  }
  // Flag to say whether to go to dual to clean up
  bool goToDual = false;
  // really for free variables in
  //if((progressFlag_&2)!=0)
  //problemStatus_=-1;;
  progressFlag_ = 0; //reset progress flag
  
  handler_->message(CLP_SIMPLEX_STATUS, messages_)
    << numberIterations_ << abcNonLinearCost_->feasibleReportCost()+objectiveOffset_;
  handler_->printing(abcNonLinearCost_->numberInfeasibilities() > 0)
    << abcNonLinearCost_->sumInfeasibilities() << abcNonLinearCost_->numberInfeasibilities();
  handler_->printing(sumDualInfeasibilities_ > 0.0)
    << sumDualInfeasibilities_ << numberDualInfeasibilities_;
  handler_->printing(numberDualInfeasibilitiesWithoutFree_
		     < numberDualInfeasibilities_)
    << numberDualInfeasibilitiesWithoutFree_;
  handler_->message() << CoinMessageEol;
  if (!primalFeasible()) {
    abcNonLinearCost_->checkInfeasibilities(primalTolerance_);
    gutsOfPrimalSolution(2);
    abcNonLinearCost_->checkInfeasibilities(primalTolerance_);
  }
  if (abcNonLinearCost_->numberInfeasibilities() > 0 && !abcProgress_.initialWeight_ && !ifValuesPass && infeasibilityCost_ == 1.0e10 
#ifdef TRY_SPLIT_VALUES_PASS
      && valuesStop<0
#endif
      ) {
    // first time infeasible - start up weight computation
    // compute with original costs
    CoinAbcMemcpy(djSaved_,abcDj_,numberTotal_);
    CoinAbcMemcpy(abcCost_,costSaved_,numberTotal_);
    gutsOfPrimalSolution(1);
    int numberSame = 0;
    int numberDifferent = 0;
    int numberZero = 0;
    int numberFreeSame = 0;
    int numberFreeDifferent = 0;
    int numberFreeZero = 0;
    int n = 0;
    for (int iSequence = 0; iSequence < numberTotal_; iSequence++) {
      if (getInternalStatus(iSequence) != basic && !flagged(iSequence)) {
	// not basic
	double distanceUp = abcUpper_[iSequence] - abcSolution_[iSequence];
	double distanceDown = abcSolution_[iSequence] - abcLower_[iSequence];
	double feasibleDj = abcDj_[iSequence];
	double infeasibleDj = djSaved_[iSequence] - feasibleDj;
	double value = feasibleDj * infeasibleDj;
	if (distanceUp > primalTolerance_) {
	  // Check if "free"
	  if (distanceDown > primalTolerance_) {
	    // free
	    if (value > dualTolerance_) {
	      numberFreeSame++;
	    } else if(value < -dualTolerance_) {
	      numberFreeDifferent++;
	      abcDj_[n++] = feasibleDj / infeasibleDj;
	    } else {
	      numberFreeZero++;
	    }
	  } else {
	    // should not be negative
	    if (value > dualTolerance_) {
	      numberSame++;
	    } else if(value < -dualTolerance_) {
	      numberDifferent++;
	      abcDj_[n++] = feasibleDj / infeasibleDj;
	    } else {
	      numberZero++;
	    }
	  }
	} else if (distanceDown > primalTolerance_) {
	  // should not be positive
	  if (value > dualTolerance_) {
	    numberSame++;
	  } else if(value < -dualTolerance_) {
	    numberDifferent++;
	    abcDj_[n++] = feasibleDj / infeasibleDj;
	  } else {
	    numberZero++;
	  }
	}
      }
      abcProgress_.initialWeight_ = -1.0;
    }
    //printf("XXXX %d same, %d different, %d zero, -- free %d %d %d\n",
    //   numberSame,numberDifferent,numberZero,
    //   numberFreeSame,numberFreeDifferent,numberFreeZero);
    // we want most to be same
    if (n) {
      double most = 0.95;
      std::sort(abcDj_, abcDj_ + n);
      int which = static_cast<int> ((1.0 - most) * static_cast<double> (n));
      double take = -abcDj_[which] * infeasibilityCost_;
      //printf("XXXXZ inf cost %g take %g (range %g %g)\n",infeasibilityCost_,take,-abcDj_[0]*infeasibilityCost_,-abcDj_[n-1]*infeasibilityCost_);
      take = -abcDj_[0] * infeasibilityCost_;
      infeasibilityCost_ = CoinMin(CoinMax(1000.0 * take, 1.0e8), 1.0000001e10);;
      //printf("XXXX increasing weight to %g\n",infeasibilityCost_);
    }
    abcNonLinearCost_->checkInfeasibilities(0.0);
    gutsOfPrimalSolution(3);
  }
  double trueInfeasibility = abcNonLinearCost_->sumInfeasibilities();
  if (!abcNonLinearCost_->numberInfeasibilities() && infeasibilityCost_ == 1.0e10 && !ifValuesPass && true) {
    // say been feasible
    abcState_ |= CLP_ABC_BEEN_FEASIBLE;
    // relax if default
    infeasibilityCost_ = CoinMin(CoinMax(100.0 * sumDualInfeasibilities_, 1.0e8), 1.00000001e10);
    // reset looping criterion
    abcProgress_.reset();
    trueInfeasibility = 1.123456e10;
  }
  if (trueInfeasibility > 1.0) {
    // If infeasibility going up may change weights
    double testValue = trueInfeasibility - 1.0e-4 * (10.0 + trueInfeasibility);
    double lastInf = abcProgress_.lastInfeasibility(1);
    double lastInf3 = abcProgress_.lastInfeasibility(3);
    double thisObj = abcProgress_.lastObjective(0);
    double thisInf = abcProgress_.lastInfeasibility(0);
    thisObj += infeasibilityCost_ * 2.0 * thisInf;
    double lastObj = abcProgress_.lastObjective(1);
    lastObj += infeasibilityCost_ * 2.0 * lastInf;
    double lastObj3 = abcProgress_.lastObjective(3);
    lastObj3 += infeasibilityCost_ * 2.0 * lastInf3;
    if (lastObj < thisObj - 1.0e-5 * CoinMax(fabs(thisObj), fabs(lastObj)) - 1.0e-7
	&& firstFree_ < 0) {
#if ABC_NORMAL_DEBUG>0
      if (handler_->logLevel() == 63)
	printf("lastobj %g this %g force %d\n", lastObj, thisObj, forceFactorization_);
#endif
      int maxFactor = abcFactorization_->maximumPivots();
      if (maxFactor > 10) {
	if (forceFactorization_ < 0)
	  forceFactorization_ = maxFactor;
	forceFactorization_ = CoinMax(1, (forceFactorization_ >> 2));
#if ABC_NORMAL_DEBUG>0
	if (handler_->logLevel() == 63)
	  printf("Reducing factorization frequency to %d\n", forceFactorization_);
#endif
      }
    } else if (lastObj3 < thisObj - 1.0e-5 * CoinMax(fabs(thisObj), fabs(lastObj3)) - 1.0e-7
	       && firstFree_ < 0) {
#if ABC_NORMAL_DEBUG>0
      if (handler_->logLevel() == 63)
	printf("lastobj3 %g this3 %g force %d\n", lastObj3, thisObj, forceFactorization_);
#endif
      int maxFactor = abcFactorization_->maximumPivots();
      if (maxFactor > 10) {
	if (forceFactorization_ < 0)
	  forceFactorization_ = maxFactor;
	forceFactorization_ = CoinMax(1, (forceFactorization_ * 2) / 3);
#if ABC_NORMAL_DEBUG>0
	if (handler_->logLevel() == 63)
	  printf("Reducing factorization frequency to %d\n", forceFactorization_);
#endif
      }
    } else if(lastInf < testValue || trueInfeasibility == 1.123456e10) {
      if (infeasibilityCost_ < 1.0e14) {
	infeasibilityCost_ *= 1.5;
	// reset looping criterion
	abcProgress_.reset();
#if ABC_NORMAL_DEBUG>0
	if (handler_->logLevel() == 63)
	  printf("increasing weight to %g\n", infeasibilityCost_);
#endif
	gutsOfPrimalSolution(3);
      }
    }
  }
  // we may wish to say it is optimal even if infeasible
  bool alwaysOptimal = (specialOptions_ & 1) != 0;
#if CLP_CAUTION
  // If twice nearly there ...
  if (lastAverageInfeasibility<2.0*dualTolerance_) {
    double averageInfeasibility = sumDualInfeasibilities_ /
      static_cast<double>(numberDualInfeasibilities_ + 1);
    printf("last av %g now %g\n",lastAverageInfeasibility,
	   averageInfeasibility);
    if (averageInfeasibility<2.0*dualTolerance_) 
      sumOfRelaxedDualInfeasibilities_ = 0.0;
  }
#endif
  // give code benefit of doubt
  if (sumOfRelaxedDualInfeasibilities_ == 0.0 &&
      sumOfRelaxedPrimalInfeasibilities_ == 0.0) {
    // say been feasible
    abcState_ |= CLP_ABC_BEEN_FEASIBLE;
    // say optimal (with these bounds etc)
    numberDualInfeasibilities_ = 0;
    sumDualInfeasibilities_ = 0.0;
    numberPrimalInfeasibilities_ = 0;
    sumPrimalInfeasibilities_ = 0.0;
    // But if real primal infeasibilities nonzero carry on
    if (abcNonLinearCost_->numberInfeasibilities()) {
      // most likely to happen if infeasible
      double relaxedToleranceP = primalTolerance_;
      // we can't really trust infeasibilities if there is primal error
      double error = CoinMin(1.0e-2, largestPrimalError_);
      // allow tolerance at least slightly bigger than standard
      relaxedToleranceP = relaxedToleranceP +  error;
      int ninfeas = abcNonLinearCost_->numberInfeasibilities();
      double sum = abcNonLinearCost_->sumInfeasibilities();
      double average = sum / static_cast<double> (ninfeas);
#if ABC_NORMAL_DEBUG>3
      if (handler_->logLevel() > 0)
	printf("nonLinearCost says infeasible %d summing to %g\n",
	       ninfeas, sum);
#endif
      if (average > relaxedToleranceP) {
	sumOfRelaxedPrimalInfeasibilities_ = sum;
	numberPrimalInfeasibilities_ = ninfeas;
	sumPrimalInfeasibilities_ = sum;
#if ABC_NORMAL_DEBUG>3
	bool unflagged =
#endif
	  unflag();
	abcProgress_.clearTimesFlagged();
#if ABC_NORMAL_DEBUG>3
	if (unflagged && handler_->logLevel() > 0)
	  printf(" - but flagged variables\n");
#endif
      }
    }
  }
  //double saveSum=sumOfRelaxedDualInfeasibilities_;
  // had ||(type==3&&problemStatus_!=-5) -- ??? why ????
  if ((dualFeasible() || problemStatus_ == -4) && !ifValuesPass) {
    // see if extra helps
    if (abcNonLinearCost_->numberInfeasibilities() &&
	(abcNonLinearCost_->sumInfeasibilities() > 1.0e-3 || sumOfRelaxedPrimalInfeasibilities_)
	&& !alwaysOptimal) {
      //may need infeasiblity cost changed
      // we can see if we can construct a ray
      // do twice to make sure Primal solution has settled
      // put non-basics to bounds in case tolerance moved
      // put back original costs
      CoinAbcMemcpy(abcCost_,costSaved_,numberTotal_);;
      abcNonLinearCost_->checkInfeasibilities(0.0);
      gutsOfPrimalSolution(3);
      // put back original costs
      CoinAbcMemcpy(abcCost_,costSaved_,numberTotal_);;
      abcNonLinearCost_->checkInfeasibilities(primalTolerance_);
      gutsOfPrimalSolution(3);
      // may have fixed infeasibilities - double check
      if (abcNonLinearCost_->numberInfeasibilities() == 0) {
	// carry on
	problemStatus_ = -1;
	abcNonLinearCost_->checkInfeasibilities(primalTolerance_);
      } else {
	if ((infeasibilityCost_ >= 1.0e18 ||
	     numberDualInfeasibilities_ == 0) && perturbation_ == 101
	    && (specialOptions_&8192)==0) {
	  goToDual = unPerturb(); // stop any further perturbation
#ifndef TRY_ABC_GUS
	  if (abcNonLinearCost_->sumInfeasibilities() > 1.0e-1)
	    goToDual = false;
#endif
	  numberDualInfeasibilities_ = 1; // carry on
	  problemStatus_ = -1;
	} else if (numberDualInfeasibilities_ == 0 && largestDualError_ > 1.0e-2 
#ifndef TRY_ABC_GUS
		   &&((moreSpecialOptions_ & 256) == 0&&(specialOptions_ & 8192) == 0)
#else
		   &&(specialOptions_ & 8192) == 0
#endif
		   ) {
	  goToDual = true;
	  abcFactorization_->pivotTolerance(CoinMax(0.5, abcFactorization_->pivotTolerance()));
	}
	if (!goToDual) {
	  if (infeasibilityCost_ >= 1.0e20 ||
	      numberDualInfeasibilities_ == 0) {
	    // we are infeasible - use as ray
	    delete [] ray_;
	    ray_ = new double [numberRows_];
	    CoinMemcpyN(dual_, numberRows_, ray_);
	    // and get feasible duals
	    infeasibilityCost_ = 0.0;
	    CoinAbcMemcpy(abcCost_,costSaved_,numberTotal_);;
	    abcNonLinearCost_->checkInfeasibilities(primalTolerance_);
	    gutsOfPrimalSolution(3);
	    // so will exit
	    infeasibilityCost_ = 1.0e30;
	    // reset infeasibilities
	    sumPrimalInfeasibilities_ = abcNonLinearCost_->sumInfeasibilities();;
	    numberPrimalInfeasibilities_ =
	      abcNonLinearCost_->numberInfeasibilities();
	  }
	  if (infeasibilityCost_ < 1.0e20) {
	    infeasibilityCost_ *= 5.0;
	    // reset looping criterion
	    abcProgress_.reset();
	    changeMade_++; // say change made
	    handler_->message(CLP_PRIMAL_WEIGHT, messages_)
	      << infeasibilityCost_
	      << CoinMessageEol;
	    // put back original costs and then check
	    CoinAbcMemcpy(abcCost_,costSaved_,numberTotal_);;
	    abcNonLinearCost_->checkInfeasibilities(0.0);
	    gutsOfPrimalSolution(3);
	    problemStatus_ = -1; //continue
#ifndef TRY_ABC_GUS
	    goToDual = false;
#else
	    if((specialOptions_&8192)==0&&!sumOfRelaxedDualInfeasibilities_)
	      goToDual=true;
#endif
	  } else {
	    // say infeasible
	    problemStatus_ = 1;
	  }
	}
      }
    } else {
      // may be optimal
      if (perturbation_ == 101) {
	goToDual = unPerturb(); // stop any further perturbation
#ifndef TRY_ABC_GUS
	if ((numberRows_ > 20000 || numberDualInfeasibilities_) && !numberTimesOptimal_)
#else
	  if ((specialOptions_&8192)!=0)
#endif
	  goToDual = false; // Better to carry on a bit longer
	lastCleaned_ = -1; // carry on
      }
      bool unflagged = (unflag() != 0);
      abcProgress_.clearTimesFlagged();
      if ( lastCleaned_ != numberIterations_ || unflagged) {
	handler_->message(CLP_PRIMAL_OPTIMAL, messages_)
	  << primalTolerance_
	  << CoinMessageEol;
	if (numberTimesOptimal_ < 4) {
	  numberTimesOptimal_++;
	  changeMade_++; // say change made
	  if (numberTimesOptimal_ == 1) {
	    // better to have small tolerance even if slower
	    abcFactorization_->zeroTolerance(CoinMin(abcFactorization_->zeroTolerance(), 1.0e-15));
	  }
	  lastCleaned_ = numberIterations_;
	  if (primalTolerance_ != dblParam_[ClpPrimalTolerance])
	    handler_->message(CLP_PRIMAL_ORIGINAL, messages_)
	      << CoinMessageEol;
	  double oldTolerance = primalTolerance_;
	  primalTolerance_ = dblParam_[ClpPrimalTolerance];
	  // put back original costs and then check
#if 0 //ndef NDEBUG
	  double largestDifference=0.0;
	  for (int i=0;i<numberTotal_;i++) 
	    largestDifference=CoinMax(largestDifference,fabs(abcCost_[i]-costSaved_[i]));
	  if (handler_->logLevel()==63)
	    printf("largest change in cost %g\n",largestDifference);
#endif
	  CoinAbcMemcpy(abcCost_,costSaved_,numberTotal_);;
	  abcNonLinearCost_->checkInfeasibilities(oldTolerance);
	  gutsOfPrimalSolution(3);
	  if (sumOfRelaxedDualInfeasibilities_ == 0.0 &&
	      sumOfRelaxedPrimalInfeasibilities_ == 0.0) {
	    // say optimal (with these bounds etc)
	    numberDualInfeasibilities_ = 0;
	    sumDualInfeasibilities_ = 0.0;
	    numberPrimalInfeasibilities_ = 0;
	    sumPrimalInfeasibilities_ = 0.0;
	    goToDual=false;
	  }
	  if (dualFeasible() && !abcNonLinearCost_->numberInfeasibilities() && lastCleaned_ >= 0)
	    problemStatus_ = 0;
	  else
	    problemStatus_ = -1;
	} else {
	  problemStatus_ = 0; // optimal
	  if (lastCleaned_ < numberIterations_) {
	    handler_->message(CLP_SIMPLEX_GIVINGUP, messages_)
	      << CoinMessageEol;
	  }
	}
      } else {
	if (!alwaysOptimal || !sumOfRelaxedPrimalInfeasibilities_)
	  problemStatus_ = 0; // optimal
	else
	  problemStatus_ = 1; // infeasible
      }
    }
  } else {
    // see if looks unbounded
    if (problemStatus_ == -5) {
      if (abcNonLinearCost_->numberInfeasibilities()) {
	if (infeasibilityCost_ > 1.0e18 && perturbation_ == 101) {
	  // back off weight
	  infeasibilityCost_ = 1.0e13;
	  // reset looping criterion
	  abcProgress_.reset();
	  unPerturb(); // stop any further perturbation
	}
	//we need infeasiblity cost changed
	if (infeasibilityCost_ < 1.0e20) {
	  infeasibilityCost_ *= 5.0;
	  // reset looping criterion
	  abcProgress_.reset();
	  changeMade_++; // say change made
	  handler_->message(CLP_PRIMAL_WEIGHT, messages_)
	    << infeasibilityCost_
	    << CoinMessageEol;
	  // put back original costs and then check
	  CoinAbcMemcpy(abcCost_,costSaved_,numberTotal_);;
	  gutsOfPrimalSolution(3);
	  problemStatus_ = -1; //continue
	} else {
	  // say infeasible
	  problemStatus_ = 1;
	  // we are infeasible - use as ray
	  delete [] ray_;
	  ray_ = new double [numberRows_];
	  CoinMemcpyN(dual_, numberRows_, ray_);
	}
      } else {
	// say unbounded
	problemStatus_ = 2;
      }
    } else {
      // carry on
      problemStatus_ = -1;
      if(type == 3 && !ifValuesPass) {
	//bool unflagged =
	unflag();
	abcProgress_.clearTimesFlagged();
	if (sumDualInfeasibilities_ < 1.0e-3 ||
	    (sumDualInfeasibilities_ / static_cast<double> (numberDualInfeasibilities_)) < 1.0e-5 ||
	    abcProgress_.lastIterationNumber(0) == numberIterations_) {
	  if (!numberPrimalInfeasibilities_) {
	    if (numberTimesOptimal_ < 4) {
	      numberTimesOptimal_++;
	      changeMade_++; // say change made
	    } else {
	      problemStatus_ = 0;
	      secondaryStatus_ = 5;
	    }
	  }
	}
      }
    }
  }
  if (problemStatus_ == 0) {
    double objVal = (abcNonLinearCost_->feasibleCost()
		     + objective_->nonlinearOffset());
    objVal /= (objectiveScale_ * rhsScale_);
    double tol = 1.0e-10 * CoinMax(fabs(objVal), fabs(objectiveValue_)) + 1.0e-8;
    if (fabs(objVal - objectiveValue_) > tol) {
#if ABC_NORMAL_DEBUG>3
      if (handler_->logLevel() > 0)
	printf("nonLinearCost has feasible obj of %g, objectiveValue_ is %g\n",
	       objVal, objectiveValue_);
#endif
      objectiveValue_ = objVal;
    }
  }
  if (type == 0 || type == 1) {
    saveGoodStatus();
  }
  // see if in Cbc etc
  bool inCbcOrOther = (specialOptions_ & 0x03000000) != 0;
  bool disaster = false;
  if (disasterArea_ && inCbcOrOther && disasterArea_->check()) {
    disasterArea_->saveInfo();
    disaster = true;
  }
  if (disaster)
    problemStatus_ = 3;
  if (problemStatus_ < 0 && !changeMade_) {
    problemStatus_ = 4; // unknown
  }
  lastGoodIteration_ = numberIterations_;
  if (numberIterations_ > lastBadIteration_ + 100)
    moreSpecialOptions_ &= ~16; // clear check accuracy flag
#ifndef TRY_ABC_GUS
  if ((moreSpecialOptions_ & 256) != 0||saveSum||(specialOptions_ & 8192) != 0)
    goToDual=false;
#else
  if ((specialOptions_ & 8192) != 0)
    goToDual=false;
#endif
  if (goToDual || (numberIterations_ > 1000+baseIteration_ && largestPrimalError_ > 1.0e6
		   && largestDualError_ > 1.0e6)) {
    problemStatus_ = 10; // try dual
    // See if second call
    if ((moreSpecialOptions_ & 256) != 0||abcNonLinearCost_->sumInfeasibilities()>1.0e20) {
      numberPrimalInfeasibilities_ = abcNonLinearCost_->numberInfeasibilities();
      sumPrimalInfeasibilities_ = abcNonLinearCost_->sumInfeasibilities();
      // say infeasible
      if (numberPrimalInfeasibilities_&&(abcState_&CLP_ABC_BEEN_FEASIBLE)==0)
	problemStatus_ = 1;
    }
  }
  // make sure first free monotonic
  if (firstFree_ >= 0 && saveFirstFree >= 0) {
    firstFree_ = (numberIterations_) ? saveFirstFree : -1;
    nextSuperBasic(1, NULL);
  }
#ifdef TRY_SPLIT_VALUES_PASS
  if (valuesChunk>0) {
    bool doFixing=firstFree_<0;
    if (numberIterations_==baseIteration_&&
	numberDualInfeasibilitiesWithoutFree_+1000<numberDualInfeasibilities_) {
      doFixing=true;
      int numberSuperBasic=0;
      for (int iSequence = 0; iSequence < numberTotal_; iSequence++) {
	if (getInternalStatus(iSequence) == superBasic)
	  numberSuperBasic++;
      }
      keepValuesPass=static_cast<int>((1.01*numberSuperBasic)/valuesChunk);
      doOrdinaryVariables=static_cast<int>(valuesRatio*keepValuesPass);
    } else if (valuesStop>0) {
      if (numberIterations_>=valuesStop||problemStatus_>=0) {
	gutsOfSolution(3);
	abcNonLinearCost_->refreshFromPerturbed(primalTolerance_);
	gutsOfSolution(3);
	int numberSuperBasic=0;
	for (int iSequence = 0; iSequence < numberTotal_; iSequence++) {
	  if (getInternalStatus(iSequence) == isFixed) {
	    if (abcUpper_[iSequence]>abcLower_[iSequence]) {
	      setInternalStatus(iSequence,superBasic);
	      numberSuperBasic++;
	    }
	  }
	}
	if (numberSuperBasic) {
	  stateOfProblem_ |= VALUES_PASS;
	  problemStatus_=-1;
	  gutsOfSolution(3);
	} else {
	  doFixing=false;
	}
	valuesStop=-1;
      } else {
	doFixing=false;
      }
    } 
    if (doFixing) {
      abcNonLinearCost_->refreshFromPerturbed(primalTolerance_);
	int numberSuperBasic=0;
      for (int iSequence = 0; iSequence < numberTotal_; iSequence++) {
	if (getInternalStatus(iSequence) == isFixed) {
	  if (abcUpper_[iSequence]>abcLower_[iSequence]) 
	    setInternalStatus(iSequence,superBasic);
	}
	if (getInternalStatus(iSequence) == superBasic) 
	  numberSuperBasic++;
      }
      int numberFixed=0;
      firstFree_=-1;
      if (numberSuperBasic) {
	double threshold = static_cast<double>(keepValuesPass)/numberSuperBasic;
	if (1.1*keepValuesPass>numberSuperBasic) 
	  threshold=1.1;
	for (int iSequence = 0; iSequence < numberTotal_; iSequence++) {
	  if (getInternalStatus(iSequence) == superBasic) {
	    if (randomNumberGenerator_.randomDouble()<threshold) {
	      if (firstFree_<0)
		firstFree_=iSequence;
	    } else {
	      numberFixed++;
	      abcLower_[iSequence]=abcSolution_[iSequence];
	      abcUpper_[iSequence]=abcSolution_[iSequence];
	      setInternalStatus(iSequence,isFixed);
	    }
	  }
	}
      }
      if (!numberFixed) {
	// end
	valuesChunk=-1;
      }
    }
  }
#endif
  if (doFactorization||doWeights) {
    // restore weights (if saved) - also recompute infeasibility list
    if (tentativeStatus > -3)
      abcPrimalColumnPivot_->saveWeights(this, (type < 2) ? 2 : 4);
    else
      abcPrimalColumnPivot_->saveWeights(this, 3);
  }
}
// Computes solutions - 1 do duals, 2 do primals, 3 both
int
AbcSimplex::gutsOfPrimalSolution(int type)
{
  //work space
  int whichArray[2];
  for (int i=0;i<2;i++)
    whichArray[i]=getAvailableArray();
  CoinIndexedVector * array1 = &usefulArray_[whichArray[0]];
  CoinIndexedVector * array2 = &usefulArray_[whichArray[1]];
  // do work and check
  int numberRefinements=0;
  if ((type&2)!=0) {
    numberRefinements=computePrimals(array1,array2);
    if (algorithm_ > 0 && abcNonLinearCost_ != NULL) {
      // primal algorithm
      // get correct bounds on all variables
      abcNonLinearCost_->checkInfeasibilities(primalTolerance_);
      if (abcNonLinearCost_->numberInfeasibilities())
	if (handler_->detail(CLP_SIMPLEX_NONLINEAR, messages_) < 100) {
	  handler_->message(CLP_SIMPLEX_NONLINEAR, messages_)
	    << abcNonLinearCost_->changeInCost()
	    << abcNonLinearCost_->numberInfeasibilities()
	    << CoinMessageEol;
	}
    }
    checkPrimalSolution(true);
  }
  if ((type&1)!=0
#if CAN_HAVE_ZERO_OBJ>1
      &&(specialOptions_&2097152)==0
#endif
      ) {
    numberRefinements+=computeDuals(NULL,array1,array2);
    checkDualSolution();
  }
  for (int i=0;i<2;i++)
    setAvailableArray(whichArray[i]);
  rawObjectiveValue_ +=sumNonBasicCosts_;
  //computeObjective(); // ? done in checkDualSolution??
  setClpSimplexObjectiveValue();
  if (handler_->logLevel() > 3 || (largestPrimalError_ > 1.0e-2 ||
				   largestDualError_ > 1.0e-2))
    handler_->message(CLP_SIMPLEX_ACCURACY, messages_)
      << largestPrimalError_
      << largestDualError_
      << CoinMessageEol;
  if ((largestPrimalError_ > 1.0e1||largestDualError_>1.0e1) 
      && numberRows_ > 100 && numberIterations_) {
#if ABC_NORMAL_DEBUG>0
    printf("Large errors - primal %g dual %g\n",
	   largestPrimalError_,largestDualError_);
#endif
    // Change factorization tolerance
    //if (abcFactorization_->zeroTolerance() > 1.0e-18)
    //abcFactorization_->zeroTolerance(1.0e-18);
  }
  return numberRefinements;
}
/*
  Row array has pivot column
  This chooses pivot row.
  For speed, we may need to go to a bucket approach when many
  variables go through bounds
  On exit rhsArray will have changes in costs of basic variables
*/
void
AbcSimplexPrimal::primalRow(CoinIndexedVector * rowArray,
                            CoinIndexedVector * rhsArray,
                            CoinIndexedVector * spareArray,
                            int valuesPass)
{
#if 1
  for (int iRow=0;iRow<numberRows_;iRow++) {
    int iPivot=abcPivotVariable_[iRow];
    assert (costBasic_[iRow]==abcCost_[iPivot]);
    assert (lowerBasic_[iRow]==abcLower_[iPivot]);
    assert (upperBasic_[iRow]==abcUpper_[iPivot]);
    assert (solutionBasic_[iRow]==abcSolution_[iPivot]);
  }
#endif
  double saveDj = dualIn_;
  if (valuesPass) {
    dualIn_ = abcCost_[sequenceIn_];
    
    double * work = rowArray->denseVector();
    int number = rowArray->getNumElements();
    int * which = rowArray->getIndices();
    
    int iIndex;
    for (iIndex = 0; iIndex < number; iIndex++) {
      
      int iRow = which[iIndex];
      double alpha = work[iRow];
      dualIn_ -= alpha * costBasic_[iRow];
    }
    // determine direction here
    if (dualIn_ < -dualTolerance_) {
      directionIn_ = 1;
    } else if (dualIn_ > dualTolerance_) {
      directionIn_ = -1;
    } else {
      // towards nearest bound
      if (valueIn_ - lowerIn_ < upperIn_ - valueIn_) {
	directionIn_ = -1;
	dualIn_ = dualTolerance_;
      } else {
	directionIn_ = 1;
	dualIn_ = -dualTolerance_;
      }
    }
  }
  
  // sequence stays as row number until end
  pivotRow_ = -1;
  int numberRemaining = 0;
  
  double totalThru = 0.0; // for when variables flip
  // Allow first few iterations to take tiny
  double acceptablePivot = 1.0e-1 * acceptablePivot_;
  if (numberIterations_ > 100)
    acceptablePivot = acceptablePivot_;
  if (abcFactorization_->pivots() > 10)
    acceptablePivot = 1.0e+3 * acceptablePivot_; // if we have iterated be more strict
  else if (abcFactorization_->pivots() > 5)
    acceptablePivot = 1.0e+2 * acceptablePivot_; // if we have iterated be slightly more strict
  else if (abcFactorization_->pivots())
    acceptablePivot = acceptablePivot_; // relax
  double bestEverPivot = acceptablePivot;
  int lastPivotRow = -1;
  double lastPivot = 0.0;
  double lastTheta = 1.0e50;
  
  // use spareArrays to put ones looked at in
  // First one is list of candidates
  // We could compress if we really know we won't need any more
  // Second array has current set of pivot candidates
  // with a backup list saved in double * part of indexed vector
  
  // pivot elements
  double * spare;
  // indices
  int * index;
  spareArray->clear();
  spare = spareArray->denseVector();
  index = spareArray->getIndices();
  
  // we also need somewhere for effective rhs
  double * rhs = rhsArray->denseVector();
  // and we can use indices to point to alpha
  // that way we can store fabs(alpha)
  int * indexPoint = rhsArray->getIndices();
  //int numberFlip=0; // Those which may change if flips
  
  /*
    First we get a list of possible pivots.  We can also see if the
    problem looks unbounded.
    
    At first we increase theta and see what happens.  We start
    theta at a reasonable guess.  If in right area then we do bit by bit.
    We save possible pivot candidates
    
  */
  
  // do first pass to get possibles
  // We can also see if unbounded
  
  double * work = rowArray->denseVector();
  int number = rowArray->getNumElements();
  int * which = rowArray->getIndices();
  
  // we need to swap sign if coming in from ub
  double way = directionIn_;
  double maximumMovement;
  if (way > 0.0)
    maximumMovement = CoinMin(1.0e30, upperIn_ - valueIn_);
  else
    maximumMovement = CoinMin(1.0e30, valueIn_ - lowerIn_);
  
  double averageTheta = abcNonLinearCost_->averageTheta();
  double tentativeTheta = CoinMin(10.0 * averageTheta, maximumMovement);
  double upperTheta = maximumMovement;
  if (tentativeTheta > 0.5 * maximumMovement)
    tentativeTheta = maximumMovement;
  bool thetaAtMaximum = tentativeTheta == maximumMovement;
  // In case tiny bounds increase
  if (maximumMovement < 1.0)
    tentativeTheta *= 1.1;
  double dualCheck = fabs(dualIn_);
  // but make a bit more pessimistic
  dualCheck = CoinMax(dualCheck - 100.0 * dualTolerance_, 0.99 * dualCheck);
  
  int iIndex;
  //#define CLP_DEBUG
#if ABC_NORMAL_DEBUG>3
  if (numberIterations_ == 1369 || numberIterations_ == -3840) {
    double dj = abcCost_[sequenceIn_];
    printf("cost in on %d is %g, dual in %g\n", sequenceIn_, dj, dualIn_);
    for (iIndex = 0; iIndex < number; iIndex++) {
      
      int iRow = which[iIndex];
      double alpha = work[iRow];
      int iPivot = abcPivotVariable_[iRow];
      dj -= alpha * costBasic_[iRow];
      printf("row %d var %d current %g %g %g, alpha %g so sol => %g (cost %g, dj %g)\n",
	     iRow, iPivot, lowerBasic_[iRow], solutionBasic_[iRow], upperBasic_[iRow],
	     alpha, solutionBasic_[iRow] - 1.0e9 * alpha, costBasic_[iRow], dj);
    }
  }
#endif
  while (true) {
    totalThru = 0.0;
    // We also re-compute reduced cost
    numberRemaining = 0;
    dualIn_ = abcCost_[sequenceIn_];
#ifndef NDEBUG
    //double tolerance = primalTolerance_ * 1.002;
#endif
    for (iIndex = 0; iIndex < number; iIndex++) {
      
      int iRow = which[iIndex];
      double alpha = work[iRow];
      dualIn_ -= alpha * costBasic_[iRow];
      alpha *= way;
      double oldValue = solutionBasic_[iRow];
      // get where in bound sequence
      // note that after this alpha is actually fabs(alpha)
      bool possible;
      // do computation same way as later on in primal
      if (alpha > 0.0) {
	// basic variable going towards lower bound
	double bound = lowerBasic_[iRow];
	// must be exactly same as when used
	double change = tentativeTheta * alpha;
	possible = (oldValue - change) <= bound + primalTolerance_;
	oldValue -= bound;
      } else {
	// basic variable going towards upper bound
	double bound = upperBasic_[iRow];
	// must be exactly same as when used
	double change = tentativeTheta * alpha;
	possible = (oldValue - change) >= bound - primalTolerance_;
	oldValue = bound - oldValue;
	alpha = - alpha;
      }
      double value;
      //assert (oldValue >= -10.0*tolerance);
      if (possible) {
	value = oldValue - upperTheta * alpha;
#ifdef CLP_USER_DRIVEN1
	if(!userChoiceValid1(this,iPivot,oldValue,
			     upperTheta,alpha,work[iRow]*way))
	  value =0.0; // say can't use
#endif
	if (value < -primalTolerance_ && alpha >= acceptablePivot) {
	  upperTheta = (oldValue + primalTolerance_) / alpha;
	}
	// add to list
	spare[numberRemaining] = alpha;
	rhs[numberRemaining] = oldValue;
	indexPoint[numberRemaining] = iIndex;
	index[numberRemaining++] = iRow;
	totalThru += alpha;
	setActive(iRow);
	//} else if (value<primalTolerance_*1.002) {
	// May change if is a flip
	//indexRhs[numberFlip++]=iRow;
      }
    }
    if (upperTheta < maximumMovement && totalThru*infeasibilityCost_ >= 1.0001 * dualCheck) {
      // Can pivot here
      break;
    } else if (!thetaAtMaximum) {
      //printf("Going round with average theta of %g\n",averageTheta);
      tentativeTheta = maximumMovement;
      thetaAtMaximum = true; // seems to be odd compiler error
    } else {
      break;
    }
  }
  totalThru = 0.0;
  
  theta_ = maximumMovement;
  
  bool goBackOne = false;
  if (objective_->type() > 1)
    dualIn_ = saveDj;
  
  //printf("%d remain out of %d\n",numberRemaining,number);
  int iTry = 0;
#define MAXTRY 1000
  if (numberRemaining && upperTheta < maximumMovement) {
    // First check if previously chosen one will work
    
    // first get ratio with tolerance
    for ( ; iTry < MAXTRY; iTry++) {
      
      upperTheta = maximumMovement;
      int iBest = -1;
      for (iIndex = 0; iIndex < numberRemaining; iIndex++) {
	
	double alpha = spare[iIndex];
	double oldValue = rhs[iIndex];
	double value = oldValue - upperTheta * alpha;
	
#ifdef CLP_USER_DRIVEN1
	int sequenceOut=abcPivotVariable_[index[iIndex]];
	if(!userChoiceValid1(this,sequenceOut,oldValue,
			     upperTheta,alpha, 0.0))
	  value =0.0; // say can't use
#endif
	if (value < -primalTolerance_ && alpha >= acceptablePivot) {
	  upperTheta = (oldValue + primalTolerance_) / alpha;
	  iBest = iIndex; // just in case weird numbers
	}
      }
      
      // now look at best in this lot
      // But also see how infeasible small pivots will make
      double sumInfeasibilities = 0.0;
      double bestPivot = acceptablePivot;
      pivotRow_ = -1;
      for (iIndex = 0; iIndex < numberRemaining; iIndex++) {
	
	int iRow = index[iIndex];
	double alpha = spare[iIndex];
	double oldValue = rhs[iIndex];
	double value = oldValue - upperTheta * alpha;
	
	if (value <= 0 || iBest == iIndex) {
	  // how much would it cost to go thru and modify bound
	  double trueAlpha = way * work[iRow];
	  totalThru += abcNonLinearCost_->changeInCost(iRow, trueAlpha, rhs[iIndex]);
	  setActive(iRow);
	  if (alpha > bestPivot) {
	    bestPivot = alpha;
	    theta_ = oldValue / bestPivot;
	    pivotRow_ = iRow;
	  } else if (alpha < acceptablePivot
#ifdef CLP_USER_DRIVEN1
		     ||!userChoiceValid1(this,abcPivotVariable_[index[iIndex]],
					 oldValue,upperTheta,alpha,0.0)
#endif
		     ) {
	    if (value < -primalTolerance_)
	      sumInfeasibilities += -value - primalTolerance_;
	  }
	}
      }
      if (bestPivot < 0.1 * bestEverPivot &&
	  bestEverPivot > 1.0e-6 && bestPivot < 1.0e-3) {
	// back to previous one
	goBackOne = true;
	break;
      } else if (pivotRow_ == -1 && upperTheta > largeValue_) {
	if (lastPivot > acceptablePivot) {
	  // back to previous one
	  goBackOne = true;
	} else {
	  // can only get here if all pivots so far too small
	}
	break;
      } else if (totalThru >= dualCheck) {
	if (sumInfeasibilities > primalTolerance_ && !abcNonLinearCost_->numberInfeasibilities()) {
	  // Looks a bad choice
	  if (lastPivot > acceptablePivot) {
	    goBackOne = true;
	  } else {
	    // say no good
	    dualIn_ = 0.0;
	  }
	}
	break; // no point trying another loop
      } else {
	lastPivotRow = pivotRow_;
	lastTheta = theta_;
	if (bestPivot > bestEverPivot)
	  bestEverPivot = bestPivot;
      }
    }
    // can get here without pivotRow_ set but with lastPivotRow
    if (goBackOne || (pivotRow_ < 0 && lastPivotRow >= 0)) {
      // back to previous one
      pivotRow_ = lastPivotRow;
      theta_ = lastTheta;
    }
  } else if (pivotRow_ < 0 && maximumMovement > 1.0e20) {
    // looks unbounded
    valueOut_ = COIN_DBL_MAX; // say odd
    if (abcNonLinearCost_->numberInfeasibilities()) {
      // but infeasible??
      // move variable but don't pivot
      tentativeTheta = 1.0e50;
      for (iIndex = 0; iIndex < number; iIndex++) {
	int iRow = which[iIndex];
	double alpha = work[iRow];
	alpha *= way;
	double oldValue = solutionBasic_[iRow];
	// get where in bound sequence
	// note that after this alpha is actually fabs(alpha)
	if (alpha > 0.0) {
	  // basic variable going towards lower bound
	  double bound = lowerBasic_[iRow];
	  oldValue -= bound;
	} else {
	  // basic variable going towards upper bound
	  double bound = upperBasic_[iRow];
	  oldValue = bound - oldValue;
	  alpha = - alpha;
	}
	if (oldValue - tentativeTheta * alpha < 0.0) {
	  tentativeTheta = oldValue / alpha;
	}
      }
      // If free in then see if we can get to 0.0
      if (lowerIn_ < -1.0e20 && upperIn_ > 1.0e20) {
	if (dualIn_ * valueIn_ > 0.0) {
	  if (fabs(valueIn_) < 1.0e-2 && (tentativeTheta < fabs(valueIn_) || tentativeTheta > 1.0e20)) {
	    tentativeTheta = fabs(valueIn_);
	  }
	}
      }
      if (tentativeTheta < 1.0e10)
	valueOut_ = valueIn_ + way * tentativeTheta;
    }
  }
  //if (iTry>50)
  //printf("** %d tries\n",iTry);
  if (pivotRow_ >= 0) {
    alpha_ = work[pivotRow_];
    // translate to sequence
    sequenceOut_ = abcPivotVariable_[pivotRow_];
    valueOut_ = solution(sequenceOut_);
    lowerOut_ = abcLower_[sequenceOut_];
    upperOut_ = abcUpper_[sequenceOut_];
    //#define MINIMUMTHETA 1.0e-11
    // Movement should be minimum for anti-degeneracy - unless
    // fixed variable out
    double minimumTheta;
    if (upperOut_ > lowerOut_)
      minimumTheta = minimumThetaMovement_;
    else
      minimumTheta = 0.0;
    // But can't go infeasible
    double distance;
    if (alpha_ * way > 0.0)
      distance = valueOut_ - lowerOut_;
    else
      distance = upperOut_ - valueOut_;
    if (distance - minimumTheta * fabs(alpha_) < -primalTolerance_)
      minimumTheta = CoinMax(0.0, (distance + 0.5 * primalTolerance_) / fabs(alpha_));
    // will we need to increase tolerance
    //#define CLP_DEBUG
    double largestInfeasibility = primalTolerance_;
    if (theta_ < minimumTheta && (specialOptions_ & 4) == 0 && !valuesPass) {
      theta_ = minimumTheta;
      for (iIndex = 0; iIndex < numberRemaining - numberRemaining; iIndex++) {
	largestInfeasibility = CoinMax(largestInfeasibility,
				       -(rhs[iIndex] - spare[iIndex] * theta_));
      }
      //#define CLP_DEBUG
#if ABC_NORMAL_DEBUG>3
      if (largestInfeasibility > primalTolerance_ && (handler_->logLevel() & 32) > -1)
	printf("Primal tolerance increased from %g to %g\n",
	       primalTolerance_, largestInfeasibility);
#endif
      //#undef CLP_DEBUG
      primalTolerance_ = CoinMax(primalTolerance_, largestInfeasibility);
    }
    // Need to look at all in some cases
    if (theta_ > tentativeTheta) {
      for (iIndex = 0; iIndex < number; iIndex++)
	setActive(which[iIndex]);
    }
    if (way < 0.0)
      theta_ = - theta_;
    double newValue = valueOut_ - theta_ * alpha_;
    // If  4 bit set - Force outgoing variables to exact bound (primal)
    if (alpha_ * way < 0.0) {
      directionOut_ = -1;    // to upper bound
      if (fabs(theta_) > 1.0e-6 || (specialOptions_ & 4) != 0) {
	upperOut_ = abcNonLinearCost_->nearest(pivotRow_, newValue);
      } else {
	upperOut_ = newValue;
      }
    } else {
      directionOut_ = 1;    // to lower bound
      if (fabs(theta_) > 1.0e-6 || (specialOptions_ & 4) != 0) {
	lowerOut_ = abcNonLinearCost_->nearest(pivotRow_, newValue);
      } else {
	lowerOut_ = newValue;
      }
    }
    dualOut_ = reducedCost(sequenceOut_);
  } else if (maximumMovement < 1.0e20) {
    // flip
    pivotRow_ = -2; // so we can tell its a flip
    sequenceOut_ = sequenceIn_;
    valueOut_ = valueIn_;
    dualOut_ = dualIn_;
    lowerOut_ = lowerIn_;
    upperOut_ = upperIn_;
    alpha_ = 0.0;
    if (way < 0.0) {
      directionOut_ = 1;    // to lower bound
      theta_ = lowerOut_ - valueOut_;
    } else {
      directionOut_ = -1;    // to upper bound
      theta_ = upperOut_ - valueOut_;
    }
  }
  
  double theta1 = CoinMax(theta_, 1.0e-12);
  double theta2 = numberIterations_ * abcNonLinearCost_->averageTheta();
  // Set average theta
  abcNonLinearCost_->setAverageTheta((theta1 + theta2) / (static_cast<double> (numberIterations_ + 1)));
  //if (numberIterations_%1000==0)
  //printf("average theta is %g\n",abcNonLinearCost_->averageTheta());
  
  // clear arrays
  CoinZeroN(spare, numberRemaining);
  CoinZeroN(rhs, numberRemaining);
  
  // put back original bounds etc
  CoinMemcpyN(index, numberRemaining, rhsArray->getIndices());
  rhsArray->setNumElements(numberRemaining);
  //rhsArray->setPacked();
  abcNonLinearCost_->goBackAll(rhsArray);
  rhsArray->clear();
  
}
/*
  Row array has pivot column
  This chooses pivot row.
  For speed, we may need to go to a bucket approach when many
  variables go through bounds
  On exit rhsArray will have changes in costs of basic variables
*/
void
AbcSimplexPrimal::primalRow(CoinIndexedVector * rowArray,
                            CoinIndexedVector * rhsArray,
                            CoinIndexedVector * spareArray,
                            pivotStruct & stuff)
{
  int valuesPass=stuff.valuesPass_;
  double dualIn=stuff.dualIn_;
  double lowerIn=stuff.lowerIn_;
  double upperIn=stuff.upperIn_;
  double valueIn=stuff.valueIn_;
  int sequenceIn=stuff.sequenceIn_;
  int directionIn=stuff.directionIn_;
  int pivotRow=-1;
  int sequenceOut=-1;
  double dualOut=0.0;
  double lowerOut=0.0;
  double upperOut=0.0;
  double valueOut=0.0;
  double theta;
  double alpha;
  int directionOut=0;
  if (valuesPass) {
    dualIn = abcCost_[sequenceIn];
    
    double * work = rowArray->denseVector();
    int number = rowArray->getNumElements();
    int * which = rowArray->getIndices();
    
    int iIndex;
    for (iIndex = 0; iIndex < number; iIndex++) {
      
      int iRow = which[iIndex];
      double alpha = work[iRow];
      dualIn -= alpha * costBasic_[iRow];
    }
    // determine direction here
    if (dualIn < -dualTolerance_) {
      directionIn = 1;
    } else if (dualIn > dualTolerance_) {
      directionIn = -1;
    } else {
      // towards nearest bound
      if (valueIn - lowerIn < upperIn - valueIn) {
	directionIn = -1;
	dualIn = dualTolerance_;
      } else {
	directionIn = 1;
	dualIn = -dualTolerance_;
      }
    }
  }
  
  // sequence stays as row number until end
  pivotRow = -1;
  int numberRemaining = 0;
  
  double totalThru = 0.0; // for when variables flip
  // Allow first few iterations to take tiny
  double acceptablePivot = 1.0e-1 * acceptablePivot_;
  if (numberIterations_ > 100)
    acceptablePivot = acceptablePivot_;
  if (abcFactorization_->pivots() > 10)
    acceptablePivot = 1.0e+3 * acceptablePivot_; // if we have iterated be more strict
  else if (abcFactorization_->pivots() > 5)
    acceptablePivot = 1.0e+2 * acceptablePivot_; // if we have iterated be slightly more strict
  else if (abcFactorization_->pivots())
    acceptablePivot = acceptablePivot_; // relax
  double bestEverPivot = acceptablePivot;
  int lastPivotRow = -1;
  double lastPivot = 0.0;
  double lastTheta = 1.0e50;
  
  // use spareArrays to put ones looked at in
  // First one is list of candidates
  // We could compress if we really know we won't need any more
  // Second array has current set of pivot candidates
  // with a backup list saved in double * part of indexed vector
  
  // pivot elements
  double * spare;
  // indices
  int * index;
  spareArray->clear();
  spare = spareArray->denseVector();
  index = spareArray->getIndices();
  
  // we also need somewhere for effective rhs
  double * rhs = rhsArray->denseVector();
  // and we can use indices to point to alpha
  // that way we can store fabs(alpha)
  int * indexPoint = rhsArray->getIndices();
  //int numberFlip=0; // Those which may change if flips
  
  /*
    First we get a list of possible pivots.  We can also see if the
    problem looks unbounded.
    
    At first we increase theta and see what happens.  We start
    theta at a reasonable guess.  If in right area then we do bit by bit.
    We save possible pivot candidates
    
  */
  
  // do first pass to get possibles
  // We can also see if unbounded
  
  double * work = rowArray->denseVector();
  int number = rowArray->getNumElements();
  int * which = rowArray->getIndices();
  
  // we need to swap sign if coming in from ub
  double way = directionIn;
  double maximumMovement;
  if (way > 0.0)
    maximumMovement = CoinMin(1.0e30, upperIn - valueIn);
  else
    maximumMovement = CoinMin(1.0e30, valueIn - lowerIn);
  
  double averageTheta = abcNonLinearCost_->averageTheta();
  double tentativeTheta = CoinMin(10.0 * averageTheta, maximumMovement);
  double upperTheta = maximumMovement;
  if (tentativeTheta > 0.5 * maximumMovement)
    tentativeTheta = maximumMovement;
  bool thetaAtMaximum = tentativeTheta == maximumMovement;
  // In case tiny bounds increase
  if (maximumMovement < 1.0)
    tentativeTheta *= 1.1;
  double dualCheck = fabs(dualIn);
  // but make a bit more pessimistic
  dualCheck = CoinMax(dualCheck - 100.0 * dualTolerance_, 0.99 * dualCheck);
  
  int iIndex;
  while (true) {
    totalThru = 0.0;
    // We also re-compute reduced cost
    numberRemaining = 0;
    dualIn = abcCost_[sequenceIn];
    for (iIndex = 0; iIndex < number; iIndex++) {
      int iRow = which[iIndex];
      double alpha = work[iRow];
      dualIn -= alpha * costBasic_[iRow];
      alpha *= way;
      double oldValue = solutionBasic_[iRow];
      // get where in bound sequence
      // note that after this alpha is actually fabs(alpha)
      bool possible;
      // do computation same way as later on in primal
      if (alpha > 0.0) {
	// basic variable going towards lower bound
	double bound = lowerBasic_[iRow];
	// must be exactly same as when used
	double change = tentativeTheta * alpha;
	possible = (oldValue - change) <= bound + primalTolerance_;
	oldValue -= bound;
      } else {
	// basic variable going towards upper bound
	double bound = upperBasic_[iRow];
	// must be exactly same as when used
	double change = tentativeTheta * alpha;
	possible = (oldValue - change) >= bound - primalTolerance_;
	oldValue = bound - oldValue;
	alpha = - alpha;
      }
      double value;
      if (possible) {
	value = oldValue - upperTheta * alpha;
#ifdef CLP_USER_DRIVEN1
	if(!userChoiceValid1(this,iPivot,oldValue,
			     upperTheta,alpha,work[iRow]*way))
	  value =0.0; // say can't use
#endif
	if (value < -primalTolerance_ && alpha >= acceptablePivot) {
	  upperTheta = (oldValue + primalTolerance_) / alpha;
	}
	// add to list
	spare[numberRemaining] = alpha;
	rhs[numberRemaining] = oldValue;
	indexPoint[numberRemaining] = iIndex;
	index[numberRemaining++] = iRow;
	totalThru += alpha;
	setActive(iRow);
      }
    }
    if (upperTheta < maximumMovement && totalThru*infeasibilityCost_ >= 1.0001 * dualCheck) {
      // Can pivot here
      break;
    } else if (!thetaAtMaximum) {
      //printf("Going round with average theta of %g\n",averageTheta);
      tentativeTheta = maximumMovement;
      thetaAtMaximum = true; // seems to be odd compiler error
    } else {
      break;
    }
  }
  totalThru = 0.0;
  
  theta = maximumMovement;
  
  bool goBackOne = false;
  
  //printf("%d remain out of %d\n",numberRemaining,number);
  int iTry = 0;
  if (numberRemaining && upperTheta < maximumMovement) {
    // First check if previously chosen one will work
    
    // first get ratio with tolerance
    for ( ; iTry < MAXTRY; iTry++) {
      
      upperTheta = maximumMovement;
      int iBest = -1;
      for (iIndex = 0; iIndex < numberRemaining; iIndex++) {
	
	double alpha = spare[iIndex];
	double oldValue = rhs[iIndex];
	double value = oldValue - upperTheta * alpha;
	
#ifdef CLP_USER_DRIVEN1
	int sequenceOut=abcPivotVariable_[index[iIndex]];
	if(!userChoiceValid1(this,sequenceOut,oldValue,
			     upperTheta,alpha, 0.0))
	  value =0.0; // say can't use
#endif
	if (value < -primalTolerance_ && alpha >= acceptablePivot) {
	  upperTheta = (oldValue + primalTolerance_) / alpha;
	  iBest = iIndex; // just in case weird numbers
	}
      }
      
      // now look at best in this lot
      // But also see how infeasible small pivots will make
      double sumInfeasibilities = 0.0;
      double bestPivot = acceptablePivot;
      pivotRow = -1;
      for (iIndex = 0; iIndex < numberRemaining; iIndex++) {
	
	int iRow = index[iIndex];
	double alpha = spare[iIndex];
	double oldValue = rhs[iIndex];
	double value = oldValue - upperTheta * alpha;
	
	if (value <= 0 || iBest == iIndex) {
	  // how much would it cost to go thru and modify bound
	  double trueAlpha = way * work[iRow];
	  totalThru += abcNonLinearCost_->changeInCost(iRow, trueAlpha, rhs[iIndex]);
	  setActive(iRow);
	  if (alpha > bestPivot) {
	    bestPivot = alpha;
	    theta = oldValue / bestPivot;
	    pivotRow = iRow;
	  } else if (alpha < acceptablePivot
#ifdef CLP_USER_DRIVEN1
		     ||!userChoiceValid1(this,abcPivotVariable_[index[iIndex]],
					 oldValue,upperTheta,alpha,0.0)
#endif
		     ) {
	    if (value < -primalTolerance_)
	      sumInfeasibilities += -value - primalTolerance_;
	  }
	}
      }
      if (bestPivot < 0.1 * bestEverPivot &&
	  bestEverPivot > 1.0e-6 && bestPivot < 1.0e-3) {
	// back to previous one
	goBackOne = true;
	break;
      } else if (pivotRow == -1 && upperTheta > largeValue_) {
	if (lastPivot > acceptablePivot) {
	  // back to previous one
	  goBackOne = true;
	} else {
	  // can only get here if all pivots so far too small
	}
	break;
      } else if (totalThru >= dualCheck) {
	if (sumInfeasibilities > primalTolerance_ && !abcNonLinearCost_->numberInfeasibilities()) {
	  // Looks a bad choice
	  if (lastPivot > acceptablePivot) {
	    goBackOne = true;
	  } else {
	    // say no good
	    dualIn = 0.0;
	  }
	}
	break; // no point trying another loop
      } else {
	lastPivotRow = pivotRow;
	lastTheta = theta;
	if (bestPivot > bestEverPivot)
	  bestEverPivot = bestPivot;
      }
    }
    // can get here without pivotRow set but with lastPivotRow
    if (goBackOne || (pivotRow < 0 && lastPivotRow >= 0)) {
      // back to previous one
      pivotRow = lastPivotRow;
      theta = lastTheta;
    }
  } else if (pivotRow < 0 && maximumMovement > 1.0e20) {
    // looks unbounded
    valueOut = COIN_DBL_MAX; // say odd
    if (abcNonLinearCost_->numberInfeasibilities()) {
      // but infeasible??
      // move variable but don't pivot
      tentativeTheta = 1.0e50;
      for (iIndex = 0; iIndex < number; iIndex++) {
	int iRow = which[iIndex];
	double alpha = work[iRow];
	alpha *= way;
	double oldValue = solutionBasic_[iRow];
	// get where in bound sequence
	// note that after this alpha is actually fabs(alpha)
	if (alpha > 0.0) {
	  // basic variable going towards lower bound
	  double bound = lowerBasic_[iRow];
	  oldValue -= bound;
	} else {
	  // basic variable going towards upper bound
	  double bound = upperBasic_[iRow];
	  oldValue = bound - oldValue;
	  alpha = - alpha;
	}
	if (oldValue - tentativeTheta * alpha < 0.0) {
	  tentativeTheta = oldValue / alpha;
	}
      }
      // If free in then see if we can get to 0.0
      if (lowerIn < -1.0e20 && upperIn > 1.0e20) {
	if (dualIn * valueIn > 0.0) {
	  if (fabs(valueIn) < 1.0e-2 && (tentativeTheta < fabs(valueIn) || tentativeTheta > 1.0e20)) {
	    tentativeTheta = fabs(valueIn);
	  }
	}
      }
      if (tentativeTheta < 1.0e10)
	valueOut = valueIn + way * tentativeTheta;
    }
  }
  if (pivotRow >= 0) {
    alpha = work[pivotRow];
    // translate to sequence
    sequenceOut = abcPivotVariable_[pivotRow];
    valueOut = solutionBasic_[pivotRow];
    lowerOut = lowerBasic_[pivotRow];
    upperOut = upperBasic_[pivotRow];
    // Movement should be minimum for anti-degeneracy - unless
    // fixed variable out
    double minimumTheta;
    if (upperOut > lowerOut)
      minimumTheta = minimumThetaMovement_;
    else
      minimumTheta = 0.0;
    // But can't go infeasible
    double distance;
    if (alpha * way > 0.0)
      distance = valueOut - lowerOut;
    else
      distance = upperOut - valueOut;
    if (distance - minimumTheta * fabs(alpha) < -primalTolerance_)
      minimumTheta = CoinMax(0.0, (distance + 0.5 * primalTolerance_) / fabs(alpha));
    // will we need to increase tolerance
    //double largestInfeasibility = primalTolerance_;
    if (theta < minimumTheta && (specialOptions_ & 4) == 0 && !valuesPass) {
      theta = minimumTheta;
      //for (iIndex = 0; iIndex < numberRemaining - numberRemaining; iIndex++) {
      //largestInfeasibility = CoinMax(largestInfeasibility,
      //			       -(rhs[iIndex] - spare[iIndex] * theta));
      //}
      //primalTolerance_ = CoinMax(primalTolerance_, largestInfeasibility);
    }
    // Need to look at all in some cases
    if (theta > tentativeTheta) {
      for (iIndex = 0; iIndex < number; iIndex++)
	setActive(which[iIndex]);
    }
    if (way < 0.0)
      theta = - theta;
    double newValue = valueOut - theta * alpha;
    // If  4 bit set - Force outgoing variables to exact bound (primal)
    if (alpha * way < 0.0) {
      directionOut = -1;    // to upper bound
      if (fabs(theta) > 1.0e-6 || (specialOptions_ & 4) != 0) {
	upperOut = abcNonLinearCost_->nearest(pivotRow, newValue);
      } else {
	upperOut = newValue;
      }
    } else {
      directionOut = 1;    // to lower bound
      if (fabs(theta) > 1.0e-6 || (specialOptions_ & 4) != 0) {
	lowerOut = abcNonLinearCost_->nearest(pivotRow, newValue);
      } else {
	lowerOut = newValue;
      }
    }
    dualOut = reducedCost(sequenceOut);
  } else if (maximumMovement < 1.0e20) {
    // flip
    pivotRow = -2; // so we can tell its a flip
    sequenceOut = sequenceIn;
    valueOut = valueIn;
    dualOut = dualIn;
    lowerOut = lowerIn;
    upperOut = upperIn;
    alpha = 0.0;
    if (way < 0.0) {
      directionOut = 1;    // to lower bound
      theta = lowerOut - valueOut;
    } else {
      directionOut = -1;    // to upper bound
      theta = upperOut - valueOut;
    }
  }
  
  
  // clear arrays
  CoinZeroN(spare, numberRemaining);
  CoinZeroN(rhs, numberRemaining);
  
  // put back original bounds etc
  CoinMemcpyN(index, numberRemaining, rhsArray->getIndices());
  rhsArray->setNumElements(numberRemaining);
  abcNonLinearCost_->goBackAll(rhsArray);
  rhsArray->clear();
  stuff.theta_=theta;
  stuff.alpha_=alpha;
  stuff.dualIn_=dualIn;
  stuff.dualOut_=dualOut;
  stuff.lowerOut_=lowerOut;
  stuff.upperOut_=upperOut;
  stuff.valueOut_=valueOut;
  stuff.sequenceOut_=sequenceOut;
  stuff.directionOut_=directionOut;
  stuff.pivotRow_=pivotRow;
}
/*
  Chooses primal pivot column
  updateArray has cost updates (also use pivotRow_ from last iteration)
  Would be faster with separate region to scan
  and will have this (with square of infeasibility) when steepest
  For easy problems we can just choose one of the first columns we look at
*/
void
AbcSimplexPrimal::primalColumn(CoinPartitionedVector * updates,
                               CoinPartitionedVector * spareRow2,
                               CoinPartitionedVector * spareColumn1)
{
  for (int i=0;i<4;i++)
    multipleSequenceIn_[i]=-1;
  sequenceIn_ = abcPrimalColumnPivot_->pivotColumn(updates,
						   spareRow2, spareColumn1);
  if (sequenceIn_ >= 0) {
    valueIn_ = abcSolution_[sequenceIn_];
    dualIn_ = abcDj_[sequenceIn_];
    lowerIn_ = abcLower_[sequenceIn_];
    upperIn_ = abcUpper_[sequenceIn_];
    if (dualIn_ > 0.0)
      directionIn_ = -1;
    else
      directionIn_ = 1;
  } else {
    sequenceIn_ = -1;
  }
}
/* The primals are updated by the given array.
   Returns number of infeasibilities.
   After rowArray will have list of cost changes
*/
int
AbcSimplexPrimal::updatePrimalsInPrimal(CoinIndexedVector * rowArray,
                                        double theta,
                                        double & objectiveChange,
                                        int valuesPass)
{
  // Cost on pivot row may change - may need to change dualIn
  double oldCost = 0.0;
  if (pivotRow_ >= 0)
    oldCost = abcCost_[sequenceOut_];
  double * work = rowArray->denseVector();
  int number = rowArray->getNumElements();
  int * which = rowArray->getIndices();
  
  int newNumber = 0;
  abcNonLinearCost_->setChangeInCost(0.0);
  // allow for case where bound+tolerance == bound
  //double tolerance = 0.999999*primalTolerance_;
  double relaxedTolerance = 1.001 * primalTolerance_;
  int iIndex;
  if (!valuesPass) {
    for (iIndex = 0; iIndex < number; iIndex++) {
      
      int iRow = which[iIndex];
      double alpha = work[iRow];
      work[iRow] = 0.0;
      int iPivot = abcPivotVariable_[iRow];
      double change = theta * alpha;
      double value = abcSolution_[iPivot] - change;
      abcSolution_[iPivot] = value;
      value = solutionBasic_[iRow] - change;
      solutionBasic_[iRow] = value;
#ifndef NDEBUG
      // check if not active then okay
      if (!active(iRow) && (specialOptions_ & 4) == 0 && pivotRow_ != -1) {
	// But make sure one going out is feasible
	if (change > 0.0) {
	  // going down
	  if (value <= abcLower_[iPivot] + primalTolerance_) {
	    if (iPivot == sequenceOut_ && value > abcLower_[iPivot] - relaxedTolerance)
	      value = abcLower_[iPivot];
	    //double difference = abcNonLinearCost_->setOneBasic(iRow, value);
	    //assert (!difference || fabs(change) > 1.0e9);
	  }
	} else {
	  // going up
	  if (value >= abcUpper_[iPivot] - primalTolerance_) {
	    if (iPivot == sequenceOut_ && value < abcUpper_[iPivot] + relaxedTolerance)
	      value = abcUpper_[iPivot];
	    //double difference = abcNonLinearCost_->setOneBasic(iRow, value);
	    //assert (!difference || fabs(change) > 1.0e9);
	  }
	}
      }
#endif
      if (active(iRow) || theta_ < 0.0) {
	clearActive(iRow);
	// But make sure one going out is feasible
	if (change > 0.0) {
	  // going down
	  if (value <= abcLower_[iPivot] + primalTolerance_) {
	    if (iPivot == sequenceOut_ && value >= abcLower_[iPivot] - relaxedTolerance)
	      value = abcLower_[iPivot];
	    double difference = abcNonLinearCost_->setOneBasic(iRow, value);
	    if (difference) {
	      work[iRow] = difference;
	      //change reduced cost on this
	      //abcDj_[iPivot] = -difference;
	      which[newNumber++] = iRow;
	    }
	  }
	} else {
	  // going up
	  if (value >= abcUpper_[iPivot] - primalTolerance_) {
	    if (iPivot == sequenceOut_ && value < abcUpper_[iPivot] + relaxedTolerance)
	      value = abcUpper_[iPivot];
	    double difference = abcNonLinearCost_->setOneBasic(iRow, value);
	    if (difference) {
	      work[iRow] = difference;
	      //change reduced cost on this
	      //abcDj_[iPivot] = -difference;
	      which[newNumber++] = iRow;
	    }
	  }
	}
      }
    }
  } else {
    // values pass so look at all
    for (iIndex = 0; iIndex < number; iIndex++) {
      
      int iRow = which[iIndex];
      double alpha = work[iRow];
      work[iRow] = 0.0;
      int iPivot = abcPivotVariable_[iRow];
      double change = theta * alpha;
      double value = abcSolution_[iPivot] - change;
      abcSolution_[iPivot] = value;
      value = solutionBasic_[iRow] - change;
      solutionBasic_[iRow] = value;
      clearActive(iRow);
      // But make sure one going out is feasible
      if (change > 0.0) {
	// going down
	if (value <= abcLower_[iPivot] + primalTolerance_) {
	  if (iPivot == sequenceOut_ && value > abcLower_[iPivot] - relaxedTolerance)
	    value = abcLower_[iPivot];
	  double difference = abcNonLinearCost_->setOneBasic(iRow, value);
	  if (difference) {
	    work[iRow] = difference;
	    //change reduced cost on this
	    abcDj_[iPivot] = -difference;
	    which[newNumber++] = iRow;
	  }
	}
      } else {
	// going up
	if (value >= abcUpper_[iPivot] - primalTolerance_) {
	  if (iPivot == sequenceOut_ && value < abcUpper_[iPivot] + relaxedTolerance)
	    value = abcUpper_[iPivot];
	  double difference = abcNonLinearCost_->setOneBasic(iRow, value);
	  if (difference) {
	    work[iRow] = difference;
	    //change reduced cost on this
	    abcDj_[iPivot] = -difference;
	    which[newNumber++] = iRow;
	  }
	}
      }
    }
  }
  objectiveChange += abcNonLinearCost_->changeInCost();
  //rowArray->setPacked();
  if (pivotRow_ >= 0) {
    double dualIn = dualIn_ + (oldCost - abcCost_[sequenceOut_]);
    // update change vector to include pivot
    if (!work[pivotRow_])
      which[newNumber++] = pivotRow_;
    work[pivotRow_] -= dualIn;
    // above seems marginally better for updates - below should also work
    //work[pivotRow_] = -dualIn_;
  }
  rowArray->setNumElements(newNumber);
  return 0;
}
// Perturbs problem
void
AbcSimplexPrimal::perturb(int /*type*/)
{
  if (perturbation_ > 100)
    return; //perturbed already
  if (perturbation_ == 100)
    perturbation_ = 50; // treat as normal
  int savePerturbation = perturbation_;
  int i;
  copyFromSaved(14); // copy bounds and costs
  // look at element range
  double smallestNegative;
  double largestNegative;
  double smallestPositive;
  double largestPositive;
  matrix_->rangeOfElements(smallestNegative, largestNegative,
			   smallestPositive, largestPositive);
  smallestPositive = CoinMin(fabs(smallestNegative), smallestPositive);
  largestPositive = CoinMax(fabs(largestNegative), largestPositive);
  double elementRatio = largestPositive / smallestPositive;
  if ((!numberIterations_ ||initialSumInfeasibilities_==1.23456789e-5)&& perturbation_ == 50) {
    // See if we need to perturb
    int numberTotal = CoinMax(numberRows_, numberColumns_);
    double * sort = new double[numberTotal];
    int nFixed = 0;
    for (i = 0; i < numberRows_; i++) {
      double lo = fabs(rowLower_[i]);
      double up = fabs(rowUpper_[i]);
      double value = 0.0;
      if (lo && lo < 1.0e20) {
	if (up && up < 1.0e20) {
	  value = 0.5 * (lo + up);
	  if (lo == up)
	    nFixed++;
	} else {
	  value = lo;
	}
      } else {
	if (up && up < 1.0e20)
	  value = up;
      }
      sort[i] = value;
    }
    std::sort(sort, sort + numberRows_);
    int number = 1;
    double last = sort[0];
    for (i = 1; i < numberRows_; i++) {
      if (last != sort[i])
	number++;
      last = sort[i];
    }
#ifdef KEEP_GOING_IF_FIXED
    //printf("ratio number diff rhs %g (%d %d fixed), element ratio %g\n",((double)number)/((double) numberRows_),
    //   numberRows_,nFixed,elementRatio);
#endif
    if (number * 4 > numberRows_ || elementRatio > 1.0e12) {
      perturbation_ = 100;
      delete [] sort;
      // Make sure feasible bounds
      if (abcNonLinearCost_) {
	abcNonLinearCost_->refresh();
	abcNonLinearCost_->checkInfeasibilities();
	//abcNonLinearCost_->feasibleBounds();
      }
      moveToBasic();
      return; // good enough
    }
    number = 0;
#ifdef KEEP_GOING_IF_FIXED
    if (!integerType_) {
      // look at columns
      nFixed = 0;
      for (i = 0; i < numberColumns_; i++) {
	double lo = fabs(columnLower_[i]);
	double up = fabs(columnUpper_[i]);
	double value = 0.0;
	if (lo && lo < 1.0e20) {
	  if (up && up < 1.0e20) {
	    value = 0.5 * (lo + up);
	    if (lo == up)
	      nFixed++;
	  } else {
	    value = lo;
	  }
	} else {
	  if (up && up < 1.0e20)
	    value = up;
	}
	sort[i] = value;
      }
      std::sort(sort, sort + numberColumns_);
      number = 1;
      last = sort[0];
      for (i = 1; i < numberColumns_; i++) {
	if (last != sort[i])
	  number++;
	last = sort[i];
      }
      //printf("cratio number diff bounds %g (%d %d fixed)\n",((double)number)/((double) numberColumns_),
      //     numberColumns_,nFixed);
    }
#endif
    delete [] sort;
    if (number * 4 > numberColumns_) {
      perturbation_ = 100;
      // Make sure feasible bounds
      if (abcNonLinearCost_) {
	abcNonLinearCost_->refresh();
	abcNonLinearCost_->checkInfeasibilities();
	//abcNonLinearCost_->feasibleBounds();
      }
      moveToBasic();
      return; // good enough
    }
  }
  // primal perturbation
  double perturbation = 1.0e-20;
  double bias = 1.0;
  int numberNonZero = 0;
  // maximum fraction of rhs/bounds to perturb
  double maximumFraction = 1.0e-5;
  double overallMultiplier= (perturbation_==50||perturbation_>54) ? 2.0 : 0.2;
#ifdef HEAVY_PERTURBATION
    if (perturbation_==50)
      perturbation_=HEAVY_PERTURBATION;
#endif
  if (perturbation_ >= 50) {
    perturbation = 1.0e-4;
    for (i = 0; i < numberColumns_ + numberRows_; i++) {
      if (abcUpper_[i] > abcLower_[i] + primalTolerance_) {
	double lowerValue, upperValue;
	if (abcLower_[i] > -1.0e20)
	  lowerValue = fabs(abcLower_[i]);
	else
	  lowerValue = 0.0;
	if (abcUpper_[i] < 1.0e20)
	  upperValue = fabs(abcUpper_[i]);
	else
	  upperValue = 0.0;
	double value = CoinMax(fabs(lowerValue), fabs(upperValue));
	value = CoinMin(value, abcUpper_[i] - abcLower_[i]);
#if 1
	if (value) {
	  perturbation += value;
	  numberNonZero++;
	}
#else
	perturbation = CoinMax(perturbation, value);
#endif
      }
    }
    if (numberNonZero)
      perturbation /= static_cast<double> (numberNonZero);
    else
      perturbation = 1.0e-1;
    if (perturbation_ > 50 && perturbation_ < 55) {
      // reduce
      while (perturbation_ < 55) {
	perturbation_++;
	perturbation *= 0.25;
	bias *= 0.25;
      }
      perturbation_ = 50;
    } else if (perturbation_ >= 55 && perturbation_ < 60) {
      // increase
      while (perturbation_ > 55) {
	overallMultiplier *= 1.2;
	perturbation_--;
	perturbation *= 4.0;
      }
      perturbation_ = 50;
    }
  } else if (perturbation_ < 100) {
    perturbation = pow(10.0, perturbation_);
    // user is in charge
    maximumFraction = 1.0;
  }
  double largestZero = 0.0;
  double largest = 0.0;
  double largestPerCent = 0.0;
  bool printOut = (handler_->logLevel() == 63);
  printOut = false; //off
  // Check if all slack
  int number = 0;
  int iSequence;
  for (iSequence = 0; iSequence < numberRows_; iSequence++) {
    if (getInternalStatus(iSequence) == basic)
      number++;
  }
  if (rhsScale_ > 100.0) {
    // tone down perturbation
    maximumFraction *= 0.1;
  }
  if (savePerturbation==51) {
    perturbation = CoinMin(0.1,perturbation);
    maximumFraction *=0.1;
  }
  //if (number != numberRows_)
  //type = 1;
  // modify bounds
  // Change so at least 1.0e-5 and no more than 0.1
  // For now just no more than 0.1
  // printf("Pert type %d perturbation %g, maxF %g\n",type,perturbation,maximumFraction);
  // seems much slower???
  //const double * COIN_RESTRICT perturbationArray = perturbationSaved_;
  // Make sure feasible bounds
  if (abcNonLinearCost_&&true) {
    abcNonLinearCost_->refresh();
    abcNonLinearCost_->checkInfeasibilities(primalTolerance_);
    //abcNonLinearCost_->feasibleBounds();
  }
  double tolerance = 100.0 * primalTolerance_;
  int numberChanged=0;
  //double multiplier = perturbation*maximumFraction;
  for (iSequence = 0; iSequence < numberRows_ + numberColumns_; iSequence++) {
    if (getInternalStatus(iSequence) == basic) {
      double lowerValue = abcLower_[iSequence];
      double upperValue = abcUpper_[iSequence];
      if (upperValue > lowerValue + tolerance) {
	double solutionValue = abcSolution_[iSequence];
	double difference = upperValue - lowerValue;
	difference = CoinMin(difference, perturbation);
	difference = CoinMin(difference, fabs(solutionValue) + 1.0);
	double value = maximumFraction * (difference + bias);
	value = CoinMin(value, 0.1);
	value = CoinMax(value,primalTolerance_);
	double perturbationValue=overallMultiplier*randomNumberGenerator_.randomDouble();
	value *= perturbationValue;
	if (solutionValue - lowerValue <= primalTolerance_) {
	  abcLower_[iSequence] -= value;
	  // change correct saved value
	  if (abcNonLinearCost_->getCurrentStatus(iSequence)==CLP_FEASIBLE)
	    abcPerturbation_[iSequence]=abcLower_[iSequence];
	  else
	    abcPerturbation_[iSequence+numberTotal_]=abcLower_[iSequence];
	} else if (upperValue - solutionValue <= primalTolerance_) {
	  abcUpper_[iSequence] += value;
	  // change correct saved value
	  if (abcNonLinearCost_->getCurrentStatus(iSequence)==CLP_FEASIBLE)
	    abcPerturbation_[iSequence+numberTotal_]=abcUpper_[iSequence];
	  else
	    abcPerturbation_[iSequence]=abcUpper_[iSequence];
	} else {
#if 0
	  if (iSequence >= numberColumns_) {
	    // may not be at bound - but still perturb (unless free)
	    if (upperValue > 1.0e30 && lowerValue < -1.0e30)
	      value = 0.0;
	    else
	      value = - value; // as -1.0 in matrix
	  } else {
	    value = 0.0;
	  }
#else
	  value = 0.0;
#endif
	}
	if (value) {
	  numberChanged++;
	  if (printOut)
	    printf("col %d lower from %g to %g, upper from %g to %g\n",
		   iSequence, abcLower_[iSequence], lowerValue, abcUpper_[iSequence], upperValue);
	  if (solutionValue) {
	    largest = CoinMax(largest, value);
	    if (value > (fabs(solutionValue) + 1.0)*largestPerCent)
	      largestPerCent = value / (fabs(solutionValue) + 1.0);
	  } else {
	    largestZero = CoinMax(largestZero, value);
	  }
	}
      }
    }
  }
  if (!numberChanged) {
    // do non basic columns?
    for (iSequence = 0; iSequence < maximumAbcNumberRows_ + numberColumns_; iSequence++) {
      if (getInternalStatus(iSequence) != basic) {
	double lowerValue = abcLower_[iSequence];
	double upperValue = abcUpper_[iSequence];
	if (upperValue > lowerValue + tolerance) {
	  double solutionValue = abcSolution_[iSequence];
	  double difference = upperValue - lowerValue;
	  difference = CoinMin(difference, perturbation);
	  difference = CoinMin(difference, fabs(solutionValue) + 1.0);
	  double value = maximumFraction * (difference + bias);
	  value = CoinMin(value, 0.1);
	  value = CoinMax(value,primalTolerance_);
	  double perturbationValue=overallMultiplier*randomNumberGenerator_.randomDouble();
	  value *= perturbationValue;
	  if (solutionValue - lowerValue <= primalTolerance_) {
	    abcLower_[iSequence] -= value;
	    // change correct saved value
	    if (abcNonLinearCost_->getCurrentStatus(iSequence)==CLP_FEASIBLE)
	      abcPerturbation_[iSequence]=abcLower_[iSequence];
	    else
	      abcPerturbation_[iSequence+numberTotal_]=abcLower_[iSequence];
	  } else if (upperValue - solutionValue <= primalTolerance_) {
	    abcUpper_[iSequence] += value;
	    // change correct saved value
	    if (abcNonLinearCost_->getCurrentStatus(iSequence)==CLP_FEASIBLE)
	      abcPerturbation_[iSequence+numberTotal_]=abcUpper_[iSequence];
	    else
	      abcPerturbation_[iSequence]=abcUpper_[iSequence];
	  } else {
	    value = 0.0;
	  }
	  if (value) {
	    if (printOut)
	      printf("col %d lower from %g to %g, upper from %g to %g\n",
		     iSequence, abcLower_[iSequence], lowerValue, abcUpper_[iSequence], upperValue);
	    if (solutionValue) {
	      largest = CoinMax(largest, value);
	      if (value > (fabs(solutionValue) + 1.0)*largestPerCent)
		largestPerCent = value / (fabs(solutionValue) + 1.0);
	    } else {
	      largestZero = CoinMax(largestZero, value);
	    }
	  }
	}
      }
    }
  }
  // Clean up
  for (i = 0; i < numberColumns_ + numberRows_; i++) {
    switch(getInternalStatus(i)) {
      
    case basic:
      break;
    case atUpperBound:
      abcSolution_[i] = abcUpper_[i];
      break;
    case isFixed:
    case atLowerBound:
      abcSolution_[i] = abcLower_[i];
      break;
    case isFree:
      break;
    case superBasic:
      break;
    }
  }
  handler_->message(CLP_SIMPLEX_PERTURB, messages_)
    << 100.0 * maximumFraction << perturbation << largest << 100.0 * largestPerCent << largestZero
    << CoinMessageEol;
  // redo nonlinear costs
  //delete abcNonLinearCost_;abcNonLinearCost_=new AbcNonLinearCost(this);//abort();// something elseabcNonLinearCost_->refresh();
  moveToBasic();
  if (!numberChanged) {
    // we changed nonbasic
    gutsOfPrimalSolution(3);
    // Make sure feasible bounds
    if (abcNonLinearCost_) {
      //abcNonLinearCost_->refresh();
      abcNonLinearCost_->checkInfeasibilities(primalTolerance_);
      //abcNonLinearCost_->feasibleBounds();
    }
    abcPrimalColumnPivot_->saveWeights(this, 3);
  }
  // say perturbed
  perturbation_ = 101;
}
// un perturb
bool
AbcSimplexPrimal::unPerturb()
{
  if (perturbation_ != 101)
    return false;
  // put back original bounds and costs
  copyFromSaved();
  // copy bounds to perturbation
  CoinAbcMemcpy(abcPerturbation_,abcLower_,numberTotal_);
  CoinAbcMemcpy(abcPerturbation_+numberTotal_,abcUpper_,numberTotal_);
  //sanityCheck();
  // unflag
  unflag();
  abcProgress_.clearTimesFlagged();
  // get a valid nonlinear cost function
  delete abcNonLinearCost_;
  abcNonLinearCost_ = new AbcNonLinearCost(this);
  perturbation_ = 102; // stop any further perturbation
  // move non basic variables to new bounds
  abcNonLinearCost_->checkInfeasibilities(0.0);
  gutsOfSolution(3);
  abcNonLinearCost_->checkInfeasibilities(primalTolerance_);
  // Try using dual
  return abcNonLinearCost_->sumInfeasibilities()!=0.0;
  
}
// Unflag all variables and return number unflagged
int
AbcSimplexPrimal::unflag()
{
  int i;
  int number = numberRows_ + numberColumns_;
  int numberFlagged = 0;
  // we can't really trust infeasibilities if there is dual error
  // allow tolerance bigger than standard to check on duals
  double relaxedToleranceD = dualTolerance_ + CoinMin(1.0e-2, 10.0 * largestDualError_);
  for (i = 0; i < number; i++) {
    if (flagged(i)) {
      clearFlagged(i);
      // only say if reasonable dj
      if (fabs(abcDj_[i]) > relaxedToleranceD)
	numberFlagged++;
    }
  }
#if ABC_NORMAL_DEBUG>0
  if (handler_->logLevel() > 2 && numberFlagged && objective_->type() > 1)
    printf("%d unflagged\n", numberFlagged);
#endif
  return numberFlagged;
}
// Do not change infeasibility cost and always say optimal
void
AbcSimplexPrimal::alwaysOptimal(bool onOff)
{
  if (onOff)
    specialOptions_ |= 1;
  else
    specialOptions_ &= ~1;
}
bool
AbcSimplexPrimal::alwaysOptimal() const
{
  return (specialOptions_ & 1) != 0;
}
// Flatten outgoing variables i.e. - always to exact bound
void
AbcSimplexPrimal::exactOutgoing(bool onOff)
{
  if (onOff)
    specialOptions_ |= 4;
  else
    specialOptions_ &= ~4;
}
bool
AbcSimplexPrimal::exactOutgoing() const
{
  return (specialOptions_ & 4) != 0;
}
/*
  Reasons to come out (normal mode/user mode):
  -1 normal
  -2 factorize now - good iteration/ NA
  -3 slight inaccuracy - refactorize - iteration done/ same but factor done
  -4 inaccuracy - refactorize - no iteration/ NA
  -5 something flagged - go round again/ pivot not possible
  +2 looks unbounded
  +3 max iterations (iteration done)
*/
int
AbcSimplexPrimal::pivotResult(int ifValuesPass)
{
  
  bool roundAgain = true;
  int returnCode = -1;
  
  // loop round if user setting and doing refactorization
  while (roundAgain) {
    roundAgain = false;
    returnCode = -1;
    pivotRow_ = -1;
    sequenceOut_ = -1;
    usefulArray_[arrayForFtran_].clear();
    
    // we found a pivot column
    // update the incoming column
    unpack(usefulArray_[arrayForFtran_]);
    // save reduced cost
    double saveDj = dualIn_;
    abcFactorization_->updateColumnFT(usefulArray_[arrayForFtran_]);
    // do ratio test and re-compute dj
#ifdef CLP_USER_DRIVEN
    if ((moreSpecialOptions_ & 512) == 0) {
#endif
      primalRow(&usefulArray_[arrayForFtran_], &usefulArray_[arrayForFlipBounds_], 
		&usefulArray_[arrayForTableauRow_],
		ifValuesPass);
#ifdef CLP_USER_DRIVEN
      // user can tell which use it is
      int status = eventHandler_->event(ClpEventHandler::pivotRow);
      if (status >= 0) {
	problemStatus_ = 5;
	secondaryStatus_ = ClpEventHandler::pivotRow;
	break;
      }
    } else {
      int status = eventHandler_->event(ClpEventHandler::pivotRow);
      if (status >= 0) {
	problemStatus_ = 5;
	secondaryStatus_ = ClpEventHandler::pivotRow;
	break;
      }
    }
#endif
    if (ifValuesPass) {
      saveDj = dualIn_;
      //assert (fabs(alpha_)>=1.0e-5||(objective_->type()<2||!objective_->activated())||pivotRow_==-2);
      if (pivotRow_ == -1 || (pivotRow_ >= 0 && fabs(alpha_) < 1.0e-5)) {
	if(fabs(dualIn_) < 1.0e2 * dualTolerance_ && objective_->type() < 2) {
	  // try other way
	  directionIn_ = -directionIn_;
	  primalRow(&usefulArray_[arrayForFtran_], &usefulArray_[arrayForFlipBounds_], 
		    &usefulArray_[arrayForTableauRow_],
		    0);
	}
	if (pivotRow_ == -1 || (pivotRow_ >= 0 && fabs(alpha_) < 1.0e-5)) {
	  // reject it
	  char x = isColumn(sequenceIn_) ? 'C' : 'R';
	  handler_->message(CLP_SIMPLEX_FLAG, messages_)
	    << x << sequenceWithin(sequenceIn_)
	    << CoinMessageEol;
	  setFlagged(sequenceIn_);
	  abcProgress_.incrementTimesFlagged();
	  abcProgress_.clearBadTimes();
	  lastBadIteration_ = numberIterations_; // say be more cautious
	  clearAll();
	  pivotRow_ = -1;
	  returnCode = -5;
	  break;
	}
      }
    }
    double checkValue = 1.0e-2;
    if (largestDualError_ > 1.0e-5)
      checkValue = 1.0e-1;
    double test2 = dualTolerance_;
    double test1 = 1.0e-20;
#if 0 //def FEB_TRY
    if (abcFactorization_->pivots() < 1) {
      test1 = -1.0e-4;
      if ((saveDj < 0.0 && dualIn_ < -1.0e-5 * dualTolerance_) ||
	  (saveDj > 0.0 && dualIn_ > 1.0e-5 * dualTolerance_))
	test2 = 0.0; // allow through
    }
#endif
    if (!ifValuesPass && (saveDj * dualIn_ < test1 ||
					     fabs(saveDj - dualIn_) > checkValue*(1.0 + fabs(saveDj)) ||
			  fabs(dualIn_) < test2)) {
      if (!(saveDj * dualIn_ > 0.0 && CoinMin(fabs(saveDj), fabs(dualIn_)) >
	    1.0e5)) {
	char x = isColumn(sequenceIn_) ? 'C' : 'R';
	handler_->message(CLP_PRIMAL_DJ, messages_)
	  << x << sequenceWithin(sequenceIn_)
	  << saveDj << dualIn_
	  << CoinMessageEol;
	if(lastGoodIteration_ != numberIterations_) {
	  clearAll();
	  pivotRow_ = -1; // say no weights update
	  returnCode = -4;
	  if(lastGoodIteration_ + 1 == numberIterations_) {
	    // not looking wonderful - try cleaning bounds
	    // put non-basics to bounds in case tolerance moved
	    abcNonLinearCost_->checkInfeasibilities(0.0);
	  }
	  sequenceOut_ = -1;
	  sequenceIn_=-1;
	  if (abcFactorization_->pivots()<10&&abcFactorization_->pivotTolerance()<0.25)
	    abcFactorization_->saferTolerances(1.0,-1.03);
	  break;
	} else {
	  // take on more relaxed criterion
	  if (saveDj * dualIn_ < test1 ||
				 fabs(saveDj - dualIn_) > 2.0e-1 * (1.0 + fabs(dualIn_)) ||
	      fabs(dualIn_) < test2) {
	    // need to reject something
	    char x = isColumn(sequenceIn_) ? 'C' : 'R';
	    handler_->message(CLP_SIMPLEX_FLAG, messages_)
	      << x << sequenceWithin(sequenceIn_)
	      << CoinMessageEol;
	    setFlagged(sequenceIn_);
	    abcProgress_.incrementTimesFlagged();
#if 1 //def FEB_TRY
	    // Make safer?
	    double tolerance=abcFactorization_->pivotTolerance();
	    abcFactorization_->saferTolerances (1.0, -1.03);
#endif
	    abcProgress_.clearBadTimes();
	    lastBadIteration_ = numberIterations_; // say be more cautious
	    clearAll();
	    pivotRow_ = -1;
	    returnCode = -5;
	    if(tolerance<abcFactorization_->pivotTolerance())
	      returnCode=-4;
	    sequenceOut_ = -1;
	    sequenceIn_=-1;
	    break;
	  }
	}
      } else {
	//printf("%d %g %g\n",numberIterations_,saveDj,dualIn_);
      }
    }
    if (pivotRow_ >= 0) {
#ifdef CLP_USER_DRIVEN1
      // Got good pivot - may need to unflag stuff
      userChoiceWasGood(this);
#endif
      // if stable replace in basis
      // check update
      //abcFactorization_->checkReplacePart1a(&usefulArray_[arrayForReplaceColumn_],pivotRow_);
      //usefulArray_[arrayForReplaceColumn_].print();
      ftAlpha_=abcFactorization_->checkReplacePart1(&usefulArray_[arrayForReplaceColumn_],pivotRow_);
      int updateStatus=abcFactorization_->checkReplacePart2(pivotRow_,btranAlpha_,alpha_,ftAlpha_);
      abcFactorization_->replaceColumnPart3(this,
					    &usefulArray_[arrayForReplaceColumn_],
					    &usefulArray_[arrayForFtran_],
					    pivotRow_,
					    ftAlpha_?ftAlpha_:alpha_);
      
      // if no pivots, bad update but reasonable alpha - take and invert
      if (updateStatus == 2 &&
	  lastGoodIteration_ == numberIterations_ && fabs(alpha_) > 1.0e-5)
	updateStatus = 4;
      if (updateStatus == 1 || updateStatus == 4) {
	// slight error
	if (abcFactorization_->pivots() > 5 || updateStatus == 4) {
	  returnCode = -3;
	}
      } else if (updateStatus == 2) {
	// major error
	// better to have small tolerance even if slower
	abcFactorization_->zeroTolerance(CoinMin(abcFactorization_->zeroTolerance(), 1.0e-15));
	int maxFactor = abcFactorization_->maximumPivots();
	if (maxFactor > 10) {
	  if (forceFactorization_ < 0)
	    forceFactorization_ = maxFactor;
	  forceFactorization_ = CoinMax(1, (forceFactorization_ >> 1));
	}
	// later we may need to unwind more e.g. fake bounds
	if(lastGoodIteration_ != numberIterations_) {
	  clearAll();
	  pivotRow_ = -1;
	  sequenceIn_ = -1;
	  sequenceOut_ = -1;
	  returnCode = -4;
	  break;
	} else {
	  // need to reject something
	  char x = isColumn(sequenceIn_) ? 'C' : 'R';
	  handler_->message(CLP_SIMPLEX_FLAG, messages_)
	    << x << sequenceWithin(sequenceIn_)
	    << CoinMessageEol;
	  setFlagged(sequenceIn_);
	  abcProgress_.incrementTimesFlagged();
	  abcProgress_.clearBadTimes();
	  lastBadIteration_ = numberIterations_; // say be more cautious
	  clearAll();
	  pivotRow_ = -1;
	  sequenceIn_ = -1;
	  sequenceOut_ = -1;
	  returnCode = -5;
	  break;
	  
	}
      } else if (updateStatus == 3) {
	// out of memory
	// increase space if not many iterations
	if (abcFactorization_->pivots() <
	    0.5 * abcFactorization_->maximumPivots() &&
	    abcFactorization_->pivots() < 200)
	  abcFactorization_->areaFactor(
					abcFactorization_->areaFactor() * 1.1);
	returnCode = -2; // factorize now
      } else if (updateStatus == 5) {
	problemStatus_ = -2; // factorize now
      }
      // here do part of steepest - ready for next iteration
      if (!ifValuesPass)
	abcPrimalColumnPivot_->updateWeights(&usefulArray_[arrayForFtran_]);
    } else {
      if (pivotRow_ == -1) {
	// no outgoing row is valid
	if (valueOut_ != COIN_DBL_MAX) {
	  double objectiveChange = 0.0;
	  theta_ = valueOut_ - valueIn_;
	  updatePrimalsInPrimal(&usefulArray_[arrayForFtran_], theta_, objectiveChange, ifValuesPass);
	  abcSolution_[sequenceIn_] += theta_;
	}
#ifdef CLP_USER_DRIVEN1
	/* Note if valueOut_ < COIN_DBL_MAX and
	   theta_ reasonable then this may be a valid sub flip */
	if(!userChoiceValid2(this)) {
	  if (abcFactorization_->pivots()<5) {
	    // flag variable
	    char x = isColumn(sequenceIn_) ? 'C' : 'R';
	    handler_->message(CLP_SIMPLEX_FLAG, messages_)
	      << x << sequenceWithin(sequenceIn_)
	      << CoinMessageEol;
	    setFlagged(sequenceIn_);
	    abcProgress_.incrementTimesFlagged();
	    abcProgress_.clearBadTimes();
	    roundAgain = true;
	    continue;
	  } else {
	    // try refactorizing first
	    returnCode = 4; //say looks odd but has iterated
	    break;
	  }
	}
#endif
	if (!abcFactorization_->pivots() && acceptablePivot_ <= 1.0e-8) {
	  returnCode = 2; //say looks unbounded
	  // do ray
	  primalRay(&usefulArray_[arrayForFtran_]);
	} else {
	  acceptablePivot_ = 1.0e-8;
	  returnCode = 4; //say looks unbounded but has iterated
	}
	break;
      } else {
	// flipping from bound to bound
      }
    }
    
    double oldCost = 0.0;
    if (sequenceOut_ >= 0)
      oldCost = abcCost_[sequenceOut_];
    // update primal solution
    
    double objectiveChange = 0.0;
    // after this usefulArray_[arrayForFtran_] is not empty - used to update djs
#ifdef CLP_USER_DRIVEN
    if (theta_<0.0) {
      if (theta_>=-1.0e-12)
	theta_=0.0;
      //else
      //printf("negative theta %g\n",theta_);
    }
#endif
    updatePrimalsInPrimal(&usefulArray_[arrayForFtran_], theta_, objectiveChange, ifValuesPass);
    
    double oldValue = valueIn_;
    if (directionIn_ == -1) {
      // as if from upper bound
      if (sequenceIn_ != sequenceOut_) {
	// variable becoming basic
	valueIn_ -= fabs(theta_);
      } else {
	valueIn_ = lowerIn_;
      }
    } else {
      // as if from lower bound
      if (sequenceIn_ != sequenceOut_) {
	// variable becoming basic
	valueIn_ += fabs(theta_);
      } else {
	valueIn_ = upperIn_;
      }
    }
    objectiveChange += dualIn_ * (valueIn_ - oldValue);
    // outgoing
    if (sequenceIn_ != sequenceOut_) {
      if (directionOut_ > 0) {
	valueOut_ = lowerOut_;
      } else {
	valueOut_ = upperOut_;
      }
      if(valueOut_ < abcLower_[sequenceOut_] - primalTolerance_)
	valueOut_ = abcLower_[sequenceOut_] - 0.9 * primalTolerance_;
      else if (valueOut_ > abcUpper_[sequenceOut_] + primalTolerance_)
	valueOut_ = abcUpper_[sequenceOut_] + 0.9 * primalTolerance_;
      // may not be exactly at bound and bounds may have changed
      // Make sure outgoing looks feasible
      directionOut_ = abcNonLinearCost_->setOneOutgoing(pivotRow_, valueOut_);
      // May have got inaccurate
      //if (oldCost!=abcCost_[sequenceOut_])
      //printf("costchange on %d from %g to %g\n",sequenceOut_,
      //       oldCost,abcCost_[sequenceOut_]);
      abcDj_[sequenceOut_] = abcCost_[sequenceOut_] - oldCost; // normally updated next iteration
      abcSolution_[sequenceOut_] = valueOut_;
    }
    // change cost and bounds on incoming if primal
    abcNonLinearCost_->setOne(sequenceIn_, valueIn_);
    int whatNext = housekeeping(/*objectiveChange*/);
    swapPrimalStuff();
    if (whatNext == 1) {
      returnCode = -2; // refactorize
    } else if (whatNext == 2) {
      // maximum iterations or equivalent
      returnCode = 3;
    } else if(numberIterations_ == lastGoodIteration_
	      + 2 * abcFactorization_->maximumPivots()) {
      // done a lot of flips - be safe
      returnCode = -2; // refactorize
    }
    // Check event
    {
      int status = eventHandler_->event(ClpEventHandler::endOfIteration);
      if (status >= 0) {
	problemStatus_ = 5;
	secondaryStatus_ = ClpEventHandler::endOfIteration;
	returnCode = 3;
      }
    }
  }
  return returnCode;
}
/*
  Reasons to come out (normal mode/user mode):
  -1 normal
  -2 factorize now - good iteration/ NA
  -3 slight inaccuracy - refactorize - iteration done/ same but factor done
  -4 inaccuracy - refactorize - no iteration/ NA
  -5 something flagged - go round again/ pivot not possible
  +2 looks unbounded
  +3 max iterations (iteration done)
*/
int
AbcSimplexPrimal::pivotResult4(int ifValuesPass)
{
  
  int returnCode = -1;
  int numberMinor=0;
  int numberDone=0;
  CoinIndexedVector * vector[16];
  pivotStruct stuff[4];
  vector[0]=&usefulArray_[arrayForFtran_];
  vector[1]=&usefulArray_[arrayForFlipBounds_]; 
  vector[2]=&usefulArray_[arrayForTableauRow_];
  vector[3]=&usefulArray_[arrayForBtran_];
  /*
    For pricing we need to get a vector with difference in costs
    later we could modify code so only costBasic changed
    and store in first [4*x+1] array to go out the indices of changed
    plus at most four for pivots
  */
  //double * saveCosts=CoinCopyOfArray(costBasic_,numberRows_);
  //double saveCostsIn[4];
  for (int iMinor=0;iMinor<4;iMinor++) {
    int sequenceIn=multipleSequenceIn_[iMinor];
    if (sequenceIn<0)
      break;
    stuff[iMinor].valuesPass_=ifValuesPass;
    stuff[iMinor].lowerIn_=abcLower_[sequenceIn];
    stuff[iMinor].upperIn_=abcUpper_[sequenceIn];
    stuff[iMinor].valueIn_=abcSolution_[sequenceIn];
    stuff[iMinor].sequenceIn_=sequenceIn;
    //saveCostsIn[iMinor]=abcCost_[sequenceIn];
    numberMinor++;
    if (iMinor) {
      vector[4*iMinor]=rowArray_[2*iMinor-2];
      vector[4*iMinor+1]=rowArray_[2*iMinor-1];
      vector[4*iMinor+2]=columnArray_[2*iMinor-2];
      vector[4*iMinor+3]=columnArray_[2*iMinor-1];
    }
    for (int i=0;i<4;i++)
      vector[4*iMinor+i]->checkClear();
    unpack(*vector[4*iMinor],sequenceIn);
  }
  int numberLeft=numberMinor;
  // parallel (with cpu)
  for (int iMinor=1;iMinor<numberMinor;iMinor++) {
    // update the incoming columns
    cilk_spawn abcFactorization_->updateColumnFT(*vector[4*iMinor],*vector[4*iMinor+3],iMinor);
  }
  abcFactorization_->updateColumnFT(*vector[0],*vector[+3],0);
  cilk_sync;
  for (int iMinor=0;iMinor<numberMinor;iMinor++) {
    // find best (or first if values pass)
    int numberDo=1;
    int jMinor=0;
    while (jMinor<numberDo) {
      int sequenceIn=stuff[jMinor].sequenceIn_;
      double dj=abcDj_[sequenceIn];
      bool bad=false;
      if (!bad) {
	stuff[jMinor].dualIn_=dj;
	stuff[jMinor].saveDualIn_=dj;
	if (dj<0.0)
	  stuff[jMinor].directionIn_=1;
	else
	  stuff[jMinor].directionIn_=-1;
	jMinor++;
      } else {
	numberDo--;
	numberLeft--;
	// throw away
	for (int i=0;i<4;i++) {
	  vector[4*jMinor+i]->clear();
	  vector[4*jMinor+i]=vector[4*numberDo+i];
	  vector[4*numberDo+i]=NULL;
	}
	stuff[jMinor]=stuff[numberDo];
      }
    }
    for (int jMinor=0;jMinor<numberDo;jMinor++) {
      // do ratio test and re-compute dj
      primalRow(vector[4*jMinor],vector[4*jMinor+1],vector[4*jMinor+2],stuff[jMinor]);
    }
    // choose best
    int iBest=-1;
    double bestMovement=-COIN_DBL_MAX;
    for (int jMinor=0;jMinor<numberDo;jMinor++) {
      double movement=stuff[jMinor].theta_*fabs(stuff[jMinor].dualIn_);
      if (movement>bestMovement) {
	bestMovement=movement;
	iBest=jMinor;
      }
    }
#if 0 //ndef MULTIPLE_PRICE
    if (maximumIterations()!=100000)
      iBest=0;
#endif
    if (iBest>=0) {
      dualIn_=stuff[iBest].dualIn_;
      dualOut_=stuff[iBest].dualOut_;
      lowerOut_=stuff[iBest].lowerOut_;
      upperOut_=stuff[iBest].upperOut_;
      valueOut_=stuff[iBest].valueOut_;
      sequenceOut_=stuff[iBest].sequenceOut_;
      sequenceIn_=stuff[iBest].sequenceIn_;
      lowerIn_=stuff[iBest].lowerIn_;
      upperIn_=stuff[iBest].upperIn_;
      valueIn_=stuff[iBest].valueIn_;
      directionIn_=stuff[iBest].directionIn_;
      directionOut_=stuff[iBest].directionOut_;
      pivotRow_=stuff[iBest].pivotRow_;
      theta_=stuff[iBest].theta_;
      alpha_=stuff[iBest].alpha_;
#ifdef MULTIPLE_PRICE
      for (int i=0;i<4*numberLeft;i++) 
	vector[i]->clear();
      return 0;
#endif
      // maybe do this on more?
      double theta1 = CoinMax(theta_, 1.0e-12);
      double theta2 = numberIterations_ * abcNonLinearCost_->averageTheta();
      // Set average theta
      abcNonLinearCost_->setAverageTheta((theta1 + theta2) / (static_cast<double> (numberIterations_ + 1)));
      if (pivotRow_ == -1 || (pivotRow_ >= 0 && fabs(alpha_) < 1.0e-5)) {
	if(fabs(dualIn_) < 1.0e2 * dualTolerance_) {
	  // try other way
	  stuff[iBest].directionIn_ = -directionIn_;
	  stuff[iBest].valuesPass_=0;
	  primalRow(vector[4*iBest],vector[4*iBest+1],vector[4*iBest+2],stuff[iBest]);
	  dualIn_=stuff[iBest].dualIn_;
	  dualOut_=stuff[iBest].dualOut_;
	  lowerOut_=stuff[iBest].lowerOut_;
	  upperOut_=stuff[iBest].upperOut_;
	  valueOut_=stuff[iBest].valueOut_;
	  sequenceOut_=stuff[iBest].sequenceOut_;
	  sequenceIn_=stuff[iBest].sequenceIn_;
	  lowerIn_=stuff[iBest].lowerIn_;
	  upperIn_=stuff[iBest].upperIn_;
	  valueIn_=stuff[iBest].valueIn_;
	  directionIn_=stuff[iBest].directionIn_;
	  directionOut_=stuff[iBest].directionOut_;
	  pivotRow_=stuff[iBest].pivotRow_;
	  theta_=stuff[iBest].theta_;
	  alpha_=stuff[iBest].alpha_;
	}
	if (pivotRow_ == -1 || (pivotRow_ >= 0 && fabs(alpha_) < 1.0e-5)) {
	  // reject it
	  char x = isColumn(sequenceIn_) ? 'C' : 'R';
	  handler_->message(CLP_SIMPLEX_FLAG, messages_)
	    << x << sequenceWithin(sequenceIn_)
	    << CoinMessageEol;
	  setFlagged(sequenceIn_);
	  abcProgress_.incrementTimesFlagged();
	  abcProgress_.clearBadTimes();
	  lastBadIteration_ = numberIterations_; // say be more cautious
	  clearAll();
	  pivotRow_ = -1;
	  returnCode = -5;
	  break;
	}
      }
      CoinIndexedVector * bestUpdate=NULL;
      if (pivotRow_ >= 0) {
	// Normal pivot
	// if stable replace in basis
	// check update
	CoinIndexedVector * tempVector[4];
	memcpy(tempVector,vector+4*iBest,4*sizeof(CoinIndexedVector *));
	returnCode = cilk_spawn doFTUpdate(tempVector);
	bestUpdate=tempVector[0];
	// after this bestUpdate is not empty - used to update djs ??
	cilk_spawn updatePrimalsInPrimal(*bestUpdate, theta_, ifValuesPass);
	numberDone++;
	numberLeft--;
	// throw away
	for (int i=0;i<4;i++) {
	  vector[4*iBest+i]=vector[4*numberLeft+i];
	  vector[4*numberLeft+i]=NULL;
	}
	stuff[iBest]=stuff[numberLeft];
	// update pi and other vectors and FT stuff
	// parallel (can go 8 way?)
	for (int jMinor=0;jMinor<numberLeft;jMinor++) {
	  cilk_spawn updatePartialUpdate(*vector[4*jMinor+3]);
	}
	// parallel
	for (int jMinor=0;jMinor<numberLeft;jMinor++) {
	  stuff[jMinor].dualIn_=cilk_spawn updateMinorCandidate(*bestUpdate,*vector[4*jMinor],stuff[jMinor].sequenceIn_);
	}
	cilk_sync;
	// throw away
	for (int i=1;i<4;i++) 
	  tempVector[i]->clear();
	if (returnCode<-3) {
	  clearAll();
	  break;
	}
	// end Normal pivot
      } else {
	// start Flip
	vector[4*iBest+3]->clear();
	if (pivotRow_ == -1) {
	  // no outgoing row is valid
	  if (valueOut_ != COIN_DBL_MAX) {
	    theta_ = valueOut_ - valueIn_;
	    updatePrimalsInPrimal(*vector[4*iBest], theta_, ifValuesPass);
	    abcSolution_[sequenceIn_] += theta_;
	  }
	  if (!abcFactorization_->pivots() && acceptablePivot_ <= 1.0e-8) {
	    returnCode = 2; //say looks unbounded
	    // do ray
	    primalRay(vector[4*iBest]);
	  } else {
	    acceptablePivot_ = 1.0e-8;
	    returnCode = 4; //say looks unbounded but has iterated
	  }
	  break;
	} else {
	  // flipping from bound to bound
	  bestUpdate=vector[4*iBest];
	  // after this bestUpdate is not empty - used to update djs ??
	  updatePrimalsInPrimal(*bestUpdate, theta_, ifValuesPass);
	  // throw away best BUT remember we are updating costs somehow
	  numberDone++;
	  numberLeft--;
	  // throw away
	  for (int i=0;i<4;i++) {
	    if (i)
	      vector[4*iBest+i]->clear();
	    vector[4*iBest+i]=vector[4*numberLeft+i];
	    vector[4*numberLeft+i]=NULL;
	  }
	  stuff[iBest]=stuff[numberLeft];
	  // parallel
	  for (int jMinor=0;jMinor<numberLeft;jMinor++) {
	    stuff[jMinor].dualIn_=cilk_spawn updateMinorCandidate(*bestUpdate,*vector[4*jMinor],stuff[jMinor].sequenceIn_);
	  }
	  cilk_sync;
	}
	// end Flip
      }
      bestUpdate->clear(); // as only works in values pass

      if (directionIn_ == -1) {
	// as if from upper bound
	if (sequenceIn_ != sequenceOut_) {
	  // variable becoming basic
	  valueIn_ -= fabs(theta_);
	} else {
	  valueIn_ = lowerIn_;
	}
      } else {
	// as if from lower bound
	if (sequenceIn_ != sequenceOut_) {
	  // variable becoming basic
	  valueIn_ += fabs(theta_);
	} else {
	  valueIn_ = upperIn_;
	}
      }
      // outgoing
      if (sequenceIn_ != sequenceOut_) {
	if (directionOut_ > 0) {
	  valueOut_ = lowerOut_;
	} else {
	  valueOut_ = upperOut_;
	}
	if(valueOut_ < abcLower_[sequenceOut_] - primalTolerance_)
	  valueOut_ = abcLower_[sequenceOut_] - 0.9 * primalTolerance_;
	else if (valueOut_ > abcUpper_[sequenceOut_] + primalTolerance_)
	  valueOut_ = abcUpper_[sequenceOut_] + 0.9 * primalTolerance_;
	// may not be exactly at bound and bounds may have changed
	// Make sure outgoing looks feasible
	directionOut_ = abcNonLinearCost_->setOneOutgoing(pivotRow_, valueOut_);
	abcSolution_[sequenceOut_] = valueOut_;
      }
      // change cost and bounds on incoming if primal
      abcNonLinearCost_->setOne(sequenceIn_, valueIn_);
      int whatNext = housekeeping(/*objectiveChange*/);
      swapPrimalStuff();
      if (whatNext == 1) {
	returnCode = -2; // refactorize
      } else if (whatNext == 2) {
	// maximum iterations or equivalent
	returnCode = 3;
      } else if(numberIterations_ == lastGoodIteration_
		+ 2 * abcFactorization_->maximumPivots()) {
	// done a lot of flips - be safe
	returnCode = -2; // refactorize
      }
      // Check event
      {
	int status = eventHandler_->event(ClpEventHandler::endOfIteration);
	if (status >= 0) {
	  problemStatus_ = 5;
	  secondaryStatus_ = ClpEventHandler::endOfIteration;
	  returnCode = 3;
	}
      }
    }
    if (returnCode!=-1)
      break;
  }
  for (int i=0;i<16;i++) {
    if (vector[i])
      vector[i]->clear();
    if (vector[i])
      vector[i]->checkClear();
  }
  //delete [] saveCosts;
  return returnCode;
}
// Do FT update as separate function for minor iterations (nonzero return code on problems)
int 
AbcSimplexPrimal::doFTUpdate(CoinIndexedVector * vector[4])
{
  ftAlpha_=abcFactorization_->checkReplacePart1(&usefulArray_[arrayForReplaceColumn_],
						vector[3],pivotRow_);
  int updateStatus=abcFactorization_->checkReplacePart2(pivotRow_,btranAlpha_,alpha_,ftAlpha_);
  if (!updateStatus)
    abcFactorization_->replaceColumnPart3(this,
					  &usefulArray_[arrayForReplaceColumn_],
					  vector[0],
					  vector[3],
					  pivotRow_,
					  ftAlpha_?ftAlpha_:alpha_);
  else
    vector[3]->clear();
  int returnCode=0;
  // if no pivots, bad update but reasonable alpha - take and invert
  if (updateStatus == 2 &&
      lastGoodIteration_ == numberIterations_ && fabs(alpha_) > 1.0e-5)
    updateStatus = 4;
  if (updateStatus == 1 || updateStatus == 4) {
    // slight error
    if (abcFactorization_->pivots() > 5 || updateStatus == 4) {
      returnCode = -3;
    }
  } else if (updateStatus == 2) {
    // major error
    // better to have small tolerance even if slower
    abcFactorization_->zeroTolerance(CoinMin(abcFactorization_->zeroTolerance(), 1.0e-15));
    int maxFactor = abcFactorization_->maximumPivots();
    if (maxFactor > 10) {
      if (forceFactorization_ < 0)
	forceFactorization_ = maxFactor;
      forceFactorization_ = CoinMax(1, (forceFactorization_ >> 1));
    }
    // later we may need to unwind more e.g. fake bounds
    if(lastGoodIteration_ != numberIterations_) {
      pivotRow_ = -1;
      sequenceIn_ = -1;
      sequenceOut_ = -1;
      returnCode = -4;
    } else {
      // need to reject something
      char x = isColumn(sequenceIn_) ? 'C' : 'R';
      handler_->message(CLP_SIMPLEX_FLAG, messages_)
	<< x << sequenceWithin(sequenceIn_)
	<< CoinMessageEol;
      setFlagged(sequenceIn_);
      abcProgress_.incrementTimesFlagged();
      abcProgress_.clearBadTimes();
      lastBadIteration_ = numberIterations_; // say be more cautious
      pivotRow_ = -1;
      sequenceIn_ = -1;
      sequenceOut_ = -1;
      returnCode = -5;
      
    }
  } else if (updateStatus == 3) {
    // out of memory
    // increase space if not many iterations
    if (abcFactorization_->pivots() <
	0.5 * abcFactorization_->maximumPivots() &&
	abcFactorization_->pivots() < 200)
      abcFactorization_->areaFactor(
				    abcFactorization_->areaFactor() * 1.1);
    returnCode = -2; // factorize now
  } else if (updateStatus == 5) {
    problemStatus_ = -2; // factorize now
    returnCode=-2;
  }
  return returnCode;
}
/* The primals are updated by the given array.
   costs are changed
*/
void
AbcSimplexPrimal::updatePrimalsInPrimal(CoinIndexedVector & rowArray,
					double theta,bool valuesPass)
{
  // Cost on pivot row may change - may need to change dualIn
  //double oldCost = 0.0;
  //if (pivotRow_ >= 0)
  //oldCost = abcCost_[sequenceOut_];
  double * work = rowArray.denseVector();
  int number = rowArray.getNumElements();
  int * which = rowArray.getIndices();
  // allow for case where bound+tolerance == bound
  //double tolerance = 0.999999*primalTolerance_;
  double relaxedTolerance = 1.001 * primalTolerance_;
  int iIndex;
  if (!valuesPass) {
    for (iIndex = 0; iIndex < number; iIndex++) {
      
      int iRow = which[iIndex];
      double alpha = work[iRow];
      //work[iRow] = 0.0;
      int iPivot = abcPivotVariable_[iRow];
      double change = theta * alpha;
      double value = abcSolution_[iPivot] - change;
      abcSolution_[iPivot] = value;
      value = solutionBasic_[iRow] - change;
      solutionBasic_[iRow] = value;
      if (active(iRow) || theta_ < 0.0) {
	clearActive(iRow);
	// But make sure one going out is feasible
	if (change > 0.0) {
	  // going down
	  if (value <= abcLower_[iPivot] + primalTolerance_) {
	    if (iPivot == sequenceOut_ && value >= abcLower_[iPivot] - relaxedTolerance)
	      value = abcLower_[iPivot];
	    abcNonLinearCost_->setOneBasic(iRow, value);
	  }
	} else {
	  // going up
	  if (value >= abcUpper_[iPivot] - primalTolerance_) {
	    if (iPivot == sequenceOut_ && value < abcUpper_[iPivot] + relaxedTolerance)
	      value = abcUpper_[iPivot];
	    abcNonLinearCost_->setOneBasic(iRow, value);
	  }
	}
      }
    }
  } else {
    // values pass so look at all
    for (iIndex = 0; iIndex < number; iIndex++) {
      
      int iRow = which[iIndex];
      double alpha = work[iRow];
      //work[iRow] = 0.0;
      int iPivot = abcPivotVariable_[iRow];
      double change = theta * alpha;
      double value = abcSolution_[iPivot] - change;
      abcSolution_[iPivot] = value;
      value = solutionBasic_[iRow] - change;
      solutionBasic_[iRow] = value;
      clearActive(iRow);
      // But make sure one going out is feasible
      if (change > 0.0) {
	// going down
	if (value <= abcLower_[iPivot] + primalTolerance_) {
	  if (iPivot == sequenceOut_ && value > abcLower_[iPivot] - relaxedTolerance)
	    value = abcLower_[iPivot];
	  abcNonLinearCost_->setOneBasic(iRow, value);
	}
      } else {
	// going up
	if (value >= abcUpper_[iPivot] - primalTolerance_) {
	  if (iPivot == sequenceOut_ && value < abcUpper_[iPivot] + relaxedTolerance)
	    value = abcUpper_[iPivot];
	  abcNonLinearCost_->setOneBasic(iRow, value);
	}
      }
    }
  }
  //rowArray.setNumElements(0);
}
/* After rowArray will have cost changes for use next major iteration
 */
void 
AbcSimplexPrimal::createUpdateDuals(CoinIndexedVector & rowArray,
				    const double * originalCost,
				    const double extraCost[4],
				    double & /*objectiveChange*/,
				    int /*valuesPass*/)
{
  int number=0;
  double * work = rowArray.denseVector();
  int * which = rowArray.getIndices();
  for (int iRow=0;iRow<numberRows_;iRow++) {
    if (originalCost[iRow]!=costBasic_[iRow]) {
      work[iRow]=costBasic_[iRow]-originalCost[iRow];
      which[number++]=iRow;
    }
  }
  rowArray.setNumElements(number);
}
// Update minor candidate vector
double
AbcSimplexPrimal::updateMinorCandidate(const CoinIndexedVector & updateBy,
				       CoinIndexedVector & candidate,
				       int sequenceIn)
{
  const double * COIN_RESTRICT regionUpdate = updateBy.denseVector (  );
  const int * COIN_RESTRICT regionIndexUpdate = updateBy.getIndices (  );
  int numberNonZeroUpdate = updateBy.getNumElements (  );
  double * COIN_RESTRICT regionCandidate = candidate.denseVector (  );
  int * COIN_RESTRICT regionIndexCandidate = candidate.getIndices (  );
  int numberNonZeroCandidate = candidate.getNumElements (  );
  assert (fabs(alpha_-regionUpdate[pivotRow_])<1.0e-5);
  double value=regionCandidate[pivotRow_];
  if (value) {
    value /= alpha_;
    for (int i=0;i<numberNonZeroUpdate;i++) {
      int iRow=regionIndexUpdate[i];
      double oldValue = regionCandidate[iRow];
      double newValue = oldValue-value*regionUpdate[iRow];
      if (oldValue) {
	if (!newValue)
	  newValue=COIN_INDEXED_REALLY_TINY_ELEMENT;
	regionCandidate[iRow]=newValue;
      } else {
	assert (newValue);
	regionCandidate[iRow]=newValue;
	regionIndexCandidate[numberNonZeroCandidate++]=iRow;
      }
    }
    double oldValue = regionCandidate[pivotRow_];
    double newValue = value;
    if (oldValue) {
      if (!newValue)
	newValue=COIN_INDEXED_REALLY_TINY_ELEMENT;
      regionCandidate[pivotRow_]=newValue;
    } else {
      assert (newValue);
      regionCandidate[pivotRow_]=newValue;
      regionIndexCandidate[numberNonZeroCandidate++]=pivotRow_;
    }
    candidate.setNumElements(numberNonZeroCandidate);
  }
  double newDj=abcCost_[sequenceIn];
  for (int i = 0; i < numberNonZeroCandidate; i++) {
    int iRow = regionIndexCandidate[i];
    double alpha = regionCandidate[iRow];
    newDj -= alpha * costBasic_[iRow];
  }
  return newDj;
}
// Update partial Ftran by R update
void 
AbcSimplexPrimal::updatePartialUpdate(CoinIndexedVector & partialUpdate)
{
  CoinAbcFactorization * factorization=
    dynamic_cast<CoinAbcFactorization *>(abcFactorization_->factorization());
  if (factorization)
    factorization->updatePartialUpdate(partialUpdate);
}
// Create primal ray
void
AbcSimplexPrimal::primalRay(CoinIndexedVector * rowArray)
{
  delete [] ray_;
  ray_ = new double [numberColumns_];
  CoinZeroN(ray_, numberColumns_);
  int number = rowArray->getNumElements();
  int * index = rowArray->getIndices();
  double * array = rowArray->denseVector();
  double way = -directionIn_;
  int i;
  double zeroTolerance = 1.0e-12;
  if (sequenceIn_ < numberColumns_)
    ray_[sequenceIn_] = directionIn_;
  for (i = 0; i < number; i++) {
    int iRow = index[i];
    int iPivot = abcPivotVariable_[iRow];
    double arrayValue = array[iRow];
    if (iPivot < numberColumns_ && fabs(arrayValue) >= zeroTolerance)
      ray_[iPivot] = way * arrayValue;
  }
}
/* Get next superbasic -1 if none,
   Normal type is 1
   If type is 3 then initializes sorted list
   if 2 uses list.
*/
int
AbcSimplexPrimal::nextSuperBasic(int superBasicType,
                                 CoinIndexedVector * columnArray)
{
  int returnValue = -1;
  bool finished = false;
  while (!finished) {
    returnValue = firstFree_;
    int iColumn = firstFree_ + 1;
    if (superBasicType > 1) {
      if (superBasicType > 2) {
	// Initialize list
	// Wild guess that lower bound more natural than upper
	int number = 0;
	double * work = columnArray->denseVector();
	int * which = columnArray->getIndices();
	for (iColumn = 0; iColumn < numberRows_ + numberColumns_; iColumn++) {
	  if (!flagged(iColumn)) {
	    if (getInternalStatus(iColumn) == superBasic) {
	      if (fabs(abcSolution_[iColumn] - abcLower_[iColumn]) <= primalTolerance_) {
		abcSolution_[iColumn] = abcLower_[iColumn];
		setInternalStatus(iColumn, atLowerBound);
	      } else if (fabs(abcSolution_[iColumn] - abcUpper_[iColumn])
			 <= primalTolerance_) {
		abcSolution_[iColumn] = abcUpper_[iColumn];
		setInternalStatus(iColumn, atUpperBound);
	      } else if (abcLower_[iColumn] < -1.0e20 && abcUpper_[iColumn] > 1.0e20) {
		setInternalStatus(iColumn, isFree);
		break;
	      } else if (!flagged(iColumn)) {
		// put ones near bounds at end after sorting
		work[number] = - CoinMin(0.1 * (abcSolution_[iColumn] - abcLower_[iColumn]),
					 abcUpper_[iColumn] - abcSolution_[iColumn]);
		which[number++] = iColumn;
	      }
	    }
	  }
	}
	CoinSort_2(work, work + number, which);
	columnArray->setNumElements(number);
	CoinZeroN(work, number);
      }
      int * which = columnArray->getIndices();
      int number = columnArray->getNumElements();
      if (!number) {
	// finished
	iColumn = numberRows_ + numberColumns_;
	returnValue = -1;
      } else {
	number--;
	returnValue = which[number];
	iColumn = returnValue;
	columnArray->setNumElements(number);
      }
    } else {
      for (; iColumn < numberRows_ + numberColumns_; iColumn++) {
	if (!flagged(iColumn)) {
	  if (getInternalStatus(iColumn) == superBasic||
	      getInternalStatus(iColumn) == isFree) {
	    if (fabs(abcSolution_[iColumn] - abcLower_[iColumn]) <= primalTolerance_) {
	      abcSolution_[iColumn] = abcLower_[iColumn];
	      setInternalStatus(iColumn, atLowerBound);
	    } else if (fabs(abcSolution_[iColumn] - abcUpper_[iColumn])
		       <= primalTolerance_) {
	      abcSolution_[iColumn] = abcUpper_[iColumn];
	      setInternalStatus(iColumn, atUpperBound);
	    } else if (abcLower_[iColumn] < -1.0e20 && abcUpper_[iColumn] > 1.0e20) {
	      setInternalStatus(iColumn, isFree);
	      if (fabs(abcDj_[iColumn])>10.0*dualTolerance_)
		break;
	    } else {
	      break;
	    }
	  }
	}
      }
    }
    firstFree_ = iColumn;
    finished = true;
    if (firstFree_ == numberRows_ + numberColumns_)
      firstFree_ = -1;
    if (returnValue >= 0 && getInternalStatus(returnValue) != superBasic && getInternalStatus(returnValue) != isFree)
      finished = false; // somehow picked up odd one
  }
  return returnValue;
}
void
AbcSimplexPrimal::clearAll()
{
  int number = usefulArray_[arrayForFtran_].getNumElements();
  int * which = usefulArray_[arrayForFtran_].getIndices();
  
  int iIndex;
  for (iIndex = 0; iIndex < number; iIndex++) {
    
    int iRow = which[iIndex];
    clearActive(iRow);
  }
  usefulArray_[arrayForFtran_].clear();
  for (int i=0;i<6;i++) {
    if (rowArray_[i])
      rowArray_[i]->clear();
    if (columnArray_[i])
      columnArray_[i]->clear();
  }
}
