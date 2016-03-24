/* $Id$ */
// Copyright (C) 2004, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include "CoinPragma.hpp"

#include "ClpEventHandler.hpp"
#include "ClpSimplex.hpp"

//#############################################################################
// Constructors / Destructor / Assignment
//#############################################################################

//-------------------------------------------------------------------
// Default Constructor
//-------------------------------------------------------------------
ClpEventHandler::ClpEventHandler (ClpSimplex * model) :
     model_(model)
{

}

//-------------------------------------------------------------------
// Copy constructor
//-------------------------------------------------------------------
ClpEventHandler::ClpEventHandler (const ClpEventHandler & rhs)
     : model_(rhs.model_)
{
}

//-------------------------------------------------------------------
// Destructor
//-------------------------------------------------------------------
ClpEventHandler::~ClpEventHandler ()
{
}

//----------------------------------------------------------------
// Assignment operator
//-------------------------------------------------------------------
ClpEventHandler &
ClpEventHandler::operator=(const ClpEventHandler& rhs)
{
     if (this != &rhs) {
          model_ = rhs.model_;
     }
     return *this;
}
// Clone
ClpEventHandler *
ClpEventHandler::clone() const
{
     return new ClpEventHandler(*this);
}
// Event
int
ClpEventHandler::event(Event whichEvent)
{
     if (whichEvent != theta)
          return -1; // do nothing
     else
          return 0; // say normal exit
}
/* This can do whatever it likes.  Return code -1 means no action.
   This passes in something
*/
int 
ClpEventHandler::eventWithInfo(Event whichEvent, void * info) 
{ 
  return -1;
}
/* set model. */
void
ClpEventHandler::setSimplex(ClpSimplex * model)
{
     model_ = model;
}
//#############################################################################
// Constructors / Destructor / Assignment
//#############################################################################

//-------------------------------------------------------------------
// Default Constructor
//-------------------------------------------------------------------
ClpDisasterHandler::ClpDisasterHandler (ClpSimplex * model) :
     model_(model)
{

}

//-------------------------------------------------------------------
// Copy constructor
//-------------------------------------------------------------------
ClpDisasterHandler::ClpDisasterHandler (const ClpDisasterHandler & rhs)
     : model_(rhs.model_)
{
}

//-------------------------------------------------------------------
// Destructor
//-------------------------------------------------------------------
ClpDisasterHandler::~ClpDisasterHandler ()
{
}

//----------------------------------------------------------------
// Assignment operator
//-------------------------------------------------------------------
ClpDisasterHandler &
ClpDisasterHandler::operator=(const ClpDisasterHandler& rhs)
{
     if (this != &rhs) {
          model_ = rhs.model_;
     }
     return *this;
}
/* set model. */
void
ClpDisasterHandler::setSimplex(ClpSimplex * model)
{
     model_ = model;
}
// Type of disaster 0 can fix, 1 abort
int
ClpDisasterHandler::typeOfDisaster()
{
     return 0;
}


