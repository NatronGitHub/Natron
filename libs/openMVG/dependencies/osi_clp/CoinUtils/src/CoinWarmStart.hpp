/* $Id: CoinWarmStart.hpp 1372 2011-01-03 23:31:00Z lou $ */
// Copyright (C) 2000, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef CoinWarmStart_H
#define CoinWarmStart_H

//#############################################################################

class CoinWarmStartDiff;

/** Abstract base class for warm start information.

    Really nothing can be generalized for warm start information --- all we
    know is that it exists. Hence the abstract base class contains only a
    virtual destructor and a virtual clone function (a virtual constructor),
    so that derived classes can provide these functions.
*/

class CoinWarmStart {
public:

  /// Abstract destructor
  virtual ~CoinWarmStart() {}

  /// `Virtual constructor'
  virtual CoinWarmStart *clone() const = 0 ;
   
  virtual CoinWarmStartDiff*
  generateDiff (const CoinWarmStart *const ) const { return 0; }
   
   
  virtual void
  applyDiff (const CoinWarmStartDiff *const ) {}

};


/*! \class CoinWarmStartDiff
    \brief Abstract base class for warm start `diff' objects

  For those types of warm start objects where the notion of a `diff' makes
  sense, this virtual base class is provided. As with CoinWarmStart, its sole
  reason for existence is to make it possible to write solver-independent code.
*/

class CoinWarmStartDiff {
public:

  /// Abstract destructor
  virtual ~CoinWarmStartDiff() {}

  /// `Virtual constructor'
  virtual CoinWarmStartDiff *clone() const = 0 ;
};

#endif
