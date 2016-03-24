/* $Id: CoinDenseVector.cpp 1373 2011-01-03 23:57:44Z lou $ */
// Copyright (C) 2003, International Business Machines
// Corporation and others.  All Rights Resized.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#if defined(_MSC_VER)
// Turn off compiler warning about long names
#  pragma warning(disable:4786)
#endif

#include <cassert>
#include "CoinDenseVector.hpp"
#include "CoinHelperFunctions.hpp"

//#############################################################################

template <typename T> void
CoinDenseVector<T>::clear()
{
   memset(elements_, 0, nElements_*sizeof(T));
}

//#############################################################################

template <typename T> CoinDenseVector<T> &
CoinDenseVector<T>::operator=(const CoinDenseVector<T> & rhs)
{
   if (this != &rhs) {
     setVector(rhs.getNumElements(), rhs.getElements());
   }
   return *this;
}

//#############################################################################

template <typename T> void
CoinDenseVector<T>::setVector(int size, const T * elems)
{
   resize(size);
   CoinMemcpyN( elems,size,elements_);
}

//#############################################################################

template <typename T> void
CoinDenseVector<T>::setConstant(int size, T value)
{
   resize(size);
   for(int i=0; i<size; i++)
     elements_[i] = value;
}

//#############################################################################

template <typename T> void
CoinDenseVector<T>::resize(int newsize, T value)
{
  if (newsize != nElements_){
    assert(newsize > 0);
    T *newarray = new T[newsize];
    int cpysize = CoinMin(newsize, nElements_);
    CoinMemcpyN( elements_,cpysize,newarray);
    delete[] elements_;
    elements_ = newarray;
    nElements_ = newsize;
    for(int i=cpysize; i<newsize; i++)
      elements_[i] = value;
  }
}

//#############################################################################

template <typename T> void
CoinDenseVector<T>::setElement(int index, T element)
{
  assert(index >= 0 && index < nElements_);
   elements_[index] = element;
}

//#############################################################################

template <typename T> void
CoinDenseVector<T>::append(const CoinDenseVector<T> & caboose)
{
   const int s = nElements_;
   const int cs = caboose.getNumElements();
   int newsize = s + cs;
   resize(newsize);
   const T * celem = caboose.getElements();
   CoinDisjointCopyN(celem, cs, elements_ + s);
}

//#############################################################################

template <typename T> void
CoinDenseVector<T>::operator+=(T value) 
{
  for(int i=0; i<nElements_; i++)
    elements_[i] += value;
}

//-----------------------------------------------------------------------------

template <typename T> void
CoinDenseVector<T>::operator-=(T value) 
{
  for(int i=0; i<nElements_; i++)
    elements_[i] -= value;
}

//-----------------------------------------------------------------------------

template <typename T> void
CoinDenseVector<T>::operator*=(T value) 
{
  for(int i=0; i<nElements_; i++)
    elements_[i] *= value;
}

//-----------------------------------------------------------------------------

template <typename T> void
CoinDenseVector<T>::operator/=(T value) 
{
  for(int i=0; i<nElements_; i++)
    elements_[i] /= value;
}

//#############################################################################

template <typename T> CoinDenseVector<T>::CoinDenseVector():
   nElements_(0),
   elements_(NULL)
{}
  
//#############################################################################

template <typename T> 
CoinDenseVector<T>::CoinDenseVector(int size, const T * elems):
   nElements_(0),
   elements_(NULL)
{
  gutsOfSetVector(size, elems);
}

//-----------------------------------------------------------------------------

template <typename T> CoinDenseVector<T>::CoinDenseVector(int size, T value):
   nElements_(0),
   elements_(NULL)
{
  gutsOfSetConstant(size, value);
}

//-----------------------------------------------------------------------------

template <typename T> 
CoinDenseVector<T>::CoinDenseVector(const CoinDenseVector<T> & rhs):
   nElements_(0),
   elements_(NULL)
{
     setVector(rhs.getNumElements(), rhs.getElements());
}

//-----------------------------------------------------------------------------

template <typename T> CoinDenseVector<T>::~CoinDenseVector ()
{
   delete [] elements_;
}

//#############################################################################

template <typename T> void
CoinDenseVector<T>::gutsOfSetVector(int size, const T * elems)
{
   if ( size != 0 ) {
      resize(size);
      nElements_ = size;
      CoinDisjointCopyN(elems, size, elements_);
   }
}

//-----------------------------------------------------------------------------

template <typename T> void
CoinDenseVector<T>::gutsOfSetConstant(int size, T value)
{
   if ( size != 0 ) {
      resize(size);
      nElements_ = size;
      CoinFillN(elements_, size, value);
   }
}

//#############################################################################
/** Access the i'th element of the dense vector.  */
template <typename T> T &
CoinDenseVector<T>::operator[](int index) const
{
  assert(index >= 0 && index < nElements_);
  T *where = elements_ + index;
  return *where;
}
//#############################################################################

// template class CoinDenseVector<int>; This works but causes warning messages
template class CoinDenseVector<float>;
template class CoinDenseVector<double>;
