/* $Id$ */
// Copyright (C) 2004, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include "CoinPragma.hpp"

#include <math.h>
#include "CoinHelperFunctions.hpp"
#include "ClpHelperFunctions.hpp"
#include "ClpSimplexNonlinear.hpp"
#include "ClpFactorization.hpp"
#include "ClpNonLinearCost.hpp"
#include "ClpLinearObjective.hpp"
#include "ClpConstraint.hpp"
#include "ClpQuadraticObjective.hpp"
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
#ifndef NDEBUG
#define CLP_DEBUG 1
#endif
// primal
int ClpSimplexNonlinear::primal ()
{

     int ifValuesPass = 1;
     algorithm_ = +3;

     // save data
     ClpDataSave data = saveData();
     matrix_->refresh(this); // make sure matrix okay

     // Save objective
     ClpObjective * saveObjective = NULL;
     if (objective_->type() > 1) {
          // expand to full if quadratic
#ifndef NO_RTTI
          ClpQuadraticObjective * quadraticObj = (dynamic_cast< ClpQuadraticObjective*>(objective_));
#else
          ClpQuadraticObjective * quadraticObj = NULL;
          if (objective_->type() == 2)
               quadraticObj = (static_cast< ClpQuadraticObjective*>(objective_));
#endif
          // for moment only if no scaling
          // May be faster if switched off - but can't see why
          if (!quadraticObj->fullMatrix() && (!rowScale_ && !scalingFlag_) && objectiveScale_ == 1.0) {
               saveObjective = objective_;
               objective_ = new ClpQuadraticObjective(*quadraticObj, 1);
          }
     }
     double bestObjectiveWhenFlagged = COIN_DBL_MAX;
     int pivotMode = 15;
     //pivotMode=20;

     // initialize - maybe values pass and algorithm_ is +1
     // true does something (? perturbs)
     if (!startup(true)) {

          // Set average theta
          nonLinearCost_->setAverageTheta(1.0e3);
          int lastCleaned = 0; // last time objective or bounds cleaned up

          // Say no pivot has occurred (for steepest edge and updates)
          pivotRow_ = -2;

          // This says whether to restore things etc
          int factorType = 0;
          // Start check for cycles
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
               // If we have done no iterations - special
               if (lastGoodIteration_ == numberIterations_ && factorType)
                    factorType = 3;

               // may factorize, checks if problem finished
               if (objective_->type() > 1 && lastFlaggedIteration_ >= 0 &&
                         numberIterations_ > lastFlaggedIteration_ + 507) {
                    unflag();
                    lastFlaggedIteration_ = numberIterations_;
                    if (pivotMode >= 10) {
                         pivotMode--;
#ifdef CLP_DEBUG
                         if (handler_->logLevel() & 32)
                              printf("pivot mode now %d\n", pivotMode);
#endif
                         if (pivotMode == 9)
                              pivotMode = 0;	// switch off fast attempt
                    }
               }
               statusOfProblemInPrimal(lastCleaned, factorType, &progress_, true,
                                       bestObjectiveWhenFlagged);

               // Say good factorization
               factorType = 1;

               // Say no pivot has occurred (for steepest edge and updates)
               pivotRow_ = -2;

               // exit if victory declared
               if (problemStatus_ >= 0)
                    break;

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
               whileIterating(pivotMode);
          }
     }
     // if infeasible get real values
     if (problemStatus_ == 1) {
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
     // correct objective value
     if (numberColumns_)
          objectiveValue_ = nonLinearCost_->feasibleCost() + objective_->nonlinearOffset();
     objectiveValue_ /= (objectiveScale_ * rhsScale_);
     // clean up
     unflag();
     finish();
     restoreData(data);
     // restore objective if full
     if (saveObjective) {
          delete objective_;
          objective_ = saveObjective;
     }
     return problemStatus_;
}
/*  Refactorizes if necessary
    Checks if finished.  Updates status.
    lastCleaned refers to iteration at which some objective/feasibility
    cleaning too place.

    type - 0 initial so set up save arrays etc
    - 1 normal -if good update save
    - 2 restoring from saved
*/
void
ClpSimplexNonlinear::statusOfProblemInPrimal(int & lastCleaned, int type,
          ClpSimplexProgress * progress,
          bool doFactorization,
          double & bestObjectiveWhenFlagged)
{
     int dummy; // for use in generalExpanded
     if (type == 2) {
          // trouble - restore solution
          CoinMemcpyN(saveStatus_, (numberColumns_ + numberRows_), status_ );
          CoinMemcpyN(savedSolution_ + numberColumns_ ,	numberRows_, rowActivityWork_);
          CoinMemcpyN(savedSolution_ ,	numberColumns_, columnActivityWork_);
          // restore extra stuff
          matrix_->generalExpanded(this, 6, dummy);
          forceFactorization_ = 1; // a bit drastic but ..
          pivotRow_ = -1; // say no weights update
          changeMade_++; // say change made
     }
     int saveThreshold = factorization_->sparseThreshold();
     int tentativeStatus = problemStatus_;
     int numberThrownOut = 1; // to loop round on bad factorization in values pass
     while (numberThrownOut) {
          if (problemStatus_ > -3 || problemStatus_ == -4) {
               // factorize
               // later on we will need to recover from singularities
               // also we could skip if first time
               // do weights
               // This may save pivotRow_ for use
               if (doFactorization)
                    primalColumnPivot_->saveWeights(this, 1);

               if (type && doFactorization) {
                    // is factorization okay?
                    int factorStatus = internalFactorize(1);
                    if (factorStatus) {
                         if (type != 1 || largestPrimalError_ > 1.0e3
                                   || largestDualError_ > 1.0e3) {
                              // was ||largestDualError_>1.0e3||objective_->type()>1) {
                              // switch off dense
                              int saveDense = factorization_->denseThreshold();
                              factorization_->setDenseThreshold(0);
                              // make sure will do safe factorization
                              pivotVariable_[0] = -1;
                              internalFactorize(2);
                              factorization_->setDenseThreshold(saveDense);
                              // Go to safe
                              factorization_->pivotTolerance(0.99);
                              // restore extra stuff
                              matrix_->generalExpanded(this, 6, dummy);
                         } else {
                              // no - restore previous basis
                              CoinMemcpyN(saveStatus_, (numberColumns_ + numberRows_), status_ );
                              CoinMemcpyN(savedSolution_ + numberColumns_ ,		numberRows_, rowActivityWork_);
                              CoinMemcpyN(savedSolution_ ,		numberColumns_, columnActivityWork_);
                              // restore extra stuff
                              matrix_->generalExpanded(this, 6, dummy);
                              matrix_->generalExpanded(this, 5, dummy);
                              forceFactorization_ = 1; // a bit drastic but ..
                              type = 2;
                              // Go to safe
                              factorization_->pivotTolerance(0.99);
                              if (internalFactorize(1) != 0)
                                   largestPrimalError_ = 1.0e4; // force other type
                         }
                         // flag incoming
                         if (sequenceIn_ >= 0 && getStatus(sequenceIn_) != basic) {
                              setFlagged(sequenceIn_);
                              saveStatus_[sequenceIn_] = status_[sequenceIn_];
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
          createRim(4);
          // May need to do more if column generation
          dummy = 4;
          matrix_->generalExpanded(this, 9, dummy);
          numberThrownOut = gutsOfSolution(NULL, NULL, (firstFree_ >= 0));
          if (numberThrownOut) {
               problemStatus_ = tentativeStatus;
               doFactorization = true;
          }
     }
     // Double check reduced costs if no action
     if (progress->lastIterationNumber(0) == numberIterations_) {
          if (primalColumnPivot_->looksOptimal()) {
               numberDualInfeasibilities_ = 0;
               sumDualInfeasibilities_ = 0.0;
          }
     }
     // Check if looping
     int loop;
     if (type != 2)
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
          gutsOfSolution(NULL, NULL, true);
     }
     // If progress then reset costs
     if (loop == -1 && !nonLinearCost_->numberInfeasibilities() && progress->oddState() < 0) {
          createRim(4, false); // costs back
          delete nonLinearCost_;
          nonLinearCost_ = new ClpNonLinearCost(this);
          progress->endOddState();
          gutsOfSolution(NULL, NULL, true);
     }
     // Flag to say whether to go to dual to clean up
     //bool goToDual = false;
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
          gutsOfSolution(NULL, NULL, true);
          nonLinearCost_->checkInfeasibilities(primalTolerance_);
     }
     double trueInfeasibility = nonLinearCost_->sumInfeasibilities();
     if (trueInfeasibility > 1.0) {
          // If infeasibility going up may change weights
          double testValue = trueInfeasibility - 1.0e-4 * (10.0 + trueInfeasibility);
          if(progress->lastInfeasibility() < testValue) {
               if (infeasibilityCost_ < 1.0e14) {
                    infeasibilityCost_ *= 1.5;
                    if (handler_->logLevel() == 63)
                         printf("increasing weight to %g\n", infeasibilityCost_);
                    gutsOfSolution(NULL, NULL, true);
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
     }
     // had ||(type==3&&problemStatus_!=-5) -- ??? why ????
     if (dualFeasible() || problemStatus_ == -4) {
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
               gutsOfSolution(NULL, NULL, true);

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
                    gutsOfSolution(NULL, NULL, false);
                    nonLinearCost_ = nonLinear;
                    infeasibilityCost_ = saveWeight;
                    if ((infeasibilityCost_ >= 1.0e18 ||
                              numberDualInfeasibilities_ == 0) && perturbation_ == 101) {
                         /* goToDual = unPerturb(); // stop any further perturbation
                         if (nonLinearCost_->sumInfeasibilities() > 1.0e-1)
                              goToDual = false;
                         */
                         unPerturb();
                         nonLinearCost_->checkInfeasibilities(primalTolerance_);
                         numberDualInfeasibilities_ = 1; // carry on
                         problemStatus_ = -1;
                    }
                    if (infeasibilityCost_ >= 1.0e20 ||
                              numberDualInfeasibilities_ == 0) {
                         // we are infeasible - use as ray
                         delete [] ray_;
                         ray_ = new double [numberRows_];
                         CoinMemcpyN(dual_, numberRows_, ray_);
                         // and get feasible duals
                         infeasibilityCost_ = 0.0;
                         createRim(4);
                         nonLinearCost_->checkInfeasibilities(primalTolerance_);
                         gutsOfSolution(NULL, NULL, true);
                         // so will exit
                         infeasibilityCost_ = 1.0e30;
                         // reset infeasibilities
                         sumPrimalInfeasibilities_ = nonLinearCost_->sumInfeasibilities();;
                         numberPrimalInfeasibilities_ =
                              nonLinearCost_->numberInfeasibilities();
                    }
                    if (infeasibilityCost_ < 1.0e20) {
                         infeasibilityCost_ *= 5.0;
                         changeMade_++; // say change made
                         unflag();
                         handler_->message(CLP_PRIMAL_WEIGHT, messages_)
                                   << infeasibilityCost_
                                   << CoinMessageEol;
                         // put back original costs and then check
                         createRim(4);
                         nonLinearCost_->checkInfeasibilities(0.0);
                         gutsOfSolution(NULL, NULL, true);
                         problemStatus_ = -1; //continue
                         //goToDual = false;
                    } else {
                         // say infeasible
                         problemStatus_ = 1;
                    }
               }
          } else {
               // may be optimal
               if (perturbation_ == 101) {
                    /* goToDual =*/ unPerturb(); // stop any further perturbation
                    lastCleaned = -1; // carry on
               }
               bool unflagged = unflag() != 0;
               if ( lastCleaned != numberIterations_ || unflagged) {
                    handler_->message(CLP_PRIMAL_OPTIMAL, messages_)
                              << primalTolerance_
                              << CoinMessageEol;
                    double current = nonLinearCost_->feasibleReportCost();
                    if (numberTimesOptimal_ < 4) {
                         if (bestObjectiveWhenFlagged <= current) {
                              numberTimesOptimal_++;
#ifdef CLP_DEBUG
                              if (handler_->logLevel() & 32)
                                   printf("%d times optimal, current %g best %g\n", numberTimesOptimal_,
                                          current, bestObjectiveWhenFlagged);
#endif
                         } else {
                              bestObjectiveWhenFlagged = current;
                         }
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
                         // put back original costs and then check
                         createRim(4);
                         nonLinearCost_->checkInfeasibilities(oldTolerance);
                         gutsOfSolution(NULL, NULL, true);
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
                    problemStatus_ = 0; // optimal
               }
          }
     } else {
          // see if looks unbounded
          if (problemStatus_ == -5) {
               if (nonLinearCost_->numberInfeasibilities()) {
                    if (infeasibilityCost_ > 1.0e18 && perturbation_ == 101) {
                         // back off weight
                         infeasibilityCost_ = 1.0e13;
                         unPerturb(); // stop any further perturbation
                    }
                    //we need infeasiblity cost changed
                    if (infeasibilityCost_ < 1.0e20) {
                         infeasibilityCost_ *= 5.0;
                         changeMade_++; // say change made
                         unflag();
                         handler_->message(CLP_PRIMAL_WEIGHT, messages_)
                                   << infeasibilityCost_
                                   << CoinMessageEol;
                         // put back original costs and then check
                         createRim(4);
                         gutsOfSolution(NULL, NULL, true);
                         problemStatus_ = -1; //continue
                    } else {
                         // say unbounded
                         problemStatus_ = 2;
                    }
               } else {
                    // say unbounded
                    problemStatus_ = 2;
               }
          } else {
               if(type == 3 && problemStatus_ != -5)
                    unflag(); // odd
               // carry on
               problemStatus_ = -1;
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
          CoinMemcpyN(status_, (numberColumns_ + numberRows_), saveStatus_);
          CoinMemcpyN(rowActivityWork_,	numberRows_, savedSolution_ + numberColumns_ );
          CoinMemcpyN(columnActivityWork_, numberColumns_, savedSolution_ );
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
     if (problemStatus_ < 0 && !changeMade_) {
          problemStatus_ = 4; // unknown
     }
     lastGoodIteration_ = numberIterations_;
     //if (goToDual)
     //problemStatus_=10; // try dual
     // Allow matrices to be sorted etc
     int fake = -999; // signal sort
     matrix_->correctSequence(this, fake, fake);
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
ClpSimplexNonlinear::whileIterating(int & pivotMode)
{
     // Say if values pass
     //int ifValuesPass=(firstFree_>=0) ? 1 : 0;
     int ifValuesPass = 1;
     int returnCode = -1;
     // status stays at -1 while iterating, >=0 finished, -2 to invert
     // status -3 to go to top without an invert
     int numberInterior = 0;
     int nextUnflag = 10;
     int nextUnflagIteration = numberIterations_ + 10;
     // get two arrays
     double * array1 = new double[2*(numberRows_+numberColumns_)];
     double solutionError = -1.0;
     while (problemStatus_ == -1) {
          int result;
          rowArray_[1]->clear();
          //#define CLP_DEBUG
#if CLP_DEBUG > 1
          rowArray_[0]->checkClear();
          rowArray_[1]->checkClear();
          rowArray_[2]->checkClear();
          rowArray_[3]->checkClear();
          columnArray_[0]->checkClear();
#endif
          if (numberInterior >= 5) {
               // this can go when we have better minimization
               if (pivotMode < 10)
                    pivotMode = 1;
               unflag();
#ifdef CLP_DEBUG
               if (handler_->logLevel() & 32)
                    printf("interior unflag\n");
#endif
               numberInterior = 0;
               nextUnflag = 10;
               nextUnflagIteration = numberIterations_ + 10;
          } else {
               if (numberInterior > nextUnflag &&
                         numberIterations_ > nextUnflagIteration) {
                    nextUnflagIteration = numberIterations_ + 10;
                    nextUnflag += 10;
                    unflag();
#ifdef CLP_DEBUG
                    if (handler_->logLevel() & 32)
                         printf("unflagging as interior\n");
#endif
               }
          }
          pivotRow_ = -1;
          result = pivotColumn(rowArray_[3], rowArray_[0],
                               columnArray_[0], rowArray_[1], pivotMode, solutionError,
                               array1);
          if (result) {
               if (result == 2 && sequenceIn_ < 0) {
                    // does not look good
                    double currentObj;
                    double thetaObj;
                    double predictedObj;
                    objective_->stepLength(this, solution_, solution_, 0.0,
                                           currentObj, thetaObj, predictedObj);
                    if (currentObj == predictedObj) {
#ifdef CLP_INVESTIGATE
                         printf("looks bad - no change in obj %g\n", currentObj);
#endif
                         if (factorization_->pivots())
                              result = 3;
                         else
                              problemStatus_ = 0;
                    }
               }
               if (result == 3)
                    break; // null vector not accurate
#ifdef CLP_DEBUG
               if (handler_->logLevel() & 32) {
                    double currentObj;
                    double thetaObj;
                    double predictedObj;
                    objective_->stepLength(this, solution_, solution_, 0.0,
                                           currentObj, thetaObj, predictedObj);
                    printf("obj %g after interior move\n", currentObj);
               }
#endif
               // just move and try again
               if (pivotMode < 10) {
                    pivotMode = result - 1;
                    numberInterior++;
               }
               continue;
          } else {
               if (pivotMode < 10) {
                    if (theta_ > 0.001)
                         pivotMode = 0;
                    else if (pivotMode == 2)
                         pivotMode = 1;
               }
               numberInterior = 0;
               nextUnflag = 10;
               nextUnflagIteration = numberIterations_ + 10;
          }
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
               // do second half of iteration
               if (pivotRow_ < 0 && theta_ < 1.0e-8) {
                    assert (sequenceIn_ >= 0);
                    returnCode = pivotResult(ifValuesPass);
               } else {
                    // specialized code
                    returnCode = pivotNonlinearResult();
                    //printf("odd pivrow %d\n",sequenceOut_);
                    if (sequenceOut_ >= 0 && theta_ < 1.0e-5) {
                         if (getStatus(sequenceOut_) != isFixed) {
                              if (getStatus(sequenceOut_) == atUpperBound)
                                   solution_[sequenceOut_] = upper_[sequenceOut_];
                              else if (getStatus(sequenceOut_) == atLowerBound)
                                   solution_[sequenceOut_] = lower_[sequenceOut_];
                              setFlagged(sequenceOut_);
                         }
                    }
               }
               if (returnCode < -1 && returnCode > -5) {
                    problemStatus_ = -2; //
               } else if (returnCode == -5) {
                    // something flagged - continue;
               } else if (returnCode == 2) {
                    problemStatus_ = -5; // looks unbounded
               } else if (returnCode == 4) {
                    problemStatus_ = -2; // looks unbounded but has iterated
               } else if (returnCode != -1) {
                    assert(returnCode == 3);
                    problemStatus_ = 3;
               }
          } else {
               // no pivot column
#ifdef CLP_DEBUG
               if (handler_->logLevel() & 32)
                    printf("** no column pivot\n");
#endif
               if (pivotMode < 10) {
                    // looks optimal
                    primalColumnPivot_->setLooksOptimal(true);
               } else {
                    pivotMode--;
#ifdef CLP_DEBUG
                    if (handler_->logLevel() & 32)
                         printf("pivot mode now %d\n", pivotMode);
#endif
                    if (pivotMode == 9)
                         pivotMode = 0;	// switch off fast attempt
                    unflag();
               }
               if (nonLinearCost_->numberInfeasibilities())
                    problemStatus_ = -4; // might be infeasible
               returnCode = 0;
               break;
          }
     }
     delete [] array1;
     return returnCode;
}
// Creates direction vector
void
ClpSimplexNonlinear::directionVector (CoinIndexedVector * vectorArray,
                                      CoinIndexedVector * spare1, CoinIndexedVector * spare2,
                                      int pivotMode2,
                                      double & normFlagged, double & normUnflagged,
                                      int & numberNonBasic)
{
#if CLP_DEBUG > 1
     vectorArray->checkClear();
     spare1->checkClear();
     spare2->checkClear();
#endif
     double *array = vectorArray->denseVector();
     int * index = vectorArray->getIndices();
     int number = 0;
     sequenceIn_ = -1;
     normFlagged = 0.0;
     normUnflagged = 1.0;
     double dualTolerance2 = CoinMin(1.0e-8, 1.0e-2 * dualTolerance_);
     double dualTolerance3 = CoinMin(1.0e-2, 1.0e3 * dualTolerance_);
     if (!numberNonBasic) {
          //if (nonLinearCost_->sumInfeasibilities()>1.0e-4)
          //printf("infeasible\n");
          if (!pivotMode2 || pivotMode2 >= 10) {
               normUnflagged = 0.0;
               double bestDj = 0.0;
               double bestSuper = 0.0;
               double sumSuper = 0.0;
               sequenceIn_ = -1;
               int nSuper = 0;
               for (int iSequence = 0; iSequence < numberColumns_ + numberRows_; iSequence++) {
                    array[iSequence] = 0.0;
                    if (flagged(iSequence)) {
                         // accumulate  norm
                         switch(getStatus(iSequence)) {

                         case basic:
                         case ClpSimplex::isFixed:
                              break;
                         case atUpperBound:
                              if (dj_[iSequence] > dualTolerance3)
                                   normFlagged += dj_[iSequence] * dj_[iSequence];
                              break;
                         case atLowerBound:
                              if (dj_[iSequence] < -dualTolerance3)
                                   normFlagged += dj_[iSequence] * dj_[iSequence];
                              break;
                         case isFree:
                         case superBasic:
                              if (fabs(dj_[iSequence]) > dualTolerance3)
                                   normFlagged += dj_[iSequence] * dj_[iSequence];
                              break;
                         }
                         continue;
                    }
                    switch(getStatus(iSequence)) {

                    case basic:
                    case ClpSimplex::isFixed:
                         break;
                    case atUpperBound:
                         if (dj_[iSequence] > dualTolerance_) {
                              if (dj_[iSequence] > dualTolerance3)
                                   normUnflagged += dj_[iSequence] * dj_[iSequence];
                              if (pivotMode2 < 10) {
                                   array[iSequence] = -dj_[iSequence];
                                   index[number++] = iSequence;
                              } else {
                                   if (dj_[iSequence] > bestDj) {
                                        bestDj = dj_[iSequence];
                                        sequenceIn_ = iSequence;
                                   }
                              }
                         }
                         break;
                    case atLowerBound:
                         if (dj_[iSequence] < -dualTolerance_) {
                              if (dj_[iSequence] < -dualTolerance3)
                                   normUnflagged += dj_[iSequence] * dj_[iSequence];
                              if (pivotMode2 < 10) {
                                   array[iSequence] = -dj_[iSequence];
                                   index[number++] = iSequence;
                              } else {
                                   if (-dj_[iSequence] > bestDj) {
                                        bestDj = -dj_[iSequence];
                                        sequenceIn_ = iSequence;
                                   }
                              }
                         }
                         break;
                    case isFree:
                    case superBasic:
                         //#define ALLSUPER
#define NOSUPER
#ifndef ALLSUPER
                         if (fabs(dj_[iSequence]) > dualTolerance_) {
                              if (fabs(dj_[iSequence]) > dualTolerance3)
                                   normUnflagged += dj_[iSequence] * dj_[iSequence];
                              nSuper++;
                              bestSuper = CoinMax(fabs(dj_[iSequence]), bestSuper);
                              sumSuper += fabs(dj_[iSequence]);
                         }
                         if (fabs(dj_[iSequence]) > dualTolerance2) {
                              array[iSequence] = -dj_[iSequence];
                              index[number++] = iSequence;
                         }
#else
                         array[iSequence] = -dj_[iSequence];
                         index[number++] = iSequence;
                         if (pivotMode2 >= 10)
                              bestSuper = CoinMax(fabs(dj_[iSequence]), bestSuper);
#endif
                         break;
                    }
               }
#ifdef NOSUPER
               // redo
               bestSuper = sumSuper;
               if(sequenceIn_ >= 0 && bestDj > bestSuper) {
                    int j;
                    // get rid of superbasics
                    for (j = 0; j < number; j++) {
                         int iSequence = index[j];
                         array[iSequence] = 0.0;
                    }
                    number = 0;
                    array[sequenceIn_] = -dj_[sequenceIn_];
                    index[number++] = sequenceIn_;
               } else {
                    sequenceIn_ = -1;
               }
#else
               if (pivotMode2 >= 10 || !nSuper) {
                    bool takeBest = true;
                    if (pivotMode2 == 100 && nSuper > 1)
                         takeBest = false;
                    if(sequenceIn_ >= 0 && takeBest) {
                         if (fabs(dj_[sequenceIn_]) > bestSuper) {
                              array[sequenceIn_] = -dj_[sequenceIn_];
                              index[number++] = sequenceIn_;
                         } else {
                              sequenceIn_ = -1;
                         }
                    } else {
                         sequenceIn_ = -1;
                    }
               }
#endif
#ifdef CLP_DEBUG
               if (handler_->logLevel() & 32) {
                    if (sequenceIn_ >= 0)
                         printf("%d superBasic, chosen %d - dj %g\n", nSuper, sequenceIn_,
                                dj_[sequenceIn_]);
                    else
                         printf("%d superBasic - none chosen\n", nSuper);
               }
#endif
          } else {
               double bestDj = 0.0;
               double saveDj = 0.0;
               if (sequenceOut_ >= 0) {
                    saveDj = dj_[sequenceOut_];
                    dj_[sequenceOut_] = 0.0;
                    switch(getStatus(sequenceOut_)) {

                    case basic:
                         sequenceOut_ = -1;
                    case ClpSimplex::isFixed:
                         break;
                    case atUpperBound:
                         if (dj_[sequenceOut_] > dualTolerance_) {
#ifdef CLP_DEBUG
                              if (handler_->logLevel() & 32)
                                   printf("after pivot out %d values %g %g %g, dj %g\n",
                                          sequenceOut_, lower_[sequenceOut_], solution_[sequenceOut_],
                                          upper_[sequenceOut_], dj_[sequenceOut_]);
#endif
                         }
                         break;
                    case atLowerBound:
                         if (dj_[sequenceOut_] < -dualTolerance_) {
#ifdef CLP_DEBUG
                              if (handler_->logLevel() & 32)
                                   printf("after pivot out %d values %g %g %g, dj %g\n",
                                          sequenceOut_, lower_[sequenceOut_], solution_[sequenceOut_],
                                          upper_[sequenceOut_], dj_[sequenceOut_]);
#endif
                         }
                         break;
                    case isFree:
                    case superBasic:
                         if (dj_[sequenceOut_] > dualTolerance_) {
#ifdef CLP_DEBUG
                              if (handler_->logLevel() & 32)
                                   printf("after pivot out %d values %g %g %g, dj %g\n",
                                          sequenceOut_, lower_[sequenceOut_], solution_[sequenceOut_],
                                          upper_[sequenceOut_], dj_[sequenceOut_]);
#endif
                         } else if (dj_[sequenceOut_] < -dualTolerance_) {
#ifdef CLP_DEBUG
                              if (handler_->logLevel() & 32)
                                   printf("after pivot out %d values %g %g %g, dj %g\n",
                                          sequenceOut_, lower_[sequenceOut_], solution_[sequenceOut_],
                                          upper_[sequenceOut_], dj_[sequenceOut_]);
#endif
                         }
                         break;
                    }
               }
               // Go for dj
               pivotMode2 = 3;
               for (int iSequence = 0; iSequence < numberColumns_ + numberRows_; iSequence++) {
                    array[iSequence] = 0.0;
                    if (flagged(iSequence))
                         continue;
                    switch(getStatus(iSequence)) {

                    case basic:
                    case ClpSimplex::isFixed:
                         break;
                    case atUpperBound:
                         if (dj_[iSequence] > dualTolerance_) {
                              double distance = CoinMin(1.0e-2, solution_[iSequence] - lower_[iSequence]);
                              double merit = distance * dj_[iSequence];
                              if (pivotMode2 == 1)
                                   merit *= 1.0e-20; // discourage
                              if (pivotMode2 == 3)
                                   merit = fabs(dj_[iSequence]);
                              if (merit > bestDj) {
                                   sequenceIn_ = iSequence;
                                   bestDj = merit;
                              }
                         }
                         break;
                    case atLowerBound:
                         if (dj_[iSequence] < -dualTolerance_) {
                              double distance = CoinMin(1.0e-2, upper_[iSequence] - solution_[iSequence]);
                              double merit = -distance * dj_[iSequence];
                              if (pivotMode2 == 1)
                                   merit *= 1.0e-20; // discourage
                              if (pivotMode2 == 3)
                                   merit = fabs(dj_[iSequence]);
                              if (merit > bestDj) {
                                   sequenceIn_ = iSequence;
                                   bestDj = merit;
                              }
                         }
                         break;
                    case isFree:
                    case superBasic:
                         if (dj_[iSequence] > dualTolerance_) {
                              double distance = CoinMin(1.0e-2, solution_[iSequence] - lower_[iSequence]);
                              distance = CoinMin(solution_[iSequence] - lower_[iSequence],
                                                 upper_[iSequence] - solution_[iSequence]);
                              double merit = distance * dj_[iSequence];
                              if (pivotMode2 == 1)
                                   merit = distance;
                              if (pivotMode2 == 3)
                                   merit = fabs(dj_[iSequence]);
                              if (merit > bestDj) {
                                   sequenceIn_ = iSequence;
                                   bestDj = merit;
                              }
                         } else if (dj_[iSequence] < -dualTolerance_) {
                              double distance = CoinMin(1.0e-2, upper_[iSequence] - solution_[iSequence]);
                              distance = CoinMin(solution_[iSequence] - lower_[iSequence],
                                                 upper_[iSequence] - solution_[iSequence]);
                              double merit = -distance * dj_[iSequence];
                              if (pivotMode2 == 1)
                                   merit = distance;
                              if (pivotMode2 == 3)
                                   merit = fabs(dj_[iSequence]);
                              if (merit > bestDj) {
                                   sequenceIn_ = iSequence;
                                   bestDj = merit;
                              }
                         }
                         break;
                    }
               }
               if (sequenceOut_ >= 0) {
                    dj_[sequenceOut_] = saveDj;
                    sequenceOut_ = -1;
               }
               if (sequenceIn_ >= 0) {
                    array[sequenceIn_] = -dj_[sequenceIn_];
                    index[number++] = sequenceIn_;
               }
          }
          numberNonBasic = number;
     } else {
          // compute norms
          normUnflagged = 0.0;
          for (int iSequence = 0; iSequence < numberColumns_ + numberRows_; iSequence++) {
               if (flagged(iSequence)) {
                    // accumulate  norm
                    switch(getStatus(iSequence)) {

                    case basic:
                    case ClpSimplex::isFixed:
                         break;
                    case atUpperBound:
                         if (dj_[iSequence] > dualTolerance_)
                              normFlagged += dj_[iSequence] * dj_[iSequence];
                         break;
                    case atLowerBound:
                         if (dj_[iSequence] < -dualTolerance_)
                              normFlagged += dj_[iSequence] * dj_[iSequence];
                         break;
                    case isFree:
                    case superBasic:
                         if (fabs(dj_[iSequence]) > dualTolerance_)
                              normFlagged += dj_[iSequence] * dj_[iSequence];
                         break;
                    }
               }
          }
          // re-use list
          number = 0;
          int j;
          for (j = 0; j < numberNonBasic; j++) {
               int iSequence = index[j];
               if (flagged(iSequence))
                    continue;
               switch(getStatus(iSequence)) {

               case basic:
               case ClpSimplex::isFixed:
                    continue; //abort();
                    break;
               case atUpperBound:
                    if (dj_[iSequence] > dualTolerance_) {
                         number++;
                         normUnflagged += dj_[iSequence] * dj_[iSequence];
                    }
                    break;
               case atLowerBound:
                    if (dj_[iSequence] < -dualTolerance_) {
                         number++;
                         normUnflagged += dj_[iSequence] * dj_[iSequence];
                    }
                    break;
               case isFree:
               case superBasic:
                    if (fabs(dj_[iSequence]) > dualTolerance_) {
                         number++;
                         normUnflagged += dj_[iSequence] * dj_[iSequence];
                    }
                    break;
               }
               array[iSequence] = -dj_[iSequence];
          }
          // switch to large
          normUnflagged = 1.0;
          if (!number) {
               for (j = 0; j < numberNonBasic; j++) {
                    int iSequence = index[j];
                    array[iSequence] = 0.0;
               }
               numberNonBasic = 0;
          }
          number = numberNonBasic;
     }
     if (number) {
          int j;
          // Now do basic ones - how do I compensate for small basic infeasibilities?
          int iRow;
          for (iRow = 0; iRow < numberRows_; iRow++) {
               int iPivot = pivotVariable_[iRow];
               double value = 0.0;
               if (solution_[iPivot] > upper_[iPivot]) {
                    value = upper_[iPivot] - solution_[iPivot];
               } else if (solution_[iPivot] < lower_[iPivot]) {
                    value = lower_[iPivot] - solution_[iPivot];
               }
               //if (value)
               //printf("inf %d %g %g %g\n",iPivot,lower_[iPivot],solution_[iPivot],
               //     upper_[iPivot]);
               //value=0.0;
               value *= -1.0e0;
               if (value) {
                    array[iPivot] = value;
                    index[number++] = iPivot;
               }
          }
          double * array2 = spare1->denseVector();
          int * index2 = spare1->getIndices();
          int number2 = 0;
          times(-1.0, array, array2);
          array = array + numberColumns_;
          for (iRow = 0; iRow < numberRows_; iRow++) {
               double value = array2[iRow] + array[iRow];
               if (value) {
                    array2[iRow] = value;
                    index2[number2++] = iRow;
               } else {
                    array2[iRow] = 0.0;
               }
          }
          array -= numberColumns_;
          spare1->setNumElements(number2);
          // Ftran
          factorization_->updateColumn(spare2, spare1);
          number2 = spare1->getNumElements();
          for (j = 0; j < number2; j++) {
               int iSequence = index2[j];
               double value = array2[iSequence];
               array2[iSequence] = 0.0;
               if (value) {
                    int iPivot = pivotVariable_[iSequence];
                    double oldValue = array[iPivot];
                    if (!oldValue) {
                         array[iPivot] = value;
                         index[number++] = iPivot;
                    } else {
                         // something already there
                         array[iPivot] = value + oldValue;
                    }
               }
          }
          spare1->setNumElements(0);
     }
     vectorArray->setNumElements(number);
}
#define MINTYPE 1
#if MINTYPE==2
static double
innerProductIndexed(const double * region1, int size, const double * region2, const int * which)
{
     int i;
     double value = 0.0;
     for (i = 0; i < size; i++) {
          int j = which[i];
          value += region1[j] * region2[j];
     }
     return value;
}
#endif
/*
   Row array and column array have direction
   Returns 0 - can do normal iteration (basis change)
   1 - no basis change
*/
int
ClpSimplexNonlinear::pivotColumn(CoinIndexedVector * longArray,
                                 CoinIndexedVector * rowArray,
                                 CoinIndexedVector * columnArray,
                                 CoinIndexedVector * spare,
                                 int & pivotMode,
                                 double & solutionError,
                                 double * dArray)
{
     // say not optimal
     primalColumnPivot_->setLooksOptimal(false);
     double acceptablePivot = 1.0e-10;
     int lastSequenceIn = -1;
     if (pivotMode && pivotMode < 10) {
          acceptablePivot = 1.0e-6;
          if (factorization_->pivots())
               acceptablePivot = 1.0e-5; // if we have iterated be more strict
     }
     double acceptableBasic = 1.0e-7;

     int number = longArray->getNumElements();
     int numberTotal = numberRows_ + numberColumns_;
     int bestSequence = -1;
     int bestBasicSequence = -1;
     double eps = 1.0e-1;
     eps = 1.0e-6;
     double basicTheta = 1.0e30;
     double objTheta = 0.0;
     bool finished = false;
     sequenceIn_ = -1;
     int nPasses = 0;
     int nTotalPasses = 0;
     int nBigPasses = 0;
     double djNorm0 = 0.0;
     double djNorm = 0.0;
     double normFlagged = 0.0;
     double normUnflagged = 0.0;
     int localPivotMode = pivotMode;
     bool allFinished = false;
     bool justOne = false;
     int returnCode = 1;
     double currentObj;
     double predictedObj;
     double thetaObj;
     objective_->stepLength(this, solution_, solution_, 0.0,
                            currentObj, predictedObj, thetaObj);
     double saveObj = currentObj;
#if MINTYPE ==2
     // try Shanno's method
     //would be memory leak
     //double * saveY=new double[numberTotal];
     //double * saveS=new double[numberTotal];
     //double * saveY2=new double[numberTotal];
     //double * saveS2=new double[numberTotal];
     double saveY[100];
     double saveS[100];
     double saveY2[100];
     double saveS2[100];
     double zz[10000];
#endif
     double * dArray2 = dArray + numberTotal;
     // big big loop
     while (!allFinished) {
          double * work = longArray->denseVector();
          int * which = longArray->getIndices();
          allFinished = true;
          // CONJUGATE 0 - never, 1 as pivotMode, 2 as localPivotMode, 3 always
          //#define SMALLTHETA1 1.0e-25
          //#define SMALLTHETA2 1.0e-25
#define SMALLTHETA1 1.0e-10
#define SMALLTHETA2 1.0e-10
#define CONJUGATE 2
#if CONJUGATE == 0
          int conjugate = 0;
#elif CONJUGATE == 1
          int conjugate = (pivotMode < 10) ? MINTYPE : 0;
#elif CONJUGATE == 2
          int conjugate = MINTYPE;
#else
          int conjugate = MINTYPE;
#endif

          //if (pivotMode==1)
          //localPivotMode=11;;
#if CLP_DEBUG > 1
          longArray->checkClear();
#endif
          int numberNonBasic = 0;
          int startLocalMode = -1;
          while (!finished) {
               double simpleObjective = COIN_DBL_MAX;
               returnCode = 1;
               int iSequence;
               objective_->reducedGradient(this, dj_, false);
               // get direction vector in longArray
               longArray->clear();
               // take out comment to try slightly different idea
               if (nPasses + nTotalPasses > 3000 || nBigPasses > 100) {
                    if (factorization_->pivots())
                         returnCode = 3;
                    break;
               }
               if (!nPasses) {
                    numberNonBasic = 0;
                    nBigPasses++;
               }
               // just do superbasic if in cleanup mode
               int local = localPivotMode;
               if (!local && pivotMode >= 10 && nBigPasses < 10) {
                    local = 100;
               } else if (local == -1 || nBigPasses >= 10) {
                    local = 0;
                    localPivotMode = 0;
               }
               if (justOne) {
                    local = 2;
                    //local=100;
                    justOne = false;
               }
               if (!nPasses)
                    startLocalMode = local;
               directionVector(longArray, spare, rowArray, local,
                               normFlagged, normUnflagged, numberNonBasic);
               {
                    // check null vector
                    double * rhs = spare->denseVector();
#if CLP_DEBUG > 1
                    spare->checkClear();
#endif
                    int iRow;
                    multiplyAdd(solution_ + numberColumns_, numberRows_, -1.0, rhs, 0.0);
                    matrix_->times(1.0, solution_, rhs, rowScale_, columnScale_);
                    double largest = 0.0;
#if CLP_DEBUG > 0
                    int iLargest = -1;
#endif
                    for (iRow = 0; iRow < numberRows_; iRow++) {
                         double value = fabs(rhs[iRow]);
                         rhs[iRow] = 0.0;
                         if (value > largest) {
                              largest = value;
#if CLP_DEBUG > 0
                              iLargest = iRow;
#endif
                         }
                    }
#if CLP_DEBUG > 0
                    if ((handler_->logLevel() & 32) && largest > 1.0e-8)
                         printf("largest rhs error %g on row %d\n", largest, iLargest);
#endif
                    if (solutionError < 0.0) {
                         solutionError = largest;
                    } else if (largest > CoinMax(1.0e-8, 1.0e2 * solutionError) &&
                               factorization_->pivots()) {
                         longArray->clear();
                         pivotRow_ = -1;
                         theta_ = 0.0;
                         return 3;
                    }
               }
               if (sequenceIn_ >= 0)
                    lastSequenceIn = sequenceIn_;
#if MINTYPE!=2
               double djNormSave = djNorm;
#endif
               djNorm = 0.0;
               int iIndex;
               for (iIndex = 0; iIndex < numberNonBasic; iIndex++) {
                    int iSequence = which[iIndex];
                    double alpha = work[iSequence];
                    djNorm += alpha * alpha;
               }
               // go to conjugate gradient if necessary
               if (numberNonBasic && localPivotMode >= 10 && (nPasses > 4 || sequenceIn_ < 0)) {
                    localPivotMode = 0;
                    nTotalPasses += nPasses;
                    nPasses = 0;
               }
#if CONJUGATE == 2
               conjugate = (localPivotMode < 10) ? MINTYPE : 0;
#endif
               //printf("bigP %d pass %d nBasic %d norm %g normI %g normF %g\n",
               //     nBigPasses,nPasses,numberNonBasic,normUnflagged,normFlagged);
               if (!nPasses) {
                    //printf("numberNon %d\n",numberNonBasic);
#if MINTYPE==2
                    assert (numberNonBasic < 100);
                    memset(zz, 0, numberNonBasic * numberNonBasic * sizeof(double));
                    int put = 0;
                    for (int iVariable = 0; iVariable < numberNonBasic; iVariable++) {
                         zz[put] = 1.0;
                         put += numberNonBasic + 1;
                    }
#endif
                    djNorm0 = CoinMax(djNorm, 1.0e-20);
                    CoinMemcpyN(work, numberTotal, dArray);
                    CoinMemcpyN(work, numberTotal, dArray2);
                    if (sequenceIn_ >= 0 && numberNonBasic == 1) {
                         // see if simple move
                         double objTheta2 = objective_->stepLength(this, solution_, work, 1.0e30,
                                            currentObj, predictedObj, thetaObj);
                         rowArray->clear();

                         // update the incoming column
                         unpackPacked(rowArray);
                         factorization_->updateColumnFT(spare, rowArray);
                         theta_ = 0.0;
                         double * work2 = rowArray->denseVector();
                         int number = rowArray->getNumElements();
                         int * which2 = rowArray->getIndices();
                         int iIndex;
                         bool easyMove = false;
                         double way;
                         if (dj_[sequenceIn_] > 0.0)
                              way = -1.0;
                         else
                              way = 1.0;
                         double largest = COIN_DBL_MAX;
#ifdef CLP_DEBUG
                         int kPivot = -1;
#endif
                         for (iIndex = 0; iIndex < number; iIndex++) {
                              int iRow = which2[iIndex];
                              double alpha = way * work2[iIndex];
                              int iPivot = pivotVariable_[iRow];
                              if (alpha < -1.0e-5) {
                                   double distance = upper_[iPivot] - solution_[iPivot];
                                   if (distance < -largest * alpha) {
#ifdef CLP_DEBUG
                                        kPivot = iPivot;
#endif
                                        largest = CoinMax(0.0, -distance / alpha);
                                   }
                                   if (distance < -1.0e-12 * alpha) {
                                        easyMove = true;
                                        break;
                                   }
                              } else if (alpha > 1.0e-5) {
                                   double distance = solution_[iPivot] - lower_[iPivot];
                                   if (distance < largest * alpha) {
#ifdef CLP_DEBUG
                                        kPivot = iPivot;
#endif
                                        largest = CoinMax(0.0, distance / alpha);
                                   }
                                   if (distance < 1.0e-12 * alpha) {
                                        easyMove = true;
                                        break;
                                   }
                              }
                         }
                         // But largest has to match up with change
                         assert (work[sequenceIn_]);
                         largest /= fabs(work[sequenceIn_]);
                         if (largest < objTheta2) {
                              easyMove = true;
                         } else if (!easyMove) {
                              double objDrop = currentObj - predictedObj;
                              double th = objective_->stepLength(this, solution_, work, largest,
                                                                 currentObj, predictedObj, simpleObjective);
                              simpleObjective = CoinMax(simpleObjective, predictedObj);
                              double easyDrop = currentObj - simpleObjective;
                              if (easyDrop > 1.0e-8 && easyDrop > 0.5 * objDrop) {
                                   easyMove = true;
#ifdef CLP_DEBUG
                                   if (handler_->logLevel() & 32)
                                        printf("easy - obj drop %g, easy drop %g\n", objDrop, easyDrop);
#endif
                                   if (easyDrop > objDrop) {
                                        // debug
                                        printf("****** th %g simple %g\n", th, simpleObjective);
                                        objective_->stepLength(this, solution_, work, 1.0e30,
                                                               currentObj, predictedObj, simpleObjective);
                                        objective_->stepLength(this, solution_, work, largest,
                                                               currentObj, predictedObj, simpleObjective);
                                   }
                              }
                         }
                         rowArray->clear();
#ifdef CLP_DEBUG
                         if (handler_->logLevel() & 32)
                              printf("largest %g piv %d\n", largest, kPivot);
#endif
                         if (easyMove) {
                              valueIn_ = solution_[sequenceIn_];
                              dualIn_ = dj_[sequenceIn_];
                              lowerIn_ = lower_[sequenceIn_];
                              upperIn_ = upper_[sequenceIn_];
                              if (dualIn_ > 0.0)
                                   directionIn_ = -1;
                              else
                                   directionIn_ = 1;
                              longArray->clear();
                              pivotRow_ = -1;
                              theta_ = 0.0;
                              return 0;
                         }
                    }
               } else {
#if MINTYPE==1
                    if (conjugate) {
                         double djNorm2 = djNorm;
                         if (numberNonBasic && 0) {
                              int iIndex;
                              djNorm2 = 0.0;
                              for (iIndex = 0; iIndex < numberNonBasic; iIndex++) {
                                   int iSequence = which[iIndex];
                                   double alpha = work[iSequence];
                                   //djNorm2 += alpha*alpha;
                                   double alpha2 = work[iSequence] - dArray2[iSequence];
                                   djNorm2 += alpha * alpha2;
                              }
                              //printf("a %.18g b %.18g\n",djNorm,djNorm2);
                         }
                         djNorm = djNorm2;
                         double beta = djNorm2 / djNormSave;
                         // reset beta every so often
                         //if (numberNonBasic&&nPasses>numberNonBasic&&(nPasses%(3*numberNonBasic))==1)
                         //beta=0.0;
                         //printf("current norm %g, old %g - beta %g\n",
                         //     djNorm,djNormSave,beta);
                         for (iSequence = 0; iSequence < numberTotal; iSequence++) {
                              dArray[iSequence] = work[iSequence] + beta * dArray[iSequence];
                              dArray2[iSequence] = work[iSequence];
                         }
                    } else {
                         for (iSequence = 0; iSequence < numberTotal; iSequence++)
                              dArray[iSequence] = work[iSequence];
                    }
#else
                    int number2 = numberNonBasic;
                    if (number2) {
                         // pack down into dArray
                         int jLast = -1;
                         for (iSequence = 0; iSequence < numberNonBasic; iSequence++) {
                              int j = which[iSequence];
                              assert(j > jLast);
                              jLast = j;
                              double value = work[j];
                              dArray[iSequence] = -value;
                         }
                         // see whether to restart
                         // check signs - gradient
                         double g1 = innerProduct(dArray, number2, dArray);
                         double g2 = innerProduct(dArray, number2, saveY2);
                         // Get differences
                         for (iSequence = 0; iSequence < numberNonBasic; iSequence++) {
                              saveY2[iSequence] = dArray[iSequence] - saveY2[iSequence];
                              saveS2[iSequence] = solution_[iSequence] - saveS2[iSequence];
                         }
                         double g3 = innerProduct(saveS2, number2, saveY2);
                         printf("inner %g\n", g3);
                         //assert(g3>0);
                         double zzz[10000];
                         int iVariable;
                         g2 = 1.0e50; // temp
                         if (fabs(g2) >= 0.2 * fabs(g1)) {
                              // restart
                              double delta = innerProduct(saveY2, number2, saveS2) /
                                             innerProduct(saveY2, number2, saveY2);
                              delta = 1.0; //temp
                              memset(zz, 0, number2 * sizeof(double));
                              int put = 0;
                              for (iVariable = 0; iVariable < number2; iVariable++) {
                                   zz[put] = delta;
                                   put += number2 + 1;
                              }
                         } else {
                         }
                         CoinMemcpyN(zz, number2 * number2, zzz);
                         double ww[100];
                         // get sk -Hkyk
                         for (iVariable = 0; iVariable < number2; iVariable++) {
                              double value = 0.0;
                              for (int jVariable = 0; jVariable < number2; jVariable++) {
                                   value += saveY2[jVariable] * zzz[iVariable+number2*jVariable];
                              }
                              ww[iVariable] = saveS2[iVariable] - value;
                         }
                         double ys = innerProduct(saveY2, number2, saveS2);
                         double multiplier1 = 1.0 / ys;
                         double multiplier2 = innerProduct(saveY2, number2, ww) / (ys * ys);
#if 1
                         // and second way
                         // Hy
                         double h[100];
                         for (iVariable = 0; iVariable < number2; iVariable++) {
                              double value = 0.0;
                              for (int jVariable = 0; jVariable < number2; jVariable++) {
                                   value += saveY2[jVariable] * zzz[iVariable+number2*jVariable];
                              }
                              h[iVariable] = value;
                         }
                         double hh[10000];
                         double yhy1 = innerProduct(h, number2, saveY2) * multiplier1 + 1.0;
                         yhy1 *= multiplier1;
                         for (iVariable = 0; iVariable < number2; iVariable++) {
                              for (int jVariable = 0; jVariable < number2; jVariable++) {
                                   int put = iVariable + number2 * jVariable;
                                   double value = zzz[put];
                                   value += yhy1 * saveS2[iVariable] * saveS2[jVariable];
                                   hh[put] = value;
                              }
                         }
                         for (iVariable = 0; iVariable < number2; iVariable++) {
                              for (int jVariable = 0; jVariable < number2; jVariable++) {
                                   int put = iVariable + number2 * jVariable;
                                   double value = hh[put];
                                   value -= multiplier1 * (saveS2[iVariable] * h[jVariable]);
                                   value -= multiplier1 * (saveS2[jVariable] * h[iVariable]);
                                   hh[put] = value;
                              }
                         }
#endif
                         // now update H
                         for (iVariable = 0; iVariable < number2; iVariable++) {
                              for (int jVariable = 0; jVariable < number2; jVariable++) {
                                   int put = iVariable + number2 * jVariable;
                                   double value = zzz[put];
                                   value += multiplier1 * (ww[iVariable] * saveS2[jVariable]
                                                           + ww[jVariable] * saveS2[iVariable]);
                                   value -= multiplier2 * saveS2[iVariable] * saveS2[jVariable];
                                   zzz[put] = value;
                              }
                         }
                         //memcpy(zzz,hh,size*sizeof(double));
                         // do search direction
                         memset(dArray, 0, numberTotal * sizeof(double));
                         for (iVariable = 0; iVariable < numberNonBasic; iVariable++) {
                              double value = 0.0;
                              for (int jVariable = 0; jVariable < number2; jVariable++) {
                                   int k = which[jVariable];
                                   value += work[k] * zzz[iVariable+number2*jVariable];
                              }
                              int i = which[iVariable];
                              dArray[i] = value;
                         }
                         // Now fill out dArray
                         {
                              int j;
                              // Now do basic ones
                              int iRow;
                              CoinIndexedVector * spare1 = spare;
                              CoinIndexedVector * spare2 = rowArray;
#if CLP_DEBUG > 1
                              spare1->checkClear();
                              spare2->checkClear();
#endif
                              double * array2 = spare1->denseVector();
                              int * index2 = spare1->getIndices();
                              int number2 = 0;
                              times(-1.0, dArray, array2);
                              dArray = dArray + numberColumns_;
                              for (iRow = 0; iRow < numberRows_; iRow++) {
                                   double value = array2[iRow] + dArray[iRow];
                                   if (value) {
                                        array2[iRow] = value;
                                        index2[number2++] = iRow;
                                   } else {
                                        array2[iRow] = 0.0;
                                   }
                              }
                              dArray -= numberColumns_;
                              spare1->setNumElements(number2);
                              // Ftran
                              factorization_->updateColumn(spare2, spare1);
                              number2 = spare1->getNumElements();
                              for (j = 0; j < number2; j++) {
                                   int iSequence = index2[j];
                                   double value = array2[iSequence];
                                   array2[iSequence] = 0.0;
                                   if (value) {
                                        int iPivot = pivotVariable_[iSequence];
                                        double oldValue = dArray[iPivot];
                                        dArray[iPivot] = value + oldValue;
                                   }
                              }
                              spare1->setNumElements(0);
                         }
                         //assert (innerProductIndexed(dArray,number2,work,which)>0);
                         //printf ("innerP1 %g\n",innerProduct(dArray,numberTotal,work));
                         printf ("innerP1 %g innerP2 %g\n", innerProduct(dArray, numberTotal, work),
                                 innerProductIndexed(dArray, numberNonBasic, work, which));
                         assert (innerProduct(dArray, numberTotal, work) > 0);
#if 1
                         {
                              // check null vector
                              int iRow;
                              double qq[10];
                              memset(qq, 0, numberRows_ * sizeof(double));
                              multiplyAdd(dArray + numberColumns_, numberRows_, -1.0, qq, 0.0);
                              matrix_->times(1.0, dArray, qq, rowScale_, columnScale_);
                              double largest = 0.0;
                              int iLargest = -1;
                              for (iRow = 0; iRow < numberRows_; iRow++) {
                                   double value = fabs(qq[iRow]);
                                   if (value > largest) {
                                        largest = value;
                                        iLargest = iRow;
                                   }
                              }
                              printf("largest null error %g on row %d\n", largest, iLargest);
                              for (iSequence = 0; iSequence < numberTotal; iSequence++) {
                                   if (getStatus(iSequence) == basic)
                                        assert (fabs(dj_[iSequence]) < 1.0e-3);
                              }
                         }
#endif
                         CoinMemcpyN(saveY2, numberNonBasic, saveY);
                         CoinMemcpyN(saveS2, numberNonBasic, saveS);
                    }
#endif
               }
#if MINTYPE==2
               for (iSequence = 0; iSequence < numberNonBasic; iSequence++) {
                    int j = which[iSequence];
                    saveY2[iSequence] = -work[j];
                    saveS2[iSequence] = solution_[j];
               }
#endif
               if (djNorm < eps * djNorm0 || (nPasses > 100 && djNorm < CoinMin(1.0e-1 * djNorm0, 1.0e-12))) {
#ifdef CLP_DEBUG
                    if (handler_->logLevel() & 32)
                         printf("dj norm reduced from %g to %g\n", djNorm0, djNorm);
#endif
                    if (pivotMode < 10 || !numberNonBasic) {
                         finished = true;
                    } else {
                         localPivotMode = pivotMode;
                         nTotalPasses += nPasses;
                         nPasses = 0;
                         continue;
                    }
               }
               //if (nPasses==100||nPasses==50)
               //printf("pass %d dj norm reduced from %g to %g - flagged norm %g\n",nPasses,djNorm0,djNorm,
               //	 normFlagged);
               if (nPasses > 100 && djNorm < 1.0e-2 * normFlagged && !startLocalMode) {
#ifdef CLP_DEBUG
                    if (handler_->logLevel() & 32)
                         printf("dj norm reduced from %g to %g - flagged norm %g - unflagging\n", djNorm0, djNorm,
                                normFlagged);
#endif
                    unflag();
                    localPivotMode = 0;
                    nTotalPasses += nPasses;
                    nPasses = 0;
                    continue;
               }
               if (djNorm > 0.99 * djNorm0 && nPasses > 1500) {
                    finished = true;
#ifdef CLP_DEBUG
                    if (handler_->logLevel() & 32)
                         printf("dj norm NOT reduced from %g to %g\n", djNorm0, djNorm);
#endif
                    djNorm = 1.2345e-20;
               }
               number = longArray->getNumElements();
               if (!numberNonBasic) {
                    // looks optimal
                    // check if any dj look plausible
                    int nSuper = 0;
                    int nFlagged = 0;
                    int nNormal = 0;
                    for (int iSequence = 0; iSequence < numberColumns_ + numberRows_; iSequence++) {
                         if (flagged(iSequence)) {
                              switch(getStatus(iSequence)) {

                              case basic:
                              case ClpSimplex::isFixed:
                                   break;
                              case atUpperBound:
                                   if (dj_[iSequence] > dualTolerance_)
                                        nFlagged++;
                                   break;
                              case atLowerBound:
                                   if (dj_[iSequence] < -dualTolerance_)
                                        nFlagged++;
                                   break;
                              case isFree:
                              case superBasic:
                                   if (fabs(dj_[iSequence]) > dualTolerance_)
                                        nFlagged++;
                                   break;
                              }
                              continue;
                         }
                         switch(getStatus(iSequence)) {

                         case basic:
                         case ClpSimplex::isFixed:
                              break;
                         case atUpperBound:
                              if (dj_[iSequence] > dualTolerance_)
                                   nNormal++;
                              break;
                         case atLowerBound:
                              if (dj_[iSequence] < -dualTolerance_)
                                   nNormal++;
                              break;
                         case isFree:
                         case superBasic:
                              if (fabs(dj_[iSequence]) > dualTolerance_)
                                   nSuper++;
                              break;
                         }
                    }
#ifdef CLP_DEBUG
                    if (handler_->logLevel() & 32)
                         printf("%d super, %d normal, %d flagged\n",
                                nSuper, nNormal, nFlagged);
#endif

                    int nFlagged2 = 1;
                    if (lastSequenceIn < 0 && !nNormal && !nSuper) {
                         nFlagged2 = unflag();
                         if (pivotMode >= 10) {
                              pivotMode--;
#ifdef CLP_DEBUG
                              if (handler_->logLevel() & 32)
                                   printf("pivot mode now %d\n", pivotMode);
#endif
                              if (pivotMode == 9)
                                   pivotMode = 0;	// switch off fast attempt
                         }
                    } else {
                         lastSequenceIn = -1;
                    }
                    if (!nFlagged2 || (normFlagged + normUnflagged < 1.0e-8)) {
                         primalColumnPivot_->setLooksOptimal(true);
                         return 0;
                    } else {
                         localPivotMode = -1;
                         nTotalPasses += nPasses;
                         nPasses = 0;
                         finished = false;
                         continue;
                    }
               }
               bestSequence = -1;
               bestBasicSequence = -1;

               // temp
               nPasses++;
               if (nPasses > 2000)
                    finished = true;
               double theta = 1.0e30;
               basicTheta = 1.0e30;
               theta_ = 1.0e30;
               double basicTolerance = 1.0e-4 * primalTolerance_;
               for (iSequence = 0; iSequence < numberTotal; iSequence++) {
                    //if (flagged(iSequence)
                    //  continue;
                    double alpha = dArray[iSequence];
                    Status thisStatus = getStatus(iSequence);
                    double oldValue = solution_[iSequence];
                    if (thisStatus != basic) {
                         if (fabs(alpha) >= acceptablePivot) {
                              if (alpha < 0.0) {
                                   // variable going towards lower bound
                                   double bound = lower_[iSequence];
                                   oldValue -= bound;
                                   if (oldValue + theta * alpha < 0.0) {
                                        bestSequence = iSequence;
                                        theta = CoinMax(0.0, oldValue / (-alpha));
                                   }
                              } else {
                                   // variable going towards upper bound
                                   double bound = upper_[iSequence];
                                   oldValue = bound - oldValue;
                                   if (oldValue - theta * alpha < 0.0) {
                                        bestSequence = iSequence;
                                        theta = CoinMax(0.0, oldValue / alpha);
                                   }
                              }
                         }
                    } else {
                         if (fabs(alpha) >= acceptableBasic) {
                              if (alpha < 0.0) {
                                   // variable going towards lower bound
                                   double bound = lower_[iSequence];
                                   oldValue -= bound;
                                   if (oldValue + basicTheta * alpha < -basicTolerance) {
                                        bestBasicSequence = iSequence;
                                        basicTheta = CoinMax(0.0, (oldValue + basicTolerance) / (-alpha));
                                   }
                              } else {
                                   // variable going towards upper bound
                                   double bound = upper_[iSequence];
                                   oldValue = bound - oldValue;
                                   if (oldValue - basicTheta * alpha < -basicTolerance) {
                                        bestBasicSequence = iSequence;
                                        basicTheta = CoinMax(0.0, (oldValue + basicTolerance) / alpha);
                                   }
                              }
                         }
                    }
               }
               theta_ = CoinMin(theta, basicTheta);
               // Now find minimum of function
               double objTheta2 = objective_->stepLength(this, solution_, dArray, CoinMin(theta, basicTheta),
                                  currentObj, predictedObj, thetaObj);
#ifdef CLP_DEBUG
               if (handler_->logLevel() & 32)
                    printf("current obj %g thetaObj %g, predictedObj %g\n", currentObj, thetaObj, predictedObj);
#endif
	       objTheta2=CoinMin(objTheta2,1.0e29);
#if MINTYPE==1
               if (conjugate) {
                    double offset;
                    const double * gradient = objective_->gradient(this,
                                              dArray, offset,
                                              true, 0);
                    double product = 0.0;
                    for (iSequence = 0; iSequence < numberColumns_; iSequence++) {
                         double alpha = dArray[iSequence];
                         double value = alpha * gradient[iSequence];
                         product += value;
                    }
                    //#define INCLUDESLACK
#ifdef INCLUDESLACK
                    for (; iSequence < numberColumns_ + numberRows_; iSequence++) {
                         double alpha = dArray[iSequence];
                         double value = alpha * cost_[iSequence];
                         product += value;
                    }
#endif
                    if (product > 0.0)
                         objTheta = djNorm / product;
                    else
                         objTheta = COIN_DBL_MAX;
#ifndef NDEBUG
                    if (product < -1.0e-8 && handler_->logLevel() > 1)
                         printf("bad product %g\n", product);
#endif
                    product = CoinMax(product, 0.0);
               } else {
                    objTheta = objTheta2;
               }
#else
               objTheta = objTheta2;
#endif
               // if very small difference then take pivot (depends on djNorm?)
               // distinguish between basic and non-basic
               bool chooseObjTheta = objTheta < theta_;
               if (chooseObjTheta) {
                    if (thetaObj < currentObj - 1.0e-12 && objTheta + 1.0e-10 > theta_)
                         chooseObjTheta = false;
                    //if (thetaObj<currentObj+1.0e-12&&objTheta+1.0e-5>theta_)
                    //chooseObjTheta=false;
               }
               //if (objTheta+SMALLTHETA1<theta_||(thetaObj>currentObj+difference&&objTheta<theta_)) {
               if (chooseObjTheta) {
                    theta_ = objTheta;
               } else {
                    objTheta = CoinMax(objTheta, 1.00000001 * theta_ + 1.0e-12);
                    //if (theta+1.0e-13>basicTheta) {
                    //theta = CoinMax(theta,1.00000001*basicTheta);
                    //theta_ = basicTheta;
                    //}
               }
               // always out if one variable in and zero move
               if (theta_ == basicTheta || (sequenceIn_ >= 0 && theta_ < 1.0e-10))
                    finished = true; // come out
#ifdef CLP_DEBUG
               if (handler_->logLevel() & 32) {
                    printf("%d pass,", nPasses);
                    if (sequenceIn_ >= 0)
                         printf (" Sin %d,", sequenceIn_);
                    if (basicTheta == theta_)
                         printf(" X(%d) basicTheta %g", bestBasicSequence, basicTheta);
                    else
                         printf(" basicTheta %g", basicTheta);
                    if (theta == theta_)
                         printf(" X(%d) non-basicTheta %g", bestSequence, theta);
                    else
                         printf(" non-basicTheta %g", theta);
                    printf(" %s objTheta %g", objTheta == theta_ ? "X" : "", objTheta);
                    printf(" djNorm %g\n", djNorm);
               }
#endif
               if (handler_->logLevel() > 3 && objTheta != theta_) {
                    printf("%d pass obj %g,", nPasses, currentObj);
                    if (sequenceIn_ >= 0)
                         printf (" Sin %d,", sequenceIn_);
                    if (basicTheta == theta_)
                         printf(" X(%d) basicTheta %g", bestBasicSequence, basicTheta);
                    else
                         printf(" basicTheta %g", basicTheta);
                    if (theta == theta_)
                         printf(" X(%d) non-basicTheta %g", bestSequence, theta);
                    else
                         printf(" non-basicTheta %g", theta);
                    printf(" %s objTheta %g", objTheta == theta_ ? "X" : "", objTheta);
                    printf(" djNorm %g\n", djNorm);
               }
               if (objTheta != theta_) {
                    //printf("hit boundary after %d passes\n",nPasses);
                    nTotalPasses += nPasses;
                    nPasses = 0; // start again
               }
               if (localPivotMode < 10 || lastSequenceIn == bestSequence) {
                    if (theta_ == theta && theta_ < basicTheta && theta_ < 1.0e-5)
                         setFlagged(bestSequence); // out of active set
               }
               // Update solution
               for (iSequence = 0; iSequence < numberTotal; iSequence++) {
                    //for (iIndex=0;iIndex<number;iIndex++) {

                    //int iSequence = which[iIndex];
                    double alpha = dArray[iSequence];
                    if (alpha) {
                         double value = solution_[iSequence] + theta_ * alpha;
                         solution_[iSequence] = value;
                         switch(getStatus(iSequence)) {

                         case basic:
                         case isFixed:
                         case isFree:
                         case atUpperBound:
                         case atLowerBound:
                              nonLinearCost_->setOne(iSequence, value);
                              break;
                         case superBasic:
                              // To get correct action
                              setStatus(iSequence, isFixed);
                              nonLinearCost_->setOne(iSequence, value);
                              //assert (getStatus(iSequence)!=isFixed);
                              break;
                         }
                    }
               }
               if (objTheta < SMALLTHETA2 && objTheta == theta_) {
                    if (sequenceIn_ >= 0 && basicTheta < 1.0e-9) {
                         // try just one
                         localPivotMode = 0;
                         sequenceIn_ = -1;
                         nTotalPasses += nPasses;
                         nPasses = 0;
                         //finished=true;
                         //objTheta=0.0;
                         //theta_=0.0;
                    } else if (sequenceIn_ < 0 && nTotalPasses > 10) {
                         if (objTheta < 1.0e-10) {
                              finished = true;
                              //printf("zero move\n");
                              break;
                         }
                    }
               }
#ifdef CLP_DEBUG
               if (handler_->logLevel() & 32) {
                    objective_->stepLength(this, solution_, work, 0.0,
                                           currentObj, predictedObj, thetaObj);
                    printf("current obj %g after update - simple was %g\n", currentObj, simpleObjective);
               }
#endif
               if (sequenceIn_ >= 0 && !finished && objTheta > 1.0e-4) {
                    // we made some progress - back to normal
                    if (localPivotMode < 10) {
                         localPivotMode = 0;
                         sequenceIn_ = -1;
                         nTotalPasses += nPasses;
                         nPasses = 0;
                    }
#ifdef CLP_DEBUG
                    if (handler_->logLevel() & 32)
                         printf("some progress?\n");
#endif
               }
#if CLP_DEBUG > 1
               longArray->checkClean();
#endif
          }
#ifdef CLP_DEBUG
          if (handler_->logLevel() & 32)
               printf("out of loop after %d (%d) passes\n", nPasses, nTotalPasses);
#endif
          if (nTotalPasses >= 1000 || (nTotalPasses > 10 && sequenceIn_ < 0 && theta_ < 1.0e-10))
               returnCode = 2;
          bool ordinaryDj = false;
          //if(sequenceIn_>=0&&numberNonBasic==1&&theta_<1.0e-7&&theta_==basicTheta)
          //printf("could try ordinary iteration %g\n",theta_);
          if(sequenceIn_ >= 0 && numberNonBasic == 1 && theta_ < 1.0e-15) {
               //printf("trying ordinary iteration\n");
               ordinaryDj = true;
          }
          if (!basicTheta && !ordinaryDj) {
               //returnCode=2;
               //objTheta=-1.0; // so we fall through
          }
          assert (theta_ < 1.0e30); // for now
          // See if we need to pivot
          if (theta_ == basicTheta || ordinaryDj) {
               if (!ordinaryDj) {
                    // find an incoming column which will force pivot
                    int iRow;
                    pivotRow_ = -1;
                    for (iRow = 0; iRow < numberRows_; iRow++) {
                         if (pivotVariable_[iRow] == bestBasicSequence) {
                              pivotRow_ = iRow;
                              break;
                         }
                    }
                    assert (pivotRow_ >= 0);
                    // Get good size for pivot
                    double acceptablePivot = 1.0e-7;
                    if (factorization_->pivots() > 10)
                         acceptablePivot = 1.0e-5; // if we have iterated be more strict
                    else if (factorization_->pivots() > 5)
                         acceptablePivot = 1.0e-6; // if we have iterated be slightly more strict
                    // should be dArray but seems better this way!
                    double direction = work[bestBasicSequence] > 0.0 ? -1.0 : 1.0;
                    // create as packed
                    rowArray->createPacked(1, &pivotRow_, &direction);
                    factorization_->updateColumnTranspose(spare, rowArray);
                    // put row of tableau in rowArray and columnArray
                    matrix_->transposeTimes(this, -1.0,
                                            rowArray, spare, columnArray);
                    // choose one futhest away from bound which has reasonable pivot
                    // If increasing we want negative alpha

                    double * work2;
                    int iSection;

                    sequenceIn_ = -1;
                    double bestValue = -1.0;
                    double bestDirection = 0.0;
                    // First pass we take correct direction and large pivots
                    // then correct direction
                    // then any
                    double check[] = {1.0e-8, -1.0e-12, -1.0e30};
                    double mult[] = {100.0, 1.0, 1.0};
                    for (int iPass = 0; iPass < 3; iPass++) {
                         //if (!bestValue&&iPass==2)
                         //bestValue=-1.0;
                         double acceptable = acceptablePivot * mult[iPass];
                         double checkValue = check[iPass];
                         for (iSection = 0; iSection < 2; iSection++) {

                              int addSequence;

                              if (!iSection) {
                                   work2 = rowArray->denseVector();
                                   number = rowArray->getNumElements();
                                   which = rowArray->getIndices();
                                   addSequence = numberColumns_;
                              } else {
                                   work2 = columnArray->denseVector();
                                   number = columnArray->getNumElements();
                                   which = columnArray->getIndices();
                                   addSequence = 0;
                              }
                              int i;

                              for (i = 0; i < number; i++) {
                                   int iSequence = which[i] + addSequence;
                                   if (flagged(iSequence))
                                        continue;
                                   //double distance = CoinMin(solution_[iSequence]-lower_[iSequence],
                                   //		  upper_[iSequence]-solution_[iSequence]);
                                   double alpha = work2[i];
                                   // should be dArray but seems better this way!
                                   double change = work[iSequence];
                                   Status thisStatus = getStatus(iSequence);
                                   double direction = 0;;
                                   switch(thisStatus) {

                                   case basic:
                                   case ClpSimplex::isFixed:
                                        break;
                                   case isFree:
                                   case superBasic:
                                        if (alpha < -acceptable && change > checkValue)
                                             direction = 1.0;
                                        else if (alpha > acceptable && change < -checkValue)
                                             direction = -1.0;
                                        break;
                                   case atUpperBound:
                                        if (alpha > acceptable && change < -checkValue)
                                             direction = -1.0;
                                        else if (iPass == 2 && alpha < -acceptable && change < -checkValue)
                                             direction = 1.0;
                                        break;
                                   case atLowerBound:
                                        if (alpha < -acceptable && change > checkValue)
                                             direction = 1.0;
                                        else if (iPass == 2 && alpha > acceptable && change > checkValue)
                                             direction = -1.0;
                                        break;
                                   }
                                   if (direction) {
                                        if (sequenceIn_ != lastSequenceIn || localPivotMode < 10) {
                                             if (CoinMin(solution_[iSequence] - lower_[iSequence],
                                                         upper_[iSequence] - solution_[iSequence]) > bestValue) {
                                                  bestValue = CoinMin(solution_[iSequence] - lower_[iSequence],
                                                                      upper_[iSequence] - solution_[iSequence]);
                                                  sequenceIn_ = iSequence;
                                                  bestDirection = direction;
                                             }
                                        } else {
                                             // choose
                                             bestValue = COIN_DBL_MAX;
                                             sequenceIn_ = iSequence;
                                             bestDirection = direction;
                                        }
                                   }
                              }
                         }
                         if (sequenceIn_ >= 0 && bestValue > 0.0)
                              break;
                    }
                    if (sequenceIn_ >= 0) {
                         valueIn_ = solution_[sequenceIn_];
                         dualIn_ = dj_[sequenceIn_];
                         if (bestDirection < 0.0) {
                              // we want positive dj
                              if (dualIn_ <= 0.0) {
                                   //printf("bad dj - xx %g\n",dualIn_);
                                   dualIn_ = 1.0;
                              }
                         } else {
                              // we want negative dj
                              if (dualIn_ >= 0.0) {
                                   //printf("bad dj - xx %g\n",dualIn_);
                                   dualIn_ = -1.0;
                              }
                         }
                         lowerIn_ = lower_[sequenceIn_];
                         upperIn_ = upper_[sequenceIn_];
                         if (dualIn_ > 0.0)
                              directionIn_ = -1;
                         else
                              directionIn_ = 1;
                    } else {
                         //ordinaryDj=true;
#ifdef CLP_DEBUG
                         if (handler_->logLevel() & 32) {
                              printf("no easy pivot - norm %g mode %d\n", djNorm, localPivotMode);
                              if (rowArray->getNumElements() + columnArray->getNumElements() < 12) {
                                   for (iSection = 0; iSection < 2; iSection++) {

                                        int addSequence;

                                        if (!iSection) {
                                             work2 = rowArray->denseVector();
                                             number = rowArray->getNumElements();
                                             which = rowArray->getIndices();
                                             addSequence = numberColumns_;
                                        } else {
                                             work2 = columnArray->denseVector();
                                             number = columnArray->getNumElements();
                                             which = columnArray->getIndices();
                                             addSequence = 0;
                                        }
                                        int i;
                                        char section[] = {'R', 'C'};
                                        for (i = 0; i < number; i++) {
                                             int iSequence = which[i] + addSequence;
                                             if (flagged(iSequence)) {
                                                  printf("%c%d flagged\n", section[iSection], which[i]);
                                                  continue;
                                             } else {
                                                  printf("%c%d status %d sol %g %g %g alpha %g change %g\n",
                                                         section[iSection], which[i], status_[iSequence],
                                                         lower_[iSequence], solution_[iSequence], upper_[iSequence],
                                                         work2[i], work[iSequence]);
                                             }
                                        }
                                   }
                              }
                         }
#endif
                         assert (sequenceIn_ < 0);
                         justOne = true;
                         allFinished = false; // Round again
                         finished = false;
                         nTotalPasses += nPasses;
                         nPasses = 0;
                         if (djNorm < 0.9 * djNorm0 && djNorm < 1.0e-3 && !localPivotMode) {
#ifdef CLP_DEBUG
                              if (handler_->logLevel() & 32)
                                   printf("no pivot - mode %d norms %g %g - unflagging\n",
                                          localPivotMode, djNorm0, djNorm);
#endif
                              unflag(); //unflagging
                              returnCode = 1;
                         } else {
                              returnCode = 2; // do single incoming
                              returnCode = 1;
                         }
                    }
               }
               rowArray->clear();
               columnArray->clear();
               longArray->clear();
               if (ordinaryDj) {
                    valueIn_ = solution_[sequenceIn_];
                    dualIn_ = dj_[sequenceIn_];
                    lowerIn_ = lower_[sequenceIn_];
                    upperIn_ = upper_[sequenceIn_];
                    if (dualIn_ > 0.0)
                         directionIn_ = -1;
                    else
                         directionIn_ = 1;
               }
               if (returnCode == 1)
                    returnCode = 0;
          } else {
               // round again
               longArray->clear();
               if (djNorm < 1.0e-3 && !localPivotMode) {
                    if (djNorm == 1.2345e-20 && djNorm0 > 1.0e-4) {
#ifdef CLP_DEBUG
                         if (handler_->logLevel() & 32)
                              printf("slow convergence djNorm0 %g, %d passes, mode %d, result %d\n", djNorm0, nPasses,
                                     localPivotMode, returnCode);
#endif
                         //if (!localPivotMode)
                         //returnCode=2; // force singleton
                    } else {
#ifdef CLP_DEBUG
                         if (handler_->logLevel() & 32)
                              printf("unflagging as djNorm %g %g, %d passes\n", djNorm, djNorm0, nPasses);
#endif
                         if (pivotMode >= 10) {
                              pivotMode--;
#ifdef CLP_DEBUG
                              if (handler_->logLevel() & 32)
                                   printf("pivot mode now %d\n", pivotMode);
#endif
                              if (pivotMode == 9)
                                   pivotMode = 0;	// switch off fast attempt
                         }
                         bool unflagged = unflag() != 0;
                         if (!unflagged && djNorm < 1.0e-10) {
                              // ? declare victory
                              sequenceIn_ = -1;
                              returnCode = 0;
                         }
                    }
               }
          }
     }
     if (djNorm0 < 1.0e-12 * normFlagged) {
#ifdef CLP_DEBUG
          if (handler_->logLevel() & 32)
               printf("unflagging as djNorm %g %g and flagged norm %g\n", djNorm, djNorm0, normFlagged);
#endif
          unflag();
     }
     if (saveObj - currentObj < 1.0e-5 && nTotalPasses > 2000) {
          normUnflagged = 0.0;
          double dualTolerance3 = CoinMin(1.0e-2, 1.0e3 * dualTolerance_);
          for (int iSequence = 0; iSequence < numberColumns_ + numberRows_; iSequence++) {
               switch(getStatus(iSequence)) {

               case basic:
               case ClpSimplex::isFixed:
                    break;
               case atUpperBound:
                    if (dj_[iSequence] > dualTolerance3)
                         normFlagged += dj_[iSequence] * dj_[iSequence];
                    break;
               case atLowerBound:
                    if (dj_[iSequence] < -dualTolerance3)
                         normFlagged += dj_[iSequence] * dj_[iSequence];
                    break;
               case isFree:
               case superBasic:
                    if (fabs(dj_[iSequence]) > dualTolerance3)
                         normFlagged += dj_[iSequence] * dj_[iSequence];
                    break;
               }
          }
          if (handler_->logLevel() > 2)
               printf("possible optimal  %d %d %g %g\n",
                      nBigPasses, nTotalPasses, saveObj - currentObj, normFlagged);
          if (normFlagged < 1.0e-5) {
               unflag();
               primalColumnPivot_->setLooksOptimal(true);
               returnCode = 0;
          }
     }
     return returnCode;
}
/* Do last half of an iteration.
   Return codes
   Reasons to come out normal mode
   -1 normal
   -2 factorize now - good iteration
   -3 slight inaccuracy - refactorize - iteration done
   -4 inaccuracy - refactorize - no iteration
   -5 something flagged - go round again
   +2 looks unbounded
   +3 max iterations (iteration done)

*/
int
ClpSimplexNonlinear::pivotNonlinearResult()
{

     int returnCode = -1;

     rowArray_[1]->clear();

     // we found a pivot column
     // update the incoming column
     unpackPacked(rowArray_[1]);
     factorization_->updateColumnFT(rowArray_[2], rowArray_[1]);
     theta_ = 0.0;
     double * work = rowArray_[1]->denseVector();
     int number = rowArray_[1]->getNumElements();
     int * which = rowArray_[1]->getIndices();
     bool keepValue = false;
     double saveValue = 0.0;
     if (pivotRow_ >= 0) {
          sequenceOut_ = pivotVariable_[pivotRow_];;
          valueOut_ = solution(sequenceOut_);
          keepValue = true;
          saveValue = valueOut_;
          lowerOut_ = lower_[sequenceOut_];
          upperOut_ = upper_[sequenceOut_];
          int iIndex;
          for (iIndex = 0; iIndex < number; iIndex++) {

               int iRow = which[iIndex];
               if (iRow == pivotRow_) {
                    alpha_ = work[iIndex];
                    break;
               }
          }
     } else {
          int iIndex;
          double smallest = COIN_DBL_MAX;
          for (iIndex = 0; iIndex < number; iIndex++) {
               int iRow = which[iIndex];
               double alpha = work[iIndex];
               if (fabs(alpha) > 1.0e-6) {
                    int iPivot = pivotVariable_[iRow];
                    double distance = CoinMin(upper_[iPivot] - solution_[iPivot],
                                              solution_[iPivot] - lower_[iPivot]);
                    if (distance < smallest) {
                         pivotRow_ = iRow;
                         alpha_ = alpha;
                         smallest = distance;
                    }
               }
          }
          if (smallest > primalTolerance_) {
               smallest = COIN_DBL_MAX;
               for (iIndex = 0; iIndex < number; iIndex++) {
                    int iRow = which[iIndex];
                    double alpha = work[iIndex];
                    if (fabs(alpha) > 1.0e-6) {
                         double distance = randomNumberGenerator_.randomDouble();
                         if (distance < smallest) {
                              pivotRow_ = iRow;
                              alpha_ = alpha;
                              smallest = distance;
                         }
                    }
               }
          }
          assert (pivotRow_ >= 0);
          sequenceOut_ = pivotVariable_[pivotRow_];;
          valueOut_ = solution(sequenceOut_);
          lowerOut_ = lower_[sequenceOut_];
          upperOut_ = upper_[sequenceOut_];
     }
     double newValue = valueOut_ - theta_ * alpha_;
     bool isSuperBasic = false;
     if (valueOut_ >= upperOut_ - primalTolerance_) {
          directionOut_ = -1;    // to upper bound
          upperOut_ = nonLinearCost_->nearest(sequenceOut_, newValue);
          upperOut_ = newValue;
     } else if (valueOut_ <= lowerOut_ + primalTolerance_) {
          directionOut_ = 1;    // to lower bound
          lowerOut_ = nonLinearCost_->nearest(sequenceOut_, newValue);
     } else {
          lowerOut_ = valueOut_;
          upperOut_ = valueOut_;
          isSuperBasic = true;
          //printf("XX superbasic out\n");
     }
     dualOut_ = dj_[sequenceOut_];
     // if stable replace in basis

     int updateStatus = factorization_->replaceColumn(this,
                        rowArray_[2],
                        rowArray_[1],
                        pivotRow_,
                        alpha_);

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
               returnCode = -4;
          } else {
               // need to reject something
               char x = isColumn(sequenceIn_) ? 'C' : 'R';
               handler_->message(CLP_SIMPLEX_FLAG, messages_)
                         << x << sequenceWithin(sequenceIn_)
                         << CoinMessageEol;
               setFlagged(sequenceIn_);
               progress_.clearBadTimes();
               lastBadIteration_ = numberIterations_; // say be more cautious
               clearAll();
               pivotRow_ = -1;
               sequenceOut_ = -1;
               returnCode = -5;

          }
          return returnCode;
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

     // update primal solution

     double objectiveChange = 0.0;
     // after this rowArray_[1] is not empty - used to update djs
     // If pivot row >= numberRows then may be gub
     updatePrimalsInPrimal(rowArray_[1], theta_, objectiveChange, 1);

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
          if (!isSuperBasic)
               directionOut_ = nonLinearCost_->setOneOutgoing(sequenceOut_, valueOut_);
          solution_[sequenceOut_] = valueOut_;
     }
     // change cost and bounds on incoming if primal
     nonLinearCost_->setOne(sequenceIn_, valueIn_);
     int whatNext = housekeeping(objectiveChange);
     if (keepValue)
          solution_[sequenceOut_] = saveValue;
     if (isSuperBasic)
          setStatus(sequenceOut_, superBasic);
     //#define CLP_DEBUG
#if CLP_DEBUG > 1
     {
          int ninf = matrix_->checkFeasible(this);
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
               returnCode = 4;
          }
     }
     return returnCode;
}
// A sequential LP method
int
ClpSimplexNonlinear::primalSLP(int numberPasses, double deltaTolerance)
{
     // Are we minimizing or maximizing
     double whichWay = optimizationDirection();
     if (whichWay < 0.0)
          whichWay = -1.0;
     else if (whichWay > 0.0)
          whichWay = 1.0;


     int numberColumns = this->numberColumns();
     int numberRows = this->numberRows();
     double * columnLower = this->columnLower();
     double * columnUpper = this->columnUpper();
     double * solution = this->primalColumnSolution();

     if (objective_->type() < 2) {
          // no nonlinear part
          return ClpSimplex::primal(0);
     }
     // Get list of non linear columns
     char * markNonlinear = new char[numberColumns];
     memset(markNonlinear, 0, numberColumns);
     int numberNonLinearColumns = objective_->markNonlinear(markNonlinear);

     if (!numberNonLinearColumns) {
          delete [] markNonlinear;
          // no nonlinear part
          return ClpSimplex::primal(0);
     }
     int iColumn;
     int * listNonLinearColumn = new int[numberNonLinearColumns];
     numberNonLinearColumns = 0;
     for (iColumn = 0; iColumn < numberColumns; iColumn++) {
          if(markNonlinear[iColumn])
               listNonLinearColumn[numberNonLinearColumns++] = iColumn;
     }
     // Replace objective
     ClpObjective * trueObjective = objective_;
     objective_ = new ClpLinearObjective(NULL, numberColumns);
     double * objective = this->objective();

     // get feasible
     if (this->status() < 0 || numberPrimalInfeasibilities())
          ClpSimplex::primal(1);
     // still infeasible
     if (numberPrimalInfeasibilities()) {
          delete [] listNonLinearColumn;
          return 0;
     }
     algorithm_ = 1;
     int jNon;
     int * last[3];

     double * trust = new double[numberNonLinearColumns];
     double * trueLower = new double[numberNonLinearColumns];
     double * trueUpper = new double[numberNonLinearColumns];
     for (jNon = 0; jNon < numberNonLinearColumns; jNon++) {
          iColumn = listNonLinearColumn[jNon];
          trust[jNon] = 0.5;
          trueLower[jNon] = columnLower[iColumn];
          trueUpper[jNon] = columnUpper[iColumn];
          if (solution[iColumn] < trueLower[jNon])
               solution[iColumn] = trueLower[jNon];
          else if (solution[iColumn] > trueUpper[jNon])
               solution[iColumn] = trueUpper[jNon];
     }
     int saveLogLevel = logLevel();
     int iPass;
     double lastObjective = 1.0e31;
     double * saveSolution = new double [numberColumns];
     double * saveRowSolution = new double [numberRows];
     memset(saveRowSolution, 0, numberRows * sizeof(double));
     double * savePi = new double [numberRows];
     double * safeSolution = new double [numberColumns];
     unsigned char * saveStatus = new unsigned char[numberRows+numberColumns];
#define MULTIPLE 0
#if MULTIPLE>2
     // Duplication but doesn't really matter
     double * saveSolutionM[MULTIPLE
                       };
for (jNon=0; jNon<MULTIPLE; jNon++)
{
     saveSolutionM[jNon] = new double[numberColumns];
     CoinMemcpyN(solution, numberColumns, saveSolutionM);
}
#endif
double targetDrop = 1.0e31;
double objectiveOffset;
getDblParam(ClpObjOffset, objectiveOffset);
// 1 bound up, 2 up, -1 bound down, -2 down, 0 no change
for (iPass = 0; iPass < 3; iPass++)
{
     last[iPass] = new int[numberNonLinearColumns];
     for (jNon = 0; jNon < numberNonLinearColumns; jNon++)
          last[iPass][jNon] = 0;
}
// goodMove +1 yes, 0 no, -1 last was bad - just halve gaps, -2 do nothing
int goodMove = -2;
char * statusCheck = new char[numberColumns];
double * changeRegion = new double [numberColumns];
double offset = 0.0;
double objValue = 0.0;
int exitPass = 2 * numberPasses + 10;
for (iPass = 0; iPass < numberPasses; iPass++)
{
     exitPass--;
     // redo objective
     offset = 0.0;
     objValue = -objectiveOffset;
     // make sure x updated
     trueObjective->newXValues();
     double theta = -1.0;
     double maxTheta = COIN_DBL_MAX;
     //maxTheta=1.0;
     if (iPass) {
          int jNon = 0;
          for (iColumn = 0; iColumn < numberColumns; iColumn++) {
               changeRegion[iColumn] = solution[iColumn] - saveSolution[iColumn];
               double alpha = changeRegion[iColumn];
               double oldValue = saveSolution[iColumn];
               if (markNonlinear[iColumn] == 0) {
                    // linear
                    if (alpha < -1.0e-15) {
                         // variable going towards lower bound
                         double bound = columnLower[iColumn];
                         oldValue -= bound;
                         if (oldValue + maxTheta * alpha < 0.0) {
                              maxTheta = CoinMax(0.0, oldValue / (-alpha));
                         }
                    } else if (alpha > 1.0e-15) {
                         // variable going towards upper bound
                         double bound = columnUpper[iColumn];
                         oldValue = bound - oldValue;
                         if (oldValue - maxTheta * alpha < 0.0) {
                              maxTheta = CoinMax(0.0, oldValue / alpha);
                         }
                    }
               } else {
                    // nonlinear
                    if (alpha < -1.0e-15) {
                         // variable going towards lower bound
                         double bound = trueLower[jNon];
                         oldValue -= bound;
                         if (oldValue + maxTheta * alpha < 0.0) {
                              maxTheta = CoinMax(0.0, oldValue / (-alpha));
                         }
                    } else if (alpha > 1.0e-15) {
                         // variable going towards upper bound
                         double bound = trueUpper[jNon];
                         oldValue = bound - oldValue;
                         if (oldValue - maxTheta * alpha < 0.0) {
                              maxTheta = CoinMax(0.0, oldValue / alpha);
                         }
                    }
                    jNon++;
               }
          }
          // make sure both accurate
          memset(rowActivity_, 0, numberRows_ * sizeof(double));
          times(1.0, solution, rowActivity_);
          memset(saveRowSolution, 0, numberRows_ * sizeof(double));
          times(1.0, saveSolution, saveRowSolution);
          for (int iRow = 0; iRow < numberRows; iRow++) {
               double alpha = rowActivity_[iRow] - saveRowSolution[iRow];
               double oldValue = saveRowSolution[iRow];
               if (alpha < -1.0e-15) {
                    // variable going towards lower bound
                    double bound = rowLower_[iRow];
                    oldValue -= bound;
                    if (oldValue + maxTheta * alpha < 0.0) {
                         maxTheta = CoinMax(0.0, oldValue / (-alpha));
                    }
               } else if (alpha > 1.0e-15) {
                    // variable going towards upper bound
                    double bound = rowUpper_[iRow];
                    oldValue = bound - oldValue;
                    if (oldValue - maxTheta * alpha < 0.0) {
                         maxTheta = CoinMax(0.0, oldValue / alpha);
                    }
               }
          }
     } else {
          for (iColumn = 0; iColumn < numberColumns; iColumn++) {
               changeRegion[iColumn] = 0.0;
               saveSolution[iColumn] = solution[iColumn];
          }
          CoinMemcpyN(rowActivity_, numberRows, saveRowSolution);
     }
     // get current value anyway
     double predictedObj, thetaObj;
     double maxTheta2 = 2.0; // to work out a b c
     double theta2 = trueObjective->stepLength(this, saveSolution, changeRegion, maxTheta2,
                     objValue, predictedObj, thetaObj);
     int lastMoveStatus = goodMove;
     if (goodMove >= 0) {
          theta = CoinMin(theta2, maxTheta);
#ifdef CLP_DEBUG
          if (handler_->logLevel() & 32)
               printf("theta %g, current %g, at maxtheta %g, predicted %g\n",
                      theta, objValue, thetaObj, predictedObj);
#endif
          if (theta > 0.0 && theta <= 1.0) {
               // update solution
               double lambda = 1.0 - theta;
               for (iColumn = 0; iColumn < numberColumns; iColumn++)
                    solution[iColumn] = lambda * saveSolution[iColumn]
                                        + theta * solution[iColumn];
               memset(rowActivity_, 0, numberRows_ * sizeof(double));
               times(1.0, solution, rowActivity_);
               if (lambda > 0.999) {
                    CoinMemcpyN(savePi, numberRows, this->dualRowSolution());
                    CoinMemcpyN(saveStatus, numberRows + numberColumns, status_);
               }
               // Do local minimization
#define LOCAL
#ifdef LOCAL
               bool absolutelyOptimal = false;
               int saveScaling = scalingFlag();
               scaling(0);
               int savePerturbation = perturbation_;
               perturbation_ = 100;
               if (saveLogLevel == 1)
                    setLogLevel(0);
               int status = startup(1);
               if (!status) {
                    int numberTotal = numberRows_ + numberColumns_;
                    // resize arrays
                    for (int i = 0; i < 4; i++) {
                         rowArray_[i]->reserve(CoinMax(numberRows_ + numberColumns_, rowArray_[i]->capacity()));
                    }
                    CoinIndexedVector * longArray = rowArray_[3];
                    CoinIndexedVector * rowArray = rowArray_[0];
                    //CoinIndexedVector * columnArray = columnArray_[0];
                    CoinIndexedVector * spare = rowArray_[1];
                    double * work = longArray->denseVector();
                    int * which = longArray->getIndices();
                    int nPass = 100;
                    //bool conjugate=false;
                    // Put back true bounds
                    for (jNon = 0; jNon < numberNonLinearColumns; jNon++) {
                         int iColumn = listNonLinearColumn[jNon];
                         double value;
                         value = trueLower[jNon];
                         trueLower[jNon] = lower_[iColumn];
                         lower_[iColumn] = value;
                         value = trueUpper[jNon];
                         trueUpper[jNon] = upper_[iColumn];
                         upper_[iColumn] = value;
                         switch(getStatus(iColumn)) {

                         case basic:
                         case isFree:
                         case superBasic:
                              break;
                         case isFixed:
                         case atUpperBound:
                         case atLowerBound:
                              if (solution_[iColumn] > lower_[iColumn] + primalTolerance_) {
                                   if(solution_[iColumn] < upper_[iColumn] - primalTolerance_) {
                                        setStatus(iColumn, superBasic);
                                   } else {
                                        setStatus(iColumn, atUpperBound);
                                   }
                              } else {
                                   if(solution_[iColumn] < upper_[iColumn] - primalTolerance_) {
                                        setStatus(iColumn, atLowerBound);
                                   } else {
                                        setStatus(iColumn, isFixed);
                                   }
                              }
                              break;
                         }
                    }
                    for (int jPass = 0; jPass < nPass; jPass++) {
                         trueObjective->reducedGradient(this, dj_, true);
                         // get direction vector in longArray
                         longArray->clear();
                         double normFlagged = 0.0;
                         double normUnflagged = 0.0;
                         int numberNonBasic = 0;
                         directionVector(longArray, spare, rowArray, 0,
                                         normFlagged, normUnflagged, numberNonBasic);
                         if (normFlagged + normUnflagged < 1.0e-8) {
                              absolutelyOptimal = true;
                              break;  //looks optimal
                         }
                         double djNorm = 0.0;
                         int iIndex;
                         for (iIndex = 0; iIndex < numberNonBasic; iIndex++) {
                              int iSequence = which[iIndex];
                              double alpha = work[iSequence];
                              djNorm += alpha * alpha;
                         }
                         //if (!jPass)
                         //printf("dj norm %g - %d \n",djNorm,numberNonBasic);
                         //int number=longArray->getNumElements();
                         if (!numberNonBasic) {
                              // looks optimal
                              absolutelyOptimal = true;
                              break;
                         }
                         double theta = 1.0e30;
                         int iSequence;
                         for (iSequence = 0; iSequence < numberTotal; iSequence++) {
                              double alpha = work[iSequence];
                              double oldValue = solution_[iSequence];
                              if (alpha < -1.0e-15) {
                                   // variable going towards lower bound
                                   double bound = lower_[iSequence];
                                   oldValue -= bound;
                                   if (oldValue + theta * alpha < 0.0) {
                                        theta = CoinMax(0.0, oldValue / (-alpha));
                                   }
                              } else if (alpha > 1.0e-15) {
                                   // variable going towards upper bound
                                   double bound = upper_[iSequence];
                                   oldValue = bound - oldValue;
                                   if (oldValue - theta * alpha < 0.0) {
                                        theta = CoinMax(0.0, oldValue / alpha);
                                   }
                              }
                         }
                         // Now find minimum of function
                         double currentObj;
                         double predictedObj;
                         double thetaObj;
                         // need to use true objective
                         double * saveCost = cost_;
                         cost_ = NULL;
                         double objTheta = trueObjective->stepLength(this, solution_, work, theta,
                                           currentObj, predictedObj, thetaObj);
                         cost_ = saveCost;
#ifdef CLP_DEBUG
                         if (handler_->logLevel() & 32)
                              printf("current obj %g thetaObj %g, predictedObj %g\n", currentObj, thetaObj, predictedObj);
#endif
                         //printf("current obj %g thetaObj %g, predictedObj %g\n",currentObj,thetaObj,predictedObj);
                         //printf("objTheta %g theta %g\n",objTheta,theta);
                         if (theta > objTheta) {
                              theta = objTheta;
                              thetaObj = predictedObj;
                         }
                         // update one used outside
                         objValue = currentObj;
                         if (theta > 1.0e-9 &&
                                   (currentObj - thetaObj < -CoinMax(1.0e-8, 1.0e-15 * fabs(currentObj)) || jPass < 5)) {
                              // Update solution
                              for (iSequence = 0; iSequence < numberTotal; iSequence++) {
                                   double alpha = work[iSequence];
                                   if (alpha) {
                                        double value = solution_[iSequence] + theta * alpha;
                                        solution_[iSequence] = value;
                                        switch(getStatus(iSequence)) {

                                        case basic:
                                        case isFixed:
                                        case isFree:
                                             break;
                                        case atUpperBound:
                                        case atLowerBound:
                                        case superBasic:
                                             if (value > lower_[iSequence] + primalTolerance_) {
                                                  if(value < upper_[iSequence] - primalTolerance_) {
                                                       setStatus(iSequence, superBasic);
                                                  } else {
                                                       setStatus(iSequence, atUpperBound);
                                                  }
                                             } else {
                                                  if(value < upper_[iSequence] - primalTolerance_) {
                                                       setStatus(iSequence, atLowerBound);
                                                  } else {
                                                       setStatus(iSequence, isFixed);
                                                  }
                                             }
                                             break;
                                        }
                                   }
                              }
                         } else {
                              break;
                         }
                    }
                    // Put back fake bounds
                    for (jNon = 0; jNon < numberNonLinearColumns; jNon++) {
                         int iColumn = listNonLinearColumn[jNon];
                         double value;
                         value = trueLower[jNon];
                         trueLower[jNon] = lower_[iColumn];
                         lower_[iColumn] = value;
                         value = trueUpper[jNon];
                         trueUpper[jNon] = upper_[iColumn];
                         upper_[iColumn] = value;
                    }
               }
               problemStatus_ = 0;
               finish();
               scaling(saveScaling);
               perturbation_ = savePerturbation;
               setLogLevel(saveLogLevel);
#endif
               // redo rowActivity
               memset(rowActivity_, 0, numberRows_ * sizeof(double));
               times(1.0, solution, rowActivity_);
               if (theta > 0.99999 && theta2 < 1.9 && !absolutelyOptimal) {
                    // If big changes then tighten
                    /*  thetaObj is objvalue + a*2*2 +b*2
                        predictedObj is objvalue + a*theta2*theta2 +b*theta2
                    */
                    double rhs1 = thetaObj - objValue;
                    double rhs2 = predictedObj - objValue;
                    double subtractB = theta2 * 0.5;
                    double a = (rhs2 - subtractB * rhs1) / (theta2 * theta2 - 4.0 * subtractB);
                    double b = 0.5 * (rhs1 - 4.0 * a);
                    if (fabs(a + b) > 1.0e-2) {
                         // tighten all
                         goodMove = -1;
                    }
               }
          }
     }
     CoinMemcpyN(trueObjective->gradient(this, solution, offset, true, 2),	numberColumns,
                 objective);
     //printf("offset comp %g orig %g\n",offset,objectiveOffset);
     // could do faster
     trueObjective->stepLength(this, solution, changeRegion, 0.0,
                               objValue, predictedObj, thetaObj);
#ifdef CLP_INVESTIGATE
     printf("offset comp %g orig %g - obj from stepLength %g\n", offset, objectiveOffset, objValue);
#endif
     setDblParam(ClpObjOffset, objectiveOffset + offset);
     int * temp = last[2];
     last[2] = last[1];
     last[1] = last[0];
     last[0] = temp;
     for (jNon = 0; jNon < numberNonLinearColumns; jNon++) {
          iColumn = listNonLinearColumn[jNon];
          double change = solution[iColumn] - saveSolution[iColumn];
          if (change < -1.0e-5) {
               if (fabs(change + trust[jNon]) < 1.0e-5)
                    temp[jNon] = -1;
               else
                    temp[jNon] = -2;
          } else if(change > 1.0e-5) {
               if (fabs(change - trust[jNon]) < 1.0e-5)
                    temp[jNon] = 1;
               else
                    temp[jNon] = 2;
          } else {
               temp[jNon] = 0;
          }
     }
     // goodMove +1 yes, 0 no, -1 last was bad - just halve gaps, -2 do nothing
     double maxDelta = 0.0;
     if (goodMove >= 0) {
          if (objValue - lastObjective <= 1.0e-15 * fabs(lastObjective))
               goodMove = 1;
          else
               goodMove = 0;
     } else {
          maxDelta = 1.0e10;
     }
     double maxGap = 0.0;
     int numberSmaller = 0;
     int numberSmaller2 = 0;
     int numberLarger = 0;
     for (jNon = 0; jNon < numberNonLinearColumns; jNon++) {
          iColumn = listNonLinearColumn[jNon];
          maxDelta = CoinMax(maxDelta,
                             fabs(solution[iColumn] - saveSolution[iColumn]));
          if (goodMove > 0) {
               if (last[0][jNon]*last[1][jNon] < 0) {
                    // halve
                    trust[jNon] *= 0.5;
                    numberSmaller2++;
               } else {
                    if (last[0][jNon] == last[1][jNon] &&
                              last[0][jNon] == last[2][jNon])
                         trust[jNon] = CoinMin(1.5 * trust[jNon], 1.0e6);
                    numberLarger++;
               }
          } else if (goodMove != -2 && trust[jNon] > 10.0 * deltaTolerance) {
               trust[jNon] *= 0.2;
               numberSmaller++;
          }
          maxGap = CoinMax(maxGap, trust[jNon]);
     }
#ifdef CLP_DEBUG
     if (handler_->logLevel() & 32)
          std::cout << "largest gap is " << maxGap << " "
                    << numberSmaller + numberSmaller2 << " reduced ("
                    << numberSmaller << " badMove ), "
                    << numberLarger << " increased" << std::endl;
#endif
     if (iPass > 10000) {
          for (jNon = 0; jNon < numberNonLinearColumns; jNon++)
               trust[jNon] *= 0.0001;
     }
     if(lastMoveStatus == -1 && goodMove == -1)
          goodMove = 1; // to force solve
     if (goodMove > 0) {
          double drop = lastObjective - objValue;
          handler_->message(CLP_SLP_ITER, messages_)
                    << iPass << objValue - objectiveOffset
                    << drop << maxDelta
                    << CoinMessageEol;
          if (iPass > 20 && drop < 1.0e-12 * fabs(objValue))
               drop = 0.999e-4; // so will exit
          if (maxDelta < deltaTolerance && drop < 1.0e-4 && goodMove && theta < 0.99999) {
               if (handler_->logLevel() > 1)
                    std::cout << "Exiting as maxDelta < tolerance and small drop" << std::endl;
               break;
          }
     } else if (!numberSmaller && iPass > 1) {
          if (handler_->logLevel() > 1)
               std::cout << "Exiting as all gaps small" << std::endl;
          break;
     }
     if (!iPass)
          goodMove = 1;
     targetDrop = 0.0;
     double * r = this->dualColumnSolution();
     for (jNon = 0; jNon < numberNonLinearColumns; jNon++) {
          iColumn = listNonLinearColumn[jNon];
          columnLower[iColumn] = CoinMax(solution[iColumn]
                                         - trust[jNon],
                                         trueLower[jNon]);
          columnUpper[iColumn] = CoinMin(solution[iColumn]
                                         + trust[jNon],
                                         trueUpper[jNon]);
     }
     if (iPass) {
          // get reduced costs
          this->matrix()->transposeTimes(savePi,
                                         this->dualColumnSolution());
          for (jNon = 0; jNon < numberNonLinearColumns; jNon++) {
               iColumn = listNonLinearColumn[jNon];
               double dj = objective[iColumn] - r[iColumn];
               r[iColumn] = dj;
               if (dj < -dualTolerance_)
                    targetDrop -= dj * (columnUpper[iColumn] - solution[iColumn]);
               else if (dj > dualTolerance_)
                    targetDrop -= dj * (columnLower[iColumn] - solution[iColumn]);
          }
     } else {
          memset(r, 0, numberColumns * sizeof(double));
     }
#if 0
     for (jNon = 0; jNon < numberNonLinearColumns; jNon++) {
          iColumn = listNonLinearColumn[jNon];
          if (statusCheck[iColumn] == 'L' && r[iColumn] < -1.0e-4) {
               columnLower[iColumn] = CoinMax(solution[iColumn],
                                              trueLower[jNon]);
               columnUpper[iColumn] = CoinMin(solution[iColumn]
                                              + trust[jNon],
                                              trueUpper[jNon]);
          } else if (statusCheck[iColumn] == 'U' && r[iColumn] > 1.0e-4) {
               columnLower[iColumn] = CoinMax(solution[iColumn]
                                              - trust[jNon],
                                              trueLower[jNon]);
               columnUpper[iColumn] = CoinMin(solution[iColumn],
                                              trueUpper[jNon]);
          } else {
               columnLower[iColumn] = CoinMax(solution[iColumn]
                                              - trust[jNon],
                                              trueLower[jNon]);
               columnUpper[iColumn] = CoinMin(solution[iColumn]
                                              + trust[jNon],
                                              trueUpper[jNon]);
          }
     }
#endif
     if (goodMove > 0) {
          CoinMemcpyN(solution, numberColumns, saveSolution);
          CoinMemcpyN(rowActivity_, numberRows, saveRowSolution);
          CoinMemcpyN(this->dualRowSolution(), numberRows, savePi);
          CoinMemcpyN(status_, numberRows + numberColumns, saveStatus);
#if MULTIPLE>2
          double * tempSol = saveSolutionM[0];
          for (jNon = 0; jNon < MULTIPLE - 1; jNon++) {
               saveSolutionM[jNon] = saveSolutionM[jNon+1];
          }
          saveSolutionM[MULTIPLE-1] = tempSol;
          CoinMemcpyN(solution, numberColumns, tempSol);

#endif

#ifdef CLP_DEBUG
          if (handler_->logLevel() & 32)
               std::cout << "Pass - " << iPass
                         << ", target drop is " << targetDrop
                         << std::endl;
#endif
          lastObjective = objValue;
          if (targetDrop < CoinMax(1.0e-8, CoinMin(1.0e-6, 1.0e-6 * fabs(objValue))) && goodMove && iPass > 3) {
               if (handler_->logLevel() > 1)
                    printf("Exiting on target drop %g\n", targetDrop);
               break;
          }
#ifdef CLP_DEBUG
          {
               double * r = this->dualColumnSolution();
               for (jNon = 0; jNon < numberNonLinearColumns; jNon++) {
                    iColumn = listNonLinearColumn[jNon];
                    if (handler_->logLevel() & 32)
                         printf("Trust %d %g - solution %d %g obj %g dj %g state %c - bounds %g %g\n",
                                jNon, trust[jNon], iColumn, solution[iColumn], objective[iColumn],
                                r[iColumn], statusCheck[iColumn], columnLower[iColumn],
                                columnUpper[iColumn]);
               }
          }
#endif
          //setLogLevel(63);
          this->scaling(false);
          if (saveLogLevel == 1)
               setLogLevel(0);
          ClpSimplex::primal(1);
          algorithm_ = 1;
          setLogLevel(saveLogLevel);
#ifdef CLP_DEBUG
          if (this->status()) {
               writeMps("xx.mps");
          }
#endif
          if (this->status() == 1) {
               // not feasible ! - backtrack and exit
               // use safe solution
               CoinMemcpyN(safeSolution, numberColumns, solution);
               CoinMemcpyN(solution, numberColumns, saveSolution);
               memset(rowActivity_, 0, numberRows_ * sizeof(double));
               times(1.0, solution, rowActivity_);
               CoinMemcpyN(rowActivity_, numberRows, saveRowSolution);
               CoinMemcpyN(savePi, numberRows, this->dualRowSolution());
               CoinMemcpyN(saveStatus, numberRows + numberColumns, status_);
               for (jNon = 0; jNon < numberNonLinearColumns; jNon++) {
                    iColumn = listNonLinearColumn[jNon];
                    columnLower[iColumn] = CoinMax(solution[iColumn]
                                                   - trust[jNon],
                                                   trueLower[jNon]);
                    columnUpper[iColumn] = CoinMin(solution[iColumn]
                                                   + trust[jNon],
                                                   trueUpper[jNon]);
               }
               break;
          } else {
               // save in case problems
               CoinMemcpyN(solution, numberColumns, safeSolution);
          }
          if (problemStatus_ == 3)
               break; // probably user interrupt
          goodMove = 1;
     } else {
          // bad pass - restore solution
#ifdef CLP_DEBUG
          if (handler_->logLevel() & 32)
               printf("Backtracking\n");
#endif
          CoinMemcpyN(saveSolution, numberColumns, solution);
          CoinMemcpyN(saveRowSolution, numberRows, rowActivity_);
          CoinMemcpyN(savePi, numberRows, this->dualRowSolution());
          CoinMemcpyN(saveStatus, numberRows + numberColumns, status_);
          iPass--;
          assert (exitPass > 0);
          goodMove = -1;
     }
}
#if MULTIPLE>2
for (jNon = 0; jNon < MULTIPLE; jNon++)
{
     delete [] saveSolutionM[jNon];
}
#endif
// restore solution
CoinMemcpyN(saveSolution, numberColumns, solution);
CoinMemcpyN(saveRowSolution, numberRows, rowActivity_);
for (jNon = 0; jNon < numberNonLinearColumns; jNon++)
{
     iColumn = listNonLinearColumn[jNon];
     columnLower[iColumn] = CoinMax(solution[iColumn],
                                    trueLower[jNon]);
     columnUpper[iColumn] = CoinMin(solution[iColumn],
                                    trueUpper[jNon]);
}
delete [] markNonlinear;
delete [] statusCheck;
delete [] savePi;
delete [] saveStatus;
// redo objective
CoinMemcpyN(trueObjective->gradient(this, solution, offset, true, 2),	numberColumns,
            objective);
ClpSimplex::primal(1);
delete objective_;
objective_ = trueObjective;
// redo values
setDblParam(ClpObjOffset, objectiveOffset);
objectiveValue_ += whichWay * offset;
for (jNon = 0; jNon < numberNonLinearColumns; jNon++)
{
     iColumn = listNonLinearColumn[jNon];
     columnLower[iColumn] = trueLower[jNon];
     columnUpper[iColumn] = trueUpper[jNon];
}
delete [] saveSolution;
delete [] safeSolution;
delete [] saveRowSolution;
for (iPass = 0; iPass < 3; iPass++)
     delete [] last[iPass];
delete [] trust;
delete [] trueUpper;
delete [] trueLower;
delete [] listNonLinearColumn;
delete [] changeRegion;
// temp
//setLogLevel(63);
return 0;
}
/* Primal algorithm for nonlinear constraints
   Using a semi-trust region approach as for pooling problem
   This is in because I have it lying around

*/
int
ClpSimplexNonlinear::primalSLP(int numberConstraints, ClpConstraint ** constraints,
                               int numberPasses, double deltaTolerance)
{
     if (!numberConstraints) {
          // no nonlinear constraints - may be nonlinear objective
          return primalSLP(numberPasses, deltaTolerance);
     }
     // Are we minimizing or maximizing
     double whichWay = optimizationDirection();
     if (whichWay < 0.0)
          whichWay = -1.0;
     else if (whichWay > 0.0)
          whichWay = 1.0;
     // check all matrix for odd rows is empty
     int iConstraint;
     int numberBad = 0;
     CoinPackedMatrix * columnCopy = matrix();
     // Get a row copy in standard format
     CoinPackedMatrix copy;
     copy.reverseOrderedCopyOf(*columnCopy);
     // get matrix data pointers
     //const int * column = copy.getIndices();
     //const CoinBigIndex * rowStart = copy.getVectorStarts();
     const int * rowLength = copy.getVectorLengths();
     //const double * elementByRow = copy.getElements();
     int numberArtificials = 0;
     // We could use nonlinearcost to do segments - maybe later
#define SEGMENTS 3
     // Penalties may be adjusted by duals
     // Both these should be modified depending on problem
     // Possibly start with big bounds
     //double penalties[]={1.0e-3,1.0e7,1.0e9};
     double penalties[] = {1.0e7, 1.0e8, 1.0e9};
     double bounds[] = {1.0e-2, 1.0e2, COIN_DBL_MAX};
     // see how many extra we need
     CoinBigIndex numberExtra = 0;
     for (iConstraint = 0; iConstraint < numberConstraints; iConstraint++) {
          ClpConstraint * constraint = constraints[iConstraint];
          int iRow = constraint->rowNumber();
          assert (iRow >= 0);
          int n = constraint->numberCoefficients() - rowLength[iRow];
          numberExtra += n;
          if (iRow >= numberRows_)
               numberBad++;
          else if (rowLength[iRow] && n)
               numberBad++;
          if (rowLower_[iRow] > -1.0e20)
               numberArtificials++;
          if (rowUpper_[iRow] < 1.0e20)
               numberArtificials++;
     }
     if (numberBad)
          return numberBad;
     ClpObjective * trueObjective = NULL;
     if (objective_->type() >= 2) {
          // Replace objective
          trueObjective = objective_;
          objective_ = new ClpLinearObjective(NULL, numberColumns_);
     }
     ClpSimplex newModel(*this);
     // we can put back true objective
     if (trueObjective) {
          // Replace objective
          delete objective_;
          objective_ = trueObjective;
     }
     int numberColumns2 = numberColumns_;
     int numberSmallGap = numberArtificials;
     if (numberArtificials) {
          numberArtificials *= SEGMENTS;
          numberColumns2 += numberArtificials;
          int * addStarts = new int [numberArtificials+1];
          int * addRow = new int[numberArtificials];
          double * addElement = new double[numberArtificials];
          double * addUpper = new double[numberArtificials];
          addStarts[0] = 0;
          double * addCost = new double [numberArtificials];
          numberArtificials = 0;
          for (iConstraint = 0; iConstraint < numberConstraints; iConstraint++) {
               ClpConstraint * constraint = constraints[iConstraint];
               int iRow = constraint->rowNumber();
               if (rowLower_[iRow] > -1.0e20) {
                    for (int k = 0; k < SEGMENTS; k++) {
                         addRow[numberArtificials] = iRow;
                         addElement[numberArtificials] = 1.0;
                         addCost[numberArtificials] = penalties[k];
                         addUpper[numberArtificials] = bounds[k];
                         numberArtificials++;
                         addStarts[numberArtificials] = numberArtificials;
                    }
               }
               if (rowUpper_[iRow] < 1.0e20) {
                    for (int k = 0; k < SEGMENTS; k++) {
                         addRow[numberArtificials] = iRow;
                         addElement[numberArtificials] = -1.0;
                         addCost[numberArtificials] = penalties[k];
                         addUpper[numberArtificials] = bounds[k];
                         numberArtificials++;
                         addStarts[numberArtificials] = numberArtificials;
                    }
               }
          }
          newModel.addColumns(numberArtificials, NULL, addUpper, addCost,
                              addStarts, addRow, addElement);
          delete [] addStarts;
          delete [] addRow;
          delete [] addElement;
          delete [] addUpper;
          delete [] addCost;
          //    newModel.primal(1);
     }
     // find nonlinear columns
     int * listNonLinearColumn = new int [numberColumns_+numberSmallGap];
     char * mark = new char[numberColumns_];
     memset(mark, 0, numberColumns_);
     for (iConstraint = 0; iConstraint < numberConstraints; iConstraint++) {
          ClpConstraint * constraint = constraints[iConstraint];
          constraint->markNonlinear(mark);
     }
     if (trueObjective)
          trueObjective->markNonlinear(mark);
     int iColumn;
     int numberNonLinearColumns = 0;
     for (iColumn = 0; iColumn < numberColumns_; iColumn++) {
          if (mark[iColumn])
               listNonLinearColumn[numberNonLinearColumns++] = iColumn;
     }
     // and small gap artificials
     for (iColumn = numberColumns_; iColumn < numberColumns2; iColumn += SEGMENTS) {
          listNonLinearColumn[numberNonLinearColumns++] = iColumn;
     }
     // build row copy of matrix with space for nonzeros
     // Get column copy
     columnCopy = newModel.matrix();
     copy.reverseOrderedCopyOf(*columnCopy);
     // get matrix data pointers
     const int * column = copy.getIndices();
     const CoinBigIndex * rowStart = copy.getVectorStarts();
     rowLength = copy.getVectorLengths();
     const double * elementByRow = copy.getElements();
     numberExtra += copy.getNumElements();
     CoinBigIndex * newStarts = new CoinBigIndex [numberRows_+1];
     int * newColumn = new int[numberExtra];
     double * newElement = new double[numberExtra];
     newStarts[0] = 0;
     int * backRow = new int [numberRows_];
     int iRow;
     for (iRow = 0; iRow < numberRows_; iRow++)
          backRow[iRow] = -1;
     for (iConstraint = 0; iConstraint < numberConstraints; iConstraint++) {
          ClpConstraint * constraint = constraints[iConstraint];
          iRow = constraint->rowNumber();
          backRow[iRow] = iConstraint;
     }
     numberExtra = 0;
     for (iRow = 0; iRow < numberRows_; iRow++) {
          if (backRow[iRow] < 0) {
               // copy normal
               for (CoinBigIndex j = rowStart[iRow]; j < rowStart[iRow] + rowLength[iRow];
                         j++) {
                    newColumn[numberExtra] = column[j];
                    newElement[numberExtra++] = elementByRow[j];
               }
          } else {
               ClpConstraint * constraint = constraints[backRow[iRow]];
               assert(iRow == constraint->rowNumber());
               int numberArtificials = 0;
               if (rowLower_[iRow] > -1.0e20)
                    numberArtificials += SEGMENTS;
               if (rowUpper_[iRow] < 1.0e20)
                    numberArtificials += SEGMENTS;
               if (numberArtificials == rowLength[iRow]) {
                    // all possible
                    memset(mark, 0, numberColumns_);
                    constraint->markNonzero(mark);
                    for (int k = 0; k < numberColumns_; k++) {
                         if (mark[k]) {
                              newColumn[numberExtra] = k;
                              newElement[numberExtra++] = 1.0;
                         }
                    }
                    // copy artificials
                    for (CoinBigIndex j = rowStart[iRow]; j < rowStart[iRow] + rowLength[iRow];
                              j++) {
                         newColumn[numberExtra] = column[j];
                         newElement[numberExtra++] = elementByRow[j];
                    }
               } else {
                    // there already
                    // copy
                    for (CoinBigIndex j = rowStart[iRow]; j < rowStart[iRow] + rowLength[iRow];
                              j++) {
                         newColumn[numberExtra] = column[j];
                         assert (elementByRow[j]);
                         newElement[numberExtra++] = elementByRow[j];
                    }
               }
          }
          newStarts[iRow+1] = numberExtra;
     }
     delete [] backRow;
     CoinPackedMatrix saveMatrix(false, numberColumns2, numberRows_,
                                 numberExtra, newElement, newColumn, newStarts, NULL, 0.0, 0.0);
     delete [] newStarts;
     delete [] newColumn;
     delete [] newElement;
     delete [] mark;
     // get feasible
     if (whichWay < 0.0) {
          newModel.setOptimizationDirection(1.0);
          double * objective = newModel.objective();
          for (int iColumn = 0; iColumn < numberColumns_; iColumn++)
               objective[iColumn] = - objective[iColumn];
     }
     newModel.primal(1);
     // still infeasible
     if (newModel.problemStatus() == 1) {
          delete [] listNonLinearColumn;
          return 0;
     } else if (newModel.problemStatus() == 2) {
          // unbounded - add bounds
          double * columnLower = newModel.columnLower();
          double * columnUpper = newModel.columnUpper();
          for (int i = 0; i < numberColumns_; i++) {
               columnLower[i] = CoinMax(-1.0e8, columnLower[i]);
               columnUpper[i] = CoinMin(1.0e8, columnUpper[i]);
          }
          newModel.primal(1);
     }
     int numberRows = newModel.numberRows();
     double * columnLower = newModel.columnLower();
     double * columnUpper = newModel.columnUpper();
     double * objective = newModel.objective();
     double * rowLower = newModel.rowLower();
     double * rowUpper = newModel.rowUpper();
     double * solution = newModel.primalColumnSolution();
     int jNon;
     int * last[3];

     double * trust = new double[numberNonLinearColumns];
     double * trueLower = new double[numberNonLinearColumns];
     double * trueUpper = new double[numberNonLinearColumns];
     double objectiveOffset;
     double objectiveOffset2;
     getDblParam(ClpObjOffset, objectiveOffset);
     objectiveOffset *= whichWay;
     for (jNon = 0; jNon < numberNonLinearColumns; jNon++) {
          iColumn = listNonLinearColumn[jNon];
          double upper = columnUpper[iColumn];
          double lower = columnLower[iColumn];
          if (solution[iColumn] < lower)
               solution[iColumn] = lower;
          else if (solution[iColumn] > upper)
               solution[iColumn] = upper;
#if 0
          double large = CoinMax(1000.0, 10.0 * fabs(solution[iColumn]));
          if (upper > 1.0e10)
               upper = solution[iColumn] + large;
          if (lower < -1.0e10)
               lower = solution[iColumn] - large;
#else
          upper = solution[iColumn] + 0.5;
          lower = solution[iColumn] - 0.5;
#endif
          //columnUpper[iColumn]=upper;
          trust[jNon] = 0.05 * (1.0 + upper - lower);
          trueLower[jNon] = columnLower[iColumn];
          //trueUpper[jNon]=upper;
          trueUpper[jNon] = columnUpper[iColumn];
     }
     bool tryFix = false;
     int iPass;
     double lastObjective = 1.0e31;
     double lastGoodObjective = 1.0e31;
     double * bestSolution = NULL;
     double * saveSolution = new double [numberColumns2+numberRows];
     char * saveStatus = new char [numberColumns2+numberRows];
     double targetDrop = 1.0e31;
     // 1 bound up, 2 up, -1 bound down, -2 down, 0 no change
     for (iPass = 0; iPass < 3; iPass++) {
          last[iPass] = new int[numberNonLinearColumns];
          for (jNon = 0; jNon < numberNonLinearColumns; jNon++)
               last[iPass][jNon] = 0;
     }
     int numberZeroPasses = 0;
     bool zeroTargetDrop = false;
     double * gradient = new double [numberColumns_];
     bool goneFeasible = false;
     // keep sum of artificials
#define KEEP_SUM 5
     double sumArt[KEEP_SUM];
     for (jNon = 0; jNon < KEEP_SUM; jNon++)
          sumArt[jNon] = COIN_DBL_MAX;
#define SMALL_FIX 0.0
     for (iPass = 0; iPass < numberPasses; iPass++) {
          objectiveOffset2 = objectiveOffset;
          for (jNon = 0; jNon < numberNonLinearColumns; jNon++) {
               iColumn = listNonLinearColumn[jNon];
               if (solution[iColumn] < trueLower[jNon])
                    solution[iColumn] = trueLower[jNon];
               else if (solution[iColumn] > trueUpper[jNon])
                    solution[iColumn] = trueUpper[jNon];
               columnLower[iColumn] = CoinMax(solution[iColumn]
                                              - trust[jNon],
                                              trueLower[jNon]);
               if (!trueLower[jNon] && columnLower[iColumn] < SMALL_FIX)
                    columnLower[iColumn] = SMALL_FIX;
               columnUpper[iColumn] = CoinMin(solution[iColumn]
                                              + trust[jNon],
                                              trueUpper[jNon]);
               if (!trueLower[jNon])
                    columnUpper[iColumn] = CoinMax(columnUpper[iColumn],
                                                   columnLower[iColumn] + SMALL_FIX);
               if (!trueLower[jNon] && tryFix &&
                         columnLower[iColumn] == SMALL_FIX &&
                         columnUpper[iColumn] < 3.0 * SMALL_FIX) {
                    columnLower[iColumn] = 0.0;
                    solution[iColumn] = 0.0;
                    columnUpper[iColumn] = 0.0;
                    printf("fixing %d\n", iColumn);
               }
          }
          // redo matrix
          double offset;
          CoinPackedMatrix newMatrix(saveMatrix);
          // get matrix data pointers
          column = newMatrix.getIndices();
          rowStart = newMatrix.getVectorStarts();
          rowLength = newMatrix.getVectorLengths();
          // make sure x updated
          if (numberConstraints)
               constraints[0]->newXValues();
          else
               trueObjective->newXValues();
          double * changeableElement = newMatrix.getMutableElements();
          if (trueObjective) {
               CoinMemcpyN(trueObjective->gradient(this, solution, offset, true, 2),	numberColumns_,
                           objective);
          } else {
               CoinMemcpyN(objective_->gradient(this, solution, offset, true, 2),	numberColumns_,
                           objective);
          }
          if (whichWay < 0.0) {
               for (int iColumn = 0; iColumn < numberColumns_; iColumn++)
                    objective[iColumn] = - objective[iColumn];
          }
          for (iConstraint = 0; iConstraint < numberConstraints; iConstraint++) {
               ClpConstraint * constraint = constraints[iConstraint];
               int iRow = constraint->rowNumber();
               double functionValue;
#ifndef NDEBUG
               int numberErrors =
#endif
                    constraint->gradient(&newModel, solution, gradient, functionValue, offset);
               assert (!numberErrors);
               // double dualValue = newModel.dualRowSolution()[iRow];
               int numberCoefficients = constraint->numberCoefficients();
               for (CoinBigIndex j = rowStart[iRow]; j < rowStart[iRow] + numberCoefficients; j++) {
                    int iColumn = column[j];
                    changeableElement[j] = gradient[iColumn];
                    //objective[iColumn] -= dualValue*gradient[iColumn];
                    gradient[iColumn] = 0.0;
               }
               for (int k = 0; k < numberColumns_; k++)
                    assert (!gradient[k]);
               if (rowLower_[iRow] > -1.0e20)
                    rowLower[iRow] = rowLower_[iRow] - offset;
               if (rowUpper_[iRow] < 1.0e20)
                    rowUpper[iRow] = rowUpper_[iRow] - offset;
          }
          // Replace matrix
          // Get a column copy in standard format
          CoinPackedMatrix * columnCopy = new CoinPackedMatrix();;
          columnCopy->reverseOrderedCopyOf(newMatrix);
          newModel.replaceMatrix(columnCopy, true);
          // solve
          newModel.primal(1);
          if (newModel.status() == 1) {
               // Infeasible!
               newModel.allSlackBasis();
               newModel.primal();
               newModel.writeMps("infeas.mps");
               assert(!newModel.status());
          }
          double sumInfeas = 0.0;
          int numberInfeas = 0;
          for (iColumn = numberColumns_; iColumn < numberColumns2; iColumn++) {
               if (solution[iColumn] > 1.0e-8) {
                    numberInfeas++;
                    sumInfeas += solution[iColumn];
               }
          }
          printf("%d artificial infeasibilities - summing to %g\n",
                 numberInfeas, sumInfeas);
          for (jNon = 0; jNon < KEEP_SUM - 1; jNon++)
               sumArt[jNon] = sumArt[jNon+1];
          sumArt[KEEP_SUM-1] = sumInfeas;
          if (sumInfeas > 0.01 && sumInfeas * 1.1 > sumArt[0] && penalties[1] < 1.0e7) {
               // not doing very well - increase - be more sophisticated later
               lastObjective = COIN_DBL_MAX;
               for (jNon = 0; jNon < KEEP_SUM; jNon++)
                    sumArt[jNon] = COIN_DBL_MAX;
               //for (iColumn=numberColumns_;iColumn<numberColumns2;iColumn+=SEGMENTS) {
               //objective[iColumn+1] *= 1.5;
               //}
               penalties[1] *= 1.5;
               for (jNon = 0; jNon < numberNonLinearColumns; jNon++)
                    if (trust[jNon] > 0.1)
                         trust[jNon] *= 2.0;
                    else
                         trust[jNon] = 0.1;
          }
          if (sumInfeas < 0.001 && !goneFeasible) {
               goneFeasible = true;
               penalties[0] = 1.0e-3;
               penalties[1] = 1.0e6;
               printf("Got feasible\n");
          }
          double infValue = 0.0;
          double objValue = 0.0;
          // make sure x updated
          if (numberConstraints)
               constraints[0]->newXValues();
          else
               trueObjective->newXValues();
          if (trueObjective) {
               objValue = trueObjective->objectiveValue(this, solution);
               printf("objective offset %g\n", offset);
               objectiveOffset2 = objectiveOffset + offset; // ? sign
               newModel.setObjectiveOffset(objectiveOffset2);
          } else {
               objValue = objective_->objectiveValue(this, solution);
          }
          objValue *= whichWay;
          double infPenalty = 0.0;
          // This penalty is for target drop
          double infPenalty2 = 0.0;
          //const int * row = columnCopy->getIndices();
          //const CoinBigIndex * columnStart = columnCopy->getVectorStarts();
          //const int * columnLength = columnCopy->getVectorLengths();
          //const double * element = columnCopy->getElements();
          double * cost = newModel.objective();
          column = newMatrix.getIndices();
          rowStart = newMatrix.getVectorStarts();
          rowLength = newMatrix.getVectorLengths();
          elementByRow = newMatrix.getElements();
          int jColumn = numberColumns_;
          double objectiveAdjustment = 0.0;
          for (iConstraint = 0; iConstraint < numberConstraints; iConstraint++) {
               ClpConstraint * constraint = constraints[iConstraint];
               int iRow = constraint->rowNumber();
               double functionValue = constraint->functionValue(this, solution);
               double dualValue = newModel.dualRowSolution()[iRow];
               if (numberConstraints < -50)
                    printf("For row %d current value is %g (row activity %g) , dual is %g\n", iRow, functionValue,
                           newModel.primalRowSolution()[iRow],
                           dualValue);
               double movement = newModel.primalRowSolution()[iRow] + constraint->offset();
               movement = fabs((movement - functionValue) * dualValue);
               infPenalty2 += movement;
               double sumOfActivities = 0.0;
               for (CoinBigIndex j = rowStart[iRow]; j < rowStart[iRow] + rowLength[iRow]; j++) {
                    int iColumn = column[j];
                    sumOfActivities += fabs(solution[iColumn] * elementByRow[j]);
               }
               if (rowLower_[iRow] > -1.0e20) {
                    if (functionValue < rowLower_[iRow] - 1.0e-5) {
                         double infeasibility = rowLower_[iRow] - functionValue;
                         double thisPenalty = 0.0;
                         infValue += infeasibility;
                         int k;
                         assert (dualValue >= -1.0e-5);
                         dualValue = CoinMax(dualValue, 0.0);
                         for ( k = 0; k < SEGMENTS; k++) {
                              if (infeasibility <= 0)
                                   break;
                              double thisPart = CoinMin(infeasibility, bounds[k]);
                              thisPenalty += thisPart * cost[jColumn+k];
                              infeasibility -= thisPart;
                         }
                         infeasibility = functionValue - rowUpper_[iRow];
                         double newPenalty = 0.0;
                         for ( k = 0; k < SEGMENTS; k++) {
                              double thisPart = CoinMin(infeasibility, bounds[k]);
                              cost[jColumn+k] = CoinMax(penalties[k], dualValue + 1.0e-3);
                              newPenalty += thisPart * cost[jColumn+k];
                              infeasibility -= thisPart;
                         }
                         infPenalty += thisPenalty;
                         objectiveAdjustment += CoinMax(0.0, newPenalty - thisPenalty);
                    }
                    jColumn += SEGMENTS;
               }
               if (rowUpper_[iRow] < 1.0e20) {
                    if (functionValue > rowUpper_[iRow] + 1.0e-5) {
                         double infeasibility = functionValue - rowUpper_[iRow];
                         double thisPenalty = 0.0;
                         infValue += infeasibility;
                         int k;
                         dualValue = -dualValue;
                         assert (dualValue >= -1.0e-5);
                         dualValue = CoinMax(dualValue, 0.0);
                         for ( k = 0; k < SEGMENTS; k++) {
                              if (infeasibility <= 0)
                                   break;
                              double thisPart = CoinMin(infeasibility, bounds[k]);
                              thisPenalty += thisPart * cost[jColumn+k];
                              infeasibility -= thisPart;
                         }
                         infeasibility = functionValue - rowUpper_[iRow];
                         double newPenalty = 0.0;
                         for ( k = 0; k < SEGMENTS; k++) {
                              double thisPart = CoinMin(infeasibility, bounds[k]);
                              cost[jColumn+k] = CoinMax(penalties[k], dualValue + 1.0e-3);
                              newPenalty += thisPart * cost[jColumn+k];
                              infeasibility -= thisPart;
                         }
                         infPenalty += thisPenalty;
                         objectiveAdjustment += CoinMax(0.0, newPenalty - thisPenalty);
                    }
                    jColumn += SEGMENTS;
               }
          }
          // adjust last objective value
          lastObjective += objectiveAdjustment;
          if (infValue)
               printf("Sum infeasibilities %g - penalty %g ", infValue, infPenalty);
          if (objectiveOffset2)
               printf("offset2 %g ", objectiveOffset2);
          objValue -= objectiveOffset2;
          printf("True objective %g or maybe %g (with penalty %g) -pen2 %g %g\n", objValue,
                 objValue + objectiveOffset2, objValue + objectiveOffset2 + infPenalty, infPenalty2, penalties[1]);
          double useObjValue = objValue + objectiveOffset2 + infPenalty;
          objValue += infPenalty + infPenalty2;
          objValue = useObjValue;
          if (iPass) {
               double drop = lastObjective - objValue;
               std::cout << "True drop was " << drop << std::endl;
               if (drop < -0.05 * fabs(objValue) - 1.0e-4) {
                    // pretty bad - go back and halve
                    CoinMemcpyN(saveSolution, numberColumns2, solution);
                    CoinMemcpyN(saveSolution + numberColumns2,
                                numberRows, newModel.primalRowSolution());
                    CoinMemcpyN(reinterpret_cast<unsigned char *> (saveStatus),
                                numberColumns2 + numberRows, newModel.statusArray());
                    for (jNon = 0; jNon < numberNonLinearColumns; jNon++)
                         if (trust[jNon] > 0.1)
                              trust[jNon] *= 0.5;
                         else
                              trust[jNon] *= 0.9;

                    printf("** Halving trust\n");
                    objValue = lastObjective;
                    continue;
               } else if ((iPass % 25) == -1) {
                    for (jNon = 0; jNon < numberNonLinearColumns; jNon++)
                         trust[jNon] *= 2.0;
                    printf("** Doubling trust\n");
               }
               int * temp = last[2];
               last[2] = last[1];
               last[1] = last[0];
               last[0] = temp;
               for (jNon = 0; jNon < numberNonLinearColumns; jNon++) {
                    iColumn = listNonLinearColumn[jNon];
                    double change = solution[iColumn] - saveSolution[iColumn];
                    if (change < -1.0e-5) {
                         if (fabs(change + trust[jNon]) < 1.0e-5)
                              temp[jNon] = -1;
                         else
                              temp[jNon] = -2;
                    } else if(change > 1.0e-5) {
                         if (fabs(change - trust[jNon]) < 1.0e-5)
                              temp[jNon] = 1;
                         else
                              temp[jNon] = 2;
                    } else {
                         temp[jNon] = 0;
                    }
               }
               double maxDelta = 0.0;
               double smallestTrust = 1.0e31;
               double smallestNonLinearGap = 1.0e31;
               double smallestGap = 1.0e31;
               bool increasing = false;
               for (iColumn = 0; iColumn < numberColumns_; iColumn++) {
                    double gap = columnUpper[iColumn] - columnLower[iColumn];
                    assert (gap >= 0.0);
                    if (gap)
                         smallestGap = CoinMin(smallestGap, gap);
               }
               for (jNon = 0; jNon < numberNonLinearColumns; jNon++) {
                    iColumn = listNonLinearColumn[jNon];
                    double gap = columnUpper[iColumn] - columnLower[iColumn];
                    assert (gap >= 0.0);
                    if (gap) {
                         smallestNonLinearGap = CoinMin(smallestNonLinearGap, gap);
                         if (gap < 1.0e-7 && iPass == 1) {
                              printf("Small gap %d %d %g %g %g\n",
                                     jNon, iColumn, columnLower[iColumn], columnUpper[iColumn],
                                     gap);
                              //trueUpper[jNon]=trueLower[jNon];
                              //columnUpper[iColumn]=columnLower[iColumn];
                         }
                    }
                    maxDelta = CoinMax(maxDelta,
                                       fabs(solution[iColumn] - saveSolution[iColumn]));
                    if (last[0][jNon]*last[1][jNon] < 0) {
                         // halve
                         if (trust[jNon] > 1.0)
                              trust[jNon] *= 0.5;
                         else
                              trust[jNon] *= 0.7;
                    } else {
                         // ? only increase if +=1 ?
                         if (last[0][jNon] == last[1][jNon] &&
                                   last[0][jNon] == last[2][jNon] &&
                                   last[0][jNon]) {
                              trust[jNon] *= 1.8;
                              increasing = true;
                         }
                    }
                    smallestTrust = CoinMin(smallestTrust, trust[jNon]);
               }
               std::cout << "largest delta is " << maxDelta
                         << ", smallest trust is " << smallestTrust
                         << ", smallest gap is " << smallestGap
                         << ", smallest nonlinear gap is " << smallestNonLinearGap
                         << std::endl;
               if (iPass > 200) {
                    //double useObjValue = objValue+objectiveOffset2+infPenalty;
                    if (useObjValue + 1.0e-4 >	lastGoodObjective && iPass > 250) {
                         std::cout << "Exiting as objective not changing much" << std::endl;
                         break;
                    } else if (useObjValue < lastGoodObjective) {
                         lastGoodObjective = useObjValue;
                         if (!bestSolution)
                              bestSolution = new double [numberColumns2];
                         CoinMemcpyN(solution, numberColumns2, bestSolution);
                    }
               }
               if (maxDelta < deltaTolerance && !increasing && iPass > 100) {
                    numberZeroPasses++;
                    if (numberZeroPasses == 3) {
                         if (tryFix) {
                              std::cout << "Exiting" << std::endl;
                              break;
                         } else {
                              tryFix = true;
                              for (jNon = 0; jNon < numberNonLinearColumns; jNon++)
                                   trust[jNon] = CoinMax(trust[jNon], 1.0e-1);
                              numberZeroPasses = 0;
                         }
                    }
               } else {
                    numberZeroPasses = 0;
               }
          }
          CoinMemcpyN(solution, numberColumns2, saveSolution);
          CoinMemcpyN(newModel.primalRowSolution(),
                      numberRows, saveSolution + numberColumns2);
          CoinMemcpyN(newModel.statusArray(),
                      numberColumns2 + numberRows,
                      reinterpret_cast<unsigned char *> (saveStatus));

          targetDrop = infPenalty + infPenalty2;
          if (iPass) {
               // get reduced costs
               const double * pi = newModel.dualRowSolution();
               newModel.matrix()->transposeTimes(pi,
                                                 newModel.dualColumnSolution());
               double * r = newModel.dualColumnSolution();
               for (iColumn = 0; iColumn < numberColumns_; iColumn++)
                    r[iColumn] = objective[iColumn] - r[iColumn];
               for (jNon = 0; jNon < numberNonLinearColumns; jNon++) {
                    iColumn = listNonLinearColumn[jNon];
                    double dj = r[iColumn];
                    if (dj < -1.0e-6) {
                         double drop = -dj * (columnUpper[iColumn] - solution[iColumn]);
                         //double upper = CoinMin(trueUpper[jNon],solution[iColumn]+0.1);
                         //double drop2 = -dj*(upper-solution[iColumn]);
#if 0
                         if (drop > 1.0e8 || drop2 > 100.0 * drop || (drop > 1.0e-2 && iPass > 100))
                              printf("Big drop %d %g %g %g %g T %g\n",
                                     iColumn, columnLower[iColumn], solution[iColumn],
                                     columnUpper[iColumn], dj, trueUpper[jNon]);
#endif
                         targetDrop += drop;
                         if (dj < -1.0e-1 && trust[jNon] < 1.0e-3
                                   && trueUpper[jNon] - solution[iColumn] > 1.0e-2) {
                              trust[jNon] *= 1.5;
                              //printf("Increasing trust on %d to %g\n",
                              //     iColumn,trust[jNon]);
                         }
                    } else if (dj > 1.0e-6) {
                         double drop = -dj * (columnLower[iColumn] - solution[iColumn]);
                         //double lower = CoinMax(trueLower[jNon],solution[iColumn]-0.1);
                         //double drop2 = -dj*(lower-solution[iColumn]);
#if 0
                         if (drop > 1.0e8 || drop2 > 100.0 * drop || (drop > 1.0e-2))
                              printf("Big drop %d %g %g %g %g T %g\n",
                                     iColumn, columnLower[iColumn], solution[iColumn],
                                     columnUpper[iColumn], dj, trueLower[jNon]);
#endif
                         targetDrop += drop;
                         if (dj > 1.0e-1 && trust[jNon] < 1.0e-3
                                   && solution[iColumn] - trueLower[jNon] > 1.0e-2) {
                              trust[jNon] *= 1.5;
                              printf("Increasing trust on %d to %g\n",
                                     iColumn, trust[jNon]);
                         }
                    }
               }
          }
          std::cout << "Pass - " << iPass
                    << ", target drop is " << targetDrop
                    << std::endl;
          if (iPass > 1 && targetDrop < 1.0e-5 && zeroTargetDrop)
               break;
          if (iPass > 1 && targetDrop < 1.0e-5)
               zeroTargetDrop = true;
          else
               zeroTargetDrop = false;
          //if (iPass==5)
          //newModel.setLogLevel(63);
          lastObjective = objValue;
          // take out when ClpPackedMatrix changed
          //newModel.scaling(false);
#if 0
          CoinMpsIO writer;
          writer.setMpsData(*newModel.matrix(), COIN_DBL_MAX,
                            newModel.getColLower(), newModel.getColUpper(),
                            newModel.getObjCoefficients(),
                            (const char*) 0 /*integrality*/,
                            newModel.getRowLower(), newModel.getRowUpper(),
                            NULL, NULL);
          writer.writeMps("xx.mps");
#endif
     }
     delete [] saveSolution;
     delete [] saveStatus;
     for (iPass = 0; iPass < 3; iPass++)
          delete [] last[iPass];
     for (jNon = 0; jNon < numberNonLinearColumns; jNon++) {
          iColumn = listNonLinearColumn[jNon];
          columnLower[iColumn] = trueLower[jNon];
          columnUpper[iColumn] = trueUpper[jNon];
     }
     delete [] trust;
     delete [] trueUpper;
     delete [] trueLower;
     objectiveValue_ = newModel.objectiveValue();
     if (bestSolution) {
          CoinMemcpyN(bestSolution, numberColumns2, solution);
          delete [] bestSolution;
          printf("restoring objective of %g\n", lastGoodObjective);
          objectiveValue_ = lastGoodObjective;
     }
     // Simplest way to get true row activity ?
     double * rowActivity = newModel.primalRowSolution();
     for (iRow = 0; iRow < numberRows; iRow++) {
          double difference;
          if (fabs(rowLower_[iRow]) < fabs(rowUpper_[iRow]))
               difference = rowLower_[iRow] - rowLower[iRow];
          else
               difference = rowUpper_[iRow] - rowUpper[iRow];
          rowLower[iRow] = rowLower_[iRow];
          rowUpper[iRow] = rowUpper_[iRow];
          if (difference) {
               if (numberRows < 50)
                    printf("For row %d activity changes from %g to %g\n",
                           iRow, rowActivity[iRow], rowActivity[iRow] + difference);
               rowActivity[iRow] += difference;
          }
     }
     delete [] listNonLinearColumn;
     delete [] gradient;
     printf("solution still in newModel - do objective etc!\n");
     numberIterations_ = newModel.numberIterations();
     problemStatus_ = newModel.problemStatus();
     secondaryStatus_ = newModel.secondaryStatus();
     CoinMemcpyN(newModel.primalColumnSolution(), numberColumns_, columnActivity_);
     // should do status region
     CoinZeroN(rowActivity_, numberRows_);
     matrix_->times(1.0, columnActivity_, rowActivity_);
     return 0;
}
