/* $Id: CoinOslC.h 1585 2013-04-06 20:42:02Z stefan $ */
#ifndef COIN_OSL_C_INCLUDE
/*
  Copyright (C) 1987, 2009, International Business Machines Corporation
  and others.  All Rights Reserved.

  This code is licensed under the terms of the Eclipse Public License (EPL).
*/
#define COIN_OSL_C_INCLUDE

#ifndef CLP_OSL
#define CLP_OSL 0
#endif
#define C_EKK_GO_SPARSE 200

#ifdef HAVE_ENDIAN_H
#include <endian.h>
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define INTEL
#endif
#endif

#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define SPARSE_UPDATE
#define NO_SHIFT
#include "CoinHelperFunctions.hpp"

#include <stddef.h>
#ifdef __cplusplus
extern "C"{
#endif

int c_ekkbtrn( const EKKfactinfo *fact,
	    double *dwork1,
	    int * mpt,int first_nonzero);
int c_ekkbtrn_ipivrw( const EKKfactinfo *fact,
		   double *dwork1,
		   int * mpt, int ipivrw,int * spare);

int c_ekketsj( /*const*/ EKKfactinfo *fact,
	    double *dwork1,
	    int *mpt2, double dalpha, int orig_nincol,
	    int npivot, int *nuspikp,
	    const int ipivrw, int * spare);
int c_ekkftrn( const EKKfactinfo *fact,
	    double *dwork1,
	    double * dpermu,int * mpt, int numberNonZero);

int c_ekkftrn_ft( EKKfactinfo *fact,
	       double *dwork1, int *mpt, int *nincolp);
void c_ekkftrn2( EKKfactinfo *fact, double *dwork1,
	      double * dpermu1,int * mpt1, int *nincolp,
	     double *dwork1_ft, int *mpt_ft, int *nincolp_ft);

int c_ekklfct( EKKfactinfo *fact);
int c_ekkslcf( const EKKfactinfo *fact);
inline void c_ekkscpy(int n, const int *marr1,int *marr2)
{ CoinMemcpyN(marr1,n,marr2);}
inline void c_ekkdcpy(int n, const double *marr1,double *marr2)
{ CoinMemcpyN(marr1,n,marr2);}
int c_ekk_IsSet(const int * array,int bit);
void c_ekk_Set(int * array,int bit);
void c_ekk_Unset(int * array,int bit);

void c_ekkzero(int length, int n, void * array);
inline void c_ekkdzero(int n, double *marray)
{CoinZeroN(marray,n);}
inline void c_ekkizero(int n, int *marray)
{CoinZeroN(marray,n);}
inline void c_ekkczero(int n, char *marray)
{CoinZeroN(marray,n);}
#ifdef __cplusplus
          }
#endif

#define c_ekkscpy_0_1(s,ival,array) CoinFillN(array,s,ival)
#define c_ekks1cpy( n,marr1,marr2)  CoinMemcpyN(marr1,n, marr2)
void clp_setup_pointers(EKKfactinfo * fact);
void clp_memory(int type);
double * clp_double(int number_entries);
int * clp_int(int number_entries);
void * clp_malloc(int number_entries);
void clp_free(void * oldArray);

#define SLACK_VALUE -1.0
#define	C_EKK_REMOVE_LINK(hpiv,hin,link,ipivot)	\
  {						\
    int ipre = link[ipivot].pre;		\
    int isuc = link[ipivot].suc;		\
    if (ipre > 0) {				\
      link[ipre].suc = isuc;			\
    }						\
    if (ipre <= 0) {				\
      hpiv[hin[ipivot]] = isuc;			\
    }						\
    if (isuc > 0) {				\
      link[isuc].pre = ipre;			\
    }						\
  }

#define	C_EKK_ADD_LINK(hpiv,nzi,link, npr)	\
  {						\
    int ifiri = hpiv[nzi];			\
    hpiv[nzi] = npr;				\
    link[npr].suc = ifiri;			\
    link[npr].pre = 0;				\
    if (ifiri != 0) {				\
      link[ifiri].pre = npr;			\
    }						\
  }
#include <assert.h>
#ifdef	NO_SHIFT

#define	SHIFT_INDEX(limit)	(limit)
#define	UNSHIFT_INDEX(limit)	(limit)
#define	SHIFT_REF(arr,ind)	(arr)[ind]

#else

#define	SHIFT_INDEX(limit)	((limit)<<3)
#define	UNSHIFT_INDEX(limit)	((unsigned int)(limit)>>3)
#define	SHIFT_REF(arr,ind)	(*(double*)((char*)(arr) + (ind)))

#endif

#ifdef INTEL
#define	NOT_ZERO(x)	(((*((reinterpret_cast<unsigned char *>(&x))+7)) & 0x7F) != 0)
#else
#define	NOT_ZERO(x)	((x) != 0.0)
#endif

#define	SWAP(type,_x,_y)	{ type _tmp = (_x); (_x) = (_y); (_y) = _tmp;}

#define	UNROLL_LOOP_BODY1(code)			\
  {{code}}
#define	UNROLL_LOOP_BODY2(code)			\
  {{code} {code}}
#define	UNROLL_LOOP_BODY4(code)			\
  {{code} {code} {code} {code}}
#endif
#ifdef COIN_OSL_CMFC
/*     Return codes in IRTCOD/IRTCOD are */
/*     4: numerical problems */
/*     5: not enough space in row file */
/*     6: not enough space in column file */
/*    23: system error at label 320 */
{
#if 1
  int *hcoli	= fact->xecadr;
  double *dluval	= fact->xeeadr;
  double *dvalpv = fact->kw3adr;
  int *mrstrt	= fact->xrsadr;
  int *hrowi	= fact->xeradr;
  int *mcstrt	= fact->xcsadr;
  int *hinrow	= fact->xrnadr;
  int *hincol	= fact->xcnadr;
  int *hpivro	= fact->krpadr;
  int *hpivco	= fact->kcpadr;
#endif
  int nnentl	= fact->nnentl;
  int nnentu	= fact->nnentu;
  int kmxeta	= fact->kmxeta;
  int xnewro	= *xnewrop;
  int ncompactions	= *ncompactionsp;

  MACTION_T *maction = reinterpret_cast<MACTION_T*>(maction_void);

  int i, j, k;
  double d1;
  int j1, j2;
  int jj, kk, kr, nz, jj1, jj2, kce, kcs, kqq, npr;
  int fill, naft;
  int enpr;
  int nres, npre;
  int knpr, irow, iadd32, ibase;
  double pivot;
  int count, nznpr;
  int nlast, epivr1;
  int kipis;
  double dpivx;
  int kipie, kcpiv, knprs, knpre;
  bool cancel;
  double multip, elemnt;
  int ipivot, jpivot, epivro, epivco, lstart, nfirst;
  int nzpivj, kfill, kstart;
  int nmove, ileft;
#ifndef C_EKKCMFY
  int iput, nspare;
  int noRoomForDense=0;
  int if_sparse_update=fact->if_sparse_update;
  int ifdens = 0;
#endif
  int irtcod	= 0;
  const int nrow	= fact->nrow;

  /* Parameter adjustments */
  --maction;

  /* Function Body */
  lstart = nnetas - nnentl + 1;
  for (i = lstart; i <= nnetas; ++i) {
      hrowi[i] = SHIFT_INDEX(hcoli[i]);
  }

  for (i = 1; i <= nrow; ++i) {
    maction[i] = 0;
    mwork[i].pre = i - 1;
    mwork[i].suc = i + 1;
  }

  iadd32 = 0;
  nlast = nrow;
  nfirst = 1;
  mwork[1].pre = nrow;
  mwork[nrow].suc = 1;

  for (count = 1; count <= nrow; ++count) {

    /* Pick column singletons */
    if (! (hpivco[1] <= 0)) {
      int small_pivot = c_ekkcsin(fact,
				 rlink, clink,
				    nsingp);

      if (small_pivot) {
	irtcod = 7; /* pivot too small */
	if (fact->invok >= 0) {
	  goto L1050;
	}
      }
      if (fact->npivots >= nrow) {
	goto L1050;
      }
    }

    /* Pick row singletons */
    if (! (hpivro[1] <= 0)) {
      irtcod = c_ekkrsin(fact,
			 rlink, clink,
			 mwork,nfirst,
			 nsingp,

		     &xnewco, &xnewro,
		     &nnentu,
		     &kmxeta, &ncompactions,
			 &nnentl);
	if (irtcod != 0) {
	  if (irtcod < 0 || fact->invok >= 0) {
	    /* -5 */
	    goto L1050;
	  }
	  /* ASSERT:  irtcod == 7 - pivot too small */
	  /* why don't we return with an error? */
	}
	if (fact->npivots >= nrow) {
	    goto L1050;
	}
	lstart = nnetas - nnentl + 1;
    }

    /* Find a pivot element */
    irtcod = c_ekkfpvt(fact,
		      rlink, clink,
		     nsingp, xrejctp, &ipivot, &jpivot);
    if (irtcod != 0) {
      /* irtcod == 10 */
	goto L1050;
    }
    /*        Update list structures and prepare for numerical phase */
    c_ekkprpv(fact, rlink, clink,
		     *xrejctp, ipivot, jpivot);

    epivco = hincol[jpivot];
    ++fact->xnetal;
    mcstrt[fact->xnetal] = lstart - 1;
    hpivco[fact->xnetal] = ipivot;
    epivro = hinrow[ipivot];
    epivr1 = epivro - 1;
    kipis = mrstrt[ipivot];
    pivot = dluval[kipis];
    dpivx = 1. / pivot;
    kipie = kipis + epivr1;
    ++kipis;
#ifndef	C_EKKCMFY
    {
      double size = nrow - fact->npivots;
      if (size > GO_DENSE && (nnentu - fact->nuspike) * GO_DENSE_RATIO > size * size) {
	/* say going to dense coding */
	if (*nsingp == 0) {
	  ifdens = 1;
	}
      }
    }
#endif
    /* copy the pivot row entries into dvalpv */
    /* the maction array tells us the index into dvalpv for a given row */
    /* the alternative would be using a large array of doubles */
    for (k = kipis; k <= kipie; ++k) {
      irow = hcoli[k];
      dvalpv[k - kipis + 1] = dluval[k];
      maction[irow] = static_cast<MACTION_T>(k - kipis + 1);
    }

    /* Loop over nonzeros in pivot column */
    kcpiv = mcstrt[jpivot] - 1;
    for (nzpivj = 1; nzpivj <= epivco; ++nzpivj) {
      ++kcpiv;
      npr = hrowi[kcpiv];
      hrowi[kcpiv] = 0;	/* zero out for possible compaction later on */

      --hincol[jpivot];

      ++mcstrt[jpivot];
      /* loop invariant:  kcpiv == mcstrt[jpivot] - 1 */

      --hinrow[npr];
      enpr = hinrow[npr];
      knprs = mrstrt[npr];
      knpre = knprs + enpr;

      /* Search for element to be eliminated */
      knpr = knprs;
      while (1) {
	  UNROLL_LOOP_BODY4({
	    if (jpivot == hcoli[knpr]) {
	      break;
	    }
	    knpr++;
	  });
      }

      multip = -dluval[knpr] * dpivx;

      /* swap last entry with pivot */
      dluval[knpr] = dluval[knpre];
      hcoli[knpr] = hcoli[knpre];
      --knpre;

#if	1
      /* MONSTER_UNROLLED_CODE - see below */
      kfill = epivr1 - (knpre - knprs + 1);
      nres = ((knpre - knprs + 1) & 1) + knprs;
      cancel = false;
      d1 = 1e33;
      j1 = hcoli[nres];

      if (nres != knprs) {
	j = hcoli[knprs];
	if (maction[j] == 0) {
	  ++kfill;
	} else {
	  jj = maction[j];
	  maction[j] = static_cast<MACTION_T>(-maction[j]);
	  dluval[knprs] += multip * dvalpv[jj];
	  d1 = fabs(dluval[knprs]);
	}
      }
      j2 = hcoli[nres + 1];
      jj1 = maction[j1];
      for (kr = nres; kr < knpre; kr += 2) {
	jj2 = maction[j2];
	if ( jj1 == 0) {
	  ++kfill;
	} else {
	  maction[j1] = static_cast<MACTION_T>(-maction[j1]);
	  dluval[kr] += multip * dvalpv[jj1];
	  cancel = cancel || ! (fact->zeroTolerance < d1);
	  d1 = fabs(dluval[kr]);
	}
	j1 = hcoli[kr + 2];
	if ( jj2 == 0) {
	  ++kfill;
	} else {
	  maction[j2] = static_cast<MACTION_T>(-maction[j2]);
	  dluval[kr + 1] += multip * dvalpv[jj2];
	  cancel = cancel || ! (fact->zeroTolerance < d1);
	  d1 = fabs(dluval[kr + 1]);
	}
	jj1 = maction[j1];
	j2 = hcoli[kr + 3];
      }
      cancel = cancel || ! (fact->zeroTolerance < d1);
#else
      /*
       * This is apparently what the above code does.
       * In addition to being unrolled, the assignments to j[12] and jj[12]
       * are shifted so that the result of dereferencing maction doesn't
       * have to be used immediately afterwards for the branch test.
       * This would would cause a pipeline delay.  (The apparent dereference
       * of hcoli will be removed by the compiler using strength reduction).
       *
       * loop through the entries in the row being processed,
       * flipping the sign of the maction entries as we go along.
       * Afterwards, we look for positive entries to see what pivot
       * row entries will cause fill-in.  We count the number of fill-ins, too.
       * "cancel" says if the result of combining the pivot row with this one
       * causes an entry to get too small; if so, we discard those entries.
       */
      kfill = epivr1 - (knpre - knprs + 1);
      cancel = false;

      for (kr = knprs; kr <= knpre; kr++) {
	j1 = hcoli[kr];
	jj1 = maction[j1];
	if ( jj1 == 0) {
	  /* no entry - this pivot column entry will have to be added */
	  ++kfill;
	} else {
	  /* there is an entry for this column in the pivot row */
	  maction[j1] = -maction[j1];
	  dluval[kr] += multip * dvalpv[jj1];
	  d1 = fabs(dluval[kr]);
	  cancel = cancel || ! (fact->zeroTolerance < d1);
	}
      }
#endif
      kstart = knpre;
      fill = kfill;

      if (cancel) {
	/* KSTART is used as a stack pointer for nonzeros in factored row */
	kstart = knprs - 1;
	for (kr = knprs; kr <= knpre; ++kr) {
	  j = hcoli[kr];
	  if (fabs(dluval[kr]) > fact->zeroTolerance) {
	    ++kstart;
	    dluval[kstart] = dluval[kr];
	    hcoli[kstart] = j;
	  } else {
	    /* Remove element from column file */
	    --nnentu;
	    --hincol[j];
	    --enpr;
	    kcs = mcstrt[j];
	    kce = kcs + hincol[j];
	    for (kk = kcs; kk <= kce; ++kk) {
	      if (hrowi[kk] == npr) {
		hrowi[kk] = hrowi[kce];
		hrowi[kce] = 0;
		break;
	      }
	    }
	    /* ASSERT !(kk>kce) */
	  }
	}
	knpre = kstart;
      }
      /* Fill contains an upper bound on the amount of fill-in */
      if (fill == 0) {
	for (k = kipis; k <= kipie; ++k) {
	  maction[hcoli[k]] = static_cast<MACTION_T>(-maction[hcoli[k]]);
	}
      }
      else {
	naft = mwork[npr].suc;
	kqq = mrstrt[naft] - knpre - 1;

	if (fill > kqq) {
	  /* Fill-in exceeds space left. Check if there is enough */
	  /* space in row file for the new row. */
	  nznpr = enpr + fill;
	  if (! (xnewro + nznpr + 1 < lstart)) {
	    if (! (nnentu + nznpr + 1 < lstart)) {
	      irtcod = -5;
	      goto L1050;
	    }
	    /* idea 1 is to compress every time xnewro increases by x thousand */
	    /* idea 2 is to copy nucleus rows with a reasonable gap */
	    /* then copy each row down when used */
	    /* compressions would just be 1 remainder which eventually will */
	    /* fit in cache */
	    {
	      int iput = c_ekkrwcs(fact,dluval, hcoli, mrstrt, hinrow, mwork, nfirst);
	      kmxeta += xnewro - iput ;
	      xnewro = iput - 1;
	      ++ncompactions;
	    }

	    kipis = mrstrt[ipivot] + 1;
	    kipie = kipis + epivr1 - 1;
	    knprs = mrstrt[npr];
	  }

	  /* I think this assignment should be inside the previous if-stmt */
	  /* otherwise, it does nothing */
	  /*assert(knpre == knprs + enpr - 1);*/
	  knpre = knprs + enpr - 1;

	  /*
	   * copy this row to the end of the row file and adjust its links.
	   * The links keep track of the order of rows in memory.
	   * Rows are only moved from the middle all the way to the end.
	   */
	  if (npr != nlast) {
	    npre = mwork[npr].pre;
	    if (npr == nfirst) {
	      nfirst = naft;
	    }
	    /*             take out of chain */
	    mwork[naft].pre = npre;
	    mwork[npre].suc = naft;
	    /*             and put in at end */
	    mwork[nfirst].pre = npr;
	    mwork[nlast].suc = npr;
	    mwork[npr].pre = nlast;
	    mwork[npr].suc = nfirst;
	    nlast = npr;
	    kstart = xnewro;
	    mrstrt[npr] = kstart + 1;
	    nmove = knpre - knprs + 1;
	    ibase = kstart + 1 - knprs;
	    for (kr = knprs; kr <= knpre; ++kr) {
	      dluval[ibase + kr] = dluval[kr];
	      hcoli[ibase + kr] = hcoli[kr];
	    }
	    kstart += nmove;
	  } else {
	    kstart = knpre;
	  }

	  /* extra space ? */
	  /*
	   * The mystery of iadd32.
	   * This code assigns to xnewro, possibly using iadd32.
	   * However, in that case xnewro is assigned to just after
	   * the for-loop below, and there is no intervening reference.
	   * Therefore, I believe that this code can be entirely eliminated;
	   * it is the leftover of an interrupted or dropped experiment.
	   * Presumably, this was trying to implement the ideas about
	   * padding expressed above.
	   */
	  if (iadd32 != 0) {
	    xnewro += iadd32;
	  } else {
	    if (kstart + (nrow << 1) + 100 < lstart) {
	      ileft = ((nrow - fact->npivots + 32) & -32);
	      if (kstart + ileft * ileft + 32 < lstart) {
		iadd32 = ileft;
		xnewro = CoinMax(kstart,xnewro);
		xnewro = (xnewro & -32) + ileft;
	      } else {
		xnewro = ((kstart + 31) & -32);
	      }
	    } else {
	      xnewro = kstart;
	    }
	  }

	  hinrow[npr] = enpr;
	} else if (! (nnentu + kqq + 2 < lstart)) {
	  irtcod = -5;
	  goto L1050;
	}
	/* Scan pivot row again to generate fill in. */
	for (kr = kipis; kr <= kipie; ++kr) {
	  j = hcoli[kr];
	  jj = maction[j];
	  if (jj >0) {
	    elemnt = multip * dvalpv[jj];
	    if (fabs(elemnt) > fact->zeroTolerance) {
	      ++kstart;
	      dluval[kstart] = elemnt;
	      //printf("pivot %d at %d col %d el %g\n",
	      // npr,kstart,j,elemnt);
	      hcoli[kstart] = j;
	      ++nnentu;
	      nz = hincol[j];
	      kcs = mcstrt[j];
	      kce = kcs + nz - 1;
	      if (kce == xnewco) {
		if (xnewco + 1 >= lstart) {
		  if (xnewco + nz + 1 >= lstart) {
		    /*                  Compress column file */
		    if (nnentu + nz + 1 < lstart) {
		      xnewco = c_ekkclco(fact,hrowi, mcstrt, hincol, xnewco);
		      ++ncompactions;

		      kcpiv = mcstrt[jpivot] - 1;
		      kcs = mcstrt[j];
		      /*                  HINCOL MAY HAVE CHANGED? (JJHF) ??? */
		      nz = hincol[j];
		      kce = kcs + nz - 1;
		    } else {
		      irtcod = -5;
		      goto L1050;
		    }
		  }
		  /*              Copy column */
		  mcstrt[j] = xnewco + 1;
		  ibase = mcstrt[j] - kcs;
		  for (kk = kcs; kk <= kce; ++kk) {
		    hrowi[ibase + kk] = hrowi[kk];
		    hrowi[kk] = 0;
		  }
		  kce = xnewco + kce - kcs + 1;
		  xnewco = kce + 1;
		} else {
		  ++xnewco;
		}
	      } else if (hrowi[kce + 1] != 0) {
		/* here we use the fact that hrowi entries not "in use" are zeroed */
		if (xnewco + nz + 1 >= lstart) {
		  /* Compress column file */
		  if (nnentu + nz + 1 < lstart) {
		    xnewco = c_ekkclco(fact,hrowi, mcstrt, hincol, xnewco);
		    ++ncompactions;

		    kcpiv = mcstrt[jpivot] - 1;
		    kcs = mcstrt[j];
		    /*                  HINCOL MAY HAVE CHANGED? (JJHF) ??? */
		    nz = hincol[j];
		    kce = kcs + nz - 1;
		  } else {
		    irtcod = -5;
		    goto L1050;
		  }
		}
		/* move the column to the end of the column file */
		mcstrt[j] = xnewco + 1;
		ibase = mcstrt[j] - kcs;
		for (kk = kcs; kk <= kce; ++kk) {
		  hrowi[ibase + kk] = hrowi[kk];
		  hrowi[kk] = 0;
		}
		kce = xnewco + kce - kcs + 1;
		xnewco = kce + 1;
	      }
	      /* store element */
	      hrowi[kce + 1] = npr;
	      hincol[j] = nz + 1;
	    }
	  } else {
	    maction[j] = static_cast<MACTION_T>(-maction[j]);
	  }
	}
	if (fill > kqq) {
	  xnewro = kstart;
	}
      }
      hinrow[npr] = kstart - mrstrt[npr] + 1;
      /* Check if row or column file needs compression */
      if (! (xnewco + 1 < lstart)) {
	xnewco = c_ekkclco(fact,hrowi, mcstrt, hincol, xnewco);
	++ncompactions;

	kcpiv = mcstrt[jpivot] - 1;
      }
      if (! (xnewro + 1 < lstart)) {
	int iput = c_ekkrwcs(fact,dluval, hcoli, mrstrt, hinrow, mwork, nfirst);
	kmxeta += xnewro - iput ;
	xnewro = iput - 1;
	++ncompactions;

	kipis = mrstrt[ipivot] + 1;
	kipie = kipis + epivr1 - 1;
      }
      /* Store elementary row transformation */
      ++nnentl;
      --nnentu;
      --lstart;
      dluval[lstart] = multip;

      hrowi[lstart] = SHIFT_INDEX(npr);
#define INLINE_AFPV 3
      /* We could do this while computing values but
	 it makes it much more complex.  At least we should get
	 reasonable cache behavior by doing it each row */
#if INLINE_AFPV
      {
	int j;
	int nel, krs;
	int koff;
	int * index;
	double * els;
	nel = hinrow[npr];
	krs = mrstrt[npr];
	index=&hcoli[krs];
	els=&dluval[krs];
#if INLINE_AFPV<3
#if INLINE_AFPV==1
	double maxaij = 0.0;
	koff = 0;
	j=0;
	while (j<nel) {
	  double d = fabs(els[j]);
	  if (maxaij < d) {
	    maxaij = d;
	    koff=j;
	  }
	  j++;
	}
#else
	assert (nel);
	koff=0;
	double maxaij=fabs(els[0]);
	for (j=1;j<nel;j++) {
	  double d = fabs(els[j]);
	  if (maxaij < d) {
	    maxaij = d;
	    koff=j;
	  }
	}
#endif
#else
	double maxaij = 0.0;
	koff = 0;
	j=0;
	if ((nel&1)!=0) {
	  maxaij=fabs(els[0]);
	  j=1;
	}

	while (j<nel) {
	  UNROLL_LOOP_BODY2({
	      double d = fabs(els[j]);
	      if (maxaij < d) {
		maxaij = d;
		koff=j;
	      }
	      j++;
	    });
	}
#endif
	SWAP(int, index[koff], index[0]);
	SWAP(double, els[koff], els[0]);
      }
#endif

      {
	int nzi = hinrow[npr];
	if (nzi > 0) {
	  C_EKK_ADD_LINK(hpivro, nzi, rlink, npr);
	}
      }
    }

    /* after pivot move biggest to first in each row */
#if INLINE_AFPV==0
    int nn = mcstrt[fact->xnetal] - lstart + 1;
    c_ekkafpv(hrowi+lstart, hcoli, dluval, mrstrt, hinrow, nn);
#endif

    /* Restore work array */
    for (k = kipis; k <= kipie; ++k) {
      maction[hcoli[k]] = 0;
    }

    if (*xrejctp > 0) {
      for (k = kipis; k <= kipie; ++k) {
	int j = hcoli[k];
	int nzj = hincol[j];
	if (! (nzj <= 0) &&
	    ! ((clink[j].pre > nrow && nzj != 1))) {
	  C_EKK_ADD_LINK(hpivco, nzj, clink, j);
	}
      }
    } else {
      for (k = kipis; k <= kipie; ++k) {
	int j = hcoli[k];
	int nzj = hincol[j];
	if (! (nzj <= 0)) {
	  C_EKK_ADD_LINK(hpivco, nzj, clink, j);
	}
      }
    }
    fact->nuspike += hinrow[ipivot];

    /* Go to dense coding if appropriate */
#ifndef	C_EKKCMFY
    if (ifdens != 0) {
      int ndense = nrow - fact->npivots;
      if (! (xnewro + ndense * ndense >= lstart)) {

	/* set up sort order in MACTION */
	c_ekkizero( nrow, reinterpret_cast<int *> (maction+1));
	iput = 0;
	for (i = 1; i <= nrow; ++i) {
	  if (clink[i].pre >= 0) {
	    ++iput;
	    maction[i] = static_cast<short int>(iput);
	  }
	}
	/* and get number spare needed */
	nspare = 0;
	for (i = 1; i <= nrow; ++i) {
	  if (rlink[i].pre >= 0) {
	    nspare = nspare + ndense - hinrow[i];
	  }
	}
	if (iput != nrow - fact->npivots) {
	  /* must be singular */
	  c_ekkizero( nrow, reinterpret_cast<int *> (maction+1));
	} else {
	  /* pack down then back up */
	  int iput = c_ekkrwcs(fact,dluval, hcoli, mrstrt, hinrow, mwork, nfirst);
	  kmxeta += xnewro - iput ;
	  xnewro = iput - 1;
	  ++ncompactions;

	  --ncompactions;
	  if (xnewro + nspare + ndense * ndense >= lstart) {
	    c_ekkizero( nrow, reinterpret_cast<int *> (maction+1));
	  }
	  else {
	    xnewro += nspare;
	    c_ekkrwct(fact,dluval, hcoli, mrstrt, hinrow, mwork,
		    rlink, maction, dvalpv,
		    nlast,  xnewro);
	    kmxeta += xnewro ;
	    if (nnentu + nnentl > nrow * 5 &&
		(ndense*ndense)>(nnentu+nnentl)>>2 &&
		!if_sparse_update) {
	      fact->ndenuc = ndense;
	    }
	    irtcod = c_ekkcmfd(fact,
			     (reinterpret_cast<int*>(dvalpv)+1),
			     rlink, clink,
			     (reinterpret_cast<int*>(maction+1))+1,
			     nnetas,
			     &nnentl, &nnentu,
			     nsingp);
	    /* irtcod == 0 || irtcod == 10 */
	    /* 10 == found 0.0 pivot */
	    goto L1050;
	  }
	}
      } else {
	/* say not enough room */
	/*printf("no room %d\n",ndense);*/
	if (1) {
	  /* return and increase size of etas if possible */
	  if (!noRoomForDense) {
	    int etasize =CoinMax(4*fact->nnentu+(nnetas-fact->nnentl)+1000,fact->eta_size);
	    noRoomForDense=ndense;
	    fact->eta_size=CoinMin(static_cast<int>(1.2*fact->eta_size),etasize);
	    if (fact->maxNNetas>0&&fact->eta_size>
		fact->maxNNetas) {
	      fact->eta_size=fact->maxNNetas;
	    }
	  }
	}
      }
    }
#endif	/* C_EKKCMFY */
  }

 L1050:
  {
    int iput = c_ekkrwcs(fact,dluval, hcoli, mrstrt, hinrow, mwork, nfirst);
    kmxeta += xnewro - iput;
    xnewro = iput - 1;
    ++ncompactions;
  }

  nnentu = xnewro;
  /* save order of row copy for c_ekkshfv */
  mwork[nrow+1].pre = nfirst;
  mwork[nrow+1].suc = nlast;

  fact->nnentl = nnentl;
  fact->nnentu = nnentu;
  fact->kmxeta = kmxeta;
  *xnewrop = xnewro;
  *ncompactionsp = ncompactions;

  return (irtcod);
} /* c_ekkcmfc */
#endif

