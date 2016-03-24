/* $Id$ */
// Copyright (C) 2003, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).


#include <cstdio>

#include "CoinPragma.hpp"
#include "CoinIndexedVector.hpp"
#include "CoinHelperFunctions.hpp"
#include "CoinPackedVector.hpp"

#include "ClpSimplex.hpp"
#include "ClpFactorization.hpp"
// at end to get min/max!
#include "ClpNetworkMatrix.hpp"
#include "ClpPlusMinusOneMatrix.hpp"
#include "ClpMessage.hpp"
#include <iostream>
#include <cassert>

//#############################################################################
// Constructors / Destructor / Assignment
//#############################################################################

//-------------------------------------------------------------------
// Default Constructor
//-------------------------------------------------------------------
ClpNetworkMatrix::ClpNetworkMatrix ()
     : ClpMatrixBase()
{
     setType(11);
     matrix_ = NULL;
     lengths_ = NULL;
     indices_ = NULL;
     numberRows_ = 0;
     numberColumns_ = 0;
     trueNetwork_ = false;
}

/* Constructor from two arrays */
ClpNetworkMatrix::ClpNetworkMatrix(int numberColumns, const int * head,
                                   const int * tail)
     : ClpMatrixBase()
{
     setType(11);
     matrix_ = NULL;
     lengths_ = NULL;
     indices_ = new int[2*numberColumns];;
     numberRows_ = -1;
     numberColumns_ = numberColumns;
     trueNetwork_ = true;
     int iColumn;
     CoinBigIndex j = 0;
     for (iColumn = 0; iColumn < numberColumns_; iColumn++, j += 2) {
          int iRow = head[iColumn];
          numberRows_ = CoinMax(numberRows_, iRow);
          indices_[j] = iRow;
          iRow = tail[iColumn];
          numberRows_ = CoinMax(numberRows_, iRow);
          indices_[j+1] = iRow;
     }
     numberRows_++;
}
//-------------------------------------------------------------------
// Copy constructor
//-------------------------------------------------------------------
ClpNetworkMatrix::ClpNetworkMatrix (const ClpNetworkMatrix & rhs)
     : ClpMatrixBase(rhs)
{
     matrix_ = NULL;
     lengths_ = NULL;
     indices_ = NULL;
     numberRows_ = rhs.numberRows_;
     numberColumns_ = rhs.numberColumns_;
     trueNetwork_ = rhs.trueNetwork_;
     if (numberColumns_) {
          indices_ = new int [ 2*numberColumns_];
          CoinMemcpyN(rhs.indices_, 2 * numberColumns_, indices_);
     }
     int numberRows = getNumRows();
     if (rhs.rhsOffset_ && numberRows) {
          rhsOffset_ = ClpCopyOfArray(rhs.rhsOffset_, numberRows);
     } else {
          rhsOffset_ = NULL;
     }
}

ClpNetworkMatrix::ClpNetworkMatrix (const CoinPackedMatrix & rhs)
     : ClpMatrixBase()
{
     setType(11);
     matrix_ = NULL;
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
     int goodNetwork = 1;
     numberRows_ = -1;
     indices_ = new int[2*numberColumns_];
     CoinBigIndex j = 0;
     for (iColumn = 0; iColumn < numberColumns_; iColumn++, j += 2) {
          CoinBigIndex k = columnStart[iColumn];
          int iRow;
          switch (columnLength[iColumn]) {
          case 0:
               goodNetwork = -1; // not classic network
               indices_[j] = -1;
               indices_[j+1] = -1;
               break;

          case 1:
               goodNetwork = -1; // not classic network
               if (fabs(elementByColumn[k] - 1.0) < 1.0e-10) {
                    indices_[j] = -1;
                    iRow = row[k];
                    numberRows_ = CoinMax(numberRows_, iRow);
                    indices_[j+1] = iRow;
               } else if (fabs(elementByColumn[k] + 1.0) < 1.0e-10) {
                    indices_[j+1] = -1;
                    iRow = row[k];
                    numberRows_ = CoinMax(numberRows_, iRow);
                    indices_[j] = iRow;
               } else {
                    goodNetwork = 0; // not a network
               }
               break;

          case 2:
               if (fabs(elementByColumn[k] - 1.0) < 1.0e-10) {
                    if (fabs(elementByColumn[k+1] + 1.0) < 1.0e-10) {
                         iRow = row[k];
                         numberRows_ = CoinMax(numberRows_, iRow);
                         indices_[j+1] = iRow;
                         iRow = row[k+1];
                         numberRows_ = CoinMax(numberRows_, iRow);
                         indices_[j] = iRow;
                    } else {
                         goodNetwork = 0; // not a network
                    }
               } else if (fabs(elementByColumn[k] + 1.0) < 1.0e-10) {
                    if (fabs(elementByColumn[k+1] - 1.0) < 1.0e-10) {
                         iRow = row[k];
                         numberRows_ = CoinMax(numberRows_, iRow);
                         indices_[j] = iRow;
                         iRow = row[k+1];
                         numberRows_ = CoinMax(numberRows_, iRow);
                         indices_[j+1] = iRow;
                    } else {
                         goodNetwork = 0; // not a network
                    }
               } else {
                    goodNetwork = 0; // not a network
               }
               break;

          default:
               goodNetwork = 0; // not a network
               break;
          }
          if (!goodNetwork)
               break;
     }
     if (!goodNetwork) {
          delete [] indices_;
          // put in message
          printf("Not a network - can test if indices_ null\n");
          indices_ = NULL;
          numberRows_ = 0;
          numberColumns_ = 0;
     } else {
          numberRows_ ++; //  correct
          trueNetwork_ = goodNetwork > 0;
     }
}

//-------------------------------------------------------------------
// Destructor
//-------------------------------------------------------------------
ClpNetworkMatrix::~ClpNetworkMatrix ()
{
     delete matrix_;
     delete [] lengths_;
     delete [] indices_;
}

//----------------------------------------------------------------
// Assignment operator
//-------------------------------------------------------------------
ClpNetworkMatrix &
ClpNetworkMatrix::operator=(const ClpNetworkMatrix& rhs)
{
     if (this != &rhs) {
          ClpMatrixBase::operator=(rhs);
          delete matrix_;
          delete [] lengths_;
          delete [] indices_;
          matrix_ = NULL;
          lengths_ = NULL;
          indices_ = NULL;
          numberRows_ = rhs.numberRows_;
          numberColumns_ = rhs.numberColumns_;
          trueNetwork_ = rhs.trueNetwork_;
          if (numberColumns_) {
               indices_ = new int [ 2*numberColumns_];
               CoinMemcpyN(rhs.indices_, 2 * numberColumns_, indices_);
          }
     }
     return *this;
}
//-------------------------------------------------------------------
// Clone
//-------------------------------------------------------------------
ClpMatrixBase * ClpNetworkMatrix::clone() const
{
     return new ClpNetworkMatrix(*this);
}

/* Returns a new matrix in reverse order without gaps */
ClpMatrixBase *
ClpNetworkMatrix::reverseOrderedCopy() const
{
     // count number in each row
     CoinBigIndex * tempP = new CoinBigIndex [numberRows_];
     CoinBigIndex * tempN = new CoinBigIndex [numberRows_];
     memset(tempP, 0, numberRows_ * sizeof(CoinBigIndex));
     memset(tempN, 0, numberRows_ * sizeof(CoinBigIndex));
     CoinBigIndex j = 0;
     int i;
     for (i = 0; i < numberColumns_; i++, j += 2) {
          int iRow = indices_[j];
          tempN[iRow]++;
          iRow = indices_[j+1];
          tempP[iRow]++;
     }
     int * newIndices = new int [2*numberColumns_];
     CoinBigIndex * newP = new CoinBigIndex [numberRows_+1];
     CoinBigIndex * newN = new CoinBigIndex[numberRows_];
     int iRow;
     j = 0;
     // do starts
     for (iRow = 0; iRow < numberRows_; iRow++) {
          newP[iRow] = j;
          j += tempP[iRow];
          tempP[iRow] = newP[iRow];
          newN[iRow] = j;
          j += tempN[iRow];
          tempN[iRow] = newN[iRow];
     }
     newP[numberRows_] = j;
     j = 0;
     for (i = 0; i < numberColumns_; i++, j += 2) {
          int iRow = indices_[j];
          CoinBigIndex put = tempN[iRow];
          newIndices[put++] = i;
          tempN[iRow] = put;
          iRow = indices_[j+1];
          put = tempP[iRow];
          newIndices[put++] = i;
          tempP[iRow] = put;
     }
     delete [] tempP;
     delete [] tempN;
     ClpPlusMinusOneMatrix * newCopy = new ClpPlusMinusOneMatrix();
     newCopy->passInCopy(numberRows_, numberColumns_,
                         false,  newIndices, newP, newN);
     return newCopy;
}
//unscaled versions
void
ClpNetworkMatrix::times(double scalar,
                        const double * x, double * y) const
{
     int iColumn;
     CoinBigIndex j = 0;
     if (trueNetwork_) {
          for (iColumn = 0; iColumn < numberColumns_; iColumn++, j += 2) {
               double value = scalar * x[iColumn];
               if (value) {
                    int iRowM = indices_[j];
                    int iRowP = indices_[j+1];
                    y[iRowM] -= value;
                    y[iRowP] += value;
               }
          }
     } else {
          // skip negative rows
          for (iColumn = 0; iColumn < numberColumns_; iColumn++, j += 2) {
               double value = scalar * x[iColumn];
               if (value) {
                    int iRowM = indices_[j];
                    int iRowP = indices_[j+1];
                    if (iRowM >= 0)
                         y[iRowM] -= value;
                    if (iRowP >= 0)
                         y[iRowP] += value;
               }
          }
     }
}
void
ClpNetworkMatrix::transposeTimes(double scalar,
                                 const double * x, double * y) const
{
     int iColumn;
     CoinBigIndex j = 0;
     if (trueNetwork_) {
          for (iColumn = 0; iColumn < numberColumns_; iColumn++, j += 2) {
               double value = y[iColumn];
               int iRowM = indices_[j];
               int iRowP = indices_[j+1];
               value -= scalar * x[iRowM];
               value += scalar * x[iRowP];
               y[iColumn] = value;
          }
     } else {
          // skip negative rows
          for (iColumn = 0; iColumn < numberColumns_; iColumn++, j += 2) {
               double value = y[iColumn];
               int iRowM = indices_[j];
               int iRowP = indices_[j+1];
               if (iRowM >= 0)
                    value -= scalar * x[iRowM];
               if (iRowP >= 0)
                    value += scalar * x[iRowP];
               y[iColumn] = value;
          }
     }
}
void
ClpNetworkMatrix::times(double scalar,
                        const double * x, double * y,
                        const double * /*rowScale*/,
                        const double * /*columnScale*/) const
{
     // we know it is not scaled
     times(scalar, x, y);
}
void
ClpNetworkMatrix::transposeTimes( double scalar,
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
ClpNetworkMatrix::transposeTimes(const ClpSimplex * model, double scalar,
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
#ifndef NO_RTTI
     ClpPlusMinusOneMatrix* rowCopy =
          dynamic_cast< ClpPlusMinusOneMatrix*>(model->rowCopy());
#else
     ClpPlusMinusOneMatrix* rowCopy =
          static_cast< ClpPlusMinusOneMatrix*>(model->rowCopy());
#endif
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
     if (numberInRowArray > factor * numberRows || !rowCopy) {
          // do by column
          int iColumn;
          assert (!y->getNumElements());
          CoinBigIndex j = 0;
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
               if (trueNetwork_) {
                    for (iColumn = 0; iColumn < numberColumns_; iColumn++, j += 2) {
                         double value = 0.0;
                         int iRowM = indices_[j];
                         int iRowP = indices_[j+1];
                         value -= pi[iRowM];
                         value += pi[iRowP];
                         if (fabs(value) > zeroTolerance) {
                              array[numberNonZero] = value;
                              index[numberNonZero++] = iColumn;
                         }
                    }
               } else {
                    // skip negative rows
                    for (iColumn = 0; iColumn < numberColumns_; iColumn++, j += 2) {
                         double value = 0.0;
                         int iRowM = indices_[j];
                         int iRowP = indices_[j+1];
                         if (iRowM >= 0)
                              value -= pi[iRowM];
                         if (iRowP >= 0)
                              value += pi[iRowP];
                         if (fabs(value) > zeroTolerance) {
                              array[numberNonZero] = value;
                              index[numberNonZero++] = iColumn;
                         }
                    }
               }
               for (i = 0; i < numberInRowArray; i++) {
                    int iRow = whichRow[i];
                    pi[iRow] = 0.0;
               }
          } else {
               if (trueNetwork_) {
                    for (iColumn = 0; iColumn < numberColumns_; iColumn++, j += 2) {
                         double value = 0.0;
                         int iRowM = indices_[j];
                         int iRowP = indices_[j+1];
                         value -= scalar * pi[iRowM];
                         value += scalar * pi[iRowP];
                         if (fabs(value) > zeroTolerance) {
                              index[numberNonZero++] = iColumn;
                              array[iColumn] = value;
                         }
                    }
               } else {
                    // skip negative rows
                    for (iColumn = 0; iColumn < numberColumns_; iColumn++, j += 2) {
                         double value = 0.0;
                         int iRowM = indices_[j];
                         int iRowP = indices_[j+1];
                         if (iRowM >= 0)
                              value -= scalar * pi[iRowM];
                         if (iRowP >= 0)
                              value += scalar * pi[iRowP];
                         if (fabs(value) > zeroTolerance) {
                              index[numberNonZero++] = iColumn;
                              array[iColumn] = value;
                         }
                    }
               }
          }
          columnArray->setNumElements(numberNonZero);
     } else {
          // do by row
          rowCopy->transposeTimesByRow(model, scalar, rowArray, y, columnArray);
     }
}
/* Return <code>x *A in <code>z</code> but
   just for indices in y. */
void
ClpNetworkMatrix::subsetTransposeTimes(const ClpSimplex * /*model*/,
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
     if (trueNetwork_) {
          for (jColumn = 0; jColumn < numberToDo; jColumn++) {
               int iColumn = which[jColumn];
               double value = 0.0;
               CoinBigIndex j = iColumn << 1;
               int iRowM = indices_[j];
               int iRowP = indices_[j+1];
               value -= pi[iRowM];
               value += pi[iRowP];
               array[jColumn] = value;
          }
     } else {
          // skip negative rows
          for (jColumn = 0; jColumn < numberToDo; jColumn++) {
               int iColumn = which[jColumn];
               double value = 0.0;
               CoinBigIndex j = iColumn << 1;
               int iRowM = indices_[j];
               int iRowP = indices_[j+1];
               if (iRowM >= 0)
                    value -= pi[iRowM];
               if (iRowP >= 0)
                    value += pi[iRowP];
               array[jColumn] = value;
          }
     }
}
/// returns number of elements in column part of basis,
CoinBigIndex
ClpNetworkMatrix::countBasis( const int * whichColumn,
                              int & numberColumnBasic)
{
     int i;
     CoinBigIndex numberElements = 0;
     if (trueNetwork_) {
          numberElements = 2 * numberColumnBasic;
     } else {
          for (i = 0; i < numberColumnBasic; i++) {
               int iColumn = whichColumn[i];
               CoinBigIndex j = iColumn << 1;
               int iRowM = indices_[j];
               int iRowP = indices_[j+1];
               if (iRowM >= 0)
                    numberElements ++;
               if (iRowP >= 0)
                    numberElements ++;
          }
     }
     return numberElements;
}
void
ClpNetworkMatrix::fillBasis(ClpSimplex * /*model*/,
                            const int * whichColumn,
                            int & numberColumnBasic,
                            int * indexRowU, int * start,
                            int * rowCount, int * columnCount,
                            CoinFactorizationDouble * elementU)
{
     int i;
     CoinBigIndex numberElements = start[0];
     if (trueNetwork_) {
          for (i = 0; i < numberColumnBasic; i++) {
               int iColumn = whichColumn[i];
               CoinBigIndex j = iColumn << 1;
               int iRowM = indices_[j];
               int iRowP = indices_[j+1];
               indexRowU[numberElements] = iRowM;
               rowCount[iRowM]++;
               elementU[numberElements] = -1.0;
               indexRowU[numberElements+1] = iRowP;
               rowCount[iRowP]++;
               elementU[numberElements+1] = 1.0;
               numberElements += 2;
               start[i+1] = numberElements;
               columnCount[i] = 2;
          }
     } else {
          for (i = 0; i < numberColumnBasic; i++) {
               int iColumn = whichColumn[i];
               CoinBigIndex j = iColumn << 1;
               int iRowM = indices_[j];
               int iRowP = indices_[j+1];
               if (iRowM >= 0) {
                    indexRowU[numberElements] = iRowM;
                    rowCount[iRowM]++;
                    elementU[numberElements++] = -1.0;
               }
               if (iRowP >= 0) {
                    indexRowU[numberElements] = iRowP;
                    rowCount[iRowP]++;
                    elementU[numberElements++] = 1.0;
               }
               start[i+1] = numberElements;
               columnCount[i] = numberElements - start[i];
          }
     }
}
/* Unpacks a column into an CoinIndexedvector
 */
void
ClpNetworkMatrix::unpack(const ClpSimplex * /*model*/, CoinIndexedVector * rowArray,
                         int iColumn) const
{
     CoinBigIndex j = iColumn << 1;
     int iRowM = indices_[j];
     int iRowP = indices_[j+1];
     if (iRowM >= 0)
          rowArray->add(iRowM, -1.0);
     if (iRowP >= 0)
          rowArray->add(iRowP, 1.0);
}
/* Unpacks a column into an CoinIndexedvector
** in packed foramt
Note that model is NOT const.  Bounds and objective could
be modified if doing column generation (just for this variable) */
void
ClpNetworkMatrix::unpackPacked(ClpSimplex * /*model*/,
                               CoinIndexedVector * rowArray,
                               int iColumn) const
{
     int * index = rowArray->getIndices();
     double * array = rowArray->denseVector();
     int number = 0;
     CoinBigIndex j = iColumn << 1;
     int iRowM = indices_[j];
     int iRowP = indices_[j+1];
     if (iRowM >= 0) {
          array[number] = -1.0;
          index[number++] = iRowM;
     }
     if (iRowP >= 0) {
          array[number] = 1.0;
          index[number++] = iRowP;
     }
     rowArray->setNumElements(number);
     rowArray->setPackedMode(true);
}
/* Adds multiple of a column into an CoinIndexedvector
      You can use quickAdd to add to vector */
void
ClpNetworkMatrix::add(const ClpSimplex * /*model*/, CoinIndexedVector * rowArray,
                      int iColumn, double multiplier) const
{
     CoinBigIndex j = iColumn << 1;
     int iRowM = indices_[j];
     int iRowP = indices_[j+1];
     if (iRowM >= 0)
          rowArray->quickAdd(iRowM, -multiplier);
     if (iRowP >= 0)
          rowArray->quickAdd(iRowP, multiplier);
}
/* Adds multiple of a column into an array */
void
ClpNetworkMatrix::add(const ClpSimplex * /*model*/, double * array,
                      int iColumn, double multiplier) const
{
     CoinBigIndex j = iColumn << 1;
     int iRowM = indices_[j];
     int iRowP = indices_[j+1];
     if (iRowM >= 0)
          array[iRowM] -= multiplier;
     if (iRowP >= 0)
          array[iRowP] += multiplier;
}

// Return a complete CoinPackedMatrix
CoinPackedMatrix *
ClpNetworkMatrix::getPackedMatrix() const
{
     if (!matrix_) {
          assert (trueNetwork_); // fix later
          int numberElements = 2 * numberColumns_;
          double * elements = new double [numberElements];
          CoinBigIndex i;
          for (i = 0; i < 2 * numberColumns_; i += 2) {
               elements[i] = -1.0;
               elements[i+1] = 1.0;
          }
          CoinBigIndex * starts = new CoinBigIndex [numberColumns_+1];
          for (i = 0; i < numberColumns_ + 1; i++) {
               starts[i] = 2 * i;
          }
          // use assignMatrix to save space
          delete [] lengths_;
          lengths_ = NULL;
          matrix_ = new CoinPackedMatrix();
          int * indices = CoinCopyOfArray(indices_, 2 * numberColumns_);
          matrix_->assignMatrix(true, numberRows_, numberColumns_,
                                getNumElements(),
                                elements, indices,
                                starts, lengths_);
          assert(!elements);
          assert(!starts);
          assert (!indices);
          assert (!lengths_);
     }
     return matrix_;
}
/* A vector containing the elements in the packed matrix. Note that there
   might be gaps in this list, entries that do not belong to any
   major-dimension vector. To get the actual elements one should look at
   this vector together with vectorStarts and vectorLengths. */
const double *
ClpNetworkMatrix::getElements() const
{
     if (!matrix_)
          getPackedMatrix();
     return matrix_->getElements();
}

const CoinBigIndex *
ClpNetworkMatrix::getVectorStarts() const
{
     if (!matrix_)
          getPackedMatrix();
     return matrix_->getVectorStarts();
}
/* The lengths of the major-dimension vectors. */
const int *
ClpNetworkMatrix::getVectorLengths() const
{
     assert (trueNetwork_); // fix later
     if (!lengths_) {
          lengths_ = new int [numberColumns_];
          int i;
          for (i = 0; i < numberColumns_; i++) {
               lengths_[i] = 2;
          }
     }
     return lengths_;
}
/* Delete the columns whose indices are listed in <code>indDel</code>. */
void ClpNetworkMatrix::deleteCols(const int numDel, const int * indDel)
{
     assert (trueNetwork_);
     int iColumn;
     int numberBad = 0;
     // Use array to make sure we can have duplicates
     char * which = new char[numberColumns_];
     memset(which, 0, numberColumns_);
     int nDuplicate = 0;
     for (iColumn = 0; iColumn < numDel; iColumn++) {
          int jColumn = indDel[iColumn];
          if (jColumn < 0 || jColumn >= numberColumns_) {
               numberBad++;
          } else {
               if (which[jColumn])
                    nDuplicate++;
               else
                    which[jColumn] = 1;
          }
     }
     if (numberBad)
          throw CoinError("Indices out of range", "deleteCols", "ClpNetworkMatrix");
     int newNumber = numberColumns_ - numDel + nDuplicate;
     // Get rid of temporary arrays
     delete [] lengths_;
     lengths_ = NULL;
     delete matrix_;
     matrix_ = NULL;
     int newSize = 2 * newNumber;
     int * newIndices = new int [newSize];
     newSize = 0;
     for (iColumn = 0; iColumn < numberColumns_; iColumn++) {
          if (!which[iColumn]) {
               CoinBigIndex start;
               CoinBigIndex i;
               start = 2 * iColumn;
               for (i = start; i < start + 2; i++)
                    newIndices[newSize++] = indices_[i];
          }
     }
     delete [] which;
     delete [] indices_;
     indices_ = newIndices;
     numberColumns_ = newNumber;
}
/* Delete the rows whose indices are listed in <code>indDel</code>. */
void ClpNetworkMatrix::deleteRows(const int numDel, const int * indDel)
{
     int iRow;
     int numberBad = 0;
     // Use array to make sure we can have duplicates
     int * which = new int [numberRows_];
     memset(which, 0, numberRows_ * sizeof(int));
     for (iRow = 0; iRow < numDel; iRow++) {
          int jRow = indDel[iRow];
          if (jRow < 0 || jRow >= numberRows_) {
               numberBad++;
          } else {
               which[jRow] = 1;
          }
     }
     if (numberBad)
          throw CoinError("Indices out of range", "deleteRows", "ClpNetworkMatrix");
     // Only valid of all columns have 0 entries
     int iColumn;
     for (iColumn = 0; iColumn < numberColumns_; iColumn++) {
          CoinBigIndex start;
          CoinBigIndex i;
          start = 2 * iColumn;
          for (i = start; i < start + 2; i++) {
               int iRow = indices_[i];
               if (which[iRow])
                    numberBad++;
          }
     }
     if (numberBad)
          throw CoinError("Row has entries", "deleteRows", "ClpNetworkMatrix");
     int newNumber = 0;
     for (iRow = 0; iRow < numberRows_; iRow++) {
          if (!which[iRow])
               which[iRow] = newNumber++;
          else
               which[iRow] = -1;
     }
     for (iColumn = 0; iColumn < numberColumns_; iColumn++) {
          CoinBigIndex start;
          CoinBigIndex i;
          start = 2 * iColumn;
          for (i = start; i < start + 2; i++) {
               int iRow = indices_[i];
               indices_[i] = which[iRow];
          }
     }
     delete [] which;
     numberRows_ = newNumber;
}
/* Given positive integer weights for each row fills in sum of weights
   for each column (and slack).
   Returns weights vector
*/
CoinBigIndex *
ClpNetworkMatrix::dubiousWeights(const ClpSimplex * model, int * inputWeights) const
{
     int numberRows = model->numberRows();
     int numberColumns = model->numberColumns();
     int number = numberRows + numberColumns;
     CoinBigIndex * weights = new CoinBigIndex[number];
     int i;
     for (i = 0; i < numberColumns; i++) {
          CoinBigIndex j = i << 1;
          CoinBigIndex count = 0;
          int iRowM = indices_[j];
          int iRowP = indices_[j+1];
          if (iRowM >= 0) {
               count += inputWeights[iRowM];
          }
          if (iRowP >= 0) {
               count += inputWeights[iRowP];
          }
          weights[i] = count;
     }
     for (i = 0; i < numberRows; i++) {
          weights[i+numberColumns] = inputWeights[i];
     }
     return weights;
}
/* Returns largest and smallest elements of both signs.
   Largest refers to largest absolute value.
*/
void
ClpNetworkMatrix::rangeOfElements(double & smallestNegative, double & largestNegative,
                                  double & smallestPositive, double & largestPositive)
{
     smallestNegative = -1.0;
     largestNegative = -1.0;
     smallestPositive = 1.0;
     largestPositive = 1.0;
}
// Says whether it can do partial pricing
bool
ClpNetworkMatrix::canDoPartialPricing() const
{
     return true;
}
// Partial pricing
void
ClpNetworkMatrix::partialPricing(ClpSimplex * model, double startFraction, double endFraction,
                                 int & bestSequence, int & numberWanted)
{
     numberWanted = currentWanted_;
     int j;
     int start = static_cast<int> (startFraction * numberColumns_);
     int end = CoinMin(static_cast<int> (endFraction * numberColumns_ + 1), numberColumns_);
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
     if (!trueNetwork_) {
          // Not true network
          int iSequence;
          for (iSequence = start; iSequence < end; iSequence++) {
               if (iSequence != sequenceOut) {
                    double value;
                    int iRowM, iRowP;
                    ClpSimplex::Status status = model->getStatus(iSequence);

                    switch(status) {

                    case ClpSimplex::basic:
                    case ClpSimplex::isFixed:
                         break;
                    case ClpSimplex::isFree:
                    case ClpSimplex::superBasic:
                         value = cost[iSequence];
                         j = iSequence << 1;
                         // skip negative rows
                         iRowM = indices_[j];
                         iRowP = indices_[j+1];
                         if (iRowM >= 0)
                              value += duals[iRowM];
                         if (iRowP >= 0)
                              value -= duals[iRowP];
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
                         j = iSequence << 1;
                         // skip negative rows
                         iRowM = indices_[j];
                         iRowP = indices_[j+1];
                         if (iRowM >= 0)
                              value += duals[iRowM];
                         if (iRowP >= 0)
                              value -= duals[iRowP];
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
                         j = iSequence << 1;
                         // skip negative rows
                         iRowM = indices_[j];
                         iRowP = indices_[j+1];
                         if (iRowM >= 0)
                              value += duals[iRowM];
                         if (iRowP >= 0)
                              value -= duals[iRowP];
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
               j = bestSequence << 1;
               // skip negative rows
               int iRowM = indices_[j];
               int iRowP = indices_[j+1];
               if (iRowM >= 0)
                    value += duals[iRowM];
               if (iRowP >= 0)
                    value -= duals[iRowP];
               reducedCost[bestSequence] = value;
               savedBestSequence_ = bestSequence;
               savedBestDj_ = reducedCost[savedBestSequence_];
          }
     } else {
          // true network
          int iSequence;
          for (iSequence = start; iSequence < end; iSequence++) {
               if (iSequence != sequenceOut) {
                    double value;
                    int iRowM, iRowP;
                    ClpSimplex::Status status = model->getStatus(iSequence);

                    switch(status) {

                    case ClpSimplex::basic:
                    case ClpSimplex::isFixed:
                         break;
                    case ClpSimplex::isFree:
                    case ClpSimplex::superBasic:
                         value = cost[iSequence];
                         j = iSequence << 1;
                         iRowM = indices_[j];
                         iRowP = indices_[j+1];
                         value += duals[iRowM];
                         value -= duals[iRowP];
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
                         j = iSequence << 1;
                         iRowM = indices_[j];
                         iRowP = indices_[j+1];
                         value += duals[iRowM];
                         value -= duals[iRowP];
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
                         j = iSequence << 1;
                         iRowM = indices_[j];
                         iRowP = indices_[j+1];
                         value += duals[iRowM];
                         value -= duals[iRowP];
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
               j = bestSequence << 1;
               int iRowM = indices_[j];
               int iRowP = indices_[j+1];
               value += duals[iRowM];
               value -= duals[iRowP];
               reducedCost[bestSequence] = value;
               savedBestSequence_ = bestSequence;
               savedBestDj_ = reducedCost[savedBestSequence_];
          }
     }
     currentWanted_ = numberWanted;
}
// Allow any parts of a created CoinMatrix to be deleted
void
ClpNetworkMatrix::releasePackedMatrix() const
{
     delete matrix_;
     delete [] lengths_;
     matrix_ = NULL;
     lengths_ = NULL;
}
// Append Columns
void
ClpNetworkMatrix::appendCols(int number, const CoinPackedVectorBase * const * columns)
{
     int iColumn;
     int numberBad = 0;
     for (iColumn = 0; iColumn < number; iColumn++) {
          int n = columns[iColumn]->getNumElements();
          const double * element = columns[iColumn]->getElements();
          if (n != 2)
               numberBad++;
          if (fabs(element[0]) != 1.0 || fabs(element[1]) != 1.0)
               numberBad++;
          else if (element[0]*element[1] != -1.0)
               numberBad++;
     }
     if (numberBad)
          throw CoinError("Not network", "appendCols", "ClpNetworkMatrix");
     // Get rid of temporary arrays
     delete [] lengths_;
     lengths_ = NULL;
     delete matrix_;
     matrix_ = NULL;
     CoinBigIndex size = 2 * number;
     int * temp2 = new int [numberColumns_*2+size];
     CoinMemcpyN(indices_, numberColumns_ * 2, temp2);
     delete [] indices_;
     indices_ = temp2;
     // now add
     size = 2 * numberColumns_;
     for (iColumn = 0; iColumn < number; iColumn++) {
          const int * row = columns[iColumn]->getIndices();
          const double * element = columns[iColumn]->getElements();
          if (element[0] == -1.0) {
               indices_[size++] = row[0];
               indices_[size++] = row[1];
          } else {
               indices_[size++] = row[1];
               indices_[size++] = row[0];
          }
     }

     numberColumns_ += number;
}
// Append Rows
void
ClpNetworkMatrix::appendRows(int number, const CoinPackedVectorBase * const * rows)
{
     // must be zero arrays
     int numberBad = 0;
     int iRow;
     for (iRow = 0; iRow < number; iRow++) {
          numberBad += rows[iRow]->getNumElements();
     }
     if (numberBad)
          throw CoinError("Not NULL rows", "appendRows", "ClpNetworkMatrix");
     numberRows_ += number;
}
#ifndef SLIM_CLP
/* Append a set of rows/columns to the end of the matrix. Returns number of errors
   i.e. if any of the new rows/columns contain an index that's larger than the
   number of columns-1/rows-1 (if numberOther>0) or duplicates
   If 0 then rows, 1 if columns */
int
ClpNetworkMatrix::appendMatrix(int number, int type,
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
/* Subset clone (without gaps).  Duplicates are allowed
   and order is as given */
ClpMatrixBase *
ClpNetworkMatrix::subsetClone (int numberRows, const int * whichRows,
                               int numberColumns,
                               const int * whichColumns) const
{
     return new ClpNetworkMatrix(*this, numberRows, whichRows,
                                 numberColumns, whichColumns);
}
/* Subset constructor (without gaps).  Duplicates are allowed
   and order is as given */
ClpNetworkMatrix::ClpNetworkMatrix (
     const ClpNetworkMatrix & rhs,
     int numberRows, const int * whichRow,
     int numberColumns, const int * whichColumn)
     : ClpMatrixBase(rhs)
{
     setType(11);
     matrix_ = NULL;
     lengths_ = NULL;
     indices_ = new int[2*numberColumns];;
     numberRows_ = numberRows;
     numberColumns_ = numberColumns;
     trueNetwork_ = true;
     int iColumn;
     int numberBad = 0;
     int * which = new int [rhs.numberRows_];
     int iRow;
     for (iRow = 0; iRow < rhs.numberRows_; iRow++)
          which[iRow] = -1;
     int n = 0;
     for (iRow = 0; iRow < numberRows; iRow++) {
          int jRow = whichRow[iRow];
          assert (jRow >= 0 && jRow < rhs.numberRows_);
          which[jRow] = n++;
     }
     for (iColumn = 0; iColumn < numberColumns; iColumn++) {
          CoinBigIndex start;
          CoinBigIndex i;
          start = 2 * iColumn;
          CoinBigIndex offset = 2 * whichColumn[iColumn] - start;
          for (i = start; i < start + 2; i++) {
               int iRow = rhs.indices_[i+offset];
               iRow = which[iRow];
               if (iRow < 0)
                    numberBad++;
               else
                    indices_[i] = iRow;
          }
     }
     if (numberBad)
          throw CoinError("Invalid rows", "subsetConstructor", "ClpNetworkMatrix");
}
