// Copyright (C) 2005, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#if defined(_MSC_VER)
// Turn off compiler warning about long names
#  pragma warning(disable:4786)
#endif

#include "CoinHelperFunctions.hpp"
#include "CoinWarmStartBasis.hpp"

#include "OsiConfig.h"
#include "CoinFinite.hpp"

#include "OsiSolverInterface.hpp"
#include "OsiSolverBranch.hpp"
#include <cassert>
#include <cmath>
#include <cfloat>
//#############################################################################
// Constructors / Destructor / Assignment
//#############################################################################

//-------------------------------------------------------------------
// Default Constructor 
//-------------------------------------------------------------------
OsiSolverBranch::OsiSolverBranch () :
  indices_(NULL), 
  bound_(NULL)
{
  memset(start_,0,sizeof(start_));
}

//-------------------------------------------------------------------
// Copy constructor 
//-------------------------------------------------------------------
OsiSolverBranch::OsiSolverBranch (const OsiSolverBranch & rhs) 
{  
  memcpy(start_,rhs.start_,sizeof(start_));
  int size = start_[4];
  if (size) {
    indices_ = CoinCopyOfArray(rhs.indices_,size);
    bound_ = CoinCopyOfArray(rhs.bound_,size);
  } else {
    indices_=NULL;
    bound_=NULL;
  }
}

//-------------------------------------------------------------------
// Destructor 
//-------------------------------------------------------------------
OsiSolverBranch::~OsiSolverBranch ()
{
  delete [] indices_;
  delete [] bound_;
}

//----------------------------------------------------------------
// Assignment operator 
//-------------------------------------------------------------------
OsiSolverBranch &
OsiSolverBranch::operator=(const OsiSolverBranch& rhs)
{
  if (this != &rhs) {
    delete [] indices_;
    delete [] bound_;
    memcpy(start_,rhs.start_,sizeof(start_));
    int size = start_[4];
    if (size) {
      indices_ = CoinCopyOfArray(rhs.indices_,size);
      bound_ = CoinCopyOfArray(rhs.bound_,size);
    } else {
      indices_=NULL;
      bound_=NULL;
    }
  }
  return *this;
}

//-----------------------------------------------------------------------------
// add simple branch
//-----------------------------------------------------------------------------

void
OsiSolverBranch::addBranch(int iColumn, double value)
{
  delete [] indices_;
  delete [] bound_;
  indices_ = new int[2];
  bound_ = new double[2];
  indices_[0]=iColumn;
  indices_[1]=iColumn;
  start_[0]=0;
  start_[1]=0;
  start_[2]=1;
  bound_[0]=floor(value);
  start_[3]=2;
  bound_[1]=ceil(value);
  start_[4]=2;
  assert (bound_[0]!=bound_[1]);
}
//-----------------------------------------------------------------------------
// Add bounds - way =-1 is first , +1 is second 
//-----------------------------------------------------------------------------

void
OsiSolverBranch::addBranch(int way,int numberTighterLower, const int * whichLower, 
                           const double * newLower,
                           int numberTighterUpper, const int * whichUpper, const double * newUpper)
{
  assert (way==-1||way==1);
  int numberNew = numberTighterLower+numberTighterUpper;
  int base = way+1; // will be 0 or 2
  int numberNow = start_[4-base]-start_[2-base];
  int * tempI = new int[numberNow+numberNew];
  double * tempD = new double[numberNow+numberNew];
  int putNew = (way==-1) ? 0 : start_[2];
  int putNow = (way==-1) ? numberNew : 0;
  memcpy(tempI+putNow,indices_+start_[2-base],numberNow*sizeof(int));
  memcpy(tempD+putNow,bound_+start_[2-base],numberNow*sizeof(double));
  memcpy(tempI+putNew,whichLower,numberTighterLower*sizeof(int));
  memcpy(tempD+putNew,newLower,numberTighterLower*sizeof(double));
  putNew += numberTighterLower;
  memcpy(tempI+putNew,whichUpper,numberTighterUpper*sizeof(int));
  memcpy(tempD+putNew,newUpper,numberTighterUpper*sizeof(double));
  delete [] indices_;
  indices_ = tempI;
  delete [] bound_;
  bound_ = tempD;
  int numberOldLower = start_[3-base]-start_[2-base];
  int numberOldUpper = start_[4-base]-start_[3-base];
  start_[0]=0;
  if (way==-1) {
    start_[1] = numberTighterLower;
    start_[2] = start_[1] + numberTighterUpper;
    start_[3] = start_[2] + numberOldLower;
    start_[4] = start_[3] + numberOldUpper;
  } else {
    start_[1] = numberOldLower;
    start_[2] = start_[1] + numberOldUpper;
    start_[3] = start_[2] + numberTighterLower;
    start_[4] = start_[3] + numberTighterUpper;
  }
}
//-----------------------------------------------------------------------------
// Add bounds - way =-1 is first , +1 is second 
//-----------------------------------------------------------------------------

void
OsiSolverBranch::addBranch(int way,int numberColumns, const double * oldLower, 
                           const double * newLower2,
                           const double * oldUpper, const double * newUpper2)
{
  assert (way==-1||way==1);
  // find
  int i;
  int * whichLower = new int[numberColumns];
  double * newLower = new double[numberColumns];
  int numberTighterLower=0;
  for (i=0;i<numberColumns;i++) {
    if (newLower2[i]>oldLower[i]) {
      whichLower[numberTighterLower]=i;
      newLower[numberTighterLower++]=newLower2[i];
    }
  }
  int * whichUpper = new int[numberColumns];
  double * newUpper = new double[numberColumns];
  int numberTighterUpper=0;
  for (i=0;i<numberColumns;i++) {
    if (newUpper2[i]<oldUpper[i]) {
      whichUpper[numberTighterUpper]=i;
      newUpper[numberTighterUpper++]=newUpper2[i];
    }
  }
  int numberNew = numberTighterLower+numberTighterUpper;
  int base = way+1; // will be 0 or 2
  int numberNow = start_[4-base]-start_[2-base];
  int * tempI = new int[numberNow+numberNew];
  double * tempD = new double[numberNow+numberNew];
  int putNew = (way==-1) ? 0 : start_[2];
  int putNow = (way==-1) ? numberNew : 0;
  memcpy(tempI+putNow,indices_+start_[2-base],numberNow*sizeof(int));
  memcpy(tempD+putNow,bound_+start_[2-base],numberNow*sizeof(double));
  memcpy(tempI+putNew,whichLower,numberTighterLower*sizeof(int));
  memcpy(tempD+putNew,newLower,numberTighterLower*sizeof(double));
  putNew += numberTighterLower;
  memcpy(tempI+putNew,whichUpper,numberTighterUpper*sizeof(int));
  memcpy(tempD+putNew,newUpper,numberTighterUpper*sizeof(double));
  delete [] indices_;
  indices_ = tempI;
  delete [] bound_;
  bound_ = tempD;
  int numberOldLower = start_[3-base]-start_[2-base];
  int numberOldUpper = start_[4-base]-start_[3-base];
  start_[0]=0;
  if (way==-1) {
    start_[1] = numberTighterLower;
    start_[2] = start_[1] + numberTighterUpper;
    start_[3] = start_[2] + numberOldLower;
    start_[4] = start_[3] + numberOldUpper;
  } else {
    start_[1] = numberOldLower;
    start_[2] = start_[1] + numberOldUpper;
    start_[3] = start_[2] + numberTighterLower;
    start_[4] = start_[3] + numberTighterUpper;
  }
  delete [] whichLower;
  delete [] newLower;
  delete [] whichUpper;
  delete [] newUpper;
}
// Apply bounds
void 
OsiSolverBranch::applyBounds(OsiSolverInterface & solver,int way) const
{
  int base = way+1;
  assert (way==-1||way==1);
  int numberColumns = solver.getNumCols();
  const double * columnLower = solver.getColLower();
  int i;
  for (i=start_[base];i<start_[base+1];i++) {
    int iColumn = indices_[i];
    if (iColumn<numberColumns) {
      double value = CoinMax(bound_[i],columnLower[iColumn]);
      solver.setColLower(iColumn,value);
    } else {
      int iRow = iColumn-numberColumns;
      const double * rowLower = solver.getRowLower();
      double value = CoinMax(bound_[i],rowLower[iRow]);
      solver.setRowLower(iRow,value);
    }
  }
  const double * columnUpper = solver.getColUpper();
  for (i=start_[base+1];i<start_[base+2];i++) {
    int iColumn = indices_[i];
    if (iColumn<numberColumns) {
      double value = CoinMin(bound_[i],columnUpper[iColumn]);
      solver.setColUpper(iColumn,value);
    } else {
      int iRow = iColumn-numberColumns;
      const double * rowUpper = solver.getRowUpper();
      double value = CoinMin(bound_[i],rowUpper[iRow]);
      solver.setRowUpper(iRow,value);
    }
  }
}
// Returns true if current solution satsifies one side of branch
bool 
OsiSolverBranch::feasibleOneWay(const OsiSolverInterface & solver) const
{
  bool feasible = false;
  int numberColumns = solver.getNumCols();
  const double * columnLower = solver.getColLower();
  const double * columnUpper = solver.getColUpper();
  const double * columnSolution = solver.getColSolution();
  double primalTolerance;
  solver.getDblParam(OsiPrimalTolerance,primalTolerance);
  for (int base = 0; base<4; base +=2) {
    feasible=true;
    int i;
    for (i=start_[base];i<start_[base+1];i++) {
      int iColumn = indices_[i];
      if (iColumn<numberColumns) {
        double value = CoinMax(bound_[i],columnLower[iColumn]);
        if (columnSolution[iColumn]<value-primalTolerance) {
          feasible=false;
          break;
        }
      } else {
        abort(); // do later (other stuff messed up anyway - e.g. CBC)
      }
    }
    if (!feasible)
      break;
    for (i=start_[base+1];i<start_[base+2];i++) {
      int iColumn = indices_[i];
      if (iColumn<numberColumns) {
        double value = CoinMin(bound_[i],columnUpper[iColumn]);
        if (columnSolution[iColumn]>value+primalTolerance) {
          feasible=false;
          break;
        }
      } else {
        abort(); // do later (other stuff messed up anyway - e.g. CBC)
      }
    }
    if (feasible)
      break; // OK this way
  }
  return feasible;
}
//#############################################################################
// Constructors / Destructor / Assignment
//#############################################################################

//-------------------------------------------------------------------
// Default Constructor 
//-------------------------------------------------------------------
OsiSolverResult::OsiSolverResult () :
  objectiveValue_(COIN_DBL_MAX),
  primalSolution_(NULL), 
  dualSolution_(NULL)
 {
}

//-------------------------------------------------------------------
// Constructor from solver
//-------------------------------------------------------------------
OsiSolverResult::OsiSolverResult (const OsiSolverInterface & solver,const double * lowerBefore,
                                  const double * upperBefore) :
  objectiveValue_(COIN_DBL_MAX),
  primalSolution_(NULL), 
  dualSolution_(NULL)
{
  if (solver.isProvenOptimal()&&!solver.isDualObjectiveLimitReached()) {
    objectiveValue_ = solver.getObjValue()*solver.getObjSense();
    CoinWarmStartBasis * basis = dynamic_cast<CoinWarmStartBasis *> (solver.getWarmStart());
    assert (basis);
    basis_ = * basis;
    delete basis;
    int numberRows = basis_.getNumArtificial();
    int numberColumns = basis_.getNumStructural();
    assert (numberColumns==solver.getNumCols());
    assert (numberRows==solver.getNumRows());
    primalSolution_ = CoinCopyOfArray(solver.getColSolution(),numberColumns);
    dualSolution_ = CoinCopyOfArray(solver.getRowPrice(),numberRows);
    fixed_.addBranch(-1,numberColumns,lowerBefore,solver.getColLower(),
                     upperBefore,solver.getColUpper());
  }
}

//-------------------------------------------------------------------
// Copy constructor 
//-------------------------------------------------------------------
OsiSolverResult::OsiSolverResult (const OsiSolverResult & rhs) 
{  
  objectiveValue_ = rhs.objectiveValue_;
  basis_ = rhs.basis_;
  fixed_ = rhs.fixed_;
  int numberRows = basis_.getNumArtificial();
  int numberColumns = basis_.getNumStructural();
  if (numberColumns) {
    primalSolution_ = CoinCopyOfArray(rhs.primalSolution_,numberColumns);
    dualSolution_ = CoinCopyOfArray(rhs.dualSolution_,numberRows);
  } else {
    primalSolution_=NULL;
    dualSolution_=NULL;
  }
}

//-------------------------------------------------------------------
// Destructor 
//-------------------------------------------------------------------
OsiSolverResult::~OsiSolverResult ()
{
  delete [] primalSolution_;
  delete [] dualSolution_;
}

//----------------------------------------------------------------
// Assignment operator 
//-------------------------------------------------------------------
OsiSolverResult &
OsiSolverResult::operator=(const OsiSolverResult& rhs)
{
  if (this != &rhs) {
    delete [] primalSolution_;
    delete [] dualSolution_;
    objectiveValue_ = rhs.objectiveValue_;
    basis_ = rhs.basis_;
    fixed_ = rhs.fixed_;
    int numberRows = basis_.getNumArtificial();
    int numberColumns = basis_.getNumStructural();
    if (numberColumns) {
      primalSolution_ = CoinCopyOfArray(rhs.primalSolution_,numberColumns);
      dualSolution_ = CoinCopyOfArray(rhs.dualSolution_,numberRows);
    } else {
      primalSolution_=NULL;
      dualSolution_=NULL;
    }
  }
  return *this;
}
// Create result
void 
OsiSolverResult::createResult(const OsiSolverInterface & solver, const double * lowerBefore,
                              const double * upperBefore)
{
  delete [] primalSolution_;
  delete [] dualSolution_;
  if (solver.isProvenOptimal()&&!solver.isDualObjectiveLimitReached()) {
    objectiveValue_ = solver.getObjValue()*solver.getObjSense();
    CoinWarmStartBasis * basis = dynamic_cast<CoinWarmStartBasis *> (solver.getWarmStart());
    assert (basis);
    basis_ = * basis;
    int numberRows = basis_.getNumArtificial();
    int numberColumns = basis_.getNumStructural();
    assert (numberColumns==solver.getNumCols());
    assert (numberRows==solver.getNumRows());
    primalSolution_ = CoinCopyOfArray(solver.getColSolution(),numberColumns);
    dualSolution_ = CoinCopyOfArray(solver.getRowPrice(),numberRows);
    fixed_.addBranch(-1,numberColumns,lowerBefore,solver.getColLower(),
                     upperBefore,solver.getColUpper());
  } else {
    // infeasible
    objectiveValue_ = COIN_DBL_MAX;
    basis_ = CoinWarmStartBasis();;
    primalSolution_=NULL;
    dualSolution_=NULL;
  }
}
// Restore result
void 
OsiSolverResult::restoreResult(OsiSolverInterface & solver) const
{
  //solver.setObjValue(objectiveValue_)*solver.getObjSense();
  solver.setWarmStart(&basis_);
  solver.setColSolution(primalSolution_);
  solver.setRowPrice(dualSolution_);
  fixed_.applyBounds(solver,-1);
}
