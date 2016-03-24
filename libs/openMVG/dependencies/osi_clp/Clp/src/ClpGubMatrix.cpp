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
#include "ClpGubMatrix.hpp"
//#include "ClpGubDynamicMatrix.hpp"
#include "ClpMessage.hpp"
//#define CLP_DEBUG
//#define CLP_DEBUG_PRINT
//#############################################################################
// Constructors / Destructor / Assignment
//#############################################################################

//-------------------------------------------------------------------
// Default Constructor
//-------------------------------------------------------------------
ClpGubMatrix::ClpGubMatrix ()
     : ClpPackedMatrix(),
       sumDualInfeasibilities_(0.0),
       sumPrimalInfeasibilities_(0.0),
       sumOfRelaxedDualInfeasibilities_(0.0),
       sumOfRelaxedPrimalInfeasibilities_(0.0),
       infeasibilityWeight_(0.0),
       start_(NULL),
       end_(NULL),
       lower_(NULL),
       upper_(NULL),
       status_(NULL),
       saveStatus_(NULL),
       savedKeyVariable_(NULL),
       backward_(NULL),
       backToPivotRow_(NULL),
       changeCost_(NULL),
       keyVariable_(NULL),
       next_(NULL),
       toIndex_(NULL),
       fromIndex_(NULL),
       model_(NULL),
       numberDualInfeasibilities_(0),
       numberPrimalInfeasibilities_(0),
       noCheck_(-1),
       numberSets_(0),
       saveNumber_(0),
       possiblePivotKey_(0),
       gubSlackIn_(-1),
       firstGub_(0),
       lastGub_(0),
       gubType_(0)
{
     setType(16);
}

//-------------------------------------------------------------------
// Copy constructor
//-------------------------------------------------------------------
ClpGubMatrix::ClpGubMatrix (const ClpGubMatrix & rhs)
     : ClpPackedMatrix(rhs)
{
     numberSets_ = rhs.numberSets_;
     saveNumber_ = rhs.saveNumber_;
     possiblePivotKey_ = rhs.possiblePivotKey_;
     gubSlackIn_ = rhs.gubSlackIn_;
     start_ = ClpCopyOfArray(rhs.start_, numberSets_);
     end_ = ClpCopyOfArray(rhs.end_, numberSets_);
     lower_ = ClpCopyOfArray(rhs.lower_, numberSets_);
     upper_ = ClpCopyOfArray(rhs.upper_, numberSets_);
     status_ = ClpCopyOfArray(rhs.status_, numberSets_);
     saveStatus_ = ClpCopyOfArray(rhs.saveStatus_, numberSets_);
     savedKeyVariable_ = ClpCopyOfArray(rhs.savedKeyVariable_, numberSets_);
     int numberColumns = getNumCols();
     backward_ = ClpCopyOfArray(rhs.backward_, numberColumns);
     backToPivotRow_ = ClpCopyOfArray(rhs.backToPivotRow_, numberColumns);
     changeCost_ = ClpCopyOfArray(rhs.changeCost_, getNumRows() + numberSets_);
     fromIndex_ = ClpCopyOfArray(rhs.fromIndex_, getNumRows() + numberSets_ + 1);
     keyVariable_ = ClpCopyOfArray(rhs.keyVariable_, numberSets_);
     // find longest set
     int * longest = new int[numberSets_];
     CoinZeroN(longest, numberSets_);
     int j;
     for (j = 0; j < numberColumns; j++) {
          int iSet = backward_[j];
          if (iSet >= 0)
               longest[iSet]++;
     }
     int length = 0;
     for (j = 0; j < numberSets_; j++)
          length = CoinMax(length, longest[j]);
     next_ = ClpCopyOfArray(rhs.next_, numberColumns + numberSets_ + 2 * length);
     toIndex_ = ClpCopyOfArray(rhs.toIndex_, numberSets_);
     sumDualInfeasibilities_ = rhs. sumDualInfeasibilities_;
     sumPrimalInfeasibilities_ = rhs.sumPrimalInfeasibilities_;
     sumOfRelaxedDualInfeasibilities_ = rhs.sumOfRelaxedDualInfeasibilities_;
     sumOfRelaxedPrimalInfeasibilities_ = rhs.sumOfRelaxedPrimalInfeasibilities_;
     infeasibilityWeight_ = rhs.infeasibilityWeight_;
     numberDualInfeasibilities_ = rhs.numberDualInfeasibilities_;
     numberPrimalInfeasibilities_ = rhs.numberPrimalInfeasibilities_;
     noCheck_ = rhs.noCheck_;
     firstGub_ = rhs.firstGub_;
     lastGub_ = rhs.lastGub_;
     gubType_ = rhs.gubType_;
     model_ = rhs.model_;
}

//-------------------------------------------------------------------
// assign matrix (for space reasons)
//-------------------------------------------------------------------
ClpGubMatrix::ClpGubMatrix (CoinPackedMatrix * rhs)
     : ClpPackedMatrix(rhs),
       sumDualInfeasibilities_(0.0),
       sumPrimalInfeasibilities_(0.0),
       sumOfRelaxedDualInfeasibilities_(0.0),
       sumOfRelaxedPrimalInfeasibilities_(0.0),
       infeasibilityWeight_(0.0),
       start_(NULL),
       end_(NULL),
       lower_(NULL),
       upper_(NULL),
       status_(NULL),
       saveStatus_(NULL),
       savedKeyVariable_(NULL),
       backward_(NULL),
       backToPivotRow_(NULL),
       changeCost_(NULL),
       keyVariable_(NULL),
       next_(NULL),
       toIndex_(NULL),
       fromIndex_(NULL),
       model_(NULL),
       numberDualInfeasibilities_(0),
       numberPrimalInfeasibilities_(0),
       noCheck_(-1),
       numberSets_(0),
       saveNumber_(0),
       possiblePivotKey_(0),
       gubSlackIn_(-1),
       firstGub_(0),
       lastGub_(0),
       gubType_(0)
{
     setType(16);
}

/* This takes over ownership (for space reasons) and is the
   real constructor*/
ClpGubMatrix::ClpGubMatrix(ClpPackedMatrix * matrix, int numberSets,
                           const int * start, const int * end,
                           const double * lower, const double * upper,
                           const unsigned char * status)
     : ClpPackedMatrix(matrix->matrix()),
       sumDualInfeasibilities_(0.0),
       sumPrimalInfeasibilities_(0.0),
       sumOfRelaxedDualInfeasibilities_(0.0),
       sumOfRelaxedPrimalInfeasibilities_(0.0),
       numberDualInfeasibilities_(0),
       numberPrimalInfeasibilities_(0),
       saveNumber_(0),
       possiblePivotKey_(0),
       gubSlackIn_(-1)
{
     model_ = NULL;
     numberSets_ = numberSets;
     start_ = ClpCopyOfArray(start, numberSets_);
     end_ = ClpCopyOfArray(end, numberSets_);
     lower_ = ClpCopyOfArray(lower, numberSets_);
     upper_ = ClpCopyOfArray(upper, numberSets_);
     // Check valid and ordered
     int last = -1;
     int numberColumns = matrix_->getNumCols();
     int numberRows = matrix_->getNumRows();
     backward_ = new int[numberColumns];
     backToPivotRow_ = new int[numberColumns];
     changeCost_ = new double [numberRows+numberSets_];
     keyVariable_ = new int[numberSets_];
     // signal to need new ordering
     next_ = NULL;
     for (int iColumn = 0; iColumn < numberColumns; iColumn++)
          backward_[iColumn] = -1;

     int iSet;
     for (iSet = 0; iSet < numberSets_; iSet++) {
          // set key variable as slack
          keyVariable_[iSet] = iSet + numberColumns;
          if (start_[iSet] < 0 || start_[iSet] >= numberColumns)
               throw CoinError("Index out of range", "constructor", "ClpGubMatrix");
          if (end_[iSet] < 0 || end_[iSet] > numberColumns)
               throw CoinError("Index out of range", "constructor", "ClpGubMatrix");
          if (end_[iSet] <= start_[iSet])
               throw CoinError("Empty or negative set", "constructor", "ClpGubMatrix");
          if (start_[iSet] < last)
               throw CoinError("overlapping or non-monotonic sets", "constructor", "ClpGubMatrix");
          last = end_[iSet];
          int j;
          for (j = start_[iSet]; j < end_[iSet]; j++)
               backward_[j] = iSet;
     }
     // Find type of gub
     firstGub_ = numberColumns + 1;
     lastGub_ = -1;
     int i;
     for (i = 0; i < numberColumns; i++) {
          if (backward_[i] >= 0) {
               firstGub_ = CoinMin(firstGub_, i);
               lastGub_ = CoinMax(lastGub_, i);
          }
     }
     gubType_ = 0;
     // adjust lastGub_
     if (lastGub_ > 0)
          lastGub_++;
     for (i = firstGub_; i < lastGub_; i++) {
          if (backward_[i] < 0) {
               gubType_ = 1;
               printf("interior non gub %d\n", i);
               break;
          }
     }
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
     noCheck_ = -1;
     infeasibilityWeight_ = 0.0;
}

ClpGubMatrix::ClpGubMatrix (const CoinPackedMatrix & rhs)
     : ClpPackedMatrix(rhs),
       sumDualInfeasibilities_(0.0),
       sumPrimalInfeasibilities_(0.0),
       sumOfRelaxedDualInfeasibilities_(0.0),
       sumOfRelaxedPrimalInfeasibilities_(0.0),
       infeasibilityWeight_(0.0),
       start_(NULL),
       end_(NULL),
       lower_(NULL),
       upper_(NULL),
       status_(NULL),
       saveStatus_(NULL),
       savedKeyVariable_(NULL),
       backward_(NULL),
       backToPivotRow_(NULL),
       changeCost_(NULL),
       keyVariable_(NULL),
       next_(NULL),
       toIndex_(NULL),
       fromIndex_(NULL),
       model_(NULL),
       numberDualInfeasibilities_(0),
       numberPrimalInfeasibilities_(0),
       noCheck_(-1),
       numberSets_(0),
       saveNumber_(0),
       possiblePivotKey_(0),
       gubSlackIn_(-1),
       firstGub_(0),
       lastGub_(0),
       gubType_(0)
{
     setType(16);

}

//-------------------------------------------------------------------
// Destructor
//-------------------------------------------------------------------
ClpGubMatrix::~ClpGubMatrix ()
{
     delete [] start_;
     delete [] end_;
     delete [] lower_;
     delete [] upper_;
     delete [] status_;
     delete [] saveStatus_;
     delete [] savedKeyVariable_;
     delete [] backward_;
     delete [] backToPivotRow_;
     delete [] changeCost_;
     delete [] keyVariable_;
     delete [] next_;
     delete [] toIndex_;
     delete [] fromIndex_;
}

//----------------------------------------------------------------
// Assignment operator
//-------------------------------------------------------------------
ClpGubMatrix &
ClpGubMatrix::operator=(const ClpGubMatrix& rhs)
{
     if (this != &rhs) {
          ClpPackedMatrix::operator=(rhs);
          delete [] start_;
          delete [] end_;
          delete [] lower_;
          delete [] upper_;
          delete [] status_;
          delete [] saveStatus_;
          delete [] savedKeyVariable_;
          delete [] backward_;
          delete [] backToPivotRow_;
          delete [] changeCost_;
          delete [] keyVariable_;
          delete [] next_;
          delete [] toIndex_;
          delete [] fromIndex_;
          numberSets_ = rhs.numberSets_;
          saveNumber_ = rhs.saveNumber_;
          possiblePivotKey_ = rhs.possiblePivotKey_;
          gubSlackIn_ = rhs.gubSlackIn_;
          start_ = ClpCopyOfArray(rhs.start_, numberSets_);
          end_ = ClpCopyOfArray(rhs.end_, numberSets_);
          lower_ = ClpCopyOfArray(rhs.lower_, numberSets_);
          upper_ = ClpCopyOfArray(rhs.upper_, numberSets_);
          status_ = ClpCopyOfArray(rhs.status_, numberSets_);
          saveStatus_ = ClpCopyOfArray(rhs.saveStatus_, numberSets_);
          savedKeyVariable_ = ClpCopyOfArray(rhs.savedKeyVariable_, numberSets_);
          int numberColumns = getNumCols();
          backward_ = ClpCopyOfArray(rhs.backward_, numberColumns);
          backToPivotRow_ = ClpCopyOfArray(rhs.backToPivotRow_, numberColumns);
          changeCost_ = ClpCopyOfArray(rhs.changeCost_, getNumRows() + numberSets_);
          fromIndex_ = ClpCopyOfArray(rhs.fromIndex_, getNumRows() + numberSets_ + 1);
          keyVariable_ = ClpCopyOfArray(rhs.keyVariable_, numberSets_);
          // find longest set
          int * longest = new int[numberSets_];
          CoinZeroN(longest, numberSets_);
          int j;
          for (j = 0; j < numberColumns; j++) {
               int iSet = backward_[j];
               if (iSet >= 0)
                    longest[iSet]++;
          }
          int length = 0;
          for (j = 0; j < numberSets_; j++)
               length = CoinMax(length, longest[j]);
          next_ = ClpCopyOfArray(rhs.next_, numberColumns + numberSets_ + 2 * length);
          toIndex_ = ClpCopyOfArray(rhs.toIndex_, numberSets_);
          sumDualInfeasibilities_ = rhs. sumDualInfeasibilities_;
          sumPrimalInfeasibilities_ = rhs.sumPrimalInfeasibilities_;
          sumOfRelaxedDualInfeasibilities_ = rhs.sumOfRelaxedDualInfeasibilities_;
          sumOfRelaxedPrimalInfeasibilities_ = rhs.sumOfRelaxedPrimalInfeasibilities_;
          infeasibilityWeight_ = rhs.infeasibilityWeight_;
          numberDualInfeasibilities_ = rhs.numberDualInfeasibilities_;
          numberPrimalInfeasibilities_ = rhs.numberPrimalInfeasibilities_;
          noCheck_ = rhs.noCheck_;
          firstGub_ = rhs.firstGub_;
          lastGub_ = rhs.lastGub_;
          gubType_ = rhs.gubType_;
          model_ = rhs.model_;
     }
     return *this;
}
//-------------------------------------------------------------------
// Clone
//-------------------------------------------------------------------
ClpMatrixBase * ClpGubMatrix::clone() const
{
     return new ClpGubMatrix(*this);
}
/* Subset clone (without gaps).  Duplicates are allowed
   and order is as given */
ClpMatrixBase *
ClpGubMatrix::subsetClone (int numberRows, const int * whichRows,
                           int numberColumns,
                           const int * whichColumns) const
{
     return new ClpGubMatrix(*this, numberRows, whichRows,
                             numberColumns, whichColumns);
}
/* Returns a new matrix in reverse order without gaps
   Is allowed to return NULL if doesn't want to have row copy */
ClpMatrixBase *
ClpGubMatrix::reverseOrderedCopy() const
{
     return NULL;
}
int
ClpGubMatrix::hiddenRows() const
{
     return numberSets_;
}
/* Subset constructor (without gaps).  Duplicates are allowed
   and order is as given */
ClpGubMatrix::ClpGubMatrix (
     const ClpGubMatrix & rhs,
     int numberRows, const int * whichRows,
     int numberColumns, const int * whichColumns)
     : ClpPackedMatrix(rhs, numberRows, whichRows, numberColumns, whichColumns)
{
     // Assuming no gub rows deleted
     // We also assume all sets in same order
     // Get array with backward pointers
     int numberColumnsOld = rhs.matrix_->getNumCols();
     int * array = new int [ numberColumnsOld];
     int i;
     for (i = 0; i < numberColumnsOld; i++)
          array[i] = -1;
     for (int iSet = 0; iSet < numberSets_; iSet++) {
          for (int j = start_[iSet]; j < end_[iSet]; j++)
               array[j] = iSet;
     }
     numberSets_ = -1;
     int lastSet = -1;
     bool inSet = false;
     for (i = 0; i < numberColumns; i++) {
          int iColumn = whichColumns[i];
          int iSet = array[iColumn];
          if (iSet < 0) {
               inSet = false;
          } else {
               if (!inSet) {
                    // start of new set but check okay
                    if (iSet <= lastSet)
                         throw CoinError("overlapping or non-monotonic sets", "subset constructor", "ClpGubMatrix");
                    lastSet = iSet;
                    numberSets_++;
                    start_[numberSets_] = i;
                    end_[numberSets_] = i + 1;
                    lower_[numberSets_] = lower_[iSet];
                    upper_[numberSets_] = upper_[iSet];
                    inSet = true;
               } else {
                    if (iSet < lastSet) {
                         throw CoinError("overlapping or non-monotonic sets", "subset constructor", "ClpGubMatrix");
                    } else if (iSet == lastSet) {
                         end_[numberSets_] = i + 1;
                    } else {
                         // new set
                         lastSet = iSet;
                         numberSets_++;
                         start_[numberSets_] = i;
                         end_[numberSets_] = i + 1;
                         lower_[numberSets_] = lower_[iSet];
                         upper_[numberSets_] = upper_[iSet];
                    }
               }
          }
     }
     delete [] array;
     numberSets_++; // adjust
     // Find type of gub
     firstGub_ = numberColumns + 1;
     lastGub_ = -1;
     for (i = 0; i < numberColumns; i++) {
          if (backward_[i] >= 0) {
               firstGub_ = CoinMin(firstGub_, i);
               lastGub_ = CoinMax(lastGub_, i);
          }
     }
     if (lastGub_ > 0)
          lastGub_++;
     gubType_ = 0;
     for (i = firstGub_; i < lastGub_; i++) {
          if (backward_[i] < 0) {
               gubType_ = 1;
               break;
          }
     }

     // Make sure key is feasible if only key in set
}
ClpGubMatrix::ClpGubMatrix (
     const CoinPackedMatrix & rhs,
     int numberRows, const int * whichRows,
     int numberColumns, const int * whichColumns)
     : ClpPackedMatrix(rhs, numberRows, whichRows, numberColumns, whichColumns),
       sumDualInfeasibilities_(0.0),
       sumPrimalInfeasibilities_(0.0),
       sumOfRelaxedDualInfeasibilities_(0.0),
       sumOfRelaxedPrimalInfeasibilities_(0.0),
       start_(NULL),
       end_(NULL),
       lower_(NULL),
       upper_(NULL),
       backward_(NULL),
       backToPivotRow_(NULL),
       changeCost_(NULL),
       keyVariable_(NULL),
       next_(NULL),
       toIndex_(NULL),
       fromIndex_(NULL),
       numberDualInfeasibilities_(0),
       numberPrimalInfeasibilities_(0),
       numberSets_(0),
       saveNumber_(0),
       possiblePivotKey_(0),
       gubSlackIn_(-1),
       firstGub_(0),
       lastGub_(0),
       gubType_(0)
{
     setType(16);
}
/* Return <code>x * A + y</code> in <code>z</code>.
	Squashes small elements and knows about ClpSimplex */
void
ClpGubMatrix::transposeTimes(const ClpSimplex * model, double scalar,
                             const CoinIndexedVector * rowArray,
                             CoinIndexedVector * y,
                             CoinIndexedVector * columnArray) const
{
     columnArray->clear();
     double * pi = rowArray->denseVector();
     int numberNonZero = 0;
     int * index = columnArray->getIndices();
     double * array = columnArray->denseVector();
     int numberInRowArray = rowArray->getNumElements();
     // maybe I need one in OsiSimplex
     double zeroTolerance = model->zeroTolerance();
     int numberRows = model->numberRows();
     ClpPackedMatrix* rowCopy =
          dynamic_cast< ClpPackedMatrix*>(model->rowCopy());
     bool packed = rowArray->packedMode();
     double factor = 0.3;
     // We may not want to do by row if there may be cache problems
     int numberColumns = model->numberColumns();
     // It would be nice to find L2 cache size - for moment 512K
     // Be slightly optimistic
     if (numberColumns * sizeof(double) > 1000000) {
          if (numberRows * 10 < numberColumns)
               factor = 0.1;
          else if (numberRows * 4 < numberColumns)
               factor = 0.15;
          else if (numberRows * 2 < numberColumns)
               factor = 0.2;
          //if (model->numberIterations()%50==0)
          //printf("%d nonzero\n",numberInRowArray);
     }
     // reduce for gub
     factor *= 0.5;
     assert (!y->getNumElements());
     if (numberInRowArray > factor * numberRows || !rowCopy) {
          // do by column
          int iColumn;
          // get matrix data pointers
          const int * row = matrix_->getIndices();
          const CoinBigIndex * columnStart = matrix_->getVectorStarts();
          const int * columnLength = matrix_->getVectorLengths();
          const double * elementByColumn = matrix_->getElements();
          const double * rowScale = model->rowScale();
          int numberColumns = model->numberColumns();
          int iSet = -1;
          double djMod = 0.0;
          if (packed) {
               // need to expand pi into y
               assert(y->capacity() >= numberRows);
               double * piOld = pi;
               pi = y->denseVector();
               const int * whichRow = rowArray->getIndices();
               int i;
               if (!rowScale) {
                    // modify pi so can collapse to one loop
                    for (i = 0; i < numberInRowArray; i++) {
                         int iRow = whichRow[i];
                         pi[iRow] = scalar * piOld[i];
                    }
                    for (iColumn = 0; iColumn < numberColumns; iColumn++) {
                         if (backward_[iColumn] != iSet) {
                              // get pi on gub row
                              iSet = backward_[iColumn];
                              if (iSet >= 0) {
                                   int iBasic = keyVariable_[iSet];
                                   if (iBasic < numberColumns) {
                                        // get dj without
                                        assert (model->getStatus(iBasic) == ClpSimplex::basic);
                                        djMod = 0.0;
                                        for (CoinBigIndex j = columnStart[iBasic];
                                                  j < columnStart[iBasic] + columnLength[iBasic]; j++) {
                                             int jRow = row[j];
                                             djMod -= pi[jRow] * elementByColumn[j];
                                        }
                                   } else {
                                        djMod = 0.0;
                                   }
                              } else {
                                   djMod = 0.0;
                              }
                         }
                         double value = -djMod;
                         CoinBigIndex j;
                         for (j = columnStart[iColumn];
                                   j < columnStart[iColumn] + columnLength[iColumn]; j++) {
                              int iRow = row[j];
                              value += pi[iRow] * elementByColumn[j];
                         }
                         if (fabs(value) > zeroTolerance) {
                              array[numberNonZero] = value;
                              index[numberNonZero++] = iColumn;
                         }
                    }
               } else {
                    // scaled
                    // modify pi so can collapse to one loop
                    for (i = 0; i < numberInRowArray; i++) {
                         int iRow = whichRow[i];
                         pi[iRow] = scalar * piOld[i] * rowScale[iRow];
                    }
                    for (iColumn = 0; iColumn < numberColumns; iColumn++) {
                         if (backward_[iColumn] != iSet) {
                              // get pi on gub row
                              iSet = backward_[iColumn];
                              if (iSet >= 0) {
                                   int iBasic = keyVariable_[iSet];
                                   if (iBasic < numberColumns) {
                                        // get dj without
                                        assert (model->getStatus(iBasic) == ClpSimplex::basic);
                                        djMod = 0.0;
                                        // scaled
                                        for (CoinBigIndex j = columnStart[iBasic];
                                                  j < columnStart[iBasic] + columnLength[iBasic]; j++) {
                                             int jRow = row[j];
                                             djMod -= pi[jRow] * elementByColumn[j] * rowScale[jRow];
                                        }
                                   } else {
                                        djMod = 0.0;
                                   }
                              } else {
                                   djMod = 0.0;
                              }
                         }
                         double value = -djMod;
                         CoinBigIndex j;
                         const double * columnScale = model->columnScale();
                         for (j = columnStart[iColumn];
                                   j < columnStart[iColumn] + columnLength[iColumn]; j++) {
                              int iRow = row[j];
                              value += pi[iRow] * elementByColumn[j];
                         }
                         value *= columnScale[iColumn];
                         if (fabs(value) > zeroTolerance) {
                              array[numberNonZero] = value;
                              index[numberNonZero++] = iColumn;
                         }
                    }
               }
               // zero out
               for (i = 0; i < numberInRowArray; i++) {
                    int iRow = whichRow[i];
                    pi[iRow] = 0.0;
               }
          } else {
               // code later
               assert (packed);
               if (!rowScale) {
                    if (scalar == -1.0) {
                         for (iColumn = 0; iColumn < numberColumns; iColumn++) {
                              double value = 0.0;
                              CoinBigIndex j;
                              for (j = columnStart[iColumn];
                                        j < columnStart[iColumn] + columnLength[iColumn]; j++) {
                                   int iRow = row[j];
                                   value += pi[iRow] * elementByColumn[j];
                              }
                              if (fabs(value) > zeroTolerance) {
                                   index[numberNonZero++] = iColumn;
                                   array[iColumn] = -value;
                              }
                         }
                    } else if (scalar == 1.0) {
                         for (iColumn = 0; iColumn < numberColumns; iColumn++) {
                              double value = 0.0;
                              CoinBigIndex j;
                              for (j = columnStart[iColumn];
                                        j < columnStart[iColumn] + columnLength[iColumn]; j++) {
                                   int iRow = row[j];
                                   value += pi[iRow] * elementByColumn[j];
                              }
                              if (fabs(value) > zeroTolerance) {
                                   index[numberNonZero++] = iColumn;
                                   array[iColumn] = value;
                              }
                         }
                    } else {
                         for (iColumn = 0; iColumn < numberColumns; iColumn++) {
                              double value = 0.0;
                              CoinBigIndex j;
                              for (j = columnStart[iColumn];
                                        j < columnStart[iColumn] + columnLength[iColumn]; j++) {
                                   int iRow = row[j];
                                   value += pi[iRow] * elementByColumn[j];
                              }
                              value *= scalar;
                              if (fabs(value) > zeroTolerance) {
                                   index[numberNonZero++] = iColumn;
                                   array[iColumn] = value;
                              }
                         }
                    }
               } else {
                    // scaled
                    if (scalar == -1.0) {
                         for (iColumn = 0; iColumn < numberColumns; iColumn++) {
                              double value = 0.0;
                              CoinBigIndex j;
                              const double * columnScale = model->columnScale();
                              for (j = columnStart[iColumn];
                                        j < columnStart[iColumn] + columnLength[iColumn]; j++) {
                                   int iRow = row[j];
                                   value += pi[iRow] * elementByColumn[j] * rowScale[iRow];
                              }
                              value *= columnScale[iColumn];
                              if (fabs(value) > zeroTolerance) {
                                   index[numberNonZero++] = iColumn;
                                   array[iColumn] = -value;
                              }
                         }
                    } else if (scalar == 1.0) {
                         for (iColumn = 0; iColumn < numberColumns; iColumn++) {
                              double value = 0.0;
                              CoinBigIndex j;
                              const double * columnScale = model->columnScale();
                              for (j = columnStart[iColumn];
                                        j < columnStart[iColumn] + columnLength[iColumn]; j++) {
                                   int iRow = row[j];
                                   value += pi[iRow] * elementByColumn[j] * rowScale[iRow];
                              }
                              value *= columnScale[iColumn];
                              if (fabs(value) > zeroTolerance) {
                                   index[numberNonZero++] = iColumn;
                                   array[iColumn] = value;
                              }
                         }
                    } else {
                         for (iColumn = 0; iColumn < numberColumns; iColumn++) {
                              double value = 0.0;
                              CoinBigIndex j;
                              const double * columnScale = model->columnScale();
                              for (j = columnStart[iColumn];
                                        j < columnStart[iColumn] + columnLength[iColumn]; j++) {
                                   int iRow = row[j];
                                   value += pi[iRow] * elementByColumn[j] * rowScale[iRow];
                              }
                              value *= scalar * columnScale[iColumn];
                              if (fabs(value) > zeroTolerance) {
                                   index[numberNonZero++] = iColumn;
                                   array[iColumn] = value;
                              }
                         }
                    }
               }
          }
          columnArray->setNumElements(numberNonZero);
          y->setNumElements(0);
     } else {
          // do by row
          transposeTimesByRow(model, scalar, rowArray, y, columnArray);
     }
     if (packed)
          columnArray->setPackedMode(true);
     if (0) {
          columnArray->checkClean();
          int numberNonZero = columnArray->getNumElements();;
          int * index = columnArray->getIndices();
          double * array = columnArray->denseVector();
          int i;
          for (i = 0; i < numberNonZero; i++) {
               int j = index[i];
               double value;
               if (packed)
                    value = array[i];
               else
                    value = array[j];
               printf("Ti %d %d %g\n", i, j, value);
          }
     }
}
/* Return <code>x * A + y</code> in <code>z</code>.
	Squashes small elements and knows about ClpSimplex */
void
ClpGubMatrix::transposeTimesByRow(const ClpSimplex * model, double scalar,
                                  const CoinIndexedVector * rowArray,
                                  CoinIndexedVector * y,
                                  CoinIndexedVector * columnArray) const
{
     // Do packed part
     ClpPackedMatrix::transposeTimesByRow(model, scalar, rowArray, y, columnArray);
     if (numberSets_) {
          /* what we need to do is do by row as normal but get list of sets touched
             and then update those ones */
          abort();
     }
}
/* Return <code>x *A in <code>z</code> but
   just for indices in y. */
void
ClpGubMatrix::subsetTransposeTimes(const ClpSimplex * model,
                                   const CoinIndexedVector * rowArray,
                                   const CoinIndexedVector * y,
                                   CoinIndexedVector * columnArray) const
{
     columnArray->clear();
     double * pi = rowArray->denseVector();
     double * array = columnArray->denseVector();
     int jColumn;
     // get matrix data pointers
     const int * row = matrix_->getIndices();
     const CoinBigIndex * columnStart = matrix_->getVectorStarts();
     const int * columnLength = matrix_->getVectorLengths();
     const double * elementByColumn = matrix_->getElements();
     const double * rowScale = model->rowScale();
     int numberToDo = y->getNumElements();
     const int * which = y->getIndices();
     assert (!rowArray->packedMode());
     columnArray->setPacked();
     int numberTouched = 0;
     if (!rowScale) {
          for (jColumn = 0; jColumn < numberToDo; jColumn++) {
               int iColumn = which[jColumn];
               double value = 0.0;
               CoinBigIndex j;
               for (j = columnStart[iColumn];
                         j < columnStart[iColumn] + columnLength[iColumn]; j++) {
                    int iRow = row[j];
                    value += pi[iRow] * elementByColumn[j];
               }
               array[jColumn] = value;
               if (value) {
                    int iSet = backward_[iColumn];
                    if (iSet >= 0) {
                         int iBasic = keyVariable_[iSet];
                         if (iBasic == iColumn) {
                              toIndex_[iSet] = jColumn;
                              fromIndex_[numberTouched++] = iSet;
                         }
                    }
               }
          }
     } else {
          // scaled
          for (jColumn = 0; jColumn < numberToDo; jColumn++) {
               int iColumn = which[jColumn];
               double value = 0.0;
               CoinBigIndex j;
               const double * columnScale = model->columnScale();
               for (j = columnStart[iColumn];
                         j < columnStart[iColumn] + columnLength[iColumn]; j++) {
                    int iRow = row[j];
                    value += pi[iRow] * elementByColumn[j] * rowScale[iRow];
               }
               value *= columnScale[iColumn];
               array[jColumn] = value;
               if (value) {
                    int iSet = backward_[iColumn];
                    if (iSet >= 0) {
                         int iBasic = keyVariable_[iSet];
                         if (iBasic == iColumn) {
                              toIndex_[iSet] = jColumn;
                              fromIndex_[numberTouched++] = iSet;
                         }
                    }
               }
          }
     }
     // adjust djs
     for (jColumn = 0; jColumn < numberToDo; jColumn++) {
          int iColumn = which[jColumn];
          int iSet = backward_[iColumn];
          if (iSet >= 0) {
               int kColumn = toIndex_[iSet];
               if (kColumn >= 0)
                    array[jColumn] -= array[kColumn];
          }
     }
     // and clear basic
     for (int j = 0; j < numberTouched; j++) {
          int iSet = fromIndex_[j];
          int kColumn = toIndex_[iSet];
          toIndex_[iSet] = -1;
          array[kColumn] = 0.0;
     }
}
/// returns number of elements in column part of basis,
CoinBigIndex
ClpGubMatrix::countBasis(const int * whichColumn,
                         int & numberColumnBasic)
{
     int i;
     int numberColumns = getNumCols();
     const int * columnLength = matrix_->getVectorLengths();
     int numberRows = getNumRows();
     int numberBasic = 0;
     CoinBigIndex numberElements = 0;
     int lastSet = -1;
     int key = -1;
     int keyLength = -1;
     double * work = new double[numberRows];
     CoinZeroN(work, numberRows);
     char * mark = new char[numberRows];
     CoinZeroN(mark, numberRows);
     const CoinBigIndex * columnStart = matrix_->getVectorStarts();
     const int * row = matrix_->getIndices();
     const double * elementByColumn = matrix_->getElements();
     //ClpGubDynamicMatrix* gubx =
     //dynamic_cast< ClpGubDynamicMatrix*>(this);
     //int * id = gubx->id();
     // just count
     for (i = 0; i < numberColumnBasic; i++) {
          int iColumn = whichColumn[i];
          int iSet = backward_[iColumn];
          int length = columnLength[iColumn];
          if (iSet < 0 || keyVariable_[iSet] >= numberColumns) {
               numberElements += length;
               numberBasic++;
               //printf("non gub - set %d id %d (column %d) nel %d\n",iSet,id[iColumn-20],iColumn,length);
          } else {
               // in gub set
               if (iColumn != keyVariable_[iSet]) {
                    numberBasic++;
                    CoinBigIndex j;
                    // not key
                    if (lastSet < iSet) {
                         // erase work
                         if (key >= 0) {
                              for (j = columnStart[key]; j < columnStart[key] + keyLength; j++)
                                   work[row[j]] = 0.0;
                         }
                         key = keyVariable_[iSet];
                         lastSet = iSet;
                         keyLength = columnLength[key];
                         for (j = columnStart[key]; j < columnStart[key] + keyLength; j++)
                              work[row[j]] = elementByColumn[j];
                    }
                    int extra = keyLength;
                    for (j = columnStart[iColumn]; j < columnStart[iColumn] + length; j++) {
                         int iRow = row[j];
                         double keyValue = work[iRow];
                         double value = elementByColumn[j];
                         if (!keyValue) {
                              if (fabs(value) > 1.0e-20)
                                   extra++;
                         } else {
                              value -= keyValue;
                              if (fabs(value) <= 1.0e-20)
                                   extra--;
                         }
                    }
                    numberElements += extra;
                    //printf("gub - set %d id %d (column %d) nel %d\n",iSet,id[iColumn-20],iColumn,extra);
               }
          }
     }
     delete [] work;
     delete [] mark;
     // update number of column basic
     numberColumnBasic = numberBasic;
     return numberElements;
}
void
ClpGubMatrix::fillBasis(ClpSimplex * model,
                        const int * whichColumn,
                        int & numberColumnBasic,
                        int * indexRowU, int * start,
                        int * rowCount, int * columnCount,
                        CoinFactorizationDouble * elementU)
{
     int i;
     int numberColumns = getNumCols();
     const int * columnLength = matrix_->getVectorLengths();
     int numberRows = getNumRows();
     assert (next_ || !elementU) ;
     CoinBigIndex numberElements = start[0];
     int lastSet = -1;
     int key = -1;
     int keyLength = -1;
     double * work = new double[numberRows];
     CoinZeroN(work, numberRows);
     char * mark = new char[numberRows];
     CoinZeroN(mark, numberRows);
     const CoinBigIndex * columnStart = matrix_->getVectorStarts();
     const int * row = matrix_->getIndices();
     const double * elementByColumn = matrix_->getElements();
     const double * rowScale = model->rowScale();
     int numberBasic = 0;
     if (0) {
          printf("%d basiccolumns\n", numberColumnBasic);
          int i;
          for (i = 0; i < numberSets_; i++) {
               int k = keyVariable_[i];
               if (k < numberColumns) {
                    printf("key %d on set %d, %d elements\n", k, i, columnStart[k+1] - columnStart[k]);
                    for (int j = columnStart[k]; j < columnStart[k+1]; j++)
                         printf("row %d el %g\n", row[j], elementByColumn[j]);
               } else {
                    printf("slack key on set %d\n", i);
               }
          }
     }
     // fill
     if (!rowScale) {
          // no scaling
          for (i = 0; i < numberColumnBasic; i++) {
               int iColumn = whichColumn[i];
               int iSet = backward_[iColumn];
               int length = columnLength[iColumn];
               if (0) {
                    int k = iColumn;
                    printf("column %d in set %d, %d elements\n", k, iSet, columnStart[k+1] - columnStart[k]);
                    for (int j = columnStart[k]; j < columnStart[k+1]; j++)
                         printf("row %d el %g\n", row[j], elementByColumn[j]);
               }
               CoinBigIndex j;
               if (iSet < 0 || keyVariable_[iSet] >= numberColumns) {
                    for (j = columnStart[iColumn]; j < columnStart[iColumn] + columnLength[iColumn]; j++) {
                         double value = elementByColumn[j];
                         if (fabs(value) > 1.0e-20) {
                              int iRow = row[j];
                              indexRowU[numberElements] = iRow;
                              rowCount[iRow]++;
                              elementU[numberElements++] = value;
                         }
                    }
                    // end of column
                    columnCount[numberBasic] = numberElements - start[numberBasic];
                    numberBasic++;
                    start[numberBasic] = numberElements;
               } else {
                    // in gub set
                    if (iColumn != keyVariable_[iSet]) {
                         // not key
                         if (lastSet != iSet) {
                              // erase work
                              if (key >= 0) {
                                   for (j = columnStart[key]; j < columnStart[key] + keyLength; j++) {
                                        int iRow = row[j];
                                        work[iRow] = 0.0;
                                        mark[iRow] = 0;
                                   }
                              }
                              key = keyVariable_[iSet];
                              lastSet = iSet;
                              keyLength = columnLength[key];
                              for (j = columnStart[key]; j < columnStart[key] + keyLength; j++) {
                                   int iRow = row[j];
                                   work[iRow] = elementByColumn[j];
                                   mark[iRow] = 1;
                              }
                         }
                         for (j = columnStart[iColumn]; j < columnStart[iColumn] + length; j++) {
                              int iRow = row[j];
                              double value = elementByColumn[j];
                              if (mark[iRow]) {
                                   mark[iRow] = 0;
                                   double keyValue = work[iRow];
                                   value -= keyValue;
                              }
                              if (fabs(value) > 1.0e-20) {
                                   indexRowU[numberElements] = iRow;
                                   rowCount[iRow]++;
                                   elementU[numberElements++] = value;
                              }
                         }
                         for (j = columnStart[key]; j < columnStart[key] + keyLength; j++) {
                              int iRow = row[j];
                              if (mark[iRow]) {
                                   double value = -work[iRow];
                                   if (fabs(value) > 1.0e-20) {
                                        indexRowU[numberElements] = iRow;
                                        rowCount[iRow]++;
                                        elementU[numberElements++] = value;
                                   }
                              } else {
                                   // just put back mark
                                   mark[iRow] = 1;
                              }
                         }
                         // end of column
                         columnCount[numberBasic] = numberElements - start[numberBasic];
                         numberBasic++;
                         start[numberBasic] = numberElements;
                    }
               }
          }
     } else {
          // scaling
          const double * columnScale = model->columnScale();
          for (i = 0; i < numberColumnBasic; i++) {
               int iColumn = whichColumn[i];
               int iSet = backward_[iColumn];
               int length = columnLength[iColumn];
               CoinBigIndex j;
               if (iSet < 0 || keyVariable_[iSet] >= numberColumns) {
                    double scale = columnScale[iColumn];
                    for (j = columnStart[iColumn]; j < columnStart[iColumn] + columnLength[iColumn]; j++) {
                         int iRow = row[j];
                         double value = elementByColumn[j] * scale * rowScale[iRow];
                         if (fabs(value) > 1.0e-20) {
                              indexRowU[numberElements] = iRow;
                              rowCount[iRow]++;
                              elementU[numberElements++] = value;
                         }
                    }
                    // end of column
                    columnCount[numberBasic] = numberElements - start[numberBasic];
                    numberBasic++;
                    start[numberBasic] = numberElements;
               } else {
                    // in gub set
                    if (iColumn != keyVariable_[iSet]) {
                         double scale = columnScale[iColumn];
                         // not key
                         if (lastSet < iSet) {
                              // erase work
                              if (key >= 0) {
                                   for (j = columnStart[key]; j < columnStart[key] + keyLength; j++) {
                                        int iRow = row[j];
                                        work[iRow] = 0.0;
                                        mark[iRow] = 0;
                                   }
                              }
                              key = keyVariable_[iSet];
                              lastSet = iSet;
                              keyLength = columnLength[key];
                              double scale = columnScale[key];
                              for (j = columnStart[key]; j < columnStart[key] + keyLength; j++) {
                                   int iRow = row[j];
                                   work[iRow] = elementByColumn[j] * scale * rowScale[iRow];
                                   mark[iRow] = 1;
                              }
                         }
                         for (j = columnStart[iColumn]; j < columnStart[iColumn] + length; j++) {
                              int iRow = row[j];
                              double value = elementByColumn[j] * scale * rowScale[iRow];
                              if (mark[iRow]) {
                                   mark[iRow] = 0;
                                   double keyValue = work[iRow];
                                   value -= keyValue;
                              }
                              if (fabs(value) > 1.0e-20) {
                                   indexRowU[numberElements] = iRow;
                                   rowCount[iRow]++;
                                   elementU[numberElements++] = value;
                              }
                         }
                         for (j = columnStart[key]; j < columnStart[key] + keyLength; j++) {
                              int iRow = row[j];
                              if (mark[iRow]) {
                                   double value = -work[iRow];
                                   if (fabs(value) > 1.0e-20) {
                                        indexRowU[numberElements] = iRow;
                                        rowCount[iRow]++;
                                        elementU[numberElements++] = value;
                                   }
                              } else {
                                   // just put back mark
                                   mark[iRow] = 1;
                              }
                         }
                         // end of column
                         columnCount[numberBasic] = numberElements - start[numberBasic];
                         numberBasic++;
                         start[numberBasic] = numberElements;
                    }
               }
          }
     }
     delete [] work;
     delete [] mark;
     // update number of column basic
     numberColumnBasic = numberBasic;
}
/* Unpacks a column into an CoinIndexedvector
 */
void
ClpGubMatrix::unpack(const ClpSimplex * model, CoinIndexedVector * rowArray,
                     int iColumn) const
{
     assert (iColumn < model->numberColumns());
     // Do packed part
     ClpPackedMatrix::unpack(model, rowArray, iColumn);
     int iSet = backward_[iColumn];
     if (iSet >= 0) {
          int iBasic = keyVariable_[iSet];
          if (iBasic < model->numberColumns()) {
               add(model, rowArray, iBasic, -1.0);
          }
     }
}
/* Unpacks a column into a CoinIndexedVector
** in packed format
Note that model is NOT const.  Bounds and objective could
be modified if doing column generation (just for this variable) */
void
ClpGubMatrix::unpackPacked(ClpSimplex * model,
                           CoinIndexedVector * rowArray,
                           int iColumn) const
{
     int numberColumns = model->numberColumns();
     if (iColumn < numberColumns) {
          // Do packed part
          ClpPackedMatrix::unpackPacked(model, rowArray, iColumn);
          int iSet = backward_[iColumn];
          if (iSet >= 0) {
               // columns are in order
               int iBasic = keyVariable_[iSet];
               if (iBasic < numberColumns) {
                    int number = rowArray->getNumElements();
                    const double * rowScale = model->rowScale();
                    const int * row = matrix_->getIndices();
                    const CoinBigIndex * columnStart = matrix_->getVectorStarts();
                    const int * columnLength = matrix_->getVectorLengths();
                    const double * elementByColumn = matrix_->getElements();
                    double * array = rowArray->denseVector();
                    int * index = rowArray->getIndices();
                    CoinBigIndex i;
                    int numberOld = number;
                    int lastIndex = 0;
                    int next = index[lastIndex];
                    if (!rowScale) {
                         for (i = columnStart[iBasic];
                                   i < columnStart[iBasic] + columnLength[iBasic]; i++) {
                              int iRow = row[i];
                              while (iRow > next) {
                                   lastIndex++;
                                   if (lastIndex == numberOld)
                                        next = matrix_->getNumRows();
                                   else
                                        next = index[lastIndex];
                              }
                              if (iRow < next) {
                                   array[number] = -elementByColumn[i];
                                   index[number++] = iRow;
                              } else {
                                   assert (iRow == next);
                                   array[lastIndex] -= elementByColumn[i];
                                   if (!array[lastIndex])
                                        array[lastIndex] = 1.0e-100;
                              }
                         }
                    } else {
                         // apply scaling
                         double scale = model->columnScale()[iBasic];
                         for (i = columnStart[iBasic];
                                   i < columnStart[iBasic] + columnLength[iBasic]; i++) {
                              int iRow = row[i];
                              while (iRow > next) {
                                   lastIndex++;
                                   if (lastIndex == numberOld)
                                        next = matrix_->getNumRows();
                                   else
                                        next = index[lastIndex];
                              }
                              if (iRow < next) {
                                   array[number] = -elementByColumn[i] * scale * rowScale[iRow];
                                   index[number++] = iRow;
                              } else {
                                   assert (iRow == next);
                                   array[lastIndex] -= elementByColumn[i] * scale * rowScale[iRow];
                                   if (!array[lastIndex])
                                        array[lastIndex] = 1.0e-100;
                              }
                         }
                    }
                    rowArray->setNumElements(number);
               }
          }
     } else {
          // key slack entering
          int iBasic = keyVariable_[gubSlackIn_];
          assert (iBasic < numberColumns);
          int number = 0;
          const double * rowScale = model->rowScale();
          const int * row = matrix_->getIndices();
          const CoinBigIndex * columnStart = matrix_->getVectorStarts();
          const int * columnLength = matrix_->getVectorLengths();
          const double * elementByColumn = matrix_->getElements();
          double * array = rowArray->denseVector();
          int * index = rowArray->getIndices();
          CoinBigIndex i;
          if (!rowScale) {
               for (i = columnStart[iBasic];
                         i < columnStart[iBasic] + columnLength[iBasic]; i++) {
                    int iRow = row[i];
                    array[number] = elementByColumn[i];
                    index[number++] = iRow;
               }
          } else {
               // apply scaling
               double scale = model->columnScale()[iBasic];
               for (i = columnStart[iBasic];
                         i < columnStart[iBasic] + columnLength[iBasic]; i++) {
                    int iRow = row[i];
                    array[number] = elementByColumn[i] * scale * rowScale[iRow];
                    index[number++] = iRow;
               }
          }
          rowArray->setNumElements(number);
          rowArray->setPacked();
     }
}
/* Adds multiple of a column into an CoinIndexedvector
      You can use quickAdd to add to vector */
void
ClpGubMatrix::add(const ClpSimplex * model, CoinIndexedVector * rowArray,
                  int iColumn, double multiplier) const
{
     assert (iColumn < model->numberColumns());
     // Do packed part
     ClpPackedMatrix::add(model, rowArray, iColumn, multiplier);
     int iSet = backward_[iColumn];
     if (iSet >= 0 && iColumn != keyVariable_[iSet]) {
          ClpPackedMatrix::add(model, rowArray, keyVariable_[iSet], -multiplier);
     }
}
/* Adds multiple of a column into an array */
void
ClpGubMatrix::add(const ClpSimplex * model, double * array,
                  int iColumn, double multiplier) const
{
     assert (iColumn < model->numberColumns());
     // Do packed part
     ClpPackedMatrix::add(model, array, iColumn, multiplier);
     if (iColumn < model->numberColumns()) {
          int iSet = backward_[iColumn];
          if (iSet >= 0 && iColumn != keyVariable_[iSet] && keyVariable_[iSet] < model->numberColumns()) {
               ClpPackedMatrix::add(model, array, keyVariable_[iSet], -multiplier);
          }
     }
}
// Partial pricing
void
ClpGubMatrix::partialPricing(ClpSimplex * model, double startFraction, double endFraction,
                             int & bestSequence, int & numberWanted)
{
     numberWanted = currentWanted_;
     if (numberSets_) {
          // Do packed part before gub
          int numberColumns = matrix_->getNumCols();
          double ratio = static_cast<double> (firstGub_) /
                         static_cast<double> (numberColumns);
          ClpPackedMatrix::partialPricing(model, startFraction * ratio,
                                          endFraction * ratio, bestSequence, numberWanted);
          if (numberWanted || minimumGoodReducedCosts_ < -1) {
               // do gub
               const double * element = matrix_->getElements();
               const int * row = matrix_->getIndices();
               const CoinBigIndex * startColumn = matrix_->getVectorStarts();
               const int * length = matrix_->getVectorLengths();
               const double * rowScale = model->rowScale();
               const double * columnScale = model->columnScale();
               int iSequence;
               CoinBigIndex j;
               double tolerance = model->currentDualTolerance();
               double * reducedCost = model->djRegion();
               const double * duals = model->dualRowSolution();
               const double * cost = model->costRegion();
               double bestDj;
               int numberColumns = model->numberColumns();
               int numberRows = model->numberRows();
               if (bestSequence >= 0)
                    bestDj = fabs(this->reducedCost(model, bestSequence));
               else
                    bestDj = tolerance;
               int sequenceOut = model->sequenceOut();
               int saveSequence = bestSequence;
               int startG = firstGub_ + static_cast<int> (startFraction * (lastGub_ - firstGub_));
               int endG = firstGub_ + static_cast<int> (endFraction * (lastGub_ - firstGub_));
               endG = CoinMin(lastGub_, endG + 1);
               // If nothing found yet can go all the way to end
               int endAll = endG;
               if (bestSequence < 0 && !startG)
                    endAll = lastGub_;
               int minSet = minimumObjectsScan_ < 0 ? 5 : minimumObjectsScan_;
               int minNeg = minimumGoodReducedCosts_ == -1 ? 5 : minimumGoodReducedCosts_;
               int nSets = 0;
               int iSet = -1;
               double djMod = 0.0;
               double infeasibilityCost = model->infeasibilityCost();
               if (rowScale) {
                    double bestDjMod = 0.0;
                    // scaled
                    for (iSequence = startG; iSequence < endAll; iSequence++) {
                         if (numberWanted + minNeg < originalWanted_ && nSets > minSet) {
                              // give up
                              numberWanted = 0;
                              break;
                         } else if (iSequence == endG && bestSequence >= 0) {
                              break;
                         }
                         if (backward_[iSequence] != iSet) {
                              // get pi on gub row
                              iSet = backward_[iSequence];
                              if (iSet >= 0) {
                                   nSets++;
                                   int iBasic = keyVariable_[iSet];
                                   if (iBasic >= numberColumns) {
                                        djMod = - weight(iSet) * infeasibilityCost;
                                   } else {
                                        // get dj without
                                        assert (model->getStatus(iBasic) == ClpSimplex::basic);
                                        djMod = 0.0;
                                        // scaled
                                        for (j = startColumn[iBasic];
                                                  j < startColumn[iBasic] + length[iBasic]; j++) {
                                             int jRow = row[j];
                                             djMod -= duals[jRow] * element[j] * rowScale[jRow];
                                        }
                                        // allow for scaling
                                        djMod +=  cost[iBasic] / columnScale[iBasic];
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
                                                       } else {
                                                            // just to make sure we don't exit before got something
                                                            numberWanted++;
                                                            abort();
                                                       }
                                                  }
                                             }
                                        }
                                   }
                              } else {
                                   // not in set
                                   djMod = 0.0;
                              }
                         }
                         if (iSequence != sequenceOut) {
                              double value;
                              ClpSimplex::Status status = model->getStatus(iSequence);

                              switch(status) {

                              case ClpSimplex::basic:
                              case ClpSimplex::isFixed:
                                   break;
                              case ClpSimplex::isFree:
                              case ClpSimplex::superBasic:
                                   value = -djMod;
                                   // scaled
                                   for (j = startColumn[iSequence];
                                             j < startColumn[iSequence] + length[iSequence]; j++) {
                                        int jRow = row[j];
                                        value -= duals[jRow] * element[j] * rowScale[jRow];
                                   }
                                   value = fabs(cost[iSequence] + value * columnScale[iSequence]);
                                   if (value > FREE_ACCEPT * tolerance) {
                                        numberWanted--;
                                        // we are going to bias towards free (but only if reasonable)
                                        value *= FREE_BIAS;
                                        if (value > bestDj) {
                                             // check flagged variable and correct dj
                                             if (!model->flagged(iSequence)) {
                                                  bestDj = value;
                                                  bestSequence = iSequence;
                                                  bestDjMod = djMod;
                                             } else {
                                                  // just to make sure we don't exit before got something
                                                  numberWanted++;
                                             }
                                        }
                                   }
                                   break;
                              case ClpSimplex::atUpperBound:
                                   value = -djMod;
                                   // scaled
                                   for (j = startColumn[iSequence];
                                             j < startColumn[iSequence] + length[iSequence]; j++) {
                                        int jRow = row[j];
                                        value -= duals[jRow] * element[j] * rowScale[jRow];
                                   }
                                   value = cost[iSequence] + value * columnScale[iSequence];
                                   if (value > tolerance) {
                                        numberWanted--;
                                        if (value > bestDj) {
                                             // check flagged variable and correct dj
                                             if (!model->flagged(iSequence)) {
                                                  bestDj = value;
                                                  bestSequence = iSequence;
                                                  bestDjMod = djMod;
                                             } else {
                                                  // just to make sure we don't exit before got something
                                                  numberWanted++;
                                             }
                                        }
                                   }
                                   break;
                              case ClpSimplex::atLowerBound:
                                   value = -djMod;
                                   // scaled
                                   for (j = startColumn[iSequence];
                                             j < startColumn[iSequence] + length[iSequence]; j++) {
                                        int jRow = row[j];
                                        value -= duals[jRow] * element[j] * rowScale[jRow];
                                   }
                                   value = -(cost[iSequence] + value * columnScale[iSequence]);
                                   if (value > tolerance) {
                                        numberWanted--;
                                        if (value > bestDj) {
                                             // check flagged variable and correct dj
                                             if (!model->flagged(iSequence)) {
                                                  bestDj = value;
                                                  bestSequence = iSequence;
                                                  bestDjMod = djMod;
                                             } else {
                                                  // just to make sure we don't exit before got something
                                                  numberWanted++;
                                             }
                                        }
                                   }
                                   break;
                              }
                         }
                         if (!numberWanted)
                              break;
                    }
                    if (bestSequence != saveSequence) {
                         if (bestSequence < numberRows + numberColumns) {
                              // recompute dj
                              double value = bestDjMod;
                              // scaled
                              for (j = startColumn[bestSequence];
                                        j < startColumn[bestSequence] + length[bestSequence]; j++) {
                                   int jRow = row[j];
                                   value -= duals[jRow] * element[j] * rowScale[jRow];
                              }
                              reducedCost[bestSequence] = cost[bestSequence] + value * columnScale[bestSequence];
                              gubSlackIn_ = -1;
                         } else {
                              // slack - make last column
                              gubSlackIn_ = bestSequence - numberRows - numberColumns;
                              bestSequence = numberColumns + 2 * numberRows;
                              reducedCost[bestSequence] = bestDjMod;
                              model->setStatus(bestSequence, getStatus(gubSlackIn_));
                              if (getStatus(gubSlackIn_) == ClpSimplex::atUpperBound)
                                   model->solutionRegion()[bestSequence] = upper_[gubSlackIn_];
                              else
                                   model->solutionRegion()[bestSequence] = lower_[gubSlackIn_];
                              model->lowerRegion()[bestSequence] = lower_[gubSlackIn_];
                              model->upperRegion()[bestSequence] = upper_[gubSlackIn_];
                              model->costRegion()[bestSequence] = 0.0;
                         }
                         savedBestSequence_ = bestSequence;
                         savedBestDj_ = reducedCost[savedBestSequence_];
                    }
               } else {
                    double bestDjMod = 0.0;
                    //printf("iteration %d start %d end %d - wanted %d\n",model->numberIterations(),
                    //     startG,endG,numberWanted);
                    for (iSequence = startG; iSequence < endG; iSequence++) {
                         if (numberWanted + minNeg < originalWanted_ && nSets > minSet) {
                              // give up
                              numberWanted = 0;
                              break;
                         } else if (iSequence == endG && bestSequence >= 0) {
                              break;
                         }
                         if (backward_[iSequence] != iSet) {
                              // get pi on gub row
                              iSet = backward_[iSequence];
                              if (iSet >= 0) {
                                   nSets++;
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
                                                       } else {
                                                            // just to make sure we don't exit before got something
                                                            numberWanted++;
                                                            abort();
                                                       }
                                                  }
                                             }
                                        }
                                   }
                              } else {
                                   // not in set
                                   djMod = 0.0;
                              }
                         }
                         if (iSequence != sequenceOut) {
                              double value;
                              ClpSimplex::Status status = model->getStatus(iSequence);

                              switch(status) {

                              case ClpSimplex::basic:
                              case ClpSimplex::isFixed:
                                   break;
                              case ClpSimplex::isFree:
                              case ClpSimplex::superBasic:
                                   value = cost[iSequence] - djMod;
                                   for (j = startColumn[iSequence];
                                             j < startColumn[iSequence] + length[iSequence]; j++) {
                                        int jRow = row[j];
                                        value -= duals[jRow] * element[j];
                                   }
                                   value = fabs(value);
                                   if (value > FREE_ACCEPT * tolerance) {
                                        numberWanted--;
                                        // we are going to bias towards free (but only if reasonable)
                                        value *= FREE_BIAS;
                                        if (value > bestDj) {
                                             // check flagged variable and correct dj
                                             if (!model->flagged(iSequence)) {
                                                  bestDj = value;
                                                  bestSequence = iSequence;
                                                  bestDjMod = djMod;
                                             } else {
                                                  // just to make sure we don't exit before got something
                                                  numberWanted++;
                                             }
                                        }
                                   }
                                   break;
                              case ClpSimplex::atUpperBound:
                                   value = cost[iSequence] - djMod;
                                   for (j = startColumn[iSequence];
                                             j < startColumn[iSequence] + length[iSequence]; j++) {
                                        int jRow = row[j];
                                        value -= duals[jRow] * element[j];
                                   }
                                   if (value > tolerance) {
                                        numberWanted--;
                                        if (value > bestDj) {
                                             // check flagged variable and correct dj
                                             if (!model->flagged(iSequence)) {
                                                  bestDj = value;
                                                  bestSequence = iSequence;
                                                  bestDjMod = djMod;
                                             } else {
                                                  // just to make sure we don't exit before got something
                                                  numberWanted++;
                                             }
                                        }
                                   }
                                   break;
                              case ClpSimplex::atLowerBound:
                                   value = cost[iSequence] - djMod;
                                   for (j = startColumn[iSequence];
                                             j < startColumn[iSequence] + length[iSequence]; j++) {
                                        int jRow = row[j];
                                        value -= duals[jRow] * element[j];
                                   }
                                   value = -value;
                                   if (value > tolerance) {
                                        numberWanted--;
                                        if (value > bestDj) {
                                             // check flagged variable and correct dj
                                             if (!model->flagged(iSequence)) {
                                                  bestDj = value;
                                                  bestSequence = iSequence;
                                                  bestDjMod = djMod;
                                             } else {
                                                  // just to make sure we don't exit before got something
                                                  numberWanted++;
                                             }
                                        }
                                   }
                                   break;
                              }
                         }
                         if (!numberWanted)
                              break;
                    }
                    if (bestSequence != saveSequence) {
                         if (bestSequence < numberRows + numberColumns) {
                              // recompute dj
                              double value = cost[bestSequence] - bestDjMod;
                              for (j = startColumn[bestSequence];
                                        j < startColumn[bestSequence] + length[bestSequence]; j++) {
                                   int jRow = row[j];
                                   value -= duals[jRow] * element[j];
                              }
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
                                   model->solutionRegion()[bestSequence] = upper_[gubSlackIn_];
                              else
                                   model->solutionRegion()[bestSequence] = lower_[gubSlackIn_];
                              model->lowerRegion()[bestSequence] = lower_[gubSlackIn_];
                              model->upperRegion()[bestSequence] = upper_[gubSlackIn_];
                              model->costRegion()[bestSequence] = 0.0;
                         }
                    }
               }
               // See if may be finished
               if (startG == firstGub_ && bestSequence < 0)
                    infeasibilityWeight_ = model_->infeasibilityCost();
               else if (bestSequence >= 0)
                    infeasibilityWeight_ = -1.0;
          }
          if (numberWanted) {
               // Do packed part after gub
               double offset = static_cast<double> (lastGub_) /
                               static_cast<double> (numberColumns);
               double ratio = static_cast<double> (numberColumns) /
                              static_cast<double> (numberColumns) - offset;
               double start2 = offset + ratio * startFraction;
               double end2 = CoinMin(1.0, offset + ratio * endFraction + 1.0e-6);
               ClpPackedMatrix::partialPricing(model, start2, end2, bestSequence, numberWanted);
          }
     } else {
          // no gub
          ClpPackedMatrix::partialPricing(model, startFraction, endFraction, bestSequence, numberWanted);
     }
     if (bestSequence >= 0)
          infeasibilityWeight_ = -1.0; // not optimal
     currentWanted_ = numberWanted;
}
/* expands an updated column to allow for extra rows which the main
   solver does not know about and returns number added.
*/
int
ClpGubMatrix::extendUpdated(ClpSimplex * model, CoinIndexedVector * update, int mode)
{
     // I think we only need to bother about sets with two in basis or incoming set
     int number = update->getNumElements();
     double * array = update->denseVector();
     int * index = update->getIndices();
     int i;
     assert (!number || update->packedMode());
     int * pivotVariable = model->pivotVariable();
     int numberRows = model->numberRows();
     int numberColumns = model->numberColumns();
     int numberTotal = numberRows + numberColumns;
     int sequenceIn = model->sequenceIn();
     int returnCode = 0;
     int iSetIn;
     if (sequenceIn < numberColumns) {
          iSetIn = backward_[sequenceIn];
          gubSlackIn_ = -1; // in case set
     } else if (sequenceIn < numberRows + numberColumns) {
          iSetIn = -1;
          gubSlackIn_ = -1; // in case set
     } else {
          iSetIn = gubSlackIn_;
     }
     double * lower = model->lowerRegion();
     double * upper = model->upperRegion();
     double * cost = model->costRegion();
     double * solution = model->solutionRegion();
     int number2 = number;
     if (!mode) {
          double primalTolerance = model->primalTolerance();
          double infeasibilityCost = model->infeasibilityCost();
          // extend
          saveNumber_ = number;
          for (i = 0; i < number; i++) {
               int iRow = index[i];
               int iPivot = pivotVariable[iRow];
               if (iPivot < numberColumns) {
                    int iSet = backward_[iPivot];
                    if (iSet >= 0) {
                         // two (or more) in set
                         int iIndex = toIndex_[iSet];
                         double otherValue = array[i];
                         double value;
                         if (iIndex < 0) {
                              toIndex_[iSet] = number2;
                              int iNew = number2 - number;
                              fromIndex_[number2-number] = iSet;
                              iIndex = number2;
                              index[number2] = numberRows + iNew;
                              // do key stuff
                              int iKey = keyVariable_[iSet];
                              if (iKey < numberColumns) {
                                   // Save current cost of key
                                   changeCost_[number2-number] = cost[iKey];
                                   if (iSet != iSetIn)
                                        value = 0.0;
                                   else if (iSetIn != gubSlackIn_)
                                        value = 1.0;
                                   else
                                        value = -1.0;
                                   pivotVariable[numberRows+iNew] = iKey;
                                   // Do I need to recompute?
                                   double sol;
                                   assert (getStatus(iSet) != ClpSimplex::basic);
                                   if (getStatus(iSet) == ClpSimplex::atLowerBound)
                                        sol = lower_[iSet];
                                   else
                                        sol = upper_[iSet];
                                   if ((gubType_ & 8) != 0) {
                                        int iColumn = next_[iKey];
                                        // sum all non-key variables
                                        while(iColumn >= 0) {
                                             sol -= solution[iColumn];
                                             iColumn = next_[iColumn];
                                        }
                                   } else {
                                        int stop = -(iKey + 1);
                                        int iColumn = next_[iKey];
                                        // sum all non-key variables
                                        while(iColumn != stop) {
                                             if (iColumn < 0)
                                                  iColumn = -iColumn - 1;
                                             sol -= solution[iColumn];
                                             iColumn = next_[iColumn];
                                        }
                                   }
                                   solution[iKey] = sol;
                                   if (model->algorithm() > 0)
                                        model->nonLinearCost()->setOne(iKey, sol);
                                   //assert (fabs(sol-solution[iKey])<1.0e-3);
                              } else {
                                   // gub slack is basic
                                   // Save current cost of key
                                   changeCost_[number2-number] = -weight(iSet) * infeasibilityCost;
                                   otherValue = - otherValue; //allow for - sign on slack
                                   if (iSet != iSetIn)
                                        value = 0.0;
                                   else
                                        value = -1.0;
                                   pivotVariable[numberRows+iNew] = iNew + numberTotal;
                                   model->djRegion()[iNew+numberTotal] = 0.0;
                                   double sol = 0.0;
                                   if ((gubType_ & 8) != 0) {
                                        int iColumn = next_[iKey];
                                        // sum all non-key variables
                                        while(iColumn >= 0) {
                                             sol += solution[iColumn];
                                             iColumn = next_[iColumn];
                                        }
                                   } else {
                                        int stop = -(iKey + 1);
                                        int iColumn = next_[iKey];
                                        // sum all non-key variables
                                        while(iColumn != stop) {
                                             if (iColumn < 0)
                                                  iColumn = -iColumn - 1;
                                             sol += solution[iColumn];
                                             iColumn = next_[iColumn];
                                        }
                                   }
                                   solution[iNew+numberTotal] = sol;
                                   // and do cost in nonLinearCost
                                   if (model->algorithm() > 0)
                                        model->nonLinearCost()->setOne(iNew + numberTotal, sol, lower_[iSet], upper_[iSet]);
                                   if (sol > upper_[iSet] + primalTolerance) {
                                        setAbove(iSet);
                                        lower[iNew+numberTotal] = upper_[iSet];
                                        upper[iNew+numberTotal] = COIN_DBL_MAX;
                                   } else if (sol < lower_[iSet] - primalTolerance) {
                                        setBelow(iSet);
                                        lower[iNew+numberTotal] = -COIN_DBL_MAX;
                                        upper[iNew+numberTotal] = lower_[iSet];
                                   } else {
                                        setFeasible(iSet);
                                        lower[iNew+numberTotal] = lower_[iSet];
                                        upper[iNew+numberTotal] = upper_[iSet];
                                   }
                                   cost[iNew+numberTotal] = weight(iSet) * infeasibilityCost;
                              }
                              number2++;
                         } else {
                              value = array[iIndex];
                              int iKey = keyVariable_[iSet];
                              if (iKey >= numberColumns)
                                   otherValue = - otherValue; //allow for - sign on slack
                         }
                         value -= otherValue;
                         array[iIndex] = value;
                    }
               }
          }
          if (iSetIn >= 0 && toIndex_[iSetIn] < 0) {
               // Do incoming
               update->setPacked(); // just in case no elements
               toIndex_[iSetIn] = number2;
               int iNew = number2 - number;
               fromIndex_[number2-number] = iSetIn;
               // Save current cost of key
               double currentCost;
               int key = keyVariable_[iSetIn];
               if (key < numberColumns)
                    currentCost = cost[key];
               else
                    currentCost = -weight(iSetIn) * infeasibilityCost;
               changeCost_[number2-number] = currentCost;
               index[number2] = numberRows + iNew;
               // do key stuff
               int iKey = keyVariable_[iSetIn];
               if (iKey < numberColumns) {
                    if (gubSlackIn_ < 0)
                         array[number2] = 1.0;
                    else
                         array[number2] = -1.0;
                    pivotVariable[numberRows+iNew] = iKey;
                    // Do I need to recompute?
                    double sol;
                    assert (getStatus(iSetIn) != ClpSimplex::basic);
                    if (getStatus(iSetIn) == ClpSimplex::atLowerBound)
                         sol = lower_[iSetIn];
                    else
                         sol = upper_[iSetIn];
                    if ((gubType_ & 8) != 0) {
                         int iColumn = next_[iKey];
                         // sum all non-key variables
                         while(iColumn >= 0) {
                              sol -= solution[iColumn];
                              iColumn = next_[iColumn];
                         }
                    } else {
                         // bounds exist - sum over all except key
                         int stop = -(iKey + 1);
                         int iColumn = next_[iKey];
                         // sum all non-key variables
                         while(iColumn != stop) {
                              if (iColumn < 0)
                                   iColumn = -iColumn - 1;
                              sol -= solution[iColumn];
                              iColumn = next_[iColumn];
                         }
                    }
                    solution[iKey] = sol;
                    if (model->algorithm() > 0)
                         model->nonLinearCost()->setOne(iKey, sol);
                    //assert (fabs(sol-solution[iKey])<1.0e-3);
               } else {
                    // gub slack is basic
                    array[number2] = -1.0;
                    pivotVariable[numberRows+iNew] = iNew + numberTotal;
                    model->djRegion()[iNew+numberTotal] = 0.0;
                    double sol = 0.0;
                    if ((gubType_ & 8) != 0) {
                         int iColumn = next_[iKey];
                         // sum all non-key variables
                         while(iColumn >= 0) {
                              sol += solution[iColumn];
                              iColumn = next_[iColumn];
                         }
                    } else {
                         // bounds exist - sum over all except key
                         int stop = -(iKey + 1);
                         int iColumn = next_[iKey];
                         // sum all non-key variables
                         while(iColumn != stop) {
                              if (iColumn < 0)
                                   iColumn = -iColumn - 1;
                              sol += solution[iColumn];
                              iColumn = next_[iColumn];
                         }
                    }
                    solution[iNew+numberTotal] = sol;
                    // and do cost in nonLinearCost
                    if (model->algorithm() > 0)
                         model->nonLinearCost()->setOne(iNew + numberTotal, sol, lower_[iSetIn], upper_[iSetIn]);
                    if (sol > upper_[iSetIn] + primalTolerance) {
                         setAbove(iSetIn);
                         lower[iNew+numberTotal] = upper_[iSetIn];
                         upper[iNew+numberTotal] = COIN_DBL_MAX;
                    } else if (sol < lower_[iSetIn] - primalTolerance) {
                         setBelow(iSetIn);
                         lower[iNew+numberTotal] = -COIN_DBL_MAX;
                         upper[iNew+numberTotal] = lower_[iSetIn];
                    } else {
                         setFeasible(iSetIn);
                         lower[iNew+numberTotal] = lower_[iSetIn];
                         upper[iNew+numberTotal] = upper_[iSetIn];
                    }
                    cost[iNew+numberTotal] = weight(iSetIn) * infeasibilityCost;
               }
               number2++;
          }
          // mark end
          fromIndex_[number2-number] = -1;
          returnCode = number2 - number;
          // make sure lower_ upper_ adjusted
          synchronize(model, 9);
     } else {
          // take off?
          if (number > saveNumber_) {
               // clear
               double theta = model->theta();
               double * solution = model->solutionRegion();
               for (i = saveNumber_; i < number; i++) {
                    int iRow = index[i];
                    int iColumn = pivotVariable[iRow];
#ifdef CLP_DEBUG_PRINT
                    printf("Column %d (set %d) lower %g, upper %g - alpha %g - old value %g, new %g (theta %g)\n",
                           iColumn, fromIndex_[i-saveNumber_], lower[iColumn], upper[iColumn], array[i],
                           solution[iColumn], solution[iColumn] - model->theta()*array[i], model->theta());
#endif
                    double value = array[i];
                    array[i] = 0.0;
                    int iSet = fromIndex_[i-saveNumber_];
                    toIndex_[iSet] = -1;
                    if (iSet == iSetIn && iColumn < numberColumns) {
                         // update as may need value
                         solution[iColumn] -= theta * value;
                    }
               }
          }
#ifdef CLP_DEBUG
          for (i = 0; i < numberSets_; i++)
               assert(toIndex_[i] == -1);
#endif
          number2 = saveNumber_;
     }
     update->setNumElements(number2);
     return returnCode;
}
/*
     utility primal function for dealing with dynamic constraints
     mode=n see ClpGubMatrix.hpp for definition
     Remember to update here when settled down
*/
void
ClpGubMatrix::primalExpanded(ClpSimplex * model, int mode)
{
     int numberColumns = model->numberColumns();
     switch (mode) {
          // If key variable then slot in gub rhs so will get correct contribution
     case 0: {
          int i;
          double * solution = model->solutionRegion();
          ClpSimplex::Status iStatus;
          for (i = 0; i < numberSets_; i++) {
               int iColumn = keyVariable_[i];
               if (iColumn < numberColumns) {
                    // key is structural - where is slack
                    iStatus = getStatus(i);
                    assert (iStatus != ClpSimplex::basic);
                    if (iStatus == ClpSimplex::atLowerBound)
                         solution[iColumn] = lower_[i];
                    else
                         solution[iColumn] = upper_[i];
               }
          }
     }
     break;
     // Compute values of key variables
     case 1: {
          int i;
          double * solution = model->solutionRegion();
          //const int * columnLength = matrix_->getVectorLengths();
          //const CoinBigIndex * columnStart = matrix_->getVectorStarts();
          //const int * row = matrix_->getIndices();
          //const double * elementByColumn = matrix_->getElements();
          //int * pivotVariable = model->pivotVariable();
          sumPrimalInfeasibilities_ = 0.0;
          numberPrimalInfeasibilities_ = 0;
          double primalTolerance = model->primalTolerance();
          double relaxedTolerance = primalTolerance;
          // we can't really trust infeasibilities if there is primal error
          double error = CoinMin(1.0e-2, model->largestPrimalError());
          // allow tolerance at least slightly bigger than standard
          relaxedTolerance = relaxedTolerance +  error;
          // but we will be using difference
          relaxedTolerance -= primalTolerance;
          sumOfRelaxedPrimalInfeasibilities_ = 0.0;
          for (i = 0; i < numberSets_; i++) { // Could just be over basics (esp if no bounds)
               int kColumn = keyVariable_[i];
               double value = 0.0;
               if ((gubType_ & 8) != 0) {
                    int iColumn = next_[kColumn];
                    // sum all non-key variables
                    while(iColumn >= 0) {
                         value += solution[iColumn];
                         iColumn = next_[iColumn];
                    }
               } else {
                    // bounds exist - sum over all except key
                    int stop = -(kColumn + 1);
                    int iColumn = next_[kColumn];
                    // sum all non-key variables
                    while(iColumn != stop) {
                         if (iColumn < 0)
                              iColumn = -iColumn - 1;
                         value += solution[iColumn];
                         iColumn = next_[iColumn];
                    }
               }
               if (kColumn < numberColumns) {
                    // make sure key is basic - so will be skipped in values pass
                    model->setStatus(kColumn, ClpSimplex::basic);
                    // feasibility will be done later
                    assert (getStatus(i) != ClpSimplex::basic);
                    if (getStatus(i) == ClpSimplex::atUpperBound)
                         solution[kColumn] = upper_[i] - value;
                    else
                         solution[kColumn] = lower_[i] - value;
                    //printf("Value of key structural %d for set %d is %g\n",kColumn,i,solution[kColumn]);
               } else {
                    // slack is key
                    assert (getStatus(i) == ClpSimplex::basic);
                    double infeasibility = 0.0;
                    if (value > upper_[i] + primalTolerance) {
                         infeasibility = value - upper_[i] - primalTolerance;
                         setAbove(i);
                    } else if (value < lower_[i] - primalTolerance) {
                         infeasibility = lower_[i] - value - primalTolerance ;
                         setBelow(i);
                    } else {
                         setFeasible(i);
                    }
                    //printf("Value of key slack for set %d is %g\n",i,value);
                    if (infeasibility > 0.0) {
                         sumPrimalInfeasibilities_ += infeasibility;
                         if (infeasibility > relaxedTolerance)
                              sumOfRelaxedPrimalInfeasibilities_ += infeasibility;
                         numberPrimalInfeasibilities_ ++;
                    }
               }
          }
     }
     break;
     // Report on infeasibilities of key variables
     case 2: {
          model->setSumPrimalInfeasibilities(model->sumPrimalInfeasibilities() +
                                             sumPrimalInfeasibilities_);
          model->setNumberPrimalInfeasibilities(model->numberPrimalInfeasibilities() +
                                                numberPrimalInfeasibilities_);
          model->setSumOfRelaxedPrimalInfeasibilities(model->sumOfRelaxedPrimalInfeasibilities() +
                    sumOfRelaxedPrimalInfeasibilities_);
     }
     break;
     }
}
/*
     utility dual function for dealing with dynamic constraints
     mode=n see ClpGubMatrix.hpp for definition
     Remember to update here when settled down
*/
void
ClpGubMatrix::dualExpanded(ClpSimplex * model,
                           CoinIndexedVector * array,
                           double * /*other*/, int mode)
{
     switch (mode) {
          // modify costs before transposeUpdate
     case 0: {
          int i;
          double * cost = model->costRegion();
          // not dual values yet
          //assert (!other);
          //double * work = array->denseVector();
          double infeasibilityCost = model->infeasibilityCost();
          int * pivotVariable = model->pivotVariable();
          int numberRows = model->numberRows();
          int numberColumns = model->numberColumns();
          for (i = 0; i < numberRows; i++) {
               int iPivot = pivotVariable[i];
               if (iPivot < numberColumns) {
                    int iSet = backward_[iPivot];
                    if (iSet >= 0) {
                         int kColumn = keyVariable_[iSet];
                         double costValue;
                         if (kColumn < numberColumns) {
                              // structural has cost
                              costValue = cost[kColumn];
                         } else {
                              // slack is key
                              assert (getStatus(iSet) == ClpSimplex::basic);
                              // negative as -1.0 for slack
                              costValue = -weight(iSet) * infeasibilityCost;
                         }
                         array->add(i, -costValue); // was work[i]-costValue
                    }
               }
          }
     }
     break;
     // create duals for key variables (without check on dual infeasible)
     case 1: {
          // If key slack then dual 0.0 (if feasible)
          // dj for key is zero so that defines dual on set
          int i;
          double * dj = model->djRegion();
          int numberColumns = model->numberColumns();
          double infeasibilityCost = model->infeasibilityCost();
          for (i = 0; i < numberSets_; i++) {
               int kColumn = keyVariable_[i];
               if (kColumn < numberColumns) {
                    // dj without set
                    double value = dj[kColumn];
                    // Now subtract out from all
                    dj[kColumn] = 0.0;
                    int iColumn = next_[kColumn];
                    // modify all non-key variables
                    while(iColumn >= 0) {
                         dj[iColumn] -= value;
                         iColumn = next_[iColumn];
                    }
               } else {
                    // slack key - may not be feasible
                    assert (getStatus(i) == ClpSimplex::basic);
                    // negative as -1.0 for slack
                    double value = -weight(i) * infeasibilityCost;
                    if (value) {
                         int iColumn = next_[kColumn];
                         // modify all non-key variables basic
                         while(iColumn >= 0) {
                              dj[iColumn] -= value;
                              iColumn = next_[iColumn];
                         }
                    }
               }
          }
     }
     break;
     // as 1 but check slacks and compute djs
     case 2: {
          // If key slack then dual 0.0
          // If not then slack could be dual infeasible
          // dj for key is zero so that defines dual on set
          int i;
          // make sure fromIndex will not confuse pricing
          fromIndex_[0] = -1;
          possiblePivotKey_ = -1;
          // Create array
          int numberColumns = model->numberColumns();
          int * pivotVariable = model->pivotVariable();
          int numberRows = model->numberRows();
          for (i = 0; i < numberRows; i++) {
               int iPivot = pivotVariable[i];
               if (iPivot < numberColumns)
                    backToPivotRow_[iPivot] = i;
          }
          if (noCheck_ >= 0) {
               if (infeasibilityWeight_ != model->infeasibilityCost()) {
                    // don't bother checking
                    sumDualInfeasibilities_ = 100.0;
                    numberDualInfeasibilities_ = 1;
                    sumOfRelaxedDualInfeasibilities_ = 100.0;
                    return;
               }
          }
          double * dj = model->djRegion();
          double * dual = model->dualRowSolution();
          double * cost = model->costRegion();
          ClpSimplex::Status iStatus;
          const int * columnLength = matrix_->getVectorLengths();
          const CoinBigIndex * columnStart = matrix_->getVectorStarts();
          const int * row = matrix_->getIndices();
          const double * elementByColumn = matrix_->getElements();
          double infeasibilityCost = model->infeasibilityCost();
          sumDualInfeasibilities_ = 0.0;
          numberDualInfeasibilities_ = 0;
          double dualTolerance = model->dualTolerance();
          double relaxedTolerance = dualTolerance;
          // we can't really trust infeasibilities if there is dual error
          double error = CoinMin(1.0e-2, model->largestDualError());
          // allow tolerance at least slightly bigger than standard
          relaxedTolerance = relaxedTolerance +  error;
          // but we will be using difference
          relaxedTolerance -= dualTolerance;
          sumOfRelaxedDualInfeasibilities_ = 0.0;
          for (i = 0; i < numberSets_; i++) {
               int kColumn = keyVariable_[i];
               if (kColumn < numberColumns) {
                    // dj without set
                    double value = cost[kColumn];
                    for (CoinBigIndex j = columnStart[kColumn];
                              j < columnStart[kColumn] + columnLength[kColumn]; j++) {
                         int iRow = row[j];
                         value -= dual[iRow] * elementByColumn[j];
                    }
                    // Now subtract out from all
                    dj[kColumn] -= value;
                    int stop = -(kColumn + 1);
                    kColumn = next_[kColumn];
                    while (kColumn != stop) {
                         if (kColumn < 0)
                              kColumn = -kColumn - 1;
                         double djValue = dj[kColumn] - value;
                         dj[kColumn] = djValue;;
                         double infeasibility = 0.0;
                         iStatus = model->getStatus(kColumn);
                         if (iStatus == ClpSimplex::atLowerBound) {
                              if (djValue < -dualTolerance)
                                   infeasibility = -djValue - dualTolerance;
                         } else if (iStatus == ClpSimplex::atUpperBound) {
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
                         kColumn = next_[kColumn];
                    }
                    // check slack
                    iStatus = getStatus(i);
                    assert (iStatus != ClpSimplex::basic);
                    double infeasibility = 0.0;
                    // dj of slack is -(-1.0)value
                    if (iStatus == ClpSimplex::atLowerBound) {
                         if (value < -dualTolerance)
                              infeasibility = -value - dualTolerance;
                    } else if (iStatus == ClpSimplex::atUpperBound) {
                         // at upper bound
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
                    double value = -weight(i) * infeasibilityCost;
                    if (value) {
                         // Now subtract out from all
                         int kColumn = i + numberColumns;
                         int stop = -(kColumn + 1);
                         kColumn = next_[kColumn];
                         while (kColumn != stop) {
                              if (kColumn < 0)
                                   kColumn = -kColumn - 1;
                              double djValue = dj[kColumn] - value;
                              dj[kColumn] = djValue;;
                              double infeasibility = 0.0;
                              iStatus = model->getStatus(kColumn);
                              if (iStatus == ClpSimplex::atLowerBound) {
                                   if (djValue < -dualTolerance)
                                        infeasibility = -djValue - dualTolerance;
                              } else if (iStatus == ClpSimplex::atUpperBound) {
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
                              kColumn = next_[kColumn];
                         }
                    }
               }
          }
          // and get statistics for column generation
          synchronize(model, 4);
          infeasibilityWeight_ = -1.0;
     }
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
     case 4: {
          // First compute new costs etc for interesting gubs
          int iLook = 0;
          int iSet = fromIndex_[0];
          double primalTolerance = model->primalTolerance();
          const double * cost = model->costRegion();
          double * solution = model->solutionRegion();
          double infeasibilityCost = model->infeasibilityCost();
          int numberColumns = model->numberColumns();
          int numberChanged = 0;
          int * pivotVariable = model->pivotVariable();
          while (iSet >= 0) {
               int key = keyVariable_[iSet];
               double value = 0.0;
               // sum over all except key
               if ((gubType_ & 8) != 0) {
                    int iColumn = next_[key];
                    // sum all non-key variables
                    while(iColumn >= 0) {
                         value += solution[iColumn];
                         iColumn = next_[iColumn];
                    }
               } else {
                    // bounds exist - sum over all except key
                    int stop = -(key + 1);
                    int iColumn = next_[key];
                    // sum all non-key variables
                    while(iColumn != stop) {
                         if (iColumn < 0)
                              iColumn = -iColumn - 1;
                         value += solution[iColumn];
                         iColumn = next_[iColumn];
                    }
               }
               double costChange;
               double oldCost = changeCost_[iLook];
               if (key < numberColumns) {
                    assert (getStatus(iSet) != ClpSimplex::basic);
                    double sol;
                    if (getStatus(iSet) == ClpSimplex::atUpperBound)
                         sol = upper_[iSet] - value;
                    else
                         sol = lower_[iSet] - value;
                    solution[key] = sol;
                    // fix up cost
                    model->nonLinearCost()->setOne(key, sol);
#ifdef CLP_DEBUG_PRINT
                    printf("yy Value of key structural %d for set %d is %g - cost %g old cost %g\n", key, iSet, sol,
                           cost[key], oldCost);
#endif
                    costChange = cost[key] - oldCost;
               } else {
                    // slack is key
                    if (value > upper_[iSet] + primalTolerance) {
                         setAbove(iSet);
                    } else if (value < lower_[iSet] - primalTolerance) {
                         setBelow(iSet);
                    } else {
                         setFeasible(iSet);
                    }
                    // negative as -1.0 for slack
                    costChange = -weight(iSet) * infeasibilityCost - oldCost;
#ifdef CLP_DEBUG_PRINT
                    printf("yy Value of key slack for set %d is %g - cost %g old cost %g\n", iSet, value,
                           weight(iSet)*infeasibilityCost, oldCost);
#endif
               }
               if (costChange) {
                    fromIndex_[numberChanged] = iSet;
                    toIndex_[iSet] = numberChanged;
                    changeCost_[numberChanged++] = costChange;
               }
               iSet = fromIndex_[++iLook];
          }
          if (numberChanged || possiblePivotKey_ >= 0) {
               // first do those in list already
               int number = array->getNumElements();
               array->setPacked();
               int i;
               double * work = array->denseVector();
               int * which = array->getIndices();
               for (i = 0; i < number; i++) {
                    int iRow = which[i];
                    int iPivot = pivotVariable[iRow];
                    if (iPivot < numberColumns) {
                         int iSet = backward_[iPivot];
                         if (iSet >= 0 && toIndex_[iSet] >= 0) {
                              double newValue = work[i] + changeCost_[toIndex_[iSet]];
                              if (!newValue)
                                   newValue = 1.0e-100;
                              work[i] = newValue;
                              // mark as done
                              backward_[iPivot] = -1;
                         }
                    }
                    if (possiblePivotKey_ == iRow) {
                         double newValue = work[i] - model->dualIn();
                         if (!newValue)
                              newValue = 1.0e-100;
                         work[i] = newValue;
                         possiblePivotKey_ = -1;
                    }
               }
               // now do rest and clean up
               for (i = 0; i < numberChanged; i++) {
                    int iSet = fromIndex_[i];
                    int key = keyVariable_[iSet];
                    int iColumn = next_[key];
                    double change = changeCost_[i];
                    while (iColumn >= 0) {
                         if (backward_[iColumn] >= 0) {
                              int iRow = backToPivotRow_[iColumn];
                              assert (iRow >= 0);
                              work[number] = change;
                              if (possiblePivotKey_ == iRow) {
                                   double newValue = work[number] - model->dualIn();
                                   if (!newValue)
                                        newValue = 1.0e-100;
                                   work[number] = newValue;
                                   possiblePivotKey_ = -1;
                              }
                              which[number++] = iRow;
                         } else {
                              // reset
                              backward_[iColumn] = iSet;
                         }
                         iColumn = next_[iColumn];
                    }
                    toIndex_[iSet] = -1;
               }
               if (possiblePivotKey_ >= 0) {
                    work[number] = -model->dualIn();
                    which[number++] = possiblePivotKey_;
                    possiblePivotKey_ = -1;
               }
               fromIndex_[0] = -1;
               array->setNumElements(number);
          }
     }
     break;
     }
}
// This is local to Gub to allow synchronization when status is good
int
ClpGubMatrix::synchronize(ClpSimplex *, int)
{
     return 0;
}
/*
     general utility function for dealing with dynamic constraints
     mode=n see ClpGubMatrix.hpp for definition
     Remember to update here when settled down
*/
int
ClpGubMatrix::generalExpanded(ClpSimplex * model, int mode, int &number)
{
     int returnCode = 0;
     int numberColumns = model->numberColumns();
     switch (mode) {
          // Fill in pivotVariable but not for key variables
     case 0: {
          if (!next_ ) {
               // do ordering
               assert (!rhsOffset_);
               // create and do gub crash
               useEffectiveRhs(model, false);
          }
          int i;
          int numberBasic = number;
          // Use different array so can build from true pivotVariable_
          //int * pivotVariable = model->pivotVariable();
          int * pivotVariable = model->rowArray(0)->getIndices();
          for (i = 0; i < numberColumns; i++) {
               if (model->getColumnStatus(i) == ClpSimplex::basic) {
                    int iSet = backward_[i];
                    if (iSet < 0 || i != keyVariable_[iSet])
                         pivotVariable[numberBasic++] = i;
               }
          }
          number = numberBasic;
          if (model->numberIterations())
               assert (number == model->numberRows());
     }
     break;
     // Make all key variables basic
     case 1: {
          int i;
          for (i = 0; i < numberSets_; i++) {
               int iColumn = keyVariable_[i];
               if (iColumn < numberColumns)
                    model->setColumnStatus(iColumn, ClpSimplex::basic);
          }
     }
     break;
     // Do initial extra rows + maximum basic
     case 2: {
          returnCode = getNumRows() + 1;
          number = model->numberRows() + numberSets_;
     }
     break;
     // Before normal replaceColumn
     case 3: {
          int sequenceIn = model->sequenceIn();
          int sequenceOut = model->sequenceOut();
          int numberColumns = model->numberColumns();
          int numberRows = model->numberRows();
          int pivotRow = model->pivotRow();
          if (gubSlackIn_ >= 0)
               assert (sequenceIn > numberRows + numberColumns);
          if (sequenceIn == sequenceOut)
               return -1;
          int iSetIn = -1;
          int iSetOut = -1;
          if (sequenceOut < numberColumns) {
               iSetOut = backward_[sequenceOut];
          } else if (sequenceOut >= numberRows + numberColumns) {
               assert (pivotRow >= numberRows);
               int iExtra = pivotRow - numberRows;
               assert (iExtra >= 0);
               if (iSetOut < 0)
                    iSetOut = fromIndex_[iExtra];
               else
                    assert(iSetOut == fromIndex_[iExtra]);
          }
          if (sequenceIn < numberColumns) {
               iSetIn = backward_[sequenceIn];
          } else if (gubSlackIn_ >= 0) {
               iSetIn = gubSlackIn_;
          }
          possiblePivotKey_ = -1;
          number = 0; // say do ordinary
          int * pivotVariable = model->pivotVariable();
          if (pivotRow >= numberRows) {
               int iExtra = pivotRow - numberRows;
               //const int * length = matrix_->getVectorLengths();

               assert (sequenceOut >= numberRows + numberColumns ||
                       sequenceOut == keyVariable_[iSetOut]);
               int incomingColumn = sequenceIn; // to be used in updates
               if (iSetIn != iSetOut) {
                    // We need to find a possible pivot for incoming
                    // look through rowArray_[1]
                    int n = model->rowArray(1)->getNumElements();
                    int * which = model->rowArray(1)->getIndices();
                    double * array = model->rowArray(1)->denseVector();
                    double bestAlpha = 1.0e-5;
                    //int shortest=numberRows+1;
                    for (int i = 0; i < n; i++) {
                         int iRow = which[i];
                         int iPivot = pivotVariable[iRow];
                         if (iPivot < numberColumns && backward_[iPivot] == iSetOut) {
                              if (fabs(array[i]) > fabs(bestAlpha)) {
                                   bestAlpha = array[i];
                                   possiblePivotKey_ = iRow;
                              }
                         }
                    }
                    assert (possiblePivotKey_ >= 0); // could set returnCode=4
                    number = 1;
                    if (sequenceIn >= numberRows + numberColumns) {
                         number = 3;
                         // need swap as gub slack in and must become key
                         // is this best way
                         int key = keyVariable_[iSetIn];
                         assert (key < numberColumns);
                         // check other basic
                         int iColumn = next_[key];
                         // set new key to be used by unpack
                         keyVariable_[iSetIn] = iSetIn + numberColumns;
                         // change cost in changeCost
                         {
                              int iLook = 0;
                              int iSet = fromIndex_[0];
                              while (iSet >= 0) {
                                   if (iSet == iSetIn) {
                                        changeCost_[iLook] = 0.0;
                                        break;
                                   }
                                   iSet = fromIndex_[++iLook];
                              }
                         }
                         while (iColumn >= 0) {
                              if (iColumn != sequenceOut) {
                                   // need partial ftran and skip accuracy check in replaceColumn
#ifdef CLP_DEBUG_PRINT
                                   printf("TTTTTry 5\n");
#endif
                                   int iRow = backToPivotRow_[iColumn];
                                   assert (iRow >= 0);
                                   unpack(model, model->rowArray(3), iColumn);
                                   model->factorization()->updateColumnFT(model->rowArray(2), model->rowArray(3));
                                   double alpha = model->rowArray(3)->denseVector()[iRow];
                                   //if (!alpha)
                                   //printf("zero alpha a\n");
                                   int updateStatus = model->factorization()->replaceColumn(model,
                                                      model->rowArray(2),
                                                      model->rowArray(3),
                                                      iRow, alpha);
                                   returnCode = CoinMax(updateStatus, returnCode);
                                   model->rowArray(3)->clear();
                                   if (returnCode)
                                        break;
                              }
                              iColumn = next_[iColumn];
                         }
                         if (!returnCode) {
                              // now factorization looks as if key is out
                              // pivot back in
#ifdef CLP_DEBUG_PRINT
                              printf("TTTTTry 6\n");
#endif
                              unpack(model, model->rowArray(3), key);
                              model->factorization()->updateColumnFT(model->rowArray(2), model->rowArray(3));
                              pivotRow = possiblePivotKey_;
                              double alpha = model->rowArray(3)->denseVector()[pivotRow];
                              //if (!alpha)
                              //printf("zero alpha b\n");
                              int updateStatus = model->factorization()->replaceColumn(model,
                                                 model->rowArray(2),
                                                 model->rowArray(3),
                                                 pivotRow, alpha);
                              returnCode = CoinMax(updateStatus, returnCode);
                              model->rowArray(3)->clear();
                         }
                         // restore key
                         keyVariable_[iSetIn] = key;
                         // now alternate column can replace key on out
                         incomingColumn = pivotVariable[possiblePivotKey_];
                    } else {
#ifdef CLP_DEBUG_PRINT
                         printf("TTTTTTry 4 %d\n", possiblePivotKey_);
#endif
                         int updateStatus = model->factorization()->replaceColumn(model,
                                            model->rowArray(2),
                                            model->rowArray(1),
                                            possiblePivotKey_,
                                            bestAlpha);
                         returnCode = CoinMax(updateStatus, returnCode);
                         incomingColumn = pivotVariable[possiblePivotKey_];
                    }

                    //returnCode=4; // need swap
               } else {
                    // key swap
                    number = -1;
               }
               int key = keyVariable_[iSetOut];
               if (key < numberColumns)
                    assert(key == sequenceOut);
               // check if any other basic
               int iColumn = next_[key];
               if (returnCode)
                    iColumn = -1; // skip if error on previous
               // set new key to be used by unpack
               if (incomingColumn < numberColumns)
                    keyVariable_[iSetOut] = incomingColumn;
               else
                    keyVariable_[iSetOut] = iSetIn + numberColumns;
               double * cost = model->costRegion();
               if (possiblePivotKey_ < 0) {
                    double dj = model->djRegion()[sequenceIn] - cost[sequenceIn];
                    changeCost_[iExtra] = -dj;
#ifdef CLP_DEBUG_PRINT
                    printf("modifying changeCost %d by %g - cost %g\n", iExtra, dj, cost[sequenceIn]);
#endif
               }
               while (iColumn >= 0) {
                    if (iColumn != incomingColumn) {
                         number = -2;
                         // need partial ftran and skip accuracy check in replaceColumn
#ifdef CLP_DEBUG_PRINT
                         printf("TTTTTTry 1\n");
#endif
                         int iRow = backToPivotRow_[iColumn];
                         assert (iRow >= 0 && iRow < numberRows);
                         unpack(model, model->rowArray(3), iColumn);
                         model->factorization()->updateColumnFT(model->rowArray(2), model->rowArray(3));
                         double * array = model->rowArray(3)->denseVector();
                         double alpha = array[iRow];
                         //if (!alpha)
                         //printf("zero alpha d\n");
                         int updateStatus = model->factorization()->replaceColumn(model,
                                            model->rowArray(2),
                                            model->rowArray(3),
                                            iRow, alpha);
                         returnCode = CoinMax(updateStatus, returnCode);
                         model->rowArray(3)->clear();
                         if (returnCode)
                              break;
                    }
                    iColumn = next_[iColumn];
               }
               // restore key
               keyVariable_[iSetOut] = key;
          } else if (sequenceIn >= numberRows + numberColumns) {
               number = 2;
               //returnCode=4;
               // need swap as gub slack in and must become key
               // is this best way
               int key = keyVariable_[iSetIn];
               assert (key < numberColumns);
               // check other basic
               int iColumn = next_[key];
               // set new key to be used by unpack
               keyVariable_[iSetIn] = iSetIn + numberColumns;
               // change cost in changeCost
               {
                    int iLook = 0;
                    int iSet = fromIndex_[0];
                    while (iSet >= 0) {
                         if (iSet == iSetIn) {
                              changeCost_[iLook] = 0.0;
                              break;
                         }
                         iSet = fromIndex_[++iLook];
                    }
               }
               while (iColumn >= 0) {
                    if (iColumn != sequenceOut) {
                         // need partial ftran and skip accuracy check in replaceColumn
#ifdef CLP_DEBUG_PRINT
                         printf("TTTTTry 2\n");
#endif
                         int iRow = backToPivotRow_[iColumn];
                         assert (iRow >= 0);
                         unpack(model, model->rowArray(3), iColumn);
                         model->factorization()->updateColumnFT(model->rowArray(2), model->rowArray(3));
                         double alpha = model->rowArray(3)->denseVector()[iRow];
                         //if (!alpha)
                         //printf("zero alpha e\n");
                         int updateStatus = model->factorization()->replaceColumn(model,
                                            model->rowArray(2),
                                            model->rowArray(3),
                                            iRow, alpha);
                         returnCode = CoinMax(updateStatus, returnCode);
                         model->rowArray(3)->clear();
                         if (returnCode)
                              break;
                    }
                    iColumn = next_[iColumn];
               }
               if (!returnCode) {
                    // now factorization looks as if key is out
                    // pivot back in
#ifdef CLP_DEBUG_PRINT
                    printf("TTTTTry 3\n");
#endif
                    unpack(model, model->rowArray(3), key);
                    model->factorization()->updateColumnFT(model->rowArray(2), model->rowArray(3));
                    double alpha = model->rowArray(3)->denseVector()[pivotRow];
                    //if (!alpha)
                    //printf("zero alpha f\n");
                    int updateStatus = model->factorization()->replaceColumn(model,
                                       model->rowArray(2),
                                       model->rowArray(3),
                                       pivotRow, alpha);
                    returnCode = CoinMax(updateStatus, returnCode);
                    model->rowArray(3)->clear();
               }
               // restore key
               keyVariable_[iSetIn] = key;
          } else {
               // normal - but might as well do here
               returnCode = model->factorization()->replaceColumn(model,
                            model->rowArray(2),
                            model->rowArray(1),
                            model->pivotRow(),
                            model->alpha());
          }
     }
#ifdef CLP_DEBUG_PRINT
     printf("Update type after %d - status %d - pivot row %d\n",
            number, returnCode, model->pivotRow());
#endif
     // see if column generation says time to re-factorize
     returnCode = CoinMax(returnCode, synchronize(model, 5));
     number = -1; // say no need for normal replaceColumn
     break;
     // To see if can dual or primal
     case 4: {
          returnCode = 1;
     }
     break;
     // save status
     case 5: {
          synchronize(model, 0);
          CoinMemcpyN(status_, numberSets_, saveStatus_);
          CoinMemcpyN(keyVariable_, numberSets_, savedKeyVariable_);
     }
     break;
     // restore status
     case 6: {
          CoinMemcpyN(saveStatus_, numberSets_, status_);
          CoinMemcpyN(savedKeyVariable_, numberSets_, keyVariable_);
          // restore firstAvailable_
          synchronize(model, 7);
          // redo next_
          int i;
          int * last = new int[numberSets_];
          for (i = 0; i < numberSets_; i++) {
               int iKey = keyVariable_[i];
               assert(iKey >= numberColumns || backward_[iKey] == i);
               last[i] = iKey;
               // make sure basic
               //if (iKey<numberColumns)
               //model->setStatus(iKey,ClpSimplex::basic);
          }
          for (i = 0; i < numberColumns; i++) {
               int iSet = backward_[i];
               if (iSet >= 0) {
                    next_[last[iSet]] = i;
                    last[iSet] = i;
               }
          }
          for (i = 0; i < numberSets_; i++) {
               next_[last[i]] = -(keyVariable_[i] + 1);
               redoSet(model, keyVariable_[i], keyVariable_[i], i);
          }
          delete [] last;
          // redo pivotVariable
          int * pivotVariable = model->pivotVariable();
          int iRow;
          int numberBasic = 0;
          int numberRows = model->numberRows();
          for (iRow = 0; iRow < numberRows; iRow++) {
               if (model->getRowStatus(iRow) == ClpSimplex::basic) {
                    numberBasic++;
                    pivotVariable[iRow] = iRow + numberColumns;
               } else {
                    pivotVariable[iRow] = -1;
               }
          }
          i = 0;
          int iColumn;
          for (iColumn = 0; iColumn < numberColumns; iColumn++) {
               if (model->getStatus(iColumn) == ClpSimplex::basic) {
                    int iSet = backward_[iColumn];
                    if (iSet < 0 || keyVariable_[iSet] != iColumn) {
                         while (pivotVariable[i] >= 0) {
                              i++;
                              assert (i < numberRows);
                         }
                         pivotVariable[i] = iColumn;
                         backToPivotRow_[iColumn] = i;
                         numberBasic++;
                    }
               }
          }
          assert (numberBasic == numberRows);
          rhsOffset(model, true);
     }
     break;
     // flag a variable
     case 7: {
          assert (number == model->sequenceIn());
          synchronize(model, 1);
          synchronize(model, 8);
     }
     break;
     // unflag all variables
     case 8: {
          returnCode = synchronize(model, 2);
     }
     break;
     // redo costs in primal
     case 9: {
          returnCode = synchronize(model, 3);
     }
     break;
     // return 1 if there may be changing bounds on variable (column generation)
     case 10: {
          returnCode = synchronize(model, 6);
     }
     break;
     // make sure set is clean
     case 11: {
          assert (number == model->sequenceIn());
          returnCode = synchronize(model, 8);
     }
     break;
     default:
          break;
     }
     return returnCode;
}
// Sets up an effective RHS and does gub crash if needed
void
ClpGubMatrix::useEffectiveRhs(ClpSimplex * model, bool cheapest)
{
     // Do basis - cheapest or slack if feasible (unless cheapest set)
     int longestSet = 0;
     int iSet;
     for (iSet = 0; iSet < numberSets_; iSet++)
          longestSet = CoinMax(longestSet, end_[iSet] - start_[iSet]);

     double * upper = new double[longestSet+1];
     double * cost = new double[longestSet+1];
     double * lower = new double[longestSet+1];
     double * solution = new double[longestSet+1];
     assert (!next_);
     int numberColumns = getNumCols();
     const int * columnLength = matrix_->getVectorLengths();
     const double * columnLower = model->lowerRegion();
     const double * columnUpper = model->upperRegion();
     double * columnSolution = model->solutionRegion();
     const double * objective = model->costRegion();
     int numberRows = getNumRows();
     toIndex_ = new int[numberSets_];
     for (iSet = 0; iSet < numberSets_; iSet++)
          toIndex_[iSet] = -1;
     fromIndex_ = new int [getNumRows()+1];
     double tolerance = model->primalTolerance();
     bool noNormalBounds = true;
     gubType_ &= ~8;
     bool gotBasis = false;
     for (iSet = 0; iSet < numberSets_; iSet++) {
          if (keyVariable_[iSet] < numberColumns)
               gotBasis = true;
          CoinBigIndex j;
          CoinBigIndex iStart = start_[iSet];
          CoinBigIndex iEnd = end_[iSet];
          for (j = iStart; j < iEnd; j++) {
               if (columnLower[j] && columnLower[j] > -1.0e20)
                    noNormalBounds = false;
               if (columnUpper[j] && columnUpper[j] < 1.0e20)
                    noNormalBounds = false;
          }
     }
     if (noNormalBounds)
          gubType_ |= 8;
     if (!gotBasis) {
          for (iSet = 0; iSet < numberSets_; iSet++) {
               CoinBigIndex j;
               int numberBasic = 0;
               int iBasic = -1;
               CoinBigIndex iStart = start_[iSet];
               CoinBigIndex iEnd = end_[iSet];
               // find one with smallest length
               int smallest = numberRows + 1;
               double value = 0.0;
               for (j = iStart; j < iEnd; j++) {
                    if (model->getStatus(j) == ClpSimplex::basic) {
                         if (columnLength[j] < smallest) {
                              smallest = columnLength[j];
                              iBasic = j;
                         }
                         numberBasic++;
                    }
                    value += columnSolution[j];
               }
               bool done = false;
               if (numberBasic > 1 || (numberBasic == 1 && getStatus(iSet) == ClpSimplex::basic)) {
                    if (getStatus(iSet) == ClpSimplex::basic)
                         iBasic = iSet + numberColumns; // slack key - use
                    done = true;
               } else if (numberBasic == 1) {
                    // see if can be key
                    double thisSolution = columnSolution[iBasic];
                    if (thisSolution > columnUpper[iBasic]) {
                         value -= thisSolution - columnUpper[iBasic];
                         thisSolution = columnUpper[iBasic];
                         columnSolution[iBasic] = thisSolution;
                    }
                    if (thisSolution < columnLower[iBasic]) {
                         value -= thisSolution - columnLower[iBasic];
                         thisSolution = columnLower[iBasic];
                         columnSolution[iBasic] = thisSolution;
                    }
                    // try setting slack to a bound
                    assert (upper_[iSet] < 1.0e20 || lower_[iSet] > -1.0e20);
                    double cost1 = COIN_DBL_MAX;
                    int whichBound = -1;
                    if (upper_[iSet] < 1.0e20) {
                         // try slack at ub
                         double newBasic = thisSolution + upper_[iSet] - value;
                         if (newBasic >= columnLower[iBasic] - tolerance &&
                                   newBasic <= columnUpper[iBasic] + tolerance) {
                              // can go
                              whichBound = 1;
                              cost1 = newBasic * objective[iBasic];
                              // But if exact then may be good solution
                              if (fabs(upper_[iSet] - value) < tolerance)
                                   cost1 = -COIN_DBL_MAX;
                         }
                    }
                    if (lower_[iSet] > -1.0e20) {
                         // try slack at lb
                         double newBasic = thisSolution + lower_[iSet] - value;
                         if (newBasic >= columnLower[iBasic] - tolerance &&
                                   newBasic <= columnUpper[iBasic] + tolerance) {
                              // can go but is it cheaper
                              double cost2 = newBasic * objective[iBasic];
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
                         CoinMemcpyN(columnLower + iStart, numberInSet, lower);
                         CoinMemcpyN(columnUpper + iStart, numberInSet, upper);
                         CoinMemcpyN(columnSolution + iStart, numberInSet, solution);
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
                              CoinMemcpyN(objective + iStart, numberInSet, cost);
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
                                   CoinMemcpyN(objective + iStart, numberInSet, cost);
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
                         // convert iBasic back and do bounds
                         if (iBasic == numberInSet) {
                              // slack basic
                              setStatus(iSet, ClpSimplex::basic);
                              iBasic = iSet + numberColumns;
                         } else {
                              iBasic += start_[iSet];
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
                              if (model->getStatus(j) != ClpSimplex::basic) {
                                   int inSet = j - iStart;
                                   columnSolution[j] = solution[inSet];
                                   if (upper[inSet] == lower[inSet])
                                        model->setStatus(j, ClpSimplex::isFixed);
                                   else if (solution[inSet] == upper[inSet])
                                        model->setStatus(j, ClpSimplex::atUpperBound);
                                   else if (solution[inSet] == lower[inSet])
                                        model->setStatus(j, ClpSimplex::atLowerBound);
                              }
                         }
                    }
               }
               keyVariable_[iSet] = iBasic;
          }
     }
     delete [] lower;
     delete [] solution;
     delete [] upper;
     delete [] cost;
     // make sure matrix is in good shape
     matrix_->orderMatrix();
     // create effective rhs
     delete [] rhsOffset_;
     rhsOffset_ = new double[numberRows];
     delete [] next_;
     next_ = new int[numberColumns+numberSets_+2*longestSet];
     char * mark = new char[numberColumns];
     memset(mark, 0, numberColumns);
     for (int iColumn = 0; iColumn < numberColumns; iColumn++)
          next_[iColumn] = COIN_INT_MAX;
     int i;
     int * keys = new int[numberSets_];
     for (i = 0; i < numberSets_; i++)
          keys[i] = COIN_INT_MAX;
     // set up chains
     for (i = 0; i < numberColumns; i++) {
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
          int j;
          if (getStatus(i) != ClpSimplex::basic) {
               // make sure fixed if it is
               if (upper_[i] == lower_[i])
                    setStatus(i, ClpSimplex::isFixed);
               // slack not key - choose one with smallest length
               int smallest = numberRows + 1;
               int key = -1;
               j = keys[i];
               if (j != COIN_INT_MAX) {
                    while (1) {
                         if (mark[j] && columnLength[j] < smallest && !gotBasis) {
                              key = j;
                              smallest = columnLength[j];
                         }
                         if (next_[j] != COIN_INT_MAX) {
                              j = next_[j];
                         } else {
                              // correct end
                              next_[j] = -(keys[i] + 1);
                              break;
                         }
                    }
               } else {
                    next_[i+numberColumns] = -(numberColumns + i + 1);
               }
               if (gotBasis)
                    key = keyVariable_[i];
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
               if (j != COIN_INT_MAX) {
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
               } else {
                    next_[i+numberColumns] = -(numberColumns + i + 1);
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
     }
     delete [] keys;
     delete [] mark;
     rhsOffset(model, true);
}
// redoes next_ for a set.
void
ClpGubMatrix::redoSet(ClpSimplex * model, int newKey, int oldKey, int iSet)
{
     int numberColumns = model->numberColumns();
     int * save = next_ + numberColumns + numberSets_;
     int number = 0;
     int stop = -(oldKey + 1);
     int j = next_[oldKey];
     while (j != stop) {
          if (j < 0)
               j = -j - 1;
          if (j != newKey)
               save[number++] = j;
          j = next_[j];
     }
     // and add oldkey
     if (newKey != oldKey)
          save[number++] = oldKey;
     // now do basic
     int lastMarker = -(newKey + 1);
     keyVariable_[iSet] = newKey;
     next_[newKey] = lastMarker;
     int last = newKey;
     for ( j = 0; j < number; j++) {
          int iColumn = save[j];
          if (iColumn < numberColumns) {
               if (model->getStatus(iColumn) == ClpSimplex::basic) {
                    next_[last] = iColumn;
                    next_[iColumn] = lastMarker;
                    last = iColumn;
               }
          }
     }
     // now add in non-basic
     for ( j = 0; j < number; j++) {
          int iColumn = save[j];
          if (iColumn < numberColumns) {
               if (model->getStatus(iColumn) != ClpSimplex::basic) {
                    next_[last] = -(iColumn + 1);
                    next_[iColumn] = lastMarker;
                    last = iColumn;
               }
          }
     }

}
/* Returns effective RHS if it is being used.  This is used for long problems
   or big gub or anywhere where going through full columns is
   expensive.  This may re-compute */
double *
ClpGubMatrix::rhsOffset(ClpSimplex * model, bool forceRefresh, bool
#ifdef CLP_DEBUG
                        check
#endif
                       )
{
     //forceRefresh=true;
     if (rhsOffset_) {
#ifdef CLP_DEBUG
          if (check) {
               // no need - but check anyway
               // zero out basic
               int numberRows = model->numberRows();
               int numberColumns = model->numberColumns();
               double * solution = new double [numberColumns];
               double * rhs = new double[numberRows];
               CoinMemcpyN(model->solutionRegion(), numberColumns, solution);
               CoinZeroN(rhs, numberRows);
               int iRow;
               for (int iColumn = 0; iColumn < numberColumns; iColumn++) {
                    if (model->getColumnStatus(iColumn) == ClpSimplex::basic)
                         solution[iColumn] = 0.0;
               }
               for (int iSet = 0; iSet < numberSets_; iSet++) {
                    int iColumn = keyVariable_[iSet];
                    if (iColumn < numberColumns)
                         solution[iColumn] = 0.0;
               }
               times(-1.0, solution, rhs);
               delete [] solution;
               const double * columnSolution = model->solutionRegion();
               // and now subtract out non basic
               ClpSimplex::Status iStatus;
               for (int iSet = 0; iSet < numberSets_; iSet++) {
                    int iColumn = keyVariable_[iSet];
                    if (iColumn < numberColumns) {
                         double b = 0.0;
                         // key is structural - where is slack
                         iStatus = getStatus(iSet);
                         assert (iStatus != ClpSimplex::basic);
                         if (iStatus == ClpSimplex::atLowerBound)
                              b = lower_[iSet];
                         else
                              b = upper_[iSet];
                         // subtract out others at bounds
                         if ((gubType_ & 8) == 0) {
                              int stop = -(iColumn + 1);
                              int jColumn = next_[iColumn];
                              // sum all non-basic variables - first skip basic
                              while(jColumn >= 0)
                                   jColumn = next_[jColumn];
                              while(jColumn != stop) {
                                   assert (jColumn < 0);
                                   jColumn = -jColumn - 1;
                                   b -= columnSolution[jColumn];
                                   jColumn = next_[jColumn];
                              }
                         }
                         // subtract out
                         ClpPackedMatrix::add(model, rhs, iColumn, -b);
                    }
               }
               for (iRow = 0; iRow < numberRows; iRow++) {
                    if (fabs(rhs[iRow] - rhsOffset_[iRow]) > 1.0e-3)
                         printf("** bad effective %d - true %g old %g\n", iRow, rhs[iRow], rhsOffset_[iRow]);
               }
               delete [] rhs;
          }
#endif
          if (forceRefresh || (refreshFrequency_ && model->numberIterations() >=
                               lastRefresh_ + refreshFrequency_)) {
               // zero out basic
               int numberRows = model->numberRows();
               int numberColumns = model->numberColumns();
               double * solution = new double [numberColumns];
               CoinMemcpyN(model->solutionRegion(), numberColumns, solution);
               CoinZeroN(rhsOffset_, numberRows);
               for (int iColumn = 0; iColumn < numberColumns; iColumn++) {
                    if (model->getColumnStatus(iColumn) == ClpSimplex::basic)
                         solution[iColumn] = 0.0;
               }
               int iSet;
               for ( iSet = 0; iSet < numberSets_; iSet++) {
                    int iColumn = keyVariable_[iSet];
                    if (iColumn < numberColumns)
                         solution[iColumn] = 0.0;
               }
               times(-1.0, solution, rhsOffset_);
               delete [] solution;
               lastRefresh_ = model->numberIterations();
               const double * columnSolution = model->solutionRegion();
               // and now subtract out non basic
               ClpSimplex::Status iStatus;
               for ( iSet = 0; iSet < numberSets_; iSet++) {
                    int iColumn = keyVariable_[iSet];
                    if (iColumn < numberColumns) {
                         double b = 0.0;
                         // key is structural - where is slack
                         iStatus = getStatus(iSet);
                         assert (iStatus != ClpSimplex::basic);
                         if (iStatus == ClpSimplex::atLowerBound)
                              b = lower_[iSet];
                         else
                              b = upper_[iSet];
                         // subtract out others at bounds
                         if ((gubType_ & 8) == 0) {
                              int stop = -(iColumn + 1);
                              int jColumn = next_[iColumn];
                              // sum all non-basic variables - first skip basic
                              while(jColumn >= 0)
                                   jColumn = next_[jColumn];
                              while(jColumn != stop) {
                                   assert (jColumn < 0);
                                   jColumn = -jColumn - 1;
                                   b -= columnSolution[jColumn];
                                   jColumn = next_[jColumn];
                              }
                         }
                         // subtract out
                         if (b)
                              ClpPackedMatrix::add(model, rhsOffset_, iColumn, -b);
                    }
               }
          }
     }
     return rhsOffset_;
}
/*
   update information for a pivot (and effective rhs)
*/
int
ClpGubMatrix::updatePivot(ClpSimplex * model, double oldInValue, double /*oldOutValue*/)
{
     int sequenceIn = model->sequenceIn();
     int sequenceOut = model->sequenceOut();
     double * solution = model->solutionRegion();
     int numberColumns = model->numberColumns();
     int numberRows = model->numberRows();
     int pivotRow = model->pivotRow();
     int iSetIn;
     // Correct sequence in
     trueSequenceIn_ = sequenceIn;
     if (sequenceIn < numberColumns) {
          iSetIn = backward_[sequenceIn];
     } else if (sequenceIn < numberColumns + numberRows) {
          iSetIn = -1;
     } else {
          iSetIn = gubSlackIn_;
          trueSequenceIn_ = numberColumns + numberRows + iSetIn;
     }
     int iSetOut = -1;
     trueSequenceOut_ = sequenceOut;
     if (sequenceOut < numberColumns) {
          iSetOut = backward_[sequenceOut];
     } else if (sequenceOut >= numberRows + numberColumns) {
          assert (pivotRow >= numberRows);
          int iExtra = pivotRow - numberRows;
          assert (iExtra >= 0);
          if (iSetOut < 0)
               iSetOut = fromIndex_[iExtra];
          else
               assert(iSetOut == fromIndex_[iExtra]);
          trueSequenceOut_ = numberColumns + numberRows + iSetOut;
     }
     if (rhsOffset_) {
          // update effective rhs
          if (sequenceIn == sequenceOut) {
               assert (sequenceIn < numberRows + numberColumns); // should be easy to deal with
               if (sequenceIn < numberColumns)
                    add(model, rhsOffset_, sequenceIn, oldInValue - solution[sequenceIn]);
          } else {
               if (sequenceIn < numberColumns) {
                    // we need to test if WILL be key
                    ClpPackedMatrix::add(model, rhsOffset_, sequenceIn, oldInValue);
                    if (iSetIn >= 0)  {
                         // old contribution to rhsOffset_
                         int key = keyVariable_[iSetIn];
                         if (key < numberColumns) {
                              double oldB = 0.0;
                              ClpSimplex::Status iStatus = getStatus(iSetIn);
                              if (iStatus == ClpSimplex::atLowerBound)
                                   oldB = lower_[iSetIn];
                              else
                                   oldB = upper_[iSetIn];
                              // subtract out others at bounds
                              if ((gubType_ & 8) == 0) {
                                   int stop = -(key + 1);
                                   int iColumn = next_[key];
                                   // skip basic
                                   while (iColumn >= 0)
                                        iColumn = next_[iColumn];
                                   // sum all non-key variables
                                   while(iColumn != stop) {
                                        assert (iColumn < 0);
                                        iColumn = -iColumn - 1;
                                        if (iColumn == sequenceIn)
                                             oldB -= oldInValue;
                                        else if ( iColumn != sequenceOut )
                                             oldB -= solution[iColumn];
                                        iColumn = next_[iColumn];
                                   }
                              }
                              if (oldB)
                                   ClpPackedMatrix::add(model, rhsOffset_, key, oldB);
                         }
                    }
               } else if (sequenceIn < numberRows + numberColumns) {
                    //rhsOffset_[sequenceIn-numberColumns] -= oldInValue;
               } else {
#ifdef CLP_DEBUG_PRINT
                    printf("** in is key slack %d\n", sequenceIn);
#endif
                    // old contribution to rhsOffset_
                    int key = keyVariable_[iSetIn];
                    if (key < numberColumns) {
                         double oldB = 0.0;
                         ClpSimplex::Status iStatus = getStatus(iSetIn);
                         if (iStatus == ClpSimplex::atLowerBound)
                              oldB = lower_[iSetIn];
                         else
                              oldB = upper_[iSetIn];
                         // subtract out others at bounds
                         if ((gubType_ & 8) == 0) {
                              int stop = -(key + 1);
                              int iColumn = next_[key];
                              // skip basic
                              while (iColumn >= 0)
                                   iColumn = next_[iColumn];
                              // sum all non-key variables
                              while(iColumn != stop) {
                                   assert (iColumn < 0);
                                   iColumn = -iColumn - 1;
                                   if ( iColumn != sequenceOut )
                                        oldB -= solution[iColumn];
                                   iColumn = next_[iColumn];
                              }
                         }
                         if (oldB)
                              ClpPackedMatrix::add(model, rhsOffset_, key, oldB);
                    }
               }
               if (sequenceOut < numberColumns) {
                    ClpPackedMatrix::add(model, rhsOffset_, sequenceOut, -solution[sequenceOut]);
                    if (iSetOut >= 0) {
                         // old contribution to rhsOffset_
                         int key = keyVariable_[iSetOut];
                         if (key < numberColumns && iSetIn != iSetOut) {
                              double oldB = 0.0;
                              ClpSimplex::Status iStatus = getStatus(iSetOut);
                              if (iStatus == ClpSimplex::atLowerBound)
                                   oldB = lower_[iSetOut];
                              else
                                   oldB = upper_[iSetOut];
                              // subtract out others at bounds
                              if ((gubType_ & 8) == 0) {
                                   int stop = -(key + 1);
                                   int iColumn = next_[key];
                                   // skip basic
                                   while (iColumn >= 0)
                                        iColumn = next_[iColumn];
                                   // sum all non-key variables
                                   while(iColumn != stop) {
                                        assert (iColumn < 0);
                                        iColumn = -iColumn - 1;
                                        if (iColumn == sequenceIn)
                                             oldB -= oldInValue;
                                        else if ( iColumn != sequenceOut )
                                             oldB -= solution[iColumn];
                                        iColumn = next_[iColumn];
                                   }
                              }
                              if (oldB)
                                   ClpPackedMatrix::add(model, rhsOffset_, key, oldB);
                         }
                    }
               } else if (sequenceOut < numberRows + numberColumns) {
                    //rhsOffset_[sequenceOut-numberColumns] -= -solution[sequenceOut];
               } else {
#ifdef CLP_DEBUG_PRINT
                    printf("** out is key slack %d\n", sequenceOut);
#endif
                    assert (pivotRow >= numberRows);
               }
          }
     }
     int * pivotVariable = model->pivotVariable();
     // may need to deal with key
     // Also need coding to mark/allow key slack entering
     if (pivotRow >= numberRows) {
          assert (sequenceOut >= numberRows + numberColumns || sequenceOut == keyVariable_[iSetOut]);
#ifdef CLP_DEBUG_PRINT
          if (sequenceIn >= numberRows + numberColumns)
               printf("key slack %d in, set out %d\n", gubSlackIn_, iSetOut);
          printf("** danger - key out for set %d in %d (set %d)\n", iSetOut, sequenceIn,
                 iSetIn);
#endif
          // if slack out mark correctly
          if (sequenceOut >= numberRows + numberColumns) {
               double value = model->valueOut();
               if (value == upper_[iSetOut]) {
                    setStatus(iSetOut, ClpSimplex::atUpperBound);
               } else if (value == lower_[iSetOut]) {
                    setStatus(iSetOut, ClpSimplex::atLowerBound);
               } else {
                    if (fabs(value - upper_[iSetOut]) <
                              fabs(value - lower_[iSetOut])) {
                         setStatus(iSetOut, ClpSimplex::atUpperBound);
                    } else {
                         setStatus(iSetOut, ClpSimplex::atLowerBound);
                    }
               }
               if (upper_[iSetOut] == lower_[iSetOut])
                    setStatus(iSetOut, ClpSimplex::isFixed);
               setFeasible(iSetOut);
          }
          if (iSetOut == iSetIn) {
               // key swap
               int key;
               if (sequenceIn >= numberRows + numberColumns) {
                    key = numberColumns + iSetIn;
                    setStatus(iSetIn, ClpSimplex::basic);
               } else {
                    key = sequenceIn;
               }
               redoSet(model, key, keyVariable_[iSetIn], iSetIn);
          } else {
               // key was chosen
               assert (possiblePivotKey_ >= 0 && possiblePivotKey_ < numberRows);
               int key = pivotVariable[possiblePivotKey_];
               // and set incoming here
               if (sequenceIn >= numberRows + numberColumns) {
                    // slack in - so use old key
                    sequenceIn = keyVariable_[iSetIn];
                    model->setStatus(sequenceIn, ClpSimplex::basic);
                    setStatus(iSetIn, ClpSimplex::basic);
                    redoSet(model, iSetIn + numberColumns, keyVariable_[iSetIn], iSetIn);
               }
               //? do not do if iSetIn<0 ? as will be done later
               pivotVariable[possiblePivotKey_] = sequenceIn;
               if (sequenceIn < numberColumns)
                    backToPivotRow_[sequenceIn] = possiblePivotKey_;
               redoSet(model, key, keyVariable_[iSetOut], iSetOut);
          }
     } else {
          if (sequenceOut < numberColumns) {
               if (iSetIn >= 0 && iSetOut == iSetIn) {
                    // key not out - only problem is if slack in
                    int key;
                    if (sequenceIn >= numberRows + numberColumns) {
                         key = numberColumns + iSetIn;
                         setStatus(iSetIn, ClpSimplex::basic);
                         assert (pivotRow < numberRows);
                         // must swap with current key
                         int key = keyVariable_[iSetIn];
                         model->setStatus(key, ClpSimplex::basic);
                         pivotVariable[pivotRow] = key;
                         backToPivotRow_[key] = pivotRow;
                    } else {
                         key = keyVariable_[iSetIn];
                    }
                    redoSet(model, key, keyVariable_[iSetIn], iSetIn);
               } else if (iSetOut >= 0) {
                    // just redo set
                    int key = keyVariable_[iSetOut];;
                    redoSet(model, key, keyVariable_[iSetOut], iSetOut);
               }
          }
     }
     if (iSetIn >= 0 && iSetIn != iSetOut) {
          int key = keyVariable_[iSetIn];
          if (sequenceIn == numberColumns + 2 * numberRows) {
               // key slack in
               assert (pivotRow < numberRows);
               // must swap with current key
               model->setStatus(key, ClpSimplex::basic);
               pivotVariable[pivotRow] = key;
               backToPivotRow_[key] = pivotRow;
               setStatus(iSetIn, ClpSimplex::basic);
               key = iSetIn + numberColumns;
          }
          // redo set to allow for new one
          redoSet(model, key, keyVariable_[iSetIn], iSetIn);
     }
     // update pivot
     if (sequenceIn < numberColumns) {
          if (pivotRow < numberRows) {
               backToPivotRow_[sequenceIn] = pivotRow;
          } else {
               if (possiblePivotKey_ >= 0) {
                    assert (possiblePivotKey_ < numberRows);
                    backToPivotRow_[sequenceIn] = possiblePivotKey_;
                    pivotVariable[possiblePivotKey_] = sequenceIn;
               }
          }
     } else if (sequenceIn >= numberRows + numberColumns) {
          // key in - something should have been done before
          int key = keyVariable_[iSetIn];
          assert (key == numberColumns + iSetIn);
          //pivotVariable[pivotRow]=key;
          //backToPivotRow_[key]=pivotRow;
          //model->setStatus(key,ClpSimplex::basic);
          //key=numberColumns+iSetIn;
          setStatus(iSetIn, ClpSimplex::basic);
          redoSet(model, key, keyVariable_[iSetIn], iSetIn);
     }
#ifdef CLP_DEBUG
     {
          char * xx = new char[numberColumns+numberRows];
          memset(xx, 0, numberRows + numberColumns);
          for (int i = 0; i < numberRows; i++) {
               int iPivot = pivotVariable[i];
               assert (iPivot < numberRows + numberColumns);
               assert (!xx[iPivot]);
               xx[iPivot] = 1;
               if (iPivot < numberColumns) {
                    int iBack = backToPivotRow_[iPivot];
                    assert (i == iBack);
               }
          }
          delete [] xx;
     }
#endif
     if (rhsOffset_) {
          // update effective rhs
          if (sequenceIn != sequenceOut) {
               if (sequenceIn < numberColumns) {
                    if (iSetIn >= 0) {
                         // new contribution to rhsOffset_
                         int key = keyVariable_[iSetIn];
                         if (key < numberColumns) {
                              double newB = 0.0;
                              ClpSimplex::Status iStatus = getStatus(iSetIn);
                              if (iStatus == ClpSimplex::atLowerBound)
                                   newB = lower_[iSetIn];
                              else
                                   newB = upper_[iSetIn];
                              // subtract out others at bounds
                              if ((gubType_ & 8) == 0) {
                                   int stop = -(key + 1);
                                   int iColumn = next_[key];
                                   // skip basic
                                   while (iColumn >= 0)
                                        iColumn = next_[iColumn];
                                   // sum all non-key variables
                                   while(iColumn != stop) {
                                        assert (iColumn < 0);
                                        iColumn = -iColumn - 1;
                                        newB -= solution[iColumn];
                                        iColumn = next_[iColumn];
                                   }
                              }
                              if (newB)
                                   ClpPackedMatrix::add(model, rhsOffset_, key, -newB);
                         }
                    }
               }
               if (iSetOut >= 0) {
                    // new contribution to rhsOffset_
                    int key = keyVariable_[iSetOut];
                    if (key < numberColumns && iSetIn != iSetOut) {
                         double newB = 0.0;
                         ClpSimplex::Status iStatus = getStatus(iSetOut);
                         if (iStatus == ClpSimplex::atLowerBound)
                              newB = lower_[iSetOut];
                         else
                              newB = upper_[iSetOut];
                         // subtract out others at bounds
                         if ((gubType_ & 8) == 0) {
                              int stop = -(key + 1);
                              int iColumn = next_[key];
                              // skip basic
                              while (iColumn >= 0)
                                   iColumn = next_[iColumn];
                              // sum all non-key variables
                              while(iColumn != stop) {
                                   assert (iColumn < 0);
                                   iColumn = -iColumn - 1;
                                   newB -= solution[iColumn];
                                   iColumn = next_[iColumn];
                              }
                         }
                         if (newB)
                              ClpPackedMatrix::add(model, rhsOffset_, key, -newB);
                    }
               }
          }
     }
#ifdef CLP_DEBUG
     // debug
     {
          int i;
          char * xxxx = new char[numberColumns];
          memset(xxxx, 0, numberColumns);
          for (i = 0; i < numberRows; i++) {
               int iPivot = pivotVariable[i];
               assert (model->getStatus(iPivot) == ClpSimplex::basic);
               if (iPivot < numberColumns && backward_[iPivot] >= 0)
                    xxxx[iPivot] = 1;
          }
          double primalTolerance = model->primalTolerance();
          for (i = 0; i < numberSets_; i++) {
               int key = keyVariable_[i];
               double value = 0.0;
               // sum over all except key
               int iColumn = next_[key];
               // sum all non-key variables
               int k = 0;
               int stop = -(key + 1);
               while (iColumn != stop) {
                    if (iColumn < 0)
                         iColumn = -iColumn - 1;
                    value += solution[iColumn];
                    k++;
                    assert (k < 100);
                    assert (backward_[iColumn] == i);
                    iColumn = next_[iColumn];
               }
               iColumn = next_[key];
               if (key < numberColumns) {
                    // feasibility will be done later
                    assert (getStatus(i) != ClpSimplex::basic);
                    double sol;
                    if (getStatus(i) == ClpSimplex::atUpperBound)
                         sol = upper_[i] - value;
                    else
                         sol = lower_[i] - value;
                    //printf("xx Value of key structural %d for set %d is %g - cost %g\n",key,i,sol,
                    //     cost[key]);
                    //if (fabs(sol-solution[key])>1.0e-3)
                    //printf("** stored value was %g\n",solution[key]);
               } else {
                    // slack is key
                    double infeasibility = 0.0;
                    if (value > upper_[i] + primalTolerance) {
                         infeasibility = value - upper_[i] - primalTolerance;
                         //setAbove(i);
                    } else if (value < lower_[i] - primalTolerance) {
                         infeasibility = lower_[i] - value - primalTolerance ;
                         //setBelow(i);
                    } else {
                         //setFeasible(i);
                    }
                    //printf("xx Value of key slack for set %d is %g\n",i,value);
               }
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
#endif
     return 0;
}
// Switches off dj checking each factorization (for BIG models)
void
ClpGubMatrix::switchOffCheck()
{
     noCheck_ = 0;
     infeasibilityWeight_ = 0.0;
}
// Correct sequence in and out to give true value
void
ClpGubMatrix::correctSequence(const ClpSimplex * /*model*/, int & sequenceIn, int & sequenceOut)
{
     if (sequenceIn != -999) {
          sequenceIn = trueSequenceIn_;
          sequenceOut = trueSequenceOut_;
     }
}
