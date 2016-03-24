/* $Id: CoinWarmStartDual.hpp 1372 2011-01-03 23:31:00Z lou $ */
// Copyright (C) 2000, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef CoinWarmStartDual_H
#define CoinWarmStartDual_H

#include "CoinHelperFunctions.hpp"
#include "CoinWarmStart.hpp"
#include "CoinWarmStartVector.hpp"


//#############################################################################

/** WarmStart information that is only a dual vector */

class CoinWarmStartDual : public virtual CoinWarmStart {
public:
   /// return the size of the dual vector
   inline int size() const { return dual_.size(); }
   /// return a pointer to the array of duals
   inline const double * dual() const { return dual_.values(); }

   /** Assign the dual vector to be the warmstart information. In this method
       the object assumes ownership of the pointer and upon return "dual" will
       be a NULL pointer. If copying is desirable use the constructor. */
   inline void assignDual(int size, double *& dual)
    { dual_.assignVector(size, dual); }

   CoinWarmStartDual() {}

   CoinWarmStartDual(int size, const double * dual) : dual_(size, dual) {}

   CoinWarmStartDual(const CoinWarmStartDual& rhs) : dual_(rhs.dual_) {}

   CoinWarmStartDual& operator=(const CoinWarmStartDual& rhs) {
     if (this != &rhs) {
       dual_ = rhs.dual_;
     }
     return *this;
   }

   /** `Virtual constructor' */
   virtual CoinWarmStart *clone() const {
      return new CoinWarmStartDual(*this);
   }

   virtual ~CoinWarmStartDual() {}

/*! \name Dual warm start `diff' methods */
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

#if 0
protected:
  inline const CoinWarmStartVector<double>& warmStartVector() const { return dual_; }
#endif

//@}

private:
   ///@name Private data members
  CoinWarmStartVector<double> dual_;
};

//#############################################################################

/*! \class CoinWarmStartDualDiff
    \brief A `diff' between two CoinWarmStartDual objects

  This class exists in order to hide from the world the details of
  calculating and representing a `diff' between two CoinWarmStartDual
  objects. For convenience, assignment, cloning, and deletion are visible to
  the world, and default and copy constructors are made available to derived
  classes.  Knowledge of the rest of this structure, and of generating and
  applying diffs, is restricted to the friend functions
  CoinWarmStartDual::generateDiff() and CoinWarmStartDual::applyDiff().

  The actual data structure is a pair of vectors, #diffNdxs_ and #diffVals_.
    
*/

class CoinWarmStartDualDiff : public virtual CoinWarmStartDiff
{ public:

  /*! \brief `Virtual constructor' */
  virtual CoinWarmStartDiff *clone() const
  {
      return new CoinWarmStartDualDiff(*this) ;
  }

  /*! \brief Assignment */
  virtual CoinWarmStartDualDiff &operator= (const CoinWarmStartDualDiff &rhs)
  {
      if (this != &rhs) {
	  diff_ = rhs.diff_;
      }
      return *this;
  }

  /*! \brief Destructor */
  virtual ~CoinWarmStartDualDiff() {}

  protected:

  /*! \brief Default constructor
  
    This is protected (rather than private) so that derived classes can
    see it when they make <i>their</i> default constructor protected or
    private.
  */
  CoinWarmStartDualDiff () : diff_() {}

  /*! \brief Copy constructor
  
    For convenience when copying objects containing CoinWarmStartDualDiff
    objects. But consider whether you should be using #clone() to retain
    polymorphism.

    This is protected (rather than private) so that derived classes can
    see it when the make <i>their</i> copy constructor protected or
    private.
  */
  CoinWarmStartDualDiff (const CoinWarmStartDualDiff &rhs) :
      diff_(rhs.diff_) {}

  private:

  friend CoinWarmStartDiff*
    CoinWarmStartDual::generateDiff(const CoinWarmStart *const oldCWS) const ;
  friend void
    CoinWarmStartDual::applyDiff(const CoinWarmStartDiff *const diff) ;

  /*! \brief Standard constructor */
  CoinWarmStartDualDiff (int sze, const unsigned int *const diffNdxs,
			 const double *const diffVals) :
      diff_(sze, diffNdxs, diffVals) {}

  /*!
      \brief The difference in the dual vector is simply the difference in a
      vector.
  */
  CoinWarmStartVectorDiff<double> diff_;
};


#endif

