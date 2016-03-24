/* $Id$ */
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include "CoinPragma.hpp"

#include <cstdio>

#include "CoinIndexedVector.hpp"

#include "ClpSimplex.hpp"
#include "ClpPrimalColumnDantzig.hpp"
#include "ClpFactorization.hpp"
#include "ClpPackedMatrix.hpp"

//#############################################################################
// Constructors / Destructor / Assignment
//#############################################################################

//-------------------------------------------------------------------
// Default Constructor
//-------------------------------------------------------------------
ClpPrimalColumnDantzig::ClpPrimalColumnDantzig ()
     : ClpPrimalColumnPivot()
{
     type_ = 1;
}

//-------------------------------------------------------------------
// Copy constructor
//-------------------------------------------------------------------
ClpPrimalColumnDantzig::ClpPrimalColumnDantzig (const ClpPrimalColumnDantzig & source)
     : ClpPrimalColumnPivot(source)
{

}

//-------------------------------------------------------------------
// Destructor
//-------------------------------------------------------------------
ClpPrimalColumnDantzig::~ClpPrimalColumnDantzig ()
{

}

//----------------------------------------------------------------
// Assignment operator
//-------------------------------------------------------------------
ClpPrimalColumnDantzig &
ClpPrimalColumnDantzig::operator=(const ClpPrimalColumnDantzig& rhs)
{
     if (this != &rhs) {
          ClpPrimalColumnPivot::operator=(rhs);
     }
     return *this;
}

// Returns pivot column, -1 if none
int
ClpPrimalColumnDantzig::pivotColumn(CoinIndexedVector * updates,
                                    CoinIndexedVector * /*spareRow1*/,
                                    CoinIndexedVector * spareRow2,
                                    CoinIndexedVector * spareColumn1,
                                    CoinIndexedVector * spareColumn2)
{
     assert(model_);
     int iSection, j;
     int number;
     int * index;
     double * updateBy;
     double * reducedCost;

     bool anyUpdates;

     if (updates->getNumElements()) {
          anyUpdates = true;
     } else {
          // sub flip - nothing to do
          anyUpdates = false;
     }
     if (anyUpdates) {
          model_->factorization()->updateColumnTranspose(spareRow2, updates);
          // put row of tableau in rowArray and columnArray
          model_->clpMatrix()->transposeTimes(model_, -1.0,
                                              updates, spareColumn2, spareColumn1);
          for (iSection = 0; iSection < 2; iSection++) {

               reducedCost = model_->djRegion(iSection);

               if (!iSection) {
                    number = updates->getNumElements();
                    index = updates->getIndices();
                    updateBy = updates->denseVector();
               } else {
                    number = spareColumn1->getNumElements();
                    index = spareColumn1->getIndices();
                    updateBy = spareColumn1->denseVector();
               }

               for (j = 0; j < number; j++) {
                    int iSequence = index[j];
                    double value = reducedCost[iSequence];
                    value -= updateBy[j];
                    updateBy[j] = 0.0;
                    reducedCost[iSequence] = value;
               }

          }
          updates->setNumElements(0);
          spareColumn1->setNumElements(0);
     }


     // update of duals finished - now do pricing

     double largest = model_->currentPrimalTolerance();
     // we can't really trust infeasibilities if there is primal error
     if (model_->largestDualError() > 1.0e-8)
          largest *= model_->largestDualError() / 1.0e-8;



     double bestDj = model_->dualTolerance();
     int bestSequence = -1;

     double bestFreeDj = model_->dualTolerance();
     int bestFreeSequence = -1;

     number = model_->numberRows() + model_->numberColumns();
     int iSequence;
     reducedCost = model_->djRegion();

#ifndef CLP_PRIMAL_SLACK_MULTIPLIER 
     for (iSequence = 0; iSequence < number; iSequence++) {
          // check flagged variable
          if (!model_->flagged(iSequence)) {
               double value = reducedCost[iSequence];
               ClpSimplex::Status status = model_->getStatus(iSequence);

               switch(status) {

               case ClpSimplex::basic:
               case ClpSimplex::isFixed:
                    break;
               case ClpSimplex::isFree:
               case ClpSimplex::superBasic:
                    if (fabs(value) > bestFreeDj) {
                         bestFreeDj = fabs(value);
                         bestFreeSequence = iSequence;
                    }
                    break;
               case ClpSimplex::atUpperBound:
                    if (value > bestDj) {
                         bestDj = value;
                         bestSequence = iSequence;
                    }
                    break;
               case ClpSimplex::atLowerBound:
                    if (value < -bestDj) {
                         bestDj = -value;
                         bestSequence = iSequence;
                    }
               }
          }
     }
#else
     // Columns
     int numberColumns = model_->numberColumns();
     for (iSequence = 0; iSequence < numberColumns; iSequence++) {
          // check flagged variable
          if (!model_->flagged(iSequence)) {
               double value = reducedCost[iSequence];
               ClpSimplex::Status status = model_->getStatus(iSequence);

               switch(status) {

               case ClpSimplex::basic:
               case ClpSimplex::isFixed:
                    break;
               case ClpSimplex::isFree:
               case ClpSimplex::superBasic:
                    if (fabs(value) > bestFreeDj) {
                         bestFreeDj = fabs(value);
                         bestFreeSequence = iSequence;
                    }
                    break;
               case ClpSimplex::atUpperBound:
                    if (value > bestDj) {
                         bestDj = value;
                         bestSequence = iSequence;
                    }
                    break;
               case ClpSimplex::atLowerBound:
                    if (value < -bestDj) {
                         bestDj = -value;
                         bestSequence = iSequence;
                    }
               }
          }
     }
     // Rows
     for ( ; iSequence < number; iSequence++) {
          // check flagged variable
          if (!model_->flagged(iSequence)) {
               double value = reducedCost[iSequence] * CLP_PRIMAL_SLACK_MULTIPLIER;
               ClpSimplex::Status status = model_->getStatus(iSequence);

               switch(status) {

               case ClpSimplex::basic:
               case ClpSimplex::isFixed:
                    break;
               case ClpSimplex::isFree:
               case ClpSimplex::superBasic:
                    if (fabs(value) > bestFreeDj) {
                         bestFreeDj = fabs(value);
                         bestFreeSequence = iSequence;
                    }
                    break;
               case ClpSimplex::atUpperBound:
                    if (value > bestDj) {
                         bestDj = value;
                         bestSequence = iSequence;
                    }
                    break;
               case ClpSimplex::atLowerBound:
                    if (value < -bestDj) {
                         bestDj = -value;
                         bestSequence = iSequence;
                    }
               }
          }
     }
#endif
     // bias towards free
     if (bestFreeSequence >= 0 && bestFreeDj > 0.1 * bestDj)
          bestSequence = bestFreeSequence;
     return bestSequence;
}

//-------------------------------------------------------------------
// Clone
//-------------------------------------------------------------------
ClpPrimalColumnPivot * ClpPrimalColumnDantzig::clone(bool CopyData) const
{
     if (CopyData) {
          return new ClpPrimalColumnDantzig(*this);
     } else {
          return new ClpPrimalColumnDantzig();
     }
}

