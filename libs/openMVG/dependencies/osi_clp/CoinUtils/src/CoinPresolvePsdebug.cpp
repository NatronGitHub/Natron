/* $Id: CoinPresolvePsdebug.cpp 1560 2012-11-24 00:29:01Z lou $ */
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include <new>
#include <stdio.h>
#include <math.h>

#include "CoinPresolveMatrix.hpp"
#include "CoinHelperFunctions.hpp"

/*
  \file

  This file contains a number of routines that are useful when doing
  serious debugging but unneeded otherwise. It also contains the methods
  that implement the CoinPresolveMonitor class.

  Presumably if you're deep enough into presolve to need these, you're
  willing to scan the file to see what can be done. See also the Presolve
  Debug Functions module in the doxygen doc'n and CoinPresolvePsdebug.hpp.

  The general approach for the matrix consistency routines is that the
  routines return void and abort when they find a problem. The routines
  that check the basis and solution complain loudly but do not abort.

  NOTE: The definitions for PRESOLVE_CONSISTENCY and PRESOLVE_DEBUG MUST
	BE CONSISTENT across all CoinPresolve source files AND OsiPresolve
	AND ClpPresolve AND OsiDylpPresolve (assuming any of the latter
	are relevant to your code).  Otherwise, at best you'll get garbage
	output. More likely, you'll get a core dump. Resist the temptation
	to define these constants in individual files. In particular,
	cdone and rdone will NOT be consistently maintained during postsolve.

  Hack away as your needs dictate.
*/


/*
  Integrity checking routines for the (loosely) packed matrices of a
  CoinPresolveMatrix object. Some routines work on column-major and row-major
  reps separately, others do cross-checking.
*/

namespace { // begin unnamed file-local namespace

#if PRESOLVE_DEBUG || PRESOLVE_CONSISTENCY
/*
  Check for duplicate entries in a major vector by walking the vector. For
  each coefficient, use presolve_find_minor1 to search the remainder of the
  major vector for an entry with the same minor index. We don't want to find
  anything.
*/
  
void no_majvec_dups (const char *majdones, const CoinBigIndex *majstrts,
		     const int *minndxs, const int *majlens, int nmaj)

{ for (int maj = 0 ; maj < nmaj ; maj++) 
  { if ((!majdones || majdones[maj]) && majlens[maj] > 0)
    { CoinBigIndex ks = majstrts[maj] ;
      CoinBigIndex ke = ks+majlens[maj] ;
      for (CoinBigIndex k = ks ; k < ke ; k++)
      { 
/*
  Assert we fell off the end of the major vector without finding the entry. 
*/
	PRESOLVEASSERT(presolve_find_minor1(minndxs[k],k+1,
					    ke,minndxs) == ke) ; } } }
  return ; }

/*
  As the name implies: scan for explicit zeros.
*/
void check_majvec_nozeros (const CoinBigIndex *majstrts, const double *majels,
			   const int *majlens, int nmaj)

{ for (int maj = 0 ; maj < nmaj ; maj++) 
  { if (majlens[maj] > 0)
    { CoinBigIndex ks = majstrts[maj] ;
      CoinBigIndex ke = ks+majlens[maj] ;
      for (CoinBigIndex k = ks ; k < ke ; k++)
      { PRESOLVEASSERT(fabs(majels[k]) > ZTOLDP) ; } } }

  return ; }

/*
  Integrity checks for the linked lists that indicate major vector ordering
  in the bulk storage area (minor index and coefficient arrays).
 */
void links_ok (presolvehlink *majlink, int *majstrts, int *majlens, int nmaj)

{ int maj ;

/*
  Confirm link integrity. Vectors of length 0 should not be part of the chain.
*/
  for (maj = 0 ; maj < nmaj ; maj++)
  { int pre = majlink[maj].pre ;
    int suc = majlink[maj].suc ;

    if (majlens[maj] == 0)
    { PRESOLVEASSERT(pre == NO_LINK && suc == NO_LINK) ; }
    if (pre != NO_LINK)
    { PRESOLVEASSERT(0 <= pre && pre <= nmaj) ;
      PRESOLVEASSERT(majlink[pre].suc == maj) ; }
    if (suc != NO_LINK)
    { PRESOLVEASSERT(0 <= suc && suc <= nmaj) ;
      PRESOLVEASSERT(majlink[suc].pre == maj) ; } }
/*
  There must be a first vector.
*/
  for (maj = 0 ; maj < nmaj ; maj++) 
  { if (majlink[maj].pre == NO_LINK)
      break ; }
  PRESOLVEASSERT(nmaj == 0 || maj < nmaj) ;
/*
  The order of the linked list should match the ordering indicated by the
  major vector start & length arrays.
*/
  while (maj != NO_LINK)
  { if (majlink[maj].suc != NO_LINK) 
    { PRESOLVEASSERT(majstrts[maj]+majlens[maj] <=
				majstrts[majlink[maj].suc]) ; }
    maj = majlink[maj].suc ; }

  return ; }


/*
 matrix_consistent checks that an entry is in the column-major representation
 if it is in the row-major representation.  If testvals is non-zero, it also
 checks that their values are the same.

 By doing the appropriate swaps of column- and row-major data structures in
 the parameter list, we can check that an entry is in the row-major
 representation if it's in the column-major representation.

 I can't see any nice way to rename the parameters (majmajstrt? minmajstrt?).

 Original comment:  ``Note that there may be entries in a row that correspond
 to empty columns and vice-versa.'' To which a previous browser had commented
 ``HUH???''. And I agree. -- lh, 040907 --
*/

void matrix_consistent (const CoinBigIndex *mrstrt, const int *hinrow,
			const int *hcol, const double *rowels,
			const CoinBigIndex *mcstrt, const int *hincol,
			const int *hrow, const double *colels,
			int nrows, int testvals,
			const char *ROW, const char *COL)
{
  for (int irow = 0 ; irow < nrows ; irow++) {
    if (hinrow[irow] > 0) {
      const CoinBigIndex krs = mrstrt[irow] ;
      const CoinBigIndex kre = krs+hinrow[irow] ;

      for (CoinBigIndex k = krs ; k < kre ; k++) {
	int jcol = hcol[k] ;
	const CoinBigIndex kcs = mcstrt[jcol] ;
	const CoinBigIndex kce = kcs+hincol[jcol] ;

	CoinBigIndex kk = presolve_find_row1(irow,kcs,kce,hrow) ;
	if (kk == kce) {
	  std::cout
	    << "MATRIX INCONSISTENT:  can't find " << ROW << " " << irow
	    << " in " << COL << " " << jcol << std::endl ;
	  fflush(stdout) ;
	  abort() ;
	}
	if (testvals && colels[kk] != rowels[k]) {
	  std::cout
	    << "MATRIX INCONSISTENT: values differ for " << ROW << " " << irow
	    << " and " << COL << " " << jcol << std::endl ;
	  fflush(stdout) ;
	  abort() ;
	}
      }
    }
  }
}
#endif
} // end unnamed file-local namespace

/*
  Utilizes matrix_consistent to check for equivalence of the column- and
  row-major representations. Checks for presence of coefficients in the
  column-major matrix, given presence in the row-major matrix, then checks
  for presence in the row-major matrix given presence in the column-major
  matrix. If testvals == true (default), the check also tests that the
  coefficients have equal value.
  
  See further comments with matrix_consistent.
*/

# if PRESOLVE_CONSISTENCY
void presolve_consistent(const CoinPresolveMatrix *preObj, bool testvals)
{
  matrix_consistent(preObj->mrstrt_,preObj->hinrow_,preObj->hcol_,
		    preObj->rowels_,
		    preObj->mcstrt_,preObj->hincol_,preObj->hrow_,
		    preObj->colels_,
		    preObj->nrows_,testvals,"row","col") ;
  matrix_consistent(preObj->mcstrt_,preObj->hincol_,preObj->hrow_,
		    preObj->colels_,
		    preObj->mrstrt_,preObj->hinrow_,preObj->hcol_,
		    preObj->rowels_, 
		    preObj->ncols_,testvals,"col","row") ;
}

/*
  Check the column- and/or row-major matrices for duplicates. By default, both
  will be checked.
*/
  
void presolve_no_dups (const CoinPresolveMatrix *preObj,
		       bool doCol, bool doRow)

{
  if (doCol)
  { no_majvec_dups(0,preObj->mcstrt_,preObj->hrow_,
		   preObj->hincol_,preObj->ncols_) ; }
  if (doRow)
  { no_majvec_dups(0,preObj->mrstrt_,preObj->hcol_,
		   preObj->hinrow_,preObj->nrows_) ; }

  return ; }


/*
  As the name implies: scan for explicit zeros. By default, both matrices are
  scanned.
*/
void presolve_no_zeros (const CoinPresolveMatrix *preObj,
			bool doCol, bool doRow)
{
  if (doCol)
  { check_majvec_nozeros(preObj->mcstrt_,preObj->colels_,preObj->hincol_,
			 preObj->ncols_) ; }
  if (doRow)
  { check_majvec_nozeros(preObj->mrstrt_,preObj->rowels_,preObj->hinrow_,
			 preObj->nrows_) ; }

  return ; }

/*
  Lazy check on column lengths. Scan the row index array for the column.
  If the relevant row length in the row-major rep is non-zero, assume we're ok.

  Not advertised in CoinPresolvePsdebug.hpp.
*/
void presolve_hincol_ok(const int *mcstrt, const int *hincol,
	       const int *hinrow,
	       const int *hrow, int ncols)
{
  int jcol;

  for (jcol=0; jcol<ncols; jcol++) 
    if (hincol[jcol] > 0) {
      int kcs = mcstrt[jcol];
      int kce = kcs + hincol[jcol];
      int n=0;
      
      int k;
      for (k=kcs; k<kce; k++) {
	int row = hrow[k];
	if (hinrow[row] > 0)
	  n++;
      }
      if (n != hincol[jcol])
	abort();
    }
}

/*
  Integrity checks for the linked lists that indicate major vector ordering
  in the bulk storage area (minor index and coefficient arrays).
 */
void presolve_links_ok (const CoinPresolveMatrix *preObj,
			bool doCol, bool doRow)
{
  if (doCol)
  { links_ok(preObj->clink_,preObj->mcstrt_,
	     preObj->hincol_,preObj->ncols_) ; }
  if (doRow)
  { links_ok(preObj->rlink_,preObj->mrstrt_,
	     preObj->hinrow_,preObj->nrows_) ; }

  return ; }



/*
  Routines to check a threaded matrix from a CoinPostsolve object.
*/

/*
  Check that the column length agrees with the column thread. There must be
  the correct number of coefficients, and the thread must end with the NO_LINK
  marker.
*/
void presolve_check_threads (const CoinPostsolveMatrix *obj)

{ 

  CoinBigIndex *mcstrt = obj->mcstrt_ ;
  int *hincol = obj->hincol_ ;
  CoinBigIndex *link = obj->link_ ;
  char *cdone = obj->cdone_ ;

  int n = obj->ncols0_ ;

/*
  Scan the columns, checking only the ones that have been processed into the
  constraint matrix.
*/
  for (int j = 0 ; j < n ; j++)
  { if (!cdone[j]) continue ;

    int lenj = hincol[j] ;
    int k ;
    for (k = mcstrt[j] ; k != NO_LINK && lenj > 0 ; k = link[k])
    { assert(k >= 0 && k < obj->maxlink_) ;
      lenj-- ; }

    assert(k == NO_LINK && lenj == 0) ; }

  return ; }

/*
  Check the free list. We're looking for gross corruption here. The notion is
  that the free list plus elements in the matrix should add up to the capacity
  of the bulk store.
*/

void presolve_check_free_list (const CoinPostsolveMatrix *obj, bool chkElemCnt)

{ 

  CoinBigIndex k = obj->free_list_ ;
  CoinBigIndex freeCnt = 0 ;
  CoinBigIndex maxlink = obj->maxlink_ ;
  CoinBigIndex *link = obj->link_ ;
/*
  Redundancy in the data structure. These should always be equal.
*/
  assert(maxlink == obj->bulk0_) ;
/*
  Walk the free list portion of link. We should never point outside the bulk
  store. If we ever come across an entry that's less than 0, it had better be
  NO_LINK, the end marker.
*/
  while (k >= 0)
  { assert(k < maxlink) ;
    freeCnt++ ;
    k = link[k] ; }
  assert(k == NO_LINK) ;
/*
  And a final test: elements in the matrix plus free space should equal the
  size of the bulk area. A good thought, but less than practical. Currently
  postsolve doesn't track the number of elements in the matrix. But you might
  find it useful if you're checking a newly constructed postsolve matrix. Even
  then, you need to make sure nelems_ is correct. In the normal scheme of
  things, this requires that somewhere there's a count of elements. Right now,
  drop_empty_cols_action::presolve does this count, and you can get an accurate
  value from the presolve object. assignPresolveToPostsolve will transfer this
  value. Otherwise you're on your own --- your constructor must somehow find
  this count. Using a standard CoinPackedMatrix is another way to get a count.
*/
  if (chkElemCnt)
  { assert(obj->nelems_+freeCnt == maxlink) ; }


  return ; }
#endif


/*
  Routines to check solution and basis composition.
*/

/*
  CoinPostsolveMatrix

  This routine performs two checks on reduced costs held in rcosts_:
    * The value held in rcosts_ is checked against the status of the
      variable. Errors reported as "Bad rcost"
    * The reduced cost is calculated from scratch and compared to the
      value held in rcosts_. Errors reported as "Inacc rcost"

  Remember that postsolve has a schizophrenic attitude about maximisation. All
  transforms assume minimisation, and that's reflected in the reduced costs we
  see here. And you must load duals and reduced costs with the correct sign for
  minimisation. But, as a small courtesy (and a big inconsistency), postsolve
  will negate objective coefficients for you. Hence the rather odd use of
  maxmin.

  The routine is specific to CoinPostsolveMatrix because the reduced cost
  calculation requires traversal of (threaded) matrix columns.

  NOTE: This routine holds static variables. It will detect when the problem
	size changes and reinitialise. If you use presolve debugging over
	multiple problems and you want to be dead sure of reinitialisation,
	use the call presolve_check_reduced_costs(0), which will reinitialise
	and return.
*/
# if PRESOLVE_DEBUG

void presolve_check_reduced_costs (const CoinPostsolveMatrix *postObj)
{

  static bool warned = false ;
  static double *warned_rcosts = 0 ;
  static int allocSize = 0 ;
  static const CoinPostsolveMatrix *lastObj = 0 ;

/*
  Is the client asking for reinitialisation only?
*/
  if (postObj == 0)
  { warned = false ;
    if (warned_rcosts != 0)
    { delete[] warned_rcosts ;
      warned_rcosts = 0 ; }
    allocSize = 0 ;
    lastObj = 0 ;
    return ; }
/*
  *Should* the client have asked for reinitialisation?
*/
  int ncols0 = postObj->ncols0_ ;
  if (allocSize < ncols0 || postObj != lastObj)
  { warned = false ;
    delete[] warned_rcosts ;
    warned_rcosts = 0 ;
    allocSize = 0 ;
    lastObj = postObj ; }

  double *rcosts = postObj->rcosts_ ;

/*
  By tracking values in warned_rcosts, we can produce a single message the
  first time a value is determined to be incorrect.
*/
  if (!warned)
  { warned = true ;
    std::cout
      << "reduced cost" << std::endl ;
    warned_rcosts = new double[ncols0] ;
    CoinZeroN(warned_rcosts,ncols0) ; }

  double *colels = postObj->colels_ ;
  int *hrow = postObj->hrow_ ;
  int *mcstrt = postObj->mcstrt_ ;
  int *hincol = postObj->hincol_ ;
  CoinBigIndex *link = postObj->link_ ;

  double *clo = postObj->clo_ ;
  double *cup = postObj->cup_ ;

  double *dcost	= postObj->cost_ ;

  double *sol = postObj->sol_ ;

  char *cdone = postObj->cdone_ ;
  char *rdone = postObj->rdone_ ;

  const double ztoldj = postObj->ztoldj_ ;
  const double ztolzb = postObj->ztolzb_ ;

  double *rowduals = postObj->rowduals_ ;

  double maxmin = postObj->maxmin_ ;
  std::string strMaxmin((maxmin < 0)?"max":"min") ;
  int checkCol = -1 ;
/*
  Scan all columns, but only check the ones that are marked as having been
  postprocessed.
*/
  for (int j = 0 ; j < ncols0 ; j++)
  { if (cdone[j] == 0) continue ;
    const char *statjstr = postObj->columnStatusString(j) ;
/*
  Check the stored reduced cost for accuracy. See note above w.r.t. maxmin.
*/
    double dj = rcosts[j] ;
    double wrndj = warned_rcosts[j] ;

    { int ndx ;
      CoinBigIndex k = mcstrt[j] ;
      int len = hincol[j] ;
      double chkdj = maxmin*dcost[j] ;
      if (j == checkCol)
        std::cout
	  << "dj for " << j << " is " << dj << " - cost is " << chkdj
	  << std::endl ;
      for (ndx = 0 ; ndx < len ; ndx++)
      { int row = hrow[k] ;
	PRESOLVEASSERT(rdone[row] != 0) ;
	chkdj -= rowduals[row]*colels[k] ;
        if (j == checkCol)
	  std::cout
	    << "row " << row << " coeff " << colels[k] << " dual "
	    << rowduals[row] << " => dj " << chkdj << std::endl ;
	k = link[k] ; }
      if (fabs(dj-chkdj) > ztoldj && wrndj != dj)
      { std::cout
          << "Inacc rcost: " << j << " " << statjstr << " "
	  << strMaxmin << " have " << dj
	  << " should be " << chkdj << " err " << fabs(dj-chkdj)
	  << std::endl ; } }
/*
  Check the stored reduced cost for consistency with the variable's status.
  The cases are
    * basic: (reduced cost) == 0
    * at upper bound and not at lower bound: (reduced cost)*(maxmin) <= 0
    * at lower bound and not at upper bound: (reduced cost)*(maxmin) >= 0
    * not at either bound: any sign is correct (the variable can move either
      way) but superbasic status is sufficiently exotic that it always
      deserves a message. (There should be no superbasic variables at the
      completion of postsolve.)
  As a courtesy, show the reduced cost with the proper sign.
*/
    { double xj = sol[j] ;
      double lj = clo[j] ;
      double uj = cup[j] ;

      if (postObj->columnIsBasic(j))
      { if (fabs(dj) > ztoldj && wrndj != dj)
	{ std::cout
	    << "Bad rcost: " << j << " " << maxmin*dj
	    << " " << statjstr << " " << strMaxmin << std::endl ; } }
      else
      if (fabs(xj-uj) < ztolzb && fabs(xj-lj) > ztolzb)
      { if (dj >= ztoldj && wrndj != dj)
	{ std::cout
	    << "Bad rcost: " << j << " " << maxmin*dj
	    << " " << statjstr << " " << strMaxmin << std::endl ; } }
      else
      if (fabs(xj-lj) < ztolzb && fabs(xj-uj) > ztolzb)
      { if (dj <= -ztoldj && wrndj != dj)
	{ std::cout
	    << "Bad rcost: " << j << " " << maxmin*dj
	    << " " << statjstr << " " << strMaxmin << std::endl ; } }
      else
      if (fabs(xj-lj) > ztolzb && fabs(xj-uj) > ztolzb)
      { if (fabs(dj) > ztoldj && wrndj != dj)
        { std::cout
	    << "Superbasic rcost: " << j << " " << maxmin*dj
	    << " " << statjstr << " " << strMaxmin
	    << " lb "<< lj << " val " << xj << " ub "<< uj << std::endl ; } }
    }

    warned_rcosts[j] = rcosts[j] ; }

}

/*
  CoinPostsolveMatrix

  This routine checks the value and status of the dual variables. It
  checks that the value and status of the dual agree with the row activity.
  Errors are reported as "Bad dual"

  See presolve_check_reduced_costs for an explanation of the use of maxmin.

  Specific to CoinPostsolveMatrix due to the use of rdone. This could be fixed,
  but probably better to clone the function and specialise it for
  CoinPresolveMatrix.
*/

void presolve_check_duals (const CoinPostsolveMatrix *postObj)
{


  int nrows0 = postObj->nrows0_ ;

  double *rowduals = postObj->rowduals_ ;

  double *acts = postObj->acts_ ;
  double *rup = postObj->rup_ ;
  double *rlo = postObj->rlo_ ;

  char *rdone = postObj->rdone_ ;

  const double ztoldj = postObj->ztoldj_ ;
  const double ztolzb = postObj->ztolzb_ ;

  double maxmin = postObj->maxmin_ ;
  std::string strMaxmin((maxmin < 0)?"max":"min") ;

/*
  Scan all processed rows. The rules are as for normal reduced costs, but
  we need to remember the various flips and inversions. In summary, the correct
  situation at optimality (minimisation) is:
    * acts[i] == rup[i] ==> artificial NBLB ==> dual[i] < 0
    * acts[i] == rlo[i] ==> artificial NBUB ==> dual[i] > 0

  We can't say much about the dual for an equality. It can go either way. As a
  courtesy, show the dual with the proper sign.
*/
  for (int i = 0 ; i < nrows0 ; i++)
  { if (rdone[i] == 0) continue ;

    double ui = rup[i] ;
    double li = rlo[i] ;

    if (ui-li < 1.0e-6) continue ;

    double yi = rowduals[i] ;
    double lhsi = acts[i] ;
    const char *statistr = postObj->rowStatusString(i) ;


    if (fabs(lhsi-li) < ztolzb)
    { if (yi < -ztoldj)
      { std::cout
	  << "Bad dual: " << i << " " << maxmin*yi
	  << " " << statistr << " " << strMaxmin << std::endl ; } }
    else
    if (fabs(lhsi-ui) < ztolzb)
    { if (yi > ztoldj)
      { std::cout
	  << "Bad dual: " << i << " " << maxmin*yi
	  << " " << statistr << " " << strMaxmin << std::endl ; } }
    else
    if (li < lhsi && lhsi < ui)
    { if (fabs(yi) > ztoldj)
      { std::cout
	  << "Bad dual: " << i << " " << maxmin*yi
	  << " " << statistr << " " << strMaxmin << std::endl ; } } }
  return ; }



/*
  CoinPresolveMatrix

  This routine will check the primal (column) solution for feasibility and
  status. If there's no column solution (sol_), the routine bails out. If the
  column solution is present, all else is assumed to be present.

  chkColSol:	check colum solution (primal variables)
		0 - checks off
		1 - check for NaN/Inf
	       *2 - check for above/below column bounds
  chkRowAct:	check row solution (evaluate constraint lhs)
		0 - checks off
	       *1 - check for NaN/Inf
		2 - check for inaccuracy, above/below row bounds
  chkStatus:	check for valid status of variables
		0 - checks off
	       *1 - check status of architecturals, if colstat_ exists
	        2 - check status rows, if rowstat_ exists

  In order to check row status we need accurate row activity. Setting
  chkStatus to 2 forces chkRowAct to 2.

  CoinPrePostsolveMatrix plays games with colstat_ and rowstat_, allocating
  them as a single vector, so if colstat_ exists, rowstat_ really should
  exist. Check it anyway; this is a debug method, be robust.

  In general, the presolve transforms are not prepared to properly adjust the
  row activity (reported as `Inacc RSOL'). Postsolve transforms do better. On
  the bright side, the code seems to work just fine without maintaining row
  activity.  You probably don't want to use the level 2 checks for the row
  solution, particularly in presolve.

  With a bit of thought, the various checks could be more cleanly separated
  to require only the minimum information for each check.
*/
void presolve_check_sol (const CoinPresolveMatrix *preObj,
			 int chkColSol, int chkRowAct, int chkStatus)

{
  double *colels = preObj->colels_ ;
  int *hrow = preObj->hrow_ ;
  int *mcstrt = preObj->mcstrt_ ;
  int *hincol = preObj->hincol_ ;
  int *hinrow = preObj->hinrow_ ;

  int n	= preObj->ncols_ ;
  int m = preObj->nrows_ ;

/*
  If there's no column solution, bail out now.
*/
  if (preObj->sol_ == 0) return ;

  double *csol = preObj->sol_ ;
  double *acts = preObj->acts_ ;
  double *clo = preObj->clo_ ;
  double *cup = preObj->cup_ ;
  double *rlo = preObj->rlo_ ;
  double *rup = preObj->rup_ ;

  double tol = preObj->ztolzb_ ;

  if (chkStatus >= 2) chkRowAct = 2 ;

  double *rsol = 0 ;
  if (chkRowAct)
  { rsol = new double[m] ;
    memset(rsol,0,m*sizeof(double)) ; }

/*
  Open a loop to scan each column. For each column, do the following:
    * Update the row solution (lhs value) by adding the contribution from
      this column.
    * Check for bogus values (NaN, infinity)
    * Check for feasibility (value within column bounds)
    * Check that the status of the variable agrees with the value and with the
      lower and upper bounds. Free should have no bounds, superbasic should
      have at least one.
*/
  for (int j = 0 ; j < n ; ++j)
  { CoinBigIndex v = mcstrt[j] ;
    int colLen = hincol[j] ;
    double xj = csol[j] ;
    double lj = clo[j] ;
    double uj = cup[j] ;

    if (chkRowAct >= 1)
    { for (int u = 0 ; u < colLen ; ++u)
      { int i = hrow[v] ;
	  double aij = colels[v] ;
	  v++ ;
	  rsol[i] += aij*xj ; } }

    if (chkColSol > 0)
    { if (CoinIsnan(xj))
      { printf("NaN CSOL: %d  : lb = %g x = %g ub = %g\n",j,lj,xj,uj) ; }
      if (xj <= -PRESOLVE_INF || xj >= PRESOLVE_INF)
      { printf("Inf CSOL: %d  : lb = %g x = %g ub = %g\n",j,lj,xj,uj) ; }
      if (chkColSol > 1)
      { if (xj < lj-tol)
	{ printf("low CSOL: %d  : lb = %g x = %g ub = %g\n",j,lj,xj,uj) ; }
	else
	if (xj > uj+tol)
	{ printf("high CSOL: %d  : lb = %g x = %g ub = %g\n",
		 j,lj,xj,uj) ; } } }
    if (chkStatus && preObj->colstat_)
    { CoinPrePostsolveMatrix::Status statj = preObj->getColumnStatus(j) ;
      switch (statj)
      { case CoinPrePostsolveMatrix::atUpperBound:
	{ if (uj >= PRESOLVE_INF || fabs(xj-uj) > tol)
	  { printf("Bad status CSOL: %d : status atUpperBound : ",j) ;
	    printf("lb = %g x = %g ub = %g\n",lj,xj,uj) ; }
	  break ; }
        case CoinPrePostsolveMatrix::atLowerBound:
	{ if (lj <= -PRESOLVE_INF || fabs(xj-lj) > tol)
	  { printf("Bad status CSOL: %d : status atLowerBound : ",j) ;
	    printf("lb = %g x = %g ub = %g\n",lj,xj,uj) ; }
	  break ; }
        case CoinPrePostsolveMatrix::isFree:
	{ if (lj > -PRESOLVE_INF || uj < PRESOLVE_INF)
	  { printf("Bad status CSOL: %d : status isFree : ",j) ;
	    printf("lb = %g x = %g ub = %g\n",lj,xj,uj) ; }
	  break ; }
        case CoinPrePostsolveMatrix::superBasic:
	{ if (!(lj > -PRESOLVE_INF || uj < PRESOLVE_INF))
	  { printf("Bad status CSOL: %d : status superBasic : ",j) ;
	    printf("lb = %g x = %g ub = %g\n",lj,xj,uj) ; }
	  break ; }
        case CoinPrePostsolveMatrix::basic:
	{ /* Nothing to do here. */
	  break ; }
	default:
	{ printf("Bad status CSOL: %d : status unrecognized : ",j) ;
	  break ; } } } }
/*
  Now check the row solution. acts[i] is what presolve thinks we have, rsol[i]
  is what we've just calculated while scanning the columns. We need only
  check nontrivial rows (i.e., rows with length > 0). For each row,
    * Check for bogus values (NaN, infinity)
    * Check for accuracy (acts == rsol)
    * Check for feasibility (rsol within row bounds)
*/
  tol *=1.0e3;
  if (chkRowAct >= 1)
  { for (int i = 0 ; i < m ; ++i)
    { if (hinrow[i])
      { double lhsi = acts[i] ;
	double evali = rsol[i] ;
	double li = rlo[i] ;
	double ui = rup[i] ;

	if (CoinIsnan(evali) || CoinIsnan(lhsi))
	{ printf("NaN RSOL: %d  : lb = %g eval = %g (expected %g) ub = %g\n",
		 i,li,evali,lhsi,ui) ; }
	if (evali <= -PRESOLVE_INF || evali >= PRESOLVE_INF ||
	    lhsi <= -PRESOLVE_INF || lhsi >= PRESOLVE_INF)
	{ printf("Inf RSOL: %d  : lb = %g eval = %g (expected %g) ub = %g\n",
		 i,li,evali,lhsi,ui) ; }
	if (chkRowAct >= 2)
	{ if (fabs(evali-lhsi) > tol)
	  { printf("Inacc RSOL: %d : lb = %g eval = %g (expected %g) ub = %g\n",
		   i,li,evali,lhsi,ui) ; }
	  if (evali < li-tol || lhsi < li-tol)
	  { printf("low RSOL: %d : lb = %g eval = %g (expected %g) ub = %g\n",
		   i,li,evali,lhsi,ui) ; }
	  else
	  if (evali > ui+tol || lhsi > ui+tol)
	  { printf("high RSOL: %d : lb = %g eval = %g (expected %g) ub = %g\n",
		   i,li,evali,lhsi,ui) ; } }
	if (chkStatus >= 2 && preObj->rowstat_)
	{ CoinPrePostsolveMatrix::Status stati = preObj->getRowStatus(i) ;
	  switch (stati)
	  { case CoinPrePostsolveMatrix::atUpperBound:
	    { if (li <= -PRESOLVE_INF || fabs(lhsi-li) > tol)
	      { printf("Bad status RSOL: %d : status atUpperBound : ",i) ;
		printf("LB = %g lhs = %g UB = %g\n",li,lhsi,ui) ; }
	      break ; }
	    case CoinPrePostsolveMatrix::atLowerBound:
	    { if (ui >= PRESOLVE_INF || fabs(lhsi-ui) > tol)
	      { printf("Bad status RSOL: %d : status atLowerBound : ",i) ;
		printf("LB = %g lhs = %g UB = %g\n",li,lhsi,ui) ; }
	      break ; }
	    case CoinPrePostsolveMatrix::isFree:
	    { if (li > -PRESOLVE_INF || ui < PRESOLVE_INF)
	      { printf("Bad status RSOL: %d : status isFree : ",i) ;
		printf("LB = %g lhs = %g UB = %g\n",li,lhsi,ui) ; }
	      break ; }
	    case CoinPrePostsolveMatrix::superBasic:
	    { printf("Bad status RSOL: %d : status superBasic : ",i) ;
	      printf("LB = %g lhs = %g UB = %g\n",li,lhsi,ui) ;
	      break ; }
	    case CoinPrePostsolveMatrix::basic:
	    { /* Nothing to do here. */
	      break ; }
	    default:
	    { printf("Bad status RSOL: %d : status unrecognized : ",i) ;
	      break ; } } } } }
    delete [] rsol ; }
  return ; }

/*
  CoinPostsolveMatrix

  check_sol overload for CoinPostsolveMatrix. Parameters and functionality
  identical to check_sol immediately above, but we have to remember we're
  working with a threaded column-major representation.
*/
void presolve_check_sol (const CoinPostsolveMatrix *postObj,
			 int chkColSol, int chkRowAct, int chkStatus)

{
  double *colels = postObj->colels_ ;
  int *hrow = postObj->hrow_ ;
  int *mcstrt = postObj->mcstrt_ ;
  int *hincol = postObj->hincol_ ;
  int *link = postObj->link_ ;

  int n	= postObj->ncols_ ;
  int m = postObj->nrows_ ;

  double *csol = postObj->sol_ ;
  double *acts = postObj->acts_ ;
  double *clo = postObj->clo_ ;
  double *cup = postObj->cup_ ;
  double *rlo = postObj->rlo_ ;
  double *rup = postObj->rup_ ;

  double tol = postObj->ztolzb_ ;

  if (chkStatus >= 2) chkRowAct = 2 ;
  double *rsol = 0 ;
  if (chkRowAct >= 1)
  { rsol = new double[m] ;
    memset(rsol,0,m*sizeof(double)) ; }

/*
  Open a loop to scan each column. For each column, do the following:
    * Update the row solution (lhs value) by adding the contribution from
      this column.
    * Check for bogus values (NaN, infinity)
    * check that the status of the variable agrees with the value and with the
      lower and upper bounds. Free should have no bounds, superbasic should
      have at least one.
*/
  for (int j = 0 ; j < n ; ++j)
  { CoinBigIndex v = mcstrt[j] ;
    int colLen = hincol[j] ;
    double xj = csol[j] ;
    double lj = clo[j] ;
    double uj = cup[j] ;

    if (chkRowAct >= 1)
    { for (int u = 0 ; u < colLen ; ++u)
      { int i = hrow[v] ;
	  double aij = colels[v] ;
	  v = link[v] ;
	  rsol[i] += aij*xj ; } }
    if (chkColSol >= 1)
    { if (CoinIsnan(xj))
      { printf("NaN CSOL: %d  : lb = %g x = %g ub = %g\n",j,lj,xj,uj) ; }
      if (xj <= -PRESOLVE_INF || xj >= PRESOLVE_INF)
      { printf("Inf CSOL: %d  : lb = %g x = %g ub = %g\n",j,lj,xj,uj) ; }
      if (chkColSol >= 2)
      { if (xj < lj-tol)
	{ printf("low CSOL: %d  : lb = %g x = %g ub = %g\n",j,lj,xj,uj) ; }
	else
	if (xj > uj+tol)
	{ printf("high CSOL: %d  : lb = %g x = %g ub = %g\n",
		 j,lj,xj,uj) ; } } }
    if (chkStatus >= 1 && postObj->colstat_)
    { CoinPrePostsolveMatrix::Status statj = postObj->getColumnStatus(j) ;
      switch (statj)
      { case CoinPrePostsolveMatrix::atUpperBound:
	{ if (uj >= PRESOLVE_INF || fabs(xj-uj) > tol)
	  { printf("Bad status CSOL: %d : status atUpperBound : ",j) ;
	    printf("lb = %g x = %g ub = %g\n",lj,xj,uj) ; }
	  break ; }
        case CoinPrePostsolveMatrix::atLowerBound:
	{ if (lj <= -PRESOLVE_INF || fabs(xj-lj) > tol)
	  { printf("Bad status CSOL: %d : status atLowerBound : ",j) ;
	    printf("lb = %g x = %g ub = %g\n",lj,xj,uj) ; }
	  break ; }
        case CoinPrePostsolveMatrix::isFree:
	{ if (lj > -PRESOLVE_INF || uj < PRESOLVE_INF)
	  { printf("Bad status CSOL: %d : status isFree : ",j) ;
	    printf("lb = %g x = %g ub = %g\n",lj,xj,uj) ; }
	  break ; }
        case CoinPrePostsolveMatrix::superBasic:
	{ if (!(lj > -PRESOLVE_INF || uj < PRESOLVE_INF))
	  { printf("Bad status CSOL: %d : status superBasic : ",j) ;
	    printf("lb = %g x = %g ub = %g\n",lj,xj,uj) ; }
	  break ; }
        case CoinPrePostsolveMatrix::basic:
	{ /* Nothing to do here. */
	  break ; }
	default:
	{ printf("Bad status CSOL: %d : status unrecognized : ",j) ;
	  break ; } } } }
/*
  Now check the row solution. acts[i] is what presolve thinks we have, rsol[i]
  is what we've just calculated while scanning the columns. CoinPostsolveMatrix
  does not contain hinrow_, so we can't check for trivial rows (cf. check_sol
  for CoinPresolveMatrix).  For each row,
    * Check for bogus values (NaN, infinity)
*/
  tol *= 1.0e4;
  if (chkRowAct >= 1)
  { for (int i = 0 ; i < m ; ++i)
    { double lhsi = acts[i] ;
      double evali = rsol[i] ;
      double li = rlo[i] ;
      double ui = rup[i] ;
      
      if (CoinIsnan(evali) || CoinIsnan(lhsi))
      { printf("NaN RSOL: %d  : lb = %g eval = %g (expected %g) ub = %g\n",
	       i,li,evali,lhsi,ui) ; }
      if (evali <= -PRESOLVE_INF || evali >= PRESOLVE_INF ||
	  lhsi <= -PRESOLVE_INF || lhsi >= PRESOLVE_INF)
      { printf("Inf RSOL: %d  : lb = %g eval = %g (expected %g) ub = %g\n",
	       i,li,evali,lhsi,ui) ; }
      if (chkRowAct >= 2)
      { if (fabs(evali-lhsi) > tol)
	{ printf("Inacc RSOL: %d : lb = %g eval = %g (expected %g) ub = %g\n",
		 i,li,evali,lhsi,ui) ; }
        if (evali < li-tol || lhsi < li-tol)
	{ printf("low RSOL: %d : lb = %g eval = %g (expected %g) ub = %g\n",
		 i,li,evali,lhsi,ui) ; }
	else
	if (evali > ui+tol || lhsi > ui+tol)
	{ printf("high RSOL: %d : lb = %g eval = %g (expected %g) ub = %g\n",
		 i,li,evali,lhsi,ui) ; } }
      if (chkStatus >= 2 && postObj->rowstat_)
      { CoinPrePostsolveMatrix::Status stati = postObj->getRowStatus(i) ;
	switch (stati)
	{ case CoinPrePostsolveMatrix::atUpperBound:
	  { if (li <= -PRESOLVE_INF || fabs(lhsi-li) > tol)
	    { printf("Bad status RSOL: %d : status atUpperBound : ",i) ;
	      printf("LB = %g lhs = %g UB = %g\n",li,lhsi,ui) ; }
	    break ; }
	  case CoinPrePostsolveMatrix::atLowerBound:
	  { if (ui >= PRESOLVE_INF || fabs(lhsi-ui) > tol)
	    { printf("Bad status RSOL: %d : status atLowerBound : ",i) ;
	      printf("LB = %g lhs = %g UB = %g\n",li,lhsi,ui) ; }
	    break ; }
	  case CoinPrePostsolveMatrix::isFree:
	  { if (li > -PRESOLVE_INF || ui < PRESOLVE_INF)
	    { printf("Bad status RSOL: %d : status isFree : ",i) ;
	      printf("LB = %g lhs = %g UB = %g\n",li,lhsi,ui) ; }
	    break ; }
	  case CoinPrePostsolveMatrix::superBasic:
	  { printf("Bad status RSOL: %d : status superBasic : ",i) ;
	    printf("LB = %g lhs = %g UB = %g\n",li,lhsi,ui) ;
	    break ; }
	  case CoinPrePostsolveMatrix::basic:
	  { /* Nothing to do here. */
	    break ; }
	  default:
	  { printf("Bad status RSOL: %d : status unrecognized : ",i) ;
	    break ; } } } }
  delete [] rsol ; }
  return ; }

/*
  CoinPostsolveMatrix

  Make sure that the number of basic variables is correct.
*/
void presolve_check_nbasic (const CoinPostsolveMatrix *postObj)

{

  int ncols0 = postObj->ncols0_ ;
  int nrows0 = postObj->nrows0_ ;

  char *cdone = postObj->cdone_ ;
  char *rdone = postObj->rdone_ ;

  int nbasic = 0 ;
  int ncdone = 0;
  int nrdone = 0; 
  int ncb = 0;
  int nrb = 0;

  for (int j = 0 ; j < ncols0 ; j++)
  { 
    if (cdone[j] != 0 && postObj->columnIsBasic(j))
    { nbasic++ ;
      ncb++ ; }
    if (cdone[j])
      ncdone++ ;
  }

  for (int i = 0 ; i < nrows0 ; i++)
  {
    if (rdone[i] && postObj->rowIsBasic(i))
    { nbasic++ ;
      nrb++ ; }
    if (rdone[i])
      nrdone++ ;
  }

  if (nbasic != postObj->nrows_)
  { printf("NBASIC (ERROR): %d basic variables, should be %d; ",
	   nbasic,postObj->nrows_) ;
    printf("cdone %d, col basic %d, rdone %d, row basic %d.\n",
	   ncdone,ncb,nrdone,nrb) ;
    fflush(stdout) ; }
# if PRESOLVE_DEBUG > 1
  else
  { std::cout
      << "NBASIC: " << nbasic << " basic variables; cdone " << ncdone
      << ", col basic " << ncb << ", rdone " << nrdone << ", row basic "
      << nrb << std::endl ;
    std::cout << std::flush ;
  }
# endif
  return ; }


/*
  CoinPresolveMatrix

  Overload of presolve_check_nbasic for a CoinPresolveMatrix. There may not be
  a solution, eh?
*/
void presolve_check_nbasic (const CoinPresolveMatrix *preObj)

{

  if (preObj->sol_ == 0) return ;

  int ncols = preObj->ncols_ ;
  int nrows = preObj->nrows_ ;

  int nbasic = 0 ;
  int ncb = 0;
  int nrb = 0;

  for (int j = 0 ; j < ncols ; j++)
  { 
    if (preObj->columnIsBasic(j))
    { nbasic++ ;
      ncb++ ; }
  }

  for (int i = 0 ; i < nrows ; i++)
  {
    if (preObj->rowIsBasic(i))
    { nbasic++ ;
      nrb++ ; }
  }

  if (nbasic != nrows)
  { printf("WRONG NUMBER NBASIC:  is:  %d  should be:  %d;",
	   nbasic,nrows) ;
    printf(" cb %d, rb %d.\n",ncb,nrb);
    fflush(stdout) ; }
  return ; }

#endif
/*
  Original comment: I've forgotton what this is all about

  Looks to me like it's confirming that the columns flagged as basic indeed
  have enough coefficients between them to cover the basis. It'd be serious
  work to get this going again. Waaaaaay out of date.   -- lh, 040831 --
*/
# if 0
void check_pivots (const int *mrstrt, const int *hinrow, const int *hcol,
		   int nrows, const unsigned char *colstat,
		   const unsigned char *rowstat, int ncols)
{
  int i ;
  int nbasic = 0 ;
  int gotone = 1 ;
  int stillmore ;

  return ;

  int *bcol = new int[nrows] ;
  memset(bcol, -1, nrows*sizeof(int)) ;

  char *coldone = new char[ncols] ;
  memset(coldone, 0, ncols) ;

  while (gotone) {
    gotone = 0 ;
    stillmore = 0 ;
    for (i=0; i<nrows; i++)
      if (!postObj->rowIsBasic(i)) {
	int krs = mrstrt[i] ;
	int kre = mrstrt[i] + hinrow[i] ;
	int nb = 0 ;
	int kk ;
	for (int k=krs; k<kre; k++)
	  if (postObj->columnIsBasic(hcol[k]) && !coldone[hcol[k]]) {
	    nb++ ;
	    kk = k ;
	    if (nb > 1)
	      break ;
	  }
	if (nb == 1) {
	  PRESOLVEASSERT(bcol[i] == -1) ;
	  bcol[i] = hcol[kk] ;
	  coldone[hcol[kk]] = 1 ;
	  nbasic++ ;
	  gotone = 1 ;
	}
	else
	  stillmore = 1 ;
      }
  }
  PRESOLVEASSERT(!stillmore) ;

  for (i=0; i<nrows; i++)
    if (postObj->rowIsBasic(i)) {
      int krs = mrstrt[i] ;
      int kre = mrstrt[i] + hinrow[i] ;
      for (int k=krs; k<kre; k++)
	PRESOLVEASSERT(!postObj->columnIsBasic(hcol[k]) || coldone[hcol[k]]) ;
      nbasic++ ;
    }
  PRESOLVEASSERT(nbasic == nrows) ;
}

# endif
