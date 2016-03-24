/* $Id: CoinPresolveSubst.cpp 1581 2013-04-06 12:48:50Z stefan $ */
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include <stdio.h>
#include <math.h>

#include "CoinPresolveMatrix.hpp"
#include "CoinPresolveEmpty.hpp"	// for DROP_COL/DROP_ROW
#include "CoinPresolvePsdebug.hpp"
#include "CoinPresolveFixed.hpp"
#include "CoinPresolveZeros.hpp"
#include "CoinPresolveSubst.hpp"
#include "CoinMessage.hpp"
#include "CoinHelperFunctions.hpp"
#include "CoinSort.hpp"
#include "CoinError.hpp"
#include "CoinFinite.hpp"

#if PRESOLVE_DEBUG > 0 || PRESOLVE_CONSISTENCY > 0
#include "CoinPresolvePsdebug.hpp"
#endif


namespace {	// begin unnamed file-local namespace

#if PRESOLVE_DEBUG > 0 || PRESOLVE_CONSISTENCY > 0
/*
  Special-purpose debug utility to look for coefficient a(i,j) in column j.
  Intended to be inserted as needed and removed when debugging is complete.
*/
void dbg_find_elem (const CoinPostsolveMatrix *postMtx, int i, int j)
{
  const CoinBigIndex kcs = postMtx->mcstrt_[j] ;
  const CoinBigIndex lenj = postMtx->hincol_[j] ;
  CoinBigIndex krow =
    presolve_find_row3(i,kcs,lenj,postMtx->hrow_,postMtx->link_) ;
  if (krow >= 0) {
    std::cout
      << "  row " << i << " present in column " << j 
      << ", a(" << i << "," << j
      << ") = " << postMtx->colels_[krow] << std::endl ;
  } else {
    std::cout
      << "  row " << i << " not present in column " << j << std::endl ;
  }
}
#endif

/*
  Add coeff_factor*rowy to rowx, for coefficients and row bounds. In the
  terminology used in ::presolve, rowy is the target row, and rowx is an
  entangled row (entangled with the target column).

  If a coefficient is < kill_ratio * coeff_factor then kill it

  Column indices in irowx and iroy must be sorted in increasing order.
  Normally one might do that here, but this routine is only called from
  subst_constraint_action::presolve and rowy will be the same over several
  calls. More efficient to sort in sca::presolve.

  Given we're called from sca::presolve, rowx will be an equality, with
  finite rlo[rowx] = rup[rowx] = rhsy.

  Fill-in in rowx has the potential to trigger compaction of the row-major
  bulk store. *All* indices into the bulk store are *not* constant if this
  happens.

  Returns false if the addition completes without error, true if there's a
  problem.
*/
bool add_row (CoinBigIndex *mrstrt, double *rlo, double *acts, double *rup,
	     double *rowels, int *hcol, int *hinrow, presolvehlink *rlink,
	      int nrows, double coeff_factor, double kill_ratio,  int irowx, int irowy,
	     int *x_to_y)
{
  CoinBigIndex krsy = mrstrt[irowy] ;
  CoinBigIndex krey = krsy+hinrow[irowy] ;
  CoinBigIndex krsx = mrstrt[irowx] ;
  CoinBigIndex krex = krsx+hinrow[irowx] ;

# if PRESOLVE_DEBUG > 3
  std::cout
    << "  ADD_ROW: adding (" << coeff_factor << ")*(row " << irowy
    << ") to row " << irowx << "; len y = " << hinrow[irowy]
    << ", len x = " << hinrow[irowx] << "." << std::endl ;
# endif

/*
  Do the simple part first: adjust the row lower and upper bounds, but only if
  they're finite.
*/
  const double rhsy = rlo[irowy] ;
  const double rhscorr = rhsy*coeff_factor ;
  const double tolerance = kill_ratio*coeff_factor;

  if (-PRESOLVE_INF < rlo[irowx]) {
    const double newrlo = rlo[irowx]+rhscorr ;
#   if PRESOLVE_DEBUG > 3
    if (rhscorr)
      std::cout
        << "  rlo(" << irowx << ") " << rlo[irowx] << " -> "
	<< newrlo << "." << std::endl ;
#   endif
    rlo[irowx] = newrlo ;
  }
  if (rup[irowx] < PRESOLVE_INF) {
    const double newrup = rup[irowx]+rhscorr ;
#   if PRESOLVE_DEBUG > 3
    if (rhscorr)
      std::cout
        << "  rup(" << irowx << ") " << rup[irowx] << " -> "
	<< newrup << "." << std::endl ;
#   endif
    rup[irowx] = newrup ;
  }
  if (acts)
  { acts[irowx] += rhscorr ; }
/*
  On to the main show. Open a loop to walk row y.  krowx is keeping track
  of where we're at in row x.  To find column j in row x, start from the
  current position and search forward, but no further than the last original
  coefficient of row x (fill will be added after this element).
*/
  CoinBigIndex krowx = krsx ;
  CoinBigIndex krex0 = krex ;
  int x_to_y_i = 0 ;

# if PRESOLVE_DEBUG > 3
  std::cout << "  ycols:" ;
# endif
  for (CoinBigIndex krowy = krsy ; krowy < krey ; krowy++) {
    int j = hcol[krowy] ;

    PRESOLVEASSERT(krex == krsx+hinrow[irowx]) ;

    while (krowx < krex0 && hcol[krowx] < j) krowx++ ;

#   if PRESOLVE_DEBUG > 3
    std::cout << " a(" << irowx << "," << j << ") " ;
#   endif
/*
  The easy case: coeff a(xj) already exists and all we need to is modify it.
*/
    if (krowx < krex0 && hcol[krowx] == j) {
      double newcoeff = rowels[krowx]+rowels[krowy]*coeff_factor ;

#     if PRESOLVE_DEBUG > 3
      std::cout << rowels[krowx] << " -> " << newcoeff << ";" ;
#     endif

      // kill small 
      if (fabs(newcoeff) <tolerance) 
	newcoeff=0.0;
      rowels[krowx] = newcoeff ;
      x_to_y[x_to_y_i++] = krowx-krsx ;
      krowx++ ;
    } else {
/*
  The hard case: a(xj) will be fill-in for row x. Make sure we have room. The
  process of making room can trigger bulk store compaction which can move all
  rows, so recalculate all pointers into the bulk store. Only then can we add
  the new coefficient.
*/
      double newValue = rowels[krowy]*coeff_factor ;
      bool outOfSpace = presolve_expand_row(mrstrt,rowels,hcol,
					    hinrow,rlink,nrows,irowx) ;
      if (outOfSpace) return (true) ;
      
      krowy = mrstrt[irowy]+(krowy-krsy) ;
      krsy = mrstrt[irowy] ;
      krey = krsy+hinrow[irowy] ;
      
      krowx = mrstrt[irowx]+(krowx-krsx) ;
      krex0 = mrstrt[irowx]+(krex0-krsx) ;
      krsx = mrstrt[irowx] ;
      krex = krsx+hinrow[irowx] ;
      
      hcol[krex] = j ;
      rowels[krex] = newValue;
      x_to_y[x_to_y_i++] = krex-krsx ;
      hinrow[irowx]++ ;
      krex++ ;
      
#     if PRESOLVE_DEBUG > 3
      std::cout << rowels[krex-1] << ";" ;
#     endif
    }
  }

# if PRESOLVE_DEBUG > 3
  std::cout << std::endl ;
# endif
  return (false) ;
}



} // end unnamed file-local namespace


const char *subst_constraint_action::name() const
{
  return ("subst_constraint_action");
}

/*
  This transform is called only from implied_free_action. See the comments at
  the head of CoinPresolveImpledFree.cpp for background.

  In addition to natural implied free singletons, implied_free_action will
  identify implied free variables that are not (yet) column singletons. This
  transform will process them.

  Suppose we have a variable x(t) and an equality r which satisfy the implied
  free condition (i.e., r imposes bounds on x(t) which are equal or better
  than the original column bounds). Then we can solve r for x(t) to get a
  substitution formula for x(t). We can use the substitution formula to
  eliminate x(t) from all other constraints where it is entangled. x(t) is now
  an implied free column singleton with equality r and we can remove x(t)
  and equality r from the constraint system.

  The paired parameter vectors implied_free and whichFree specify the indices
  for equality r and variable t, respectively. NOTE that these vectors are
  held in the first two blocks of usefulColumnInt_. Don't reuse them!

  Fill-in can cause a major vector to be moved to free space at the end
  of the bulk store. If there's not enough free space, this can trigger
  compaction of the entire bulk store. The upshot is that *all* major vector
  starts and ends are *not* constant over calls that could expand a major
  vector. Deletion, on the other hand, will never move a major vector (but
  it will move the end element into the hole left by the deleted element).
*/

const CoinPresolveAction *subst_constraint_action::presolve (
    CoinPresolveMatrix *prob,
    const int *implied_free, const int *whichFree, int numberFree,
    const CoinPresolveAction *next, int maxLook)
{

# if PRESOLVE_DEBUG > 0 || PRESOLVE_CONSISTENCY > 0
# if PRESOLVE_DEBUG > 0
  std::cout
    << "Entering subst_constraint_action::presolve, fill level "
    << maxLook << ", " << numberFree << " candidates." << std::endl ;
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

/*
  Unpack the row- and column-major representations.
*/
  const int ncols = prob->ncols_ ;
  const int nrows = prob->nrows_ ;

  CoinBigIndex *rowStarts = prob->mrstrt_ ;
  int *rowLengths = prob->hinrow_ ;
  double *rowCoeffs = prob->rowels_ ;
  int *colIndices = prob->hcol_ ;
  presolvehlink *rlink = prob->rlink_ ;

  CoinBigIndex *colStarts = prob->mcstrt_ ;
  int *colLengths = prob->hincol_ ;
  double *colCoeffs = prob->colels_ ;
  int *rowIndices = prob->hrow_ ;
  presolvehlink *clink = prob->clink_ ;

/*
  Row bounds and activity, objective.
*/
  double *rlo = prob->rlo_ ;
  double *rup = prob->rup_ ;
  double *acts = prob->acts_ ;
  double *cost = prob->cost_ ;


  const double tol = prob->feasibilityTolerance_ ;

  action *actions = new action [ncols] ;
# ifdef ZEROFAULT
  CoinZeroN(reinterpret_cast<char *>(actions),ncols*sizeof(action)) ;
# endif
  int nactions = 0 ;
/*
  This array is used to hold the indices of columns involved in substitutions,
  where we have the potential for cancellation. At the end they'll be
  checked to eliminate any actual zeros that may result. At the end of
  processing of each target row, the column indices of the target row are
  copied into zerocols.

  NOTE that usefulColumnInt_ is already in use for parameters implied_free and
  whichFree when this routine is called from implied_free.
*/
  int *zerocols = new int[ncols] ;
  int nzerocols = 0 ;

  int *x_to_y = new int[ncols] ;

  int *rowsUsed = &prob->usefulRowInt_[0] ;
  int nRowsUsed = 0 ;
/*
  Open a loop to process the (equality r, implied free variable t) pairs
  in whichFree and implied_free.

  It can happen that removal of (row, natural singleton) pairs back in
  implied_free will reduce the length of column t. It can also happen
  that previous processing here has resulted in fillin or cancellation. So
  check again for column length and exclude natural singletons and overly
  dense columns.
*/
  for (int iLook = 0 ; iLook < numberFree ; iLook++) {
    const int tgtcol = whichFree[iLook] ;
    const int tgtcol_len = colLengths[tgtcol] ;
    const int tgtrow = implied_free[iLook] ;
    const int tgtrow_len = rowLengths[tgtrow] ;

    assert(fabs(rlo[tgtrow]-rup[tgtrow]) < tol) ;

    if (colLengths[tgtcol] < 2 || colLengths[tgtcol] > maxLook) {
#     if PRESOLVE_DEBUG > 3
      std::cout
        << "    skipping eqn " << tgtrow << " x(" << tgtcol
	<< "); length now " << colLengths[tgtcol] << "." << std::endl ;
#     endif
      continue ;
    }

    CoinBigIndex tgtcs = colStarts[tgtcol] ;
    CoinBigIndex tgtce = tgtcs+colLengths[tgtcol] ;
/*
  A few checks to make sure that the candidate pair is still suitable.
  Processing candidates earlier in the list can eliminate coefficients.
    * Don't use this pair if any involved row i has become a row singleton
      or empty.
    * Don't use this pair if any involved row has been modified as part of
      the processing for a previous candidate pair on this call.
    * Don't use this pair if a(i,tgtcol) has become zero.

  The checks on a(i,tgtcol) seem superfluous but it's possible that
  implied_free identified two candidate pairs to eliminate the same column. If
  we've already processed one of them, we could be in trouble.
*/
    double tgtcoeff = 0.0 ;
    bool dealBreaker = false ;
    for (CoinBigIndex kcol = tgtcs ; kcol < tgtce ; ++kcol) {
      const int i = rowIndices[kcol] ;
      if (rowLengths[i] < 2 || prob->rowUsed(i)) {
        dealBreaker = true ;
	break ;
      }
      const double aij = colCoeffs[kcol] ;
      if (fabs(aij) <= ZTOLDP2) {
	dealBreaker = true ;
	break ;
      }
      if (i == tgtrow) tgtcoeff = aij ;
    }

    if (dealBreaker == true) {
#     if PRESOLVE_DEBUG > 3
      std::cout
        << "    skipping eqn " << tgtrow << " x(" << tgtcol
	<< "); deal breaker (1)." << std::endl ;
#     endif
      continue ;
    }
/*
  Check for numerical stability.A large coeff_factor will inflate the
  coefficients in the substitution formula.
*/
    dealBreaker = false ;
    for (CoinBigIndex kcol = tgtcs ; kcol < tgtce ; ++kcol) {
      const double coeff_factor = fabs(colCoeffs[kcol]/tgtcoeff) ;
      if (coeff_factor > 10.0)
	dealBreaker = true ;
    }
/*
  Given enough target rows with sufficient overlap, there's an outside chance
  we could overflow zerocols. Unlikely to ever happen.
*/
    if (!dealBreaker && nzerocols+rowLengths[tgtrow] >= ncols)
      dealBreaker = true ;
    if (dealBreaker == true) {
#     if PRESOLVE_DEBUG > 3
      std::cout
        << "    skipping eqn " << tgtrow << " x(" << tgtcol
	<< "); deal breaker (2)." << std::endl ;
#     endif
      continue ;
    }
/*
  If c(t) != 0, we will need to modify the objective coefficients and remember
  the original objective.
*/
    const bool nonzero_cost = (fabs(cost[tgtcol]) > tol) ;
    double *costsx = (nonzero_cost?new double[rowLengths[tgtrow]]:0) ;

#   if PRESOLVE_DEBUG > 1
    std::cout << "  Eliminating row " << tgtrow << ", col " << tgtcol ;
    if (nonzero_cost) std::cout << ", cost " << cost[tgtcol] ;
    std::cout << "." << std::endl ;
#   endif

/*
  Count up the total number of coefficients in entangled rows and mark them as
  contaminated.
*/
    int ntotels = 0 ;
    for (CoinBigIndex kcol = tgtcs ; kcol < tgtce ; ++kcol) {
      const int i = rowIndices[kcol] ;
      ntotels += rowLengths[i] ;
      PRESOLVEASSERT(!prob->rowUsed(i)) ;
      prob->setRowUsed(i) ;
      rowsUsed[nRowsUsed++] = i ;
    }
/*
  Create the postsolve object. Copy in all the affected rows. Take the
  opportunity to mark the entangled rows as changed and put them on the list
  of rows to process in the next round.

  coeffxs in particular holds the coefficients of the target column.
*/
    action *ap = &actions[nactions++] ;

    ap->col = tgtcol ;
    ap->rowy = tgtrow ;
    PRESOLVE_DETAIL_PRINT(printf("pre_subst %dC %dR E\n",tgtcol,tgtrow)) ;

    ap->nincol = tgtcol_len ;
    ap->rows = new int[tgtcol_len] ;
    ap->rlos = new double[tgtcol_len] ;
    ap->rups = new double[tgtcol_len] ;

    ap->costsx = costsx ;
    ap->coeffxs = new double[tgtcol_len] ;

    ap->ninrowxs = new int[tgtcol_len] ;
    ap->rowcolsxs = new int[ntotels] ;
    ap->rowelsxs = new double[ntotels] ;

    ntotels = 0 ;
    for (CoinBigIndex kcol = tgtcs ; kcol < tgtce ; ++kcol) {
      const int ndx = kcol-tgtcs ;
      const int i = rowIndices[kcol] ;
      const CoinBigIndex krs = rowStarts[i] ;
      prob->addRow(i) ;
      ap->rows[ndx] = i ;
      ap->ninrowxs[ndx] = rowLengths[i] ;
      ap->rlos[ndx] = rlo[i] ;
      ap->rups[ndx] = rup[i] ;
      ap->coeffxs[ndx] = colCoeffs[kcol] ;

      CoinMemcpyN(&colIndices[krs],rowLengths[i],&ap->rowcolsxs[ntotels]) ;
      CoinMemcpyN(&rowCoeffs[krs],rowLengths[i],&ap->rowelsxs[ntotels]) ;

      ntotels += rowLengths[i] ;
    }

    CoinBigIndex tgtrs = rowStarts[tgtrow] ;
    CoinBigIndex tgtre = tgtrs+rowLengths[tgtrow] ;

/*
  Adjust the objective coefficients based on the substitution formula
    c'(j) = c(j) - a(rj)c(t)/a(rt)
*/
    if (nonzero_cost) {
      const double tgtcost = cost[tgtcol] ;
      for (CoinBigIndex krow = tgtrs ; krow < tgtre ; krow ++) {
	const int j = colIndices[krow] ;
	prob->addCol(j) ;
	costsx[krow-tgtrs] = cost[j] ;
	double coeff = rowCoeffs[krow] ;
	cost[j] -= (tgtcost*coeff)/tgtcoeff ;
      }
      prob->change_bias(tgtcost*rlo[tgtrow]/tgtcoeff) ;
      cost[tgtcol] = 0.0 ;
    }

#   if PRESOLVE_DEBUG > 1
    std::cout << "  tgt (" << tgtrow << ") (" << tgtrow_len << "): " ;
    for (CoinBigIndex krow = tgtrs ; krow < tgtre ; ++krow) {
      const int j = colIndices[krow] ;
      const double arj = rowCoeffs[krow] ;
      std::cout
	<< "x(" << j << ") = " << arj << " (" << colLengths[j] << ") " ;
    }
    std::cout << std::endl ;
#   endif

/*
  Sort the target row for efficiency when doing elimination.
*/
      CoinSort_2(colIndices+tgtrs,colIndices+tgtre,rowCoeffs+tgtrs) ;
/*
  Get down to the business of substituting for tgtcol in the entangled rows.
  Open a loop to walk the target column. We walk the saved column because the
  bulk store can change as we work. We don't want to repeat or miss a row.
*/
    for (int colndx = 0 ; colndx < tgtcol_len ; ++colndx) {
      int i = ap->rows[colndx] ;
      if (i == tgtrow) continue ;

      double ait = ap->coeffxs[colndx] ;
      double coeff_factor = -ait/tgtcoeff ;
      
      CoinBigIndex krs = rowStarts[i] ;
      CoinBigIndex kre = krs+rowLengths[i] ;

#     if PRESOLVE_DEBUG > 1
      std::cout
	<< "  subst pre (" << i << ") (" << rowLengths[i] << "): " ;
      for (CoinBigIndex krow = krs ; krow < kre ; ++krow) {
	const int j = colIndices[krow] ;
	const double aij = rowCoeffs[krow] ;
	std::cout
	  << "x(" << j << ") = " << aij << " (" << colLengths[j] << ") " ;
      }
      std::cout << std::endl ;
#     endif

/*
  Sort the row for efficiency and call add_row to do the actual business of
  changing coefficients due to substitution. This has the potential to trigger
  compaction of the row-major bulk store, so update bulk store indices.
*/
      CoinSort_2(colIndices+krs,colIndices+kre,rowCoeffs+krs) ;
      // kill small if wanted
      double tolerance = ((prob->presolveOptions()&0x20000)!=0) ?
	1.0e-9*coeff_factor : 1.0e-12*coeff_factor;
      
      bool outOfSpace = add_row(rowStarts,rlo,acts,rup,rowCoeffs,colIndices,
				rowLengths,rlink,nrows,coeff_factor,tolerance,i,tgtrow,
				x_to_y) ;
      if (outOfSpace)
	throwCoinError("out of memory","CoinImpliedFree::presolve") ;

      krs = rowStarts[i] ;
      kre = krs+rowLengths[i] ;
      tgtrs = rowStarts[tgtrow] ;
      tgtre = tgtrs+rowLengths[tgtrow] ;

#     if PRESOLVE_DEBUG > 1
      std::cout
	<< "  subst aft (" << i << ") (" << rowLengths[i] << "): " ;
      for (CoinBigIndex krow = krs ; krow < kre ; ++krow) {
	const int j = colIndices[krow] ;
	const double aij = rowCoeffs[krow] ;
	std::cout
	  << "x(" << j << ") = " << aij << " (" << colLengths[j] << ") " ;
      }
      std::cout << std::endl ;
#     endif

/*
  Now update the column-major representation from the row-major
  representation. This is easy if the coefficient already exists, but
  painful if there's fillin. presolve_find_row1 will return the index of
  the row in the column vector, or one past the end if it's missing. If the
  coefficient is fill, presolve_expand_col will make sure that there's room in
  the column for one more coefficient. This may require that the column be
  moved in the bulk store, so we need to update kcs and kce.

  Once we're done, a(it) = 0 (i.e., we've eliminated x(t) from row i).
  Physically remove the explicit zero from the row-major representation
  with presolve_delete_from_row.
*/
      for (CoinBigIndex rowndx = 0 ; rowndx < tgtrow_len ; ++rowndx) {
	const CoinBigIndex ktgt = tgtrs+rowndx ;
	const int j = colIndices[ktgt] ;
	CoinBigIndex kcs = colStarts[j] ;
	CoinBigIndex kce = kcs+colLengths[j] ;

	assert(colIndices[krs+x_to_y[rowndx]] == j) ;

	const double coeff = rowCoeffs[krs+x_to_y[rowndx]] ;

	CoinBigIndex kcol = presolve_find_row1(i,kcs,kce,rowIndices) ;
	
	if (kcol < kce) {
	  colCoeffs[kcol] = coeff ;
	} else {
	  outOfSpace = presolve_expand_col(colStarts,colCoeffs,rowIndices,
	  				   colLengths,clink,ncols,j) ;
	  if (outOfSpace)
	    throwCoinError("out of memory","CoinImpliedFree::presolve") ;
	  kcs = colStarts[j] ;
	  kce = kcs+colLengths[j] ;
	  
	  rowIndices[kce] = i ;
	  colCoeffs[kce] = coeff ;
	  colLengths[j]++ ;
	}
      }
      presolve_delete_from_row(i,tgtcol,
      			       rowStarts,rowLengths,colIndices,rowCoeffs) ;
#     if PRESOLVE_DEBUG > 1
      kre-- ;
      std::cout
	<< "  subst fin (" << i << ") (" << rowLengths[i] << "): " ;
      for (CoinBigIndex krow = krs ; krow < kre ; ++krow) {
	const int j = colIndices[krow] ;
	const double aij = rowCoeffs[krow] ;
	std::cout
	  << "x(" << j << ") = " << aij << " (" << colLengths[j] << ") " ;
      }
      std::cout << std::endl ;
#     endif
      
    }
/*
  End of the substitution loop.

  Record the column indices of the target row so we can groom these columns
  later to remove possible explicit zeros.
*/
    CoinMemcpyN(&colIndices[rowStarts[tgtrow]],rowLengths[tgtrow],
    		&zerocols[nzerocols]) ;
    nzerocols += rowLengths[tgtrow] ;
/*
  Remove the target equality from the column- and row-major representations
  Somewhat painful in the colum-major representation.  We have to walk the
  target row in the row-major representation and look up each coefficient
  in the column-major representation.
*/
    for (CoinBigIndex krow = tgtrs ; krow < tgtre ; ++krow) {
      const int j = colIndices[krow] ;
#     if PRESOLVE_DEBUG > 1
      std::cout
        << "  removing row " << tgtrow << " from col " << j << std::endl ;
#     endif
      presolve_delete_from_col(tgtrow,j,
      			       colStarts,colLengths,rowIndices,colCoeffs) ;
      if (colLengths[j] == 0) {
        PRESOLVE_REMOVE_LINK(clink,j) ;
      }
    }
/*
  Finally, physically remove the column from the column-major representation
  and the row from the row-major representation.
*/
    PRESOLVE_REMOVE_LINK(clink, tgtcol) ;
    colLengths[tgtcol] = 0 ;

    PRESOLVE_REMOVE_LINK(rlink, tgtrow) ;
    rowLengths[tgtrow] = 0 ;

    rlo[tgtrow] = 0.0 ;
    rup[tgtrow] = 0.0 ;

#   if PRESOLVE_CONSISTENCY > 0
    presolve_links_ok(prob) ;
    presolve_consistent(prob) ;
#   endif

  }
/*
  That's it, we've processed all the candidate pairs.

  Clear the row used flags.
*/
  for (int i = 0 ; i < nRowsUsed ; i++) prob->unsetRowUsed(rowsUsed[i]) ;
/*
  Trim the array of substitution transforms and queue up objects for postsolve.
  Also groom the problem representation to remove explicit zeros.
*/
  if (nactions) {
#   if PRESOLVE_SUMMARY > 0
    std::cout << "NSUBSTS: " << nactions << std::endl ;
#   endif
    next = new subst_constraint_action(nactions,
				   CoinCopyOfArray(actions,nactions),next) ;
    next = drop_zero_coefficients_action::presolve(prob,zerocols,
    						   nzerocols, next) ;
#   if PRESOLVE_CONSISTENCY > 0
    presolve_links_ok(prob) ;
    presolve_consistent(prob) ;
#   endif
  }

  deleteAction(actions,action*) ;
  delete [] x_to_y ;
  delete [] zerocols ;

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
    << "Leaving subst_constraint_action::presolve, "
    << droppedRows << " rows, " << droppedColumns << " columns dropped" ;
# if COIN_PRESOLVE_TUNING > 0
  std::cout << " in " << thisTime-startTime << "s" ;
# endif
  std::cout << "." << std::endl ;
# endif

  return (next) ;
}


/*
  Undo the substitutions from presolve and reintroduce the target constraint
  and column.
*/
void subst_constraint_action::postsolve(CoinPostsolveMatrix *prob) const
{

# if PRESOLVE_DEBUG > 0 || PRESOLVE_CONSISTENCY > 0
# if PRESOLVE_DEBUG > 0
  std::cout
    << "Entering subst_constraint_action::postsolve, "
    << nactions_ << " constraints to process." << std::endl ;
# endif
  int ncols = prob->ncols_ ;
  char *cdone = prob->cdone_ ;
  char *rdone = prob->rdone_ ;
  const double ztolzb = prob->ztolzb_ ;

  presolve_check_threads(prob) ;
  presolve_check_free_list(prob) ;
  presolve_check_reduced_costs(prob) ;
  presolve_check_duals(prob) ;
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
/*
  Rim vectors, solution, reduced costs, duals, row activity.
*/
  double *rlo = prob->rlo_ ;
  double *rup = prob->rup_ ;
  double *cost = prob->cost_ ;
  double *sol = prob->sol_ ;
  double *rcosts = prob->rcosts_ ;
  double *acts = prob->acts_ ;
  double *rowduals = prob->rowduals_ ;

  CoinBigIndex *link = prob->link_ ;
  CoinBigIndex &free_list = prob->free_list_ ;

  const double maxmin = prob->maxmin_ ;

  const action *const actions = actions_ ;
  const int nactions = nactions_ ;

/*
  Open the main loop to step through the postsolve objects.

  First activity is to unpack the postsolve object. We have the target
  column and row indices, the full target column, and complete copies of
  all entangled rows (column indices, coefficients, lower and upper bounds).
  There may be a vector of objective coefficients which we'll get to later.
*/
  for (const action *f = &actions[nactions-1] ; actions <= f ; f--) {
    const int tgtcol = f->col ;
    const int tgtrow = f->rowy ;

    const int tgtcol_len = f->nincol ;
    const double *tgtcol_coeffs = f->coeffxs ;

    const int *entngld_rows = f->rows ;
    const int *entngld_lens = f->ninrowxs ;
    const int *entngld_colndxs = f->rowcolsxs ;
    const double *entngld_colcoeffs = f->rowelsxs ;
    const double *entngld_rlos = f->rlos ;
    const double *entngld_rups = f->rups ;
    const double *costs = f->costsx ;

#   if PRESOLVE_DEBUG > 0 || PRESOLVE_CONSISTENCY > 0
#   if PRESOLVE_DEBUG > 1
    std::cout
      << "  reintroducing column x(" << tgtcol << ") and row " << tgtrow ;
      if (costs) std::cout << ", nonzero costs" ;
      std::cout << "." << std::endl ;
#   endif
/*
  We're about to reintroduce the target row and column; empty stubs should be
  present. All other rows should already be present.
*/
    PRESOLVEASSERT(cdone[tgtcol] == DROP_COL) ;
    PRESOLVEASSERT(colLengths[tgtcol] == 0) ;
    PRESOLVEASSERT(rdone[tgtrow] == DROP_ROW) ;
    for (int cndx = 0 ; cndx < tgtcol_len ; ++cndx) {
      if (entngld_rows[cndx] != tgtrow)
	PRESOLVEASSERT(rdone[entngld_rows[cndx]]) ;
    }
/*
  In a postsolve matrix, we can't just check that the length of the row is
  zero. We need to look at all columns and confirm its absence.
*/
    for (int j = 0 ; j < ncols ; ++j) {
      if (colLengths[j] > 0 && cdone[j]) {
        const CoinBigIndex kcs = colStarts[j] ;
        const int lenj = colLengths[j] ;
	CoinBigIndex krow =
	  presolve_find_row3(tgtrow,kcs,lenj,rowIndices,link) ;
	if (krow >= 0) {
	  std::cout
	    << "  BAD COEFF! row " << tgtrow << " present in column " << j
	    << " before reintroduction; a(" << tgtrow << "," << j
	    << ") = " << colCoeffs[krow] << "; x(" << j << ") = " << sol[j]
	    << "; cdone " << static_cast<int>(cdone[j]) << "." << std::endl ;
	}
      }
    }
#   endif

/*
  Find the copy of the target row. Restore the upper and lower bounds
  of entangled rows while we're looking. Recall that the target row is
  an equality.
*/
    int tgtrow_len = -1 ;
    const int *tgtrow_colndxs = NULL ;
    const double *tgtrow_coeffs = NULL ;
    double tgtcoeff = 0.0 ;
    double tgtrhs = 1.0e50 ;

    int nel = 0 ;
    for (int cndx = 0 ; cndx < tgtcol_len ; ++cndx) {
      int i = entngld_rows[cndx] ;
      rlo[i] = entngld_rlos[cndx] ;
      rup[i] = entngld_rups[cndx] ;
      if (i == tgtrow) {
	tgtrow_len = entngld_lens[cndx] ;
	tgtrow_colndxs = &entngld_colndxs[nel] ;
	tgtrow_coeffs  = &entngld_colcoeffs[nel] ;
	tgtcoeff = tgtcol_coeffs[cndx] ;
	tgtrhs = rlo[i] ;
      }
      nel += entngld_lens[cndx] ;
    }
/*
  Solve the target equality to find the solution for the eliminated col.
  tgtcol is present in tgtrow_colndxs, so initialise sol[tgtcol] to zero
  to make sure it doesn't contribute.

  If we're debugging, check that the result is within bounds.
*/
    double tgtexp = tgtrhs ;
    sol[tgtcol] = 0.0 ;
    for (int ndx = 0 ; ndx < tgtrow_len ; ++ndx) {
      int j = tgtrow_colndxs[ndx] ;
      double coeffj = tgtrow_coeffs[ndx] ;
      tgtexp -= coeffj*sol[j] ;
    }
    sol[tgtcol] = tgtexp/tgtcoeff ;

#   if PRESOLVE_DEBUG > 0
    double *clo = prob->clo_ ;
    double *cup = prob->cup_ ;

    if (!(sol[tgtcol] > (clo[tgtcol]-ztolzb) &&
	  (cup[tgtcol]+ztolzb) > sol[tgtcol])) {
      std::cout
	<< "BAD SOL: x(" << tgtcol << ") " << sol[tgtcol]
	<< "; lb " << clo[tgtcol] << "; ub " << cup[tgtcol] << "."
	<< std::endl ;
    }
#   endif
/*
  Now restore the original entangled rows. We first delete any columns present
  in tgtrow. This will remove any fillin, but may also remove columns that
  were originally present in both the entangled row and the target row.

  Note that even cancellations (explicit zeros) are present at this
  point --- in presolve, they were removed after the substition transform
  completed, hence they're already restored. What isn't present is the target
  column, which is deleted as part of the transform.
*/
    {
#     if PRESOLVE_DEBUG > 2
      std::cout << "    removing coefficients:" ;
#     endif
      for (int rndx = 0 ; rndx < tgtrow_len ; ++rndx) {
	int j = tgtrow_colndxs[rndx] ;
	if (j != tgtcol)
	  for (int cndx = 0 ; cndx < tgtcol_len ; ++cndx) {
	    if (entngld_rows[cndx] != tgtrow) {
#             if PRESOLVE_DEBUG > 2
	      std::cout << " a(" << entngld_rows[cndx] << "," << j << ")" ;
#             endif
	      presolve_delete_from_col2(entngld_rows[cndx],j,colStarts,
				      colLengths,rowIndices,link,&free_list) ;
	    }
	  }
      }
#     if PRESOLVE_DEBUG > 2
      std::cout << std::endl ;
#     endif
#     if PRESOLVE_CONSISTENCY > 0
      presolve_check_threads(prob) ;
      presolve_check_free_list(prob) ;
#     endif
/*
  Next we restore the original coefficients. The outer loop walks tgtcol;
  cols_i and coeffs_i are advanced as we go to point to each entangled
  row. The inner loop walks the entangled row and restores the row's
  coefficients. Tgtcol is handled as any other column. Skip tgtrow, we'll
  do it below.

  Since we don't have a row-major representation, we have to look for a(i,j)
  from entangled row i in the existing column j. If we find a(i,j), simply
  update it (and a(tgtrow,j) should not exist). If we don't find a(i,j),
  introduce it (and a(tgtrow,j) should exist).

  Recalculate the row activity while we're at it.
*/
#     if PRESOLVE_DEBUG > 2
      std::cout << "    restoring coefficients:" ;
#     endif

      colLengths[tgtcol] = 0 ;
      const int *cols_i = entngld_colndxs ;
      const double *coeffs_i = entngld_colcoeffs ;

      for (int cndx = 0 ; cndx < tgtcol_len ; ++cndx) {
	const int leni = entngld_lens[cndx] ;
	const int i = entngld_rows[cndx] ;

	if (i != tgtrow) {
	  double acti = 0.0 ;
	  for (int rndx = 0 ; rndx < leni ; ++rndx) {
	    const int j = cols_i[rndx] ;
	    CoinBigIndex kcoli =
	      presolve_find_row3(i,colStarts[j],
	      			 colLengths[j],rowIndices,link) ;
	    if (kcoli != -1) {
#             if PRESOLVE_DEBUG > 2
	      std::cout << " u a(" << i << "," << j << ")" ;
	      PRESOLVEASSERT(presolve_find_col1(j,0,tgtrow_len,
	      					tgtrow_colndxs) == tgtrow_len) ;
#	      endif
	      colCoeffs[kcoli] = coeffs_i[rndx] ;
	    } else {
#             if PRESOLVE_DEBUG > 2
	      std::cout << " f a(" << i << "," << j << ")" ;
	      PRESOLVEASSERT(presolve_find_col1(j,0,tgtrow_len,
						tgtrow_colndxs) < tgtrow_len) ;
#	      endif
	      CoinBigIndex kk = free_list ;
	      assert(kk >= 0 && kk < prob->bulk0_) ;
	      free_list = link[free_list] ;
	      link[kk] = colStarts[j] ;
	      colStarts[j] = kk ;
	      colCoeffs[kk] = coeffs_i[rndx] ;
	      rowIndices[kk] = i ;
	      ++colLengths[j] ;
	    }
	    acti += coeffs_i[rndx]*sol[j] ;
	  }
	  acts[i] = acti ;
	}
	cols_i += leni ;
	coeffs_i += leni ;
      }
#     if PRESOLVE_DEBUG > 2
      std::cout << std::endl ;
#     endif
#     if PRESOLVE_CONSISTENCY > 0
      presolve_check_threads(prob) ;
      presolve_check_free_list(prob) ;
#     endif
/*
  Restore tgtrow. Arguably we could to this in the previous loop, but we'd do
  a lot of unnecessary work. By construction, the target row is tight.
*/
#     if PRESOLVE_DEBUG > 2
      std::cout << "    restoring row " << tgtrow << ":" ;
#     endif

      for (int rndx = 0 ; rndx < tgtrow_len ; ++rndx) {
	int j = tgtrow_colndxs[rndx] ;
#       if PRESOLVE_DEBUG > 2
	std::cout << " a(" << tgtrow << "," << j << ")" ;
#       endif
	CoinBigIndex kk = free_list ;
	assert(kk >= 0 && kk < prob->bulk0_) ;
	free_list = link[free_list] ;
	link[kk] = colStarts[j] ;
	colStarts[j] = kk ;
	colCoeffs[kk] = tgtrow_coeffs[rndx] ;
	rowIndices[kk] = tgtrow ;
	++colLengths[j] ;
      }
      acts[tgtrow] = tgtrhs ;

#     if PRESOLVE_DEBUG > 2
      std::cout << std::endl ;
#     endif
#     if PRESOLVE_CONSISTENCY > 0
      presolve_check_threads(prob) ;
      presolve_check_free_list(prob) ;
#     endif
    }
/*
  Restore original cost coefficients, if necessary.
*/
    if (costs) {
      for (int ndx = 0 ; ndx < tgtrow_len ; ++ndx) {
	cost[tgtrow_colndxs[ndx]] = costs[ndx] ;
      }
    }
/*
  Calculate the reduced cost for the column absent any contribution from
  tgtrow, then set the dual for tgtrow so that the reduced cost of tgtcol
  is zero.
*/
    double dj = maxmin*cost[tgtcol] ;
    rowduals[tgtrow] = 0.0 ;
    for (int cndx = 0 ; cndx < tgtcol_len ; ++cndx) {
      int i = entngld_rows[cndx] ;
      double coeff = tgtcol_coeffs[cndx] ;
      dj -= rowduals[i]*coeff ;
    }
    rowduals[tgtrow] = dj/tgtcoeff ;
    rcosts[tgtcol] = 0.0 ;
    if (rowduals[tgtrow] > 0)
      prob->setRowStatus(tgtrow,CoinPrePostsolveMatrix::atUpperBound) ;
    else
      prob->setRowStatus(tgtrow,CoinPrePostsolveMatrix::atLowerBound) ;
    prob->setColumnStatus(tgtcol,CoinPrePostsolveMatrix::basic) ;

#   if PRESOLVE_DEBUG > 2
    std::cout
      << "  row " << tgtrow << " "
      << prob->rowStatusString(prob->getRowStatus(tgtrow))
      << " dual " << rowduals[tgtrow] << std::endl ;
    std::cout
      << "  col " << tgtcol << " "
      << prob->columnStatusString(prob->getColumnStatus(tgtcol))
      << " dj " << dj << std::endl ;
#   endif

#   if PRESOLVE_DEBUG > 0 || PRESOLVE_CONSISTENCY > 0
    cdone[tgtcol] = SUBST_ROW ;
    rdone[tgtrow] = SUBST_ROW ;
#   endif
  }

# if PRESOLVE_DEBUG > 0 || PRESOLVE_CONSISTENCY > 0
  presolve_check_threads(prob) ;
  presolve_check_free_list(prob) ;
  presolve_check_reduced_costs(prob) ;
  presolve_check_duals(prob) ;
  presolve_check_sol(prob,2,2,2) ;
  presolve_check_nbasic(prob) ;
# if PRESOLVE_DEBUG > 0
  std::cout << "Leaving subst_constraint_action::postsolve." << std::endl ;
# endif
# endif

  return ;
}


/*
  Next time someone builds this code on Windows, check to see if deleteAction
  is still necessary.   -- lh, 121114 --
*/
subst_constraint_action::~subst_constraint_action()
{
  const action *actions = actions_ ;

  for (int i = 0 ; i < nactions_ ; ++i) {
    delete [] actions[i].rows ;
    delete [] actions[i].rlos ;
    delete [] actions[i].rups ;
    delete [] actions[i].coeffxs ;
    delete [] actions[i].ninrowxs ;
    delete [] actions[i].rowcolsxs ;
    delete [] actions[i].rowelsxs ;


    //delete [](double*)actions[i].costsx ;
    deleteAction(actions[i].costsx,double*) ;
  }

  // Must add cast to placate MS compiler
  //delete [] (subst_constraint_action::action*)actions_ ;
  deleteAction(actions_,subst_constraint_action::action*) ;
}
