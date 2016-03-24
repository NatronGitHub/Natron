/* $Id$ */
// Copyright (C) 2002, International Business Machines
// Corporation and others, Copyright (C) 2012, FasterCoin.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).
/*
  Authors
  
  John Forrest
  
*/
#ifndef AbcSimplex_H
#define AbcSimplex_H

#include <iostream>
#include <cfloat>
#include "ClpModel.hpp"
#include "ClpMatrixBase.hpp"
#include "CoinIndexedVector.hpp"
#include "AbcCommon.hpp"
class AbcSimplex;
#include "ClpSolve.hpp"
#include "CoinAbcCommon.hpp"
class ClpSimplex;
class AbcDualRowPivot;
class AbcPrimalColumnPivot;
class AbcSimplexFactorization;
class AbcNonLinearCost;
class OsiAbcSolverInterface;
class CoinWarmStartBasis;
class ClpDisasterHandler;
class AbcSimplexProgress;
class AbcMatrix;
class AbcTolerancesEtc;

/** This solves LPs using the simplex method
    
    It inherits from ClpModel and all its arrays are created at
    algorithm time. Originally I tried to work with model arrays
    but for simplicity of coding I changed to single arrays with
    structural variables then row variables.  Some coding is still
    based on old style and needs cleaning up.
    
    For a description of algorithms:
    
    for dual see AbcSimplexDual.hpp and at top of AbcSimplexDual.cpp
    for primal see AbcSimplexPrimal.hpp and at top of AbcSimplexPrimal.cpp
    
    There is an algorithm data member.  + for primal variations
    and - for dual variations
    
*/
#define PAN
#if ABC_NORMAL_DEBUG>0
#define PRINT_PAN 1
#endif
#define TRY_ABC_GUS
#define HEAVY_PERTURBATION 57
#if ABC_PARALLEL==1
// Use pthreads
#include <pthread.h>
#endif
typedef struct {
  double result;
  //const CoinIndexedVector * constVector; // can get rid of
  //CoinIndexedVector * vectors[2]; // can get rid of
  int status;
  int stuff[4];
} CoinAbcThreadInfo;
#include "ClpSimplex.hpp"
class AbcSimplex : public ClpSimplex {
  friend void AbcSimplexUnitTest(const std::string & mpsDir);
  
public:
  /** enums for status of various sorts.
      ClpModel order (and warmstart) is
      isFree = 0x00,
      basic = 0x01,
      atUpperBound = 0x02,
      atLowerBound = 0x03,
      isFixed means fixed at lower bound and out of basis
  */
  enum Status {
    atLowerBound = 0x00, // so we can use bottom two bits to sort and swap signs
    atUpperBound = 0x01,
    isFree = 0x04,
    superBasic = 0x05,
    basic = 0x06,
    isFixed = 0x07
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
  AbcSimplex (bool emptyMessages = false  );
  
  /** Copy constructor. 
  */
  AbcSimplex(const AbcSimplex & rhs);
  /** Copy constructor from model.
  */
  AbcSimplex(const ClpSimplex & rhs);
  /** Subproblem constructor.  A subset of whole model is created from the
      row and column lists given.  The new order is given by list order and
      duplicates are allowed.  Name and integer information can be dropped
      Can optionally modify rhs to take into account variables NOT in list
      in this case duplicates are not allowed (also see getbackSolution)
  */
  AbcSimplex (const ClpSimplex * wholeModel,
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
  AbcSimplex (const AbcSimplex * wholeModel,
	      int numberRows, const int * whichRows,
	      int numberColumns, const int * whichColumns,
	      bool dropNames = true, bool dropIntegers = true,
	      bool fixOthers = false);
  /** This constructor modifies original AbcSimplex and stores
      original stuff in created AbcSimplex.  It is only to be used in
      conjunction with originalModel */
  AbcSimplex (AbcSimplex * wholeModel,
	      int numberColumns, const int * whichColumns);
  /** This copies back stuff from miniModel and then deletes miniModel.
      Only to be used with mini constructor */
  void originalModel(AbcSimplex * miniModel);
  /** This constructor copies from ClpSimplex */
  AbcSimplex (const ClpSimplex * clpSimplex);
  /// Put back solution into ClpSimplex
  void putBackSolution(ClpSimplex * simplex);
  /** Array persistence flag
      If 0 then as now (delete/new)
      1 then only do arrays if bigger needed
      2 as 1 but give a bit extra if bigger needed
  */
  //void setPersistenceFlag(int value);
  /// Save a copy of model with certain state - normally without cuts
  void makeBaseModel();
  /// Switch off base model
  void deleteBaseModel();
  /// See if we have base model
  inline AbcSimplex *  baseModel() const {
    return abcBaseModel_;
  }
  /** Reset to base model (just size and arrays needed)
      If model NULL use internal copy
  */
  void setToBaseModel(AbcSimplex * model = NULL);
  /// Assignment operator. This copies the data
  AbcSimplex & operator=(const AbcSimplex & rhs);
  /// Destructor
  ~AbcSimplex (  );
  //@}
  
  /**@name Functions most useful to user */
  //@{
  /** Dual algorithm - see AbcSimplexDual.hpp for method.
  */
  int dual();
  int doAbcDual();
  /** Primal algorithm - see AbcSimplexPrimal.hpp for method.
  */
  int primal(int ifValuesPass);
  int doAbcPrimal(int ifValuesPass);
  /// Returns a basis (to be deleted by user)
  CoinWarmStartBasis * getBasis() const;
  /// Passes in factorization
  void setFactorization( AbcSimplexFactorization & factorization);
  /// Swaps factorization
  AbcSimplexFactorization * swapFactorization( AbcSimplexFactorization * factorization);
  /// Gets clean and emptyish factorization
  AbcSimplexFactorization * getEmptyFactorization();
  /** Tightens primal bounds to make dual faster.  Unless
      fixed or doTight>10, bounds are slightly looser than they could be.
      This is to make dual go faster and is probably not needed
      with a presolve.  Returns non-zero if problem infeasible.
      
      Fudge for branch and bound - put bounds on columns of factor *
      largest value (at continuous) - should improve stability
      in branch and bound on infeasible branches (0.0 is off)
  */
  int tightenPrimalBounds();
  /// Sets row pivot choice algorithm in dual
  void setDualRowPivotAlgorithm(AbcDualRowPivot & choice);
  /// Sets column pivot choice algorithm in primal
  void setPrimalColumnPivotAlgorithm(AbcPrimalColumnPivot & choice);
  //@}
  /// If user left factorization frequency then compute
  void defaultFactorizationFrequency();
  //@}
  
  /**@name most useful gets and sets */
  //@{
  /// factorization
  inline AbcSimplexFactorization * factorization() const {
    return reinterpret_cast<AbcSimplexFactorization *>(abcFactorization_);
  }
#ifdef EARLY_FACTORIZE
  /// Early factorization
  inline AbcSimplexFactorization * earlyFactorization() const {
    return reinterpret_cast<AbcSimplexFactorization *>(abcEarlyFactorization_);
  }
#endif
  /// Factorization frequency
  int factorizationFrequency() const;
  void setFactorizationFrequency(int value);
  /// Maximum rows
  inline int maximumAbcNumberRows() const
  { return maximumAbcNumberRows_;}
  /// Maximum Total
  inline int maximumNumberTotal() const
  { return maximumNumberTotal_;}
  inline int maximumTotal() const
  { return maximumNumberTotal_;}
  /// Return true if the objective limit test can be relied upon
  bool isObjectiveLimitTestValid() const ;
  /// Number of variables (includes spare rows)
  inline int numberTotal() const
  { return numberTotal_;}
  /// Number of variables without fixed to zero (includes spare rows)
  inline int numberTotalWithoutFixed() const
  { return numberTotalWithoutFixed_;}
  /// Useful arrays (0,1,2,3,4,5,6,7)
  inline CoinPartitionedVector * usefulArray(int index) {
    return & usefulArray_[index];
  }
  inline CoinPartitionedVector * usefulArray(int index) const {
    return const_cast<CoinPartitionedVector *>(&usefulArray_[index]);
  }
  //@}
  
  /******************** End of most useful part **************/
  /**@name Functions less likely to be useful to casual user */
  //@{
  /** Given an existing factorization computes and checks
      primal and dual solutions.  Uses current problem arrays for
      bounds.  Returns feasibility states */
  int getSolution ();
  /// Sets objectiveValue_ from rawObjectiveValue_
  void setClpSimplexObjectiveValue();
  /** Sets dual values pass djs using unscaled duals
      type 1 - values pass
      type 2 - just use as infeasibility weights 
      type 3 - as 2 but crash
  */
  void setupDualValuesPass(const double * fakeDuals,
			   const double * fakePrimals,
			   int type);
  /// Gets objective value with all offsets but as for minimization
  inline double minimizationObjectiveValue() const
  { return objectiveValue_-dblParam_[ClpObjOffset];}
  /// Current dualTolerance (will end up as dualTolerance_)
  inline double currentDualTolerance() const
  { return currentDualTolerance_;}
  inline void setCurrentDualTolerance(double value) {
    currentDualTolerance_ = value;
  }
  /// Return pointer to details of costs
  inline AbcNonLinearCost * abcNonLinearCost() const {
    return abcNonLinearCost_;
  }
  /// Perturbation (fixed) - is just scaled random numbers
  double * perturbationSaved() const
  { return perturbationSaved_;}
  /// Acceptable pivot for this iteration
  inline double acceptablePivot() const
  { return acceptablePivot_;}
  /// Set to 1 if no free or super basic
  inline int ordinaryVariables() const
  { return ordinaryVariables_;}
  /// Number of ordinary (lo/up) in tableau row
  inline int numberOrdinary() const
  { return numberOrdinary_;}
  /// Set number of ordinary (lo/up) in tableau row
  inline void setNumberOrdinary(int number)
  { numberOrdinary_=number;}
  /// Current dualBound (will end up as dualBound_)
  inline double currentDualBound() const
  { return currentDualBound_;}
  /// dual row pivot choice
  inline AbcDualRowPivot * dualRowPivot() const {
    return abcDualRowPivot_;
  }
  /// primal column pivot choice
  inline AbcPrimalColumnPivot * primalColumnPivot() const {
    return abcPrimalColumnPivot_;
  }
  /// Abc Matrix
  inline AbcMatrix * abcMatrix() const     {
    return abcMatrix_;
  }
  /** Factorizes using current basis.
      solveType - 1 iterating, 0 initial, -1 external
      If 10 added then in primal values pass
      Return codes are as from AbcSimplexFactorization unless initial factorization
      when total number of singularities is returned.
      Special case is numberRows_+1 -> all slack basis.
      if initial should be before permute in
      pivotVariable may be same as toExternal
  */
  int internalFactorize(int solveType);
  /**
     Permutes in from ClpModel data - assumes scale factors done
     and AbcMatrix exists but is in original order (including slacks)
     For now just add basicArray at end
     ==
     But could partition into
     normal (i.e. reasonable lower/upper)
     abnormal - free, odd bounds
     fixed
     ==
     sets a valid pivotVariable
     Slacks always shifted by offset
     Fixed variables always shifted by offset
     Recode to allow row objective so can use pi from idiot etc
  */
  void permuteIn();
  /// deals with new basis and puts in abcPivotVariable_
  void permuteBasis();
  /// Permutes out - bit settings same as stateOfProblem
  void permuteOut(int whatsWanted);
  /// Save data
  ClpDataSave saveData() ;
  /// Restore data
  void restoreData(ClpDataSave saved);
  /// Clean up status
  void cleanStatus();
  /** Computes duals from scratch. If givenDjs then
      allows for nonzero basic djs.  Returns number of refinements  */
  int computeDuals(double * givenDjs, CoinIndexedVector * array1, CoinIndexedVector * array2);
  /// Computes primals from scratch.  Returns number of refinements
  int computePrimals (CoinIndexedVector * array1, CoinIndexedVector * array2);
  /// Computes nonbasic cost and total cost
  void computeObjective ();
  /// set multiple sequence in
  void setMultipleSequenceIn(int sequenceIn[4]);
  /**
     Unpacks one column of the matrix into indexed array
     Uses sequenceIn_
  */
  inline void unpack(CoinIndexedVector & rowArray) const 
  {unpack(rowArray,sequenceIn_);}
  /**
     Unpacks one column of the matrix into indexed array
  */
  void unpack(CoinIndexedVector & rowArray, int sequence) const;
  /**
     This does basis housekeeping and does values for in/out variables.
     Can also decide to re-factorize
  */
  int housekeeping(/*double objectiveChange*/);
  /** This sets largest infeasibility and most infeasible and sum
      and number of infeasibilities (Primal) */
  void checkPrimalSolution(bool justBasic);
  /** This sets largest infeasibility and most infeasible and sum
      and number of infeasibilities (Dual) */
  void checkDualSolution();
  /** This sets largest infeasibility and most infeasible and sum
      and number of infeasibilities AND sumFakeInfeasibilites_ (Dual) */
  void checkDualSolutionPlusFake();
  /** This sets sum and number of infeasibilities (Dual and Primal) */
  void checkBothSolutions();
  /// Computes solutions - 1 do duals, 2 do primals, 3 both (returns number of refinements)
  int gutsOfSolution(int type);
  /// Computes solutions - 1 do duals, 2 do primals, 3 both (returns number of refinements)
  int gutsOfPrimalSolution(int type);
  /// Saves good status etc 
  void saveGoodStatus();
  /// Restores previous good status and says trouble
  void restoreGoodStatus(int type);
#define rowUseScale_ scaleFromExternal_
#define inverseRowUseScale_ scaleToExternal_
  /// After modifying first copy refreshes second copy and marks as updated
  void refreshCosts();
  void refreshLower(unsigned int type=~(ROW_LOWER_SAME|COLUMN_UPPER_SAME));
  void refreshUpper(unsigned int type=~(ROW_LOWER_SAME|COLUMN_LOWER_SAME));
  /// Sets up all extra pointers
  void setupPointers(int maxRows,int maxColumns);
  /// Copies all saved versions to working versions and may do something for perturbation
  void copyFromSaved(int type=31);
  /// fills in perturbationSaved_ from start with 0.5+random
  void fillPerturbation(int start, int number);
  /// For debug - prints summary of arrays which are out of kilter
  void checkArrays(int ignoreEmpty=0) const;
  /// For debug - summarizes dj situation (1 recomputes duals first, 2 checks duals as well)
  void checkDjs(int type=1) const;
  /// For debug - checks solutionBasic
  void checkSolutionBasic() const;
  /// For debug - moves solution back to external and computes stuff (always checks djs)
  void checkMoveBack(bool checkDuals);
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
  /// Move status and solution to ClpSimplex
  void moveStatusToClp(ClpSimplex * clpModel);
  /// Move status and solution from ClpSimplex
  void moveStatusFromClp(ClpSimplex * clpModel);
  //@}
  /**@name most useful gets and sets */
  //@{
public:

  /// Objective value
  inline double clpObjectiveValue() const {
  return (objectiveValue_+objectiveOffset_-bestPossibleImprovement_)*optimizationDirection_ - dblParam_[ClpObjOffset];
  }
  /** Basic variables pivoting on which rows
      may be same as toExternal but may be as at invert */
  inline int * pivotVariable() const {
    return abcPivotVariable_;
  }
  /// State of problem
  inline int stateOfProblem() const
  { return stateOfProblem_;}
  /// State of problem
  inline void setStateOfProblem(int value)
  { stateOfProblem_=value;}
  /// Points from external to internal
  //inline int * fromExternal() const
  //{ return fromExternal_;}
  /// Points from internal to external
  //inline int * toExternal() const
  //{return toExternal_;}
  /** Scale from primal external to internal (in external order) Or other way for dual
   */
  inline double * scaleFromExternal() const
  {return scaleFromExternal_;}
  /** Scale from primal internal to external (in external order) Or other way for dual
   */
  inline double * scaleToExternal() const
  {return scaleToExternal_;}
  /// corresponds to rowScale etc
  inline double * rowScale2() const
  {return rowUseScale_;}
  inline double * inverseRowScale2() const
  {return inverseRowUseScale_;}
  inline double * inverseColumnScale2() const
  {return inverseColumnUseScale_;}
  inline double * columnScale2() const
  {return columnUseScale_;}
  inline int arrayForDualColumn() const
  {return arrayForDualColumn_;}
  /// upper theta from dual column
  inline double upperTheta() const
  {return upperTheta_;}
  inline int arrayForReplaceColumn() const
  { return arrayForReplaceColumn_;}
  inline int arrayForFlipBounds() const
  { return arrayForFlipBounds_;}
  inline int arrayForFlipRhs() const
  { return arrayForFlipRhs_;}
  inline int arrayForBtran() const
  { return arrayForBtran_;}
  inline int arrayForFtran() const
  { return arrayForFtran_;}
  inline int arrayForTableauRow() const
  { return arrayForTableauRow_;}
  /// value of incoming variable (in Dual)
  double valueIncomingDual() const;
  /// Get pointer to array[getNumCols()] of primal solution vector
  const double * getColSolution() const; 
  
  /// Get pointer to array[getNumRows()] of dual prices
  const double * getRowPrice() const;
  
  /// Get a pointer to array[getNumCols()] of reduced costs
  const double * getReducedCost() const; 
  
  /** Get pointer to array[getNumRows()] of row activity levels (constraint
      matrix times the solution vector */
  const double * getRowActivity() const; 
  //@}
  
  /**@name protected methods */
  //@{
  /** May change basis and then returns number changed.
      Computation of solutions may be overriden by given pi and solution
  */
  int gutsOfSolution ( double * givenDuals,
		       const double * givenPrimals,
		       bool valuesPass = false);
  /// Does most of deletion for arrays etc(0 just null arrays, 1 delete first)
  void gutsOfDelete(int type);
  /// Does most of copying
  void gutsOfCopy(const AbcSimplex & rhs);
  /// Initializes arrays
  void gutsOfInitialize(int numberRows,int numberColumns,bool doMore);
  /// resizes arrays
  void gutsOfResize(int numberRows,int numberColumns);
  /** Translates ClpModel to AbcSimplex
      See DO_ bits in stateOfProblem_ for type e.g. DO_BASIS_AND_ORDER
  */
  void translate(int type);
  /// Moves basic stuff to basic area
  void moveToBasic(int which=15);
  //@}
public:
  /**@name public methods */
  //@{
  /// Return region
  inline double * solutionRegion() const {
    return abcSolution_;
  }
  inline double * djRegion() const {
    return abcDj_;
  }
  inline double * lowerRegion() const {
    return abcLower_;
  }
  inline double * upperRegion() const {
    return abcUpper_;
  }
  inline double * costRegion() const {
    return abcCost_;
  }
  /// Return region
  inline double * solutionRegion(int which) const {
    return abcSolution_+which*maximumAbcNumberRows_;
  }
  inline double * djRegion(int which) const {
    return abcDj_+which*maximumAbcNumberRows_;
  }
  inline double * lowerRegion(int which) const {
    return abcLower_+which*maximumAbcNumberRows_;
  }
  inline double * upperRegion(int which) const {
    return abcUpper_+which*maximumAbcNumberRows_;
  }
  inline double * costRegion(int which) const {
    return abcCost_+which*maximumAbcNumberRows_;
  }
  /// Return region
  inline double * solutionBasic() const {
    return solutionBasic_;
  }
  inline double * djBasic() const {
    return djBasic_;
  }
  inline double * lowerBasic() const {
    return lowerBasic_;
  }
  inline double * upperBasic() const {
    return upperBasic_;
  }
  inline double * costBasic() const {
    return costBasic_;
  }
  /// Perturbation
  inline double * abcPerturbation() const
  { return abcPerturbation_;}
  /// Fake djs
  inline double * fakeDjs() const
  { return djSaved_;}
  inline unsigned char * internalStatus() const 
  { return internalStatus_;}
  inline AbcSimplex::Status getInternalStatus(int sequence) const {
    return static_cast<Status> (internalStatus_[sequence] & 7);
  }
  inline AbcSimplex::Status getInternalColumnStatus(int sequence) const {
    return static_cast<Status> (internalStatus_[sequence+maximumAbcNumberRows_] & 7);
  }
  inline void setInternalStatus(int sequence, AbcSimplex::Status newstatus) {
    unsigned char & st_byte = internalStatus_[sequence];
    st_byte = static_cast<unsigned char>(st_byte & ~7);
    st_byte = static_cast<unsigned char>(st_byte | newstatus);
  }
  inline void setInternalColumnStatus(int sequence, AbcSimplex::Status newstatus) {
    unsigned char & st_byte = internalStatus_[sequence+maximumAbcNumberRows_];
    st_byte = static_cast<unsigned char>(st_byte & ~7);
    st_byte = static_cast<unsigned char>(st_byte | newstatus);
  }
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
#if 0
  /** Return sequenceInternal In or Out */
  inline int sequenceInternalIn() const {
    return sequenceInternalIn_;
  }
  inline int sequenceInternalOut() const {
    return sequenceInternalOut_;
  }
  /** Set sequenceInternalIn or Out */
  inline void  setSequenceInternalIn(int sequence) {
    sequenceInternalIn_ = sequence;
  }
  inline void  setSequenceInternalOut(int sequence) {
    sequenceInternalOut_ = sequence;
  }
#endif
  /// Returns 1 if sequence indicates column
  inline int isColumn(int sequence) const {
    return sequence >= maximumAbcNumberRows_ ? 1 : 0;
  }
  /// Returns sequence number within section
  inline int sequenceWithin(int sequence) const {
    return sequence < maximumAbcNumberRows_ ? sequence : sequence - maximumAbcNumberRows_;
  }
  /// Current/last pivot row (set after END of choosing pivot row in dual)
  inline int lastPivotRow() const
  { return lastPivotRow_;}
  /// First Free_
  inline int firstFree() const
  { return firstFree_;}
  /// Last firstFree_
  inline int lastFirstFree() const
  { return lastFirstFree_;}
  /// Free chosen vector
  inline int freeSequenceIn() const
  { return freeSequenceIn_;}
  /// Acceptable pivot for this iteration
  inline double currentAcceptablePivot() const
  { return currentAcceptablePivot_;}
#ifdef PAN
  /** Returns 
      1 if fake superbasic
      0 if free or true superbasic
      -1 if was fake but has cleaned itself up (sets status)
      -2 if wasn't fake
   */
  inline int fakeSuperBasic(int iSequence) {
    if ((internalStatus_[iSequence]&7)==4)
      return 0; // free
    if ((internalStatus_[iSequence]&7)!=5)
      return -2;
    double value=abcSolution_[iSequence];
    if (value<abcLower_[iSequence]+primalTolerance_) {
      if(abcDj_[iSequence]>=-currentDualTolerance_) {
	setInternalStatus(iSequence,atLowerBound);
#if PRINT_PAN>1 
	printf("Pansetting %d to lb\n",iSequence);
#endif
	return -1;
      } else {
	return 1;
      }
    } else if (value>abcUpper_[iSequence]-primalTolerance_) {
      if (abcDj_[iSequence]<=currentDualTolerance_) {
	setInternalStatus(iSequence,atUpperBound);
#if PRINT_PAN>1 
	printf("Pansetting %d to ub\n",iSequence);
#endif
	return -1;
      } else {
	return 1;
      }
    } else {
      return 0;
    }
  }
#endif
  /// Return row or column values
  inline double solution(int sequence) {
    return abcSolution_[sequence];
  }
  /// Return address of row or column values
  inline double & solutionAddress(int sequence) {
    return abcSolution_[sequence];
  }
  inline double reducedCost(int sequence) {
    return abcDj_[sequence];
  }
  inline double & reducedCostAddress(int sequence) {
    return abcDj_[sequence];
  }
  inline double lower(int sequence) {
    return abcLower_[sequence];
  }
  /// Return address of row or column lower bound
  inline double & lowerAddress(int sequence) {
    return abcLower_[sequence];
  }
  inline double upper(int sequence) {
    return abcUpper_[sequence];
  }
  /// Return address of row or column upper bound
  inline double & upperAddress(int sequence) {
    return abcUpper_[sequence];
  }
  inline double cost(int sequence) {
    return abcCost_[sequence];
  }
  /// Return address of row or column cost
  inline double & costAddress(int sequence) {
    return abcCost_[sequence];
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
  /// For dealing with all issues of cycling etc
  inline AbcSimplexProgress * abcProgress()
  { return &abcProgress_;}
public:
  /** Clears an array and says available (-1 does all)
      when no possibility of going parallel */
  inline void clearArraysPublic(int which)
  { clearArrays(which);}
  /** Returns first available empty array (and sets flag)
      when no possibility of going parallel */
  inline int getAvailableArrayPublic() const
  { return getAvailableArray();}
#if ABC_PARALLEL
  /// get parallel mode
  inline int parallelMode() const
  { return parallelMode_;}
  /// set parallel mode
  inline void setParallelMode(int value)
  { parallelMode_=value;}
  /// Number of cpus
  inline int numberCpus() const
  { return parallelMode_+1;}
#if ABC_PARALLEL==1
  /// set stop start
  inline void setStopStart(int value)
  { stopStart_=value;}
#endif
#endif
  //protected:
  /// Clears an array and says available (-1 does all)
  void clearArrays(int which);
  /// Clears an array and says available
  void clearArrays(CoinPartitionedVector * which);
  /// Returns first available empty array (and sets flag)
  int getAvailableArray() const;
  /// Say array going to be used
  inline void setUsedArray(int which) const 
  {int check=1<<which;assert ((stateOfProblem_&check)==0);stateOfProblem_|=check;}
  /// Say array going available
  inline void setAvailableArray(int which) const
  {int check=1<<which;assert ((stateOfProblem_&check)!=0);
    assert (!usefulArray_[which].getNumElements());stateOfProblem_&=~check;}
  /// Swaps primal stuff 
  void swapPrimalStuff();
  /// Swaps dual stuff
  void swapDualStuff(int lastSequenceOut,int lastDirectionOut);
protected:
  //@}
  /**@name status methods */
  //@{
  /// Swaps two variables and does status
  void swap(int pivotRow,int nonBasicPosition,Status newStatus);
  inline void setFakeBound(int sequence, FakeBound fakeBound) {
    unsigned char & st_byte = internalStatus_[sequence];
    st_byte = static_cast<unsigned char>(st_byte & ~24);
    st_byte = static_cast<unsigned char>(st_byte | (fakeBound << 3));
  }
  inline FakeBound getFakeBound(int sequence) const {
    return static_cast<FakeBound> ((internalStatus_[sequence] >> 3) & 3);
  }
  bool atFakeBound(int sequence) const;
  inline void setPivoted( int sequence) {
    internalStatus_[sequence] = static_cast<unsigned char>(internalStatus_[sequence] | 32);
  }
  inline void clearPivoted( int sequence) {
    internalStatus_[sequence] = static_cast<unsigned char>(internalStatus_[sequence] & ~32);
  }
  inline bool pivoted(int sequence) const {
    return (((internalStatus_[sequence] >> 5) & 1) != 0);
  }
public:
  /// Swaps two variables
  void swap(int pivotRow,int nonBasicPosition);
  /// To flag a variable
  void setFlagged( int sequence);
  inline void clearFlagged( int sequence) {
    internalStatus_[sequence] = static_cast<unsigned char>(internalStatus_[sequence] & ~64);
  }
  inline bool flagged(int sequence) const {
    return ((internalStatus_[sequence] & 64) != 0);
  }
protected:
  /// To say row active in primal pivot row choice
  inline void setActive( int iRow) {
    internalStatus_[iRow] = static_cast<unsigned char>(internalStatus_[iRow] | 128);
  }
  inline void clearActive( int iRow) {
    internalStatus_[iRow] = static_cast<unsigned char>(internalStatus_[iRow] & ~128);
  }
  inline bool active(int iRow) const {
    return ((internalStatus_[iRow] & 128) != 0);
  }
public:
  /** Set up status array (can be used by OsiAbc).
      Also can be used to set up all slack basis */
  void createStatus() ;
  /// Does sort of crash
  void crash(int type);
  /** Puts more stuff in basis
      1 bit set - do even if basis exists
      2 bit set - don't bother staying triangular
   */
  void putStuffInBasis(int type);
  /** Sets up all slack basis and resets solution to
      as it was after initial load or readMps */
  void allSlackBasis();
  /// For debug - check pivotVariable consistent
  void checkConsistentPivots() const;
  /// Print stuff
  void printStuff() const;
  /// Common bits of coding for dual and primal
  int startup(int ifValuesPass);
  
  /// Raw objective value (so always minimize in primal)
  inline double rawObjectiveValue() const {
    return objectiveValue_;
  }
  /// Compute objective value from solution and put in objectiveValue_
  void computeObjectiveValue(bool useWorkingSolution = false);
  /// Compute minimization objective value from internal solution without perturbation
  double computeInternalObjectiveValue();
  /// Move status and solution across
  void moveInfo(const AbcSimplex & rhs, bool justStatus = false);
#define NUMBER_THREADS 3
#if ABC_PARALLEL==1
  // For waking up thread
  inline pthread_mutex_t * mutexPointer(int which,int thread=0) 
  { return mutex_+which+3*thread;}
  inline pthread_barrier_t * barrierPointer() 
  { return &barrier_;}
  inline int whichLocked(int thread=0) const
  { return locked_[thread];}
  inline CoinAbcThreadInfo * threadInfoPointer(int thread=0) 
  { return threadInfo_+thread;}
  void startParallelStuff(int type);
  int stopParallelStuff(int type);
  /// so thread can find out which one it is 
  int whichThread() const; 
#elif ABC_PARALLEL==2
  //inline CoinAbcThreadInfo * threadInfoPointer(int thread=0) 
  //{ return threadInfo_+thread;}
#endif
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
  /// Sum of nonbasic costs 
  double sumNonBasicCosts_;
  /// Sum of costs (raw objective value) 
  double rawObjectiveValue_;
  /// Objective offset (from offset_)
  double objectiveOffset_;
  /**  Perturbation factor
       If <0.0 then virtual
       if 0.0 none
       if >0.0 use this as factor */
  double perturbationFactor_;
  /// Current dualTolerance (will end up as dualTolerance_)
  double currentDualTolerance_;
  /// Current dualBound (will end up as dualBound_)
  double currentDualBound_;
  /// Largest gap
  double largestGap_;
  /// Last dual bound
  double lastDualBound_;
  /// Sum of infeasibilities when using fake perturbation tolerance
  double sumFakeInfeasibilities_;
  /// Last primal error
  double lastPrimalError_;
  /// Last dual error
  double lastDualError_;
  /// Acceptable pivot for this iteration
  double currentAcceptablePivot_;
  /// Movement of variable
  double movement_;
  /// Objective change
  double objectiveChange_;
  /// Btran alpha
  double btranAlpha_;
  /// FT alpha
#ifdef ABC_LONG_FACTORIZATION 
  long
#endif
  double ftAlpha_;
  /// Minimum theta movement
  double minimumThetaMovement_;
  /// Initial sum of infeasibilities
  double initialSumInfeasibilities_;
public:
  /// Where we are in iteration
  int stateOfIteration_;
protected:
  /// Last firstFree_
  int lastFirstFree_;
  /// Free chosen vector
  int freeSequenceIn_;
  /// Maximum number rows
  int maximumAbcNumberRows_;
  /// Maximum number columns
  int maximumAbcNumberColumns_;
  /// Maximum numberTotal
  int maximumNumberTotal_;
  /// Current number of variables flagged
  int numberFlagged_;
  /// Iteration at which to do relaxed dualColumn
  int normalDualColumnIteration_;
  /** State of dual waffle
      -2 - in initial large tolerance phase
      -1 - in medium tolerance phase
      n - in correct tolerance phase and thought optimal n times
   */
  int stateDualColumn_;
  /*
    May want to put some arrays into struct 
    Two arrays point to/from external
    Order is basic,unused basic, at lower, at upper, superbasic, free, fixed with starts
  */
  /// Number of variables (includes spare rows)
  int numberTotal_;
  /// Number of variables without fixed to zero (includes spare rows)
  int numberTotalWithoutFixed_;
  /// Start of variables at lower bound with no upper
#define startAtLowerNoOther_ maximumAbcNumberRows_
  /// Start of variables at lower bound with upper 
  int startAtLowerOther_;
  /// Start of variables at upper bound with no lower
  int startAtUpperNoOther_;
  /// Start of variables at upper bound with lower
  int startAtUpperOther_;
  /// Start of superBasic, free or awkward bounds variables
  int startOther_;
  /// Start of fixed variables
  int startFixed_;
#ifdef EARLY_FACTORIZE
  /// Number of iterations to try factorizing early
  int numberEarly_;
#endif
  /**
     State of problem
     State of external arrays
     2048 - status OK
     4096 - row primal solution OK
     8192 - row dual solution OK
     16384 - column primal solution OK
     32768 - column dual solution OK
     65536 - Everything not going smoothly (when smooth we forget about tiny bad djs)
     131072 - when increasing rows add a bit
     262144 - scale matrix and create new one
     524288 - do basis and order
     1048576 - just status (and check if order needed)
     2097152 - just solution
     4194304 - just redo bounds (and offset)
     Bottom bits say if usefulArray in use
   */
#define ALL_STATUS_OK 2048
#define ROW_PRIMAL_OK 4096
#define ROW_DUAL_OK 8192
#define COLUMN_PRIMAL_OK 16384
#define COLUMN_DUAL_OK 32768
#define PESSIMISTIC 65536
#define ADD_A_BIT 131072
#define DO_SCALE_AND_MATRIX 262144
#define DO_BASIS_AND_ORDER 524288
#define DO_STATUS 1048576
#define DO_SOLUTION 2097152
#define DO_JUST_BOUNDS 0x400000
#define NEED_BASIS_SORT 0x800000
#define FAKE_SUPERBASIC 0x1000000
#define VALUES_PASS 0x2000000
#define VALUES_PASS2 0x4000000
  mutable int stateOfProblem_;
#if ABC_PARALLEL
public:
  /// parallel mode
  int parallelMode_;
protected:
#endif
  /// Number of ordinary (lo/up) in tableau row
  int numberOrdinary_;
  /// Set to 1 if no free or super basic
  int ordinaryVariables_;
  /// Number of free nonbasic variables
  int numberFreeNonBasic_;
  /// Last time cleaned up
  int lastCleaned_;
  /// Current/last pivot row (set after END of choosing pivot row in dual)
  int lastPivotRow_;
  /// Nonzero (probably 10) if swapped algorithms
  int swappedAlgorithm_;
  /// Initial number of infeasibilities
  int initialNumberInfeasibilities_;
  /// Points from external to internal
  //int * fromExternal_;
  /// Points from internal to external
  //int * toExternal_;
  /** Scale from primal external to internal (in external order) Or other way for dual
   */
  double * scaleFromExternal_;
  /** Scale from primal internal to external (in external order) Or other way for dual
   */
  double * scaleToExternal_;
  /// use this instead of columnScale
  double * columnUseScale_;
  /// use this instead of inverseColumnScale
  double * inverseColumnUseScale_;
  /** Primal offset (in external order)
      So internal value is (external-offset)*scaleFromExternal
   */
  double * offset_;
  /// Offset for accumulated offsets*matrix
  double * offsetRhs_;
  /// Useful array of numberTotal length
  double * tempArray_;
  /** Working status
      ? may be signed
      ? link pi_ to an indexed array?
      may have saved from last factorization at end */
  unsigned char * internalStatus_;
  /// Saved status
  unsigned char * internalStatusSaved_;
  /** Perturbation (fixed) - is just scaled random numbers
      If perturbationFactor_<0 then virtual perturbation */
  double * abcPerturbation_;
  /// saved perturbation
  double * perturbationSaved_;
  /// basic perturbation
  double * perturbationBasic_;
  /// Working matrix
  AbcMatrix * abcMatrix_;
  /** Working scaled copy of lower bounds 
      has original scaled copy at end */
  double * abcLower_;
  /** Working scaled copy of upper bounds 
      has original scaled copy at end */
  double * abcUpper_;
  /** Working scaled copy of objective
      ? where perturbed copy or can we
      always work with perturbed copy (in B&B) if we adjust increments/cutoffs
      ? should we save a fixed perturbation offset array
      has original scaled copy at end */
  double * abcCost_;
  /** Working scaled primal solution
      may have saved from last factorization at end */
  double * abcSolution_;
  /** Working scaled dual solution
      may have saved from last factorization at end */
  double * abcDj_;
  /// Saved scaled copy of  lower bounds 
  double * lowerSaved_;
  /// Saved scaled copy of  upper bounds 
  double * upperSaved_;
  /// Saved scaled copy of  objective
  double * costSaved_;
  /// Saved scaled  primal solution
  double * solutionSaved_;
  /// Saved scaled  dual solution
  double * djSaved_;
  /// Working scaled copy of basic lower bounds 
  double * lowerBasic_;
  /// Working scaled copy of basic upper bounds 
  double * upperBasic_;
  /// Working scaled copy of basic objective
  double * costBasic_;
  /// Working scaled basic primal solution
  double * solutionBasic_;
  /// Working scaled basic dual solution (want it to be zero)
  double * djBasic_;
  /// dual row pivot choice
  AbcDualRowPivot * abcDualRowPivot_;
  /// primal column pivot choice
  AbcPrimalColumnPivot * abcPrimalColumnPivot_;
  /** Basic variables pivoting on which rows
      followed by atLo/atUp then free/superbasic then fixed
  */
  int * abcPivotVariable_;
  /// Reverse abcPivotVariable_ for moving around
  int * reversePivotVariable_;
  /// factorization
  AbcSimplexFactorization * abcFactorization_;
#ifdef EARLY_FACTORIZE
  /// Alternative factorization
  AbcSimplexFactorization * abcEarlyFactorization_;
#endif
#ifdef TEMPORARY_FACTORIZATION
  /// Alternative factorization
  AbcSimplexFactorization * abcOtherFactorization_;
#endif
  /// Saved version of solution
  //double * savedSolution_;
  /// A copy of model with certain state - normally without cuts
  AbcSimplex * abcBaseModel_;
  /// A copy of model as ClpSimplex with certain state
  ClpSimplex * clpModel_;
  /** Very wasteful way of dealing with infeasibilities in primal.
      However it will allow non-linearities and use of dual
      analysis.  If it doesn't work it can easily be replaced.
  */
  AbcNonLinearCost * abcNonLinearCost_;
  /// Useful arrays (all of row+column+2 length)
  /* has secondary offset and counts so row goes first then column
     Probably back to CoinPartitionedVector as AbcMatrix has slacks
     also says if in use - so we can just get next available one */
#define ABC_NUMBER_USEFUL 8
  mutable CoinPartitionedVector usefulArray_[ABC_NUMBER_USEFUL];
  /// For dealing with all issues of cycling etc
  AbcSimplexProgress abcProgress_;
  /// For saving stuff at beginning
  ClpDataSave saveData_;
  /// upper theta from dual column
  double upperTheta_;
  /// Multiple sequence in
  int multipleSequenceIn_[4];
public:
  int arrayForDualColumn_;
  int arrayForReplaceColumn_;
  int arrayForFlipBounds_; //2
  int arrayForFlipRhs_; // if sequential can re-use
  int arrayForBtran_; // 0
  int arrayForFtran_; // 1
  int arrayForTableauRow_; //3
protected:
  int numberFlipped_;
  int numberDisasters_;
  //int nextCleanNonBasicIteration_;
#if ABC_PARALLEL==1
  // For waking up thread
  pthread_mutex_t mutex_[3*NUMBER_THREADS];
  pthread_barrier_t barrier_; 
  CoinAbcThreadInfo threadInfo_[NUMBER_THREADS];
  pthread_t abcThread_[NUMBER_THREADS];
  int locked_[NUMBER_THREADS];
  int stopStart_;
#elif ABC_PARALLEL==2
  //CoinAbcThreadInfo threadInfo_[NUMBER_THREADS];
#endif
  //@}
};
//#############################################################################
/** A function that tests the methods in the AbcSimplex class. The
    only reason for it not to be a member method is that this way it doesn't
    have to be compiled into the library. And that's a gain, because the
    library should be compiled with optimization on, but this method should be
    compiled with debugging.
    
    It also does some testing of AbcSimplexFactorization class
*/
void
AbcSimplexUnitTest(const std::string & mpsDir);
#endif
