/* $Id$ */
// Copyright (C) 2003, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

/*
   Implements crude primal dual predictor corrector algorithm

 */
//#define SOME_DEBUG

#include "CoinPragma.hpp"
#include <math.h>

#include "CoinHelperFunctions.hpp"
#include "ClpPredictorCorrector.hpp"
#include "ClpEventHandler.hpp"
#include "CoinPackedMatrix.hpp"
#include "ClpMessage.hpp"
#include "ClpCholeskyBase.hpp"
#include "ClpHelperFunctions.hpp"
#include "ClpQuadraticObjective.hpp"
#include <cfloat>
#include <cassert>
#include <string>
#include <cstdio>
#include <iostream>
#if 0
static int yyyyyy = 0;
void ClpPredictorCorrector::saveSolution(std::string fileName)
{
     FILE * fp = fopen(fileName.c_str(), "wb");
     if (fp) {
          int numberRows = numberRows_;
          int numberColumns = numberColumns_;
          fwrite(&numberRows, sizeof(int), 1, fp);
          fwrite(&numberColumns, sizeof(int), 1, fp);
          CoinWorkDouble dsave[20];
          memset(dsave, 0, sizeof(dsave));
          fwrite(dsave, sizeof(CoinWorkDouble), 20, fp);
          int msave[20];
          memset(msave, 0, sizeof(msave));
          msave[0] = numberIterations_;
          fwrite(msave, sizeof(int), 20, fp);
          fwrite(dual_, sizeof(CoinWorkDouble), numberRows, fp);
          fwrite(errorRegion_, sizeof(CoinWorkDouble), numberRows, fp);
          fwrite(rhsFixRegion_, sizeof(CoinWorkDouble), numberRows, fp);
          fwrite(solution_, sizeof(CoinWorkDouble), numberColumns, fp);
          fwrite(solution_ + numberColumns, sizeof(CoinWorkDouble), numberRows, fp);
          fwrite(diagonal_, sizeof(CoinWorkDouble), numberColumns, fp);
          fwrite(diagonal_ + numberColumns, sizeof(CoinWorkDouble), numberRows, fp);
          fwrite(wVec_, sizeof(CoinWorkDouble), numberColumns, fp);
          fwrite(wVec_ + numberColumns, sizeof(CoinWorkDouble), numberRows, fp);
          fwrite(zVec_, sizeof(CoinWorkDouble), numberColumns, fp);
          fwrite(zVec_ + numberColumns, sizeof(CoinWorkDouble), numberRows, fp);
          fwrite(upperSlack_, sizeof(CoinWorkDouble), numberColumns, fp);
          fwrite(upperSlack_ + numberColumns, sizeof(CoinWorkDouble), numberRows, fp);
          fwrite(lowerSlack_, sizeof(CoinWorkDouble), numberColumns, fp);
          fwrite(lowerSlack_ + numberColumns, sizeof(CoinWorkDouble), numberRows, fp);
          fclose(fp);
     } else {
          std::cout << "Unable to open file " << fileName << std::endl;
     }
}
#endif
// Could change on CLP_LONG_CHOLESKY or COIN_LONG_WORK?
static CoinWorkDouble eScale = 1.0e27;
static CoinWorkDouble eBaseCaution = 1.0e-12;
static CoinWorkDouble eBase = 1.0e-12;
static CoinWorkDouble eDiagonal = 1.0e25;
static CoinWorkDouble eDiagonalCaution = 1.0e18;
static CoinWorkDouble eExtra = 1.0e-12;

// main function

int ClpPredictorCorrector::solve ( )
{
     problemStatus_ = -1;
     algorithm_ = 1;
     //create all regions
     if (!createWorkingData()) {
          problemStatus_ = 4;
          return 2;
     }
#if COIN_LONG_WORK
     // reallocate some regions
     double * dualSave = dual_;
     dual_ = reinterpret_cast<double *>(new CoinWorkDouble[numberRows_]);
     double * reducedCostSave = reducedCost_;
     reducedCost_ = reinterpret_cast<double *>(new CoinWorkDouble[numberColumns_]);
#endif
     //diagonalPerturbation_=1.0e-25;
     ClpMatrixBase * saveMatrix = NULL;
     // If quadratic then make copy so we can actually scale or normalize
#ifndef NO_RTTI
     ClpQuadraticObjective * quadraticObj = (dynamic_cast< ClpQuadraticObjective*>(objective_));
#else
     ClpQuadraticObjective * quadraticObj = NULL;
     if (objective_->type() == 2)
          quadraticObj = (static_cast< ClpQuadraticObjective*>(objective_));
#endif
     /* If modeSwitch is :
        0 - normal
        1 - bit switch off centering
        2 - bit always do type 2
        4 - accept corrector nearly always
     */
     int modeSwitch = 0;
     //if (quadraticObj)
     //modeSwitch |= 1; // switch off centring for now
     //if (quadraticObj)
     //modeSwitch |=4;
     ClpObjective * saveObjective = NULL;
     if (quadraticObj) {
          // check KKT is on
          if (!cholesky_->kkt()) {
               //No!
               handler_->message(CLP_BARRIER_KKT, messages_)
                         << CoinMessageEol;
               return -1;
          }
          saveObjective = objective_;
          // We are going to make matrix full rather half
          objective_ = new ClpQuadraticObjective(*quadraticObj, 1);
     }
     bool allowIncreasingGap = (modeSwitch & 4) != 0;
     // If scaled then really scale matrix
     if (scalingFlag_ > 0 && rowScale_) {
          saveMatrix = matrix_;
          matrix_ = matrix_->scaledColumnCopy(this);
     }
     //initializeFeasible(); - this just set fixed flag
     smallestInfeasibility_ = COIN_DBL_MAX;
     int i;
     for (i = 0; i < LENGTH_HISTORY; i++)
          historyInfeasibility_[i] = COIN_DBL_MAX;

     //bool firstTime=true;
     //firstFactorization(true);
     int returnCode = cholesky_->order(this);
     if (returnCode || cholesky_->symbolic()) {
       COIN_DETAIL_PRINT(printf("Error return from symbolic - probably not enough memory\n"));
          problemStatus_ = 4;
          //delete all temporary regions
          deleteWorkingData();
          if (saveMatrix) {
               // restore normal copy
               delete matrix_;
               matrix_ = saveMatrix;
          }
          // Restore quadratic objective if necessary
          if (saveObjective) {
               delete objective_;
               objective_ = saveObjective;
          }
          return -1;
     }
     mu_ = 1.0e10;
     diagonalScaleFactor_ = 1.0;
     //set iterations
     numberIterations_ = -1;
     int numberTotal = numberRows_ + numberColumns_;
     //initialize solution here
     if(createSolution() < 0) {
       COIN_DETAIL_PRINT(printf("Not enough memory\n"));
          problemStatus_ = 4;
          //delete all temporary regions
          deleteWorkingData();
          if (saveMatrix) {
               // restore normal copy
               delete matrix_;
               matrix_ = saveMatrix;
          }
          return -1;
     }
     CoinWorkDouble * dualArray = reinterpret_cast<CoinWorkDouble *>(dual_);
     // Could try centering steps without any original step i.e. just center
     //firstFactorization(false);
     CoinZeroN(dualArray, numberRows_);
     multiplyAdd(solution_ + numberColumns_, numberRows_, -1.0, errorRegion_, 0.0);
     matrix_->times(1.0, solution_, errorRegion_);
     maximumRHSError_ = maximumAbsElement(errorRegion_, numberRows_);
     maximumBoundInfeasibility_ = maximumRHSError_;
     //CoinWorkDouble maximumDualError_=COIN_DBL_MAX;
     //initialize
     actualDualStep_ = 0.0;
     actualPrimalStep_ = 0.0;
     gonePrimalFeasible_ = false;
     goneDualFeasible_ = false;
     //bool hadGoodSolution=false;
     diagonalNorm_ = solutionNorm_;
     mu_ = solutionNorm_;
     int numberFixed = updateSolution(-COIN_DBL_MAX);
     int numberFixedTotal = numberFixed;
     //int numberRows_DroppedBefore=0;
     //CoinWorkDouble extra=eExtra;
     //CoinWorkDouble maximumPerturbation=COIN_DBL_MAX;
     //constants for infeas interior point
     const CoinWorkDouble beta2 = 0.99995;
     const CoinWorkDouble tau   = 0.00002;
     CoinWorkDouble lastComplementarityGap = COIN_DBL_MAX * 1.0e-20;
     CoinWorkDouble lastStep = 1.0;
     // use to see if to take affine
     CoinWorkDouble checkGap = COIN_DBL_MAX;
     int lastGoodIteration = 0;
     CoinWorkDouble bestObjectiveGap = COIN_DBL_MAX;
     CoinWorkDouble bestObjective = COIN_DBL_MAX;
     int bestKilled = -1;
     int saveIteration = -1;
     int saveIteration2 = -1;
     bool sloppyOptimal = false;
     // this just to be used to exit
     bool sloppyOptimal2 = false;
     CoinWorkDouble * savePi = NULL;
     CoinWorkDouble * savePrimal = NULL;
     CoinWorkDouble * savePi2 = NULL;
     CoinWorkDouble * savePrimal2 = NULL;
     // Extra regions for centering
     CoinWorkDouble * saveX = new CoinWorkDouble[numberTotal];
     CoinWorkDouble * saveY = new CoinWorkDouble[numberRows_];
     CoinWorkDouble * saveZ = new CoinWorkDouble[numberTotal];
     CoinWorkDouble * saveW = new CoinWorkDouble[numberTotal];
     CoinWorkDouble * saveSL = new CoinWorkDouble[numberTotal];
     CoinWorkDouble * saveSU = new CoinWorkDouble[numberTotal];
     // Save smallest mu used in primal dual moves
     CoinWorkDouble objScale = optimizationDirection_ /
                               (rhsScale_ * objectiveScale_);
     while (problemStatus_ < 0) {
          //#define FULL_DEBUG
#ifdef FULL_DEBUG
          {
               int i;
               printf("row    pi          artvec       rhsfx\n");
               for (i = 0; i < numberRows_; i++) {
                    printf("%d %g %g %g\n", i, dual_[i], errorRegion_[i], rhsFixRegion_[i]);
               }
               printf(" col  dsol  ddiag  dwvec  dzvec dbdslu dbdsll\n");
               for (i = 0; i < numberColumns_ + numberRows_; i++) {
                    printf(" %d %g %g %g %g %g %g\n", i, solution_[i], diagonal_[i], wVec_[i],
                           zVec_[i], upperSlack_[i], lowerSlack_[i]);
               }
          }
#endif
          complementarityGap_ = complementarityGap(numberComplementarityPairs_,
                                numberComplementarityItems_, 0);
          handler_->message(CLP_BARRIER_ITERATION, messages_)
                    << numberIterations_
                    << static_cast<double>(primalObjective_ * objScale - dblParam_[ClpObjOffset])
                    << static_cast<double>(dualObjective_ * objScale - dblParam_[ClpObjOffset])
                    << static_cast<double>(complementarityGap_)
                    << numberFixedTotal
                    << cholesky_->rank()
                    << CoinMessageEol;
	  // Check event
	  {
	    int status = eventHandler_->event(ClpEventHandler::endOfIteration);
	    if (status >= 0) {
	      problemStatus_ = 5;
	      secondaryStatus_ = ClpEventHandler::endOfIteration;
	      break;
	    }
	  }
#if 0
          if (numberIterations_ == -1) {
               saveSolution("xxx.sav");
               if (yyyyyy)
                    exit(99);
          }
#endif
          // move up history
          for (i = 1; i < LENGTH_HISTORY; i++)
               historyInfeasibility_[i-1] = historyInfeasibility_[i];
          historyInfeasibility_[LENGTH_HISTORY-1] = complementarityGap_;
          // switch off saved if changes
          //if (saveIteration+10<numberIterations_&&
          //complementarityGap_*2.0<historyInfeasibility_[0])
          //saveIteration=-1;
          lastStep = CoinMin(actualPrimalStep_, actualDualStep_);
          CoinWorkDouble goodGapChange;
          //#define KEEP_GOING_IF_FIXED 5
#ifndef KEEP_GOING_IF_FIXED
#define KEEP_GOING_IF_FIXED 10000
#endif
          if (!sloppyOptimal) {
               goodGapChange = 0.93;
          } else {
               goodGapChange = 0.7;
               if (numberFixed > KEEP_GOING_IF_FIXED)
                    goodGapChange = 0.99; // make more likely to carry on
          }
          CoinWorkDouble gapO;
          CoinWorkDouble lastGood = bestObjectiveGap;
          if (gonePrimalFeasible_ && goneDualFeasible_) {
               CoinWorkDouble largestObjective;
               if (CoinAbs(primalObjective_) > CoinAbs(dualObjective_)) {
                    largestObjective = CoinAbs(primalObjective_);
               } else {
                    largestObjective = CoinAbs(dualObjective_);
               }
               if (largestObjective < 1.0) {
                    largestObjective = 1.0;
               }
               gapO = CoinAbs(primalObjective_ - dualObjective_) / largestObjective;
               handler_->message(CLP_BARRIER_OBJECTIVE_GAP, messages_)
                         << static_cast<double>(gapO)
                         << CoinMessageEol;
               //start saving best
               bool saveIt = false;
               if (gapO < bestObjectiveGap) {
                    bestObjectiveGap = gapO;
#ifndef SAVE_ON_OBJ
                    saveIt = true;
#endif
               }
               if (primalObjective_ < bestObjective) {
                    bestObjective = primalObjective_;
#ifdef SAVE_ON_OBJ
                    saveIt = true;
#endif
               }
               if (numberFixedTotal > bestKilled) {
                    bestKilled = numberFixedTotal;
#if KEEP_GOING_IF_FIXED<10
                    saveIt = true;
#endif
               }
               if (saveIt) {
#if KEEP_GOING_IF_FIXED<10
		 COIN_DETAIL_PRINT(printf("saving\n"));
#endif
                    saveIteration = numberIterations_;
                    if (!savePi) {
                         savePi = new CoinWorkDouble[numberRows_];
                         savePrimal = new CoinWorkDouble [numberTotal];
                    }
                    CoinMemcpyN(dualArray, numberRows_, savePi);
                    CoinMemcpyN(solution_, numberTotal, savePrimal);
               } else if(gapO > 2.0 * bestObjectiveGap) {
                    //maybe be more sophisticated e.g. re-initialize having
                    //fixed variables and dropped rows
                    //std::cout <<" gap increasing "<<std::endl;
               }
               //std::cout <<"could stop"<<std::endl;
               //gapO=0.0;
               if (CoinAbs(primalObjective_ - dualObjective_) < dualTolerance()) {
                    gapO = 0.0;
               }
          } else {
               gapO = COIN_DBL_MAX;
               if (saveIteration >= 0) {
                    handler_->message(CLP_BARRIER_GONE_INFEASIBLE, messages_)
                              << CoinMessageEol;
                    CoinWorkDouble scaledRHSError = maximumRHSError_ / (solutionNorm_ + 10.0);
                    // save alternate
                    if (numberFixedTotal > bestKilled &&
                              maximumBoundInfeasibility_ < 1.0e-6 &&
                              scaledRHSError < 1.0e-2) {
                         bestKilled = numberFixedTotal;
#if KEEP_GOING_IF_FIXED<10
                         COIN_DETAIL_PRINT(printf("saving alternate\n"));
#endif
                         saveIteration2 = numberIterations_;
                         if (!savePi2) {
                              savePi2 = new CoinWorkDouble[numberRows_];
                              savePrimal2 = new CoinWorkDouble [numberTotal];
                         }
                         CoinMemcpyN(dualArray, numberRows_, savePi2);
                         CoinMemcpyN(solution_, numberTotal, savePrimal2);
                    }
                    if (sloppyOptimal2) {
                         // vaguely optimal
                         if (maximumBoundInfeasibility_ > 1.0e-2 ||
                                   scaledRHSError > 1.0e-2 ||
                                   maximumDualError_ > objectiveNorm_ * 1.0e-2) {
                              handler_->message(CLP_BARRIER_EXIT2, messages_)
                                        << saveIteration
                                        << CoinMessageEol;
                              problemStatus_ = 0; // benefit of doubt
                              break;
                         }
                    } else {
                         // not close to optimal but check if getting bad
                         CoinWorkDouble scaledRHSError = maximumRHSError_ / (solutionNorm_ + 10.0);
                         if ((maximumBoundInfeasibility_ > 1.0e-1 ||
                                   scaledRHSError > 1.0e-1 ||
                                   maximumDualError_ > objectiveNorm_ * 1.0e-1)
                                   && (numberIterations_ > 50
                                       && complementarityGap_ > 0.9 * historyInfeasibility_[0])) {
                              handler_->message(CLP_BARRIER_EXIT2, messages_)
                                        << saveIteration
                                        << CoinMessageEol;
                              break;
                         }
                         if (complementarityGap_ > 0.95 * checkGap && bestObjectiveGap < 1.0e-3 &&
                                   (numberIterations_ > saveIteration + 5 || numberIterations_ > 100)) {
                              handler_->message(CLP_BARRIER_EXIT2, messages_)
                                        << saveIteration
                                        << CoinMessageEol;
                              break;
                         }
                    }
               }
               if (complementarityGap_ > 0.5 * checkGap && primalObjective_ >
                         bestObjective + 1.0e-9 &&
                         (numberIterations_ > saveIteration + 5 || numberIterations_ > 100)) {
                    handler_->message(CLP_BARRIER_EXIT2, messages_)
                              << saveIteration
                              << CoinMessageEol;
                    break;
               }
          }
	  // See if we should be thinking about exit if diverging
	  double relativeMultiplier = 1.0 + fabs(primalObjective_) + fabs(dualObjective_);
	  // Quadratic coding is rubbish so be more forgiving?
	  double gapO2 = gapO;
	  if (quadraticObj) {
	    relativeMultiplier *= 5.0;
	    CoinWorkDouble largestObjective;
	    if (CoinAbs(primalObjective_) > CoinAbs(dualObjective_)) {
	      largestObjective = CoinAbs(primalObjective_);
	    } else {
	      largestObjective = CoinAbs(dualObjective_);
	    }
	    if (largestObjective < 1.0) {
	      largestObjective = 1.0;
	    }
	    gapO2 = CoinAbs(primalObjective_ - dualObjective_) / largestObjective;
	  }
	  if (gapO2 < 1.0e-5 + 1.0e-9 * relativeMultiplier
	      || complementarityGap_ < 0.1 + 1.0e-9 * relativeMultiplier)
	      sloppyOptimal2 = true;
          if ((gapO2 < 1.0e-6 || (gapO2 < 1.0e-4 && complementarityGap_ < 0.1)) && !sloppyOptimal) {
	       // save solution if none saved
	       if (saveIteration<0) {
		 saveIteration = numberIterations_;
		 if (!savePi) {
		   savePi = new CoinWorkDouble[numberRows_];
		   savePrimal = new CoinWorkDouble [numberTotal];
		 }
		 CoinMemcpyN(dualArray, numberRows_, savePi);
		 CoinMemcpyN(solution_, numberTotal, savePrimal);
	       }
               sloppyOptimal = true;
               sloppyOptimal2 = true;
               handler_->message(CLP_BARRIER_CLOSE_TO_OPTIMAL, messages_)
                         << numberIterations_ << static_cast<double>(complementarityGap_)
                         << CoinMessageEol;
          }
          int numberBack = quadraticObj ? 10 : 5;
          //tryJustPredictor=true;
          //printf("trying just predictor\n");
          //}
          if (complementarityGap_ >= 1.05 * lastComplementarityGap) {
               handler_->message(CLP_BARRIER_COMPLEMENTARITY, messages_)
                         << static_cast<double>(complementarityGap_) << "increasing"
                         << CoinMessageEol;
               if (saveIteration >= 0 && sloppyOptimal2) {
                    handler_->message(CLP_BARRIER_EXIT2, messages_)
                              << saveIteration
                              << CoinMessageEol;
                    break;
               } else if (numberIterations_ - lastGoodIteration >= numberBack &&
                          complementarityGap_ < 1.0e-6) {
                    break; // not doing very well - give up
               }
          } else if (complementarityGap_ < goodGapChange * lastComplementarityGap) {
               lastGoodIteration = numberIterations_;
               lastComplementarityGap = complementarityGap_;
          } else if (numberIterations_ - lastGoodIteration >= numberBack &&
                     complementarityGap_ < 1.0e-3) {
               handler_->message(CLP_BARRIER_COMPLEMENTARITY, messages_)
                         << static_cast<double>(complementarityGap_) << "not decreasing"
                         << CoinMessageEol;
               if (gapO > 0.75 * lastGood && numberFixed < KEEP_GOING_IF_FIXED) {
                    break;
               }
          } else if (numberIterations_ - lastGoodIteration >= 2 &&
                     complementarityGap_ < 1.0e-6) {
               handler_->message(CLP_BARRIER_COMPLEMENTARITY, messages_)
                         << static_cast<double>(complementarityGap_) << "not decreasing"
                         << CoinMessageEol;
               break;
          }
          if (numberIterations_ > maximumBarrierIterations_ || hitMaximumIterations()) {
               handler_->message(CLP_BARRIER_STOPPING, messages_)
                         << CoinMessageEol;
               problemStatus_ = 3;
               onStopped(); // set secondary status
               break;
          }
          if (gapO < targetGap_) {
               problemStatus_ = 0;
               handler_->message(CLP_BARRIER_EXIT, messages_)
                         << " "
                         << CoinMessageEol;
               break;//finished
          }
          if (complementarityGap_ < 1.0e-12) {
               problemStatus_ = 0;
               handler_->message(CLP_BARRIER_EXIT, messages_)
                         << "- small complementarity gap"
                         << CoinMessageEol;
               break;//finished
          }
          if (complementarityGap_ < 1.0e-10 && gapO < 1.0e-10) {
               problemStatus_ = 0;
               handler_->message(CLP_BARRIER_EXIT, messages_)
                         << "- objective gap and complementarity gap both small"
                         << CoinMessageEol;
               break;//finished
          }
          if (gapO < 1.0e-9) {
               CoinWorkDouble value = gapO * complementarityGap_;
               value *= actualPrimalStep_;
               value *= actualDualStep_;
               //std::cout<<value<<std::endl;
               if (value < 1.0e-17 && numberIterations_ > lastGoodIteration) {
                    problemStatus_ = 0;
                    handler_->message(CLP_BARRIER_EXIT, messages_)
                              << "- objective gap and complementarity gap both smallish and small steps"
                              << CoinMessageEol;
                    break;//finished
               }
          }
          CoinWorkDouble nextGap = COIN_DBL_MAX;
          int nextNumber = 0;
          int nextNumberItems = 0;
          worstDirectionAccuracy_ = 0.0;
          int newDropped = 0;
          //Predictor step
          //prepare for cholesky.  Set up scaled diagonal in deltaX
          //  ** for efficiency may be better if scale factor known before
          CoinWorkDouble norm2 = 0.0;
          CoinWorkDouble maximumValue;
          getNorms(diagonal_, numberTotal, maximumValue, norm2);
          diagonalNorm_ = CoinSqrt(norm2 / numberComplementarityPairs_);
          diagonalScaleFactor_ = 1.0;
          CoinWorkDouble maximumAllowable = eScale;
          //scale so largest is less than allowable ? could do better
          CoinWorkDouble factor = 0.5;
          while (maximumValue > maximumAllowable) {
               diagonalScaleFactor_ *= factor;
               maximumValue *= factor;
          } /* endwhile */
          if (diagonalScaleFactor_ != 1.0) {
               handler_->message(CLP_BARRIER_SCALING, messages_)
                         << "diagonal" << static_cast<double>(diagonalScaleFactor_)
                         << CoinMessageEol;
               diagonalNorm_ *= diagonalScaleFactor_;
          }
          multiplyAdd(NULL, numberTotal, 0.0, diagonal_,
                      diagonalScaleFactor_);
          int * rowsDroppedThisTime = new int [numberRows_];
          newDropped = cholesky_->factorize(diagonal_, rowsDroppedThisTime);
          if (newDropped) {
               if (newDropped == -1) {
		 COIN_DETAIL_PRINT(printf("Out of memory\n"));
                    problemStatus_ = 4;
                    //delete all temporary regions
                    deleteWorkingData();
                    if (saveMatrix) {
                         // restore normal copy
                         delete matrix_;
                         matrix_ = saveMatrix;
                    }
                    return -1;
               } else {
#ifndef NDEBUG
                    //int newDropped2=cholesky_->factorize(diagonal_,rowsDroppedThisTime);
                    //assert(!newDropped2);
#endif
                    if (newDropped < 0 && 0) {
                         //replace dropped
                         newDropped = -newDropped;
                         //off 1 to allow for reset all
                         newDropped--;
                         //set all bits false
                         cholesky_->resetRowsDropped();
                    }
               }
          }
          delete [] rowsDroppedThisTime;
          if (cholesky_->status()) {
               std::cout << "bad cholesky?" << std::endl;
               abort();
          }
          int phase = 0; // predictor, corrector , primal dual
          CoinWorkDouble directionAccuracy = 0.0;
          bool doCorrector = true;
          bool goodMove = true;
          //set up for affine direction
          setupForSolve(phase);
          if ((modeSwitch & 2) == 0) {
               directionAccuracy = findDirectionVector(phase);
               if (directionAccuracy > worstDirectionAccuracy_) {
                    worstDirectionAccuracy_ = directionAccuracy;
               }
               if (saveIteration > 0 && directionAccuracy > 1.0) {
                    handler_->message(CLP_BARRIER_EXIT2, messages_)
                              << saveIteration
                              << CoinMessageEol;
                    break;
               }
               findStepLength(phase);
               nextGap = complementarityGap(nextNumber, nextNumberItems, 1);
               debugMove(0, actualPrimalStep_, actualDualStep_);
               debugMove(0, 1.0e-2, 1.0e-2);
          }
          CoinWorkDouble affineGap = nextGap;
          int bestPhase = 0;
          CoinWorkDouble bestNextGap = nextGap;
          // ?
          bestNextGap = CoinMax(nextGap, 0.8 * complementarityGap_);
          if (quadraticObj)
               bestNextGap = CoinMax(nextGap, 0.99 * complementarityGap_);
          if (complementarityGap_ > 1.0e-4 * numberComplementarityPairs_) {
               //std::cout <<"predicted duality gap "<<nextGap<<std::endl;
               CoinWorkDouble part1 = nextGap / numberComplementarityPairs_;
               part1 = nextGap / numberComplementarityItems_;
               CoinWorkDouble part2 = nextGap / complementarityGap_;
               mu_ = part1 * part2 * part2;
#if 0
               CoinWorkDouble papermu = complementarityGap_ / numberComplementarityPairs_;
               CoinWorkDouble affmu = nextGap / nextNumber;
               CoinWorkDouble sigma = pow(affmu / papermu, 3);
               printf("mu %g, papermu %g, affmu %g, sigma %g sigmamu %g\n",
                      mu_, papermu, affmu, sigma, sigma * papermu);
#endif
               //printf("paper mu %g\n",(nextGap*nextGap*nextGap)/(complementarityGap_*complementarityGap_*
               //					    (CoinWorkDouble) numberComplementarityPairs_));
          } else {
               CoinWorkDouble phi;
               if (numberComplementarityPairs_ <= 5000) {
                    phi = pow(static_cast<CoinWorkDouble> (numberComplementarityPairs_), 2.0);
               } else {
                    phi = pow(static_cast<CoinWorkDouble> (numberComplementarityPairs_), 1.5);
                    if (phi < 500.0 * 500.0) {
                         phi = 500.0 * 500.0;
                    }
               }
               mu_ = complementarityGap_ / phi;
          }
          //save information
          CoinWorkDouble product = affineProduct();
#if 0
          //can we do corrector step?
          CoinWorkDouble xx = complementarityGap_ * (beta2 - tau) + product;
          if (xx > 0.0) {
               CoinWorkDouble saveMu = mu_;
               CoinWorkDouble mu2 = numberComplementarityPairs_;
               mu2 = xx / mu2;
               if (mu2 > mu_) {
                    //std::cout<<" could increase to "<<mu2<<std::endl;
                    //was mu2=mu2*0.25;
                    mu2 = mu2 * 0.99;
                    if (mu2 < mu_) {
                         mu_ = mu2;
                         //std::cout<<" changing mu to "<<mu_<<std::endl;
                    } else {
                         //std::cout<<std::endl;
                    }
               } else {
                    //std::cout<<" should decrease to "<<mu2<<std::endl;
                    mu_ = 0.5 * mu2;
                    //std::cout<<" changing mu to "<<mu_<<std::endl;
               }
               handler_->message(CLP_BARRIER_MU, messages_)
                         << saveMu << mu_
                         << CoinMessageEol;
          } else {
               //std::cout<<" bad by any standards"<<std::endl;
          }
#endif
          if (complementarityGap_*(beta2 - tau) + product - mu_ * numberComplementarityPairs_ < 0.0 && 0) {
#ifdef SOME_DEBUG
               printf("failed 1 product %.18g mu %.18g - %.18g < 0.0, nextGap %.18g\n", product, mu_,
                      complementarityGap_*(beta2 - tau) + product - mu_ * numberComplementarityPairs_,
                      nextGap);
#endif
               doCorrector = false;
               if (nextGap > 0.9 * complementarityGap_ || 1) {
                    goodMove = false;
                    bestNextGap = COIN_DBL_MAX;
               }
               //CoinWorkDouble floatNumber = 2.0*numberComplementarityPairs_;
               //floatNumber = 1.0*numberComplementarityItems_;
               //mu_=nextGap/floatNumber;
               handler_->message(CLP_BARRIER_INFO, messages_)
                         << "no corrector step"
                         << CoinMessageEol;
          } else {
               phase = 1;
          }
          // If bad gap - try standard primal dual
          if (nextGap > complementarityGap_ * 1.001)
               goodMove = false;
          if ((modeSwitch & 2) != 0)
               goodMove = false;
          if (goodMove && doCorrector) {
               CoinMemcpyN(deltaX_, numberTotal, saveX);
               CoinMemcpyN(deltaY_, numberRows_, saveY);
               CoinMemcpyN(deltaZ_, numberTotal, saveZ);
               CoinMemcpyN(deltaW_, numberTotal, saveW);
               CoinMemcpyN(deltaSL_, numberTotal, saveSL);
               CoinMemcpyN(deltaSU_, numberTotal, saveSU);
#ifdef HALVE
               CoinWorkDouble savePrimalStep = actualPrimalStep_;
               CoinWorkDouble saveDualStep = actualDualStep_;
               CoinWorkDouble saveMu = mu_;
#endif
               //set up for next step
               setupForSolve(phase);
               CoinWorkDouble directionAccuracy2 = findDirectionVector(phase);
               if (directionAccuracy2 > worstDirectionAccuracy_) {
                    worstDirectionAccuracy_ = directionAccuracy2;
               }
               CoinWorkDouble testValue = 1.0e2 * directionAccuracy;
               if (1.0e2 * projectionTolerance_ > testValue) {
                    testValue = 1.0e2 * projectionTolerance_;
               }
               if (primalTolerance() > testValue) {
                    testValue = primalTolerance();
               }
               if (maximumRHSError_ > testValue) {
                    testValue = maximumRHSError_;
               }
               if (directionAccuracy2 > testValue && numberIterations_ >= -77) {
                    goodMove = false;
#ifdef SOME_DEBUG
                    printf("accuracy %g phase 1 failed, test value %g\n",
                           directionAccuracy2, testValue);
#endif
               }
               if (goodMove) {
                    phase = 1;
                    CoinWorkDouble norm = findStepLength(phase);
                    nextGap = complementarityGap(nextNumber, nextNumberItems, 1);
                    debugMove(1, actualPrimalStep_, actualDualStep_);
                    //debugMove(1,1.0e-7,1.0e-7);
                    goodMove = checkGoodMove(true, bestNextGap, allowIncreasingGap);
                    if (norm < 0)
                         goodMove = false;
                    if (!goodMove) {
#ifdef SOME_DEBUG
                         printf("checkGoodMove failed\n");
#endif
                    }
               }
#ifdef HALVE
               int nHalve = 0;
               // relax test
               bestNextGap = CoinMax(bestNextGap, 0.9 * complementarityGap_);
               while (!goodMove) {
                    mu_ = saveMu;
                    actualPrimalStep_ = savePrimalStep;
                    actualDualStep_ = saveDualStep;
                    int i;
                    //printf("halve %d\n",nHalve);
                    nHalve++;
                    const CoinWorkDouble lambda = 0.5;
                    for (i = 0; i < numberRows_; i++)
                         deltaY_[i] = lambda * deltaY_[i] + (1.0 - lambda) * saveY[i];
                    for (i = 0; i < numberTotal; i++) {
                         deltaX_[i] = lambda * deltaX_[i] + (1.0 - lambda) * saveX[i];
                         deltaZ_[i] = lambda * deltaZ_[i] + (1.0 - lambda) * saveZ[i];
                         deltaW_[i] = lambda * deltaW_[i] + (1.0 - lambda) * saveW[i];
                         deltaSL_[i] = lambda * deltaSL_[i] + (1.0 - lambda) * saveSL[i];
                         deltaSU_[i] = lambda * deltaSU_[i] + (1.0 - lambda) * saveSU[i];
                    }
                    CoinMemcpyN(saveX, numberTotal, deltaX_);
                    CoinMemcpyN(saveY, numberRows_, deltaY_);
                    CoinMemcpyN(saveZ, numberTotal, deltaZ_);
                    CoinMemcpyN(saveW, numberTotal, deltaW_);
                    CoinMemcpyN(saveSL, numberTotal, deltaSL_);
                    CoinMemcpyN(saveSU, numberTotal, deltaSU_);
                    findStepLength(1);
                    nextGap = complementarityGap(nextNumber, nextNumberItems, 1);
                    goodMove = checkGoodMove(true, bestNextGap, allowIncreasingGap);
                    if (nHalve > 10)
                         break;
                    //assert (goodMove);
               }
               if (nHalve && handler_->logLevel() > 2)
                    printf("halved %d times\n", nHalve);
#endif
          }
          //bestPhase=-1;
          //goodMove=false;
          if (!goodMove) {
               // Just primal dual step
               CoinWorkDouble floatNumber;
               floatNumber = 2.0 * numberComplementarityPairs_;
               //floatNumber = numberComplementarityItems_;
               CoinWorkDouble saveMu = mu_; // use one from predictor corrector
               mu_ = complementarityGap_ / floatNumber;
               // If going well try small mu
               mu_ *= CoinSqrt((1.0 - lastStep) / (1.0 + 10.0 * lastStep));
               CoinWorkDouble mu1 = mu_;
               CoinWorkDouble phi;
               if (numberComplementarityPairs_ <= 500) {
                    phi = pow(static_cast<CoinWorkDouble> (numberComplementarityPairs_), 2.0);
               } else {
                    phi = pow(static_cast<CoinWorkDouble> (numberComplementarityPairs_), 1.5);
                    if (phi < 500.0 * 500.0) {
                         phi = 500.0 * 500.0;
                    }
               }
               mu_ = complementarityGap_ / phi;
               //printf("pd mu %g, alternate %g, smallest\n",mu_,mu1);
               mu_ = CoinSqrt(mu_ * mu1);
               mu_ = mu1;
               if ((numberIterations_ & 1) == 0 || numberIterations_ < 10)
                    mu_ = saveMu;
               // Try simpler
               floatNumber = numberComplementarityItems_;
               mu_ = 0.5 * complementarityGap_ / floatNumber;
               //if ((modeSwitch&2)==0) {
               //if ((numberIterations_&1)==0)
               //  mu_ *= 0.5;
               //} else {
               //mu_ *= 0.8;
               //}
               //set up for next step
               setupForSolve(2);
               findDirectionVector(2);
               CoinWorkDouble norm = findStepLength(2);
               // just for debug
               bestNextGap = complementarityGap_ * 1.0005;
               //bestNextGap=COIN_DBL_MAX;
               nextGap = complementarityGap(nextNumber, nextNumberItems, 2);
               debugMove(2, actualPrimalStep_, actualDualStep_);
               //debugMove(2,1.0e-7,1.0e-7);
               checkGoodMove(false, bestNextGap, allowIncreasingGap);
               if ((nextGap > 0.9 * complementarityGap_ && bestPhase == 0 && affineGap < nextGap
                         && (numberIterations_ > 80 || (numberIterations_ > 20 && quadraticObj))) || norm < 0.0) {
                    // Back to affine
                    phase = 0;
                    setupForSolve(phase);
                    directionAccuracy = findDirectionVector(phase);
                    findStepLength(phase);
                    nextGap = complementarityGap(nextNumber, nextNumberItems, 1);
                    bestNextGap = complementarityGap_;
                    //checkGoodMove(false, bestNextGap,allowIncreasingGap);
               }
          }
          if (!goodMove)
               mu_ = nextGap / (static_cast<CoinWorkDouble> (nextNumber) * 1.1);
          //if (quadraticObj)
          //goodMove=true;
          //goodMove=false; //TEMP
          // Do centering steps
          int numberTries = 0;
          int numberGoodTries = 0;
#ifdef COIN_DETAIL
          CoinWorkDouble nextCenterGap = 0.0;
          CoinWorkDouble originalDualStep = actualDualStep_;
          CoinWorkDouble originalPrimalStep = actualPrimalStep_;
#endif
          if (actualDualStep_ > 0.9 && actualPrimalStep_ > 0.9)
               goodMove = false; // don't bother
          if ((modeSwitch & 1) != 0)
               goodMove = false;
          while (goodMove && numberTries < 5) {
               goodMove = false;
               numberTries++;
               CoinMemcpyN(deltaX_, numberTotal, saveX);
               CoinMemcpyN(deltaY_, numberRows_, saveY);
               CoinMemcpyN(deltaZ_, numberTotal, saveZ);
               CoinMemcpyN(deltaW_, numberTotal, saveW);
               CoinWorkDouble savePrimalStep = actualPrimalStep_;
               CoinWorkDouble saveDualStep = actualDualStep_;
               CoinWorkDouble saveMu = mu_;
               setupForSolve(3);
               findDirectionVector(3);
               findStepLength(3);
               debugMove(3, actualPrimalStep_, actualDualStep_);
               //debugMove(3,1.0e-7,1.0e-7);
               CoinWorkDouble xGap = complementarityGap(nextNumber, nextNumberItems, 3);
               // If one small then that's the one that counts
               CoinWorkDouble checkDual = saveDualStep;
               CoinWorkDouble checkPrimal = savePrimalStep;
               if (checkDual > 5.0 * checkPrimal) {
                    checkDual = 2.0 * checkPrimal;
               } else if (checkPrimal > 5.0 * checkDual) {
                    checkPrimal = 2.0 * checkDual;
               }
               if (actualPrimalStep_ < checkPrimal ||
                         actualDualStep_ < checkDual ||
                         (xGap > nextGap && xGap > 0.9 * complementarityGap_)) {
                    //if (actualPrimalStep_<=checkPrimal||
                    //actualDualStep_<=checkDual) {
#ifdef SOME_DEBUG
                    printf("PP rejected gap %.18g, steps %.18g %.18g, 2 gap %.18g, steps %.18g %.18g\n", xGap,
                           actualPrimalStep_, actualDualStep_, nextGap, savePrimalStep, saveDualStep);
#endif
                    mu_ = saveMu;
                    actualPrimalStep_ = savePrimalStep;
                    actualDualStep_ = saveDualStep;
                    CoinMemcpyN(saveX, numberTotal, deltaX_);
                    CoinMemcpyN(saveY, numberRows_, deltaY_);
                    CoinMemcpyN(saveZ, numberTotal, deltaZ_);
                    CoinMemcpyN(saveW, numberTotal, deltaW_);
               } else {
#ifdef SOME_DEBUG
                    printf("PPphase 3 gap %.18g, steps %.18g %.18g, 2 gap %.18g, steps %.18g %.18g\n", xGap,
                           actualPrimalStep_, actualDualStep_, nextGap, savePrimalStep, saveDualStep);
#endif
                    numberGoodTries++;
#ifdef COIN_DETAIL
                    nextCenterGap = xGap;
#endif
                    // See if big enough change
                    if (actualPrimalStep_ < 1.01 * checkPrimal ||
                              actualDualStep_ < 1.01 * checkDual) {
                         // stop now
                    } else {
                         // carry on
                         goodMove = true;
                    }
               }
          }
          if (numberGoodTries && handler_->logLevel() > 1) {
               COIN_DETAIL_PRINT(printf("%d centering steps moved from (gap %.18g, dual %.18g, primal %.18g) to (gap %.18g, dual %.18g, primal %.18g)\n",
                      numberGoodTries, static_cast<double>(nextGap), static_cast<double>(originalDualStep),
                      static_cast<double>(originalPrimalStep),
                      static_cast<double>(nextCenterGap), static_cast<double>(actualDualStep_),
					static_cast<double>(actualPrimalStep_)));
          }
          // save last gap
          checkGap = complementarityGap_;
          numberFixed = updateSolution(nextGap);
          numberFixedTotal += numberFixed;
     } /* endwhile */
     delete [] saveX;
     delete [] saveY;
     delete [] saveZ;
     delete [] saveW;
     delete [] saveSL;
     delete [] saveSU;
     if (savePi) {
          if (numberIterations_ - saveIteration > 20 &&
                    numberIterations_ - saveIteration2 < 5) {
#if KEEP_GOING_IF_FIXED<10
               std::cout << "Restoring2 from iteration " << saveIteration2 << std::endl;
#endif
               CoinMemcpyN(savePi2, numberRows_, dualArray);
               CoinMemcpyN(savePrimal2, numberTotal, solution_);
          } else {
#if KEEP_GOING_IF_FIXED<10
               std::cout << "Restoring from iteration " << saveIteration << std::endl;
#endif
               CoinMemcpyN(savePi, numberRows_, dualArray);
               CoinMemcpyN(savePrimal, numberTotal, solution_);
          }
          delete [] savePi;
          delete [] savePrimal;
     }
     delete [] savePi2;
     delete [] savePrimal2;
     //recompute slacks
     // Split out solution
     CoinZeroN(rowActivity_, numberRows_);
     CoinMemcpyN(solution_, numberColumns_, columnActivity_);
     matrix_->times(1.0, columnActivity_, rowActivity_);
     //unscale objective
     multiplyAdd(NULL, numberTotal, 0.0, cost_, scaleFactor_);
     multiplyAdd(NULL, numberRows_, 0, dualArray, scaleFactor_);
     checkSolution();
     //CoinMemcpyN(reducedCost_,numberColumns_,dj_);
     // If quadratic use last solution
     // Restore quadratic objective if necessary
     if (saveObjective) {
          delete objective_;
          objective_ = saveObjective;
          objectiveValue_ = 0.5 * (primalObjective_ + dualObjective_);
     }
     handler_->message(CLP_BARRIER_END, messages_)
               << static_cast<double>(sumPrimalInfeasibilities_)
               << static_cast<double>(sumDualInfeasibilities_)
               << static_cast<double>(complementarityGap_)
               << static_cast<double>(objectiveValue())
               << CoinMessageEol;
     //#ifdef SOME_DEBUG
     if (handler_->logLevel() > 1)
       COIN_DETAIL_PRINT(printf("ENDRUN status %d after %d iterations\n", problemStatus_, numberIterations_));
     //#endif
     //std::cout<<"Absolute primal infeasibility at end "<<sumPrimalInfeasibilities_<<std::endl;
     //std::cout<<"Absolute dual infeasibility at end "<<sumDualInfeasibilities_<<std::endl;
     //std::cout<<"Absolute complementarity at end "<<complementarityGap_<<std::endl;
     //std::cout<<"Primal objective "<<objectiveValue()<<std::endl;
     //std::cout<<"maximum complementarity "<<worstComplementarity_<<std::endl;
#if COIN_LONG_WORK
     // put back dual
     delete [] dual_;
     delete [] reducedCost_;
     dual_ = dualSave;
     reducedCost_ = reducedCostSave;
#endif
     //delete all temporary regions
     deleteWorkingData();
#if KEEP_GOING_IF_FIXED<10
#if 0 //ndef NDEBUG
     {
          static int kk = 0;
          char name[20];
          sprintf(name, "save.sol.%d", kk);
          kk++;
          printf("saving to file %s\n", name);
          FILE * fp = fopen(name, "wb");
          int numberWritten;
          numberWritten = fwrite(&numberColumns_, sizeof(int), 1, fp);
          assert (numberWritten == 1);
          numberWritten = fwrite(columnActivity_, sizeof(double), numberColumns_, fp);
          assert (numberWritten == numberColumns_);
          fclose(fp);
     }
#endif
#endif
     if (saveMatrix) {
          // restore normal copy
          delete matrix_;
          matrix_ = saveMatrix;
     }
     return problemStatus_;
}
// findStepLength.
//phase  - 0 predictor
//         1 corrector
//         2 primal dual
CoinWorkDouble ClpPredictorCorrector::findStepLength( int phase)
{
     CoinWorkDouble directionNorm = 0.0;
     CoinWorkDouble maximumPrimalStep = COIN_DBL_MAX * 1.0e-20;
     CoinWorkDouble maximumDualStep = COIN_DBL_MAX;
     int numberTotal = numberRows_ + numberColumns_;
     CoinWorkDouble tolerance = 1.0e-12;
#ifdef SOME_DEBUG
     int chosenPrimalSequence = -1;
     int chosenDualSequence = -1;
     bool lowPrimal = false;
     bool lowDual = false;
#endif
     // If done many iterations then allow to hit boundary
     CoinWorkDouble hitTolerance;
     //printf("objective norm %g\n",objectiveNorm_);
     if (numberIterations_ < 80 || !gonePrimalFeasible_)
          hitTolerance = COIN_DBL_MAX;
     else
          hitTolerance = CoinMax(1.0e3, 1.0e-3 * objectiveNorm_);
     int iColumn;
     //printf("dual value %g\n",dual_[0]);
     //printf("     X     dX      lS     dlS     uS     dUs    dj    Z dZ     t   dT\n");
     for (iColumn = 0; iColumn < numberTotal; iColumn++) {
          if (!flagged(iColumn)) {
               CoinWorkDouble directionElement = deltaX_[iColumn];
               if (directionNorm < CoinAbs(directionElement)) {
                    directionNorm = CoinAbs(directionElement);
               }
               if (lowerBound(iColumn)) {
                    CoinWorkDouble delta = - deltaSL_[iColumn];
                    CoinWorkDouble z1 = deltaZ_[iColumn];
                    CoinWorkDouble newZ = zVec_[iColumn] + z1;
                    if (zVec_[iColumn] > tolerance) {
                         if (zVec_[iColumn] < -z1 * maximumDualStep) {
                              maximumDualStep = -zVec_[iColumn] / z1;
#ifdef SOME_DEBUG
                              chosenDualSequence = iColumn;
                              lowDual = true;
#endif
                         }
                    }
                    if (lowerSlack_[iColumn] < maximumPrimalStep * delta) {
                         CoinWorkDouble newStep = lowerSlack_[iColumn] / delta;
                         if (newStep > 0.2 || newZ < hitTolerance || delta > 1.0e3 || delta <= 1.0e-6 || dj_[iColumn] < hitTolerance) {
                              maximumPrimalStep = newStep;
#ifdef SOME_DEBUG
                              chosenPrimalSequence = iColumn;
                              lowPrimal = true;
#endif
                         } else {
                              //printf("small %d delta %g newZ %g step %g\n",iColumn,delta,newZ,newStep);
                         }
                    }
               }
               if (upperBound(iColumn)) {
                    CoinWorkDouble delta = - deltaSU_[iColumn];;
                    CoinWorkDouble w1 = deltaW_[iColumn];
                    CoinWorkDouble newT = wVec_[iColumn] + w1;
                    if (wVec_[iColumn] > tolerance) {
                         if (wVec_[iColumn] < -w1 * maximumDualStep) {
                              maximumDualStep = -wVec_[iColumn] / w1;
#ifdef SOME_DEBUG
                              chosenDualSequence = iColumn;
                              lowDual = false;
#endif
                         }
                    }
                    if (upperSlack_[iColumn] < maximumPrimalStep * delta) {
                         CoinWorkDouble newStep = upperSlack_[iColumn] / delta;
                         if (newStep > 0.2 || newT < hitTolerance || delta > 1.0e3 || delta <= 1.0e-6 || dj_[iColumn] > -hitTolerance) {
                              maximumPrimalStep = newStep;
#ifdef SOME_DEBUG
                              chosenPrimalSequence = iColumn;
                              lowPrimal = false;
#endif
                         } else {
                              //printf("small %d delta %g newT %g step %g\n",iColumn,delta,newT,newStep);
                         }
                    }
               }
          }
     }
#ifdef SOME_DEBUG
     printf("new step - phase %d, norm %.18g, dual step %.18g, primal step %.18g\n",
            phase, directionNorm, maximumDualStep, maximumPrimalStep);
     if (lowDual)
          printf("ld %d %g %g => %g (dj %g,sol %g) ",
                 chosenDualSequence, zVec_[chosenDualSequence],
                 deltaZ_[chosenDualSequence], zVec_[chosenDualSequence] +
                 maximumDualStep * deltaZ_[chosenDualSequence], dj_[chosenDualSequence],
                 solution_[chosenDualSequence]);
     else
          printf("ud %d %g %g => %g (dj %g,sol %g) ",
                 chosenDualSequence, wVec_[chosenDualSequence],
                 deltaW_[chosenDualSequence], wVec_[chosenDualSequence] +
                 maximumDualStep * deltaW_[chosenDualSequence], dj_[chosenDualSequence],
                 solution_[chosenDualSequence]);
     if (lowPrimal)
          printf("lp %d %g %g => %g (dj %g,sol %g)\n",
                 chosenPrimalSequence, lowerSlack_[chosenPrimalSequence],
                 deltaSL_[chosenPrimalSequence], lowerSlack_[chosenPrimalSequence] +
                 maximumPrimalStep * deltaSL_[chosenPrimalSequence],
                 dj_[chosenPrimalSequence], solution_[chosenPrimalSequence]);
     else
          printf("up %d %g %g => %g (dj %g,sol %g)\n",
                 chosenPrimalSequence, upperSlack_[chosenPrimalSequence],
                 deltaSU_[chosenPrimalSequence], upperSlack_[chosenPrimalSequence] +
                 maximumPrimalStep * deltaSU_[chosenPrimalSequence],
                 dj_[chosenPrimalSequence], solution_[chosenPrimalSequence]);
#endif
     actualPrimalStep_ = stepLength_ * maximumPrimalStep;
     if (phase >= 0 && actualPrimalStep_ > 1.0) {
          actualPrimalStep_ = 1.0;
     }
     actualDualStep_ = stepLength_ * maximumDualStep;
     if (phase >= 0 && actualDualStep_ > 1.0) {
          actualDualStep_ = 1.0;
     }
     // See if quadratic objective
#ifndef NO_RTTI
     ClpQuadraticObjective * quadraticObj = (dynamic_cast< ClpQuadraticObjective*>(objective_));
#else
     ClpQuadraticObjective * quadraticObj = NULL;
     if (objective_->type() == 2)
          quadraticObj = (static_cast< ClpQuadraticObjective*>(objective_));
#endif
     if (quadraticObj) {
          // Use smaller unless very small
          CoinWorkDouble smallerStep = CoinMin(actualDualStep_, actualPrimalStep_);
          if (smallerStep > 0.0001) {
               actualDualStep_ = smallerStep;
               actualPrimalStep_ = smallerStep;
          }
     }
#define OFFQ
#ifndef OFFQ
     if (quadraticObj) {
          // Don't bother if phase 0 or 3 or large gap
          //if ((phase==1||phase==2||phase==0)&&maximumDualError_>0.1*complementarityGap_
          //&&smallerStep>0.001) {
          if ((phase == 1 || phase == 2 || phase == 0 || phase == 3)) {
               // minimize complementarity + norm*dual inf ? primal inf
               // at first - just check better - if not
               // Complementarity gap will be a*change*change + b*change +c
               CoinWorkDouble a = 0.0;
               CoinWorkDouble b = 0.0;
               CoinWorkDouble c = 0.0;
               /* SQUARE of dual infeasibility will be:
               square of dj - ......
               */
               CoinWorkDouble aq = 0.0;
               CoinWorkDouble bq = 0.0;
               CoinWorkDouble cq = 0.0;
               CoinWorkDouble gamma2 = gamma_ * gamma_; // gamma*gamma will be added to diagonal
               CoinWorkDouble * linearDjChange = new CoinWorkDouble[numberTotal];
               CoinZeroN(linearDjChange, numberColumns_);
               multiplyAdd(deltaY_, numberRows_, 1.0, linearDjChange + numberColumns_, 0.0);
               matrix_->transposeTimes(-1.0, deltaY_, linearDjChange);
               CoinPackedMatrix * quadratic = quadraticObj->quadraticObjective();
               const int * columnQuadratic = quadratic->getIndices();
               const CoinBigIndex * columnQuadraticStart = quadratic->getVectorStarts();
               const int * columnQuadraticLength = quadratic->getVectorLengths();
               CoinWorkDouble * quadraticElement = quadratic->getMutableElements();
               for (iColumn = 0; iColumn < numberTotal; iColumn++) {
                    CoinWorkDouble oldPrimal = solution_[iColumn];
                    if (!flagged(iColumn)) {
                         if (lowerBound(iColumn)) {
                              CoinWorkDouble change = oldPrimal + deltaX_[iColumn] - lowerSlack_[iColumn] - lower_[iColumn];
                              c += lowerSlack_[iColumn] * zVec_[iColumn];
                              b += lowerSlack_[iColumn] * deltaZ_[iColumn] + zVec_[iColumn] * change;
                              a += deltaZ_[iColumn] * change;
                         }
                         if (upperBound(iColumn)) {
                              CoinWorkDouble change = upper_[iColumn] - oldPrimal - deltaX_[iColumn] - upperSlack_[iColumn];
                              c += upperSlack_[iColumn] * wVec_[iColumn];
                              b += upperSlack_[iColumn] * deltaW_[iColumn] + wVec_[iColumn] * change;
                              a += deltaW_[iColumn] * change;
                         }
                         // new djs are dj_ + change*value
                         CoinWorkDouble djChange = linearDjChange[iColumn];
                         if (iColumn < numberColumns_) {
                              for (CoinBigIndex j = columnQuadraticStart[iColumn];
                                        j < columnQuadraticStart[iColumn] + columnQuadraticLength[iColumn]; j++) {
                                   int jColumn = columnQuadratic[j];
                                   CoinWorkDouble changeJ = deltaX_[jColumn];
                                   CoinWorkDouble elementValue = quadraticElement[j];
                                   djChange += changeJ * elementValue;
                              }
                         }
                         CoinWorkDouble gammaTerm = gamma2;
                         if (primalR_) {
                              gammaTerm += primalR_[iColumn];
                         }
                         djChange += gammaTerm;
                         // and dual infeasibility
                         CoinWorkDouble oldInf = dj_[iColumn] - zVec_[iColumn] + wVec_[iColumn] +
                                                 gammaTerm * solution_[iColumn];
                         CoinWorkDouble changeInf = djChange - deltaZ_[iColumn] + deltaW_[iColumn];
                         cq += oldInf * oldInf;
                         bq += 2.0 * oldInf * changeInf;
                         aq += changeInf * changeInf;
                    } else {
                         // fixed
                         if (lowerBound(iColumn)) {
                              c += lowerSlack_[iColumn] * zVec_[iColumn];
                         }
                         if (upperBound(iColumn)) {
                              c += upperSlack_[iColumn] * wVec_[iColumn];
                         }
                         // new djs are dj_ + change*value
                         CoinWorkDouble djChange = linearDjChange[iColumn];
                         if (iColumn < numberColumns_) {
                              for (CoinBigIndex j = columnQuadraticStart[iColumn];
                                        j < columnQuadraticStart[iColumn] + columnQuadraticLength[iColumn]; j++) {
                                   int jColumn = columnQuadratic[j];
                                   CoinWorkDouble changeJ = deltaX_[jColumn];
                                   CoinWorkDouble elementValue = quadraticElement[j];
                                   djChange += changeJ * elementValue;
                              }
                         }
                         CoinWorkDouble gammaTerm = gamma2;
                         if (primalR_) {
                              gammaTerm += primalR_[iColumn];
                         }
                         djChange += gammaTerm;
                         // and dual infeasibility
                         CoinWorkDouble oldInf = dj_[iColumn] - zVec_[iColumn] + wVec_[iColumn] +
                                                 gammaTerm * solution_[iColumn];
                         CoinWorkDouble changeInf = djChange - deltaZ_[iColumn] + deltaW_[iColumn];
                         cq += oldInf * oldInf;
                         bq += 2.0 * oldInf * changeInf;
                         aq += changeInf * changeInf;
                    }
               }
               delete [] linearDjChange;
               // ? We want to minimize complementarityGap + solutionNorm_*square of inf ??
               // maybe use inf and do line search
               // To check see if matches at current step
               CoinWorkDouble step = actualPrimalStep_;
               //Current gap + solutionNorm_ * CoinSqrt (sum square inf)
               CoinWorkDouble multiplier = solutionNorm_;
               multiplier *= 0.01;
               multiplier = 1.0;
               CoinWorkDouble currentInf =  multiplier * CoinSqrt(cq);
               CoinWorkDouble nextInf = 	multiplier * CoinSqrt(CoinMax(cq + step * bq + step * step * aq, 0.0));
               CoinWorkDouble allowedIncrease = 1.4;
#ifdef SOME_DEBUG
               printf("lin %g %g %g -> %g\n", a, b, c,
                      c + b * step + a * step * step);
               printf("quad %g %g %g -> %g\n", aq, bq, cq,
                      cq + bq * step + aq * step * step);
               debugMove(7, step, step);
               printf ("current dualInf %g, with step of %g is %g\n",
                       currentInf, step, nextInf);
#endif
               if (b > -1.0e-6) {
                    if (phase != 0)
                         directionNorm = -1.0;
               }
               if ((phase == 1 || phase == 2 || phase == 0 || phase == 3) && nextInf > 0.1 * complementarityGap_ &&
                         nextInf > currentInf * allowedIncrease) {
                    //cq = CoinMax(cq,10.0);
                    // convert to (x+q)*(x+q) = w
                    CoinWorkDouble q = bq / (1.0 * aq);
                    CoinWorkDouble w = CoinMax(q * q + (cq / aq) * (allowedIncrease - 1.0), 0.0);
                    w = CoinSqrt(w);
                    CoinWorkDouble stepX = w - q;
                    step = stepX;
                    nextInf =
                         multiplier * CoinSqrt(CoinMax(cq + step * bq + step * step * aq, 0.0));
#ifdef SOME_DEBUG
                    printf ("with step of %g dualInf is %g\n",
                            step, nextInf);
#endif
                    actualDualStep_ = CoinMin(step, actualDualStep_);
                    actualPrimalStep_ = CoinMin(step, actualPrimalStep_);
               }
          }
     } else {
          // probably pointless as linear
          // minimize complementarity
          // Complementarity gap will be a*change*change + b*change +c
          CoinWorkDouble a = 0.0;
          CoinWorkDouble b = 0.0;
          CoinWorkDouble c = 0.0;
          for (iColumn = 0; iColumn < numberTotal; iColumn++) {
               CoinWorkDouble oldPrimal = solution_[iColumn];
               if (!flagged(iColumn)) {
                    if (lowerBound(iColumn)) {
                         CoinWorkDouble change = oldPrimal + deltaX_[iColumn] - lowerSlack_[iColumn] - lower_[iColumn];
                         c += lowerSlack_[iColumn] * zVec_[iColumn];
                         b += lowerSlack_[iColumn] * deltaZ_[iColumn] + zVec_[iColumn] * change;
                         a += deltaZ_[iColumn] * change;
                    }
                    if (upperBound(iColumn)) {
                         CoinWorkDouble change = upper_[iColumn] - oldPrimal - deltaX_[iColumn] - upperSlack_[iColumn];
                         c += upperSlack_[iColumn] * wVec_[iColumn];
                         b += upperSlack_[iColumn] * deltaW_[iColumn] + wVec_[iColumn] * change;
                         a += deltaW_[iColumn] * change;
                    }
               } else {
                    // fixed
                    if (lowerBound(iColumn)) {
                         c += lowerSlack_[iColumn] * zVec_[iColumn];
                    }
                    if (upperBound(iColumn)) {
                         c += upperSlack_[iColumn] * wVec_[iColumn];
                    }
               }
          }
          // ? We want to minimize complementarityGap;
          // maybe use inf and do line search
          // To check see if matches at current step
          CoinWorkDouble step = CoinMin(actualPrimalStep_, actualDualStep_);
          CoinWorkDouble next = c + b * step + a * step * step;
#ifdef SOME_DEBUG
          printf("lin %g %g %g -> %g\n", a, b, c,
                 c + b * step + a * step * step);
          debugMove(7, step, step);
#endif
          if (b > -1.0e-6) {
               if (phase == 0) {
#ifdef SOME_DEBUG
                    printf("*** odd phase 0 direction\n");
#endif
               } else {
                    directionNorm = -1.0;
               }
          }
          // and with ratio
          a = 0.0;
          b = 0.0;
          CoinWorkDouble ratio = actualDualStep_ / actualPrimalStep_;
          for (iColumn = 0; iColumn < numberTotal; iColumn++) {
               CoinWorkDouble oldPrimal = solution_[iColumn];
               if (!flagged(iColumn)) {
                    if (lowerBound(iColumn)) {
                         CoinWorkDouble change = oldPrimal + deltaX_[iColumn] - lowerSlack_[iColumn] - lower_[iColumn];
                         b += lowerSlack_[iColumn] * deltaZ_[iColumn] * ratio + zVec_[iColumn] * change;
                         a += deltaZ_[iColumn] * change * ratio;
                    }
                    if (upperBound(iColumn)) {
                         CoinWorkDouble change = upper_[iColumn] - oldPrimal - deltaX_[iColumn] - upperSlack_[iColumn];
                         b += upperSlack_[iColumn] * deltaW_[iColumn] * ratio + wVec_[iColumn] * change;
                         a += deltaW_[iColumn] * change * ratio;
                    }
               }
          }
          // ? We want to minimize complementarityGap;
          // maybe use inf and do line search
          // To check see if matches at current step
          step = actualPrimalStep_;
          CoinWorkDouble next2 = c + b * step + a * step * step;
          if (next2 > next) {
               actualPrimalStep_ = CoinMin(actualPrimalStep_, actualDualStep_);
               actualDualStep_ = actualPrimalStep_;
          }
#ifdef SOME_DEBUG
          printf("linb %g %g %g -> %g\n", a, b, c,
                 c + b * step + a * step * step);
          debugMove(7, actualPrimalStep_, actualDualStep_);
#endif
          if (b > -1.0e-6) {
               if (phase == 0) {
#ifdef SOME_DEBUG
                    printf("*** odd phase 0 direction\n");
#endif
               } else {
                    directionNorm = -1.0;
               }
          }
     }
#else
     //actualPrimalStep_ =0.5*actualDualStep_;
#endif
#ifdef FULL_DEBUG
     if (phase == 3) {
          CoinWorkDouble minBeta = 0.1 * mu_;
          CoinWorkDouble maxBeta = 10.0 * mu_;
          for (iColumn = 0; iColumn < numberRows_ + numberColumns_; iColumn++) {
               if (!flagged(iColumn)) {
                    if (lowerBound(iColumn)) {
                         CoinWorkDouble change = -rhsL_[iColumn] + deltaX_[iColumn];
                         CoinWorkDouble dualValue = zVec_[iColumn] + actualDualStep_ * deltaZ_[iColumn];
                         CoinWorkDouble primalValue = lowerSlack_[iColumn] + actualPrimalStep_ * change;
                         CoinWorkDouble gapProduct = dualValue * primalValue;
                         if (delta2Z_[iColumn] < minBeta || delta2Z_[iColumn] > maxBeta)
                              printf("3lower %d primal %g, dual %g, gap %g, old gap %g\n",
                                     iColumn, primalValue, dualValue, gapProduct, delta2Z_[iColumn]);
                    }
                    if (upperBound(iColumn)) {
                         CoinWorkDouble change = rhsU_[iColumn] - deltaX_[iColumn];
                         CoinWorkDouble dualValue = wVec_[iColumn] + actualDualStep_ * deltaW_[iColumn];
                         CoinWorkDouble primalValue = upperSlack_[iColumn] + actualPrimalStep_ * change;
                         CoinWorkDouble gapProduct = dualValue * primalValue;
                         if (delta2W_[iColumn] < minBeta || delta2W_[iColumn] > maxBeta)
                              printf("3upper %d primal %g, dual %g, gap %g, old gap %g\n",
                                     iColumn, primalValue, dualValue, gapProduct, delta2W_[iColumn]);
                    }
               }
          }
     }
#endif
#ifdef SOME_DEBUG_not
     {
          CoinWorkDouble largestL = 0.0;
          CoinWorkDouble smallestL = COIN_DBL_MAX;
          CoinWorkDouble largestU = 0.0;
          CoinWorkDouble smallestU = COIN_DBL_MAX;
          CoinWorkDouble sumL = 0.0;
          CoinWorkDouble sumU = 0.0;
          int nL = 0;
          int nU = 0;
          for (iColumn = 0; iColumn < numberRows_ + numberColumns_; iColumn++) {
               if (!flagged(iColumn)) {
                    if (lowerBound(iColumn)) {
                         CoinWorkDouble change = -rhsL_[iColumn] + deltaX_[iColumn];
                         CoinWorkDouble dualValue = zVec_[iColumn] + actualDualStep_ * deltaZ_[iColumn];
                         CoinWorkDouble primalValue = lowerSlack_[iColumn] + actualPrimalStep_ * change;
                         CoinWorkDouble gapProduct = dualValue * primalValue;
                         largestL = CoinMax(largestL, gapProduct);
                         smallestL = CoinMin(smallestL, gapProduct);
                         nL++;
                         sumL += gapProduct;
                    }
                    if (upperBound(iColumn)) {
                         CoinWorkDouble change = rhsU_[iColumn] - deltaX_[iColumn];
                         CoinWorkDouble dualValue = wVec_[iColumn] + actualDualStep_ * deltaW_[iColumn];
                         CoinWorkDouble primalValue = upperSlack_[iColumn] + actualPrimalStep_ * change;
                         CoinWorkDouble gapProduct = dualValue * primalValue;
                         largestU = CoinMax(largestU, gapProduct);
                         smallestU = CoinMin(smallestU, gapProduct);
                         nU++;
                         sumU += gapProduct;
                    }
               }
          }
          CoinWorkDouble mu = (sumL + sumU) / (static_cast<CoinWorkDouble> (nL + nU));

          CoinWorkDouble minBeta = 0.1 * mu;
          CoinWorkDouble maxBeta = 10.0 * mu;
          int nBL = 0;
          int nAL = 0;
          int nBU = 0;
          int nAU = 0;
          for (iColumn = 0; iColumn < numberRows_ + numberColumns_; iColumn++) {
               if (!flagged(iColumn)) {
                    if (lowerBound(iColumn)) {
                         CoinWorkDouble change = -rhsL_[iColumn] + deltaX_[iColumn];
                         CoinWorkDouble dualValue = zVec_[iColumn] + actualDualStep_ * deltaZ_[iColumn];
                         CoinWorkDouble primalValue = lowerSlack_[iColumn] + actualPrimalStep_ * change;
                         CoinWorkDouble gapProduct = dualValue * primalValue;
                         if (gapProduct < minBeta)
                              nBL++;
                         else if (gapProduct > maxBeta)
                              nAL++;
                         //if (gapProduct<0.1*minBeta)
                         //printf("Lsmall one %d dual %g primal %g\n",iColumn,
                         //   dualValue,primalValue);
                    }
                    if (upperBound(iColumn)) {
                         CoinWorkDouble change = rhsU_[iColumn] - deltaX_[iColumn];
                         CoinWorkDouble dualValue = wVec_[iColumn] + actualDualStep_ * deltaW_[iColumn];
                         CoinWorkDouble primalValue = upperSlack_[iColumn] + actualPrimalStep_ * change;
                         CoinWorkDouble gapProduct = dualValue * primalValue;
                         if (gapProduct < minBeta)
                              nBU++;
                         else if (gapProduct > maxBeta)
                              nAU++;
                         //if (gapProduct<0.1*minBeta)
                         //printf("Usmall one %d dual %g primal %g\n",iColumn,
                         //   dualValue,primalValue);
                    }
               }
          }
          printf("phase %d new mu %.18g new gap %.18g\n", phase, mu, sumL + sumU);
          printf("          %d lower, smallest %.18g, %d below - largest %.18g, %d above\n",
                 nL, smallestL, nBL, largestL, nAL);
          printf("          %d upper, smallest %.18g, %d below - largest %.18g, %d above\n",
                 nU, smallestU, nBU, largestU, nAU);
     }
#endif
     return directionNorm;
}
/* Does solve. region1 is for deltaX (columns+rows), region2 for deltaPi (rows) */
void
ClpPredictorCorrector::solveSystem(CoinWorkDouble * region1, CoinWorkDouble * region2,
                                   const CoinWorkDouble * region1In, const CoinWorkDouble * region2In,
                                   const CoinWorkDouble * saveRegion1, const CoinWorkDouble * saveRegion2,
                                   bool gentleRefine)
{
     int iRow;
     int numberTotal = numberRows_ + numberColumns_;
     if (region2In) {
          // normal
          for (iRow = 0; iRow < numberRows_; iRow++)
               region2[iRow] = region2In[iRow];
     } else {
          // initial solution - (diagonal is 1 or 0)
          CoinZeroN(region2, numberRows_);
     }
     int iColumn;
     if (cholesky_->type() < 20) {
          // not KKT
          for (iColumn = 0; iColumn < numberTotal; iColumn++)
               region1[iColumn] = region1In[iColumn] * diagonal_[iColumn];
          multiplyAdd(region1 + numberColumns_, numberRows_, -1.0, region2, 1.0);
          matrix_->times(1.0, region1, region2);
          CoinWorkDouble maximumRHS = maximumAbsElement(region2, numberRows_);
          CoinWorkDouble scale = 1.0;
          CoinWorkDouble unscale = 1.0;
          if (maximumRHS > 1.0e-30) {
               if (maximumRHS <= 0.5) {
                    CoinWorkDouble factor = 2.0;
                    while (maximumRHS <= 0.5) {
                         maximumRHS *= factor;
                         scale *= factor;
                    } /* endwhile */
               } else if (maximumRHS >= 2.0 && maximumRHS <= COIN_DBL_MAX) {
                    CoinWorkDouble factor = 0.5;
                    while (maximumRHS >= 2.0) {
                         maximumRHS *= factor;
                         scale *= factor;
                    } /* endwhile */
               }
               unscale = diagonalScaleFactor_ / scale;
          } else {
               //effectively zero
               scale = 0.0;
               unscale = 0.0;
          }
          multiplyAdd(NULL, numberRows_, 0.0, region2, scale);
          cholesky_->solve(region2);
          multiplyAdd(NULL, numberRows_, 0.0, region2, unscale);
          multiplyAdd(region2, numberRows_, -1.0, region1 + numberColumns_, 0.0);
          CoinZeroN(region1, numberColumns_);
          matrix_->transposeTimes(1.0, region2, region1);
          for (iColumn = 0; iColumn < numberTotal; iColumn++)
               region1[iColumn] = (region1[iColumn] - region1In[iColumn]) * diagonal_[iColumn];
     } else {
          for (iColumn = 0; iColumn < numberTotal; iColumn++)
               region1[iColumn] = region1In[iColumn];
          cholesky_->solveKKT(region1, region2, diagonal_, diagonalScaleFactor_);
     }
     if (saveRegion2) {
          //refine?
          CoinWorkDouble scaleX = 1.0;
          if (gentleRefine)
               scaleX = 0.8;
          multiplyAdd(saveRegion2, numberRows_, 1.0, region2, scaleX);
          assert (saveRegion1);
          multiplyAdd(saveRegion1, numberTotal, 1.0, region1, scaleX);
     }
}
// findDirectionVector.
CoinWorkDouble ClpPredictorCorrector::findDirectionVector(const int phase)
{
     CoinWorkDouble projectionTolerance = projectionTolerance_;
     //temporary
     //projectionTolerance=1.0e-15;
     CoinWorkDouble errorCheck = 0.9 * maximumRHSError_ / solutionNorm_;
     if (errorCheck > primalTolerance()) {
          if (errorCheck < projectionTolerance) {
               projectionTolerance = errorCheck;
          }
     } else {
          if (primalTolerance() < projectionTolerance) {
               projectionTolerance = primalTolerance();
          }
     }
     CoinWorkDouble * newError = new CoinWorkDouble [numberRows_];
     int numberTotal = numberRows_ + numberColumns_;
     //if flagged then entries zero so can do
     // For KKT separate out
     CoinWorkDouble * region1Save = NULL; //for refinement
     int iColumn;
     if (cholesky_->type() < 20) {
          int iColumn;
          for (iColumn = 0; iColumn < numberTotal; iColumn++)
               deltaX_[iColumn] = workArray_[iColumn] - solution_[iColumn];
          multiplyAdd(deltaX_ + numberColumns_, numberRows_, -1.0, deltaY_, 0.0);
          matrix_->times(1.0, deltaX_, deltaY_);
     } else {
          // regions in will be workArray and newError
          // regions out deltaX_ and deltaY_
          multiplyAdd(solution_ + numberColumns_, numberRows_, 1.0, newError, 0.0);
          matrix_->times(-1.0, solution_, newError);
          // This is inefficient but just for now get values which will be in deltay
          int iColumn;
          for (iColumn = 0; iColumn < numberTotal; iColumn++)
               deltaX_[iColumn] = workArray_[iColumn] - solution_[iColumn];
          multiplyAdd(deltaX_ + numberColumns_, numberRows_, -1.0, deltaY_, 0.0);
          matrix_->times(1.0, deltaX_, deltaY_);
     }
     bool goodSolve = false;
     CoinWorkDouble * regionSave = NULL; //for refinement
     int numberTries = 0;
     CoinWorkDouble relativeError = COIN_DBL_MAX;
     CoinWorkDouble tryError = 1.0e31;
     CoinWorkDouble saveMaximum = 0.0;
     double firstError = 0.0;
     double lastError2 = 0.0;
     while (!goodSolve && numberTries < 30) {
          CoinWorkDouble lastError = relativeError;
          goodSolve = true;
          CoinWorkDouble maximumRHS;
          maximumRHS = CoinMax(maximumAbsElement(deltaY_, numberRows_), 1.0e-12);
          if (!numberTries)
               saveMaximum = maximumRHS;
          if (cholesky_->type() < 20) {
               // no kkt
               CoinWorkDouble scale = 1.0;
               CoinWorkDouble unscale = 1.0;
               if (maximumRHS > 1.0e-30) {
                    if (maximumRHS <= 0.5) {
                         CoinWorkDouble factor = 2.0;
                         while (maximumRHS <= 0.5) {
                              maximumRHS *= factor;
                              scale *= factor;
                         } /* endwhile */
                    } else if (maximumRHS >= 2.0 && maximumRHS <= COIN_DBL_MAX) {
                         CoinWorkDouble factor = 0.5;
                         while (maximumRHS >= 2.0) {
                              maximumRHS *= factor;
                              scale *= factor;
                         } /* endwhile */
                    }
                    unscale = diagonalScaleFactor_ / scale;
               } else {
                    //effectively zero
                    scale = 0.0;
                    unscale = 0.0;
               }
               //printf("--putting scales to 1.0\n");
               //scale=1.0;
               //unscale=1.0;
               multiplyAdd(NULL, numberRows_, 0.0, deltaY_, scale);
               cholesky_->solve(deltaY_);
               multiplyAdd(NULL, numberRows_, 0.0, deltaY_, unscale);
#if 0
               {
                    printf("deltay\n");
                    for (int i = 0; i < numberRows_; i++)
                         printf("%d %.18g\n", i, deltaY_[i]);
               }
               exit(66);
#endif
               if (numberTries) {
                    //refine?
                    CoinWorkDouble scaleX = 1.0;
                    if (lastError > 1.0e-5)
                         scaleX = 0.8;
                    multiplyAdd(regionSave, numberRows_, 1.0, deltaY_, scaleX);
               }
               //CoinZeroN(newError,numberRows_);
               multiplyAdd(deltaY_, numberRows_, -1.0, deltaX_ + numberColumns_, 0.0);
               CoinZeroN(deltaX_, numberColumns_);
               matrix_->transposeTimes(1.0, deltaY_, deltaX_);
               //if flagged then entries zero so can do
               for (iColumn = 0; iColumn < numberTotal; iColumn++)
                    deltaX_[iColumn] = deltaX_[iColumn] * diagonal_[iColumn]
                                       - workArray_[iColumn];
          } else {
               // KKT
               solveSystem(deltaX_, deltaY_,
                           workArray_, newError, region1Save, regionSave, lastError > 1.0e-5);
          }
          multiplyAdd(deltaX_ + numberColumns_, numberRows_, -1.0, newError, 0.0);
          matrix_->times(1.0, deltaX_, newError);
          numberTries++;

          //now add in old Ax - doing extra checking
          CoinWorkDouble maximumRHSError = 0.0;
          CoinWorkDouble maximumRHSChange = 0.0;
          int iRow;
          char * dropped = cholesky_->rowsDropped();
          for (iRow = 0; iRow < numberRows_; iRow++) {
               if (!dropped[iRow]) {
                    CoinWorkDouble newValue = newError[iRow];
                    CoinWorkDouble oldValue = errorRegion_[iRow];
                    //severity of errors depend on signs
                    //**later                                                             */
                    if (CoinAbs(newValue) > maximumRHSChange) {
                         maximumRHSChange = CoinAbs(newValue);
                    }
                    CoinWorkDouble result = newValue + oldValue;
                    if (CoinAbs(result) > maximumRHSError) {
                         maximumRHSError = CoinAbs(result);
                    }
                    newError[iRow] = result;
               } else {
                    CoinWorkDouble newValue = newError[iRow];
                    CoinWorkDouble oldValue = errorRegion_[iRow];
                    if (CoinAbs(newValue) > maximumRHSChange) {
                         maximumRHSChange = CoinAbs(newValue);
                    }
                    CoinWorkDouble result = newValue + oldValue;
                    newError[iRow] = result;
                    //newError[iRow]=0.0;
                    //assert(deltaY_[iRow]==0.0);
                    deltaY_[iRow] = 0.0;
               }
          }
          relativeError = maximumRHSError / solutionNorm_;
          relativeError = maximumRHSError / saveMaximum;
          if (relativeError > tryError)
               relativeError = tryError;
          if (numberTries == 1)
               firstError = relativeError;
          if (relativeError < lastError) {
               lastError2 = relativeError;
               maximumRHSChange_ = maximumRHSChange;
               if (relativeError > projectionTolerance && numberTries <= 3) {
                    //try and refine
                    goodSolve = false;
               }
               //*** extra test here
               if (!goodSolve) {
                    if (!regionSave) {
                         regionSave = new CoinWorkDouble [numberRows_];
                         if (cholesky_->type() >= 20)
                              region1Save = new CoinWorkDouble [numberTotal];
                    }
                    CoinMemcpyN(deltaY_, numberRows_, regionSave);
                    if (cholesky_->type() < 20) {
                         // not KKT
                         multiplyAdd(newError, numberRows_, -1.0, deltaY_, 0.0);
                    } else {
                         // KKT
                         CoinMemcpyN(deltaX_, numberTotal, region1Save);
                         // and back to input region
                         CoinMemcpyN(deltaY_, numberRows_, newError);
                    }
               }
          } else {
               //std::cout <<" worse residual = "<<relativeError;
               //bring back previous
               relativeError = lastError;
               if (regionSave) {
                    CoinMemcpyN(regionSave, numberRows_, deltaY_);
                    if (cholesky_->type() < 20) {
                         // not KKT
                         multiplyAdd(deltaY_, numberRows_, -1.0, deltaX_ + numberColumns_, 0.0);
                         CoinZeroN(deltaX_, numberColumns_);
                         matrix_->transposeTimes(1.0, deltaY_, deltaX_);
                         //if flagged then entries zero so can do
                         for (iColumn = 0; iColumn < numberTotal; iColumn++)
                              deltaX_[iColumn] = deltaX_[iColumn] * diagonal_[iColumn]
                                                 - workArray_[iColumn];
                    } else {
                         // KKT
                         CoinMemcpyN(region1Save, numberTotal, deltaX_);
                    }
               } else {
                    // disaster
                    CoinFillN(deltaX_, numberTotal, static_cast<CoinWorkDouble>(1.0));
                    CoinFillN(deltaY_, numberRows_, static_cast<CoinWorkDouble>(1.0));
                    COIN_DETAIL_PRINT(printf("bad cholesky\n"));
               }
          }
     } /* endwhile */
     if (firstError > 1.0e-8 || numberTries > 1) {
          handler_->message(CLP_BARRIER_ACCURACY, messages_)
                    << phase << numberTries << static_cast<double>(firstError)
                    << static_cast<double>(lastError2)
                    << CoinMessageEol;
     }
     delete [] regionSave;
     delete [] region1Save;
     delete [] newError;
     // now rest
     CoinWorkDouble extra = eExtra;
     //multiplyAdd(deltaY_,numberRows_,1.0,deltaW_+numberColumns_,0.0);
     //CoinZeroN(deltaW_,numberColumns_);
     //matrix_->transposeTimes(-1.0,deltaY_,deltaW_);

     for (iColumn = 0; iColumn < numberRows_ + numberColumns_; iColumn++) {
          deltaSU_[iColumn] = 0.0;
          deltaSL_[iColumn] = 0.0;
          deltaZ_[iColumn] = 0.0;
          CoinWorkDouble dd = deltaW_[iColumn];
          deltaW_[iColumn] = 0.0;
          if (!flagged(iColumn)) {
               CoinWorkDouble deltaX = deltaX_[iColumn];
               if (lowerBound(iColumn)) {
                    CoinWorkDouble zValue = rhsZ_[iColumn];
                    CoinWorkDouble gHat = zValue + zVec_[iColumn] * rhsL_[iColumn];
                    CoinWorkDouble slack = lowerSlack_[iColumn] + extra;
                    deltaSL_[iColumn] = -rhsL_[iColumn] + deltaX;
                    deltaZ_[iColumn] = (gHat - zVec_[iColumn] * deltaX) / slack;
               }
               if (upperBound(iColumn)) {
                    CoinWorkDouble wValue = rhsW_[iColumn];
                    CoinWorkDouble hHat = wValue - wVec_[iColumn] * rhsU_[iColumn];
                    CoinWorkDouble slack = upperSlack_[iColumn] + extra;
                    deltaSU_[iColumn] = rhsU_[iColumn] - deltaX;
                    deltaW_[iColumn] = (hHat + wVec_[iColumn] * deltaX) / slack;
               }
               if (0) {
                    // different way of calculating
                    CoinWorkDouble gamma2 = gamma_ * gamma_;
                    CoinWorkDouble dZ = 0.0;
                    CoinWorkDouble dW = 0.0;
                    CoinWorkDouble zValue = rhsZ_[iColumn];
                    CoinWorkDouble gHat = zValue + zVec_[iColumn] * rhsL_[iColumn];
                    CoinWorkDouble slackL = lowerSlack_[iColumn] + extra;
                    CoinWorkDouble wValue = rhsW_[iColumn];
                    CoinWorkDouble hHat = wValue - wVec_[iColumn] * rhsU_[iColumn];
                    CoinWorkDouble slackU = upperSlack_[iColumn] + extra;
                    CoinWorkDouble q = rhsC_[iColumn] + gamma2 * deltaX + dd;
                    if (primalR_)
                         q += deltaX * primalR_[iColumn];
                    dW = (gHat + hHat - slackL * q + (wValue - zValue) * deltaX) / (slackL + slackU);
                    dZ = dW + q;
                    //printf("B %d old %g %g new %g %g\n",iColumn,deltaZ_[iColumn],
                    //deltaW_[iColumn],dZ,dW);
                    if (lowerBound(iColumn)) {
                         if (upperBound(iColumn)) {
                              //printf("B %d old %g %g new %g %g\n",iColumn,deltaZ_[iColumn],
                              //deltaW_[iColumn],dZ,dW);
                              deltaW_[iColumn] = dW;
                              deltaZ_[iColumn] = dZ;
                         } else {
                              // just lower
                              //printf("L %d old %g new %g\n",iColumn,deltaZ_[iColumn],
                              //dZ);
                         }
                    } else {
                         assert (upperBound(iColumn));
                         //printf("U %d old %g new %g\n",iColumn,deltaW_[iColumn],
                         //dW);
                    }
               }
          }
     }
#if 0
     CoinWorkDouble * check = new CoinWorkDouble[numberTotal];
     // Check out rhsC_
     multiplyAdd(deltaY_, numberRows_, -1.0, check + numberColumns_, 0.0);
     CoinZeroN(check, numberColumns_);
     matrix_->transposeTimes(1.0, deltaY_, check);
     quadraticDjs(check, deltaX_, -1.0);
     for (iColumn = 0; iColumn < numberTotal; iColumn++) {
          check[iColumn] += deltaZ_[iColumn] - deltaW_[iColumn];
          if (CoinAbs(check[iColumn] - rhsC_[iColumn]) > 1.0e-3)
               printf("rhsC %d %g %g\n", iColumn, check[iColumn], rhsC_[iColumn]);
     }
     // Check out rhsZ_
     for (iColumn = 0; iColumn < numberTotal; iColumn++) {
          check[iColumn] += lowerSlack_[iColumn] * deltaZ_[iColumn] +
                            zVec_[iColumn] * deltaSL_[iColumn];
          if (CoinAbs(check[iColumn] - rhsZ_[iColumn]) > 1.0e-3)
               printf("rhsZ %d %g %g\n", iColumn, check[iColumn], rhsZ_[iColumn]);
     }
     // Check out rhsW_
     for (iColumn = 0; iColumn < numberTotal; iColumn++) {
          check[iColumn] += upperSlack_[iColumn] * deltaW_[iColumn] +
                            wVec_[iColumn] * deltaSU_[iColumn];
          if (CoinAbs(check[iColumn] - rhsW_[iColumn]) > 1.0e-3)
               printf("rhsW %d %g %g\n", iColumn, check[iColumn], rhsW_[iColumn]);
     }
     delete [] check;
#endif
     return relativeError;
}
// createSolution.  Creates solution from scratch
int ClpPredictorCorrector::createSolution()
{
     int numberTotal = numberRows_ + numberColumns_;
     int iColumn;
     CoinWorkDouble tolerance = primalTolerance();
     // See if quadratic objective
#ifndef NO_RTTI
     ClpQuadraticObjective * quadraticObj = (dynamic_cast< ClpQuadraticObjective*>(objective_));
#else
     ClpQuadraticObjective * quadraticObj = NULL;
     if (objective_->type() == 2)
          quadraticObj = (static_cast< ClpQuadraticObjective*>(objective_));
#endif
     if (!quadraticObj) {
          for (iColumn = 0; iColumn < numberTotal; iColumn++) {
               if (upper_[iColumn] - lower_[iColumn] > tolerance)
                    clearFixed(iColumn);
               else
                    setFixed(iColumn);
          }
     } else {
          // try leaving fixed
          for (iColumn = 0; iColumn < numberTotal; iColumn++)
               clearFixed(iColumn);
     }

     CoinWorkDouble maximumObjective = 0.0;
     CoinWorkDouble objectiveNorm2 = 0.0;
     getNorms(cost_, numberTotal, maximumObjective, objectiveNorm2);
     if (!maximumObjective) {
          maximumObjective = 1.0; // objective all zero
     }
     objectiveNorm2 = CoinSqrt(objectiveNorm2) / static_cast<CoinWorkDouble> (numberTotal);
     objectiveNorm_ = maximumObjective;
     scaleFactor_ = 1.0;
     if (maximumObjective > 0.0) {
          if (maximumObjective < 1.0) {
               scaleFactor_ = maximumObjective;
          } else if (maximumObjective > 1.0e4) {
               scaleFactor_ = maximumObjective / 1.0e4;
          }
     }
     if (scaleFactor_ != 1.0) {
          objectiveNorm2 *= scaleFactor_;
          multiplyAdd(NULL, numberTotal, 0.0, cost_, 1.0 / scaleFactor_);
          objectiveNorm_ = maximumObjective / scaleFactor_;
     }
     // See if quadratic objective
     if (quadraticObj) {
          // If scaled then really scale matrix
          CoinWorkDouble scaleFactor =
               scaleFactor_ * optimizationDirection_ * objectiveScale_ *
               rhsScale_;
          if ((scalingFlag_ > 0 && rowScale_) || scaleFactor != 1.0) {
               CoinPackedMatrix * quadratic = quadraticObj->quadraticObjective();
               const int * columnQuadratic = quadratic->getIndices();
               const CoinBigIndex * columnQuadraticStart = quadratic->getVectorStarts();
               const int * columnQuadraticLength = quadratic->getVectorLengths();
               double * quadraticElement = quadratic->getMutableElements();
               int numberColumns = quadratic->getNumCols();
               CoinWorkDouble scale = 1.0 / scaleFactor;
               if (scalingFlag_ > 0 && rowScale_) {
                    for (int iColumn = 0; iColumn < numberColumns; iColumn++) {
                         CoinWorkDouble scaleI = columnScale_[iColumn] * scale;
                         for (CoinBigIndex j = columnQuadraticStart[iColumn];
                                   j < columnQuadraticStart[iColumn] + columnQuadraticLength[iColumn]; j++) {
                              int jColumn = columnQuadratic[j];
                              CoinWorkDouble scaleJ = columnScale_[jColumn];
                              quadraticElement[j] *= scaleI * scaleJ;
                              objectiveNorm_ = CoinMax(objectiveNorm_, CoinAbs(quadraticElement[j]));
                         }
                    }
               } else {
                    // not scaled
                    for (int iColumn = 0; iColumn < numberColumns; iColumn++) {
                         for (CoinBigIndex j = columnQuadraticStart[iColumn];
                                   j < columnQuadraticStart[iColumn] + columnQuadraticLength[iColumn]; j++) {
                              quadraticElement[j] *= scale;
                              objectiveNorm_ = CoinMax(objectiveNorm_, CoinAbs(quadraticElement[j]));
                         }
                    }
               }
          }
     }
     baseObjectiveNorm_ = objectiveNorm_;
     //accumulate fixed in dj region (as spare)
     //accumulate primal solution in primal region
     //DZ in lowerDual
     //DW in upperDual
     CoinWorkDouble infiniteCheck = 1.0e40;
     //CoinWorkDouble     fakeCheck=1.0e10;
     //use deltaX region for work region
     for (iColumn = 0; iColumn < numberTotal; iColumn++) {
          CoinWorkDouble primalValue = solution_[iColumn];
          clearFlagged(iColumn);
          clearFixedOrFree(iColumn);
          clearLowerBound(iColumn);
          clearUpperBound(iColumn);
          clearFakeLower(iColumn);
          clearFakeUpper(iColumn);
          if (!fixed(iColumn)) {
               dj_[iColumn] = 0.0;
               diagonal_[iColumn] = 1.0;
               deltaX_[iColumn] = 1.0;
               CoinWorkDouble lowerValue = lower_[iColumn];
               CoinWorkDouble upperValue = upper_[iColumn];
               if (lowerValue > -infiniteCheck) {
                    if (upperValue < infiniteCheck) {
                         //upper and lower bounds
                         setLowerBound(iColumn);
                         setUpperBound(iColumn);
                         if (lowerValue >= 0.0) {
                              solution_[iColumn] = lowerValue;
                         } else if (upperValue <= 0.0) {
                              solution_[iColumn] = upperValue;
                         } else {
                              solution_[iColumn] = 0.0;
                         }
                    } else {
                         //just lower bound
                         setLowerBound(iColumn);
                         if (lowerValue >= 0.0) {
                              solution_[iColumn] = lowerValue;
                         } else {
                              solution_[iColumn] = 0.0;
                         }
                    }
               } else {
                    if (upperValue < infiniteCheck) {
                         //just upper bound
                         setUpperBound(iColumn);
                         if (upperValue <= 0.0) {
                              solution_[iColumn] = upperValue;
                         } else {
                              solution_[iColumn] = 0.0;
                         }
                    } else {
                         //free
                         setFixedOrFree(iColumn);
                         solution_[iColumn] = 0.0;
                         //std::cout<<" free "<<i<<std::endl;
                    }
               }
          } else {
               setFlagged(iColumn);
               setFixedOrFree(iColumn);
               setLowerBound(iColumn);
               setUpperBound(iColumn);
               dj_[iColumn] = primalValue;;
               solution_[iColumn] = lower_[iColumn];
               diagonal_[iColumn] = 0.0;
               deltaX_[iColumn] = 0.0;
          }
     }
     //   modify fixed RHS
     multiplyAdd(dj_ + numberColumns_, numberRows_, -1.0, rhsFixRegion_, 0.0);
     //   create plausible RHS?
     matrix_->times(-1.0, dj_, rhsFixRegion_);
     multiplyAdd(solution_ + numberColumns_, numberRows_, 1.0, errorRegion_, 0.0);
     matrix_->times(-1.0, solution_, errorRegion_);
     rhsNorm_ = maximumAbsElement(errorRegion_, numberRows_);
     if (rhsNorm_ < 1.0) {
          rhsNorm_ = 1.0;
     }
     int * rowsDropped = new int [numberRows_];
     int returnCode = cholesky_->factorize(diagonal_, rowsDropped);
     if (returnCode == -1) {
       COIN_DETAIL_PRINT(printf("Out of memory\n"));
          problemStatus_ = 4;
          return -1;
     }
     if (cholesky_->status()) {
          std::cout << "singular on initial cholesky?" << std::endl;
          cholesky_->resetRowsDropped();
          //cholesky_->factorize(rowDropped_);
          //if (cholesky_->status()) {
          //std::cout << "bad cholesky??? (after retry)" <<std::endl;
          //abort();
          //}
     }
     delete [] rowsDropped;
     if (cholesky_->type() < 20) {
          // not KKT
          cholesky_->solve(errorRegion_);
          //create information for solution
          multiplyAdd(errorRegion_, numberRows_, -1.0, deltaX_ + numberColumns_, 0.0);
          CoinZeroN(deltaX_, numberColumns_);
          matrix_->transposeTimes(1.0, errorRegion_, deltaX_);
     } else {
          // KKT
          // reverse sign on solution
          multiplyAdd(NULL, numberRows_ + numberColumns_, 0.0, solution_, -1.0);
          solveSystem(deltaX_, errorRegion_, solution_, NULL, NULL, NULL, false);
     }
     CoinWorkDouble initialValue = 1.0e2;
     if (rhsNorm_ * 1.0e-2 > initialValue) {
          initialValue = rhsNorm_ * 1.0e-2;
     }
     //initialValue = CoinMax(1.0,rhsNorm_);
     CoinWorkDouble smallestBoundDifference = COIN_DBL_MAX;
     CoinWorkDouble * fakeSolution = deltaX_;
     for ( iColumn = 0; iColumn < numberTotal; iColumn++) {
          if (!flagged(iColumn)) {
               if (lower_[iColumn] - fakeSolution[iColumn] > initialValue) {
                    initialValue = lower_[iColumn] - fakeSolution[iColumn];
               }
               if (fakeSolution[iColumn] - upper_[iColumn] > initialValue) {
                    initialValue = fakeSolution[iColumn] - upper_[iColumn];
               }
               if (upper_[iColumn] - lower_[iColumn] < smallestBoundDifference) {
                    smallestBoundDifference = upper_[iColumn] - lower_[iColumn];
               }
          }
     }
     solutionNorm_ = 1.0e-12;
     handler_->message(CLP_BARRIER_SAFE, messages_)
               << static_cast<double>(initialValue) << static_cast<double>(objectiveNorm_)
               << CoinMessageEol;
     CoinWorkDouble extra = 1.0e-10;
     CoinWorkDouble largeGap = 1.0e15;
     //CoinWorkDouble safeObjectiveValue=2.0*objectiveNorm_;
     CoinWorkDouble safeObjectiveValue = objectiveNorm_ + 1.0;
     CoinWorkDouble safeFree = 1.0e-1 * initialValue;
     //printf("normal safe dual value of %g, primal value of %g\n",
     // safeObjectiveValue,initialValue);
     //safeObjectiveValue=CoinMax(2.0,1.0e-1*safeObjectiveValue);
     //initialValue=CoinMax(100.0,1.0e-1*initialValue);;
     //printf("temp safe dual value of %g, primal value of %g\n",
     // safeObjectiveValue,initialValue);
     CoinWorkDouble zwLarge = 1.0e2 * initialValue;
     //zwLarge=1.0e40;
     if (cholesky_->choleskyCondition() < 0.0 && cholesky_->type() < 20) {
          // looks bad - play safe
          initialValue *= 10.0;
          safeObjectiveValue *= 10.0;
          safeFree *= 10.0;
     }
     CoinWorkDouble gamma2 = gamma_ * gamma_; // gamma*gamma will be added to diagonal
     // First do primal side
     for ( iColumn = 0; iColumn < numberTotal; iColumn++) {
          if (!flagged(iColumn)) {
               CoinWorkDouble lowerValue = lower_[iColumn];
               CoinWorkDouble upperValue = upper_[iColumn];
               CoinWorkDouble newValue;
               CoinWorkDouble setPrimal = initialValue;
               if (quadraticObj) {
                    // perturb primal solution a bit
                    //fakeSolution[iColumn]  *= 0.002*CoinDrand48()+0.999;
               }
               if (lowerBound(iColumn)) {
                    if (upperBound(iColumn)) {
                         //upper and lower bounds
                         if (upperValue - lowerValue > 2.0 * setPrimal) {
                              CoinWorkDouble fakeValue = fakeSolution[iColumn];
                              if (fakeValue < lowerValue + setPrimal) {
                                   fakeValue = lowerValue + setPrimal;
                              }
                              if (fakeValue > upperValue - setPrimal) {
                                   fakeValue = upperValue - setPrimal;
                              }
                              newValue = fakeValue;
                         } else {
                              newValue = 0.5 * (upperValue + lowerValue);
                         }
                    } else {
                         //just lower bound
                         CoinWorkDouble fakeValue = fakeSolution[iColumn];
                         if (fakeValue < lowerValue + setPrimal) {
                              fakeValue = lowerValue + setPrimal;
                         }
                         newValue = fakeValue;
                    }
               } else {
                    if (upperBound(iColumn)) {
                         //just upper bound
                         CoinWorkDouble fakeValue = fakeSolution[iColumn];
                         if (fakeValue > upperValue - setPrimal) {
                              fakeValue = upperValue - setPrimal;
                         }
                         newValue = fakeValue;
                    } else {
                         //free
                         newValue = fakeSolution[iColumn];
                         if (newValue >= 0.0) {
                              if (newValue < safeFree) {
                                   newValue = safeFree;
                              }
                         } else {
                              if (newValue > -safeFree) {
                                   newValue = -safeFree;
                              }
                         }
                    }
               }
               solution_[iColumn] = newValue;
          } else {
               // fixed
               lowerSlack_[iColumn] = 0.0;
               upperSlack_[iColumn] = 0.0;
               solution_[iColumn] = lower_[iColumn];
               zVec_[iColumn] = 0.0;
               wVec_[iColumn] = 0.0;
               diagonal_[iColumn] = 0.0;
          }
     }
     solutionNorm_ =  maximumAbsElement(solution_, numberTotal);
     // Set bounds and do dj including quadratic
     largeGap = CoinMax(1.0e7, 1.02 * solutionNorm_);
     CoinPackedMatrix * quadratic = NULL;
     const int * columnQuadratic = NULL;
     const CoinBigIndex * columnQuadraticStart = NULL;
     const int * columnQuadraticLength = NULL;
     const double * quadraticElement = NULL;
     if (quadraticObj) {
          quadratic = quadraticObj->quadraticObjective();
          columnQuadratic = quadratic->getIndices();
          columnQuadraticStart = quadratic->getVectorStarts();
          columnQuadraticLength = quadratic->getVectorLengths();
          quadraticElement = quadratic->getElements();
     }
     CoinWorkDouble quadraticNorm = 0.0;
     for ( iColumn = 0; iColumn < numberTotal; iColumn++) {
          if (!flagged(iColumn)) {
               CoinWorkDouble primalValue = solution_[iColumn];
               CoinWorkDouble lowerValue = lower_[iColumn];
               CoinWorkDouble upperValue = upper_[iColumn];
               // Do dj
               CoinWorkDouble reducedCost = cost_[iColumn];
               if (lowerBound(iColumn)) {
                    reducedCost += linearPerturbation_;
               }
               if (upperBound(iColumn)) {
                    reducedCost -= linearPerturbation_;
               }
               if (quadraticObj && iColumn < numberColumns_) {
                    for (CoinBigIndex j = columnQuadraticStart[iColumn];
                              j < columnQuadraticStart[iColumn] + columnQuadraticLength[iColumn]; j++) {
                         int jColumn = columnQuadratic[j];
                         CoinWorkDouble valueJ = solution_[jColumn];
                         CoinWorkDouble elementValue = quadraticElement[j];
                         reducedCost += valueJ * elementValue;
                    }
                    quadraticNorm = CoinMax(quadraticNorm, CoinAbs(reducedCost));
               }
               dj_[iColumn] = reducedCost;
               if (primalValue > lowerValue + largeGap && primalValue < upperValue - largeGap) {
                    clearFixedOrFree(iColumn);
                    setLowerBound(iColumn);
                    setUpperBound(iColumn);
                    lowerValue = CoinMax(lowerValue, primalValue - largeGap);
                    upperValue = CoinMin(upperValue, primalValue + largeGap);
                    lower_[iColumn] = lowerValue;
                    upper_[iColumn] = upperValue;
               }
          }
     }
     safeObjectiveValue = CoinMax(safeObjectiveValue, quadraticNorm);
     for ( iColumn = 0; iColumn < numberTotal; iColumn++) {
          if (!flagged(iColumn)) {
               CoinWorkDouble primalValue = solution_[iColumn];
               CoinWorkDouble lowerValue = lower_[iColumn];
               CoinWorkDouble upperValue = upper_[iColumn];
               CoinWorkDouble reducedCost = dj_[iColumn];
               CoinWorkDouble low = 0.0;
               CoinWorkDouble high = 0.0;
               if (lowerBound(iColumn)) {
                    if (upperBound(iColumn)) {
                         //upper and lower bounds
                         if (upperValue - lowerValue > 2.0 * initialValue) {
                              low = primalValue - lowerValue;
                              high = upperValue - primalValue;
                         } else {
                              low = initialValue;
                              high = initialValue;
                         }
                         CoinWorkDouble s = low + extra;
                         CoinWorkDouble ratioZ;
                         if (s < zwLarge) {
                              ratioZ = 1.0;
                         } else {
                              ratioZ = CoinSqrt(zwLarge / s);
                         }
                         CoinWorkDouble t = high + extra;
                         CoinWorkDouble ratioT;
                         if (t < zwLarge) {
                              ratioT = 1.0;
                         } else {
                              ratioT = CoinSqrt(zwLarge / t);
                         }
                         //modify s and t
                         if (s > largeGap) {
                              s = largeGap;
                         }
                         if (t > largeGap) {
                              t = largeGap;
                         }
                         //modify if long long way away from bound
                         if (reducedCost >= 0.0) {
                              zVec_[iColumn] = reducedCost + safeObjectiveValue * ratioZ;
                              zVec_[iColumn] = CoinMax(reducedCost, safeObjectiveValue * ratioZ);
                              wVec_[iColumn] = safeObjectiveValue * ratioT;
                         } else {
                              zVec_[iColumn] = safeObjectiveValue * ratioZ;
                              wVec_[iColumn] = -reducedCost + safeObjectiveValue * ratioT;
                              wVec_[iColumn] = CoinMax(-reducedCost , safeObjectiveValue * ratioT);
                         }
                         CoinWorkDouble gammaTerm = gamma2;
                         if (primalR_)
                              gammaTerm += primalR_[iColumn];
                         diagonal_[iColumn] = (t * s) /
                                              (s * wVec_[iColumn] + t * zVec_[iColumn] + gammaTerm * t * s);
                    } else {
                         //just lower bound
                         low = primalValue - lowerValue;
                         high = 0.0;
                         CoinWorkDouble s = low + extra;
                         CoinWorkDouble ratioZ;
                         if (s < zwLarge) {
                              ratioZ = 1.0;
                         } else {
                              ratioZ = CoinSqrt(zwLarge / s);
                         }
                         //modify s
                         if (s > largeGap) {
                              s = largeGap;
                         }
                         if (reducedCost >= 0.0) {
                              zVec_[iColumn] = reducedCost + safeObjectiveValue * ratioZ;
                              zVec_[iColumn] = CoinMax(reducedCost , safeObjectiveValue * ratioZ);
                              wVec_[iColumn] = 0.0;
                         } else {
                              zVec_[iColumn] = safeObjectiveValue * ratioZ;
                              wVec_[iColumn] = 0.0;
                         }
                         CoinWorkDouble gammaTerm = gamma2;
                         if (primalR_)
                              gammaTerm += primalR_[iColumn];
                         diagonal_[iColumn] = s / (zVec_[iColumn] + s * gammaTerm);
                    }
               } else {
                    if (upperBound(iColumn)) {
                         //just upper bound
                         low = 0.0;
                         high = upperValue - primalValue;
                         CoinWorkDouble t = high + extra;
                         CoinWorkDouble ratioT;
                         if (t < zwLarge) {
                              ratioT = 1.0;
                         } else {
                              ratioT = CoinSqrt(zwLarge / t);
                         }
                         //modify t
                         if (t > largeGap) {
                              t = largeGap;
                         }
                         if (reducedCost >= 0.0) {
                              zVec_[iColumn] = 0.0;
                              wVec_[iColumn] = safeObjectiveValue * ratioT;
                         } else {
                              zVec_[iColumn] = 0.0;
                              wVec_[iColumn] = -reducedCost + safeObjectiveValue * ratioT;
                              wVec_[iColumn] = CoinMax(-reducedCost , safeObjectiveValue * ratioT);
                         }
                         CoinWorkDouble gammaTerm = gamma2;
                         if (primalR_)
                              gammaTerm += primalR_[iColumn];
                         diagonal_[iColumn] =  t / (wVec_[iColumn] + t * gammaTerm);
                    }
               }
               lowerSlack_[iColumn] = low;
               upperSlack_[iColumn] = high;
          }
     }
#if 0
     if (solution_[0] > 0.0) {
          for (int i = 0; i < numberTotal; i++)
               printf("%d %.18g %.18g %.18g %.18g %.18g %.18g %.18g\n", i, CoinAbs(solution_[i]),
                      diagonal_[i], CoinAbs(dj_[i]),
                      lowerSlack_[i], zVec_[i],
                      upperSlack_[i], wVec_[i]);
     } else {
          for (int i = 0; i < numberTotal; i++)
               printf("%d %.18g %.18g %.18g %.18g %.18g %.18g %.18g\n", i, CoinAbs(solution_[i]),
                      diagonal_[i], CoinAbs(dj_[i]),
                      upperSlack_[i], wVec_[i],
                      lowerSlack_[i], zVec_[i] );
     }
     exit(66);
#endif
     return 0;
}
// complementarityGap.  Computes gap
//phase 0=as is , 1 = after predictor , 2 after corrector
CoinWorkDouble ClpPredictorCorrector::complementarityGap(int & numberComplementarityPairs,
          int & numberComplementarityItems,
          const int phase)
{
     CoinWorkDouble gap = 0.0;
     //seems to be same coding for phase = 1 or 2
     numberComplementarityPairs = 0;
     numberComplementarityItems = 0;
     int numberTotal = numberRows_ + numberColumns_;
     CoinWorkDouble toleranceGap = 0.0;
     CoinWorkDouble largestGap = 0.0;
     CoinWorkDouble smallestGap = COIN_DBL_MAX;
     //seems to be same coding for phase = 1 or 2
     int numberNegativeGaps = 0;
     CoinWorkDouble sumNegativeGap = 0.0;
     CoinWorkDouble largeGap = 1.0e2 * solutionNorm_;
     if (largeGap < 1.0e10) {
          largeGap = 1.0e10;
     }
     largeGap = 1.0e30;
     CoinWorkDouble dualTolerance =  dblParam_[ClpDualTolerance];
     CoinWorkDouble primalTolerance =  dblParam_[ClpPrimalTolerance];
     dualTolerance = dualTolerance / scaleFactor_;
     for (int iColumn = 0; iColumn < numberTotal; iColumn++) {
          if (!fixedOrFree(iColumn)) {
               numberComplementarityPairs++;
               //can collapse as if no lower bound both zVec and deltaZ 0.0
               if (lowerBound(iColumn)) {
                    numberComplementarityItems++;
                    CoinWorkDouble dualValue;
                    CoinWorkDouble primalValue;
                    if (!phase) {
                         dualValue = zVec_[iColumn];
                         primalValue = lowerSlack_[iColumn];
                    } else {
                         CoinWorkDouble change;
                         change = solution_[iColumn] + deltaX_[iColumn] - lowerSlack_[iColumn] - lower_[iColumn];
                         dualValue = zVec_[iColumn] + actualDualStep_ * deltaZ_[iColumn];
                         primalValue = lowerSlack_[iColumn] + actualPrimalStep_ * change;
                    }
                    //reduce primalValue
                    if (primalValue > largeGap) {
                         primalValue = largeGap;
                    }
                    CoinWorkDouble gapProduct = dualValue * primalValue;
                    if (gapProduct < 0.0) {
                         //cout<<"negative gap component "<<iColumn<<" "<<dualValue<<" "<<
                         //primalValue<<endl;
                         numberNegativeGaps++;
                         sumNegativeGap -= gapProduct;
                         gapProduct = 0.0;
                    }
                    gap += gapProduct;
                    //printf("l %d prim %g dual %g totalGap %g\n",
                    //   iColumn,primalValue,dualValue,gap);
                    if (gapProduct > largestGap) {
                         largestGap = gapProduct;
                    }
                    smallestGap = CoinMin(smallestGap, gapProduct);
                    if (dualValue > dualTolerance && primalValue > primalTolerance) {
                         toleranceGap += dualValue * primalValue;
                    }
               }
               if (upperBound(iColumn)) {
                    numberComplementarityItems++;
                    CoinWorkDouble dualValue;
                    CoinWorkDouble primalValue;
                    if (!phase) {
                         dualValue = wVec_[iColumn];
                         primalValue = upperSlack_[iColumn];
                    } else {
                         CoinWorkDouble change;
                         change = upper_[iColumn] - solution_[iColumn] - deltaX_[iColumn] - upperSlack_[iColumn];
                         dualValue = wVec_[iColumn] + actualDualStep_ * deltaW_[iColumn];
                         primalValue = upperSlack_[iColumn] + actualPrimalStep_ * change;
                    }
                    //reduce primalValue
                    if (primalValue > largeGap) {
                         primalValue = largeGap;
                    }
                    CoinWorkDouble gapProduct = dualValue * primalValue;
                    if (gapProduct < 0.0) {
                         //cout<<"negative gap component "<<iColumn<<" "<<dualValue<<" "<<
                         //primalValue<<endl;
                         numberNegativeGaps++;
                         sumNegativeGap -= gapProduct;
                         gapProduct = 0.0;
                    }
                    gap += gapProduct;
                    //printf("u %d prim %g dual %g totalGap %g\n",
                    //   iColumn,primalValue,dualValue,gap);
                    if (gapProduct > largestGap) {
                         largestGap = gapProduct;
                    }
                    if (dualValue > dualTolerance && primalValue > primalTolerance) {
                         toleranceGap += dualValue * primalValue;
                    }
               }
          }
     }
     //if (numberIterations_>4)
     //exit(9);
     if (!phase && numberNegativeGaps) {
          handler_->message(CLP_BARRIER_NEGATIVE_GAPS, messages_)
                    << numberNegativeGaps << static_cast<double>(sumNegativeGap)
                    << CoinMessageEol;
     }

     //in case all free!
     if (!numberComplementarityPairs) {
          numberComplementarityPairs = 1;
     }
#ifdef SOME_DEBUG
     printf("with d,p steps %g,%g gap %g - smallest %g, largest %g, pairs %d\n",
            actualDualStep_, actualPrimalStep_,
            gap, smallestGap, largestGap, numberComplementarityPairs);
#endif
     return gap;
}
// setupForSolve.
//phase 0=affine , 1 = corrector , 2 = primal-dual
void ClpPredictorCorrector::setupForSolve(const int phase)
{
     CoinWorkDouble extra = eExtra;
     int numberTotal = numberRows_ + numberColumns_;
     int iColumn;
#ifdef SOME_DEBUG
     printf("phase %d in setupForSolve, mu %.18g\n", phase, mu_);
#endif
     CoinWorkDouble gamma2 = gamma_ * gamma_; // gamma*gamma will be added to diagonal
     CoinWorkDouble * dualArray = reinterpret_cast<CoinWorkDouble *>(dual_);
     switch (phase) {
     case 0:
          CoinMemcpyN(errorRegion_, numberRows_, rhsB_);
          if (delta_ || dualR_) {
               // add in regularization
               CoinWorkDouble delta2 = delta_ * delta_;
               for (int iRow = 0; iRow < numberRows_; iRow++) {
                    rhsB_[iRow] -= delta2 * dualArray[iRow];
                    if (dualR_)
                         rhsB_[iRow] -= dualR_[iRow] * dualArray[iRow];
               }
          }
          for (iColumn = 0; iColumn < numberTotal; iColumn++) {
               rhsC_[iColumn] = 0.0;
               rhsU_[iColumn] = 0.0;
               rhsL_[iColumn] = 0.0;
               rhsZ_[iColumn] = 0.0;
               rhsW_[iColumn] = 0.0;
               if (!flagged(iColumn)) {
                    rhsC_[iColumn] = dj_[iColumn] - zVec_[iColumn] + wVec_[iColumn];
                    rhsC_[iColumn] += gamma2 * solution_[iColumn];
                    if (primalR_)
                         rhsC_[iColumn] += primalR_[iColumn] * solution_[iColumn];
                    if (lowerBound(iColumn)) {
                         rhsZ_[iColumn] = -zVec_[iColumn] * (lowerSlack_[iColumn] + extra);
                         rhsL_[iColumn] = CoinMax(0.0, (lower_[iColumn] + lowerSlack_[iColumn]) - solution_[iColumn]);
                    }
                    if (upperBound(iColumn)) {
                         rhsW_[iColumn] = -wVec_[iColumn] * (upperSlack_[iColumn] + extra);
                         rhsU_[iColumn] = CoinMin(0.0, (upper_[iColumn] - upperSlack_[iColumn]) - solution_[iColumn]);
                    }
               }
          }
#if 0
          for (int i = 0; i < 3; i++) {
               if (!CoinAbs(rhsZ_[i]))
                    rhsZ_[i] = 0.0;
               if (!CoinAbs(rhsW_[i]))
                    rhsW_[i] = 0.0;
               if (!CoinAbs(rhsU_[i]))
                    rhsU_[i] = 0.0;
               if (!CoinAbs(rhsL_[i]))
                    rhsL_[i] = 0.0;
          }
          if (solution_[0] > 0.0) {
               for (int i = 0; i < 3; i++)
                    printf("%d %.18g %.18g %.18g %.18g %.18g %.18g %.18g\n", i, solution_[i],
                           diagonal_[i], dj_[i],
                           lowerSlack_[i], zVec_[i],
                           upperSlack_[i], wVec_[i]);
               for (int i = 0; i < 3; i++)
                    printf("%d %.18g %.18g %.18g %.18g %.18g\n", i, rhsC_[i],
                           rhsZ_[i], rhsL_[i],
                           rhsW_[i], rhsU_[i]);
          } else {
               for (int i = 0; i < 3; i++)
                    printf("%d %.18g %.18g %.18g %.18g %.18g %.18g %.18g\n", i, solution_[i],
                           diagonal_[i], dj_[i],
                           lowerSlack_[i], zVec_[i],
                           upperSlack_[i], wVec_[i]);
               for (int i = 0; i < 3; i++)
                    printf("%d %.18g %.18g %.18g %.18g %.18g\n", i, rhsC_[i],
                           rhsZ_[i], rhsL_[i],
                           rhsW_[i], rhsU_[i]);
          }
#endif
          break;
     case 1:
          // could be stored in delta2?
          for (iColumn = 0; iColumn < numberTotal; iColumn++) {
               rhsZ_[iColumn] = 0.0;
               rhsW_[iColumn] = 0.0;
               if (!flagged(iColumn)) {
                    if (lowerBound(iColumn)) {
                         rhsZ_[iColumn] = mu_ - zVec_[iColumn] * (lowerSlack_[iColumn] + extra)
                                          - deltaZ_[iColumn] * deltaX_[iColumn];
                         // To bring in line with OSL
                         rhsZ_[iColumn] += deltaZ_[iColumn] * rhsL_[iColumn];
                    }
                    if (upperBound(iColumn)) {
                         rhsW_[iColumn] = mu_ - wVec_[iColumn] * (upperSlack_[iColumn] + extra)
                                          + deltaW_[iColumn] * deltaX_[iColumn];
                         // To bring in line with OSL
                         rhsW_[iColumn] -= deltaW_[iColumn] * rhsU_[iColumn];
                    }
               }
          }
#if 0
          for (int i = 0; i < numberTotal; i++) {
               if (!CoinAbs(rhsZ_[i]))
                    rhsZ_[i] = 0.0;
               if (!CoinAbs(rhsW_[i]))
                    rhsW_[i] = 0.0;
               if (!CoinAbs(rhsU_[i]))
                    rhsU_[i] = 0.0;
               if (!CoinAbs(rhsL_[i]))
                    rhsL_[i] = 0.0;
          }
          if (solution_[0] > 0.0) {
               for (int i = 0; i < numberTotal; i++)
                    printf("%d %.18g %.18g %.18g %.18g %.18g %.18g %.18g\n", i, CoinAbs(solution_[i]),
                           diagonal_[i], CoinAbs(dj_[i]),
                           lowerSlack_[i], zVec_[i],
                           upperSlack_[i], wVec_[i]);
               for (int i = 0; i < numberTotal; i++)
                    printf("%d %.18g %.18g %.18g %.18g %.18g\n", i, CoinAbs(rhsC_[i]),
                           rhsZ_[i], rhsL_[i],
                           rhsW_[i], rhsU_[i]);
          } else {
               for (int i = 0; i < numberTotal; i++)
                    printf("%d %.18g %.18g %.18g %.18g %.18g %.18g %.18g\n", i, CoinAbs(solution_[i]),
                           diagonal_[i], CoinAbs(dj_[i]),
                           upperSlack_[i], wVec_[i],
                           lowerSlack_[i], zVec_[i] );
               for (int i = 0; i < numberTotal; i++)
                    printf("%d %.18g %.18g %.18g %.18g %.18g\n", i, CoinAbs(rhsC_[i]),
                           rhsW_[i], rhsU_[i],
                           rhsZ_[i], rhsL_[i]);
          }
          exit(66);
#endif
          break;
     case 2:
          CoinMemcpyN(errorRegion_, numberRows_, rhsB_);
          for (iColumn = 0; iColumn < numberTotal; iColumn++) {
               rhsZ_[iColumn] = 0.0;
               rhsW_[iColumn] = 0.0;
               if (!flagged(iColumn)) {
                    if (lowerBound(iColumn)) {
                         rhsZ_[iColumn] = mu_ - zVec_[iColumn] * (lowerSlack_[iColumn] + extra);
                    }
                    if (upperBound(iColumn)) {
                         rhsW_[iColumn] = mu_ - wVec_[iColumn] * (upperSlack_[iColumn] + extra);
                    }
               }
          }
          break;
     case 3: {
          CoinWorkDouble minBeta = 0.1 * mu_;
          CoinWorkDouble maxBeta = 10.0 * mu_;
          CoinWorkDouble dualStep = CoinMin(1.0, actualDualStep_ + 0.1);
          CoinWorkDouble primalStep = CoinMin(1.0, actualPrimalStep_ + 0.1);
#ifdef SOME_DEBUG
          printf("good complementarity range %g to %g\n", minBeta, maxBeta);
#endif
          //minBeta=0.0;
          //maxBeta=COIN_DBL_MAX;
          for (iColumn = 0; iColumn < numberTotal; iColumn++) {
               if (!flagged(iColumn)) {
                    if (lowerBound(iColumn)) {
                         CoinWorkDouble change = -rhsL_[iColumn] + deltaX_[iColumn];
                         CoinWorkDouble dualValue = zVec_[iColumn] + dualStep * deltaZ_[iColumn];
                         CoinWorkDouble primalValue = lowerSlack_[iColumn] + primalStep * change;
                         CoinWorkDouble gapProduct = dualValue * primalValue;
                         if (gapProduct > 0.0 && dualValue < 0.0)
                              gapProduct = - gapProduct;
#ifdef FULL_DEBUG
                         delta2Z_[iColumn] = gapProduct;
                         if (delta2Z_[iColumn] < minBeta || delta2Z_[iColumn] > maxBeta)
                              printf("lower %d primal %g, dual %g, gap %g\n",
                                     iColumn, primalValue, dualValue, gapProduct);
#endif
                         CoinWorkDouble value = 0.0;
                         if (gapProduct < minBeta) {
                              value = 2.0 * (minBeta - gapProduct);
                              value = (mu_ - gapProduct);
                              value = (minBeta - gapProduct);
                              assert (value > 0.0);
                         } else if (gapProduct > maxBeta) {
                              value = CoinMax(maxBeta - gapProduct, -maxBeta);
                              assert (value < 0.0);
                         }
                         rhsZ_[iColumn] += value;
                    }
                    if (upperBound(iColumn)) {
                         CoinWorkDouble change = rhsU_[iColumn] - deltaX_[iColumn];
                         CoinWorkDouble dualValue = wVec_[iColumn] + dualStep * deltaW_[iColumn];
                         CoinWorkDouble primalValue = upperSlack_[iColumn] + primalStep * change;
                         CoinWorkDouble gapProduct = dualValue * primalValue;
                         if (gapProduct > 0.0 && dualValue < 0.0)
                              gapProduct = - gapProduct;
#ifdef FULL_DEBUG
                         delta2W_[iColumn] = gapProduct;
                         if (delta2W_[iColumn] < minBeta || delta2W_[iColumn] > maxBeta)
                              printf("upper %d primal %g, dual %g, gap %g\n",
                                     iColumn, primalValue, dualValue, gapProduct);
#endif
                         CoinWorkDouble value = 0.0;
                         if (gapProduct < minBeta) {
                              value = (minBeta - gapProduct);
                              assert (value > 0.0);
                         } else if (gapProduct > maxBeta) {
                              value = CoinMax(maxBeta - gapProduct, -maxBeta);
                              assert (value < 0.0);
                         }
                         rhsW_[iColumn] += value;
                    }
               }
          }
     }
     break;
     } /* endswitch */
     if (cholesky_->type() < 20) {
          for (iColumn = 0; iColumn < numberTotal; iColumn++) {
               CoinWorkDouble value = rhsC_[iColumn];
               CoinWorkDouble zValue = rhsZ_[iColumn];
               CoinWorkDouble wValue = rhsW_[iColumn];
#if 0
#if 1
               if (phase == 0) {
                    // more accurate
                    value = dj[iColumn];
                    zValue = 0.0;
                    wValue = 0.0;
               } else if (phase == 2) {
                    // more accurate
                    value = dj[iColumn];
                    zValue = mu_;
                    wValue = mu_;
               }
#endif
               assert (rhsL_[iColumn] >= 0.0);
               assert (rhsU_[iColumn] <= 0.0);
               if (lowerBound(iColumn)) {
                    value += (-zVec_[iColumn] * rhsL_[iColumn] - zValue) /
                             (lowerSlack_[iColumn] + extra);
               }
               if (upperBound(iColumn)) {
                    value += (wValue - wVec_[iColumn] * rhsU_[iColumn]) /
                             (upperSlack_[iColumn] + extra);
               }
#else
               if (lowerBound(iColumn)) {
                    CoinWorkDouble gHat = zValue + zVec_[iColumn] * rhsL_[iColumn];
                    value -= gHat / (lowerSlack_[iColumn] + extra);
               }
               if (upperBound(iColumn)) {
                    CoinWorkDouble hHat = wValue - wVec_[iColumn] * rhsU_[iColumn];
                    value += hHat / (upperSlack_[iColumn] + extra);
               }
#endif
               workArray_[iColumn] = diagonal_[iColumn] * value;
          }
#if 0
          if (solution_[0] > 0.0) {
               for (int i = 0; i < numberTotal; i++)
                    printf("%d %.18g\n", i, workArray_[i]);
          } else {
               for (int i = 0; i < numberTotal; i++)
                    printf("%d %.18g\n", i, workArray_[i]);
          }
          exit(66);
#endif
     } else {
          // KKT
          for (iColumn = 0; iColumn < numberTotal; iColumn++) {
               CoinWorkDouble value = rhsC_[iColumn];
               CoinWorkDouble zValue = rhsZ_[iColumn];
               CoinWorkDouble wValue = rhsW_[iColumn];
               if (lowerBound(iColumn)) {
                    CoinWorkDouble gHat = zValue + zVec_[iColumn] * rhsL_[iColumn];
                    value -= gHat / (lowerSlack_[iColumn] + extra);
               }
               if (upperBound(iColumn)) {
                    CoinWorkDouble hHat = wValue - wVec_[iColumn] * rhsU_[iColumn];
                    value += hHat / (upperSlack_[iColumn] + extra);
               }
               workArray_[iColumn] = value;
          }
     }
}
//method: sees if looks plausible change in complementarity
bool ClpPredictorCorrector::checkGoodMove(const bool doCorrector,
          CoinWorkDouble & bestNextGap,
          bool allowIncreasingGap)
{
     const CoinWorkDouble beta3 = 0.99997;
     bool goodMove = false;
     int nextNumber;
     int nextNumberItems;
     int numberTotal = numberRows_ + numberColumns_;
     CoinWorkDouble returnGap = bestNextGap;
     CoinWorkDouble nextGap = complementarityGap(nextNumber, nextNumberItems, 2);
#ifndef NO_RTTI
     ClpQuadraticObjective * quadraticObj = (dynamic_cast< ClpQuadraticObjective*>(objective_));
#else
     ClpQuadraticObjective * quadraticObj = NULL;
     if (objective_->type() == 2)
          quadraticObj = (static_cast< ClpQuadraticObjective*>(objective_));
#endif
     if (nextGap > bestNextGap && nextGap > 0.9 * complementarityGap_ && doCorrector
               && !quadraticObj && !allowIncreasingGap) {
#ifdef SOME_DEBUG
          printf("checkGood phase 1 next gap %.18g, phase 0 %.18g, old gap %.18g\n",
                 nextGap, bestNextGap, complementarityGap_);
#endif
          return false;
     } else {
          returnGap = nextGap;
     }
     CoinWorkDouble step;
     if (actualDualStep_ > actualPrimalStep_) {
          step = actualDualStep_;
     } else {
          step = actualPrimalStep_;
     }
     CoinWorkDouble testValue = 1.0 - step * (1.0 - beta3);
     //testValue=0.0;
     testValue *= complementarityGap_;
     if (nextGap < testValue) {
          //std::cout <<"predicted duality gap "<<nextGap<<std::endl;
          goodMove = true;
     } else if(doCorrector) {
          //if (actualDualStep_<actualPrimalStep_) {
          //step=actualDualStep_;
          //} else {
          //step=actualPrimalStep_;
          //}
          CoinWorkDouble gap = bestNextGap;
          goodMove = checkGoodMove2(step, gap, allowIncreasingGap);
          if (goodMove)
               returnGap = gap;
     } else {
          goodMove = true;
     }
     if (goodMove)
          goodMove = checkGoodMove2(step, bestNextGap, allowIncreasingGap);
     // Say good if small
     //if (quadraticObj) {
     if (CoinMax(actualDualStep_, actualPrimalStep_) < 1.0e-6)
          goodMove = true;
     if (!goodMove) {
          //try smaller of two
          if (actualDualStep_ < actualPrimalStep_) {
               step = actualDualStep_;
          } else {
               step = actualPrimalStep_;
          }
          if (step > 1.0) {
               step = 1.0;
          }
          actualPrimalStep_ = step;
          //if (quadraticObj)
          //actualPrimalStep_ *=0.5;
          actualDualStep_ = step;
          goodMove = checkGoodMove2(step, bestNextGap, allowIncreasingGap);
          int pass = 0;
          while (!goodMove) {
               pass++;
               CoinWorkDouble gap = bestNextGap;
               goodMove = checkGoodMove2(step, gap, allowIncreasingGap);
               if (goodMove || pass > 3) {
                    returnGap = gap;
                    break;
               }
               if (step < 1.0e-4) {
                    break;
               }
               step *= 0.5;
               actualPrimalStep_ = step;
               //if (quadraticObj)
               //actualPrimalStep_ *=0.5;
               actualDualStep_ = step;
          } /* endwhile */
          if (doCorrector) {
               //say bad move if both small
               if (numberIterations_ & 1) {
                    if (actualPrimalStep_ < 1.0e-2 && actualDualStep_ < 1.0e-2) {
                         goodMove = false;
                    }
               } else {
                    if (actualPrimalStep_ < 1.0e-5 && actualDualStep_ < 1.0e-5) {
                         goodMove = false;
                    }
                    if (actualPrimalStep_ * actualDualStep_ < 1.0e-20) {
                         goodMove = false;
                    }
               }
          }
     }
     if (goodMove) {
          //compute delta in objectives
          CoinWorkDouble deltaObjectivePrimal = 0.0;
          CoinWorkDouble deltaObjectiveDual =
               innerProduct(deltaY_, numberRows_,
                            rhsFixRegion_);
          CoinWorkDouble error = 0.0;
          CoinWorkDouble * workArray = workArray_;
          CoinZeroN(workArray, numberColumns_);
          CoinMemcpyN(deltaY_, numberRows_, workArray + numberColumns_);
          matrix_->transposeTimes(-1.0, deltaY_, workArray);
          //CoinWorkDouble sumPerturbCost=0.0;
          for (int iColumn = 0; iColumn < numberTotal; iColumn++) {
               if (!flagged(iColumn)) {
                    if (lowerBound(iColumn)) {
                         //sumPerturbCost+=deltaX_[iColumn];
                         deltaObjectiveDual += deltaZ_[iColumn] * lower_[iColumn];
                    }
                    if (upperBound(iColumn)) {
                         //sumPerturbCost-=deltaX_[iColumn];
                         deltaObjectiveDual -= deltaW_[iColumn] * upper_[iColumn];
                    }
                    CoinWorkDouble change = CoinAbs(workArray_[iColumn] - deltaZ_[iColumn] + deltaW_[iColumn]);
                    error = CoinMax(change, error);
               }
               deltaObjectivePrimal += cost_[iColumn] * deltaX_[iColumn];
          }
          //deltaObjectivePrimal+=sumPerturbCost*linearPerturbation_;
          CoinWorkDouble testValue;
          if (error > 0.0) {
               testValue = 1.0e1 * CoinMax(maximumDualError_, 1.0e-12) / error;
          } else {
               testValue = 1.0e1;
          }
          // If quadratic then primal step may compensate
          if (testValue < actualDualStep_ && !quadraticObj) {
               handler_->message(CLP_BARRIER_REDUCING, messages_)
                         << "dual" << static_cast<double>(actualDualStep_)
                         << static_cast<double>(testValue)
                         << CoinMessageEol;
               actualDualStep_ = testValue;
          }
     }
     if (maximumRHSError_ < 1.0e1 * solutionNorm_ * primalTolerance()
               && maximumRHSChange_ > 1.0e-16 * solutionNorm_) {
          //check change in AX not too much
          //??? could be dropped row going infeasible
          CoinWorkDouble ratio = 1.0e1 * CoinMax(maximumRHSError_, 1.0e-12) / maximumRHSChange_;
          if (ratio < actualPrimalStep_) {
               handler_->message(CLP_BARRIER_REDUCING, messages_)
                         << "primal" << static_cast<double>(actualPrimalStep_)
                         << static_cast<double>(ratio)
                         << CoinMessageEol;
               if (ratio > 1.0e-6) {
                    actualPrimalStep_ = ratio;
               } else {
                    actualPrimalStep_ = ratio;
                    //std::cout <<"sign we should be stopping"<<std::endl;
               }
          }
     }
     if (goodMove)
          bestNextGap = returnGap;
     return goodMove;
}
//:  checks for one step size
bool ClpPredictorCorrector::checkGoodMove2(CoinWorkDouble move,
          CoinWorkDouble & bestNextGap,
          bool allowIncreasingGap)
{
     CoinWorkDouble complementarityMultiplier = 1.0 / numberComplementarityPairs_;
     const CoinWorkDouble gamma = 1.0e-8;
     const CoinWorkDouble gammap = 1.0e-8;
     CoinWorkDouble gammad = 1.0e-8;
     int nextNumber;
     int nextNumberItems;
     CoinWorkDouble nextGap = complementarityGap(nextNumber, nextNumberItems, 2);
     if (nextGap > bestNextGap && !allowIncreasingGap)
          return false;
     CoinWorkDouble lowerBoundGap = gamma * nextGap * complementarityMultiplier;
     bool goodMove = true;
     int iColumn;
     for ( iColumn = 0; iColumn < numberRows_ + numberColumns_; iColumn++) {
          if (!flagged(iColumn)) {
               if (lowerBound(iColumn)) {
                    CoinWorkDouble part1 = lowerSlack_[iColumn] + actualPrimalStep_ * deltaSL_[iColumn];
                    CoinWorkDouble part2 = zVec_[iColumn] + actualDualStep_ * deltaZ_[iColumn];
                    if (part1 * part2 < lowerBoundGap) {
                         goodMove = false;
                         break;
                    }
               }
               if (upperBound(iColumn)) {
                    CoinWorkDouble part1 = upperSlack_[iColumn] + actualPrimalStep_ * deltaSU_[iColumn];
                    CoinWorkDouble part2 = wVec_[iColumn] + actualDualStep_ * deltaW_[iColumn];
                    if (part1 * part2 < lowerBoundGap) {
                         goodMove = false;
                         break;
                    }
               }
          }
     }
     CoinWorkDouble * nextDj = NULL;
     CoinWorkDouble maximumDualError = maximumDualError_;
#ifndef NO_RTTI
     ClpQuadraticObjective * quadraticObj = (dynamic_cast< ClpQuadraticObjective*>(objective_));
#else
     ClpQuadraticObjective * quadraticObj = NULL;
     if (objective_->type() == 2)
          quadraticObj = (static_cast< ClpQuadraticObjective*>(objective_));
#endif
     CoinWorkDouble * dualArray = reinterpret_cast<CoinWorkDouble *>(dual_);
     if (quadraticObj) {
          // change gammad
          gammad = 1.0e-4;
          CoinWorkDouble gamma2 = gamma_ * gamma_;
          nextDj = new CoinWorkDouble [numberColumns_];
          CoinWorkDouble * nextSolution = new CoinWorkDouble [numberColumns_];
          // put next primal into nextSolution
          for ( iColumn = 0; iColumn < numberColumns_; iColumn++) {
               if (!flagged(iColumn)) {
                    nextSolution[iColumn] = solution_[iColumn] +
                                            actualPrimalStep_ * deltaX_[iColumn];
               } else {
                    nextSolution[iColumn] = solution_[iColumn];
               }
          }
          // do reduced costs
          CoinMemcpyN(cost_, numberColumns_, nextDj);
          matrix_->transposeTimes(-1.0, dualArray, nextDj);
          matrix_->transposeTimes(-actualDualStep_, deltaY_, nextDj);
          quadraticDjs(nextDj, nextSolution, 1.0);
          delete [] nextSolution;
          CoinPackedMatrix * quadratic = quadraticObj->quadraticObjective();
          const int * columnQuadraticLength = quadratic->getVectorLengths();
          for (int iColumn = 0; iColumn < numberColumns_; iColumn++) {
               if (!fixedOrFree(iColumn)) {
                    CoinWorkDouble newZ = 0.0;
                    CoinWorkDouble newW = 0.0;
                    if (lowerBound(iColumn)) {
                         newZ = zVec_[iColumn] + actualDualStep_ * deltaZ_[iColumn];
                    }
                    if (upperBound(iColumn)) {
                         newW = wVec_[iColumn] + actualDualStep_ * deltaW_[iColumn];
                    }
                    if (columnQuadraticLength[iColumn]) {
                         CoinWorkDouble gammaTerm = gamma2;
                         if (primalR_)
                              gammaTerm += primalR_[iColumn];
                         //CoinWorkDouble dualInfeasibility=
                         //dj_[iColumn]-zVec_[iColumn]+wVec_[iColumn]
                         //+gammaTerm*solution_[iColumn];
                         CoinWorkDouble newInfeasibility =
                              nextDj[iColumn] - newZ + newW
                              + gammaTerm * (solution_[iColumn] + actualPrimalStep_ * deltaX_[iColumn]);
                         maximumDualError = CoinMax(maximumDualError, newInfeasibility);
                         //if (CoinAbs(newInfeasibility)>CoinMax(2000.0*maximumDualError_,1.0e-2)) {
                         //if (dualInfeasibility*newInfeasibility<0.0) {
                         //  printf("%d current %g next %g\n",iColumn,dualInfeasibility,
                         //       newInfeasibility);
                         //  goodMove=false;
                         //}
                         //}
                    }
               }
          }
          delete [] nextDj;
     }
//      Satisfy g_p(alpha)?
     if (rhsNorm_ > solutionNorm_) {
          solutionNorm_ = rhsNorm_;
     }
     CoinWorkDouble errorCheck = maximumRHSError_ / solutionNorm_;
     if (errorCheck < maximumBoundInfeasibility_) {
          errorCheck = maximumBoundInfeasibility_;
     }
     // scale back move
     move = CoinMin(move, 0.95);
     //scale
     if ((1.0 - move)*errorCheck > primalTolerance()) {
          if (nextGap < gammap*(1.0 - move)*errorCheck) {
               goodMove = false;
          }
     }
     //      Satisfy g_d(alpha)?
     errorCheck = maximumDualError / objectiveNorm_;
     if ((1.0 - move)*errorCheck > dualTolerance()) {
          if (nextGap < gammad*(1.0 - move)*errorCheck) {
               goodMove = false;
          }
     }
     if (goodMove)
          bestNextGap = nextGap;
     return goodMove;
}
// updateSolution.  Updates solution at end of iteration
//returns number fixed
int ClpPredictorCorrector::updateSolution(CoinWorkDouble /*nextGap*/)
{
     CoinWorkDouble * dualArray = reinterpret_cast<CoinWorkDouble *>(dual_);
     int numberTotal = numberRows_ + numberColumns_;
     //update pi
     multiplyAdd(deltaY_, numberRows_, actualDualStep_, dualArray, 1.0);
     CoinZeroN(errorRegion_, numberRows_);
     CoinZeroN(rhsFixRegion_, numberRows_);
     CoinWorkDouble maximumRhsInfeasibility = 0.0;
     CoinWorkDouble maximumBoundInfeasibility = 0.0;
     CoinWorkDouble maximumDualError = 1.0e-12;
     CoinWorkDouble primalObjectiveValue = 0.0;
     CoinWorkDouble dualObjectiveValue = 0.0;
     CoinWorkDouble solutionNorm = 1.0e-12;
     int numberKilled = 0;
     CoinWorkDouble freeMultiplier = 1.0e6;
     CoinWorkDouble trueNorm = diagonalNorm_ / diagonalScaleFactor_;
     if (freeMultiplier < trueNorm) {
          freeMultiplier = trueNorm;
     }
     if (freeMultiplier > 1.0e12) {
          freeMultiplier = 1.0e12;
     }
     freeMultiplier = 0.5 / freeMultiplier;
     CoinWorkDouble condition = CoinAbs(cholesky_->choleskyCondition());
     bool caution;
     if ((condition < 1.0e10 && trueNorm < 1.0e12) || numberIterations_ < 20) {
          caution = false;
     } else {
          caution = true;
     }
     CoinWorkDouble extra = eExtra;
     const CoinWorkDouble largeFactor = 1.0e2;
     CoinWorkDouble largeGap = largeFactor * solutionNorm_;
     if (largeGap < largeFactor) {
          largeGap = largeFactor;
     }
     CoinWorkDouble dualFake = 0.0;
     CoinWorkDouble dualTolerance =  dblParam_[ClpDualTolerance];
     dualTolerance = dualTolerance / scaleFactor_;
     if (dualTolerance < 1.0e-12) {
          dualTolerance = 1.0e-12;
     }
     CoinWorkDouble offsetObjective = 0.0;
     CoinWorkDouble killTolerance = primalTolerance();
     //CoinWorkDouble nextMu = nextGap/(static_cast<CoinWorkDouble>(2*numberComplementarityPairs_));
     //printf("using gap of %g\n",nextMu);
     //largest allowable ratio of lowerSlack/zVec (etc)
     CoinWorkDouble epsilonBase;
     CoinWorkDouble diagonalLimit;
     if (!caution) {
          epsilonBase = eBase;
          diagonalLimit = eDiagonal;
     } else {
          epsilonBase = eBaseCaution;
          diagonalLimit = eDiagonalCaution;
     }
     CoinWorkDouble maximumDJInfeasibility = 0.0;
     int numberIncreased = 0;
     int numberDecreased = 0;
     CoinWorkDouble largestDiagonal = 0.0;
     CoinWorkDouble smallestDiagonal = 1.0e50;
     CoinWorkDouble largeGap2 = CoinMax(1.0e7, 1.0e2 * solutionNorm_);
     //largeGap2 = 1.0e9;
     // When to start looking at killing (factor0
     CoinWorkDouble killFactor;
#ifndef NO_RTTI
     ClpQuadraticObjective * quadraticObj = (dynamic_cast< ClpQuadraticObjective*>(objective_));
#else
     ClpQuadraticObjective * quadraticObj = NULL;
     if (objective_->type() == 2)
          quadraticObj = (static_cast< ClpQuadraticObjective*>(objective_));
#endif
#ifndef CLP_CAUTION
#define KILL_ITERATION 50
#else
#if CLP_CAUTION < 1
#define KILL_ITERATION 50
#else
#define KILL_ITERATION 100
#endif
#endif
     if (!quadraticObj || 1) {
          if (numberIterations_ < KILL_ITERATION) {
               killFactor = 1.0;
          } else if (numberIterations_ < 2 * KILL_ITERATION) {
               killFactor = 5.0;
               stepLength_ = CoinMax(stepLength_, 0.9995);
          } else if (numberIterations_ < 4 * KILL_ITERATION) {
               killFactor = 20.0;
               stepLength_ = CoinMax(stepLength_, 0.99995);
          } else {
               killFactor = 1.0e2;
               stepLength_ = CoinMax(stepLength_, 0.999995);
          }
     } else {
          killFactor = 1.0;
     }
     // put next primal into deltaSL_
     int iColumn;
     int iRow;
     for (iColumn = 0; iColumn < numberTotal; iColumn++) {
          CoinWorkDouble thisWeight = deltaX_[iColumn];
          CoinWorkDouble newPrimal = solution_[iColumn] + 1.0 * actualPrimalStep_ * thisWeight;
          deltaSL_[iColumn] = newPrimal;
     }
#if 0
     // nice idea but doesn't work
     multiplyAdd(solution_ + numberColumns_, numberRows_, -1.0, errorRegion_, 0.0);
     matrix_->times(1.0, solution_, errorRegion_);
     multiplyAdd(deltaSL_ + numberColumns_, numberRows_, -1.0, rhsFixRegion_, 0.0);
     matrix_->times(1.0, deltaSL_, rhsFixRegion_);
     CoinWorkDouble newNorm =  maximumAbsElement(deltaSL_, numberTotal);
     CoinWorkDouble tol = newNorm * primalTolerance();
     bool goneInf = false;
     for (iRow = 0; iRow < numberRows_; iRow++) {
          CoinWorkDouble value = errorRegion_[iRow];
          CoinWorkDouble valueNew = rhsFixRegion_[iRow];
          if (CoinAbs(value) < tol && CoinAbs(valueNew) > tol) {
               printf("row %d old %g new %g\n", iRow, value, valueNew);
               goneInf = true;
          }
     }
     if (goneInf) {
          actualPrimalStep_ *= 0.5;
          for (iColumn = 0; iColumn < numberTotal; iColumn++) {
               CoinWorkDouble thisWeight = deltaX_[iColumn];
               CoinWorkDouble newPrimal = solution_[iColumn] + 1.0 * actualPrimalStep_ * thisWeight;
               deltaSL_[iColumn] = newPrimal;
          }
     }
     CoinZeroN(errorRegion_, numberRows_);
     CoinZeroN(rhsFixRegion_, numberRows_);
#endif
     // do reduced costs
     CoinMemcpyN(dualArray, numberRows_, dj_ + numberColumns_);
     CoinMemcpyN(cost_, numberColumns_, dj_);
     CoinWorkDouble quadraticOffset = quadraticDjs(dj_, deltaSL_, 1.0);
     // Save modified costs for fixed variables
     CoinMemcpyN(dj_, numberColumns_, deltaSU_);
     matrix_->transposeTimes(-1.0, dualArray, dj_);
     CoinWorkDouble gamma2 = gamma_ * gamma_; // gamma*gamma will be added to diagonal
     CoinWorkDouble gammaOffset = 0.0;
#if 0
     const CoinBigIndex * columnStart = matrix_->getVectorStarts();
     const int * columnLength = matrix_->getVectorLengths();
     const int * row = matrix_->getIndices();
     const double * element = matrix_->getElements();
#endif
     for (iColumn = 0; iColumn < numberTotal; iColumn++) {
          if (!flagged(iColumn)) {
               CoinWorkDouble reducedCost = dj_[iColumn];
               bool thisKilled = false;
               CoinWorkDouble zValue = zVec_[iColumn] + actualDualStep_ * deltaZ_[iColumn];
               CoinWorkDouble wValue = wVec_[iColumn] + actualDualStep_ * deltaW_[iColumn];
               zVec_[iColumn] = zValue;
               wVec_[iColumn] = wValue;
               CoinWorkDouble thisWeight = deltaX_[iColumn];
               CoinWorkDouble oldPrimal = solution_[iColumn];
               CoinWorkDouble newPrimal = solution_[iColumn] + actualPrimalStep_ * thisWeight;
               CoinWorkDouble dualObjectiveThis = 0.0;
               CoinWorkDouble sUpper = extra;
               CoinWorkDouble sLower = extra;
               CoinWorkDouble kill;
               if (CoinAbs(newPrimal) > 1.0e4) {
                    kill = killTolerance * 1.0e-4 * newPrimal;
               } else {
                    kill = killTolerance;
               }
               kill *= 1.0e-3; //be conservative
               CoinWorkDouble smallerSlack = COIN_DBL_MAX;
               bool fakeOldBounds = false;
               bool fakeNewBounds = false;
               CoinWorkDouble trueLower;
               CoinWorkDouble trueUpper;
               if (iColumn < numberColumns_) {
                    trueLower = columnLower_[iColumn];
                    trueUpper = columnUpper_[iColumn];
               } else {
                    trueLower = rowLower_[iColumn-numberColumns_];
                    trueUpper = rowUpper_[iColumn-numberColumns_];
               }
               if (oldPrimal > trueLower + largeGap2 &&
                         oldPrimal < trueUpper - largeGap2)
                    fakeOldBounds = true;
               if (newPrimal > trueLower + largeGap2 &&
                         newPrimal < trueUpper - largeGap2)
                    fakeNewBounds = true;
               if (fakeOldBounds) {
                    if (fakeNewBounds) {
                         lower_[iColumn] = newPrimal - largeGap2;
                         lowerSlack_[iColumn] = largeGap2;
                         upper_[iColumn] = newPrimal + largeGap2;
                         upperSlack_[iColumn] = largeGap2;
                    } else {
                         lower_[iColumn] = trueLower;
                         setLowerBound(iColumn);
                         lowerSlack_[iColumn] = CoinMax(newPrimal - trueLower, 1.0);
                         upper_[iColumn] = trueUpper;
                         setUpperBound(iColumn);
                         upperSlack_[iColumn] = CoinMax(trueUpper - newPrimal, 1.0);
                    }
               } else if (fakeNewBounds) {
                    lower_[iColumn] = newPrimal - largeGap2;
                    lowerSlack_[iColumn] = largeGap2;
                    upper_[iColumn] = newPrimal + largeGap2;
                    upperSlack_[iColumn] = largeGap2;
                    // so we can just have one test
                    fakeOldBounds = true;
               }
               CoinWorkDouble lowerBoundInfeasibility = 0.0;
               CoinWorkDouble upperBoundInfeasibility = 0.0;
               //double saveNewPrimal = newPrimal;
               if (lowerBound(iColumn)) {
                    CoinWorkDouble oldSlack = lowerSlack_[iColumn];
                    CoinWorkDouble newSlack;
                    newSlack =
                         lowerSlack_[iColumn] + actualPrimalStep_ * (oldPrimal - oldSlack
                                   + thisWeight - lower_[iColumn]);
                    if (fakeOldBounds)
                         newSlack = lowerSlack_[iColumn];
                    CoinWorkDouble epsilon = CoinAbs(newSlack) * epsilonBase;
                    epsilon = CoinMin(epsilon, 1.0e-5);
                    //epsilon=1.0e-14;
                    //make sure reasonable
                    if (zValue < epsilon) {
                         zValue = epsilon;
                    }
                    CoinWorkDouble feasibleSlack = newPrimal - lower_[iColumn];
                    if (feasibleSlack > 0.0 && newSlack > 0.0) {
                         CoinWorkDouble larger;
                         if (newSlack > feasibleSlack) {
                              larger = newSlack;
                         } else {
                              larger = feasibleSlack;
                         }
                         if (CoinAbs(feasibleSlack - newSlack) < 1.0e-6 * larger) {
                              newSlack = feasibleSlack;
                         }
                    }
                    if (zVec_[iColumn] > dualTolerance) {
                         dualObjectiveThis += lower_[iColumn] * zVec_[iColumn];
                    }
                    lowerSlack_[iColumn] = newSlack;
                    if (newSlack < smallerSlack) {
                         smallerSlack = newSlack;
                    }
                    lowerBoundInfeasibility = CoinAbs(newPrimal - lowerSlack_[iColumn] - lower_[iColumn]);
                    if (lowerSlack_[iColumn] <= kill * killFactor && CoinAbs(newPrimal - lower_[iColumn]) <= kill * killFactor) {
                         CoinWorkDouble step = CoinMin(actualPrimalStep_ * 1.1, 1.0);
                         CoinWorkDouble newPrimal2 = solution_[iColumn] + step * thisWeight;
                         if (newPrimal2 < newPrimal && dj_[iColumn] > 1.0e-5 && numberIterations_ > 50 - 40) {
                              newPrimal = lower_[iColumn];
                              lowerSlack_[iColumn] = 0.0;
                              //printf("fixing %d to lower\n",iColumn);
                         }
                    }
                    if (lowerSlack_[iColumn] <= kill && CoinAbs(newPrimal - lower_[iColumn]) <= kill) {
                         //may be better to leave at value?
                         newPrimal = lower_[iColumn];
                         lowerSlack_[iColumn] = 0.0;
                         thisKilled = true;
                         //cout<<j<<" l "<<reducedCost<<" "<<zVec_[iColumn]<<endl;
                    } else {
                         sLower += lowerSlack_[iColumn];
                    }
               }
               if (upperBound(iColumn)) {
                    CoinWorkDouble oldSlack = upperSlack_[iColumn];
                    CoinWorkDouble newSlack;
                    newSlack =
                         upperSlack_[iColumn] + actualPrimalStep_ * (-oldPrimal - oldSlack
                                   - thisWeight + upper_[iColumn]);
                    if (fakeOldBounds)
                         newSlack = upperSlack_[iColumn];
                    CoinWorkDouble epsilon = CoinAbs(newSlack) * epsilonBase;
                    epsilon = CoinMin(epsilon, 1.0e-5);
                    //make sure reasonable
                    //epsilon=1.0e-14;
                    if (wValue < epsilon) {
                         wValue = epsilon;
                    }
                    CoinWorkDouble feasibleSlack = upper_[iColumn] - newPrimal;
                    if (feasibleSlack > 0.0 && newSlack > 0.0) {
                         CoinWorkDouble larger;
                         if (newSlack > feasibleSlack) {
                              larger = newSlack;
                         } else {
                              larger = feasibleSlack;
                         }
                         if (CoinAbs(feasibleSlack - newSlack) < 1.0e-6 * larger) {
                              newSlack = feasibleSlack;
                         }
                    }
                    if (wVec_[iColumn] > dualTolerance) {
                         dualObjectiveThis -= upper_[iColumn] * wVec_[iColumn];
                    }
                    upperSlack_[iColumn] = newSlack;
                    if (newSlack < smallerSlack) {
                         smallerSlack = newSlack;
                    }
                    upperBoundInfeasibility = CoinAbs(newPrimal + upperSlack_[iColumn] - upper_[iColumn]);
                    if (upperSlack_[iColumn] <= kill * killFactor && CoinAbs(newPrimal - upper_[iColumn]) <= kill * killFactor) {
                         CoinWorkDouble step = CoinMin(actualPrimalStep_ * 1.1, 1.0);
                         CoinWorkDouble newPrimal2 = solution_[iColumn] + step * thisWeight;
                         if (newPrimal2 > newPrimal && dj_[iColumn] < -1.0e-5 && numberIterations_ > 50 - 40) {
                              newPrimal = upper_[iColumn];
                              upperSlack_[iColumn] = 0.0;
                              //printf("fixing %d to upper\n",iColumn);
                         }
                    }
                    if (upperSlack_[iColumn] <= kill && CoinAbs(newPrimal - upper_[iColumn]) <= kill) {
                         //may be better to leave at value?
                         newPrimal = upper_[iColumn];
                         upperSlack_[iColumn] = 0.0;
                         thisKilled = true;
                    } else {
                         sUpper += upperSlack_[iColumn];
                    }
               }
#if 0
               if (newPrimal != saveNewPrimal && iColumn < numberColumns_) {
                    // adjust slacks
                    double movement = newPrimal - saveNewPrimal;
                    for (CoinBigIndex j = columnStart[iColumn];
                              j < columnStart[iColumn] + columnLength[iColumn]; j++) {
                         int iRow = row[j];
                         double slackMovement = element[j] * movement;
                         solution_[iRow+numberColumns_] += slackMovement; // sign?
                    }
               }
#endif
               solution_[iColumn] = newPrimal;
               if (CoinAbs(newPrimal) > solutionNorm) {
                    solutionNorm = CoinAbs(newPrimal);
               }
               if (!thisKilled) {
                    CoinWorkDouble gammaTerm = gamma2;
                    if (primalR_) {
                         gammaTerm += primalR_[iColumn];
                         quadraticOffset += newPrimal * newPrimal * primalR_[iColumn];
                    }
                    CoinWorkDouble dualInfeasibility =
                         reducedCost - zVec_[iColumn] + wVec_[iColumn] + gammaTerm * newPrimal;
                    if (CoinAbs(dualInfeasibility) > dualTolerance) {
#if 0
                         if (dualInfeasibility > 0.0) {
                              // To improve we could reduce t and/or increase z
                              if (lowerBound(iColumn)) {
                                   CoinWorkDouble complementarity = zVec_[iColumn] * lowerSlack_[iColumn];
                                   if (complementarity < nextMu) {
                                        CoinWorkDouble change =
                                             CoinMin(dualInfeasibility,
                                                     (nextMu - complementarity) / lowerSlack_[iColumn]);
                                        dualInfeasibility -= change;
                                        COIN_DETAIL_PRINT(printf("%d lb locomp %g - dual inf from %g to %g\n",
                                               iColumn, complementarity, dualInfeasibility + change,
								 dualInfeasibility));
                                        zVec_[iColumn] += change;
                                        zValue = CoinMax(zVec_[iColumn], 1.0e-12);
                                   }
                              }
                              if (upperBound(iColumn)) {
                                   CoinWorkDouble complementarity = wVec_[iColumn] * upperSlack_[iColumn];
                                   if (complementarity > nextMu) {
                                        CoinWorkDouble change =
                                             CoinMin(dualInfeasibility,
                                                     (complementarity - nextMu) / upperSlack_[iColumn]);
                                        dualInfeasibility -= change;
                                        COIN_DETAIL_PRINT(printf("%d ub hicomp %g - dual inf from %g to %g\n",
                                               iColumn, complementarity, dualInfeasibility + change,
								 dualInfeasibility));
                                        wVec_[iColumn] -= change;
                                        wValue = CoinMax(wVec_[iColumn], 1.0e-12);
                                   }
                              }
                         } else {
                              // To improve we could reduce z and/or increase t
                              if (lowerBound(iColumn)) {
                                   CoinWorkDouble complementarity = zVec_[iColumn] * lowerSlack_[iColumn];
                                   if (complementarity > nextMu) {
                                        CoinWorkDouble change =
                                             CoinMax(dualInfeasibility,
                                                     (nextMu - complementarity) / lowerSlack_[iColumn]);
                                        dualInfeasibility -= change;
                                        COIN_DETAIL_PRINT(printf("%d lb hicomp %g - dual inf from %g to %g\n",
                                               iColumn, complementarity, dualInfeasibility + change,
								 dualInfeasibility));
                                        zVec_[iColumn] += change;
                                        zValue = CoinMax(zVec_[iColumn], 1.0e-12);
                                   }
                              }
                              if (upperBound(iColumn)) {
                                   CoinWorkDouble complementarity = wVec_[iColumn] * upperSlack_[iColumn];
                                   if (complementarity < nextMu) {
                                        CoinWorkDouble change =
                                             CoinMax(dualInfeasibility,
                                                     (complementarity - nextMu) / upperSlack_[iColumn]);
                                        dualInfeasibility -= change;
                                        COIN_DETAIL_PRINT(printf("%d ub locomp %g - dual inf from %g to %g\n",
                                               iColumn, complementarity, dualInfeasibility + change,
								 dualInfeasibility));
                                        wVec_[iColumn] -= change;
                                        wValue = CoinMax(wVec_[iColumn], 1.0e-12);
                                   }
                              }
                         }
#endif
                         dualFake += newPrimal * dualInfeasibility;
                    }
                    if (lowerBoundInfeasibility > maximumBoundInfeasibility) {
                         maximumBoundInfeasibility = lowerBoundInfeasibility;
                    }
                    if (upperBoundInfeasibility > maximumBoundInfeasibility) {
                         maximumBoundInfeasibility = upperBoundInfeasibility;
                    }
                    dualInfeasibility = CoinAbs(dualInfeasibility);
                    if (dualInfeasibility > maximumDualError) {
                         //printf("bad dual %d %g\n",iColumn,
                         // reducedCost-zVec_[iColumn]+wVec_[iColumn]+gammaTerm*newPrimal);
                         maximumDualError = dualInfeasibility;
                    }
                    dualObjectiveValue += dualObjectiveThis;
                    gammaOffset += newPrimal * newPrimal;
                    if (sLower > largeGap) {
                         sLower = largeGap;
                    }
                    if (sUpper > largeGap) {
                         sUpper = largeGap;
                    }
#if 1
                    CoinWorkDouble divisor = sLower * wValue + sUpper * zValue + gammaTerm * sLower * sUpper;
                    CoinWorkDouble diagonalValue = (sUpper * sLower) / divisor;
#else
                    CoinWorkDouble divisor = sLower * wValue + sUpper * zValue + gammaTerm * sLower * sUpper;
                    CoinWorkDouble diagonalValue2 = (sUpper * sLower) / divisor;
                    CoinWorkDouble diagonalValue;
                    if (!lowerBound(iColumn)) {
                         diagonalValue = wValue / sUpper + gammaTerm;
                    } else if (!upperBound(iColumn)) {
                         diagonalValue = zValue / sLower + gammaTerm;
                    } else {
                         diagonalValue = zValue / sLower + wValue / sUpper + gammaTerm;
                    }
                    diagonalValue = 1.0 / diagonalValue;
#endif
                    diagonal_[iColumn] = diagonalValue;
                    //FUDGE
                    if (diagonalValue > diagonalLimit) {
#ifdef COIN_DEVELOP
                         std::cout << "large diagonal " << diagonalValue << std::endl;
#endif
                         diagonal_[iColumn] = diagonalLimit;
                    }
#ifdef COIN_DEVELOP
                    if (diagonalValue < 1.0e-10) {
                         //std::cout<<"small diagonal "<<diagonalValue<<std::endl;
                    }
#endif
                    if (diagonalValue > largestDiagonal) {
                         largestDiagonal = diagonalValue;
                    }
                    if (diagonalValue < smallestDiagonal) {
                         smallestDiagonal = diagonalValue;
                    }
                    deltaX_[iColumn] = 0.0;
               } else {
                    numberKilled++;
                    if (solution_[iColumn] != lower_[iColumn] &&
                              solution_[iColumn] != upper_[iColumn]) {
                         COIN_DETAIL_PRINT(printf("%d %g %g %g\n", iColumn, static_cast<double>(lower_[iColumn]),
						  static_cast<double>(solution_[iColumn]), static_cast<double>(upper_[iColumn])));
                    }
                    diagonal_[iColumn] = 0.0;
                    zVec_[iColumn] = 0.0;
                    wVec_[iColumn] = 0.0;
                    setFlagged(iColumn);
                    setFixedOrFree(iColumn);
                    deltaX_[iColumn] = newPrimal;
                    offsetObjective += newPrimal * deltaSU_[iColumn];
               }
          } else {
               deltaX_[iColumn] = solution_[iColumn];
               diagonal_[iColumn] = 0.0;
               offsetObjective += solution_[iColumn] * deltaSU_[iColumn];
               if (upper_[iColumn] - lower_[iColumn] > 1.0e-5) {
                    if (solution_[iColumn] < lower_[iColumn] + 1.0e-8 && dj_[iColumn] < -1.0e-8) {
                         if (-dj_[iColumn] > maximumDJInfeasibility) {
                              maximumDJInfeasibility = -dj_[iColumn];
                         }
                    }
                    if (solution_[iColumn] > upper_[iColumn] - 1.0e-8 && dj_[iColumn] > 1.0e-8) {
                         if (dj_[iColumn] > maximumDJInfeasibility) {
                              maximumDJInfeasibility = dj_[iColumn];
                         }
                    }
               }
          }
          primalObjectiveValue += solution_[iColumn] * cost_[iColumn];
     }
     handler_->message(CLP_BARRIER_DIAGONAL, messages_)
               << static_cast<double>(largestDiagonal) << static_cast<double>(smallestDiagonal)
               << CoinMessageEol;
#if 0
     // If diagonal wild - kill some
     if (largestDiagonal > 1.0e17 * smallestDiagonal) {
          CoinWorkDouble killValue = largestDiagonal * 1.0e-17;
          for (int iColumn = 0; iColumn < numberTotal; iColumn++) {
               if (CoinAbs(diagonal_[iColumn]) < killValue)
                    diagonal_[iolumn] = 0.0;
          }
     }
#endif
     // update rhs region
     multiplyAdd(deltaX_ + numberColumns_, numberRows_, -1.0, rhsFixRegion_, 1.0);
     matrix_->times(1.0, deltaX_, rhsFixRegion_);
     primalObjectiveValue += 0.5 * gamma2 * gammaOffset + 0.5 * quadraticOffset;
     if (quadraticOffset) {
          //  printf("gamma offset %g %g, quadoffset %g\n",gammaOffset,gamma2*gammaOffset,quadraticOffset);
     }

     dualObjectiveValue += offsetObjective + dualFake;
     dualObjectiveValue -= 0.5 * gamma2 * gammaOffset + 0.5 * quadraticOffset;
     if (numberIncreased || numberDecreased) {
          handler_->message(CLP_BARRIER_SLACKS, messages_)
                    << numberIncreased << numberDecreased
                    << CoinMessageEol;
     }
     if (maximumDJInfeasibility) {
          handler_->message(CLP_BARRIER_DUALINF, messages_)
                    << static_cast<double>(maximumDJInfeasibility)
                    << CoinMessageEol;
     }
     // Need to rethink (but it is only for printing)
     sumPrimalInfeasibilities_ = maximumRhsInfeasibility;
     sumDualInfeasibilities_ = maximumDualError;
     maximumBoundInfeasibility_ = maximumBoundInfeasibility;
     //compute error and fixed RHS
     multiplyAdd(solution_ + numberColumns_, numberRows_, -1.0, errorRegion_, 0.0);
     matrix_->times(1.0, solution_, errorRegion_);
     maximumDualError_ = maximumDualError;
     maximumBoundInfeasibility_ = maximumBoundInfeasibility;
     solutionNorm_ = solutionNorm;
     //finish off objective computation
     primalObjective_ = primalObjectiveValue * scaleFactor_;
     CoinWorkDouble dualValue2 = innerProduct(dualArray, numberRows_,
                                 rhsFixRegion_);
     dualObjectiveValue -= dualValue2;
     dualObjective_ = dualObjectiveValue * scaleFactor_;
     if (numberKilled) {
          handler_->message(CLP_BARRIER_KILLED, messages_)
                    << numberKilled
                    << CoinMessageEol;
     }
     CoinWorkDouble maximumRHSError1 = 0.0;
     CoinWorkDouble maximumRHSError2 = 0.0;
     CoinWorkDouble primalOffset = 0.0;
     char * dropped = cholesky_->rowsDropped();
     for (iRow = 0; iRow < numberRows_; iRow++) {
          CoinWorkDouble value = errorRegion_[iRow];
          if (!dropped[iRow]) {
               if (CoinAbs(value) > maximumRHSError1) {
                    maximumRHSError1 = CoinAbs(value);
               }
          } else {
               if (CoinAbs(value) > maximumRHSError2) {
                    maximumRHSError2 = CoinAbs(value);
               }
               primalOffset += value * dualArray[iRow];
          }
     }
     primalObjective_ -= primalOffset * scaleFactor_;
     if (maximumRHSError1 > maximumRHSError2) {
          maximumRHSError_ = maximumRHSError1;
     } else {
          maximumRHSError_ = maximumRHSError1; //note change
          if (maximumRHSError2 > primalTolerance()) {
               handler_->message(CLP_BARRIER_ABS_DROPPED, messages_)
                         << static_cast<double>(maximumRHSError2)
                         << CoinMessageEol;
          }
     }
     objectiveNorm_ = maximumAbsElement(dualArray, numberRows_);
     if (objectiveNorm_ < 1.0e-12) {
          objectiveNorm_ = 1.0e-12;
     }
     if (objectiveNorm_ < baseObjectiveNorm_) {
          //std::cout<<" base "<<baseObjectiveNorm_<<" "<<objectiveNorm_<<std::endl;
          if (objectiveNorm_ < baseObjectiveNorm_ * 1.0e-4) {
               objectiveNorm_ = baseObjectiveNorm_ * 1.0e-4;
          }
     }
     bool primalFeasible = true;
     if (maximumRHSError_ > primalTolerance() ||
               maximumDualError_ > dualTolerance / scaleFactor_) {
          handler_->message(CLP_BARRIER_ABS_ERROR, messages_)
                    << static_cast<double>(maximumRHSError_) << static_cast<double>(maximumDualError_)
                    << CoinMessageEol;
     }
     if (rhsNorm_ > solutionNorm_) {
          solutionNorm_ = rhsNorm_;
     }
     CoinWorkDouble scaledRHSError = maximumRHSError_ / (solutionNorm_ + 10.0);
     bool dualFeasible = true;
#if KEEP_GOING_IF_FIXED > 5
     if (maximumBoundInfeasibility_ > primalTolerance() ||
               scaledRHSError > primalTolerance())
          primalFeasible = false;
#else
     if (maximumBoundInfeasibility_ > primalTolerance() ||
               scaledRHSError > CoinMax(CoinMin(100.0 * primalTolerance(), 1.0e-5),
                                        primalTolerance()))
          primalFeasible = false;
#endif
     // relax dual test if obj big and gap smallish
     CoinWorkDouble gap = CoinAbs(primalObjective_ - dualObjective_);
     CoinWorkDouble sizeObj = CoinMin(CoinAbs(primalObjective_), CoinAbs(dualObjective_)) + 1.0e-50;
     //printf("gap %g sizeObj %g ratio %g comp %g\n",
     //     gap,sizeObj,gap/sizeObj,complementarityGap_);
     if (numberIterations_ > 100 && gap / sizeObj < 1.0e-9 && complementarityGap_ < 1.0e-7 * sizeObj)
          dualTolerance *= 1.0e2;
     if (maximumDualError_ > objectiveNorm_ * dualTolerance)
          dualFeasible = false;
     if (!primalFeasible || !dualFeasible) {
          handler_->message(CLP_BARRIER_FEASIBLE, messages_)
                    << static_cast<double>(maximumBoundInfeasibility_) << static_cast<double>(scaledRHSError)
                    << static_cast<double>(maximumDualError_ / objectiveNorm_)
                    << CoinMessageEol;
     }
     if (!gonePrimalFeasible_) {
          gonePrimalFeasible_ = primalFeasible;
     } else if (!primalFeasible) {
          gonePrimalFeasible_ = primalFeasible;
          if (!numberKilled) {
               handler_->message(CLP_BARRIER_GONE_INFEASIBLE, messages_)
                         << CoinMessageEol;
          }
     }
     if (!goneDualFeasible_) {
          goneDualFeasible_ = dualFeasible;
     } else if (!dualFeasible) {
          handler_->message(CLP_BARRIER_GONE_INFEASIBLE, messages_)
                    << CoinMessageEol;
          goneDualFeasible_ = dualFeasible;
     }
     //objectiveValue();
     if (solutionNorm_ > 1.0e40) {
          std::cout << "primal off to infinity" << std::endl;
          abort();
     }
     if (objectiveNorm_ > 1.0e40) {
          std::cout << "dual off to infinity" << std::endl;
          abort();
     }
     handler_->message(CLP_BARRIER_STEP, messages_)
               << static_cast<double>(actualPrimalStep_)
               << static_cast<double>(actualDualStep_)
               << static_cast<double>(mu_)
               << CoinMessageEol;
     numberIterations_++;
     return numberKilled;
}
//  Save info on products of affine deltaSU*deltaW and deltaSL*deltaZ
CoinWorkDouble
ClpPredictorCorrector::affineProduct()
{
     CoinWorkDouble product = 0.0;
     //IF zVec starts as 0 then deltaZ always zero
     //(remember if free then zVec not 0)
     //I think free can be done with careful use of boundSlacks to zero
     //out all we want
     for (int iColumn = 0; iColumn < numberRows_ + numberColumns_; iColumn++) {
          CoinWorkDouble w3 = deltaZ_[iColumn] * deltaX_[iColumn];
          CoinWorkDouble w4 = -deltaW_[iColumn] * deltaX_[iColumn];
          if (lowerBound(iColumn)) {
               w3 += deltaZ_[iColumn] * (solution_[iColumn] - lowerSlack_[iColumn] - lower_[iColumn]);
               product += w3;
          }
          if (upperBound(iColumn)) {
               w4 += deltaW_[iColumn] * (-solution_[iColumn] - upperSlack_[iColumn] + upper_[iColumn]);
               product += w4;
          }
     }
     return product;
}
//See exactly what would happen given current deltas
void
ClpPredictorCorrector::debugMove(int /*phase*/,
                                 CoinWorkDouble primalStep, CoinWorkDouble dualStep)
{
#ifndef SOME_DEBUG
     return;
#endif
     CoinWorkDouble * dualArray = reinterpret_cast<CoinWorkDouble *>(dual_);
     int numberTotal = numberRows_ + numberColumns_;
     CoinWorkDouble * dualNew = ClpCopyOfArray(dualArray, numberRows_);
     CoinWorkDouble * errorRegionNew = new CoinWorkDouble [numberRows_];
     CoinWorkDouble * rhsFixRegionNew = new CoinWorkDouble [numberRows_];
     CoinWorkDouble * primalNew = ClpCopyOfArray(solution_, numberTotal);
     CoinWorkDouble * djNew = new CoinWorkDouble[numberTotal];
     //update pi
     multiplyAdd(deltaY_, numberRows_, dualStep, dualNew, 1.0);
     // do reduced costs
     CoinMemcpyN(dualNew, numberRows_, djNew + numberColumns_);
     CoinMemcpyN(cost_, numberColumns_, djNew);
     matrix_->transposeTimes(-1.0, dualNew, djNew);
     // update x
     int iColumn;
     for (iColumn = 0; iColumn < numberTotal; iColumn++) {
          if (!flagged(iColumn))
               primalNew[iColumn] += primalStep * deltaX_[iColumn];
     }
     CoinWorkDouble quadraticOffset = quadraticDjs(djNew, primalNew, 1.0);
     CoinZeroN(errorRegionNew, numberRows_);
     CoinZeroN(rhsFixRegionNew, numberRows_);
     CoinWorkDouble maximumBoundInfeasibility = 0.0;
     CoinWorkDouble maximumDualError = 1.0e-12;
     CoinWorkDouble primalObjectiveValue = 0.0;
     CoinWorkDouble dualObjectiveValue = 0.0;
     CoinWorkDouble solutionNorm = 1.0e-12;
     const CoinWorkDouble largeFactor = 1.0e2;
     CoinWorkDouble largeGap = largeFactor * solutionNorm_;
     if (largeGap < largeFactor) {
          largeGap = largeFactor;
     }
     CoinWorkDouble dualFake = 0.0;
     CoinWorkDouble dualTolerance =  dblParam_[ClpDualTolerance];
     dualTolerance = dualTolerance / scaleFactor_;
     if (dualTolerance < 1.0e-12) {
          dualTolerance = 1.0e-12;
     }
     CoinWorkDouble newGap = 0.0;
     CoinWorkDouble offsetObjective = 0.0;
     CoinWorkDouble gamma2 = gamma_ * gamma_; // gamma*gamma will be added to diagonal
     CoinWorkDouble gammaOffset = 0.0;
     CoinWorkDouble maximumDjInfeasibility = 0.0;
     for ( iColumn = 0; iColumn < numberTotal; iColumn++) {
          if (!flagged(iColumn)) {
               CoinWorkDouble reducedCost = djNew[iColumn];
               CoinWorkDouble zValue = zVec_[iColumn] + dualStep * deltaZ_[iColumn];
               CoinWorkDouble wValue = wVec_[iColumn] + dualStep * deltaW_[iColumn];
               CoinWorkDouble thisWeight = deltaX_[iColumn];
               CoinWorkDouble oldPrimal = solution_[iColumn];
               CoinWorkDouble newPrimal = primalNew[iColumn];
               CoinWorkDouble lowerBoundInfeasibility = 0.0;
               CoinWorkDouble upperBoundInfeasibility = 0.0;
               if (lowerBound(iColumn)) {
                    CoinWorkDouble oldSlack = lowerSlack_[iColumn];
                    CoinWorkDouble newSlack =
                         lowerSlack_[iColumn] + primalStep * (oldPrimal - oldSlack
                                   + thisWeight - lower_[iColumn]);
                    if (zValue > dualTolerance) {
                         dualObjectiveValue += lower_[iColumn] * zVec_[iColumn];
                    }
                    lowerBoundInfeasibility = CoinAbs(newPrimal - newSlack - lower_[iColumn]);
                    newGap += newSlack * zValue;
               }
               if (upperBound(iColumn)) {
                    CoinWorkDouble oldSlack = upperSlack_[iColumn];
                    CoinWorkDouble newSlack =
                         upperSlack_[iColumn] + primalStep * (-oldPrimal - oldSlack
                                   - thisWeight + upper_[iColumn]);
                    if (wValue > dualTolerance) {
                         dualObjectiveValue -= upper_[iColumn] * wVec_[iColumn];
                    }
                    upperBoundInfeasibility = CoinAbs(newPrimal + newSlack - upper_[iColumn]);
                    newGap += newSlack * wValue;
               }
               if (CoinAbs(newPrimal) > solutionNorm) {
                    solutionNorm = CoinAbs(newPrimal);
               }
               CoinWorkDouble gammaTerm = gamma2;
               if (primalR_) {
                    gammaTerm += primalR_[iColumn];
                    quadraticOffset += newPrimal * newPrimal * primalR_[iColumn];
               }
               CoinWorkDouble dualInfeasibility =
                    reducedCost - zValue + wValue + gammaTerm * newPrimal;
               if (CoinAbs(dualInfeasibility) > dualTolerance) {
                    dualFake += newPrimal * dualInfeasibility;
               }
               if (lowerBoundInfeasibility > maximumBoundInfeasibility) {
                    maximumBoundInfeasibility = lowerBoundInfeasibility;
               }
               if (upperBoundInfeasibility > maximumBoundInfeasibility) {
                    maximumBoundInfeasibility = upperBoundInfeasibility;
               }
               dualInfeasibility = CoinAbs(dualInfeasibility);
               if (dualInfeasibility > maximumDualError) {
                    //printf("bad dual %d %g\n",iColumn,
                    // reducedCost-zVec_[iColumn]+wVec_[iColumn]+gammaTerm*newPrimal);
                    maximumDualError = dualInfeasibility;
               }
               gammaOffset += newPrimal * newPrimal;
               djNew[iColumn] = 0.0;
          } else {
               offsetObjective += primalNew[iColumn] * cost_[iColumn];
               if (upper_[iColumn] - lower_[iColumn] > 1.0e-5) {
                    if (primalNew[iColumn] < lower_[iColumn] + 1.0e-8 && djNew[iColumn] < -1.0e-8) {
                         if (-djNew[iColumn] > maximumDjInfeasibility) {
                              maximumDjInfeasibility = -djNew[iColumn];
                         }
                    }
                    if (primalNew[iColumn] > upper_[iColumn] - 1.0e-8 && djNew[iColumn] > 1.0e-8) {
                         if (djNew[iColumn] > maximumDjInfeasibility) {
                              maximumDjInfeasibility = djNew[iColumn];
                         }
                    }
               }
               djNew[iColumn] = primalNew[iColumn];
          }
          primalObjectiveValue += solution_[iColumn] * cost_[iColumn];
     }
     // update rhs region
     multiplyAdd(djNew + numberColumns_, numberRows_, -1.0, rhsFixRegionNew, 1.0);
     matrix_->times(1.0, djNew, rhsFixRegionNew);
     primalObjectiveValue += 0.5 * gamma2 * gammaOffset + 0.5 * quadraticOffset;
     dualObjectiveValue += offsetObjective + dualFake;
     dualObjectiveValue -= 0.5 * gamma2 * gammaOffset + 0.5 * quadraticOffset;
     // Need to rethink (but it is only for printing)
     //compute error and fixed RHS
     multiplyAdd(primalNew + numberColumns_, numberRows_, -1.0, errorRegionNew, 0.0);
     matrix_->times(1.0, primalNew, errorRegionNew);
     //finish off objective computation
     CoinWorkDouble primalObjectiveNew = primalObjectiveValue * scaleFactor_;
     CoinWorkDouble dualValue2 = innerProduct(dualNew, numberRows_,
                                 rhsFixRegionNew);
     dualObjectiveValue -= dualValue2;
     //CoinWorkDouble dualObjectiveNew=dualObjectiveValue*scaleFactor_;
     CoinWorkDouble maximumRHSError1 = 0.0;
     CoinWorkDouble maximumRHSError2 = 0.0;
     CoinWorkDouble primalOffset = 0.0;
     char * dropped = cholesky_->rowsDropped();
     int iRow;
     for (iRow = 0; iRow < numberRows_; iRow++) {
          CoinWorkDouble value = errorRegionNew[iRow];
          if (!dropped[iRow]) {
               if (CoinAbs(value) > maximumRHSError1) {
                    maximumRHSError1 = CoinAbs(value);
               }
          } else {
               if (CoinAbs(value) > maximumRHSError2) {
                    maximumRHSError2 = CoinAbs(value);
               }
               primalOffset += value * dualNew[iRow];
          }
     }
     primalObjectiveNew -= primalOffset * scaleFactor_;
     //CoinWorkDouble maximumRHSError;
     if (maximumRHSError1 > maximumRHSError2) {
          ;//maximumRHSError = maximumRHSError1;
     } else {
          //maximumRHSError = maximumRHSError1; //note change
          if (maximumRHSError2 > primalTolerance()) {
               handler_->message(CLP_BARRIER_ABS_DROPPED, messages_)
                         << static_cast<double>(maximumRHSError2)
                         << CoinMessageEol;
          }
     }
     /*printf("PH %d %g, %g new comp %g, b %g, p %g, d %g\n",phase,
      primalStep,dualStep,newGap,maximumBoundInfeasibility,
      maximumRHSError,maximumDualError);
     if (handler_->logLevel()>1)
       printf("       objs %g %g\n",
        primalObjectiveNew,dualObjectiveNew);
     if (maximumDjInfeasibility) {
       printf(" max dj error on fixed %g\n",
        maximumDjInfeasibility);
        } */
     delete [] dualNew;
     delete [] errorRegionNew;
     delete [] rhsFixRegionNew;
     delete [] primalNew;
     delete [] djNew;
}
