/* $Id$ */
// Copyright (C) 2003, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef MyMessageHandler_H
#define MyMessageHandler_H

#include <deque>

#include "CoinPragma.hpp"
#include <stdio.h>
#include "CoinMessageHandler.hpp"

/** This just adds a model to CoinMessage and a void pointer so
    user can trap messages and do useful stuff.
    This is used in Clp/Test/unitTest.cpp

    The file pointer is just there as an example of user stuff.
    In practice you might have

*/
class ClpSimplex;


typedef std::vector<double> StdVectorDouble;

class MyMessageHandler : public CoinMessageHandler {

public:
     /**@name Overrides */
     //@{
     virtual int print();
     //@}
     /**@name set and get */
     //@{
     /// Model
     const ClpSimplex * model() const;
     void setModel(ClpSimplex * model);
     /// Get queue of feasible extreme points
     const std::deque<StdVectorDouble> & getFeasibleExtremePoints() const;
     /// Empty queue of feasible extreme points
     void clearFeasibleExtremePoints();
     //@}

     /**@name Constructors, destructor */
     //@{
     /** Default constructor. */
     MyMessageHandler();
     /// Constructor with pointer to model
     MyMessageHandler(ClpSimplex * model,
                      FILE * userPointer = NULL);
     /** Destructor */
     virtual ~MyMessageHandler();
     //@}

     /**@name Copy method */
     //@{
     /** The copy constructor. */
     MyMessageHandler(const MyMessageHandler&);
     /** The copy constructor from an CoinSimplexMessageHandler. */
     MyMessageHandler(const CoinMessageHandler&);

     MyMessageHandler& operator=(const MyMessageHandler&);
     /// Clone
     virtual CoinMessageHandler * clone() const ;
     //@}


protected:
     /**@name Data members
        The data members are protected to allow access for derived classes. */
     //@{
     /// Pointer back to model
     ClpSimplex * model_;
     /// Saved extreme points
     std::deque<StdVectorDouble> feasibleExtremePoints_;
     /// Iteration number so won't do same one twice
     int iterationNumber_;
     //@}
};

#endif
