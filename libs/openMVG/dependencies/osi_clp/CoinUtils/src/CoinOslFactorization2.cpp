/* $Id: CoinOslFactorization2.cpp 1585 2013-04-06 20:42:02Z stefan $ */
/*
  Copyright (C) 1987, 2009, International Business Machines
  Corporation and others.  All Rights Reserved.

  This code is licensed under the terms of the Eclipse Public License (EPL).
*/
/*
  CLP_OSL - if defined use osl
  0 - don't unroll 2 and 3 - don't use in Gomory
  1 - don't unroll - do use in Gomory
  2 - unroll - don't use in Gomory
  3 - unroll and use in Gomory
*/
#include "CoinOslFactorization.hpp"
#include "CoinOslC.h"
#include "CoinFinite.hpp"

#ifndef NDEBUG
extern int ets_count;
extern int ets_check;
#endif
#define COIN_REGISTER register
#define COIN_REGISTER2
#define COIN_REGISTER3 register
#ifdef COIN_USE_RESTRICT
# define COIN_RESTRICT2 __restrict
#else
# define COIN_RESTRICT2
#endif
static int c_ekkshfpo_scan2zero(COIN_REGISTER const EKKfactinfo * COIN_RESTRICT2 fact,const int * COIN_RESTRICT mpermu,
		       double *COIN_RESTRICT worki, double *COIN_RESTRICT worko, int * COIN_RESTRICT mptr)
{
  
  /* Local variables */
  int irow;
  double tolerance = fact->zeroTolerance;
  int nin=fact->nrow;
  int * COIN_RESTRICT mptrX=mptr;
  if ((nin&1)!=0) {
    irow=1;
    if (fact->packedMode) {
      int irow0= *mpermu;
      double dval;
      assert (irow0>=1&&irow0<=nin);
      mpermu++;
      dval=worki[irow0];
      if (NOT_ZERO(dval)) {
	worki[irow0]=0.0;
	if (fabs(dval) >= tolerance) {
	  *(worko++)=dval;
	  *(mptrX++) = 0;
	}
      }
    } else {
      int irow0= *mpermu;
      double dval;
      assert (irow0>=1&&irow0<=nin);
      mpermu++;
      dval=worki[irow0];
      if (NOT_ZERO(dval)) {
	worki[irow0]=0.0;
	if (fabs(dval) >= tolerance) {
	  *worko=dval;
	  *(mptrX++) = 0;
	}
      }
      worko++;
    }
  } else {
    irow=0;
  }
  if (fact->packedMode) {
    for (; irow < nin; irow+=2) {
      int irow0,irow1;
      double dval0,dval1;
      irow0=mpermu[0];
      irow1=mpermu[1];
      assert (irow0>=1&&irow0<=nin);
      assert (irow1>=1&&irow1<=nin);
      dval0=worki[irow0];
      dval1=worki[irow1];
      if (NOT_ZERO(dval0)) {
	worki[irow0]=0.0;
	if (fabs(dval0) >= tolerance) {
	  *(worko++)=dval0;
	  *(mptrX++) = irow+0;
	}
      }
      if (NOT_ZERO(dval1)) {
	worki[irow1]=0.0;
	if (fabs(dval1) >= tolerance) {
	  *(worko++)=dval1;
	  *(mptrX++) = irow+1;
	}
      }
      mpermu+=2;
    }
  } else {
    for (; irow < nin; irow+=2) {
      int irow0,irow1;
      double dval0,dval1;
      irow0=mpermu[0];
      irow1=mpermu[1];
      assert (irow0>=1&&irow0<=nin);
      assert (irow1>=1&&irow1<=nin);
      dval0=worki[irow0];
      dval1=worki[irow1];
      if (NOT_ZERO(dval0)) {
	worki[irow0]=0.0;
	if (fabs(dval0) >= tolerance) {
	  worko[0]=dval0;
	  *(mptrX++) = irow+0;
	}
      }
      if (NOT_ZERO(dval1)) {
	worki[irow1]=0.0;
	if (fabs(dval1) >= tolerance) {
	  worko[1]=dval1;
	  *(mptrX++) = irow+1;
	}
      }
      mpermu+=2;
      worko+=2;
    }
  }
  return static_cast<int>(mptrX-mptr);
}
/*
 * c_ekkshfpi_list executes the following loop:
 *
 * for (k=nincol, i=1; k; k--, i++) {
 *   int ipt = mptr[i];
 *   int irow = mpermu[ipt]; 
 *   worko[mpermu[irow]] = worki[i];
 *   worki[i] = 0.0;
 * }
 */
static int c_ekkshfpi_list(const int *COIN_RESTRICT mpermu, 
			   double *COIN_RESTRICT worki, 
			   double *COIN_RESTRICT worko,
			   const int * COIN_RESTRICT mptr, int nincol,
			   int * lastNonZero)
{
  int i,k,irow0,irow1;
  int first=COIN_INT_MAX;
  int last=0;
  /* worko was zeroed out outside */
  k = nincol;
  i = 0;
  if ((k&1)!=0) {
    int ipt=mptr[i];
    irow0=mpermu[ipt];
    first = CoinMin(irow0,first);
    last = CoinMax(irow0,last);
    i++;
    worko[irow0]=*worki;
    *worki++=0.0;
  }
  k=k>>1;
  for (; k; k--) {
    int ipt0 = mptr[i];
    int ipt1 = mptr[i+1];
    irow0 = mpermu[ipt0];
    irow1 = mpermu[ipt1];
    i+=2;
    first = CoinMin(irow0,first);
    last = CoinMax(irow0,last);
    first = CoinMin(irow1,first);
    last = CoinMax(irow1,last);
    worko[irow0] = worki[0];
    worko[irow1] = worki[1];
    worki[0]=0.0;
    worki[1]=0.0;
    worki+=2;
  }
  *lastNonZero=last;
  return first;
}
/*
 * c_ekkshfpi_list2 executes the following loop:
 *
 * for (k=nincol, i=1; k; k--, i++) {
 *   int ipt = mptr[i];
 *   int irow = mpermu[ipt]; 
 *   worko[mpermu[irow]] = worki[ipt];
 *   worki[ipt] = 0.0;
 * }
 */
static int c_ekkshfpi_list2(const int *COIN_RESTRICT mpermu, double *COIN_RESTRICT worki, double *COIN_RESTRICT worko,
			    const int * COIN_RESTRICT mptr, int nincol,
			   int * lastNonZero)
{
#if 1
  int i,k,irow0,irow1;
  int first=COIN_INT_MAX;
  int last=0;
  /* worko was zeroed out outside */
  k = nincol;
  i = 0;
  if ((k&1)!=0) {
    int ipt=mptr[i];
    irow0=mpermu[ipt];
    first = CoinMin(irow0,first);
    last = CoinMax(irow0,last);
    i++;
    worko[irow0]=worki[ipt];
    worki[ipt]=0.0;
  }
  k=k>>1;
  for (; k; k--) {
    int ipt0 = mptr[i];
    int ipt1 = mptr[i+1];
    irow0 = mpermu[ipt0];
    irow1 = mpermu[ipt1];
    i+=2;
    first = CoinMin(irow0,first);
    last = CoinMax(irow0,last);
    first = CoinMin(irow1,first);
    last = CoinMax(irow1,last);
    worko[irow0] = worki[ipt0];
    worko[irow1] = worki[ipt1];
    worki[ipt0]=0.0;
    worki[ipt1]=0.0;
  }
#else
  int first=COIN_INT_MAX;
  int last=0;
  /* worko was zeroed out outside */
  for (int i=0; i<nincol; i++) {
    int ipt = mptr[i];
    int irow = mpermu[ipt];
    first = CoinMin(irow,first);
    last = CoinMax(irow,last);
    worko[irow] = worki[ipt];
    worki[ipt]=0.0;
  }
#endif
  *lastNonZero=last;
  return first;
}
/*
 * c_ekkshfpi_list3 executes the following loop:
 *
 * for (k=nincol, i=1; k; k--, i++) {
 *   int ipt  = mptr[i];
 *   int irow = mpermu[ipt];
 *   worko[irow] = worki[i];
 *   worki[i] = 0.0;
 *   mptr[i] = mpermu[ipt];
 * }
 */
static void c_ekkshfpi_list3(const int *COIN_RESTRICT mpermu,
		    double *COIN_RESTRICT worki, double *COIN_RESTRICT worko,
		    int * COIN_RESTRICT mptr, int nincol)
{
  int i,k,irow0,irow1;
  /* worko was zeroed out outside */
  k = nincol;
  i = 0;
  if ((k&1)!=0) {
    int ipt=mptr[i];
    irow0=mpermu[ipt];
    mptr[i] = irow0;
    i++;
    worko[irow0]=*worki;
    *worki++=0.0;
  }
  k=k>>1;
  for (; k; k--) {
    int ipt0 = mptr[i];
    int ipt1 = mptr[i+1];
    irow0 = mpermu[ipt0];
    irow1 = mpermu[ipt1];
    mptr[i] = irow0;
    mptr[i+1] = irow1;
    i+=2;
    worko[irow0] = worki[0];
    worko[irow1] = worki[1];
    worki[0]=0.0;
    worki[1]=0.0;
    worki+=2;
  }
}
static int c_ekkscmv(COIN_REGISTER const EKKfactinfo * COIN_RESTRICT2 fact,int n, double *COIN_RESTRICT dwork, int *COIN_RESTRICT mptr, 
		   double *COIN_RESTRICT dwork2)
{
  double tolerance = fact->zeroTolerance;
  int irow;
  const int * COIN_RESTRICT mptrsave = mptr;
  double * COIN_RESTRICT dwhere = dwork+1;
  if ((n&1)!=0) {
    if (NOT_ZERO(*dwhere)) {
      if (fabs(*dwhere) >= tolerance) {
	*++dwork2 = *dwhere;
	*++mptr = SHIFT_INDEX(1);
      } else {
	*dwhere = 0.0;
      }
    }
    dwhere++;
    irow=2;
  } else {
    irow=1;
  }
  for (n=n>>1;n;n--) {
    int second = NOT_ZERO(*(dwhere+1));
    if (NOT_ZERO(*dwhere)) {
      if (fabs(*dwhere) >= tolerance) {
	*++dwork2 = *dwhere;
	*++mptr = SHIFT_INDEX(irow);
      } else {
	*dwhere = 0.0;
      }
    }
    if (second) {
      if (fabs(*(dwhere+1)) >= tolerance) {
	*++dwork2 = *(dwhere+1);
	*++mptr = SHIFT_INDEX(irow+1);
      } else {
	*(dwhere+1) = 0.0;
      }
    }
    dwhere+=2;
    irow+=2;
  }
  
  return static_cast<int>(mptr-mptrsave);
} /* c_ekkscmv */
double c_ekkputl(const EKKfactinfo * COIN_RESTRICT2 fact,
	     const int *COIN_RESTRICT mpt2,
	     double *COIN_RESTRICT dwork1,
	     double del3, 
	     int nincol, int nuspik)
{
  double * COIN_RESTRICT dwork3	= fact->xeeadr+fact->nnentu;
  int * COIN_RESTRICT hrowi	= fact->xeradr+fact->nnentu;
  int offset = fact->R_etas_start[fact->nR_etas+1];
  int *COIN_RESTRICT hrowiR = fact->R_etas_index+offset;
  double *COIN_RESTRICT dluval = fact->R_etas_element+offset;
  int i, j;
  
  /* dwork1 is r', the new R transform
   * dwork3 is the updated incoming column, alpha_p
   * del3 apparently has the pivot of the incoming column (???).
   * Here, we compute the p'th element of R^-1 alpha_p
   * (as described on p. 273), which is just a dot product.
   * I don't know why we subtract.
   */
  for (i = 1; i <= nuspik; ++i) {
    j = UNSHIFT_INDEX(hrowi[ i]);
    del3 -= dwork3[i] * dwork1[j];
  }
  
  /* here we finally copy the r' to where we want it, the end */
  /* also take into account that the p'th row of R^-1 is -(p'th row of R). */
  /* also zero out dwork1 as we go */
  for (i = 0; i < nincol; ++i) {
    j = mpt2[i];
    hrowiR[ - i ] = SHIFT_INDEX(j);
    dluval[ - i ] = -dwork1[j];
    dwork1[j] = 0.;
  }
  
  return del3;
} /* c_ekkputl */
/* making this static seems to slow code down!
   may be being inlined
*/
int c_ekkputl2( const EKKfactinfo * COIN_RESTRICT2 fact,
	     double *COIN_RESTRICT dwork1,
	     double *del3p, 
	     int nuspik)
{
  double * COIN_RESTRICT dwork3	= fact->xeeadr+fact->nnentu;
  int * COIN_RESTRICT hrowi	= fact->xeradr+fact->nnentu;
  int offset = fact->R_etas_start[fact->nR_etas+1];
  int *COIN_RESTRICT hrowiR = fact->R_etas_index+offset;
  double *COIN_RESTRICT dluval = fact->R_etas_element+offset;
  int i, j;
#if 0
  int nincol=c_ekksczr(fact,fact->nrow,dwork1,hrowiR);
#else
  int nrow=fact->nrow;
  const double tolerance = fact->zeroTolerance;
  int * COIN_RESTRICT mptrX=hrowiR;
  for (i = 1; i <= nrow; ++i) {
    if (dwork1[i] != 0.) {
      if (fabs(dwork1[i]) >= tolerance) {
	*(mptrX--) = SHIFT_INDEX(i);
      } else {
	dwork1[i] = 0.0;
      }
    }
  }
  int nincol=static_cast<int>(hrowiR-mptrX);
#endif
  double del3 = *del3p;
  /* dwork1 is r', the new R transform
   * dwork3 is the updated incoming column, alpha_p
   * del3 apparently has the pivot of the incoming column (???).
   * Here, we compute the p'th element of R^-1 alpha_p
   * (as described on p. 273), which is just a dot product.
   * I don't know why we subtract.
   */
  for (i = 1; i <= nuspik; ++i) {
    j = UNSHIFT_INDEX(hrowi[ i]);
    del3 -= dwork3[i] * dwork1[j];
  }
  
  /* here we finally copy the r' to where we want it, the end */
  /* also take into account that the p'th row of R^-1 is -(p'th row of R). */
  /* also zero out dwork1 as we go */
  for (i = 0; i < nincol; ++i) {
    j = UNSHIFT_INDEX(hrowiR[-i]);
    dluval[ - i ] = -dwork1[j];
    dwork1[j] = 0.;
  }
  
  *del3p = del3;
  return nincol;
} /* c_ekkputl */
static void c_ekkbtj4p_no_dense(const int nrow,const double * COIN_RESTRICT dluval, 
				const int * COIN_RESTRICT hrowi,
				const int * COIN_RESTRICT mcstrt,  
				double * COIN_RESTRICT dwork1, int ndo,int jpiv)
{
  int i;
  double dv1;
  int iel;
  int irow;
  int i1,i2;
  
  /* count down to first nonzero */
  for (i=nrow;i >=1;i--) {
    if (dwork1[i]) {
      break;
    }
  }
  i--; /* as pivot is just 1.0 */
  if (i>ndo+jpiv) {
    i=ndo+jpiv;
  }
  mcstrt -= jpiv;
  i2=mcstrt[i+1];
  for (; i > jpiv; --i) {
    double dv1b=0.0;
    int nel;
    i1 = mcstrt[i];
    nel= i1-i2;
    dv1 = dwork1[i];
    iel=i2;
    if ((nel&1)!=0) {
      irow = hrowi[iel];
      dv1b = SHIFT_REF(dwork1, irow) * dluval[iel];
      iel++;
    }
    for ( ; iel < i1; iel+=2) {
      int irow = hrowi[iel];
      int irowb = hrowi[iel+1];
      dv1 += SHIFT_REF(dwork1, irow) * dluval[iel];
      dv1b += SHIFT_REF(dwork1, irowb) * dluval[iel+1];
    }
    i2=i1;
    dwork1[i] = dv1+dv1b;
  }
} /* c_ekkbtj4 */

static int c_ekkbtj4p_dense(const int nrow,const double * COIN_RESTRICT dluval,
			    const int * COIN_RESTRICT hrowi,
			    const int * COIN_RESTRICT mcstrt,  double * COIN_RESTRICT dwork1,
		   int ndenuc,
		   int ndo,int jpiv)
{
  int i;
  int i2;
  
  int last=ndo-ndenuc+1;
  double * COIN_RESTRICT densew = &dwork1[nrow-1];
  int nincol=0;
  const double * COIN_RESTRICT dlu1;
  dluval--;
  hrowi--;
  /* count down to first nonzero */
  for (i=nrow;i >=1;i--) {
    if (dwork1[i]) {
      break;
    }
  }
  if (i<ndo+jpiv) {
    int diff = ndo+jpiv-i;
    ndo -= diff;
    densew-=diff;
    nincol=diff;
  }
  i2=mcstrt[ndo+1];
  dlu1=&dluval[i2+1];
  for (i = ndo; i >last; i-=2) {
    int k;
    double dv1,dv2;
    const double * COIN_RESTRICT dlu2;
    dv1=densew[1];
    dlu2=dlu1+nincol;
    dv2=densew[0];
    for (k=0;k<nincol;k++) {
#ifdef DEBUG
      int kk=dlu1-dluval;
      int jj = (densew+(nincol-k+1))-dwork1;
      int ll=hrowi[k+kk];
      if (ll!=jj) abort();
#endif
      dv1 += densew[nincol-k+1]*dlu1[k];
      dv2 += densew[nincol-k+1]*dlu2[k];
    }
    densew[1]=dv1;
    dlu1=dlu2+nincol;
    dv2 += dv1*dlu1[0];
    dlu1++;
    nincol+=2;
    densew[0]=dv2;
    densew-=2;
  }
  return i;
} /* c_ekkbtj4 */

static void c_ekkbtj4p_after_dense(const double * COIN_RESTRICT dluval, 
				   const int * COIN_RESTRICT hrowi,
				   const int * COIN_RESTRICT mcstrt,  
				   double * COIN_RESTRICT dwork1, int i,int jpiv)
{
  int iel;
  mcstrt -= jpiv;
  i += jpiv;
  iel=mcstrt[i+1];
  for (; i > jpiv+1; i-=2) {
    int i1 = mcstrt[i];
    double dv1 = dwork1[i];
    double dv2;
    for (; iel < i1; iel++) {
      int irow = hrowi[iel];
      dv1 += SHIFT_REF(dwork1, irow) * dluval[iel];
    }
    i1 = mcstrt[i-1];
    dv2 = dwork1[i-1];
    dwork1[i] = dv1;
    for (; iel < i1; iel++) {
      int irow = hrowi[iel];
      dv2 += SHIFT_REF(dwork1, irow) * dluval[iel];
    }
    dwork1[i-1] = dv2;
  }
  if (i>jpiv) {
    int i1 = mcstrt[i];
    double dv1 = dwork1[i];
    for (; iel < i1; iel++) {
      int irow = hrowi[iel];
      dv1 += SHIFT_REF(dwork1, irow) * dluval[iel];
    }
    dwork1[i] = dv1;
  }
}

static void c_ekkbtj4p(COIN_REGISTER const EKKfactinfo * COIN_RESTRICT2 fact,
	      double * COIN_RESTRICT dwork1)
{
  int lstart=fact->lstart;
  const int * COIN_RESTRICT hpivco	= fact->kcpadr;
  const double * COIN_RESTRICT dluval	= fact->xeeadr+1;
  const int * COIN_RESTRICT hrowi	= fact->xeradr+1;
  const int * COIN_RESTRICT mcstrt	= fact->xcsadr+lstart-1;
  int jpiv=hpivco[lstart]-1;
  int ndo=fact->xnetalval;
  /*     see if dense enough to unroll */
  if (fact->ndenuc<5) {
    c_ekkbtj4p_no_dense(fact->nrow,dluval,hrowi,mcstrt,dwork1,ndo,jpiv);
  } else {
    int i = c_ekkbtj4p_dense(fact->nrow,dluval,hrowi,mcstrt,dwork1,
			   fact->ndenuc, ndo,jpiv);
    c_ekkbtj4p_after_dense(dluval,hrowi,mcstrt,dwork1,i,jpiv);
  }
} /* c_ekkbtj4p */

static int c_ekkbtj4_sparse(COIN_REGISTER2 const EKKfactinfo * COIN_RESTRICT2 fact,
			  double * COIN_RESTRICT dwork1, 
			  int * COIN_RESTRICT mpt,	/* C style */
			  double * COIN_RESTRICT dworko,
			  int nincol, int * COIN_RESTRICT spare)
{
  const int nrow		= fact->nrow;
  const int * COIN_RESTRICT hcoli	= fact->xecadr;
  const int * COIN_RESTRICT mrstrt	= fact->xrsadr+nrow;
  char * COIN_RESTRICT nonzero = fact->nonzero;
  const int * COIN_RESTRICT hpivro	= fact->krpadr;
  const double * COIN_RESTRICT de2val	= fact->xe2adr-1;
  double tolerance = fact->zeroTolerance;
  double dv;
  int iel;
  
  int k,nStack,kx;
  int nList=0;
  int * COIN_RESTRICT list = spare;
  int * COIN_RESTRICT stack = spare+nrow;
  int * COIN_RESTRICT next = stack+nrow;
  int iPivot,kPivot;
  int iput,nput=0,kput=nrow;
  int j;
  int firstDoRow=fact->firstDoRow;
  
  for (k=0;k<nincol;k++) {
    nStack=1;
    iPivot=mpt[k];
    if (nonzero[iPivot]!=1&&iPivot>=firstDoRow) {
      stack[0]=iPivot;
      next[0]=mrstrt[iPivot];
      while (nStack) {
	/* take off stack */
	kPivot=stack[--nStack];
	if (nonzero[kPivot]!=1&&kPivot>=firstDoRow) {
	  j=next[nStack];
	  if (j==mrstrt[kPivot+1]) {
	    /* finished so mark */
	    list[nList++]=kPivot;
	    nonzero[kPivot]=1;
	  } else {
	    kPivot=hcoli[j];
	    /* put back on stack */
	    next[nStack++] ++;
	    if (!nonzero[kPivot]) {
	      /* and new one */
	      stack[nStack]=kPivot;
	      nonzero[kPivot]=2;
	      next[nStack++]=mrstrt[kPivot];
	    }
	  }
	} else if (kPivot<firstDoRow) {
	  list[--kput]=kPivot;
	  nonzero[kPivot]=1;
	}
      }
    } else if (nonzero[iPivot]!=1) {
      /* nothing to do (except check size at end) */
      list[--kput]=iPivot;
      nonzero[iPivot]=1;
    }
  }
  if (fact->packedMode) {
    dworko++;
    for (k=nList-1;k>=0;k--) {
      double dv;
      iPivot = list[k];
      dv = dwork1[iPivot];
      dwork1[iPivot]=0.0;
      nonzero[iPivot]=0;
      if (fabs(dv) > tolerance) {
	iput=hpivro[iPivot];
	kx=mrstrt[iPivot];
	dworko[nput]=dv;
	for (iel = kx; iel < mrstrt[iPivot+1]; iel++) {
	  double dval;
	  int irow = hcoli[iel];
	  dval=de2val[iel];
	  dwork1[irow] += dv*dval;
	}
	mpt[nput++]=iput-1;
      } else {
	dwork1[iPivot]=0.0;	/* force to zero, not just near zero */
      }
    }
    /* check remainder */
    for (k=kput;k<nrow;k++) {
      iPivot = list[k];
      nonzero[iPivot]=0;
      dv = dwork1[iPivot];
      dwork1[iPivot]=0.0;
      iput=hpivro[iPivot];
      if (fabs(dv) > tolerance) {
	dworko[nput]=dv;
	mpt[nput++]=iput-1;
      }
    }
  } else {
    /* not packed */
    for (k=nList-1;k>=0;k--) {
      double dv;
      iPivot = list[k];
      dv = dwork1[iPivot];
      dwork1[iPivot]=0.0;
      nonzero[iPivot]=0;
      if (fabs(dv) > tolerance) {
	iput=hpivro[iPivot];
	kx=mrstrt[iPivot];
	dworko[iput]=dv;
	for (iel = kx; iel < mrstrt[iPivot+1]; iel++) {
	  double dval;
	  int irow = hcoli[iel];
	  dval=de2val[iel];
	  dwork1[irow] += dv*dval;
	}
	mpt[nput++]=iput-1;
      } else {
	dwork1[iPivot]=0.0;	/* force to zero, not just near zero */
      }
    }
    /* check remainder */
    for (k=kput;k<nrow;k++) {
      iPivot = list[k];
      nonzero[iPivot]=0;
      dv = dwork1[iPivot];
      dwork1[iPivot]=0.0;
      iput=hpivro[iPivot];
      if (fabs(dv) > tolerance) {
	dworko[iput]=dv;
	mpt[nput++]=iput-1;
      }
    }
  }
  
  return (nput);
} /* c_ekkbtj4 */

static void c_ekkbtjl(COIN_REGISTER2 const EKKfactinfo * COIN_RESTRICT2 fact,
		    double * COIN_RESTRICT dwork1)
{
  int i, j, k, k1;
  int l1;
  const double * COIN_RESTRICT dluval = fact->R_etas_element;
  const int * COIN_RESTRICT hrowi = fact->R_etas_index;
  const int * COIN_RESTRICT mcstrt = fact->R_etas_start;
  const int * COIN_RESTRICT hpivco = fact->hpivcoR;
  int ndo=fact->nR_etas;
#ifndef UNROLL1
#define UNROLL1 4
#endif
#if UNROLL1>2
  int l2;
#endif
  int kn;
  double dv;
  int iel;
  int ipiv;
  int knext;
  
  knext = mcstrt[ndo + 1];
#if UNROLL1>2
  for (i = ndo; i > 0; --i) {
    k1 = knext;
    knext = mcstrt[i];
    ipiv = hpivco[i];
    dv = dwork1[ipiv];
    /*       fast floating */
    k = knext - k1;
    kn = k >> 2;
    iel = k1 + 1;
    if (dv != 0.) {
      l1 = (k & 1) != 0;
      l2 = (k & 2) != 0;
      for (j = 1; j <= kn; j++) {
	int irow0 = hrowi[iel + 0];
	int irow1 = hrowi[iel + 1];
	int irow2 = hrowi[iel + 2];
	int irow3 = hrowi[iel + 3];
	double dval0 = dv * dluval[iel + 0] + SHIFT_REF(dwork1, irow0);
	double dval1 = dv * dluval[iel + 1] + SHIFT_REF(dwork1, irow1);
	double dval2 = dv * dluval[iel + 2] + SHIFT_REF(dwork1, irow2);
	double dval3 = dv * dluval[iel + 3] + SHIFT_REF(dwork1, irow3);
	SHIFT_REF(dwork1, irow0) = dval0;
	SHIFT_REF(dwork1, irow1) = dval1;
	SHIFT_REF(dwork1, irow2) = dval2;
	SHIFT_REF(dwork1, irow3) = dval3;
	iel+=4;
      }
      if (l1) {
	int irow0 = hrowi[iel];
	SHIFT_REF(dwork1, irow0) += dv* dluval[iel];
	++iel;
      }
      if (l2) {
	int irow0 = hrowi[iel + 0];
	int irow1 = hrowi[iel + 1];
	SHIFT_REF(dwork1, irow0) += dv* dluval[iel];
	SHIFT_REF(dwork1, irow1) += dv* dluval[iel+1];
      }
    }
  }
#else
  for (i = ndo; i > 0; --i) {
    k1 = knext;
    knext = mcstrt[i];
    ipiv = hpivco[i];
    dv = dwork1[ipiv];
    k = knext - k1;
    kn = k >> 1;
    iel = k1 + 1;
    if (dv != 0.) {
      l1 = (k & 1) != 0;
      for (j = 1; j <= kn; j++) {
	int irow0 = hrowi[iel + 0];
	int irow1 = hrowi[iel + 1];
	double dval0 = dv * dluval[iel + 0] + SHIFT_REF(dwork1, irow0);
	double dval1 = dv * dluval[iel + 1] + SHIFT_REF(dwork1, irow1);
	SHIFT_REF(dwork1, irow0) = dval0;
	SHIFT_REF(dwork1, irow1) = dval1;
	iel+=2;
      }
      if (l1) {
	int irow0 = hrowi[iel];
	SHIFT_REF(dwork1, irow0) += dv* dluval[iel];
	++iel;
      }
    }
  }
#endif
} /* c_ekkbtjl */

static int c_ekkbtjl_sparse(COIN_REGISTER2 const EKKfactinfo * COIN_RESTRICT2 fact,
		   double * COIN_RESTRICT dwork1, 
		   int * COIN_RESTRICT mpt , int nincol)
{
  const double * COIN_RESTRICT dluval = fact->R_etas_element;
  const int * COIN_RESTRICT hrowi = fact->R_etas_index;
  const int * COIN_RESTRICT mcstrt = fact->R_etas_start;
  const int * COIN_RESTRICT hpivco = fact->hpivcoR;
  char * COIN_RESTRICT nonzero = fact->nonzero;
  int ndo=fact->nR_etas;
  int i, j, k1;
  double dv;
  int ipiv;
  int irow0, irow1;
  int knext;
  int number=nincol;
  
  /*     ------------------------------------------- */
  /* adjust back */
  hrowi++;
  dluval++;
  
  /*         DO ANY ROW TRANSFORMATIONS */
  
  /* Function Body */
  knext = mcstrt[ndo + 1];
  for (i = ndo; i > 0; --i) {
    k1 = knext;
    knext = mcstrt[i];
    ipiv = hpivco[i];
    dv = dwork1[ipiv];
    if (dv) {
      for (j = k1; j <knext-1; j+=2) {
	irow0 = hrowi[j];
	irow1 = hrowi[j+1];
	SHIFT_REF(dwork1, irow0) += dv * dluval[j];
	SHIFT_REF(dwork1, irow1) += dv * dluval[j+1];
	if (!nonzero[irow0]) {
	  nonzero[irow0]=1;
	  mpt[++number]=UNSHIFT_INDEX(irow0);
	}
	if (!nonzero[irow1]) {
	  nonzero[irow1]=1;
	  mpt[++number]=UNSHIFT_INDEX(irow1);
	}
      }
      if (j<knext) {
	irow0 = hrowi[j];
	SHIFT_REF(dwork1, irow0) += dv * dluval[j];
	if (!nonzero[irow0]) {
	  nonzero[irow0]=1;
	  mpt[++number]=UNSHIFT_INDEX(irow0);
	}
      }
    }
  }
  return (number);
} /* c_ekkbtjl */



static void c_ekkbtju_dense(const int nrow,
			    const double * COIN_RESTRICT dluval,
			    const int * COIN_RESTRICT hrowi,
			    const int * COIN_RESTRICT mcstrt, 
			    int * COIN_RESTRICT hpivco, 
			    double * COIN_RESTRICT dwork1,
			    int * COIN_RESTRICT start,int last,int offset,
			    double * COIN_RESTRICT densew)
{
  /* Local variables */
  int ipiv1,ipiv2;
  int save=hpivco[last];
  
  hpivco[last]=nrow+1;
  
  ipiv1=*start;
  ipiv2=hpivco[ipiv1];
  while(ipiv2<last) {
    int iel,k;
    const int   kx1	= mcstrt[ipiv1];
    const int   kx2  = mcstrt[ipiv2];
    const int   nel1 = hrowi[kx1-1];
    const int   nel2 = hrowi[kx2-1];
    const double dpiv1 = dluval[kx1-1];
    const double dpiv2 = dluval[kx2-1];
    const int   n1	= offset+ipiv1; /* number in dense part */
    const int   nsparse1=nel1-n1;
    const int   nsparse2=nel2-n1-(ipiv2-ipiv1);
    const int   k1 = kx1+nsparse1;
    const int   k2 = kx2+nsparse2;
    const double *dlu1 = &dluval[k1];
    const double *dlu2 = &dluval[k2];
    
    double dv1 = dwork1[ipiv1];
    double dv2 = dwork1[ipiv2];
    
    for (iel = kx1; iel < k1; ++iel) {
      dv1 -= SHIFT_REF(dwork1, hrowi[iel]) * dluval[iel];
    }
    for (iel = kx2; iel < k2; ++iel) {
      dv2 -= SHIFT_REF(dwork1, hrowi[iel]) * dluval[iel];
    }
    for (k=0;k<n1;k++) {
      dv1 -= dlu1[k] * densew[k];
      dv2 -= dlu2[k] * densew[k];
    }
    dv1 *= dpiv1;
    dv2 -= dlu2[n1] * dv1;
    dwork1[ipiv1] = dv1;
    dwork1[ipiv2] = dv2*dpiv2;
    ipiv1 = hpivco[ipiv2];
    ipiv2 = hpivco[ipiv1];
  }
  hpivco[last]=save;
  
  *start=ipiv1;
  return;
}
/* about 8-10% of execution time is spent in this routine */
static int c_ekkbtju_aux(const double * COIN_RESTRICT dluval, 
			 const int * COIN_RESTRICT hrowi,
			 const int * COIN_RESTRICT mcstrt, 
			 const int * COIN_RESTRICT hpivco,
			 double * COIN_RESTRICT dwork1,
			 int ipiv, int loop_end)
{
#define UNROLL2 2
#ifndef UNROLL2
#if CLP_OSL==2||CLP_OSL==3
#define UNROLL2 2
#else
#define UNROLL2 1
#endif
#endif
  while (ipiv<=loop_end) {
    int kx = mcstrt[ipiv];
    const int nel = hrowi[kx-1];
#if UNROLL2<2
    const int kxe = kx + nel;
#endif
    
    double dv = dwork1[ipiv];	/* rhs */
#if UNROLL2>1
    const int * hrowi2=hrowi+kx;
    const int * hrowi2end=hrowi2+nel;
    const double * dluval2=dluval+kx;
#else
    int iel;
#endif
    const double dpiv = dluval[kx-1];	/* inverse of pivot */
    
    
    /* subtract terms whose unknowns have been solved for */
    
    /* a significant proportion of these loops may not modify dv at all.
     * However, it seems to be just as expensive to check if the loop
     * would modify dv as it is to just do it.
     * The only difference would be that dluval wouldn't be referenced
     * for those loops, would might save some cache paging,
     * but unfortunately the code generated to search for zeros (on AIX)
     * is *worse* than code that just multiplies by dval.
     */
#if UNROLL2<2
    for (iel = kx; iel < kxe; ++iel) {
      const int irow = hrowi[iel];
      const double dval=dluval[iel];
      dv -= SHIFT_REF(dwork1, irow) * dval;
    }
    
    dwork1[ipiv] = dv * dpiv;	/* divide by the pivot */
#else
    if ((nel&1)!=0) {
      int irow = *hrowi2;
      double dval=*dluval2;
      dv -= SHIFT_REF(dwork1, irow) * dval;
      hrowi2++;
      dluval2++;
    }
    for (; hrowi2 < hrowi2end; hrowi2 +=2,dluval2 +=2) {
      int irow0 = hrowi2[0];
      int irow1 = hrowi2[1];
      double dval0=dluval2[0];
      double dval1=dluval2[1];
      double d0=SHIFT_REF(dwork1, irow0);
      double d1=SHIFT_REF(dwork1, irow1);
      dv -= d0 * dval0;
      dv -= d1 * dval1;
    }
    dwork1[ipiv] = dv * dpiv;	/* divide by the pivot */
#endif
    
    ipiv=hpivco[ipiv];
  }
  
  return (ipiv);
}

/*
 * We are given the upper diagonal matrix U from the LU factorization
 * and a rhs dwork1.
 * This solves the system U x = dwork1
 * by back substitution, overwriting dwork1 with the solution x.
 *
 * It does this in textbook style by solving the equations "bottom" up,
 * so for each equation one new unknown is solved for by subtracting
 * from the rhs the sum of the terms whose unknowns have been solved for,
 * then dividing by the coefficient of the new unknown.
 *
 * Since we update the U matrix using F-T, the order of the columns
 * changes slightly each iteration.  Initially, hpivco[i] == i+1,
 * and each iteration (generally) introduces one element where this
 * is no longer true.  However, because we periodically refactorize,
 * it is much more common for hpivco[i] == i+1 than not.
 *
 * The one quirk is that value referred to as the pivot is actually
 * the reciprocal of the pivot, to avoid a division.
 *
 * Solving in this fashion is inappropriate if there are frequently
 * cases where all unknowns in an equation have value zero.
 * This seems to happen frequently if the sparsity of the rhs is, say, 10%.
 */
static void c_ekkbtju(COIN_REGISTER2 const EKKfactinfo * COIN_RESTRICT2 fact,	     
	     double * COIN_RESTRICT dwork1,
	     int ipiv)
{
  const int nrow	= fact->nrow;
  double * COIN_RESTRICT dluval	= fact->xeeadr;
  int * COIN_RESTRICT hrowi	= fact->xeradr;
  int * COIN_RESTRICT mcstrt	= fact->xcsadr;
  int * COIN_RESTRICT hpivco_new = fact->kcpadr+1;
  int ndenuc=fact->ndenuc;
  int first_dense = fact->first_dense;
  int last_dense = fact->last_dense;
  
  const int has_dense = (first_dense<last_dense &&
			 mcstrt[ipiv]<=mcstrt[last_dense]);
  
  /* Parameter adjustments */
  /* dluval and hrowi were NOT decremented here.
     I believe that they are used as C-style arrays below.
     At this point, I am going to convert them from Fortran- to C-style
     here by incrementing them; at some later time, I will convert their
     uses in this file to Fortran-style.
  */
  dluval++;
  hrowi++;
  
  if (has_dense) 
    ipiv = c_ekkbtju_aux(dluval, hrowi, mcstrt, hpivco_new, dwork1, ipiv,
		       first_dense - 1);
  
  if (has_dense) {
    int n=0;
    int firstDense = nrow-ndenuc+1;
    double *densew = &dwork1[firstDense];
    
    /* check first dense to see where in triangle it is */
    int last=first_dense;
    int j=mcstrt[last]-1;
    int k1=j;
    int k2=j+hrowi[j];
    
    for (j=k2;j>k1;j--) {
      int irow=UNSHIFT_INDEX(hrowi[j]);
      if (irow<firstDense) {
	break;
      } else {
#ifdef DEBUG
	if (irow!=last-1) {
	  abort();
	}
#endif
	last=irow;
	n++;
      }
    }
    c_ekkbtju_dense(nrow,dluval,hrowi,mcstrt,const_cast<int *> (hpivco_new),
		  dwork1,&ipiv,last_dense, n - first_dense, densew);
  }
  
  (void) c_ekkbtju_aux(dluval, hrowi, mcstrt, hpivco_new, dwork1, ipiv, nrow);
} /* c_ekkbtju */


/*
 * mpt / *nincolp contain the indices of nonzeros in dwork1.
 * nonzero contains the same information as a byte-mask.
 *
 * currently, erase_nonzero is true iff this is called from c_ekketsj.
 */
static int c_ekkbtju_sparse(COIN_REGISTER2 const EKKfactinfo * COIN_RESTRICT2 fact,
		   double * COIN_RESTRICT dwork1,
		   int * COIN_RESTRICT mpt, int nincol,
		   int * COIN_RESTRICT spare)
{
  const double * COIN_RESTRICT dluval	= fact->xeeadr+1;
  const int * COIN_RESTRICT mcstrt	= fact->xcsadr;
  char * COIN_RESTRICT nonzero = fact->nonzero;
  const int * COIN_RESTRICT hcoli	= fact->xecadr;
  const int * COIN_RESTRICT mrstrt	= fact->xrsadr;
  const int * COIN_RESTRICT hinrow	= fact->xrnadr;
  const double * COIN_RESTRICT de2val	= fact->xe2adr-1;
  int i;
  int iPivot;
  int nList=0;
  int nStack,k,kx;
  const int nrow=fact->nrow;
  const double tolerance = fact->zeroTolerance;
  int * COIN_RESTRICT list = spare;
  int * COIN_RESTRICT stack = spare+nrow;
  int * COIN_RESTRICT next = stack+nrow;
  /*
   * Examine all nonzero elements and determine which elements may be
   * nonzero in the result.
   * Any row in U that contains terms that may have nonzero variable values
   * may produce a nonzero value.
   */
  for (k=0;k<nincol;k++) {
    nStack=1;
    iPivot=mpt[k];
    stack[0]=iPivot;
    next[0]=0;
    while (nStack) {
      int kPivot,ninrow,j;
      /* take off stack */
      kPivot=stack[--nStack];
      /*printf("nStack %d kPivot %d, ninrow %d, j %d, nList %d\n",
	nStack,kPivot,hinrow[kPivot],
	next[nStack],nList);*/
      if (nonzero[kPivot]!=1) {
	ninrow = hinrow[kPivot];
	j=next[nStack];
	if (j!=ninrow) {
	  kx = mrstrt[kPivot];
	  kPivot=hcoli[kx+j];
	  /* put back on stack */
	  next[nStack++] ++;
	  if (!nonzero[kPivot]) {
	    /* and new one */
	    stack[nStack]=kPivot;
	    nonzero[kPivot]=2;
	    next[nStack++]=0;
	  }
	} else {
	  /* finished so mark */
	  list[nList++]=kPivot;
	  nonzero[kPivot]=1;
	}
      }
    }
  }
  
  i=nList-1;
  nList=0;
  for (;i>=0;i--) {
    double dpiv;
    double dv;
    iPivot = list[i];
    kx	= mcstrt[iPivot];
    dpiv = dluval[kx-1];
    dv	= dpiv * dwork1[iPivot];
    nonzero[iPivot] = 0;
    if (fabs(dv)>=tolerance) {
      int iel;
      int krx	= mrstrt[iPivot];
      int krxe	= krx+hinrow[iPivot];
      dwork1[iPivot]=dv;
      mpt[nList++]=iPivot;
      for (iel = krx; iel < krxe; iel++) {
	int irow0 = hcoli[iel];
	double dval=de2val[iel];
	dwork1[irow0] -= dv*dval;
      }
    } else {
      dwork1[iPivot]=0.0;
    }
  }
  
  return (nList);
} /* c_ekkbtjuRow */

/*
 * dpermu is supposed to be zeroed on entry to this routine.
 * It is used as a working buffer.
 * The input vector dwork1 is permuted into dpermu, operated on,
 * and the answer is permuted back into dwork1, zeroing dpermu in
 * the process.
 */
/*
 * nincol > 0 ==> mpt contains indices of non-zeros in dpermu
 *
 * first_nonzero contains index of first (last??)nonzero;
 * only used if nincol==0.
 *
 * dpermu contains permuted input; dwork1 is now zero
 */
int c_ekkbtrn(COIN_REGISTER3 const EKKfactinfo * COIN_RESTRICT2 fact,
	    double * COIN_RESTRICT dwork1,
	    int * COIN_RESTRICT mpt, int first_nonzero)
{
  double * COIN_RESTRICT dpermu = fact->kadrpm;
  const int * COIN_RESTRICT mpermu=fact->mpermu;
  const int * COIN_RESTRICT hpivco_new= fact->kcpadr+1;
  
  const int nrow	= fact->nrow;
  int i;
  int nincol;
  /* find the first non-zero input */
  int ipiv;
  if (first_nonzero) {
    ipiv = first_nonzero;
#if 1
    if (c_ekk_IsSet(fact->bitArray,ipiv)) {
      /* slack */
      int lastSlack = fact->lastSlack;
      int firstDo=hpivco_new[lastSlack];
      assert (dpermu[ipiv]);
      while (ipiv!=firstDo) {
	assert (c_ekk_IsSet(fact->bitArray,ipiv));
	if (dpermu[ipiv])
	  dpermu[ipiv]=-dpermu[ipiv];
	ipiv=hpivco_new[ipiv];
      }
    }
#endif
  } else {
    int lastSlack = fact->numberSlacks;
    ipiv=hpivco_new[0];
    for (i=0;i<lastSlack;i++) {
      int next_piv = hpivco_new[ipiv];
      assert (c_ekk_IsSet(fact->bitArray,ipiv));
      if (dpermu[ipiv]) {
	break;
      } else {
	ipiv=next_piv;
      }
    }
    
    /* usually, there is a non-zero slack entry... */
    if (i==lastSlack) {
      /* but if there isn't... */
      for (;i<nrow;i++) {
	if (!dpermu[ipiv]) {
	  ipiv=hpivco_new[ipiv];
	} else {
	  break;
	}
      }
    } else {
      /* reverse signs for slacks */
      for (;i<lastSlack;i++) {
	assert (c_ekk_IsSet(fact->bitArray,ipiv));
	if (dpermu[ipiv])
	  dpermu[ipiv]=-dpermu[ipiv];
	ipiv=hpivco_new[ipiv];
      }
      assert (!c_ekk_IsSet(fact->bitArray,ipiv)||ipiv>fact->nrow);
      
      /* this is presumably the first non-zero non slack */
      /*ipiv=firstDo;*/
    }
  }
  if (ipiv<=fact->nrow) {
    /* skipBtju is always (?) 0 first the first call,
     * ipiv tends to be >nrow for the second */
    
    /*       DO U */
    c_ekkbtju(fact,dpermu,
	    ipiv); 
  }
  
  
  /*       DO ROW ETAS IN L */
  c_ekkbtjl(fact, dpermu); 
  c_ekkbtj4p(fact,dpermu);
  
  /* dwork1[mpermu] = dpermu; dpermu = 0; mpt = indices of non-zeros */
  nincol = 
    c_ekkshfpo_scan2zero(fact,&mpermu[1],dpermu,&dwork1[1],&mpt[1]);
  
  /* dpermu should be zero now */
#ifdef DEBUG
  for (i=1;i<=nrow ;i++ ) {
    if (dpermu[i]) {
      abort();
    } /* endif */
  } /* endfor */
#endif
  return (nincol);
} /* c_ekkbtrn */

static int c_ekkbtrn0_new(COIN_REGISTER3 const EKKfactinfo * COIN_RESTRICT2 fact,
			double * COIN_RESTRICT dwork1,
			int * COIN_RESTRICT mpt, int nincol, 
			   int * COIN_RESTRICT spare)
{
  double * COIN_RESTRICT dpermu = fact->kadrpm;
  const int * COIN_RESTRICT mpermu=fact->mpermu;
  const int * COIN_RESTRICT hpivro	= fact->krpadr;
  
  const int nrow	= fact->nrow;
  
  int i;
  char * nonzero=fact->nonzero;
  int doSparse=1;
  
  /* so:  dpermu must contain room for:
   * nrow doubles, followed by
   * nrow ints (mpermu), followed by
   * nrow ints (the inverse permutation), followed by
   * an unused area (?) of nrow ints, followed by
   * nrow chars (this non-zero array).
   *
   * and apparently the first nrow elements of nonzero are expected
   * to already be zero.
   */    
#ifdef DEBUG
  for (i=1;i<=nrow ;i++ ) {
    if (nonzero[i]) {
      abort();
    } /* endif */
  } /* endfor */
#endif
  /* now nonzero[i]==1 iff there is an entry for i in mpt */
  
  nincol=c_ekkbtju_sparse(fact, dpermu,
			&mpt[1], nincol,
			spare);
  
  /* the vector may have more nonzero elements now */
  /*       DO ROW ETAS IN L */
#define DENSE_THRESHOLD (nincol*10+100)
  if (DENSE_THRESHOLD>nrow) {
    doSparse=0;
    c_ekkbtjl(fact, dpermu); 
  } else {
    /* set nonzero */
    for(i=0;i<nincol;i++) {
      int j=mpt[i+1];
      nonzero[j]=1;
    }
    nincol =
      c_ekkbtjl_sparse(fact,
		     dpermu,
		     mpt, 
		     nincol);
    for(i=0;i<nincol;i++) {
      int j=mpt[i+1];
      nonzero[j]=0;
    }
    if (DENSE_THRESHOLD>nrow) {
      doSparse=0;
#ifdef DEBUG
      for (i=1;i<=nrow;i++) {
	if (nonzero[i]) {
	  abort();
	}
      }
#endif
    }
  }
  if (!doSparse) {
    c_ekkbtj4p(fact,dpermu);
    /* dwork1[mpermu] = dpermu; dpermu = 0; mpt = indices of non-zeros */
    nincol = 
      c_ekkshfpo_scan2zero(fact,&mpermu[1],dpermu,&dwork1[1],&mpt[1]);
    
    /* dpermu should be zero now */
#ifdef DEBUG
    for (i=1;i<=nrow ;i++ ) {
      if (dpermu[i]) {
	abort();
      } /* endif */
    } /* endfor */
#endif
  } else {
    /* still sparse */
    if (fact->nnentl) {
      nincol =
	c_ekkbtj4_sparse(fact,
		       dpermu,
		       &mpt[1],
		       dwork1,
		       nincol,spare);
    } else {
      double tolerance=fact->zeroTolerance;
      int irow;
      int nput=0;
      if (fact->packedMode) {
	for (i = 0; i <nincol; i++) {
	  int irow0;
	  double dval;
	  irow=mpt[i+1];
	  dval=dpermu[irow];
	  if (NOT_ZERO(dval)) {
	    if (fabs(dval) >= tolerance) {
	      irow0= hpivro[irow];
	      dwork1[1+nput]=dval;
	      mpt[1 + nput++]=irow0-1;
	    }
	    dpermu[irow]=0.0;
	  }
	}
      } else {
	for (i = 0; i <nincol; i++) {
	  int irow0;
	  double dval;
	  irow=mpt[i+1];
	  dval=dpermu[irow];
	  if (NOT_ZERO(dval)) {
	    if (fabs(dval) >= tolerance) {
	      irow0= hpivro[irow];
	      dwork1[irow0]=dval;
	      mpt[1 + nput++]=irow0-1;
	    }
	    dpermu[irow]=0.0;
	  }
	}
      }
      nincol=nput;
    }
  }
  
  
  return (nincol);
} /* c_ekkbtrn */


/* returns c_ekkbtrn(fact, dwork1, mpt)
 *
 * but since mpt[1..nincol] contains the indices of non-zeros in dwork1,
 * we can do faster.
 */
static int c_ekkbtrn_mpt(COIN_REGISTER3 const EKKfactinfo * COIN_RESTRICT2 fact,
		double * COIN_RESTRICT dwork1,
		int * COIN_RESTRICT mpt, int nincol,int * COIN_RESTRICT spare)
{
  double * COIN_RESTRICT dpermu = fact->kadrpm;
  const int nrow	= fact->nrow;
  
  const int * COIN_RESTRICT mpermu=fact->mpermu;
  /*const int *mrstrt	= fact->xrsadr;*/
  
#ifdef DEBUG
  int i;
  memset(spare,'A',3*nrow*sizeof(int));
  {
    
    for (i=1;i<=nrow;i++) {
      if (dpermu[i]) {
	abort();
      }
    }
  } 
#endif
  
  
  int i;
#ifdef DEBUG
  for (i=1;i<=nrow;i++) {
    if (fact->nonzero[i]||dpermu[i]) {
      abort();
    }
  }
#endif
  assert (fact->if_sparse_update>0&&mpt&&fact->rows_ok) ;
  
  /* read the input vector from mpt/dwork1;
   * permute it into dpermu;
   * construct a nonzero mask in nonzero;
   * overwrite mpt with the permuted indices;
   * clear the dwork1 vector.
   */
  for (i=0;i<nincol;i++) {
    int irow=mpt[i+1];
    int jrow=mpermu[irow];
    dpermu[jrow]=dwork1[irow];
    /*nonzero[jrow-1]=1; this is done in btrn0 */
    mpt[i+1]=jrow;
    dwork1[irow]=0.0;
  }
  
  if (DENSE_THRESHOLD<nrow) {
    nincol = c_ekkbtrn0_new(fact, dwork1, mpt, nincol,spare);
  } else {
    nincol = c_ekkbtrn(fact, dwork1, mpt, 0);
  }
#ifdef DEBUG
  {
    
    for (i=1;i<=nrow;i++) {
      if (dpermu[i]) {
	abort();
      }
    }
    if (fact->if_sparse_update>0) {
      for (i=1;i<=nrow;i++) {
	if (fact->nonzero[i]) {
	  abort();
	}
      }
    }
  } 
#endif
  return nincol;
}

/* returns c_ekkbtrn(fact, dwork1, mpt)
 *
 * but since (dwork1[i]!=0) == (i==ipivrw),
 * we can do faster.
 */
int c_ekkbtrn_ipivrw(COIN_REGISTER3 const EKKfactinfo * COIN_RESTRICT2 fact,
		   double * COIN_RESTRICT dwork1,
		   int * COIN_RESTRICT mpt, int ipivrw,int * COIN_RESTRICT spare)
{
  double * COIN_RESTRICT dpermu = fact->kadrpm;
  const int nrow	= fact->nrow;
  
  const int * COIN_RESTRICT mpermu=fact->mpermu;
  const double * COIN_RESTRICT dluval	= fact->xeeadr;
  const int * COIN_RESTRICT mrstrt	= fact->xrsadr;
  const int * COIN_RESTRICT hinrow	= fact->xrnadr;
  const int * COIN_RESTRICT hcoli	= fact->xecadr;
  const int * COIN_RESTRICT mcstrt	= fact->xcsadr;
  
  int nincol;
  
#ifdef DEBUG
  int i;
  for (i=1;i<=nrow ;i++ ) {
    if (dpermu[i]) {
      abort();
    } /* endif */
  } /* endfor */
#endif
  
  if (fact->if_sparse_update>0&&mpt&& fact->rows_ok) {
    mpt[1] = ipivrw;
    nincol = c_ekkbtrn_mpt(fact, dwork1, mpt, 1,spare);
  } else {
    int ipiv;
    int kpivrw = mpermu[ipivrw];
    dpermu[kpivrw]=dwork1[ipivrw];
    dwork1[ipivrw]=0.0;
    
    if (fact->rows_ok) {
      /* !fact->if_sparse_update
       * but we still have rowwise info,
       * so we may as well use it to do the slack row
       */
      int iipivrw=nrow+1;
      int itest = fact->nnentu+1;
      int k=mrstrt[kpivrw];
      int lastInRow= k+hinrow[kpivrw];
      double dpiv,dv;
      for (;k<lastInRow;k++) {
	int icol=hcoli[k];
	int start=mcstrt[icol];
	if (start<itest) {
	  iipivrw=icol;
	  itest=start;
	}
      }
      /* do missed pivot */
      itest=mcstrt[kpivrw];
      dpiv=dluval[itest];
      dv=dpermu[kpivrw];
      dv*=dpiv;
      dpermu[kpivrw]=dv;
      ipiv=iipivrw;
    } else {
      /* no luck - c_ekkbtju will slog through slacks (?) */
      ipiv=kpivrw;
    }
    /* nincol not read */
    /* not sparse */
    /* do slacks */
    if (ipiv<=fact->nrow) {
      if (c_ekk_IsSet(fact->bitArray,ipiv)) {
	const int * hpivco_new= fact->kcpadr+1;
	int lastSlack = fact->lastSlack;
	int firstDo=hpivco_new[lastSlack];
	/* slack */
	/* need pivot row of first nonslack */
	dpermu[ipiv]=-dpermu[ipiv];
#ifndef NDEBUG
	while (1) {
	  assert (c_ekk_IsSet(fact->bitArray,ipiv));
	  ipiv=hpivco_new[ipiv];
	  if (ipiv>fact->nrow||ipiv==firstDo)
	    break;
	}
	assert (!c_ekk_IsSet(fact->bitArray,ipiv)||ipiv>fact->nrow);
	assert (ipiv==firstDo);
#endif
	ipiv=firstDo;
      }
    }
    nincol = c_ekkbtrn(fact, dwork1, mpt, ipiv);
  }
  
  return nincol;
}
/*
 * Does work associated with eq. 3.7:
 *	r' = u' U^-1
 *
 * where u' (can't write the overbar) is the p'th row of U, without
 * the entry for column p.  (here, jpivrw is p).
 * We solve this as for btju.  We know
 *	r' U = u'
 *
 * so we solve from low index to hi, determining the next value u_i'
 * by doing the dot-product of r' and the i'th column of U (excluding
 * element i itself), subtracting that from u'_i, and dividing by
 * U_ii (we store the reciprocal, so here we multiply).
 *
 * Now, in principle dwork1 should be initialized to the p'th row of U.
 * Instead, it is initially zeroed and filled in as we go along.
 * Of the entries in u' that we reference during a dot product with
 * a column of U, either
 *	the entry is 0 by definition, since it is < p, or
 *	it has already been set by a previous iteration, or
 *	it is p.
 *
 * Because of this, we know that all elements < p will be zero;
 * that's why we start with p (kpivrw).
 
 * While we do this product, we also zero out the p'th row.
 */
static void c_ekketju_aux(COIN_REGISTER2 EKKfactinfo * COIN_RESTRICT2 fact,int sparse,
			double * COIN_RESTRICT dluval, int * COIN_RESTRICT hrowi,
			const int * COIN_RESTRICT mcstrt, const int * COIN_RESTRICT hpivco,
			double * COIN_RESTRICT dwork1,
			int *ipivp, int jpivrw, int stop_col)
{
  int ipiv = *ipivp;
  if (1&&ipiv<stop_col&&c_ekk_IsSet(fact->bitArray,ipiv)) {
    /* slack */
    int lastSlack = fact->lastSlack;
    int firstDo=hpivco[lastSlack];
    while (1) {
      assert (c_ekk_IsSet(fact->bitArray,ipiv));
      dwork1[ipiv] = -dwork1[ipiv];
      ipiv=hpivco[ipiv];	/* next column - generally ipiv+1 */
      if (ipiv==firstDo||ipiv>=stop_col)
	break;
    }
  }
  
  while(ipiv<stop_col) {
    double dv = dwork1[ipiv];
    int kx = mcstrt[ipiv];
    int nel = hrowi[kx];
    double dpiv = dluval[kx];
    int kcs = kx + 1;
    int kce = kx + nel;
    int iel;
    
    for (iel = kcs; iel <= kce; ++iel) {
      int irow = hrowi[iel];
      irow = UNSHIFT_INDEX(irow);
      dv -= dwork1[irow] * dluval[iel];
      if (irow == jpivrw) {
	break;
      }
    }
    
    /* assuming the p'th row is sparse,
     * this branch will be infrequently taken */
    if (iel <= kce) {
      int irow = hrowi[iel];
      /* irow == jpivrw */
      dv += dluval[iel];
      
      if (sparse) {
	/* delete this entry by overwriting it with the last */
	--nel;
	hrowi[kx] = nel;
	hrowi[iel] = hrowi[kce];
#ifdef CLP_REUSE_ETAS
	double temp=dluval[iel];
	dluval[iel] = dluval[kce];
	hrowi[kce]=jpivrw;
	dluval[kce]=temp;
#else
	dluval[iel] = dluval[kce];
#endif
	kce--;
      } else {
	/* we can't delete an entry from a dense column,
	 * so we just zero it out */
	dluval[iel]=0.0;
	iel++;
      }
      
      /* finish up the remaining entries; same as above loop, but no check */
      for (; iel <= kce; ++iel) {
	irow = UNSHIFT_INDEX(hrowi[iel]);
	dv -= dwork1[irow] * dluval[iel];
      }
    }
    dwork1[ipiv] = dv * dpiv;	/* divide by pivot */
    ipiv=hpivco[ipiv];	/* next column - generally ipiv+1 */
  }
  
  /* ? is it guaranteed that ipiv==stop_col at this point?? */
  *ipivp = ipiv;
}

/* dwork1 is assumed to be zeroed on entry */
static void c_ekketju(COIN_REGISTER EKKfactinfo * COIN_RESTRICT2 fact,double *dluval, int *hrowi,
		    const int * COIN_RESTRICT mcstrt, const int * COIN_RESTRICT hpivco,
		    double * COIN_RESTRICT dwork1,
		    int kpivrw, int first_dense , int last_dense)
{
  int ipiv = hpivco[kpivrw];
  int jpivrw = SHIFT_INDEX(kpivrw);
  
  const int nrow	= fact->nrow;
  
  if (first_dense < last_dense &&
      mcstrt[ipiv] <= mcstrt[last_dense]) {
    /* There are dense columns, and
     * some dense columns precede the pivot column */
    
    /* first do any sparse columns "on the left" */
    c_ekketju_aux(fact, true, dluval, hrowi, mcstrt, hpivco, dwork1,
		&ipiv, jpivrw, first_dense);
    
    /* then do dense columns */
    c_ekketju_aux(fact, false, dluval, hrowi, mcstrt, hpivco, dwork1,
		&ipiv, jpivrw, last_dense+1);
    
    /* final sparse columns "on the right" ...*/
  } 
  /* ...are the same as sparse columns if there are no dense */
  c_ekketju_aux(fact, true, dluval, hrowi, mcstrt, hpivco, dwork1,
	      &ipiv, jpivrw, nrow+1);
} /* c_ekketju */












/*#define PRINT_DEBUG*/
/* dwork1 is assumed to be zeroed on entry */
int c_ekketsj(COIN_REGISTER2 /*const*/ EKKfactinfo * COIN_RESTRICT2 fact,
	    double * COIN_RESTRICT dwork1,
	    int * COIN_RESTRICT mpt2, double dalpha, int orig_nincol,
	    int npivot, int *nuspikp,
	    const int ipivrw,int * spare)
{
  int nuspik	= *nuspikp;
  
  int * COIN_RESTRICT mpermu=fact->mpermu;
  
  int * COIN_RESTRICT hcoli	= fact->xecadr;
  double * COIN_RESTRICT dluval	= fact->xeeadr;
  int * COIN_RESTRICT mrstrt	= fact->xrsadr;
  int * COIN_RESTRICT hrowi	= fact->xeradr;
  int * COIN_RESTRICT mcstrt	= fact->xcsadr;
  int * COIN_RESTRICT hinrow	= fact->xrnadr;
  /*int *hincol	= fact->xcnadr;
    int *hpivro	= fact->krpadr;*/
  int * COIN_RESTRICT hpivco	= fact->kcpadr;
  double * COIN_RESTRICT de2val	= fact->xe2adr;
  
  const int nrow	= fact->nrow;
  const int ifRowCopy	= fact->rows_ok;
  
  int i, j=-1, k, i1, i2, k1;
  int kc, iel;
  double del3;
  int nroom;
  bool ifrows= (mrstrt[1] != 0);
  int kpivrw, jpivrw;
  int first_dense_mcstrt,last_dense_mcstrt;
  int nnentl;			/* includes row stuff */
  int doSparse=(fact->if_sparse_update>0);
#ifdef MORE_DEBUG
  {
    const int * COIN_RESTRICT hrowi = fact->R_etas_index;
    const int * COIN_RESTRICT mcstrt = fact->R_etas_start;
    int ndo=fact->nR_etas;
    int knext;
  
    knext = mcstrt[ndo + 1];
    for (int i = ndo; i > 0; --i) {
      int k1 = knext;
      knext = mcstrt[i];
      for (int j = k1+1; j < knext; j++) {
	assert (hrowi[j]>0&&hrowi[j]<100000);
      }
    }
  }
#endif
  
  int mcstrt_piv;
  int nincol=0;
  int * COIN_RESTRICT hpivco_new=fact->kcpadr+1;
  int * COIN_RESTRICT back=fact->back;
  int irtcod = 0;
  
  /* Parameter adjustments */
  de2val--;
  
  /* Function Body */
  if (!ifRowCopy) {
    doSparse=0;
    fact->if_sparse_update=-abs(fact->if_sparse_update);
  }
  if (npivot==1) {
    fact->num_resets=0;
  }
  kpivrw = mpermu[ipivrw];
#if 0 //ndef NDEBUG
  ets_count++;
  if (ets_check>=0&&ets_count>=ets_check) {
    printf("trouble\n");
  }
#endif
  mcstrt_piv=mcstrt[kpivrw];
  /* ndenuc - top has number deleted */
  if (fact->ndenuc) {
    first_dense_mcstrt = mcstrt[fact->first_dense];
    last_dense_mcstrt  = mcstrt[fact->last_dense];
  } else {
    first_dense_mcstrt=0;
    last_dense_mcstrt=0;
  }
  {
    int kdnspt = fact->nnetas - fact->nnentl;
    
    i1 = ((kdnspt - 1) + fact->R_etas_start[fact->nR_etas + 1]);
    /*i1 = -99999999;*/
    
    /* fact->R_etas_start[fact->nR_etas + 1] is -(the number of els in R) */
    nnentl = fact->nnetas - ((kdnspt - 1) + fact->R_etas_start[fact->nR_etas + 1]);
  }
  fact->demark=fact->nnentu+nnentl;
  jpivrw = SHIFT_INDEX(kpivrw);
  
#ifdef CLP_REUSE_ETAS
  double del3Orig=0.0;
#endif
  if (nuspik < 0) {
    goto L7000;
  } else if (nuspik == 0) {
    del3 = 0.;
  } else {
    del3 = 0.;
    i1 = fact->nnentu + 1;
    i2 = fact->nnentu + nuspik;
    if (fact->sortedEta) {
      /* binary search */
      if (hrowi[i2] == jpivrw) {
	/* sitting right on the end - easy */
	del3 = dluval[i2];
	--nuspik;
      } else {
	bool foundit = true;
	
	/* binary search - sort of implies hrowi is sorted */
	i = i1;
	if (hrowi[i] != jpivrw) {
	  while (1) {
	    i = (i1 + i2) >>1;
	    if (i == i1) {
	      foundit = false;
	      break;
	    }
	    if (hrowi[i] < jpivrw) {
	      i1 = i;
	    } else if (hrowi[i] > jpivrw) {
	      i2 = i;
	    }
	    else
	      break;
	  }
	}
	/* ??? what if we didn't find it? */
	
	if (foundit) {
	  del3 = dluval[i];
	  --nuspik;
	  /* remove it and move the last element into its place */
	  hrowi[i] = hrowi[nuspik + fact->nnentu+1];
	  dluval[i] = dluval[nuspik + fact->nnentu+1];
	}
      }
    } else {
      /* search */
      for (i=i1;i<=i2;i++) {
	if (hrowi[i] == jpivrw) {
	  del3 = dluval[i];
	  --nuspik;
	  /* remove it and move the last element into its place */
	  hrowi[i] = hrowi[i2];
	  dluval[i] = dluval[i2];
	  break;
	}
      }
    }
  }
#ifdef CLP_REUSE_ETAS
  del3Orig=del3;
#endif
  
  /*      OLD COLUMN POINTERS */
  /* **************************************************************** */
  if (!ifRowCopy) {
    /*       old method */
    /*       DO U */
    c_ekketju(fact,dluval, hrowi, mcstrt, hpivco_new,
	    dwork1, kpivrw,fact->first_dense,
	    fact->last_dense);
  } else {
    
    /*       could take out of old column but lets try being crude */
    /*       try taking out */
    if (fact->xe2adr != 0&&doSparse) {
      
      /*
       * There is both a column and row representation of U.
       * For each row in the kpivrw'th column of the col U rep,
       * find its position in the U row rep and remove it
       * by overwriting it with the last element.
       */
      int k1x = mcstrt[kpivrw];
      int nel = hrowi[k1x];	/* yes, this is the nel, for the pivot */
      int k2x = k1x + nel;
      
      for (k = k1x + 1; k <= k2x; ++k) {
	int irow = UNSHIFT_INDEX(hrowi[k]);
	int kx = mrstrt[irow];
	int nel = hinrow[irow]-1;
	hinrow[irow]=nel;
	
	int jlast = kx + nel;
	for (int iel=kx;iel<jlast;iel++) {
	  if (kpivrw==hcoli[iel]) {
	    hcoli[iel] = hcoli[jlast];
	    de2val[iel] = de2val[jlast];
	    break;
	  }
	}
      }
    } else if (ifRowCopy) {
      /* still take out */
      int k1x = mcstrt[kpivrw];
      int nel = hrowi[k1x];	/* yes, this is the nel, for the pivot */
      int k2x = k1x + nel;
      
      for (k = k1x + 1; k <= k2x; ++k) {
	int irow = UNSHIFT_INDEX(hrowi[k]);
	int kx = mrstrt[irow];
	int nel = hinrow[irow]-1;
	hinrow[irow]=nel;
	int jlast = kx + nel ;
	for (;kx<jlast;kx++) {
	  if (kpivrw==hcoli[kx]) {
	    hcoli[kx] = hcoli[jlast];
	    break;
	  }
	}
      }
    }
    
    /*       add to row version */
    /* the updated column (alpha_p) was written to entries
     * nnentu+1..nnentu+nuspik by routine c_ekkftrn_ft.
     * That was just an intermediate value of the usual ftrn.
     */
    i1 = fact->nnentu + 1;
    i2 = fact->nnentu + nuspik;
    int * COIN_RESTRICT eta_last=mpermu+nrow*2+3;
    int * COIN_RESTRICT eta_next=eta_last+nrow+2;
    if (fact->xe2adr == 0||!doSparse) {
      /* we have column indices by row, but not the actual values */
      for (iel = i1; iel <= i2; ++iel) {
	int irow = UNSHIFT_INDEX(hrowi[iel]);
	int iput = hinrow[irow];
	int kput = mrstrt[irow];
	int nextRow=eta_next[irow];
	assert (kput>0);
	kput += iput;
	if (kput < mrstrt[nextRow]) {
	  /* there is room - append the pivot column;
	   * this corresponds making alpha_p the rightmost column of U (p. 268)*/
	  hinrow[irow] = iput + 1;
	  hcoli[kput] = kpivrw;
	} else {
	  /* no room - switch off */
	  doSparse=0;
	  /* possible kpivrw 1 */
	  k1 = mrstrt[kpivrw];
	  mrstrt[1]=-1;
	  fact->rows_ok = false;
	  goto L1226;
	}
      }
    } else {
      if (! doSparse) {
	/* we have both column indices and values by row */
	/* just like loop above, but with extra assign to de2val */
	for (iel = i1; iel <= i2; ++iel) {
	  int irow = UNSHIFT_INDEX(hrowi[iel]);
	  int iput = hinrow[irow];
	  int kput = mrstrt[irow];
	  int nextRow=eta_next[irow];
	  assert (kput>0);
	  kput += iput;
	  if (kput < mrstrt[nextRow]) {
	    hinrow[irow] = iput + 1;
	    hcoli[kput] = kpivrw;
	    de2val[kput] = dluval[iel];
	  } else {
	    /* no room - switch off */
	    doSparse=0;
	    /* possible kpivrw 1 */
	    k1 = mrstrt[kpivrw];
	    mrstrt[1]=-1;
	    fact->rows_ok = false;
	    goto L1226;
	  }
	}
      } else {
	for (iel = i1; iel <= i2; ++iel) {
	  int j,k;
	  int irow = UNSHIFT_INDEX(hrowi[iel]);
	  int iput = hinrow[irow];
	  k=mrstrt[irow]+iput;
	  j=eta_next[irow];
	  if (k >= mrstrt[j]) {
	    /* no room - can we make some? */
	    int klast=eta_last[nrow+1];
	    int jput=mrstrt[klast]+hinrow[klast]+2;
	    int distance=mrstrt[nrow+1]-jput;
	    if (iput+1<distance) {
	      /* this presumably copies the row to the end */
	      int jn,jl;
	      int kstart=mrstrt[irow];
	      int nin=hinrow[irow];
	      /* out */
	      jn=eta_next[irow];
	      jl=eta_last[irow];
	      eta_next[jl]=jn;
	      eta_last[jn]=jl;
	      /* in */
	      eta_next[klast]=irow;
	      eta_last[nrow+1]=irow;
	      eta_last[irow]=klast;
	      eta_next[irow]=nrow+1;
	      mrstrt[irow]=jput;
#if 0
	      memcpy(&hcoli[jput],&hcoli[kstart],nin*sizeof(int));
	      memcpy(&de2val[jput],&de2val[kstart],nin*sizeof(double));
#else
	      c_ekkscpy(nin,hcoli+kstart,hcoli+jput);
	      c_ekkdcpy(nin,
		      (de2val+kstart),(de2val+jput));
#endif
	      k=jput+iput;
	    } else {
	      /* shuffle down */
	      int spare=(fact->nnetas-fact->nnentu-fact->nnentl-3);
	      if (spare>nrow<<1) {
		/* presumbly, this compacts the rows */
		int jrow,jput;
		if (1) {
		  if (fact->num_resets<1000000) {
		    int etasize =CoinMax(4*fact->nnentu+
					 (fact->nnetas-fact->nnentl)+1000,fact->eta_size);
		    if (ifrows) {
		      fact->num_resets++;
		      if (npivot>40&&fact->num_resets<<4>npivot) {
			fact->eta_size=static_cast<int>(1.05*fact->eta_size);
			fact->num_resets=1000000;
		      }
		    } else {
		      fact->eta_size=static_cast<int>(1.1*fact->eta_size);
		      fact->num_resets=1000000;
		    }
		    fact->eta_size=CoinMin(fact->eta_size,etasize);
		    if (fact->maxNNetas>0&&fact->eta_size>
			fact->maxNNetas) {
		      fact->eta_size=fact->maxNNetas;
		    }
		  }
		}
		jrow=eta_next[0];
		jput=1;
		for (j=0;j<nrow;j++) {
		  int k,nin=hinrow[jrow];
		  k=mrstrt[jrow];
		  mrstrt[jrow]=jput;
		  for (;nin;nin--) {
		    hcoli[jput]=hcoli[k];
		    de2val[jput++]=de2val[k++];
		  }
		  jrow=eta_next[jrow];
		}
		if (spare>nrow<<3) {
		  spare=3;
		} else if (spare>nrow<<2) {
		  spare=1;
		} else {
		  spare=0;
		} 
		jput+=nrow*spare;;
		jrow=eta_last[nrow+1];
		for (j=0;j<nrow;j++) {
		  int k,nin=hinrow[jrow];
		  k=mrstrt[jrow]+nin;
		  jput-=spare;
		  for (;nin;nin--) {
		    hcoli[--jput]=hcoli[--k];
		    de2val[jput]=de2val[k];
		  }
		  mrstrt[jrow]=jput;
		  jrow=eta_last[jrow];
		}
		/* set up for copy below */
		k=mrstrt[irow]+iput;
	      } else {
		/* no room - switch off */
		doSparse=0;
		/* possible kpivrw 1 */
		k1 = mrstrt[kpivrw];
		mrstrt[1]=-1;
		fact->rows_ok = false;
		goto L1226;
	      }
	    }
	  }
	  /* now we have room - append the new value */
	  hinrow[irow] = iput + 1;
	  hcoli[k] = kpivrw;
	  de2val[k] = dluval[iel];
	}
      }
    }
    
    /*       TAKE OUT ALL ELEMENTS IN PIVOT ROW */
    k1 = mrstrt[kpivrw];
    
  L1226:
    {
      int k2 = k1 + hinrow[kpivrw] - 1;
      
      /* "delete" the row */
      hinrow[kpivrw] = 0;
      j = 0;
      if (doSparse) {
	/* remove pivot row entries from the corresponding columns */
	for (k = k1; k <= k2; ++k) {
	  int icol = hcoli[k];
	  int kx = mcstrt[icol];
	  int nel = hrowi[kx];
	  for (iel = kx + 1; iel <= kx+nel; iel ++) {
	    if (hrowi[iel] == jpivrw) {
	      break;
	    }
	  }
	  if (iel <= kx+nel) {
	    /* this has to happen, right?? */
	    
	    /* copy the element into a temporary */
	    dwork1[icol] = dluval[iel];
	    mpt2[nincol++]=icol;
	    /*nonzero[icol-1]=1;*/
	    
	    hrowi[kx]=nel-1;	/* column is shorter by one */
	    j=1;
	    hrowi[iel]=hrowi[kx+nel];
	    dluval[iel]=dluval[kx+nel];
#ifdef CLP_REUSE_ETAS
	    hrowi[kx+nel]=jpivrw;
	    dluval[kx+nel]=dwork1[icol];
#endif
	  }
	}
	if (j != 0) {
	  /* now compute r', the new R transform */
	  orig_nincol=c_ekkbtju_sparse(fact, dwork1,
				     mpt2, nincol,
				     spare);
	  dwork1[kpivrw]=0.0;
	}
      } else {
	/* row version isn't ok (?) */
	for (k = k1; k <= k2; ++k) {
	  int icol = hcoli[k];
	  int kx = mcstrt[icol];
	  int nel = hrowi[kx];
	  j = kx+nel;
	  int iel;
	  for (iel=kx+1;iel<=j;iel++) {
	    if (hrowi[iel]==jpivrw)
	      break;
	  }
	  dwork1[icol] = dluval[iel];
	  if (kx<first_dense_mcstrt || kx>last_dense_mcstrt) {
	    hrowi[kx] = nel - 1;	/* shorten column */
	    /* not packing - presumably column isn't sorted */
	    hrowi[iel] = hrowi[j];
	    dluval[iel] = dluval[j];
#ifdef CLP_REUSE_ETAS
	    hrowi[j]=jpivrw;
	    dluval[j]=dwork1[icol];
#endif
	  } else {
	    /* dense element - just zero it */
	    dluval[iel]=0.0;
	  }
	}
	if (j != 0) {
	  /* Find first nonzero */
	  int ipiv = hpivco_new[kpivrw];
	  while(ipiv<=nrow) {
	    if (!dwork1[ipiv]) {
	      ipiv=hpivco_new[ipiv];
	    } else {
	      break;
	    }
	  }
	  if (ipiv<=nrow) {
	    /*       DO U */
	    /* now compute r', the new R transform */
	    c_ekkbtju(fact, dwork1,
		      ipiv);
	  }
	}
      }
    }
  }
  
  if (kpivrw==fact->first_dense) {
    /* increase until valid pivot */
    fact->first_dense=hpivco_new[fact->first_dense];
  } else if (kpivrw==fact->last_dense) {
    fact->last_dense=back[fact->last_dense];
  }
  if (fact->first_dense==fact->last_dense) {
    fact->ndenuc=0;
    fact->first_dense=0;
    fact->last_dense=-1;
  }
  if (! (ifRowCopy && j==0)) {
    
    /*     increase amount of work on Etas */
    
    /* **************************************************************** */
    /*       DO ROW ETAS IN L */
    {
      if (!doSparse) {
	dwork1[kpivrw] = 0.;
#if 0	
	orig_nincol=c_ekksczr(fact,nrow, dwork1, mpt2);
	del3=c_ekkputl(fact, mpt2, dwork1, del3, 
		     orig_nincol, nuspik);
#else
	orig_nincol=c_ekkputl2(fact,
		      dwork1, &del3, 
		     nuspik);
#endif
      } else {
	del3=c_ekkputl(fact, mpt2,
		      dwork1, del3, 
		     orig_nincol, nuspik);
      }
    }
    if (orig_nincol != 0) {
      /* STORE AS A ROW VECTOR */
      int n = fact->nR_etas+1;
      int i1 = fact->R_etas_start[n];
      fact->nR_etas=n;
      fact->R_etas_start[n + 1] = i1 - orig_nincol;
      hpivco[fact->nR_etas + nrow+3] = kpivrw;
    }
  }
  
  /*       CHECK DEL3 AGAINST DALPHA/DOUT */
  {
    int kx = mcstrt[kpivrw];
    double dout = dluval[kx];
    double dcheck = fabs(dalpha / dout);
    double difference=0.0;
    if (fabs(del3) > CoinMin(1.0e-8,fact->drtpiv*0.99999)) {
      double checkTolerance;
      if ( fact->npivots < 2 ) {
	checkTolerance = 1.0e-5;
      } else if ( fact->npivots < 10 ) {
	checkTolerance = 1.0e-6;
      } else if ( fact->npivots < 50 ) {
	checkTolerance = 1.0e-8;
      } else {
	checkTolerance = 1.0e-9;
      }
      difference = fabs(1.0-fabs(del3)/dcheck);
      if (difference > 0.1*checkTolerance) {
	if (difference < checkTolerance||
	    (difference<1.0e-7&&fact->npivots>=50)) {
	  irtcod=1;
#ifdef PRINT_DEBUG
	  printf("mildly bad %g after %d pivots, etsj %g ftncheck %g ftnalpha %g\n",
		 difference,fact->npivots,del3,dcheck,dalpha);
#endif    
	} else {
	  irtcod=2;
#ifdef PRINT_DEBUG
	  printf("bad %g after %d pivots, etsj %g ftncheck %g ftnalpha %g\n",
		 difference,fact->npivots,del3,dcheck,dalpha);
#endif    
	}
      }
    } else {
      irtcod=2;
#ifdef PRINT_DEBUG
      printf("bad small %g after %d pivots, etsj %g ftncheck %g ftnalpha %g\n",
	     difference,fact->npivots,del3,dcheck,dalpha);
#endif    
    }
    if (irtcod>1)
      goto L8000;
    fact->npivots++;
  }
  
  mcstrt[kpivrw] = fact->nnentu;
#ifdef CLP_REUSE_ETAS
  {
    int * putSeq = fact->xrsadr+2*fact->nrowmx+2;
    int * position = putSeq+fact->maxinv;
    int * putStart = position+fact->maxinv;
    putStart[fact->nrow+fact->npivots-1]=fact->nnentu;
  }
#endif
  dluval[fact->nnentu] = 1. / del3;	/* new pivot */
  hrowi[fact->nnentu] = nuspik;	/* new nelems */
#ifndef NDEBUG
  {
    int lastSlack = fact->lastSlack;
    int firstDo=hpivco_new[lastSlack];
    int ipiv=hpivco_new[0];
    int now = fact->numberSlacks;
    if (now) {
      while (1) {
	if (ipiv>fact->nrow||ipiv==firstDo)
	  break;
	assert (c_ekk_IsSet(fact->bitArray,ipiv));
	ipiv=hpivco_new[ipiv];
      }
      if (ipiv<=fact->nrow) {
	while (1) {
	  if (ipiv>fact->nrow)
	    break;
	  assert (!c_ekk_IsSet(fact->bitArray,ipiv));
	  ipiv=hpivco_new[ipiv];
	}
      }
    }
  }
#endif
  {
    /* do new hpivco */
    int inext=hpivco_new[kpivrw];
    int iback=back[kpivrw];
    if (inext!=nrow+1) {
      int ilast=back[nrow+1];
      hpivco_new[iback]=inext;
      back[inext]=iback;
      assert (hpivco_new[ilast]==nrow+1);
      hpivco_new[ilast]=kpivrw;
      back[kpivrw]=ilast;
      hpivco_new[kpivrw]=nrow+1;
      back[nrow+1]=kpivrw;
    }
  }
  {
    int lastSlack = fact->lastSlack;
    int now = fact->numberSlacks;
    if (now&&mcstrt_piv<=mcstrt[lastSlack]) {
      if (c_ekk_IsSet(fact->bitArray,kpivrw)) {
	/*printf("piv %d lastSlack %d\n",mcstrt_piv,lastSlack);*/
	fact->numberSlacks--;	
	now--;
	/* one less slack */
	c_ekk_Unset(fact->bitArray,kpivrw);
	if (now&&kpivrw==lastSlack) {
	  int i;
	  int ipiv;
	  ipiv=hpivco_new[0];
	  for (i=0;i<now-1;i++)
	    ipiv=hpivco_new[ipiv];
	  lastSlack=ipiv;
	  assert (c_ekk_IsSet(fact->bitArray,ipiv));
	  assert (!c_ekk_IsSet(fact->bitArray,hpivco_new[ipiv])||hpivco_new[ipiv]>fact->nrow);
	  fact->lastSlack = lastSlack;
	} else if (!now) {
	  fact->lastSlack=0;
	}
      }
    }
    fact->firstNonSlack=hpivco_new[lastSlack];
#ifndef NDEBUG
    {
      int lastSlack = fact->lastSlack;
      int firstDo=hpivco_new[lastSlack];
      int ipiv=hpivco_new[0];
      int now = fact->numberSlacks;
      if (now) {
	while (1) {
	  if (ipiv>fact->nrow||ipiv==firstDo)
	    break;
	  assert (c_ekk_IsSet(fact->bitArray,ipiv));
	  ipiv=hpivco_new[ipiv];
	}
	if (ipiv<=fact->nrow) {
	  while (1) {
	    if (ipiv>fact->nrow)
	      break;
	    assert (!c_ekk_IsSet(fact->bitArray,ipiv));
	    ipiv=hpivco_new[ipiv];
	  }
	}
      }
    }
#endif
  }
  fact->nnentu += nuspik;
#ifdef CLP_REUSE_ETAS
  if (fact->first_dense>=fact->last_dense) {
    // save
    fact->nnentu++;
    dluval[fact->nnentu]=del3Orig;
    hrowi[fact->nnentu]=kpivrw;
    int * putSeq = fact->xrsadr+2*fact->nrowmx+2;
    int * position = putSeq+fact->maxinv;
    int * putStart = position+fact->maxinv;
    int nnentu_at_factor=putStart[fact->nrow]&0x7fffffff;
    //putStart[fact->nrow+fact->npivots]=fact->nnentu+1;
    int where;
    if (mcstrt_piv<nnentu_at_factor) {
      // original LU
      where=kpivrw-1;
    } else {
      // could do binary search
      int * look = putStart+fact->nrow;
      for (where=fact->npivots-1;where>=0;where--) {
	if (mcstrt_piv==(look[where]&0x7fffffff))
	  break;
      }
      assert (where>=0);
      where += fact->nrow;
    }
    position[fact->npivots-1]=where;
    if (orig_nincol == 0) {
      // flag
      putStart[fact->nrow+fact->npivots-1] |= 0x80000000;
    }
  }
#endif
  {
    int kdnspt = fact->nnetas - fact->nnentl;
    
    /* fact->R_etas_start[fact->nR_etas + 1] is -(the number of els in R) */
    nnentl = fact->nnetas - ((kdnspt - 1) + fact->R_etas_start[fact->nR_etas + 1]);
  }
  fact->demark = (fact->nnentu + nnentl) - fact->demark;
  
  /*     if need to redo row version */
  if (! fact->rows_ok&&fact->first_dense>=fact->last_dense) {
    int extraSpace=10000;
    int spareSpace;
    if (fact->if_sparse_update>0) {
      spareSpace=(fact->nnetas-fact->nnentu-fact->nnentl);
    } else {
      /* missing out nnentl stuff */
      spareSpace=fact->nnetas-fact->nnentu;
    }
    /*       save clean row copy if enough room */
    nroom = spareSpace / nrow;
    
    if ((fact->nnentu<<3)>150*fact->maxinv) {
      extraSpace=150*fact->maxinv;
    } else {
      extraSpace=fact->nnentu<<3;
    }
    
    ifrows = false;
    if (fact->nnetas>fact->nnentu+fact->nnentl+extraSpace) {
      ifrows = true;
    }
    if (nroom < 5) {
      ifrows = false;
    }
    
    if (nroom > CoinMin(50, fact->maxinv - (fact->iterno - fact->iterin))) {
      ifrows = true;
    }
    
#ifdef PRINT_DEBUGx
    printf(" redoing row copy %d %d %d\n",ifrows,nroom,spareSpace);
#endif
    if (1) {
      if (fact->num_resets<1000000) {
	if (ifrows) {
	  fact->num_resets++;
	  if (npivot>40&&fact->num_resets<<4>npivot) {
	    fact->eta_size=static_cast<int>(1.05*fact->eta_size);
	    fact->num_resets=1000000;
	  }
	} else {
	  fact->eta_size=static_cast<int>(1.1*fact->eta_size);
	  fact->num_resets=1000000;
	}
	if (fact->maxNNetas>0&&fact->eta_size>
	    fact->maxNNetas) {
	  fact->eta_size=fact->maxNNetas;
	}
      }
    }
    fact->rows_ok = ifrows;
    if (ifrows) {
      int ibase = 1;
      c_ekkizero(nrow,&hinrow[1]);
      for (i = 1; i <= nrow; ++i) {
	int kx = mcstrt[i];
	int nel = hrowi[kx];
	int kcs = kx + 1;
	int kce = kx + nel;
	for (kc = kcs; kc <= kce; ++kc) {
	  int irow = UNSHIFT_INDEX(hrowi[kc]);
	  if (dluval[kc]) {
	    hinrow[irow]++;
	  }
	}
      }
      int * eta_last=mpermu+nrow*2+3;
      int * eta_next=eta_last+nrow+2;
      eta_next[0]=1;
      for (i = 1; i <= nrow; ++i) {
	eta_next[i]=i+1;
	eta_last[i]=i-1;
	mrstrt[i] = ibase;
	ibase = ibase + hinrow[i] + nroom;
	hinrow[i] = 0;
      }
      eta_last[nrow+1]=nrow;
      //eta_next[nrow+1]=nrow+2;
      mrstrt[nrow+1]=ibase;
      if (fact->xe2adr == 0) {
	for (i = 1; i <= nrow; ++i) {
	  int kx = mcstrt[i];
	  int nel = hrowi[kx];
	  int kcs = kx + 1;
	  int kce = kx + nel;
	  for (kc = kcs; kc <= kce; ++kc) {
	    if (dluval[kc]) {
	      int irow = UNSHIFT_INDEX(hrowi[kc]);
	      int iput = hinrow[irow];
	      assert (irow);
	      hcoli[mrstrt[irow] + iput] = i;
	      hinrow[irow] = iput + 1;
	    }
	  }
	}
      } else {
	for (i = 1; i <= nrow; ++i) {
	  int kx = mcstrt[i];
	  int nel = hrowi[kx];
	  int kcs = kx + 1;
	  int kce = kx + nel;
	  for (kc = kcs; kc <= kce; ++kc) {
	    int irow = UNSHIFT_INDEX(hrowi[kc]);
	    int iput = hinrow[irow];
	    hcoli[mrstrt[irow] + iput] = i;
	    de2val[mrstrt[irow] + iput] = dluval[kc];
	    hinrow[irow] = iput + 1;
	  }
	}
      }
    } else {
      mrstrt[1] = 0;
      if (fact->if_sparse_update>0&&fact->iterno-fact->iterin>100) {
	goto L7000;
      }
    }
  }
  goto L8000;
  
  /*       OUT OF SPACE - COULD PACK DOWN */
 L7000:
  irtcod = 1;
#ifdef PRINT_DEBUG
  printf(" out of space\n");
#endif    
  if (1) {
    if ((npivot<<3)<fact->nbfinv) {
      /* low on space */
      if (npivot<10) {
	fact->eta_size=fact->eta_size<<1;
      } else {
	double ratio=fact->nbfinv;
	double ratio2=npivot<<3;
	ratio=ratio/ratio2;
	if (ratio>2.0) {
	  ratio=2.0;
	} /* endif */
	fact->eta_size=static_cast<int>(ratio*fact->eta_size);
      } /* endif */
    } else {
      fact->eta_size=static_cast<int>(1.05*fact->eta_size);
    } /* endif */
    if (fact->maxNNetas>0&&fact->eta_size>
	fact->maxNNetas) {
      fact->eta_size=fact->maxNNetas;
    }
  }
  
  /* ================= IF ERROR SHOULD WE GET RID OF LAST ITERATION??? */
 L8000:
  
  *nuspikp = nuspik;
#ifdef MORE_DEBUG
  for (int i=1;i<=fact->nrow;i++) {
    int kx=mcstrt[i];
    int nel=hrowi[kx];
    for (int j=0;j<nel;j++) {
      assert (i!=hrowi[j+kx+1]);
    }
  }
#endif
#ifdef CLP_REUSE_ETAS
  fact->save_nnentu=fact->nnentu;
#endif
  return (irtcod);
} /* c_ekketsj */
static void c_ekkftj4p(COIN_REGISTER2 const EKKfactinfo * COIN_RESTRICT2 fact, 
		     double * COIN_RESTRICT dwork1, int firstNonZero)
{
  /* this is where the L factors start, because this is the place
   * where c_ekktria starts laying them down (see initialization of xnetal).
   */
  int lstart=fact->lstart;
  const int * COIN_RESTRICT hpivco	= fact->kcpadr;
  int firstLRow = hpivco[lstart];
  if (firstNonZero>firstLRow) {
    lstart += firstNonZero-firstLRow;
  }
  assert (firstLRow==fact->firstLRow);
  int jpiv=hpivco[lstart];
  const double * COIN_RESTRICT dluval	= fact->xeeadr;
  const int * COIN_RESTRICT hrowi	= fact->xeradr;
  const int * COIN_RESTRICT mcstrt	= fact->xcsadr+lstart;
  int ndo=fact->xnetal-lstart;
  int i, iel;
  
  /* find first non-zero */
  for (i=0;i<ndo;i++) {
    if (dwork1[i+jpiv]!=0.0)
      break;
  }
  for (; i < ndo; ++i) {
    double dv = dwork1[i+jpiv];
    
    if (dv != 0.) {
      int kce1 = mcstrt[i + 1] ;
      
      for (iel = mcstrt[i]; iel > kce1; --iel) {
	int irow0 = hrowi[iel];
	SHIFT_REF(dwork1, irow0) += dv * dluval[iel];
      }
    }
  }
  
} /* c_ekkftj4p */

/*
 * This version is more efficient for input columns that are sparse.
 * It is instructive to consider the case of an especially sparse column,
 * which is a slack.  The slack for row r has exactly one non-zero element,
 * in row r, which is +-1.0.  Let pr = mpermu[r].
 * In this case, nincol==1 and mpt[0] == pr on entry.
 * if mpt[0] == pr <= jpiv
 * then this slack is completely unaffected by L;
 *	this is reflected by the fact that save_where = last
 *	after the first loop, so none of the remaining loops
 *	ever execute,
 * else if mpt[0] == pr > jpiv, but pr-jpiv > ndo
 * then the slack is also unaffected by L, this time because
 *	its row is "after" L.  During factorization, it may
 *	be the case that the first part of the basis is upper
 *	triangular (c_ekktria), but it may also be the case that the
 *	last part of the basis is upper triangular (in which case the
 *	L triangle gets "chopped off" on the right).  In both cases,
 *	no L entries are required.  Since in this case the tests
 *	(i<=ndo) will fail (and dwork1[ipiv]==1.0), the code will
 *	do nothing.
 * else if mpt[0] == pr > jpiv and pr-jpiv <= ndo
 * then the slack *is* affected by L.
 *	the for-loop inside the second while-loop will discover
 *	that none of the factors for the corresponding column of L
 *	are non-zero in the slack column, so last will not be incremented.
 *	We multiply the eta-vector, and the last loop does nothing.
 */
static int c_ekkftj4_sparse(COIN_REGISTER2 const EKKfactinfo * COIN_RESTRICT2 fact,
		   double * COIN_RESTRICT dwork1, int * COIN_RESTRICT mpt,
		   int nincol,int * COIN_RESTRICT spare)
{
  const int nrow	= fact->nrow;
  /* this is where the L factors start, because this is the place
   * where c_ekktria starts laying them down (see initialization of xnetal).
   */
  int lstart=fact->lstart;
  const int * COIN_RESTRICT hpivco	= fact->kcpadr;
  const double * COIN_RESTRICT dluval	= fact->xeeadr;
  const int * COIN_RESTRICT hrowi	= fact->xeradr;
  const int * COIN_RESTRICT mcstrt	= fact->xcsadr+lstart-1;
  double tolerance = fact->zeroTolerance;
  int jpiv=hpivco[lstart]-1;
  char * COIN_RESTRICT nonzero=fact->nonzero;
  int ndo=fact->xnetalval;
  int k,nStack;
  int nList=0;
  int iPivot;
  int * COIN_RESTRICT list = spare;
  int * COIN_RESTRICT stack = spare+nrow;
  int * COIN_RESTRICT next = stack+nrow;
  double dv;
  int iel;
  int nput=0,kput=nrow;
  int check=jpiv+ndo+1;
  const int * COIN_RESTRICT mcstrt2 = mcstrt-jpiv;
  
  for (k=0;k<nincol;k++) {
    nStack=1;
    iPivot=mpt[k];
    if (nonzero[iPivot]!=1&&iPivot>jpiv&&iPivot<check) {
      stack[0]=iPivot;
      next[0]=mcstrt2[iPivot+1]+1;
      while (nStack) {
	int kPivot,j;
	/* take off stack */
	kPivot=stack[--nStack];
	if (nonzero[kPivot]!=1&&kPivot>jpiv&&kPivot<check) {
	  j=next[nStack];
	  if (j>mcstrt2[kPivot]) {
	    /* finished so mark */
	    list[nList++]=kPivot;
	    nonzero[kPivot]=1;
	  } else {
	    kPivot=UNSHIFT_INDEX(hrowi[j]);
	    /* put back on stack */
	    next[nStack++] ++;
	    if (!nonzero[kPivot]) {
	      /* and new one */
	      stack[nStack]=kPivot;
	      nonzero[kPivot]=2;
	      next[nStack++]=mcstrt2[kPivot+1]+1;
	    }
	  }
	} else if (kPivot>=check) {
	  list[--kput]=kPivot;
	  nonzero[kPivot]=1;
	}
      }
    } else if (nonzero[iPivot]!=1) {
      /* nothing to do (except check size at end) */
      list[--kput]=iPivot;
      nonzero[iPivot]=1;
    }
  }
  for (k=nList-1;k>=0;k--) {
    double dv;
    iPivot = list[k];
    dv = dwork1[iPivot];
    nonzero[iPivot]=0;
    if (fabs(dv) > tolerance) {
      /* the same code as in c_ekkftj4p */
      int kce1 = mcstrt2[iPivot + 1];
      for (iel = mcstrt2[iPivot]; iel > kce1; --iel) {
	int irow0 = hrowi[iel];
	SHIFT_REF(dwork1, irow0) += dv * dluval[iel];
      }
      mpt[nput++]=iPivot;
    } else {
      dwork1[iPivot]=0.0;	/* force to zero, not just near zero */
    }
  }
  /* check remainder */
  for (k=kput;k<nrow;k++) {
    iPivot = list[k];
    nonzero[iPivot]=0;
    dv = dwork1[iPivot];
    if (fabs(dv) > tolerance) {
      mpt[nput++]=iPivot;
    } else {
      dwork1[iPivot]=0.0;	/* force to zero, not just near zero */
    }
  }
  
  return (nput);
} /* c_ekkftj4 */
/*
 * This applies the R transformations of the F-T LU update procedure,
 * equation 3.11 on p. 270 in the 1972 Math Programming paper.
 * Note that since the non-zero off-diagonal elements are in a row,
 * multiplying an R by a column is a reduction, not like applying
 * L or U.
 *
 * Note that this may introduce new non-zeros in dwork1,
 * since an hpivco entry may correspond to a zero element,
 * and that some non-zeros in dwork1 may be cancelled.
 */
static int c_ekkftjl_sparse3(COIN_REGISTER2 const EKKfactinfo * COIN_RESTRICT2 fact,
		    double * COIN_RESTRICT dwork1, 
		    int * COIN_RESTRICT mpt,
		    int * COIN_RESTRICT hput, double * COIN_RESTRICT dluput ,
		    int nincol)
{
  int i;
  int knext;
  int ipiv;
  double dv;
  const double * COIN_RESTRICT dluval = fact->R_etas_element+1;
  const int * COIN_RESTRICT hrowi = fact->R_etas_index+1;
  const int * COIN_RESTRICT mcstrt = fact->R_etas_start;
  int ndo=fact->nR_etas;
  double tolerance = fact->zeroTolerance;
  const int * COIN_RESTRICT hpivco = fact->hpivcoR;
  /* and make cleaner */
  hput++;
  dluput++;
  
  /* DO ANY ROW TRANSFORMATIONS */
  
  /* Function Body */
  /* mpt has correct list of nonzeros */
  if (ndo != 0) {
    knext = mcstrt[1];
    for (i = 1; i <= ndo; ++i) {
      int k1 = knext;	/* == mcstrt[i] */
      int iel;
      ipiv = hpivco[i];
      dv = dwork1[ipiv];
      bool onList = (dv!=0.0);
      knext = mcstrt[i + 1];
      
      for (iel = knext ; iel < k1; ++iel) {
	int irow = hrowi[iel];
	dv += SHIFT_REF(dwork1, irow) * dluval[iel];
      }
      /* (1) if dwork[ipiv] == 0.0, then this may add a non-zero.
       * (2) if dwork[ipiv] != 0.0, then this may cancel out a non-zero.
       */
      if (onList) {
	if (fabs(dv) > tolerance) {
	  dwork1[ipiv]=dv;
	} else {
	  dwork1[ipiv] = 1.0e-128;
	}
      } else {
	if (fabs(dv) > tolerance) {
	  /* put on list if not there */
	  mpt[nincol++]=ipiv;
	  dwork1[ipiv]=dv;
	} 
      }
    }
  }
  knext=0;
  for (i=0; i<nincol; i++) {
    ipiv=mpt[i];
    dv=dwork1[ipiv];
    if (fabs(dv) > tolerance) {
      hput[knext]=SHIFT_INDEX(ipiv);
      dluput[knext]=dv;
      mpt[knext++]=ipiv;
    } else {
      dwork1[ipiv]=0.0;
    }
  }
  return knext;
} /* c_ekkftjl */

static int c_ekkftjl_sparse2(COIN_REGISTER2 const EKKfactinfo * COIN_RESTRICT2 fact,
		    double * COIN_RESTRICT dwork1, 
		    int * COIN_RESTRICT mpt,
		    int nincol)
{
  double tolerance = fact->zeroTolerance;
  const double * COIN_RESTRICT dluval = fact->R_etas_element+1;
  const int * COIN_RESTRICT hrowi = fact->R_etas_index+1;
  const int * COIN_RESTRICT mcstrt = fact->R_etas_start;
  int ndo=fact->nR_etas;
  const int * COIN_RESTRICT hpivco = fact->hpivcoR;
  int i;
  int knext;
  int ipiv;
  double dv;
  
  /* DO ANY ROW TRANSFORMATIONS */
  
  /* Function Body */
  /* mpt has correct list of nonzeros */
  if (ndo != 0) {
    knext = mcstrt[1];
    for (i = 1; i <= ndo; ++i) {
      int k1 = knext;	/* == mcstrt[i] */
      int iel;
      ipiv = hpivco[i];
      dv = dwork1[ipiv];
      bool onList = (dv!=0.0);
      knext = mcstrt[i + 1];
      
      for (iel = knext ; iel < k1; ++iel) {
	int irow = hrowi[iel];
	dv += SHIFT_REF(dwork1, irow) * dluval[iel];
      }
      /* (1) if dwork[ipiv] == 0.0, then this may add a non-zero.
       * (2) if dwork[ipiv] != 0.0, then this may cancel out a non-zero.
       */
      if (onList) {
	if (fabs(dv) > tolerance) {
	  dwork1[ipiv]=dv;
	} else {
	  dwork1[ipiv] = 1.0e-128;
	}
      } else {
	if (fabs(dv) > tolerance) {
	  /* put on list if not there */
	  mpt[nincol++]=ipiv;
	  dwork1[ipiv]=dv;
	} 
      }
    }
  }
  knext=0;
  for (i=0; i<nincol; i++) {
    ipiv=mpt[i];
    dv=dwork1[ipiv];
    if (fabs(dv) > tolerance) {
      mpt[knext++]=ipiv;
    } else {
      dwork1[ipiv]=0.0;
    }
  }
  return knext;
} /* c_ekkftjl */

static void c_ekkftjl(COIN_REGISTER2 const EKKfactinfo * COIN_RESTRICT2 fact,
	     double * COIN_RESTRICT dwork1)
{
  double tolerance = fact->zeroTolerance;
  const double * COIN_RESTRICT dluval = fact->R_etas_element+1;
  const int * COIN_RESTRICT hrowi = fact->R_etas_index+1;
  const int * COIN_RESTRICT mcstrt = fact->R_etas_start;
  int ndo=fact->nR_etas;
  const int * COIN_RESTRICT hpivco = fact->hpivcoR;
  int i;
  int knext;
  
  /* DO ANY ROW TRANSFORMATIONS */
  
  /* Function Body */
  if (ndo != 0) {
    /*
     * The following three lines are here just to ensure that this
     * new formulation of the loop has exactly the same effect
     * as the original.
     */
    {
      int ipiv = hpivco[1];
      double dv = dwork1[ipiv];
      dwork1[ipiv] = (fabs(dv) > tolerance) ? dv : 0.0;
    }
    
    knext = mcstrt[1];
    for (i = 1; i <= ndo; ++i) {
      int k1 = knext;	/* == mcstrt[i] */
      int ipiv = hpivco[i];
      double dv = dwork1[ipiv];
      int iel;
      //#define UNROLL3 2
#ifndef UNROLL3
#if CLP_OSL==2||CLP_OSL==3
#define UNROLL3 2
#else
#define UNROLL3 1
#endif
#endif
      knext = mcstrt[i + 1];
      
#if UNROLL3<2
      for (iel = knext ; iel < k1; ++iel) {
	int irow = hrowi[iel];
	dv += SHIFT_REF(dwork1, irow) * dluval[iel];
      }
#else
      iel = knext;
      if (((k1-knext)&1)!=0) {
	int irow = hrowi[iel];
	dv += SHIFT_REF(dwork1, irow) * dluval[iel];
	iel++;
      }
      for ( ; iel < k1; iel+=2) {
	int irow0 = hrowi[iel];
	double dval0 = dluval[iel];
	int irow1 = hrowi[iel+1];
	double dval1 = dluval[iel+1];
	dv += SHIFT_REF(dwork1, irow0) * dval0;
	dv += SHIFT_REF(dwork1, irow1) * dval1; 
      }
#endif
      /* (1) if dwork[ipiv] == 0.0, then this may add a non-zero.
       * (2) if dwork[ipiv] != 0.0, then this may cancel out a non-zero.
       */
      dwork1[ipiv] = (fabs(dv) > tolerance) ? dv : 0.0;
    }
  }
} /* c_ekkftjl */
/* this assumes it is ok to reference back[loop_limit] */
/* another 3 seconds from a ~570 second run can be trimmed
 * by using two routines, one with scan==true and the other false,
 * since that eliminates the branch instructions involving them
 * entirely.  This was how the code was originally written.
 * However, I'm still hoping that eventually we can use
 * C++ templates to do that for us automatically.
 */
static void
c_ekkftjup_scan_aux(COIN_REGISTER2 const EKKfactinfo * COIN_RESTRICT2 fact,
		  double * COIN_RESTRICT dwork1, double * COIN_RESTRICT dworko ,
		  int loop_limit, int *ip, int ** mptp)
{
  const double * COIN_RESTRICT dluval	= fact->xeeadr+1;
  const int * COIN_RESTRICT hrowi	= fact->xeradr+1;
  const int * COIN_RESTRICT mcstrt	= fact->xcsadr;
  const int * COIN_RESTRICT hpivro	= fact->krpadr;
  const int * COIN_RESTRICT back=fact->back;
  double tolerance = fact->zeroTolerance;
  int ipiv = *ip;
  double dv = dwork1[ipiv];
  
  int * mptX = *mptp;
  assert (mptX);
  while (ipiv != loop_limit) {
    int next_ipiv = back[ipiv];
    
    dwork1[ipiv] = 0.0;
#ifndef UNROLL4
#define UNROLL4 2
#endif
    /* invariant:  dv == dwork1[ipiv] */
    
    /* in the case of world.mps with dual, this condition is true
     * only 20-60% of the time. */
    if (fabs(dv) > tolerance) {
      const int kx = mcstrt[ipiv];
      const int nel = hrowi[kx-1];
      const double dpiv = dluval[kx-1];
#if UNROLL4>1
      const int * hrowi2=hrowi+kx;
      const int * hrowi2end=hrowi2+nel;
      const double * dluval2=dluval+kx;
#else
      int iel;
#endif
      
      dv*=dpiv;
      
      /*
       * The following loop is the unrolled version of this:
       *
       * for (iel = kx+1; iel <= kx + nel; iel++) {
       *   SHIFT_REF(dwork1, hrowi[iel]) -= dv * dluval[iel];
       * }
       */
#if UNROLL4<2
      iel = kx;
      if (nel&1) {
	int irow = hrowi[iel];
	double dval=dluval[iel];
	SHIFT_REF(dwork1, irow) -= dv*dval;
	iel++;
      }
      for (; iel < kx + nel; iel+=2) {
	int irow0 = hrowi[iel];
	int irow1 = hrowi[iel+1];
	double dval0=dluval[iel];
	double dval1=dluval[iel+1];
	double d0=SHIFT_REF(dwork1, irow0);
	double d1=SHIFT_REF(dwork1, irow1);
	
	d0-=dv*dval0;
	d1-=dv*dval1;
	SHIFT_REF(dwork1, irow0)=d0;
	SHIFT_REF(dwork1, irow1)=d1;
      } /* end loop */
#else
      if ((nel&1)!=0) {
	int irow = *hrowi2;
	double dval=*dluval2;
	SHIFT_REF(dwork1, irow) -= dv*dval;
	hrowi2++;
	dluval2++;
      }
      for (; hrowi2 < hrowi2end; hrowi2 +=2,dluval2 +=2) {
	int irow0 = hrowi2[0];
	int irow1 = hrowi2[1];
	double dval0=dluval2[0];
	double dval1=dluval2[1];
	double d0=SHIFT_REF(dwork1, irow0);
	double d1=SHIFT_REF(dwork1, irow1);
	
	d0-=dv*dval0;
	d1-=dv*dval1;
	SHIFT_REF(dwork1, irow0)=d0;
	SHIFT_REF(dwork1, irow1)=d1;
      }
#endif
      /* put this down here so that dv is less likely to cause a stall */
      if (fabs(dv) >= tolerance) {
	int iput=hpivro[ipiv];
	dworko[iput]=dv;
	*mptX++=iput-1;
      }
    }
    
    dv = dwork1[next_ipiv];
    ipiv=next_ipiv;
  } /* endwhile */
  
  *mptp = mptX;
  *ip = ipiv;
}
static void c_ekkftjup_aux3(COIN_REGISTER2 const EKKfactinfo * COIN_RESTRICT2 fact,
			     double * COIN_RESTRICT dwork1, double * COIN_RESTRICT dworko,
			  const int * COIN_RESTRICT back,
			  const int * COIN_RESTRICT hpivro,
			  int *ipivp, int loop_limit,
			  int **mptXp)
  
{
  double tolerance = fact->zeroTolerance;
  int ipiv = *ipivp;
  if (ipiv!=loop_limit) {
    int *mptX = *mptXp;
    
    double dv = dwork1[ipiv];
    
    do {
      int next_ipiv = back[ipiv];
      double next_dv=dwork1[next_ipiv];
      
      dwork1[ipiv]=0.0;
      
      if (fabs(dv)>=tolerance) {
	int iput=hpivro[ipiv];
	dworko[iput]=dv;
	*mptX++=iput-1;
      }
      
      ipiv = next_ipiv;
      dv = next_dv;
    } while (ipiv!=loop_limit);
    
    *mptXp = mptX;
    *ipivp = ipiv;
  }
}
static void c_ekkftju_dense(const double *dluval, 
			    const int * COIN_RESTRICT hrowi, 
			    const int * COIN_RESTRICT mcstrt,
			    const int * COIN_RESTRICT back,
			    double * COIN_RESTRICT dwork1, 
			    int * start, int last,
		   int offset , double *densew)
{
  int ipiv=*start;
  
  while (ipiv>last ) {
    const int ipiv1=ipiv;
    double dv1=dwork1[ipiv1];
    ipiv=back[ipiv];
    if (fabs(dv1) > 1.0e-14) {
      const int kx1 = mcstrt[ipiv1];
      const int nel1 = hrowi[kx1-1];
      const double dpiv1 = dluval[kx1-1];
      
      int iel,k;
      const int n1=offset+ipiv1;	/* number in dense part */
      
      const int nsparse1=nel1-n1;
      const int k1=kx1+nsparse1;
      const double *dlu1=&dluval[k1];
      
      int ipiv2=back[ipiv1];
      const int nskip=ipiv1-ipiv2;
      
      dv1*=dpiv1;
      
      dwork1[ipiv1]=dv1;
      
      for (k = n1 - (nskip-1) -1; k >=0 ; k--) {
        const double dval = dv1*dlu1[k];
	double dv2=densew[k]-dval;
        ipiv=back[ipiv];
        if (fabs(dv2) > 1.0e-14) {
          const int kx2 = mcstrt[ipiv2];
          const int nel2 = hrowi[kx2-1];
          const double dpiv2 = dluval[kx2-1];
	  
          /* number in dense part is k */
          const int nsparse2=nel2-k;
	  
          const int k2=kx2+nsparse2;
          const double *dlu2=&dluval[k2];
	  
          dv2*=dpiv2;
          densew[k]=dv2;	/* was dwork1[ipiv2]=dv2; */
	  
          k--;
	  
	  /*
	   * The following loop is the unrolled version of:
	   *
	   * for (; k >= 0; k--) {
	   *   densew[k]-=dv1*dlu1[k]+dv2*dlu2[k];
	   * }
	   */
          if ((k&1)==0) {
            densew[k]-=dv1*dlu1[k]+dv2*dlu2[k];
            k--;
          }
          for (; k >=0 ; k-=2) {
            double da,db;
            da=densew[k];
            db=densew[k-1];
            da-=dv1*dlu1[k];
            db-=dv1*dlu1[k-1];
            da-=dv2*dlu2[k];
            db-=dv2*dlu2[k-1];
            densew[k]=da;
            densew[k-1]=db;
          }
	  /* end loop */
	  
	  /*
	   * The following loop is the unrolled version of:
	   *
	   * for (iel=kx2+nsparse2-1; iel >= kx2; iel--) {
	   *   SHIFT_REF(dwork1, hrowi[iel]) -= dv2*dluval[iel];
	   * }
	   */
	  iel=kx2+nsparse2-1;
          if ((nsparse2&1)!=0) {
            int irow0 = hrowi[iel];
            double dval=dluval[iel];
            SHIFT_REF(dwork1,irow0) -= dv2*dval;
            iel--;
          }
          for (; iel >=kx2 ; iel-=2) {
            double dval0 = dluval[iel];
	    double dval1 = dluval[iel-1];
            int irow0 = hrowi[iel];
            int irow1 = hrowi[iel-1];
            double d0 = SHIFT_REF(dwork1, irow0);
            double d1 = SHIFT_REF(dwork1, irow1);
	    
            d0-=dv2*dval0;
            d1-=dv2*dval1;
	    SHIFT_REF(dwork1, irow0) = d0;
	    SHIFT_REF(dwork1, irow1) = d1;
          }
	  /* end loop */
	  
        } else {
          densew[k]=0.0;
          /* skip if next deleted */
          k-=ipiv2-ipiv-1;
          ipiv2=ipiv;
          if (ipiv<last) {
	    k--;
	    for (; k >=0 ; k--) {
	      double dval;
	      dval=dv1*dlu1[k];
	      densew[k]=densew[k]-dval;
	    }
	  }
        }
      }
      
      /*
       * The following loop is the unrolled version of:
       *
       * for (iel=kx1+nsparse1-1; iel >= kx1; iel--) {
       *   SHIFT_REF(dwork1, hrowi[iel]) -= dv1*dluval[iel];
       * }
       */
      iel=kx1+nsparse1-1;
      if ((nsparse1&1)!=0) {
        int irow0 = hrowi[iel];
        double dval=dluval[iel];
        SHIFT_REF(dwork1, irow0) -= dv1*dval;
        iel--;
      }
      for (; iel >=kx1 ; iel-=2) {
        double dval0=dluval[iel];
        double dval1=dluval[iel-1];
        int irow0 = hrowi[iel];
        int irow1 = hrowi[iel-1];
        double d0=SHIFT_REF(dwork1, irow0);
        double d1=SHIFT_REF(dwork1, irow1);
	
        d0-=dv1*dval0;
        d1-=dv1*dval1;
        SHIFT_REF(dwork1, irow0) = d0;
        SHIFT_REF(dwork1, irow1) = d1;
      }
      /* end loop */
    } else {
      dwork1[ipiv1]=0.0;
    } /* endif */
  } /* endwhile */
  *start=ipiv;
}

/* do not use return value if mpt==0 */
/* using dual, this is usually called via c_ekkftrn_ft, from c_ekksdul
 * (so mpt is non-null).
 * it is generally called every iteration, but sometimes several iterations
 * are skipped (null moves?).
 *
 * generally, back[i] == i-1 (initialized in c_ekkshfv towards the end).
 */
static int c_ekkftjup(COIN_REGISTER3 const EKKfactinfo * COIN_RESTRICT2 fact,
	     double * COIN_RESTRICT dwork1, int last,
	     double * COIN_RESTRICT dworko , int * COIN_RESTRICT mpt)
{
  const double * COIN_RESTRICT dluval	= fact->xeeadr;
  const int * COIN_RESTRICT hrowi	= fact->xeradr;
  const int * COIN_RESTRICT mcstrt	= fact->xcsadr;
  const int * COIN_RESTRICT hpivro	= fact->krpadr;
  double tolerance = fact->zeroTolerance;
  int ndenuc=fact->ndenuc;
  const int first_dense=fact->first_dense;
  const int last_dense=fact->last_dense;
  int i;
  int * mptX = mpt;
  
  const int nrow		= fact->nrow;
  const int * COIN_RESTRICT back=fact->back;
  int ipiv=back[nrow+1];
  
  if (last_dense>first_dense&&mcstrt[ipiv]>=mcstrt[last_dense]) {
    c_ekkftjup_scan_aux(fact,
		      dwork1, dworko, last_dense, &ipiv,
		      &mptX);
    
    {
      int j;
      int n=0;
      const int firstDense	= nrow- ndenuc+1;
      double *densew = &dwork1[firstDense];
      int offset;
      
      /* check first dense to see where in triangle it is */
      int last=first_dense;
      const int k1=mcstrt[last];
      const int k2=k1+hrowi[k1];
      
      for (j=k2; j>k1; j--) {
        int irow = UNSHIFT_INDEX(hrowi[j]);
        if (irow<firstDense) {
          break;
        } else {
#ifdef DEBUG
          if (irow!=last-1) {
            abort();
          }
#endif
          last=irow;
          n++;
        }
      }
      offset=n-first_dense;
      i=ipiv;
      /* loop counter i may be modified by this call */
      c_ekkftju_dense(&dluval[1],&hrowi[1],mcstrt,back,
		    dwork1, &i, first_dense,offset,densew);
      
      c_ekkftjup_aux3(fact,dwork1, dworko, back, hpivro, &ipiv, i, &mptX);
    }
  }
  
  c_ekkftjup_scan_aux(fact,
		    dwork1, dworko, last, &ipiv,
		    &mptX);
  
  if (ipiv!=0) {
    double dv = dwork1[ipiv];
    
    do {
      int next_ipiv = back[ipiv];
      double next_dv=dwork1[next_ipiv];
      
      dwork1[ipiv]=0.0;
      
      if (fabs(dv)>=tolerance) {
	int iput=hpivro[ipiv];
	dworko[iput]=-dv;
	*mptX++=iput-1;
      }
      
      ipiv = next_ipiv;
      dv = next_dv;
    } while (ipiv!=0);
    
  }
  return static_cast<int>(mptX-mpt);
}
/* this assumes it is ok to reference back[loop_limit] */
/* another 3 seconds from a ~570 second run can be trimmed
 * by using two routines, one with scan==true and the other false,
 * since that eliminates the branch instructions involving them
 * entirely.  This was how the code was originally written.
 * However, I'm still hoping that eventually we can use
 * C++ templates to do that for us automatically.
 */
static void
c_ekkftjup_scan_aux_pack(COIN_REGISTER2 const EKKfactinfo * COIN_RESTRICT2 fact,
		       double * COIN_RESTRICT dwork1, double * COIN_RESTRICT dworko ,
		       int loop_limit, int *ip, int ** mptp)
{
  double tolerance = fact->zeroTolerance;
  const double *dluval	= fact->xeeadr+1;
  const int *hrowi	= fact->xeradr+1;
  const int *mcstrt	= fact->xcsadr;
  const int *hpivro	= fact->krpadr;
  const int * back=fact->back;
  int ipiv = *ip;
  double dv = dwork1[ipiv];
  
  int * mptX = *mptp;
#if 0
  int inSlacks=0;
  int lastSlack;
  if (fact->numberSlacks!=0)
    lastSlack=fact->lastSlack;
  else
    lastSlack=0;
  if (c_ekk_IsSet(fact->bitArray,ipiv)) {
    printf("already in slacks - ipiv %d\n",ipiv);
    inSlacks=1;
    return;
  }
#endif
  assert (mptX);
  while (ipiv != loop_limit) {
    int next_ipiv = back[ipiv];
#if 0
    if (ipiv==lastSlack) {
      printf("now in slacks - ipiv %d\n",ipiv);
      inSlacks=1;
      break;
    }
    if (inSlacks) {
      assert (c_ekk_IsSet(fact->bitArray,ipiv));
      assert (dluval[mcstrt[ipiv]-1]==-1.0);
      assert (hrowi[mcstrt[ipiv]-1]==0);
    }
#endif
    dwork1[ipiv] = 0.0;
    /* invariant:  dv == dwork1[ipiv] */
    
    /* in the case of world.mps with dual, this condition is true
     * only 20-60% of the time. */
    if (fabs(dv) > tolerance) {
      const int kx = mcstrt[ipiv];
      const int nel = hrowi[kx-1];
      const double dpiv = dluval[kx-1];
#ifndef UNROLL5
#define UNROLL5 2
#endif
#if UNROLL5>1
      const int * hrowi2=hrowi+kx;
      const int * hrowi2end=hrowi2+nel;
      const double * dluval2=dluval+kx;
#else
      int iel;
#endif
      
      dv*=dpiv;
      
      /*
       * The following loop is the unrolled version of this:
       *
       * for (iel = kx+1; iel <= kx + nel; iel++) {
       *   SHIFT_REF(dwork1, hrowi[iel]) -= dv * dluval[iel];
       * }
       */
#if UNROLL5<2
      iel = kx;
      if (nel&1) {
	int irow = hrowi[iel];
	double dval=dluval[iel];
	SHIFT_REF(dwork1, irow) -= dv*dval;
	iel++;
      }
      for (; iel < kx + nel; iel+=2) {
	int irow0 = hrowi[iel];
	int irow1 = hrowi[iel+1];
	double dval0=dluval[iel];
	double dval1=dluval[iel+1];
	double d0=SHIFT_REF(dwork1, irow0);
	double d1=SHIFT_REF(dwork1, irow1);
	
	d0-=dv*dval0;
	d1-=dv*dval1;
	SHIFT_REF(dwork1, irow0)=d0;
	SHIFT_REF(dwork1, irow1)=d1;
      } /* end loop */
#else
      if ((nel&1)!=0) {
	int irow = *hrowi2;
	double dval=*dluval2;
	SHIFT_REF(dwork1, irow) -= dv*dval;
	hrowi2++;
	dluval2++;
      }
      for (; hrowi2 < hrowi2end; hrowi2 +=2,dluval2 +=2) {
	int irow0 = hrowi2[0];
	int irow1 = hrowi2[1];
	double dval0=dluval2[0];
	double dval1=dluval2[1];
	double d0=SHIFT_REF(dwork1, irow0);
	double d1=SHIFT_REF(dwork1, irow1);
	
	d0-=dv*dval0;
	d1-=dv*dval1;
	SHIFT_REF(dwork1, irow0)=d0;
	SHIFT_REF(dwork1, irow1)=d1;
      }
#endif
      /* put this down here so that dv is less likely to cause a stall */
      if (fabs(dv) >= tolerance) {
	int iput=hpivro[ipiv];
	*dworko++=dv;
	*mptX++=iput-1;
      }
    }
    
    dv = dwork1[next_ipiv];
    ipiv=next_ipiv;
  } /* endwhile */
  
  *mptp = mptX;
  *ip = ipiv;
}
static void c_ekkftjup_aux3_pack(COIN_REGISTER2 const EKKfactinfo * COIN_RESTRICT2 fact,
				  double * COIN_RESTRICT dwork1, double * COIN_RESTRICT dworko,
			       const int * COIN_RESTRICT back,
			       const int * COIN_RESTRICT hpivro,
			       int *ipivp, int loop_limit,
			       int **mptXp)
  
{
  double tolerance = fact->zeroTolerance;
  int ipiv = *ipivp;
  if (ipiv!=loop_limit) {
    int *mptX = *mptXp;
    
    double dv = dwork1[ipiv];
    do {
      int next_ipiv = back[ipiv];
      double next_dv=dwork1[next_ipiv];
      
      dwork1[ipiv]=0.0;
      
      if (fabs(dv)>=tolerance) {
	int iput=hpivro[ipiv];
	*dworko++=dv;
	*mptX++=iput-1;
      }
      
      ipiv = next_ipiv;
      dv = next_dv;
    } while (ipiv!=loop_limit);
    
    *mptXp = mptX;
    
    *ipivp = ipiv;
  }
}

/* do not use return value if mpt==0 */
/* using dual, this is usually called via c_ekkftrn_ft, from c_ekksdul
 * (so mpt is non-null).
 * it is generally called every iteration, but sometimes several iterations
 * are skipped (null moves?).
 *
 * generally, back[i] == i-1 (initialized in c_ekkshfv towards the end).
 */
static int c_ekkftjup_pack(COIN_REGISTER3 const EKKfactinfo * COIN_RESTRICT2 fact,
		  double * COIN_RESTRICT dwork1, int last,
		  double * COIN_RESTRICT dworko , int * COIN_RESTRICT mpt)
{
  const double * COIN_RESTRICT dluval	= fact->xeeadr;
  const int * COIN_RESTRICT hrowi	= fact->xeradr;
  const int * COIN_RESTRICT mcstrt	= fact->xcsadr;
  const int * COIN_RESTRICT hpivro	= fact->krpadr;
  double tolerance = fact->zeroTolerance;
  int ndenuc=fact->ndenuc;
  const int first_dense=fact->first_dense;
  const int last_dense=fact->last_dense;
  int * mptX = mpt; int * mptY = mpt;
  
  const int nrow		= fact->nrow;
  const int * COIN_RESTRICT back=fact->back;
  int ipiv=back[nrow+1];
  assert (mpt);
  
  if (last_dense>first_dense&&mcstrt[ipiv]>=mcstrt[last_dense]) {
    c_ekkftjup_scan_aux_pack(fact,
			   dwork1, dworko, last_dense, &ipiv,
			   &mptX );
    /* adjust */
    dworko+= (mptX-mpt);
    mpt=mptX;
    {
      int j;
      int n=0;
      const int firstDense	= nrow- ndenuc+1;
      double *densew = &dwork1[firstDense];
      int offset;
      
      /* check first dense to see where in triangle it is */
      int last=first_dense;
      const int k1=mcstrt[last];
      const int k2=k1+hrowi[k1];
      
      for (j=k2; j>k1; j--) {
        int irow = UNSHIFT_INDEX(hrowi[j]);
        if (irow<firstDense) {
          break;
        } else {
#ifdef DEBUG
          if (irow!=last-1) {
            abort();
          }
#endif
          last=irow;
          n++;
        }
      }
      offset=n-first_dense;
      int ipiv2=ipiv;
      /* loop counter i may be modified by this call */
      c_ekkftju_dense(&dluval[1],&hrowi[1],mcstrt,back,
		    dwork1, &ipiv2, first_dense,offset,densew);
      
      c_ekkftjup_aux3_pack(fact,dwork1, dworko, back, hpivro, &ipiv, ipiv2,&mptX);
      /* adjust dworko */
      dworko += (mptX-mpt);
      mpt=mptX;
    }
  }
  
  c_ekkftjup_scan_aux_pack(fact,
			 dwork1, dworko, last, &ipiv,
			 &mptX );
  /* adjust dworko */
  dworko += (mptX-mpt);
  while (ipiv!=0) {
    double dv = dwork1[ipiv];
    int next_ipiv = back[ipiv];
    
    dwork1[ipiv]=0.0;
    
    if (fabs(dv)>=tolerance) {
      int iput=hpivro[ipiv];
      *dworko++=-dv;
      *mptX++=iput-1;
    }
    
    ipiv = next_ipiv;
  }
  
  return static_cast<int>(mptX-mptY);
}
static int c_ekkftju_sparse_a(COIN_REGISTER2 const EKKfactinfo * COIN_RESTRICT2 fact,
			    int * COIN_RESTRICT mpt,
			    int nincol,int * COIN_RESTRICT spare)
{
  const int * COIN_RESTRICT hrowi	= fact->xeradr+1;
  const int * COIN_RESTRICT mcstrt	= fact->xcsadr;
  const int nrow		= fact->nrow;
  char * COIN_RESTRICT nonzero=fact->nonzero;
  
  int k,nStack,kx,nel;
  int nList=0;
  int iPivot;
  /*int kkk=nincol;*/
  int * COIN_RESTRICT list = spare;
  int * COIN_RESTRICT stack = spare+nrow;
  int * COIN_RESTRICT next = stack+nrow;
  for (k=0;k<nincol;k++) {
    nStack=1;
    iPivot=mpt[k];
    stack[0]=iPivot;
    next[0]=0;
    while (nStack) {
      int kPivot,j;
      /* take off stack */
      kPivot=stack[--nStack];
      if (nonzero[kPivot]!=1) {
	kx = mcstrt[kPivot];
	nel = hrowi[kx-1];
	j=next[nStack];
	if (j==nel) {
	  /* finished so mark */
	  list[nList++]=kPivot;
	  nonzero[kPivot]=1;
	} else {
	  kPivot=hrowi[kx+j];
	  /* put back on stack */
	  next[nStack++] ++;
	  if (!nonzero[kPivot]) {
	    /* and new one */
	    stack[nStack]=kPivot;
	    nonzero[kPivot]=2;
	    next[nStack++]=0;
	    /*kkk++;*/
	  }
	}
      }
    }
  }
  return (nList);
}
static int c_ekkftju_sparse_b(COIN_REGISTER2 const EKKfactinfo * COIN_RESTRICT2 fact,
			    double * COIN_RESTRICT dwork1, 
			    double * COIN_RESTRICT dworko , int * COIN_RESTRICT mpt,
			    int nList,int * COIN_RESTRICT spare)
{
  
  const double * COIN_RESTRICT dluval	= fact->xeeadr+1;
  const int * COIN_RESTRICT hrowi	= fact->xeradr+1;
  const int * COIN_RESTRICT mcstrt	= fact->xcsadr;
  const int * COIN_RESTRICT hpivro	= fact->krpadr;
  double tolerance = fact->zeroTolerance;
  char * COIN_RESTRICT nonzero=fact->nonzero;
  int i,k,kx,nel;
  int iPivot;
  /*int kkk=nincol;*/
  int * COIN_RESTRICT list = spare;
  i=nList-1;
  nList=0;
  for (;i>=0;i--) {
    double dpiv;
    double dv;
    iPivot = list[i];
    /*printf("pivot %d %d\n",i,iPivot);*/
    dv=dwork1[iPivot];
    kx = mcstrt[iPivot];
    nel = hrowi[kx-1];
    dwork1[iPivot]=0.0;
    dpiv = dluval[kx-1];
    dv*=dpiv;
    nonzero[iPivot]=0;
    iPivot=hpivro[iPivot];
    if (fabs(dv)>=tolerance) {
      *dworko++=dv;
      mpt[nList++]=iPivot-1;
      for (k = kx; k < kx+nel; k++) {
        double dval;
        double dd;
        int irow = hrowi[k];
        dval=dluval[k];
        dd=dwork1[irow];
        dd-=dv*dval;
        dwork1[irow]=dd;
      }
    }
  }
  return (nList);
}
/* dwork1 = (B^-1)dwork1;
 * I think dpermu[1..nrow+1] is zeroed on exit (?)
 * I don't think it is expected to have any particular value on entry (?)
 */
int c_ekkftrn(COIN_REGISTER const EKKfactinfo * COIN_RESTRICT2 fact, 
	    double * COIN_RESTRICT dwork1, 
	    double * COIN_RESTRICT dpermu, int * COIN_RESTRICT mpt,int numberNonZero)
{
  const int * COIN_RESTRICT mpermu = fact->mpermu;
  int lastNonZero;
  int firstNonZero = c_ekkshfpi_list2(mpermu+1, dwork1+1, dpermu, mpt,
				      numberNonZero,&lastNonZero);
  if (fact->nnentl&&lastNonZero>=fact->firstLRow) {
    /* dpermu = (L^-1)dpermu */
    c_ekkftj4p(fact, dpermu, firstNonZero);
  }
  
  
  int lastSlack;
  
  /* dpermu = (R^-1) dpermu */
  c_ekkftjl(fact, dpermu);
  
  assert (fact->numberSlacks!=0||!fact->lastSlack);
  lastSlack=fact->lastSlack;
  
  /* dwork1 = (U^-1)dpermu; dpermu zeroed (?) */
  return c_ekkftjup(fact, 
		  dpermu, lastSlack, dwork1, mpt);
  
} /* c_ekkftrn */

int c_ekkftrn_ft(COIN_REGISTER EKKfactinfo * COIN_RESTRICT2 fact,
	       double * COIN_RESTRICT dwork1_ft, int * COIN_RESTRICT mpt_ft, int *nincolp_ft)
{
  double * COIN_RESTRICT dpermu_ft = fact->kadrpm;
  int * COIN_RESTRICT spare = reinterpret_cast<int *>(fact->kp1adr);
  int nincol	= *nincolp_ft;
  
  int nuspik;
  double * COIN_RESTRICT dluvalPut = fact->xeeadr+fact->nnentu+1;
  int * COIN_RESTRICT hrowiPut	= fact->xeradr+fact->nnentu+1;
  
  const int nrow	= fact->nrow;
  /* mpermu contains the permutation */
  const int * COIN_RESTRICT mpermu=fact->mpermu;
  
  int lastSlack;
  
  int kdnspt = fact->nnetas - fact->nnentl;
  bool isRoom = (fact->nnentu + (nrow << 1) < (kdnspt - 2) 
		 + fact->R_etas_start[fact->nR_etas + 1]);
  
  /* say F-T will be sorted */
  fact->sortedEta=1;
  
  assert (fact->numberSlacks!=0||!fact->lastSlack);
  lastSlack=fact->lastSlack;
#ifdef CLP_REUSE_ETAS
  bool skipStuff = (fact->reintro>=0);
  
  int save_nR_etas=fact->nR_etas;
  int * save_hpivcoR=fact->hpivcoR;
  int * save_R_etas_start=fact->R_etas_start;
  if (skipStuff) {
    // just move
    int * putSeq = fact->xrsadr+2*fact->nrowmx+2;
    int * position = putSeq+fact->maxinv;
    int * putStart = position+fact->maxinv;
    memset(dwork1_ft,0,nincol*sizeof(double));
    int iPiv=fact->reintro;
    int start=putStart[iPiv]&0x7fffffff;
    int end=putStart[iPiv+1]&0x7fffffff;
    double * COIN_RESTRICT dluval	= fact->xeeadr;
    int * COIN_RESTRICT hrowi	= fact->xeradr;
    double dValue;
    if (fact->reintro<fact->nrow) {
      iPiv++;
      dValue=1.0/dluval[start++];
    } else {
      iPiv=hrowi[--end];
      dValue=dluval[end];
      start++;
      int ndoSkip=0;
      for (int i=fact->nrow;i<fact->reintro;i++) {
	if ((putStart[i]&0x80000000)==0)
	ndoSkip++;
      }
      fact->nR_etas-=ndoSkip;
      fact->hpivcoR+=ndoSkip;
      fact->R_etas_start+=ndoSkip;
    }
    dpermu_ft[iPiv]=dValue;
    if (fact->if_sparse_update>0 &&  DENSE_THRESHOLD<nrow) {
      nincol=0;
      if (dValue)
	mpt_ft[nincol++]=iPiv;
      for (int i=start;i<end;i++) {
	int iRow=hrowi[i];
	dpermu_ft[iRow]=dluval[i];
	mpt_ft[nincol++]=iRow;
      }
    } else {
      for (int i=start;i<end;i++) {
	int iRow=hrowi[i];
	dpermu_ft[iRow]=dluval[i];
      }
    }
  }
#else
  bool skipStuff = false;
#endif
  if (fact->if_sparse_update>0 &&  DENSE_THRESHOLD<nrow) {
    if (!skipStuff) {
      /* iterating so c_ekkgtcl will have list */
      /* in order for this to make sense, nonzero[1..nrow] must already be zeroed */
      c_ekkshfpi_list3(mpermu+1, dwork1_ft, dpermu_ft, mpt_ft, nincol);
      
      /* it may be the case that the basis was entirely upper-triangular */
      if (fact->nnentl) {
	nincol = 
	  c_ekkftj4_sparse(fact,
			 dpermu_ft,  mpt_ft,
			 nincol,spare);
      }
    }
    /*       DO ROW ETAS IN L */
    if (isRoom) {
      ++fact->nnentu;
      nincol=
	c_ekkftjl_sparse3(fact,
			  dpermu_ft, 
			  mpt_ft, hrowiPut,
			  dluvalPut,nincol);
      nuspik = nincol;
      /* temporary */
      /* say not sorted */
      fact->sortedEta=0;
    } else {
      /* no room */
      nuspik=-3;
      nincol=
	c_ekkftjl_sparse2(fact,
			  dpermu_ft, 
			  mpt_ft,  nincol);
    }
    /*         DO U */
    if (DENSE_THRESHOLD>nrow-fact->numberSlacks) {
      nincol = c_ekkftjup_pack(fact, 
			       dpermu_ft,lastSlack, dwork1_ft, 
			       mpt_ft);
    } else {
      nincol= c_ekkftju_sparse_a(fact,
				 mpt_ft,
				 nincol, spare);
      nincol = c_ekkftju_sparse_b(fact,
				  dpermu_ft, 
				  dwork1_ft , mpt_ft,
				  nincol, spare);
    }
  } else {
    if (!skipStuff) {
      int lastNonZero;
      int firstNonZero = c_ekkshfpi_list(mpermu+1, dwork1_ft, dpermu_ft, 
				       mpt_ft, nincol,&lastNonZero);
      if (fact->nnentl&&lastNonZero>=fact->firstLRow) {
	/* dpermu_ft = (L^-1)dpermu_ft */
	c_ekkftj4p(fact, dpermu_ft, firstNonZero);
      }
    }
    
    /* dpermu_ft = (R^-1) dpermu_ft */
    c_ekkftjl(fact, dpermu_ft);
    
    if (isRoom) {
      
      /*        fake start to allow room for pivot */
      /* dluval[fact->nnentu...] = non-zeros of dpermu_ft;
       * hrowi[fact->nnentu..] = indices of these non-zeros;
       * near-zeros in dluval flattened
       */
      ++fact->nnentu;
      nincol= c_ekkscmv(fact,fact->nrow, dpermu_ft, hrowiPut,
		      dluvalPut);
      
      /*
       * note that this is not the value of nincol determined by c_ekkftjup. 
       * For Forrest-Tomlin update we want vector before U
       * this vector will replace one in U
       */
      nuspik = nincol;
    } else {
      /* no room */
      nuspik = -3;
    }
    
    /* dwork1_ft = (U^-1)dpermu_ft; dpermu_ft zeroed (?) */
    nincol = c_ekkftjup_pack(fact, 
			   dpermu_ft, lastSlack, dwork1_ft, mpt_ft);
  }
#ifdef CLP_REUSE_ETAS
  fact->nR_etas=save_nR_etas;
  fact->hpivcoR=save_hpivcoR;
  fact->R_etas_start=save_R_etas_start;
#endif
  
  *nincolp_ft = nincol;
  return (nuspik);
} /* c_ekkftrn */
void c_ekkftrn2(COIN_REGISTER EKKfactinfo * COIN_RESTRICT2 fact, double * COIN_RESTRICT dwork1,
	     double * COIN_RESTRICT dpermu1,int * COIN_RESTRICT mpt1, int *nincolp,
	     double * COIN_RESTRICT dwork1_ft, int * COIN_RESTRICT mpt_ft, int *nincolp_ft)
{
  double * COIN_RESTRICT dluvalPut = fact->xeeadr+fact->nnentu+1;
  int * COIN_RESTRICT hrowiPut	= fact->xeradr+fact->nnentu+1;
  
  const int nrow	= fact->nrow;
  /* mpermu contains the permutation */
  const int * COIN_RESTRICT mpermu=fact->mpermu;
  
  int lastSlack;
  assert (fact->numberSlacks!=0||!fact->lastSlack);
  lastSlack=fact->lastSlack;
  
  int nincol	= *nincolp_ft;

  /* using dwork1 instead double *dpermu_ft = fact->kadrpm; */
  int * spare = reinterpret_cast<int *>(fact->kp1adr);
  
  int kdnspt = fact->nnetas - fact->nnentl;
  bool isRoom = (fact->nnentu + (nrow << 1) < (kdnspt - 2) 
		 + fact->R_etas_start[fact->nR_etas + 1]);
  /* say F-T will be sorted */
  fact->sortedEta=1;
  int lastNonZero;
  int firstNonZero = c_ekkshfpi_list2(mpermu+1, dwork1+1, dpermu1, 
				      mpt1, *nincolp,&lastNonZero);
  if (fact->nnentl&&lastNonZero>=fact->firstLRow) {
    /* dpermu1 = (L^-1)dpermu1 */
    c_ekkftj4p(fact, dpermu1, firstNonZero);
  }

#ifdef CLP_REUSE_ETAS
  bool skipStuff = (fact->reintro>=0);
  int save_nR_etas=fact->nR_etas;
  int * save_hpivcoR=fact->hpivcoR;
  int * save_R_etas_start=fact->R_etas_start;
  if (skipStuff) {
    // just move
    int * putSeq = fact->xrsadr+2*fact->nrowmx+2;
    int * position = putSeq+fact->maxinv;
    int * putStart = position+fact->maxinv;
    memset(dwork1_ft,0,nincol*sizeof(double));
    int iPiv=fact->reintro;
    int start=putStart[iPiv]&0x7fffffff;
    int end=putStart[iPiv+1]&0x7fffffff;
    double * COIN_RESTRICT dluval	= fact->xeeadr;
    int * COIN_RESTRICT hrowi	= fact->xeradr;
    double dValue;
    if (fact->reintro<fact->nrow) {
      iPiv++;
      dValue=1.0/dluval[start++];
    } else {
      iPiv=hrowi[--end];
      dValue=dluval[end];
      start++;
      int ndoSkip=0;
      for (int i=fact->nrow;i<fact->reintro;i++) {
	if ((putStart[i]&0x80000000)==0)
	ndoSkip++;
      }
      fact->nR_etas-=ndoSkip;
      fact->hpivcoR+=ndoSkip;
      fact->R_etas_start+=ndoSkip;
    }
    dwork1[iPiv]=dValue;
    if (fact->if_sparse_update>0 &&  DENSE_THRESHOLD<nrow) {
      nincol=0;
      if (dValue)
	mpt_ft[nincol++]=iPiv;
      for (int i=start;i<end;i++) {
	int iRow=hrowi[i];
	dwork1[iRow]=dluval[i];
	mpt_ft[nincol++]=iRow;
      }
    } else {
      for (int i=start;i<end;i++) {
	int iRow=hrowi[i];
	dwork1[iRow]=dluval[i];
      }
    }
  }
#else
  bool skipStuff = false;
#endif
  if (fact->if_sparse_update>0 &&  DENSE_THRESHOLD<nrow) {
    if (!skipStuff) {
      /* iterating so c_ekkgtcl will have list */
      /* in order for this to make sense, nonzero[1..nrow] must already be zeroed */
      c_ekkshfpi_list3(mpermu+1, dwork1_ft, dwork1, mpt_ft, nincol);
      
      /* it may be the case that the basis was entirely upper-triangular */
      if (fact->nnentl) {
	nincol = 
	  c_ekkftj4_sparse(fact,
			   dwork1, mpt_ft,
			   nincol,spare);
      }
    }
    /*       DO ROW ETAS IN L */
    if (isRoom) {
      ++fact->nnentu;
      nincol=
	c_ekkftjl_sparse3(fact,
			  dwork1, 
			  mpt_ft, hrowiPut,
			  dluvalPut,
			  nincol);
      fact->nuspike = nincol;
      /* say not sorted */
      fact->sortedEta=0;
    } else {
      /* no room */
      fact->nuspike=-3;
      nincol=
	c_ekkftjl_sparse2(fact,
			  dwork1, 
			  mpt_ft, nincol);
    }
  } else {
    if (!skipStuff) {
      int lastNonZero;
      int firstNonZero = c_ekkshfpi_list(mpermu+1, dwork1_ft, dwork1, 
				       mpt_ft, nincol,&lastNonZero);
      if (fact->nnentl&&lastNonZero>=fact->firstLRow) {
	/* dpermu_ft = (L^-1)dpermu_ft */
	c_ekkftj4p(fact, dwork1, firstNonZero);
      }
    }
    c_ekkftjl(fact, dwork1);
    
    if (isRoom) {
      
      /*        fake start to allow room for pivot */
      /* dluval[fact->nnentu...] = non-zeros of dpermu_ft;
       * hrowi[fact->nnentu..] = indices of these non-zeros;
       * near-zeros in dluval flattened
       */
      ++fact->nnentu;
      nincol= c_ekkscmv(fact,fact->nrow, dwork1, 
		      hrowiPut,
		      dluvalPut);
      
      /*
       * note that this is not the value of nincol determined by c_ekkftjup. 
       * For Forrest-Tomlin update we want vector before U
       * this vector will replace one in U
       */
      fact->nuspike = nincol;
    } else {
      /* no room */
      fact->nuspike = -3;
    }
  }
#ifdef CLP_REUSE_ETAS
  fact->nR_etas=save_nR_etas;
  fact->hpivcoR=save_hpivcoR;
  fact->R_etas_start=save_R_etas_start;
#endif
  
  
  /* dpermu1 = (R^-1) dpermu1 */
  c_ekkftjl(fact, dpermu1);
  
  /*         DO U */
  if (fact->if_sparse_update<=0 ||  DENSE_THRESHOLD>nrow-fact->numberSlacks) {
    nincol = c_ekkftjup_pack(fact,
			   dwork1,lastSlack, dwork1_ft, mpt_ft);
  } else {
    nincol= c_ekkftju_sparse_a(fact,
			     mpt_ft,
			     nincol, spare);
    nincol = c_ekkftju_sparse_b(fact,
			      dwork1, 
			      dwork1_ft , mpt_ft,
			      nincol, spare);
  }
  *nincolp_ft = nincol;
  /* dwork1 = (U^-1)dpermu1; dpermu1 zeroed (?) */
  *nincolp = c_ekkftjup(fact,
		      dpermu1,lastSlack, dwork1, mpt1);

}


