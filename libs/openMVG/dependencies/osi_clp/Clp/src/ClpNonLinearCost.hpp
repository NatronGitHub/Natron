/* $Id$ */
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef ClpNonLinearCost_H
#define ClpNonLinearCost_H


#include "CoinPragma.hpp"

class ClpSimplex;
class CoinIndexedVector;

/** Trivial class to deal with non linear costs

    I don't make any explicit assumptions about convexity but I am
    sure I do make implicit ones.

    One interesting idea for normal LP's will be to allow non-basic
    variables to come into basis as infeasible i.e. if variable at
    lower bound has very large positive reduced cost (when problem
    is infeasible) could it reduce overall problem infeasibility more
    by bringing it into basis below its lower bound.

    Another feature would be to automatically discover when problems
    are convex piecewise linear and re-formulate to use non-linear.
    I did some work on this many years ago on "grade" problems, but
    while it improved primal interior point algorithms were much better
    for that particular problem.
*/
/* status has original status and current status
   0 - below lower so stored is upper
   1 - in range
   2 - above upper so stored is lower
   4 - (for current) - same as original
*/
#define CLP_BELOW_LOWER 0
#define CLP_FEASIBLE 1
#define CLP_ABOVE_UPPER 2
#define CLP_SAME 4
inline int originalStatus(unsigned char status)
{
     return (status & 15);
}
inline int currentStatus(unsigned char status)
{
     return (status >> 4);
}
inline void setOriginalStatus(unsigned char & status, int value)
{
     status = static_cast<unsigned char>(status & ~15);
     status = static_cast<unsigned char>(status | value);
}
inline void setCurrentStatus(unsigned char &status, int value)
{
     status = static_cast<unsigned char>(status & ~(15 << 4));
     status = static_cast<unsigned char>(status | (value << 4));
}
inline void setInitialStatus(unsigned char &status)
{
     status = static_cast<unsigned char>(CLP_FEASIBLE | (CLP_SAME << 4));
}
inline void setSameStatus(unsigned char &status)
{
     status = static_cast<unsigned char>(status & ~(15 << 4));
     status = static_cast<unsigned char>(status | (CLP_SAME << 4));
}
// Use second version to get more speed
//#define FAST_CLPNON
#ifndef FAST_CLPNON
#define CLP_METHOD1 ((method_&1)!=0)
#define CLP_METHOD2 ((method_&2)!=0)
#else
#define CLP_METHOD1 (false)
#define CLP_METHOD2 (true)
#endif
class ClpNonLinearCost  {

public:

public:

     /**@name Constructors, destructor */
     //@{
     /// Default constructor.
     ClpNonLinearCost();
     /** Constructor from simplex.
         This will just set up wasteful arrays for linear, but
         later may do dual analysis and even finding duplicate columns .
     */
     ClpNonLinearCost(ClpSimplex * model, int method = 1);
     /** Constructor from simplex and list of non-linearities (columns only)
         First lower of each column has to match real lower
         Last lower has to be <= upper (if == then cost ignored)
         This could obviously be changed to make more user friendly
     */
     ClpNonLinearCost(ClpSimplex * model, const int * starts,
                      const double * lower, const double * cost);
     /// Destructor
     ~ClpNonLinearCost();
     // Copy
     ClpNonLinearCost(const ClpNonLinearCost&);
     // Assignment
     ClpNonLinearCost& operator=(const ClpNonLinearCost&);
     //@}


     /**@name Actual work in primal */
     //@{
     /** Changes infeasible costs and computes number and cost of infeas
         Puts all non-basic (non free) variables to bounds
         and all free variables to zero if oldTolerance is non-zero
         - but does not move those <= oldTolerance away*/
     void checkInfeasibilities(double oldTolerance = 0.0);
     /** Changes infeasible costs for each variable
         The indices are row indices and need converting to sequences
     */
     void checkInfeasibilities(int numberInArray, const int * index);
     /** Puts back correct infeasible costs for each variable
         The input indices are row indices and need converting to sequences
         for costs.
         On input array is empty (but indices exist).  On exit just
         changed costs will be stored as normal CoinIndexedVector
     */
     void checkChanged(int numberInArray, CoinIndexedVector * update);
     /** Goes through one bound for each variable.
         If multiplier*work[iRow]>0 goes down, otherwise up.
         The indices are row indices and need converting to sequences
         Temporary offsets may be set
         Rhs entries are increased
     */
     void goThru(int numberInArray, double multiplier,
                 const int * index, const double * work,
                 double * rhs);
     /** Takes off last iteration (i.e. offsets closer to 0)
     */
     void goBack(int numberInArray, const int * index,
                 double * rhs);
     /** Puts back correct infeasible costs for each variable
         The input indices are row indices and need converting to sequences
         for costs.
         At the end of this all temporary offsets are zero
     */
     void goBackAll(const CoinIndexedVector * update);
     /// Temporary zeroing of feasible costs
     void zapCosts();
     /// Refreshes costs always makes row costs zero
     void refreshCosts(const double * columnCosts);
     /// Puts feasible bounds into lower and upper
     void feasibleBounds();
     /// Refresh - assuming regions OK
     void refresh();
     /** Sets bounds and cost for one variable
         Returns change in cost
      May need to be inline for speed */
     double setOne(int sequence, double solutionValue);
     /** Sets bounds and infeasible cost and true cost for one variable
         This is for gub and column generation etc */
     void setOne(int sequence, double solutionValue, double lowerValue, double upperValue,
                 double costValue = 0.0);
     /** Sets bounds and cost for outgoing variable
         may change value
         Returns direction */
     int setOneOutgoing(int sequence, double &solutionValue);
     /// Returns nearest bound
     double nearest(int sequence, double solutionValue);
     /** Returns change in cost - one down if alpha >0.0, up if <0.0
         Value is current - new
      */
     inline double changeInCost(int sequence, double alpha) const {
          double returnValue = 0.0;
          if (CLP_METHOD1) {
               int iRange = whichRange_[sequence] + offset_[sequence];
               if (alpha > 0.0)
                    returnValue = cost_[iRange] - cost_[iRange-1];
               else
                    returnValue = cost_[iRange] - cost_[iRange+1];
          }
          if (CLP_METHOD2) {
               returnValue = (alpha > 0.0) ? infeasibilityWeight_ : -infeasibilityWeight_;
          }
          return returnValue;
     }
     inline double changeUpInCost(int sequence) const {
          double returnValue = 0.0;
          if (CLP_METHOD1) {
               int iRange = whichRange_[sequence] + offset_[sequence];
               if (iRange + 1 != start_[sequence+1] && !infeasible(iRange + 1))
                    returnValue = cost_[iRange] - cost_[iRange+1];
               else
                    returnValue = -1.0e100;
          }
          if (CLP_METHOD2) {
               returnValue = -infeasibilityWeight_;
          }
          return returnValue;
     }
     inline double changeDownInCost(int sequence) const {
          double returnValue = 0.0;
          if (CLP_METHOD1) {
               int iRange = whichRange_[sequence] + offset_[sequence];
               if (iRange != start_[sequence] && !infeasible(iRange - 1))
                    returnValue = cost_[iRange] - cost_[iRange-1];
               else
                    returnValue = 1.0e100;
          }
          if (CLP_METHOD2) {
               returnValue = infeasibilityWeight_;
          }
          return returnValue;
     }
     /// This also updates next bound
     inline double changeInCost(int sequence, double alpha, double &rhs) {
          double returnValue = 0.0;
#ifdef NONLIN_DEBUG
          double saveRhs = rhs;
#endif
          if (CLP_METHOD1) {
               int iRange = whichRange_[sequence] + offset_[sequence];
               if (alpha > 0.0) {
                    assert(iRange - 1 >= start_[sequence]);
                    offset_[sequence]--;
                    rhs += lower_[iRange] - lower_[iRange-1];
                    returnValue = alpha * (cost_[iRange] - cost_[iRange-1]);
               } else {
                    assert(iRange + 1 < start_[sequence+1] - 1);
                    offset_[sequence]++;
                    rhs += lower_[iRange+2] - lower_[iRange+1];
                    returnValue = alpha * (cost_[iRange] - cost_[iRange+1]);
               }
          }
          if (CLP_METHOD2) {
#ifdef NONLIN_DEBUG
               double saveRhs1 = rhs;
               rhs = saveRhs;
#endif
               unsigned char iStatus = status_[sequence];
               int iWhere = currentStatus(iStatus);
               if (iWhere == CLP_SAME)
                    iWhere = originalStatus(iStatus);
               // rhs always increases
               if (iWhere == CLP_FEASIBLE) {
                    if (alpha > 0.0) {
                         // going below
                         iWhere = CLP_BELOW_LOWER;
                         rhs = COIN_DBL_MAX;
                    } else {
                         // going above
                         iWhere = CLP_ABOVE_UPPER;
                         rhs = COIN_DBL_MAX;
                    }
               } else if (iWhere == CLP_BELOW_LOWER) {
                    assert (alpha < 0);
                    // going feasible
                    iWhere = CLP_FEASIBLE;
                    rhs += bound_[sequence] - model_->upperRegion()[sequence];
               } else {
                    assert (iWhere == CLP_ABOVE_UPPER);
                    // going feasible
                    iWhere = CLP_FEASIBLE;
                    rhs += model_->lowerRegion()[sequence] - bound_[sequence];
               }
               setCurrentStatus(status_[sequence], iWhere);
#ifdef NONLIN_DEBUG
               assert(saveRhs1 == rhs);
#endif
               returnValue = fabs(alpha) * infeasibilityWeight_;
          }
          return returnValue;
     }
     /// Returns current lower bound
     inline double lower(int sequence) const {
          return lower_[whichRange_[sequence] + offset_[sequence]];
     }
     /// Returns current upper bound
     inline double upper(int sequence) const {
          return lower_[whichRange_[sequence] + offset_[sequence] + 1];
     }
     /// Returns current cost
     inline double cost(int sequence) const {
          return cost_[whichRange_[sequence] + offset_[sequence]];
     }
     //@}


     /**@name Gets and sets */
     //@{
     /// Number of infeasibilities
     inline int numberInfeasibilities() const {
          return numberInfeasibilities_;
     }
     /// Change in cost
     inline double changeInCost() const {
          return changeCost_;
     }
     /// Feasible cost
     inline double feasibleCost() const {
          return feasibleCost_;
     }
     /// Feasible cost with offset and direction (i.e. for reporting)
     double feasibleReportCost() const;
     /// Sum of infeasibilities
     inline double sumInfeasibilities() const {
          return sumInfeasibilities_;
     }
     /// Largest infeasibility
     inline double largestInfeasibility() const {
          return largestInfeasibility_;
     }
     /// Average theta
     inline double averageTheta() const {
          return averageTheta_;
     }
     inline void setAverageTheta(double value) {
          averageTheta_ = value;
     }
     inline void setChangeInCost(double value) {
          changeCost_ = value;
     }
     inline void setMethod(int value) {
          method_ = value;
     }
     /// See if may want to look both ways
     inline bool lookBothWays() const {
          return bothWays_;
     }
     //@}
     ///@name Private functions to deal with infeasible regions
     inline bool infeasible(int i) const {
          return ((infeasible_[i>>5] >> (i & 31)) & 1) != 0;
     }
     inline void setInfeasible(int i, bool trueFalse) {
          unsigned int & value = infeasible_[i>>5];
          int bit = i & 31;
          if (trueFalse)
               value |= (1 << bit);
          else
               value &= ~(1 << bit);
     }
     inline unsigned char * statusArray() const {
          return status_;
     }
     /// For debug
     void validate();
     //@}

private:
     /**@name Data members */
     //@{
     /// Change in cost because of infeasibilities
     double changeCost_;
     /// Feasible cost
     double feasibleCost_;
     /// Current infeasibility weight
     double infeasibilityWeight_;
     /// Largest infeasibility
     double largestInfeasibility_;
     /// Sum of infeasibilities
     double sumInfeasibilities_;
     /// Average theta - kept here as only for primal
     double averageTheta_;
     /// Number of rows (mainly for checking and copy)
     int numberRows_;
     /// Number of columns (mainly for checking and copy)
     int numberColumns_;
     /// Starts for each entry (columns then rows)
     int * start_;
     /// Range for each entry (columns then rows)
     int * whichRange_;
     /// Temporary range offset for each entry (columns then rows)
     int * offset_;
     /** Lower bound for each range (upper bound is next lower).
         For various reasons there is always an infeasible range
         at bottom - even if lower bound is - infinity */
     double * lower_;
     /// Cost for each range
     double * cost_;
     /// Model
     ClpSimplex * model_;
     // Array to say which regions are infeasible
     unsigned int * infeasible_;
     /// Number of infeasibilities found
     int numberInfeasibilities_;
     // new stuff
     /// Contains status at beginning and current
     unsigned char * status_;
     /// Bound which has been replaced in lower_ or upper_
     double * bound_;
     /// Feasible cost array
     double * cost2_;
     /// Method 1 old, 2 new, 3 both!
     int method_;
     /// If all non-linear costs convex
     bool convex_;
     /// If we should look both ways for djs
     bool bothWays_;
     //@}
};

#endif
