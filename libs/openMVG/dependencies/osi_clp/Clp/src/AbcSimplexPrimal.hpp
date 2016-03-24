/* $Id$ */
// Copyright (C) 2002, International Business Machines
// Corporation and others, Copyright (C) 2012, FasterCoin.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).
/*
   Authors

   John Forrest

 */
#ifndef AbcSimplexPrimal_H
#define AbcSimplexPrimal_H

#include "AbcSimplex.hpp"

/** This solves LPs using the primal simplex method

    It inherits from AbcSimplex.  It has no data of its own and
    is never created - only cast from a AbcSimplex object at algorithm time.

*/

class AbcSimplexPrimal : public AbcSimplex {

public:

     /**@name Description of algorithm */
     //@{
     /** Primal algorithm

         Method

        It tries to be a single phase approach with a weight of 1.0 being
        given to getting optimal and a weight of infeasibilityCost_ being
        given to getting primal feasible.  In this version I have tried to
        be clever in a stupid way.  The idea of fake bounds in dual
        seems to work so the primal analogue would be that of getting
        bounds on reduced costs (by a presolve approach) and using
        these for being above or below feasible region.  I decided to waste
        memory and keep these explicitly.  This allows for non-linear
        costs!  I have not tested non-linear costs but will be glad
        to do something if a reasonable example is provided.

        The code is designed to take advantage of sparsity so arrays are
        seldom zeroed out from scratch or gone over in their entirety.
        The only exception is a full scan to find incoming variable for
        Dantzig row choice.  For steepest edge we keep an updated list
        of dual infeasibilities (actually squares).
        On easy problems we don't need full scan - just
        pick first reasonable.  This method has not been coded.

        One problem is how to tackle degeneracy and accuracy.  At present
        I am using the modification of costs which I put in OSL and which was
        extended by Gill et al.  I am still not sure whether we will also
        need explicit perturbation.

        The flow of primal is three while loops as follows:

        while (not finished) {

          while (not clean solution) {

             Factorize and/or clean up solution by changing bounds so
         primal feasible.  If looks finished check fake primal bounds.
         Repeat until status is iterating (-1) or finished (0,1,2)

          }

          while (status==-1) {

            Iterate until no pivot in or out or time to re-factorize.

            Flow is:

            choose pivot column (incoming variable).  if none then
        we are primal feasible so looks as if done but we need to
        break and check bounds etc.

        Get pivot column in tableau

            Choose outgoing row.  If we don't find one then we look
        primal unbounded so break and check bounds etc.  (Also the
        pivot tolerance is larger after any iterations so that may be
        reason)

            If we do find outgoing row, we may have to adjust costs to
        keep going forwards (anti-degeneracy).  Check pivot will be stable
        and if unstable throw away iteration and break to re-factorize.
        If minor error re-factorize after iteration.

        Update everything (this may involve changing bounds on
        variables to stay primal feasible.

          }

        }

        TODO's (or maybe not)

        At present we never check we are going forwards.  I overdid that in
        OSL so will try and make a last resort.

        Needs partial scan pivot in option.

        May need other anti-degeneracy measures, especially if we try and use
        loose tolerances as a way to solve in fewer iterations.

        I like idea of dynamic scaling.  This gives opportunity to decouple
        different implications of scaling for accuracy, iteration count and
        feasibility tolerance.

        for use of exotic parameter startFinishoptions see Clpsimplex.hpp
     */

     int primal(int ifValuesPass = 0, int startFinishOptions = 0);
     //@}

     /**@name For advanced users */
     //@{
     /// Do not change infeasibility cost and always say optimal
     void alwaysOptimal(bool onOff);
     bool alwaysOptimal() const;
     /** Normally outgoing variables can go out to slightly negative
         values (but within tolerance) - this is to help stability and
         and degeneracy.  This can be switched off
     */
     void exactOutgoing(bool onOff);
     bool exactOutgoing() const;
     //@}

     /**@name Functions used in primal */
     //@{
     /** This has the flow between re-factorizations

         Returns a code to say where decision to exit was made
         Problem status set to:

         -2 re-factorize
         -4 Looks optimal/infeasible
         -5 Looks unbounded
         +3 max iterations

         valuesOption has original value of valuesPass
      */
     int whileIterating(int valuesOption);

     /** Do last half of an iteration.  This is split out so people can
         force incoming variable.  If solveType_ is 2 then this may
         re-factorize while normally it would exit to re-factorize.
         Return codes
         Reasons to come out (normal mode/user mode):
         -1 normal
         -2 factorize now - good iteration/ NA
         -3 slight inaccuracy - refactorize - iteration done/ same but factor done
         -4 inaccuracy - refactorize - no iteration/ NA
         -5 something flagged - go round again/ pivot not possible
         +2 looks unbounded
         +3 max iterations (iteration done)

         With solveType_ ==2 this should
         Pivot in a variable and choose an outgoing one.  Assumes primal
         feasible - will not go through a bound.  Returns step length in theta
         Returns ray in ray_
     */
     int pivotResult(int ifValuesPass = 0);
     int pivotResult4(int ifValuesPass = 0);


     /** The primals are updated by the given array.
         Returns number of infeasibilities.
         After rowArray will have cost changes for use next iteration
     */
     int updatePrimalsInPrimal(CoinIndexedVector * rowArray,
                               double theta,
                               double & objectiveChange,
                               int valuesPass);
     /** The primals are updated by the given array.
	 costs are changed 
     */
     void updatePrimalsInPrimal(CoinIndexedVector & rowArray,
				double theta,bool valuesPass);
     /** After rowArray will have cost changes for use next iteration
     */
     void createUpdateDuals(CoinIndexedVector & rowArray,
			   const double * originalCost,
			    const double extraCost[4],
			   double & objectiveChange,
			   int valuesPass);
     /** Update minor candidate vector - new reduced cost returned
	 later try and get change in reduced cost (then may not need sequence in)*/
     double updateMinorCandidate(const CoinIndexedVector & updateBy,
				 CoinIndexedVector & candidate,
				 int sequenceIn);
     /// Update partial Ftran by R update
     void updatePartialUpdate(CoinIndexedVector & partialUpdate);
  /// Do FT update as separate function for minor iterations (nonzero return code on problems)
  int doFTUpdate(CoinIndexedVector * vector[4]);
     /**
         Row array has pivot column
         This chooses pivot row.
         Rhs array is used for distance to next bound (for speed)
         For speed, we may need to go to a bucket approach when many
         variables go through bounds
         If valuesPass non-zero then compute dj for direction
     */
     void primalRow(CoinIndexedVector * rowArray,
                    CoinIndexedVector * rhsArray,
                    CoinIndexedVector * spareArray,
		    int valuesPass);
  typedef struct {
    double theta_;
    double alpha_;
    double saveDualIn_;
    double dualIn_;
    double lowerIn_;
    double upperIn_;
    double valueIn_;
    int sequenceIn_;
    int directionIn_;
    double dualOut_;
    double lowerOut_;
    double upperOut_;
    double valueOut_;
    int sequenceOut_;
    int directionOut_;
    int pivotRow_;
    int valuesPass_;
  } pivotStruct;
     void primalRow(CoinIndexedVector * rowArray,
                    CoinIndexedVector * rhsArray,
                    CoinIndexedVector * spareArray,
		    pivotStruct & stuff);
     /**
         Chooses primal pivot column
         updateArray has cost updates (also use pivotRow_ from last iteration)
         Would be faster with separate region to scan
         and will have this (with square of infeasibility) when steepest
         For easy problems we can just choose one of the first columns we look at
     */
     void primalColumn(CoinPartitionedVector * updateArray,
                       CoinPartitionedVector * spareRow2,
                       CoinPartitionedVector * spareColumn1);

     /** Checks if tentative optimal actually means unbounded in primal
         Returns -3 if not, 2 if is unbounded */
     int checkUnbounded(CoinIndexedVector * ray, CoinIndexedVector * spare,
                        double changeCost);
     /**  Refactorizes if necessary
          Checks if finished.  Updates status.
          lastCleaned refers to iteration at which some objective/feasibility
          cleaning too place.

          type - 0 initial so set up save arrays etc
               - 1 normal -if good update save
           - 2 restoring from saved
     */
  void statusOfProblemInPrimal(int type);
     /// Perturbs problem (method depends on perturbation())
     void perturb(int type);
     /// Take off effect of perturbation and say whether to try dual
     bool unPerturb();
     /// Unflag all variables and return number unflagged
     int unflag();
     /** Get next superbasic -1 if none,
         Normal type is 1
         If type is 3 then initializes sorted list
         if 2 uses list.
     */
     int nextSuperBasic(int superBasicType, CoinIndexedVector * columnArray);

     /// Create primal ray
     void primalRay(CoinIndexedVector * rowArray);
     /// Clears all bits and clears rowArray[1] etc
     void clearAll();

     /// Sort of lexicographic resolve
     int lexSolve();

     //@}
};
#endif

