/* $Id: CoinPresolveDoubleton.cpp 1581 2013-04-06 12:48:50Z stefan $ */
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include <stdio.h>
#include <math.h>

#include "CoinFinite.hpp"
#include "CoinHelperFunctions.hpp"
#include "CoinPresolveMatrix.hpp"

#include "CoinPresolveEmpty.hpp"	// for DROP_COL/DROP_ROW
#include "CoinPresolveZeros.hpp"
#include "CoinPresolveFixed.hpp"
#include "CoinPresolveDoubleton.hpp"

#include "CoinPresolvePsdebug.hpp"
#include "CoinMessage.hpp"

#if PRESOLVE_DEBUG > 0 || PRESOLVE_CONSISTENCY > 0
#include "CoinPresolvePsdebug.hpp"
#endif

namespace {	/* begin unnamed local namespace */

#if PRESOLVE_DEBUG > 0
#define DBGPARAM(zz_param_zz) zz_param_zz
#else
#define DBGPARAM(zz_param_zz)
#endif

/*
   This routine does the grunt work needed to substitute x for y in all rows i
   where coeff[i,y] != 0. Given ax + by = c, we have
  
  	 y = (c - a*x)/b = c/b + (-a/b)*x

   Suppose we're fixing row i. We need to adjust the row bounds by
   -coeff[i,y]*(c/b) and coeff[i,x] by coeff[i,y]*(-a/b). The value
   c/b is passed as the bounds_factor, and -a/b as the coeff_factor.

   row0 is the doubleton row.  It is assumed that coeff[row0,y] has been
   removed from the column major representation before this routine is
   called. (Otherwise, we'd have to check for it to avoid a useless row
   update.)

   Both the row and col representations are updated. There are two cases:

   * coeff[i,x] != 0:
	in the column rep, modify coeff[i,x] ;
	in the row rep, modify coeff[i,x] and drop coeff[i,y].

   * coeff[i,x] == 0 (i.e., non-existent):
        in the column rep, add coeff[i,x]; mcstrt is modified if the column
	must be moved ;
	in the row rep, convert coeff[i,y] to coeff[i,x].
  
   The row and column reps are inconsistent during the routine and at
   completion.  In the row rep, column x and y are updated except for
   the doubleton row, and in the column rep only column x is updated
   except for coeff[row0,x]. On return, column y and row row0 will be deleted
   and consistency will be restored.
*/

bool elim_doubleton (const char *DBGPARAM(msg),
		     CoinBigIndex *mcstrt, 
		     double *rlo, double *rup,
		     double *colels,
		     int *hrow, int *hcol,
		     int *hinrow, int *hincol,
		     presolvehlink *clink, int ncols,
		     CoinBigIndex *mrstrt, double *rowels,
		     double coeff_factor,
		     double bounds_factor,
		     int DBGPARAM(row0),
		     int icolx, int icoly)

{
  CoinBigIndex kcsx = mcstrt[icolx] ;
  CoinBigIndex kcex = kcsx + hincol[icolx] ;

# if PRESOLVE_DEBUG > 1
  printf("%s %d x=%d y=%d cf=%g bf=%g nx=%d yrows=(", msg,
	 row0, icolx, icoly, coeff_factor, bounds_factor, hincol[icolx]) ;
# endif
/*
  Open a loop to scan column y. For each nonzero coefficient (row,y),
  update column x and the row bounds for the row.

  The initial assert checks that we're properly updating column x.
*/
  CoinBigIndex base = mcstrt[icoly] ;
  int numberInY = hincol[icoly] ;
  for (int kwhere = 0 ; kwhere < numberInY ; kwhere++) {
    PRESOLVEASSERT(kcex == kcsx+hincol[icolx]) ;
    const CoinBigIndex kcoly = base+kwhere ;
    const int row = hrow[kcoly] ;
    const double coeffy = colels[kcoly] ;
    double delta = coeffy*coeff_factor ;
/*
  Look for coeff[row,x], then update accordingly.
*/
    CoinBigIndex kcolx = presolve_find_row1(row,kcsx,kcex,hrow) ;
#   if PRESOLVE_DEBUG > 1
    printf("%d%s ",row,(kcolx < kcex)?"+":"") ;
#   endif
/*
  Case 1: coeff[i,x] != 0: update it in column and row reps; drop coeff[i,y]
  from row rep.
*/
    if (kcolx < kcex) {
      colels[kcolx] += delta ;

      const CoinBigIndex kmi =
        presolve_find_col(icolx,mrstrt[row],mrstrt[row]+hinrow[row],hcol) ;
      rowels[kmi] = colels[kcolx] ;
      presolve_delete_from_row(row,icoly,mrstrt,hinrow,hcol,rowels) ;
/*
  Case 2: coeff[i,x] == 0: add it in the column rep; convert coeff[i,y] in
  the row rep. presolve_expand_col ensures an empty entry exists at the
  end of the column. The location of column x may change with expansion.
*/
    } else {
      const bool no_mem = presolve_expand_col(mcstrt,colels,hrow,hincol,
					      clink,ncols,icolx) ;
      if (no_mem) return (true) ;
	  
      kcsx = mcstrt[icolx] ;
      kcex = kcsx+hincol[icolx] ;
      // recompute y as well
      base = mcstrt[icoly] ;

      hrow[kcex] = row ;
      colels[kcex] = delta ;
      hincol[icolx]++ ;
      kcex++ ;

      CoinBigIndex k2 =
        presolve_find_col(icoly,mrstrt[row],mrstrt[row]+hinrow[row],hcol) ;
      hcol[k2] = icolx ;
      rowels[k2] = delta ;
    }
/*
  Update the row bounds, if necessary. Avoid updating finite infinity.
*/
    if (bounds_factor != 0.0) {
      delta = coeffy*bounds_factor ;
      if (-PRESOLVE_INF < rlo[row])
	rlo[row] -= delta ;
      if (rup[row] < PRESOLVE_INF)
	rup[row] -= delta ;
    }
  }

# if PRESOLVE_DEBUG > 1
  printf(")\n") ;
# endif

  return (false) ;
}

#if PRESOLVE_DEBUG > 0
/*
  Debug helpers
*/

double *doubleton_mult ;
int *doubleton_id ;

void check_doubletons (const CoinPresolveAction *paction)
{
  const CoinPresolveAction * paction0 = paction ;
  
  if (paction) {
    check_doubletons(paction->next) ;
    
    if (strcmp(paction0->name(),"doubleton_action") == 0) {
      const doubleton_action *daction = 
	dynamic_cast<const doubleton_action *>(paction0) ;
      for (int i = daction->nactions_-1 ; i >= 0 ; --i) {
	int icolx = daction->actions_[i].icolx ;
	int icoly = daction->actions_[i].icoly ;
	double coeffx = daction->actions_[i].coeffx ;
	double coeffy = daction->actions_[i].coeffy ;

	doubleton_mult[icoly] = -coeffx/coeffy ;
	doubleton_id[icoly] = icolx ;
      }
    }
  }
}

void check_doubletons1(const CoinPresolveAction * paction,
		       int ncols)
{
  doubleton_mult = new double[ncols] ;
  doubleton_id = new int[ncols] ;
  int i ;
  for ( i=0; i<ncols; ++i)
    doubleton_id[i] = i ;
  check_doubletons(paction) ;
  double minmult = 1.0 ;
  int minid = -1 ;
  for ( i=0; i<ncols; ++i) {
    double mult = 1.0 ;
    int j = i ;
    if (doubleton_id[j] != j) {
      printf("MULTS (%d):  ", j) ;
      while (doubleton_id[j] != j) {
	printf("%d %g, ", doubleton_id[j], doubleton_mult[j]) ;
	mult *= doubleton_mult[j] ;
	j = doubleton_id[j] ;
      }
      printf(" == %g\n", mult) ;
      if (minmult > fabs(mult)) {
	minmult = fabs(mult) ;
	minid = i ;
      }
    }
  }
  if (minid != -1)
    printf("MIN MULT:  %d %g\n", minid, minmult) ;
}
#endif  // PRESOLVE_DEBUG


} /* end unnamed local namespace */



/*
  It is always the case that one of the variables of a doubleton is, or
  can be made, implied free, but neither will necessarily be a singleton.
  Since in the case of a doubleton the number of non-zero entries will never
  increase if one is eliminated, it makes sense to always eliminate them.

  The col rep and row rep must be consistent.
 */
const CoinPresolveAction
  *doubleton_action::presolve (CoinPresolveMatrix *prob,
			      const CoinPresolveAction *next)

{
# if PRESOLVE_DEBUG > 0 || PRESOLVE_CONSISTENCY > 0
# if PRESOLVE_DEBUG > 0
  std::cout
    << "Entering doubleton_action::presolve; considering "
    << prob->numberRowsToDo_ << " rows." << std::endl ;
# endif
  presolve_consistent(prob) ;
  presolve_links_ok(prob) ;
  presolve_check_sol(prob) ;
  presolve_check_nbasic(prob) ;
# endif
# if PRESOLVE_DEBUG > 0 || COIN_PRESOLVE_TUNING > 0
  int startEmptyRows = 0 ;
  int startEmptyColumns = 0 ;
  startEmptyRows = prob->countEmptyRows() ;
  startEmptyColumns = prob->countEmptyCols() ;
# if COIN_PRESOLVE_TUNING > 0
  double startTime = 0.0 ;
  if (prob->tuning_) startTime = CoinCpuTime() ;
# endif
# endif

  const int n = prob->ncols_ ;
  const int m = prob->nrows_ ;

/*
  Unpack column-major and row-major representations, along with rim vectors.
*/
  CoinBigIndex *colStarts = prob->mcstrt_ ;
  int *colLengths = prob->hincol_ ;
  double *colCoeffs = prob->colels_ ;
  int *rowIndices = prob->hrow_ ;
  presolvehlink *clink = prob->clink_ ;

  double *clo = prob->clo_ ;
  double *cup = prob->cup_ ;

  CoinBigIndex *rowStarts = prob->mrstrt_ ;
  int *rowLengths = prob->hinrow_ ;
  double *rowCoeffs = prob->rowels_ ;
  int *colIndices = prob->hcol_ ;
  presolvehlink *rlink = prob->rlink_ ;

  double *rlo = prob->rlo_ ;
  double *rup = prob->rup_ ;

  const unsigned char *integerType = prob->integerType_ ;

  double *cost = prob->cost_ ;

  int numberLook = prob->numberRowsToDo_ ;
  int *look = prob->rowsToDo_ ;
  const double ztolzb	= prob->ztolzb_ ;
  const double ztolzero = 1.0e-12 ;

  action *actions = new action [m] ;
  int nactions = 0 ;

/*
  zeros will hold columns that should be groomed to remove explicit zeros when
  we're finished.

  fixed will hold columns that have ended up as fixed variables.
*/
  int *zeros = prob->usefulColumnInt_ ;
  int nzeros = 0 ;

  int *fixed = zeros+n ;
  int nfixed = 0 ;

  unsigned char *rowstat = prob->rowstat_ ;
  double *acts	= prob->acts_ ;
  double *sol = prob->sol_ ;
/*
  More like `ignore infeasibility'.
*/
  bool fixInfeasibility = ((prob->presolveOptions_&0x4000) != 0) ;

/*
  Open the main loop to scan for doubleton candidates.
*/
  for (int iLook = 0 ; iLook < numberLook ; iLook++) {
    const int tgtrow = look[iLook] ;
/*
  We need an equality with two coefficients. Avoid isolated constraints, lest
  both variables vanish.

  Failure of the assert indicates that the row- and column-major
  representations are out of sync.
*/
    if ((rowLengths[tgtrow] != 2) ||
        (fabs(rup[tgtrow]-rlo[tgtrow]) > ZTOLDP)) continue ;

    const CoinBigIndex krs = rowStarts[tgtrow] ;
    int tgtcolx = colIndices[krs] ;
    int tgtcoly = colIndices[krs+1] ;

    PRESOLVEASSERT(colLengths[tgtcolx] > 0 || colLengths[tgtcoly] > 0) ;
    if (colLengths[tgtcolx] == 1 && colLengths[tgtcoly] == 1) continue ;
/*
  Avoid prohibited columns and fixed columns. Make sure the coefficients are
  nonzero.
  JJF - test should allow one to be prohibited as long as you leave that
  one.  I modified earlier code but hope I have got this right.
*/
    if (prob->colProhibited(tgtcolx) && prob->colProhibited(tgtcoly))
      continue ;
    if (fabs(rowCoeffs[krs]) < ZTOLDP2 || fabs(rowCoeffs[krs+1]) < ZTOLDP2)
      continue ;
    if ((fabs(cup[tgtcolx]-clo[tgtcolx]) < ZTOLDP) ||
	(fabs(cup[tgtcoly]-clo[tgtcoly]) < ZTOLDP)) continue ;

#   if PRESOLVE_DEBUG > 2
    std::cout
      << "  row " << tgtrow << " colx " << tgtcolx << " coly " << tgtcoly
      << " passes preliminary eval." << std::endl ;
#   endif

/*
  Find this row in each column. The indices are not const because we may flip
  them below, once we decide which column will be eliminated.
*/
    CoinBigIndex krowx =
        presolve_find_row(tgtrow,colStarts[tgtcolx],
			  colStarts[tgtcolx]+colLengths[tgtcolx],rowIndices) ;
    double coeffx = colCoeffs[krowx] ;
    CoinBigIndex krowy =
        presolve_find_row(tgtrow,colStarts[tgtcoly],
			  colStarts[tgtcoly]+colLengths[tgtcoly],rowIndices) ;
    double coeffy = colCoeffs[krowy] ;
    const double rhs = rlo[tgtrow] ;
/*
  Avoid obscuring a requirement for integrality.

  If only one variable is integer, keep it and substitute for the continuous
  variable.

  If both are integer, substitute only for the forms x = k*y (k integral
  and non-empty intersection on bounds on x) or x = 1-y, where both x and
  y are binary.

  flag bits for integerStatus: 0x01: x integer;  0x02: y integer

  This bit of code works because 0 is continuous, 1 is integer. Make sure
  that's true.
*/
    assert((integerType[tgtcolx] == 0) || (integerType[tgtcolx] == 1)) ;
    assert((integerType[tgtcoly] == 0) || (integerType[tgtcoly] == 1)) ;

    int integerX = integerType[tgtcolx];
    int integerY = integerType[tgtcoly];
    /* if one prohibited then treat that as integer. This
       may be pessimistic - but will catch SOS etc */
    if (prob->colProhibited2(tgtcolx))
      integerX=1;
    if (prob->colProhibited2(tgtcoly))
      integerY=1;
    int integerStatus = (integerY<<1)|integerX ;

    if (integerStatus == 3) {
      int good = 0 ;
      double rhs2 = rhs ;
      if (coeffx < 0.0) {
	coeffx = -coeffx ;
	rhs2 += 1 ;
      }
      if ((cup[tgtcolx] == 1.0) && (clo[tgtcolx] == 0.0) &&
	  (fabs(coeffx-1.0) < 1.0e-7) && !prob->colProhibited2(tgtcoly))
	good = 1 ;
      if (coeffy < 0.0) {
	coeffy = -coeffy ;
	rhs2 += 1 ;
      }
      if ((cup[tgtcoly] == 1.0) && (clo[tgtcoly] == 0.0) &&
	  (fabs(coeffy-1.0) < 1.0e-7) && !prob->colProhibited2(tgtcolx))
	good |= 2 ;
      if (!(good == 3 && fabs(rhs2-1.0) < 1.0e-7))
	integerStatus = -1 ;
/*
  Not x+y = 1. Try for ax+by = 0
*/
      if (integerStatus < 0 && rhs == 0.0) {
	coeffx = colCoeffs[krowx] ;
	coeffy = colCoeffs[krowy] ;
	double ratio ;
	bool swap = false ;
	if (fabs(coeffx) > fabs(coeffy)) {
	  ratio = coeffx/coeffy ;
	} else {
	  ratio = coeffy/coeffx ;
	  swap = true ;
	}
	ratio = fabs(ratio) ;
	if (fabs(ratio-floor(ratio+0.5)) < ztolzero) {
	  integerStatus = swap ? 2 : 1 ;
	}
      }
/*
  One last try --- just require an integral substitution formula.

  But ax+by = 0 above is a subset of ax+by = c below and should pass the
  test below. For that matter, so will x+y = 1. Why separate special cases
  above?  -- lh, 121106 --
*/
      if (integerStatus < 0) {
	bool canDo = false ;
	coeffx = colCoeffs[krowx] ;
	coeffy = colCoeffs[krowy] ;
	double ratio ;
	bool swap = false ;
	double rhsRatio ;
	if (fabs(coeffx) > fabs(coeffy)) {
	  ratio = coeffx/coeffy ;
	  rhsRatio = rhs/coeffx ;
	} else {
	  ratio = coeffy/coeffx ;
	  rhsRatio = rhs/coeffy ;
	  swap = true ;
	}
	ratio = fabs(ratio) ;
	if (fabs(ratio-floor(ratio+0.5)) < ztolzero) {
	  // possible
	  integerStatus = swap ? 2 : 1 ;
	  // but check rhs
	  if (rhsRatio==floor(rhsRatio+0.5))
	    canDo=true ;
	}
#       ifdef COIN_DEVELOP2
	if (canDo)
	  printf("Good CoinPresolveDoubleton tgtcolx %d (%g and bounds %g %g) tgtcoly %d (%g and bound %g %g) - rhs %g\n",
		 tgtcolx,colCoeffs[krowx],clo[tgtcolx],cup[tgtcolx],
		 tgtcoly,colCoeffs[krowy],clo[tgtcoly],cup[tgtcoly],rhs) ;
	else
	printf("Bad CoinPresolveDoubleton tgtcolx %d (%g) tgtcoly %d (%g) - rhs %g\n",
	       tgtcolx,colCoeffs[krowx],tgtcoly,colCoeffs[krowy],rhs) ;
#       endif
	if (!canDo)
	  continue ;
      }
    }
/*
  We've resolved integrality concerns. If we concluded that we need to
  switch the roles of x and y because of integrality, do that now. If both
  variables are continuous, we may still want to swap for numeric stability.
  Eliminate the variable with the larger coefficient.
*/
    if (integerStatus == 2) {
      CoinSwap(tgtcoly,tgtcolx) ;
      CoinSwap(krowy,krowx) ;
    } else if (integerStatus == 0) {
      if (fabs(colCoeffs[krowy]) < fabs(colCoeffs[krowx])) {
	CoinSwap(tgtcoly,tgtcolx) ;
	CoinSwap(krowy,krowx) ;
      }
    }
/*
  Don't eliminate y just yet if it's entangled in a singleton row (we want to
  capture that explicit bound in a column bound).
*/
    const CoinBigIndex kcsy = colStarts[tgtcoly] ;
    const CoinBigIndex kcey = kcsy+colLengths[tgtcoly] ;
    bool singletonRow = false ;
    for (CoinBigIndex kcol = kcsy ; kcol < kcey ; kcol++) {
      if (rowLengths[rowIndices[kcol]] == 1) {
        singletonRow = true ;
	break ;
      }
    }
    // skip if y prohibited
    if (singletonRow || prob->colProhibited2(tgtcoly)) continue ;

    coeffx = colCoeffs[krowx] ;
    coeffy = colCoeffs[krowy] ;
#   if PRESOLVE_DEBUG > 2
    std::cout
      << "  doubleton row " << tgtrow << ", keep x(" << tgtcolx
      << ") elim x(" << tgtcoly << ")." << std::endl ;
#   endif
    PRESOLVE_DETAIL_PRINT(printf("pre_doubleton %dC %dC %dR E\n",
				 tgtcoly,tgtcolx,tgtrow)) ;
/*
  Capture the existing columns and other information before we start to modify
  the constraint system. Save the shorter column.
*/
    action *s = &actions[nactions] ;
    nactions++ ;
    s->row = tgtrow ;
    s->icolx = tgtcolx ;
    s->clox = clo[tgtcolx] ;
    s->cupx = cup[tgtcolx] ;
    s->costx = cost[tgtcolx] ;
    s->icoly = tgtcoly ;
    s->costy = cost[tgtcoly] ;
    s->rlo = rlo[tgtrow] ;
    s->coeffx = coeffx ;
    s->coeffy = coeffy ;
    s->ncolx = colLengths[tgtcolx] ;
    s->ncoly = colLengths[tgtcoly] ;
    if (s->ncoly < s->ncolx) {
      s->colel	= presolve_dupmajor(colCoeffs,rowIndices,colLengths[tgtcoly],
				    colStarts[tgtcoly],tgtrow) ;
      s->ncolx = 0 ;
    } else {
      s->colel = presolve_dupmajor(colCoeffs,rowIndices,colLengths[tgtcolx],
				   colStarts[tgtcolx],tgtrow) ;
      s->ncoly = 0 ;
    }
/*
  Move finite bound information from y to x, so that y is implied free.
    a x + b y = c
    l1 <= x <= u1
    l2 <= y <= u2
   
    l2 <= (c - a x) / b <= u2
    b/-a > 0 ==> (b l2 - c) / -a <= x <= (b u2 - c) / -a
    b/-a < 0 ==> (b u2 - c) / -a <= x <= (b l2 - c) / -a
*/
    {
      double lo1 = -PRESOLVE_INF ;
      double up1 = PRESOLVE_INF ;
      
      if (-PRESOLVE_INF < clo[tgtcoly]) {
	if (coeffx*coeffy < 0)
	  lo1 = (coeffy*clo[tgtcoly]-rhs)/-coeffx ;
	else 
	  up1 = (coeffy*clo[tgtcoly]-rhs)/-coeffx ;
      }
      
      if (cup[tgtcoly] < PRESOLVE_INF) {
	if (coeffx*coeffy < 0)
	  up1 = (coeffy*cup[tgtcoly]-rhs)/-coeffx ;
	else 
	  lo1 = (coeffy*cup[tgtcoly]-rhs)/-coeffx ;
      }
/*
  Don't forget the objective coefficient.
    costy y = costy ((c - a x) / b) = (costy c)/b + x (costy -a)/b
*/
      cost[tgtcolx] += (cost[tgtcoly]*-coeffx)/coeffy ;
      prob->change_bias((cost[tgtcoly]*rhs)/coeffy) ;
/*
  The transfer of bounds could make x infeasible. Patch it up if the problem
  is minor or if the user was so incautious as to instruct us to ignore it.
  Prefer an integer value if there's one nearby. If there's nothing to be
  done, break out of the main loop.
*/
      {
	double lo2 = CoinMax(clo[tgtcolx],lo1) ;
	double up2 = CoinMin(cup[tgtcolx],up1) ;
	if (lo2 > up2) {
	  if (lo2 <= up2+prob->feasibilityTolerance_ || fixInfeasibility) {
	    double nearest = floor(lo2+0.5) ;
	    if (fabs(nearest-lo2) < 2.0*prob->feasibilityTolerance_) {
	      lo2 = nearest ;
	      up2 = nearest ;
	    } else {
	      lo2 = up2 ;
	    }
	  } else {
	    prob->status_ |= 1 ;
	    prob->messageHandler()->message(COIN_PRESOLVE_COLINFEAS,
	    				    prob->messages())
		 << tgtcolx << lo2 << up2 << CoinMessageEol ;
	    break ;
	  }
	}
#       if PRESOLVE_DEBUG > 2
	std::cout
	  << "  x(" << tgtcolx << ") lb " << clo[tgtcolx] << " --> " << lo2
	  << ", ub " << cup[tgtcolx] << " --> " << up2 << std::endl ;
#       endif
	clo[tgtcolx] = lo2 ;
	cup[tgtcolx] = up2 ;
/*
  Do we have a solution to maintain? If so, take a stab at it. If x ends up at
  bound, prefer to set it nonbasic, but if we're short of basic variables
  after eliminating y and the logical for the row, make it basic.

  This code will snap the value of x to bound if it's within the primal
  feasibility tolerance.
*/
	if (rowstat && sol) {
	  int numberBasic = 0 ;
	  double movement = 0 ;
	  if (prob->columnIsBasic(tgtcolx))
	    numberBasic++ ;
	  if (prob->columnIsBasic(tgtcoly))
	    numberBasic++ ;
	  if (prob->rowIsBasic(tgtrow))
	    numberBasic++ ;
	  if (sol[tgtcolx] <= lo2+ztolzb) {
	    movement = lo2-sol[tgtcolx] ;
	    sol[tgtcolx] = lo2 ;
	    prob->setColumnStatus(tgtcolx,
	    			  CoinPrePostsolveMatrix::atLowerBound) ;
	  } else if (sol[tgtcolx] >= up2-ztolzb) {
	    movement = up2-sol[tgtcolx] ;
	    sol[tgtcolx] = up2 ;
	    prob->setColumnStatus(tgtcolx,
	    			  CoinPrePostsolveMatrix::atUpperBound) ;
	  }
	  if (numberBasic > 1)
	    prob->setColumnStatus(tgtcolx,CoinPrePostsolveMatrix::basic) ;
/*
  We need to compensate if x was forced to move. Beyond that, even if x
  didn't move, we've forced y = (c-ax)/b, and that might not have been
  true before. So even if x didn't move, y may have moved. Note that the
  constant term c/b is subtracted out as the constraints are modified,
  so we don't include it when calculating movement for y.
*/
	  if (movement) { 
	    const CoinBigIndex kkcsx = colStarts[tgtcolx] ;
	    const CoinBigIndex kkcex = kkcsx+colLengths[tgtcolx] ;
	    for (CoinBigIndex kcol = kkcsx ; kcol < kkcex ; kcol++) {
	      int row = rowIndices[kcol] ;
	      if (rowLengths[row])
		acts[row] += movement*colCoeffs[kcol] ;
	    }
	  }
	  movement = ((-coeffx*sol[tgtcolx])/coeffy)-sol[tgtcoly] ;
	  if (movement) {
	    const CoinBigIndex kkcsy = colStarts[tgtcoly] ;
	    const CoinBigIndex kkcey = kkcsy+colLengths[tgtcoly] ;
	    for (CoinBigIndex kcol = kkcsy ; kcol < kkcey ; kcol++) {
	      int row = rowIndices[kcol] ;
	      if (rowLengths[row])
		acts[row] += movement*colCoeffs[kcol] ;
	    }
	  }
	}
	if (lo2 == up2)
	  fixed[nfixed++] = tgtcolx ;
      }
    }
/*
  We're done transferring bounds from y to x, and we've patched up the
  solution if one existed to patch. One last thing to do before we eliminate
  column y and the doubleton row: put column x and the entangled rows on
  the lists of columns and rows to look at in the next round of transforms.
*/
    {
      prob->addCol(tgtcolx) ;
      const CoinBigIndex kkcsy = colStarts[tgtcoly] ;
      const CoinBigIndex kkcey = kkcsy+colLengths[tgtcoly] ;
      for (CoinBigIndex kcol = kkcsy ; kcol < kkcey ; kcol++) {
	int row = rowIndices[kcol] ;
	prob->addRow(row) ;
      }
      const CoinBigIndex kkcsx = colStarts[tgtcolx] ;
      const CoinBigIndex kkcex = kkcsx+colLengths[tgtcolx] ;
      for (CoinBigIndex kcol = kkcsx ; kcol < kkcex ; kcol++) {
	int row = rowIndices[kcol] ;
	prob->addRow(row) ;
      }
    }

/*
  Empty tgtrow in the column-major matrix.  Deleting the coefficient for
  (tgtrow,tgtcoly) is a bit costly (given that we're about to drop the whole
  column), but saves the trouble of checking for it in elim_doubleton.
*/
    presolve_delete_from_col(tgtrow,tgtcolx,
    			     colStarts,colLengths,rowIndices,colCoeffs) ;
    presolve_delete_from_col(tgtrow,tgtcoly,
    			     colStarts,colLengths,rowIndices,colCoeffs) ;
/*
  Drop tgtrow in the row-major representation: set the length to 0
  and reclaim the major vector space in bulk storage.
*/
    rowLengths[tgtrow] = 0 ;
    PRESOLVE_REMOVE_LINK(rlink,tgtrow) ;

/*
  Transfer the colx factors to coly. This modifies coefficients in column x
  as it removes coefficients in column y.
*/
    bool no_mem = elim_doubleton("ELIMD",
				 colStarts,rlo,rup,colCoeffs,
				 rowIndices,colIndices,rowLengths,colLengths,
				 clink,n, 
				 rowStarts,rowCoeffs,
				 -coeffx/coeffy,
				 rhs/coeffy,
				 tgtrow,tgtcolx,tgtcoly) ;
    if (no_mem) 
      throwCoinError("out of memory","doubleton_action::presolve") ;

/*
  Eliminate coly entirely from the col rep. We'll want to groom colx to remove
  explicit zeros.
*/
    colLengths[tgtcoly] = 0 ;
    PRESOLVE_REMOVE_LINK(clink, tgtcoly) ;
    cost[tgtcoly] = 0.0 ;

    rlo[tgtrow] = 0.0 ;
    rup[tgtrow] = 0.0 ;

    zeros[nzeros++] = tgtcolx ;

#   if PRESOLVE_CONSISTENCY > 0
    presolve_consistent(prob) ;
    presolve_links_ok(prob) ;
#   endif
  }
/*
  Tidy up the collected actions and clean up explicit zeros and fixed
  variables. Don't bother unless we're feasible (status of 0).
*/
  if (nactions && !prob->status_) {
#   if PRESOLVE_SUMMARY > 0
    printf("NDOUBLETONS:  %d\n", nactions) ;
#   endif
    action *actions1 = new action[nactions] ;
    CoinMemcpyN(actions, nactions, actions1) ;

    next = new doubleton_action(nactions, actions1, next) ;

    if (nzeros)
      next = drop_zero_coefficients_action::presolve(prob, zeros, nzeros, next) ;
    if (nfixed)
      next = remove_fixed_action::presolve(prob, fixed, nfixed, next) ;
  }

  deleteAction(actions,action*) ;

# if COIN_PRESOLVE_TUNING > 0
  if (prob->tuning_) double thisTime = CoinCpuTime() ;
# endif
# if PRESOLVE_CONSISTENCY > 0 || PRESOLVE_DEBUG > 0
  presolve_check_sol(prob) ;
# endif
# if PRESOLVE_DEBUG > 0 || COIN_PRESOLVE_TUNING > 0
  int droppedRows = prob->countEmptyRows()-startEmptyRows ;
  int droppedColumns = prob->countEmptyCols()-startEmptyColumns ;
  std::cout
    << "Leaving doubleton_action::presolve, " << droppedRows << " rows, "
    << droppedColumns << " columns dropped" ;
# if COIN_PRESOLVE_TUNING > 0
  std::cout << " in " << thisTime-startTime << "s" ;
# endif
  std::cout << "." << std::endl ;
# endif

  return (next) ;
}



/*
  Reintroduce the column (y) and doubleton row (irow) removed in presolve.
  Correct the other column (x) involved in the doubleton, update the solution,
  etc.

  A fair amount of complication arises because the presolve transform saves the
  shorter of x or y. Postsolve thus includes portions to restore either.
*/
void doubleton_action::postsolve(CoinPostsolveMatrix *prob) const
{
  const action *const actions = actions_ ;
  const int nactions = nactions_ ;

  double *colels	= prob->colels_ ;
  int *hrow		= prob->hrow_ ;
  CoinBigIndex *mcstrt	= prob->mcstrt_ ;
  int *hincol		= prob->hincol_ ;
  int *link		= prob->link_ ;

  double *clo	= prob->clo_ ;
  double *cup	= prob->cup_ ;

  double *rlo	= prob->rlo_ ;
  double *rup	= prob->rup_ ;

  double *dcost	= prob->cost_ ;

  double *sol	= prob->sol_ ;
  double *acts	= prob->acts_ ;
  double *rowduals = prob->rowduals_ ;
  double *rcosts = prob->rcosts_ ;

  unsigned char *colstat = prob->colstat_ ;
  unsigned char *rowstat = prob->rowstat_ ;

  const double maxmin	= prob->maxmin_ ;

  CoinBigIndex &free_list = prob->free_list_ ;

  const double ztolzb	= prob->ztolzb_ ;
  const double ztoldj	= prob->ztoldj_ ;
  const double ztolzero = 1.0e-12 ;

  int nrows = prob->nrows_ ;

  // Arrays to rebuild the unsaved column.
  int *index1 = new int[nrows] ;
  double *element1 = new double[nrows] ;
  CoinZeroN(element1,nrows) ;

# if PRESOLVE_DEBUG > 0 || PRESOLVE_CONSISTENCY > 0
  char *cdone	= prob->cdone_ ;
  char *rdone	= prob->rdone_ ;

  presolve_check_threads(prob) ;
  presolve_check_sol(prob,2,2,2) ;
  presolve_check_nbasic(prob) ;
  presolve_check_reduced_costs(prob) ;

# if PRESOLVE_DEBUG > 0
  std::cout
    << "Entering doubleton_action::postsolve, " << nactions
    << " transforms to undo." << std::endl ;
# endif
# endif
/*
  The outer loop: step through the doubletons in this array of actions.
  The first activity is to unpack the doubleton.
*/
  for (const action *f = &actions[nactions-1] ; actions <= f ; f--) {

    const int irow = f->row ;
    const double lo0 = f->clox ;
    const double up0 = f->cupx ;


    const double coeffx = f->coeffx ;
    const double coeffy = f->coeffy ;
    const int jcolx = f->icolx ;
    const int jcoly = f->icoly ;

    const double rhs = f->rlo ;

#   if PRESOLVE_DEBUG > 2
    std::cout
      << std::endl
      << "  restoring doubleton " << irow << ", elim x(" << jcoly
      << "), kept x(" << jcolx << "); stored col " ;
    if (f->ncoly)
      std::cout << jcoly ;
    else 
      std::cout << jcolx ;
    std::cout << "." << std::endl ;
    std::cout
      << "  x(" << jcolx << ") " << prob->columnStatusString(jcolx) << " "
      << clo[jcolx] << " <= " << sol[jcolx] << " <= " << cup[jcolx]
      << "; cj " << f->costx << " dj " << rcosts[jcolx] << "." << std::endl ;
#   endif
/*
  jcolx is in the problem (for whatever reason), and the doubleton row (irow)
  and column (jcoly) have only been processed by empty row/column postsolve
  (i.e., reintroduced with length 0).
*/
    PRESOLVEASSERT(cdone[jcolx] && rdone[irow] == DROP_ROW) ;
    PRESOLVEASSERT(cdone[jcoly] == DROP_COL) ;

/*
  Restore bounds for doubleton row, bounds and objective coefficient for x,
  objective for y.

  Original comment: restoration of rlo and rup likely isn't necessary.
*/
    rlo[irow] = f->rlo ;
    rup[irow] = f->rlo ;

    clo[jcolx] = lo0 ;
    cup[jcolx] = up0 ;

    dcost[jcolx] = f->costx ;
    dcost[jcoly] = f->costy ;

/*
  Set primal solution for y (including status) and row activity for the
  doubleton row. The motivation (up in presolve) for wanting coeffx < coeffy
  is to avoid inflation into sol[y]. Since this is a (satisfied) equality,
  activity is the rhs value and the logical is nonbasic.
*/
    const double diffy = rhs-coeffx*sol[jcolx] ;
    if (fabs(diffy) < ztolzero)
      sol[jcoly] = 0 ;
    else
      sol[jcoly] = diffy/coeffy ;
    acts[irow] = rhs ;
    if (rowstat)
      prob->setRowStatus(irow,CoinPrePostsolveMatrix::atLowerBound) ;

#   if PRESOLVE_DEBUG > 2
/*
  Original comment: I've forgotten what this is about

  We have sol[y] = (rhs - coeffx*sol[x])/coeffy. As best I can figure,
  the original check here tested for the possibility of loss of significant
  digits through cancellation, followed by inflation if coeffy is small.
  The hazard is clear enough, but the test was puzzling. Overly complicated
  and it generated false warnings for the common case of sol[y] a clean zero.
  Replaced with something that I hope is more useful. The tolerances are, sad
  to say, completely arbitrary.    -- lh, 121106 --
*/
    if ((fabs(diffy) < 1.0e-6) && (fabs(diffy) >= ztolzero) &&
        (fabs(coeffy) < 1.0e-3))
      std::cout
        << "  loss of significance? rhs " << rhs
	<< " (coeffx*sol[jcolx])" << (coeffx*sol[jcolx])
	<< " diff " << diffy << "." << std::endl ;
#   endif

/*
  Time to get into the correction/restoration of coefficients for columns x
  and y, with attendant correction of row bounds and activities. Accumulate
  partial reduced costs (missing the contribution from the doubleton row) so
  that we can eventually calculate a dual for the doubleton row.
*/
    double djy = maxmin*dcost[jcoly] ;
    double djx = maxmin*dcost[jcolx] ;
/*
  We saved column y in the action, so we'll use it to reconstruct column x.
  There are two aspects: correction of existing x coefficients, and fill in.
  Given
    coeffx'[k] = coeffx[k]+coeffy[k]*coeff_factor
  we have
    coeffx[k] = coeffx'[k]-coeffy[k]*coeff_factor
  where
    coeff_factor = -coeffx[dblton]/coeffy[dblton].

  Keep in mind that the major vector stored in the action does not include
  the coefficient from the doubleton row --- the doubleton coefficients are
  held in coeffx and coeffy.
*/
    if (f->ncoly) {
      int ncoly = f->ncoly-1 ;
      int *indy = reinterpret_cast<int *>(f->colel+ncoly) ;
/*
  Rebuild a threaded column y, starting with the end of the thread and working
  back to the beginning. In the process, accumulate corrections to column x
  in element1 and index1. Fix row bounds and activity as we go (add back the
  constant correction removed in presolve), and accumulate contributions to
  the reduced cost for y. Don't tweak finite infinity.

  The PRESOLVEASSERT says this row should already be present. 
*/
      int ystart = NO_LINK ;
      int nX = 0 ;
      for (int kcol = 0 ; kcol < ncoly ; ++kcol) {
	const int i = indy[kcol] ;
	PRESOLVEASSERT(rdone[i]) ;

	double yValue = f->colel[kcol] ;

	if (-PRESOLVE_INF < rlo[i])
	  rlo[i] += (yValue*rhs)/coeffy ;
	if (rup[i] < PRESOLVE_INF)
	  rup[i] += (yValue*rhs)/coeffy ;

	acts[i] += (yValue*rhs)/coeffy ;

	djy -= rowduals[i]*yValue ;
/*
  Link the coefficient into column y: Acquire the first free slot in the
  bulk arrays and store the row index and coefficient. Then link the slot
  in front of coefficients we've already processed.
*/
	const CoinBigIndex kfree = free_list ;
	assert(kfree >= 0 && kfree < prob->bulk0_) ;
	free_list = link[free_list] ;
	hrow[kfree] = i ;
	colels[kfree] = yValue ;
	link[kfree] = ystart ;
	ystart = kfree ;

#       if PRESOLVE_DEBUG > 4
	std::cout
	  << "  link y " << kfree << " row " << i << " coeff " << yValue
	  << " dual " << rowduals[i] << std::endl ;
#       endif
/*
  Calculate and store the correction to the x coefficient.
*/
	yValue = (yValue*coeffx)/coeffy ;
	element1[i] = yValue ;
	index1[nX++] = i ;
      }
#     if PRESOLVE_CONSISTENCY > 0
      presolve_check_free_list(prob) ;
#     endif
/*
  Handle the coefficients of the doubleton row. Insert coeffy, coeffx.
*/
      const CoinBigIndex kfree = free_list ;
      assert(kfree >= 0 && kfree < prob->bulk0_) ;
      free_list = link[free_list] ;
      hrow[kfree] = irow ;
      colels[kfree] = coeffy ;
      link[kfree] = ystart ;
      ystart = kfree ;

#     if PRESOLVE_DEBUG > 4
      std::cout
	<< "  link y " << kfree << " row " << irow << " coeff " << coeffy
	<< " dual n/a" << std::endl ;
#     endif

      element1[irow] = coeffx ;
      index1[nX++] = irow ;
/*
  Attach the threaded column y to mcstrt and record the length.
*/
      mcstrt[jcoly] = ystart ;
      hincol[jcoly] = f->ncoly ;
/*
  Now integrate the corrections to column x. Scan the column and correct the
  existing entries.  The correction could cancel the existing coefficient and
  we don't want to leave an explicit zero. In this case, relink the column
  around it.  The freed slot is linked at the beginning of the free list.
*/
      CoinBigIndex kcs = mcstrt[jcolx] ;
      CoinBigIndex last_nonzero = NO_LINK ;
      int numberInColumn = hincol[jcolx] ;
      const int numberToDo = numberInColumn ;
      for (int kcol = 0 ; kcol < numberToDo ; ++kcol) {
	const int i = hrow[kcs] ;
	assert(i >= 0 && i < nrows && i != irow) ;
	double value = colels[kcs]+element1[i] ;
	element1[i] = 0.0 ;
	if (fabs(value) >= 1.0e-15) {
	  colels[kcs] = value ;
	  last_nonzero = kcs ;
	  kcs = link[kcs] ;
	  djx -= rowduals[i]*value ;

#         if PRESOLVE_DEBUG > 4
	  std::cout
	    << "  link x " << last_nonzero << " row " << i << " coeff "
	    << value << " dual " << rowduals[i] << std::endl ;
#         endif

	} else {

#         if PRESOLVE_DEBUG > 4
	  std::cout
	    << "  link x skipped row  " << i << " dual "
	    << rowduals[i] << std::endl ;
#         endif

	  numberInColumn-- ;
	  // add to free list
	  int nextk = link[kcs] ;
	  assert(free_list >= 0) ;
	  link[kcs] = free_list ;
	  free_list = kcs ;
	  assert(kcs >= 0) ;
	  kcs = nextk ;
	  if (last_nonzero != NO_LINK)
	    link[last_nonzero] = kcs ;
	  else
	    mcstrt[jcolx] = kcs ;
	}
      }
      if (last_nonzero != NO_LINK)
	link[last_nonzero] = NO_LINK ;
/*
  We've dealt with the existing nonzeros in column x. Any remaining
  nonzeros in element1 will be fill in, which we insert at the beginning of
  the column.
*/
      for (int kcol = 0 ; kcol < nX ; kcol++) {
	const int i = index1[kcol] ;
	double xValue = element1[i] ;
	element1[i] = 0.0 ;
	if (fabs(xValue) >= 1.0e-15) {
	  if (i != irow)
	    djx -= rowduals[i]*xValue ;
	  numberInColumn++ ;
	  CoinBigIndex kfree = free_list ;
	  assert(kfree >= 0 && kfree < prob->bulk0_) ;
	  free_list = link[free_list] ;
	  hrow[kfree] = i ;
	  PRESOLVEASSERT(rdone[hrow[kfree]] || (hrow[kfree] == irow)) ;
	  colels[kfree] = xValue ;
	  link[kfree] = mcstrt[jcolx] ;
	  mcstrt[jcolx] = kfree ;
#         if PRESOLVE_DEBUG > 4
	  std::cout
	    << "  link x " << kfree << " row " << i << " coeff " << xValue
	    << " dual " ;
	  if (i != irow)
	    std::cout << rowduals[i] ;
	  else
	    std::cout << "n/a" ;
	  std::cout << std::endl ;
#         endif
	}
      }
	  
#     if PRESOLVE_CONSISTENCY > 0
      presolve_check_free_list(prob) ;
#     endif
	  
/*
  Whew! Set the column length and we're done.
*/
      assert(numberInColumn) ;
      hincol[jcolx] = numberInColumn ;
    } else {
/*
  Of course, we could have saved column x in the action. Now we need to
  regenerate coefficients of column y.
  Given
    coeffx'[k] = coeffx[k]+coeffy[k]*coeff_factor
  we have
    coeffy[k] = (coeffx'[k]-coeffx[k])*(1/coeff_factor)
  where
    coeff_factor = -coeffx[dblton]/coeffy[dblton].
*/
      const int ncolx = f->ncolx-1 ;
      int *indx = reinterpret_cast<int *> (f->colel+ncolx) ;
/*
  Scan existing column x to find the end. While we're at it, accumulate part
  of the new y coefficients in index1 and element1.
*/
      CoinBigIndex kcs = mcstrt[jcolx] ;
      int nX = 0 ;
      for (int kcol = 0 ; kcol < hincol[jcolx]-1 ; ++kcol) {
	if (colels[kcs]) {
	  const int i = hrow[kcs] ;
	  index1[nX++] = i ;
	  element1[i] = -(colels[kcs]*coeffy)/coeffx ;
	}
	kcs = link[kcs] ;
      }
      if (colels[kcs]) {
        const int i = hrow[kcs] ;
        index1[nX++] = i ;
        element1[i] = -(colels[kcs]*coeffy)/coeffx ;
      }
/*
  Replace column x with the the original column x held in the doubleton action
  (recall that this column does not include coeffx). We first move column
  x to the free list, then thread a column with the original coefficients,
  back to front.  While we're at it, add the second part of the y coefficients
  to index1 and element1.
*/
      link[kcs] = free_list ;
      free_list = mcstrt[jcolx] ;
      int xstart = NO_LINK ;
      for (int kcol = 0 ; kcol < ncolx ; ++kcol) {
	const int i = indx[kcol] ;
	PRESOLVEASSERT(rdone[i] && i != irow) ;

	double xValue = f->colel[kcol] ;
	CoinBigIndex k = free_list ;
	assert(k >= 0 && k < prob->bulk0_) ;
	free_list = link[free_list] ;
	hrow[k] = i ;
	colels[k] = xValue ;
	link[k] = xstart ;
	xstart = k ;

	djx -= rowduals[i]*xValue ;

	xValue = (xValue*coeffy)/coeffx ;
	if (!element1[i]) {
	  element1[i] = xValue ;
	  index1[nX++] = i ;
	} else {
	  element1[i] += xValue ;
	}
      }
#     if PRESOLVE_CONSISTENCY > 0
      presolve_check_free_list(prob) ;
#     endif
/*
  The same, for the doubleton row.
*/
      {
	double xValue = coeffx ;
	CoinBigIndex k = free_list ;
	assert(k >= 0 && k < prob->bulk0_) ;
	free_list = link[free_list] ;
	hrow[k] = irow ;
	colels[k] = xValue ;
	link[k] = xstart ;
	xstart = k ;
	element1[irow] = coeffy ;
	index1[nX++] = irow ;
      }
/*
  Link the new column x to mcstrt and set the length.
*/
      mcstrt[jcolx] = xstart ;
      hincol[jcolx] = f->ncolx ;
/*
  Now get to work building a threaded column y from the nonzeros in element1.
  As before, build the thread in reverse.
*/
      int ystart = NO_LINK ;
      int leny = 0 ;
      for (int kcol = 0 ; kcol < nX ; kcol++) {
	const int i = index1[kcol] ;
	PRESOLVEASSERT(rdone[i] || i == irow) ;
	double yValue = element1[i] ;
	element1[i] = 0.0 ;
	if (fabs(yValue) >= ztolzero) {
	  leny++ ;
	  CoinBigIndex k = free_list ;
	  assert(k >= 0 && k < prob->bulk0_) ;
	  free_list = link[free_list] ;
	  hrow[k] = i ;
	  colels[k] = yValue ;
	  link[k] = ystart ;
	  ystart = k ;
	}
      }
#     if PRESOLVE_CONSISTENCY > 0
      presolve_check_free_list(prob) ;
#     endif
/*
  Tidy up --- link the new column into mcstrt and set the length.
*/
      mcstrt[jcoly] = ystart ;
      assert(leny) ;
      hincol[jcoly] = leny ;
/*
  Now that we have the original y, we can scan it and do the corrections to
  the row bounds and activity, and get a start on a reduced cost for y.
*/
      kcs = mcstrt[jcoly] ;
      const int ny = hincol[jcoly] ;
      for (int kcol = 0 ; kcol < ny ; ++kcol) {
	const int row = hrow[kcs] ;
	const double coeff = colels[kcs] ;
	kcs = link[kcs] ;

	if (row != irow) {
	  
	  // undo elim_doubleton(1)
	  if (-PRESOLVE_INF < rlo[row])
	    rlo[row] += (coeff*rhs)/coeffy ;
	  
	  // undo elim_doubleton(2)
	  if (rup[row] < PRESOLVE_INF)
	    rup[row] += (coeff*rhs)/coeffy ;
	  
	  acts[row] += (coeff*rhs)/coeffy ;
	  
	  djy -= rowduals[row]*coeff ;
	}
      }
    }
#   if PRESOLVE_DEBUG > 2
/*
  Sanity checks. The doubleton coefficients should be linked in the first
  position of the each column (for no good reason except that it makes it much
  easier to write these checks).
*/
#   if PRESOLVE_DEBUG > 4
    std::cout
      << "  kept: saved " << jcolx << " " << coeffx << ", reconstructed "
      << hrow[mcstrt[jcolx]] << " " << colels[mcstrt[jcolx]]
      << "." << std::endl ;
    std::cout
      << "  elim: saved " << jcoly << " " << coeffy << ", reconstructed "
      << hrow[mcstrt[jcoly]] << " " << colels[mcstrt[jcoly]]
      << "." << std::endl ;
#   endif
    assert((coeffx == colels[mcstrt[jcolx]]) &&
	   (coeffy == colels[mcstrt[jcoly]])) ;
#   endif
/*
  Time to calculate a dual for the doubleton row, and settle the status of x
  and y. Ideally, we'll leave x at whatever nonbasic status it currently has
  and make y basic. There's a potential problem, however: Remember that we
  transferred bounds from y to x when we eliminated y. If those bounds were
  tighter than x's original bounds, we may not be able to maintain x at its
  present status, or even as nonbasic.

  We'll make two claims here:

    * If the dual value for the doubleton row is chosen to keep the reduced
      cost djx of col x at its prior value, then the reduced cost djy of col
      y will be 0. (Crank through the linear algebra to convince yourself.)

    * If the bounds on x have loosened, then it must be possible to make y
      nonbasic, because we've transferred the tight bound back to y. (Yeah,
      I'm waving my hands. But it sounds good.  -- lh, 040907 --)

  So ... if we can maintain x nonbasic, then we need to set y basic, which
  means we should calculate rowduals[dblton] so that rcost[jcoly] == 0. We
  may need to change the status of x (an artifact of loosening a bound when
  x was previously a fixed variable).
  
  If we need to push x into the basis, then we calculate rowduals[dblton] so
  that rcost[jcolx] == 0 and make y nonbasic.
*/
#   if PRESOLVE_DEBUG > 2
    std::cout
      << "  pre status: x(" << jcolx << ") " << prob->columnStatusString(jcolx)
      << " " << clo[jcolx] << " <= " << sol[jcolx] << " <= " << cup[jcolx]
      << ", cj " << dcost[jcolx]
      << ", dj " << djx << "." << std::endl ;
    std::cout
      << "  pre status: x(" << jcoly << ") "
      << clo[jcoly] << " <= " << sol[jcoly] << " <= " << cup[jcoly]
      << ", cj " << dcost[jcoly]
      << ", dj " << djy << "." << std::endl ;
#   endif
    if (colstat) {
      bool basicx = prob->columnIsBasic(jcolx) ;
      bool nblbxok = (fabs(lo0 - sol[jcolx]) < ztolzb) &&
		     (rcosts[jcolx] >= -ztoldj) ;
      bool nbubxok = (fabs(up0 - sol[jcolx]) < ztolzb) &&
		     (rcosts[jcolx] <= ztoldj) ;
      if (basicx || nblbxok || nbubxok) {
        if (!basicx) {
	  if (nblbxok) {
	    prob->setColumnStatus(jcolx,
				  CoinPrePostsolveMatrix::atLowerBound) ;
	  } else if (nbubxok) {
	    prob->setColumnStatus(jcolx,
				  CoinPrePostsolveMatrix::atUpperBound) ;
	  }
	}
	prob->setColumnStatus(jcoly,CoinPrePostsolveMatrix::basic) ;
	rowduals[irow] = djy/coeffy ;
	rcosts[jcolx] = djx-rowduals[irow]*coeffx ;
	rcosts[jcoly] = 0.0 ;
      } else {
	prob->setColumnStatus(jcolx,CoinPrePostsolveMatrix::basic) ;
	prob->setColumnStatusUsingValue(jcoly) ;
	rowduals[irow] = djx/coeffx ;
	rcosts[jcoly] = djy-rowduals[irow]*coeffy ;
	rcosts[jcolx] = 0.0 ;
      }
#     if PRESOLVE_DEBUG > 2
      std::cout
        << "  post status: " << irow << " dual " << rowduals[irow]
	<< " rhs " << rlo[irow]
	<< std::endl ;
      std::cout
	<< "  post status: x(" << jcolx << ") "
	<< prob->columnStatusString(jcolx) << " "
	<< clo[jcolx] << " <= " << sol[jcolx] << " <= " << cup[jcolx]
	<< ", cj " << dcost[jcolx]
	<< ", dj = " << rcosts[jcolx] << "." << std::endl ;
      std::cout
	<< "  post status: x(" << jcoly << ") "
	<< prob->columnStatusString(jcoly) << " "
	<< clo[jcoly] << " <= " << sol[jcoly] << " <= " << cup[jcoly]
	<< ", cj " << dcost[jcoly]
	<< ", dj " << rcosts[jcoly] << "." << std::endl ;
/*
  These asserts are valid but need a scaled tolerance to work well over
  a range of problems. Occasionally useful for a hard stop while debugging.

      assert(!prob->columnIsBasic(jcolx) || (fabs(rcosts[jcolx]) < 1.0e-5)) ;
      assert(!prob->columnIsBasic(jcoly) || (fabs(rcosts[jcoly]) < 1.0e-5)) ;
*/
#     endif
    } else {
      // No status array
      // this is the coefficient we need to force col y's reduced cost to 0.0 ;
      // for example, this is obviously true if y is a singleton column
      rowduals[irow] = djy/coeffy ;
      rcosts[jcoly] = 0.0 ;
    }
    
#   if PRESOLVE_DEBUG > 0 || PRESOLVE_CONSISTENCY > 0
/*
  Mark the column and row as processed by doubleton action. Then check
  integrity of the threaded matrix.
*/
    cdone[jcoly] = DOUBLETON ;
    rdone[irow] = DOUBLETON ;
    presolve_check_threads(prob) ;
#   endif
#   if PRESOLVE_DEBUG > 0
/*
  Confirm accuracy of reduced cost for columns x and y.
*/
    {
      CoinBigIndex k = mcstrt[jcolx] ;
      const int nx = hincol[jcolx] ;
      double dj = maxmin*dcost[jcolx] ;
      
      for (int kcol = 0 ; kcol < nx ; ++kcol) {
	const int row = hrow[k] ;
	const double coeff = colels[k] ;
	k = link[k] ;
	dj -= rowduals[row]*coeff ;
      }
      if (!(fabs(rcosts[jcolx]-dj) < 100*ZTOLDP))
	printf("BAD DOUBLE X DJ:  %d %d %g %g\n",
	       irow,jcolx,rcosts[jcolx],dj) ;
      rcosts[jcolx] = dj ;
    }
    {
      CoinBigIndex k = mcstrt[jcoly] ;
      const int ny = hincol[jcoly] ;
      double dj = maxmin*dcost[jcoly] ;
      
      for (int kcol = 0 ; kcol < ny ; ++kcol) {
	const int row = hrow[k] ;
	const double coeff = colels[k] ;
	k = link[k] ;
	dj -= rowduals[row]*coeff ;
      }
      if (!(fabs(rcosts[jcoly]-dj) < 100*ZTOLDP))
	printf("BAD DOUBLE Y DJ:  %d %d %g %g\n",
	       irow,jcoly,rcosts[jcoly],dj) ;
      rcosts[jcoly] = dj ;
    }
#   endif
  }
/*
  Done at last. Delete the scratch arrays.
*/
  delete [] index1 ;
  delete [] element1 ;

# if PRESOLVE_DEBUG > 0 || PRESOLVE_CONSISTENCY > 0
  presolve_check_sol(prob,2,2,2) ;
  presolve_check_nbasic(prob) ;
  presolve_check_reduced_costs(prob) ;
# if PRESOLVE_DEBUG > 0
  std::cout << "Leaving doubleton_action::postsolve." << std::endl ;
# endif
# endif
}


doubleton_action::~doubleton_action()
{
  for (int i=nactions_-1; i>=0; i--) {
    delete[]actions_[i].colel ;
  }
  deleteAction(actions_,action*) ;
}


