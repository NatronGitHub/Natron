/* $Id$ */
// Copyright (C) 2004, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef ClpCholeskyWssmpKKT_H
#define ClpCholeskyWssmpKKT_H

#include "ClpCholeskyBase.hpp"
class ClpMatrixBase;
class ClpCholeskyDense;


/** WssmpKKT class for Clp Cholesky factorization

*/
class ClpCholeskyWssmpKKT : public ClpCholeskyBase {

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
     /** Uses factorization to solve. - given as if KKT.
      region1 is rows+columns, region2 is rows */
     virtual void solveKKT (double * region1, double * region2, const double * diagonal,
                            double diagonalScaleFactor);
     //@}


     /**@name Constructors, destructor */
     //@{
     /** Constructor which has dense columns activated.
         Default is off. */
     ClpCholeskyWssmpKKT(int denseThreshold = -1);
     /** Destructor  */
     virtual ~ClpCholeskyWssmpKKT();
     // Copy
     ClpCholeskyWssmpKKT(const ClpCholeskyWssmpKKT&);
     // Assignment
     ClpCholeskyWssmpKKT& operator=(const ClpCholeskyWssmpKKT&);
     /// Clone
     virtual ClpCholeskyBase * clone() const ;
     //@}


private:
     /**@name Data members */
     //@{
     //@}
};

#endif
