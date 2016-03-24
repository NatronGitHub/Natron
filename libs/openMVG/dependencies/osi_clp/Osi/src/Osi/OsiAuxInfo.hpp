// Copyright (C) 2006, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef OsiAuxInfo_H
#define OsiAuxInfo_H

class OsiSolverInterface;

//#############################################################################
/** This class allows for a more structured use of algorithmic tweaking to
    an OsiSolverInterface.  It is designed to replace the simple use of
    appData_ pointer.

    This has been done to make it easier to use NonLinear solvers and other
    exotic beasts in a branch and bound mode.  After this class definition
    there is one for a derived class for just such a purpose.

*/

class OsiAuxInfo {
public:
  // Default Constructor 
  OsiAuxInfo (void * appData = NULL);

  // Copy Constructor 
  OsiAuxInfo (const OsiAuxInfo & rhs);
  // Destructor
  virtual ~OsiAuxInfo();
  
  /// Clone
  virtual OsiAuxInfo * clone() const;
  /// Assignment operator 
  OsiAuxInfo & operator=(const OsiAuxInfo& rhs);
  
  /// Get application data
  inline void * getApplicationData() const
  { return appData_;}
protected:
    /// Pointer to user-defined data structure
    void * appData_;
};
//#############################################################################
/** This class allows for the use of more exotic solvers e.g. Non-Linear or Volume.

    You can derive from this although at present I can't see the need.
*/

class OsiBabSolver : public OsiAuxInfo {
public:
  // Default Constructor 
  OsiBabSolver (int solverType=0);

  // Copy Constructor 
  OsiBabSolver (const OsiBabSolver & rhs);
  // Destructor
  virtual ~OsiBabSolver();
  
  /// Clone
  virtual OsiAuxInfo * clone() const;
  /// Assignment operator 
  OsiBabSolver & operator=(const OsiBabSolver& rhs);
  
  /// Update solver 
  inline void setSolver(const OsiSolverInterface * solver)
  { solver_ = solver;}
  /// Update solver 
  inline void setSolver(const OsiSolverInterface & solver)
  { solver_ = &solver;}

  /** returns 0 if no heuristic solution, 1 if valid solution
      with better objective value than one passed in
      Sets solution values if good, sets objective value 
      numberColumns is size of newSolution
  */
  int solution(double & objectiveValue,
		       double * newSolution, int numberColumns);
  /** Set solution and objective value.
      Number of columns and optimization direction taken from current solver.
      Size of solution is numberColumns (may be padded or truncated in function) */
  void setSolution(const double * solution, int numberColumns, double objectiveValue);

  /** returns true if the object stores a solution, false otherwise. If there
	  is a solution then solutionValue and solution will be filled out as well.
      In that case the user needs to allocate solution to be a big enough
	  array.
  */
  bool hasSolution(double & solutionValue, double * solution);

  /** Sets solver type
      0 - normal LP solver
      1 - DW - may also return heuristic solutions
      2 - NLP solver or similar - can't compute objective value just from solution
          check solver to see if feasible and what objective value is
          - may also return heuristic solution
      3 - NLP solver or similar - can't compute objective value just from solution
          check this (rather than solver) to see if feasible and what objective value is.
          Using Outer Approximation so called lp based
          - may also return heuristic solution
      4 - normal solver but cuts are needed for integral solution    
  */
  inline void setSolverType(int value)
  { solverType_=value;}
  /** gets solver type
      0 - normal LP solver
      1 - DW - may also return heuristic solutions
      2 - NLP solver or similar - can't compute objective value just from solution
          check this (rather than solver) to see if feasible and what objective value is
          - may also return heuristic solution
      3 - NLP solver or similar - can't compute objective value just from solution
          check this (rather than solver) to see if feasible and what objective value is.
          Using Outer Approximation so called lp based
          - may also return heuristic solution
      4 - normal solver but cuts are needed for integral solution    
  */
  inline int solverType() const
  { return solverType_;}
  /** Return true if getting solution may add cuts so hot start etc will
      be obsolete */
  inline bool solutionAddsCuts() const
  { return solverType_==3;}
  /// Return true if we should try cuts at root even if looks satisfied
  inline bool alwaysTryCutsAtRootNode() const
  { return solverType_==4;}
  /** Returns true if can use solver objective or feasible values,
      otherwise use mipBound etc */
  inline bool solverAccurate() const
  { return solverType_==0||solverType_==2||solverType_==4;}
  /// Returns true if can use reduced costs for fixing
  inline bool reducedCostsAccurate() const
  { return solverType_==0||solverType_==4;}
  /// Get objective  (well mip bound)
  double mipBound() const;
  /// Returns true if node feasible
  bool mipFeasible() const;
  /// Set mip bound (only used for some solvers)
  inline void setMipBound(double value)
  { mipBound_ = value;}
  /// Get objective value of saved solution
  inline double bestObjectiveValue() const
  { return bestObjectiveValue_;}
  /// Says whether we want to try cuts at all
  inline bool tryCuts() const
  { return solverType_!=2;}
  /// Says whether we have a warm start (so can do strong branching)
  inline bool warmStart() const
  { return solverType_!=2;}
  /** Get bit mask for odd actions of solvers
      1 - solution or bound arrays may move in mysterious ways e.g. cplex
      2 - solver may want bounds before branch
  */
  inline int extraCharacteristics() const
  { return extraCharacteristics_;}
  /** Set bit mask for odd actions of solvers
      1 - solution or bound arrays may move in mysterious ways e.g. cplex
      2 - solver may want bounds before branch
  */
  inline void setExtraCharacteristics(int value)
  { extraCharacteristics_=value;}
  /// Pointer to lower bounds before branch (only if extraCharacteristics set)
  inline const double * beforeLower() const
  { return beforeLower_;}
  /// Set pointer to lower bounds before branch (only if extraCharacteristics set)
  inline void setBeforeLower(const double * array)
  { beforeLower_ = array;}
  /// Pointer to upper bounds before branch (only if extraCharacteristics set)
  inline const double * beforeUpper() const
  { return beforeUpper_;}
  /// Set pointer to upper bounds before branch (only if extraCharacteristics set)
  inline void setBeforeUpper(const double * array)
  { beforeUpper_ = array;}
protected:
  /// Objective value of best solution (if there is one) (minimization)
  double bestObjectiveValue_;
  /// Current lower bound on solution ( if > 1.0e50 infeasible)
  double mipBound_;
  /// Solver to use for getting/setting solutions etc
  const OsiSolverInterface * solver_;
  /// Best integer feasible solution
  double * bestSolution_;
  /// Pointer to lower bounds before branch (only if extraCharacteristics set)
  const double * beforeLower_;
  /// Pointer to upper bounds before branch (only if extraCharacteristics set)
  const double * beforeUpper_;
  /** Solver type
      0 - normal LP solver
      1 - DW - may also return heuristic solutions
      2 - NLP solver or similar - can't compute objective value just from solution
          check this (rather than solver) to see if feasible and what objective value is
          - may also return heuristic solution
      3 - NLP solver or similar - can't compute objective value just from solution
          check this (rather than solver) to see if feasible and what objective value is.
          Using Outer Approximation so called lp based
          - may also return heuristic solution
  */
  int solverType_;
  /// Size of solution
  int sizeSolution_;
  /** Bit mask for odd actions of solvers
      1 - solution or bound arrays may move in mysterious ways e.g. cplex
      2 - solver may want bounds before branch
  */
  int extraCharacteristics_;
};

#endif
