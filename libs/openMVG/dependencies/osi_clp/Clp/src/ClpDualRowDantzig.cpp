/* $Id$ */
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include "CoinPragma.hpp"
#include "ClpSimplex.hpp"
#include "ClpDualRowDantzig.hpp"
#include "ClpFactorization.hpp"
#include "CoinIndexedVector.hpp"
#include "CoinHelperFunctions.hpp"
#ifndef CLP_DUAL_COLUMN_MULTIPLIER
#define CLP_DUAL_COLUMN_MULTIPLIER 1.01
#endif

//#############################################################################
// Constructors / Destructor / Assignment
//#############################################################################

//-------------------------------------------------------------------
// Default Constructor
//-------------------------------------------------------------------
ClpDualRowDantzig::ClpDualRowDantzig ()
     : ClpDualRowPivot()
{
     type_ = 1;
}

//-------------------------------------------------------------------
// Copy constructor
//-------------------------------------------------------------------
ClpDualRowDantzig::ClpDualRowDantzig (const ClpDualRowDantzig & source)
     : ClpDualRowPivot(source)
{

}

//-------------------------------------------------------------------
// Destructor
//-------------------------------------------------------------------
ClpDualRowDantzig::~ClpDualRowDantzig ()
{

}

//----------------------------------------------------------------
// Assignment operator
//-------------------------------------------------------------------
ClpDualRowDantzig &
ClpDualRowDantzig::operator=(const ClpDualRowDantzig& rhs)
{
     if (this != &rhs) {
          ClpDualRowPivot::operator=(rhs);
     }
     return *this;
}

// Returns pivot row, -1 if none
int
ClpDualRowDantzig::pivotRow()
{
     assert(model_);
     int iRow;
     const int * pivotVariable = model_->pivotVariable();
     double tolerance = model_->currentPrimalTolerance();
     // we can't really trust infeasibilities if there is primal error
     if (model_->largestPrimalError() > 1.0e-8)
          tolerance *= model_->largestPrimalError() / 1.0e-8;
     double largest = 0.0;
     int chosenRow = -1;
     int numberRows = model_->numberRows();
#ifdef CLP_DUAL_COLUMN_MULTIPLIER
     int numberColumns = model_->numberColumns();
#endif
     for (iRow = 0; iRow < numberRows; iRow++) {
          int iSequence = pivotVariable[iRow];
          double value = model_->solution(iSequence);
          double lower = model_->lower(iSequence);
          double upper = model_->upper(iSequence);
	  double infeas = CoinMax(value - upper , lower - value);
          if (infeas > tolerance) {
#ifdef CLP_DUAL_COLUMN_MULTIPLIER
	      if (iSequence < numberColumns)
		infeas *= CLP_DUAL_COLUMN_MULTIPLIER;
#endif
	      if (infeas > largest) {
		if (!model_->flagged(iSequence)) {
		  chosenRow = iRow;
		  largest = infeas;
		}
	      }
          }
     }
     return chosenRow;
}
// FT update and returns pivot alpha
double
ClpDualRowDantzig::updateWeights(CoinIndexedVector * /*input*/,
                                 CoinIndexedVector * spare,
                                 CoinIndexedVector * /*spare2*/,
                                 CoinIndexedVector * updatedColumn)
{
     // Do FT update
     model_->factorization()->updateColumnFT(spare, updatedColumn);
     // pivot element
     double alpha = 0.0;
     // look at updated column
     double * work = updatedColumn->denseVector();
     int number = updatedColumn->getNumElements();
     int * which = updatedColumn->getIndices();
     int i;
     int pivotRow = model_->pivotRow();

     if (updatedColumn->packedMode()) {
          for (i = 0; i < number; i++) {
               int iRow = which[i];
               if (iRow == pivotRow) {
                    alpha = work[i];
                    break;
               }
          }
     } else {
          alpha = work[pivotRow];
     }
     return alpha;
}

/* Updates primal solution (and maybe list of candidates)
   Uses input vector which it deletes
   Computes change in objective function
*/
void
ClpDualRowDantzig::updatePrimalSolution(CoinIndexedVector * primalUpdate,
                                        double primalRatio,
                                        double & objectiveChange)
{
     double * work = primalUpdate->denseVector();
     int number = primalUpdate->getNumElements();
     int * which = primalUpdate->getIndices();
     int i;
     double changeObj = 0.0;
     const int * pivotVariable = model_->pivotVariable();
     if (primalUpdate->packedMode()) {
          for (i = 0; i < number; i++) {
               int iRow = which[i];
               int iPivot = pivotVariable[iRow];
               double & value = model_->solutionAddress(iPivot);
               double cost = model_->cost(iPivot);
               double change = primalRatio * work[i];
               value -= change;
               changeObj -= change * cost;
               work[i] = 0.0;
          }
     } else {
          for (i = 0; i < number; i++) {
               int iRow = which[i];
               int iPivot = pivotVariable[iRow];
               double & value = model_->solutionAddress(iPivot);
               double cost = model_->cost(iPivot);
               double change = primalRatio * work[iRow];
               value -= change;
               changeObj -= change * cost;
               work[iRow] = 0.0;
          }
     }
     primalUpdate->setNumElements(0);
     objectiveChange += changeObj;
}
//-------------------------------------------------------------------
// Clone
//-------------------------------------------------------------------
ClpDualRowPivot * ClpDualRowDantzig::clone(bool CopyData) const
{
     if (CopyData) {
          return new ClpDualRowDantzig(*this);
     } else {
          return new ClpDualRowDantzig();
     }
}

