/* $Id$ */
// Copyright (C) 2002, International Business Machines
// Corporation and others, Copyright (C) 2012, FasterCoin.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include "CoinPragma.hpp"

#include "AbcSimplex.hpp"
#include "AbcPrimalColumnPivot.hpp"

//#############################################################################
// Constructors / Destructor / Assignment
//#############################################################################

//-------------------------------------------------------------------
// Default Constructor
//-------------------------------------------------------------------
AbcPrimalColumnPivot::AbcPrimalColumnPivot () :
     model_(NULL),
     type_(-1),
     looksOptimal_(false)
{

}

//-------------------------------------------------------------------
// Copy constructor
//-------------------------------------------------------------------
AbcPrimalColumnPivot::AbcPrimalColumnPivot (const AbcPrimalColumnPivot & source) :
     model_(source.model_),
     type_(source.type_),
     looksOptimal_(source.looksOptimal_)
{

}

//-------------------------------------------------------------------
// Destructor
//-------------------------------------------------------------------
AbcPrimalColumnPivot::~AbcPrimalColumnPivot ()
{

}

//----------------------------------------------------------------
// Assignment operator
//-------------------------------------------------------------------
AbcPrimalColumnPivot &
AbcPrimalColumnPivot::operator=(const AbcPrimalColumnPivot& rhs)
{
     if (this != &rhs) {
          type_ = rhs.type_;
          model_ = rhs.model_;
          looksOptimal_ = rhs.looksOptimal_;
     }
     return *this;
}
void
AbcPrimalColumnPivot::saveWeights(AbcSimplex * model, int )
{
     model_ = model;
}
// checks accuracy and may re-initialize (may be empty)

void
AbcPrimalColumnPivot::updateWeights(CoinIndexedVector *)
{
}

// Gets rid of all arrays
void
AbcPrimalColumnPivot::clearArrays()
{
}
/* Returns number of extra columns for sprint algorithm - 0 means off.
   Also number of iterations before recompute
*/
int
AbcPrimalColumnPivot::numberSprintColumns(int & ) const
{
     return 0;
}
// Switch off sprint idea
void
AbcPrimalColumnPivot::switchOffSprint()
{
}
