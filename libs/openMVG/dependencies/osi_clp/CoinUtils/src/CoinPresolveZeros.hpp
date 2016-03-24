/* $Id: CoinPresolveZeros.hpp 1498 2011-11-02 15:25:35Z mjs $ */
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef CoinPresolveZeros_H
#define CoinPresolveZeros_H

/*! \file

  Drop/reintroduce explicit zeros.
*/

#define	DROP_ZERO	8

/*! \brief Tracking information for an explicit zero coefficient

  \todo Why isn't this a nested class in drop_zero_coefficients_action?
	That would match the structure of other presolve classes.
*/

struct dropped_zero {
  int row;
  int col;
};

/*! \brief Removal of explicit zeros

  The presolve action for this class removes explicit zeros from the constraint
  matrix. The postsolve action puts them back.
*/
class drop_zero_coefficients_action : public CoinPresolveAction {

  const int nzeros_;
  const dropped_zero *const zeros_;

  drop_zero_coefficients_action(int nzeros,
				const dropped_zero *zeros,
				const CoinPresolveAction *next) :
    CoinPresolveAction(next),
    nzeros_(nzeros), zeros_(zeros)
{}

 public:
  const char *name() const { return ("drop_zero_coefficients_action"); }

  static const CoinPresolveAction *presolve(CoinPresolveMatrix *prob,
					 int *checkcols,
					 int ncheckcols,
					 const CoinPresolveAction *next);

  void postsolve(CoinPostsolveMatrix *prob) const;

  virtual ~drop_zero_coefficients_action() { deleteAction(zeros_,dropped_zero*); }
};

const CoinPresolveAction *drop_zero_coefficients(CoinPresolveMatrix *prob,
					      const CoinPresolveAction *next);

#endif
