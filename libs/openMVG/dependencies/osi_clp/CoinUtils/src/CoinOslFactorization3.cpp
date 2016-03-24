/* $Id: CoinOslFactorization3.cpp 1585 2013-04-06 20:42:02Z stefan $ */
/*
  Copyright (C) 1987, 2009, International Business Machines
  Corporation and others.  All Rights Reserved.

  This code is licensed under the terms of the Eclipse Public License (EPL).
*/
#include "CoinOslFactorization.hpp"
#include "CoinOslC.h"
#include "CoinFinite.hpp"
#define GO_DENSE 70
#define GO_DENSE_RATIO 1.8
int c_ekkclco(const EKKfactinfo *fact,int *hcoli,
	    int *mrstrt, int *hinrow, int xnewro);

void c_ekkclcp(const int *hcol, const double *dels, const int * mrstrt,
	     int *hrow, double *dels2, int *mcstrt,
	     int *hincol, int itype, int nnrow, int nncol,
	     int ninbas);

int c_ekkcmfc(EKKfactinfo *fact,
	    EKKHlink *rlink, EKKHlink *clink,
	    EKKHlink *mwork, void *maction_void,
	    int nnetas,
	    int *nsingp, int *xrejctp,
	    int *xnewrop, int xnewco,
	    int *ncompactionsp);

int c_ekkcmfy(EKKfactinfo *fact,
	    EKKHlink *rlink, EKKHlink *clink,
	    EKKHlink *mwork, void *maction_void,
	    int nnetas,
	    int *nsingp, int *xrejctp,
	    int *xnewrop, int xnewco,
	    int *ncompactionsp);

int c_ekkcmfd(EKKfactinfo *fact,
	    int *mcol,
	    EKKHlink *rlink, EKKHlink *clink,
	    int *maction,
	    int nnetas,
	    int *nnentlp, int *nnentup,
	    int *nsingp);
int c_ekkford(const EKKfactinfo *fact,const int *hinrow, const int *hincol,
	    int *hpivro, int *hpivco,
	    EKKHlink *rlink, EKKHlink *clink);
void c_ekkrowq(int *hrow, int *hcol, double *dels,
	     int *mrstrt,
	     const int *hinrow, int nnrow, int ninbas);
int c_ekkrwco(const EKKfactinfo *fact,double *dluval, int *hcoli, int *
	    mrstrt, int *hinrow, int xnewro);

int c_ekkrwcs(const EKKfactinfo *fact,double *dluval, int *hcoli, int *mrstrt,
	    const int *hinrow, const EKKHlink *mwork,
	    int nfirst);

void c_ekkrwct(const EKKfactinfo *fact,double *dluval, int *hcoli, int *mrstrt,
	     const int *hinrow, const EKKHlink *mwork,
	     const EKKHlink *rlink,
	     const short *msort, double *dsort,
	     int nlast, int xnewro);

int c_ekkshff(EKKfactinfo *fact,
	    EKKHlink *clink, EKKHlink *rlink,
	    int xnewro);

void c_ekkshfv(EKKfactinfo *fact, EKKHlink *rlink, EKKHlink *clink,
	     int xnewro);
int c_ekktria(EKKfactinfo *fact,
	      EKKHlink * rlink,
	      EKKHlink * clink,
	    int *nsingp,
	    int *xnewcop, int *xnewrop,
	    int *nlrowtp,
	    const int ninbas);
#if 0
static void c_ekkafpv(int *hentry, int *hcoli,
	     double *dluval, int *mrstrt,
	     int *hinrow, int nentry)
{
  int j;
  int nel, krs;
  int koff;
  int irow;
  int ientry;
  int * index;

  for (ientry = 0; ientry < nentry; ++ientry) {
#ifdef INTEL
    int * els_long,maxaij_long;
#endif
    double * els;
    irow = UNSHIFT_INDEX(hentry[ientry]);
    nel = hinrow[irow];
    krs = mrstrt[irow];
    index=&hcoli[krs];
    els=&dluval[krs];
#ifdef INTEL
    els_long=reinterpret_cast<int *> (els);
    maxaij_long=0;
#else
    double maxaij = 0.f;
#endif
    koff = 0;
    j=0;
    if ((nel&1)!=0) {
#ifdef INTEL
      maxaij_long = els_long[1] & 0x7fffffff;
#else
      maxaij=fabs(els[0]);
#endif
      j=1;
    }

    while (j<nel) {
#ifdef INTEL
      UNROLL_LOOP_BODY2({
	  int d_long = els_long[1+(j<<1)] & 0x7fffffff;
	  if (maxaij_long < d_long) {
	    maxaij_long = d_long;
	    koff=j;
	  }
	  j++;
	});
#else
      UNROLL_LOOP_BODY2({
	  double d = fabs(els[j]);
	  if (maxaij < d) {
	    maxaij = d;
	    koff=j;
	  }
	  j++;
	});
#endif
    }

    SWAP(int, index[koff], index[0]);
    SWAP(double, els[koff], els[0]);
  }
} /* c_ekkafpv */
#endif

/*     Uwe H. Suhl, March 1987 */
/*     This routine processes col singletons during the LU-factorization. */
/*     Return codes (checked version 1.11): */
/*	0: ok */
/*      6: pivot element too small */
/*     -43: system error at label 420 (ipivot not found) */
/*
 * This routine processes singleton columns during factorization of the
 * nucleus.  It is very similar to the first part of c_ekktria,
 * but is more complex, because we now have to maintain the length
 * lists.
 * The differences are:
 * (1) here we use the length list for length 1 rather than a queue.
 * This routine is only called if it is known that there is a singleton
 * column.
 *
 * (2) here we maintain hrowi by moving the last entry into the pivot
 * column entry; that means we don't have to search for the pivot row
 * entry like we do in c_ekktria.
 *
 * (3) here the hlink data structure is in use for the length lists,
 * so we maintain it as we shorten rows and removing columns altogether.
 *
 */
int c_ekkcsin(EKKfactinfo *fact,
	      EKKHlink *rlink, EKKHlink *clink,

	      int *nsingp)
{
#if 1
  int *hcoli	= fact->xecadr;
  double *dluval	= fact->xeeadr;
  //double *dvalpv = fact->kw3adr;
  int *mrstrt	= fact->xrsadr;
  int *hrowi	= fact->xeradr;
  int *mcstrt	= fact->xcsadr;
  int *hinrow	= fact->xrnadr;
  int *hincol	= fact->xcnadr;
  int *hpivro	= fact->krpadr;
  int *hpivco	= fact->kcpadr;
#endif
  const int nrow	= fact->nrow;
  const double drtpiv	= fact->drtpiv;


  int j, k, kc, kce, kcs, nzj;
  double pivot;
  int kipis, kipie;
  int jpivot;
#ifndef NDEBUG
  int kpivot=-1;
#else
  int kpivot=-1;
#endif

  bool small_pivot = false;


  /* next singleton column.
   * Note that when the pivot column itself was removed from the
   * list, the column in the list after it (if any) moves to the
   * head of the list.
   * Also, if any column from the pivot row was reduced to length 1,
   * then it will have been added to the list and now be in front.
   */
  for (jpivot = hpivco[1]; jpivot > 0; jpivot = hpivco[1]) {
    const int ipivot = hrowi[mcstrt[jpivot]]; /* (2) */
    assert(ipivot);
    /* The pivot row is being eliminated (3) */
    C_EKK_REMOVE_LINK(hpivro, hinrow, rlink, ipivot);

    /* Loop over nonzeros in pivot row: */
    kipis = mrstrt[ipivot];
    kipie = kipis + hinrow[ipivot] - 1;
    for (k = kipis; k <= kipie; ++k) {
      j = hcoli[k];

      /*
       * We're eliminating column jpivot,
       * so we're eliminating the row it occurs in,
       * so every column in this row is becoming one shorter.
       *
       * I don't know why we don't do the same for rejected columns.
       *
       * if xrejct is false, then no column has ever been rejected
       * and this test wouldn't have to be made.
       * However, that means this whole loop would have to be copied.
       */
      if (! (clink[j].pre > nrow)) {
	C_EKK_REMOVE_LINK(hpivco, hincol, clink, j); /* (3) */
      }
      --hincol[j];

      kcs = mcstrt[j];
      kce = kcs + hincol[j];
      for (kc = kcs; kc <= kce; ++kc) {
	if (ipivot == hrowi[kc]) {
	  break;
	}
      }
      /* ASSERT !(kc>kce) */

      /* (2) */
      hrowi[kc] = hrowi[kce];
      hrowi[kce] = 0;

      if (j == jpivot) {
	/* remember the slot corresponding to the pivot column */
	kpivot = k;
      }
      else {
	/*
	 * We just reduced the length of the column.
	 * If we haven't eliminated all of its elements completely,
	 * then we have to put it back in its new length list.
	 *
	 * If the column was rejected, we only put it back in a length
	 * list when it has been reduced to a singleton column,
	 * because it would just be rejected again.
	 */
	nzj = hincol[j];
	if (! (nzj <= 0) &&
	    ! (clink[j].pre > nrow && nzj != 1)) {
	  C_EKK_ADD_LINK(hpivco, nzj, clink, j); /* (3) */
	}
      }
    }
    assert (kpivot>0);

    /* store pivot sequence number */
    ++fact->npivots;
    rlink[ipivot].pre = -fact->npivots;
    clink[jpivot].pre = -fact->npivots;

    /* compute how much room we'll need later */
    fact->nuspike += hinrow[ipivot];

    /* check the pivot */
    pivot = dluval[kpivot];
    if (fabs(pivot) < drtpiv) {
      /* pivot element too small */
      small_pivot = true;
      rlink[ipivot].pre = -nrow - 1;
      clink[jpivot].pre = -nrow - 1;
      ++(*nsingp);
    }

    /* swap the pivoted column entry with the first entry in the row */
    dluval[kpivot] = dluval[kipis];
    dluval[kipis] = pivot;
    hcoli[kpivot] = hcoli[kipis];
    hcoli[kipis] = jpivot;
  }

  return (small_pivot);
} /* c_ekkcsin */


/*     Uwe H. Suhl, March 1987 */
/*     This routine processes row singletons during the computation of */
/*     an LU-decomposition for the nucleus. */
/*     Return codes (checked version 1.16): */
/*      -5: not enough space in row file */
/*      -6: not enough space in column file */
/*      7: pivot element too small */
/*     -52: system error at label 220 (ipivot not found) */
/*     -53: system error at label 400 (jpivot not found) */
int c_ekkrsin(EKKfactinfo *fact,
	    EKKHlink *rlink, EKKHlink *clink,
	    EKKHlink *mwork, int nfirst,
	    int *nsingp,
	    int *xnewcop, int *xnewrop,
	    int *nnentup,
	    int *kmxetap, int *ncompactionsp,
	    int *nnentlp)

{
#if 1
  int *hcoli	= fact->xecadr;
  double *dluval	= fact->xeeadr;
  //double *dvalpv = fact->kw3adr;
  int *mrstrt	= fact->xrsadr;
  int *hrowi	= fact->xeradr;
  int *mcstrt	= fact->xcsadr;
  int *hinrow	= fact->xrnadr;
  int *hincol	= fact->xcnadr;
  int *hpivro	= fact->krpadr;
  int *hpivco	= fact->kcpadr;
#endif
  const int nrow	= fact->nrow;
  const double drtpiv	= fact->drtpiv;

  int xnewro	= *xnewrop;
  int xnewco	= *xnewcop;
  int kmxeta	= *kmxetap;
  int nnentu	= *nnentup;
  int ncompactions	= *ncompactionsp;
  int nnentl	= *nnentlp;

  int i, j, k, kc, kr, npr, nzi;
  double pivot;
  int kjpis, kjpie, knprs, knpre;
  double elemnt, maxaij;
  int ipivot, epivco, lstart;
#ifndef NDEBUG
  int kpivot=-1;
#else
  int kpivot=-1;
#endif
  int irtcod = 0;
  const int nnetas	= fact->nnetas;

  lstart = nnetas - nnentl + 1;


  for (ipivot = hpivro[1]; ipivot > 0; ipivot = hpivro[1]) {
    const int jpivot = hcoli[mrstrt[ipivot]];

    kjpis = mcstrt[jpivot];
    kjpie = kjpis + hincol[jpivot] ;
    for (k = kjpis; k < kjpie; ++k) {
      i = hrowi[k];

      /*
       * We're eliminating row ipivot,
       * so we're eliminating the column it occurs in,
       * so every row in this column is becoming one shorter.
       *
       * No exception is made for rejected rows.
       */
      C_EKK_REMOVE_LINK(hpivro, hinrow, rlink, i);
    }

    /* The pivot column is being eliminated */
    /* I don't know why there is an exception for rejected columns */
    if (! (clink[jpivot].pre > nrow)) {
      C_EKK_REMOVE_LINK(hpivco, hincol, clink, jpivot);
    }

    epivco = hincol[jpivot] - 1;
    kjpie = kjpis + epivco;
    for (kc = kjpis; kc <= kjpie; ++kc) {
      if (ipivot == hrowi[kc]) {
	break;
      }
    }
    /* ASSERT !(kc>kjpie) */

    /* move the last column entry into this deleted one to keep */
    /* the entries compact */
    hrowi[kc] = hrowi[kjpie];

    hrowi[kjpie] = 0;

    /* store pivot sequence number */
    ++fact->npivots;
    rlink[ipivot].pre = -fact->npivots;
    clink[jpivot].pre = -fact->npivots;

    /* Check if row or column files have to be compressed */
    if (! (xnewro + epivco < lstart)) {
      if (! (nnentu + epivco < lstart)) {
	return (-5);
      }
      {
	int iput = c_ekkrwcs(fact,dluval, hcoli, mrstrt, hinrow, mwork, nfirst);
	kmxeta += xnewro - iput ;
	xnewro = iput - 1;
	++ncompactions;
      }
    }
    if (! (xnewco + epivco < lstart)) {
      if (! (nnentu + epivco < lstart)) {
	return (-5);
      }
      xnewco = c_ekkclco(fact,hrowi, mcstrt, hincol, xnewco);
      ++ncompactions;
    }

    /* This column has no more entries in it */
    hincol[jpivot] = 0;

    /* Perform numerical part of elimination. */
    pivot = dluval[mrstrt[ipivot]];
    if (fabs(pivot) < drtpiv) {
      irtcod = 7;
      rlink[ipivot].pre = -nrow - 1;
      clink[jpivot].pre = -nrow - 1;
      ++(*nsingp);
    }

    /* If epivco is 0, then we can treat this like a singleton column (?)*/
    if (! (epivco <= 0)) {
      ++fact->xnetal;
      mcstrt[fact->xnetal] = lstart - 1;
      hpivco[fact->xnetal] = ipivot;

      /* Loop over nonzeros in pivot column. */
      kjpis = mcstrt[jpivot];
      kjpie = kjpis + epivco ;
      nnentl+=epivco;
      nnentu-=epivco;
      for (kc = kjpis; kc < kjpie; ++kc) {
	npr = hrowi[kc];
	/* zero out the row entries as we go along */
	hrowi[kc] = 0;

	/* each row in the column is getting shorter */
	--hinrow[npr];

	/* find the entry in this row for the pivot column */
	knprs = mrstrt[npr];
	knpre = knprs + hinrow[npr];
	for (kr = knprs; kr <= knpre; ++kr) {
	  if (jpivot == hcoli[kr])
	    break;
	}
	/* ASSERT !(kr>knpre) */

	elemnt = dluval[kr];
	/* move the last pivot column entry into this one */
	/* to keep entries compact */
	dluval[kr] = dluval[knpre];
	hcoli[kr] = hcoli[knpre];

	/*
	 * c_ekkmltf put the largest entries in front, and
	 * we want to maintain that property.
	 * There is only a problem if we just pivoted out the first
	 * entry, and there is more than one entry in the list.
	 */
	if (! (kr != knprs || hinrow[npr] <= 1)) {
	  maxaij = 0.f;
	  for (k = knprs; k <= knpre; ++k) {
	    if (! (fabs(dluval[k]) <= maxaij)) {
	      maxaij = fabs(dluval[k]);
	      kpivot = k;
	    }
	  }
	  assert (kpivot>0);
	  maxaij = dluval[kpivot];
	  dluval[kpivot] = dluval[knprs];
	  dluval[knprs] = maxaij;

	  j = hcoli[kpivot];
	  hcoli[kpivot] = hcoli[knprs];
	  hcoli[knprs] = j;
	}

	/* store elementary row transformation */
	--lstart;
	dluval[lstart] = -elemnt / pivot;
	hrowi[lstart] = SHIFT_INDEX(npr);

	/* Only add the row back in a length list if it isn't empty */
	nzi = hinrow[npr];
	if (! (nzi <= 0)) {
	  C_EKK_ADD_LINK(hpivro, nzi, rlink, npr);
	}
      }
      ++fact->nuspike;
    }
  }

  *xnewrop = xnewro;
  *xnewcop = xnewco;
  *kmxetap = kmxeta;
  *nnentup = nnentu;
  *ncompactionsp = ncompactions;
  *nnentlp = nnentl;

  return (irtcod);
} /* c_ekkrsin */


int c_ekkfpvt(const EKKfactinfo *fact,
	    EKKHlink *rlink, EKKHlink *clink,
	    int *nsingp, int *xrejctp,
	    int *xipivtp, int *xjpivtp)
{
  double zpivlu = fact->zpivlu;
#if 1
  int *hcoli	= fact->xecadr;
  double *dluval	= fact->xeeadr;
  //double *dvalpv = fact->kw3adr;
  int *mrstrt	= fact->xrsadr;
  int *hrowi	= fact->xeradr;
  int *mcstrt	= fact->xcsadr;
  int *hinrow	= fact->xrnadr;
  int *hincol	= fact->xcnadr;
  int *hpivro	= fact->krpadr;
  int *hpivco	= fact->kcpadr;
#endif
  int i, j, k, ke, kk, ks, nz, nz1, kce, kcs, kre, krs;
  double minsze;
  int marcst, mincst, mincnt, trials, nentri;
  int jpivot=-1;
  bool rjectd;
  int ipivot;
  const int nrow	= fact->nrow;
  int irtcod = 0;

  /* this used to be initialized in c_ekklfct */
  const int xtrial = 1;

  trials = 0;
  ipivot = 0;
  mincst = COIN_INT_MAX;
  mincnt = COIN_INT_MAX;
  for (nz = 2; nz <= nrow; ++nz) {
    nz1 = nz - 1;
    if (mincnt <= nz) {
      goto L900;
    }

    /* Search rows for a pivot */
    for (i = hpivro[nz]; ! (i <= 0); i = rlink[i].suc) {

      ks = mrstrt[i];
      ke = ks + nz - 1;
      /* Determine magnitude of minimal acceptable element */
      minsze = fabs(dluval[ks]) * zpivlu;
      for (k = ks; k <= ke; ++k) {
	/* Consider a column only if it passes the stability test */
	if (! (fabs(dluval[k]) < minsze)) {
	  j = hcoli[k];
	  marcst = nz1 * hincol[j];
	  if (! (marcst >= mincst)) {
	    mincst = marcst;
	    mincnt = hincol[j];
	    ipivot = i;
	    jpivot = j;
	    if (mincnt <= nz + 1) {
	      goto L900;
	    }
	  }
	}
      }
      ++trials;

      if (trials >= xtrial) {
	goto L900;
      }
    }

    /* Search columns for a pivot */
    j = hpivco[nz];
    while (! (j <= 0)) {
      /* XSEARD = XSEARD + 1 */
      rjectd = false;
      kcs = mcstrt[j];
      kce = kcs + nz - 1;
      for (k = kcs; k <= kce; ++k) {
	i = hrowi[k];
	nentri = hinrow[i];
	marcst = nz1 * nentri;
	if (! (marcst >= mincst)) {
	  /* Determine magnitude of minimal acceptable element */
	  minsze = fabs(dluval[mrstrt[i]]) * zpivlu;
	  krs = mrstrt[i];
	  kre = krs + nentri - 1;
	  for (kk = krs; kk <= kre; ++kk) {
	    if (hcoli[kk] == j)
	      break;
	  }
	  /* ASSERT (kk <= kre) */

	  /* perform stability test */
	  if (! (fabs(dluval[kk]) < minsze)) {
	    mincst = marcst;
	    mincnt = nentri;
	    ipivot = i;
	    jpivot = j;
	    rjectd = false;
	    if (mincnt <= nz) {
	      goto L900;
	    }
	  }
	  else {
	    if (ipivot == 0) {
	      rjectd = true;
	    }
	  }
	}
      }
      ++trials;
      if (trials >= xtrial && ipivot > 0) {
	goto L900;
      }
      if (rjectd) {
	int jsuc = clink[j].suc;
	++(*xrejctp);
	C_EKK_REMOVE_LINK(hpivco, hincol, clink, j);
	clink[j].pre = nrow + 1;
	j = jsuc;
      }
      else {
	j = clink[j].suc;
      }
    }
  }

  /* FLAG REJECTED ROWS (should this be columns ?) */
  for (j = 1; j <= nrow; ++j) {
    if (hinrow[j] == 0) {
      rlink[j].pre = -nrow - 1;
      ++(*nsingp);
    }
  }
  irtcod = 10;

 L900:
  *xipivtp = ipivot;
  *xjpivtp = jpivot;
  return (irtcod);
} /* c_ekkfpvt */
void c_ekkprpv(EKKfactinfo *fact,
	     EKKHlink *rlink, EKKHlink *clink,

	     int xrejct,
	     int ipivot, int jpivot)
{
#if 1
  int *hcoli	= fact->xecadr;
  double *dluval	= fact->xeeadr;
  //double *dvalpv = fact->kw3adr;
  int *mrstrt	= fact->xrsadr;
  int *hrowi	= fact->xeradr;
  int *mcstrt	= fact->xcsadr;
  int *hinrow	= fact->xrnadr;
  int *hincol	= fact->xcnadr;
  int *hpivro	= fact->krpadr;
  int *hpivco	= fact->kcpadr;
#endif
  int i, k;
  int kc;
  double pivot;
  int kipis = mrstrt[ipivot];
  int kipie = kipis + hinrow[ipivot] - 1;

#ifndef NDEBUG
  int kpivot=-1;
#else
  int kpivot=-1;
#endif
  const int nrow	= fact->nrow;

  /*     Update data structures */
  {
    int kjpis = mcstrt[jpivot];
    int kjpie = kjpis + hincol[jpivot] ;
    for (k = kjpis; k < kjpie; ++k) {
      i = hrowi[k];
      C_EKK_REMOVE_LINK(hpivro, hinrow, rlink, i);
    }
  }

  for (k = kipis; k <= kipie; ++k) {
    int j = hcoli[k];

    if ((xrejct == 0) ||
	! (clink[j].pre > nrow)) {
      C_EKK_REMOVE_LINK(hpivco, hincol, clink, j);
    }

    --hincol[j];
    int kcs = mcstrt[j];
    int kce = kcs + hincol[j];

    for (kc = kcs; kc < kce ; kc ++) {
      if (hrowi[kc] == ipivot)
	break;
    }
    assert (kc<kce||hrowi[kce]==ipivot);
    hrowi[kc] = hrowi[kce];
    hrowi[kce] = 0;
    if (j == jpivot) {
      kpivot = k;
    }
  }
  assert (kpivot>0);

  /* Store the pivot sequence number */
  ++fact->npivots;
  rlink[ipivot].pre = -fact->npivots;
  clink[jpivot].pre = -fact->npivots;

  pivot = dluval[kpivot];
  dluval[kpivot] = dluval[kipis];
  dluval[kipis] = pivot;
  hcoli[kpivot] = hcoli[kipis];
  hcoli[kipis] = jpivot;

} /* c_ekkprpv */

/*
 * c_ekkclco is almost exactly like c_ekkrwco.
 */
int c_ekkclco(const EKKfactinfo *fact,int *hcoli, int *mrstrt, int *hinrow, int xnewro)
{
#if 0
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
  int i, k, nz, kold;
  int kstart;
  const int nrow	= fact->nrow;

  for (i = 1; i <= nrow; ++i) {
    nz = hinrow[i];
    if (0 < nz) {
      /* save the last column entry of row i in hinrow */
      /* and replace that entry with -i */
      k = mrstrt[i] + nz - 1;
      hinrow[i] = hcoli[k];
      hcoli[k] = -i;
    }
  }

  kstart = 0;
  kold = 0;
  for (k = 1; k <= xnewro; ++k) {
    if (hcoli[k] != 0) {
      ++kstart;

      /* if this is the last entry for the row... */
      if (hcoli[k] < 0) {
	/* restore the entry */
	i = -hcoli[k];
	hcoli[k] = hinrow[i];

	/* update mrstart and hinrow */
	mrstrt[i] = kold + 1;
	hinrow[i] = kstart - kold;
	kold = kstart;
      }

      hcoli[kstart] = hcoli[k];
    }
  }

  /* INSERTED INCASE CALLED FROM YTRIAN JJHF */
  mrstrt[nrow + 1] = kstart + 1;

  return (kstart);
} /* c_ekkclco */



#undef MACTION_T
#define COIN_OSL_CMFC
#define MACTION_T short int
int c_ekkcmfc(EKKfactinfo *fact,
	    EKKHlink *rlink, EKKHlink *clink,
	    EKKHlink *mwork, void *maction_void,
	    int nnetas,
	    int *nsingp, int *xrejctp,
	    int *xnewrop, int xnewco,
	    int *ncompactionsp)

#include "CoinOslC.h"
#undef COIN_OSL_CMFC
#undef MACTION_T
  static int c_ekkidmx(int n, const double *dx)
{
  int ret_val;
  int i;
  double dmax;
  --dx;

  /* Function Body */

  if (n < 1) {
    return (0);
  }

  if (n == 1) {
    return (1);
  }

  ret_val = 1;
  dmax = fabs(dx[1]);
  for (i = 2; i <= n; ++i) {
    if (fabs(dx[i]) > dmax) {
      ret_val = i;
      dmax = fabs(dx[i]);
    }
  }

  return ret_val;
} /* c_ekkidmx */
/*     Return codes in IRTCOD/IRTCOD are */
/*     4: numerical problems */
/*     5: not enough space in row file */
/*     6: not enough space in column file */
int c_ekkcmfd(EKKfactinfo *fact,
	    int *mcol,
	    EKKHlink *rlink, EKKHlink *clink,
	    int *maction,
	    int nnetas,
	    int *nnentlp, int *nnentup,
	    int *nsingp)
{
  int *hcoli	= fact->xecadr;
  double *dluval	= fact->xeeadr;
  int *mrstrt	= fact->xrsadr;
  int *hrowi	= fact->xeradr;
  int *mcstrt	= fact->xcsadr;
  int *hinrow	= fact->xrnadr;
  int *hincol	= fact->xcnadr;
  int *hpivro	= fact->krpadr;
  int *hpivco	= fact->kcpadr;
  int nnentl	= *nnentlp;
  int nnentu	= *nnentup;
  int storeZero = fact->ndenuc;

  int mkrs[8];
  double dpivyy[8];

  /* Local variables */
  int i, j;
  double d0, dx;
  int nz, ndo, krs;
  int kend, jcol;
  int irow, iput, jrow, krxs;
  int mjcol[8];
  double pivot;
  int count;
  int ilast, isort;
  double dpivx, dsave;
  double dpivxx[8];
  double multip;
  int lstart, ndense, krlast, kcount, idense, ipivot,
    jdense, kchunk, jpivot;

  const int nrow	= fact->nrow;

  int irtcod = 0;

  lstart = nnetas - nnentl + 1;

  /* put list of columns in last HROWI */
  /* fix row order once for all */
  ndense = nrow - fact->npivots;
  iput = ndense + 1;
  for (i = 1; i <= nrow; ++i) {
    if (hpivro[i] > 0) {
      irow = hpivro[i];
      for (j = 1; j <= nrow; ++j) {
	--iput;
	maction[iput] = irow;
	irow = rlink[irow].suc;
	if (irow == 0) {
	  break;
	}
      }
    }
  }
  if (iput != 1) {
    ++(*nsingp);
  }
  else {
    /*     Use HCOLI just for last row */
    ilast = maction[1];
    krlast = mrstrt[ilast];
    /*     put list of columns in last HCOLI */
    iput = 0;
    for (i = 1; i <= nrow; ++i) {
      if (clink[i].pre >= 0) {
	hcoli[krlast + iput] = i;
	++iput;
      }
    }
    if (iput != ndense) {
      ++(*nsingp);
    }
    else {
      ndo = ndense / 8;
      /*     do most */
      for (kcount = 1; kcount <= ndo; ++kcount) {
        idense = ndense;
        isort = 8;
        for (count = ndense; count >= ndense - 7; --count) {
	  ipivot = maction[count];
	  krs = mrstrt[ipivot];
	  --isort;
	  mkrs[isort] = krs;
        }
        isort = 8;
        for (count = ndense; count >= ndense - 7; --count) {
	  /* Find a pivot element */
	  --isort;
	  ipivot = maction[count];
	  krs = mkrs[isort];
	  jcol = c_ekkidmx(idense, &dluval[krs]) - 1;
	  pivot = dluval[krs + jcol];
	  --idense;
	  mcol[count] = jcol;
	  mjcol[isort] = mcol[count];
	  dluval[krs + jcol] = dluval[krs + idense];
	  if (fabs(pivot) < fact->zeroTolerance) {
	    pivot = 0.;
	    dpivx = 0.;
	  } else {
	    dpivx = 1. / pivot;
	  }
	  dluval[krs + idense] = pivot;
	  dpivxx[isort] = dpivx;
	  for (j = isort - 1; j >= 0; --j) {
	    krxs = mkrs[j];
	    multip = -dluval[krxs + jcol] * dpivx;
	    dluval[krxs + jcol] = dluval[krxs + idense];
	    /*           for moment skip if zero */
	    if (fabs(multip) > fact->zeroTolerance) {
	      for (i = 0; i < idense; ++i) {
		dluval[krxs + i] += multip * dluval[krs + i];
	      }
	    } else {
	      multip = 0.;
	    }
	    dluval[krxs + idense] = multip;
	  }
        }
	/*       sort all U in rows already done */
        for (i = 7; i >= 0; --i) {
	  /* ****     this is important bit */
	  krs = mkrs[i];
	  for (j = i - 1; j >= 0; --j) {
	    jcol = mjcol[j];
	    dsave = dluval[krs + jcol];
	    dluval[krs + jcol] = dluval[krs + idense + j];
	    dluval[krs + idense + j] = dsave;
	  }
        }
	/*       leave IDENSE as it is */
        if (ndense <= 400) {
	  for (jrow = ndense - 8; jrow >= 1; --jrow) {
	    irow = maction[jrow];
	    krxs = mrstrt[irow];
	    for (j = 7; j >= 0; --j) {
	      jcol = mjcol[j];
	      dsave = dluval[krxs + jcol];
	      dluval[krxs + jcol] = dluval[krxs + idense + j];
	      dluval[krxs + idense + j] = dsave;
	    }
	    for (j = 7; j >= 0; --j) {
	      krs = mkrs[j];
	      jdense = idense + j;
	      dpivx = dpivxx[j];
	      multip = -dluval[krxs + jdense] * dpivx;
	      if (fabs(multip) <= fact->zeroTolerance) {
		multip = 0.;
	      }
	      dpivyy[j] = multip;
	      dluval[krxs + jdense] = multip;
	      for (i = idense; i < jdense; ++i) {
		dluval[krxs + i] += multip * dluval[krs + i];
	      }
	    }
	    for (i = 0; i < idense; ++i) {
	      dx = dluval[krxs + i];
	      d0 = dpivyy[0] * dluval[mkrs[0] + i];
	      dx += dpivyy[1] * dluval[mkrs[1] + i];
	      d0 += dpivyy[2] * dluval[mkrs[2] + i];
	      dx += dpivyy[3] * dluval[mkrs[3] + i];
	      d0 += dpivyy[4] * dluval[mkrs[4] + i];
	      dx += dpivyy[5] * dluval[mkrs[5] + i];
	      d0 += dpivyy[6] * dluval[mkrs[6] + i];
	      dx += dpivyy[7] * dluval[mkrs[7] + i];
	      dluval[krxs + i] = d0 + dx;
	    }
	  }
        } else {
	  for (jrow = ndense - 8; jrow >= 1; --jrow) {
	    irow = maction[jrow];
	    krxs = mrstrt[irow];
	    for (j = 7; j >= 0; --j) {
	      jcol = mjcol[j];
	      dsave = dluval[krxs + jcol];
	      dluval[krxs + jcol] = dluval[krxs + idense + j];
	      dluval[krxs + idense + j] = dsave;
	    }
	    for (j = 7; j >= 0; --j) {
	      krs = mkrs[j];
	      jdense = idense + j;
	      dpivx = dpivxx[j];
	      multip = -dluval[krxs + jdense] * dpivx;
	      if (fabs(multip) <= fact->zeroTolerance) {
		multip = 0.;
	      }
	      dluval[krxs + jdense] = multip;
	      for (i = idense; i < jdense; ++i) {
		dluval[krxs + i] += multip * dluval[krs + i];
	      }
	    }
	  }
	  for (kchunk = 0; kchunk < idense; kchunk += 400) {
	    kend = CoinMin(idense - 1, kchunk + 399);
	    for (jrow = ndense - 8; jrow >= 1; --jrow) {
	      irow = maction[jrow];
	      krxs = mrstrt[irow];
	      for (j = 7; j >= 0; --j) {
		dpivyy[j] = dluval[krxs + idense + j];
	      }
	      for (i = kchunk; i <= kend; ++i) {
		dx = dluval[krxs + i];
		d0 = dpivyy[0] * dluval[mkrs[0] + i];
		dx += dpivyy[1] * dluval[mkrs[1] + i];
		d0 += dpivyy[2] * dluval[mkrs[2] + i];
		dx += dpivyy[3] * dluval[mkrs[3] + i];
		d0 += dpivyy[4] * dluval[mkrs[4] + i];
		dx += dpivyy[5] * dluval[mkrs[5] + i];
		d0 += dpivyy[6] * dluval[mkrs[6] + i];
		dx += dpivyy[7] * dluval[mkrs[7] + i];
		dluval[krxs + i] = d0 + dx;
	      }
	    }
	  }
        }
	/*       resort all U in rows already done */
        for (i = 7; i >= 0; --i) {
	  krs = mkrs[i];
	  for (j = 0; j < i; ++j) {
	    jcol = mjcol[j];
	    dsave = dluval[krs + jcol];
	    dluval[krs + jcol] = dluval[krs + idense + j];
	    dluval[krs + idense + j] = dsave;
	  }
        }
        ndense += -8;
      }
      idense = ndense;
      /*     do remainder */
      for (count = ndense; count >= 1; --count) {
	/*        Find a pivot element */
        ipivot = maction[count];
        krs = mrstrt[ipivot];
        jcol = c_ekkidmx(idense, &dluval[krs]) - 1;
        pivot = dluval[krs + jcol];
        --idense;
        mcol[count] = jcol;
        dluval[krs + jcol] = dluval[krs + idense];
        if (fabs(pivot) < fact->zeroTolerance) {
	  dluval[krs + idense] = 0.;
        } else {
	  dpivx = 1. / pivot;
	  dluval[krs + idense] = pivot;
	  for (jrow = idense; jrow >= 1; --jrow) {
	    irow = maction[jrow];
	    krxs = mrstrt[irow];
	    multip = -dluval[krxs + jcol] * dpivx;
	    dluval[krxs + jcol] = dluval[krxs + idense];
	    /*           for moment skip if zero */
	    if (fabs(multip) > fact->zeroTolerance) {
	      dluval[krxs + idense] = multip;
	      for (i = 0; i < idense; ++i) {
		dluval[krxs + i] += multip * dluval[krs + i];
	      }
	    } else {
	      dluval[krxs + idense] = 0.;
	    }
	  }
        }
      }
      /*     now create in form for OSL */
      ndense = nrow - fact->npivots;
      idense = ndense;
      for (count = ndense; count >= 1; --count) {
	/*        Find a pivot element */
        ipivot = maction[count];
        krs = mrstrt[ipivot];
        --idense;
        jcol = mcol[count];
        jpivot = hcoli[krlast + jcol];
        ++fact->npivots;
        pivot = dluval[krs + idense];
        if (pivot == 0.) {
	  hinrow[ipivot] = 0;
	  rlink[ipivot].pre = -nrow - 1;
	  ++(*nsingp);
	  irtcod = 10;
        } else {
	  rlink[ipivot].pre = -fact->npivots;
	  clink[jpivot].pre = -fact->npivots;
	  hincol[jpivot] = 0;
	  ++fact->xnetal;
	  mcstrt[fact->xnetal] = lstart - 1;
	  hpivco[fact->xnetal] = ipivot;
	  for (jrow = idense; jrow >= 1; --jrow) {
	    irow = maction[jrow];
	    krxs = mrstrt[irow];
	    multip = dluval[krxs + idense];
	    /*           for moment skip if zero */
	    if (multip != 0.||storeZero) {
	      /* Store elementary row transformation */
	      ++nnentl;
	      --nnentu;
	      --lstart;
	      dluval[lstart] = multip;
	      hrowi[lstart] = SHIFT_INDEX(irow);
	    }
	  }
	  hcoli[krlast + jcol] = hcoli[krlast + idense];
	  /*         update pivot row and last row HCOLI */
	  dluval[krs + idense] = dluval[krs];
	  hcoli[krlast + idense] = hcoli[krlast];
	  nz = 1;
	  dluval[krs] = pivot;
	  hcoli[krs] = jpivot;
	  if (!storeZero) {
	    for (i = 1; i <= idense; ++i) {
	      if (fabs(dluval[krs + i]) > fact->zeroTolerance) {
		++nz;
		hcoli[krs + nz - 1] = hcoli[krlast + i];
		dluval[krs + nz - 1] = dluval[krs + i];
	      }
	    }
	    hinrow[ipivot] = nz;
	  } else {
	    for (i = 1; i <= idense; ++i) {
	      ++nz;
	      hcoli[krs + nz - 1] = hcoli[krlast + i];
	      dluval[krs + nz - 1] = dluval[krs + i];
	    }
	    hinrow[ipivot] = nz;
	  }
        }
      }
    }
  }

  *nnentlp = nnentl;
  *nnentup = nnentu;

  return (irtcod);
} /* c_ekkcmfd */
/* ***C_EKKCMFC */

/*
 * Generate a variant of c_ekkcmfc that uses an maction array of type
 * int rather than short.
 */
#undef MACTION_T
#define C_EKKCMFY
#define COIN_OSL_CMFC
#define MACTION_T int
int c_ekkcmfy(EKKfactinfo *fact,
	    EKKHlink *rlink, EKKHlink *clink,
	    EKKHlink *mwork, void *maction_void,
	    int nnetas,
	    int *nsingp, int *xrejctp,
	    int *xnewrop, int xnewco,
	    int *ncompactionsp)

#include "CoinOslC.h"
#undef COIN_OSL_CMFC
#undef C_EKKCMFY
#undef MACTION_T
int c_ekkford(const EKKfactinfo *fact,const int *hinrow, const int *hincol,
	    int *hpivro, int *hpivco,
	    EKKHlink *rlink, EKKHlink *clink)
{
  int i, iri, nzi;
  const int nrow	= fact->nrow;
  int nsing = 0;

  /*     Uwe H. Suhl, August 1986 */
  /*     Builds linked lists of rows and cols of nucleus for efficient */
  /*     pivot searching. */

  memset(hpivro+1,0,nrow*sizeof(int));
  memset(hpivco+1,0,nrow*sizeof(int));
  for (i = 1; i <= nrow; ++i) {
    //hpivro[i] = 0;
    //hpivco[i] = 0;
    assert(rlink[i].suc == 0);
    assert(clink[i].suc == 0);
  }

  /*     Generate double linked list of rows having equal numbers of */
  /*     nonzeros in each row. Skip pivotal rows. */
  for (i = 1; i <= nrow; ++i) {
    if (! (rlink[i].pre < 0)) {
      nzi = hinrow[i];
      if (nzi <= 0) {
	++nsing;
	rlink[i].pre = -nrow - 1;
      }
      else {
	iri = hpivro[nzi];
	hpivro[nzi] = i;
	rlink[i].suc = iri;
	rlink[i].pre = 0;
	if (iri != 0) {
	  rlink[iri].pre = i;
	}
      }
    }
  }

  /*     Generate double linked list of cols having equal numbers of */
  /*     nonzeros in each col. Skip pivotal cols. */
  for (i = 1; i <= nrow; ++i) {
    if (! (clink[i].pre < 0)) {
      nzi = hincol[i];
      if (nzi <= 0) {
	++nsing;
	clink[i].pre = -nrow - 1;
      }
      else {
	iri = hpivco[nzi];
	hpivco[nzi] = i;
	clink[i].suc = iri;
	clink[i].pre = 0;
	if (iri != 0) {
	  clink[iri].pre = i;
	}
      }
    }
  }

  return (nsing);
} /* c_ekkford */

/*    c version of OSL from 36100 */

/*     Assumes that a basis exists in correct form */
/*     Calls Uwe's routines (approximately) */
/*     Then if OK shuffles U into column order */
/*     Return codes: */

/*     0: everything ok */
/*     1: everything ok but performance would be better if more space */
/*        would be make available */
/*     4: growth rate of element in U too big */
/*     5: not enough space in row file */
/*     6: not enough space in column file */
/*     7: pivot too small - col sing */
/*     8: pivot too small - row sing */
/*    10: matrix is singular */

/* I suspect c_ekklfct never returns 1 */
/*
 * layout of data
 *
 * dluval/hcoli:	(L^-1)B	- hole - L factors
 *
 * The L factors are written from high to low, starting from nnetas.
 * There are nnentl factors in L.  lstart the next entry to use for the
 * L factors.  Eventually, (L^-1)B turns into U.

 * The ninbas coefficients of matrix B are originally in the start of
 * dluval/hcoli.  As L transforms it, rows may have to be expanded.
 * If there is room, they are copied to the start of the hole,
 * otherwise the first part of this area is compacted, and hopefully
 * there is then room.
 * There are nnentu coefficients in (L^-1)B.
 * nnentu + nnentl >= ninbas.
 * nnentu + nnentl == ninbas if there has been no fill-in.
 * nnentu is decreased when the pivot eliminates elements
 * (in which case there is a corresponding increase in nnentl),
 * and if pivoting happens to cancel out factors (in which case
 * there is no corresponding increase in L).
 * nnentu is increased if there is fill-in (no decrease in L).
 * If nnentu + nnentl >= nnetas, then we've run out of room.
 * It is not the case that the elements of (L^-1)B are all in the
 * first nnentu positions of dluval/hcoli, but that is of course
 * the lower bound on the number of positions needed to store it.
 * nuspik is roughly the sum of the row lengths of the rows that were pivoted
 * out.  singleton rows in c_ekktria do not change nuspik, but
 * c_ekkrsin does increment it for each singleton row.
 * That is, there are nuspik elements that in the upper part of (L^-1)B,
 * and (nnentu - nuspik) elements left in B.
 */
/*
 * As part of factorization, we test candidate pivots for numerical
 * stability; if the largest element in a row/col is much larger than
 * the smallest, this generally causes problems.  To easily determine
 * what the largest element is, we ensure that it is always in front.
 * This establishes this property; later on we take steps to preserve it.
 */
static void c_ekkmltf(const EKKfactinfo *fact,double *dluval, int *hcoli,
		    const int *mrstrt, const int *hinrow,
		    const EKKHlink *rlink)
{
#if 0
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
  int i, j, k;
  int koff=-1;
  const int nrow	= fact->nrow;


  for (i = 1; i <= nrow; ++i) {
    /* ignore rows that have already been pivoted */
    /* if it is a singleton row, the property trivially holds */
    if (! (rlink[i].pre < 0 || hinrow[i] <= 1)) {
      const int krs = mrstrt[i];
      const int kre = krs + hinrow[i] - 1;

      double maxaij = 0.f;

      /* this assumes that at least one of the dluvals is non-zero. */
      for (k = krs; k <= kre; ++k) {
	if (! (fabs(dluval[k]) <= maxaij)) {
	  maxaij = fabs(dluval[k]);
	  koff = k;
	}
      }
      assert (koff>0);
      maxaij = dluval[koff];
      j = hcoli[koff];

      dluval[koff] = dluval[krs];
      hcoli[koff] = hcoli[krs];

      dluval[krs] = maxaij;
      hcoli[krs] = j;
    }
  }
} /* c_ekkmltf */
int c_ekklfct( EKKfactinfo *fact)
{
  const int nrow	= fact->nrow;
  int ninbas = fact->xcsadr[nrow+1]-1;
  int ifvsol = fact->ifvsol;
  int *hcoli	= fact->xecadr;
  double *dluval	= fact->xeeadr;
  int *mrstrt	= fact->xrsadr;
  int *hrowi	= fact->xeradr;
  int *mcstrt	= fact->xcsadr;
  int *hinrow	= fact->xrnadr;
  int *hincol	= fact->xcnadr;
  int *hpivro	= fact->krpadr;
  int *hpivco	= fact->kcpadr;


  EKKHlink *rlink	= fact->kp1adr;
  EKKHlink *clink	= fact->kp2adr;
  EKKHlink *mwork	= (reinterpret_cast<EKKHlink*>(fact->kw1adr))-1;

  int nsing, kdnspt, xnewro, xnewco;
  int i;
  int xrejct;
  int irtcod;
  const int nnetas	= fact->nnetas;

  int ncompactions;
  double save_drtpiv = fact->drtpiv;
  double save_zpivlu = fact->zpivlu;
  if (ifvsol > 0 && fact->invok < 0) {
    fact->zpivlu =  CoinMin(0.9, fact->zpivlu * 10.);
    fact->drtpiv=1.0e-8;
  }

  rlink --;
  clink --;

  /* Function Body */
  hcoli[nnetas] = 1;
  hrowi[nnetas] = 1;
  dluval[nnetas] = 0.0;
  /*     set amount of work */
  xrejct = 0;
  nsing = 0;
  kdnspt = nnetas + 1;
  fact->ndenuc = 0;
  /*     Triangularize */
  irtcod = c_ekktria(fact,rlink,clink,
		   &nsing,
		   &xnewco, &xnewro,
		   &ncompactions, ninbas);
  fact->nnentl = ninbas - fact->nnentu;

  if (irtcod < 0) {
    /* no space or system error */
    goto L8000;
  }

  if (irtcod != 0 && fact->invok >= 0) {
    goto L8500;	/* 7 or 8 - pivot too small */
  }
#if 0
  /* is this necessary ? */
  lstart = nnetas - fact->nnentl + 1;
  for (i = lstart; i <= nnetas; ++i) {
    hrowi[i] = (hcoli[i] << 3);
  }
#endif

  /* See if finished */
  if (! (fact->npivots >= nrow)) {
    int nsing1;

    /*     No - do nucleus */

    nsing1 = c_ekkford(fact,hinrow, hincol, hpivro, hpivco, rlink, clink);
    nsing+= nsing1;
    if (nsing1 != 0 && fact->invok >= 0) {
      irtcod=7;
      goto L8500;
    }
    c_ekkmltf(fact,dluval, hcoli, mrstrt, hinrow, rlink);

    {
      bool callcmfy = false;

      if (nrow > 32767) {
	int count = 0;
	for (i = 1; i <= nrow; ++i) {
	  count = CoinMax(count,hinrow[i]);
	}
	if (count + nrow - fact->npivots > 32767) {
	  /* will have to use I*4 version of CMFC */
	  /* no changes to pointer params */
	  callcmfy = true;
	}
      }

      irtcod = (callcmfy ? c_ekkcmfy : c_ekkcmfc)
	(fact,
	 rlink, clink,
	 mwork, &mwork[nrow + 1],
	 nnetas,
	 &nsing, &xrejct,
	 &xnewro, xnewco,
	 &ncompactions);

      /* irtcod one of 0,-5,7,10 */
    }

    if (irtcod < 0) {
      goto L8000;
    }
    kdnspt = nnetas - fact->nnentl;
  }

  /*     return if error */
  if (nsing > 0 || irtcod == 10) {
    irtcod = 99;
  }
  /* irtcod one of 0,7,99 */

  if (irtcod != 0) {
    goto L8500;
  }
  ++fact->xnetal;
  mcstrt[fact->xnetal] = nnetas - fact->nnentl;

  /* give message if tight on memory */
  if (ncompactions > 2 ) {
    if (1) {
      int etasize =CoinMax(4*fact->nnentu+(nnetas-fact->nnentl)+1000,fact->eta_size);
      fact->eta_size=CoinMin(static_cast<int>(1.2*fact->eta_size),etasize);
      if (fact->maxNNetas>0&&fact->eta_size>
	  fact->maxNNetas) {
	fact->eta_size=fact->maxNNetas;
      }
    } /* endif */
  }
  /*       Shuffle U and multiply L by 8 (if assembler) */
  {
    int jrtcod = c_ekkshff(fact, clink, rlink,
			   xnewro);

    /* nR_etas is the number of R transforms;
     * it is incremented only in c_ekketsj.
     */
    fact->nR_etas = 0;
    /*fact->R_etas_start = mcstrt+nrow+fact->nnentl+3;*/
    fact->R_etas_start[1] = /*kdnspt - 1*/0;	/* magic */
    fact->R_etas_index = &fact->xeradr[kdnspt - 1];
    fact->R_etas_element = &fact->xeeadr[kdnspt - 1];



    if (jrtcod != 0) {
      irtcod = jrtcod;
      /* irtcod == 2 */
    }
  }
  goto L8500;

  /* Fatal error */
 L8000:

  if (1) {
    if (fact->maxNNetas != fact->eta_size &&
	nnetas) {
      /* return and get more space */

      /* double eta_size, unless that exceeds max (if there is one) */
      fact->eta_size = fact->eta_size<<1;
      if (fact->maxNNetas > 0 &&
	  fact->eta_size > fact->maxNNetas) {
	fact->eta_size = fact->maxNNetas;
      }
      return (5);
    }
  }
  /*c_ekkmesg_no_i1(121, -irtcod);*/
  irtcod = 3;

 L8500:
  /* restore pivot tolerance */
  fact->drtpiv=save_drtpiv;
  fact->zpivlu=save_zpivlu;
#ifndef NDEBUG
  if (fact->rows_ok) {
    int * hinrow=fact->xrnadr;
    if (!fact->xe2adr) {
      for (int i=1;i<=fact->nrow;i++) {
	assert (hinrow[i]>=0&&hinrow[i]<=fact->nrow);
      }
    }
  }
#endif
  return (irtcod);
} /* c_ekklfct */


/*
  summary of return codes

  c_ekktria:
  7 small pivot
  -5 no memory

  c_ekkcsin:
  returns true if small pivot

  c_ekkrsin:
  -5 no memory
  7 small pivot

  c_ekkfpvt:
  10: no pivots found (singular)

  c_ekkcmfd:
  10: zero pivot (not just small)

  c_ekkcmfc:
  -5:  no memory
  any non-zero code from c_ekkcsin, c_ekkrsin, c_ekkfpvt, c_ekkprpv, c_ekkcmfd

  c_ekkshff:
  2:  singular

  c_ekklfct:
  any positive code from c_ekktria, c_ekkcmfc, c_ekkshff (2,7,10)
  *except* 10, which is changed to 99.
  all negative return codes are changed to 5 or 3
  (5 == ran out of memory but could get more,
  3 == ran out of memory, no luck)
  so:  2,3,5,7,99

  c_ekklfct1:
  1: c_ekksmem_invert failed
  2: c_ekkslcf/c_ekkslct ran out of room
  any return code from c_ekklfct, except 2 and 5
*/

void c_ekkrowq(int *hrow, int *hcol, double *dels,
	     int *mrstrt,
	     const int *hinrow, int nnrow, int ninbas)
{
  int i, k, iak, jak;
  double daik;
  int iloc;
  double dsave;
  int isave, jsave;

  /* Order matrix rowwise using MRSTRT, DELS, HCOL */

  k = 1;
  /* POSITION AFTER END OF ROW */
  for (i = 1; i <= nnrow; ++i) {
    k += hinrow[i];
    mrstrt[i] = k;
  }

  for (k = ninbas; k >= 1; --k) {
    iak = hrow[k];
    if (iak != 0) {
      daik = dels[k];
      jak = hcol[k];
      hrow[k] = 0;
      while (1) {
	--mrstrt[iak];

	iloc = mrstrt[iak];

	dsave = dels[iloc];
	isave = hrow[iloc];
	jsave = hcol[iloc];
	dels[iloc] = daik;
	hrow[iloc] = 0;
	hcol[iloc] = jak;

	if (isave == 0)
	  break;
	daik = dsave;
	iak = isave;
	jak = jsave;
      }

    }
  }
} /* c_ekkrowq */



int c_ekkrwco(const EKKfactinfo *fact,double *dluval,
	    int *hcoli, int *mrstrt, int *hinrow, int xnewro)
{
  int i, k, nz, kold;
  int kstart;
  const int nrow	= fact->nrow;

  for (i = 1; i <= nrow; ++i) {
    nz = hinrow[i];
    if (0 < nz) {
      /* save the last column entry of row i in hinrow */
      /* and replace that entry with -i */
      k = mrstrt[i] + nz - 1;
      hinrow[i] = hcoli[k];
      hcoli[k] = -i;
    }
  }

  kstart = 0;
  kold = 0;
  for (k = 1; k <= xnewro; ++k) {
    if (hcoli[k] != 0) {
      ++kstart;

      /* if this is the last entry for the row... */
      if (hcoli[k] < 0) {
	/* restore the entry */
	i = -hcoli[k];
	hcoli[k] = hinrow[i];

	/* update mrstart and hinrow */
	/* ACTUALLY, hinrow should already be accurate */
	mrstrt[i] = kold + 1;
	hinrow[i] = kstart - kold;
	kold = kstart;
      }

      /* move the entry */
      dluval[kstart] = dluval[k];
      hcoli[kstart] = hcoli[k];
    }
  }

  return (kstart);
} /* c_ekkrwco */



int c_ekkrwcs(const EKKfactinfo *fact,double *dluval, int *hcoli, int *mrstrt,
	    const int *hinrow, const EKKHlink *mwork,
	    int nfirst)
{
#if 0
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
  int i, k, k1, k2, nz;
  int irow, iput;
  const int nrow	= fact->nrow;

  /*     Compress row file */

  iput = 1;
  irow = nfirst;
  for (i = 1; i <= nrow; ++i) {
    nz = hinrow[irow];
    k1 = mrstrt[irow];
    if (k1 != iput) {
      mrstrt[irow] = iput;
      k2 = k1 + nz - 1;
      for (k = k1; k <= k2; ++k) {
	dluval[iput] = dluval[k];
	hcoli[iput] = hcoli[k];
	++iput;
      }
    } else {
      iput += nz;
    }
    irow = mwork[irow].suc;
  }

  return (iput);
} /* c_ekkrwcs */
void c_ekkrwct(const EKKfactinfo *fact,double *dluval, int *hcoli, int *mrstrt,
	     const int *hinrow, const EKKHlink *mwork,
	     const EKKHlink *rlink,
	     const short *msort, double *dsort,
	     int nlast, int xnewro)
{
#if 0
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
  int i, k, k1, nz, icol;
  int kmax;
  int irow, iput;
  int ilook;
  const int nrow	= fact->nrow;

  iput = xnewro;
  irow = nlast;
  kmax = nrow - fact->npivots;
  for (i = 1; i <= nrow; ++i) {
    nz = hinrow[irow];
    k1 = mrstrt[irow] - 1;
    if (rlink[irow].pre < 0) {
      /* pivoted on already */
      iput -= nz;
      if (k1 != iput) {
	mrstrt[irow] = iput + 1;
	for (k = nz; k >= 1; --k) {
	  dluval[iput + k] = dluval[k1 + k];
	  hcoli[iput + k] = hcoli[k1 + k];
	}
      }
    } else {
      /* not pivoted - going dense */
      iput -= kmax;
      mrstrt[irow] = iput + 1;
      c_ekkdzero( kmax, &dsort[1]);
      for (k = 1; k <= nz; ++k) {
	icol = hcoli[k1 + k];
	ilook = msort[icol];
	dsort[ilook] = dluval[k1 + k];
      }
      c_ekkdcpy(kmax,
	      (dsort+1), (dluval+iput + 1));
    }
    irow = mwork[irow].pre;
  }
} /* c_ekkrwct */
/*     takes Uwe's modern structures and puts them back 20 years */
int c_ekkshff(EKKfactinfo *fact,
	    EKKHlink *clink, EKKHlink *rlink,
	    int xnewro)
{
  int *hpivro	= fact->krpadr;

  int i, j;
  int nbas, icol;
  int ipiv;
  const int nrow	= fact->nrow;
  int nsing;

  for (i = 1; i <= nrow; ++i) {
    j = -rlink[i].pre;
    rlink[i].pre = j;
    if (j > 0 && j <= nrow) {
      hpivro[j] = i;
    }
    j = -clink[i].pre;
    clink[i].pre = j;
  }
  /* hpivro[j] is now (hopefully) the row that was pivoted on step j */
  /* rlink[i].pre is the step in which row i was pivoted */

  nbas = 0;
  nsing = 0;
  /* Decide if permutation wanted */
  fact->first_dense=nrow-fact->ndenuc+1+1;
  fact->last_dense=nrow;

  /* rlink[].suc is dead at this point */

  /*
   * replace the the basis index
   * with the pivot (or permuted) index generated by factorization.
   * This eventually goes into mpermu.
   */
  for (icol = 1; icol <= nrow; ++icol) {
    int ibasis = icol;
    ipiv = clink[ibasis].pre;

    if (0 < ipiv && ipiv <= nrow) {
      rlink[ibasis].suc = ipiv;
      ++nbas;
    }
  }

  nsing = nrow - nbas;
  if (nsing > 0) {
    abort();
  }

  /* if we reach here, then rlink[1..nrow].suc == clink[1..nrow].pre */

  /* switch off sparse update if any dense section */
  {
    const int notMuchRoom = (fact->nnentu + xnewro + 10 > fact->nnetas - fact->nnentl);

    /* must be same as in c_ekkshfv */
    if (fact->ndenuc || notMuchRoom||nrow<C_EKK_GO_SPARSE) {
#if PRINT_DEBUG
      if (fact->if_sparse_update) {
	printf("**** Switching off sparse update - dense - c_ekkshff\n");
      }
#endif
      fact->if_sparse_update=0;
    }
  }

  /* hpivro[1..nrow] is not read by c_ekkshfv */
  c_ekkshfv(fact,
	    rlink, clink,
	  xnewro);

  return (0);
} /* c_ekkshff */
/* sorts on indices dragging elements with */
static void c_ekk_sort2(int * key , double * array2,int number)
{
  int minsize=10;
  int n = number;
  int sp;
  int *v = key;
  int *m, t;
  int * ls[32] , * rs[32];
  int *l , *r , c;
  double it;
  int j;
  /*check already sorted  */
#ifndef LONG_MAX
#define LONG_MAX 0x7fffffff;
#endif
  int last=-LONG_MAX;
  for (j=0;j<number;j++) {
    if (key[j]>=last) {
      last=key[j];
    } else {
      break;
    } /* endif */
  } /* endfor */
  if (j==number) {
    return;
  } /* endif */
  sp = 0 ; ls[sp] = v ; rs[sp] = v + (n-1) ;
  while( sp >= 0 )
    {
      if ( rs[sp] - ls[sp] > minsize )
	{
	  l = ls[sp] ; r = rs[sp] ; m = l + (r-l)/2 ;
	  if ( *l > *m )
	    {
	      t = *l ; *l = *m ; *m = t ;
	      it = array2[l-v] ; array2[l-v] = array2[m-v] ; array2[m-v] = it ;
	    }
	  if ( *m > *r )
	    {
	      t = *m ; *m = *r ; *r = t ;
	      it = array2[m-v] ; array2[m-v] = array2[r-v] ; array2[r-v] = it ;
	      if ( *l > *m )
		{
		  t = *l ; *l = *m ; *m = t ;
		  it = array2[l-v] ; array2[l-v] = array2[m-v] ; array2[m-v] = it ;
		}
	    }
	  c = *m ;
	  while ( r - l > 1 )
	    {
	      while ( *(++l) < c ) ;
	      while ( *(--r) > c ) ;
	      t = *l ; *l = *r ; *r = t ;
	      it = array2[l-v] ; array2[l-v] = array2[r-v] ; array2[r-v] = it ;
	    }
	  l = r - 1 ;
	  if ( l < m )
	    {  ls[sp+1] = ls[sp] ;
	      rs[sp+1] = l      ;
	      ls[sp  ] = r      ;
	    }
	  else
	    {  ls[sp+1] = r      ;
	      rs[sp+1] = rs[sp] ;
	      rs[sp  ] = l      ;
	    }
	  sp++ ;
	}
      else sp-- ;
    }
  for ( l = v , m = v + (n-1) ; l < m ; l++ )
    {  if ( *l > *(l+1) )
	{
	  c = *(l+1) ;
	  it = array2[(l-v)+1] ;
	  for ( r = l ; r >= v && *r > c ; r-- )
	    {
	      *(r+1) = *r ;
	      array2[(r-v)+1] = array2[(r-v)] ;
	    }
	  *(r+1) = c ;
	  array2[(r-v)+1] = it ;
	}
    }
}
/*     For each row compute reciprocal of pivot element and  take out of */
/*     Also use HLINK(1 to permute column numbers */
/*     and HPIVRO to permute row numbers */
/*     Sort into column order as was stored by row */
/*     If Assembler then shift row numbers in L by 3 */
/*     Put column numbers in U for L-U update */
/*     and multiply U elements by - reciprocal of pivot element */
/*     and set up backward pointers for pivot rows */
void c_ekkshfv(EKKfactinfo *fact,
	       EKKHlink *rlink, EKKHlink *clink,
	     int xnewro)
{
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
  double *dpermu = fact->kadrpm;
  double * de2val = fact->xe2adr ? fact->xe2adr-1: 0;
  int nnentu	= fact->nnentu;
  int xnetal	= fact->xnetal;

  int numberSlacks; /* numberSlacks not read */

  int i, j, k, kk, nel;
  int nroom;
  bool need_more_space;
  int ndenuc=fact->ndenuc;
  int if_sparse_update=fact->if_sparse_update;
  int nnentl = fact->nnentl;
  int nnetas = fact->nnetas;

  int *ihlink	= (reinterpret_cast<int*> (clink))+1;	/* can't use rlink for simple loop below */

  const int nrow		= fact->nrow;
  const int maxinv	= fact->maxinv;

  /* this is not just a temporary - c_ekkbtrn etc use this */
  int *mpermu	= (reinterpret_cast<int*> (dpermu+nrow))+1;

  int * temp = ihlink+nrow;
  int * temp2 = temp+nrow;
  const int notMuchRoom = (nnentu + xnewro + 10 > nnetas - nnentl);

  /* compress hlink and make simpler */
  for (i = 1; i <= nrow; ++i) {
    mpermu[i] = rlink[i].pre;
    ihlink[i] = rlink[i].suc;
  }
  /* mpermu[i] == the step in which row i was pivoted */
  /* ihlink[i] == the step in which col i was pivoted */

  /* must be same as in c_ekkshff */
  if (fact->ndenuc||notMuchRoom||nrow<C_EKK_GO_SPARSE) {
    int ninbas;

    /* CHANGE COLUMN NUMBERS AND FILL IN RECIPROCALS */
    /* ALSO RECOMPUTE NUMBER IN COLUMN */
    /* initialize with a fake pivot in each column */
    c_ekkscpy_0_1(nrow, 1, &hincol[1]);

    if (notMuchRoom) {
      fact->eta_size=static_cast<int>(1.05*fact->eta_size);

      /* eta_size can be no larger than maxNNetas */
      if (fact->maxNNetas > 0 &&
	  fact->eta_size > fact->maxNNetas) {
	fact->eta_size=fact->maxNNetas;
      }
    } /* endif */

    /* For each row compute reciprocal of pivot element and take out of U */
    /* Also use ihlink to permute column numbers */
    /* the rows are not stored compactly or in order,
     * so we have to find out where the last one is stored */
    ninbas=0;
    for (i = 1; i <= nrow; ++i) {
      int jpiv=mpermu[i];
      int nin=hinrow[i];
      int krs = mrstrt[i];
      int kre = krs + nin;

      temp[jpiv]=krs;
      temp2[jpiv]=nin;

      ninbas = CoinMax(kre, ninbas);

      /* c_ekktria etc ensure that the first row entry is the pivot */
      dvalpv[jpiv] = 1. / dluval[krs];
      hcoli[krs] = 0;	/* probably needed for c_ekkrowq */
      /* room for the pivot has already been allocated, so hincol ok */

      for (kk = krs + 1; kk < kre; ++kk) {
	int j = ihlink[hcoli[kk]];
	hcoli[kk] = j;		/* permute the col index */
	hrowi[kk] = jpiv;	/* permute the row index */
	++hincol[j];
      }
    }
    /* temp [mpermu[i]] == mrstrt[i] */
    /* temp2[mpermu[i]] == hinrow[i] */

    ninbas--;	/* ???? */
    c_ekkscpy(nrow, &temp[1], &mrstrt[1]);
    c_ekkscpy(nrow, &temp2[1], &hinrow[1]);

    /* now mrstrt, hinrow, hcoli and hrowi have been permuted */

    /* Sort into column order as was stored by row */
    /* There will be an empty entry in front of each each column,
     * because we initialized hincol to 1s, and c_ekkrowq fills in
     * entries from the back */
    c_ekkrowq(hcoli, hrowi, dluval, mcstrt, hincol, nrow, ninbas);


    /* The shuffle zeroed out column pointers */
    /* Put them back for L-U update */
    /* Also multiply U elements by - reciprocal of pivot element */
    /* Also decrement mcstrt/hincol to give "real" sizes */
    for (i = 1; i <= nrow; ++i) {
      int kx = --mcstrt[i];
      nel = --hincol[i];
      hrowi[kx] = nel;
      dluval[kx] = dvalpv[i];
#ifndef NO_SHIFT
      for (int j=kx+1;j<=kx+nel;j++)
	hrowi[j] = SHIFT_INDEX(hrowi[j]);
#endif
    }

    /* sort dense part */
    for (i=nrow-ndenuc+1; i<=nrow; i++) {
      int kx = mcstrt[i]+1;	/* "real" entries start after pivot */
      int nel = hincol[i];
      c_ekk_sort2(&hrowi[kx],&dluval[kx],nel);
    }

    /* Recompute number in U */
    nnentu = mcstrt[nrow] + hincol[nrow];
    mcstrt[nrow + 4] = nnentu + 1;	/* magic - AND DEAD */

    /* as not much room switch off fast etas */
    mrstrt[1] = 0;			/* magic */
    fact->rows_ok = false;
    i = nrow + maxinv + 5;	/* DEAD */
  } else {
    /* *************************************** */
    /*       enough memory to do a bit faster */
    /*       For each row compute reciprocal of pivot element and */
    /*       take out of U */
    /*       Also use HLINK(1 to permute column numbers */
    int ninbas=0;
    int ilast; /* last available entry */
    int spareSpace;
    double * dluval2;
    /*int * hlink2 = ihlink+nrow;
      int * mrstrt2 = hlink2+nrow;*/
    /* mwork has order of row copy */
    EKKHlink *mwork	= (reinterpret_cast<EKKHlink*>(fact->kw1adr))-1;
    fact->rows_ok = true;

    if (if_sparse_update) {
      ilast=nnetas-nnentl;
    } else {
      /* missing out nnentl stuff */
      ilast=nnetas;
    }
    spareSpace=ilast-nnentu;
    need_more_space=false;
    /*     save clean row copy if enough room */
    nroom = (spareSpace) / nrow;
    if (nrow<10000) {
      if (nroom < 10) {
	need_more_space=true;
      }
    } else {
      if (nroom < 5&&!if_sparse_update) {
	need_more_space=true;
      }
    }
    if (nroom > CoinMin(50,maxinv)) {
      need_more_space=false;
    }
    if (need_more_space) {
      if (if_sparse_update) {
	int i1=fact->eta_size+10*nrow;
	fact->eta_size=static_cast<int>(1.2*fact->eta_size);
	if (i1>fact->eta_size) {
	  fact->eta_size=i1;
	}
      } else {
	fact->eta_size=static_cast<int>(1.05*fact->eta_size);
      }
    } else {
      if (nroom<11) {
	if (if_sparse_update) {
	  int i1=fact->eta_size+(11-nroom)*nrow;
	  fact->eta_size=static_cast<int>(1.2*fact->eta_size);
	  if (i1>fact->eta_size) {
	    fact->eta_size=i1;
	  }
	}
      }
    }
    if (fact->maxNNetas>0&&fact->eta_size>
	fact->maxNNetas) {
      fact->eta_size=fact->maxNNetas;
    }
    {
      /* we can swap de2val and dluval to save copying */
      int * eta_last=mpermu+nrow*2+3;
      int * eta_next=eta_last+nrow+2;
      int last=0;
      eta_last[0]=-1;
      if (nnentl) {
	/* went into c_ekkcmfc - if not then in order */
	int next;
	/*next=mwork[((nrow+1)<<1)+1];*/
	next=mwork[nrow+1].pre;
#ifdef DEBUG
	j=mrstrt[next];
#endif
	for (i = 1; i <= nrow; ++i) {
	  int iperm=mpermu[next];
	  eta_next[last]=iperm;
	  eta_last[iperm]=last;
	  temp[iperm] = mrstrt[next];
	  temp2[iperm] = hinrow[next];
#ifdef DEBUG
	  if (mrstrt[next]!=j) abort();
	  j=mrstrt[next]+hinrow[next];
#endif
	  /*next= mwork[(next<<1)+2];*/
	  next= mwork[next].suc;
	  last=iperm;
	}
      } else {
#ifdef DEBUG
	j=0;
#endif
	for (i = 1; i <= nrow; ++i) {
	  int iperm=mpermu[i];
	  eta_next[last]=iperm;
	  eta_last[iperm]=last;
	  temp[iperm] = mrstrt[i];
	  temp2[iperm] = hinrow[i];
	  last=iperm;
#ifdef DEBUG
	  if (mrstrt[i]<=j) abort();
	  if (i>1&&mrstrt[i]!=j+hinrow[i-1]) abort();
	  j=mrstrt[i];
#endif
	}
      }
      eta_next[last]=nrow+1;
      eta_last[nrow+1]=last;
      eta_next[nrow+1]=nrow+2;
      c_ekkscpy(nrow, &temp[1], &mrstrt[1]);
      c_ekkscpy(nrow, &temp2[1], &hinrow[1]);
      i=eta_last[nrow+1];
      ninbas=mrstrt[i]+hinrow[i]-1;
#ifdef DEBUG
      if (spareSpace<ninbas) {
        abort();
      }
#endif
      c_ekkizero( nrow, &hincol[1]);
#ifdef DEBUG
      for (i=nrow; i>0; i--) {
	int krs = mrstrt[i];
	int jpiv = hcoli[krs];
	if (ihlink[jpiv]!=i) abort();
      }
#endif
      for (i = 1; i <= ninbas; ++i) {
	k = hcoli[i];
	k = ihlink[k];
#ifdef DEBUG
        if (k<=0||k>nrow) abort();
#endif
	hcoli[i]=k;
	hincol[k]++;
      }
#ifdef DEBUG
      for (i=nrow; i>0; i--) {
	int krs = mrstrt[i];
	int jpiv = hcoli[krs];
	if (jpiv!=i) abort();
	if (krs>ninbas) abort();
      }
#endif
      /*       Sort into column order as was stored by row */
      k = 1;
      /*        Position */
      for (kk = 1; kk <= nrow; ++kk) {
	nel=hincol[kk];
        mcstrt[kk] = k;
	hrowi[k]=nel-1;
        k += hincol[kk];
	hincol[kk]=0;
      }
      if (de2val) {
	dluval2=de2val;
      } else {
	dluval2=dluval+ninbas;
      }
      nnentu = k-1;
      mcstrt[nrow + 4] = nnentu + 1;
      /* create column copy */
      for (i=nrow; i>0; i--) {
	int krs = mrstrt[i];
	int kre = krs + hinrow[i];
	hinrow[i]--;
	mrstrt[i]++;
        {
	  int kx = mcstrt[i];
	  /*nel = hincol[i];
	    if (hrowi[kx]!=nel) abort();
	    hrowi[kx] = nel-1;*/
	  dluval2[kx] = 1.0 /dluval[krs];
	  /*hincol[i]=0;*/
	  for (kk = krs + 1; kk < kre; ++kk) {
	    int j = hcoli[kk];
	    int iput = hincol[j]+1;
	    hincol[j]=iput;
	    iput+= mcstrt[j];
	    hrowi[iput] = SHIFT_INDEX(i);
	    dluval2[iput] = dluval[kk];
	  }
	}
      }
      if (de2val) {
	double * a=dluval;
	double * address;
	/* move first down */
	i=eta_next[0];
	{
	  int krs=mrstrt[i];
	  nel=hinrow[i];
	  for (j=1;j<=nel;j++) {
	    hcoli[j]=hcoli[j+krs-1];
	    dluval[j]=dluval[j+krs-1];
	  }
	}
	mrstrt[i]=1;
	/****** swap dluval and de2val !!!! ******/
	/* should work even for dspace */
	/* move L part across */
	address=fact->xeeadr+1;
	fact->xeeadr=fact->xe2adr-1;
	fact->xe2adr=address;
	if (nnentl) {
	  int n=xnetal-nrow-maxinv-5;
	  int j1,j2;
	  int * mcstrt2=mcstrt+nrow+maxinv+4;
	  j2 = mcstrt2[1];
	  j1 = mcstrt2[n+1]+1;
#if 0
	  memcpy(de2val+j1,dluval+j1,(j2-j1+1)*sizeof(double));
#else
	  c_ekkdcpy(j2-j1+1,
		  (dluval+j1),(de2val+j1));
#endif
	}
	dluval = de2val;
	de2val = a;
      } else {
	/* copy down dluval */
#if 0
	memcpy(&dluval[1],&dluval2[1],ninbas*sizeof(double));
#else
	c_ekkdcpy(ninbas,
		(dluval2+1),(dluval+1));
#endif
      }
      /* sort dense part */
      for (i=nrow-ndenuc+1;i<=nrow;i++) {
	int kx = mcstrt[i]+1;
	int nel = hincol[i];
	c_ekk_sort2(&hrowi[kx],&dluval[kx],nel);
      }
    }
    mrstrt[nrow + 1] = ilast + 1;
  }
  /* Find first non slack */
  for (i = 1; i <= nrow; ++i) {
    int kcs = mcstrt[i];
    if (hincol[i] != 0 || dluval[kcs] != SLACK_VALUE) {
      break;
    }
  }
  numberSlacks = i - 1;
  {
    /* set slacks to 1 */
    int * array = fact->krpadr + ( fact->nrowmx+2);
    int nSet = (numberSlacks)>>5;
    int n2 = (fact->nrowmx+32)>>5;
    int i;
    memset(array,0xff,nSet*sizeof(int));
    memset(array+nSet,0,(n2-nSet)*sizeof(int));
    for (i=nSet<<5;i<=numberSlacks;i++)
      c_ekk_Set(array,i);
    c_ekk_Unset(array,fact->nrow+1); /* make sure off end not slack */
#ifndef NDEBUG
    for (i=1;i<=numberSlacks;i++)
      assert (c_ekk_IsSet(array,i));
    for (;i<=fact->nrow;i++)
      assert (!c_ekk_IsSet(array,i));
#endif
  }

  /* and set up backward pointers */
  /* clean up HPIVCO for fancy assembler stuff */
  /* xnetal was initialized to nrow + maxinv + 4 in c_ekktria, and grows */
  c_ekkscpy_0_1(maxinv + 1, 1, &hpivco[nrow+4]);	/* magic */

  hpivco[xnetal] = 1;
  /* shuffle down for gaps so can get rid of hpivco for L */
  {
    const int lstart	= nrow + maxinv + 5;
    int n=xnetal-lstart ;	/* number of L entries */
    int add,iel;
    int * hpivco_L = &hpivco[lstart];
    int * mcstrt_L = &mcstrt[lstart];
    if (nnentl) {
      /* elements of L were stored in descending order in dluval/hcoli */
      int kle = mcstrt_L[0];
      int kls = mcstrt_L[n]+1;

      if(if_sparse_update) {
	int i2,iel;
	int * mrstrt2 = &mrstrt[nrow];

	/* need row copy of L */
	/* hpivro is spare for counts; just used as a temp buffer */
	c_ekkizero( nrow, &hpivro[1]);

	/* permute L indices; count L row lengths */
	for (iel = kls; iel <= kle; ++iel) {
	  int jrow = mpermu[UNSHIFT_INDEX(hrowi[iel])];
	  hpivro[jrow]++;
	  hrowi[iel] = SHIFT_INDEX(jrow);
	}
	{
	  int ibase=nnetas-nnentl+1;
	  int firstDoRow=0;
	  for (i=1;i<=nrow;i++) {
	    mrstrt2[i]=ibase;
	    if (hpivro[i]&&!firstDoRow) {
	      firstDoRow=i;
	    }
	    ibase+=hpivro[i];
	    hpivro[i]=mrstrt2[i];
	  }
	  if (!firstDoRow) {
	    firstDoRow=nrow+1;
	  }
	  mrstrt2[i]=ibase;
	  fact->firstDoRow = firstDoRow;
	}
	i2=mcstrt_L[n];
	for (i = n-1; i >= 0; --i) {
	  int i1 = mcstrt_L[i];
	  int ipiv=hpivco_L[i];
	  ipiv=mpermu[ipiv];
	  hpivco_L[i]=ipiv;
	  for (iel=i2 ; iel < i1; iel++) {
	    int irow = UNSHIFT_INDEX(hrowi[iel+1]);
	    int iput=hpivro[irow];
	    hpivro[irow]=iput+1;
	    hcoli[iput]=ipiv;
	    de2val[iput]=dluval[iel+1];
	  }
	  i2=i1;
	}
      } else {
	/* just permute row numbers */

	for (j = 0; j < n; ++j) {
	  hpivco_L[j] = mpermu[hpivco_L[j]];
	}
	for (iel = kls; iel <= kle; ++iel) {
	  int jrow = mpermu[UNSHIFT_INDEX(hrowi[iel])];
	  hrowi[iel] = SHIFT_INDEX(jrow);
	}
      }

      add=hpivco_L[n-1]-hpivco_L[0]-n+1;
      if (add) {
	int i;
	int last = hpivco_L[n-1];
	int laststart = mcstrt_L[n];
	int base=hpivco_L[0]-1;
	/* adjust so numbers match */
	mcstrt_L-=base;
	hpivco_L-=base;
	mcstrt_L[last]=laststart;
	for (i=n-1;i>=0;i--) {
	  int ipiv=hpivco_L[i+base];
	  while (ipiv<last) {
	    mcstrt_L[last-1]=laststart;
	    hpivco_L[last-1]=last;
	    last--;
	  }
	  laststart=mcstrt_L[i+base];
	  mcstrt_L[last-1]=laststart;
	  hpivco_L[last-1]=last;
	  last--;
	}
	xnetal+=add;
      }
    }
    //int lstart=fact->lstart;
    //const int * COIN_RESTRICT hpivco	= fact->kcpadr;
    fact->firstLRow = hpivco[lstart];
  }
  fact->nnentu = nnentu;
  fact->xnetal = xnetal;
  /* now we have xnetal * we can set up pointers */
  clp_setup_pointers(fact);

  /* this is the array used in c_ekkbtrn; it is passed to c_ekkbtju as hpivco.
   * this gets modified by F-T as we pivot columns in and out.
   */
  {
    /* do new hpivco */
    int * hpivco_new = fact->kcpadr+1;
    int * back = &fact->kcpadr[2*nrow+maxinv+4];
    /* set zeroth to stop illegal read */
    back[0]=1;

    hpivco_new[nrow+1]=nrow+1; /* deliberate loop for dense tests */
    hpivco_new[0]=1;

    for (i=1;i<=nrow;i++) {
      hpivco_new[i]=i+1;
      back[i+1]=i;
    }
    back[1]=0;

    fact->first_dense = CoinMax(fact->first_dense,4);
    fact->numberSlacks=numberSlacks;
    fact->lastSlack=numberSlacks;
    fact->firstNonSlack=hpivco_new[numberSlacks];
  }

  /* also zero out permute region and nonzero */
  c_ekkdzero( nrow, (dpermu+1));

  if (if_sparse_update) {
    char * nonzero = reinterpret_cast<char *> (&mpermu[nrow+1]);	/* used in c_ekkbtrn */
    /*c_ekkizero(nrow,(int *)nonzero);*/
    c_ekkczero(nrow,nonzero);
    /*memset(nonzero,0,nrow*sizeof(int));*/ /* for faster method */
  }
  for (i = 1; i <= nrow; ++i) {
    hpivro[mpermu[i]] = i;
  }

} /* c_ekkshfv */


static void c_ekkclcp1(const int *hcol, const int * mrstrt,
		     int *hrow, int *mcstrt,
		     int *hincol, int nnrow, int nncol,
		     int ninbas)
{
  int i, j, kc, kr, kre, krs, icol;
  int iput;

  /* Create columnwise storage of row indices */

  kc = 1;
  for (j = 1; j <= nncol; ++j) {
    mcstrt[j] = kc;
    kc += hincol[j];
    hincol[j] = 0;
  }
  mcstrt[nncol + 1] = ninbas + 1;

  for (i = 1; i <= nnrow; ++i) {
    krs = mrstrt[i];
    kre = mrstrt[i + 1] - 1;
    for (kr = krs; kr <= kre; ++kr) {
      icol = hcol[kr];
      iput = hincol[icol];
      hincol[icol] = iput + 1;
      iput += mcstrt[icol];
      hrow[iput] = i;
    }
  }
} /* c_ekkclcp */
inline void c_ekkclcp2(const int *hcol, const double *dels, const int * mrstrt,
		     int *hrow, double *dels2, int *mcstrt,
		     int *hincol, int nnrow, int nncol,
		     int ninbas)
{
  int i, j, kc, kr, kre, krs, icol;
  int iput;

  /* Create columnwise storage of row indices */

  kc = 1;
  for (j = 1; j <= nncol; ++j) {
    mcstrt[j] = kc;
    kc += hincol[j];
    hincol[j] = 0;
  }
  mcstrt[nncol + 1] = ninbas + 1;

  for (i = 1; i <= nnrow; ++i) {
    krs = mrstrt[i];
    kre = mrstrt[i + 1] - 1;
    for (kr = krs; kr <= kre; ++kr) {
      icol = hcol[kr];
      iput = hincol[icol];
      hincol[icol] = iput + 1;
      iput += mcstrt[icol];
      hrow[iput] = i;
      dels2[iput] = dels[kr];
    }
  }
} /* c_ekkclcp */
int c_ekkslcf( const EKKfactinfo *fact)
{
  int * hrow = fact->xeradr;
  int * hcol = fact->xecadr;
  double * dels = fact->xeeadr;
  int * hinrow = fact->xrnadr;
  int * hincol = fact->xcnadr;
  int * mrstrt = fact->xrsadr;
  int * mcstrt = fact->xcsadr;
  const int nrow = fact->nrow;
  int ninbas;
  /* space for etas */
  const int nnetas	= fact->nnetas;
  ninbas=mcstrt[nrow+1]-1;

  /* Now sort */
  if (ninbas << 1 > nnetas) {
    /* Put it in row order */
    int i,k;
    c_ekkrowq(hrow, hcol, dels, mrstrt, hinrow, nrow, ninbas);
    k = 1;
    for (i = 1; i <= nrow; ++i) {
      mrstrt[i] = k;
      k += hinrow[i];
    }
    mrstrt[nrow + 1] = k;

    /* make a column copy without the extra values */
    c_ekkclcp1(hcol, mrstrt, hrow, mcstrt, hincol, nrow, nrow, ninbas);
  } else {
    /* Move elements up memory */
    c_ekkdcpy(ninbas,
	    (dels+1), (dels+ninbas + 1));

    /* make a row copy with the extra values */
    c_ekkclcp2(hrow, &dels[ninbas], mcstrt, hcol, dels, mrstrt, hinrow, nrow, nrow, ninbas);
  }
  return (ninbas);
} /* c_ekkslcf */
/*     Uwe H. Suhl, September 1986 */
/*     Removes lower and upper triangular factors from the matrix. */
/*     Code for routine: 102 */
/*     Return codes: */
/*	0: ok */
/*      -5: not enough space in row file */
/*      7: pivot too small - col sing */
/*
 * This selects singleton columns and rows for the LU factorization.
 * Singleton columns require no
 *
 * (1) Note that columns are processed using a queue, not a stack;
 * this produces better pivots.
 *
 * (2) At most nrows elements are ever entered into the queue.
 *
 * (3) When pivoting singleton columns, every column that is part of
 * the pivot row is shortened by one, including the singleton column
 * itself; the hincol entries are updated appropriately.
 * Thus, pivoting on a singleton column may create other singleton columns
 * (but not singleton rows).
 * The dual property is true for rows.
 *
 * (4) Row entries (hrowi) are not changed when pivoting singleton columns.
 * Singleton columns that are created as a result of pivoting the
 * rows of other singleton columns will therefore have row entries
 * corresponding to those pivoted rows.  Since we need to find the
 * row entry for the row being pivoted, we have to
 * search its row entries for the one whose hlink entry indicates
 * that it has not yet been pivoted.
 *
 * (9) As a result of pivoting columns, sections in hrowi corresponding to
 * pivoted columns are no longer needed, and entries in sections
 * for non-pivoted columns may have entries corresponding to pivoted rows.
 * This is why hrowi needs to be compacted.
 *
 * (5) When the row_pre and col_pre fields of the hlink struct contain
 * negative values, they indicate that the row has been pivoted, and
 * the negative of that value is the pivot order.
 * That is the only use for these fields in this routine.
 *
 * (6) This routine assumes that hlink is initialized to zeroes.
 * Under this assumption, the following is an invariant in this routine:
 *
 *	(clink[i].pre < 0) ==> (hincol[i]==0)
 *
 * The converse is not true; see (15).
 *
 * The dual is also true, but only while pivoting singletong rows,
 * since we don't update hinrow while pivoting columns;
 * THESE VALUES ARE USED LATER, BUT I DON'T UNDERSTAND HOW YET.
 *
 * (7) hpivco is used for two purposes.  The low end is used to implement the
 * queue when pivoting columns; the high end is used to hold eta-matrix
 * entries.
 *
 * (8) As a result of pivoting columns, for all i:1<=i<=nrow, either
 *	hinrow[i] has not changed
 * or
 *	hinrow[i] = 0
 * This is another way of saying that pivoting singleton columns cannot
 * create singleton rows.
 * The dual holds for hincol after pivoting rows.
 *
 * (10) In constrast to (4), while pivoting rows we
 * do not let the hcoli get out-of-date.  That is because as part of
 * the process of numerical pivoting we have to find the row entries
 * for all the rows in the pivot column, so we may as well keep the
 * entries up to date.  This is done by moving the last column entry
 * for each row into the entry that was used for the pivot column.
 *
 * (11) When pivoting a column, we must find the pivot row entry in
 * its row table.  Sometimes we search for other things at the same time.
 * The same is true for pivoting columns.  This search should never
 * fail.
 *
 * (12) Information concerning the eta matrices is stored in the high
 * ends of arrays that are also used to store information concerning
 * the basis; these arrays are: hpivco, mcstrt, dluval and hcoli.
 * Information is only stored in these arrays as a part of pivoting
 * singleton rows, since the only thing that needs to be saved as
 * a part of pivoting singleton columns is which rows and columns were chosen,
 * and this is stored in hlink.
 * Since they have to share the same array, the eta information grows
 * downward instead of upward. Eventually, eta information may grow
 * down to the top of the basis information.  As pivoting proceeds,
 * more and more of this information is no longer needed, so when this
 * happens we can try compacting the arrays to see if we can recover
 * enough space.  lstart points at the bottom entry in the arrays,
 * xnewro/xnewco at the top of the basis information, and each time we
 * pivot a singleton row we know that we will need exactly as many new
 * entries as there are rows in the pivot column, so we can easily
 * determine if we need more room.  The variable maxinv may be used
 * to reserve extra room when inversion starts.
 *
 * (13) Eta information is stored in a fashion that is similar to how
 * matrices are stored.  There is one entry in hpivco and mcstrt for
 * each eta (other than the initial ones for singleton columns and
 * for singleton rows that turn out to be singleton columns),
 * in the order they were chosen.  hpivco records the pivot row,
 * and mcstrt points at the first entry in the other two arrays
 * for this row.  dluval contains the actual eta values for the column,
 * and hcoli the rows these values were in.
 * These entries in mcstrt and hpivco grow upward; they start above
 * the entries used to store basis information.
 * (Actually, I don't see why they need to start maxinv+4 entries past the top).
 *
 * (14) c_ekkrwco assumes that invalidated hrowi/hcoli entries contain 0.
 *
 * (15) When pivoting singleton columns, it may possibly happen
 * that a row with all singleton column entries is created.
 * In this case, all of the columns will be enqueued, and pivoting
 * on any of them eliminates the rest, without their being chosen
 * as pivots.  The dual holds for singleton rows.
 * DOES THIS INDICATE A SINGULARITY?
 *
 * (15) There are some aspects of the implementation that I find odd.
 * hrowi is not set to 0 for pivot rows while pivoting singleton columns,
 * which would make sense to me.  Things don't work if this isn't done,
 * so the information is used somehow later on.  Also, the information
 * for the pivot column is shifted to the front of the pivot row
 * when pivoting singleton columns; this is also necessary for reasons
 * I don't understand.
 */
int c_ekktria(EKKfactinfo *fact,
	      EKKHlink * rlink,
	      EKKHlink * clink,
	    int *nsingp,
	    int *xnewcop, int *xnewrop,
	    int *ncompactionsp,
	    const int ninbas)
{
  const int nrow	= fact->nrow;
  const int maxinv	= fact->maxinv;
  int *hcoli	= fact->xecadr;
  double *dluval	= fact->xeeadr;
  int *mrstrt	= fact->xrsadr;
  int *hrowi	= fact->xeradr;
  int *mcstrt	= fact->xcsadr;
  int *hinrow	= fact->xrnadr;
  int *hincol	= fact->xcnadr;
  int *stack	= fact->krpadr; /* normally hpivro */
  int *hpivco	= fact->kcpadr;
  const double drtpiv	= fact->drtpiv;
  CoinZeroN(reinterpret_cast<int *>(rlink+1),static_cast<int>(nrow*(sizeof(EKKHlink)/sizeof(int))));
  CoinZeroN(reinterpret_cast<int *>(clink+1),static_cast<int>(nrow*(sizeof(EKKHlink)/sizeof(int))));

  fact->npivots	= 0;
  /*      Use NUSPIK to keep sum of deactivated row counts */
  fact->nuspike	= 0;
  int xnetal	= nrow + maxinv + 4;
  int xnewro	= mrstrt[nrow] + hinrow[nrow] - 1;
  int xnewco	= xnewro;
  int kmxeta	= ninbas;
  int ncompactions	= 0;

  int i, j, k, kc, kce, kcs, npr;
  double pivot;
  int kipis, kipie, kjpis, kjpie, knprs, knpre;
  int ipivot, jpivot, stackc, stackr;
#ifndef NDEBUG
  int kpivot=-1;
#else
  int kpivot=-1;
#endif
  int epivco, kstart, maxstk;
  int irtcod = 0;
  int lastSlack=0;

  int lstart = fact->nnetas + 1;
  /*int nnentu	= ninbas; */
  int lstart_minus_nnentu=lstart-ninbas;
  /* do initial column singletons - as can do faster */
  for (jpivot = 1; jpivot <= nrow; ++jpivot) {
    if (hincol[jpivot] == 1) {
      ipivot = hrowi[mcstrt[jpivot]];
      if (ipivot>lastSlack) {
	lastSlack=ipivot;
      } else {
        /* so we can't put a structural over a slack */
	break;
      }
      kipis = mrstrt[ipivot];
#if 1
      assert (hcoli[kipis]==jpivot);
#else
      if (hcoli[kipis]!=jpivot) {
	kpivot=kipis+1;
	while(hcoli[kpivot]!=jpivot) kpivot++;
#ifdef DEBUG
	kipie = kipis + hinrow[ipivot] ;
	if (kpivot>=kipie) {
	  abort();
	}
#endif
        pivot=dluval[kpivot];
	dluval[kpivot] = dluval[kipis];
	dluval[kipis] = pivot;
	hcoli[kpivot] = hcoli[kipis];
	hcoli[kipis] = jpivot;
      }
#endif
      if (dluval[kipis]==SLACK_VALUE) {
	/* record the new pivot row and column */
	++fact->npivots;
	rlink[ipivot].pre = -fact->npivots;
	clink[jpivot].pre = -fact->npivots;
	hincol[jpivot]=0;
	fact->nuspike += hinrow[ipivot];
      } else {
	break;
      }
    } else {
      break;
    }
  }
  /* Fill queue with other column singletons and clean up */
  maxstk = 0;
  for (j = 1; j <= nrow; ++j) {
    if (hincol[j]) {
      int n=0;
      kcs = mcstrt[j];
      kce = mcstrt[j + 1];
      for (k = kcs; k < kce; ++k) {
	if (! (rlink[hrowi[k]].pre < 0)) {
	  n++;
	}
      }
      hincol[j] = n;
      if (n == 1) {
	/* we just created a new singleton column - enqueue it */
	++maxstk;
	stack[maxstk] = j;
      }
    }
  }
  stackc = 0; /* (1) */

  while (! (stackc >= maxstk)) {	/* (1) */
    /* dequeue the next entry */
    ++stackc;
    jpivot = stack[stackc];

    /* (15) */
    if (hincol[jpivot] != 0) {

      for (k = mcstrt[jpivot]; rlink[hrowi[k]].pre < 0; k++) {
	/* (4) */
      }
      ipivot = hrowi[k];

      /* All the columns in this row are being shortened. */
      kipis = mrstrt[ipivot];
      kipie = kipis + hinrow[ipivot] ;
      for (k = kipis; k < kipie; ++k) {
	j = hcoli[k];
	--hincol[j];	/* (3) (6) */

	if (j == jpivot) {
	  kpivot = k;		/* (11) */
	} else if (hincol[j] == 1) {
	  /* we just created a new singleton column - enqueue it */
	  ++maxstk;
	  stack[maxstk] = j;
	}
      }

      /* record the new pivot row and column */
      ++fact->npivots;
      rlink[ipivot].pre = -fact->npivots;
      clink[jpivot].pre = -fact->npivots;

      fact->nuspike += hinrow[ipivot];

      /* check the pivot */
      assert (kpivot>0);
      pivot = dluval[kpivot];
      if (fabs(pivot) < drtpiv) {
	irtcod = 7;
	++(*nsingp);
	rlink[ipivot].pre = -nrow - 1;
	clink[jpivot].pre = -nrow - 1;
      }

      /* swap the pivot column entry with the first one. */
      /* I don't know why. */
      dluval[kpivot] = dluval[kipis];
      dluval[kipis] = pivot;
      hcoli[kpivot] = hcoli[kipis];
      hcoli[kipis] = jpivot;
    }
  }
  /* (8) */

  /* The entire basis may already be triangular */
  if (fact->npivots < nrow) {

    /* (9) */
    kstart = 0;
    for (j = 1; j <= nrow; ++j) {
      if (! (clink[j].pre < 0)) {
	kcs = mcstrt[j];
	kce = mcstrt[j + 1];

	mcstrt[j] = kstart + 1;

	for (k = kcs; k < kce; ++k) {
	  if (! (rlink[hrowi[k]].pre < 0)) {
	    ++kstart;
	    hrowi[kstart] = hrowi[k];
	  }
	}
	hincol[j] = kstart - mcstrt[j] + 1;
      }
    }
    xnewco = kstart;


    /* Fill stack with initial row singletons that haven't been pivoted away */
    stackr = 0;
    for (i = 1; i <= nrow; ++i) {
      if (! (rlink[i].pre < 0) &&
	  (hinrow[i] == 1)) {
	++stackr;
	stack[stackr] = i;
      }
    }

    while (! (stackr <= 0)) {
      ipivot = stack[stackr];
      assert (ipivot);
      --stackr;

#if 1
      assert (rlink[ipivot].pre>=0);
#else
      /* This test is probably unnecessary:  rlink[i].pre < 0 ==> hinrow[i]==0 */
      if (rlink[ipivot].pre < 0) {
	continue;
      }
#endif

      /* (15) */
      if (hinrow[ipivot] != 0) {

	/* This is a singleton row, which means it has exactly one column */
	jpivot = hcoli[mrstrt[ipivot]];

	kjpis = mcstrt[jpivot];
	epivco = hincol[jpivot] - 1;
	hincol[jpivot] = 0;	/* this column is being pivoted away */

	/* (11) */
	kjpie = kjpis + epivco;
	for (k = kjpis; k <= kjpie; ++k) {
	  if (ipivot == hrowi[k])
	    break;
	}
	/* ASSERT (k <= kjpie) */

	/* move the last row entry for the pivot column into the pivot row's entry */
	/* I don't know why */
	hrowi[k] = hrowi[kjpie];

	/* invalidate the (old) last row entry of the pivot column */
	/* I don't know why */
	hrowi[kjpie] = 0;

	/* (12) */
	if (! (xnewro + epivco < lstart)) {
	  int kstart;

	  if (! (epivco < lstart_minus_nnentu)) {
	    irtcod = -5;
	    break;
	  }
	  kstart = c_ekkrwco(fact,dluval, hcoli, mrstrt, hinrow, xnewro);
	  ++ncompactions;
	  kmxeta += (xnewro - kstart) << 1;
	  xnewro = kstart;
	}
	if (! (xnewco + epivco < lstart)) {
	  if (! (epivco < lstart_minus_nnentu)) {
	    irtcod = -5;
	    break;
	  }
	  xnewco = c_ekkclco(fact,hrowi, mcstrt, hincol, xnewco);
	  ++ncompactions;

	  /*     HINCOL MAY HAVE CHANGED ??? (JJHF) */
	  epivco = hincol[jpivot];
	}

	/* record the new pivot row and column */
	++fact->npivots;
	rlink[ipivot].pre = -fact->npivots;
	clink[jpivot].pre = -fact->npivots;

	/* no update for nuspik */

	/* check the pivot */
	pivot = dluval[mrstrt[ipivot]];
	if (fabs(pivot) < drtpiv) {
	  /* If the pivot is too small, reject it, but keep going */
	  irtcod = 7;
	  rlink[ipivot].pre = -nrow - 1;
	  clink[jpivot].pre = -nrow - 1;
	}

	/* Perform numerical part of elimination. */
	if (! (epivco <= 0)) {
	  ++xnetal;
	  mcstrt[xnetal] = lstart - 1;
	  hpivco[xnetal] = ipivot;
	  pivot = -1.f / pivot;

	  kcs = mcstrt[jpivot];
	  kce = kcs + epivco - 1;
	  hincol[jpivot] = 0;

	  for (kc = kcs; kc <= kce; ++kc) {
	    npr = hrowi[kc];

	    /* why bother? */
	    hrowi[kc] = 0;

	    --hinrow[npr];	/* (3) */
	    if (hinrow[npr] == 1) {
	      /* this may create new singleton rows */
	      ++stackr;
	      stack[stackr] = npr;
	    }

	    /* (11) */
	    knprs = mrstrt[npr];
	    knpre = knprs + hinrow[npr];
	    for (k = knprs; k <= knpre; ++k) {
	      if (jpivot == hcoli[k]) {
		kpivot = k;
		break;
	      }
	    }
	    /* ASSERT (kpivot <= knpre) */

	    {
	      /* (10) */
	      double elemnt = dluval[kpivot];
	      dluval[kpivot] = dluval[knpre];
	      hcoli[kpivot] = hcoli[knpre];

	      hcoli[knpre] = 0;	/* (14) */

	      /* store elementary row transformation */
	      --lstart;
	      dluval[lstart] = elemnt * pivot;
	      hcoli[lstart] = npr;
	    }
	  }
	}
      }
    }
  }
  /* (8) */

  *xnewcop = xnewco;
  *xnewrop = xnewro;
  fact->xnetal = xnetal;
  fact->nnentu = lstart - lstart_minus_nnentu;
  fact->kmxeta = kmxeta;
  *ncompactionsp = ncompactions;

  return (irtcod);
} /* c_ekktria */
