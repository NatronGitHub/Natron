/* $Id: CoinPresolveTripleton.hpp 1498 2011-11-02 15:25:35Z mjs $ */
// Copyright (C) 2003, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef CoinPresolveTripleton_H
#define CoinPresolveTripleton_H
#define TRIPLETON 11
/** We are only going to do this if it does not increase number of elements?.
    It could be generalized to more than three but it seems unlikely it would
    help.

    As it is adapted from doubleton icoly is one dropped.
 */
class tripleton_action : public CoinPresolveAction {
 public:
  struct action {
    int icolx;
    int icolz;
    int row;

    int icoly;
    double cloy;
    double cupy;
    double costy;
    double clox;
    double cupx;
    double costx;

    double rlo;
    double rup;

    double coeffx;
    double coeffy;
    double coeffz;

    double *colel;

    int ncolx;
    int ncoly;
  };

  const int nactions_;
  const action *const actions_;

 private:
  tripleton_action(int nactions,
		      const action *actions,
		      const CoinPresolveAction *next) :
    CoinPresolveAction(next),
    nactions_(nactions), actions_(actions)
{}

 public:
  const char *name() const { return ("tripleton_action"); }

  static const CoinPresolveAction *presolve(CoinPresolveMatrix *,
					 const CoinPresolveAction *next);
  
  void postsolve(CoinPostsolveMatrix *prob) const;

  virtual ~tripleton_action();
};
#endif


