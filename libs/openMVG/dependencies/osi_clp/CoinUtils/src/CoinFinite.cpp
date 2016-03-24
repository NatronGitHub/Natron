/* $Id: CoinFinite.cpp 1420 2011-04-29 18:09:32Z stefan $ */
// Copyright (C) 2011, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include "CoinFinite.hpp"
#include "CoinUtilsConfig.h"
# include <cfloat>
#ifdef HAVE_CFLOAT
# include <cfloat>
#else
# ifdef HAVE_FLOAT_H
#  include <float.h>
# endif
#endif

#ifdef HAVE_CMATH
# include <cmath>
#else
# ifdef HAVE_MATH_H
#  include <math.h>
# endif
#endif

#ifdef HAVE_CIEEEFP
# include <cieeefp>
#else
# ifdef HAVE_IEEEFP_H
#  include <ieeefp.h>
# endif
#endif

bool CoinFinite(double val)
{
#ifdef COIN_C_FINITE
    return COIN_C_FINITE(val)!=0;
#else
    return val != DBL_MAX && val != -DBL_MAX;
#endif
}

bool CoinIsnan(double val)
{
#ifdef COIN_C_ISNAN
    return COIN_C_ISNAN(val)!=0;
#else
    return false;
#endif
}
