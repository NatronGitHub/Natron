/* $Id$ */
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include "CoinPragma.hpp"

#include "ClpSimplex.hpp"
#include "ClpPrimalColumnPivot.hpp"

//#############################################################################
// Constructors / Destructor / Assignment
//#############################################################################

//-------------------------------------------------------------------
// Default Constructor
//-------------------------------------------------------------------
ClpPrimalColumnPivot::ClpPrimalColumnPivot () :
     model_(NULL),
     type_(-1),
     looksOptimal_(false)
{

}

//-------------------------------------------------------------------
// Copy constructor
//-------------------------------------------------------------------
ClpPrimalColumnPivot::ClpPrimalColumnPivot (const ClpPrimalColumnPivot & source) :
     model_(source.model_),
     type_(source.type_),
     looksOptimal_(source.looksOptimal_)
{

}

//-------------------------------------------------------------------
// Destructor
//-------------------------------------------------------------------
ClpPrimalColumnPivot::~ClpPrimalColumnPivot ()
{

}

//----------------------------------------------------------------
// Assignment operator
//-------------------------------------------------------------------
ClpPrimalColumnPivot &
ClpPrimalColumnPivot::operator=(const ClpPrimalColumnPivot& rhs)
{
     if (this != &rhs) {
          type_ = rhs.type_;
          model_ = rhs.model_;
          looksOptimal_ = rhs.looksOptimal_;
     }
     return *this;
}
void
ClpPrimalColumnPivot::saveWeights(ClpSimplex * model, int )
{
     model_ = model;
}
// checks accuracy and may re-initialize (may be empty)

void
ClpPrimalColumnPivot::updateWeights(CoinIndexedVector *)
{
}

// Gets rid of all arrays
void
ClpPrimalColumnPivot::clearArrays()
{
}
/* Returns number of extra columns for sprint algorithm - 0 means off.
   Also number of iterations before recompute
*/
int
ClpPrimalColumnPivot::numberSprintColumns(int & ) const
{
     return 0;
}
// Switch off sprint idea
void
ClpPrimalColumnPivot::switchOffSprint()
{
}
