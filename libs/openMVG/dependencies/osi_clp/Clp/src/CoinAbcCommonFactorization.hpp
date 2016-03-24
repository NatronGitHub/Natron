/* $Id$ */
// Copyright (C) 2000, International Business Machines
// Corporation and others, Copyright (C) 2012, FasterCoin.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).
#ifndef CoinAbcCommonFactorization_H
#define CoinAbcCommonFactorization_H
/* meaning of ABC_SMALL 
   -1 force copies (and no tests)
   0 force copy of U
   2 force no copies (and no tests)
*/

#include "CoinAbcCommon.hpp"
//#define DONT_USE_SLACKS
//#define COIN_ONE_ETA_COPY 100
//#define COIN_FAC_NEW 1
#define INITIAL_AVERAGE 1.0
#define INITIAL_AVERAGE2 1.0
#define AVERAGE_SCALE_BACK 0.8
//#define SWITCHABLE_STATISTICS
#ifndef SWITCHABLE_STATISTICS
#define setStatistics(x)
#define factorizationStatistics() (true)
#else
#define setStatistics(x) collectStatistics_=x
#define factorizationStatistics() (collectStatistics_)
#endif
#include "CoinAbcDenseFactorization.hpp"
class CoinPackedMatrix;
class CoinFactorization;
#define FACTORIZATION_STATISTICS 0 //1
typedef struct {
  double countInput_;
  double countAfterL_;
  double countAfterR_;
  double countAfterU_;
  double averageAfterL_;
  double averageAfterR_;
  double averageAfterU_;
#if FACTORIZATION_STATISTICS
  double twiddleFactor1_;
  double twiddleFactor2_;
#endif  
  CoinSimplexInt numberCounts_;
} CoinAbcStatistics;
#if FACTORIZATION_STATISTICS
#define twiddleFactor1S()  (statistics.twiddleFactor1_)
#define twiddleFactor2S()  (statistics.twiddleFactor2_)
#define twiddleFtranFactor1()  (ftranTwiddleFactor1_)
#define twiddleFtranFTFactor1()  (ftranFTTwiddleFactor1_)
#define twiddleBtranFactor1()  (btranTwiddleFactor1_)
#define twiddleFtranFactor2()  (ftranTwiddleFactor2_)
#define twiddleFtranFTFactor2()  (ftranFTTwiddleFactor2_)
#define twiddleBtranFactor2()  (btranTwiddleFactor2_)
#define twiddleBtranFullFactor1()  (btranFullTwiddleFactor1_)
#else
#define twiddleFactor1S()  (1.0)
#define twiddleFactor2S()  (1.0)
#define twiddleFtranFactor1()  (1.0)
#define twiddleFtranFTFactor1()  (1.0)
#define twiddleBtranFactor1()  (1.0)
#define twiddleFtranFactor2()  (1.0)
#define twiddleFtranFTFactor2()  (1.0)
#define twiddleBtranFactor2()  (1.0)
#define twiddleBtranFullFactor1()  (1.0)
#endif  
#define ABC_FAC_GOT_LCOPY 4
#define ABC_FAC_GOT_RCOPY 8
#define ABC_FAC_GOT_UCOPY 16
#define ABC_FAC_GOT_SPARSE 32
typedef struct {
  CoinBigIndex next;
  CoinBigIndex start;
  CoinSimplexUnsignedInt stack;
} CoinAbcStack;
void CoinAbcDgetrs(char trans,int m, double * a, double * work);
int  CoinAbcDgetrf(int m, int n, double * a, int lda, int * ipiv
#if ABC_PARALLEL==2
			  ,int parallelMode
#endif
);
void CoinAbcDgetrs(char trans,int m, long double * a, long double * work);
int  CoinAbcDgetrf(int m, int n, long double * a, int lda, int * ipiv
#if ABC_PARALLEL==2
			  ,int parallelMode
#endif
);
#define SWAP_FACTOR 2
#define BLOCKING8 8
#define BLOCKING8X8 BLOCKING8*BLOCKING8
#endif
