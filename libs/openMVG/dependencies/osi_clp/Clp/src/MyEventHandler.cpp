/* $Id$ */
// Copyright (C) 2004, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#if defined(_MSC_VER)
#pragma warning(disable:4786)
#pragma warning(disable:4503)
#endif

#include "MyEventHandler.hpp"


//#############################################################################
// Constructors / Destructor / Assignment
//#############################################################################

//-------------------------------------------------------------------
// Default Constructor
//-------------------------------------------------------------------
MyEventHandler::MyEventHandler ()
     : ClpEventHandler()
{
}

//-------------------------------------------------------------------
// Copy constructor
//-------------------------------------------------------------------
MyEventHandler::MyEventHandler (const MyEventHandler & rhs)
     : ClpEventHandler(rhs)
{
}

// Constructor with pointer to model
MyEventHandler::MyEventHandler(ClpSimplex * model)
     : ClpEventHandler(model)
{
}

//-------------------------------------------------------------------
// Destructor
//-------------------------------------------------------------------
MyEventHandler::~MyEventHandler ()
{
}

//----------------------------------------------------------------
// Assignment operator
//-------------------------------------------------------------------
MyEventHandler &
MyEventHandler::operator=(const MyEventHandler& rhs)
{
     if (this != &rhs) {
          ClpEventHandler::operator=(rhs);
     }
     return *this;
}
//-------------------------------------------------------------------
// Clone
//-------------------------------------------------------------------
ClpEventHandler * MyEventHandler::clone() const
{
     return new MyEventHandler(*this);
}

int
MyEventHandler::event(Event whichEvent)
{
     if (whichEvent == endOfValuesPass)
          return 0; // say optimal
     else
          return -1; // carry on

#if 0
     // This is how one can get some progress information at the end of each iteration.
     if (whichEvent == endOfIteration) {
          int numIter = model_->getIterationCount();
          double sumDualInfeas = model_->sumDualInfeasibilities();
          double sumPrimalInfeas = model_->sumPrimalInfeasibilities();
          double obj = model_->getObjValue();
     }

     // This is how one can tell CLP to stop now.
     // The value of cancelAsap needs to come from the application using CLP.
     if ( cancelAsap ) return 5;
     else return -1;
#endif


}
