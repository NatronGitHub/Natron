/* $Id: CoinPresolveDoubleton.hpp 1498 2011-11-02 15:25:35Z mjs $ */
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef CoinPresolveDoubleton_H
#define CoinPresolveDoubleton_H

#define	DOUBLETON	5

/*! \class doubleton_action
    \brief Solve ax+by=c for y and substitute y out of the problem.

  This moves the bounds information for y onto x, making y free and allowing
  us to substitute it away.
  \verbatim
	   a x + b y = c
	   l1 <= x <= u1
	   l2 <= y <= u2	==>
	  
	   l2 <= (c - a x) / b <= u2
	   b/-a > 0 ==> (b l2 - c) / -a <= x <= (b u2 - c) / -a
	   b/-a < 0 ==> (b u2 - c) / -a <= x <= (b l2 - c) / -a
  \endverbatim
*/
class doubleton_action : public CoinPresolveAction {
 public:
  struct action {

    double clox;
    double cupx;
    double costx;
    
    double costy;

    double rlo;

    double coeffx;
    double coeffy;

    double *colel;

    int icolx;
    int icoly;
    int row;
    int ncolx;
    int ncoly;
  };

  const int nactions_;
  const action *const actions_;

 private:
  doubleton_action(int nactions,
		      const action *actions,
		      const CoinPresolveAction *next) :
    CoinPresolveAction(next),
    nactions_(nactions), actions_(actions)
{}

 public:
  const char *name() const { return ("doubleton_action"); }

  static const CoinPresolveAction *presolve(CoinPresolveMatrix *,
					 const CoinPresolveAction *next);
  
  void postsolve(CoinPostsolveMatrix *prob) const;

  virtual ~doubleton_action();
};
#endif


