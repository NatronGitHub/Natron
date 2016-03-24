/* $Id$ */
// Copyright (C) 2000, International Business Machines
// Corporation and others, Copyright (C) 2012, FasterCoin.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).
#ifndef CoinAbcCommon_H
#define CoinAbcCommon_H
#ifndef COIN_FAC_NEW
#define COIN_FAC_NEW
#endif

#include "CoinPragma.hpp"
#include "CoinUtilsConfig.h"
#include <iostream>
#include <string>
#include <cassert>
#include <cstdio>
#include <cmath>
#include "AbcCommon.hpp"
#include "CoinHelperFunctions.hpp"
//#include "config.h"
typedef double CoinSimplexDouble;
typedef int CoinSimplexInt;
typedef unsigned int CoinSimplexUnsignedInt;
//#define MOVE_REPLACE_PART1A
#if defined(_MSC_VER)
#define ABC_INLINE __forceinline
#elif defined(__GNUC__)
#define ABC_INLINE __attribute__((always_inline))
#else
#define ABC_INLINE
#endif
#ifndef ABC_PARALLEL
#ifdef HAS_CILK 
#define ABC_PARALLEL 2
#else
#define ABC_PARALLEL 0
#endif
#endif
#if ABC_PARALLEL==2
//#define EARLY_FACTORIZE
#ifndef FAKE_CILK
#include <cilk/cilk.h>
#else
#define cilk_for for
#define cilk_spawn
#define cilk_sync
#endif
#else
#define cilk_for for
#define cilk_spawn
#define cilk_sync
//#define ABC_PARALLEL 1
#endif
#define SLACK_VALUE 1
#define ABC_INSTRUMENT 1 //2
#if ABC_INSTRUMENT!=2
// Below so can do deterministic B&B
#define instrument_start(name,x)
#define instrument_add(x)
#define instrument_end()
// one off
#define instrument_do(name,x)
// as end but multiply by factor
#define instrument_end_and_adjust(x)
#else
void instrument_start(const char * type,int numberRowsEtc);
void instrument_add(int count);
void instrument_do(const char * type,double count);
void instrument_end();
void instrument_end_and_adjust(double factor);
#endif
#ifndef __BYTE_ORDER
#include <endian.h>
#endif
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define ABC_INTEL
#endif
#if COIN_BIG_DOUBLE==1
#undef USE_TEST_ZERO
#undef USE_TEST_REALLY_ZERO
#undef USE_TEST_ZERO_REGISTER
#undef USE_TEST_LESS_TOLERANCE
#undef USE_TEST_LESS_TOLERANCE_REGISTER
#define CoinFabs(x) fabsl(x)
#else
#define CoinFabs(x) fabs(x)
#endif
#ifdef USE_TEST_ZERO
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define TEST_DOUBLE_NONZERO(x) ((reinterpret_cast<int *>(&x))[1]!=0)
#else
#define TEST_DOUBLE_NONZERO(x) ((reinterpret_cast<int *>(&x))[0]!=0)
#endif
#else
//always drop through
#define TEST_DOUBLE_NONZERO(x) (true)
#endif
#define USE_TEST_INT_ZERO
#ifdef USE_TEST_INT_ZERO
#define TEST_INT_NONZERO(x) (x)
#else
//always drop through
#define TEST_INT_NONZERO(x) (true)
#endif
#ifdef USE_TEST_REALLY_ZERO
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define TEST_DOUBLE_REALLY_NONZERO(x) ((reinterpret_cast<int *>(&x))[1]!=0)
#else
#define TEST_DOUBLE_REALLY_NONZERO(x) ((reinterpret_cast<int *>(&x))[0]!=0)
#endif
#else
#define TEST_DOUBLE_REALLY_NONZERO(x) (x)
#endif
#ifdef USE_TEST_ZERO_REGISTER
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define TEST_DOUBLE_NONZERO_REGISTER(x) ((reinterpret_cast<int *>(&x))[1]!=0)
#else
#define TEST_DOUBLE_NONZERO_REGISTER(x) ((reinterpret_cast<int *>(&x))[0]!=0)
#endif
#else
//always drop through
#define TEST_DOUBLE_NONZERO_REGISTER(x) (true)
#endif
#define USE_FIXED_ZERO_TOLERANCE
#ifdef USE_FIXED_ZERO_TOLERANCE
// 3d400000... 0.5**43 approx 1.13687e-13
#ifdef USE_TEST_LESS_TOLERANCE
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define TEST_LESS_THAN_TOLERANCE(x) ((reinterpret_cast<int *>(&x))[1]&0x7ff00000<0x3d400000)
#define TEST_LESS_THAN_UPDATE_TOLERANCE(x) ((reinterpret_cast<int *>(&x))[1]&0x7ff00000<0x3d400000)
#else
#define TEST_LESS_THAN_TOLERANCE(x) ((reinterpret_cast<int *>(&x))[0]&0x7ff00000<0x3d400000)
#define TEST_LESS_THAN_UPDATE_TOLERANCE(x) ((reinterpret_cast<int *>(&x))[0]&0x7ff00000<0x3d400000)
#endif
#else
#define TEST_LESS_THAN_TOLERANCE(x) (fabs(x)<pow(0.5,43))
#define TEST_LESS_THAN_UPDATE_TOLERANCE(x) (fabs(x)<pow(0.5,43))
#endif
#ifdef USE_TEST_LESS_TOLERANCE_REGISTER
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define TEST_LESS_THAN_TOLERANCE_REGISTER(x) ((reinterpret_cast<int *>(&x))[1]&0x7ff00000<0x3d400000)
#else
#define TEST_LESS_THAN_TOLERANCE_REGISTER(x) ((reinterpret_cast<int *>(&x))[0]&0x7ff00000<0x3d400000)
#endif
#else
#define TEST_LESS_THAN_TOLERANCE_REGISTER(x) (fabs(x)<pow(0.5,43))
#endif
#else
#define TEST_LESS_THAN_TOLERANCE(x) (fabs(x)<zeroTolerance_)
#define TEST_LESS_THAN_TOLERANCE_REGISTER(x) (fabs(x)<zeroTolerance_)
#endif
#if COIN_BIG_DOUBLE!=1
typedef unsigned int CoinExponent;
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define ABC_EXPONENT(x) ((reinterpret_cast<int *>(&x))[1]&0x7ff00000)
#else
#define ABC_EXPONENT(x) ((reinterpret_cast<int *>(&x))[0]&0x7ff00000)
#endif
#define TEST_EXPONENT_LESS_THAN_TOLERANCE(x) (x<0x3d400000)
#define TEST_EXPONENT_LESS_THAN_UPDATE_TOLERANCE(x) (x<0x3d400000)
#define TEST_EXPONENT_NON_ZERO(x) (x)
#else
typedef long double  CoinExponent;
#define ABC_EXPONENT(x) (x)
#define TEST_EXPONENT_LESS_THAN_TOLERANCE(x) (fabs(x)<pow(0.5,43))
#define TEST_EXPONENT_LESS_THAN_UPDATE_TOLERANCE(x) (fabs(x)<pow(0.5,43))
#define TEST_EXPONENT_NON_ZERO(x) (x)
#endif
#ifdef INT_IS_8
#define COINFACTORIZATION_BITS_PER_INT 64
#define COINFACTORIZATION_SHIFT_PER_INT 6
#define COINFACTORIZATION_MASK_PER_INT 0x3f
#else
#define COINFACTORIZATION_BITS_PER_INT 32
#define COINFACTORIZATION_SHIFT_PER_INT 5
#define COINFACTORIZATION_MASK_PER_INT 0x1f
#endif
#if ABC_USE_HOMEGROWN_LAPACK==1
#define ABC_USE_LAPACK
#endif
#ifdef ABC_USE_LAPACK
#define F77_FUNC(x,y) x##_
#define ABC_DENSE_CODE 1
/* Type of Fortran integer translated into C */
#ifndef ipfint
//typedef ipfint FORTRAN_INTEGER_TYPE ;
typedef int ipfint;
typedef const int cipfint;
#endif
enum CBLAS_ORDER {CblasRowMajor=101, CblasColMajor=102 };
enum CBLAS_TRANSPOSE {CblasNoTrans=111, CblasTrans=112, CblasConjTrans=113,
		      AtlasConj=114};
#define CLAPACK
// using simple lapack interface
extern "C" 
{
  /** LAPACK Fortran subroutine DGETRS. */
  void F77_FUNC(dgetrs,DGETRS)(char *trans, cipfint *n,
                               cipfint *nrhs, const CoinSimplexDouble *A, cipfint *ldA,
                               cipfint * ipiv, CoinSimplexDouble *B, cipfint *ldB, ipfint *info,
			       int trans_len);
  /** LAPACK Fortran subroutine DGETRF. */
  void F77_FUNC(dgetrf,DGETRF)(ipfint * m, ipfint *n,
                               CoinSimplexDouble *A, ipfint *ldA,
                               ipfint * ipiv, ipfint *info);
  int clapack_dgetrf(const enum CBLAS_ORDER Order, const int M, const int N,
		     double *A, const int lda, int *ipiv);
  int clapack_dgetrs
  (const enum CBLAS_ORDER Order, const enum CBLAS_TRANSPOSE Trans,
   const int N, const int NRHS, const double *A, const int lda,
   const int *ipiv, double *B, const int ldb);
}
#else // use home grown
/* Dense coding
   -1 use homegrown but just for factorization
   0 off all dense
   2 use homegrown for factorization and solves
*/
#ifndef ABC_USE_HOMEGROWN_LAPACK
#define ABC_DENSE_CODE 2
#else
#define ABC_DENSE_CODE ABC_USE_HOMEGROWN_LAPACK
#endif
#endif
typedef unsigned char CoinCheckZero;
template <class T> inline void
CoinAbcMemset0(register T* to, const int size)
{
#ifndef NDEBUG
  // Some debug so check
  if (size < 0)
    throw CoinError("trying to fill negative number of entries",
		    "CoinAbcMemset0", "");
#endif
  std::memset(to,0,size*sizeof(T));
}
template <class T> inline void
CoinAbcMemcpy(register T* to, register const T* from, const int size )
{
#ifndef NDEBUG
  // Some debug so check
  if (size < 0)
    throw CoinError("trying to copy negative number of entries",
		    "CoinAbcMemcpy", "");
  
#endif
  std::memcpy(to,from,size*sizeof(T));
}
class ClpSimplex;
class AbcSimplex;
class AbcTolerancesEtc  {
  
public:
  
  
  
  ///@name Constructors and destructors
  //@{
  /// Default Constructor
  AbcTolerancesEtc();
  
  /// Useful Constructors
  AbcTolerancesEtc(const ClpSimplex * model);
  AbcTolerancesEtc(const AbcSimplex * model);
  
  /// Copy constructor
  AbcTolerancesEtc(const AbcTolerancesEtc &);
  
  /// Assignment operator
  AbcTolerancesEtc & operator=(const AbcTolerancesEtc& rhs);
  
  /// Destructor
  ~AbcTolerancesEtc ();
  //@}
  
  
  //---------------------------------------------------------------------------
  
public:
  ///@name Public member data
  //@{
  /// Zero tolerance
  double zeroTolerance_;
  /// Primal tolerance needed to make dual feasible (<largeTolerance)
  double primalToleranceToGetOptimal_;
  /// Large bound value (for complementarity etc)
  double largeValue_;
  /// For computing whether to re-factorize
  double alphaAccuracy_;
  /// Dual bound
  double dualBound_;
  /// Current dual tolerance for algorithm
  double dualTolerance_;
  /// Current primal tolerance for algorithm
  double primalTolerance_;
  /// Weight assigned to being infeasible in primal
  double infeasibilityCost_;
  /** For advanced use.  When doing iterative solves things can get
      nasty so on values pass if incoming solution has largest
      infeasibility < incomingInfeasibility throw out variables
      from basis until largest infeasibility < allowedInfeasibility.
      if allowedInfeasibility>= incomingInfeasibility this is
      always possible altough you may end up with an all slack basis.
      
      Defaults are 1.0,10.0
  */
  double incomingInfeasibility_;
  double allowedInfeasibility_;
  /// Iteration when we entered dual or primal
  int baseIteration_;
  /// How many iterative refinements to do
  int numberRefinements_;
  /** Now for some reliability aids
      This forces re-factorization early */
  int forceFactorization_;
  /** Perturbation:
      -50 to +50 - perturb by this power of ten (-6 sounds good)
      100 - auto perturb if takes too long (1.0e-6 largest nonzero)
      101 - we are perturbed
      102 - don't try perturbing again
      default is 100
  */
  int perturbation_;
  /// If may skip final factorize then allow up to this pivots (default 20)
  int dontFactorizePivots_;
  /// For factorization
  /// Maximum number of pivots before factorization
  int maximumPivots_;
  //@}
};
#endif
