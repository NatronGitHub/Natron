/* $Id$ */
// Copyright (C) 2004, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).
/*
   Authors

   John Forrest

 */
#ifndef ClpSimplexOther_H
#define ClpSimplexOther_H

#include "ClpSimplex.hpp"

/** This is for Simplex stuff which is neither dual nor primal

    It inherits from ClpSimplex.  It has no data of its own and
    is never created - only cast from a ClpSimplex object at algorithm time.

*/

class ClpSimplexOther : public ClpSimplex {

public:

     /**@name Methods */
     //@{
     /** Dual ranging.
         This computes increase/decrease in cost for each given variable and corresponding
         sequence numbers which would change basis.  Sequence numbers are 0..numberColumns
         and numberColumns.. for artificials/slacks.
         For non-basic variables the information is trivial to compute and the change in cost is just minus the
         reduced cost and the sequence number will be that of the non-basic variables.
         For basic variables a ratio test is between the reduced costs for non-basic variables
         and the row of the tableau corresponding to the basic variable.
         The increase/decrease value is always >= 0.0

         Up to user to provide correct length arrays where each array is of length numberCheck.
         which contains list of variables for which information is desired.  All other
         arrays will be filled in by function.  If fifth entry in which is variable 7 then fifth entry in output arrays
         will be information for variable 7.

         If valueIncrease/Decrease not NULL (both must be NULL or both non NULL) then these are filled with
         the value of variable if such a change in cost were made (the existing bounds are ignored)

         When here - guaranteed optimal
     */
     void dualRanging(int numberCheck, const int * which,
                      double * costIncrease, int * sequenceIncrease,
                      double * costDecrease, int * sequenceDecrease,
                      double * valueIncrease = NULL, double * valueDecrease = NULL);
     /** Primal ranging.
         This computes increase/decrease in value for each given variable and corresponding
         sequence numbers which would change basis.  Sequence numbers are 0..numberColumns
         and numberColumns.. for artificials/slacks.
         This should only be used for non-basic variabls as otherwise information is pretty useless
         For basic variables the sequence number will be that of the basic variables.

         Up to user to provide correct length arrays where each array is of length numberCheck.
         which contains list of variables for which information is desired.  All other
         arrays will be filled in by function.  If fifth entry in which is variable 7 then fifth entry in output arrays
         will be information for variable 7.

         When here - guaranteed optimal
     */
     void primalRanging(int numberCheck, const int * which,
                        double * valueIncrease, int * sequenceIncrease,
                        double * valueDecrease, int * sequenceDecrease);
     /** Parametrics
         This is an initial slow version.
         The code uses current bounds + theta * change (if change array not NULL)
         and similarly for objective.
         It starts at startingTheta and returns ending theta in endingTheta.
         If reportIncrement 0.0 it will report on any movement
         If reportIncrement >0.0 it will report at startingTheta+k*reportIncrement.
         If it can not reach input endingTheta return code will be 1 for infeasible,
         2 for unbounded, if error on ranges -1,  otherwise 0.
         Normal report is just theta and objective but
         if event handler exists it may do more
         On exit endingTheta is maximum reached (can be used for next startingTheta)
     */
     int parametrics(double startingTheta, double & endingTheta, double reportIncrement,
                     const double * changeLowerBound, const double * changeUpperBound,
                     const double * changeLowerRhs, const double * changeUpperRhs,
                     const double * changeObjective);
     /** Version of parametrics which reads from file
	 See CbcClpParam.cpp for details of format
	 Returns -2 if unable to open file */
     int parametrics(const char * dataFile);
     /** Parametrics
         This is an initial slow version.
         The code uses current bounds + theta * change (if change array not NULL)
         It starts at startingTheta and returns ending theta in endingTheta.
         If it can not reach input endingTheta return code will be 1 for infeasible,
         2 for unbounded, if error on ranges -1,  otherwise 0.
         Event handler may do more
         On exit endingTheta is maximum reached (can be used for next startingTheta)
     */
     int parametrics(double startingTheta, double & endingTheta, 
                     const double * changeLowerBound, const double * changeUpperBound,
                     const double * changeLowerRhs, const double * changeUpperRhs);
     int parametricsObj(double startingTheta, double & endingTheta, 
			const double * changeObjective);
    /// Finds best possible pivot
    double bestPivot(bool justColumns=false);
  typedef struct {
    double startingTheta;
    double endingTheta;
    double maxTheta;
    double acceptableMaxTheta; // if this far then within tolerances
    double * lowerChange; // full array of lower bound changes
    int * lowerList; // list of lower bound changes
    double * upperChange; // full array of upper bound changes
    int * upperList; // list of upper bound changes
    char * markDone; // mark which ones looked at
    int * backwardBasic; // from sequence to pivot row
    int * lowerActive;
    double * lowerGap;
    double * lowerCoefficient;
    int * upperActive;
    double * upperGap;
    double * upperCoefficient;
    int unscaledChangesOffset; 
    bool firstIteration; // so can update rhs for accuracy
  } parametricsData;

private:
     /** Parametrics - inner loop
         This first attempt is when reportIncrement non zero and may
         not report endingTheta correctly
         If it can not reach input endingTheta return code will be 1 for infeasible,
         2 for unbounded,  otherwise 0.
         Normal report is just theta and objective but
         if event handler exists it may do more
     */
     int parametricsLoop(parametricsData & paramData, double reportIncrement,
                         const double * changeLower, const double * changeUpper,
                         const double * changeObjective, ClpDataSave & data,
                         bool canTryQuick);
     int parametricsLoop(parametricsData & paramData,
                         ClpDataSave & data,bool canSkipFactorization=false);
     int parametricsObjLoop(parametricsData & paramData,
                         ClpDataSave & data,bool canSkipFactorization=false);
     /**  Refactorizes if necessary
          Checks if finished.  Updates status.

          type - 0 initial so set up save arrays etc
               - 1 normal -if good update save
           - 2 restoring from saved
     */
     void statusOfProblemInParametrics(int type, ClpDataSave & saveData);
     void statusOfProblemInParametricsObj(int type, ClpDataSave & saveData);
     /** This has the flow between re-factorizations

         Reasons to come out:
         -1 iterations etc
         -2 inaccuracy
         -3 slight inaccuracy (and done iterations)
         +0 looks optimal (might be unbounded - but we will investigate)
         +1 looks infeasible
         +3 max iterations
      */
     int whileIterating(parametricsData & paramData, double reportIncrement,
                        const double * changeObjective);
     /** Computes next theta and says if objective or bounds (0= bounds, 1 objective, -1 none).
         theta is in theta_.
         type 1 bounds, 2 objective, 3 both.
     */
     int nextTheta(int type, double maxTheta, parametricsData & paramData,
                   const double * changeObjective);
     int whileIteratingObj(parametricsData & paramData);
     int nextThetaObj(double maxTheta, parametricsData & paramData);
     /// Restores bound to original bound
     void originalBound(int iSequence, double theta, const double * changeLower,
		     const double * changeUpper);
     /// Compute new rowLower_ etc (return negative if infeasible - otherwise largest change)
     double computeRhsEtc(parametricsData & paramData);
     /// Redo lower_ from rowLower_ etc
     void redoInternalArrays();
     /**
         Row array has row part of pivot row
         Column array has column part.
         This is used in dual ranging
     */
     void checkDualRatios(CoinIndexedVector * rowArray,
                          CoinIndexedVector * columnArray,
                          double & costIncrease, int & sequenceIncrease, double & alphaIncrease,
                          double & costDecrease, int & sequenceDecrease, double & alphaDecrease);
     /**
         Row array has pivot column
         This is used in primal ranging
     */
     void checkPrimalRatios(CoinIndexedVector * rowArray,
                            int direction);
     /// Returns new value of whichOther when whichIn enters basis
     double primalRanging1(int whichIn, int whichOther);

public:
     /** Write the basis in MPS format to the specified file.
     If writeValues true writes values of structurals
     (and adds VALUES to end of NAME card)

     Row and column names may be null.
     formatType is
     <ul>
       <li> 0 - normal
       <li> 1 - extra accuracy
       <li> 2 - IEEE hex (later)
     </ul>

     Returns non-zero on I/O error
     */
     int writeBasis(const char *filename,
                    bool writeValues = false,
                    int formatType = 0) const;
     /// Read a basis from the given filename
     int readBasis(const char *filename);
     /** Creates dual of a problem if looks plausible
         (defaults will always create model)
         fractionRowRanges is fraction of rows allowed to have ranges
         fractionColumnRanges is fraction of columns allowed to have ranges
     */
     ClpSimplex * dualOfModel(double fractionRowRanges = 1.0, double fractionColumnRanges = 1.0) const;
     /** Restores solution from dualized problem
         non-zero return code indicates minor problems
     */
  int restoreFromDual(const ClpSimplex * dualProblem,
		      bool checkAccuracy=false);
     /** Does very cursory presolve.
         rhs is numberRows, whichRows is 3*numberRows and whichColumns is 2*numberColumns.
     */
     ClpSimplex * crunch(double * rhs, int * whichRows, int * whichColumns,
                         int & nBound, bool moreBounds = false, bool tightenBounds = false);
     /** After very cursory presolve.
         rhs is numberRows, whichRows is 3*numberRows and whichColumns is 2*numberColumns.
     */
     void afterCrunch(const ClpSimplex & small,
                      const int * whichRows, const int * whichColumns,
                      int nBound);
     /** Returns gub version of model or NULL
	 whichRows has to be numberRows
	 whichColumns has to be numberRows+numberColumns */
     ClpSimplex * gubVersion(int * whichRows, int * whichColumns,
			     int neededGub,
			     int factorizationFrequency=50);
     /// Sets basis from original
     void setGubBasis(ClpSimplex &original,const int * whichRows,
		      const int * whichColumns);
     /// Restores basis to original
     void getGubBasis(ClpSimplex &original,const int * whichRows,
		      const int * whichColumns) const;
     /// Quick try at cleaning up duals if postsolve gets wrong
     void cleanupAfterPostsolve();
     /** Tightens integer bounds - returns number tightened or -1 if infeasible
     */
     int tightenIntegerBounds(double * rhsSpace);
     /** Expands out all possible combinations for a knapsack
         If buildObj NULL then just computes space needed - returns number elements
         On entry numberOutput is maximum allowed, on exit it is number needed or
         -1 (as will be number elements) if maximum exceeded.  numberOutput will have at
         least space to return values which reconstruct input.
         Rows returned will be original rows but no entries will be returned for
         any rows all of whose entries are in knapsack.  So up to user to allow for this.
         If reConstruct >=0 then returns number of entrie which make up item "reConstruct"
         in expanded knapsack.  Values in buildRow and buildElement;
     */
     int expandKnapsack(int knapsackRow, int & numberOutput,
                        double * buildObj, CoinBigIndex * buildStart,
                        int * buildRow, double * buildElement, int reConstruct = -1) const;
     //@}
};
#endif
