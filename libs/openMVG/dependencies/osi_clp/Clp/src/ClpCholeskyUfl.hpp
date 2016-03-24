/* $Id$ */
// Copyright (C) 2004, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef ClpCholeskyUfl_H
#define ClpCholeskyUfl_H

#include "ClpCholeskyBase.hpp"

class ClpMatrixBase;
class ClpCholeskyDense;

typedef struct cholmod_factor_struct cholmod_factor;
typedef struct cholmod_common_struct cholmod_common;

/** Ufl class for Clp Cholesky factorization

If  you wish to use AMD code from University of Florida see

    http://www.cise.ufl.edu/research/sparse/amd

for terms of use

If  you wish to use CHOLMOD code from University of Florida see

    http://www.cise.ufl.edu/research/sparse/cholmod

for terms of use

*/
class ClpCholeskyUfl : public ClpCholeskyBase {

public:
     /**@name Virtual methods that the derived classes provides  */
     //@{
     /** Orders rows and saves pointer to matrix.and model.
      Returns non-zero if not enough memory */
     virtual int order(ClpInterior * model) ;
     /** Does Symbolic factorization given permutation using CHOLMOD (if available).
         This is called immediately after order.  If user provides this then
         user must provide factorize and solve.  Otherwise the default factorization is used
         returns non-zero if not enough memory. */
     virtual int symbolic();
     /** Factorize - filling in rowsDropped and returning number dropped using CHOLMOD (if available).
         If return code negative then out of memory */
     virtual int factorize(const double * diagonal, int * rowsDropped) ;
     /** Uses factorization to solve. Uses CHOLMOD (if available). */
     virtual void solve (double * region) ;
     //@}


     /**@name Constructors, destructor */
     //@{
     /** Constructor which has dense columns activated.
         Default is off. */
     ClpCholeskyUfl(int denseThreshold = -1);
     /** Destructor  */
     virtual ~ClpCholeskyUfl();
     /// Clone
     virtual ClpCholeskyBase * clone() const ;
     //@}


private:
     cholmod_factor * L_ ;
     cholmod_common * c_ ;

     // Copy
     ClpCholeskyUfl(const ClpCholeskyUfl&);
     // Assignment
     ClpCholeskyUfl& operator=(const ClpCholeskyUfl&);
};

#endif
