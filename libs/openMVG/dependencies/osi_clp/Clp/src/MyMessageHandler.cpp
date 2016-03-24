/* $Id$ */
// Copyright (C) 2003, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#if defined(_MSC_VER)
// move into CoinPragma.hpp ?
#pragma warning(disable:4503)
#endif

#include <cstdio>

#include "CoinPragma.hpp"
#include "ClpSimplex.hpp"
#include "ClpNonLinearCost.hpp"
#include "MyMessageHandler.hpp"
#include "ClpMessage.hpp"


//#############################################################################
// Constructors / Destructor / Assignment
//#############################################################################

//-------------------------------------------------------------------
// Default Constructor
//-------------------------------------------------------------------
MyMessageHandler::MyMessageHandler ()
     : CoinMessageHandler(),
       model_(NULL),
       feasibleExtremePoints_(),
       iterationNumber_(-1)
{
}

//-------------------------------------------------------------------
// Copy constructor
//-------------------------------------------------------------------
MyMessageHandler::MyMessageHandler (const MyMessageHandler & rhs)
     : CoinMessageHandler(rhs),
       model_(rhs.model_),
       feasibleExtremePoints_(rhs.feasibleExtremePoints_),
       iterationNumber_(rhs.iterationNumber_)
{
}

MyMessageHandler::MyMessageHandler (const CoinMessageHandler & rhs)
     : CoinMessageHandler(rhs),
       model_(NULL),
       feasibleExtremePoints_(),
       iterationNumber_(-1)
{
}

// Constructor with pointer to model
MyMessageHandler::MyMessageHandler(ClpSimplex * model,
                                   FILE * /*userPointer*/)
     : CoinMessageHandler(),
       model_(model),
       feasibleExtremePoints_(),
       iterationNumber_(-1)
{
}

//-------------------------------------------------------------------
// Destructor
//-------------------------------------------------------------------
MyMessageHandler::~MyMessageHandler ()
{
}

//----------------------------------------------------------------
// Assignment operator
//-------------------------------------------------------------------
MyMessageHandler &
MyMessageHandler::operator=(const MyMessageHandler& rhs)
{
     if (this != &rhs) {
          CoinMessageHandler::operator=(rhs);
          model_ = rhs.model_;
          feasibleExtremePoints_ = rhs.feasibleExtremePoints_;
          iterationNumber_ = rhs.iterationNumber_;
     }
     return *this;
}
//-------------------------------------------------------------------
// Clone
//-------------------------------------------------------------------
CoinMessageHandler * MyMessageHandler::clone() const
{
     return new MyMessageHandler(*this);
}

int
MyMessageHandler::print()
{
     if (currentSource() == "Clp") {
          if (currentMessage().externalNumber() == 102) {
               printf("There are %d primal infeasibilities\n",
                      model_->nonLinearCost()->numberInfeasibilities());
               // Feasibility
               if (!model_->nonLinearCost()->numberInfeasibilities()) {
                    // Column solution
                    int numberColumns = model_->numberColumns();
                    const double * solution = model_->solutionRegion(1);

                    // Create vector to contain solution
                    StdVectorDouble feasibleExtremePoint;

                    const double *objective = model_->objective();
                    double objectiveValue = 0;

                    if (!model_->columnScale()) {
                         // No scaling
                         for (int i = 0; i < numberColumns; i++) {
                              feasibleExtremePoint.push_back(solution[i]);
                              objectiveValue += solution[i] * objective[i];
                         }
                    } else {
                         // scaled
                         const double * columnScale = model_->columnScale();
                         for (int i = 0; i < numberColumns; i++) {
                              feasibleExtremePoint.push_back(solution[i]*columnScale[i]);
                              objectiveValue += solution[i] * objective[i] * columnScale[i];
                         }
                    }
                    std::cout << "Objective " << objectiveValue << std::endl;
                    // Save solution
                    feasibleExtremePoints_.push_front(feasibleExtremePoint);

                    // Want maximum of 10 solutions, so if more then 10 get rid of oldest
                    size_t numExtremePointsSaved = feasibleExtremePoints_.size();
                    if ( numExtremePointsSaved >= 10 ) {
                         feasibleExtremePoints_.pop_back();
                         assert( feasibleExtremePoints_.size() == numExtremePointsSaved - 1 );
                    };

               }
               return 0; // skip printing
          }
     }

     // If one wants access to the message text,
     // it is available using method messageBuffer().
     // For example, one could code:
     // std::cout <<messageBuffer() <<std::endl;

     return CoinMessageHandler::print();
}
const ClpSimplex *
MyMessageHandler::model() const
{
     return model_;
}
void
MyMessageHandler::setModel(ClpSimplex * model)
{
     model_ = model;
}

const std::deque<StdVectorDouble> & MyMessageHandler::getFeasibleExtremePoints() const
{
     return feasibleExtremePoints_;
}
void MyMessageHandler::clearFeasibleExtremePoints()
{
     feasibleExtremePoints_.clear();
}
