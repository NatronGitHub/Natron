// Copyright (C) 2006, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include <string>
#include <cassert>
#include <cfloat>
#include <cmath>
#include "CoinPragma.hpp"
#include "OsiSolverInterface.hpp"
#include "OsiAuxInfo.hpp"
#include "OsiSolverBranch.hpp"
#include "CoinWarmStartBasis.hpp"
#include "CoinPackedMatrix.hpp"
#include "CoinTime.hpp"
#include "CoinSort.hpp"
#include "CoinFinite.hpp"
#include "OsiChooseVariable.hpp"
using namespace std;

OsiChooseVariable::OsiChooseVariable() :
  goodObjectiveValue_(COIN_DBL_MAX),
  upChange_(0.0),
  downChange_(0.0),
  goodSolution_(NULL),
  list_(NULL),
  useful_(NULL),
  solver_(NULL),
  status_(-1),
  bestObjectIndex_(-1),
  bestWhichWay_(-1),
  firstForcedObjectIndex_(-1),
  firstForcedWhichWay_(-1),
  numberUnsatisfied_(0),
  numberStrong_(0),
  numberOnList_(0),
  numberStrongDone_(0),
  numberStrongIterations_(0),
  numberStrongFixed_(0),
  trustStrongForBound_(true),
  trustStrongForSolution_(true)
{
}

OsiChooseVariable::OsiChooseVariable(const OsiSolverInterface * solver) :
  goodObjectiveValue_(COIN_DBL_MAX),
  upChange_(0.0),
  downChange_(0.0),
  goodSolution_(NULL),
  solver_(solver),
  status_(-1),
  bestObjectIndex_(-1),
  bestWhichWay_(-1),
  firstForcedObjectIndex_(-1),
  firstForcedWhichWay_(-1),
  numberUnsatisfied_(0),
  numberStrong_(0),
  numberOnList_(0),
  numberStrongDone_(0),
  numberStrongIterations_(0),
  numberStrongFixed_(0),
  trustStrongForBound_(true),
  trustStrongForSolution_(true)
{
  // create useful arrays
  int numberObjects = solver_->numberObjects();
  list_ = new int [numberObjects];
  useful_ = new double [numberObjects];
}

OsiChooseVariable::OsiChooseVariable(const OsiChooseVariable & rhs) 
{  
  goodObjectiveValue_ = rhs.goodObjectiveValue_;
  upChange_ = rhs.upChange_;
  downChange_ = rhs.downChange_;
  status_ = rhs.status_;
  bestObjectIndex_ = rhs.bestObjectIndex_;
  bestWhichWay_ = rhs.bestWhichWay_;
  firstForcedObjectIndex_ = rhs.firstForcedObjectIndex_;
  firstForcedWhichWay_ = rhs.firstForcedWhichWay_;
  numberUnsatisfied_ = rhs.numberUnsatisfied_;
  numberStrong_ = rhs.numberStrong_;
  numberOnList_ = rhs.numberOnList_;
  numberStrongDone_ = rhs.numberStrongDone_;
  numberStrongIterations_ = rhs.numberStrongIterations_;
  numberStrongFixed_ = rhs.numberStrongFixed_;
  trustStrongForBound_ = rhs.trustStrongForBound_;
  trustStrongForSolution_ = rhs.trustStrongForSolution_;
  solver_ = rhs.solver_;
  if (solver_) {
    int numberObjects = solver_->numberObjects();
    int numberColumns = solver_->getNumCols();
    if (rhs.goodSolution_) {
      goodSolution_ = CoinCopyOfArray(rhs.goodSolution_,numberColumns);
    } else {
      goodSolution_ = NULL;
    }
    list_ = CoinCopyOfArray(rhs.list_,numberObjects);
    useful_ = CoinCopyOfArray(rhs.useful_,numberObjects);
  } else {
    goodSolution_ = NULL;
    list_ = NULL;
    useful_ = NULL;
  }
}

OsiChooseVariable &
OsiChooseVariable::operator=(const OsiChooseVariable & rhs)
{
  if (this != &rhs) {
    delete [] goodSolution_;
    delete [] list_;
    delete [] useful_;
    goodObjectiveValue_ = rhs.goodObjectiveValue_;
    upChange_ = rhs.upChange_;
    downChange_ = rhs.downChange_;
    status_ = rhs.status_;
    bestObjectIndex_ = rhs.bestObjectIndex_;
    bestWhichWay_ = rhs.bestWhichWay_;
    firstForcedObjectIndex_ = rhs.firstForcedObjectIndex_;
    firstForcedWhichWay_ = rhs.firstForcedWhichWay_;
    numberUnsatisfied_ = rhs.numberUnsatisfied_;
    numberStrong_ = rhs.numberStrong_;
    numberOnList_ = rhs.numberOnList_;
    numberStrongDone_ = rhs.numberStrongDone_;
    numberStrongIterations_ = rhs.numberStrongIterations_;
    numberStrongFixed_ = rhs.numberStrongFixed_;
    trustStrongForBound_ = rhs.trustStrongForBound_;
    trustStrongForSolution_ = rhs.trustStrongForSolution_;
    solver_ = rhs.solver_;
    if (solver_) {
      int numberObjects = solver_->numberObjects();
      int numberColumns = solver_->getNumCols();
      if (rhs.goodSolution_) {
	goodSolution_ = CoinCopyOfArray(rhs.goodSolution_,numberColumns);
      } else {
	goodSolution_ = NULL;
      }
      list_ = CoinCopyOfArray(rhs.list_,numberObjects);
      useful_ = CoinCopyOfArray(rhs.useful_,numberObjects);
    } else {
      goodSolution_ = NULL;
      list_ = NULL;
      useful_ = NULL;
    }
  }
  return *this;
}


OsiChooseVariable::~OsiChooseVariable ()
{
  delete [] goodSolution_;
  delete [] list_;
  delete [] useful_;
}

// Clone
OsiChooseVariable *
OsiChooseVariable::clone() const
{
  return new OsiChooseVariable(*this);
}
// Set solver and redo arrays
void 
OsiChooseVariable::setSolver (const OsiSolverInterface * solver) 
{
  solver_ = solver;
  delete [] list_;
  delete [] useful_;
  // create useful arrays
  int numberObjects = solver_->numberObjects();
  list_ = new int [numberObjects];
  useful_ = new double [numberObjects];
}


// Initialize
int 
OsiChooseVariable::setupList ( OsiBranchingInformation *info, bool initialize)
{
  if (initialize) {
    status_=-2;
    delete [] goodSolution_;
    bestObjectIndex_=-1;
    numberStrongDone_=0;
    numberStrongIterations_ = 0;
    numberStrongFixed_ = 0;
    goodSolution_ = NULL;
    goodObjectiveValue_ = COIN_DBL_MAX;
  }
  numberOnList_=0;
  numberUnsatisfied_=0;
  int numberObjects = solver_->numberObjects();
  assert (numberObjects);
  double check = 0.0;
  int checkIndex=0;
  int bestPriority=COIN_INT_MAX;
  // pretend one strong even if none
  int maximumStrong= numberStrong_ ? CoinMin(numberStrong_,numberObjects) : 1;
  int putOther = numberObjects;
  int i;
  for (i=0;i<maximumStrong;i++) {
    list_[i]=-1;
    useful_[i]=0.0;
  }
  OsiObject ** object = info->solver_->objects();
  // Say feasible
  bool feasible = true;
  for ( i=0;i<numberObjects;i++) {
    int way;
    double value = object[i]->infeasibility(info,way);
    if (value>0.0) {
      numberUnsatisfied_++;
      if (value==COIN_DBL_MAX) {
	// infeasible
	feasible=false;
	break;
      }
      int priorityLevel = object[i]->priority();
      // Better priority? Flush choices.
      if (priorityLevel<bestPriority) {
	for (int j=0;j<maximumStrong;j++) {
	  if (list_[j]>=0) {
	    int iObject = list_[j];
	    list_[j]=-1;
	    useful_[j]=0.0;
	    list_[--putOther]=iObject;
	  }
	}
	bestPriority = priorityLevel;
	check=0.0;
      } 
      if (priorityLevel==bestPriority) {
	if (value>check) {
	  //add to list
	  int iObject = list_[checkIndex];
	  if (iObject>=0)
	    list_[--putOther]=iObject;  // to end
	  list_[checkIndex]=i;
	  useful_[checkIndex]=value;
	  // find worst
	  check=COIN_DBL_MAX;
	  for (int j=0;j<maximumStrong;j++) {
	    if (list_[j]>=0) {
	      if (useful_[j]<check) {
		check=useful_[j];
		checkIndex=j;
	      }
	    } else {
	      check=0.0;
	      checkIndex = j;
	      break;
	    }
	  }
	} else {
	  // to end
	  list_[--putOther]=i;
	}
      } else {
	// to end
	list_[--putOther]=i;
      }
    }
  }
  // Get list
  numberOnList_=0;
  if (feasible) {
    for (i=0;i<maximumStrong;i++) {
      if (list_[i]>=0) {
	list_[numberOnList_]=list_[i];
	useful_[numberOnList_++]=-useful_[i];
      }
    }
    if (numberOnList_) {
      // Sort 
      CoinSort_2(useful_,useful_+numberOnList_,list_);
      // move others
      i = numberOnList_;
      for (;putOther<numberObjects;putOther++) 
	list_[i++]=list_[putOther];
      assert (i==numberUnsatisfied_);
      if (!numberStrong_)
	numberOnList_=0;
    } 
  } else {
    // not feasible
    numberUnsatisfied_=-1;
  }
  return numberUnsatisfied_;
}
/* Choose a variable
   Returns - 
   -1 Node is infeasible
   0  Normal termination - we have a candidate
   1  All looks satisfied - no candidate
   2  We can change the bound on a variable - but we also have a strong branching candidate
   3  We can change the bound on a variable - but we have a non-strong branching candidate
   4  We can change the bound on a variable - no other candidates
   We can pick up branch from whichObject() and whichWay()
   We can pick up a forced branch (can change bound) from whichForcedObject() and whichForcedWay()
   If we have a solution then we can pick up from goodObjectiveValue() and goodSolution()
*/
int 
OsiChooseVariable::chooseVariable( OsiSolverInterface * solver, OsiBranchingInformation *, bool )
{
  if (numberUnsatisfied_) {
    bestObjectIndex_=list_[0];
    bestWhichWay_ = solver->object(bestObjectIndex_)->whichWay();
    firstForcedObjectIndex_ = -1;
    firstForcedWhichWay_ =-1;
    return 0;
  } else {
    return 1;
  }
}
// Returns true if solution looks feasible against given objects
bool 
OsiChooseVariable::feasibleSolution(const OsiBranchingInformation * info,
				    const double * solution,
				    int numberObjects,
				    const OsiObject ** objects)
{
  bool satisfied=true;
  const double * saveSolution = info->solution_;
  info->solution_ = solution;
  for (int i=0;i<numberObjects;i++) {
    double value = objects[i]->checkInfeasibility(info);
    if (value>0.0) {
      satisfied=false;
      break;
    }
  }
  info->solution_ = saveSolution;
  return satisfied;
}
// Saves a good solution
void 
OsiChooseVariable::saveSolution(const OsiSolverInterface * solver)
{
  delete [] goodSolution_;
  int numberColumns = solver->getNumCols();
  goodSolution_ = CoinCopyOfArray(solver->getColSolution(),numberColumns);
  goodObjectiveValue_ = solver->getObjSense()*solver->getObjValue();
}
// Clears out good solution after use
void 
OsiChooseVariable::clearGoodSolution()
{
  delete [] goodSolution_;
  goodSolution_ = NULL;
  goodObjectiveValue_ = COIN_DBL_MAX;
}

/*  This is a utility function which does strong branching on
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
int 
OsiChooseStrong::doStrongBranching( OsiSolverInterface * solver, 
				    OsiBranchingInformation *info,
				    int numberToDo, int returnCriterion)
{

  // Might be faster to extend branch() to return bounds changed
  double * saveLower = NULL;
  double * saveUpper = NULL;
  int numberColumns = solver->getNumCols();
  solver->markHotStart();
  const double * lower = info->lower_;
  const double * upper = info->upper_;
  saveLower = CoinCopyOfArray(info->lower_,numberColumns);
  saveUpper = CoinCopyOfArray(info->upper_,numberColumns);
  numResults_=0;
  int returnCode=0;
  double timeStart = CoinCpuTime();
  for (int iDo=0;iDo<numberToDo;iDo++) {
    OsiHotInfo * result = results_ + iDo;
    // For now just 2 way
    OsiBranchingObject * branch = result->branchingObject();
    assert (branch->numberBranches()==2);
    /*
      Try the first direction.  Each subsequent call to branch() performs the
      specified branch and advances the branch object state to the next branch
      alternative.)
    */
    OsiSolverInterface * thisSolver = solver; 
    if (branch->boundBranch()) {
      // ordinary
      branch->branch(solver);
      // maybe we should check bounds for stupidities here?
      solver->solveFromHotStart() ;
    } else {
      // adding cuts or something 
      thisSolver = solver->clone();
      branch->branch(thisSolver);
      // set hot start iterations
      int limit;
      thisSolver->getIntParam(OsiMaxNumIterationHotStart,limit);
      thisSolver->setIntParam(OsiMaxNumIteration,limit); 
      thisSolver->resolve();
    }
    // can check if we got solution
    // status is 0 finished, 1 infeasible and 2 unfinished and 3 is solution
    int status0 = result->updateInformation(thisSolver,info,this);
    numberStrongIterations_ += thisSolver->getIterationCount();
    if (status0==3) {
      // new solution already saved
      if (trustStrongForSolution_) {
	info->cutoff_ = goodObjectiveValue_;
	status0=0;
      }
    }
    if (solver!=thisSolver)
      delete thisSolver;
    // Restore bounds
    for (int j=0;j<numberColumns;j++) {
      if (saveLower[j] != lower[j])
	solver->setColLower(j,saveLower[j]);
      if (saveUpper[j] != upper[j])
	solver->setColUpper(j,saveUpper[j]);
    }
    /*
      Try the next direction
    */
    thisSolver = solver; 
    if (branch->boundBranch()) {
      // ordinary
      branch->branch(solver);
      // maybe we should check bounds for stupidities here?
      solver->solveFromHotStart() ;
    } else {
      // adding cuts or something 
      thisSolver = solver->clone();
      branch->branch(thisSolver);
      // set hot start iterations
      int limit;
      thisSolver->getIntParam(OsiMaxNumIterationHotStart,limit);
      thisSolver->setIntParam(OsiMaxNumIteration,limit); 
      thisSolver->resolve();
    }
    // can check if we got solution
    // status is 0 finished, 1 infeasible and 2 unfinished and 3 is solution
    int status1 = result->updateInformation(thisSolver,info,this);
    numberStrongDone_++;
    numberStrongIterations_ += thisSolver->getIterationCount();
    if (status1==3) {
      // new solution already saved
      if (trustStrongForSolution_) {
	info->cutoff_ = goodObjectiveValue_;
	status1=0;
      }
    }
    if (solver!=thisSolver)
      delete thisSolver;
    // Restore bounds
    for (int j=0;j<numberColumns;j++) {
      if (saveLower[j] != lower[j])
	solver->setColLower(j,saveLower[j]);
      if (saveUpper[j] != upper[j])
	solver->setColUpper(j,saveUpper[j]);
    }
    /*
      End of evaluation for this candidate variable. Possibilities are:
      * Both sides below cutoff; this variable is a candidate for branching.
      * Both sides infeasible or above the objective cutoff: no further action
      here. Break from the evaluation loop and assume the node will be purged
      by the caller.
      * One side below cutoff: Install the branch (i.e., fix the variable). Possibly break
      from the evaluation loop and assume the node will be reoptimised by the
      caller.
    */
    numResults_++;
    if (status0==1&&status1==1) {
      // infeasible
      returnCode=-1;
      break; // exit loop
    } else if (status0==1||status1==1) {
      numberStrongFixed_++;
      if (!returnCriterion) {
	returnCode=1;
      } else {
	returnCode=2;
	break;
      }
    }
    bool hitMaxTime = ( CoinCpuTime()-timeStart > info->timeRemaining_);
    if (hitMaxTime) {
      returnCode=3;
      break;
    }
  }
  delete [] saveLower;
  delete [] saveUpper;
  // Delete the snapshot
  solver->unmarkHotStart();
  return returnCode;
}

// Given a candidate fill in useful information e.g. estimates
void 
OsiChooseVariable::updateInformation(const OsiBranchingInformation *info,
				  int , OsiHotInfo * hotInfo)
{
  int index = hotInfo->whichObject();
  assert (index<solver_->numberObjects());
  //assert (branch<2);
  OsiObject ** object = info->solver_->objects();
  upChange_ = object[index]->upEstimate();
  downChange_ = object[index]->downEstimate();
}
#if 1
// Given a branch fill in useful information e.g. estimates
void 
OsiChooseVariable::updateInformation( int index, int branch, 
				      double , double ,
				      int )
{
  assert (index<solver_->numberObjects());
  assert (branch<2);
  OsiObject ** object = solver_->objects();
  if (branch)
    upChange_ = object[index]->upEstimate();
  else
    downChange_ = object[index]->downEstimate();
}
#endif

//##############################################################################

void
OsiPseudoCosts::gutsOfDelete()
{
  if (numberObjects_ > 0) {
    numberObjects_ = 0;
    numberBeforeTrusted_ = 0;
    delete[] upTotalChange_;   upTotalChange_ = NULL;
    delete[] downTotalChange_; downTotalChange_ = NULL;
    delete[] upNumber_;        upNumber_ = NULL;
    delete[] downNumber_;      downNumber_ = NULL;
  }
}

void
OsiPseudoCosts::gutsOfCopy(const OsiPseudoCosts& rhs)
{
  numberObjects_ = rhs.numberObjects_;
  numberBeforeTrusted_ = rhs.numberBeforeTrusted_;
  if (numberObjects_ > 0) {
    upTotalChange_ = CoinCopyOfArray(rhs.upTotalChange_,numberObjects_);
    downTotalChange_ = CoinCopyOfArray(rhs.downTotalChange_,numberObjects_);
    upNumber_ = CoinCopyOfArray(rhs.upNumber_,numberObjects_);
    downNumber_ = CoinCopyOfArray(rhs.downNumber_,numberObjects_);
  }
}

OsiPseudoCosts::OsiPseudoCosts() :
  upTotalChange_(NULL),
  downTotalChange_(NULL),
  upNumber_(NULL),
  downNumber_(NULL),
  numberObjects_(0),
  numberBeforeTrusted_(0)
{
}

OsiPseudoCosts::~OsiPseudoCosts()
{
  gutsOfDelete();
}

OsiPseudoCosts::OsiPseudoCosts(const OsiPseudoCosts& rhs) :
  upTotalChange_(NULL),
  downTotalChange_(NULL),
  upNumber_(NULL),
  downNumber_(NULL),
  numberObjects_(0),
  numberBeforeTrusted_(0)
{
  gutsOfCopy(rhs);
}

OsiPseudoCosts&
OsiPseudoCosts::operator=(const OsiPseudoCosts& rhs)
{
  if (this != &rhs) {
    gutsOfDelete();
    gutsOfCopy(rhs);
  }
  return *this;
}

void
OsiPseudoCosts::initialize(int n)
{
  gutsOfDelete();
  numberObjects_ = n;
  if (numberObjects_ > 0) {
    upTotalChange_ = new double [numberObjects_];
    downTotalChange_ = new double [numberObjects_];
    upNumber_ = new int [numberObjects_];
    downNumber_ = new int [numberObjects_];
    CoinZeroN(upTotalChange_,numberObjects_);
    CoinZeroN(downTotalChange_,numberObjects_);
    CoinZeroN(upNumber_,numberObjects_);
    CoinZeroN(downNumber_,numberObjects_);
  }
}
  

//##############################################################################

OsiChooseStrong::OsiChooseStrong() :
  OsiChooseVariable(),
  shadowPriceMode_(0),
  pseudoCosts_(),
  results_(NULL),
  numResults_(0)
{
}

OsiChooseStrong::OsiChooseStrong(const OsiSolverInterface * solver) :
  OsiChooseVariable(solver),
  shadowPriceMode_(0),
  pseudoCosts_(),
  results_(NULL),
  numResults_(0)
{
  // create useful arrays
  pseudoCosts_.initialize(solver_->numberObjects());
}

OsiChooseStrong::OsiChooseStrong(const OsiChooseStrong & rhs) :
  OsiChooseVariable(rhs),
  shadowPriceMode_(rhs.shadowPriceMode_),
  pseudoCosts_(rhs.pseudoCosts_),
  results_(NULL),
  numResults_(0)
{  
}

OsiChooseStrong &
OsiChooseStrong::operator=(const OsiChooseStrong & rhs)
{
  if (this != &rhs) {
    OsiChooseVariable::operator=(rhs);
    shadowPriceMode_ = rhs.shadowPriceMode_;
    pseudoCosts_ = rhs.pseudoCosts_;
    delete[] results_;
    results_ = NULL;
    numResults_ = 0;
  }
  return *this;
}


OsiChooseStrong::~OsiChooseStrong ()
{
  delete[] results_;
}

// Clone
OsiChooseVariable *
OsiChooseStrong::clone() const
{
  return new OsiChooseStrong(*this);
}
#define MAXMIN_CRITERION 0.85
// Initialize
int 
OsiChooseStrong::setupList ( OsiBranchingInformation *info, bool initialize)
{
  if (initialize) {
    status_=-2;
    delete [] goodSolution_;
    bestObjectIndex_=-1;
    numberStrongDone_=0;
    numberStrongIterations_ = 0;
    numberStrongFixed_ = 0;
    goodSolution_ = NULL;
    goodObjectiveValue_ = COIN_DBL_MAX;
  }
  numberOnList_=0;
  numberUnsatisfied_=0;
  int numberObjects = solver_->numberObjects();
  if (numberObjects>pseudoCosts_.numberObjects()) {
    // redo useful arrays
    pseudoCosts_.initialize(numberObjects);
  }
  double check = -COIN_DBL_MAX;
  int checkIndex=0;
  int bestPriority=COIN_INT_MAX;
  int maximumStrong= CoinMin(numberStrong_,numberObjects) ;
  int putOther = numberObjects;
  int i;
  for (i=0;i<numberObjects;i++) {
    list_[i]=-1;
    useful_[i]=0.0;
  }
  OsiObject ** object = info->solver_->objects();
  // Get average pseudo costs and see if pseudo shadow prices possible
  int shadowPossible=shadowPriceMode_;
  if (shadowPossible) {
    for ( i=0;i<numberObjects;i++) {
      if ( !object[i]->canHandleShadowPrices()) {
	shadowPossible=0;
	break;
      }
    }
    if (shadowPossible) {
      int numberRows = solver_->getNumRows();
      const double * pi = info->pi_;
      double sumPi=0.0;
      for (i=0;i<numberRows;i++) 
	sumPi += fabs(pi[i]);
      sumPi /= static_cast<double> (numberRows);
      // and scale back
      sumPi *= 0.01;
      info->defaultDual_ = sumPi; // switch on
      int numberColumns = solver_->getNumCols();
      int size = CoinMax(numberColumns,2*numberRows);
      info->usefulRegion_ = new double [size];
      CoinZeroN(info->usefulRegion_,size);
      info->indexRegion_ = new int [size];
    }
  }
  double sumUp=0.0;
  double numberUp=0.0;
  double sumDown=0.0;
  double numberDown=0.0;
  const double* upTotalChange = pseudoCosts_.upTotalChange();
  const double* downTotalChange = pseudoCosts_.downTotalChange();
  const int* upNumber = pseudoCosts_.upNumber();
  const int* downNumber = pseudoCosts_.downNumber();
  const int numberBeforeTrusted = pseudoCosts_.numberBeforeTrusted();
  for ( i=0;i<numberObjects;i++) {
    sumUp += upTotalChange[i];
    numberUp += upNumber[i];
    sumDown += downTotalChange[i];
    numberDown += downNumber[i];
  }
  double upMultiplier=(1.0+sumUp)/(1.0+numberUp);
  double downMultiplier=(1.0+sumDown)/(1.0+numberDown);
  // Say feasible
  bool feasible = true;
#if 0
  int pri[]={10,1000,10000};
  int priCount[]={0,0,0};
#endif
  for ( i=0;i<numberObjects;i++) {
    int way;
    double value = object[i]->infeasibility(info,way);
    if (value>0.0) {
      numberUnsatisfied_++;
      if (value==COIN_DBL_MAX) {
	// infeasible
	feasible=false;
	break;
      }
      int priorityLevel = object[i]->priority();
#if 0
      for (int k=0;k<3;k++) {
	if (priorityLevel==pri[k])
	  priCount[k]++;
      }
#endif
      // Better priority? Flush choices.
      if (priorityLevel<bestPriority) {
	for (int j=maximumStrong-1;j>=0;j--) {
	  if (list_[j]>=0) {
	    int iObject = list_[j];
	    list_[j]=-1;
	    useful_[j]=0.0;
	    list_[--putOther]=iObject;
	  }
	}
	maximumStrong = CoinMin(maximumStrong,putOther);
	bestPriority = priorityLevel;
	check=-COIN_DBL_MAX;
	checkIndex=0;
      } 
      if (priorityLevel==bestPriority) {
	// Modify value
	sumUp = upTotalChange[i]+1.0e-30;
	numberUp = upNumber[i];
	sumDown = downTotalChange[i]+1.0e-30;
	numberDown = downNumber[i];
	double upEstimate = object[i]->upEstimate();
	double downEstimate = object[i]->downEstimate();
	if (shadowPossible<2) {
	  upEstimate = numberUp ? ((upEstimate*sumUp)/numberUp) : (upEstimate*upMultiplier);
	  if (numberUp<numberBeforeTrusted)
	    upEstimate *= (numberBeforeTrusted+1.0)/(numberUp+1.0);
	  downEstimate = numberDown ? ((downEstimate*sumDown)/numberDown) : (downEstimate*downMultiplier);
	  if (numberDown<numberBeforeTrusted)
	    downEstimate *= (numberBeforeTrusted+1.0)/(numberDown+1.0);
	} else {
	  // use shadow prices always
	}
	value = MAXMIN_CRITERION*CoinMin(upEstimate,downEstimate) + (1.0-MAXMIN_CRITERION)*CoinMax(upEstimate,downEstimate);
	if (value>check) {
	  //add to list
	  int iObject = list_[checkIndex];
	  if (iObject>=0) {
	    assert (list_[putOther-1]<0);
	    list_[--putOther]=iObject;  // to end
	  }
	  list_[checkIndex]=i;
	  assert (checkIndex<putOther);
	  useful_[checkIndex]=value;
	  // find worst
	  check=COIN_DBL_MAX;
	  maximumStrong = CoinMin(maximumStrong,putOther);
	  for (int j=0;j<maximumStrong;j++) {
	    if (list_[j]>=0) {
	      if (useful_[j]<check) {
		check=useful_[j];
		checkIndex=j;
	      }
	    } else {
	      check=0.0;
	      checkIndex = j;
	      break;
	    }
	  }
	} else {
	  // to end
	  assert (list_[putOther-1]<0);
	  list_[--putOther]=i;
	  maximumStrong = CoinMin(maximumStrong,putOther);
	}
      } else {
	// worse priority
	// to end
	assert (list_[putOther-1]<0);
	list_[--putOther]=i;
	maximumStrong = CoinMin(maximumStrong,putOther);
      }
    }
  }
#if 0
  printf("%d at %d, %d at %d and %d at %d\n",priCount[0],pri[0],
	 priCount[1],pri[1],priCount[2],pri[2]);
#endif
  // Get list
  numberOnList_=0;
  if (feasible) {
    for (i=0;i<CoinMin(maximumStrong,putOther);i++) {
      if (list_[i]>=0) {
	list_[numberOnList_]=list_[i];
	useful_[numberOnList_++]=-useful_[i];
      }
    }
    if (numberOnList_) {
      // Sort 
      CoinSort_2(useful_,useful_+numberOnList_,list_);
      // move others
      i = numberOnList_;
      for (;putOther<numberObjects;putOther++) 
	list_[i++]=list_[putOther];
      assert (i==numberUnsatisfied_);
      if (!numberStrong_)
	numberOnList_=0;
    }
  } else {
    // not feasible
    numberUnsatisfied_=-1;
  }
  // Get rid of any shadow prices info
  info->defaultDual_ = -1.0; // switch off
  delete [] info->usefulRegion_;
  delete [] info->indexRegion_;
  return numberUnsatisfied_;
}

void
OsiChooseStrong::resetResults(int num)
{
  delete[] results_;
  numResults_ = 0;
  results_ = new OsiHotInfo[num];
}
  
/* Choose a variable
   Returns - 
   -1 Node is infeasible
   0  Normal termination - we have a candidate
   1  All looks satisfied - no candidate
   2  We can change the bound on a variable - but we also have a strong branching candidate
   3  We can change the bound on a variable - but we have a non-strong branching candidate
   4  We can change the bound on a variable - no other candidates
   We can pick up branch from whichObject() and whichWay()
   We can pick up a forced branch (can change bound) from whichForcedObject() and whichForcedWay()
   If we have a solution then we can pick up from goodObjectiveValue() and goodSolution()
*/
int 
OsiChooseStrong::chooseVariable( OsiSolverInterface * solver, OsiBranchingInformation *info, bool fixVariables)
{
  if (numberUnsatisfied_) {
    const double* upTotalChange = pseudoCosts_.upTotalChange();
    const double* downTotalChange = pseudoCosts_.downTotalChange();
    const int* upNumber = pseudoCosts_.upNumber();
    const int* downNumber = pseudoCosts_.downNumber();
    int numberBeforeTrusted = pseudoCosts_.numberBeforeTrusted();
    // Somehow we can get here with it 0 !
    if (!numberBeforeTrusted) {
      numberBeforeTrusted=5;
      pseudoCosts_.setNumberBeforeTrusted(numberBeforeTrusted);
    }

    int numberLeft = CoinMin(numberStrong_-numberStrongDone_,numberUnsatisfied_);
    int numberToDo=0;
    resetResults(numberLeft);
    int returnCode=0;
    bestObjectIndex_ = -1;
    bestWhichWay_ = -1;
    firstForcedObjectIndex_ = -1;
    firstForcedWhichWay_ =-1;
    double bestTrusted=-COIN_DBL_MAX;
    for (int i=0;i<numberLeft;i++) {
      int iObject = list_[i];
      if (upNumber[iObject]<numberBeforeTrusted||downNumber[iObject]<numberBeforeTrusted) {
	results_[numberToDo++] = OsiHotInfo(solver, info,
					    solver->objects(), iObject);
      } else {
	const OsiObject * obj = solver->object(iObject);
	double upEstimate = (upTotalChange[iObject]*obj->upEstimate())/upNumber[iObject];
	double downEstimate = (downTotalChange[iObject]*obj->downEstimate())/downNumber[iObject];
	double value = MAXMIN_CRITERION*CoinMin(upEstimate,downEstimate) + (1.0-MAXMIN_CRITERION)*CoinMax(upEstimate,downEstimate);
	if (value > bestTrusted) {
	  bestObjectIndex_=iObject;
	  bestWhichWay_ = upEstimate>downEstimate ? 0 : 1;
	  bestTrusted = value;
	}
      }
    }
    int numberFixed=0;
    if (numberToDo) {
      returnCode = doStrongBranching(solver, info, numberToDo, 1);
      if (returnCode>=0&&returnCode<=2) {
	if (returnCode) {
	  returnCode=4;
	  if (bestObjectIndex_>=0)
	    returnCode=3;
	}
	for (int i=0;i<numResults_;i++) {
	  int iObject = results_[i].whichObject();
	  double upEstimate;
	  if (results_[i].upStatus()!=1) {
	    assert (results_[i].upStatus()>=0);
	    upEstimate = results_[i].upChange();
	  } else {
	    // infeasible - just say expensive
	    if (info->cutoff_<1.0e50)
	      upEstimate = 2.0*(info->cutoff_-info->objectiveValue_);
	    else
	      upEstimate = 2.0*fabs(info->objectiveValue_);
	    if (firstForcedObjectIndex_ <0) {
	      firstForcedObjectIndex_ = iObject;
	      firstForcedWhichWay_ =0;
	    }
	    numberFixed++;
	    if (fixVariables) {
	      const OsiObject * obj = solver->object(iObject);
	      OsiBranchingObject * branch = obj->createBranch(solver,info,0);
	      branch->branch(solver);
	      delete branch;
	    }
	  }
	  double downEstimate;
	  if (results_[i].downStatus()!=1) {
	    assert (results_[i].downStatus()>=0);
	    downEstimate = results_[i].downChange();
	  } else {
	    // infeasible - just say expensive
	    if (info->cutoff_<1.0e50)
	      downEstimate = 2.0*(info->cutoff_-info->objectiveValue_);
	    else
	      downEstimate = 2.0*fabs(info->objectiveValue_);
	    if (firstForcedObjectIndex_ <0) {
	      firstForcedObjectIndex_ = iObject;
	      firstForcedWhichWay_ =1;
	    }
	    numberFixed++;
	    if (fixVariables) {
	      const OsiObject * obj = solver->object(iObject);
	      OsiBranchingObject * branch = obj->createBranch(solver,info,1);
	      branch->branch(solver);
	      delete branch;
	    }
	  }
	  double value = MAXMIN_CRITERION*CoinMin(upEstimate,downEstimate) + (1.0-MAXMIN_CRITERION)*CoinMax(upEstimate,downEstimate);
	  if (value>bestTrusted) {
	    bestTrusted = value;
	    bestObjectIndex_ = iObject;
	    bestWhichWay_ = upEstimate>downEstimate ? 0 : 1;
	    // but override if there is a preferred way
	    const OsiObject * obj = solver->object(iObject);
	    if (obj->preferredWay()>=0&&obj->infeasibility())
	      bestWhichWay_ = obj->preferredWay();
	    if (returnCode)
	      returnCode=2;
	  }
	}
      } else if (returnCode==3) {
	// max time - just choose one
	bestObjectIndex_ = list_[0];
	bestWhichWay_ = 0;
	returnCode=0;
      }
    } else {
      bestObjectIndex_=list_[0];
    }
    if ( bestObjectIndex_ >=0 ) {
      OsiObject * obj = solver->objects()[bestObjectIndex_];
      obj->setWhichWay(	bestWhichWay_);
    }
    if (numberFixed==numberUnsatisfied_&&numberFixed)
      returnCode=4;
    return returnCode;
  } else {
    return 1;
  }
}
// Given a candidate  fill in useful information e.g. estimates
void 
OsiPseudoCosts::updateInformation(const OsiBranchingInformation *info,
				  int branch, OsiHotInfo * hotInfo)
{
  int index = hotInfo->whichObject();
  assert (index<info->solver_->numberObjects());
  const OsiObject * object = info->solver_->object(index);
  assert (object->upEstimate()>0.0&&object->downEstimate()>0.0);
  assert (branch<2);
  if (branch) {
    if (hotInfo->upStatus()!=1) {
      assert (hotInfo->upStatus()>=0);
      upTotalChange_[index] += hotInfo->upChange()/object->upEstimate();
      upNumber_[index]++;
    } else {
#if 0
      // infeasible - just say expensive
      if (info->cutoff_<1.0e50)
	upTotalChange_[index] += 2.0*(info->cutoff_-info->objectiveValue_)/object->upEstimate();
      else
	upTotalChange_[index] += 2.0*fabs(info->objectiveValue_)/object->upEstimate();
#endif
    }
  } else {
    if (hotInfo->downStatus()!=1) {
      assert (hotInfo->downStatus()>=0);
      downTotalChange_[index] += hotInfo->downChange()/object->downEstimate();
      downNumber_[index]++;
    } else {
#if 0
      // infeasible - just say expensive
      if (info->cutoff_<1.0e50)
	downTotalChange_[index] += 2.0*(info->cutoff_-info->objectiveValue_)/object->downEstimate();
      else
	downTotalChange_[index] += 2.0*fabs(info->objectiveValue_)/object->downEstimate();
#endif
    }
  }  
}
#if 1
// Given a branch fill in useful information e.g. estimates
void 
OsiPseudoCosts::updateInformation(int index, int branch, 
				  double changeInObjective,
				  double changeInValue,
				  int status)
{
  //assert (index<solver_->numberObjects());
  assert (branch<2);
  assert (changeInValue>0.0);
  assert (branch<2);
  if (branch) {
    if (status!=1) {
      assert (status>=0);
      upTotalChange_[index] += changeInObjective/changeInValue;
      upNumber_[index]++;
    }
  } else {
    if (status!=1) {
      assert (status>=0);
      downTotalChange_[index] += changeInObjective/changeInValue;
      downNumber_[index]++;
    }
  }  
}
#endif

OsiHotInfo::OsiHotInfo() :
  originalObjectiveValue_(COIN_DBL_MAX),
  changes_(NULL),
  iterationCounts_(NULL),
  statuses_(NULL),
  branchingObject_(NULL),
  whichObject_(-1)
{
}

OsiHotInfo::OsiHotInfo(OsiSolverInterface * solver,
		       const OsiBranchingInformation * info,
		       const OsiObject * const * objects,
		       int whichObject) :
  originalObjectiveValue_(COIN_DBL_MAX),
  whichObject_(whichObject)
{
  originalObjectiveValue_ = info->objectiveValue_;
  const OsiObject * object = objects[whichObject_];
  // create object - "down" first
  branchingObject_ = object->createBranch(solver,info,0);
  // create arrays
  int numberBranches = branchingObject_->numberBranches();
  changes_ = new double [numberBranches];
  iterationCounts_ = new int [numberBranches];
  statuses_ = new int [numberBranches];
  CoinZeroN(changes_,numberBranches);
  CoinZeroN(iterationCounts_,numberBranches);
  CoinFillN(statuses_,numberBranches,-1);
}

OsiHotInfo::OsiHotInfo(const OsiHotInfo & rhs) 
{  
  originalObjectiveValue_ = rhs.originalObjectiveValue_;
  whichObject_ = rhs.whichObject_;
  if (rhs.branchingObject_) {
    branchingObject_ = rhs.branchingObject_->clone();
    int numberBranches = branchingObject_->numberBranches();
    changes_ = CoinCopyOfArray(rhs.changes_,numberBranches);
    iterationCounts_ = CoinCopyOfArray(rhs.iterationCounts_,numberBranches);
    statuses_ = CoinCopyOfArray(rhs.statuses_,numberBranches);
  } else {
    branchingObject_ = NULL;
    changes_ = NULL;
    iterationCounts_ = NULL;
    statuses_ = NULL;
  }
}

OsiHotInfo &
OsiHotInfo::operator=(const OsiHotInfo & rhs)
{
  if (this != &rhs) {
    delete branchingObject_;
    delete [] changes_;
    delete [] iterationCounts_;
    delete [] statuses_;
    originalObjectiveValue_ = rhs.originalObjectiveValue_;
    whichObject_ = rhs.whichObject_;
    if (rhs.branchingObject_) {
      branchingObject_ = rhs.branchingObject_->clone();
      int numberBranches = branchingObject_->numberBranches();
      changes_ = CoinCopyOfArray(rhs.changes_,numberBranches);
      iterationCounts_ = CoinCopyOfArray(rhs.iterationCounts_,numberBranches);
      statuses_ = CoinCopyOfArray(rhs.statuses_,numberBranches);
    } else {
      branchingObject_ = NULL;
      changes_ = NULL;
      iterationCounts_ = NULL;
      statuses_ = NULL;
    }
  }
  return *this;
}


OsiHotInfo::~OsiHotInfo ()
{
  delete branchingObject_;
  delete [] changes_;
  delete [] iterationCounts_;
  delete [] statuses_;
}

// Clone
OsiHotInfo *
OsiHotInfo::clone() const
{
  return new OsiHotInfo(*this);
}
/* Fill in useful information after strong branch 
 */
int OsiHotInfo::updateInformation( const OsiSolverInterface * solver, const OsiBranchingInformation * info,
				   OsiChooseVariable * choose)
{
  int iBranch = branchingObject_->branchIndex()-1;
  assert (iBranch>=0&&iBranch<branchingObject_->numberBranches());
  iterationCounts_[iBranch] += solver->getIterationCount();
  int status;
  if (solver->isProvenOptimal())
    status=0; // optimal
  else if (solver->isIterationLimitReached()
	   &&!solver->isDualObjectiveLimitReached())
    status=2; // unknown 
  else
    status=1; // infeasible
  // Could do something different if we can't trust
  double newObjectiveValue = solver->getObjSense()*solver->getObjValue();
  changes_[iBranch] =CoinMax(0.0,newObjectiveValue-originalObjectiveValue_);
  // we might have got here by primal
  if (choose->trustStrongForBound()) {
    if (!status&&newObjectiveValue>=info->cutoff_) {
      status=1; // infeasible
      changes_[iBranch] = 1.0e100;
    }
  }
  statuses_[iBranch] = status;
  if (!status&&choose->trustStrongForSolution()&&newObjectiveValue<choose->goodObjectiveValue()) {
    // check if solution
    const OsiSolverInterface * saveSolver = info->solver_;
    info->solver_=solver;
    const double * saveLower = info->lower_;
    info->lower_ = solver->getColLower();
    const double * saveUpper = info->upper_;
    info->upper_ = solver->getColUpper();
    // also need to make sure bounds OK as may not be info solver
#if 0
    if (saveSolver->getMatrixByCol()) {
	const CoinBigIndex * columnStart = info->columnStart_;
	assert (saveSolver->getMatrixByCol()->getVectorStarts()==columnStart);
    }
#endif
    if (choose->feasibleSolution(info,solver->getColSolution(),solver->numberObjects(),
				 const_cast<const OsiObject **> (solver->objects()))) {
      // put solution somewhere
      choose->saveSolution(solver);
      status=3;
    }
    info->solver_=saveSolver;
    info->lower_ = saveLower;
    info->upper_ = saveUpper;
  }
  // Now update - possible strong branching info
  choose->updateInformation( info,iBranch,this);
  return status;
}
