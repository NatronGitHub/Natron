/* $Id: CoinPresolveEmpty.cpp 1607 2013-07-16 09:01:29Z stefan $ */
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include <stdio.h>
#include <math.h>

#include "CoinFinite.hpp"
#include "CoinHelperFunctions.hpp"
#include "CoinPresolveMatrix.hpp"

#include "CoinPresolveEmpty.hpp"
#include "CoinMessage.hpp"

#if PRESOLVE_DEBUG > 0 || PRESOLVE_CONSISTENCY > 0
#include "CoinPresolvePsdebug.hpp"
#endif

/* \file

  Routines to remove/reinsert empty columns and rows.
*/


/*
  Physically remove empty columns, compressing mcstrt and hincol. The major
  side effect is that columns are renumbered, thus clink_ is no longer valid
  and must be rebuilt. And we're totally inconsistent with the row-major
  representation.

  It's necessary to rebuild clink_ in order to do direct conversion of a
  CoinPresolveMatrix to a CoinPostsolveMatrix by transferring the data arrays.
  Without clink_, it's impractical to build link_ to match the transferred bulk
  storage.
*/
const CoinPresolveAction
  *drop_empty_cols_action::presolve (CoinPresolveMatrix *prob,
				     const int *ecols,
				     int necols,
				     const CoinPresolveAction *next)
{

# if PRESOLVE_CONSISTENCY > 0
  presolve_links_ok(prob) ;
  presolve_consistent(prob) ;
# endif

  const int n_orig = prob->ncols_ ;

  CoinBigIndex *mcstrt = prob->mcstrt_ ;
  int *hincol = prob->hincol_ ;
  presolvehlink *clink = prob->clink_ ;

  double *clo = prob->clo_ ;
  double *cup = prob->cup_ ;
  double *cost = prob->cost_ ;

  const double ztoldj = prob->ztoldj_ ;

  unsigned char *integerType = prob->integerType_ ;
  int *originalColumn = prob->originalColumn_ ;

  const double maxmin = prob->maxmin_ ;

  double *sol = prob->sol_ ;
  unsigned char *colstat = prob->colstat_ ;

  action *actions = new action[necols] ;
  int *colmapping = new int [n_orig+1] ;
  CoinZeroN(colmapping,n_orig) ;

  // More like `ignore infeasibility'
  bool fixInfeasibility = ((prob->presolveOptions_&0x4000) != 0) ;

/*
  Open a loop to walk the list of empty columns. Mark them as empty in
  colmapping.
*/
  for (int ndx = necols-1 ; ndx >= 0 ; ndx--) {
    const int j = ecols[ndx] ;
    colmapping[j] = -1 ;
/*
  Groom bounds on integral variables. Check for previously undetected
  infeasibility unless the user wants to ignore it. If we find it, quit.
*/
    double &lj = clo[j] ;
    double &uj = cup[j] ;

    if (integerType[j]) {
      lj = ceil(lj-1.0e-9) ;
      uj = floor(uj+1.0e-9) ;
      if (lj > uj && !fixInfeasibility) {
	prob->status_ |= 1 ;
	prob->messageHandler()->message(COIN_PRESOLVE_COLINFEAS,
				        prob->messages())
	    << j << lj << uj << CoinMessageEol ;
	break ;
      }
    }
/*
  Load up the postsolve action with the index and rim vector components.
*/
    action &e = actions[ndx] ;
    e.jcol = j ;
    e.clo = lj ;
    e.cup = uj ;
    e.cost = cost[j] ;
/*
  There are no more constraints on this variable so we had better be able to
  compute the answer now. Try to make it nonbasic at bound. If we're
  unbounded, say so and quit.
*/
    if (fabs(cost[j]) < ztoldj) cost[j] = 0.0 ;
    if (cost[j] == 0.0) {
      if (-PRESOLVE_INF < lj)
        e.sol = lj ;
      else if (uj < PRESOLVE_INF)
        e.sol = uj ;
      else
        e.sol = 0.0 ;
    } else if (cost[j]*maxmin > 0.0) {
      if (-PRESOLVE_INF < lj)
	e.sol = lj ;
      else {
	prob->messageHandler()->message(COIN_PRESOLVE_COLUMNBOUNDB,
					prob->messages())
	    << j << CoinMessageEol ;
	prob->status_ |= 2 ;
	break ;
      }
    } else {
      if (uj < PRESOLVE_INF)
	e.sol = uj ;
      else {
	prob->messageHandler()->message(COIN_PRESOLVE_COLUMNBOUNDA,
					prob->messages())
	    << j << CoinMessageEol ;
	prob->status_ |= 2 ;
	break ;
      }
    }

#   if PRESOLVE_DEBUG > 2
    if (e.sol*cost[j]) {
      std::cout
        << "  non-zero cost " << cost[j] << " for empty col " << j << "."
	<< std::endl ;
    }
#   endif
    prob->change_bias(e.sol*cost[j]) ;
  }
/*
  No sense doing the actual work of compression unless we're still feasible
  and bounded. If we are, start out by compressing out the entries associated
  with empty columns. Empty columns are nonzero in colmapping.
*/
  if (prob->status_ == 0) {
    int n_compressed = 0 ;
    for (int ndx = 0 ; ndx < n_orig ; ndx++) {
      if (!colmapping[ndx]) {
	mcstrt[n_compressed] = mcstrt[ndx] ;
	hincol[n_compressed] = hincol[ndx] ;
      
	clo[n_compressed]   = clo[ndx] ;
	cup[n_compressed]   = cup[ndx] ;

	cost[n_compressed] = cost[ndx] ;
	if (sol) {
	  sol[n_compressed] = sol[ndx] ;
	  colstat[n_compressed] = colstat[ndx] ;
	}

	integerType[n_compressed] = integerType[ndx] ;
	originalColumn[n_compressed] = originalColumn[ndx] ;
	colmapping[ndx] = n_compressed++ ;
      }
    }
    mcstrt[n_compressed] = mcstrt[n_orig] ;
    colmapping[n_orig] = n_compressed ;
/*
  Rebuild clink_. At this point, all empty columns are linked out, so the
  only columns left are columns that are to be saved, hence available in
  colmapping.  All we need to do is walk clink_ and write the new entries
  into a new array.
*/

    presolvehlink *newclink = new presolvehlink [n_compressed+1] ;
    for (int oldj = n_orig ; oldj >= 0 ; oldj = clink[oldj].pre) {
      presolvehlink &oldlnk = clink[oldj] ;
      int newj = colmapping[oldj] ;
      assert(newj >= 0 && newj <= n_compressed) ;
      presolvehlink &newlnk = newclink[newj] ;
      if (oldlnk.suc >= 0) {
        newlnk.suc = colmapping[oldlnk.suc] ;
      } else {
        newlnk.suc = NO_LINK ;
      }
      if (oldlnk.pre >= 0) {
        newlnk.pre = colmapping[oldlnk.pre] ;
      } else {
        newlnk.pre = NO_LINK ;
      }
    }
    delete [] clink ;
    prob->clink_ = newclink ;

    prob->ncols_ = n_compressed ;
  }

  delete [] colmapping ;

# if PRESOLVE_CONSISTENCY > 0 || PRESOLVE_DEBUG > 0
  presolve_links_ok(prob,true,false) ;
  presolve_check_sol(prob) ;
  presolve_check_nbasic(prob) ;
# endif

  return (new drop_empty_cols_action(necols,actions,next)) ;
}

/*
  The top-level method scans the matrix for empty columns and calls a worker
  routine to do the heavy lifting.

  NOTE: At the end of this routine, the column- and row-major representations
	are not consistent. Empty columns have been compressed out,
	effectively renumbering the columns.
*/
const CoinPresolveAction
  *drop_empty_cols_action::presolve (CoinPresolveMatrix *prob,
  				     const CoinPresolveAction *next)
{

# if PRESOLVE_DEBUG > 0 || PRESOLVE_CONSISTENCY > 0
# if PRESOLVE_DEBUG > 0
  std::cout << "Entering drop_empty_cols_action::presolve." << std::endl ;
# endif
  presolve_consistent(prob) ;
  presolve_links_ok(prob) ;
  presolve_check_sol(prob) ;
  presolve_check_nbasic(prob) ;
# endif

  const int *hincol = prob->hincol_ ;
  int ncols = prob->ncols_ ;
  int nempty = 0 ;
  int *empty = new int [ncols] ;
  CoinBigIndex nelems2 = 0 ;

  // count empty cols
  for (int i = 0 ; i < ncols ; i++) {
    nelems2 += hincol[i] ;
    if (hincol[i] == 0&&!prob->colProhibited2(i)) {
#     if PRESOLVE_DEBUG > 1
      if (nempty == 0)
	std::cout << "UNUSED COLS:" ;
      else
      if (i < 100 && nempty%25 == 0)
	std::cout << std::endl ;
      else
      if (i >= 100 && i < 1000 && nempty%19 == 0)
	std::cout << std::endl ;
      else
      if (i >= 1000 && nempty%15 == 0)
	std::cout << std::endl ;
      std::cout << " " << i ;
#     endif
      empty[nempty++] = i;
    }
  }
  prob->nelems_ = nelems2 ;

  if (nempty)
    next = drop_empty_cols_action::presolve(prob,empty,nempty,next) ;

  delete [] empty ;

# if PRESOLVE_DEBUG > 0 || COIN_PRESOLVE_TUNING > 0
  std::cout << "Leaving drop_empty_cols_action::presolve" ;
  if (nempty) std::cout << ", dropped " << nempty << " columns" ;
  std::cout << "." << std::endl ;
# endif

  return (next);
}


/*
  Reintroduce empty columns dropped at the end of presolve.
*/
void drop_empty_cols_action::postsolve(CoinPostsolveMatrix *prob) const
{
  const int nactions = nactions_ ;
  const action *const actions = actions_ ;

  int ncols = prob->ncols_ ;

# if PRESOLVE_CONSISTENCY > 0 || PRESOLVE_DEBUG > 0
# if PRESOLVE_DEBUG > 0
  std::cout
    << "Entering drop_empty_cols_action::postsolve, initial system "
    << prob->nrows_ << "x" << ncols << ", " << nactions
    << " columns to restore." << std::endl ;
# endif
  char *cdone	= prob->cdone_ ;

  presolve_check_sol(prob,2,2,2) ;
  presolve_check_nbasic(prob) ;
# endif

  CoinBigIndex *colStarts = prob->mcstrt_ ;
  int *colLengths = prob->hincol_ ;

  double *clo = prob->clo_ ;
  double *cup = prob->cup_ ;

  double *sol = prob->sol_ ;
  double *cost = prob->cost_ ;
  double *rcosts = prob->rcosts_ ;
  unsigned char *colstat = prob->colstat_ ;
  const double maxmin = prob->maxmin_ ;

/*
  Set up a mapping vector, coded 0 for existing columns, -1 for columns we're
  about to reintroduce.
*/
  int ncols2 = ncols+nactions ;
  int *colmapping = new int [ncols2] ;
  CoinZeroN(colmapping,ncols2) ;
  for (int ndx = 0 ; ndx < nactions ; ndx++) {
    const action *e = &actions[ndx] ;
    int j = e->jcol ;
    colmapping[j] = -1 ;
  }
/*
  Working back from the highest index, expand the existing ncols columns over
  the full range ncols2, leaving holes for the columns we want to reintroduce.
*/
  for (int j = ncols2-1 ; j >= 0 ; j--) {
    if (!colmapping[j]) {
      ncols-- ;
      colStarts[j] = colStarts[ncols] ;
      colLengths[j] = colLengths[ncols] ;

      clo[j]   = clo[ncols] ;
      cup[j]   = cup[ncols] ;
      cost[j] = cost[ncols] ;

      if (sol) sol[j] = sol[ncols] ;
      if (rcosts) rcosts[j] = rcosts[ncols] ;
      if (colstat) colstat[j] = colstat[ncols] ;

#     if PRESOLVE_DEBUG > 0
      cdone[j] = cdone[ncols] ;
#     endif
    }
  }
  assert (!ncols) ;
  
  delete [] colmapping ;
/*
  Reintroduce the dropped columns.
*/
  for (int ndx = 0 ; ndx < nactions ; ndx++) {
    const action *e = &actions[ndx] ;
    int j = e->jcol ;
    colLengths[j] = 0 ;
    colStarts[j] = NO_LINK ;
    clo[j] = e->clo ;
    cup[j] = e->cup ;
    cost[j] = e->cost ;

    if (sol) sol[j] = e->sol ;
    if (rcosts) rcosts[j] = maxmin*cost[j] ;
    if (colstat) prob->setColumnStatusUsingValue(j) ;

#   if PRESOLVE_DEBUG > 0
    cdone[j] = DROP_COL ;
#   if PRESOLVE_DEBUG > 1
    std::cout
      << "  restoring col " << j << ", lb = " << clo[j] << ", ub = " << cup[j]
      << std::endl ;
#   endif
#   endif
  }

  prob->ncols_ += nactions ;

# if PRESOLVE_CONSISTENCY > 0 || PRESOLVE_DEBUG > 0
  presolve_check_threads(prob) ;
  presolve_check_sol(prob,2,2,2) ;
  presolve_check_nbasic(prob) ;
# if PRESOLVE_DEBUG > 0
  std::cout << "Leaving drop_empty_cols_action::postsolve, system "
    << prob->nrows_ << "x" << prob->ncols_ << "." << std::endl ;
# endif
# endif

}


const CoinPresolveAction
  *drop_empty_rows_action::presolve (CoinPresolveMatrix *prob,
				     const CoinPresolveAction *next)
{
# if PRESOLVE_DEBUG > 0
  std::cout << "Entering drop_empty_rows_action::presolve." << std::endl ;
# endif

  int ncols	= prob->ncols_;
  CoinBigIndex *mcstrt	= prob->mcstrt_;
  int *hincol	= prob->hincol_;
  int *hrow	= prob->hrow_;

  int nrows	= prob->nrows_;
  // This is done after row copy needed
  //int *mrstrt	= prob->mrstrt_;
  int *hinrow	= prob->hinrow_;
  //int *hcol	= prob->hcol_;
  
  double *rlo	= prob->rlo_;
  double *rup	= prob->rup_;

  unsigned char *rowstat	= prob->rowstat_;
  double *acts	= prob->acts_;
  int * originalRow  = prob->originalRow_;

  //presolvehlink *rlink = prob->rlink_;
  bool fixInfeasibility = ((prob->presolveOptions_&0x4000) != 0) ;
  // Relax tolerance
  double tolerance = 10.0*prob->feasibilityTolerance_;
  

  int i;
  int nactions = 0;
  for (i=0; i<nrows; i++)
    if (hinrow[i] == 0)
      nactions++;
/*
  Bail out if there's nothing to be done.
*/
  if (nactions == 0) {
#   if PRESOLVE_DEBUG > 0
    std::cout << "Leaving drop_empty_rows_action::presolve." << std::endl ;
#   endif
    return (next) ;
  }
/*
  Work to do.
*/
  action *actions 	= new action[nactions];
  int * rowmapping = new int [nrows];

  nactions = 0;
  int nrows2=0;
  for (i=0; i<nrows; i++) {
    if (hinrow[i] == 0) {
      action &e = actions[nactions];

#     if PRESOLVE_DEBUG > 1
      if (nactions == 0)
        std::cout << "UNUSED ROWS:" ;
      else
      if (i < 100 && nactions%25 == 0)
        std::cout << std::endl ;
      else
      if (i >= 100 && i < 1000 && nactions%19 == 0)
        std::cout << std::endl ;
      else
      if (i >= 1000 && nactions%15 == 0)
        std::cout << std::endl ;
      std::cout << " " << i ;
#     endif

      nactions++;
      if (rlo[i] > 0.0 || rup[i] < 0.0) {
	if ((rlo[i]<=tolerance &&
	     rup[i]>=-tolerance)||fixInfeasibility) {
	  rlo[i]=0.0;
	  rup[i]=0.0;
	} else {
	  prob->status_|= 1;
	prob->messageHandler()->message(COIN_PRESOLVE_ROWINFEAS,
					   prob->messages())
					     <<i
					     <<rlo[i]
					     <<rup[i]
					     <<CoinMessageEol;
	  break;
	}
      }
      e.row	= i;
      e.rlo	= rlo[i];
      e.rup	= rup[i];
      rowmapping[i]=-1;

    } else {
      // move down - we want to preserve order
      rlo[nrows2]=rlo[i];
      rup[nrows2]=rup[i];
      originalRow[nrows2]=i;
      if (acts) {
	acts[nrows2]=acts[i];
	rowstat[nrows2]=rowstat[i];
      }
      rowmapping[i]=nrows2++;
    }
  }
# if PRESOLVE_DEBUG > 1
  std::cout << std::endl ;
# endif

  // remap matrix
  for (i=0;i<ncols;i++) {
    int j;
    for (j=mcstrt[i];j<mcstrt[i]+hincol[i];j++) 
      hrow[j] = rowmapping[hrow[j]];
  }
  delete [] rowmapping;

  prob->nrows_ = nrows2;

  next = new drop_empty_rows_action(nactions,actions,next) ;

# if PRESOLVE_DEBUG > 0 || PRESOLVE_CONSISTENCY > 0
  presolve_check_nbasic(prob) ;
# if PRESOLVE_DEBUG > 0
  std::cout << "Leaving drop_empty_rows_action::presolve" ;
  if (nactions) std::cout << ", dropped " << nactions << " rows" ;
  std::cout << "." << std::endl ;
# endif
# endif

  return (next) ;
}

void drop_empty_rows_action::postsolve(CoinPostsolveMatrix *prob) const
{
  const int nactions = nactions_ ;
  const action *const actions = actions_ ;

  int ncols = prob->ncols_ ;
  int nrows0 = prob->nrows0_ ;
  int nrows = prob->nrows_ ;

  CoinBigIndex *mcstrt	= prob->mcstrt_ ;
  int *hincol = prob->hincol_ ;

  int *hrow = prob->hrow_ ;

  double *rlo = prob->rlo_ ;
  double *rup = prob->rup_ ;
  unsigned char *rowstat = prob->rowstat_ ;
  double *rowduals = prob->rowduals_ ;
  double *acts = prob->acts_ ;

# if PRESOLVE_CONSISTENCY > 0 || PRESOLVE_DEBUG > 0
# if PRESOLVE_DEBUG > 0
  std::cout
    << "Entering drop_empty_rows_action::postsolve, initial system "
    << nrows << "x" << ncols << ", " << nactions
    << " rows to restore." << std::endl ;
# endif
  char *rdone = prob->rdone_ ;

  presolve_check_sol(prob,2,2,2) ;
  presolve_check_nbasic(prob) ;
# endif

/*
  Process the array of actions and mark rowmapping[i] if constraint i was
  eliminated in presolve.
*/
  int *rowmapping = new int [nrows0] ;
  CoinZeroN(rowmapping,nrows0) ;
  for (int k = 0 ; k < nactions ; k++) {
    const action *e = &actions[k] ;
    int i = e->row ;
    rowmapping[i] = -1 ;
  }
/*
  Now walk the vectors for row bounds, activity, duals, and status. Expand
  the existing entries in 0..(nrows-1) to occupy 0..(nrows0-1), leaving
  holes for the rows we're about to reintroduce.
*/
  for (int i = nrows0-1 ; i >= 0 ; i--) {
    if (!rowmapping[i]) {
      nrows-- ;
      rlo[i] = rlo[nrows] ;
      rup[i] = rup[nrows] ;
      acts[i] = acts[nrows] ;
      rowduals[i] = rowduals[nrows] ;
      if (rowstat)
	rowstat[i] = rowstat[nrows] ;
#     if PRESOLVE_DEBUG > 0
      rdone[i] = rdone[nrows] ;
#     endif
    }
  }
  assert (!nrows) ;
/*
  Rewrite rowmapping so that it maps presolved row indices to row indices in
  the restored matrix.
*/
  for (int i = 0 ; i < nrows0 ; i++) {
    if (!rowmapping[i])
      rowmapping[nrows++] = i ;
  }
/*
  Now walk the row index array for each column, rewriting the row indices so
  they are correct for the restored matrix.
*/
  for (int j = 0 ; j < ncols ; j++) {
    const CoinBigIndex &start = mcstrt[j] ;
    const CoinBigIndex &end = start+hincol[j] ;
    for (CoinBigIndex k = start ; k < end ; k++) {
      hrow[k] = rowmapping[hrow[k]] ;
    }
  }
  delete [] rowmapping;
/*
  And reintroduce the (still empty) rows that were removed in presolve. The
  assumption is that an empty row cannot be tight, hence the logical is basic
  and the dual is zero.
*/
  for (int k = 0 ; k < nactions ; k++) {
    const action *e = &actions[k] ;
    int i = e->row ;
    rlo[i] = e->rlo ;
    rup[i] = e->rup ;
    acts[i] = 0.0 ;
    if (rowstat)
      prob->setRowStatus(i,CoinPrePostsolveMatrix::basic) ;
    rowduals[i] = 0.0 ;
#   if PRESOLVE_DEBUG > 0
    rdone[i] = DROP_ROW;
#   if PRESOLVE_DEBUG > 1
    std::cout
      << "  restoring row " << i << ", LB = " << rlo[i] << ", UB = " << rup[i]
      << std::endl ;
#   endif
#   endif
  }
  prob->nrows_ += nactions ;
  assert(prob->nrows_ == prob->nrows0_) ;

# if PRESOLVE_CONSISTENCY > 0 || PRESOLVE_DEBUG > 0
  presolve_check_threads(prob) ;
  presolve_check_sol(prob,2,2,2) ;
  presolve_check_nbasic(prob) ;
# if PRESOLVE_DEBUG > 0
  std::cout << "Leaving drop_empty_rows_action::postsolve, system "
    << prob->nrows_ << "x" << prob->ncols_ << "." << std::endl ;
# endif
# endif

}

