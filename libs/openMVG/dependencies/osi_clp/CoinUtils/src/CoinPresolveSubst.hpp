/* $Id: CoinPresolveSubst.hpp 1562 2012-11-24 00:36:15Z lou $ */
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef CoinPresolveSubst_H
#define CoinPresolveSubst_H

/*!
  \file
*/
  
#define	SUBST_ROW	21

#include "CoinPresolveMatrix.hpp"

/*! \class subst_constraint_action
    \brief Detect and process implied free variables

  Consider a variable x. Suppose that we can find an equality such that the
  bound on the equality, combined with
  the bounds on the other variables involved in the equality, are such that
  even the worst case values of the other variables still imply bounds for x
  which are tighter than the variable's original bounds. Since x can never
  reach its upper or lower bounds, it is an implied free variable. By solving
  the equality for x and substituting for x in every other constraint
  entangled with x, we can make x into a column singleton. Now x is an implied
  free column singleton and both x and the equality can be removed.

  A similar transform for the case where the variable is a natural column
  singleton is handled by #implied_free_action. In the current presolve
  architecture, #implied_free_action is responsible for detecting implied free
  variables that are natural column singletons or can be reduced to column
  singletons. #implied_free_action calls subst_constraint_action to process
  variables that must be reduced to column singletons.
*/
class subst_constraint_action : public CoinPresolveAction {
private:
  subst_constraint_action();
  subst_constraint_action(const subst_constraint_action& rhs);
  subst_constraint_action& operator=(const subst_constraint_action& rhs);

  struct action {
    double *rlos;
    double *rups;

    double *coeffxs;
    int *rows;
    
    int *ninrowxs;
    int *rowcolsxs;
    double *rowelsxs;

    const double *costsx;
    int col;
    int rowy;

    int nincol;
  };

  const int nactions_;
  // actions_ is owned by the class and must be deleted at destruction
  const action *const actions_;

  subst_constraint_action(int nactions,
			  action *actions,
			  const CoinPresolveAction *next) :
    CoinPresolveAction(next),
    nactions_(nactions), actions_(actions) {}

 public:
  const char *name() const;

  static const CoinPresolveAction *presolve(CoinPresolveMatrix * prob,
					    const int *implied_free,
					    const int * which,
					    int numberFree,
					    const CoinPresolveAction *next,
					    int fill_level);
  static const CoinPresolveAction *presolveX(CoinPresolveMatrix * prob,
				  const CoinPresolveAction *next,
				  int fillLevel);

  void postsolve(CoinPostsolveMatrix *prob) const;

  virtual ~subst_constraint_action();
};





/*static*/ void implied_bounds(const double *els,
			   const double *clo, const double *cup,
			   const int *hcol,
			   CoinBigIndex krs, CoinBigIndex kre,
			   double *maxupp, double *maxdownp,
			   int jcol,
			   double rlo, double rup,
			   double *iclb, double *icub);
#endif
