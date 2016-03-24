/* $Id$ */
// Copyright (C) 2002, International Business Machines
// Corporation and others, Copyright (C) 2012, FasterCoin.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include "CoinPragma.hpp"
#include "AbcSimplex.hpp"
#include "AbcDualRowPivot.hpp"

//#############################################################################
// Constructors / Destructor / Assignment
//#############################################################################

//-------------------------------------------------------------------
// Default Constructor
//-------------------------------------------------------------------
AbcDualRowPivot::AbcDualRowPivot () :
  model_(NULL),
  type_(-1)
{
  
}

//-------------------------------------------------------------------
// Copy constructor
//-------------------------------------------------------------------
AbcDualRowPivot::AbcDualRowPivot (const AbcDualRowPivot & source) :
  model_(source.model_),
  type_(source.type_)
{
  
}

//-------------------------------------------------------------------
// Destructor
//-------------------------------------------------------------------
AbcDualRowPivot::~AbcDualRowPivot ()
{
  
}

//----------------------------------------------------------------
// Assignment operator
//-------------------------------------------------------------------
AbcDualRowPivot &
AbcDualRowPivot::operator=(const AbcDualRowPivot& rhs)
{
  if (this != &rhs) {
    type_ = rhs.type_;
    model_ = rhs.model_;
  }
  return *this;
}
void
AbcDualRowPivot::saveWeights(AbcSimplex * model, int /*mode*/)
{
  model_ = model;
}
// Recompute infeasibilities
void 
AbcDualRowPivot::recomputeInfeasibilities()
{
}
void 
AbcDualRowPivot::updatePrimalSolutionAndWeights(CoinIndexedVector & weightsVector,
				      CoinIndexedVector & updateColumn,
						double theta)
{
  // finish doing weights
  updateWeights2(weightsVector,updateColumn);
  updatePrimalSolution(updateColumn,theta);
}
// checks accuracy and may re-initialize (may be empty)
void
AbcDualRowPivot::checkAccuracy()
{
}
// Gets rid of all arrays
void
AbcDualRowPivot::clearArrays()
{
}
