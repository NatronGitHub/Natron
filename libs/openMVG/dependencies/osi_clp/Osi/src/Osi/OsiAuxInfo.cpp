// Copyright (C) 2006, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include <cassert>
#include <cstdlib>
#include <cmath>
#include <cfloat>

#include "CoinPragma.hpp"
#include "CoinHelperFunctions.hpp"
#include "OsiSolverInterface.hpp"
#include "OsiAuxInfo.hpp"

// Default Constructor
OsiAuxInfo::OsiAuxInfo(void * appData) 
  : appData_(appData)
{
}

// Destructor 
OsiAuxInfo::~OsiAuxInfo ()
{
}

// Clone
OsiAuxInfo *
OsiAuxInfo::clone() const
{
  return new OsiAuxInfo(*this);
}

// Copy constructor 
OsiAuxInfo::OsiAuxInfo(const OsiAuxInfo & rhs)
:
  appData_(rhs.appData_)
{
}
OsiAuxInfo &
OsiAuxInfo::operator=(const OsiAuxInfo &rhs)
{
  if (this != &rhs) {
    appData_ = rhs.appData_;
  }
  return *this;
}
// Default Constructor
OsiBabSolver::OsiBabSolver(int solverType) 
  :OsiAuxInfo(),
   bestObjectiveValue_(1.0e100),
   mipBound_(-1.0e100),
   solver_(NULL),
   bestSolution_(NULL),
   beforeLower_(NULL),
   beforeUpper_(NULL),
   solverType_(solverType),
   sizeSolution_(0),
   extraCharacteristics_(0)
{
}

// Destructor 
OsiBabSolver::~OsiBabSolver ()
{
  delete [] bestSolution_;
}

// Clone
OsiAuxInfo *
OsiBabSolver::clone() const
{
  return new OsiBabSolver(*this);
}

// Copy constructor 
OsiBabSolver::OsiBabSolver(const OsiBabSolver & rhs)
:
  OsiAuxInfo(rhs),
  bestObjectiveValue_(rhs.bestObjectiveValue_),
  mipBound_(rhs.mipBound_),
  solver_(rhs.solver_),
  bestSolution_(NULL),
  beforeLower_(rhs.beforeLower_),
  beforeUpper_(rhs.beforeUpper_),
  solverType_(rhs.solverType_),
  sizeSolution_(rhs.sizeSolution_),
  extraCharacteristics_(rhs.extraCharacteristics_)
{
  if (rhs.bestSolution_) {
    assert (solver_);
    bestSolution_ = CoinCopyOfArray(rhs.bestSolution_,sizeSolution_);
  }
}
OsiBabSolver &
OsiBabSolver::operator=(const OsiBabSolver &rhs)
{
  if (this != &rhs) {
    OsiAuxInfo::operator=(rhs);
    delete [] bestSolution_;
    solver_ = rhs.solver_;
    solverType_ = rhs.solverType_;
    bestObjectiveValue_ = rhs.bestObjectiveValue_;
    bestSolution_ = NULL;
    mipBound_ = rhs.mipBound_;
    sizeSolution_ = rhs.sizeSolution_;
    extraCharacteristics_ = rhs.extraCharacteristics_;
    beforeLower_ = rhs.beforeLower_;
    beforeUpper_ = rhs.beforeUpper_;
    if (rhs.bestSolution_) {
      assert (solver_);
      bestSolution_ = CoinCopyOfArray(rhs.bestSolution_,sizeSolution_);
    }
  }
  return *this;
}
// Returns 1 if solution, 0 if not
int
OsiBabSolver::solution(double & solutionValue,
                       double * betterSolution,
                       int numberColumns)
{
  if (!solver_)
    return 0;
  //printf("getSol %x solution_address %x - value %g\n",
  //       this,bestSolution_,bestObjectiveValue_);
  if (bestObjectiveValue_<solutionValue&&bestSolution_) {
    // new solution
    memcpy(betterSolution,bestSolution_,CoinMin(numberColumns,sizeSolution_)*sizeof(double));
    if (sizeSolution_<numberColumns)
      CoinZeroN(betterSolution+sizeSolution_,numberColumns-sizeSolution_);
    solutionValue = bestObjectiveValue_;
    // free up
    //delete [] bestSolution_;
    //bestSolution_=NULL;
    //bestObjectiveValue_=1.0e100;
    return 1;
  } else {
    return 0;
  }
}

bool
OsiBabSolver::hasSolution(double & solutionValue, double * solution)
{
  if (! bestSolution_)
    return false;
  
  int numberColumns = solver_->getNumCols();
  memcpy(solution,bestSolution_,numberColumns*sizeof(double));
  solutionValue = bestObjectiveValue_;
  return true;
}

// set solution
void
OsiBabSolver::setSolution(const double * solution, int numberColumns, double objectiveValue)
{
  assert (solver_);
  // just in case size has changed
  delete [] bestSolution_;
  sizeSolution_ = CoinMin(solver_->getNumCols(),numberColumns);
  bestSolution_ = new double [sizeSolution_];
  CoinZeroN(bestSolution_,sizeSolution_);
  CoinMemcpyN(solution,CoinMin(sizeSolution_,numberColumns),bestSolution_);
  bestObjectiveValue_ = objectiveValue*solver_->getObjSense();
}
// Get objective  (well mip bound)
double 
OsiBabSolver::mipBound() const
{
  assert (solver_);
  if (solverType_!=3)
    return solver_->getObjSense()*solver_->getObjValue();
  else
    return mipBound_;
}
// Returns true if node feasible
bool 
OsiBabSolver::mipFeasible() const
{
  assert (solver_);
  if (solverType_==0)
    return true;
  else if (solverType_!=3)
    return solver_->isProvenOptimal();
  else
    return mipBound_<1.0e50;
}
