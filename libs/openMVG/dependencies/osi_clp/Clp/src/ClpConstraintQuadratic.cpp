/* $Id$ */
// Copyright (C) 2007, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include "CoinPragma.hpp"
#include "CoinHelperFunctions.hpp"
#include "CoinIndexedVector.hpp"
#include "ClpSimplex.hpp"
#include "ClpConstraintQuadratic.hpp"
#include "CoinSort.hpp"
//#############################################################################
// Constructors / Destructor / Assignment
//#############################################################################

//-------------------------------------------------------------------
// Default Constructor
//-------------------------------------------------------------------
ClpConstraintQuadratic::ClpConstraintQuadratic ()
     : ClpConstraint()
{
     type_ = 0;
     start_ = NULL;
     column_ = NULL;
     coefficient_ = NULL;
     numberColumns_ = 0;
     numberCoefficients_ = 0;
     numberQuadraticColumns_ = 0;
}

//-------------------------------------------------------------------
// Useful Constructor
//-------------------------------------------------------------------
ClpConstraintQuadratic::ClpConstraintQuadratic (int row, int numberQuadraticColumns ,
          int numberColumns, const CoinBigIndex * start,
          const int * column, const double * coefficient)
     : ClpConstraint()
{
     type_ = 0;
     rowNumber_ = row;
     numberColumns_ = numberColumns;
     numberQuadraticColumns_ = numberQuadraticColumns;
     start_ = CoinCopyOfArray(start, numberQuadraticColumns + 1);
     int numberElements = start_[numberQuadraticColumns_];
     column_ = CoinCopyOfArray(column, numberElements);
     coefficient_ = CoinCopyOfArray(coefficient, numberElements);
     char * mark = new char [numberQuadraticColumns_];
     memset(mark, 0, numberQuadraticColumns_);
     int iColumn;
     for (iColumn = 0; iColumn < numberQuadraticColumns_; iColumn++) {
          CoinBigIndex j;
          for (j = start_[iColumn]; j < start_[iColumn+1]; j++) {
               int jColumn = column_[j];
               if (jColumn >= 0) {
                    assert (jColumn < numberQuadraticColumns_);
                    mark[jColumn] = 1;
               }
               mark[iColumn] = 1;
          }
     }
     numberCoefficients_ = 0;
     for (iColumn = 0; iColumn < numberQuadraticColumns_; iColumn++) {
          if (mark[iColumn])
               numberCoefficients_++;
     }
     delete [] mark;
}

//-------------------------------------------------------------------
// Copy constructor
//-------------------------------------------------------------------
ClpConstraintQuadratic::ClpConstraintQuadratic (const ClpConstraintQuadratic & rhs)
     : ClpConstraint(rhs)
{
     numberColumns_ = rhs.numberColumns_;
     numberCoefficients_ = rhs.numberCoefficients_;
     numberQuadraticColumns_ = rhs.numberQuadraticColumns_;
     start_ = CoinCopyOfArray(rhs.start_, numberQuadraticColumns_ + 1);
     int numberElements = start_[numberQuadraticColumns_];
     column_ = CoinCopyOfArray(rhs.column_, numberElements);
     coefficient_ = CoinCopyOfArray(rhs.coefficient_, numberElements);
}


//-------------------------------------------------------------------
// Destructor
//-------------------------------------------------------------------
ClpConstraintQuadratic::~ClpConstraintQuadratic ()
{
     delete [] start_;
     delete [] column_;
     delete [] coefficient_;
}

//----------------------------------------------------------------
// Assignment operator
//-------------------------------------------------------------------
ClpConstraintQuadratic &
ClpConstraintQuadratic::operator=(const ClpConstraintQuadratic& rhs)
{
     if (this != &rhs) {
          delete [] start_;
          delete [] column_;
          delete [] coefficient_;
          numberColumns_ = rhs.numberColumns_;
          numberCoefficients_ = rhs.numberCoefficients_;
          numberQuadraticColumns_ = rhs.numberQuadraticColumns_;
          start_ = CoinCopyOfArray(rhs.start_, numberQuadraticColumns_ + 1);
          int numberElements = start_[numberQuadraticColumns_];
          column_ = CoinCopyOfArray(rhs.column_, numberElements);
          coefficient_ = CoinCopyOfArray(rhs.coefficient_, numberElements);
     }
     return *this;
}
//-------------------------------------------------------------------
// Clone
//-------------------------------------------------------------------
ClpConstraint * ClpConstraintQuadratic::clone() const
{
     return new ClpConstraintQuadratic(*this);
}

// Returns gradient
int
ClpConstraintQuadratic::gradient(const ClpSimplex * model,
                                 const double * solution,
                                 double * gradient,
                                 double & functionValue,
                                 double & offset,
                                 bool useScaling,
                                 bool refresh) const
{
     if (refresh || !lastGradient_) {
          offset_ = 0.0;
          functionValue_ = 0.0;
          if (!lastGradient_)
               lastGradient_ = new double[numberColumns_];
          CoinZeroN(lastGradient_, numberColumns_);
          bool scaling = (model && model->rowScale() && useScaling);
          if (!scaling) {
               int iColumn;
               for (iColumn = 0; iColumn < numberQuadraticColumns_; iColumn++) {
                    double valueI = solution[iColumn];
                    CoinBigIndex j;
                    for (j = start_[iColumn]; j < start_[iColumn+1]; j++) {
                         int jColumn = column_[j];
                         if (jColumn >= 0) {
                              double valueJ = solution[jColumn];
                              double elementValue = coefficient_[j];
                              if (iColumn != jColumn) {
                                   offset_ -= valueI * valueJ * elementValue;
                                   double gradientI = valueJ * elementValue;
                                   double gradientJ = valueI * elementValue;
                                   lastGradient_[iColumn] += gradientI;
                                   lastGradient_[jColumn] += gradientJ;
                              } else {
                                   offset_ -= 0.5 * valueI * valueI * elementValue;
                                   double gradientI = valueI * elementValue;
                                   lastGradient_[iColumn] += gradientI;
                              }
                         } else {
                              // linear part
                              lastGradient_[iColumn] += coefficient_[j];
                              functionValue_ += valueI * coefficient_[j];
                         }
                    }
               }
               functionValue_ -= offset_;
          } else {
               abort();
               // do scaling
               const double * columnScale = model->columnScale();
               for (int i = 0; i < numberCoefficients_; i++) {
                    int iColumn = column_[i];
                    double value = solution[iColumn]; // already scaled
                    double coefficient = coefficient_[i] * columnScale[iColumn];
                    functionValue_ += value * coefficient;
                    lastGradient_[iColumn] = coefficient;
               }
          }
     }
     functionValue = functionValue_;
     offset = offset_;
     CoinMemcpyN(lastGradient_, numberColumns_, gradient);
     return 0;
}
// Resize constraint
void
ClpConstraintQuadratic::resize(int newNumberColumns)
{
     if (numberColumns_ != newNumberColumns) {
          abort();
#ifndef NDEBUG
          int lastColumn = column_[numberCoefficients_-1];
#endif
          assert (newNumberColumns > lastColumn);
          delete [] lastGradient_;
          lastGradient_ = NULL;
          numberColumns_ = newNumberColumns;
     }
}
// Delete columns in  constraint
void
ClpConstraintQuadratic::deleteSome(int numberToDelete, const int * which)
{
     if (numberToDelete) {
          abort();
          int i ;
          char * deleted = new char[numberColumns_];
          memset(deleted, 0, numberColumns_ * sizeof(char));
          for (i = 0; i < numberToDelete; i++) {
               int j = which[i];
               if (j >= 0 && j < numberColumns_ && !deleted[j]) {
                    deleted[j] = 1;
               }
          }
          int n = 0;
          for (i = 0; i < numberCoefficients_; i++) {
               int iColumn = column_[i];
               if (!deleted[iColumn]) {
                    column_[n] = iColumn;
                    coefficient_[n++] = coefficient_[i];
               }
          }
          numberCoefficients_ = n;
     }
}
// Scale constraint
void
ClpConstraintQuadratic::reallyScale(const double * )
{
     abort();
}
/* Given a zeroed array sets nonquadratic columns to 1.
   Returns number of nonlinear columns
*/
int
ClpConstraintQuadratic::markNonlinear(char * which) const
{
     int iColumn;
     for (iColumn = 0; iColumn < numberQuadraticColumns_; iColumn++) {
          CoinBigIndex j;
          for (j = start_[iColumn]; j < start_[iColumn+1]; j++) {
               int jColumn = column_[j];
               if (jColumn >= 0) {
                    assert (jColumn < numberQuadraticColumns_);
                    which[jColumn] = 1;
                    which[iColumn] = 1;
               }
          }
     }
     int numberCoefficients = 0;
     for (iColumn = 0; iColumn < numberQuadraticColumns_; iColumn++) {
          if (which[iColumn])
               numberCoefficients++;
     }
     return numberCoefficients;
}
/* Given a zeroed array sets possible nonzero coefficients to 1.
   Returns number of nonzeros
*/
int
ClpConstraintQuadratic::markNonzero(char * which) const
{
     int iColumn;
     for (iColumn = 0; iColumn < numberQuadraticColumns_; iColumn++) {
          CoinBigIndex j;
          for (j = start_[iColumn]; j < start_[iColumn+1]; j++) {
               int jColumn = column_[j];
               if (jColumn >= 0) {
                    assert (jColumn < numberQuadraticColumns_);
                    which[jColumn] = 1;
               }
               which[iColumn] = 1;
          }
     }
     int numberCoefficients = 0;
     for (iColumn = 0; iColumn < numberQuadraticColumns_; iColumn++) {
          if (which[iColumn])
               numberCoefficients++;
     }
     return numberCoefficients;
}
// Number of coefficients
int
ClpConstraintQuadratic::numberCoefficients() const
{
     return numberCoefficients_;
}
