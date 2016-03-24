// Copyright (C) 2005, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef OsiSolverBranch_H
#define OsiSolverBranch_H

class OsiSolverInterface;
#include "CoinWarmStartBasis.hpp"

//#############################################################################

/** Solver Branch Class

    This provides information on a branch as a set of tighter bounds on both ways
*/

class OsiSolverBranch  {
  
public:
  ///@name Add and Get methods 
  //@{
  /// Add a simple branch (i.e. first sets ub of floor(value), second lb of ceil(value))
  void addBranch(int iColumn, double value); 
  
  /// Add bounds - way =-1 is first , +1 is second 
  void addBranch(int way,int numberTighterLower, const int * whichLower, const double * newLower,
                 int numberTighterUpper, const int * whichUpper, const double * newUpper);
  /// Add bounds - way =-1 is first , +1 is second 
  void addBranch(int way,int numberColumns,const double * oldLower, const double * newLower,
                 const double * oldUpper, const double * newUpper);
  
  /// Apply bounds
  void applyBounds(OsiSolverInterface & solver,int way) const;
  /// Returns true if current solution satsifies one side of branch
  bool feasibleOneWay(const OsiSolverInterface & solver) const;
  /// Starts
  inline const int * starts() const
  { return start_;}
  /// Which variables
  inline const int * which() const
  { return indices_;}
  /// Bounds
  inline const double * bounds() const
  { return bound_;}
  //@}
  
  
  ///@name Constructors and destructors
  //@{
  /// Default Constructor
  OsiSolverBranch(); 
  
  /// Copy constructor 
  OsiSolverBranch(const OsiSolverBranch & rhs);
  
  /// Assignment operator 
  OsiSolverBranch & operator=(const OsiSolverBranch & rhs);
  
  /// Destructor 
  ~OsiSolverBranch ();
  
  //@}
  
private:
  ///@name Private member data 
  //@{
  /// Start of lower first, upper first, lower second, upper second
  int start_[5];
  /// Column numbers (if >= numberColumns treat as rows)
  int * indices_;
  /// New bounds
  double * bound_;
 //@}
};
//#############################################################################

/** Solver Result Class

    This provides information on a result as a set of tighter bounds on both ways
*/

class OsiSolverResult  {
  
public:
  ///@name Add and Get methods 
  //@{
  /// Create result
  void createResult(const OsiSolverInterface & solver,const double * lowerBefore,
                    const double * upperBefore);
  
  /// Restore result
  void restoreResult(OsiSolverInterface & solver) const;
  
  /// Get basis
  inline const CoinWarmStartBasis & basis() const
  { return basis_;}
  
  /// Objective value (as minimization)
  inline double objectiveValue() const
  { return objectiveValue_;}

  /// Primal solution
  inline const double * primalSolution() const
  { return primalSolution_;}

  /// Dual solution
  inline const double * dualSolution() const
  { return dualSolution_;}

  /// Extra fixed
  inline const OsiSolverBranch & fixed() const
  { return fixed_;}
  //@}
  
  
  ///@name Constructors and destructors
  //@{
  /// Default Constructor
  OsiSolverResult(); 
  
  /// Constructor from solver
  OsiSolverResult(const OsiSolverInterface & solver,const double * lowerBefore,
                  const double * upperBefore); 
  
  /// Copy constructor 
  OsiSolverResult(const OsiSolverResult & rhs);
  
  /// Assignment operator 
  OsiSolverResult & operator=(const OsiSolverResult & rhs);
  
  /// Destructor 
  ~OsiSolverResult ();
  
  //@}
  
private:
  ///@name Private member data 
  //@{
  /// Value of objective (if >= OsiSolverInterface::getInfinity() then infeasible) 
  double objectiveValue_;
  /// Warm start information
  CoinWarmStartBasis basis_;
  /// Primal solution (numberColumns)
  double * primalSolution_;
  /// Dual solution (numberRows)
  double * dualSolution_;
  /// Which extra variables have been fixed (only way==-1 counts)
  OsiSolverBranch fixed_;
 //@}
};
#endif
