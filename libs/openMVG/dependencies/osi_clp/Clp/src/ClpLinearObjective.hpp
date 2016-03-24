/* $Id$ */
// Copyright (C) 2003, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef ClpLinearObjective_H
#define ClpLinearObjective_H

#include "ClpObjective.hpp"

//#############################################################################

/** Linear Objective Class

*/

class ClpLinearObjective : public ClpObjective {

public:

     ///@name Stuff
     //@{

     /** Returns objective coefficients.
       
       Offset is always set to 0.0. All other parameters unused.
     */
     virtual double * gradient(const ClpSimplex * model,
                               const double * solution, double & offset, bool refresh,
                               int includeLinear = 2);
     /** Returns reduced gradient.Returns an offset (to be added to current one).
     */
     virtual double reducedGradient(ClpSimplex * model, double * region,
                                    bool useFeasibleCosts);
     /** Returns step length which gives minimum of objective for
         solution + theta * change vector up to maximum theta.

         arrays are numberColumns+numberRows
         Also sets current objective, predicted and at maximumTheta
     */
     virtual double stepLength(ClpSimplex * model,
                               const double * solution,
                               const double * change,
                               double maximumTheta,
                               double & currentObj,
                               double & predictedObj,
                               double & thetaObj);
     /// Return objective value (without any ClpModel offset) (model may be NULL)
     virtual double objectiveValue(const ClpSimplex * model, const double * solution) const ;
     /// Resize objective
     virtual void resize(int newNumberColumns) ;
     /// Delete columns in  objective
     virtual void deleteSome(int numberToDelete, const int * which) ;
     /// Scale objective
     virtual void reallyScale(const double * columnScale) ;

     //@}


     ///@name Constructors and destructors
     //@{
     /// Default Constructor
     ClpLinearObjective();

     /// Constructor from objective
     ClpLinearObjective(const double * objective, int numberColumns);

     /// Copy constructor
     ClpLinearObjective(const ClpLinearObjective &);
     /** Subset constructor.  Duplicates are allowed
         and order is as given.
     */
     ClpLinearObjective (const ClpLinearObjective &rhs, int numberColumns,
                         const int * whichColumns) ;

     /// Assignment operator
     ClpLinearObjective & operator=(const ClpLinearObjective& rhs);

     /// Destructor
     virtual ~ClpLinearObjective ();

     /// Clone
     virtual ClpObjective * clone() const;
     /** Subset clone.  Duplicates are allowed
         and order is as given.
     */
     virtual ClpObjective * subsetClone (int numberColumns,
                                         const int * whichColumns) const;

     //@}

     //---------------------------------------------------------------------------

private:
     ///@name Private member data
     /// Objective
     double * objective_;
     /// number of columns
     int numberColumns_;
     //@}
};

#endif
