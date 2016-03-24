/* $Id: CoinPresolveFixed.hpp 1510 2011-12-08 23:56:01Z lou $ */
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef CoinPresolveFixed_H
#define CoinPresolveFixed_H
#define	FIXED_VARIABLE	1

/*! \class remove_fixed_action
    \brief Excise fixed variables from the model.

  Implements the action of virtually removing one or more fixed variables
  x_j from the model by substituting the value sol_j in each constraint.
  Specifically, for each constraint i where a_ij != 0, rlo_i and rup_i
  are adjusted by -a_ij*sol_j and a_ij is set to 0.

  There is an implicit assumption that the variable already has the correct
  value. If this isn't true, corrections to row activity may be incorrect.
  If you want to guard against this possibility, consider make_fixed_action.

  Actual removal of the empty column from the matrix is handled by
  drop_empty_cols_action. Correction of the objective function is done there.
*/
class remove_fixed_action : public CoinPresolveAction {
 public:
  /*! \brief Structure to hold information necessary to reintroduce a
	     column into the problem representation.
  */
  struct action {
    int col;		///< column index of variable
    int start;		///< start of coefficients in #colels_ and #colrows_
    double sol;		///< value of variable
  };
  /// Array of row indices for coefficients of excised columns
  int *colrows_;
  /// Array of coefficients of excised columns
  double *colels_;
  /// Number of entries in #actions_
  int nactions_;
  /// Vector specifying variable(s) affected by this object
  action *actions_;

 private:
  /*! \brief Constructor */
  remove_fixed_action(int nactions,
		      action *actions,
		      double * colels,
		      int * colrows,
		      const CoinPresolveAction *next);

 public:
  /// Returns string "remove_fixed_action".
  const char *name() const;

  /*! \brief Excise the specified columns.

    Remove the specified columns (\p nfcols, \p fcols) from the problem
    representation (\p prob), leaving the appropriate postsolve object
    linked as the head of the list of postsolve objects (currently headed
    by \p next).
  */
  static const remove_fixed_action *presolve(CoinPresolveMatrix *prob,
					 int *fcols,
					 int nfcols,
					 const CoinPresolveAction *next);

  void postsolve(CoinPostsolveMatrix *prob) const;

  /// Destructor
  virtual ~remove_fixed_action();
};


/*! \relates remove_fixed_action
    \brief Scan the problem for fixed columns and remove them.

  A front end to collect a list of columns with equal bounds and hand them to
  remove_fixed_action::presolve() for processing.
*/

const CoinPresolveAction *remove_fixed(CoinPresolveMatrix *prob,
				    const CoinPresolveAction *next);


/*! \class make_fixed_action
    \brief Fix a variable at a specified bound.

  Implements the action of fixing a variable by forcing both bounds to the same
  value and forcing the value of the variable to match.

  If the bounds are already equal, and the value of the variable is already
  correct, consider remove_fixed_action.
*/
class make_fixed_action : public CoinPresolveAction {

  /// Structure to preserve the bound overwritten when fixing a variable
  struct action {
    double bound;	///< Value of bound overwritten to fix variable.
    int col ;		///< column index of variable
  };

  /// Number of preserved bounds
  int nactions_;
  /// Vector of preserved bounds, one for each variable fixed in this object
  const action *actions_;

  /*! \brief True to fix at lower bound, false to fix at upper bound.

    Note that this applies to all variables fixed in this object.
  */
  const bool fix_to_lower_;

  /*! \brief The postsolve object with the information required to repopulate
  	     the fixed columns.
  */
  const remove_fixed_action *faction_;

  /*! \brief Constructor */
  make_fixed_action(int nactions, const action *actions, bool fix_to_lower,
		    const remove_fixed_action *faction,
		    const CoinPresolveAction *next)
    : CoinPresolveAction(next),
      nactions_(nactions), actions_(actions),
      fix_to_lower_(fix_to_lower),
      faction_(faction)
  {}

 public:
  /// Returns string "make_fixed_action".
  const char *name() const;

  /*! \brief Perform actions to fix variables and return postsolve object

    For each specified variable (\p nfcols, \p fcols), fix the variable to
    the specified bound (\p fix_to_lower) by setting the variable's bounds
    to be equal in \p prob. Create a postsolve object, link it at the head of
    the list of postsolve objects (\p next), and return the object.
  */
  static const CoinPresolveAction *presolve(CoinPresolveMatrix *prob,
					 int *fcols,
					 int nfcols,
					 bool fix_to_lower,
					 const CoinPresolveAction *next);

  /*! \brief Postsolve (unfix variables)

    Back out the variables fixed by the presolve side of this object.
  */
  void postsolve(CoinPostsolveMatrix *prob) const;

  /// Destructor
  virtual ~make_fixed_action() {
    deleteAction(actions_,action*); 
    delete faction_;
  }
};

/*! \relates make_fixed_action
    \brief Scan variables and fix any with equal bounds

  A front end to collect a list of columns with equal bounds and hand them to
  make_fixed_action::presolve() for processing.
*/

const CoinPresolveAction *make_fixed(CoinPresolveMatrix *prob,
				    const CoinPresolveAction *next) ;

/*! \brief Transfer costs from singleton variables
    \relates make_fixed_action

  Transfers costs from singleton variables in equalities onto the other
  variables. Will also transfer costs from one integer variable to other
  integer variables with zero cost if there's a net gain in integer variables
  with non-zero cost.

  The relation to make_fixed_action is tenuous, but this transform should be
  attempted before the initial round of variable fixing.
*/
void transferCosts(CoinPresolveMatrix * prob);
#endif
