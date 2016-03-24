/* $Id$ */
// Copyright (C) 2004, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef ClpCholeskyTaucs_H
#define ClpCholeskyTaucs_H
#include "taucs.h"
#include "ClpCholeskyBase.hpp"
class ClpMatrixBase;


/** Taucs class for Clp Cholesky factorization

If  you wish to use Sivan Toledo's TAUCS code see

http://www.tau.ac.il/~stoledo/taucs/

for terms of use

The taucs.h file was modified to put

#ifdef __cplusplus
extern "C"{
#endif
               after line 440 (#endif) and
#ifdef __cplusplus
          }
#endif
               at end

I also modified LAPACK dpotf2.f (two places) to change the GO TO 30 on AJJ.Lt.0.0

to

            IF( AJJ.LE.1.0e-20 ) THEN
               AJJ = 1.0e100;
            ELSE
               AJJ = SQRT( AJJ )
            END IF

*/
class ClpCholeskyTaucs : public ClpCholeskyBase {

public:
     /**@name Virtual methods that the derived classes provides  */
     //@{
     /** Orders rows and saves pointer to matrix.and model.
      Returns non-zero if not enough memory */
     virtual int order(ClpInterior * model) ;
     /// Dummy
     virtual int symbolic();
     /** Factorize - filling in rowsDropped and returning number dropped.
         If return code negative then out of memory */
     virtual int factorize(const double * diagonal, int * rowsDropped) ;
     /** Uses factorization to solve. */
     virtual void solve (double * region) ;
     //@}


     /**@name Constructors, destructor */
     //@{
     /** Default constructor. */
     ClpCholeskyTaucs();
     /** Destructor  */
     virtual ~ClpCholeskyTaucs();
     // Copy
     ClpCholeskyTaucs(const ClpCholeskyTaucs&);
     // Assignment
     ClpCholeskyTaucs& operator=(const ClpCholeskyTaucs&);
     /// Clone
     virtual ClpCholeskyBase * clone() const ;
     //@}


private:
     /**@name Data members */
     //@{
     /// Taucs matrix (== sparseFactor etc)
     taucs_ccs_matrix * matrix_;
     /// Taucs factor
     void * factorization_;
     /// sparseFactor.
     double * sparseFactorT_;
     /// choleskyStart
     CoinBigIndex * choleskyStartT_;
     /// choleskyRow
     int * choleskyRowT_;
     /// sizeFactor.
     CoinBigIndex sizeFactorT_;
     /// Row copy of matrix
     ClpMatrixBase * rowCopyT_;
     //@}
};

#endif
