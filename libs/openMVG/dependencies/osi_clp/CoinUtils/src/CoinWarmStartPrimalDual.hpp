/* $Id: CoinWarmStartPrimalDual.hpp 1372 2011-01-03 23:31:00Z lou $ */
// Copyright (C) 2000, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef CoinWarmStartPrimalDual_H
#define CoinWarmStartPrimalDual_H

#include "CoinHelperFunctions.hpp"
#include "CoinWarmStart.hpp"
#include "CoinWarmStartVector.hpp"


//#############################################################################

/** WarmStart information that is only a dual vector */

class CoinWarmStartPrimalDual : public virtual CoinWarmStart {
public:
  /// return the size of the dual vector
  inline int dualSize() const { return dual_.size(); }
  /// return a pointer to the array of duals
  inline const double * dual() const { return dual_.values(); }

  /// return the size of the primal vector
  inline int primalSize() const { return primal_.size(); }
  /// return a pointer to the array of primals
  inline const double * primal() const { return primal_.values(); }

  /** Assign the primal/dual vectors to be the warmstart information. In this
      method the object assumes ownership of the pointers and upon return \c
      primal and \c dual will be a NULL pointers. If copying is desirable use
      the constructor.

      NOTE: \c primal and \c dual must have been allocated by new double[],
      because they will be freed by delete[] upon the desructtion of this
      object...
  */
  void assign(int primalSize, int dualSize, double*& primal, double *& dual) {
    primal_.assignVector(primalSize, primal);
    dual_.assignVector(dualSize, dual);
  }

  CoinWarmStartPrimalDual() : primal_(), dual_() {}

  CoinWarmStartPrimalDual(int primalSize, int dualSize,
			  const double* primal, const double * dual) :
    primal_(primalSize, primal), dual_(dualSize, dual) {}

  CoinWarmStartPrimalDual(const CoinWarmStartPrimalDual& rhs) :
    primal_(rhs.primal_), dual_(rhs.dual_) {}

  CoinWarmStartPrimalDual& operator=(const CoinWarmStartPrimalDual& rhs) {
    if (this != &rhs) {
      primal_ = rhs.primal_;
      dual_ = rhs.dual_;
    }
    return *this;
  }

  /*! \brief Clear the data

  Make it appear as if the warmstart was just created using the default
  constructor.
  */
  inline void clear() {
    primal_.clear();
    dual_.clear();
  }

  inline void swap(CoinWarmStartPrimalDual& rhs) {
    if (this != &rhs) {
      primal_.swap(rhs.primal_);
      dual_.swap(rhs.dual_);
    }
  }

  /** `Virtual constructor' */
  virtual CoinWarmStart *clone() const {
    return new CoinWarmStartPrimalDual(*this);
  }

  virtual ~CoinWarmStartPrimalDual() {}

  /*! \name PrimalDual warm start `diff' methods */
  //@{

  /*! \brief Generate a `diff' that can convert the warm start passed as a
    parameter to the warm start specified by \c this.

    The capabilities are limited: the basis passed as a parameter can be no
    larger than the basis pointed to by \c this.
  */

  virtual CoinWarmStartDiff*
  generateDiff (const CoinWarmStart *const oldCWS) const ;

  /*! \brief Apply \p diff to this warm start.

  Update this warm start by applying \p diff. It's assumed that the
  allocated capacity of the warm start is sufficiently large.
  */

  virtual void applyDiff (const CoinWarmStartDiff *const cwsdDiff) ;

  //@}

#if 0
protected:
  inline const CoinWarmStartVector<double>& primalWarmStartVector() const
  { return primal_; }
  inline const CoinWarmStartVector<double>& dualWarmStartVector() const
  { return dual_; }
#endif

private:
  ///@name Private data members
  //@{
  CoinWarmStartVector<double> primal_;
  CoinWarmStartVector<double> dual_;
  //@}
};

//#############################################################################

/*! \class CoinWarmStartPrimalDualDiff
  \brief A `diff' between two CoinWarmStartPrimalDual objects

  This class exists in order to hide from the world the details of calculating
  and representing a `diff' between two CoinWarmStartPrimalDual objects. For
  convenience, assignment, cloning, and deletion are visible to the world, and
  default and copy constructors are made available to derived classes.
  Knowledge of the rest of this structure, and of generating and applying
  diffs, is restricted to the friend functions
  CoinWarmStartPrimalDual::generateDiff() and
  CoinWarmStartPrimalDual::applyDiff().

  The actual data structure is a pair of vectors, #diffNdxs_ and #diffVals_.
    
*/

class CoinWarmStartPrimalDualDiff : public virtual CoinWarmStartDiff
{
  friend CoinWarmStartDiff*
  CoinWarmStartPrimalDual::generateDiff(const CoinWarmStart *const oldCWS) const;
  friend void
  CoinWarmStartPrimalDual::applyDiff(const CoinWarmStartDiff *const diff) ;

public:

  /*! \brief `Virtual constructor'. To be used when retaining polymorphism is
    important */
  virtual CoinWarmStartDiff *clone() const
  {
    return new CoinWarmStartPrimalDualDiff(*this);
  }

  /*! \brief Destructor */
  virtual ~CoinWarmStartPrimalDualDiff() {}

protected:

  /*! \brief Default constructor
  
  This is protected (rather than private) so that derived classes can
  see it when they make <i>their</i> default constructor protected or
  private.
  */
  CoinWarmStartPrimalDualDiff () : primalDiff_(), dualDiff_() {}

  /*! \brief Copy constructor
  
  For convenience when copying objects containing
  CoinWarmStartPrimalDualDiff objects. But consider whether you should be
  using #clone() to retain polymorphism.

  This is protected (rather than private) so that derived classes can
  see it when the make <i>their</i> copy constructor protected or
  private.
  */
  CoinWarmStartPrimalDualDiff (const CoinWarmStartPrimalDualDiff &rhs) :
    primalDiff_(rhs.primalDiff_), dualDiff_(rhs.dualDiff_) {}

  /*! \brief Clear the data

  Make it appear as if the diff was just created using the default
  constructor.
  */
  inline void clear() {
    primalDiff_.clear();
    dualDiff_.clear();
  }

  inline void swap(CoinWarmStartPrimalDualDiff& rhs) {
    if (this != &rhs) {
      primalDiff_.swap(rhs.primalDiff_);
      dualDiff_.swap(rhs.dualDiff_);
    }
  }

private:

  /*!
    \brief These two differences describe the differences in the primal and
    in the dual vector.
  */
  CoinWarmStartVectorDiff<double> primalDiff_;
  CoinWarmStartVectorDiff<double> dualDiff_;
} ;

#endif
