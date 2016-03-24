/* $Id$ */
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
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
#include "ClpSimplexPrimal.hpp"
#include "ClpFactorization.hpp"
#include "ClpNonLinearCost.hpp"
#include "CoinPackedMatrix.hpp"
#include "CoinIndexedVector.hpp"
#include "ClpPrimalColumnPivot.hpp"
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
bool userChoiceValid1(const ClpSimplex * model,
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
bool userChoiceValid2(const ClpSimplex * model);
/* If a good pivot then you may wish to unflag some variables.
 */
void userChoiceWasGood(ClpSimplex * model);
#endif
// primal
int ClpSimplexPrimal::primal (int ifValuesPass , int startFinishOptions)
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

     // save data
     ClpDataSave data = saveData();
     matrix_->refresh(this); // make sure matrix okay

     // Save so can see if doing after dual
     int initialStatus = problemStatus_;
     int initialIterations = numberIterations_;
     int initialNegDjs = -1;
     // initialize - maybe values pass and algorithm_ is +1
#if 0
     // if so - put in any superbasic costed slacks
     if (ifValuesPass && specialOptions_ < 0x01000000) {
          // Get column copy
          const CoinPackedMatrix * columnCopy = matrix();
          const int * row = columnCopy->getIndices();
          const CoinBigIndex * columnStart = columnCopy->getVectorStarts();
          const int * columnLength = columnCopy->getVectorLengths();
          //const double * element = columnCopy->getElements();
          int n = 0;
          for (int iColumn = 0; iColumn < numberColumns_; iColumn++) {
               if (columnLength[iColumn] == 1) {
                    Status status = getColumnStatus(iColumn);
                    if (status != basic && status != isFree) {
                         double value = columnActivity_[iColumn];
                         if (fabs(value - columnLower_[iColumn]) > primalTolerance_ &&
                                   fabs(value - columnUpper_[iColumn]) > primalTolerance_) {
                              int iRow = row[columnStart[iColumn]];
                              if (getRowStatus(iRow) == basic) {
                                   setRowStatus(iRow, superBasic);
                                   setColumnStatus(iColumn, basic);
                                   n++;
                              }
                         }
                    }
               }
          }
          printf("%d costed slacks put in basis\n", n);
     }
#endif
     // Start can skip some things in transposeTimes
     specialOptions_ |= 131072;
     if (!startup(ifValuesPass, startFinishOptions)) {

          // Set average theta
          nonLinearCost_->setAverageTheta(1.0e3);
          int lastCleaned = 0; // last time objective or bounds cleaned up

          // Say no pivot has occurred (for steepest edge and updates)
          pivotRow_ = -2;

          // This says whether to restore things etc
          int factorType = 0;
          if (problemStatus_ < 0 && perturbation_ < 100 && !ifValuesPass) {
               perturb(0);
               // Can't get here if values pass
               assert (!ifValuesPass);
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
          ClpSimplex * saveModel = NULL;
          int stopSprint = -1;
          int sprintPass = 0;
          int reasonableSprintIteration = 0;
          int lastSprintIteration = 0;
          double lastObjectiveValue = COIN_DBL_MAX;
          // Start check for cycles
          progress_.fillFromModel(this);
          progress_.startCheck();
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
#if 1
               if (perturbation_ < 101 && numberIterations_ > 2 * (numberRows_ + numberColumns_) && (specialOptions_ & 4) == 0
                         && initialStatus != 10) {
                    perturb(1);
                    matrix_->rhsOffset(this, true, false);
               }
#endif
               // If we have done no iterations - special
               if (lastGoodIteration_ == numberIterations_ && factorType)
                    factorType = 3;
               if (saveModel) {
                    // Doing sprint
                    if (sequenceIn_ < 0 || numberIterations_ >= stopSprint) {
                         problemStatus_ = -1;
                         originalModel(saveModel);
                         saveModel = NULL;
                         if (sequenceIn_ < 0 && numberIterations_ < reasonableSprintIteration &&
                                   sprintPass > 100)
                              primalColumnPivot_->switchOffSprint();
                         //lastSprintIteration=numberIterations_;
                         COIN_DETAIL_PRINT(printf("End small model\n"));
                    }
               }

               // may factorize, checks if problem finished
               statusOfProblemInPrimal(lastCleaned, factorType, &progress_, true, ifValuesPass, saveModel);
               if (initialStatus == 10) {
                    // cleanup phase
                    if(initialIterations != numberIterations_) {
                         if (numberDualInfeasibilities_ > 10000 && numberDualInfeasibilities_ > 10 * initialNegDjs) {
                              // getting worse - try perturbing
                              if (perturbation_ < 101 && (specialOptions_ & 4) == 0) {
                                   perturb(1);
                                   matrix_->rhsOffset(this, true, false);
                                   statusOfProblemInPrimal(lastCleaned, factorType, &progress_, true, ifValuesPass, saveModel);
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
               // See if sprint says redo because of problems
               if (numberDualInfeasibilities_ == -776) {
                    // Need new set of variables
                    problemStatus_ = -1;
                    originalModel(saveModel);
                    saveModel = NULL;
                    //lastSprintIteration=numberIterations_;
                    COIN_DETAIL_PRINT(printf("End small model after\n"));
                    statusOfProblemInPrimal(lastCleaned, factorType, &progress_, true, ifValuesPass, saveModel);
               }
               int numberSprintIterations = 0;
               int numberSprintColumns = primalColumnPivot_->numberSprintColumns(numberSprintIterations);
               if (problemStatus_ == 777) {
                    // problems so do one pass with normal
                    problemStatus_ = -1;
                    originalModel(saveModel);
                    saveModel = NULL;
                    // Skip factorization
                    //statusOfProblemInPrimal(lastCleaned,factorType,&progress_,false,saveModel);
                    statusOfProblemInPrimal(lastCleaned, factorType, &progress_, true, ifValuesPass, saveModel);
               } else if (problemStatus_ < 0 && !saveModel && numberSprintColumns && firstFree_ < 0) {
                    int numberSort = 0;
                    int numberFixed = 0;
                    int numberBasic = 0;
                    reasonableSprintIteration = numberIterations_ + 100;
                    int * whichColumns = new int[numberColumns_];
                    double * weight = new double[numberColumns_];
                    int numberNegative = 0;
                    double sumNegative = 0.0;
                    // now massage weight so all basic in plus good djs
                    for (iColumn = 0; iColumn < numberColumns_; iColumn++) {
                         double dj = dj_[iColumn];
                         switch(getColumnStatus(iColumn)) {

                         case basic:
                              dj = -1.0e50;
                              numberBasic++;
                              break;
                         case atUpperBound:
                              dj = -dj;
                              break;
                         case isFixed:
                              dj = 1.0e50;
                              numberFixed++;
                              break;
                         case atLowerBound:
                              dj = dj;
                              break;
                         case isFree:
                              dj = -100.0 * fabs(dj);
                              break;
                         case superBasic:
                              dj = -100.0 * fabs(dj);
                              break;
                         }
                         if (dj < -dualTolerance_ && dj > -1.0e50) {
                              numberNegative++;
                              sumNegative -= dj;
                         }
                         weight[iColumn] = dj;
                         whichColumns[iColumn] = iColumn;
                    }
                    handler_->message(CLP_SPRINT, messages_)
                              << sprintPass << numberIterations_ - lastSprintIteration << objectiveValue() << sumNegative
                              << numberNegative
                              << CoinMessageEol;
                    sprintPass++;
                    lastSprintIteration = numberIterations_;
                    if (objectiveValue()*optimizationDirection_ > lastObjectiveValue - 1.0e-7 && sprintPass > 5) {
                         // switch off
		      COIN_DETAIL_PRINT(printf("Switching off sprint\n"));
                         primalColumnPivot_->switchOffSprint();
                    } else {
                         lastObjectiveValue = objectiveValue() * optimizationDirection_;
                         // sort
                         CoinSort_2(weight, weight + numberColumns_, whichColumns);
                         numberSort = CoinMin(numberColumns_ - numberFixed, numberBasic + numberSprintColumns);
                         // Sort to make consistent ?
                         std::sort(whichColumns, whichColumns + numberSort);
                         saveModel = new ClpSimplex(this, numberSort, whichColumns);
                         delete [] whichColumns;
                         delete [] weight;
                         // Skip factorization
                         //statusOfProblemInPrimal(lastCleaned,factorType,&progress_,false,saveModel);
                         //statusOfProblemInPrimal(lastCleaned,factorType,&progress_,true,saveModel);
                         stopSprint = numberIterations_ + numberSprintIterations;
                         COIN_DETAIL_PRINT(printf("Sprint with %d columns for %d iterations\n",
						  numberSprintColumns, numberSprintIterations));
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
                         if (perturbation_ < 100)
                              perturb(0);
#endif
                    }
               }
               // Check event
               {
                    int status = eventHandler_->event(ClpEventHandler::endOfFactorization);
                    if (status >= 0) {
		        // if >=100 - then special e.g. unperturb
		        if (status!=101) {
			  problemStatus_ = 5;
			  secondaryStatus_ = ClpEventHandler::endOfFactorization;
			  break;
			} else {
			  unPerturb();
			  continue;
			}
                    }
               }
               // Iterate
               whileIterating(ifValuesPass ? 1 : 0);
          }
     }
     // if infeasible get real values
     //printf("XXXXY final cost %g\n",infeasibilityCost_);
     progress_.initialWeight_ = 0.0;
     if (problemStatus_ == 1 && secondaryStatus_ != 6) {
          infeasibilityCost_ = 0.0;
          createRim(1 + 4);
          delete nonLinearCost_;
          nonLinearCost_ = new ClpNonLinearCost(this);
          nonLinearCost_->checkInfeasibilities(0.0);
          sumPrimalInfeasibilities_ = nonLinearCost_->sumInfeasibilities();
          numberPrimalInfeasibilities_ = nonLinearCost_->numberInfeasibilities();
          // and get good feasible duals
          computeDuals(NULL);
     }
     // Stop can skip some things in transposeTimes
     specialOptions_ &= ~131072;
     // clean up
     unflag();
     finish(startFinishOptions);
     restoreData(data);
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
ClpSimplexPrimal::whileIterating(int valuesOption)
{
     // Say if values pass
     int ifValuesPass = (firstFree_ >= 0) ? 1 : 0;
     int returnCode = -1;
     int superBasicType = 1;
     if (valuesOption > 1)
          superBasicType = 3;
     // delete any rays
     delete [] ray_;
     ray_ = NULL;
     // status stays at -1 while iterating, >=0 finished, -2 to invert
     // status -3 to go to top without an invert
     while (problemStatus_ == -1) {
          //#define CLP_DEBUG 1
#ifdef CLP_DEBUG
          {
               int i;
               // not [1] as has information
               for (i = 0; i < 4; i++) {
                    if (i != 1)
                         rowArray_[i]->checkClear();
               }
               for (i = 0; i < 2; i++) {
                    columnArray_[i]->checkClear();
               }
          }
#endif
#if 0
          {
               int iPivot;
               double * array = rowArray_[3]->denseVector();
               int * index = rowArray_[3]->getIndices();
               int i;
               for (iPivot = 0; iPivot < numberRows_; iPivot++) {
                    int iSequence = pivotVariable_[iPivot];
                    unpackPacked(rowArray_[3], iSequence);
                    factorization_->updateColumn(rowArray_[2], rowArray_[3]);
                    int number = rowArray_[3]->getNumElements();
                    for (i = 0; i < number; i++) {
                         int iRow = index[i];
                         if (iRow == iPivot)
                              assert (fabs(array[i] - 1.0) < 1.0e-4);
                         else
                              assert (fabs(array[i]) < 1.0e-4);
                    }
                    rowArray_[3]->clear();
               }
          }
#endif
#if 0
          nonLinearCost_->checkInfeasibilities(primalTolerance_);
          printf("suminf %g number %d\n", nonLinearCost_->sumInfeasibilities(),
                 nonLinearCost_->numberInfeasibilities());
#endif
#if CLP_DEBUG>2
          // very expensive
          if (numberIterations_ > 0 && numberIterations_ < 100 && !ifValuesPass) {
               handler_->setLogLevel(63);
               double saveValue = objectiveValue_;
               double * saveRow1 = new double[numberRows_];
               double * saveRow2 = new double[numberRows_];
               CoinMemcpyN(rowReducedCost_, numberRows_, saveRow1);
               CoinMemcpyN(rowActivityWork_, numberRows_, saveRow2);
               double * saveColumn1 = new double[numberColumns_];
               double * saveColumn2 = new double[numberColumns_];
               CoinMemcpyN(reducedCostWork_, numberColumns_, saveColumn1);
               CoinMemcpyN(columnActivityWork_, numberColumns_, saveColumn2);
               gutsOfSolution(NULL, NULL, false);
               printf("xxx %d old obj %g, recomputed %g, sum primal inf %g\n",
                      numberIterations_,
                      saveValue, objectiveValue_, sumPrimalInfeasibilities_);
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
          if (!ifValuesPass) {
               // choose column to come in
               // can use pivotRow_ to update weights
               // pass in list of cost changes so can do row updates (rowArray_[1])
               // NOTE rowArray_[0] is used by computeDuals which is a
               // slow way of getting duals but might be used
               primalColumn(rowArray_[1], rowArray_[2], rowArray_[3],
                            columnArray_[0], columnArray_[1]);
          } else {
               // in values pass
               int sequenceIn = nextSuperBasic(superBasicType, columnArray_[0]);
               if (valuesOption > 1)
                    superBasicType = 2;
               if (sequenceIn < 0) {
                    // end of values pass - initialize weights etc
                    handler_->message(CLP_END_VALUES_PASS, messages_)
                              << numberIterations_;
                    primalColumnPivot_->saveWeights(this, 5);
                    problemStatus_ = -2; // factorize now
                    pivotRow_ = -1; // say no weights update
                    returnCode = -4;
                    // Clean up
                    int i;
                    for (i = 0; i < numberRows_ + numberColumns_; i++) {
                         if (getColumnStatus(i) == atLowerBound || getColumnStatus(i) == isFixed)
                              solution_[i] = lower_[i];
                         else if (getColumnStatus(i) == atUpperBound)
                              solution_[i] = upper_[i];
                    }
                    break;
               } else {
                    // normal
                    sequenceIn_ = sequenceIn;
                    valueIn_ = solution_[sequenceIn_];
                    lowerIn_ = lower_[sequenceIn_];
                    upperIn_ = upper_[sequenceIn_];
                    dualIn_ = dj_[sequenceIn_];
               }
          }
          pivotRow_ = -1;
          sequenceOut_ = -1;
          rowArray_[1]->clear();
          if (sequenceIn_ >= 0) {
               // we found a pivot column
               assert (!flagged(sequenceIn_));
#ifdef CLP_DEBUG
               if ((handler_->logLevel() & 32)) {
                    char x = isColumn(sequenceIn_) ? 'C' : 'R';
                    std::cout << "pivot column " <<
                              x << sequenceWithin(sequenceIn_) << std::endl;
               }
#endif
#ifdef CLP_DEBUG
               {
                    int checkSequence = -2077;
                    if (checkSequence >= 0 && checkSequence < numberRows_ + numberColumns_ && !ifValuesPass) {
                         rowArray_[2]->checkClear();
                         rowArray_[3]->checkClear();
                         double * array = rowArray_[3]->denseVector();
                         int * index = rowArray_[3]->getIndices();
                         unpackPacked(rowArray_[3], checkSequence);
                         factorization_->updateColumnForDebug(rowArray_[2], rowArray_[3]);
                         int number = rowArray_[3]->getNumElements();
                         double dualIn = cost_[checkSequence];
                         int i;
                         for (i = 0; i < number; i++) {
                              int iRow = index[i];
                              int iPivot = pivotVariable_[iRow];
                              double alpha = array[i];
                              dualIn -= alpha * cost_[iPivot];
                         }
                         printf("old dj for %d was %g, recomputed %g\n", checkSequence,
                                dj_[checkSequence], dualIn);
                         rowArray_[3]->clear();
                         if (numberIterations_ > 2000)
                              exit(1);
                    }
               }
#endif
               // do second half of iteration
               returnCode = pivotResult(ifValuesPass);
               if (returnCode < -1 && returnCode > -5) {
                    problemStatus_ = -2; //
               } else if (returnCode == -5) {
                    if ((moreSpecialOptions_ & 16) == 0 && factorization_->pivots()) {
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
#ifdef CLP_DEBUG
               if (handler_->logLevel() & 32)
                    printf("** no column pivot\n");
#endif
               if (nonLinearCost_->numberInfeasibilities())
                    problemStatus_ = -4; // might be infeasible
               // Force to re-factorize early next time
               int numberPivots = factorization_->pivots();
	       returnCode = 0;
#ifdef CLP_USER_DRIVEN
	       // If large number of pivots trap later?
	       if (problemStatus_==-1 && numberPivots<1000000) {
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
          columnArray_[0]->setNumElements(0);
     return returnCode;
}
/* Checks if finished.  Updates status */
void
ClpSimplexPrimal::statusOfProblemInPrimal(int & lastCleaned, int type,
          ClpSimplexProgress * progress,
          bool doFactorization,
          int ifValuesPass,
          ClpSimplex * originalModel)
{
     int dummy; // for use in generalExpanded
     int saveFirstFree = firstFree_;
     // number of pivots done
     int numberPivots = factorization_->pivots();
     if (type == 2) {
          // trouble - restore solution
          CoinMemcpyN(saveStatus_, numberColumns_ + numberRows_, status_);
          CoinMemcpyN(savedSolution_ + numberColumns_ ,
                      numberRows_, rowActivityWork_);
          CoinMemcpyN(savedSolution_ ,
                      numberColumns_, columnActivityWork_);
          // restore extra stuff
          matrix_->generalExpanded(this, 6, dummy);
          forceFactorization_ = 1; // a bit drastic but ..
          pivotRow_ = -1; // say no weights update
          changeMade_++; // say change made
     }
     int saveThreshold = factorization_->sparseThreshold();
     int tentativeStatus = problemStatus_;
     int numberThrownOut = 1; // to loop round on bad factorization in values pass
     double lastSumInfeasibility = COIN_DBL_MAX;
     if (numberIterations_)
          lastSumInfeasibility = nonLinearCost_->sumInfeasibilities();
     int nPass = 0;
     while (numberThrownOut) {
          int nSlackBasic = 0;
          if (nPass) {
               for (int i = 0; i < numberRows_; i++) {
                    if (getRowStatus(i) == basic)
                         nSlackBasic++;
               }
          }
          nPass++;
          if (problemStatus_ > -3 || problemStatus_ == -4) {
               // factorize
               // later on we will need to recover from singularities
               // also we could skip if first time
               // do weights
               // This may save pivotRow_ for use
               if (doFactorization)
                    primalColumnPivot_->saveWeights(this, 1);

               if ((type && doFactorization) || nSlackBasic == numberRows_) {
                    // is factorization okay?
                    int factorStatus = internalFactorize(1);
                    if (factorStatus) {
                         if (solveType_ == 2 + 8) {
                              // say odd
                              problemStatus_ = 5;
                              return;
                         }
                         if (type != 1 || largestPrimalError_ > 1.0e3
                                   || largestDualError_ > 1.0e3) {
                              // switch off dense
                              int saveDense = factorization_->denseThreshold();
                              factorization_->setDenseThreshold(0);
                              // Go to safe
                              factorization_->pivotTolerance(0.99);
                              // make sure will do safe factorization
                              pivotVariable_[0] = -1;
                              internalFactorize(2);
                              factorization_->setDenseThreshold(saveDense);
                              // restore extra stuff
                              matrix_->generalExpanded(this, 6, dummy);
                         } else {
                              // no - restore previous basis
                              // Keep any flagged variables
                              int i;
                              for (i = 0; i < numberRows_ + numberColumns_; i++) {
                                   if (flagged(i))
                                        saveStatus_[i] |= 64; //say flagged
                              }
                              CoinMemcpyN(saveStatus_, numberColumns_ + numberRows_, status_);
                              if (numberPivots <= 1) {
                                   // throw out something
                                   if (sequenceIn_ >= 0 && getStatus(sequenceIn_) != basic) {
                                        setFlagged(sequenceIn_);
                                   } else if (sequenceOut_ >= 0 && getStatus(sequenceOut_) != basic) {
                                        setFlagged(sequenceOut_);
                                   }
                                   double newTolerance = CoinMax(0.5 + 0.499 * randomNumberGenerator_.randomDouble(), factorization_->pivotTolerance());
                                   factorization_->pivotTolerance(newTolerance);
                              } else {
                                   // Go to safe
                                   factorization_->pivotTolerance(0.99);
                              }
                              CoinMemcpyN(savedSolution_ + numberColumns_ ,
                                          numberRows_, rowActivityWork_);
                              CoinMemcpyN(savedSolution_ ,
                                          numberColumns_, columnActivityWork_);
                              // restore extra stuff
                              matrix_->generalExpanded(this, 6, dummy);
                              matrix_->generalExpanded(this, 5, dummy);
                              forceFactorization_ = 1; // a bit drastic but ..
                              type = 2;
                              if (internalFactorize(2) != 0) {
                                   largestPrimalError_ = 1.0e4; // force other type
                              }
                         }
                         changeMade_++; // say change made
                    }
               }
               if (problemStatus_ != -4)
                    problemStatus_ = -3;
          }
          // at this stage status is -3 or -5 if looks unbounded
          // get primal and dual solutions
          // put back original costs and then check
          // createRim(4); // costs do not change
          // May need to do more if column generation
          dummy = 4;
          matrix_->generalExpanded(this, 9, dummy);
#ifndef CLP_CAUTION
#define CLP_CAUTION 1
#endif
#if CLP_CAUTION
          double lastAverageInfeasibility = sumDualInfeasibilities_ /
                                            static_cast<double>(numberDualInfeasibilities_ + 10);
#endif
#ifdef CLP_USER_DRIVEN
	  int status = eventHandler_->event(ClpEventHandler::goodFactorization);
	  if (status >= 0) {
	    lastSumInfeasibility = COIN_DBL_MAX;
	  }
#endif
          numberThrownOut = gutsOfSolution(NULL, NULL, (firstFree_ >= 0));
          double sumInfeasibility =  nonLinearCost_->sumInfeasibilities();
          int reason2 = 0;
#if CLP_CAUTION
#if CLP_CAUTION==2
          double test2 = 1.0e5;
#else
          double test2 = 1.0e-1;
#endif
          if (!lastSumInfeasibility && sumInfeasibility &&
                    lastAverageInfeasibility < test2 && numberPivots > 10)
               reason2 = 3;
          if (lastSumInfeasibility < 1.0e-6 && sumInfeasibility > 1.0e-3 &&
                    numberPivots > 10)
               reason2 = 4;
#endif
          if (numberThrownOut)
               reason2 = 1;
          if ((sumInfeasibility > 1.0e7 && sumInfeasibility > 100.0 * lastSumInfeasibility
                    && factorization_->pivotTolerance() < 0.11) ||
                    (largestPrimalError_ > 1.0e10 && largestDualError_ > 1.0e10))
               reason2 = 2;
          if (reason2) {
               problemStatus_ = tentativeStatus;
               doFactorization = true;
               if (numberPivots) {
                    // go back
                    // trouble - restore solution
                    CoinMemcpyN(saveStatus_, numberColumns_ + numberRows_, status_);
                    CoinMemcpyN(savedSolution_ + numberColumns_ ,
                                numberRows_, rowActivityWork_);
                    CoinMemcpyN(savedSolution_ ,
                                numberColumns_, columnActivityWork_);
                    // restore extra stuff
                    matrix_->generalExpanded(this, 6, dummy);
                    if (reason2 < 3) {
                         // Go to safe
                         factorization_->pivotTolerance(CoinMin(0.99, 1.01 * factorization_->pivotTolerance()));
                         forceFactorization_ = 1; // a bit drastic but ..
                    } else if (forceFactorization_ < 0) {
                         forceFactorization_ = CoinMin(numberPivots / 2, 100);
                    } else {
                         forceFactorization_ = CoinMin(forceFactorization_,
                                                       CoinMax(3, numberPivots / 2));
                    }
                    pivotRow_ = -1; // say no weights update
                    changeMade_++; // say change made
                    if (numberPivots == 1) {
                         // throw out something
                         if (sequenceIn_ >= 0 && getStatus(sequenceIn_) != basic) {
                              setFlagged(sequenceIn_);
                         } else if (sequenceOut_ >= 0 && getStatus(sequenceOut_) != basic) {
                              setFlagged(sequenceOut_);
                         }
                    }
                    type = 2; // so will restore weights
                    if (internalFactorize(2) != 0) {
                         largestPrimalError_ = 1.0e4; // force other type
                    }
                    numberPivots = 0;
                    numberThrownOut = gutsOfSolution(NULL, NULL, (firstFree_ >= 0));
                    assert (!numberThrownOut);
                    sumInfeasibility =  nonLinearCost_->sumInfeasibilities();
               }
          }
     }
     // Double check reduced costs if no action
     if (progress->lastIterationNumber(0) == numberIterations_) {
          if (primalColumnPivot_->looksOptimal()) {
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
     if (type != 2 && !ifValuesPass)
          loop = progress->looping();
     else
          loop = -1;
     if (loop >= 0) {
          if (!problemStatus_) {
               // declaring victory
               numberPrimalInfeasibilities_ = 0;
               sumPrimalInfeasibilities_ = 0.0;
          } else {
               problemStatus_ = loop; //exit if in loop
               problemStatus_ = 10; // instead - try other algorithm
               numberPrimalInfeasibilities_ = nonLinearCost_->numberInfeasibilities();
          }
          problemStatus_ = 10; // instead - try other algorithm
          return ;
     } else if (loop < -1) {
          // Is it time for drastic measures
          if (nonLinearCost_->numberInfeasibilities() && progress->badTimes() > 5 &&
                    progress->oddState() < 10 && progress->oddState() >= 0) {
               progress->newOddState();
               nonLinearCost_->zapCosts();
          }
          // something may have changed
          gutsOfSolution(NULL, NULL, ifValuesPass != 0);
     }
     // If progress then reset costs
     if (loop == -1 && !nonLinearCost_->numberInfeasibilities() && progress->oddState() < 0) {
          createRim(4, false); // costs back
          delete nonLinearCost_;
          nonLinearCost_ = new ClpNonLinearCost(this);
          progress->endOddState();
          gutsOfSolution(NULL, NULL, ifValuesPass != 0);
     }
     // Flag to say whether to go to dual to clean up
     bool goToDual = false;
     // really for free variables in
     //if((progressFlag_&2)!=0)
     //problemStatus_=-1;;
     progressFlag_ = 0; //reset progress flag

     handler_->message(CLP_SIMPLEX_STATUS, messages_)
               << numberIterations_ << nonLinearCost_->feasibleReportCost();
     handler_->printing(nonLinearCost_->numberInfeasibilities() > 0)
               << nonLinearCost_->sumInfeasibilities() << nonLinearCost_->numberInfeasibilities();
     handler_->printing(sumDualInfeasibilities_ > 0.0)
               << sumDualInfeasibilities_ << numberDualInfeasibilities_;
     handler_->printing(numberDualInfeasibilitiesWithoutFree_
                        < numberDualInfeasibilities_)
               << numberDualInfeasibilitiesWithoutFree_;
     handler_->message() << CoinMessageEol;
     if (!primalFeasible()) {
          nonLinearCost_->checkInfeasibilities(primalTolerance_);
          gutsOfSolution(NULL, NULL, ifValuesPass != 0);
          nonLinearCost_->checkInfeasibilities(primalTolerance_);
     }
     if (nonLinearCost_->numberInfeasibilities() > 0 && !progress->initialWeight_ && !ifValuesPass && infeasibilityCost_ == 1.0e10) {
          // first time infeasible - start up weight computation
          double * oldDj = dj_;
          double * oldCost = cost_;
          int numberRows2 = numberRows_ + numberExtraRows_;
          int numberTotal = numberRows2 + numberColumns_;
          dj_ = new double[numberTotal];
          cost_ = new double[numberTotal];
          reducedCostWork_ = dj_;
          rowReducedCost_ = dj_ + numberColumns_;
          objectiveWork_ = cost_;
          rowObjectiveWork_ = cost_ + numberColumns_;
          double direction = optimizationDirection_ * objectiveScale_;
          const double * obj = objective();
          memset(rowObjectiveWork_, 0, numberRows_ * sizeof(double));
          int iSequence;
          if (columnScale_)
               for (iSequence = 0; iSequence < numberColumns_; iSequence++)
                    cost_[iSequence] = obj[iSequence] * direction * columnScale_[iSequence];
          else
               for (iSequence = 0; iSequence < numberColumns_; iSequence++)
                    cost_[iSequence] = obj[iSequence] * direction;
          computeDuals(NULL);
          int numberSame = 0;
          int numberDifferent = 0;
          int numberZero = 0;
          int numberFreeSame = 0;
          int numberFreeDifferent = 0;
          int numberFreeZero = 0;
          int n = 0;
          for (iSequence = 0; iSequence < numberTotal; iSequence++) {
               if (getStatus(iSequence) != basic && !flagged(iSequence)) {
                    // not basic
                    double distanceUp = upper_[iSequence] - solution_[iSequence];
                    double distanceDown = solution_[iSequence] - lower_[iSequence];
                    double feasibleDj = dj_[iSequence];
                    double infeasibleDj = oldDj[iSequence] - feasibleDj;
                    double value = feasibleDj * infeasibleDj;
                    if (distanceUp > primalTolerance_) {
                         // Check if "free"
                         if (distanceDown > primalTolerance_) {
                              // free
                              if (value > dualTolerance_) {
                                   numberFreeSame++;
                              } else if(value < -dualTolerance_) {
                                   numberFreeDifferent++;
                                   dj_[n++] = feasibleDj / infeasibleDj;
                              } else {
                                   numberFreeZero++;
                              }
                         } else {
                              // should not be negative
                              if (value > dualTolerance_) {
                                   numberSame++;
                              } else if(value < -dualTolerance_) {
                                   numberDifferent++;
                                   dj_[n++] = feasibleDj / infeasibleDj;
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
                              dj_[n++] = feasibleDj / infeasibleDj;
                         } else {
                              numberZero++;
                         }
                    }
               }
               progress->initialWeight_ = -1.0;
          }
          //printf("XXXX %d same, %d different, %d zero, -- free %d %d %d\n",
          //   numberSame,numberDifferent,numberZero,
          //   numberFreeSame,numberFreeDifferent,numberFreeZero);
          // we want most to be same
          if (n) {
               double most = 0.95;
               std::sort(dj_, dj_ + n);
               int which = static_cast<int> ((1.0 - most) * static_cast<double> (n));
               double take = -dj_[which] * infeasibilityCost_;
               //printf("XXXXZ inf cost %g take %g (range %g %g)\n",infeasibilityCost_,take,-dj_[0]*infeasibilityCost_,-dj_[n-1]*infeasibilityCost_);
               take = -dj_[0] * infeasibilityCost_;
               infeasibilityCost_ = CoinMin(CoinMax(1000.0 * take, 1.0e8), 1.0000001e10);;
               //printf("XXXX increasing weight to %g\n",infeasibilityCost_);
          }
          delete [] dj_;
          delete [] cost_;
          dj_ = oldDj;
          cost_ = oldCost;
          reducedCostWork_ = dj_;
          rowReducedCost_ = dj_ + numberColumns_;
          objectiveWork_ = cost_;
          rowObjectiveWork_ = cost_ + numberColumns_;
          if (n||matrix_->type()>=15)
               gutsOfSolution(NULL, NULL, ifValuesPass != 0);
     }
     double trueInfeasibility = nonLinearCost_->sumInfeasibilities();
     if (!nonLinearCost_->numberInfeasibilities() && infeasibilityCost_ == 1.0e10 && !ifValuesPass && true) {
          // relax if default
          infeasibilityCost_ = CoinMin(CoinMax(100.0 * sumDualInfeasibilities_, 1.0e8), 1.00000001e10);
          // reset looping criterion
          progress->reset();
          trueInfeasibility = 1.123456e10;
     }
     if (trueInfeasibility > 1.0) {
          // If infeasibility going up may change weights
          double testValue = trueInfeasibility - 1.0e-4 * (10.0 + trueInfeasibility);
          double lastInf = progress->lastInfeasibility(1);
          double lastInf3 = progress->lastInfeasibility(3);
          double thisObj = progress->lastObjective(0);
          double thisInf = progress->lastInfeasibility(0);
          thisObj += infeasibilityCost_ * 2.0 * thisInf;
          double lastObj = progress->lastObjective(1);
          lastObj += infeasibilityCost_ * 2.0 * lastInf;
          double lastObj3 = progress->lastObjective(3);
          lastObj3 += infeasibilityCost_ * 2.0 * lastInf3;
          if (lastObj < thisObj - 1.0e-5 * CoinMax(fabs(thisObj), fabs(lastObj)) - 1.0e-7
                    && firstFree_ < 0) {
               if (handler_->logLevel() == 63)
                    printf("lastobj %g this %g force %d\n", lastObj, thisObj, forceFactorization_);
               int maxFactor = factorization_->maximumPivots();
               if (maxFactor > 10) {
                    if (forceFactorization_ < 0)
                         forceFactorization_ = maxFactor;
                    forceFactorization_ = CoinMax(1, (forceFactorization_ >> 2));
                    if (handler_->logLevel() == 63)
                         printf("Reducing factorization frequency to %d\n", forceFactorization_);
               }
          } else if (lastObj3 < thisObj - 1.0e-5 * CoinMax(fabs(thisObj), fabs(lastObj3)) - 1.0e-7
                     && firstFree_ < 0) {
               if (handler_->logLevel() == 63)
                    printf("lastobj3 %g this3 %g force %d\n", lastObj3, thisObj, forceFactorization_);
               int maxFactor = factorization_->maximumPivots();
               if (maxFactor > 10) {
                    if (forceFactorization_ < 0)
                         forceFactorization_ = maxFactor;
                    forceFactorization_ = CoinMax(1, (forceFactorization_ * 2) / 3);
                    if (handler_->logLevel() == 63)
                         printf("Reducing factorization frequency to %d\n", forceFactorization_);
               }
          } else if(lastInf < testValue || trueInfeasibility == 1.123456e10) {
               if (infeasibilityCost_ < 1.0e14) {
                    infeasibilityCost_ *= 1.5;
                    // reset looping criterion
                    progress->reset();
                    if (handler_->logLevel() == 63)
                         printf("increasing weight to %g\n", infeasibilityCost_);
                    gutsOfSolution(NULL, NULL, ifValuesPass != 0);
               }
          }
     }
     // we may wish to say it is optimal even if infeasible
     bool alwaysOptimal = (specialOptions_ & 1) != 0;
     // give code benefit of doubt
     if (sumOfRelaxedDualInfeasibilities_ == 0.0 &&
               sumOfRelaxedPrimalInfeasibilities_ == 0.0) {
          // say optimal (with these bounds etc)
          numberDualInfeasibilities_ = 0;
          sumDualInfeasibilities_ = 0.0;
          numberPrimalInfeasibilities_ = 0;
          sumPrimalInfeasibilities_ = 0.0;
          // But check if in sprint
          if (originalModel) {
               // Carry on and re-do
               numberDualInfeasibilities_ = -776;
          }
          // But if real primal infeasibilities nonzero carry on
          if (nonLinearCost_->numberInfeasibilities()) {
               // most likely to happen if infeasible
               double relaxedToleranceP = primalTolerance_;
               // we can't really trust infeasibilities if there is primal error
               double error = CoinMin(1.0e-2, largestPrimalError_);
               // allow tolerance at least slightly bigger than standard
               relaxedToleranceP = relaxedToleranceP +  error;
               int ninfeas = nonLinearCost_->numberInfeasibilities();
               double sum = nonLinearCost_->sumInfeasibilities();
               double average = sum / static_cast<double> (ninfeas);
#ifdef COIN_DEVELOP
               if (handler_->logLevel() > 0)
                    printf("nonLinearCost says infeasible %d summing to %g\n",
                           ninfeas, sum);
#endif
               if (average > relaxedToleranceP) {
                    sumOfRelaxedPrimalInfeasibilities_ = sum;
                    numberPrimalInfeasibilities_ = ninfeas;
                    sumPrimalInfeasibilities_ = sum;
#ifdef COIN_DEVELOP
                    bool unflagged =
#endif
                         unflag();
#ifdef COIN_DEVELOP
                    if (unflagged && handler_->logLevel() > 0)
                         printf(" - but flagged variables\n");
#endif
               }
          }
     }
     // had ||(type==3&&problemStatus_!=-5) -- ??? why ????
     if ((dualFeasible() || problemStatus_ == -4) && !ifValuesPass) {
          // see if extra helps
          if (nonLinearCost_->numberInfeasibilities() &&
                    (nonLinearCost_->sumInfeasibilities() > 1.0e-3 || sumOfRelaxedPrimalInfeasibilities_)
                    && !alwaysOptimal) {
               //may need infeasiblity cost changed
               // we can see if we can construct a ray
               // make up a new objective
               double saveWeight = infeasibilityCost_;
               // save nonlinear cost as we are going to switch off costs
               ClpNonLinearCost * nonLinear = nonLinearCost_;
               // do twice to make sure Primal solution has settled
               // put non-basics to bounds in case tolerance moved
               // put back original costs
               createRim(4);
               nonLinearCost_->checkInfeasibilities(0.0);
               gutsOfSolution(NULL, NULL, ifValuesPass != 0);

               infeasibilityCost_ = 1.0e100;
               // put back original costs
               createRim(4);
               nonLinearCost_->checkInfeasibilities(primalTolerance_);
               // may have fixed infeasibilities - double check
               if (nonLinearCost_->numberInfeasibilities() == 0) {
                    // carry on
                    problemStatus_ = -1;
                    infeasibilityCost_ = saveWeight;
                    nonLinearCost_->checkInfeasibilities(primalTolerance_);
               } else {
                    nonLinearCost_ = NULL;
                    // scale
                    int i;
                    for (i = 0; i < numberRows_ + numberColumns_; i++)
                         cost_[i] *= 1.0e-95;
                    gutsOfSolution(NULL, NULL, ifValuesPass != 0);
                    nonLinearCost_ = nonLinear;
                    infeasibilityCost_ = saveWeight;
                    if ((infeasibilityCost_ >= 1.0e18 ||
                              numberDualInfeasibilities_ == 0) && perturbation_ == 101) {
                         goToDual = unPerturb(); // stop any further perturbation
                         if (nonLinearCost_->sumInfeasibilities() > 1.0e-1)
			   goToDual = false;
                         nonLinearCost_->checkInfeasibilities(primalTolerance_);
                         numberDualInfeasibilities_ = 1; // carry on
                         problemStatus_ = -1;
                    } else if (numberDualInfeasibilities_ == 0 && largestDualError_ > 1.0e-2 &&
                               (moreSpecialOptions_ & (256|8192)) == 0) {
                         goToDual = true;
                         factorization_->pivotTolerance(CoinMax(0.9, factorization_->pivotTolerance()));
                    }
                    if (!goToDual) {
                         if (infeasibilityCost_ >= 1.0e20 ||
                                   numberDualInfeasibilities_ == 0) {
                              // we are infeasible - use as ray
                              delete [] ray_;
                              ray_ = new double [numberRows_];
			      // swap sign
			      for (int i=0;i<numberRows_;i++) 
				ray_[i] = -dual_[i];
#ifdef PRINT_RAY_METHOD
			      printf("Primal creating infeasibility ray\n");
#endif
                              // and get feasible duals
                              infeasibilityCost_ = 0.0;
                              createRim(4);
                              nonLinearCost_->checkInfeasibilities(primalTolerance_);
                              gutsOfSolution(NULL, NULL, ifValuesPass != 0);
                              // so will exit
                              infeasibilityCost_ = 1.0e30;
                              // reset infeasibilities
                              sumPrimalInfeasibilities_ = nonLinearCost_->sumInfeasibilities();;
                              numberPrimalInfeasibilities_ =
                                   nonLinearCost_->numberInfeasibilities();
                         }
                         if (infeasibilityCost_ < 1.0e20) {
                              infeasibilityCost_ *= 5.0;
                              // reset looping criterion
                              progress->reset();
                              changeMade_++; // say change made
                              handler_->message(CLP_PRIMAL_WEIGHT, messages_)
                                        << infeasibilityCost_
                                        << CoinMessageEol;
                              // put back original costs and then check
                              createRim(4);
                              nonLinearCost_->checkInfeasibilities(0.0);
                              gutsOfSolution(NULL, NULL, ifValuesPass != 0);
                              problemStatus_ = -1; //continue
                              goToDual = false;
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
                    if ((numberRows_ > 20000 || numberDualInfeasibilities_) && !numberTimesOptimal_)
                         goToDual = false; // Better to carry on a bit longer
                    lastCleaned = -1; // carry on
               }
               bool unflagged = (unflag() != 0);
               if ( lastCleaned != numberIterations_ || unflagged) {
                    handler_->message(CLP_PRIMAL_OPTIMAL, messages_)
                              << primalTolerance_
                              << CoinMessageEol;
                    if (numberTimesOptimal_ < 4) {
                         numberTimesOptimal_++;
                         changeMade_++; // say change made
                         if (numberTimesOptimal_ == 1) {
                              // better to have small tolerance even if slower
                              factorization_->zeroTolerance(CoinMin(factorization_->zeroTolerance(), 1.0e-15));
                         }
                         lastCleaned = numberIterations_;
                         if (primalTolerance_ != dblParam_[ClpPrimalTolerance])
                              handler_->message(CLP_PRIMAL_ORIGINAL, messages_)
                                        << CoinMessageEol;
                         double oldTolerance = primalTolerance_;
                         primalTolerance_ = dblParam_[ClpPrimalTolerance];
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
                         // put back original costs and then check
                         createRim(4);
                         nonLinearCost_->checkInfeasibilities(oldTolerance);
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
                         gutsOfSolution(NULL, NULL, ifValuesPass != 0);
                         if (sumOfRelaxedDualInfeasibilities_ == 0.0 &&
                                   sumOfRelaxedPrimalInfeasibilities_ == 0.0) {
                              // say optimal (with these bounds etc)
                              numberDualInfeasibilities_ = 0;
                              sumDualInfeasibilities_ = 0.0;
                              numberPrimalInfeasibilities_ = 0;
                              sumPrimalInfeasibilities_ = 0.0;
                         }
                         if (dualFeasible() && !nonLinearCost_->numberInfeasibilities() && lastCleaned >= 0)
                              problemStatus_ = 0;
                         else
                              problemStatus_ = -1;
                    } else {
                         problemStatus_ = 0; // optimal
                         if (lastCleaned < numberIterations_) {
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
               if (nonLinearCost_->numberInfeasibilities()) {
                    if (infeasibilityCost_ > 1.0e18 && perturbation_ == 101) {
                         // back off weight
                         infeasibilityCost_ = 1.0e13;
                         // reset looping criterion
                         progress->reset();
                         unPerturb(); // stop any further perturbation
                    }
                    //we need infeasiblity cost changed
                    if (infeasibilityCost_ < 1.0e20) {
                         infeasibilityCost_ *= 5.0;
                         // reset looping criterion
                         progress->reset();
                         changeMade_++; // say change made
                         handler_->message(CLP_PRIMAL_WEIGHT, messages_)
                                   << infeasibilityCost_
                                   << CoinMessageEol;
                         // put back original costs and then check
                         createRim(4);
                         gutsOfSolution(NULL, NULL, ifValuesPass != 0);
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
                    if (sumDualInfeasibilities_ < 1.0e-3 ||
                              (sumDualInfeasibilities_ / static_cast<double> (numberDualInfeasibilities_)) < 1.0e-5 ||
                              progress->lastIterationNumber(0) == numberIterations_) {
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
          double objVal = (nonLinearCost_->feasibleCost()
			+ objective_->nonlinearOffset());
	  objVal /= (objectiveScale_ * rhsScale_);
          double tol = 1.0e-10 * CoinMax(fabs(objVal), fabs(objectiveValue_)) + 1.0e-8;
          if (fabs(objVal - objectiveValue_) > tol) {
#ifdef COIN_DEVELOP
               if (handler_->logLevel() > 0)
                    printf("nonLinearCost has feasible obj of %g, objectiveValue_ is %g\n",
                           objVal, objectiveValue_);
#endif
               objectiveValue_ = objVal;
          }
     }
     // save extra stuff
     matrix_->generalExpanded(this, 5, dummy);
     if (type == 0 || type == 1) {
          if (type != 1 || !saveStatus_) {
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
     if ((moreSpecialOptions_ & 256) != 0)
       goToDual=false;
     if (goToDual || (numberIterations_ > 1000 && largestPrimalError_ > 1.0e6
                      && largestDualError_ > 1.0e6)) {
          problemStatus_ = 10; // try dual
          // See if second call
          if ((moreSpecialOptions_ & 256) != 0||nonLinearCost_->sumInfeasibilities()>1.0e2) {
               numberPrimalInfeasibilities_ = nonLinearCost_->numberInfeasibilities();
               sumPrimalInfeasibilities_ = nonLinearCost_->sumInfeasibilities();
               // say infeasible
               if (numberPrimalInfeasibilities_)
                    problemStatus_ = 1;
          }
     }
     // make sure first free monotonic
     if (firstFree_ >= 0 && saveFirstFree >= 0) {
          firstFree_ = (numberIterations_) ? saveFirstFree : -1;
          nextSuperBasic(1, NULL);
     }
     if (doFactorization) {
          // restore weights (if saved) - also recompute infeasibility list
          if (tentativeStatus > -3)
               primalColumnPivot_->saveWeights(this, (type < 2) ? 2 : 4);
          else
               primalColumnPivot_->saveWeights(this, 3);
          if (saveThreshold) {
               // use default at present
               factorization_->sparseThreshold(0);
               factorization_->goSparse();
          }
     }
     // Allow matrices to be sorted etc
     int fake = -999; // signal sort
     matrix_->correctSequence(this, fake, fake);
}
/*
   Row array has pivot column
   This chooses pivot row.
   For speed, we may need to go to a bucket approach when many
   variables go through bounds
   On exit rhsArray will have changes in costs of basic variables
*/
void
ClpSimplexPrimal::primalRow(CoinIndexedVector * rowArray,
                            CoinIndexedVector * rhsArray,
                            CoinIndexedVector * spareArray,
                            int valuesPass)
{
     double saveDj = dualIn_;
     if (valuesPass && objective_->type() < 2) {
          dualIn_ = cost_[sequenceIn_];

          double * work = rowArray->denseVector();
          int number = rowArray->getNumElements();
          int * which = rowArray->getIndices();

          int iIndex;
          for (iIndex = 0; iIndex < number; iIndex++) {

               int iRow = which[iIndex];
               double alpha = work[iIndex];
               int iPivot = pivotVariable_[iRow];
               dualIn_ -= alpha * cost_[iPivot];
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
     if (factorization_->pivots() > 10)
          acceptablePivot = 1.0e+3 * acceptablePivot_; // if we have iterated be more strict
     else if (factorization_->pivots() > 5)
          acceptablePivot = 1.0e+2 * acceptablePivot_; // if we have iterated be slightly more strict
     else if (factorization_->pivots())
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

     double averageTheta = nonLinearCost_->averageTheta();
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
     int pivotOne = -1;
     //#define CLP_DEBUG
#ifdef CLP_DEBUG
     if (numberIterations_ == -3839 || numberIterations_ == -3840) {
          double dj = cost_[sequenceIn_];
          printf("cost in on %d is %g, dual in %g\n", sequenceIn_, dj, dualIn_);
          for (iIndex = 0; iIndex < number; iIndex++) {

               int iRow = which[iIndex];
               double alpha = work[iIndex];
               int iPivot = pivotVariable_[iRow];
               dj -= alpha * cost_[iPivot];
               printf("row %d var %d current %g %g %g, alpha %g so sol => %g (cost %g, dj %g)\n",
                      iRow, iPivot, lower_[iPivot], solution_[iPivot], upper_[iPivot],
                      alpha, solution_[iPivot] - 1.0e9 * alpha, cost_[iPivot], dj);
          }
     }
#endif
     while (true) {
          pivotOne = -1;
          totalThru = 0.0;
          // We also re-compute reduced cost
          numberRemaining = 0;
          dualIn_ = cost_[sequenceIn_];
#ifndef NDEBUG
          //double tolerance = primalTolerance_ * 1.002;
#endif
          for (iIndex = 0; iIndex < number; iIndex++) {

               int iRow = which[iIndex];
               double alpha = work[iIndex];
               int iPivot = pivotVariable_[iRow];
               if (cost_[iPivot])
                    dualIn_ -= alpha * cost_[iPivot];
               alpha *= way;
               double oldValue = solution_[iPivot];
               // get where in bound sequence
               // note that after this alpha is actually fabs(alpha)
               bool possible;
               // do computation same way as later on in primal
               if (alpha > 0.0) {
                    // basic variable going towards lower bound
                    double bound = lower_[iPivot];
                    // must be exactly same as when used
                    double change = tentativeTheta * alpha;
                    possible = (oldValue - change) <= bound + primalTolerance_;
                    oldValue -= bound;
               } else {
                    // basic variable going towards upper bound
                    double bound = upper_[iPivot];
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
					 upperTheta,alpha,work[iIndex]*way))
		      value =0.0; // say can't use
#endif
                    if (value < -primalTolerance_ && alpha >= acceptablePivot) {
                         upperTheta = (oldValue + primalTolerance_) / alpha;
                         pivotOne = numberRemaining;
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
          if (pivotOne >= 0 && 0) {
               double thruCost = infeasibilityCost_ * spare[pivotOne];
               if (thruCost >= 0.99 * fabs(dualIn_))
                    COIN_DETAIL_PRINT(printf("Could pivot on %d as change %g dj %g\n",
					     index[pivotOne], thruCost, dualIn_));
               double alpha = spare[pivotOne];
               double oldValue = rhs[pivotOne];
               theta_ = oldValue / alpha;
               pivotRow_ = pivotOne;
               // Stop loop
               iTry = MAXTRY;
          }

          // first get ratio with tolerance
          for ( ; iTry < MAXTRY; iTry++) {

               upperTheta = maximumMovement;
               int iBest = -1;
               for (iIndex = 0; iIndex < numberRemaining; iIndex++) {

                    double alpha = spare[iIndex];
                    double oldValue = rhs[iIndex];
                    double value = oldValue - upperTheta * alpha;

#ifdef CLP_USER_DRIVEN1
		    int sequenceOut=pivotVariable_[index[iIndex]];
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
                         double trueAlpha = way * work[indexPoint[iIndex]];
                         totalThru += nonLinearCost_->changeInCost(pivotVariable_[iRow], trueAlpha, rhs[iIndex]);
                         setActive(iRow);
                         if (alpha > bestPivot) {
                              bestPivot = alpha;
                              theta_ = oldValue / bestPivot;
                              pivotRow_ = iIndex;
                         } else if (alpha < acceptablePivot
#ifdef CLP_USER_DRIVEN1
		      ||!userChoiceValid1(this,pivotVariable_[index[iIndex]],
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
                    if (sumInfeasibilities > primalTolerance_ && !nonLinearCost_->numberInfeasibilities()) {
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
          if (nonLinearCost_->numberInfeasibilities()) {
               // but infeasible??
               // move variable but don't pivot
               tentativeTheta = 1.0e50;
               for (iIndex = 0; iIndex < number; iIndex++) {
                    int iRow = which[iIndex];
                    double alpha = work[iIndex];
                    int iPivot = pivotVariable_[iRow];
                    alpha *= way;
                    double oldValue = solution_[iPivot];
                    // get where in bound sequence
                    // note that after this alpha is actually fabs(alpha)
                    if (alpha > 0.0) {
                         // basic variable going towards lower bound
                         double bound = lower_[iPivot];
                         oldValue -= bound;
                    } else {
                         // basic variable going towards upper bound
                         double bound = upper_[iPivot];
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
          int position = pivotRow_; // position in list
          pivotRow_ = index[position];
          alpha_ = work[indexPoint[position]];
          // translate to sequence
          sequenceOut_ = pivotVariable_[pivotRow_];
          valueOut_ = solution(sequenceOut_);
          lowerOut_ = lower_[sequenceOut_];
          upperOut_ = upper_[sequenceOut_];
#define MINIMUMTHETA 1.0e-12
          // Movement should be minimum for anti-degeneracy - unless
          // fixed variable out
          double minimumTheta;
          if (upperOut_ > lowerOut_)
               minimumTheta = MINIMUMTHETA;
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
#ifdef CLP_DEBUG
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
                    upperOut_ = nonLinearCost_->nearest(sequenceOut_, newValue);
               } else {
                    upperOut_ = newValue;
               }
          } else {
               directionOut_ = 1;    // to lower bound
               if (fabs(theta_) > 1.0e-6 || (specialOptions_ & 4) != 0) {
                    lowerOut_ = nonLinearCost_->nearest(sequenceOut_, newValue);
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
     double theta2 = numberIterations_ * nonLinearCost_->averageTheta();
     // Set average theta
     nonLinearCost_->setAverageTheta((theta1 + theta2) / (static_cast<double> (numberIterations_ + 1)));
     //if (numberIterations_%1000==0)
     //printf("average theta is %g\n",nonLinearCost_->averageTheta());

     // clear arrays

     CoinZeroN(spare, numberRemaining);

     // put back original bounds etc
     CoinMemcpyN(index, numberRemaining, rhsArray->getIndices());
     rhsArray->setNumElements(numberRemaining);
     rhsArray->setPacked();
     nonLinearCost_->goBackAll(rhsArray);
     rhsArray->clear();

}
/*
   Chooses primal pivot column
   updateArray has cost updates (also use pivotRow_ from last iteration)
   Would be faster with separate region to scan
   and will have this (with square of infeasibility) when steepest
   For easy problems we can just choose one of the first columns we look at
*/
void
ClpSimplexPrimal::primalColumn(CoinIndexedVector * updates,
                               CoinIndexedVector * spareRow1,
                               CoinIndexedVector * spareRow2,
                               CoinIndexedVector * spareColumn1,
                               CoinIndexedVector * spareColumn2)
{

     ClpMatrixBase * saveMatrix = matrix_;
     double * saveRowScale = rowScale_;
     if (scaledMatrix_) {
          rowScale_ = NULL;
          matrix_ = scaledMatrix_;
     }
     sequenceIn_ = primalColumnPivot_->pivotColumn(updates, spareRow1,
                   spareRow2, spareColumn1,
                   spareColumn2);
     if (scaledMatrix_) {
          matrix_ = saveMatrix;
          rowScale_ = saveRowScale;
     }
     if (sequenceIn_ >= 0) {
          valueIn_ = solution_[sequenceIn_];
          dualIn_ = dj_[sequenceIn_];
          if (nonLinearCost_->lookBothWays()) {
               // double check
               ClpSimplex::Status status = getStatus(sequenceIn_);

               switch(status) {
               case ClpSimplex::atUpperBound:
                    if (dualIn_ < 0.0) {
                         // move to other side
                         COIN_DETAIL_PRINT(printf("For %d U (%g, %g, %g) dj changed from %g",
                                sequenceIn_, lower_[sequenceIn_], solution_[sequenceIn_],
						  upper_[sequenceIn_], dualIn_));
                         dualIn_ -= nonLinearCost_->changeUpInCost(sequenceIn_);
                         COIN_DETAIL_PRINT(printf(" to %g\n", dualIn_));
                         nonLinearCost_->setOne(sequenceIn_, upper_[sequenceIn_] + 2.0 * currentPrimalTolerance());
                         setStatus(sequenceIn_, ClpSimplex::atLowerBound);
                    }
                    break;
               case ClpSimplex::atLowerBound:
                    if (dualIn_ > 0.0) {
                         // move to other side
                         COIN_DETAIL_PRINT(printf("For %d L (%g, %g, %g) dj changed from %g",
                                sequenceIn_, lower_[sequenceIn_], solution_[sequenceIn_],
						  upper_[sequenceIn_], dualIn_));
                         dualIn_ -= nonLinearCost_->changeDownInCost(sequenceIn_);
                         COIN_DETAIL_PRINT(printf(" to %g\n", dualIn_));
                         nonLinearCost_->setOne(sequenceIn_, lower_[sequenceIn_] - 2.0 * currentPrimalTolerance());
                         setStatus(sequenceIn_, ClpSimplex::atUpperBound);
                    }
                    break;
               default:
                    break;
               }
          }
          lowerIn_ = lower_[sequenceIn_];
          upperIn_ = upper_[sequenceIn_];
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
ClpSimplexPrimal::updatePrimalsInPrimal(CoinIndexedVector * rowArray,
                                        double theta,
                                        double & objectiveChange,
                                        int valuesPass)
{
     // Cost on pivot row may change - may need to change dualIn
     double oldCost = 0.0;
     if (pivotRow_ >= 0)
          oldCost = cost_[sequenceOut_];
     //rowArray->scanAndPack();
     double * work = rowArray->denseVector();
     int number = rowArray->getNumElements();
     int * which = rowArray->getIndices();

     int newNumber = 0;
     int pivotPosition = -1;
     nonLinearCost_->setChangeInCost(0.0);
     //printf("XX 4138 sol %g lower %g upper %g cost %g status %d\n",
     //   solution_[4138],lower_[4138],upper_[4138],cost_[4138],status_[4138]);
     // allow for case where bound+tolerance == bound
     //double tolerance = 0.999999*primalTolerance_;
     double relaxedTolerance = 1.001 * primalTolerance_;
     int iIndex;
     if (!valuesPass) {
          for (iIndex = 0; iIndex < number; iIndex++) {

               int iRow = which[iIndex];
               double alpha = work[iIndex];
               work[iIndex] = 0.0;
               int iPivot = pivotVariable_[iRow];
               double change = theta * alpha;
               double value = solution_[iPivot] - change;
               solution_[iPivot] = value;
#ifndef NDEBUG
               // check if not active then okay
               if (!active(iRow) && (specialOptions_ & 4) == 0 && pivotRow_ != -1) {
                    // But make sure one going out is feasible
                    if (change > 0.0) {
                         // going down
                         if (value <= lower_[iPivot] + primalTolerance_) {
                              if (iPivot == sequenceOut_ && value > lower_[iPivot] - relaxedTolerance)
                                   value = lower_[iPivot];
                              //double difference = nonLinearCost_->setOne(iPivot, value);
                              //assert (!difference || fabs(change) > 1.0e9);
                         }
                    } else {
                         // going up
                         if (value >= upper_[iPivot] - primalTolerance_) {
                              if (iPivot == sequenceOut_ && value < upper_[iPivot] + relaxedTolerance)
                                   value = upper_[iPivot];
                              //double difference = nonLinearCost_->setOne(iPivot, value);
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
                         if (value <= lower_[iPivot] + primalTolerance_) {
                              if (iPivot == sequenceOut_ && value >= lower_[iPivot] - relaxedTolerance)
                                   value = lower_[iPivot];
                              double difference = nonLinearCost_->setOne(iPivot, value);
                              if (difference) {
                                   if (iRow == pivotRow_)
                                        pivotPosition = newNumber;
                                   work[newNumber] = difference;
                                   //change reduced cost on this
                                   dj_[iPivot] = -difference;
                                   which[newNumber++] = iRow;
                              }
                         }
                    } else {
                         // going up
                         if (value >= upper_[iPivot] - primalTolerance_) {
                              if (iPivot == sequenceOut_ && value < upper_[iPivot] + relaxedTolerance)
                                   value = upper_[iPivot];
                              double difference = nonLinearCost_->setOne(iPivot, value);
                              if (difference) {
                                   if (iRow == pivotRow_)
                                        pivotPosition = newNumber;
                                   work[newNumber] = difference;
                                   //change reduced cost on this
                                   dj_[iPivot] = -difference;
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
               double alpha = work[iIndex];
               work[iIndex] = 0.0;
               int iPivot = pivotVariable_[iRow];
               double change = theta * alpha;
               double value = solution_[iPivot] - change;
               solution_[iPivot] = value;
               clearActive(iRow);
               // But make sure one going out is feasible
               if (change > 0.0) {
                    // going down
                    if (value <= lower_[iPivot] + primalTolerance_) {
                         if (iPivot == sequenceOut_ && value > lower_[iPivot] - relaxedTolerance)
                              value = lower_[iPivot];
                         double difference = nonLinearCost_->setOne(iPivot, value);
                         if (difference) {
                              if (iRow == pivotRow_)
                                   pivotPosition = newNumber;
                              work[newNumber] = difference;
                              //change reduced cost on this
                              dj_[iPivot] = -difference;
                              which[newNumber++] = iRow;
                         }
                    }
               } else {
                    // going up
                    if (value >= upper_[iPivot] - primalTolerance_) {
                         if (iPivot == sequenceOut_ && value < upper_[iPivot] + relaxedTolerance)
                              value = upper_[iPivot];
                         double difference = nonLinearCost_->setOne(iPivot, value);
                         if (difference) {
                              if (iRow == pivotRow_)
                                   pivotPosition = newNumber;
                              work[newNumber] = difference;
                              //change reduced cost on this
                              dj_[iPivot] = -difference;
                              which[newNumber++] = iRow;
                         }
                    }
               }
          }
     }
     objectiveChange += nonLinearCost_->changeInCost();
     rowArray->setPacked();
#if 0
     rowArray->setNumElements(newNumber);
     rowArray->expand();
     if (pivotRow_ >= 0) {
          dualIn_ += (oldCost - cost_[sequenceOut_]);
          // update change vector to include pivot
          rowArray->add(pivotRow_, -dualIn_);
          // and convert to packed
          rowArray->scanAndPack();
     } else {
          // and convert to packed
          rowArray->scanAndPack();
     }
#else
     if (pivotRow_ >= 0) {
          double dualIn = dualIn_ + (oldCost - cost_[sequenceOut_]);
          // update change vector to include pivot
          if (pivotPosition >= 0) {
               work[pivotPosition] -= dualIn;
          } else {
               work[newNumber] = -dualIn;
               which[newNumber++] = pivotRow_;
          }
     }
     rowArray->setNumElements(newNumber);
#endif
     return 0;
}
// Perturbs problem
void
ClpSimplexPrimal::perturb(int type)
{
     if (perturbation_ > 100)
          return; //perturbed already
     if (perturbation_ == 100)
          perturbation_ = 50; // treat as normal
     int savePerturbation = perturbation_;
     int i;
     if (!numberIterations_)
          cleanStatus(); // make sure status okay
     // Make sure feasible bounds
     if (nonLinearCost_) {
       nonLinearCost_->checkInfeasibilities();
       //nonLinearCost_->feasibleBounds();
     }
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
     if (!numberIterations_ && perturbation_ == 50) {
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
               return; // good enough
          }
     }
     // primal perturbation
     double perturbation = 1.0e-20;
     double bias = 1.0;
     int numberNonZero = 0;
     // maximum fraction of rhs/bounds to perturb
     double maximumFraction = 1.0e-5;
     if (perturbation_ >= 50) {
          perturbation = 1.0e-4;
          for (i = 0; i < numberColumns_ + numberRows_; i++) {
               if (upper_[i] > lower_[i] + primalTolerance_) {
                    double lowerValue, upperValue;
                    if (lower_[i] > -1.0e20)
                         lowerValue = fabs(lower_[i]);
                    else
                         lowerValue = 0.0;
                    if (upper_[i] < 1.0e20)
                         upperValue = fabs(upper_[i]);
                    else
                         upperValue = 0.0;
                    double value = CoinMax(fabs(lowerValue), fabs(upperValue));
                    value = CoinMin(value, upper_[i] - lower_[i]);
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
               while (perturbation_ > 50) {
                    perturbation_--;
                    perturbation *= 0.25;
                    bias *= 0.25;
               }
          } else if (perturbation_ >= 55 && perturbation_ < 60) {
               // increase
               while (perturbation_ > 55) {
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
          if (getRowStatus(iSequence) == basic)
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
     if (number != numberRows_)
          type = 1;
     // modify bounds
     // Change so at least 1.0e-5 and no more than 0.1
     // For now just no more than 0.1
     // printf("Pert type %d perturbation %g, maxF %g\n",type,perturbation,maximumFraction);
     // seems much slower???#define SAVE_PERT
#ifdef SAVE_PERT
     if (2 * numberColumns_ > maximumPerturbationSize_) {
          delete [] perturbationArray_;
          maximumPerturbationSize_ = 2 * numberColumns_;
          perturbationArray_ = new double [maximumPerturbationSize_];
          for (int iColumn = 0; iColumn < maximumPerturbationSize_; iColumn++) {
               perturbationArray_[iColumn] = randomNumberGenerator_.randomDouble();
          }
     }
#endif
     if (type == 1) {
          double tolerance = 100.0 * primalTolerance_;
          //double multiplier = perturbation*maximumFraction;
          for (iSequence = 0; iSequence < numberRows_ + numberColumns_; iSequence++) {
               if (getStatus(iSequence) == basic) {
                    double lowerValue = lower_[iSequence];
                    double upperValue = upper_[iSequence];
                    if (upperValue > lowerValue + tolerance) {
                         double solutionValue = solution_[iSequence];
                         double difference = upperValue - lowerValue;
                         difference = CoinMin(difference, perturbation);
                         difference = CoinMin(difference, fabs(solutionValue) + 1.0);
                         double value = maximumFraction * (difference + bias);
                         value = CoinMin(value, 0.1);
			 value = CoinMax(value,primalTolerance_);
#ifndef SAVE_PERT
                         value *= randomNumberGenerator_.randomDouble();
#else
                         value *= perturbationArray_[2*iSequence];
#endif
                         if (solutionValue - lowerValue <= primalTolerance_) {
                              lower_[iSequence] -= value;
                         } else if (upperValue - solutionValue <= primalTolerance_) {
                              upper_[iSequence] += value;
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
                              if (printOut)
                                   printf("col %d lower from %g to %g, upper from %g to %g\n",
                                          iSequence, lower_[iSequence], lowerValue, upper_[iSequence], upperValue);
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
     } else {
          double tolerance = 100.0 * primalTolerance_;
          for (i = 0; i < numberColumns_; i++) {
               double lowerValue = lower_[i], upperValue = upper_[i];
               if (upperValue > lowerValue + primalTolerance_) {
                    double value = perturbation * maximumFraction;
                    value = CoinMin(value, 0.1);
#ifndef SAVE_PERT
                    value *= randomNumberGenerator_.randomDouble();
#else
                    value *= perturbationArray_[2*i+1];
#endif
                    value *= randomNumberGenerator_.randomDouble();
                    if (savePerturbation != 50) {
                         if (fabs(value) <= primalTolerance_)
                              value = 0.0;
                         if (lowerValue > -1.0e20 && lowerValue)
                              lowerValue -= value * (CoinMax(1.0e-2, 1.0e-5 * fabs(lowerValue)));
                         if (upperValue < 1.0e20 && upperValue)
                              upperValue += value * (CoinMax(1.0e-2, 1.0e-5 * fabs(upperValue)));
                    } else if (value) {
                         double valueL = value * (CoinMax(1.0e-2, 1.0e-5 * fabs(lowerValue)));
                         // get in range
                         if (valueL <= tolerance) {
                              valueL *= 10.0;
                              while (valueL <= tolerance)
                                   valueL *= 10.0;
                         } else if (valueL > 1.0) {
                              valueL *= 0.1;
                              while (valueL > 1.0)
                                   valueL *= 0.1;
                         }
                         if (lowerValue > -1.0e20 && lowerValue)
                              lowerValue -= valueL;
                         double valueU = value * (CoinMax(1.0e-2, 1.0e-5 * fabs(upperValue)));
                         // get in range
                         if (valueU <= tolerance) {
                              valueU *= 10.0;
                              while (valueU <= tolerance)
                                   valueU *= 10.0;
                         } else if (valueU > 1.0) {
                              valueU *= 0.1;
                              while (valueU > 1.0)
                                   valueU *= 0.1;
                         }
                         if (upperValue < 1.0e20 && upperValue)
                              upperValue += valueU;
                    }
                    if (lowerValue != lower_[i]) {
                         double difference = fabs(lowerValue - lower_[i]);
                         largest = CoinMax(largest, difference);
                         if (difference > fabs(lower_[i])*largestPerCent)
                              largestPerCent = fabs(difference / lower_[i]);
                    }
                    if (upperValue != upper_[i]) {
                         double difference = fabs(upperValue - upper_[i]);
                         largest = CoinMax(largest, difference);
                         if (difference > fabs(upper_[i])*largestPerCent)
                              largestPerCent = fabs(difference / upper_[i]);
                    }
                    if (printOut)
                         printf("col %d lower from %g to %g, upper from %g to %g\n",
                                i, lower_[i], lowerValue, upper_[i], upperValue);
               }
               lower_[i] = lowerValue;
               upper_[i] = upperValue;
          }
          for (; i < numberColumns_ + numberRows_; i++) {
               double lowerValue = lower_[i], upperValue = upper_[i];
               double value = perturbation * maximumFraction;
               value = CoinMin(value, 0.1);
               value *= randomNumberGenerator_.randomDouble();
               if (upperValue > lowerValue + tolerance) {
                    if (savePerturbation != 50) {
                         if (fabs(value) <= primalTolerance_)
                              value = 0.0;
                         if (lowerValue > -1.0e20 && lowerValue)
                              lowerValue -= value * (CoinMax(1.0e-2, 1.0e-5 * fabs(lowerValue)));
                         if (upperValue < 1.0e20 && upperValue)
                              upperValue += value * (CoinMax(1.0e-2, 1.0e-5 * fabs(upperValue)));
                    } else if (value) {
                         double valueL = value * (CoinMax(1.0e-2, 1.0e-5 * fabs(lowerValue)));
                         // get in range
                         if (valueL <= tolerance) {
                              valueL *= 10.0;
                              while (valueL <= tolerance)
                                   valueL *= 10.0;
                         } else if (valueL > 1.0) {
                              valueL *= 0.1;
                              while (valueL > 1.0)
                                   valueL *= 0.1;
                         }
                         if (lowerValue > -1.0e20 && lowerValue)
                              lowerValue -= valueL;
                         double valueU = value * (CoinMax(1.0e-2, 1.0e-5 * fabs(upperValue)));
                         // get in range
                         if (valueU <= tolerance) {
                              valueU *= 10.0;
                              while (valueU <= tolerance)
                                   valueU *= 10.0;
                         } else if (valueU > 1.0) {
                              valueU *= 0.1;
                              while (valueU > 1.0)
                                   valueU *= 0.1;
                         }
                         if (upperValue < 1.0e20 && upperValue)
                              upperValue += valueU;
                    }
               } else if (upperValue > 0.0) {
                    upperValue -= value * (CoinMax(1.0e-2, 1.0e-5 * fabs(lowerValue)));
                    lowerValue -= value * (CoinMax(1.0e-2, 1.0e-5 * fabs(lowerValue)));
               } else if (upperValue < 0.0) {
                    upperValue += value * (CoinMax(1.0e-2, 1.0e-5 * fabs(lowerValue)));
                    lowerValue += value * (CoinMax(1.0e-2, 1.0e-5 * fabs(lowerValue)));
               } else {
               }
               if (lowerValue != lower_[i]) {
                    double difference = fabs(lowerValue - lower_[i]);
                    largest = CoinMax(largest, difference);
                    if (difference > fabs(lower_[i])*largestPerCent)
                         largestPerCent = fabs(difference / lower_[i]);
               }
               if (upperValue != upper_[i]) {
                    double difference = fabs(upperValue - upper_[i]);
                    largest = CoinMax(largest, difference);
                    if (difference > fabs(upper_[i])*largestPerCent)
                         largestPerCent = fabs(difference / upper_[i]);
               }
               if (printOut)
                    printf("row %d lower from %g to %g, upper from %g to %g\n",
                           i - numberColumns_, lower_[i], lowerValue, upper_[i], upperValue);
               lower_[i] = lowerValue;
               upper_[i] = upperValue;
          }
     }
     // Clean up
     for (i = 0; i < numberColumns_ + numberRows_; i++) {
          switch(getStatus(i)) {

          case basic:
               break;
          case atUpperBound:
               solution_[i] = upper_[i];
               break;
          case isFixed:
          case atLowerBound:
               solution_[i] = lower_[i];
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
     // say perturbed
     perturbation_ = 101;
}
// un perturb
bool
ClpSimplexPrimal::unPerturb()
{
     if (perturbation_ != 101)
          return false;
     // put back original bounds and costs
     createRim(1 + 4);
     sanityCheck();
     // unflag
     unflag();
     // get a valid nonlinear cost function
     delete nonLinearCost_;
     nonLinearCost_ = new ClpNonLinearCost(this);
     perturbation_ = 102; // stop any further perturbation
     // move non basic variables to new bounds
     nonLinearCost_->checkInfeasibilities(0.0);
#if 1
     // Try using dual
     return true;
#else
     gutsOfSolution(NULL, NULL, ifValuesPass != 0);
     return false;
#endif

}
// Unflag all variables and return number unflagged
int
ClpSimplexPrimal::unflag()
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
               if (fabs(dj_[i]) > relaxedToleranceD)
                    numberFlagged++;
          }
     }
     numberFlagged += matrix_->generalExpanded(this, 8, i);
     if (handler_->logLevel() > 2 && numberFlagged && objective_->type() > 1)
          printf("%d unflagged\n", numberFlagged);
     return numberFlagged;
}
// Do not change infeasibility cost and always say optimal
void
ClpSimplexPrimal::alwaysOptimal(bool onOff)
{
     if (onOff)
          specialOptions_ |= 1;
     else
          specialOptions_ &= ~1;
}
bool
ClpSimplexPrimal::alwaysOptimal() const
{
     return (specialOptions_ & 1) != 0;
}
// Flatten outgoing variables i.e. - always to exact bound
void
ClpSimplexPrimal::exactOutgoing(bool onOff)
{
     if (onOff)
          specialOptions_ |= 4;
     else
          specialOptions_ &= ~4;
}
bool
ClpSimplexPrimal::exactOutgoing() const
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
ClpSimplexPrimal::pivotResult(int ifValuesPass)
{

     bool roundAgain = true;
     int returnCode = -1;

     // loop round if user setting and doing refactorization
     while (roundAgain) {
          roundAgain = false;
          returnCode = -1;
          pivotRow_ = -1;
          sequenceOut_ = -1;
          rowArray_[1]->clear();
#if 0
          {
               int seq[] = {612, 643};
               int k;
               for (k = 0; k < sizeof(seq) / sizeof(int); k++) {
                    int iSeq = seq[k];
                    if (getColumnStatus(iSeq) != basic) {
                         double djval;
                         double * work;
                         int number;
                         int * which;

                         int iIndex;
                         unpack(rowArray_[1], iSeq);
                         factorization_->updateColumn(rowArray_[2], rowArray_[1]);
                         djval = cost_[iSeq];
                         work = rowArray_[1]->denseVector();
                         number = rowArray_[1]->getNumElements();
                         which = rowArray_[1]->getIndices();

                         for (iIndex = 0; iIndex < number; iIndex++) {

                              int iRow = which[iIndex];
                              double alpha = work[iRow];
                              int iPivot = pivotVariable_[iRow];
                              djval -= alpha * cost_[iPivot];
                         }
                         double comp = 1.0e-8 + 1.0e-7 * (CoinMax(fabs(dj_[iSeq]), fabs(djval)));
                         if (fabs(djval - dj_[iSeq]) > comp)
                              printf("Bad dj %g for %d - true is %g\n",
                                     dj_[iSeq], iSeq, djval);
                         assert (fabs(djval) < 1.0e-3 || djval * dj_[iSeq] > 0.0);
                         rowArray_[1]->clear();
                    }
               }
          }
#endif

          // we found a pivot column
          // update the incoming column
          unpackPacked(rowArray_[1]);
          // save reduced cost
          double saveDj = dualIn_;
          factorization_->updateColumnFT(rowArray_[2], rowArray_[1]);
          // Get extra rows
          matrix_->extendUpdated(this, rowArray_[1], 0);
          // do ratio test and re-compute dj
#ifdef CLP_USER_DRIVEN
          if (solveType_ != 2 || (moreSpecialOptions_ & 512) == 0) {
#endif
               primalRow(rowArray_[1], rowArray_[3], rowArray_[2],
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
                         primalRow(rowArray_[1], rowArray_[3], rowArray_[2],
                                   0);
                    }
                    if (pivotRow_ == -1 || (pivotRow_ >= 0 && fabs(alpha_) < 1.0e-5)) {
                         if (solveType_ == 1) {
                              // reject it
                              char x = isColumn(sequenceIn_) ? 'C' : 'R';
                              handler_->message(CLP_SIMPLEX_FLAG, messages_)
                                        << x << sequenceWithin(sequenceIn_)
                                        << CoinMessageEol;
                              setFlagged(sequenceIn_);
                              progress_.clearBadTimes();
                              lastBadIteration_ = numberIterations_; // say be more cautious
                              clearAll();
                              pivotRow_ = -1;
                         }
                         returnCode = -5;
                         break;
                    }
               }
          }
          // need to clear toIndex_ in gub
          // ? when can I clear stuff
          // Clean up any gub stuff
          matrix_->extendUpdated(this, rowArray_[1], 1);
          double checkValue = 1.0e-2;
          if (largestDualError_ > 1.0e-5)
               checkValue = 1.0e-1;
          double test2 = dualTolerance_;
          double test1 = 1.0e-20;
#if 0 //def FEB_TRY
          if (factorization_->pivots() < 1) {
               test1 = -1.0e-4;
               if ((saveDj < 0.0 && dualIn_ < -1.0e-5 * dualTolerance_) ||
                         (saveDj > 0.0 && dualIn_ > 1.0e-5 * dualTolerance_))
                    test2 = 0.0; // allow through
          }
#endif
          if (!ifValuesPass && solveType_ == 1 && (saveDj * dualIn_ < test1 ||
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
                              nonLinearCost_->checkInfeasibilities(0.0);
                         }
                         sequenceOut_ = -1;
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
#if 1 //def FEB_TRY
                              // Make safer?
                              factorization_->saferTolerances (-0.99, -1.03);
#endif
                              progress_.clearBadTimes();
                              lastBadIteration_ = numberIterations_; // say be more cautious
                              clearAll();
                              pivotRow_ = -1;
                              returnCode = -5;
                              sequenceOut_ = -1;
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
               if (solveType_ >= 2 && (moreSpecialOptions_ & 512) == 0) {
                    // **** Coding for user interface
                    // do ray
		    if (solveType_==2)
                      primalRay(rowArray_[1]);
                    // update duals
                    // as packed need to find pivot row
                    //assert (rowArray_[1]->packedMode());
                    //int i;

                    //alpha_ = rowArray_[1]->denseVector()[pivotRow_];
                    CoinAssert (fabs(alpha_) > 1.0e-12);
                    double multiplier = dualIn_ / alpha_;
#ifndef NDEBUG
		    rowArray_[0]->checkClear();
#endif
                    rowArray_[0]->insert(pivotRow_, multiplier);
                    factorization_->updateColumnTranspose(rowArray_[2], rowArray_[0]);
                    // put row of tableau in rowArray[0] and columnArray[0]
                    matrix_->transposeTimes(this, -1.0,
                                            rowArray_[0], columnArray_[1], columnArray_[0]);
                    // update column djs
                    int i;
                    int * index = columnArray_[0]->getIndices();
                    int number = columnArray_[0]->getNumElements();
                    double * element = columnArray_[0]->denseVector();
                    for (i = 0; i < number; i++) {
                         int ii = index[i];
                         dj_[ii] += element[ii];
                         reducedCost_[ii] = dj_[ii];
                         element[ii] = 0.0;
                    }
                    columnArray_[0]->setNumElements(0);
                    // and row djs
                    index = rowArray_[0]->getIndices();
                    number = rowArray_[0]->getNumElements();
                    element = rowArray_[0]->denseVector();
                    for (i = 0; i < number; i++) {
                         int ii = index[i];
                         dj_[ii+numberColumns_] += element[ii];
                         dual_[ii] = dj_[ii+numberColumns_];
                         element[ii] = 0.0;
                    }
                    rowArray_[0]->setNumElements(0);
                    // check incoming
                    CoinAssert (fabs(dj_[sequenceIn_]) < 1.0e-1);
               }
               // if stable replace in basis
               // If gub or odd then alpha and pivotRow may change
               int updateType = 0;
               int updateStatus = matrix_->generalExpanded(this, 3, updateType);
               if (updateType >= 0)
                    updateStatus = factorization_->replaceColumn(this,
                                   rowArray_[2],
                                   rowArray_[1],
                                   pivotRow_,
                                   alpha_,
                                   (moreSpecialOptions_ & 16) != 0);

               // if no pivots, bad update but reasonable alpha - take and invert
               if (updateStatus == 2 &&
                         lastGoodIteration_ == numberIterations_ && fabs(alpha_) > 1.0e-5)
                    updateStatus = 4;
               if (updateStatus == 1 || updateStatus == 4) {
                    // slight error
                    if (factorization_->pivots() > 5 || updateStatus == 4) {
                         returnCode = -3;
                    }
               } else if (updateStatus == 2) {
                    // major error
                    // better to have small tolerance even if slower
                    factorization_->zeroTolerance(CoinMin(factorization_->zeroTolerance(), 1.0e-15));
                    int maxFactor = factorization_->maximumPivots();
                    if (maxFactor > 10) {
                         if (forceFactorization_ < 0)
                              forceFactorization_ = maxFactor;
                         forceFactorization_ = CoinMax(1, (forceFactorization_ >> 1));
                    }
                    // later we may need to unwind more e.g. fake bounds
                    if(lastGoodIteration_ != numberIterations_) {
                         clearAll();
                         pivotRow_ = -1;
                         if (solveType_ == 1 || (moreSpecialOptions_ & 512) != 0) {
                              returnCode = -4;
                              break;
                         } else {
                              // user in charge - re-factorize
                              int lastCleaned = 0;
                              ClpSimplexProgress dummyProgress;
                              if (saveStatus_)
                                   statusOfProblemInPrimal(lastCleaned, 1, &dummyProgress, true, ifValuesPass);
                              else
                                   statusOfProblemInPrimal(lastCleaned, 0, &dummyProgress, true, ifValuesPass);
                              roundAgain = true;
                              continue;
                         }
                    } else {
                         // need to reject something
                         if (solveType_ == 1) {
                              char x = isColumn(sequenceIn_) ? 'C' : 'R';
                              handler_->message(CLP_SIMPLEX_FLAG, messages_)
                                        << x << sequenceWithin(sequenceIn_)
                                        << CoinMessageEol;
                              setFlagged(sequenceIn_);
                              progress_.clearBadTimes();
                         }
                         lastBadIteration_ = numberIterations_; // say be more cautious
                         clearAll();
                         pivotRow_ = -1;
                         sequenceOut_ = -1;
                         returnCode = -5;
                         break;

                    }
               } else if (updateStatus == 3) {
                    // out of memory
                    // increase space if not many iterations
                    if (factorization_->pivots() <
                              0.5 * factorization_->maximumPivots() &&
                              factorization_->pivots() < 200)
                         factorization_->areaFactor(
                              factorization_->areaFactor() * 1.1);
                    returnCode = -2; // factorize now
               } else if (updateStatus == 5) {
                    problemStatus_ = -2; // factorize now
               }
               // here do part of steepest - ready for next iteration
               if (!ifValuesPass)
                    primalColumnPivot_->updateWeights(rowArray_[1]);
          } else {
               if (pivotRow_ == -1) {
                    // no outgoing row is valid
                    if (valueOut_ != COIN_DBL_MAX) {
                         double objectiveChange = 0.0;
                         theta_ = valueOut_ - valueIn_;
                         updatePrimalsInPrimal(rowArray_[1], theta_, objectiveChange, ifValuesPass);
                         solution_[sequenceIn_] += theta_;
                    }
                    rowArray_[0]->clear();
 #ifdef CLP_USER_DRIVEN1
		    /* Note if valueOut_ < COIN_DBL_MAX and
		       theta_ reasonable then this may be a valid sub flip */
		    if(!userChoiceValid2(this)) {
		      if (factorization_->pivots()<5) {
			// flag variable
			char x = isColumn(sequenceIn_) ? 'C' : 'R';
			handler_->message(CLP_SIMPLEX_FLAG, messages_)
			  << x << sequenceWithin(sequenceIn_)
			  << CoinMessageEol;
			setFlagged(sequenceIn_);
			progress_.clearBadTimes();
			roundAgain = true;
			continue;
		      } else {
			// try refactorizing first
			returnCode = 4; //say looks odd but has iterated
			break;
		      }
		    }
 #endif
                    if (!factorization_->pivots() && acceptablePivot_ <= 1.0e-8 ) {
                         returnCode = 2; //say looks unbounded
                         // do ray
			 if (!nonLinearCost_->sumInfeasibilities())
			   primalRay(rowArray_[1]);
                    } else if (solveType_ == 2 && (moreSpecialOptions_ & 512) == 0) {
                         // refactorize
                         int lastCleaned = 0;
                         ClpSimplexProgress dummyProgress;
                         if (saveStatus_)
                              statusOfProblemInPrimal(lastCleaned, 1, &dummyProgress, true, ifValuesPass);
                         else
                              statusOfProblemInPrimal(lastCleaned, 0, &dummyProgress, true, ifValuesPass);
                         roundAgain = true;
                         continue;
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
               oldCost = cost_[sequenceOut_];
          // update primal solution

          double objectiveChange = 0.0;
          // after this rowArray_[1] is not empty - used to update djs
          // If pivot row >= numberRows then may be gub
          int savePivot = pivotRow_;
          if (pivotRow_ >= numberRows_)
               pivotRow_ = -1;
#ifdef CLP_USER_DRIVEN
	  if (theta_<0.0) {
	    if (theta_>=-1.0e-12)
	      theta_=0.0;
	    //else
	    //printf("negative theta %g\n",theta_);
	  }
#endif
          updatePrimalsInPrimal(rowArray_[1], theta_, objectiveChange, ifValuesPass);
          pivotRow_ = savePivot;

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
               if(valueOut_ < lower_[sequenceOut_] - primalTolerance_)
                    valueOut_ = lower_[sequenceOut_] - 0.9 * primalTolerance_;
               else if (valueOut_ > upper_[sequenceOut_] + primalTolerance_)
                    valueOut_ = upper_[sequenceOut_] + 0.9 * primalTolerance_;
               // may not be exactly at bound and bounds may have changed
               // Make sure outgoing looks feasible
               directionOut_ = nonLinearCost_->setOneOutgoing(sequenceOut_, valueOut_);
               // May have got inaccurate
               //if (oldCost!=cost_[sequenceOut_])
               //printf("costchange on %d from %g to %g\n",sequenceOut_,
               //       oldCost,cost_[sequenceOut_]);
               if (solveType_ < 2)
                    dj_[sequenceOut_] = cost_[sequenceOut_] - oldCost; // normally updated next iteration
               solution_[sequenceOut_] = valueOut_;
          }
          // change cost and bounds on incoming if primal
          nonLinearCost_->setOne(sequenceIn_, valueIn_);
          int whatNext = housekeeping(objectiveChange);
          //nonLinearCost_->validate();
#if CLP_DEBUG >1
          {
               double sum;
               int ninf = matrix_->checkFeasible(this, sum);
               if (ninf)
                    printf("infeas %d\n", ninf);
          }
#endif
          if (whatNext == 1) {
               returnCode = -2; // refactorize
          } else if (whatNext == 2) {
               // maximum iterations or equivalent
               returnCode = 3;
          } else if(numberIterations_ == lastGoodIteration_
                    + 2 * factorization_->maximumPivots()) {
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
     if ((solveType_ == 2 && (moreSpecialOptions_ & 512) == 0) &&
               (returnCode == -2 || returnCode == -3)) {
          // refactorize here
          int lastCleaned = 0;
          ClpSimplexProgress dummyProgress;
          if (saveStatus_)
               statusOfProblemInPrimal(lastCleaned, 1, &dummyProgress, true, ifValuesPass);
          else
               statusOfProblemInPrimal(lastCleaned, 0, &dummyProgress, true, ifValuesPass);
          if (problemStatus_ == 5) {
	    COIN_DETAIL_PRINT(printf("Singular basis\n"));
               problemStatus_ = -1;
               returnCode = 5;
          }
     }
#ifdef CLP_DEBUG
     {
          int i;
          // not [1] as may have information
          for (i = 0; i < 4; i++) {
               if (i != 1)
                    rowArray_[i]->checkClear();
          }
          for (i = 0; i < 2; i++) {
               columnArray_[i]->checkClear();
          }
     }
#endif
     return returnCode;
}
// Create primal ray
void
ClpSimplexPrimal::primalRay(CoinIndexedVector * rowArray)
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
     if (!rowArray->packedMode()) {
          for (i = 0; i < number; i++) {
               int iRow = index[i];
               int iPivot = pivotVariable_[iRow];
               double arrayValue = array[iRow];
               if (iPivot < numberColumns_ && fabs(arrayValue) >= zeroTolerance)
                    ray_[iPivot] = way * arrayValue;
          }
     } else {
          for (i = 0; i < number; i++) {
               int iRow = index[i];
               int iPivot = pivotVariable_[iRow];
               double arrayValue = array[i];
               if (iPivot < numberColumns_ && fabs(arrayValue) >= zeroTolerance)
                    ray_[iPivot] = way * arrayValue;
          }
     }
}
/* Get next superbasic -1 if none,
   Normal type is 1
   If type is 3 then initializes sorted list
   if 2 uses list.
*/
int
ClpSimplexPrimal::nextSuperBasic(int superBasicType,
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
                              if (getStatus(iColumn) == superBasic) {
                                   if (fabs(solution_[iColumn] - lower_[iColumn]) <= primalTolerance_) {
                                        solution_[iColumn] = lower_[iColumn];
                                        setStatus(iColumn, atLowerBound);
                                   } else if (fabs(solution_[iColumn] - upper_[iColumn])
                                              <= primalTolerance_) {
                                        solution_[iColumn] = upper_[iColumn];
                                        setStatus(iColumn, atUpperBound);
                                   } else if (lower_[iColumn] < -1.0e20 && upper_[iColumn] > 1.0e20) {
                                        setStatus(iColumn, isFree);
                                        break;
                                   } else if (!flagged(iColumn)) {
                                        // put ones near bounds at end after sorting
                                        work[number] = - CoinMin(0.1 * (solution_[iColumn] - lower_[iColumn]),
                                                                 upper_[iColumn] - solution_[iColumn]);
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
                         if (getStatus(iColumn) == superBasic) {
                              if (fabs(solution_[iColumn] - lower_[iColumn]) <= primalTolerance_) {
                                   solution_[iColumn] = lower_[iColumn];
                                   setStatus(iColumn, atLowerBound);
                              } else if (fabs(solution_[iColumn] - upper_[iColumn])
                                         <= primalTolerance_) {
                                   solution_[iColumn] = upper_[iColumn];
                                   setStatus(iColumn, atUpperBound);
                              } else if (lower_[iColumn] < -1.0e20 && upper_[iColumn] > 1.0e20) {
                                   setStatus(iColumn, isFree);
				   if (fabs(dj_[iColumn])>dualTolerance_)
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
          if (returnValue >= 0 && getStatus(returnValue) != superBasic && getStatus(returnValue) != isFree)
               finished = false; // somehow picked up odd one
     }
     return returnValue;
}
void
ClpSimplexPrimal::clearAll()
{
     // Clean up any gub stuff
     matrix_->extendUpdated(this, rowArray_[1], 1);
     int number = rowArray_[1]->getNumElements();
     int * which = rowArray_[1]->getIndices();

     int iIndex;
     for (iIndex = 0; iIndex < number; iIndex++) {

          int iRow = which[iIndex];
          clearActive(iRow);
     }
     rowArray_[1]->clear();
     // make sure any gub sets are clean
     matrix_->generalExpanded(this, 11, sequenceIn_);

}
// Sort of lexicographic resolve
int
ClpSimplexPrimal::lexSolve()
{
     algorithm_ = +1;
     //specialOptions_ |= 4;

     // save data
     ClpDataSave data = saveData();
     matrix_->refresh(this); // make sure matrix okay

     // Save so can see if doing after dual
     int initialStatus = problemStatus_;
     int initialIterations = numberIterations_;
     int initialNegDjs = -1;
     // initialize - maybe values pass and algorithm_ is +1
     int ifValuesPass = 0;
#if 0
     // if so - put in any superbasic costed slacks
     // Start can skip some things in transposeTimes
     specialOptions_ |= 131072;
     if (ifValuesPass && specialOptions_ < 0x01000000) {
          // Get column copy
          const CoinPackedMatrix * columnCopy = matrix();
          const int * row = columnCopy->getIndices();
          const CoinBigIndex * columnStart = columnCopy->getVectorStarts();
          const int * columnLength = columnCopy->getVectorLengths();
          //const double * element = columnCopy->getElements();
          int n = 0;
          for (int iColumn = 0; iColumn < numberColumns_; iColumn++) {
               if (columnLength[iColumn] == 1) {
                    Status status = getColumnStatus(iColumn);
                    if (status != basic && status != isFree) {
                         double value = columnActivity_[iColumn];
                         if (fabs(value - columnLower_[iColumn]) > primalTolerance_ &&
                                   fabs(value - columnUpper_[iColumn]) > primalTolerance_) {
                              int iRow = row[columnStart[iColumn]];
                              if (getRowStatus(iRow) == basic) {
                                   setRowStatus(iRow, superBasic);
                                   setColumnStatus(iColumn, basic);
                                   n++;
                              }
                         }
                    }
               }
          }
          printf("%d costed slacks put in basis\n", n);
     }
#endif
     double * originalCost = NULL;
     double * originalLower = NULL;
     double * originalUpper = NULL;
     if (!startup(0, 0)) {

          // Set average theta
          nonLinearCost_->setAverageTheta(1.0e3);
          int lastCleaned = 0; // last time objective or bounds cleaned up

          // Say no pivot has occurred (for steepest edge and updates)
          pivotRow_ = -2;

          // This says whether to restore things etc
          int factorType = 0;
          if (problemStatus_ < 0 && perturbation_ < 100) {
               perturb(0);
               // Can't get here if values pass
               assert (!ifValuesPass);
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
          ClpSimplex * saveModel = NULL;
          int stopSprint = -1;
          int sprintPass = 0;
          int reasonableSprintIteration = 0;
          int lastSprintIteration = 0;
          double lastObjectiveValue = COIN_DBL_MAX;
          // Start check for cycles
          progress_.fillFromModel(this);
          progress_.startCheck();
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
          originalCost = CoinCopyOfArray(cost_, numberColumns_ + numberRows_);
          originalLower = CoinCopyOfArray(lower_, numberColumns_ + numberRows_);
          originalUpper = CoinCopyOfArray(upper_, numberColumns_ + numberRows_);
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
#if 1
               if (perturbation_ < 101 && numberIterations_ > 2 * (numberRows_ + numberColumns_) && (specialOptions_ & 4) == 0
                         && initialStatus != 10) {
                    perturb(1);
                    matrix_->rhsOffset(this, true, false);
               }
#endif
               // If we have done no iterations - special
               if (lastGoodIteration_ == numberIterations_ && factorType)
                    factorType = 3;
               if (saveModel) {
                    // Doing sprint
                    if (sequenceIn_ < 0 || numberIterations_ >= stopSprint) {
                         problemStatus_ = -1;
                         originalModel(saveModel);
                         saveModel = NULL;
                         if (sequenceIn_ < 0 && numberIterations_ < reasonableSprintIteration &&
                                   sprintPass > 100)
                              primalColumnPivot_->switchOffSprint();
                         //lastSprintIteration=numberIterations_;
                         COIN_DETAIL_PRINT(printf("End small model\n"));
                    }
               }

               // may factorize, checks if problem finished
               statusOfProblemInPrimal(lastCleaned, factorType, &progress_, true, ifValuesPass, saveModel);
               if (initialStatus == 10) {
                    // cleanup phase
                    if(initialIterations != numberIterations_) {
                         if (numberDualInfeasibilities_ > 10000 && numberDualInfeasibilities_ > 10 * initialNegDjs) {
                              // getting worse - try perturbing
                              if (perturbation_ < 101 && (specialOptions_ & 4) == 0) {
                                   perturb(1);
                                   matrix_->rhsOffset(this, true, false);
                                   statusOfProblemInPrimal(lastCleaned, factorType, &progress_, true, ifValuesPass, saveModel);
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
               // See if sprint says redo because of problems
               if (numberDualInfeasibilities_ == -776) {
                    // Need new set of variables
                    problemStatus_ = -1;
                    originalModel(saveModel);
                    saveModel = NULL;
                    //lastSprintIteration=numberIterations_;
                    COIN_DETAIL_PRINT(printf("End small model after\n"));
                    statusOfProblemInPrimal(lastCleaned, factorType, &progress_, true, ifValuesPass, saveModel);
               }
               int numberSprintIterations = 0;
               int numberSprintColumns = primalColumnPivot_->numberSprintColumns(numberSprintIterations);
               if (problemStatus_ == 777) {
                    // problems so do one pass with normal
                    problemStatus_ = -1;
                    originalModel(saveModel);
                    saveModel = NULL;
                    // Skip factorization
                    //statusOfProblemInPrimal(lastCleaned,factorType,&progress_,false,saveModel);
                    statusOfProblemInPrimal(lastCleaned, factorType, &progress_, true, ifValuesPass, saveModel);
               } else if (problemStatus_ < 0 && !saveModel && numberSprintColumns && firstFree_ < 0) {
                    int numberSort = 0;
                    int numberFixed = 0;
                    int numberBasic = 0;
                    reasonableSprintIteration = numberIterations_ + 100;
                    int * whichColumns = new int[numberColumns_];
                    double * weight = new double[numberColumns_];
                    int numberNegative = 0;
                    double sumNegative = 0.0;
                    // now massage weight so all basic in plus good djs
                    for (iColumn = 0; iColumn < numberColumns_; iColumn++) {
                         double dj = dj_[iColumn];
                         switch(getColumnStatus(iColumn)) {

                         case basic:
                              dj = -1.0e50;
                              numberBasic++;
                              break;
                         case atUpperBound:
                              dj = -dj;
                              break;
                         case isFixed:
                              dj = 1.0e50;
                              numberFixed++;
                              break;
                         case atLowerBound:
                              dj = dj;
                              break;
                         case isFree:
                              dj = -100.0 * fabs(dj);
                              break;
                         case superBasic:
                              dj = -100.0 * fabs(dj);
                              break;
                         }
                         if (dj < -dualTolerance_ && dj > -1.0e50) {
                              numberNegative++;
                              sumNegative -= dj;
                         }
                         weight[iColumn] = dj;
                         whichColumns[iColumn] = iColumn;
                    }
                    handler_->message(CLP_SPRINT, messages_)
                              << sprintPass << numberIterations_ - lastSprintIteration << objectiveValue() << sumNegative
                              << numberNegative
                              << CoinMessageEol;
                    sprintPass++;
                    lastSprintIteration = numberIterations_;
                    if (objectiveValue()*optimizationDirection_ > lastObjectiveValue - 1.0e-7 && sprintPass > 5) {
                         // switch off
		      COIN_DETAIL_PRINT(printf("Switching off sprint\n"));
                         primalColumnPivot_->switchOffSprint();
                    } else {
                         lastObjectiveValue = objectiveValue() * optimizationDirection_;
                         // sort
                         CoinSort_2(weight, weight + numberColumns_, whichColumns);
                         numberSort = CoinMin(numberColumns_ - numberFixed, numberBasic + numberSprintColumns);
                         // Sort to make consistent ?
                         std::sort(whichColumns, whichColumns + numberSort);
                         saveModel = new ClpSimplex(this, numberSort, whichColumns);
                         delete [] whichColumns;
                         delete [] weight;
                         // Skip factorization
                         //statusOfProblemInPrimal(lastCleaned,factorType,&progress_,false,saveModel);
                         //statusOfProblemInPrimal(lastCleaned,factorType,&progress_,true,saveModel);
                         stopSprint = numberIterations_ + numberSprintIterations;
                         COIN_DETAIL_PRINT(printf("Sprint with %d columns for %d iterations\n",
						  numberSprintColumns, numberSprintIterations));
                    }
               }

               // Say good factorization
               factorType = 1;

               // Say no pivot has occurred (for steepest edge and updates)
               pivotRow_ = -2;

               // exit if victory declared
               if (problemStatus_ >= 0) {
                    if (originalCost) {
                         // find number nonbasic with zero reduced costs
                         int numberDegen = 0;
                         int numberTotal = numberColumns_; //+numberRows_;
                         for (int i = 0; i < numberTotal; i++) {
                              cost_[i] = 0.0;
                              if (getStatus(i) == atLowerBound) {
                                   if (dj_[i] <= dualTolerance_) {
                                        cost_[i] = numberTotal - i + randomNumberGenerator_.randomDouble() * 0.5;
                                        numberDegen++;
                                   } else {
                                        // fix
                                        cost_[i] = 1.0e10; //upper_[i]=lower_[i];
                                   }
                              } else if (getStatus(i) == atUpperBound) {
                                   if (dj_[i] >= -dualTolerance_) {
                                        cost_[i] = (numberTotal - i) + randomNumberGenerator_.randomDouble() * 0.5;
                                        numberDegen++;
                                   } else {
                                        // fix
                                        cost_[i] = -1.0e10; //lower_[i]=upper_[i];
                                   }
                              } else if (getStatus(i) == basic) {
                                   cost_[i] = (numberTotal - i) + randomNumberGenerator_.randomDouble() * 0.5;
                              }
                         }
                         problemStatus_ = -1;
                         lastObjectiveValue = COIN_DBL_MAX;
                         // Start check for cycles
                         progress_.fillFromModel(this);
                         progress_.startCheck();
                         COIN_DETAIL_PRINT(printf("%d degenerate after %d iterations\n", numberDegen,
						  numberIterations_));
                         if (!numberDegen) {
                              CoinMemcpyN(originalCost, numberTotal, cost_);
                              delete [] originalCost;
                              originalCost = NULL;
                              CoinMemcpyN(originalLower, numberTotal, lower_);
                              delete [] originalLower;
                              CoinMemcpyN(originalUpper, numberTotal, upper_);
                              delete [] originalUpper;
                         }
                         delete nonLinearCost_;
                         nonLinearCost_ = new ClpNonLinearCost(this);
                         progress_.endOddState();
                         continue;
                    } else {
		      COIN_DETAIL_PRINT(printf("exiting after %d iterations\n", numberIterations_));
                         break;
                    }
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
     assert (!originalCost);
     // if infeasible get real values
     //printf("XXXXY final cost %g\n",infeasibilityCost_);
     progress_.initialWeight_ = 0.0;
     if (problemStatus_ == 1 && secondaryStatus_ != 6) {
          infeasibilityCost_ = 0.0;
          createRim(1 + 4);
          nonLinearCost_->checkInfeasibilities(0.0);
          sumPrimalInfeasibilities_ = nonLinearCost_->sumInfeasibilities();
          numberPrimalInfeasibilities_ = nonLinearCost_->numberInfeasibilities();
          // and get good feasible duals
          computeDuals(NULL);
     }
     // Stop can skip some things in transposeTimes
     specialOptions_ &= ~131072;
     // clean up
     unflag();
     finish(0);
     restoreData(data);
     return problemStatus_;
}
