/* $Id$ */
// Copyright (C) 2003, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef ClpLsqr_H_
#define ClpLsqr_H_

#include "CoinDenseVector.hpp"

#include "ClpInterior.hpp"


/**
This class implements LSQR

@verbatim
 LSQR solves  Ax = b  or  min ||b - Ax||_2  if damp = 0,
 or   min || (b)  -  (  A   )x ||   otherwise.
          || (0)     (damp I)  ||2
 A  is an m by n matrix defined by user provided routines
 matVecMult(mode, y, x)
 which performs the matrix-vector operations where y and x
 are references or pointers to CoinDenseVector objects.
 If mode = 1, matVecMult  must return  y = Ax   without altering x.
 If mode = 2, matVecMult  must return  y = A'x  without altering x.

-----------------------------------------------------------------------
 LSQR uses an iterative (conjugate-gradient-like) method.
 For further information, see
 1. C. C. Paige and M. A. Saunders (1982a).
    LSQR: An algorithm for sparse linear equations and sparse least squares,
    ACM TOMS 8(1), 43-71.
 2. C. C. Paige and M. A. Saunders (1982b).
    Algorithm 583.  LSQR: Sparse linear equations and least squares problems,
    ACM TOMS 8(2), 195-209.
 3. M. A. Saunders (1995).  Solution of sparse rectangular systems using
    LSQR and CRAIG, BIT 35, 588-604.

 Input parameters:

 atol, btol  are stopping tolerances.  If both are 1.0e-9 (say),
             the final residual norm should be accurate to about 9 digits.
             (The final x will usually have fewer correct digits,
             depending on cond(A) and the size of damp.)
 conlim      is also a stopping tolerance.  lsqr terminates if an estimate
             of cond(A) exceeds conlim.  For compatible systems Ax = b,
             conlim could be as large as 1.0e+12 (say).  For least-squares
             problems, conlim should be less than 1.0e+8.
             Maximum precision can be obtained by setting
             atol = btol = conlim = zero, but the number of iterations
             may then be excessive.
 itnlim      is an explicit limit on iterations (for safety).
 show = 1    gives an iteration log,
 show = 0    suppresses output.
 info        is a structure special to pdco.m, used to test if
             was small enough, and continuing if necessary with smaller atol.


 Output parameters:
 x           is the final solution.
 *istop       gives the reason for termination.
 *istop       = 1 means x is an approximate solution to Ax = b.
             = 2 means x approximately solves the least-squares problem.
 rnorm       = norm(r) if damp = 0, where r = b - Ax,
             = sqrt( norm(r)**2  +  damp**2 * norm(x)**2 ) otherwise.
 xnorm       = norm(x).
 var         estimates diag( inv(A'A) ).  Omitted in this special version.
 outfo       is a structure special to pdco.m, returning information
             about whether atol had to be reduced.

 Other potential output parameters:
 anorm, acond, arnorm, xnorm
@endverbatim
 */
class ClpLsqr {
private:
     /**@name Private member data */
     //@{
     //@}

public:
     /**@name Public member data */
     //@{
     /// Row dimension of matrix
     int          nrows_;
     /// Column dimension of matrix
     int          ncols_;
     /// Pointer to Model object for this instance
     ClpInterior        *model_;
     /// Diagonal array 1
     double         *diag1_;
     /// Constant diagonal 2
     double         diag2_;
     //@}

     /**@name Constructors and destructors */
     /** Default constructor */
     ClpLsqr();

     /** Constructor for use with Pdco model (note modified for pdco!!!!) */
     ClpLsqr(ClpInterior *model);
     /// Copy constructor
     ClpLsqr(const ClpLsqr &);
     /// Assignment operator. This copies the data
     ClpLsqr & operator=(const ClpLsqr & rhs);
     /** Destructor */
     ~ClpLsqr();
     //@}

     /**@name Methods */
     //@{
     /// Set an int parameter
     bool setParam(char *parmName, int parmValue);
     /// Call the Lsqr algorithm
     void do_lsqr( CoinDenseVector<double> &b,
                   double damp, double atol, double btol, double conlim, int itnlim,
                   bool show, Info info, CoinDenseVector<double> &x , int *istop,
                   int *itn, Outfo *outfo, bool precon, CoinDenseVector<double> &Pr );
     /// Matrix-vector multiply - implemented by user
     void matVecMult( int, CoinDenseVector<double> *, CoinDenseVector<double> *);

     void matVecMult( int, CoinDenseVector<double> &, CoinDenseVector<double> &);
     /// diag1 - we just borrow as it is part of a CoinDenseVector<double>
     void borrowDiag1(double * array) {
          diag1_ = array;
     };
     //@}
};
#endif




