/* $Id$ */
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include "CoinPragma.hpp"
#include <iostream>
#include <cassert>

#include "CoinIndexedVector.hpp"

#include "ClpSimplex.hpp"
#include "CoinHelperFunctions.hpp"
#include "ClpNonLinearCost.hpp"
//#############################################################################
// Constructors / Destructor / Assignment
//#############################################################################

//-------------------------------------------------------------------
// Default Constructor
//-------------------------------------------------------------------
ClpNonLinearCost::ClpNonLinearCost () :
     changeCost_(0.0),
     feasibleCost_(0.0),
     infeasibilityWeight_(-1.0),
     largestInfeasibility_(0.0),
     sumInfeasibilities_(0.0),
     averageTheta_(0.0),
     numberRows_(0),
     numberColumns_(0),
     start_(NULL),
     whichRange_(NULL),
     offset_(NULL),
     lower_(NULL),
     cost_(NULL),
     model_(NULL),
     infeasible_(NULL),
     numberInfeasibilities_(-1),
     status_(NULL),
     bound_(NULL),
     cost2_(NULL),
     method_(1),
     convex_(true),
     bothWays_(false)
{

}
//#define VALIDATE
#ifdef VALIDATE
static double * saveLowerV = NULL;
static double * saveUpperV = NULL;
#ifdef NDEBUG
Validate sgould not be set if no debug
#endif
#endif
/* Constructor from simplex.
   This will just set up wasteful arrays for linear, but
   later may do dual analysis and even finding duplicate columns
*/
ClpNonLinearCost::ClpNonLinearCost ( ClpSimplex * model, int method)
{
     method = 2;
     model_ = model;
     numberRows_ = model_->numberRows();
     //if (numberRows_==402) {
     //model_->setLogLevel(63);
     //model_->setMaximumIterations(30000);
     //}
     numberColumns_ = model_->numberColumns();
     // If gub then we need this extra
     int numberExtra = model_->numberExtraRows();
     if (numberExtra)
          method = 1;
     int numberTotal1 = numberRows_ + numberColumns_;
     int numberTotal = numberTotal1 + numberExtra;
     convex_ = true;
     bothWays_ = false;
     method_ = method;
     numberInfeasibilities_ = 0;
     changeCost_ = 0.0;
     feasibleCost_ = 0.0;
     infeasibilityWeight_ = -1.0;
     double * cost = model_->costRegion();
     // check if all 0
     int iSequence;
     bool allZero = true;
     for (iSequence = 0; iSequence < numberTotal1; iSequence++) {
          if (cost[iSequence]) {
               allZero = false;
               break;
          }
     }
     if (allZero&&model_->clpMatrix()->type()<15)
          model_->setInfeasibilityCost(1.0);
     double infeasibilityCost = model_->infeasibilityCost();
     sumInfeasibilities_ = 0.0;
     averageTheta_ = 0.0;
     largestInfeasibility_ = 0.0;
     // All arrays NULL to start
     status_ = NULL;
     bound_ = NULL;
     cost2_ = NULL;
     start_ = NULL;
     whichRange_ = NULL;
     offset_ = NULL;
     lower_ = NULL;
     cost_ = NULL;
     infeasible_ = NULL;

     double * upper = model_->upperRegion();
     double * lower = model_->lowerRegion();

     // See how we are storing things
     bool always4 = (model_->clpMatrix()->
                     generalExpanded(model_, 10, iSequence) != 0);
     if (always4)
          method_ = 1;
     if (CLP_METHOD1) {
          start_ = new int [numberTotal+1];
          whichRange_ = new int [numberTotal];
          offset_ = new int [numberTotal];
          memset(offset_, 0, numberTotal * sizeof(int));


          // First see how much space we need
          int put = 0;

          // For quadratic we need -inf,0,0,+inf
          for (iSequence = 0; iSequence < numberTotal1; iSequence++) {
               if (!always4) {
                    if (lower[iSequence] > -COIN_DBL_MAX)
                         put++;
                    if (upper[iSequence] < COIN_DBL_MAX)
                         put++;
                    put += 2;
               } else {
                    put += 4;
               }
          }

          // and for extra
          put += 4 * numberExtra;
#ifndef NDEBUG
          int kPut = put;
#endif
          lower_ = new double [put];
          cost_ = new double [put];
          infeasible_ = new unsigned int[(put+31)>>5];
          memset(infeasible_, 0, ((put + 31) >> 5)*sizeof(unsigned int));

          put = 0;

          start_[0] = 0;

          for (iSequence = 0; iSequence < numberTotal1; iSequence++) {
               if (!always4) {
                    if (lower[iSequence] > -COIN_DBL_MAX) {
                         lower_[put] = -COIN_DBL_MAX;
                         setInfeasible(put, true);
                         cost_[put++] = cost[iSequence] - infeasibilityCost;
                    }
                    whichRange_[iSequence] = put;
                    lower_[put] = lower[iSequence];
                    cost_[put++] = cost[iSequence];
                    lower_[put] = upper[iSequence];
                    cost_[put++] = cost[iSequence] + infeasibilityCost;
                    if (upper[iSequence] < COIN_DBL_MAX) {
                         lower_[put] = COIN_DBL_MAX;
                         setInfeasible(put - 1, true);
                         cost_[put++] = 1.0e50;
                    }
               } else {
                    lower_[put] = -COIN_DBL_MAX;
                    setInfeasible(put, true);
                    cost_[put++] = cost[iSequence] - infeasibilityCost;
                    whichRange_[iSequence] = put;
                    lower_[put] = lower[iSequence];
                    cost_[put++] = cost[iSequence];
                    lower_[put] = upper[iSequence];
                    cost_[put++] = cost[iSequence] + infeasibilityCost;
                    lower_[put] = COIN_DBL_MAX;
                    setInfeasible(put - 1, true);
                    cost_[put++] = 1.0e50;
               }
               start_[iSequence+1] = put;
          }
          for (; iSequence < numberTotal; iSequence++) {

               lower_[put] = -COIN_DBL_MAX;
               setInfeasible(put, true);
               put++;
               whichRange_[iSequence] = put;
               lower_[put] = 0.0;
               cost_[put++] = 0.0;
               lower_[put] = 0.0;
               cost_[put++] = 0.0;
               lower_[put] = COIN_DBL_MAX;
               setInfeasible(put - 1, true);
               cost_[put++] = 1.0e50;
               start_[iSequence+1] = put;
          }
          assert (put <= kPut);
     }
#ifdef FAST_CLPNON
     // See how we are storing things
     CoinAssert (model_->clpMatrix()->
                 generalExpanded(model_, 10, iSequence) == 0);
#endif
     if (CLP_METHOD2) {
          assert (!numberExtra);
          bound_ = new double[numberTotal];
          cost2_ = new double[numberTotal];
          status_ = new unsigned char[numberTotal];
#ifdef VALIDATE
          delete [] saveLowerV;
          saveLowerV = CoinCopyOfArray(model_->lowerRegion(), numberTotal);
          delete [] saveUpperV;
          saveUpperV = CoinCopyOfArray(model_->upperRegion(), numberTotal);
#endif
          for (iSequence = 0; iSequence < numberTotal; iSequence++) {
               bound_[iSequence] = 0.0;
               cost2_[iSequence] = cost[iSequence];
               setInitialStatus(status_[iSequence]);
          }
     }
}
// Refresh - assuming regions OK
void 
ClpNonLinearCost::refresh()
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
       cost2_[iSequence] = cost[iSequence];
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
// Refreshes costs always makes row costs zero
void
ClpNonLinearCost::refreshCosts(const double * columnCosts)
{
     double * cost = model_->costRegion();
     // zero row costs
     memset(cost + numberColumns_, 0, numberRows_ * sizeof(double));
     // copy column costs
     CoinMemcpyN(columnCosts, numberColumns_, cost);
     if ((method_ & 1) != 0) {
          for (int iSequence = 0; iSequence < numberRows_ + numberColumns_; iSequence++) {
               int start = start_[iSequence];
               int end = start_[iSequence+1] - 1;
               double thisFeasibleCost = cost[iSequence];
               if (infeasible(start)) {
                    cost_[start] = thisFeasibleCost - infeasibilityWeight_;
                    cost_[start+1] = thisFeasibleCost;
               } else {
                    cost_[start] = thisFeasibleCost;
               }
               if (infeasible(end - 1)) {
                    cost_[end-1] = thisFeasibleCost + infeasibilityWeight_;
               }
          }
     }
     if (CLP_METHOD2) {
          for (int iSequence = 0; iSequence < numberRows_ + numberColumns_; iSequence++) {
               cost2_[iSequence] = cost[iSequence];
          }
     }
}
ClpNonLinearCost::ClpNonLinearCost(ClpSimplex * model, const int * starts,
                                   const double * lowerNon, const double * costNon)
{
#ifndef FAST_CLPNON
     // what about scaling? - only try without it initially
     assert(!model->scalingFlag());
     model_ = model;
     numberRows_ = model_->numberRows();
     numberColumns_ = model_->numberColumns();
     int numberTotal = numberRows_ + numberColumns_;
     convex_ = true;
     bothWays_ = true;
     start_ = new int [numberTotal+1];
     whichRange_ = new int [numberTotal];
     offset_ = new int [numberTotal];
     memset(offset_, 0, numberTotal * sizeof(int));

     double whichWay = model_->optimizationDirection();
     COIN_DETAIL_PRINT(printf("Direction %g\n", whichWay));

     numberInfeasibilities_ = 0;
     changeCost_ = 0.0;
     feasibleCost_ = 0.0;
     double infeasibilityCost = model_->infeasibilityCost();
     infeasibilityWeight_ = infeasibilityCost;;
     largestInfeasibility_ = 0.0;
     sumInfeasibilities_ = 0.0;

     int iSequence;
     assert (!model_->rowObjective());
     double * cost = model_->objective();

     // First see how much space we need
     // Also set up feasible bounds
     int put = starts[numberColumns_];

     double * columnUpper = model_->columnUpper();
     double * columnLower = model_->columnLower();
     for (iSequence = 0; iSequence < numberColumns_; iSequence++) {
          if (columnLower[iSequence] > -1.0e20)
               put++;
          if (columnUpper[iSequence] < 1.0e20)
               put++;
     }

     double * rowUpper = model_->rowUpper();
     double * rowLower = model_->rowLower();
     for (iSequence = 0; iSequence < numberRows_; iSequence++) {
          if (rowLower[iSequence] > -1.0e20)
               put++;
          if (rowUpper[iSequence] < 1.0e20)
               put++;
          put += 2;
     }
     lower_ = new double [put];
     cost_ = new double [put];
     infeasible_ = new unsigned int[(put+31)>>5];
     memset(infeasible_, 0, ((put + 31) >> 5)*sizeof(unsigned int));

     // now fill in
     put = 0;

     start_[0] = 0;
     for (iSequence = 0; iSequence < numberTotal; iSequence++) {
          lower_[put] = -COIN_DBL_MAX;
          whichRange_[iSequence] = put + 1;
          double thisCost;
          double lowerValue;
          double upperValue;
          if (iSequence >= numberColumns_) {
               // rows
               lowerValue = rowLower[iSequence-numberColumns_];
               upperValue = rowUpper[iSequence-numberColumns_];
               if (lowerValue > -1.0e30) {
                    setInfeasible(put, true);
                    cost_[put++] = -infeasibilityCost;
                    lower_[put] = lowerValue;
               }
               cost_[put++] = 0.0;
               thisCost = 0.0;
          } else {
               // columns - move costs and see if convex
               lowerValue = columnLower[iSequence];
               upperValue = columnUpper[iSequence];
               if (lowerValue > -1.0e30) {
                    setInfeasible(put, true);
                    cost_[put++] = whichWay * cost[iSequence] - infeasibilityCost;
                    lower_[put] = lowerValue;
               }
               int iIndex = starts[iSequence];
               int end = starts[iSequence+1];
               assert (fabs(columnLower[iSequence] - lowerNon[iIndex]) < 1.0e-8);
               thisCost = -COIN_DBL_MAX;
               for (; iIndex < end; iIndex++) {
                    if (lowerNon[iIndex] < columnUpper[iSequence] - 1.0e-8) {
                         lower_[put] = lowerNon[iIndex];
                         cost_[put++] = whichWay * costNon[iIndex];
                         // check convexity
                         if (whichWay * costNon[iIndex] < thisCost - 1.0e-12)
                              convex_ = false;
                         thisCost = whichWay * costNon[iIndex];
                    } else {
                         break;
                    }
               }
          }
          lower_[put] = upperValue;
          setInfeasible(put, true);
          cost_[put++] = thisCost + infeasibilityCost;
          if (upperValue < 1.0e20) {
               lower_[put] = COIN_DBL_MAX;
               cost_[put++] = 1.0e50;
          }
          int iFirst = start_[iSequence];
          if (lower_[iFirst] != -COIN_DBL_MAX) {
               setInfeasible(iFirst, true);
               whichRange_[iSequence] = iFirst + 1;
          } else {
               whichRange_[iSequence] = iFirst;
          }
          start_[iSequence+1] = put;
     }
     // can't handle non-convex at present
     assert(convex_);
     status_ = NULL;
     bound_ = NULL;
     cost2_ = NULL;
     method_ = 1;
#else
     printf("recompile ClpNonLinearCost without FAST_CLPNON\n");
     abort();
#endif
}
//-------------------------------------------------------------------
// Copy constructor
//-------------------------------------------------------------------
ClpNonLinearCost::ClpNonLinearCost (const ClpNonLinearCost & rhs) :
     changeCost_(0.0),
     feasibleCost_(0.0),
     infeasibilityWeight_(-1.0),
     largestInfeasibility_(0.0),
     sumInfeasibilities_(0.0),
     averageTheta_(0.0),
     numberRows_(rhs.numberRows_),
     numberColumns_(rhs.numberColumns_),
     start_(NULL),
     whichRange_(NULL),
     offset_(NULL),
     lower_(NULL),
     cost_(NULL),
     model_(NULL),
     infeasible_(NULL),
     numberInfeasibilities_(-1),
     status_(NULL),
     bound_(NULL),
     cost2_(NULL),
     method_(rhs.method_),
     convex_(true),
     bothWays_(rhs.bothWays_)
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
          convex_ = rhs.convex_;
          if (CLP_METHOD1) {
               start_ = new int [numberTotal+1];
               CoinMemcpyN(rhs.start_, (numberTotal + 1), start_);
               whichRange_ = new int [numberTotal];
               CoinMemcpyN(rhs.whichRange_, numberTotal, whichRange_);
               offset_ = new int [numberTotal];
               CoinMemcpyN(rhs.offset_, numberTotal, offset_);
               int numberEntries = start_[numberTotal];
               lower_ = new double [numberEntries];
               CoinMemcpyN(rhs.lower_, numberEntries, lower_);
               cost_ = new double [numberEntries];
               CoinMemcpyN(rhs.cost_, numberEntries, cost_);
               infeasible_ = new unsigned int[(numberEntries+31)>>5];
               CoinMemcpyN(rhs.infeasible_, ((numberEntries + 31) >> 5), infeasible_);
          }
          if (CLP_METHOD2) {
               bound_ = CoinCopyOfArray(rhs.bound_, numberTotal);
               cost2_ = CoinCopyOfArray(rhs.cost2_, numberTotal);
               status_ = CoinCopyOfArray(rhs.status_, numberTotal);
          }
     }
}

//-------------------------------------------------------------------
// Destructor
//-------------------------------------------------------------------
ClpNonLinearCost::~ClpNonLinearCost ()
{
     delete [] start_;
     delete [] whichRange_;
     delete [] offset_;
     delete [] lower_;
     delete [] cost_;
     delete [] infeasible_;
     delete [] status_;
     delete [] bound_;
     delete [] cost2_;
}

//----------------------------------------------------------------
// Assignment operator
//-------------------------------------------------------------------
ClpNonLinearCost &
ClpNonLinearCost::operator=(const ClpNonLinearCost& rhs)
{
     if (this != &rhs) {
          numberRows_ = rhs.numberRows_;
          numberColumns_ = rhs.numberColumns_;
          delete [] start_;
          delete [] whichRange_;
          delete [] offset_;
          delete [] lower_;
          delete [] cost_;
          delete [] infeasible_;
          delete [] status_;
          delete [] bound_;
          delete [] cost2_;
          start_ = NULL;
          whichRange_ = NULL;
          lower_ = NULL;
          cost_ = NULL;
          infeasible_ = NULL;
          status_ = NULL;
          bound_ = NULL;
          cost2_ = NULL;
          method_ = rhs.method_;
          if (numberRows_) {
               int numberTotal = numberRows_ + numberColumns_;
               if (CLP_METHOD1) {
                    start_ = new int [numberTotal+1];
                    CoinMemcpyN(rhs.start_, (numberTotal + 1), start_);
                    whichRange_ = new int [numberTotal];
                    CoinMemcpyN(rhs.whichRange_, numberTotal, whichRange_);
                    offset_ = new int [numberTotal];
                    CoinMemcpyN(rhs.offset_, numberTotal, offset_);
                    int numberEntries = start_[numberTotal];
                    lower_ = new double [numberEntries];
                    CoinMemcpyN(rhs.lower_, numberEntries, lower_);
                    cost_ = new double [numberEntries];
                    CoinMemcpyN(rhs.cost_, numberEntries, cost_);
                    infeasible_ = new unsigned int[(numberEntries+31)>>5];
                    CoinMemcpyN(rhs.infeasible_, ((numberEntries + 31) >> 5), infeasible_);
               }
               if (CLP_METHOD2) {
                    bound_ = CoinCopyOfArray(rhs.bound_, numberTotal);
                    cost2_ = CoinCopyOfArray(rhs.cost2_, numberTotal);
                    status_ = CoinCopyOfArray(rhs.status_, numberTotal);
               }
          }
          model_ = rhs.model_;
          numberInfeasibilities_ = rhs.numberInfeasibilities_;
          changeCost_ = rhs.changeCost_;
          feasibleCost_ = rhs.feasibleCost_;
          infeasibilityWeight_ = rhs.infeasibilityWeight_;
          largestInfeasibility_ = rhs.largestInfeasibility_;
          sumInfeasibilities_ = rhs.sumInfeasibilities_;
          averageTheta_ = rhs.averageTheta_;
          convex_ = rhs.convex_;
          bothWays_ = rhs.bothWays_;
     }
     return *this;
}
// Changes infeasible costs and computes number and cost of infeas
// We will need to re-think objective offsets later
// We will also need a 2 bit per variable array for some
// purpose which will come to me later
void
ClpNonLinearCost::checkInfeasibilities(double oldTolerance)
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
     //#define NONLIN_DEBUG
#ifdef NONLIN_DEBUG
     double * saveSolution = NULL;
     double * saveLower = NULL;
     double * saveUpper = NULL;
     unsigned char * saveStatus = NULL;
     if (method_ == 3) {
          // Save solution as we will be checking
          saveSolution = CoinCopyOfArray(solution, numberTotal);
          saveLower = CoinCopyOfArray(lower, numberTotal);
          saveUpper = CoinCopyOfArray(upper, numberTotal);
          saveStatus = CoinCopyOfArray(model_->statusArray(), numberTotal);
     }
#else
     assert (method_ != 3);
#endif
     if (CLP_METHOD1) {
          // nonbasic should be at a valid bound
          for (iSequence = 0; iSequence < numberTotal; iSequence++) {
               double lowerValue;
               double upperValue;
               double value = solution[iSequence];
               int iRange;
               // get correct place
               int start = start_[iSequence];
               int end = start_[iSequence+1] - 1;
               // correct costs for this infeasibility weight
               // If free then true cost will be first
               double thisFeasibleCost = cost_[start];
               if (infeasible(start)) {
                    thisFeasibleCost = cost_[start+1];
                    cost_[start] = thisFeasibleCost - infeasibilityCost;
               }
               if (infeasible(end - 1)) {
                    thisFeasibleCost = cost_[end-2];
                    cost_[end-1] = thisFeasibleCost + infeasibilityCost;
               }
               for (iRange = start; iRange < end; iRange++) {
                    if (value < lower_[iRange+1] + primalTolerance) {
                         // put in better range if infeasible
                         if (value >= lower_[iRange+1] - primalTolerance && infeasible(iRange) && iRange == start)
                              iRange++;
                         whichRange_[iSequence] = iRange;
                         break;
                    }
               }
               assert(iRange < end);
               lowerValue = lower_[iRange];
               upperValue = lower_[iRange+1];
               ClpSimplex::Status status = model_->getStatus(iSequence);
               if (upperValue == lowerValue && status != ClpSimplex::isFixed) {
                    if (status != ClpSimplex::basic) {
                         model_->setStatus(iSequence, ClpSimplex::isFixed);
                         status = ClpSimplex::isFixed;
                    }
               }
	       //#define PRINT_DETAIL7 2
#if PRINT_DETAIL7>1
	       printf("NL %d sol %g bounds %g %g\n",
		      iSequence,solution[iSequence],
		      lowerValue,upperValue);
#endif
               switch(status) {

               case ClpSimplex::basic:
               case ClpSimplex::superBasic:
                    // iRange is in correct place
                    // slot in here
                    if (infeasible(iRange)) {
                         if (lower_[iRange] < -1.0e50) {
                              //cost_[iRange] = cost_[iRange+1]-infeasibilityCost;
                              // possibly below
                              lowerValue = lower_[iRange+1];
                              if (value - lowerValue < -primalTolerance) {
                                   value = lowerValue - value - primalTolerance;
#ifndef NDEBUG
                                   if(value > 1.0e15)
                                        printf("nonlincostb %d %g %g %g\n",
                                               iSequence, lowerValue, solution[iSequence], lower_[iRange+2]);
#endif
#if PRINT_DETAIL7
				   printf("**NL %d sol %g below %g\n",
					  iSequence,solution[iSequence],
					  lowerValue);
#endif
                                   sumInfeasibilities_ += value;
                                   largestInfeasibility_ = CoinMax(largestInfeasibility_, value);
                                   changeCost_ -= lowerValue *
                                                  (cost_[iRange] - cost[iSequence]);
                                   numberInfeasibilities_++;
                              }
                         } else {
                              //cost_[iRange] = cost_[iRange-1]+infeasibilityCost;
                              // possibly above
                              upperValue = lower_[iRange];
                              if (value - upperValue > primalTolerance) {
                                   value = value - upperValue - primalTolerance;
#ifndef NDEBUG
                                   if(value > 1.0e15)
                                        printf("nonlincostu %d %g %g %g\n",
                                               iSequence, lower_[iRange-1], solution[iSequence], upperValue);
#endif
#if PRINT_DETAIL7
				   printf("**NL %d sol %g above %g\n",
					  iSequence,solution[iSequence],
					  upperValue);
#endif
                                   sumInfeasibilities_ += value;
                                   largestInfeasibility_ = CoinMax(largestInfeasibility_, value);
                                   changeCost_ -= upperValue *
                                                  (cost_[iRange] - cost[iSequence]);
                                   numberInfeasibilities_++;
                              }
                         }
                    }
                    //lower[iSequence] = lower_[iRange];
                    //upper[iSequence] = lower_[iRange+1];
                    //cost[iSequence] = cost_[iRange];
                    break;
               case ClpSimplex::isFree:
                    //if (toNearest)
                    //solution[iSequence] = 0.0;
                    break;
               case ClpSimplex::atUpperBound:
                    if (!toNearest) {
                         // With increasing tolerances - we may be at wrong place
                         if (fabs(value - upperValue) > oldTolerance * 1.0001) {
                              if (fabs(value - lowerValue) <= oldTolerance * 1.0001) {
                                   if  (fabs(value - lowerValue) > primalTolerance)
                                        solution[iSequence] = lowerValue;
                                   model_->setStatus(iSequence, ClpSimplex::atLowerBound);
                              } else {
                                   model_->setStatus(iSequence, ClpSimplex::superBasic);
                              }
                         } else if  (fabs(value - upperValue) > primalTolerance) {
                              solution[iSequence] = upperValue;
                         }
                    } else {
                         // Set to nearest and make at upper bound
                         int kRange;
                         iRange = -1;
                         double nearest = COIN_DBL_MAX;
                         for (kRange = start; kRange < end; kRange++) {
                              if (fabs(lower_[kRange] - value) < nearest) {
                                   nearest = fabs(lower_[kRange] - value);
                                   iRange = kRange;
                              }
                         }
                         assert (iRange >= 0);
                         iRange--;
                         whichRange_[iSequence] = iRange;
                         solution[iSequence] = lower_[iRange+1];
                    }
                    break;
               case ClpSimplex::atLowerBound:
                    if (!toNearest) {
                         // With increasing tolerances - we may be at wrong place
                         // below stops compiler error with gcc 3.2!!!
                         if (iSequence == -119)
                              printf("ZZ %g %g %g %g\n", lowerValue, value, upperValue, oldTolerance);
                         if (fabs(value - lowerValue) > oldTolerance * 1.0001) {
                              if (fabs(value - upperValue) <= oldTolerance * 1.0001) {
                                   if  (fabs(value - upperValue) > primalTolerance)
                                        solution[iSequence] = upperValue;
                                   model_->setStatus(iSequence, ClpSimplex::atUpperBound);
                              } else {
                                   model_->setStatus(iSequence, ClpSimplex::superBasic);
                              }
                         } else if  (fabs(value - lowerValue) > primalTolerance) {
                              solution[iSequence] = lowerValue;
                         }
                    } else {
                         // Set to nearest and make at lower bound
                         int kRange;
                         iRange = -1;
                         double nearest = COIN_DBL_MAX;
                         for (kRange = start; kRange < end; kRange++) {
                              if (fabs(lower_[kRange] - value) < nearest) {
                                   nearest = fabs(lower_[kRange] - value);
                                   iRange = kRange;
                              }
                         }
                         assert (iRange >= 0);
                         whichRange_[iSequence] = iRange;
                         solution[iSequence] = lower_[iRange];
                    }
                    break;
               case ClpSimplex::isFixed:
                    if (toNearest) {
                         // Set to true fixed
                         for (iRange = start; iRange < end; iRange++) {
                              if (lower_[iRange] == lower_[iRange+1])
                                   break;
                         }
                         if (iRange == end) {
                              // Odd - but make sensible
                              // Set to nearest and make at lower bound
                              int kRange;
                              iRange = -1;
                              double nearest = COIN_DBL_MAX;
                              for (kRange = start; kRange < end; kRange++) {
                                   if (fabs(lower_[kRange] - value) < nearest) {
                                        nearest = fabs(lower_[kRange] - value);
                                        iRange = kRange;
                                   }
                              }
                              assert (iRange >= 0);
                              whichRange_[iSequence] = iRange;
                              if (lower_[iRange] != lower_[iRange+1])
                                   model_->setStatus(iSequence, ClpSimplex::atLowerBound);
                              else
                                   model_->setStatus(iSequence, ClpSimplex::atUpperBound);
                         }
                         solution[iSequence] = lower_[iRange];
                    }
                    break;
               }
               lower[iSequence] = lower_[iRange];
               upper[iSequence] = lower_[iRange+1];
               cost[iSequence] = cost_[iRange];
               feasibleCost_ += thisFeasibleCost * solution[iSequence];
               //assert (iRange==whichRange_[iSequence]);
          }
     }
#ifdef NONLIN_DEBUG
     double saveCost = feasibleCost_;
     if (method_ == 3) {
          feasibleCost_ = 0.0;
          // Put back solution as we will be checking
          unsigned char * statusA = model_->statusArray();
          for (iSequence = 0; iSequence < numberTotal; iSequence++) {
               double value = solution[iSequence];
               solution[iSequence] = saveSolution[iSequence];
               saveSolution[iSequence] = value;
               value = lower[iSequence];
               lower[iSequence] = saveLower[iSequence];
               saveLower[iSequence] = value;
               value = upper[iSequence];
               upper[iSequence] = saveUpper[iSequence];
               saveUpper[iSequence] = value;
               unsigned char value2 = statusA[iSequence];
               statusA[iSequence] = saveStatus[iSequence];
               saveStatus[iSequence] = value2;
          }
     }
#endif
     if (CLP_METHOD2) {
       //#define CLP_NON_JUST_BASIC
#ifndef CLP_NON_JUST_BASIC
          // nonbasic should be at a valid bound
          for (iSequence = 0; iSequence < numberTotal; iSequence++) {
#else
	  const int * pivotVariable = model_->pivotVariable();
	  for (int i=0;i<numberRows_;i++) {
	    int iSequence = pivotVariable[i];
#endif
               double value = solution[iSequence];
               unsigned char iStatus = status_[iSequence];
               assert (currentStatus(iStatus) == CLP_SAME);
               double lowerValue = lower[iSequence];
               double upperValue = upper[iSequence];
               double costValue = cost2_[iSequence];
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
               ClpSimplex::Status status = model_->getStatus(iSequence);
               if (upperValue == lowerValue && status != ClpSimplex::isFixed) {
                    if (status != ClpSimplex::basic) {
                         model_->setStatus(iSequence, ClpSimplex::isFixed);
                         status = ClpSimplex::isFixed;
                    }
               }
               switch(status) {

               case ClpSimplex::basic:
               case ClpSimplex::superBasic:
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
               case ClpSimplex::isFree:
                    break;
               case ClpSimplex::atUpperBound:
                    if (!toNearest) {
                         // With increasing tolerances - we may be at wrong place
                         if (fabs(value - upperValue) > oldTolerance * 1.0001) {
                              if (fabs(value - lowerValue) <= oldTolerance * 1.0001) {
                                   if  (fabs(value - lowerValue) > primalTolerance) {
                                        solution[iSequence] = lowerValue;
                                        value = lowerValue;
                                   }
                                   model_->setStatus(iSequence, ClpSimplex::atLowerBound);
                              } else {
                                   if (value < upperValue) {
                                        if (value > lowerValue) {
                                             model_->setStatus(iSequence, ClpSimplex::superBasic);
                                        } else {
                                             // set to lower bound as infeasible
                                             solution[iSequence] = lowerValue;
                                             value = lowerValue;
                                             model_->setStatus(iSequence, ClpSimplex::atLowerBound);
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
                              model_->setStatus(iSequence, ClpSimplex::atLowerBound);
                         } else {
                              solution[iSequence] = upperValue;
                              value = upperValue;
                         }
                    }
                    break;
               case ClpSimplex::atLowerBound:
                    if (!toNearest) {
                         // With increasing tolerances - we may be at wrong place
                         if (fabs(value - lowerValue) > oldTolerance * 1.0001) {
                              if (fabs(value - upperValue) <= oldTolerance * 1.0001) {
                                   if  (fabs(value - upperValue) > primalTolerance) {
                                        solution[iSequence] = upperValue;
                                        value = upperValue;
                                   }
                                   model_->setStatus(iSequence, ClpSimplex::atUpperBound);
                              } else {
                                   if (value < upperValue) {
                                        if (value > lowerValue) {
                                             model_->setStatus(iSequence, ClpSimplex::superBasic);
                                        } else {
                                             // set to lower bound as infeasible
                                             solution[iSequence] = lowerValue;
                                             value = lowerValue;
                                        }
                                   } else {
                                        // set to upper bound as infeasible
                                        solution[iSequence] = upperValue;
                                        value = upperValue;
                                        model_->setStatus(iSequence, ClpSimplex::atUpperBound);
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
                              model_->setStatus(iSequence, ClpSimplex::atUpperBound);
                         }
                    }
                    break;
               case ClpSimplex::isFixed:
                    solution[iSequence] = lowerValue;
                    value = lowerValue;
                    break;
               }
#ifdef NONLIN_DEBUG
               double lo = saveLower[iSequence];
               double up = saveUpper[iSequence];
               double cc = cost[iSequence];
               unsigned char ss = saveStatus[iSequence];
               unsigned char snow = model_->statusArray()[iSequence];
#endif
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
#ifdef NONLIN_DEBUG
               if (method_ == 3) {
                    assert (ss == snow);
                    assert (cc == cost[iSequence]);
                    assert (lo == lower[iSequence]);
                    assert (up == upper[iSequence]);
                    assert (value == saveSolution[iSequence]);
               }
#endif
               feasibleCost_ += trueCost * value;
          }
     }
#ifdef NONLIN_DEBUG
     if (method_ == 3)
          assert (fabs(saveCost - feasibleCost_) < 0.001 * (1.0 + CoinMax(fabs(saveCost), fabs(feasibleCost_))));
     delete [] saveSolution;
     delete [] saveLower;
     delete [] saveUpper;
     delete [] saveStatus;
#endif
     //feasibleCost_ /= (model_->rhsScale()*model_->objScale());
}
// Puts feasible bounds into lower and upper
void
ClpNonLinearCost::feasibleBounds()
{
     if (CLP_METHOD2) {
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
               double costValue = cost2_[iSequence];
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
}
/* Goes through one bound for each variable.
   If array[i]*multiplier>0 goes down, otherwise up.
   The indices are row indices and need converting to sequences
*/
void
ClpNonLinearCost::goThru(int numberInArray, double multiplier,
                         const int * index, const double * array,
                         double * rhs)
{
     assert (model_ != NULL);
     abort();
     const int * pivotVariable = model_->pivotVariable();
     if (CLP_METHOD1) {
          for (int i = 0; i < numberInArray; i++) {
               int iRow = index[i];
               int iSequence = pivotVariable[iRow];
               double alpha = multiplier * array[iRow];
               // get where in bound sequence
               int iRange = whichRange_[iSequence];
               iRange += offset_[iSequence]; //add temporary bias
               double value = model_->solution(iSequence);
               if (alpha > 0.0) {
                    // down one
                    iRange--;
                    assert(iRange >= start_[iSequence]);
                    rhs[iRow] = value - lower_[iRange];
               } else {
                    // up one
                    iRange++;
                    assert(iRange < start_[iSequence+1] - 1);
                    rhs[iRow] = lower_[iRange+1] - value;
               }
               offset_[iSequence] = iRange - whichRange_[iSequence];
          }
     }
#ifdef NONLIN_DEBUG
     double * saveRhs = NULL;
     if (method_ == 3) {
          int numberRows = model_->numberRows();
          saveRhs = CoinCopyOfArray(rhs, numberRows);
     }
#endif
     if (CLP_METHOD2) {
          const double * solution = model_->solutionRegion();
          const double * upper = model_->upperRegion();
          const double * lower = model_->lowerRegion();
          for (int i = 0; i < numberInArray; i++) {
               int iRow = index[i];
               int iSequence = pivotVariable[iRow];
               double alpha = multiplier * array[iRow];
               double value = solution[iSequence];
               unsigned char iStatus = status_[iSequence];
               int iWhere = currentStatus(iStatus);
               if (iWhere == CLP_SAME)
                    iWhere = originalStatus(iStatus);
               if (iWhere == CLP_FEASIBLE) {
                    if (alpha > 0.0) {
                         // going below
                         iWhere = CLP_BELOW_LOWER;
                         rhs[iRow] = value - lower[iSequence];
                    } else {
                         // going above
                         iWhere = CLP_ABOVE_UPPER;
                         rhs[iRow] = upper[iSequence] - value;
                    }
               } else if(iWhere == CLP_BELOW_LOWER) {
                    assert (alpha < 0);
                    // going feasible
                    iWhere = CLP_FEASIBLE;
                    rhs[iRow] = upper[iSequence] - value;
               } else {
                    assert (iWhere == CLP_ABOVE_UPPER);
                    // going feasible
                    iWhere = CLP_FEASIBLE;
                    rhs[iRow] = value - lower[iSequence];
               }
#ifdef NONLIN_DEBUG
               if (method_ == 3)
                    assert (rhs[iRow] == saveRhs[iRow]);
#endif
               setCurrentStatus(status_[iSequence], iWhere);
          }
     }
#ifdef NONLIN_DEBUG
     delete [] saveRhs;
#endif
}
/* Takes off last iteration (i.e. offsets closer to 0)
 */
void
ClpNonLinearCost::goBack(int numberInArray, const int * index,
                         double * rhs)
{
     assert (model_ != NULL);
     abort();
     const int * pivotVariable = model_->pivotVariable();
     if (CLP_METHOD1) {
          for (int i = 0; i < numberInArray; i++) {
               int iRow = index[i];
               int iSequence = pivotVariable[iRow];
               // get where in bound sequence
               int iRange = whichRange_[iSequence];
               // get closer to original
               if (offset_[iSequence] > 0) {
                    offset_[iSequence]--;
                    assert (offset_[iSequence] >= 0);
                    iRange += offset_[iSequence]; //add temporary bias
                    double value = model_->solution(iSequence);
                    // up one
                    assert(iRange < start_[iSequence+1] - 1);
                    rhs[iRow] = lower_[iRange+1] - value; // was earlier lower_[iRange]
               } else {
                    offset_[iSequence]++;
                    assert (offset_[iSequence] <= 0);
                    iRange += offset_[iSequence]; //add temporary bias
                    double value = model_->solution(iSequence);
                    // down one
                    assert(iRange >= start_[iSequence]);
                    rhs[iRow] = value - lower_[iRange]; // was earlier lower_[iRange+1]
               }
          }
     }
#ifdef NONLIN_DEBUG
     double * saveRhs = NULL;
     if (method_ == 3) {
          int numberRows = model_->numberRows();
          saveRhs = CoinCopyOfArray(rhs, numberRows);
     }
#endif
     if (CLP_METHOD2) {
          const double * solution = model_->solutionRegion();
          const double * upper = model_->upperRegion();
          const double * lower = model_->lowerRegion();
          for (int i = 0; i < numberInArray; i++) {
               int iRow = index[i];
               int iSequence = pivotVariable[iRow];
               double value = solution[iSequence];
               unsigned char iStatus = status_[iSequence];
               int iWhere = currentStatus(iStatus);
               int original = originalStatus(iStatus);
               assert (iWhere != CLP_SAME);
               if (iWhere == CLP_FEASIBLE) {
                    if (original == CLP_BELOW_LOWER) {
                         // going below
                         iWhere = CLP_BELOW_LOWER;
                         rhs[iRow] = lower[iSequence] - value;
                    } else {
                         // going above
                         iWhere = CLP_ABOVE_UPPER;
                         rhs[iRow] = value - upper[iSequence];
                    }
               } else if(iWhere == CLP_BELOW_LOWER) {
                    // going feasible
                    iWhere = CLP_FEASIBLE;
                    rhs[iRow] = value - upper[iSequence];
               } else {
                    // going feasible
                    iWhere = CLP_FEASIBLE;
                    rhs[iRow] = lower[iSequence] - value;
               }
#ifdef NONLIN_DEBUG
               if (method_ == 3)
                    assert (rhs[iRow] == saveRhs[iRow]);
#endif
               setCurrentStatus(status_[iSequence], iWhere);
          }
     }
#ifdef NONLIN_DEBUG
     delete [] saveRhs;
#endif
}
void
ClpNonLinearCost::goBackAll(const CoinIndexedVector * update)
{
     assert (model_ != NULL);
     const int * pivotVariable = model_->pivotVariable();
     int number = update->getNumElements();
     const int * index = update->getIndices();
     if (CLP_METHOD1) {
          for (int i = 0; i < number; i++) {
               int iRow = index[i];
               int iSequence = pivotVariable[iRow];
               offset_[iSequence] = 0;
          }
#ifdef CLP_DEBUG
          for (i = 0; i < numberRows_ + numberColumns_; i++)
               assert(!offset_[i]);
#endif
     }
     if (CLP_METHOD2) {
          for (int i = 0; i < number; i++) {
               int iRow = index[i];
               int iSequence = pivotVariable[iRow];
               setSameStatus(status_[iSequence]);
          }
     }
}
void
ClpNonLinearCost::checkInfeasibilities(int numberInArray, const int * index)
{
     assert (model_ != NULL);
     double primalTolerance = model_->currentPrimalTolerance();
     const int * pivotVariable = model_->pivotVariable();
     if (CLP_METHOD1) {
          for (int i = 0; i < numberInArray; i++) {
               int iRow = index[i];
               int iSequence = pivotVariable[iRow];
               // get where in bound sequence
               int iRange;
               int currentRange = whichRange_[iSequence];
               double value = model_->solution(iSequence);
               int start = start_[iSequence];
               int end = start_[iSequence+1] - 1;
               for (iRange = start; iRange < end; iRange++) {
                    if (value < lower_[iRange+1] + primalTolerance) {
                         // put in better range
                         if (value >= lower_[iRange+1] - primalTolerance && infeasible(iRange) && iRange == start)
                              iRange++;
                         break;
                    }
               }
               assert(iRange < end);
               assert(model_->getStatus(iSequence) == ClpSimplex::basic);
               double & lower = model_->lowerAddress(iSequence);
               double & upper = model_->upperAddress(iSequence);
               double & cost = model_->costAddress(iSequence);
               whichRange_[iSequence] = iRange;
               if (iRange != currentRange) {
                    if (infeasible(iRange))
                         numberInfeasibilities_++;
                    if (infeasible(currentRange))
                         numberInfeasibilities_--;
               }
               lower = lower_[iRange];
               upper = lower_[iRange+1];
               cost = cost_[iRange];
          }
     }
     if (CLP_METHOD2) {
          double * solution = model_->solutionRegion();
          double * upper = model_->upperRegion();
          double * lower = model_->lowerRegion();
          double * cost = model_->costRegion();
          for (int i = 0; i < numberInArray; i++) {
               int iRow = index[i];
               int iSequence = pivotVariable[iRow];
               double value = solution[iSequence];
               unsigned char iStatus = status_[iSequence];
               assert (currentStatus(iStatus) == CLP_SAME);
               double lowerValue = lower[iSequence];
               double upperValue = upper[iSequence];
               double costValue = cost2_[iSequence];
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
               }
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
ClpNonLinearCost::checkChanged(int numberInArray, CoinIndexedVector * update)
{
     assert (model_ != NULL);
     double primalTolerance = model_->currentPrimalTolerance();
     const int * pivotVariable = model_->pivotVariable();
     int number = 0;
     int * index = update->getIndices();
     double * work = update->denseVector();
     if (CLP_METHOD1) {
          for (int i = 0; i < numberInArray; i++) {
               int iRow = index[i];
               int iSequence = pivotVariable[iRow];
               // get where in bound sequence
               int iRange;
               double value = model_->solution(iSequence);
               int start = start_[iSequence];
               int end = start_[iSequence+1] - 1;
               for (iRange = start; iRange < end; iRange++) {
                    if (value < lower_[iRange+1] + primalTolerance) {
                         // put in better range
                         if (value >= lower_[iRange+1] - primalTolerance && infeasible(iRange) && iRange == start)
                              iRange++;
                         break;
                    }
               }
               assert(iRange < end);
               assert(model_->getStatus(iSequence) == ClpSimplex::basic);
               int jRange = whichRange_[iSequence];
               if (iRange != jRange) {
                    // changed
                    work[iRow] = cost_[jRange] - cost_[iRange];
                    index[number++] = iRow;
                    double & lower = model_->lowerAddress(iSequence);
                    double & upper = model_->upperAddress(iSequence);
                    double & cost = model_->costAddress(iSequence);
                    whichRange_[iSequence] = iRange;
                    if (infeasible(iRange))
                         numberInfeasibilities_++;
                    if (infeasible(jRange))
                         numberInfeasibilities_--;
                    lower = lower_[iRange];
                    upper = lower_[iRange+1];
                    cost = cost_[iRange];
               }
          }
     }
     if (CLP_METHOD2) {
          double * solution = model_->solutionRegion();
          double * upper = model_->upperRegion();
          double * lower = model_->lowerRegion();
          double * cost = model_->costRegion();
          for (int i = 0; i < numberInArray; i++) {
               int iRow = index[i];
               int iSequence = pivotVariable[iRow];
               double value = solution[iSequence];
               unsigned char iStatus = status_[iSequence];
               assert (currentStatus(iStatus) == CLP_SAME);
               double lowerValue = lower[iSequence];
               double upperValue = upper[iSequence];
               double costValue = cost2_[iSequence];
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
               }
          }
     }
     update->setNumElements(number);
}
/* Sets bounds and cost for one variable - returns change in cost*/
double
ClpNonLinearCost::setOne(int iSequence, double value)
{
     assert (model_ != NULL);
     double primalTolerance = model_->currentPrimalTolerance();
     // difference in cost
     double difference = 0.0;
     if (CLP_METHOD1) {
          // get where in bound sequence
          int iRange;
          int currentRange = whichRange_[iSequence];
          int start = start_[iSequence];
          int end = start_[iSequence+1] - 1;
          if (!bothWays_) {
               // If fixed try and get feasible
               if (lower_[start+1] == lower_[start+2] && fabs(value - lower_[start+1]) < 1.001 * primalTolerance) {
                    iRange = start + 1;
               } else {
                    for (iRange = start; iRange < end; iRange++) {
                         if (value <= lower_[iRange+1] + primalTolerance) {
                              // put in better range
                              if (value >= lower_[iRange+1] - primalTolerance && infeasible(iRange) && iRange == start)
                                   iRange++;
                              break;
                         }
                    }
               }
          } else {
               // leave in current if possible
               iRange = whichRange_[iSequence];
               if (value < lower_[iRange] - primalTolerance || value > lower_[iRange+1] + primalTolerance) {
                    for (iRange = start; iRange < end; iRange++) {
                         if (value < lower_[iRange+1] + primalTolerance) {
                              // put in better range
                              if (value >= lower_[iRange+1] - primalTolerance && infeasible(iRange) && iRange == start)
                                   iRange++;
                              break;
                         }
                    }
               }
          }
          assert(iRange < end);
          whichRange_[iSequence] = iRange;
          if (iRange != currentRange) {
               if (infeasible(iRange))
                    numberInfeasibilities_++;
               if (infeasible(currentRange))
                    numberInfeasibilities_--;
          }
          double & lower = model_->lowerAddress(iSequence);
          double & upper = model_->upperAddress(iSequence);
          double & cost = model_->costAddress(iSequence);
          lower = lower_[iRange];
          upper = lower_[iRange+1];
          ClpSimplex::Status status = model_->getStatus(iSequence);
          if (upper == lower) {
               if (status != ClpSimplex::basic) {
                    model_->setStatus(iSequence, ClpSimplex::isFixed);
                    status = ClpSimplex::basic; // so will skip
               }
          }
          switch(status) {

          case ClpSimplex::basic:
          case ClpSimplex::superBasic:
          case ClpSimplex::isFree:
               break;
          case ClpSimplex::atUpperBound:
          case ClpSimplex::atLowerBound:
          case ClpSimplex::isFixed:
               // set correctly
               if (fabs(value - lower) <= primalTolerance * 1.001) {
                    model_->setStatus(iSequence, ClpSimplex::atLowerBound);
               } else if (fabs(value - upper) <= primalTolerance * 1.001) {
                    model_->setStatus(iSequence, ClpSimplex::atUpperBound);
               } else {
                    // set superBasic
                    model_->setStatus(iSequence, ClpSimplex::superBasic);
               }
               break;
          }
          difference = cost - cost_[iRange];
          cost = cost_[iRange];
     }
     if (CLP_METHOD2) {
          double * upper = model_->upperRegion();
          double * lower = model_->lowerRegion();
          double * cost = model_->costRegion();
          unsigned char iStatus = status_[iSequence];
          assert (currentStatus(iStatus) == CLP_SAME);
          double lowerValue = lower[iSequence];
          double upperValue = upper[iSequence];
          double costValue = cost2_[iSequence];
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
          ClpSimplex::Status status = model_->getStatus(iSequence);
          if (upperValue == lowerValue) {
               if (status != ClpSimplex::basic) {
                    model_->setStatus(iSequence, ClpSimplex::isFixed);
                    status = ClpSimplex::basic; // so will skip
               }
          }
          switch(status) {

          case ClpSimplex::basic:
          case ClpSimplex::superBasic:
          case ClpSimplex::isFree:
               break;
          case ClpSimplex::atUpperBound:
          case ClpSimplex::atLowerBound:
          case ClpSimplex::isFixed:
               // set correctly
               if (fabs(value - lowerValue) <= primalTolerance * 1.001) {
                    model_->setStatus(iSequence, ClpSimplex::atLowerBound);
               } else if (fabs(value - upperValue) <= primalTolerance * 1.001) {
                    model_->setStatus(iSequence, ClpSimplex::atUpperBound);
               } else {
                    // set superBasic
                    model_->setStatus(iSequence, ClpSimplex::superBasic);
               }
               break;
          }
     }
     changeCost_ += value * difference;
     return difference;
}
/* Sets bounds and infeasible cost and true cost for one variable
   This is for gub and column generation etc */
void
ClpNonLinearCost::setOne(int sequence, double solutionValue, double lowerValue, double upperValue,
                         double costValue)
{
     if (CLP_METHOD1) {
          int iRange = -1;
          int start = start_[sequence];
          double infeasibilityCost = model_->infeasibilityCost();
          cost_[start] = costValue - infeasibilityCost;
          lower_[start+1] = lowerValue;
          cost_[start+1] = costValue;
          lower_[start+2] = upperValue;
          cost_[start+2] = costValue + infeasibilityCost;
          double primalTolerance = model_->currentPrimalTolerance();
          if (solutionValue - lowerValue >= -primalTolerance) {
               if (solutionValue - upperValue <= primalTolerance) {
                    iRange = start + 1;
               } else {
                    iRange = start + 2;
               }
          } else {
               iRange = start;
          }
          model_->costRegion()[sequence] = cost_[iRange];
          whichRange_[sequence] = iRange;
     }
     if (CLP_METHOD2) {
       bound_[sequence]=0.0;
       cost2_[sequence]=costValue;
       setInitialStatus(status_[sequence]);
     }

}
/* Sets bounds and cost for outgoing variable
   may change value
   Returns direction */
int
ClpNonLinearCost::setOneOutgoing(int iSequence, double & value)
{
     assert (model_ != NULL);
     double primalTolerance = model_->currentPrimalTolerance();
     // difference in cost
     double difference = 0.0;
     int direction = 0;
     if (CLP_METHOD1) {
          // get where in bound sequence
          int iRange;
          int currentRange = whichRange_[iSequence];
          int start = start_[iSequence];
          int end = start_[iSequence+1] - 1;
          // Set perceived direction out
          if (value <= lower_[currentRange] + 1.001 * primalTolerance) {
               direction = 1;
          } else if (value >= lower_[currentRange+1] - 1.001 * primalTolerance) {
               direction = -1;
          } else {
               // odd
               direction = 0;
          }
          // If fixed try and get feasible
          if (lower_[start+1] == lower_[start+2] && fabs(value - lower_[start+1]) < 1.001 * primalTolerance) {
               iRange = start + 1;
          } else {
               // See if exact
               for (iRange = start; iRange < end; iRange++) {
                    if (value == lower_[iRange+1]) {
                         // put in better range
                         if (infeasible(iRange) && iRange == start)
                              iRange++;
                         break;
                    }
               }
               if (iRange == end) {
                    // not exact
                    for (iRange = start; iRange < end; iRange++) {
                         if (value <= lower_[iRange+1] + primalTolerance) {
                              // put in better range
                              if (value >= lower_[iRange+1] - primalTolerance && infeasible(iRange) && iRange == start)
                                   iRange++;
                              break;
                         }
                    }
               }
          }
          assert(iRange < end);
          whichRange_[iSequence] = iRange;
          if (iRange != currentRange) {
               if (infeasible(iRange))
                    numberInfeasibilities_++;
               if (infeasible(currentRange))
                    numberInfeasibilities_--;
          }
          double & lower = model_->lowerAddress(iSequence);
          double & upper = model_->upperAddress(iSequence);
          double & cost = model_->costAddress(iSequence);
          lower = lower_[iRange];
          upper = lower_[iRange+1];
          if (upper == lower) {
               value = upper;
          } else {
               // set correctly
               if (fabs(value - lower) <= primalTolerance * 1.001) {
                    value = CoinMin(value, lower + primalTolerance);
               } else if (fabs(value - upper) <= primalTolerance * 1.001) {
                    value = CoinMax(value, upper - primalTolerance);
               } else {
                    //printf("*** variable wandered off bound %g %g %g!\n",
                    //     lower,value,upper);
                    if (value - lower <= upper - value)
                         value = lower + primalTolerance;
                    else
                         value = upper - primalTolerance;
               }
          }
          difference = cost - cost_[iRange];
          cost = cost_[iRange];
     }
     if (CLP_METHOD2) {
          double * upper = model_->upperRegion();
          double * lower = model_->lowerRegion();
          double * cost = model_->costRegion();
          unsigned char iStatus = status_[iSequence];
          assert (currentStatus(iStatus) == CLP_SAME);
          double lowerValue = lower[iSequence];
          double upperValue = upper[iSequence];
          double costValue = cost2_[iSequence];
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
     }
     changeCost_ += value * difference;
     return direction;
}
// Returns nearest bound
double
ClpNonLinearCost::nearest(int iSequence, double solutionValue)
{
     assert (model_ != NULL);
     double nearest = 0.0;
     if (CLP_METHOD1) {
          // get where in bound sequence
          int iRange;
          int start = start_[iSequence];
          int end = start_[iSequence+1];
          int jRange = -1;
          nearest = COIN_DBL_MAX;
          for (iRange = start; iRange < end; iRange++) {
               if (fabs(solutionValue - lower_[iRange]) < nearest) {
                    jRange = iRange;
                    nearest = fabs(solutionValue - lower_[iRange]);
               }
          }
          assert(jRange < end);
          nearest = lower_[jRange];
     }
     if (CLP_METHOD2) {
          const double * upper = model_->upperRegion();
          const double * lower = model_->lowerRegion();
          double lowerValue = lower[iSequence];
          double upperValue = upper[iSequence];
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
     }
     return nearest;
}
/// Feasible cost with offset and direction (i.e. for reporting)
double
ClpNonLinearCost::feasibleReportCost() const
{
     double value;
     model_->getDblParam(ClpObjOffset, value);
     return (feasibleCost_ + model_->objectiveAsObject()->nonlinearOffset()) * model_->optimizationDirection() /
            (model_->objectiveScale() * model_->rhsScale()) - value;
}
// Get rid of real costs (just for moment)
void
ClpNonLinearCost::zapCosts()
{
     int iSequence;
     double infeasibilityCost = model_->infeasibilityCost();
     // zero out all costs
     int numberTotal = numberColumns_ + numberRows_;
     if (CLP_METHOD1) {
          int n = start_[numberTotal];
          memset(cost_, 0, n * sizeof(double));
          for (iSequence = 0; iSequence < numberTotal; iSequence++) {
               int start = start_[iSequence];
               int end = start_[iSequence+1] - 1;
               // correct costs for this infeasibility weight
               if (infeasible(start)) {
                    cost_[start] = -infeasibilityCost;
               }
               if (infeasible(end - 1)) {
                    cost_[end-1] = infeasibilityCost;
               }
          }
     }
     if (CLP_METHOD2) {
     }
}
#ifdef VALIDATE
// For debug
void
ClpNonLinearCost::validate()
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
          double costValue = cost2_[iSequence];
          int iWhere = originalStatus(iStatus);
          if (iWhere == CLP_BELOW_LOWER) {
               lowerValue = upperValue;
               upperValue = bound_[iSequence];
               assert (fabs(lowerValue) < 1.0e100);
               costValue -= infeasibilityCost;
               assert (value <= lowerValue - primalTolerance);
               numberInfeasibilities++;
               sumInfeasibilities += lowerValue - value - primalTolerance;
               assert (model_->getStatus(iSequence) == ClpSimplex::basic);
          } else if (iWhere == CLP_ABOVE_UPPER) {
               upperValue = lowerValue;
               lowerValue = bound_[iSequence];
               costValue += infeasibilityCost;
               assert (value >= upperValue + primalTolerance);
               numberInfeasibilities++;
               sumInfeasibilities += value - upperValue - primalTolerance;
               assert (model_->getStatus(iSequence) == ClpSimplex::basic);
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
