/* $Id$ */
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include "CoinPragma.hpp"

#include <iostream>

#include "CoinIndexedVector.hpp"
#include "CoinHelperFunctions.hpp"
#include "ClpMatrixBase.hpp"
#include "ClpSimplex.hpp"

//#############################################################################
// Constructors / Destructor / Assignment
//#############################################################################

//-------------------------------------------------------------------
// Default Constructor
//-------------------------------------------------------------------
ClpMatrixBase::ClpMatrixBase () :
     rhsOffset_(NULL),
     startFraction_(0.0),
     endFraction_(1.0),
     savedBestDj_(0.0),
     originalWanted_(0),
     currentWanted_(0),
     savedBestSequence_(-1),
     type_(-1),
     lastRefresh_(-1),
     refreshFrequency_(0),
     minimumObjectsScan_(-1),
     minimumGoodReducedCosts_(-1),
     trueSequenceIn_(-1),
     trueSequenceOut_(-1),
     skipDualCheck_(false)
{

}

//-------------------------------------------------------------------
// Copy constructor
//-------------------------------------------------------------------
ClpMatrixBase::ClpMatrixBase (const ClpMatrixBase & rhs) :
     type_(rhs.type_),
     skipDualCheck_(rhs.skipDualCheck_)
{
     startFraction_ = rhs.startFraction_;
     endFraction_ = rhs.endFraction_;
     savedBestDj_ = rhs.savedBestDj_;
     originalWanted_ = rhs.originalWanted_;
     currentWanted_ = rhs.currentWanted_;
     savedBestSequence_ = rhs.savedBestSequence_;
     lastRefresh_ = rhs.lastRefresh_;
     refreshFrequency_ = rhs.refreshFrequency_;
     minimumObjectsScan_ = rhs.minimumObjectsScan_;
     minimumGoodReducedCosts_ = rhs.minimumGoodReducedCosts_;
     trueSequenceIn_ = rhs.trueSequenceIn_;
     trueSequenceOut_ = rhs.trueSequenceOut_;
     skipDualCheck_ = rhs.skipDualCheck_;
     int numberRows = rhs.getNumRows();
     if (rhs.rhsOffset_ && numberRows) {
          rhsOffset_ = ClpCopyOfArray(rhs.rhsOffset_, numberRows);
     } else {
          rhsOffset_ = NULL;
     }
}

//-------------------------------------------------------------------
// Destructor
//-------------------------------------------------------------------
ClpMatrixBase::~ClpMatrixBase ()
{
     delete [] rhsOffset_;
}

//----------------------------------------------------------------
// Assignment operator
//-------------------------------------------------------------------
ClpMatrixBase &
ClpMatrixBase::operator=(const ClpMatrixBase& rhs)
{
     if (this != &rhs) {
          type_ = rhs.type_;
          delete [] rhsOffset_;
          int numberRows = rhs.getNumRows();
          if (rhs.rhsOffset_ && numberRows) {
               rhsOffset_ = ClpCopyOfArray(rhs.rhsOffset_, numberRows);
          } else {
               rhsOffset_ = NULL;
          }
          startFraction_ = rhs.startFraction_;
          endFraction_ = rhs.endFraction_;
          savedBestDj_ = rhs.savedBestDj_;
          originalWanted_ = rhs.originalWanted_;
          currentWanted_ = rhs.currentWanted_;
          savedBestSequence_ = rhs.savedBestSequence_;
          lastRefresh_ = rhs.lastRefresh_;
          refreshFrequency_ = rhs.refreshFrequency_;
          minimumObjectsScan_ = rhs.minimumObjectsScan_;
          minimumGoodReducedCosts_ = rhs.minimumGoodReducedCosts_;
          trueSequenceIn_ = rhs.trueSequenceIn_;
          trueSequenceOut_ = rhs.trueSequenceOut_;
          skipDualCheck_ = rhs.skipDualCheck_;
     }
     return *this;
}
// And for scaling - default aborts for when scaling not supported
void
ClpMatrixBase::times(double scalar,
                     const double * x, double * y,
                     const double * rowScale,
                     const double * /*columnScale*/) const
{
     if (rowScale) {
          std::cerr << "Scaling not supported - ClpMatrixBase" << std::endl;
          abort();
     } else {
          times(scalar, x, y);
     }
}
// And for scaling - default aborts for when scaling not supported
void
ClpMatrixBase::transposeTimes(double scalar,
                              const double * x, double * y,
                              const double * rowScale,
                              const double * /*columnScale*/,
                              double * /*spare*/) const
{
     if (rowScale) {
          std::cerr << "Scaling not supported - ClpMatrixBase" << std::endl;
          abort();
     } else {
          transposeTimes(scalar, x, y);
     }
}
/* Subset clone (without gaps).  Duplicates are allowed
   and order is as given.
   Derived classes need not provide this as it may not always make
   sense */
ClpMatrixBase *
ClpMatrixBase::subsetClone (
     int /*numberRows*/, const int * /*whichRows*/,
     int /*numberColumns*/, const int * /*whichColumns*/) const


{
     std::cerr << "subsetClone not supported - ClpMatrixBase" << std::endl;
     abort();
     return NULL;
}
/* Given positive integer weights for each row fills in sum of weights
   for each column (and slack).
   Returns weights vector
   Default returns vector of ones
*/
CoinBigIndex *
ClpMatrixBase::dubiousWeights(const ClpSimplex * model, int * /*inputWeights*/) const
{
     int number = model->numberRows() + model->numberColumns();
     CoinBigIndex * weights = new CoinBigIndex[number];
     int i;
     for (i = 0; i < number; i++)
          weights[i] = 1;
     return weights;
}
#ifndef CLP_NO_VECTOR
// Append Columns
void
ClpMatrixBase::appendCols(int /*number*/,
                          const CoinPackedVectorBase * const * /*columns*/)
{
     std::cerr << "appendCols not supported - ClpMatrixBase" << std::endl;
     abort();
}
// Append Rows
void
ClpMatrixBase::appendRows(int /*number*/,
                          const CoinPackedVectorBase * const * /*rows*/)
{
     std::cerr << "appendRows not supported - ClpMatrixBase" << std::endl;
     abort();
}
#endif
/* Returns largest and smallest elements of both signs.
   Largest refers to largest absolute value.
*/
void
ClpMatrixBase::rangeOfElements(double & smallestNegative, double & largestNegative,
                               double & smallestPositive, double & largestPositive)
{
     smallestNegative = 0.0;
     largestNegative = 0.0;
     smallestPositive = 0.0;
     largestPositive = 0.0;
}
/* The length of a major-dimension vector. */
int
ClpMatrixBase::getVectorLength(int index) const
{
     return getVectorLengths()[index];
}
// Says whether it can do partial pricing
bool
ClpMatrixBase::canDoPartialPricing() const
{
     return false; // default is no
}
/* Return <code>x *A</code> in <code>z</code> but
   just for number indices in y.
   Default cheats with fake CoinIndexedVector and
   then calls subsetTransposeTimes */
void
ClpMatrixBase::listTransposeTimes(const ClpSimplex * model,
                                  double * x,
                                  int * y,
                                  int number,
                                  double * z) const
{
     CoinIndexedVector pi;
     CoinIndexedVector list;
     CoinIndexedVector output;
     int * saveIndices = list.getIndices();
     list.setNumElements(number);
     list.setIndexVector(y);
     double * savePi = pi.denseVector();
     pi.setDenseVector(x);
     double * saveOutput = output.denseVector();
     output.setDenseVector(z);
     output.setPacked();
     subsetTransposeTimes(model, &pi, &list, &output);
     // restore settings
     list.setIndexVector(saveIndices);
     pi.setDenseVector(savePi);
     output.setDenseVector(saveOutput);
}
// Partial pricing
void
ClpMatrixBase::partialPricing(ClpSimplex * , double , double ,
                              int & , int & )
{
     std::cerr << "partialPricing not supported - ClpMatrixBase" << std::endl;
     abort();
}
/* expands an updated column to allow for extra rows which the main
   solver does not know about and returns number added.  If the arrays are NULL
   then returns number of extra entries needed.

   This will normally be a no-op - it is in for GUB!
*/
int
ClpMatrixBase::extendUpdated(ClpSimplex * , CoinIndexedVector * , int )
{
     return 0;
}
/*
     utility primal function for dealing with dynamic constraints
     mode=n see ClpGubMatrix.hpp for definition
     Remember to update here when settled down
*/
void
ClpMatrixBase::primalExpanded(ClpSimplex * , int )
{
}
/*
     utility dual function for dealing with dynamic constraints
     mode=n see ClpGubMatrix.hpp for definition
     Remember to update here when settled down
*/
void
ClpMatrixBase::dualExpanded(ClpSimplex * ,
                            CoinIndexedVector * ,
                            double * , int )
{
}
/*
     general utility function for dealing with dynamic constraints
     mode=n see ClpGubMatrix.hpp for definition
     Remember to update here when settled down
*/
int
ClpMatrixBase::generalExpanded(ClpSimplex * model, int mode, int &number)
{
     int returnCode = 0;
     switch (mode) {
          // Fill in pivotVariable but not for key variables
     case 0: {
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
     // To see if can dual or primal
     case 4: {
          returnCode = 3;
     }
     break;
     default:
          break;
     }
     return returnCode;
}
// Sets up an effective RHS
void
ClpMatrixBase::useEffectiveRhs(ClpSimplex * )
{
     std::cerr << "useEffectiveRhs not supported - ClpMatrixBase" << std::endl;
     abort();
}
/* Returns effective RHS if it is being used.  This is used for long problems
   or big gub or anywhere where going through full columns is
   expensive.  This may re-compute */
double *
ClpMatrixBase::rhsOffset(ClpSimplex * model, bool forceRefresh, bool
#ifdef CLP_DEBUG
                         check
#endif
                        )
{
     if (rhsOffset_) {
#ifdef CLP_DEBUG
          if (check) {
               // no need - but check anyway
               // zero out basic
               int numberRows = model->numberRows();
               int numberColumns = model->numberColumns();
               double * solution = new double [numberColumns];
               double * rhs = new double[numberRows];
               const double * solutionSlack = model->solutionRegion(0);
               CoinMemcpyN(model->solutionRegion(), numberColumns, solution);
               int iRow;
               for (iRow = 0; iRow < numberRows; iRow++) {
                    if (model->getRowStatus(iRow) != ClpSimplex::basic)
                         rhs[iRow] = solutionSlack[iRow];
                    else
                         rhs[iRow] = 0.0;
               }
               for (int iColumn = 0; iColumn < numberColumns; iColumn++) {
                    if (model->getColumnStatus(iColumn) == ClpSimplex::basic)
                         solution[iColumn] = 0.0;
               }
               times(-1.0, solution, rhs);
               delete [] solution;
               for (iRow = 0; iRow < numberRows; iRow++) {
                    if (fabs(rhs[iRow] - rhsOffset_[iRow]) > 1.0e-3)
                         printf("** bad effective %d - true %g old %g\n", iRow, rhs[iRow], rhsOffset_[iRow]);
               }
          }
#endif
          if (forceRefresh || (refreshFrequency_ && model->numberIterations() >=
                               lastRefresh_ + refreshFrequency_)) {
               // zero out basic
               int numberRows = model->numberRows();
               int numberColumns = model->numberColumns();
               double * solution = new double [numberColumns];
               const double * solutionSlack = model->solutionRegion(0);
               CoinMemcpyN(model->solutionRegion(), numberColumns, solution);
               for (int iRow = 0; iRow < numberRows; iRow++) {
                    if (model->getRowStatus(iRow) != ClpSimplex::basic)
                         rhsOffset_[iRow] = solutionSlack[iRow];
                    else
                         rhsOffset_[iRow] = 0.0;
               }
               for (int iColumn = 0; iColumn < numberColumns; iColumn++) {
                    if (model->getColumnStatus(iColumn) == ClpSimplex::basic)
                         solution[iColumn] = 0.0;
               }
               times(-1.0, solution, rhsOffset_);
               delete [] solution;
               lastRefresh_ = model->numberIterations();
          }
     }
     return rhsOffset_;
}
/*
   update information for a pivot (and effective rhs)
*/
int
ClpMatrixBase::updatePivot(ClpSimplex * model, double oldInValue, double )
{
     if (rhsOffset_) {
          // update effective rhs
          int sequenceIn = model->sequenceIn();
          int sequenceOut = model->sequenceOut();
          double * solution = model->solutionRegion();
          int numberColumns = model->numberColumns();
          if (sequenceIn == sequenceOut) {
               if (sequenceIn < numberColumns)
                    add(model, rhsOffset_, sequenceIn, oldInValue - solution[sequenceIn]);
          } else {
               if (sequenceIn < numberColumns)
                    add(model, rhsOffset_, sequenceIn, oldInValue);
               if (sequenceOut < numberColumns)
                    add(model, rhsOffset_, sequenceOut, -solution[sequenceOut]);
          }
     }
     return 0;
}
int
ClpMatrixBase::hiddenRows() const
{
     return 0;
}
/* Creates a variable.  This is called after partial pricing and may modify matrix.
   May update bestSequence.
*/
void
ClpMatrixBase::createVariable(ClpSimplex *, int &)
{
}
// Returns reduced cost of a variable
double
ClpMatrixBase::reducedCost(ClpSimplex * model, int sequence) const
{
     int numberRows = model->numberRows();
     int numberColumns = model->numberColumns();
     if (sequence < numberRows + numberColumns)
          return model->djRegion()[sequence];
     else
          return savedBestDj_;
}
/* Just for debug if odd type matrix.
   Returns number and sum of primal infeasibilities.
*/
int
ClpMatrixBase::checkFeasible(ClpSimplex * model, double & sum) const
{
     int numberRows = model->numberRows();
     double * rhs = new double[numberRows];
     int numberColumns = model->numberColumns();
     int iRow;
     CoinZeroN(rhs, numberRows);
     times(1.0, model->solutionRegion(), rhs, model->rowScale(), model->columnScale());
     int iColumn;
     int logLevel = model->messageHandler()->logLevel();
     int numberInfeasible = 0;
     const double * rowLower = model->lowerRegion(0);
     const double * rowUpper = model->upperRegion(0);
     const double * solution;
     solution = model->solutionRegion(0);
     double tolerance = model->primalTolerance() * 1.01;
     sum = 0.0;
     for (iRow = 0; iRow < numberRows; iRow++) {
          double value = rhs[iRow];
          double value2 = solution[iRow];
          if (logLevel > 3) {
               if (fabs(value - value2) > 1.0e-8)
                    printf("Row %d stored %g, computed %g\n", iRow, value2, value);
          }
          if (value < rowLower[iRow] - tolerance ||
                    value > rowUpper[iRow] + tolerance) {
               numberInfeasible++;
               sum += CoinMax(rowLower[iRow] - value, value - rowUpper[iRow]);
          }
          if (value2 > rowLower[iRow] + tolerance &&
                    value2 < rowUpper[iRow] - tolerance &&
                    model->getRowStatus(iRow) != ClpSimplex::basic) {
               assert (model->getRowStatus(iRow) == ClpSimplex::superBasic);
          }
     }
     const double * columnLower = model->lowerRegion(1);
     const double * columnUpper = model->upperRegion(1);
     solution = model->solutionRegion(1);
     for (iColumn = 0; iColumn < numberColumns; iColumn++) {
          double value = solution[iColumn];
          if (value < columnLower[iColumn] - tolerance ||
                    value > columnUpper[iColumn] + tolerance) {
               numberInfeasible++;
               sum += CoinMax(columnLower[iColumn] - value, value - columnUpper[iColumn]);
          }
          if (value > columnLower[iColumn] + tolerance &&
                    value < columnUpper[iColumn] - tolerance &&
                    model->getColumnStatus(iColumn) != ClpSimplex::basic) {
               assert (model->getColumnStatus(iColumn) == ClpSimplex::superBasic);
          }
     }
     delete [] rhs;
     return numberInfeasible;
}
// These have to match ClpPrimalColumnSteepest version
#define reference(i)  (((reference[i>>5]>>(i&31))&1)!=0)
// Updates second array for steepest and does devex weights (need not be coded)
void
ClpMatrixBase::subsetTimes2(const ClpSimplex * model,
                            CoinIndexedVector * dj1,
                            const CoinIndexedVector * pi2, CoinIndexedVector * dj2,
                            double referenceIn, double devex,
                            // Array for exact devex to say what is in reference framework
                            unsigned int * reference,
                            double * weights, double scaleFactor)
{
     // get subset which have nonzero tableau elements
     subsetTransposeTimes(model, pi2, dj1, dj2);
     bool killDjs = (scaleFactor == 0.0);
     if (!scaleFactor)
          scaleFactor = 1.0;
     // columns

     int number = dj1->getNumElements();
     const int * index = dj1->getIndices();
     double * updateBy = dj1->denseVector();
     double * updateBy2 = dj2->denseVector();

     for (int j = 0; j < number; j++) {
          double thisWeight;
          double pivot;
          double pivotSquared;
          int iSequence = index[j];
          double value2 = updateBy[j];
          if (killDjs)
               updateBy[j] = 0.0;
          double modification = updateBy2[j];
          updateBy2[j] = 0.0;
          ClpSimplex::Status status = model->getStatus(iSequence);

          if (status != ClpSimplex::basic && status != ClpSimplex::isFixed) {
               thisWeight = weights[iSequence];
               pivot = value2 * scaleFactor;
               pivotSquared = pivot * pivot;

               thisWeight += pivotSquared * devex + pivot * modification;
               if (thisWeight < DEVEX_TRY_NORM) {
                    if (referenceIn < 0.0) {
                         // steepest
                         thisWeight = CoinMax(DEVEX_TRY_NORM, DEVEX_ADD_ONE + pivotSquared);
                    } else {
                         // exact
                         thisWeight = referenceIn * pivotSquared;
                         if (reference(iSequence))
                              thisWeight += 1.0;
                         thisWeight = CoinMax(thisWeight, DEVEX_TRY_NORM);
                    }
               }
               weights[iSequence] = thisWeight;
          }
     }
     dj2->setNumElements(0);
}
// Correct sequence in and out to give true value
void
ClpMatrixBase::correctSequence(const ClpSimplex * , int & , int & )
{
}
// Really scale matrix
void
ClpMatrixBase::reallyScale(const double * , const double * )
{
     std::cerr << "reallyScale not supported - ClpMatrixBase" << std::endl;
     abort();
}
// Updates two arrays for steepest
void
ClpMatrixBase::transposeTimes2(const ClpSimplex * ,
                               const CoinIndexedVector * , CoinIndexedVector *,
                               const CoinIndexedVector * ,
                               CoinIndexedVector * ,
                               double , double ,
                               // Array for exact devex to say what is in reference framework
                               unsigned int * ,
                               double * , double )
{
     std::cerr << "transposeTimes2 not supported - ClpMatrixBase" << std::endl;
     abort();
}
/* Set the dimensions of the matrix. In effect, append new empty
   columns/rows to the matrix. A negative number for either dimension
   means that that dimension doesn't change. Otherwise the new dimensions
   MUST be at least as large as the current ones otherwise an exception
   is thrown. */
void
ClpMatrixBase::setDimensions(int , int )
{
     // If odd matrix assume user knows what they are doing
}
/* Append a set of rows/columns to the end of the matrix. Returns number of errors
   i.e. if any of the new rows/columns contain an index that's larger than the
   number of columns-1/rows-1 (if numberOther>0) or duplicates
   If 0 then rows, 1 if columns */
int
ClpMatrixBase::appendMatrix(int , int ,
                            const CoinBigIndex * , const int * ,
                            const double * , int )
{
     std::cerr << "appendMatrix not supported - ClpMatrixBase" << std::endl;
     abort();
     return -1;
}

/* Modify one element of packed matrix.  An element may be added.
   This works for either ordering If the new element is zero it will be
   deleted unless keepZero true */
void
ClpMatrixBase::modifyCoefficient(int , int , double ,
                                 bool )
{
     std::cerr << "modifyCoefficient not supported - ClpMatrixBase" << std::endl;
     abort();
}
#if COIN_LONG_WORK
// For long double versions (aborts if not supported)
void
ClpMatrixBase::times(CoinWorkDouble scalar,
                     const CoinWorkDouble * x, CoinWorkDouble * y) const
{
     std::cerr << "long times not supported - ClpMatrixBase" << std::endl;
     abort();
}
void
ClpMatrixBase::transposeTimes(CoinWorkDouble scalar,
                              const CoinWorkDouble * x, CoinWorkDouble * y) const
{
     std::cerr << "long transposeTimes not supported - ClpMatrixBase" << std::endl;
     abort();
}
#endif
