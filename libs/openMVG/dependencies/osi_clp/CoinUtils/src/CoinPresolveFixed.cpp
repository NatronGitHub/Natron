/* $Id: CoinPresolveFixed.cpp 1565 2012-11-29 19:32:14Z lou $ */
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include <stdio.h>
#include <math.h>

#include "CoinPresolveMatrix.hpp"
#include "CoinPresolveFixed.hpp"
#include "CoinHelperFunctions.hpp"
#include "CoinFinite.hpp"

#if PRESOLVE_DEBUG > 0 || PRESOLVE_CONSISTENCY > 0
#include "CoinPresolvePsdebug.hpp"
#endif

/* Begin routines associated with remove_fixed_action */

const char *remove_fixed_action::name() const
{
  return ("remove_fixed_action");
}

/*
 * Original comment:
 *
 * invariant:  both reps are loosely packed.
 * coefficients of both reps remain consistent.
 *
 * Note that this concerns variables whose column bounds determine that
 * they are slack; this does NOT concern singleton row constraints that
 * determine that the relevant variable is slack.
 *
 * Invariant:  col and row rep are consistent
 */

/*
  This routine empties the columns for the list of fixed variables passed in
  (fcols, nfcols). As each coefficient a<ij> is set to 0, rlo<i> and rup<i>
  are adjusted accordingly. Note, however, that c<j> is not considered to be
  removed from the objective until column j is physically removed from the
  matrix (drop_empty_cols_action), so the correction to the objective is
  adjusted there.

  If a column solution is available, row activity (acts_) is adjusted.
  remove_fixed_action implicitly assumes that the value of the variable has
  already been forced within bounds. If this isn't true, the correction to
  acts_ will be wrong. See make_fixed_action if you need to force the value
  within bounds first.
*/
const remove_fixed_action*
  remove_fixed_action::presolve (CoinPresolveMatrix *prob,
				 int *fcols, int nfcols,
				 const CoinPresolveAction *next)
{
  double *colels	= prob->colels_;
  int *hrow		= prob->hrow_;
  CoinBigIndex *mcstrt	= prob->mcstrt_;
  int *hincol		= prob->hincol_;

  double *rowels	= prob->rowels_;
  int *hcol		= prob->hcol_;
  CoinBigIndex *mrstrt	= prob->mrstrt_;
  int *hinrow		= prob->hinrow_;

  double *clo	= prob->clo_;
  double *rlo	= prob->rlo_;
  double *rup	= prob->rup_;
  double *sol	= prob->sol_;
  double *acts	= prob->acts_;

  presolvehlink *clink = prob->clink_;
  presolvehlink *rlink = prob->rlink_;

  action *actions 	= new  action[nfcols+1];

# if PRESOLVE_DEBUG > 0 || PRESOLVE_CONSISTENCY > 0
# if PRESOLVE_DEBUG > 0
  std::cout
    << "Entering remove_fixed_action::presolve; processing " << nfcols
    << " fixed columns." << std::endl ;
# endif
  presolve_check_sol(prob) ;
  presolve_check_nbasic(prob) ;
# endif

/*
  Scan columns to be removed and total up the number of coefficients.
*/
  int estsize=0;
  int ckc;
  for (ckc = 0 ; ckc < nfcols ; ckc++) {
    int j = fcols[ckc];
    estsize += hincol[j];
  }
// Allocate arrays to hold coefficients and associated row indices
  double * els_action = new double[estsize];
  int * rows_action = new int[estsize];
  int actsize=0;
  // faster to do all deletes in row copy at once
  int nrows		= prob->nrows_;
  CoinBigIndex * rstrt = new int[nrows+1];
  CoinZeroN(rstrt,nrows);

/*
  Open a loop to excise each column a<j>. The first thing to do is load the
  action entry with the index j, the value of x<j>, and the number of
  entries in a<j>. After we walk the column and tweak the row-major
  representation, we'll simply claim this column is empty by setting
  hincol[j] = 0.
*/
  for (ckc = 0 ; ckc < nfcols ; ckc++) {
    int j = fcols[ckc];
    double solj = clo[j];
    CoinBigIndex kcs = mcstrt[j];
    CoinBigIndex kce = kcs + hincol[j];
    CoinBigIndex k;

    { action &f = actions[ckc];
      f.col = j;
      f.sol = solj;
      f.start = actsize;
    }
/*
  Now walk a<j>. For each row i with a coefficient a<ij> != 0:
    * save the coefficient and row index,
    * substitute the value of x<j>, adjusting the row bounds and lhs value
      accordingly, then
    * delete a<ij> from the row-major representation.
    * Finally: mark the row as changed and add it to the list of rows to be
	processed next. Then, for each remaining column in the row, put it on
	the list of columns to be processed.
*/
    for (k = kcs ; k < kce ; k++) {
      int row = hrow[k];
      double coeff = colels[k];
     
      els_action[actsize]=coeff;
      rstrt[row]++; // increase counts
      rows_action[actsize++]=row;

      // Avoid reducing finite infinity.
      if (-PRESOLVE_INF < rlo[row])
	rlo[row] -= solj*coeff;
      if (rup[row] < PRESOLVE_INF)
	rup[row] -= solj*coeff;
      if (sol) {
	acts[row] -= solj*coeff;
      }
#define TRY2
#ifndef TRY2
      presolve_delete_from_row(row,j,mrstrt,hinrow,hcol,rowels);
      if (hinrow[row] == 0)
      { PRESOLVE_REMOVE_LINK(rlink,row) ; }

      // mark unless already marked
      if (!prob->rowChanged(row)) {
	prob->addRow(row);
	CoinBigIndex krs = mrstrt[row];
	CoinBigIndex kre = krs + hinrow[row];
	for (CoinBigIndex k=krs; k<kre; k++) {
	  int jcol = hcol[k];
	  prob->addCol(jcol);
	}
      }
#endif
    }
/*
  Remove the column's link from the linked list of columns, and declare
  it empty in the column-major representation. Link removal must execute
  even if the column is already of length 0 when it arrives.
*/
    PRESOLVE_REMOVE_LINK(clink, j);
    hincol[j] = 0;
  }
/*
  Set the actual end of the coefficient and row index arrays.
*/
  actions[nfcols].start=actsize;
# if PRESOLVE_SUMMARY
  printf("NFIXED:  %d", nfcols);
  if (estsize-actsize > 0)
  { printf(", overalloc %d",estsize-actsize) ; }
  printf("\n") ;
# endif
  // Now get columns by row
  int * column = new int[actsize];
  int nel=0;
  int iRow;
  for (iRow=0;iRow<nrows;iRow++) {
    int n=rstrt[iRow];
    rstrt[iRow]=nel;
    nel += n;
  }
  rstrt[nrows]=nel;
  for (ckc = 0 ; ckc < nfcols ; ckc++) {
    int kcs = actions[ckc].start;
    int j=actions[ckc].col;
    int kce;
    if (ckc<nfcols-1)
      kce = actions[ckc+1].start;
    else
      kce = actsize;
    for (int k=kcs;k<kce;k++) {
      int iRow = rows_action[k];
      CoinBigIndex put = rstrt[iRow];
      rstrt[iRow]++;
      column[put]=j;
    }
  }
  // Now do rows
  int ncols		= prob->ncols_;
  char * mark = new char[ncols];
  memset(mark,0,ncols);
  // rstrts are now one out i.e. rstrt[0] is end of row 0
  nel=0;
#ifdef TRY2
  for (iRow=0;iRow<nrows;iRow++) {
    int k;
    for (k=nel;k<rstrt[iRow];k++) {
      mark[column[k]]=1;
    }
    presolve_delete_many_from_major(iRow,mark,mrstrt,hinrow,hcol,rowels);
#if PRESOLVE_DEBUG > 0
    for (k = nel ; k < rstrt[iRow] ; k++) {
      assert(mark[column[k]] == 0) ;
    }
#endif
    if (hinrow[iRow] == 0)
      {
        PRESOLVE_REMOVE_LINK(rlink,iRow) ;
      }
    // mark unless already marked
    if (!prob->rowChanged(iRow)) {
      prob->addRow(iRow);
      CoinBigIndex krs = mrstrt[iRow];
      CoinBigIndex kre = krs + hinrow[iRow];
      for (CoinBigIndex k=krs; k<kre; k++) {
        int jcol = hcol[k];
        prob->addCol(jcol);
      }
    }
    nel=rstrt[iRow];
  }
#endif
  delete [] mark;
  delete [] column;
  delete [] rstrt;

/*
  Create the postsolve object, link it at the head of the list of postsolve
  objects, and return a pointer.
*/
  const remove_fixed_action *fixedActions =
      new remove_fixed_action(nfcols,actions,els_action,rows_action,next) ;

# if PRESOLVE_DEBUG > 0 || PRESOLVE_CONSISTENCY > 0
  presolve_check_sol(prob) ;
# if PRESOLVE_DEBUG > 0
  std::cout << "Leaving remove_fixed_action::presolve." << std::endl ;
# endif
# endif

  return (fixedActions) ;
}


remove_fixed_action::remove_fixed_action(int nactions,
					 action *actions,
					 double * els_action,
					 int * rows_action,
					 const CoinPresolveAction *next) :
  CoinPresolveAction(next),
  colrows_(rows_action),
  colels_(els_action),
  nactions_(nactions),
  actions_(actions)
{
}

remove_fixed_action::~remove_fixed_action()
{
  deleteAction(actions_,action*);
  delete [] colels_;
  delete [] colrows_;
}

/*
 * Say we determined that cup - clo <= ztolzb, so we fixed sol at clo.
 * This involved subtracting clo*coeff from ub/lb for each row the
 * variable occurred in.
 * Now when we put the variable back in, by construction the variable
 * is within tolerance, the non-slacks are unchanged, and the 
 * distances of the affected slacks from their bounds should remain
 * unchanged (ignoring roundoff errors).
 * It may be that by adding the term back in, the affected constraints
 * now aren't as accurate due to round-off errors; this could happen
 * if only one summand and the slack in the original formulation were large
 * (and naturally had opposite signs), and the new term in the constraint
 * is about the size of the old slack, so the new slack becomes about 0.
 * It may be that there is catastrophic cancellation in the summation,
 * so it might not compute to 0.
 */
void remove_fixed_action::postsolve(CoinPostsolveMatrix *prob) const
{
  action * actions	= actions_;
  const int nactions	= nactions_;

  double *colels	= prob->colels_;
  int *hrow		= prob->hrow_;
  CoinBigIndex *mcstrt	= prob->mcstrt_;
  int *hincol		= prob->hincol_;
  int *link		= prob->link_;
  CoinBigIndex &free_list = prob->free_list_;

  double *clo	= prob->clo_;
  double *cup	= prob->cup_;
  double *rlo	= prob->rlo_;
  double *rup	= prob->rup_;

  double *sol	= prob->sol_;
  double *dcost	= prob->cost_;
  double *rcosts	= prob->rcosts_;

  double *acts	= prob->acts_;
  double *rowduals = prob->rowduals_;

  unsigned char *colstat	= prob->colstat_;

  const double maxmin	= prob->maxmin_;

# if PRESOLVE_DEBUG > 0 || PRESOLVE_CONSISTENCY > 0
  char *cdone	= prob->cdone_;
# if PRESOLVE_DEBUG > 0
  std::cout
    << "Entering remove_fixed_action::postsolve, repopulating " << nactions
    << " columns." << std::endl ;
# endif
  presolve_check_threads(prob) ;
  presolve_check_free_list(prob) ;
  presolve_check_sol(prob,2,2,2) ;
  presolve_check_nbasic(prob) ;
# endif

  double * els_action = colels_;
  int * rows_action = colrows_;
  int end = actions[nactions].start;

/*
  At one point, it turned out that forcing_constraint_action was putting
  duplicates in the column list it passed to remove_fixed_action. This is now
  fixed, but ... it looks to me like we could be in trouble here if we
  reinstate a column multiple times. Hence the assert.
*/
  for (const action *f = &actions[nactions-1]; actions<=f; f--) {
    int icol = f->col;
    const double thesol = f->sol;

# if PRESOLVE_DEBUG > 0 || PRESOLVE_CONSISTENCY > 0
    if (cdone[icol] == FIXED_VARIABLE) {
      std::cout
        << "RFA::postsolve: column " << icol << " already unfixed!"
	<< std::endl ;
      assert(cdone[icol] != FIXED_VARIABLE) ;
    }
    cdone[icol] = FIXED_VARIABLE ;
# endif

    sol[icol] = thesol;
    clo[icol] = thesol;
    cup[icol] = thesol;

    int cs = NO_LINK ;
    int start = f->start;
    double dj = maxmin * dcost[icol];
    
    for (int i=start; i<end; ++i) {
      int row = rows_action[i];
      double coeff =els_action[i];
      
      // pop free_list
      CoinBigIndex k = free_list;
      assert(k >= 0 && k < prob->bulk0_) ;
      free_list = link[free_list];
      // restore
      hrow[k] = row;
      colels[k] = coeff;
      link[k] = cs;
      cs = k;

      if (-PRESOLVE_INF < rlo[row])
	rlo[row] += coeff * thesol;
      if (rup[row] < PRESOLVE_INF)
	rup[row] += coeff * thesol;
      acts[row] += coeff * thesol;
      
      dj -= rowduals[row] * coeff;
    }

#   if PRESOLVE_CONSISTENCY > 0
    presolve_check_free_list(prob) ;
#   endif
      
    mcstrt[icol] = cs;
    
    rcosts[icol] = dj;
    hincol[icol] = end-start;
    end=start;

    /* Original comment:
     * the bounds in the reduced problem were tightened.
     * that means that this variable may not have been basic
     * because it didn't have to be,
     * but now it may have to.
     * no - the bounds aren't changed by this operation
     */
/*
  We've reintroduced the variable, but it's still fixed (equal bounds).
  Pick the nonbasic status that agrees with the reduced cost. Later, if
  postsolve unfixes the variable, we'll need to confirm that this status is
  still viable. We live in a minimisation world here.
*/
    if (colstat)
    { if (dj < 0)
	prob->setColumnStatus(icol,CoinPrePostsolveMatrix::atUpperBound);
      else
	prob->setColumnStatus(icol,CoinPrePostsolveMatrix::atLowerBound); }

  }


# if PRESOLVE_CONSISTENCY > 0 || PRESOLVE_DEBUG > 0
  presolve_check_threads(prob) ;
  presolve_check_sol(prob,2,2,2) ;
  presolve_check_nbasic(prob) ;
# if PRESOLVE_DEBUG > 0
  std::cout << "Leaving remove_fixed_action::postsolve." << std::endl ;
# endif
# endif

  return ;
}

/*
  Scan the problem for variables that are already fixed, and remove them.
  There's an implicit assumption that the value of the variable is already
  within bounds. If you want to protect against this possibility, you want to
  use make_fixed.
*/
const CoinPresolveAction *remove_fixed (CoinPresolveMatrix *prob,
					const CoinPresolveAction *next)
{
  int ncols	= prob->ncols_;
  int *fcols	= new int[ncols];
  int nfcols	= 0;

  int *hincol		= prob->hincol_;

  double *clo	= prob->clo_;
  double *cup	= prob->cup_;

  for (int i = 0 ; i < ncols ; i++)
    if (hincol[i] > 0 && clo[i] == cup[i]&&!prob->colProhibited2(i))
      fcols[nfcols++] = i;

  if (nfcols > 0)
  { next = remove_fixed_action::presolve(prob, fcols, nfcols, next) ; }
  delete[]fcols;
  return (next);
}

/* End routines associated with remove_fixed_action */

/* Begin routines associated with make_fixed_action */

const char *make_fixed_action::name() const
{
  return ("make_fixed_action");
}


/*
  This routine does the actual job of fixing one or more variables. The set
  of indices to be fixed is specified by nfcols and fcols. fix_to_lower
  specifies the bound where the variable(s) should be fixed. The other bound
  is preserved as part of the action and the bounds are set equal. Note that
  you don't get to specify the bound on a per-variable basis.

  If a primal solution is available, make_fixed_action will adjust the the
  row activity to compensate for forcing the variable within bounds. If the
  bounds are already equal, and the variable is within bounds, you should
  consider remove_fixed_action.
*/
const CoinPresolveAction*
make_fixed_action::presolve (CoinPresolveMatrix *prob,
			     int *fcols, int nfcols,
			     bool fix_to_lower,
			     const CoinPresolveAction *next)

{ double *clo	= prob->clo_;
  double *cup	= prob->cup_;
  double *csol	= prob->sol_;

  double *colels = prob->colels_;
  int *hrow	= prob->hrow_;
  CoinBigIndex *mcstrt	= prob->mcstrt_;
  int *hincol	= prob->hincol_;

  double *acts	= prob->acts_;

# if PRESOLVE_DEBUG > 0 || PRESOLVE_CONSISTENCY > 0
# if PRESOLVE_DEBUG > 0
  std::cout
    << "Entering make_fixed_action::presolve, fixed = " << nfcols << "."
    << std::endl ;
# endif
  presolve_check_sol(prob) ;
  presolve_check_nbasic(prob) ;
# endif

/*
  Shouldn't happen, but ...
*/
  if (nfcols <= 0) {
#   if PRESOLVE_DEBUG > 0
    std::cout
      << "make_fixed_action::presolve: useless call, " << nfcols
      << " to fix." << std::endl ;
#   endif
    return (next) ;
  }

  action *actions = new action[nfcols] ;

/*
  Scan the set of indices specifying variables to be fixed. For each variable,
  stash the unused bound in the action and set the bounds equal. If the client
  has passed in a primal solution, update it if the value of the variable
  changes.
*/
  for (int ckc = 0 ; ckc < nfcols ; ckc++)
  { int j = fcols[ckc] ;
    double movement = 0 ;

    action &f = actions[ckc] ;

    f.col = j ;
    if (fix_to_lower) {
      f.bound = cup[j];
      cup[j] = clo[j];
      if (csol) {
	movement = clo[j]-csol[j] ;
	csol[j] = clo[j] ;
      }
    } else {
      f.bound = clo[j];
      clo[j] = cup[j];
      if (csol) {
	movement = cup[j]-csol[j];
	csol[j] = cup[j];
      }
    }
    if (movement) {
      CoinBigIndex k;
      for (k = mcstrt[j] ; k < mcstrt[j]+hincol[j] ; k++) {
	int row = hrow[k];
	acts[row] += movement*colels[k];
      }
    }
  }
/*
  Original comment:
  This is unusual in that the make_fixed_action transform contains within it
  a remove_fixed_action transform. Bad idea?

  Explanatory comment:
  Now that we've adjusted the bounds, time to create the postsolve action
  that will restore the original bounds. But wait! We're not done. By calling
  remove_fixed_action::presolve, we will (virtually) remove these variables
  from the model by substituting for the variable where it occurs and emptying
  the column. Cache the postsolve transform that will repopulate the column
  inside the postsolve transform for fixing the bounds.
*/
  if (nfcols > 0) {
    next = new make_fixed_action(nfcols,actions,fix_to_lower,
			   remove_fixed_action::presolve(prob,fcols,nfcols,0),
				 next) ;
  }

# if PRESOLVE_DEBUG > 0 || PRESOLVE_CONSISTENCY > 0
  presolve_check_sol(prob) ;
  presolve_check_nbasic(prob) ;
# if PRESOLVE_DEBUG > 0
  std::cout << "Leaving make_fixed_action::presolve." << std::endl ;
# endif
# endif

  return (next) ;
}

/*
  Recall that in presolve, make_fixed_action forced a bound to fix a variable,
  then called remove_fixed_action to empty the column. removed_fixed_action
  left a postsolve object hanging off faction_, and our first act here is to
  call r_f_a::postsolve to repopulate the columns. The m_f_a postsolve activity
  consists of relaxing one of the bounds and making sure that the status is
  still viable (we can potentially eliminate the bound here).
*/
void make_fixed_action::postsolve(CoinPostsolveMatrix *prob) const
{
  const action *const actions = actions_;
  const int nactions = nactions_;
  const bool fix_to_lower = fix_to_lower_;

  double *clo	= prob->clo_;
  double *cup	= prob->cup_;
  double *sol	= prob->sol_ ;
  unsigned char *colstat = prob->colstat_;

# if PRESOLVE_CONSISTENCY > 0 || PRESOLVE_DEBUG > 0
# if PRESOLVE_DEBUG > 0
  std::cout << "Entering make_fixed_action::postsolve." << std::endl ;
# endif
  presolve_check_threads(prob) ;
  presolve_check_sol(prob,2,2,2) ;
  presolve_check_nbasic(prob) ;
# endif

/*
  Repopulate the columns.
*/
  assert(nactions == faction_->nactions_) ;
  faction_->postsolve(prob);
/*
  Walk the actions: restore each bound and check that the status is still
  appropriate. Given that we're unfixing a fixed variable, it's safe to assume
  that the unaffected bound is finite.
*/
  for (int cnt = nactions-1 ; cnt >= 0 ; cnt--)
  { const action *f = &actions[cnt];
    int icol = f->col;
    double xj = sol[icol] ;

    assert(faction_->actions_[cnt].col == icol) ;

    if (fix_to_lower)
    { double ub = f->bound ;
      cup[icol] = ub ;
      if (colstat)
      { if (ub >= PRESOLVE_INF || xj != ub)
	{ prob->setColumnStatus(icol,
				CoinPrePostsolveMatrix::atLowerBound) ; } } }
    else
    { double lb = f->bound ;
      clo[icol] = lb ;
      if (colstat)
      { if (lb <= -PRESOLVE_INF || xj != lb)
	{ prob->setColumnStatus(icol,
				CoinPrePostsolveMatrix::atUpperBound) ; } } } }

# if PRESOLVE_CONSISTENCY > 0 || PRESOLVE_DEBUG > 0
  presolve_check_threads(prob) ;
  presolve_check_sol(prob,2,2,2) ;
  presolve_check_nbasic(prob) ;
  std::cout << "Leaving make_fixed_action::postsolve." << std::endl ;
# endif
  return ; }

/*
  Scan the columns and collect indices of columns that have upper and lower
  bounds within the zero tolerance of one another. Hand this list to
  make_fixed_action::presolve() to do the heavy lifting.

  make_fixed_action will compensate for variables which are infeasible, forcing
  them to feasibility and correcting the row activity, before invoking
  remove_fixed_action to remove the variable from the problem. If you're
  confident of feasibility, consider remove_fixed.
*/
const CoinPresolveAction *make_fixed (CoinPresolveMatrix *prob,
				      const CoinPresolveAction *next)
{
# if PRESOLVE_DEBUG > 0 || PRESOLVE_CONSISTENCY > 0
# if PRESOLVE_DEBUG > 0
  std::cout
    << "Entering make_fixed, checking " << prob->ncols_ << " columns."
    << std::endl ;
# endif
  presolve_check_sol(prob) ;
  presolve_check_nbasic(prob) ;
# endif

  int ncols = prob->ncols_ ;
  int *fcols = prob->usefulColumnInt_ ;
  int nfcols = 0 ;

  int *hincol = prob->hincol_ ;

  double *clo = prob->clo_ ;
  double *cup = prob->cup_ ;

  for (int i = 0 ; i < ncols ; i++)
  { if (hincol[i] > 0 &&
	fabs(cup[i]-clo[i]) < ZTOLDP && !prob->colProhibited2(i)) 
    { fcols[nfcols++] = i ; } }

/*
  Call m_f_a::presolve to do the heavy lifting. This will create a new
  CoinPresolveAction, which will become the head of the list of
  CoinPresolveAction's currently pointed to by next.

  No point in going through the effort of a call if there are no fixed
  variables.
*/
  if (nfcols > 0)
    next = make_fixed_action::presolve(prob,fcols,nfcols,true,next) ;

# if PRESOLVE_DEBUG > 0 || PRESOLVE_CONSISTENCY > 0
  presolve_check_sol(prob) ;
  presolve_check_nbasic(prob) ;
# if PRESOLVE_DEBUG > 0
  std::cout
    << "Leaving make_fixed, fixed " << nfcols << " columns." << std::endl ;
# endif
# endif

  return (next) ; }

/*
  Transfer the cost coefficient of a column singleton in an equality to the
  cost coefficients of the remaining variables. Suppose x<s> is the singleton
  and x<t> is some other variable. For movement delta<t>, there must be
  compensating change delta<s> = -(a<it>/a<is>)delta<t>. Substituting in the
  objective, c<s>delta<s> + c<t>delta<t> becomes
    c<s>(-a<it>/a<is>)delta<t> + c<t>delta<t>
    (c<t> - c<s>(a<it>/a<is>))delta<t>
  This is transform (A) below.
*/
void transferCosts (CoinPresolveMatrix *prob)
{
  double *colels = prob->colels_ ;
  int *hrow = prob->hrow_ ;
  CoinBigIndex *mcstrt = prob->mcstrt_ ;
  int *hincol = prob->hincol_ ;

  double *rowels = prob->rowels_ ;
  int *hcol = prob->hcol_ ;
  CoinBigIndex *mrstrt = prob->mrstrt_ ;
  int *hinrow = prob->hinrow_ ;

  double *rlo = prob->rlo_ ;
  double *rup = prob->rup_ ;
  double *clo = prob->clo_ ;
  double *cup = prob->cup_ ;
  int ncols = prob->ncols_ ;
  double *cost	= prob->cost_ ; 
  unsigned char *integerType = prob->integerType_ ;
  double bias = prob->dobias_ ;

# if PRESOLVE_DEBUG > 0
  std::cout << "Entering transferCosts." << std::endl ;
  presolve_check_sol(prob) ;
  presolve_check_nbasic(prob) ;
# endif

  int numberIntegers = 0 ;
  for (int icol = 0 ; icol < ncols ; icol++) {
    if (integerType[icol]) numberIntegers++ ;
  }
/*
  For unfixed column singletons in equalities, calculate and install transform
  (A) described in the comments at the head of the method.
*/
  int nchanged = 0 ;
  for (int js = 0 ; js < ncols ; js++) {
    if (cost[js] && hincol[js] == 1 && cup[js] > clo[js]) {
      const CoinBigIndex &jsstrt = mcstrt[js] ;
      const int &i = hrow[jsstrt];
      if (rlo[i] == rup[i]) {
        const double ratio = cost[js]/colels[jsstrt];
        bias += rlo[i]*ratio ;
	const CoinBigIndex &istrt = mrstrt[i] ;
	const CoinBigIndex iend = istrt+hinrow[i] ;
        for (CoinBigIndex jj = istrt ; jj < iend ; jj++) {
          int j = hcol[jj] ;
          double aij = rowels[jj] ;
          cost[j] -= ratio*aij ;
        }
        cost[js] = 0.0 ;
        nchanged++ ;
      }
    }
  }
# if PRESOLVE_DEBUG > 0
  if (nchanged)
    std::cout
      << "  transferred costs for " << nchanged << " singleton columns."
      << std::endl ;

  int nPasses = 0 ;
# endif
/*
  We don't really need a singleton column to do this trick, just an equality.
  But if the column's not a singleton, it's only worth doing if we can move
  costs onto integer variables that start with costs of zero. Try and find some
  unfixed variable with a nonzero cost, that's involved in an equality where
  there are integer variables with costs of zero. If there's a net gain in the
  number of integer variables with costs (nThen > nNow), do the transform.
  One per column, please.
*/
  if (numberIntegers) {
    int changed = -1 ;
    while (changed) {
      changed = 0 ;
      for (int js = 0 ; js < ncols ; js++) {
        if (cost[js] && cup[js] > clo[js]) {
	  const CoinBigIndex &jsstrt = mcstrt[js] ;
	  const CoinBigIndex jsend = jsstrt+hincol[js] ;
          for (CoinBigIndex ii = jsstrt ; ii < jsend ; ii++) {
            const int &i = hrow[ii] ;
            if (rlo[i] == rup[i]) {
              int nNow = ((integerType[js])?1:0) ;
              int nThen = 0 ;
	      const CoinBigIndex &istrt = mrstrt[i] ;
	      const CoinBigIndex iend = istrt+hinrow[i] ;
              for (CoinBigIndex jj = istrt ; jj < iend ; jj++) {
                int j = hcol[jj] ;
                if (!cost[j] && integerType[j]) nThen++ ;
              }
              if (nThen > nNow) {
                const double ratio = cost[js]/colels[jsstrt] ;
                bias += rlo[i]*ratio ;
                for (CoinBigIndex jj = istrt ; jj < iend ; jj++) {
                  int j = hcol[jj] ;
                  double aij = rowels[jj] ;
                  cost[j] -= ratio*aij ;
                }
                cost[js] = 0.0 ;
                changed++ ;
                break ;
              }
            }
          }
        }
      }
      if (changed) {
        nchanged += changed ;
#	if PRESOLVE_DEBUG > 0
	std::cout
	  << "    pass " << nPasses++ << " transferred costs to "
	  << changed << " integer variables." << std::endl ;
#       endif
      }
    }
  }
# if PRESOLVE_DEBUG > 0
  if (bias != prob->dobias_)
    std::cout << "  new bias " << bias << "." << std::endl ;
# endif
  prob->dobias_ = bias;

# if PRESOLVE_DEBUG > 0
  std::cout << "Leaving transferCosts." << std::endl ;
  presolve_check_sol(prob) ;
  presolve_check_nbasic(prob) ;
# endif
}


