/* $Id: CoinPresolveTripleton.cpp 1585 2013-04-06 20:42:02Z stefan $ */
// Copyright (C) 2003, International Business Machines
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
#include "CoinPresolveTripleton.hpp"

#include "CoinPresolvePsdebug.hpp"
#include "CoinMessage.hpp"

#if PRESOLVE_DEBUG > 0 || PRESOLVE_CONSISTENCY > 0
#include "CoinPresolvePsdebug.hpp"
#endif

/*
 * Substituting y away:
 *
 *	 y = (c - a x - d z) / b
 *
 * so adjust bounds by:   c/b
 *           and x  by:  -a/b
 *           and z  by:  -d/b
 *
 * This affects both the row and col representations.
 *
 * mcstrt only modified if the column must be moved.
 *
 * for every row in icoly
 *	if icolx is also has an entry for row
 *		modify the icolx entry for row
 *		drop the icoly entry from row and modify the icolx entry
 *	else 
 *		add a new entry to icolx column
 *		create a new icolx entry
 *		(this may require moving the column in memory)
 *		replace icoly entry from row and replace with icolx entry
 *
 *   same for icolz
 * The row and column reps are inconsistent during the routine,
 * because icolx in the column rep is updated, and the entries corresponding
 * to icolx in the row rep are updated, but nothing concerning icoly
 * in the col rep is changed.  icoly entries in the row rep are deleted,
 * and icolx entries in both reps are consistent.
 * At the end, we set the length of icoly to be zero, so the reps would
 * be consistent if the row were deleted from the row rep.
 * Both the row and icoly must be removed from both reps.
 * In the col rep, icoly will be eliminated entirely, at the end of the routine;
 * irow occurs in just two columns, one of which (icoly) is eliminated
 * entirely, the other is icolx, which is not deleted here.
 * In the row rep, irow will be eliminated entirely, but not here;
 * icoly is removed from the rows it occurs in.
 */
static bool elim_tripleton(const char * 
#if PRESOLVE_DEBUG > 1
msg
#endif
			   ,
			   CoinBigIndex *mcstrt, 
			   double *rlo, double * acts, double *rup,
			   double *colels,
			   int *hrow, int *hcol,
			   int *hinrow, int *hincol,
			   presolvehlink *clink, int ncols,
			   presolvehlink *rlink, int nrows,
			   CoinBigIndex *mrstrt, double *rowels,
			   //double a, double b, double c,
			   double coeff_factorx,double coeff_factorz,
			   double bounds_factor,
			   int row0, int icolx, int icoly, int icolz)
{
  CoinBigIndex kcs = mcstrt[icoly];
  CoinBigIndex kce = kcs + hincol[icoly];
  CoinBigIndex kcsx = mcstrt[icolx];
  CoinBigIndex kcex = kcsx + hincol[icolx];
  CoinBigIndex kcsz = mcstrt[icolz];
  CoinBigIndex kcez = kcsz + hincol[icolz];

# if PRESOLVE_DEBUG > 1
  printf("%s %d x=%d y=%d z=%d cfx=%g cfz=%g nx=%d yrows=(", msg,
	 row0,icolx,icoly,icolz,coeff_factorx,coeff_factorz,hincol[icolx]) ;
# endif
  for (CoinBigIndex kcoly=kcs; kcoly<kce; kcoly++) {
    int row = hrow[kcoly];

    // even though these values are updated, they remain consistent
    PRESOLVEASSERT(kcex == kcsx + hincol[icolx]);
    PRESOLVEASSERT(kcez == kcsz + hincol[icolz]);

    // we don't need to update the row being eliminated 
    if (row != row0/* && hinrow[row] > 0*/) {
      if (bounds_factor != 0.0) {
	// (1)
	if (-PRESOLVE_INF < rlo[row])
	  rlo[row] -= colels[kcoly] * bounds_factor;

	// (2)
	if (rup[row] < PRESOLVE_INF)
	  rup[row] -= colels[kcoly] * bounds_factor;

	// and solution
	if (acts)
	{ acts[row] -= colels[kcoly] * bounds_factor; }
      }
      // see if row appears in colx
      CoinBigIndex kcolx = presolve_find_row1(row, kcsx, kcex, hrow);
#     if PRESOLVE_DEBUG > 1
      printf("%d%s ",row,(kcolx<kcex)?"x+":"") ;
#     endif
      // see if row appears in colz
      CoinBigIndex kcolz = presolve_find_row1(row, kcsz, kcez, hrow);
#     if PRESOLVE_DEBUG > 1
      printf("%d%s ",row,(kcolz<kcez)?"x+":"") ;
#     endif

      if (kcolx>=kcex&&kcolz<kcez) {
	// swap
	int iTemp;
	iTemp=kcolx;
	kcolx=kcolz;
	kcolz=iTemp;
	iTemp=kcsx;
	kcsx=kcsz;
	kcsz=iTemp;
	iTemp=kcex;
	kcex=kcez;
	kcez=iTemp;
	iTemp=icolx;
	icolx=icolz;
	icolz=iTemp;
	double dTemp=coeff_factorx;
	coeff_factorx=coeff_factorz;
	coeff_factorz=dTemp;
      }
      if (kcolx<kcex) {
	// before:  both x and y are in the row
	// after:   only x is in the row
	// so: number of elems in col x unchanged, and num elems in row is one less

	// update col rep - just modify coefficent
	// column y is deleted as a whole at the end of the loop
	colels[kcolx] += colels[kcoly] * coeff_factorx;
	// update row rep
	// first, copy new value for col x into proper place in rowels
	CoinBigIndex k2 = presolve_find_col(icolx, mrstrt[row], mrstrt[row]+hinrow[row], hcol);
	rowels[k2] = colels[kcolx];
	if (kcolz<kcez) {
	  // before:  both z and y are in the row
	  // after:   only z is in the row
	  // so: number of elems in col z unchanged, and num elems in row is one less
	  
	  // update col rep - just modify coefficent
	  // column y is deleted as a whole at the end of the loop
	  colels[kcolz] += colels[kcoly] * coeff_factorz;
	  // update row rep
	  // first, copy new value for col z into proper place in rowels
	  CoinBigIndex k2 = presolve_find_col(icolz, mrstrt[row], mrstrt[row]+hinrow[row], hcol);
	  rowels[k2] = colels[kcolz];
	  // now delete col y from the row; this changes hinrow[row]
	  presolve_delete_from_row(row, icoly, mrstrt, hinrow, hcol, rowels);
	} else {
	  // before:  only y is in the row
	  // after:   only z is in the row
	  // so: number of elems in col z is one greater, but num elems in row remains same
	  // update entry corresponding to icolz in row rep 
	  // by just overwriting the icoly entry
	  {
	    CoinBigIndex k2 = presolve_find_col(icoly, mrstrt[row], mrstrt[row]+hinrow[row], hcol);
	    hcol[k2] = icolz;
	    rowels[k2] = colels[kcoly] * coeff_factorz;
	  }
	  
	  {
	    bool no_mem = presolve_expand_col(mcstrt,colels,hrow,hincol,
					      clink,ncols,icolz);
	    if (no_mem)
	      return (true);
	    
	    // have to adjust various induction variables
 	    kcolx = mcstrt[icolx] + (kcolx - kcsx);
 	    kcsx = mcstrt[icolx];			
 	    kcex = mcstrt[icolx] + hincol[icolx];
	    kcoly = mcstrt[icoly] + (kcoly - kcs);
	    kcs = mcstrt[icoly];			// do this for ease of debugging
	    kce = mcstrt[icoly] + hincol[icoly];
	    
	    kcolz = mcstrt[icolz] + (kcolz - kcs);	// don't really need to do this
	    kcsz = mcstrt[icolz];
	    kcez = mcstrt[icolz] + hincol[icolz];
	  }
	  
	  // there is now an unused entry in the memory after the column - use it
	  // mcstrt[ncols] == penultimate index of arrays hrow/colels
	  hrow[kcez] = row;
	  colels[kcez] = colels[kcoly] * coeff_factorz;	// y factor is 0.0 
	  hincol[icolz]++, kcez++;	// expand the col
	}
      } else {
	// before:  only y is in the row
	// after:   only x and z are in the row
	// update entry corresponding to icolx in row rep 
	// by just overwriting the icoly entry
	{
	  CoinBigIndex k2 = presolve_find_col(icoly, mrstrt[row], mrstrt[row]+hinrow[row], hcol);
	  hcol[k2] = icolx;
	  rowels[k2] = colels[kcoly] * coeff_factorx;
	}
	presolve_expand_row(mrstrt,rowels,hcol,hinrow,rlink,nrows,row) ;
	// there is now an unused entry in the memory after the column - use it
	int krez = mrstrt[row]+hinrow[row];
	hcol[krez] = icolz;
	rowels[krez] = colels[kcoly] * coeff_factorz;
	hinrow[row]++;

	{
	  bool no_mem = presolve_expand_col(mcstrt,colels,hrow,hincol,
					    clink,ncols,icolx) ;
	  if (no_mem)
	    return (true);

	  // have to adjust various induction variables
	  kcoly = mcstrt[icoly] + (kcoly - kcs);
	  kcs = mcstrt[icoly];			// do this for ease of debugging
	  kce = mcstrt[icoly] + hincol[icoly];
	    
	  kcolx = mcstrt[icolx] + (kcolx - kcs);	// don't really need to do this
	  kcsx = mcstrt[icolx];
	  kcex = mcstrt[icolx] + hincol[icolx];
	  kcolz = mcstrt[icolz] + (kcolz - kcs);	// don't really need to do this
	  kcsz = mcstrt[icolz];
	  kcez = mcstrt[icolz] + hincol[icolz];
	}

	// there is now an unused entry in the memory after the column - use it
	hrow[kcex] = row;
	colels[kcex] = colels[kcoly] * coeff_factorx;	// y factor is 0.0 
	hincol[icolx]++, kcex++;	// expand the col

	{
	  bool no_mem = presolve_expand_col(mcstrt,colels,hrow,hincol,clink,
					    ncols,icolz);
	  if (no_mem)
	    return (true);

	  // have to adjust various induction variables
	  kcoly = mcstrt[icoly] + (kcoly - kcs);
	  kcs = mcstrt[icoly];			// do this for ease of debugging
	  kce = mcstrt[icoly] + hincol[icoly];
	    
	  kcsx = mcstrt[icolx];
	  kcex = mcstrt[icolx] + hincol[icolx];
	  kcsz = mcstrt[icolz];
	  kcez = mcstrt[icolz] + hincol[icolz];
	}

	// there is now an unused entry in the memory after the column - use it
	hrow[kcez] = row;
	colels[kcez] = colels[kcoly] * coeff_factorz;	// y factor is 0.0 
	hincol[icolz]++, kcez++;	// expand the col
      }
    }
  }

# if PRESOLVE_DEBUG > 1
  printf(")\n") ;
# endif

  // delete the whole column
  hincol[icoly] = 0;

  return (false);
}




/*
 *
 * The col rep and row rep must be consistent.
 */
const CoinPresolveAction *tripleton_action::presolve(CoinPresolveMatrix *prob,
						  const CoinPresolveAction *next)
{
  double *colels	= prob->colels_;
  int *hrow		= prob->hrow_;
  CoinBigIndex *mcstrt		= prob->mcstrt_;
  int *hincol		= prob->hincol_;
  int ncols		= prob->ncols_;

  double *clo	= prob->clo_;
  double *cup	= prob->cup_;

  double *rowels	= prob->rowels_;
  int *hcol		= prob->hcol_;
  CoinBigIndex *mrstrt		= prob->mrstrt_;
  int *hinrow		= prob->hinrow_;
  int nrows		= prob->nrows_;

  double *rlo	= prob->rlo_;
  double *rup	= prob->rup_;

  presolvehlink *clink = prob->clink_;
  presolvehlink *rlink = prob->rlink_;

  const unsigned char *integerType = prob->integerType_;

  double *cost	= prob->cost_;

  int numberLook = prob->numberRowsToDo_;
  int iLook;
  int * look = prob->rowsToDo_;
  const double ztolzb	= prob->ztolzb_;

# if PRESOLVE_DEBUG > 0 || PRESOLVE_CONSISTENCY > 0
# if PRESOLVE_DEBUG > 0
  std::cout
    << "Entering tripleton_action::presolve; considering " << numberLook
    << " rows." << std::endl ;
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
  double startTime = 0.0;
  if (prob->tuning_) startTime = CoinCpuTime() ;
# endif
# endif

  action * actions = new action [nrows];
# ifdef ZEROFAULT
  // initialise alignment padding bytes
  memset(actions,0,nrows*sizeof(action)) ;
# endif
  int nactions = 0;

  int *zeros	= prob->usefulColumnInt_; //new int[ncols];
  char * mark = reinterpret_cast<char *>(zeros+ncols);
  memset(mark,0,ncols);
  int nzeros	= 0;

  // If rowstat exists then all do
  unsigned char *rowstat	= prob->rowstat_;
  double *acts	= prob->acts_;
  //  unsigned char * colstat = prob->colstat_;


# if PRESOLVE_CONSISTENCY > 0
  presolve_links_ok(prob) ;
# endif

  // wasfor (int irow=0; irow<nrows; irow++)
  for (iLook=0;iLook<numberLook;iLook++) {
    int irow = look[iLook];
    if (hinrow[irow] == 3 &&
	fabs(rup[irow] - rlo[irow]) <= ZTOLDP) {
      double rhs = rlo[irow];
      CoinBigIndex krs = mrstrt[irow];
      CoinBigIndex kre = krs + hinrow[irow];
      int icolx, icoly, icolz;
      double coeffx, coeffy, coeffz;
      CoinBigIndex k;
      
      /* locate first column */
      for (k=krs; k<kre; k++) {
	if (hincol[hcol[k]] > 0) {
	  break;
	}
      }
      PRESOLVEASSERT(k<kre);
      coeffx = rowels[k];
      if (fabs(coeffx) < ZTOLDP2)
	continue;
      icolx = hcol[k];
      
      
      /* locate second column */
      for (k++; k<kre; k++) {
	if (hincol[hcol[k]] > 0) {
	  break;
	}
      }
      PRESOLVEASSERT(k<kre);
      coeffy = rowels[k];
      if (fabs(coeffy) < ZTOLDP2)
	continue;
      icoly = hcol[k];
      
      /* locate third column */
      for (k++; k<kre; k++) {
	if (hincol[hcol[k]] > 0) {
	  break;
	}
      }
      PRESOLVEASSERT(k<kre);
      coeffz = rowels[k];
      if (fabs(coeffz) < ZTOLDP2)
	continue;
      icolz = hcol[k];
      
      // For now let's do obvious one
      if (coeffx*coeffz>0.0) {
	if(coeffx*coeffy>0.0) 
	  continue;
      } else if (coeffx*coeffy>0.0) {
	int iTemp = icoly;
	icoly=icolz;
	icolz=iTemp;
	double dTemp = coeffy;
	coeffy=coeffz;
	coeffz=dTemp;
      } else {
	int iTemp = icoly;
	icoly=icolx;
	icolx=iTemp;
	double dTemp = coeffy;
	coeffy=coeffx;
	coeffx=dTemp;
      }
      // Not all same sign and y is odd one out
      // don't bother with fixed variables
      if (!(fabs(cup[icolx] - clo[icolx]) < ZTOLDP) &&
	  !(fabs(cup[icoly] - clo[icolx]) < ZTOLDP) &&
	  !(fabs(cup[icolz] - clo[icoly]) < ZTOLDP)) {
	assert (coeffx*coeffz>0.0&&coeffx*coeffy<0.0);
	// Only do if does not give implicit bounds on x and z
	double cx = - coeffx/coeffy;
	double cz = - coeffz/coeffy;
	/* don't do if y integer for now */
	if (integerType[icoly]) {
#define PRESOLVE_DANGEROUS
#ifndef PRESOLVE_DANGEROUS
	  continue;
#else
	  if (!integerType[icolx]||!integerType[icolz])
	    continue;
	  if (cx!=floor(cx+0.5)||cz!=floor(cz+0.5))
	    continue;
#endif
	}
	double rhsRatio = rhs/coeffy;
	if (clo[icoly]>-1.0e30) {
	  if (clo[icolx]<-1.0e30||clo[icolz]<-1.0e30)
	    continue;
	  if (cx*clo[icolx]+cz*clo[icolz]+rhsRatio<clo[icoly]-ztolzb)
	    continue;
	}
	if (cup[icoly]<1.0e30) {
	  if (cup[icolx]>1.0e30||cup[icolz]>1.0e30)
	    continue;
	  if (cx*cup[icolx]+cz*cup[icolz]+rhsRatio>cup[icoly]+ztolzb)
	    continue;
	}
	/* find this row in each of the columns and do counts */
	bool singleton=false;
	for (k=mcstrt[icoly]; k<mcstrt[icoly]+hincol[icoly]; k++) {
	  int jrow=hrow[k];
	  if (hinrow[jrow]==1)
	    singleton=true;
	  if (jrow != irow)
	    prob->setRowUsed(jrow);
	}
	int nDuplicate=0;
	for (k=mcstrt[icolx]; k<mcstrt[icolx]+hincol[icolx]; k++) {
	  int jrow=hrow[k];
	  if (jrow != irow && prob->rowUsed(jrow))
	    nDuplicate++;;
	}
	for (k=mcstrt[icolz]; k<mcstrt[icolz]+hincol[icolz]; k++) {
	  int jrow=hrow[k];
	  if (jrow != irow && prob->rowUsed(jrow))
	    nDuplicate++;;
	}
	int nAdded=hincol[icoly]-3-nDuplicate;
	for (k=mcstrt[icoly]; k<mcstrt[icoly]+hincol[icoly]; k++) {
	  int jrow=hrow[k];
	  prob->unsetRowUsed(jrow);
	}
	// let singleton rows be taken care of first
	if (singleton)
	  continue;
	//if (nAdded<=1) 
	//printf("%d elements added, hincol %d , dups %d\n",nAdded,hincol[icoly],nDuplicate);
	if (nAdded>2)
	  continue;

	// it is possible that both x/z and y are singleton columns
	// that can cause problems
	if ((hincol[icolx] == 1 ||hincol[icolz] == 1) && hincol[icoly] == 1)
	  continue;

	// common equations are of the form ax + by = 0, or x + y >= lo
	{
	  action *s = &actions[nactions];	  
	  nactions++;
	  PRESOLVE_DETAIL_PRINT(printf("pre_tripleton %dR %dC %dC %dC E\n",
				       irow,icoly,icolx,icolz));
	  
	  s->row = irow;
	  s->icolx = icolx;
	  s->icolz = icolz;
	  
	  s->icoly = icoly;
	  s->cloy = clo[icoly];
	  s->cupy = cup[icoly];
	  s->costy = cost[icoly];
	  
	  s->rlo = rlo[irow];
	  s->rup = rup[irow];
	  
	  s->coeffx = coeffx;
	  s->coeffy = coeffy;
	  s->coeffz = coeffz;
	  
	  s->ncoly	= hincol[icoly];
	  s->colel	= presolve_dupmajor(colels, hrow, hincol[icoly],
					    mcstrt[icoly]);
	}

	// costs
	// the effect of maxmin cancels out
	cost[icolx] += cost[icoly] * cx;
	cost[icolz] += cost[icoly] * cz;

	prob->change_bias(cost[icoly] * rhs / coeffy);
	//if (cost[icoly]*rhs)
	//printf("change %g col %d cost %g rhs %g coeff %g\n",cost[icoly]*rhs/coeffy,
	// icoly,cost[icoly],rhs,coeffy);

	if (rowstat) {
	  // update solution and basis
	  int numberBasic=0;
	  if (prob->columnIsBasic(icoly))
	    numberBasic++;
	  if (prob->rowIsBasic(irow))
	    numberBasic++;
	  if (numberBasic>1) {
	    if (!prob->columnIsBasic(icolx))
	      prob->setColumnStatus(icolx,CoinPrePostsolveMatrix::basic);
	    else
	      prob->setColumnStatus(icolz,CoinPrePostsolveMatrix::basic);
	  }
	}
	  
	// Update next set of actions
	{
	  prob->addCol(icolx);
	  int i,kcs,kce;
	  kcs = mcstrt[icoly];
	  kce = kcs + hincol[icoly];
	  for (i=kcs;i<kce;i++) {
	    int row = hrow[i];
	    prob->addRow(row);
	  }
	  kcs = mcstrt[icolx];
	  kce = kcs + hincol[icolx];
	  for (i=kcs;i<kce;i++) {
	    int row = hrow[i];
	    prob->addRow(row);
	  }
	  prob->addCol(icolz);
	  kcs = mcstrt[icolz];
	  kce = kcs + hincol[icolz];
	  for (i=kcs;i<kce;i++) {
	    int row = hrow[i];
	    prob->addRow(row);
	  }
	}

	/* transfer the colx factors to coly */
	bool no_mem = elim_tripleton("ELIMT",
				     mcstrt, rlo, acts, rup, colels,
				     hrow, hcol, hinrow, hincol,
				     clink, ncols, rlink, nrows,
				     mrstrt, rowels,
				     cx,
				     cz,
				     rhs / coeffy,
				     irow, icolx, icoly,icolz);
	if (no_mem) 
	  throwCoinError("out of memory",
			 "tripleton_action::presolve");

	// now remove irow from icolx and icolz in the col rep
	// better if this were first.
	presolve_delete_from_col(irow,icolx,mcstrt,hincol,hrow,colels) ;
	presolve_delete_from_col(irow,icolz,mcstrt,hincol,hrow,colels) ;

	// eliminate irow entirely from the row rep
	hinrow[irow] = 0;

	// eliminate irow entirely from the row rep
	PRESOLVE_REMOVE_LINK(rlink, irow);

	// eliminate coly entirely from the col rep
	PRESOLVE_REMOVE_LINK(clink, icoly);
	cost[icoly] = 0.0;

	rlo[irow] = 0.0;
	rup[irow] = 0.0;

	if (!mark[icolx]) {
	  mark[icolx]=1;
	  zeros[nzeros++]=icolx;
	}
	if (!mark[icolz]) {
	  mark[icolz]=1;
	  zeros[nzeros++]=icolz;
	}
      }
      
#     if PRESOLVE_CONSISTENCY > 0
      presolve_links_ok(prob) ;
      presolve_consistent(prob);
#     endif
    }
  }
  if (nactions) {
#   if PRESOLVE_SUMMARY > 0
    printf("NTRIPLETONS:  %d\n", nactions);
#   endif
    action *actions1 = new action[nactions];
    CoinMemcpyN(actions, nactions, actions1);

    next = new tripleton_action(nactions, actions1, next);

    if (nzeros) {
      next = drop_zero_coefficients_action::presolve(prob, zeros, nzeros, next);
    }
  }

  //delete[]zeros;
  deleteAction(actions,action*);

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
    << "Leaving tripleton_action::presolve, " << droppedRows << " rows, "
    << droppedColumns << " columns dropped" ;
# if COIN_PRESOLVE_TUNING > 0
  std::cout << " in " << thisTime-startTime << "s" ;
# endif
  std::cout << "." << std::endl ;
# endif

  return (next);
}


void tripleton_action::postsolve(CoinPostsolveMatrix *prob) const
{
  const action *const actions = actions_;
  const int nactions = nactions_;

  double *colels	= prob->colels_;
  int *hrow		= prob->hrow_;
  CoinBigIndex *mcstrt		= prob->mcstrt_;
  int *hincol		= prob->hincol_;
  int *link		= prob->link_;

  double *clo	= prob->clo_;
  double *cup	= prob->cup_;

  double *rlo	= prob->rlo_;
  double *rup	= prob->rup_;

  double *dcost	= prob->cost_;

  double *sol	= prob->sol_;
  double *rcosts	= prob->rcosts_;

  double *acts	= prob->acts_;
  double *rowduals = prob->rowduals_;

  unsigned char *colstat	= prob->colstat_;
  unsigned char *rowstat	= prob->rowstat_;

  const double maxmin	= prob->maxmin_;

# if PRESOLVE_DEBUG > 0 || PRESOLVE_CONSISTENCY > 0
  char *cdone	= prob->cdone_;
  char *rdone	= prob->rdone_;
  presolve_check_threads(prob) ;
  presolve_check_sol(prob,2,2,2) ;
  presolve_check_nbasic(prob) ;
# if PRESOLVE_DEBUG > 0
  std::cout << "Entering tripleton_action::postsolve." << std::endl ;
# endif
# endif

  CoinBigIndex &free_list = prob->free_list_;

  const double ztolzb	= prob->ztolzb_;
  const double ztoldj	= prob->ztoldj_;

  // Space for accumulating two columns
  int nrows = prob->nrows_;
  int * index1 = new int[nrows];
  double * element1 = new double[nrows];
  memset(element1,0,nrows*sizeof(double));
  int * index2 = new int[nrows];
  double * element2 = new double[nrows];
  memset(element2,0,nrows*sizeof(double));

  for (const action *f = &actions[nactions-1]; actions<=f; f--) {
    int irow = f->row;

    // probably don't need this
    double ylo0 = f->cloy;
    double yup0 = f->cupy;

    double coeffx = f->coeffx;
    double coeffy = f->coeffy;
    double coeffz = f->coeffz;
    int jcolx = f->icolx;
    int jcoly = f->icoly;
    int jcolz = f->icolz;

    // needed?
    double rhs = f->rlo;

    /* the column was in the reduced problem */
    PRESOLVEASSERT(cdone[jcolx] && rdone[irow]==DROP_ROW&&cdone[jcolz]);
    PRESOLVEASSERT(cdone[jcoly]==DROP_COL);

    // probably don't need this
    rlo[irow] = f->rlo;
    rup[irow] = f->rup;

    // probably don't need this
    clo[jcoly] = ylo0;
    cup[jcoly] = yup0;

    dcost[jcoly] = f->costy;
    dcost[jcolx] += f->costy*coeffx/coeffy;
    dcost[jcolz] += f->costy*coeffz/coeffy;

    // this is why we want coeffx < coeffy (55)
    sol[jcoly] = (rhs - coeffx * sol[jcolx] - coeffz * sol[jcolz]) / coeffy;
	  
    // since this row is fixed 
    acts[irow] = rhs;

    // acts[irow] always ok, since slack is fixed
    if (rowstat)
      prob->setRowStatus(irow,CoinPrePostsolveMatrix::atLowerBound);


    // CLAIM:
    // if the new pi value is chosen to keep the reduced cost
    // of col x at its prior value, then the reduced cost of
    // col y will be 0.
    
    // also have to update row activities and bounds for rows affected by jcoly
    //
    // sol[jcolx] was found for coeffx that
    // was += colels[kcoly] * coeff_factor;
    // where coeff_factor == -coeffx / coeffy
    //
    // its contribution to activity was
    // (colels[kcolx] + colels[kcoly] * coeff_factor) * sol[jcolx]	(1)
    //
    // After adjustment, the two columns contribute:
    // colels[kcoly] * sol[jcoly] + colels[kcolx] * sol[jcolx]
    // == colels[kcoly] * ((rhs - coeffx * sol[jcolx]) / coeffy) + colels[kcolx] * sol[jcolx]
    // == colels[kcoly] * rhs/coeffy + colels[kcoly] * coeff_factor * sol[jcolx] + colels[kcolx] * sol[jcolx]
    // colels[kcoly] * rhs/coeffy + the expression (1)
    //
    // therefore, we must increase the row bounds by colels[kcoly] * rhs/coeffy,
    // which is similar to the bias
    double djy = maxmin * dcost[jcoly];
    double djx = maxmin * dcost[jcolx];
    double djz = maxmin * dcost[jcolz];
    double bounds_factor = rhs/coeffy;
    // need to reconstruct x and z
    double multiplier1 = coeffx/coeffy;
    double multiplier2 = coeffz/coeffy;
    int * indy = reinterpret_cast<int *>(f->colel+f->ncoly);
    int ystart = NO_LINK;
    int nX=0,nZ=0;
    int i,iRow;
    for (i=0; i<f->ncoly; ++i) {
      int iRow = indy[i];
      double yValue = f->colel[i];
      CoinBigIndex k = free_list;
      assert(k >= 0 && k < prob->bulk0_) ;
      free_list = link[free_list];
      if (iRow != irow) {
	// are these tests always true???

	// undo elim_tripleton(1)
	if (-PRESOLVE_INF < rlo[iRow])
	  rlo[iRow] += yValue * bounds_factor;

	// undo elim_tripleton(2)
	if (rup[iRow] < PRESOLVE_INF)
	  rup[iRow] += yValue * bounds_factor;

	acts[iRow] += yValue * bounds_factor;

	djy -= rowduals[iRow] * yValue;
      } 
      
      hrow[k] = iRow;
      PRESOLVEASSERT(rdone[hrow[k]] || hrow[k] == irow);
      colels[k] = yValue;
      link[k] = ystart;
      ystart = k;
      element1[iRow]=yValue*multiplier1;
      index1[nX++]=iRow;
      element2[iRow]=yValue*multiplier2;
      index2[nZ++]=iRow;
    }
#   if PRESOLVE_CONSISTENCY > 0
    presolve_check_free_list(prob) ;
#   endif
    mcstrt[jcoly] = ystart;
    hincol[jcoly] = f->ncoly;
    // find the tail
    CoinBigIndex k=mcstrt[jcolx];
    CoinBigIndex last = NO_LINK;
    int numberInColumn = hincol[jcolx];
    int numberToDo=numberInColumn;
    for (i=0; i<numberToDo; ++i) {
      iRow = hrow[k];
      assert (iRow>=0&&iRow<nrows);
      double value = colels[k]+element1[iRow];
      element1[iRow]=0.0;
      if (fabs(value)>=1.0e-15) {
	colels[k]=value;
	last=k;
	k = link[k];
	if (iRow != irow) 
	  djx -= rowduals[iRow] * value;
      } else {
	numberInColumn--;
	// add to free list
	int nextk = link[k];
	link[k]=free_list;
	free_list=k;
	assert (k>=0);
	k=nextk;
	if (last!=NO_LINK)
	  link[last]=k;
	else
	  mcstrt[jcolx]=k;
      }
    }
    for (i=0;i<nX;i++) {
      int iRow = index1[i];
      double xValue = element1[iRow];
      element1[iRow]=0.0;
      if (fabs(xValue)>=1.0e-15) {
	if (iRow != irow)
	  djx -= rowduals[iRow] * xValue;
	numberInColumn++;
	CoinBigIndex k = free_list;
	assert(k >= 0 && k < prob->bulk0_) ;
	free_list = link[free_list];
	hrow[k] = iRow;
	PRESOLVEASSERT(rdone[hrow[k]] || hrow[k] == irow);
	colels[k] = xValue;
	if (last!=NO_LINK)
	  link[last]=k;
	else
	  mcstrt[jcolx]=k;
	last = k;
      }
    }
#   if PRESOLVE_CONSISTENCY > 0
    presolve_check_free_list(prob) ;
#   endif
    link[last]=NO_LINK;
    assert(numberInColumn);
    hincol[jcolx] = numberInColumn;
    // find the tail
    k=mcstrt[jcolz];
    last = NO_LINK;
    numberInColumn = hincol[jcolz];
    numberToDo=numberInColumn;
    for (i=0; i<numberToDo; ++i) {
      iRow = hrow[k];
      assert (iRow>=0&&iRow<nrows);
      double value = colels[k]+element2[iRow];
      element2[iRow]=0.0;
      if (fabs(value)>=1.0e-15) {
	colels[k]=value;
	last=k;
	k = link[k];
	if (iRow != irow) 
	  djz -= rowduals[iRow] * value;
      } else {
	numberInColumn--;
	// add to free list
	int nextk = link[k];
	assert(free_list>=0);
	link[k]=free_list;
	free_list=k;
	assert (k>=0);
	k=nextk;
	if (last!=NO_LINK)
	  link[last]=k;
	else
	  mcstrt[jcolz]=k;
      }
    }
    for (i=0;i<nZ;i++) {
      int iRow = index2[i];
      double zValue = element2[iRow];
      element2[iRow]=0.0;
      if (fabs(zValue)>=1.0e-15) {
	if (iRow != irow)
	  djz -= rowduals[iRow] * zValue;
	numberInColumn++;
	CoinBigIndex k = free_list;
	assert(k >= 0 && k < prob->bulk0_) ;
	free_list = link[free_list];
	hrow[k] = iRow;
	PRESOLVEASSERT(rdone[hrow[k]] || hrow[k] == irow);
	colels[k] = zValue;
	if (last!=NO_LINK)
	  link[last]=k;
	else
	  mcstrt[jcolz]=k;
	last = k;
      }
    }
#   if PRESOLVE_CONSISTENCY > 0
    presolve_check_free_list(prob) ;
#   endif
    link[last]=NO_LINK;
    assert(numberInColumn);
    hincol[jcolz] = numberInColumn;
    
    
    
    // The only problem with keeping the reduced costs the way they were
    // was that the variable's bound may have moved, requiring it
    // to become basic.
    //printf("djs x - %g (%g), y - %g (%g)\n",djx,coeffx,djy,coeffy);
    if (colstat) {
      if (prob->columnIsBasic(jcolx) ||
	  (fabs(clo[jcolx] - sol[jcolx]) < ztolzb && rcosts[jcolx] >= -ztoldj) ||
	  (fabs(cup[jcolx] - sol[jcolx]) < ztolzb && rcosts[jcolx] <= ztoldj) ||
	  (prob->getColumnStatus(jcolx) ==CoinPrePostsolveMatrix::isFree&&
           fabs(rcosts[jcolx]) <= ztoldj)) {
	// colx or y is fine as it is - make coly basic

	prob->setColumnStatus(jcoly,CoinPrePostsolveMatrix::basic);
	// this is the coefficient we need to force col y's reduced cost to 0.0;
	// for example, this is obviously true if y is a singleton column
	rowduals[irow] = djy / coeffy;
	rcosts[jcolx] = djx - rowduals[irow] * coeffx;
#       if PRESOLVE_DEBUG > 0
	if (prob->columnIsBasic(jcolx)&&fabs(rcosts[jcolx])>1.0e-5)
	  printf("bad dj %d %g\n",jcolx,rcosts[jcolx]);
#       endif
	rcosts[jcolz] = djz - rowduals[irow] * coeffz;
	//if (prob->columnIsBasic(jcolz))
	//assert (fabs(rcosts[jcolz])<1.0e-5);
	rcosts[jcoly] = 0.0;
      } else {
	prob->setColumnStatus(jcolx,CoinPrePostsolveMatrix::basic);
	prob->setColumnStatusUsingValue(jcoly);

	// change rowduals[jcolx] enough to cancel out rcosts[jcolx]
	rowduals[irow] = djx / coeffx;
	rcosts[jcolx] = 0.0;
	// change rowduals[jcolx] enough to cancel out rcosts[jcolx]
	//rowduals[irow] = djz / coeffz;
	//rcosts[jcolz] = 0.0;
	rcosts[jcolz] = djz - rowduals[irow] * coeffz;
	rcosts[jcoly] = djy - rowduals[irow] * coeffy;
      }
    } else {
      // No status array
      // this is the coefficient we need to force col y's reduced cost to 0.0;
      // for example, this is obviously true if y is a singleton column
      rowduals[irow] = djy / coeffy;
      rcosts[jcoly] = 0.0;
    }
    
    // DEBUG CHECK
#   if PRESOLVE_DEBUG > 0
    {
      CoinBigIndex k = mcstrt[jcolx];
      int nx = hincol[jcolx];
      double dj = maxmin * dcost[jcolx];
      
      for (int i=0; i<nx; ++i) {
	int row = hrow[k];
	double coeff = colels[k];
	k = link[k];

	dj -= rowduals[row] * coeff;
      }
      if (! (fabs(rcosts[jcolx] - dj) < 100*ZTOLDP))
	printf("BAD DOUBLE X DJ:  %d %d %g %g\n",
	       irow, jcolx, rcosts[jcolx], dj);
      rcosts[jcolx]=dj;
    }
    {
      CoinBigIndex k = mcstrt[jcoly];
      int ny = hincol[jcoly];
      double dj = maxmin * dcost[jcoly];
      
      for (int i=0; i<ny; ++i) {
	int row = hrow[k];
	double coeff = colels[k];
	k = link[k];

	dj -= rowduals[row] * coeff;
	//printf("b %d coeff %g dual %g dj %g\n",
	// row,coeff,rowduals[row],dj);
      }
      if (! (fabs(rcosts[jcoly] - dj) < 100*ZTOLDP))
	printf("BAD DOUBLE Y DJ:  %d %d %g %g\n",
	       irow, jcoly, rcosts[jcoly], dj);
      rcosts[jcoly]=dj;
      //exit(0);
    }
#   endif
    
#   if PRESOLVE_DEBUG > 0 || PRESOLVE_CONSISTENCY > 0
    cdone[jcoly] = TRIPLETON;
    rdone[irow] = TRIPLETON;
#   endif
  }
  delete [] index1;
  delete [] element1;
  delete [] index2;
  delete [] element2;

# if PRESOLVE_DEBUG > 0 || PRESOLVE_CONSISTENCY > 0
  presolve_check_threads(prob) ;
  presolve_check_sol(prob,2,2,2) ;
  presolve_check_nbasic(prob) ;
# if PRESOLVE_DEBUG > 0
  std::cout << "Leaving tripleton_action::postsolve." << std::endl ;
# endif
# endif
}


tripleton_action::~tripleton_action()
{
  for (int i=nactions_-1; i>=0; i--) {
    delete[]actions_[i].colel;
  }
  deleteAction(actions_,action*);
}



static double *tripleton_mult;
static int *tripleton_id;
void check_tripletons(const CoinPresolveAction * paction)
{
  const CoinPresolveAction * paction0 = paction;
  
  if (paction) {
    check_tripletons(paction->next);
    
    if (strcmp(paction0->name(), "tripleton_action") == 0) {
      const tripleton_action *daction = reinterpret_cast<const tripleton_action *>(paction0);
      for (int i=daction->nactions_-1; i>=0; --i) {
	int icolx = daction->actions_[i].icolx;
	int icoly = daction->actions_[i].icoly;
	double coeffx = daction->actions_[i].coeffx;
	double coeffy = daction->actions_[i].coeffy;

	tripleton_mult[icoly] = -coeffx/coeffy;
	tripleton_id[icoly] = icolx;
      }
    }
  }
}

