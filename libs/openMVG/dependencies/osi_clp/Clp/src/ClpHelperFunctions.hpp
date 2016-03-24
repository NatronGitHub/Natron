/* $Id$ */
// Copyright (C) 2003, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef ClpHelperFunctions_H
#define ClpHelperFunctions_H

#include "ClpConfig.h"
#include <cmath>
// #ifdef HAVE_CMATH
// # include <cmath>
// #else
// # ifdef HAVE_MATH_H
// #  include <math.h>
// # else
// #  error "don't have header file for math"
// # endif
// #endif

/**
    Note (JJF) I have added some operations on arrays even though they may
    duplicate CoinDenseVector.  I think the use of templates was a mistake
    as I don't think inline generic code can take as much advantage of
    parallelism or machine architectures or memory hierarchies.

*/

double maximumAbsElement(const double * region, int size);
void setElements(double * region, int size, double value);
void multiplyAdd(const double * region1, int size, double multiplier1,
                 double * region2, double multiplier2);
double innerProduct(const double * region1, int size, const double * region2);
void getNorms(const double * region, int size, double & norm1, double & norm2);
#if COIN_LONG_WORK
// For long double versions
CoinWorkDouble maximumAbsElement(const CoinWorkDouble * region, int size);
void setElements(CoinWorkDouble * region, int size, CoinWorkDouble value);
void multiplyAdd(const CoinWorkDouble * region1, int size, CoinWorkDouble multiplier1,
                 CoinWorkDouble * region2, CoinWorkDouble multiplier2);
CoinWorkDouble innerProduct(const CoinWorkDouble * region1, int size, const CoinWorkDouble * region2);
void getNorms(const CoinWorkDouble * region, int size, CoinWorkDouble & norm1, CoinWorkDouble & norm2);
inline void
CoinMemcpyN(const double * from, const int size, CoinWorkDouble * to)
{
     for (int i = 0; i < size; i++)
          to[i] = from[i];
}
inline void
CoinMemcpyN(const CoinWorkDouble * from, const int size, double * to)
{
     for (int i = 0; i < size; i++)
          to[i] = static_cast<double>(from[i]);
}
inline CoinWorkDouble
CoinMax(const CoinWorkDouble x1, const double x2)
{
     return (x1 > x2) ? x1 : x2;
}
inline CoinWorkDouble
CoinMax(double x1, const CoinWorkDouble x2)
{
     return (x1 > x2) ? x1 : x2;
}
inline CoinWorkDouble
CoinMin(const CoinWorkDouble x1, const double x2)
{
     return (x1 < x2) ? x1 : x2;
}
inline CoinWorkDouble
CoinMin(double x1, const CoinWorkDouble x2)
{
     return (x1 < x2) ? x1 : x2;
}
inline CoinWorkDouble CoinSqrt(CoinWorkDouble x)
{
     return sqrtl(x);
}
#else
inline double CoinSqrt(double x)
{
     return sqrt(x);
}
#endif
/// Trace
#   ifdef NDEBUG
#      define ClpTraceDebug(expression)		{}
#   else
void ClpTracePrint(std::string fileName, std::string message, int line);
#      define ClpTraceDebug(expression) { \
       if (!(expression)) { ClpTracePrint(__FILE__,__STRING(expression),__LINE__); } \
  }
#   endif
/// Following only included if ClpPdco defined
#ifdef ClpPdco_H


inline double pdxxxmerit(int nlow, int nupp, int *low, int *upp, CoinDenseVector <double> &r1,
                         CoinDenseVector <double> &r2, CoinDenseVector <double> &rL,
                         CoinDenseVector <double> &rU, CoinDenseVector <double> &cL,
                         CoinDenseVector <double> &cU )
{

// Evaluate the merit function for Newton's method.
// It is the 2-norm of the three sets of residuals.
     double sum1, sum2;
     CoinDenseVector <double> f(6);
     f[0] = r1.twoNorm();
     f[1] = r2.twoNorm();
     sum1 = sum2 = 0.0;
     for (int k = 0; k < nlow; k++) {
          sum1 += rL[low[k]] * rL[low[k]];
          sum2 += cL[low[k]] * cL[low[k]];
     }
     f[2] = sqrt(sum1);
     f[4] = sqrt(sum2);
     sum1 = sum2 = 0.0;
     for (int k = 0; k < nupp; k++) {
          sum1 += rL[upp[k]] * rL[upp[k]];
          sum2 += cL[upp[k]] * cL[upp[k]];
     }
     f[3] = sqrt(sum1);
     f[5] = sqrt(sum2);

     return f.twoNorm();
}

//-----------------------------------------------------------------------
// End private function pdxxxmerit
//-----------------------------------------------------------------------


//function [r1,r2,rL,rU,Pinf,Dinf] =    ...
//      pdxxxresid1( Aname,fix,low,upp, ...
//                   b,bl,bu,d1,d2,grad,rL,rU,x,x1,x2,y,z1,z2 )

inline void pdxxxresid1(ClpPdco *model, const int nlow, const int nupp, const int nfix,
                        int *low, int *upp, int *fix,
                        CoinDenseVector <double> &b, double *bl, double *bu, double d1, double d2,
                        CoinDenseVector <double> &grad, CoinDenseVector <double> &rL,
                        CoinDenseVector <double> &rU, CoinDenseVector <double> &x,
                        CoinDenseVector <double> &x1, CoinDenseVector <double> &x2,
                        CoinDenseVector <double> &y,  CoinDenseVector <double> &z1,
                        CoinDenseVector <double> &z2, CoinDenseVector <double> &r1,
                        CoinDenseVector <double> &r2, double *Pinf, double *Dinf)
{

// Form residuals for the primal and dual equations.
// rL, rU are output, but we input them as full vectors
// initialized (permanently) with any relevant zeros.

// Get some element pointers for efficiency
     double *x_elts  = x.getElements();
     double *r2_elts = r2.getElements();

     for (int k = 0; k < nfix; k++)
          x_elts[fix[k]]  = 0;

     r1.clear();
     r2.clear();
     model->matVecMult( 1, r1, x );
     model->matVecMult( 2, r2, y );
     for (int k = 0; k < nfix; k++)
          r2_elts[fix[k]]  = 0;


     r1      = b    - r1 - d2 * d2 * y;
     r2      = grad - r2 - z1;              // grad includes d1*d1*x
     if (nupp > 0)
          r2    = r2 + z2;

     for (int k = 0; k < nlow; k++)
          rL[low[k]] = bl[low[k]] - x[low[k]] + x1[low[k]];
     for (int k = 0; k < nupp; k++)
          rU[upp[k]] = - bu[upp[k]] + x[upp[k]] + x2[upp[k]];

     double normL = 0.0;
     double normU = 0.0;
     for (int k = 0; k < nlow; k++)
          if (rL[low[k]] > normL) normL = rL[low[k]];
     for (int k = 0; k < nupp; k++)
          if (rU[upp[k]] > normU) normU = rU[upp[k]];

     *Pinf    = CoinMax(normL, normU);
     *Pinf    = CoinMax( r1.infNorm() , *Pinf );
     *Dinf    = r2.infNorm();
     *Pinf    = CoinMax( *Pinf, 1e-99 );
     *Dinf    = CoinMax( *Dinf, 1e-99 );
}

//-----------------------------------------------------------------------
// End private function pdxxxresid1
//-----------------------------------------------------------------------


//function [cL,cU,center,Cinf,Cinf0] = ...
//      pdxxxresid2( mu,low,upp,cL,cU,x1,x2,z1,z2 )

inline void pdxxxresid2(double mu, int nlow, int nupp, int *low, int *upp,
                        CoinDenseVector <double> &cL, CoinDenseVector <double> &cU,
                        CoinDenseVector <double> &x1, CoinDenseVector <double> &x2,
                        CoinDenseVector <double> &z1, CoinDenseVector <double> &z2,
                        double *center, double *Cinf, double *Cinf0)
{

// Form residuals for the complementarity equations.
// cL, cU are output, but we input them as full vectors
// initialized (permanently) with any relevant zeros.
// Cinf  is the complementarity residual for X1 z1 = mu e, etc.
// Cinf0 is the same for mu=0 (i.e., for the original problem).

     double maxXz = -1e20;
     double minXz = 1e20;

     double *x1_elts = x1.getElements();
     double *z1_elts = z1.getElements();
     double *cL_elts = cL.getElements();
     for (int k = 0; k < nlow; k++) {
          double x1z1    = x1_elts[low[k]] * z1_elts[low[k]];
          cL_elts[low[k]] = mu - x1z1;
          if (x1z1 > maxXz) maxXz = x1z1;
          if (x1z1 < minXz) minXz = x1z1;
     }

     double *x2_elts = x2.getElements();
     double *z2_elts = z2.getElements();
     double *cU_elts = cU.getElements();
     for (int k = 0; k < nupp; k++) {
          double x2z2    = x2_elts[upp[k]] * z2_elts[upp[k]];
          cU_elts[upp[k]] = mu - x2z2;
          if (x2z2 > maxXz) maxXz = x2z2;
          if (x2z2 < minXz) minXz = x2z2;
     }

     maxXz   = CoinMax( maxXz, 1e-99 );
     minXz   = CoinMax( minXz, 1e-99 );
     *center  = maxXz / minXz;

     double normL = 0.0;
     double normU = 0.0;
     for (int k = 0; k < nlow; k++)
          if (cL_elts[low[k]] > normL) normL = cL_elts[low[k]];
     for (int k = 0; k < nupp; k++)
          if (cU_elts[upp[k]] > normU) normU = cU_elts[upp[k]];
     *Cinf    = CoinMax( normL, normU);
     *Cinf0   = maxXz;
}
//-----------------------------------------------------------------------
// End private function pdxxxresid2
//-----------------------------------------------------------------------

inline double  pdxxxstep( CoinDenseVector <double> &x, CoinDenseVector <double> &dx )
{

// Assumes x > 0.
// Finds the maximum step such that x + step*dx >= 0.

     double step     = 1e+20;

     int n = x.size();
     double *x_elts = x.getElements();
     double *dx_elts = dx.getElements();
     for (int k = 0; k < n; k++)
          if (dx_elts[k] < 0)
               if ((x_elts[k] / (-dx_elts[k])) < step)
                    step = x_elts[k] / (-dx_elts[k]);
     return step;
}
//-----------------------------------------------------------------------
// End private function pdxxxstep
//-----------------------------------------------------------------------

inline double  pdxxxstep(int nset, int *set, CoinDenseVector <double> &x, CoinDenseVector <double> &dx )
{

// Assumes x > 0.
// Finds the maximum step such that x + step*dx >= 0.

     double step     = 1e+20;

     int n = x.size();
     double *x_elts = x.getElements();
     double *dx_elts = dx.getElements();
     for (int k = 0; k < n; k++)
          if (dx_elts[k] < 0)
               if ((x_elts[k] / (-dx_elts[k])) < step)
                    step = x_elts[k] / (-dx_elts[k]);
     return step;
}
//-----------------------------------------------------------------------
// End private function pdxxxstep
//-----------------------------------------------------------------------
#endif
#endif
