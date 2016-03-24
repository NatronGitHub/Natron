/* $Id$ */
// Copyright (C) 2002, International Business Machines
// Corporation and others, Copyright (C) 2012, FasterCoin.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).
/*
  Authors
  
  John Forrest
  
*/
#ifndef AbcSimplexDual_H
#define AbcSimplexDual_H

#include "AbcSimplex.hpp"
#if 0
#undef ABC_PARALLEL
#define ABC_PARALLEL 2
#undef cilk_for 
#undef cilk_spawn
#undef cilk_sync
#include <cilk/cilk.h>
#endif
typedef struct {
  double theta;
  double totalThru;
  double useThru;
  double bestEverPivot;
  double increaseInObjective;
  double tentativeTheta;
  double lastPivotValue;
  double thisPivotValue;
  double thruThis;
  double increaseInThis;
  int lastSequence;
  int sequence;
  int block;
  int numberSwapped;
  int numberRemaining;
  int numberLastSwapped;
  bool modifyCosts;
} dualColumnResult;
/** This solves LPs using the dual simplex method
    
    It inherits from AbcSimplex.  It has no data of its own and
    is never created - only cast from a AbcSimplex object at algorithm time.
    
*/

class AbcSimplexDual : public AbcSimplex {
  
public:
  
  /**@name Description of algorithm */
  //@{
  /** Dual algorithm
      
      Method
      
      It tries to be a single phase approach with a weight of 1.0 being
      given to getting optimal and a weight of updatedDualBound_ being
      given to getting dual feasible.  In this version I have used the
      idea that this weight can be thought of as a fake bound.  If the
      distance between the lower and upper bounds on a variable is less
      than the feasibility weight then we are always better off flipping
      to other bound to make dual feasible.  If the distance is greater
      then we make up a fake bound updatedDualBound_ away from one bound.
      If we end up optimal or primal infeasible, we check to see if
      bounds okay.  If so we have finished, if not we increase updatedDualBound_
      and continue (after checking if unbounded). I am undecided about
      free variables - there is coding but I am not sure about it.  At
      present I put them in basis anyway.
      
      The code is designed to take advantage of sparsity so arrays are
      seldom zeroed out from scratch or gone over in their entirety.
      The only exception is a full scan to find outgoing variable for
      Dantzig row choice.  For steepest edge we keep an updated list
      of infeasibilities (actually squares).
      On easy problems we don't need full scan - just
      pick first reasonable.
      
      One problem is how to tackle degeneracy and accuracy.  At present
      I am using the modification of costs which I put in OSL and some
      of what I think is the dual analog of Gill et al.
      I am still not sure of the exact details.
      
      The flow of dual is three while loops as follows:
      
      while (not finished) {
      
      while (not clean solution) {
      
      Factorize and/or clean up solution by flipping variables so
      dual feasible.  If looks finished check fake dual bounds.
      Repeat until status is iterating (-1) or finished (0,1,2)
      
      }
      
      while (status==-1) {
      
      Iterate until no pivot in or out or time to re-factorize.
      
      Flow is:
      
      choose pivot row (outgoing variable).  if none then
      we are primal feasible so looks as if done but we need to
      break and check bounds etc.
      
      Get pivot row in tableau
      
      Choose incoming column.  If we don't find one then we look
      primal infeasible so break and check bounds etc.  (Also the
      pivot tolerance is larger after any iterations so that may be
      reason)
      
      If we do find incoming column, we may have to adjust costs to
      keep going forwards (anti-degeneracy).  Check pivot will be stable
      and if unstable throw away iteration and break to re-factorize.
      If minor error re-factorize after iteration.
      
      Update everything (this may involve flipping variables to stay
      dual feasible.
      
      }
      
      }
      
      TODO's (or maybe not)
      
      At present we never check we are going forwards.  I overdid that in
      OSL so will try and make a last resort.
      
      Needs partial scan pivot out option.
      
      May need other anti-degeneracy measures, especially if we try and use
      loose tolerances as a way to solve in fewer iterations.
      
      I like idea of dynamic scaling.  This gives opportunity to decouple
      different implications of scaling for accuracy, iteration count and
      feasibility tolerance.
      
      for use of exotic parameter startFinishoptions see Abcsimplex.hpp
  */
  
  int dual();
  /** For strong branching.  On input lower and upper are new bounds
      while on output they are change in objective function values
      (>1.0e50 infeasible).
      Return code is 0 if nothing interesting, -1 if infeasible both
      ways and +1 if infeasible one way (check values to see which one(s))
      Solutions are filled in as well - even down, odd up - also
      status and number of iterations
  */
  int strongBranching(int numberVariables, const int * variables,
		      double * newLower, double * newUpper,
		      double ** outputSolution,
		      int * outputStatus, int * outputIterations,
		      bool stopOnFirstInfeasible = true,
		      bool alwaysFinish = false,
		      int startFinishOptions = 0);
  /// This does first part of StrongBranching
  AbcSimplexFactorization * setupForStrongBranching(char * arrays, int numberRows,
						    int numberColumns, bool solveLp = false);
  /// This cleans up after strong branching
  void cleanupAfterStrongBranching(AbcSimplexFactorization * factorization);
  //@}
  
  /**@name Functions used in dual */
  //@{
  /** This has the flow between re-factorizations
      Broken out for clarity and will be used by strong branching
      
      Reasons to come out:
      -1 iterations etc
      -2 inaccuracy
      -3 slight inaccuracy (and done iterations)
      +0 looks optimal (might be unbounded - but we will investigate)
      +1 looks infeasible
      +3 max iterations
      
      If givenPi not NULL then in values pass (copy from ClpSimplexDual)
  */
  int whileIteratingSerial();
#if ABC_PARALLEL==1
  int whileIteratingThread();
#endif
#if ABC_PARALLEL==2
  int whileIteratingCilk();
#endif
  void whileIterating2();
  int whileIteratingParallel(int numberIterations);
  int whileIterating3();
  void updatePrimalSolution();
  int noPivotRow();
  int noPivotColumn();
  void dualPivotColumn();
  /// Create dual pricing vector
  void createDualPricingVectorSerial();
  int getTableauColumnFlipAndStartReplaceSerial();
  void getTableauColumnPart1Serial();
#if ABC_PARALLEL==1
  /// Create dual pricing vector
  void createDualPricingVectorThread();
  int getTableauColumnFlipAndStartReplaceThread();
  void getTableauColumnPart1Thread();
#endif
#if ABC_PARALLEL==2
  /// Create dual pricing vector
  void createDualPricingVectorCilk();
  int getTableauColumnFlipAndStartReplaceCilk();
  void getTableauColumnPart1Cilk();
#endif
  void getTableauColumnPart2();
  int checkReplace();
  void replaceColumnPart3();
  void checkReplacePart1();
  void checkReplacePart1a();
  void checkReplacePart1b();
  /// The duals are updated
  void updateDualsInDual();
  /** The duals are updated by the given arrays.
      This is in values pass - so no changes to primal is made
  */
  //void updateDualsInValuesPass(CoinIndexedVector * array,
  //                           double theta);
  /** While dualColumn gets flips 
      this does actual flipping.
      returns number flipped
  */
  int flipBounds();
  /** Undo a flip
  */
  void flipBack(int number);
  /**
     Array has tableau row (row section)
     Puts candidates for rows in list
     Returns guess at upper theta (infinite if no pivot) and may set sequenceIn_ if free
     Can do all (if tableauRow created)
  */
  void dualColumn1(bool doAll=false);
  /**
     Array has tableau row (row section)
     Just does slack part
     Returns guess at upper theta (infinite if no pivot) and may set sequenceIn_ if free
  */
  double dualColumn1A();
  /// Do all given tableau row
  double dualColumn1B();
  /**
     Chooses incoming
     Puts flipped ones in list
     If necessary will modify costs
  */
  void dualColumn2();
  void dualColumn2Most(dualColumnResult & result);
  void dualColumn2First(dualColumnResult & result);
  /**
     Chooses part of incoming
     Puts flipped ones in list
     If necessary will modify costs
  */
  void dualColumn2(dualColumnResult & result);
  /**
     This sees what is best thing to do in branch and bound cleanup
     If sequenceIn_ < 0 then can't do anything
  */
  void checkPossibleCleanup(CoinIndexedVector * array);
  /**
     Chooses dual pivot row
     Would be faster with separate region to scan
     and will have this (with square of infeasibility) when steepest
     For easy problems we can just choose one of the first rows we look at
  */
  void dualPivotRow();
  /** Checks if any fake bounds active - if so returns number and modifies
      updatedDualBound_ and everything.
      Free variables will be left as free
      Returns number of bounds changed if >=0
      Returns -1 if not initialize and no effect
      fills cost of change vector
  */
  int changeBounds(int initialize, double & changeCost);
  /** As changeBounds but just changes new bounds for a single variable.
      Returns true if change */
  bool changeBound( int iSequence);
  /// Restores bound to original bound
  void originalBound(int iSequence);
  /** Checks if tentative optimal actually means unbounded in dual
      Returns -3 if not, 2 if is unbounded */
  int checkUnbounded(CoinIndexedVector & ray,  double changeCost);
  /**  Refactorizes if necessary
       Checks if finished.  Updates status.
       lastCleaned refers to iteration at which some objective/feasibility
       cleaning too place.
       
       type - 0 initial so set up save arrays etc
       - 1 normal -if good update save
       - 2 restoring from saved
  */
  void statusOfProblemInDual(int type);
  /** Fast iterations.  Misses out a lot of initialization.
      Normally stops on maximum iterations, first re-factorization
      or tentative optimum.  If looks interesting then continues as
      normal.  Returns 0 if finished properly, 1 otherwise.
  */
  /// Gets tableau column - does flips and checks what to do next
  /// Knows tableau column in 1, flips in 2 and gets an array for flips (as serial here)
  int whatNext();
  /// see if cutoff reached
  bool checkCutoff(bool computeObjective);
  /// Does something about fake tolerances
  int bounceTolerances(int type);
  /// Perturbs problem
  void perturb(double factor);
  /// Perturbs problem B
  void perturbB(double factor,int type);
  /// Make non free variables dual feasible by moving to a bound
  int makeNonFreeVariablesDualFeasible(bool changeCosts=false);
  int fastDual(bool alwaysFinish = false);
  /** Checks number of variables at fake bounds.  This is used by fastDual
      so can exit gracefully before end */
  int numberAtFakeBound();
  
  /** Pivot in a variable and choose an outgoing one.  Assumes dual
      feasible - will not go through a reduced cost.  Returns step length in theta
      Return codes as before but -1 means no acceptable pivot
  */
  int pivotResultPart1();
  /** Get next free , -1 if none */
  int nextSuperBasic();
  /// Startup part of dual
  void startupSolve();
  /// Ending part of dual 
  void finishSolve();
  void gutsOfDual();
  //int dual2(int ifValuesPass,int startFinishOptions=0);
  int resetFakeBounds(int type);
  
  //@}
};
#if ABC_PARALLEL==1
void * abc_parallelManager(void * simplex);
#endif
#endif
