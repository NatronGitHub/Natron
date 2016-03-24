/* $Id: CoinShallowPackedVector.cpp 1498 2011-11-02 15:25:35Z mjs $ */
// Copyright (C) 2000, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#if defined(_MSC_VER)
// Turn off compiler warning about long names
#  pragma warning(disable:4786)
#endif

#include "CoinHelperFunctions.hpp"
#include "CoinShallowPackedVector.hpp"

//#############################################################################

void
CoinShallowPackedVector::clear()
{
   clearBase();
   indices_ = NULL;
   elements_ = NULL;
   nElements_ = 0;
}

//#############################################################################

CoinShallowPackedVector&
CoinShallowPackedVector::operator=(const CoinPackedVectorBase & x)
{
   if (&x != this) {
      indices_ = x.getIndices();
      elements_ = x.getElements();
      nElements_ = x.getNumElements();
      CoinPackedVectorBase::clearBase();
      CoinPackedVectorBase::copyMaxMinIndex(x);
      try {
	 CoinPackedVectorBase::duplicateIndex();
      }
      catch (CoinError& e) {
	 throw CoinError("duplicate index", "operator= from base",
			"CoinShallowPackedVector");
      }
   }
   return *this;
}

//#############################################################################

CoinShallowPackedVector&
CoinShallowPackedVector::operator=(const CoinShallowPackedVector & x)
{
   if (&x != this) {
      indices_ = x.indices_;
      elements_ = x.elements_;
      nElements_ = x.nElements_;
      CoinPackedVectorBase::clearBase();
      CoinPackedVectorBase::copyMaxMinIndex(x);
      try {
	 CoinPackedVectorBase::duplicateIndex();
      }
      catch (CoinError& e) {
	 throw CoinError("duplicate index", "operator=",
			"CoinShallowPackedVector");
      }
   }
   return *this;
}

//#############################################################################

void
CoinShallowPackedVector::setVector(int size,
				  const int * inds, const double * elems,
				  bool testForDuplicateIndex)
{
   indices_ = inds;
   elements_ = elems;
   nElements_ = size;
   CoinPackedVectorBase::clearBase();
   try {
      CoinPackedVectorBase::setTestForDuplicateIndex(testForDuplicateIndex);
   }
   catch (CoinError& e) {
      throw CoinError("duplicate index", "setVector",
		     "CoinShallowPackedVector");
   }
}

//#############################################################################

//-------------------------------------------------------------------
// Default
//-------------------------------------------------------------------
CoinShallowPackedVector::CoinShallowPackedVector(bool testForDuplicateIndex) :
   CoinPackedVectorBase(),
   indices_(NULL),
   elements_(NULL),
   nElements_(0)
{
   try {
      CoinPackedVectorBase::setTestForDuplicateIndex(testForDuplicateIndex);
   }
   catch (CoinError& e) {
      throw CoinError("duplicate index", "default constructor",
		     "CoinShallowPackedVector");
   }
}
   
//-------------------------------------------------------------------
// Explicit
//-------------------------------------------------------------------
CoinShallowPackedVector::CoinShallowPackedVector(int size, 
					       const int * inds,
					       const double * elems,
					       bool testForDuplicateIndex) :
   CoinPackedVectorBase(),
   indices_(inds),
   elements_(elems),
   nElements_(size)
{
   try {
      CoinPackedVectorBase::setTestForDuplicateIndex(testForDuplicateIndex);
   }
   catch (CoinError& e) {
      throw CoinError("duplicate index", "explicit constructor",
		     "CoinShallowPackedVector");
   }
}

//-------------------------------------------------------------------
// Copy
//-------------------------------------------------------------------
CoinShallowPackedVector::CoinShallowPackedVector(const CoinPackedVectorBase& x) :
   CoinPackedVectorBase(),
   indices_(x.getIndices()),
   elements_(x.getElements()),
   nElements_(x.getNumElements())
{
   CoinPackedVectorBase::copyMaxMinIndex(x);
   try {
      CoinPackedVectorBase::setTestForDuplicateIndex(x.testForDuplicateIndex());
   }
   catch (CoinError& e) {
      throw CoinError("duplicate index", "copy constructor from base",
		     "CoinShallowPackedVector");
   }
}

//-------------------------------------------------------------------
// Copy
//-------------------------------------------------------------------
CoinShallowPackedVector::CoinShallowPackedVector(const
					       CoinShallowPackedVector& x) :
   CoinPackedVectorBase(),
   indices_(x.getIndices()),
   elements_(x.getElements()),
   nElements_(x.getNumElements())
{
   CoinPackedVectorBase::copyMaxMinIndex(x);
   try {
      CoinPackedVectorBase::setTestForDuplicateIndex(x.testForDuplicateIndex());
   }
   catch (CoinError& e) {
      throw CoinError("duplicate index", "copy constructor",
		     "CoinShallowPackedVector");
   }
}

//-------------------------------------------------------------------
// Print
//-------------------------------------------------------------------
void CoinShallowPackedVector::print()
{
for (int i=0; i < nElements_; i++)
  {
  std::cout << indices_[i] << ":" << elements_[i];
  if (i < nElements_-1)
    std::cout << ", ";
  }
 std::cout << std::endl;
}

