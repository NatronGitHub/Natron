/* $Id: CoinWarmStartDual.cpp 1373 2011-01-03 23:57:44Z lou $ */
// Copyright (C) 2003, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#if defined(_MSC_VER)
// Turn off compiler warning about long names
#  pragma warning(disable:4786)
#endif

#include <cassert>

#include "CoinWarmStartDual.hpp"
#include <cmath>

//#############################################################################

/*
  Generate a `diff' that can convert the warm start passed as a parameter to
  the warm start specified by this.

  The capabilities are limited: the basis passed as a parameter can be no
  larger than the basis pointed to by this.
*/

CoinWarmStartDiff*
CoinWarmStartDual::generateDiff (const CoinWarmStart *const oldCWS) const
{ 
/*
  Make sure the parameter is CoinWarmStartDual or derived class.
*/
  const CoinWarmStartDual *oldDual =
      dynamic_cast<const CoinWarmStartDual *>(oldCWS) ;
  if (!oldDual)
  { throw CoinError("Old warm start not derived from CoinWarmStartDual.",
		    "generateDiff","CoinWarmStartDual") ; }

  CoinWarmStartDualDiff* diff = new CoinWarmStartDualDiff;
  CoinWarmStartDiff* vecdiff = dual_.generateDiff(&oldDual->dual_);
  diff->diff_.swap(*dynamic_cast<CoinWarmStartVectorDiff<double>*>(vecdiff));
  delete vecdiff;

  return diff;
}

//=============================================================================
/*
  Apply diff to this warm start.

  Update this warm start by applying diff. It's assumed that the
  allocated capacity of the warm start is sufficiently large.
*/

void CoinWarmStartDual::applyDiff (const CoinWarmStartDiff *const cwsdDiff)
{
/*
  Make sure we have a CoinWarmStartDualDiff
*/
  const CoinWarmStartDualDiff *diff =
    dynamic_cast<const CoinWarmStartDualDiff *>(cwsdDiff) ;
  if (!diff)
  { throw CoinError("Diff not derived from CoinWarmStartDualDiff.",
		    "applyDiff","CoinWarmStartDual") ; }

  dual_.applyDiff(&diff->diff_);
}
