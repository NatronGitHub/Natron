/* $Id$ */
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).
/*
   Authors

   John Forrest

 */
#ifndef ClpSimplex_H
#define ClpSimplex_H

#include <iostream>
#include <cfloat>
#include "ClpModel.hpp"
#include "ClpMatrixBase.hpp"
#include "ClpSolve.hpp"
#include "ClpConfig.h"
class ClpDualRowPivot;
class ClpPrimalColumnPivot;
class ClpFactorization;
class CoinIndexedVector;
class ClpNonLinearCost;
class ClpNodeStuff;
class CoinStructuredModel;
class OsiClpSolverInterface;
class CoinWarmStartBasis;
class ClpDisasterHandler;
class ClpConstraint;
#ifdef CLP_HAS_ABC
#include "AbcCommon.hpp"
class AbcTolerancesEtc;
class AbcSimplex;
#include "CoinAbcCommon.hpp"
#endif
/** This solves LPs using the simplex method

    It inherits from ClpModel and all its arrays are created at
    algorithm time. Originally I tried to work with model arrays
    but for simplicity of coding I changed to single arrays with
    structural variables then row variables.  Some coding is still
    based on old style and needs cleaning up.

    For a description of algorithms:

    for dual see ClpSimplexDual.hpp and at top of ClpSimplexDual.cpp
    for primal see ClpSimplexPrimal.hpp and at top of ClpSimplexPrimal.cpp

    There is an algorithm data member.  + for primal variations
    and - for dual variations

*/

class ClpSimplex : public ClpModel {
     friend void ClpSimplexUnitTest(const std::string & mpsDir);

public:
     /** enums for status of various sorts.
         First 4 match CoinWarmStartBasis,
         isFixed means fixed at lower bound and out of basis
     */
     enum Status {
          isFree = 0x00,
          basic = 0x01,
          atUpperBound = 0x02,
          atLowerBound = 0x03,
          superBasic = 0x04,
          isFixed = 0x05
     };
     // For Dual
     enum FakeBound {
          noFake = 0x00,
          lowerFake = 0x01,
          upperFake = 0x02,
          bothFake = 0x03
     };

     /**@name Constructors and destructor and copy */
     //@{
     /// Default constructor
     ClpSimplex (bool emptyMessages = false  );

     /** Copy constructor. May scale depending on mode
         -1 leave mode as is
         0 -off, 1 equilibrium, 2 geometric, 3, auto, 4 dynamic(later)
     */
     ClpSimplex(const ClpSimplex & rhs, int scalingMode = -1);
     /** Copy constructor from model. May scale depending on mode
         -1 leave mode as is
         0 -off, 1 equilibrium, 2 geometric, 3, auto, 4 dynamic(later)
     */
     ClpSimplex(const ClpModel & rhs, int scalingMode = -1);
     /** Subproblem constructor.  A subset of whole model is created from the
         row and column lists given.  The new order is given by list order and
         duplicates are allowed.  Name and integer information can be dropped
         Can optionally modify rhs to take into account variables NOT in list
         in this case duplicates are not allowed (also see getbackSolution)
     */
     ClpSimplex (const ClpModel * wholeModel,
                 int numberRows, const int * whichRows,
                 int numberColumns, const int * whichColumns,
                 bool dropNames = true, bool dropIntegers = true,
                 bool fixOthers = false);
     /** Subproblem constructor.  A subset of whole model is created from the
         row and column lists given.  The new order is given by list order and
         duplicates are allowed.  Name and integer information can be dropped
         Can optionally modify rhs to take into account variables NOT in list
         in this case duplicates are not allowed (also see getbackSolution)
     */
     ClpSimplex (const ClpSimplex * wholeModel,
                 int numberRows, const int * whichRows,
                 int numberColumns, const int * whichColumns,
                 bool dropNames = true, bool dropIntegers = true,
                 bool fixOthers = false);
     /** This constructor modifies original ClpSimplex and stores
         original stuff in created ClpSimplex.  It is only to be used in
         conjunction with originalModel */
     ClpSimplex (ClpSimplex * wholeModel,
                 int numberColumns, const int * whichColumns);
     /** This copies back stuff from miniModel and then deletes miniModel.
         Only to be used with mini constructor */
     void originalModel(ClpSimplex * miniModel);
  inline int abcState() const
  { return abcState_;}
  inline void setAbcState(int state)
  { abcState_=state;}
#ifdef ABC_INHERIT
  inline AbcSimplex * abcSimplex() const
  { return abcSimplex_;}
  inline void setAbcSimplex(AbcSimplex * simplex)
  { abcSimplex_=simplex;}
  /// Returns 0 if dual can be skipped
  int doAbcDual();
  /// Returns 0 if primal can be skipped
  int doAbcPrimal(int ifValuesPass);
#endif
     /** Array persistence flag
         If 0 then as now (delete/new)
         1 then only do arrays if bigger needed
         2 as 1 but give a bit extra if bigger needed
     */
     void setPersistenceFlag(int value);
     /// Save a copy of model with certain state - normally without cuts
     void makeBaseModel();
     /// Switch off base model
     void deleteBaseModel();
     /// See if we have base model
     inline ClpSimplex *  baseModel() const {
          return baseModel_;
     }
     /** Reset to base model (just size and arrays needed)
         If model NULL use internal copy
     */
     void setToBaseModel(ClpSimplex * model = NULL);
     /// Assignment operator. This copies the data
     ClpSimplex & operator=(const ClpSimplex & rhs);
     /// Destructor
     ~ClpSimplex (  );
     // Ones below are just ClpModel with some changes
     /** Loads a problem (the constraints on the
           rows are given by lower and upper bounds). If a pointer is 0 then the
           following values are the default:
           <ul>
             <li> <code>colub</code>: all columns have upper bound infinity
             <li> <code>collb</code>: all columns have lower bound 0
             <li> <code>rowub</code>: all rows have upper bound infinity
             <li> <code>rowlb</code>: all rows have lower bound -infinity
         <li> <code>obj</code>: all variables have 0 objective coefficient
           </ul>
       */
     void loadProblem (  const ClpMatrixBase& matrix,
                         const double* collb, const double* colub,
                         const double* obj,
                         const double* rowlb, const double* rowub,
                         const double * rowObjective = NULL);
     void loadProblem (  const CoinPackedMatrix& matrix,
                         const double* collb, const double* colub,
                         const double* obj,
                         const double* rowlb, const double* rowub,
                         const double * rowObjective = NULL);

     /** Just like the other loadProblem() method except that the matrix is
       given in a standard column major ordered format (without gaps). */
     void loadProblem (  const int numcols, const int numrows,
                         const CoinBigIndex* start, const int* index,
                         const double* value,
                         const double* collb, const double* colub,
                         const double* obj,
                         const double* rowlb, const double* rowub,
                         const double * rowObjective = NULL);
     /// This one is for after presolve to save memory
     void loadProblem (  const int numcols, const int numrows,
                         const CoinBigIndex* start, const int* index,
                         const double* value, const int * length,
                         const double* collb, const double* colub,
                         const double* obj,
                         const double* rowlb, const double* rowub,
                         const double * rowObjective = NULL);
     /** This loads a model from a coinModel object - returns number of errors.
         If keepSolution true and size is same as current then
         keeps current status and solution
     */
     int loadProblem (  CoinModel & modelObject, bool keepSolution = false);
     /// Read an mps file from the given filename
     int readMps(const char *filename,
                 bool keepNames = false,
                 bool ignoreErrors = false);
     /// Read GMPL files from the given filenames
     int readGMPL(const char *filename, const char * dataName,
                  bool keepNames = false);
     /// Read file in LP format from file with name filename.
     /// See class CoinLpIO for description of this format.
     int readLp(const char *filename, const double epsilon = 1e-5);
     /** Borrow model.  This is so we dont have to copy large amounts
         of data around.  It assumes a derived class wants to overwrite
         an empty model with a real one - while it does an algorithm.
         This is same as ClpModel one, but sets scaling on etc. */
     void borrowModel(ClpModel & otherModel);
     void borrowModel(ClpSimplex & otherModel);
     /// Pass in Event handler (cloned and deleted at end)
     void passInEventHandler(const ClpEventHandler * eventHandler);
     /// Puts solution back into small model
     void getbackSolution(const ClpSimplex & smallModel, const int * whichRow, const int * whichColumn);
     /** Load nonlinear part of problem from AMPL info
         Returns 0 if linear
         1 if quadratic objective
         2 if quadratic constraints
         3 if nonlinear objective
         4 if nonlinear constraints
         -1 on failure
     */
     int loadNonLinear(void * info, int & numberConstraints,
                       ClpConstraint ** & constraints);
#ifdef ABC_INHERIT
  /// Loads tolerances etc
  void loadTolerancesEtc(const AbcTolerancesEtc & data);
  /// Unloads tolerances etc
  void unloadTolerancesEtc(AbcTolerancesEtc & data);
#endif
     //@}

     /**@name Functions most useful to user */
     //@{
     /** General solve algorithm which can do presolve.
         See  ClpSolve.hpp for options
      */
     int initialSolve(ClpSolve & options);
     /// Default initial solve
     int initialSolve();
     /// Dual initial solve
     int initialDualSolve();
     /// Primal initial solve
     int initialPrimalSolve();
/// Barrier initial solve
     int initialBarrierSolve();
     /// Barrier initial solve, not to be followed by crossover
     int initialBarrierNoCrossSolve();
     /** Dual algorithm - see ClpSimplexDual.hpp for method.
         ifValuesPass==2 just does values pass and then stops.

         startFinishOptions - bits
         1 - do not delete work areas and factorization at end
         2 - use old factorization if same number of rows
         4 - skip as much initialization of work areas as possible
             (based on whatsChanged in clpmodel.hpp) ** work in progress
         maybe other bits later
     */
     int dual(int ifValuesPass = 0, int startFinishOptions = 0);
     // If using Debug
     int dualDebug(int ifValuesPass = 0, int startFinishOptions = 0);
     /** Primal algorithm - see ClpSimplexPrimal.hpp for method.
         ifValuesPass==2 just does values pass and then stops.

         startFinishOptions - bits
         1 - do not delete work areas and factorization at end
         2 - use old factorization if same number of rows
         4 - skip as much initialization of work areas as possible
             (based on whatsChanged in clpmodel.hpp) ** work in progress
         maybe other bits later
     */
     int primal(int ifValuesPass = 0, int startFinishOptions = 0);
     /** Solves nonlinear problem using SLP - may be used as crash
         for other algorithms when number of iterations small.
         Also exits if all problematical variables are changing
         less than deltaTolerance
     */
     int nonlinearSLP(int numberPasses, double deltaTolerance);
     /** Solves problem with nonlinear constraints using SLP - may be used as crash
         for other algorithms when number of iterations small.
         Also exits if all problematical variables are changing
         less than deltaTolerance
     */
     int nonlinearSLP(int numberConstraints, ClpConstraint ** constraints,
                      int numberPasses, double deltaTolerance);
     /** Solves using barrier (assumes you have good cholesky factor code).
         Does crossover to simplex if asked*/
     int barrier(bool crossover = true);
     /** Solves non-linear using reduced gradient.  Phase = 0 get feasible,
         =1 use solution */
     int reducedGradient(int phase = 0);
     /// Solve using structure of model and maybe in parallel
     int solve(CoinStructuredModel * model);
#ifdef ABC_INHERIT
  /** solvetype 0 for dual, 1 for primal
      startup 1 for values pass
      interrupt whether to pass across interrupt handler
  */
  void dealWithAbc(int solveType,int startUp,bool interrupt=false);
#endif
     /** This loads a model from a CoinStructuredModel object - returns number of errors.
         If originalOrder then keep to order stored in blocks,
         otherwise first column/rows correspond to first block - etc.
         If keepSolution true and size is same as current then
         keeps current status and solution
     */
     int loadProblem (  CoinStructuredModel & modelObject,
                        bool originalOrder = true, bool keepSolution = false);
     /**
        When scaling is on it is possible that the scaled problem
        is feasible but the unscaled is not.  Clp returns a secondary
        status code to that effect.  This option allows for a cleanup.
        If you use it I would suggest 1.
        This only affects actions when scaled optimal
        0 - no action
        1 - clean up using dual if primal infeasibility
        2 - clean up using dual if dual infeasibility
        3 - clean up using dual if primal or dual infeasibility
        11,12,13 - as 1,2,3 but use primal

        return code as dual/primal
     */
     int cleanup(int cleanupScaling);
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

         Returns non-zero if infeasible unbounded etc
     */
     int dualRanging(int numberCheck, const int * which,
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

         Returns non-zero if infeasible unbounded etc
     */
     int primalRanging(int numberCheck, const int * which,
                       double * valueIncrease, int * sequenceIncrease,
                       double * valueDecrease, int * sequenceDecrease);
     /**
	Modifies coefficients etc and if necessary pivots in and out.
	All at same status will be done (basis may go singular).
	User can tell which others have been done (i.e. if status matches).
	If called from outside will change status and return 0.
	If called from event handler returns non-zero if user has to take action.
	indices>=numberColumns are slacks (obviously no coefficients)
	status array is (char) Status enum
     */
     int modifyCoefficientsAndPivot(int number,
				 const int * which,
				 const CoinBigIndex * start,
				 const int * row,
				 const double * newCoefficient,
				 const unsigned char * newStatus=NULL,
				 const double * newLower=NULL,
				 const double * newUpper=NULL,
				 const double * newObjective=NULL);
     /** Take out duplicate rows (includes scaled rows and intersections).
	 On exit whichRows has rows to delete - return code is number can be deleted 
	 or -1 if would be infeasible.
	 If tolerance is -1.0 use primalTolerance for equality rows and infeasibility
	 If cleanUp not zero then spend more time trying to leave more stable row
	 and make row bounds exact multiple of cleanUp if close enough
     */
     int outDuplicateRows(int numberLook,int * whichRows, bool noOverlaps=false, double tolerance=-1.0,
			  double cleanUp=0.0);
     /** Try simple crash like techniques to get closer to primal feasibility
	 returns final sum of infeasibilities */
     double moveTowardsPrimalFeasible();
     /** Try simple crash like techniques to remove super basic slacks
	 but only if > threshold */
     void removeSuperBasicSlacks(int threshold=0);
     /** Mini presolve (faster)
	 Char arrays must be numberRows and numberColumns long
	 on entry second part must be filled in as follows -
	 0 - possible
	 >0 - take out and do something (depending on value - TBD)
	 -1 row/column can't vanish but can have entries removed/changed
	 -2 don't touch at all
	 on exit <=0 ones will be in presolved problem
	 struct will be created and will be long enough
	 (information on length etc in first entry)
	 user must delete struct
     */
     ClpSimplex * miniPresolve(char * rowType, char * columnType,void ** info);
     /// After mini presolve
     void miniPostsolve(const ClpSimplex * presolvedModel,void * info);
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
     /** Read a basis from the given filename,
         returns -1 on file error, 0 if no values, 1 if values */
     int readBasis(const char *filename);
     /// Returns a basis (to be deleted by user)
     CoinWarmStartBasis * getBasis() const;
     /// Passes in factorization
     void setFactorization( ClpFactorization & factorization);
     // Swaps factorization
     ClpFactorization * swapFactorization( ClpFactorization * factorization);
     /// Copies in factorization to existing one
     void copyFactorization( ClpFactorization & factorization);
     /** Tightens primal bounds to make dual faster.  Unless
         fixed or doTight>10, bounds are slightly looser than they could be.
         This is to make dual go faster and is probably not needed
         with a presolve.  Returns non-zero if problem infeasible.

         Fudge for branch and bound - put bounds on columns of factor *
         largest value (at continuous) - should improve stability
         in branch and bound on infeasible branches (0.0 is off)
     */
     int tightenPrimalBounds(double factor = 0.0, int doTight = 0, bool tightIntegers = false);
     /** Crash - at present just aimed at dual, returns
         -2 if dual preferred and crash basis created
         -1 if dual preferred and all slack basis preferred
          0 if basis going in was not all slack
          1 if primal preferred and all slack basis preferred
          2 if primal preferred and crash basis created.

          if gap between bounds <="gap" variables can be flipped
          ( If pivot -1 then can be made super basic!)

          If "pivot" is
          -1 No pivoting - always primal
          0 No pivoting (so will just be choice of algorithm)
          1 Simple pivoting e.g. gub
          2 Mini iterations
     */
     int crash(double gap, int pivot);
     /// Sets row pivot choice algorithm in dual
     void setDualRowPivotAlgorithm(ClpDualRowPivot & choice);
     /// Sets column pivot choice algorithm in primal
     void setPrimalColumnPivotAlgorithm(ClpPrimalColumnPivot & choice);
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
     /// Fathom - 1 if solution
     int fathom(void * stuff);
     /** Do up to N deep - returns
         -1 - no solution nNodes_ valid nodes
         >= if solution and that node gives solution
         ClpNode array is 2**N long.  Values for N and
         array are in stuff (nNodes_ also in stuff) */
     int fathomMany(void * stuff);
     /// Double checks OK
     double doubleCheck();
     /// Starts Fast dual2
     int startFastDual2(ClpNodeStuff * stuff);
     /// Like Fast dual
     int fastDual2(ClpNodeStuff * stuff);
     /// Stops Fast dual2
     void stopFastDual2(ClpNodeStuff * stuff);
     /** Deals with crunch aspects
         mode 0 - in
              1 - out with solution
          2 - out without solution
         returns small model or NULL
     */
     ClpSimplex * fastCrunch(ClpNodeStuff * stuff, int mode);
     //@}

     /**@name Needed for functionality of OsiSimplexInterface */
     //@{
     /** Pivot in a variable and out a variable.  Returns 0 if okay,
         1 if inaccuracy forced re-factorization, -1 if would be singular.
         Also updates primal/dual infeasibilities.
         Assumes sequenceIn_ and pivotRow_ set and also directionIn and Out.
     */
     int pivot();

     /** Pivot in a variable and choose an outgoing one.  Assumes primal
         feasible - will not go through a bound.  Returns step length in theta
         Returns ray in ray_ (or NULL if no pivot)
         Return codes as before but -1 means no acceptable pivot
     */
     int primalPivotResult();

     /** Pivot out a variable and choose an incoing one.  Assumes dual
         feasible - will not go through a reduced cost.
         Returns step length in theta
         Return codes as before but -1 means no acceptable pivot
     */
     int dualPivotResultPart1();
     /** Do actual pivot
	 state is 0 if need tableau column, 1 if in rowArray_[1]
     */
  int pivotResultPart2(int algorithm,int state);

     /** Common bits of coding for dual and primal.  Return 0 if okay,
         1 if bad matrix, 2 if very bad factorization

         startFinishOptions - bits
         1 - do not delete work areas and factorization at end
         2 - use old factorization if same number of rows
         4 - skip as much initialization of work areas as possible
             (based on whatsChanged in clpmodel.hpp) ** work in progress
         maybe other bits later

     */
     int startup(int ifValuesPass, int startFinishOptions = 0);
     void finish(int startFinishOptions = 0);

     /** Factorizes and returns true if optimal.  Used by user */
     bool statusOfProblem(bool initial = false);
     /// If user left factorization frequency then compute
     void defaultFactorizationFrequency();
     /// Copy across enabled stuff from one solver to another
     void copyEnabledStuff(const ClpSimplex * rhs);
     //@}

     /**@name most useful gets and sets */
     //@{
     /// If problem is primal feasible
     inline bool primalFeasible() const {
          return (numberPrimalInfeasibilities_ == 0);
     }
     /// If problem is dual feasible
     inline bool dualFeasible() const {
          return (numberDualInfeasibilities_ == 0);
     }
     /// factorization
     inline ClpFactorization * factorization() const {
          return factorization_;
     }
     /// Sparsity on or off
     bool sparseFactorization() const;
     void setSparseFactorization(bool value);
     /// Factorization frequency
     int factorizationFrequency() const;
     void setFactorizationFrequency(int value);
     /// Dual bound
     inline double dualBound() const {
          return dualBound_;
     }
     void setDualBound(double value);
     /// Infeasibility cost
     inline double infeasibilityCost() const {
          return infeasibilityCost_;
     }
     void setInfeasibilityCost(double value);
     /** Amount of print out:
         0 - none
         1 - just final
         2 - just factorizations
         3 - as 2 plus a bit more
         4 - verbose
         above that 8,16,32 etc just for selective debug
     */
     /** Perturbation:
         50  - switch on perturbation
         100 - auto perturb if takes too long (1.0e-6 largest nonzero)
         101 - we are perturbed
         102 - don't try perturbing again
         default is 100
         others are for playing
     */
     inline int perturbation() const {
          return perturbation_;
     }
     void setPerturbation(int value);
     /// Current (or last) algorithm
     inline int algorithm() const {
          return algorithm_;
     }
     /// Set algorithm
     inline void setAlgorithm(int value) {
          algorithm_ = value;
     }
     /// Return true if the objective limit test can be relied upon
     bool isObjectiveLimitTestValid() const ;
     /// Sum of dual infeasibilities
     inline double sumDualInfeasibilities() const {
          return sumDualInfeasibilities_;
     }
     inline void setSumDualInfeasibilities(double value) {
          sumDualInfeasibilities_ = value;
     }
     /// Sum of relaxed dual infeasibilities
     inline double sumOfRelaxedDualInfeasibilities() const {
          return sumOfRelaxedDualInfeasibilities_;
     }
     inline void setSumOfRelaxedDualInfeasibilities(double value) {
          sumOfRelaxedDualInfeasibilities_ = value;
     }
     /// Number of dual infeasibilities
     inline int numberDualInfeasibilities() const {
          return numberDualInfeasibilities_;
     }
     inline void setNumberDualInfeasibilities(int value) {
          numberDualInfeasibilities_ = value;
     }
     /// Number of dual infeasibilities (without free)
     inline int numberDualInfeasibilitiesWithoutFree() const {
          return numberDualInfeasibilitiesWithoutFree_;
     }
     /// Sum of primal infeasibilities
     inline double sumPrimalInfeasibilities() const {
          return sumPrimalInfeasibilities_;
     }
     inline void setSumPrimalInfeasibilities(double value) {
          sumPrimalInfeasibilities_ = value;
     }
     /// Sum of relaxed primal infeasibilities
     inline double sumOfRelaxedPrimalInfeasibilities() const {
          return sumOfRelaxedPrimalInfeasibilities_;
     }
     inline void setSumOfRelaxedPrimalInfeasibilities(double value) {
          sumOfRelaxedPrimalInfeasibilities_ = value;
     }
     /// Number of primal infeasibilities
     inline int numberPrimalInfeasibilities() const {
          return numberPrimalInfeasibilities_;
     }
     inline void setNumberPrimalInfeasibilities(int value) {
          numberPrimalInfeasibilities_ = value;
     }
     /** Save model to file, returns 0 if success.  This is designed for
         use outside algorithms so does not save iterating arrays etc.
     It does not save any messaging information.
     Does not save scaling values.
     It does not know about all types of virtual functions.
     */
     int saveModel(const char * fileName);
     /** Restore model from file, returns 0 if success,
         deletes current model */
     int restoreModel(const char * fileName);

     /** Just check solution (for external use) - sets sum of
         infeasibilities etc.
         If setToBounds 0 then primal column values not changed
         and used to compute primal row activity values.  If 1 or 2
         then status used - so all nonbasic variables set to
         indicated bound and if any values changed (or ==2)  basic values re-computed.
     */
     void checkSolution(int setToBounds = 0);
     /** Just check solution (for internal use) - sets sum of
         infeasibilities etc. */
     void checkSolutionInternal();
     /// Check unscaled primal solution but allow for rounding error
     void checkUnscaledSolution();
     /// Useful row length arrays (0,1,2,3,4,5)
     inline CoinIndexedVector * rowArray(int index) const {
          return rowArray_[index];
     }
     /// Useful column length arrays (0,1,2,3,4,5)
     inline CoinIndexedVector * columnArray(int index) const {
          return columnArray_[index];
     }
     //@}

     /******************** End of most useful part **************/
     /**@name Functions less likely to be useful to casual user */
     //@{
     /** Given an existing factorization computes and checks
         primal and dual solutions.  Uses input arrays for variables at
         bounds.  Returns feasibility states */
     int getSolution (  const double * rowActivities,
                        const double * columnActivities);
     /** Given an existing factorization computes and checks
         primal and dual solutions.  Uses current problem arrays for
         bounds.  Returns feasibility states */
     int getSolution ();
     /** Constructs a non linear cost from list of non-linearities (columns only)
         First lower of each column is taken as real lower
         Last lower is taken as real upper and cost ignored

         Returns nonzero if bad data e.g. lowers not monotonic
     */
     int createPiecewiseLinearCosts(const int * starts,
                                    const double * lower, const double * gradient);
     /// dual row pivot choice
     inline ClpDualRowPivot * dualRowPivot() const {
          return dualRowPivot_;
     }
     /// primal column pivot choice
     inline ClpPrimalColumnPivot * primalColumnPivot() const {
          return primalColumnPivot_;
     }
     /// Returns true if model looks OK
     inline bool goodAccuracy() const {
          return (largestPrimalError_ < 1.0e-7 && largestDualError_ < 1.0e-7);
     }
     /** Return model - updates any scalars */
     void returnModel(ClpSimplex & otherModel);
     /** Factorizes using current basis.
         solveType - 1 iterating, 0 initial, -1 external
         If 10 added then in primal values pass
         Return codes are as from ClpFactorization unless initial factorization
         when total number of singularities is returned.
         Special case is numberRows_+1 -> all slack basis.
     */
     int internalFactorize(int solveType);
     /// Save data
     ClpDataSave saveData() ;
     /// Restore data
     void restoreData(ClpDataSave saved);
     /// Clean up status
     void cleanStatus();
     /// Factorizes using current basis. For external use
     int factorize();
     /** Computes duals from scratch. If givenDjs then
         allows for nonzero basic djs */
     void computeDuals(double * givenDjs);
     /// Computes primals from scratch
     void computePrimals (  const double * rowActivities,
                            const double * columnActivities);
     /** Adds multiple of a column into an array */
     void add(double * array,
              int column, double multiplier) const;
     /**
        Unpacks one column of the matrix into indexed array
        Uses sequenceIn_
        Also applies scaling if needed
     */
     void unpack(CoinIndexedVector * rowArray) const ;
     /**
        Unpacks one column of the matrix into indexed array
        Slack if sequence>= numberColumns
        Also applies scaling if needed
     */
     void unpack(CoinIndexedVector * rowArray, int sequence) const;
     /**
        Unpacks one column of the matrix into indexed array
        ** as packed vector
        Uses sequenceIn_
        Also applies scaling if needed
     */
     void unpackPacked(CoinIndexedVector * rowArray) ;
     /**
        Unpacks one column of the matrix into indexed array
        ** as packed vector
        Slack if sequence>= numberColumns
        Also applies scaling if needed
     */
     void unpackPacked(CoinIndexedVector * rowArray, int sequence);
#ifndef CLP_USER_DRIVEN
protected:
#endif
     /**
         This does basis housekeeping and does values for in/out variables.
         Can also decide to re-factorize
     */
     int housekeeping(double objectiveChange);
     /** This sets largest infeasibility and most infeasible and sum
         and number of infeasibilities (Primal) */
     void checkPrimalSolution(const double * rowActivities = NULL,
                              const double * columnActivies = NULL);
     /** This sets largest infeasibility and most infeasible and sum
         and number of infeasibilities (Dual) */
     void checkDualSolution();
     /** This sets sum and number of infeasibilities (Dual and Primal) */
     void checkBothSolutions();
     /**  If input negative scales objective so maximum <= -value
          and returns scale factor used.  If positive unscales and also
          redoes dual stuff
     */
     double scaleObjective(double value);
     /// Solve using Dantzig-Wolfe decomposition and maybe in parallel
     int solveDW(CoinStructuredModel * model);
     /// Solve using Benders decomposition and maybe in parallel
     int solveBenders(CoinStructuredModel * model);
public:
     /** For advanced use.  When doing iterative solves things can get
         nasty so on values pass if incoming solution has largest
         infeasibility < incomingInfeasibility throw out variables
         from basis until largest infeasibility < allowedInfeasibility
         or incoming largest infeasibility.
         If allowedInfeasibility>= incomingInfeasibility this is
         always possible altough you may end up with an all slack basis.

         Defaults are 1.0,10.0
     */
     void setValuesPassAction(double incomingInfeasibility,
                              double allowedInfeasibility);
     /** Get a clean factorization - i.e. throw out singularities
	 may do more later */
     int cleanFactorization(int ifValuesPass);
     //@}
     /**@name most useful gets and sets */
     //@{
public:
     /// Initial value for alpha accuracy calculation (-1.0 off)
     inline double alphaAccuracy() const {
          return alphaAccuracy_;
     }
     inline void setAlphaAccuracy(double value) {
          alphaAccuracy_ = value;
     }
public:
     /// Objective value
     //inline double objectiveValue() const {
     //return (objectiveValue_-bestPossibleImprovement_)*optimizationDirection_ - dblParam_[ClpObjOffset];
     //}
     /// Set disaster handler
     inline void setDisasterHandler(ClpDisasterHandler * handler) {
          disasterArea_ = handler;
     }
     /// Get disaster handler
     inline ClpDisasterHandler * disasterHandler() const {
          return disasterArea_;
     }
     /// Large bound value (for complementarity etc)
     inline double largeValue() const {
          return largeValue_;
     }
     void setLargeValue( double value) ;
     /// Largest error on Ax-b
     inline double largestPrimalError() const {
          return largestPrimalError_;
     }
     /// Largest error on basic duals
     inline double largestDualError() const {
          return largestDualError_;
     }
     /// Largest error on Ax-b
     inline void setLargestPrimalError(double value) {
          largestPrimalError_ = value;
     }
     /// Largest error on basic duals
     inline void setLargestDualError(double value) {
          largestDualError_ = value;
     }
     /// Get zero tolerance
     inline double zeroTolerance() const {
          return zeroTolerance_;/*factorization_->zeroTolerance();*/
     }
     /// Set zero tolerance
     inline void setZeroTolerance( double value) {
          zeroTolerance_ = value;
     }
     /// Basic variables pivoting on which rows
     inline int * pivotVariable() const {
          return pivotVariable_;
     }
     /// If automatic scaling on
     inline bool automaticScaling() const {
          return automaticScale_ != 0;
     }
     inline void setAutomaticScaling(bool onOff) {
          automaticScale_ = onOff ? 1 : 0;
     }
     /// Current dual tolerance
     inline double currentDualTolerance() const {
          return dualTolerance_;
     }
     inline void setCurrentDualTolerance(double value) {
          dualTolerance_ = value;
     }
     /// Current primal tolerance
     inline double currentPrimalTolerance() const {
          return primalTolerance_;
     }
     inline void setCurrentPrimalTolerance(double value) {
          primalTolerance_ = value;
     }
     /// How many iterative refinements to do
     inline int numberRefinements() const {
          return numberRefinements_;
     }
     void setNumberRefinements( int value) ;
     /// Alpha (pivot element) for use by classes e.g. steepestedge
     inline double alpha() const {
          return alpha_;
     }
     inline void setAlpha(double value) {
          alpha_ = value;
     }
     /// Reduced cost of last incoming for use by classes e.g. steepestedge
     inline double dualIn() const {
          return dualIn_;
     }
     /// Set reduced cost of last incoming to force error
     inline void setDualIn(double value) {
          dualIn_ = value;
     }
     /// Pivot Row for use by classes e.g. steepestedge
     inline int pivotRow() const {
          return pivotRow_;
     }
     inline void setPivotRow(int value) {
          pivotRow_ = value;
     }
     /// value of incoming variable (in Dual)
     double valueIncomingDual() const;
     //@}

#ifndef CLP_USER_DRIVEN
protected:
#endif
     /**@name protected methods */
     //@{
     /** May change basis and then returns number changed.
         Computation of solutions may be overriden by given pi and solution
     */
     int gutsOfSolution ( double * givenDuals,
                          const double * givenPrimals,
                          bool valuesPass = false);
     /// Does most of deletion (0 = all, 1 = most, 2 most + factorization)
     void gutsOfDelete(int type);
     /// Does most of copying
     void gutsOfCopy(const ClpSimplex & rhs);
     /** puts in format I like (rowLower,rowUpper) also see StandardMatrix
         1 bit does rows (now and columns), (2 bit does column bounds), 4 bit does objective(s).
         8 bit does solution scaling in
         16 bit does rowArray and columnArray indexed vectors
         and makes row copy if wanted, also sets columnStart_ etc
         Also creates scaling arrays if needed.  It does scaling if needed.
         16 also moves solutions etc in to work arrays
         On 16 returns false if problem "bad" i.e. matrix or bounds bad
         If startFinishOptions is -1 then called by user in getSolution
         so do arrays but keep pivotVariable_
     */
     bool createRim(int what, bool makeRowCopy = false, int startFinishOptions = 0);
     /// Does rows and columns
     void createRim1(bool initial);
     /// Does objective
     void createRim4(bool initial);
     /// Does rows and columns and objective
     void createRim5(bool initial);
     /** releases above arrays and does solution scaling out.  May also
         get rid of factorization data -
         0 get rid of nothing, 1 get rid of arrays, 2 also factorization
     */
     void deleteRim(int getRidOfFactorizationData = 2);
     /// Sanity check on input rim data (after scaling) - returns true if okay
     bool sanityCheck();
     //@}
public:
     /**@name public methods */
     //@{
     /** Return row or column sections - not as much needed as it
         once was.  These just map into single arrays */
     inline double * solutionRegion(int section) const {
          if (!section) return rowActivityWork_;
          else return columnActivityWork_;
     }
     inline double * djRegion(int section) const {
          if (!section) return rowReducedCost_;
          else return reducedCostWork_;
     }
     inline double * lowerRegion(int section) const {
          if (!section) return rowLowerWork_;
          else return columnLowerWork_;
     }
     inline double * upperRegion(int section) const {
          if (!section) return rowUpperWork_;
          else return columnUpperWork_;
     }
     inline double * costRegion(int section) const {
          if (!section) return rowObjectiveWork_;
          else return objectiveWork_;
     }
     /// Return region as single array
     inline double * solutionRegion() const {
          return solution_;
     }
     inline double * djRegion() const {
          return dj_;
     }
     inline double * lowerRegion() const {
          return lower_;
     }
     inline double * upperRegion() const {
          return upper_;
     }
     inline double * costRegion() const {
          return cost_;
     }
     inline Status getStatus(int sequence) const {
          return static_cast<Status> (status_[sequence] & 7);
     }
     inline void setStatus(int sequence, Status newstatus) {
          unsigned char & st_byte = status_[sequence];
          st_byte = static_cast<unsigned char>(st_byte & ~7);
          st_byte = static_cast<unsigned char>(st_byte | newstatus);
     }
     /// Start or reset using maximumRows_ and Columns_ - true if change
     bool startPermanentArrays();
     /** Normally the first factorization does sparse coding because
         the factorization could be singular.  This allows initial dense
         factorization when it is known to be safe
     */
     void setInitialDenseFactorization(bool onOff);
     bool  initialDenseFactorization() const;
     /** Return sequence In or Out */
     inline int sequenceIn() const {
          return sequenceIn_;
     }
     inline int sequenceOut() const {
          return sequenceOut_;
     }
     /** Set sequenceIn or Out */
     inline void  setSequenceIn(int sequence) {
          sequenceIn_ = sequence;
     }
     inline void  setSequenceOut(int sequence) {
          sequenceOut_ = sequence;
     }
     /** Return direction In or Out */
     inline int directionIn() const {
          return directionIn_;
     }
     inline int directionOut() const {
          return directionOut_;
     }
     /** Set directionIn or Out */
     inline void  setDirectionIn(int direction) {
          directionIn_ = direction;
     }
     inline void  setDirectionOut(int direction) {
          directionOut_ = direction;
     }
     /// Value of Out variable
     inline double valueOut() const {
          return valueOut_;
     }
     /// Set value of out variable
     inline void setValueOut(double value) {
          valueOut_ = value;
     }
     /// Dual value of Out variable
     inline double dualOut() const {
          return dualOut_;
     }
     /// Set dual value of out variable
     inline void setDualOut(double value) {
          dualOut_ = value;
     }
     /// Set lower of out variable
     inline void setLowerOut(double value) {
          lowerOut_ = value;
     }
     /// Set upper of out variable
     inline void setUpperOut(double value) {
          upperOut_ = value;
     }
     /// Set theta of out variable
     inline void setTheta(double value) {
          theta_ = value;
     }
     /// Returns 1 if sequence indicates column
     inline int isColumn(int sequence) const {
          return sequence < numberColumns_ ? 1 : 0;
     }
     /// Returns sequence number within section
     inline int sequenceWithin(int sequence) const {
          return sequence < numberColumns_ ? sequence : sequence - numberColumns_;
     }
     /// Return row or column values
     inline double solution(int sequence) {
          return solution_[sequence];
     }
     /// Return address of row or column values
     inline double & solutionAddress(int sequence) {
          return solution_[sequence];
     }
     inline double reducedCost(int sequence) {
          return dj_[sequence];
     }
     inline double & reducedCostAddress(int sequence) {
          return dj_[sequence];
     }
     inline double lower(int sequence) {
          return lower_[sequence];
     }
     /// Return address of row or column lower bound
     inline double & lowerAddress(int sequence) {
          return lower_[sequence];
     }
     inline double upper(int sequence) {
          return upper_[sequence];
     }
     /// Return address of row or column upper bound
     inline double & upperAddress(int sequence) {
          return upper_[sequence];
     }
     inline double cost(int sequence) {
          return cost_[sequence];
     }
     /// Return address of row or column cost
     inline double & costAddress(int sequence) {
          return cost_[sequence];
     }
     /// Return original lower bound
     inline double originalLower(int iSequence) const {
          if (iSequence < numberColumns_) return columnLower_[iSequence];
          else
               return rowLower_[iSequence-numberColumns_];
     }
     /// Return original lower bound
     inline double originalUpper(int iSequence) const {
          if (iSequence < numberColumns_) return columnUpper_[iSequence];
          else
               return rowUpper_[iSequence-numberColumns_];
     }
     /// Theta (pivot change)
     inline double theta() const {
          return theta_;
     }
     /** Best possible improvement using djs (primal) or
         obj change by flipping bounds to make dual feasible (dual) */
     inline double bestPossibleImprovement() const {
          return bestPossibleImprovement_;
     }
     /// Return pointer to details of costs
     inline ClpNonLinearCost * nonLinearCost() const {
          return nonLinearCost_;
     }
     /** Return more special options
         1 bit - if presolve says infeasible in ClpSolve return
         2 bit - if presolved problem infeasible return
         4 bit - keep arrays like upper_ around
         8 bit - if factorization kept can still declare optimal at once
         16 bit - if checking replaceColumn accuracy before updating
         32 bit - say optimal if primal feasible!
         64 bit - give up easily in dual (and say infeasible)
         128 bit - no objective, 0-1 and in B&B
         256 bit - in primal from dual or vice versa
         512 bit - alternative use of solveType_
         1024 bit - don't do row copy of factorization
	 2048 bit - perturb in complete fathoming
	 4096 bit - try more for complete fathoming
	 8192 bit - don't even think of using primal if user asks for dual (and vv)
	 16384 bit - in initialSolve so be more flexible
	 debug
	 32768 bit - do dual in netlibd
     */
     inline int moreSpecialOptions() const {
          return moreSpecialOptions_;
     }
     /** Set more special options
         1 bit - if presolve says infeasible in ClpSolve return
         2 bit - if presolved problem infeasible return
         4 bit - keep arrays like upper_ around
         8 bit - no free or superBasic variables
         16 bit - if checking replaceColumn accuracy before updating
         32 bit - say optimal if primal feasible!
         64 bit - give up easily in dual (and say infeasible)
         128 bit - no objective, 0-1 and in B&B
         256 bit - in primal from dual or vice versa
         512 bit - alternative use of solveType_
         1024 bit - don't do row copy of factorization
	 2048 bit - perturb in complete fathoming
	 4096 bit - try more for complete fathoming
	 8192 bit - don't even think of using primal if user asks for dual (and vv)
	 16384 bit - in initialSolve so be more flexible
	 32768 bit - don't swap algorithms from dual if small infeasibility
	 65536 bit - perturb in postsolve cleanup (even if < 10000 rows)
	 1048576 bit - don't perturb even if long time
     */
     inline void setMoreSpecialOptions(int value) {
          moreSpecialOptions_ = value;
     }
     //@}
     /**@name status methods */
     //@{
     inline void setFakeBound(int sequence, FakeBound fakeBound) {
          unsigned char & st_byte = status_[sequence];
          st_byte = static_cast<unsigned char>(st_byte & ~24);
          st_byte = static_cast<unsigned char>(st_byte | (fakeBound << 3));
     }
     inline FakeBound getFakeBound(int sequence) const {
          return static_cast<FakeBound> ((status_[sequence] >> 3) & 3);
     }
     inline void setRowStatus(int sequence, Status newstatus) {
          unsigned char & st_byte = status_[sequence+numberColumns_];
          st_byte = static_cast<unsigned char>(st_byte & ~7);
          st_byte = static_cast<unsigned char>(st_byte | newstatus);
     }
     inline Status getRowStatus(int sequence) const {
          return static_cast<Status> (status_[sequence+numberColumns_] & 7);
     }
     inline void setColumnStatus(int sequence, Status newstatus) {
          unsigned char & st_byte = status_[sequence];
          st_byte = static_cast<unsigned char>(st_byte & ~7);
          st_byte = static_cast<unsigned char>(st_byte | newstatus);
     }
     inline Status getColumnStatus(int sequence) const {
          return static_cast<Status> (status_[sequence] & 7);
     }
     inline void setPivoted( int sequence) {
          status_[sequence] = static_cast<unsigned char>(status_[sequence] | 32);
     }
     inline void clearPivoted( int sequence) {
          status_[sequence] = static_cast<unsigned char>(status_[sequence] & ~32);
     }
     inline bool pivoted(int sequence) const {
          return (((status_[sequence] >> 5) & 1) != 0);
     }
     /// To flag a variable (not inline to allow for column generation)
     void setFlagged( int sequence);
     inline void clearFlagged( int sequence) {
          status_[sequence] = static_cast<unsigned char>(status_[sequence] & ~64);
     }
     inline bool flagged(int sequence) const {
          return ((status_[sequence] & 64) != 0);
     }
     /// To say row active in primal pivot row choice
     inline void setActive( int iRow) {
          status_[iRow] = static_cast<unsigned char>(status_[iRow] | 128);
     }
     inline void clearActive( int iRow) {
          status_[iRow] = static_cast<unsigned char>(status_[iRow] & ~128);
     }
     inline bool active(int iRow) const {
          return ((status_[iRow] & 128) != 0);
     }
     /** Set up status array (can be used by OsiClp).
         Also can be used to set up all slack basis */
     void createStatus() ;
     /** Sets up all slack basis and resets solution to
         as it was after initial load or readMps */
     void allSlackBasis(bool resetSolution = false);

     /// So we know when to be cautious
     inline int lastBadIteration() const {
          return lastBadIteration_;
     }
     /// Set so we know when to be cautious
     inline void setLastBadIteration(int value) {
          lastBadIteration_=value;
     }
     /// Progress flag - at present 0 bit says artificials out
     inline int progressFlag() const {
          return (progressFlag_ & 3);
     }
     /// For dealing with all issues of cycling etc
     inline ClpSimplexProgress * progress()
     { return &progress_;}
     /// Force re-factorization early value
     inline int forceFactorization() const {
          return forceFactorization_ ;
     }
     /// Force re-factorization early
     inline void forceFactorization(int value) {
          forceFactorization_ = value;
     }
     /// Raw objective value (so always minimize in primal)
     inline double rawObjectiveValue() const {
          return objectiveValue_;
     }
     /// Compute objective value from solution and put in objectiveValue_
     void computeObjectiveValue(bool useWorkingSolution = false);
     /// Compute minimization objective value from internal solution without perturbation
     double computeInternalObjectiveValue();
     /** Infeasibility/unbounded ray (NULL returned if none/wrong)
         Up to user to use delete [] on these arrays.  */
     double * infeasibilityRay(bool fullRay=false) const;
     /** Number of extra rows.  These are ones which will be dynamically created
         each iteration.  This is for GUB but may have other uses.
     */
     inline int numberExtraRows() const {
          return numberExtraRows_;
     }
     /** Maximum number of basic variables - can be more than number of rows if GUB
     */
     inline int maximumBasic() const {
          return maximumBasic_;
     }
     /// Iteration when we entered dual or primal
     inline int baseIteration() const {
          return baseIteration_;
     }
     /// Create C++ lines to get to current state
     void generateCpp( FILE * fp, bool defaultFactor = false);
     /// Gets clean and emptyish factorization
     ClpFactorization * getEmptyFactorization();
     /// May delete or may make clean and emptyish factorization
     void setEmptyFactorization();
     /// Move status and solution across
     void moveInfo(const ClpSimplex & rhs, bool justStatus = false);
     //@}

     ///@name Basis handling
     // These are only to be used using startFinishOptions (ClpSimplexDual, ClpSimplexPrimal)
     // *** At present only without scaling
     // *** Slacks havve -1.0 element (so == row activity) - take care
     ///Get a row of the tableau (slack part in slack if not NULL)
     void getBInvARow(int row, double* z, double * slack = NULL);

     ///Get a row of the basis inverse
     void getBInvRow(int row, double* z);

     ///Get a column of the tableau
     void getBInvACol(int col, double* vec);

     ///Get a column of the basis inverse
     void getBInvCol(int col, double* vec);

     /** Get basic indices (order of indices corresponds to the
         order of elements in a vector retured by getBInvACol() and
         getBInvCol()).
     */
     void getBasics(int* index);

     //@}
     //-------------------------------------------------------------------------
     /**@name Changing bounds on variables and constraints */
     //@{
     /** Set an objective function coefficient */
     void setObjectiveCoefficient( int elementIndex, double elementValue );
     /** Set an objective function coefficient */
     inline void setObjCoeff( int elementIndex, double elementValue ) {
          setObjectiveCoefficient( elementIndex, elementValue);
     }

     /** Set a single column lower bound<br>
         Use -DBL_MAX for -infinity. */
     void setColumnLower( int elementIndex, double elementValue );

     /** Set a single column upper bound<br>
         Use DBL_MAX for infinity. */
     void setColumnUpper( int elementIndex, double elementValue );

     /** Set a single column lower and upper bound */
     void setColumnBounds( int elementIndex,
                           double lower, double upper );

     /** Set the bounds on a number of columns simultaneously<br>
         The default implementation just invokes setColLower() and
         setColUpper() over and over again.
         @param indexFirst,indexLast pointers to the beginning and after the
            end of the array of the indices of the variables whose
        <em>either</em> bound changes
         @param boundList the new lower/upper bound pairs for the variables
     */
     void setColumnSetBounds(const int* indexFirst,
                             const int* indexLast,
                             const double* boundList);

     /** Set a single column lower bound<br>
         Use -DBL_MAX for -infinity. */
     inline void setColLower( int elementIndex, double elementValue ) {
          setColumnLower(elementIndex, elementValue);
     }
     /** Set a single column upper bound<br>
         Use DBL_MAX for infinity. */
     inline void setColUpper( int elementIndex, double elementValue ) {
          setColumnUpper(elementIndex, elementValue);
     }

     /** Set a single column lower and upper bound */
     inline void setColBounds( int elementIndex,
                               double newlower, double newupper ) {
          setColumnBounds(elementIndex, newlower, newupper);
     }

     /** Set the bounds on a number of columns simultaneously<br>
         @param indexFirst,indexLast pointers to the beginning and after the
            end of the array of the indices of the variables whose
        <em>either</em> bound changes
         @param boundList the new lower/upper bound pairs for the variables
     */
     inline void setColSetBounds(const int* indexFirst,
                                 const int* indexLast,
                                 const double* boundList) {
          setColumnSetBounds(indexFirst, indexLast, boundList);
     }

     /** Set a single row lower bound<br>
         Use -DBL_MAX for -infinity. */
     void setRowLower( int elementIndex, double elementValue );

     /** Set a single row upper bound<br>
         Use DBL_MAX for infinity. */
     void setRowUpper( int elementIndex, double elementValue ) ;

     /** Set a single row lower and upper bound */
     void setRowBounds( int elementIndex,
                        double lower, double upper ) ;

     /** Set the bounds on a number of rows simultaneously<br>
         @param indexFirst,indexLast pointers to the beginning and after the
            end of the array of the indices of the constraints whose
        <em>either</em> bound changes
         @param boundList the new lower/upper bound pairs for the constraints
     */
     void setRowSetBounds(const int* indexFirst,
                          const int* indexLast,
                          const double* boundList);
     /// Resizes rim part of model
     void resize (int newNumberRows, int newNumberColumns);

     //@}

////////////////// data //////////////////
protected:

     /**@name data.  Many arrays have a row part and a column part.
      There is a single array with both - columns then rows and
      then normally two arrays pointing to rows and columns.  The
      single array is the owner of memory
     */
     //@{
     /** Best possible improvement using djs (primal) or
         obj change by flipping bounds to make dual feasible (dual) */
     double bestPossibleImprovement_;
     /// Zero tolerance
     double zeroTolerance_;
     /// Sequence of worst (-1 if feasible)
     int columnPrimalSequence_;
     /// Sequence of worst (-1 if feasible)
     int rowPrimalSequence_;
     /// "Best" objective value
     double bestObjectiveValue_;
     /// More special options - see set for details
     int moreSpecialOptions_;
     /// Iteration when we entered dual or primal
     int baseIteration_;
     /// Primal tolerance needed to make dual feasible (<largeTolerance)
     double primalToleranceToGetOptimal_;
     /// Large bound value (for complementarity etc)
     double largeValue_;
     /// Largest error on Ax-b
     double largestPrimalError_;
     /// Largest error on basic duals
     double largestDualError_;
     /// For computing whether to re-factorize
     double alphaAccuracy_;
     /// Dual bound
     double dualBound_;
     /// Alpha (pivot element)
     double alpha_;
     /// Theta (pivot change)
     double theta_;
     /// Lower Bound on In variable
     double lowerIn_;
     /// Value of In variable
     double valueIn_;
     /// Upper Bound on In variable
     double upperIn_;
     /// Reduced cost of In variable
     double dualIn_;
     /// Lower Bound on Out variable
     double lowerOut_;
     /// Value of Out variable
     double valueOut_;
     /// Upper Bound on Out variable
     double upperOut_;
     /// Infeasibility (dual) or ? (primal) of Out variable
     double dualOut_;
     /// Current dual tolerance for algorithm
     double dualTolerance_;
     /// Current primal tolerance for algorithm
     double primalTolerance_;
     /// Sum of dual infeasibilities
     double sumDualInfeasibilities_;
     /// Sum of primal infeasibilities
     double sumPrimalInfeasibilities_;
     /// Weight assigned to being infeasible in primal
     double infeasibilityCost_;
     /// Sum of Dual infeasibilities using tolerance based on error in duals
     double sumOfRelaxedDualInfeasibilities_;
     /// Sum of Primal infeasibilities using tolerance based on error in primals
     double sumOfRelaxedPrimalInfeasibilities_;
     /// Acceptable pivot value just after factorization
     double acceptablePivot_;
     /// Working copy of lower bounds (Owner of arrays below)
     double * lower_;
     /// Row lower bounds - working copy
     double * rowLowerWork_;
     /// Column lower bounds - working copy
     double * columnLowerWork_;
     /// Working copy of upper bounds (Owner of arrays below)
     double * upper_;
     /// Row upper bounds - working copy
     double * rowUpperWork_;
     /// Column upper bounds - working copy
     double * columnUpperWork_;
     /// Working copy of objective (Owner of arrays below)
     double * cost_;
     /// Row objective - working copy
     double * rowObjectiveWork_;
     /// Column objective - working copy
     double * objectiveWork_;
     /// Useful row length arrays
     CoinIndexedVector * rowArray_[6];
     /// Useful column length arrays
     CoinIndexedVector * columnArray_[6];
     /// Sequence of In variable
     int sequenceIn_;
     /// Direction of In, 1 going up, -1 going down, 0 not a clude
     int directionIn_;
     /// Sequence of Out variable
     int sequenceOut_;
     /// Direction of Out, 1 to upper bound, -1 to lower bound, 0 - superbasic
     int directionOut_;
     /// Pivot Row
     int pivotRow_;
     /// Last good iteration (immediately after a re-factorization)
     int lastGoodIteration_;
     /// Working copy of reduced costs (Owner of arrays below)
     double * dj_;
     /// Reduced costs of slacks not same as duals (or - duals)
     double * rowReducedCost_;
     /// Possible scaled reduced costs
     double * reducedCostWork_;
     /// Working copy of primal solution (Owner of arrays below)
     double * solution_;
     /// Row activities - working copy
     double * rowActivityWork_;
     /// Column activities - working copy
     double * columnActivityWork_;
     /// Number of dual infeasibilities
     int numberDualInfeasibilities_;
     /// Number of dual infeasibilities (without free)
     int numberDualInfeasibilitiesWithoutFree_;
     /// Number of primal infeasibilities
     int numberPrimalInfeasibilities_;
     /// How many iterative refinements to do
     int numberRefinements_;
     /// dual row pivot choice
     ClpDualRowPivot * dualRowPivot_;
     /// primal column pivot choice
     ClpPrimalColumnPivot * primalColumnPivot_;
     /// Basic variables pivoting on which rows
     int * pivotVariable_;
     /// factorization
     ClpFactorization * factorization_;
     /// Saved version of solution
     double * savedSolution_;
     /// Number of times code has tentatively thought optimal
     int numberTimesOptimal_;
     /// Disaster handler
     ClpDisasterHandler * disasterArea_;
     /// If change has been made (first attempt at stopping looping)
     int changeMade_;
     /// Algorithm >0 == Primal, <0 == Dual
     int algorithm_;
     /** Now for some reliability aids
         This forces re-factorization early */
     int forceFactorization_;
     /** Perturbation:
         -50 to +50 - perturb by this power of ten (-6 sounds good)
         100 - auto perturb if takes too long (1.0e-6 largest nonzero)
         101 - we are perturbed
         102 - don't try perturbing again
         default is 100
     */
     int perturbation_;
     /// Saved status regions
     unsigned char * saveStatus_;
     /** Very wasteful way of dealing with infeasibilities in primal.
         However it will allow non-linearities and use of dual
         analysis.  If it doesn't work it can easily be replaced.
     */
     ClpNonLinearCost * nonLinearCost_;
     /// So we know when to be cautious
     int lastBadIteration_;
     /// So we know when to open up again
     int lastFlaggedIteration_;
     /// Can be used for count of fake bounds (dual) or fake costs (primal)
     int numberFake_;
     /// Can be used for count of changed costs (dual) or changed bounds (primal)
     int numberChanged_;
     /// Progress flag - at present 0 bit says artificials out, 1 free in
     int progressFlag_;
     /// First free/super-basic variable (-1 if none)
     int firstFree_;
     /** Number of extra rows.  These are ones which will be dynamically created
         each iteration.  This is for GUB but may have other uses.
     */
     int numberExtraRows_;
     /** Maximum number of basic variables - can be more than number of rows if GUB
     */
     int maximumBasic_;
     /// If may skip final factorize then allow up to this pivots (default 20)
     int dontFactorizePivots_;
     /** For advanced use.  When doing iterative solves things can get
         nasty so on values pass if incoming solution has largest
         infeasibility < incomingInfeasibility throw out variables
         from basis until largest infeasibility < allowedInfeasibility.
         if allowedInfeasibility>= incomingInfeasibility this is
         always possible altough you may end up with an all slack basis.

         Defaults are 1.0,10.0
     */
     double incomingInfeasibility_;
     double allowedInfeasibility_;
     /// Automatic scaling of objective and rhs and bounds
     int automaticScale_;
     /// Maximum perturbation array size (take out when code rewritten)
     int maximumPerturbationSize_;
     /// Perturbation array (maximumPerturbationSize_)
     double * perturbationArray_;
     /// A copy of model with certain state - normally without cuts
     ClpSimplex * baseModel_;
     /// For dealing with all issues of cycling etc
     ClpSimplexProgress progress_;
#ifdef ABC_INHERIT
  AbcSimplex * abcSimplex_;
#define CLP_ABC_WANTED 1
#define CLP_ABC_WANTED_PARALLEL 2
#define CLP_ABC_FULL_DONE 8
  // bits 256,512,1024 for crash
#endif
#define CLP_ABC_BEEN_FEASIBLE 65536
  int abcState_;
public:
     /// Spare int array for passing information [0]!=0 switches on
     mutable int spareIntArray_[4];
     /// Spare double array for passing information [0]!=0 switches on
     mutable double spareDoubleArray_[4];
protected:
     /// Allow OsiClp certain perks
     friend class OsiClpSolverInterface;
     //@}
};
//#############################################################################
/** A function that tests the methods in the ClpSimplex class. The
    only reason for it not to be a member method is that this way it doesn't
    have to be compiled into the library. And that's a gain, because the
    library should be compiled with optimization on, but this method should be
    compiled with debugging.

    It also does some testing of ClpFactorization class
 */
void
ClpSimplexUnitTest(const std::string & mpsDir);

// For Devex stuff
#define DEVEX_TRY_NORM 1.0e-4
#define DEVEX_ADD_ONE 1.0
#endif
