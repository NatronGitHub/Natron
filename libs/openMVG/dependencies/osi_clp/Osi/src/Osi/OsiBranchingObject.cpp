// Copyright (C) 2006, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#if defined(_MSC_VER)
// Turn off compiler warning about long names
#  pragma warning(disable:4786)
#endif
#include <cassert>
#include <cstdlib>
#include <cmath>
#include <cfloat>
//#define OSI_DEBUG
#include "OsiSolverInterface.hpp"
#include "OsiBranchingObject.hpp"
#include "CoinHelperFunctions.hpp"
#include "CoinPackedMatrix.hpp"
#include "CoinSort.hpp"
#include "CoinError.hpp"
#include "CoinFinite.hpp"

// Default Constructor
OsiObject::OsiObject() 
  :infeasibility_(0.0),
   whichWay_(0),
   numberWays_(2),
   priority_(1000)
{
}


// Destructor 
OsiObject::~OsiObject ()
{
}

// Copy constructor 
OsiObject::OsiObject ( const OsiObject & rhs)
{
  infeasibility_ = rhs.infeasibility_;
  whichWay_ = rhs.whichWay_;
  priority_ = rhs.priority_;
  numberWays_ = rhs.numberWays_;
}

// Assignment operator 
OsiObject & 
OsiObject::operator=( const OsiObject& rhs)
{
  if (this!=&rhs) {
    infeasibility_ = rhs.infeasibility_;
    whichWay_ = rhs.whichWay_;
    priority_ = rhs.priority_;
    numberWays_ = rhs.numberWays_;
  }
  return *this;
}
// Return "up" estimate (default 1.0e-5)
double 
OsiObject::upEstimate() const
{
  return 1.0e-5;
}
// Return "down" estimate (default 1.0e-5)
double 
OsiObject::downEstimate() const
{
  return 1.0e-5;
}
// Column number if single column object -1 otherwise
int 
OsiObject::columnNumber() const
{
  return -1;
}
// Infeasibility - large is 0.5
double 
OsiObject::infeasibility(const OsiSolverInterface * solver, int & preferredWay) const
{
  // Can't guarantee has matrix
  OsiBranchingInformation info(solver,false,false);
  return infeasibility(&info,preferredWay);
}
// This does NOT set mutable stuff
double 
OsiObject::checkInfeasibility(const OsiBranchingInformation * info) const
{
  int way;
  double saveInfeasibility = infeasibility_;
  short int saveWhichWay = whichWay_ ;
  double value = infeasibility(info,way);
  infeasibility_ = saveInfeasibility;
  whichWay_ = saveWhichWay;
  return value;
}

/* For the variable(s) referenced by the object,
   look at the current solution and set bounds to match the solution.
   Returns measure of how much it had to move solution to make feasible
*/
double 
OsiObject::feasibleRegion(OsiSolverInterface * solver) const 
{
  // Can't guarantee has matrix
  OsiBranchingInformation info(solver,false,false);
  return feasibleRegion(solver,&info);
}

// Default Constructor
OsiObject2::OsiObject2() 
  : OsiObject(),
    preferredWay_(-1),
    otherInfeasibility_(0.0)
{
}


// Destructor 
OsiObject2::~OsiObject2 ()
{
}

// Copy constructor 
OsiObject2::OsiObject2 ( const OsiObject2 & rhs)
  : OsiObject(rhs),
    preferredWay_(rhs.preferredWay_),
    otherInfeasibility_ (rhs.otherInfeasibility_)
{
}

// Assignment operator 
OsiObject2 & 
OsiObject2::operator=( const OsiObject2& rhs)
{
  if (this!=&rhs) {
    OsiObject::operator=(rhs);
    preferredWay_ = rhs.preferredWay_;
    otherInfeasibility_ = rhs.otherInfeasibility_;
  }
  return *this;
}
// Default Constructor 
OsiBranchingObject::OsiBranchingObject()
{
  originalObject_=NULL;
  branchIndex_=0;
  value_=0.0;
  numberBranches_=2;
}

// Useful constructor
OsiBranchingObject::OsiBranchingObject (OsiSolverInterface * ,
					 double value)
{
  originalObject_=NULL;
  branchIndex_=0;
  value_=value;
  numberBranches_=2;
}

// Copy constructor 
OsiBranchingObject::OsiBranchingObject ( const OsiBranchingObject & rhs)
{
  originalObject_=rhs.originalObject_;
  branchIndex_=rhs.branchIndex_;
  value_=rhs.value_;
  numberBranches_=rhs.numberBranches_;
}

// Assignment operator 
OsiBranchingObject & 
OsiBranchingObject::operator=( const OsiBranchingObject& rhs)
{
  if (this != &rhs) {
    originalObject_=rhs.originalObject_;
    branchIndex_=rhs.branchIndex_;
    value_=rhs.value_;
    numberBranches_=rhs.numberBranches_;
  }
  return *this;
}

// Destructor 
OsiBranchingObject::~OsiBranchingObject ()
{
}
// For debug
int 
OsiBranchingObject::columnNumber() const
{
  if (originalObject_)
    return originalObject_->columnNumber();
  else
    return -1;
}
/** Default Constructor

*/
OsiBranchingInformation::OsiBranchingInformation ()
  : objectiveValue_(COIN_DBL_MAX),
    cutoff_(COIN_DBL_MAX),
    direction_(COIN_DBL_MAX),
    integerTolerance_(1.0e-7),
    primalTolerance_(1.0e-7),
    timeRemaining_(COIN_DBL_MAX),
    defaultDual_(-1.0),
    solver_(NULL),
    numberColumns_(0),
    lower_(NULL),
    solution_(NULL),
    upper_(NULL),
    hotstartSolution_(NULL),
    pi_(NULL),
    rowActivity_(NULL),
    objective_(NULL),
    rowLower_(NULL),
    rowUpper_(NULL),
    elementByColumn_(NULL),
    columnStart_(NULL),
    columnLength_(NULL),
    row_(NULL),
    usefulRegion_(NULL),
    indexRegion_(NULL),
    numberSolutions_(0),
    numberBranchingSolutions_(0),
    depth_(0),
    owningSolution_(false)
{
}

/** Useful constructor
*/
OsiBranchingInformation::OsiBranchingInformation (const OsiSolverInterface * solver,
						  bool /*normalSolver*/,
						  bool owningSolution)
  : timeRemaining_(COIN_DBL_MAX),
    defaultDual_(-1.0),
    solver_(solver),
    hotstartSolution_(NULL),
    usefulRegion_(NULL),
    indexRegion_(NULL),
    numberSolutions_(0),
    numberBranchingSolutions_(0),
    depth_(0),
    owningSolution_(owningSolution)
{
  direction_ = solver_->getObjSense();
  objectiveValue_ = solver_->getObjValue();
  objectiveValue_ *= direction_;
  solver_->getDblParam(OsiDualObjectiveLimit,cutoff_) ;
  cutoff_ *= direction_;
  integerTolerance_ = solver_->getIntegerTolerance();
  solver_->getDblParam(OsiPrimalTolerance,primalTolerance_) ;
  numberColumns_ = solver_->getNumCols();
  lower_ = solver_->getColLower();
  if (owningSolution_)
    solution_ = CoinCopyOfArray(solver_->getColSolution(),numberColumns_);
  else
    solution_ = solver_->getColSolution();
  upper_ = solver_->getColUpper();
  pi_ = solver_->getRowPrice();
  rowActivity_ = solver_->getRowActivity();
  objective_ = solver_->getObjCoefficients();
  rowLower_ = solver_->getRowLower();
  rowUpper_ = solver_->getRowUpper();
  const CoinPackedMatrix* matrix = solver_->getMatrixByCol();
  if (matrix) {
    // Column copy of matrix if matrix exists
    elementByColumn_ = matrix->getElements();
    row_ = matrix->getIndices();
    columnStart_ = matrix->getVectorStarts();
    columnLength_ = matrix->getVectorLengths();
  } else {
    // Matrix does not exist
    elementByColumn_ = NULL;
    row_ = NULL;
    columnStart_ = NULL;
    columnLength_ = NULL;
  }
}
// Copy constructor 
OsiBranchingInformation::OsiBranchingInformation ( const OsiBranchingInformation & rhs)
{
  objectiveValue_ = rhs.objectiveValue_;
  cutoff_ = rhs.cutoff_;
  direction_ = rhs.direction_;
  integerTolerance_ = rhs.integerTolerance_;
  primalTolerance_ = rhs.primalTolerance_;
  timeRemaining_ = rhs.timeRemaining_;
  defaultDual_ = rhs.defaultDual_;
  solver_ = rhs.solver_;
  numberColumns_ = rhs.numberColumns_;
  lower_ = rhs.lower_;
  owningSolution_ = rhs.owningSolution_;
  if (owningSolution_)
    solution_ = CoinCopyOfArray(rhs.solution_,numberColumns_);
  else
    solution_ = rhs.solution_;
  upper_ = rhs.upper_;
  hotstartSolution_ = rhs.hotstartSolution_;
  pi_ = rhs.pi_;
  rowActivity_ = rhs.rowActivity_;
  objective_ = rhs.objective_;
  rowLower_ = rhs.rowLower_;
  rowUpper_ = rhs.rowUpper_;
  elementByColumn_ = rhs.elementByColumn_;
  row_ = rhs.row_;
  columnStart_ = rhs.columnStart_;
  columnLength_ = rhs.columnLength_;
  usefulRegion_ = rhs.usefulRegion_;
  assert (!usefulRegion_);
  indexRegion_ = rhs.indexRegion_;
  numberSolutions_ = rhs.numberSolutions_;
  numberBranchingSolutions_ = rhs.numberBranchingSolutions_;
  depth_ = rhs.depth_;
}

// Clone
OsiBranchingInformation *
OsiBranchingInformation::clone() const
{
  return new OsiBranchingInformation(*this);
}

// Assignment operator 
OsiBranchingInformation & 
OsiBranchingInformation::operator=( const OsiBranchingInformation& rhs)
{
  if (this!=&rhs) {
    objectiveValue_ = rhs.objectiveValue_;
    cutoff_ = rhs.cutoff_;
    direction_ = rhs.direction_;
    integerTolerance_ = rhs.integerTolerance_;
    primalTolerance_ = rhs.primalTolerance_;
    timeRemaining_ = rhs.timeRemaining_;
    defaultDual_ = rhs.defaultDual_;
    numberColumns_ = rhs.numberColumns_;
    lower_ = rhs.lower_;
    owningSolution_ = rhs.owningSolution_;
    if (owningSolution_) {
      solution_ = CoinCopyOfArray(rhs.solution_,numberColumns_);
      delete [] solution_;
    } else {
      solution_ = rhs.solution_;
    }
    upper_ = rhs.upper_;
    hotstartSolution_ = rhs.hotstartSolution_;
    pi_ = rhs.pi_;
    rowActivity_ = rhs.rowActivity_;
    objective_ = rhs.objective_;
    rowLower_ = rhs.rowLower_;
    rowUpper_ = rhs.rowUpper_;
    elementByColumn_ = rhs.elementByColumn_;
    row_ = rhs.row_;
    columnStart_ = rhs.columnStart_;
    columnLength_ = rhs.columnLength_;
    usefulRegion_ = rhs.usefulRegion_;
    assert (!usefulRegion_);
    indexRegion_ = rhs.indexRegion_;
    numberSolutions_ = rhs.numberSolutions_;
    numberBranchingSolutions_ = rhs.numberBranchingSolutions_;
    depth_ = rhs.depth_;
  }
  return *this;
}

// Destructor 
OsiBranchingInformation::~OsiBranchingInformation ()
{
  if (owningSolution_) 
    delete[] solution_;
}
// Default Constructor 
OsiTwoWayBranchingObject::OsiTwoWayBranchingObject()
  :OsiBranchingObject()
{
  firstBranch_=0;
}

// Useful constructor
OsiTwoWayBranchingObject::OsiTwoWayBranchingObject (OsiSolverInterface * solver, 
						      const OsiObject * object,
						      int way , double value)
  :OsiBranchingObject(solver,value)
{
  originalObject_ = object;
  firstBranch_=way;
}
  

// Copy constructor 
OsiTwoWayBranchingObject::OsiTwoWayBranchingObject ( const OsiTwoWayBranchingObject & rhs) :OsiBranchingObject(rhs)
{
  firstBranch_=rhs.firstBranch_;
}

// Assignment operator 
OsiTwoWayBranchingObject & 
OsiTwoWayBranchingObject::operator=( const OsiTwoWayBranchingObject& rhs)
{
  if (this != &rhs) {
    OsiBranchingObject::operator=(rhs);
    firstBranch_=rhs.firstBranch_;
  }
  return *this;
}

// Destructor 
OsiTwoWayBranchingObject::~OsiTwoWayBranchingObject ()
{
}

/********* Simple Integers *******************************/
/** Default Constructor

  Equivalent to an unspecified binary variable.
*/
OsiSimpleInteger::OsiSimpleInteger ()
  : OsiObject2(),
    originalLower_(0.0),
    originalUpper_(1.0),
    columnNumber_(-1)
{
}

/** Useful constructor

  Loads actual upper & lower bounds for the specified variable.
*/
OsiSimpleInteger::OsiSimpleInteger (const OsiSolverInterface * solver, int iColumn)
  : OsiObject2()
{
  columnNumber_ = iColumn ;
  originalLower_ = solver->getColLower()[columnNumber_] ;
  originalUpper_ = solver->getColUpper()[columnNumber_] ;
}

  
// Useful constructor - passed solver index and original bounds
OsiSimpleInteger::OsiSimpleInteger ( int iColumn, double lower, double upper)
  : OsiObject2()
{
  columnNumber_ = iColumn ;
  originalLower_ = lower;
  originalUpper_ = upper;
}

// Copy constructor 
OsiSimpleInteger::OsiSimpleInteger ( const OsiSimpleInteger & rhs)
  :OsiObject2(rhs)

{
  columnNumber_ = rhs.columnNumber_;
  originalLower_ = rhs.originalLower_;
  originalUpper_ = rhs.originalUpper_;
}

// Clone
OsiObject *
OsiSimpleInteger::clone() const
{
  return new OsiSimpleInteger(*this);
}

// Assignment operator 
OsiSimpleInteger & 
OsiSimpleInteger::operator=( const OsiSimpleInteger& rhs)
{
  if (this!=&rhs) {
    OsiObject2::operator=(rhs);
    columnNumber_ = rhs.columnNumber_;
    originalLower_ = rhs.originalLower_;
    originalUpper_ = rhs.originalUpper_;
  }
  return *this;
}

// Destructor 
OsiSimpleInteger::~OsiSimpleInteger ()
{
}
/* Reset variable bounds to their original values.
   
Bounds may be tightened, so it may be good to be able to reset them to
their original values.
*/
void 
OsiSimpleInteger::resetBounds(const OsiSolverInterface * solver) 
{
  originalLower_ = solver->getColLower()[columnNumber_] ;
  originalUpper_ = solver->getColUpper()[columnNumber_] ;
}
// Redoes data when sequence numbers change
void 
OsiSimpleInteger::resetSequenceEtc(int numberColumns, const int * originalColumns)
{
  int i;
  for (i=0;i<numberColumns;i++) {
    if (originalColumns[i]==columnNumber_)
      break;
  }
  if (i<numberColumns)
    columnNumber_=i;
  else
    abort(); // should never happen
}

// Infeasibility - large is 0.5
double 
OsiSimpleInteger::infeasibility(const OsiBranchingInformation * info, int & whichWay) const
{
  double value = info->solution_[columnNumber_];
  value = CoinMax(value, info->lower_[columnNumber_]);
  value = CoinMin(value, info->upper_[columnNumber_]);
  double nearest = floor(value+(1.0-0.5));
  if (nearest>value) { 
    whichWay=1;
  } else {
    whichWay=0;
  }
  infeasibility_ = fabs(value-nearest);
  double returnValue = infeasibility_;
  if (infeasibility_<=info->integerTolerance_) {
    otherInfeasibility_ = 1.0;
    returnValue = 0.0;
  } else if (info->defaultDual_<0.0) {
    otherInfeasibility_ = 1.0-infeasibility_;
  } else {
    const double * pi = info->pi_;
    const double * activity = info->rowActivity_;
    const double * lower = info->rowLower_;
    const double * upper = info->rowUpper_;
    const double * element = info->elementByColumn_;
    const int * row = info->row_;
    const CoinBigIndex * columnStart = info->columnStart_;
    const int * columnLength = info->columnLength_;
    double direction = info->direction_;
    double downMovement = value - floor(value);
    double upMovement = 1.0-downMovement;
    double valueP = info->objective_[columnNumber_]*direction;
    CoinBigIndex start = columnStart[columnNumber_];
    CoinBigIndex end = start + columnLength[columnNumber_];
    double upEstimate = 0.0;
    double downEstimate = 0.0;
    if (valueP>0.0)
      upEstimate = valueP*upMovement;
    else
      downEstimate -= valueP*downMovement;
    double tolerance = info->primalTolerance_;
    for (CoinBigIndex j=start;j<end;j++) {
      int iRow = row[j];
      if (lower[iRow]<-1.0e20) 
	assert (pi[iRow]<=1.0e-4);
      if (upper[iRow]>1.0e20) 
	assert (pi[iRow]>=-1.0e-4);
      valueP = pi[iRow]*direction;
      double el2 = element[j];
      double value2 = valueP*el2;
      double u=0.0;
      double d=0.0;
      if (value2>0.0)
	u = value2;
      else
	d = -value2;
      // if up makes infeasible then make at least default
      double newUp = activity[iRow] + upMovement*el2;
      if (newUp>upper[iRow]+tolerance||newUp<lower[iRow]-tolerance)
	u = CoinMax(u,info->defaultDual_);
      upEstimate += u*upMovement;
      // if down makes infeasible then make at least default
      double newDown = activity[iRow] - downMovement*el2;
      if (newDown>upper[iRow]+tolerance||newDown<lower[iRow]-tolerance)
	d = CoinMax(d,info->defaultDual_);
      downEstimate += d*downMovement;
    }
    if (downEstimate>=upEstimate) {
      infeasibility_ = CoinMax(1.0e-12,upEstimate);
      otherInfeasibility_ = CoinMax(1.0e-12,downEstimate);
      whichWay = 1;
    } else {
      infeasibility_ = CoinMax(1.0e-12,downEstimate);
      otherInfeasibility_ = CoinMax(1.0e-12,upEstimate);
      whichWay = 0;
    }
    returnValue = infeasibility_;
  }
  if (preferredWay_>=0&&returnValue)
    whichWay = preferredWay_;
  whichWay_ = static_cast<short int>(whichWay) ;
  return returnValue;
}

// This looks at solution and sets bounds to contain solution
/** More precisely: it first forces the variable within the existing
    bounds, and then tightens the bounds to fix the variable at the
    nearest integer value.
*/
double
OsiSimpleInteger::feasibleRegion(OsiSolverInterface * solver,
				 const OsiBranchingInformation * info) const
{
  double value = info->solution_[columnNumber_];
  double newValue = CoinMax(value, info->lower_[columnNumber_]);
  newValue = CoinMin(newValue, info->upper_[columnNumber_]);
  newValue = floor(newValue+0.5);
  solver->setColLower(columnNumber_,newValue);
  solver->setColUpper(columnNumber_,newValue);
  return fabs(value-newValue);
}
/* Column number if single column object -1 otherwise,
   so returns >= 0
   Used by heuristics
*/
int 
OsiSimpleInteger::columnNumber() const
{
  return columnNumber_;
}
// Creates a branching object
OsiBranchingObject * 
OsiSimpleInteger::createBranch(OsiSolverInterface * solver, const OsiBranchingInformation * info, int way) const 
{
  double value = info->solution_[columnNumber_];
  value = CoinMax(value, info->lower_[columnNumber_]);
  value = CoinMin(value, info->upper_[columnNumber_]);
  assert (info->upper_[columnNumber_]>info->lower_[columnNumber_]);
#ifndef NDEBUG
  double nearest = floor(value+0.5);
  assert (fabs(value-nearest)>info->integerTolerance_);
#endif
  OsiBranchingObject * branch = new OsiIntegerBranchingObject(solver,this,way,
					     value);
  return branch;
}
// Return "down" estimate
double 
OsiSimpleInteger::downEstimate() const
{
  if (whichWay_)
    return 1.0-infeasibility_;
  else
    return infeasibility_;
}
// Return "up" estimate
double 
OsiSimpleInteger::upEstimate() const
{
  if (!whichWay_)
    return 1.0-infeasibility_;
  else
    return infeasibility_;
}

// Default Constructor 
OsiIntegerBranchingObject::OsiIntegerBranchingObject()
  :OsiTwoWayBranchingObject()
{
  down_[0] = 0.0;
  down_[1] = 0.0;
  up_[0] = 0.0;
  up_[1] = 0.0;
}

// Useful constructor
OsiIntegerBranchingObject::OsiIntegerBranchingObject (OsiSolverInterface * solver, 
						      const OsiSimpleInteger * object,
						      int way , double value)
  :OsiTwoWayBranchingObject(solver,object, way, value)
{
  int iColumn = object->columnNumber();
  down_[0] = solver->getColLower()[iColumn];
  down_[1] = floor(value_);
  up_[0] = ceil(value_);
  up_[1] = solver->getColUpper()[iColumn];
}
/* Create a standard floor/ceiling branch object
   Specifies a simple two-way branch in a more flexible way. One arm of the
   branch will be lb <= x <= downUpperBound, the other upLowerBound <= x <= ub.
   Specify way = -1 to set the object state to perform the down arm first,
   way = 1 for the up arm.
*/
OsiIntegerBranchingObject::OsiIntegerBranchingObject (OsiSolverInterface * solver, 
						      const OsiSimpleInteger * object,
						      int way , double value, double downUpperBound, 
						      double upLowerBound) 
  :OsiTwoWayBranchingObject(solver,object, way, value)
{
  int iColumn = object->columnNumber();
  down_[0] = solver->getColLower()[iColumn];
  down_[1] = downUpperBound;
  up_[0] = upLowerBound;
  up_[1] = solver->getColUpper()[iColumn];
}
  

// Copy constructor 
OsiIntegerBranchingObject::OsiIntegerBranchingObject ( const OsiIntegerBranchingObject & rhs) :OsiTwoWayBranchingObject(rhs)
{
  down_[0] = rhs.down_[0];
  down_[1] = rhs.down_[1];
  up_[0] = rhs.up_[0];
  up_[1] = rhs.up_[1];
}

// Assignment operator 
OsiIntegerBranchingObject & 
OsiIntegerBranchingObject::operator=( const OsiIntegerBranchingObject& rhs)
{
  if (this != &rhs) {
    OsiTwoWayBranchingObject::operator=(rhs);
    down_[0] = rhs.down_[0];
    down_[1] = rhs.down_[1];
    up_[0] = rhs.up_[0];
    up_[1] = rhs.up_[1];
  }
  return *this;
}
OsiBranchingObject * 
OsiIntegerBranchingObject::clone() const
{ 
  return (new OsiIntegerBranchingObject(*this));
}


// Destructor 
OsiIntegerBranchingObject::~OsiIntegerBranchingObject ()
{
}

/*
  Perform a branch by adjusting the bounds of the specified variable. Note
  that each arm of the branch advances the object to the next arm by
  advancing the value of branchIndex_.

  Providing new values for the variable's lower and upper bounds for each
  branching direction gives a little bit of additional flexibility and will
  be easily extensible to multi-way branching.
  Returns change in guessed objective on next branch
*/
double
OsiIntegerBranchingObject::branch(OsiSolverInterface * solver)
{
  const OsiSimpleInteger * obj =
    dynamic_cast <const OsiSimpleInteger *>(originalObject_) ;
  assert (obj);
  int iColumn = obj->columnNumber();
  double olb,oub ;
  olb = solver->getColLower()[iColumn] ;
  oub = solver->getColUpper()[iColumn] ;
  int way = (!branchIndex_) ? (2*firstBranch_-1) : -(2*firstBranch_-1);
  if (0) {
    printf("branching %s on %d bounds %g %g / %g %g\n",
	   (way==-1) ? "down" :"up",iColumn,
	   down_[0],down_[1],up_[0],up_[1]);
    const double * lower = solver->getColLower();
    const double * upper = solver->getColUpper();
    for (int i=0;i<8;i++) 
      printf(" [%d (%g,%g)]",i,lower[i],upper[i]);
    printf("\n");
  }
  if (way<0) {
#ifdef OSI_DEBUG
  { double olb,oub ;
    olb = solver->getColLower()[iColumn] ;
    oub = solver->getColUpper()[iColumn] ;
    printf("branching down on var %d: [%g,%g] => [%g,%g]\n",
	   iColumn,olb,oub,down_[0],down_[1]) ; }
#endif
    solver->setColLower(iColumn,down_[0]);
    solver->setColUpper(iColumn,down_[1]);
  } else {
#ifdef OSI_DEBUG
  { double olb,oub ;
    olb = solver->getColLower()[iColumn] ;
    oub = solver->getColUpper()[iColumn] ;
    printf("branching up on var %d: [%g,%g] => [%g,%g]\n",
	   iColumn,olb,oub,up_[0],up_[1]) ; }
#endif
    solver->setColLower(iColumn,up_[0]);
    solver->setColUpper(iColumn,up_[1]);
  }
  double nlb = solver->getColLower()[iColumn];
  if (nlb<olb) {
#ifndef NDEBUG
    printf("bad lb change for column %d from %g to %g\n",iColumn,olb,nlb);
#endif
    solver->setColLower(iColumn,olb);
  }
  double nub = solver->getColUpper()[iColumn];
  if (nub>oub) {
#ifndef NDEBUG
    printf("bad ub change for column %d from %g to %g\n",iColumn,oub,nub);
#endif
    solver->setColUpper(iColumn,oub);
  }
#ifndef NDEBUG
  if (nlb<olb+1.0e-8&&nub>oub-1.0e-8)
    printf("bad null change for column %d - bounds %g,%g\n",iColumn,olb,oub);
#endif
  branchIndex_++;
  return 0.0;
}
// Print what would happen  
void
OsiIntegerBranchingObject::print(const OsiSolverInterface * solver)
{
  const OsiSimpleInteger * obj =
    dynamic_cast <const OsiSimpleInteger *>(originalObject_) ;
  assert (obj);
  int iColumn = obj->columnNumber();
  int way = (!branchIndex_) ? (2*firstBranch_-1) : -(2*firstBranch_-1);
  if (way<0) {
  { double olb,oub ;
    olb = solver->getColLower()[iColumn] ;
    oub = solver->getColUpper()[iColumn] ;
    printf("OsiInteger would branch down on var %d : [%g,%g] => [%g,%g]\n",
	   iColumn,olb,oub,down_[0],down_[1]) ; }
  } else {
  { double olb,oub ;
    olb = solver->getColLower()[iColumn] ;
    oub = solver->getColUpper()[iColumn] ;
    printf("OsiInteger would branch up on var %d : [%g,%g] => [%g,%g]\n",
	   iColumn,olb,oub,up_[0],up_[1]) ; }
  }
}
// Default Constructor 
OsiSOS::OsiSOS ()
  : OsiObject2(),
    members_(NULL),
    weights_(NULL),
    numberMembers_(0),
    sosType_(-1),
    integerValued_(false)
{
}

// Useful constructor (which are indices)
OsiSOS::OsiSOS (const OsiSolverInterface * ,  int numberMembers,
	   const int * which, const double * weights, int type)
  : OsiObject2(),
    numberMembers_(numberMembers),
    sosType_(type)
{
  integerValued_ = type==1; // not strictly true - should check problem
  if (numberMembers_) {
    members_ = new int[numberMembers_];
    weights_ = new double[numberMembers_];
    memcpy(members_,which,numberMembers_*sizeof(int));
    if (weights) {
      memcpy(weights_,weights,numberMembers_*sizeof(double));
    } else {
      for (int i=0;i<numberMembers_;i++)
        weights_[i]=i;
    }
    // sort so weights increasing
    CoinSort_2(weights_,weights_+numberMembers_,members_);
    double last = -COIN_DBL_MAX;
    int i;
    for (i=0;i<numberMembers_;i++) {
      double possible = CoinMax(last+1.0e-10,weights_[i]);
      weights_[i] = possible;
      last=possible;
    }
  } else {
    members_ = NULL;
    weights_ = NULL;
  }
  assert (sosType_>0&&sosType_<3);
}

// Copy constructor 
OsiSOS::OsiSOS ( const OsiSOS & rhs)
  :OsiObject2(rhs)
{
  numberMembers_ = rhs.numberMembers_;
  sosType_ = rhs.sosType_;
  integerValued_ = rhs.integerValued_;
  if (numberMembers_) {
    members_ = new int[numberMembers_];
    weights_ = new double[numberMembers_];
    memcpy(members_,rhs.members_,numberMembers_*sizeof(int));
    memcpy(weights_,rhs.weights_,numberMembers_*sizeof(double));
  } else {
    members_ = NULL;
    weights_ = NULL;
  }
}

// Clone
OsiObject *
OsiSOS::clone() const
{
  return new OsiSOS(*this);
}

// Assignment operator 
OsiSOS & 
OsiSOS::operator=( const OsiSOS& rhs)
{
  if (this!=&rhs) {
    OsiObject2::operator=(rhs);
    delete [] members_;
    delete [] weights_;
    numberMembers_ = rhs.numberMembers_;
    sosType_ = rhs.sosType_;
    integerValued_ = rhs.integerValued_;
    if (numberMembers_) {
      members_ = new int[numberMembers_];
      weights_ = new double[numberMembers_];
      memcpy(members_,rhs.members_,numberMembers_*sizeof(int));
      memcpy(weights_,rhs.weights_,numberMembers_*sizeof(double));
    } else {
      members_ = NULL;
      weights_ = NULL;
    }
  }
  return *this;
}

// Destructor 
OsiSOS::~OsiSOS ()
{
  delete [] members_;
  delete [] weights_;
}

// Infeasibility - large is 0.5
double 
OsiSOS::infeasibility(const OsiBranchingInformation * info,int & whichWay) const
{
  int j;
  int firstNonZero=-1;
  int lastNonZero = -1;
  int firstNonFixed=-1;
  int lastNonFixed = -1;
  const double * solution = info->solution_;
  //const double * lower = info->lower_;
  const double * upper = info->upper_;
  //double largestValue=0.0;
  double integerTolerance = info->integerTolerance_;
  double primalTolerance = info->primalTolerance_;
  double weight = 0.0;
  double sum =0.0;

  // check bounds etc
  double lastWeight=-1.0e100;
  for (j=0;j<numberMembers_;j++) {
    int iColumn = members_[j];
    if (lastWeight>=weights_[j]-1.0e-12)
      throw CoinError("Weights too close together in SOS","infeasibility","OsiSOS");
    lastWeight = weights_[j];
    if (upper[iColumn]) {
      double value = CoinMax(0.0,solution[iColumn]);
      if (value>integerTolerance) {
	// Possibly due to scaling a fixed variable might slip through
#ifdef COIN_DEVELOP
	if (value>upper[iColumn]+10.0*primalTolerance)
	  printf("** Variable %d (%d) has value %g and upper bound of %g\n",
		 iColumn,j,value,upper[iColumn]);
#endif
	if (value>upper[iColumn]) {
	  value=upper[iColumn];
	} 
	sum += value;
	weight += weights_[j]*value;
	if (firstNonZero<0)
	  firstNonZero=j;
	lastNonZero=j;
      }
      if (firstNonFixed<0)
	firstNonFixed=j;
      lastNonFixed=j;
    }
  }
  whichWay=1;
  whichWay_=1;
  if (lastNonZero-firstNonZero>=sosType_) {
    // find where to branch
    assert (sum>0.0);
    // probably best to use pseudo duals
    double value = lastNonZero-firstNonZero+1;
    value *= 0.5/static_cast<double> (numberMembers_);
    infeasibility_=value;
    otherInfeasibility_=1.0-value;
    if (info->defaultDual_>=0.0) {
      // Using pseudo shadow prices
      weight /= sum;
      int iWhere;
      for (iWhere=firstNonZero;iWhere<lastNonZero;iWhere++) 
	if (weight<weights_[iWhere+1])
	  break;
      assert (iWhere!=lastNonZero);
      /* Complicated - infeasibility is being used for branching so we
	 don't want estimate of satisfying set but of each way on branch.
	 So let us suppose that all on side being fixed to 0 goes to closest
      */
      int lastDown=iWhere;
      int firstUp=iWhere+1;
      if (sosType_==2) {
	// SOS 2 - choose nearest
	if (weight-weights_[iWhere]>=weights_[iWhere+1]-weight)
	  lastDown++;
	// But make sure OK
	if (lastDown==firstNonFixed) {
	  lastDown ++;
	} else if (lastDown==lastNonFixed) {
	  lastDown --;
	} 
	firstUp=lastDown;
      }
      // Now get current contribution and compute weight for end points
      double weightDown = 0.0;
      double weightUp = 0.0;
      const double * element = info->elementByColumn_;
      const int * row = info->row_;
      const CoinBigIndex * columnStart = info->columnStart_;
      const int * columnLength = info->columnLength_;
      double direction = info->direction_;
      const double * objective = info->objective_;
      // Compute where we would move to
      double objValue=0.0;
      double * useful = info->usefulRegion_;
      int * index = info->indexRegion_;
      int n=0;
      for (j=firstNonZero;j<=lastNonZero;j++) {
	int iColumn = members_[j];
	double multiplier = solution[iColumn];
	if (j>=lastDown)
	  weightDown += multiplier;
	if (j<=firstUp)
	  weightUp += multiplier;
	if (multiplier>0.0) {
	  objValue += objective[iColumn]*multiplier;
	  CoinBigIndex start = columnStart[iColumn];
	  CoinBigIndex end = start + columnLength[iColumn];
	  for (CoinBigIndex j=start;j<end;j++) {
	    int iRow = row[j];
	    double value = element[j]*multiplier;
	    if (useful[iRow]) {
	      value += useful[iRow];
	      if (!value)
		value = 1.0e-100;
	    } else {
	      assert (value);
	      index[n++]=iRow;
	    }
	    useful[iRow] = value;
	  }
	}
      }
      if (sosType_==2) 
	assert (fabs(weightUp+weightDown-sum-solution[members_[lastDown]])<1.0e-4);
      int startX[2];
      int endX[2];
      startX[0]=firstNonZero;
      startX[1]=firstUp;
      endX[0]=lastDown;
      endX[1]=lastNonZero;
      double fakeSolution[2];
      int check[2];
      fakeSolution[0]=weightDown;
      check[0]=members_[lastDown];
      fakeSolution[1]=weightUp;
      check[1]=members_[firstUp];
      const double * pi = info->pi_;
      const double * activity = info->rowActivity_;
      const double * lower = info->rowLower_;
      const double * upper = info->rowUpper_;
      int numberRows = info->solver_->getNumRows();
      double * useful2 = useful+numberRows;
      int * index2 = index+numberRows;
      for (int i=0;i<2;i++) {
	double obj=0.0;
	int n2=0;
	for (j=startX[i];j<=endX[i];j++) {
	  int iColumn = members_[j];
	  double multiplier = solution[iColumn];
	  if (iColumn==check[i])
	    multiplier=fakeSolution[i];
	  if (multiplier>0.0) {
	    obj += objective[iColumn]*multiplier;
	    CoinBigIndex start = columnStart[iColumn];
	    CoinBigIndex end = start + columnLength[iColumn];
	    for (CoinBigIndex j=start;j<end;j++) {
	      int iRow = row[j];
	      double value = element[j]*multiplier;
	      if (useful2[iRow]) {
		value += useful2[iRow];
		if (!value)
		  value = 1.0e-100;
	      } else {
		assert (value);
		index2[n2++]=iRow;
	      }
	      useful2[iRow] = value;
	    }
	  }
	}
	// movement in objective
	obj = (obj-objValue) * direction;
	double estimate = (obj>0.0) ? obj : 0.0;
	for (j=0;j<n;j++) {
	  int iRow = index[j];
	  // movement
	  double movement = useful2[iRow]-useful[iRow];
	  useful[iRow]=0.0;
	  useful2[iRow]=0.0;
	  double valueP = pi[iRow]*direction;
	  if (lower[iRow]<-1.0e20) 
	    assert (valueP<=1.0e-4);
	  if (upper[iRow]>1.0e20) 
	    assert (valueP>=-1.0e-4);
	  double value2 = valueP*movement;
	  double thisEstimate = (value2>0.0) ? value2 : 0;
	  // if makes infeasible then make at least default
	  double newValue = activity[iRow] + movement;
	  if (newValue>upper[iRow]+primalTolerance||newValue<lower[iRow]-primalTolerance)
	    thisEstimate = CoinMax(thisEstimate,info->defaultDual_);
	  estimate += thisEstimate;
	}
	for (j=0;j<n2;j++) {
	  int iRow = index2[j];
	  // movement
	  double movement = useful2[iRow]-useful[iRow];
	  useful[iRow]=0.0;
	  useful2[iRow]=0.0;
	  if (movement) {
	    double valueP = pi[iRow]*direction;
	    if (lower[iRow]<-1.0e20) 
	      assert (valueP<=1.0e-4);
	    if (upper[iRow]>1.0e20) 
	      assert (valueP>=-1.0e-4);
	    double value2 = valueP*movement;
	    double thisEstimate = (value2>0.0) ? value2 : 0;
	    // if makes infeasible then make at least default
	    double newValue = activity[iRow] + movement;
	    if (newValue>upper[iRow]+primalTolerance||newValue<lower[iRow]-primalTolerance)
	      thisEstimate = CoinMax(thisEstimate,info->defaultDual_);
	    estimate += thisEstimate;
	  }
	}
	// store in fakeSolution
	fakeSolution[i]=estimate;
      }
      double downEstimate = fakeSolution[0];
      double upEstimate = fakeSolution[1];
      if (downEstimate>=upEstimate) {
	infeasibility_ = CoinMax(1.0e-12,upEstimate);
	otherInfeasibility_ = CoinMax(1.0e-12,downEstimate);
	whichWay = 1;
      } else {
	infeasibility_ = CoinMax(1.0e-12,downEstimate);
	otherInfeasibility_ = CoinMax(1.0e-12,upEstimate);
	whichWay = 0;
      }
      whichWay_=static_cast<short>(whichWay);
      value=infeasibility_;
    }
    return value;
  } else {
    infeasibility_=0.0;
    otherInfeasibility_=1.0;
    return 0.0; // satisfied
  }
}

// This looks at solution and sets bounds to contain solution
double
OsiSOS::feasibleRegion(OsiSolverInterface * solver, const OsiBranchingInformation * info) const
{
  int j;
  int firstNonZero=-1;
  int lastNonZero = -1;
  const double * solution = info->solution_;
  //const double * lower = info->lower_;
  const double * upper = info->upper_;
  double sum =0.0;
  // Find largest one or pair
  double movement=0.0;
  if (sosType_==1) {
    for (j=0;j<numberMembers_;j++) {
      int iColumn = members_[j];
      double value = CoinMax(0.0,solution[iColumn]);
      if (value>sum&&upper[iColumn]) {
	firstNonZero=j;
	sum=value;
      }
    }
    lastNonZero=firstNonZero;
  } else {
    // type 2
    for (j=1;j<numberMembers_;j++) {
      int iColumn = members_[j];
      int jColumn = members_[j-1];
      double value1 = CoinMax(0.0,solution[iColumn]);
      double value0 = CoinMax(0.0,solution[jColumn]);
      double value = value0+value1;
      if (value>sum) {
	if (upper[iColumn]||upper[jColumn]) {
	  firstNonZero=upper[jColumn] ? j-1 : j;
	  lastNonZero=upper[iColumn] ? j : j-1;
	  sum=value;
	}
      }
    }
  }
  for (j=0;j<numberMembers_;j++) {
    if (j<firstNonZero||j>lastNonZero) {
      int iColumn = members_[j];
      double value = CoinMax(0.0,solution[iColumn]);
      movement += value;
      solver->setColUpper(iColumn,0.0);
    }
  }
  return movement;
}
// Redoes data when sequence numbers change
void 
OsiSOS::resetSequenceEtc(int numberColumns, const int * originalColumns)
{
  int n2=0;
  for (int j=0;j<numberMembers_;j++) {
    int iColumn = members_[j];
    int i;
    for (i=0;i<numberColumns;i++) {
      if (originalColumns[i]==iColumn)
        break;
    }
    if (i<numberColumns) {
      members_[n2]=i;
      weights_[n2++]=weights_[j];
    }
  }
  if (n2<numberMembers_) {
    printf("** SOS number of members reduced from %d to %d!\n",numberMembers_,n2);
    numberMembers_=n2;
  }
}
// Return "down" estimate
double 
OsiSOS::downEstimate() const
{
  if (whichWay_)
    return otherInfeasibility_;
  else
    return infeasibility_;
}
// Return "up" estimate
double 
OsiSOS::upEstimate() const
{
  if (!whichWay_)
    return otherInfeasibility_;
  else
    return infeasibility_;
}

// Creates a branching object
OsiBranchingObject * 
OsiSOS::createBranch(OsiSolverInterface * solver, const OsiBranchingInformation * info, int way) const
{
  int j;
  const double * solution = info->solution_;
  double tolerance = info->primalTolerance_;
  const double * upper = info->upper_;
  int firstNonFixed=-1;
  int lastNonFixed=-1;
  int firstNonZero=-1;
  int lastNonZero = -1;
  double weight = 0.0;
  double sum =0.0;
  for (j=0;j<numberMembers_;j++) {
    int iColumn = members_[j];
    if (upper[iColumn]) {
      double value = CoinMax(0.0,solution[iColumn]);
      sum += value;
      if (firstNonFixed<0)
	firstNonFixed=j;
      lastNonFixed=j;
      if (value>tolerance) {
	weight += weights_[j]*value;
	if (firstNonZero<0)
	  firstNonZero=j;
	lastNonZero=j;
      }
    }
  }
  assert (lastNonZero-firstNonZero>=sosType_) ;
  // find where to branch
  assert (sum>0.0);
  weight /= sum;
  int iWhere;
  double separator=0.0;
  for (iWhere=firstNonZero;iWhere<lastNonZero;iWhere++) 
    if (weight<weights_[iWhere+1])
      break;
  if (sosType_==1) {
    // SOS 1
    separator = 0.5 *(weights_[iWhere]+weights_[iWhere+1]);
  } else {
    // SOS 2
    if (iWhere==lastNonFixed-1)
      iWhere = lastNonFixed-2;
    separator = weights_[iWhere+1];
  }
  // create object
  OsiBranchingObject * branch;
  branch = new OsiSOSBranchingObject(solver,this,way,separator);
  return branch;
}
// Default Constructor 
OsiSOSBranchingObject::OsiSOSBranchingObject()
  :OsiTwoWayBranchingObject()
{
}

// Useful constructor
OsiSOSBranchingObject::OsiSOSBranchingObject (OsiSolverInterface * solver,
					      const OsiSOS * set,
					      int way ,
					      double separator)
  :OsiTwoWayBranchingObject(solver, set,way,separator)
{
}

// Copy constructor 
OsiSOSBranchingObject::OsiSOSBranchingObject ( const OsiSOSBranchingObject & rhs) :OsiTwoWayBranchingObject(rhs)
{
}

// Assignment operator 
OsiSOSBranchingObject & 
OsiSOSBranchingObject::operator=( const OsiSOSBranchingObject& rhs)
{
  if (this != &rhs) {
    OsiTwoWayBranchingObject::operator=(rhs);
  }
  return *this;
}
OsiBranchingObject * 
OsiSOSBranchingObject::clone() const
{ 
  return (new OsiSOSBranchingObject(*this));
}


// Destructor 
OsiSOSBranchingObject::~OsiSOSBranchingObject ()
{
}
double
OsiSOSBranchingObject::branch(OsiSolverInterface * solver)
{
  const OsiSOS * set =
    dynamic_cast <const OsiSOS *>(originalObject_) ;
  assert (set);
  int way = (!branchIndex_) ? (2*firstBranch_-1) : -(2*firstBranch_-1);
  branchIndex_++;
  int numberMembers = set->numberMembers();
  const int * which = set->members();
  const double * weights = set->weights();
  //const double * lower = solver->getColLower();
  //const double * upper = solver->getColUpper();
  // *** for way - up means fix all those in down section
  if (way<0) {
    int i;
    for ( i=0;i<numberMembers;i++) {
      if (weights[i] > value_)
	break;
    }
    assert (i<numberMembers);
    for (;i<numberMembers;i++) 
      solver->setColUpper(which[i],0.0);
  } else {
    int i;
    for ( i=0;i<numberMembers;i++) {
      if (weights[i] >= value_)
	break;
      else
	solver->setColUpper(which[i],0.0);
    }
    assert (i<numberMembers);
  }
  return 0.0;
}
// Print what would happen  
void
OsiSOSBranchingObject::print(const OsiSolverInterface * solver)
{
  const OsiSOS * set =
    dynamic_cast <const OsiSOS *>(originalObject_) ;
  assert (set);
  int way = (!branchIndex_) ? (2*firstBranch_-1) : -(2*firstBranch_-1);
  int numberMembers = set->numberMembers();
  const int * which = set->members();
  const double * weights = set->weights();
  //const double * lower = solver->getColLower();
  const double * upper = solver->getColUpper();
  int first=numberMembers;
  int last=-1;
  int numberFixed=0;
  int numberOther=0;
  int i;
  for ( i=0;i<numberMembers;i++) {
    double bound = upper[which[i]];
    if (bound) {
      first = CoinMin(first,i);
      last = CoinMax(last,i);
    }
  }
  // *** for way - up means fix all those in down section
  if (way<0) {
    printf("SOS Down");
    for ( i=0;i<numberMembers;i++) {
      double bound = upper[which[i]];
      if (weights[i] > value_)
	break;
      else if (bound)
	numberOther++;
    }
    assert (i<numberMembers);
    for (;i<numberMembers;i++) {
      double bound = upper[which[i]];
      if (bound)
	numberFixed++;
    }
  } else {
    printf("SOS Up");
    for ( i=0;i<numberMembers;i++) {
      double bound = upper[which[i]];
      if (weights[i] >= value_)
	break;
      else if (bound)
	numberFixed++;
    }
    assert (i<numberMembers);
    for (;i<numberMembers;i++) {
      double bound = upper[which[i]];
      if (bound)
	numberOther++;
    }
  }
  printf(" - at %g, free range %d (%g) => %d (%g), %d would be fixed, %d other way\n",
	 value_,which[first],weights[first],which[last],weights[last],numberFixed,numberOther);
}
/** Default Constructor

*/
OsiLotsize::OsiLotsize ()
  : OsiObject2(),
    columnNumber_(-1),
    rangeType_(0),
    numberRanges_(0),
    largestGap_(0),
    bound_(NULL),
    range_(0)
{
}

/** Useful constructor

  Loads actual upper & lower bounds for the specified variable.
*/
OsiLotsize::OsiLotsize (const OsiSolverInterface * , 
				    int iColumn, int numberPoints,
			const double * points, bool range)
  : OsiObject2()
{
  assert (numberPoints>0);
  columnNumber_ = iColumn ;
  // sort ranges
  int * sort = new int[numberPoints];
  double * weight = new double [numberPoints];
  int i;
  if (range) {
    rangeType_=2;
  } else {
    rangeType_=1;
  }
  for (i=0;i<numberPoints;i++) {
    sort[i]=i;
    weight[i]=points[i*rangeType_];
  }
  CoinSort_2(weight,weight+numberPoints,sort);
  numberRanges_=1;
  largestGap_=0;
  if (rangeType_==1) {
    bound_ = new double[numberPoints+1];
    bound_[0]=weight[0];
    for (i=1;i<numberPoints;i++) {
      if (weight[i]!=weight[i-1]) 
	bound_[numberRanges_++]=weight[i];
    }
    // and for safety
    bound_[numberRanges_]=bound_[numberRanges_-1];
    for (i=1;i<numberRanges_;i++) {
      largestGap_ = CoinMax(largestGap_,bound_[i]-bound_[i-1]);
    }
  } else {
    bound_ = new double[2*numberPoints+2];
    bound_[0]=points[sort[0]*2];
    bound_[1]=points[sort[0]*2+1];
    double hi=bound_[1];
    assert (hi>=bound_[0]);
    for (i=1;i<numberPoints;i++) {
      double thisLo =points[sort[i]*2];
      double thisHi =points[sort[i]*2+1];
      assert (thisHi>=thisLo);
      if (thisLo>hi) {
	bound_[2*numberRanges_]=thisLo;
	bound_[2*numberRanges_+1]=thisHi;
	numberRanges_++;
	hi=thisHi;
      } else {
	//overlap
	hi=CoinMax(hi,thisHi);
	bound_[2*numberRanges_-1]=hi;
      }
    }
    // and for safety
    bound_[2*numberRanges_]=bound_[2*numberRanges_-2];
    bound_[2*numberRanges_+1]=bound_[2*numberRanges_-1];
    for (i=1;i<numberRanges_;i++) {
      largestGap_ = CoinMax(largestGap_,bound_[2*i]-bound_[2*i-1]);
    }
  }
  delete [] sort;
  delete [] weight;
  range_=0;
}

// Copy constructor 
OsiLotsize::OsiLotsize ( const OsiLotsize & rhs)
  :OsiObject2(rhs)

{
  columnNumber_ = rhs.columnNumber_;
  rangeType_ = rhs.rangeType_;
  numberRanges_ = rhs.numberRanges_;
  range_ = rhs.range_;
  largestGap_ = rhs.largestGap_;
  if (numberRanges_) {
    assert (rangeType_>0&&rangeType_<3);
    bound_= new double [(numberRanges_+1)*rangeType_];
    memcpy(bound_,rhs.bound_,(numberRanges_+1)*rangeType_*sizeof(double));
  } else {
    bound_=NULL;
  }
}

// Clone
OsiObject *
OsiLotsize::clone() const
{
  return new OsiLotsize(*this);
}

// Assignment operator 
OsiLotsize & 
OsiLotsize::operator=( const OsiLotsize& rhs)
{
  if (this!=&rhs) {
    OsiObject2::operator=(rhs);
    columnNumber_ = rhs.columnNumber_;
    rangeType_ = rhs.rangeType_;
    numberRanges_ = rhs.numberRanges_;
    largestGap_ = rhs.largestGap_;
    delete [] bound_;
    range_ = rhs.range_;
    if (numberRanges_) {
      assert (rangeType_>0&&rangeType_<3);
      bound_= new double [(numberRanges_+1)*rangeType_];
      memcpy(bound_,rhs.bound_,(numberRanges_+1)*rangeType_*sizeof(double));
    } else {
      bound_=NULL;
    }
  }
  return *this;
}

// Destructor 
OsiLotsize::~OsiLotsize ()
{
  delete [] bound_;
}
/* Finds range of interest so value is feasible in range range_ or infeasible 
   between hi[range_] and lo[range_+1].  Returns true if feasible.
*/
bool 
OsiLotsize::findRange(double value, double integerTolerance) const
{
  assert (range_>=0&&range_<numberRanges_+1);
  int iLo;
  int iHi;
  double infeasibility=0.0;
  if (rangeType_==1) {
    if (value<bound_[range_]-integerTolerance) {
      iLo=0;
      iHi=range_-1;
    } else if (value<bound_[range_]+integerTolerance) {
      return true;
    } else if (value<bound_[range_+1]-integerTolerance) {
      return false;
    } else {
      iLo=range_+1;
      iHi=numberRanges_-1;
    }
    // check lo and hi
    bool found=false;
    if (value>bound_[iLo]-integerTolerance&&value<bound_[iLo+1]+integerTolerance) {
      range_=iLo;
      found=true;
    } else if (value>bound_[iHi]-integerTolerance&&value<bound_[iHi+1]+integerTolerance) {
      range_=iHi;
      found=true;
    } else {
      range_ = (iLo+iHi)>>1;
    }
    //points
    while (!found) {
      if (value<bound_[range_]) {
	if (value>=bound_[range_-1]) {
	  // found
	  range_--;
	  break;
	} else {
	  iHi = range_;
	}
      } else {
	if (value<bound_[range_+1]) {
	  // found
	  break;
	} else {
	  iLo = range_;
	}
      }
      range_ = (iLo+iHi)>>1;
    }
    if (value-bound_[range_]<=bound_[range_+1]-value) {
      infeasibility = value-bound_[range_];
    } else {
      infeasibility = bound_[range_+1]-value;
      if (infeasibility<integerTolerance)
	range_++;
    }
    return (infeasibility<integerTolerance);
  } else {
    // ranges
    if (value<bound_[2*range_]-integerTolerance) {
      iLo=0;
      iHi=range_-1;
    } else if (value<bound_[2*range_+1]+integerTolerance) {
      return true;
    } else if (value<bound_[2*range_+2]-integerTolerance) {
      return false;
    } else {
      iLo=range_+1;
      iHi=numberRanges_-1;
    }
    // check lo and hi
    bool found=false;
    if (value>bound_[2*iLo]-integerTolerance&&value<bound_[2*iLo+2]-integerTolerance) {
      range_=iLo;
      found=true;
    } else if (value>=bound_[2*iHi]-integerTolerance) {
      range_=iHi;
      found=true;
    } else {
      range_ = (iLo+iHi)>>1;
    }
    //points
    while (!found) {
      if (value<bound_[2*range_]) {
	if (value>=bound_[2*range_-2]) {
	  // found
	  range_--;
	  break;
	} else {
	  iHi = range_;
	}
      } else {
	if (value<bound_[2*range_+2]) {
	  // found
	  break;
	} else {
	  iLo = range_;
	}
      }
      range_ = (iLo+iHi)>>1;
    }
    if (value>=bound_[2*range_]-integerTolerance&&value<=bound_[2*range_+1]+integerTolerance)
      infeasibility=0.0;
    else if (value-bound_[2*range_+1]<bound_[2*range_+2]-value) {
      infeasibility = value-bound_[2*range_+1];
    } else {
      infeasibility = bound_[2*range_+2]-value;
    }
    return (infeasibility<integerTolerance);
  }
}
/* Returns floor and ceiling
 */
void 
OsiLotsize::floorCeiling(double & floorLotsize, double & ceilingLotsize, double value,
			 double tolerance) const
{
  bool feasible=findRange(value,tolerance);
  if (rangeType_==1) {
    floorLotsize=bound_[range_];
    ceilingLotsize=bound_[range_+1];
    // may be able to adjust
    if (feasible&&fabs(value-floorLotsize)>fabs(value-ceilingLotsize)) {
      floorLotsize=bound_[range_+1];
      ceilingLotsize=bound_[range_+2];
    }
  } else {
    // ranges
    assert (value>=bound_[2*range_+1]);
    floorLotsize=bound_[2*range_+1];
    ceilingLotsize=bound_[2*range_+2];
  }
}

// Infeasibility - large is 0.5
double 
OsiLotsize::infeasibility(const OsiBranchingInformation * info, int & preferredWay) const
{
  const double * solution = info->solution_;
  const double * lower = info->lower_;
  const double * upper = info->upper_;
  double value = solution[columnNumber_];
  value = CoinMax(value, lower[columnNumber_]);
  value = CoinMin(value, upper[columnNumber_]);
  double integerTolerance = info->integerTolerance_;
  /*printf("%d %g %g %g %g\n",columnNumber_,value,lower[columnNumber_],
    solution[columnNumber_],upper[columnNumber_]);*/
  assert (value>=bound_[0]-integerTolerance
          &&value<=bound_[rangeType_*numberRanges_-1]+integerTolerance);
  infeasibility_=0.0;
  bool feasible = findRange(value,integerTolerance);
  if (!feasible) {
    if (rangeType_==1) {
      if (value-bound_[range_]<bound_[range_+1]-value) {
	preferredWay=-1;
	infeasibility_ = value-bound_[range_];
	otherInfeasibility_ = bound_[range_+1] - value ;
      } else {
	preferredWay=1;
	infeasibility_ = bound_[range_+1]-value;
	otherInfeasibility_ = value-bound_[range_];
      }
    } else {
      // ranges
      if (value-bound_[2*range_+1]<bound_[2*range_+2]-value) {
	preferredWay=-1;
	infeasibility_ = value-bound_[2*range_+1];
	otherInfeasibility_ = bound_[2*range_+2]-value;
      } else {
	preferredWay=1;
	infeasibility_ = bound_[2*range_+2]-value;
	otherInfeasibility_ = value-bound_[2*range_+1];
      }
    }
  } else {
    // always satisfied
    preferredWay=-1;
    otherInfeasibility_ = 1.0;
  }
  if (infeasibility_<integerTolerance)
    infeasibility_=0.0;
  else
    infeasibility_ /= largestGap_;
  return infeasibility_;
}
/* Column number if single column object -1 otherwise,
   so returns >= 0
   Used by heuristics
*/
int 
OsiLotsize::columnNumber() const
{
  return columnNumber_;
}
/* Set bounds to contain the current solution.
   More precisely, for the variable associated with this object, take the
   value given in the current solution, force it within the current bounds
   if required, then set the bounds to fix the variable at the integer
   nearest the solution value.  Returns amount it had to move variable.
*/
double 
OsiLotsize::feasibleRegion(OsiSolverInterface * solver, const OsiBranchingInformation * info) const
{
  const double * lower = solver->getColLower();
  const double * upper = solver->getColUpper();
  const double * solution = info->solution_;
  double value = solution[columnNumber_];
  value = CoinMax(value, lower[columnNumber_]);
  value = CoinMin(value, upper[columnNumber_]);
  findRange(value,info->integerTolerance_);
  double nearest;
  if (rangeType_==1) {
    nearest = bound_[range_];
    solver->setColLower(columnNumber_,nearest);
    solver->setColUpper(columnNumber_,nearest);
  } else {
    // ranges
    solver->setColLower(columnNumber_,bound_[2*range_]);
    solver->setColUpper(columnNumber_,bound_[2*range_+1]);
    if (value>bound_[2*range_+1]) 
      nearest=bound_[2*range_+1];
    else if (value<bound_[2*range_]) 
      nearest = bound_[2*range_];
    else
      nearest = value;
  }
  // Scaling may have moved it a bit
  // Lotsizing variables could be a lot larger
#ifndef NDEBUG
  assert (fabs(value-nearest)<=(100.0+10.0*fabs(nearest))*info->integerTolerance_);
#endif
  return fabs(value-nearest);
}

// Creates a branching object
// Creates a branching object
OsiBranchingObject * 
OsiLotsize::createBranch(OsiSolverInterface * solver, const OsiBranchingInformation * info, int way) const 
{
  const double * solution = info->solution_;
  const double * lower = solver->getColLower();
  const double * upper = solver->getColUpper();
  double value = solution[columnNumber_];
  value = CoinMax(value, lower[columnNumber_]);
  value = CoinMin(value, upper[columnNumber_]);
  assert (!findRange(value,info->integerTolerance_));
  return new OsiLotsizeBranchingObject(solver,this,way,
					     value);
}

  
/*
  Bounds may be tightened, so it may be good to be able to refresh the local
  copy of the original bounds.
 */
void 
OsiLotsize::resetBounds(const OsiSolverInterface * )
{
}
// Return "down" estimate
double 
OsiLotsize::downEstimate() const
{
  if (whichWay_)
    return otherInfeasibility_;
  else
    return infeasibility_;
}
// Return "up" estimate
double 
OsiLotsize::upEstimate() const
{
  if (!whichWay_)
    return otherInfeasibility_;
  else
    return infeasibility_;
}
// Redoes data when sequence numbers change
void 
OsiLotsize::resetSequenceEtc(int numberColumns, const int * originalColumns)
{
  int i;
  for (i=0;i<numberColumns;i++) {
    if (originalColumns[i]==columnNumber_)
      break;
  }
  if (i<numberColumns)
    columnNumber_=i;
  else
    abort(); // should never happen
}


// Default Constructor 
OsiLotsizeBranchingObject::OsiLotsizeBranchingObject()
  :OsiTwoWayBranchingObject()
{
  down_[0] = 0.0;
  down_[1] = 0.0;
  up_[0] = 0.0;
  up_[1] = 0.0;
}

// Useful constructor
OsiLotsizeBranchingObject::OsiLotsizeBranchingObject (OsiSolverInterface * solver, 
						      const OsiLotsize * originalObject, 
						      int way , double value)
  :OsiTwoWayBranchingObject(solver,originalObject,way,value)
{
  int iColumn = originalObject->columnNumber();
  down_[0] = solver->getColLower()[iColumn];
  double integerTolerance = solver->getIntegerTolerance();
  originalObject->floorCeiling(down_[1],up_[0],value,integerTolerance);
  up_[1] = solver->getColUpper()[iColumn];
}

// Copy constructor 
OsiLotsizeBranchingObject::OsiLotsizeBranchingObject ( const OsiLotsizeBranchingObject & rhs) :OsiTwoWayBranchingObject(rhs)
{
  down_[0] = rhs.down_[0];
  down_[1] = rhs.down_[1];
  up_[0] = rhs.up_[0];
  up_[1] = rhs.up_[1];
}

// Assignment operator 
OsiLotsizeBranchingObject & 
OsiLotsizeBranchingObject::operator=( const OsiLotsizeBranchingObject& rhs)
{
  if (this != &rhs) {
    OsiTwoWayBranchingObject::operator=(rhs);
    down_[0] = rhs.down_[0];
    down_[1] = rhs.down_[1];
    up_[0] = rhs.up_[0];
    up_[1] = rhs.up_[1];
  }
  return *this;
}
OsiBranchingObject * 
OsiLotsizeBranchingObject::clone() const
{ 
  return (new OsiLotsizeBranchingObject(*this));
}


// Destructor 
OsiLotsizeBranchingObject::~OsiLotsizeBranchingObject ()
{
}

/*
  Perform a branch by adjusting the bounds of the specified variable. Note
  that each arm of the branch advances the object to the next arm by
  advancing the value of way_.

  Providing new values for the variable's lower and upper bounds for each
  branching direction gives a little bit of additional flexibility and will
  be easily extensible to multi-way branching.
*/
double
OsiLotsizeBranchingObject::branch(OsiSolverInterface * solver)
{
  const OsiLotsize * obj =
    dynamic_cast <const OsiLotsize *>(originalObject_) ;
  assert (obj);
  int iColumn = obj->columnNumber();
  int way = (!branchIndex_) ? (2*firstBranch_-1) : -(2*firstBranch_-1);
  if (way<0) {
#ifdef OSI_DEBUG
  { double olb,oub ;
    olb = solver->getColLower()[iColumn] ;
    oub = solver->getColUpper()[iColumn] ;
    printf("branching down on var %d: [%g,%g] => [%g,%g]\n",
	   iColumn,olb,oub,down_[0],down_[1]) ; }
#endif
    solver->setColLower(iColumn,down_[0]);
    solver->setColUpper(iColumn,down_[1]);
  } else {
#ifdef OSI_DEBUG
  { double olb,oub ;
    olb = solver->getColLower()[iColumn] ;
    oub = solver->getColUpper()[iColumn] ;
    printf("branching up on var %d: [%g,%g] => [%g,%g]\n",
	   iColumn,olb,oub,up_[0],up_[1]) ; }
#endif
    solver->setColLower(iColumn,up_[0]);
    solver->setColUpper(iColumn,up_[1]);
  }
  branchIndex_++;
  return 0.0;
}
// Print
void
OsiLotsizeBranchingObject::print(const OsiSolverInterface * solver)
{
  const OsiLotsize * obj =
    dynamic_cast <const OsiLotsize *>(originalObject_) ;
  assert (obj);
  int iColumn = obj->columnNumber();
  int way = (!branchIndex_) ? (2*firstBranch_-1) : -(2*firstBranch_-1);
  if (way<0) {
  { double olb,oub ;
    olb = solver->getColLower()[iColumn] ;
    oub = solver->getColUpper()[iColumn] ;
    printf("branching down on var %d: [%g,%g] => [%g,%g]\n",
	   iColumn,olb,oub,down_[0],down_[1]) ; }
  } else {
  { double olb,oub ;
    olb = solver->getColLower()[iColumn] ;
    oub = solver->getColUpper()[iColumn] ;
    printf("branching up on var %d: [%g,%g] => [%g,%g]\n",
	   iColumn,olb,oub,up_[0],up_[1]) ; }
  }
}
  
