/* $Id: CoinPresolveUseless.cpp 1566 2012-11-29 19:33:56Z lou $ */
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include <stdio.h>
#include <math.h>
#include "CoinPresolveMatrix.hpp"
#include "CoinPresolveUseless.hpp"
#include "CoinPresolveFixed.hpp"
#include "CoinHelperFunctions.hpp"
#include "CoinFinite.hpp"

#if PRESOLVE_DEBUG || PRESOLVE_CONSISTENCY
#include "CoinPresolvePsdebug.hpp"
#endif

/*
  This routine implements a greedy algorithm to find a set of necessary
  constraints which imply tighter bounds for some subset of the variables. It
  then uses the tighter column bounds to identify constraints that are
  useless as long as the necessary set is present.

  The algorithm is as follows:
    * Initially mark all constraints as in play.
    * For each in play constraint, calculate row activity bounds L(i) and U(i).
      Use these bounds in an attempt to tighten column bounds for all
      variables entangled with the row.
    * If some column bound is tightened, the row is necessary. Mark it as
      such, taking it out of play.
  Eventually the rows are divided into two sets, necessary and unnecessary. Go
  through the unnecessary rows and check L(i) and U(i) using the tightened
  column bounds. Remove any useless rows where row activity cannot exceed the
  row bounds.

  For efficiency, rows are marked as processed when L(i) and U(i) are first
  calculated and these values are not recalculated unless a column bound
  changes.
  
*/
const CoinPresolveAction *testRedundant (CoinPresolveMatrix *prob,
					 const CoinPresolveAction *next)
{
# if PRESOLVE_DEBUG > 0 || PRESOLVE_CONSISTENCY > 0
# if PRESOLVE_DEBUG > 0
  std::cout
    << "Entering testRedundant, considering " << prob->nrows_
    << " rows." << std::endl ;
# endif
  presolve_consistent(prob) ;
  presolve_links_ok(prob) ;
  presolve_check_sol(prob) ;
  presolve_check_nbasic(prob) ;
# endif

# if PRESOLVE_DEBUG > 0 || COIN_PRESOLVE_TUNING > 0
  const int startEmptyRows = prob->countEmptyRows() ;
# if COIN_PRESOLVE_TUNING > 0
  double startTime = 0.0 ;
  if (prob->tuning_) startTime = CoinCpuTime() ;
# endif
# endif

  const int m = prob->nrows_ ;
  const int n = prob->ncols_ ;
/*
  Unpack the row- and column-major representations.
*/
  const CoinBigIndex *rowStarts = prob->mrstrt_ ;
  const int *rowLengths = prob->hinrow_ ;
  const double *rowCoeffs = prob->rowels_ ;
  const int *colIndices = prob->hcol_ ;

  const CoinBigIndex *colStarts = prob->mcstrt_ ;
  const int *rowIndices = prob->hrow_ ;
  const int *colLengths = prob->hincol_ ;
/*
  Rim vectors.
  
  We need copies of the column bounds to modify as we do bound propagation.
*/
  const double *rlo = prob->rlo_ ;
  const double *rup = prob->rup_ ;

  double *columnLower = new double[n] ;
  double *columnUpper = new double[n] ;
  CoinMemcpyN(prob->clo_,n,columnLower) ;
  CoinMemcpyN(prob->cup_,n,columnUpper) ;
/*
  And we'll need somewhere to record the unnecessary rows.
*/
  int *useless_rows = prob->usefulRowInt_+m ;
  int nuseless_rows = 0 ;

/*
  The usual finite infinity.
*/
# define USE_SMALL_LARGE
# ifdef USE_SMALL_LARGE
  const double large = 1.0e15 ;
# else
  const double large = 1.0e20 ;
# endif
# ifndef NDEBUG
  const double large2 = 1.0e10*large ;
# endif

  double feasTol = prob->feasibilityTolerance_ ;
  double relaxedTol = 100.0*feasTol ;

/*
  More like `ignore infeasibility.'
*/
  bool fixInfeasibility = ((prob->presolveOptions_&0x4000) != 0) ;

/*
  Scan the rows and do an initial classification. MarkIgnore is used for
  constraints that are not in play in bound propagation for whatever reason,
  either because they're not interesting, or, later, because they're
  necessary.

  Interesting rows are nonempty with at least one finite row bound. The
  rest we can ignore during propagation. Nonempty rows with no finite row
  bounds are useless; record them for later.
*/
  const char markIgnore = 1 ;
  const char markInPlay = -1 ;
  const char markActOK = -2 ;
  char *markRow = reinterpret_cast<char *>(prob->usefulRowInt_) ;

  for (int i = 0 ; i < m ; i++) {
    if ((rlo[i] > -large || rup[i] < large) && rowLengths[i] > 0) {
      markRow[i] = markInPlay ;
    } else {
      markRow[i] = markIgnore ;
      if (rowLengths[i] > 0) {
	useless_rows[nuseless_rows++] = i ;
	prob->addRow(i) ;
      }
    }
  }

/*
  Open the main loop. We'll keep trying to tighten bounds until we get to
  diminishing returns. numberCheck is set at the end of the loop based on how
  well we did in the first pass. numberChanged tracks how well we do on the
  current pass.
*/

  int numberInfeasible = 0 ;
  int numberChanged = 0 ;
  int totalTightened = 0 ;
  int numberCheck = -1 ;

  const int MAXPASS = 10 ;
  int iPass = -1 ;
  while (iPass < MAXPASS && numberChanged > numberCheck) {
    iPass++ ;
    numberChanged = 0 ;
/*
  Open a loop to work through the rows computing upper and lower bounds
  on row activity. Each bound is calculated as a finite component and an
  infinite component.

  The state transitions for a row i go like this:
    * Row i starts out marked with markInPlay.
    * When row i is processed by the loop, L(i) and U(i) are calculated and
      we try to tighten the column bounds of entangled columns. If nothing
      happens, row i is marked with markActOK. It's still in play but
      won't be processed again unless the mark changes back to markInPlay.
    * If a column bound is tightened when we process row i, it's necessary.
      Mark row i with markIgnore and do not process it again.
      Other rows k entangled with the column and marked with markActOK are
      remarked to markInPlay and L(k) and U(k) will be recalculated on
      the next major pass.

  If the row is not marked as markInPlay, either we're ignoring the row or
  L(i) and U(i) have not changed since the last time the row was processed.
  There's nothing to gain by processing it again.
*/
    for (int i = 0 ; i < m ; i++) {

      if (markRow[i] != markInPlay) continue ;

      int infUpi = 0 ;
      int infLoi = 0 ;
      double finUpi = 0.0 ;
      double finDowni = 0.0 ;
      const CoinBigIndex krs = rowStarts[i] ;
      const CoinBigIndex kre = krs+rowLengths[i] ;
/*
  Open a loop to walk the row and calculate upper (U) and lower (L) bounds
  on row activity. Expand the finite component just a wee bit to make sure the
  bounds are conservative, then add in a whopping big value to account for the
  infinite component.
*/
      for (CoinBigIndex kcol = krs ; kcol < kre ; ++kcol) {
	const double value = rowCoeffs[kcol] ;
	const int j = colIndices[kcol] ;
	if (value > 0.0) {
	  if (columnUpper[j] < large) 
	    finUpi += columnUpper[j]*value ;
	  else
	    ++infUpi ;
	  if (columnLower[j] > -large) 
	    finDowni += columnLower[j]*value ;
	  else
	    ++infLoi ;
	} else if (value<0.0) {
	  if (columnUpper[j] < large) 
	    finDowni += columnUpper[j]*value ;
	  else
	    ++infLoi ;
	  if (columnLower[j] > -large) 
	    finUpi += columnLower[j]*value ;
	  else
	    ++infUpi ;
	}
      }
      markRow[i] = markActOK ;
      finUpi += 1.0e-8*fabs(finUpi) ;
      finDowni -= 1.0e-8*fabs(finDowni) ;
      const double maxUpi = finUpi+infUpi*1.0e31 ;
      const double maxDowni = finDowni-infLoi*1.0e31 ;
/*
  If LB(i) > rup(i) or UB(i) < rlo(i), we're infeasible. Break for the exit,
  unless the user has been so foolish as to tell us to ignore infeasibility,
  in which case just move on to the next row.
*/
      if (maxUpi < rlo[i]-relaxedTol || maxDowni > rup[i]+relaxedTol) {
	if (!fixInfeasibility) {
	  numberInfeasible++ ;
	  prob->messageHandler()->message(COIN_PRESOLVE_ROWINFEAS,
					  prob->messages())
	      << i << rlo[i] << rup[i] << CoinMessageEol ;
	  break ;
	} else {
	  continue ;
	}
      }
/*
  Remember why we're here --- this is presolve. If the constraint satisfies
  this condition, it already qualifies as useless and it's highly unlikely
  to produce tightened column bounds. But don't record it as useless just yet;
  leave that for phase 2.

  LOU: Well, why not mark it as useless? A possible optimisation.
*/
      if (maxUpi <= rup[i]+feasTol && maxDowni >= rlo[i]-feasTol) continue ;
/*
  We're not infeasible and one of U(i) or L(i) is not inside the row bounds.
  If we have something that looks like a forcing constraint but is just over
  the line, force the bound to exact equality to ensure conservative column
  bounds.
*/
      const double rloi = rlo[i] ;
      const double rupi = rup[i] ;
      if (finUpi < rloi && finUpi > rloi-relaxedTol)
	finUpi = rloi ;
      if (finDowni > rupi && finDowni < rupi+relaxedTol)
	finDowni = rupi ;
/*
  Open a loop to walk the row and try to tighten column bounds.
*/
      for (CoinBigIndex kcol = krs ; kcol < kre ; ++kcol) {
	const double ait = rowCoeffs[kcol] ;
	const int t = colIndices[kcol] ;
	double lt = columnLower[t] ;
	double ut = columnUpper[t] ;
	double newlt = COIN_DBL_MAX ;
	double newut = -COIN_DBL_MAX ;
/*
  For a target variable x(t) with a(it) > 0, we have

    new l(t) = (rlo(i)-(U(i)-a(it)u(t)))/a(it) = u(t) + (rlo(i)-U(i))/a(it)
    new u(t) = (rup(i)-(L(i)-a(it)l(t)))/a(it) = l(t) + (rup(i)-L(i))/a(it)

  Notice that if there's a single infinite contribution to L(i) or U(i) and it
  comes from x(t), then the finite portion finDowni or finUpi is correct and
  finite.

  Start by calculating new l(t) against rlo(i) and U(i). If the change is
  large, put some slack in the bound.
*/

	if (ait > 0.0) {
	  if (rloi > -large) {
	    if (!infUpi) {
	      assert(ut < large2) ;
	      newlt = ut+(rloi-finUpi)/ait ;
	      if (fabs(finUpi) > 1.0e8)
		newlt -= 1.0e-12*fabs(finUpi) ;
	    } else if (infUpi == 1 && ut >= large) {
	      newlt = (rloi-finUpi)/ ait ;
	      if (fabs(finUpi) > 1.0e8)
		newlt -= 1.0e-12*fabs(finUpi) ;
	    } else {
	      newlt = -COIN_DBL_MAX ;
	    }
/*
  Did we improve the bound? If we're infeasible, head for the exit. If we're
  still feasible, walk the column and reset the marks on the entangled rows
  so that the activity bounds will be recalculated. Mark this constraint
  so we don't use it again, and correct L(i).
*/
	    if (newlt > lt+1.0e-12 && newlt > -large) {
	      if (ut-newlt < -relaxedTol) {
		numberInfeasible++ ;
		break ;
	      }
	      columnLower[t] = newlt ;
	      markRow[i] = markIgnore ;
	      numberChanged++ ;
	      const CoinBigIndex kcs = colStarts[t] ;
	      const CoinBigIndex kce = kcs+colLengths[t] ;
	      for (CoinBigIndex kcol = kcs ; kcol < kce ; ++kcol) {
		const int k = rowIndices[kcol] ;
		if (markRow[k] == markActOK) {
		  markRow[k] = markInPlay ;
		}
	      }
	      if (lt <= -large) {
		finDowni += newlt*ait ;
		infLoi-- ;
	      } else {
		finDowni += (newlt-lt)*ait ;
	      }
	      lt = newlt ;
	    }
	  }
/*
  Perform the same actions, for new u(t) against rup(i) and L(i).
*/
	  if (rupi < large) {
	    if (!infLoi) {
	      assert(lt >- large2) ;
	      newut = lt+(rupi-finDowni)/ait ;
	      if (fabs(finDowni) > 1.0e8)
		newut += 1.0e-12*fabs(finDowni) ;
	    } else if (infLoi == 1 && lt <= -large) {
	      newut = (rupi-finDowni)/ait ;
	      if (fabs(finDowni) > 1.0e8)
		newut += 1.0e-12*fabs(finDowni) ;
	    } else {
	      newut = COIN_DBL_MAX ;
	    }
	    if (newut < ut-1.0e-12 && newut < large) {
	      columnUpper[t] = newut ;
	      if (newut-lt < -relaxedTol) {
		numberInfeasible++ ;
		break ;
	      }
	      markRow[i] = markIgnore ;
	      numberChanged++ ;
	      const CoinBigIndex kcs = colStarts[t] ;
	      const CoinBigIndex kce = kcs+colLengths[t] ;
	      for (CoinBigIndex kcol = kcs ; kcol < kce ; ++kcol) {
		const int k = rowIndices[kcol] ;
		if (markRow[k] == markActOK) {
		  markRow[k] = markInPlay ;
		}
	      }
	      if (ut >= large) {
		finUpi += newut*ait ;
		infUpi-- ;
	      } else {
		finUpi += (newut-ut)*ait ;
	      }
	      ut = newut ;
	    }
	  }
	} else {
/*
  And repeat both sets with the appropriate flips for a(it) < 0.
*/
	  if (rloi > -large) {
	    if (!infUpi) {
	      assert(lt < large2) ;
	      newut = lt+(rloi-finUpi)/ait ;
	      if (fabs(finUpi) > 1.0e8)
		newut += 1.0e-12*fabs(finUpi) ;
	    } else if (infUpi == 1 && lt <= -large) {
	      newut = (rloi-finUpi)/ait ;
	      if (fabs(finUpi) > 1.0e8)
		newut += 1.0e-12*fabs(finUpi) ;
	    } else {
	      newut = COIN_DBL_MAX ;
	    }
	    if (newut < ut-1.0e-12 && newut < large) {
	      columnUpper[t] = newut  ;
	      if (newut-lt < -relaxedTol) {
		numberInfeasible++ ;
		break ;
	      }
	      markRow[i] = markIgnore ;
	      numberChanged++ ;
	      const CoinBigIndex kcs = colStarts[t] ;
	      const CoinBigIndex kce = kcs+colLengths[t] ;
	      for (CoinBigIndex kcol = kcs ; kcol < kce ; ++kcol) {
		const int k = rowIndices[kcol] ;
		if (markRow[k] == markActOK) {
		  markRow[k] = markInPlay ;
		}
	      }
	      if (ut >= large) {
		finDowni += newut*ait ;
		infLoi-- ;
	      } else {
		finDowni += (newut-ut)*ait ;
	      }
	      ut = newut  ;
	    }
	  }
	  if (rupi < large) {
	    if (!infLoi) {
	      assert(ut < large2) ;
	      newlt = ut+(rupi-finDowni)/ait ;
	      if (fabs(finDowni) > 1.0e8)
		newlt -= 1.0e-12*fabs(finDowni) ;
	    } else if (infLoi == 1 && ut >= large) {
	      newlt = (rupi-finDowni)/ait ;
	      if (fabs(finDowni) > 1.0e8)
		newlt -= 1.0e-12*fabs(finDowni) ;
	    } else {
	      newlt = -COIN_DBL_MAX ;
	    }
	    if (newlt > lt+1.0e-12 && newlt > -large) {
	      columnLower[t] = newlt ;
	      if (ut-newlt < -relaxedTol) {
		numberInfeasible++ ;
		break ;
	      }
	      markRow[i] = markIgnore ;
	      numberChanged++ ;
	      const CoinBigIndex kcs = colStarts[t] ;
	      const CoinBigIndex kce = kcs+colLengths[t] ;
	      for (CoinBigIndex kcol = kcs ; kcol < kce ; ++kcol) {
		const int k = rowIndices[kcol] ;
		if (markRow[k] == markActOK) {
		  markRow[k] = markInPlay ;
		}
	      }
	      if (lt <= -large) {
		finUpi += newlt*ait ;
		infUpi-- ;
	      } else {
		finUpi += (newlt-lt)*ait ;
	      }
	      lt = newlt ;
	    }
	  }
	}
      }
    }

    totalTightened += numberChanged ;
    if (iPass == 1)
      numberCheck = CoinMax(10,numberChanged>>5) ;
    if (numberInfeasible) {
      prob->status_ = 1 ;
      break ;
    }
  }
/*
  At this point, we have rows marked with markIgnore, markInPlay, and
  markActOK. Rows marked with markIgnore may have been used to tighten the
  column bound on some variable, hence they are necessary. (Or they may
  have been marked in the initial scan, in which case they're already on
  the useless list or empty.)  Rows marked with markInPlay or markActOK
  are candidates for forcing or useless constraints.
*/
  if (!numberInfeasible) {
/*
  Open a loop to scan the rows again looking for useless constraints.
*/
    for (int i = 0 ; i < m ; i++) {
      
      if (markRow[i] == markIgnore) continue ;
/*
  Recalculate L(i) and U(i).

  LOU: Arguably this would not be necessary if we saved L(i) and U(i).
*/
      int infUpi = 0 ;
      int infLoi = 0 ;
      double finUpi = 0.0 ;
      double finDowni = 0.0 ;
      const CoinBigIndex krs = rowStarts[i] ;
      const CoinBigIndex kre = krs+rowLengths[i] ;

      for (CoinBigIndex krow = krs; krow < kre; ++krow) {
	const double value = rowCoeffs[krow] ;
	const int j = colIndices[krow] ;
	if (value > 0.0) {
	  if (columnUpper[j] < large) 
	    finUpi += columnUpper[j] * value ;
	  else
	    ++infUpi ;
	  if (columnLower[j] > -large) 
	    finDowni += columnLower[j] * value ;
	  else
	    ++infLoi ;
	} else if (value<0.0) {
	  if (columnUpper[j] < large) 
	    finDowni += columnUpper[j] * value ;
	  else
	    ++infLoi ;
	  if (columnLower[j] > -large) 
	    finUpi += columnLower[j] * value ;
	  else
	    ++infUpi ;
	}
      }
      finUpi += 1.0e-8*fabs(finUpi) ;
      finDowni -= 1.0e-8*fabs(finDowni) ;
      const double maxUpi = finUpi+infUpi*1.0e31 ;
      const double maxDowni = finDowni-infLoi*1.0e31 ;
/*
  If we have L(i) and U(i) at or inside the row bounds, we have a useless
  constraint.

  You would think we could detect forcing constraints here but that's a bit of
  a problem, because the tightened column bounds we're working with will not
  escape this routine.
*/
      if (maxUpi <= rup[i]+feasTol && maxDowni >= rlo[i]-feasTol) {
	useless_rows[nuseless_rows++] = i ;
      }
    }

    if (nuseless_rows) {
      next = useless_constraint_action::presolve(prob,
						 useless_rows,nuseless_rows,
						 next) ;
    }
/*
  See if we can transfer tightened bounds from the work above. This is
  problematic. When we loosen these bounds in postsolve it's entirely possible
  that the variable will end up superbasic.

  Original comment: may not unroll
*/
    if (prob->presolveOptions_&0x10) {
      const unsigned char *integerType = prob->integerType_ ;
      double *csol  = prob->sol_ ;
      double *clo = prob->clo_ ;
      double *cup = prob->cup_ ;
      int *fixed = prob->usefulColumnInt_ ;
      int nFixed = 0 ;
      int nChanged = 0 ;
      for (int j = 0 ; j < n ; j++) {
	if (clo[j] == cup[j])
	  continue ;
	double lower = columnLower[j] ;
	double upper = columnUpper[j] ;
	if (integerType[j]) {
	  upper = floor(upper+1.0e-4) ;
	  lower = ceil(lower-1.0e-4) ;
	}
	if (upper-lower < 1.0e-8) {
	  if (upper-lower < -feasTol)
	    numberInfeasible++ ;
	  if (CoinMin(fabs(upper),fabs(lower)) <= 1.0e-7) 
	    upper = 0.0 ;
	  fixed[nFixed++] = j ;
	  prob->addCol(j) ;
	  cup[j] = upper ;
	  clo[j] = upper ;
	  if (csol != 0) 
	    csol[j] = upper ;
	} else {
	  if (integerType[j]) {
	    if (upper < cup[j]) {
	      cup[j] = upper ;
	      nChanged++ ;
	      prob->addCol(j) ;
	    }
	    if (lower > clo[j]) {
	      clo[j] = lower ;
	      nChanged++ ;
	      prob->addCol(j) ;
	    }
	  }
	}
      }
#     ifdef CLP_INVESTIGATE
      if (nFixed||nChanged)
	printf("%d fixed in impliedfree, %d changed\n",nFixed,nChanged) ;
#     endif
      if (nFixed)
	next = remove_fixed_action::presolve(prob,fixed,nFixed,next) ; 
    }
  }

  delete [] columnLower ;
  delete [] columnUpper ;

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
  std::cout << "Leaving testRedundant, " << droppedRows << " rows dropped" ;
# if COIN_PRESOLVE_TUNING > 0
  std::cout << " in " << thisTime-startTime << "s" ;
# endif
  std::cout << "." << std::endl ;
# endif

  return (next) ;
}






// WHAT HAPPENS IF COLS ARE DROPPED AS A RESULT??
// should be like do_tighten.
// not really - one could fix costed variables to appropriate bound.
// ok, don't bother about it.  If it is costed, it will be checked
// when it is eliminated as an empty col; if it is costed in the
// wrong direction, the problem is unbounded, otherwise it is pegged
// at its bound.  no special action need be taken here.
const CoinPresolveAction *useless_constraint_action::presolve(CoinPresolveMatrix * prob,
								  const int *useless_rows,
								  int nuseless_rows,
				       const CoinPresolveAction *next)
{
# if PRESOLVE_DEBUG > 0 || PRESOLVE_CONSISTENCY > 0
# if PRESOLVE_DEBUG > 0
  std::cout
    << "Entering useless_constraint_action::presolve, "
    << nuseless_rows << " rows." << std::endl ;
# endif
  presolve_check_sol(prob) ;
  presolve_check_nbasic(prob) ;
# endif

  // may be modified by useless constraint
  double *colels	= prob->colels_;

  // may be modified by useless constraint
        int *hrow	= prob->hrow_;

  const CoinBigIndex *mcstrt	= prob->mcstrt_;

  // may be modified by useless constraint
        int *hincol	= prob->hincol_;

	//  double *clo	= prob->clo_;
	//  double *cup	= prob->cup_;

  const double *rowels	= prob->rowels_;
  const int *hcol	= prob->hcol_;
  const CoinBigIndex *mrstrt	= prob->mrstrt_;

  // may be written by useless constraint
        int *hinrow	= prob->hinrow_;
	//  const int nrows	= prob->nrows_;

  double *rlo	= prob->rlo_;
  double *rup	= prob->rup_;

  action *actions	= new action [nuseless_rows];

  for (int i=0; i<nuseless_rows; ++i) {
    int irow = useless_rows[i];
#   if PRESOLVE_DEBUG > 2
    std::cout << "  removing row " << irow << std::endl ;
#   endif
    CoinBigIndex krs = mrstrt[irow];
    CoinBigIndex kre = krs + hinrow[irow];
    PRESOLVE_DETAIL_PRINT(printf("pre_useless %dR E\n",irow));

    action *f = &actions[i];

    f->row = irow;
    f->ninrow = hinrow[irow];
    f->rlo = rlo[irow];
    f->rup = rup[irow];
    f->rowcols = CoinCopyOfArray(&hcol[krs], hinrow[irow]);
    f->rowels  = CoinCopyOfArray(&rowels[krs], hinrow[irow]);

    for (CoinBigIndex k=krs; k<kre; k++)
    { presolve_delete_from_col(irow,hcol[k],mcstrt,hincol,hrow,colels) ;
      if (hincol[hcol[k]] == 0)
      { PRESOLVE_REMOVE_LINK(prob->clink_,hcol[k]) ; } }
    hinrow[irow] = 0;
    PRESOLVE_REMOVE_LINK(prob->rlink_,irow) ;

    // just to make things squeeky
    rlo[irow] = 0.0;
    rup[irow] = 0.0;
  }

  next = new useless_constraint_action(nuseless_rows,actions,next) ;

# if PRESOLVE_DEBUG > 0 || PRESOLVE_CONSISTENCY > 0
  presolve_check_sol(prob) ;
  presolve_check_nbasic(prob) ;
# if PRESOLVE_DEBUG > 0
  std::cout << "Leaving useless_constraint_action::presolve." << std::endl ;
# endif
# endif

  return (next) ;
}

// Put constructors here
useless_constraint_action::useless_constraint_action(int nactions,
                                                     const action *actions,
                                                     const CoinPresolveAction *next) 
  :   CoinPresolveAction(next),
      nactions_(nactions),
      actions_(actions)
{}
useless_constraint_action::~useless_constraint_action() 
{
  for (int i=0;i<nactions_;i++) {
    deleteAction(actions_[i].rowcols, int *);
    deleteAction(actions_[i].rowels, double *);
  }
  deleteAction(actions_, action *);
}

const char *useless_constraint_action::name() const
{
  return ("useless_constraint_action");
}

void useless_constraint_action::postsolve(CoinPostsolveMatrix *prob) const
{
  const action *const actions = actions_;
  const int nactions = nactions_;

# if PRESOLVE_DEBUG > 0 || PRESOLVE_CONSISTENCY > 0
# if PRESOLVE_DEBUG > 0
  std::cout
    << "Entering useless_constraint_action::postsolve, "
    << nactions << " rows." << std::endl ;
# endif
  presolve_check_sol(prob) ;
  presolve_check_nbasic(prob) ;
# endif

  double *colels	= prob->colels_;
  int *hrow		= prob->hrow_;
  CoinBigIndex *mcstrt		= prob->mcstrt_;
  int *link		= prob->link_;
  int *hincol		= prob->hincol_;
  
  //  double *rowduals	= prob->rowduals_;
  double *rowacts	= prob->acts_;
  const double *sol	= prob->sol_;


  CoinBigIndex &free_list		= prob->free_list_;

  double *rlo	= prob->rlo_;
  double *rup	= prob->rup_;

  for (const action *f = &actions[nactions-1]; actions<=f; f--) {

    int irow	= f->row;
    int ninrow	= f->ninrow;
    const int *rowcols	= f->rowcols;
    const double *rowels = f->rowels;
    double rowact = 0.0;

    rup[irow] = f->rup;
    rlo[irow] = f->rlo;

    for (CoinBigIndex k=0; k<ninrow; k++) {
      int jcol = rowcols[k];
      //      CoinBigIndex kk = mcstrt[jcol];

      // append deleted row element to each col
      {
	CoinBigIndex kk = free_list;
	assert(kk >= 0 && kk < prob->bulk0_) ;
	free_list = link[free_list];
	hrow[kk] = irow;
	colels[kk] = rowels[k];
	link[kk] = mcstrt[jcol];
	mcstrt[jcol] = kk;
      }
      
      rowact += rowels[k] * sol[jcol];
      hincol[jcol]++;
    }
#   if PRESOLVE_CONSISTENCY
    presolve_check_free_list(prob) ;
#   endif
    
    // I don't know if this is always true
    PRESOLVEASSERT(prob->getRowStatus(irow)==CoinPrePostsolveMatrix::basic);
    // rcosts are unaffected since rowdual is 0

    rowacts[irow] = rowact;
    // leave until desctructor
    //deleteAction(rowcols,int *);
    //deleteAction(rowels,double *);
  }

  //deleteAction(actions_,action *);

# if PRESOLVE_DEBUG > 0 || PRESOLVE_CONSISTENCY > 0
  presolve_check_threads(prob) ;
  presolve_check_sol(prob) ;
  presolve_check_nbasic(prob) ;
# if PRESOLVE_DEBUG > 0
  std::cout << "Leaving useless_constraint_action::postsolve." << std::endl ;
# endif
# endif

}
