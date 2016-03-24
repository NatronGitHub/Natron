/* $Id$ */
// Copyright (C) 2003, International Business Machines
// Corporation and others, Copyright (C) 2012, FasterCoin.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).
/*
   Authors

   John Forrest

 */
#ifndef AbcCommon_H
#define AbcCommon_H

#include "ClpConfig.h"

/*
  0 - off
  1 - build Abc serial but no inherit code
  2 - build Abc serial and inherit code
  3 - build Abc cilk parallel but no inherit code
  4 - build Abc cilk parallel and inherit code
 */
#ifdef CLP_HAS_ABC
#if CLP_HAS_ABC==1
#ifndef ABC_PARALLEL
#define ABC_PARALLEL 0
#endif
#ifndef ABC_USE_HOMEGROWN_LAPACK
#define ABC_USE_HOMEGROWN_LAPACK 2
#endif
#elif CLP_HAS_ABC==2
#ifndef ABC_PARALLEL
#define ABC_PARALLEL 0
#endif
#ifndef ABC_USE_HOMEGROWN_LAPACK
#define ABC_USE_HOMEGROWN_LAPACK 2
#endif
#ifndef ABC_INHERIT
#define ABC_INHERIT
#endif
#elif CLP_HAS_ABC==3
#ifndef ABC_PARALLEL
#define ABC_PARALLEL 2
#endif
#ifndef ABC_USE_HOMEGROWN_LAPACK
#define ABC_USE_HOMEGROWN_LAPACK 2
#endif
#elif CLP_HAS_ABC==4
#ifndef ABC_PARALLEL
#define ABC_PARALLEL 2
#endif
#ifndef ABC_USE_HOMEGROWN_LAPACK
#define ABC_USE_HOMEGROWN_LAPACK 2
#endif
#ifndef ABC_INHERIT
#define ABC_INHERIT
#endif
#else
#error "Valid values for CLP_HAS_ABC are 0-4"
#endif
#endif
#endif
