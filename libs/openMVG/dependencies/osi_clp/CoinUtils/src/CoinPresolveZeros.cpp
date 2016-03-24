/* $Id: CoinPresolveZeros.cpp 1561 2012-11-24 00:32:16Z lou $ */
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include <stdio.h>
#include <math.h>

#include "CoinHelperFunctions.hpp"
#include "CoinPresolveMatrix.hpp"
#include "CoinPresolveZeros.hpp"

#if PRESOLVE_DEBUG > 0 || PRESOLVE_CONSISTENCY > 0
#include "CoinPresolvePsdebug.hpp"
#endif

namespace {	// begin unnamed file-local namespace

/*
  Count the number of zeros in the columns listed in checkcols. Trim back
  checkcols to just the columns with zeros.
*/
int count_col_zeros (int &ncheckcols, int *checkcols,
		     const CoinBigIndex *mcstrt, const double *colels,
		     const int *hincol)
{
  int nzeros = 0 ;
  int zeroCols = 0 ;

  for (int ndx = 0 ; ndx < ncheckcols ; ndx++) {
    const int j = checkcols[ndx] ;
    const CoinBigIndex kcs = mcstrt[j] ;
    const CoinBigIndex kce = kcs+hincol[j] ;
    int zerosj = 0 ;

    for (CoinBigIndex kcol = kcs ; kcol < kce ; ++kcol) {
      if (fabs(colels[kcol]) < ZTOLDP) {
	zerosj++ ;
      }
    }
    if (zerosj) {
      checkcols[zeroCols++] = j ;
      nzeros += zerosj ;
    }
  }
  ncheckcols = zeroCols ;
  return (nzeros) ;
}

/*
  Count the number of zeros. Intended for the case where all columns 0 ..
  ncheckcols should be scanned. Typically ncheckcols is the number of columns
  in the matrix. Leave a list of columns with explicit zeros at the front of
  checkcols, with the count in ncheckcols.
*/
int count_col_zeros2 (int &ncheckcols, int *checkcols,
		      const CoinBigIndex *mcstrt, const double *colels,
		      const int *hincol)
{
  int nzeros = 0 ;
  int zeroCols = 0 ;

  for (int j = 0 ; j < ncheckcols ; j++) {
    const CoinBigIndex kcs = mcstrt[j] ;
    CoinBigIndex kce = kcs+hincol[j] ;

    int zerosj = 0 ;
    for (CoinBigIndex k = kcs ; k < kce ; ++k) {
      if (fabs(colels[k]) < ZTOLDP) {
        zerosj++ ;
      }
    }
    if (zerosj) {
      checkcols[zeroCols++] = j ;
      nzeros += zerosj ;
    }
  }
  ncheckcols = zeroCols ;
  return (nzeros) ;
}

/*
  Searches the cols in checkcols for zero entries.
  Creates a dropped_zero entry for each one; doesn't check for out-of-memory.
  Returns number of zeros found.
*/
int drop_col_zeros (int ncheckcols, const int *checkcols,
		    const CoinBigIndex *mcstrt, double *colels, int *hrow,
		    int *hincol, presolvehlink *clink,
		    dropped_zero *actions)
{
  int nactions = 0 ;

/*
  Physically remove explicit zeros. To maintain loose packing, move the
  element at the end of the column into the empty space. Of course, that
  element could also be zero, so recheck the position.
*/
  for (int i = 0 ; i < ncheckcols ; i++) {
    int col = checkcols[i] ;
    const CoinBigIndex kcs = mcstrt[col] ;
    CoinBigIndex kce = kcs+hincol[col] ;

#   if PRESOLVE_DEBUG > 1
    std::cout << "  scanning column " << col << "..." ;
#   endif

    for (CoinBigIndex k = kcs ; k < kce ; ++k) {
      if (fabs(colels[k]) < ZTOLDP) {
	actions[nactions].col = col ;
	actions[nactions].row = hrow[k] ;

#       if PRESOLVE_DEBUG > 2
	std::cout << " (" << hrow[k] << "," << col << ") " ;
#       endif

	nactions++ ;

	kce-- ;
	colels[k] = colels[kce] ;
	hrow[k] = hrow[kce] ;
	hincol[col]-- ;

	--k ;
      }
    }
#   if PRESOLVE_DEBUG > 1
    if (nactions)
      std::cout << std::endl ;
#   endif

    if (hincol[col] == 0)
      PRESOLVE_REMOVE_LINK(clink,col) ;
  }
  return (nactions) ;
}

/*
  Scan rows to remove explicit zeros.

  This will, in general, scan a row once for each explicit zero in the row,
  but will remove all zeros the first time through. It's tempting to try and
  do something about this, but given the relatively small number of explicit
  zeros created by presolve, the bookkeeping likely exceeds the gain.
*/
void drop_row_zeros(int nzeros, const dropped_zero *zeros,
		    const CoinBigIndex *mrstrt, double *rowels, int *hcol,
		    int *hinrow, presolvehlink *rlink)
{
  for (int i = 0 ; i < nzeros ; i++) {
    int row = zeros[i].row ;
    const CoinBigIndex krs = mrstrt[row] ;
    CoinBigIndex kre = krs+hinrow[row] ;

#   if PRESOLVE_DEBUG > 2
    std::cout
      << "  scanning row " << row <<  " for a(" << row << "," << zeros[i].col
      << ") ..." ;
    bool found = false ;
#   endif

    for (CoinBigIndex k = krs ; k < kre ; k++) {
      if (fabs(rowels[k]) < ZTOLDP) {

#       if PRESOLVE_DEBUG > 2
	std::cout << " (" << row << "," << hcol[k] << ") " ;
        found = true ;
#       endif

	rowels[k] = rowels[kre-1] ;
	hcol[k] = hcol[kre-1] ;
	kre-- ;
	hinrow[row]-- ;

	--k ;
      }
    }

#   if PRESOLVE_DEBUG > 2
    if (found)
      std::cout
        << " found; " << hinrow[row] << " coeffs remaining." << std::endl ;
#   endif

    if (hinrow[row] == 0)
      PRESOLVE_REMOVE_LINK(rlink,row) ;
  }
}

}	// end unnamed file-local namespace

/*
  Scan the columns listed in checkcols for explicit zeros and eliminate them.

  For the special case where all columns should be scanned (ncheckcols ==
  prob->ncols_), there is no need to initialise checkcols.
*/
const CoinPresolveAction
  *drop_zero_coefficients_action::presolve (CoinPresolveMatrix *prob,
					    int *checkcols,
					    int ncheckcols,
					    const CoinPresolveAction *next)
{
  double *colels = prob->colels_ ;
  int *hrow = prob->hrow_ ;
  CoinBigIndex *mcstrt = prob->mcstrt_ ;
  int *hincol = prob->hincol_ ;
  presolvehlink *clink = prob->clink_ ;
  presolvehlink *rlink = prob->rlink_ ;

# if PRESOLVE_DEBUG > 0 || PRESOLVE_CONSISTENCY > 0
# if PRESOLVE_DEBUG > 0
  std::cout
    << "Entering drop_zero_action::presolve, " << ncheckcols
    << " columns to scan." << std::endl ;
# endif
  presolve_consistent(prob) ;
  presolve_links_ok(prob) ;
  presolve_check_sol(prob) ;
  presolve_check_nbasic(prob) ;
# endif

/*
  Scan for zeros.
*/
  int nzeros ;
  if (ncheckcols == prob->ncols_) {
    nzeros = count_col_zeros2(ncheckcols,checkcols,mcstrt,colels,hincol) ;
  } else {
    nzeros = count_col_zeros(ncheckcols,checkcols,mcstrt,colels,hincol) ;
  }
# if PRESOLVE_DEBUG > 1
  std::cout
    << "  drop_zero_action: " << nzeros << " zeros in " << ncheckcols
    << " columns." << std::endl ;
# endif
/*
  Do we have zeros to remove? If so, get to it.
*/
  if (nzeros != 0) {
/*
  We have zeros to remove. drop_col_zeros will scan the columns and remove
  zeros, adding records of the dropped entries to zeros. The we need to clean
  the row representation.
*/
    dropped_zero *zeros = new dropped_zero[nzeros] ;

    nzeros = drop_col_zeros(ncheckcols,checkcols,mcstrt,colels,
    			    hrow,hincol,clink,zeros) ;

    double *rowels = prob->rowels_ ;
    int *hcol = prob->hcol_ ;
    CoinBigIndex *mrstrt = prob->mrstrt_ ;
    int *hinrow = prob->hinrow_ ;
    drop_row_zeros(nzeros,zeros,mrstrt,rowels,hcol,hinrow,rlink) ;
    next = new drop_zero_coefficients_action(nzeros,zeros,next) ;
  }

# if PRESOLVE_CONSISTENCY > 0 || PRESOLVE_DEBUG > 0
  presolve_consistent(prob) ;
  presolve_links_ok(prob) ;
  presolve_check_sol(prob) ;
  presolve_check_nbasic(prob) ;
# endif
# if PRESOLVE_DEBUG > 0 || COIN_PRESOLVE_TUNING > 0
  std::cout
    << "Leaving drop_zero_action::presolve, dropped " << nzeros
    << " zeroes." << std::endl ;
# endif

  return (next) ;
}


/*
  This wrapper initialises checkcols for the case where the entire matrix
  should be scanned. Typically used from the presolve driver as part of final
  cleanup.
*/
const CoinPresolveAction
  *drop_zero_coefficients (CoinPresolveMatrix *prob,
			   const CoinPresolveAction *next)
{
  int ncheck = prob->ncols_ ;
  int *checkcols = new int[ncheck] ;

  if (prob->anyProhibited()) {
    ncheck = 0 ;
    for (int i = 0 ; i < prob->ncols_ ; i++)
      if (!prob->colProhibited(i))
	checkcols[ncheck++] = i ;
  }

  const CoinPresolveAction *retval =
      drop_zero_coefficients_action::presolve(prob,checkcols,ncheck,next) ;

  delete [] checkcols ;
  return (retval) ;
}

void drop_zero_coefficients_action::postsolve(CoinPostsolveMatrix *prob) const
{
  const int nzeros = nzeros_ ;
  const dropped_zero *const zeros = zeros_ ;

  double *colels = prob->colels_ ;
  int *hrow = prob->hrow_ ;
  CoinBigIndex *mcstrt = prob->mcstrt_ ;
  int *hincol = prob->hincol_ ;
  int *link = prob->link_ ;
  CoinBigIndex &free_list = prob->free_list_ ;

  for (const dropped_zero *z = &zeros[nzeros-1] ; zeros <= z ; z--) {
    const int i = z->row ;
    const int j = z->col ;

    CoinBigIndex k = free_list ;
    assert(k >= 0 && k < prob->bulk0_) ;
    free_list = link[free_list] ;
    hrow[k] = i ;
    colels[k] = 0.0 ;
    link[k] = mcstrt[j] ;
    mcstrt[j] = k ;

    hincol[j]++ ;
  }

# if PRESOLVE_CONSISTENCY > 0
  presolve_check_free_list(prob) ;
# endif

}
