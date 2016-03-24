/* $Id$ */
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).
/*
   Authors

   John Forrest

 */
#ifndef ClpSimplexDual_H
#define ClpSimplexDual_H

#include "ClpSimplex.hpp"

/** This solves LPs using the dual simplex method

    It inherits from ClpSimplex.  It has no data of its own and
    is never created - only cast from a ClpSimplex object at algorithm time.

*/

class ClpSimplexDual : public ClpSimplex {

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

        for use of exotic parameter startFinishoptions see Clpsimplex.hpp
     */

     int dual(int ifValuesPass, int startFinishOptions = 0);
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
     ClpFactorization * setupForStrongBranching(char * arrays, int numberRows,
               int numberColumns, bool solveLp = false);
     /// This cleans up after strong branching
     void cleanupAfterStrongBranching(ClpFactorization * factorization);
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

         If givenPi not NULL then in values pass
      */
     int whileIterating(double * & givenPi, int ifValuesPass);
     /** The duals are updated by the given arrays.
         Returns number of infeasibilities.
         After rowArray and columnArray will just have those which
         have been flipped.
         Variables may be flipped between bounds to stay dual feasible.
         The output vector has movement of primal
         solution (row length array) */
     int updateDualsInDual(CoinIndexedVector * rowArray,
                           CoinIndexedVector * columnArray,
                           CoinIndexedVector * outputArray,
                           double theta,
                           double & objectiveChange,
                           bool fullRecompute);
     /** The duals are updated by the given arrays.
         This is in values pass - so no changes to primal is made
     */
     void updateDualsInValuesPass(CoinIndexedVector * rowArray,
                                  CoinIndexedVector * columnArray,
                                  double theta);
     /** While updateDualsInDual sees what effect is of flip
         this does actual flipping.
     */
     void flipBounds(CoinIndexedVector * rowArray,
                     CoinIndexedVector * columnArray);
     /**
         Row array has row part of pivot row
         Column array has column part.
         This chooses pivot column.
         Spare arrays are used to save pivots which will go infeasible
         We will check for basic so spare array will never overflow.
         If necessary will modify costs
         For speed, we may need to go to a bucket approach when many
         variables are being flipped.
         Returns best possible pivot value
     */
     double dualColumn(CoinIndexedVector * rowArray,
                       CoinIndexedVector * columnArray,
                       CoinIndexedVector * spareArray,
                       CoinIndexedVector * spareArray2,
                       double accpetablePivot,
                       CoinBigIndex * dubiousWeights);
     /// Does first bit of dualColumn
     int dualColumn0(const CoinIndexedVector * rowArray,
                     const CoinIndexedVector * columnArray,
                     CoinIndexedVector * spareArray,
                     double acceptablePivot,
                     double & upperReturn, double &bestReturn, double & badFree);
     /**
         Row array has row part of pivot row
         Column array has column part.
         This sees what is best thing to do in dual values pass
         if sequenceIn==sequenceOut can change dual on chosen row and leave variable in basis
     */
     void checkPossibleValuesMove(CoinIndexedVector * rowArray,
                                  CoinIndexedVector * columnArray,
                                  double acceptablePivot);
     /**
         Row array has row part of pivot row
         Column array has column part.
         This sees what is best thing to do in branch and bound cleanup
         If sequenceIn_ < 0 then can't do anything
     */
     void checkPossibleCleanup(CoinIndexedVector * rowArray,
                               CoinIndexedVector * columnArray,
                               double acceptablePivot);
     /**
         This sees if we can move duals in dual values pass.
         This is done before any pivoting
     */
     void doEasyOnesInValuesPass(double * givenReducedCosts);
     /**
         Chooses dual pivot row
         Would be faster with separate region to scan
         and will have this (with square of infeasibility) when steepest
         For easy problems we can just choose one of the first rows we look at

         If alreadyChosen >=0 then in values pass and that row has been
         selected
     */
     void dualRow(int alreadyChosen);
     /** Checks if any fake bounds active - if so returns number and modifies
         updatedDualBound_ and everything.
         Free variables will be left as free
         Returns number of bounds changed if >=0
         Returns -1 if not initialize and no effect
         Fills in changeVector which can be used to see if unbounded
         and cost of change vector
         If 2 sets to original (just changed)
     */
     int changeBounds(int initialize, CoinIndexedVector * outputArray,
                      double & changeCost);
     /** As changeBounds but just changes new bounds for a single variable.
         Returns true if change */
     bool changeBound( int iSequence);
     /// Restores bound to original bound
     void originalBound(int iSequence);
     /** Checks if tentative optimal actually means unbounded in dual
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
     void statusOfProblemInDual(int & lastCleaned, int type,
                                double * givenDjs, ClpDataSave & saveData,
                                int ifValuesPass);
     /** Perturbs problem (method depends on perturbation())
         returns nonzero if should go to dual */
     int perturb();
     /** Fast iterations.  Misses out a lot of initialization.
         Normally stops on maximum iterations, first re-factorization
         or tentative optimum.  If looks interesting then continues as
         normal.  Returns 0 if finished properly, 1 otherwise.
     */
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
     /** Startup part of dual (may be extended to other algorithms)
         returns 0 if good, 1 if bad */
     int startupSolve(int ifValuesPass, double * saveDuals, int startFinishOptions);
     void finishSolve(int startFinishOptions);
     void gutsOfDual(int ifValuesPass, double * & saveDuals, int initialStatus,
                     ClpDataSave & saveData);
     //int dual2(int ifValuesPass,int startFinishOptions=0);
     void resetFakeBounds(int type);

     //@}
};
#endif
