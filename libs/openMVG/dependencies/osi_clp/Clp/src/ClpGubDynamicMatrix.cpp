/* $Id$ */
// Copyright (C) 2002, International Business Machines
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
#include "ClpGubDynamicMatrix.hpp"
#include "ClpMessage.hpp"
//#define CLP_DEBUG
//#define CLP_DEBUG_PRINT
//#############################################################################
// Constructors / Destructor / Assignment
//#############################################################################

//-------------------------------------------------------------------
// Default Constructor
//-------------------------------------------------------------------
ClpGubDynamicMatrix::ClpGubDynamicMatrix ()
     : ClpGubMatrix(),
       objectiveOffset_(0.0),
       startColumn_(NULL),
       row_(NULL),
       element_(NULL),
       cost_(NULL),
       fullStart_(NULL),
       id_(NULL),
       dynamicStatus_(NULL),
       lowerColumn_(NULL),
       upperColumn_(NULL),
       lowerSet_(NULL),
       upperSet_(NULL),
       numberGubColumns_(0),
       firstAvailable_(0),
       savedFirstAvailable_(0),
       firstDynamic_(0),
       lastDynamic_(0),
       numberElements_(0)
{
     setType(13);
}

//-------------------------------------------------------------------
// Copy constructor
//-------------------------------------------------------------------
ClpGubDynamicMatrix::ClpGubDynamicMatrix (const ClpGubDynamicMatrix & rhs)
     : ClpGubMatrix(rhs)
{
     objectiveOffset_ = rhs.objectiveOffset_;
     numberGubColumns_ = rhs.numberGubColumns_;
     firstAvailable_ = rhs.firstAvailable_;
     savedFirstAvailable_ = rhs.savedFirstAvailable_;
     firstDynamic_ = rhs.firstDynamic_;
     lastDynamic_ = rhs.lastDynamic_;
     numberElements_ = rhs.numberElements_;
     startColumn_ = ClpCopyOfArray(rhs.startColumn_, numberGubColumns_ + 1);
     CoinBigIndex numberElements = startColumn_[numberGubColumns_];
     row_ = ClpCopyOfArray(rhs.row_, numberElements);;
     element_ = ClpCopyOfArray(rhs.element_, numberElements);;
     cost_ = ClpCopyOfArray(rhs.cost_, numberGubColumns_);
     fullStart_ = ClpCopyOfArray(rhs.fullStart_, numberSets_ + 1);
     id_ = ClpCopyOfArray(rhs.id_, lastDynamic_ - firstDynamic_);
     lowerColumn_ = ClpCopyOfArray(rhs.lowerColumn_, numberGubColumns_);
     upperColumn_ = ClpCopyOfArray(rhs.upperColumn_, numberGubColumns_);
     dynamicStatus_ = ClpCopyOfArray(rhs.dynamicStatus_, numberGubColumns_);
     lowerSet_ = ClpCopyOfArray(rhs.lowerSet_, numberSets_);
     upperSet_ = ClpCopyOfArray(rhs.upperSet_, numberSets_);
}

/* This is the real constructor*/
ClpGubDynamicMatrix::ClpGubDynamicMatrix(ClpSimplex * model, int numberSets,
          int numberGubColumns, const int * starts,
          const double * lower, const double * upper,
          const CoinBigIndex * startColumn, const int * row,
          const double * element, const double * cost,
          const double * lowerColumn, const double * upperColumn,
          const unsigned char * status)
     : ClpGubMatrix()
{
     objectiveOffset_ = model->objectiveOffset();
     model_ = model;
     numberSets_ = numberSets;
     numberGubColumns_ = numberGubColumns;
     fullStart_ = ClpCopyOfArray(starts, numberSets_ + 1);
     lower_ = ClpCopyOfArray(lower, numberSets_);
     upper_ = ClpCopyOfArray(upper, numberSets_);
     int numberColumns = model->numberColumns();
     int numberRows = model->numberRows();
     // Number of columns needed
     int numberGubInSmall = numberSets_ + numberRows + 2 * model->factorizationFrequency() + 2;
     // for small problems this could be too big
     //numberGubInSmall = CoinMin(numberGubInSmall,numberGubColumns_);
     int numberNeeded = numberGubInSmall + numberColumns;
     firstAvailable_ = numberColumns;
     savedFirstAvailable_ = numberColumns;
     firstDynamic_ = numberColumns;
     lastDynamic_ = numberNeeded;
     startColumn_ = ClpCopyOfArray(startColumn, numberGubColumns_ + 1);
     CoinBigIndex numberElements = startColumn_[numberGubColumns_];
     row_ = ClpCopyOfArray(row, numberElements);
     element_ = new double[numberElements];
     CoinBigIndex i;
     for (i = 0; i < numberElements; i++)
          element_[i] = element[i];
     cost_ = new double[numberGubColumns_];
     for (i = 0; i < numberGubColumns_; i++) {
          cost_[i] = cost[i];
          // need sorted
          CoinSort_2(row_ + startColumn_[i], row_ + startColumn_[i+1], element_ + startColumn_[i]);
     }
     if (lowerColumn) {
          lowerColumn_ = new double[numberGubColumns_];
          for (i = 0; i < numberGubColumns_; i++)
               lowerColumn_[i] = lowerColumn[i];
     } else {
          lowerColumn_ = NULL;
     }
     if (upperColumn) {
          upperColumn_ = new double[numberGubColumns_];
          for (i = 0; i < numberGubColumns_; i++)
               upperColumn_[i] = upperColumn[i];
     } else {
          upperColumn_ = NULL;
     }
     if (upperColumn || lowerColumn) {
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
     } else {
          lowerSet_ = NULL;
          upperSet_ = NULL;
     }
     start_ = NULL;
     end_ = NULL;
     dynamicStatus_ = NULL;
     id_ = new int[numberGubInSmall];
     for (i = 0; i < numberGubInSmall; i++)
          id_[i] = -1;
     ClpPackedMatrix* originalMatrixA =
          dynamic_cast< ClpPackedMatrix*>(model->clpMatrix());
     assert (originalMatrixA);
     CoinPackedMatrix * originalMatrix = originalMatrixA->getPackedMatrix();
     originalMatrixA->setMatrixNull(); // so can be deleted safely
     // guess how much space needed
     double guess = originalMatrix->getNumElements() + 10;
     guess /= static_cast<double> (numberColumns);
     guess *= 2 * numberGubColumns_;
     numberElements_ = static_cast<int> (CoinMin(guess, 10000000.0));
     numberElements_ = CoinMin(numberElements_, numberElements) + originalMatrix->getNumElements();
     matrix_ = originalMatrix;
     flags_ &= ~1;
     // resize model (matrix stays same)
     model->resize(numberRows, numberNeeded);
     if (upperColumn_) {
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
     // redo number of columns
     numberColumns = matrix_->getNumCols();
     backward_ = new int[numberNeeded];
     backToPivotRow_ = new int[numberNeeded];
     // We know a bit better
     delete [] changeCost_;
     changeCost_ = new double [numberRows+numberSets_];
     keyVariable_ = new int[numberSets_];
     // signal to need new ordering
     next_ = NULL;
     for (int iColumn = 0; iColumn < numberNeeded; iColumn++)
          backward_[iColumn] = -1;

     firstGub_ = firstDynamic_;
     lastGub_ = lastDynamic_;
     if (!lowerColumn_ && !upperColumn_)
          gubType_ = 8;
     if (status) {
          status_ = ClpCopyOfArray(status, numberSets_);
     } else {
          status_ = new unsigned char [numberSets_];
          memset(status_, 0, numberSets_);
          int i;
          for (i = 0; i < numberSets_; i++) {
               // make slack key
               setStatus(i, ClpSimplex::basic);
          }
     }
     saveStatus_ = new unsigned char [numberSets_];
     memset(saveStatus_, 0, numberSets_);
     savedKeyVariable_ = new int [numberSets_];
     memset(savedKeyVariable_, 0, numberSets_ * sizeof(int));
}

//-------------------------------------------------------------------
// Destructor
//-------------------------------------------------------------------
ClpGubDynamicMatrix::~ClpGubDynamicMatrix ()
{
     delete [] startColumn_;
     delete [] row_;
     delete [] element_;
     delete [] cost_;
     delete [] fullStart_;
     delete [] id_;
     delete [] dynamicStatus_;
     delete [] lowerColumn_;
     delete [] upperColumn_;
     delete [] lowerSet_;
     delete [] upperSet_;
}

//----------------------------------------------------------------
// Assignment operator
//-------------------------------------------------------------------
ClpGubDynamicMatrix &
ClpGubDynamicMatrix::operator=(const ClpGubDynamicMatrix& rhs)
{
     if (this != &rhs) {
          ClpGubMatrix::operator=(rhs);
          delete [] startColumn_;
          delete [] row_;
          delete [] element_;
          delete [] cost_;
          delete [] fullStart_;
          delete [] id_;
          delete [] dynamicStatus_;
          delete [] lowerColumn_;
          delete [] upperColumn_;
          delete [] lowerSet_;
          delete [] upperSet_;
          objectiveOffset_ = rhs.objectiveOffset_;
          numberGubColumns_ = rhs.numberGubColumns_;
          firstAvailable_ = rhs.firstAvailable_;
          savedFirstAvailable_ = rhs.savedFirstAvailable_;
          firstDynamic_ = rhs.firstDynamic_;
          lastDynamic_ = rhs.lastDynamic_;
          numberElements_ = rhs.numberElements_;
          startColumn_ = ClpCopyOfArray(rhs.startColumn_, numberGubColumns_ + 1);
          int numberElements = startColumn_[numberGubColumns_];
          row_ = ClpCopyOfArray(rhs.row_, numberElements);;
          element_ = ClpCopyOfArray(rhs.element_, numberElements);;
          cost_ = ClpCopyOfArray(rhs.cost_, numberGubColumns_);
          fullStart_ = ClpCopyOfArray(rhs.fullStart_, numberSets_ + 1);
          id_ = ClpCopyOfArray(rhs.id_, lastDynamic_ - firstDynamic_);
          lowerColumn_ = ClpCopyOfArray(rhs.lowerColumn_, numberGubColumns_);
          upperColumn_ = ClpCopyOfArray(rhs.upperColumn_, numberGubColumns_);
          dynamicStatus_ = ClpCopyOfArray(rhs.dynamicStatus_, numberGubColumns_);
          lowerSet_ = ClpCopyOfArray(rhs.lowerSet_, numberSets_);
          upperSet_ = ClpCopyOfArray(rhs.upperSet_, numberSets_);
     }
     return *this;
}
//-------------------------------------------------------------------
// Clone
//-------------------------------------------------------------------
ClpMatrixBase * ClpGubDynamicMatrix::clone() const
{
     return new ClpGubDynamicMatrix(*this);
}
// Partial pricing
void
ClpGubDynamicMatrix::partialPricing(ClpSimplex * model, double startFraction, double endFraction,
                                    int & bestSequence, int & numberWanted)
{
     assert(!model->rowScale());
     numberWanted = currentWanted_;
     if (!numberSets_) {
          // no gub
          ClpPackedMatrix::partialPricing(model, startFraction, endFraction, bestSequence, numberWanted);
          return;
     } else {
          // and do some proportion of full set
          int startG2 = static_cast<int> (startFraction * numberSets_);
          int endG2 = static_cast<int> (endFraction * numberSets_ + 0.1);
          endG2 = CoinMin(endG2, numberSets_);
          //printf("gub price - set start %d end %d\n",
          //   startG2,endG2);
          double tolerance = model->currentDualTolerance();
          double * reducedCost = model->djRegion();
          const double * duals = model->dualRowSolution();
          double * cost = model->costRegion();
          double bestDj;
          int numberRows = model->numberRows();
          int numberColumns = lastDynamic_;
          // If nothing found yet can go all the way to end
          int endAll = endG2;
          if (bestSequence < 0 && !startG2)
               endAll = numberSets_;
          if (bestSequence >= 0)
               bestDj = fabs(reducedCost[bestSequence]);
          else
               bestDj = tolerance;
          int saveSequence = bestSequence;
          double djMod = 0.0;
          double infeasibilityCost = model->infeasibilityCost();
          double bestDjMod = 0.0;
          //printf("iteration %d start %d end %d - wanted %d\n",model->numberIterations(),
          //     startG2,endG2,numberWanted);
          int bestType = -1;
          int bestSet = -1;
          const double * element = matrix_->getElements();
          const int * row = matrix_->getIndices();
          const CoinBigIndex * startColumn = matrix_->getVectorStarts();
          int * length = matrix_->getMutableVectorLengths();
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
#ifdef CLP_DEBUG
          {
               for (int i = firstDynamic_; i < firstAvailable_; i++) {
                    assert (getDynamicStatus(id_[i-firstDynamic_]) == inSmall);
               }
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
               CoinBigIndex j;
               int iBasic = keyVariable_[iSet];
               if (iBasic >= numberColumns) {
                    djMod = - weight(iSet) * infeasibilityCost;
               } else {
                    // get dj without
                    assert (model->getStatus(iBasic) == ClpSimplex::basic);
                    djMod = 0.0;

                    for (j = startColumn[iBasic];
                              j < startColumn[iBasic] + length[iBasic]; j++) {
                         int jRow = row[j];
                         djMod -= duals[jRow] * element[j];
                    }
                    djMod += cost[iBasic];
                    // See if gub slack possible - dj is djMod
                    if (getStatus(iSet) == ClpSimplex::atLowerBound) {
                         double value = -djMod;
                         if (value > tolerance) {
                              numberWanted--;
                              if (value > bestDj) {
                                   // check flagged variable and correct dj
                                   if (!flagged(iSet)) {
                                        bestDj = value;
                                        bestSequence = numberRows + numberColumns + iSet;
                                        bestDjMod = djMod;
                                        bestType = 0;
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
                                        bestSequence = numberRows + numberColumns + iSet;
                                        bestDjMod = djMod;
                                        bestType = 0;
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
               for (int iSequence = fullStart_[iSet]; iSequence < fullStart_[iSet+1]; iSequence++) {
                    DynamicStatus status = getDynamicStatus(iSequence);
                    if (status != inSmall) {
                         double value = cost_[iSequence] - djMod;
                         for (j = startColumn_[iSequence];
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
                                        bestDj = value;
                                        bestSequence = iSequence;
                                        bestDjMod = djMod;
                                        bestType = 1;
                                        bestSet = iSet;
                                   } else {
                                        // just to make sure we don't exit before got something
                                        numberWanted++;
                                   }
                              }
                         }
                    }
               }
               if (numberWanted <= 0) {
                    numberWanted = 0;
                    break;
               }
          }
          // Do packed part before gub and small gub - but lightly
          int saveMinNeg = minimumGoodReducedCosts_;
          int saveSequence2 = bestSequence;
          if (bestSequence >= 0)
               minimumGoodReducedCosts_ = -2;
          int saveLast = lastGub_;
          lastGub_ = firstAvailable_;
          currentWanted_ = numberWanted;
          ClpGubMatrix::partialPricing(model, startFraction, endFraction, bestSequence, numberWanted);
          minimumGoodReducedCosts_ = saveMinNeg;
          lastGub_ = saveLast;
          if (bestSequence != saveSequence2) {
               bestType = -1; // in normal or small gub part
               saveSequence = bestSequence;
          }
          if (bestSequence != saveSequence || bestType >= 0) {
               double * lowerColumn = model->lowerRegion();
               double * upperColumn = model->upperRegion();
               double * solution = model->solutionRegion();
               if (bestType > 0) {
                    // recompute dj and create
                    double value = cost_[bestSequence] - bestDjMod;
                    for (CoinBigIndex jBigIndex = startColumn_[bestSequence];
                              jBigIndex < startColumn_[bestSequence+1]; jBigIndex++) {
                         int jRow = row_[jBigIndex];
                         value -= duals[jRow] * element_[jBigIndex];
                    }
                    double * element =  matrix_->getMutableElements();
                    int * row = matrix_->getMutableIndices();
                    CoinBigIndex * startColumn = matrix_->getMutableVectorStarts();
                    int * length = matrix_->getMutableVectorLengths();
                    CoinBigIndex numberElements = startColumn[firstAvailable_];
                    int numberThis = startColumn_[bestSequence+1] - startColumn_[bestSequence];
                    if (numberElements + numberThis > numberElements_) {
                         // need to redo
                         numberElements_ = CoinMax(3 * numberElements_ / 2, numberElements + numberThis);
                         matrix_->reserve(numberColumns, numberElements_);
                         element =  matrix_->getMutableElements();
                         row = matrix_->getMutableIndices();
                         // these probably okay but be safe
                         startColumn = matrix_->getMutableVectorStarts();
                         length = matrix_->getMutableVectorLengths();
                    }
                    // already set startColumn[firstAvailable_]=numberElements;
                    length[firstAvailable_] = numberThis;
                    model->costRegion()[firstAvailable_] = cost_[bestSequence];
                    CoinBigIndex base = startColumn_[bestSequence];
                    for (int j = 0; j < numberThis; j++) {
                         row[numberElements] = row_[base+j];
                         element[numberElements++] = element_[base+j];
                    }
                    id_[firstAvailable_-firstDynamic_] = bestSequence;
                    //printf("best %d\n",bestSequence);
                    backward_[firstAvailable_] = bestSet;
                    model->solutionRegion()[firstAvailable_] = 0.0;
                    if (!lowerColumn_ && !upperColumn_) {
                         model->setStatus(firstAvailable_, ClpSimplex::atLowerBound);
                         lowerColumn[firstAvailable_] = 0.0;
                         upperColumn[firstAvailable_] = COIN_DBL_MAX;
                    }  else {
                         DynamicStatus status = getDynamicStatus(bestSequence);
                         if (lowerColumn_)
                              lowerColumn[firstAvailable_] = lowerColumn_[bestSequence];
                         else
                              lowerColumn[firstAvailable_] = 0.0;
                         if (upperColumn_)
                              upperColumn[firstAvailable_] = upperColumn_[bestSequence];
                         else
                              upperColumn[firstAvailable_] = COIN_DBL_MAX;
                         if (status == atLowerBound) {
                              solution[firstAvailable_] = lowerColumn[firstAvailable_];
                              model->setStatus(firstAvailable_, ClpSimplex::atLowerBound);
                         } else {
                              solution[firstAvailable_] = upperColumn[firstAvailable_];
                              model->setStatus(firstAvailable_, ClpSimplex::atUpperBound);
                         }
                    }
                    model->nonLinearCost()->setOne(firstAvailable_, solution[firstAvailable_],
                                                   lowerColumn[firstAvailable_],
                                                   upperColumn[firstAvailable_], cost_[bestSequence]);
                    bestSequence = firstAvailable_;
                    // firstAvailable_ only updated if good pivot (in updatePivot)
                    startColumn[firstAvailable_+1] = numberElements;
                    //printf("price struct %d - dj %g gubpi %g\n",bestSequence,value,bestDjMod);
                    reducedCost[bestSequence] = value;
                    gubSlackIn_ = -1;
               } else {
                    // slack - make last column
                    gubSlackIn_ = bestSequence - numberRows - numberColumns;
                    bestSequence = numberColumns + 2 * numberRows;
                    reducedCost[bestSequence] = bestDjMod;
                    //printf("price slack %d - gubpi %g\n",gubSlackIn_,bestDjMod);
                    model->setStatus(bestSequence, getStatus(gubSlackIn_));
                    if (getStatus(gubSlackIn_) == ClpSimplex::atUpperBound)
                         solution[bestSequence] = upper_[gubSlackIn_];
                    else
                         solution[bestSequence] = lower_[gubSlackIn_];
                    lowerColumn[bestSequence] = lower_[gubSlackIn_];
                    upperColumn[bestSequence] = upper_[gubSlackIn_];
                    model->costRegion()[bestSequence] = 0.0;
                    model->nonLinearCost()->setOne(bestSequence, solution[bestSequence], lowerColumn[bestSequence],
                                                   upperColumn[bestSequence], 0.0);
               }
               savedBestSequence_ = bestSequence;
               savedBestDj_ = reducedCost[savedBestSequence_];
          }
          // See if may be finished
          if (!startG2 && bestSequence < 0)
               infeasibilityWeight_ = model_->infeasibilityCost();
          else if (bestSequence >= 0)
               infeasibilityWeight_ = -1.0;
     }
     currentWanted_ = numberWanted;
}
// This is local to Gub to allow synchronization when status is good
int
ClpGubDynamicMatrix::synchronize(ClpSimplex * model, int mode)
{
     int returnNumber = 0;
     switch (mode) {
     case 0: {
#ifdef CLP_DEBUG
          {
               for (int i = 0; i < numberSets_; i++)
                    assert(toIndex_[i] == -1);
          }
#endif
          // lookup array
          int * lookup = new int[lastDynamic_];
          int iColumn;
          int numberColumns = model->numberColumns();
          double * element =  matrix_->getMutableElements();
          int * row = matrix_->getMutableIndices();
          CoinBigIndex * startColumn = matrix_->getMutableVectorStarts();
          int * length = matrix_->getMutableVectorLengths();
          double * cost = model->costRegion();
          double * lowerColumn = model->lowerRegion();
          double * upperColumn = model->upperRegion();
          int * pivotVariable = model->pivotVariable();
          CoinBigIndex numberElements = startColumn[firstDynamic_];
          // first just do lookup and basic stuff
          int currentNumber = firstAvailable_;
          firstAvailable_ = firstDynamic_;
          int numberToDo = 0;
          double objectiveChange = 0.0;
          double * solution = model->solutionRegion();
          for (iColumn = firstDynamic_; iColumn < currentNumber; iColumn++) {
               int iSet = backward_[iColumn];
               if (toIndex_[iSet] < 0) {
                    toIndex_[iSet] = 0;
                    fromIndex_[numberToDo++] = iSet;
               }
               if (model->getStatus(iColumn) == ClpSimplex::basic || iColumn == keyVariable_[iSet]) {
                    lookup[iColumn] = firstAvailable_;
                    if (iColumn != keyVariable_[iSet]) {
                         int iPivot = backToPivotRow_[iColumn];
                         backToPivotRow_[firstAvailable_] = iPivot;
                         pivotVariable[iPivot] = firstAvailable_;
                    }
                    firstAvailable_++;
               } else {
                    int jColumn = id_[iColumn-firstDynamic_];
                    setDynamicStatus(jColumn, atLowerBound);
                    if (lowerColumn_ || upperColumn_) {
                         if (model->getStatus(iColumn) == ClpSimplex::atUpperBound)
                              setDynamicStatus(jColumn, atUpperBound);
                         // treat solution as if exactly at a bound
                         double value = solution[iColumn];
                         if (fabs(value - lowerColumn[iColumn]) < fabs(value - upperColumn[iColumn]))
                              value = lowerColumn[iColumn];
                         else
                              value = upperColumn[iColumn];
                         objectiveChange += cost[iColumn] * value;
                         // redo lower and upper on sets
                         double shift = value;
                         if (lowerSet_[iSet] > -1.0e20)
                              lower_[iSet] = lowerSet_[iSet] - shift;
                         if (upperSet_[iSet] < 1.0e20)
                              upper_[iSet] = upperSet_[iSet] - shift;
                    }
                    lookup[iColumn] = -1;
               }
          }
          model->setObjectiveOffset(model->objectiveOffset() + objectiveChange);
          firstAvailable_ = firstDynamic_;
          for (iColumn = firstDynamic_; iColumn < currentNumber; iColumn++) {
               if (lookup[iColumn] >= 0) {
                    // move
                    int jColumn = id_[iColumn-firstDynamic_];
                    id_[firstAvailable_-firstDynamic_] = jColumn;
                    int numberThis = startColumn_[jColumn+1] - startColumn_[jColumn];
                    length[firstAvailable_] = numberThis;
                    cost[firstAvailable_] = cost[iColumn];
                    lowerColumn[firstAvailable_] = lowerColumn[iColumn];
                    upperColumn[firstAvailable_] = upperColumn[iColumn];
                    double originalLower = lowerColumn_ ? lowerColumn_[jColumn] : 0.0;
                    double originalUpper = upperColumn_ ? upperColumn_[jColumn] : COIN_DBL_MAX;
                    if (originalUpper > 1.0e30)
                         originalUpper = COIN_DBL_MAX;
                    model->nonLinearCost()->setOne(firstAvailable_, solution[iColumn],
                                                   originalLower, originalUpper,
                                                   cost_[jColumn]);
                    CoinBigIndex base = startColumn_[jColumn];
                    for (int j = 0; j < numberThis; j++) {
                         row[numberElements] = row_[base+j];
                         element[numberElements++] = element_[base+j];
                    }
                    model->setStatus(firstAvailable_, model->getStatus(iColumn));
                    backward_[firstAvailable_] = backward_[iColumn];
                    solution[firstAvailable_] = solution[iColumn];
                    firstAvailable_++;
                    startColumn[firstAvailable_] = numberElements;
               }
          }
          // clean up next_
          int * temp = new int [firstAvailable_];
          for (int jSet = 0; jSet < numberToDo; jSet++) {
               int iSet = fromIndex_[jSet];
               toIndex_[iSet] = -1;
               int last = keyVariable_[iSet];
               int j = next_[last];
               bool setTemp = true;
               if (last < lastDynamic_) {
                    last = lookup[last];
                    assert (last >= 0);
                    keyVariable_[iSet] = last;
               } else if (j >= 0) {
                    int newJ = lookup[j];
                    assert (newJ >= 0);
                    j = next_[j];
                    next_[last] = newJ;
                    last = newJ;
               } else {
                    next_[last] = -(iSet + numberColumns + 1);
                    setTemp = false;
               }
               while (j >= 0) {
                    int newJ = lookup[j];
                    assert (newJ >= 0);
                    temp[last] = newJ;
                    last = newJ;
                    j = next_[j];
               }
               if (setTemp)
                    temp[last] = -(keyVariable_[iSet] + 1);
               if (lowerSet_) {
                    // we only need to get lower_ and upper_ correct
                    double shift = 0.0;
                    for (int j = fullStart_[iSet]; j < fullStart_[iSet+1]; j++)
                         if (getDynamicStatus(j) == atUpperBound)
                              shift += upperColumn_[j];
                         else if (getDynamicStatus(j) == atLowerBound && lowerColumn_)
                              shift += lowerColumn_[j];
                    if (lowerSet_[iSet] > -1.0e20)
                         lower_[iSet] = lowerSet_[iSet] - shift;
                    if (upperSet_[iSet] < 1.0e20)
                         upper_[iSet] = upperSet_[iSet] - shift;
               }
          }
          // move to next_
          CoinMemcpyN(temp + firstDynamic_, (firstAvailable_ - firstDynamic_), next_ + firstDynamic_);
          // if odd iterations may be one out so adjust currentNumber
          currentNumber = CoinMin(currentNumber + 1, lastDynamic_);
          // zero solution
          CoinZeroN(solution + firstAvailable_, currentNumber - firstAvailable_);
          // zero cost
          CoinZeroN(cost + firstAvailable_, currentNumber - firstAvailable_);
          // zero lengths
          CoinZeroN(length + firstAvailable_, currentNumber - firstAvailable_);
          for ( iColumn = firstAvailable_; iColumn < currentNumber; iColumn++) {
               model->nonLinearCost()->setOne(iColumn, 0.0, 0.0, COIN_DBL_MAX, 0.0);
               model->setStatus(iColumn, ClpSimplex::atLowerBound);
               backward_[iColumn] = -1;
          }
          delete [] lookup;
          delete [] temp;
          // make sure fromIndex clean
          fromIndex_[0] = -1;
          //#define CLP_DEBUG
#ifdef CLP_DEBUG
          // debug
          {
               int i;
               int numberRows = model->numberRows();
               char * xxxx = new char[numberColumns];
               memset(xxxx, 0, numberColumns);
               for (i = 0; i < numberRows; i++) {
                    int iPivot = pivotVariable[i];
                    assert (model->getStatus(iPivot) == ClpSimplex::basic);
                    if (iPivot < numberColumns && backward_[iPivot] >= 0)
                         xxxx[iPivot] = 1;
               }
               for (i = 0; i < numberSets_; i++) {
                    int key = keyVariable_[i];
                    int iColumn = next_[key];
                    int k = 0;
                    while(iColumn >= 0) {
                         k++;
                         assert (k < 100);
                         assert (backward_[iColumn] == i);
                         iColumn = next_[iColumn];
                    }
                    int stop = -(key + 1);
                    while (iColumn != stop) {
                         assert (iColumn < 0);
                         iColumn = -iColumn - 1;
                         k++;
                         assert (k < 100);
                         assert (backward_[iColumn] == i);
                         iColumn = next_[iColumn];
                    }
                    iColumn = next_[key];
                    while (iColumn >= 0) {
                         assert (xxxx[iColumn]);
                         xxxx[iColumn] = 0;
                         iColumn = next_[iColumn];
                    }
               }
               for (i = 0; i < numberColumns; i++) {
                    if (i < numberColumns && backward_[i] >= 0) {
                         assert (!xxxx[i] || i == keyVariable_[backward_[i]]);
                    }
               }
               delete [] xxxx;
          }
          {
               for (int i = 0; i < numberSets_; i++)
                    assert(toIndex_[i] == -1);
          }
#endif
          savedFirstAvailable_ = firstAvailable_;
     }
     break;
     // flag a variable
     case 1: {
          // id will be sitting at firstAvailable
          int sequence = id_[firstAvailable_-firstDynamic_];
          assert (!flagged(sequence));
          setFlagged(sequence);
          model->clearFlagged(firstAvailable_);
     }
     break;
     // unflag all variables
     case 2: {
          for (int i = 0; i < numberGubColumns_; i++) {
               if (flagged(i)) {
                    unsetFlagged(i);
                    returnNumber++;
               }
          }
     }
     break;
     //  just reset costs and bounds (primal)
     case 3: {
          double * cost = model->costRegion();
          double * solution = model->solutionRegion();
          double * lowerColumn = model->columnLower();
          double * upperColumn = model->columnUpper();
          for (int i = firstDynamic_; i < firstAvailable_; i++) {
               int jColumn = id_[i-firstDynamic_];
               cost[i] = cost_[jColumn];
               if (!lowerColumn_ && !upperColumn_) {
                    lowerColumn[i] = 0.0;
                    upperColumn[i] = COIN_DBL_MAX;
               }  else {
                    if (lowerColumn_)
                         lowerColumn[i] = lowerColumn_[jColumn];
                    else
                         lowerColumn[i] = 0.0;
                    if (upperColumn_)
                         upperColumn[i] = upperColumn_[jColumn];
                    else
                         upperColumn[i] = COIN_DBL_MAX;
               }
               if (model->nonLinearCost())
                    model->nonLinearCost()->setOne(i, solution[i],
                                                   lowerColumn[i],
                                                   upperColumn[i], cost_[jColumn]);
          }
          if (!model->numberIterations() && rhsOffset_) {
               lastRefresh_ = - refreshFrequency_; // force refresh
          }
     }
     break;
     // and get statistics for column generation
     case 4: {
          // In theory we should subtract out ones we have done but ....
          // If key slack then dual 0.0
          // If not then slack could be dual infeasible
          // dj for key is zero so that defines dual on set
          int i;
          int numberColumns = model->numberColumns();
          double * dual = model->dualRowSolution();
          double infeasibilityCost = model->infeasibilityCost();
          double dualTolerance = model->dualTolerance();
          double relaxedTolerance = dualTolerance;
          // we can't really trust infeasibilities if there is dual error
          double error = CoinMin(1.0e-2, model->largestDualError());
          // allow tolerance at least slightly bigger than standard
          relaxedTolerance = relaxedTolerance +  error;
          // but we will be using difference
          relaxedTolerance -= dualTolerance;
          double objectiveOffset = 0.0;
          for (i = 0; i < numberSets_; i++) {
               int kColumn = keyVariable_[i];
               double value = 0.0;
               if (kColumn < numberColumns) {
                    kColumn = id_[kColumn-firstDynamic_];
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
               } else {
                    // slack key - may not be feasible
                    assert (getStatus(i) == ClpSimplex::basic);
                    // negative as -1.0 for slack
                    value = -weight(i) * infeasibilityCost;
               }
               // Now subtract out from all
               for (CoinBigIndex k = fullStart_[i]; k < fullStart_[i+1]; k++) {
                    if (getDynamicStatus(k) != inSmall) {
                         double djValue = cost_[k] - value;
                         for (CoinBigIndex j = startColumn_[k];
                                   j < startColumn_[k+1]; j++) {
                              int iRow = row_[j];
                              djValue -= dual[iRow] * element_[j];
                         }
                         double infeasibility = 0.0;
                         double shift = 0.0;
                         if (getDynamicStatus(k) == atLowerBound) {
                              if (lowerColumn_)
                                   shift = lowerColumn_[k];
                              if (djValue < -dualTolerance)
                                   infeasibility = -djValue - dualTolerance;
                         } else {
                              // at upper bound
                              shift = upperColumn_[k];
                              if (djValue > dualTolerance)
                                   infeasibility = djValue - dualTolerance;
                         }
                         objectiveOffset += shift * cost_[k];
                         if (infeasibility > 0.0) {
                              sumDualInfeasibilities_ += infeasibility;
                              if (infeasibility > relaxedTolerance)
                                   sumOfRelaxedDualInfeasibilities_ += infeasibility;
                              numberDualInfeasibilities_ ++;
                         }
                    }
               }
          }
          model->setObjectiveOffset(objectiveOffset_ - objectiveOffset);
     }
     break;
     // see if time to re-factorize
     case 5: {
          if (firstAvailable_ > numberSets_ + model->numberRows() + model->factorizationFrequency())
               returnNumber = 4;
     }
     break;
     // return 1 if there may be changing bounds on variable (column generation)
     case 6: {
          returnNumber = (lowerColumn_ != NULL || upperColumn_ != NULL) ? 1 : 0;
#if 0
          if (!returnNumber) {
               // may be gub slacks
               for (int i = 0; i < numberSets_; i++) {
                    if (upper_[i] > lower_[i]) {
                         returnNumber = 1;
                         break;
                    }
               }
          }
#endif
     }
     break;
     // restore firstAvailable_
     case 7: {
          int iColumn;
          int * length = matrix_->getMutableVectorLengths();
          double * cost = model->costRegion();
          double * solution = model->solutionRegion();
          int currentNumber = firstAvailable_;
          firstAvailable_ = savedFirstAvailable_;
          // zero solution
          CoinZeroN(solution + firstAvailable_, currentNumber - firstAvailable_);
          // zero cost
          CoinZeroN(cost + firstAvailable_, currentNumber - firstAvailable_);
          // zero lengths
          CoinZeroN(length + firstAvailable_, currentNumber - firstAvailable_);
          for ( iColumn = firstAvailable_; iColumn < currentNumber; iColumn++) {
               model->nonLinearCost()->setOne(iColumn, 0.0, 0.0, COIN_DBL_MAX, 0.0);
               model->setStatus(iColumn, ClpSimplex::atLowerBound);
               backward_[iColumn] = -1;
          }
     }
     break;
     // make sure set is clean
     case 8: {
          int sequenceIn = model->sequenceIn();
          if (sequenceIn < model->numberColumns()) {
               int iSet = backward_[sequenceIn];
               if (iSet >= 0 && lowerSet_) {
                    // we only need to get lower_ and upper_ correct
                    double shift = 0.0;
                    for (int j = fullStart_[iSet]; j < fullStart_[iSet+1]; j++)
                         if (getDynamicStatus(j) == atUpperBound)
                              shift += upperColumn_[j];
                         else if (getDynamicStatus(j) == atLowerBound && lowerColumn_)
                              shift += lowerColumn_[j];
                    if (lowerSet_[iSet] > -1.0e20)
                         lower_[iSet] = lowerSet_[iSet] - shift;
                    if (upperSet_[iSet] < 1.0e20)
                         upper_[iSet] = upperSet_[iSet] - shift;
               }
               if (sequenceIn == firstAvailable_) {
                    // not really in small problem
                    int iBig = id_[sequenceIn-firstDynamic_];
                    if (model->getStatus(sequenceIn) == ClpSimplex::atLowerBound)
                         setDynamicStatus(iBig, atLowerBound);
                    else
                         setDynamicStatus(iBig, atUpperBound);
               }
          }
     }
     break;
     // adjust lower,upper
     case 9: {
          int sequenceIn = model->sequenceIn();
          if (sequenceIn >= firstDynamic_ && sequenceIn < lastDynamic_ && lowerSet_) {
               int iSet = backward_[sequenceIn];
               assert (iSet >= 0);
               int inBig = id_[sequenceIn-firstDynamic_];
               const double * solution = model->solutionRegion();
               setDynamicStatus(inBig, inSmall);
               if (lowerSet_[iSet] > -1.0e20)
                    lower_[iSet] += solution[sequenceIn];
               if (upperSet_[iSet] < 1.0e20)
                    upper_[iSet] += solution[sequenceIn];
               model->setObjectiveOffset(model->objectiveOffset() -
                                         solution[sequenceIn]*cost_[inBig]);
          }
     }
     }
     return returnNumber;
}
// Add a new variable to a set
void
ClpGubDynamicMatrix::insertNonBasic(int sequence, int iSet)
{
     int last = keyVariable_[iSet];
     int j = next_[last];
     while (j >= 0) {
          last = j;
          j = next_[j];
     }
     next_[last] = -(sequence + 1);
     next_[sequence] = j;
}
// Sets up an effective RHS and does gub crash if needed
void
ClpGubDynamicMatrix::useEffectiveRhs(ClpSimplex * model, bool cheapest)
{
     // Do basis - cheapest or slack if feasible (unless cheapest set)
     int longestSet = 0;
     int iSet;
     for (iSet = 0; iSet < numberSets_; iSet++)
          longestSet = CoinMax(longestSet, fullStart_[iSet+1] - fullStart_[iSet]);

     double * upper = new double[longestSet+1];
     double * cost = new double[longestSet+1];
     double * lower = new double[longestSet+1];
     double * solution = new double[longestSet+1];
     assert (!next_);
     delete [] next_;
     int numberColumns = model->numberColumns();
     next_ = new int[numberColumns+numberSets_+CoinMax(2*longestSet, lastDynamic_-firstDynamic_)];
     char * mark = new char[numberColumns];
     memset(mark, 0, numberColumns);
     for (int iColumn = 0; iColumn < numberColumns; iColumn++)
          next_[iColumn] = COIN_INT_MAX;
     int i;
     int * keys = new int[numberSets_];
     int * back = new int[numberGubColumns_];
     CoinFillN(back, numberGubColumns_, -1);
     for (i = 0; i < numberSets_; i++)
          keys[i] = COIN_INT_MAX;
     delete [] dynamicStatus_;
     dynamicStatus_ = new unsigned char [numberGubColumns_];
     memset(dynamicStatus_, 0, numberGubColumns_); // for clarity
     for (i = 0; i < numberGubColumns_; i++)
          setDynamicStatus(i, atLowerBound);
     // set up chains
     for (i = firstDynamic_; i < lastDynamic_; i++) {
          if (id_[i-firstDynamic_] >= 0) {
               if (model->getStatus(i) == ClpSimplex::basic)
                    mark[i] = 1;
               int iSet = backward_[i];
               assert (iSet >= 0);
               int iNext = keys[iSet];
               next_[i] = iNext;
               keys[iSet] = i;
               back[id_[i-firstDynamic_]] = i;
          } else {
               model->setStatus(i, ClpSimplex::atLowerBound);
               backward_[i] = -1;
          }
     }
     double * columnSolution = model->solutionRegion();
     int numberRows = getNumRows();
     toIndex_ = new int[numberSets_];
     for (iSet = 0; iSet < numberSets_; iSet++)
          toIndex_[iSet] = -1;
     fromIndex_ = new int [numberRows+numberSets_];
     double tolerance = model->primalTolerance();
     double * element =  matrix_->getMutableElements();
     int * row = matrix_->getMutableIndices();
     CoinBigIndex * startColumn = matrix_->getMutableVectorStarts();
     int * length = matrix_->getMutableVectorLengths();
     double objectiveOffset = 0.0;
     for (iSet = 0; iSet < numberSets_; iSet++) {
          int j;
          int numberBasic = 0;
          int iBasic = -1;
          int iStart = fullStart_[iSet];
          int iEnd = fullStart_[iSet+1];
          // find one with smallest length
          int smallest = numberRows + 1;
          double value = 0.0;
          j = keys[iSet];
          while (j != COIN_INT_MAX) {
               if (model->getStatus(j) == ClpSimplex::basic) {
                    if (length[j] < smallest) {
                         smallest = length[j];
                         iBasic = j;
                    }
                    numberBasic++;
               }
               value += columnSolution[j];
               j = next_[j];
          }
          bool done = false;
          if (numberBasic > 1 || (numberBasic == 1 && getStatus(iSet) == ClpSimplex::basic)) {
               if (getStatus(iSet) == ClpSimplex::basic)
                    iBasic = iSet + numberColumns; // slack key - use
               done = true;
          } else if (numberBasic == 1) {
               // see if can be key
               double thisSolution = columnSolution[iBasic];
               if (thisSolution < 0.0) {
                    value -= thisSolution;
                    thisSolution = 0.0;
                    columnSolution[iBasic] = thisSolution;
               }
               // try setting slack to a bound
               assert (upper_[iSet] < 1.0e20 || lower_[iSet] > -1.0e20);
               double cost1 = COIN_DBL_MAX;
               int whichBound = -1;
               if (upper_[iSet] < 1.0e20) {
                    // try slack at ub
                    double newBasic = thisSolution + upper_[iSet] - value;
                    if (newBasic >= -tolerance) {
                         // can go
                         whichBound = 1;
                         cost1 = newBasic * cost_[iBasic];
                         // But if exact then may be good solution
                         if (fabs(upper_[iSet] - value) < tolerance)
                              cost1 = -COIN_DBL_MAX;
                    }
               }
               if (lower_[iSet] > -1.0e20) {
                    // try slack at lb
                    double newBasic = thisSolution + lower_[iSet] - value;
                    if (newBasic >= -tolerance) {
                         // can go but is it cheaper
                         double cost2 = newBasic * cost_[iBasic];
                         // But if exact then may be good solution
                         if (fabs(lower_[iSet] - value) < tolerance)
                              cost2 = -COIN_DBL_MAX;
                         if (cost2 < cost1)
                              whichBound = 0;
                    }
               }
               if (whichBound != -1) {
                    // key
                    done = true;
                    if (whichBound) {
                         // slack to upper
                         columnSolution[iBasic] = thisSolution + upper_[iSet] - value;
                         setStatus(iSet, ClpSimplex::atUpperBound);
                    } else {
                         // slack to lower
                         columnSolution[iBasic] = thisSolution + lower_[iSet] - value;
                         setStatus(iSet, ClpSimplex::atLowerBound);
                    }
               }
          }
          if (!done) {
               if (!cheapest) {
                    // see if slack can be key
                    if (value >= lower_[iSet] - tolerance && value <= upper_[iSet] + tolerance) {
                         done = true;
                         setStatus(iSet, ClpSimplex::basic);
                         iBasic = iSet + numberColumns;
                    }
               }
               if (!done) {
                    // set non basic if there was one
                    if (iBasic >= 0)
                         model->setStatus(iBasic, ClpSimplex::atLowerBound);
                    // find cheapest
                    int numberInSet = iEnd - iStart;
                    if (!lowerColumn_) {
                         CoinZeroN(lower, numberInSet);
                    } else {
                         for (int j = 0; j < numberInSet; j++)
                              lower[j] = lowerColumn_[j+iStart];
                    }
                    if (!upperColumn_) {
                         CoinFillN(upper, numberInSet, COIN_DBL_MAX);
                    } else {
                         for (int j = 0; j < numberInSet; j++)
                              upper[j] = upperColumn_[j+iStart];
                    }
                    CoinFillN(solution, numberInSet, 0.0);
                    // and slack
                    iBasic = numberInSet;
                    solution[iBasic] = -value;
                    lower[iBasic] = -upper_[iSet];
                    upper[iBasic] = -lower_[iSet];
                    int kphase;
                    if (value >= lower_[iSet] - tolerance && value <= upper_[iSet] + tolerance) {
                         // feasible
                         kphase = 1;
                         cost[iBasic] = 0.0;
                         for (int j = 0; j < numberInSet; j++)
                              cost[j] = cost_[j+iStart];
                    } else {
                         // infeasible
                         kphase = 0;
                         // remember bounds are flipped so opposite to natural
                         if (value < lower_[iSet] - tolerance)
                              cost[iBasic] = 1.0;
                         else
                              cost[iBasic] = -1.0;
                         CoinZeroN(cost, numberInSet);
                    }
                    double dualTolerance = model->dualTolerance();
                    for (int iphase = kphase; iphase < 2; iphase++) {
                         if (iphase) {
                              cost[numberInSet] = 0.0;
                              for (int j = 0; j < numberInSet; j++)
                                   cost[j] = cost_[j+iStart];
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
                    // do solution i.e. bounds
                    if (lowerColumn_ || upperColumn_) {
                         for (int j = 0; j < numberInSet; j++) {
                              if (j != iBasic) {
                                   objectiveOffset += solution[j] * cost[j];
                                   if (lowerColumn_ && upperColumn_) {
                                        if (fabs(solution[j] - lowerColumn_[j+iStart]) >
                                                  fabs(solution[j] - upperColumn_[j+iStart]))
                                             setDynamicStatus(j + iStart, atUpperBound);
                                   } else if (upperColumn_ && solution[j] > 0.0) {
                                        setDynamicStatus(j + iStart, atUpperBound);
                                   } else {
                                        setDynamicStatus(j + iStart, atLowerBound);
                                   }
                              }
                         }
                    }
                    // convert iBasic back and do bounds
                    if (iBasic == numberInSet) {
                         // slack basic
                         setStatus(iSet, ClpSimplex::basic);
                         iBasic = iSet + numberColumns;
                    } else {
                         iBasic += fullStart_[iSet];
                         if (back[iBasic] >= 0) {
                              // exists
                              iBasic = back[iBasic];
                         } else {
                              // create
                              CoinBigIndex numberElements = startColumn[firstAvailable_];
                              int numberThis = startColumn_[iBasic+1] - startColumn_[iBasic];
                              if (numberElements + numberThis > numberElements_) {
                                   // need to redo
                                   numberElements_ = CoinMax(3 * numberElements_ / 2, numberElements + numberThis);
                                   matrix_->reserve(numberColumns, numberElements_);
                                   element =  matrix_->getMutableElements();
                                   row = matrix_->getMutableIndices();
                                   // these probably okay but be safe
                                   startColumn = matrix_->getMutableVectorStarts();
                                   length = matrix_->getMutableVectorLengths();
                              }
                              length[firstAvailable_] = numberThis;
                              model->costRegion()[firstAvailable_] = cost_[iBasic];
                              if (lowerColumn_)
                                   model->lowerRegion()[firstAvailable_] = lowerColumn_[iBasic];
                              else
                                   model->lowerRegion()[firstAvailable_] = 0.0;
                              if (upperColumn_)
                                   model->upperRegion()[firstAvailable_] = upperColumn_[iBasic];
                              else
                                   model->upperRegion()[firstAvailable_] = COIN_DBL_MAX;
                              columnSolution[firstAvailable_] = solution[iBasic-fullStart_[iSet]];
                              CoinBigIndex base = startColumn_[iBasic];
                              for (int j = 0; j < numberThis; j++) {
                                   row[numberElements] = row_[base+j];
                                   element[numberElements++] = element_[base+j];
                              }
                              // already set startColumn[firstAvailable_]=numberElements;
                              id_[firstAvailable_-firstDynamic_] = iBasic;
                              setDynamicStatus(iBasic, inSmall);
                              backward_[firstAvailable_] = iSet;
                              iBasic = firstAvailable_;
                              firstAvailable_++;
                              startColumn[firstAvailable_] = numberElements;
                         }
                         model->setStatus(iBasic, ClpSimplex::basic);
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
                    for (j = iStart; j < iEnd; j++) {
                         int iBack = back[j];
                         if (iBack >= 0) {
                              if (model->getStatus(iBack) != ClpSimplex::basic) {
                                   int inSet = j - iStart;
                                   columnSolution[iBack] = solution[inSet];
                                   if (upper[inSet] == lower[inSet])
                                        model->setStatus(iBack, ClpSimplex::isFixed);
                                   else if (solution[inSet] == upper[inSet])
                                        model->setStatus(iBack, ClpSimplex::atUpperBound);
                                   else if (solution[inSet] == lower[inSet])
                                        model->setStatus(iBack, ClpSimplex::atLowerBound);
                              }
                         }
                    }
               }
          }
          keyVariable_[iSet] = iBasic;
     }
     model->setObjectiveOffset(objectiveOffset_ - objectiveOffset);
     delete [] lower;
     delete [] solution;
     delete [] upper;
     delete [] cost;
     // make sure matrix is in good shape
     matrix_->orderMatrix();
     // create effective rhs
     delete [] rhsOffset_;
     rhsOffset_ = new double[numberRows];
     // and redo chains
     memset(mark, 0, numberColumns);
     for (int iColumnX = 0; iColumnX < firstAvailable_; iColumnX++)
          next_[iColumnX] = COIN_INT_MAX;
     for (i = 0; i < numberSets_; i++) {
          keys[i] = COIN_INT_MAX;
          int iKey = keyVariable_[i];
          if (iKey < numberColumns)
               model->setStatus(iKey, ClpSimplex::basic);
     }
     // set up chains
     for (i = 0; i < firstAvailable_; i++) {
          if (model->getStatus(i) == ClpSimplex::basic)
               mark[i] = 1;
          int iSet = backward_[i];
          if (iSet >= 0) {
               int iNext = keys[iSet];
               next_[i] = iNext;
               keys[iSet] = i;
          }
     }
     for (i = 0; i < numberSets_; i++) {
          if (keys[i] != COIN_INT_MAX) {
               // something in set
               int j;
               if (getStatus(i) != ClpSimplex::basic) {
                    // make sure fixed if it is
                    if (upper_[i] == lower_[i])
                         setStatus(i, ClpSimplex::isFixed);
                    // slack not key - choose one with smallest length
                    int smallest = numberRows + 1;
                    int key = -1;
                    j = keys[i];
                    while (1) {
                         if (mark[j] && length[j] < smallest) {
                              key = j;
                              smallest = length[j];
                         }
                         if (next_[j] != COIN_INT_MAX) {
                              j = next_[j];
                         } else {
                              // correct end
                              next_[j] = -(keys[i] + 1);
                              break;
                         }
                    }
                    if (key >= 0) {
                         keyVariable_[i] = key;
                    } else {
                         // nothing basic - make slack key
                         //((ClpGubMatrix *)this)->setStatus(i,ClpSimplex::basic);
                         // fudge to avoid const problem
                         status_[i] = 1;
                    }
               } else {
                    // slack key
                    keyVariable_[i] = numberColumns + i;
                    int j;
                    double sol = 0.0;
                    j = keys[i];
                    while (1) {
                         sol += columnSolution[j];
                         if (next_[j] != COIN_INT_MAX) {
                              j = next_[j];
                         } else {
                              // correct end
                              next_[j] = -(keys[i] + 1);
                              break;
                         }
                    }
                    if (sol > upper_[i] + tolerance) {
                         setAbove(i);
                    } else if (sol < lower_[i] - tolerance) {
                         setBelow(i);
                    } else {
                         setFeasible(i);
                    }
               }
               // Create next_
               int key = keyVariable_[i];
               redoSet(model, key, keys[i], i);
          } else {
               // nothing in set!
               next_[i+numberColumns] = -(i + numberColumns + 1);
               keyVariable_[i] = numberColumns + i;
               double sol = 0.0;
               if (sol > upper_[i] + tolerance) {
                    setAbove(i);
               } else if (sol < lower_[i] - tolerance) {
                    setBelow(i);
               } else {
                    setFeasible(i);
               }
          }
     }
     delete [] keys;
     delete [] mark;
     delete [] back;
     rhsOffset(model, true);
}
/* Returns effective RHS if it is being used.  This is used for long problems
   or big gub or anywhere where going through full columns is
   expensive.  This may re-compute */
double *
ClpGubDynamicMatrix::rhsOffset(ClpSimplex * model, bool forceRefresh,
                               bool
#ifdef CLP_DEBUG
                               check
#endif
                              )
{
     //forceRefresh=true;
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
               int numberColumns = model->numberColumns();
               int iRow;
               CoinZeroN(rhs, numberRows);
               // do ones at bounds before gub
               const double * smallSolution = model->solutionRegion();
               const double * element = matrix_->getElements();
               const int * row = matrix_->getIndices();
               const CoinBigIndex * startColumn = matrix_->getVectorStarts();
               const int * length = matrix_->getVectorLengths();
               int iColumn;
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
               if (lowerColumn_ || upperColumn_) {
                    double * solution = new double [numberGubColumns_];
                    for (iColumn = 0; iColumn < numberGubColumns_; iColumn++) {
                         double value = 0.0;
                         if(getDynamicStatus(iColumn) == atUpperBound)
                              value = upperColumn_[iColumn];
                         else if (lowerColumn_)
                              value = lowerColumn_[iColumn];
                         solution[iColumn] = value;
                    }
                    // ones at bounds in small and gub
                    for (iColumn = firstDynamic_; iColumn < firstAvailable_; iColumn++) {
                         int jFull = id_[iColumn-firstDynamic_];
                         solution[jFull] = smallSolution[iColumn];
                    }
                    // zero all basic in small model
                    int * pivotVariable = model->pivotVariable();
                    for (iRow = 0; iRow < numberRows; iRow++) {
                         int iColumn = pivotVariable[iRow];
                         if (iColumn >= firstDynamic_ && iColumn < lastDynamic_) {
                              int iSequence = id_[iColumn-firstDynamic_];
                              solution[iSequence] = 0.0;
                         }
                    }
                    // and now compute value to use for key
                    ClpSimplex::Status iStatus;
                    for (int iSet = 0; iSet < numberSets_; iSet++) {
                         iColumn = keyVariable_[iSet];
                         if (iColumn < numberColumns) {
                              int iSequence = id_[iColumn-firstDynamic_];
                              solution[iSequence] = 0.0;
                              double b = 0.0;
                              // key is structural - where is slack
                              iStatus = getStatus(iSet);
                              assert (iStatus != ClpSimplex::basic);
                              if (iStatus == ClpSimplex::atLowerBound)
                                   b = lowerSet_[iSet];
                              else
                                   b = upperSet_[iSet];
                              // subtract out others at bounds
                              for (int j = fullStart_[iSet]; j < fullStart_[iSet+1]; j++)
                                   b -= solution[j];
                              solution[iSequence] = b;
                         }
                    }
                    for (iColumn = 0; iColumn < numberGubColumns_; iColumn++) {
                         double value = solution[iColumn];
                         if (value) {
                              for (CoinBigIndex j = startColumn_[iColumn]; j < startColumn_[iColumn+1]; j++) {
                                   int iRow = row_[j];
                                   rhs[iRow] -= element_[j] * value;
                              }
                         }
                    }
                    // now do lower and upper bounds on sets
                    for (int iSet = 0; iSet < numberSets_; iSet++) {
                         iColumn = keyVariable_[iSet];
                         double shift = 0.0;
                         for (int j = fullStart_[iSet]; j < fullStart_[iSet+1]; j++) {
                              if (getDynamicStatus(j) != inSmall && j != iColumn) {
                                   if (getDynamicStatus(j) == atLowerBound) {
                                        if (lowerColumn_)
                                             shift += lowerColumn_[j];
                                   } else {
                                        shift += upperColumn_[j];
                                   }
                              }
                         }
                         if (lowerSet_[iSet] > -1.0e20)
                              assert(fabs(lower_[iSet] - (lowerSet_[iSet] - shift)) < 1.0e-3);
                         if (upperSet_[iSet] < 1.0e20)
                              assert(fabs(upper_[iSet] - ( upperSet_[iSet] - shift)) < 1.0e-3);
                    }
                    delete [] solution;
               } else {
                    // no bounds
                    ClpSimplex::Status iStatus;
                    for (int iSet = 0; iSet < numberSets_; iSet++) {
                         int iColumn = keyVariable_[iSet];
                         if (iColumn < numberColumns) {
                              int iSequence = id_[iColumn-firstDynamic_];
                              double b = 0.0;
                              // key is structural - where is slack
                              iStatus = getStatus(iSet);
                              assert (iStatus != ClpSimplex::basic);
                              if (iStatus == ClpSimplex::atLowerBound)
                                   b = lower_[iSet];
                              else
                                   b = upper_[iSet];
                              if (b) {
                                   for (CoinBigIndex j = startColumn_[iSequence]; j < startColumn_[iSequence+1]; j++) {
                                        int iRow = row_[j];
                                        rhs[iRow] -= element_[j] * b;
                                   }
                              }
                         }
                    }
               }
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
               int numberColumns = model->numberColumns();
               int iRow;
               CoinZeroN(rhsOffset_, numberRows);
               // do ones at bounds before gub
               const double * smallSolution = model->solutionRegion();
               const double * element = matrix_->getElements();
               const int * row = matrix_->getIndices();
               const CoinBigIndex * startColumn = matrix_->getVectorStarts();
               const int * length = matrix_->getVectorLengths();
               int iColumn;
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
               if (lowerColumn_ || upperColumn_) {
                    double * solution = new double [numberGubColumns_];
                    for (iColumn = 0; iColumn < numberGubColumns_; iColumn++) {
                         double value = 0.0;
                         if(getDynamicStatus(iColumn) == atUpperBound)
                              value = upperColumn_[iColumn];
                         else if (lowerColumn_)
                              value = lowerColumn_[iColumn];
                         solution[iColumn] = value;
                    }
                    // ones in gub and in small problem
                    for (iColumn = firstDynamic_; iColumn < firstAvailable_; iColumn++) {
                         int jFull = id_[iColumn-firstDynamic_];
                         solution[jFull] = smallSolution[iColumn];
                    }
                    // zero all basic in small model
                    int * pivotVariable = model->pivotVariable();
                    for (iRow = 0; iRow < numberRows; iRow++) {
                         int iColumn = pivotVariable[iRow];
                         if (iColumn >= firstDynamic_ && iColumn < lastDynamic_) {
                              int iSequence = id_[iColumn-firstDynamic_];
                              solution[iSequence] = 0.0;
                         }
                    }
                    // and now compute value to use for key
                    ClpSimplex::Status iStatus;
                    int iSet;
                    for ( iSet = 0; iSet < numberSets_; iSet++) {
                         iColumn = keyVariable_[iSet];
                         if (iColumn < numberColumns) {
                              int iSequence = id_[iColumn-firstDynamic_];
                              solution[iSequence] = 0.0;
                              double b = 0.0;
                              // key is structural - where is slack
                              iStatus = getStatus(iSet);
                              assert (iStatus != ClpSimplex::basic);
                              if (iStatus == ClpSimplex::atLowerBound)
                                   b = lowerSet_[iSet];
                              else
                                   b = upperSet_[iSet];
                              // subtract out others at bounds
                              for (int j = fullStart_[iSet]; j < fullStart_[iSet+1]; j++)
                                   b -= solution[j];
                              solution[iSequence] = b;
                         }
                    }
                    for (iColumn = 0; iColumn < numberGubColumns_; iColumn++) {
                         double value = solution[iColumn];
                         if (value) {
                              for (CoinBigIndex j = startColumn_[iColumn]; j < startColumn_[iColumn+1]; j++) {
                                   int iRow = row_[j];
                                   rhsOffset_[iRow] -= element_[j] * value;
                              }
                         }
                    }
                    // now do lower and upper bounds on sets
                    // and offset
                    double objectiveOffset = 0.0;
                    for ( iSet = 0; iSet < numberSets_; iSet++) {
                         iColumn = keyVariable_[iSet];
                         double shift = 0.0;
                         for (CoinBigIndex j = fullStart_[iSet]; j < fullStart_[iSet+1]; j++) {
                              if (getDynamicStatus(j) != inSmall) {
                                   double value = 0.0;
                                   if (getDynamicStatus(j) == atLowerBound) {
                                        if (lowerColumn_)
                                             value = lowerColumn_[j];
                                   } else {
                                        value = upperColumn_[j];
                                   }
                                   if (j != iColumn)
                                        shift += value;
                                   objectiveOffset += value * cost_[j];
                              }
                         }
                         if (lowerSet_[iSet] > -1.0e20)
                              lower_[iSet] = lowerSet_[iSet] - shift;
                         if (upperSet_[iSet] < 1.0e20)
                              upper_[iSet] = upperSet_[iSet] - shift;
                    }
                    delete [] solution;
                    model->setObjectiveOffset(objectiveOffset_ - objectiveOffset);
               } else {
                    // no bounds
                    ClpSimplex::Status iStatus;
                    for (int iSet = 0; iSet < numberSets_; iSet++) {
                         int iColumn = keyVariable_[iSet];
                         if (iColumn < numberColumns) {
                              int iSequence = id_[iColumn-firstDynamic_];
                              double b = 0.0;
                              // key is structural - where is slack
                              iStatus = getStatus(iSet);
                              assert (iStatus != ClpSimplex::basic);
                              if (iStatus == ClpSimplex::atLowerBound)
                                   b = lower_[iSet];
                              else
                                   b = upper_[iSet];
                              if (b) {
                                   for (CoinBigIndex j = startColumn_[iSequence]; j < startColumn_[iSequence+1]; j++) {
                                        int iRow = row_[j];
                                        rhsOffset_[iRow] -= element_[j] * b;
                                   }
                              }
                         }
                    }
               }
#ifdef CLP_DEBUG
               if (saveE) {
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
ClpGubDynamicMatrix::updatePivot(ClpSimplex * model, double oldInValue, double oldOutValue)
{

     // now update working model
     int sequenceIn = model->sequenceIn();
     int sequenceOut = model->sequenceOut();
     bool doPrinting = (model->messageHandler()->logLevel() == 63);
     bool print = false;
     int iSet;
     int trueIn = -1;
     int trueOut = -1;
     int numberRows = model->numberRows();
     int numberColumns = model->numberColumns();
     if (sequenceIn == firstAvailable_) {
          if (doPrinting)
               printf("New variable ");
          if (sequenceIn != sequenceOut) {
               insertNonBasic(firstAvailable_, backward_[firstAvailable_]);
               setDynamicStatus(id_[sequenceIn-firstDynamic_], inSmall);
               firstAvailable_++;
          } else {
               int bigSequence = id_[sequenceIn-firstDynamic_];
               if (model->getStatus(sequenceIn) == ClpSimplex::atUpperBound)
                    setDynamicStatus(bigSequence, atUpperBound);
               else
                    setDynamicStatus(bigSequence, atLowerBound);
          }
          synchronize(model, 8);
     }
     if (sequenceIn < lastDynamic_) {
          iSet = backward_[sequenceIn];
          if (iSet >= 0) {
               int bigSequence = id_[sequenceIn-firstDynamic_];
               trueIn = bigSequence + numberRows + numberColumns + numberSets_;
               if (doPrinting)
                    printf(" incoming set %d big seq %d", iSet, bigSequence);
               print = true;
          }
     } else if (sequenceIn >= numberRows + numberColumns) {
          trueIn = numberRows + numberColumns + gubSlackIn_;
     }
     if (sequenceOut < lastDynamic_) {
          iSet = backward_[sequenceOut];
          if (iSet >= 0) {
               int bigSequence = id_[sequenceOut-firstDynamic_];
               trueOut = bigSequence + firstDynamic_;
               if (getDynamicStatus(bigSequence) != inSmall) {
                    if (model->getStatus(sequenceOut) == ClpSimplex::atUpperBound)
                         setDynamicStatus(bigSequence, atUpperBound);
                    else
                         setDynamicStatus(bigSequence, atLowerBound);
               }
               if (doPrinting)
                    printf(" ,outgoing set %d big seq %d,", iSet, bigSequence);
               print = true;
               model->setSequenceIn(sequenceOut);
               synchronize(model, 8);
               model->setSequenceIn(sequenceIn);
          }
     }
     if (print && doPrinting)
          printf("\n");
     ClpGubMatrix::updatePivot(model, oldInValue, oldOutValue);
     // Redo true in and out
     if (trueIn >= 0)
          trueSequenceIn_ = trueIn;
     if (trueOut >= 0)
          trueSequenceOut_ = trueOut;
     if (doPrinting && 0) {
          for (int i = 0; i < numberSets_; i++) {
               printf("set %d key %d lower %g upper %g\n", i, keyVariable_[i], lower_[i], upper_[i]);
               for (int j = fullStart_[i]; j < fullStart_[i+1]; j++)
                    if (getDynamicStatus(j) == atUpperBound) {
                         bool print = true;
                         for (int k = firstDynamic_; k < firstAvailable_; k++) {
                              if (id_[k-firstDynamic_] == j)
                                   print = false;
                              if (id_[k-firstDynamic_] == j)
                                   assert(getDynamicStatus(j) == inSmall);
                         }
                         if (print)
                              printf("variable %d at ub\n", j);
                    }
          }
     }
#ifdef CLP_DEBUG
     char * inSmall = new char [numberGubColumns_];
     memset(inSmall, 0, numberGubColumns_);
     for (int i = 0; i < numberGubColumns_; i++)
          if (getDynamicStatus(i) == ClpGubDynamicMatrix::inSmall)
               inSmall[i] = 1;
     for (int i = firstDynamic_; i < firstAvailable_; i++) {
          int k = id_[i-firstDynamic_];
          inSmall[k] = 0;
     }
     for (int i = 0; i < numberGubColumns_; i++)
          assert (!inSmall[i]);
     delete [] inSmall;
#endif
     return 0;
}
void
ClpGubDynamicMatrix::times(double scalar,
                           const double * x, double * y) const
{
     if (model_->specialOptions() != 16) {
          ClpPackedMatrix::times(scalar, x, y);
     } else {
          int iRow;
          int numberColumns = model_->numberColumns();
          int numberRows = model_->numberRows();
          const double * element =  matrix_->getElements();
          const int * row = matrix_->getIndices();
          const CoinBigIndex * startColumn = matrix_->getVectorStarts();
          const int * length = matrix_->getVectorLengths();
          int * pivotVariable = model_->pivotVariable();
          int numberToDo = 0;
          for (iRow = 0; iRow < numberRows; iRow++) {
               y[iRow] -= scalar * rhsOffset_[iRow];
               int iColumn = pivotVariable[iRow];
               if (iColumn < numberColumns) {
                    int iSet = backward_[iColumn];
                    if (iSet >= 0 && toIndex_[iSet] < 0) {
                         toIndex_[iSet] = 0;
                         fromIndex_[numberToDo++] = iSet;
                    }
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
          // and gubs which are interacting
          for (int jSet = 0; jSet < numberToDo; jSet++) {
               int iSet = fromIndex_[jSet];
               toIndex_[iSet] = -1;
               int iKey = keyVariable_[iSet];
               if (iKey < numberColumns) {
                    double valueKey;
                    if (getStatus(iSet) == ClpSimplex::atLowerBound)
                         valueKey = lower_[iSet];
                    else
                         valueKey = upper_[iSet];
                    double value = scalar * (x[iKey] - valueKey);
                    if (value) {
                         for (CoinBigIndex j = startColumn[iKey];
                                   j < startColumn[iKey] + length[iKey]; j++) {
                              int jRow = row[j];
                              y[jRow] += value * element[j];
                         }
                    }
               }
          }
     }
}
/* Just for debug - may be extended to other matrix types later.
   Returns number and sum of primal infeasibilities.
*/
int
ClpGubDynamicMatrix::checkFeasible(ClpSimplex * /*model*/, double & sum) const
{
     int numberRows = model_->numberRows();
     double * rhs = new double[numberRows];
     int numberColumns = model_->numberColumns();
     int iRow;
     CoinZeroN(rhs, numberRows);
     // do ones at bounds before gub
     const double * smallSolution = model_->solutionRegion();
     const double * element = matrix_->getElements();
     const int * row = matrix_->getIndices();
     const CoinBigIndex * startColumn = matrix_->getVectorStarts();
     const int * length = matrix_->getVectorLengths();
     int iColumn;
     int numberInfeasible = 0;
     const double * rowLower = model_->rowLower();
     const double * rowUpper = model_->rowUpper();
     sum = 0.0;
     for (iRow = 0; iRow < numberRows; iRow++) {
          double value = smallSolution[numberColumns+iRow];
          if (value < rowLower[iRow] - 1.0e-5 ||
                    value > rowUpper[iRow] + 1.0e-5) {
               //printf("row %d %g %g %g\n",
               //     iRow,rowLower[iRow],value,rowUpper[iRow]);
               numberInfeasible++;
               sum += CoinMax(rowLower[iRow] - value, value - rowUpper[iRow]);
          }
          rhs[iRow] = value;
     }
     const double * columnLower = model_->columnLower();
     const double * columnUpper = model_->columnUpper();
     for (iColumn = 0; iColumn < firstDynamic_; iColumn++) {
          double value = smallSolution[iColumn];
          if (value < columnLower[iColumn] - 1.0e-5 ||
                    value > columnUpper[iColumn] + 1.0e-5) {
               //printf("column %d %g %g %g\n",
               //     iColumn,columnLower[iColumn],value,columnUpper[iColumn]);
               numberInfeasible++;
               sum += CoinMax(columnLower[iColumn] - value, value - columnUpper[iColumn]);
          }
          for (CoinBigIndex j = startColumn[iColumn];
                    j < startColumn[iColumn] + length[iColumn]; j++) {
               int jRow = row[j];
               rhs[jRow] -= value * element[j];
          }
     }
     double * solution = new double [numberGubColumns_];
     for (iColumn = 0; iColumn < numberGubColumns_; iColumn++) {
          double value = 0.0;
          if(getDynamicStatus(iColumn) == atUpperBound)
               value = upperColumn_[iColumn];
          else if (lowerColumn_)
               value = lowerColumn_[iColumn];
          solution[iColumn] = value;
     }
     // ones in small and gub
     for (iColumn = firstDynamic_; iColumn < firstAvailable_; iColumn++) {
          int jFull = id_[iColumn-firstDynamic_];
          solution[jFull] = smallSolution[iColumn];
     }
     // fill in all basic in small model
     int * pivotVariable = model_->pivotVariable();
     for (iRow = 0; iRow < numberRows; iRow++) {
          int iColumn = pivotVariable[iRow];
          if (iColumn >= firstDynamic_ && iColumn < lastDynamic_) {
               int iSequence = id_[iColumn-firstDynamic_];
               solution[iSequence] = smallSolution[iColumn];
          }
     }
     // and now compute value to use for key
     ClpSimplex::Status iStatus;
     for (int iSet = 0; iSet < numberSets_; iSet++) {
          iColumn = keyVariable_[iSet];
          if (iColumn < numberColumns) {
               int iSequence = id_[iColumn-firstDynamic_];
               solution[iSequence] = 0.0;
               double b = 0.0;
               // key is structural - where is slack
               iStatus = getStatus(iSet);
               assert (iStatus != ClpSimplex::basic);
               if (iStatus == ClpSimplex::atLowerBound)
                    b = lower_[iSet];
               else
                    b = upper_[iSet];
               // subtract out others at bounds
               for (int j = fullStart_[iSet]; j < fullStart_[iSet+1]; j++)
                    b -= solution[j];
               solution[iSequence] = b;
          }
     }
     for (iColumn = 0; iColumn < numberGubColumns_; iColumn++) {
          double value = solution[iColumn];
          if ((lowerColumn_ && value < lowerColumn_[iColumn] - 1.0e-5) ||
                    (!lowerColumn_ && value < -1.0e-5) ||
                    (upperColumn_ && value > upperColumn_[iColumn] + 1.0e-5)) {
               //printf("column %d %g %g %g\n",
               //     iColumn,lowerColumn_[iColumn],value,upperColumn_[iColumn]);
               numberInfeasible++;
          }
          if (value) {
               for (CoinBigIndex j = startColumn_[iColumn]; j < startColumn_[iColumn+1]; j++) {
                    int iRow = row_[j];
                    rhs[iRow] -= element_[j] * value;
               }
          }
     }
     for (iRow = 0; iRow < numberRows; iRow++) {
          if (fabs(rhs[iRow]) > 1.0e-5)
               printf("rhs mismatch %d %g\n", iRow, rhs[iRow]);
     }
     delete [] solution;
     delete [] rhs;
     return numberInfeasible;
}
// Cleans data after setWarmStart
void
ClpGubDynamicMatrix::cleanData(ClpSimplex * model)
{
     // and redo chains
     int numberColumns = model->numberColumns();
     int iColumn;
     // do backward
     int * mark = new int [numberGubColumns_];
     for (iColumn = 0; iColumn < numberGubColumns_; iColumn++)
          mark[iColumn] = -1;
     int i;
     for (i = 0; i < firstDynamic_; i++) {
          assert (backward_[i] == -1);
          next_[i] = -1;
     }
     for (i = firstDynamic_; i < firstAvailable_; i++) {
          iColumn = id_[i-firstDynamic_];
          mark[iColumn] = i;
     }
     for (i = 0; i < numberSets_; i++) {
          int iKey = keyVariable_[i];
          int lastNext = -1;
          int firstNext = -1;
          for (CoinBigIndex k = fullStart_[i]; k < fullStart_[i+1]; k++) {
               iColumn = mark[k];
               if (iColumn >= 0) {
                    if (iColumn != iKey) {
                         if (lastNext >= 0)
                              next_[lastNext] = iColumn;
                         else
                              firstNext = iColumn;
                         lastNext = iColumn;
                    }
                    backward_[iColumn] = i;
               }
          }
          setFeasible(i);
          if (firstNext >= 0) {
               // others
               next_[iKey] = firstNext;
               next_[lastNext] = -(iKey + 1);
          } else if (iKey < numberColumns) {
               next_[iKey] = -(iKey + 1);
          }
     }
     delete [] mark;
     // fill matrix
     double * element =  matrix_->getMutableElements();
     int * row = matrix_->getMutableIndices();
     CoinBigIndex * startColumn = matrix_->getMutableVectorStarts();
     int * length = matrix_->getMutableVectorLengths();
     CoinBigIndex numberElements = startColumn[firstDynamic_];
     for (i = firstDynamic_; i < firstAvailable_; i++) {
          int iColumn = id_[i-firstDynamic_];
          int numberThis = startColumn_[iColumn+1] - startColumn_[iColumn];
          length[i] = numberThis;
          for (CoinBigIndex jBigIndex = startColumn_[iColumn];
                    jBigIndex < startColumn_[iColumn+1]; jBigIndex++) {
               row[numberElements] = row_[jBigIndex];
               element[numberElements++] = element_[jBigIndex];
          }
          startColumn[i+1] = numberElements;
     }
}
