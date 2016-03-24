/* $Id$ */
// Copyright (C) 2004, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).


#include <cstdio>

#include "CoinPragma.hpp"
#include "CoinIndexedVector.hpp"
#include "CoinHelperFunctions.hpp"

#include "ClpSimplex.hpp"
#include "ClpFactorization.hpp"
#include "ClpQuadraticObjective.hpp"
#include "ClpNonLinearCost.hpp"
// at end to get min/max!
#include "ClpDynamicMatrix.hpp"
#include "ClpMessage.hpp"
//#define CLP_DEBUG
//#define CLP_DEBUG_PRINT
//#############################################################################
// Constructors / Destructor / Assignment
//#############################################################################

//-------------------------------------------------------------------
// Default Constructor
//-------------------------------------------------------------------
ClpDynamicMatrix::ClpDynamicMatrix ()
     : ClpPackedMatrix(),
       sumDualInfeasibilities_(0.0),
       sumPrimalInfeasibilities_(0.0),
       sumOfRelaxedDualInfeasibilities_(0.0),
       sumOfRelaxedPrimalInfeasibilities_(0.0),
       savedBestGubDual_(0.0),
       savedBestSet_(0),
       backToPivotRow_(NULL),
       keyVariable_(NULL),
       toIndex_(NULL),
       fromIndex_(NULL),
       numberSets_(0),
       numberActiveSets_(0),
       objectiveOffset_(0.0),
       lowerSet_(NULL),
       upperSet_(NULL),
       status_(NULL),
       model_(NULL),
       firstAvailable_(0),
       firstAvailableBefore_(0),
       firstDynamic_(0),
       lastDynamic_(0),
       numberStaticRows_(0),
       numberElements_(0),
       numberDualInfeasibilities_(0),
       numberPrimalInfeasibilities_(0),
       noCheck_(-1),
       infeasibilityWeight_(0.0),
       numberGubColumns_(0),
       maximumGubColumns_(0),
       maximumElements_(0),
       startSet_(NULL),
       next_(NULL),
       startColumn_(NULL),
       row_(NULL),
       element_(NULL),
       cost_(NULL),
       id_(NULL),
       dynamicStatus_(NULL),
       columnLower_(NULL),
       columnUpper_(NULL)
{
     setType(15);
}

//-------------------------------------------------------------------
// Copy constructor
//-------------------------------------------------------------------
ClpDynamicMatrix::ClpDynamicMatrix (const ClpDynamicMatrix & rhs)
     : ClpPackedMatrix(rhs)
{
     objectiveOffset_ = rhs.objectiveOffset_;
     numberSets_ = rhs.numberSets_;
     numberActiveSets_ = rhs.numberActiveSets_;
     firstAvailable_ = rhs.firstAvailable_;
     firstAvailableBefore_ = rhs.firstAvailableBefore_;
     firstDynamic_ = rhs.firstDynamic_;
     lastDynamic_ = rhs.lastDynamic_;
     numberStaticRows_ = rhs.numberStaticRows_;
     numberElements_ = rhs.numberElements_;
     backToPivotRow_ = ClpCopyOfArray(rhs.backToPivotRow_, lastDynamic_);
     keyVariable_ = ClpCopyOfArray(rhs.keyVariable_, numberSets_);
     toIndex_ = ClpCopyOfArray(rhs.toIndex_, numberSets_);
     fromIndex_ = ClpCopyOfArray(rhs.fromIndex_, getNumRows() + 1 - numberStaticRows_);
     lowerSet_ = ClpCopyOfArray(rhs.lowerSet_, numberSets_);
     upperSet_ = ClpCopyOfArray(rhs.upperSet_, numberSets_);
     status_ = ClpCopyOfArray(rhs.status_, static_cast<int>(2*numberSets_+4*sizeof(int)));
     model_ = rhs.model_;
     sumDualInfeasibilities_ = rhs. sumDualInfeasibilities_;
     sumPrimalInfeasibilities_ = rhs.sumPrimalInfeasibilities_;
     sumOfRelaxedDualInfeasibilities_ = rhs.sumOfRelaxedDualInfeasibilities_;
     sumOfRelaxedPrimalInfeasibilities_ = rhs.sumOfRelaxedPrimalInfeasibilities_;
     numberDualInfeasibilities_ = rhs.numberDualInfeasibilities_;
     numberPrimalInfeasibilities_ = rhs.numberPrimalInfeasibilities_;
     savedBestGubDual_ = rhs.savedBestGubDual_;
     savedBestSet_ = rhs.savedBestSet_;
     noCheck_ = rhs.noCheck_;
     infeasibilityWeight_ = rhs.infeasibilityWeight_;
     // Now secondary data
     numberGubColumns_ = rhs.numberGubColumns_;
     maximumGubColumns_ = rhs.maximumGubColumns_;
     maximumElements_ = rhs.maximumElements_;
     startSet_ = ClpCopyOfArray(rhs.startSet_, numberSets_+1);
     next_ = ClpCopyOfArray(rhs.next_, maximumGubColumns_);
     startColumn_ = ClpCopyOfArray(rhs.startColumn_, maximumGubColumns_ + 1);
     row_ = ClpCopyOfArray(rhs.row_, maximumElements_);
     element_ = ClpCopyOfArray(rhs.element_, maximumElements_);
     cost_ = ClpCopyOfArray(rhs.cost_, maximumGubColumns_);
     id_ = ClpCopyOfArray(rhs.id_, lastDynamic_ - firstDynamic_);
     columnLower_ = ClpCopyOfArray(rhs.columnLower_, maximumGubColumns_);
     columnUpper_ = ClpCopyOfArray(rhs.columnUpper_, maximumGubColumns_);
     dynamicStatus_ = ClpCopyOfArray(rhs.dynamicStatus_, 2*maximumGubColumns_);
}

/* This is the real constructor*/
ClpDynamicMatrix::ClpDynamicMatrix(ClpSimplex * model, int numberSets,
                                   int numberGubColumns, const int * starts,
                                   const double * lower, const double * upper,
                                   const CoinBigIndex * startColumn, const int * row,
                                   const double * element, const double * cost,
                                   const double * columnLower, const double * columnUpper,
                                   const unsigned char * status,
                                   const unsigned char * dynamicStatus)
     : ClpPackedMatrix()
{
     setType(15);
     objectiveOffset_ = model->objectiveOffset();
     model_ = model;
     numberSets_ = numberSets;
     numberGubColumns_ = numberGubColumns;
     maximumGubColumns_ = numberGubColumns_;
     if (numberGubColumns_)
          maximumElements_ = startColumn[numberGubColumns_];
     else
          maximumElements_ = 0;
     startSet_ = new int [numberSets_+1];
     next_ = new int [maximumGubColumns_];
     // fill in startSet and next
     int iSet;
     if (numberGubColumns_) {
          for (iSet = 0; iSet < numberSets_; iSet++) {
               int first = starts[iSet];
               int last = starts[iSet+1] - 1;
               startSet_[iSet] = first;
               for (int i = first; i < last; i++)
                    next_[i] = i + 1;
               next_[last] = -iSet - 1;
          }
	  startSet_[numberSets_] = starts[numberSets_];
     }
     int numberColumns = model->numberColumns();
     int numberRows = model->numberRows();
     numberStaticRows_ = numberRows;
     savedBestGubDual_ = 0.0;
     savedBestSet_ = 0;
     // Number of columns needed
     int frequency = model->factorizationFrequency();
     int numberGubInSmall = numberRows + frequency + CoinMin(frequency, numberSets_) + 4;
     // But we may have two per row + one for incoming (make it two) 
     numberGubInSmall = CoinMax(2*numberRows+2,numberGubInSmall);
     // for small problems this could be too big
     //numberGubInSmall = CoinMin(numberGubInSmall,numberGubColumns_);
     int numberNeeded = numberGubInSmall + numberColumns;
     firstAvailable_ = numberColumns;
     firstAvailableBefore_ = firstAvailable_;
     firstDynamic_ = numberColumns;
     lastDynamic_ = numberNeeded;
     startColumn_ = ClpCopyOfArray(startColumn, numberGubColumns_ + 1);
     if (!numberGubColumns_) {
       if (!startColumn_)
	 startColumn_ = new CoinBigIndex [1];
       startColumn_[0] = 0;
     }
     CoinBigIndex numberElements = startColumn_[numberGubColumns_];
     row_ = ClpCopyOfArray(row, numberElements);
     element_ = new double[numberElements];
     CoinBigIndex i;
     for (i = 0; i < numberElements; i++)
          element_[i] = element[i];
     cost_ = new double[numberGubColumns_];
     for (i = 0; i < numberGubColumns_; i++) {
          cost_[i] = cost[i];
          // I don't think I need sorted but ...
          CoinSort_2(row_ + startColumn_[i], row_ + startColumn_[i+1], element_ + startColumn_[i]);
     }
     if (columnLower) {
          columnLower_ = new double[numberGubColumns_];
          for (i = 0; i < numberGubColumns_; i++)
               columnLower_[i] = columnLower[i];
     } else {
          columnLower_ = NULL;
     }
     if (columnUpper) {
          columnUpper_ = new double[numberGubColumns_];
          for (i = 0; i < numberGubColumns_; i++)
               columnUpper_[i] = columnUpper[i];
     } else {
          columnUpper_ = NULL;
     }
     lowerSet_ = new double[numberSets_];
     for (i = 0; i < numberSets_; i++) {
          if (lower[i] > -1.0e20)
               lowerSet_[i] = lower[i];
          else
               lowerSet_[i] = -1.0e30;
     }
     upperSet_ = new double[numberSets_];
     for (i = 0; i < numberSets_; i++) {
          if (upper[i] < 1.0e20)
               upperSet_[i] = upper[i];
          else
               upperSet_[i] = 1.0e30;
     }
     id_ = new int[numberGubInSmall];
     for (i = 0; i < numberGubInSmall; i++)
          id_[i] = -1;
     ClpPackedMatrix* originalMatrixA =
          dynamic_cast< ClpPackedMatrix*>(model->clpMatrix());
     assert (originalMatrixA);
     CoinPackedMatrix * originalMatrix = originalMatrixA->getPackedMatrix();
     originalMatrixA->setMatrixNull(); // so can be deleted safely
     // guess how much space needed
     double guess = numberElements;
     guess /= static_cast<double> (numberColumns);
     guess *= 2 * numberGubInSmall;
     numberElements_ = static_cast<int> (guess);
     numberElements_ = CoinMin(numberElements_, numberElements) + originalMatrix->getNumElements();
     matrix_ = originalMatrix;
     //delete originalMatrixA;
     flags_ &= ~1;
     // resize model (matrix stays same)
     // modify frequency
     if (frequency>=50)
       frequency = 50+(frequency-50)/2;
     int newRowSize = numberRows + CoinMin(numberSets_, frequency+numberRows) + 1;
     model->resize(newRowSize, numberNeeded);
     for (i = numberRows; i < newRowSize; i++)
          model->setRowStatus(i, ClpSimplex::basic);
     if (columnUpper_) {
          // set all upper bounds so we have enough space
          double * columnUpper = model->columnUpper();
          for(i = firstDynamic_; i < lastDynamic_; i++)
               columnUpper[i] = 1.0e10;
     }
     // resize matrix
     // extra 1 is so can keep number of elements handy
     originalMatrix->reserve(numberNeeded, numberElements_, true);
     originalMatrix->reserve(numberNeeded + 1, numberElements_, false);
     originalMatrix->getMutableVectorStarts()[numberColumns] = originalMatrix->getNumElements();
     originalMatrix->setDimensions(newRowSize, -1);
     numberActiveColumns_ = firstDynamic_;
     // redo number of columns
     numberColumns = matrix_->getNumCols();
     backToPivotRow_ = new int[numberNeeded];
     keyVariable_ = new int[numberSets_];
     if (status) {
          status_ = ClpCopyOfArray(status, static_cast<int>(2*numberSets_+4*sizeof(int)));
          assert (dynamicStatus);
          dynamicStatus_ = ClpCopyOfArray(dynamicStatus, 2*numberGubColumns_);
     } else {
          status_ = new unsigned char [2*numberSets_+4*sizeof(int)];
          memset(status_, 0, numberSets_);
          int i;
          for (i = 0; i < numberSets_; i++) {
               // make slack key
               setStatus(i, ClpSimplex::basic);
          }
          dynamicStatus_ = new unsigned char [2*numberGubColumns_];
          memset(dynamicStatus_, 0, numberGubColumns_); // for clarity
          for (i = 0; i < numberGubColumns_; i++)
               setDynamicStatus(i, atLowerBound);
     }
     toIndex_ = new int[numberSets_];
     for (iSet = 0; iSet < numberSets_; iSet++)
          toIndex_[iSet] = -1;
     fromIndex_ = new int [newRowSize-numberStaticRows_+1];
     numberActiveSets_ = 0;
     rhsOffset_ = NULL;
     if (numberGubColumns_) {
          if (!status) {
               gubCrash();
          } else {
               initialProblem();
          }
     }
     noCheck_ = -1;
     infeasibilityWeight_ = 0.0;
}

//-------------------------------------------------------------------
// Destructor
//-------------------------------------------------------------------
ClpDynamicMatrix::~ClpDynamicMatrix ()
{
     delete [] backToPivotRow_;
     delete [] keyVariable_;
     delete [] toIndex_;
     delete [] fromIndex_;
     delete [] lowerSet_;
     delete [] upperSet_;
     delete [] status_;
     delete [] startSet_;
     delete [] next_;
     delete [] startColumn_;
     delete [] row_;
     delete [] element_;
     delete [] cost_;
     delete [] id_;
     delete [] dynamicStatus_;
     delete [] columnLower_;
     delete [] columnUpper_;
}

//----------------------------------------------------------------
// Assignment operator
//-------------------------------------------------------------------
ClpDynamicMatrix &
ClpDynamicMatrix::operator=(const ClpDynamicMatrix& rhs)
{
     if (this != &rhs) {
          ClpPackedMatrix::operator=(rhs);
          delete [] backToPivotRow_;
          delete [] keyVariable_;
          delete [] toIndex_;
          delete [] fromIndex_;
          delete [] lowerSet_;
          delete [] upperSet_;
          delete [] status_;
          delete [] startSet_;
          delete [] next_;
          delete [] startColumn_;
          delete [] row_;
          delete [] element_;
          delete [] cost_;
          delete [] id_;
          delete [] dynamicStatus_;
          delete [] columnLower_;
          delete [] columnUpper_;
          objectiveOffset_ = rhs.objectiveOffset_;
          numberSets_ = rhs.numberSets_;
          numberActiveSets_ = rhs.numberActiveSets_;
          firstAvailable_ = rhs.firstAvailable_;
          firstAvailableBefore_ = rhs.firstAvailableBefore_;
          firstDynamic_ = rhs.firstDynamic_;
          lastDynamic_ = rhs.lastDynamic_;
          numberStaticRows_ = rhs.numberStaticRows_;
          numberElements_ = rhs.numberElements_;
          backToPivotRow_ = ClpCopyOfArray(rhs.backToPivotRow_, lastDynamic_);
          keyVariable_ = ClpCopyOfArray(rhs.keyVariable_, numberSets_);
          toIndex_ = ClpCopyOfArray(rhs.toIndex_, numberSets_);
          fromIndex_ = ClpCopyOfArray(rhs.fromIndex_, getNumRows() + 1 - numberStaticRows_);
          lowerSet_ = ClpCopyOfArray(rhs.lowerSet_, numberSets_);
          upperSet_ = ClpCopyOfArray(rhs.upperSet_, numberSets_);
          status_ = ClpCopyOfArray(rhs.status_, static_cast<int>(2*numberSets_+4*sizeof(int)));
          model_ = rhs.model_;
          sumDualInfeasibilities_ = rhs. sumDualInfeasibilities_;
          sumPrimalInfeasibilities_ = rhs.sumPrimalInfeasibilities_;
          sumOfRelaxedDualInfeasibilities_ = rhs.sumOfRelaxedDualInfeasibilities_;
          sumOfRelaxedPrimalInfeasibilities_ = rhs.sumOfRelaxedPrimalInfeasibilities_;
          numberDualInfeasibilities_ = rhs.numberDualInfeasibilities_;
          numberPrimalInfeasibilities_ = rhs.numberPrimalInfeasibilities_;
          savedBestGubDual_ = rhs.savedBestGubDual_;
          savedBestSet_ = rhs.savedBestSet_;
          noCheck_ = rhs.noCheck_;
          infeasibilityWeight_ = rhs.infeasibilityWeight_;
          // Now secondary data
          numberGubColumns_ = rhs.numberGubColumns_;
          maximumGubColumns_ = rhs.maximumGubColumns_;
          maximumElements_ = rhs.maximumElements_;
          startSet_ = ClpCopyOfArray(rhs.startSet_, numberSets_+1);
          next_ = ClpCopyOfArray(rhs.next_, maximumGubColumns_);
          startColumn_ = ClpCopyOfArray(rhs.startColumn_, maximumGubColumns_ + 1);
          row_ = ClpCopyOfArray(rhs.row_, maximumElements_);
          element_ = ClpCopyOfArray(rhs.element_, maximumElements_);
          cost_ = ClpCopyOfArray(rhs.cost_, maximumGubColumns_);
          id_ = ClpCopyOfArray(rhs.id_, lastDynamic_ - firstDynamic_);
          columnLower_ = ClpCopyOfArray(rhs.columnLower_, maximumGubColumns_);
          columnUpper_ = ClpCopyOfArray(rhs.columnUpper_, maximumGubColumns_);
          dynamicStatus_ = ClpCopyOfArray(rhs.dynamicStatus_, 2*maximumGubColumns_);
     }
     return *this;
}
//-------------------------------------------------------------------
// Clone
//-------------------------------------------------------------------
ClpMatrixBase * ClpDynamicMatrix::clone() const
{
     return new ClpDynamicMatrix(*this);
}
// Partial pricing
void
ClpDynamicMatrix::partialPricing(ClpSimplex * model, double startFraction, double endFraction,
                                 int & bestSequence, int & numberWanted)
{
     numberWanted = currentWanted_;
     assert(!model->rowScale());
     if (numberSets_) {
          // Do packed part before gub
          // always???
          //printf("normal packed price - start %d end %d (passed end %d, first %d)\n",
          ClpPackedMatrix::partialPricing(model, startFraction, endFraction, bestSequence, numberWanted);
     } else {
          // no gub
          ClpPackedMatrix::partialPricing(model, startFraction, endFraction, bestSequence, numberWanted);
          return;
     }
     if (numberWanted > 0) {
          // and do some proportion of full set
          int startG2 = static_cast<int> (startFraction * numberSets_);
          int endG2 = static_cast<int> (endFraction * numberSets_ + 0.1);
          endG2 = CoinMin(endG2, numberSets_);
          //printf("gub price - set start %d end %d\n",
          //   startG2,endG2);
          double tolerance = model->currentDualTolerance();
          double * reducedCost = model->djRegion();
          const double * duals = model->dualRowSolution();
          double bestDj;
          int numberRows = model->numberRows();
          int slackOffset = lastDynamic_ + numberRows;
          int structuralOffset = slackOffset + numberSets_;
          // If nothing found yet can go all the way to end
          int endAll = endG2;
          if (bestSequence < 0 && !startG2)
               endAll = numberSets_;
          if (bestSequence >= 0) {
               if (bestSequence != savedBestSequence_)
                    bestDj = fabs(reducedCost[bestSequence]); // dj from slacks or permanent
               else
                    bestDj = savedBestDj_;
          } else {
               bestDj = tolerance;
          }
          int saveSequence = bestSequence;
          double djMod = 0.0;
          double bestDjMod = 0.0;
          //printf("iteration %d start %d end %d - wanted %d\n",model->numberIterations(),
          //     startG2,endG2,numberWanted);
          int bestSet = -1;
#if 0
          // make sure first available is clean (in case last iteration rejected)
          cost[firstAvailable_] = 0.0;
          length[firstAvailable_] = 0;
          model->nonLinearCost()->setOne(firstAvailable_, 0.0, 0.0, COIN_DBL_MAX, 0.0);
          model->setStatus(firstAvailable_, ClpSimplex::atLowerBound);
          {
               for (int i = firstAvailable_; i < lastDynamic_; i++)
                    assert(!cost[i]);
          }
#endif
          int minSet = minimumObjectsScan_ < 0 ? 5 : minimumObjectsScan_;
          int minNeg = minimumGoodReducedCosts_ < 0 ? 5 : minimumGoodReducedCosts_;
          for (int iSet = startG2; iSet < endAll; iSet++) {
               if (numberWanted + minNeg < originalWanted_ && iSet > startG2 + minSet) {
                    // give up
                    numberWanted = 0;
                    break;
               } else if (iSet == endG2 && bestSequence >= 0) {
                    break;
               }
               int gubRow = toIndex_[iSet];
               if (gubRow >= 0) {
                    djMod = duals[gubRow+numberStaticRows_]; // have I got sign right?
               } else {
                    int iBasic = keyVariable_[iSet];
                    if (iBasic >= maximumGubColumns_) {
                         djMod = 0.0; // set not in
                    } else {
                         // get dj without
                         djMod = 0.0;
                         for (CoinBigIndex j = startColumn_[iBasic];
                                   j < startColumn_[iBasic+1]; j++) {
                              int jRow = row_[j];
                              djMod -= duals[jRow] * element_[j];
                         }
                         djMod += cost_[iBasic];
                         // See if gub slack possible - dj is djMod
                         if (getStatus(iSet) == ClpSimplex::atLowerBound) {
                              double value = -djMod;
                              if (value > tolerance) {
                                   numberWanted--;
                                   if (value > bestDj) {
                                        // check flagged variable and correct dj
                                        if (!flagged(iSet)) {
                                             bestDj = value;
                                             bestSequence = slackOffset + iSet;
                                             bestDjMod = djMod;
                                             bestSet = iSet;
                                        } else {
                                             // just to make sure we don't exit before got something
                                             numberWanted++;
                                             abort();
                                        }
                                   }
                              }
                         } else if (getStatus(iSet) == ClpSimplex::atUpperBound) {
                              double value = djMod;
                              if (value > tolerance) {
                                   numberWanted--;
                                   if (value > bestDj) {
                                        // check flagged variable and correct dj
                                        if (!flagged(iSet)) {
                                             bestDj = value;
                                             bestSequence = slackOffset + iSet;
                                             bestDjMod = djMod;
                                             bestSet = iSet;
                                        } else {
                                             // just to make sure we don't exit before got something
                                             numberWanted++;
                                             abort();
                                        }
                                   }
                              }
                         }
                    }
               }
               int iSequence = startSet_[iSet];
               while (iSequence >= 0) {
                    DynamicStatus status = getDynamicStatus(iSequence);
                    if (status == atLowerBound || status == atUpperBound) {
                         double value = cost_[iSequence] - djMod;
                         for (CoinBigIndex j = startColumn_[iSequence];
                                   j < startColumn_[iSequence+1]; j++) {
                              int jRow = row_[j];
                              value -= duals[jRow] * element_[j];
                         }
                         // change sign if at lower bound
                         if (status == atLowerBound)
                              value = -value;
                         if (value > tolerance) {
                              numberWanted--;
                              if (value > bestDj) {
                                   // check flagged variable and correct dj
                                   if (!flagged(iSequence)) {
				     if (false/*status == atLowerBound
						&&keyValue(iSet)<1.0e-7*/) {
				       // can't come in because
				       // of ones at ub
				       numberWanted++;
				     } else {
				     
                                        bestDj = value;
                                        bestSequence = structuralOffset + iSequence;
                                        bestDjMod = djMod;
                                        bestSet = iSet;
				     }
                                   } else {
                                        // just to make sure we don't exit before got something
                                        numberWanted++;
                                   }
                              }
                         }
                    }
                    iSequence = next_[iSequence]; //onto next in set
               }
               if (numberWanted <= 0) {
                    numberWanted = 0;
                    break;
               }
          }
          if (bestSequence != saveSequence) {
               savedBestGubDual_ = bestDjMod;
               savedBestDj_ = bestDj;
               savedBestSequence_ = bestSequence;
               savedBestSet_ = bestSet;
          }
          // See if may be finished
          if (!startG2 && bestSequence < 0)
               infeasibilityWeight_ = model_->infeasibilityCost();
          else if (bestSequence >= 0)
               infeasibilityWeight_ = -1.0;
     }
     currentWanted_ = numberWanted;
}
/* Returns effective RHS if it is being used.  This is used for long problems
   or big gub or anywhere where going through full columns is
   expensive.  This may re-compute */
double *
ClpDynamicMatrix::rhsOffset(ClpSimplex * model, bool forceRefresh,
                            bool
#ifdef CLP_DEBUG
                            check
#endif
                           )
{
     // forceRefresh=true;printf("take out forceRefresh\n");
     if (!model_->numberIterations())
          forceRefresh = true;
     //check=false;
#ifdef CLP_DEBUG
     double * saveE = NULL;
     if (rhsOffset_ && check) {
          int numberRows = model->numberRows();
          saveE = new double[numberRows];
     }
#endif
     if (rhsOffset_) {
#ifdef CLP_DEBUG
          if (check) {
               // no need - but check anyway
               int numberRows = model->numberRows();
               double * rhs = new double[numberRows];
               int iRow;
               int iSet;
               CoinZeroN(rhs, numberRows);
               // do ones at bounds before gub
               const double * smallSolution = model->solutionRegion();
               const double * element = matrix_->getElements();
               const int * row = matrix_->getIndices();
               const CoinBigIndex * startColumn = matrix_->getVectorStarts();
               const int * length = matrix_->getVectorLengths();
               int iColumn;
               double objectiveOffset = 0.0;
               for (iColumn = 0; iColumn < firstDynamic_; iColumn++) {
                    if (model->getStatus(iColumn) != ClpSimplex::basic) {
                         double value = smallSolution[iColumn];
                         for (CoinBigIndex j = startColumn[iColumn];
                                   j < startColumn[iColumn] + length[iColumn]; j++) {
                              int jRow = row[j];
                              rhs[jRow] -= value * element[j];
                         }
                    }
               }
               if (columnLower_ || columnUpper_) {
                    double * solution = new double [numberGubColumns_];
                    for (iSet = 0; iSet < numberSets_; iSet++) {
                         int j = startSet_[iSet];
                         while (j >= 0) {
                              double value = 0.0;
                              if (getDynamicStatus(j) != inSmall) {
                                   if (getDynamicStatus(j) == atLowerBound) {
                                        if (columnLower_)
                                             value = columnLower_[j];
                                   } else if (getDynamicStatus(j) == atUpperBound) {
                                        value = columnUpper_[j];
                                   } else if (getDynamicStatus(j) == soloKey) {
                                        value = keyValue(iSet);
                                   }
                                   objectiveOffset += value * cost_[j];
                              }
                              solution[j] = value;
                              j = next_[j]; //onto next in set
                         }
                    }
                    // ones in gub and in small problem
                    for (iColumn = firstDynamic_; iColumn < firstAvailable_; iColumn++) {
                         if (model_->getStatus(iColumn) != ClpSimplex::basic) {
                              int jFull = id_[iColumn-firstDynamic_];
                              solution[jFull] = smallSolution[iColumn];
                         }
                    }
                    for (iSet = 0; iSet < numberSets_; iSet++) {
                         int kRow = toIndex_[iSet];
                         if (kRow >= 0)
                              kRow += numberStaticRows_;
                         int j = startSet_[iSet];
                         while (j >= 0) {
                              double value = solution[j];
                              if (value) {
                                   for (CoinBigIndex k = startColumn_[j]; k < startColumn_[j+1]; k++) {
                                        int iRow = row_[k];
                                        rhs[iRow] -= element_[k] * value;
                                   }
                                   if (kRow >= 0)
                                        rhs[kRow] -= value;
                              }
                              j = next_[j]; //onto next in set
                         }
                    }
                    delete [] solution;
               } else {
                    // bounds
                    ClpSimplex::Status iStatus;
                    for (int iSet = 0; iSet < numberSets_; iSet++) {
                         int kRow = toIndex_[iSet];
                         if (kRow < 0) {
                              int iColumn = keyVariable_[iSet];
                              if (iColumn < maximumGubColumns_) {
                                   // key is not treated as basic
                                   double b = 0.0;
                                   // key is structural - where is slack
                                   iStatus = getStatus(iSet);
                                   assert (iStatus != ClpSimplex::basic);
                                   if (iStatus == ClpSimplex::atLowerBound)
                                        b = lowerSet_[iSet];
                                   else
                                        b = upperSet_[iSet];
                                   if (b) {
                                        objectiveOffset += b * cost_[iColumn];
                                        for (CoinBigIndex j = startColumn_[iColumn]; j < startColumn_[iColumn+1]; j++) {
                                             int iRow = row_[j];
                                             rhs[iRow] -= element_[j] * b;
                                        }
                                   }
                              }
                         }
                    }
               }
               if (fabs(model->objectiveOffset() - objectiveOffset_ - objectiveOffset) > 1.0e-1)
                    printf("old offset %g, true %g\n", model->objectiveOffset(),
                           objectiveOffset_ - objectiveOffset);
               for (iRow = 0; iRow < numberRows; iRow++) {
                    if (fabs(rhs[iRow] - rhsOffset_[iRow]) > 1.0e-3)
                         printf("** bad effective %d - true %g old %g\n", iRow, rhs[iRow], rhsOffset_[iRow]);
               }
               CoinMemcpyN(rhs, numberRows, saveE);
               delete [] rhs;
          }
#endif
          if (forceRefresh || (refreshFrequency_ && model->numberIterations() >=
                               lastRefresh_ + refreshFrequency_)) {
               int numberRows = model->numberRows();
               int iSet;
               CoinZeroN(rhsOffset_, numberRows);
               // do ones at bounds before gub
               const double * smallSolution = model->solutionRegion();
               const double * element = matrix_->getElements();
               const int * row = matrix_->getIndices();
               const CoinBigIndex * startColumn = matrix_->getVectorStarts();
               const int * length = matrix_->getVectorLengths();
               int iColumn;
               double objectiveOffset = 0.0;
               for (iColumn = 0; iColumn < firstDynamic_; iColumn++) {
                    if (model->getStatus(iColumn) != ClpSimplex::basic) {
                         double value = smallSolution[iColumn];
                         for (CoinBigIndex j = startColumn[iColumn];
                                   j < startColumn[iColumn] + length[iColumn]; j++) {
                              int jRow = row[j];
                              rhsOffset_[jRow] -= value * element[j];
                         }
                    }
               }
               if (columnLower_ || columnUpper_) {
                    double * solution = new double [numberGubColumns_];
                    for (iSet = 0; iSet < numberSets_; iSet++) {
                         int j = startSet_[iSet];
                         while (j >= 0) {
                              double value = 0.0;
                              if (getDynamicStatus(j) != inSmall) {
                                   if (getDynamicStatus(j) == atLowerBound) {
                                        if (columnLower_)
                                             value = columnLower_[j];
                                   } else if (getDynamicStatus(j) == atUpperBound) {
                                        value = columnUpper_[j];
					assert (value<1.0e30);
                                   } else if (getDynamicStatus(j) == soloKey) {
                                        value = keyValue(iSet);
				   }
                                   objectiveOffset += value * cost_[j];
                              }
                              solution[j] = value;
                              j = next_[j]; //onto next in set
                         }
                    }
                    // ones in gub and in small problem
                    for (iColumn = firstDynamic_; iColumn < firstAvailable_; iColumn++) {
                         if (model_->getStatus(iColumn) != ClpSimplex::basic) {
                              int jFull = id_[iColumn-firstDynamic_];
                              solution[jFull] = smallSolution[iColumn];
                         }
                    }
                    for (iSet = 0; iSet < numberSets_; iSet++) {
                         int kRow = toIndex_[iSet];
                         if (kRow >= 0) 
			   kRow += numberStaticRows_;
			 int j = startSet_[iSet];
			 while (j >= 0) {
			   //? use DynamicStatus status = getDynamicStatus(j);
			   double value = solution[j];
			   if (value) {
			     for (CoinBigIndex k = startColumn_[j]; k < startColumn_[j+1]; k++) {
			       int iRow = row_[k];
			       rhsOffset_[iRow] -= element_[k] * value;
			     }
			     if (kRow >= 0)
			       rhsOffset_[kRow] -= value;
			   }
			   j = next_[j]; //onto next in set
			 }
                    }
                    delete [] solution;
               } else {
                    // bounds
                    ClpSimplex::Status iStatus;
                    for (int iSet = 0; iSet < numberSets_; iSet++) {
                         int kRow = toIndex_[iSet];
                         if (kRow < 0) {
                              int iColumn = keyVariable_[iSet];
                              if (iColumn < maximumGubColumns_) {
                                   // key is not treated as basic
                                   double b = 0.0;
                                   // key is structural - where is slack
                                   iStatus = getStatus(iSet);
                                   assert (iStatus != ClpSimplex::basic);
                                   if (iStatus == ClpSimplex::atLowerBound)
                                        b = lowerSet_[iSet];
                                   else
                                        b = upperSet_[iSet];
                                   if (b) {
                                        objectiveOffset += b * cost_[iColumn];
                                        for (CoinBigIndex j = startColumn_[iColumn]; j < startColumn_[iColumn+1]; j++) {
                                             int iRow = row_[j];
                                             rhsOffset_[iRow] -= element_[j] * b;
                                        }
                                   }
                              }
                         }
                    }
               }
               model->setObjectiveOffset(objectiveOffset_ - objectiveOffset);
#ifdef CLP_DEBUG
               if (saveE) {
                    int iRow;
                    for (iRow = 0; iRow < numberRows; iRow++) {
                         if (fabs(saveE[iRow] - rhsOffset_[iRow]) > 1.0e-3)
                              printf("** %d - old eff %g new %g\n", iRow, saveE[iRow], rhsOffset_[iRow]);
                    }
                    delete [] saveE;
               }
#endif
               lastRefresh_ = model->numberIterations();
          }
     }
     return rhsOffset_;
}
/*
  update information for a pivot (and effective rhs)
*/
int
ClpDynamicMatrix::updatePivot(ClpSimplex * model, double oldInValue, double oldOutValue)
{

     // now update working model
     int sequenceIn = model->sequenceIn();
     int sequenceOut = model->sequenceOut();
     int numberColumns = model->numberColumns();
     if (sequenceIn != sequenceOut && sequenceIn < numberColumns)
          backToPivotRow_[sequenceIn] = model->pivotRow();
     if (sequenceIn >= firstDynamic_ && sequenceIn < numberColumns) {
          int bigSequence = id_[sequenceIn-firstDynamic_];
          if (getDynamicStatus(bigSequence) != inSmall) {
               firstAvailable_++;
               setDynamicStatus(bigSequence, inSmall);
          }
     }
     // make sure slack is synchronized
     if (sequenceIn >= numberColumns + numberStaticRows_) {
          int iDynamic = sequenceIn - numberColumns - numberStaticRows_;
          int iSet = fromIndex_[iDynamic];
          setStatus(iSet, model->getStatus(sequenceIn));
     }
     if (sequenceOut >= numberColumns + numberStaticRows_) {
          int iDynamic = sequenceOut - numberColumns - numberStaticRows_;
          int iSet = fromIndex_[iDynamic];
          // out may have gone through barrier - so check
          double valueOut = model->lowerRegion()[sequenceOut];
          if (fabs(valueOut - static_cast<double> (lowerSet_[iSet])) <
                    fabs(valueOut - static_cast<double> (upperSet_[iSet])))
               setStatus(iSet, ClpSimplex::atLowerBound);
          else
               setStatus(iSet, ClpSimplex::atUpperBound);
          if (lowerSet_[iSet] == upperSet_[iSet])
               setStatus(iSet, ClpSimplex::isFixed);
#if 0
          if (getStatus(iSet) != model->getStatus(sequenceOut))
               printf("** set %d status %d, var status %d\n", iSet,
                      getStatus(iSet), model->getStatus(sequenceOut));
#endif
     }
     ClpMatrixBase::updatePivot(model, oldInValue, oldOutValue);
#ifdef CLP_DEBUG
     char * inSmall = new char [numberGubColumns_];
     memset(inSmall, 0, numberGubColumns_);
     const double * solution = model->solutionRegion();
     for (int i = 0; i < numberGubColumns_; i++)
          if (getDynamicStatus(i) == ClpDynamicMatrix::inSmall)
               inSmall[i] = 1;
     for (int i = firstDynamic_; i < firstAvailable_; i++) {
          int k = id_[i-firstDynamic_];
          inSmall[k] = 0;
          //if (k>=23289&&k<23357&&solution[i])
          //printf("var %d (in small %d) has value %g\n",k,i,solution[i]);
     }
     for (int i = 0; i < numberGubColumns_; i++)
          assert (!inSmall[i]);
     delete [] inSmall;
#ifndef NDEBUG
     for (int i = 0; i < numberActiveSets_; i++) {
          int iSet = fromIndex_[i];
          assert (toIndex_[iSet] == i);
          //if (getStatus(iSet)!=model->getRowStatus(i+numberStaticRows_))
          //printf("*** set %d status %d, var status %d\n",iSet,
          //     getStatus(iSet),model->getRowStatus(i+numberStaticRows_));
          //assert (model->getRowStatus(i+numberStaticRows_)==getStatus(iSet));
          //if (iSet==1035) {
          //printf("rhs for set %d (%d) is %g %g - cost %g\n",iSet,i,model->lowerRegion(0)[i+numberStaticRows_],
          //     model->upperRegion(0)[i+numberStaticRows_],model->costRegion(0)[i+numberStaticRows_]);
          //}
     }
#endif
#endif
     if (numberStaticRows_+numberActiveSets_<model->numberRows())
       return 0;
     else
       return 1;
}
/*
     utility dual function for dealing with dynamic constraints
     mode=n see ClpGubMatrix.hpp for definition
     Remember to update here when settled down
*/
void
ClpDynamicMatrix::dualExpanded(ClpSimplex * model,
                               CoinIndexedVector * /*array*/,
                               double * /*other*/, int mode)
{
     switch (mode) {
          // modify costs before transposeUpdate
     case 0:
          break;
          // create duals for key variables (without check on dual infeasible)
     case 1:
          break;
          // as 1 but check slacks and compute djs
     case 2: {
          // do pivots
          int * pivotVariable = model->pivotVariable();
          int numberRows = numberStaticRows_ + numberActiveSets_;
          int numberColumns = model->numberColumns();
          for (int iRow = 0; iRow < numberRows; iRow++) {
               int iPivot = pivotVariable[iRow];
               if (iPivot < numberColumns)
                    backToPivotRow_[iPivot] = iRow;
          }
          if (noCheck_ >= 0) {
               if (infeasibilityWeight_ != model_->infeasibilityCost()) {
                    // don't bother checking
                    sumDualInfeasibilities_ = 100.0;
                    numberDualInfeasibilities_ = 1;
                    sumOfRelaxedDualInfeasibilities_ = 100.0;
                    return;
               }
          }
          // In theory we should subtract out ones we have done but ....
          // If key slack then dual 0.0
          // If not then slack could be dual infeasible
          // dj for key is zero so that defines dual on set
          int i;
          double * dual = model->dualRowSolution();
          double dualTolerance = model->dualTolerance();
          double relaxedTolerance = dualTolerance;
          // we can't really trust infeasibilities if there is dual error
          double error = CoinMin(1.0e-2, model->largestDualError());
          // allow tolerance at least slightly bigger than standard
          relaxedTolerance = relaxedTolerance +  error;
          // but we will be using difference
          relaxedTolerance -= dualTolerance;
          sumDualInfeasibilities_ = 0.0;
          numberDualInfeasibilities_ = 0;
          sumOfRelaxedDualInfeasibilities_ = 0.0;
          for (i = 0; i < numberSets_; i++) {
               double value = 0.0;
               int gubRow = toIndex_[i];
               if (gubRow < 0) {
                    int kColumn = keyVariable_[i];
                    if (kColumn < maximumGubColumns_) {
                         // dj without set
                         value = cost_[kColumn];
                         for (CoinBigIndex j = startColumn_[kColumn];
                                   j < startColumn_[kColumn+1]; j++) {
                              int iRow = row_[j];
                              value -= dual[iRow] * element_[j];
                         }
                         double infeasibility = 0.0;
                         if (getStatus(i) == ClpSimplex::atLowerBound) {
                              if (-value > dualTolerance)
                                   infeasibility = -value - dualTolerance;
                         } else if (getStatus(i) == ClpSimplex::atUpperBound) {
                              if (value > dualTolerance)
                                   infeasibility = value - dualTolerance;
                         }
                         if (infeasibility > 0.0) {
                              sumDualInfeasibilities_ += infeasibility;
                              if (infeasibility > relaxedTolerance)
                                   sumOfRelaxedDualInfeasibilities_ += infeasibility;
                              numberDualInfeasibilities_ ++;
                         }
                    }
               } else {
                    value = dual[gubRow+numberStaticRows_];
               }
               // Now subtract out from all
               int k = startSet_[i];
               while (k >= 0) {
                    if (getDynamicStatus(k) != inSmall) {
                         double djValue = cost_[k] - value;
                         for (CoinBigIndex j = startColumn_[k];
                                   j < startColumn_[k+1]; j++) {
                              int iRow = row_[j];
                              djValue -= dual[iRow] * element_[j];
                         }
                         double infeasibility = 0.0;
                         if (getDynamicStatus(k) == atLowerBound) {
                              if (djValue < -dualTolerance)
                                   infeasibility = -djValue - dualTolerance;
                         } else if (getDynamicStatus(k) == atUpperBound) {
                              // at upper bound
                              if (djValue > dualTolerance)
                                   infeasibility = djValue - dualTolerance;
                         }
                         if (infeasibility > 0.0) {
                              sumDualInfeasibilities_ += infeasibility;
                              if (infeasibility > relaxedTolerance)
                                   sumOfRelaxedDualInfeasibilities_ += infeasibility;
                              numberDualInfeasibilities_ ++;
                         }
                    }
                    k = next_[k]; //onto next in set
               }
          }
     }
     infeasibilityWeight_ = -1.0;
     break;
     // Report on infeasibilities of key variables
     case 3: {
          model->setSumDualInfeasibilities(model->sumDualInfeasibilities() +
                                           sumDualInfeasibilities_);
          model->setNumberDualInfeasibilities(model->numberDualInfeasibilities() +
                                              numberDualInfeasibilities_);
          model->setSumOfRelaxedDualInfeasibilities(model->sumOfRelaxedDualInfeasibilities() +
                    sumOfRelaxedDualInfeasibilities_);
     }
     break;
     // modify costs before transposeUpdate for partial pricing
     case 4:
          break;
     }
}
/*
     general utility function for dealing with dynamic constraints
     mode=n see ClpGubMatrix.hpp for definition
     Remember to update here when settled down
*/
int
ClpDynamicMatrix::generalExpanded(ClpSimplex * model, int mode, int &number)
{
     int returnCode = 0;
#if 0 //ndef NDEBUG
     {
       int numberColumns = model->numberColumns();
       int numberRows = model->numberRows();
       int * pivotVariable = model->pivotVariable();
       if (pivotVariable&&model->numberIterations()) {
	 for (int i=numberStaticRows_+numberActiveSets_;i<numberRows;i++) {
	   assert (pivotVariable[i]==i+numberColumns);
	 }
       }
     }
#endif
     switch (mode) {
          // Fill in pivotVariable
     case 0: {
          // If no effective rhs - form it
          if (!rhsOffset_) {
               rhsOffset_ = new double[model->numberRows()];
               rhsOffset(model, true);
          }
          int i;
          int numberBasic = number;
          int numberColumns = model->numberColumns();
          // Use different array so can build from true pivotVariable_
          //int * pivotVariable = model->pivotVariable();
          int * pivotVariable = model->rowArray(0)->getIndices();
          for (i = 0; i < numberColumns; i++) {
               if (model->getColumnStatus(i) == ClpSimplex::basic)
                    pivotVariable[numberBasic++] = i;
          }
          number = numberBasic;
     }
     break;
     // Do initial extra rows + maximum basic
     case 2: {
          number = model->numberRows();
     }
     break;
     // Before normal replaceColumn
     case 3: {
          if (numberActiveSets_ + numberStaticRows_ == model_->numberRows()) {
               // no space - re-factorize
               returnCode = 4;
               number = -1; // say no need for normal replaceColumn
          }
     }
     break;
     // To see if can dual or primal
     case 4: {
          returnCode = 1;
     }
     break;
     // save status
     case 5: {
       memcpy(status_+numberSets_,status_,numberSets_);
       memcpy(status_+2*numberSets_,&numberActiveSets_,sizeof(int));
       memcpy(dynamicStatus_+maximumGubColumns_,
	      dynamicStatus_,maximumGubColumns_);
     }
     break;
     // restore status
     case 6: {
       memcpy(status_,status_+numberSets_,numberSets_);
       memcpy(&numberActiveSets_,status_+2*numberSets_,sizeof(int));
       memcpy(dynamicStatus_,dynamicStatus_+maximumGubColumns_,
	      maximumGubColumns_);
       initialProblem();
     }
     break;
     // unflag all variables
     case 8: {
          for (int i = 0; i < numberGubColumns_; i++) {
               if (flagged(i)) {
                    unsetFlagged(i);
                    returnCode++;
               }
          }
     }
     break;
     // redo costs in primal
     case 9: {
          double * cost = model->costRegion();
          double * solution = model->solutionRegion();
          double * columnLower = model->lowerRegion();
          double * columnUpper = model->upperRegion();
          int i;
          bool doCosts = (number & 4) != 0;
          bool doBounds = (number & 1) != 0;
          for ( i = firstDynamic_; i < firstAvailable_; i++) {
               int jColumn = id_[i-firstDynamic_];
               if (doBounds) {
                    if (!columnLower_ && !columnUpper_) {
                         columnLower[i] = 0.0;
                         columnUpper[i] = COIN_DBL_MAX;
                    }  else {
                         if (columnLower_)
                              columnLower[i] = columnLower_[jColumn];
                         else
                              columnLower[i] = 0.0;
                         if (columnUpper_)
                              columnUpper[i] = columnUpper_[jColumn];
                         else
                              columnUpper[i] = COIN_DBL_MAX;
                    }
               }
               if (doCosts) {
                    cost[i] = cost_[jColumn];
                    // Original bounds
                    if (model->nonLinearCost())
                         model->nonLinearCost()->setOne(i, solution[i],
                                                        this->columnLower(jColumn),
                                                        this->columnUpper(jColumn), cost_[jColumn]);
               }
          }
          // and active sets
          for (i = 0; i < numberActiveSets_; i++ ) {
               int iSet = fromIndex_[i];
               int iSequence = lastDynamic_ + numberStaticRows_ + i;
               if (doBounds) {
                    if (lowerSet_[iSet] > -1.0e20)
                         columnLower[iSequence] = lowerSet_[iSet];
                    else
                         columnLower[iSequence] = -COIN_DBL_MAX;
                    if (upperSet_[iSet] < 1.0e20)
                         columnUpper[iSequence] = upperSet_[iSet];
                    else
                         columnUpper[iSequence] = COIN_DBL_MAX;
               }
               if (doCosts) {
                    if (model->nonLinearCost()) {
                         double trueLower;
                         if (lowerSet_[iSet] > -1.0e20)
                              trueLower = lowerSet_[iSet];
                         else
                              trueLower = -COIN_DBL_MAX;
                         double trueUpper;
                         if (upperSet_[iSet] < 1.0e20)
                              trueUpper = upperSet_[iSet];
                         else
                              trueUpper = COIN_DBL_MAX;
                         model->nonLinearCost()->setOne(iSequence, solution[iSequence],
                                                        trueLower, trueUpper, 0.0);
                    }
               }
          }
     }
     break;
     // return 1 if there may be changing bounds on variable (column generation)
     case 10: {
          // return 1 as bounds on rhs will change
          returnCode = 1;
     }
     break;
     // make sure set is clean
     case 7: {
          // first flag
          if (number >= firstDynamic_ && number < lastDynamic_) {
	    int sequence = id_[number-firstDynamic_];
	    setFlagged(sequence);
	    //model->clearFlagged(number);
	  } else if (number>=model_->numberColumns()+numberStaticRows_) {
	    // slack
	    int iSet = fromIndex_[number-model_->numberColumns()-
				  numberStaticRows_];
	    setFlaggedSlack(iSet);
	    //model->clearFlagged(number);
	  }
     }
     case 11: {
          //int sequenceIn = model->sequenceIn();
          if (number >= firstDynamic_ && number < lastDynamic_) {
	    //assert (number == model->sequenceIn());
               // take out variable (but leave key)
               double * cost = model->costRegion();
               double * columnLower = model->lowerRegion();
               double * columnUpper = model->upperRegion();
               double * solution = model->solutionRegion();
               int * length = matrix_->getMutableVectorLengths();
#ifndef NDEBUG
               if (length[number]) {
                    int * row = matrix_->getMutableIndices();
                    CoinBigIndex * startColumn = matrix_->getMutableVectorStarts();
                    int iRow = row[startColumn[number] + length[number] - 1];
                    int which = iRow - numberStaticRows_;
                    assert (which >= 0);
                    int iSet = fromIndex_[which];
                    assert (toIndex_[iSet] == which);
               }
#endif
               // no need firstAvailable_--;
               solution[firstAvailable_] = 0.0;
               cost[firstAvailable_] = 0.0;
               length[firstAvailable_] = 0;
               model->nonLinearCost()->setOne(firstAvailable_, 0.0, 0.0, COIN_DBL_MAX, 0.0);
               model->setStatus(firstAvailable_, ClpSimplex::atLowerBound);
               columnLower[firstAvailable_] = 0.0;
               columnUpper[firstAvailable_] = COIN_DBL_MAX;

               // not really in small problem
               int iBig = id_[number-firstDynamic_];
               if (model->getStatus(number) == ClpSimplex::atLowerBound) {
                    setDynamicStatus(iBig, atLowerBound);
                    if (columnLower_)
                         modifyOffset(number, columnLower_[iBig]);
               } else {
                    setDynamicStatus(iBig, atUpperBound);
                    modifyOffset(number, columnUpper_[iBig]);
               }
	  } else if (number>=model_->numberColumns()+numberStaticRows_) {
	    // slack
	    int iSet = fromIndex_[number-model_->numberColumns()-
				  numberStaticRows_];
	    printf("what now - set %d\n",iSet);
          }
     }
     break;
     default:
          break;
     }
     return returnCode;
}
/* Purely for column generation and similar ideas.  Allows
   matrix and any bounds or costs to be updated (sensibly).
   Returns non-zero if any changes.
*/
int
ClpDynamicMatrix::refresh(ClpSimplex * model)
{
     // If at beginning then create initial problem
     if (firstDynamic_ == firstAvailable_) {
          initialProblem();
          return 1;
     } else if (!model->nonLinearCost()) {
          // will be same as last time
          return 1;
     }
#ifndef NDEBUG
     {
       int numberColumns = model->numberColumns();
       int numberRows = model->numberRows();
       int * pivotVariable = model->pivotVariable();
       for (int i=numberStaticRows_+numberActiveSets_;i<numberRows;i++) {
	 assert (pivotVariable[i]==i+numberColumns);
       }
     }
#endif
     // lookup array
     int * active = new int [numberActiveSets_];
     CoinZeroN(active, numberActiveSets_);
     int iColumn;
     int numberColumns = model->numberColumns();
     double * element =  matrix_->getMutableElements();
     int * row = matrix_->getMutableIndices();
     CoinBigIndex * startColumn = matrix_->getMutableVectorStarts();
     int * length = matrix_->getMutableVectorLengths();
     double * cost = model->costRegion();
     double * columnLower = model->lowerRegion();
     double * columnUpper = model->upperRegion();
     CoinBigIndex numberElements = startColumn[firstDynamic_];
     // first just do lookup and basic stuff
     int currentNumber = firstAvailable_;
     firstAvailable_ = firstDynamic_;
     double objectiveChange = 0.0;
     double * solution = model->solutionRegion();
     int currentNumberActiveSets = numberActiveSets_;
     for (iColumn = firstDynamic_; iColumn < currentNumber; iColumn++) {
          int iRow = row[startColumn[iColumn] + length[iColumn] - 1];
          int which = iRow - numberStaticRows_;
          assert (which >= 0);
          int iSet = fromIndex_[which];
          assert (toIndex_[iSet] == which);
          if (model->getStatus(iColumn) == ClpSimplex::basic) {
               active[which]++;
               // may as well make key
               keyVariable_[iSet] = id_[iColumn-firstDynamic_];
          }
     }
     int i;
     numberActiveSets_ = 0;
     int numberDeleted = 0;
     for (i = 0; i < currentNumberActiveSets; i++) {
          int iRow = i + numberStaticRows_;
          int numberActive = active[i];
          int iSet = fromIndex_[i];
          if (model->getRowStatus(iRow) == ClpSimplex::basic) {
               numberActive++;
               // may as well make key
               keyVariable_[iSet] = maximumGubColumns_ + iSet;
          }
          if (numberActive > 1) {
               // keep
               active[i] = numberActiveSets_;
               numberActiveSets_++;
          } else {
               active[i] = -1;
          }
     }

     for (iColumn = firstDynamic_; iColumn < currentNumber; iColumn++) {
          int iRow = row[startColumn[iColumn] + length[iColumn] - 1];
          int which = iRow - numberStaticRows_;
          int jColumn = id_[iColumn-firstDynamic_];
          if (model->getStatus(iColumn) == ClpSimplex::atLowerBound ||
                    model->getStatus(iColumn) == ClpSimplex::atUpperBound) {
               double value = solution[iColumn];
               bool toLowerBound = true;
	       assert (jColumn>=0);assert (iColumn>=0);
               if (columnUpper_) {
                    if (!columnLower_) {
                         if (fabs(value - columnUpper_[jColumn]) < fabs(value))
                              toLowerBound = false;
                    } else if (fabs(value - columnUpper_[jColumn]) < fabs(value - columnLower_[jColumn])) {
                         toLowerBound = false;
                    }
               }
               if (toLowerBound) {
                    // throw out to lower bound
                    if (columnLower_) {
                         setDynamicStatus(jColumn, atLowerBound);
                         // treat solution as if exactly at a bound
                         double value = columnLower[iColumn];
                         objectiveChange += cost[iColumn] * value;
                    } else {
                         setDynamicStatus(jColumn, atLowerBound);
                    }
               } else {
                    // throw out to upper bound
                    setDynamicStatus(jColumn, atUpperBound);
                    double value = columnUpper[iColumn];
                    objectiveChange += cost[iColumn] * value;
               }
               numberDeleted++;
          } else {
               assert(model->getStatus(iColumn) != ClpSimplex::superBasic); // deal with later
               int iPut = active[which];
               if (iPut >= 0) {
                    // move
                    id_[firstAvailable_-firstDynamic_] = jColumn;
                    int numberThis = startColumn_[jColumn+1] - startColumn_[jColumn] + 1;
                    length[firstAvailable_] = numberThis;
                    cost[firstAvailable_] = cost[iColumn];
                    columnLower[firstAvailable_] = columnLower[iColumn];
                    columnUpper[firstAvailable_] = columnUpper[iColumn];
                    model->nonLinearCost()->setOne(firstAvailable_, solution[iColumn], 0.0, COIN_DBL_MAX,
                                                   cost_[jColumn]);
                    CoinBigIndex base = startColumn_[jColumn];
                    for (int j = 0; j < numberThis - 1; j++) {
                         row[numberElements] = row_[base+j];
                         element[numberElements++] = element_[base+j];
                    }
                    row[numberElements] = iPut + numberStaticRows_;
                    element[numberElements++] = 1.0;
                    model->setStatus(firstAvailable_, model->getStatus(iColumn));
                    solution[firstAvailable_] = solution[iColumn];
                    firstAvailable_++;
                    startColumn[firstAvailable_] = numberElements;
               }
          }
     }
     // zero solution
     CoinZeroN(solution + firstAvailable_, currentNumber - firstAvailable_);
     // zero cost
     CoinZeroN(cost + firstAvailable_, currentNumber - firstAvailable_);
     // zero lengths
     CoinZeroN(length + firstAvailable_, currentNumber - firstAvailable_);
     for ( iColumn = firstAvailable_; iColumn < currentNumber; iColumn++) {
          model->nonLinearCost()->setOne(iColumn, 0.0, 0.0, COIN_DBL_MAX, 0.0);
          model->setStatus(iColumn, ClpSimplex::atLowerBound);
          columnLower[iColumn] = 0.0;
          columnUpper[iColumn] = COIN_DBL_MAX;
     }
     // move rhs and set rest to infinite
     numberActiveSets_ = 0;
     for (i = 0; i < currentNumberActiveSets; i++) {
          int iSet = fromIndex_[i];
          assert (toIndex_[iSet] == i);
          int iRow = i + numberStaticRows_;
          int iPut = active[i];
          if (iPut >= 0) {
               int in = iRow + numberColumns;
               int out = iPut + numberColumns + numberStaticRows_;
               solution[out] = solution[in];
               columnLower[out] = columnLower[in];
               columnUpper[out] = columnUpper[in];
               cost[out] = cost[in];
               double trueLower;
               if (lowerSet_[iSet] > -1.0e20)
                    trueLower = lowerSet_[iSet];
               else
                    trueLower = -COIN_DBL_MAX;
               double trueUpper;
               if (upperSet_[iSet] < 1.0e20)
                    trueUpper = upperSet_[iSet];
               else
                    trueUpper = COIN_DBL_MAX;
               model->nonLinearCost()->setOne(out, solution[out],
                                              trueLower, trueUpper, 0.0);
               model->setStatus(out, model->getStatus(in));
               toIndex_[iSet] = numberActiveSets_;
               rhsOffset_[numberActiveSets_+numberStaticRows_] = rhsOffset_[i+numberStaticRows_];
               fromIndex_[numberActiveSets_++] = fromIndex_[i];
          } else {
               // adjust offset
               // put out as key
               int jColumn = keyVariable_[iSet];
               toIndex_[iSet] = -1;
               if (jColumn < maximumGubColumns_) {
                    setDynamicStatus(jColumn, soloKey);
                    double value = keyValue(iSet);
                    objectiveChange += cost_[jColumn] * value;
                    modifyOffset(jColumn, -value);
               }
          }
     }
     model->setObjectiveOffset(model->objectiveOffset() - objectiveChange);
     delete [] active;
     for (i = numberActiveSets_; i < currentNumberActiveSets; i++) {
          int iSequence = i + numberStaticRows_ + numberColumns;
          solution[iSequence] = 0.0;
          columnLower[iSequence] = -COIN_DBL_MAX;
          columnUpper[iSequence] = COIN_DBL_MAX;
          cost[iSequence] = 0.0;
          model->nonLinearCost()->setOne(iSequence, solution[iSequence],
                                         columnLower[iSequence],
                                         columnUpper[iSequence], 0.0);
          model->setStatus(iSequence, ClpSimplex::basic);
          rhsOffset_[i+numberStaticRows_] = 0.0;
     }
     if (currentNumberActiveSets != numberActiveSets_ || numberDeleted) {
          // deal with pivotVariable
          int * pivotVariable = model->pivotVariable();
          int sequence = firstDynamic_;
          int iRow = 0;
          int base1 = firstDynamic_;
          int base2 = lastDynamic_;
          int base3 = numberColumns + numberStaticRows_;
          int numberRows = numberStaticRows_ + currentNumberActiveSets;
          while (sequence < firstAvailable_) {
               int iPivot = pivotVariable[iRow];
               while (iPivot < base1 || (iPivot >= base2 && iPivot < base3)) {
                    iPivot = pivotVariable[++iRow];
               }
               pivotVariable[iRow++] = sequence;
               sequence++;
          }
          // move normal basic ones down
          int iPut = iRow;
          for (; iRow < numberRows; iRow++) {
               int iPivot = pivotVariable[iRow];
               if (iPivot < base1 || (iPivot >= base2 && iPivot < base3)) {
                    pivotVariable[iPut++] = iPivot;
               }
          }
          // and basic others
          for (i = 0; i < numberActiveSets_; i++) {
               if (model->getRowStatus(i + numberStaticRows_) == ClpSimplex::basic) {
                    pivotVariable[iPut++] = i + base3;
               }
          }
	  if (iPut<numberStaticRows_+numberActiveSets_) {
	    printf("lost %d sets\n",
		   numberStaticRows_+numberActiveSets_-iPut);
	    iPut = numberStaticRows_+numberActiveSets_;
	  }
          for (i = numberActiveSets_; i < currentNumberActiveSets; i++) {
               pivotVariable[iPut++] = i + base3;
          }
          //assert (iPut == numberRows);
     }
#ifdef CLP_DEBUG
#if 0
     printf("row for set 244 is %d, row status %d value %g ", toIndex_[244], status_[244],
            keyValue(244));
     int jj = startSet_[244];
     while (jj >= 0) {
          printf("var %d status %d ", jj, dynamicStatus_[jj]);
          jj = next_[jj];
     }
     printf("\n");
#endif
#ifdef CLP_DEBUG
     {
          // debug coding to analyze set
          int i;
          int kSet = -6;
          if (kSet >= 0) {
               int * back = new int [numberGubColumns_];
               for (i = 0; i < numberGubColumns_; i++)
                    back[i] = -1;
               for (i = firstDynamic_; i < firstAvailable_; i++)
                    back[id_[i-firstDynamic_]] = i;
               int sequence = startSet_[kSet];
               if (toIndex_[kSet] < 0) {
                    printf("Not in - Set %d - slack status %d, key %d\n", kSet, status_[kSet], keyVariable_[kSet]);
                    while (sequence >= 0) {
                         printf("( %d status %d ) ", sequence, getDynamicStatus(sequence));
                         sequence = next_[sequence];
                    }
               } else {
                    int iRow = numberStaticRows_ + toIndex_[kSet];
                    printf("In - Set %d - slack status %d, key %d offset %g slack %g\n", kSet, status_[kSet], keyVariable_[kSet],
                           rhsOffset_[iRow], model->solutionRegion(0)[iRow]);
                    while (sequence >= 0) {
                         int iBack = back[sequence];
                         printf("( %d status %d value %g) ", sequence, getDynamicStatus(sequence), model->solutionRegion()[iBack]);
                         sequence = next_[sequence];
                    }
               }
               printf("\n");
               delete [] back;
          }
     }
#endif
     int n = numberActiveSets_;
     for (i = 0; i < numberSets_; i++) {
          if (toIndex_[i] < 0) {
               //assert(keyValue(i)>=lowerSet_[i]&&keyValue(i)<=upperSet_[i]);
               n++;
          }
	  int k=0;
	  for (int j=startSet_[i];j<startSet_[i+1];j++) {
	    if (getDynamicStatus(j)==soloKey)
	      k++;
	  }
	  assert (k<2);
     }
     assert (n == numberSets_);
#endif
     return 1;
}
void
ClpDynamicMatrix::times(double scalar,
                        const double * x, double * y) const
{
     if (model_->specialOptions() != 16) {
          ClpPackedMatrix::times(scalar, x, y);
     } else {
          int iRow;
          const double * element =  matrix_->getElements();
          const int * row = matrix_->getIndices();
          const CoinBigIndex * startColumn = matrix_->getVectorStarts();
          const int * length = matrix_->getVectorLengths();
          int * pivotVariable = model_->pivotVariable();
          for (iRow = 0; iRow < numberStaticRows_ + numberActiveSets_; iRow++) {
               y[iRow] -= scalar * rhsOffset_[iRow];
               int iColumn = pivotVariable[iRow];
               if (iColumn < lastDynamic_) {
                    CoinBigIndex j;
                    double value = scalar * x[iColumn];
                    if (value) {
                         for (j = startColumn[iColumn];
                                   j < startColumn[iColumn] + length[iColumn]; j++) {
                              int jRow = row[j];
                              y[jRow] += value * element[j];
                         }
                    }
               }
          }
     }
}
// Modifies rhs offset
void
ClpDynamicMatrix::modifyOffset(int sequence, double amount)
{
     if (amount) {
          assert (rhsOffset_);
          CoinBigIndex j;
          for (j = startColumn_[sequence]; j < startColumn_[sequence+1]; j++) {
               int iRow = row_[j];
               rhsOffset_[iRow] += amount * element_[j];
          }
     }
}
// Gets key value when none in small
double
ClpDynamicMatrix::keyValue(int iSet) const
{
     double value = 0.0;
     if (toIndex_[iSet] < 0) {
          int key = keyVariable_[iSet];
          if (key < maximumGubColumns_) {
               if (getStatus(iSet) == ClpSimplex::atLowerBound)
                    value = lowerSet_[iSet];
               else
                    value = upperSet_[iSet];
               int numberKey = 0;
               int j = startSet_[iSet];
               while (j >= 0) {
                    DynamicStatus status = getDynamicStatus(j);
                    assert (status != inSmall);
                    if (status == soloKey) {
                         numberKey++;
                    } else if (status == atUpperBound) {
                         value -= columnUpper_[j];
                    } else if (columnLower_) {
                         value -= columnLower_[j];
                    }
                    j = next_[j]; //onto next in set
               }
               assert (numberKey == 1);
          } else {
               int j = startSet_[iSet];
               while (j >= 0) {
                    DynamicStatus status = getDynamicStatus(j);
                    assert (status != inSmall);
                    assert (status != soloKey);
                    if (status == atUpperBound) {
                         value += columnUpper_[j];
                    } else if (columnLower_) {
                         value += columnLower_[j];
                    }
                    j = next_[j]; //onto next in set
               }
#if 0
	       // slack must be feasible
	       double oldValue=value;
	       value = CoinMax(value,lowerSet_[iSet]);
               value = CoinMin(value,upperSet_[iSet]);
	       if (value!=oldValue)
		 printf("using %g (not %g) for slack on set %d (%g,%g)\n",
			value,oldValue,iSet,lowerSet_[iSet],upperSet_[iSet]);
#endif
            }
     }
     return value;
}
// Switches off dj checking each factorization (for BIG models)
void
ClpDynamicMatrix::switchOffCheck()
{
     noCheck_ = 0;
     infeasibilityWeight_ = 0.0;
}
/* Creates a variable.  This is called after partial pricing and may modify matrix.
   May update bestSequence.
*/
void
ClpDynamicMatrix::createVariable(ClpSimplex * model, int & bestSequence)
{
     int numberRows = model->numberRows();
     int slackOffset = lastDynamic_ + numberRows;
     int structuralOffset = slackOffset + numberSets_;
     int bestSequence2 = savedBestSequence_ - structuralOffset;
     if (bestSequence >= slackOffset) {
          double * columnLower = model->lowerRegion();
          double * columnUpper = model->upperRegion();
          double * solution = model->solutionRegion();
          double * reducedCost = model->djRegion();
          const double * duals = model->dualRowSolution();
          if (toIndex_[savedBestSet_] < 0) {
               // need to put key into basis
               int newRow = numberActiveSets_ + numberStaticRows_;
               model->dualRowSolution()[newRow] = savedBestGubDual_;
               double valueOfKey = keyValue(savedBestSet_); // done before toIndex_ set
               toIndex_[savedBestSet_] = numberActiveSets_;
               fromIndex_[numberActiveSets_++] = savedBestSet_;
               int iSequence = lastDynamic_ + newRow;
               // we need to get lower and upper correct
               double shift = 0.0;
               int j = startSet_[savedBestSet_];
               while (j >= 0) {
                    if (getDynamicStatus(j) == atUpperBound)
                         shift += columnUpper_[j];
                    else if (getDynamicStatus(j) == atLowerBound && columnLower_)
                         shift += columnLower_[j];
                    j = next_[j]; //onto next in set
               }
               if (lowerSet_[savedBestSet_] > -1.0e20)
                    columnLower[iSequence] = lowerSet_[savedBestSet_];
               else
                    columnLower[iSequence] = -COIN_DBL_MAX;
               if (upperSet_[savedBestSet_] < 1.0e20)
                    columnUpper[iSequence] = upperSet_[savedBestSet_];
               else
                    columnUpper[iSequence] = COIN_DBL_MAX;
#ifdef CLP_DEBUG
               if (model_->logLevel() == 63) {
                    printf("%d in in set %d, key is %d rhs %g %g - keyvalue %g\n",
                           bestSequence2, savedBestSet_, keyVariable_[savedBestSet_],
                           columnLower[iSequence], columnUpper[iSequence], valueOfKey);
                    int j = startSet_[savedBestSet_];
                    while (j >= 0) {
                         if (getDynamicStatus(j) == atUpperBound)
                              printf("%d atup ", j);
                         else if (getDynamicStatus(j) == atLowerBound)
                              printf("%d atlo ", j);
                         else if (getDynamicStatus(j) == soloKey)
                              printf("%d solo ", j);
                         else
                              abort();
                         j = next_[j]; //onto next in set
                    }
                    printf("\n");
               }
#endif
               if (keyVariable_[savedBestSet_] < maximumGubColumns_) {
                    // slack not key
                    model_->pivotVariable()[newRow] = firstAvailable_;
                    backToPivotRow_[firstAvailable_] = newRow;
                    model->setStatus(iSequence, getStatus(savedBestSet_));
                    model->djRegion()[iSequence] = savedBestGubDual_;
                    solution[iSequence] = valueOfKey;
		    // create variable and pivot in
                    int key = keyVariable_[savedBestSet_];
                    setDynamicStatus(key, inSmall);
                    double * element =  matrix_->getMutableElements();
                    int * row = matrix_->getMutableIndices();
                    CoinBigIndex * startColumn = matrix_->getMutableVectorStarts();
                    int * length = matrix_->getMutableVectorLengths();
                    CoinBigIndex numberElements = startColumn[firstAvailable_];
                    int numberThis = startColumn_[key+1] - startColumn_[key] + 1;
                    if (numberElements + numberThis > numberElements_) {
                         // need to redo
                         numberElements_ = CoinMax(3 * numberElements_ / 2, numberElements + numberThis);
                         matrix_->reserve(lastDynamic_, numberElements_);
                         element =  matrix_->getMutableElements();
                         row = matrix_->getMutableIndices();
                         // these probably okay but be safe
                         startColumn = matrix_->getMutableVectorStarts();
                         length = matrix_->getMutableVectorLengths();
                    }
                    // already set startColumn[firstAvailable_]=numberElements;
                    length[firstAvailable_] = numberThis;
                    model->costRegion()[firstAvailable_] = cost_[key];
                    CoinBigIndex base = startColumn_[key];
                    for (int j = 0; j < numberThis - 1; j++) {
                         row[numberElements] = row_[base+j];
                         element[numberElements++] = element_[base+j];
                    }
                    row[numberElements] = newRow;
                    element[numberElements++] = 1.0;
                    id_[firstAvailable_-firstDynamic_] = key;
                    model->setObjectiveOffset(model->objectiveOffset() + cost_[key]*
                                              valueOfKey);
                    model->solutionRegion()[firstAvailable_] = valueOfKey;
                    model->setStatus(firstAvailable_, ClpSimplex::basic);
                    // ***** need to adjust effective rhs
                    if (!columnLower_ && !columnUpper_) {
                         columnLower[firstAvailable_] = 0.0;
                         columnUpper[firstAvailable_] = COIN_DBL_MAX;
                    }  else {
                         if (columnLower_)
                              columnLower[firstAvailable_] = columnLower_[key];
                         else
                              columnLower[firstAvailable_] = 0.0;
                         if (columnUpper_)
                              columnUpper[firstAvailable_] = columnUpper_[key];
                         else
                              columnUpper[firstAvailable_] = COIN_DBL_MAX;
                    }
                    model->nonLinearCost()->setOne(firstAvailable_, solution[firstAvailable_],
                                                   columnLower[firstAvailable_],
                                                   columnUpper[firstAvailable_], cost_[key]);
                    startColumn[firstAvailable_+1] = numberElements;
                    reducedCost[firstAvailable_] = 0.0;
                    modifyOffset(key, valueOfKey);
                    rhsOffset_[newRow] = -shift; // sign?
#ifdef CLP_DEBUG
                    model->rowArray(1)->checkClear();
#endif
                    // now pivot in
                    unpack(model, model->rowArray(1), firstAvailable_);
                    model->factorization()->updateColumnFT(model->rowArray(2), model->rowArray(1));
                    double alpha = model->rowArray(1)->denseVector()[newRow];
                    int updateStatus =
                         model->factorization()->replaceColumn(model,
                                   model->rowArray(2),
                                   model->rowArray(1),
                                   newRow, alpha);
                    model->rowArray(1)->clear();
                    if (updateStatus) {
                         if (updateStatus == 3) {
                              // out of memory
                              // increase space if not many iterations
                              if (model->factorization()->pivots() <
                                        0.5 * model->factorization()->maximumPivots() &&
                                        model->factorization()->pivots() < 400)
                                   model->factorization()->areaFactor(
                                        model->factorization()->areaFactor() * 1.1);
                         } else {
                              printf("Bad returncode %d from replaceColumn\n", updateStatus);
                         }
                         bestSequence = -1;
                         return;
                    }
                    // firstAvailable_ only finally updated if good pivot (in updatePivot)
                    // otherwise it reverts to firstAvailableBefore_
                    firstAvailable_++;
               } else {
                    // slack key
                    model->setStatus(iSequence, ClpSimplex::basic);
                    model->djRegion()[iSequence] = 0.0;
                    solution[iSequence] = valueOfKey+shift;
                    rhsOffset_[newRow] = -shift; // sign?
               }
               // correct slack
               model->costRegion()[iSequence] = 0.0;
               model->nonLinearCost()->setOne(iSequence, solution[iSequence], columnLower[iSequence],
                                              columnUpper[iSequence], 0.0);
          }
          if (savedBestSequence_ >= structuralOffset) {
               // recompute dj and create
               double value = cost_[bestSequence2] - savedBestGubDual_;
               for (CoinBigIndex jBigIndex = startColumn_[bestSequence2];
                         jBigIndex < startColumn_[bestSequence2+1]; jBigIndex++) {
                    int jRow = row_[jBigIndex];
                    value -= duals[jRow] * element_[jBigIndex];
               }
               int gubRow = toIndex_[savedBestSet_] + numberStaticRows_;
               double * element =  matrix_->getMutableElements();
               int * row = matrix_->getMutableIndices();
               CoinBigIndex * startColumn = matrix_->getMutableVectorStarts();
               int * length = matrix_->getMutableVectorLengths();
               CoinBigIndex numberElements = startColumn[firstAvailable_];
               int numberThis = startColumn_[bestSequence2+1] - startColumn_[bestSequence2] + 1;
               if (numberElements + numberThis > numberElements_) {
                    // need to redo
                    numberElements_ = CoinMax(3 * numberElements_ / 2, numberElements + numberThis);
                    matrix_->reserve(lastDynamic_, numberElements_);
                    element =  matrix_->getMutableElements();
                    row = matrix_->getMutableIndices();
                    // these probably okay but be safe
                    startColumn = matrix_->getMutableVectorStarts();
                    length = matrix_->getMutableVectorLengths();
               }
               // already set startColumn[firstAvailable_]=numberElements;
               length[firstAvailable_] = numberThis;
               model->costRegion()[firstAvailable_] = cost_[bestSequence2];
               CoinBigIndex base = startColumn_[bestSequence2];
               for (int j = 0; j < numberThis - 1; j++) {
                    row[numberElements] = row_[base+j];
                    element[numberElements++] = element_[base+j];
               }
               row[numberElements] = gubRow;
               element[numberElements++] = 1.0;
               id_[firstAvailable_-firstDynamic_] = bestSequence2;
               //printf("best %d\n",bestSequence2);
               model->solutionRegion()[firstAvailable_] = 0.0;
	       model->clearFlagged(firstAvailable_);
               if (!columnLower_ && !columnUpper_) {
                    model->setStatus(firstAvailable_, ClpSimplex::atLowerBound);
                    columnLower[firstAvailable_] = 0.0;
                    columnUpper[firstAvailable_] = COIN_DBL_MAX;
               }  else {
                    DynamicStatus status = getDynamicStatus(bestSequence2);
                    if (columnLower_)
                         columnLower[firstAvailable_] = columnLower_[bestSequence2];
                    else
                         columnLower[firstAvailable_] = 0.0;
                    if (columnUpper_)
                         columnUpper[firstAvailable_] = columnUpper_[bestSequence2];
                    else
                         columnUpper[firstAvailable_] = COIN_DBL_MAX;
                    if (status == atLowerBound) {
                         solution[firstAvailable_] = columnLower[firstAvailable_];
                         model->setStatus(firstAvailable_, ClpSimplex::atLowerBound);
                    } else {
                         solution[firstAvailable_] = columnUpper[firstAvailable_];
                         model->setStatus(firstAvailable_, ClpSimplex::atUpperBound);
                    }
               }
               model->setObjectiveOffset(model->objectiveOffset() + cost_[bestSequence2]*
                                         solution[firstAvailable_]);
               model->nonLinearCost()->setOne(firstAvailable_, solution[firstAvailable_],
                                              columnLower[firstAvailable_],
                                              columnUpper[firstAvailable_], cost_[bestSequence2]);
               bestSequence = firstAvailable_;
               // firstAvailable_ only updated if good pivot (in updatePivot)
               startColumn[firstAvailable_+1] = numberElements;
               //printf("price struct %d - dj %g gubpi %g\n",bestSequence,value,savedBestGubDual_);
               reducedCost[bestSequence] = value;
          } else {
               // slack - row must just have been created
               assert (toIndex_[savedBestSet_] == numberActiveSets_ - 1);
               int newRow = numberStaticRows_ + numberActiveSets_ - 1;
               bestSequence = lastDynamic_ + newRow;
               reducedCost[bestSequence] = savedBestGubDual_;
          }
     }
     // clear for next iteration
     savedBestSequence_ = -1;
}
// Returns reduced cost of a variable
double
ClpDynamicMatrix::reducedCost(ClpSimplex * model, int sequence) const
{
     int numberRows = model->numberRows();
     int slackOffset = lastDynamic_ + numberRows;
     if (sequence < slackOffset)
          return model->djRegion()[sequence];
     else
          return savedBestDj_;
}
// Does gub crash
void
ClpDynamicMatrix::gubCrash()
{
     // Do basis - cheapest or slack if feasible
     int longestSet = 0;
     int iSet;
     for (iSet = 0; iSet < numberSets_; iSet++) {
          int n = 0;
          int j = startSet_[iSet];
          while (j >= 0) {
               n++;
               j = next_[j];
          }
          longestSet = CoinMax(longestSet, n);
     }
     double * upper = new double[longestSet+1];
     double * cost = new double[longestSet+1];
     double * lower = new double[longestSet+1];
     double * solution = new double[longestSet+1];
     int * back = new int[longestSet+1];
     double tolerance = model_->primalTolerance();
     double objectiveOffset = 0.0;
     for (iSet = 0; iSet < numberSets_; iSet++) {
          int iBasic = -1;
          double value = 0.0;
          // find cheapest
          int numberInSet = 0;
          int j = startSet_[iSet];
          while (j >= 0) {
               if (!columnLower_)
                    lower[numberInSet] = 0.0;
               else
                    lower[numberInSet] = columnLower_[j];
               if (!columnUpper_)
                    upper[numberInSet] = COIN_DBL_MAX;
               else
                    upper[numberInSet] = columnUpper_[j];
               back[numberInSet++] = j;
               j = next_[j];
          }
          CoinFillN(solution, numberInSet, 0.0);
          // and slack
          iBasic = numberInSet;
          solution[iBasic] = -value;
          lower[iBasic] = -upperSet_[iSet];
          upper[iBasic] = -lowerSet_[iSet];
          int kphase;
          if (value < lowerSet_[iSet] - tolerance || value > upperSet_[iSet] + tolerance) {
               // infeasible
               kphase = 0;
               // remember bounds are flipped so opposite to natural
               if (value < lowerSet_[iSet] - tolerance)
                    cost[iBasic] = 1.0;
               else
                    cost[iBasic] = -1.0;
               CoinZeroN(cost, numberInSet);
               double dualTolerance = model_->dualTolerance();
               for (int iphase = kphase; iphase < 2; iphase++) {
                    if (iphase) {
                         cost[numberInSet] = 0.0;
                         for (int j = 0; j < numberInSet; j++)
                              cost[j] = cost_[back[j]];
                    }
                    // now do one row lp
                    bool improve = true;
                    while (improve) {
                         improve = false;
                         double dual = cost[iBasic];
                         int chosen = -1;
                         double best = dualTolerance;
                         int way = 0;
                         for (int i = 0; i <= numberInSet; i++) {
                              double dj = cost[i] - dual;
                              double improvement = 0.0;
                              if (iphase || i < numberInSet)
                                   assert (solution[i] >= lower[i] && solution[i] <= upper[i]);
                              if (dj > dualTolerance)
                                   improvement = dj * (solution[i] - lower[i]);
                              else if (dj < -dualTolerance)
                                   improvement = dj * (solution[i] - upper[i]);
                              if (improvement > best) {
                                   best = improvement;
                                   chosen = i;
                                   if (dj < 0.0) {
                                        way = 1;
                                   } else {
                                        way = -1;
                                   }
                              }
                         }
                         if (chosen >= 0) {
                              improve = true;
                              // now see how far
                              if (way > 0) {
                                   // incoming increasing so basic decreasing
                                   // if phase 0 then go to nearest bound
                                   double distance = upper[chosen] - solution[chosen];
                                   double basicDistance;
                                   if (!iphase) {
                                        assert (iBasic == numberInSet);
                                        assert (solution[iBasic] > upper[iBasic]);
                                        basicDistance = solution[iBasic] - upper[iBasic];
                                   } else {
                                        basicDistance = solution[iBasic] - lower[iBasic];
                                   }
                                   // need extra coding for unbounded
                                   assert (CoinMin(distance, basicDistance) < 1.0e20);
                                   if (distance > basicDistance) {
                                        // incoming becomes basic
                                        solution[chosen] += basicDistance;
                                        if (!iphase)
                                             solution[iBasic] = upper[iBasic];
                                        else
                                             solution[iBasic] = lower[iBasic];
                                        iBasic = chosen;
                                   } else {
                                        // flip
                                        solution[chosen] = upper[chosen];
                                        solution[iBasic] -= distance;
                                   }
                              } else {
                                   // incoming decreasing so basic increasing
                                   // if phase 0 then go to nearest bound
                                   double distance = solution[chosen] - lower[chosen];
                                   double basicDistance;
                                   if (!iphase) {
                                        assert (iBasic == numberInSet);
                                        assert (solution[iBasic] < lower[iBasic]);
                                        basicDistance = lower[iBasic] - solution[iBasic];
                                   } else {
                                        basicDistance = upper[iBasic] - solution[iBasic];
                                   }
                                   // need extra coding for unbounded - for now just exit
                                   if (CoinMin(distance, basicDistance) > 1.0e20) {
                                        printf("unbounded on set %d\n", iSet);
                                        iphase = 1;
                                        iBasic = numberInSet;
                                        break;
                                   }
                                   if (distance > basicDistance) {
                                        // incoming becomes basic
                                        solution[chosen] -= basicDistance;
                                        if (!iphase)
                                             solution[iBasic] = lower[iBasic];
                                        else
                                             solution[iBasic] = upper[iBasic];
                                        iBasic = chosen;
                                   } else {
                                        // flip
                                        solution[chosen] = lower[chosen];
                                        solution[iBasic] += distance;
                                   }
                              }
                              if (!iphase) {
                                   if(iBasic < numberInSet)
                                        break; // feasible
                                   else if (solution[iBasic] >= lower[iBasic] &&
                                             solution[iBasic] <= upper[iBasic])
                                        break; // feasible (on flip)
                              }
                         }
                    }
               }
          }
          // do solution i.e. bounds
          if (columnLower_ || columnUpper_) {
               for (int j = 0; j < numberInSet; j++) {
                    if (j != iBasic) {
                         objectiveOffset += solution[j] * cost[j];
                         if (columnLower_ && columnUpper_) {
                              if (fabs(solution[j] - columnLower_[back[j]]) >
                                        fabs(solution[j] - columnUpper_[back[j]]))
                                   setDynamicStatus(back[j], atUpperBound);
                         } else if (columnUpper_ && solution[j] > 0.0) {
                              setDynamicStatus(back[j], atUpperBound);
                         } else {
                              setDynamicStatus(back[j], atLowerBound);
                              assert(!solution[j]);
                         }
                    }
               }
          }
          // convert iBasic back and do bounds
          if (iBasic == numberInSet) {
               // slack basic
               setStatus(iSet, ClpSimplex::basic);
               iBasic = iSet + maximumGubColumns_;
          } else {
               iBasic = back[iBasic];
               setDynamicStatus(iBasic, soloKey);
               // remember bounds flipped
               if (upper[numberInSet] == lower[numberInSet])
                    setStatus(iSet, ClpSimplex::isFixed);
               else if (solution[numberInSet] == upper[numberInSet])
                    setStatus(iSet, ClpSimplex::atLowerBound);
               else if (solution[numberInSet] == lower[numberInSet])
                    setStatus(iSet, ClpSimplex::atUpperBound);
               else
                    abort();
          }
          keyVariable_[iSet] = iBasic;
     }
     model_->setObjectiveOffset(objectiveOffset_ - objectiveOffset);
     delete [] lower;
     delete [] solution;
     delete [] upper;
     delete [] cost;
     delete [] back;
     // make sure matrix is in good shape
     matrix_->orderMatrix();
}
// Populates initial matrix from dynamic status
void
ClpDynamicMatrix::initialProblem()
{
     int iSet;
     double * element =  matrix_->getMutableElements();
     int * row = matrix_->getMutableIndices();
     CoinBigIndex * startColumn = matrix_->getMutableVectorStarts();
     int * length = matrix_->getMutableVectorLengths();
     double * cost = model_->objective();
     double * solution = model_->primalColumnSolution();
     double * columnLower = model_->columnLower();
     double * columnUpper = model_->columnUpper();
     double * rowSolution = model_->primalRowSolution();
     double * rowLower = model_->rowLower();
     double * rowUpper = model_->rowUpper();
     CoinBigIndex numberElements = startColumn[firstDynamic_];

     firstAvailable_ = firstDynamic_;
     numberActiveSets_ = 0;
     for (iSet = 0; iSet < numberSets_; iSet++) {
          toIndex_[iSet] = -1;
          int numberActive = 0;
          int whichKey = -1;
          if (getStatus(iSet) == ClpSimplex::basic) {
               whichKey = maximumGubColumns_ + iSet;
	       numberActive = 1;
          } else {
               whichKey = -1;
	  }
          int j = startSet_[iSet];
          while (j >= 0) {
               assert (getDynamicStatus(j) != soloKey || whichKey == -1);
               if (getDynamicStatus(j) == inSmall) {
                    numberActive++;
               } else if (getDynamicStatus(j) == soloKey) {
                    whichKey = j;
                    numberActive++;
	       }
               j = next_[j]; //onto next in set
          }
          if (numberActive > 1) {
               int iRow = numberActiveSets_ + numberStaticRows_;
               rowSolution[iRow] = 0.0;
               double lowerValue;
               if (lowerSet_[iSet] > -1.0e20)
                    lowerValue = lowerSet_[iSet];
               else
                    lowerValue = -COIN_DBL_MAX;
               double upperValue;
               if (upperSet_[iSet] < 1.0e20)
                    upperValue = upperSet_[iSet];
               else
                    upperValue = COIN_DBL_MAX;
               rowLower[iRow] = lowerValue;
               rowUpper[iRow] = upperValue;
               if (getStatus(iSet) == ClpSimplex::basic) {
                    model_->setRowStatus(iRow, ClpSimplex::basic);
                    rowSolution[iRow] = 0.0;
               } else if (getStatus(iSet) == ClpSimplex::atLowerBound) {
                    model_->setRowStatus(iRow, ClpSimplex::atLowerBound);
                    rowSolution[iRow] = lowerValue;
               } else {
                    model_->setRowStatus(iRow, ClpSimplex::atUpperBound);
                    rowSolution[iRow] = upperValue;
               }
               j = startSet_[iSet];
               while (j >= 0) {
                    DynamicStatus status = getDynamicStatus(j);
                    if (status == inSmall) {
                         int numberThis = startColumn_[j+1] - startColumn_[j] + 1;
                         if (numberElements + numberThis > numberElements_) {
                              // need to redo
                              numberElements_ = CoinMax(3 * numberElements_ / 2, numberElements + numberThis);
                              matrix_->reserve(lastDynamic_, numberElements_);
                              element =  matrix_->getMutableElements();
                              row = matrix_->getMutableIndices();
                              // these probably okay but be safe
                              startColumn = matrix_->getMutableVectorStarts();
                              length = matrix_->getMutableVectorLengths();
                         }
                         length[firstAvailable_] = numberThis;
                         cost[firstAvailable_] = cost_[j];
                         CoinBigIndex base = startColumn_[j];
                         for (int k = 0; k < numberThis - 1; k++) {
                              row[numberElements] = row_[base+k];
                              element[numberElements++] = element_[base+k];
                         }
                         row[numberElements] = iRow;
                         element[numberElements++] = 1.0;
                         id_[firstAvailable_-firstDynamic_] = j;
                         solution[firstAvailable_] = 0.0;
                         model_->setStatus(firstAvailable_, ClpSimplex::basic);
                         if (!columnLower_ && !columnUpper_) {
                              columnLower[firstAvailable_] = 0.0;
                              columnUpper[firstAvailable_] = COIN_DBL_MAX;
                         }  else {
                              if (columnLower_)
                                   columnLower[firstAvailable_] = columnLower_[j];
                              else
                                   columnLower[firstAvailable_] = 0.0;
                              if (columnUpper_)
                                   columnUpper[firstAvailable_] = columnUpper_[j];
                              else
                                   columnUpper[firstAvailable_] = COIN_DBL_MAX;
                              if (status != atUpperBound) {
                                   solution[firstAvailable_] = columnLower[firstAvailable_];
                              } else {
                                   solution[firstAvailable_] = columnUpper[firstAvailable_];
                              }
                         }
                         firstAvailable_++;
                         startColumn[firstAvailable_] = numberElements;
                    }
                    j = next_[j]; //onto next in set
               }
               model_->setRowStatus(numberActiveSets_ + numberStaticRows_, getStatus(iSet));
               toIndex_[iSet] = numberActiveSets_;
               fromIndex_[numberActiveSets_++] = iSet;
	  } else {
	    // solo key
	    bool needKey=false;
	    if (numberActive) {
	      if (whichKey<maximumGubColumns_) {
		// structural - assume ok
		needKey = false;
	      } else {
		// slack
		keyVariable_[iSet] = maximumGubColumns_ + iSet;
		double value = keyValue(iSet);
		if (value<lowerSet_[iSet]-1.0e-8||
		    value>upperSet_[iSet]+1.0e-8)
		  needKey=true;
	      }
	    } else {
	      needKey = true;
	    }
	    if (needKey) {
	      // all to lb then up some (slack/null if possible)
	      int length=99999999;
	      int which=-1;
	      double sum=0.0;
	      for (int iColumn=startSet_[iSet];iColumn<startSet_[iSet+1];iColumn++) {
		setDynamicStatus(iColumn,atLowerBound);
		sum += columnLower_[iColumn];
		if (length>startColumn_[iColumn+1]-startColumn_[iColumn]) {
		  which=iColumn;
		  length=startColumn_[iColumn+1]-startColumn_[iColumn];
		}
	      }
	      if (sum>lowerSet_[iSet]-1.0e-8) {
		// slack can be basic
		setStatus(iSet,ClpSimplex::basic);
		keyVariable_[iSet] = maximumGubColumns_ + iSet;
	      } else {
		// use shortest
		setDynamicStatus(which,soloKey);
		keyVariable_[iSet] = which;
		setStatus(iSet,ClpSimplex::atLowerBound);
	      }
	    }
          }
          assert (toIndex_[iSet] >= 0 || whichKey >= 0);
          keyVariable_[iSet] = whichKey;
     }
     // clean up pivotVariable
     int numberColumns = model_->numberColumns();
     int numberRows = model_->numberRows();
     int * pivotVariable = model_->pivotVariable();
     if (pivotVariable) {
       for (int i=0; i<numberStaticRows_+numberActiveSets_;i++) {
	 if (model_->getRowStatus(i)!=ClpSimplex::basic)
	   pivotVariable[i]=-1;
	 else
	   pivotVariable[i]=numberColumns+i;
       }
       for (int i=numberStaticRows_+numberActiveSets_;i<numberRows;i++) {
	 pivotVariable[i]=i+numberColumns;
       }
       int put=-1;
       for (int i=0;i<numberColumns;i++) {
	 if (model_->getColumnStatus(i)==ClpSimplex::basic) {
	   while(put<numberRows) {
	     put++;
	     if (pivotVariable[put]==-1) {
	       pivotVariable[put]=i;
	       break;
	     }
	   }
	 }
       }
       for (int i=CoinMax(put,0);i<numberRows;i++) {
	 if (pivotVariable[i]==-1) 
	   pivotVariable[i]=i+numberColumns;
       }
     }
     if (rhsOffset_) {
       double * cost = model_->costRegion();
       double * columnLower = model_->lowerRegion();
       double * columnUpper = model_->upperRegion();
       double * solution = model_->solutionRegion();
       int numberRows = model_->numberRows();
       for (int i = numberActiveSets_; i < numberRows-numberStaticRows_; i++) {
	 int iSequence = i + numberStaticRows_ + numberColumns;
	 solution[iSequence] = 0.0;
	 columnLower[iSequence] = -COIN_DBL_MAX;
	 columnUpper[iSequence] = COIN_DBL_MAX;
	 cost[iSequence] = 0.0;
	 model_->nonLinearCost()->setOne(iSequence, solution[iSequence],
					columnLower[iSequence],
					columnUpper[iSequence], 0.0);
	 model_->setStatus(iSequence, ClpSimplex::basic);
	 rhsOffset_[i+numberStaticRows_] = 0.0;
       }
#if 0
       for (int i=0;i<numberStaticRows_;i++)
	 printf("%d offset %g\n",
		i,rhsOffset_[i]);
#endif
     }
     numberActiveColumns_ = firstAvailable_;
#if 0
     for (iSet = 0; iSet < numberSets_; iSet++) {
       for (int j=startSet_[iSet];j<startSet_[iSet+1];j++) {
	 if (getDynamicStatus(j)==soloKey)
	   printf("%d in set %d is solo key\n",j,iSet);
	 else if (getDynamicStatus(j)==inSmall)
	   printf("%d in set %d is in small\n",j,iSet);
       }
     }
#endif
     return;
}
// Writes out model (without names)
void 
ClpDynamicMatrix::writeMps(const char * name)
{
  int numberTotalRows = numberStaticRows_+numberSets_;
  int numberTotalColumns = firstDynamic_+numberGubColumns_;
  // over estimate
  int numberElements = getNumElements()+startColumn_[numberGubColumns_]
    + numberGubColumns_;
  double * columnLower = new double [numberTotalColumns];
  double * columnUpper = new double [numberTotalColumns];
  double * cost = new double [numberTotalColumns];
  double * rowLower = new double [numberTotalRows];
  double * rowUpper = new double [numberTotalRows];
  CoinBigIndex * start = new CoinBigIndex[numberTotalColumns+1];
  int * row = new int [numberElements];
  double * element = new double [numberElements];
  // Fill in
  const CoinBigIndex * startA = getVectorStarts();
  const int * lengthA = getVectorLengths();
  const int * rowA = getIndices();
  const double * elementA = getElements();
  const double * columnLowerA = model_->columnLower();
  const double * columnUpperA = model_->columnUpper();
  const double * costA = model_->objective();
  const double * rowLowerA = model_->rowLower();
  const double * rowUpperA = model_->rowUpper();
  start[0]=0;
  numberElements=0;
  for (int i=0;i<firstDynamic_;i++) {
    columnLower[i] = columnLowerA[i];
    columnUpper[i] = columnUpperA[i];
    cost[i] = costA[i];
    for (CoinBigIndex j = startA[i];j<startA[i]+lengthA[i];j++) {
      row[numberElements] = rowA[j];
      element[numberElements++]=elementA[j];
    }
    start[i+1]=numberElements;
  }
  for (int i=0;i<numberStaticRows_;i++) {
    rowLower[i] = rowLowerA[i];
    rowUpper[i] = rowUpperA[i];
  }
  int putC=firstDynamic_;
  int putR=numberStaticRows_;
  for (int i=0;i<numberSets_;i++) {
    rowLower[putR]=lowerSet_[i];
    rowUpper[putR]=upperSet_[i];
    for (CoinBigIndex k=startSet_[i];k<startSet_[i+1];k++) {
      columnLower[putC]=columnLower_[k];
      columnUpper[putC]=columnUpper_[k];
      cost[putC]=cost_[k];
      putC++;
      for (CoinBigIndex j = startColumn_[k];j<startColumn_[k+1];j++) {
	row[numberElements] = row_[j];
	element[numberElements++]=element_[j];
      }
      row[numberElements] = putR;
      element[numberElements++]=1.0;
      start[putC]=numberElements;
    }
    putR++;
  }

  assert (putR==numberTotalRows);
  assert (putC==numberTotalColumns);
  ClpSimplex modelOut;
  modelOut.loadProblem(numberTotalColumns,numberTotalRows,
		       start,row,element,
		       columnLower,columnUpper,cost,
		       rowLower,rowUpper);
  modelOut.writeMps(name);
  delete [] columnLower;
  delete [] columnUpper;
  delete [] cost;
  delete [] rowLower;
  delete [] rowUpper;
  delete [] start;
  delete [] row; 
  delete [] element;
}
// Adds in a column to gub structure (called from descendant)
int
ClpDynamicMatrix::addColumn(int numberEntries, const int * row, const double * element,
                            double cost, double lower, double upper, int iSet,
                            DynamicStatus status)
{
     // check if already in
     int j = startSet_[iSet];
     while (j >= 0) {
          if (startColumn_[j+1] - startColumn_[j] == numberEntries) {
               const int * row2 = row_ + startColumn_[j];
               const double * element2 = element_ + startColumn_[j];
               bool same = true;
               for (int k = 0; k < numberEntries; k++) {
                    if (row[k] != row2[k] || element[k] != element2[k]) {
                         same = false;
                         break;
                    }
               }
               if (same) {
                    bool odd = false;
                    if (cost != cost_[j])
                         odd = true;
                    if (columnLower_ && lower != columnLower_[j])
                         odd = true;
                    if (columnUpper_ && upper != columnUpper_[j])
                         odd = true;
                    if (odd) {
                         printf("seems odd - same els but cost,lo,up are %g,%g,%g and %g,%g,%g\n",
                                cost, lower, upper, cost_[j],
                                columnLower_ ? columnLower_[j] : 0.0,
                                columnUpper_ ? columnUpper_[j] : 1.0e100);
                    } else {
		         setDynamicStatus(j, status);
                         return j;
		    }
               }
          }
          j = next_[j];
     }

     if (numberGubColumns_ == maximumGubColumns_ ||
               startColumn_[numberGubColumns_] + numberEntries > maximumElements_) {
          CoinBigIndex j;
          int i;
          int put = 0;
          int numberElements = 0;
          CoinBigIndex start = 0;
          // compress - leave ones at ub and basic
          int * which = new int [numberGubColumns_];
          for (i = 0; i < numberGubColumns_; i++) {
               CoinBigIndex end = startColumn_[i+1];
	       // what about ubs if column generation?
               if (getDynamicStatus(i) != atLowerBound) {
                    // keep in
                    for (j = start; j < end; j++) {
                         row_[numberElements] = row_[j];
                         element_[numberElements++] = element_[j];
                    }
                    startColumn_[put+1] = numberElements;
                    cost_[put] = cost_[i];
                    if (columnLower_)
                         columnLower_[put] = columnLower_[i];
                    if (columnUpper_)
                         columnUpper_[put] = columnUpper_[i];
                    dynamicStatus_[put] = dynamicStatus_[i];
                    id_[put] = id_[i];
                    which[i] = put;
                    put++;
               } else {
                    // out
                    which[i] = -1;
               }
               start = end;
          }
          // now redo startSet_ and next_
          int * newNext = new int [maximumGubColumns_];
          for (int jSet = 0; jSet < numberSets_; jSet++) {
               int sequence = startSet_[jSet];
               while (which[sequence] < 0) {
                    // out
                    assert (next_[sequence] >= 0);
                    sequence = next_[sequence];
               }
               startSet_[jSet] = which[sequence];
               int last = which[sequence];
               while (next_[sequence] >= 0) {
                    sequence = next_[sequence];
                    if(which[sequence] >= 0) {
                         // keep
                         newNext[last] = which[sequence];
                         last = which[sequence];
                    }
               }
               newNext[last] = -jSet - 1;
          }
          delete [] next_;
          next_ = newNext;
          delete [] which;
          abort();
     }
     CoinBigIndex start = startColumn_[numberGubColumns_];
     CoinMemcpyN(row, numberEntries, row_ + start);
     CoinMemcpyN(element, numberEntries, element_ + start);
     startColumn_[numberGubColumns_+1] = start + numberEntries;
     cost_[numberGubColumns_] = cost;
     if (columnLower_)
          columnLower_[numberGubColumns_] = lower;
     else
          assert (!lower);
     if (columnUpper_)
          columnUpper_[numberGubColumns_] = upper;
     else
          assert (upper > 1.0e20);
     setDynamicStatus(numberGubColumns_, status);
     // Do next_
     j = startSet_[iSet];
     startSet_[iSet] = numberGubColumns_;
     next_[numberGubColumns_] = j;
     numberGubColumns_++;
     return numberGubColumns_ - 1;
}
// Returns which set a variable is in
int
ClpDynamicMatrix::whichSet (int sequence) const
{
     while (next_[sequence] >= 0)
          sequence = next_[sequence];
     int iSet = - next_[sequence] - 1;
     return iSet;
}
