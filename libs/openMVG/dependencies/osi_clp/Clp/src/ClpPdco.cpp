/* $Id$ */
// Copyright (C) 2003, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

/* Pdco algorithm

Method


*/



#include "CoinPragma.hpp"

#include <math.h>

#include "CoinDenseVector.hpp"
#include "ClpPdco.hpp"
#include "ClpPdcoBase.hpp"
#include "CoinHelperFunctions.hpp"
#include "ClpHelperFunctions.hpp"
#include "ClpLsqr.hpp"
#include "CoinTime.hpp"
#include "ClpMessage.hpp"
#include <cfloat>
#include <cassert>
#include <string>
#include <stdio.h>
#include <iostream>

int
ClpPdco::pdco()
{
     printf("Dummy pdco solve\n");
     return 0;
}
// ** Temporary version
int
ClpPdco::pdco( ClpPdcoBase * stuff, Options &options, Info &info, Outfo &outfo)
{
//    D1, D2 are positive-definite diagonal matrices defined from d1, d2.
//           In particular, d2 indicates the accuracy required for
//           satisfying each row of Ax = b.
//
// D1 and D2 (via d1 and d2) provide primal and dual regularization
// respectively.  They ensure that the primal and dual solutions
// (x,r) and (y,z) are unique and bounded.
//
// A scalar d1 is equivalent to d1 = ones(n,1), D1 = diag(d1).
// A scalar d2 is equivalent to d2 = ones(m,1), D2 = diag(d2).
// Typically, d1 = d2 = 1e-4.
// These values perturb phi(x) only slightly  (by about 1e-8) and request
// that A*x = b be satisfied quite accurately (to about 1e-8).
// Set d1 = 1e-4, d2 = 1 for least-squares problems with bound constraints.
// The problem is then
//
//    minimize    phi(x) + 1/2 norm(d1*x)^2 + 1/2 norm(A*x - b)^2
//    subject to  bl <= x <= bu.
//
// More generally, d1 and d2 may be n and m vectors containing any positive
// values (preferably not too small, and typically no larger than 1).
// Bigger elements of d1 and d2 improve the stability of the solver.
//
// At an optimal solution, if x(j) is on its lower or upper bound,
// the corresponding z(j) is positive or negative respectively.
// If x(j) is between its bounds, z(j) = 0.
// If bl(j) = bu(j), x(j) is fixed at that value and z(j) may have
// either sign.
//
// Also, r and y satisfy r = D2 y, so that Ax + D2^2 y = b.
// Thus if d2(i) = 1e-4, the i-th row of Ax = b will be satisfied to
// approximately 1e-8.  This determines how large d2(i) can safely be.
//
//
// EXTERNAL FUNCTIONS:
// options         = pdcoSet;                  provided with pdco.m
// [obj,grad,hess] = pdObj( x );               provided by user
//               y = pdMat( name,mode,m,n,x ); provided by user if pdMat
//                                             is a string, not a matrix
//
// INPUT ARGUMENTS:
// pdObj      is a string containing the name of a function pdObj.m
//            or a function_handle for such a function
//            such that [obj,grad,hess] = pdObj(x) defines
//            obj  = phi(x)              : a scalar,
//            grad = gradient of phi(x)  : an n-vector,
//            hess = diag(Hessian of phi): an n-vector.
//         Examples:
//            If phi(x) is the linear function c"x, pdObj should return
//               [obj,grad,hess] = [c"*x, c, zeros(n,1)].
//            If phi(x) is the entropy function E(x) = sum x(j) log x(j),
//               [obj,grad,hess] = [E(x), log(x)+1, 1./x].
// pdMat      may be an ifexplicit m x n matrix A (preferably sparse!),
//            or a string containing the name of a function pdMat.m
//            or a function_handle for such a function
//            such that y = pdMat( name,mode,m,n,x )
//            returns   y = A*x (mode=1)  or  y = A"*x (mode=2).
//            The input parameter "name" will be the string pdMat.
// b          is an m-vector.
// bl         is an n-vector of lower bounds.  Non-existent bounds
//            may be represented by bl(j) = -Inf or bl(j) <= -1e+20.
// bu         is an n-vector of upper bounds.  Non-existent bounds
//            may be represented by bu(j) =  Inf or bu(j) >=  1e+20.
// d1, d2     may be positive scalars or positive vectors (see above).
// options    is a structure that may be set and altered by pdcoSet
//            (type help pdcoSet).
// x0, y0, z0 provide an initial solution.
// xsize, zsize are estimates of the biggest x and z at the solution.
//            They are used to scale (x,y,z).  Good estimates
//            should improve the performance of the barrier method.
//
//
// OUTPUT ARGUMENTS:
// x          is the primal solution.
// y          is the dual solution associated with Ax + D2 r = b.
// z          is the dual solution associated with bl <= x <= bu.
// inform = 0 if a solution is found;
//        = 1 if too many iterations were required;
//        = 2 if the linesearch failed too often.
// PDitns     is the number of Primal-Dual Barrier iterations required.
// CGitns     is the number of Conjugate-Gradient  iterations required
//            if an iterative solver is used (LSQR).
// time       is the cpu time used.
//----------------------------------------------------------------------

// PRIVATE FUNCTIONS:
//    pdxxxbounds
//    pdxxxdistrib
//    pdxxxlsqr
//    pdxxxlsqrmat
//    pdxxxmat
//    pdxxxmerit
//    pdxxxresid1
//    pdxxxresid2
//    pdxxxstep
//
// GLOBAL VARIABLES:
//    global pdDDD1 pdDDD2 pdDDD3
//
//
// NOTES:
// The matrix A should be reasonably well scaled: norm(A,inf) =~ 1.
// The vector b and objective phi(x) may be of any size, but ensure that
// xsize and zsize are reasonably close to norm(x,inf) and norm(z,inf)
// at the solution.
//
// The files defining pdObj  and pdMat
// must not be called Fname.m or Aname.m!!
//
//
// AUTHOR:
//    Michael Saunders, Systems Optimization Laboratory (SOL),
//    Stanford University, Stanford, California, USA.
//    saunders@stanford.edu
//
// CONTRIBUTORS:
//    Byunggyoo Kim, SOL, Stanford University.
//    bgkim@stanford.edu
//
// DEVELOPMENT:
// 20 Jun 1997: Original version of pdsco.m derived from pdlp0.m.
// 29 Sep 2002: Original version of pdco.m  derived from pdsco.m.
//              Introduced D1, D2 in place of gamma*I, delta*I
//              and allowed for general bounds bl <= x <= bu.
// 06 Oct 2002: Allowed for fixed variabes: bl(j) = bu(j) for any j.
// 15 Oct 2002: Eliminated some work vectors (since m, n might be LARGE).
//              Modularized residuals, linesearch
// 16 Oct 2002: pdxxx..., pdDDD... names rationalized.
//              pdAAA eliminated (global copy of A).
//              Aname is now used directly as an ifexplicit A or a function.
//              NOTE: If Aname is a function, it now has an extra parameter.
// 23 Oct 2002: Fname and Aname can now be function handles.
// 01 Nov 2002: Bug fixed in feval in pdxxxmat.
//-----------------------------------------------------------------------

//  global pdDDD1 pdDDD2 pdDDD3
     double inf = 1.0e30;
     double eps = 1.0e-15;
     double atolold = -1.0, r3ratio = -1.0, Pinf, Dinf, Cinf, Cinf0;

     printf("\n   --------------------------------------------------------");
     printf("\n   pdco.m                            Version of 01 Nov 2002");
     printf("\n   Primal-dual barrier method to minimize a convex function");
     printf("\n   subject to linear constraints Ax + r = b,  bl <= x <= bu");
     printf("\n   --------------------------------------------------------\n");

     int m = numberRows_;
     int n = numberColumns_;
     bool ifexplicit = true;

     CoinDenseVector<double> b(m, rhs_);
     CoinDenseVector<double> x(n, x_);
     CoinDenseVector<double> y(m, y_);
     CoinDenseVector<double> z(n, dj_);
     //delete old arrays
     delete [] rhs_;
     delete [] x_;
     delete [] y_;
     delete [] dj_;
     rhs_ = NULL;
     x_ = NULL;
     y_ = NULL;
     dj_ = NULL;

     // Save stuff so available elsewhere
     pdcoStuff_ = stuff;

     double normb  = b.infNorm();
     double normx0 = x.infNorm();
     double normy0 = y.infNorm();
     double normz0 = z.infNorm();

     printf("\nmax |b | = %8g     max |x0| = %8g", normb , normx0);
     printf(                "      xsize   = %8g", xsize_);
     printf("\nmax |y0| = %8g     max |z0| = %8g", normy0, normz0);
     printf(                "      zsize   = %8g", zsize_);

     //---------------------------------------------------------------------
     // Initialize.
     //---------------------------------------------------------------------
     //true   = 1;
     //false  = 0;
     //zn     = zeros(n,1);
     //int nb     = n + m;
     int CGitns = 0;
     int inform = 0;
     //---------------------------------------------------------------------
     //  Only allow scalar d1, d2 for now
     //---------------------------------------------------------------------
     /*
     if (d1_->size()==1)
         d1_->resize(n, d1_->getElements()[0]);  // Allow scalar d1, d2
     if (d2_->size()==1)
         d2->resize(m, d2->getElements()[0]);  // to mean dk * unit vector
      */
     assert (stuff->sizeD1() == 1);
     double d1 = stuff->getD1();
     double d2 = stuff->getD2();

     //---------------------------------------------------------------------
     // Grab input options.
     //---------------------------------------------------------------------
     int  maxitn    = options.MaxIter;
     double featol    = options.FeaTol;
     double opttol    = options.OptTol;
     double steptol   = options.StepTol;
     int  stepSame  = 1;  /* options.StepSame;   // 1 means stepx == stepz */
     double x0min     = options.x0min;
     double z0min     = options.z0min;
     double mu0       = options.mu0;
     int  LSproblem = options.LSproblem;  // See below
     int  LSmethod  = options.LSmethod;   // 1=Cholesky    2=QR    3=LSQR
     int  itnlim    = options.LSQRMaxIter * CoinMin(m, n);
     double atol1     = options.LSQRatol1;  // Initial  atol
     double atol2     = options.LSQRatol2;  // Smallest atol,unless atol1 is smaller
     double conlim    = options.LSQRconlim;
     //int  wait      = options.wait;

     // LSproblem:
     //  1 = dy          2 = dy shifted, DLS
     // 11 = s          12 =  s shifted, DLS    (dx = Ds)
     // 21 = dx
     // 31 = 3x3 system, symmetrized by Z^{1/2}
     // 32 = 2x2 system, symmetrized by X^{1/2}

     //---------------------------------------------------------------------
     // Set other parameters.
     //---------------------------------------------------------------------
     int  kminor    = 0;      // 1 stops after each iteration
     double eta       = 1e-4;   // Linesearch tolerance for "sufficient descent"
     double maxf      = 10;     // Linesearch backtrack limit (function evaluations)
     double maxfail   = 1;      // Linesearch failure limit (consecutive iterations)
     double bigcenter = 1e+3;   // mu is reduced if center < bigcenter.

     // Parameters for LSQR.
     double atolmin   = eps;    // Smallest atol if linesearch back-tracks
     double btol      = 0;      // Should be small (zero is ok)
     double show      = false;  // Controls lsqr iteration log
     /*
     double gamma     = d1->infNorm();
     double delta     = d2->infNorm();
     */
     double gamma = d1;
     double delta = d2;

     printf("\n\nx0min    = %8g     featol   = %8.1e", x0min, featol);
     printf(                  "      d1max   = %8.1e", gamma);
     printf(  "\nz0min    = %8g     opttol   = %8.1e", z0min, opttol);
     printf(                  "      d2max   = %8.1e", delta);
     printf(  "\nmu0      = %8.1e     steptol  = %8g", mu0  , steptol);
     printf(                  "     bigcenter= %8g"  , bigcenter);

     printf("\n\nLSQR:");
     printf("\natol1    = %8.1e     atol2    = %8.1e", atol1 , atol2 );
     printf(                  "      btol    = %8.1e", btol );
     printf("\nconlim   = %8.1e     itnlim   = %8d"  , conlim, itnlim);
     printf(                  "      show    = %8g"  , show );

// LSmethod  = 3;  ////// Hardwire LSQR
// LSproblem = 1;  ////// and LS problem defining "dy".
     /*
       if wait
         printf("\n\nReview parameters... then type "return"\n")
         keyboard
       end
     */
     if (eta < 0)
          printf("\n\nLinesearch disabled by eta < 0");

     //---------------------------------------------------------------------
     // All parameters have now been set.
     //---------------------------------------------------------------------
     double time    = CoinCpuTime();
     //bool useChol = (LSmethod == 1);
     //bool useQR   = (LSmethod == 2);
     bool direct  = (LSmethod <= 2 && ifexplicit);
     char solver[6];
     strcpy(solver, "  LSQR");


     //---------------------------------------------------------------------
     // Categorize bounds and allow for fixed variables by modifying b.
     //---------------------------------------------------------------------

     int nlow, nupp, nfix;
     int *bptrs[3] = {0};
     getBoundTypes(&nlow, &nupp, &nfix, bptrs );
     int *low = bptrs[0];
     int *upp = bptrs[1];
     int *fix = bptrs[2];

     int nU = n;
     if (nupp == 0) nU = 1;  //Make dummy vectors if no Upper bounds

     //---------------------------------------------------------------------
     //  Get pointers to local copy of model bounds
     //---------------------------------------------------------------------

     CoinDenseVector<double> bl(n, columnLower_);
     double *bl_elts = bl.getElements();
     CoinDenseVector<double> bu(nU, columnUpper_);  // this is dummy if no UB
     double *bu_elts = bu.getElements();

     CoinDenseVector<double> r1(m, 0.0);
     double *r1_elts = r1.getElements();
     CoinDenseVector<double> x1(n, 0.0);
     double *x1_elts = x1.getElements();

     if (nfix > 0) {
          for (int k = 0; k < nfix; k++)
               x1_elts[fix[k]] = bl[fix[k]];
          matVecMult(1, r1, x1);
          b = b - r1;
          // At some stage, might want to look at normfix = norm(r1,inf);
     }

     //---------------------------------------------------------------------
     // Scale the input data.
     // The scaled variables are
     //    xbar     = x/beta,
     //    ybar     = y/zeta,
     //    zbar     = z/zeta.
     // Define
     //    theta    = beta*zeta;
     // The scaled function is
     //    phibar   = ( 1   /theta) fbar(beta*xbar),
     //    gradient = (beta /theta) grad,
     //    Hessian  = (beta2/theta) hess.
     //---------------------------------------------------------------------
     double beta = xsize_;
     if (beta == 0) beta = 1; // beta scales b, x.
     double zeta = zsize_;
     if (zeta == 0) zeta = 1; // zeta scales y, z.
     double theta  = beta * zeta;                          // theta scales obj.
     // (theta could be anything, but theta = beta*zeta makes
     // scaled grad = grad/zeta = 1 approximately if zeta is chosen right.)

     for (int k = 0; k < nlow; k++)
          bl_elts[low[k]] = bl_elts[low[k]] / beta;
     for (int k = 0; k < nupp; k++)
          bu_elts[upp[k]] = bu_elts[upp[k]] / beta;
     d1     = d1 * ( beta / sqrt(theta) );
     d2     = d2 * ( sqrt(theta) / beta );

     double beta2  = beta * beta;
     b.scale( (1.0 / beta) );
     y.scale( (1.0 / zeta) );
     x.scale( (1.0 / beta) );
     z.scale( (1.0 / zeta) );

     //---------------------------------------------------------------------
     // Initialize vectors that are not fully used if bounds are missing.
     //---------------------------------------------------------------------
     CoinDenseVector<double> rL(n, 0.0);
     CoinDenseVector<double> cL(n, 0.0);
     CoinDenseVector<double> z1(n, 0.0);
     CoinDenseVector<double> dx1(n, 0.0);
     CoinDenseVector<double> dz1(n, 0.0);
     CoinDenseVector<double> r2(n, 0.0);

     // Assign upper bd regions (dummy if no UBs)

     CoinDenseVector<double> rU(nU, 0.0);
     CoinDenseVector<double> cU(nU, 0.0);
     CoinDenseVector<double> x2(nU, 0.0);
     CoinDenseVector<double> z2(nU, 0.0);
     CoinDenseVector<double> dx2(nU, 0.0);
     CoinDenseVector<double> dz2(nU, 0.0);

     //---------------------------------------------------------------------
     // Initialize x, y, z, objective, etc.
     //---------------------------------------------------------------------
     CoinDenseVector<double> dx(n, 0.0);
     CoinDenseVector<double> dy(m, 0.0);
     CoinDenseVector<double> Pr(m);
     CoinDenseVector<double> D(n);
     double *D_elts = D.getElements();
     CoinDenseVector<double> w(n);
     double *w_elts = w.getElements();
     CoinDenseVector<double> rhs(m + n);


     //---------------------------------------------------------------------
     // Pull out the element array pointers for efficiency
     //---------------------------------------------------------------------
     double *x_elts  = x.getElements();
     double *x2_elts = x2.getElements();
     double *z_elts  = z.getElements();
     double *z1_elts = z1.getElements();
     double *z2_elts = z2.getElements();

     for (int k = 0; k < nlow; k++) {
          x_elts[low[k]]  = CoinMax( x_elts[low[k]], bl[low[k]]);
          x1_elts[low[k]] = CoinMax( x_elts[low[k]] - bl[low[k]], x0min  );
          z1_elts[low[k]] = CoinMax( z_elts[low[k]], z0min  );
     }
     for (int k = 0; k < nupp; k++) {
          x_elts[upp[k]]  = CoinMin( x_elts[upp[k]], bu[upp[k]]);
          x2_elts[upp[k]] = CoinMax(bu[upp[k]] -  x_elts[upp[k]], x0min  );
          z2_elts[upp[k]] = CoinMax(-z_elts[upp[k]], z0min  );
     }
     //////////////////// Assume hessian is diagonal. //////////////////////

//  [obj,grad,hess] = feval( Fname, (x*beta) );
     x.scale(beta);
     double obj = getObj(x);
     CoinDenseVector<double> grad(n);
     getGrad(x, grad);
     CoinDenseVector<double> H(n);
     getHessian(x , H);
     x.scale((1.0 / beta));

     //double * g_elts = grad.getElements();
     double * H_elts = H.getElements();

     obj /= theta;                       // Scaled obj.
     grad = grad * (beta / theta) + (d1 * d1) * x; // grad includes x regularization.
     H  = H * (beta2 / theta) + (d1 * d1)      ; // H    includes x regularization.


     /*---------------------------------------------------------------------
     // Compute primal and dual residuals:
     // r1 =  b - Aprod(x) - d2*d2*y;
     // r2 =  grad - Atprod(y) + z2 - z1;
     //  rL =  bl - x + x1;
     //  rU =  x + x2 - bu; */
     //---------------------------------------------------------------------
     //  [r1,r2,rL,rU,Pinf,Dinf] = ...
     //      pdxxxresid1( Aname,fix,low,upp, ...
     //                   b,bl,bu,d1,d2,grad,rL,rU,x,x1,x2,y,z1,z2 );
     pdxxxresid1( this, nlow, nupp, nfix, low, upp, fix,
                  b, bl_elts, bu_elts, d1, d2, grad, rL, rU, x, x1, x2, y, z1, z2,
                  r1, r2, &Pinf, &Dinf);
     //---------------------------------------------------------------------
     // Initialize mu and complementarity residuals:
     //    cL   = mu*e - X1*z1.
     //    cU   = mu*e - X2*z2.
     //
     // 25 Jan 2001: Now that b and obj are scaled (and hence x,y,z),
     //              we should be able to use mufirst = mu0 (absolute value).
     //              0.1 worked poorly on StarTest1 with x0min = z0min = 0.1.
     // 29 Jan 2001: We might as well use mu0 = x0min * z0min;
     //              so that most variables are centered after a warm start.
     // 29 Sep 2002: Use mufirst = mu0*(x0min * z0min),
     //              regarding mu0 as a scaling of the initial center.
     //---------------------------------------------------------------------
     //  double mufirst = mu0*(x0min * z0min);
     double mufirst = mu0;   // revert to absolute value
     double mulast  = 0.1 * opttol;
     mulast  = CoinMin( mulast, mufirst );
     double mu      = mufirst;
     double center,  fmerit;
     pdxxxresid2( mu, nlow, nupp, low, upp, cL, cU, x1, x2,
                  z1, z2, &center, &Cinf, &Cinf0 );
     fmerit = pdxxxmerit(nlow, nupp, low, upp, r1, r2, rL, rU, cL, cU );

     // Initialize other things.

     bool  precon   = true;
     double PDitns    = 0;
     //bool converged = false;
     double atol      = atol1;
     atol2     = CoinMax( atol2, atolmin );
     atolmin   = atol2;
     //  pdDDD2    = d2;    // Global vector for diagonal matrix D2

     //  Iteration log.

     int nf      = 0;
     int itncg   = 0;
     int nfail   = 0;

     printf("\n\nItn   mu   stepx   stepz  Pinf  Dinf");
     printf("  Cinf   Objective    nf  center");
     if (direct) {
          printf("\n");
     } else {
          printf("  atol   solver   Inexact\n");
     }

     double regx = (d1 * x).twoNorm();
     double regy = (d2 * y).twoNorm();
     //  regterm = twoNorm(d1.*x)^2  +  norm(d2.*y)^2;
     double regterm = regx * regx + regy * regy;
     double objreg  = obj  +  0.5 * regterm;
     double objtrue = objreg * theta;

     printf("\n%3g                     ", PDitns        );
     printf("%6.1f%6.1f" , log10(Pinf ), log10(Dinf));
     printf("%6.1f%15.7e", log10(Cinf0), objtrue    );
     printf("   %8.1f\n"   , center                   );
     /*
     if kminor
       printf("\n\nStart of first minor itn...\n");
       keyboard
     end
     */
     //---------------------------------------------------------------------
     // Main loop.
     //---------------------------------------------------------------------
     // Lsqr
     ClpLsqr  thisLsqr(this);
     //  while (converged) {
     while(PDitns < maxitn) {
          PDitns = PDitns + 1;

          // 31 Jan 2001: Set atol according to progress, a la Inexact Newton.
          // 07 Feb 2001: 0.1 not small enough for Satellite problem.  Try 0.01.
          // 25 Apr 2001: 0.01 seems wasteful for Star problem.
          //              Now that starting conditions are better, go back to 0.1.

          double r3norm = CoinMax(Pinf,   CoinMax(Dinf,  Cinf));
          atol   = CoinMin(atol,  r3norm * 0.1);
          atol   = CoinMax(atol,  atolmin   );
          info.r3norm = r3norm;

          //-------------------------------------------------------------------
          //  Define a damped Newton iteration for solving f = 0,
          //  keeping  x1, x2, z1, z2 > 0.  We eliminate dx1, dx2, dz1, dz2
          //  to obtain the system
          //
          //     [-H2  A"  ] [ dx ] = [ w ],   H2 = H + D1^2 + X1inv Z1 + X2inv Z2,
          //     [ A   D2^2] [ dy ] = [ r1]    w  = r2 - X1inv(cL + Z1 rL)
          //                                           + X2inv(cU + Z2 rU),
          //
          //  which is equivalent to the least-squares problem
          //
          //     min || [ D A"]dy  -  [  D w   ] ||,   D = H2^{-1/2}.         (*)
          //         || [  D2 ]       [D2inv r1] ||
          //-------------------------------------------------------------------
          for (int k = 0; k < nlow; k++)
               H_elts[low[k]]  = H_elts[low[k]] + z1[low[k]] / x1[low[k]];
          for (int k = 0; k < nupp; k++)
               H[upp[k]]  = H[upp[k]] + z2[upp[k]] / x2[upp[k]];
          w = r2;
          for (int k = 0; k < nlow; k++)
               w[low[k]]  = w[low[k]] - (cL[low[k]] + z1[low[k]] * rL[low[k]]) / x1[low[k]];
          for (int k = 0; k < nupp; k++)
               w[upp[k]]  = w[upp[k]] + (cU[upp[k]] + z2[upp[k]] * rU[upp[k]]) / x2[upp[k]];

          if (LSproblem == 1) {
               //-----------------------------------------------------------------
               //  Solve (*) for dy.
               //-----------------------------------------------------------------
               H      = 1.0 / H;  // H is now Hinv (NOTE!)
               for (int k = 0; k < nfix; k++)
                    H[fix[k]] = 0;
               for (int k = 0; k < n; k++)
                    D_elts[k] = sqrt(H_elts[k]);
               thisLsqr.borrowDiag1(D_elts);
               thisLsqr.diag2_ = d2;

               if (direct) {
                    // Omit direct option for now
               } else {// Iterative solve using LSQR.
                    //rhs     = [ D.*w; r1./d2 ];
                    for (int k = 0; k < n; k++)
                         rhs[k] = D_elts[k] * w_elts[k];
                    for (int k = 0; k < m; k++)
                         rhs[n+k] = r1_elts[k] * (1.0 / d2);
                    double damp    = 0;

                    if (precon) {   // Construct diagonal preconditioner for LSQR
                         matPrecon(d2, Pr, D);
                    }
                    /*
                    	rw(7)        = precon;
                            info.atolmin = atolmin;
                            info.r3norm  = fmerit;  // Must be the 2-norm here.

                            [ dy, istop, itncg, outfo ] = ...
                       pdxxxlsqr( nb,m,"pdxxxlsqrmat",Aname,rw,rhs,damp, ...
                                  atol,btol,conlim,itnlim,show,info );


                    	thisLsqr.input->rhs_vec = &rhs;
                    	thisLsqr.input->sol_vec = &dy;
                    	thisLsqr.input->rel_mat_err = atol;
                    	thisLsqr.do_lsqr(this);
                    	*/
                    //  New version of lsqr

                    int istop;
                    dy.clear();
                    show = false;
                    info.atolmin = atolmin;
                    info.r3norm  = fmerit;  // Must be the 2-norm here.

                    thisLsqr.do_lsqr( rhs, damp, atol, btol, conlim, itnlim,
                                      show, info, dy , &istop, &itncg, &outfo, precon, Pr);
                    if (precon)
                         dy = dy * Pr;

                    if (!precon && itncg > 999999)
                         precon = true;

                    if (istop == 3  ||  istop == 7 )  // conlim or itnlim
                         printf("\n    LSQR stopped early:  istop = //%d", istop);


                    atolold   = outfo.atolold;
                    atol      = outfo.atolnew;
                    r3ratio   = outfo.r3ratio;
               }// LSproblem 1

               //      grad      = pdxxxmat( Aname,2,m,n,dy );   // grad = A"dy
               grad.clear();
               matVecMult(2, grad, dy);
               for (int k = 0; k < nfix; k++)
                    grad[fix[k]] = 0;                            // grad is a work vector
               dx = H * (grad - w);

          } else {
               perror( "This LSproblem not yet implemented\n" );
          }
          //-------------------------------------------------------------------

          CGitns += itncg;

          //-------------------------------------------------------------------
          // dx and dy are now known.  Get dx1, dx2, dz1, dz2.
          //-------------------------------------------------------------------
          for (int k = 0; k < nlow; k++) {
               dx1[low[k]] = - rL[low[k]] + dx[low[k]];
               dz1[low[k]] =  (cL[low[k]] - z1[low[k]] * dx1[low[k]]) / x1[low[k]];
          }
          for (int k = 0; k < nupp; k++) {
               dx2[upp[k]] = - rU[upp[k]] - dx[upp[k]];
               dz2[upp[k]] =  (cU[upp[k]] - z2[upp[k]] * dx2[upp[k]]) / x2[upp[k]];
          }
          //-------------------------------------------------------------------
          // Find the maximum step.
          //--------------------------------------------------------------------
          double stepx1 = pdxxxstep(nlow, low, x1, dx1 );
          double stepx2 = inf;
          if (nupp > 0)
               stepx2 = pdxxxstep(nupp, upp, x2, dx2 );
          double stepz1 = pdxxxstep( z1     , dz1      );
          double stepz2 = inf;
          if (nupp > 0)
               stepz2 = pdxxxstep( z2     , dz2      );
          double stepx  = CoinMin( stepx1, stepx2 );
          double stepz  = CoinMin( stepz1, stepz2 );
          stepx  = CoinMin( steptol * stepx, 1.0 );
          stepz  = CoinMin( steptol * stepz, 1.0 );
          if (stepSame) {                  // For NLPs, force same step
               stepx = CoinMin( stepx, stepz );   // (true Newton method)
               stepz = stepx;
          }

          //-------------------------------------------------------------------
          // Backtracking linesearch.
          //-------------------------------------------------------------------
          bool fail     =  true;
          nf       =  0;

          while (nf < maxf) {
               nf      = nf + 1;
               x       = x        +  stepx * dx;
               y       = y        +  stepz * dy;
               for (int k = 0; k < nlow; k++) {
                    x1[low[k]] = x1[low[k]]  +  stepx * dx1[low[k]];
                    z1[low[k]] = z1[low[k]]  +  stepz * dz1[low[k]];
               }
               for (int k = 0; k < nupp; k++) {
                    x2[upp[k]] = x2[upp[k]]  +  stepx * dx2[upp[k]];
                    z2[upp[k]] = z2[upp[k]]  +  stepz * dz2[upp[k]];
               }
               //      [obj,grad,hess] = feval( Fname, (x*beta) );
               x.scale(beta);
               obj = getObj(x);
               getGrad(x, grad);
               getHessian(x, H);
               x.scale((1.0 / beta));

               obj        /= theta;
               grad       = grad * (beta / theta)  +  d1 * d1 * x;
               H          = H * (beta2 / theta)  +  d1 * d1;

               //      [r1,r2,rL,rU,Pinf,Dinf] = ...
               pdxxxresid1( this, nlow, nupp, nfix, low, upp, fix,
                            b, bl_elts, bu_elts, d1, d2, grad, rL, rU, x, x1, x2,
                            y, z1, z2, r1, r2, &Pinf, &Dinf );
               //double center, Cinf, Cinf0;
               //      [cL,cU,center,Cinf,Cinf0] = ...
               pdxxxresid2( mu, nlow, nupp, low, upp, cL, cU, x1, x2, z1, z2,
                            &center, &Cinf, &Cinf0);
               double fmeritnew = pdxxxmerit(nlow, nupp, low, upp, r1, r2, rL, rU, cL, cU );
               double step      = CoinMin( stepx, stepz );

               if (fmeritnew <= (1 - eta * step)*fmerit) {
                    fail = false;
                    break;
               }

               // Merit function didn"t decrease.
               // Restore variables to previous values.
               // (This introduces a little error, but save lots of space.)

               x       = x        -  stepx * dx;
               y       = y        -  stepz * dy;
               for (int k = 0; k < nlow; k++) {
                    x1[low[k]] = x1[low[k]]  -  stepx * dx1[low[k]];
                    z1[low[k]] = z1[low[k]]  -  stepz * dz1[low[k]];
               }
               for (int k = 0; k < nupp; k++) {
                    x2[upp[k]] = x2[upp[k]]  -  stepx * dx2[upp[k]];
                    z2[upp[k]] = z2[upp[k]]  -  stepz * dz2[upp[k]];
               }
               // Back-track.
               // If it"s the first time,
               // make stepx and stepz the same.

               if (nf == 1 && stepx != stepz) {
                    stepx = step;
               } else if (nf < maxf) {
                    stepx = stepx / 2;
               }
               stepz = stepx;
          }

          if (fail) {
               printf("\n     Linesearch failed (nf too big)");
               nfail += 1;
          } else {
               nfail = 0;
          }

          //-------------------------------------------------------------------
          // Set convergence measures.
          //--------------------------------------------------------------------
          regx = (d1 * x).twoNorm();
          regy = (d2 * y).twoNorm();
          regterm = regx * regx + regy * regy;
          objreg  = obj  +  0.5 * regterm;
          objtrue = objreg * theta;

          bool primalfeas    = Pinf  <=  featol;
          bool dualfeas      = Dinf  <=  featol;
          bool complementary = Cinf0 <=  opttol;
          bool enough        = PDitns >=       4; // Prevent premature termination.
          bool converged     = primalfeas  &  dualfeas  &  complementary  &  enough;

          //-------------------------------------------------------------------
          // Iteration log.
          //-------------------------------------------------------------------
          char str1[100], str2[100], str3[100], str4[100], str5[100];
          sprintf(str1, "\n%3g%5.1f" , PDitns      , log10(mu)   );
          sprintf(str2, "%8.5f%8.5f" , stepx       , stepz       );
          if (stepx < 0.0001 || stepz < 0.0001) {
               sprintf(str2, " %6.1e %6.1e" , stepx       , stepz       );
          }

          sprintf(str3, "%6.1f%6.1f" , log10(Pinf) , log10(Dinf));
          sprintf(str4, "%6.1f%15.7e", log10(Cinf0), objtrue     );
          sprintf(str5, "%3d%8.1f"   , nf          , center      );
          if (center > 99999) {
               sprintf(str5, "%3d%8.1e"   , nf          , center      );
          }
          printf("%s%s%s%s%s", str1, str2, str3, str4, str5);
          if (direct) {
               // relax
          } else {
               printf(" %5.1f%7d%7.3f", log10(atolold), itncg, r3ratio);
          }
          //-------------------------------------------------------------------
          // Test for termination.
          //-------------------------------------------------------------------
          if (kminor) {
               printf( "\nStart of next minor itn...\n");
               //      keyboard;
          }

          if (converged) {
               printf("\n   Converged");
               break;
          } else if (PDitns >= maxitn) {
               printf("\n   Too many iterations");
               inform = 1;
               break;
          } else if (nfail  >= maxfail) {
               printf("\n   Too many linesearch failures");
               inform = 2;
               break;
          } else {

               // Reduce mu, and reset certain residuals.

               double stepmu  = CoinMin( stepx , stepz   );
               stepmu  = CoinMin( stepmu, steptol );
               double muold   = mu;
               mu      = mu   -  stepmu * mu;
               if (center >= bigcenter)
                    mu = muold;

               // mutrad = mu0*(sum(Xz)/n); // 24 May 1998: Traditional value, but
               // mu     = CoinMin(mu,mutrad ); // it seemed to decrease mu too much.

               mu      = CoinMax(mu, mulast); // 13 Jun 1998: No need for smaller mu.
               //      [cL,cU,center,Cinf,Cinf0] = ...
               pdxxxresid2( mu, nlow, nupp, low, upp, cL, cU, x1, x2, z1, z2,
                            &center, &Cinf, &Cinf0 );
               fmerit = pdxxxmerit( nlow, nupp, low, upp, r1, r2, rL, rU, cL, cU );

               // Reduce atol for LSQR (and SYMMLQ).
               // NOW DONE AT TOP OF LOOP.

               atolold = atol;
               // if atol > atol2
               //   atolfac = (mu/mufirst)^0.25;
               //   atol    = CoinMax( atol*atolfac, atol2 );
               // end

               // atol = CoinMin( atol, mu );     // 22 Jan 2001: a la Inexact Newton.
               // atol = CoinMin( atol, 0.5*mu ); // 30 Jan 2001: A bit tighter

               // If the linesearch took more than one function (nf > 1),
               // we assume the search direction needed more accuracy
               // (though this may be true only for LPs).
               // 12 Jun 1998: Ask for more accuracy if nf > 2.
               // 24 Nov 2000: Also if the steps are small.
               // 30 Jan 2001: Small steps might be ok with warm start.
               // 06 Feb 2001: Not necessarily.  Reinstated tests in next line.

               if (nf > 2  ||  CoinMin( stepx, stepz ) <= 0.01)
                    atol = atolold * 0.1;
          }
          //---------------------------------------------------------------------
          // End of main loop.
          //---------------------------------------------------------------------
     }


     for (int k = 0; k < nfix; k++)
          x[fix[k]] = bl[fix[k]];
     z      = z1;
     if (nupp > 0)
          z = z - z2;
     printf("\n\nmax |x| =%10.3f", x.infNorm() );
     printf("    max |y| =%10.3f", y.infNorm() );
     printf("    max |z| =%10.3f", z.infNorm() );
     printf("   scaled");

     x.scale(beta);
     y.scale(zeta);
     z.scale(zeta);   // Unscale x, y, z.

     printf(  "\nmax |x| =%10.3f", x.infNorm() );
     printf("    max |y| =%10.3f", y.infNorm() );
     printf("    max |z| =%10.3f", z.infNorm() );
     printf(" unscaled\n");

     time   = CoinCpuTime() - time;
     char str1[100], str2[100];
     sprintf(str1, "\nPDitns  =%10g", PDitns );
     sprintf(str2, "itns =%10d", CGitns );
     //  printf( [str1 " " solver str2] );
     printf("    time    =%10.1f\n", time);
     /*
     pdxxxdistrib( abs(x),abs(z) );   // Private function

     if (wait)
       keyboard;
     */
//-----------------------------------------------------------------------
// End function pdco.m
//-----------------------------------------------------------------------
     /*  printf("Solution x values:\n\n");
       for (int k=0; k<n; k++)
         printf(" %d   %e\n", k, x[k]);
     */
// Print distribution
     double thresh[9] = { 0.00000001, 0.0000001, 0.000001, 0.00001, 0.0001, 0.001, 0.01, 0.1, 1.00001};
     int counts[9] = {0};
     for (int ij = 0; ij < n; ij++) {
          for (int j = 0; j < 9; j++) {
               if(x[ij] < thresh[j]) {
                    counts[j] += 1;
                    break;
               }
          }
     }
     printf ("Distribution of Solution Values\n");
     for (int j = 8; j > 1; j--)
          printf(" %g  to  %g %d\n", thresh[j-1], thresh[j], counts[j]);
     printf("   Less than   %g %d\n", thresh[2], counts[0]);

     return inform;
}
// LSQR
void
ClpPdco::lsqr()
{
}

void ClpPdco::matVecMult( int mode, double* x_elts, double* y_elts)
{
     pdcoStuff_->matVecMult(this, mode, x_elts, y_elts);
}
void ClpPdco::matVecMult( int mode, CoinDenseVector<double> &x, double *y_elts)
{
     double *x_elts = x.getElements();
     matVecMult( mode, x_elts, y_elts);
     return;
}

void ClpPdco::matVecMult( int mode, CoinDenseVector<double> &x, CoinDenseVector<double> &y)
{
     double *x_elts = x.getElements();
     double *y_elts = y.getElements();
     matVecMult( mode, x_elts, y_elts);
     return;
}

void ClpPdco::matVecMult( int mode, CoinDenseVector<double> *x, CoinDenseVector<double> *y)
{
     double *x_elts = x->getElements();
     double *y_elts = y->getElements();
     matVecMult( mode, x_elts, y_elts);
     return;
}
void ClpPdco::matPrecon(double delta, double* x_elts, double* y_elts)
{
     pdcoStuff_->matPrecon(this, delta, x_elts, y_elts);
}
void ClpPdco::matPrecon(double delta, CoinDenseVector<double> &x, double *y_elts)
{
     double *x_elts = x.getElements();
     matPrecon(delta, x_elts, y_elts);
     return;
}

void ClpPdco::matPrecon(double delta, CoinDenseVector<double> &x, CoinDenseVector<double> &y)
{
     double *x_elts = x.getElements();
     double *y_elts = y.getElements();
     matPrecon(delta, x_elts, y_elts);
     return;
}

void ClpPdco::matPrecon(double delta, CoinDenseVector<double> *x, CoinDenseVector<double> *y)
{
     double *x_elts = x->getElements();
     double *y_elts = y->getElements();
     matPrecon(delta, x_elts, y_elts);
     return;
}
void ClpPdco::getBoundTypes(int *nlow, int *nupp, int *nfix, int **bptrs)
{
     *nlow = numberColumns_;
     *nupp = *nfix = 0;
     int *low_ = (int *)malloc(numberColumns_ * sizeof(int)) ;
     for (int k = 0; k < numberColumns_; k++)
          low_[k] = k;
     bptrs[0] = low_;
     return;
}

double ClpPdco::getObj(CoinDenseVector<double> &x)
{
     return pdcoStuff_->getObj(this, x);
}

void ClpPdco::getGrad(CoinDenseVector<double> &x, CoinDenseVector<double> &g)
{
     pdcoStuff_->getGrad(this, x, g);
}

void ClpPdco::getHessian(CoinDenseVector<double> &x, CoinDenseVector<double> &H)
{
     pdcoStuff_->getHessian(this, x, H);
}
