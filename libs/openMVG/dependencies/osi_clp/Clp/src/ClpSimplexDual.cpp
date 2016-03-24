/* $Id$ */
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
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
#ifdef COIN_DEVELOP
#undef COIN_DEVELOP
#define COIN_DEVELOP 2
#endif

#include "CoinPragma.hpp"

#include <math.h>

#include "CoinHelperFunctions.hpp"
#include "ClpHelperFunctions.hpp"
#include "ClpSimplexDual.hpp"
#include "ClpEventHandler.hpp"
#include "ClpFactorization.hpp"
#include "CoinPackedMatrix.hpp"
#include "CoinIndexedVector.hpp"
#include "CoinFloatEqual.hpp"
#include "ClpDualRowDantzig.hpp"
#include "ClpMessage.hpp"
#include "ClpLinearObjective.hpp"
#include <cfloat>
#include <cassert>
#include <string>
#include <stdio.h>
#include <iostream>
//#define CLP_DEBUG 1
// To force to follow another run put logfile name here and define
//#define FORCE_FOLLOW
#ifdef FORCE_FOLLOW
static FILE * fpFollow = NULL;
static char * forceFile = "old.log";
static int force_in = -1;
static int force_out = -1;
static int force_iteration = 0;
#endif
//#define VUB
#ifdef VUB
extern int * vub;
extern int * toVub;
extern int * nextDescendent;
#endif
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
int
ClpSimplexDual::startupSolve(int ifValuesPass, double * saveDuals, int startFinishOptions)
{
     // If values pass then save given duals round check solution
     // sanity check
     // initialize - no values pass and algorithm_ is -1
     // put in standard form (and make row copy)
     // create modifiable copies of model rim and do optional scaling
     // If problem looks okay
     // Do initial factorization
     // If user asked for perturbation - do it
     numberFake_ = 0; // Number of variables at fake bounds
     numberChanged_ = 0; // Number of variables with changed costs
     if (!startup(0, startFinishOptions)) {
          int usePrimal = 0;
          // looks okay
          // Superbasic variables not allowed
          // If values pass then scale pi
          if (ifValuesPass) {
               if (problemStatus_ && perturbation_ < 100)
                    usePrimal = perturb();
               int i;
               if (scalingFlag_ > 0) {
                    for (i = 0; i < numberRows_; i++) {
                         dual_[i] = saveDuals[i] * inverseRowScale_[i];
                    }
               } else {
                    CoinMemcpyN(saveDuals, numberRows_, dual_);
               }
               // now create my duals
               for (i = 0; i < numberRows_; i++) {
                    // slack
                    double value = dual_[i];
                    value += rowObjectiveWork_[i];
                    saveDuals[i+numberColumns_] = value;
               }
               CoinMemcpyN(objectiveWork_, numberColumns_, saveDuals);
               transposeTimes(-1.0, dual_, saveDuals);
               // make reduced costs okay
               for (i = 0; i < numberColumns_; i++) {
                    if (getStatus(i) == atLowerBound) {
                         if (saveDuals[i] < 0.0) {
                              //if (saveDuals[i]<-1.0e-3)
                              //printf("bad dj at lb %d %g\n",i,saveDuals[i]);
                              saveDuals[i] = 0.0;
                         }
                    } else if (getStatus(i) == atUpperBound) {
                         if (saveDuals[i] > 0.0) {
                              //if (saveDuals[i]>1.0e-3)
                              //printf("bad dj at ub %d %g\n",i,saveDuals[i]);
                              saveDuals[i] = 0.0;
                         }
                    }
               }
               CoinMemcpyN(saveDuals, (numberColumns_ + numberRows_), dj_);
               // set up possible ones
               for (i = 0; i < numberRows_ + numberColumns_; i++)
                    clearPivoted(i);
               int iRow;
               for (iRow = 0; iRow < numberRows_; iRow++) {
                    int iPivot = pivotVariable_[iRow];
                    if (fabs(saveDuals[iPivot]) > dualTolerance_) {
                         if (getStatus(iPivot) != isFree)
                              setPivoted(iPivot);
                    }
               }
          } else if ((specialOptions_ & 1024) != 0 && CLEAN_FIXED) {
               // set up possible ones
               for (int i = 0; i < numberRows_ + numberColumns_; i++)
                    clearPivoted(i);
               int iRow;
               for (iRow = 0; iRow < numberRows_; iRow++) {
                    int iPivot = pivotVariable_[iRow];
                    if (iPivot < numberColumns_ && lower_[iPivot] == upper_[iPivot]) {
                         setPivoted(iPivot);
                    }
               }
          }

          double objectiveChange;
          assert (!numberFake_);
          assert (numberChanged_ == 0);
          if (!numberFake_) // if nonzero then adjust
               changeBounds(1, NULL, objectiveChange);

          if (!ifValuesPass) {
               // Check optimal
               if (!numberDualInfeasibilities_ && !numberPrimalInfeasibilities_)
                    problemStatus_ = 0;
          }
          if (problemStatus_ < 0 && perturbation_ < 100) {
               bool inCbcOrOther = (specialOptions_ & 0x03000000) != 0;
               if (!inCbcOrOther)
                    usePrimal = perturb();
               // Can't get here if values pass
               gutsOfSolution(NULL, NULL);
#ifdef CLP_INVESTIGATE
               if (numberDualInfeasibilities_)
                    printf("ZZZ %d primal %d dual - sumdinf %g\n",
                           numberPrimalInfeasibilities_,
                           numberDualInfeasibilities_, sumDualInfeasibilities_);
#endif
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
               if (inCbcOrOther) {
                    if (numberPrimalInfeasibilities_) {
                         usePrimal = perturb();
                         if (perturbation_ >= 101) {
                              computeDuals(NULL);
                              //gutsOfSolution(NULL,NULL);
                              checkDualSolution(); // recompute objective
                         }
                    } else if (numberDualInfeasibilities_) {
                         problemStatus_ = 10;
                         if ((moreSpecialOptions_ & 32) != 0 && false)
                              problemStatus_ = 0; // say optimal!!
#if COIN_DEVELOP>2

                         printf("returning at %d\n", __LINE__);
#endif
                         return 1; // to primal
                    }
               }
          } else if (!ifValuesPass) {
               gutsOfSolution(NULL, NULL);
               // double check
               if (numberDualInfeasibilities_ || numberPrimalInfeasibilities_)
                    problemStatus_ = -1;
          }
          if (usePrimal) {
               problemStatus_ = 10;
#if COIN_DEVELOP>2
               printf("returning to use primal (no obj) at %d\n", __LINE__);
#endif
          }
          return usePrimal;
     } else {
          return 1;
     }
}
void
ClpSimplexDual::finishSolve(int startFinishOptions)
{
     assert (problemStatus_ || !sumPrimalInfeasibilities_);

     // clean up
     finish(startFinishOptions);
}
//#define CLP_REPORT_PROGRESS
#ifdef CLP_REPORT_PROGRESS
static int ixxxxxx = 0;
static int ixxyyyy = 90;
#endif
#ifdef CLP_INVESTIGATE_SERIAL
static int z_reason[7] = {0, 0, 0, 0, 0, 0, 0};
static int z_thinks = -1;
#endif
void
ClpSimplexDual::gutsOfDual(int ifValuesPass, double * & saveDuals, int initialStatus,
                           ClpDataSave & data)
{
#ifdef CLP_INVESTIGATE_SERIAL
     z_reason[0]++;
     z_thinks = -1;
     int nPivots = 9999;
#endif
     double largestPrimalError = 0.0;
     double largestDualError = 0.0;
     // Start can skip some things in transposeTimes
     specialOptions_ |= 131072;
     int lastCleaned = 0; // last time objective or bounds cleaned up

     // This says whether to restore things etc
     // startup will have factorized so can skip
     int factorType = 0;
     // Start check for cycles
     progress_.startCheck();
     // Say change made on first iteration
     changeMade_ = 1;
     // Say last objective infinite
     //lastObjectiveValue_=-COIN_DBL_MAX;
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
     while (problemStatus_ < 0) {
          int iRow, iColumn;
          // clear
          for (iRow = 0; iRow < 4; iRow++) {
               rowArray_[iRow]->clear();
          }

          for (iColumn = 0; iColumn < 2; iColumn++) {
               columnArray_[iColumn]->clear();
          }

          // give matrix (and model costs and bounds a chance to be
          // refreshed (normally null)
          matrix_->refresh(this);
          // If getting nowhere - why not give it a kick
          // does not seem to work too well - do some more work
          if (perturbation_ < 101 && numberIterations_ > 2 * (numberRows_ + numberColumns_) && (moreSpecialOptions_&1048576)==0
                    && initialStatus != 10) {
               perturb();
               // Can't get here if values pass
               gutsOfSolution(NULL, NULL);
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
          // see if in Cbc etc
          bool inCbcOrOther = (specialOptions_ & 0x03000000) != 0;
#if 0
          bool gotoPrimal = false;
          if (inCbcOrOther && numberIterations_ > disasterArea_ + numberRows_ &&
                    numberDualInfeasibilitiesWithoutFree_ && largestDualError_ > 1.0e-1) {
               if (!disasterArea_) {
                    printf("trying all slack\n");
                    // try all slack basis
                    allSlackBasis(true);
                    disasterArea_ = 2 * numberRows_;
               } else {
                    printf("going to primal\n");
                    // go to primal
                    gotoPrimal = true;
                    allSlackBasis(true);
               }
          }
#endif
          bool disaster = false;
          if (disasterArea_ && inCbcOrOther && disasterArea_->check()) {
               disasterArea_->saveInfo();
               disaster = true;
          }
          // may factorize, checks if problem finished
          statusOfProblemInDual(lastCleaned, factorType, saveDuals, data,
                                ifValuesPass);
          largestPrimalError = CoinMax(largestPrimalError, largestPrimalError_);
          largestDualError = CoinMax(largestDualError, largestDualError_);
          if (disaster)
               problemStatus_ = 3;
          // If values pass then do easy ones on first time
          if (ifValuesPass &&
                    progress_.lastIterationNumber(0) < 0 && saveDuals) {
               doEasyOnesInValuesPass(saveDuals);
          }

          // Say good factorization
          factorType = 1;
          if (data.sparseThreshold_) {
               // use default at present
               factorization_->sparseThreshold(0);
               factorization_->goSparse();
          }

          // exit if victory declared
          if (problemStatus_ >= 0)
               break;

          // test for maximum iterations
          if (hitMaximumIterations() || (ifValuesPass == 2 && !saveDuals)) {
               problemStatus_ = 3;
               break;
          }
          if (ifValuesPass && !saveDuals) {
               // end of values pass
               ifValuesPass = 0;
               int status = eventHandler_->event(ClpEventHandler::endOfValuesPass);
               if (status >= 0) {
                    problemStatus_ = 5;
                    secondaryStatus_ = ClpEventHandler::endOfValuesPass;
                    break;
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
          // Do iterations
          int returnCode = whileIterating(saveDuals, ifValuesPass);
	  if (problemStatus_ == 1 && (progressFlag_&8) != 0 &&
	      fabs(objectiveValue_) > 1.0e10 )
	    problemStatus_ = 10; // infeasible - but has looked feasible
#ifdef CLP_INVESTIGATE_SERIAL
          nPivots = factorization_->pivots();
#endif
          if (!problemStatus_ && factorization_->pivots())
               computeDuals(NULL); // need to compute duals
          if (returnCode == -2)
               factorType = 3;
     }
#ifdef CLP_INVESTIGATE_SERIAL
     // NOTE - can fail if parallel
     if (z_thinks != -1) {
          assert (z_thinks < 4);
          if ((!factorization_->pivots() && nPivots < 20) && z_thinks >= 0 && z_thinks < 2)
               z_thinks += 4;
          z_reason[1+z_thinks]++;
     }
     if ((z_reason[0] % 1000) == 0) {
          printf("Reason");
          for (int i = 0; i < 7; i++)
               printf(" %d", z_reason[i]);
          printf("\n");
     }
#endif
     // Stop can skip some things in transposeTimes
     specialOptions_ &= ~131072;
     largestPrimalError_ = largestPrimalError;
     largestDualError_ = largestDualError;
}
int
ClpSimplexDual::dual(int ifValuesPass, int startFinishOptions)
{
  //handler_->setLogLevel(63);
  //yprintf("STARTing dual %d rows\n",numberRows_);
     bestObjectiveValue_ = -COIN_DBL_MAX;
     algorithm_ = -1;
     moreSpecialOptions_ &= ~16; // clear check replaceColumn accuracy
     // save data
     ClpDataSave data = saveData();
     double * saveDuals = NULL;
     int saveDont = dontFactorizePivots_;
     if ((specialOptions_ & 2048) == 0)
          dontFactorizePivots_ = 0;
     else if(!dontFactorizePivots_)
          dontFactorizePivots_ = 20;
     if (ifValuesPass) {
          saveDuals = new double [numberRows_+numberColumns_];
          CoinMemcpyN(dual_, numberRows_, saveDuals);
     }
     if (alphaAccuracy_ != -1.0)
          alphaAccuracy_ = 1.0;
     int returnCode = startupSolve(ifValuesPass, saveDuals, startFinishOptions);
     // Save so can see if doing after primal
     int initialStatus = problemStatus_;
     if (!returnCode && !numberDualInfeasibilities_ &&
               !numberPrimalInfeasibilities_ && perturbation_ < 101) {
          returnCode = 1; // to skip gutsOfDual
          problemStatus_ = 0;
     }

     if (!returnCode)
          gutsOfDual(ifValuesPass, saveDuals, initialStatus, data);
     if (!problemStatus_) {
          // see if cutoff reached
          double limit = 0.0;
          getDblParam(ClpDualObjectiveLimit, limit);
          if(fabs(limit) < 1.0e30 && objectiveValue()*optimizationDirection_ >
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
       
     if (problemStatus_ == 10)
          startFinishOptions |= 1;
     finishSolve(startFinishOptions);
     delete [] saveDuals;

     // Restore any saved stuff
     restoreData(data);
     dontFactorizePivots_ = saveDont;
     if (problemStatus_ == 3)
          objectiveValue_ = CoinMax(bestObjectiveValue_, objectiveValue_ - bestPossibleImprovement_);
     return problemStatus_;
}
// old way
#if 0
int ClpSimplexDual::dual (int ifValuesPass , int startFinishOptions)
{
     algorithm_ = -1;

     // save data
     ClpDataSave data = saveData();
     // Save so can see if doing after primal
     int initialStatus = problemStatus_;

     // If values pass then save given duals round check solution
     double * saveDuals = NULL;
     if (ifValuesPass) {
          saveDuals = new double [numberRows_+numberColumns_];
          CoinMemcpyN(dual_, numberRows_, saveDuals);
     }
     // sanity check
     // initialize - no values pass and algorithm_ is -1
     // put in standard form (and make row copy)
     // create modifiable copies of model rim and do optional scaling
     // If problem looks okay
     // Do initial factorization
     // If user asked for perturbation - do it
     if (!startup(0, startFinishOptions)) {
          // looks okay
          // Superbasic variables not allowed
          // If values pass then scale pi
          if (ifValuesPass) {
               if (problemStatus_ && perturbation_ < 100)
                    perturb();
               int i;
               if (scalingFlag_ > 0) {
                    for (i = 0; i < numberRows_; i++) {
                         dual_[i] = saveDuals[i] * inverseRowScale_[i];
                    }
               } else {
                    CoinMemcpyN(saveDuals, numberRows_, dual_);
               }
               // now create my duals
               for (i = 0; i < numberRows_; i++) {
                    // slack
                    double value = dual_[i];
                    value += rowObjectiveWork_[i];
                    saveDuals[i+numberColumns_] = value;
               }
               CoinMemcpyN(objectiveWork_, numberColumns_, saveDuals);
               transposeTimes(-1.0, dual_, saveDuals);
               // make reduced costs okay
               for (i = 0; i < numberColumns_; i++) {
                    if (getStatus(i) == atLowerBound) {
                         if (saveDuals[i] < 0.0) {
                              //if (saveDuals[i]<-1.0e-3)
                              //printf("bad dj at lb %d %g\n",i,saveDuals[i]);
                              saveDuals[i] = 0.0;
                         }
                    } else if (getStatus(i) == atUpperBound) {
                         if (saveDuals[i] > 0.0) {
                              //if (saveDuals[i]>1.0e-3)
                              //printf("bad dj at ub %d %g\n",i,saveDuals[i]);
                              saveDuals[i] = 0.0;
                         }
                    }
               }
               CoinMemcpyN(saveDuals, numberColumns_ + numberRows_, dj_);
               // set up possible ones
               for (i = 0; i < numberRows_ + numberColumns_; i++)
                    clearPivoted(i);
               int iRow;
               for (iRow = 0; iRow < numberRows_; iRow++) {
                    int iPivot = pivotVariable_[iRow];
                    if (fabs(saveDuals[iPivot]) > dualTolerance_) {
                         if (getStatus(iPivot) != isFree)
                              setPivoted(iPivot);
                    }
               }
          } else if ((specialOptions_ & 1024) != 0 && CLEAN_FIXED) {
               // set up possible ones
               for (int i = 0; i < numberRows_ + numberColumns_; i++)
                    clearPivoted(i);
               int iRow;
               for (iRow = 0; iRow < numberRows_; iRow++) {
                    int iPivot = pivotVariable_[iRow];
                    if (iPivot < numberColumns_ && lower_[iPivot] == upper_[iPivot]) {
                         setPivoted(iPivot);
                    }
               }
          }

          double objectiveChange;
          numberFake_ = 0; // Number of variables at fake bounds
          numberChanged_ = 0; // Number of variables with changed costs
          changeBounds(1, NULL, objectiveChange);

          int lastCleaned = 0; // last time objective or bounds cleaned up

          if (!ifValuesPass) {
               // Check optimal
               if (!numberDualInfeasibilities_ && !numberPrimalInfeasibilities_)
                    problemStatus_ = 0;
          }
          if (problemStatus_ < 0 && perturbation_ < 100) {
               perturb();
               // Can't get here if values pass
               gutsOfSolution(NULL, NULL);
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

          // This says whether to restore things etc
          // startup will have factorized so can skip
          int factorType = 0;
          // Start check for cycles
          progress_.startCheck();
          // Say change made on first iteration
          changeMade_ = 1;
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
          while (problemStatus_ < 0) {
               int iRow, iColumn;
               // clear
               for (iRow = 0; iRow < 4; iRow++) {
                    rowArray_[iRow]->clear();
               }

               for (iColumn = 0; iColumn < 2; iColumn++) {
                    columnArray_[iColumn]->clear();
               }

               // give matrix (and model costs and bounds a chance to be
               // refreshed (normally null)
               matrix_->refresh(this);
               // If getting nowhere - why not give it a kick
               // does not seem to work too well - do some more work
               if (perturbation_ < 101 && numberIterations_ > 2 * (numberRows_ + numberColumns_) && (moreSpecialOptions_&1048576)==0
                         && initialStatus != 10) {
                    perturb();
                    // Can't get here if values pass
                    gutsOfSolution(NULL, NULL);
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
               // may factorize, checks if problem finished
               statusOfProblemInDual(lastCleaned, factorType, saveDuals, data,
                                     ifValuesPass);
               // If values pass then do easy ones on first time
               if (ifValuesPass &&
                         progress_.lastIterationNumber(0) < 0 && saveDuals) {
                    doEasyOnesInValuesPass(saveDuals);
               }

               // Say good factorization
               factorType = 1;
               if (data.sparseThreshold_) {
                    // use default at present
                    factorization_->sparseThreshold(0);
                    factorization_->goSparse();
               }

               // exit if victory declared
               if (problemStatus_ >= 0)
                    break;

               // test for maximum iterations
               if (hitMaximumIterations() || (ifValuesPass == 2 && !saveDuals)) {
                    problemStatus_ = 3;
                    break;
               }
               if (ifValuesPass && !saveDuals) {
                    // end of values pass
                    ifValuesPass = 0;
                    int status = eventHandler_->event(ClpEventHandler::endOfValuesPass);
                    if (status >= 0) {
                         problemStatus_ = 5;
                         secondaryStatus_ = ClpEventHandler::endOfValuesPass;
                         break;
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
               // Do iterations
               whileIterating(saveDuals, ifValuesPass);
          }
     }

     assert (problemStatus_ || !sumPrimalInfeasibilities_);

     // clean up
     finish(startFinishOptions);
     delete [] saveDuals;

     // Restore any saved stuff
     restoreData(data);
     return problemStatus_;
}
#endif
//#define CHECK_ACCURACY
#ifdef CHECK_ACCURACY
static double zzzzzz[100000];
#endif
/* Reasons to come out:
   -1 iterations etc
   -2 inaccuracy
   -3 slight inaccuracy (and done iterations)
   +0 looks optimal (might be unbounded - but we will investigate)
   +1 looks infeasible
   +3 max iterations
 */
int
ClpSimplexDual::whileIterating(double * & givenDuals, int ifValuesPass)
{
#ifdef CLP_INVESTIGATE_SERIAL
     z_thinks = -1;
#endif
#ifdef CLP_DEBUG
     int debugIteration = -1;
#endif
     {
          int i;
          for (i = 0; i < 4; i++) {
               rowArray_[i]->clear();
          }
          for (i = 0; i < 2; i++) {
               columnArray_[i]->clear();
          }
     }
#ifdef CLP_REPORT_PROGRESS
     double * savePSol = new double [numberRows_+numberColumns_];
     double * saveDj = new double [numberRows_+numberColumns_];
     double * saveCost = new double [numberRows_+numberColumns_];
     unsigned char * saveStat = new unsigned char [numberRows_+numberColumns_];
#endif
     // if can't trust much and long way from optimal then relax
     if (largestPrimalError_ > 10.0)
          factorization_->relaxAccuracyCheck(CoinMin(1.0e2, largestPrimalError_ / 10.0));
     else
          factorization_->relaxAccuracyCheck(1.0);
     // status stays at -1 while iterating, >=0 finished, -2 to invert
     // status -3 to go to top without an invert
     int returnCode = -1;
     double saveSumDual = sumDualInfeasibilities_; // so we know to be careful

#if 0
     // compute average infeasibility for backward test
     double averagePrimalInfeasibility = sumPrimalInfeasibilities_ /
                                         ((double ) (numberPrimalInfeasibilities_ + 1));
#endif

     // Get dubious weights
     CoinBigIndex * dubiousWeights = NULL;
#ifdef DUBIOUS_WEIGHTS
     factorization_->getWeights(rowArray_[0]->getIndices());
     dubiousWeights = matrix_->dubiousWeights(this, rowArray_[0]->getIndices());
#endif
     // If values pass then get list of candidates
     int * candidateList = NULL;
     int numberCandidates = 0;
#ifdef CLP_DEBUG
     bool wasInValuesPass = (givenDuals != NULL);
#endif
     int candidate = -1;
     if (givenDuals) {
          assert (ifValuesPass);
          ifValuesPass = 1;
          candidateList = new int[numberRows_];
          // move reduced costs across
          CoinMemcpyN(givenDuals, numberRows_ + numberColumns_, dj_);
          int iRow;
          for (iRow = 0; iRow < numberRows_; iRow++) {
               int iPivot = pivotVariable_[iRow];
               if (flagged(iPivot))
                    continue;
               if (fabs(dj_[iPivot]) > dualTolerance_) {
                    // for now safer to ignore free ones
                    if (lower_[iPivot] > -1.0e50 || upper_[iPivot] < 1.0e50)
                         if (pivoted(iPivot))
                              candidateList[numberCandidates++] = iRow;
               } else {
                    clearPivoted(iPivot);
               }
          }
          // and set first candidate
          if (!numberCandidates) {
               delete [] candidateList;
               delete [] givenDuals;
               givenDuals = NULL;
               candidateList = NULL;
               int iRow;
               for (iRow = 0; iRow < numberRows_; iRow++) {
                    int iPivot = pivotVariable_[iRow];
                    clearPivoted(iPivot);
               }
          }
     } else {
          assert (!ifValuesPass);
     }
#ifdef CHECK_ACCURACY
     {
          if (numberIterations_) {
               int il = -1;
               double largest = 1.0e-1;
               int ilnb = -1;
               double largestnb = 1.0e-8;
               for (int i = 0; i < numberRows_ + numberColumns_; i++) {
                    double diff = fabs(solution_[i] - zzzzzz[i]);
                    if (diff > largest) {
                         largest = diff;
                         il = i;
                    }
                    if (getColumnStatus(i) != basic) {
                         if (diff > largestnb) {
                              largestnb = diff;
                              ilnb = i;
                         }
                    }
               }
               if (il >= 0 && ilnb < 0)
                    printf("largest diff of %g at %d, nonbasic %g at %d\n",
                           largest, il, largestnb, ilnb);
          }
     }
#endif
     while (problemStatus_ == -1) {
          //if (numberIterations_>=101624)
          //resetFakeBounds(-1);
#ifdef CLP_DEBUG
          if (givenDuals) {
               double value5 = 0.0;
               int i;
               for (i = 0; i < numberRows_ + numberColumns_; i++) {
                    if (dj_[i] < -1.0e-6)
                         if (upper_[i] < 1.0e20)
                              value5 += dj_[i] * upper_[i];
                         else
                              printf("bad dj %g on %d with large upper status %d\n",
                                     dj_[i], i, status_[i] & 7);
                    else if (dj_[i] > 1.0e-6)
                         if (lower_[i] > -1.0e20)
                              value5 += dj_[i] * lower_[i];
                         else
                              printf("bad dj %g on %d with large lower status %d\n",
                                     dj_[i], i, status_[i] & 7);
               }
               printf("Values objective Value %g\n", value5);
          }
          if ((handler_->logLevel() & 32) && wasInValuesPass) {
               double value5 = 0.0;
               int i;
               for (i = 0; i < numberRows_ + numberColumns_; i++) {
                    if (dj_[i] < -1.0e-6)
                         if (upper_[i] < 1.0e20)
                              value5 += dj_[i] * upper_[i];
                         else if (dj_[i] > 1.0e-6)
                              if (lower_[i] > -1.0e20)
                                   value5 += dj_[i] * lower_[i];
               }
               printf("Values objective Value %g\n", value5);
               {
                    int i;
                    for (i = 0; i < numberRows_ + numberColumns_; i++) {
                         int iSequence = i;
                         double oldValue;

                         switch(getStatus(iSequence)) {

                         case basic:
                         case ClpSimplex::isFixed:
                              break;
                         case isFree:
                         case superBasic:
                              abort();
                              break;
                         case atUpperBound:
                              oldValue = dj_[iSequence];
                              //assert (oldValue<=tolerance);
                              assert (fabs(solution_[iSequence] - upper_[iSequence]) < 1.0e-7);
                              break;
                         case atLowerBound:
                              oldValue = dj_[iSequence];
                              //assert (oldValue>=-tolerance);
                              assert (fabs(solution_[iSequence] - lower_[iSequence]) < 1.0e-7);
                              break;
                         }
                    }
               }
          }
#endif
#ifdef CLP_DEBUG
          {
               int i;
               for (i = 0; i < 4; i++) {
                    rowArray_[i]->checkClear();
               }
               for (i = 0; i < 2; i++) {
                    columnArray_[i]->checkClear();
               }
          }
#endif
#if CLP_DEBUG>2
          // very expensive
          if (numberIterations_ > 3063 && numberIterations_ < 30700) {
               //handler_->setLogLevel(63);
               double saveValue = objectiveValue_;
               double * saveRow1 = new double[numberRows_];
               double * saveRow2 = new double[numberRows_];
               CoinMemcpyN(rowReducedCost_, numberRows_, saveRow1);
               CoinMemcpyN(rowActivityWork_, numberRows_, saveRow2);
               double * saveColumn1 = new double[numberColumns_];
               double * saveColumn2 = new double[numberColumns_];
               CoinMemcpyN(reducedCostWork_, numberColumns_, saveColumn1);
               CoinMemcpyN(columnActivityWork_, numberColumns_, saveColumn2);
               gutsOfSolution(NULL, NULL);
               printf("xxx %d old obj %g, recomputed %g, sum dual inf %g\n",
                      numberIterations_,
                      saveValue, objectiveValue_, sumDualInfeasibilities_);
               if (saveValue > objectiveValue_ + 1.0e-2)
                    printf("**bad**\n");
               CoinMemcpyN(saveRow1, numberRows_, rowReducedCost_);
               CoinMemcpyN(saveRow2, numberRows_, rowActivityWork_);
               CoinMemcpyN(saveColumn1, numberColumns_, reducedCostWork_);
               CoinMemcpyN(saveColumn2, numberColumns_, columnActivityWork_);
               delete [] saveRow1;
               delete [] saveRow2;
               delete [] saveColumn1;
               delete [] saveColumn2;
               objectiveValue_ = saveValue;
          }
#endif
#if 0
          //    if (factorization_->pivots()){
          {
               int iPivot;
               double * array = rowArray_[3]->denseVector();
               int i;
               for (iPivot = 0; iPivot < numberRows_; iPivot++) {
                    int iSequence = pivotVariable_[iPivot];
                    unpack(rowArray_[3], iSequence);
                    factorization_->updateColumn(rowArray_[2], rowArray_[3]);
                    assert (fabs(array[iPivot] - 1.0) < 1.0e-4);
                    array[iPivot] = 0.0;
                    for (i = 0; i < numberRows_; i++)
                         assert (fabs(array[i]) < 1.0e-4);
                    rowArray_[3]->clear();
               }
          }
#endif
#ifdef CLP_DEBUG
          {
               int iSequence, number = numberRows_ + numberColumns_;
               for (iSequence = 0; iSequence < number; iSequence++) {
                    double lowerValue = lower_[iSequence];
                    double upperValue = upper_[iSequence];
                    double value = solution_[iSequence];
                    if(getStatus(iSequence) != basic && getStatus(iSequence) != isFree) {
                         assert(lowerValue > -1.0e20);
                         assert(upperValue < 1.0e20);
                    }
                    switch(getStatus(iSequence)) {

                    case basic:
                         break;
                    case isFree:
                    case superBasic:
                         break;
                    case atUpperBound:
                         assert (fabs(value - upperValue) <= primalTolerance_) ;
                         break;
                    case atLowerBound:
                    case ClpSimplex::isFixed:
                         assert (fabs(value - lowerValue) <= primalTolerance_) ;
                         break;
                    }
               }
          }
          if(numberIterations_ == debugIteration) {
               printf("dodgy iteration coming up\n");
          }
#endif
#if 0
          printf("checking nz\n");
          for (int i = 0; i < 3; i++) {
               if (!rowArray_[i]->getNumElements())
                    rowArray_[i]->checkClear();
          }
#endif
          // choose row to go out
          // dualRow will go to virtual row pivot choice algorithm
          // make sure values pass off if it should be
          if (numberCandidates)
               candidate = candidateList[--numberCandidates];
          else
               candidate = -1;
          dualRow(candidate);
          if (pivotRow_ >= 0) {
               // we found a pivot row
               if (handler_->detail(CLP_SIMPLEX_PIVOTROW, messages_) < 100) {
                    handler_->message(CLP_SIMPLEX_PIVOTROW, messages_)
                              << pivotRow_
                              << CoinMessageEol;
               }
               // check accuracy of weights
               dualRowPivot_->checkAccuracy();
               // Get good size for pivot
               // Allow first few iterations to take tiny
               double acceptablePivot = 1.0e-1 * acceptablePivot_;
               if (numberIterations_ > 100)
                    acceptablePivot = acceptablePivot_;
               if (factorization_->pivots() > 10 ||
                         (factorization_->pivots() && saveSumDual))
                    acceptablePivot = 1.0e+3 * acceptablePivot_; // if we have iterated be more strict
               else if (factorization_->pivots() > 5)
                    acceptablePivot = 1.0e+2 * acceptablePivot_; // if we have iterated be slightly more strict
               else if (factorization_->pivots())
                    acceptablePivot = acceptablePivot_; // relax
               // But factorizations complain if <1.0e-8
               //acceptablePivot=CoinMax(acceptablePivot,1.0e-8);
               double bestPossiblePivot = 1.0;
               // get sign for finding row of tableau
               if (candidate < 0) {
                    // normal iteration
                    // create as packed
                    double direction = directionOut_;
                    rowArray_[0]->createPacked(1, &pivotRow_, &direction);
                    factorization_->updateColumnTranspose(rowArray_[1], rowArray_[0]);
                    // Allow to do dualColumn0
                    if (numberThreads_ < -1)
                         spareIntArray_[0] = 1;
                    spareDoubleArray_[0] = acceptablePivot;
                    rowArray_[3]->clear();
                    sequenceIn_ = -1;
                    // put row of tableau in rowArray[0] and columnArray[0]
                    assert (!rowArray_[1]->getNumElements());
                    if (!scaledMatrix_) {
                         if ((moreSpecialOptions_ & 8) != 0 && !rowScale_)
                              spareIntArray_[0] = 1;
                         matrix_->transposeTimes(this, -1.0,
                                                 rowArray_[0], rowArray_[1], columnArray_[0]);
                    } else {
                         double * saveR = rowScale_;
                         double * saveC = columnScale_;
                         rowScale_ = NULL;
                         columnScale_ = NULL;
                         if ((moreSpecialOptions_ & 8) != 0)
                              spareIntArray_[0] = 1;
                         scaledMatrix_->transposeTimes(this, -1.0,
                                                       rowArray_[0], rowArray_[1], columnArray_[0]);
                         rowScale_ = saveR;
                         columnScale_ = saveC;
                    }
#ifdef CLP_REPORT_PROGRESS
                    memcpy(savePSol, solution_, (numberColumns_ + numberRows_)*sizeof(double));
                    memcpy(saveDj, dj_, (numberColumns_ + numberRows_)*sizeof(double));
                    memcpy(saveCost, cost_, (numberColumns_ + numberRows_)*sizeof(double));
                    memcpy(saveStat, status_, (numberColumns_ + numberRows_)*sizeof(char));
#endif
                    // do ratio test for normal iteration
                    bestPossiblePivot = dualColumn(rowArray_[0], columnArray_[0], rowArray_[3],
                                                   columnArray_[1], acceptablePivot, dubiousWeights);
#if CAN_HAVE_ZERO_OBJ>1
		    if ((specialOptions_&2097152)!=0) 
		      theta_=0.0;
#endif
               } else {
                    // Make sure direction plausible
                    CoinAssert (upperOut_ < 1.0e50 || lowerOut_ > -1.0e50);
                    // If in integer cleanup do direction using duals
                    // may be wrong way round
                    if(ifValuesPass == 2) {
                         if (dual_[pivotRow_] > 0.0) {
                              // this will give a -1 in pivot row (as slacks are -1.0)
                              directionOut_ = 1;
                         } else {
                              directionOut_ = -1;
                         }
                    }
                    if (directionOut_ < 0 && fabs(valueOut_ - upperOut_) > dualBound_ + primalTolerance_) {
                         if (fabs(valueOut_ - upperOut_) > fabs(valueOut_ - lowerOut_))
                              directionOut_ = 1;
                    } else if (directionOut_ > 0 && fabs(valueOut_ - lowerOut_) > dualBound_ + primalTolerance_) {
                         if (fabs(valueOut_ - upperOut_) < fabs(valueOut_ - lowerOut_))
                              directionOut_ = -1;
                    }
                    double direction = directionOut_;
                    rowArray_[0]->createPacked(1, &pivotRow_, &direction);
                    factorization_->updateColumnTranspose(rowArray_[1], rowArray_[0]);
                    // put row of tableau in rowArray[0] and columnArray[0]
                    if (!scaledMatrix_) {
                         matrix_->transposeTimes(this, -1.0,
                                                 rowArray_[0], rowArray_[3], columnArray_[0]);
                    } else {
                         double * saveR = rowScale_;
                         double * saveC = columnScale_;
                         rowScale_ = NULL;
                         columnScale_ = NULL;
                         scaledMatrix_->transposeTimes(this, -1.0,
                                                       rowArray_[0], rowArray_[3], columnArray_[0]);
                         rowScale_ = saveR;
                         columnScale_ = saveC;
                    }
                    acceptablePivot *= 10.0;
                    // do ratio test
                    if (ifValuesPass == 1) {
                         checkPossibleValuesMove(rowArray_[0], columnArray_[0],
                                                 acceptablePivot);
                    } else {
                         checkPossibleCleanup(rowArray_[0], columnArray_[0],
                                              acceptablePivot);
                         if (sequenceIn_ < 0) {
                              rowArray_[0]->clear();
                              columnArray_[0]->clear();
                              continue; // can't do anything
                         }
                    }

                    // recompute true dualOut_
                    if (directionOut_ < 0) {
                         dualOut_ = valueOut_ - upperOut_;
                    } else {
                         dualOut_ = lowerOut_ - valueOut_;
                    }
                    // check what happened if was values pass
                    // may want to move part way i.e. movement
                    bool normalIteration = (sequenceIn_ != sequenceOut_);

                    clearPivoted(sequenceOut_);  // make sure won't be done again
                    // see if end of values pass
                    if (!numberCandidates) {
                         int iRow;
                         delete [] candidateList;
                         delete [] givenDuals;
                         candidate = -2; // -2 signals end
                         givenDuals = NULL;
                         candidateList = NULL;
                         ifValuesPass = 1;
                         for (iRow = 0; iRow < numberRows_; iRow++) {
                              int iPivot = pivotVariable_[iRow];
                              //assert (fabs(dj_[iPivot]),1.0e-5);
                              clearPivoted(iPivot);
                         }
                    }
                    if (!normalIteration) {
                         //rowArray_[0]->cleanAndPackSafe(1.0e-60);
                         //columnArray_[0]->cleanAndPackSafe(1.0e-60);
                         updateDualsInValuesPass(rowArray_[0], columnArray_[0], theta_);
                         if (candidate == -2)
                              problemStatus_ = -2;
                         continue; // skip rest of iteration
                    } else {
                         // recompute dualOut_
                         if (directionOut_ < 0) {
                              dualOut_ = valueOut_ - upperOut_;
                         } else {
                              dualOut_ = lowerOut_ - valueOut_;
                         }
                    }
               }
               if (sequenceIn_ >= 0) {
                    // normal iteration
                    // update the incoming column
                    double btranAlpha = -alpha_ * directionOut_; // for check
                    unpackPacked(rowArray_[1]);
                    // moved into updateWeights - factorization_->updateColumnFT(rowArray_[2],rowArray_[1]);
                    // and update dual weights (can do in parallel - with extra array)
                    alpha_ = dualRowPivot_->updateWeights(rowArray_[0],
                                                          rowArray_[2],
                                                          rowArray_[3],
                                                          rowArray_[1]);
                    // see if update stable
#ifdef CLP_DEBUG
                    if ((handler_->logLevel() & 32))
                         printf("btran alpha %g, ftran alpha %g\n", btranAlpha, alpha_);
#endif
                    double checkValue = 1.0e-7;
                    // if can't trust much and long way from optimal then relax
                    if (largestPrimalError_ > 10.0)
                         checkValue = CoinMin(1.0e-4, 1.0e-8 * largestPrimalError_);
                    if (fabs(btranAlpha) < 1.0e-12 || fabs(alpha_) < 1.0e-12 ||
                              fabs(btranAlpha - alpha_) > checkValue*(1.0 + fabs(alpha_))) {
                         handler_->message(CLP_DUAL_CHECK, messages_)
                                   << btranAlpha
                                   << alpha_
                                   << CoinMessageEol;
                         if (factorization_->pivots()) {
                              dualRowPivot_->unrollWeights();
                              problemStatus_ = -2; // factorize now
                              rowArray_[0]->clear();
                              rowArray_[1]->clear();
                              columnArray_[0]->clear();
                              returnCode = -2;
                              break;
                         } else {
                              // take on more relaxed criterion
                              double test;
                              if (fabs(btranAlpha) < 1.0e-8 || fabs(alpha_) < 1.0e-8)
                                   test = 1.0e-1 * fabs(alpha_);
                              else
                                   test = 1.0e-4 * (1.0 + fabs(alpha_));
                              if (fabs(btranAlpha) < 1.0e-12 || fabs(alpha_) < 1.0e-12 ||
                                        fabs(btranAlpha - alpha_) > test) {
                                   dualRowPivot_->unrollWeights();
                                   // need to reject something
                                   char x = isColumn(sequenceOut_) ? 'C' : 'R';
                                   handler_->message(CLP_SIMPLEX_FLAG, messages_)
                                             << x << sequenceWithin(sequenceOut_)
                                             << CoinMessageEol;
#ifdef COIN_DEVELOP
                                   printf("flag a %g %g\n", btranAlpha, alpha_);
#endif
				   //#define FEB_TRY
#if 1 //def FEB_TRY
                                   // Make safer?
                                   factorization_->saferTolerances (-0.99, -1.03);
#endif
                                   setFlagged(sequenceOut_);
                                   progress_.clearBadTimes();
                                   lastBadIteration_ = numberIterations_; // say be more cautious
                                   rowArray_[0]->clear();
                                   rowArray_[1]->clear();
                                   columnArray_[0]->clear();
                                   if (fabs(alpha_) < 1.0e-10 && fabs(btranAlpha) < 1.0e-8 && numberIterations_ > 100) {
                                        //printf("I think should declare infeasible\n");
                                        problemStatus_ = 1;
                                        returnCode = 1;
                                        break;
                                   }
                                   continue;
                              }
                         }
                    }
                    // update duals BEFORE replaceColumn so can do updateColumn
                    double objectiveChange = 0.0;
                    // do duals first as variables may flip bounds
                    // rowArray_[0] and columnArray_[0] may have flips
                    // so use rowArray_[3] for work array from here on
                    int nswapped = 0;
                    //rowArray_[0]->cleanAndPackSafe(1.0e-60);
                    //columnArray_[0]->cleanAndPackSafe(1.0e-60);
                    if (candidate == -1) {
#if CLP_CAN_HAVE_ZERO_OBJ>1
		    if ((specialOptions_&2097152)==0) {
#endif
                         // make sure incoming doesn't count
                         Status saveStatus = getStatus(sequenceIn_);
                         setStatus(sequenceIn_, basic);
                         nswapped = updateDualsInDual(rowArray_[0], columnArray_[0],
                                                      rowArray_[2], theta_,
                                                      objectiveChange, false);
                         setStatus(sequenceIn_, saveStatus);
#if CLP_CAN_HAVE_ZERO_OBJ>1
		    } else {
		      rowArray_[0]->clear();
		      rowArray_[2]->clear();
		      columnArray_[0]->clear();
		    }
#endif
                    } else {
                         updateDualsInValuesPass(rowArray_[0], columnArray_[0], theta_);
                    }
                    double oldDualOut = dualOut_;
                    // which will change basic solution
                    if (nswapped) {
                         if (rowArray_[2]->getNumElements()) {
                              factorization_->updateColumn(rowArray_[3], rowArray_[2]);
                              dualRowPivot_->updatePrimalSolution(rowArray_[2],
                                                                  1.0, objectiveChange);
                         }
                         // recompute dualOut_
                         valueOut_ = solution_[sequenceOut_];
                         if (directionOut_ < 0) {
                              dualOut_ = valueOut_ - upperOut_;
                         } else {
                              dualOut_ = lowerOut_ - valueOut_;
                         }
#if 0
                         if (dualOut_ < 0.0) {
#ifdef CLP_DEBUG
                              if (handler_->logLevel() & 32) {
                                   printf(" dualOut_ %g %g save %g\n", dualOut_, averagePrimalInfeasibility, saveDualOut);
                                   printf("values %g %g %g %g %g %g %g\n", lowerOut_, valueOut_, upperOut_,
                                          objectiveChange,);
                              }
#endif
                              if (upperOut_ == lowerOut_)
                                   dualOut_ = 0.0;
                         }
                         if(dualOut_ < -CoinMax(1.0e-12 * averagePrimalInfeasibility, 1.0e-8)
                                   && factorization_->pivots() > 100 &&
                                   getStatus(sequenceIn_) != isFree) {
                              // going backwards - factorize
                              dualRowPivot_->unrollWeights();
                              problemStatus_ = -2; // factorize now
                              returnCode = -2;
                              break;
                         }
#endif
                    }
                    // amount primal will move
                    double movement = -dualOut_ * directionOut_ / alpha_;
                    double movementOld = oldDualOut * directionOut_ / alpha_;
                    // so objective should increase by fabs(dj)*movement
                    // but we already have objective change - so check will be good
                    if (objectiveChange + fabs(movementOld * dualIn_) < -CoinMax(1.0e-5, 1.0e-12 * fabs(objectiveValue_))) {
#ifdef CLP_DEBUG
                         if (handler_->logLevel() & 32)
                              printf("movement %g, swap change %g, rest %g  * %g\n",
                                     objectiveChange + fabs(movement * dualIn_),
                                     objectiveChange, movement, dualIn_);
#endif
                         if(factorization_->pivots()) {
                              // going backwards - factorize
                              dualRowPivot_->unrollWeights();
                              problemStatus_ = -2; // factorize now
                              returnCode = -2;
                              break;
                         }
                    }
                    // if stable replace in basis
                    int updateStatus = factorization_->replaceColumn(this,
                                       rowArray_[2],
                                       rowArray_[1],
                                       pivotRow_,
                                       alpha_,
                                       (moreSpecialOptions_ & 16) != 0,
                                       acceptablePivot);
                    // If looks like bad pivot - refactorize
                    if (fabs(dualOut_) > 1.0e50)
                         updateStatus = 2;
                    // if no pivots, bad update but reasonable alpha - take and invert
                    if (updateStatus == 2 &&
                              !factorization_->pivots() && fabs(alpha_) > 1.0e-5)
                         updateStatus = 4;
                    if (updateStatus == 1 || updateStatus == 4) {
                         // slight error
                         if (factorization_->pivots() > 5 || updateStatus == 4) {
                              problemStatus_ = -2; // factorize now
                              returnCode = -3;
                         }
                    } else if (updateStatus == 2) {
                         // major error
                         dualRowPivot_->unrollWeights();
                         // later we may need to unwind more e.g. fake bounds
                         if (factorization_->pivots() &&
                                   ((moreSpecialOptions_ & 16) == 0 || factorization_->pivots() > 4)) {
                              problemStatus_ = -2; // factorize now
                              returnCode = -2;
                              moreSpecialOptions_ |= 16;
                              break;
                         } else {
                              // need to reject something
                              char x = isColumn(sequenceOut_) ? 'C' : 'R';
                              handler_->message(CLP_SIMPLEX_FLAG, messages_)
                                        << x << sequenceWithin(sequenceOut_)
                                        << CoinMessageEol;
#ifdef COIN_DEVELOP
                              printf("flag b %g\n", alpha_);
#endif
                              setFlagged(sequenceOut_);
                              progress_.clearBadTimes();
                              lastBadIteration_ = numberIterations_; // say be more cautious
                              rowArray_[0]->clear();
                              rowArray_[1]->clear();
                              columnArray_[0]->clear();
                              // make sure dual feasible
                              // look at all rows and columns
                              double objectiveChange = 0.0;
                              updateDualsInDual(rowArray_[0], columnArray_[0], rowArray_[1],
                                                0.0, objectiveChange, true);
                              rowArray_[1]->clear();
                              columnArray_[0]->clear();
                              continue;
                         }
                    } else if (updateStatus == 3) {
                         // out of memory
                         // increase space if not many iterations
                         if (factorization_->pivots() <
                                   0.5 * factorization_->maximumPivots() &&
                                   factorization_->pivots() < 200)
                              factorization_->areaFactor(
                                   factorization_->areaFactor() * 1.1);
                         problemStatus_ = -2; // factorize now
                    } else if (updateStatus == 5) {
                         problemStatus_ = -2; // factorize now
                    }
                    // update primal solution
                    if (theta_ < 0.0 && candidate == -1) {
#ifdef CLP_DEBUG
                         if (handler_->logLevel() & 32)
                              printf("negative theta %g\n", theta_);
#endif
                         theta_ = 0.0;
                    }
                    // do actual flips
                    flipBounds(rowArray_[0], columnArray_[0]);
                    //rowArray_[1]->expand();
                    dualRowPivot_->updatePrimalSolution(rowArray_[1],
                                                        movement,
                                                        objectiveChange);
#ifdef CLP_DEBUG
                    double oldobj = objectiveValue_;
#endif
                    // modify dualout
                    dualOut_ /= alpha_;
                    dualOut_ *= -directionOut_;
                    //setStatus(sequenceIn_,basic);
                    dj_[sequenceIn_] = 0.0;
                    double oldValue = valueIn_;
                    if (directionIn_ == -1) {
                         // as if from upper bound
                         valueIn_ = upperIn_ + dualOut_;
                    } else {
                         // as if from lower bound
                         valueIn_ = lowerIn_ + dualOut_;
                    }
                    objectiveChange += cost_[sequenceIn_] * (valueIn_ - oldValue);
                    // outgoing
                    // set dj to zero unless values pass
                    if (directionOut_ > 0) {
                         valueOut_ = lowerOut_;
                         if (candidate == -1)
                              dj_[sequenceOut_] = theta_;
                    } else {
                         valueOut_ = upperOut_;
                         if (candidate == -1)
                              dj_[sequenceOut_] = -theta_;
                    }
                    solution_[sequenceOut_] = valueOut_;
                    int whatNext = housekeeping(objectiveChange);
#if 0
		    for (int i=0;i<numberRows_+numberColumns_;i++) {
		      if (getStatus(i)==atLowerBound) {
			assert (dj_[i]>-1.0e-5);
			assert (solution_[i]<=lower_[i]+1.0e-5);
		      } else if (getStatus(i)==atUpperBound) {
			assert (dj_[i]<1.0e-5);
			assert (solution_[i]>=upper_[i]-1.0e-5);
		      }
		    }
#endif
#ifdef CLP_REPORT_PROGRESS
                    if (ixxxxxx > ixxyyyy - 5) {
                         handler_->setLogLevel(63);
                         int nTotal = numberColumns_ + numberRows_;
                         double oldObj = 0.0;
                         double newObj = 0.0;
                         for (int i = 0; i < nTotal; i++) {
                              if (savePSol[i])
                                   oldObj += savePSol[i] * saveCost[i];
                              if (solution_[i])
                                   newObj += solution_[i] * cost_[i];
                              bool printIt = false;
                              if (cost_[i] != saveCost[i])
                                   printIt = true;
                              if (status_[i] != saveStat[i])
                                   printIt = true;
                              if (printIt)
                                   printf("%d old %d cost %g sol %g, new %d cost %g sol %g\n",
                                          i, saveStat[i], saveCost[i], savePSol[i],
                                          status_[i], cost_[i], solution_[i]);
                              // difference
                              savePSol[i] = solution_[i] - savePSol[i];
                         }
                         printf("pivots %d, old obj %g new %g\n",
                                factorization_->pivots(),
                                oldObj, newObj);
                         memset(saveDj, 0, numberRows_ * sizeof(double));
                         times(1.0, savePSol, saveDj);
                         double largest = 1.0e-6;
                         int k = -1;
                         for (int i = 0; i < numberRows_; i++) {
                              saveDj[i] -= savePSol[i+numberColumns_];
                              if (fabs(saveDj[i]) > largest) {
                                   largest = fabs(saveDj[i]);
                                   k = i;
                              }
                         }
                         if (k >= 0)
                              printf("Not null %d %g\n", k, largest);
                    }
#endif
#ifdef VUB
                    {
                         if ((sequenceIn_ < numberColumns_ && vub[sequenceIn_] >= 0) || toVub[sequenceIn_] >= 0 ||
                                   (sequenceOut_ < numberColumns_ && vub[sequenceOut_] >= 0) || toVub[sequenceOut_] >= 0) {
                              int inSequence = sequenceIn_;
                              int inVub = -1;
                              if (sequenceIn_ < numberColumns_)
                                   inVub = vub[sequenceIn_];
                              int inBack = toVub[inSequence];
                              int inSlack = -1;
                              if (inSequence >= numberColumns_ && inBack >= 0) {
                                   inSlack = inSequence - numberColumns_;
                                   inSequence = inBack;
                                   inBack = toVub[inSequence];
                              }
                              if (inVub >= 0)
                                   printf("Vub %d in ", inSequence);
                              if (inBack >= 0 && inSlack < 0)
                                   printf("%d (descendent of %d) in ", inSequence, inBack);
                              if (inSlack >= 0)
                                   printf("slack for row %d -> %d (descendent of %d) in ", inSlack, inSequence, inBack);
                              int outSequence = sequenceOut_;
                              int outVub = -1;
                              if (sequenceOut_ < numberColumns_)
                                   outVub = vub[sequenceOut_];
                              int outBack = toVub[outSequence];
                              int outSlack = -1;
                              if (outSequence >= numberColumns_ && outBack >= 0) {
                                   outSlack = outSequence - numberColumns_;
                                   outSequence = outBack;
                                   outBack = toVub[outSequence];
                              }
                              if (outVub >= 0)
                                   printf("Vub %d out ", outSequence);
                              if (outBack >= 0 && outSlack < 0)
                                   printf("%d (descendent of %d) out ", outSequence, outBack);
                              if (outSlack >= 0)
                                   printf("slack for row %d -> %d (descendent of %d) out ", outSlack, outSequence, outBack);
                              printf("\n");
                         }
                    }
#endif
#if 0
                    if (numberIterations_ > 206033)
                         handler_->setLogLevel(63);
                    if (numberIterations_ > 210567)
                         exit(77);
#endif
                    if (!givenDuals && ifValuesPass && ifValuesPass != 2) {
                         handler_->message(CLP_END_VALUES_PASS, messages_)
                                   << numberIterations_;
                         whatNext = 1;
                    }
#ifdef CHECK_ACCURACY
                    if (whatNext) {
                         CoinMemcpyN(solution_, (numberRows_ + numberColumns_), zzzzzz);
                    }
#endif
                    //if (numberIterations_==1890)
                    //whatNext=1;
                    //if (numberIterations_>2000)
                    //exit(77);
                    // and set bounds correctly
                    originalBound(sequenceIn_);
                    changeBound(sequenceOut_);
#ifdef CLP_DEBUG
                    if (objectiveValue_ < oldobj - 1.0e-5 && (handler_->logLevel() & 16))
                         printf("obj backwards %g %g\n", objectiveValue_, oldobj);
#endif
#if 0
                    {
                         for (int i = 0; i < numberRows_ + numberColumns_; i++) {
                              FakeBound bound = getFakeBound(i);
                              if (bound == ClpSimplexDual::upperFake) {
                                   assert (upper_[i] < 1.0e20);
                              } else if (bound == ClpSimplexDual::lowerFake) {
                                   assert (lower_[i] > -1.0e20);
                              } else if (bound == ClpSimplexDual::bothFake) {
                                   assert (upper_[i] < 1.0e20);
                                   assert (lower_[i] > -1.0e20);
                              }
                         }
                    }
#endif
                    if (whatNext == 1 || candidate == -2) {
                         problemStatus_ = -2; // refactorize
                    } else if (whatNext == 2) {
                         // maximum iterations or equivalent
                         problemStatus_ = 3;
                         returnCode = 3;
                         break;
                    }
                    // Check event
                    {
                         int status = eventHandler_->event(ClpEventHandler::endOfIteration);
                         if (status >= 0) {
                              problemStatus_ = 5;
                              secondaryStatus_ = ClpEventHandler::endOfIteration;
                              returnCode = 4;
                              break;
                         }
                    }
               } else {
#ifdef CLP_INVESTIGATE_SERIAL
                    z_thinks = 1;
#endif
                    // no incoming column is valid
		    spareIntArray_[3]=pivotRow_;
                    pivotRow_ = -1;
#ifdef CLP_DEBUG
                    if (handler_->logLevel() & 32)
                         printf("** no column pivot\n");
#endif
                    if (factorization_->pivots() < 2 && acceptablePivot_ <= 1.0e-8) {
                         //&&goodAccuracy()) {
                         // If not in branch and bound etc save ray
                         delete [] ray_;
                         if ((specialOptions_&(1024 | 4096)) == 0 || (specialOptions_ & 32) != 0) {
                              // create ray anyway
#ifdef PRINT_RAY_METHOD
			   printf("Dual creating infeasibility ray direction out %d - pivRow %d seqOut %d lower %g,val %g,upper %g\n",
				  directionOut_,spareIntArray_[3],sequenceOut_,lowerOut_,valueOut_,upperOut_);
#endif
                              ray_ = new double [ numberRows_];
                              rowArray_[0]->expand(); // in case packed
			      const double * array = rowArray_[0]->denseVector();
			      for (int i=0;i<numberRows_;i++) 
				ray_[i] = array[i];
                         } else {
                              ray_ = NULL;
                         }
                         // If we have just factorized and infeasibility reasonable say infeas
                         double dualTest = ((specialOptions_ & 4096) != 0) ? 1.0e8 : 1.0e13;
                         if (((specialOptions_ & 4096) != 0 || bestPossiblePivot < 1.0e-11) && dualBound_ > dualTest) {
                              double testValue = 1.0e-4;
                              if (!factorization_->pivots() && numberPrimalInfeasibilities_ == 1)
                                   testValue = 1.0e-6;
                              if (valueOut_ > upperOut_ + testValue || valueOut_ < lowerOut_ - testValue
                                        || (specialOptions_ & 64) == 0) {
                                   // say infeasible
                                   problemStatus_ = 1;
                                   // unless primal feasible!!!!
                                   //printf("%d %g %d %g\n",numberPrimalInfeasibilities_,sumPrimalInfeasibilities_,
                                   //   numberDualInfeasibilities_,sumDualInfeasibilities_);
                                   //#define TEST_CLP_NODE
#ifndef TEST_CLP_NODE
                                   // Should be correct - but ...
                                   int numberFake = numberAtFakeBound();
                                   double sumPrimal =  (!numberFake) ? 2.0e5 : sumPrimalInfeasibilities_;
                                   if (sumPrimalInfeasibilities_ < 1.0e-3 || sumDualInfeasibilities_ > 1.0e-5 ||
                                             (sumPrimal < 1.0e5 && (specialOptions_ & 1024) != 0 && factorization_->pivots())) {
                                        if (sumPrimal > 50.0 && factorization_->pivots() > 2) {
                                             problemStatus_ = -4;
#ifdef COIN_DEVELOP
                                             printf("status to -4 at %d - primalinf %g pivots %d\n",
                                                    __LINE__, sumPrimalInfeasibilities_,
                                                    factorization_->pivots());
#endif
                                        } else {
                                             problemStatus_ = 10;
#if COIN_DEVELOP>1
                                             printf("returning at %d - primal %d %g - dual %d %g fake %d weight %g - pivs %d - options (1024-16384) %d %d %d %d %d\n",
                                                    __LINE__, numberPrimalInfeasibilities_,
                                                    sumPrimalInfeasibilities_,
                                                    numberDualInfeasibilities_, sumDualInfeasibilities_,
                                                    numberFake_, dualBound_, factorization_->pivots(),
                                                    (specialOptions_ & 1024) != 0 ? 1 : 0,
                                                    (specialOptions_ & 2048) != 0 ? 1 : 0,
                                                    (specialOptions_ & 4096) != 0 ? 1 : 0,
                                                    (specialOptions_ & 8192) != 0 ? 1 : 0,
                                                    (specialOptions_ & 16384) != 0 ? 1 : 0
                                                   );
#endif
                                             // Get rid of objective
                                             if ((specialOptions_ & 16384) == 0)
                                                  objective_ = new ClpLinearObjective(NULL, numberColumns_);
                                        }
                                   }
#else
                                   if (sumPrimalInfeasibilities_ < 1.0e-3 || sumDualInfeasibilities_ > 1.0e-6) {
#ifdef COIN_DEVELOP
                                        printf("at %d - primal %d %g - dual %d %g fake %d weight %g - pivs %d\n",
                                               __LINE__, numberPrimalInfeasibilities_,
                                               sumPrimalInfeasibilities_,
                                               numberDualInfeasibilities_, sumDualInfeasibilities_,
                                               numberFake_, dualBound_, factorization_->pivots());
#endif
                                        if ((specialOptions_ & 1024) != 0 && factorization_->pivots()) {
                                             problemStatus_ = 10;
#if COIN_DEVELOP>1
                                             printf("returning at %d\n", __LINE__);
#endif
                                             // Get rid of objective
                                             if ((specialOptions_ & 16384) == 0)
                                                  objective_ = new ClpLinearObjective(NULL, numberColumns_);
                                        }
                                   }
#endif
                                   rowArray_[0]->clear();
                                   columnArray_[0]->clear();
                                   returnCode = 1;
                                   break;
                              }
                         }
                         // If special option set - put off as long as possible
                         if ((specialOptions_ & 64) == 0 || (moreSpecialOptions_ & 64) != 0) {
                              if (factorization_->pivots() == 0)
                                   problemStatus_ = -4; //say looks infeasible
                         } else {
                              // flag
                              char x = isColumn(sequenceOut_) ? 'C' : 'R';
                              handler_->message(CLP_SIMPLEX_FLAG, messages_)
                                        << x << sequenceWithin(sequenceOut_)
                                        << CoinMessageEol;
#ifdef COIN_DEVELOP
                              printf("flag c\n");
#endif
                              setFlagged(sequenceOut_);
                              if (!factorization_->pivots()) {
                                   rowArray_[0]->clear();
                                   columnArray_[0]->clear();
                                   continue;
                              }
                         }
                    }
                    if (factorization_->pivots() < 5 && acceptablePivot_ > 1.0e-8)
                         acceptablePivot_ = 1.0e-8;
                    rowArray_[0]->clear();
                    columnArray_[0]->clear();
                    returnCode = 1;
                    break;
               }
          } else {
#ifdef CLP_INVESTIGATE_SERIAL
               z_thinks = 0;
#endif
               // no pivot row
#ifdef CLP_DEBUG
               if (handler_->logLevel() & 32)
                    printf("** no row pivot\n");
#endif
               // If in branch and bound try and get rid of fixed variables
               if ((specialOptions_ & 1024) != 0 && CLEAN_FIXED) {
                    assert (!candidateList);
                    candidateList = new int[numberRows_];
                    int iRow;
                    for (iRow = 0; iRow < numberRows_; iRow++) {
                         int iPivot = pivotVariable_[iRow];
                         if (flagged(iPivot) || !pivoted(iPivot))
                              continue;
                         assert (iPivot < numberColumns_ && lower_[iPivot] == upper_[iPivot]);
                         candidateList[numberCandidates++] = iRow;
                    }
                    // and set first candidate
                    if (!numberCandidates) {
                         delete [] candidateList;
                         candidateList = NULL;
                         int iRow;
                         for (iRow = 0; iRow < numberRows_; iRow++) {
                              int iPivot = pivotVariable_[iRow];
                              clearPivoted(iPivot);
                         }
                    } else {
                         ifValuesPass = 2;
                         continue;
                    }
               }
               int numberPivots = factorization_->pivots();
               bool specialCase;
               int useNumberFake;
               returnCode = 0;
               if (numberPivots <= CoinMax(dontFactorizePivots_, 20) &&
                         (specialOptions_ & 2048) != 0 && (true || !numberChanged_ || perturbation_ == 101)
                         && dualBound_ >= 1.0e8) {
                    specialCase = true;
                    // as dual bound high - should be okay
                    useNumberFake = 0;
               } else {
                    specialCase = false;
                    useNumberFake = numberFake_;
               }
               if (!numberPivots || specialCase) {
                    // may have crept through - so may be optimal
                    // check any flagged variables
                    int iRow;
                    for (iRow = 0; iRow < numberRows_; iRow++) {
                         int iPivot = pivotVariable_[iRow];
                         if (flagged(iPivot))
                              break;
                    }
                    if (iRow < numberRows_ && numberPivots) {
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
                         if (iRow < numberRows_) {
#ifdef COIN_DEVELOP
                              std::cout << "Flagged variables at end - infeasible?" << std::endl;
                              printf("Probably infeasible - pivot was %g\n", alpha_);
#endif
                              //if (fabs(alpha_)<1.0e-4) {
                              //problemStatus_=1;
                              //} else {
#ifdef CLP_DEBUG
                              abort();
#endif
                              //}
                              problemStatus_ = -5;
                         } else {
                              problemStatus_ = 0;
#ifndef CLP_CHECK_NUMBER_PIVOTS
#define CLP_CHECK_NUMBER_PIVOTS 10
#endif
#if CLP_CHECK_NUMBER_PIVOTS < 20
                              if (numberPivots > CLP_CHECK_NUMBER_PIVOTS) {
#ifndef NDEBUG_CLP
                                   int nTotal = numberRows_ + numberColumns_;
                                   double * comp = CoinCopyOfArray(solution_, nTotal);
#endif
                                   computePrimals(rowActivityWork_, columnActivityWork_);
#ifndef NDEBUG_CLP
                                   double largest = 1.0e-5;
                                   int bad = -1;
                                   for (int i = 0; i < nTotal; i++) {
                                        double value = solution_[i];
                                        double larger = CoinMax(fabs(value), fabs(comp[i]));
                                        double tol = 1.0e-5 + 1.0e-5 * larger;
                                        double diff = fabs(value - comp[i]);
                                        if (diff - tol > largest) {
                                             bad = i;
                                             largest = diff - tol;
                                        }
                                   }
                                   if (bad >= 0)
				     COIN_DETAIL_PRINT(printf("bad %d old %g new %g\n", bad, comp[bad], solution_[bad]));
#endif
                                   checkPrimalSolution(rowActivityWork_, columnActivityWork_);
                                   if (numberPrimalInfeasibilities_) {
#ifdef CLP_INVESTIGATE
                                        printf("XXX Infeas ? %d inf summing to %g\n", numberPrimalInfeasibilities_,
                                               sumPrimalInfeasibilities_);
#endif
                                        problemStatus_ = -1;
                                        returnCode = -2;
                                   }
#ifndef NDEBUG_CLP
                                   memcpy(solution_, comp, nTotal * sizeof(double));
                                   delete [] comp;
#endif
                              }
#endif
                              if (!problemStatus_) {
                                   // make it look OK
                                   numberPrimalInfeasibilities_ = 0;
                                   sumPrimalInfeasibilities_ = 0.0;
                                   numberDualInfeasibilities_ = 0;
                                   sumDualInfeasibilities_ = 0.0;
                                   // May be perturbed
                                   if (perturbation_ == 101 || numberChanged_) {
                                        numberChanged_ = 0; // Number of variables with changed costs
                                        perturbation_ = 102; // stop any perturbations
                                        //double changeCost;
                                        //changeBounds(1,NULL,changeCost);
                                        createRim4(false);
                                        // make sure duals are current
                                        computeDuals(givenDuals);
                                        checkDualSolution();
                                        progress_.modifyObjective(-COIN_DBL_MAX);
                                        if (numberDualInfeasibilities_) {
                                             problemStatus_ = 10; // was -3;
                                        } else {
                                             computeObjectiveValue(true);
                                        }
                                   } else if (numberPivots) {
                                        computeObjectiveValue(true);
                                   }
                                   if (numberPivots < -1000) {
                                        // objective may be wrong
                                        objectiveValue_ = innerProduct(cost_, numberColumns_ + numberRows_, solution_);
                                        objectiveValue_ += objective_->nonlinearOffset();
                                        objectiveValue_ /= (objectiveScale_ * rhsScale_);
                                        if ((specialOptions_ & 16384) == 0) {
                                             // and dual_ may be wrong (i.e. for fixed or basic)
                                             CoinIndexedVector * arrayVector = rowArray_[1];
                                             arrayVector->clear();
                                             int iRow;
                                             double * array = arrayVector->denseVector();
                                             /* Use dual_ instead of array
                                                Even though dual_ is only numberRows_ long this is
                                                okay as gets permuted to longer rowArray_[2]
                                             */
                                             arrayVector->setDenseVector(dual_);
                                             int * index = arrayVector->getIndices();
                                             int number = 0;
                                             for (iRow = 0; iRow < numberRows_; iRow++) {
                                                  int iPivot = pivotVariable_[iRow];
                                                  double value = cost_[iPivot];
                                                  dual_[iRow] = value;
                                                  if (value) {
                                                       index[number++] = iRow;
                                                  }
                                             }
                                             arrayVector->setNumElements(number);
                                             // Extended duals before "updateTranspose"
                                             matrix_->dualExpanded(this, arrayVector, NULL, 0);
                                             // Btran basic costs
                                             rowArray_[2]->clear();
                                             factorization_->updateColumnTranspose(rowArray_[2], arrayVector);
                                             // and return vector
                                             arrayVector->setDenseVector(array);
                                        }
                                   }
                                   sumPrimalInfeasibilities_ = 0.0;
                              }
                              if ((specialOptions_&(1024 + 16384)) != 0 && !problemStatus_) {
                                   CoinIndexedVector * arrayVector = rowArray_[1];
                                   arrayVector->clear();
                                   double * rhs = arrayVector->denseVector();
                                   times(1.0, solution_, rhs);
#ifdef CHECK_ACCURACY
                                   bool bad = false;
#endif
                                   bool bad2 = false;
                                   int i;
                                   for ( i = 0; i < numberRows_; i++) {
                                        if (rhs[i] < rowLowerWork_[i] - primalTolerance_ ||
                                                  rhs[i] > rowUpperWork_[i] + primalTolerance_) {
                                             bad2 = true;
#ifdef CHECK_ACCURACY
                                             printf("row %d out of bounds %g, %g correct %g bad %g\n", i,
                                                    rowLowerWork_[i], rowUpperWork_[i],
                                                    rhs[i], rowActivityWork_[i]);
#endif
                                        } else if (fabs(rhs[i] - rowActivityWork_[i]) > 1.0e-3) {
#ifdef CHECK_ACCURACY
                                             bad = true;
                                             printf("row %d correct %g bad %g\n", i, rhs[i], rowActivityWork_[i]);
#endif
                                        }
                                        rhs[i] = 0.0;
                                   }
                                   for ( i = 0; i < numberColumns_; i++) {
                                        if (solution_[i] < columnLowerWork_[i] - primalTolerance_ ||
                                                  solution_[i] > columnUpperWork_[i] + primalTolerance_) {
                                             bad2 = true;
#ifdef CHECK_ACCURACY
                                             printf("column %d out of bounds %g, %g correct %g bad %g\n", i,
                                                    columnLowerWork_[i], columnUpperWork_[i],
                                                    solution_[i], columnActivityWork_[i]);
#endif
                                        }
                                   }
                                   if (bad2) {
                                        problemStatus_ = -3;
                                        returnCode = -2;
                                        // Force to re-factorize early next time
                                        int numberPivots = factorization_->pivots();
                                        forceFactorization_ = CoinMin(forceFactorization_, (numberPivots + 1) >> 1);
                                   }
                              }
                         }
                    }
               } else {
                    problemStatus_ = -3;
                    returnCode = -2;
                    // Force to re-factorize early next time
                    int numberPivots = factorization_->pivots();
                    forceFactorization_ = CoinMin(forceFactorization_, (numberPivots + 1) >> 1);
               }
               break;
          }
     }
     if (givenDuals) {
          CoinMemcpyN(dj_, numberRows_ + numberColumns_, givenDuals);
          // get rid of any values pass array
          delete [] candidateList;
     }
     delete [] dubiousWeights;
#ifdef CLP_REPORT_PROGRESS
     if (ixxxxxx > ixxyyyy - 5) {
          int nTotal = numberColumns_ + numberRows_;
          double oldObj = 0.0;
          double newObj = 0.0;
          for (int i = 0; i < nTotal; i++) {
               if (savePSol[i])
                    oldObj += savePSol[i] * saveCost[i];
               if (solution_[i])
                    newObj += solution_[i] * cost_[i];
               bool printIt = false;
               if (cost_[i] != saveCost[i])
                    printIt = true;
               if (status_[i] != saveStat[i])
                    printIt = true;
               if (printIt)
                    printf("%d old %d cost %g sol %g, new %d cost %g sol %g\n",
                           i, saveStat[i], saveCost[i], savePSol[i],
                           status_[i], cost_[i], solution_[i]);
               // difference
               savePSol[i] = solution_[i] - savePSol[i];
          }
          printf("exit pivots %d, old obj %g new %g\n",
                 factorization_->pivots(),
                 oldObj, newObj);
          memset(saveDj, 0, numberRows_ * sizeof(double));
          times(1.0, savePSol, saveDj);
          double largest = 1.0e-6;
          int k = -1;
          for (int i = 0; i < numberRows_; i++) {
               saveDj[i] -= savePSol[i+numberColumns_];
               if (fabs(saveDj[i]) > largest) {
                    largest = fabs(saveDj[i]);
                    k = i;
               }
          }
          if (k >= 0)
               printf("Not null %d %g\n", k, largest);
     }
     delete [] savePSol ;
     delete [] saveDj ;
     delete [] saveCost ;
     delete [] saveStat ;
#endif
     return returnCode;
}
/* The duals are updated by the given arrays.
   Returns number of infeasibilities.
   rowArray and columnarray will have flipped
   The output vector has movement (row length array) */
int
ClpSimplexDual::updateDualsInDual(CoinIndexedVector * rowArray,
                                  CoinIndexedVector * columnArray,
                                  CoinIndexedVector * outputArray,
                                  double theta,
                                  double & objectiveChange,
                                  bool fullRecompute)
{

     outputArray->clear();


     int numberInfeasibilities = 0;
     int numberRowInfeasibilities = 0;

     // get a tolerance
     double tolerance = dualTolerance_;
     // we can't really trust infeasibilities if there is dual error
     double error = CoinMin(1.0e-2, largestDualError_);
     // allow tolerance at least slightly bigger than standard
     tolerance = tolerance +  error;

     double changeObj = 0.0;

     // Coding is very similar but we can save a bit by splitting
     // Do rows
     if (!fullRecompute) {
          int i;
          double * COIN_RESTRICT reducedCost = djRegion(0);
          const double * COIN_RESTRICT lower = lowerRegion(0);
          const double * COIN_RESTRICT upper = upperRegion(0);
          const double * COIN_RESTRICT cost = costRegion(0);
          double * COIN_RESTRICT work;
          int number;
          int * COIN_RESTRICT which;
          const unsigned char * COIN_RESTRICT statusArray = status_ + numberColumns_;
          assert(rowArray->packedMode());
          work = rowArray->denseVector();
          number = rowArray->getNumElements();
          which = rowArray->getIndices();
          double multiplier[] = { -1.0, 1.0};
          for (i = 0; i < number; i++) {
               int iSequence = which[i];
               double alphaI = work[i];
               work[i] = 0.0;
               int iStatus = (statusArray[iSequence] & 3) - 1;
               if (iStatus) {
                    double value = reducedCost[iSequence] - theta * alphaI;
                    reducedCost[iSequence] = value;
                    double mult = multiplier[iStatus-1];
                    value *= mult;
                    if (value < -tolerance) {
                         // flipping bounds
                         double movement = mult * (lower[iSequence] - upper[iSequence]);
                         which[numberInfeasibilities++] = iSequence;
#ifndef NDEBUG
                         if (fabs(movement) >= 1.0e30)
                              resetFakeBounds(-1000 - iSequence);
#endif
#ifdef CLP_DEBUG
                         if ((handler_->logLevel() & 32))
                              printf("%d %d, new dj %g, alpha %g, movement %g\n",
                                     0, iSequence, value, alphaI, movement);
#endif
                         changeObj -= movement * cost[iSequence];
                         outputArray->quickAdd(iSequence, movement);
                    }
               }
          }
          // Do columns
          reducedCost = djRegion(1);
          lower = lowerRegion(1);
          upper = upperRegion(1);
          cost = costRegion(1);
          // set number of infeasibilities in row array
          numberRowInfeasibilities = numberInfeasibilities;
          rowArray->setNumElements(numberInfeasibilities);
          numberInfeasibilities = 0;
          work = columnArray->denseVector();
          number = columnArray->getNumElements();
          which = columnArray->getIndices();
          if ((moreSpecialOptions_ & 8) != 0) {
               const unsigned char * COIN_RESTRICT statusArray = status_;
               for (i = 0; i < number; i++) {
                    int iSequence = which[i];
                    double alphaI = work[i];
                    work[i] = 0.0;

                    int iStatus = (statusArray[iSequence] & 3) - 1;
                    if (iStatus) {
                         double value = reducedCost[iSequence] - theta * alphaI;
                         reducedCost[iSequence] = value;
                         double mult = multiplier[iStatus-1];
                         value *= mult;
                         if (value < -tolerance) {
                              // flipping bounds
                              double movement = mult * (upper[iSequence] - lower[iSequence]);
                              which[numberInfeasibilities++] = iSequence;
#ifndef NDEBUG
                              if (fabs(movement) >= 1.0e30)
                                   resetFakeBounds(-1000 - iSequence);
#endif
#ifdef CLP_DEBUG
                              if ((handler_->logLevel() & 32))
                                   printf("%d %d, new dj %g, alpha %g, movement %g\n",
                                          1, iSequence, value, alphaI, movement);
#endif
                              changeObj += movement * cost[iSequence];
                              matrix_->add(this, outputArray, iSequence, movement);
                         }
                    }
               }
          } else {
               for (i = 0; i < number; i++) {
                    int iSequence = which[i];
                    double alphaI = work[i];
                    work[i] = 0.0;

                    Status status = getStatus(iSequence);
                    if (status == atLowerBound) {
                         double value = reducedCost[iSequence] - theta * alphaI;
                         reducedCost[iSequence] = value;
                         double movement = 0.0;

                         if (value < -tolerance) {
                              // to upper bound
                              which[numberInfeasibilities++] = iSequence;
                              movement = upper[iSequence] - lower[iSequence];
#ifndef NDEBUG
                              if (fabs(movement) >= 1.0e30)
                                   resetFakeBounds(-1000 - iSequence);
#endif
#ifdef CLP_DEBUG
                              if ((handler_->logLevel() & 32))
                                   printf("%d %d, new dj %g, alpha %g, movement %g\n",
                                          1, iSequence, value, alphaI, movement);
#endif
                              changeObj += movement * cost[iSequence];
                              matrix_->add(this, outputArray, iSequence, movement);
                         }
                    } else if (status == atUpperBound) {
                         double value = reducedCost[iSequence] - theta * alphaI;
                         reducedCost[iSequence] = value;
                         double movement = 0.0;

                         if (value > tolerance) {
                              // to lower bound (if swap)
                              which[numberInfeasibilities++] = iSequence;
                              movement = lower[iSequence] - upper[iSequence];
#ifndef NDEBUG
                              if (fabs(movement) >= 1.0e30)
                                   resetFakeBounds(-1000 - iSequence);
#endif
#ifdef CLP_DEBUG
                              if ((handler_->logLevel() & 32))
                                   printf("%d %d, new dj %g, alpha %g, movement %g\n",
                                          1, iSequence, value, alphaI, movement);
#endif
                              changeObj += movement * cost[iSequence];
                              matrix_->add(this, outputArray, iSequence, movement);
                         }
                    } else if (status == isFree) {
                         double value = reducedCost[iSequence] - theta * alphaI;
                         reducedCost[iSequence] = value;
                    }
               }
          }
     } else  {
          double * COIN_RESTRICT solution = solutionRegion(0);
          double * COIN_RESTRICT reducedCost = djRegion(0);
          const double * COIN_RESTRICT lower = lowerRegion(0);
          const double * COIN_RESTRICT upper = upperRegion(0);
          const double * COIN_RESTRICT cost = costRegion(0);
          int * COIN_RESTRICT which;
          which = rowArray->getIndices();
          int iSequence;
          for (iSequence = 0; iSequence < numberRows_; iSequence++) {
               double value = reducedCost[iSequence];

               Status status = getStatus(iSequence + numberColumns_);
               // more likely to be at upper bound ?
               if (status == atUpperBound) {
                    double movement = 0.0;
                    //#define NO_SWAP7
                    if (value > tolerance) {
                         // to lower bound (if swap)
                         // put back alpha
                         which[numberInfeasibilities++] = iSequence;
                         movement = lower[iSequence] - upper[iSequence];
                         changeObj += movement * cost[iSequence];
                         outputArray->quickAdd(iSequence, -movement);
#ifndef NDEBUG
                         if (fabs(movement) >= 1.0e30)
                              resetFakeBounds(-1000 - iSequence);
#endif
#ifndef NO_SWAP7
                    } else if (value > -tolerance) {
                         // at correct bound but may swap
                         FakeBound bound = getFakeBound(iSequence + numberColumns_);
                         if (bound == ClpSimplexDual::upperFake) {
                              movement = lower[iSequence] - upper[iSequence];
#ifndef NDEBUG
                              if (fabs(movement) >= 1.0e30)
                                   resetFakeBounds(-1000 - iSequence);
#endif
                              setStatus(iSequence + numberColumns_, atLowerBound);
                              solution[iSequence] = lower[iSequence];
                              changeObj += movement * cost[iSequence];
                              //numberFake_--;
                              //setFakeBound(iSequence+numberColumns_,noFake);
                         }
#endif
                    }
               } else if (status == atLowerBound) {
                    double movement = 0.0;

                    if (value < -tolerance) {
                         // to upper bound
                         // put back alpha
                         which[numberInfeasibilities++] = iSequence;
                         movement = upper[iSequence] - lower[iSequence];
#ifndef NDEBUG
                         if (fabs(movement) >= 1.0e30)
                              resetFakeBounds(-1000 - iSequence);
#endif
                         changeObj += movement * cost[iSequence];
                         outputArray->quickAdd(iSequence, -movement);
#ifndef NO_SWAP7
                    } else if (value < tolerance) {
                         // at correct bound but may swap
                         FakeBound bound = getFakeBound(iSequence + numberColumns_);
                         if (bound == ClpSimplexDual::lowerFake) {
                              movement = upper[iSequence] - lower[iSequence];
#ifndef NDEBUG
                              if (fabs(movement) >= 1.0e30)
                                   resetFakeBounds(-1000 - iSequence);
#endif
                              setStatus(iSequence + numberColumns_, atUpperBound);
                              solution[iSequence] = upper[iSequence];
                              changeObj += movement * cost[iSequence];
                              //numberFake_--;
                              //setFakeBound(iSequence+numberColumns_,noFake);
                         }
#endif
                    }
               }
          }
          // Do columns
          solution = solutionRegion(1);
          reducedCost = djRegion(1);
          lower = lowerRegion(1);
          upper = upperRegion(1);
          cost = costRegion(1);
          // set number of infeasibilities in row array
          numberRowInfeasibilities = numberInfeasibilities;
          rowArray->setNumElements(numberInfeasibilities);
          numberInfeasibilities = 0;
          which = columnArray->getIndices();
          for (iSequence = 0; iSequence < numberColumns_; iSequence++) {
               double value = reducedCost[iSequence];

               Status status = getStatus(iSequence);
               if (status == atLowerBound) {
                    double movement = 0.0;

                    if (value < -tolerance) {
                         // to upper bound
                         // put back alpha
                         which[numberInfeasibilities++] = iSequence;
                         movement = upper[iSequence] - lower[iSequence];
#ifndef NDEBUG
                         if (fabs(movement) >= 1.0e30)
                              resetFakeBounds(-1000 - iSequence);
#endif
                         changeObj += movement * cost[iSequence];
                         matrix_->add(this, outputArray, iSequence, movement);
#ifndef NO_SWAP7
                    } else if (value < tolerance) {
                         // at correct bound but may swap
                         FakeBound bound = getFakeBound(iSequence);
                         if (bound == ClpSimplexDual::lowerFake) {
                              movement = upper[iSequence] - lower[iSequence];
#ifndef NDEBUG
                              if (fabs(movement) >= 1.0e30)
                                   resetFakeBounds(-1000 - iSequence);
#endif
                              setStatus(iSequence, atUpperBound);
                              solution[iSequence] = upper[iSequence];
                              changeObj += movement * cost[iSequence];
                              //numberFake_--;
                              //setFakeBound(iSequence,noFake);
                         }
#endif
                    }
               } else if (status == atUpperBound) {
                    double movement = 0.0;

                    if (value > tolerance) {
                         // to lower bound (if swap)
                         // put back alpha
                         which[numberInfeasibilities++] = iSequence;
                         movement = lower[iSequence] - upper[iSequence];
#ifndef NDEBUG
                         if (fabs(movement) >= 1.0e30)
                              resetFakeBounds(-1000 - iSequence);
#endif
                         changeObj += movement * cost[iSequence];
                         matrix_->add(this, outputArray, iSequence, movement);
#ifndef NO_SWAP7
                    } else if (value > -tolerance) {
                         // at correct bound but may swap
                         FakeBound bound = getFakeBound(iSequence);
                         if (bound == ClpSimplexDual::upperFake) {
                              movement = lower[iSequence] - upper[iSequence];
#ifndef NDEBUG
                              if (fabs(movement) >= 1.0e30)
                                   resetFakeBounds(-1000 - iSequence);
#endif
                              setStatus(iSequence, atLowerBound);
                              solution[iSequence] = lower[iSequence];
                              changeObj += movement * cost[iSequence];
                              //numberFake_--;
                              //setFakeBound(iSequence,noFake);
                         }
#endif
                    }
               }
          }
     }

#ifdef CLP_DEBUG
     if (fullRecompute && numberFake_ && (handler_->logLevel() & 16) != 0)
          printf("%d fake after full update\n", numberFake_);
#endif
     // set number of infeasibilities
     columnArray->setNumElements(numberInfeasibilities);
     numberInfeasibilities += numberRowInfeasibilities;
     if (fullRecompute) {
          // do actual flips
          flipBounds(rowArray, columnArray);
     }
     objectiveChange += changeObj;
     return numberInfeasibilities;
}
void
ClpSimplexDual::updateDualsInValuesPass(CoinIndexedVector * rowArray,
                                        CoinIndexedVector * columnArray,
                                        double theta)
{

     // use a tighter tolerance except for all being okay
     double tolerance = dualTolerance_;

     // Coding is very similar but we can save a bit by splitting
     // Do rows
     {
          int i;
          double * reducedCost = djRegion(0);
          double * work;
          int number;
          int * which;
          work = rowArray->denseVector();
          number = rowArray->getNumElements();
          which = rowArray->getIndices();
          for (i = 0; i < number; i++) {
               int iSequence = which[i];
               double alphaI = work[i];
               double value = reducedCost[iSequence] - theta * alphaI;
               work[i] = 0.0;
               reducedCost[iSequence] = value;

               Status status = getStatus(iSequence + numberColumns_);
               // more likely to be at upper bound ?
               if (status == atUpperBound) {

                    if (value > tolerance)
                         reducedCost[iSequence] = 0.0;
               } else if (status == atLowerBound) {

                    if (value < -tolerance) {
                         reducedCost[iSequence] = 0.0;
                    }
               }
          }
     }
     rowArray->setNumElements(0);

     // Do columns
     {
          int i;
          double * reducedCost = djRegion(1);
          double * work;
          int number;
          int * which;
          work = columnArray->denseVector();
          number = columnArray->getNumElements();
          which = columnArray->getIndices();

          for (i = 0; i < number; i++) {
               int iSequence = which[i];
               double alphaI = work[i];
               double value = reducedCost[iSequence] - theta * alphaI;
               work[i] = 0.0;
               reducedCost[iSequence] = value;

               Status status = getStatus(iSequence);
               if (status == atLowerBound) {
                    if (value < -tolerance)
                         reducedCost[iSequence] = 0.0;
               } else if (status == atUpperBound) {
                    if (value > tolerance)
                         reducedCost[iSequence] = 0.0;
               }
          }
     }
     columnArray->setNumElements(0);
}
/*
   Chooses dual pivot row
   Would be faster with separate region to scan
   and will have this (with square of infeasibility) when steepest
   For easy problems we can just choose one of the first rows we look at
*/
void
ClpSimplexDual::dualRow(int alreadyChosen)
{
     // get pivot row using whichever method it is
     int chosenRow = -1;
#ifdef FORCE_FOLLOW
     bool forceThis = false;
     if (!fpFollow && strlen(forceFile)) {
          fpFollow = fopen(forceFile, "r");
          assert (fpFollow);
     }
     if (fpFollow) {
          if (numberIterations_ <= force_iteration) {
               // read to next Clp0102
               char temp[300];
               while (fgets(temp, 250, fpFollow)) {
                    if (strncmp(temp, "Clp0102", 7))
                         continue;
                    char cin, cout;
                    sscanf(temp + 9, "%d%*f%*s%*c%c%d%*s%*c%c%d",
                           &force_iteration, &cin, &force_in, &cout, &force_out);
                    if (cin == 'R')
                         force_in += numberColumns_;
                    if (cout == 'R')
                         force_out += numberColumns_;
                    forceThis = true;
                    assert (numberIterations_ == force_iteration - 1);
                    printf("Iteration %d will force %d out and %d in\n",
                           force_iteration, force_out, force_in);
                    alreadyChosen = force_out;
                    break;
               }
          } else {
               // use old
               forceThis = true;
          }
          if (!forceThis) {
               fclose(fpFollow);
               fpFollow = NULL;
               forceFile = "";
          }
     }
#endif
     //double freeAlpha = 0.0;
     if (alreadyChosen < 0) {
          // first see if any free variables and put them in basis
          int nextFree = nextSuperBasic();
          //nextFree=-1; //off
          if (nextFree >= 0) {
               // unpack vector and find a good pivot
               unpack(rowArray_[1], nextFree);
               factorization_->updateColumn(rowArray_[2], rowArray_[1]);

               double * work = rowArray_[1]->denseVector();
               int number = rowArray_[1]->getNumElements();
               int * which = rowArray_[1]->getIndices();
               double bestFeasibleAlpha = 0.0;
               int bestFeasibleRow = -1;
               double bestInfeasibleAlpha = 0.0;
               int bestInfeasibleRow = -1;
               int i;

               for (i = 0; i < number; i++) {
                    int iRow = which[i];
                    double alpha = fabs(work[iRow]);
                    if (alpha > 1.0e-3) {
                         int iSequence = pivotVariable_[iRow];
                         double value = solution_[iSequence];
                         double lower = lower_[iSequence];
                         double upper = upper_[iSequence];
                         double infeasibility = 0.0;
                         if (value > upper)
                              infeasibility = value - upper;
                         else if (value < lower)
                              infeasibility = lower - value;
                         if (infeasibility * alpha > bestInfeasibleAlpha && alpha > 1.0e-1) {
                              if (!flagged(iSequence)) {
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
                    chosenRow = bestInfeasibleRow;
               else if (bestFeasibleAlpha > 1.0e-2)
                    chosenRow = bestFeasibleRow;
               if (chosenRow >= 0) {
                    pivotRow_ = chosenRow;
                    //freeAlpha = work[chosenRow];
               }
               rowArray_[1]->clear();
          }
     } else {
          // in values pass
          chosenRow = alreadyChosen;
#ifdef FORCE_FOLLOW
          if(forceThis) {
               alreadyChosen = -1;
               chosenRow = -1;
               for (int i = 0; i < numberRows_; i++) {
                    if (pivotVariable_[i] == force_out) {
                         chosenRow = i;
                         break;
                    }
               }
               assert (chosenRow >= 0);
          }
#endif
          pivotRow_ = chosenRow;
     }
     if (chosenRow < 0)
          pivotRow_ = dualRowPivot_->pivotRow();

     if (pivotRow_ >= 0) {
          sequenceOut_ = pivotVariable_[pivotRow_];
          valueOut_ = solution_[sequenceOut_];
          lowerOut_ = lower_[sequenceOut_];
          upperOut_ = upper_[sequenceOut_];
          if (alreadyChosen < 0) {
               // if we have problems we could try other way and hope we get a
               // zero pivot?
               if (valueOut_ > upperOut_) {
                    directionOut_ = -1;
                    dualOut_ = valueOut_ - upperOut_;
               } else if (valueOut_ < lowerOut_) {
                    directionOut_ = 1;
                    dualOut_ = lowerOut_ - valueOut_;
               } else {
#if 1
                    // odd (could be free) - it's feasible - go to nearest
                    if (valueOut_ - lowerOut_ < upperOut_ - valueOut_) {
                         directionOut_ = 1;
                         dualOut_ = lowerOut_ - valueOut_;
                    } else {
                         directionOut_ = -1;
                         dualOut_ = valueOut_ - upperOut_;
                    }
#else
                    // odd (could be free) - it's feasible - improve obj
                    printf("direction from alpha of %g is %d\n",
                           freeAlpha, freeAlpha > 0.0 ? 1 : -1);
                    if (valueOut_ - lowerOut_ > 1.0e20)
                         freeAlpha = 1.0;
                    else if(upperOut_ - valueOut_ > 1.0e20)
                         freeAlpha = -1.0;
                    //if (valueOut_-lowerOut_<upperOut_-valueOut_) {
                    if (freeAlpha < 0.0) {
                         directionOut_ = 1;
                         dualOut_ = lowerOut_ - valueOut_;
                    } else {
                         directionOut_ = -1;
                         dualOut_ = valueOut_ - upperOut_;
                    }
                    printf("direction taken %d - bounds %g %g %g\n",
                           directionOut_, lowerOut_, valueOut_, upperOut_);
#endif
               }
#ifdef CLP_DEBUG
               assert(dualOut_ >= 0.0);
#endif
          } else {
               // in values pass so just use sign of dj
               // We don't want to go through any barriers so set dualOut low
               // free variables will never be here
               dualOut_ = 1.0e-6;
               if (dj_[sequenceOut_] > 0.0) {
                    // this will give a -1 in pivot row (as slacks are -1.0)
                    directionOut_ = 1;
               } else {
                    directionOut_ = -1;
               }
          }
     }
     return ;
}
// Checks if any fake bounds active - if so returns number and modifies
// dualBound_ and everything.
// Free variables will be left as free
// Returns number of bounds changed if >=0
// Returns -1 if not initialize and no effect
// Fills in changeVector which can be used to see if unbounded
// and cost of change vector
int
ClpSimplexDual::changeBounds(int initialize,
                             CoinIndexedVector * outputArray,
                             double & changeCost)
{
     numberFake_ = 0;
     if (!initialize) {
          int numberInfeasibilities;
          double newBound;
          newBound = 5.0 * dualBound_;
          numberInfeasibilities = 0;
          changeCost = 0.0;
          // put back original bounds and then check
          createRim1(false);
          int iSequence;
          // bounds will get bigger - just look at ones at bounds
          for (iSequence = 0; iSequence < numberRows_ + numberColumns_; iSequence++) {
               double lowerValue = lower_[iSequence];
               double upperValue = upper_[iSequence];
               double value = solution_[iSequence];
               setFakeBound(iSequence, ClpSimplexDual::noFake);
               switch(getStatus(iSequence)) {

               case basic:
               case ClpSimplex::isFixed:
                    break;
               case isFree:
               case superBasic:
                    break;
               case atUpperBound:
                    if (fabs(value - upperValue) > primalTolerance_)
                         numberInfeasibilities++;
                    break;
               case atLowerBound:
                    if (fabs(value - lowerValue) > primalTolerance_)
                         numberInfeasibilities++;
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
                    double lowerValue = lower_[iSequence];
                    double upperValue = upper_[iSequence];
                    double newLowerValue;
                    double newUpperValue;
                    Status status = getStatus(iSequence);
                    if (status == atUpperBound ||
                              status == atLowerBound) {
                         double value = solution_[iSequence];
                         if (value - lowerValue <= upperValue - value) {
                              newLowerValue = CoinMax(lowerValue, value - 0.666667 * newBound);
                              newUpperValue = CoinMin(upperValue, newLowerValue + newBound);
                         } else {
                              newUpperValue = CoinMin(upperValue, value + 0.666667 * newBound);
                              newLowerValue = CoinMax(lowerValue, newUpperValue - newBound);
                         }
                         lower_[iSequence] = newLowerValue;
                         upper_[iSequence] = newUpperValue;
                         if (newLowerValue > lowerValue) {
                              if (newUpperValue < upperValue) {
                                   setFakeBound(iSequence, ClpSimplexDual::bothFake);
#ifdef CLP_INVESTIGATE
                                   abort(); // No idea what should happen here - I have never got here
#endif
                                   numberFake_++;
                              } else {
                                   setFakeBound(iSequence, ClpSimplexDual::lowerFake);
                                   numberFake_++;
                              }
                         } else {
                              if (newUpperValue < upperValue) {
                                   setFakeBound(iSequence, ClpSimplexDual::upperFake);
                                   numberFake_++;
                              }
                         }
                         if (status == atUpperBound)
                              solution_[iSequence] = newUpperValue;
                         else
                              solution_[iSequence] = newLowerValue;
                         double movement = solution_[iSequence] - value;
                         if (movement && outputArray) {
                              if (iSequence >= numberColumns_) {
                                   outputArray->quickAdd(iSequence, -movement);
                                   changeCost += movement * cost_[iSequence];
                              } else {
                                   matrix_->add(this, outputArray, iSequence, movement);
                                   changeCost += movement * cost_[iSequence];
                              }
                         }
                    }
               }
               dualBound_ = newBound;
          } else {
               numberInfeasibilities = -1;
          }
          return numberInfeasibilities;
     } else if (initialize == 1 || initialize == 3) {
          int iSequence;
          if (initialize == 3) {
               for (iSequence = 0; iSequence < numberRows_ + numberColumns_; iSequence++) {
                    setFakeBound(iSequence, ClpSimplexDual::noFake);
               }
          }
          double testBound = 0.999999 * dualBound_;
          for (iSequence = 0; iSequence < numberRows_ + numberColumns_; iSequence++) {
               Status status = getStatus(iSequence);
               if (status == atUpperBound ||
                         status == atLowerBound) {
                    double lowerValue = lower_[iSequence];
                    double upperValue = upper_[iSequence];
                    double value = solution_[iSequence];
                    if (lowerValue > -largeValue_ || upperValue < largeValue_) {
                         if (true || lowerValue - value > -0.5 * dualBound_ ||
                                   upperValue - value < 0.5 * dualBound_) {
                              if (fabs(lowerValue - value) <= fabs(upperValue - value)) {
                                   if (upperValue > lowerValue + testBound) {
                                        if (getFakeBound(iSequence) == ClpSimplexDual::noFake)
                                             numberFake_++;
                                        upper_[iSequence] = lowerValue + dualBound_;
                                        setFakeBound(iSequence, ClpSimplexDual::upperFake);
                                   }
                              } else {
                                   if (lowerValue < upperValue - testBound) {
                                        if (getFakeBound(iSequence) == ClpSimplexDual::noFake)
                                             numberFake_++;
                                        lower_[iSequence] = upperValue - dualBound_;
                                        setFakeBound(iSequence, ClpSimplexDual::lowerFake);
                                   }
                              }
                         } else {
                              if (getFakeBound(iSequence) == ClpSimplexDual::noFake)
                                   numberFake_++;
                              lower_[iSequence] = -0.5 * dualBound_;
                              upper_[iSequence] = 0.5 * dualBound_;
                              setFakeBound(iSequence, ClpSimplexDual::bothFake);
                              abort();
                         }
                         if (status == atUpperBound)
                              solution_[iSequence] = upper_[iSequence];
                         else
                              solution_[iSequence] = lower_[iSequence];
                    } else {
                         // set non basic free variables to fake bounds
                         // I don't think we should ever get here
                         CoinAssert(!("should not be here"));
                         lower_[iSequence] = -0.5 * dualBound_;
                         upper_[iSequence] = 0.5 * dualBound_;
                         setFakeBound(iSequence, ClpSimplexDual::bothFake);
                         numberFake_++;
                         setStatus(iSequence, atUpperBound);
                         solution_[iSequence] = 0.5 * dualBound_;
                    }
               } else if (status == basic) {
                    // make sure not at fake bound and bounds correct
                    setFakeBound(iSequence, ClpSimplexDual::noFake);
                    double gap = upper_[iSequence] - lower_[iSequence];
                    if (gap > 0.5 * dualBound_ && gap < 2.0 * dualBound_) {
                         if (iSequence < numberColumns_) {
                              if (columnScale_) {
                                   double multiplier = rhsScale_ * inverseColumnScale_[iSequence];
                                   // lower
                                   double value = columnLower_[iSequence];
                                   if (value > -1.0e30) {
                                        value *= multiplier;
                                   }
                                   lower_[iSequence] = value;
                                   // upper
                                   value = columnUpper_[iSequence];
                                   if (value < 1.0e30) {
                                        value *= multiplier;
                                   }
                                   upper_[iSequence] = value;
                              } else {
                                   lower_[iSequence] = columnLower_[iSequence];;
                                   upper_[iSequence] = columnUpper_[iSequence];;
                              }
                         } else {
                              int iRow = iSequence - numberColumns_;
                              if (rowScale_) {
                                   // lower
                                   double multiplier = rhsScale_ * rowScale_[iRow];
                                   double value = rowLower_[iRow];
                                   if (value > -1.0e30) {
                                        value *= multiplier;
                                   }
                                   lower_[iSequence] = value;
                                   // upper
                                   value = rowUpper_[iRow];
                                   if (value < 1.0e30) {
                                        value *= multiplier;
                                   }
                                   upper_[iSequence] = value;
                              } else {
                                   lower_[iSequence] = rowLower_[iRow];;
                                   upper_[iSequence] = rowUpper_[iRow];;
                              }
                         }
                    }
               }
          }

          return 1;
     } else {
          // just reset changed ones
          if (columnScale_) {
               int iSequence;
               for (iSequence = 0; iSequence < numberColumns_; iSequence++) {
                    FakeBound fakeStatus = getFakeBound(iSequence);
                    if (fakeStatus != noFake) {
                         if ((static_cast<int> (fakeStatus) & 1) != 0) {
                              // lower
                              double value = columnLower_[iSequence];
                              if (value > -1.0e30) {
                                   double multiplier = rhsScale_ * inverseColumnScale_[iSequence];
                                   value *= multiplier;
                              }
                              columnLowerWork_[iSequence] = value;
                         }
                         if ((static_cast<int> (fakeStatus) & 2) != 0) {
                              // upper
                              double value = columnUpper_[iSequence];
                              if (value < 1.0e30) {
                                   double multiplier = rhsScale_ * inverseColumnScale_[iSequence];
                                   value *= multiplier;
                              }
                              columnUpperWork_[iSequence] = value;
                         }
                    }
               }
               for (iSequence = 0; iSequence < numberRows_; iSequence++) {
                    FakeBound fakeStatus = getFakeBound(iSequence + numberColumns_);
                    if (fakeStatus != noFake) {
                         if ((static_cast<int> (fakeStatus) & 1) != 0) {
                              // lower
                              double value = rowLower_[iSequence];
                              if (value > -1.0e30) {
                                   double multiplier = rhsScale_ * rowScale_[iSequence];
                                   value *= multiplier;
                              }
                              rowLowerWork_[iSequence] = value;
                         }
                         if ((static_cast<int> (fakeStatus) & 2) != 0) {
                              // upper
                              double value = rowUpper_[iSequence];
                              if (value < 1.0e30) {
                                   double multiplier = rhsScale_ * rowScale_[iSequence];
                                   value *= multiplier;
                              }
                              rowUpperWork_[iSequence] = value;
                         }
                    }
               }
          } else {
               int iSequence;
               for (iSequence = 0; iSequence < numberColumns_; iSequence++) {
                    FakeBound fakeStatus = getFakeBound(iSequence);
                    if ((static_cast<int> (fakeStatus) & 1) != 0) {
                         // lower
                         columnLowerWork_[iSequence] = columnLower_[iSequence];
                    }
                    if ((static_cast<int> (fakeStatus) & 2) != 0) {
                         // upper
                         columnUpperWork_[iSequence] = columnUpper_[iSequence];
                    }
               }
               for (iSequence = 0; iSequence < numberRows_; iSequence++) {
                    FakeBound fakeStatus = getFakeBound(iSequence + numberColumns_);
                    if ((static_cast<int> (fakeStatus) & 1) != 0) {
                         // lower
                         rowLowerWork_[iSequence] = rowLower_[iSequence];
                    }
                    if ((static_cast<int> (fakeStatus) & 2) != 0) {
                         // upper
                         rowUpperWork_[iSequence] = rowUpper_[iSequence];
                    }
               }
          }
          return 0;
     }
}
int
ClpSimplexDual::dualColumn0(const CoinIndexedVector * rowArray,
                            const CoinIndexedVector * columnArray,
                            CoinIndexedVector * spareArray,
                            double acceptablePivot,
                            double & upperReturn, double &bestReturn, double & badFree)
{
     // do first pass to get possibles
     double * spare = spareArray->denseVector();
     int * index = spareArray->getIndices();
     const double * work;
     int number;
     const int * which;
     const double * reducedCost;
     // We can also see if infeasible or pivoting on free
     double tentativeTheta = 1.0e15;
     double upperTheta = 1.0e31;
     double freePivot = acceptablePivot;
     double bestPossible = 0.0;
     int numberRemaining = 0;
     int i;
     badFree = 0.0;
     if ((moreSpecialOptions_ & 8) != 0) {
          // No free or super basic
          double multiplier[] = { -1.0, 1.0};
          double dualT = - dualTolerance_;
          for (int iSection = 0; iSection < 2; iSection++) {

               int addSequence;
               unsigned char * statusArray;
               if (!iSection) {
                    work = rowArray->denseVector();
                    number = rowArray->getNumElements();
                    which = rowArray->getIndices();
                    reducedCost = rowReducedCost_;
                    addSequence = numberColumns_;
                    statusArray = status_ + numberColumns_;
               } else {
                    work = columnArray->denseVector();
                    number = columnArray->getNumElements();
                    which = columnArray->getIndices();
                    reducedCost = reducedCostWork_;
                    addSequence = 0;
                    statusArray = status_;
               }

               for (i = 0; i < number; i++) {
                    int iSequence = which[i];
                    double alpha;
                    double oldValue;
                    double value;

                    assert (getStatus(iSequence + addSequence) != isFree
                            && getStatus(iSequence + addSequence) != superBasic);
                    int iStatus = (statusArray[iSequence] & 3) - 1;
                    if (iStatus) {
                         double mult = multiplier[iStatus-1];
                         alpha = work[i] * mult;
                         if (alpha > 0.0) {
                              oldValue = reducedCost[iSequence] * mult;
                              value = oldValue - tentativeTheta * alpha;
                              if (value < dualT) {
                                   bestPossible = CoinMax(bestPossible, alpha);
                                   value = oldValue - upperTheta * alpha;
                                   if (value < dualT && alpha >= acceptablePivot) {
                                        upperTheta = (oldValue - dualT) / alpha;
                                        //tentativeTheta = CoinMin(2.0*upperTheta,tentativeTheta);
                                   }
                                   // add to list
                                   spare[numberRemaining] = alpha * mult;
                                   index[numberRemaining++] = iSequence + addSequence;
                              }
                         }
                    }
               }
          }
     } else {
          // some free or super basic
          for (int iSection = 0; iSection < 2; iSection++) {

               int addSequence;

               if (!iSection) {
                    work = rowArray->denseVector();
                    number = rowArray->getNumElements();
                    which = rowArray->getIndices();
                    reducedCost = rowReducedCost_;
                    addSequence = numberColumns_;
               } else {
                    work = columnArray->denseVector();
                    number = columnArray->getNumElements();
                    which = columnArray->getIndices();
                    reducedCost = reducedCostWork_;
                    addSequence = 0;
               }

               for (i = 0; i < number; i++) {
                    int iSequence = which[i];
                    double alpha;
                    double oldValue;
                    double value;
                    bool keep;

                    switch(getStatus(iSequence + addSequence)) {

                    case basic:
                    case ClpSimplex::isFixed:
                         break;
                    case isFree:
                    case superBasic:
                         alpha = work[i];
                         bestPossible = CoinMax(bestPossible, fabs(alpha));
                         oldValue = reducedCost[iSequence];
                         // If free has to be very large - should come in via dualRow
                         //if (getStatus(iSequence+addSequence)==isFree&&fabs(alpha)<1.0e-3)
                         //break;
                         if (oldValue > dualTolerance_) {
                              keep = true;
                         } else if (oldValue < -dualTolerance_) {
                              keep = true;
                         } else {
                              if (fabs(alpha) > CoinMax(10.0 * acceptablePivot, 1.0e-5)) {
                                   keep = true;
                              } else {
                                   keep = false;
                                   badFree = CoinMax(badFree, fabs(alpha));
                              }
                         }
                         if (keep) {
                              // free - choose largest
                              if (fabs(alpha) > freePivot) {
                                   freePivot = fabs(alpha);
                                   sequenceIn_ = iSequence + addSequence;
                                   theta_ = oldValue / alpha;
                                   alpha_ = alpha;
                              }
                         }
                         break;
                    case atUpperBound:
                         alpha = work[i];
                         oldValue = reducedCost[iSequence];
                         value = oldValue - tentativeTheta * alpha;
                         //assert (oldValue<=dualTolerance_*1.0001);
                         if (value > dualTolerance_) {
                              bestPossible = CoinMax(bestPossible, -alpha);
                              value = oldValue - upperTheta * alpha;
                              if (value > dualTolerance_ && -alpha >= acceptablePivot) {
                                   upperTheta = (oldValue - dualTolerance_) / alpha;
                                   //tentativeTheta = CoinMin(2.0*upperTheta,tentativeTheta);
                              }
                              // add to list
                              spare[numberRemaining] = alpha;
                              index[numberRemaining++] = iSequence + addSequence;
                         }
                         break;
                    case atLowerBound:
                         alpha = work[i];
                         oldValue = reducedCost[iSequence];
                         value = oldValue - tentativeTheta * alpha;
                         //assert (oldValue>=-dualTolerance_*1.0001);
                         if (value < -dualTolerance_) {
                              bestPossible = CoinMax(bestPossible, alpha);
                              value = oldValue - upperTheta * alpha;
                              if (value < -dualTolerance_ && alpha >= acceptablePivot) {
                                   upperTheta = (oldValue + dualTolerance_) / alpha;
                                   //tentativeTheta = CoinMin(2.0*upperTheta,tentativeTheta);
                              }
                              // add to list
                              spare[numberRemaining] = alpha;
                              index[numberRemaining++] = iSequence + addSequence;
                         }
                         break;
                    }
               }
          }
     }
     upperReturn = upperTheta;
     bestReturn = bestPossible;
     return numberRemaining;
}
/*
   Row array has row part of pivot row (as duals so sign may be switched)
   Column array has column part.
   This chooses pivot column.
   Spare array will be needed when we start getting clever.
   We will check for basic so spare array will never overflow.
   If necessary will modify costs
*/
double
ClpSimplexDual::dualColumn(CoinIndexedVector * rowArray,
                           CoinIndexedVector * columnArray,
                           CoinIndexedVector * spareArray,
                           CoinIndexedVector * spareArray2,
                           double acceptablePivot,
                           CoinBigIndex * /*dubiousWeights*/)
{
     int numberPossiblySwapped = 0;
     int numberRemaining = 0;

     double totalThru = 0.0; // for when variables flip
     //double saveAcceptable=acceptablePivot;
     //acceptablePivot=1.0e-9;

     double bestEverPivot = acceptablePivot;
     int lastSequence = -1;
     double lastPivot = 0.0;
     double upperTheta;
     double newTolerance = dualTolerance_;
     //newTolerance = dualTolerance_+1.0e-6*dblParam_[ClpDualTolerance];
     // will we need to increase tolerance
     //bool thisIncrease = false;
     // If we think we need to modify costs (not if something from broad sweep)
     bool modifyCosts = false;
     // Increase in objective due to swapping bounds (may be negative)
     double increaseInObjective = 0.0;

     // use spareArrays to put ones looked at in
     // we are going to flip flop between
     int iFlip = 0;
     // Possible list of pivots
     int interesting[2];
     // where possible swapped ones are
     int swapped[2];
     // for zeroing out arrays after
     int marker[2][2];
     // pivot elements
     double * array[2], * spare, * spare2;
     // indices
     int * indices[2], * index, * index2;
     spareArray2->clear();
     array[0] = spareArray->denseVector();
     indices[0] = spareArray->getIndices();
     spare = array[0];
     index = indices[0];
     array[1] = spareArray2->denseVector();
     indices[1] = spareArray2->getIndices();
     int i;

     // initialize lists
     for (i = 0; i < 2; i++) {
          interesting[i] = 0;
          swapped[i] = numberColumns_;
          marker[i][0] = 0;
          marker[i][1] = numberColumns_;
     }
     /*
       First we get a list of possible pivots.  We can also see if the
       problem looks infeasible or whether we want to pivot in free variable.
       This may make objective go backwards but can only happen a finite
       number of times and I do want free variables basic.

       Then we flip back and forth.  At the start of each iteration
       interesting[iFlip] should have possible candidates and swapped[iFlip]
       will have pivots if we decide to take a previous pivot.
       At end of each iteration interesting[1-iFlip] should have
       candidates if we go through this theta and swapped[1-iFlip]
       pivots if we don't go through.

       At first we increase theta and see what happens.  We start
       theta at a reasonable guess.  If in right area then we do bit by bit.

      */

     // do first pass to get possibles
     upperTheta = 1.0e31;
     double bestPossible = 0.0;
     double badFree = 0.0;
     alpha_ = 0.0;
     if (spareIntArray_[0] >= 0) {
          numberRemaining = dualColumn0(rowArray, columnArray, spareArray,
                                        acceptablePivot, upperTheta, bestPossible, badFree);
     } else {
          // already done
          numberRemaining = spareArray->getNumElements();
          spareArray->setNumElements(0);
          upperTheta = spareDoubleArray_[0];
          bestPossible = spareDoubleArray_[1];
          if (spareIntArray_[0] == -1) {
               theta_ = spareDoubleArray_[2];
               alpha_ = spareDoubleArray_[3];
               sequenceIn_ = spareIntArray_[1];
          } else {
#if 0
               int n = numberRemaining;
               double u = upperTheta;
               double b = bestPossible;
               upperTheta = 1.0e31;
               bestPossible = 0.0;
               numberRemaining = dualColumn0(rowArray, columnArray, spareArray,
                                             acceptablePivot, upperTheta, bestPossible, badFree);
               assert (n == numberRemaining);
               assert (fabs(b - bestPossible) < 1.0e-7);
               assert (fabs(u - upperTheta) < 1.0e-7);
#endif
          }
     }
     // switch off
     spareIntArray_[0] = 0;
     // We can also see if infeasible or pivoting on free
     double tentativeTheta = 1.0e25;
     interesting[0] = numberRemaining;
     marker[0][0] = numberRemaining;

     if (!numberRemaining && sequenceIn_ < 0)
          return 0.0; // Looks infeasible

     // If sum of bad small pivots too much
#define MORE_CAREFUL
#ifdef MORE_CAREFUL
     bool badSumPivots = false;
#endif
     if (sequenceIn_ >= 0) {
          // free variable - always choose
     } else {

          theta_ = 1.0e50;
          // now flip flop between spare arrays until reasonable theta
          tentativeTheta = CoinMax(10.0 * upperTheta, 1.0e-7);

          // loops increasing tentative theta until can't go through

          while (tentativeTheta < 1.0e22) {
               double thruThis = 0.0;

               double bestPivot = acceptablePivot;
               int bestSequence = -1;

               numberPossiblySwapped = numberColumns_;
               numberRemaining = 0;

               upperTheta = 1.0e50;

               spare = array[iFlip];
               index = indices[iFlip];
               spare2 = array[1-iFlip];
               index2 = indices[1-iFlip];

               // try 3 different ways
               // 1 bias increase by ones with slightly wrong djs
               // 2 bias by all
               // 3 bias by all - tolerance
#define TRYBIAS 3


               double increaseInThis = 0.0; //objective increase in this loop

               for (i = 0; i < interesting[iFlip]; i++) {
                    int iSequence = index[i];
                    double alpha = spare[i];
                    double oldValue = dj_[iSequence];
                    double value = oldValue - tentativeTheta * alpha;

                    if (alpha < 0.0) {
                         //at upper bound
                         if (value > newTolerance) {
                              double range = upper_[iSequence] - lower_[iSequence];
                              thruThis -= range * alpha;
#if TRYBIAS==1
                              if (oldValue > 0.0)
                                   increaseInThis -= oldValue * range;
#elif TRYBIAS==2
                              increaseInThis -= oldValue * range;
#else
                              increaseInThis -= (oldValue + dualTolerance_) * range;
#endif
                              // goes on swapped list (also means candidates if too many)
                              spare2[--numberPossiblySwapped] = alpha;
                              index2[numberPossiblySwapped] = iSequence;
                              if (fabs(alpha) > bestPivot) {
                                   bestPivot = fabs(alpha);
                                   bestSequence = numberPossiblySwapped;
                              }
                         } else {
                              value = oldValue - upperTheta * alpha;
                              if (value > newTolerance && -alpha >= acceptablePivot)
                                   upperTheta = (oldValue - newTolerance) / alpha;
                              spare2[numberRemaining] = alpha;
                              index2[numberRemaining++] = iSequence;
                         }
                    } else {
                         // at lower bound
                         if (value < -newTolerance) {
                              double range = upper_[iSequence] - lower_[iSequence];
                              thruThis += range * alpha;
                              //?? is this correct - and should we look at good ones
#if TRYBIAS==1
                              if (oldValue < 0.0)
                                   increaseInThis += oldValue * range;
#elif TRYBIAS==2
                              increaseInThis += oldValue * range;
#else
                              increaseInThis += (oldValue - dualTolerance_) * range;
#endif
                              // goes on swapped list (also means candidates if too many)
                              spare2[--numberPossiblySwapped] = alpha;
                              index2[numberPossiblySwapped] = iSequence;
                              if (fabs(alpha) > bestPivot) {
                                   bestPivot = fabs(alpha);
                                   bestSequence = numberPossiblySwapped;
                              }
                         } else {
                              value = oldValue - upperTheta * alpha;
                              if (value < -newTolerance && alpha >= acceptablePivot)
                                   upperTheta = (oldValue + newTolerance) / alpha;
                              spare2[numberRemaining] = alpha;
                              index2[numberRemaining++] = iSequence;
                         }
                    }
               }
               swapped[1-iFlip] = numberPossiblySwapped;
               interesting[1-iFlip] = numberRemaining;
               marker[1-iFlip][0] = CoinMax(marker[1-iFlip][0], numberRemaining);
               marker[1-iFlip][1] = CoinMin(marker[1-iFlip][1], numberPossiblySwapped);

               if (totalThru + thruThis >= fabs(dualOut_) ||
                         increaseInObjective + increaseInThis < 0.0) {
                    // We should be pivoting in this batch
                    // so compress down to this lot
                    numberRemaining = 0;
                    for (i = numberColumns_ - 1; i >= swapped[1-iFlip]; i--) {
                         spare[numberRemaining] = spare2[i];
                         index[numberRemaining++] = index2[i];
                    }
                    interesting[iFlip] = numberRemaining;
                    int iTry;
#define MAXTRY 100
                    // first get ratio with tolerance
                    for (iTry = 0; iTry < MAXTRY; iTry++) {

                         upperTheta = 1.0e50;
                         numberPossiblySwapped = numberColumns_;
                         numberRemaining = 0;

                         increaseInThis = 0.0; //objective increase in this loop

                         thruThis = 0.0;

                         spare = array[iFlip];
                         index = indices[iFlip];
                         spare2 = array[1-iFlip];
                         index2 = indices[1-iFlip];
                         for (i = 0; i < interesting[iFlip]; i++) {
                              int iSequence = index[i];
                              double alpha = spare[i];
                              double oldValue = dj_[iSequence];
                              double value = oldValue - upperTheta * alpha;

                              if (alpha < 0.0) {
                                   //at upper bound
                                   if (value > newTolerance) {
                                        if (-alpha >= acceptablePivot) {
                                             upperTheta = (oldValue - newTolerance) / alpha;
                                        }
                                   }
                              } else {
                                   // at lower bound
                                   if (value < -newTolerance) {
                                        if (alpha >= acceptablePivot) {
                                             upperTheta = (oldValue + newTolerance) / alpha;
                                        }
                                   }
                              }
                         }
                         bestPivot = acceptablePivot;
                         sequenceIn_ = -1;
#ifdef DUBIOUS_WEIGHTS
                         double bestWeight = COIN_DBL_MAX;
#endif
                         double largestPivot = acceptablePivot;
                         // now choose largest and sum all ones which will go through
                         //printf("XX it %d number %d\n",numberIterations_,interesting[iFlip]);
                         // Sum of bad small pivots
#ifdef MORE_CAREFUL
                         double sumBadPivots = 0.0;
                         badSumPivots = false;
#endif
                         // Make sure upperTheta will work (-O2 and above gives problems)
                         upperTheta *= 1.0000000001;
                         for (i = 0; i < interesting[iFlip]; i++) {
                              int iSequence = index[i];
                              double alpha = spare[i];
                              double value = dj_[iSequence] - upperTheta * alpha;
                              double badDj = 0.0;

                              bool addToSwapped = false;

                              if (alpha < 0.0) {
                                   //at upper bound
                                   if (value >= 0.0) {
                                        addToSwapped = true;
#if TRYBIAS==1
                                        badDj = -CoinMax(dj_[iSequence], 0.0);
#elif TRYBIAS==2
                                        badDj = -dj_[iSequence];
#else
                                        badDj = -dj_[iSequence] - dualTolerance_;
#endif
                                   }
                              } else {
                                   // at lower bound
                                   if (value <= 0.0) {
                                        addToSwapped = true;
#if TRYBIAS==1
                                        badDj = CoinMin(dj_[iSequence], 0.0);
#elif TRYBIAS==2
                                        badDj = dj_[iSequence];
#else
                                        badDj = dj_[iSequence] - dualTolerance_;
#endif
                                   }
                              }
                              if (!addToSwapped) {
                                   // add to list of remaining
                                   spare2[numberRemaining] = alpha;
                                   index2[numberRemaining++] = iSequence;
                              } else {
                                   // add to list of swapped
                                   spare2[--numberPossiblySwapped] = alpha;
                                   index2[numberPossiblySwapped] = iSequence;
                                   // select if largest pivot
                                   bool take = false;
                                   double absAlpha = fabs(alpha);
#ifdef DUBIOUS_WEIGHTS
                                   // User could do anything to break ties here
                                   double weight;
                                   if (dubiousWeights)
                                        weight = dubiousWeights[iSequence];
                                   else
                                        weight = 1.0;
                                   weight += randomNumberGenerator_.randomDouble() * 1.0e-2;
                                   if (absAlpha > 2.0 * bestPivot) {
                                        take = true;
                                   } else if (absAlpha > largestPivot) {
                                        // could multiply absAlpha and weight
                                        if (weight * bestPivot < bestWeight * absAlpha)
                                             take = true;
                                   }
#else
                                   if (absAlpha > bestPivot)
                                        take = true;
#endif
#ifdef MORE_CAREFUL
                                   if (absAlpha < acceptablePivot && upperTheta < 1.0e20) {
                                        if (alpha < 0.0) {
                                             //at upper bound
                                             if (value > dualTolerance_) {
                                                  double gap = upper_[iSequence] - lower_[iSequence];
                                                  if (gap < 1.0e20)
                                                       sumBadPivots += value * gap;
                                                  else
                                                       sumBadPivots += 1.0e20;
                                                  //printf("bad %d alpha %g dj at upper %g\n",
                                                  //     iSequence,alpha,value);
                                             }
                                        } else {
                                             //at lower bound
                                             if (value < -dualTolerance_) {
                                                  double gap = upper_[iSequence] - lower_[iSequence];
                                                  if (gap < 1.0e20)
                                                       sumBadPivots -= value * gap;
                                                  else
                                                       sumBadPivots += 1.0e20;
                                                  //printf("bad %d alpha %g dj at lower %g\n",
                                                  //     iSequence,alpha,value);
                                             }
                                        }
                                   }
#endif
#ifdef FORCE_FOLLOW
                                   if (iSequence == force_in) {
                                        printf("taking %d - alpha %g best %g\n", force_in, absAlpha, largestPivot);
                                        take = true;
                                   }
#endif
                                   if (take) {
                                        sequenceIn_ = numberPossiblySwapped;
                                        bestPivot =  absAlpha;
                                        theta_ = dj_[iSequence] / alpha;
                                        largestPivot = CoinMax(largestPivot, 0.5 * bestPivot);
#ifdef DUBIOUS_WEIGHTS
                                        bestWeight = weight;
#endif
                                        //printf(" taken seq %d alpha %g weight %d\n",
                                        //   iSequence,absAlpha,dubiousWeights[iSequence]);
                                   } else {
                                        //printf(" not taken seq %d alpha %g weight %d\n",
                                        //   iSequence,absAlpha,dubiousWeights[iSequence]);
                                   }
                                   double range = upper_[iSequence] - lower_[iSequence];
                                   thruThis += range * fabs(alpha);
                                   increaseInThis += badDj * range;
                              }
                         }
                         marker[1-iFlip][0] = CoinMax(marker[1-iFlip][0], numberRemaining);
                         marker[1-iFlip][1] = CoinMin(marker[1-iFlip][1], numberPossiblySwapped);
#ifdef MORE_CAREFUL
                         // If we have done pivots and things look bad set alpha_ 0.0 to force factorization
                         if (sumBadPivots > 1.0e4) {
                              if (handler_->logLevel() > 1)
                                   *handler_ << "maybe forcing re-factorization - sum " << sumBadPivots << " " << factorization_->pivots() << " pivots" << CoinMessageEol;
                              if(factorization_->pivots() > 3) {
                                   badSumPivots = true;
                                   break;
                              }
                         }
#endif
                         swapped[1-iFlip] = numberPossiblySwapped;
                         interesting[1-iFlip] = numberRemaining;
                         // If we stop now this will be increase in objective (I think)
                         double increase = (fabs(dualOut_) - totalThru) * theta_;
                         increase += increaseInObjective;
                         if (theta_ < 0.0)
                              thruThis += fabs(dualOut_); // force using this one
                         if (increaseInObjective < 0.0 && increase < 0.0 && lastSequence >= 0) {
                              // back
                              // We may need to be more careful - we could do by
                              // switch so we always do fine grained?
                              bestPivot = 0.0;
                         } else {
                              // add in
                              totalThru += thruThis;
                              increaseInObjective += increaseInThis;
                         }
                         if (bestPivot < 0.1 * bestEverPivot &&
                                   bestEverPivot > 1.0e-6 &&
                                   (bestPivot < 1.0e-3 || totalThru * 2.0 > fabs(dualOut_))) {
                              // back to previous one
                              sequenceIn_ = lastSequence;
                              // swap regions
                              iFlip = 1 - iFlip;
                              break;
                         } else if (sequenceIn_ == -1 && upperTheta > largeValue_) {
                              if (lastPivot > acceptablePivot) {
                                   // back to previous one
                                   sequenceIn_ = lastSequence;
                                   // swap regions
                                   iFlip = 1 - iFlip;
                              } else {
                                   // can only get here if all pivots too small
                              }
                              break;
                         } else if (totalThru >= fabs(dualOut_)) {
                              modifyCosts = true; // fine grain - we can modify costs
                              break; // no point trying another loop
                         } else {
                              lastSequence = sequenceIn_;
                              if (bestPivot > bestEverPivot)
                                   bestEverPivot = bestPivot;
                              iFlip = 1 - iFlip;
                              modifyCosts = true; // fine grain - we can modify costs
                         }
                    }
                    if (iTry == MAXTRY)
                         iFlip = 1 - iFlip; // flip back
                    break;
               } else {
                    // skip this lot
                    if (bestPivot > 1.0e-3 || bestPivot > bestEverPivot) {
                         bestEverPivot = bestPivot;
                         lastSequence = bestSequence;
                    } else {
                         // keep old swapped
                         CoinMemcpyN(array[iFlip] + swapped[iFlip],
                                     numberColumns_ - swapped[iFlip], array[1-iFlip] + swapped[iFlip]);
                         CoinMemcpyN(indices[iFlip] + swapped[iFlip],
                                     numberColumns_ - swapped[iFlip], indices[1-iFlip] + swapped[iFlip]);
                         marker[1-iFlip][1] = CoinMin(marker[1-iFlip][1], swapped[iFlip]);
                         swapped[1-iFlip] = swapped[iFlip];
                    }
                    increaseInObjective += increaseInThis;
                    iFlip = 1 - iFlip; // swap regions
                    tentativeTheta = 2.0 * upperTheta;
                    totalThru += thruThis;
               }
          }

          // can get here without sequenceIn_ set but with lastSequence
          if (sequenceIn_ < 0 && lastSequence >= 0) {
               // back to previous one
               sequenceIn_ = lastSequence;
               // swap regions
               iFlip = 1 - iFlip;
          }

#define MINIMUMTHETA 1.0e-18
          // Movement should be minimum for anti-degeneracy - unless
          // fixed variable out
          double minimumTheta;
          if (upperOut_ > lowerOut_)
               minimumTheta = MINIMUMTHETA;
          else
               minimumTheta = 0.0;
          if (sequenceIn_ >= 0) {
               // at this stage sequenceIn_ is just pointer into index array
               // flip just so we can use iFlip
               iFlip = 1 - iFlip;
               spare = array[iFlip];
               index = indices[iFlip];
               double oldValue;
               alpha_ = spare[sequenceIn_];
               sequenceIn_ = indices[iFlip][sequenceIn_];
               oldValue = dj_[sequenceIn_];
               theta_ = CoinMax(oldValue / alpha_, 0.0);
               if (theta_ < minimumTheta && fabs(alpha_) < 1.0e5 && 1) {
                    // can't pivot to zero
#if 0
                    if (oldValue - minimumTheta*alpha_ >= -dualTolerance_) {
                         theta_ = minimumTheta;
                    } else if (oldValue - minimumTheta*alpha_ >= -newTolerance) {
                         theta_ = minimumTheta;
                         thisIncrease = true;
                    } else {
                         theta_ = CoinMax((oldValue + newTolerance) / alpha_, 0.0);
                         thisIncrease = true;
                    }
#else
                    theta_ = minimumTheta;
#endif
               }
               // may need to adjust costs so all dual feasible AND pivoted is exactly 0
               //int costOffset = numberRows_+numberColumns_;
               if (modifyCosts && !badSumPivots) {
                    int i;
                    for (i = numberColumns_ - 1; i >= swapped[iFlip]; i--) {
                         int iSequence = index[i];
                         double alpha = spare[i];
                         double value = dj_[iSequence] - theta_ * alpha;

                         // can't be free here

                         if (alpha < 0.0) {
                              //at upper bound
                              if (value > dualTolerance_) {
                                   //thisIncrease = true;
#if CLP_CAN_HAVE_ZERO_OBJ<2
#define MODIFYCOST 2
#endif
#if MODIFYCOST
                                   // modify cost to hit new tolerance
                                   double modification = alpha * theta_ - dj_[iSequence]
                                                         + newTolerance;
                                   if ((specialOptions_&(2048 + 4096 + 16384)) != 0) {
                                        if ((specialOptions_ & 16384) != 0) {
                                             if (fabs(modification) < 1.0e-8)
                                                  modification = 0.0;
                                        } else if ((specialOptions_ & 2048) != 0) {
                                             if (fabs(modification) < 1.0e-10)
                                                  modification = 0.0;
                                        } else {
                                             if (fabs(modification) < 1.0e-12)
                                                  modification = 0.0;
                                        }
                                   }
                                   dj_[iSequence] += modification;
                                   cost_[iSequence] +=  modification;
                                   if (modification)
                                        numberChanged_ ++; // Say changed costs
                                   //cost_[iSequence+costOffset] += modification; // save change
#endif
                              }
                         } else {
                              // at lower bound
                              if (-value > dualTolerance_) {
                                   //thisIncrease = true;
#if MODIFYCOST
                                   // modify cost to hit new tolerance
                                   double modification = alpha * theta_ - dj_[iSequence]
                                                         - newTolerance;
                                   //modification = CoinMax(modification,-dualTolerance_);
                                   //assert (fabs(modification)<1.0e-7);
                                   if ((specialOptions_&(2048 + 4096)) != 0) {
                                        if ((specialOptions_ & 2048) != 0) {
                                             if (fabs(modification) < 1.0e-10)
                                                  modification = 0.0;
                                        } else {
                                             if (fabs(modification) < 1.0e-12)
                                                  modification = 0.0;
                                        }
                                   }
                                   dj_[iSequence] += modification;
                                   cost_[iSequence] +=  modification;
                                   if (modification)
                                        numberChanged_ ++; // Say changed costs
                                   //cost_[iSequence+costOffset] += modification; // save change
#endif
                              }
                         }
                    }
               }
          }
     }

#ifdef MORE_CAREFUL
     // If we have done pivots and things look bad set alpha_ 0.0 to force factorization
     if ((badSumPivots ||
               fabs(theta_ * badFree) > 10.0 * dualTolerance_) && factorization_->pivots()) {
          if (handler_->logLevel() > 1)
               *handler_ << "forcing re-factorization" << CoinMessageEol;
          sequenceIn_ = -1;
     }
#endif
     if (sequenceIn_ >= 0) {
          lowerIn_ = lower_[sequenceIn_];
          upperIn_ = upper_[sequenceIn_];
          valueIn_ = solution_[sequenceIn_];
          dualIn_ = dj_[sequenceIn_];

          if (numberTimesOptimal_) {
               // can we adjust cost back closer to original
               //*** add coding
          }
#if MODIFYCOST>1
          // modify cost to hit zero exactly
          // so (dualIn_+modification)==theta_*alpha_
          double modification = theta_ * alpha_ - dualIn_;
          // But should not move objective too much ??
#define DONT_MOVE_OBJECTIVE
#ifdef DONT_MOVE_OBJECTIVE
          double moveObjective = fabs(modification * solution_[sequenceIn_]);
          double smallMove = CoinMax(fabs(objectiveValue_), 1.0e-3);
          if (moveObjective > smallMove) {
               if (handler_->logLevel() > 1)
                    printf("would move objective by %g - original mod %g sol value %g\n", moveObjective,
                           modification, solution_[sequenceIn_]);
               modification *= smallMove / moveObjective;
          }
#endif
          if (badSumPivots)
               modification = 0.0;
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
          dj_[sequenceIn_] = dualIn_;
          cost_[sequenceIn_] += modification;
          if (modification)
               numberChanged_ ++; // Say changed costs
          //int costOffset = numberRows_+numberColumns_;
          //cost_[sequenceIn_+costOffset] += modification; // save change
          //assert (fabs(modification)<1.0e-6);
#ifdef CLP_DEBUG
          if ((handler_->logLevel() & 32) && fabs(modification) > 1.0e-15)
               printf("exact %d new cost %g, change %g\n", sequenceIn_,
                      cost_[sequenceIn_], modification);
#endif
#endif

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
          bestPossible = 0.0;
          alpha_ = 0.0;
     }
     //if (thisIncrease)
     //dualTolerance_+= 1.0e-6*dblParam_[ClpDualTolerance];

     // clear arrays

     for (i = 0; i < 2; i++) {
          CoinZeroN(array[i], marker[i][0]);
          CoinZeroN(array[i] + marker[i][1], numberColumns_ - marker[i][1]);
     }
     return bestPossible;
}
#ifdef CLP_ALL_ONE_FILE
#undef MAXTRY
#endif
/* Checks if tentative optimal actually means unbounded
   Returns -3 if not, 2 if is unbounded */
int
ClpSimplexDual::checkUnbounded(CoinIndexedVector * ray,
                               CoinIndexedVector * spare,
                               double changeCost)
{
     int status = 2; // say unbounded
     factorization_->updateColumn(spare, ray);
     // get reduced cost
     int i;
     int number = ray->getNumElements();
     int * index = ray->getIndices();
     double * array = ray->denseVector();
     for (i = 0; i < number; i++) {
          int iRow = index[i];
          int iPivot = pivotVariable_[iRow];
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
#ifdef CLP_DEBUG
          printf("can't decide on up or down\n");
#endif
          way = 0.0;
          status = -3;
     }
     double movement = 1.0e10 * way; // some largish number
     double zeroTolerance = 1.0e-14 * dualBound_;
     for (i = 0; i < number; i++) {
          int iRow = index[i];
          int iPivot = pivotVariable_[iRow];
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
          for (i = 0; i < number; i++) {
               int iRow = index[i];
               int iPivot = pivotVariable_[iRow];
               double arrayValue = array[iRow];
               if (iPivot < numberColumns_ && fabs(arrayValue) >= zeroTolerance)
                    ray_[iPivot] = way * array[iRow];
          }
     }
     ray->clear();
     return status;
}
//static int count_alpha=0;
/* Checks if finished.  Updates status */
void
ClpSimplexDual::statusOfProblemInDual(int & lastCleaned, int type,
                                      double * givenDuals, ClpDataSave & saveData,
                                      int ifValuesPass)
{
#ifdef CLP_INVESTIGATE_SERIAL
     if (z_thinks > 0 && z_thinks < 2)
          z_thinks += 2;
#endif
     bool arraysNotCreated = (type==0);
     // If lots of iterations then adjust costs if large ones
     if (numberIterations_ > 4 * (numberRows_ + numberColumns_) && objectiveScale_ == 1.0) {
          double largest = 0.0;
          for (int i = 0; i < numberRows_; i++) {
               int iColumn = pivotVariable_[i];
               largest = CoinMax(largest, fabs(cost_[iColumn]));
          }
          if (largest > 1.0e6) {
               objectiveScale_ = 1.0e6 / largest;
               for (int i = 0; i < numberRows_ + numberColumns_; i++)
                    cost_[i] *= objectiveScale_;
          }
     }
     int numberPivots = factorization_->pivots();
     double realDualInfeasibilities = 0.0;
     if (type == 2) {
          if (alphaAccuracy_ != -1.0)
               alphaAccuracy_ = -2.0;
          // trouble - restore solution
          CoinMemcpyN(saveStatus_, numberColumns_ + numberRows_, status_);
          CoinMemcpyN(savedSolution_ + numberColumns_ ,
                      numberRows_, rowActivityWork_);
          CoinMemcpyN(savedSolution_ ,
                      numberColumns_, columnActivityWork_);
          // restore extra stuff
          int dummy;
          matrix_->generalExpanded(this, 6, dummy);
          forceFactorization_ = 1; // a bit drastic but ..
          changeMade_++; // say something changed
          // get correct bounds on all variables
          resetFakeBounds(0);
     }
     int tentativeStatus = problemStatus_;
     double changeCost;
     bool unflagVariables = true;
     bool weightsSaved = false;
     bool weightsSaved2 = numberIterations_ && !numberPrimalInfeasibilities_;
     int dontFactorizePivots = dontFactorizePivots_;
     if (type == 3) {
          type = 1;
          dontFactorizePivots = 1;
     }
     if (alphaAccuracy_ < 0.0 || !numberPivots || alphaAccuracy_ > 1.0e4 || numberPivots > 20) {
          if (problemStatus_ > -3 || numberPivots > dontFactorizePivots) {
               // factorize
               // later on we will need to recover from singularities
               // also we could skip if first time
               // save dual weights
               dualRowPivot_->saveWeights(this, 1);
               weightsSaved = true;
               if (type) {
                    // is factorization okay?
                    if (internalFactorize(1)) {
                         // no - restore previous basis
                         unflagVariables = false;
                         assert (type == 1);
                         changeMade_++; // say something changed
                         // Keep any flagged variables
                         int i;
                         for (i = 0; i < numberRows_ + numberColumns_; i++) {
                              if (flagged(i))
                                   saveStatus_[i] |= 64; //say flagged
                         }
                         CoinMemcpyN(saveStatus_, numberColumns_ + numberRows_, status_);
                         CoinMemcpyN(savedSolution_ + numberColumns_ ,
                                     numberRows_, rowActivityWork_);
                         CoinMemcpyN(savedSolution_ ,
                                     numberColumns_, columnActivityWork_);
                         // restore extra stuff
                         int dummy;
                         matrix_->generalExpanded(this, 6, dummy);
                         // get correct bounds on all variables
                         resetFakeBounds(1);
                         // need to reject something
                         char x = isColumn(sequenceOut_) ? 'C' : 'R';
                         handler_->message(CLP_SIMPLEX_FLAG, messages_)
                                   << x << sequenceWithin(sequenceOut_)
                                   << CoinMessageEol;
#ifdef COIN_DEVELOP
                         printf("flag d\n");
#endif
                         setFlagged(sequenceOut_);
                         progress_.clearBadTimes();

                         // Go to safe
                         factorization_->pivotTolerance(0.99);
                         forceFactorization_ = 1; // a bit drastic but ..
                         type = 2;
                         //assert (internalFactorize(1)==0);
                         if (internalFactorize(1)) {
                              CoinMemcpyN(saveStatus_, numberColumns_ + numberRows_, status_);
                              CoinMemcpyN(savedSolution_ + numberColumns_ ,
                                          numberRows_, rowActivityWork_);
                              CoinMemcpyN(savedSolution_ ,
                                          numberColumns_, columnActivityWork_);
                              // restore extra stuff
                              int dummy;
                              matrix_->generalExpanded(this, 6, dummy);
                              // debug
                              int returnCode = internalFactorize(1);
                              while (returnCode) {
                                   // ouch
                                   // switch off dense
                                   int saveDense = factorization_->denseThreshold();
                                   factorization_->setDenseThreshold(0);
                                   // Go to safe
                                   factorization_->pivotTolerance(0.99);
                                   // make sure will do safe factorization
                                   pivotVariable_[0] = -1;
                                   returnCode = internalFactorize(2);
                                   factorization_->setDenseThreshold(saveDense);
                              }
                              // get correct bounds on all variables
                              resetFakeBounds(1);
                         }
                    }
               }
               if (problemStatus_ != -4 || numberPivots > 10)
                    problemStatus_ = -3;
          }
     } else {
          //printf("testing with accuracy of %g and status of %d\n",alphaAccuracy_,problemStatus_);
          //count_alpha++;
          //if ((count_alpha%5000)==0)
          //printf("count alpha %d\n",count_alpha);
     }
     // at this stage status is -3 or -4 if looks infeasible
     // get primal and dual solutions
#if 0
     {
          int numberTotal = numberRows_ + numberColumns_;
          double * saveSol = CoinCopyOfArray(solution_, numberTotal);
          double * saveDj = CoinCopyOfArray(dj_, numberTotal);
          double tolerance = type ? 1.0e-4 : 1.0e-8;
          // always if values pass
          double saveObj = objectiveValue_;
          double sumPrimal = sumPrimalInfeasibilities_;
          int numberPrimal = numberPrimalInfeasibilities_;
          double sumDual = sumDualInfeasibilities_;
          int numberDual = numberDualInfeasibilities_;
          gutsOfSolution(givenDuals, NULL);
          int j;
          double largestPrimal = tolerance;
          int iPrimal = -1;
          for (j = 0; j < numberTotal; j++) {
               double difference = solution_[j] - saveSol[j];
               if (fabs(difference) > largestPrimal) {
                    iPrimal = j;
                    largestPrimal = fabs(difference);
               }
          }
          double largestDual = tolerance;
          int iDual = -1;
          for (j = 0; j < numberTotal; j++) {
               double difference = dj_[j] - saveDj[j];
               if (fabs(difference) > largestDual && upper_[j] > lower_[j]) {
                    iDual = j;
                    largestDual = fabs(difference);
               }
          }
          if (!type) {
               if (fabs(saveObj - objectiveValue_) > 1.0e-5 ||
                         numberPrimal != numberPrimalInfeasibilities_ || numberPrimal != 1 ||
                         fabs(sumPrimal - sumPrimalInfeasibilities_) > 1.0e-5 || iPrimal >= 0 ||
                         numberDual != numberDualInfeasibilities_ || numberDual != 0 ||
                         fabs(sumDual - sumDualInfeasibilities_) > 1.0e-5 || iDual >= 0)
                    printf("type %d its %d pivots %d primal n(%d,%d) s(%g,%g) diff(%g,%d) dual n(%d,%d) s(%g,%g) diff(%g,%d) obj(%g,%g)\n",
                           type, numberIterations_, numberPivots,
                           numberPrimal, numberPrimalInfeasibilities_, sumPrimal, sumPrimalInfeasibilities_,
                           largestPrimal, iPrimal,
                           numberDual, numberDualInfeasibilities_, sumDual, sumDualInfeasibilities_,
                           largestDual, iDual,
                           saveObj, objectiveValue_);
          } else {
               if (fabs(saveObj - objectiveValue_) > 1.0e-5 ||
                         numberPrimalInfeasibilities_ || iPrimal >= 0 ||
                         numberDualInfeasibilities_ || iDual >= 0)
                    printf("type %d its %d pivots %d primal n(%d,%d) s(%g,%g) diff(%g,%d) dual n(%d,%d) s(%g,%g) diff(%g,%d) obj(%g,%g)\n",
                           type, numberIterations_, numberPivots,
                           numberPrimal, numberPrimalInfeasibilities_, sumPrimal, sumPrimalInfeasibilities_,
                           largestPrimal, iPrimal,
                           numberDual, numberDualInfeasibilities_, sumDual, sumDualInfeasibilities_,
                           largestDual, iDual,
                           saveObj, objectiveValue_);
          }
          delete [] saveSol;
          delete [] saveDj;
     }
#else
     if (type || ifValuesPass)
          gutsOfSolution(givenDuals, NULL);
#endif
     // If bad accuracy treat as singular
     if ((largestPrimalError_ > 1.0e15 || largestDualError_ > 1.0e15) && numberIterations_) {
          // restore previous basis
          unflagVariables = false;
          changeMade_++; // say something changed
          // Keep any flagged variables
          int i;
          for (i = 0; i < numberRows_ + numberColumns_; i++) {
               if (flagged(i))
                    saveStatus_[i] |= 64; //say flagged
          }
          CoinMemcpyN(saveStatus_, numberColumns_ + numberRows_, status_);
          CoinMemcpyN(savedSolution_ + numberColumns_ ,
                      numberRows_, rowActivityWork_);
          CoinMemcpyN(savedSolution_ ,
                      numberColumns_, columnActivityWork_);
          // restore extra stuff
          int dummy;
          matrix_->generalExpanded(this, 6, dummy);
          // get correct bounds on all variables
          resetFakeBounds(1);
          // need to reject something
          char x = isColumn(sequenceOut_) ? 'C' : 'R';
          handler_->message(CLP_SIMPLEX_FLAG, messages_)
                    << x << sequenceWithin(sequenceOut_)
                    << CoinMessageEol;
#ifdef COIN_DEVELOP
          printf("flag e\n");
#endif
          setFlagged(sequenceOut_);
          progress_.clearBadTimes();

          // Go to safer
          double newTolerance = CoinMin(1.1 * factorization_->pivotTolerance(), 0.99);
          factorization_->pivotTolerance(newTolerance);
          forceFactorization_ = 1; // a bit drastic but ..
          if (alphaAccuracy_ != -1.0)
               alphaAccuracy_ = -2.0;
          type = 2;
          //assert (internalFactorize(1)==0);
          if (internalFactorize(1)) {
               CoinMemcpyN(saveStatus_, numberColumns_ + numberRows_, status_);
               CoinMemcpyN(savedSolution_ + numberColumns_ ,
                           numberRows_, rowActivityWork_);
               CoinMemcpyN(savedSolution_ ,
                           numberColumns_, columnActivityWork_);
               // restore extra stuff
               int dummy;
               matrix_->generalExpanded(this, 6, dummy);
               // debug
               int returnCode = internalFactorize(1);
               while (returnCode) {
                    // ouch
                    // switch off dense
                    int saveDense = factorization_->denseThreshold();
                    factorization_->setDenseThreshold(0);
                    // Go to safe
                    factorization_->pivotTolerance(0.99);
                    // make sure will do safe factorization
                    pivotVariable_[0] = -1;
                    returnCode = internalFactorize(2);
                    factorization_->setDenseThreshold(saveDense);
               }
               // get correct bounds on all variables
               resetFakeBounds(1);
          }
          // get primal and dual solutions
          gutsOfSolution(givenDuals, NULL);
     } else if (goodAccuracy()) {
          // Can reduce tolerance
          double newTolerance = CoinMax(0.99 * factorization_->pivotTolerance(), saveData.pivotTolerance_);
          factorization_->pivotTolerance(newTolerance);
     }
     bestObjectiveValue_ = CoinMax(bestObjectiveValue_,
                                   objectiveValue_ - bestPossibleImprovement_);
     bool reallyBadProblems = false;
     // Double check infeasibility if no action
     if (progress_.lastIterationNumber(0) == numberIterations_) {
          if (dualRowPivot_->looksOptimal()) {
               numberPrimalInfeasibilities_ = 0;
               sumPrimalInfeasibilities_ = 0.0;
          }
#if 1
     } else {
          double thisObj = objectiveValue_ - bestPossibleImprovement_;
#ifdef CLP_INVESTIGATE
          assert (bestPossibleImprovement_ > -1000.0 && objectiveValue_ > -1.0e100);
          if (bestPossibleImprovement_)
               printf("obj %g add in %g -> %g\n", objectiveValue_, bestPossibleImprovement_,
                      thisObj);
#endif
          double lastObj = progress_.lastObjective(0);
#ifndef NDEBUG
#ifdef COIN_DEVELOP
          resetFakeBounds(-1);
#endif
#endif
#ifdef CLP_REPORT_PROGRESS
          ixxxxxx++;
          if (ixxxxxx >= ixxyyyy - 4 && ixxxxxx <= ixxyyyy) {
               char temp[20];
               sprintf(temp, "sol%d.out", ixxxxxx);
               printf("sol%d.out\n", ixxxxxx);
               FILE * fp = fopen(temp, "w");
               int nTotal = numberRows_ + numberColumns_;
               for (int i = 0; i < nTotal; i++)
                    fprintf(fp, "%d %d %g %g %g %g %g\n",
                            i, status_[i], lower_[i], solution_[i], upper_[i], cost_[i], dj_[i]);
               fclose(fp);
          }
#endif
          if(!ifValuesPass && firstFree_ < 0) {
               double testTol = 5.0e-3;
               if (progress_.timesFlagged() > 10) {
                    testTol *= pow(2.0, progress_.timesFlagged() - 8);
               } else if (progress_.timesFlagged() > 5) {
                    testTol *= 5.0;
               }
               if (lastObj > thisObj +
                         testTol*(fabs(thisObj) + fabs(lastObj)) + testTol) {
                    int maxFactor = factorization_->maximumPivots();
                    if ((specialOptions_ & 1048576) == 0) {
                         if (progress_.timesFlagged() > 10)
                              progress_.incrementReallyBadTimes();
                         if (maxFactor > 10 - 9) {
#ifdef COIN_DEVELOP
                              printf("lastobj %g thisobj %g\n", lastObj, thisObj);
#endif
                              //if (forceFactorization_<0)
                              //forceFactorization_= maxFactor;
                              //forceFactorization_ = CoinMax(1,(forceFactorization_>>1));
                              if ((progressFlag_ & 4) == 0 && lastObj < thisObj + 1.0e4 &&
                                        largestPrimalError_ < 1.0e2) {
                                   // Just save costs
                                   // save extra copy of cost_
                                   int nTotal = numberRows_ + numberColumns_;
                                   double * temp = new double [2*nTotal];
                                   memcpy(temp, cost_, nTotal * sizeof(double));
                                   memcpy(temp + nTotal, cost_, nTotal * sizeof(double));
                                   delete [] cost_;
                                   cost_ = temp;
                                   objectiveWork_ = cost_;
                                   rowObjectiveWork_ = cost_ + numberColumns_;
                                   progressFlag_ |= 4;
                              } else {
                                   forceFactorization_ = 1;
#ifdef COIN_DEVELOP
                                   printf("Reducing factorization frequency - bad backwards\n");
#endif
#if 1
                                   unflagVariables = false;
                                   changeMade_++; // say something changed
                                   int nTotal = numberRows_ + numberColumns_;
                                   CoinMemcpyN(saveStatus_, nTotal, status_);
                                   CoinMemcpyN(savedSolution_ + numberColumns_ ,
                                               numberRows_, rowActivityWork_);
                                   CoinMemcpyN(savedSolution_ ,
                                               numberColumns_, columnActivityWork_);
                                   if ((progressFlag_ & 4) == 0) {
                                        // save extra copy of cost_
                                        double * temp = new double [2*nTotal];
                                        memcpy(temp, cost_, nTotal * sizeof(double));
                                        memcpy(temp + nTotal, cost_, nTotal * sizeof(double));
                                        delete [] cost_;
                                        cost_ = temp;
                                        objectiveWork_ = cost_;
                                        rowObjectiveWork_ = cost_ + numberColumns_;
                                        progressFlag_ |= 4;
                                   } else {
                                        memcpy(cost_, cost_ + nTotal, nTotal * sizeof(double));
                                   }
                                   // restore extra stuff
                                   int dummy;
                                   matrix_->generalExpanded(this, 6, dummy);
                                   double pivotTolerance = factorization_->pivotTolerance();
                                   if(pivotTolerance < 0.2)
                                        factorization_->pivotTolerance(0.2);
                                   else if(progress_.timesFlagged() > 2)
                                        factorization_->pivotTolerance(CoinMin(pivotTolerance * 1.1, 0.99));
                                   if (alphaAccuracy_ != -1.0)
                                        alphaAccuracy_ = -2.0;
                                   if (internalFactorize(1)) {
                                        CoinMemcpyN(saveStatus_, numberColumns_ + numberRows_, status_);
                                        CoinMemcpyN(savedSolution_ + numberColumns_ ,
                                                    numberRows_, rowActivityWork_);
                                        CoinMemcpyN(savedSolution_ ,
                                                    numberColumns_, columnActivityWork_);
                                        // restore extra stuff
                                        int dummy;
                                        matrix_->generalExpanded(this, 6, dummy);
                                        // debug
                                        int returnCode = internalFactorize(1);
                                        while (returnCode) {
                                             // ouch
                                             // switch off dense
                                             int saveDense = factorization_->denseThreshold();
                                             factorization_->setDenseThreshold(0);
                                             // Go to safe
                                             factorization_->pivotTolerance(0.99);
                                             // make sure will do safe factorization
                                             pivotVariable_[0] = -1;
                                             returnCode = internalFactorize(2);
                                             factorization_->setDenseThreshold(saveDense);
                                        }
                                   }
                                   resetFakeBounds(0);
                                   type = 2; // so will restore weights
                                   // get primal and dual solutions
                                   gutsOfSolution(givenDuals, NULL);
                                   if (numberPivots < 2) {
                                        // need to reject something
                                        char x = isColumn(sequenceOut_) ? 'C' : 'R';
                                        handler_->message(CLP_SIMPLEX_FLAG, messages_)
                                                  << x << sequenceWithin(sequenceOut_)
                                                  << CoinMessageEol;
#ifdef COIN_DEVELOP
                                        printf("flag d\n");
#endif
                                        setFlagged(sequenceOut_);
                                        progress_.clearBadTimes();
                                        progress_.incrementTimesFlagged();
                                   }
                                   if (numberPivots < 10)
                                        reallyBadProblems = true;
#ifdef COIN_DEVELOP
                                   printf("obj now %g\n", objectiveValue_);
#endif
                                   progress_.modifyObjective(objectiveValue_
                                                             - bestPossibleImprovement_);
#endif
                              }
                         }
                    } else {
                         // in fast dual give up
#ifdef COIN_DEVELOP
                         printf("In fast dual?\n");
#endif
                         problemStatus_ = 3;
                    }
               } else if (lastObj < thisObj - 1.0e-5 * CoinMax(fabs(thisObj), fabs(lastObj)) - 1.0e-3) {
                    numberTimesOptimal_ = 0;
               }
          }
#endif
     }
     // Up tolerance if looks a bit odd
     if (numberIterations_ > CoinMax(1000, numberRows_ >> 4) && (specialOptions_ & 64) != 0) {
          if (sumPrimalInfeasibilities_ && sumPrimalInfeasibilities_ < 1.0e5) {
               int backIteration = progress_.lastIterationNumber(CLP_PROGRESS - 1);
               if (backIteration > 0 && numberIterations_ - backIteration < 9 * CLP_PROGRESS) {
                    if (factorization_->pivotTolerance() < 0.9) {
                         // up tolerance
                         factorization_->pivotTolerance(CoinMin(factorization_->pivotTolerance() * 1.05 + 0.02, 0.91));
                         //printf("tol now %g\n",factorization_->pivotTolerance());
                         progress_.clearIterationNumbers();
                    }
               }
          }
     }
     // Check if looping
     int loop;
     if (!givenDuals && type != 2)
          loop = progress_.looping();
     else
          loop = -1;
     if (progress_.reallyBadTimes() > 10) {
          problemStatus_ = 10; // instead - try other algorithm
#if COIN_DEVELOP>2
          printf("returning at %d\n", __LINE__);
#endif
     }
     int situationChanged = 0;
     if (loop >= 0) {
          problemStatus_ = loop; //exit if in loop
          if (!problemStatus_) {
               // declaring victory
               numberPrimalInfeasibilities_ = 0;
               sumPrimalInfeasibilities_ = 0.0;
          } else {
               problemStatus_ = 10; // instead - try other algorithm
#if COIN_DEVELOP>2
               printf("returning at %d\n", __LINE__);
#endif
          }
          return;
     } else if (loop < -1) {
          // something may have changed
          gutsOfSolution(NULL, NULL);
          situationChanged = 1;
     }
     // really for free variables in
     if((progressFlag_ & 2) != 0) {
          situationChanged = 2;
     }
     progressFlag_ &= (~3); //reset progress flag
     if ((progressFlag_ & 4) != 0) {
          // save copy of cost_
          int nTotal = numberRows_ + numberColumns_;
          memcpy(cost_ + nTotal, cost_, nTotal * sizeof(double));
     }
     /*if (!numberIterations_&&sumDualInfeasibilities_)
       printf("OBJ %g sumPinf %g sumDinf %g\n",
        objectiveValue(),sumPrimalInfeasibilities_,
        sumDualInfeasibilities_);*/
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
#if 0
     printf("IT %d %g %g(%d) %g(%d)\n",
            numberIterations_, objectiveValue(),
            sumPrimalInfeasibilities_, numberPrimalInfeasibilities_,
            sumDualInfeasibilities_, numberDualInfeasibilities_);
#endif
     double approximateObjective = objectiveValue_;
#ifdef CLP_REPORT_PROGRESS
     if (ixxxxxx >= ixxyyyy - 4 && ixxxxxx <= ixxyyyy) {
          char temp[20];
          sprintf(temp, "x_sol%d.out", ixxxxxx);
          FILE * fp = fopen(temp, "w");
          int nTotal = numberRows_ + numberColumns_;
          for (int i = 0; i < nTotal; i++)
               fprintf(fp, "%d %d %g %g %g %g %g\n",
                       i, status_[i], lower_[i], solution_[i], upper_[i], cost_[i], dj_[i]);
          fclose(fp);
          if (ixxxxxx == ixxyyyy)
               exit(6);
     }
#endif
     realDualInfeasibilities = sumDualInfeasibilities_;
     double saveTolerance = dualTolerance_;
     // If we need to carry on cleaning variables
     if (!numberPrimalInfeasibilities_ && (specialOptions_ & 1024) != 0 && CLEAN_FIXED) {
          for (int iRow = 0; iRow < numberRows_; iRow++) {
               int iPivot = pivotVariable_[iRow];
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
               numberDualInfeasibilities_)
          problemStatus_ = 10;
     // dual bound coming in
     //double saveDualBound = dualBound_;
     bool needCleanFake = false;
     while (problemStatus_ <= -3) {
          int cleanDuals = 0;
          if (situationChanged != 0)
               cleanDuals = 1;
          int numberChangedBounds = 0;
          int doOriginalTolerance = 0;
          if ( lastCleaned == numberIterations_)
               doOriginalTolerance = 1;
          // check optimal
          // give code benefit of doubt
          if (sumOfRelaxedDualInfeasibilities_ == 0.0 &&
                    sumOfRelaxedPrimalInfeasibilities_ == 0.0) {
               // say optimal (with these bounds etc)
               numberDualInfeasibilities_ = 0;
               sumDualInfeasibilities_ = 0.0;
               numberPrimalInfeasibilities_ = 0;
               sumPrimalInfeasibilities_ = 0.0;
          }
          //if (dualFeasible()||problemStatus_==-4||(primalFeasible()&&!numberDualInfeasibilitiesWithoutFree_)) {
          if (dualFeasible() || problemStatus_ == -4) {
               progress_.modifyObjective(objectiveValue_
                                         - bestPossibleImprovement_);
#ifdef COIN_DEVELOP
               if (sumDualInfeasibilities_ || bestPossibleImprovement_)
                    printf("improve %g dualinf %g -> %g\n",
                           bestPossibleImprovement_, sumDualInfeasibilities_,
                           sumDualInfeasibilities_ * dualBound_);
#endif
               // see if cutoff reached
               double limit = 0.0;
               getDblParam(ClpDualObjectiveLimit, limit);
#if 0
               if(fabs(limit) < 1.0e30 && objectiveValue()*optimizationDirection_ >
                         limit + 1.0e-7 + 1.0e-8 * fabs(limit) && !numberAtFakeBound()) {
                    //looks infeasible on objective
                    if (perturbation_ == 101) {
                         cleanDuals = 1;
                         // Save costs
                         int numberTotal = numberRows_ + numberColumns_;
                         double * saveCost = CoinCopyOfArray(cost_, numberTotal);
                         // make sure fake bounds are back
                         changeBounds(1, NULL, changeCost);
                         createRim4(false);
                         // make sure duals are current
                         computeDuals(givenDuals);
                         checkDualSolution();
                         if(objectiveValue()*optimizationDirection_ >
                                   limit + 1.0e-7 + 1.0e-8 * fabs(limit) && !numberDualInfeasibilities_) {
                              perturbation_ = 102; // stop any perturbations
                              printf("cutoff test succeeded\n");
                         } else {
                              printf("cutoff test failed\n");
                              // put back
                              memcpy(cost_, saveCost, numberTotal * sizeof(double));
                              // make sure duals are current
                              computeDuals(givenDuals);
                              checkDualSolution();
                              progress_.modifyObjective(-COIN_DBL_MAX);
                              problemStatus_ = -1;
                         }
                         delete [] saveCost;
                    }
               }
#endif
               if (primalFeasible() && !givenDuals) {
                    // may be optimal - or may be bounds are wrong
                    handler_->message(CLP_DUAL_BOUNDS, messages_)
                              << dualBound_
                              << CoinMessageEol;
                    // save solution in case unbounded
                    double * saveColumnSolution = NULL;
                    double * saveRowSolution = NULL;
                    bool inCbc = (specialOptions_ & (0x01000000 | 16384)) != 0;
                    if (!inCbc) {
                         saveColumnSolution = CoinCopyOfArray(columnActivityWork_, numberColumns_);
                         saveRowSolution = CoinCopyOfArray(rowActivityWork_, numberRows_);
                    }
                    numberChangedBounds = changeBounds(0, rowArray_[3], changeCost);
                    if (numberChangedBounds <= 0 && !numberDualInfeasibilities_) {
                         //looks optimal - do we need to reset tolerance
                         if (perturbation_ == 101) {
                              perturbation_ = 102; // stop any perturbations
                              cleanDuals = 1;
                              // make sure fake bounds are back
                              //computeObjectiveValue();
                              changeBounds(1, NULL, changeCost);
                              //computeObjectiveValue();
                              createRim4(false);
                              // make sure duals are current
                              computeDuals(givenDuals);
                              checkDualSolution();
                              progress_.modifyObjective(-COIN_DBL_MAX);
#define DUAL_TRY_FASTER
#ifdef DUAL_TRY_FASTER
                              if (numberDualInfeasibilities_) {
#endif
                                   numberChanged_ = 1; // force something to happen
                                   lastCleaned = numberIterations_ - 1;
#ifdef DUAL_TRY_FASTER
                              } else {
                                   //double value = objectiveValue_;
                                   computeObjectiveValue(true);
                                   //printf("old %g new %g\n",value,objectiveValue_);
                                   //numberChanged_=1;
                              }
#endif
                         }
                         if (lastCleaned < numberIterations_ && numberTimesOptimal_ < 4 &&
                                   (numberChanged_ || (specialOptions_ & 4096) == 0)) {
#if CLP_CAN_HAVE_ZERO_OBJ
			   if ((specialOptions_&2097152)==0) {
#endif
                              doOriginalTolerance = 2;
                              numberTimesOptimal_++;
                              changeMade_++; // say something changed
                              if (numberTimesOptimal_ == 1) {
                                   dualTolerance_ = dblParam_[ClpDualTolerance];
                              } else {
                                   if (numberTimesOptimal_ == 2) {
                                        // better to have small tolerance even if slower
                                        factorization_->zeroTolerance(CoinMin(factorization_->zeroTolerance(), 1.0e-15));
                                   }
                                   dualTolerance_ = dblParam_[ClpDualTolerance];
                                   dualTolerance_ *= pow(2.0, numberTimesOptimal_ - 1);
                              }
                              cleanDuals = 2; // If nothing changed optimal else primal
#if CLP_CAN_HAVE_ZERO_OBJ
			   } else {
			     // no cost - skip checks
			     problemStatus_=0;
			   }
#endif
                         } else {
                              problemStatus_ = 0; // optimal
                              if (lastCleaned < numberIterations_ && numberChanged_) {
                                   handler_->message(CLP_SIMPLEX_GIVINGUP, messages_)
                                             << CoinMessageEol;
                              }
                         }
                    } else {
                         cleanDuals = 1;
                         if (doOriginalTolerance == 1) {
                              // check unbounded
                              // find a variable with bad dj
                              int iSequence;
                              int iChosen = -1;
                              if (!inCbc) {
                                   double largest = 100.0 * primalTolerance_;
                                   for (iSequence = 0; iSequence < numberRows_ + numberColumns_;
                                             iSequence++) {
                                        double djValue = dj_[iSequence];
                                        double originalLo = originalLower(iSequence);
                                        double originalUp = originalUpper(iSequence);
                                        if (fabs(djValue) > fabs(largest)) {
                                             if (getStatus(iSequence) != basic) {
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
                                   int iSave = sequenceIn_;
                                   sequenceIn_ = iChosen;
                                   unpack(rowArray_[1]);
                                   sequenceIn_ = iSave;
                                   // if dual infeasibilities then must be free vector so add in dual
                                   if (numberDualInfeasibilities_) {
                                        if (fabs(changeCost) > 1.0e-5)
					  COIN_DETAIL_PRINT(printf("Odd free/unbounded combo\n"));
                                        changeCost += cost_[iChosen];
                                   }
                                   problemStatus_ = checkUnbounded(rowArray_[1], rowArray_[0],
                                                                   changeCost);
                                   rowArray_[1]->clear();
                              } else {
                                   problemStatus_ = -3;
                              }
                              if (problemStatus_ == 2 && perturbation_ == 101) {
                                   perturbation_ = 102; // stop any perturbations
                                   cleanDuals = 1;
                                   createRim4(false);
                                   progress_.modifyObjective(-COIN_DBL_MAX);
                                   problemStatus_ = -1;
                              }
                              if (problemStatus_ == 2) {
                                   // it is unbounded - restore solution
                                   // but first add in changes to non-basic
                                   int iColumn;
                                   double * original = columnArray_[0]->denseVector();
                                   for (iColumn = 0; iColumn < numberColumns_; iColumn++) {
                                        if(getColumnStatus(iColumn) != basic)
                                             ray_[iColumn] +=
                                                  saveColumnSolution[iColumn] - original[iColumn];
                                        columnActivityWork_[iColumn] = original[iColumn];
                                   }
                                   CoinMemcpyN(saveRowSolution, numberRows_,
                                               rowActivityWork_);
                              }
                         } else {
                              doOriginalTolerance = 2;
                              rowArray_[0]->clear();
                         }
                    }
                    delete [] saveColumnSolution;
                    delete [] saveRowSolution;
               }
               if (problemStatus_ == -4 || problemStatus_ == -5) {
                    // may be infeasible - or may be bounds are wrong
                    numberChangedBounds = changeBounds(0, NULL, changeCost);
                    needCleanFake = true;
                    /* Should this be here as makes no difference to being feasible.
                       But seems to make a difference to run times. */
                    if (perturbation_ == 101 && 0) {
                         perturbation_ = 102; // stop any perturbations
                         cleanDuals = 1;
                         numberChangedBounds = 1;
                         // make sure fake bounds are back
                         changeBounds(1, NULL, changeCost);
                         needCleanFake = true;
                         createRim4(false);
                         progress_.modifyObjective(-COIN_DBL_MAX);
                    }
                    if ((numberChangedBounds <= 0 || dualBound_ > 1.0e20 ||
                              (largestPrimalError_ > 1.0 && dualBound_ > 1.0e17)) &&
                              (numberPivots < 4 || sumPrimalInfeasibilities_ > 1.0e-6)) {
                         problemStatus_ = 1; // infeasible
                         if (perturbation_ == 101) {
                              perturbation_ = 102; // stop any perturbations
                              //cleanDuals=1;
                              //numberChangedBounds=1;
                              //createRim4(false);
                         }
                    } else {
                         problemStatus_ = -1; //iterate
                         cleanDuals = 1;
                         if (numberChangedBounds <= 0)
                              doOriginalTolerance = 2;
                         // and delete ray which has been created
                         delete [] ray_;
                         ray_ = NULL;
                    }

               }
          } else {
               cleanDuals = 1;
          }
          if (problemStatus_ < 0) {
               if (doOriginalTolerance == 2) {
                    // put back original tolerance
                    lastCleaned = numberIterations_;
                    numberChanged_ = 0; // Number of variables with changed costs
                    handler_->message(CLP_DUAL_ORIGINAL, messages_)
                              << CoinMessageEol;
                    perturbation_ = 102; // stop any perturbations
#if 0
                    double * xcost = new double[numberRows_+numberColumns_];
                    double * xlower = new double[numberRows_+numberColumns_];
                    double * xupper = new double[numberRows_+numberColumns_];
                    double * xdj = new double[numberRows_+numberColumns_];
                    double * xsolution = new double[numberRows_+numberColumns_];
                    CoinMemcpyN(cost_, (numberRows_ + numberColumns_), xcost);
                    CoinMemcpyN(lower_, (numberRows_ + numberColumns_), xlower);
                    CoinMemcpyN(upper_, (numberRows_ + numberColumns_), xupper);
                    CoinMemcpyN(dj_, (numberRows_ + numberColumns_), xdj);
                    CoinMemcpyN(solution_, (numberRows_ + numberColumns_), xsolution);
#endif
                    createRim4(false);
                    progress_.modifyObjective(-COIN_DBL_MAX);
                    // make sure duals are current
                    computeDuals(givenDuals);
                    checkDualSolution();
#if 0
                    int i;
                    for (i = 0; i < numberRows_ + numberColumns_; i++) {
                         if (cost_[i] != xcost[i])
                              printf("** %d old cost %g new %g sol %g\n",
                                     i, xcost[i], cost_[i], solution_[i]);
                         if (lower_[i] != xlower[i])
                              printf("** %d old lower %g new %g sol %g\n",
                                     i, xlower[i], lower_[i], solution_[i]);
                         if (upper_[i] != xupper[i])
                              printf("** %d old upper %g new %g sol %g\n",
                                     i, xupper[i], upper_[i], solution_[i]);
                         if (dj_[i] != xdj[i])
                              printf("** %d old dj %g new %g sol %g\n",
                                     i, xdj[i], dj_[i], solution_[i]);
                         if (solution_[i] != xsolution[i])
                              printf("** %d old solution %g new %g sol %g\n",
                                     i, xsolution[i], solution_[i], solution_[i]);
                    }
                    //delete [] xcost;
                    //delete [] xupper;
                    //delete [] xlower;
                    //delete [] xdj;
                    //delete [] xsolution;
#endif
                    // put back bounds as they were if was optimal
                    if (doOriginalTolerance == 2 && cleanDuals != 2) {
                         changeMade_++; // say something changed
                         /* We may have already changed some bounds in this function
                            so save numberFake_ and add in.

                            Worst that can happen is that we waste a bit of time  - but it must be finite.
                         */
                         //int saveNumberFake = numberFake_;
                         //resetFakeBounds(-1);
                         changeBounds(3, NULL, changeCost);
                         needCleanFake = true;
                         //numberFake_ += saveNumberFake;
                         //resetFakeBounds(-1);
                         cleanDuals = 2;
                         //cleanDuals=1;
                    }
#if 0
                    //int i;
                    for (i = 0; i < numberRows_ + numberColumns_; i++) {
                         if (cost_[i] != xcost[i])
                              printf("** %d old cost %g new %g sol %g\n",
                                     i, xcost[i], cost_[i], solution_[i]);
                         if (lower_[i] != xlower[i])
                              printf("** %d old lower %g new %g sol %g\n",
                                     i, xlower[i], lower_[i], solution_[i]);
                         if (upper_[i] != xupper[i])
                              printf("** %d old upper %g new %g sol %g\n",
                                     i, xupper[i], upper_[i], solution_[i]);
                         if (dj_[i] != xdj[i])
                              printf("** %d old dj %g new %g sol %g\n",
                                     i, xdj[i], dj_[i], solution_[i]);
                         if (solution_[i] != xsolution[i])
                              printf("** %d old solution %g new %g sol %g\n",
                                     i, xsolution[i], solution_[i], solution_[i]);
                    }
                    delete [] xcost;
                    delete [] xupper;
                    delete [] xlower;
                    delete [] xdj;
                    delete [] xsolution;
#endif
               }
               if (cleanDuals == 1 || (cleanDuals == 2 && !numberDualInfeasibilities_)) {
                    // make sure dual feasible
                    // look at all rows and columns
                    rowArray_[0]->clear();
                    columnArray_[0]->clear();
                    double objectiveChange = 0.0;
		    double savePrimalInfeasibilities = sumPrimalInfeasibilities_;
		    if (!numberIterations_) {
		      int nTotal = numberRows_ + numberColumns_;
		      if (arraysNotCreated) {
			// create save arrays
			delete [] saveStatus_;
			delete [] savedSolution_;
			saveStatus_ = new unsigned char [nTotal];
			savedSolution_ = new double [nTotal];
			arraysNotCreated = false;
		      }
		      // save arrays
		      CoinMemcpyN(status_, nTotal, saveStatus_);
		      CoinMemcpyN(rowActivityWork_,
				  numberRows_, savedSolution_ + numberColumns_);
		      CoinMemcpyN(columnActivityWork_, numberColumns_, savedSolution_);
		    }
#if 0
                    double * xcost = new double[numberRows_+numberColumns_];
                    double * xlower = new double[numberRows_+numberColumns_];
                    double * xupper = new double[numberRows_+numberColumns_];
                    double * xdj = new double[numberRows_+numberColumns_];
                    double * xsolution = new double[numberRows_+numberColumns_];
                    CoinMemcpyN(cost_, (numberRows_ + numberColumns_), xcost);
                    CoinMemcpyN(lower_, (numberRows_ + numberColumns_), xlower);
                    CoinMemcpyN(upper_, (numberRows_ + numberColumns_), xupper);
                    CoinMemcpyN(dj_, (numberRows_ + numberColumns_), xdj);
                    CoinMemcpyN(solution_, (numberRows_ + numberColumns_), xsolution);
#endif
                    if (givenDuals)
                         dualTolerance_ = 1.0e50;
#if CLP_CAN_HAVE_ZERO_OBJ>1
		    if ((specialOptions_&2097152)==0) {
#endif
                    updateDualsInDual(rowArray_[0], columnArray_[0], rowArray_[1],
                                      0.0, objectiveChange, true);
#if CLP_CAN_HAVE_ZERO_OBJ>1
		    } else {
		      rowArray_[0]->clear();
		      rowArray_[1]->clear();
		      columnArray_[0]->clear();
		    }
#endif
                    dualTolerance_ = saveTolerance;
#if 0
                    int i;
                    for (i = 0; i < numberRows_ + numberColumns_; i++) {
                         if (cost_[i] != xcost[i])
                              printf("** %d old cost %g new %g sol %g\n",
                                     i, xcost[i], cost_[i], solution_[i]);
                         if (lower_[i] != xlower[i])
                              printf("** %d old lower %g new %g sol %g\n",
                                     i, xlower[i], lower_[i], solution_[i]);
                         if (upper_[i] != xupper[i])
                              printf("** %d old upper %g new %g sol %g\n",
                                     i, xupper[i], upper_[i], solution_[i]);
                         if (dj_[i] != xdj[i])
                              printf("** %d old dj %g new %g sol %g\n",
                                     i, xdj[i], dj_[i], solution_[i]);
                         if (solution_[i] != xsolution[i])
                              printf("** %d old solution %g new %g sol %g\n",
                                     i, xsolution[i], solution_[i], solution_[i]);
                    }
                    delete [] xcost;
                    delete [] xupper;
                    delete [] xlower;
                    delete [] xdj;
                    delete [] xsolution;
#endif
                    // for now - recompute all
                    gutsOfSolution(NULL, NULL);
                    if (givenDuals)
                         dualTolerance_ = 1.0e50;
#if CLP_CAN_HAVE_ZERO_OBJ>1
		    if ((specialOptions_&2097152)==0) {
#endif
                    updateDualsInDual(rowArray_[0], columnArray_[0], rowArray_[1],
                                      0.0, objectiveChange, true);
#if CLP_CAN_HAVE_ZERO_OBJ>1
		    } else {
		      rowArray_[0]->clear();
		      rowArray_[1]->clear();
		      columnArray_[0]->clear();
		    }
#endif
                    dualTolerance_ = saveTolerance;
		    if (!numberIterations_ && sumPrimalInfeasibilities_ >
			1.0e5*(savePrimalInfeasibilities+1.0e3) &&
			(moreSpecialOptions_ & (256|8192)) == 0) {
		      // Use primal
		      int nTotal = numberRows_ + numberColumns_;
		      CoinMemcpyN(saveStatus_, nTotal, status_);
		      CoinMemcpyN(savedSolution_ + numberColumns_ ,
				  numberRows_, rowActivityWork_);
		      CoinMemcpyN(savedSolution_ ,
				  numberColumns_, columnActivityWork_);
		      problemStatus_ = 10;
		      situationChanged = 0;
		    }
                    //assert(numberDualInfeasibilitiesWithoutFree_==0);
                    if (numberDualInfeasibilities_) {
		        if ((numberPrimalInfeasibilities_ || numberPivots)
			    && problemStatus_!=10) {
                              problemStatus_ = -1; // carry on as normal
                         } else {
                              problemStatus_ = 10; // try primal
#if COIN_DEVELOP>1
                              printf("returning at %d\n", __LINE__);
#endif
                         }
                    } else if (situationChanged == 2) {
                         problemStatus_ = -1; // carry on as normal
                         // need to reset bounds
                         changeBounds(3, NULL, changeCost);
                    }
                    situationChanged = 0;
               } else {
                    // iterate
                    if (cleanDuals != 2) {
                         problemStatus_ = -1;
                    } else {
                         problemStatus_ = 10; // try primal
#if COIN_DEVELOP>2
                         printf("returning at %d\n", __LINE__);
#endif
                    }
               }
          }
     }
     // unflag all variables (we may want to wait a bit?)
     if ((tentativeStatus != -2 && tentativeStatus != -1) && unflagVariables) {
          int iRow;
          int numberFlagged = 0;
          for (iRow = 0; iRow < numberRows_; iRow++) {
               int iPivot = pivotVariable_[iRow];
               if (flagged(iPivot)) {
                    numberFlagged++;
                    clearFlagged(iPivot);
               }
          }
#ifdef COIN_DEVELOP
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
               if (factorization_->pivotTolerance() < 0.9) {
                    factorization_->pivotTolerance(0.99);
                    problemStatus_ = -1;
               } else if (numberTimesOptimal_ < 3) {
                    numberTimesOptimal_++;
                    problemStatus_ = -1;
               } else {
                    unflagVariables = false;
                    //secondaryStatus_ = 1; // and say probably infeasible
                    if ((moreSpecialOptions_ & 256) == 0) {
                         // try primal
                         problemStatus_ = 10;
                    } else {
                         // almost certainly infeasible
                         problemStatus_ = 1;
                    }
#if COIN_DEVELOP>1
                    printf("returning at %d\n", __LINE__);
#endif
               }
          }
     }
     if (problemStatus_ < 0) {
          if (needCleanFake) {
               double dummyChangeCost = 0.0;
               changeBounds(3, NULL, dummyChangeCost);
          }
#if 0
          if (objectiveValue_ < lastObjectiveValue_ - 1.0e-8 *
                    CoinMax(fabs(objectivevalue_), fabs(lastObjectiveValue_))) {
          } else {
               lastObjectiveValue_ = objectiveValue_;
          }
#endif
          if (type == 0 || type == 1) {
               if (!type && arraysNotCreated) {
                    // create save arrays
                    delete [] saveStatus_;
                    delete [] savedSolution_;
                    saveStatus_ = new unsigned char [numberRows_+numberColumns_];
                    savedSolution_ = new double [numberRows_+numberColumns_];
               }
               // save arrays
               CoinMemcpyN(status_, numberColumns_ + numberRows_, saveStatus_);
               CoinMemcpyN(rowActivityWork_,
                           numberRows_, savedSolution_ + numberColumns_);
               CoinMemcpyN(columnActivityWork_, numberColumns_, savedSolution_);
               // save extra stuff
               int dummy;
               matrix_->generalExpanded(this, 5, dummy);
          }
          if (weightsSaved) {
               // restore weights (if saved) - also recompute infeasibility list
               if (!reallyBadProblems && (largestPrimalError_ < 100.0 || numberPivots > 10)) {
                    if (tentativeStatus > -3)
                         dualRowPivot_->saveWeights(this, (type < 2) ? 2 : 4);
                    else
                         dualRowPivot_->saveWeights(this, 3);
               } else {
                    // reset weights or scale back
                    dualRowPivot_->saveWeights(this, 6);
               }
          } else if (weightsSaved2 && numberPrimalInfeasibilities_) {
               dualRowPivot_->saveWeights(this, 3);
          }
     }
     // see if cutoff reached
     double limit = 0.0;
     getDblParam(ClpDualObjectiveLimit, limit);
#if 0
     if(fabs(limit) < 1.0e30 && objectiveValue()*optimizationDirection_ >
               limit + 100.0) {
          printf("lim %g obj %g %g - wo perturb %g sum dual %g\n",
                 limit, objectiveValue_, objectiveValue(), computeInternalObjectiveValue(), sumDualInfeasibilities_);
     }
#endif
     if(fabs(limit) < 1.0e30 && objectiveValue()*optimizationDirection_ >
               limit && !numberAtFakeBound()) {
          bool looksInfeasible = !numberDualInfeasibilities_;
          if (objectiveValue()*optimizationDirection_ > limit + fabs(0.1 * limit) + 1.0e2 * sumDualInfeasibilities_ + 1.0e4 &&
                    sumDualInfeasibilities_ < largestDualError_ && numberIterations_ > 0.5 * numberRows_ + 1000)
               looksInfeasible = true;
          if (looksInfeasible) {
               // Even if not perturbed internal costs may have changed
               // be careful
               if (true || numberIterations_) {
                    if(computeInternalObjectiveValue() > limit) {
                         problemStatus_ = 1;
                         secondaryStatus_ = 1; // and say was on cutoff
                    }
               } else {
                    problemStatus_ = 1;
                    secondaryStatus_ = 1; // and say was on cutoff
               }
          }
     }
     // If we are in trouble and in branch and bound give up
     if ((specialOptions_ & 1024) != 0) {
          int looksBad = 0;
          if (largestPrimalError_ * largestDualError_ > 1.0e2) {
               looksBad = 1;
          } else if (largestPrimalError_ > 1.0e-2
                     && objectiveValue_ > CoinMin(1.0e15, 1.0e3 * limit)) {
               looksBad = 2;
          }
          if (looksBad) {
               if (factorization_->pivotTolerance() < 0.9) {
                    // up tolerance
                    factorization_->pivotTolerance(CoinMin(factorization_->pivotTolerance() * 1.05 + 0.02, 0.91));
               } else if (numberIterations_ > 10000) {
                    if (handler_->logLevel() > 2)
                         printf("bad dual - saying infeasible %d\n", looksBad);
                    problemStatus_ = 1;
                    secondaryStatus_ = 1; // and say was on cutoff
               } else if (largestPrimalError_ > 1.0e5) {
                    {
		      //int iBigB = -1;
                         double bigB = 0.0;
                         //int iBigN = -1;
                         double bigN = 0.0;
                         for (int i = 0; i < numberRows_ + numberColumns_; i++) {
                              double value = fabs(solution_[i]);
                              if (getStatus(i) == basic) {
                                   if (value > bigB) {
                                        bigB = value;
                                        //iBigB = i;
                                   }
                              } else {
                                   if (value > bigN) {
                                        bigN = value;
                                        //iBigN = i;
                                   }
                              }
                         }
#ifdef CLP_INVESTIGATE
                         if (bigB > 1.0e8 || bigN > 1.0e8) {
                              if (handler_->logLevel() > 0)
                                   printf("it %d - basic %d %g, nonbasic %d %g\n",
                                          numberIterations_, iBigB, bigB, iBigN, bigN);
                         }
#endif
                    }
#if COIN_DEVELOP!=2
                    if (handler_->logLevel() > 2)
#endif
                         printf("bad dual - going to primal %d %g\n", looksBad, largestPrimalError_);
                    allSlackBasis(true);
                    problemStatus_ = 10;
               }
          }
     }
     if (problemStatus_ < 0 && !changeMade_) {
          problemStatus_ = 4; // unknown
     }
     lastGoodIteration_ = numberIterations_;
     if (numberIterations_ > lastBadIteration_ + 100)
          moreSpecialOptions_ &= ~16; // clear check accuracy flag
     if (problemStatus_ < 0) {
          sumDualInfeasibilities_ = realDualInfeasibilities; // back to say be careful
          if (sumDualInfeasibilities_)
               numberDualInfeasibilities_ = 1;
     }
#ifdef CLP_REPORT_PROGRESS
     if (ixxxxxx > ixxyyyy - 3) {
          printf("objectiveValue_ %g\n", objectiveValue_);
          handler_->setLogLevel(63);
          int nTotal = numberColumns_ + numberRows_;
          double newObj = 0.0;
          for (int i = 0; i < nTotal; i++) {
               if (solution_[i])
                    newObj += solution_[i] * cost_[i];
          }
          printf("xxx obj %g\n", newObj);
          // for now - recompute all
          gutsOfSolution(NULL, NULL);
          newObj = 0.0;
          for (int i = 0; i < nTotal; i++) {
               if (solution_[i])
                    newObj += solution_[i] * cost_[i];
          }
          printf("yyy obj %g %g\n", newObj, objectiveValue_);
          progress_.modifyObjective(objectiveValue_
                                    - bestPossibleImprovement_);
     }
#endif
#if 1
     double thisObj = progress_.lastObjective(0);
     double lastObj = progress_.lastObjective(1);
     if (lastObj > thisObj + 1.0e-4 * CoinMax(fabs(thisObj), fabs(lastObj)) + 1.0e-4
               && givenDuals == NULL && firstFree_ < 0) {
          int maxFactor = factorization_->maximumPivots();
          if (maxFactor > 10) {
               if (forceFactorization_ < 0)
                    forceFactorization_ = maxFactor;
               forceFactorization_ = CoinMax(1, (forceFactorization_ >> 1));
               //printf("Reducing factorization frequency\n");
          }
     }
#endif
     // Allow matrices to be sorted etc
     int fake = -999; // signal sort
     matrix_->correctSequence(this, fake, fake);
     if (alphaAccuracy_ > 0.0)
          alphaAccuracy_ = 1.0;
     // If we are stopping - use plausible objective
     // Maybe only in fast dual
     if (problemStatus_ > 2)
          objectiveValue_ = approximateObjective;
     if (problemStatus_ == 1 && (progressFlag_&8) != 0 &&
	 fabs(objectiveValue_) > 1.0e10 )
       problemStatus_ = 10; // infeasible - but has looked feasible
}
/* While updateDualsInDual sees what effect is of flip
   this does actual flipping.
   If change >0.0 then value in array >0.0 => from lower to upper
*/
void
ClpSimplexDual::flipBounds(CoinIndexedVector * rowArray,
                           CoinIndexedVector * columnArray)
{
     int number;
     int * which;

     int iSection;

     for (iSection = 0; iSection < 2; iSection++) {
          int i;
          double * solution = solutionRegion(iSection);
          double * lower = lowerRegion(iSection);
          double * upper = upperRegion(iSection);
          int addSequence;
          if (!iSection) {
               number = rowArray->getNumElements();
               which = rowArray->getIndices();
               addSequence = numberColumns_;
          } else {
               number = columnArray->getNumElements();
               which = columnArray->getIndices();
               addSequence = 0;
          }

          for (i = 0; i < number; i++) {
               int iSequence = which[i];
               Status status = getStatus(iSequence + addSequence);

               switch(status) {

               case basic:
               case isFree:
               case superBasic:
               case ClpSimplex::isFixed:
                    break;
               case atUpperBound:
                    // to lower bound
                    setStatus(iSequence + addSequence, atLowerBound);
                    solution[iSequence] = lower[iSequence];
                    break;
               case atLowerBound:
                    // to upper bound
                    setStatus(iSequence + addSequence, atUpperBound);
                    solution[iSequence] = upper[iSequence];
                    break;
               }
          }
     }
     rowArray->setNumElements(0);
     columnArray->setNumElements(0);
}
// Restores bound to original bound
void
ClpSimplexDual::originalBound( int iSequence)
{
     if (getFakeBound(iSequence) != noFake) {
          numberFake_--;;
          setFakeBound(iSequence, noFake);
          if (iSequence >= numberColumns_) {
               // rows
               int iRow = iSequence - numberColumns_;
               rowLowerWork_[iRow] = rowLower_[iRow];
               rowUpperWork_[iRow] = rowUpper_[iRow];
               if (rowScale_) {
                    if (rowLowerWork_[iRow] > -1.0e50)
                         rowLowerWork_[iRow] *= rowScale_[iRow] * rhsScale_;
                    if (rowUpperWork_[iRow] < 1.0e50)
                         rowUpperWork_[iRow] *= rowScale_[iRow] * rhsScale_;
               } else if (rhsScale_ != 1.0) {
                    if (rowLowerWork_[iRow] > -1.0e50)
                         rowLowerWork_[iRow] *= rhsScale_;
                    if (rowUpperWork_[iRow] < 1.0e50)
                         rowUpperWork_[iRow] *= rhsScale_;
               }
          } else {
               // columns
               columnLowerWork_[iSequence] = columnLower_[iSequence];
               columnUpperWork_[iSequence] = columnUpper_[iSequence];
               if (rowScale_) {
                    double multiplier = 1.0 * inverseColumnScale_[iSequence];
                    if (columnLowerWork_[iSequence] > -1.0e50)
                         columnLowerWork_[iSequence] *= multiplier * rhsScale_;
                    if (columnUpperWork_[iSequence] < 1.0e50)
                         columnUpperWork_[iSequence] *= multiplier * rhsScale_;
               } else if (rhsScale_ != 1.0) {
                    if (columnLowerWork_[iSequence] > -1.0e50)
                         columnLowerWork_[iSequence] *= rhsScale_;
                    if (columnUpperWork_[iSequence] < 1.0e50)
                         columnUpperWork_[iSequence] *= rhsScale_;
               }
          }
     }
}
/* As changeBounds but just changes new bounds for a single variable.
   Returns true if change */
bool
ClpSimplexDual::changeBound( int iSequence)
{
     // old values
     double oldLower = lower_[iSequence];
     double oldUpper = upper_[iSequence];
     double value = solution_[iSequence];
     bool modified = false;
     originalBound(iSequence);
     // original values
     double lowerValue = lower_[iSequence];
     double upperValue = upper_[iSequence];
     // back to altered values
     lower_[iSequence] = oldLower;
     upper_[iSequence] = oldUpper;
     assert (getFakeBound(iSequence) == noFake);
     //if (getFakeBound(iSequence)!=noFake)
     //numberFake_--;;
     if (value == oldLower) {
          if (upperValue > oldLower + dualBound_) {
               upper_[iSequence] = oldLower + dualBound_;
               setFakeBound(iSequence, upperFake);
               modified = true;
               numberFake_++;
          }
     } else if (value == oldUpper) {
          if (lowerValue < oldUpper - dualBound_) {
               lower_[iSequence] = oldUpper - dualBound_;
               setFakeBound(iSequence, lowerFake);
               modified = true;
               numberFake_++;
          }
     } else {
          assert(value == oldLower || value == oldUpper);
     }
     return modified;
}
#if ABC_NORMAL_DEBUG>0
//#define PERT_STATISTICS
#endif
#ifdef PERT_STATISTICS
static void breakdown(const char * name, int numberLook, const double * region)
{
     double range[] = {
          -COIN_DBL_MAX,
          -1.0e15, -1.0e11, -1.0e8, -1.0e5, -1.0e4, -1.0e3, -1.0e2, -1.0e1,
          -1.0,
          -1.0e-1, -1.0e-2, -1.0e-3, -1.0e-4, -1.0e-5, -1.0e-8, -1.0e-11, -1.0e-15,
          0.0,
          1.0e-15, 1.0e-11, 1.0e-8, 1.0e-5, 1.0e-4, 1.0e-3, 1.0e-2, 1.0e-1,
          1.0,
          1.0e1, 1.0e2, 1.0e3, 1.0e4, 1.0e5, 1.0e8, 1.0e11, 1.0e15,
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
int
ClpSimplexDual::perturb()
{
     if (perturbation_ > 100)
          return 0; //perturbed already
     if (perturbation_ == 100)
          perturbation_ = 50; // treat as normal
     int savePerturbation = perturbation_;
     bool modifyRowCosts = false;
     // dual perturbation
     double perturbation = 1.0e-20;
     // maximum fraction of cost to perturb
     double maximumFraction = 1.0e-5;
     double constantPerturbation = 100.0 * dualTolerance_;
     int maxLength = 0;
     int minLength = numberRows_;
     double averageCost = 0.0;
#if 0
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
#endif
     int numberNonZero = 0;
     if (!numberIterations_ && perturbation_ >= 50) {
          // See if we need to perturb
          double * sort = new double[numberColumns_];
          // Use objective BEFORE scaling
          const double * obj = ((moreSpecialOptions_ & 128) == 0) ? objective() : cost_;
          int i;
          for (i = 0; i < numberColumns_; i++) {
               double value = fabs(obj[i]);
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
          for (i = 1; i < numberColumns_; i++) {
               if (last != sort[i])
                    number++;
               last = sort[i];
          }
          delete [] sort;
          if (!numberNonZero && perturbation_ < 55)
               return 1; // safer to use primal
#if 0
          printf("nnz %d percent %d", number, (number * 100) / numberColumns_);
          if (number * 4 > numberColumns_)
               printf(" - Would not perturb\n");
          else
               printf(" - Would perturb\n");
          //exit(0);
#endif
          //printf("ratio number diff costs %g, element ratio %g\n",((double)number)/((double) numberColumns_),
          //								      elementRatio);
          //number=0;
          //if (number*4>numberColumns_||elementRatio>1.0e12) {
          if (number * 4 > numberColumns_) {
               perturbation_ = 100;
               return 0; // good enough
          }
     }
     int iColumn;
     const int * columnLength = matrix_->getVectorLengths();
     for (iColumn = 0; iColumn < numberColumns_; iColumn++) {
          if (columnLowerWork_[iColumn] < columnUpperWork_[iColumn]) {
               int length = columnLength[iColumn];
               if (length > 2) {
                    maxLength = CoinMax(maxLength, length);
                    minLength = CoinMin(minLength, length);
               }
          }
     }
     // If > 70 then do rows
     if (perturbation_ >= 70) {
          modifyRowCosts = true;
          perturbation_ -= 20;
          printf("Row costs modified, ");
     }
     bool uniformChange = false;
     bool inCbcOrOther = (specialOptions_ & 0x03000000) != 0;
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
               double lo = rowLowerWork_[iRow];
               double up = rowUpperWork_[iRow];
               if (lo < up) {
                    double value = fabs(rowObjectiveWork_[iRow]);
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
          for (iColumn = 0; iColumn < numberColumns_; iColumn++) {
               double lo = columnLowerWork_[iColumn];
               double up = columnUpperWork_[iColumn];
               if (lo < up) {
                    double value =
                         fabs(objectiveWork_[iColumn]);
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
     } else {
          // user is in charge
          maximumFraction = 1.0e-1;
          // but some experiments
          if (perturbation_ <= -900) {
               modifyRowCosts = true;
               perturbation_ += 1000;
               printf("Row costs modified, ");
          }
          if (perturbation_ <= -10) {
               perturbation_ += 10;
               maximumFraction = 1.0;
               if ((-perturbation_) % 100 >= 10) {
                    uniformChange = true;
                    perturbation_ += 20;
               }
               while (perturbation_ < -10) {
                    perturbation_ += 100;
                    maximumFraction *= 1.0e-1;
               }
          }
          perturbation = pow(10.0, perturbation_);
     }
     double largestZero = 0.0;
     double largest = 0.0;
     double largestPerCent = 0.0;
     // modify costs
     bool printOut = (handler_->logLevel() == 63);
     printOut = false;
     //assert (!modifyRowCosts);
     modifyRowCosts = false;
     if (modifyRowCosts) {
          for (iRow = 0; iRow < numberRows_; iRow++) {
               if (rowLowerWork_[iRow] < rowUpperWork_[iRow]) {
                    double value = perturbation;
                    double currentValue = rowObjectiveWork_[iRow];
                    value = CoinMin(value, maximumFraction * (fabs(currentValue) + 1.0e-1 * perturbation + 1.0e-3));
                    if (rowLowerWork_[iRow] > -largeValue_) {
                         if (fabs(rowLowerWork_[iRow]) < fabs(rowUpperWork_[iRow]))
                              value *= randomNumberGenerator_.randomDouble();
                         else
                              value *= -randomNumberGenerator_.randomDouble();
                    } else if (rowUpperWork_[iRow] < largeValue_) {
                         value *= -randomNumberGenerator_.randomDouble();
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
                         printf("row %d cost %g change %g\n", iRow, rowObjectiveWork_[iRow], value);
                    rowObjectiveWork_[iRow] += value;
               }
          }
     }
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
#define SAVE_PERT
#ifdef SAVE_PERT
     if (2 * numberColumns_ > maximumPerturbationSize_) {
          delete [] perturbationArray_;
          maximumPerturbationSize_ = 2 * numberColumns_;
          perturbationArray_ = new double [maximumPerturbationSize_];
          for (iColumn = 0; iColumn < maximumPerturbationSize_; iColumn++) {
               perturbationArray_[iColumn] = randomNumberGenerator_.randomDouble();
          }
     }
#endif
     for (iColumn = 0; iColumn < numberColumns_; iColumn++) {
          if (columnLowerWork_[iColumn] < columnUpperWork_[iColumn] && getStatus(iColumn) != basic) {
               double value = perturbation;
               double currentValue = objectiveWork_[iColumn];
               value = CoinMin(value, constantPerturbation + maximumFraction * (fabs(currentValue) + 1.0e-1 * perturbation + 1.0e-8));
               //value = CoinMin(value,constantPerturbation;+maximumFraction*fabs(currentValue));
               double value2 = constantPerturbation + 1.0e-1 * smallestNonZero;
               if (uniformChange) {
                    value = maximumFraction;
                    value2 = maximumFraction;
               }
               if (columnLowerWork_[iColumn] > -largeValue_) {
                    if (fabs(columnLowerWork_[iColumn]) <
                              fabs(columnUpperWork_[iColumn])) {
#ifndef SAVE_PERT
                         value *= (1.0 - m1 + m1 * randomNumberGenerator_.randomDouble());
                         value2 *= (1.0 - m1 + m1 * randomNumberGenerator_.randomDouble());
#else
                         value *= (1.0 - m1 + m1 * perturbationArray_[2*iColumn]);
                         value2 *= (1.0 - m1 + m1 * perturbationArray_[2*iColumn+1]);
#endif
                    } else {
                         //value *= -(1.0-m1+m1*randomNumberGenerator_.randomDouble());
                         //value2 *= -(1.0-m1+m1*randomNumberGenerator_.randomDouble());
                         value = 0.0;
                    }
               } else if (columnUpperWork_[iColumn] < largeValue_) {
#ifndef SAVE_PERT
                    value *= -(1.0 - m1 + m1 * randomNumberGenerator_.randomDouble());
                    value2 *= -(1.0 - m1 + m1 * randomNumberGenerator_.randomDouble());
#else
                    value *= -(1.0 - m1 + m1 * perturbationArray_[2*iColumn]);
                    value2 *= -(1.0 - m1 + m1 * perturbationArray_[2*iColumn+1]);
#endif
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
                    if (savePerturbation < 50 || savePerturbation > 60) {
                         if (fabs(value) <= dualTolerance_)
                              value = 0.0;
                    } else if (value) {
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
                    if (getStatus(iColumn) == atUpperBound)
                         value = -value;
                    if (printOut)
                         printf("col %d cost %g change %g\n", iColumn, objectiveWork_[iColumn], value);
                    objectiveWork_[iColumn] += value;
               }
          }
     }
     handler_->message(CLP_SIMPLEX_PERTURB, messages_)
               << 100.0 * maximumFraction << perturbation << largest << 100.0 * largestPerCent << largestZero
               << CoinMessageEol;
     // and zero changes
     //int nTotal = numberRows_+numberColumns_;
     //CoinZeroN(cost_+nTotal,nTotal);
     // say perturbed
#ifdef PERT_STATISTICS
  {
    double averageCost = 0.0;
    int numberNonZero = 0;
    double * COIN_RESTRICT sort = new double[numberColumns_];
    for (int i = 0; i < numberColumns_; i++) {
      double value = fabs(cost_[i]);
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
    printf("nnz %d percent %d", number, (number * 100) / numberColumns_);
    delete [] sort;
    breakdown("Objective", numberColumns_+numberRows_, cost_);
  }
#endif
     perturbation_ = 101;
     return 0;
}
/* For strong branching.  On input lower and upper are new bounds
   while on output they are change in objective function values
   (>1.0e50 infeasible).
   Return code is 0 if nothing interesting, -1 if infeasible both
   ways and +1 if infeasible one way (check values to see which one(s))
   Returns -2 if bad factorization
*/
int ClpSimplexDual::strongBranching(int numberVariables, const int * variables,
                                    double * newLower, double * newUpper,
                                    double ** outputSolution,
                                    int * outputStatus, int * outputIterations,
                                    bool stopOnFirstInfeasible,
                                    bool alwaysFinish,
                                    int startFinishOptions)
{
     int i;
     int returnCode = 0;
     double saveObjectiveValue = objectiveValue_;
     algorithm_ = -1;

     //scaling(false);

     // put in standard form (and make row copy)
     // create modifiable copies of model rim and do optional scaling
     createRim(7 + 8 + 16 + 32, true, startFinishOptions);

     // change newLower and newUpper if scaled

     // Do initial factorization
     // and set certain stuff
     // We can either set increasing rows so ...IsBasic gives pivot row
     // or we can just increment iBasic one by one
     // for now let ...iBasic give pivot row
     int useFactorization = false;
     if ((startFinishOptions & 2) != 0 && (whatsChanged_&(2 + 512)) == 2 + 512)
          useFactorization = true; // Keep factorization if possible
     // switch off factorization if bad
     if (pivotVariable_[0] < 0)
          useFactorization = false;
     if (!useFactorization || factorization_->numberRows() != numberRows_) {
          useFactorization = false;
          factorization_->setDefaultValues();

          int factorizationStatus = internalFactorize(0);
          if (factorizationStatus < 0) {
               // some error
               // we should either debug or ignore
#ifndef NDEBUG
               printf("***** ClpDual strong branching factorization error - debug\n");
#endif
               return -2;
          } else if (factorizationStatus && factorizationStatus <= numberRows_) {
               handler_->message(CLP_SINGULARITIES, messages_)
                         << factorizationStatus
                         << CoinMessageEol;
          }
     }
     // save stuff
     ClpFactorization saveFactorization(*factorization_);
     // Get fake bounds correctly
     double changeCost;
     changeBounds(3, NULL, changeCost);
     int saveNumberFake = numberFake_;
     // save basis and solution
     double * saveSolution = new double[numberRows_+numberColumns_];
     CoinMemcpyN(solution_,
                 numberRows_ + numberColumns_, saveSolution);
     unsigned char * saveStatus =
          new unsigned char [numberRows_+numberColumns_];
     CoinMemcpyN(status_, numberColumns_ + numberRows_, saveStatus);
     // save bounds as createRim makes clean copies
     double * saveLower = new double[numberRows_+numberColumns_];
     CoinMemcpyN(lower_,
                 numberRows_ + numberColumns_, saveLower);
     double * saveUpper = new double[numberRows_+numberColumns_];
     CoinMemcpyN(upper_,
                 numberRows_ + numberColumns_, saveUpper);
     double * saveObjective = new double[numberRows_+numberColumns_];
     CoinMemcpyN(cost_,
                 numberRows_ + numberColumns_, saveObjective);
     int * savePivot = new int [numberRows_];
     CoinMemcpyN(pivotVariable_, numberRows_, savePivot);
     // need to save/restore weights.

     int iSolution = 0;
     for (i = 0; i < numberVariables; i++) {
          int iColumn = variables[i];
          double objectiveChange;
          double saveBound;

          // try down

          saveBound = columnUpper_[iColumn];
          // external view - in case really getting optimal
          columnUpper_[iColumn] = newUpper[i];
          assert (inverseColumnScale_ || scalingFlag_ <= 0);
          if (scalingFlag_ <= 0)
               upper_[iColumn] = newUpper[i] * rhsScale_;
          else
               upper_[iColumn] = (newUpper[i] * inverseColumnScale_[iColumn]) * rhsScale_; // scale
          // Start of fast iterations
          int status = fastDual(alwaysFinish);
          CoinAssert (problemStatus_ || objectiveValue_ < 1.0e50);
#ifdef CLP_DEBUG
	  printf("first status %d obj %g\n",problemStatus_,objectiveValue_);
#endif
	  if(problemStatus_==10) 
 	      problemStatus_=3;
          // make sure plausible
          double obj = CoinMax(objectiveValue_, saveObjectiveValue);
          if (status && problemStatus_ != 3) {
               // not finished - might be optimal
               checkPrimalSolution(rowActivityWork_, columnActivityWork_);
               double limit = 0.0;
               getDblParam(ClpDualObjectiveLimit, limit);
               if (!numberPrimalInfeasibilities_ && obj < limit) {
                    problemStatus_ = 0;
               }
               status = problemStatus_;
          }
          if (problemStatus_ == 3)
               status = 2;
          if (status || (problemStatus_ == 0 && !isDualObjectiveLimitReached())) {
               objectiveChange = obj - saveObjectiveValue;
          } else {
               objectiveChange = 1.0e100;
               status = 1;
          }

          if (scalingFlag_ <= 0) {
               CoinMemcpyN(solution_, numberColumns_, outputSolution[iSolution]);
          } else {
               int j;
               double * sol = outputSolution[iSolution];
               for (j = 0; j < numberColumns_; j++)
                    sol[j] = solution_[j] * columnScale_[j];
          }
          outputStatus[iSolution] = status;
          outputIterations[iSolution] = numberIterations_;
          iSolution++;
          // restore
          numberFake_ = saveNumberFake;
          CoinMemcpyN(saveSolution,
                      numberRows_ + numberColumns_, solution_);
          CoinMemcpyN(saveStatus, numberColumns_ + numberRows_, status_);
          CoinMemcpyN(saveLower,
                      numberRows_ + numberColumns_, lower_);
          CoinMemcpyN(saveUpper,
                      numberRows_ + numberColumns_, upper_);
          CoinMemcpyN(saveObjective,
                      numberRows_ + numberColumns_, cost_);
          columnUpper_[iColumn] = saveBound;
          CoinMemcpyN(savePivot, numberRows_, pivotVariable_);
          //delete factorization_;
          //factorization_ = new ClpFactorization(saveFactorization,numberRows_);
          setFactorization(saveFactorization);
          newUpper[i] = objectiveChange;
#ifdef CLP_DEBUG
          printf("down on %d costs %g\n", iColumn, objectiveChange);
#endif

          // try up

          saveBound = columnLower_[iColumn];
          // external view - in case really getting optimal
          columnLower_[iColumn] = newLower[i];
          assert (inverseColumnScale_ || scalingFlag_ <= 0);
          if (scalingFlag_ <= 0)
               lower_[iColumn] = newLower[i] * rhsScale_;
          else
               lower_[iColumn] = (newLower[i] * inverseColumnScale_[iColumn]) * rhsScale_; // scale
          // Start of fast iterations
          status = fastDual(alwaysFinish);
	  CoinAssert (problemStatus_||objectiveValue_<1.0e50);
#ifdef CLP_DEBUG
	  printf("second status %d obj %g\n",problemStatus_,objectiveValue_);
#endif
	  if(problemStatus_==10) 
	      problemStatus_=3;
          // make sure plausible
          obj = CoinMax(objectiveValue_, saveObjectiveValue);
          if (status && problemStatus_ != 3) {
               // not finished - might be optimal
               checkPrimalSolution(rowActivityWork_, columnActivityWork_);
               double limit = 0.0;
               getDblParam(ClpDualObjectiveLimit, limit);
               if (!numberPrimalInfeasibilities_ && obj < limit) {
                    problemStatus_ = 0;
               }
               status = problemStatus_;
          }
          if (problemStatus_ == 3)
               status = 2;
          if (status || (problemStatus_ == 0 && !isDualObjectiveLimitReached())) {
               objectiveChange = obj - saveObjectiveValue;
          } else {
               objectiveChange = 1.0e100;
               status = 1;
          }
          if (scalingFlag_ <= 0) {
               CoinMemcpyN(solution_, numberColumns_, outputSolution[iSolution]);
          } else {
               int j;
               double * sol = outputSolution[iSolution];
               for (j = 0; j < numberColumns_; j++)
                    sol[j] = solution_[j] * columnScale_[j];
          }
          outputStatus[iSolution] = status;
          outputIterations[iSolution] = numberIterations_;
          iSolution++;

          // restore
          numberFake_ = saveNumberFake;
          CoinMemcpyN(saveSolution,
                      numberRows_ + numberColumns_, solution_);
          CoinMemcpyN(saveStatus, numberColumns_ + numberRows_, status_);
          CoinMemcpyN(saveLower,
                      numberRows_ + numberColumns_, lower_);
          CoinMemcpyN(saveUpper,
                      numberRows_ + numberColumns_, upper_);
          CoinMemcpyN(saveObjective,
                      numberRows_ + numberColumns_, cost_);
          columnLower_[iColumn] = saveBound;
          CoinMemcpyN(savePivot, numberRows_, pivotVariable_);
          //delete factorization_;
          //factorization_ = new ClpFactorization(saveFactorization,numberRows_);
          setFactorization(saveFactorization);

          newLower[i] = objectiveChange;
#ifdef CLP_DEBUG
          printf("up on %d costs %g\n", iColumn, objectiveChange);
#endif

          /* Possibilities are:
             Both sides feasible - store
             Neither side feasible - set objective high and exit if desired
             One side feasible - change bounds and resolve
          */
          if (newUpper[i] < 1.0e100) {
               if(newLower[i] < 1.0e100) {
                    // feasible - no action
               } else {
                    // up feasible, down infeasible
                    returnCode = 1;
                    if (stopOnFirstInfeasible)
                         break;
               }
          } else {
               if(newLower[i] < 1.0e100) {
                    // down feasible, up infeasible
                    returnCode = 1;
                    if (stopOnFirstInfeasible)
                         break;
               } else {
                    // neither side feasible
                    returnCode = -1;
                    break;
               }
          }
     }
     delete [] saveSolution;
     delete [] saveLower;
     delete [] saveUpper;
     delete [] saveObjective;
     delete [] saveStatus;
     delete [] savePivot;
     if ((startFinishOptions & 1) == 0) {
          deleteRim(1);
          whatsChanged_ &= ~0xffff;
     } else {
          // Original factorization will have been put back by last loop
          //delete factorization_;
          //factorization_ = new ClpFactorization(saveFactorization);
          deleteRim(0);
          // mark all as current
          whatsChanged_ = 0x3ffffff;
     }
     objectiveValue_ = saveObjectiveValue;
     return returnCode;
}
// treat no pivot as finished (unless interesting)
int ClpSimplexDual::fastDual(bool alwaysFinish)
{
     progressFlag_ = 0;
     bestObjectiveValue_ = objectiveValue_;
     algorithm_ = -1;
     secondaryStatus_ = 0;
     // Say in fast dual
     if (!alwaysFinish)
          specialOptions_ |= 1048576;
     specialOptions_ |= 16384;
     int saveDont = dontFactorizePivots_;
     if ((specialOptions_ & 2048) == 0)
          dontFactorizePivots_ = 0;
     else if(!dontFactorizePivots_)
          dontFactorizePivots_ = 20;
     //handler_->setLogLevel(63);
     // save data
     ClpDataSave data = saveData();
     dualTolerance_ = dblParam_[ClpDualTolerance];
     primalTolerance_ = dblParam_[ClpPrimalTolerance];

     // save dual bound
     double saveDualBound = dualBound_;

     // Start can skip some things in transposeTimes
     specialOptions_ |= 131072;
     if (alphaAccuracy_ != -1.0)
          alphaAccuracy_ = 1.0;
     // for dual we will change bounds using dualBound_
     // for this we need clean basis so it is after factorize
#if 0
     {
          int numberTotal = numberRows_ + numberColumns_;
          double * saveSol = CoinCopyOfArray(solution_, numberTotal);
          double * saveDj = CoinCopyOfArray(dj_, numberTotal);
          double tolerance = 1.0e-8;
          gutsOfSolution(NULL, NULL);
          int j;
          double largestPrimal = tolerance;
          int iPrimal = -1;
          for (j = 0; j < numberTotal; j++) {
               double difference = solution_[j] - saveSol[j];
               if (fabs(difference) > largestPrimal) {
                    iPrimal = j;
                    largestPrimal = fabs(difference);
               }
          }
          double largestDual = tolerance;
          int iDual = -1;
          for (j = 0; j < numberTotal; j++) {
               double difference = dj_[j] - saveDj[j];
               if (fabs(difference) > largestDual && upper_[j] > lower_[j]) {
                    iDual = j;
                    largestDual = fabs(difference);
               }
          }
          if (iPrimal >= 0 || iDual >= 0)
               printf("pivots %d primal diff(%g,%d) dual diff(%g,%d)\n",
                      factorization_->pivots(),
                      largestPrimal, iPrimal,
                      largestDual, iDual);
          delete [] saveSol;
          delete [] saveDj;
     }
#else
     if ((specialOptions_ & 524288) == 0)
          gutsOfSolution(NULL, NULL);
#endif
#if 0
     if (numberPrimalInfeasibilities_ != 1 ||
               numberDualInfeasibilities_)
          printf("dual %g (%d) primal %g (%d)\n",
                 sumDualInfeasibilities_, numberDualInfeasibilities_,
                 sumPrimalInfeasibilities_, numberPrimalInfeasibilities_);
#endif
#ifndef NDEBUG
#ifdef COIN_DEVELOP
     resetFakeBounds(-1);
#endif
#endif
     //numberFake_ =0; // Number of variables at fake bounds
     numberChanged_ = 0; // Number of variables with changed costs
     //changeBounds(1,NULL,objectiveChange);

     problemStatus_ = -1;
     numberIterations_ = 0;
     if ((specialOptions_ & 524288) == 0) {
          factorization_->sparseThreshold(0);
          factorization_->goSparse();
     }

     int lastCleaned = 0; // last time objective or bounds cleaned up

     // number of times we have declared optimality
     numberTimesOptimal_ = 0;

     // This says whether to restore things etc
     int factorType = 0;
     /*
       Status of problem:
       0 - optimal
       1 - infeasible
       2 - unbounded
       -1 - iterating
       -2 - factorization wanted
       -3 - redo checking without factorization
       -4 - looks infeasible

       BUT also from whileIterating return code is:

      -1 iterations etc
      -2 inaccuracy
      -3 slight inaccuracy (and done iterations)
      +0 looks optimal (might be unbounded - but we will investigate)
      +1 looks infeasible
      +3 max iterations

     */

     int returnCode = 0;

     int iRow, iColumn;
     int maxPass = maximumIterations();
     while (problemStatus_ < 0) {
          // clear
          for (iRow = 0; iRow < 4; iRow++) {
               rowArray_[iRow]->clear();
          }

          for (iColumn = 0; iColumn < 2; iColumn++) {
               columnArray_[iColumn]->clear();
          }

          // give matrix (and model costs and bounds a chance to be
          // refreshed (normally null)
          matrix_->refresh(this);
          // If getting nowhere - why not give it a kick
          // does not seem to work too well - do some more work
          if ((specialOptions_ & 524288) != 0 && (moreSpecialOptions_&2048) == 0 &&
                    perturbation_ < 101 && numberIterations_ > 2 * (numberRows_ + numberColumns_) && (moreSpecialOptions_&1048576)==0) {
               perturb();
               // Can't get here if values pass
               gutsOfSolution(NULL, NULL);
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
          // may factorize, checks if problem finished
          // should be able to speed this up on first time
          statusOfProblemInDual(lastCleaned, factorType, NULL, data, 0);

          // Say good factorization
          factorType = 1;
          maxPass--;
          if (maxPass < -10) {
               // odd
               returnCode = 1;
               problemStatus_ = 3;
               // can't say anything interesting - might as well return
#ifdef CLP_DEBUG
               printf("returning from fastDual after %d iterations with code %d because of loop\n",
                      numberIterations_, returnCode);
#endif
               break;
          }

          // Do iterations
          if (problemStatus_ < 0) {
               double * givenPi = NULL;
               returnCode = whileIterating(givenPi, 0);
               if ((!alwaysFinish && returnCode < 0) || returnCode == 3) {
                    if (returnCode != 3)
                         assert (problemStatus_ < 0);
                    returnCode = 1;
                    problemStatus_ = 3;
                    // can't say anything interesting - might as well return
#ifdef CLP_DEBUG
                    printf("returning from fastDual after %d iterations with code %d\n",
                           numberIterations_, returnCode);
#endif
                    break;
               }
               if (returnCode == -2)
                    factorType = 3;
               returnCode = 0;
          }
     }

     // clear
     for (iRow = 0; iRow < 4; iRow++) {
          rowArray_[iRow]->clear();
     }

     for (iColumn = 0; iColumn < 2; iColumn++) {
          columnArray_[iColumn]->clear();
     }
     // Say not in fast dual
     specialOptions_ &= ~(16384 | 1048576);
     assert(!numberFake_ || ((specialOptions_&(2048 | 4096)) != 0 && dualBound_ >= 1.0e8)
            || returnCode || problemStatus_); // all bounds should be okay
     if (numberFake_ > 0 && false) {
          // Set back
          double dummy;
          changeBounds(2, NULL, dummy);
     }
     // Restore any saved stuff
     restoreData(data);
     dontFactorizePivots_ = saveDont;
     dualBound_ = saveDualBound;
     // Stop can skip some things in transposeTimes
     specialOptions_ &= ~131072;
     if (!problemStatus_) {
          // see if cutoff reached
          double limit = 0.0;
          getDblParam(ClpDualObjectiveLimit, limit);
          if(fabs(limit) < 1.0e30 && objectiveValue()*optimizationDirection_ >
                    limit + 1.0e-7 + 1.0e-8 * fabs(limit)) {
               // actually infeasible on objective
               problemStatus_ = 1;
               secondaryStatus_ = 1;
          }
     }
     if (problemStatus_ == 3)
          objectiveValue_ = CoinMax(bestObjectiveValue_, objectiveValue_ - bestPossibleImprovement_);
     return returnCode;
}
// This does first part of StrongBranching
ClpFactorization *
ClpSimplexDual::setupForStrongBranching(char * arrays, int numberRows,
                                        int numberColumns, bool solveLp)
{
     if (solveLp) {
          // make sure won't create fake objective
          int saveOptions = specialOptions_;
          specialOptions_ |= 16384;
          // solve
	  int saveMaximumIterations = intParam_[ClpMaxNumIteration];
	  intParam_[ClpMaxNumIteration] = 100+numberRows_+numberColumns_;
          dual(0, 7);
          if (problemStatus_ == 10) {
               ClpSimplex::dual(0, 0);
               assert (problemStatus_ != 10);
               if (problemStatus_ == 0) {
                    dual(0, 7);
                    //assert (problemStatus_!=10);
               }
          }
	  intParam_[ClpMaxNumIteration] = saveMaximumIterations;
          specialOptions_ = saveOptions;
          if (problemStatus_ != 0 && problemStatus_ != 10)
               return NULL; // say infeasible or odd
          // May be empty
          solveLp = (solution_ != NULL && problemStatus_ == 0);
     }
     problemStatus_ = 0;
     if (!solveLp) {
          algorithm_ = -1;
          // put in standard form (and make row copy)
          // create modifiable copies of model rim and do optional scaling
          int startFinishOptions;
          if((specialOptions_ & 4096) == 0) {
               startFinishOptions = 0;
          } else {
               startFinishOptions = 1 + 2 + 4;
          }
          createRim(7 + 8 + 16 + 32, true, startFinishOptions);
          // Do initial factorization
          // and set certain stuff
          // We can either set increasing rows so ...IsBasic gives pivot row
          // or we can just increment iBasic one by one
          // for now let ...iBasic give pivot row
          bool useFactorization = false;
          if ((startFinishOptions & 2) != 0 && (whatsChanged_&(2 + 512)) == 2 + 512) {
               useFactorization = true; // Keep factorization if possible
               // switch off factorization if bad
               if (pivotVariable_[0] < 0 || factorization_->numberRows() != numberRows_)
                    useFactorization = false;
          }
          if (!useFactorization) {
               factorization_->setDefaultValues();

               int factorizationStatus = internalFactorize(0);
               if (factorizationStatus < 0) {
                    // some error
                    // we should either debug or ignore
#ifndef NDEBUG
                    printf("***** ClpDual strong branching factorization error - debug\n");
#endif
               } else if (factorizationStatus && factorizationStatus <= numberRows_) {
                    handler_->message(CLP_SINGULARITIES, messages_)
                              << factorizationStatus
                              << CoinMessageEol;
               }
          }
     }
     // Get fake bounds correctly
     double dummyChangeCost;
     changeBounds(3, NULL, dummyChangeCost);
     double * arrayD = reinterpret_cast<double *> (arrays);
     arrayD[0] = objectiveValue() * optimizationDirection_;
     double * saveSolution = arrayD + 1;
     double * saveLower = saveSolution + (numberRows + numberColumns);
     double * saveUpper = saveLower + (numberRows + numberColumns);
     double * saveObjective = saveUpper + (numberRows + numberColumns);
     double * saveLowerOriginal = saveObjective + (numberRows + numberColumns);
     double * saveUpperOriginal = saveLowerOriginal + numberColumns;
     arrayD = saveUpperOriginal + numberColumns;
     int * savePivot = reinterpret_cast<int *> (arrayD);
     int * whichRow = savePivot + numberRows;
     int * whichColumn = whichRow + 3 * numberRows;
     int * arrayI = whichColumn + 2 * numberColumns;
     unsigned char * saveStatus = reinterpret_cast<unsigned char *> (arrayI + 1);
     // save stuff
     // save basis and solution
     CoinMemcpyN(solution_,
                 numberRows_ + numberColumns_, saveSolution);
     CoinMemcpyN(status_, numberColumns_ + numberRows_, saveStatus);
     CoinMemcpyN(lower_,
                 numberRows_ + numberColumns_, saveLower);
     CoinMemcpyN(upper_,
                 numberRows_ + numberColumns_, saveUpper);
     CoinMemcpyN(cost_,
                 numberRows_ + numberColumns_, saveObjective);
     CoinMemcpyN(pivotVariable_, numberRows_, savePivot);
     ClpFactorization * factorization = factorization_;
     factorization_ = NULL;
     return factorization;
}
// This cleans up after strong branching
void
ClpSimplexDual::cleanupAfterStrongBranching(ClpFactorization * factorization)
{
     int startFinishOptions;
     /*  COIN_CLP_VETTED
         Looks safe for Cbc
     */
     if((specialOptions_ & 4096) == 0) {
          startFinishOptions = 0;
     } else {
          startFinishOptions = 1 + 2 + 4;
     }
     if ((startFinishOptions & 1) == 0 && cost_) {
          deleteRim(1);
     } else {
          // Original factorization will have been put back by last loop
          delete factorization_;
          factorization_ = factorization;
          //deleteRim(0);
          // mark all as current
     }
     whatsChanged_ &= ~0xffff;
}
/* Checks number of variables at fake bounds.  This is used by fastDual
   so can exit gracefully before end */
int
ClpSimplexDual::numberAtFakeBound()
{
     int iSequence;
     int numberFake = 0;

     for (iSequence = 0; iSequence < numberRows_ + numberColumns_; iSequence++) {
          FakeBound bound = getFakeBound(iSequence);
          switch(getStatus(iSequence)) {

          case basic:
               break;
          case isFree:
          case superBasic:
          case ClpSimplex::isFixed:
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
     //numberFake_ = numberFake;
     return numberFake;
}
/* Pivot out a variable and choose an incoing one.  Assumes dual
   feasible - will not go through a reduced cost.
   Returns step length in theta
   Return codes as before but -1 means no acceptable pivot
*/
int
ClpSimplexDual::pivotResultPart1()
{
  // Get good size for pivot
  // Allow first few iterations to take tiny
  double acceptablePivot = 1.0e-1 * acceptablePivot_;
  if (numberIterations_ > 100)
    acceptablePivot = acceptablePivot_;
  if (factorization_->pivots() > 10)
    acceptablePivot = 1.0e+3 * acceptablePivot_; // if we have iterated be more strict
  else if (factorization_->pivots() > 5)
    acceptablePivot = 1.0e+2 * acceptablePivot_; // if we have iterated be slightly more strict
  else if (factorization_->pivots())
    acceptablePivot = acceptablePivot_; // relax
  // But factorizations complain if <1.0e-8
  //acceptablePivot=CoinMax(acceptablePivot,1.0e-8);
  double bestPossiblePivot = 1.0;
  // get sign for finding row of tableau
  // create as packed
  double direction = directionOut_;
  assert (!rowArray_[0]->getNumElements());
  rowArray_[1]->clear(); //assert (!rowArray_[1]->getNumElements());
  assert (!columnArray_[0]->getNumElements());
  assert (!columnArray_[1]->getNumElements());
  rowArray_[0]->createPacked(1, &pivotRow_, &direction);
  factorization_->updateColumnTranspose(rowArray_[1], rowArray_[0]);
  // Allow to do dualColumn0
  if (numberThreads_ < -1)
    spareIntArray_[0] = 1;
  spareDoubleArray_[0] = acceptablePivot;
  rowArray_[3]->clear();
  sequenceIn_ = -1;
  // put row of tableau in rowArray[0] and columnArray[0]
  assert (!rowArray_[1]->getNumElements());
  if (!scaledMatrix_) {
    if ((moreSpecialOptions_ & 8) != 0 && !rowScale_)
      spareIntArray_[0] = 1;
    matrix_->transposeTimes(this, -1.0,
			    rowArray_[0], rowArray_[1], columnArray_[0]);
  } else {
    double * saveR = rowScale_;
    double * saveC = columnScale_;
    rowScale_ = NULL;
    columnScale_ = NULL;
    if ((moreSpecialOptions_ & 8) != 0)
      spareIntArray_[0] = 1;
    scaledMatrix_->transposeTimes(this, -1.0,
				  rowArray_[0], rowArray_[1], columnArray_[0]);
    rowScale_ = saveR;
    columnScale_ = saveC;
  }
  // do ratio test for normal iteration
  dualOut_ *= 1.0e-8;
  bestPossiblePivot = dualColumn(rowArray_[0], columnArray_[0], rowArray_[3],
				 columnArray_[1], acceptablePivot, 
				 NULL/*dubiousWeights*/);
  dualOut_ *= 1.0e8;
  if (fabs(bestPossiblePivot)<1.0e-6)
    return -1;
  else
    return 0;
}
/*
   Row array has row part of pivot row
   Column array has column part.
   This is used in dual values pass
*/
void
ClpSimplexDual::checkPossibleValuesMove(CoinIndexedVector * rowArray,
                                        CoinIndexedVector * columnArray,
                                        double acceptablePivot)
{
     double * work;
     int number;
     int * which;
     int iSection;

     double tolerance = dualTolerance_ * 1.001;

     double thetaDown = 1.0e31;
     double changeDown ;
     double thetaUp = 1.0e31;
     double bestAlphaDown = acceptablePivot * 0.99999;
     double bestAlphaUp = acceptablePivot * 0.99999;
     int sequenceDown = -1;
     int sequenceUp = sequenceOut_;

     double djBasic = dj_[sequenceOut_];
     if (djBasic > 0.0) {
          // basic at lower bound so directionOut_ 1 and -1 in pivot row
          // dj will go to zero on other way
          thetaUp = djBasic;
          changeDown = -lower_[sequenceOut_];
     } else {
          // basic at upper bound so directionOut_ -1 and 1 in pivot row
          // dj will go to zero on other way
          thetaUp = -djBasic;
          changeDown = upper_[sequenceOut_];
     }
     bestAlphaUp = 1.0;
     int addSequence;

     double alphaUp = 0.0;
     double alphaDown = 0.0;

     for (iSection = 0; iSection < 2; iSection++) {

          int i;
          if (!iSection) {
               work = rowArray->denseVector();
               number = rowArray->getNumElements();
               which = rowArray->getIndices();
               addSequence = numberColumns_;
          } else {
               work = columnArray->denseVector();
               number = columnArray->getNumElements();
               which = columnArray->getIndices();
               addSequence = 0;
          }

          for (i = 0; i < number; i++) {
               int iSequence = which[i];
               int iSequence2 = iSequence + addSequence;
               double alpha;
               double oldValue;
               double value;

               switch(getStatus(iSequence2)) {

               case basic:
                    break;
               case ClpSimplex::isFixed:
                    alpha = work[i];
                    changeDown += alpha * upper_[iSequence2];
                    break;
               case isFree:
               case superBasic:
                    alpha = work[i];
                    // dj must be effectively zero as dual feasible
                    if (fabs(alpha) > bestAlphaUp) {
                         thetaDown = 0.0;
                         thetaUp = 0.0;
                         bestAlphaDown = fabs(alpha);
                         bestAlphaUp = bestAlphaDown;
                         sequenceDown = iSequence2;
                         sequenceUp = sequenceDown;
                         alphaUp = alpha;
                         alphaDown = alpha;
                    }
                    break;
               case atUpperBound:
                    alpha = work[i];
                    oldValue = dj_[iSequence2];
                    changeDown += alpha * upper_[iSequence2];
                    if (alpha >= acceptablePivot) {
                         // might do other way
                         value = oldValue + thetaUp * alpha;
                         if (value > -tolerance) {
                              if (value > tolerance || fabs(alpha) > bestAlphaUp) {
                                   thetaUp = -oldValue / alpha;
                                   bestAlphaUp = fabs(alpha);
                                   sequenceUp = iSequence2;
                                   alphaUp = alpha;
                              }
                         }
                    } else if (alpha <= -acceptablePivot) {
                         // might do this way
                         value = oldValue - thetaDown * alpha;
                         if (value > -tolerance) {
                              if (value > tolerance || fabs(alpha) > bestAlphaDown) {
                                   thetaDown = oldValue / alpha;
                                   bestAlphaDown = fabs(alpha);
                                   sequenceDown = iSequence2;
                                   alphaDown = alpha;
                              }
                         }
                    }
                    break;
               case atLowerBound:
                    alpha = work[i];
                    oldValue = dj_[iSequence2];
                    changeDown += alpha * lower_[iSequence2];
                    if (alpha <= -acceptablePivot) {
                         // might do other way
                         value = oldValue + thetaUp * alpha;
                         if (value < tolerance) {
                              if (value < -tolerance || fabs(alpha) > bestAlphaUp) {
                                   thetaUp = -oldValue / alpha;
                                   bestAlphaUp = fabs(alpha);
                                   sequenceUp = iSequence2;
                                   alphaUp = alpha;
                              }
                         }
                    } else if (alpha >= acceptablePivot) {
                         // might do this way
                         value = oldValue - thetaDown * alpha;
                         if (value < tolerance) {
                              if (value < -tolerance || fabs(alpha) > bestAlphaDown) {
                                   thetaDown = oldValue / alpha;
                                   bestAlphaDown = fabs(alpha);
                                   sequenceDown = iSequence2;
                                   alphaDown = alpha;
                              }
                         }
                    }
                    break;
               }
          }
     }
     thetaUp *= -1.0;
     double changeUp = -thetaUp * changeDown;
     changeDown = -thetaDown * changeDown;
     if (CoinMax(fabs(thetaDown), fabs(thetaUp)) < 1.0e-8) {
          // largest
          if (fabs(alphaDown) < fabs(alphaUp)) {
               sequenceDown = -1;
          }
     }
     // choose
     sequenceIn_ = -1;
     if (changeDown > changeUp && sequenceDown >= 0) {
          theta_ = thetaDown;
          if (fabs(changeDown) < 1.0e30)
               sequenceIn_ = sequenceDown;
          alpha_ = alphaDown;
#ifdef CLP_DEBUG
          if ((handler_->logLevel() & 32))
               printf("predicted way - dirout %d, change %g,%g theta %g\n",
                      directionOut_, changeDown, changeUp, theta_);
#endif
     } else {
          theta_ = thetaUp;
          if (fabs(changeUp) < 1.0e30)
               sequenceIn_ = sequenceUp;
          alpha_ = alphaUp;
          if (sequenceIn_ != sequenceOut_) {
#ifdef CLP_DEBUG
               if ((handler_->logLevel() & 32))
                    printf("opposite way - dirout %d, change %g,%g theta %g\n",
                           directionOut_, changeDown, changeUp, theta_);
#endif
          } else {
#ifdef CLP_DEBUG
               if ((handler_->logLevel() & 32))
                    printf("opposite way to zero dj - dirout %d, change %g,%g theta %g\n",
                           directionOut_, changeDown, changeUp, theta_);
#endif
          }
     }
     if (sequenceIn_ >= 0) {
          lowerIn_ = lower_[sequenceIn_];
          upperIn_ = upper_[sequenceIn_];
          valueIn_ = solution_[sequenceIn_];
          dualIn_ = dj_[sequenceIn_];

          if (alpha_ < 0.0) {
               // as if from upper bound
               directionIn_ = -1;
               upperIn_ = valueIn_;
          } else {
               // as if from lower bound
               directionIn_ = 1;
               lowerIn_ = valueIn_;
          }
     }
}
/*
   Row array has row part of pivot row
   Column array has column part.
   This is used in cleanup
*/
void
ClpSimplexDual::checkPossibleCleanup(CoinIndexedVector * rowArray,
                                     CoinIndexedVector * columnArray,
                                     double acceptablePivot)
{
     double * work;
     int number;
     int * which;
     int iSection;

     double tolerance = dualTolerance_ * 1.001;

     double thetaDown = 1.0e31;
     double thetaUp = 1.0e31;
     double bestAlphaDown = acceptablePivot * 10.0;
     double bestAlphaUp = acceptablePivot * 10.0;
     int sequenceDown = -1;
     int sequenceUp = -1;

     double djSlack = dj_[pivotRow_];
     if (getRowStatus(pivotRow_) == basic)
          djSlack = COIN_DBL_MAX;
     if (fabs(djSlack) < tolerance)
          djSlack = 0.0;
     int addSequence;

     double alphaUp = 0.0;
     double alphaDown = 0.0;
     for (iSection = 0; iSection < 2; iSection++) {

          int i;
          if (!iSection) {
               work = rowArray->denseVector();
               number = rowArray->getNumElements();
               which = rowArray->getIndices();
               addSequence = numberColumns_;
          } else {
               work = columnArray->denseVector();
               number = columnArray->getNumElements();
               which = columnArray->getIndices();
               addSequence = 0;
          }

          for (i = 0; i < number; i++) {
               int iSequence = which[i];
               int iSequence2 = iSequence + addSequence;
               double alpha;
               double oldValue;
               double value;

               switch(getStatus(iSequence2)) {

               case basic:
                    break;
               case ClpSimplex::isFixed:
                    alpha = work[i];
                    if (addSequence) {
		      COIN_DETAIL_PRINT(printf("possible - pivot row %d this %d\n", pivotRow_, iSequence));
                         oldValue = dj_[iSequence2];
                         if (alpha <= -acceptablePivot) {
                              // might do other way
                              value = oldValue + thetaUp * alpha;
                              if (value < tolerance) {
                                   if (value < -tolerance || fabs(alpha) > bestAlphaUp) {
                                        thetaUp = -oldValue / alpha;
                                        bestAlphaUp = fabs(alpha);
                                        sequenceUp = iSequence2;
                                        alphaUp = alpha;
                                   }
                              }
                         } else if (alpha >= acceptablePivot) {
                              // might do this way
                              value = oldValue - thetaDown * alpha;
                              if (value < tolerance) {
                                   if (value < -tolerance || fabs(alpha) > bestAlphaDown) {
                                        thetaDown = oldValue / alpha;
                                        bestAlphaDown = fabs(alpha);
                                        sequenceDown = iSequence2;
                                        alphaDown = alpha;
                                   }
                              }
                         }
                    }
                    break;
               case isFree:
               case superBasic:
                    alpha = work[i];
                    // dj must be effectively zero as dual feasible
                    if (fabs(alpha) > bestAlphaUp) {
                         thetaDown = 0.0;
                         thetaUp = 0.0;
                         bestAlphaDown = fabs(alpha);
                         bestAlphaUp = bestAlphaDown;
                         sequenceDown = iSequence2;
                         sequenceUp = sequenceDown;
                         alphaUp = alpha;
                         alphaDown = alpha;
                    }
                    break;
               case atUpperBound:
                    alpha = work[i];
                    oldValue = dj_[iSequence2];
                    if (alpha >= acceptablePivot) {
                         // might do other way
                         value = oldValue + thetaUp * alpha;
                         if (value > -tolerance) {
                              if (value > tolerance || fabs(alpha) > bestAlphaUp) {
                                   thetaUp = -oldValue / alpha;
                                   bestAlphaUp = fabs(alpha);
                                   sequenceUp = iSequence2;
                                   alphaUp = alpha;
                              }
                         }
                    } else if (alpha <= -acceptablePivot) {
                         // might do this way
                         value = oldValue - thetaDown * alpha;
                         if (value > -tolerance) {
                              if (value > tolerance || fabs(alpha) > bestAlphaDown) {
                                   thetaDown = oldValue / alpha;
                                   bestAlphaDown = fabs(alpha);
                                   sequenceDown = iSequence2;
                                   alphaDown = alpha;
                              }
                         }
                    }
                    break;
               case atLowerBound:
                    alpha = work[i];
                    oldValue = dj_[iSequence2];
                    if (alpha <= -acceptablePivot) {
                         // might do other way
                         value = oldValue + thetaUp * alpha;
                         if (value < tolerance) {
                              if (value < -tolerance || fabs(alpha) > bestAlphaUp) {
                                   thetaUp = -oldValue / alpha;
                                   bestAlphaUp = fabs(alpha);
                                   sequenceUp = iSequence2;
                                   alphaUp = alpha;
                              }
                         }
                    } else if (alpha >= acceptablePivot) {
                         // might do this way
                         value = oldValue - thetaDown * alpha;
                         if (value < tolerance) {
                              if (value < -tolerance || fabs(alpha) > bestAlphaDown) {
                                   thetaDown = oldValue / alpha;
                                   bestAlphaDown = fabs(alpha);
                                   sequenceDown = iSequence2;
                                   alphaDown = alpha;
                              }
                         }
                    }
                    break;
               }
          }
     }
     thetaUp *= -1.0;
     // largest
     if (bestAlphaDown < bestAlphaUp)
          sequenceDown = -1;
     else
          sequenceUp = -1;

     sequenceIn_ = -1;

     if (sequenceDown >= 0) {
          theta_ = thetaDown;
          sequenceIn_ = sequenceDown;
          alpha_ = alphaDown;
#ifdef CLP_DEBUG
          if ((handler_->logLevel() & 32))
               printf("predicted way - dirout %d, theta %g\n",
                      directionOut_, theta_);
#endif
     } else if (sequenceUp >= 0) {
          theta_ = thetaUp;
          sequenceIn_ = sequenceUp;
          alpha_ = alphaUp;
#ifdef CLP_DEBUG
          if ((handler_->logLevel() & 32))
               printf("opposite way - dirout %d,theta %g\n",
                      directionOut_, theta_);
#endif
     }
     if (sequenceIn_ >= 0) {
          lowerIn_ = lower_[sequenceIn_];
          upperIn_ = upper_[sequenceIn_];
          valueIn_ = solution_[sequenceIn_];
          dualIn_ = dj_[sequenceIn_];

          if (alpha_ < 0.0) {
               // as if from upper bound
               directionIn_ = -1;
               upperIn_ = valueIn_;
          } else {
               // as if from lower bound
               directionIn_ = 1;
               lowerIn_ = valueIn_;
          }
     }
}
/*
   This sees if we can move duals in dual values pass.
   This is done before any pivoting
*/
void ClpSimplexDual::doEasyOnesInValuesPass(double * dj)
{
     // Get column copy
     CoinPackedMatrix * columnCopy = matrix();
     // Get a row copy in standard format
     CoinPackedMatrix copy;
     copy.setExtraGap(0.0);
     copy.setExtraMajor(0.0);
     copy.reverseOrderedCopyOf(*columnCopy);
     // get matrix data pointers
     const int * column = copy.getIndices();
     const CoinBigIndex * rowStart = copy.getVectorStarts();
     const int * rowLength = copy.getVectorLengths();
     const double * elementByRow = copy.getElements();
     double tolerance = dualTolerance_ * 1.001;

     int iRow;
#ifdef CLP_DEBUG
     {
          double value5 = 0.0;
          int i;
          for (i = 0; i < numberRows_ + numberColumns_; i++) {
               if (dj[i] < -1.0e-6)
                    value5 += dj[i] * upper_[i];
               else if (dj[i] > 1.0e-6)
                    value5 += dj[i] * lower_[i];
          }
          printf("Values objective Value before %g\n", value5);
     }
#endif
     // for scaled row
     double * scaled = NULL;
     if (rowScale_)
          scaled = new double[numberColumns_];
     for (iRow = 0; iRow < numberRows_; iRow++) {

          int iSequence = iRow + numberColumns_;
          double djBasic = dj[iSequence];
          if (getRowStatus(iRow) == basic && fabs(djBasic) > tolerance) {

               double changeUp ;
               // always -1 in pivot row
               if (djBasic > 0.0) {
                    // basic at lower bound
                    changeUp = -lower_[iSequence];
               } else {
                    // basic at upper bound
                    changeUp = upper_[iSequence];
               }
               bool canMove = true;
               int i;
               const double * thisElements = elementByRow + rowStart[iRow];
               const int * thisIndices = column + rowStart[iRow];
               if (rowScale_) {
                    // scale row
                    double scale = rowScale_[iRow];
                    for (i = 0; i < rowLength[iRow]; i++) {
                         int iColumn = thisIndices[i];
                         double alpha = thisElements[i];
                         scaled[i] = scale * alpha * columnScale_[iColumn];
                    }
                    thisElements = scaled;
               }
               for (i = 0; i < rowLength[iRow]; i++) {
                    int iColumn = thisIndices[i];
                    double alpha = thisElements[i];
                    double oldValue = dj[iColumn];;
                    double value;

                    switch(getStatus(iColumn)) {

                    case basic:
                         if (dj[iColumn] < -tolerance &&
                                   fabs(solution_[iColumn] - upper_[iColumn]) < 1.0e-8) {
                              // at ub
                              changeUp += alpha * upper_[iColumn];
                              // might do other way
                              value = oldValue + djBasic * alpha;
                              if (value > tolerance)
                                   canMove = false;
                         } else if (dj[iColumn] > tolerance &&
                                    fabs(solution_[iColumn] - lower_[iColumn]) < 1.0e-8) {
                              changeUp += alpha * lower_[iColumn];
                              // might do other way
                              value = oldValue + djBasic * alpha;
                              if (value < -tolerance)
                                   canMove = false;
                         } else {
                              canMove = false;
                         }
                         break;
                    case ClpSimplex::isFixed:
                         changeUp += alpha * upper_[iColumn];
                         break;
                    case isFree:
                    case superBasic:
                         canMove = false;
                         break;
                    case atUpperBound:
                         changeUp += alpha * upper_[iColumn];
                         // might do other way
                         value = oldValue + djBasic * alpha;
                         if (value > tolerance)
                              canMove = false;
                         break;
                    case atLowerBound:
                         changeUp += alpha * lower_[iColumn];
                         // might do other way
                         value = oldValue + djBasic * alpha;
                         if (value < -tolerance)
                              canMove = false;
                         break;
                    }
               }
               if (canMove) {
                    if (changeUp * djBasic > 1.0e-12 || fabs(changeUp) < 1.0e-8) {
                         // move
                         for (i = 0; i < rowLength[iRow]; i++) {
                              int iColumn = thisIndices[i];
                              double alpha = thisElements[i];
                              dj[iColumn] += djBasic * alpha;
                         }
                         dj[iSequence] = 0.0;
#ifdef CLP_DEBUG
                         {
                              double value5 = 0.0;
                              int i;
                              for (i = 0; i < numberRows_ + numberColumns_; i++) {
                                   if (dj[i] < -1.0e-6)
                                        value5 += dj[i] * upper_[i];
                                   else if (dj[i] > 1.0e-6)
                                        value5 += dj[i] * lower_[i];
                              }
                              printf("Values objective Value after row %d old dj %g %g\n",
                                     iRow, djBasic, value5);
                         }
#endif
                    }
               }
          }
     }
     delete [] scaled;
}
int
ClpSimplexDual::nextSuperBasic()
{
     if (firstFree_ >= 0) {
          int returnValue = firstFree_;
          int iColumn = firstFree_ + 1;
          for (; iColumn < numberRows_ + numberColumns_; iColumn++) {
               if (getStatus(iColumn) == isFree)
                    if (fabs(dj_[iColumn]) > 1.0e2 * dualTolerance_)
                         break;
          }
          firstFree_ = iColumn;
          if (firstFree_ == numberRows_ + numberColumns_)
               firstFree_ = -1;
          return returnValue;
     } else {
          return -1;
     }
}
void
ClpSimplexDual::resetFakeBounds(int type)
{
     if (type == 0) {
          // put back original bounds and then check
          createRim1(false);
          double dummyChangeCost = 0.0;
          changeBounds(3, NULL, dummyChangeCost);
     } else if (type < 0) {
#ifndef NDEBUG
          // just check
          int nTotal = numberRows_ + numberColumns_;
          double * tempLower = CoinCopyOfArray(lower_, nTotal);
          double * tempUpper = CoinCopyOfArray(upper_, nTotal);
          int iSequence;
          // Get scaled true bounds
          if (columnScale_) {
               for (iSequence = 0; iSequence < numberColumns_; iSequence++) {
                    // lower
                    double value = columnLower_[iSequence];
                    if (value > -1.0e30) {
                         double multiplier = rhsScale_ * inverseColumnScale_[iSequence];
                         value *= multiplier;
                    }
                    tempLower[iSequence] = value;
                    // upper
                    value = columnUpper_[iSequence];
                    if (value < 1.0e30) {
                         double multiplier = rhsScale_ * inverseColumnScale_[iSequence];
                         value *= multiplier;
                    }
                    tempUpper[iSequence] = value;
               }
               for (iSequence = 0; iSequence < numberRows_; iSequence++) {
                    // lower
                    double value = rowLower_[iSequence];
                    if (value > -1.0e30) {
                         double multiplier = rhsScale_ * rowScale_[iSequence];
                         value *= multiplier;
                    }
                    tempLower[iSequence+numberColumns_] = value;
                    // upper
                    value = rowUpper_[iSequence];
                    if (value < 1.0e30) {
                         double multiplier = rhsScale_ * rowScale_[iSequence];
                         value *= multiplier;
                    }
                    tempUpper[iSequence+numberColumns_] = value;
               }
          } else {
               for (iSequence = 0; iSequence < numberColumns_; iSequence++) {
                    // lower
                    tempLower[iSequence] = columnLower_[iSequence];
                    // upper
                    tempUpper[iSequence] = columnUpper_[iSequence];
               }
               for (iSequence = 0; iSequence < numberRows_; iSequence++) {
                    // lower
                    tempLower[iSequence+numberColumns_] = rowLower_[iSequence];
                    // upper
                    tempUpper[iSequence+numberColumns_] = rowUpper_[iSequence];
               }
          }
          int nFake = 0;
          int nErrors = 0;
          int nSuperBasic = 0;
          int nWarnings = 0;
          for (iSequence = 0; iSequence < nTotal; iSequence++) {
               FakeBound fakeStatus = getFakeBound(iSequence);
               Status status = getStatus(iSequence);
               bool isFake = false;
#ifdef CLP_INVESTIGATE
               char RC = 'C';
#endif
               int jSequence = iSequence;
               if (jSequence >= numberColumns_) {
#ifdef CLP_INVESTIGATE
                    RC = 'R';
#endif
                    jSequence -= numberColumns_;
               }
               double lowerValue = tempLower[iSequence];
               double upperValue = tempUpper[iSequence];
               double value = solution_[iSequence];
               CoinRelFltEq equal;
               if (status == atUpperBound ||
                         status == atLowerBound) {
                    if (fakeStatus == ClpSimplexDual::upperFake) {
                         if(!equal(upper_[iSequence], (lowerValue + dualBound_)) ||
                                   !(equal(upper_[iSequence], value) ||
                                     equal(lower_[iSequence], value))) {
                              nErrors++;
#ifdef CLP_INVESTIGATE
                              printf("** upperFake %c%d %g <= %g <= %g true %g, %g\n",
                                     RC, jSequence, lower_[iSequence], solution_[iSequence],
                                     upper_[iSequence], lowerValue, upperValue);
#endif
                         }
                         isFake = true;;
                    } else if (fakeStatus == ClpSimplexDual::lowerFake) {
                         if(!equal(lower_[iSequence], (upperValue - dualBound_)) ||
                                   !(equal(upper_[iSequence], value) ||
                                     equal(lower_[iSequence], value))) {
                              nErrors++;
#ifdef CLP_INVESTIGATE
                              printf("** lowerFake %c%d %g <= %g <= %g true %g, %g\n",
                                     RC, jSequence, lower_[iSequence], solution_[iSequence],
                                     upper_[iSequence], lowerValue, upperValue);
#endif
                         }
                         isFake = true;;
                    } else if (fakeStatus == ClpSimplexDual::bothFake) {
                         nWarnings++;
#ifdef CLP_INVESTIGATE
                         printf("** %d at bothFake?\n", iSequence);
#endif
                    } else if (upper_[iSequence] - lower_[iSequence] > 2.0 * dualBound_) {
                         nErrors++;
#ifdef CLP_INVESTIGATE
                         printf("** noFake! %c%d %g <= %g <= %g true %g, %g\n",
                                RC, jSequence, lower_[iSequence], solution_[iSequence],
                                upper_[iSequence], lowerValue, upperValue);
#endif
                    }
               } else if (status == superBasic || status == isFree) {
                    nSuperBasic++;
                    //printf("** free or superbasic %c%d %g <= %g <= %g true %g, %g - status %d\n",
                    //     RC,jSequence,lower_[iSequence],solution_[iSequence],
                    //     upper_[iSequence],lowerValue,upperValue,status);
               } else if (status == basic) {
                    bool odd = false;
                    if (!equal(lower_[iSequence], lowerValue))
                         odd = true;
                    if (!equal(upper_[iSequence], upperValue))
                         odd = true;
                    if (odd) {
#ifdef CLP_INVESTIGATE
                         printf("** basic %c%d %g <= %g <= %g true %g, %g\n",
                                RC, jSequence, lower_[iSequence], solution_[iSequence],
                                upper_[iSequence], lowerValue, upperValue);
#endif
                         nWarnings++;
                    }
               } else if (status == isFixed) {
                    if (!equal(upper_[iSequence], lower_[iSequence])) {
                         nErrors++;
#ifdef CLP_INVESTIGATE
                         printf("** fixed! %c%d %g <= %g <= %g true %g, %g\n",
                                RC, jSequence, lower_[iSequence], solution_[iSequence],
                                upper_[iSequence], lowerValue, upperValue);
#endif
                    }
               }
               if (isFake) {
                    nFake++;
               } else {
                    if (fakeStatus != ClpSimplexDual::noFake) {
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
               printf("dualBound %g\n",
                      dualBound_);
#endif
               if (type <= -1000) {
                    iSequence = -type;
                    iSequence -= 1000;
#ifdef CLP_INVESTIGATE
                    char RC = 'C';
#endif
                    int jSequence = iSequence;
                    if (jSequence >= numberColumns_) {
#ifdef CLP_INVESTIGATE
                         RC = 'R';
#endif
                         jSequence -= numberColumns_;
                    }
#ifdef CLP_INVESTIGATE
                    double lowerValue = tempLower[iSequence];
                    double upperValue = tempUpper[iSequence];
                    printf("*** movement>1.0e30 for  %c%d %g <= %g <= %g true %g, %g - status %d\n",
                           RC, jSequence, lower_[iSequence], solution_[iSequence],
                           upper_[iSequence], lowerValue, upperValue, status_[iSequence]);
#endif
                    assert (nErrors); // should have been picked up
               }
               assert (!nErrors);
          }
          delete [] tempLower;
          delete [] tempUpper;
#endif
     } else if (lower_) {
          // reset using status
          int nTotal = numberRows_ + numberColumns_;
          int iSequence;
          if (columnScale_) {
               for (iSequence = 0; iSequence < numberColumns_; iSequence++) {
                    double multiplier = rhsScale_ * inverseColumnScale_[iSequence];
                    // lower
                    double value = columnLower_[iSequence];
                    if (value > -1.0e30) {
                         value *= multiplier;
                    }
                    lower_[iSequence] = value;
                    // upper
                    value = columnUpper_[iSequence];
                    if (value < 1.0e30) {
                         value *= multiplier;
                    }
                    upper_[iSequence] = value;
               }
               for (iSequence = 0; iSequence < numberRows_; iSequence++) {
                    // lower
                    double multiplier = rhsScale_ * rowScale_[iSequence];
                    double value = rowLower_[iSequence];
                    if (value > -1.0e30) {
                         value *= multiplier;
                    }
                    lower_[iSequence+numberColumns_] = value;
                    // upper
                    value = rowUpper_[iSequence];
                    if (value < 1.0e30) {
                         value *= multiplier;
                    }
                    upper_[iSequence+numberColumns_] = value;
               }
          } else {
               memcpy(lower_, columnLower_, numberColumns_ * sizeof(double));
               memcpy(upper_, columnUpper_, numberColumns_ * sizeof(double));
               memcpy(lower_ + numberColumns_, rowLower_, numberRows_ * sizeof(double));
               memcpy(upper_ + numberColumns_, rowUpper_, numberRows_ * sizeof(double));
          }
          numberFake_ = 0;
          for (iSequence = 0; iSequence < nTotal; iSequence++) {
               FakeBound fakeStatus = getFakeBound(iSequence);
               if (fakeStatus != ClpSimplexDual::noFake) {
                    Status status = getStatus(iSequence);
                    if (status == basic || status == isFixed) {
                         setFakeBound(iSequence, ClpSimplexDual::noFake);
                         continue;
                    }
                    double lowerValue = lower_[iSequence];
                    double upperValue = upper_[iSequence];
                    double value = solution_[iSequence];
                    numberFake_++;
                    if (fakeStatus == ClpSimplexDual::upperFake) {
		         upper_[iSequence] = lowerValue + dualBound_;
                         if (status == ClpSimplex::atLowerBound) {
			      solution_[iSequence] = lowerValue;
                         } else if (status == ClpSimplex::atUpperBound) {
                              solution_[iSequence] = upper_[iSequence];
                         } else {
			      printf("Unknown status %d for variable %d in %s line %d\n",
				  status,iSequence,__FILE__,__LINE__);
			      abort();
                         }
                    } else if (fakeStatus == ClpSimplexDual::lowerFake) {
		         lower_[iSequence] = upperValue - dualBound_;
                         if (status == ClpSimplex::atLowerBound) {
			      solution_[iSequence] = lower_[iSequence];
                         } else if (status == ClpSimplex::atUpperBound) {
                              solution_[iSequence] = upperValue;
                         } else {
			      printf("Unknown status %d for variable %d in %s line %d\n",
				  status,iSequence,__FILE__,__LINE__);
			      abort();
                         }
		    } else {
		         assert (fakeStatus == ClpSimplexDual::bothFake);
                         if (status == ClpSimplex::atLowerBound) {
                              lower_[iSequence] = value;
                              upper_[iSequence] = value + dualBound_;
                         } else if (status == ClpSimplex::atUpperBound) {
                              upper_[iSequence] = value;
                              lower_[iSequence] = value - dualBound_;
                         } else if (status == ClpSimplex::isFree ||
                                    status == ClpSimplex::superBasic) {
                              lower_[iSequence] = value - 0.5 * dualBound_;
                              upper_[iSequence] = value + 0.5 * dualBound_;
                         } else {
			      printf("Unknown status %d for variable %d in %s line %d\n",
				  status,iSequence,__FILE__,__LINE__);
			      abort();
                         }
		    }
               }
          }
#ifndef NDEBUG
     } else {
       COIN_DETAIL_PRINT(printf("NULL lower\n"));
#endif
     }
}
