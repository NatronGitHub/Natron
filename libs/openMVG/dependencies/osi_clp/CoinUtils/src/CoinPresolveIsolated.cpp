/* $Id: CoinPresolveIsolated.cpp 1373 2011-01-03 23:57:44Z lou $ */
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include <stdio.h>
#include <math.h>

#include "CoinPresolveMatrix.hpp"
#include "CoinPresolveIsolated.hpp"
#include "CoinHelperFunctions.hpp"

#if PRESOLVE_DEBUG || PRESOLVE_CONSISTENCY
#include "CoinPresolvePsdebug.hpp"
#endif

// Rarely, there may a constraint whose variables only
// occur in that constraint.
// In this case it is a completely independent problem.
// We should be able to solve it right now.
// Since that is actually not trivial, I'm just going to ignore
// them and stick them back in at postsolve.
const CoinPresolveAction *isolated_constraint_action::presolve(CoinPresolveMatrix *prob,
							    int irow,
							    const CoinPresolveAction *next)
{
  int *hincol	= prob->hincol_;
  const CoinBigIndex *mcstrt	= prob->mcstrt_;
  int *hrow	= prob->hrow_;
  double *colels	= prob->colels_;

  double *clo	= prob->clo_;
  double *cup	= prob->cup_;

  const double *rowels	= prob->rowels_;
  const int *hcol	= prob->hcol_;
  const CoinBigIndex *mrstrt	= prob->mrstrt_;

  // may be written by useless constraint
  int *hinrow	= prob->hinrow_;

  double *rlo	= prob->rlo_;
  double *rup	= prob->rup_;

  CoinBigIndex krs = mrstrt[irow];
  CoinBigIndex kre = krs + hinrow[irow];
  
  double *dcost	= prob->cost_;
  const double maxmin	= prob->maxmin_;

# if PRESOLVE_DEBUG
  {
    printf("ISOLATED:  %d - ", irow);
    CoinBigIndex k;
    for ( k = krs; k<kre; ++k)
      printf("%d ", hcol[k]);
    printf("\n");
  }
# endif

  if (rlo[irow] != 0.0 || rup[irow] != 0.0) {
#   if PRESOLVE_DEBUG
    printf("can't handle non-trivial isolated constraints for now\n");
#   endif
    return NULL;
  }
  CoinBigIndex k;
  for ( k = krs; k<kre; ++k) {
    int jcol = hcol[k];
    if ((clo[jcol] != 0.0 && cup[jcol] != 0.0)||
        (maxmin*dcost[jcol] > 0.0 && clo[jcol] != 0.0) ||
        (maxmin*dcost[jcol] < 0.0 && cup[jcol] != 0.0) ){
#     if PRESOLVE_DEBUG
      printf("can't handle non-trivial isolated constraints for now\n");
#     endif
      return NULL;
    }
  }

  int nc = hinrow[irow];

#if 0
  double tableau = new double[nc];
  double sol = new double[nc];
  double clo = new double[nc];
  double cup = new double[nc];


  for (int i=0; i<nc; ++i) {
    int col = hcol[krs+1];
    tableau[i] = rowels[krs+i];
    clo[i] = prob->clo[krs+i];
    cup[i] = prob->cup[krs+i];

    sol[i] = clo[i];
  }
#endif

  // HACK - set costs to 0.0 so empty.cpp doesn't complain
  double *costs = new double[nc];
  for (k = krs; k<kre; ++k) {
    costs[k-krs] = dcost[hcol[k]];
    dcost[hcol[k]] = 0.0;
  }

  next = new isolated_constraint_action(rlo[irow], rup[irow],
					irow, nc,
					CoinCopyOfArray(&hcol[krs], nc),
					CoinCopyOfArray(&rowels[krs], nc),
					costs,
					next);

  for ( k=krs; k<kre; k++)
  { presolve_delete_from_col(irow,hcol[k],mcstrt,hincol,hrow,colels) ;
    if (hincol[hcol[k]] == 0)
    { PRESOLVE_REMOVE_LINK(prob->clink_,hcol[k]) ; } }
  hinrow[irow] = 0 ;
  PRESOLVE_REMOVE_LINK(prob->rlink_,irow) ;

  // just to make things squeeky
  rlo[irow] = 0.0;
  rup[irow] = 0.0;

# if CHECK_CONSISTENCY
  presolve_links_ok(prob) ;
  presolve_consistent(prob);
# endif

  return (next);
}

const char *isolated_constraint_action::name() const
{
  return ("isolated_constraint_action");
}

void isolated_constraint_action::postsolve(CoinPostsolveMatrix *prob) const
{
  double *colels	= prob->colels_;
  int *hrow		= prob->hrow_;
  CoinBigIndex *mcstrt		= prob->mcstrt_;
  int *link		= prob->link_;
  int *hincol		= prob->hincol_;
  
  double *rowduals	= prob->rowduals_;
  double *rowacts	= prob->acts_;
  double *sol		= prob->sol_;

  CoinBigIndex &free_list		= prob->free_list_;


  // hides fields
  double *rlo	= prob->rlo_;
  double *rup	= prob->rup_;

  double rowact = 0.0;

  int irow  = this->row_;

  rup[irow] = this->rup_;
  rlo[irow] = this->rlo_;
  int k;

  for (k=0; k<this->ninrow_; k++) {
    int jcol = this->rowcols_[k];

    sol[jcol] = 0.0;	// ONLY ACCEPTED SUCH CONSTRAINTS

    CoinBigIndex kk = free_list;
    assert(kk >= 0 && kk < prob->bulk0_) ;
    free_list = link[free_list];

    mcstrt[jcol] = kk;

    //rowact += rowels[k] * sol[jcol];

    colels[kk] = this->rowels_[k];
    hrow[kk]   = irow;
    link[kk] = NO_LINK ;

    hincol[jcol] = 1;
  }

# if PRESOLVE_CONSISTENCY
  presolve_check_free_list(prob) ;
# endif

  // ???
  prob->setRowStatus(irow,CoinPrePostsolveMatrix::basic);
    rowduals[irow] = 0.0;

  rowacts[irow] = rowact;

  // leave until desctructor
  //  deleteAction(rowcols_,int *);
  //  deleteAction(rowels_,double *);
  //  deleteAction(costs_,double *);
}

isolated_constraint_action::~isolated_constraint_action()
{
    deleteAction(rowcols_,int *);
    deleteAction(rowels_,double *);
    deleteAction(costs_,double *);
}
