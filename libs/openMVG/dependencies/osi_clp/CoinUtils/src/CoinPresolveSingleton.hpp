/* $Id: CoinPresolveSingleton.hpp 1498 2011-11-02 15:25:35Z mjs $ */
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef CoinPresolveSingleton_H
#define CoinPresolveSingleton_H
#define	SLACK_DOUBLETON	2
#define	SLACK_SINGLETON	8

/*!
  \file
*/

//const int MAX_SLACK_DOUBLETONS	= 1000;

/*! \class slack_doubleton_action
    \brief Convert an explicit bound constraint to a column bound

  This transform looks for explicit bound constraints for a variable and
  transfers the bound to the appropriate column bound array.
  The constraint is removed from the constraint system.
*/
class slack_doubleton_action : public CoinPresolveAction {
  struct action {
    double clo;
    double cup;

    double rlo;
    double rup;

    double coeff;

    int col;
    int row;
  };

  const int nactions_;
  const action *const actions_;

  slack_doubleton_action(int nactions,
			 const action *actions,
			 const CoinPresolveAction *next) :
    CoinPresolveAction(next),
    nactions_(nactions),
    actions_(actions)
{}

 public:
  const char *name() const { return ("slack_doubleton_action"); }

  /*! \brief Convert explicit bound constraints to column bounds.
  
    Not now There is a hard limit (#MAX_SLACK_DOUBLETONS) on the number of
    constraints processed in a given call. \p notFinished is set to true
    if candidates remain.
  */
  static const CoinPresolveAction *presolve(CoinPresolveMatrix *prob,
					   const CoinPresolveAction *next,
					bool &notFinished);

  void postsolve(CoinPostsolveMatrix *prob) const;


  virtual ~slack_doubleton_action() { deleteAction(actions_,action*); }
};
/*! \class slack_singleton_action
    \brief For variables with one entry

    If we have a variable with one entry and no cost then we can
    transform the row from E to G etc.
    If there is a row objective region then we may be able to do
    this even with a cost.
*/
class slack_singleton_action : public CoinPresolveAction {
  struct action {
    double clo;
    double cup;

    double rlo;
    double rup;

    double coeff;

    int col;
    int row;
  };

  const int nactions_;
  const action *const actions_;

  slack_singleton_action(int nactions,
			 const action *actions,
			 const CoinPresolveAction *next) :
    CoinPresolveAction(next),
    nactions_(nactions),
    actions_(actions)
{}

 public:
  const char *name() const { return ("slack_singleton_action"); }

  static const CoinPresolveAction *presolve(CoinPresolveMatrix *prob,
                                            const CoinPresolveAction *next,
                                            double * rowObjective);

  void postsolve(CoinPostsolveMatrix *prob) const;


  virtual ~slack_singleton_action() { deleteAction(actions_,action*); }
};
#endif
