/* $Id: CoinPresolveIsolated.hpp 1498 2011-11-02 15:25:35Z mjs $ */
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef CoinPresolveIsolated_H
#define CoinPresolveIsolated_H

#include "CoinPresolveMatrix.hpp"

class isolated_constraint_action : public CoinPresolveAction {
  isolated_constraint_action();
  isolated_constraint_action(const isolated_constraint_action& rhs);
  isolated_constraint_action& operator=(const isolated_constraint_action& rhs);

  double rlo_;
  double rup_;
  int row_;
  int ninrow_;
  // the arrays are owned by the class and must be deleted at destruction
  const int *rowcols_;
  const double *rowels_;
  const double *costs_;

  isolated_constraint_action(double rlo,
			     double rup,
			     int row,
			     int ninrow,
			     const int *rowcols,
			     const double *rowels,
			     const double *costs,
			     const CoinPresolveAction *next) :
    CoinPresolveAction(next),
    rlo_(rlo), rup_(rup), row_(row), ninrow_(ninrow),
    rowcols_(rowcols), rowels_(rowels), costs_(costs) {}
      
 public:
  const char *name() const;

  static const CoinPresolveAction *presolve(CoinPresolveMatrix * prob,
					 int row,
					 const CoinPresolveAction *next);

  void postsolve(CoinPostsolveMatrix *prob) const;

  virtual ~isolated_constraint_action();
};



#endif
