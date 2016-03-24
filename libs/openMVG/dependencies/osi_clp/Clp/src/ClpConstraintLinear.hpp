/* $Id$ */
// Copyright (C) 2007, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef ClpConstraintLinear_H
#define ClpConstraintLinear_H

#include "ClpConstraint.hpp"

//#############################################################################

/** Linear Constraint Class

*/

class ClpConstraintLinear : public ClpConstraint {

public:

     ///@name Stuff
     //@{


     /** Fills gradient.  If Linear then solution may be NULL,
         also returns true value of function and offset so we can use x not deltaX in constraint
         If refresh is false then uses last solution
         Uses model for scaling
         Returns non-zero if gradient udefined at current solution
     */
     virtual int gradient(const ClpSimplex * model,
                          const double * solution,
                          double * gradient,
                          double & functionValue ,
                          double & offset,
                          bool useScaling = false,
                          bool refresh = true) const ;
     /// Resize constraint
     virtual void resize(int newNumberColumns) ;
     /// Delete columns in  constraint
     virtual void deleteSome(int numberToDelete, const int * which) ;
     /// Scale constraint
     virtual void reallyScale(const double * columnScale) ;
     /** Given a zeroed array sets nonlinear columns to 1.
         Returns number of nonlinear columns
      */
     virtual int markNonlinear(char * which) const ;
     /** Given a zeroed array sets possible nonzero coefficients to 1.
         Returns number of nonzeros
      */
     virtual int markNonzero(char * which) const;
     //@}


     ///@name Constructors and destructors
     //@{
     /// Default Constructor
     ClpConstraintLinear();

     /// Constructor from constraint
     ClpConstraintLinear(int row, int numberCoefficients, int numberColumns,
                         const int * column, const double * element);

     /** Copy constructor .
     */
     ClpConstraintLinear(const ClpConstraintLinear & rhs);

     /// Assignment operator
     ClpConstraintLinear & operator=(const ClpConstraintLinear& rhs);

     /// Destructor
     virtual ~ClpConstraintLinear ();

     /// Clone
     virtual ClpConstraint * clone() const;
     //@}
     ///@name Gets and sets
     //@{
     /// Number of coefficients
     virtual int numberCoefficients() const;
     /// Number of columns in linear constraint
     inline int numberColumns() const {
          return numberColumns_;
     }
     /// Columns
     inline const int * column() const {
          return column_;
     }
     /// Coefficients
     inline const double * coefficient() const {
          return coefficient_;
     }
     //@}

     //---------------------------------------------------------------------------

private:
     ///@name Private member data
     /// Column
     int * column_;
     /// Coefficients
     double * coefficient_;
     /// Useful to have number of columns about
     int numberColumns_;
     /// Number of coefficients
     int numberCoefficients_;
     //@}
};

#endif
