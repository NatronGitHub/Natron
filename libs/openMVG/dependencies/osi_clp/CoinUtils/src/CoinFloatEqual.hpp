/* $Id: CoinFloatEqual.hpp 1416 2011-04-17 09:57:29Z stefan $ */
// Copyright (C) 2000, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef CoinFloatEqual_H
#define CoinFloatEqual_H

#include <algorithm>
#include <cmath>

#include "CoinFinite.hpp"

/*! \file CoinFloatEqual.hpp
    \brief Function objects for testing equality of real numbers.

  Two objects are provided; one tests for equality to an absolute tolerance,
  one to a scaled tolerance. The tests will handle IEEE floating point, but
  note that infinity == infinity. Mathematicians are rolling in their graves,
  but this matches the behaviour for the common practice of using
  <code>DBL_MAX</code> (<code>numeric_limits<double>::max()</code>, or similar
  large finite number) as infinity.

  <p>
  Example usage:
  @verbatim
    double d1 = 3.14159 ;
    double d2 = d1 ;
    double d3 = d1+.0001 ;

    CoinAbsFltEq eq1 ;
    CoinAbsFltEq eq2(.001) ;

    assert(  eq1(d1,d2) ) ;
    assert( !eq1(d1,d3) ) ;
    assert(  eq2(d1,d3) ) ;
  @endverbatim
  CoinRelFltEq follows the same pattern.  */

/*! \brief Equality to an absolute tolerance

  Operands are considered equal if their difference is within an epsilon ;
  the test does not consider the relative magnitude of the operands.
*/

class CoinAbsFltEq
{
  public:

  //! Compare function

  inline bool operator() (const double f1, const double f2) const

  { if (CoinIsnan(f1) || CoinIsnan(f2)) return false ;
    if (f1 == f2) return true ;
    return (fabs(f1-f2) < epsilon_) ; } 

  /*! \name Constructors and destructors */
  //@{

  /*! \brief Default constructor

    Default tolerance is 1.0e-10.
  */

  CoinAbsFltEq () : epsilon_(1.e-10) {} 

  //! Alternate constructor with epsilon as a parameter

  CoinAbsFltEq (const double epsilon) : epsilon_(epsilon) {} 

  //! Destructor

  virtual ~CoinAbsFltEq () {} 

  //! Copy constructor

  CoinAbsFltEq (const CoinAbsFltEq& src) : epsilon_(src.epsilon_) {} 

  //! Assignment

  CoinAbsFltEq& operator= (const CoinAbsFltEq& rhs)

  { if (this != &rhs) epsilon_ = rhs.epsilon_ ;
    return (*this) ; } 

  //@}

  private:  

  /*! \name Private member data */
  //@{

  //! Equality tolerance.

  double epsilon_ ;

  //@}

} ;



/*! \brief Equality to a scaled tolerance

  Operands are considered equal if their difference is within a scaled
  epsilon calculated as epsilon_*(1+CoinMax(|f1|,|f2|)).
*/

class CoinRelFltEq
{
  public:

  //! Compare function

  inline bool operator() (const double f1, const double f2) const

  { if (CoinIsnan(f1) || CoinIsnan(f2)) return false ;
    if (f1 == f2) return true ;
    if (!CoinFinite(f1) || !CoinFinite(f2)) return false ;

    double tol = (fabs(f1)>fabs(f2))?fabs(f1):fabs(f2) ;

    return (fabs(f1-f2) <= epsilon_*(1+tol)) ; }

  /*! \name Constructors and destructors */
  //@{

#ifndef COIN_FLOAT
  /*! Default constructor

    Default tolerance is 1.0e-10.
  */
  CoinRelFltEq () : epsilon_(1.e-10) {} 
#else
  /*! Default constructor

    Default tolerance is 1.0e-6.
  */
  CoinRelFltEq () : epsilon_(1.e-6) {} ; // as float
#endif

  //! Alternate constructor with epsilon as a parameter

  CoinRelFltEq (const double epsilon) : epsilon_(epsilon) {} 

  //! Destructor

  virtual ~CoinRelFltEq () {} 

  //! Copy constructor

  CoinRelFltEq (const CoinRelFltEq & src) : epsilon_(src.epsilon_) {} 

  //! Assignment

  CoinRelFltEq& operator= (const CoinRelFltEq& rhs)

  { if (this != &rhs) epsilon_ = rhs.epsilon_ ;
    return (*this) ; } 

  //@}

private: 

  /*! \name Private member data */
  //@{

  //! Base equality tolerance

  double epsilon_ ;

  //@}

} ;

#endif
