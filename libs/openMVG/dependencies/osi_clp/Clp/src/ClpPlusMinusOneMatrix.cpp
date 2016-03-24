/* $Id$ */
// Copyright (C) 2003, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).


#include <cstdio>

#include "CoinPragma.hpp"
#include "CoinIndexedVector.hpp"
#include "CoinPackedVector.hpp"
#include "CoinHelperFunctions.hpp"

#include "ClpSimplex.hpp"
#include "ClpFactorization.hpp"
// at end to get min/max!
#include "ClpPlusMinusOneMatrix.hpp"
#include "ClpMessage.hpp"

//#############################################################################
// Constructors / Destructor / Assignment
//#############################################################################

//-------------------------------------------------------------------
// Default Constructor
//-------------------------------------------------------------------
ClpPlusMinusOneMatrix::ClpPlusMinusOneMatrix ()
     : ClpMatrixBase()
{
     setType(12);
     matrix_ = NULL;
     startPositive_ = NULL;
     startNegative_ = NULL;
     lengths_ = NULL;
     indices_ = NULL;
     numberRows_ = 0;
     numberColumns_ = 0;
     columnOrdered_ = true;
}

//-------------------------------------------------------------------
// Copy constructor
//-------------------------------------------------------------------
ClpPlusMinusOneMatrix::ClpPlusMinusOneMatrix (const ClpPlusMinusOneMatrix & rhs)
     : ClpMatrixBase(rhs)
{
     matrix_ = NULL;
     startPositive_ = NULL;
     startNegative_ = NULL;
     lengths_ = NULL;
     indices_ = NULL;
     numberRows_ = rhs.numberRows_;
     numberColumns_ = rhs.numberColumns_;
     columnOrdered_ = rhs.columnOrdered_;
     if (numberColumns_) {
          CoinBigIndex numberElements = rhs.startPositive_[numberColumns_];
          indices_ = new int [ numberElements];
          CoinMemcpyN(rhs.indices_, numberElements, indices_);
          startPositive_ = new CoinBigIndex [ numberColumns_+1];
          CoinMemcpyN(rhs.startPositive_, (numberColumns_ + 1), startPositive_);
          startNegative_ = new CoinBigIndex [ numberColumns_];
          CoinMemcpyN(rhs.startNegative_, numberColumns_, startNegative_);
     }
     int numberRows = getNumRows();
     if (rhs.rhsOffset_ && numberRows) {
          rhsOffset_ = ClpCopyOfArray(rhs.rhsOffset_, numberRows);
     } else {
          rhsOffset_ = NULL;
     }
}
// Constructor from arrays
ClpPlusMinusOneMatrix::ClpPlusMinusOneMatrix(int numberRows, int numberColumns,
          bool columnOrdered, const int * indices,
          const CoinBigIndex * startPositive,
          const CoinBigIndex * startNegative)
     : ClpMatrixBase()
{
     setType(12);
     matrix_ = NULL;
     lengths_ = NULL;
     numberRows_ = numberRows;
     numberColumns_ = numberColumns;
     columnOrdered_ = columnOrdered;
     int numberMajor = (columnOrdered_) ? numberColumns_ : numberRows_;
     int numberElements = startPositive[numberMajor];
     startPositive_ = ClpCopyOfArray(startPositive, numberMajor + 1);
     startNegative_ = ClpCopyOfArray(startNegative, numberMajor);
     indices_ = ClpCopyOfArray(indices, numberElements);
     // Check valid
     checkValid(false);
}

ClpPlusMinusOneMatrix::ClpPlusMinusOneMatrix (const CoinPackedMatrix & rhs)
     : ClpMatrixBase()
{
     setType(12);
     matrix_ = NULL;
     startPositive_ = NULL;
     startNegative_ = NULL;
     lengths_ = NULL;
     indices_ = NULL;
     int iColumn;
     assert (rhs.isColOrdered());
     // get matrix data pointers
     const int * row = rhs.getIndices();
     const CoinBigIndex * columnStart = rhs.getVectorStarts();
     const int * columnLength = rhs.getVectorLengths();
     const double * elementByColumn = rhs.getElements();
     numberColumns_ = rhs.getNumCols();
     numberRows_ = -1;
     indices_ = new int[rhs.getNumElements()];
     startPositive_ = new CoinBigIndex [numberColumns_+1];
     startNegative_ = new CoinBigIndex [numberColumns_];
     int * temp = new int [rhs.getNumRows()];
     CoinBigIndex j = 0;
     CoinBigIndex numberGoodP = 0;
     CoinBigIndex numberGoodM = 0;
     CoinBigIndex numberBad = 0;
     for (iColumn = 0; iColumn < numberColumns_; iColumn++) {
          CoinBigIndex k;
          int iNeg = 0;
          startPositive_[iColumn] = j;
          for (k = columnStart[iColumn]; k < columnStart[iColumn] + columnLength[iColumn];
                    k++) {
               int iRow;
               if (fabs(elementByColumn[k] - 1.0) < 1.0e-10) {
                    iRow = row[k];
                    numberRows_ = CoinMax(numberRows_, iRow);
                    indices_[j++] = iRow;
                    numberGoodP++;
               } else if (fabs(elementByColumn[k] + 1.0) < 1.0e-10) {
                    iRow = row[k];
                    numberRows_ = CoinMax(numberRows_, iRow);
                    temp[iNeg++] = iRow;
                    numberGoodM++;
               } else {
                    numberBad++;
               }
          }
          // move negative
          startNegative_[iColumn] = j;
          for (k = 0; k < iNeg; k++) {
               indices_[j++] = temp[k];
          }
     }
     startPositive_[numberColumns_] = j;
     delete [] temp;
     if (numberBad) {
          delete [] indices_;
          indices_ = NULL;
          numberRows_ = 0;
          numberColumns_ = 0;
          delete [] startPositive_;
          delete [] startNegative_;
          // Put in statistics
          startPositive_ = new CoinBigIndex [3];
          startPositive_[0] = numberGoodP;
          startPositive_[1] = numberGoodM;
          startPositive_[2] = numberBad;
          startNegative_ = NULL;
     } else {
          numberRows_ ++; //  correct
          // but number should be same as rhs
          assert (numberRows_ <= rhs.getNumRows());
          numberRows_ = rhs.getNumRows();
          columnOrdered_ = true;
     }
     // Check valid
     if (!numberBad)
          checkValid(false);
}

//-------------------------------------------------------------------
// Destructor
//-------------------------------------------------------------------
ClpPlusMinusOneMatrix::~ClpPlusMinusOneMatrix ()
{
     delete matrix_;
     delete [] startPositive_;
     delete [] startNegative_;
     delete [] lengths_;
     delete [] indices_;
}

//----------------------------------------------------------------
// Assignment operator
//-------------------------------------------------------------------
ClpPlusMinusOneMatrix &
ClpPlusMinusOneMatrix::operator=(const ClpPlusMinusOneMatrix& rhs)
{
     if (this != &rhs) {
          ClpMatrixBase::operator=(rhs);
          delete matrix_;
          delete [] startPositive_;
          delete [] startNegative_;
          delete [] lengths_;
          delete [] indices_;
          matrix_ = NULL;
          startPositive_ = NULL;
          lengths_ = NULL;
          indices_ = NULL;
          numberRows_ = rhs.numberRows_;
          numberColumns_ = rhs.numberColumns_;
          columnOrdered_ = rhs.columnOrdered_;
          if (numberColumns_) {
               CoinBigIndex numberElements = rhs.startPositive_[numberColumns_];
               indices_ = new int [ numberElements];
               CoinMemcpyN(rhs.indices_, numberElements, indices_);
               startPositive_ = new CoinBigIndex [ numberColumns_+1];
               CoinMemcpyN(rhs.startPositive_, (numberColumns_ + 1), startPositive_);
               startNegative_ = new CoinBigIndex [ numberColumns_];
               CoinMemcpyN(rhs.startNegative_, numberColumns_, startNegative_);
          }
     }
     return *this;
}
//-------------------------------------------------------------------
// Clone
//-------------------------------------------------------------------
ClpMatrixBase * ClpPlusMinusOneMatrix::clone() const
{
     return new ClpPlusMinusOneMatrix(*this);
}
/* Subset clone (without gaps).  Duplicates are allowed
   and order is as given */
ClpMatrixBase *
ClpPlusMinusOneMatrix::subsetClone (int numberRows, const int * whichRows,
                                    int numberColumns,
                                    const int * whichColumns) const
{
     return new ClpPlusMinusOneMatrix(*this, numberRows, whichRows,
                                      numberColumns, whichColumns);
}
/* Subset constructor (without gaps).  Duplicates are allowed
   and order is as given */
ClpPlusMinusOneMatrix::ClpPlusMinusOneMatrix (
     const ClpPlusMinusOneMatrix & rhs,
     int numberRows, const int * whichRow,
     int numberColumns, const int * whichColumn)
     : ClpMatrixBase(rhs)
{
     matrix_ = NULL;
     startPositive_ = NULL;
     startNegative_ = NULL;
     lengths_ = NULL;
     indices_ = NULL;
     numberRows_ = 0;
     numberColumns_ = 0;
     columnOrdered_ = rhs.columnOrdered_;
     if (numberRows <= 0 || numberColumns <= 0) {
          startPositive_ = new CoinBigIndex [1];
          startPositive_[0] = 0;
     } else {
          numberColumns_ = numberColumns;
          numberRows_ = numberRows;
          const int * index1 = rhs.indices_;
          CoinBigIndex * startPositive1 = rhs.startPositive_;

          int numberMinor = (!columnOrdered_) ? numberColumns_ : numberRows_;
          int numberMajor = (columnOrdered_) ? numberColumns_ : numberRows_;
          int numberMinor1 = (!columnOrdered_) ? rhs.numberColumns_ : rhs.numberRows_;
          int numberMajor1 = (columnOrdered_) ? rhs.numberColumns_ : rhs.numberRows_;
          // Also swap incoming if not column ordered
          if (!columnOrdered_) {
               int temp1 = numberRows;
               numberRows = numberColumns;
               numberColumns = temp1;
               const int * temp2;
               temp2 = whichRow;
               whichRow = whichColumn;
               whichColumn = temp2;
          }
          // Throw exception if rhs empty
          if (numberMajor1 <= 0 || numberMinor1 <= 0)
               throw CoinError("empty rhs", "subset constructor", "ClpPlusMinusOneMatrix");
          // Array to say if an old row is in new copy
          int * newRow = new int [numberMinor1];
          int iRow;
          for (iRow = 0; iRow < numberMinor1; iRow++)
               newRow[iRow] = -1;
          // and array for duplicating rows
          int * duplicateRow = new int [numberMinor];
          int numberBad = 0;
          for (iRow = 0; iRow < numberMinor; iRow++) {
               duplicateRow[iRow] = -1;
               int kRow = whichRow[iRow];
               if (kRow >= 0  && kRow < numberMinor1) {
                    if (newRow[kRow] < 0) {
                         // first time
                         newRow[kRow] = iRow;
                    } else {
                         // duplicate
                         int lastRow = newRow[kRow];
                         newRow[kRow] = iRow;
                         duplicateRow[iRow] = lastRow;
                    }
               } else {
                    // bad row
                    numberBad++;
               }
          }

          if (numberBad)
               throw CoinError("bad minor entries",
                               "subset constructor", "ClpPlusMinusOneMatrix");
          // now get size and check columns
          CoinBigIndex size = 0;
          int iColumn;
          numberBad = 0;
          for (iColumn = 0; iColumn < numberMajor; iColumn++) {
               int kColumn = whichColumn[iColumn];
               if (kColumn >= 0  && kColumn < numberMajor1) {
                    CoinBigIndex i;
                    for (i = startPositive1[kColumn]; i < startPositive1[kColumn+1]; i++) {
                         int kRow = index1[i];
                         kRow = newRow[kRow];
                         while (kRow >= 0) {
                              size++;
                              kRow = duplicateRow[kRow];
                         }
                    }
               } else {
                    // bad column
                    numberBad++;
                    printf("%d %d %d %d\n", iColumn, numberMajor, numberMajor1, kColumn);
               }
          }
          if (numberBad)
               throw CoinError("bad major entries",
                               "subset constructor", "ClpPlusMinusOneMatrix");
          // now create arrays
          startPositive_ = new CoinBigIndex [numberMajor+1];
          startNegative_ = new CoinBigIndex [numberMajor];
          indices_ = new int[size];
          // and fill them
          size = 0;
          startPositive_[0] = 0;
          CoinBigIndex * startNegative1 = rhs.startNegative_;
          for (iColumn = 0; iColumn < numberMajor; iColumn++) {
               int kColumn = whichColumn[iColumn];
               CoinBigIndex i;
               for (i = startPositive1[kColumn]; i < startNegative1[kColumn]; i++) {
                    int kRow = index1[i];
                    kRow = newRow[kRow];
                    while (kRow >= 0) {
                         indices_[size++] = kRow;
                         kRow = duplicateRow[kRow];
                    }
               }
               startNegative_[iColumn] = size;
               for (; i < startPositive1[kColumn+1]; i++) {
                    int kRow = index1[i];
                    kRow = newRow[kRow];
                    while (kRow >= 0) {
                         indices_[size++] = kRow;
                         kRow = duplicateRow[kRow];
                    }
               }
               startPositive_[iColumn+1] = size;
          }
          delete [] newRow;
          delete [] duplicateRow;
     }
     // Check valid
     checkValid(false);
}


/* Returns a new matrix in reverse order without gaps */
ClpMatrixBase *
ClpPlusMinusOneMatrix::reverseOrderedCopy() const
{
     int numberMinor = (!columnOrdered_) ? numberColumns_ : numberRows_;
     int numberMajor = (columnOrdered_) ? numberColumns_ : numberRows_;
     // count number in each row/column
     CoinBigIndex * tempP = new CoinBigIndex [numberMinor];
     CoinBigIndex * tempN = new CoinBigIndex [numberMinor];
     memset(tempP, 0, numberMinor * sizeof(CoinBigIndex));
     memset(tempN, 0, numberMinor * sizeof(CoinBigIndex));
     CoinBigIndex j = 0;
     int i;
     for (i = 0; i < numberMajor; i++) {
          for (; j < startNegative_[i]; j++) {
               int iRow = indices_[j];
               tempP[iRow]++;
          }
          for (; j < startPositive_[i+1]; j++) {
               int iRow = indices_[j];
               tempN[iRow]++;
          }
     }
     int * newIndices = new int [startPositive_[numberMajor]];
     CoinBigIndex * newP = new CoinBigIndex [numberMinor+1];
     CoinBigIndex * newN = new CoinBigIndex[numberMinor];
     int iRow;
     j = 0;
     // do starts
     for (iRow = 0; iRow < numberMinor; iRow++) {
          newP[iRow] = j;
          j += tempP[iRow];
          tempP[iRow] = newP[iRow];
          newN[iRow] = j;
          j += tempN[iRow];
          tempN[iRow] = newN[iRow];
     }
     newP[numberMinor] = j;
     j = 0;
     for (i = 0; i < numberMajor; i++) {
          for (; j < startNegative_[i]; j++) {
               int iRow = indices_[j];
               CoinBigIndex put = tempP[iRow];
               newIndices[put++] = i;
               tempP[iRow] = put;
          }
          for (; j < startPositive_[i+1]; j++) {
               int iRow = indices_[j];
               CoinBigIndex put = tempN[iRow];
               newIndices[put++] = i;
               tempN[iRow] = put;
          }
     }
     delete [] tempP;
     delete [] tempN;
     ClpPlusMinusOneMatrix * newCopy = new ClpPlusMinusOneMatrix();
     newCopy->passInCopy(numberMinor, numberMajor,
                         !columnOrdered_,  newIndices, newP, newN);
     return newCopy;
}
//unscaled versions
void
ClpPlusMinusOneMatrix::times(double scalar,
                             const double * x, double * y) const
{
     int numberMajor = (columnOrdered_) ? numberColumns_ : numberRows_;
     int i;
     CoinBigIndex j;
     assert (columnOrdered_);
     for (i = 0; i < numberMajor; i++) {
          double value = scalar * x[i];
          if (value) {
               for (j = startPositive_[i]; j < startNegative_[i]; j++) {
                    int iRow = indices_[j];
                    y[iRow] += value;
               }
               for (; j < startPositive_[i+1]; j++) {
                    int iRow = indices_[j];
                    y[iRow] -= value;
               }
          }
     }
}
void
ClpPlusMinusOneMatrix::transposeTimes(double scalar,
                                      const double * x, double * y) const
{
     int numberMajor = (columnOrdered_) ? numberColumns_ : numberRows_;
     int i;
     CoinBigIndex j = 0;
     assert (columnOrdered_);
     for (i = 0; i < numberMajor; i++) {
          double value = 0.0;
          for (; j < startNegative_[i]; j++) {
               int iRow = indices_[j];
               value += x[iRow];
          }
          for (; j < startPositive_[i+1]; j++) {
               int iRow = indices_[j];
               value -= x[iRow];
          }
          y[i] += scalar * value;
     }
}
void
ClpPlusMinusOneMatrix::times(double scalar,
                             const double * x, double * y,
                             const double * /*rowScale*/,
                             const double * /*columnScale*/) const
{
     // we know it is not scaled
     times(scalar, x, y);
}
void
ClpPlusMinusOneMatrix::transposeTimes( double scalar,
                                       const double * x, double * y,
                                       const double * /*rowScale*/,
                                       const double * /*columnScale*/,
                                       double * /*spare*/) const
{
     // we know it is not scaled
     transposeTimes(scalar, x, y);
}
/* Return <code>x * A + y</code> in <code>z</code>.
	Squashes small elements and knows about ClpSimplex */
void
ClpPlusMinusOneMatrix::transposeTimes(const ClpSimplex * model, double scalar,
                                      const CoinIndexedVector * rowArray,
                                      CoinIndexedVector * y,
                                      CoinIndexedVector * columnArray) const
{
     // we know it is not scaled
     columnArray->clear();
     double * pi = rowArray->denseVector();
     int numberNonZero = 0;
     int * index = columnArray->getIndices();
     double * array = columnArray->denseVector();
     int numberInRowArray = rowArray->getNumElements();
     // maybe I need one in OsiSimplex
     double zeroTolerance = model->zeroTolerance();
     int numberRows = model->numberRows();
     bool packed = rowArray->packedMode();
#ifndef NO_RTTI
     ClpPlusMinusOneMatrix* rowCopy =
          dynamic_cast< ClpPlusMinusOneMatrix*>(model->rowCopy());
#else
     ClpPlusMinusOneMatrix* rowCopy =
          static_cast< ClpPlusMinusOneMatrix*>(model->rowCopy());
#endif
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
     }
     if (numberInRowArray > factor * numberRows || !rowCopy) {
          assert (!y->getNumElements());
          // do by column
          // Need to expand if packed mode
          int iColumn;
          CoinBigIndex j = 0;
          assert (columnOrdered_);
          if (packed) {
               // need to expand pi into y
               assert(y->capacity() >= numberRows);
               double * piOld = pi;
               pi = y->denseVector();
               const int * whichRow = rowArray->getIndices();
               int i;
               // modify pi so can collapse to one loop
               for (i = 0; i < numberInRowArray; i++) {
                    int iRow = whichRow[i];
                    pi[iRow] = scalar * piOld[i];
               }
               for (iColumn = 0; iColumn < numberColumns_; iColumn++) {
                    double value = 0.0;
                    for (; j < startNegative_[iColumn]; j++) {
                         int iRow = indices_[j];
                         value += pi[iRow];
                    }
                    for (; j < startPositive_[iColumn+1]; j++) {
                         int iRow = indices_[j];
                         value -= pi[iRow];
                    }
                    if (fabs(value) > zeroTolerance) {
                         array[numberNonZero] = value;
                         index[numberNonZero++] = iColumn;
                    }
               }
               for (i = 0; i < numberInRowArray; i++) {
                    int iRow = whichRow[i];
                    pi[iRow] = 0.0;
               }
          } else {
               for (iColumn = 0; iColumn < numberColumns_; iColumn++) {
                    double value = 0.0;
                    for (; j < startNegative_[iColumn]; j++) {
                         int iRow = indices_[j];
                         value += pi[iRow];
                    }
                    for (; j < startPositive_[iColumn+1]; j++) {
                         int iRow = indices_[j];
                         value -= pi[iRow];
                    }
                    value *= scalar;
                    if (fabs(value) > zeroTolerance) {
                         index[numberNonZero++] = iColumn;
                         array[iColumn] = value;
                    }
               }
          }
          columnArray->setNumElements(numberNonZero);
     } else {
          // do by row
          rowCopy->transposeTimesByRow(model, scalar, rowArray, y, columnArray);
     }
}
/* Return <code>x * A + y</code> in <code>z</code>.
	Squashes small elements and knows about ClpSimplex */
void
ClpPlusMinusOneMatrix::transposeTimesByRow(const ClpSimplex * model, double scalar,
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
     const int * column = indices_;
     const CoinBigIndex * startPositive = startPositive_;
     const CoinBigIndex * startNegative = startNegative_;
     const int * whichRow = rowArray->getIndices();
     bool packed = rowArray->packedMode();
     if (numberInRowArray > 2) {
          // do by rows
          int iRow;
          double * markVector = y->denseVector(); // probably empty .. but
          int numberOriginal = 0;
          int i;
          if (packed) {
               numberNonZero = 0;
               // and set up mark as char array
               char * marked = reinterpret_cast<char *> (index + columnArray->capacity());
               double * array2 = y->denseVector();
#ifdef CLP_DEBUG
               int numberColumns = model->numberColumns();
               for (i = 0; i < numberColumns; i++) {
                    assert(!marked[i]);
                    assert(!array2[i]);
               }
#endif
               for (i = 0; i < numberInRowArray; i++) {
                    iRow = whichRow[i];
                    double value = pi[i] * scalar;
                    CoinBigIndex j;
                    for (j = startPositive[iRow]; j < startNegative[iRow]; j++) {
                         int iColumn = column[j];
                         if (!marked[iColumn]) {
                              marked[iColumn] = 1;
                              index[numberNonZero++] = iColumn;
                         }
                         array2[iColumn] += value;
                    }
                    for (j = startNegative[iRow]; j < startPositive[iRow+1]; j++) {
                         int iColumn = column[j];
                         if (!marked[iColumn]) {
                              marked[iColumn] = 1;
                              index[numberNonZero++] = iColumn;
                         }
                         array2[iColumn] -= value;
                    }
               }
               // get rid of tiny values and zero out marked
               numberOriginal = numberNonZero;
               numberNonZero = 0;
               for (i = 0; i < numberOriginal; i++) {
                    int iColumn = index[i];
                    if (marked[iColumn]) {
                         double value = array2[iColumn];
                         array2[iColumn] = 0.0;
                         marked[iColumn] = 0;
                         if (fabs(value) > zeroTolerance) {
                              array[numberNonZero] = value;
                              index[numberNonZero++] = iColumn;
                         }
                    }
               }
          } else {
               numberNonZero = 0;
               // and set up mark as char array
               char * marked = reinterpret_cast<char *> (markVector);
               for (i = 0; i < numberOriginal; i++) {
                    int iColumn = index[i];
                    marked[iColumn] = 0;
               }
               for (i = 0; i < numberInRowArray; i++) {
                    iRow = whichRow[i];
                    double value = pi[iRow] * scalar;
                    CoinBigIndex j;
                    for (j = startPositive[iRow]; j < startNegative[iRow]; j++) {
                         int iColumn = column[j];
                         if (!marked[iColumn]) {
                              marked[iColumn] = 1;
                              index[numberNonZero++] = iColumn;
                         }
                         array[iColumn] += value;
                    }
                    for (j = startNegative[iRow]; j < startPositive[iRow+1]; j++) {
                         int iColumn = column[j];
                         if (!marked[iColumn]) {
                              marked[iColumn] = 1;
                              index[numberNonZero++] = iColumn;
                         }
                         array[iColumn] -= value;
                    }
               }
               // get rid of tiny values and zero out marked
               numberOriginal = numberNonZero;
               numberNonZero = 0;
               for (i = 0; i < numberOriginal; i++) {
                    int iColumn = index[i];
                    marked[iColumn] = 0;
                    if (fabs(array[iColumn]) > zeroTolerance) {
                         index[numberNonZero++] = iColumn;
                    } else {
                         array[iColumn] = 0.0;
                    }
               }
          }
     } else if (numberInRowArray == 2) {
          /* do by rows when two rows (do longer first when not packed
             and shorter first if packed */
          int iRow0 = whichRow[0];
          int iRow1 = whichRow[1];
          CoinBigIndex j;
          if (packed) {
               double pi0 = pi[0];
               double pi1 = pi[1];
               if (startPositive[iRow0+1] - startPositive[iRow0] >
                         startPositive[iRow1+1] - startPositive[iRow1]) {
                    int temp = iRow0;
                    iRow0 = iRow1;
                    iRow1 = temp;
                    pi0 = pi1;
                    pi1 = pi[0];
               }
               // and set up mark as char array
               char * marked = reinterpret_cast<char *> (index + columnArray->capacity());
               int * lookup = y->getIndices();
               double value = pi0 * scalar;
               for (j = startPositive[iRow0]; j < startNegative[iRow0]; j++) {
                    int iColumn = column[j];
                    array[numberNonZero] = value;
                    marked[iColumn] = 1;
                    lookup[iColumn] = numberNonZero;
                    index[numberNonZero++] = iColumn;
               }
               for (j = startNegative[iRow0]; j < startPositive[iRow0+1]; j++) {
                    int iColumn = column[j];
                    array[numberNonZero] = -value;
                    marked[iColumn] = 1;
                    lookup[iColumn] = numberNonZero;
                    index[numberNonZero++] = iColumn;
               }
               int numberOriginal = numberNonZero;
               value = pi1 * scalar;
               for (j = startPositive[iRow1]; j < startNegative[iRow1]; j++) {
                    int iColumn = column[j];
                    if (marked[iColumn]) {
                         int iLookup = lookup[iColumn];
                         array[iLookup] += value;
                    } else {
                         if (fabs(value) > zeroTolerance) {
                              array[numberNonZero] = value;
                              index[numberNonZero++] = iColumn;
                         }
                    }
               }
               for (j = startNegative[iRow1]; j < startPositive[iRow1+1]; j++) {
                    int iColumn = column[j];
                    if (marked[iColumn]) {
                         int iLookup = lookup[iColumn];
                         array[iLookup] -= value;
                    } else {
                         if (fabs(value) > zeroTolerance) {
                              array[numberNonZero] = -value;
                              index[numberNonZero++] = iColumn;
                         }
                    }
               }
               // get rid of tiny values and zero out marked
               int nDelete = 0;
               for (j = 0; j < numberOriginal; j++) {
                    int iColumn = index[j];
                    marked[iColumn] = 0;
                    if (fabs(array[j]) <= zeroTolerance)
                         nDelete++;
               }
               if (nDelete) {
                    numberOriginal = numberNonZero;
                    numberNonZero = 0;
                    for (j = 0; j < numberOriginal; j++) {
                         int iColumn = index[j];
                         double value = array[j];
                         array[j] = 0.0;
                         if (fabs(value) > zeroTolerance) {
                              array[numberNonZero] = value;
                              index[numberNonZero++] = iColumn;
                         }
                    }
               }
          } else {
               if (startPositive[iRow0+1] - startPositive[iRow0] <
                         startPositive[iRow1+1] - startPositive[iRow1]) {
                    int temp = iRow0;
                    iRow0 = iRow1;
                    iRow1 = temp;
               }
               int numberOriginal;
               int i;
               numberNonZero = 0;
               double value;
               value = pi[iRow0] * scalar;
               CoinBigIndex j;
               for (j = startPositive[iRow0]; j < startNegative[iRow0]; j++) {
                    int iColumn = column[j];
                    index[numberNonZero++] = iColumn;
                    array[iColumn] = value;
               }
               for (j = startNegative[iRow0]; j < startPositive[iRow0+1]; j++) {
                    int iColumn = column[j];
                    index[numberNonZero++] = iColumn;
                    array[iColumn] = -value;
               }
               value = pi[iRow1] * scalar;
               for (j = startPositive[iRow1]; j < startNegative[iRow1]; j++) {
                    int iColumn = column[j];
                    double value2 = array[iColumn];
                    if (value2) {
                         value2 += value;
                    } else {
                         value2 = value;
                         index[numberNonZero++] = iColumn;
                    }
                    array[iColumn] = value2;
               }
               for (j = startNegative[iRow1]; j < startPositive[iRow1+1]; j++) {
                    int iColumn = column[j];
                    double value2 = array[iColumn];
                    if (value2) {
                         value2 -= value;
                    } else {
                         value2 = -value;
                         index[numberNonZero++] = iColumn;
                    }
                    array[iColumn] = value2;
               }
               // get rid of tiny values and zero out marked
               numberOriginal = numberNonZero;
               numberNonZero = 0;
               for (i = 0; i < numberOriginal; i++) {
                    int iColumn = index[i];
                    if (fabs(array[iColumn]) > zeroTolerance) {
                         index[numberNonZero++] = iColumn;
                    } else {
                         array[iColumn] = 0.0;
                    }
               }
          }
     } else if (numberInRowArray == 1) {
          // Just one row
          int iRow = rowArray->getIndices()[0];
          numberNonZero = 0;
          double value;
          iRow = whichRow[0];
          CoinBigIndex j;
          if (packed) {
               value = pi[0] * scalar;
               if (fabs(value) > zeroTolerance) {
                    for (j = startPositive[iRow]; j < startNegative[iRow]; j++) {
                         int iColumn = column[j];
                         array[numberNonZero] = value;
                         index[numberNonZero++] = iColumn;
                    }
                    for (j = startNegative[iRow]; j < startPositive[iRow+1]; j++) {
                         int iColumn = column[j];
                         array[numberNonZero] = -value;
                         index[numberNonZero++] = iColumn;
                    }
               }
          } else {
               value = pi[iRow] * scalar;
               if (fabs(value) > zeroTolerance) {
                    for (j = startPositive[iRow]; j < startNegative[iRow]; j++) {
                         int iColumn = column[j];
                         array[iColumn] = value;
                         index[numberNonZero++] = iColumn;
                    }
                    for (j = startNegative[iRow]; j < startPositive[iRow+1]; j++) {
                         int iColumn = column[j];
                         array[iColumn] = -value;
                         index[numberNonZero++] = iColumn;
                    }
               }
          }
     }
     columnArray->setNumElements(numberNonZero);
     if (packed)
          columnArray->setPacked();
     y->setNumElements(0);
}
/* Return <code>x *A in <code>z</code> but
   just for indices in y. */
void
ClpPlusMinusOneMatrix::subsetTransposeTimes(const ClpSimplex * ,
          const CoinIndexedVector * rowArray,
          const CoinIndexedVector * y,
          CoinIndexedVector * columnArray) const
{
     columnArray->clear();
     double * pi = rowArray->denseVector();
     double * array = columnArray->denseVector();
     int jColumn;
     int numberToDo = y->getNumElements();
     const int * which = y->getIndices();
     assert (!rowArray->packedMode());
     columnArray->setPacked();
     for (jColumn = 0; jColumn < numberToDo; jColumn++) {
          int iColumn = which[jColumn];
          double value = 0.0;
          CoinBigIndex j = startPositive_[iColumn];
          for (; j < startNegative_[iColumn]; j++) {
               int iRow = indices_[j];
               value += pi[iRow];
          }
          for (; j < startPositive_[iColumn+1]; j++) {
               int iRow = indices_[j];
               value -= pi[iRow];
          }
          array[jColumn] = value;
     }
}
/// returns number of elements in column part of basis,
CoinBigIndex
ClpPlusMinusOneMatrix::countBasis(const int * whichColumn,
                                  int & numberColumnBasic)
{
     int i;
     CoinBigIndex numberElements = 0;
     for (i = 0; i < numberColumnBasic; i++) {
          int iColumn = whichColumn[i];
          numberElements += startPositive_[iColumn+1] - startPositive_[iColumn];
     }
     return numberElements;
}
void
ClpPlusMinusOneMatrix::fillBasis(ClpSimplex * ,
                                 const int * whichColumn,
                                 int & numberColumnBasic,
                                 int * indexRowU, int * start,
                                 int * rowCount, int * columnCount,
                                 CoinFactorizationDouble * elementU)
{
     int i;
     CoinBigIndex numberElements = start[0];
     assert (columnOrdered_);
     for (i = 0; i < numberColumnBasic; i++) {
          int iColumn = whichColumn[i];
          CoinBigIndex j = startPositive_[iColumn];
          for (; j < startNegative_[iColumn]; j++) {
               int iRow = indices_[j];
               indexRowU[numberElements] = iRow;
               rowCount[iRow]++;
               elementU[numberElements++] = 1.0;
          }
          for (; j < startPositive_[iColumn+1]; j++) {
               int iRow = indices_[j];
               indexRowU[numberElements] = iRow;
               rowCount[iRow]++;
               elementU[numberElements++] = -1.0;
          }
          start[i+1] = numberElements;
          columnCount[i] = numberElements - start[i];
     }
}
/* Unpacks a column into an CoinIndexedvector
 */
void
ClpPlusMinusOneMatrix::unpack(const ClpSimplex * ,
                              CoinIndexedVector * rowArray,
                              int iColumn) const
{
     CoinBigIndex j = startPositive_[iColumn];
     for (; j < startNegative_[iColumn]; j++) {
          int iRow = indices_[j];
          rowArray->add(iRow, 1.0);
     }
     for (; j < startPositive_[iColumn+1]; j++) {
          int iRow = indices_[j];
          rowArray->add(iRow, -1.0);
     }
}
/* Unpacks a column into an CoinIndexedvector
** in packed foramt
Note that model is NOT const.  Bounds and objective could
be modified if doing column generation (just for this variable) */
void
ClpPlusMinusOneMatrix::unpackPacked(ClpSimplex * ,
                                    CoinIndexedVector * rowArray,
                                    int iColumn) const
{
     int * index = rowArray->getIndices();
     double * array = rowArray->denseVector();
     int number = 0;
     CoinBigIndex j = startPositive_[iColumn];
     for (; j < startNegative_[iColumn]; j++) {
          int iRow = indices_[j];
          array[number] = 1.0;
          index[number++] = iRow;
     }
     for (; j < startPositive_[iColumn+1]; j++) {
          int iRow = indices_[j];
          array[number] = -1.0;
          index[number++] = iRow;
     }
     rowArray->setNumElements(number);
     rowArray->setPackedMode(true);
}
/* Adds multiple of a column into an CoinIndexedvector
      You can use quickAdd to add to vector */
void
ClpPlusMinusOneMatrix::add(const ClpSimplex * , CoinIndexedVector * rowArray,
                           int iColumn, double multiplier) const
{
     CoinBigIndex j = startPositive_[iColumn];
     for (; j < startNegative_[iColumn]; j++) {
          int iRow = indices_[j];
          rowArray->quickAdd(iRow, multiplier);
     }
     for (; j < startPositive_[iColumn+1]; j++) {
          int iRow = indices_[j];
          rowArray->quickAdd(iRow, -multiplier);
     }
}
/* Adds multiple of a column into an array */
void
ClpPlusMinusOneMatrix::add(const ClpSimplex * , double * array,
                           int iColumn, double multiplier) const
{
     CoinBigIndex j = startPositive_[iColumn];
     for (; j < startNegative_[iColumn]; j++) {
          int iRow = indices_[j];
          array[iRow] += multiplier;
     }
     for (; j < startPositive_[iColumn+1]; j++) {
          int iRow = indices_[j];
          array[iRow] -= multiplier;
     }
}

// Return a complete CoinPackedMatrix
CoinPackedMatrix *
ClpPlusMinusOneMatrix::getPackedMatrix() const
{
     if (!matrix_) {
          int numberMinor = (!columnOrdered_) ? numberColumns_ : numberRows_;
          int numberMajor = (columnOrdered_) ? numberColumns_ : numberRows_;
          int numberElements = startPositive_[numberMajor];
          double * elements = new double [numberElements];
          CoinBigIndex j = 0;
          int i;
          for (i = 0; i < numberMajor; i++) {
               for (; j < startNegative_[i]; j++) {
                    elements[j] = 1.0;
               }
               for (; j < startPositive_[i+1]; j++) {
                    elements[j] = -1.0;
               }
          }
          matrix_ =  new CoinPackedMatrix(columnOrdered_, numberMinor, numberMajor,
                                          getNumElements(),
                                          elements, indices_,
                                          startPositive_, getVectorLengths());
          delete [] elements;
          delete [] lengths_;
          lengths_ = NULL;
     }
     return matrix_;
}
/* A vector containing the elements in the packed matrix. Note that there
   might be gaps in this list, entries that do not belong to any
   major-dimension vector. To get the actual elements one should look at
   this vector together with vectorStarts and vectorLengths. */
const double *
ClpPlusMinusOneMatrix::getElements() const
{
     if (!matrix_)
          getPackedMatrix();
     return matrix_->getElements();
}

const CoinBigIndex *
ClpPlusMinusOneMatrix::getVectorStarts() const
{
     return startPositive_;
}
/* The lengths of the major-dimension vectors. */
const int *
ClpPlusMinusOneMatrix::getVectorLengths() const
{
     if (!lengths_) {
          int numberMajor = (columnOrdered_) ? numberColumns_ : numberRows_;
          lengths_ = new int [numberMajor];
          int i;
          for (i = 0; i < numberMajor; i++) {
               lengths_[i] = startPositive_[i+1] - startPositive_[i];
          }
     }
     return lengths_;
}
/* Delete the columns whose indices are listed in <code>indDel</code>. */
void
ClpPlusMinusOneMatrix::deleteCols(const int numDel, const int * indDel)
{
     int iColumn;
     CoinBigIndex newSize = startPositive_[numberColumns_];;
     int numberBad = 0;
     // Use array to make sure we can have duplicates
     int * which = new int[numberColumns_];
     memset(which, 0, numberColumns_ * sizeof(int));
     int nDuplicate = 0;
     for (iColumn = 0; iColumn < numDel; iColumn++) {
          int jColumn = indDel[iColumn];
          if (jColumn < 0 || jColumn >= numberColumns_) {
               numberBad++;
          } else {
               newSize -= startPositive_[jColumn+1] - startPositive_[jColumn];
               if (which[jColumn])
                    nDuplicate++;
               else
                    which[jColumn] = 1;
          }
     }
     if (numberBad)
          throw CoinError("Indices out of range", "deleteCols", "ClpPlusMinusOneMatrix");
     int newNumber = numberColumns_ - numDel + nDuplicate;
     // Get rid of temporary arrays
     delete [] lengths_;
     lengths_ = NULL;
     delete matrix_;
     matrix_ = NULL;
     CoinBigIndex * newPositive = new CoinBigIndex [newNumber+1];
     CoinBigIndex * newNegative = new CoinBigIndex [newNumber];
     int * newIndices = new int [newSize];
     newNumber = 0;
     newSize = 0;
     for (iColumn = 0; iColumn < numberColumns_; iColumn++) {
          if (!which[iColumn]) {
               CoinBigIndex start, end;
               CoinBigIndex i;
               start = startPositive_[iColumn];
               end = startNegative_[iColumn];
               newPositive[newNumber] = newSize;
               for (i = start; i < end; i++)
                    newIndices[newSize++] = indices_[i];
               start = startNegative_[iColumn];
               end = startPositive_[iColumn+1];
               newNegative[newNumber++] = newSize;
               for (i = start; i < end; i++)
                    newIndices[newSize++] = indices_[i];
          }
     }
     newPositive[newNumber] = newSize;
     delete [] which;
     delete [] startPositive_;
     startPositive_ = newPositive;
     delete [] startNegative_;
     startNegative_ = newNegative;
     delete [] indices_;
     indices_ = newIndices;
     numberColumns_ = newNumber;
}
/* Delete the rows whose indices are listed in <code>indDel</code>. */
void
ClpPlusMinusOneMatrix::deleteRows(const int numDel, const int * indDel)
{
     int iRow;
     int numberBad = 0;
     // Use array to make sure we can have duplicates
     int * which = new int[numberRows_];
     memset(which, 0, numberRows_ * sizeof(int));
     int nDuplicate = 0;
     for (iRow = 0; iRow < numDel; iRow++) {
          int jRow = indDel[iRow];
          if (jRow < 0 || jRow >= numberRows_) {
               numberBad++;
          } else {
               if (which[jRow])
                    nDuplicate++;
               else
                    which[jRow] = 1;
          }
     }
     if (numberBad)
          throw CoinError("Indices out of range", "deleteRows", "ClpPlusMinusOneMatrix");
     CoinBigIndex iElement;
     CoinBigIndex numberElements = startPositive_[numberColumns_];
     CoinBigIndex newSize = 0;
     for (iElement = 0; iElement < numberElements; iElement++) {
          iRow = indices_[iElement];
          if (!which[iRow])
               newSize++;
     }
     int newNumber = numberRows_ - numDel + nDuplicate;
     // Get rid of temporary arrays
     delete [] lengths_;
     lengths_ = NULL;
     delete matrix_;
     matrix_ = NULL;
     int * newIndices = new int [newSize];
     newSize = 0;
     int iColumn;
     for (iColumn = 0; iColumn < numberColumns_; iColumn++) {
          CoinBigIndex start, end;
          CoinBigIndex i;
          start = startPositive_[iColumn];
          end = startNegative_[iColumn];
          startPositive_[newNumber] = newSize;
          for (i = start; i < end; i++) {
               iRow = indices_[i];
               if (!which[iRow])
                    newIndices[newSize++] = iRow;
          }
          start = startNegative_[iColumn];
          end = startPositive_[iColumn+1];
          startNegative_[newNumber] = newSize;
          for (i = start; i < end; i++) {
               iRow = indices_[i];
               if (!which[iRow])
                    newIndices[newSize++] = iRow;
          }
     }
     startPositive_[numberColumns_] = newSize;
     delete [] which;
     delete [] indices_;
     indices_ = newIndices;
     numberRows_ = newNumber;
}
bool
ClpPlusMinusOneMatrix::isColOrdered() const
{
     return columnOrdered_;
}
/* Number of entries in the packed matrix. */
CoinBigIndex
ClpPlusMinusOneMatrix::getNumElements() const
{
     int numberMajor = (columnOrdered_) ? numberColumns_ : numberRows_;
     if (startPositive_)
          return startPositive_[numberMajor];
     else
          return 0;
}
// pass in copy (object takes ownership)
void
ClpPlusMinusOneMatrix::passInCopy(int numberRows, int numberColumns,
                                  bool columnOrdered, int * indices,
                                  CoinBigIndex * startPositive, CoinBigIndex * startNegative)
{
     columnOrdered_ = columnOrdered;
     startPositive_ = startPositive;
     startNegative_ = startNegative;
     indices_ = indices;
     numberRows_ = numberRows;
     numberColumns_ = numberColumns;
     // Check valid
     checkValid(false);
}
// Just checks matrix valid - will say if dimensions not quite right if detail
void
ClpPlusMinusOneMatrix::checkValid(bool detail) const
{
     int maxIndex = -1;
     int minIndex = columnOrdered_ ? numberRows_ : numberColumns_;
     int number = !columnOrdered_ ? numberRows_ : numberColumns_;
     int numberElements = getNumElements();
     CoinBigIndex last = -1;
     int bad = 0;
     for (int i = 0; i < number; i++) {
          if(startPositive_[i] < last)
               bad++;
          else
               last = startPositive_[i];
          if(startNegative_[i] < last)
               bad++;
          else
               last = startNegative_[i];
     }
     if(startPositive_[number] < last)
          bad++;
     CoinAssertHint(!bad, "starts are not monotonic");
     for (CoinBigIndex cbi = 0; cbi < numberElements; cbi++) {
          maxIndex = CoinMax(indices_[cbi], maxIndex);
          minIndex = CoinMin(indices_[cbi], minIndex);
     }
     CoinAssert(maxIndex < (columnOrdered_ ? numberRows_ : numberColumns_));
     CoinAssert(minIndex >= 0);
     if (detail) {
          if (minIndex > 0 || maxIndex + 1 < (columnOrdered_ ? numberRows_ : numberColumns_))
               printf("Not full range of indices - %d to %d\n", minIndex, maxIndex);
     }
}
/* Given positive integer weights for each row fills in sum of weights
   for each column (and slack).
   Returns weights vector
*/
CoinBigIndex *
ClpPlusMinusOneMatrix::dubiousWeights(const ClpSimplex * model, int * inputWeights) const
{
     int numberRows = model->numberRows();
     int numberColumns = model->numberColumns();
     int number = numberRows + numberColumns;
     CoinBigIndex * weights = new CoinBigIndex[number];
     int i;
     for (i = 0; i < numberColumns; i++) {
          CoinBigIndex j;
          CoinBigIndex count = 0;
          for (j = startPositive_[i]; j < startPositive_[i+1]; j++) {
               int iRow = indices_[j];
               count += inputWeights[iRow];
          }
          weights[i] = count;
     }
     for (i = 0; i < numberRows; i++) {
          weights[i+numberColumns] = inputWeights[i];
     }
     return weights;
}
// Append Columns
void
ClpPlusMinusOneMatrix::appendCols(int number, const CoinPackedVectorBase * const * columns)
{
     int iColumn;
     CoinBigIndex size = 0;
     int numberBad = 0;
     for (iColumn = 0; iColumn < number; iColumn++) {
          int n = columns[iColumn]->getNumElements();
          const double * element = columns[iColumn]->getElements();
          size += n;
          int i;
          for (i = 0; i < n; i++) {
               if (fabs(element[i]) != 1.0)
                    numberBad++;
          }
     }
     if (numberBad)
          throw CoinError("Not +- 1", "appendCols", "ClpPlusMinusOneMatrix");
     // Get rid of temporary arrays
     delete [] lengths_;
     lengths_ = NULL;
     delete matrix_;
     matrix_ = NULL;
     int numberNow = startPositive_[numberColumns_];
     CoinBigIndex * temp;
     temp = new CoinBigIndex [numberColumns_+1+number];
     CoinMemcpyN(startPositive_, (numberColumns_ + 1), temp);
     delete [] startPositive_;
     startPositive_ = temp;
     temp = new CoinBigIndex [numberColumns_+number];
     CoinMemcpyN(startNegative_, numberColumns_, temp);
     delete [] startNegative_;
     startNegative_ = temp;
     int * temp2 = new int [numberNow+size];
     CoinMemcpyN(indices_, numberNow, temp2);
     delete [] indices_;
     indices_ = temp2;
     // now add
     size = numberNow;
     for (iColumn = 0; iColumn < number; iColumn++) {
          int n = columns[iColumn]->getNumElements();
          const int * row = columns[iColumn]->getIndices();
          const double * element = columns[iColumn]->getElements();
          int i;
          for (i = 0; i < n; i++) {
               if (element[i] == 1.0)
                    indices_[size++] = row[i];
          }
          startNegative_[iColumn+numberColumns_] = size;
          for (i = 0; i < n; i++) {
               if (element[i] == -1.0)
                    indices_[size++] = row[i];
          }
          startPositive_[iColumn+numberColumns_+1] = size;
     }

     numberColumns_ += number;
}
// Append Rows
void
ClpPlusMinusOneMatrix::appendRows(int number, const CoinPackedVectorBase * const * rows)
{
     // Allocate arrays to use for counting
     int * countPositive = new int [numberColumns_+1];
     memset(countPositive, 0, numberColumns_ * sizeof(int));
     int * countNegative = new int [numberColumns_];
     memset(countNegative, 0, numberColumns_ * sizeof(int));
     int iRow;
     CoinBigIndex size = 0;
     int numberBad = 0;
     for (iRow = 0; iRow < number; iRow++) {
          int n = rows[iRow]->getNumElements();
          const int * column = rows[iRow]->getIndices();
          const double * element = rows[iRow]->getElements();
          size += n;
          int i;
          for (i = 0; i < n; i++) {
               int iColumn = column[i];
               if (element[i] == 1.0)
                    countPositive[iColumn]++;
               else if (element[i] == -1.0)
                    countNegative[iColumn]++;
               else
                    numberBad++;
          }
     }
     if (numberBad)
          throw CoinError("Not +- 1", "appendRows", "ClpPlusMinusOneMatrix");
     // Get rid of temporary arrays
     delete [] lengths_;
     lengths_ = NULL;
     delete matrix_;
     matrix_ = NULL;
     int numberNow = startPositive_[numberColumns_];
     int * newIndices = new int [numberNow+size];
     // Update starts and turn counts into positions
     // also move current indices
     int iColumn;
     CoinBigIndex numberAdded = 0;
     for (iColumn = 0; iColumn < numberColumns_; iColumn++) {
          int n, move;
          CoinBigIndex now;
          now = startPositive_[iColumn];
          move = startNegative_[iColumn] - now;
          n = countPositive[iColumn];
          startPositive_[iColumn] += numberAdded;
          CoinMemcpyN(newIndices + startPositive_[iColumn], move, indices_ + now);
          countPositive[iColumn] = startNegative_[iColumn] + numberAdded;
          numberAdded += n;
          now = startNegative_[iColumn];
          move = startPositive_[iColumn+1] - now;
          n = countNegative[iColumn];
          startNegative_[iColumn] += numberAdded;
          CoinMemcpyN(newIndices + startNegative_[iColumn], move, indices_ + now);
          countNegative[iColumn] = startPositive_[iColumn+1] + numberAdded;
          numberAdded += n;
     }
     delete [] indices_;
     indices_ = newIndices;
     startPositive_[numberColumns_] += numberAdded;
     // Now put in
     for (iRow = 0; iRow < number; iRow++) {
          int newRow = numberRows_ + iRow;
          int n = rows[iRow]->getNumElements();
          const int * column = rows[iRow]->getIndices();
          const double * element = rows[iRow]->getElements();
          int i;
          for (i = 0; i < n; i++) {
               int iColumn = column[i];
               int put;
               if (element[i] == 1.0) {
                    put = countPositive[iColumn];
                    countPositive[iColumn] = put + 1;
               } else {
                    put = countNegative[iColumn];
                    countNegative[iColumn] = put + 1;
               }
               indices_[put] = newRow;
          }
     }
     delete [] countPositive;
     delete [] countNegative;
     numberRows_ += number;
}
/* Returns largest and smallest elements of both signs.
   Largest refers to largest absolute value.
*/
void
ClpPlusMinusOneMatrix::rangeOfElements(double & smallestNegative, double & largestNegative,
                                       double & smallestPositive, double & largestPositive)
{
     int iColumn;
     bool plusOne = false;
     bool minusOne = false;
     for (iColumn = 0; iColumn < numberColumns_; iColumn++) {
          if (startNegative_[iColumn] > startPositive_[iColumn])
               plusOne = true;
          if (startPositive_[iColumn+1] > startNegative_[iColumn])
               minusOne = true;
     }
     if (minusOne) {
          smallestNegative = -1.0;
          largestNegative = -1.0;
     } else {
          smallestNegative = 0.0;
          largestNegative = 0.0;
     }
     if (plusOne) {
          smallestPositive = 1.0;
          largestPositive = 1.0;
     } else {
          smallestPositive = 0.0;
          largestPositive = 0.0;
     }
}
// Says whether it can do partial pricing
bool
ClpPlusMinusOneMatrix::canDoPartialPricing() const
{
     return true;
}
// Partial pricing
void
ClpPlusMinusOneMatrix::partialPricing(ClpSimplex * model, double startFraction, double endFraction,
                                      int & bestSequence, int & numberWanted)
{
     numberWanted = currentWanted_;
     int start = static_cast<int> (startFraction * numberColumns_);
     int end = CoinMin(static_cast<int> (endFraction * numberColumns_ + 1), numberColumns_);
     CoinBigIndex j;
     double tolerance = model->currentDualTolerance();
     double * reducedCost = model->djRegion();
     const double * duals = model->dualRowSolution();
     const double * cost = model->costRegion();
     double bestDj;
     if (bestSequence >= 0)
          bestDj = fabs(reducedCost[bestSequence]);
     else
          bestDj = tolerance;
     int sequenceOut = model->sequenceOut();
     int saveSequence = bestSequence;
     int iSequence;
     for (iSequence = start; iSequence < end; iSequence++) {
          if (iSequence != sequenceOut) {
               double value;
               ClpSimplex::Status status = model->getStatus(iSequence);

               switch(status) {

               case ClpSimplex::basic:
               case ClpSimplex::isFixed:
                    break;
               case ClpSimplex::isFree:
               case ClpSimplex::superBasic:
                    value = cost[iSequence];
                    j = startPositive_[iSequence];
                    for (; j < startNegative_[iSequence]; j++) {
                         int iRow = indices_[j];
                         value -= duals[iRow];
                    }
                    for (; j < startPositive_[iSequence+1]; j++) {
                         int iRow = indices_[j];
                         value += duals[iRow];
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
                              } else {
                                   // just to make sure we don't exit before got something
                                   numberWanted++;
                              }
                         }
                    }
                    break;
               case ClpSimplex::atUpperBound:
                    value = cost[iSequence];
                    j = startPositive_[iSequence];
                    for (; j < startNegative_[iSequence]; j++) {
                         int iRow = indices_[j];
                         value -= duals[iRow];
                    }
                    for (; j < startPositive_[iSequence+1]; j++) {
                         int iRow = indices_[j];
                         value += duals[iRow];
                    }
                    if (value > tolerance) {
                         numberWanted--;
                         if (value > bestDj) {
                              // check flagged variable and correct dj
                              if (!model->flagged(iSequence)) {
                                   bestDj = value;
                                   bestSequence = iSequence;
                              } else {
                                   // just to make sure we don't exit before got something
                                   numberWanted++;
                              }
                         }
                    }
                    break;
               case ClpSimplex::atLowerBound:
                    value = cost[iSequence];
                    j = startPositive_[iSequence];
                    for (; j < startNegative_[iSequence]; j++) {
                         int iRow = indices_[j];
                         value -= duals[iRow];
                    }
                    for (; j < startPositive_[iSequence+1]; j++) {
                         int iRow = indices_[j];
                         value += duals[iRow];
                    }
                    value = -value;
                    if (value > tolerance) {
                         numberWanted--;
                         if (value > bestDj) {
                              // check flagged variable and correct dj
                              if (!model->flagged(iSequence)) {
                                   bestDj = value;
                                   bestSequence = iSequence;
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
          // recompute dj
          double value = cost[bestSequence];
          j = startPositive_[bestSequence];
          for (; j < startNegative_[bestSequence]; j++) {
               int iRow = indices_[j];
               value -= duals[iRow];
          }
          for (; j < startPositive_[bestSequence+1]; j++) {
               int iRow = indices_[j];
               value += duals[iRow];
          }
          reducedCost[bestSequence] = value;
          savedBestSequence_ = bestSequence;
          savedBestDj_ = reducedCost[savedBestSequence_];
     }
     currentWanted_ = numberWanted;
}
// Allow any parts of a created CoinMatrix to be deleted
void
ClpPlusMinusOneMatrix::releasePackedMatrix() const
{
     delete matrix_;
     delete [] lengths_;
     matrix_ = NULL;
     lengths_ = NULL;
}
/* Returns true if can combine transposeTimes and subsetTransposeTimes
   and if it would be faster */
bool
ClpPlusMinusOneMatrix::canCombine(const ClpSimplex * model,
                                  const CoinIndexedVector * pi) const
{
     int numberInRowArray = pi->getNumElements();
     int numberRows = model->numberRows();
     bool packed = pi->packedMode();
     // factor should be smaller if doing both with two pi vectors
     double factor = 0.27;
     // We may not want to do by row if there may be cache problems
     // It would be nice to find L2 cache size - for moment 512K
     // Be slightly optimistic
     if (numberColumns_ * sizeof(double) > 1000000) {
          if (numberRows * 10 < numberColumns_)
               factor *= 0.333333333;
          else if (numberRows * 4 < numberColumns_)
               factor *= 0.5;
          else if (numberRows * 2 < numberColumns_)
               factor *= 0.66666666667;
          //if (model->numberIterations()%50==0)
          //printf("%d nonzero\n",numberInRowArray);
     }
     // if not packed then bias a bit more towards by column
     if (!packed)
          factor *= 0.9;
     return (numberInRowArray > factor * numberRows || !model->rowCopy());
}
// These have to match ClpPrimalColumnSteepest version
#define reference(i)  (((reference[i>>5]>>(i&31))&1)!=0)
// Updates two arrays for steepest
void
ClpPlusMinusOneMatrix::transposeTimes2(const ClpSimplex * model,
                                       const CoinIndexedVector * pi1, CoinIndexedVector * dj1,
                                       const CoinIndexedVector * pi2,
                                       CoinIndexedVector * spare,
                                       double referenceIn, double devex,
                                       // Array for exact devex to say what is in reference framework
                                       unsigned int * reference,
                                       double * weights, double scaleFactor)
{
     // put row of tableau in dj1
     double * pi = pi1->denseVector();
     int numberNonZero = 0;
     int * index = dj1->getIndices();
     double * array = dj1->denseVector();
     int numberInRowArray = pi1->getNumElements();
     double zeroTolerance = model->zeroTolerance();
     bool packed = pi1->packedMode();
     // do by column
     int iColumn;
     assert (!spare->getNumElements());
     double * piWeight = pi2->denseVector();
     assert (!pi2->packedMode());
     bool killDjs = (scaleFactor == 0.0);
     if (!scaleFactor)
          scaleFactor = 1.0;
     // Note scale factor was -1.0
     if (packed) {
          // need to expand pi into y
          assert(spare->capacity() >= model->numberRows());
          double * piOld = pi;
          pi = spare->denseVector();
          const int * whichRow = pi1->getIndices();
          int i;
          // modify pi so can collapse to one loop
          for (i = 0; i < numberInRowArray; i++) {
               int iRow = whichRow[i];
               pi[iRow] = piOld[i];
          }
          CoinBigIndex j;
          for (iColumn = 0; iColumn < numberColumns_; iColumn++) {
               ClpSimplex::Status status = model->getStatus(iColumn);
               if (status == ClpSimplex::basic || status == ClpSimplex::isFixed) continue;
               double value = 0.0;
               for (j = startPositive_[iColumn]; j < startNegative_[iColumn]; j++) {
                    int iRow = indices_[j];
                    value -= pi[iRow];
               }
               for (; j < startPositive_[iColumn+1]; j++) {
                    int iRow = indices_[j];
                    value += pi[iRow];
               }
               if (fabs(value) > zeroTolerance) {
                    // and do other array
                    double modification = 0.0;
                    for (j = startPositive_[iColumn]; j < startNegative_[iColumn]; j++) {
                         int iRow = indices_[j];
                         modification += piWeight[iRow];
                    }
                    for (; j < startPositive_[iColumn+1]; j++) {
                         int iRow = indices_[j];
                         modification -= piWeight[iRow];
                    }
                    double thisWeight = weights[iColumn];
                    double pivot = value * scaleFactor;
                    double pivotSquared = pivot * pivot;
                    thisWeight += pivotSquared * devex + pivot * modification;
                    if (thisWeight < DEVEX_TRY_NORM) {
                         if (referenceIn < 0.0) {
                              // steepest
                              thisWeight = CoinMax(DEVEX_TRY_NORM, DEVEX_ADD_ONE + pivotSquared);
                         } else {
                              // exact
                              thisWeight = referenceIn * pivotSquared;
                              if (reference(iColumn))
                                   thisWeight += 1.0;
                              thisWeight = CoinMax(thisWeight, DEVEX_TRY_NORM);
                         }
                    }
                    weights[iColumn] = thisWeight;
                    if (!killDjs) {
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
          CoinBigIndex j;
          for (iColumn = 0; iColumn < numberColumns_; iColumn++) {
               ClpSimplex::Status status = model->getStatus(iColumn);
               if (status == ClpSimplex::basic || status == ClpSimplex::isFixed) continue;
               double value = 0.0;
               for (j = startPositive_[iColumn]; j < startNegative_[iColumn]; j++) {
                    int iRow = indices_[j];
                    value -= pi[iRow];
               }
               for (; j < startPositive_[iColumn+1]; j++) {
                    int iRow = indices_[j];
                    value += pi[iRow];
               }
               if (fabs(value) > zeroTolerance) {
                    // and do other array
                    double modification = 0.0;
                    for (j = startPositive_[iColumn]; j < startNegative_[iColumn]; j++) {
                         int iRow = indices_[j];
                         modification += piWeight[iRow];
                    }
                    for (; j < startPositive_[iColumn+1]; j++) {
                         int iRow = indices_[j];
                         modification -= piWeight[iRow];
                    }
                    double thisWeight = weights[iColumn];
                    double pivot = value * scaleFactor;
                    double pivotSquared = pivot * pivot;
                    thisWeight += pivotSquared * devex + pivot * modification;
                    if (thisWeight < DEVEX_TRY_NORM) {
                         if (referenceIn < 0.0) {
                              // steepest
                              thisWeight = CoinMax(DEVEX_TRY_NORM, DEVEX_ADD_ONE + pivotSquared);
                         } else {
                              // exact
                              thisWeight = referenceIn * pivotSquared;
                              if (reference(iColumn))
                                   thisWeight += 1.0;
                              thisWeight = CoinMax(thisWeight, DEVEX_TRY_NORM);
                         }
                    }
                    weights[iColumn] = thisWeight;
                    if (!killDjs) {
                         array[iColumn] = value;
                         index[numberNonZero++] = iColumn;
                    }
               }
          }
     }
     dj1->setNumElements(numberNonZero);
     spare->setNumElements(0);
     if (packed)
          dj1->setPackedMode(true);
}
// Updates second array for steepest and does devex weights
void
ClpPlusMinusOneMatrix::subsetTimes2(const ClpSimplex * ,
                                    CoinIndexedVector * dj1,
                                    const CoinIndexedVector * pi2, CoinIndexedVector *,
                                    double referenceIn, double devex,
                                    // Array for exact devex to say what is in reference framework
                                    unsigned int * reference,
                                    double * weights, double scaleFactor)
{
     int number = dj1->getNumElements();
     const int * index = dj1->getIndices();
     double * array = dj1->denseVector();
     assert( dj1->packedMode());

     double * piWeight = pi2->denseVector();
     bool killDjs = (scaleFactor == 0.0);
     if (!scaleFactor)
          scaleFactor = 1.0;
     for (int k = 0; k < number; k++) {
          int iColumn = index[k];
          double pivot = array[k] * scaleFactor;
          if (killDjs)
               array[k] = 0.0;
          // and do other array
          double modification = 0.0;
          CoinBigIndex j;
          for (j = startPositive_[iColumn]; j < startNegative_[iColumn]; j++) {
               int iRow = indices_[j];
               modification += piWeight[iRow];
          }
          for (; j < startPositive_[iColumn+1]; j++) {
               int iRow = indices_[j];
               modification -= piWeight[iRow];
          }
          double thisWeight = weights[iColumn];
          double pivotSquared = pivot * pivot;
          thisWeight += pivotSquared * devex + pivot * modification;
          if (thisWeight < DEVEX_TRY_NORM) {
               if (referenceIn < 0.0) {
                    // steepest
                    thisWeight = CoinMax(DEVEX_TRY_NORM, DEVEX_ADD_ONE + pivotSquared);
               } else {
                    // exact
                    thisWeight = referenceIn * pivotSquared;
                    if (reference(iColumn))
                         thisWeight += 1.0;
                    thisWeight = CoinMax(thisWeight, DEVEX_TRY_NORM);
               }
          }
          weights[iColumn] = thisWeight;
     }
}
/* Set the dimensions of the matrix. In effect, append new empty
   columns/rows to the matrix. A negative number for either dimension
   means that that dimension doesn't change. Otherwise the new dimensions
   MUST be at least as large as the current ones otherwise an exception
   is thrown. */
void
ClpPlusMinusOneMatrix::setDimensions(int newnumrows, int newnumcols)
{
     if (newnumrows < 0)
          newnumrows = numberRows_;
     if (newnumrows < numberRows_)
          throw CoinError("Bad new rownum (less than current)",
                          "setDimensions", "CoinPackedMatrix");

     if (newnumcols < 0)
          newnumcols = numberColumns_;
     if (newnumcols < numberColumns_)
          throw CoinError("Bad new colnum (less than current)",
                          "setDimensions", "CoinPackedMatrix");

     int number = 0;
     int length = 0;
     if (columnOrdered_) {
          length = numberColumns_;
          numberColumns_ = newnumcols;
          number = numberColumns_;

     } else {
          length = numberRows_;
          numberRows_ = newnumrows;
          number = numberRows_;
     }
     if (number > length) {
          CoinBigIndex * temp;
          int i;
          CoinBigIndex end = startPositive_[length];
          temp = new CoinBigIndex [number+1];
          CoinMemcpyN(startPositive_, (length + 1), temp);
          delete [] startPositive_;
          for (i = length + 1; i < number + 1; i++)
               temp[i] = end;
          startPositive_ = temp;
          temp = new CoinBigIndex [number];
          CoinMemcpyN(startNegative_, length, temp);
          delete [] startNegative_;
          for (i = length; i < number; i++)
               temp[i] = end;
          startNegative_ = temp;
     }
}
#ifndef SLIM_CLP
/* Append a set of rows/columns to the end of the matrix. Returns number of errors
   i.e. if any of the new rows/columns contain an index that's larger than the
   number of columns-1/rows-1 (if numberOther>0) or duplicates
   If 0 then rows, 1 if columns */
int
ClpPlusMinusOneMatrix::appendMatrix(int number, int type,
                                    const CoinBigIndex * starts, const int * index,
                                    const double * element, int /*numberOther*/)
{
     int numberErrors = 0;
     // make into CoinPackedVector
     CoinPackedVectorBase ** vectors =
          new CoinPackedVectorBase * [number];
     int iVector;
     for (iVector = 0; iVector < number; iVector++) {
          int iStart = starts[iVector];
          vectors[iVector] =
               new CoinPackedVector(starts[iVector+1] - iStart,
                                    index + iStart, element + iStart);
     }
     if (type == 0) {
          // rows
          appendRows(number, vectors);
     } else {
          // columns
          appendCols(number, vectors);
     }
     for (iVector = 0; iVector < number; iVector++)
          delete vectors[iVector];
     delete [] vectors;
     return numberErrors;
}
#endif
