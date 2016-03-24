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
#include "ClpDynamicExampleMatrix.hpp"
#include "ClpMessage.hpp"
//#define CLP_DEBUG
//#define CLP_DEBUG_PRINT
//#############################################################################
// Constructors / Destructor / Assignment
//#############################################################################

//-------------------------------------------------------------------
// Default Constructor
//-------------------------------------------------------------------
ClpDynamicExampleMatrix::ClpDynamicExampleMatrix ()
     : ClpDynamicMatrix(),
       numberColumns_(0),
       startColumnGen_(NULL),
       rowGen_(NULL),
       elementGen_(NULL),
       costGen_(NULL),
       fullStartGen_(NULL),
       dynamicStatusGen_(NULL),
       idGen_(NULL),
       columnLowerGen_(NULL),
       columnUpperGen_(NULL)
{
     setType(25);
}

//-------------------------------------------------------------------
// Copy constructor
//-------------------------------------------------------------------
ClpDynamicExampleMatrix::ClpDynamicExampleMatrix (const ClpDynamicExampleMatrix & rhs)
     : ClpDynamicMatrix(rhs)
{
     numberColumns_ = rhs.numberColumns_;
     startColumnGen_ = ClpCopyOfArray(rhs.startColumnGen_, numberColumns_ + 1);
     CoinBigIndex numberElements = startColumnGen_[numberColumns_];
     rowGen_ = ClpCopyOfArray(rhs.rowGen_, numberElements);;
     elementGen_ = ClpCopyOfArray(rhs.elementGen_, numberElements);;
     costGen_ = ClpCopyOfArray(rhs.costGen_, numberColumns_);
     fullStartGen_ = ClpCopyOfArray(rhs.fullStartGen_, numberSets_ + 1);
     dynamicStatusGen_ = ClpCopyOfArray(rhs.dynamicStatusGen_, numberColumns_);
     idGen_ = ClpCopyOfArray(rhs.idGen_, maximumGubColumns_);
     columnLowerGen_ = ClpCopyOfArray(rhs.columnLowerGen_, numberColumns_);
     columnUpperGen_ = ClpCopyOfArray(rhs.columnUpperGen_, numberColumns_);
}

/* This is the real constructor*/
ClpDynamicExampleMatrix::ClpDynamicExampleMatrix(ClpSimplex * model, int numberSets,
          int numberGubColumns, const int * starts,
          const double * lower, const double * upper,
          const CoinBigIndex * startColumn, const int * row,
          const double * element, const double * cost,
          const double * columnLower, const double * columnUpper,
          const unsigned char * status,
          const unsigned char * dynamicStatus,
          int numberIds, const int *ids)
     : ClpDynamicMatrix(model, numberSets, 0, NULL, lower, upper, NULL, NULL, NULL, NULL, NULL, NULL,
                        NULL, NULL)
{
     setType(25);
     numberColumns_ = numberGubColumns;
     // start with safe values - then experiment
     maximumGubColumns_ = numberColumns_;
     maximumElements_ = startColumn[numberColumns_];
     // delete odd stuff created by ClpDynamicMatrix constructor
     delete [] startSet_;
     startSet_ = new int [numberSets_];
     delete [] next_;
     next_ = new int [maximumGubColumns_];
     delete [] row_;
     delete [] element_;
     delete [] startColumn_;
     delete [] cost_;
     delete [] columnLower_;
     delete [] columnUpper_;
     delete [] dynamicStatus_;
     delete [] status_;
     delete [] id_;
     // and size correctly
     row_ = new int [maximumElements_];
     element_ = new double [maximumElements_];
     startColumn_ = new CoinBigIndex [maximumGubColumns_+1];
     // say no columns yet
     numberGubColumns_ = 0;
     startColumn_[0] = 0;
     cost_ = new double[maximumGubColumns_];
     dynamicStatus_ = new unsigned char [2*maximumGubColumns_];
     memset(dynamicStatus_, 0, maximumGubColumns_);
     id_ = new int[maximumGubColumns_];
     if (columnLower)
          columnLower_ = new double[maximumGubColumns_];
     else
          columnLower_ = NULL;
     if (columnUpper)
          columnUpper_ = new double[maximumGubColumns_];
     else
          columnUpper_ = NULL;
     // space for ids
     idGen_ = new int [maximumGubColumns_];
     int iSet;
     for (iSet = 0; iSet < numberSets_; iSet++)
          startSet_[iSet] = -1;
     // This starts code specific to this storage method
     CoinBigIndex i;
     fullStartGen_ = ClpCopyOfArray(starts, numberSets_ + 1);
     startColumnGen_ = ClpCopyOfArray(startColumn, numberColumns_ + 1);
     CoinBigIndex numberElements = startColumnGen_[numberColumns_];
     rowGen_ = ClpCopyOfArray(row, numberElements);
     elementGen_ = new double[numberElements];
     for (i = 0; i < numberElements; i++)
          elementGen_[i] = element[i];
     costGen_ = new double[numberColumns_];
     for (i = 0; i < numberColumns_; i++) {
          costGen_[i] = cost[i];
          // I don't think I need sorted but ...
          CoinSort_2(rowGen_ + startColumnGen_[i], rowGen_ + startColumnGen_[i+1], elementGen_ + startColumnGen_[i]);
     }
     if (columnLower) {
          columnLowerGen_ = new double[numberColumns_];
          for (i = 0; i < numberColumns_; i++) {
               columnLowerGen_[i] = columnLower[i];
               if (columnLowerGen_[i]) {
                    printf("Non-zero lower bounds not allowed - subtract from model\n");
                    abort();
               }
          }
     } else {
          columnLowerGen_ = NULL;
     }
     if (columnUpper) {
          columnUpperGen_ = new double[numberColumns_];
          for (i = 0; i < numberColumns_; i++)
               columnUpperGen_[i] = columnUpper[i];
     } else {
          columnUpperGen_ = NULL;
     }
     // end specific coding
     if (columnUpper_) {
          // set all upper bounds so we have enough space
          double * columnUpper = model->columnUpper();
          for(i = firstDynamic_; i < lastDynamic_; i++)
               columnUpper[i] = 1.0e10;
     }
     status_ = new unsigned char [2*numberSets_+4];
     if (status) {
          memcpy(status_,status, numberSets_ * sizeof(char));
          assert (dynamicStatus);
          CoinMemcpyN(dynamicStatus, numberIds, dynamicStatus_);
          assert (numberIds);
     } else {
          assert (!numberIds);
          memset(status_, 0, numberSets_);
          for (i = 0; i < numberSets_; i++) {
               // make slack key
               setStatus(i, ClpSimplex::basic);
          }
     }
     dynamicStatusGen_ = new unsigned char [numberColumns_];
     memset(dynamicStatusGen_, 0, numberColumns_); // for clarity
     for (i = 0; i < numberColumns_; i++)
          setDynamicStatusGen(i, atLowerBound);
     // Populate with enough columns
     if (!numberIds) {
          // This could be made more sophisticated
          for (iSet = 0; iSet < numberSets_; iSet++) {
               int sequence = fullStartGen_[iSet];
               CoinBigIndex start = startColumnGen_[sequence];
               addColumn(startColumnGen_[sequence+1] - start,
                         rowGen_ + start,
                         elementGen_ + start,
                         costGen_[sequence],
                         columnLowerGen_ ? columnLowerGen_[sequence] : 0,
                         columnUpperGen_ ? columnUpperGen_[sequence] : 1.0e30,
                         iSet, getDynamicStatusGen(sequence));
               idGen_[iSet] = sequence; // say which one in
               setDynamicStatusGen(sequence, inSmall);
          }
     } else {
          // put back old ones
          int * set = new int[numberColumns_];
          for (iSet = 0; iSet < numberSets_; iSet++) {
               for (CoinBigIndex j = fullStartGen_[iSet]; j < fullStartGen_[iSet+1]; j++)
                    set[j] = iSet;
          }
          for (int i = 0; i < numberIds; i++) {
               int sequence = ids[i];
               CoinBigIndex start = startColumnGen_[sequence];
               addColumn(startColumnGen_[sequence+1] - start,
                         rowGen_ + start,
                         elementGen_ + start,
                         costGen_[sequence],
                         columnLowerGen_ ? columnLowerGen_[sequence] : 0,
                         columnUpperGen_ ? columnUpperGen_[sequence] : 1.0e30,
                         set[sequence], getDynamicStatus(i));
               idGen_[iSet] = sequence; // say which one in
               setDynamicStatusGen(sequence, inSmall);
          }
          delete [] set;
     }
     if (!status) {
          gubCrash();
     } else {
          initialProblem();
     }
}
#if 0
// This constructor just takes over ownership
ClpDynamicExampleMatrix::ClpDynamicExampleMatrix(ClpSimplex * model, int numberSets,
          int numberGubColumns, int * starts,
          const double * lower, const double * upper,
          int * startColumn, int * row,
          double * element, double * cost,
          double * columnLower, double * columnUpper,
          const unsigned char * status,
          const unsigned char * dynamicStatus,
          int numberIds, const int *ids)
     : ClpDynamicMatrix(model, numberSets, 0, NULL, lower, upper, NULL, NULL, NULL, NULL, NULL, NULL,
                        NULL, NULL)
{
     setType(25);
     numberColumns_ = numberGubColumns;
     // start with safe values - then experiment
     maximumGubColumns_ = numberColumns_;
     maximumElements_ = startColumn[numberColumns_];
     // delete odd stuff created by ClpDynamicMatrix constructor
     delete [] startSet_;
     startSet_ = new int [numberSets_];
     delete [] next_;
     next_ = new int [maximumGubColumns_];
     delete [] row_;
     delete [] element_;
     delete [] startColumn_;
     delete [] cost_;
     delete [] columnLower_;
     delete [] columnUpper_;
     delete [] dynamicStatus_;
     delete [] status_;
     delete [] id_;
     // and size correctly
     row_ = new int [maximumElements_];
     element_ = new double [maximumElements_];
     startColumn_ = new CoinBigIndex [maximumGubColumns_+1];
     // say no columns yet
     numberGubColumns_ = 0;
     startColumn_[0] = 0;
     cost_ = new double[maximumGubColumns_];
     dynamicStatus_ = new unsigned char [2*maximumGubColumns_];
     memset(dynamicStatus_, 0, maximumGubColumns_);
     id_ = new int[maximumGubColumns_];
     if (columnLower)
          columnLower_ = new double[maximumGubColumns_];
     else
          columnLower_ = NULL;
     if (columnUpper)
          columnUpper_ = new double[maximumGubColumns_];
     else
          columnUpper_ = NULL;
     // space for ids
     idGen_ = new int [maximumGubColumns_];
     int iSet;
     for (iSet = 0; iSet < numberSets_; iSet++)
          startSet_[iSet] = -1;
     // This starts code specific to this storage method
     CoinBigIndex i;
     fullStartGen_ = starts;
     startColumnGen_ = startColumn;
     rowGen_ = row;
     elementGen_ = element;
     costGen_ = cost;
     for (i = 0; i < numberColumns_; i++) {
          // I don't think I need sorted but ...
          CoinSort_2(rowGen_ + startColumnGen_[i], rowGen_ + startColumnGen_[i+1], elementGen_ + startColumnGen_[i]);
     }
     if (columnLower) {
          columnLowerGen_ = columnLower;
          for (i = 0; i < numberColumns_; i++) {
               if (columnLowerGen_[i]) {
                    printf("Non-zero lower bounds not allowed - subtract from model\n");
                    abort();
               }
          }
     } else {
          columnLowerGen_ = NULL;
     }
     if (columnUpper) {
          columnUpperGen_ = columnUpper;
     } else {
          columnUpperGen_ = NULL;
     }
     // end specific coding
     if (columnUpper_) {
          // set all upper bounds so we have enough space
          double * columnUpper = model->columnUpper();
          for(i = firstDynamic_; i < lastDynamic_; i++)
               columnUpper[i] = 1.0e10;
     }
     status_ = new unsigned char [2*numberSets_+4];
     if (status) {
          memcpy(status_,status, numberSets_ * sizeof(char));
          assert (dynamicStatus);
          CoinMemcpyN(dynamicStatus, numberIds, dynamicStatus_);
          assert (numberIds);
     } else {
          assert (!numberIds);
          memset(status_, 0, numberSets_);
          for (i = 0; i < numberSets_; i++) {
               // make slack key
               setStatus(i, ClpSimplex::basic);
          }
     }
     dynamicStatusGen_ = new unsigned char [numberColumns_];
     memset(dynamicStatusGen_, 0, numberColumns_); // for clarity
     for (i = 0; i < numberColumns_; i++)
          setDynamicStatusGen(i, atLowerBound);
     // Populate with enough columns
     if (!numberIds) {
          // This could be made more sophisticated
          for (iSet = 0; iSet < numberSets_; iSet++) {
               int sequence = fullStartGen_[iSet];
               CoinBigIndex start = startColumnGen_[sequence];
               addColumn(startColumnGen_[sequence+1] - start,
                         rowGen_ + start,
                         elementGen_ + start,
                         costGen_[sequence],
                         columnLowerGen_ ? columnLowerGen_[sequence] : 0,
                         columnUpperGen_ ? columnUpperGen_[sequence] : 1.0e30,
                         iSet, getDynamicStatusGen(sequence));
               idGen_[iSet] = sequence; // say which one in
               setDynamicStatusGen(sequence, inSmall);
          }
     } else {
          // put back old ones
          int * set = new int[numberColumns_];
          for (iSet = 0; iSet < numberSets_; iSet++) {
               for (CoinBigIndex j = fullStartGen_[iSet]; j < fullStartGen_[iSet+1]; j++)
                    set[j] = iSet;
          }
          for (int i = 0; i < numberIds; i++) {
               int sequence = ids[i];
               int iSet = set[sequence];
               CoinBigIndex start = startColumnGen_[sequence];
               addColumn(startColumnGen_[sequence+1] - start,
                         rowGen_ + start,
                         elementGen_ + start,
                         costGen_[sequence],
                         columnLowerGen_ ? columnLowerGen_[sequence] : 0,
                         columnUpperGen_ ? columnUpperGen_[sequence] : 1.0e30,
                         iSet, getDynamicStatus(i));
               idGen_[i] = sequence; // say which one in
               setDynamicStatusGen(sequence, inSmall);
          }
          delete [] set;
     }
     if (!status) {
          gubCrash();
     } else {
          initialProblem();
     }
}
#endif
//-------------------------------------------------------------------
// Destructor
//-------------------------------------------------------------------
ClpDynamicExampleMatrix::~ClpDynamicExampleMatrix ()
{
     delete [] startColumnGen_;
     delete [] rowGen_;
     delete [] elementGen_;
     delete [] costGen_;
     delete [] fullStartGen_;
     delete [] dynamicStatusGen_;
     delete [] idGen_;
     delete [] columnLowerGen_;
     delete [] columnUpperGen_;
}

//----------------------------------------------------------------
// Assignment operator
//-------------------------------------------------------------------
ClpDynamicExampleMatrix &
ClpDynamicExampleMatrix::operator=(const ClpDynamicExampleMatrix& rhs)
{
     if (this != &rhs) {
          ClpDynamicMatrix::operator=(rhs);
          numberColumns_ = rhs.numberColumns_;
          delete [] startColumnGen_;
          delete [] rowGen_;
          delete [] elementGen_;
          delete [] costGen_;
          delete [] fullStartGen_;
          delete [] dynamicStatusGen_;
          delete [] idGen_;
          delete [] columnLowerGen_;
          delete [] columnUpperGen_;
          startColumnGen_ = ClpCopyOfArray(rhs.startColumnGen_, numberColumns_ + 1);
          CoinBigIndex numberElements = startColumnGen_[numberColumns_];
          rowGen_ = ClpCopyOfArray(rhs.rowGen_, numberElements);;
          elementGen_ = ClpCopyOfArray(rhs.elementGen_, numberElements);;
          costGen_ = ClpCopyOfArray(rhs.costGen_, numberColumns_);
          fullStartGen_ = ClpCopyOfArray(rhs.fullStartGen_, numberSets_ + 1);
          dynamicStatusGen_ = ClpCopyOfArray(rhs.dynamicStatusGen_, numberColumns_);
          idGen_ = ClpCopyOfArray(rhs.idGen_, maximumGubColumns_);
          columnLowerGen_ = ClpCopyOfArray(rhs.columnLowerGen_, numberColumns_);
          columnUpperGen_ = ClpCopyOfArray(rhs.columnUpperGen_, numberColumns_);
     }
     return *this;
}
//-------------------------------------------------------------------
// Clone
//-------------------------------------------------------------------
ClpMatrixBase * ClpDynamicExampleMatrix::clone() const
{
     return new ClpDynamicExampleMatrix(*this);
}
// Partial pricing
void
ClpDynamicExampleMatrix::partialPricing(ClpSimplex * model, double startFraction, double endFraction,
                                        int & bestSequence, int & numberWanted)
{
     numberWanted = currentWanted_;
     assert(!model->rowScale());
     if (!numberSets_) {
          // no gub
          ClpPackedMatrix::partialPricing(model, startFraction, endFraction, bestSequence, numberWanted);
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
          double bestDj;
          int numberRows = model->numberRows();
          int slackOffset = lastDynamic_ + numberRows;
          int structuralOffset = slackOffset + numberSets_;
          int structuralOffset2 = structuralOffset + maximumGubColumns_;
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
                    if (iBasic >= numberColumns_) {
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
               // do ones in small
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
                                        bestDj = value;
                                        bestSequence = structuralOffset + iSequence;
                                        bestDjMod = djMod;
                                        bestSet = iSet;
                                   } else {
                                        // just to make sure we don't exit before got something
                                        numberWanted++;
                                   }
                              }
                         }
                    }
                    iSequence = next_[iSequence]; //onto next in set
               }
               // and now get best by column generation
               // If no upper bounds we may not need status test
               for (iSequence = fullStartGen_[iSet]; iSequence < fullStartGen_[iSet+1]; iSequence++) {
                    DynamicStatus status = getDynamicStatusGen(iSequence);
                    assert (status != atUpperBound && status != soloKey);
                    if (status == atLowerBound) {
                         double value = costGen_[iSequence] - djMod;
                         for (CoinBigIndex j = startColumnGen_[iSequence];
                                   j < startColumnGen_[iSequence+1]; j++) {
                              int jRow = rowGen_[j];
                              value -= duals[jRow] * elementGen_[j];
                         }
                         // change sign as at lower bound
                         value = -value;
                         if (value > tolerance) {
                              numberWanted--;
                              if (value > bestDj) {
                                   // check flagged variable and correct dj
                                   if (!flaggedGen(iSequence)) {
                                        bestDj = value;
                                        bestSequence = structuralOffset2 + iSequence;
                                        bestDjMod = djMod;
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
          if (bestSequence != saveSequence) {
               savedBestGubDual_ = bestDjMod;
               savedBestDj_ = bestDj;
               savedBestSequence_ = bestSequence;
               savedBestSet_ = bestSet;
          }
          // Do packed part before gub
          // always???
          // Resize so just do to gub
          numberActiveColumns_ = firstDynamic_;
          int saveMinNeg = minimumGoodReducedCosts_;
          if (bestSequence >= 0)
               minimumGoodReducedCosts_ = -2;
          currentWanted_ = numberWanted;
          ClpPackedMatrix::partialPricing(model, startFraction, endFraction, bestSequence, numberWanted);
          numberActiveColumns_ = matrix_->getNumCols();
          minimumGoodReducedCosts_ = saveMinNeg;
          // See if may be finished
          if (!startG2 && bestSequence < 0)
               infeasibilityWeight_ = model_->infeasibilityCost();
          else if (bestSequence >= 0)
               infeasibilityWeight_ = -1.0;
          currentWanted_ = numberWanted;
     }
}
/* Creates a variable.  This is called after partial pricing and may modify matrix.
   May update bestSequence.
*/
void
ClpDynamicExampleMatrix::createVariable(ClpSimplex * model, int & bestSequence)
{
     int numberRows = model->numberRows();
     int slackOffset = lastDynamic_ + numberRows;
     int structuralOffset = slackOffset + numberSets_;
     int bestSequence2 = savedBestSequence_ - structuralOffset;
     if (bestSequence2 >= 0) {
          // See if needs new
          if (bestSequence2 >= maximumGubColumns_) {
               bestSequence2 -= maximumGubColumns_;
               int sequence = addColumn(startColumnGen_[bestSequence2+1] - startColumnGen_[bestSequence2],
                                        rowGen_ + startColumnGen_[bestSequence2],
                                        elementGen_ + startColumnGen_[bestSequence2],
                                        costGen_[bestSequence2],
                                        columnLowerGen_ ? columnLowerGen_[bestSequence2] : 0,
                                        columnUpperGen_ ? columnUpperGen_[bestSequence2] : 1.0e30,
                                        savedBestSet_, getDynamicStatusGen(bestSequence2));
               savedBestSequence_ = structuralOffset + sequence;
               idGen_[sequence] = bestSequence2;
               setDynamicStatusGen(bestSequence2, inSmall);
          }
     }
     ClpDynamicMatrix::createVariable(model, bestSequence/*, bestSequence2*/);
     // clear for next iteration
     savedBestSequence_ = -1;
}
/* If addColumn forces compression then this allows descendant to know what to do.
   If >=0 then entry stayed in, if -1 then entry went out to lower bound.of zero.
   Entries at upper bound (really nonzero) never go out (at present).
*/
void
ClpDynamicExampleMatrix::packDown(const int * in, int numberToPack)
{
     int put = 0;
     for (int i = 0; i < numberToPack; i++) {
          int id = idGen_[i];
          if (in[i] >= 0) {
               // stays in
               assert (put == in[i]); // true for now
               idGen_[put++] = id;
          } else {
               // out to lower bound
               setDynamicStatusGen(id, atLowerBound);
          }
     }
     assert (put == numberGubColumns_);
}
