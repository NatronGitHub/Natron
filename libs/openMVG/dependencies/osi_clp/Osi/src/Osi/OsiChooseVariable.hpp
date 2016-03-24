// Copyright (C) 2006, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef OsiChooseVariable_H
#define OsiChooseVariable_H

#include <string>
#include <vector>

#include "CoinWarmStartBasis.hpp"
#include "OsiBranchingObject.hpp"

class OsiSolverInterface;
class OsiHotInfo;

/** This class chooses a variable to branch on

    The base class just chooses the variable and direction without strong branching but it 
    has information which would normally be used by strong branching e.g. to re-enter
    having fixed a variable but using same candidates for strong branching.

    The flow is :
    a) initialize the process.  This decides on strong branching list
       and stores indices of all infeasible objects  
    b) do strong branching on list.  If list is empty then just
       choose one candidate and return without strong branching.  If not empty then
       go through list and return best.  However we may find that the node is infeasible
       or that we can fix a variable.  If so we return and it is up to user to call
       again (after fixing a variable).
*/

class OsiChooseVariable  {
 
public:
    
  /// Default Constructor 
  OsiChooseVariable ();

  /// Constructor from solver (so we can set up arrays etc)
  OsiChooseVariable (const OsiSolverInterface * solver);

  /// Copy constructor 
  OsiChooseVariable (const OsiChooseVariable &);
   
  /// Assignment operator 
  OsiChooseVariable & operator= (const OsiChooseVariable& rhs);

  /// Clone
  virtual OsiChooseVariable * clone() const;

  /// Destructor 
  virtual ~OsiChooseVariable ();

  /** Sets up strong list and clears all if initialize is true.
      Returns number of infeasibilities. 
      If returns -1 then has worked out node is infeasible!
  */
  virtual int setupList ( OsiBranchingInformation *info, bool initialize);
  /** Choose a variable
      Returns - 
     -1 Node is infeasible
     0  Normal termination - we have a candidate
     1  All looks satisfied - no candidate
     2  We can change the bound on a variable - but we also have a strong branching candidate
     3  We can change the bound on a variable - but we have a non-strong branching candidate
     4  We can change the bound on a variable - no other candidates
     We can pick up branch from bestObjectIndex() and bestWhichWay()
     We can pick up a forced branch (can change bound) from firstForcedObjectIndex() and firstForcedWhichWay()
     If we have a solution then we can pick up from goodObjectiveValue() and goodSolution()
     If fixVariables is true then 2,3,4 are all really same as problem changed
  */
  virtual int chooseVariable( OsiSolverInterface * solver, OsiBranchingInformation *info, bool fixVariables);
  /// Returns true if solution looks feasible against given objects
  virtual bool feasibleSolution(const OsiBranchingInformation * info,
			const double * solution,
			int numberObjects,
			const OsiObject ** objects);
  /// Saves a good solution
  void saveSolution(const OsiSolverInterface * solver);
  /// Clears out good solution after use
  void clearGoodSolution();
  /// Given a candidate fill in useful information e.g. estimates
  virtual void updateInformation( const OsiBranchingInformation *info,
				  int branch, OsiHotInfo * hotInfo);
#if 1
  /// Given a branch fill in useful information e.g. estimates
  virtual void updateInformation( int whichObject, int branch, 
				  double changeInObjective, double changeInValue,
				  int status);
#endif
  /// Objective value for feasible solution
  inline double goodObjectiveValue() const
  { return goodObjectiveValue_;}
  /// Estimate of up change or change on chosen if n-way
  inline double upChange() const
  { return upChange_;}
  /// Estimate of down change or max change on other possibilities if n-way
  inline double downChange() const
  { return downChange_;}
  /// Good solution - deleted by finalize
  inline const double * goodSolution() const
  { return goodSolution_;}
  /// Index of chosen object
  inline int bestObjectIndex() const
  { return bestObjectIndex_;}
  /// Set index of chosen object
  inline void setBestObjectIndex(int value)
  { bestObjectIndex_ = value;}
  /// Preferred way of chosen object
  inline int bestWhichWay() const
  { return bestWhichWay_;}
  /// Set preferred way of chosen object
  inline void setBestWhichWay(int value)
  { bestWhichWay_ = value;}
  /// Index of forced object
  inline int firstForcedObjectIndex() const
  { return firstForcedObjectIndex_;}
  /// Set index of forced object
  inline void setFirstForcedObjectIndex(int value)
  { firstForcedObjectIndex_ = value;}
  /// Preferred way of forced object
  inline int firstForcedWhichWay() const
  { return firstForcedWhichWay_;}
  /// Set preferred way of forced object
  inline void setFirstForcedWhichWay(int value)
  { firstForcedWhichWay_ = value;}
  /// Get the number of objects unsatisfied at this node - accurate on first pass
  inline int numberUnsatisfied() const
  {return numberUnsatisfied_;}
  /// Number of objects to choose for strong branching
  inline int numberStrong() const
  { return numberStrong_;}
  /// Set number of objects to choose for strong branching
  inline void setNumberStrong(int value)
  { numberStrong_ = value;}
  /// Number left on strong list
  inline int numberOnList() const
  { return numberOnList_;}
  /// Number of strong branches actually done 
  inline int numberStrongDone() const
  { return numberStrongDone_;}
  /// Number of strong iterations actually done 
  inline int numberStrongIterations() const
  { return numberStrongIterations_;}
  /// Number of strong branches which changed bounds 
  inline int numberStrongFixed() const
  { return numberStrongFixed_;}
  /// List of candidates
  inline const int * candidates() const
  { return list_;}
  /// Trust results from strong branching for changing bounds
  inline bool trustStrongForBound() const
  { return trustStrongForBound_;}
  /// Set trust results from strong branching for changing bounds
  inline void setTrustStrongForBound(bool yesNo)
  { trustStrongForBound_ = yesNo;}
  /// Trust results from strong branching for valid solution
  inline bool trustStrongForSolution() const
  { return trustStrongForSolution_;}
  /// Set trust results from strong branching for valid solution
  inline void setTrustStrongForSolution(bool yesNo)
  { trustStrongForSolution_ = yesNo;}
  /// Set solver and redo arrays
  void setSolver (const OsiSolverInterface * solver);
  /** Return status - 
     -1 Node is infeasible
     0  Normal termination - we have a candidate
     1  All looks satisfied - no candidate
     2  We can change the bound on a variable - but we also have a strong branching candidate
     3  We can change the bound on a variable - but we have a non-strong branching candidate
     4  We can change the bound on a variable - no other candidates
     We can pick up branch from bestObjectIndex() and bestWhichWay()
     We can pick up a forced branch (can change bound) from firstForcedObjectIndex() and firstForcedWhichWay()
     If we have a solution then we can pick up from goodObjectiveValue() and goodSolution()
  */
  inline int status() const
  { return status_;}
  inline void setStatus(int value)
  { status_ = value;}


protected:
  // Data
  /// Objective value for feasible solution
  double goodObjectiveValue_;
  /// Estimate of up change or change on chosen if n-way
  double upChange_;
  /// Estimate of down change or max change on other possibilities if n-way
  double downChange_;
  /// Good solution - deleted by finalize
  double * goodSolution_;
  /// List of candidates
  int * list_;
  /// Useful array (for sorting etc)
  double * useful_;
  /// Pointer to solver
  const OsiSolverInterface * solver_;
  /* Status -
     -1 Node is infeasible
     0  Normal termination - we have a candidate
     1  All looks satisfied - no candidate
     2  We can change the bound on a variable - but we also have a strong branching candidate
     3  We can change the bound on a variable - but we have a non-strong branching candidate
     4  We can change the bound on a variable - no other candidates
  */
  int status_;
  /// Index of chosen object
  int bestObjectIndex_;
  /// Preferred way of chosen object
  int bestWhichWay_;
  /// Index of forced object
  int firstForcedObjectIndex_;
  /// Preferred way of forced object
  int firstForcedWhichWay_;
  /// The number of objects unsatisfied at this node.
  int numberUnsatisfied_;
  /// Number of objects to choose for strong branching
  int numberStrong_;
  /// Number left on strong list
  int numberOnList_;
  /// Number of strong branches actually done 
  int numberStrongDone_;
  /// Number of strong iterations actually done 
  int numberStrongIterations_;
  /// Number of bound changes due to strong branching
  int numberStrongFixed_;
  /// List of unsatisfied objects - first numberOnList_ for strong branching
  /// Trust results from strong branching for changing bounds
  bool trustStrongForBound_;
  /// Trust results from strong branching for valid solution
  bool trustStrongForSolution_;
};

/** This class is the placeholder for the pseudocosts used by OsiChooseStrong.
    It can also be used by any other pseudocost based strong branching
    algorithm.
*/

class OsiPseudoCosts {
protected:
   // Data
  /// Total of all changes up
  double * upTotalChange_;
  /// Total of all changes down
  double * downTotalChange_;
  /// Number of times up
  int * upNumber_;
  /// Number of times down
  int * downNumber_;
  /// Number of objects (could be found from solver)
  int numberObjects_;
  /// Number before we trust
  int numberBeforeTrusted_;

private:
  void gutsOfDelete();
  void gutsOfCopy(const OsiPseudoCosts& rhs);

public:
  OsiPseudoCosts();
  virtual ~OsiPseudoCosts();
  OsiPseudoCosts(const OsiPseudoCosts& rhs);
  OsiPseudoCosts& operator=(const OsiPseudoCosts& rhs);

  /// Number of times before trusted
  inline int numberBeforeTrusted() const
  { return numberBeforeTrusted_; }
  /// Set number of times before trusted
  inline void setNumberBeforeTrusted(int value)
  { numberBeforeTrusted_ = value; }
  /// Initialize the pseudocosts with n entries
  void initialize(int n);
  /// Give the number of objects for which pseudo costs are stored
  inline int numberObjects() const
  { return numberObjects_; }

  /** @name Accessor methods to pseudo costs data */
  //@{
  inline double* upTotalChange()               { return upTotalChange_; }
  inline const double* upTotalChange() const   { return upTotalChange_; }

  inline double* downTotalChange()             { return downTotalChange_; }
  inline const double* downTotalChange() const { return downTotalChange_; }

  inline int* upNumber()                       { return upNumber_; }
  inline const int* upNumber() const           { return upNumber_; }

  inline int* downNumber()                     { return downNumber_; }
  inline const int* downNumber() const         { return downNumber_; }
  //@}

  /// Given a candidate fill in useful information e.g. estimates
  virtual void updateInformation(const OsiBranchingInformation *info,
				  int branch, OsiHotInfo * hotInfo);
#if 1 
  /// Given a branch fill in useful information e.g. estimates
  virtual void updateInformation( int whichObject, int branch, 
				  double changeInObjective, double changeInValue,
				  int status);
#endif
};

/** This class chooses a variable to branch on

    This chooses the variable and direction with reliability strong branching.

    The flow is :
    a) initialize the process.  This decides on strong branching list
       and stores indices of all infeasible objects  
    b) do strong branching on list.  If list is empty then just
       choose one candidate and return without strong branching.  If not empty then
       go through list and return best.  However we may find that the node is infeasible
       or that we can fix a variable.  If so we return and it is up to user to call
       again (after fixing a variable).
*/

class OsiChooseStrong  : public OsiChooseVariable {
 
public:
    
  /// Default Constructor 
  OsiChooseStrong ();

  /// Constructor from solver (so we can set up arrays etc)
  OsiChooseStrong (const OsiSolverInterface * solver);

  /// Copy constructor 
  OsiChooseStrong (const OsiChooseStrong &);
   
  /// Assignment operator 
  OsiChooseStrong & operator= (const OsiChooseStrong& rhs);

  /// Clone
  virtual OsiChooseVariable * clone() const;

  /// Destructor 
  virtual ~OsiChooseStrong ();

  /** Sets up strong list and clears all if initialize is true.
      Returns number of infeasibilities. 
      If returns -1 then has worked out node is infeasible!
  */
  virtual int setupList ( OsiBranchingInformation *info, bool initialize);
  /** Choose a variable
      Returns - 
     -1 Node is infeasible
     0  Normal termination - we have a candidate
     1  All looks satisfied - no candidate
     2  We can change the bound on a variable - but we also have a strong branching candidate
     3  We can change the bound on a variable - but we have a non-strong branching candidate
     4  We can change the bound on a variable - no other candidates
     We can pick up branch from bestObjectIndex() and bestWhichWay()
     We can pick up a forced branch (can change bound) from firstForcedObjectIndex() and firstForcedWhichWay()
     If we have a solution then we can pick up from goodObjectiveValue() and goodSolution()
     If fixVariables is true then 2,3,4 are all really same as problem changed
  */
  virtual int chooseVariable( OsiSolverInterface * solver, OsiBranchingInformation *info, bool fixVariables);

  /** Pseudo Shadow Price mode
      0 - off
      1 - use if no strong info
      2 - use if strong not trusted
      3 - use even if trusted
  */
  inline int shadowPriceMode() const
  { return shadowPriceMode_;}
  /// Set Shadow price mode
  inline void setShadowPriceMode(int value)
  { shadowPriceMode_ = value;}

  /** Accessor method to pseudo cost object*/
  const OsiPseudoCosts& pseudoCosts() const
  { return pseudoCosts_; }

  /** Accessor method to pseudo cost object*/
  OsiPseudoCosts& pseudoCosts()
  { return pseudoCosts_; }

  /** A feww pass-through methods to access members of pseudoCosts_ as if they
      were members of OsiChooseStrong object */
  inline int numberBeforeTrusted() const {
    return pseudoCosts_.numberBeforeTrusted(); }
  inline void setNumberBeforeTrusted(int value) {
    pseudoCosts_.setNumberBeforeTrusted(value); }
  inline int numberObjects() const {
    return pseudoCosts_.numberObjects(); }

protected:

  /**  This is a utility function which does strong branching on
       a list of objects and stores the results in OsiHotInfo.objects.
       On entry the object sequence is stored in the OsiHotInfo object
       and maybe more.
       It returns -
       -1 - one branch was infeasible both ways
       0 - all inspected - nothing can be fixed
       1 - all inspected - some can be fixed (returnCriterion==0)
       2 - may be returning early - one can be fixed (last one done) (returnCriterion==1) 
       3 - returning because max time
       
  */
  int doStrongBranching( OsiSolverInterface * solver, 
			 OsiBranchingInformation *info,
			 int numberToDo, int returnCriterion);

  /** Clear out the results array */
  void resetResults(int num);

protected:
  /** Pseudo Shadow Price mode
      0 - off
      1 - use and multiply by strong info
      2 - use 
  */
  int shadowPriceMode_;

  /** The pseudo costs for the chooser */
  OsiPseudoCosts pseudoCosts_;

  /** The results of the strong branching done on the candidates where the
      pseudocosts were not sufficient */
  OsiHotInfo* results_;
  /** The number of OsiHotInfo objetcs that contain information */
  int numResults_;
};

/** This class contains the result of strong branching on a variable
    When created it stores enough information for strong branching
*/

class OsiHotInfo  {
 
public:
    
  /// Default Constructor 
  OsiHotInfo ();

  /// Constructor from useful information
  OsiHotInfo ( OsiSolverInterface * solver, 
	       const OsiBranchingInformation *info,
	       const OsiObject * const * objects,
	       int whichObject);

  /// Copy constructor 
  OsiHotInfo (const OsiHotInfo &);
   
  /// Assignment operator 
  OsiHotInfo & operator= (const OsiHotInfo& rhs);

  /// Clone
  virtual OsiHotInfo * clone() const;

  /// Destructor 
  virtual ~OsiHotInfo ();

  /** Fill in useful information after strong branch.
      Return status
  */
  int updateInformation( const OsiSolverInterface * solver, const OsiBranchingInformation * info,
			 OsiChooseVariable * choose);
  /// Original objective value
  inline double originalObjectiveValue() const
  { return originalObjectiveValue_;}
  /// Up change  - invalid if n-way
  inline double upChange() const
  { assert (branchingObject_->numberBranches()==2); return changes_[1];}
  /// Down change  - invalid if n-way
  inline double downChange() const
  { assert (branchingObject_->numberBranches()==2); return changes_[0];}
  /// Set up change  - invalid if n-way
  inline void setUpChange(double value)
  { assert (branchingObject_->numberBranches()==2); changes_[1] = value;}
  /// Set down change  - invalid if n-way
  inline void setDownChange(double value)
  { assert (branchingObject_->numberBranches()==2); changes_[0] = value;}
  /// Change on way k
  inline double change(int k) const
  { return changes_[k];}

  /// Up iteration count  - invalid if n-way
  inline int upIterationCount() const
  { assert (branchingObject_->numberBranches()==2); return iterationCounts_[1];}
  /// Down iteration count  - invalid if n-way
  inline int downIterationCount() const
  { assert (branchingObject_->numberBranches()==2); return iterationCounts_[0];}
  /// Iteration count on way k
  inline int iterationCount(int k) const
  { return iterationCounts_[k];}

  /// Up status  - invalid if n-way
  inline int upStatus() const
  { assert (branchingObject_->numberBranches()==2); return statuses_[1];}
  /// Down status  - invalid if n-way
  inline int downStatus() const
  { assert (branchingObject_->numberBranches()==2); return statuses_[0];}
  /// Set up status  - invalid if n-way
  inline void setUpStatus(int value)
  { assert (branchingObject_->numberBranches()==2); statuses_[1] = value;}
  /// Set down status  - invalid if n-way
  inline void setDownStatus(int value)
  { assert (branchingObject_->numberBranches()==2); statuses_[0] = value;}
  /// Status on way k
  inline int status(int k) const
  { return statuses_[k];}
  /// Branching object
  inline OsiBranchingObject * branchingObject() const
  { return branchingObject_;}
  inline int whichObject() const
  { return whichObject_;}

protected:
  // Data
  /// Original objective value
  double originalObjectiveValue_;
    /// Objective changes
  double * changes_;
  /// Iteration counts
  int * iterationCounts_;
  /** Status
      -1 - not done
      0 - feasible and finished
      1 -  infeasible
      2 - not finished
  */
  int * statuses_;
  /// Branching object
  OsiBranchingObject * branchingObject_;
  /// Which object on list
  int whichObject_;
};


#endif
