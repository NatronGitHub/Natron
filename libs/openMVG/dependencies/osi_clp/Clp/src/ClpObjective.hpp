/* $Id$ */
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef ClpObjective_H
#define ClpObjective_H


//#############################################################################
class ClpSimplex;
class ClpModel;

/** Objective Abstract Base Class

Abstract Base Class for describing an objective function

*/
class ClpObjective  {

public:

     ///@name Stuff
     //@{

     /** Returns gradient.  If Linear then solution may be NULL,
         also returns an offset (to be added to current one)
         If refresh is false then uses last solution
         Uses model for scaling
         includeLinear 0 - no, 1 as is, 2 as feasible
     */
     virtual double * gradient(const ClpSimplex * model,
                               const double * solution,
                               double & offset, bool refresh,
                               int includeLinear = 2) = 0;
     /** Returns reduced gradient.Returns an offset (to be added to current one).
     */
     virtual double reducedGradient(ClpSimplex * model, double * region,
                                    bool useFeasibleCosts) = 0;
     /** Returns step length which gives minimum of objective for
         solution + theta * change vector up to maximum theta.

         arrays are numberColumns+numberRows
         Also sets current objective, predicted  and at maximumTheta
     */
     virtual double stepLength(ClpSimplex * model,
                               const double * solution,
                               const double * change,
                               double maximumTheta,
                               double & currentObj,
                               double & predictedObj,
                               double & thetaObj) = 0;
     /// Return objective value (without any ClpModel offset) (model may be NULL)
     virtual double objectiveValue(const ClpSimplex * model, const double * solution) const = 0;
     /// Resize objective
     virtual void resize(int newNumberColumns) = 0;
     /// Delete columns in  objective
     virtual void deleteSome(int numberToDelete, const int * which) = 0;
     /// Scale objective
     virtual void reallyScale(const double * columnScale) = 0;
     /** Given a zeroed array sets nonlinear columns to 1.
         Returns number of nonlinear columns
      */
     virtual int markNonlinear(char * which);
     /// Say we have new primal solution - so may need to recompute
     virtual void newXValues() {}
     //@}


     ///@name Constructors and destructors
     //@{
     /// Default Constructor
     ClpObjective();

     /// Copy constructor
     ClpObjective(const ClpObjective &);

     /// Assignment operator
     ClpObjective & operator=(const ClpObjective& rhs);

     /// Destructor
     virtual ~ClpObjective ();

     /// Clone
     virtual ClpObjective * clone() const = 0;
     /** Subset clone.  Duplicates are allowed
         and order is as given.
         Derived classes need not provide this as it may not always make
         sense */
     virtual ClpObjective * subsetClone (int numberColumns,
                                         const int * whichColumns) const;

     //@}

     ///@name Other
     //@{
     /// Returns type (above 63 is extra information)
     inline int type() const {
          return type_;
     }
     /// Sets type (above 63 is extra information)
     inline void setType(int value) {
          type_ = value;
     }
     /// Whether activated
     inline int activated() const {
          return activated_;
     }
     /// Set whether activated
     inline void setActivated(int value) {
          activated_ = value;
     }

     /// Objective offset
     inline double nonlinearOffset () const {
          return offset_;
     }
     //@}

     //---------------------------------------------------------------------------

protected:
     ///@name Protected member data
     //@{
     /// Value of non-linear part of objective
     double offset_;
     /// Type of objective - linear is 1
     int type_;
     /// Whether activated
     int activated_;
     //@}
};

#endif
