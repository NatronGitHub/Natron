/* $Id: CoinPresolveEmpty.hpp 1561 2012-11-24 00:32:16Z lou $ */
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef CoinPresolveEmpty_H
#define CoinPresolveEmpty_H

/*! \file

  Drop/reinsert empty rows/columns.
*/

const int DROP_ROW = 3;
const int DROP_COL = 4;

/*! \class drop_empty_cols_action
    \brief Physically removes empty columns in presolve, and reinserts
	   empty columns in postsolve.

  Physical removal of rows and columns should be the last activities
  performed during presolve. Do them exactly once. The row-major matrix
  is <b>not</b> maintained by this transform.

  To physically drop the columns, CoinPrePostsolveMatrix::mcstrt_ and
  CoinPrePostsolveMatrix::hincol_ are compressed, along with column bounds,
  objective, and (if present) the column portions of the solution. This
  renumbers the columns. drop_empty_cols_action::presolve will reconstruct
  CoinPresolveMatrix::clink_.

  \todo Confirm correct behaviour with solution in presolve.
*/

class drop_empty_cols_action : public CoinPresolveAction {
private:
  const int nactions_;

  struct action {
    double clo;
    double cup;
    double cost;
    double sol;
    int jcol;
  };
  const action *const actions_;

  drop_empty_cols_action(int nactions,
			 const action *const actions,
			 const CoinPresolveAction *next) :
    CoinPresolveAction(next),
    nactions_(nactions), 
    actions_(actions)
  {}

 public:
  const char *name() const { return ("drop_empty_cols_action"); }

  static const CoinPresolveAction *presolve(CoinPresolveMatrix *,
					 const int *ecols,
					 int necols,
					 const CoinPresolveAction*);

  static const CoinPresolveAction *presolve(CoinPresolveMatrix *prob,
					 const CoinPresolveAction *next);

  void postsolve(CoinPostsolveMatrix *prob) const;

  virtual ~drop_empty_cols_action() { deleteAction(actions_,action*); }
};


/*! \class drop_empty_rows_action
    \brief Physically removes empty rows in presolve, and reinserts
	   empty rows in postsolve.

  Physical removal of rows and columns should be the last activities
  performed during presolve. Do them exactly once. The row-major matrix
  is <b>not</b> maintained by this transform.

  To physically drop the rows, the rows are renumbered, excluding empty
  rows. This involves rewriting CoinPrePostsolveMatrix::hrow_ and compressing
  the row bounds and (if present) the row portions of the solution.

  \todo Confirm behaviour when a solution is present in presolve.
*/
class drop_empty_rows_action : public CoinPresolveAction {
private:
  struct action {
    double rlo;
    double rup;
    int row;
    int fill_row;	// which row was moved into position row to fill it
  };

  const int nactions_;
  const action *const actions_;

  drop_empty_rows_action(int nactions,
			 const action *actions,
			 const CoinPresolveAction *next) :
    CoinPresolveAction(next),
    nactions_(nactions), actions_(actions)
{}

 public:
  const char *name() const { return ("drop_empty_rows_action"); }

  static const CoinPresolveAction *presolve(CoinPresolveMatrix *prob,
					    const CoinPresolveAction *next);

  void postsolve(CoinPostsolveMatrix *prob) const;

  virtual ~drop_empty_rows_action() { deleteAction(actions_,action*); }
};
#endif

