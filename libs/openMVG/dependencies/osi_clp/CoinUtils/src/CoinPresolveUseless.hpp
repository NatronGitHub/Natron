/* $Id: CoinPresolveUseless.hpp 1566 2012-11-29 19:33:56Z lou $ */
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef CoinPresolveUseless_H
#define CoinPresolveUseless_H
#define	USELESS		20

class useless_constraint_action : public CoinPresolveAction {
  struct action {
    double rlo;
    double rup;
    const int *rowcols;
    const double *rowels;
    int row;
    int ninrow;
  };

  const int nactions_;
  const action *const actions_;

  useless_constraint_action(int nactions,
                            const action *actions,
                            const CoinPresolveAction *next);

 public:
  const char *name() const;

  // These rows are asserted to be useless,
  // that is, given a solution the row activity
  // must be in range.
  static const CoinPresolveAction *presolve(CoinPresolveMatrix * prob,
					 const int *useless_rows,
					 int nuseless_rows,
					 const CoinPresolveAction *next);

  void postsolve(CoinPostsolveMatrix *prob) const;

  virtual ~useless_constraint_action();

};

/*! \relates useless_constraint_action
    \brief Scan constraints looking for useless constraints

  A front end to identify useless constraints and hand them to
  useless_constraint_action::presolve() for processing.

  In a bit more detail, the routine implements a greedy algorithm that
  identifies a set of necessary constraints. A constraint is necessary if it
  implies a tighter bound on a variable than the original column bound. These
  tighter column bounds are then used to calculate row activity and identify
  constraints that are useless given the presence of the necessary
  constraints. 
*/

const CoinPresolveAction *testRedundant(CoinPresolveMatrix *prob,
					const CoinPresolveAction *next) ;



#endif
