/* $Id: CoinPresolveImpliedFree.hpp 1562 2012-11-24 00:36:15Z lou $ */
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef CoinPresolveImpliedFree_H
#define CoinPresolveImpliedFree_H

/*!
  \file
*/

#define	IMPLIED_FREE	9

/*! \class implied_free_action
    \brief Detect and process implied free variables

  Consider a singleton variable x (<i>i.e.</i>, a variable involved in only
  one constraint).  Suppose that the bounds on that constraint, combined with
  the bounds on the other variables involved in the constraint, are such that
  even the worst case values of the other variables still imply bounds for x
  which are tighter than the variable's original bounds. Since x can never
  reach its upper or lower bounds, it is an implied free variable. Both x and
  the constraint can be deleted from the problem.

  A similar transform for the case where the variable is not a natural column
  singleton is handled by #subst_constraint_action.
*/
class implied_free_action : public CoinPresolveAction {
  struct action {
    int row, col;
    double clo, cup;
    double rlo, rup;
    const double *rowels;
    const double *costs;
    int ninrow;
  };

  const int nactions_;
  const action *const actions_;

  implied_free_action(int nactions,
		      const action *actions,
		      const CoinPresolveAction *next) :
    CoinPresolveAction(next),
    nactions_(nactions), actions_(actions) {}

 public:
  const char *name() const;

  static const CoinPresolveAction *presolve(CoinPresolveMatrix * prob,
					 const CoinPresolveAction *next,
					int & fillLevel);

  void postsolve(CoinPostsolveMatrix *prob) const;

  virtual ~implied_free_action();
};

#endif
