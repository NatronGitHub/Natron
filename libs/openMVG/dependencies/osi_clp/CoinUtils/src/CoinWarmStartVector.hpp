/* $Id: CoinWarmStartVector.hpp 1498 2011-11-02 15:25:35Z mjs $ */
// Copyright (C) 2000, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef CoinWarmStartVector_H
#define CoinWarmStartVector_H

#if defined(_MSC_VER)
// Turn off compiler warning about long names
#  pragma warning(disable:4786)
#endif

#include <cassert>
#include <cmath>

#include "CoinHelperFunctions.hpp"
#include "CoinWarmStart.hpp"


//#############################################################################

/** WarmStart information that is only a vector */

template <typename T>
class CoinWarmStartVector : public virtual CoinWarmStart
{
protected:
  inline void gutsOfDestructor() {
    delete[] values_;
  }
  inline void gutsOfCopy(const CoinWarmStartVector<T>& rhs) {
    size_  = rhs.size_;
    values_ = new T[size_];
    CoinDisjointCopyN(rhs.values_, size_, values_);
  }

public:
  /// return the size of the vector
  int size() const { return size_; }
  /// return a pointer to the array of vectors
  const T* values() const { return values_; }

  /** Assign the vector to be the warmstart information. In this method
      the object assumes ownership of the pointer and upon return #vector will
      be a NULL pointer. If copying is desirable use the constructor. */
  void assignVector(int size, T*& vec) {
    size_ = size;
    delete[] values_;
    values_ = vec;
    vec = NULL;
  }

  CoinWarmStartVector() : size_(0), values_(NULL) {}

  CoinWarmStartVector(int size, const T* vec) :
    size_(size), values_(new T[size]) {
    CoinDisjointCopyN(vec, size, values_);
  }

  CoinWarmStartVector(const CoinWarmStartVector& rhs) {
    gutsOfCopy(rhs);
  }

  CoinWarmStartVector& operator=(const CoinWarmStartVector& rhs) {
    if (this != &rhs) {
      gutsOfDestructor();
      gutsOfCopy(rhs);
    }
    return *this;
  }

  inline void swap(CoinWarmStartVector& rhs) {
    if (this != &rhs) {
      std::swap(size_, rhs.size_);
      std::swap(values_, rhs.values_);
    }
  }

  /** `Virtual constructor' */
  virtual CoinWarmStart *clone() const {
    return new CoinWarmStartVector(*this);
  }
     
  virtual ~CoinWarmStartVector() {
    gutsOfDestructor();
  }

  /*! \brief Clear the data

  Make it appear as if the warmstart was just created using the default
  constructor.
  */
  inline void clear() {
    size_ = 0;
    delete[] values_;
    values_ = NULL;
  }

  /*! \name Vector warm start `diff' methods */
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

private:
  ///@name Private data members
  //@{
  /// the size of the vector
  int size_;
  /// the vector itself
  T* values_;
  //@}
};

//=============================================================================

/*! \class CoinWarmStartVectorDiff
  \brief A `diff' between two CoinWarmStartVector objects

  This class exists in order to hide from the world the details of calculating
  and representing a `diff' between two CoinWarmStartVector objects. For
  convenience, assignment, cloning, and deletion are visible to the world, and
  default and copy constructors are made available to derived classes.
  Knowledge of the rest of this structure, and of generating and applying
  diffs, is restricted to the friend functions
  CoinWarmStartVector::generateDiff() and CoinWarmStartVector::applyDiff().

  The actual data structure is a pair of vectors, #diffNdxs_ and #diffVals_.
    
*/

template <typename T>
class CoinWarmStartVectorDiff : public virtual CoinWarmStartDiff
{
  friend CoinWarmStartDiff*
  CoinWarmStartVector<T>::generateDiff(const CoinWarmStart *const oldCWS) const;
  friend void
  CoinWarmStartVector<T>::applyDiff(const CoinWarmStartDiff *const diff) ;

public:

  /*! \brief `Virtual constructor' */
  virtual CoinWarmStartDiff * clone() const {
    return new CoinWarmStartVectorDiff(*this) ;
  }

  /*! \brief Assignment */
  virtual CoinWarmStartVectorDiff &
  operator= (const CoinWarmStartVectorDiff<T>& rhs) ;

  /*! \brief Destructor */
  virtual ~CoinWarmStartVectorDiff() {
    delete[] diffNdxs_ ;
    delete[] diffVals_ ;
  }

  inline void swap(CoinWarmStartVectorDiff& rhs) {
    if (this != &rhs) {
      std::swap(sze_, rhs.sze_);
      std::swap(diffNdxs_, rhs.diffNdxs_);
      std::swap(diffVals_, rhs.diffVals_);
    }
  }

  /*! \brief Default constructor
   */
  CoinWarmStartVectorDiff () : sze_(0), diffNdxs_(0), diffVals_(NULL) {} 

  /*! \brief Copy constructor
  
  For convenience when copying objects containing CoinWarmStartVectorDiff
  objects. But consider whether you should be using #clone() to retain
  polymorphism.
  */
  CoinWarmStartVectorDiff(const CoinWarmStartVectorDiff<T>& rhs) ;

  /*! \brief Standard constructor */
  CoinWarmStartVectorDiff(int sze, const unsigned int* const diffNdxs,
			  const T* const diffVals) ;
  
  /*! \brief Clear the data

  Make it appear as if the diff was just created using the default
  constructor.
  */
  inline void clear() {
    sze_ = 0;
    delete[] diffNdxs_;  diffNdxs_ = NULL;
    delete[] diffVals_;  diffVals_ = NULL;
  }

private:

  /*!
    \brief Number of entries (and allocated capacity), in units of \c T.
  */
  int sze_ ;

  /*! \brief Array of diff indices */

  unsigned int* diffNdxs_ ;

  /*! \brief Array of diff values */

  T* diffVals_ ;
};

//##############################################################################

template <typename T, typename U>
class CoinWarmStartVectorPair : public virtual CoinWarmStart 
{
private:
  CoinWarmStartVector<T> t_;
  CoinWarmStartVector<U> u_;

public:
  inline int size0() const { return t_.size(); }
  inline int size1() const { return u_.size(); }
  inline const T* values0() const { return t_.values(); }
  inline const U* values1() const { return u_.values(); }

  inline void assignVector0(int size, T*& vec) { t_.assignVector(size, vec); }
  inline void assignVector1(int size, U*& vec) { u_.assignVector(size, vec); }

  CoinWarmStartVectorPair() {}
  CoinWarmStartVectorPair(int s0, const T* v0, int s1, const U* v1) :
    t_(s0, v0), u_(s1, v1) {}

  CoinWarmStartVectorPair(const CoinWarmStartVectorPair<T,U>& rhs) :
    t_(rhs.t_), u_(rhs.u_) {}
  CoinWarmStartVectorPair& operator=(const CoinWarmStartVectorPair<T,U>& rhs) {
    if (this != &rhs) {
      t_ = rhs.t_;
      u_ = rhs.u_;
    }	
    return *this;
  }

  inline void swap(CoinWarmStartVectorPair<T,U>& rhs) {
    t_.swap(rhs.t_);
    u_.swap(rhs.u_);
  }

  virtual CoinWarmStart *clone() const {
    return new CoinWarmStartVectorPair(*this);
  }

  virtual ~CoinWarmStartVectorPair() {}

  inline void clear() {
    t_.clear();
    u_.clear();
  }

  virtual CoinWarmStartDiff*
  generateDiff (const CoinWarmStart *const oldCWS) const ;

  virtual void applyDiff (const CoinWarmStartDiff *const cwsdDiff) ;
};

//=============================================================================

template <typename T, typename U>
class CoinWarmStartVectorPairDiff : public virtual CoinWarmStartDiff
{
  friend CoinWarmStartDiff*
  CoinWarmStartVectorPair<T,U>::generateDiff(const CoinWarmStart *const oldCWS) const;
  friend void
  CoinWarmStartVectorPair<T,U>::applyDiff(const CoinWarmStartDiff *const diff) ;

private:
  CoinWarmStartVectorDiff<T> tdiff_;
  CoinWarmStartVectorDiff<U> udiff_;

public:
  CoinWarmStartVectorPairDiff() {}
  CoinWarmStartVectorPairDiff(const CoinWarmStartVectorPairDiff<T,U>& rhs) :
    tdiff_(rhs.tdiff_), udiff_(rhs.udiff_) {}
  virtual ~CoinWarmStartVectorPairDiff() {}

  virtual CoinWarmStartVectorPairDiff&
  operator=(const CoinWarmStartVectorPairDiff<T,U>& rhs) {
	  if (this != &rhs) {
		  tdiff_ = rhs.tdiff_;
		  udiff_ = rhs.udiff_;
	  }
    return *this;
  }

  virtual CoinWarmStartDiff * clone() const {
    return new CoinWarmStartVectorPairDiff(*this) ;
  }

  inline void swap(CoinWarmStartVectorPairDiff<T,U>& rhs) {
    tdiff_.swap(rhs.tdiff_);
    udiff_.swap(rhs.udiff_);
  }

  inline void clear() {
    tdiff_.clear();
    udiff_.clear();
  }
};

//##############################################################################
//#############################################################################

/*
  Generate a `diff' that can convert the warm start passed as a parameter to
  the warm start specified by this.

  The capabilities are limited: the basis passed as a parameter can be no
  larger than the basis pointed to by this.
*/

template <typename T> CoinWarmStartDiff*
CoinWarmStartVector<T>::generateDiff(const CoinWarmStart *const oldCWS) const
{ 
/*
  Make sure the parameter is CoinWarmStartVector or derived class.
*/
  const CoinWarmStartVector<T>* oldVector =
    dynamic_cast<const CoinWarmStartVector<T>*>(oldCWS);
  if (!oldVector)
    { throw CoinError("Old warm start not derived from CoinWarmStartVector.",
		      "generateDiff","CoinWarmStartVector") ; }
  const CoinWarmStartVector<T>* newVector = this ;
  /*
    Make sure newVector is equal or bigger than oldVector. Calculate the worst
    case number of diffs and allocate vectors to hold them.
  */
  const int oldCnt = oldVector->size() ;
  const int newCnt = newVector->size() ;

  assert(newCnt >= oldCnt) ;

  unsigned int *diffNdx = new unsigned int [newCnt]; 
  T* diffVal = new T[newCnt]; 
  /*
    Scan the vector vectors.  For the portion of the vectors which overlap,
    create diffs. Then add any additional entries from newVector.
  */
  const T*oldVal = oldVector->values() ;
  const T*newVal = newVector->values() ;
  int numberChanged = 0 ;
  int i ;
  for (i = 0 ; i < oldCnt ; i++) {
    if (oldVal[i] != newVal[i]) {
      diffNdx[numberChanged] = i ;
      diffVal[numberChanged++] = newVal[i] ;
    }
  }
  for ( ; i < newCnt ; i++) {
    diffNdx[numberChanged] = i ;
    diffVal[numberChanged++] = newVal[i] ;
  }
  /*
    Create the object of our desire.
  */
  CoinWarmStartVectorDiff<T> *diff =
    new CoinWarmStartVectorDiff<T>(numberChanged,diffNdx,diffVal) ;
  /*
    Clean up and return.
  */
  delete[] diffNdx ;
  delete[] diffVal ;

  return diff;
  //  return (dynamic_cast<CoinWarmStartDiff<T>*>(diff)) ;
}


/*
  Apply diff to this warm start.
  
  Update this warm start by applying diff. It's assumed that the
  allocated capacity of the warm start is sufficiently large.
*/

template <typename T> void
CoinWarmStartVector<T>::applyDiff (const CoinWarmStartDiff *const cwsdDiff)
{
  /*
    Make sure we have a CoinWarmStartVectorDiff
  */
  const CoinWarmStartVectorDiff<T>* diff =
    dynamic_cast<const CoinWarmStartVectorDiff<T>*>(cwsdDiff) ;
  if (!diff) {
    throw CoinError("Diff not derived from CoinWarmStartVectorDiff.",
		    "applyDiff","CoinWarmStartVector") ;
  }
  /*
    Application is by straighforward replacement of words in the vector vector.
  */
  const int numberChanges = diff->sze_ ;
  const unsigned int *diffNdxs = diff->diffNdxs_ ;
  const T* diffVals = diff->diffVals_ ;
  T* vals = this->values_ ;

  for (int i = 0 ; i < numberChanges ; i++) {
    unsigned int diffNdx = diffNdxs[i] ;
    T diffVal = diffVals[i] ;
    vals[diffNdx] = diffVal ;
  }
}

//#############################################################################


// Assignment

template <typename T> CoinWarmStartVectorDiff<T>&
CoinWarmStartVectorDiff<T>::operator=(const CoinWarmStartVectorDiff<T> &rhs)
{
  if (this != &rhs) {
    if (sze_ > 0) {
      delete[] diffNdxs_ ;
      delete[] diffVals_ ;
    }
    sze_ = rhs.sze_ ;
    if (sze_ > 0) {
      diffNdxs_ = new unsigned int[sze_] ;
      memcpy(diffNdxs_,rhs.diffNdxs_,sze_*sizeof(unsigned int)) ;
      diffVals_ = new T[sze_] ;
      memcpy(diffVals_,rhs.diffVals_,sze_*sizeof(T)) ;
    } else {
      diffNdxs_ = 0 ;
      diffVals_ = 0 ;
    }
  }

  return (*this) ;
}


// Copy constructor

template <typename T>
CoinWarmStartVectorDiff<T>::CoinWarmStartVectorDiff(const CoinWarmStartVectorDiff<T> &rhs)
  : sze_(rhs.sze_),
    diffNdxs_(0),
    diffVals_(0)
{
  if (sze_ > 0) {
    diffNdxs_ = new unsigned int[sze_] ;
    memcpy(diffNdxs_,rhs.diffNdxs_,sze_*sizeof(unsigned int)) ;
    diffVals_ = new T[sze_] ;
    memcpy(diffVals_,rhs.diffVals_,sze_*sizeof(T)) ;
  }
}

/// Standard constructor

template <typename T>
CoinWarmStartVectorDiff<T>::CoinWarmStartVectorDiff
(int sze, const unsigned int *const diffNdxs, const T *const diffVals)
  : sze_(sze),
    diffNdxs_(0),
    diffVals_(0)
{
  if (sze > 0) {
    diffNdxs_ = new unsigned int[sze] ;
    memcpy(diffNdxs_,diffNdxs,sze*sizeof(unsigned int)) ;
    diffVals_ = new T[sze] ;
    memcpy(diffVals_,diffVals,sze*sizeof(T)) ;
  }
}

#endif
