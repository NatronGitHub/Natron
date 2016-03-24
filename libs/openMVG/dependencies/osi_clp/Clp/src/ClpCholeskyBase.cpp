/* $Id$ */
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

/*----------------------------------------------------------------------------*/
/*      Ordering code - courtesy of Anshul Gupta                              */
/*	(C) Copyright IBM Corporation 1997, 2009.  All Rights Reserved.       */
/*----------------------------------------------------------------------------*/

/*  A compact no-frills Approximate Minimum Local Fill ordering code.

    References:

[1] Ordering Sparse Matrices Using Approximate Minimum Local Fill.
    Edward Rothberg, SGI Manuscript, April 1996.
[2] An Approximate Minimum Degree Ordering Algorithm.
    T. Davis, P. Amestoy, and I. Duff, TR-94-039, CIS Department,
    University of Florida, December 1994.
*/
/*----------------------------------------------------------------------------*/


#include "CoinPragma.hpp"

#include <iostream>

#include "ClpCholeskyBase.hpp"
#include "ClpInterior.hpp"
#include "ClpHelperFunctions.hpp"
#include "CoinHelperFunctions.hpp"
#include "CoinSort.hpp"
#include "ClpCholeskyDense.hpp"
#include "ClpMessage.hpp"
#include "ClpQuadraticObjective.hpp"

//#############################################################################
// Constructors / Destructor / Assignment
//#############################################################################

//-------------------------------------------------------------------
// Default Constructor
//-------------------------------------------------------------------
ClpCholeskyBase::ClpCholeskyBase (int denseThreshold) :
     type_(0),
     doKKT_(false),
     goDense_(0.7),
     choleskyCondition_(0.0),
     model_(NULL),
     numberTrials_(),
     numberRows_(0),
     status_(0),
     rowsDropped_(NULL),
     permuteInverse_(NULL),
     permute_(NULL),
     numberRowsDropped_(0),
     sparseFactor_(NULL),
     choleskyStart_(NULL),
     choleskyRow_(NULL),
     indexStart_(NULL),
     diagonal_(NULL),
     workDouble_(NULL),
     link_(NULL),
     workInteger_(NULL),
     clique_(NULL),
     sizeFactor_(0),
     sizeIndex_(0),
     firstDense_(0),
     rowCopy_(NULL),
     whichDense_(NULL),
     denseColumn_(NULL),
     dense_(NULL),
     denseThreshold_(denseThreshold)
{
     memset(integerParameters_, 0, 64 * sizeof(int));
     memset(doubleParameters_, 0, 64 * sizeof(double));
}

//-------------------------------------------------------------------
// Copy constructor
//-------------------------------------------------------------------
ClpCholeskyBase::ClpCholeskyBase (const ClpCholeskyBase & rhs) :
     type_(rhs.type_),
     doKKT_(rhs.doKKT_),
     goDense_(rhs.goDense_),
     choleskyCondition_(rhs.choleskyCondition_),
     model_(rhs.model_),
     numberTrials_(rhs.numberTrials_),
     numberRows_(rhs.numberRows_),
     status_(rhs.status_),
     numberRowsDropped_(rhs.numberRowsDropped_)
{
     rowsDropped_ = ClpCopyOfArray(rhs.rowsDropped_, numberRows_);
     permuteInverse_ = ClpCopyOfArray(rhs.permuteInverse_, numberRows_);
     permute_ = ClpCopyOfArray(rhs.permute_, numberRows_);
     sizeFactor_ = rhs.sizeFactor_;
     sizeIndex_ = rhs.sizeIndex_;
     firstDense_ = rhs.firstDense_;
     sparseFactor_ = ClpCopyOfArray(rhs.sparseFactor_, rhs.sizeFactor_);
     choleskyStart_ = ClpCopyOfArray(rhs.choleskyStart_, numberRows_ + 1);
     indexStart_ = ClpCopyOfArray(rhs.indexStart_, numberRows_);
     choleskyRow_ = ClpCopyOfArray(rhs.choleskyRow_, sizeIndex_);
     diagonal_ = ClpCopyOfArray(rhs.diagonal_, numberRows_);
#if CLP_LONG_CHOLESKY!=1
     workDouble_ = ClpCopyOfArray(rhs.workDouble_, numberRows_);
#else
     // actually long double
     workDouble_ = reinterpret_cast<double *> (ClpCopyOfArray(reinterpret_cast<CoinWorkDouble *> (rhs.workDouble_), numberRows_));
#endif
     link_ = ClpCopyOfArray(rhs.link_, numberRows_);
     workInteger_ = ClpCopyOfArray(rhs.workInteger_, numberRows_);
     clique_ = ClpCopyOfArray(rhs.clique_, numberRows_);
     CoinMemcpyN(rhs.integerParameters_, 64, integerParameters_);
     CoinMemcpyN(rhs.doubleParameters_, 64, doubleParameters_);
     rowCopy_ = rhs.rowCopy_->clone();
     whichDense_ = NULL;
     denseColumn_ = NULL;
     dense_ = NULL;
     denseThreshold_ = rhs.denseThreshold_;
}

//-------------------------------------------------------------------
// Destructor
//-------------------------------------------------------------------
ClpCholeskyBase::~ClpCholeskyBase ()
{
     delete [] rowsDropped_;
     delete [] permuteInverse_;
     delete [] permute_;
     delete [] sparseFactor_;
     delete [] choleskyStart_;
     delete [] choleskyRow_;
     delete [] indexStart_;
     delete [] diagonal_;
     delete [] workDouble_;
     delete [] link_;
     delete [] workInteger_;
     delete [] clique_;
     delete rowCopy_;
     delete [] whichDense_;
     delete [] denseColumn_;
     delete dense_;
}

//----------------------------------------------------------------
// Assignment operator
//-------------------------------------------------------------------
ClpCholeskyBase &
ClpCholeskyBase::operator=(const ClpCholeskyBase& rhs)
{
     if (this != &rhs) {
          type_ = rhs.type_;
          doKKT_ = rhs.doKKT_;
          goDense_ = rhs.goDense_;
          choleskyCondition_ = rhs.choleskyCondition_;
          model_ = rhs.model_;
          numberTrials_ = rhs.numberTrials_;
          numberRows_ = rhs.numberRows_;
          status_ = rhs.status_;
          numberRowsDropped_ = rhs.numberRowsDropped_;
          delete [] rowsDropped_;
          delete [] permuteInverse_;
          delete [] permute_;
          delete [] sparseFactor_;
          delete [] choleskyStart_;
          delete [] choleskyRow_;
          delete [] indexStart_;
          delete [] diagonal_;
          delete [] workDouble_;
          delete [] link_;
          delete [] workInteger_;
          delete [] clique_;
          delete rowCopy_;
          delete [] whichDense_;
          delete [] denseColumn_;
          delete dense_;
          rowsDropped_ = ClpCopyOfArray(rhs.rowsDropped_, numberRows_);
          permuteInverse_ = ClpCopyOfArray(rhs.permuteInverse_, numberRows_);
          permute_ = ClpCopyOfArray(rhs.permute_, numberRows_);
          sizeFactor_ = rhs.sizeFactor_;
          sizeIndex_ = rhs.sizeIndex_;
          firstDense_ = rhs.firstDense_;
          sparseFactor_ = ClpCopyOfArray(rhs.sparseFactor_, rhs.sizeFactor_);
          choleskyStart_ = ClpCopyOfArray(rhs.choleskyStart_, numberRows_ + 1);
          choleskyRow_ = ClpCopyOfArray(rhs.choleskyRow_, rhs.sizeFactor_);
          indexStart_ = ClpCopyOfArray(rhs.indexStart_, numberRows_);
          choleskyRow_ = ClpCopyOfArray(rhs.choleskyRow_, sizeIndex_);
          diagonal_ = ClpCopyOfArray(rhs.diagonal_, numberRows_);
#if CLP_LONG_CHOLESKY!=1
          workDouble_ = ClpCopyOfArray(rhs.workDouble_, numberRows_);
#else
          // actually long double
          workDouble_ = reinterpret_cast<double *> (ClpCopyOfArray(reinterpret_cast<CoinWorkDouble *> (rhs.workDouble_), numberRows_));
#endif
          link_ = ClpCopyOfArray(rhs.link_, numberRows_);
          workInteger_ = ClpCopyOfArray(rhs.workInteger_, numberRows_);
          clique_ = ClpCopyOfArray(rhs.clique_, numberRows_);
          delete rowCopy_;
          rowCopy_ = rhs.rowCopy_->clone();
          whichDense_ = NULL;
          denseColumn_ = NULL;
          dense_ = NULL;
          denseThreshold_ = rhs.denseThreshold_;
     }
     return *this;
}
// reset numberRowsDropped and rowsDropped.
void
ClpCholeskyBase::resetRowsDropped()
{
     numberRowsDropped_ = 0;
     memset(rowsDropped_, 0, numberRows_);
}
/* Uses factorization to solve. - given as if KKT.
   region1 is rows+columns, region2 is rows */
void
ClpCholeskyBase::solveKKT (CoinWorkDouble * region1, CoinWorkDouble * region2, const CoinWorkDouble * diagonal,
                           CoinWorkDouble diagonalScaleFactor)
{
     if (!doKKT_) {
          int iColumn;
          int numberColumns = model_->numberColumns();
          int numberTotal = numberRows_ + numberColumns;
          CoinWorkDouble * region1Save = new CoinWorkDouble[numberTotal];
          for (iColumn = 0; iColumn < numberTotal; iColumn++) {
               region1[iColumn] *= diagonal[iColumn];
               region1Save[iColumn] = region1[iColumn];
          }
          multiplyAdd(region1 + numberColumns, numberRows_, -1.0, region2, 1.0);
          model_->clpMatrix()->times(1.0, region1, region2);
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
               unscale = diagonalScaleFactor / scale;
          } else {
               //effectively zero
               scale = 0.0;
               unscale = 0.0;
          }
          multiplyAdd(NULL, numberRows_, 0.0, region2, scale);
          solve(region2);
          multiplyAdd(NULL, numberRows_, 0.0, region2, unscale);
          multiplyAdd(region2, numberRows_, -1.0, region1 + numberColumns, 0.0);
          CoinZeroN(region1, numberColumns);
          model_->clpMatrix()->transposeTimes(1.0, region2, region1);
          for (iColumn = 0; iColumn < numberTotal; iColumn++)
               region1[iColumn] = region1[iColumn] * diagonal[iColumn] - region1Save[iColumn];
          delete [] region1Save;
     } else {
          // KKT
          int numberRowsModel = model_->numberRows();
          int numberColumns = model_->numberColumns();
          int numberTotal = numberColumns + numberRowsModel;
          CoinWorkDouble * array = new CoinWorkDouble [numberRows_];
          CoinMemcpyN(region1, numberTotal, array);
          CoinMemcpyN(region2, numberRowsModel, array + numberTotal);
          assert (numberRows_ >= numberRowsModel + numberTotal);
          solve(array);
          int iRow;
          for (iRow = 0; iRow < numberTotal; iRow++) {
               if (rowsDropped_[iRow] && CoinAbs(array[iRow]) > 1.0e-8) {
		 COIN_DETAIL_PRINT(printf("row region1 %d dropped %g\n", iRow, array[iRow]));
               }
          }
          for (; iRow < numberRows_; iRow++) {
               if (rowsDropped_[iRow] && CoinAbs(array[iRow]) > 1.0e-8) {
		 COIN_DETAIL_PRINT(printf("row region2 %d dropped %g\n", iRow, array[iRow]));
               }
          }
          CoinMemcpyN(array + numberTotal, numberRowsModel, region2);
          CoinMemcpyN(array, numberTotal, region1);
          delete [] array;
     }
}
//-------------------------------------------------------------------
// Clone
//-------------------------------------------------------------------
ClpCholeskyBase * ClpCholeskyBase::clone() const
{
     return new ClpCholeskyBase(*this);
}
// Forms ADAT - returns nonzero if not enough memory
int
ClpCholeskyBase::preOrder(bool lowerTriangular, bool includeDiagonal, bool doKKT)
{
     delete rowCopy_;
     rowCopy_ = model_->clpMatrix()->reverseOrderedCopy();
     if (!doKKT) {
          numberRows_ = model_->numberRows();
          rowsDropped_ = new char [numberRows_];
          memset(rowsDropped_, 0, numberRows_);
          numberRowsDropped_ = 0;
          // Space for starts
          choleskyStart_ = new CoinBigIndex[numberRows_+1];
          const CoinBigIndex * columnStart = model_->clpMatrix()->getVectorStarts();
          const int * columnLength = model_->clpMatrix()->getVectorLengths();
          const int * row = model_->clpMatrix()->getIndices();
          const CoinBigIndex * rowStart = rowCopy_->getVectorStarts();
          const int * rowLength = rowCopy_->getVectorLengths();
          const int * column = rowCopy_->getIndices();
          // We need two arrays for counts
          int * which = new int [numberRows_];
          int * used = new int[numberRows_+1];
          CoinZeroN(used, numberRows_);
          int iRow;
          sizeFactor_ = 0;
          int numberColumns = model_->numberColumns();
          int numberDense = 0;
          //denseThreshold_=3;
          if (denseThreshold_ > 0) {
               delete [] whichDense_;
               delete [] denseColumn_;
               delete dense_;
               whichDense_ = new char[numberColumns];
               int iColumn;
               used[numberRows_] = 0;
               for (iColumn = 0; iColumn < numberColumns; iColumn++) {
                    int length = columnLength[iColumn];
                    used[length] += 1;
               }
               int nLong = 0;
               int stop = CoinMax(denseThreshold_ / 2, 100);
               for (iRow = numberRows_; iRow >= stop; iRow--) {
                    if (used[iRow])
		      COIN_DETAIL_PRINT(printf("%d columns are of length %d\n", used[iRow], iRow));
                    nLong += used[iRow];
                    if (nLong > 50 || nLong > (numberColumns >> 2))
                         break;
               }
               CoinZeroN(used, numberRows_);
               for (iColumn = 0; iColumn < numberColumns; iColumn++) {
                    if (columnLength[iColumn] < denseThreshold_) {
                         whichDense_[iColumn] = 0;
                    } else {
                         whichDense_[iColumn] = 1;
                         numberDense++;
                    }
               }
               if (!numberDense || numberDense > 100) {
                    // free
                    delete [] whichDense_;
                    whichDense_ = NULL;
                    denseColumn_ = NULL;
                    dense_ = NULL;
               } else {
                    // space for dense columns
                    denseColumn_ = new longDouble [numberDense*numberRows_];
                    // dense cholesky
                    dense_ = new ClpCholeskyDense();
                    dense_->reserveSpace(NULL, numberDense);
                    COIN_DETAIL_PRINT(printf("Taking %d columns as dense\n", numberDense));
               }
          }
          int offset = includeDiagonal ? 0 : 1;
          if (lowerTriangular)
               offset = -offset;
          for (iRow = 0; iRow < numberRows_; iRow++) {
               int number = 0;
               // make sure diagonal exists if includeDiagonal
               if (!offset) {
                    which[0] = iRow;
                    used[iRow] = 1;
                    number = 1;
               }
               CoinBigIndex startRow = rowStart[iRow];
               CoinBigIndex endRow = rowStart[iRow] + rowLength[iRow];
               if (lowerTriangular) {
                    for (CoinBigIndex k = startRow; k < endRow; k++) {
                         int iColumn = column[k];
                         if (!whichDense_ || !whichDense_[iColumn]) {
                              CoinBigIndex start = columnStart[iColumn];
                              CoinBigIndex end = columnStart[iColumn] + columnLength[iColumn];
                              for (CoinBigIndex j = start; j < end; j++) {
                                   int jRow = row[j];
                                   if (jRow <= iRow + offset) {
                                        if (!used[jRow]) {
                                             used[jRow] = 1;
                                             which[number++] = jRow;
                                        }
                                   }
                              }
                         }
                    }
               } else {
                    for (CoinBigIndex k = startRow; k < endRow; k++) {
                         int iColumn = column[k];
                         if (!whichDense_ || !whichDense_[iColumn]) {
                              CoinBigIndex start = columnStart[iColumn];
                              CoinBigIndex end = columnStart[iColumn] + columnLength[iColumn];
                              for (CoinBigIndex j = start; j < end; j++) {
                                   int jRow = row[j];
                                   if (jRow >= iRow + offset) {
                                        if (!used[jRow]) {
                                             used[jRow] = 1;
                                             which[number++] = jRow;
                                        }
                                   }
                              }
                         }
                    }
               }
               sizeFactor_ += number;
               int j;
               for (j = 0; j < number; j++)
                    used[which[j]] = 0;
          }
          delete [] which;
          // Now we have size - create arrays and fill in
          try {
               choleskyRow_ = new int [sizeFactor_];
          } catch (...) {
               // no memory
               delete [] choleskyStart_;
               choleskyStart_ = NULL;
               return -1;
          }
          sizeFactor_ = 0;
          which = choleskyRow_;
          for (iRow = 0; iRow < numberRows_; iRow++) {
               int number = 0;
               // make sure diagonal exists if includeDiagonal
               if (!offset) {
                    which[0] = iRow;
                    used[iRow] = 1;
                    number = 1;
               }
               choleskyStart_[iRow] = sizeFactor_;
               CoinBigIndex startRow = rowStart[iRow];
               CoinBigIndex endRow = rowStart[iRow] + rowLength[iRow];
               if (lowerTriangular) {
                    for (CoinBigIndex k = startRow; k < endRow; k++) {
                         int iColumn = column[k];
                         if (!whichDense_ || !whichDense_[iColumn]) {
                              CoinBigIndex start = columnStart[iColumn];
                              CoinBigIndex end = columnStart[iColumn] + columnLength[iColumn];
                              for (CoinBigIndex j = start; j < end; j++) {
                                   int jRow = row[j];
                                   if (jRow <= iRow + offset) {
                                        if (!used[jRow]) {
                                             used[jRow] = 1;
                                             which[number++] = jRow;
                                        }
                                   }
                              }
                         }
                    }
               } else {
                    for (CoinBigIndex k = startRow; k < endRow; k++) {
                         int iColumn = column[k];
                         if (!whichDense_ || !whichDense_[iColumn]) {
                              CoinBigIndex start = columnStart[iColumn];
                              CoinBigIndex end = columnStart[iColumn] + columnLength[iColumn];
                              for (CoinBigIndex j = start; j < end; j++) {
                                   int jRow = row[j];
                                   if (jRow >= iRow + offset) {
                                        if (!used[jRow]) {
                                             used[jRow] = 1;
                                             which[number++] = jRow;
                                        }
                                   }
                              }
                         }
                    }
               }
               sizeFactor_ += number;
               int j;
               for (j = 0; j < number; j++)
                    used[which[j]] = 0;
               // Sort
               std::sort(which, which + number);
               // move which on
               which += number;
          }
          choleskyStart_[numberRows_] = sizeFactor_;
          delete [] used;
          return 0;
     } else {
          int numberRowsModel = model_->numberRows();
          int numberColumns = model_->numberColumns();
          int numberTotal = numberColumns + numberRowsModel;
          numberRows_ = 2 * numberRowsModel + numberColumns;
          rowsDropped_ = new char [numberRows_];
          memset(rowsDropped_, 0, numberRows_);
          numberRowsDropped_ = 0;
          CoinPackedMatrix * quadratic = NULL;
          ClpQuadraticObjective * quadraticObj =
               (dynamic_cast< ClpQuadraticObjective*>(model_->objectiveAsObject()));
          if (quadraticObj)
               quadratic = quadraticObj->quadraticObjective();
          int numberElements = model_->clpMatrix()->getNumElements();
          numberElements = numberElements + 2 * numberRowsModel + numberTotal;
          if (quadratic)
               numberElements += quadratic->getNumElements();
          // Space for starts
          choleskyStart_ = new CoinBigIndex[numberRows_+1];
          const CoinBigIndex * columnStart = model_->clpMatrix()->getVectorStarts();
          const int * columnLength = model_->clpMatrix()->getVectorLengths();
          const int * row = model_->clpMatrix()->getIndices();
          //const double * element = model_->clpMatrix()->getElements();
          // Now we have size - create arrays and fill in
          try {
               choleskyRow_ = new int [numberElements];
          } catch (...) {
               // no memory
               delete [] choleskyStart_;
               choleskyStart_ = NULL;
               return -1;
          }
          int iRow, iColumn;

          sizeFactor_ = 0;
          // matrix
          if (lowerTriangular) {
               if (!quadratic) {
                    for (iColumn = 0; iColumn < numberColumns; iColumn++) {
                         choleskyStart_[iColumn] = sizeFactor_;
                         choleskyRow_[sizeFactor_++] = iColumn;
                         CoinBigIndex start = columnStart[iColumn];
                         CoinBigIndex end = columnStart[iColumn] + columnLength[iColumn];
                         if (!includeDiagonal)
                              start++;
                         for (CoinBigIndex j = start; j < end; j++) {
                              choleskyRow_[sizeFactor_++] = row[j] + numberTotal;
                         }
                    }
               } else {
                    // Quadratic
                    const int * columnQuadratic = quadratic->getIndices();
                    const CoinBigIndex * columnQuadraticStart = quadratic->getVectorStarts();
                    const int * columnQuadraticLength = quadratic->getVectorLengths();
                    //const double * quadraticElement = quadratic->getElements();
                    for (iColumn = 0; iColumn < numberColumns; iColumn++) {
                         choleskyStart_[iColumn] = sizeFactor_;
                         if (includeDiagonal)
                              choleskyRow_[sizeFactor_++] = iColumn;
                         CoinBigIndex j;
                         for ( j = columnQuadraticStart[iColumn];
                                   j < columnQuadraticStart[iColumn] + columnQuadraticLength[iColumn]; j++) {
                              int jColumn = columnQuadratic[j];
                              if (jColumn > iColumn)
                                   choleskyRow_[sizeFactor_++] = jColumn;
                         }
                         CoinBigIndex start = columnStart[iColumn];
                         CoinBigIndex end = columnStart[iColumn] + columnLength[iColumn];
                         for ( j = start; j < end; j++) {
                              choleskyRow_[sizeFactor_++] = row[j] + numberTotal;
                         }
                    }
               }
               // slacks
               for (; iColumn < numberTotal; iColumn++) {
                    choleskyStart_[iColumn] = sizeFactor_;
                    if (includeDiagonal)
                         choleskyRow_[sizeFactor_++] = iColumn;
                    choleskyRow_[sizeFactor_++] = iColumn - numberColumns + numberTotal;
               }
               // Transpose - nonzero diagonal (may regularize)
               for (iRow = 0; iRow < numberRowsModel; iRow++) {
                    choleskyStart_[iRow+numberTotal] = sizeFactor_;
                    // diagonal
                    if (includeDiagonal)
                         choleskyRow_[sizeFactor_++] = iRow + numberTotal;
               }
               choleskyStart_[numberRows_] = sizeFactor_;
          } else {
               // transpose
               ClpMatrixBase * rowCopy = model_->clpMatrix()->reverseOrderedCopy();
               const CoinBigIndex * rowStart = rowCopy->getVectorStarts();
               const int * rowLength = rowCopy->getVectorLengths();
               const int * column = rowCopy->getIndices();
               if (!quadratic) {
                    for (iColumn = 0; iColumn < numberColumns; iColumn++) {
                         choleskyStart_[iColumn] = sizeFactor_;
                         if (includeDiagonal)
                              choleskyRow_[sizeFactor_++] = iColumn;
                    }
               } else {
                    // Quadratic
                    // transpose
                    CoinPackedMatrix quadraticT;
                    quadraticT.reverseOrderedCopyOf(*quadratic);
                    const int * columnQuadratic = quadraticT.getIndices();
                    const CoinBigIndex * columnQuadraticStart = quadraticT.getVectorStarts();
                    const int * columnQuadraticLength = quadraticT.getVectorLengths();
                    //const double * quadraticElement = quadraticT.getElements();
                    for (iColumn = 0; iColumn < numberColumns; iColumn++) {
                         choleskyStart_[iColumn] = sizeFactor_;
                         for (CoinBigIndex j = columnQuadraticStart[iColumn];
                                   j < columnQuadraticStart[iColumn] + columnQuadraticLength[iColumn]; j++) {
                              int jColumn = columnQuadratic[j];
                              if (jColumn < iColumn)
                                   choleskyRow_[sizeFactor_++] = jColumn;
                         }
                         if (includeDiagonal)
                              choleskyRow_[sizeFactor_++] = iColumn;
                    }
               }
               int iRow;
               // slacks
               for (iRow = 0; iRow < numberRowsModel; iRow++) {
                    choleskyStart_[iRow+numberColumns] = sizeFactor_;
                    if (includeDiagonal)
                         choleskyRow_[sizeFactor_++] = iRow + numberColumns;
               }
               for (iRow = 0; iRow < numberRowsModel; iRow++) {
                    choleskyStart_[iRow+numberTotal] = sizeFactor_;
                    CoinBigIndex start = rowStart[iRow];
                    CoinBigIndex end = rowStart[iRow] + rowLength[iRow];
                    for (CoinBigIndex j = start; j < end; j++) {
                         choleskyRow_[sizeFactor_++] = column[j];
                    }
                    // slack
                    choleskyRow_[sizeFactor_++] = numberColumns + iRow;
                    if (includeDiagonal)
                         choleskyRow_[sizeFactor_++] = iRow + numberTotal;
               }
               choleskyStart_[numberRows_] = sizeFactor_;
          }
     }
     return 0;
}
/* Orders rows and saves pointer to matrix.and model */
int
ClpCholeskyBase::order(ClpInterior * model)
{
     model_ = model;
#define BASE_ORDER 2
#if BASE_ORDER>0
     if (!doKKT_ && model_->numberRows() > 6) {
          if (preOrder(false, true, false))
               return -1;
          //rowsDropped_ = new char [numberRows_];
          numberRowsDropped_ = 0;
          memset(rowsDropped_, 0, numberRows_);
          //rowCopy_ = model->clpMatrix()->reverseOrderedCopy();
          // approximate minimum degree
          return orderAMD();
     }
#endif
     int numberRowsModel = model_->numberRows();
     int numberColumns = model_->numberColumns();
     int numberTotal = numberColumns + numberRowsModel;
     CoinPackedMatrix * quadratic = NULL;
     ClpQuadraticObjective * quadraticObj =
          (dynamic_cast< ClpQuadraticObjective*>(model_->objectiveAsObject()));
     if (quadraticObj)
          quadratic = quadraticObj->quadraticObjective();
     if (!doKKT_) {
          numberRows_ = model->numberRows();
     } else {
          numberRows_ = 2 * numberRowsModel + numberColumns;
     }
     rowsDropped_ = new char [numberRows_];
     numberRowsDropped_ = 0;
     memset(rowsDropped_, 0, numberRows_);
     rowCopy_ = model->clpMatrix()->reverseOrderedCopy();
     const CoinBigIndex * columnStart = model_->clpMatrix()->getVectorStarts();
     const int * columnLength = model_->clpMatrix()->getVectorLengths();
     const int * row = model_->clpMatrix()->getIndices();
     const CoinBigIndex * rowStart = rowCopy_->getVectorStarts();
     const int * rowLength = rowCopy_->getVectorLengths();
     const int * column = rowCopy_->getIndices();
     // We need arrays for counts
     int * which = new int [numberRows_];
     int * used = new int[numberRows_+1];
     int * count = new int[numberRows_];
     CoinZeroN(count, numberRows_);
     CoinZeroN(used, numberRows_);
     int iRow;
     sizeFactor_ = 0;
     permute_ = new int[numberRows_];
     for (iRow = 0; iRow < numberRows_; iRow++)
          permute_[iRow] = iRow;
     if (!doKKT_) {
          int numberDense = 0;
          if (denseThreshold_ > 0) {
               delete [] whichDense_;
               delete [] denseColumn_;
               delete dense_;
               whichDense_ = new char[numberColumns];
               int iColumn;
               used[numberRows_] = 0;
               for (iColumn = 0; iColumn < numberColumns; iColumn++) {
                    int length = columnLength[iColumn];
                    used[length] += 1;
               }
               int nLong = 0;
               int stop = CoinMax(denseThreshold_ / 2, 100);
               for (iRow = numberRows_; iRow >= stop; iRow--) {
                    if (used[iRow])
		      COIN_DETAIL_PRINT(printf("%d columns are of length %d\n", used[iRow], iRow));
                    nLong += used[iRow];
                    if (nLong > 50 || nLong > (numberColumns >> 2))
                         break;
               }
               CoinZeroN(used, numberRows_);
               for (iColumn = 0; iColumn < numberColumns; iColumn++) {
                    if (columnLength[iColumn] < denseThreshold_) {
                         whichDense_[iColumn] = 0;
                    } else {
                         whichDense_[iColumn] = 1;
                         numberDense++;
                    }
               }
               if (!numberDense || numberDense > 100) {
                    // free
                    delete [] whichDense_;
                    whichDense_ = NULL;
                    denseColumn_ = NULL;
                    dense_ = NULL;
               } else {
                    // space for dense columns
                    denseColumn_ = new longDouble [numberDense*numberRows_];
                    // dense cholesky
                    dense_ = new ClpCholeskyDense();
                    dense_->reserveSpace(NULL, numberDense);
                    COIN_DETAIL_PRINT(printf("Taking %d columns as dense\n", numberDense));
               }
          }
          /*
             Get row counts and size
          */
          for (iRow = 0; iRow < numberRows_; iRow++) {
               int number = 1;
               // make sure diagonal exists
               which[0] = iRow;
               used[iRow] = 1;
               CoinBigIndex startRow = rowStart[iRow];
               CoinBigIndex endRow = rowStart[iRow] + rowLength[iRow];
               for (CoinBigIndex k = startRow; k < endRow; k++) {
                    int iColumn = column[k];
                    if (!whichDense_ || !whichDense_[iColumn]) {
                         CoinBigIndex start = columnStart[iColumn];
                         CoinBigIndex end = columnStart[iColumn] + columnLength[iColumn];
                         for (CoinBigIndex j = start; j < end; j++) {
                              int jRow = row[j];
                              if (jRow < iRow) {
                                   if (!used[jRow]) {
                                        used[jRow] = 1;
                                        which[number++] = jRow;
                                        count[jRow]++;
                                   }
                              }
                         }
                    }
               }
               sizeFactor_ += number;
               count[iRow] += number;
               int j;
               for (j = 0; j < number; j++)
                    used[which[j]] = 0;
          }
          CoinSort_2(count, count + numberRows_, permute_);
     } else {
          // KKT
          int numberElements = model_->clpMatrix()->getNumElements();
          numberElements = numberElements + 2 * numberRowsModel + numberTotal;
          if (quadratic)
               numberElements += quadratic->getNumElements();
          // off diagonal
          numberElements -= numberRows_;
          sizeFactor_ = numberElements;
          // If we sort we need to redo symbolic etc
     }
     delete [] which;
     delete [] used;
     delete [] count;
     permuteInverse_ = new int [numberRows_];
     for (iRow = 0; iRow < numberRows_; iRow++) {
          //permute_[iRow]=iRow; // force no permute
          //permute_[iRow]=numberRows_-1-iRow; // force odd permute
          //permute_[iRow]=(iRow+1)%numberRows_; // force odd permute
          permuteInverse_[permute_[iRow]] = iRow;
     }
     return 0;
}
#if BASE_ORDER==1
/* Orders rows and saves pointer to matrix.and model */
int
ClpCholeskyBase::orderAMD()
{
     permuteInverse_ = new int [numberRows_];
     permute_ = new int[numberRows_];
     // Do ordering
     int returnCode = 0;
     // get more space and full matrix
     int space = 6 * sizeFactor_ + 100000;
     int * temp = new int [space];
     int * which = new int[2*numberRows_];
     CoinBigIndex * tempStart = new CoinBigIndex [numberRows_+1];
     memset(which, 0, numberRows_ * sizeof(int));
     for (int iRow = 0; iRow < numberRows_; iRow++) {
          which[iRow] += choleskyStart_[iRow+1] - choleskyStart_[iRow] - 1;
          for (CoinBigIndex j = choleskyStart_[iRow] + 1; j < choleskyStart_[iRow+1]; j++) {
               int jRow = choleskyRow_[j];
               which[jRow]++;
          }
     }
     CoinBigIndex sizeFactor = 0;
     for (int iRow = 0; iRow < numberRows_; iRow++) {
          int length = which[iRow];
          permute_[iRow] = length;
          tempStart[iRow] = sizeFactor;
          which[iRow] = sizeFactor;
          sizeFactor += length;
     }
     for (int iRow = 0; iRow < numberRows_; iRow++) {
          assert (choleskyRow_[choleskyStart_[iRow]] == iRow);
          for (CoinBigIndex j = choleskyStart_[iRow] + 1; j < choleskyStart_[iRow+1]; j++) {
               int jRow = choleskyRow_[j];
               int put = which[iRow];
               temp[put] = jRow;
               which[iRow]++;
               put = which[jRow];
               temp[put] = iRow;
               which[jRow]++;
          }
     }
     for (int iRow = 1; iRow < numberRows_; iRow++)
          assert (which[iRow-1] == tempStart[iRow]);
     CoinBigIndex lastSpace = sizeFactor;
     delete [] choleskyRow_;
     choleskyRow_ = temp;
     delete [] choleskyStart_;
     choleskyStart_ = tempStart;
     // linked lists of sizes and lengths
     int * first = new int [numberRows_];
     int * next = new int [numberRows_];
     int * previous = new int [numberRows_];
     char * mark = new char[numberRows_];
     memset(mark, 0, numberRows_);
     CoinBigIndex * sort = new CoinBigIndex [numberRows_];
     int * order = new int [numberRows_];
     for (int iRow = 0; iRow < numberRows_; iRow++) {
          first[iRow] = -1;
          next[iRow] = -1;
          previous[iRow] = -1;
          permuteInverse_[iRow] = -1;
     }
     int large = 1000 + 2 * numberRows_;
     for (int iRow = 0; iRow < numberRows_; iRow++) {
          int n = permute_[iRow];
          if (first[n] < 0) {
               first[n] = iRow;
               previous[iRow] = n + large;
               next[iRow] = n + 2 * large;
          } else {
               int k = first[n];
               assert (k < numberRows_);
               first[n] = iRow;
               previous[iRow] = n + large;
               assert (previous[k] == n + large);
               next[iRow] = k;
               previous[k] = iRow;
          }
     }
     int smallest = 0;
     int done = 0;
     int numberCompressions = 0;
     int numberExpansions = 0;
     while (smallest < numberRows_) {
          if (first[smallest] < 0 || first[smallest] > numberRows_) {
               smallest++;
               continue;
          }
          int iRow = first[smallest];
          if (false) {
               int kRow = -1;
               int ss = 999999;
               for (int jRow = numberRows_ - 1; jRow >= 0; jRow--) {
                    if (permuteInverse_[jRow] < 0) {
                         int length = permute_[jRow];
                         assert (length > 0);
                         for (CoinBigIndex j = choleskyStart_[jRow];
                                   j < choleskyStart_[jRow] + length; j++) {
                              int jjRow = choleskyRow_[j];
                              assert (permuteInverse_[jjRow] < 0);
                         }
                         if (length < ss) {
                              ss = length;
                              kRow = jRow;
                         }
                    }
               }
               assert (smallest == ss);
               printf("list chose %d - full chose %d - length %d\n",
                      iRow, kRow, ss);
          }
          int kNext = next[iRow];
          first[smallest] = kNext;
          if (kNext < numberRows_)
               previous[kNext] = previous[iRow];
          previous[iRow] = -1;
          next[iRow] = -1;
          permuteInverse_[iRow] = done;
          done++;
          // Now add edges
          CoinBigIndex start = choleskyStart_[iRow];
          CoinBigIndex end = choleskyStart_[iRow] + permute_[iRow];
          int nSave = 0;
          for (CoinBigIndex k = start; k < end; k++) {
               int kRow = choleskyRow_[k];
               assert (permuteInverse_[kRow] < 0);
               //if (permuteInverse_[kRow]<0)
               which[nSave++] = kRow;
          }
          for (int i = 0; i < nSave; i++) {
               int kRow = which[i];
               int length = permute_[kRow];
               CoinBigIndex start = choleskyStart_[kRow];
               CoinBigIndex end = choleskyStart_[kRow] + length;
               for (CoinBigIndex j = start; j < end; j++) {
                    int jRow = choleskyRow_[j];
                    mark[jRow] = 1;
               }
               mark[kRow] = 1;
               int n = nSave;
               for (int j = 0; j < nSave; j++) {
                    int jRow = which[j];
                    if (!mark[jRow]) {
                         which[n++] = jRow;
                    }
               }
               for (CoinBigIndex j = start; j < end; j++) {
                    int jRow = choleskyRow_[j];
                    mark[jRow] = 0;
               }
               mark[kRow] = 0;
               if (n > nSave) {
                    bool inPlace = (n - nSave == 1);
                    if (!inPlace) {
                         // extend
                         int length = n - nSave + end - start;
                         if (length + lastSpace > space) {
                              // need to compress
                              numberCompressions++;
                              int newN = 0;
                              for (int iRow = 0; iRow < numberRows_; iRow++) {
                                   CoinBigIndex start = choleskyStart_[iRow];
                                   if (permuteInverse_[iRow] < 0) {
                                        sort[newN] = start;
                                        order[newN++] = iRow;
                                   } else {
                                        choleskyStart_[iRow] = -1;
                                        permute_[iRow] = 0;
                                   }
                              }
                              CoinSort_2(sort, sort + newN, order);
                              sizeFactor = 0;
                              for (int k = 0; k < newN; k++) {
                                   int iRow = order[k];
                                   int length = permute_[iRow];
                                   CoinBigIndex start = choleskyStart_[iRow];
                                   choleskyStart_[iRow] = sizeFactor;
                                   for (int j = 0; j < length; j++)
                                        choleskyRow_[sizeFactor+j] = choleskyRow_[start+j];
                                   sizeFactor += length;
                              }
                              lastSpace = sizeFactor;
                              if ((sizeFactor + numberRows_ + 1000) * 4 > 3 * space) {
                                   // need to expand
                                   numberExpansions++;
                                   space = (3 * space) / 2;
                                   int * temp = new int [space];
                                   memcpy(temp, choleskyRow_, sizeFactor * sizeof(int));
                                   delete [] choleskyRow_;
                                   choleskyRow_ = temp;
                              }
                         }
                    }
                    // Now add
                    start = choleskyStart_[kRow];
                    end = choleskyStart_[kRow] + permute_[kRow];
                    if (!inPlace)
                         choleskyStart_[kRow] = lastSpace;
                    CoinBigIndex put = choleskyStart_[kRow];
                    for (CoinBigIndex j = start; j < end; j++) {
                         int jRow = choleskyRow_[j];
                         if (permuteInverse_[jRow] < 0)
                              choleskyRow_[put++] = jRow;
                    }
                    for (int j = nSave; j < n; j++) {
                         int jRow = which[j];
                         choleskyRow_[put++] = jRow;
                    }
                    if (!inPlace) {
                         permute_[kRow] = put - lastSpace;
                         lastSpace = put;
                    } else {
                         permute_[kRow] = put - choleskyStart_[kRow];
                    }
               } else {
                    // pack down for new counts
                    CoinBigIndex put = start;
                    for (CoinBigIndex j = start; j < end; j++) {
                         int jRow = choleskyRow_[j];
                         if (permuteInverse_[jRow] < 0)
                              choleskyRow_[put++] = jRow;
                    }
                    permute_[kRow] = put - start;
               }
               // take out
               int iNext = next[kRow];
               int iPrevious = previous[kRow];
               if (iPrevious < numberRows_) {
                    next[iPrevious] = iNext;
               } else {
                    assert (iPrevious == length + large);
                    first[length] = iNext;
               }
               if (iNext < numberRows_) {
                    previous[iNext] = iPrevious;
               } else {
                    assert (iNext == length + 2 * large);
               }
               // put in
               length = permute_[kRow];
               smallest = CoinMin(smallest, length);
               if (first[length] < 0 || first[length] > numberRows_) {
                    first[length] = kRow;
                    previous[kRow] = length + large;
                    next[kRow] = length + 2 * large;
               } else {
                    int k = first[length];
                    assert (k < numberRows_);
                    first[length] = kRow;
                    previous[kRow] = length + large;
                    assert (previous[k] == length + large);
                    next[kRow] = k;
                    previous[k] = kRow;
               }
          }
     }
     // do rest
     for (int iRow = 0; iRow < numberRows_; iRow++) {
          if (permuteInverse_[iRow] < 0)
               permuteInverse_[iRow] = done++;
     }
     COIN_DETAIL_PRINT(printf("%d compressions, %d expansions\n",
			      numberCompressions, numberExpansions));
     assert (done == numberRows_);
     delete [] sort;
     delete [] order;
     delete [] which;
     delete [] mark;
     delete [] first;
     delete [] next;
     delete [] previous;
     delete [] choleskyRow_;
     choleskyRow_ = NULL;
     delete [] choleskyStart_;
     choleskyStart_ = NULL;
#ifndef NDEBUG
     for (int iRow = 0; iRow < numberRows_; iRow++) {
          permute_[iRow] = -1;
     }
#endif
     for (int iRow = 0; iRow < numberRows_; iRow++) {
          permute_[permuteInverse_[iRow]] = iRow;
     }
#ifndef NDEBUG
     for (int iRow = 0; iRow < numberRows_; iRow++) {
          assert(permute_[iRow] >= 0 && permute_[iRow] < numberRows_);
     }
#endif
     return returnCode;
}
#elif BASE_ORDER==2
/*----------------------------------------------------------------------------*/
/*	(C) Copyright IBM Corporation 1997, 2009.  All Rights Reserved.       */
/*----------------------------------------------------------------------------*/

/*  A compact no-frills Approximate Minimum Local Fill ordering code.

    References:

[1] Ordering Sparse Matrices Using Approximate Minimum Local Fill.
    Edward Rothberg, SGI Manuscript, April 1996.
[2] An Approximate Minimum Degree Ordering Algorithm.
    T. Davis, P. Amestoy, and I. Duff, TR-94-039, CIS Department,
    University of Florida, December 1994.
*/

#include <math.h>
#include <stdlib.h>

typedef int             WSI;

#define NORDTHRESH	7
#define MAXIW		2147000000

//#define WSSMP_DBG
#ifdef WSSMP_DBG
static void chk (WSI ind, WSI i, WSI lim)
{

     if (i <= 0 || i > lim) {
          printf("%d: chk: myamlf: WAR**: i, lim = %d, %d\n", ind, i, lim);
     }
}
#endif

static void
myamlf(WSI n, WSI xadj[], WSI adjncy[], WSI dgree[], WSI varbl[],
       WSI snxt[], WSI perm[], WSI invp[], WSI head[], WSI lsize[],
       WSI flag[], WSI erscore[], WSI locaux, WSI adjln, WSI speed)
{
     WSI l, i, j, k;
     double x, y;
     WSI maxmum, fltag, nodeg, scln, nm1, deg, tn,
         locatns, ipp, jpp, nnode, numpiv, node,
         nodeln, currloc, counter, numii, mindeg,
         i0, i1, i2, i4, i5, i6, i7, i9,
         j0, j1, j2, j3, j4, j5, j6, j7, j8, j9;
     /*                                             n cannot be less than NORDTHRESH
      if (n < 3) {
         if (n > 0) perm[0] = invp[0] = 1;
         if (n > 1) perm[1] = invp[1] = 2;
         return;
      }
     */
#ifdef WSSMP_DBG
     printf("Myamlf: n, locaux, adjln, speed = %d,%d,%d,%d\n", n, locaux, adjln, speed);
     for (i = 0; i < n; i++) flag[i] = 0;
     k = xadj[0];
     for (i = 1; i <= n; i++) {
          j = k + dgree[i-1];
          if (j < k || k < 1) printf("ERR**: myamlf: i, j, k = %d,%d,%d\n", i, j, k);
          for (l = k; l < j; l++) {
               if (adjncy[l - 1] < 1 || adjncy[l - 1] > n || adjncy[l - 1] == i)
                    printf("ERR**: myamlf: i, l, adjj[l] = %d,%d,%d\n", i, l, adjncy[l - 1]);
               if (flag[adjncy[l - 1] - 1] == i)
                    printf("ERR**: myamlf: duplicate entry: %d - %d\n", i, adjncy[l - 1]);
               flag[adjncy[l - 1] - 1] = i;
               nm1 = adjncy[l - 1] - 1;
               for (fltag = xadj[nm1]; fltag < xadj[nm1] + dgree[nm1]; fltag++) {
                    if (adjncy[fltag - 1] == i) goto L100;
               }
               printf("ERR**: Unsymmetric %d %d\n", i, fltag);
L100:
               ;
          }
          k = j;
          if (k > locaux) printf("ERR**: i, j, k, locaux = %d, %d, %d, %d\n",
                                      i, j, k, locaux);
     }
     if (n*(n - 1) < locaux - 1) printf("ERR**: myamlf: too many nnz, n, ne = %d, %d\n",
                                             n, adjln);
     for (i = 0; i < n; i++) flag[i] = 1;
     if (n > 10000) printf ("Finished error checking in AMF\n");
     for (i = locaux; i <= adjln; i++) adjncy[i - 1] = -22;
#endif

     maxmum = 0;
     mindeg = 1;
     fltag = 2;
     locatns = locaux - 1;
     nm1 = n - 1;
     counter = 1;
     for (l = 0; l < n; l++) {
          j = erscore[l];
#ifdef WSSMP_DBG
          chk(1, j, n);
#endif
          if (j > 0) {
               nnode = head[j - 1];
               if (nnode) perm[nnode - 1] = l + 1;
               snxt[l] = nnode;
               head[j - 1] = l + 1;
          } else {
               invp[l] = -(counter++);
               flag[l] = xadj[l] = 0;
          }
     }
     while (counter <= n) {
          for (deg = mindeg; head[deg - 1] < 1; deg++) {};
          nodeg = 0;
#ifdef WSSMP_DBG
          chk(2, deg, n - 1);
#endif
          node = head[deg - 1];
#ifdef WSSMP_DBG
          chk(3, node, n);
#endif
          mindeg = deg;
          nnode = snxt[node - 1];
          if (nnode) perm[nnode - 1] = 0;
          head[deg - 1] = nnode;
          nodeln = invp[node - 1];
          numpiv = varbl[node - 1];
          invp[node - 1] = -counter;
          counter += numpiv;
          varbl[node - 1] = -numpiv;
          if (nodeln) {
               j4 = locaux;
               i5 = lsize[node - 1] - nodeln;
               i2 = nodeln + 1;
               l = xadj[node - 1];
               for (i6 = 1; i6 <= i2; ++i6) {
                    if (i6 == i2) {
                         tn = node, i0 = l, scln = i5;
                    } else {
#ifdef WSSMP_DBG
                         chk(4, l, adjln);
#endif
                         tn = adjncy[l-1];
                         l++;
#ifdef WSSMP_DBG
                         chk(5, tn, n);
#endif
                         i0 = xadj[tn - 1];
                         scln = lsize[tn - 1];
                    }
                    for (i7 = 1; i7 <= scln; ++i7) {
#ifdef WSSMP_DBG
                         chk(6, i0, adjln);
#endif
                         i = adjncy[i0 - 1];
                         i0++;
#ifdef WSSMP_DBG
                         chk(7, i, n);
#endif
                         numii = varbl[i - 1];
                         if (numii > 0) {
                              if (locaux > adjln) {
                                   lsize[node - 1] -= i6;
                                   xadj[node - 1] = l;
                                   if (!lsize[node - 1]) xadj[node - 1] = 0;
                                   xadj[tn - 1] = i0;
                                   lsize[tn - 1] = scln - i7;
                                   if (!lsize[tn - 1]) xadj[tn - 1] = 0;
                                   for (j = 1; j <= n; j++) {
                                        i4 = xadj[j - 1];
                                        if (i4 > 0) {
                                             xadj[j - 1] = adjncy[i4 - 1];
#ifdef WSSMP_DBG
                                             chk(8, i4, adjln);
#endif
                                             adjncy[i4 - 1] = -j;
                                        }
                                   }
                                   i9 = j4 - 1;
                                   j6 = j7 = 1;
                                   while (j6 <= i9) {
#ifdef WSSMP_DBG
                                        chk(9, j6, adjln);
#endif
                                        j = -adjncy[j6 - 1];
                                        j6++;
                                        if (j > 0) {
#ifdef WSSMP_DBG
                                             chk(10, j7, adjln);
                                             chk(11, j, n);
#endif
                                             adjncy[j7 - 1] = xadj[j - 1];
                                             xadj[j - 1] = j7++;
                                             j8 = lsize[j - 1] - 1 + j7;
                                             for (; j7 < j8; j7++, j6++) adjncy[j7-1] = adjncy[j6-1];
                                        }
                                   }
                                   for (j0 = j7; j4 < locaux; j4++, j7++) {
#ifdef WSSMP_DBG
                                        chk(12, j4, adjln);
#endif
                                        adjncy[j7 - 1] = adjncy[j4 - 1];
                                   }
                                   j4 = j0;
                                   locaux = j7;
                                   i0 = xadj[tn - 1];
                                   l = xadj[node - 1];
                              }
                              adjncy[locaux-1] = i;
                              locaux++;
                              varbl[i - 1] = -numii;
                              nodeg += numii;
                              ipp = perm[i - 1];
                              nnode = snxt[i - 1];
#ifdef WSSMP_DBG
                              if (ipp) chk(13, ipp, n);
                              if (nnode) chk(14, nnode, n);
#endif
                              if (ipp) snxt[ipp - 1] = nnode;
                              else head[erscore[i - 1] - 1] = nnode;
                              if (nnode) perm[nnode - 1] = ipp;
                         }
                    }
                    if (tn != node) {
                         flag[tn - 1] = 0, xadj[tn - 1] = -node;
                    }
               }
               currloc = (j5 = locaux) - j4;
               locatns += currloc;
          } else {
               i1 = (j4 = xadj[node - 1]) + lsize[node - 1];
               for (j = j5 = j4; j < i1; j++) {
#ifdef WSSMP_DBG
                    chk(15, j, adjln);
#endif
                    i = adjncy[j - 1];
#ifdef WSSMP_DBG
                    chk(16, i, n);
#endif
                    numii = varbl[i - 1];
                    if (numii > 0) {
                         nodeg += numii;
                         varbl[i - 1] = -numii;
#ifdef WSSMP_DBG
                         chk(17, j5, adjln);
#endif
                         adjncy[j5-1] = i;
                         ipp = perm[i - 1];
                         nnode = snxt[i - 1];
                         j5++;
#ifdef WSSMP_DBG
                         if (ipp) chk(18, ipp, n);
                         if (nnode) chk(19, nnode, n);
#endif
                         if (ipp) snxt[ipp - 1] = nnode;
                         else head[erscore[i - 1] - 1] = nnode;
                         if (nnode) perm[nnode - 1] = ipp;
                    }
               }
               currloc = 0;
          }
#ifdef WSSMP_DBG
          chk(20, node, n);
#endif
          lsize[node - 1] = j5 - (xadj[node - 1] = j4);
          dgree[node - 1] = nodeg;
          if (fltag + n < 0 || fltag + n > MAXIW) {
               for (i = 1; i <= n; i++) if (flag[i - 1]) flag[i - 1] = 1;
               fltag = 2;
          }
          for (j3 = j4; j3 < j5; j3++) {
#ifdef WSSMP_DBG
               chk(21, j3, adjln);
#endif
               i = adjncy[j3 - 1];
#ifdef WSSMP_DBG
               chk(22, i, n);
#endif
               j = invp[i - 1];
               if (j > 0) {
                    i4 = fltag - (numii = -varbl[i - 1]);
                    i2 = xadj[i - 1] + j;
                    for (l = xadj[i - 1]; l < i2; l++) {
#ifdef WSSMP_DBG
                         chk(23, l, adjln);
#endif
                         tn = adjncy[l - 1];
#ifdef WSSMP_DBG
                         chk(24, tn, n);
#endif
                         j9 = flag[tn - 1];
                         if (j9 >= fltag) j9 -= numii;
                         else if (j9) j9 = dgree[tn - 1] + i4;
                         flag[tn - 1] = j9;
                    }
               }
          }
          for (j3 = j4; j3 < j5; j3++) {
#ifdef WSSMP_DBG
               chk(25, j3, adjln);
#endif
               i = adjncy[j3 - 1];
               i5 = deg = 0;
#ifdef WSSMP_DBG
               chk(26, i, n);
#endif
               j1 = (i4 = xadj[i - 1]) + invp[i - 1];
               for (l = j0 = i4; l < j1; l++) {
#ifdef WSSMP_DBG
                    chk(27, l, adjln);
#endif
                    tn = adjncy[l - 1];
#ifdef WSSMP_DBG
                    chk(70, tn, n);
#endif
                    j8 = flag[tn - 1];
                    if (j8) {
                         deg += (j8 - fltag);
#ifdef WSSMP_DBG
                         chk(28, i4, adjln);
#endif
                         adjncy[i4-1] = tn;
                         i5 += tn;
                         i4++;
                         while (i5 >= nm1) i5 -= nm1;
                    }
               }
               invp[i - 1] = (j2 = i4) - j0 + 1;
               i2 = j0 + lsize[i - 1];
               for (l = j1; l < i2; l++) {
#ifdef WSSMP_DBG
                    chk(29, l, adjln);
#endif
                    j = adjncy[l - 1];
#ifdef WSSMP_DBG
                    chk(30, j, n);
#endif
                    numii = varbl[j - 1];
                    if (numii > 0) {
                         deg += numii;
#ifdef WSSMP_DBG
                         chk(31, i4, adjln);
#endif
                         adjncy[i4-1] = j;
                         i5 += j;
                         i4++;
                         while (i5 >= nm1) i5 -= nm1;
                    }
               }
               if (invp[i - 1] == 1 && j2 == i4) {
                    numii = -varbl[i - 1];
                    xadj[i - 1] = -node;
                    nodeg -= numii;
                    counter += numii;
                    numpiv += numii;
                    invp[i - 1] = varbl[i - 1] = 0;
               } else {
                    if (dgree[i - 1] > deg) dgree[i - 1] = deg;
#ifdef WSSMP_DBG
                    chk(32, j2, adjln);
                    chk(33, j0, adjln);
#endif
                    adjncy[i4 - 1] = adjncy[j2 - 1];
                    adjncy[j2 - 1] = adjncy[j0 - 1];
                    adjncy[j0 - 1] = node;
                    lsize[i - 1] = i4 - j0 + 1;
                    i5++;
#ifdef WSSMP_DBG
                    chk(35, i5, n);
#endif
                    j = head[i5 - 1];
                    if (j > 0) {
                         snxt[i - 1] = perm[j - 1];
                         perm[j - 1] = i;
                    } else {
                         snxt[i - 1] = -j;
                         head[i5 - 1] = -i;
                    }
                    perm[i - 1] = i5;
               }
          }
#ifdef WSSMP_DBG
          chk(36, node, n);
#endif
          dgree[node - 1] = nodeg;
          if (maxmum < nodeg) maxmum = nodeg;
          fltag += maxmum;
#ifdef WSSMP_DBG
          if (fltag + n + 1 < 0) printf("ERR**: fltag = %d (A)\n", fltag);
#endif
          for (j3 = j4; j3 < j5; ++j3) {
#ifdef WSSMP_DBG
               chk(37, j3, adjln);
#endif
               i = adjncy[j3 - 1];
#ifdef WSSMP_DBG
               chk(38, i, n);
#endif
               if (varbl[i - 1] < 0) {
                    i5 = perm[i - 1];
#ifdef WSSMP_DBG
                    chk(39, i5, n);
#endif
                    j = head[i5 - 1];
                    if (j) {
                         if (j < 0) {
                              head[i5 - 1] = 0, i = -j;
                         } else {
#ifdef WSSMP_DBG
                              chk(40, j, n);
#endif
                              i = perm[j - 1];
                              perm[j - 1] = 0;
                         }
                         while (i) {
#ifdef WSSMP_DBG
                              chk(41, i, n);
#endif
                              if (!snxt[i - 1]) {
                                   i = 0;
                              } else {
                                   k = invp[i - 1];
                                   i2 = xadj[i - 1] + (scln = lsize[i - 1]);
                                   for (l = xadj[i - 1] + 1; l < i2; l++) {
#ifdef WSSMP_DBG
                                        chk(42, l, adjln);
                                        chk(43, adjncy[l - 1], n);
#endif
                                        flag[adjncy[l - 1] - 1] = fltag;
                                   }
                                   jpp = i;
                                   j = snxt[i - 1];
                                   while (j) {
#ifdef WSSMP_DBG
                                        chk(44, j, n);
#endif
                                        if (lsize[j - 1] == scln && invp[j - 1] == k) {
                                             i2 = xadj[j - 1] + scln;
                                             j8 = 1;
                                             for (l = xadj[j - 1] + 1; l < i2; l++) {
#ifdef WSSMP_DBG
                                                  chk(45, l, adjln);
                                                  chk(46, adjncy[l - 1], n);
#endif
                                                  if (flag[adjncy[l - 1] - 1] != fltag) {
                                                       j8 = 0;
                                                       break;
                                                  }
                                             }
                                             if (j8) {
                                                  xadj[j - 1] = -i;
                                                  varbl[i - 1] += varbl[j - 1];
                                                  varbl[j - 1] = invp[j - 1] = 0;
#ifdef WSSMP_DBG
                                                  chk(47, j, n);
                                                  chk(48, jpp, n);
#endif
                                                  j = snxt[j - 1];
                                                  snxt[jpp - 1] = j;
                                             } else {
                                                  jpp = j;
#ifdef WSSMP_DBG
                                                  chk(49, j, n);
#endif
                                                  j = snxt[j - 1];
                                             }
                                        } else {
                                             jpp = j;
#ifdef WSSMP_DBG
                                             chk(50, j, n);
#endif
                                             j = snxt[j - 1];
                                        }
                                   }
                                   fltag++;
#ifdef WSSMP_DBG
                                   if (fltag + n + 1 < 0) printf("ERR**: fltag = %d (B)\n", fltag);
#endif
#ifdef WSSMP_DBG
                                   chk(51, i, n);
#endif
                                   i = snxt[i - 1];
                              }
                         }
                    }
               }
          }
          j8 = nm1 - counter;
          switch (speed) {
          case 1:
               for (j = j3 = j4; j3 < j5; j3++) {
#ifdef WSSMP_DBG
                    chk(52, j3, adjln);
#endif
                    i = adjncy[j3 - 1];
#ifdef WSSMP_DBG
                    chk(53, i, n);
#endif
                    numii = varbl[i - 1];
                    if (numii < 0) {
                         k = 0;
                         i4 = (l = xadj[i - 1]) + invp[i - 1];
                         for (; l < i4; l++) {
#ifdef WSSMP_DBG
                              chk(54, l, adjln);
                              chk(55, adjncy[l - 1], n);
#endif
                              i5 = dgree[adjncy[l - 1] - 1];
                              if (k < i5) k = i5;
                         }
                         x = static_cast<double> (k - 1);
                         varbl[i - 1] = abs(numii);
                         j9 = dgree[i - 1] + nodeg;
                         deg = (j8 > j9 ? j9 : j8) + numii;
                         if (deg < 1) deg = 1;
                         y = static_cast<double> (deg);
                         dgree[i - 1] = deg;
                         /*
                                    printf("%i %f should >= %i %f\n",deg,y,k-1,x);
                                    if (y < x) printf("** \n"); else printf("\n");
                         */
                         y = y * y - y;
                         x = y - (x * x - x);
                         if (x < 1.1) x = 1.1;
                         deg = static_cast<WSI>(sqrt(x));
                         /*         if (deg < 1) deg = 1; */
                         erscore[i - 1] = deg;
#ifdef WSSMP_DBG
                         chk(56, deg, n - 1);
#endif
                         nnode = head[deg - 1];
                         if (nnode) perm[nnode - 1] = i;
                         snxt[i - 1] = nnode;
                         perm[i - 1] = 0;
#ifdef WSSMP_DBG
                         chk(57, j, adjln);
#endif
                         head[deg - 1] = adjncy[j-1] = i;
                         j++;
                         if (deg < mindeg) mindeg = deg;
                    }
               }
               break;
          case 2:
               for (j = j3 = j4; j3 < j5; j3++) {
#ifdef WSSMP_DBG
                    chk(58, j3, adjln);
#endif
                    i = adjncy[j3 - 1];
#ifdef WSSMP_DBG
                    chk(59, i, n);
#endif
                    numii = varbl[i - 1];
                    if (numii < 0) {
                         x = static_cast<double> (dgree[adjncy[xadj[i - 1] - 1] - 1] - 1);
                         varbl[i - 1] = abs(numii);
                         j9 = dgree[i - 1] + nodeg;
                         deg = (j8 > j9 ? j9 : j8) + numii;
                         if (deg < 1) deg = 1;
                         y = static_cast<double> (deg);
                         dgree[i - 1] = deg;
                         /*
                                    printf("%i %f should >= %i %f",deg,y,dgree[adjncy[xadj[i - 1] - 1] - 1]-1,x);
                                    if (y < x) printf("** \n"); else printf("\n");
                         */
                         y = y * y - y;
                         x = y - (x * x - x);
                         if (x < 1.1) x = 1.1;
                         deg = static_cast<WSI>(sqrt(x));
                         /*         if (deg < 1) deg = 1; */
                         erscore[i - 1] = deg;
#ifdef WSSMP_DBG
                         chk(60, deg, n - 1);
#endif
                         nnode = head[deg - 1];
                         if (nnode) perm[nnode - 1] = i;
                         snxt[i - 1] = nnode;
                         perm[i - 1] = 0;
#ifdef WSSMP_DBG
                         chk(61, j, adjln);
#endif
                         head[deg - 1] = adjncy[j-1] = i;
                         j++;
                         if (deg < mindeg) mindeg = deg;
                    }
               }
               break;
          default:
               for (j = j3 = j4; j3 < j5; j3++) {
#ifdef WSSMP_DBG
                    chk(62, j3, adjln);
#endif
                    i = adjncy[j3 - 1];
#ifdef WSSMP_DBG
                    chk(63, i, n);
#endif
                    numii = varbl[i - 1];
                    if (numii < 0) {
                         varbl[i - 1] = abs(numii);
                         j9 = dgree[i - 1] + nodeg;
                         deg = (j8 > j9 ? j9 : j8) + numii;
                         if (deg < 1) deg = 1;
                         dgree[i - 1] = deg;
#ifdef WSSMP_DBG
                         chk(64, deg, n - 1);
#endif
                         nnode = head[deg - 1];
                         if (nnode) perm[nnode - 1] = i;
                         snxt[i - 1] = nnode;
                         perm[i - 1] = 0;
#ifdef WSSMP_DBG
                         chk(65, j, adjln);
#endif
                         head[deg - 1] = adjncy[j-1] = i;
                         j++;
                         if (deg < mindeg) mindeg = deg;
                    }
               }
               break;
          }
          if (currloc) {
#ifdef WSSMP_DBG
               chk(66, node, n);
#endif
               locatns += (lsize[node - 1] - currloc), locaux = j;
          }
          varbl[node - 1] = numpiv + nodeg;
          lsize[node - 1] = j - j4;
          if (!lsize[node - 1]) flag[node - 1] = xadj[node - 1] = 0;
     }
     for (l = 1; l <= n; l++) {
          if (!invp[l - 1]) {
               for (i = -xadj[l - 1]; invp[i - 1] >= 0; i = -xadj[i - 1]) {};
               tn = i;
#ifdef WSSMP_DBG
               chk(67, tn, n);
#endif
               k = -invp[tn - 1];
               i = l;
#ifdef WSSMP_DBG
               chk(68, i, n);
#endif
               while (invp[i - 1] >= 0) {
                    nnode = -xadj[i - 1];
                    xadj[i - 1] = -tn;
                    if (!invp[i - 1]) invp[i - 1] = k++;
                    i = nnode;
               }
               invp[tn - 1] = -k;
          }
     }
     for (l = 0; l < n; l++) {
          i = abs(invp[l]);
#ifdef WSSMP_DBG
          chk(69, i, n);
#endif
          invp[l] = i;
          perm[i - 1] = l + 1;
     }
     return;
}
// This code is not needed, but left in in case needed sometime
#if 0
/*C--------------------------------------------------------------------------*/
void amlfdr(WSI *n, WSI xadj[], WSI adjncy[], WSI dgree[], WSI *adjln,
            WSI *locaux, WSI varbl[], WSI snxt[], WSI perm[],
            WSI head[], WSI invp[], WSI lsize[], WSI flag[], WSI *ispeed)
{
     WSI nn, nlocaux, nadjln, speed, i, j, mx, mxj, *erscore;

#ifdef WSSMP_DBG
     printf("Calling amlfdr for n, speed = %d, %d\n", *n, *ispeed);
#endif

     if ((nn = *n) == 0) return;

#ifdef WSSMP_DBG
     if (nn == 31) {
          printf("n = %d; adjln = %d; locaux = %d; ispeed = %d\n",
                 *n, *adjln, *locaux, *ispeed);
     }
#endif

     if (nn < NORDTHRESH) {
          for (i = 0; i < nn; i++) lsize[i] = i;
          for (i = nn; i > 0; i--) {
               mx = dgree[0];
               mxj = 0;
               for (j = 1; j < i; j++)
                    if (dgree[j] > mx) {
                         mx = dgree[j];
                         mxj = j;
                    }
               invp[lsize[mxj]] = i;
               dgree[mxj] = dgree[i-1];
               lsize[mxj] = lsize[i-1];
          }
          return;
     }
     speed = *ispeed;
     if (speed < 3) {
          /*
              erscore = (WSI *)malloc(nn * sizeof(WSI));
              if (erscore == NULL) speed = 3;
          */
          wscmal (&nn, &i, &erscore);
          if (i != 0) speed = 3;
     }
     if (speed > 2) erscore = dgree;
     if (speed < 3) {
          for (i = 0; i < nn; i++) {
               perm[i] = 0;
               invp[i] = 0;
               head[i] = 0;
               flag[i] = 1;
               varbl[i] = 1;
               lsize[i] = dgree[i];
               erscore[i] = dgree[i];
          }
     } else {
          for (i = 0; i < nn; i++) {
               perm[i] = 0;
               invp[i] = 0;
               head[i] = 0;
               flag[i] = 1;
               varbl[i] = 1;
               lsize[i] = dgree[i];
          }
     }
     nlocaux = *locaux;
     nadjln = *adjln;

     myamlf(nn, xadj, adjncy, dgree, varbl, snxt, perm, invp,
            head, lsize, flag, erscore, nlocaux, nadjln, speed);
     /*
       if (speed < 3) free(erscore);
     */
     if (speed < 3) wscfr(&erscore);
     return;
}
#endif // end of taking out amlfdr
/*C--------------------------------------------------------------------------*/
#endif
// Orders rows
int
ClpCholeskyBase::orderAMD()
{
     permuteInverse_ = new int [numberRows_];
     permute_ = new int[numberRows_];
     // Do ordering
     int returnCode = 0;
     // get full matrix
     int space = 2 * sizeFactor_ + 10000 + 4 * numberRows_;
     int * temp = new int [space];
     CoinBigIndex * count = new CoinBigIndex [numberRows_];
     CoinBigIndex * tempStart = new CoinBigIndex [numberRows_+1];
     memset(count, 0, numberRows_ * sizeof(int));
     for (int iRow = 0; iRow < numberRows_; iRow++) {
          count[iRow] += choleskyStart_[iRow+1] - choleskyStart_[iRow] - 1;
          for (CoinBigIndex j = choleskyStart_[iRow] + 1; j < choleskyStart_[iRow+1]; j++) {
               int jRow = choleskyRow_[j];
               count[jRow]++;
          }
     }
#define OFFSET 1
     CoinBigIndex sizeFactor = 0;
     for (int iRow = 0; iRow < numberRows_; iRow++) {
          int length = count[iRow];
          permute_[iRow] = length;
          // add 1 to starts
          tempStart[iRow] = sizeFactor + OFFSET;
          count[iRow] = sizeFactor;
          sizeFactor += length;
     }
     tempStart[numberRows_] = sizeFactor + OFFSET;
     // add 1 to rows
     for (int iRow = 0; iRow < numberRows_; iRow++) {
          assert (choleskyRow_[choleskyStart_[iRow]] == iRow);
          for (CoinBigIndex j = choleskyStart_[iRow] + 1; j < choleskyStart_[iRow+1]; j++) {
               int jRow = choleskyRow_[j];
               int put = count[iRow];
               temp[put] = jRow + OFFSET;
               count[iRow]++;
               put = count[jRow];
               temp[put] = iRow + OFFSET;
               count[jRow]++;
          }
     }
     for (int iRow = 1; iRow < numberRows_; iRow++)
          assert (count[iRow-1] == tempStart[iRow] - OFFSET);
     delete [] choleskyRow_;
     choleskyRow_ = temp;
     delete [] choleskyStart_;
     choleskyStart_ = tempStart;
     int locaux = sizeFactor + OFFSET;
     delete [] count;
     int speed = integerParameters_[0];
     if (speed < 1 || speed > 2)
          speed = 3;
     int * use = new int [((speed<3) ? 7 : 6)*numberRows_];
     int * dgree = use;
     int * varbl = dgree + numberRows_;
     int * snxt = varbl + numberRows_;
     int * head = snxt + numberRows_;
     int * lsize = head + numberRows_;
     int * flag = lsize + numberRows_;
     int * erscore;
     for (int i = 0; i < numberRows_; i++) {
          dgree[i] = choleskyStart_[i+1] - choleskyStart_[i];
          head[i] = dgree[i];
          snxt[i] = 0;
          permute_[i] = 0;
          permuteInverse_[i] = 0;
          head[i] = 0;
          flag[i] = 1;
          varbl[i] = 1;
          lsize[i] = dgree[i];
     }
     if (speed < 3) {
          erscore = flag + numberRows_;
          for (int i = 0; i < numberRows_; i++)
               erscore[i] = dgree[i];
     } else {
          erscore = dgree;
     }
     myamlf(numberRows_, choleskyStart_, choleskyRow_,
            dgree, varbl, snxt, permute_, permuteInverse_,
            head, lsize, flag, erscore, locaux, space, speed);
     for (int iRow = 0; iRow < numberRows_; iRow++) {
          permute_[iRow]--;
     }
     for (int iRow = 0; iRow < numberRows_; iRow++) {
          permuteInverse_[permute_[iRow]] = iRow;
     }
     for (int iRow = 0; iRow < numberRows_; iRow++) {
          assert (permuteInverse_[iRow] >= 0 && permuteInverse_[iRow] < numberRows_);
     }
     delete [] use;
     delete [] choleskyRow_;
     choleskyRow_ = NULL;
     delete [] choleskyStart_;
     choleskyStart_ = NULL;
     return returnCode;
}
/* Does Symbolic factorization given permutation.
   This is called immediately after order.  If user provides this then
   user must provide factorize and solve.  Otherwise the default factorization is used
   returns non-zero if not enough memory */
int
ClpCholeskyBase::symbolic()
{
     const CoinBigIndex * columnStart = model_->clpMatrix()->getVectorStarts();
     const int * columnLength = model_->clpMatrix()->getVectorLengths();
     const int * row = model_->clpMatrix()->getIndices();
     const CoinBigIndex * rowStart = rowCopy_->getVectorStarts();
     const int * rowLength = rowCopy_->getVectorLengths();
     const int * column = rowCopy_->getIndices();
     int numberRowsModel = model_->numberRows();
     int numberColumns = model_->numberColumns();
     int numberTotal = numberColumns + numberRowsModel;
     CoinPackedMatrix * quadratic = NULL;
     ClpQuadraticObjective * quadraticObj =
          (dynamic_cast< ClpQuadraticObjective*>(model_->objectiveAsObject()));
     if (quadraticObj)
          quadratic = quadraticObj->quadraticObjective();
     // We need an array for counts
     int * used = new int[numberRows_+1];
     // If KKT then re-order so negative first
     if (doKKT_) {
          int nn = 0;
          int np = 0;
          int iRow;
          for (iRow = 0; iRow < numberRows_; iRow++) {
               int originalRow = permute_[iRow];
               if (originalRow < numberTotal)
                    permute_[nn++] = originalRow;
               else
                    used[np++] = originalRow;
          }
          CoinMemcpyN(used, np, permute_ + nn);
          for (iRow = 0; iRow < numberRows_; iRow++)
               permuteInverse_[permute_[iRow]] = iRow;
     }
     CoinZeroN(used, numberRows_);
     int iRow;
     int iColumn;
     bool noMemory = false;
     CoinBigIndex * Astart = new CoinBigIndex[numberRows_+1];
     int * Arow = NULL;
     try {
          Arow = new int [sizeFactor_];
     } catch (...) {
          // no memory
          delete [] Astart;
          return -1;
     }
     choleskyStart_ = new int[numberRows_+1];
     link_ = new int[numberRows_];
     workInteger_ = new CoinBigIndex[numberRows_];
     indexStart_ = new CoinBigIndex[numberRows_];
     clique_ = new int[numberRows_];
     // Redo so permuted upper triangular
     sizeFactor_ = 0;
     int * which = Arow;
     if (!doKKT_) {
          for (iRow = 0; iRow < numberRows_; iRow++) {
               int number = 0;
               int iOriginalRow = permute_[iRow];
               Astart[iRow] = sizeFactor_;
               CoinBigIndex startRow = rowStart[iOriginalRow];
               CoinBigIndex endRow = rowStart[iOriginalRow] + rowLength[iOriginalRow];
               for (CoinBigIndex k = startRow; k < endRow; k++) {
                    int iColumn = column[k];
                    if (!whichDense_ || !whichDense_[iColumn]) {
                         CoinBigIndex start = columnStart[iColumn];
                         CoinBigIndex end = columnStart[iColumn] + columnLength[iColumn];
                         for (CoinBigIndex j = start; j < end; j++) {
                              int jRow = row[j];
                              int jNewRow = permuteInverse_[jRow];
                              if (jNewRow < iRow) {
                                   if (!used[jNewRow]) {
                                        used[jNewRow] = 1;
                                        which[number++] = jNewRow;
                                   }
                              }
                         }
                    }
               }
               sizeFactor_ += number;
               int j;
               for (j = 0; j < number; j++)
                    used[which[j]] = 0;
               // Sort
               std::sort(which, which + number);
               // move which on
               which += number;
          }
     } else {
          // KKT
          // transpose
          ClpMatrixBase * rowCopy = model_->clpMatrix()->reverseOrderedCopy();
          const CoinBigIndex * rowStart = rowCopy->getVectorStarts();
          const int * rowLength = rowCopy->getVectorLengths();
          const int * column = rowCopy->getIndices();
          // temp
          bool permuted = false;
          for (iRow = 0; iRow < numberRows_; iRow++) {
               if (permute_[iRow] != iRow) {
                    permuted = true;
                    break;
               }
          }
          if (permuted) {
               // Need to permute - ugly
               if (!quadratic) {
                    for (iRow = 0; iRow < numberRows_; iRow++) {
                         Astart[iRow] = sizeFactor_;
                         int iOriginalRow = permute_[iRow];
                         if (iOriginalRow < numberColumns) {
                              // A may be upper triangular by mistake
                              iColumn = iOriginalRow;
                              CoinBigIndex start = columnStart[iColumn];
                              CoinBigIndex end = columnStart[iColumn] + columnLength[iColumn];
                              for (CoinBigIndex j = start; j < end; j++) {
                                   int kRow = row[j] + numberTotal;
                                   kRow = permuteInverse_[kRow];
                                   if (kRow < iRow)
                                        Arow[sizeFactor_++] = kRow;
                              }
                         } else if (iOriginalRow < numberTotal) {
                              int kRow = permuteInverse_[iOriginalRow+numberRowsModel];
                              if (kRow < iRow)
                                   Arow[sizeFactor_++] = kRow;
                         } else {
                              int kRow = iOriginalRow - numberTotal;
                              CoinBigIndex start = rowStart[kRow];
                              CoinBigIndex end = rowStart[kRow] + rowLength[kRow];
                              for (CoinBigIndex j = start; j < end; j++) {
                                   int jRow = column[j];
                                   int jNewRow = permuteInverse_[jRow];
                                   if (jNewRow < iRow)
                                        Arow[sizeFactor_++] = jNewRow;
                              }
                              // slack - should it be permute
                              kRow = permuteInverse_[kRow+numberColumns];
                              if (kRow < iRow)
                                   Arow[sizeFactor_++] = kRow;
                         }
                         // Sort
                         std::sort(Arow + Astart[iRow], Arow + sizeFactor_);
                    }
               } else {
                    // quadratic
                    // transpose
                    CoinPackedMatrix quadraticT;
                    quadraticT.reverseOrderedCopyOf(*quadratic);
                    const int * columnQuadratic = quadraticT.getIndices();
                    const CoinBigIndex * columnQuadraticStart = quadraticT.getVectorStarts();
                    const int * columnQuadraticLength = quadraticT.getVectorLengths();
                    for (iRow = 0; iRow < numberRows_; iRow++) {
                         Astart[iRow] = sizeFactor_;
                         int iOriginalRow = permute_[iRow];
                         if (iOriginalRow < numberColumns) {
                              // Quadratic bit
                              CoinBigIndex j;
                              for ( j = columnQuadraticStart[iOriginalRow];
                                        j < columnQuadraticStart[iOriginalRow] + columnQuadraticLength[iOriginalRow]; j++) {
                                   int jColumn = columnQuadratic[j];
                                   int jNewColumn = permuteInverse_[jColumn];
                                   if (jNewColumn < iRow)
                                        Arow[sizeFactor_++] = jNewColumn;
                              }
                              // A may be upper triangular by mistake
                              iColumn = iOriginalRow;
                              CoinBigIndex start = columnStart[iColumn];
                              CoinBigIndex end = columnStart[iColumn] + columnLength[iColumn];
                              for (j = start; j < end; j++) {
                                   int kRow = row[j] + numberTotal;
                                   kRow = permuteInverse_[kRow];
                                   if (kRow < iRow)
                                        Arow[sizeFactor_++] = kRow;
                              }
                         } else if (iOriginalRow < numberTotal) {
                              int kRow = permuteInverse_[iOriginalRow+numberRowsModel];
                              if (kRow < iRow)
                                   Arow[sizeFactor_++] = kRow;
                         } else {
                              int kRow = iOriginalRow - numberTotal;
                              CoinBigIndex start = rowStart[kRow];
                              CoinBigIndex end = rowStart[kRow] + rowLength[kRow];
                              for (CoinBigIndex j = start; j < end; j++) {
                                   int jRow = column[j];
                                   int jNewRow = permuteInverse_[jRow];
                                   if (jNewRow < iRow)
                                        Arow[sizeFactor_++] = jNewRow;
                              }
                              // slack - should it be permute
                              kRow = permuteInverse_[kRow+numberColumns];
                              if (kRow < iRow)
                                   Arow[sizeFactor_++] = kRow;
                         }
                         // Sort
                         std::sort(Arow + Astart[iRow], Arow + sizeFactor_);
                    }
               }
          } else {
               if (!quadratic) {
                    for (iRow = 0; iRow < numberRows_; iRow++) {
                         Astart[iRow] = sizeFactor_;
                    }
               } else {
                    // Quadratic
                    // transpose
                    CoinPackedMatrix quadraticT;
                    quadraticT.reverseOrderedCopyOf(*quadratic);
                    const int * columnQuadratic = quadraticT.getIndices();
                    const CoinBigIndex * columnQuadraticStart = quadraticT.getVectorStarts();
                    const int * columnQuadraticLength = quadraticT.getVectorLengths();
                    //const double * quadraticElement = quadraticT.getElements();
                    for (iRow = 0; iRow < numberColumns; iRow++) {
                         int iOriginalRow = permute_[iRow];
                         Astart[iRow] = sizeFactor_;
                         for (CoinBigIndex j = columnQuadraticStart[iOriginalRow];
                                   j < columnQuadraticStart[iOriginalRow] + columnQuadraticLength[iOriginalRow]; j++) {
                              int jColumn = columnQuadratic[j];
                              int jNewColumn = permuteInverse_[jColumn];
                              if (jNewColumn < iRow)
                                   Arow[sizeFactor_++] = jNewColumn;
                         }
                    }
               }
               int iRow;
               // slacks
               for (iRow = 0; iRow < numberRowsModel; iRow++) {
                    Astart[iRow+numberColumns] = sizeFactor_;
               }
               for (iRow = 0; iRow < numberRowsModel; iRow++) {
                    Astart[iRow+numberTotal] = sizeFactor_;
                    CoinBigIndex start = rowStart[iRow];
                    CoinBigIndex end = rowStart[iRow] + rowLength[iRow];
                    for (CoinBigIndex j = start; j < end; j++) {
                         Arow[sizeFactor_++] = column[j];
                    }
                    // slack
                    Arow[sizeFactor_++] = numberColumns + iRow;
               }
          }
          delete rowCopy;
     }
     Astart[numberRows_] = sizeFactor_;
     firstDense_ = numberRows_;
     symbolic1(Astart, Arow);
     // Now fill in indices
     try {
          // too big
          choleskyRow_ = new int[sizeFactor_];
     } catch (...) {
          // no memory
          noMemory = true;
     }
     double sizeFactor = sizeFactor_;
     if (!noMemory) {
          // Do lower triangular
          sizeFactor_ = 0;
          int * which = Arow;
          if (!doKKT_) {
               for (iRow = 0; iRow < numberRows_; iRow++) {
                    int number = 0;
                    int iOriginalRow = permute_[iRow];
                    Astart[iRow] = sizeFactor_;
                    if (!rowsDropped_[iOriginalRow]) {
                         CoinBigIndex startRow = rowStart[iOriginalRow];
                         CoinBigIndex endRow = rowStart[iOriginalRow] + rowLength[iOriginalRow];
                         for (CoinBigIndex k = startRow; k < endRow; k++) {
                              int iColumn = column[k];
                              if (!whichDense_ || !whichDense_[iColumn]) {
                                   CoinBigIndex start = columnStart[iColumn];
                                   CoinBigIndex end = columnStart[iColumn] + columnLength[iColumn];
                                   for (CoinBigIndex j = start; j < end; j++) {
                                        int jRow = row[j];
                                        int jNewRow = permuteInverse_[jRow];
                                        if (jNewRow > iRow && !rowsDropped_[jRow]) {
                                             if (!used[jNewRow]) {
                                                  used[jNewRow] = 1;
                                                  which[number++] = jNewRow;
                                             }
                                        }
                                   }
                              }
                         }
                         sizeFactor_ += number;
                         int j;
                         for (j = 0; j < number; j++)
                              used[which[j]] = 0;
                         // Sort
                         std::sort(which, which + number);
                         // move which on
                         which += number;
                    }
               }
          } else {
               // KKT
               // temp
               bool permuted = false;
               for (iRow = 0; iRow < numberRows_; iRow++) {
                    if (permute_[iRow] != iRow) {
                         permuted = true;
                         break;
                    }
               }
               // but fake it
               for (iRow = 0; iRow < numberRows_; iRow++) {
                    //permute_[iRow]=iRow; // force no permute
                    //permuteInverse_[permute_[iRow]]=iRow;
               }
               if (permuted) {
                    // Need to permute - ugly
                    if (!quadratic) {
                         for (iRow = 0; iRow < numberRows_; iRow++) {
                              Astart[iRow] = sizeFactor_;
                              int iOriginalRow = permute_[iRow];
                              if (iOriginalRow < numberColumns) {
                                   iColumn = iOriginalRow;
                                   CoinBigIndex start = columnStart[iColumn];
                                   CoinBigIndex end = columnStart[iColumn] + columnLength[iColumn];
                                   for (CoinBigIndex j = start; j < end; j++) {
                                        int kRow = row[j] + numberTotal;
                                        kRow = permuteInverse_[kRow];
                                        if (kRow > iRow)
                                             Arow[sizeFactor_++] = kRow;
                                   }
                              } else if (iOriginalRow < numberTotal) {
                                   int kRow = permuteInverse_[iOriginalRow+numberRowsModel];
                                   if (kRow > iRow)
                                        Arow[sizeFactor_++] = kRow;
                              } else {
                                   int kRow = iOriginalRow - numberTotal;
                                   CoinBigIndex start = rowStart[kRow];
                                   CoinBigIndex end = rowStart[kRow] + rowLength[kRow];
                                   for (CoinBigIndex j = start; j < end; j++) {
                                        int jRow = column[j];
                                        int jNewRow = permuteInverse_[jRow];
                                        if (jNewRow > iRow)
                                             Arow[sizeFactor_++] = jNewRow;
                                   }
                                   // slack - should it be permute
                                   kRow = permuteInverse_[kRow+numberColumns];
                                   if (kRow > iRow)
                                        Arow[sizeFactor_++] = kRow;
                              }
                              // Sort
                              std::sort(Arow + Astart[iRow], Arow + sizeFactor_);
                         }
                    } else {
                         // quadratic
                         const int * columnQuadratic = quadratic->getIndices();
                         const CoinBigIndex * columnQuadraticStart = quadratic->getVectorStarts();
                         const int * columnQuadraticLength = quadratic->getVectorLengths();
                         for (iRow = 0; iRow < numberRows_; iRow++) {
                              Astart[iRow] = sizeFactor_;
                              int iOriginalRow = permute_[iRow];
                              if (iOriginalRow < numberColumns) {
                                   // Quadratic bit
                                   CoinBigIndex j;
                                   for ( j = columnQuadraticStart[iOriginalRow];
                                             j < columnQuadraticStart[iOriginalRow] + columnQuadraticLength[iOriginalRow]; j++) {
                                        int jColumn = columnQuadratic[j];
                                        int jNewColumn = permuteInverse_[jColumn];
                                        if (jNewColumn > iRow)
                                             Arow[sizeFactor_++] = jNewColumn;
                                   }
                                   iColumn = iOriginalRow;
                                   CoinBigIndex start = columnStart[iColumn];
                                   CoinBigIndex end = columnStart[iColumn] + columnLength[iColumn];
                                   for (j = start; j < end; j++) {
                                        int kRow = row[j] + numberTotal;
                                        kRow = permuteInverse_[kRow];
                                        if (kRow > iRow)
                                             Arow[sizeFactor_++] = kRow;
                                   }
                              } else if (iOriginalRow < numberTotal) {
                                   int kRow = permuteInverse_[iOriginalRow+numberRowsModel];
                                   if (kRow > iRow)
                                        Arow[sizeFactor_++] = kRow;
                              } else {
                                   int kRow = iOriginalRow - numberTotal;
                                   CoinBigIndex start = rowStart[kRow];
                                   CoinBigIndex end = rowStart[kRow] + rowLength[kRow];
                                   for (CoinBigIndex j = start; j < end; j++) {
                                        int jRow = column[j];
                                        int jNewRow = permuteInverse_[jRow];
                                        if (jNewRow > iRow)
                                             Arow[sizeFactor_++] = jNewRow;
                                   }
                                   // slack - should it be permute
                                   kRow = permuteInverse_[kRow+numberColumns];
                                   if (kRow > iRow)
                                        Arow[sizeFactor_++] = kRow;
                              }
                              // Sort
                              std::sort(Arow + Astart[iRow], Arow + sizeFactor_);
                         }
                    }
               } else {
                    if (!quadratic) {
                         for (iColumn = 0; iColumn < numberColumns; iColumn++) {
                              Astart[iColumn] = sizeFactor_;
                              CoinBigIndex start = columnStart[iColumn];
                              CoinBigIndex end = columnStart[iColumn] + columnLength[iColumn];
                              for (CoinBigIndex j = start; j < end; j++) {
                                   Arow[sizeFactor_++] = row[j] + numberTotal;
                              }
                         }
                    } else {
                         // Quadratic
                         const int * columnQuadratic = quadratic->getIndices();
                         const CoinBigIndex * columnQuadraticStart = quadratic->getVectorStarts();
                         const int * columnQuadraticLength = quadratic->getVectorLengths();
                         //const double * quadraticElement = quadratic->getElements();
                         for (iColumn = 0; iColumn < numberColumns; iColumn++) {
                              Astart[iColumn] = sizeFactor_;
                              CoinBigIndex j;
                              for ( j = columnQuadraticStart[iColumn];
                                        j < columnQuadraticStart[iColumn] + columnQuadraticLength[iColumn]; j++) {
                                   int jColumn = columnQuadratic[j];
                                   if (jColumn > iColumn)
                                        Arow[sizeFactor_++] = jColumn;
                              }
                              CoinBigIndex start = columnStart[iColumn];
                              CoinBigIndex end = columnStart[iColumn] + columnLength[iColumn];
                              for ( j = start; j < end; j++) {
                                   Arow[sizeFactor_++] = row[j] + numberTotal;
                              }
                         }
                    }
                    // slacks
                    for (iRow = 0; iRow < numberRowsModel; iRow++) {
                         Astart[iRow+numberColumns] = sizeFactor_;
                         Arow[sizeFactor_++] = iRow + numberTotal;
                    }
                    // Transpose - nonzero diagonal (may regularize)
                    for (iRow = 0; iRow < numberRowsModel; iRow++) {
                         Astart[iRow+numberTotal] = sizeFactor_;
                    }
               }
               Astart[numberRows_] = sizeFactor_;
          }
          symbolic2(Astart, Arow);
          if (sizeIndex_ < sizeFactor_) {
               int * indices = NULL;
               try {
                    indices = new int[sizeIndex_];
               } catch (...) {
                    // no memory
                    noMemory = true;
               }
               if (!noMemory)  {
                    CoinMemcpyN(choleskyRow_, sizeIndex_, indices);
                    delete [] choleskyRow_;
                    choleskyRow_ = indices;
               }
          }
     }
     delete [] used;
     // Use cholesky regions
     delete [] Astart;
     delete [] Arow;
     double flops = 0.0;
     for (iRow = 0; iRow < numberRows_; iRow++) {
          int length = choleskyStart_[iRow+1] - choleskyStart_[iRow];
          flops += static_cast<double> (length) * (length + 2.0);
     }
     if (model_->messageHandler()->logLevel() > 0)
          std::cout << sizeFactor << " elements in sparse Cholesky, flop count " << flops << std::endl;
     try {
          sparseFactor_ = new longDouble [sizeFactor_];
#if CLP_LONG_CHOLESKY!=1
          workDouble_ = new longDouble[numberRows_];
#else
          // actually long double
          workDouble_ = reinterpret_cast<double *> (new CoinWorkDouble[numberRows_]);
#endif
          diagonal_ = new longDouble[numberRows_];
     } catch (...) {
          // no memory
          noMemory = true;
     }
     if (noMemory) {
          delete [] choleskyRow_;
          choleskyRow_ = NULL;
          delete [] choleskyStart_;
          choleskyStart_ = NULL;
          delete [] permuteInverse_;
          permuteInverse_ = NULL;
          delete [] permute_;
          permute_ = NULL;
          delete [] choleskyStart_;
          choleskyStart_ = NULL;
          delete [] indexStart_;
          indexStart_ = NULL;
          delete [] link_;
          link_ = NULL;
          delete [] workInteger_;
          workInteger_ = NULL;
          delete [] sparseFactor_;
          sparseFactor_ = NULL;
          delete [] workDouble_;
          workDouble_ = NULL;
          delete [] diagonal_;
          diagonal_ = NULL;
          delete [] clique_;
          clique_ = NULL;
          return -1;
     }
     return 0;
}
int
ClpCholeskyBase::symbolic1(const CoinBigIndex * Astart, const int * Arow)
{
     int * marked = reinterpret_cast<int *> (workInteger_);
     int iRow;
     // may not need to do this here but makes debugging easier
     for (iRow = 0; iRow < numberRows_; iRow++) {
          marked[iRow] = -1;
          link_[iRow] = -1;
          choleskyStart_[iRow] = 0; // counts
     }
     for (iRow = 0; iRow < numberRows_; iRow++) {
          marked[iRow] = iRow;
          for (CoinBigIndex j = Astart[iRow]; j < Astart[iRow+1]; j++) {
               int kRow = Arow[j];
               while (marked[kRow] != iRow ) {
                    if (link_[kRow] < 0 )
                         link_[kRow] = iRow;
                    choleskyStart_[kRow]++;
                    marked[kRow] = iRow;
                    kRow = link_[kRow];
               }
          }
     }
     sizeFactor_ = 0;
     for (iRow = 0; iRow < numberRows_; iRow++) {
          int number = choleskyStart_[iRow];
          choleskyStart_[iRow] = sizeFactor_;
          sizeFactor_ += number;
     }
     choleskyStart_[numberRows_] = sizeFactor_;
     return sizeFactor_;;
}
void
ClpCholeskyBase::symbolic2(const CoinBigIndex * Astart, const int * Arow)
{
     int * mergeLink = clique_;
     int * marker = reinterpret_cast<int *> (workInteger_);
     int iRow;
     for (iRow = 0; iRow < numberRows_; iRow++) {
          marker[iRow] = -1;
          mergeLink[iRow] = -1;
          link_[iRow] = -1; // not needed but makes debugging easier
     }
     int start = 0;
     int end = 0;
     choleskyStart_[0] = 0;

     for (iRow = 0; iRow < numberRows_; iRow++) {
          int nz = 0;
          int merge = mergeLink[iRow];
          bool marked = false;
          if (merge < 0)
               marker[iRow] = iRow;
          else
               marker[iRow] = merge;
          start = end;
          int startSub = start;
          link_[iRow] = numberRows_;
          CoinBigIndex j;
          for ( j = Astart[iRow]; j < Astart[iRow+1]; j++) {
               int kRow = Arow[j];
               int k = iRow;
               int linked = link_[iRow];
#ifndef NDEBUG
               int count = 0;
#endif
               while (linked <= kRow) {
                    k = linked;
                    linked = link_[k];
#ifndef NDEBUG
                    count++;
                    assert (count < numberRows_);
#endif
               }
               nz++;
               link_[k] = kRow;
               link_[kRow] = linked;
               if (marker[kRow] != marker[iRow])
                    marked = true;
          }
          bool reuse = false;
          // Check if we can re-use indices
          if (!marked && merge >= 0 && mergeLink[merge] < 0) {
               // can re-use all
               startSub = indexStart_[merge] + 1;
               nz = choleskyStart_[merge+1] - (choleskyStart_[merge] + 1);
               reuse = true;
          } else {
               // See if we can re-use any
               int k = mergeLink[iRow];
               int maxLength = 0;
               while (k >= 0) {
                    int length = choleskyStart_[k+1] - (choleskyStart_[k] + 1);
                    int start = indexStart_[k] + 1;
                    int stop = start + length;
                    if (length > maxLength) {
                         maxLength = length;
                         startSub = start;
                    }
                    int linked = iRow;
                    for (CoinBigIndex j = start; j < stop; j++) {
                         int kRow = choleskyRow_[j];
                         int kk = linked;
                         linked = link_[kk];
                         while (linked < kRow) {
                              kk = linked;
                              linked = link_[kk];
                         }
                         if (linked != kRow) {
                              nz++;
                              link_[kk] = kRow;
                              link_[kRow] = linked;
                              linked = kRow;
                         }
                    }
                    k = mergeLink[k];
               }
               if (nz == maxLength)
                    reuse = true; // can re-use
          }
          //reuse=false; //temp
          if (!reuse) {
               end += nz;
               startSub = start;
               int kRow = iRow;
               for (int j = start; j < end; j++) {
                    kRow = link_[kRow];
                    choleskyRow_[j] = kRow;
                    assert (kRow < numberRows_);
                    marker[kRow] = iRow;
               }
               marker[iRow] = iRow;
          }
          indexStart_[iRow] = startSub;
          choleskyStart_[iRow+1] = choleskyStart_[iRow] + nz;
          if (nz > 1) {
               int kRow = choleskyRow_[startSub];
               mergeLink[iRow] = mergeLink[kRow];
               mergeLink[kRow] = iRow;
          }
          // should not be needed
          //std::sort(choleskyRow_+indexStart_[iRow]
          //      ,choleskyRow_+indexStart_[iRow]+nz);
          //#define CLP_DEBUG
#ifdef CLP_DEBUG
          int last = -1;
          for ( j = indexStart_[iRow]; j < indexStart_[iRow] + nz; j++) {
               int kRow = choleskyRow_[j];
               assert (kRow > last);
               last = kRow;
          }
#endif
     }
     sizeFactor_ = choleskyStart_[numberRows_];
     sizeIndex_ = start;
     // find dense segment here
     int numberleft = numberRows_;
     for (iRow = 0; iRow < numberRows_; iRow++) {
          CoinBigIndex left = sizeFactor_ - choleskyStart_[iRow];
          double n = numberleft;
          double threshold = n * (n - 1.0) * 0.5 * goDense_;
          if ( left >= threshold)
               break;
          numberleft--;
     }
     //iRow=numberRows_;
     int nDense = numberRows_ - iRow;
#define DENSE_THRESHOLD 8
     // don't do if dense columns
     if (nDense >= DENSE_THRESHOLD && !dense_) {
       COIN_DETAIL_PRINT(printf("Going dense for last %d rows\n", nDense));
          // make sure we don't disturb any indices
          CoinBigIndex k = 0;
          for (int jRow = 0; jRow < iRow; jRow++) {
               int nz = choleskyStart_[jRow+1] - choleskyStart_[jRow];
               k = CoinMax(k, indexStart_[jRow] + nz);
          }
          indexStart_[iRow] = k;
          int j;
          for (j = iRow + 1; j < numberRows_; j++) {
               choleskyRow_[k++] = j;
               indexStart_[j] = k;
          }
          sizeIndex_ = k;
          assert (k <= sizeFactor_); // can't happen with any reasonable defaults
          k = choleskyStart_[iRow];
          for (j = iRow + 1; j <= numberRows_; j++) {
               k += numberRows_ - j;
               choleskyStart_[j] = k;
          }
          // allow for blocked dense
          ClpCholeskyDense dense;
          sizeFactor_ = choleskyStart_[iRow] + dense.space(nDense);
          firstDense_ = iRow;
          if (doKKT_) {
               // redo permute so negative ones first
               int putN = firstDense_;
               int putP = 0;
               int numberRowsModel = model_->numberRows();
               int numberColumns = model_->numberColumns();
               int numberTotal = numberColumns + numberRowsModel;
               for (iRow = firstDense_; iRow < numberRows_; iRow++) {
                    int originalRow = permute_[iRow];
                    if (originalRow < numberTotal)
                         permute_[putN++] = originalRow;
                    else
                         permuteInverse_[putP++] = originalRow;
               }
               for (iRow = putN; iRow < numberRows_; iRow++) {
                    permute_[iRow] = permuteInverse_[iRow-putN];
               }
               for (iRow = 0; iRow < numberRows_; iRow++) {
                    permuteInverse_[permute_[iRow]] = iRow;
               }
          }
     }
     // Clean up clique info
     for (iRow = 0; iRow < numberRows_; iRow++)
          clique_[iRow] = 0;
     int lastClique = -1;
     bool inClique = false;
     for (iRow = 1; iRow < firstDense_; iRow++) {
          int sizeLast = choleskyStart_[iRow] - choleskyStart_[iRow-1];
          int sizeThis = choleskyStart_[iRow+1] - choleskyStart_[iRow];
          if (indexStart_[iRow] == indexStart_[iRow-1] + 1 &&
                    sizeThis == sizeLast - 1 &&
                    sizeThis) {
               // in clique
               if (!inClique) {
                    inClique = true;
                    lastClique = iRow - 1;
               }
          } else if (inClique) {
               int sizeClique = iRow - lastClique;
               for (int i = lastClique; i < iRow; i++) {
                    clique_[i] = sizeClique;
                    sizeClique--;
               }
               inClique = false;
          }
     }
     if (inClique) {
          int sizeClique = iRow - lastClique;
          for (int i = lastClique; i < iRow; i++) {
               clique_[i] = sizeClique;
               sizeClique--;
          }
     }
     //for (iRow=0;iRow<numberRows_;iRow++)
     //clique_[iRow]=0;
}
/* Factorize - filling in rowsDropped and returning number dropped */
int
ClpCholeskyBase::factorize(const CoinWorkDouble * diagonal, int * rowsDropped)
{
     const CoinBigIndex * columnStart = model_->clpMatrix()->getVectorStarts();
     const int * columnLength = model_->clpMatrix()->getVectorLengths();
     const int * row = model_->clpMatrix()->getIndices();
     const double * element = model_->clpMatrix()->getElements();
     const CoinBigIndex * rowStart = rowCopy_->getVectorStarts();
     const int * rowLength = rowCopy_->getVectorLengths();
     const int * column = rowCopy_->getIndices();
     const double * elementByRow = rowCopy_->getElements();
     int numberColumns = model_->clpMatrix()->getNumCols();
     //perturbation
     CoinWorkDouble perturbation = model_->diagonalPerturbation() * model_->diagonalNorm();
     //perturbation=perturbation*perturbation*100000000.0;
     if (perturbation > 1.0) {
#ifdef COIN_DEVELOP
          //if (model_->model()->logLevel()&4)
          std::cout << "large perturbation " << perturbation << std::endl;
#endif
          perturbation = CoinSqrt(perturbation);
          perturbation = 1.0;
     }
     int iRow;
     int iColumn;
     longDouble * work = workDouble_;
     CoinZeroN(work, numberRows_);
     int newDropped = 0;
     CoinWorkDouble largest = 1.0;
     CoinWorkDouble smallest = COIN_DBL_MAX;
     int numberDense = 0;
     if (!doKKT_) {
          const CoinWorkDouble * diagonalSlack = diagonal + numberColumns;
          if (dense_)
               numberDense = dense_->numberRows();
          if (whichDense_) {
               longDouble * denseDiagonal = dense_->diagonal();
               longDouble * dense = denseColumn_;
               int iDense = 0;
               for (int iColumn = 0; iColumn < numberColumns; iColumn++) {
                    if (whichDense_[iColumn]) {
                         CoinZeroN(dense, numberRows_);
                         CoinBigIndex start = columnStart[iColumn];
                         CoinBigIndex end = columnStart[iColumn] + columnLength[iColumn];
                         if (diagonal[iColumn]) {
                              denseDiagonal[iDense++] = 1.0 / diagonal[iColumn];
                              for (CoinBigIndex j = start; j < end; j++) {
                                   int jRow = row[j];
                                   dense[jRow] = element[j];
                              }
                         } else {
                              denseDiagonal[iDense++] = 1.0;
                         }
                         dense += numberRows_;
                    }
               }
          }
          CoinWorkDouble delta2 = model_->delta(); // add delta*delta to diagonal
          delta2 *= delta2;
          // largest in initial matrix
          CoinWorkDouble largest2 = 1.0e-20;
          for (iRow = 0; iRow < numberRows_; iRow++) {
               longDouble * put = sparseFactor_ + choleskyStart_[iRow];
               int * which = choleskyRow_ + indexStart_[iRow];
               int iOriginalRow = permute_[iRow];
               int number = choleskyStart_[iRow+1] - choleskyStart_[iRow];
               if (!rowLength[iOriginalRow])
                    rowsDropped_[iOriginalRow] = 1;
               if (!rowsDropped_[iOriginalRow]) {
                    CoinBigIndex startRow = rowStart[iOriginalRow];
                    CoinBigIndex endRow = rowStart[iOriginalRow] + rowLength[iOriginalRow];
                    work[iRow] = diagonalSlack[iOriginalRow] + delta2;
                    for (CoinBigIndex k = startRow; k < endRow; k++) {
                         int iColumn = column[k];
                         if (!whichDense_ || !whichDense_[iColumn]) {
                              CoinBigIndex start = columnStart[iColumn];
                              CoinBigIndex end = columnStart[iColumn] + columnLength[iColumn];
                              CoinWorkDouble multiplier = diagonal[iColumn] * elementByRow[k];
                              for (CoinBigIndex j = start; j < end; j++) {
                                   int jRow = row[j];
                                   int jNewRow = permuteInverse_[jRow];
                                   if (jNewRow >= iRow && !rowsDropped_[jRow]) {
                                        CoinWorkDouble value = element[j] * multiplier;
                                        work[jNewRow] += value;
                                   }
                              }
                         }
                    }
                    diagonal_[iRow] = work[iRow];
                    largest2 = CoinMax(largest2, CoinAbs(work[iRow]));
                    work[iRow] = 0.0;
                    int j;
                    for (j = 0; j < number; j++) {
                         int jRow = which[j];
                         put[j] = work[jRow];
                         largest2 = CoinMax(largest2, CoinAbs(work[jRow]));
                         work[jRow] = 0.0;
                    }
               } else {
                    // dropped
                    diagonal_[iRow] = 1.0;
                    int j;
                    for (j = 1; j < number; j++) {
                         put[j] = 0.0;
                    }
               }
          }
          //check sizes
          largest2 *= 1.0e-20;
          largest = CoinMin(largest2, CHOL_SMALL_VALUE);
          int numberDroppedBefore = 0;
          for (iRow = 0; iRow < numberRows_; iRow++) {
               int dropped = rowsDropped_[iRow];
               // Move to int array
               rowsDropped[iRow] = dropped;
               if (!dropped) {
                    CoinWorkDouble diagonal = diagonal_[iRow];
                    if (diagonal > largest2) {
                         diagonal_[iRow] = diagonal + perturbation;
                    } else {
                         diagonal_[iRow] = diagonal + perturbation;
                         rowsDropped[iRow] = 2;
                         numberDroppedBefore++;
                         //printf("dropped - small diagonal %g\n",diagonal);
                    }
               }
          }
          doubleParameters_[10] = CoinMax(1.0e-20, largest);
          integerParameters_[20] = 0;
          doubleParameters_[3] = 0.0;
          doubleParameters_[4] = COIN_DBL_MAX;
          integerParameters_[34] = 0; // say all must be positive
          factorizePart2(rowsDropped);
          newDropped = integerParameters_[20] + numberDroppedBefore;
          largest = doubleParameters_[3];
          smallest = doubleParameters_[4];
          if (model_->messageHandler()->logLevel() > 1)
               std::cout << "Cholesky - largest " << largest << " smallest " << smallest << std::endl;
          choleskyCondition_ = largest / smallest;
          if (whichDense_) {
               int i;
               for ( i = 0; i < numberRows_; i++) {
                    assert (diagonal_[i] >= 0.0);
                    diagonal_[i] = CoinSqrt(diagonal_[i]);
               }
               // Update dense columns (just L)
               // Zero out dropped rows
               for (i = 0; i < numberDense; i++) {
                    longDouble * a = denseColumn_ + i * numberRows_;
                    for (int j = 0; j < numberRows_; j++) {
                         if (rowsDropped[j])
                              a[j] = 0.0;
                    }
                    for (i = 0; i < numberRows_; i++) {
                         int iRow = permute_[i];
                         workDouble_[i] = a[iRow];
                    }
                    for (i = 0; i < numberRows_; i++) {
                         CoinWorkDouble value = workDouble_[i];
                         CoinBigIndex offset = indexStart_[i] - choleskyStart_[i];
                         CoinBigIndex j;
                         for (j = choleskyStart_[i]; j < choleskyStart_[i+1]; j++) {
                              int iRow = choleskyRow_[j+offset];
                              workDouble_[iRow] -= sparseFactor_[j] * value;
                         }
                    }
                    for (i = 0; i < numberRows_; i++) {
                         int iRow = permute_[i];
                         a[iRow] = workDouble_[i] * diagonal_[i];
                    }
               }
               dense_->resetRowsDropped();
               longDouble * denseBlob = dense_->aMatrix();
               longDouble * denseDiagonal = dense_->diagonal();
               // Update dense matrix
               for (i = 0; i < numberDense; i++) {
                    const longDouble * a = denseColumn_ + i * numberRows_;
                    // do diagonal
                    CoinWorkDouble value = denseDiagonal[i];
                    const longDouble * b = denseColumn_ + i * numberRows_;
                    for (int k = 0; k < numberRows_; k++)
                         value += a[k] * b[k];
                    denseDiagonal[i] = value;
                    for (int j = i + 1; j < numberDense; j++) {
                         CoinWorkDouble value = 0.0;
                         const longDouble * b = denseColumn_ + j * numberRows_;
                         for (int k = 0; k < numberRows_; k++)
                              value += a[k] * b[k];
                         *denseBlob = value;
                         denseBlob++;
                    }
               }
               // dense cholesky (? long double)
               int * dropped = new int [numberDense];
               dense_->factorizePart2(dropped);
               delete [] dropped;
          }
          // try allowing all every time
          //printf("trying ?\n");
          //for (iRow=0;iRow<numberRows_;iRow++) {
          //rowsDropped[iRow]=0;
          //rowsDropped_[iRow]=0;
          //}
          bool cleanCholesky;
          //if (model_->numberIterations()<20||(model_->numberIterations()&1)==0)
          if (model_->numberIterations() < 2000)
               cleanCholesky = true;
          else
               cleanCholesky = false;
          if (cleanCholesky) {
               //drop fresh makes some formADAT easier
               if (newDropped || numberRowsDropped_) {
                    newDropped = 0;
                    for (int i = 0; i < numberRows_; i++) {
                         char dropped = static_cast<char>(rowsDropped[i]);
                         rowsDropped_[i] = dropped;
                         rowsDropped_[i] = 0;
                         if (dropped == 2) {
                              //dropped this time
                              rowsDropped[newDropped++] = i;
                              rowsDropped_[i] = 0;
                         }
                    }
                    numberRowsDropped_ = newDropped;
                    newDropped = -(2 + newDropped);
               }
          } else {
               if (newDropped) {
                    newDropped = 0;
                    for (int i = 0; i < numberRows_; i++) {
                         char dropped = static_cast<char>(rowsDropped[i]);
                         rowsDropped_[i] = dropped;
                         if (dropped == 2) {
                              //dropped this time
                              rowsDropped[newDropped++] = i;
                              rowsDropped_[i] = 1;
                         }
                    }
               }
               numberRowsDropped_ += newDropped;
               if (numberRowsDropped_ && 0) {
                    std::cout << "Rank " << numberRows_ - numberRowsDropped_ << " ( " <<
                              numberRowsDropped_ << " dropped)";
                    if (newDropped) {
                         std::cout << " ( " << newDropped << " dropped this time)";
                    }
                    std::cout << std::endl;
               }
          }
     } else {
          //KKT
          CoinPackedMatrix * quadratic = NULL;
          ClpQuadraticObjective * quadraticObj =
               (dynamic_cast< ClpQuadraticObjective*>(model_->objectiveAsObject()));
          if (quadraticObj)
               quadratic = quadraticObj->quadraticObjective();
          // matrix
          int numberRowsModel = model_->numberRows();
          int numberColumns = model_->numberColumns();
          int numberTotal = numberColumns + numberRowsModel;
          // temp
          bool permuted = false;
          for (iRow = 0; iRow < numberRows_; iRow++) {
               if (permute_[iRow] != iRow) {
                    permuted = true;
                    break;
               }
          }
          // but fake it
          for (iRow = 0; iRow < numberRows_; iRow++) {
               //permute_[iRow]=iRow; // force no permute
               //permuteInverse_[permute_[iRow]]=iRow;
          }
          if (permuted) {
               CoinWorkDouble delta2 = model_->delta(); // add delta*delta to bottom
               delta2 *= delta2;
               // Need to permute - ugly
               if (!quadratic) {
                    for (iRow = 0; iRow < numberRows_; iRow++) {
                         longDouble * put = sparseFactor_ + choleskyStart_[iRow];
                         int * which = choleskyRow_ + indexStart_[iRow];
                         int iOriginalRow = permute_[iRow];
                         if (iOriginalRow < numberColumns) {
                              iColumn = iOriginalRow;
                              CoinWorkDouble value = diagonal[iColumn];
                              if (CoinAbs(value) > 1.0e-100) {
                                   value = 1.0 / value;
                                   largest = CoinMax(largest, CoinAbs(value));
                                   diagonal_[iRow] = -value;
                                   CoinBigIndex start = columnStart[iColumn];
                                   CoinBigIndex end = columnStart[iColumn] + columnLength[iColumn];
                                   for (CoinBigIndex j = start; j < end; j++) {
                                        int kRow = row[j] + numberTotal;
                                        kRow = permuteInverse_[kRow];
                                        if (kRow > iRow) {
                                             work[kRow] = element[j];
                                             largest = CoinMax(largest, CoinAbs(element[j]));
                                        }
                                   }
                              } else {
                                   diagonal_[iRow] = -value;
                              }
                         } else if (iOriginalRow < numberTotal) {
                              CoinWorkDouble value = diagonal[iOriginalRow];
                              if (CoinAbs(value) > 1.0e-100) {
                                   value = 1.0 / value;
                                   largest = CoinMax(largest, CoinAbs(value));
                              } else {
                                   value = 1.0e100;
                              }
                              diagonal_[iRow] = -value;
                              int kRow = permuteInverse_[iOriginalRow+numberRowsModel];
                              if (kRow > iRow)
                                   work[kRow] = -1.0;
                         } else {
                              diagonal_[iRow] = delta2;
                              int kRow = iOriginalRow - numberTotal;
                              CoinBigIndex start = rowStart[kRow];
                              CoinBigIndex end = rowStart[kRow] + rowLength[kRow];
                              for (CoinBigIndex j = start; j < end; j++) {
                                   int jRow = column[j];
                                   int jNewRow = permuteInverse_[jRow];
                                   if (jNewRow > iRow) {
                                        work[jNewRow] = elementByRow[j];
                                        largest = CoinMax(largest, CoinAbs(elementByRow[j]));
                                   }
                              }
                              // slack - should it be permute
                              kRow = permuteInverse_[kRow+numberColumns];
                              if (kRow > iRow)
                                   work[kRow] = -1.0;
                         }
                         CoinBigIndex j;
                         int number = choleskyStart_[iRow+1] - choleskyStart_[iRow];
                         for (j = 0; j < number; j++) {
                              int jRow = which[j];
                              put[j] = work[jRow];
                              work[jRow] = 0.0;
                         }
                    }
               } else {
                    // quadratic
                    const int * columnQuadratic = quadratic->getIndices();
                    const CoinBigIndex * columnQuadraticStart = quadratic->getVectorStarts();
                    const int * columnQuadraticLength = quadratic->getVectorLengths();
                    const double * quadraticElement = quadratic->getElements();
                    for (iRow = 0; iRow < numberRows_; iRow++) {
                         longDouble * put = sparseFactor_ + choleskyStart_[iRow];
                         int * which = choleskyRow_ + indexStart_[iRow];
                         int iOriginalRow = permute_[iRow];
                         if (iOriginalRow < numberColumns) {
                              CoinBigIndex j;
                              iColumn = iOriginalRow;
                              CoinWorkDouble value = diagonal[iColumn];
                              if (CoinAbs(value) > 1.0e-100) {
                                   value = 1.0 / value;
                                   for (j = columnQuadraticStart[iColumn];
                                             j < columnQuadraticStart[iColumn] + columnQuadraticLength[iColumn]; j++) {
                                        int jColumn = columnQuadratic[j];
                                        int jNewColumn = permuteInverse_[jColumn];
                                        if (jNewColumn > iRow) {
                                             work[jNewColumn] = -quadraticElement[j];
                                        } else if (iColumn == jColumn) {
                                             value += quadraticElement[j];
                                        }
                                   }
                                   largest = CoinMax(largest, CoinAbs(value));
                                   diagonal_[iRow] = -value;
                                   CoinBigIndex start = columnStart[iColumn];
                                   CoinBigIndex end = columnStart[iColumn] + columnLength[iColumn];
                                   for (j = start; j < end; j++) {
                                        int kRow = row[j] + numberTotal;
                                        kRow = permuteInverse_[kRow];
                                        if (kRow > iRow) {
                                             work[kRow] = element[j];
                                             largest = CoinMax(largest, CoinAbs(element[j]));
                                        }
                                   }
                              } else {
                                   diagonal_[iRow] = -value;
                              }
                         } else if (iOriginalRow < numberTotal) {
                              CoinWorkDouble value = diagonal[iOriginalRow];
                              if (CoinAbs(value) > 1.0e-100) {
                                   value = 1.0 / value;
                                   largest = CoinMax(largest, CoinAbs(value));
                              } else {
                                   value = 1.0e100;
                              }
                              diagonal_[iRow] = -value;
                              int kRow = permuteInverse_[iOriginalRow+numberRowsModel];
                              if (kRow > iRow)
                                   work[kRow] = -1.0;
                         } else {
                              diagonal_[iRow] = delta2;
                              int kRow = iOriginalRow - numberTotal;
                              CoinBigIndex start = rowStart[kRow];
                              CoinBigIndex end = rowStart[kRow] + rowLength[kRow];
                              for (CoinBigIndex j = start; j < end; j++) {
                                   int jRow = column[j];
                                   int jNewRow = permuteInverse_[jRow];
                                   if (jNewRow > iRow) {
                                        work[jNewRow] = elementByRow[j];
                                        largest = CoinMax(largest, CoinAbs(elementByRow[j]));
                                   }
                              }
                              // slack - should it be permute
                              kRow = permuteInverse_[kRow+numberColumns];
                              if (kRow > iRow)
                                   work[kRow] = -1.0;
                         }
                         CoinBigIndex j;
                         int number = choleskyStart_[iRow+1] - choleskyStart_[iRow];
                         for (j = 0; j < number; j++) {
                              int jRow = which[j];
                              put[j] = work[jRow];
                              work[jRow] = 0.0;
                         }
                         for (j = 0; j < numberRows_; j++)
                              assert (!work[j]);
                    }
               }
          } else {
               if (!quadratic) {
                    for (iColumn = 0; iColumn < numberColumns; iColumn++) {
                         longDouble * put = sparseFactor_ + choleskyStart_[iColumn];
                         int * which = choleskyRow_ + indexStart_[iColumn];
                         CoinWorkDouble value = diagonal[iColumn];
                         if (CoinAbs(value) > 1.0e-100) {
                              value = 1.0 / value;
                              largest = CoinMax(largest, CoinAbs(value));
                              diagonal_[iColumn] = -value;
                              CoinBigIndex start = columnStart[iColumn];
                              CoinBigIndex end = columnStart[iColumn] + columnLength[iColumn];
                              for (CoinBigIndex j = start; j < end; j++) {
                                   //choleskyRow_[numberElements]=row[j]+numberTotal;
                                   //sparseFactor_[numberElements++]=element[j];
                                   work[row[j] + numberTotal] = element[j];
                                   largest = CoinMax(largest, CoinAbs(element[j]));
                              }
                         } else {
                              diagonal_[iColumn] = -value;
                         }
                         CoinBigIndex j;
                         int number = choleskyStart_[iColumn+1] - choleskyStart_[iColumn];
                         for (j = 0; j < number; j++) {
                              int jRow = which[j];
                              put[j] = work[jRow];
                              work[jRow] = 0.0;
                         }
                    }
               } else {
                    // Quadratic
                    const int * columnQuadratic = quadratic->getIndices();
                    const CoinBigIndex * columnQuadraticStart = quadratic->getVectorStarts();
                    const int * columnQuadraticLength = quadratic->getVectorLengths();
                    const double * quadraticElement = quadratic->getElements();
                    for (iColumn = 0; iColumn < numberColumns; iColumn++) {
                         longDouble * put = sparseFactor_ + choleskyStart_[iColumn];
                         int * which = choleskyRow_ + indexStart_[iColumn];
                         int number = choleskyStart_[iColumn+1] - choleskyStart_[iColumn];
                         CoinWorkDouble value = diagonal[iColumn];
                         CoinBigIndex j;
                         if (CoinAbs(value) > 1.0e-100) {
                              value = 1.0 / value;
                              for (j = columnQuadraticStart[iColumn];
                                        j < columnQuadraticStart[iColumn] + columnQuadraticLength[iColumn]; j++) {
                                   int jColumn = columnQuadratic[j];
                                   if (jColumn > iColumn) {
                                        work[jColumn] = -quadraticElement[j];
                                   } else if (iColumn == jColumn) {
                                        value += quadraticElement[j];
                                   }
                              }
                              largest = CoinMax(largest, CoinAbs(value));
                              diagonal_[iColumn] = -value;
                              CoinBigIndex start = columnStart[iColumn];
                              CoinBigIndex end = columnStart[iColumn] + columnLength[iColumn];
                              for (j = start; j < end; j++) {
                                   work[row[j] + numberTotal] = element[j];
                                   largest = CoinMax(largest, CoinAbs(element[j]));
                              }
                              for (j = 0; j < number; j++) {
                                   int jRow = which[j];
                                   put[j] = work[jRow];
                                   work[jRow] = 0.0;
                              }
                         } else {
                              value = 1.0e100;
                              diagonal_[iColumn] = -value;
                              for (j = 0; j < number; j++) {
                                   int jRow = which[j];
                                   put[j] = work[jRow];
                              }
                         }
                    }
               }
               // slacks
               for (iColumn = numberColumns; iColumn < numberTotal; iColumn++) {
                    longDouble * put = sparseFactor_ + choleskyStart_[iColumn];
                    int * which = choleskyRow_ + indexStart_[iColumn];
                    CoinWorkDouble value = diagonal[iColumn];
                    if (CoinAbs(value) > 1.0e-100) {
                         value = 1.0 / value;
                         largest = CoinMax(largest, CoinAbs(value));
                    } else {
                         value = 1.0e100;
                    }
                    diagonal_[iColumn] = -value;
                    work[iColumn-numberColumns+numberTotal] = -1.0;
                    CoinBigIndex j;
                    int number = choleskyStart_[iColumn+1] - choleskyStart_[iColumn];
                    for (j = 0; j < number; j++) {
                         int jRow = which[j];
                         put[j] = work[jRow];
                         work[jRow] = 0.0;
                    }
               }
               // Finish diagonal
               CoinWorkDouble delta2 = model_->delta(); // add delta*delta to bottom
               delta2 *= delta2;
               for (iRow = 0; iRow < numberRowsModel; iRow++) {
                    longDouble * put = sparseFactor_ + choleskyStart_[iRow+numberTotal];
                    diagonal_[iRow+numberTotal] = delta2;
                    CoinBigIndex j;
                    int number = choleskyStart_[iRow+numberTotal+1] - choleskyStart_[iRow+numberTotal];
                    for (j = 0; j < number; j++) {
                         put[j] = 0.0;
                    }
               }
          }
          //check sizes
          largest *= 1.0e-20;
          largest = CoinMin(largest, CHOL_SMALL_VALUE);
          doubleParameters_[10] = CoinMax(1.0e-20, largest);
          integerParameters_[20] = 0;
          doubleParameters_[3] = 0.0;
          doubleParameters_[4] = COIN_DBL_MAX;
          // Set up LDL cutoff
          integerParameters_[34] = numberTotal;
          // KKT
          int * rowsDropped2 = new int[numberRows_];
          CoinZeroN(rowsDropped2, numberRows_);
          factorizePart2(rowsDropped2);
          newDropped = integerParameters_[20];
          largest = doubleParameters_[3];
          smallest = doubleParameters_[4];
          if (model_->messageHandler()->logLevel() > 1)
               std::cout << "Cholesky - largest " << largest << " smallest " << smallest << std::endl;
          choleskyCondition_ = largest / smallest;
          // Should save adjustments in ..R_
          int n1 = 0, n2 = 0;
          CoinWorkDouble * primalR = model_->primalR();
          CoinWorkDouble * dualR = model_->dualR();
          for (iRow = 0; iRow < numberTotal; iRow++) {
               if (rowsDropped2[iRow]) {
                    n1++;
                    //printf("row region1 %d dropped\n",iRow);
                    //rowsDropped_[iRow]=1;
                    rowsDropped_[iRow] = 0;
                    primalR[iRow] = doubleParameters_[20];
               } else {
                    rowsDropped_[iRow] = 0;
                    primalR[iRow] = 0.0;
               }
          }
          for (; iRow < numberRows_; iRow++) {
               if (rowsDropped2[iRow]) {
                    n2++;
                    //printf("row region2 %d dropped\n",iRow);
                    //rowsDropped_[iRow]=1;
                    rowsDropped_[iRow] = 0;
                    dualR[iRow-numberTotal] = doubleParameters_[34];
               } else {
                    rowsDropped_[iRow] = 0;
                    dualR[iRow-numberTotal] = 0.0;
               }
          }
          delete [] rowsDropped2;
     }
     status_ = 0;
     return newDropped;
}
/* Factorize - filling in rowsDropped and returning number dropped
   in integerParam.
*/
void
ClpCholeskyBase::factorizePart2(int * rowsDropped)
{
     CoinWorkDouble largest = doubleParameters_[3];
     CoinWorkDouble smallest = doubleParameters_[4];
     // probably done before
     largest = 0.0;
     smallest = COIN_DBL_MAX;
     double dropValue = doubleParameters_[10];
     int firstPositive = integerParameters_[34];
     longDouble * d = ClpCopyOfArray(diagonal_, numberRows_);
     int iRow;
     // minimum size before clique done
     //#define MINCLIQUE INT_MAX
#define MINCLIQUE 3
     longDouble * work = workDouble_;
     CoinBigIndex * first = workInteger_;

     for (iRow = 0; iRow < numberRows_; iRow++) {
          link_[iRow] = -1;
          work[iRow] = 0.0;
          first[iRow] = choleskyStart_[iRow];
     }

     int lastClique = -1;
     bool inClique = false;
     bool newClique = false;
     bool endClique = false;
     int lastRow = 0;
     int nextRow2 = -1;

     for (iRow = 0; iRow < firstDense_ + 1; iRow++) {
          if (iRow < firstDense_) {
               endClique = false;
               if (clique_[iRow] > 0) {
                    // this is in a clique
                    inClique = true;
                    if (clique_[iRow] > lastClique) {
                         // new Clique
                         newClique = true;
                         // If we have clique going then signal to do old one
                         endClique = (lastClique > 0);
                    } else {
                         // Still in clique
                         newClique = false;
                    }
               } else {
                    // not in clique
                    inClique = false;
                    newClique = false;
                    // If we have clique going then signal to do old one
                    endClique = (lastClique > 0);
               }
               lastClique = clique_[iRow];
          } else if (inClique) {
               // Finish off
               endClique = true;
          } else {
               break;
          }
          if (endClique) {
               // We have just finished updating a clique - do block pivot and clean up
               int jRow;
               for ( jRow = lastRow; jRow < iRow; jRow++) {
                    int jCount = jRow - lastRow;
                    CoinWorkDouble diagonalValue = diagonal_[jRow];
                    CoinBigIndex start = choleskyStart_[jRow];
                    CoinBigIndex end = choleskyStart_[jRow+1];
                    for (int kRow = lastRow; kRow < jRow; kRow++) {
                         jCount--;
                         CoinBigIndex get = choleskyStart_[kRow] + jCount;
                         CoinWorkDouble a_jk = sparseFactor_[get];
                         CoinWorkDouble value1 = d[kRow] * a_jk;
                         diagonalValue -= a_jk * value1;
                         for (CoinBigIndex j = start; j < end; j++)
                              sparseFactor_[j] -= value1 * sparseFactor_[++get];
                    }
                    // check
                    int originalRow = permute_[jRow];
                    if (originalRow < firstPositive) {
                         // must be negative
                         if (diagonalValue <= -dropValue) {
                              smallest = CoinMin(smallest, -diagonalValue);
                              largest = CoinMax(largest, -diagonalValue);
                              d[jRow] = diagonalValue;
                              diagonalValue = 1.0 / diagonalValue;
                         } else {
                              rowsDropped[originalRow] = 2;
                              d[jRow] = -1.0e100;
                              diagonalValue = 0.0;
                              integerParameters_[20]++;
                         }
                    } else {
                         // must be positive
                         if (diagonalValue >= dropValue) {
                              smallest = CoinMin(smallest, diagonalValue);
                              largest = CoinMax(largest, diagonalValue);
                              d[jRow] = diagonalValue;
                              diagonalValue = 1.0 / diagonalValue;
                         } else {
                              rowsDropped[originalRow] = 2;
                              d[jRow] = 1.0e100;
                              diagonalValue = 0.0;
                              integerParameters_[20]++;
                         }
                    }
                    diagonal_[jRow] = diagonalValue;
                    for (CoinBigIndex j = start; j < end; j++) {
                         sparseFactor_[j] *= diagonalValue;
                    }
               }
               if (nextRow2 >= 0) {
                    for ( jRow = lastRow; jRow < iRow - 1; jRow++) {
                         link_[jRow] = jRow + 1;
                    }
                    link_[iRow-1] = link_[nextRow2];
                    link_[nextRow2] = lastRow;
               }
          }
          if (iRow == firstDense_)
               break; // we were just cleaning up
          if (newClique) {
               // initialize new clique
               lastRow = iRow;
          }
          // for each column L[*,kRow] that affects L[*,iRow]
          CoinWorkDouble diagonalValue = diagonal_[iRow];
          int nextRow = link_[iRow];
          int kRow = 0;
          while (1) {
               kRow = nextRow;
               if (kRow < 0)
                    break; // out of loop
               nextRow = link_[kRow];
               // Modify by outer product of L[*,irow] by L[*,krow] from first
               CoinBigIndex k = first[kRow];
               CoinBigIndex end = choleskyStart_[kRow+1];
               assert(k < end);
               CoinWorkDouble a_ik = sparseFactor_[k++];
               CoinWorkDouble value1 = d[kRow] * a_ik;
               // update first
               first[kRow] = k;
               diagonalValue -= value1 * a_ik;
               CoinBigIndex offset = indexStart_[kRow] - choleskyStart_[kRow];
               if (k < end) {
                    int jRow = choleskyRow_[k+offset];
                    if (clique_[kRow] < MINCLIQUE) {
                         link_[kRow] = link_[jRow];
                         link_[jRow] = kRow;
                         for (; k < end; k++) {
                              int jRow = choleskyRow_[k+offset];
                              work[jRow] += sparseFactor_[k] * value1;
                         }
                    } else {
                         // Clique
                         CoinBigIndex currentIndex = k + offset;
                         int linkSave = link_[jRow];
                         link_[jRow] = kRow;
                         work[kRow] = value1; // ? or a_jk
                         int last = kRow + clique_[kRow];
                         for (int kkRow = kRow + 1; kkRow < last; kkRow++) {
                              CoinBigIndex j = first[kkRow];
                              //int iiRow = choleskyRow_[j+indexStart_[kkRow]-choleskyStart_[kkRow]];
                              CoinWorkDouble a = sparseFactor_[j];
                              CoinWorkDouble dValue = d[kkRow] * a;
                              diagonalValue -= a * dValue;
                              work[kkRow] = dValue;
                              first[kkRow]++;
                              link_[kkRow-1] = kkRow;
                         }
                         nextRow = link_[last-1];
                         link_[last-1] = linkSave;
                         int length = end - k;
                         for (int i = 0; i < length; i++) {
                              int lRow = choleskyRow_[currentIndex++];
                              CoinWorkDouble t0 = work[lRow];
                              for (int kkRow = kRow; kkRow < last; kkRow++) {
                                   CoinBigIndex j = first[kkRow] + i;
                                   t0 += work[kkRow] * sparseFactor_[j];
                              }
                              work[lRow] = t0;
                         }
                    }
               }
          }
          // Now apply
          if (inClique) {
               // in clique
               diagonal_[iRow] = diagonalValue;
               CoinBigIndex start = choleskyStart_[iRow];
               CoinBigIndex end = choleskyStart_[iRow+1];
               CoinBigIndex currentIndex = indexStart_[iRow];
               nextRow2 = -1;
               CoinBigIndex get = start + clique_[iRow] - 1;
               if (get < end) {
                    nextRow2 = choleskyRow_[currentIndex+get-start];
                    first[iRow] = get;
               }
               for (CoinBigIndex j = start; j < end; j++) {
                    int kRow = choleskyRow_[currentIndex++];
                    sparseFactor_[j] -= work[kRow]; // times?
                    work[kRow] = 0.0;
               }
          } else {
               // not in clique
               int originalRow = permute_[iRow];
               if (originalRow < firstPositive) {
                    // must be negative
                    if (diagonalValue <= -dropValue) {
                         smallest = CoinMin(smallest, -diagonalValue);
                         largest = CoinMax(largest, -diagonalValue);
                         d[iRow] = diagonalValue;
                         diagonalValue = 1.0 / diagonalValue;
                    } else {
                         rowsDropped[originalRow] = 2;
                         d[iRow] = -1.0e100;
                         diagonalValue = 0.0;
                         integerParameters_[20]++;
                    }
               } else {
                    // must be positive
                    if (diagonalValue >= dropValue) {
                         smallest = CoinMin(smallest, diagonalValue);
                         largest = CoinMax(largest, diagonalValue);
                         d[iRow] = diagonalValue;
                         diagonalValue = 1.0 / diagonalValue;
                    } else {
                         rowsDropped[originalRow] = 2;
                         d[iRow] = 1.0e100;
                         diagonalValue = 0.0;
                         integerParameters_[20]++;
                    }
               }
               diagonal_[iRow] = diagonalValue;
               CoinBigIndex offset = indexStart_[iRow] - choleskyStart_[iRow];
               CoinBigIndex start = choleskyStart_[iRow];
               CoinBigIndex end = choleskyStart_[iRow+1];
               assert (first[iRow] == start);
               if (start < end) {
                    int nextRow = choleskyRow_[start+offset];
                    link_[iRow] = link_[nextRow];
                    link_[nextRow] = iRow;
                    for (CoinBigIndex j = start; j < end; j++) {
                         int jRow = choleskyRow_[j+offset];
                         CoinWorkDouble value = sparseFactor_[j] - work[jRow];
                         work[jRow] = 0.0;
                         sparseFactor_[j] = diagonalValue * value;
                    }
               }
          }
     }
     if (firstDense_ < numberRows_) {
          // do dense
          // update dense part
          updateDense(d,/*work,*/first);
          ClpCholeskyDense dense;
          // just borrow space
          int nDense = numberRows_ - firstDense_;
          if (doKKT_) {
               for (iRow = firstDense_; iRow < numberRows_; iRow++) {
                    int originalRow = permute_[iRow];
                    if (originalRow >= firstPositive) {
                         firstPositive = iRow - firstDense_;
                         break;
                    }
               }
          }
          dense.reserveSpace(this, nDense);
          int * dropped = new int[nDense];
          memset(dropped, 0, nDense * sizeof(int));
          dense.setDoubleParameter(3, largest);
          dense.setDoubleParameter(4, smallest);
          dense.setDoubleParameter(10, dropValue);
          dense.setIntegerParameter(20, 0);
          dense.setIntegerParameter(34, firstPositive);
          dense.setModel(model_);
          dense.factorizePart2(dropped);
          largest = dense.getDoubleParameter(3);
          smallest = dense.getDoubleParameter(4);
          integerParameters_[20] += dense.getIntegerParameter(20);
          for (iRow = firstDense_; iRow < numberRows_; iRow++) {
               int originalRow = permute_[iRow];
               rowsDropped[originalRow] = dropped[iRow-firstDense_];
          }
          delete [] dropped;
     }
     delete [] d;
     doubleParameters_[3] = largest;
     doubleParameters_[4] = smallest;
     return;
}
// Updates dense part (broken out for profiling)
void ClpCholeskyBase::updateDense(longDouble * d, /*longDouble * work,*/ int * first)
{
     for (int iRow = 0; iRow < firstDense_; iRow++) {
          CoinBigIndex start = first[iRow];
          CoinBigIndex end = choleskyStart_[iRow+1];
          if (start < end) {
               CoinBigIndex offset = indexStart_[iRow] - choleskyStart_[iRow];
               if (clique_[iRow] < 2) {
                    CoinWorkDouble dValue = d[iRow];
                    for (CoinBigIndex k = start; k < end; k++) {
                         int kRow = choleskyRow_[k+offset];
                         assert(kRow >= firstDense_);
                         CoinWorkDouble a_ik = sparseFactor_[k];
                         CoinWorkDouble value1 = dValue * a_ik;
                         diagonal_[kRow] -= value1 * a_ik;
                         CoinBigIndex base = choleskyStart_[kRow] - kRow - 1;
                         for (CoinBigIndex j = k + 1; j < end; j++) {
                              int jRow = choleskyRow_[j+offset];
                              CoinWorkDouble a_jk = sparseFactor_[j];
                              sparseFactor_[base+jRow] -= a_jk * value1;
                         }
                    }
               } else if (clique_[iRow] < 3) {
                    // do as pair
                    CoinWorkDouble dValue0 = d[iRow];
                    CoinWorkDouble dValue1 = d[iRow+1];
                    int offset1 = first[iRow+1] - start;
                    // skip row
                    iRow++;
                    for (CoinBigIndex k = start; k < end; k++) {
                         int kRow = choleskyRow_[k+offset];
                         assert(kRow >= firstDense_);
                         CoinWorkDouble a_ik0 = sparseFactor_[k];
                         CoinWorkDouble value0 = dValue0 * a_ik0;
                         CoinWorkDouble a_ik1 = sparseFactor_[k+offset1];
                         CoinWorkDouble value1 = dValue1 * a_ik1;
                         diagonal_[kRow] -= value0 * a_ik0 + value1 * a_ik1;
                         CoinBigIndex base = choleskyStart_[kRow] - kRow - 1;
                         for (CoinBigIndex j = k + 1; j < end; j++) {
                              int jRow = choleskyRow_[j+offset];
                              CoinWorkDouble a_jk0 = sparseFactor_[j];
                              CoinWorkDouble a_jk1 = sparseFactor_[j+offset1];
                              sparseFactor_[base+jRow] -= a_jk0 * value0 + a_jk1 * value1;
                         }
                    }
#define MANY_REGISTERS
#ifdef MANY_REGISTERS
               } else if (clique_[iRow] == 3) {
#else
               } else {
#endif
                    // do as clique
                    // maybe later get fancy on big cliques and do transpose copy
                    // seems only worth going to 3 on Intel
                    CoinWorkDouble dValue0 = d[iRow];
                    CoinWorkDouble dValue1 = d[iRow+1];
                    CoinWorkDouble dValue2 = d[iRow+2];
                    // get offsets and skip rows
                    int offset1 = first[++iRow] - start;
                    int offset2 = first[++iRow] - start;
                    for (CoinBigIndex k = start; k < end; k++) {
                         int kRow = choleskyRow_[k+offset];
                         assert(kRow >= firstDense_);
                         CoinWorkDouble diagonalValue = diagonal_[kRow];
                         CoinWorkDouble a_ik0 = sparseFactor_[k];
                         CoinWorkDouble value0 = dValue0 * a_ik0;
                         CoinWorkDouble a_ik1 = sparseFactor_[k+offset1];
                         CoinWorkDouble value1 = dValue1 * a_ik1;
                         CoinWorkDouble a_ik2 = sparseFactor_[k+offset2];
                         CoinWorkDouble value2 = dValue2 * a_ik2;
                         CoinBigIndex base = choleskyStart_[kRow] - kRow - 1;
                         diagonal_[kRow] = diagonalValue - value0 * a_ik0 - value1 * a_ik1 - value2 * a_ik2;
                         for (CoinBigIndex j = k + 1; j < end; j++) {
                              int jRow = choleskyRow_[j+offset];
                              CoinWorkDouble a_jk0 = sparseFactor_[j];
                              CoinWorkDouble a_jk1 = sparseFactor_[j+offset1];
                              CoinWorkDouble a_jk2 = sparseFactor_[j+offset2];
                              sparseFactor_[base+jRow] -= a_jk0 * value0 + a_jk1 * value1 + a_jk2 * value2;
                         }
                    }
#ifdef MANY_REGISTERS
               }
               else {
                    // do as clique
                    // maybe later get fancy on big cliques and do transpose copy
                    // maybe only worth going to 3 on Intel (but may have hidden registers)
                    CoinWorkDouble dValue0 = d[iRow];
                    CoinWorkDouble dValue1 = d[iRow+1];
                    CoinWorkDouble dValue2 = d[iRow+2];
                    CoinWorkDouble dValue3 = d[iRow+3];
                    // get offsets and skip rows
                    int offset1 = first[++iRow] - start;
                    int offset2 = first[++iRow] - start;
                    int offset3 = first[++iRow] - start;
                    for (CoinBigIndex k = start; k < end; k++) {
                         int kRow = choleskyRow_[k+offset];
                         assert(kRow >= firstDense_);
                         CoinWorkDouble diagonalValue = diagonal_[kRow];
                         CoinWorkDouble a_ik0 = sparseFactor_[k];
                         CoinWorkDouble value0 = dValue0 * a_ik0;
                         CoinWorkDouble a_ik1 = sparseFactor_[k+offset1];
                         CoinWorkDouble value1 = dValue1 * a_ik1;
                         CoinWorkDouble a_ik2 = sparseFactor_[k+offset2];
                         CoinWorkDouble value2 = dValue2 * a_ik2;
                         CoinWorkDouble a_ik3 = sparseFactor_[k+offset3];
                         CoinWorkDouble value3 = dValue3 * a_ik3;
                         CoinBigIndex base = choleskyStart_[kRow] - kRow - 1;
                         diagonal_[kRow] = diagonalValue - (value0 * a_ik0 + value1 * a_ik1 + value2 * a_ik2 + value3 * a_ik3);
                         for (CoinBigIndex j = k + 1; j < end; j++) {
                              int jRow = choleskyRow_[j+offset];
                              CoinWorkDouble a_jk0 = sparseFactor_[j];
                              CoinWorkDouble a_jk1 = sparseFactor_[j+offset1];
                              CoinWorkDouble a_jk2 = sparseFactor_[j+offset2];
                              CoinWorkDouble a_jk3 = sparseFactor_[j+offset3];
                              sparseFactor_[base+jRow] -= a_jk0 * value0 + a_jk1 * value1 + a_jk2 * value2 + a_jk3 * value3;
                         }
                    }
#endif
               }
          }
     }
}
/* Uses factorization to solve. */
void
ClpCholeskyBase::solve (CoinWorkDouble * region)
{
     if (!whichDense_) {
          solve(region, 3);
     } else {
          // dense columns
          int i;
          solve(region, 1);
          // do change;
          int numberDense = dense_->numberRows();
          CoinWorkDouble * change = new CoinWorkDouble[numberDense];
          for (i = 0; i < numberDense; i++) {
               const longDouble * a = denseColumn_ + i * numberRows_;
               CoinWorkDouble value = 0.0;
               for (int iRow = 0; iRow < numberRows_; iRow++)
                    value += a[iRow] * region[iRow];
               change[i] = value;
          }
          // solve
          dense_->solve(change);
          for (i = 0; i < numberDense; i++) {
               const longDouble * a = denseColumn_ + i * numberRows_;
               CoinWorkDouble value = change[i];
               for (int iRow = 0; iRow < numberRows_; iRow++)
                    region[iRow] -= value * a[iRow];
          }
          delete [] change;
          // and finish off
          solve(region, 2);
     }
}
/* solve - 1 just first half, 2 just second half - 3 both.
   If 1 and 2 then diagonal has sqrt of inverse otherwise inverse
*/
void
ClpCholeskyBase::solve(CoinWorkDouble * region, int type)
{
#ifdef CLP_DEBUG
     double * regionX = NULL;
     if (sizeof(CoinWorkDouble) != sizeof(double) && type == 3) {
          regionX = ClpCopyOfArray(region, numberRows_);
     }
#endif
     CoinWorkDouble * work = reinterpret_cast<CoinWorkDouble *> (workDouble_);
     int i;
     CoinBigIndex j;
     for (i = 0; i < numberRows_; i++) {
          int iRow = permute_[i];
          work[i] = region[iRow];
     }
     switch (type) {
     case 1:
          for (i = 0; i < numberRows_; i++) {
               CoinWorkDouble value = work[i];
               CoinBigIndex offset = indexStart_[i] - choleskyStart_[i];
               for (j = choleskyStart_[i]; j < choleskyStart_[i+1]; j++) {
                    int iRow = choleskyRow_[j+offset];
                    work[iRow] -= sparseFactor_[j] * value;
               }
          }
          for (i = 0; i < numberRows_; i++) {
               int iRow = permute_[i];
               region[iRow] = work[i] * diagonal_[i];
          }
          break;
     case 2:
          for (i = numberRows_ - 1; i >= 0; i--) {
               CoinBigIndex offset = indexStart_[i] - choleskyStart_[i];
               CoinWorkDouble value = work[i] * diagonal_[i];
               for (j = choleskyStart_[i]; j < choleskyStart_[i+1]; j++) {
                    int iRow = choleskyRow_[j+offset];
                    value -= sparseFactor_[j] * work[iRow];
               }
               work[i] = value;
               int iRow = permute_[i];
               region[iRow] = value;
          }
          break;
     case 3:
          for (i = 0; i < firstDense_; i++) {
               CoinBigIndex offset = indexStart_[i] - choleskyStart_[i];
               CoinWorkDouble value = work[i];
               for (j = choleskyStart_[i]; j < choleskyStart_[i+1]; j++) {
                    int iRow = choleskyRow_[j+offset];
                    work[iRow] -= sparseFactor_[j] * value;
               }
          }
          if (firstDense_ < numberRows_) {
               // do dense
               ClpCholeskyDense dense;
               // just borrow space
               int nDense = numberRows_ - firstDense_;
               dense.reserveSpace(this, nDense);
               dense.solve(work + firstDense_);
               for (i = numberRows_ - 1; i >= firstDense_; i--) {
                    CoinWorkDouble value = work[i];
                    int iRow = permute_[i];
                    region[iRow] = value;
               }
          }
          for (i = firstDense_ - 1; i >= 0; i--) {
               CoinBigIndex offset = indexStart_[i] - choleskyStart_[i];
               CoinWorkDouble value = work[i] * diagonal_[i];
               for (j = choleskyStart_[i]; j < choleskyStart_[i+1]; j++) {
                    int iRow = choleskyRow_[j+offset];
                    value -= sparseFactor_[j] * work[iRow];
               }
               work[i] = value;
               int iRow = permute_[i];
               region[iRow] = value;
          }
          break;
     }
#ifdef CLP_DEBUG
     if (regionX) {
          longDouble * work = workDouble_;
          int i;
          CoinBigIndex j;
          double largestO = 0.0;
          for (i = 0; i < numberRows_; i++) {
               largestO = CoinMax(largestO, CoinAbs(regionX[i]));
          }
          for (i = 0; i < numberRows_; i++) {
               int iRow = permute_[i];
               work[i] = regionX[iRow];
          }
          for (i = 0; i < firstDense_; i++) {
               CoinBigIndex offset = indexStart_[i] - choleskyStart_[i];
               CoinWorkDouble value = work[i];
               for (j = choleskyStart_[i]; j < choleskyStart_[i+1]; j++) {
                    int iRow = choleskyRow_[j+offset];
                    work[iRow] -= sparseFactor_[j] * value;
               }
          }
          if (firstDense_ < numberRows_) {
               // do dense
               ClpCholeskyDense dense;
               // just borrow space
               int nDense = numberRows_ - firstDense_;
               dense.reserveSpace(this, nDense);
               dense.solve(work + firstDense_);
               for (i = numberRows_ - 1; i >= firstDense_; i--) {
                    CoinWorkDouble value = work[i];
                    int iRow = permute_[i];
                    regionX[iRow] = value;
               }
          }
          for (i = firstDense_ - 1; i >= 0; i--) {
               CoinBigIndex offset = indexStart_[i] - choleskyStart_[i];
               CoinWorkDouble value = work[i] * diagonal_[i];
               for (j = choleskyStart_[i]; j < choleskyStart_[i+1]; j++) {
                    int iRow = choleskyRow_[j+offset];
                    value -= sparseFactor_[j] * work[iRow];
               }
               work[i] = value;
               int iRow = permute_[i];
               regionX[iRow] = value;
          }
          double largest = 0.0;
          double largestV = 0.0;
          for (i = 0; i < numberRows_; i++) {
               largest = CoinMax(largest, CoinAbs(region[i] - regionX[i]));
               largestV = CoinMax(largestV, CoinAbs(region[i]));
          }
          printf("largest difference %g, largest %g, largest original %g\n",
                 largest, largestV, largestO);
          delete [] regionX;
     }
#endif
}
#if 0 //CLP_LONG_CHOLESKY
/* Uses factorization to solve. */
void
ClpCholeskyBase::solve (CoinWorkDouble * region)
{
     assert (!whichDense_) ;
     CoinWorkDouble * work = reinterpret_cast<CoinWorkDouble *> (workDouble_);
     int i;
     CoinBigIndex j;
     for (i = 0; i < numberRows_; i++) {
          int iRow = permute_[i];
          work[i] = region[iRow];
     }
     for (i = 0; i < firstDense_; i++) {
          CoinBigIndex offset = indexStart_[i] - choleskyStart_[i];
          CoinWorkDouble value = work[i];
          for (j = choleskyStart_[i]; j < choleskyStart_[i+1]; j++) {
               int iRow = choleskyRow_[j+offset];
               work[iRow] -= sparseFactor_[j] * value;
          }
     }
     if (firstDense_ < numberRows_) {
          // do dense
          ClpCholeskyDense dense;
          // just borrow space
          int nDense = numberRows_ - firstDense_;
          dense.reserveSpace(this, nDense);
          dense.solve(work + firstDense_);
          for (i = numberRows_ - 1; i >= firstDense_; i--) {
               CoinWorkDouble value = work[i];
               int iRow = permute_[i];
               region[iRow] = value;
          }
     }
     for (i = firstDense_ - 1; i >= 0; i--) {
          CoinBigIndex offset = indexStart_[i] - choleskyStart_[i];
          CoinWorkDouble value = work[i] * diagonal_[i];
          for (j = choleskyStart_[i]; j < choleskyStart_[i+1]; j++) {
               int iRow = choleskyRow_[j+offset];
               value -= sparseFactor_[j] * work[iRow];
          }
          work[i] = value;
          int iRow = permute_[i];
          region[iRow] = value;
     }
}
#endif
