/* $Id$ */
// Copyright (C) 2002, International Business Machines
// Corporation and others, Copyright (C) 2012, FasterCoin.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include "CoinPragma.hpp"
#include <iostream>
#include <cassert>

#include "CoinIndexedVector.hpp"

#include "AbcSimplex.hpp"
#include "CoinHelperFunctions.hpp"
#include "AbcNonLinearCost.hpp"
//#############################################################################
// Constructors / Destructor / Assignment
//#############################################################################

//-------------------------------------------------------------------
// Default Constructor
//-------------------------------------------------------------------
AbcNonLinearCost::AbcNonLinearCost () :
  changeCost_(0.0),
  feasibleCost_(0.0),
  infeasibilityWeight_(-1.0),
  largestInfeasibility_(0.0),
  sumInfeasibilities_(0.0),
  averageTheta_(0.0),
  numberRows_(0),
  numberColumns_(0),
  model_(NULL),
  numberInfeasibilities_(-1),
  status_(NULL),
  bound_(NULL),
  cost_(NULL)
{
  
}
//#define VALIDATE
#ifdef VALIDATE
static double * saveLowerV = NULL;
static double * saveUpperV = NULL;
#ifdef NDEBUG
Validate should not be set if no debug
#endif
#endif
				 /* Constructor from simplex.
				    This will just set up wasteful arrays for linear, but
				    later may do dual analysis and even finding duplicate columns
				 */
AbcNonLinearCost::AbcNonLinearCost ( AbcSimplex * model)
{
  model_ = model;
  numberRows_ = model_->numberRows();
  numberColumns_ = model_->numberColumns();
  // If gub then we need this extra
  int numberTotal = numberRows_ + numberColumns_;
  numberInfeasibilities_ = 0;
  changeCost_ = 0.0;
  feasibleCost_ = 0.0;
  infeasibilityWeight_ = -1.0;
  double * cost = model_->costRegion();
  // check if all 0
  int iSequence;
  bool allZero = true;
  for (iSequence = 0; iSequence < numberTotal; iSequence++) {
    if (cost[iSequence]) {
      allZero = false;
      break;
    }
  }
  if (allZero)
    model_->setInfeasibilityCost(1.0);
  sumInfeasibilities_ = 0.0;
  averageTheta_ = 0.0;
  largestInfeasibility_ = 0.0;
  bound_ = new double[numberTotal];
  cost_ = new double[numberTotal];
  status_ = new unsigned char[numberTotal];
  for (iSequence = 0; iSequence < numberTotal; iSequence++) {
    bound_[iSequence] = 0.0;
    cost_[iSequence] = cost[iSequence];
    setInitialStatus(status_[iSequence]);
  }
}
// Refresh - assuming regions OK
void 
AbcNonLinearCost::refresh()
{
  int numberTotal = numberRows_ + numberColumns_;
  numberInfeasibilities_ = 0;
  sumInfeasibilities_ = 0.0;
  largestInfeasibility_ = 0.0;
  double infeasibilityCost = model_->infeasibilityCost();
  double primalTolerance = model_->currentPrimalTolerance();
  double * cost = model_->costRegion();
  double * upper = model_->upperRegion();
  double * lower = model_->lowerRegion();
  double * solution = model_->solutionRegion();
  for (int iSequence = 0; iSequence < numberTotal; iSequence++) {
    cost_[iSequence] = cost[iSequence];
    double value = solution[iSequence];
    double lowerValue = lower[iSequence];
    double upperValue = upper[iSequence];
    if (value - upperValue <= primalTolerance) {
      if (value - lowerValue >= -primalTolerance) {
	// feasible
	status_[iSequence] = static_cast<unsigned char>(CLP_FEASIBLE | (CLP_SAME << 4));
	bound_[iSequence] = 0.0;
      } else {
	// below
	double infeasibility = lowerValue - value - primalTolerance;
	sumInfeasibilities_ += infeasibility;
	largestInfeasibility_ = CoinMax(largestInfeasibility_, infeasibility);
	cost[iSequence] -= infeasibilityCost;
	numberInfeasibilities_++;
	status_[iSequence] = static_cast<unsigned char>(CLP_BELOW_LOWER | (CLP_SAME << 4));
	bound_[iSequence] = upperValue;
	upper[iSequence] = lowerValue;
	lower[iSequence] = -COIN_DBL_MAX;
      }
    } else {
      // above
      double infeasibility = value - upperValue - primalTolerance;
      sumInfeasibilities_ += infeasibility;
      largestInfeasibility_ = CoinMax(largestInfeasibility_, infeasibility);
      cost[iSequence] += infeasibilityCost;
      numberInfeasibilities_++;
      status_[iSequence] = static_cast<unsigned char>(CLP_ABOVE_UPPER | (CLP_SAME << 4));
      bound_[iSequence] = lowerValue;
      lower[iSequence] = upperValue;
      upper[iSequence] = COIN_DBL_MAX;
    }
  }
  //     checkInfeasibilities(model_->primalTolerance());
  
}
// Refresh - from original
void 
AbcNonLinearCost::refreshFromPerturbed(double tolerance)
{
  // original costs and perturbed bounds
  model_->copyFromSaved(32+2);
  refresh();
  //checkInfeasibilities(tolerance);
}
// Refreshes costs always makes row costs zero
void
AbcNonLinearCost::refreshCosts(const double * columnCosts)
{
  double * cost = model_->costRegion();
  // zero row costs
  memset(cost + numberColumns_, 0, numberRows_ * sizeof(double));
  // copy column costs
  CoinMemcpyN(columnCosts, numberColumns_, cost);
  for (int iSequence = 0; iSequence < numberRows_ + numberColumns_; iSequence++) {
    cost_[iSequence] = cost[iSequence];
  }
}
//-------------------------------------------------------------------
// Copy constructor
//-------------------------------------------------------------------
AbcNonLinearCost::AbcNonLinearCost (const AbcNonLinearCost & rhs) :
  changeCost_(0.0),
  feasibleCost_(0.0),
  infeasibilityWeight_(-1.0),
  largestInfeasibility_(0.0),
  sumInfeasibilities_(0.0),
  averageTheta_(0.0),
  numberRows_(rhs.numberRows_),
  numberColumns_(rhs.numberColumns_),
  model_(NULL),
  numberInfeasibilities_(-1),
  status_(NULL),
  bound_(NULL),
  cost_(NULL)
{
  if (numberRows_) {
    int numberTotal = numberRows_ + numberColumns_;
    model_ = rhs.model_;
    numberInfeasibilities_ = rhs.numberInfeasibilities_;
    changeCost_ = rhs.changeCost_;
    feasibleCost_ = rhs.feasibleCost_;
    infeasibilityWeight_ = rhs.infeasibilityWeight_;
    largestInfeasibility_ = rhs.largestInfeasibility_;
    sumInfeasibilities_ = rhs.sumInfeasibilities_;
    averageTheta_ = rhs.averageTheta_;
    bound_ = CoinCopyOfArray(rhs.bound_, numberTotal);
    cost_ = CoinCopyOfArray(rhs.cost_, numberTotal);
    status_ = CoinCopyOfArray(rhs.status_, numberTotal);
  }
}

//-------------------------------------------------------------------
// Destructor
//-------------------------------------------------------------------
AbcNonLinearCost::~AbcNonLinearCost ()
{
  delete [] status_;
  delete [] bound_;
  delete [] cost_;
}

//----------------------------------------------------------------
// Assignment operator
//-------------------------------------------------------------------
AbcNonLinearCost &
AbcNonLinearCost::operator=(const AbcNonLinearCost& rhs)
{
  if (this != &rhs) {
    numberRows_ = rhs.numberRows_;
    numberColumns_ = rhs.numberColumns_;
    delete [] status_;
    delete [] bound_;
    delete [] cost_;
    status_ = NULL;
    bound_ = NULL;
    cost_ = NULL;
    if (numberRows_) {
      int numberTotal = numberRows_ + numberColumns_;
      bound_ = CoinCopyOfArray(rhs.bound_, numberTotal);
      cost_ = CoinCopyOfArray(rhs.cost_, numberTotal);
      status_ = CoinCopyOfArray(rhs.status_, numberTotal);
    }
    model_ = rhs.model_;
    numberInfeasibilities_ = rhs.numberInfeasibilities_;
    changeCost_ = rhs.changeCost_;
    feasibleCost_ = rhs.feasibleCost_;
    infeasibilityWeight_ = rhs.infeasibilityWeight_;
    largestInfeasibility_ = rhs.largestInfeasibility_;
    sumInfeasibilities_ = rhs.sumInfeasibilities_;
    averageTheta_ = rhs.averageTheta_;
  }
  return *this;
}
// Changes infeasible costs and computes number and cost of infeas
// We will need to re-think objective offsets later
// We will also need a 2 bit per variable array for some
// purpose which will come to me later
void
AbcNonLinearCost::checkInfeasibilities(double oldTolerance)
{
  numberInfeasibilities_ = 0;
  double infeasibilityCost = model_->infeasibilityCost();
  changeCost_ = 0.0;
  largestInfeasibility_ = 0.0;
  sumInfeasibilities_ = 0.0;
  double primalTolerance = model_->currentPrimalTolerance();
  int iSequence;
  double * solution = model_->solutionRegion();
  double * upper = model_->upperRegion();
  double * lower = model_->lowerRegion();
  double * cost = model_->costRegion();
  bool toNearest = oldTolerance <= 0.0;
  feasibleCost_ = 0.0;
  //bool checkCosts = (infeasibilityWeight_ != infeasibilityCost);
  infeasibilityWeight_ = infeasibilityCost;
  int numberTotal = numberColumns_ + numberRows_;
  // nonbasic should be at a valid bound
  for (iSequence = 0; iSequence < numberTotal; iSequence++) {
    double value = solution[iSequence];
    unsigned char iStatus = status_[iSequence];
    assert (currentStatus(iStatus) == CLP_SAME);
    double lowerValue = lower[iSequence];
    double upperValue = upper[iSequence];
    double costValue = cost_[iSequence];
    double trueCost = costValue;
    int iWhere = originalStatus(iStatus);
    if (iWhere == CLP_BELOW_LOWER) {
      lowerValue = upperValue;
      upperValue = bound_[iSequence];
      costValue -= infeasibilityCost;
    } else if (iWhere == CLP_ABOVE_UPPER) {
      upperValue = lowerValue;
      lowerValue = bound_[iSequence];
      costValue += infeasibilityCost;
    }
    // get correct place
    int newWhere = CLP_FEASIBLE;
    AbcSimplex::Status status = model_->getInternalStatus(iSequence);
    if (upperValue == lowerValue && status != AbcSimplex::isFixed) {
      if (status != AbcSimplex::basic) {
	model_->setInternalStatus(iSequence, AbcSimplex::isFixed);
	status = AbcSimplex::isFixed;
      }
    }
    switch(status) {
      
    case AbcSimplex::basic:
    case AbcSimplex::superBasic:
      if (value - upperValue <= primalTolerance) {
	if (value - lowerValue >= -primalTolerance) {
	  // feasible
	  //newWhere=CLP_FEASIBLE;
	} else {
	  // below
	  newWhere = CLP_BELOW_LOWER;
	  assert (fabs(lowerValue) < 1.0e100);
	  double infeasibility = lowerValue - value - primalTolerance;
	  sumInfeasibilities_ += infeasibility;
	  largestInfeasibility_ = CoinMax(largestInfeasibility_, infeasibility);
	  costValue = trueCost - infeasibilityCost;
	  changeCost_ -= lowerValue * (costValue - cost[iSequence]);
	  numberInfeasibilities_++;
	}
      } else {
	// above
	newWhere = CLP_ABOVE_UPPER;
	double infeasibility = value - upperValue - primalTolerance;
	sumInfeasibilities_ += infeasibility;
	largestInfeasibility_ = CoinMax(largestInfeasibility_, infeasibility);
	costValue = trueCost + infeasibilityCost;
	changeCost_ -= upperValue * (costValue - cost[iSequence]);
	numberInfeasibilities_++;
      }
      break;
    case AbcSimplex::isFree:
      break;
    case AbcSimplex::atUpperBound:
      if (!toNearest) {
	// With increasing tolerances - we may be at wrong place
	if (fabs(value - upperValue) > oldTolerance * 1.0001) {
	  if (fabs(value - lowerValue) <= oldTolerance * 1.0001) {
	    if  (fabs(value - lowerValue) > primalTolerance) {
	      solution[iSequence] = lowerValue;
	      value = lowerValue;
	    }
	    model_->setInternalStatus(iSequence, AbcSimplex::atLowerBound);
	  } else {
	    if (value < upperValue) {
	      if (value > lowerValue) {
		model_->setInternalStatus(iSequence, AbcSimplex::superBasic);
	      } else {
		// set to lower bound as infeasible
		solution[iSequence] = lowerValue;
		value = lowerValue;
		model_->setInternalStatus(iSequence, AbcSimplex::atLowerBound);
	      }
	    } else {
	      // set to upper bound as infeasible
	      solution[iSequence] = upperValue;
	      value = upperValue;
	    }
	  }
	} else if  (fabs(value - upperValue) > primalTolerance) {
	  solution[iSequence] = upperValue;
	  value = upperValue;
	}
      } else {
	// Set to nearest and make at bound
	if (fabs(value - lowerValue) < fabs(value - upperValue)) {
	  solution[iSequence] = lowerValue;
	  value = lowerValue;
	  model_->setInternalStatus(iSequence, AbcSimplex::atLowerBound);
	} else {
	  solution[iSequence] = upperValue;
	  value = upperValue;
	}
      }
      break;
    case AbcSimplex::atLowerBound:
      if (!toNearest) {
	// With increasing tolerances - we may be at wrong place
	if (fabs(value - lowerValue) > oldTolerance * 1.0001) {
	  if (fabs(value - upperValue) <= oldTolerance * 1.0001) {
	    if  (fabs(value - upperValue) > primalTolerance) {
	      solution[iSequence] = upperValue;
	      value = upperValue;
	    }
	    model_->setInternalStatus(iSequence, AbcSimplex::atUpperBound);
	  } else {
	    if (value < upperValue) {
	      if (value > lowerValue) {
		model_->setInternalStatus(iSequence, AbcSimplex::superBasic);
	      } else {
		// set to lower bound as infeasible
		solution[iSequence] = lowerValue;
		value = lowerValue;
	      }
	    } else {
	      // set to upper bound as infeasible
	      solution[iSequence] = upperValue;
	      value = upperValue;
	      model_->setInternalStatus(iSequence, AbcSimplex::atUpperBound);
	    }
	  }
	} else if  (fabs(value - lowerValue) > primalTolerance) {
	  solution[iSequence] = lowerValue;
	  value = lowerValue;
	}
      } else {
	// Set to nearest and make at bound
	if (fabs(value - lowerValue) < fabs(value - upperValue)) {
	  solution[iSequence] = lowerValue;
	  value = lowerValue;
	} else {
	  solution[iSequence] = upperValue;
	  value = upperValue;
	  model_->setInternalStatus(iSequence, AbcSimplex::atUpperBound);
	}
      }
      break;
    case AbcSimplex::isFixed:
      solution[iSequence] = lowerValue;
      value = lowerValue;
      break;
    }
    if (iWhere != newWhere) {
      setOriginalStatus(status_[iSequence], newWhere);
      if (newWhere == CLP_BELOW_LOWER) {
	bound_[iSequence] = upperValue;
	upperValue = lowerValue;
	lowerValue = -COIN_DBL_MAX;
	costValue = trueCost - infeasibilityCost;
      } else if (newWhere == CLP_ABOVE_UPPER) {
	bound_[iSequence] = lowerValue;
	lowerValue = upperValue;
	upperValue = COIN_DBL_MAX;
	costValue = trueCost + infeasibilityCost;
      } else {
	costValue = trueCost;
      }
      lower[iSequence] = lowerValue;
      upper[iSequence] = upperValue;
    }
    // always do as other things may change
    cost[iSequence] = costValue;
    feasibleCost_ += trueCost * value;
  }
  model_->moveToBasic(14); // all except solution
}
// Puts feasible bounds into lower and upper
void
AbcNonLinearCost::feasibleBounds()
{
  int iSequence;
  double * upper = model_->upperRegion();
  double * lower = model_->lowerRegion();
  double * cost = model_->costRegion();
  int numberTotal = numberColumns_ + numberRows_;
  for (iSequence = 0; iSequence < numberTotal; iSequence++) {
    unsigned char iStatus = status_[iSequence];
    assert (currentStatus(iStatus) == CLP_SAME);
    double lowerValue = lower[iSequence];
    double upperValue = upper[iSequence];
    double costValue = cost_[iSequence];
    int iWhere = originalStatus(iStatus);
    if (iWhere == CLP_BELOW_LOWER) {
      lowerValue = upperValue;
      upperValue = bound_[iSequence];
      assert (fabs(lowerValue) < 1.0e100);
    } else if (iWhere == CLP_ABOVE_UPPER) {
      upperValue = lowerValue;
      lowerValue = bound_[iSequence];
    }
    setOriginalStatus(status_[iSequence], CLP_FEASIBLE);
    lower[iSequence] = lowerValue;
    upper[iSequence] = upperValue;
    cost[iSequence] = costValue;
  }
}
void
AbcNonLinearCost::goBackAll(const CoinIndexedVector * update)
{
  assert (model_ != NULL);
  const int * pivotVariable = model_->pivotVariable();
  int number = update->getNumElements();
  const int * index = update->getIndices();
  for (int i = 0; i < number; i++) {
    int iRow = index[i];
    int iSequence = pivotVariable[iRow];
    setSameStatus(status_[iSequence]);
  }
}
void
AbcNonLinearCost::checkInfeasibilities(int numberInArray, const int * index)
{
  assert (model_ != NULL);
  double primalTolerance = model_->currentPrimalTolerance();
  const int * pivotVariable = model_->pivotVariable();
  double * upper = model_->upperRegion();
  double * lower = model_->lowerRegion();
  double * cost = model_->costRegion();
  double * solutionBasic = model_->solutionBasic();
  double * upperBasic = model_->upperBasic();
  double * lowerBasic = model_->lowerBasic();
  double * costBasic = model_->costBasic();
  for (int i = 0; i < numberInArray; i++) {
    int iRow = index[i];
    int iSequence = pivotVariable[iRow];
    double value = solutionBasic[iRow];
    unsigned char iStatus = status_[iSequence];
    assert (currentStatus(iStatus) == CLP_SAME);
    double lowerValue = lowerBasic[iRow];
    double upperValue = upperBasic[iRow];
    double costValue = cost_[iSequence];
    int iWhere = originalStatus(iStatus);
    if (iWhere == CLP_BELOW_LOWER) {
      lowerValue = upperValue;
      upperValue = bound_[iSequence];
      numberInfeasibilities_--;
      assert (fabs(lowerValue) < 1.0e100);
    } else if (iWhere == CLP_ABOVE_UPPER) {
      upperValue = lowerValue;
      lowerValue = bound_[iSequence];
      numberInfeasibilities_--;
    }
    // get correct place
    int newWhere = CLP_FEASIBLE;
    if (value - upperValue <= primalTolerance) {
      if (value - lowerValue >= -primalTolerance) {
	// feasible
	//newWhere=CLP_FEASIBLE;
      } else {
	// below
	newWhere = CLP_BELOW_LOWER;
	assert (fabs(lowerValue) < 1.0e100);
	costValue -= infeasibilityWeight_;
	numberInfeasibilities_++;
      }
    } else {
      // above
      newWhere = CLP_ABOVE_UPPER;
      costValue += infeasibilityWeight_;
      numberInfeasibilities_++;
    }
    if (iWhere != newWhere) {
      setOriginalStatus(status_[iSequence], newWhere);
      if (newWhere == CLP_BELOW_LOWER) {
	bound_[iSequence] = upperValue;
	upperValue = lowerValue;
	lowerValue = -COIN_DBL_MAX;
      } else if (newWhere == CLP_ABOVE_UPPER) {
	bound_[iSequence] = lowerValue;
	lowerValue = upperValue;
	upperValue = COIN_DBL_MAX;
      }
      lower[iSequence] = lowerValue;
      upper[iSequence] = upperValue;
      cost[iSequence] = costValue;
      lowerBasic[iRow] = lowerValue;
      upperBasic[iRow] = upperValue;
      costBasic[iRow] = costValue;
    }
  }
}
/* Puts back correct infeasible costs for each variable
   The input indices are row indices and need converting to sequences
   for costs.
   On input array is empty (but indices exist).  On exit just
   changed costs will be stored as normal CoinIndexedVector
*/
void
AbcNonLinearCost::checkChanged(int numberInArray, CoinIndexedVector * update)
{
  assert (model_ != NULL);
  double primalTolerance = model_->currentPrimalTolerance();
  const int * pivotVariable = model_->pivotVariable();
  int number = 0;
  int * index = update->getIndices();
  double * work = update->denseVector();
  double * upper = model_->upperRegion();
  double * lower = model_->lowerRegion();
  double * cost = model_->costRegion();
  double * solutionBasic = model_->solutionBasic();
  double * upperBasic = model_->upperBasic();
  double * lowerBasic = model_->lowerBasic();
  double * costBasic = model_->costBasic();
  for (int i = 0; i < numberInArray; i++) {
    int iRow = index[i];
    int iSequence = pivotVariable[iRow];
    double value = solutionBasic[iRow];
    unsigned char iStatus = status_[iSequence];
    assert (currentStatus(iStatus) == CLP_SAME);
    double lowerValue = lowerBasic[iRow];
    double upperValue = upperBasic[iRow];
    double costValue = cost_[iSequence];
    int iWhere = originalStatus(iStatus);
    if (iWhere == CLP_BELOW_LOWER) {
      lowerValue = upperValue;
      upperValue = bound_[iSequence];
      numberInfeasibilities_--;
      assert (fabs(lowerValue) < 1.0e100);
    } else if (iWhere == CLP_ABOVE_UPPER) {
      upperValue = lowerValue;
      lowerValue = bound_[iSequence];
      numberInfeasibilities_--;
    }
    // get correct place
    int newWhere = CLP_FEASIBLE;
    if (value - upperValue <= primalTolerance) {
      if (value - lowerValue >= -primalTolerance) {
	// feasible
	//newWhere=CLP_FEASIBLE;
      } else {
	// below
	newWhere = CLP_BELOW_LOWER;
	costValue -= infeasibilityWeight_;
	numberInfeasibilities_++;
	assert (fabs(lowerValue) < 1.0e100);
      }
    } else {
      // above
      newWhere = CLP_ABOVE_UPPER;
      costValue += infeasibilityWeight_;
      numberInfeasibilities_++;
    }
    if (iWhere != newWhere) {
      work[iRow] = cost[iSequence] - costValue;
      index[number++] = iRow;
      setOriginalStatus(status_[iSequence], newWhere);
      if (newWhere == CLP_BELOW_LOWER) {
	bound_[iSequence] = upperValue;
	upperValue = lowerValue;
	lowerValue = -COIN_DBL_MAX;
      } else if (newWhere == CLP_ABOVE_UPPER) {
	bound_[iSequence] = lowerValue;
	lowerValue = upperValue;
	upperValue = COIN_DBL_MAX;
      }
      lower[iSequence] = lowerValue;
      upper[iSequence] = upperValue;
      cost[iSequence] = costValue;
      lowerBasic[iRow] = lowerValue;
      upperBasic[iRow] = upperValue;
      costBasic[iRow] = costValue;
    }
  }
  update->setNumElements(number);
}
/* Sets bounds and cost for one variable - returns change in cost*/
double
AbcNonLinearCost::setOne(int iSequence, double value)
{
  assert (model_ != NULL);
  double primalTolerance = model_->currentPrimalTolerance();
  // difference in cost
  double difference = 0.0;
  double * upper = model_->upperRegion();
  double * lower = model_->lowerRegion();
  double * cost = model_->costRegion();
  unsigned char iStatus = status_[iSequence];
  assert (currentStatus(iStatus) == CLP_SAME);
  double lowerValue = lower[iSequence];
  double upperValue = upper[iSequence];
  double costValue = cost_[iSequence];
  int iWhere = originalStatus(iStatus);
  if (iWhere == CLP_BELOW_LOWER) {
    lowerValue = upperValue;
    upperValue = bound_[iSequence];
    numberInfeasibilities_--;
    assert (fabs(lowerValue) < 1.0e100);
  } else if (iWhere == CLP_ABOVE_UPPER) {
    upperValue = lowerValue;
    lowerValue = bound_[iSequence];
    numberInfeasibilities_--;
  }
  // get correct place
  int newWhere = CLP_FEASIBLE;
  if (value - upperValue <= primalTolerance) {
    if (value - lowerValue >= -primalTolerance) {
      // feasible
      //newWhere=CLP_FEASIBLE;
    } else {
      // below
      newWhere = CLP_BELOW_LOWER;
      costValue -= infeasibilityWeight_;
      numberInfeasibilities_++;
      assert (fabs(lowerValue) < 1.0e100);
    }
  } else {
    // above
    newWhere = CLP_ABOVE_UPPER;
    costValue += infeasibilityWeight_;
    numberInfeasibilities_++;
  }
  if (iWhere != newWhere) {
    difference = cost[iSequence] - costValue;
    setOriginalStatus(status_[iSequence], newWhere);
    if (newWhere == CLP_BELOW_LOWER) {
      bound_[iSequence] = upperValue;
      upperValue = lowerValue;
      lowerValue = -COIN_DBL_MAX;
    } else if (newWhere == CLP_ABOVE_UPPER) {
      bound_[iSequence] = lowerValue;
      lowerValue = upperValue;
      upperValue = COIN_DBL_MAX;
    }
    lower[iSequence] = lowerValue;
    upper[iSequence] = upperValue;
    cost[iSequence] = costValue;
  }
  AbcSimplex::Status status = model_->getInternalStatus(iSequence);
  if (upperValue == lowerValue) {
    model_->setInternalStatus(iSequence, AbcSimplex::isFixed);
  }
  switch(status) {
    
  case AbcSimplex::basic:
  case AbcSimplex::superBasic:
  case AbcSimplex::isFree:
    break;
  case AbcSimplex::atUpperBound:
  case AbcSimplex::atLowerBound:
  case AbcSimplex::isFixed:
    // set correctly
    if (fabs(value - lowerValue) <= primalTolerance * 1.001) {
      model_->setInternalStatus(iSequence, AbcSimplex::atLowerBound);
    } else if (fabs(value - upperValue) <= primalTolerance * 1.001) {
      model_->setInternalStatus(iSequence, AbcSimplex::atUpperBound);
    } else {
      // set superBasic
      model_->setInternalStatus(iSequence, AbcSimplex::superBasic);
    }
    break;
  }
  changeCost_ += value * difference;
  return difference;
}
/* Sets bounds and cost for one variable - returns change in cost*/
double
AbcNonLinearCost::setOneBasic(int iRow, double value)
{
  assert (model_ != NULL);
  int iSequence=model_->pivotVariable()[iRow];
  double primalTolerance = model_->currentPrimalTolerance();
  // difference in cost
  double difference = 0.0;
  double * upper = model_->upperRegion();
  double * lower = model_->lowerRegion();
  double * cost = model_->costRegion();
  double * upperBasic = model_->upperBasic();
  double * lowerBasic = model_->lowerBasic();
  double * costBasic = model_->costBasic();
  unsigned char iStatus = status_[iSequence];
  assert (currentStatus(iStatus) == CLP_SAME);
  double lowerValue = lowerBasic[iRow];
  double upperValue = upperBasic[iRow];
  double costValue = cost_[iSequence];
  int iWhere = originalStatus(iStatus);
  if (iWhere == CLP_BELOW_LOWER) {
    lowerValue = upperValue;
    upperValue = bound_[iSequence];
    numberInfeasibilities_--;
    assert (fabs(lowerValue) < 1.0e100);
  } else if (iWhere == CLP_ABOVE_UPPER) {
    upperValue = lowerValue;
    lowerValue = bound_[iSequence];
    numberInfeasibilities_--;
  }
  // get correct place
  int newWhere = CLP_FEASIBLE;
  if (value - upperValue <= primalTolerance) {
    if (value - lowerValue >= -primalTolerance) {
      // feasible
      //newWhere=CLP_FEASIBLE;
    } else {
      // below
      newWhere = CLP_BELOW_LOWER;
      costValue -= infeasibilityWeight_;
      numberInfeasibilities_++;
      assert (fabs(lowerValue) < 1.0e100);
    }
  } else {
    // above
    newWhere = CLP_ABOVE_UPPER;
    costValue += infeasibilityWeight_;
    numberInfeasibilities_++;
  }
  if (iWhere != newWhere) {
    difference = cost[iSequence] - costValue;
    setOriginalStatus(status_[iSequence], newWhere);
    if (newWhere == CLP_BELOW_LOWER) {
      bound_[iSequence] = upperValue;
      upperValue = lowerValue;
      lowerValue = -COIN_DBL_MAX;
    } else if (newWhere == CLP_ABOVE_UPPER) {
      bound_[iSequence] = lowerValue;
      lowerValue = upperValue;
      upperValue = COIN_DBL_MAX;
    }
    lower[iSequence] = lowerValue;
    upper[iSequence] = upperValue;
    cost[iSequence] = costValue;
    lowerBasic[iRow] = lowerValue;
    upperBasic[iRow] = upperValue;
    costBasic[iRow] = costValue;
  }
  changeCost_ += value * difference;
  return difference;
}
/* Sets bounds and cost for outgoing variable
   may change value
   Returns direction */
int
AbcNonLinearCost::setOneOutgoing(int iRow, double & value)
{
  assert (model_ != NULL);
  int iSequence=model_->pivotVariable()[iRow];
  double primalTolerance = model_->currentPrimalTolerance();
  // difference in cost
  double difference = 0.0;
  int direction = 0;
  double * upper = model_->upperRegion();
  double * lower = model_->lowerRegion();
  double * cost = model_->costRegion();
  double * upperBasic = model_->upperBasic();
  double * lowerBasic = model_->lowerBasic();
  unsigned char iStatus = status_[iSequence];
  assert (currentStatus(iStatus) == CLP_SAME);
  double lowerValue = lowerBasic[iRow];
  double upperValue = upperBasic[iRow];
  double costValue = cost_[iSequence];
  // Set perceived direction out
  if (value <= lowerValue + 1.001 * primalTolerance) {
    direction = 1;
  } else if (value >= upperValue - 1.001 * primalTolerance) {
    direction = -1;
  } else {
    // odd
    direction = 0;
  }
  int iWhere = originalStatus(iStatus);
  if (iWhere == CLP_BELOW_LOWER) {
    lowerValue = upperValue;
    upperValue = bound_[iSequence];
    numberInfeasibilities_--;
    assert (fabs(lowerValue) < 1.0e100);
  } else if (iWhere == CLP_ABOVE_UPPER) {
    upperValue = lowerValue;
    lowerValue = bound_[iSequence];
    numberInfeasibilities_--;
  }
  // get correct place
  // If fixed give benefit of doubt
  if (lowerValue == upperValue)
    value = lowerValue;
  int newWhere = CLP_FEASIBLE;
  if (value - upperValue <= primalTolerance) {
    if (value - lowerValue >= -primalTolerance) {
      // feasible
      //newWhere=CLP_FEASIBLE;
    } else {
      // below
      newWhere = CLP_BELOW_LOWER;
      costValue -= infeasibilityWeight_;
      numberInfeasibilities_++;
      assert (fabs(lowerValue) < 1.0e100);
    }
  } else {
    // above
    newWhere = CLP_ABOVE_UPPER;
    costValue += infeasibilityWeight_;
    numberInfeasibilities_++;
  }
  if (iWhere != newWhere) {
    difference = cost[iSequence] - costValue;
    setOriginalStatus(status_[iSequence], newWhere);
    if (newWhere == CLP_BELOW_LOWER) {
      bound_[iSequence] = upperValue;
      upper[iSequence] = lowerValue;
      lower[iSequence] = -COIN_DBL_MAX;
    } else if (newWhere == CLP_ABOVE_UPPER) {
      bound_[iSequence] = lowerValue;
      lower[iSequence] = upperValue;
      upper[iSequence] = COIN_DBL_MAX;
    } else {
      lower[iSequence] = lowerValue;
      upper[iSequence] = upperValue;
    }
    cost[iSequence] = costValue;
  }
  // set correctly
  if (fabs(value - lowerValue) <= primalTolerance * 1.001) {
    value = CoinMin(value, lowerValue + primalTolerance);
  } else if (fabs(value - upperValue) <= primalTolerance * 1.001) {
    value = CoinMax(value, upperValue - primalTolerance);
  } else {
    //printf("*** variable wandered off bound %g %g %g!\n",
    //     lowerValue,value,upperValue);
    if (value - lowerValue <= upperValue - value)
      value = lowerValue + primalTolerance;
    else
      value = upperValue - primalTolerance;
  }
  changeCost_ += value * difference;
  return direction;
}
// Returns nearest bound
double
AbcNonLinearCost::nearest(int iRow, double solutionValue)
{
  assert (model_ != NULL);
  int iSequence=model_->pivotVariable()[iRow];
  double nearest = 0.0;
  const double * upperBasic = model_->upperBasic();
  const double * lowerBasic = model_->lowerBasic();
  double lowerValue = lowerBasic[iRow];
  double upperValue = upperBasic[iRow];
  int iWhere = originalStatus(status_[iSequence]);
  if (iWhere == CLP_BELOW_LOWER) {
    lowerValue = upperValue;
    upperValue = bound_[iSequence];
    assert (fabs(lowerValue) < 1.0e100);
  } else if (iWhere == CLP_ABOVE_UPPER) {
    upperValue = lowerValue;
    lowerValue = bound_[iSequence];
  }
  if (fabs(solutionValue - lowerValue) < fabs(solutionValue - upperValue))
    nearest = lowerValue;
  else
    nearest = upperValue;
  return nearest;
}
/// Feasible cost with offset and direction (i.e. for reporting)
double
AbcNonLinearCost::feasibleReportCost() const
{
  double value;
  model_->getDblParam(ClpObjOffset, value);
  return (feasibleCost_ + model_->objectiveAsObject()->nonlinearOffset()) * model_->optimizationDirection() /
    (model_->objectiveScale() * model_->rhsScale()) - value;
}
// Get rid of real costs (just for moment)
void
AbcNonLinearCost::zapCosts()
{
}
#ifdef VALIDATE
// For debug
void
AbcNonLinearCost::validate()
{
  double primalTolerance = model_->currentPrimalTolerance();
  int iSequence;
  const double * solution = model_->solutionRegion();
  const double * upper = model_->upperRegion();
  const double * lower = model_->lowerRegion();
  const double * cost = model_->costRegion();
  double infeasibilityCost = model_->infeasibilityCost();
  int numberTotal = numberRows_ + numberColumns_;
  int numberInfeasibilities = 0;
  double sumInfeasibilities = 0.0;
  
  for (iSequence = 0; iSequence < numberTotal; iSequence++) {
    double value = solution[iSequence];
    int iStatus = status_[iSequence];
    assert (currentStatus(iStatus) == CLP_SAME);
    double lowerValue = lower[iSequence];
    double upperValue = upper[iSequence];
    double costValue = cost_[iSequence]; 
    int iWhere = originalStatus(iStatus);
    if (iWhere == CLP_BELOW_LOWER) {
      lowerValue = upperValue;
      upperValue = bound_[iSequence];
      assert (fabs(lowerValue) < 1.0e100);
      costValue -= infeasibilityCost;
      assert (value <= lowerValue - primalTolerance);
      numberInfeasibilities++;
      sumInfeasibilities += lowerValue - value - primalTolerance;
      assert (model_->getInternalStatus(iSequence) == AbcSimplex::basic);
    } else if (iWhere == CLP_ABOVE_UPPER) {
      upperValue = lowerValue;
      lowerValue = bound_[iSequence];
      costValue += infeasibilityCost;
      assert (value >= upperValue + primalTolerance);
      numberInfeasibilities++;
      sumInfeasibilities += value - upperValue - primalTolerance;
      assert (model_->getInternalStatus(iSequence) == AbcSimplex::basic);
    } else {
      assert (value >= lowerValue - primalTolerance && value <= upperValue + primalTolerance);
    }
    assert (lowerValue == saveLowerV[iSequence]);
    assert (upperValue == saveUpperV[iSequence]);
    assert (costValue == cost[iSequence]);
  }
  if (numberInfeasibilities)
    printf("JJ %d infeasibilities summing to %g\n",
	   numberInfeasibilities, sumInfeasibilities);
}
#endif
