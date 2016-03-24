/* $Id$ */
// Copyright (C) 2009, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef ClpCholeskyMumps_H
#define ClpCholeskyMumps_H
#include "ClpCholeskyBase.hpp"
class ClpMatrixBase;
class ClpCholeskyDense;

// unfortunately, DMUMPS_STRUC_C is an anonymous struct in MUMPS, so we define it to void for everyone outside ClpCholeskyMumps
// if this file is included by ClpCholeskyMumps.cpp, then after dmumps_c.h has been included, which defines MUMPS_VERSION
#ifndef MUMPS_VERSION
typedef void DMUMPS_STRUC_C;
#endif

/** Mumps class for Clp Cholesky factorization

*/
class ClpCholeskyMumps : public ClpCholeskyBase {

public:
     /**@name Virtual methods that the derived classes provides  */
     //@{
     /** Orders rows and saves pointer to matrix.and model.
      Returns non-zero if not enough memory */
     virtual int order(ClpInterior * model) ;
     /** Does Symbolic factorization given permutation.
         This is called immediately after order.  If user provides this then
         user must provide factorize and solve.  Otherwise the default factorization is used
         returns non-zero if not enough memory */
     virtual int symbolic();
     /** Factorize - filling in rowsDropped and returning number dropped.
         If return code negative then out of memory */
     virtual int factorize(const double * diagonal, int * rowsDropped) ;
     /** Uses factorization to solve. */
     virtual void solve (double * region) ;
     //@}


     /**@name Constructors, destructor */
     //@{
     /** Constructor which has dense columns activated.
         Default is off. */
     ClpCholeskyMumps(int denseThreshold = -1);
     /** Destructor  */
     virtual ~ClpCholeskyMumps();
     /// Clone
     virtual ClpCholeskyBase * clone() const ;
     //@}

private:
     // Mumps structure
     DMUMPS_STRUC_C* mumps_;
     
          // Copy
     ClpCholeskyMumps(const ClpCholeskyMumps&);
     // Assignment
     ClpCholeskyMumps& operator=(const ClpCholeskyMumps&);
};

#endif
