/* $Id$ */
// Copyright (C) 2007, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include "CoinPragma.hpp"
#include "CoinHelperFunctions.hpp"
#include "CoinIndexedVector.hpp"
#include "ClpSimplex.hpp"
#include "ClpConstraintLinear.hpp"
#include "CoinSort.hpp"
//#############################################################################
// Constructors / Destructor / Assignment
//#############################################################################

//-------------------------------------------------------------------
// Default Constructor
//-------------------------------------------------------------------
ClpConstraintLinear::ClpConstraintLinear ()
     : ClpConstraint()
{
     type_ = 0;
     column_ = NULL;
     coefficient_ = NULL;
     numberColumns_ = 0;
     numberCoefficients_ = 0;
}

//-------------------------------------------------------------------
// Useful Constructor
//-------------------------------------------------------------------
ClpConstraintLinear::ClpConstraintLinear (int row, int numberCoefficents ,
          int numberColumns,
          const int * column, const double * coefficient)
     : ClpConstraint()
{
     type_ = 0;
     rowNumber_ = row;
     numberColumns_ = numberColumns;
     numberCoefficients_ = numberCoefficents;
     column_ = CoinCopyOfArray(column, numberCoefficients_);
     coefficient_ = CoinCopyOfArray(coefficient, numberCoefficients_);
     CoinSort_2(column_, column_ + numberCoefficients_, coefficient_);
}

//-------------------------------------------------------------------
// Copy constructor
//-------------------------------------------------------------------
ClpConstraintLinear::ClpConstraintLinear (const ClpConstraintLinear & rhs)
     : ClpConstraint(rhs)
{
     numberColumns_ = rhs.numberColumns_;
     numberCoefficients_ = rhs.numberCoefficients_;
     column_ = CoinCopyOfArray(rhs.column_, numberCoefficients_);
     coefficient_ = CoinCopyOfArray(rhs.coefficient_, numberCoefficients_);
}


//-------------------------------------------------------------------
// Destructor
//-------------------------------------------------------------------
ClpConstraintLinear::~ClpConstraintLinear ()
{
     delete [] column_;
     delete [] coefficient_;
}

//----------------------------------------------------------------
// Assignment operator
//-------------------------------------------------------------------
ClpConstraintLinear &
ClpConstraintLinear::operator=(const ClpConstraintLinear& rhs)
{
     if (this != &rhs) {
          delete [] column_;
          delete [] coefficient_;
          numberColumns_ = rhs.numberColumns_;
          numberCoefficients_ = rhs.numberCoefficients_;
          column_ = CoinCopyOfArray(rhs.column_, numberCoefficients_);
          coefficient_ = CoinCopyOfArray(rhs.coefficient_, numberCoefficients_);
     }
     return *this;
}
//-------------------------------------------------------------------
// Clone
//-------------------------------------------------------------------
ClpConstraint * ClpConstraintLinear::clone() const
{
     return new ClpConstraintLinear(*this);
}

// Returns gradient
int
ClpConstraintLinear::gradient(const ClpSimplex * model,
                              const double * solution,
                              double * gradient,
                              double & functionValue,
                              double & offset,
                              bool useScaling,
                              bool refresh) const
{
     if (refresh || !lastGradient_) {
          functionValue_ = 0.0;
          if (!lastGradient_)
               lastGradient_ = new double[numberColumns_];
          CoinZeroN(lastGradient_, numberColumns_);
          bool scaling = (model && model->rowScale() && useScaling);
          if (!scaling) {
               for (int i = 0; i < numberCoefficients_; i++) {
                    int iColumn = column_[i];
                    double value = solution[iColumn];
                    double coefficient = coefficient_[i];
                    functionValue_ += value * coefficient;
                    lastGradient_[iColumn] = coefficient;
               }
          } else {
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
     offset = 0.0;
     CoinMemcpyN(lastGradient_, numberColumns_, gradient);
     return 0;
}
// Resize constraint
void
ClpConstraintLinear::resize(int newNumberColumns)
{
     if (numberColumns_ != newNumberColumns) {
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
ClpConstraintLinear::deleteSome(int numberToDelete, const int * which)
{
     if (numberToDelete) {
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
ClpConstraintLinear::reallyScale(const double * columnScale)
{
     for (int i = 0; i < numberCoefficients_; i++) {
          int iColumn = column_[i];
          coefficient_[i] *= columnScale[iColumn];
     }
}
/* Given a zeroed array sets nonlinear columns to 1.
   Returns number of nonlinear columns
*/
int
ClpConstraintLinear::markNonlinear(char *) const
{
     return 0;
}
/* Given a zeroed array sets possible nonzero coefficients to 1.
   Returns number of nonzeros
*/
int
ClpConstraintLinear::markNonzero(char * which) const
{
     for (int i = 0; i < numberCoefficients_; i++) {
          int iColumn = column_[i];
          which[iColumn] = 1;
     }
     return numberCoefficients_;
}
// Number of coefficients
int
ClpConstraintLinear::numberCoefficients() const
{
     return numberCoefficients_;
}
