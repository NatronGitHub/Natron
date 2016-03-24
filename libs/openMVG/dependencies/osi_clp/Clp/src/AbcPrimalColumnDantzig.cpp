/* $Id$ */
// Copyright (C) 2002, International Business Machines
// Corporation and others, Copyright (C) 2012, FasterCoin.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include "CoinPragma.hpp"

#include <cstdio>

#include "CoinIndexedVector.hpp"

#include "AbcSimplex.hpp"
#include "AbcPrimalColumnDantzig.hpp"
#include "AbcSimplexFactorization.hpp"
#include "AbcMatrix.hpp"

//#############################################################################
// Constructors / Destructor / Assignment
//#############################################################################

//-------------------------------------------------------------------
// Default Constructor
//-------------------------------------------------------------------
AbcPrimalColumnDantzig::AbcPrimalColumnDantzig ()
     : AbcPrimalColumnPivot()
{
     type_ = 1;
}

//-------------------------------------------------------------------
// Copy constructor
//-------------------------------------------------------------------
AbcPrimalColumnDantzig::AbcPrimalColumnDantzig (const AbcPrimalColumnDantzig & source)
     : AbcPrimalColumnPivot(source)
{

}

//-------------------------------------------------------------------
// Destructor
//-------------------------------------------------------------------
AbcPrimalColumnDantzig::~AbcPrimalColumnDantzig ()
{

}

//----------------------------------------------------------------
// Assignment operator
//-------------------------------------------------------------------
AbcPrimalColumnDantzig &
AbcPrimalColumnDantzig::operator=(const AbcPrimalColumnDantzig& rhs)
{
     if (this != &rhs) {
          AbcPrimalColumnPivot::operator=(rhs);
     }
     return *this;
}
// Returns pivot column, -1 if none
int
AbcPrimalColumnDantzig::pivotColumn(CoinPartitionedVector * updates,
                                    CoinPartitionedVector * /*spareRow2*/,
                                    CoinPartitionedVector * spareColumn1)
{
     assert(model_);
     int iSection, j;
     int number;
     double multiplier;
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
          model_->factorization()->updateColumnTranspose(*updates);
	  int iVector=model_->getAvailableArray();
	  int bestSequence= model_->abcMatrix()->pivotColumnDantzig(*updates,*model_->usefulArray(iVector));
	  model_->setAvailableArray(iVector);
	  int pivotRow = model_->pivotRow();
	  if (pivotRow >= 0) {
	    // make sure infeasibility on incoming is 0.0
	    int sequenceIn = model_->sequenceIn();
	    double * reducedCost = model_->djRegion();
	    reducedCost[sequenceIn]=0.0;
	  }
#if 1
	  if (model_->logLevel()==127) {
	    double * reducedCost = model_->djRegion();
	    int numberRows=model_->numberRows();
	    printf("Best sequence %d\n",bestSequence);
	    for (int i=0;i<numberRows;i++)
	      printf("row %d dj %g\n",i,reducedCost[i]);
	    for (int i=0;i<model_->numberColumns();i++)
	      printf("column %d dj %g\n",i,reducedCost[i+numberRows]);
	  }
#endif
	  looksOptimal_=bestSequence<0;
	  return bestSequence;
          // put row of tableau in rowArray and columnArray
          model_->abcMatrix()->transposeTimes(*updates, *spareColumn1);
          for (iSection = 0; iSection < 2; iSection++) {

               reducedCost = model_->djRegion(iSection);

               if (!iSection) {
                    number = updates->getNumElements();
                    index = updates->getIndices();
                    updateBy = updates->denseVector();
		    multiplier=-1.0;
               } else {
                    number = spareColumn1->getNumElements();
                    index = spareColumn1->getIndices();
                    updateBy = spareColumn1->denseVector();
		    multiplier=1.0;
               }

               for (j = 0; j < number; j++) {
                    int iSequence = index[j];
                    double value = reducedCost[iSequence];
                    value -= multiplier*updateBy[iSequence];
                    updateBy[iSequence] = 0.0;
                    reducedCost[iSequence] = value;
               }

          }
          updates->setNumElements(0);
          spareColumn1->setNumElements(0);
     }


     // update of duals finished - now do pricing
#if 0
     double largest = model_->currentPrimalTolerance();
     // we can't really trust infeasibilities if there is primal error
     if (model_->largestDualError() > 1.0e-8)
          largest *= model_->largestDualError() / 1.0e-8;
#endif


     double bestDj = model_->dualTolerance();
     int bestSequence = -1;

     double bestFreeDj = model_->dualTolerance();
     int bestFreeSequence = -1;

     number = model_->numberTotal();
     int iSequence;
     reducedCost = model_->djRegion();

#ifndef CLP_PRIMAL_SLACK_MULTIPLIER 
     for (iSequence = 0; iSequence < number; iSequence++) {
          // check flagged variable
          if (!model_->flagged(iSequence)) {
               double value = reducedCost[iSequence];
               AbcSimplex::Status status = model_->getInternalStatus(iSequence);

               switch(status) {

               case AbcSimplex::basic:
               case AbcSimplex::isFixed:
                    break;
               case AbcSimplex::isFree:
               case AbcSimplex::superBasic:
                    if (fabs(value) > bestFreeDj) {
                         bestFreeDj = fabs(value);
                         bestFreeSequence = iSequence;
                    }
                    break;
               case AbcSimplex::atUpperBound:
                    if (value > bestDj) {
                         bestDj = value;
                         bestSequence = iSequence;
                    }
                    break;
               case AbcSimplex::atLowerBound:
                    if (value < -bestDj) {
                         bestDj = -value;
                         bestSequence = iSequence;
                    }
               }
          }
     }
#else
     int numberColumns = model_->numberColumns();
     int maximumRows=model_->maximumAbcNumberRows();
     for (iSequence = maximumRows; iSequence < numberColumns+maximumRows; iSequence++) {
          // check flagged variable
          if (!model_->flagged(iSequence)) {
               double value = reducedCost[iSequence];
               AbcSimplex::Status status = model_->getInternalStatus(iSequence);

               switch(status) {

               case AbcSimplex::basic:
               case AbcSimplex::isFixed:
                    break;
               case AbcSimplex::isFree:
               case AbcSimplex::superBasic:
                    if (fabs(value) > bestFreeDj) {
                         bestFreeDj = fabs(value);
                         bestFreeSequence = iSequence;
                    }
                    break;
               case AbcSimplex::atUpperBound:
                    if (value > bestDj) {
                         bestDj = value;
                         bestSequence = iSequence;
                    }
                    break;
               case AbcSimplex::atLowerBound:
                    if (value < -bestDj) {
                         bestDj = -value;
                         bestSequence = iSequence;
                    }
               }
          }
     }
     // Rows
     number=model_->numberRows();
     for (iSequence=0 ; iSequence < number; iSequence++) {
          // check flagged variable
          if (!model_->flagged(iSequence)) {
               double value = reducedCost[iSequence] * CLP_PRIMAL_SLACK_MULTIPLIER;
               AbcSimplex::Status status = model_->getInternalStatus(iSequence);

               switch(status) {

               case AbcSimplex::basic:
               case AbcSimplex::isFixed:
                    break;
               case AbcSimplex::isFree:
               case AbcSimplex::superBasic:
                    if (fabs(value) > bestFreeDj) {
                         bestFreeDj = fabs(value);
                         bestFreeSequence = iSequence;
                    }
                    break;
               case AbcSimplex::atUpperBound:
                    if (value > bestDj) {
                         bestDj = value;
                         bestSequence = iSequence;
                    }
                    break;
               case AbcSimplex::atLowerBound:
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
#if 1
     if (model_->logLevel()==127) {
       double * reducedCost = model_->djRegion();
       int numberRows=model_->numberRows();
       printf("Best sequence %d\n",bestSequence);
       for (int i=0;i<numberRows;i++)
	 printf("row %d dj %g\n",i,reducedCost[i]);
       for (int i=0;i<model_->numberColumns();i++)
	 printf("column %d dj %g\n",i,reducedCost[i+numberRows]);
     }
#endif
     looksOptimal_=bestSequence<0;
     return bestSequence;
}

//-------------------------------------------------------------------
// Clone
//-------------------------------------------------------------------
AbcPrimalColumnPivot * AbcPrimalColumnDantzig::clone(bool CopyData) const
{
     if (CopyData) {
          return new AbcPrimalColumnDantzig(*this);
     } else {
          return new AbcPrimalColumnDantzig();
     }
}
