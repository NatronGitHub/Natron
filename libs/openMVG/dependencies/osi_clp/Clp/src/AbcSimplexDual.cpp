/* $Id$ */
// Copyright (C) 2002, International Business Machines
// Corporation and others, Copyright (C) 2012, FasterCoin.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

/* Notes on implementation of dual simplex algorithm.
   
   When dual feasible:
   
   If primal feasible, we are optimal.  Otherwise choose an infeasible
   basic variable to leave basis (normally going to nearest bound) (B).  We
   now need to find an incoming variable which will leave problem
   dual feasible so we get the row of the tableau corresponding to
   the basic variable (with the correct sign depending if basic variable
   above or below feasibility region - as that affects whether reduced
   cost on outgoing variable has to be positive or negative).
   
   We now perform a ratio test to determine which incoming variable will
   preserve dual feasibility (C).  If no variable found then problem
   is infeasible (in primal sense).  If there is a variable, we then
   perform pivot and repeat.  Trivial?
   
   -------------------------------------------
   
   A) How do we get dual feasible?  If all variables have bounds then
   it is trivial to get feasible by putting non-basic variables to
   correct bounds.  OSL did not have a phase 1/phase 2 approach but
   instead effectively put fake bounds on variables and this is the
   approach here, although I had hoped to make it cleaner.
   
   If there is a weight of X on getting dual feasible:
   Non-basic variables with negative reduced costs are put to
   lesser of their upper bound and their lower bound + X.
   Similarly, mutatis mutandis, for positive reduced costs.
   
   Free variables should normally be in basis, otherwise I have
   coding which may be able to come out (and may not be correct).
   
   In OSL, this weight was changed heuristically, here at present
   it is only increased if problem looks finished.  If problem is
   feasible I check for unboundedness.  If not unbounded we
   could play with going into primal.  As long as weights increase
   any algorithm would be finite.
   
   B) Which outgoing variable to choose is a virtual base class.
   For difficult problems steepest edge is preferred while for
   very easy (large) problems we will need partial scan.
   
   C) Sounds easy, but this is hardest part of algorithm.
   1) Instead of stopping at first choice, we may be able
   to flip that variable to other bound and if objective
   still improving choose again.  These mini iterations can
   increase speed by orders of magnitude but we may need to
   go to more of a bucket choice of variable rather than looking
   at them one by one (for speed).
   2) Accuracy.  Reduced costs may be of wrong sign but less than
   tolerance.  Pivoting on these makes objective go backwards.
   OSL modified cost so a zero move was made, Gill et al
   (in primal analogue) modified so a strictly positive move was
   made.  It is not quite as neat in dual but that is what we
   try and do.  The two problems are that re-factorizations can
   change reduced costs above and below tolerances and that when
   finished we need to reset costs and try again.
   3) Degeneracy.  Gill et al helps but may not be enough.  We
   may need more.  Also it can improve speed a lot if we perturb
   the costs significantly.
   
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
#include "AbcSimplexDual.hpp"
#include "ClpEventHandler.hpp"
#include "AbcSimplexFactorization.hpp"
#include "CoinPackedMatrix.hpp"
#include "CoinIndexedVector.hpp"
#include "CoinFloatEqual.hpp"
#include "AbcDualRowDantzig.hpp"
#include "AbcDualRowSteepest.hpp"
#include "ClpMessage.hpp"
#include "ClpLinearObjective.hpp"
class ClpSimplex;
#include <cfloat>
#include <cassert>
#include <string>
#include <stdio.h>
#include <iostream>
//#define CLP_DEBUG 1
#ifdef NDEBUG
#define NDEBUG_CLP
#endif
#ifndef CLP_INVESTIGATE
#define NDEBUG_CLP
#endif
// dual

/* *** Method
   This is a vanilla version of dual simplex.
   
   It tries to be a single phase approach with a weight of 1.0 being
   given to getting optimal and a weight of dualBound_ being
   given to getting dual feasible.  In this version I have used the
   idea that this weight can be thought of as a fake bound.  If the
   distance between the lower and upper bounds on a variable is less
   than the feasibility weight then we are always better off flipping
   to other bound to make dual feasible.  If the distance is greater
   then we make up a fake bound dualBound_ away from one bound.
   If we end up optimal or primal infeasible, we check to see if
   bounds okay.  If so we have finished, if not we increase dualBound_
   and continue (after checking if unbounded). I am undecided about
   free variables - there is coding but I am not sure about it.  At
   present I put them in basis anyway.
   
   The code is designed to take advantage of sparsity so arrays are
   seldom zeroed out from scratch or gone over in their entirety.
   The only exception is a full scan to find outgoing variable.  This
   will be changed to keep an updated list of infeasibilities (or squares
   if steepest edge).  Also on easy problems we don't need full scan - just
   pick first reasonable.
   
   One problem is how to tackle degeneracy and accuracy.  At present
   I am using the modification of costs which I put in OSL and which was
   extended by Gill et al.  I am still not sure of the exact details.
   
   The flow of dual is three while loops as follows:
   
   while (not finished) {
   
   while (not clean solution) {
   
   Factorize and/or clean up solution by flipping variables so
   dual feasible.  If looks finished check fake dual bounds.
   Repeat until status is iterating (-1) or finished (0,1,2)
   
   }
   
   while (status==-1) {
   
   Iterate until no pivot in or out or time to re-factorize.
   
   Flow is:
   
   choose pivot row (outgoing variable).  if none then
   we are primal feasible so looks as if done but we need to
   break and check bounds etc.
   
   Get pivot row in tableau
   
   Choose incoming column.  If we don't find one then we look
   primal infeasible so break and check bounds etc.  (Also the
   pivot tolerance is larger after any iterations so that may be
   reason)
   
   If we do find incoming column, we may have to adjust costs to
   keep going forwards (anti-degeneracy).  Check pivot will be stable
   and if unstable throw away iteration (we will need to implement
   flagging of basic variables sometime) and break to re-factorize.
   If minor error re-factorize after iteration.
   
   Update everything (this may involve flipping variables to stay
   dual feasible.
   
   }
   
   }
   
   At present we never check we are going forwards.  I overdid that in
   OSL so will try and make a last resort.
   
   Needs partial scan pivot out option.
   Needs dantzig, uninitialized and full steepest edge options (can still
   use partial scan)
   
   May need other anti-degeneracy measures, especially if we try and use
   loose tolerances as a way to solve in fewer iterations.
   
   I like idea of dynamic scaling.  This gives opportunity to decouple
   different implications of scaling for accuracy, iteration count and
   feasibility tolerance.
   
*/
#define CLEAN_FIXED 0
// Startup part of dual (may be extended to other algorithms)
// To force to follow another run put logfile name here and define
//#define FORCE_FOLLOW
#ifdef FORCE_FOLLOW
static FILE * fpFollow = NULL;
static const char * forceFile = NULL;
static int force_in = -1;
static int force_out = -1;
static int force_way_in = -1;
static int force_way_out = -1;
static int force_iterations = 0;
int force_argc=0;
const char ** force_argv=NULL;
#endif
void
AbcSimplexDual::startupSolve()
{
#ifdef FORCE_FOLLOW
  int ifld;
  for (ifld=1;ifld<force_argc;ifld++) {
    if (strstr(argv[ifld],".log")) {
      forceFile=argv[ifld];
      break;
    }
  }
  if (!fpFollow && strlen(forceFile)) {
    fpFollow = fopen(forceFile, "r");
    assert (fpFollow);
  }
  if (fpFollow) {
    int numberRead= atoi(argv[ifld+1]);
    force_iterations=atoi(argv[ifld+1]);
    printf("skipping %d iterations and then doing %d\n",numberRead,force_iterations);
    for (int iteration=0;iteration<numberRead;iteration++) {
      // read to next Clp0102
      char temp[300];
      while (fgets(temp, 250, fpFollow)) {
	if (!strncmp(temp, "dirOut", 6))
	  break;
      }
      sscanf(temp+7 , "%d%*s%d%*s%*s%d",
	     &force_way_out, &force_way_in);
      fgets(temp, 250, fpFollow);
      char cin, cout;
      int whichIteration;
      sscanf(temp , "%d%*f%*s%*c%c%d%*s%*c%c%d",
	     &whichIteration, &cin, &force_in, &cout, &force_out);
      assert (whichIterations==iteration+1);
      if (cin == 'C')
	force_in += numberColumns_;
      if (cout == 'C')
	force_out += numberColumns_;
      printf("Iteration %d will force %d out (way %d) and %d in (way %d)\n",
	     whichIteration, force_out, force_way_out,force_in,force_way_in);
      assert (getInternalStatus(force_out)==basic);
      assert (getInternalStatus(force_in)!=basic);
      setInternalStatus(force_in,basic);
      if (force_way_out==-1)
	setInternalStatus(force_out,atUpperBound);
      else
	setInternalStatus(force_out,atLowerBound);
    }
    // clean up
    int numberBasic=0;
    for (int i=0;i<numberTotal_;i++) {
      if (getInternalStatus(i)==basic) {
	abcPivotVariable_[numberBasic++]=i;
      } else if (getInternalStatus(i)==atLowerBound) {
	if (abcLower_[i]<-1.0e30) {
	  abcLower_[i]=abcUpper_[i]-dualBound_;
	  setFakeLower(i);
	}
      } else if (getInternalStatus(i)==atUpperBound) {
	if (abcUpper_[i]>1.0e30) {
	  abcUpper_[i]=abcLower_[i]+dualBound_;
	  setFakeUpper(i);
	}
      }
    }
    assert (numberBasic==numberRows_);
  }
#endif
  // initialize - no values pass and algorithm_ is -1
  // put in standard form (and make row copy)
  // create modifiable copies of model rim and do optional scaling
  // If problem looks okay
  // Do initial factorization
  numberFake_ = 0; // Number of variables at fake bounds
  numberChanged_ = 0; // Number of variables with changed costs
  /// Initial coding here
  statusOfProblemInDual(0);
}
void
AbcSimplexDual::finishSolve()
{
  assert (problemStatus_ || !sumPrimalInfeasibilities_);
}
void
AbcSimplexDual::gutsOfDual()
{
  double largestPrimalError = 0.0;
  double largestDualError = 0.0;
  lastCleaned_ = 0; // last time objective or bounds cleaned up
  numberDisasters_=0;
  
  // This says whether to restore things etc
  //startupSolve();
  // startup will have factorized so can skip
  // Start check for cycles
  abcProgress_.startCheck();
  // Say change made on first iteration
  changeMade_ = 1;
  // Say last objective infinite
  //lastObjectiveValue_=-COIN_DBL_MAX;
  if ((stateOfProblem_&VALUES_PASS)!=0) {
    // modify costs so nonbasic dual feasible with fake
    for (int iSequence = 0; iSequence < numberTotal_; iSequence++) {
      switch(getInternalStatus(iSequence)) {
      case basic:
      case AbcSimplex::isFixed:
	break;
      case isFree:
      case superBasic:
	//abcCost_[iSequence]-=djSaved_[iSequence];
	//abcDj_[iSequence]-=djSaved_[iSequence];
	djSaved_[iSequence]=0.0;
	break;
      case atLowerBound:
	if (djSaved_[iSequence]<0.0) {
	  //abcCost_[iSequence]-=djSaved_[iSequence];
	  //abcDj_[iSequence]-=djSaved_[iSequence];
	  djSaved_[iSequence]=0.0;
	}
	break;
      case atUpperBound:
	if (djSaved_[iSequence]>0.0) {
	  //abcCost_[iSequence]-=djSaved_[iSequence];
	  //abcDj_[iSequence]-=djSaved_[iSequence];
	  djSaved_[iSequence]=0.0;
	}
	break;
      }
    }
  }
  progressFlag_ = 0;
  /*
    Status of problem:
    0 - optimal
    1 - infeasible
    2 - unbounded
    -1 - iterating
    -2 - factorization wanted
    -3 - redo checking without factorization
    -4 - looks infeasible
  */
  int factorizationType=1;
  double pivotTolerance=abcFactorization_->pivotTolerance();
  while (problemStatus_ < 0) {
    // If getting nowhere - return
    double limit = numberRows_ + numberColumns_;
    limit = 10000.0 +2000.0 * limit;
#if ABC_NORMAL_DEBUG>0
    if (numberIterations_ > limit) {
      printf("Set flag so ClpSimplexDual done\n");
      abort();
    }
#endif
    bool disaster = false;
    if (disasterArea_ && disasterArea_->check()) {
      disasterArea_->saveInfo();
      disaster = true;
    }
    if (numberIterations_>baseIteration_) {
      // may factorize, checks if problem finished
      statusOfProblemInDual(factorizationType);
      factorizationType=1;
      largestPrimalError = CoinMax(largestPrimalError, largestPrimalError_);
      largestDualError = CoinMax(largestDualError, largestDualError_);
      if (disaster)
	problemStatus_ = 3;
    }
    int saveNumberIterations=numberIterations_;
    // exit if victory declared
    if (problemStatus_ >= 0)
      break;
    
    // test for maximum iterations
    if (hitMaximumIterations()) {
      problemStatus_ = 3;
      break;
    }
    // Check event
    int eventStatus = eventHandler_->event(ClpEventHandler::endOfFactorization);
    if (eventStatus >= 0) {
      problemStatus_ = 5;
      secondaryStatus_ = ClpEventHandler::endOfFactorization;
      break;
    }
    if (!numberPrimalInfeasibilities_&&problemStatus_<0) {
#if ABC_NORMAL_DEBUG>0
      printf("Primal feasible - sum dual %g - go to primal\n",sumDualInfeasibilities_);
#endif
      problemStatus_=10;
      break;
    }
    // Do iterations ? loop round
#if PARTITION_ROW_COPY==1
    stateOfProblem_ |= NEED_BASIS_SORT;
#endif
    while (true) {
#if PARTITION_ROW_COPY==1
      if ((stateOfProblem_&NEED_BASIS_SORT)!=0) {
	int iVector=getAvailableArray();
	abcMatrix_->sortUseful(usefulArray_[iVector]);
	setAvailableArray(iVector);
	stateOfProblem_ &= ~NEED_BASIS_SORT;
      }
#endif
      problemStatus_=-1;
      if ((stateOfProblem_&VALUES_PASS)!=0) {
	assert (djSaved_-abcDj_==numberTotal_);
	abcDj_=djSaved_;
	djSaved_=NULL;
      }
#if ABC_PARALLEL
      if (parallelMode_==0) {
#endif
	whileIteratingSerial();
#if ABC_PARALLEL
      } else {
#if ABC_PARALLEL==1
	whileIteratingThread();
#else
	whileIteratingCilk();
#endif
      }
#endif
      if ((stateOfProblem_&VALUES_PASS)!=0) {
	djSaved_=abcDj_;
	abcDj_=djSaved_-numberTotal_;
      }
      if (pivotRow_!=-100) {
	if (numberIterations_>saveNumberIterations||problemStatus_>=0) {
	  if (problemStatus_==10&&abcFactorization_->pivots()&&numberTimesOptimal_<3) {
	    factorizationType=5;
	    problemStatus_=-4;
	    if (!numberPrimalInfeasibilities_)
	      factorizationType=9;
	  } else if (problemStatus_==-2) {
	    factorizationType=5;
	  }
	  break;
	} else if (abcFactorization_->pivots()||problemStatus_==-6||
		   pivotTolerance<abcFactorization_->pivotTolerance()) {
	  factorizationType=5;
	  if (pivotTolerance<abcFactorization_->pivotTolerance()) {
	    pivotTolerance=abcFactorization_->pivotTolerance();
	    if (!abcFactorization_->pivots())
	      abcFactorization_->minimumPivotTolerance(CoinMin(0.25,1.05*abcFactorization_->minimumPivotTolerance()));
	  }
	  break;
	} else {
	  bool tryFake=numberAtFakeBound()>0&&currentDualBound_<1.0e17;
	  if (problemStatus_==-5||tryFake) {
	    if (numberTimesOptimal_>10)
	      problemStatus_=4;
	    numberTimesOptimal_++;
	    int numberChanged=-1;
	    if (numberFlagged_) {
	      numberFlagged_=0;
	      for (int iRow = 0; iRow < numberRows_; iRow++) {
		int iPivot = abcPivotVariable_[iRow];
		if (flagged(iPivot)) {
		  clearFlagged(iPivot);
		}
	      }
	      numberChanged=0;
	    } else if (tryFake) {
	      double dummyChange;
	      numberChanged = changeBounds(0,dummyChange);
	      if (numberChanged>0) {
		handler_->message(CLP_DUAL_ORIGINAL, messages_)
		  << CoinMessageEol;
		bounceTolerances(3 /*(numberChanged>0) ? 3 : 1*/);
		// temp
#if ABC_NORMAL_DEBUG>0
		//printf("should not need to break out of loop\n");
#endif
		abcProgress_.reset();
		break;
	      } else if (numberChanged<-1) {
		// some flipped
		bounceTolerances(3);
		abcProgress_.reset();
	      }
	    } else if (numberChanged<0) {
	      // what now
	      printf("No iterations since last inverts A - nothing done - think\n");
	      abort();
	    }
	  } else if (problemStatus_!=-4) {
	    // what now
	    printf("No iterations since last inverts B - nothing done - think\n");
	    abort();
	  }
	}
      } else {
	// end of values pass 
	// we need to tell statusOf.. to restore costs etc
	// and take off flag
	bounceTolerances(2);
	assert (!solution_);
	delete [] solution_;
	solution_=NULL;
	gutsOfSolution(3);
	makeNonFreeVariablesDualFeasible();
	stateOfProblem_ &= ~VALUES_PASS;
	problemStatus_=-2;
	factorizationType=5;
	break;
      }
    }
    if (problemStatus_ == 1 && (progressFlag_&8) != 0 &&
	fabs(rawObjectiveValue_) > 1.0e10 ) {
#if ABC_NORMAL_DEBUG>0
      printf("fix looks infeasible when has been\n"); // goto primal
#endif
      problemStatus_ = 10; // infeasible - but has looked feasible
    }
    if (!problemStatus_ && abcFactorization_->pivots()) {
      gutsOfSolution(1); // need to compute duals
      //computeInternalObjectiveValue();
      checkCutoff(true);
    }
  }
  largestPrimalError_ = largestPrimalError;
  largestDualError_ = largestDualError;
}
/// The duals are updated by the given arrays.
static void updateDualsInDualBit2(CoinPartitionedVector & row,
				  double * COIN_RESTRICT djs,
				  double theta, int iBlock)
{
  int offset=row.startPartition(iBlock);
  double * COIN_RESTRICT work=row.denseVector()+offset;
  const int * COIN_RESTRICT which=row.getIndices()+offset;
  int number=row.getNumElements(iBlock);
  for (int i = 0; i < number; i++) {
    int iSequence = which[i];
    double alphaI = work[i];
    work[i] = 0.0;
    double value = djs[iSequence] - theta * alphaI;
    djs[iSequence] = value;
  }
  row.setNumElementsPartition(iBlock,0);
}
static void updateDualsInDualBit(CoinPartitionedVector & row,
				 double * djs,
				 double theta, int numberBlocks)
{
  for (int iBlock=1;iBlock<numberBlocks;iBlock++) {
    cilk_spawn updateDualsInDualBit2(row,djs,theta,iBlock);
  }
  updateDualsInDualBit2(row,djs,theta,0);
  cilk_sync;
}
/// The duals are updated by the given arrays.
   
void
AbcSimplexDual::updateDualsInDual()
{
  //dual->usefulArray(3)->compact();
  CoinPartitionedVector & array=usefulArray_[arrayForTableauRow_];
  //array.compact();
  int numberBlocks=array.getNumPartitions();
  int number=array.getNumElements();
  if(numberBlocks) {
    if (number>100) {
      updateDualsInDualBit(array,abcDj_,theta_,numberBlocks);
    } else {
      for (int iBlock=0;iBlock<numberBlocks;iBlock++) {
	updateDualsInDualBit2(array,abcDj_,theta_,iBlock);
      }
    }
    array.compact();
  } else {
    double * COIN_RESTRICT work=array.denseVector();
    const int * COIN_RESTRICT which=array.getIndices();
    double * COIN_RESTRICT reducedCost=abcDj_;
#pragma cilk_grainsize=128
    cilk_for (int i = 0; i < number; i++) {
      //for (int i = 0; i < number; i++) {
      int iSequence = which[i];
      double alphaI = work[i];
      work[i] = 0.0;
      double value = reducedCost[iSequence] - theta_ * alphaI;
      reducedCost[iSequence] = value;
    }
    array.setNumElements(0);
  }
}
int
AbcSimplexDual::nextSuperBasic()
{
  if (firstFree_ >= 0) {
    // make sure valid
    int returnValue = firstFree_;
    int iColumn = firstFree_ + 1;
#ifdef PAN
    if (fakeSuperBasic(firstFree_)<0) {
      // somehow cleaned up
      returnValue=-1;
      for (; iColumn < numberTotal_; iColumn++) {
	if (getInternalStatus(iColumn) == isFree||
	    getInternalStatus(iColumn) == superBasic) {
	  if (fakeSuperBasic(iColumn)>=0) {
	    if (fabs(abcDj_[iColumn]) > 1.0e2 * dualTolerance_)
	      break;
	  }
	}
      }
      if (iColumn<numberTotal_) {
	returnValue=iColumn;
	iColumn++;
      }
    }
#endif
    for (; iColumn < numberTotal_; iColumn++) {
      if (getInternalStatus(iColumn) == isFree||
	  getInternalStatus(iColumn) == superBasic) {
#ifdef PAN
	if (fakeSuperBasic(iColumn)>=0) {
#endif
	  if (fabs(abcDj_[iColumn]) > dualTolerance_)
	    break;
#ifdef PAN
	}
#endif
      }
    }
    firstFree_ = iColumn;
    if (firstFree_ == numberTotal_)
      firstFree_ = -1;
    return returnValue;
  } else {
    return -1;
  }
}
/*
  Chooses dual pivot row
  For easy problems we can just choose one of the first rows we look at
*/
void
AbcSimplexDual::dualPivotRow()
{
  //int lastPivotRow=pivotRow_;
  pivotRow_=-1;
  const double * COIN_RESTRICT lowerBasic = lowerBasic_;
  const double * COIN_RESTRICT upperBasic = upperBasic_;
  const double * COIN_RESTRICT solutionBasic = solutionBasic_;
  const int * COIN_RESTRICT pivotVariable=abcPivotVariable_;
  freeSequenceIn_=-1;
  double worstValuesPass=0.0;
  if ((stateOfProblem_&VALUES_PASS)!=0) {
    for (int iSequence = 0; iSequence < numberTotal_; iSequence++) {
      switch(getInternalStatus(iSequence)) {
      case basic:
#if ABC_NORMAL_DEBUG>0
	if (fabs(abcDj_[iSequence])>1.0e-6)
	  printf("valuespass basic dj %d status %d dj %g\n",iSequence,
		 getInternalStatus(iSequence),abcDj_[iSequence]);
#endif
	break;
      case AbcSimplex::isFixed:
	break;
      case isFree:
      case superBasic:
#if ABC_NORMAL_DEBUG>0
	if (fabs(abcDj_[iSequence])>1.0e-6)
	  printf("Bad valuespass dj %d status %d dj %g\n",iSequence,
		 getInternalStatus(iSequence),abcDj_[iSequence]);
#endif
	break;
      case atLowerBound:
#if ABC_NORMAL_DEBUG>0
	if (abcDj_[iSequence]<-1.0e-6)
	  printf("Bad valuespass dj %d status %d dj %g\n",iSequence,
		 getInternalStatus(iSequence),abcDj_[iSequence]);
#endif
	break;
      case atUpperBound:
#if ABC_NORMAL_DEBUG>0
	if (abcDj_[iSequence]>1.0e-6)
	  printf("Bad valuespass dj %d status %d dj %g\n",iSequence,
		 getInternalStatus(iSequence),abcDj_[iSequence]);
#endif
	break;
      }
    }
    // get worst basic dual - later do faster using steepest infeasibility array
    // does not have to be worst - just bad
    int iWorst=-1;
    double worst=dualTolerance_;
    for (int i=0;i<numberRows_;i++) {
      int iSequence=abcPivotVariable_[i];
      if (fabs(abcDj_[iSequence])>worst) {
	iWorst=i;
	worst=fabs(abcDj_[iSequence]);
      }
    }
    if (iWorst>=0) {
      pivotRow_=iWorst;
      worstValuesPass=abcDj_[abcPivotVariable_[iWorst]];
    } else {
      // factorize and clean up costs
      pivotRow_=-100;
      return;
    }
  }
  if (false&&numberFreeNonBasic_>abcFactorization_->maximumPivots()) {
    lastFirstFree_=-1;
    firstFree_=-1;
  }
  if (firstFree_>=0) {
    // free
    int nextFree = nextSuperBasic();
    if (nextFree>=0) {
      freeSequenceIn_=nextFree;
      // **** should skip price (and maybe weights update)
      // unpack vector and find a good pivot
      unpack(usefulArray_[arrayForBtran_], nextFree);
      abcFactorization_->updateColumn(usefulArray_[arrayForBtran_]);
      
      double *  COIN_RESTRICT work = usefulArray_[arrayForBtran_].denseVector();
      int number = usefulArray_[arrayForBtran_].getNumElements();
      int *  COIN_RESTRICT which = usefulArray_[arrayForBtran_].getIndices();
      double bestFeasibleAlpha = 0.0;
      int bestFeasibleRow = -1;
      double bestInfeasibleAlpha = 0.0;
      int bestInfeasibleRow = -1;
      // Go for big infeasibilities unless big changes
      double maxInfeas = (fabs(dualOut_)>1.0e3) ? 0.001 : 1.0e6;
      for (int i = 0; i < number; i++) {
	int iRow = which[i];
	double alpha = fabs(work[iRow]);
	if (alpha > 1.0e-3) {
	  double value = solutionBasic[iRow];
	  double lower = lowerBasic[iRow];
	  double upper = upperBasic[iRow];
	  double infeasibility = 0.0;
	  if (value > upper)
	    infeasibility = value - upper;
	  else if (value < lower)
	    infeasibility = lower - value;
	  infeasibility=CoinMin(maxInfeas,infeasibility);
	  if (infeasibility * alpha > bestInfeasibleAlpha && alpha > 1.0e-1) {
	    if (!flagged(pivotVariable[iRow])) {
	      bestInfeasibleAlpha = infeasibility * alpha;
	      bestInfeasibleRow = iRow;
	    }
	  }
	  if (alpha > bestFeasibleAlpha && (lower > -1.0e20 || upper < 1.0e20)) {
	    bestFeasibleAlpha = alpha;
	    bestFeasibleRow = iRow;
	  }
	}
      }
      if (bestInfeasibleRow >= 0)
	pivotRow_ = bestInfeasibleRow;
      else if (bestFeasibleAlpha > 1.0e-2)
	pivotRow_ = bestFeasibleRow;
      usefulArray_[arrayForBtran_].clear();
      if (pivotRow_<0) {
 	// no good
 	freeSequenceIn_=-1;
 	if (!abcFactorization_->pivots()) {
 	  lastFirstFree_=-1;
 	  firstFree_=-1;
 	}
      }
    }
  }
  if (pivotRow_<0) {
    // put back so can test
    //pivotRow_=lastPivotRow;
    // get pivot row using whichever method it is
    pivotRow_ = abcDualRowPivot_->pivotRow();
  } 
  
  if (pivotRow_ >= 0) {
    sequenceOut_ = pivotVariable[pivotRow_];
    valueOut_ = solutionBasic[pivotRow_];
    lowerOut_ = lowerBasic[pivotRow_];
    upperOut_ = upperBasic[pivotRow_];
    // if we have problems we could try other way and hope we get a
    // zero pivot?
    if (valueOut_ > upperOut_) {
      directionOut_ = -1;
      dualOut_ = valueOut_ - upperOut_;
    } else if (valueOut_ < lowerOut_) {
      directionOut_ = 1;
      dualOut_ = lowerOut_ - valueOut_;
    } else {
      // odd (could be free) - it's feasible - go to nearest
      if (valueOut_ - lowerOut_ < upperOut_ - valueOut_) {
	directionOut_ = 1;
	dualOut_ = lowerOut_ - valueOut_;
      } else {
	directionOut_ = -1;
	dualOut_ = valueOut_ - upperOut_;
      }
    }
    if (worstValuesPass) {
      // in values pass so just use sign of dj
      // We don't want to go through any barriers so set dualOut low
      // free variables will never be here
      // need to think about tolerances with inexact solutions
      // could try crashing in variables away from bounds and with near zero djs
      // pivot to take out bad slack and increase objective
      double fakeValueOut=solution_[sequenceOut_];
      dualOut_ = 1.0e-10;
      if (upperOut_>lowerOut_||
	  fabs(fakeValueOut-upperOut_)<100.0*primalTolerance_) {
	if (abcDj_[sequenceOut_] > 0.0) {
	  directionOut_ = 1;
	} else {
	  directionOut_ = -1;
	}
      } else {
	// foolishly trust primal solution
	if (fakeValueOut > upperOut_) {
	  directionOut_ = -1;
	} else {
	  directionOut_ = 1;
	}
      }
#if 1
      // make sure plausible but only when fake primals stored
      if (directionOut_ < 0 && fabs(fakeValueOut - upperOut_) > dualBound_ + primalTolerance_) {
	if (fabs(fakeValueOut - upperOut_) > fabs(fakeValueOut - lowerOut_))
	  directionOut_ = 1;
      } else if (directionOut_ > 0 && fabs(fakeValueOut - lowerOut_) > dualBound_ + primalTolerance_) {
	if (fabs(fakeValueOut - upperOut_) < fabs(fakeValueOut - lowerOut_))
	  directionOut_ = -1;
      }
#endif
    }
#if ABC_NORMAL_DEBUG>3
    assert(dualOut_ >= 0.0);
#endif
  } else {
    sequenceOut_=-1;
  }
  alpha_=0.0;
  upperTheta_=COIN_DBL_MAX;
  return ;
}
// Checks if any fake bounds active - if so returns number and modifies
// dualBound_ and everything.
// Free variables will be left as free
// Returns number of bounds changed if >=0
// Returns -1 if not initialize and no effect
// Fills in changeVector which can be used to see if unbounded
// initialize is 0 or 3
// and cost of change vector
int
AbcSimplexDual::changeBounds(int initialize,
                             double & changeCost)
{
  assert (initialize==0||initialize==1||initialize==3||initialize==4);
  numberFake_ = 0;
  double *  COIN_RESTRICT abcLower = abcLower_;
  double *  COIN_RESTRICT abcUpper = abcUpper_;
  double *  COIN_RESTRICT abcSolution = abcSolution_;
  double *  COIN_RESTRICT abcCost = abcCost_;
  if (!initialize) {
    int numberInfeasibilities;
    double newBound;
    newBound = 5.0 * currentDualBound_;
    numberInfeasibilities = 0;
    changeCost = 0.0;
    // put back original bounds and then check
    CoinAbcMemcpy(abcLower,lowerSaved_,numberTotal_);
    CoinAbcMemcpy(abcUpper,upperSaved_,numberTotal_);
    const double * COIN_RESTRICT dj = abcDj_;
    int iSequence;
    // bounds will get bigger - just look at ones at bounds
    int numberFlipped=0;
    double checkBound=1.000000000001*currentDualBound_;
    for (iSequence = 0; iSequence < numberTotal_; iSequence++) {
      double lowerValue = abcLower[iSequence];
      double upperValue = abcUpper[iSequence];
      double value = abcSolution[iSequence];
      setFakeBound(iSequence, AbcSimplexDual::noFake);
      switch(getInternalStatus(iSequence)) {
	
      case basic:
      case AbcSimplex::isFixed:
	break;
      case isFree:
      case superBasic:
	break;
      case atUpperBound:
	if (fabs(value - upperValue) > primalTolerance_) {
	  // can we flip
	  if (value-lowerValue<checkBound&&dj[iSequence]>-currentDualTolerance_) {
	    abcSolution_[iSequence]=lowerValue;
	    setInternalStatus(iSequence,atLowerBound);
	    numberFlipped++;
	  } else {
	    numberInfeasibilities++;
	  }
	}
	break;
      case atLowerBound:
	if (fabs(value - lowerValue) > primalTolerance_) {
	  // can we flip
	  if (upperValue-value<checkBound&&dj[iSequence]<currentDualTolerance_) {
	    abcSolution_[iSequence]=upperValue;
	    setInternalStatus(iSequence,atUpperBound);
	    numberFlipped++;
	  } else {
	    numberInfeasibilities++;
	  }
	}
	break;
      }
    }
    // If dual infeasible then carry on
    if (numberInfeasibilities) {
      handler_->message(CLP_DUAL_CHECKB, messages_)
	<< newBound
	<< CoinMessageEol;
      int iSequence;
      for (iSequence = 0; iSequence < numberRows_ + numberColumns_; iSequence++) {
	double lowerValue = abcLower[iSequence];
	double upperValue = abcUpper[iSequence];
	double newLowerValue;
	double newUpperValue;
	Status status = getInternalStatus(iSequence);
	if (status == atUpperBound ||
	    status == atLowerBound) {
	  double value = abcSolution[iSequence];
	  if (value - lowerValue <= upperValue - value) {
	    newLowerValue = CoinMax(lowerValue, value - 0.666667 * newBound);
	    newUpperValue = CoinMin(upperValue, newLowerValue + newBound);
	  } else {
	    newUpperValue = CoinMin(upperValue, value + 0.666667 * newBound);
	    newLowerValue = CoinMax(lowerValue, newUpperValue - newBound);
	  }
	  abcLower[iSequence] = newLowerValue;
	  abcUpper[iSequence] = newUpperValue;
	  if (newLowerValue > lowerValue) {
	    if (newUpperValue < upperValue) {
	      setFakeBound(iSequence, AbcSimplexDual::bothFake);
#ifdef CLP_INVESTIGATE
	      abort(); // No idea what should happen here - I have never got here
#endif
	      numberFake_++;
	    } else {
	      setFakeBound(iSequence, AbcSimplexDual::lowerFake);
	      assert (abcLower[iSequence]>lowerSaved_[iSequence]);
	      assert (abcUpper[iSequence]==upperSaved_[iSequence]);
	      numberFake_++;
	    }
	  } else {
	    if (newUpperValue < upperValue) {
	      setFakeBound(iSequence, AbcSimplexDual::upperFake);
	      assert (abcLower[iSequence]==lowerSaved_[iSequence]);
	      assert (abcUpper[iSequence]<upperSaved_[iSequence]);
	      numberFake_++;
	    }
	  }
	  if (status == atUpperBound)
	    abcSolution[iSequence] = newUpperValue;
	  else
	    abcSolution[iSequence] = newLowerValue;
	  double movement = abcSolution[iSequence] - value;
	  changeCost += movement * abcCost[iSequence];
	}
      }
      currentDualBound_ = newBound;
#if ABC_NORMAL_DEBUG>0
      printf("new dual bound %g\n",currentDualBound_);
#endif
    } else {
      numberInfeasibilities = -1-numberFlipped;
    }
    return numberInfeasibilities;
  } else {
    int iSequence;
    if (initialize == 3) {
      for (iSequence = 0; iSequence < numberTotal_; iSequence++) {
#if ABC_NORMAL_DEBUG>1
	int bad=-1;
	if (getFakeBound(iSequence)==noFake) {
	  if (abcLower_[iSequence]!=lowerSaved_[iSequence]||
	      abcUpper_[iSequence]!=upperSaved_[iSequence]) 
	    bad=0;
	} else if (getFakeBound(iSequence)==lowerFake) {
	  if (abcLower_[iSequence]==lowerSaved_[iSequence]||
	      abcUpper_[iSequence]!=upperSaved_[iSequence]) 
	    bad=1;
	} else if (getFakeBound(iSequence)==upperFake) {
	  if (abcLower_[iSequence]!=lowerSaved_[iSequence]||
	      abcUpper_[iSequence]==upperSaved_[iSequence]) 
	    bad=2;
	} else {
	  if (abcLower_[iSequence]==lowerSaved_[iSequence]||
	      abcUpper_[iSequence]==upperSaved_[iSequence]) 
	    bad=3;
	}
	if (bad>=0) 
	  printf("%d status %d l %g,%g u %g,%g\n",iSequence,internalStatus_[iSequence],
		 abcLower_[iSequence],lowerSaved_[iSequence],
		 abcUpper_[iSequence],upperSaved_[iSequence]);
#endif
	setFakeBound(iSequence, AbcSimplexDual::noFake);
      }
      CoinAbcMemcpy(abcLower_,lowerSaved_,numberTotal_);
      CoinAbcMemcpy(abcUpper_,upperSaved_,numberTotal_);
    }
    double testBound = /*0.999999 **/ currentDualBound_;
    for (iSequence = 0; iSequence < numberTotal_; iSequence++) {
      Status status = getInternalStatus(iSequence);
      if (status == atUpperBound ||
	  status == atLowerBound) {
	double lowerValue = abcLower[iSequence];
	double upperValue = abcUpper[iSequence];
	double value = abcSolution[iSequence];
	if (lowerValue > -largeValue_ || upperValue < largeValue_) {
	  if (true || lowerValue - value > -0.5 * currentDualBound_ ||
	      upperValue - value < 0.5 * currentDualBound_) {
	    if (fabs(lowerValue - value) <= fabs(upperValue - value)) {
	      if (upperValue > lowerValue + testBound) {
		if (getFakeBound(iSequence) == AbcSimplexDual::noFake)
		  numberFake_++;
		abcUpper[iSequence] = lowerValue + currentDualBound_;
		double difference = upperSaved_[iSequence]-abcUpper[iSequence];
		if (difference<1.0e-10*(1.0+fabs(upperSaved_[iSequence])+dualBound_))
		  abcUpper[iSequence] = upperSaved_[iSequence];
		assert (abcUpper[iSequence]<=upperSaved_[iSequence]+1.0e-3*currentDualBound_);
		assert (abcLower[iSequence]>=lowerSaved_[iSequence]-1.0e-3*currentDualBound_);
		if (upperSaved_[iSequence]-abcUpper[iSequence]>
		    abcLower[iSequence]-lowerSaved_[iSequence]) {
		  setFakeBound(iSequence, AbcSimplexDual::upperFake);
		  assert (abcLower[iSequence]==lowerSaved_[iSequence]);
		  assert (abcUpper[iSequence]<upperSaved_[iSequence]);
		} else {
		  setFakeBound(iSequence, AbcSimplexDual::lowerFake);
		  assert (abcLower[iSequence]>lowerSaved_[iSequence]);
		  assert (abcUpper[iSequence]==upperSaved_[iSequence]);
		}
	      }
	    } else {
	      if (lowerValue < upperValue - testBound) {
		if (getFakeBound(iSequence) == AbcSimplexDual::noFake)
		  numberFake_++;
		abcLower[iSequence] = upperValue - currentDualBound_;
		double difference = abcLower[iSequence]-lowerSaved_[iSequence];
		if (difference<1.0e-10*(1.0+fabs(lowerSaved_[iSequence])+dualBound_))
		  abcLower[iSequence] = lowerSaved_[iSequence];
		assert (abcUpper[iSequence]<=upperSaved_[iSequence]+1.0e-3*currentDualBound_);
		assert (abcLower[iSequence]>=lowerSaved_[iSequence]-1.0e-3*currentDualBound_);
		if (upperSaved_[iSequence]-abcUpper[iSequence]>
		    abcLower[iSequence]-lowerSaved_[iSequence]) {
		  setFakeBound(iSequence, AbcSimplexDual::upperFake);
		  //assert (abcLower[iSequence]==lowerSaved_[iSequence]);
		  abcLower[iSequence]=CoinMax(abcLower[iSequence],lowerSaved_[iSequence]);
		  assert (abcUpper[iSequence]<upperSaved_[iSequence]);
		} else {
		  setFakeBound(iSequence, AbcSimplexDual::lowerFake);
		  assert (abcLower[iSequence]>lowerSaved_[iSequence]);
		  //assert (abcUpper[iSequence]==upperSaved_[iSequence]);
		  abcUpper[iSequence]=CoinMin(abcUpper[iSequence],upperSaved_[iSequence]);
		}
	      }
	    }
	  } else {
	    if (getFakeBound(iSequence) == AbcSimplexDual::noFake)
	      numberFake_++;
	    abcLower[iSequence] = -0.5 * currentDualBound_;
	    abcUpper[iSequence] = 0.5 * currentDualBound_;
	    setFakeBound(iSequence, AbcSimplexDual::bothFake);
	    abort();
	  }
	  if (status == atUpperBound)
	    abcSolution[iSequence] = abcUpper[iSequence];
	  else
	    abcSolution[iSequence] = abcLower[iSequence];
	} else {
	  // set non basic free variables to fake bounds
	  // I don't think we should ever get here
	  CoinAssert(!("should not be here"));
	  abcLower[iSequence] = -0.5 * currentDualBound_;
	  abcUpper[iSequence] = 0.5 * currentDualBound_;
	  setFakeBound(iSequence, AbcSimplexDual::bothFake);
	  numberFake_++;
	  setInternalStatus(iSequence, atUpperBound);
	  abcSolution[iSequence] = 0.5 * currentDualBound_;
	}
      } else if (status == basic) {
	// make sure not at fake bound and bounds correct
	setFakeBound(iSequence, AbcSimplexDual::noFake);
	double gap = abcUpper[iSequence] - abcLower[iSequence];
	if (gap > 0.5 * currentDualBound_ && gap < 2.0 * currentDualBound_) {
	  abcLower[iSequence] = lowerSaved_[iSequence];;
	  abcUpper[iSequence] = upperSaved_[iSequence];;
	}
      }
    }
    return 0;
  }
}
/*
  order of variables is a) atLo/atUp
                        b) free/superBasic
			pre pass done in AbcMatrix to get possibles
			dj's will be stored in a parallel array (tempArray) for first pass
			this pass to be done inside AbcMatrix then as now
			use perturbationArray as variable tolerance
  idea is to use large tolerances and then balance and reduce every so often
  this should be in conjunction with dualBound changes
  variables always stay at bounds on accuracy issues - only deliberate flips allowed
  variable tolerance should be 0.8-1.0 desired (at end)
  use going to clpMatrix_ as last resort
 */
/*
  Array has tableau row
  Puts candidates in list
  Returns upper theta (infinity if no pivot) and may set sequenceIn_ if free
*/
void
AbcSimplexDual::dualColumn1(bool doAll)
{
  const CoinIndexedVector & update = usefulArray_[arrayForBtran_];
  CoinPartitionedVector & tableauRow = usefulArray_[arrayForTableauRow_];
  CoinPartitionedVector & candidateList = usefulArray_[arrayForDualColumn_];
  if ((stateOfProblem_&VALUES_PASS)==0)
    upperTheta_=1.0e31;
  else
    upperTheta_=fabs(abcDj_[sequenceOut_])-0.5*dualTolerance_;
  assert (upperTheta_>0.0);
  if (!doAll) 
    upperTheta_ = abcMatrix_->dualColumn1(update,tableauRow,candidateList);
  else
    upperTheta_ = dualColumn1B();
  //tableauRow.compact();
  //candidateList.compact();
  //tableauRow.setPackedMode(true);
  //candidateList.setPackedMode(true);
}
/*
  Array has tableau row
  Puts candidates in list
  Returns upper theta (infinity if no pivot) and may set sequenceIn_ if free
*/
double
AbcSimplexDual::dualColumn1A()
{
  const CoinIndexedVector & update = usefulArray_[arrayForBtran_];
  CoinPartitionedVector & tableauRow = usefulArray_[arrayForTableauRow_];
  CoinPartitionedVector & candidateList = usefulArray_[arrayForDualColumn_];
  int numberRemaining = 0;
  double upperTheta = upperTheta_;
  // pivot elements
  double *  COIN_RESTRICT arrayCandidate=candidateList.denseVector();
  // indices
  int *  COIN_RESTRICT indexCandidate = candidateList.getIndices();
  const double *  COIN_RESTRICT abcDj = abcDj_;
  //const double *  COIN_RESTRICT abcPerturbation = abcPerturbation_;
  const unsigned char *  COIN_RESTRICT internalStatus = internalStatus_;
  const double *  COIN_RESTRICT pi=update.denseVector();
  const int *  COIN_RESTRICT piIndex = update.getIndices();
  int number = update.getNumElements();
  int *  COIN_RESTRICT index = tableauRow.getIndices();
  double *  COIN_RESTRICT array = tableauRow.denseVector();
  assert (!tableauRow.getNumElements());
  int numberNonZero=0;
  // do first pass to get possibles
  double bestPossible = 0.0;
  // We can also see if infeasible or pivoting on free
  double tentativeTheta = 1.0e25; // try with this much smaller as guess
  double acceptablePivot = currentAcceptablePivot_;
  double dualT=-currentDualTolerance_;
  const double multiplier[] = { 1.0, -1.0};
  if (ordinaryVariables_) {
    for (int i=0;i<number;i++) {
      int iRow=piIndex[i];
      unsigned char type=internalStatus[iRow];
      if ((type&4)==0) {
	index[numberNonZero]=iRow;
	double piValue=pi[iRow];
	array[numberNonZero++]=piValue;
	int iStatus = internalStatus[iRow] & 3;
	double mult = multiplier[iStatus];
	double alpha = piValue * mult;
	double oldValue = abcDj[iRow] * mult;
	double value = oldValue - tentativeTheta * alpha;
	if (value < dualT) {
	  bestPossible = CoinMax(bestPossible, alpha);
	  value = oldValue - upperTheta * alpha;
	  if (value < dualT && alpha >= acceptablePivot) {
	    upperTheta = (oldValue - dualT) / alpha;
	  }
	  // add to list
	  arrayCandidate[numberRemaining] = alpha;
	  indexCandidate[numberRemaining++] = iRow;
	}
      }
    }
  } else {
    double badFree = 0.0;
    double freePivot = currentAcceptablePivot_;
    alpha_ = 0.0;
    // We can also see if infeasible or pivoting on free
    for (int i=0;i<number;i++) {
      int iRow=piIndex[i];
      unsigned char type=internalStatus[iRow];
      double piValue=pi[iRow];
      if ((type&4)==0) {
	index[numberNonZero]=iRow;
	array[numberNonZero++]=piValue;
	int iStatus = internalStatus[iRow] & 3;
	double mult = multiplier[iStatus];
	double alpha = piValue * mult;
	double oldValue = abcDj[iRow] * mult;
	double value = oldValue - tentativeTheta * alpha;
	if (value < dualT) {
	  bestPossible = CoinMax(bestPossible, alpha);
	  value = oldValue - upperTheta * alpha;
	  if (value < dualT && alpha >= acceptablePivot) {
	    upperTheta = (oldValue - dualT) / alpha;
	  }
	  // add to list
	  arrayCandidate[numberRemaining] = alpha;
	  indexCandidate[numberRemaining++] = iRow;
	}
      } else if ((type&7)<6) {
	//} else if (getInternalStatus(iRow)==isFree||getInternalStatus(iRow)==superBasic) {
	bool keep;
	index[numberNonZero]=iRow;
	array[numberNonZero++]=piValue;
	bestPossible = CoinMax(bestPossible, fabs(piValue));
	double oldValue = abcDj[iRow];
	// If free has to be very large - should come in via dualRow
	//if (getInternalStatus(iRow+addSequence)==isFree&&fabs(piValue)<1.0e-3)
	//break;
	if (oldValue > currentDualTolerance_) {
	  keep = true;
	} else if (oldValue < -currentDualTolerance_) {
	  keep = true;
	} else {
	  if (fabs(piValue) > CoinMax(10.0 * currentAcceptablePivot_, 1.0e-5)) {
	    keep = true;
	  } else {
	    keep = false;
	    badFree = CoinMax(badFree, fabs(piValue));
	  }
	}
	if (keep) {
	  // free - choose largest
	  if (fabs(piValue) > freePivot) {
	    freePivot = fabs(piValue);
	    sequenceIn_ = iRow;
	    theta_ = oldValue / piValue;
	    alpha_ = piValue;
	  }
	}
      }
    }
  }
  //tableauRow.setNumElements(numberNonZero);
  //candidateList.setNumElements(numberRemaining);
  tableauRow.setNumElementsPartition(0,numberNonZero);
  candidateList.setNumElementsPartition(0,numberRemaining);
 if (!numberRemaining && sequenceIn_ < 0&&upperTheta_>1.0e29) {
    return COIN_DBL_MAX; // Looks infeasible
  } else {
    return upperTheta;
  }
}
/*
  Array has tableau row
  Puts candidates in list
  Returns upper theta (infinity if no pivot) and may set sequenceIn_ if free
*/
double
AbcSimplexDual::dualColumn1B()
{
  CoinPartitionedVector & tableauRow = usefulArray_[arrayForTableauRow_];
  CoinPartitionedVector & candidateList = usefulArray_[arrayForDualColumn_];
  tableauRow.compact();
  candidateList.clearAndReset();
  tableauRow.setPackedMode(true);
  candidateList.setPackedMode(true);
  int numberRemaining = 0;
  double upperTheta = upperTheta_;
  // pivot elements
  double *  COIN_RESTRICT arrayCandidate=candidateList.denseVector();
  // indices
  int *  COIN_RESTRICT indexCandidate = candidateList.getIndices();
  const double *  COIN_RESTRICT abcDj = abcDj_;
  const unsigned char *  COIN_RESTRICT internalStatus = internalStatus_;
  int *  COIN_RESTRICT index = tableauRow.getIndices();
  double *  COIN_RESTRICT array = tableauRow.denseVector();
  int numberNonZero=tableauRow.getNumElements();
  // do first pass to get possibles
  double bestPossible = 0.0;
  // We can also see if infeasible or pivoting on free
  double tentativeTheta = 1.0e25; // try with this much smaller as guess
  double acceptablePivot = currentAcceptablePivot_;
  double dualT=-currentDualTolerance_;
  const double multiplier[] = { 1.0, -1.0};
  if (ordinaryVariables_) {
    for (int i=0;i<numberNonZero;i++) {
      int iSequence=index[i];
      unsigned char iStatus=internalStatus[iSequence]&7;
      if (iStatus<4) {
	double tableauValue=array[i];
	if (fabs(tableauValue)>zeroTolerance_) {
	  double mult = multiplier[iStatus];
	  double alpha = tableauValue * mult;
	  double oldValue = abcDj[iSequence] * mult;
	  double value = oldValue - tentativeTheta * alpha;
	  if (value < dualT) {
	    bestPossible = CoinMax(bestPossible, alpha);
	    value = oldValue - upperTheta * alpha;
	    if (value < dualT && alpha >= acceptablePivot) {
	      upperTheta = (oldValue - dualT) / alpha;
	    }
	    // add to list
	    arrayCandidate[numberRemaining] = alpha;
	    indexCandidate[numberRemaining++] = iSequence;
	  }
	}
      }
    }
  } else {
    double badFree = 0.0;
    double freePivot = currentAcceptablePivot_;
    double currentDualTolerance = currentDualTolerance_;
    for (int i=0;i<numberNonZero;i++) {
      int iSequence=index[i];
      unsigned char iStatus=internalStatus[iSequence]&7;
      if (iStatus<6) {
	double tableauValue=array[i];
	if (fabs(tableauValue)>zeroTolerance_) {
	  if ((iStatus&4)==0) {
	    double mult = multiplier[iStatus];
	    double alpha = tableauValue * mult;
	    double oldValue = abcDj[iSequence] * mult;
	    double value = oldValue - tentativeTheta * alpha;
	    if (value < dualT) {
	      bestPossible = CoinMax(bestPossible, alpha);
	      value = oldValue - upperTheta * alpha;
	      if (value < dualT && alpha >= acceptablePivot) {
		upperTheta = (oldValue - dualT) / alpha;
	      }
	      // add to list
	      arrayCandidate[numberRemaining] = alpha;
	      indexCandidate[numberRemaining++] = iSequence;
	    }
	  } else {
	    bool keep;
	    bestPossible = CoinMax(bestPossible, fabs(tableauValue));
	    double oldValue = abcDj[iSequence];
	    // If free has to be very large - should come in via dualRow
	    //if (getInternalStatus(iSequence+addSequence)==isFree&&fabs(tableauValue)<1.0e-3)
	    //break;
	    if (oldValue > currentDualTolerance) {
	      keep = true;
	    } else if (oldValue < -currentDualTolerance) {
	      keep = true;
	    } else {
	      if (fabs(tableauValue) > CoinMax(10.0 * acceptablePivot, 1.0e-5)) {
		keep = true;
	      } else {
		keep = false;
		badFree = CoinMax(badFree, fabs(tableauValue));
	      }
	    }
	    if (keep) {
#ifdef PAN
	      if (fakeSuperBasic(iSequence)>=0) {
#endif
		if (iSequence==freeSequenceIn_)
		  tableauValue=COIN_DBL_MAX;
		// free - choose largest
		if (fabs(tableauValue) > fabs(freePivot)) {
		  freePivot = tableauValue;
		  sequenceIn_ = iSequence;
		  theta_ = oldValue / tableauValue;
		  alpha_ = tableauValue;
		}
#ifdef PAN
	      }
#endif
	    }
	  }
	}
      }
    }
  } 
  candidateList.setNumElements(numberRemaining);
  if (!numberRemaining && sequenceIn_ < 0&&upperTheta_>1.0e29) {
    return COIN_DBL_MAX; // Looks infeasible
  } else {
    return upperTheta;
  }
}
//#define MORE_CAREFUL
//#define PRINT_RATIO_PROGRESS
#define MINIMUMTHETA 1.0e-18
#define MAXTRY 100
#define TOL_TYPE -1
#if TOL_TYPE==-1
#define useTolerance1() (0.0)      
#define useTolerance2() (0.0)
#define goToTolerance1() (0.0)
#define goToTolerance2() (0.0)
#elif TOL_TYPE==0
#define useTolerance1() (currentDualTolerance_)      
#define useTolerance2() (currentDualTolerance_)
#define goToTolerance1() (0.0)
#define goToTolerance2() (0.0)
#elif TOL_TYPE==1
#define useTolerance1() (perturbedTolerance)      
#define useTolerance2() (0.0)
#define goToTolerance1() (0.0)
#define goToTolerance2() (0.0)
#elif TOL_TYPE==2
#define useTolerance1() (perturbedTolerance)      
#define useTolerance2() (perturbedTolerance)
#define goToTolerance1() (0.0)
#define goToTolerance2() (0.0)
#elif TOL_TYPE==3
#define useTolerance1() (perturbedTolerance)      
#define useTolerance2() (perturbedTolerance)
#define goToTolerance1() (perturbedTolerance)
#define goToTolerance2() (0.0)
#elif TOL_TYPE==4
#define useTolerance1() (perturbedTolerance)      
#define useTolerance2() (perturbedTolerance)
#define goToTolerance1() (perturbedTolerance)
#define goToTolerance2() (perturbedTolerance)
#else
XXXX
#endif      
#if CLP_CAN_HAVE_ZERO_OBJ<2
#define MODIFYCOST 2
#endif
#define DONT_MOVE_OBJECTIVE
static void dualColumn2Bit(AbcSimplexDual * dual,dualColumnResult * result,
			     int numberBlocks)
{
  for (int iBlock=1;iBlock<numberBlocks;iBlock++) {
    cilk_spawn dual->dualColumn2First(result[iBlock]);
  }
  dual->dualColumn2First(result[0]);
  cilk_sync;
}
void
AbcSimplexDual::dualColumn2First(dualColumnResult & result)
{
  CoinPartitionedVector & candidateList = usefulArray_[arrayForDualColumn_]; 
  CoinPartitionedVector & flipList = usefulArray_[arrayForFlipBounds_];
  int iBlock=result.block;
  int numberSwapped = result.numberSwapped;
  int numberRemaining = result.numberRemaining;
  double tentativeTheta=result.tentativeTheta;
  // pivot elements
  double *  COIN_RESTRICT array=candidateList.denseVector();
  // indices
  int *  COIN_RESTRICT indices = candidateList.getIndices();
  const double *  COIN_RESTRICT abcLower = abcLower_;
  const double *  COIN_RESTRICT abcUpper = abcUpper_;
  const double *  COIN_RESTRICT abcDj = abcDj_;
  const unsigned char *  COIN_RESTRICT internalStatus = internalStatus_;
  const double multiplier[] = { 1.0, -1.0};

  // For list of flipped
  int *  COIN_RESTRICT flippedIndex = flipList.getIndices();
  double *  COIN_RESTRICT flippedElement = flipList.denseVector();
  // adjust
  //if (iBlock>=0) {
    int offset=candidateList.startPartition(iBlock);
    assert(offset==flipList.startPartition(iBlock));
    array += offset;
    indices+=offset;
    flippedIndex+=offset;
    flippedElement+=offset;
    //}
  double thruThis = 0.0;
  
  double bestPivot = currentAcceptablePivot_;
  int bestSequence = -1;
  int numberInList=numberRemaining;
  numberRemaining = 0;
  double upperTheta = 1.0e50;
  double increaseInThis = 0.0; //objective increase in this loop
  for (int i = 0; i < numberInList; i++) {
    int iSequence = indices[i];
    assert (getInternalStatus(iSequence) != isFree
	    && getInternalStatus(iSequence) != superBasic);
    int iStatus = internalStatus[iSequence] & 3;
    // treat as if at lower bound
    double mult = multiplier[iStatus];
    double alpha = array[i];
    double oldValue = abcDj[iSequence]*mult;
    double value = oldValue - tentativeTheta * alpha;
    assert (alpha>0.0);
    //double perturbedTolerance = abcPerturbation[iSequence];
    if (value < -currentDualTolerance_) {
      // possible here or could go through
      double range = abcUpper[iSequence] - abcLower[iSequence];
      thruThis += range * alpha;
      increaseInThis += (oldValue - useTolerance1()) * range;
      // goes on swapped list (also means candidates if too many)
      flippedElement[numberSwapped] = alpha;
      flippedIndex[numberSwapped++] = iSequence;
      if (alpha > bestPivot) {
	bestPivot = alpha;
	bestSequence = iSequence;
      }
    } else {
      // won't go through this time
      value = oldValue - upperTheta * alpha;
      if (value < -currentDualTolerance_ && alpha >= currentAcceptablePivot_)
	upperTheta = (oldValue + currentDualTolerance_) / alpha;
      array[numberRemaining] = alpha;
      indices[numberRemaining++] = iSequence;
    }
  }
  result.bestEverPivot=bestPivot;
  result.sequence=bestSequence;
  result.numberSwapped=numberSwapped;
  result.numberRemaining=numberRemaining;
  result.thruThis=thruThis;
  result.increaseInThis=increaseInThis;
  result.theta=upperTheta;
}
void
AbcSimplexDual::dualColumn2Most(dualColumnResult & result)
{
  CoinPartitionedVector & candidateList = usefulArray_[arrayForDualColumn_]; 
  CoinPartitionedVector & flipList = usefulArray_[arrayForFlipBounds_];
  int numberBlocksReal=candidateList.getNumPartitions();
  flipList.setPartitions(numberBlocksReal,candidateList.startPartitions());
  int numberBlocks;
  if (!numberBlocksReal)
    numberBlocks=1;
  else
    numberBlocks=numberBlocksReal;
  //usefulArray_[arrayForTableauRow_].compact();
  //candidateList.compact();
  int numberSwapped = 0;
  int numberRemaining = candidateList.getNumElements();
  
  double totalThru = 0.0; // for when variables flip
  int sequenceIn=-1;
  double bestEverPivot = currentAcceptablePivot_;
  // If we think we need to modify costs (not if something from broad sweep)
  bool modifyCosts = false;
  // Increase in objective due to swapping bounds (may be negative)
  double increaseInObjective = 0.0;
  // pivot elements
  double *  COIN_RESTRICT array=candidateList.denseVector();
  // indices
  int *  COIN_RESTRICT indices = candidateList.getIndices();
  const double *  COIN_RESTRICT abcLower = abcLower_;
  const double *  COIN_RESTRICT abcUpper = abcUpper_;
  //const double *  COIN_RESTRICT abcSolution = abcSolution_;
  const double *  COIN_RESTRICT abcDj = abcDj_;
  //const double *  COIN_RESTRICT abcPerturbation = abcPerturbation_;
  const unsigned char *  COIN_RESTRICT internalStatus = internalStatus_;
  // We can also see if infeasible or pivoting on free
  double tentativeTheta = 1.0e25; // try with this much smaller as guess
  const double multiplier[] = { 1.0, -1.0};

  // For list of flipped
  int numberLastSwapped=0;
  int *  COIN_RESTRICT flippedIndex = flipList.getIndices();
  double *  COIN_RESTRICT flippedElement = flipList.denseVector();
  // If sum of bad small pivots too much
#ifdef MORE_CAREFUL
  bool badSumPivots = false;
#endif
  // not free
  int lastSequence = -1;
  double lastTheta = 0.0; 
  double lastPivotValue = 0.0;
  double thisPivotValue=0.0;
  // if free then always choose , otherwise ...
  double theta = 1.0e50;
  // now flip flop between spare arrays until reasonable theta
  tentativeTheta = CoinMax(10.0 * upperTheta_, 1.0e-7);
  
  // loops increasing tentative theta until can't go through
#ifdef PRINT_RATIO_PROGRESS
  int iPass=0;
#endif
  dualColumnResult result2[8];
  for (int iBlock=0;iBlock<numberBlocks;iBlock++) {
    result2[iBlock].block=iBlock;
    if (numberBlocksReal)
      result2[iBlock].numberRemaining=candidateList.getNumElements(iBlock);
    else
      result2[iBlock].numberRemaining=candidateList.getNumElements();
    result2[iBlock].numberSwapped=0;
    result2[iBlock].numberLastSwapped=0;
  }
  double useThru=0.0;
  while (tentativeTheta < 1.0e22) {
    double thruThis = 0.0;
    
    double bestPivot = currentAcceptablePivot_;
    int bestSequence = -1;
    sequenceIn=-1;
    thisPivotValue=0.0;
    int numberInList=numberRemaining;
    double upperTheta = 1.0e50;
    double increaseInThis = 0.0; //objective increase in this loop
    for (int iBlock=0;iBlock<numberBlocks;iBlock++) {
      result2[iBlock].tentativeTheta=tentativeTheta;
      //result2[iBlock].numberRemaining=numberRemaining;
      //result2[iBlock].numberSwapped=numberSwapped;
      //result2[iBlock].numberLastSwapped=numberLastSwapped;
    }
    for (int iBlock=1;iBlock<numberBlocks;iBlock++) {
      cilk_spawn dualColumn2First(result2[iBlock]);
    }
    dualColumn2First(result2[0]);
    cilk_sync;
    //dualColumn2Bit(this,result2,numberBlocks);
    numberSwapped=0;
    numberRemaining=0;
    for (int iBlock=0;iBlock<numberBlocks;iBlock++) {
      if (result2[iBlock].bestEverPivot>bestPivot) {
	bestPivot=result2[iBlock].bestEverPivot;
	bestSequence=result2[iBlock].sequence;
      }
      numberSwapped+=result2[iBlock].numberSwapped;
      numberRemaining+=result2[iBlock].numberRemaining;
      thruThis+=result2[iBlock].thruThis;
      increaseInThis+=result2[iBlock].increaseInThis;
      if (upperTheta>result2[iBlock].theta)
	upperTheta=result2[iBlock].theta;
    }
    // end of pass
#ifdef PRINT_RATIO_PROGRESS
    iPass++;
    printf("Iteration %d pass %d dualOut %g thru %g increase %g numberRemain %d\n",
	   numberIterations_,iPass,fabs(dualOut_),totalThru+thruThis,increaseInObjective+increaseInThis,
	   numberRemaining);
#endif
    if (totalThru + thruThis > fabs(dualOut_)-fabs(1.0e-9*dualOut_)-1.0e-12 ||
	(increaseInObjective + increaseInThis < 0.0&&bestSequence>=0) || !numberRemaining) {
      // We should be pivoting in this batch
      // so compress down to this lot
      numberRemaining=0;
      candidateList.clearAndReset();
      for (int iBlock=0;iBlock<numberBlocks;iBlock++) {
	int offset=flipList.startPartition(iBlock);
	// set so can compact
	int numberKeep=result2[iBlock].numberLastSwapped;
	flipList.setNumElementsPartition(iBlock,numberKeep);
	for (int i = offset+numberKeep; i <offset+result2[iBlock].numberSwapped; i++) {
	  array[numberRemaining] = flippedElement[i];
	  flippedElement[i]=0;
	  indices[numberRemaining++] = flippedIndex[i];
	}
      }
      // make sure candidateList will be cleared correctly
      candidateList.setNumElements(numberRemaining);
      candidateList.setPackedMode(true);
      flipList.compact();
      numberSwapped=numberLastSwapped;
      useThru=totalThru;
      assert (totalThru<=fabs(dualOut_));
      int iTry;
      // first get ratio with tolerance
      for (iTry = 0; iTry < MAXTRY; iTry++) {
	
	upperTheta = 1.0e50;
	numberInList=numberRemaining;
	numberRemaining = 0;
	increaseInThis = 0.0; //objective increase in this loop
	thruThis = 0.0;
	for (int i = 0; i < numberInList; i++) {
	  int iSequence = indices[i];
	  assert (getInternalStatus(iSequence) != isFree
		  && getInternalStatus(iSequence) != superBasic);
	  int iStatus = internalStatus[iSequence] & 3;
	  // treat as if at lower bound
	  double mult = multiplier[iStatus];
	  double alpha = array[i];
	  double oldValue = abcDj[iSequence]*mult;
	  double value = oldValue - upperTheta * alpha;
	  assert (alpha>0.0);
	  //double perturbedTolerance = abcPerturbation[iSequence];
	  if (value < -currentDualTolerance_) {
	    if (alpha >= currentAcceptablePivot_) {
	      upperTheta = (oldValue + currentDualTolerance_) / alpha;
	    }
	  }
	}
	bestPivot = currentAcceptablePivot_;
	sequenceIn = -1;
	double largestPivot = currentAcceptablePivot_;
	// Sum of bad small pivots
#ifdef MORE_CAREFUL
	double sumBadPivots = 0.0;
	badSumPivots = false;
#endif
	// Make sure upperTheta will work (-O2 and above gives problems)
	upperTheta *= 1.0000000001;
	for (int i = 0; i < numberInList; i++) {
	  int iSequence = indices[i];
	  int iStatus = internalStatus[iSequence] & 3;
	  // treat as if at lower bound
	  double mult = multiplier[iStatus];
	  double alpha = array[i];
	  double oldValue = abcDj[iSequence]*mult;
	  double value = oldValue - upperTheta * alpha;
	  assert (alpha>0.0);
	  //double perturbedTolerance = abcPerturbation[iSequence];
	  double badDj = 0.0;
	  if (value <= 0.0) {
	    // add to list of swapped
	    flippedElement[numberSwapped] = alpha;
	    flippedIndex[numberSwapped++] = iSequence;
	    badDj = oldValue - useTolerance2();
	    // select if largest pivot
	    bool take = (alpha > bestPivot); 
#ifdef MORE_CAREFUL
	    if (alpha < currentAcceptablePivot_ && upperTheta < 1.0e20) {
	      if (value < -currentDualTolerance_) {
		double gap = abcUpper[iSequence] - abcLower[iSequence];
		if (gap < 1.0e20)
		  sumBadPivots -= value * gap;
		else
		  sumBadPivots += 1.0e20;
		//printf("bad %d alpha %g dj at lower %g\n",
		//     iSequence,alpha,value);
	      }
	    }
#endif
	    if (take&&flagged(iSequence)) {
	      if (bestPivot>currentAcceptablePivot_)
		take=false;
	    }
	    if (take) {
	      sequenceIn = iSequence;
	      thisPivotValue=alpha;
	      bestPivot =  alpha;
	      if (flagged(iSequence)) 
		bestPivot=currentAcceptablePivot_;
	      bestSequence = iSequence;
	      theta = (oldValue+goToTolerance1()) / alpha;
	      //assert (oldValue==abcDj[iSequence]);
	      largestPivot = CoinMax(largestPivot, 0.5 * bestPivot);
	    }
	    double range = abcUpper[iSequence] - abcLower[iSequence];
	    thruThis += range * fabs(alpha);
	    increaseInThis += badDj * range;
	  } else {
	    // add to list of remaining
	    array[numberRemaining] = alpha;
	    indices[numberRemaining++] = iSequence;
	  }
	}
#ifdef MORE_CAREFUL
	// If we have done pivots and things look bad set alpha_ 0.0 to force factorization
	if (sumBadPivots > 1.0e4) {
#if ABC_NORMAL_DEBUG>0
	  if (handler_->logLevel() > 1)
	    printf("maybe forcing re-factorization - sum %g  %d pivots\n", sumBadPivots,
		   abcFactorization_->pivots());
#endif
	  if(abcFactorization_->pivots() > 3) {
	    badSumPivots = true;
#ifdef PRINT_RATIO_PROGRESS
	    printf("maybe forcing re-factorization - sum %g  %d pivots\n", sumBadPivots,
		   abcFactorization_->pivots());
#endif
	    break;
	  }
	}
#endif
	// If we stop now this will be increase in objective (I think)
	double increase = (fabs(dualOut_) - totalThru) * theta;
	increase += increaseInObjective;
	if (theta < 0.0)
	  thruThis += fabs(dualOut_); // force using this one
	if (((increaseInObjective < 0.0 && increase < 0.0)||
	     (theta>1.0e9 && lastTheta < 1.0e-5))
	    && lastSequence >= 0) {
	  // back
	  // We may need to be more careful - we could do by
	  // switch so we always do fine grained?
	  bestPivot = 0.0;
	  bestSequence=-1;
	} else {
	  // add in
	  totalThru += thruThis;
	  increaseInObjective += increaseInThis;
	  lastTheta=theta;
	}
	if (bestPivot < 0.1 * bestEverPivot &&
	    bestEverPivot > 1.0e-6 &&
	    (bestPivot < 1.0e-3 || totalThru * 2.0 > fabs(dualOut_))) {
	  // back to previous one
	  sequenceIn = lastSequence;
	  thisPivotValue=lastPivotValue;
#ifdef PRINT_RATIO_PROGRESS
	  printf("Try %d back to previous bestPivot %g bestEver %g totalThru %g\n",
		 iTry,bestPivot,bestEverPivot,totalThru);
#endif
	  break;
	} else if (sequenceIn == -1 && upperTheta > largeValue_) {
	  if (lastPivotValue > currentAcceptablePivot_) {
	    // back to previous one
	    sequenceIn = lastSequence;
	    thisPivotValue=lastPivotValue;
#ifdef PRINT_RATIO_PROGRESS
	    printf("Try %d no pivot this time large theta %g lastPivot %g\n",
		   iTry,upperTheta,lastPivotValue);
#endif
	  } else {
	    // can only get here if all pivots too small
#ifdef PRINT_RATIO_PROGRESS
	    printf("XXBAD Try %d no pivot this time large theta %g lastPivot %g\n",
		   iTry,upperTheta,lastPivotValue);
#endif
	  }
	  break;
	} else if (totalThru >= fabs(dualOut_)) {
#ifdef PRINT_RATIO_PROGRESS
	  printf("Try %d upperTheta %g totalThru %g - can modify costs\n",
		 iTry,upperTheta,lastPivotValue);
#endif
	  modifyCosts = true; // fine grain - we can modify costs
	  break; // no point trying another loop
	} else {
	  lastSequence = sequenceIn;
	  lastPivotValue=thisPivotValue;
	  if (bestPivot > bestEverPivot) {
	    bestEverPivot = bestPivot;
	    //bestEverSequence=bestSequence;
	  }
	  modifyCosts = true; // fine grain - we can modify costs
	}
#ifdef PRINT_RATIO_PROGRESS
	printf("Onto next pass totalthru %g bestPivot %g\n",
	       totalThru,lastPivotValue);
#endif
	numberLastSwapped=numberSwapped;
      }
      break;
    } else {
      // skip this lot
      if (bestPivot > 1.0e-3 || bestPivot > bestEverPivot) {
#ifdef PRINT_RATIO_PROGRESS
	printf("Continuing bestPivot %g bestEver %g %d swapped - new tentative %g\n",
	       bestPivot,bestEverPivot,numberSwapped,2*upperTheta);
#endif
	bestEverPivot = bestPivot;
	//bestEverSequence=bestSequence;
	lastPivotValue=bestPivot;
	lastSequence = bestSequence;
	for (int iBlock=0;iBlock<numberBlocks;iBlock++) 
	  result2[iBlock].numberLastSwapped=result2[iBlock].numberSwapped;
	numberLastSwapped=numberSwapped;
      } else {
#ifdef PRINT_RATIO_PROGRESS
	printf("keep old swapped bestPivot %g bestEver %g %d swapped - new tentative %g\n",
	       bestPivot,bestEverPivot,numberSwapped,2*upperTheta);
#endif
	// keep old swapped
	numberRemaining=0;
	for (int iBlock=0;iBlock<numberBlocks;iBlock++) {
	  int offset=flipList.startPartition(iBlock);
	  int numberRemaining2=offset+result2[iBlock].numberRemaining;
	  int numberLastSwapped=result2[iBlock].numberLastSwapped;
	  for (int i = offset+numberLastSwapped; 
	       i <offset+result2[iBlock].numberSwapped; i++) {
	    array[numberRemaining2] = flippedElement[i];
	    flippedElement[i]=0;
	    indices[numberRemaining2++] = flippedIndex[i];
	  }
	  result2[iBlock].numberRemaining=numberRemaining2-offset;
	  numberRemaining += result2[iBlock].numberRemaining;
	  result2[iBlock].numberSwapped=numberLastSwapped;
	}
	numberSwapped=numberLastSwapped;
      }
      increaseInObjective += increaseInThis;
      tentativeTheta = 2.0 * upperTheta;
      totalThru += thruThis;
    }
  }
  if (flipList.getNumPartitions()) {
    // need to compress
    numberRemaining=0;
    candidateList.clearAndReset();
    for (int iBlock=0;iBlock<numberBlocks;iBlock++) {
      int offset=flipList.startPartition(iBlock);
      // set so can compact
      int numberKeep=result2[iBlock].numberLastSwapped;
      flipList.setNumElementsPartition(iBlock,numberKeep);
      for (int i = offset+numberKeep; i <offset+result2[iBlock].numberSwapped; i++) {
	array[numberRemaining] = flippedElement[i];
	flippedElement[i]=0;
	indices[numberRemaining++] = flippedIndex[i];
      }
    }
    // make sure candidateList will be cleared correctly
    candidateList.setNumElements(numberRemaining);
    candidateList.setPackedMode(true);
    flipList.compact();
    numberSwapped=numberLastSwapped;
  }
  result.theta=theta; //?
  result.totalThru=totalThru;
  result.useThru=useThru;
  result.bestEverPivot=bestEverPivot; //?
  result.increaseInObjective=increaseInObjective; //?
  result.tentativeTheta=tentativeTheta; //?
  result.lastPivotValue=lastPivotValue;
  result.thisPivotValue=thisPivotValue;
  result.lastSequence=lastSequence;
  result.sequence=sequenceIn;
  //result.block;
  //result.pass;
  //result.phase;
  result.numberSwapped=numberSwapped;
  result.numberLastSwapped=numberLastSwapped;
  result.modifyCosts=modifyCosts;
}
/*
  Chooses incoming
  Puts flipped ones in list
  If necessary will modify costs
*/
void
AbcSimplexDual::dualColumn2()
{
  CoinPartitionedVector & candidateList = usefulArray_[arrayForDualColumn_]; 
  CoinPartitionedVector & flipList = usefulArray_[arrayForFlipBounds_];
  //usefulArray_[arrayForTableauRow_].compact();
  usefulArray_[arrayForTableauRow_].computeNumberElements();
  if (candidateList.getNumElements()<100)
    candidateList.compact();
  //usefulArray_[arrayForTableauRow_].compact();
  // But factorizations complain if <1.0e-8
  //acceptablePivot=CoinMax(acceptablePivot,1.0e-8);
  // do ratio test for normal iteration
  currentAcceptablePivot_ = 1.0e-1 * acceptablePivot_;
  if (numberIterations_ > 100)
    currentAcceptablePivot_ = acceptablePivot_;
  if (abcFactorization_->pivots() > 10 ||
      (abcFactorization_->pivots() && sumDualInfeasibilities_))
    currentAcceptablePivot_ = 1.0e+3 * acceptablePivot_; // if we have iterated be more strict
  else if (abcFactorization_->pivots() > 5)
    currentAcceptablePivot_ = 1.0e+2 * acceptablePivot_; // if we have iterated be slightly more strict
  else if (abcFactorization_->pivots())
    currentAcceptablePivot_ = acceptablePivot_; // relax
  // If very large infeasibility use larger acceptable pivot
  if (abcFactorization_->pivots()) {
    double value=CoinMax(currentAcceptablePivot_,1.0e-12*fabs(dualOut_));
    currentAcceptablePivot_=CoinMin(1.0e2*currentAcceptablePivot_,value);
  }
  // return at once if no free but there were free
  if (lastFirstFree_>=0&&sequenceIn_<0) {
    candidateList.clearAndReset();
    upperTheta_=COIN_DBL_MAX;
  }
  if (upperTheta_==COIN_DBL_MAX) {
    usefulArray_[arrayForTableauRow_].compact();
    assert (!candidateList.getNumElements());
    return;
  }
  int numberSwapped = 0;
  //int numberRemaining = candidateList.getNumElements();
  
  double useThru = 0.0; // for when variables flip
  
  // If we think we need to modify costs (not if something from broad sweep)
  bool modifyCosts = false;
  // pivot elements
  //double *  COIN_RESTRICT array=candidateList.denseVector();
  // indices
  const double *  COIN_RESTRICT abcLower = abcLower_;
  const double *  COIN_RESTRICT abcUpper = abcUpper_;
  const double *  COIN_RESTRICT abcSolution = abcSolution_;
  const double *  COIN_RESTRICT abcDj = abcDj_;
  //const double *  COIN_RESTRICT abcPerturbation = abcPerturbation_;
  const unsigned char *  COIN_RESTRICT internalStatus = internalStatus_;
  // We can also see if infeasible or pivoting on free
  const double multiplier[] = { 1.0, -1.0};

  // For list of flipped
  int numberLastSwapped=0;
  int *  COIN_RESTRICT flippedIndex = flipList.getIndices();
  double *  COIN_RESTRICT flippedElement = flipList.denseVector();
  // If sum of bad small pivots too much
#ifdef MORE_CAREFUL
  bool badSumPivots = false;
#endif
  if (sequenceIn_<0) {
    int lastSequence = -1;
    double lastPivotValue = 0.0;
    double thisPivotValue=0.0;
    dualColumnResult result;
    dualColumn2Most(result);
    sequenceIn_=result.sequence;
    lastSequence=result.lastSequence;
    thisPivotValue=result.thisPivotValue;
    lastPivotValue=result.lastPivotValue;
    numberSwapped=result.numberSwapped;
    numberLastSwapped=result.numberLastSwapped;
    modifyCosts=result.modifyCosts;
    useThru=result.useThru;
#ifdef PRINT_RATIO_PROGRESS
    printf("Iteration %d - out of loop - sequenceIn %d pivotValue %g\n",
	   numberIterations_,sequenceIn_,thisPivotValue);
#endif
    // can get here without sequenceIn_ set but with lastSequence
    if (sequenceIn_ < 0 && lastSequence >= 0) {
      // back to previous one
      sequenceIn_ = lastSequence;
      thisPivotValue=lastPivotValue;
#ifdef PRINT_RATIO_PROGRESS
      printf("BUT back one - out of loop - sequenceIn %d pivotValue %g\n",
	   sequenceIn_,thisPivotValue);
#endif
    }
    
    // Movement should be minimum for anti-degeneracy - unless
    // fixed variable out
    // no need if just one candidate
    assert( numberSwapped-numberLastSwapped>=0);
    double minimumTheta;
    if (upperOut_ > lowerOut_&&numberSwapped-numberLastSwapped>1)
#ifndef TRY_ABC_GUS
      minimumTheta = MINIMUMTHETA;
#else
      minimumTheta = minimumThetaMovement_;
#endif
    else
      minimumTheta = 0.0;
    if (sequenceIn_ >= 0) {
      double oldValue;
      alpha_ = thisPivotValue; 
      // correct sign
      int iStatus = internalStatus[sequenceIn_] & 3;
      alpha_ *= multiplier[iStatus];
      oldValue = abcDj[sequenceIn_];
      theta_ = CoinMax(oldValue / alpha_, 0.0);
      if (theta_ < minimumTheta && fabs(alpha_) < 1.0e5 && 1) {
	// can't pivot to zero
	theta_ = minimumTheta;
      }
      // may need to adjust costs so all dual feasible AND pivoted is exactly 0
      if (modifyCosts
#ifdef MORE_CAREFUL
	  &&!badSumPivots
#endif
	  ) {
	for (int i = numberLastSwapped; i < numberSwapped; i++) {
	  int iSequence = flippedIndex[i];
	  int iStatus = internalStatus[iSequence] & 3;
	  // treat as if at lower bound
	  double mult = multiplier[iStatus];
	  double alpha = flippedElement[i];
	  flippedElement[i]=0.0;
	  if (iSequence==sequenceIn_)
	    continue;
	  double oldValue = abcDj[iSequence]*mult;
	  double value = oldValue - theta_ * alpha;
	  //double perturbedTolerance = abcPerturbation[iSequence];
	  if (-value > currentDualTolerance_) {
	    //thisIncrease = true;
#if MODIFYCOST
	    if (numberIterations_>=normalDualColumnIteration_) {
	      // modify cost to hit new tolerance
	      double modification = alpha * theta_ - oldValue - currentDualTolerance_;
	      if ((specialOptions_&(2048 + 4096)) != 0) {
		if ((specialOptions_ & 2048) != 0) {
		  if (fabs(modification) < 1.0e-10)
		    modification = 0.0;
		} else {
		  if (fabs(modification) < 1.0e-12)
		    modification = 0.0;
		}
	      }
	      abcDj_[iSequence] += mult*modification;
	      abcCost_[iSequence] +=  mult*modification;
	      if (modification)
		numberChanged_ ++; // Say changed costs
	    }
#endif
	  }
	}
	numberSwapped=numberLastSwapped;
      }
    }
  }
  
#ifdef MORE_CAREFUL
  // If we have done pivots and things look bad set alpha_ 0.0 to force factorization
  if ((badSumPivots ||
       fabs(theta_ * badFree) > 10.0 * currentDualTolerance_) && abcFactorization_->pivots()) {
#if ABC_NORMAL_DEBUG>0
    if (handler_->logLevel() > 1)
      printf("forcing re-factorization\n");
#endif
    sequenceIn_ = -1;
  }
#endif
  if (sequenceIn_ >= 0) {
    lowerIn_ = abcLower[sequenceIn_];
    upperIn_ = abcUpper[sequenceIn_];
    valueIn_ = abcSolution[sequenceIn_];
    dualIn_ = abcDj_[sequenceIn_];
    
    if (numberTimesOptimal_) {
      // can we adjust cost back closer to original
      //*** add coding
    }
    if (numberIterations_>=normalDualColumnIteration_) {
#if MODIFYCOST>1
      // modify cost to hit zero exactly
      // so (dualIn_+modification)==theta_*alpha_
      //double perturbedTolerance = abcPerturbation[sequenceIn_];
      double modification = theta_ * alpha_ - (dualIn_+goToTolerance2());
      // But should not move objective too much ??
#ifdef DONT_MOVE_OBJECTIVE
      double moveObjective = fabs(modification * abcSolution[sequenceIn_]);
      double smallMove = CoinMax(fabs(rawObjectiveValue_), 1.0e-3);
      if (moveObjective > smallMove) {
#if ABC_NORMAL_DEBUG>0
	if (handler_->logLevel() > 1)
	  printf("would move objective by %g - original mod %g sol value %g\n", moveObjective,
		 modification, abcSolution[sequenceIn_]);
#endif
	modification *= smallMove / moveObjective;
      }
#endif
#ifdef MORE_CAREFUL
      if (badSumPivots)
	modification = 0.0;
#endif
      if (atFakeBound(sequenceIn_)&&fabs(modification)<currentDualTolerance_)
	modification=0.0;
      if ((specialOptions_&(2048 + 4096)) != 0) {
	if ((specialOptions_ & 16384) != 0) {
	  // in fast dual
	  if (fabs(modification) < 1.0e-7)
	    modification = 0.0;
	} else if ((specialOptions_ & 2048) != 0) {
	  if (fabs(modification) < 1.0e-10)
	    modification = 0.0;
	} else {
	  if (fabs(modification) < 1.0e-12)
	    modification = 0.0;
	}
      }
      
      dualIn_ += modification;
      abcDj_[sequenceIn_] = dualIn_;
      abcCost_[sequenceIn_] += modification;
      if (modification)
	numberChanged_ ++; // Say changed costs
      //int costOffset = numberRows_+numberColumns_;
      //abcCost[sequenceIn_+costOffset] += modification; // save change
      //assert (fabs(modification)<1.0e-6);
#if ABC_NORMAL_DEBUG>3
      if ((handler_->logLevel() & 32) && fabs(modification) > 1.0e-15)
	printf("exact %d new cost %g, change %g\n", sequenceIn_,
	       abcCost_[sequenceIn_], modification);
#endif
#endif
    }
    
    if (alpha_ < 0.0) {
      // as if from upper bound
      directionIn_ = -1;
      upperIn_ = valueIn_;
    } else {
      // as if from lower bound
      directionIn_ = 1;
      lowerIn_ = valueIn_;
    }
  } else {
    // no pivot
    alpha_ = 0.0;
  }
  // clear array
  //printf("dualout %g theta %g alpha %g dj %g thru %g swapped %d product %g guess %g useThru %g\n",
  //	 dualOut_,theta_,alpha_,abcDj_[sequenceIn_],useThru,numberSwapped,
  //	 fabs(dualOut_)*theta_,objectiveValue()+(fabs(dualOut_)-useThru)*theta_,useThru);
  objectiveChange_ = (fabs(dualOut_)-useThru)*theta_;
  rawObjectiveValue_+= objectiveChange_;
  //CoinZeroN(array, maximumNumberInList);
  //candidateList.setNumElements(0);
  candidateList.clearAndReset();
  flipList.setNumElements(numberSwapped);
  if (numberSwapped)
    flipList.setPackedMode(true);
#ifdef PRINT_RATIO_PROGRESS
  printf("%d flipped\n",numberSwapped);
#endif
}
/* Checks if tentative optimal actually means unbounded
   Returns -3 if not, 2 if is unbounded */
int
AbcSimplexDual::checkUnbounded(CoinIndexedVector & ray,
                               double changeCost)
{
  int status = 2; // say unbounded
  abcFactorization_->updateColumn(ray);
  // get reduced cost
  int number = ray.getNumElements();
  int *  COIN_RESTRICT index = ray.getIndices();
  double *  COIN_RESTRICT array = ray.denseVector();
  const int * COIN_RESTRICT abcPivotVariable = abcPivotVariable_;
  for (int i = 0; i < number; i++) {
    int iRow = index[i];
    int iPivot = abcPivotVariable[iRow];
    changeCost -= cost(iPivot) * array[iRow];
  }
  double way;
  if (changeCost > 0.0) {
    //try going down
    way = 1.0;
  } else if (changeCost < 0.0) {
    //try going up
    way = -1.0;
  } else {
#if ABC_NORMAL_DEBUG>3
    printf("can't decide on up or down\n");
#endif
    way = 0.0;
    status = -3;
  }
  double movement = 1.0e10 * way; // some largish number
  double zeroTolerance = 1.0e-14 * dualBound_;
  for (int i = 0; i < number; i++) {
    int iRow = index[i];
    int iPivot = abcPivotVariable[iRow];
    double arrayValue = array[iRow];
    if (fabs(arrayValue) < zeroTolerance)
      arrayValue = 0.0;
    double newValue = solution(iPivot) + movement * arrayValue;
    if (newValue > upper(iPivot) + primalTolerance_ ||
	newValue < lower(iPivot) - primalTolerance_)
      status = -3; // not unbounded
  }
  if (status == 2) {
    // create ray
    delete [] ray_;
    ray_ = new double [numberColumns_];
    CoinZeroN(ray_, numberColumns_);
    for (int i = 0; i < number; i++) {
      int iRow = index[i];
      int iPivot = abcPivotVariable[iRow];
      double arrayValue = array[iRow];
      if (iPivot < numberColumns_ && fabs(arrayValue) >= zeroTolerance)
	ray_[iPivot] = way * array[iRow];
    }
  }
  ray.clear();
  return status;
}
// Saves good status etc 
void 
AbcSimplex::saveGoodStatus()
{
  // save arrays
  CoinMemcpyN(internalStatus_, numberTotal_, internalStatusSaved_);
  // save costs
  CoinMemcpyN(abcCost_,numberTotal_,abcCost_+2*numberTotal_);
  CoinMemcpyN(abcSolution_,numberTotal_,solutionSaved_);
}
// Restores previous good status and says trouble
void 
AbcSimplex::restoreGoodStatus(int type)
{
#if PARTITION_ROW_COPY==1
  stateOfProblem_ |= NEED_BASIS_SORT;
#endif
  if (type) {
    if (alphaAccuracy_ != -1.0)
      alphaAccuracy_ = -2.0;
    // trouble - restore solution
    CoinMemcpyN(internalStatusSaved_, numberTotal_, internalStatus_);
    CoinMemcpyN(abcCost_+2*numberTotal_,numberTotal_,abcCost_);
    CoinMemcpyN(solutionSaved_,numberTotal_, abcSolution_);
    int *  COIN_RESTRICT abcPivotVariable = abcPivotVariable_;
    // redo pivotVariable
    int numberBasic=0;
    for (int i=0;i<numberTotal_;i++) {
      if (getInternalStatus(i)==basic)
	abcPivotVariable[numberBasic++]=i;
    }
    assert (numberBasic==numberRows_);
    forceFactorization_ = 1; // a bit drastic but ..
    changeMade_++; // say something changed
  }
}
static int computePrimalsAndCheck(AbcSimplexDual * model,const int whichArray[2])
{
  CoinIndexedVector * array1 = model->usefulArray(whichArray[0]);
  CoinIndexedVector * array2 = model->usefulArray(whichArray[1]);
  int numberRefinements=model->computePrimals(array1,array2);
  model->checkPrimalSolution(true);
  return numberRefinements;
}
static int computeDualsAndCheck(AbcSimplexDual * model,const int whichArray[2])
{
  CoinIndexedVector * array1 = model->usefulArray(whichArray[0]);
  CoinIndexedVector * array2 = model->usefulArray(whichArray[1]);
  int numberRefinements=model->computeDuals(NULL,array1,array2);
  model->checkDualSolutionPlusFake();
  return numberRefinements;
}
// Computes solutions - 1 do duals, 2 do primals, 3 both
int
AbcSimplex::gutsOfSolution(int type)
{
  AbcSimplexDual * dual = reinterpret_cast<AbcSimplexDual *>(this);
  // do work and check
  int numberRefinements=0;
#if ABC_PARALLEL
  if (type!=3) {
#endif
    if ((type&2)!=0) {
      //work space
      int whichArray[2];
      for (int i=0;i<2;i++)
	whichArray[i]=getAvailableArray();
      numberRefinements=computePrimalsAndCheck(dual,whichArray);
      for (int i=0;i<2;i++)
	setAvailableArray(whichArray[i]);
    }
    if ((type&1)!=0
#if CAN_HAVE_ZERO_OBJ>1
	&&(specialOptions_&2097152)==0
#endif
	) {
      //work space
      int whichArray[2];
      for (int i=0;i<2;i++)
	whichArray[i]=getAvailableArray();
      numberRefinements+=computeDualsAndCheck(dual,whichArray);
      for (int i=0;i<2;i++)
	setAvailableArray(whichArray[i]);
    }
#if ABC_PARALLEL
  } else {
#ifndef NDEBUG
    abcFactorization_->checkMarkArrays();
#endif
    // redo to do in parallel
      //work space
      int whichArray[5];
      for (int i=1;i<5;i++)
	whichArray[i]=getAvailableArray();
      if (parallelMode_==0) {
	numberRefinements+=computePrimalsAndCheck(dual,whichArray+3);
	numberRefinements+=computeDualsAndCheck(dual,whichArray+1);
      } else {
#if ABC_PARALLEL==1
	threadInfo_[0].stuff[0]=whichArray[1];
	threadInfo_[0].stuff[1]=whichArray[2];
	stopStart_=1+32*1;
	startParallelStuff(1);
#else
	whichArray[0]=1;
	CoinAbcThreadInfo info;
	info.status=1;
	info.stuff[0]=whichArray[1];
	info.stuff[1]=whichArray[2];
	int n=cilk_spawn computeDualsAndCheck(dual,whichArray+1);
#endif
	numberRefinements=computePrimalsAndCheck(dual,whichArray+3);
#if ABC_PARALLEL==1
	numberRefinements+=stopParallelStuff(1);
#else
	cilk_sync;
	numberRefinements+=n;
#endif
      }
      for (int i=1;i<5;i++)
	setAvailableArray(whichArray[i]);
  }
#endif
  rawObjectiveValue_ +=sumNonBasicCosts_;
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
// Make non free variables dual feasible by moving to a bound
int 
AbcSimplexDual::makeNonFreeVariablesDualFeasible(bool changeCosts)
{
  const double multiplier[] = { 1.0, -1.0}; // sign?????????????
  double dualT = - 100.0*currentDualTolerance_;
  int numberMoved=0;
  int numberChanged=0;
  double *  COIN_RESTRICT solution = abcSolution_;
  double *  COIN_RESTRICT lower = abcLower_;
  double *  COIN_RESTRICT upper = abcUpper_;
  double saveNonBasicCosts =sumNonBasicCosts_;
  double largestCost=dualTolerance_;
  for (int iSequence = 0; iSequence < numberTotal_; iSequence++) {
    unsigned char iStatus = internalStatus_[iSequence] & 7;
    if (iStatus<4) {
      double mult = multiplier[iStatus];
      double dj = abcDj_[iSequence] * mult;
      if (dj < dualT) {
	largestCost=CoinMax(fabs(abcPerturbation_[iSequence]),largestCost);
      }
    } else if (iStatus!=7) {
      largestCost=CoinMax(fabs(abcPerturbation_[iSequence]),largestCost);
    }
  }
  for (int iSequence = 0; iSequence < numberTotal_; iSequence++) {
    unsigned char iStatus = internalStatus_[iSequence] & 7;
    if (iStatus<4) {
      double mult = multiplier[iStatus];
      double dj = abcDj_[iSequence] * mult;
      // ? should use perturbationArray
      if (dj < dualT&&changeCosts) {
	double gap=upper[iSequence]-lower[iSequence];
	if (gap>1.1*currentDualBound_) {
	  assert(getFakeBound(iSequence)==AbcSimplexDual::noFake);
	  if (!iStatus) {
	    upper[iSequence]=lower[iSequence]+currentDualBound_;
	    setFakeBound(iSequence, AbcSimplexDual::upperFake);
	  } else {
	    lower[iSequence]=upper[iSequence]-currentDualBound_;
	    setFakeBound(iSequence, AbcSimplexDual::lowerFake);
	  }
	}
	double costDifference=abcPerturbation_[iSequence]-abcCost_[iSequence];
	if (costDifference*mult>0.0) {
#if ABC_NORMAL_DEBUG>0
	  //printf("can improve situation on %d by %g\n",iSequence,costDifference*mult);
#endif
	  dj+=costDifference*mult;
	  abcDj_[iSequence]+=costDifference;
	  abcCost_[iSequence]=abcPerturbation_[iSequence];
	  costDifference=0.0;
	}
	double changeInObjective=-gap*dj;
	// probably need to be more intelligent
	if ((changeInObjective>1.0e3||gap>CoinMax(0.5*dualBound_,1.0e3))&&dj>-1.0e-1) {
	  // change cost
	  costDifference=(-dj-0.5*dualTolerance_)*mult;
	  if (fabs(costDifference)<100000.0*dualTolerance_) {
	    dj+=costDifference*mult;
	    abcDj_[iSequence]+=costDifference;
	    abcCost_[iSequence]+=costDifference;
	    numberChanged++;
	  }
	}
      }
      if (dj < dualT&&!changeCosts) {
	numberMoved++;
	if (iStatus==0) {
	  // at lower bound
	  // to upper bound
	  setInternalStatus(iSequence, atUpperBound);
	  solution[iSequence] = upper[iSequence];
	  sumNonBasicCosts_ += abcCost_[iSequence]*(upper[iSequence]-lower[iSequence]);
	} else {
	  // at upper bound
	  // to lower bound
	  setInternalStatus(iSequence, atLowerBound);
	  solution[iSequence] = lower[iSequence];
	  sumNonBasicCosts_ -= abcCost_[iSequence]*(upper[iSequence]-lower[iSequence]);
	}
      }
    }
  }
#if ABC_NORMAL_DEBUG>0
  if (numberMoved+numberChanged)
  printf("makeDualfeasible %d moved, %d had costs changed - sum dual %g\n",
	 numberMoved,numberChanged,sumDualInfeasibilities_);
#endif
  rawObjectiveValue_ +=(sumNonBasicCosts_-saveNonBasicCosts);
  setClpSimplexObjectiveValue();
  return numberMoved;
}
int 
AbcSimplexDual::whatNext()
{
  int returnCode=0;
  assert (!stateOfIteration_);
  // see if update stable
#if ABC_NORMAL_DEBUG>3
  if ((handler_->logLevel() & 32))
    printf("btran alpha %g, ftran alpha %g\n", btranAlpha_, alpha_);
#endif
  int numberPivots=abcFactorization_->pivots();
  //double checkValue = 1.0e-7; // numberPivots<5 ? 1.0e-7 : 1.0e-6;
  double checkValue = numberPivots ? 1.0e-7 : 1.0e-5;
  // relax if free variable in
  if ((internalStatus_[sequenceIn_]&7)>1)
    checkValue=1.0e-5;
  // if can't trust much and long way from optimal then relax
  if (largestPrimalError_ > 10.0)
    checkValue = CoinMin(1.0e-4, 1.0e-8 * largestPrimalError_);
  if (fabs(btranAlpha_) < 1.0e-12 || fabs(alpha_) < 1.0e-12 ||
      fabs(btranAlpha_ - alpha_) > checkValue*(1.0 + fabs(alpha_))) {
    handler_->message(CLP_DUAL_CHECK, messages_)
      << btranAlpha_
      << alpha_
      << CoinMessageEol;
    // see with more relaxed criterion
    double test;
    if (fabs(btranAlpha_) < 1.0e-8 || fabs(alpha_) < 1.0e-8)
      test = 1.0e-1 * fabs(alpha_);
    else
      test = 10.0*checkValue;//1.0e-4 * (1.0 + fabs(alpha_));
    bool needFlag = (fabs(btranAlpha_) < 1.0e-12 || fabs(alpha_) < 1.0e-12 ||
		     fabs(btranAlpha_ - alpha_) > test);
#if ABC_NORMAL_DEBUG>0
    if (fabs(btranAlpha_ + alpha_) < 1.0e-5*(1.0 + fabs(alpha_))) {
      printf("alpha diff (%g,%g) %g check %g\n",btranAlpha_,alpha_,fabs(btranAlpha_-alpha_),checkValue*(1.0+fabs(alpha_)));
    }
#endif
    if (numberPivots) {
      if (needFlag&&numberPivots<10) {
	// need to reject something
	assert (sequenceOut_>=0);
	char x = isColumn(sequenceOut_) ? 'C' : 'R';
	handler_->message(CLP_SIMPLEX_FLAG, messages_)
	  << x << sequenceWithin(sequenceOut_)
	  << CoinMessageEol;
#if ABC_NORMAL_DEBUG>0
	printf("flag %d as alpha  %g %g - after %d pivots\n", sequenceOut_,
	       btranAlpha_, alpha_,numberPivots);
#endif
	// Make safer?
	abcFactorization_->saferTolerances (1.0, -1.03);
	setFlagged(sequenceOut_);
	// so can't happen immediately again
	sequenceOut_=-1;
	lastBadIteration_ = numberIterations_; // say be more cautious
      }
      // Make safer?
      //if (numberPivots<5)
      //abcFactorization_->saferTolerances (-0.99, -1.01);
      problemStatus_ = -2; // factorize now
      returnCode = -2;
      stateOfIteration_=2;
    } else {
      if (needFlag) {
	// need to reject something
	assert (sequenceOut_>=0);
	char x = isColumn(sequenceOut_) ? 'C' : 'R';
	handler_->message(CLP_SIMPLEX_FLAG, messages_)
	  << x << sequenceWithin(sequenceOut_)
	  << CoinMessageEol;
#if ABC_NORMAL_DEBUG>3
	printf("flag a %g %g\n", btranAlpha_, alpha_);
#endif
	// Make safer?
	double tolerance=abcFactorization_->pivotTolerance();
	abcFactorization_->saferTolerances (1.0, -1.03);
	setFlagged(sequenceOut_);
	// so can't happen immediately again
	sequenceOut_=-1;
	//abcProgress_.clearBadTimes();
	lastBadIteration_ = numberIterations_; // say be more cautious
	if (fabs(alpha_) < 1.0e-10 && fabs(btranAlpha_) < 1.0e-8 && numberIterations_ > 100) {
	  //printf("I think should declare infeasible\n");
	  problemStatus_ = 1;
	  returnCode = 1;
	  stateOfIteration_=2;
	} else {
	  stateOfIteration_=1;
	  if (tolerance<abcFactorization_->pivotTolerance()) {
	    stateOfIteration_=2;
	    returnCode=-2;
	  }
	}
      }
    }
  }
  if (!stateOfIteration_) {
    // check update
    returnCode=abcFactorization_->checkReplacePart2(pivotRow_,btranAlpha_,alpha_,ftAlpha_);
  }
  return returnCode;
}
/* Checks if finished.  
   type 0 initial
   1 normal
   2 initial but we have solved problem before
   3 can skip factorization
   Updates status */
void
AbcSimplexDual::statusOfProblemInDual(int type)
{
  // If lots of iterations then adjust costs if large ones
  int numberPivots = abcFactorization_->pivots();
#if 0
  printf("statusOf %d pivots type %d pstatus %d forceFac %d dont %d\n",
  	 numberPivots,type,problemStatus_,forceFactorization_,dontFactorizePivots_);
#endif
  int saveType=type;
  double lastSumDualInfeasibilities=sumDualInfeasibilities_;
  double lastSumPrimalInfeasibilities=sumPrimalInfeasibilities_;
  if (type>=4) {
    // force factorization
    type &= 3;
    numberPivots=9999999;
  }
  //double realDualInfeasibilities = 0.0;
  double changeCost;
  bool unflagVariables = numberFlagged_>0;
  int weightsSaved = 0;
  int numberBlackMarks=0;
  if (!type) {
    // be optimistic
    stateOfProblem_ &= ~PESSIMISTIC;
    // but use 0.1
    double newTolerance = CoinMax(0.1, saveData_.pivotTolerance_);
    abcFactorization_->pivotTolerance(newTolerance);
  }
  if (type != 3&&(numberPivots>dontFactorizePivots_||numberIterations_==baseIteration_)) {
    // factorize
    // save dual weights
    abcDualRowPivot_->saveWeights(this, 1);
    weightsSaved = 2;
    // is factorization okay?
    int factorizationStatus=0;
  if ((largestPrimalError_ > 1.0e1||largestDualError_>1.0e1) 
      && numberRows_ > 100 && numberIterations_>baseIteration_) {
    double newTolerance = CoinMin(1.1* abcFactorization_->pivotTolerance(), 0.99);
    abcFactorization_->pivotTolerance(newTolerance);
  }
#ifdef EARLY_FACTORIZE
    if ((numberEarly_&0xffff)<=5||!usefulArray_[ABC_NUMBER_USEFUL-1].getNumElements()) {
	//#define KEEP_TRYING_EARLY
#ifdef KEEP_TRYING_EARLY
      int savedEarly=numberEarly_>>16;
      int newEarly=(numberEarly_&0xffff);
      newEarly=CoinMin(savedEarly,1+2*newEarly);
      numberEarly_=(savedEarly<<16)|newEarly;
#endif
      factorizationStatus = internalFactorize(type ? 1 : 2);
    } else {
      CoinIndexedVector & vector = usefulArray_[ABC_NUMBER_USEFUL-1];
      bool bad = vector.getNumElements()<0;
      const int * indices = vector.getIndices();
      double * array = vector.denseVector();
      int capacity = vector.capacity();
      capacity--;
      int numberPivotsStored = indices[capacity];
#if ABC_NORMAL_DEBUG>0
      printf("early status %s nPivots %d\n",bad ? "bad" : "good",numberPivotsStored);
#endif
      if (!bad) {
	// do remaining pivots
	int iterationsDone=abcEarlyFactorization_->pivots();
	factorizationStatus = 
	  abcEarlyFactorization_->replaceColumns(this,vector,
						 iterationsDone,numberPivotsStored,true);
	if (!factorizationStatus) {
	  // should be able to compare e.g. basics 1.0
#define GOOD_EARLY
#ifdef GOOD_EARLY
	  AbcSimplexFactorization * temp = abcFactorization_;
	  abcFactorization_=abcEarlyFactorization_;
	  abcEarlyFactorization_=temp;
#endif
	  moveToBasic();
#ifndef GOOD_EARLY
	  int * pivotSave = CoinCopyOfArray(abcPivotVariable_,numberRows_);
	  factorizationStatus = internalFactorize(type ? 1 : 2);
	  {
	    double * array = usefulArray_[3].denseVector();
	    double * array2 = usefulArray_[4].denseVector();
	    for (int iPivot = 0; iPivot < numberRows_; iPivot++) {
	      int iSequence = abcPivotVariable_[iPivot];
	      unpack(usefulArray_[3], iSequence);
	      unpack(usefulArray_[4], iSequence);
	      abcFactorization_->updateColumn(usefulArray_[3]);
	      abcEarlyFactorization_->updateColumn(usefulArray_[4]);
	      assert (fabs(array[iPivot] - 1.0) < 1.0e-4);
	      array[iPivot] = 0.0;
	      int iPivot2;
	      for (iPivot2=0;iPivot2<numberRows_;iPivot2++) {
		if (pivotSave[iPivot2]==iSequence)
		  break;
	      }
	      assert (iPivot2<numberRows_);
	      assert (fabs(array2[iPivot2] - 1.0) < 1.0e-4);
	      array2[iPivot2] = 0.0;
	      for (int i = 0; i < numberRows_; i++)
		assert (fabs(array[i]) < 1.0e-4);
	      for (int i = 0; i < numberRows_; i++)
		assert (fabs(array2[i]) < 1.0e-4);
	      usefulArray_[3].clear();
	      usefulArray_[4].clear();
	    }
	    delete [] pivotSave;
	  }
#endif
	} else {
	  bad=true;
	}
      }
      // clean up
      vector.setNumElements(0);
      for (int i=0;i<2*numberPivotsStored;i++) {
	array[--capacity]=0.0;
      }
      if (bad) {
	// switch off
#if ABC_NORMAL_DEBUG>0
	printf("switching off early factorization(in statusOf)\n");
#endif
#ifndef KEEP_TRYING_EARLY
	numberEarly_=0;
	delete abcEarlyFactorization_;
	abcEarlyFactorization_=NULL;
#else
	numberEarly_&=0xffff0000;
#endif
	factorizationStatus = internalFactorize(1);
      }
    }
#else
    factorizationStatus = internalFactorize(type ? 1 : 2);
#endif
    if (factorizationStatus) {
      if (type == 1&&lastFlaggedIteration_!=1) {
	// use for real disaster 
	lastFlaggedIteration_ = 1;
	// no - restore previous basis
	unflagVariables = false;
	// restore weights
	weightsSaved = 4;
	changeMade_++; // say something changed
	// Keep any flagged variables
	for (int i = 0; i < numberTotal_; i++) {
	  if (flagged(i))
	    internalStatusSaved_[i] |= 64; //say flagged
	}
	restoreGoodStatus(type);
	// get correct bounds on all variables
	resetFakeBounds(1);
	numberBlackMarks=10;
	if (sequenceOut_>=0) {
	  // need to reject something
	  char x = isColumn(sequenceOut_) ? 'C' : 'R';
	  handler_->message(CLP_SIMPLEX_FLAG, messages_)
	    << x << sequenceWithin(sequenceOut_)
	    << CoinMessageEol;
#if ABC_NORMAL_DEBUG>3
	  printf("flag d\n");
#endif
	  setFlagged(sequenceOut_);
	  lastBadIteration_ = numberIterations_; // say be more cautious
	  // so can't happen immediately again
	  sequenceOut_=-1;
	}
	//abcProgress_.clearBadTimes();
	
	// Go to safe
	abcFactorization_->pivotTolerance(0.99);
      } else {
#if ABC_NORMAL_DEBUG>0
	printf("Bad initial basis\n");
#endif
      }
      if (internalFactorize(1)) {
	// try once more
      if (internalFactorize(0)) 
	abort();
      }
#if PARTITION_ROW_COPY==1
      int iVector=getAvailableArray();
      abcMatrix_->sortUseful(usefulArray_[iVector]);
      setAvailableArray(iVector);
#endif
    } else {
      lastFlaggedIteration_ = 0;
    }
  }
  // get primal and dual solutions
  numberBlackMarks+=gutsOfSolution(3)/2;
  assert (type!=0||numberIterations_==baseIteration_); // may be wrong
  if (!type) {
    initialSumInfeasibilities_=sumPrimalInfeasibilities_;
    initialNumberInfeasibilities_=numberPrimalInfeasibilities_;
    CoinAbcMemcpy(solutionSaved_+numberTotal_,abcSolution_,numberTotal_);
    CoinAbcMemcpy(internalStatusSaved_+numberTotal_,internalStatus_,numberTotal_);
  }
  double savePrimalInfeasibilities=sumPrimalInfeasibilities_;
  if ((moreSpecialOptions_&16384)!=0&&(moreSpecialOptions_&8192)==0) {
    if (numberIterations_==baseIteration_) {
      // If primal feasible and looks suited to primal go to primal
      if (!sumPrimalInfeasibilities_&&sumDualInfeasibilities_>1.0&&
	  numberRows_*4>numberColumns_) {
	// do primal
	// mark so can perturb
	initialSumInfeasibilities_=-1.0;
	problemStatus_=10;
	return;
      }
    } else if ((numberIterations_>1000&&
	       numberIterations_*10>numberRows_&&numberIterations_*4<numberRows_)||(sumPrimalInfeasibilities_+sumDualInfeasibilities_>
										    1.0e10*(initialSumInfeasibilities_+1000.0)&&numberIterations_>10000&&sumDualInfeasibilities_>1.0)) 
 {
      if (sumPrimalInfeasibilities_+1.0e2*sumDualInfeasibilities_>
	  2000.0*(initialSumInfeasibilities_+100.0)) {
#if ABC_NORMAL_DEBUG>0
	printf ("sump %g sumd %g initial %g\n",sumPrimalInfeasibilities_,sumDualInfeasibilities_,
		initialSumInfeasibilities_);
#endif
	// but only if average gone up as well
	if (sumPrimalInfeasibilities_*initialNumberInfeasibilities_>
	    100.0*(initialSumInfeasibilities_+10.0)*numberPrimalInfeasibilities_) {
#if ABC_NORMAL_DEBUG>0
	  printf("returning to do primal\n");
#endif
	  // do primal
	  problemStatus_=10;
	  CoinAbcMemcpy(abcSolution_,solutionSaved_+numberTotal_,numberTotal_);
	  CoinAbcMemcpy(internalStatus_,internalStatusSaved_+numberTotal_,numberTotal_);
	  // mark so can perturb
	  initialSumInfeasibilities_=-1.0;
	  return;
	}
      }
    }
  } else if ((sumPrimalInfeasibilities_+sumDualInfeasibilities_>
	     1.0e6*(initialSumInfeasibilities_+1000.0)&&(moreSpecialOptions_&8192)==0&&
	     numberIterations_>1000&&
	      numberIterations_*10>numberRows_&&numberIterations_*4<numberRows_)||
	     (sumPrimalInfeasibilities_+sumDualInfeasibilities_>
	      1.0e10*(initialSumInfeasibilities_+1000.0)&&(moreSpecialOptions_&8192)==0&&
	      numberIterations_>10000&&sumDualInfeasibilities_>1.0)) {
    // do primal
    problemStatus_=10;
    CoinAbcMemcpy(abcSolution_,solutionSaved_+numberTotal_,numberTotal_);
    CoinAbcMemcpy(internalStatus_,internalStatusSaved_+numberTotal_,numberTotal_);
    // mark so can perturb
    initialSumInfeasibilities_=-1.0;
    return;
  }
  //double saveDualInfeasibilities=sumDualInfeasibilities_;
  // 0 don't ignore, 1 ignore errors, 2 values pass
  int ignoreStuff=(firstFree_<0&&lastFirstFree_<0) ? 0 : 1;
  if ((stateOfProblem_&VALUES_PASS)!=0)
    ignoreStuff=2;
  {
    double thisObj = rawObjectiveValue();
    double lastObj = abcProgress_.lastObjective(0);
    if ((lastObj > thisObj + 1.0e-3 * CoinMax(fabs(thisObj), fabs(lastObj)) + 1.0e-4||
	 (sumPrimalInfeasibilities_>10.0*lastSumPrimalInfeasibilities+1.0e3&&
	  sumDualInfeasibilities_>10.0*CoinMax(lastSumDualInfeasibilities,1.0)))
	&&!ignoreStuff) {
#if ABC_NORMAL_DEBUG>0
      printf("bad - objective backwards %g %g errors %g %g suminf %g %g\n",lastObj,thisObj,largestDualError_,largestPrimalError_,sumDualInfeasibilities_,sumPrimalInfeasibilities_);
#endif
      numberBlackMarks++;
      if (largestDualError_>1.0e-3||largestPrimalError_>1.0e-3||
	  (sumDualInfeasibilities_>10.0*CoinMax(lastSumDualInfeasibilities,1.0)
	   &&firstFree_<0)) {
#if ABC_NORMAL_DEBUG>0
	printf("backtracking %d iterations - dual error %g primal error %g - sumDualInf %g\n",
	       numberPivots,
	       largestDualError_,largestPrimalError_,sumDualInfeasibilities_);
#endif
	// restore previous basis
	unflagVariables = false;
	// restore weights
	weightsSaved = 4;
	changeMade_++; // say something changed
	// Keep any flagged variables
	for (int i = 0; i < numberTotal_; i++) {
	  if (flagged(i))
	    internalStatusSaved_[i] |= 64; //say flagged
	}
	restoreGoodStatus(type);
	// get correct bounds on all variables
	resetFakeBounds(1);
	numberBlackMarks+=10;
	if (sequenceOut_>=0) {
	  // need to reject something
#if ABC_NORMAL_DEBUG>0
	  if (flagged(sequenceOut_))
	    printf("BAD %x already flagged\n",sequenceOut_);
#endif
	  char x = isColumn(sequenceOut_) ? 'C' : 'R';
	  handler_->message(CLP_SIMPLEX_FLAG, messages_)
	    << x << sequenceWithin(sequenceOut_)
	    << CoinMessageEol;
#if ABC_NORMAL_DEBUG>3
	  printf("flag d\n");
#endif
	  setFlagged(sequenceOut_);
	  lastBadIteration_ = numberIterations_; // say be more cautious
#if ABC_NORMAL_DEBUG>0
	} else {
	  printf("backtracking - no valid sequence out\n");
#endif
	}
	// so can't happen immediately again
	sequenceOut_=-1;
	//abcProgress_.clearBadTimes();
	// Go to safe
	//abcFactorization_->pivotTolerance(0.99);
	if (internalFactorize(1)) {
	  abort();
	}
	moveToBasic();
#if PARTITION_ROW_COPY==1
	int iVector=getAvailableArray();
	abcMatrix_->sortUseful(usefulArray_[iVector]);
	setAvailableArray(iVector);
#endif
	// get primal and dual solutions
	numberBlackMarks+=gutsOfSolution(3);
	//gutsOfSolution(1);
	makeNonFreeVariablesDualFeasible();
      }
    }
  }
  //#define PAN
#ifdef PAN
    if (!type&&(specialOptions_&COIN_CBC_USING_CLP)==0&&firstFree_<0) {
    // first time - set large bad ones superbasic
    int whichArray=getAvailableArray();
    double * COIN_RESTRICT work=usefulArray_[whichArray].denseVector();
    int number=0;
    int * COIN_RESTRICT which=usefulArray_[whichArray].getIndices();
    const double * COIN_RESTRICT reducedCost=abcDj_;
    const double * COIN_RESTRICT lower=abcLower_;
    const double * COIN_RESTRICT upper=abcUpper_;
#if PRINT_PAN
    int nAtUb=0;
#endif
    for (int i=0;i<numberTotal_;i++) {
      Status status = getInternalStatus(i);
      double djValue=reducedCost[i];
      double badValue=0.0;
      if (status==atLowerBound&&djValue<-currentDualTolerance_) {
	double gap=upper[i]-lower[i];
	if (gap>100000000.0)
	  badValue = -djValue*CoinMin(gap,1.0e10);
      } else if (status==atUpperBound&&djValue>currentDualTolerance_) {
	double gap=upper[i]-lower[i];
	if (gap>100000000.0) {
	  badValue = djValue*CoinMin(gap,1.0e10);
#if PRINT_PAN
	  nAtUb++;
#endif
	}
      }
      if (badValue) {
	work[number]=badValue;
	which[number++]=i;
      }
    }
    // for now don't do if primal feasible (probably should go to primal)
    if (!numberPrimalInfeasibilities_) {
      number=0;
#if ABC_NORMAL_DEBUG>1
      printf("XXXX doesn't work if primal feasible - also fix switch to Clp primal\n");
#endif
    }
    if (number&&ignoreStuff!=2) {
      CoinSort_2(work,work+number,which);
      int numberToDo=CoinMin(number,numberRows_/10); // ??
#if PRINT_PAN>1
      CoinSort_2(which,which+numberToDo,work);
      int n=0;
#endif
#if PRINT_PAN
      printf("Panning %d variables (out of %d at lb and %d at ub)\n",numberToDo,number-nAtUb,nAtUb);
#endif
      firstFree_=numberTotal_;
      for (int i=0;i<numberToDo;i++) {
	int iSequence=which[i];
#if PRINT_PAN>1
	if (!n)
	  printf("Pan ");
	printf("%d ",iSequence);
	n++;
	if (n==10) {
	  n=0;
	  printf("\n");
	}
#endif
	setInternalStatus(iSequence,superBasic);
	firstFree_=CoinMin(firstFree_,iSequence);
      }
#if PRINT_PAN>1
      if (n)
	printf("\n");
#endif
      ordinaryVariables_=0;
      CoinAbcMemset0(work,number);
      stateOfProblem_ |= FAKE_SUPERBASIC;
    }
    setAvailableArray(whichArray);
  }
#endif
  if (!sumOfRelaxedPrimalInfeasibilities_&&sumDualInfeasibilities_&&type&&saveType<9
      &&ignoreStuff!=2) {
    // say been feasible
    abcState_ |= CLP_ABC_BEEN_FEASIBLE;
    int numberFake=numberAtFakeBound();
    if (!numberFake) {
      makeNonFreeVariablesDualFeasible();
      numberDualInfeasibilities_=0;
      sumDualInfeasibilities_=0.0;
      // just primal
      gutsOfSolution(2);
    }
  }
  bool computeNewSumInfeasibilities=numberIterations_==baseIteration_;
  if ((!type||(firstFree_<0&&lastFirstFree_>=0))&&ignoreStuff!=2) {
#ifdef PAN
    if ((stateOfProblem_&FAKE_SUPERBASIC)!=0&&firstFree_<0&&lastFirstFree_>=0&&type) {
      // clean up
#if PRINT_PAN
      printf("Pan switch off after %d iterations\n",numberIterations_);
#endif
      computeNewSumInfeasibilities=true;
      ordinaryVariables_=1;
      const double * COIN_RESTRICT reducedCost=abcDj_;
      const double * COIN_RESTRICT lower=abcLower_;
      const double * COIN_RESTRICT upper=abcUpper_;
      for (int i=0;i<numberTotal_;i++) {
	Status status = getInternalStatus(i);
	if (status==superBasic) {
	  double djValue=reducedCost[i];
	  double value=abcSolution_[i];
	  if (value<lower[i]+primalTolerance_) {
	    setInternalStatus(i,atLowerBound);
#if ABC_NORMAL_DEBUG>0
	    if(djValue<-dualTolerance_) 
	      printf("Odd at lb %d dj %g\n",i,djValue);
#endif
	  } else if (value>upper[i]-primalTolerance_) {
	    setInternalStatus(i,atUpperBound);
#if ABC_NORMAL_DEBUG>0
	    if(djValue>dualTolerance_) 
	      printf("Odd at ub %d dj %g\n",i,djValue);
#endif
	  } else {
	    assert (status!=superBasic);
	  }
	}
	if (status==isFree)
	  ordinaryVariables_=0;
      }
      stateOfProblem_ &= ~FAKE_SUPERBASIC;
    }
#endif
    // set up perturbation and dualBound_
    // make dual feasible
    if (firstFree_<0) {
      if (numberFlagged_) {
	numberFlagged_=0;
	for (int iRow = 0; iRow < numberRows_; iRow++) {
	  int iPivot = abcPivotVariable_[iRow];
	  if (flagged(iPivot)) {
	    clearFlagged(iPivot);
	  }
	}
      }
      int numberChanged=resetFakeBounds(0);
#if ABC_NORMAL_DEBUG>0
      if (numberChanged>0)
	printf("Re-setting %d fake bounds %d gone dual infeasible - can expect backward objective\n",
	       numberChanged,numberDualInfeasibilities_);
#endif
      if (numberDualInfeasibilities_||numberChanged!=-1) {
	makeNonFreeVariablesDualFeasible();
	numberDualInfeasibilities_=0;
	sumDualInfeasibilities_=0.0;
	sumOfRelaxedDualInfeasibilities_=0.0;
	// just primal
	gutsOfSolution(2);
	if (!numberIterations_ && sumPrimalInfeasibilities_ >
	    1.0e5*(savePrimalInfeasibilities+1.0e3) &&
	    (moreSpecialOptions_ & (256|8192)) == 0) {
	  // Use primal
	  problemStatus_=10;
	  // mark so can perturb
	  initialSumInfeasibilities_=-1.0;
	  CoinAbcMemcpy(abcSolution_,solutionSaved_,numberTotal_);
	  CoinAbcMemcpy(internalStatus_,internalStatusSaved_,numberTotal_);
	  return;
	}
	// redo
	if(computeNewSumInfeasibilities)
	  initialSumInfeasibilities_=sumPrimalInfeasibilities_;
	//computeObjective();
      } 
    }
    //bounceTolerances(type);
  } else if (numberIterations_>numberRows_&&stateDualColumn_==-2) {
    bounceTolerances(4);
  }
  // If bad accuracy treat as singular
  if ((largestPrimalError_ > 1.0e15 || largestDualError_ > 1.0e15) && numberIterations_>1000) {
    problemStatus_=10; //abort();
#if ABC_NORMAL_DEBUG>0
    printf("Big problems - errors %g %g try primal\n",largestPrimalError_,largestDualError_);
#endif
  } else if (goodAccuracy()&&numberIterations_>lastBadIteration_+200) {
    // Can reduce tolerance
    double newTolerance = CoinMax(0.995 * abcFactorization_->pivotTolerance(), saveData_.pivotTolerance_);
    if (!type)
      newTolerance = saveData_.pivotTolerance_;
    if (newTolerance>abcFactorization_->minimumPivotTolerance())
      abcFactorization_->pivotTolerance(newTolerance);
  }
  bestObjectiveValue_ = CoinMax(bestObjectiveValue_,
				rawObjectiveValue_ - bestPossibleImprovement_);
  // Double check infeasibility if no action
  if (abcProgress_.lastIterationNumber(0) == numberIterations_) {
    if (abcDualRowPivot_->looksOptimal()) {
      // say been feasible
      abcState_ |= CLP_ABC_BEEN_FEASIBLE;
      numberPrimalInfeasibilities_ = 0;
      sumPrimalInfeasibilities_ = 0.0;
    }
  } else {
    double thisObj = rawObjectiveValue_ - bestPossibleImprovement_;
#ifdef CLP_INVESTIGATE
    assert (bestPossibleImprovement_ > -1000.0 && rawObjectiveValue_ > -1.0e100);
    if (bestPossibleImprovement_)
      printf("obj %g add in %g -> %g\n", rawObjectiveValue_, bestPossibleImprovement_,
	     thisObj);
#endif
    double lastObj = abcProgress_.lastObjective(0);
#ifndef NDEBUG
#if ABC_NORMAL_DEBUG>3
    resetFakeBounds(-1);
#endif
#endif
    if (lastObj < thisObj - 1.0e-5 * CoinMax(fabs(thisObj), fabs(lastObj)) - 1.0e-3 && saveType<9) {
      numberTimesOptimal_ = 0;
    }
  }
  // Up tolerance if looks a bit odd
  if (numberIterations_ > CoinMax(1000, numberRows_ >> 4) && (specialOptions_ & 64) != 0) {
    if (sumPrimalInfeasibilities_ && sumPrimalInfeasibilities_ < 1.0e5) {
      int backIteration = abcProgress_.lastIterationNumber(CLP_PROGRESS - 1);
      if (backIteration > 0 && numberIterations_ - backIteration < 9 * CLP_PROGRESS) {
	if (abcFactorization_->pivotTolerance() < 0.9) {
	  // up tolerance
	  abcFactorization_->pivotTolerance(CoinMin(abcFactorization_->pivotTolerance() * 1.05 + 0.02, 0.91));
	  //printf("tol now %g\n",abcFactorization_->pivotTolerance());
	  abcProgress_.clearIterationNumbers();
	}
      }
    }
  }
  // Check if looping
  int loop;
  if (type != 2)
    loop = abcProgress_.looping();
  else
    loop = -1;
  if (abcProgress_.reallyBadTimes() > 10) {
    problemStatus_ = 10; // instead - try other algorithm
#if ABC_NORMAL_DEBUG>3
    printf("returning at %d\n", __LINE__);
#endif
  }
  if (loop >= 0) {
    problemStatus_ = loop; //exit if in loop
    if (!problemStatus_) {
      // declaring victory
      // say been feasible
      abcState_ |= CLP_ABC_BEEN_FEASIBLE;
      numberPrimalInfeasibilities_ = 0;
      sumPrimalInfeasibilities_ = 0.0;
    } else {
      problemStatus_ = 10; // instead - try other algorithm
#if ABC_NORMAL_DEBUG>3
      printf("returning at %d\n", __LINE__);
#endif
    }
    return;
  }
  // really for free variables in
  if((progressFlag_ & 2) != 0) {
    //situationChanged = 2;
  }
  progressFlag_ &= (~3); //reset progress flag
  // mark as having gone optimal if looks like it
  if (!numberPrimalInfeasibilities_&&
      !numberDualInfeasibilities_)
    progressFlag_ |= 8;
  if (handler_->detail(CLP_SIMPLEX_STATUS, messages_) < 100) {
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
  double approximateObjective = rawObjectiveValue_;
  //realDualInfeasibilities = sumDualInfeasibilities_;
  // If we need to carry on cleaning variables
  if (!numberPrimalInfeasibilities_ && (specialOptions_ & 1024) != 0 && CLEAN_FIXED) {
    for (int iRow = 0; iRow < numberRows_; iRow++) {
      int iPivot = abcPivotVariable_[iRow];
      if (!flagged(iPivot) && pivoted(iPivot)) {
	// carry on
	numberPrimalInfeasibilities_ = -1;
	sumOfRelaxedPrimalInfeasibilities_ = 1.0;
	sumPrimalInfeasibilities_ = 1.0;
	break;
      }
    }
  }
  /* If we are primal feasible and any dual infeasibilities are on
     free variables then it is better to go to primal */
  if (!numberPrimalInfeasibilities_ && !numberDualInfeasibilitiesWithoutFree_ &&
      numberDualInfeasibilities_) {
    // say been feasible
    abcState_ |= CLP_ABC_BEEN_FEASIBLE;
    if (type&&ignoreStuff!=2)
      problemStatus_ = 10;
    else
      numberPrimalInfeasibilities_=1;
  }
  // check optimal
  // give code benefit of doubt
  if (sumOfRelaxedDualInfeasibilities_ == 0.0 &&
      sumOfRelaxedPrimalInfeasibilities_ == 0.0&&ignoreStuff!=2) {
    // say been feasible
    abcState_ |= CLP_ABC_BEEN_FEASIBLE;
    // say optimal (with these bounds etc)
    numberDualInfeasibilities_ = 0;
    sumDualInfeasibilities_ = 0.0;
    numberPrimalInfeasibilities_ = 0;
    sumPrimalInfeasibilities_ = 0.0;
  } else if (sumOfRelaxedDualInfeasibilities_>1.0e-3&&firstFree_<0&&ignoreStuff!=2
	     &&numberIterations_>lastCleaned_+10) {
#if ABC_NORMAL_DEBUG>0
    printf("Making dual feasible\n");
#endif
    makeNonFreeVariablesDualFeasible(true);
    lastCleaned_=numberIterations_;
    numberDualInfeasibilities_=0;
    sumDualInfeasibilities_=0.0;
    // just primal
    gutsOfSolution(2);
  }
  // see if cutoff reached
  double limit = 0.0;
  getDblParam(ClpDualObjectiveLimit, limit);
  if (primalFeasible()&&ignoreStuff!=2) {
    // may be optimal - or may be bounds are wrong
    int numberChangedBounds = changeBounds(0, changeCost);
    if (numberChangedBounds <= 0 && !numberDualInfeasibilities_) {
      //looks optimal - do we need to reset tolerance
      if (lastCleaned_ < numberIterations_ && /*numberTimesOptimal_*/ stateDualColumn_ < 4 &&
	  (numberChanged_ || (specialOptions_ & 4096) == 0)) {
#if CLP_CAN_HAVE_ZERO_OBJ
	if ((specialOptions_&2097152)==0) {
#endif
	  numberTimesOptimal_++;
	  if (stateDualColumn_==-2) {
#if ABC_NORMAL_DEBUG>0
	    printf("Going straight from state -2 to 0\n");
#endif
	    stateDualColumn_=-1;
	  }
	  changeMade_++; // say something changed
	  if (numberTimesOptimal_ == 1) {
	    if (fabs(currentDualTolerance_-dualTolerance_)>1.0e-12) {
#if ABC_NORMAL_DEBUG>0
	      printf("changing dual tolerance from %g to %g (numberTimesOptimal_==%d)\n",
		     currentDualTolerance_,dualTolerance_,numberTimesOptimal_);
#endif
	      currentDualTolerance_ = dualTolerance_;
	    }
	  } else {
	    if (numberTimesOptimal_ == 2) {
	      // better to have small tolerance even if slower
	      abcFactorization_->zeroTolerance(CoinMin(abcFactorization_->zeroTolerance(), 1.0e-15));
	    }
#if ABC_NORMAL_DEBUG>0
	    printf("changing dual tolerance from %g to %g (numberTimesOptimal_==%d)\n",
		   currentDualTolerance_,dualTolerance_*pow(2.0,numberTimesOptimal_-1),numberTimesOptimal_);
#endif
	    currentDualTolerance_ = dualTolerance_ * pow(2.0, numberTimesOptimal_ - 1);
	  }
	  if (numberChanged_>0) {
	    handler_->message(CLP_DUAL_ORIGINAL, messages_)
	      << CoinMessageEol;
	    //realDualInfeasibilities = sumDualInfeasibilities_; 
	  } else if (numberChangedBounds<-1) {
	    // bounds were flipped
	    //gutsOfSolution(2);
	  }
	  if (stateDualColumn_>=0)
	    stateDualColumn_++;
#ifndef TRY_ABC_GUS
	  int returnCode=bounceTolerances(1);
	  if (returnCode)
#endif // always go to primal
	    problemStatus_=10;
	  lastCleaned_ = numberIterations_;
#if CLP_CAN_HAVE_ZERO_OBJ
	} else {
	  // no cost - skip checks
	  problemStatus_=0;
	}
#endif
      } else {
	problemStatus_ = 0; // optimal
	if (lastCleaned_ < numberIterations_ && numberChanged_) {
	  handler_->message(CLP_SIMPLEX_GIVINGUP, messages_)
	    << CoinMessageEol;
	}
      }
    } else if (numberChangedBounds<-1) {
      // some flipped
      bounceTolerances(2);
#if ABC_NORMAL_DEBUG>0
      printf("numberChangedBounds %d numberAtFakeBound %d at line %d in file %s\n",
	     numberChangedBounds,numberAtFakeBound(),__LINE__,__FILE__);
#endif
    } else if (numberAtFakeBound()) {
      handler_->message(CLP_DUAL_BOUNDS, messages_)
	<< currentDualBound_
	<< CoinMessageEol;
#if ABC_NORMAL_DEBUG>0
      printf("changing bounds\n");
      printf("numberChangedBounds %d numberAtFakeBound %d at line %d in file %s\n",
	     numberChangedBounds,numberAtFakeBound(),__LINE__,__FILE__);
#endif
      if (!numberDualInfeasibilities_&&!numberPrimalInfeasibilities_) {
	// check unbounded
	// find a variable with bad dj
	int iChosen = -1;
	if ((specialOptions_ & 0x03000000) == 0) {
	  double largest = 100.0 * primalTolerance_;
	  for (int iSequence = 0; iSequence < numberTotal_; iSequence++) {
	    double djValue = abcDj_[iSequence];
	    double originalLo = lowerSaved_[iSequence];
	    double originalUp = upperSaved_[iSequence];
	    if (fabs(djValue) > fabs(largest)) {
	      if (getInternalStatus(iSequence) != basic) {
		if (djValue > 0 && originalLo < -1.0e20) {
		  if (djValue > fabs(largest)) {
		    largest = djValue;
		    iChosen = iSequence;
		  }
		} else if (djValue < 0 && originalUp > 1.0e20) {
		  if (-djValue > fabs(largest)) {
		    largest = djValue;
		    iChosen = iSequence;
		  }
		}
	      }
	    }
	  }
	}
	if (iChosen >= 0) {
	  int iVector=getAvailableArray();
	  unpack(usefulArray_[iVector],iChosen);
	  //double changeCost;
	  problemStatus_ = checkUnbounded(usefulArray_[iVector], 0.0/*changeCost*/);
	  usefulArray_[iVector].clear();
	  setAvailableArray(iVector);
	}
      }
      gutsOfSolution(3);
      //bounceTolerances(2);
      //realDualInfeasibilities = sumDualInfeasibilities_; 
    }
    // unflag all variables (we may want to wait a bit?)
    if (unflagVariables) {
      int numberFlagged = numberFlagged_;
      numberFlagged_=0;
      for (int iRow = 0; iRow < numberRows_; iRow++) {
	int iPivot = abcPivotVariable_[iRow];
	if (flagged(iPivot)) {
	  clearFlagged(iPivot);
	}
      }
#if ABC_NORMAL_DEBUG>3
      if (numberFlagged) {
	printf("unflagging %d variables - tentativeStatus %d probStat %d ninf %d nopt %d\n", numberFlagged, tentativeStatus,
	       problemStatus_, numberPrimalInfeasibilities_,
	       numberTimesOptimal_);
      }
#endif
      unflagVariables = numberFlagged > 0;
      if (numberFlagged && !numberPivots) {
	/* looks like trouble as we have not done any iterations.
	   Try changing pivot tolerance then give it a few goes and give up */
	if (abcFactorization_->pivotTolerance() < 0.9) {
	  abcFactorization_->pivotTolerance(0.99);
	  problemStatus_ = -1;
	} else if (numberTimesOptimal_ < 3) {
	  numberTimesOptimal_++;
	  problemStatus_ = -1;
	} else {
	  unflagVariables = false;
	  //secondaryStatus_ = 1; // and say probably infeasible
	  if ((moreSpecialOptions_ & 256) == 0||(abcState_&CLP_ABC_BEEN_FEASIBLE)!=0) {
	    // try primal
	    problemStatus_ = 10;
	  } else {
	    // almost certainly infeasible
	    problemStatus_ = 1;
	  }
#if ABC_NORMAL_DEBUG>3
	  printf("returning at %d\n", __LINE__);
#endif
	}
      }
    }
  }
  if (problemStatus_ < 0) {
    saveGoodStatus();
    if (weightsSaved) {
      // restore weights (if saved) - also recompute infeasibility list
      abcDualRowPivot_->saveWeights(this, weightsSaved);
    } else {
      abcDualRowPivot_->recomputeInfeasibilities();
    }
  }
  // see if cutoff reached
  checkCutoff(false);
  // If we are in trouble and in branch and bound give up
  if ((specialOptions_ & 1024) != 0) {
    int looksBad = 0;
    if (largestPrimalError_ * largestDualError_ > 1.0e2) {
      looksBad = 1;
    } else if (largestPrimalError_ > 1.0e-2
	       && rawObjectiveValue_ > CoinMin(1.0e15, 1.0e3 * limit)) {
      looksBad = 2;
    }
    if (looksBad) {
      if (abcFactorization_->pivotTolerance() < 0.9) {
	// up tolerance
	abcFactorization_->pivotTolerance(CoinMin(abcFactorization_->pivotTolerance() * 1.05 + 0.02, 0.91));
      } else if (numberIterations_ > 10000) {
#if ABC_NORMAL_DEBUG>0
	if (handler_->logLevel() > 2)
	  printf("bad dual - saying infeasible %d\n", looksBad);
#endif
	problemStatus_ = 1;
	secondaryStatus_ = 1; // and say was on cutoff
      } else if (largestPrimalError_ > 1.0e5) {
#if ABC_NORMAL_DEBUG>0
	if (handler_->logLevel() > 2)
	  printf("bad dual - going to primal %d %g\n", looksBad, largestPrimalError_);
#endif
	allSlackBasis();
	problemStatus_ = 10;
      }
    }
  }
  //if (problemStatus_ < 0 && !changeMade_) {
  //problemStatus_ = 4; // unknown
  //}
  lastGoodIteration_ = numberIterations_;
  if (numberIterations_ > lastBadIteration_ + 200) {
    moreSpecialOptions_ &= ~16; // clear check accuracy flag
    if (numberFlagged_) {
      numberFlagged_=0;
      for (int iRow = 0; iRow < numberRows_; iRow++) {
	int iPivot = abcPivotVariable_[iRow];
	if (flagged(iPivot)) {
	  clearFlagged(iPivot);
	}
      }
    }
  }
  if (problemStatus_ < 0&&ignoreStuff!=2) {
    //sumDualInfeasibilities_ = realDualInfeasibilities; // back to say be careful
    if (sumDualInfeasibilities_) {
      numberDualInfeasibilities_ = 1;
    } else if (!numberPrimalInfeasibilities_) {
      // Looks good
      problemStatus_=0;
    }
  }
  double thisObj = abcProgress_.lastObjective(0);
  double lastObj = abcProgress_.lastObjective(1);
  if (lastObj > thisObj + 1.0e-4 * CoinMax(fabs(thisObj), fabs(lastObj)) + 1.0e-4&&
      lastFirstFree_<0) {
    //printf("BAD - objective backwards\n");
    //bounceTolerances(3);
    numberBlackMarks+=3;
  }
  if (numberBlackMarks>0&&numberIterations_>baseIteration_) {
    // be very careful
    int maxFactor = abcFactorization_->maximumPivots();
    if (maxFactor > 10) {
      if ((stateOfProblem_&PESSIMISTIC)==0||true) {
	if (largestDualError_>10.0*CoinMax(lastDualError_,1.0e-6)
	    ||largestPrimalError_>10.0*CoinMax(lastPrimalError_,1.0e-6))
	  maxFactor = CoinMin(maxFactor,20);
	forceFactorization_ = CoinMax(1, (maxFactor >> 1));
      } else if (numberBlackMarks>2) {
	forceFactorization_ = CoinMax(1, (forceFactorization_ >> 1));
      }
    }
    stateOfProblem_ |= PESSIMISTIC;
    if (numberBlackMarks>=10)
      forceFactorization_ = 1;
  }
  lastPrimalError_=largestPrimalError_;
  lastDualError_=largestDualError_;
  lastFirstFree_=firstFree_;
  if (ignoreStuff==2) {
    // in values pass
    lastFirstFree_=-1;
    // firstFree_=-1;
  }
  if (alphaAccuracy_ > 0.0)
    alphaAccuracy_ = 1.0;
  // If we are stopping - use plausible objective
  // Maybe only in fast dual
  if (problemStatus_ > 2)
    rawObjectiveValue_ = approximateObjective;
  if (problemStatus_ == 1 && (progressFlag_&8) != 0 &&
      fabs(rawObjectiveValue_) > 1.0e10 )
    problemStatus_ = 10; // infeasible - but has looked feasible
  checkMoveBack(true);
#if 0
  {
    double * array = usefulArray_[arrayForTableauRow_].denseVector();
    for (int iPivot = 0; iPivot < numberRows_; iPivot++) {
      int iSequence = abcPivotVariable_[iPivot];
      unpack(usefulArray_[arrayForTableauRow_],iSequence);
      abcFactorization_->updateColumn(usefulArray_[arrayForTableauRow_]);
      assert (fabs(array[iPivot] - 1.0) < 1.0e-4);
      array[iPivot] = 0.0;
      for (int i = 0; i < numberRows_; i++)
	assert (fabs(array[i]) < 1.0e-4);
      usefulArray_[arrayForTableauRow_].clear();
    }
  }
#endif
#if 0
  {
    // save status, cost and solution
    unsigned char * saveStatus = CoinCopyOfArray(internalStatus_,numberTotal_);
    double * saveSolution = CoinCopyOfArray(abcSolution_,numberTotal_);
    double * saveCost = CoinCopyOfArray(abcCost_,numberTotal_);
    printf("TestA sumDual %g sumPrimal %g\n",sumDualInfeasibilities_,sumPrimalInfeasibilities_);
    if (perturbation_>100)
      CoinAbcMemcpy(abcCost_,costSaved_,numberTotal_);
    else
      CoinAbcMemcpy(abcCost_,abcPerturbation_,numberTotal_);
    moveToBasic();
    gutsOfSolution(3);
    for (int iSequence = 0; iSequence < numberTotal_; iSequence++) {
      bool changed=fabs(saveCost[iSequence]-abcCost_[iSequence])>1.0e-10;
      bool bad=false;
      int iStatus=internalStatus_[iSequence]&7;
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
		bad=true;
	      }
	    }
	  } else {
	    // free
	    value=fabs(value);
	    if (value > currentDualTolerance_) {
	      bad=true;
	    }
	  }
	} else if (distanceDown > primalTolerance_) {
	  // should not be positive
	  if (value > 0.0) {
	    if (value > currentDualTolerance_) {
	      bad=true;
	    }
	  }
	}
      }
      if (changed||bad)
	printf("seq %d status %d current cost %g original %g dj %g %s\n",
	       iSequence,iStatus,saveCost[iSequence],abcCost_[iSequence],
	       abcDj_[iSequence],bad ? "(bad)" : "");
    }
    printf("TestB sumDual %g sumPrimal %g\n",sumDualInfeasibilities_,sumPrimalInfeasibilities_);
    if (numberDualInfeasibilities_) {
      makeNonFreeVariablesDualFeasible();
      moveToBasic();
      gutsOfSolution(3);
      printf("TestC sumDual %g sumPrimal %g\n",sumDualInfeasibilities_,sumPrimalInfeasibilities_);
    }
    CoinAbcMemcpy(internalStatus_,saveStatus,numberTotal_);
    CoinAbcMemcpy(abcSolution_,saveSolution,numberTotal_);
    CoinAbcMemcpy(abcCost_,saveCost,numberTotal_);
    delete [] saveStatus;
    delete [] saveSolution;
    delete [] saveCost;
    moveToBasic();
    gutsOfSolution(3);
    printf("TestD sumDual %g sumPrimal %g\n",sumDualInfeasibilities_,sumPrimalInfeasibilities_);
  }
#endif
#if 0
  {
    const double multiplier[] = { 1.0, -1.0}; // sign?????????????
    double dualT = - currentDualTolerance_;
    double largestCostDifference=0.1*currentDualTolerance_;
    int numberCostDifference=0;
    double largestBadDj=0.0;
    int numberBadDj=0;
    double sumBadDj=0.0;
    double sumCanMoveBad=0.0;
    int numberCanMoveBad=0;
    double sumCanMoveOthers=0.0;
    int numberCanMoveOthers=0;
    for (int iSequence = 0; iSequence < numberTotal_; iSequence++) {
      double costDifference=abcPerturbation_[iSequence]-abcCost_[iSequence];
      if (fabs(costDifference)>0.1*currentDualTolerance_) {
	if (fabs(costDifference)>largestCostDifference) 
	  largestCostDifference=fabs(costDifference);
	numberCostDifference++;
      }
      unsigned char iStatus = internalStatus_[iSequence] & 7;
      if (iStatus<4) {
	double mult = multiplier[iStatus];
	double dj = abcDj_[iSequence] * mult;
	if (dj < dualT) {
	  sumBadDj+=dualT-dj;
	  numberBadDj++;
	  if (dualT-dj>largestBadDj)
	    largestBadDj=dualT-dj;
	  if (costDifference*mult>0.0) {
	    sumCanMoveBad+=costDifference*mult;
	    numberCanMoveBad++;
	    //dj+=costDifference*mult;
	    abcDj_[iSequence]+=costDifference;
	    abcCost_[iSequence]=abcPerturbation_[iSequence];
	  }
	} else if (costDifference*mult>0.0) {
	  sumCanMoveOthers+=costDifference*mult;
	  numberCanMoveOthers++;
	  //dj+=costDifference*mult;
	  abcDj_[iSequence]+=costDifference;
	  abcCost_[iSequence]=abcPerturbation_[iSequence];
	}
      }
    }
    if (numberCostDifference+numberBadDj+numberCanMoveBad+numberCanMoveOthers) {
      printf("largest diff %g (%d), largest bad dj %g (%d - sum %g), can move bad %g (%d), good %g (%d) - %d at fake bound\n",largestCostDifference,numberCostDifference,largestBadDj,
	     numberBadDj, sumBadDj,sumCanMoveBad=0.0,numberCanMoveBad,
	     sumCanMoveOthers,numberCanMoveOthers,numberAtFakeBound());
    }
  }
#endif
}
// see if cutoff reached
bool
AbcSimplexDual::checkCutoff(bool computeObjective)
{
  double objValue=COIN_DBL_MAX;
  if (computeObjective)
    objValue = computeInternalObjectiveValue();
  else
    objValue = minimizationObjectiveValue();
  double limit = 0.0;
  getDblParam(ClpDualObjectiveLimit, limit);
  // Looks as if numberAtFakeBound() being computed too often
  if(fabs(limit) < 1.0e30 && objValue >
     limit && !numberAtFakeBound()) {
    bool looksInfeasible = !numberDualInfeasibilities_;
    if (objValue > limit + fabs(0.1 * limit) + 1.0e2 * sumDualInfeasibilities_ + 1.0e4 &&
	sumDualInfeasibilities_ < largestDualError_ && numberIterations_ > 0.5 * numberRows_ + 1000)
      looksInfeasible = true;
    if (looksInfeasible&&!computeObjective)
      objValue = computeInternalObjectiveValue();
    if (looksInfeasible) {
      // Even if not perturbed internal costs may have changed
      // be careful
      if(objValue > limit) {
	problemStatus_ = 1;
	secondaryStatus_ = 1; // and say was on cutoff
	return true;
      }
    }
  }
  return false;
}
#define CLEANUP_DJS 0
/* While updateDualsInDual sees what effect is of flip
   this does actual flipping.
   If change >0.0 then value in array >0.0 => from lower to upper
   returns 3 if skip this iteration and re-factorize
*/
int
AbcSimplexDual::flipBounds()
{
  CoinIndexedVector & array = usefulArray_[arrayForFlipBounds_];
  CoinIndexedVector & output = usefulArray_[arrayForFlipRhs_];
  output.clear();
  int numberFlipped=array.getNumElements();
  if (numberFlipped) {
#if CLP_CAN_HAVE_ZERO_OBJ>1
    if ((specialOptions_&2097152)==0) {
#endif
      // do actual flips
      // do in parallel once got spare space in factorization
#if ABC_PARALLEL==2
#endif
      int number=array.getNumElements();
      const int *  COIN_RESTRICT which=array.getIndices();
      double *  COIN_RESTRICT work = array.denseVector();
      
      double *  COIN_RESTRICT solution = abcSolution_;
      const double *  COIN_RESTRICT lower = abcLower_;
      const double *  COIN_RESTRICT upper = abcUpper_;
#if CLEANUP_DJS
      double *  COIN_RESTRICT cost = abcCost_;
      double *  COIN_RESTRICT dj = abcDj_;
      // see if we can clean up djs
      // may have perturbation
      const double * COIN_RESTRICT originalCost = abcPerturbation_;
#else
      const double *  COIN_RESTRICT cost = abcCost_;
      const double *  COIN_RESTRICT dj = abcDj_;
#endif
      const double multiplier[] = { 1.0, -1.0};
      for (int i = 0; i < number; i++) {
	double value=work[i]*theta_;
	work[i]=0.0;
	int iSequence = which[i];
	unsigned char iStatus = internalStatus_[iSequence]&3;
	assert ((internalStatus_[iSequence]&7)==0||
		(internalStatus_[iSequence]&7)==1);
	double mult = multiplier[iStatus];
	double djValue = dj[iSequence] * mult-value;
	//double perturbedTolerance = abcPerturbation_[iSequence];
	if (djValue<-currentDualTolerance_) {
	  // might just happen - if so just skip
	  assert (iSequence!=sequenceIn_);
	  double movement=(upper[iSequence]-lower[iSequence])*mult;
	  if (iStatus==0) {
	    // at lower bound
	    // to upper bound
	    setInternalStatus(iSequence, atUpperBound);
	    solution[iSequence] = upper[iSequence];
#if CLEANUP_DJS
	    if (cost[iSequence]<originalCost[iSequence]) {
	      double difference = CoinMax(cost[iSequence]-originalCost[iSequence],djValue);
	      dj[iSequence]-=difference;
	      cost[iSequence]-=difference;
	    }
#endif
	  } else {
	    // at upper bound
	    // to lower bound
	    setInternalStatus(iSequence, atLowerBound);
	    solution[iSequence] = lower[iSequence];
#if CLEANUP_DJS
	    if (cost[iSequence]>originalCost[iSequence]) {
	      double difference = CoinMin(cost[iSequence]-originalCost[iSequence],-djValue);
	      dj[iSequence]-=difference;
	      cost[iSequence]-=difference;
	    }
#endif
	  }
	  abcMatrix_->add(output, iSequence, movement);
	  objectiveChange_ += cost[iSequence]*movement;
	}
      }
      array.setNumElements(0);
      // double check something flipped
      if (output.getNumElements()) {
#if ABC_PARALLEL==0
	abcFactorization_->updateColumn(output);
#else
	assert (FACTOR_CPU>2);
	abcFactorization_->updateColumnCpu(output,1);
#endif
	abcDualRowPivot_->updatePrimalSolution(output,1.0);
      }
#if CLP_CAN_HAVE_ZERO_OBJ>1
    } else {
      clearArrays(3);
    }
#endif
  }
  // recompute dualOut_
  valueOut_ = solutionBasic_[pivotRow_];
  if (directionOut_ < 0) {
    dualOut_ = valueOut_ - upperOut_;
  } else {
    dualOut_ = lowerOut_ - valueOut_;
  }
#if 0
  // amount primal will move
  movement_ = -dualOut_ * directionOut_ / alpha_;
  double movementOld = oldDualOut * directionOut_ / alpha_;
  // so objective should increase by fabs(dj)*movement_
  // but we already have objective change - so check will be good
  if (objectiveChange_ + fabs(movementOld * dualIn_) < -CoinMax(1.0e-5, 1.0e-12 * fabs(rawObjectiveValue_))) {
#if ABC_NORMAL_DEBUG>3
    if (handler_->logLevel() & 32)
      printf("movement %g, movementOld %g swap change %g, rest %g  * %g\n",
	     objectiveChange_ + fabs(movement_ * dualIn_),
	     movementOld,objectiveChange_, movement_, dualIn_);
#endif
  }
  if (0) {
    if(abcFactorization_->pivots()) {
      // going backwards - factorize
      problemStatus_ = -2; // factorize now
      stateOfIteration_=2;
    }
  }
#endif
  return numberFlipped;
}
/* Undo a flip
 */
void 
AbcSimplexDual::flipBack(int number)
{
  CoinIndexedVector & array = usefulArray_[arrayForFlipBounds_];
  const int *  COIN_RESTRICT which=array.getIndices();
  double *  COIN_RESTRICT solution = abcSolution_;
  const double *  COIN_RESTRICT lower = abcLower_;
  const double *  COIN_RESTRICT upper = abcUpper_;
  for (int i = 0; i < number; i++) {
    int iSequence = which[i];
    unsigned char iStatus = internalStatus_[iSequence]&3;
    assert ((internalStatus_[iSequence]&7)==0||
	    (internalStatus_[iSequence]&7)==1);
    if (iStatus==0) {
      // at lower bound
      // to upper bound
      setInternalStatus(iSequence, atUpperBound);
      solution[iSequence] = upper[iSequence];
    } else {
      // at upper bound
      // to lower bound
      setInternalStatus(iSequence, atLowerBound);
      solution[iSequence] = lower[iSequence];
    }
  }
}
// Restores bound to original bound
void
AbcSimplexDual::originalBound( int iSequence)
{
  if (getFakeBound(iSequence) != noFake) {
    numberFake_--;;
    setFakeBound(iSequence, noFake);
    abcLower_[iSequence] = lowerSaved_[iSequence];
    abcUpper_[iSequence] = upperSaved_[iSequence];
  }
}
/* As changeBounds but just changes new bounds for a single variable.
   Returns true if change */
bool
AbcSimplexDual::changeBound( int iSequence)
{
  // old values
  double oldLower = abcLower_[iSequence];
  double oldUpper = abcUpper_[iSequence];
  double value = abcSolution_[iSequence];
  bool modified = false;
  // original values
  double lowerValue = lowerSaved_[iSequence];
  double upperValue = upperSaved_[iSequence];
  setFakeBound(iSequence,noFake);
  if (value == oldLower) {
    if (upperValue > oldLower + currentDualBound_) {
      abcUpper_[iSequence] = oldLower + currentDualBound_;
      setFakeBound(iSequence, upperFake);
      assert (abcLower_[iSequence]==lowerSaved_[iSequence]);
      assert (abcUpper_[iSequence]<upperSaved_[iSequence]);
      modified = true;
      numberFake_++;
    }
  } else if (value == oldUpper) {
    if (lowerValue < oldUpper - currentDualBound_) {
      abcLower_[iSequence] = oldUpper - currentDualBound_;
      setFakeBound(iSequence, lowerFake);
      assert (abcLower_[iSequence]>lowerSaved_[iSequence]);
      assert (abcUpper_[iSequence]==upperSaved_[iSequence]);
      modified = true;
      numberFake_++;
    }
  } else {
    assert(value == oldLower || value == oldUpper);
  }
  return modified;
}
/* Checks number of variables at fake bounds.  This is used by fastDual
   so can exit gracefully before end */
int
AbcSimplexDual::numberAtFakeBound()
{
  int numberFake = 0;

  if (numberFake_) {
    for (int iSequence = 0; iSequence < numberTotal_; iSequence++) {
      FakeBound bound = getFakeBound(iSequence);
      switch(getInternalStatus(iSequence)) {
	
      case basic:
	break;
      case isFree:
      case superBasic:
      case AbcSimplex::isFixed:
	//setFakeBound (iSequence, noFake);
	break;
      case atUpperBound:
	if (bound == upperFake || bound == bothFake)
	  numberFake++;
	break;
      case atLowerBound:
	if (bound == lowerFake || bound == bothFake)
	  numberFake++;
	break;
      }
    }
  }
  return numberFake;
}
int
AbcSimplexDual::resetFakeBounds(int type)
{
  if (type == 0||type==1) {
    // put back original bounds and then check
    //CoinAbcMemcpy(abcLower_,lowerSaved_,numberTotal_);
    //CoinAbcMemcpy(abcUpper_,upperSaved_,numberTotal_);
    double dummyChangeCost = 0.0;
    return changeBounds(3+type, dummyChangeCost);
  } else if (type < 0) {
#ifndef NDEBUG
    // just check
    int iSequence;
    int nFake = 0;
    int nErrors = 0;
    int nSuperBasic = 0;
    int nWarnings = 0;
    for (iSequence = 0; iSequence < numberTotal_; iSequence++) {
      FakeBound fakeStatus = getFakeBound(iSequence);
      Status status = getInternalStatus(iSequence);
      bool isFake = false;
#ifdef CLP_INVESTIGATE
      char RC = 'C';
      int jSequence = iSequence;
      if (jSequence >= numberColumns_) {
	RC = 'R';
	jSequence -= numberColumns_;
      }
#endif
      double lowerValue = lowerSaved_[iSequence];
      double upperValue = upperSaved_[iSequence];
      double value = abcSolution_[iSequence];
      CoinRelFltEq equal;
      if (status == atUpperBound ||
	  status == atLowerBound) {
	if (fakeStatus == AbcSimplexDual::upperFake) {
	  if(!equal(abcUpper_[iSequence], (lowerValue + currentDualBound_)) ||
	     !(equal(abcUpper_[iSequence], value) ||
	       equal(abcLower_[iSequence], value))) {
	    nErrors++;
#ifdef CLP_INVESTIGATE
	    printf("** upperFake %c%d %g <= %g <= %g true %g, %g\n",
		   RC, jSequence, abcLower_[iSequence], abcSolution_[iSequence],
		   abcUpper_[iSequence], lowerValue, upperValue);
#endif
	  }
	  isFake = true;;
	} else if (fakeStatus == AbcSimplexDual::lowerFake) {
	  if(!equal(abcLower_[iSequence], (upperValue - currentDualBound_)) ||
	     !(equal(abcUpper_[iSequence], value) ||
	       equal(abcLower_[iSequence], value))) {
	    nErrors++;
#ifdef CLP_INVESTIGATE
	    printf("** lowerFake %c%d %g <= %g <= %g true %g, %g\n",
		   RC, jSequence, abcLower_[iSequence], abcSolution_[iSequence],
		   abcUpper_[iSequence], lowerValue, upperValue);
#endif
	  }
	  isFake = true;;
	} else if (fakeStatus == AbcSimplexDual::bothFake) {
	  nWarnings++;
#ifdef CLP_INVESTIGATE
	  printf("** %d at bothFake?\n", iSequence);
#endif
	} else if (abcUpper_[iSequence] - abcLower_[iSequence] > 2.0 * currentDualBound_) {
	  nErrors++;
#ifdef CLP_INVESTIGATE
	  printf("** noFake! %c%d %g <= %g <= %g true %g, %g\n",
		 RC, jSequence, abcLower_[iSequence], abcSolution_[iSequence],
		 abcUpper_[iSequence], lowerValue, upperValue);
#endif
	}
      } else if (status == superBasic || status == isFree) {
	nSuperBasic++;
	//printf("** free or superbasic %c%d %g <= %g <= %g true %g, %g - status %d\n",
	//     RC,jSequence,abcLower_[iSequence],abcSolution_[iSequence],
	//     abcUpper_[iSequence],lowerValue,upperValue,status);
      } else if (status == basic) {
	bool odd = false;
	if (!equal(abcLower_[iSequence], lowerValue))
	  odd = true;
	if (!equal(abcUpper_[iSequence], upperValue))
	  odd = true;
	if (odd) {
#ifdef CLP_INVESTIGATE
	  printf("** basic %c%d %g <= %g <= %g true %g, %g\n",
		 RC, jSequence, abcLower_[iSequence], abcSolution_[iSequence],
		 abcUpper_[iSequence], lowerValue, upperValue);
#endif
	  nWarnings++;
	}
      } else if (status == isFixed) {
	if (!equal(abcUpper_[iSequence], abcLower_[iSequence])) {
	  nErrors++;
#ifdef CLP_INVESTIGATE
	  printf("** fixed! %c%d %g <= %g <= %g true %g, %g\n",
		 RC, jSequence, abcLower_[iSequence], abcSolution_[iSequence],
		 abcUpper_[iSequence], lowerValue, upperValue);
#endif
	}
      }
      if (isFake) {
	nFake++;
      } else {
	if (fakeStatus != AbcSimplexDual::noFake) {
	  nErrors++;
#ifdef CLP_INVESTIGATE
	  printf("** bad fake status %c%d %d\n",
		 RC, jSequence, fakeStatus);
#endif
	}
      }
    }
    if (nFake != numberFake_) {
#ifdef CLP_INVESTIGATE
      printf("nfake %d numberFake %d\n", nFake, numberFake_);
#endif
      nErrors++;
    }
    if (nErrors || type <= -1000) {
#ifdef CLP_INVESTIGATE
      printf("%d errors, %d warnings, %d free/superbasic, %d fake\n",
	     nErrors, nWarnings, nSuperBasic, numberFake_);
      printf("currentDualBound %g\n",
	     currentDualBound_);
#endif
      if (type <= -1000) {
	iSequence = -type;
	iSequence -= 1000;
#ifdef CLP_INVESTIGATE
	char RC = 'C';
	int jSequence = iSequence;
	if (jSequence >= numberColumns_) {
	  RC = 'R';
	  jSequence -= numberColumns_;
	}
	double lowerValue = lowerSaved_[iSequence];
	double upperValue = upperSaved_[iSequence];
	printf("*** movement>1.0e30 for  %c%d %g <= %g <= %g true %g, %g - status %d\n",
	       RC, jSequence, abcLower_[iSequence], abcSolution_[iSequence],
	       abcUpper_[iSequence], lowerValue, upperValue, internalStatus_[iSequence]);
#endif
	assert (nErrors); // should have been picked up
      }
      assert (!nErrors);
    }
#endif
  } else {
    CoinAbcMemcpy(abcLower_,lowerSaved_,numberTotal_);
    CoinAbcMemcpy(abcUpper_,upperSaved_,numberTotal_);
    // reset using status
    numberFake_ = 0;
    for (int iSequence = 0; iSequence < numberTotal_; iSequence++) {
      FakeBound fakeStatus = getFakeBound(iSequence);
      if (fakeStatus != AbcSimplexDual::noFake) {
	Status status = getInternalStatus(iSequence);
	if (status == basic) {
	  setFakeBound(iSequence, AbcSimplexDual::noFake);
	  continue;
	}
	double lowerValue = abcLower_[iSequence];
	double upperValue = abcUpper_[iSequence];
	double value = abcSolution_[iSequence];
	numberFake_++;
	if (fakeStatus == AbcSimplexDual::upperFake) {
	  abcUpper_[iSequence] = lowerValue + currentDualBound_;
	  if (status == AbcSimplex::atLowerBound) {
	    abcSolution_[iSequence] = lowerValue;
	  } else if (status == AbcSimplex::atUpperBound) {
	    abcSolution_[iSequence] = abcUpper_[iSequence];
	  } else {
	    abort();
	  }
	} else if (fakeStatus == AbcSimplexDual::lowerFake) {
	  abcLower_[iSequence] = upperValue - currentDualBound_;
	  if (status == AbcSimplex::atLowerBound) {
	    abcSolution_[iSequence] = abcLower_[iSequence];
	  } else if (status == AbcSimplex::atUpperBound) {
	    abcSolution_[iSequence] = upperValue;
	  } else {
	    abort();
	  }
	} else {
	  assert (fakeStatus == AbcSimplexDual::bothFake);
	  if (status == AbcSimplex::atLowerBound) {
	    abcLower_[iSequence] = value;
	    abcUpper_[iSequence] = value + currentDualBound_;
	  } else if (status == AbcSimplex::atUpperBound) {
	    abcUpper_[iSequence] = value;
	    abcLower_[iSequence] = value - currentDualBound_;
	  } else if (status == AbcSimplex::isFree ||
		     status == AbcSimplex::superBasic) {
	    abcLower_[iSequence] = value - 0.5 * currentDualBound_;
	    abcUpper_[iSequence] = value + 0.5 * currentDualBound_;
	  } else {
	    abort();
	  }
	}
      }
    }
  }
  return -1;
}
// maybe could JUST check against perturbationArray
void
AbcSimplex::checkDualSolutionPlusFake()
{
  
  sumDualInfeasibilities_ = 0.0;
  numberDualInfeasibilities_ = 0;
  sumFakeInfeasibilities_=0.0;
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
  //rawObjectiveValue_ -=sumNonBasicCosts_;
  sumNonBasicCosts_ = 0.0;
  const double *  COIN_RESTRICT abcLower = abcLower_;
  const double *  COIN_RESTRICT abcUpper = abcUpper_;
  const double *  COIN_RESTRICT abcSolution = abcSolution_;
#ifdef CLEANUP_DJS
  double *  COIN_RESTRICT abcDj = abcDj_;
  double *  COIN_RESTRICT abcCost = abcCost_;
  // see if we can clean up djs
  // may have perturbation
  const double * COIN_RESTRICT originalCost = abcPerturbation_;
#else
  const double *  COIN_RESTRICT abcDj = abcDj_;
  const double *  COIN_RESTRICT abcCost = abcCost_;
#endif
  //const double *  COIN_RESTRICT abcPerturbation = abcPerturbation_;
  int badDjSequence=-1;
  for (int iSequence = 0; iSequence < numberTotal_; iSequence++) {
    if (getInternalStatus(iSequence) != basic) {
      sumNonBasicCosts_ += abcSolution[iSequence]*abcCost[iSequence];
      // not basic
      double distanceUp = abcUpper[iSequence] - abcSolution[iSequence];
      double distanceDown = abcSolution[iSequence] - abcLower[iSequence];
      double value = abcDj[iSequence];
      if (distanceUp > primalTolerance_) {
	// Check if "free"
	if (distanceDown <= primalTolerance_) {
	  // should not be negative
	  if (value < 0.0) {
	    value = - value;
	    if (value > currentDualTolerance_) {
	      bool treatAsSuperBasic=false;
	      if (getInternalStatus(iSequence)==superBasic) {
#ifdef PAN
		if (fakeSuperBasic(iSequence)>=0) {
#endif
		  treatAsSuperBasic=true;
		  numberSuperBasicWithDj++;
		  if (firstFreeDual < 0)
		    firstFreeDual = iSequence;
#ifdef PAN
		}
#endif
	      }
	      if (!treatAsSuperBasic) {
		sumDualInfeasibilities_ += value - currentDualTolerance_;
		badDjSequence=iSequence;
		if (value > possTolerance)
		  bestPossibleImprovement_ += CoinMin(distanceUp, 1.0e10) * value;
		if (value > relaxedTolerance)
		  sumOfRelaxedDualInfeasibilities_ += value - relaxedTolerance;
		numberDualInfeasibilities_ ++;
	      }
	    }
#ifdef CLEANUP_DJS
	  } else {
	    // adjust cost etc
	    if (abcCost[iSequence]>originalCost[iSequence]) {
	      double difference = CoinMin(abcCost[iSequence]-originalCost[iSequence],value);
	      //printf("Lb %d changing cost from %g to %g, dj from %g to %g\n",
	      //     iSequence,abcCost[iSequence],abcCost[iSequence]-difference,
	      //     abcDj[iSequence],abcDj[iSequence]-difference);
	      abcDj[iSequence]=value-difference;
	      abcCost[iSequence]-=difference;
	    }
#endif
	  }
	} else {
	  // free
	  value=fabs(value);
	  if (value > 1.0 * relaxedTolerance) {
	    numberSuperBasicWithDj++;
	    if (firstFreeDual < 0)
	      firstFreeDual = iSequence;
	  }
	  if (value > currentDualTolerance_) {
	    sumDualInfeasibilities_ += value - currentDualTolerance_;
	    if (value > possTolerance)
	      bestPossibleImprovement_ += CoinMin(distanceUp, 1.0e10) * value;
	    if (value > relaxedTolerance)
	      sumOfRelaxedDualInfeasibilities_ += value - relaxedTolerance;
	    numberDualInfeasibilities_ ++;
	  }
	  if (firstFreePrimal < 0)
	    firstFreePrimal = iSequence;
	}
      } else if (distanceDown > primalTolerance_) {
	// should not be positive
	if (value > 0.0) {
	  if (value > currentDualTolerance_) {
	    bool treatAsSuperBasic=false;
	    if (getInternalStatus(iSequence)==superBasic) {
#ifdef PAN
	      if (fakeSuperBasic(iSequence)>=0) {
#endif
		treatAsSuperBasic=true;
		numberSuperBasicWithDj++;
		if (firstFreeDual < 0)
		  firstFreeDual = iSequence;
#ifdef PAN
	      }
#endif
	    }
	    if (!treatAsSuperBasic) {
	      sumDualInfeasibilities_ += value - currentDualTolerance_;
	      badDjSequence=iSequence;
	      if (value > possTolerance)
		bestPossibleImprovement_ += value * CoinMin(distanceDown, 1.0e10);
	      if (value > relaxedTolerance)
		sumOfRelaxedDualInfeasibilities_ += value - relaxedTolerance;
	      numberDualInfeasibilities_ ++;
	    }
	  }
#ifdef CLEANUP_DJS
	} else {
	  // adjust cost etc
	  if (abcCost[iSequence]<originalCost[iSequence]) {
	    double difference = CoinMax(abcCost[iSequence]-originalCost[iSequence],value);
	    //printf("Ub %d changing cost from %g to %g, dj from %g to %g\n",
	    //	     iSequence,abcCost[iSequence],abcCost[iSequence]-difference,
	    //	     abcDj[iSequence],abcDj[iSequence]-difference);
	    abcDj[iSequence]=value-difference;
	    abcCost[iSequence]-=difference;
	  }
#endif
	}
      }
    }
  }
#ifdef CLEANUP_DJS
  if (algorithm_<0&&numberDualInfeasibilities_==1&&firstFreeDual<0&&sumDualInfeasibilities_<1.0e-1) {
    // clean bad one
    assert (badDjSequence>=0);
    abcCost[badDjSequence]-=abcDj_[badDjSequence];
    abcDj_[badDjSequence]=0.0;
  }
#endif
  numberDualInfeasibilitiesWithoutFree_ = numberDualInfeasibilities_-numberSuperBasicWithDj;
  numberFreeNonBasic_=numberSuperBasicWithDj;
  firstFree_=-1;
  if (algorithm_ < 0 && firstFreeDual >= 0) {
    // dual
    firstFree_ = firstFreeDual;
  } else if ((numberSuperBasicWithDj ||
	      (abcProgress_.lastIterationNumber(0) <= 0))&&algorithm_>0) {
    firstFree_ = firstFreePrimal;
  }
}
// Perturbs problem
void 
AbcSimplexDual::perturb(double factor)
{
  const double * COIN_RESTRICT lower = lowerSaved_;
  const double * COIN_RESTRICT upper = upperSaved_;
  const double * COIN_RESTRICT perturbation = abcPerturbation_;
  double * COIN_RESTRICT cost = abcCost_;
  for (int i=0;i<numberTotal_;i++) {
    if (getInternalStatus(i)!=basic) {
      if (fabs(lower[i])<fabs(upper[i])) {
	cost[i]+=factor*perturbation[i];
      } else if (upper[i]!=COIN_DBL_MAX&&upper[i]!=lower[i]) {
	cost[i]-=factor*perturbation[i];
      } else {
	// free
	//cost[i]=costSaved[i];
      }
    }
  }
#ifdef CLEANUP_DJS
  // copy cost to perturbation
  CoinAbcMemcpy(abcPerturbation_,abcCost_,numberTotal_);
#endif
}
#if ABC_NORMAL_DEBUG>0
#define PERT_STATISTICS
#endif
#ifdef PERT_STATISTICS
static void breakdown(const char * name, int numberLook, const double * region)
{
     double range[] = {
          -COIN_DBL_MAX,
          -1.0e15, -1.0e11, -1.0e8, -1.0e5, -3.0e4, -1.0e4, -3.0e3, -1.0e3, -3.0e2, -1.0e2, -3.0e1, -1.0e1, -3.0,
          -1.0, -3.0e-1,
          -1.0e-1, -3.0e-1, -1.0e-2, -3.0e-2, -1.0e-3, -3.0e-3, -1.0e-4, -3.0e-4, -1.0e-5, -1.0e-8, -1.0e-11, -1.0e-15,
          0.0,
          1.0e-15, 1.0e-11, 1.0e-8, 1.0e-5, 3.0e-5, 1.0e-4, 3.0e-4, 1.0e-3, 3.0e-3, 1.0e-2, 3.0e-2, 1.0e-1, 3.0e-1,
          1.0, 3.0,
          1.0e1, 3.0e1, 1.0e2, 3.0e2, 1.0e3, 3.0e3, 1.0e4, 3.0e4, 1.0e5, 1.0e8, 1.0e11, 1.0e15,
          COIN_DBL_MAX
     };
     int nRanges = static_cast<int> (sizeof(range) / sizeof(double));
     int * number = new int[nRanges];
     memset(number, 0, nRanges * sizeof(int));
     int * numberExact = new int[nRanges];
     memset(numberExact, 0, nRanges * sizeof(int));
     int i;
     for ( i = 0; i < numberLook; i++) {
          double value = region[i];
          for (int j = 0; j < nRanges; j++) {
               if (value == range[j]) {
                    numberExact[j]++;
                    break;
               } else if (value < range[j]) {
                    number[j]++;
                    break;
               }
          }
     }
     printf("\n%s has %d entries\n", name, numberLook);
     for (i = 0; i < nRanges; i++) {
          if (number[i])
               printf("%d between %g and %g", number[i], range[i-1], range[i]);
          if (numberExact[i]) {
               if (number[i])
                    printf(", ");
               printf("%d exactly at %g", numberExact[i], range[i]);
          }
          if (number[i] + numberExact[i])
               printf("\n");
     }
     delete [] number;
     delete [] numberExact;
}
#endif
// Perturbs problem
void 
AbcSimplexDual::perturbB(double /*factorIn*/,int /*type*/)
{
  double overallMultiplier= (perturbation_==50||perturbation_>54) ? 1.0 : 0.1;
  // dual perturbation
  double perturbation = 1.0e-20;
  // maximum fraction of cost to perturb
  double maximumFraction = 1.0e-5;
  int maxLength = 0;
  int minLength = numberRows_;
  double averageCost = 0.0;
  int numberNonZero = 0;
  // See if we need to perturb
  double * COIN_RESTRICT sort = new double[numberColumns_];
  // Use objective BEFORE scaling ??
  const double * COIN_RESTRICT obj = objective();
  for (int i = 0; i < numberColumns_; i++) {
    double value = fabs(obj[i]);
    sort[i] = value;
    averageCost += fabs(abcCost_[i+maximumAbcNumberRows_]);
    if (value)
      numberNonZero++;
  }
  if (numberNonZero)
    averageCost /= static_cast<double> (numberNonZero);
  else
    averageCost = 1.0;
  // allow for cost scaling
  averageCost *= objectiveScale_;
  std::sort(sort, sort + numberColumns_);
  int number = 1;
  double last = sort[0];
  for (int i = 1; i < numberColumns_; i++) {
    if (last != sort[i])
      number++;
    last = sort[i];
  }
  delete [] sort;
#ifdef PERT_STATISTICS
  printf("%d different values\n",number);
#endif
#if 0
  printf("nnz %d percent %d", number, (number * 100) / numberColumns_);
  if (number * 4 > numberColumns_)
    printf(" - Would not perturb\n");
  else
    printf(" - Would perturb\n");
#endif
  // If small costs then more dangerous (better to scale)
  double numberMultiplier=1.25;
  if (averageCost<1.0e-3)
    numberMultiplier=20.0;
  else if (averageCost<1.0e-2)
    numberMultiplier=10.0;
  else if (averageCost<1.0e-1)
    numberMultiplier=3.0;
  else if (averageCost<1.0)
    numberMultiplier=2.0;
  numberMultiplier *= 2.0; // try
  if (number * numberMultiplier > numberColumns_||perturbation_>=100) {
    perturbation_=101;
#if ABC_NORMAL_DEBUG>0
    printf("Not perturbing - %d different costs, average %g\n",
	   number,averageCost);
#endif
#ifdef CLEANUP_DJS
    // copy cost to perturbation
    CoinAbcMemcpy(abcPerturbation_,abcCost_,numberTotal_);
#endif
    return ; // good enough
  }
  const int * COIN_RESTRICT columnLength = matrix_->getVectorLengths()-maximumAbcNumberRows_;
  const double * COIN_RESTRICT lower = lowerSaved_;
  const double * COIN_RESTRICT upper = upperSaved_;
  for (int iSequence = maximumAbcNumberRows_; iSequence < numberTotal_; iSequence++) {
    if (lower[iSequence] < upper[iSequence]) {
      int length = columnLength[iSequence];
      if (length > 2) {
	maxLength = CoinMax(maxLength, length);
	minLength = CoinMin(minLength, length);
      }
    }
  }
  bool uniformChange = false;
  bool inCbcOrOther = (specialOptions_ & 0x03000000) != 0;
  double constantPerturbation = 10.0 * dualTolerance_;
#ifdef PERT_STATISTICS
  breakdown("Objective before perturbation", numberColumns_, abcCost_+numberRows_);
#endif
  //#define PERTURB_OLD_WAY
#ifndef PERTURB_OLD_WAY
#if 1
#ifdef HEAVY_PERTURBATION
    if (perturbation_==50)
      perturbation_=HEAVY_PERTURBATION;
#endif
  if (perturbation_ >= 50) {
    // Experiment
    // maximumFraction could be 1.0e-10 to 1.0
    double m[] = {1.0e-10, 1.0e-9, 1.0e-8, 1.0e-7, 1.0e-6, 1.0e-5, 1.0e-4, 1.0e-3, 1.0e-2, 1.0e-1, 1.0};
    int whichOne = CoinMin(perturbation_ - 51,10);
    if (whichOne<0) {
      if (numberRows_<5000)
	whichOne=2; // treat 50 as if 53
      else
	whichOne=2; // treat 50 as if 52
    }
    if (inCbcOrOther&&whichOne>0)
      whichOne--;
    maximumFraction = m[whichOne];
    if (whichOne>7)
      constantPerturbation *= 10.0; 
    //if (whichOne<2)
    //constantPerturbation = maximumFraction*1.0e-10 * dualTolerance_;
  } else if (inCbcOrOther) {
    maximumFraction = 1.0e-6;
  }
#endif
  double smallestNonZero = 1.0e100;
  bool modifyRowCosts=false;
  numberNonZero = 0;
  //perturbation = 1.0e-10;
  perturbation = CoinMax(1.0e-10,maximumFraction);
  double * COIN_RESTRICT cost = abcCost_;
  bool allSame = true;
  double lastValue = 0.0;
  
  for (int iRow = 0; iRow < numberRows_; iRow++) {
    double lo = lower[iRow];
    double up = upper[iRow];
    if (lo < up) {
      double value = fabs(cost[iRow]);
      perturbation = CoinMax(perturbation, value);
      if (value) {
	modifyRowCosts = true;
	smallestNonZero = CoinMin(smallestNonZero, value);
      }
    }
    if (lo && lo > -1.0e10) {
      numberNonZero++;
      lo = fabs(lo);
      if (!lastValue)
	lastValue = lo;
      else if (fabs(lo - lastValue) > 1.0e-7)
	allSame = false;
    }
    if (up && up < 1.0e10) {
      numberNonZero++;
      up = fabs(up);
      if (!lastValue)
	lastValue = up;
      else if (fabs(up - lastValue) > 1.0e-7)
	allSame = false;
    }
  }
  double lastValue2 = 0.0;
  for (int iSequence = maximumAbcNumberRows_; iSequence < numberTotal_; iSequence++) {
    double lo = lower[iSequence];
    double up = upper[iSequence];
    if (lo < up) {
      double value =
	fabs(cost[iSequence]);
      perturbation = CoinMax(perturbation, value);
      if (value) {
	smallestNonZero = CoinMin(smallestNonZero, value);
      }
    }
    if (lo && lo > -1.0e10) {
      //numberNonZero++;
      lo = fabs(lo);
      if (!lastValue2)
	lastValue2 = lo;
      else if (fabs(lo - lastValue2) > 1.0e-7)
	allSame = false;
    }
    if (up && up < 1.0e10) {
      //numberNonZero++;
      up = fabs(up);
      if (!lastValue2)
	lastValue2 = up;
      else if (fabs(up - lastValue2) > 1.0e-7)
	allSame = false;
    }
  }
  if (allSame) {
    // Check elements
    double smallestNegative;
    double largestNegative;
    double smallestPositive;
    double largestPositive;
    matrix_->rangeOfElements(smallestNegative, largestNegative,
			     smallestPositive, largestPositive);
    if (smallestNegative == largestNegative &&
	smallestPositive == largestPositive) {
      // Really hit perturbation
      double adjust = CoinMin(100.0 * maximumFraction, 1.0e-3 * CoinMax(lastValue, lastValue2));
      maximumFraction = CoinMax(adjust, maximumFraction);
    }
  }
  perturbation = CoinMin(perturbation, smallestNonZero / maximumFraction);
  double largestZero = 0.0;
  double largest = 0.0;
  double largestPerCent = 0.0;
  // modify costs
  bool printOut = (handler_->logLevel() == 63);
  printOut = false;
  //assert (!modifyRowCosts);
  modifyRowCosts = false;
  const double * COIN_RESTRICT perturbationArray = perturbationSaved_;
  if (modifyRowCosts) {
    for (int iRow = 0; iRow < numberRows_; iRow++) {
      if (lower[iRow] < upper[iRow]) {
	double value = perturbation;
	double currentValue = cost[iRow];
	value = CoinMin(value, maximumFraction * (fabs(currentValue) + 1.0e-1 * perturbation + 1.0e-3));
	double perturbationValue=2.0*(perturbationArray[iRow]-0.5); // for now back to normal random
	if (lower[iRow] > -largeValue_) {
	  if (fabs(lower[iRow]) < fabs(upper[iRow]))
	    value *= perturbationValue;
	  else
	    value *= -perturbationValue;
	} else if (upper[iRow] < largeValue_) {
	  value *= -perturbationValue;
	} else {
	  value = 0.0;
	}
	if (currentValue) {
	  largest = CoinMax(largest, fabs(value));
	  if (fabs(value) > fabs(currentValue)*largestPerCent)
	    largestPerCent = fabs(value / currentValue);
	} else {
	  largestZero = CoinMax(largestZero, fabs(value));
	}
	if (printOut)
	  printf("row %d cost %g change %g\n", iRow, cost[iRow], value);
	cost[iRow] += value;
      }
    }
  }

  //double extraWeight = 10.0;
  // more its but faster double weight[]={1.0e-4,1.0e-2,1.0e-1,1.0,2.0,10.0,100.0,200.0,400.0,600.0,1000.0};
  // good its double weight[]={1.0e-4,1.0e-2,5.0e-1,1.0,2.0,5.0,10.0,20.0,30.0,40.0,100.0};
  double weight[] = {1.0e-4, 1.0e-2, 5.0e-1, 1.0, 2.0, 5.0, 10.0, 20.0, 30.0, 40.0, 100.0};
  //double weight[]={1.0e-4,1.0e-2,5.0e-1,1.0,20.0,50.0,100.0,120.0,130.0,140.0,200.0};
  // Scale back if wanted
  double weight2[] = {1.0e-4, 1.0e-2, 5.0e-1, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0};
  if (constantPerturbation < 99.0 * dualTolerance_&&false) {
    perturbation *= 0.1;
    //extraWeight = 0.5;
    memcpy(weight, weight2, sizeof(weight2));
  }
  // adjust weights if all columns long
  double factor = 1.0;
  if (maxLength) {
    factor = 3.0 / static_cast<double> (minLength);
  }
  // Make variables with more elements more expensive
  const double m1 = 0.5;
  double smallestAllowed = overallMultiplier*CoinMin(1.0e-2 * dualTolerance_, maximumFraction);
  double largestAllowed = overallMultiplier*CoinMax(1.0e3 * dualTolerance_, maximumFraction * averageCost);
  // smaller if in BAB
  //if (inCbcOrOther)
  //largestAllowed=CoinMin(largestAllowed,1.0e-5);
  //smallestAllowed = CoinMin(smallestAllowed,0.1*largestAllowed);
  randomNumberGenerator_.setSeed(1000000000);
#if ABC_NORMAL_DEBUG>0
  printf("perturbation %g constantPerturbation %g smallestNonZero %g maximumFraction %g smallestAllowed %g\n",
	 perturbation,constantPerturbation,smallestNonZero,maximumFraction,
	 smallestAllowed);
#endif
  for (int iSequence = maximumAbcNumberRows_; iSequence < numberTotal_; iSequence++) {
    if (lower[iSequence] < upper[iSequence] && getInternalStatus(iSequence) != basic) {
      double value = perturbation;
      double currentValue = cost[iSequence];
      double currentLargestAllowed=fabs(currentValue)*1.0e-3;
      if (!currentValue)
	currentLargestAllowed=1.0e-3;
      if (currentLargestAllowed<=10.0*smallestAllowed)
	continue; // don't perturb tiny value
      value = CoinMin(value, constantPerturbation + maximumFraction * (fabs(currentValue) + 1.0e-1 * perturbation + 1.0e-8));
      //value = CoinMin(value,constantPerturbation;+maximumFraction*fabs(currentValue));
      double value2 = constantPerturbation + 1.0e-1 * smallestNonZero;
      if (uniformChange) {
	value = maximumFraction;
	value2 = maximumFraction;
      }
      //double perturbationValue=2.0*(perturbationArray[iSequence]-0.5); // for now back to normal random
      double perturbationValue=randomNumberGenerator_.randomDouble();
      perturbationValue = 1.0-m1+m1*perturbationValue; // clean up
      if (lower[iSequence] > -largeValue_) {
	if (fabs(lower[iSequence]) <
	    fabs(upper[iSequence])) {
	  value *= perturbationValue;
	  value2 *= perturbationValue;
	} else {
	  //value *= -(1.0-m1+m1*randomNumberGenerator_.randomDouble());
	  //value2 *= -(1.0-m1+m1*randomNumberGenerator_.randomDouble());
	  value = 0.0;
	}
      } else if (upper[iSequence] < largeValue_) {
	value *= -perturbationValue;
	value2 *= -perturbationValue;
      } else {
	value = 0.0;
      }
      if (value) {
	int length = columnLength[iSequence];
	if (length > 3) {
	  length = static_cast<int> (static_cast<double> (length) * factor);
	  length = CoinMax(3, length);
	}
	double multiplier;
	if (length < 10)
	  multiplier = weight[length];
	else
	  multiplier = weight[10];
	value *= multiplier;
	value = CoinMin(value, value2);
	if (value) {
	  // get in range
	  if (fabs(value) <= smallestAllowed) {
	    value *= 10.0;
	    while (fabs(value) <= smallestAllowed)
	      value *= 10.0;
	  } else if (fabs(value) > currentLargestAllowed) {
	    value *= 0.1;
	    while (fabs(value) > currentLargestAllowed)
	      value *= 0.1;
	  }
	}
	if (currentValue) {
	  largest = CoinMax(largest, fabs(value));
	  if (fabs(value) > fabs(currentValue)*largestPerCent)
	    largestPerCent = fabs(value / currentValue);
	} else {
	  largestZero = CoinMax(largestZero, fabs(value));
	}
	// but negative if at ub
	if (getInternalStatus(iSequence) == atUpperBound)
	  value = -value;
	if (printOut)
	  printf("col %d cost %g change %g\n", iSequence, cost[iSequence], value);
	cost[iSequence] += value;
      }
    }
  }
#else
  // old way
     if (perturbation_ > 50) {
          // Experiment
          // maximumFraction could be 1.0e-10 to 1.0
          double m[] = {1.0e-10, 1.0e-9, 1.0e-8, 1.0e-7, 1.0e-6, 1.0e-5, 1.0e-4, 1.0e-3, 1.0e-2, 1.0e-1, 1.0};
          int whichOne = perturbation_ - 51;
          //if (inCbcOrOther&&whichOne>0)
          //whichOne--;
          maximumFraction = m[CoinMin(whichOne, 10)];
     } else if (inCbcOrOther) {
          //maximumFraction = 1.0e-6;
     }
     int iRow;
     double smallestNonZero = 1.0e100;
     numberNonZero = 0;
     if (perturbation_ >= 50) {
          perturbation = 1.0e-8;
	  if (perturbation_ > 50 && perturbation_ < 60) 
	    perturbation = CoinMax(1.0e-8,maximumFraction);
          bool allSame = true;
          double lastValue = 0.0;
          for (iRow = 0; iRow < numberRows_; iRow++) {
               double lo = abcLower_[iRow];
               double up = abcUpper_[iRow];
               if (lo && lo > -1.0e10) {
                    numberNonZero++;
                    lo = fabs(lo);
                    if (!lastValue)
                         lastValue = lo;
                    else if (fabs(lo - lastValue) > 1.0e-7)
                         allSame = false;
               }
               if (up && up < 1.0e10) {
                    numberNonZero++;
                    up = fabs(up);
                    if (!lastValue)
                         lastValue = up;
                    else if (fabs(up - lastValue) > 1.0e-7)
                         allSame = false;
               }
          }
          double lastValue2 = 0.0;
          for (int iColumn = maximumAbcNumberRows_; iColumn < numberTotal_; iColumn++) {
               double lo = abcLower_[iColumn];
               double up = abcUpper_[iColumn];
               if (lo < up) {
                    double value =
                         fabs(abcCost_[iColumn]);
                    perturbation = CoinMax(perturbation, value);
                    if (value) {
                         smallestNonZero = CoinMin(smallestNonZero, value);
                    }
               }
               if (lo && lo > -1.0e10) {
                    //numberNonZero++;
                    lo = fabs(lo);
                    if (!lastValue2)
                         lastValue2 = lo;
                    else if (fabs(lo - lastValue2) > 1.0e-7)
                         allSame = false;
               }
               if (up && up < 1.0e10) {
                    //numberNonZero++;
                    up = fabs(up);
                    if (!lastValue2)
                         lastValue2 = up;
                    else if (fabs(up - lastValue2) > 1.0e-7)
                         allSame = false;
               }
          }
          if (allSame) {
               // Check elements
               double smallestNegative;
               double largestNegative;
               double smallestPositive;
               double largestPositive;
               matrix_->rangeOfElements(smallestNegative, largestNegative,
                                        smallestPositive, largestPositive);
               if (smallestNegative == largestNegative &&
                         smallestPositive == largestPositive) {
                    // Really hit perturbation
                    double adjust = CoinMin(100.0 * maximumFraction, 1.0e-3 * CoinMax(lastValue, lastValue2));
                    maximumFraction = CoinMax(adjust, maximumFraction);
               }
          }
          perturbation = CoinMin(perturbation, smallestNonZero / maximumFraction);
     }
     double largestZero = 0.0;
     double largest = 0.0;
     double largestPerCent = 0.0;
     // modify costs
     bool printOut = (handler_->logLevel() == 63);
     printOut = false;
     // more its but faster double weight[]={1.0e-4,1.0e-2,1.0e-1,1.0,2.0,10.0,100.0,200.0,400.0,600.0,1000.0};
     // good its double weight[]={1.0e-4,1.0e-2,5.0e-1,1.0,2.0,5.0,10.0,20.0,30.0,40.0,100.0};
     double weight[] = {1.0e-4, 1.0e-2, 5.0e-1, 1.0, 2.0, 5.0, 10.0, 20.0, 30.0, 40.0, 100.0};
     //double weight[]={1.0e-4,1.0e-2,5.0e-1,1.0,20.0,50.0,100.0,120.0,130.0,140.0,200.0};
     //double extraWeight = 10.0;
     // Scale back if wanted
     double weight2[] = {1.0e-4, 1.0e-2, 5.0e-1, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0};
     if (constantPerturbation < 99.0 * dualTolerance_) {
          perturbation *= 0.1;
          //extraWeight = 0.5;
          memcpy(weight, weight2, sizeof(weight2));
     }
     // adjust weights if all columns long
     double factor = 1.0;
     if (maxLength) {
          factor = 3.0 / static_cast<double> (minLength);
     }
     // Make variables with more elements more expensive
     const double m1 = 0.5;
     double smallestAllowed = CoinMin(1.0e-2 * dualTolerance_, maximumFraction);
     double largestAllowed = CoinMax(1.0e3 * dualTolerance_, maximumFraction * averageCost);
     // smaller if in BAB
     //if (inCbcOrOther)
     //largestAllowed=CoinMin(largestAllowed,1.0e-5);
     //smallestAllowed = CoinMin(smallestAllowed,0.1*largestAllowed);
     for (int iColumn = maximumAbcNumberRows_; iColumn < numberTotal_; iColumn++) {
          if (abcLower_[iColumn] < abcUpper_[iColumn] 
	      && getInternalStatus(iColumn) != basic) {
               double value = perturbation;
               double currentValue = abcCost_[iColumn];
               value = CoinMin(value, constantPerturbation + maximumFraction * (fabs(currentValue) + 1.0e-1 * perturbation + 1.0e-8));
               //value = CoinMin(value,constantPerturbation;+maximumFraction*fabs(currentValue));
               double value2 = constantPerturbation + 1.0e-1 * smallestNonZero;
               if (uniformChange) {
                    value = maximumFraction;
                    value2 = maximumFraction;
               }
               if (abcLower_[iColumn] > -largeValue_) {
                    if (fabs(abcLower_[iColumn]) <
                              fabs(abcUpper_[iColumn])) {
                         value *= (1.0 - m1 + m1 * randomNumberGenerator_.randomDouble());
                         value2 *= (1.0 - m1 + m1 * randomNumberGenerator_.randomDouble());
                    } else {
                         //value *= -(1.0-m1+m1*randomNumberGenerator_.randomDouble());
                         //value2 *= -(1.0-m1+m1*randomNumberGenerator_.randomDouble());
                         value = 0.0;
                    }
               } else if (abcUpper_[iColumn] < largeValue_) {
                    value *= -(1.0 - m1 + m1 * randomNumberGenerator_.randomDouble());
                    value2 *= -(1.0 - m1 + m1 * randomNumberGenerator_.randomDouble());
               } else {
                    value = 0.0;
               }
               if (value) {
                    int length = columnLength[iColumn];
                    if (length > 3) {
                         length = static_cast<int> (static_cast<double> (length) * factor);
                         length = CoinMax(3, length);
                    }
                    double multiplier;
#if 1
                    if (length < 10)
                         multiplier = weight[length];
                    else
                         multiplier = weight[10];
#else
                    if (length < 10)
                         multiplier = weight[length];
                    else
                         multiplier = weight[10] + extraWeight * (length - 10);
                    multiplier *= 0.5;
#endif
                    value *= multiplier;
                    value = CoinMin(value, value2);
                    if (value) {
                         // get in range
                         if (fabs(value) <= smallestAllowed) {
                              value *= 10.0;
                              while (fabs(value) <= smallestAllowed)
                                   value *= 10.0;
                         } else if (fabs(value) > largestAllowed) {
                              value *= 0.1;
                              while (fabs(value) > largestAllowed)
                                   value *= 0.1;
                         }
                    }
                    if (currentValue) {
                         largest = CoinMax(largest, fabs(value));
                         if (fabs(value) > fabs(currentValue)*largestPerCent)
                              largestPerCent = fabs(value / currentValue);
                    } else {
                         largestZero = CoinMax(largestZero, fabs(value));
                    }
                    // but negative if at ub
                    if (getInternalStatus(iColumn) == atUpperBound)
                         value = -value;
                    if (printOut)
                         printf("col %d cost %g change %g\n", iColumn, objectiveWork_[iColumn], value);
                    abcCost_[iColumn] += value;
               }
          }
     }
#endif
#ifdef PERT_STATISTICS
  {
    double averageCost = 0.0;
    int numberNonZero = 0;
    double * COIN_RESTRICT sort = new double[numberColumns_];
    for (int i = 0; i < numberColumns_; i++) {
      double value = fabs(abcCost_[i+maximumAbcNumberRows_]);
      sort[i] = value;
      averageCost += value;
      if (value)
	numberNonZero++;
    }
    if (numberNonZero)
      averageCost /= static_cast<double> (numberNonZero);
    else
      averageCost = 1.0;
    std::sort(sort, sort + numberColumns_);
    int number = 1;
    double last = sort[0];
    for (int i = 1; i < numberColumns_; i++) {
      if (last != sort[i])
	number++;
    last = sort[i];
    }
    delete [] sort;
#if ABC_NORMAL_DEBUG>0
    printf("nnz %d percent %d", number, (number * 100) / numberColumns_);
    breakdown("Objective after perturbation", numberColumns_, abcCost_+numberRows_);
#endif
  }
#endif
  perturbation_=100;
  // copy cost to  perturbation
#ifdef CLEANUP_DJS
  CoinAbcMemcpy(abcPerturbation_,abcCost_,numberTotal_);
#endif
  handler_->message(CLP_SIMPLEX_PERTURB, messages_)
    << 100.0 * maximumFraction << perturbation << largest << 100.0 * largestPerCent << largestZero
    << CoinMessageEol;
}
/* Does something about fake tolerances
   -1 initializes
   1 might be optimal (back to original costs)
   2 after change currentDualBound
   3 might be optimal BUT change currentDualBound
*/
int 
AbcSimplexDual::bounceTolerances(int type)
{
  // for now turn off
  if (type==-1) {
    if (!perturbationFactor_)
      perturbationFactor_=0.001;
    CoinAbcMemcpy(abcCost_,costSaved_,numberTotal_);
    if (!perturbationFactor_) {
      //currentDualBound_=CoinMin(dualBound_,1.0e11);
      //CoinAbcMultiplyAdd(perturbationSaved_,numberTotal_,currentDualTolerance_,
      //		 abcPerturbation_,0.0);
    } else {
      //currentDualBound_=CoinMin(dualBound_,1.0e11);
      double factor=currentDualTolerance_*perturbationFactor_;
      perturbB(factor,0);
    }
#if ABC_NORMAL_DEBUG>0
    //printf("new dual bound %g (bounceTolerances)\n",currentDualBound_);
#endif
  } else {
    if (stateDualColumn_==-2) {
      double newTolerance=CoinMin(1.0e-1,dualTolerance_*100.0);
#if ABC_NORMAL_DEBUG>0
      printf("After %d iterations reducing dual tolerance from %g to %g\n",
	     numberIterations_,currentDualTolerance_,newTolerance);
#endif
      currentDualTolerance_=newTolerance;
      stateDualColumn_=-1;
    } else if (stateDualColumn_==-1) {
#if ABC_NORMAL_DEBUG>0
      printf("changing dual tolerance from %g to %g (vaguely optimal)\n",
	     currentDualTolerance_,dualTolerance_);
#endif
      currentDualTolerance_=dualTolerance_;
      stateDualColumn_=0;
    } else {
      if (stateDualColumn_>1) {
	normalDualColumnIteration_ = numberIterations_+CoinMax(10,numberRows_/4);
#if ABC_NORMAL_DEBUG>0
	printf("no cost modification until iteration %d\n",normalDualColumnIteration_);
#endif
      }
    }
    resetFakeBounds(0);
    CoinAbcMemcpy(abcCost_,abcPerturbation_,numberTotal_);
    numberChanged_ = 0; // Number of variables with changed costs
    moveToBasic();
    if (type!=1)
      return 0;
    // save status and solution
    CoinAbcMemcpy(status_,internalStatus_,numberTotal_);
    CoinAbcMemcpy(solutionSaved_,abcSolution_,numberTotal_);
#if ABC_NORMAL_DEBUG>0
    printf("A sumDual %g sumPrimal %g\n",sumDualInfeasibilities_,sumPrimalInfeasibilities_);
#endif
    gutsOfSolution(3);
#if ABC_NORMAL_DEBUG>0
    printf("B sumDual %g sumPrimal %g\n",sumDualInfeasibilities_,sumPrimalInfeasibilities_);
#endif
    double saveSum1=sumPrimalInfeasibilities_;
    double saveSumD1=sumDualInfeasibilities_;
    bool goToPrimal=false;
    if (!sumOfRelaxedDualInfeasibilities_) {
      if (perturbation_<101) {
	// try going back to original costs
	CoinAbcMemcpy(abcCost_,costSaved_,numberTotal_);
	perturbation_=101;
	numberChanged_ = 0; // Number of variables with changed costs
	moveToBasic();
	gutsOfSolution(3);
#if ABC_NORMAL_DEBUG>0
	printf("B2 sumDual %g sumPrimal %g\n",sumDualInfeasibilities_,sumPrimalInfeasibilities_);
#endif
	saveSumD1=sumDualInfeasibilities_;
      }
      if (sumOfRelaxedDualInfeasibilities_) {
	makeNonFreeVariablesDualFeasible();
	moveToBasic();
	abcProgress_.reset();
	gutsOfSolution(3);
#if ABC_NORMAL_DEBUG>0
	printf("C sumDual %g sumPrimal %g\n",sumDualInfeasibilities_,sumPrimalInfeasibilities_);
#endif
	if (sumPrimalInfeasibilities_>1.0e2*saveSum1+currentDualBound_*0.2
	  /*|| sumOfRelaxedDualInfeasibilities_<1.0e-4*sumPrimalInfeasibilities_*/) {
	  numberDisasters_++;
	  if (numberDisasters_>2)
	    goToPrimal=true;
	}
      } else {
	numberDualInfeasibilities_=0; 
	sumDualInfeasibilities_=0.0; 
      }
    } else {
      // carry on
      makeNonFreeVariablesDualFeasible();
      moveToBasic();
      abcProgress_.reset();
      gutsOfSolution(3);
#if ABC_NORMAL_DEBUG>0
      printf("C2 sumDual %g sumPrimal %g\n",sumDualInfeasibilities_,sumPrimalInfeasibilities_);
#endif
      numberDisasters_++;
      if (sumPrimalInfeasibilities_>1.0e2*saveSum1+currentDualBound_*0.2
	  ||(saveSumD1<1.0e-4*sumPrimalInfeasibilities_&&saveSumD1<1.0)) {
	numberDisasters_++;
	if (numberDisasters_>2||saveSumD1<1.0e-10*sumPrimalInfeasibilities_)
	  goToPrimal=true;
      }
    }
    if (goToPrimal) {
      // go to primal
      CoinAbcMemcpy(internalStatus_,status_,numberTotal_);
      CoinAbcMemcpy(abcSolution_,solutionSaved_,numberTotal_);
      int *  COIN_RESTRICT abcPivotVariable = abcPivotVariable_;
      // redo pivotVariable
      int numberBasic=0;
      for (int i=0;i<numberTotal_;i++) {
	if (getInternalStatus(i)==basic)
	  abcPivotVariable[numberBasic++]=i;
      }
      assert (numberBasic==numberRows_);
      moveToBasic();
      checkPrimalSolution(false);
#if ABC_NORMAL_DEBUG>0
      printf("D sumPrimal %g\n",sumPrimalInfeasibilities_);
#endif
      return 1;
    }
  }

  CoinAbcGatherFrom(abcCost_,costBasic_,abcPivotVariable_,numberRows_);
  return 0;
}
#if ABC_NORMAL_DEBUG>0
extern int cilk_conflict;
#endif
int 
AbcSimplexDual::noPivotRow()
{
#if ABC_NORMAL_DEBUG>3
  if (handler_->logLevel() & 32)
    printf("** no row pivot\n");
#endif
  int numberPivots = abcFactorization_->pivots();
  bool specialCase;
  int useNumberFake;
  int returnCode = 0;
  if (numberPivots <= CoinMax(dontFactorizePivots_, 20) &&
      (specialOptions_ & 2048) != 0 && currentDualBound_ >= 1.0e8) {
    specialCase = true;
    // as dual bound high - should be okay
    useNumberFake = 0;
  } else {
    specialCase = false;
    useNumberFake = numberAtFakeBound();
  }
  if (!numberPivots || specialCase) {
    // may have crept through - so may be optimal
    // check any flagged variables
    if (numberFlagged_ && numberPivots) {
      // try factorization
      returnCode = -2;
    }
    
    if (useNumberFake || numberDualInfeasibilities_) {
      // may be dual infeasible
      if ((specialOptions_ & 1024) == 0)
	problemStatus_ = -5;
      else if (!useNumberFake && numberPrimalInfeasibilities_
	       && !numberPivots)
	problemStatus_ = 1;
    } else {
      if (numberFlagged_) {
#if ABC_NORMAL_DEBUG>3
	std::cout << "Flagged variables at end - infeasible?" << std::endl;
	printf("Probably infeasible - pivot was %g\n", alpha_);
#endif
#if ABC_NORMAL_DEBUG>3
	abort();
#endif
	problemStatus_ = -5;
      } else {
	problemStatus_ = 0;
#ifndef CLP_CHECK_NUMBER_PIVOTS
#define CLP_CHECK_NUMBER_PIVOTS 10
#endif
#if CLP_CHECK_NUMBER_PIVOTS < 20
	if (numberPivots > CLP_CHECK_NUMBER_PIVOTS) {
	  gutsOfSolution(2);
	  rawObjectiveValue_ +=sumNonBasicCosts_;
	  setClpSimplexObjectiveValue();
	  if (numberPrimalInfeasibilities_) {
	    problemStatus_ = -1;
	    returnCode = -2;
	  }
	}
#endif
	if (!problemStatus_) {
	  // make it look OK
	  numberPrimalInfeasibilities_ = 0;
	  sumPrimalInfeasibilities_ = 0.0;
	  numberDualInfeasibilities_ = 0;
	  sumDualInfeasibilities_ = 0.0;
	  if (numberChanged_) {
	    numberChanged_ = 0; // Number of variables with changed costs
	    CoinAbcMemcpy(abcCost_,abcCost_+maximumNumberTotal_,numberTotal_);
	    CoinAbcGatherFrom(abcCost_,costBasic_,abcPivotVariable_,numberRows_);
		  // make sure duals are current
	    gutsOfSolution(1);
	    setClpSimplexObjectiveValue();
	    abcProgress_.modifyObjective(-COIN_DBL_MAX);
	    if (numberDualInfeasibilities_) {
	      problemStatus_ = 10; // was -3;
	      numberTimesOptimal_++;
	    } else {
	      computeObjectiveValue(true);
	    }
	  } else if (numberPivots) {
	    computeObjectiveValue(true);
	  }
	  sumPrimalInfeasibilities_ = 0.0;
	}
	if ((specialOptions_&(1024 + 16384)) != 0 && !problemStatus_&&abcFactorization_->pivots()) {
	  CoinIndexedVector * arrayVector = &usefulArray_[arrayForFtran_];
	  double * rhs = arrayVector->denseVector();
	  arrayVector->clear();
	  CoinAbcScatterTo(solutionBasic_,abcSolution_,abcPivotVariable_,numberRows_);
	  abcMatrix_->timesModifyExcludingSlacks(-1.0, abcSolution_,rhs);
	  //#define CHECK_ACCURACY
#ifdef CHECK_ACCURACY
	  bool bad = false;
#endif
	  bool bad2 = false;
	  int i;
	  for ( i = 0; i < numberRows_; i++) {
	    if (rhs[i] < abcLower_[i] - primalTolerance_ ||
		rhs[i] > abcUpper_[i] + primalTolerance_) {
	      bad2 = true;
#ifdef CHECK_ACCURACY
	      printf("row %d out of bounds %g, %g correct %g bad %g\n", i,
		     abcLower_[i], abcUpper_[i],
		     rhs[i], abcSolution_[i]);
#endif
	    } else if (fabs(rhs[i] - abcSolution_[i]) > 1.0e-3) {
#ifdef CHECK_ACCURACY
	      bad = true;
	      printf("row %d correct %g bad %g\n", i, rhs[i], abcSolution_[i]);
#endif
	    }
	    rhs[i] = 0.0;
	  }
	  for (int i = 0; i < numberTotal_; i++) {
	    if (abcSolution_[i] < abcLower_[i] - primalTolerance_ ||
		abcSolution_[i] > abcUpper_[i] + primalTolerance_) {
	      bad2 = true;
#ifdef CHECK_ACCURACY
	      printf("column %d out of bounds %g, %g value %g\n", i,
		     abcLower_[i], abcUpper_[i],
		     abcSolution_[i]);
#endif
	    }
	  }
	  if (bad2) {
	    problemStatus_ = -3;
	    returnCode = -2;
	    // Force to re-factorize early next time
	    int numberPivots = abcFactorization_->pivots();
	    if (forceFactorization_<0)
	      forceFactorization_=100000;
	    forceFactorization_ = CoinMin(forceFactorization_, (numberPivots + 1) >> 1);
	  }
	}
      }
    }
  } else {
    problemStatus_ = -3;
    returnCode = -2;
    // Force to re-factorize early next time
    int numberPivots = abcFactorization_->pivots();
    if (forceFactorization_<0)
      forceFactorization_=100000;
    forceFactorization_ = CoinMin(forceFactorization_, (numberPivots + 1) >> 1);
  }
  return returnCode;
}
void 
AbcSimplexDual::dualPivotColumn()
{
  double saveAcceptable=acceptablePivot_;
  if (largestPrimalError_>1.0e-5||largestDualError_>1.0e-5) {
    //if ((largestPrimalError_>1.0e-5||largestDualError_>1.0e-5)&&false) {
    if (!abcFactorization_->pivots())
      acceptablePivot_*=1.0e2;
    else if (abcFactorization_->pivots()<5)
      acceptablePivot_*=1.0e1;
  }
  dualColumn2();
  if (sequenceIn_<0&&acceptablePivot_>saveAcceptable) {
    acceptablePivot_=saveAcceptable;
#if ABC_NORMAL_DEBUG>0
    printf("No pivot!!!\n");
#endif
    // try again
    dualColumn1(true);
    dualColumn2();
  } else {
    acceptablePivot_=saveAcceptable;
  }
  if ((stateOfProblem_&VALUES_PASS)!=0) {
    // check no flips
    assert (!usefulArray_[arrayForFlipBounds_].getNumElements());
    // If no pivot - then try other way if plausible
    if (sequenceIn_<0) {
      // could use fake primal basic values
      if ((directionOut_<0&&lowerOut_>-1.0e30)||
	  (directionOut_>0&&upperOut_<1.30)) {
	directionOut_=-directionOut_;
	if (directionOut_<0&&abcDj_[sequenceOut_]<0.0)
	  upperTheta_=-abcDj_[sequenceOut_];
	else if (directionOut_>0&&abcDj_[sequenceOut_]>0.0)
	  upperTheta_=abcDj_[sequenceOut_];
	else
	  upperTheta_=1.0e30;
	CoinPartitionedVector & tableauRow = usefulArray_[arrayForTableauRow_];
	CoinPartitionedVector & candidateList = usefulArray_[arrayForDualColumn_];
	assert (!candidateList.getNumElements());
	tableauRow.compact();
	candidateList.compact();
	// redo candidate list
	int *  COIN_RESTRICT index = tableauRow.getIndices();
	double *  COIN_RESTRICT array = tableauRow.denseVector();
	double *  COIN_RESTRICT arrayCandidate=candidateList.denseVector();
	int *  COIN_RESTRICT indexCandidate = candidateList.getIndices();
	const double *  COIN_RESTRICT abcDj = abcDj_;
	const unsigned char *  COIN_RESTRICT internalStatus = internalStatus_;
	// do first pass to get possibles
	double bestPossible = 0.0;
	// We can also see if infeasible or pivoting on free
	double tentativeTheta = 1.0e25; // try with this much smaller as guess
	double acceptablePivot = currentAcceptablePivot_;
	double dualT=-currentDualTolerance_;
	// fixed will have been taken out by now
	const double multiplier[] = { 1.0, -1.0};
	int freeSequence=-1;
	double freeAlpha=0.0;
	int numberNonZero=tableauRow.getNumElements();
	int numberRemaining=0;
	if (ordinaryVariables()) {
	  for (int i = 0; i < numberNonZero; i++) {
	    int iSequence = index[i];
	    double tableauValue=array[i];
	    unsigned char iStatus=internalStatus[iSequence]&7;
	    double mult = multiplier[iStatus];
	    double alpha = tableauValue * mult;
	    double oldValue = abcDj[iSequence] * mult;
	    double value = oldValue - tentativeTheta * alpha;
	    if (value < dualT) {
	      bestPossible = CoinMax(bestPossible, alpha);
	      value = oldValue - upperTheta_ * alpha;
	      if (value < dualT && alpha >= acceptablePivot) {
		upperTheta_ = (oldValue - dualT) / alpha;
	      }
	      // add to list
	      arrayCandidate[numberRemaining] = alpha;
	      indexCandidate[numberRemaining++] = iSequence;
	    }
	  }
	} else {
	  double badFree = 0.0;
	  freeAlpha = currentAcceptablePivot_;
	  double currentDualTolerance = currentDualTolerance_;
	  for (int i = 0; i < numberNonZero; i++) {
	    int iSequence = index[i];
	    double tableauValue=array[i];
	    unsigned char iStatus=internalStatus[iSequence]&7;
	    if ((iStatus&4)==0) {
	      double mult = multiplier[iStatus];
	      double alpha = tableauValue * mult;
	      double oldValue = abcDj[iSequence] * mult;
	      double value = oldValue - tentativeTheta * alpha;
	      if (value < dualT) {
		bestPossible = CoinMax(bestPossible, alpha);
		value = oldValue - upperTheta_ * alpha;
		if (value < dualT && alpha >= acceptablePivot) {
		  upperTheta_ = (oldValue - dualT) / alpha;
		}
		// add to list
		arrayCandidate[numberRemaining] = alpha;
		indexCandidate[numberRemaining++] = iSequence;
	      }
	    } else {
	      bool keep;
	      bestPossible = CoinMax(bestPossible, fabs(tableauValue));
	      double oldValue = abcDj[iSequence];
	      // If free has to be very large - should come in via dualRow
	      //if (getInternalStatus(iSequence+addSequence)==isFree&&fabs(tableauValue)<1.0e-3)
	      //break;
	      if (oldValue > currentDualTolerance) {
		keep = true;
	      } else if (oldValue < -currentDualTolerance) {
		keep = true;
	      } else {
		if (fabs(tableauValue) > CoinMax(10.0 * acceptablePivot, 1.0e-5)) {
		  keep = true;
		} else {
		  keep = false;
		  badFree = CoinMax(badFree, fabs(tableauValue));
		}
	      }
	      if (keep) {
		// free - choose largest
		if (fabs(tableauValue) > fabs(freeAlpha)) {
		  freeAlpha = tableauValue;
		  freeSequence = iSequence;
		}
	      }
	    }
	  }
	}
	candidateList.setNumElements(numberRemaining);
	if (freeSequence>=0) {
	  sequenceIn_=freeSequence;
	} else if (fabs(upperTheta_-fabs(abcDj_[sequenceOut_]))<dualTolerance_) {
	  // can just move
	  stateOfIteration_=-1;
	  return;
	}
	dualColumn2();
      }
    }
    // redo dualOut_
    if (directionOut_<0) {
      dualOut_ = valueOut_ - upperOut_;
    } else {
      dualOut_ = lowerOut_ - valueOut_;
    }
  }
  usefulArray_[arrayForDualColumn_].clear();
  //clearArrays(arrayForDualColumn_);
  if (sequenceIn_ >= 0&&upperTheta_==COIN_DBL_MAX) {
    lowerIn_ = abcLower_[sequenceIn_];
    upperIn_ = abcUpper_[sequenceIn_];
    valueIn_ = abcSolution_[sequenceIn_];
    dualIn_ = abcDj_[sequenceIn_];
    if (valueIn_<lowerIn_+primalTolerance_) {
      directionIn_ = 1;
    } else if (valueIn_>upperIn_-primalTolerance_) {
      directionIn_ = -1;
    } else {
      if (alpha_ < 0.0) {
	// as if from upper bound
	directionIn_ = -1;
	//assert(upperIn_ == valueIn_);
      } else {
	// as if from lower bound
	directionIn_ = 1;
	//assert(lowerIn_ == valueIn_);
      }
    }
  }
#if ABC_DEBUG
  checkArrays(4);
#endif
#if CAN_HAVE_ZERO_OBJ>1
  if ((specialOptions_&2097152)!=0) 
    theta_=0.0;
#endif
  movement_=0.0;
  objectiveChange_=0.0;
  /*
    0 - take iteration
    1 - don't take but continue
    2 - don't take and break
  */
  btranAlpha_ = -alpha_ * directionOut_; // for check
}
void
AbcSimplexDual::getTableauColumnPart2()
{
#if ABC_PARALLEL
  if (parallelMode_!=0) {
    abcFactorization_->updateColumnFTPart2(usefulArray_[arrayForFtran_]);
    // pivot element
    alpha_ = usefulArray_[arrayForFtran_].denseVector()[pivotRow_];
  }
#endif
}
void 
AbcSimplexDual::replaceColumnPart3()
{
  abcFactorization_->replaceColumnPart3(this,
					&usefulArray_[arrayForReplaceColumn_],
					&usefulArray_[arrayForFtran_],
					lastPivotRow_,
					ftAlpha_?ftAlpha_:alpha_);
}
void
AbcSimplexDual::checkReplacePart1()
{
  //abcFactorization_->checkReplacePart1a(&usefulArray_[arrayForReplaceColumn_],pivotRow_);
  //usefulArray_[arrayForReplaceColumn_].print();
  ftAlpha_=abcFactorization_->checkReplacePart1(&usefulArray_[arrayForReplaceColumn_],pivotRow_);
}
#if 0
void
AbcSimplexDual::checkReplacePart1a()
{
  abcFactorization_->checkReplacePart1a(&usefulArray_[arrayForReplaceColumn_],pivotRow_);
}
void
AbcSimplexDual::checkReplacePart1b()
{
  //usefulArray_[arrayForReplaceColumn_].print();
  ftAlpha_=abcFactorization_->checkReplacePart1b(&usefulArray_[arrayForReplaceColumn_],pivotRow_);
}
#endif
/*
  returns 
  0 - OK
  1 - take iteration then re-factorize
  2 - flag something and skip
  3 - break and re-factorize
  5 - take iteration then re-factorize because of memory
 */
int 
AbcSimplexDual::checkReplace()
{
  int returnCode=0;
  if (!stateOfIteration_) {
    int saveStatus=problemStatus_;
    // check update
#if MOVE_REPLACE_PART1A < 0
    checkReplacePart1();
#endif
    int updateStatus=abcFactorization_->checkReplacePart2(pivotRow_,btranAlpha_,alpha_,ftAlpha_);
#if ABC_DEBUG
    checkArrays();
#endif
    // If looks like bad pivot - refactorize
    if (fabs(dualOut_) > 1.0e50)
      updateStatus = 2;
    // if no pivots, bad update but reasonable alpha - take and invert
    if (updateStatus == 2 &&
	!abcFactorization_->pivots() && fabs(alpha_) > 1.0e-5)
      updateStatus = 4;
    if (updateStatus == 1 || updateStatus == 4) {
      // slight error
      if (abcFactorization_->pivots() > 5 || updateStatus == 4) {
	problemStatus_ = -2; // factorize now
	returnCode = -3;
      }
    } else if (updateStatus == 2) {
      // major error
      // later we may need to unwind more e.g. fake bounds
      if (abcFactorization_->pivots() &&
	  ((moreSpecialOptions_ & 16) == 0 || abcFactorization_->pivots() > 4)) {
	problemStatus_ = -2; // factorize now
	returnCode = -2;
	moreSpecialOptions_ |= 16;
	stateOfIteration_=2;
      } else {
	assert (sequenceOut_>=0);
	// need to reject something
	char x = isColumn(sequenceOut_) ? 'C' : 'R';
	handler_->message(CLP_SIMPLEX_FLAG, messages_)
	  << x << sequenceWithin(sequenceOut_)
	  << CoinMessageEol;
#if ABC_NORMAL_DEBUG>0
	printf("BAD flag b %g pivotRow %d (seqout %d) sequenceIn %d\n", alpha_,pivotRow_,
	       sequenceOut_,sequenceIn_);
#endif
	setFlagged(sequenceOut_);
	// so can't happen immediately again
	sequenceOut_=-1;
	//abcProgress_.clearBadTimes();
	lastBadIteration_ = numberIterations_; // say be more cautious
	//clearArrays(-1); 
	//if(primalSolutionUpdated) 
	//gutsOfSolution(2);
	stateOfIteration_=1;
      }
    } else if (updateStatus == 3) {
      // out of memory
      // increase space if not many iterations
      if (abcFactorization_->pivots() <
	  0.5 * abcFactorization_->maximumPivots() &&
	  abcFactorization_->pivots() < 200)
	abcFactorization_->areaFactor(
				      abcFactorization_->areaFactor() * 1.1);
      problemStatus_ = -2; // factorize now
    } else if (updateStatus == 5) {
      problemStatus_ = -2; // factorize now
    }
    if (theta_ < 0.0) {
#if ABC_NORMAL_DEBUG>3
      if (handler_->logLevel() & 32)
	printf("negative theta %g\n", theta_);
#endif
      theta_ = 0.0;
    }
    //printf("check pstatus %d ustatus %d returncode %d nels %d\n",
    //	   problemStatus_,updateStatus,returnCode,usefulArray_[arrayForReplaceColumn_].getNumElements());
    if (saveStatus!=-1)
      problemStatus_=saveStatus;
  }
  return returnCode;
}
int 
AbcSimplexDual::noPivotColumn()
{
  // no incoming column is valid
  int returnCode=0;
  if (abcFactorization_->pivots() < 5 && acceptablePivot_ > 1.0e-8)
    acceptablePivot_ = 1.0e-8;
  returnCode = 1;
#if ABC_NORMAL_DEBUG>3
  if (handler_->logLevel() & 32)
    printf("** no column pivot\n");
#endif
  if (abcFactorization_->pivots() <=dontFactorizePivots_ && acceptablePivot_ <= 1.0e-8 &&
      lastFirstFree_<0) {
    // If not in branch and bound etc save ray
    delete [] ray_;
    if ((specialOptions_&(1024 | 4096)) == 0 || (specialOptions_ & 32) != 0) {
      // create ray anyway
      ray_ = new double [ numberRows_];
      setUsedArray(0);
      usefulArray_[arrayForBtran_].createOneUnpackedElement(pivotRow_, -directionOut_);
      abcFactorization_->updateColumnTranspose(usefulArray_[arrayForBtran_]);
      CoinMemcpyN(usefulArray_[arrayForBtran_].denseVector(), numberRows_, ray_);
      clearArrays(0);
    } else {
      ray_ = NULL;
    }
    // If we have just factorized and infeasibility reasonable say infeas
    double dualTest = ((specialOptions_ & 4096) != 0) ? 1.0e8 : 1.0e13;
    if (largestGap_)
      dualTest=CoinMin(largestGap_,dualTest);
    // but ignore if none at fake bound
    if (currentDualBound_<dualTest) {
      if (!numberAtFakeBound())
	dualTest=1.0e-12;
    }
    if (((specialOptions_ & 4096) != 0 || fabs(alpha_) < 1.0e-11) && currentDualBound_ >= dualTest) {
      double testValue = 1.0e-4;
      if (!abcFactorization_->pivots() && numberPrimalInfeasibilities_ == 1)
	testValue = 1.0e-6;
      if (valueOut_ > upperOut_ + testValue || valueOut_ < lowerOut_ - testValue
	  || (specialOptions_ & 64) == 0) {
	// say infeasible
	problemStatus_ = 1;
	// unless primal feasible!!!!
	//#define TEST_CLP_NODE
#ifndef TEST_CLP_NODE
	// Should be correct - but ...
	int numberFake = numberAtFakeBound();
	double sumPrimal =  (!numberFake) ? 2.0e5 : sumPrimalInfeasibilities_;
	if (sumPrimalInfeasibilities_ < 1.0e-3 || sumDualInfeasibilities_ > 1.0e-5 ||
	    (sumPrimal < 1.0e5 && (specialOptions_ & 1024) != 0 && abcFactorization_->pivots())) {
	  if (sumPrimal > 50.0 && abcFactorization_->pivots() > 2) {
	    problemStatus_ = -4;
	  } else {
	    problemStatus_ = 10;
	    // Get rid of objective
	    if ((specialOptions_ & 16384) == 0)
	      objective_ = new ClpLinearObjective(NULL, numberColumns_);
	  }
	}
#else
	if (sumPrimalInfeasibilities_ < 1.0e-3 || sumDualInfeasibilities_ > 1.0e-6) {
	  if ((specialOptions_ & 1024) != 0 && abcFactorization_->pivots()) {
	    problemStatus_ = 10;
	    // Get rid of objective
	    if ((specialOptions_ & 16384) == 0)
	      objective_ = new ClpLinearObjective(NULL, numberColumns_);
	  }
	}
#endif
	returnCode = 1;
	stateOfIteration_=2;
      }
    }
    if (abcFactorization_->pivots() != 0) {
      // If special option set - put off as long as possible
      if ((specialOptions_ & 64) == 0 || (moreSpecialOptions_ & 64) != 0) {
	if (numberTimesOptimal_<3&&abcFactorization_->pivots()>dontFactorizePivots_)
	  problemStatus_ = -4; //say looks infeasible
	else
	  problemStatus_ = 1; //say looks really infeasible
      } else {
	assert (sequenceOut_>=0);
	// flag
	char x = isColumn(sequenceOut_) ? 'C' : 'R';
	handler_->message(CLP_SIMPLEX_FLAG, messages_)
	  << x << sequenceWithin(sequenceOut_)
	  << CoinMessageEol;
#if ABC_NORMAL_DEBUG>3
	printf("flag c\n");
#endif
	setFlagged(sequenceOut_);
	lastBadIteration_ = numberIterations_; // say be more cautious
	// so can't happen immediately again
	sequenceOut_=-1;
	if (!abcFactorization_->pivots()) {
	  stateOfIteration_=1;
	}
      }
    }
  }
  pivotRow_ = -1;
  lastPivotRow_=-1;
  return returnCode;
}
#if 0
#include "CoinTime.hpp"
// Timed wait in nanoseconds - if negative then seconds
static int
timedWait(AbcSimplexDual * dual,int time, int which)
{
  pthread_mutex_lock(dual->mutexPointer(which));
  struct timespec absTime;
  clock_gettime(CLOCK_REALTIME, &absTime);
  if (time > 0) {
    absTime.tv_nsec += time;
    if (absTime.tv_nsec >= 1000000000) {
      absTime.tv_nsec -= 1000000000;
      absTime.tv_sec++;
    }
  } else {
    absTime.tv_sec -= time;
  }
  int rc = 0;
  while (! dual->threadInfoPointer()->status && rc == 0) {
    printf("waiting for signal on %d\n",which); 
    rc = pthread_cond_timedwait(dual->conditionPointer(which), dual->mutexPointer(which), &absTime);
  }
  if (rc == 0) 
    rc=dual->threadInfoPointer()->status;
  printf("got signal %d on %d\n",rc,which); 
  //pthread_mutex_unlock(dual->mutexPointer(which));
  return rc;
}
#endif
#if ABC_PARALLEL==1
void * abc_parallelManager(void * simplex)
{
  AbcSimplexDual * dual = reinterpret_cast<AbcSimplexDual *>(simplex);
  int whichThread=dual->whichThread();
  CoinAbcThreadInfo * threadInfo = dual->threadInfoPointer(whichThread);
  pthread_mutex_lock(dual->mutexPointer(2,whichThread));
  pthread_barrier_wait(dual->barrierPointer());
#if 0
  int status=-1;
  while (status!=100)
    status=timedWait(dual,1000,2);
  pthread_cond_signal(dual->conditionPointer(1));
  pthread_mutex_unlock(dual->mutexPointer(1,whichThread));
#endif
  // so now mutex_ is locked
  int whichLocked=0;
  while (true) {
    pthread_mutex_t * mutexPointer = dual->mutexPointer(whichLocked,whichThread);
    // wait
    //printf("Child waiting for %d - status %d %d %d\n",
    //	   whichLocked,lockedX[0],lockedX[1],lockedX[2]);
    pthread_mutex_lock (mutexPointer);
    whichLocked++;
    if (whichLocked==3)
      whichLocked=0;
    int unLock=whichLocked+1;
    if (unLock==3)
      unLock=0;
    //printf("child pointer %x status %d\n",threadInfo,threadInfo->status);
    assert(threadInfo->status>=0);
    if (threadInfo->status==1000)
      pthread_exit(NULL);
    int type=threadInfo->status;
    int * which = threadInfo->stuff;
    int & returnCode=which[0];
    CoinIndexedVector * array;
    double dummy;
    switch(type) {
      // dummy
    case 0:
      break;
    case 1:
      returnCode=computeDualsAndCheck(dual,which);
      break;
    case 2:
      array=dual->usefulArray(dual->arrayForBtran()); // assuming not big overlap
      dual->dualRowPivot()->updateWeightsOnly(*array);
      //array->checkClean();
      break;
    case 3:
      //dual->replaceColumnPart3();
      dual->updateDualsInDual();
      break;
    case 4:
      which[2]=-1;
      dual->abcMatrix()->dualColumn1Part(which[1],which[2],threadInfo->result,
					 *dual->usefulArray(dual->arrayForBtran()),
					 *dual->usefulArray(dual->arrayForTableauRow()),
					 *dual->usefulArray(dual->arrayForDualColumn()));
      break;
    case 5:
      which[2]=-1;
      threadInfo->result=
	dual->abcMatrix()->dualColumn1Row(which[1],COIN_DBL_MAX,which[2],
					  *dual->usefulArray(dual->arrayForBtran()),
					  *dual->usefulArray(dual->arrayForTableauRow()),
					  *dual->usefulArray(dual->arrayForDualColumn()));
      break;
    case 6:// not used
      dual->getTableauColumnPart2();
      break;
    case 7: 
	dual->checkReplacePart1();
	//dual->checkReplacePart1a();
	//dual->checkReplacePart1b();
      break;
    case 8:
      which[2] = dual->flipBounds();
      break;
    case 9:
      dual->replaceColumnPart3();
      break;
      
    case 100:
      // initialization
      break;
    }
    threadInfo->status=-1;
    pthread_mutex_unlock (dual->mutexPointer(unLock,whichThread));
  }
}
void 
AbcSimplex::startParallelStuff(int type)
{
  /*
    first time 0,1 owned by main 2 by child
    at end of cycle should be 1,2 by main 0 by child then 2,0 by main 1 by child
  */
  int iStart=1;
  for (int i=0;i<NUMBER_THREADS;i++) {
    if ((stopStart_&iStart)!=0) {
      threadInfo_[i].status=type;
      pthread_mutex_unlock (&mutex_[locked_[i]+3*i]);
    }
    iStart = iStart<<1;
  }
}
#define NUMBER_PARALLEL_TASKS 10
//static int count_x[NUMBER_PARALLEL_TASKS]={0,0,0,0,0,0,0,0,0,0};
//static int count_y[NUMBER_PARALLEL_TASKS]={0,0,0,0,0,0,0,0,0,0};
int
AbcSimplex::stopParallelStuff(int type)
{
  //if (threadInfo_[0].status==-1)
  //count_y[type-1]++;
  //count_x[type-1]++;
  //printf("XXStop type %d iteration %d\n",type,numberIterations_);
  //if (threadInfo_[0].status!=-1) {
  //printf("Main waiting for %d - status %d\n",
  //	   locked,type);
  //pthread_mutex_lock(&mutex_[locked]);
  int iStop=32;
  for (int i=0;i<NUMBER_THREADS;i++) {
    if ((stopStart_&iStop)!=0) {
      int locked=locked_[i]+2;
      if (locked>=3)
	locked-=3;
      pthread_mutex_lock(&mutex_[locked+i*3]);
      locked_[i]++;
      if (locked_[i]==3)
	locked_[i]=0;
    }
    iStop = iStop<<1;
  }
  return threadInfo_[0].stuff[0];
}
#endif
//#define PRINT_WEIGHTS_FREQUENCY
#ifdef PRINT_WEIGHTS_FREQUENCY
extern int xweights1;
extern int xweights2;
extern int xweights3;
#endif
//static int whichModel=0;
int
AbcSimplexDual::dual()
{
  //handler_->setLogLevel(63);
#if 0
  whichModel++;
  printf("STARTing dual %d rows - model %d\n",numberRows_,whichModel);
  char modelName[20];
  sprintf(modelName,"model%d.mps",whichModel);
  writeMps(modelName,2);
  sprintf(modelName,"model%d.bas",whichModel);
  writeBasis(modelName,true,2);
#endif
  bestObjectiveValue_ = -COIN_DBL_MAX;
  algorithm_ = -1;
  moreSpecialOptions_ &= ~16; // clear check replaceColumn accuracy
  normalDualColumnIteration_=0;
  problemStatus_=-1;
  numberIterations_=baseIteration_;
  //baseIteration_=0;
  if (!lastDualBound_) {
    dualTolerance_ = dblParam_[ClpDualTolerance];
    primalTolerance_ = dblParam_[ClpPrimalTolerance];
    int tryType=moreSpecialOptions_/65536;
    if (tryType<5) {
      currentDualBound_=1.0e8;
    } else {
      currentDualBound_=1.0e4;
      tryType -=5;
    }
    currentDualBound_=CoinMin(currentDualBound_,dualBound_);
#if ABC_NORMAL_DEBUG>0
    printf("dual bound %g (initialize)\n",currentDualBound_);
#endif
    switch(tryType) {
    default:
      currentDualTolerance_=dualTolerance_;
      stateDualColumn_=0;
      break;
    case 1:
      currentDualTolerance_=CoinMin(1.0e-1,dualTolerance_*10.0);
      stateDualColumn_=-1;
      break;
    case 2:
      currentDualTolerance_=CoinMin(1.0e-1,dualTolerance_*100.0);
      stateDualColumn_=-1;
      break;
    case 3:
      currentDualTolerance_=CoinMin(1.0e-1,dualTolerance_*10000.0);
      stateDualColumn_=-1; // was -2 but that doesn't seem to work
      break;
    case 4:
      currentDualTolerance_=CoinMin(1.0e-1,dualTolerance_*1000000.0);
      stateDualColumn_=-1; // was -2 but that doesn't seem to work
      break;
    }
  } else {
    // done before
    currentDualBound_=1.1*lastDualBound_;
    currentDualTolerance_=dualTolerance_;
#if ABC_NORMAL_DEBUG>0
    printf("new dual bound %g (resolve)\n",currentDualBound_);
#endif
  }
  // save data
  saveData_ = saveData();
  // initialize 
  copyFromSaved();
  // temp
  if (fabs(primalTolerance()-1.001e-6)<1.0e-12&&false) {
    printf("trying idiot idea - need scaled matrix\n");
    primalTolerance_=1.0e-6;
    //for (int i=0;i<numberRows_;i++)
    //cost_[i]=-cost_[i];
    abcMatrix_->transposeTimesAll(cost_,abcCost_);
    delete [] cost_;
    cost_=NULL;
    CoinAbcMemcpy(costSaved_,abcCost_,numberTotal_);
    CoinAbcMemcpy(abcDj_,abcCost_,numberTotal_);
    static_cast<AbcSimplexDual *> (this)->makeNonFreeVariablesDualFeasible();
    const double multiplier[] = { 1.0, -1.0};
    double dualT = - currentDualTolerance_;
    double *  COIN_RESTRICT solution = abcSolution_;
    double *  COIN_RESTRICT lower = abcLower_;
    double *  COIN_RESTRICT upper = abcUpper_;
    for (int iSequence = 0; iSequence < numberTotal_; iSequence++) {
      int iStatus = internalStatus_[iSequence] & 7;
      if (iStatus<4) {
	double lowerValue=lower[iSequence];
	double upperValue=upper[iSequence];
	if (lowerValue>-1.0e10||upperValue<1.0e10) {
	  double mult = multiplier[iStatus];
	  double dj = abcDj_[iSequence] * mult;
	  if (dj < dualT) 
	    iStatus=1-iStatus;
	  if (lowerValue<=-1.0e10)
	    iStatus=1;
	  else if (upperValue>=1.0e10)
	    iStatus=0;
	  if (iStatus==0) {
	    setInternalStatus(iSequence, atLowerBound);
	    solution[iSequence] = lower[iSequence];
	  } else {
	    setInternalStatus(iSequence, atUpperBound);
	    solution[iSequence] = upper[iSequence];
	  }
	}
	assert (fabs(solution[iSequence])<1.0e10);
      }
    }
    CoinAbcMemcpy(solutionSaved_,abcSolution_,numberTotal_);
  }
  numberFlagged_=0;
#ifndef NDEBUG
  for (int iRow = 0; iRow < numberRows_; iRow++) {
    int iPivot = abcPivotVariable_[iRow];
    assert (!flagged(iPivot));
  }
#endif
#if ABC_PARALLEL==1
  // redo to test on parallelMode_
  if (parallelMode_!=0) {
    // For waking up thread
    memset(mutex_,0,sizeof(mutex_));
    for (int iThread=0;iThread<NUMBER_THREADS;iThread++) {
      for (int i=0;i<3;i++) {
	pthread_mutex_init(&mutex_[i+3*iThread], NULL);
	if (i<2)
	  pthread_mutex_lock(&mutex_[i+3*iThread]);
      }
      threadInfo_[iThread].status = 100;
    }
    //pthread_barrierattr_t attr;
    pthread_barrier_init(&barrier_, /*&attr*/ NULL, NUMBER_THREADS+1); 
    for (int iThread=0;iThread<NUMBER_THREADS;iThread++) {
      pthread_create(&abcThread_[iThread], NULL, abc_parallelManager, reinterpret_cast<void *>(this));
    }
    pthread_barrier_wait(&barrier_);
    pthread_barrier_destroy(&barrier_);
    for (int iThread=0;iThread<NUMBER_THREADS;iThread++) {
      threadInfo_[iThread].status = 0;
    }
    for (int i=0;i<NUMBER_THREADS;i++) {
      locked_[i]=0; 
    }
  }
#endif
#if ABC_PARALLEL==2
  abcFactorization_->setParallelMode(parallelMode_);
#endif
  static_cast<AbcSimplexDual *>(this)->bounceTolerances(-1);
  int saveDont = dontFactorizePivots_;
  if ((specialOptions_ & 2048) == 0)
    dontFactorizePivots_ = 0;
  else if(!dontFactorizePivots_)
    dontFactorizePivots_ = 20;
  if (alphaAccuracy_ != -1.0)
    alphaAccuracy_ = 1.0;
  startupSolve();
  lastPivotRow_=-1;
  pivotRow_=-1;
  // Save so can see if doing after primal
  swappedAlgorithm_ = (problemStatus_==10) ? 10 : 0;
  if (problemStatus_) {
#ifdef PRINT_WEIGHTS_FREQUENCY
    xweights1=0;
    xweights2=0;
    xweights3=0;
#endif
    numberFreeNonBasic_=0;
    gutsOfDual();
#ifdef PRINT_WEIGHTS_FREQUENCY
    if (dynamic_cast<AbcDualRowSteepest *>(abcDualRowPivot_)) {
      printf("ZZZZ %d iterations weights updated %d times\n",numberIterations_,xweights1+xweights2+xweights3);
      assert(numberIterations_==xweights1+xweights2+xweights3);
    }
#endif
  }
  if (!problemStatus_) {
    // see if cutoff reached
    double limit = 0.0;
    getDblParam(ClpDualObjectiveLimit, limit);
    if(fabs(limit) < 1.0e30 && minimizationObjectiveValue() >
       limit + 1.0e-7 + 1.0e-8 * fabs(limit)) {
      // actually infeasible on objective
      problemStatus_ = 1;
      secondaryStatus_ = 1;
    }
  }
  // If infeasible but primal errors - try dual
  if (problemStatus_==1 && numberPrimalInfeasibilities_) {
    bool inCbcOrOther = (specialOptions_ & 0x03000000) != 0;
    double factor = (!inCbcOrOther) ? 1.0 : 0.3;
    double averageInfeasibility = sumPrimalInfeasibilities_/
      static_cast<double>(numberPrimalInfeasibilities_);
    if (averageInfeasibility<factor*largestPrimalError_)
      problemStatus_= 10;
  }
  
  finishSolve();
  
  // Restore any saved stuff
  restoreData(saveData_);
  dontFactorizePivots_ = saveDont;
  if (problemStatus_ == 3) {
    rawObjectiveValue_=CoinMax(bestObjectiveValue_,rawObjectiveValue_-bestPossibleImprovement_);
    setClpSimplexObjectiveValue();
  }
#if ABC_PARALLEL==1
  if (parallelMode_!=0) {
    //for (int i=0;i<NUMBER_PARALLEL_TASKS;i++) {
    //if (count_x[i]||count_y[i])
    //printf("For parallel task %d spawned finished first %d times out of %d\n",
    //	     i+1,count_y[i],count_x[i]);
    //count_y[i]=count_x[i]=0;
    //}
    stopStart_=31+32*31; // all
    startParallelStuff(1000);
    for (int iThread=0;iThread<NUMBER_THREADS;iThread++) {
      pthread_join(abcThread_[iThread],NULL);
      for (int i=0;i<3;i++) {
	pthread_mutex_destroy (&mutex_[i+3*iThread]);
      }
    }
  }
#endif
  return problemStatus_;
}
