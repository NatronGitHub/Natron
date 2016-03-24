/* $Id: CoinWarmStartPrimalDual.cpp 1373 2011-01-03 23:57:44Z lou $ */
// Copyright (C) 2003, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#if defined(_MSC_VER)
// Turn off compiler warning about long names
#  pragma warning(disable:4786)
#endif

#include <cassert>

#include "CoinWarmStartPrimalDual.hpp"
#include <cmath>

//#############################################################################

/*
  Generate a `diff' that can convert the warm start passed as a parameter to
  the warm start specified by this.

  The capabilities are limited: the basis passed as a parameter can be no
  larger than the basis pointed to by this.
*/

CoinWarmStartDiff*
CoinWarmStartPrimalDual::generateDiff (const CoinWarmStart *const oldCWS) const
{ 
/*
  Make sure the parameter is CoinWarmStartPrimalDual or derived class.
*/
  const CoinWarmStartPrimalDual *old =
      dynamic_cast<const CoinWarmStartPrimalDual *>(oldCWS) ;
  if (!old)
  { throw CoinError("Old warm start not derived from CoinWarmStartPrimalDual.",
		    "generateDiff","CoinWarmStartPrimalDual") ; }

  CoinWarmStartPrimalDualDiff* diff = new CoinWarmStartPrimalDualDiff;
  CoinWarmStartDiff* vecdiff;
  vecdiff = primal_.generateDiff(&old->primal_);
  diff->primalDiff_.swap(*dynamic_cast<CoinWarmStartVectorDiff<double>*>(vecdiff));
  delete vecdiff;
  vecdiff = dual_.generateDiff(&old->dual_);
  diff->dualDiff_.swap(*dynamic_cast<CoinWarmStartVectorDiff<double>*>(vecdiff));
  delete vecdiff;

  return diff;
}

//#############################################################################

/*
  Apply diff to this warm start.

  Update this warm start by applying diff. It's assumed that the
  allocated capacity of the warm start is sufficiently large.
*/

void
CoinWarmStartPrimalDual::applyDiff (const CoinWarmStartDiff *const cwsdDiff)
{
/*
  Make sure we have a CoinWarmStartPrimalDualDiff
*/
  const CoinWarmStartPrimalDualDiff *diff =
    dynamic_cast<const CoinWarmStartPrimalDualDiff *>(cwsdDiff) ;
  if (!diff)
  { throw CoinError("Diff not derived from CoinWarmStartPrimalDualDiff.",
		    "applyDiff","CoinWarmStartPrimalDual") ; }

  primal_.applyDiff(&diff->primalDiff_);
  dual_.applyDiff(&diff->dualDiff_);
}
