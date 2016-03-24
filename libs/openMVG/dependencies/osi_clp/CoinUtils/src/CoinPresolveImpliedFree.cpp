/* $Id: CoinPresolveImpliedFree.cpp 1595 2013-04-19 14:39:06Z forrest $ */
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include <stdio.h>
#include <math.h>

#include "CoinPresolveMatrix.hpp"
#include "CoinPresolveSubst.hpp"
#include "CoinPresolveIsolated.hpp"
#include "CoinPresolveFixed.hpp"
#include "CoinPresolveImpliedFree.hpp"
#include "CoinPresolveUseless.hpp"
#include "CoinPresolveForcing.hpp"
#include "CoinMessage.hpp"
#include "CoinHelperFunctions.hpp"
#include "CoinSort.hpp"
#include "CoinFinite.hpp"

#if PRESOLVE_DEBUG > 0 || PRESOLVE_CONSISTENCY > 0
#include "CoinPresolvePsdebug.hpp"
#endif

/*
  Implied Free and Subst Transforms

  If there is a constraint i entangled with a singleton column x(t) such
  that for any feasible (with respect to column bounds) values of the other
  variables in the constraint we can calculate a feasible value for x(t),
  then we can drop x(t) and constraint i, since we can compute the value
  of x(t) from the values of the other variables during postsolve.

  To put this into practice, calculate bounds L(i\t) and U(i\t) on row
  activity and use those bounds to calculate implied lower and upper
  bounds l'(t) and u'(t) on x(t). If the implied bounds are tighter than
  the original column bounds l(t) and u(t) (the implied free condition),
  we can drop constraint i and x(t).

  If x(t) is not a natural singleton, we can still do something similar. Let
  constraint i be an equality constraint that satisfies the implied free
  condition.  In that case, we use constraint i to generate a substitution
  formula and substitute away x(t) in any other constraints where it appears.
  This introduces new coefficients, but the total number of coefficients
  never increases if x(t) is entangled with only two constraints
  (coefficients added during substitution are cancelled by dropping
  constraint i). The total may not increase much even if there are more.

  Both situations are detected in implied_free_action::presolve. Natural
  singletons are processed by implied_free_action::presolve. The rest are
  passed to subst_constraint_action (which see).

  It is possible for two singleton columns to be in the same row.  In that
  case, the other one will become empty.  If its bounds and costs aren't
  just right, this signals an unbounded problem.  We don't need to check
  that specially here.

  There is a superficial (and misleading) similarity to a useless constraint.
  A useless constraint cannot be made tight for *any* feasible values of
  its variables.  Here, it's possible we could pick some feasible value
  of x(t) that *would* violate the constraint.
*/


/*
  Scan for candidates for the implied free and subst transforms (see
  comments at head of file and in CoinPresolveSubst.cpp). Process natural
  singletons. Pass more complicated cases to subst_constraint_action.

  fill_level limits the allowable number of coefficients in a column under
  consideration as the implied free variable. There's a feedback loop between
  implied_free_action and the presolve driver.  The feedback loop operates
  as follows: If we're not finding enough transforms and fill_level <
  maxSubstLevel_, increase it by one and pass it back out negated. The
  presolve driver can act on this, if it chooses, or simply pass it back
  in on the next call to implied_free_action.  If implied_free_action sees
  a negative value, it will look at all columns, instead of just those
  in colsToDo_.
*/

const CoinPresolveAction *implied_free_action::presolve (
    CoinPresolveMatrix *prob, const CoinPresolveAction *next, int &fill_level)
{
# if PRESOLVE_DEBUG > 0 || PRESOLVE_CONSISTENCY > 0
# if PRESOLVE_DEBUG > 0
  std::cout
    << "Entering implied_free_action::presolve, fill level " << fill_level
    << "." << std::endl ;
# endif
  presolve_consistent(prob) ;
  presolve_links_ok(prob) ;
  presolve_check_sol(prob) ;
  presolve_check_nbasic(prob) ;
# endif

# if PRESOLVE_DEBUG > 0 || COIN_PRESOLVE_TUNING > 0
  const int startEmptyRows = prob->countEmptyRows() ;
  const int startEmptyColumns = prob->countEmptyCols() ;
# if COIN_PRESOLVE_TUNING > 0
  double startTime = 0.0 ;
  if (prob->tuning_) startTime = CoinCpuTime() ;
# endif
# endif

/*
  Unpack the row- and column-major representations.
*/
  const int m = prob->nrows_ ;
  const int n = prob->ncols_ ;

  const CoinBigIndex *rowStarts = prob->mrstrt_ ;
  int *rowLengths = prob->hinrow_ ;
  const int *colIndices = prob->hcol_ ;
  const double *rowCoeffs = prob->rowels_ ;
  presolvehlink *rlink = prob->rlink_ ;

  CoinBigIndex *colStarts = prob->mcstrt_ ;
  int *colLengths = prob->hincol_ ;
  int *rowIndices = prob->hrow_ ;
  double *colCoeffs = prob->colels_ ;
  presolvehlink *clink = prob->clink_ ;

/*
  Column bounds, row bounds, cost, integrality.
*/
  double *clo = prob->clo_ ;
  double *cup = prob->cup_ ;
  double *rlo = prob->rlo_ ;
  double *rup = prob->rup_ ;
  double *cost = prob->cost_ ;
  const unsigned char *integerType = prob->integerType_ ;

/*
  Documented as `inhibit x+y+z = 1 mods'.  From the code below, it's clear
  that this is intended to avoid removing SOS equalities with length >= 5
  (hardcoded). 
*/
  const bool stopSomeStuff = ((prob->presolveOptions()&0x04) != 0) ;
/*
  Ignore infeasibility. `Fix' is overly optimistic.
*/
  const bool fixInfeasibility = ((prob->presolveOptions_&0x4000) != 0) ;
/*
  Defaults to 0.0.
*/
  const double feasTol = prob->feasibilityTolerance_ ;

# if 0  
/*
  Tentatively moved to be a front-end function for useless_constraint_action,
  much as make_fixed is a front-end for make_fixed_action. This bit of code
  left for possible tuning.
  -- lh, 121127 --

  Original comment: This needs to be made faster.
*/
# ifdef COIN_LIGHTWEIGHT_PRESOLVE
  if (prob->pass_ == 1) {
# endif
    next = testRedundant(prob,next) ;
    if (prob->status_&0x01 != 0) {
      if ((prob->presolveOptions_&0x4000) != 0)
        prob->status &= !0x01 ;
      else
	return (next) ;
    }
# ifdef COIN_LIGHTWEIGHT_PRESOLVE
  }
# endif
# endif

/*
  implied_free and subst take a fair bit of effort to scan for candidates.
  This is a hook to allow a presolve driver to avoid that work.
*/
  if (prob->pass_ > 15 && (prob->presolveOptions_&0x10000) != 0) { 
    fill_level = 2 ;
    return (next) ;
  }

/*
  Set up to collect implied_free actions.
*/
  action *actions = new action [n] ;
# ifdef ZEROFAULT
  CoinZeroN(reinterpret_cast<char *>(actions),n*sizeof(action)) ;
# endif
  int nactions = 0 ;

  int *implied_free = prob->usefulColumnInt_ ;
  int *whichFree = implied_free+n ;
  int numberFree = 0 ;
/*
  Arrays to hold row activity (row lhs) min and max values. Each row lhs bound
  is held as two components: a sum of finite column bounds and a count of
  infinite column bounds.
*/
  int *infiniteDown = new int[m] ;
  int *infiniteUp = new int[m] ;
  double *maxDown = new double[m] ;
  double *maxUp = new double[m] ;
/*
  Overload infiniteUp with row status codes:
  -1: L(i)/U(i) not yet computed,
  -2: do not use (empty or useless row),
  -3: give up (infeasible)
  -4: chosen as implied free row
*/
  for (int i = 0 ; i < m ; i++) {
    if (rowLengths[i] > 1)
      infiniteUp[i] = -1 ;
    else
      infiniteUp[i] = -2 ;
  }
  // Get rid of rows with prohibited columns
  if (prob->anyProhibited_) {
    for (int i = 0 ; i < m ; i++) {
      CoinBigIndex rStart = rowStarts[i];
      CoinBigIndex rEnd = rStart+rowLengths[i];
      bool badRow=false;
      for (CoinBigIndex j = rStart; j < rEnd; ++j) {
	if (prob->colProhibited(colIndices[j])) {
	  badRow=true;
	  break;
	}
      }
      if (badRow)
	infiniteUp[i] = -2 ;
    }
  }

// Can't go on without a suitable finite infinity, can we?
#ifdef USE_SMALL_LARGE
  const double large = 1.0e10 ;
#else
  const double large = 1.0e20 ;
#endif

/*
  Decide which columns we're going to look at. There are columns already queued
  in colsToDo_, but sometimes we want to look at all of them. Don't suck in
  prohibited columns. See comments at the head of the routine for fill_level.

  NOTE the overload on usefulColumnInt_. It was assigned above to whichFree
       (indices of interesting columns). We'll be ok because columns are
       consumed out of look at one per main loop iteration, but not all
       columns are interesting.

  Original comment: if gone from 2 to 3 look at all
*/
  int numberLook = prob->numberColsToDo_ ;
  int iLook ;
  int *look = prob->colsToDo_ ;
  if (fill_level < 0) {
    look = prob->usefulColumnInt_+n ;
    if (!prob->anyProhibited()) {
      CoinIotaN(look,n,0) ;
      numberLook = n ;
    } else {
      numberLook = 0 ;
      for (iLook = 0 ; iLook < n ; iLook++) 
	if (!prob->colProhibited(iLook))
	  look[numberLook++] = iLook ;
    }
  }
/*
  Step through the columns of interest looking for suitable x(tgt).

  Interesting columns are limited by number of nonzeros to minimise fill-in
  during substitution.
*/
  bool infeas = false ;
  const int maxLook = abs(fill_level) ;
  for (iLook = 0 ; iLook < numberLook ; iLook++) {
    const int tgtcol = look[iLook] ;
    const int tgtcol_len = colLengths[tgtcol] ;

    if (tgtcol_len <= 0 || tgtcol_len > maxLook) continue ;
/*
  Set up to reconnoiter the column.

  The initial value for ait_max is chosen to make sure that anything that
  satisfies the stability check is big enough to use (though we'd clearly like
  something better).
*/
    const CoinBigIndex kcs = colStarts[tgtcol] ;
    const CoinBigIndex kce = kcs+tgtcol_len ;
    const bool singletonCol = (tgtcol_len == 1) ;
    bool possibleRow = false ;
    bool singletonRow = false ;
    double ait_max = 20*ZTOLDP2 ;
/*
  If this is a singleton column, the only concern is that the row is not a
  singleton row (that has its own, simpler, transform: slack_doubleton). But
  make sure we're not dealing with some tiny a(it).

  Note that there's no point in marking a singleton row. By definition, we
  won't encounter it again.
*/
    if (singletonCol) {
      const int i = rowIndices[kcs] ;
      singletonRow = (rowLengths[i] == 1) ;
      possibleRow = (fabs(colCoeffs[kcs]) > ZTOLDP2) ;
    } else {
      
/*
  If the column is not a singleton, we'll need a numerically stable
  substitution formula. Check that this is possible.  One of the entangled
  rows must be an equality with a numerically stable coefficient, at least
  .1*MAX{i}a(it).
*/
      for (CoinBigIndex kcol = kcs ; kcol < kce ; ++kcol) {
	const int i = rowIndices[kcol] ;
	if (rowLengths[i] == 1) {
	  singletonRow = true ;
	  break ;
	}
	const double abs_ait = fabs(colCoeffs[kcol]) ;
	ait_max = CoinMax(ait_max,abs_ait) ;
	if (fabs(rlo[i]-rup[i]) < feasTol && abs_ait > .1*ait_max) {
	  possibleRow = true ;
	}
      }
    }
    if (singletonRow || !possibleRow) continue ;
/*
  The column has possibilities. Walk the column, calculate row activity
  bounds L(i) and U(i) for suitable entangled rows, then calculate the
  improvement (if any) on the column bounds for l(j) and u(j). The goal is to
  satisfy the implied free condition over all entangled rows and find at least
  one row suitable for a substitution formula (if the column is not a natural
  singleton).

  Suitable: If this is a natural singleton, we need to look at the single
  entangled constraint.  If we're attempting to create a singleton by
  substitution, only look at equalities with stable coefficients. If x(t) is
  integral, make sure the scaled rhs will be integral.
*/
#   if PRESOLVE_DEBUG > 2
    std::cout
      << "  Checking x(" << tgtcol << "), " << tgtcol_len << " nonzeros"
      << ", l(" << tgtcol << ") " << clo[tgtcol] << ", u(" << tgtcol
      << ") " << cup[tgtcol] << ", c(" << tgtcol << ") " << cost[tgtcol]
      << "." << std::endl ;
#   endif
    const double lt = clo[tgtcol] ;
    const double ut = cup[tgtcol] ;
    double impliedLow = -COIN_DBL_MAX ;
    double impliedHigh = COIN_DBL_MAX ;
    int subst_ndx = -1 ;
    int subst_len = n ;
    for (CoinBigIndex kcol = kcs ; kcol < kce ; ++kcol) {
      const int i = rowIndices[kcol] ;

      assert(infiniteUp[i] != -3) ;
      if (infiniteUp[i] <= -2) continue ;

      const double ait = colCoeffs[kcol] ;
      const int leni = rowLengths[i] ;
      const double rloi = rlo[i] ;
      const double rupi = rup[i] ;
/*
  A suitable row for substitution must
    * be an equality;
    * the entangled coefficient must be large enough to be numerically stable;
    * if x(t) is integer, the constant term in the substitution formula must be
      integer.
*/
      bool rowiOK = (fabs(rloi-rupi) < feasTol) && (fabs(ait) > .1*ait_max) ;
      rowiOK = rowiOK && ((integerType[tgtcol] == 0) ||
                          (fabs((rloi/ait)-floor((rloi/ait)+0.5)) < feasTol)) ;
/*
  If we don't already have L(i) and U(i), calculate now. Check for useless and
  infeasible constraints when that's done.
*/
      int infUi = 0 ;
      int infLi = 0 ;
      double maxUi = 0.0 ;
      double maxLi = 0.0 ;
      const CoinBigIndex krs = rowStarts[i] ;
      const CoinBigIndex kre = krs+leni ;

      if (infiniteUp[i] == -1) {
	for (CoinBigIndex krow = krs ; krow < kre ; ++krow) {
	  const double aik = rowCoeffs[krow] ;
	  const int k = colIndices[krow] ;
	  const double lk = clo[k] ;
	  const double uk = cup[k] ;
	  if (aik > 0.0) {
	    if (uk < large) 
	      maxUi += uk*aik ;
	    else
	      ++infUi ;
	    if (lk > -large) 
	      maxLi += lk*aik ;
	    else
	      ++infLi ;
	  } else if (aik < 0.0) {
	    if (uk < large) 
	      maxLi += uk*aik ;
	    else
	      ++infLi ;
	    if (lk > -large) 
	      maxUi += lk*aik ;
	    else
	      ++infUi ;
	  }
	}
	const double maxUinf = maxUi+infUi*1.0e31 ;
	const double maxLinf = maxLi-infLi*1.0e31 ;
	if (maxUinf <= rupi+feasTol && maxLinf >= rloi-feasTol) {
	  infiniteUp[i] = -2 ;
	} else if (maxUinf < rloi-feasTol && !fixInfeasibility) {
	  prob->status_|= 1 ;
	  infeas = true ;
	  prob->messageHandler()->message(COIN_PRESOLVE_ROWINFEAS,
					  prob->messages())
	    << i << rloi << rupi << CoinMessageEol ;
	  infiniteUp[i] = -3 ;
	} else if (maxLinf > rupi+feasTol && !fixInfeasibility) {
	  prob->status_|= 1 ;
	  infeas = true ;
	  prob->messageHandler()->message(COIN_PRESOLVE_ROWINFEAS,
					  prob->messages())
	    << i << rloi << rupi << CoinMessageEol ;
	  infiniteUp[i] = -3 ;
	} else {
	  infiniteUp[i] = infUi ;
	  infiniteDown[i] = infLi ;
	  maxUp[i] = maxUi ;
	  maxDown[i] = maxLi ;
	}
      } else {
        infUi = infiniteUp[i] ;
	infLi = infiniteDown[i] ;
	maxUi = maxUp[i] ;
	maxLi = maxDown[i] ;
      }
#     if PRESOLVE_DEBUG > 2
      std::cout
        << "    row(" << i << ") " << leni << " nonzeros, blow " << rloi
	<< ", L (" << infLi << "," << maxLi
	<< "), U (" << infUi << "," << maxUi
	<< "), b " << rupi ;
      if (infeas) std::cout << " infeas" ;
      if (infiniteUp[i] == -2) std::cout << " useless" ;
      std::cout << "." << std::endl ;
#     endif
/*
  If we're infeasible, no sense checking further; escape the implied bound
  loop. The other possibility is that we've just discovered the constraint
  is useless, in which case we just move on to the next one in the column.
*/
      if (infeas) break ;
      if (infiniteUp[i] == -2) continue ;
      assert(infiniteUp[i] >= 0 && infiniteUp[i] <= leni) ;
/*
  At this point we have L(i) and U(i), expressed as finite and infinite
  components, and constraint i is neither useless or infeasible. Calculate
  the implied bounds l'(t) and u'(t) on x(t). The calculation (for a(it) > 0)
  is
    u'(t) <= (b(i) - (L(i)-a(it)l(t)))/a(it) = l(t)+(b(i)-L(i))/a(it)
    l'(t) >= (blow(i) - (U(i)-a(it)u(t)))/a(it) = u(t)+(blow(i)-U(i))/a(it)
  Insert the appropriate flips for a(it) < 0. Notice that if there's exactly
  one infinite contribution to L(i) or U(i) and x(t) is responsible, then the
  finite portion of L(i) or U(i) is already correct.

  Cut some slack for possible numerical inaccuracy if the finite portion of
  L(i) or U(i) is very large. If the new bound is very large, force it to
  infinity.
*/
      double ltprime = -COIN_DBL_MAX ;
      double utprime = COIN_DBL_MAX ;
      if (ait > 0.0) {
	if (rloi > -large) {
	  if (!infUi) {
	    assert(ut < large) ;
	    ltprime = ut+(rloi-maxUi)/ait ;
	    if (fabs(maxUi) > 1.0e8 && !singletonCol)
	      ltprime -= 1.0e-12*fabs(maxUi) ;
	  } else if (infUi == 1 && ut > large) {
	    ltprime = (rloi-maxUi)/ait ;
	    if (fabs(maxUi) > 1.0e8 && !singletonCol)
	      ltprime -= 1.0e-12*fabs(maxUi) ;
	  } else {
	    ltprime = -COIN_DBL_MAX ;
	  }
	  impliedLow = CoinMax(impliedLow,ltprime) ;
	}
	if (rupi < large) {
	  if (!infLi) {
	    assert(lt > -large) ;
	    utprime = lt+(rupi-maxLi)/ait ;
	    if (fabs(maxLi) > 1.0e8 && !singletonCol)
	      utprime += 1.0e-12*fabs(maxLi) ;
	  } else if (infLi == 1 && lt < -large) {
	    utprime = (rupi-maxLi)/ait ;
	    if (fabs(maxLi) > 1.0e8 && !singletonCol)
	      utprime += 1.0e-12*fabs(maxLi) ;
	  } else {
	    utprime = COIN_DBL_MAX ;
	  }
	  impliedHigh = CoinMin(impliedHigh,utprime) ;
	}
      } else {
	if (rloi > -large) {
	  if (!infUi) {
	    assert(lt > -large) ;
	    utprime = lt+(rloi-maxUi)/ait ;
	    if (fabs(maxUi) > 1.0e8 && !singletonCol)
	      utprime += 1.0e-12*fabs(maxUi) ;
	  } else if (infUi == 1 && lt < -large) {
	    utprime = (rloi-maxUi)/ait ;
	    if (fabs(maxUi) > 1.0e8 && !singletonCol)
	      utprime += 1.0e-12*fabs(maxUi) ;
	  } else {
	    utprime = COIN_DBL_MAX ;
	  }
	  impliedHigh = CoinMin(impliedHigh,utprime) ;
	}
	if (rupi < large) {
	  if (!infLi) {
	    assert(ut < large) ;
	    ltprime = ut+(rupi-maxLi)/ait ;
	    if (fabs(maxLi) > 1.0e8 && !singletonCol)
	      ltprime -= 1.0e-12*fabs(maxLi) ;
	  } else if (infLi == 1 && ut > large) {
	    ltprime = (rupi-maxLi)/ait ;
	    if (fabs(maxLi) > 1.0e8 && !singletonCol)
	      ltprime -= 1.0e-12*fabs(maxLi) ;
	  } else {
	    ltprime = -COIN_DBL_MAX ;
	  }
	  impliedLow = CoinMax(impliedLow,ltprime) ;
	}
      }
#     if PRESOLVE_DEBUG > 2
      std::cout
        << "    row(" << i << ") l'(" << tgtcol << ") " << ltprime
	<< ", u'(" << tgtcol << ") " << utprime ;
      if (lt <= impliedLow && impliedHigh <= ut)
	std::cout << "; implied free satisfied" ;
      std::cout << "." << std::endl ;
#     endif
/*
  For x(t) integral, see if a substitution formula based on row i will
  preserve integrality.  The final check in this clause aims to preserve
  SOS equalities (i.e., don't eliminate a non-trivial SOS equality from
  the system using this transform).

  Note that this can't be folded into the L(i)/U(i) loop because the answer
  changes with x(t).

  Original comment: can only accept if good looking row
*/
      if (integerType[tgtcol]) {
	possibleRow = true ;
	bool allOnes = true ;
	for (CoinBigIndex krow = krs ; krow < kre ; ++krow) {
	  const int j = colIndices[krow] ;
	  const double scaled_aij = rowCoeffs[krow]/ait ;
	  if (fabs(scaled_aij) != 1.0)
	    allOnes = false ;
	  if (!integerType[j] ||
	      fabs(scaled_aij-floor(scaled_aij+0.5)) > feasTol) {
	    possibleRow = false ;
	    break ;
	  }
	}
	if (rloi == 1.0 && leni >= 5 && stopSomeStuff && allOnes)
	  possibleRow = false ;
	rowiOK = rowiOK && possibleRow ;
      }
/*
  Do we have a winner? If we have an incumbent, prefer the one with fewer
  coefficients.
*/
      if (rowiOK) {
	if (subst_ndx < 0 || (leni < subst_len)) {
#         if PRESOLVE_DEBUG > 2
          std::cout
	    << "    row(" << i << ") now candidate for x(" << tgtcol << ")."
	    << std::endl ;
#         endif
	  subst_ndx = i ;
	  subst_len = leni ;
	}
      }
    }

    if (infeas) break ;
/*
  Can we do the transform? If so, subst_ndx will have a valid row.
  Record the implied free variable and the equality we'll use to substitute
  it out. Take the row out of the running --- we can't use the same row
  for two substitutions.
*/
    if (lt <= impliedLow && impliedHigh <= ut &&
        (subst_ndx >= 0 || singletonRow)) {
      implied_free[numberFree] = subst_ndx ;
      infiniteUp[subst_ndx] = -4 ;
      whichFree[numberFree++] = tgtcol ;
#     if PRESOLVE_DEBUG > 1
      std::cout
        << "  x(" << tgtcol << ") implied free by row " << subst_ndx
	<< std::endl ;
#     endif
    }
  }

  delete[] infiniteDown ;
  delete[] infiniteUp ;
  delete[] maxDown ;
  delete[] maxUp ;

/*
  If we're infeasible, there's nothing more to be done.
*/
  if (infeas) {
#   if PRESOLVE_SUMMARY > 0 || PRESOLVE_DEBUG > 0
    std::cout << "  IMPLIED_FREE: infeasible." << std::endl ;
#   endif
    return (next) ;
  }

/*
  We have a list of implied free variables, each with a row that can be used
  to substitute the variable to singleton status if the variable is not a
  natural singleton. The loop here will only process natural singletons.
  We'll hand the remainder to subst_constraint_action below, if there is
  a remainder.

  The natural singletons processed here are compressed out of whichFree and
  implied_free.
*/
  int unprocessed = 0 ;
  for (iLook = 0 ; iLook < numberFree ; iLook++) {
    const int tgtcol = whichFree[iLook] ;

    if (colLengths[tgtcol] != 1) {
      whichFree[unprocessed] = whichFree[iLook] ;
      implied_free[unprocessed] = implied_free[iLook] ;
      unprocessed++ ;
      continue ;
    }

    const int tgtrow = implied_free[iLook] ;
    const int tgtrow_len = rowLengths[tgtrow] ;

    const CoinBigIndex kcs = colStarts[tgtcol] ;
    const double tgtcol_coeff = colCoeffs[kcs] ;
    const double tgtcol_cost = cost[tgtcol] ;

    const CoinBigIndex krs = rowStarts[tgtrow] ;
    const CoinBigIndex kre = krs+tgtrow_len ;
    if (tgtcol_cost != 0.0) {
      // Check costs don't make unstable
      //double minOldCost=COIN_DBL_MAX;
      double maxOldCost=0.0;
      //double minNewCost=COIN_DBL_MAX;
      double maxNewCost=0.0;
      for (CoinBigIndex krow = krs ; krow < kre ; krow++) {
	const int j = colIndices[krow] ;
	if (j != tgtcol) {
	  double oldCost = cost[j] ;
	  double newCost = oldCost - (tgtcol_cost*rowCoeffs[krow])/tgtcol_coeff ;
	  oldCost = fabs(oldCost);
	  newCost = fabs(newCost);
	  //minOldCost=CoinMin(minOldCost,oldCost);
	  maxOldCost=CoinMax(maxOldCost,oldCost);
	  //minNewCost=CoinMin(minNewCost,newCost);
	  maxNewCost=CoinMax(maxNewCost,newCost);
	}
      }
      if (maxNewCost>1000.0*(maxOldCost+1.0) /*&& maxOldCost*/) {
	//printf("too big %d tgtcost %g maxOld %g maxNew %g\n",
	//     tgtcol,tgtcol_cost,maxOldCost,maxNewCost);
	continue;
      }
    }
/*
  Initialise the postsolve action. We need to remember the row and column.
*/
    action *s = &actions[nactions++] ;
    s->row = tgtrow ;
    s->col = tgtcol ;
    s->clo = clo[tgtcol] ;
    s->cup = cup[tgtcol] ;
    s->rlo = rlo[tgtrow] ;
    s->rup = rup[tgtrow] ;
    s->ninrow = tgtrow_len ;
    s->rowels = presolve_dupmajor(rowCoeffs,colIndices,tgtrow_len,krs) ;
    s->costs = NULL ;
/*
  We're processing a singleton, hence no substitutions in the matrix, but we
  do need to fix up the cost vector. The substitution formula is
    x(t) = (rhs(i) - SUM{j\t}a(ik)x(k))/a(it)
  hence
    c'(k) = c(k)-c(t)a(ik)/a(it)
  and there's a constant offset
    c(t)rhs(i)/a(it).
  where rhs(i) is one of blow(i) or b(i).

  For general constraints where blow(i) != b(i), we need to take a bit
  of care. If only one of blow(i) or b(i) is finite, that's the one to
  use. Where we have two finite but unequal bounds, choose the bound that
  will result in the most favourable value for x(t). For minimisation, if
  c(t) < 0 we want to maximise x(t), so choose b(i) if a(it) > 0, blow(i)
  if a(it) < 0.  A bit of case analysis says choose b(i) when c(t)a(it) <
  0, blow(i) when c(t)a(it) > 0. We shouldn't be here if both row bounds
  are infinite.

  Fortunately, the objective coefficients are not affected by this.
*/
    if (tgtcol_cost != 0.0) {
      double tgtrow_rhs = rup[tgtrow] ;
      if (fabs(rlo[tgtrow]-rup[tgtrow]) > feasTol) {
	const double rlot = rlo[tgtrow] ;
	const double rupt = rup[tgtrow] ;
        if (rlot > -COIN_DBL_MAX && rupt < COIN_DBL_MAX) {
	  if ((tgtcol_cost*tgtcol_coeff) > 0)
	    tgtrow_rhs = rlot ;
	  else
	    tgtrow_rhs = rupt ;
	} else if (rupt >= COIN_DBL_MAX) {
	  tgtrow_rhs = rlot ;
	}
      }
      assert(fabs(tgtrow_rhs) <= large) ;
      double *save_costs = new double[tgtrow_len] ;

      for (CoinBigIndex krow = krs ; krow < kre ; krow++) {
	const int j = colIndices[krow] ;
	save_costs[krow-krs] = cost[j] ;
	cost[j] -= (tgtcol_cost*rowCoeffs[krow])/tgtcol_coeff ;
      }
      prob->change_bias((tgtcol_cost*tgtrow_rhs)/tgtcol_coeff) ;
      cost[tgtcol] = 0.0 ;
      s->costs = save_costs ;
    }
/*
  Remove the row from the column-major representation, queuing up each column
  for reconsideration. Then remove the row from the row-major representation.
*/
    for (CoinBigIndex krow = krs ; krow < kre ; krow++) {
      const int j = colIndices[krow] ;
      presolve_delete_from_col(tgtrow,j,colStarts,colLengths,rowIndices,
      			       colCoeffs) ;
      if (colLengths[j] == 0) {
        PRESOLVE_REMOVE_LINK(prob->clink_,j) ;
      } else {
	prob->addCol(j) ;
      }
    }
    PRESOLVE_REMOVE_LINK(clink,tgtcol) ;
    colLengths[tgtcol] = 0 ;

    PRESOLVE_REMOVE_LINK(rlink,tgtrow) ;
    rowLengths[tgtrow] = 0 ;
    rlo[tgtrow] = 0.0 ;
    rup[tgtrow] = 0.0 ;
  }
/*
  We're done with the natural singletons. Trim actions to length and create
  the postsolve object.
*/
  if (nactions) {
#   if PRESOLVE_SUMMARY > 0 || PRESOLVE_DEBUG > 0
    printf("NIMPLIED FREE:  %d\n", nactions) ;
#   endif
    action *actions1 = new action[nactions] ;
    CoinMemcpyN(actions, nactions, actions1) ;
    next = new implied_free_action(nactions,actions1,next) ;
  } 
  delete [] actions ;
# if PRESOLVE_DEBUG > 0
  std::cout
    << "  IMPLIED_FREE: identified " << numberFree
    << " implied free transforms, processed " << numberFree-unprocessed
    << " natural singletons." << std::endl ;
# endif

/*
  Now take a stab at the columns that aren't natural singletons, if there are
  any left.
*/
  if (unprocessed != 0) {
    next = subst_constraint_action::presolve(prob,implied_free,whichFree,
    					     unprocessed,next,maxLook) ;
  }
/*
  Give some feedback to the presolve driver. If we aren't finding enough
  candidates and haven't reached the limit, bump fill_level and return a
  negated value. The presolve driver can tweak this value or simply return
  it on the next call. See the top of the routine for a full explanation.
*/
  if (numberFree < 30 && maxLook < prob->maxSubstLevel_) {
    fill_level = -(maxLook+1) ;
  } else {
    fill_level = maxLook ;
  }

# if COIN_PRESOLVE_TUNING > 0
  double thisTime ;
  if (prob->tuning_) thisTime = CoinCpuTime() ;
# endif
# if PRESOLVE_CONSISTENCY > 0 || PRESOLVE_DEBUG > 0
  presolve_consistent(prob) ;
  presolve_links_ok(prob) ;
  presolve_check_sol(prob) ;
  presolve_check_nbasic(prob) ;
# endif
# if PRESOLVE_DEBUG > 0 || COIN_PRESOLVE_TUNING > 0
  int droppedRows = prob->countEmptyRows()-startEmptyRows ;
  int droppedColumns = prob->countEmptyCols()-startEmptyColumns ;
  std::cout
    << "Leaving implied_free_action::presolve, fill level " << fill_level
    << ", " << droppedRows << " rows, "
    << droppedColumns << " columns dropped" ;
# if COIN_PRESOLVE_TUNING > 0
  std::cout << " in " << thisTime-startTime << "s" ;
# endif
  std::cout << "." << std::endl ;
# endif

  return (next) ;
}


const char *implied_free_action::name() const
{
  return ("implied_free_action") ;
}


/*
  Restore the target constraint and target column x(t) eliminated by the
  implied free presolve action. We'll solve the target constraint to get a
  value for x(t), so by construction the constraint is tight.

  For range constraints, we need to consider both the upper and lower row
  bounds when calculating a value for x(t). x(t) can end up at one of its
  column bounds or strictly between bounds.

  Then there's the matter of patching up the basis and solution. The
  natural thing to do is to make the logical nonbasic and x(t) basic.
  No corrections to duals or reduced costs are required because we built
  that correction into the problem when we modified the objective. Work the
  linear algebra; it's very neat.
  
  It will frequently happen that the implied bounds on x(t) are simply equal
  to the existing column bounds and the value we calculate here will be at
  one of the original column bounds. This leaves x(t) degenerate basic. The
  original version of this routine looked for this and attempted to make
  x(t) nonbasic and use the logical as the basic variable. The linear
  algebra for this is ugly. To calculate the proper correction for the dual
  variables requires the basis inverse (terms do not conveniently cancel).
  The original code made no attempt to fix the duals and applied a correction
  to the reduced costs that was valid only for the nonbasic partition. It
  accepted the result if it was dual feasible. In general this would leave
  slack dual constraints. It's really not possible to predict the effect
  going forward on subsequent postsolve transforms. Nor is it clear to
  me that a degenerate basic architectural is inherently worse than a
  degenerate basic logical. This version of the code always makes x(t) basic.
*/

  

void implied_free_action::postsolve(CoinPostsolveMatrix *prob) const
{
  const action *const actions = actions_ ;
  const int nactions = nactions_ ;

# if PRESOLVE_DEBUG > 0 || PRESOLVE_CONSISTENCY > 0
  char *cdone	= prob->cdone_ ;
  char *rdone	= prob->rdone_ ;
# if PRESOLVE_DEBUG > 0
  std::cout
    << "Entering implied_free_action::postsolve, " << nactions
    << " transforms to undo." << std::endl ;
# endif
  presolve_check_threads(prob) ;
  presolve_check_free_list(prob) ;
  presolve_check_sol(prob,2,2,2) ;
  presolve_check_nbasic(prob) ;
# endif

/*
  Unpack the column-major representation.
*/
  CoinBigIndex *colStarts = prob->mcstrt_ ;
  int *colLengths = prob->hincol_ ;
  int *rowIndices = prob->hrow_ ;
  double *colCoeffs = prob->colels_ ;
  CoinBigIndex *link = prob->link_ ;
  CoinBigIndex &free_list = prob->free_list_ ;
/*
  Column bounds, row bounds, and cost.
*/
  double *clo = prob->clo_ ;
  double *cup = prob->cup_ ;
  double *rlo = prob->rlo_ ;
  double *rup = prob->rup_ ;
  double *cost = prob->cost_ ;
/*
  Solution, reduced costs, duals, row activity.
*/
  double *sol = prob->sol_ ;
  double *rcosts = prob->rcosts_ ;
  double *acts = prob->acts_ ;
  double *rowduals = prob->rowduals_ ;
/*
  In your dreams ... hardwired to minimisation.
*/
  const double maxmin = 1.0 ;
/*
  And a suitably small infinity.
*/
  const double large = 1.0e20 ;
/*
  Open a loop to restore the row and column for each action. Start by
  unpacking the action. There won't be saved costs if the original cost c(t)
  was zero.
*/
  for (const action *f = &actions[nactions-1] ; actions <= f ; f--) {

    const int tgtrow = f->row ;
    const int tgtcol = f->col ;
    const int tgtrow_len = f->ninrow ;
    const double *tgtrow_coeffs = f->rowels ;
    const int *tgtrow_cols =
        reinterpret_cast<const int *>(tgtrow_coeffs+tgtrow_len) ;
    const double *saved_costs = f->costs ;

#   if PRESOLVE_DEBUG > 2
    std::cout
      << "  restoring col " << tgtcol << " row " << tgtrow ;
    if (saved_costs != 0) std::cout << ", modified costs" ;
    std::cout << "." << std::endl ;
#   endif

/*
  Restore the target row and column and the original cost coefficients.
  We need to initialise the target column; for others, just bump the
  coefficient count. While we're restoring the row, pick off the coefficient
  for x(t) and calculate the row activity.
*/
    double tgt_coeff = 0.0 ;
    double tgtrow_act = 0.0 ;
    for (int krow = 0 ; krow < tgtrow_len ; krow++) {
      const int j = tgtrow_cols[krow] ;
      const double atj = tgtrow_coeffs[krow] ;

      assert(free_list >= 0 && free_list < prob->bulk0_) ;
      CoinBigIndex kk = free_list ;
      free_list = link[free_list] ;
      link[kk] = colStarts[j] ;
      colStarts[j] = kk ;
      colCoeffs[kk] = atj ;
      rowIndices[kk] = tgtrow ;

      if (saved_costs) cost[j] = saved_costs[krow] ;

      if (j == tgtcol) {
	colLengths[j] = 1 ;
	clo[tgtcol] = f->clo ;
	cup[tgtcol] = f->cup ;
	rcosts[j] = -cost[tgtcol]/atj ;
	tgt_coeff = atj ;
      } else {
	colLengths[j]++ ;
	tgtrow_act += atj*sol[j] ;
      }
    }
    rlo[tgtrow] = f->rlo ;
    rup[tgtrow] = f->rup ;

    PRESOLVEASSERT(fabs(tgt_coeff) > ZTOLDP) ;
#   if PRESOLVE_DEBUG > 0 || PRESOLVE_CONSISTENCY > 0
    cdone[tgtcol] = IMPLIED_FREE ;
    rdone[tgtrow] = IMPLIED_FREE ;
#   endif
#   if PRESOLVE_CONSISTENCY > 0
    presolve_check_free_list(prob) ;
#   endif
/*
  Calculate a value for x(t).  We have two possible values for x(t),
  calculated against the upper and lower bound of the constraint. x(t)
  could end up at one of its original bounds or it could end up strictly
  within bounds. In either event, the constraint will be tight.

  The code simply forces the calculated value for x(t) to be within
  bounds. Arguably it should complain more loudly as this likely indicates
  algorithmic error or numerical inaccuracy. You'll get a warning if
  debugging is enabled.
*/
    double xt_lo,xt_up ;
    if (tgt_coeff > 0) {
      xt_lo = (rlo[tgtrow]-tgtrow_act)/tgt_coeff ;
      xt_up = (rup[tgtrow]-tgtrow_act)/tgt_coeff ;
    } else {
      xt_lo = (rup[tgtrow]-tgtrow_act)/tgt_coeff ;
      xt_up = (rlo[tgtrow]-tgtrow_act)/tgt_coeff ;
    }
    const double lt = clo[tgtcol] ;
    const double ut = cup[tgtcol] ;

#   if PRESOLVE_DEBUG > 0 || PRESOLVE_CONSISTENCY > 0
    bool chklo = true ;
    bool chkup = true ;
    if (tgt_coeff > 0) {
      if (rlo[tgtrow] < -large) chklo = false ;
      if (rup[tgtrow] > large) chkup = false ;
    } else {
      if (rup[tgtrow] > large) chklo = false ;
      if (rlo[tgtrow] < -large) chkup = false ;
    }
    if (chklo && (xt_lo < lt-prob->ztolzb_)) {
      std::cout
        << "  LOW CSOL (implied_free): x(" << tgtcol << ") lb " << lt
	<< ", sol = " << xt_lo << ", err " << (lt-xt_lo)
	<< "." << std::endl ;
    }
    if (chkup && (xt_up > ut+prob->ztolzb_)) {
      std::cout
        << "  HIGH CSOL (implied_free): x(" << tgtcol << ") ub " << ut
	<< ", sol = " << xt_up << ", err " << (xt_up-ut)
	<< "." << std::endl ;
    }
#   if PRESOLVE_DEBUG > 2
    std::cout
      << "  x(" << tgtcol << ") lb " << lt << " lo " << xt_lo
      << ", up " << xt_up << " ub " << ut << "." << std::endl ;
#   endif
#   endif

    xt_lo = CoinMax(xt_lo,lt) ;
    xt_up = CoinMin(xt_up,ut) ;

/*
  Time to make x(t) basic and the logical nonbasic.  The sign of the
  dual determines the tight row bound, which in turn determines the value
  of x(t). Because the row is tight, activity is by definition equal to
  the bound.

  Coin convention is that a <= constraint puts a lower bound on the slack and
  a >= constraint puts an upper bound on the slack. Case analysis
  (minimisation) says:

  dual >= 0 ==> reduced cost <= 0 ==> NBUB ==> finite rlo
  dual <= 0 ==> reduced cost >= 0 ==> NBLB ==> finite rup
*/
    const double ct = maxmin*cost[tgtcol] ;
    double possibleDual = ct/tgt_coeff ;
    rowduals[tgtrow] = possibleDual ;
    if (possibleDual >= 0 && rlo[tgtrow] > -large) {
      sol[tgtcol] = (rlo[tgtrow]-tgtrow_act)/tgt_coeff ;
      acts[tgtrow] = rlo[tgtrow] ;
      prob->setRowStatus(tgtrow,CoinPrePostsolveMatrix::atUpperBound) ;
    } else
    if (possibleDual <= 0 && rup[tgtrow] < large) {
      sol[tgtcol] = (rup[tgtrow]-tgtrow_act)/tgt_coeff ;
      acts[tgtrow] = rup[tgtrow] ;
      prob->setRowStatus(tgtrow,CoinPrePostsolveMatrix::atLowerBound) ;
    } else {
      assert(rup[tgtrow] < large || rlo[tgtrow] > -large) ;
      if (rup[tgtrow] < large) {
	sol[tgtcol] = (rup[tgtrow]-tgtrow_act)/tgt_coeff ;
	acts[tgtrow] = rup[tgtrow] ;
	prob->setRowStatus(tgtrow,CoinPrePostsolveMatrix::atLowerBound) ;
      } else {
	sol[tgtcol] = (rlo[tgtrow]-tgtrow_act)/tgt_coeff ;
	acts[tgtrow] = rlo[tgtrow] ;
	prob->setRowStatus(tgtrow,CoinPrePostsolveMatrix::atUpperBound) ;
      }
#     if PRESOLVE_DEBUG > 0
      std::cout
        << "BAD ROW STATUS row " << tgtrow << ": dual "
	<< rowduals[tgtrow] << " but row "
	<< ((rowduals[tgtrow] > 0)?"upper":"lower")
	<< " bound is not finite; forcing status "
	<< prob->rowStatusString(tgtrow)
	<< "." << std::endl ;
#     endif
    }
    prob->setColumnStatus(tgtcol,CoinPrePostsolveMatrix::basic) ;
    rcosts[tgtcol] = 0.0 ;

#   if PRESOLVE_DEBUG > 2
    std::cout
      << "  x(" << tgtcol << ") B dj " << rcosts[tgtcol] << "." << std::endl ;
    std::cout
      << "  row " << tgtrow << " dual " << rowduals[tgtrow] << "."
      << std::endl ;
#   endif
    PRESOLVEASSERT(acts[tgtrow] >= rlo[tgtrow]-1.0e-5 &&
		   acts[tgtrow] <= rup[tgtrow]+1.0e-5) ;

#   if PRESOLVE_DEBUG > 2
/*
   Debug code to compare the reduced costs against a calculation from
   scratch as c(j)-ya(j).
*/
    for (int krow = 0 ; krow < tgtrow_len ; krow++) {
      const int j = tgtrow_cols[krow] ;
      const int lenj = colLengths[j] ;
      double dj = cost[j] ;
      CoinBigIndex kcol = colStarts[j] ;
      for (int cntj = 0 ; cntj < lenj ; ++cntj) {
	const int i = rowIndices[kcol] ;
	const double aij = colCoeffs[kcol] ;
	dj -= rowduals[i]*aij ;
	kcol = link[kcol] ;
      }
      if (fabs(dj-rcosts[j]) > 1.0e-3) {
        std::cout
	  << "  cbar(" << j << ") update " << rcosts[j]
	  << " expected " << dj << " err " << fabs(dj-rcosts[j])
	  << "." << std::endl ;
      }
    }
#   endif
  }

# if PRESOLVE_CONSISTENCY > 0 || PRESOLVE_DEBUG > 0
  presolve_check_threads(prob) ;
  presolve_check_sol(prob,2,2,2) ;
  presolve_check_nbasic(prob) ;
# if PRESOLVE_DEBUG > 0
  std::cout << "Leaving implied_free_action::postsolve." << std::endl ;
# endif
# endif

  return ;
}



implied_free_action::~implied_free_action() 
{ 
  int i ;
  for (i=0;i<nactions_;i++) {
    deleteAction(actions_[i].rowels,double *) ;
    deleteAction( actions_[i].costs,double *) ;
  }
  deleteAction(actions_,action *) ;
}

