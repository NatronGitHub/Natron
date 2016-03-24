/* $Id: CoinPresolveForcing.cpp 1581 2013-04-06 12:48:50Z stefan $ */
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include <stdio.h>
#include <math.h>

#include "CoinPresolveMatrix.hpp"
#include "CoinPresolveEmpty.hpp"	// for DROP_COL/DROP_ROW
#include "CoinPresolveFixed.hpp"
#include "CoinPresolveSubst.hpp"
#include "CoinHelperFunctions.hpp"
#include "CoinPresolveUseless.hpp"
#include "CoinPresolveForcing.hpp"
#include "CoinMessage.hpp"
#include "CoinFinite.hpp"

#if PRESOLVE_DEBUG > 0 || PRESOLVE_CONSISTENCY > 0
#include "CoinPresolvePsdebug.hpp"
#endif


namespace {

/*
  Calculate the minimum and maximum row activity (also referred to as lhs
  bounds, from the common form ax <= b) for the row specified by the range
  krs, kre in els and hcol. Return the result in maxupp, maxdnp.
*/
void implied_row_bounds (const double *els,
			 const double *clo, const double *cup,
			 const int *hcol, CoinBigIndex krs, CoinBigIndex kre,
			 double &maxupp, double &maxdnp)
{
  bool posinf = false ;
  bool neginf = false ;
  double maxup = 0.0 ;
  double maxdown = 0.0 ;

/*
  Walk the row and add up the upper and lower bounds on the variables. Once
  both maxup and maxdown have an infinity contribution the row cannot be shown
  to be useless or forcing, so break as soon as that's detected.
*/
  for (CoinBigIndex kk = krs ; kk < kre ; kk++) {

    const int col = hcol[kk] ;
    const double coeff = els[kk] ;
    const double lb = clo[col] ;
    const double ub = cup[col] ;

    if (coeff > 0.0) {
      if (PRESOLVE_INF <= ub) {
	posinf = true ;
	if (neginf) break ;
      } else {
	maxup += ub*coeff ;
      }
      if (lb <= -PRESOLVE_INF) {
	neginf = true ;
	if (posinf) break ;
      } else {
	maxdown += lb*coeff ;
      }
    } else {
      if (PRESOLVE_INF <= ub) {
	neginf = true ;
	if (posinf) break ;
      } else {
	maxdown += ub*coeff ;
      }
      if (lb <= -PRESOLVE_INF) {
	posinf = true ;
	if (neginf) break ;
      } else {
	maxup += lb*coeff ;
      }
    }
  }

  maxupp   = (posinf)?PRESOLVE_INF:maxup ;
  maxdnp = (neginf)?-PRESOLVE_INF:maxdown ;
}

}	// end file-local namespace



const char *forcing_constraint_action::name() const
{
  return ("forcing_constraint_action") ;
}

/*
  It may be the case that the bounds on the variables in a constraint are
  such that no matter what feasible value the variables take, the constraint
  cannot be violated. In this case we can drop the constraint as useless.

  On the other hand, it may be that the only way to satisfy a constraint
  is to jam all the variables in the constraint to one of their bounds, fixing
  the variables. This is a forcing constraint, the primary target of this
  transform.

  Detection of both useless and forcing constraints requires calculation of
  bounds on the row activity (often referred to as lhs bounds, from the common
  form ax <= b). This routine will remember useless constraints as it finds
  them and invoke useless_constraint_action to deal with them.
  
  The transform applied here simply tightens the bounds on the variables.
  Other transforms will remove the fixed variables, leaving an empty row which
  is ultimately dropped.

  A reasonable question to ask is ``If a variable is already fixed, why do
  we need a record in the postsolve object?'' The answer is that in postsolve
  we'll be dealing with a column-major representation and we may need to scan
  the row (see postsolve comments). So it's useful to record all variables in
  the constraint.
  
  On the other hand, it's definitely harmful to ask remove_fixed_action
  to process a variable more than once (causes problems in
  remove_fixed_action::postsolve).

  Original comments:

  It looks like these checks could be performed in parallel, that is,
  the tests could be carried out for all rows in parallel, and then the
  rows deleted and columns tightened afterward.  Obviously, this is true
  for useless rows.  By doing it in parallel rather than sequentially,
  we may miss transformations due to variables that were fixed by forcing
  constraints, though.

  Note that both of these operations will cause problems if the variables
  in question really need to exceed their bounds in order to make the
  problem feasible.
*/
const CoinPresolveAction*
  forcing_constraint_action::presolve (CoinPresolveMatrix *prob,
  				       const CoinPresolveAction *next)
{
# if PRESOLVE_DEBUG > 0 || COIN_PRESOLVE_TUNING
  int startEmptyRows = 0 ;
  int startEmptyColumns = 0 ;
  startEmptyRows = prob->countEmptyRows() ;
  startEmptyColumns = prob->countEmptyCols() ;
# endif
# if PRESOLVE_DEBUG > 0 || PRESOLVE_CONSISTENCY > 0
# if PRESOLVE_DEBUG > 0
  std::cout
    << "Entering forcing_constraint_action::presolve, considering "
    << prob->numberRowsToDo_ << " rows." << std::endl ;
# endif
  presolve_check_sol(prob) ;
  presolve_check_nbasic(prob) ;
# endif
# if COIN_PRESOLVE_TUNING
  double startTime = 0.0 ;
  if (prob->tuning_) {
    startTime = CoinCpuTime() ;
  }
# endif

  // Column solution and bounds
  double *clo = prob->clo_ ;
  double *cup = prob->cup_ ;
  double *csol = prob->sol_ ;

  // Row-major representation
  const CoinBigIndex *mrstrt = prob->mrstrt_ ;
  const double *rowels = prob->rowels_ ;
  const int *hcol = prob->hcol_ ;
  const int *hinrow = prob->hinrow_ ;
  const int nrows = prob->nrows_ ;

  const double *rlo = prob->rlo_ ;
  const double *rup = prob->rup_ ;

  const double tol = ZTOLDP ;
  const double inftol = prob->feasibilityTolerance_ ;
  const int ncols = prob->ncols_ ;

  int *fixed_cols = new int[ncols] ;
  int nfixed_cols = 0 ;

  action *actions = new action [nrows] ;
  int nactions = 0 ;

  int *useless_rows = new int[nrows] ;
  int nuseless_rows = 0 ;

  const int numberLook = prob->numberRowsToDo_ ;
  int *look = prob->rowsToDo_ ;

  bool fixInfeasibility = ((prob->presolveOptions_&0x4000) != 0) ;
/*
  Open a loop to scan the constraints of interest. There must be variables
  left in the row.
*/
  for (int iLook = 0 ; iLook < numberLook ; iLook++) {
    int irow = look[iLook] ;
    if (hinrow[irow] <= 0) continue ;

    const CoinBigIndex krs = mrstrt[irow] ;
    const CoinBigIndex kre = krs+hinrow[irow] ;
/*
  Calculate upper and lower bounds on the row activity based on upper and lower
  bounds on the variables. If these are finite and incompatible with the given
  row bounds, we have infeasibility.
*/
    double maxup, maxdown ;
    implied_row_bounds(rowels,clo,cup,hcol,krs,kre,maxup,maxdown) ;
#   if PRESOLVE_DEBUG > 2
    std::cout
      << "  considering row " << irow << ", rlo " << rlo[irow]
      << " LB " << maxdown << " UB " << maxup << " rup " << rup[irow] ;
#   endif
/*
  If the maximum lhs value is less than L(i) or the minimum lhs value is
  greater than U(i), we're infeasible.
*/
    if (maxup < PRESOLVE_INF &&
        maxup+inftol < rlo[irow] && !fixInfeasibility) {
      CoinMessageHandler *hdlr = prob->messageHandler() ;
      prob->status_|= 1 ;
      hdlr->message(COIN_PRESOLVE_ROWINFEAS,prob->messages())
	 << irow << rlo[irow] << rup[irow] << CoinMessageEol ;
#     if PRESOLVE_DEBUG > 2
      std::cout << "; infeasible." << std::endl ;
#     endif
      break ;
    }
    if (-PRESOLVE_INF < maxdown &&
        rup[irow] < maxdown-inftol && !fixInfeasibility) {
      CoinMessageHandler *hdlr = prob->messageHandler() ;
      prob->status_|= 1 ;
      hdlr->message(COIN_PRESOLVE_ROWINFEAS,prob->messages())
	 << irow << rlo[irow] << rup[irow] << CoinMessageEol ;
#     if PRESOLVE_DEBUG > 2
      std::cout << "; infeasible." << std::endl ;
#     endif
      break ;
    }
/*
  We've dealt with prima facie infeasibility. Now check if the constraint
  is trivially satisfied. If so, add it to the list of useless rows and move
  on.

  The reason we require maxdown and maxup to be finite if the row bound is
  finite is to guard against some subsequent transform changing a column
  bound from infinite to finite. Once finite, bounds continue to tighten,
  so we're safe.
*/
    if (((rlo[irow] <= -PRESOLVE_INF) ||
	 (-PRESOLVE_INF < maxdown && rlo[irow] <= maxdown-inftol)) &&
	((rup[irow] >= PRESOLVE_INF) ||
	 (maxup < PRESOLVE_INF && rup[irow] >= maxup+inftol))) {
      // check none prohibited
      if (prob->anyProhibited_) {
	bool anyProhibited=false;
	for (int k=krs; k<kre; k++) {
	  int jcol = hcol[k];
	  if (prob->colProhibited(jcol)) {
	    anyProhibited=true;
	    break;
	  }
	}
	if (anyProhibited)
	  continue; // skip row
      }
      useless_rows[nuseless_rows++] = irow ;
#     if PRESOLVE_DEBUG > 2
      std::cout << "; useless." << std::endl ;
#     endif
      continue ;
    }
/*
  Is it the case that we can just barely attain L(i) or U(i)? If so, we have a
  forcing constraint. As explained above, we need maxup and maxdown to be
  finite in order for the test to be valid.
*/
    const bool tightAtLower = ((maxup < PRESOLVE_INF) &&
    			       (fabs(rlo[irow]-maxup) < tol)) ;
    const bool tightAtUpper = ((-PRESOLVE_INF < maxdown) &&
			       (fabs(rup[irow]-maxdown) < tol)) ;
#   if PRESOLVE_DEBUG > 2
    if (tightAtLower || tightAtUpper) std::cout << "; forcing." ;
    std::cout << std::endl ;
#   endif
    if (!(tightAtLower || tightAtUpper)) continue ;
    // check none prohibited
    if (prob->anyProhibited_) {
      bool anyProhibited=false;
      for (int k=krs; k<kre; k++) {
	int jcol = hcol[k];
	if (prob->colProhibited(jcol)) {
	  anyProhibited=true;
	  break;
	}
      }
      if (anyProhibited)
	continue; // skip row
    }
/*
  We have a forcing constraint.
  Get down to the business of fixing the variables at the appropriate bound.
  We need to remember the original value of the bound we're tightening.
  Allocate a pair of arrays the size of the row. Load variables fixed at l<j>
  from the start, variables fixed at u<j> from the end. Add the column to
  the list of columns to be processed further.
*/
    double *bounds = new double[hinrow[irow]] ;
    int *rowcols = new int[hinrow[irow]] ;
    CoinBigIndex lk = krs ;
    CoinBigIndex uk = kre ;
    for (CoinBigIndex k = krs ; k < kre ; k++) {
      const int j = hcol[k] ;
      const double lj = clo[j] ;
      const double uj = cup[j] ;
      const double coeff = rowels[k] ;
      PRESOLVEASSERT(fabs(coeff) > ZTOLDP) ;
/*
  If maxup is tight at L(i), then we want to force variables x<j> to the bound
  that produced maxup: u<j> if a<ij> > 0, l<j> if a<ij> < 0. If maxdown is
  tight at U(i), it'll be just the opposite.
*/
      if (tightAtLower == (coeff > 0.0)) {
	--uk ;
	bounds[uk-krs] = lj ;
	rowcols[uk-krs] = j ;
	if (csol != 0) {
	  csol[j] = uj ;
	}
	clo[j] = uj ;
      } else {
	bounds[lk-krs] = uj ;
	rowcols[lk-krs] = j ;
	++lk ;
	if (csol != 0) {
	  csol[j] = lj ;
	}
	cup[j] = lj ;
      }
/*
  Only add a column to the list of fixed columns the first time it's fixed.
*/
      if (lj != uj) {
	fixed_cols[nfixed_cols++] = j ;
	prob->addCol(j) ;
      }
    }
    PRESOLVEASSERT(uk == lk) ;
    PRESOLVE_DETAIL_PRINT(printf("pre_forcing %dR E\n",irow)) ;
#   if PRESOLVE_DEBUG > 1
    std::cout
      << "FORCING: row(" << irow << "), " << (kre-krs) << " variables."
      << std::endl ;
#   endif
/*
  Done with this row. Remember the changes in a postsolve action.
*/
    action *f = &actions[nactions] ;
    nactions++ ;
    f->row = irow ;
    f->nlo = lk-krs ;
    f->nup = kre-uk ;
    f->rowcols = rowcols ;
    f->bounds = bounds ;
  }

/*
  Done processing the rows of interest.  No sense doing any additional work
  unless we're feasible.
*/
  if (prob->status_ == 0) {
#   if PRESOLVE_DEBUG > 0
    std::cout
      << "FORCING: " << nactions << " forcing, " << nuseless_rows << " useless."
      << std::endl ;
#   endif
/*
  Trim the actions array to size and create a postsolve object.
*/
    if (nactions) {
      next = new forcing_constraint_action(nactions, 
				 CoinCopyOfArray(actions,nactions),next) ;
    }
/*
  Hand off the job of dealing with the useless rows to a specialist.
*/
    if (nuseless_rows) {
      next = useless_constraint_action::presolve(prob,
      					useless_rows,nuseless_rows,next) ;
    }
/*
  Hand off the job of dealing with the fixed columns to a specialist.

  Note that there *cannot* be duplicates in this list or we'll get in trouble
  `unfixing' a column multiple times. The code above now adds a variable
  to fixed_cols only if it's not already fixed. If that ever changes,
  the disabled code (sort, unique) will need to be reenabled.
*/
    if (nfixed_cols) {
      if (false && nfixed_cols > 1) {
	std::sort(fixed_cols,fixed_cols+nfixed_cols) ;
	int *end = std::unique(fixed_cols,fixed_cols+nfixed_cols) ;
	nfixed_cols = static_cast<int>(end-fixed_cols) ;
      }
      next = remove_fixed_action::presolve(prob,fixed_cols,nfixed_cols,next) ;
    }
  }

  deleteAction(actions,action*) ;
  delete [] useless_rows ;
  delete [] fixed_cols ;

# if COIN_PRESOLVE_TUNING
  if (prob->tuning_) double thisTime = CoinCpuTime() ;
# endif
# if PRESOLVE_DEBUG > 0 || PRESOLVE_CONSISTENCY > 0
  presolve_check_sol(prob) ;
  presolve_check_nbasic(prob) ;
# endif
# if PRESOLVE_DEBUG > 0 || COIN_PRESOLVE_TUNING
  int droppedRows = prob->countEmptyRows()-startEmptyRows ;
  int droppedColumns =  prob->countEmptyCols()-startEmptyColumns ;
  std::cout
    << "Leaving forcing_constraint_action::presolve: removed " << droppedRows
    << " rows, " << droppedColumns << " columns" ;
# if COIN_PRESOLVE_TUNING > 0
  if (prob->tuning_)
    std::cout << " in " << (thisTime-prob->startTime_) << "s" ;
# endif
  std::cout << "." << std::endl ;
# endif

  return (next) ;
}

/*
  We're here to undo the bound changes that were put in place for forcing
  constraints.  This is a bit trickier than it appears.

  Assume we are working with constraint r.  The situation on arrival is
  that constraint r exists and is fully populated with fixed variables, all
  of which are nonbasic. Even though the constraint is tight, the logical
  s(r) is basic and the dual y(r) is zero.
  
  We may need to change that if a bound is relaxed to infinity on some
  variable x(t), making x(t)'s current nonbasic status untenable. We'll need
  to make s(r) nonbasic so that y(r) can be nonzero. Then we can make x(t)
  basic and use y(r) to force cbar(t) to zero. The code below will choose
  the variable x(t) whose reduced cost cbar(t) is most wrong and adjust y(r)
  to drive cbar(t) to zero using
     cbar(t) = c(t) - SUM{i\r} y(i) a(it) - y(r)a(rt)
     cbar(t) = cbar(t\r) - y(r)a(rt)
  Setting cbar(t) to zero,
     y(r) = cbar(t\r)/a(rt)

  We will need to scan row r, correcting cbar(j) for all x(j) entangled
  with the row. We may need to change the nonbasic status of x(j) if the
  adjustment causes cbar(j) to change sign.
*/
void forcing_constraint_action::postsolve(CoinPostsolveMatrix *prob) const
{
  const action *const actions = actions_ ;
  const int nactions = nactions_ ;

  const double *colels = prob->colels_ ;
  const int *hrow = prob->hrow_ ;
  const CoinBigIndex *mcstrt = prob->mcstrt_ ;
  const int *hincol = prob->hincol_ ;
  const int *link = prob->link_ ;

  double *clo = prob->clo_ ;
  double *cup = prob->cup_ ;
  double *rlo = prob->rlo_ ;
  double *rup = prob->rup_ ;

  double *rcosts = prob->rcosts_ ;

  double *acts = prob->acts_ ;
  double *rowduals = prob->rowduals_ ;

  const double ztoldj = prob->ztoldj_ ;
  const double ztolzb = prob->ztolzb_ ;

# if PRESOLVE_DEBUG > 0 || PRESOLVE_CONSISTENCY > 0
  const double *sol = prob->sol_ ;
# if PRESOLVE_DEBUG > 0
  std::cout
    << "Entering forcing_constraint_action::postsolve, "
    << nactions << " constraints to process." << std::endl ;
# endif
  presolve_check_threads(prob) ;
  presolve_check_free_list(prob) ;
  presolve_check_sol(prob,2,2,2) ;
  presolve_check_nbasic(prob) ;
# endif
/*
  Open a loop to process the actions. One action per constraint.
*/
  for (const action *f = &actions[nactions-1] ; actions <= f ; f--) {
    const int irow = f->row ;
    const int nlo = f->nlo ;
    const int nup = f->nup ;
    const int ninrow = nlo+nup ;
    const int *rowcols = f->rowcols ;
    const double *bounds = f->bounds ;

#   if PRESOLVE_DEBUG > 1
    std::cout
      << "  Restoring constraint " << irow << ", " << ninrow
      << " variables." << std::endl ;
#   endif

    PRESOLVEASSERT(prob->getRowStatus(irow) == CoinPrePostsolveMatrix::basic) ;
    PRESOLVEASSERT(rowduals[irow] == 0.0) ;
/*
  Process variables where the upper bound is relaxed.
    * If the variable is basic, we should leave the status unchanged. Relaxing
      the bound cannot make nonbasic status feasible.
    * The bound change may be a noop, in which nothing needs to be done.
    * Otherwise, the status should be set to NBLB.
*/
    bool dualfeas = true ;
    for (int k = 0 ; k < nlo ; k++) {
      const int jcol = rowcols[k] ;
      PRESOLVEASSERT(fabs(sol[jcol]-clo[jcol]) <= ztolzb) ;
      const double cbarj = rcosts[jcol] ;
      const double olduj = cup[jcol] ;
      const double newuj = bounds[k] ;
      const bool change = (fabs(newuj-olduj) > ztolzb) ;

#     if PRESOLVE_DEBUG > 2
      std::cout
        << "    x(" << jcol << ") " << prob->columnStatusString(jcol)
	<< " cbar = " << cbarj << ", lb = " << clo[jcol]
	<< ", ub = " << olduj << " -> " << newuj ;
#     endif

      if (change &&
          prob->getColumnStatus(jcol) != CoinPrePostsolveMatrix::basic) {
	prob->setColumnStatus(jcol,CoinPrePostsolveMatrix::atLowerBound) ;
	if (cbarj < -ztoldj || clo[jcol] <= -COIN_DBL_MAX) dualfeas = false ;
      }
      cup[jcol] = bounds[k] ;

#     if PRESOLVE_DEBUG > 2
      std::cout
        << " -> " << prob->columnStatusString(jcol) << "." << std::endl ;
#     endif
    }
/*
  Process variables where the lower bound is relaxed. The comments above
  apply.
*/
    for (int k = nlo ; k < ninrow ; k++) {
      const int jcol = rowcols[k] ;
      PRESOLVEASSERT(fabs(sol[jcol]-cup[jcol]) <= ztolzb) ;
      const double cbarj = rcosts[jcol] ;
      const double oldlj = clo[jcol] ;
      const double newlj = bounds[k] ;
      const bool change = (fabs(newlj-oldlj) > ztolzb) ;

#     if PRESOLVE_DEBUG > 2
      std::cout
        << "    x(" << jcol << ") " << prob->columnStatusString(jcol)
	<< " cbar = " << cbarj << ", ub = " << cup[jcol]
	<< ", lb = " << oldlj << " -> " << newlj ;
#     endif

      if (change &&
          prob->getColumnStatus(jcol) != CoinPrePostsolveMatrix::basic) {
	prob->setColumnStatus(jcol,CoinPrePostsolveMatrix::atUpperBound) ;
	if (cbarj > ztoldj || cup[jcol] >= COIN_DBL_MAX) dualfeas = false ;
      }
      clo[jcol] = bounds[k] ;

#     if PRESOLVE_DEBUG > 2
      std::cout
        << " -> " << prob->columnStatusString(jcol) << "." << std::endl ;
#     endif
    }
/*
  The reduced costs and status for the columns may or may not be ok for
  the relaxed column bounds.  If not, find the variable x<joow> most
  out-of-whack with respect to reduced cost and calculate the value of
  y<irow> required to reduce cbar<joow> to zero.
*/

    if (dualfeas == false) {
      int joow = -1 ;
      double yi = 0.0 ;
      for (int k = 0 ; k < ninrow ; k++) {
	int jcol = rowcols[k] ;
	CoinBigIndex kk = presolve_find_row2(irow,mcstrt[jcol],
					     hincol[jcol],hrow,link) ;
	const double &cbarj = rcosts[jcol] ;
	const CoinPrePostsolveMatrix::Status statj =
					prob->getColumnStatus(jcol) ;
	if ((cbarj < -ztoldj &&
	     statj != CoinPrePostsolveMatrix::atUpperBound) ||
	    (cbarj > ztoldj &&
	     statj != CoinPrePostsolveMatrix::atLowerBound)) {
	  double yi_j = cbarj/colels[kk] ;
	  if (fabs(yi_j) > fabs(yi)) {
	    joow = jcol ;
	    yi = yi_j ;
	  }
#         if PRESOLVE_DEBUG > 3
	  std::cout
	    << "      oow: x(" << jcol << ") "
	    << prob->columnStatusString(jcol) << " cbar " << cbarj << " aij "
	    << colels[kk] << " corr " << yi_j << "." << std::endl ;
#         endif
	}
      }
      assert(joow != -1) ;
/*
  Make x<joow> basic and set the row status according to whether we're
  tight at the lower or upper bound. Keep in mind the convention that a
  <= constraint has a slack 0 <= s <= infty, while a >= constraint has a
  surplus -infty <= s <= 0.
*/

#     if PRESOLVE_DEBUG > 1
      std::cout
	<< "    Adjusting row dual; x(" << joow
	<< ") " << prob->columnStatusString(joow) << " -> "
	<< statusName(CoinPrePostsolveMatrix::basic)
	<< ", y = 0.0 -> " << yi << "." << std::endl ;
#     endif

      prob->setColumnStatus(joow,CoinPrePostsolveMatrix::basic) ;
      if (acts[irow]-rlo[irow] < rup[irow]-acts[irow])
	prob->setRowStatus(irow,CoinPrePostsolveMatrix::atUpperBound) ;
      else
	prob->setRowStatus(irow,CoinPrePostsolveMatrix::atLowerBound) ;
      rowduals[irow] = yi ;

#     if PRESOLVE_DEBUG > 1
      std::cout
	<< "    Row status " << prob->rowStatusString(irow)
	<< ", lb = " << rlo[irow] << ", ax = " << acts[irow]
	<< ", ub = " << rup[irow] << "." << std::endl ;
#     endif
/*
  Now correct the reduced costs for other variables in the row. This may
  cause a reduced cost to change sign, in which case we need to change status.

  The code implicitly assumes that if it's necessary to change the status
  of a variable because the reduced cost has changed sign, then it will be
  possible to do it. I'm not sure I could prove that, however.
  -- lh, 121108 --
*/
      for (int k = 0 ; k < ninrow ; k++) {
	int jcol = rowcols[k] ;
	CoinBigIndex kk = presolve_find_row2(irow,mcstrt[jcol],
					     hincol[jcol],hrow,link) ;
	const double old_cbarj = rcosts[jcol] ;
	rcosts[jcol] -= yi*colels[kk] ;
	const double new_cbarj = rcosts[jcol] ;

	if ((old_cbarj < 0) != (new_cbarj < 0)) {
	  if (new_cbarj < -ztoldj && cup[jcol] < COIN_DBL_MAX)
	    prob->setColumnStatus(jcol,CoinPrePostsolveMatrix::atUpperBound) ;
	  else if (new_cbarj > ztoldj && clo[jcol] > -COIN_DBL_MAX)
	    prob->setColumnStatus(jcol,CoinPrePostsolveMatrix::atLowerBound) ;
	}

#       if PRESOLVE_DEBUG > 3
	const CoinPrePostsolveMatrix::Status statj =
						prob->getColumnStatus(jcol) ;
	std::cout
	  << "      corr: x(" << jcol << ") "
	  << prob->columnStatusString(jcol) << " cbar " << new_cbarj ;
	if ((new_cbarj < -ztoldj &&
	     statj != CoinPrePostsolveMatrix::atUpperBound) ||
	    (new_cbarj > ztoldj &&
	     statj != CoinPrePostsolveMatrix::atLowerBound) ||
	    (statj == CoinPrePostsolveMatrix::basic &&
	     fabs(new_cbarj) > ztoldj))
	  std::cout << " error!" << std::endl ;
	else
	  std::cout << "." << std::endl ;
#       endif

      }
    }
# if PRESOLVE_DEBUG > 0
  presolve_check_nbasic(prob) ;
# endif
  }

# if PRESOLVE_DEBUG > 0 || PRESOLVE_CONSISTENCY > 0
  presolve_check_threads(prob) ;
  presolve_check_sol(prob,2,2,2) ;
  presolve_check_nbasic(prob) ;
# if PRESOLVE_DEBUG > 0
  std::cout << "Leaving forcing_constraint_action::postsolve." << std::endl ;
# endif
# endif

}


forcing_constraint_action::~forcing_constraint_action() 
{ 
  int i ;
  for (i=0;i<nactions_;i++) {
    //delete [] actions_[i].rowcols; MS Visual C++ V6 can not compile
    //delete [] actions_[i].bounds; MS Visual C++ V6 can not compile
    deleteAction(actions_[i].rowcols,int *) ;
    deleteAction(actions_[i].bounds,double *) ;
  }
  // delete [] actions_; MS Visual C++ V6 can not compile
  deleteAction(actions_,action *) ;
}

