/* $Id$ */
// Copyright (C) 2002, International Business Machines
// Corporation and others, Copyright (C) 2012, FasterCoin.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

/* 
   Authors
   
   John Forrest

 */
#ifndef CoinAbcFactorization_H
#define CoinAbcFactorization_H
#include "CoinAbcCommonFactorization.hpp"
#ifndef ABC_JUST_ONE_FACTORIZATION
#define CoinAbcTypeFactorization CoinAbcFactorization
#define ABC_SMALL -1
#include "CoinAbcBaseFactorization.hpp"
#undef CoinAbcTypeFactorization
#undef ABC_SMALL 
#undef COIN_BIG_DOUBLE
#define COIN_BIG_DOUBLE 1
#define CoinAbcTypeFactorization CoinAbcLongFactorization
#define ABC_SMALL -1
#include "CoinAbcBaseFactorization.hpp"
#undef CoinAbcTypeFactorization
#undef ABC_SMALL 
#undef COIN_BIG_DOUBLE
#define CoinAbcTypeFactorization CoinAbcSmallFactorization
#define ABC_SMALL 4
#include "CoinAbcBaseFactorization.hpp"
#undef CoinAbcTypeFactorization
#undef ABC_SMALL 
#define CoinAbcTypeFactorization CoinAbcOrderedFactorization
#define ABC_SMALL -1
#include "CoinAbcBaseFactorization.hpp"
#undef CoinAbcTypeFactorization
#undef ABC_SMALL
#else
#define CoinAbcTypeFactorization CoinAbcBaseFactorization
#define ABC_SMALL -1
#include "CoinAbcBaseFactorization.hpp"
#endif 
#endif
