/* $Id$ */
// Copyright (C) 2004, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef ClpEventHandler_H
#define ClpEventHandler_H

#include "ClpSimplex.hpp"
/** Base class for Clp event handling

This is just here to allow for event handling.  By event I mean a Clp event
e.g. end of values pass.

One use would be to let a user handle a system event e.g. Control-C.  This could be done
by deriving a class MyEventHandler which knows about such events.  If one occurs
MyEventHandler::event() could clear event status and return 3 (stopped).

Clp would then return to user code.

As it is called every iteration this should be fine grained enough.

User can derive and construct from CbcModel  - not pretty

*/

class ClpEventHandler  {

public:
     /** enums for what sort of event.

         These will also be returned in ClpModel::secondaryStatus() as int
     */
     enum Event {
          endOfIteration = 100, // used to set secondary status
          endOfFactorization, // after gutsOfSolution etc
          endOfValuesPass,
          node, // for Cbc
          treeStatus, // for Cbc
          solution, // for Cbc
          theta, // hit in parametrics
          pivotRow, // used to choose pivot row
	  presolveStart, // ClpSolve presolve start
	  presolveSize, // sees if ClpSolve presolve too big or too small
	  presolveInfeasible, // ClpSolve presolve infeasible
	  presolveBeforeSolve, // ClpSolve presolve before solve
	  presolveAfterFirstSolve, // ClpSolve presolve after solve
	  presolveAfterSolve, // ClpSolve presolve after solve
	  presolveEnd, // ClpSolve presolve end
	  goodFactorization, // before gutsOfSolution
	  complicatedPivotIn, // in modifyCoefficients
	  noCandidateInPrimal, // tentative end
	  looksEndInPrimal, // About to declare victory (or defeat)
	  endInPrimal, // Victory (or defeat)
	  beforeStatusOfProblemInPrimal,
	  startOfStatusOfProblemInPrimal,
	  complicatedPivotOut, // in modifyCoefficients
	  noCandidateInDual, // tentative end
	  looksEndInDual, // About to declare victory (or defeat)
	  endInDual, // Victory (or defeat)
	  beforeStatusOfProblemInDual,
	  startOfStatusOfProblemInDual,
	  startOfIterationInDual,
	  updateDualsInDual,
	  endOfCreateRim,
	  slightlyInfeasible,
	  modifyMatrixInMiniPresolve,
	  moreMiniPresolve,
	  modifyMatrixInMiniPostsolve,
	  noTheta // At end (because no pivot)
     };
     /**@name Virtual method that the derived classes should provide.
      The base class instance does nothing and as event() is only useful method
      it would not be very useful NOT providing one!
     */
     //@{
     /** This can do whatever it likes.  If return code -1 then carries on
         if 0 sets ClpModel::status() to 5 (stopped by event) and will return to user.
         At present if <-1 carries on and if >0 acts as if 0 - this may change.
	 For ClpSolve 2 -> too big return status of -2 and -> too small 3
     */
     virtual int event(Event whichEvent);
     /** This can do whatever it likes.  Return code -1 means no action.
	 This passes in something
     */
     virtual int eventWithInfo(Event whichEvent, void * info) ;
     //@}


     /**@name Constructors, destructor */

     //@{
     /** Default constructor. */
     ClpEventHandler(ClpSimplex * model = NULL);
     /** Destructor */
     virtual ~ClpEventHandler();
     // Copy
     ClpEventHandler(const ClpEventHandler&);
     // Assignment
     ClpEventHandler& operator=(const ClpEventHandler&);
     /// Clone
     virtual ClpEventHandler * clone() const;

     //@}

     /**@name Sets/gets */

     //@{
     /** set model. */
     void setSimplex(ClpSimplex * model);
     /// Get model
     inline ClpSimplex * simplex() const {
          return model_;
     }
     //@}


protected:
     /**@name Data members
        The data members are protected to allow access for derived classes. */
     //@{
     /// Pointer to simplex
     ClpSimplex * model_;
     //@}
};
/** Base class for Clp disaster handling

This is here to allow for disaster handling.  By disaster I mean that Clp
would otherwise give up

*/

class ClpDisasterHandler  {

public:
     /**@name Virtual methods that the derived classe should provide.
     */
     //@{
     /// Into simplex
     virtual void intoSimplex() = 0;
     /// Checks if disaster
     virtual bool check() const = 0;
     /// saves information for next attempt
     virtual void saveInfo() = 0;
     /// Type of disaster 0 can fix, 1 abort
     virtual int typeOfDisaster();
     //@}


     /**@name Constructors, destructor */

     //@{
     /** Default constructor. */
     ClpDisasterHandler(ClpSimplex * model = NULL);
     /** Destructor */
     virtual ~ClpDisasterHandler();
     // Copy
     ClpDisasterHandler(const ClpDisasterHandler&);
     // Assignment
     ClpDisasterHandler& operator=(const ClpDisasterHandler&);
     /// Clone
     virtual ClpDisasterHandler * clone() const = 0;

     //@}

     /**@name Sets/gets */

     //@{
     /** set model. */
     void setSimplex(ClpSimplex * model);
     /// Get model
     inline ClpSimplex * simplex() const {
          return model_;
     }
     //@}


protected:
     /**@name Data members
        The data members are protected to allow access for derived classes. */
     //@{
     /// Pointer to simplex
     ClpSimplex * model_;
     //@}
};
#endif
