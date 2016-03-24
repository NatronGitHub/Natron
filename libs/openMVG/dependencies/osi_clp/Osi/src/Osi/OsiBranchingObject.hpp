// Copyright (C) 2006, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef OsiBranchingObject_H
#define OsiBranchingObject_H

#include <cassert>
#include <string>
#include <vector>

#include "CoinError.hpp"
#include "CoinTypes.hpp"

class OsiSolverInterface;
class OsiSolverBranch;

class OsiBranchingObject;
class OsiBranchingInformation;

//#############################################################################
//This contains the abstract base class for an object and for branching.
//It also contains a simple integer class
//#############################################################################

/** Abstract base class for `objects'.

  The branching model used in Osi is based on the idea of an <i>object</i>.
  In the abstract, an object is something that has a feasible region, can be
  evaluated for infeasibility, can be branched on (<i>i.e.</i>, there's some
  constructive action to be taken to move toward feasibility), and allows
  comparison of the effect of branching.

  This class (OsiObject) is the base class for an object. To round out the
  branching model, the class OsiBranchingObject describes how to perform a
  branch, and the class OsiBranchDecision describes how to compare two
  OsiBranchingObjects.

  To create a new type of object you need to provide three methods:
  #infeasibility(), #feasibleRegion(), and #createBranch(), described below.

  This base class is primarily virtual to allow for any form of structure.
  Any form of discontinuity is allowed.

  As there is an overhead in getting information from solvers and because
  other useful information is available there is also an OsiBranchingInformation 
  class which can contain pointers to information.
  If used it must at minimum contain pointers to current value of objective,
  maximum allowed objective and pointers to arrays for bounds and solution
  and direction of optimization.  Also integer and primal tolerance.
  
  Classes which inherit might have other information such as depth, number of
  solutions, pseudo-shadow prices etc etc.
  May be easier just to throw in here - as I keep doing
*/
class OsiObject {

public:
  
  /// Default Constructor 
  OsiObject ();
  
  /// Copy constructor 
  OsiObject ( const OsiObject &);
  
  /// Assignment operator 
  OsiObject & operator=( const OsiObject& rhs);
  
  /// Clone
  virtual OsiObject * clone() const=0;
  
  /// Destructor 
  virtual ~OsiObject ();
  
  /** Infeasibility of the object
      
    This is some measure of the infeasibility of the object. 0.0 
    indicates that the object is satisfied.
  
    The preferred branching direction is returned in whichWay, where for
    normal two-way branching 0 is down, 1 is up
  
    This is used to prepare for strong branching but should also think of
    case when no strong branching
  
    The object may also compute an estimate of cost of going "up" or "down".
    This will probably be based on pseudo-cost ideas

    This should also set mutable infeasibility_ and whichWay_
    This is for instant re-use for speed

    Default for this just calls infeasibility with OsiBranchingInformation
    NOTE - Convention says that an infeasibility of COIN_DBL_MAX means 
    object has worked out it can't be satisfied!
  */
  double infeasibility(const OsiSolverInterface * solver,int &whichWay) const ;
  // Faster version when more information available
  virtual double infeasibility(const OsiBranchingInformation * info, int &whichWay) const =0;
  // This does NOT set mutable stuff
  virtual double checkInfeasibility(const OsiBranchingInformation * info) const;
  
  /** For the variable(s) referenced by the object,
      look at the current solution and set bounds to match the solution.
      Returns measure of how much it had to move solution to make feasible
  */
  virtual double feasibleRegion(OsiSolverInterface * solver) const ;
  /** For the variable(s) referenced by the object,
      look at the current solution and set bounds to match the solution.
      Returns measure of how much it had to move solution to make feasible
      Faster version
  */
  virtual double feasibleRegion(OsiSolverInterface * solver, const OsiBranchingInformation * info) const =0;
  
  /** Create a branching object and indicate which way to branch first.
      
      The branching object has to know how to create branches (fix
      variables, etc.)
  */
  virtual OsiBranchingObject * createBranch(OsiSolverInterface * /*solver*/,
					    const OsiBranchingInformation * /*info*/,
					    int /*way*/) const {throw CoinError("Need code","createBranch","OsiBranchingObject"); return NULL; }
  
  /** \brief Return true if object can take part in normal heuristics
  */
  virtual bool canDoHeuristics() const 
  {return true;}
  /** \brief Return true if object can take part in move to nearest heuristic
  */
  virtual bool canMoveToNearest() const 
  {return false;}
  /** Column number if single column object -1 otherwise,
      Used by heuristics
  */
  virtual int columnNumber() const;
  /// Return Priority - note 1 is highest priority
  inline int priority() const
  { return priority_;}
  /// Set priority
  inline void setPriority(int priority)
  { priority_ = priority;}
  /** \brief Return true if branch should only bound variables
  */
  virtual bool boundBranch() const 
  {return true;}
  /// Return true if knows how to deal with Pseudo Shadow Prices
  virtual bool canHandleShadowPrices() const
  { return false;}
  /// Return maximum number of ways branch may have
  inline int numberWays() const
  { return numberWays_;}
  /// Set maximum number of ways branch may have
  inline void setNumberWays(int numberWays)
  { numberWays_ = static_cast<short int>(numberWays) ; }
  /** Return preferred way to branch.  If two
      then way=0 means down and 1 means up, otherwise
      way points to preferred branch
  */
  inline void setWhichWay(int way)
  { whichWay_ = static_cast<short int>(way) ; }
  /** Return current preferred way to branch.  If two
      then way=0 means down and 1 means up, otherwise
      way points to preferred branch
  */
  inline int whichWay() const
  { return whichWay_;}
  /// Get pre-emptive preferred way of branching - -1 off, 0 down, 1 up (for 2-way)
  virtual int preferredWay() const
  { return -1;}
  /// Return infeasibility
  inline double infeasibility() const
  { return infeasibility_;}
  /// Return "up" estimate (default 1.0e-5)
  virtual double upEstimate() const;
  /// Return "down" estimate (default 1.0e-5)
  virtual double downEstimate() const;
  /** Reset variable bounds to their original values.
    Bounds may be tightened, so it may be good to be able to reset them to
    their original values.
   */
  virtual void resetBounds(const OsiSolverInterface * ) {}
  /**  Change column numbers after preprocessing
   */
  virtual void resetSequenceEtc(int , const int * ) {}
  /// Updates stuff like pseudocosts before threads
  virtual void updateBefore(const OsiObject * ) {}
  /// Updates stuff like pseudocosts after threads finished
  virtual void updateAfter(const OsiObject * , const OsiObject * ) {}

protected:
  /// data

  /// Computed infeasibility
  mutable double infeasibility_;
  /// Computed preferred way to branch 
  mutable short whichWay_;
  /// Maximum number of ways on branch
  short  numberWays_;
  /// Priority
  int priority_;

};
/// Define a class to add a bit of complexity to OsiObject
/// This assumes 2 way branching


class OsiObject2 : public OsiObject {

public:

  /// Default Constructor 
  OsiObject2 ();

  /// Copy constructor 
  OsiObject2 ( const OsiObject2 &);
   
  /// Assignment operator 
  OsiObject2 & operator=( const OsiObject2& rhs);

  /// Destructor 
  virtual ~OsiObject2 ();
  
  /// Set preferred way of branching - -1 off, 0 down, 1 up (for 2-way)
  inline void setPreferredWay(int value)
  {preferredWay_=value;}
  
  /// Get preferred way of branching - -1 off, 0 down, 1 up (for 2-way)
  virtual int preferredWay() const
  { return preferredWay_;}
protected:
  /// Preferred way of branching - -1 off, 0 down, 1 up (for 2-way)
  int preferredWay_;
  /// "Infeasibility" on other way
  mutable double otherInfeasibility_;
  
};

/** \brief Abstract branching object base class

  In the abstract, an OsiBranchingObject contains instructions for how to
  branch. We want an abstract class so that we can describe how to branch on
  simple objects (<i>e.g.</i>, integers) and more exotic objects
  (<i>e.g.</i>, cliques or hyperplanes).

  The #branch() method is the crucial routine: it is expected to be able to
  step through a set of branch arms, executing the actions required to create
  each subproblem in turn. The base class is primarily virtual to allow for
  a wide range of problem modifications.

  See OsiObject for an overview of the two classes (OsiObject and
  OsiBranchingObject) which make up Osi's branching
  model.
*/

class OsiBranchingObject {

public:

  /// Default Constructor 
  OsiBranchingObject ();

  /// Constructor 
  OsiBranchingObject (OsiSolverInterface * solver, double value);
  
  /// Copy constructor 
  OsiBranchingObject ( const OsiBranchingObject &);
   
  /// Assignment operator 
  OsiBranchingObject & operator=( const OsiBranchingObject& rhs);

  /// Clone
  virtual OsiBranchingObject * clone() const=0;

  /// Destructor 
  virtual ~OsiBranchingObject ();

  /// The number of branch arms created for this branching object
  inline int numberBranches() const
  {return numberBranches_;}

  /// The number of branch arms left for this branching object
  inline int numberBranchesLeft() const
  {return numberBranches_-branchIndex_;}

  /// Increment the number of branch arms left for this branching object
  inline void incrementNumberBranchesLeft()
  { numberBranches_ ++;}

  /** Set the number of branch arms left for this branching object
      Just for forcing
  */
  inline void setNumberBranchesLeft(int /*value*/)
  {/*assert (value==1&&!branchIndex_);*/ numberBranches_=1;}

  /// Decrement the number of branch arms left for this branching object
  inline void decrementNumberBranchesLeft()
  {branchIndex_++;}

  /** \brief Execute the actions required to branch, as specified by the
	     current state of the branching object, and advance the object's
	     state. 
	     Returns change in guessed objective on next branch
  */
  virtual double branch(OsiSolverInterface * solver)=0;
  /** \brief Execute the actions required to branch, as specified by the
	     current state of the branching object, and advance the object's
	     state. 
	     Returns change in guessed objective on next branch
  */
  virtual double branch() {return branch(NULL);}
  /** \brief Return true if branch should fix variables
  */
  virtual bool boundBranch() const 
  {return true;}
  /** Get the state of the branching object
      This is just the branch index
  */
  inline int branchIndex() const
  {return branchIndex_;}

  /** Set the state of the branching object.
  */
  inline void setBranchingIndex(int branchIndex)
  { branchIndex_ = static_cast<short int>(branchIndex) ; }

  /// Current value
  inline double value() const
  {return value_;}
  
  /// Return pointer back to object which created
  inline const OsiObject * originalObject() const
  {return  originalObject_;}
  /// Set pointer back to object which created
  inline void setOriginalObject(const OsiObject * object)
  {originalObject_=object;}
  /** Double checks in case node can change its mind!
      Returns objective value
      Can change objective etc */
  virtual void checkIsCutoff(double ) {}
  /// For debug
  int columnNumber() const;
  /** \brief Print something about branch - only if log level high
  */
  virtual void print(const OsiSolverInterface * =NULL) const {}

protected:

  /// Current value - has some meaning about branch
  double value_;

  /// Pointer back to object which created
  const OsiObject * originalObject_;

  /** Number of branches
  */
  int numberBranches_;

  /** The state of the branching object. i.e. branch index
      This starts at 0 when created
  */
  short branchIndex_;

};
/* This contains information
   This could also contain pseudo shadow prices
   or information for dealing with computing and trusting pseudo-costs
*/
class OsiBranchingInformation {

public:
  
  /// Default Constructor 
  OsiBranchingInformation ();
  
  /** Useful Constructor 
      (normalSolver true if has matrix etc etc)
      copySolution true if constructot should make a copy
  */
  OsiBranchingInformation (const OsiSolverInterface * solver, bool normalSolver,bool copySolution=false);
  
  /// Copy constructor 
  OsiBranchingInformation ( const OsiBranchingInformation &);
  
  /// Assignment operator 
  OsiBranchingInformation & operator=( const OsiBranchingInformation& rhs);
  
  /// Clone
  virtual OsiBranchingInformation * clone() const;
  
  /// Destructor 
  virtual ~OsiBranchingInformation ();
  
  // Note public
public:
  /// data

  /** State of search
      0 - no solution
      1 - only heuristic solutions
      2 - branched to a solution 
      3 - no solution but many nodes
  */
  int stateOfSearch_;
  /// Value of objective function (in minimization sense)
  double objectiveValue_;
  /// Value of objective cutoff (in minimization sense)
  double cutoff_;
  /// Direction 1.0 for minimization, -1.0 for maximization
  double direction_;
  /// Integer tolerance
  double integerTolerance_;
  /// Primal tolerance
  double primalTolerance_;
  /// Maximum time remaining before stopping on time
  double timeRemaining_;
  /// Dual to use if row bound violated (if negative then pseudoShadowPrices off)
  double defaultDual_;
  /// Pointer to solver
  mutable const OsiSolverInterface * solver_;
  /// The number of columns
  int numberColumns_;
  /// Pointer to current lower bounds on columns
  mutable const double * lower_;
  /// Pointer to current solution
  mutable const double * solution_;
  /// Pointer to current upper bounds on columns
  mutable const double * upper_;
  /// Highly optional target (hot start) solution
  const double * hotstartSolution_;
  /// Pointer to duals
  const double * pi_;
  /// Pointer to row activity
  const double * rowActivity_;
  /// Objective
  const double * objective_;
  /// Pointer to current lower bounds on rows
  const double * rowLower_;
  /// Pointer to current upper bounds on rows
  const double * rowUpper_;
  /// Elements in column copy of matrix
  const double * elementByColumn_;
  /// Column starts
  const CoinBigIndex * columnStart_;
  /// Column lengths
  const int * columnLength_;
  /// Row indices
  const int * row_;
  /** Useful region of length CoinMax(numberColumns,2*numberRows)
      This is allocated and deleted before OsiObject::infeasibility
      It is zeroed on entry and should be so on exit
      It only exists if defaultDual_>=0.0
  */
  double * usefulRegion_;
  /// Useful index region to go with usefulRegion_
  int * indexRegion_;
  /// Number of solutions found
  int numberSolutions_;
  /// Number of branching solutions found (i.e. exclude heuristics)
  int numberBranchingSolutions_;
  /// Depth in tree
  int depth_;
  /// TEMP
  bool owningSolution_;
};

/// This just adds two-wayness to a branching object

class OsiTwoWayBranchingObject : public OsiBranchingObject {

public:

  /// Default constructor 
  OsiTwoWayBranchingObject ();

  /** Create a standard tw0-way branch object

    Specifies a simple two-way branch.
    Specify way = -1 to set the object state to perform the down arm first,
    way = 1 for the up arm.
  */
  OsiTwoWayBranchingObject (OsiSolverInterface *solver,const OsiObject * originalObject,
			     int way , double value) ;
    
  /// Copy constructor 
  OsiTwoWayBranchingObject ( const OsiTwoWayBranchingObject &);
   
  /// Assignment operator 
  OsiTwoWayBranchingObject & operator= (const OsiTwoWayBranchingObject& rhs);

  /// Destructor 
  virtual ~OsiTwoWayBranchingObject ();

  using OsiBranchingObject::branch ;
  /** \brief Sets the bounds for the variable according to the current arm
	     of the branch and advances the object state to the next arm.
	     state. 
	     Returns change in guessed objective on next branch
  */
  virtual double branch(OsiSolverInterface * solver)=0;

  inline int firstBranch() const { return firstBranch_; }
  /// Way returns -1 on down +1 on up
  inline int way() const
  { return !branchIndex_ ? firstBranch_ : -firstBranch_;}
protected:
  /// Which way was first branch -1 = down, +1 = up
  int firstBranch_;
};
/// Define a single integer class


class OsiSimpleInteger : public OsiObject2 {

public:

  /// Default Constructor 
  OsiSimpleInteger ();

  /// Useful constructor - passed solver index
  OsiSimpleInteger (const OsiSolverInterface * solver, int iColumn);
  
  /// Useful constructor - passed solver index and original bounds
  OsiSimpleInteger (int iColumn, double lower, double upper);
  
  /// Copy constructor 
  OsiSimpleInteger ( const OsiSimpleInteger &);
   
  /// Clone
  virtual OsiObject * clone() const;

  /// Assignment operator 
  OsiSimpleInteger & operator=( const OsiSimpleInteger& rhs);

  /// Destructor 
  virtual ~OsiSimpleInteger ();
  
  using OsiObject::infeasibility ;
  /// Infeasibility - large is 0.5
  virtual double infeasibility(const OsiBranchingInformation * info, int & whichWay) const;

  using OsiObject::feasibleRegion ;
  /** Set bounds to fix the variable at the current (integer) value.

    Given an integer value, set the lower and upper bounds to fix the
    variable. Returns amount it had to move variable.
  */
  virtual double feasibleRegion(OsiSolverInterface * solver, const OsiBranchingInformation * info) const;

  /** Creates a branching object

    The preferred direction is set by \p way, 0 for down, 1 for up.
  */
  virtual OsiBranchingObject * createBranch(OsiSolverInterface * solver, const OsiBranchingInformation * info, int way) const;


  /// Set solver column number
  inline void setColumnNumber(int value)
  {columnNumber_=value;}
  
  /** Column number if single column object -1 otherwise,
      so returns >= 0
      Used by heuristics
  */
  virtual int columnNumber() const;

  /// Original bounds
  inline double originalLowerBound() const
  { return originalLower_;}
  inline void setOriginalLowerBound(double value)
  { originalLower_=value;}
  inline double originalUpperBound() const
  { return originalUpper_;}
  inline void setOriginalUpperBound(double value)
  { originalUpper_=value;}
  /** Reset variable bounds to their original values.
    Bounds may be tightened, so it may be good to be able to reset them to
    their original values.
   */
  virtual void resetBounds(const OsiSolverInterface * solver) ;
  /**  Change column numbers after preprocessing
   */
  virtual void resetSequenceEtc(int numberColumns, const int * originalColumns);
  
  /// Return "up" estimate (default 1.0e-5)
  virtual double upEstimate() const;
  /// Return "down" estimate (default 1.0e-5)
  virtual double downEstimate() const;
  /// Return true if knows how to deal with Pseudo Shadow Prices
  virtual bool canHandleShadowPrices() const
  { return false;}
protected:
  /// data
  /// Original lower bound
  double originalLower_;
  /// Original upper bound
  double originalUpper_;
  /// Column number in solver
  int columnNumber_;
  
};
/** Simple branching object for an integer variable

  This object can specify a two-way branch on an integer variable. For each
  arm of the branch, the upper and lower bounds on the variable can be
  independently specified. 0 -> down, 1-> up.
*/

class OsiIntegerBranchingObject : public OsiTwoWayBranchingObject {

public:

  /// Default constructor 
  OsiIntegerBranchingObject ();

  /** Create a standard floor/ceiling branch object

    Specifies a simple two-way branch. Let \p value = x*. One arm of the
    branch will be lb <= x <= floor(x*), the other ceil(x*) <= x <= ub.
    Specify way = -1 to set the object state to perform the down arm first,
    way = 1 for the up arm.
  */
  OsiIntegerBranchingObject (OsiSolverInterface *solver,const OsiSimpleInteger * originalObject,
			     int way , double value) ;
  /** Create a standard floor/ceiling branch object

    Specifies a simple two-way branch in a more flexible way. One arm of the
    branch will be lb <= x <= downUpperBound, the other upLowerBound <= x <= ub.
    Specify way = -1 to set the object state to perform the down arm first,
    way = 1 for the up arm.
  */
  OsiIntegerBranchingObject (OsiSolverInterface *solver,const OsiSimpleInteger * originalObject,
			     int way , double value, double downUpperBound, double upLowerBound) ;
    
  /// Copy constructor 
  OsiIntegerBranchingObject ( const OsiIntegerBranchingObject &);
   
  /// Assignment operator 
  OsiIntegerBranchingObject & operator= (const OsiIntegerBranchingObject& rhs);

  /// Clone
  virtual OsiBranchingObject * clone() const;

  /// Destructor 
  virtual ~OsiIntegerBranchingObject ();
  
  using OsiBranchingObject::branch ;
  /** \brief Sets the bounds for the variable according to the current arm
	     of the branch and advances the object state to the next arm.
	     state. 
	     Returns change in guessed objective on next branch
  */
  virtual double branch(OsiSolverInterface * solver);

  using OsiBranchingObject::print ;
  /** \brief Print something about branch - only if log level high
  */
  virtual void print(const OsiSolverInterface * solver=NULL);

protected:
  // Probably could get away with just value which is already stored 
  /// Lower [0] and upper [1] bounds for the down arm (way_ = -1)
  double down_[2];
  /// Lower [0] and upper [1] bounds for the up arm (way_ = 1)
  double up_[2];
};


/** Define Special Ordered Sets of type 1 and 2.  These do not have to be
    integer - so do not appear in lists of integers.
    
    which_ points columns of matrix
*/


class OsiSOS : public OsiObject2 {

public:

  // Default Constructor 
  OsiSOS ();

  /** Useful constructor - which are indices
      and  weights are also given.  If null then 0,1,2..
      type is SOS type
  */
  OsiSOS (const OsiSolverInterface * solver, int numberMembers,
	   const int * which, const double * weights, int type=1);
  
  // Copy constructor 
  OsiSOS ( const OsiSOS &);
   
  /// Clone
  virtual OsiObject * clone() const;

  // Assignment operator 
  OsiSOS & operator=( const OsiSOS& rhs);

  // Destructor 
  virtual ~OsiSOS ();
  
  using OsiObject::infeasibility ;
  /// Infeasibility - large is 0.5
  virtual double infeasibility(const OsiBranchingInformation * info,int & whichWay) const;

  using OsiObject::feasibleRegion ;
  /** Set bounds to fix the variable at the current (integer) value.

    Given an integer value, set the lower and upper bounds to fix the
    variable. Returns amount it had to move variable.
  */
  virtual double feasibleRegion(OsiSolverInterface * solver, const OsiBranchingInformation * info) const;

  /** Creates a branching object

    The preferred direction is set by \p way, 0 for down, 1 for up.
  */
  virtual OsiBranchingObject * createBranch(OsiSolverInterface * solver, const OsiBranchingInformation * info, int way) const;
  /// Return "up" estimate (default 1.0e-5)
  virtual double upEstimate() const;
  /// Return "down" estimate (default 1.0e-5)
  virtual double downEstimate() const;
  
  /// Redoes data when sequence numbers change
  virtual void resetSequenceEtc(int numberColumns, const int * originalColumns);
  
  /// Number of members
  inline int numberMembers() const
  {return numberMembers_;}

  /// Members (indices in range 0 ... numberColumns-1)
  inline const int * members() const
  {return members_;}

  /// SOS type
  inline int sosType() const
  {return sosType_;}

  /// SOS type
  inline int setType() const
  {return sosType_;}

  /** Array of weights */
  inline const double * weights() const
  { return weights_;}

  /** \brief Return true if object can take part in normal heuristics
  */
  virtual bool canDoHeuristics() const 
  {return (sosType_==1&&integerValued_);}
  /// Set whether set is integer valued or not
  inline void setIntegerValued(bool yesNo)
  { integerValued_=yesNo;}
  /// Return true if knows how to deal with Pseudo Shadow Prices
  virtual bool canHandleShadowPrices() const
  { return true;}
  /// Set number of members
  inline void setNumberMembers(int value)
  {numberMembers_=value;}

  /// Members (indices in range 0 ... numberColumns-1)
  inline int * mutableMembers() const
  {return members_;}

  /// Set SOS type
  inline void setSosType(int value)
  {sosType_=value;}

  /** Array of weights */
  inline  double * mutableWeights() const
  { return weights_;}
protected:
  /// data

  /// Members (indices in range 0 ... numberColumns-1)
  int * members_;
  /// Weights
  double * weights_;

  /// Number of members
  int numberMembers_;
  /// SOS type
  int sosType_;
  /// Whether integer valued
  bool integerValued_;
};

/** Branching object for Special ordered sets

 */
class OsiSOSBranchingObject : public OsiTwoWayBranchingObject {

public:

  // Default Constructor 
  OsiSOSBranchingObject ();

  // Useful constructor
  OsiSOSBranchingObject (OsiSolverInterface * solver,  const OsiSOS * originalObject,
			    int way,
			 double separator);
  
  // Copy constructor 
  OsiSOSBranchingObject ( const OsiSOSBranchingObject &);
   
  // Assignment operator 
  OsiSOSBranchingObject & operator=( const OsiSOSBranchingObject& rhs);

  /// Clone
  virtual OsiBranchingObject * clone() const;

  // Destructor 
  virtual ~OsiSOSBranchingObject ();
  
  using OsiBranchingObject::branch ;
  /// Does next branch and updates state
  virtual double branch(OsiSolverInterface * solver);

  using OsiBranchingObject::print ;
  /** \brief Print something about branch - only if log level high
  */
  virtual void print(const OsiSolverInterface * solver=NULL);
private:
  /// data
};
/** Lotsize class */


class OsiLotsize : public OsiObject2 {

public:

  // Default Constructor 
  OsiLotsize ();

  /* Useful constructor - passed model index.
     Also passed valid values - if range then pairs
  */
  OsiLotsize (const OsiSolverInterface * solver, int iColumn,
	      int numberPoints, const double * points, bool range=false);
  
  // Copy constructor 
  OsiLotsize ( const OsiLotsize &);
   
  /// Clone
  virtual OsiObject * clone() const;

  // Assignment operator 
  OsiLotsize & operator=( const OsiLotsize& rhs);

  // Destructor 
  virtual ~OsiLotsize ();
  
  using OsiObject::infeasibility ;
  /// Infeasibility - large is 0.5
  virtual double infeasibility(const OsiBranchingInformation * info, int & whichWay) const;

  using OsiObject::feasibleRegion ;
  /** Set bounds to contain the current solution.

    More precisely, for the variable associated with this object, take the
    value given in the current solution, force it within the current bounds
    if required, then set the bounds to fix the variable at the integer
    nearest the solution value.  Returns amount it had to move variable.
  */
  virtual double feasibleRegion(OsiSolverInterface * solver, const OsiBranchingInformation * info) const;

  /** Creates a branching object

    The preferred direction is set by \p way, 0 for down, 1 for up.
  */
  virtual OsiBranchingObject * createBranch(OsiSolverInterface * solver, const OsiBranchingInformation * info, int way) const;


  /// Set solver column number
  inline void setColumnNumber(int value)
  {columnNumber_=value;}
  
  /** Column number if single column object -1 otherwise,
      so returns >= 0
      Used by heuristics
  */
  virtual int columnNumber() const;
  /** Reset original upper and lower bound values from the solver.
  
    Handy for updating bounds held in this object after bounds held in the
    solver have been tightened.
   */
  virtual void resetBounds(const OsiSolverInterface * solver);

  /** Finds range of interest so value is feasible in range range_ or infeasible 
      between hi[range_] and lo[range_+1].  Returns true if feasible.
  */
  bool findRange(double value, double integerTolerance) const;
  
  /** Returns floor and ceiling
  */
  virtual void floorCeiling(double & floorLotsize, double & ceilingLotsize, double value,
			    double tolerance) const;
  
  /// Original bounds
  inline double originalLowerBound() const
  { return bound_[0];}
  inline double originalUpperBound() const
  { return bound_[rangeType_*numberRanges_-1];}
  /// Type - 1 points, 2 ranges
  inline int rangeType() const
  { return rangeType_;}
  /// Number of points
  inline int numberRanges() const
  { return numberRanges_;}
  /// Ranges
  inline double * bound() const
  { return bound_;}
  /**  Change column numbers after preprocessing
   */
  virtual void resetSequenceEtc(int numberColumns, const int * originalColumns);
  
  /// Return "up" estimate (default 1.0e-5)
  virtual double upEstimate() const;
  /// Return "down" estimate (default 1.0e-5)
  virtual double downEstimate() const;
  /// Return true if knows how to deal with Pseudo Shadow Prices
  virtual bool canHandleShadowPrices() const
  { return true;}
  /** \brief Return true if object can take part in normal heuristics
  */
  virtual bool canDoHeuristics() const 
  {return false;}

private:
  /// data

  /// Column number in model
  int columnNumber_;
  /// Type - 1 points, 2 ranges
  int rangeType_;
  /// Number of points
  int numberRanges_;
  // largest gap
  double largestGap_;
  /// Ranges
  double * bound_;
  /// Current range
  mutable int range_;
};


/** Lotsize branching object

  This object can specify a two-way branch on an integer variable. For each
  arm of the branch, the upper and lower bounds on the variable can be
  independently specified.
  
  Variable_ holds the index of the integer variable in the integerVariable_
  array of the model.
*/

class OsiLotsizeBranchingObject : public OsiTwoWayBranchingObject {

public:

  /// Default constructor 
  OsiLotsizeBranchingObject ();

  /** Create a lotsize floor/ceiling branch object

    Specifies a simple two-way branch. Let \p value = x*. One arm of the
    branch will be is lb <= x <= valid range below(x*), the other valid range above(x*) <= x <= ub.
    Specify way = -1 to set the object state to perform the down arm first,
    way = 1 for the up arm.
  */
  OsiLotsizeBranchingObject (OsiSolverInterface *solver,const OsiLotsize * originalObject, 
			     int way , double value) ;
  
  /// Copy constructor 
  OsiLotsizeBranchingObject ( const OsiLotsizeBranchingObject &);
   
  /// Assignment operator 
  OsiLotsizeBranchingObject & operator= (const OsiLotsizeBranchingObject& rhs);

  /// Clone
  virtual OsiBranchingObject * clone() const;

  /// Destructor 
  virtual ~OsiLotsizeBranchingObject ();

  using OsiBranchingObject::branch ;
  /** \brief Sets the bounds for the variable according to the current arm
	     of the branch and advances the object state to the next arm.
	     state. 
	     Returns change in guessed objective on next branch
  */
  virtual double branch(OsiSolverInterface * solver);

  using OsiBranchingObject::print ;
  /** \brief Print something about branch - only if log level high
  */
  virtual void print(const OsiSolverInterface * solver=NULL);

protected:
  /// Lower [0] and upper [1] bounds for the down arm (way_ = -1)
  double down_[2];
  /// Lower [0] and upper [1] bounds for the up arm (way_ = 1)
  double up_[2];
};
#endif
