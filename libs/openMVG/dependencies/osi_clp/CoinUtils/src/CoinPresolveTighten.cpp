/* $Id: CoinPresolveTighten.cpp 1518 2011-12-10 23:44:40Z lou $ */
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include <stdio.h>
#include <math.h>

#include "CoinPresolveMatrix.hpp"
#include "CoinPresolveFixed.hpp"
#include "CoinPresolveTighten.hpp"
#include "CoinPresolveUseless.hpp"
#include "CoinHelperFunctions.hpp"
#include "CoinFinite.hpp"

#if PRESOLVE_DEBUG > 0 || PRESOLVE_CONSISTENCY > 0
#include "CoinPresolvePsdebug.hpp"
#endif


const char *do_tighten_action::name() const
{
  return ("do_tighten_action");
}

/*
  This is ekkredc2.  This fairly simple transformation is not mentioned
  in the paper.  Say there is a costless variable x<t> such that all the
  constraints it's entangled with (i.e., a<it> != 0) would be satisfied
  as it approaches plus or minus infinity, because all its constraints
  have only one bound, and increasing/decreasing the variable makes the
  row activity grow away from the bound (in the right direction).

  If x<j> is unbounded in that direction, it can always be made large
  enough to satisfy the constraints, so we can just drop the variable and
  the entangled constraints from the problem.

  If x<j> *is* bounded in that direction, there is no reason not to set
  it to that bound.  This effectively weakens the constraints, which in
  fact may be subsequently presolved away.

  Note that none of the constraints may be bounded both above and below,
  since then we don't know which way to move the variable in order to
  satisfy the constraint.

  To drop constraints, we just make them useless and let other transformations
  take care of the rest.

  Note that more than one such costless unbounded variable may be part of
  a given constraint.  In that case, the first one processed will make
  the constraint useless, and the second will ignore it.  In postsolve,
  the first will be responsible for satisfying the constraint.

  Note that if the constraints are dropped (as in the first case), then we
  just make them useless.  It will subsequently be discovered the the variable
  does not appear in any constraints, and since it has no cost it is just set
  to some value (either zero or a bound) and removed (by remove_empty_cols).

  Oddly, pilots and baxter do *worse* when this transform is applied.

  It's informative to compare this transform to the very similar transform
  implemented in remove_dual_action. Surely they could be merged.
*/

const CoinPresolveAction *do_tighten_action::presolve(CoinPresolveMatrix *prob,
					       const CoinPresolveAction *next)
{
  double *colels	= prob->colels_;
  int *hrow		= prob->hrow_;
  CoinBigIndex *mcstrt		= prob->mcstrt_;
  int *hincol		= prob->hincol_;
  int ncols		= prob->ncols_;

  double *clo	= prob->clo_;
  double *cup	= prob->cup_;

  double *rlo	= prob->rlo_;
  double *rup	= prob->rup_;

  double *dcost	= prob->cost_;

  const unsigned char *integerType = prob->integerType_;

  int *fix_cols	= prob->usefulColumnInt_;
  int nfixup_cols	= 0;

  int nfixdown_cols	= ncols;

  int *useless_rows	= prob->usefulRowInt_;
  int nuseless_rows	= 0;
  
  action *actions	= new action [ncols];
  int nactions		= 0;

  int numberLook = prob->numberColsToDo_;
  int iLook;
  int * look = prob->colsToDo_;
  bool fixInfeasibility = ((prob->presolveOptions_&0x4000) != 0) ;

# if PRESOLVE_DEBUG > 0 || PRESOLVE_CONSISTENCY > 0
# if PRESOLVE_DEBUG > 0
  std::cout
    << "Entering do_tighten_action::presolve; considering " << numberLook
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


  // singleton columns are especially likely to be caught here
  for (iLook=0;iLook<numberLook;iLook++) {
    int j = look[iLook];
    // modify bounds if integer
    if (integerType[j]) {
      clo[j] = ceil(clo[j]-1.0e-12);
      cup[j] = floor(cup[j]+1.0e-12);
      if (clo[j]>cup[j]&&!fixInfeasibility) {
        // infeasible
	prob->status_|= 1;
	prob->messageHandler()->message(COIN_PRESOLVE_COLINFEAS,
					     prob->messages())
				 	       <<j
					       <<clo[j]
					       <<cup[j]
					       <<CoinMessageEol;
      }
    }
    if (dcost[j]==0.0) {
      int iflag=0; /* 1 - up is towards feasibility, -1 down is towards */
      int nonFree=0; // Number of non-free rows

      CoinBigIndex kcs = mcstrt[j];
      CoinBigIndex kce = kcs + hincol[j];

      // check constraints
      for (CoinBigIndex k=kcs; k<kce; ++k) {
	int i = hrow[k];
	double coeff = colels[k];
	double rlb = rlo[i];
	double rub = rup[i];

	if (-1.0e28 < rlb && rub < 1.0e28) {
	  // bounded - we lose
	  iflag=0;
	  break;
	} else if (-1.0e28 < rlb || rub < 1.0e28) {
	  nonFree++;
	}

	PRESOLVEASSERT(fabs(coeff) > ZTOLDP);

	// see what this particular row says
	// jflag == 1 ==> up is towards feasibility
	int jflag = (coeff > 0.0
		     ? (rub >  1.0e28 ? 1 : -1)
		     : (rlb < -1.0e28 ? 1 : -1));

	if (iflag) {
	  // check that it agrees with iflag.
	  if (iflag!=jflag) {
	    iflag=0;
	    break;
	  }
	} else {
	  // first row -- initialize iflag
	  iflag=jflag;
	}
      }
      // done checking constraints
      if (!nonFree)
	iflag=0; // all free anyway
      if (iflag) {
	if (iflag==1 && cup[j]<1.0e10) {
#if	PRESOLVE_DEBUG > 1
	  printf("TIGHTEN UP:  %d\n", j);
#endif
	  fix_cols[nfixup_cols++] = j;

	} else if (iflag==-1&&clo[j]>-1.0e10) {
	  // symmetric case
	  //mpre[j] = PRESOLVE_XUP;

#if	PRESOLVE_DEBUG > 1
	  printf("TIGHTEN DOWN:  %d\n", j);
#endif

	  fix_cols[--nfixdown_cols] = j;

	} else {
#if 0
	  static int limit;
	  static int which = atoi(getenv("WZ"));
	  if (which == -1)
	    ;
	  else if (limit != which) {
	    limit++;
	    continue;
	  } else
	    limit++;

	  printf("TIGHTEN STATS %d %g %g %d:  \n", j, clo[j], cup[j], integerType[j]); 
  double *rowels	= prob->rowels_;
  int *hcol		= prob->hcol_;
  int *mrstrt		= prob->mrstrt_;
  int *hinrow		= prob->hinrow_;
	  for (CoinBigIndex k=kcs; k<kce; ++k) {
	    int irow = hrow[k];
	    CoinBigIndex krs = mrstrt[irow];
	    CoinBigIndex kre = krs + hinrow[irow];
	    printf("%d  %g %g %g:  ",
		   irow, rlo[irow], rup[irow], colels[irow]);
	    for (CoinBigIndex kk=krs; kk<kre; ++kk)
	      printf("%d(%g) ", hcol[kk], rowels[kk]);
	    printf("\n");
	  }
#endif

	  {
	    action *s = &actions[nactions];	  
	    nactions++;
	    s->col = j;
	    PRESOLVE_DETAIL_PRINT(printf("pre_tighten %dC E\n",j));
	    if (integerType[j]) {
	      assert (iflag==-1||iflag==1);
	      iflag *= 2; // say integer
	    }
	    s->direction = iflag;

	    s->rows =   new int[hincol[j]];
	    s->lbound = new double[hincol[j]];
	    s->ubound = new double[hincol[j]];
#if         PRESOLVE_DEBUG > 1
	    printf("TIGHTEN FREE:  %d   ", j);
#endif
	    int nr = 0;
            prob->addCol(j);
	    for (CoinBigIndex k=kcs; k<kce; ++k) {
	      int irow = hrow[k];
	      // ignore this if we've already made it useless
	      if (! (rlo[irow] == -PRESOLVE_INF && rup[irow] == PRESOLVE_INF)) {
		prob->addRow(irow);
		s->rows  [nr] = irow;
		s->lbound[nr] = rlo[irow];
		s->ubound[nr] = rup[irow];
		nr++;

		useless_rows[nuseless_rows++] = irow;

		rlo[irow] = -PRESOLVE_INF;
		rup[irow] = PRESOLVE_INF;

#if             PRESOLVE_DEBUG > 1
		printf("%d ", irow);
#endif
	      }
	    }
	    s->nrows = nr;

#if         PRESOLVE_DEBUG > 1
	    printf("\n");
#endif
	  }
	}
      }
    }
  }


#if	PRESOLVE_SUMMARY > 0
  if (nfixdown_cols<ncols || nfixup_cols || nuseless_rows) {
    printf("NTIGHTENED:  %d %d %d\n", ncols-nfixdown_cols, nfixup_cols, nuseless_rows);
  }
#endif

  if (nuseless_rows) {
    next = new do_tighten_action(nactions, CoinCopyOfArray(actions,nactions), next);

    next = useless_constraint_action::presolve(prob,
					       useless_rows, nuseless_rows,
					       next);
  }
  deleteAction(actions, action*);
  //delete[]useless_rows;

  if (nfixdown_cols<ncols) {
    int * fixdown_cols = fix_cols+nfixdown_cols; 
    nfixdown_cols = ncols-nfixdown_cols;
    next = make_fixed_action::presolve(prob, fixdown_cols, nfixdown_cols,
				       true,
				       next);
  }
  //delete[]fixdown_cols;

  if (nfixup_cols) {
    next = make_fixed_action::presolve(prob, fix_cols, nfixup_cols,
				       false,
				       next);
  }
  //delete[]fixup_cols;

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
    << "Leaving do_tighten_action::presolve, " << droppedRows << " rows, "
    << droppedColumns << " columns dropped" ;
# if COIN_PRESOLVE_TUNING > 0
  std::cout << " in " << thisTime-startTime << "s" ;
# endif
  std::cout << "." << std::endl ;
# endif

  return (next);
}

void do_tighten_action::postsolve(CoinPostsolveMatrix *prob) const
{
  const action *const actions = actions_;
  const int nactions	= nactions_;

  double *colels	= prob->colels_;
  int *hrow		= prob->hrow_;
  CoinBigIndex *mcstrt		= prob->mcstrt_;
  int *hincol		= prob->hincol_;
  int *link		= prob->link_;

  double *clo	= prob->clo_;
  double *cup	= prob->cup_;
  double *rlo	= prob->rlo_;
  double *rup	= prob->rup_;

  double *sol	= prob->sol_;
  double *acts	= prob->acts_;

# if PRESOLVE_DEBUG > 0 || PRESOLVE_CONSISTENCY > 0
  char *cdone	= prob->cdone_;
  char *rdone	= prob->rdone_;

  presolve_check_threads(prob) ;
  presolve_check_sol(prob,2,2,2) ;
  presolve_check_nbasic(prob) ;

# if PRESOLVE_DEBUG > 0
  std::cout << "Entering do_tighten_action::postsolve." << std::endl ;
# endif
# endif

  for (const action *f = &actions[nactions-1]; actions<=f; f--) {
    int jcol = f->col;
    int iflag = f->direction;
    int nr   = f->nrows;
    const int *rows = f->rows;
    const double *lbound = f->lbound;
    const double *ubound = f->ubound;

    PRESOLVEASSERT(prob->getColumnStatus(jcol)!=CoinPrePostsolveMatrix::basic);
    int i;
    for (i=0;i<nr; ++i) {
      int irow = rows[i];

      rlo[irow] = lbound[i];
      rup[irow] = ubound[i];

     PRESOLVEASSERT(prob->getRowStatus(irow)==CoinPrePostsolveMatrix::basic);
    }

    // We have just tightened the row bounds.
    // That means we'll have to compute a new value
    // for this variable that will satisfy everybody.
    // We are supposed to be in a position where this
    // is always possible.

    // Each constraint has exactly one bound.
    // The correction should only ever be forced to move in one direction.
    //    double orig_sol = sol[jcol];
    double correction = 0.0;
    
    int last_corrected = -1;
    CoinBigIndex k = mcstrt[jcol];
    int nk = hincol[jcol];
    for (i=0; i<nk; ++i) {
      int irow = hrow[k];
      double coeff = colels[k];
      k = link[k];
      double newrlo = rlo[irow];
      double newrup = rup[irow];
      double activity = acts[irow];

      if (activity + correction * coeff < newrlo) {
	// only one of these two should fire
	PRESOLVEASSERT( ! (activity + correction * coeff > newrup) );

	last_corrected = irow;

	// adjust to just meet newrlo (solve for correction)
	double new_correction = (newrlo - activity) / coeff;
	//adjust if integer
	if (iflag==-2||iflag==2) {
	  new_correction += sol[jcol];
	  if (fabs(floor(new_correction+0.5)-new_correction)>1.0e-4) {
	    new_correction = ceil(new_correction)-sol[jcol];
#ifdef COIN_DEVELOP
	    printf("integer postsolve changing correction from %g to %g - flag %d\n",
		   (newrlo-activity)/coeff,new_correction,iflag);
#endif
	  }
	}
	correction = new_correction;
      } else if (activity + correction * coeff > newrup) {
	last_corrected = irow;

	double new_correction = (newrup - activity) / coeff;
	//adjust if integer
	if (iflag==-2||iflag==2) {
	  new_correction += sol[jcol];
	  if (fabs(floor(new_correction+0.5)-new_correction)>1.0e-4) {
	    new_correction = ceil(new_correction)-sol[jcol];
#ifdef COIN_DEVELOP
	    printf("integer postsolve changing correction from %g to %g - flag %d\n",
		   (newrup-activity)/coeff,new_correction,iflag);
#endif
	  }
	}
	correction = new_correction;
      }
    }

    if (last_corrected>=0) {
      sol[jcol] += correction;
      
      // by construction, the last row corrected (if there was one)
      // must be at its bound, so it can be non-basic.
      // All other rows may not be at a bound (but may if the difference
      // is very small, causing a new correction by a tiny amount).
      
      // now adjust the activities
      k = mcstrt[jcol];
      for (i=0; i<nk; ++i) {
	int irow = hrow[k];
	double coeff = colels[k];
	k = link[k];
	//      double activity = acts[irow];

	acts[irow] += correction * coeff;
      }
      /*
        If the col happens to get pushed to its bound, we may as well leave
	it non-basic. Otherwise, set the status to basic.

	Why do we correct the row status only when the column is made basic?
	Need to look at preceding code.  -- lh, 110528 --
      */
      if (fabs(sol[jcol]-clo[jcol]) > ZTOLDP &&
          fabs(sol[jcol]-cup[jcol]) > ZTOLDP) {
        
        prob->setColumnStatus(jcol,CoinPrePostsolveMatrix::basic);
	if (acts[last_corrected]-rlo[last_corrected] <
				rup[last_corrected]-acts[last_corrected])
	  prob->setRowStatus(last_corrected,
	  		     CoinPrePostsolveMatrix::atUpperBound);
	else
	  prob->setRowStatus(last_corrected,
	  		     CoinPrePostsolveMatrix::atLowerBound);
      }
    }
  }

# if PRESOLVE_DEBUG > 0 || PRESOLVE_CONSISTENCY > 0
  presolve_check_threads(prob) ;
  presolve_check_sol(prob,2,2,2) ;
  presolve_check_nbasic(prob) ;
# if PRESOLVE_DEBUG > 0
  std::cout << "Leaving do_tighten_action::postsolve." << std::endl ;
# endif
# endif
}

do_tighten_action::~do_tighten_action()
{
    if (nactions_ > 0) {
	for (int i = nactions_ - 1; i >= 0; --i) {
	    delete[] actions_[i].rows;
	    delete[] actions_[i].lbound;
	    delete[] actions_[i].ubound;
	}
	deleteAction(actions_, action*);
    }
}
