/* $Id: CoinPackedVector.cpp 1509 2011-12-05 13:50:48Z forrest $ */
// Copyright (C) 2000, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#if defined(_MSC_VER)
// Turn off compiler warning about long names
#  pragma warning(disable:4786)
#endif

#include <cassert>

#include "CoinHelperFunctions.hpp"
#include "CoinPackedVector.hpp"

//#############################################################################

void
CoinPackedVector::clear()
{
   nElements_ = 0;
   clearBase();
}

//#############################################################################

CoinPackedVector &
CoinPackedVector::operator=(const CoinPackedVector & rhs)
{
   if (this != &rhs) {
      clear();
      gutsOfSetVector(rhs.getVectorNumElements(), rhs.getVectorIndices(), rhs.getVectorElements(),
		      CoinPackedVectorBase::testForDuplicateIndex(),
		      "operator=");
   }
   return *this;
}

//#############################################################################

CoinPackedVector &
CoinPackedVector::operator=(const CoinPackedVectorBase & rhs)
{
   if (this != &rhs) {
      clear();
      gutsOfSetVector(rhs.getNumElements(), rhs.getIndices(), rhs.getElements(),
		      CoinPackedVectorBase::testForDuplicateIndex(),
		      "operator= from base");
   }
   return *this;
}

//#############################################################################
#if 0
void
CoinPackedVector::assignVector(int size, int*& inds, double*& elems,
			      bool testForDuplicateIndex)
{
   clear();
   // Allocate storage
   if ( size != 0 ) {
      reserve(size);
      nElements_ = size;
      indices_ = inds;    inds = NULL;
      elements_ = elems;  elems = NULL;
      CoinIotaN(origIndices_, size, 0);
   }
   try {
      CoinPackedVectorBase::setTestForDuplicateIndex(testForDuplicateIndex);
   }
   catch (CoinError& e) {
      throw CoinError("duplicate index", "assignVector", "CoinPackedVector");
   }
}
#else
void
CoinPackedVector::assignVector(int size, int*& inds, double*& elems,
                              bool testForDuplicateIndex)
{
  clear();
  // Allocate storage
  if ( size != 0 ) {
		  //reserve(size); //This is a BUG!!!
    nElements_ = size;
    if (indices_ != NULL) delete[] indices_;
    indices_ = inds;    inds = NULL;
    if (elements_ != NULL) delete[] elements_;
    elements_ = elems;  elems = NULL;
    if (origIndices_ != NULL) delete[] origIndices_;
    origIndices_ = new int[size];
    CoinIotaN(origIndices_, size, 0);
    capacity_ = size;
  }
  if (testForDuplicateIndex) {
    try {    
      CoinPackedVectorBase::setTestForDuplicateIndex(testForDuplicateIndex);
    }
    catch (CoinError& e) {
    throw CoinError("duplicate index", "assignVector",
		    "CoinPackedVector");
    }
  } else {
    setTestsOff();
  }
}
#endif

//#############################################################################

void
CoinPackedVector::setVector(int size, const int * inds, const double * elems,
			   bool testForDuplicateIndex)
{
   clear();
   gutsOfSetVector(size, inds, elems, testForDuplicateIndex, "setVector");
}

//#############################################################################

void
CoinPackedVector::setConstant(int size, const int * inds, double value,
			     bool testForDuplicateIndex)
{
   clear();
   gutsOfSetConstant(size, inds, value, testForDuplicateIndex, "setConstant");
}

//#############################################################################

void
CoinPackedVector::setFull(int size, const double * elems,
			 bool testForDuplicateIndex) 
{
  // Clear out any values presently stored
  clear();
  
  // Allocate storage
  if ( size!=0 ) {
    reserve(size);  
    nElements_ = size;

    CoinIotaN(origIndices_, size, 0);
    CoinIotaN(indices_, size, 0);
    CoinDisjointCopyN(elems, size, elements_);
  }
  // Full array can not have duplicates
  CoinPackedVectorBase::setTestForDuplicateIndexWhenTrue(testForDuplicateIndex);
}

//#############################################################################
/* Indices are not specified and are taken to be 0,1,...,size-1,
    but only where non zero*/

void
CoinPackedVector::setFullNonZero(int size, const double * elems,
			 bool testForDuplicateIndex) 
{
  // Clear out any values presently stored
  clear();

  // For now waste space
  // Allocate storage
  if ( size!=0 ) {
    reserve(size);  
    nElements_ = 0;
    int i;
    for (i=0;i<size;i++) {
      if (elems[i]) {
	origIndices_[nElements_]= i;
	indices_[nElements_]= i;
	elements_[nElements_++] = elems[i];
      }
    }
  }
  // Full array can not have duplicates
  CoinPackedVectorBase::setTestForDuplicateIndexWhenTrue(testForDuplicateIndex);
}


//#############################################################################

void
CoinPackedVector::setElement(int index, double element)
{
#ifndef COIN_FAST_CODE
   if ( index >= nElements_ ) 
      throw CoinError("index >= size()", "setElement", "CoinPackedVector");
   if ( index < 0 ) 
      throw CoinError("index < 0" , "setElement", "CoinPackedVector");
#endif
   elements_[index] = element;
}

//#############################################################################

void
CoinPackedVector::insert( int index, double element )
{
   const int s = nElements_;
   if (testForDuplicateIndex()) {
      std::set<int>& is = *indexSet("insert", "CoinPackedVector");
      if (! is.insert(index).second)
	 throw CoinError("Index already exists", "insert", "CoinPackedVector");
   }

   if( capacity_ <= s ) {
      reserve( CoinMax(5, 2*capacity_) );
      assert( capacity_ > s );
   }
   indices_[s] = index;
   elements_[s] = element;
   origIndices_[s] = s;
   ++nElements_;
}

//#############################################################################

void
CoinPackedVector::append(const CoinPackedVectorBase & caboose)
{
   const int cs = caboose.getNumElements();
   if (cs == 0) {
       return;
   }
   if (testForDuplicateIndex()) {
       // Just to initialize the index heap
       indexSet("append (1st call)", "CoinPackedVector");
   }
   const int s = nElements_;
   // Make sure there is enough room for the caboose
   if ( capacity_ < s + cs)
      reserve(CoinMax(s + cs, 2 * capacity_));

   const int * cind = caboose.getIndices();
   const double * celem = caboose.getElements();
   CoinDisjointCopyN(cind, cs, indices_ + s);
   CoinDisjointCopyN(celem, cs, elements_ + s);
   CoinIotaN(origIndices_ + s, cs, s);
   nElements_ += cs;
   if (testForDuplicateIndex()) {
      std::set<int>& is = *indexSet("append (2nd call)", "CoinPackedVector");
      for (int i = 0; i < cs; ++i) {
	 if (!is.insert(cind[i]).second)
	    throw CoinError("duplicate index", "append", "CoinPackedVector");
      }
   }
}

//#############################################################################

void
CoinPackedVector::swap(int i, int j)
{
   if ( i >= nElements_ ) 
      throw CoinError("index i >= size()","swap","CoinPackedVector");
   if ( i < 0 ) 
      throw CoinError("index i < 0" ,"swap","CoinPackedVector");
   if ( i >= nElements_ ) 
      throw CoinError("index j >= size()","swap","CoinPackedVector");
   if ( i < 0 ) 
      throw CoinError("index j < 0" ,"swap","CoinPackedVector");

   // Swap positions i and j of the
   // indices and elements arrays
   std::swap(indices_[i], indices_[j]);
   std::swap(elements_[i], elements_[j]);
}

//#############################################################################

void
CoinPackedVector::truncate( int n )
{
   if ( n > nElements_ ) 
      throw CoinError("n > size()","truncate","CoinPackedVector");
   if ( n < 0 ) 
      throw CoinError("n < 0","truncate","CoinPackedVector");
   nElements_ = n;
   clearBase();
}

//#############################################################################

void
CoinPackedVector::operator+=(double value) 
{
   std::transform(elements_, elements_ + nElements_, elements_,
		  std::bind2nd(std::plus<double>(), value) );
}

//-----------------------------------------------------------------------------

void
CoinPackedVector::operator-=(double value) 
{
   std::transform(elements_, elements_ + nElements_, elements_,
		  std::bind2nd(std::minus<double>(), value) );
}

//-----------------------------------------------------------------------------

void
CoinPackedVector::operator*=(double value) 
{
   std::transform(elements_, elements_ + nElements_, elements_,
		  std::bind2nd(std::multiplies<double>(), value) );
}

//-----------------------------------------------------------------------------

void
CoinPackedVector::operator/=(double value) 
{
   std::transform(elements_, elements_ + nElements_, elements_,
		  std::bind2nd(std::divides<double>(), value) );
}

//#############################################################################

void
CoinPackedVector::sortOriginalOrder() {
  CoinSort_3(origIndices_, origIndices_ + nElements_, indices_, elements_);
}

//#############################################################################

void
CoinPackedVector::reserve(int n)
{
   // don't make allocated space smaller
   if ( n <= capacity_ )
      return;
   capacity_ = n;

   // save pointers to existing data
   int * tempIndices = indices_;
   int * tempOrigIndices = origIndices_;
   double * tempElements = elements_;

   // allocate new space
   indices_ = new int [capacity_];
   origIndices_ = new int [capacity_];
   elements_ = new double [capacity_];

   // copy data to new space
   if (nElements_ > 0) {
      CoinDisjointCopyN(tempIndices, nElements_, indices_);
      CoinDisjointCopyN(tempOrigIndices, nElements_, origIndices_);
      CoinDisjointCopyN(tempElements, nElements_, elements_);
   }

   // free old data
   delete [] tempElements;
   delete [] tempOrigIndices;
   delete [] tempIndices;
}

//#############################################################################

CoinPackedVector::CoinPackedVector (bool testForDuplicateIndex) :
   CoinPackedVectorBase(),
   indices_(NULL),
   elements_(NULL),
   nElements_(0),
   origIndices_(NULL),
   capacity_(0)
{
   // This won't fail, the packed vector is empty. There can't be duplicate
   // indices.
   CoinPackedVectorBase::setTestForDuplicateIndex(testForDuplicateIndex);
}

//-----------------------------------------------------------------------------

CoinPackedVector::CoinPackedVector(int size,
				 const int * inds, const double * elems,
				 bool testForDuplicateIndex) :
   CoinPackedVectorBase(),
   indices_(NULL),
   elements_(NULL),
   nElements_(0),
   origIndices_(NULL),
   capacity_(0)
{
   gutsOfSetVector(size, inds, elems, testForDuplicateIndex,
		   "constructor for array value");
}

//-----------------------------------------------------------------------------

CoinPackedVector::CoinPackedVector(int size,
				 const int * inds, double value,
				 bool testForDuplicateIndex) :
   CoinPackedVectorBase(),
   indices_(NULL),
   elements_(NULL),
   nElements_(0),
   origIndices_(NULL),
   capacity_(0)
{
   gutsOfSetConstant(size, inds, value, testForDuplicateIndex,
		     "constructor for constant value");
}

//-----------------------------------------------------------------------------

CoinPackedVector::CoinPackedVector(int capacity, int size,
 				   int *&inds, double *&elems,
				   bool /*testForDuplicateIndex*/) :
    CoinPackedVectorBase(),
    indices_(inds),
    elements_(elems),
    nElements_(size),
    origIndices_(NULL),
    capacity_(capacity)
{
   assert( size <= capacity );
   inds = NULL;
   elems = NULL;
   origIndices_ = new int[capacity_];
   CoinIotaN(origIndices_, size, 0);
}

//-----------------------------------------------------------------------------

CoinPackedVector::CoinPackedVector(int size, const double * element,
				 bool testForDuplicateIndex) :
   CoinPackedVectorBase(),
   indices_(NULL),
   elements_(NULL),
   nElements_(0),
   origIndices_(NULL),
   capacity_(0)
{
   setFull(size, element, testForDuplicateIndex);
}

//-----------------------------------------------------------------------------

CoinPackedVector::CoinPackedVector(const CoinPackedVectorBase & rhs) :
   CoinPackedVectorBase(),
   indices_(NULL),
   elements_(NULL),
   nElements_(0),
   origIndices_(NULL),
   capacity_(0)
{  
   gutsOfSetVector(rhs.getNumElements(), rhs.getIndices(), rhs.getElements(),
		   rhs.testForDuplicateIndex(), "copy constructor from base");
}

//-----------------------------------------------------------------------------

CoinPackedVector::CoinPackedVector(const CoinPackedVector & rhs) :
   CoinPackedVectorBase(),
   indices_(NULL),
   elements_(NULL),
   nElements_(0),
   origIndices_(NULL),
   capacity_(0)
{  
   gutsOfSetVector(rhs.getVectorNumElements(), rhs.getVectorIndices(), rhs.getVectorElements(),
		   rhs.testForDuplicateIndex(), "copy constructor");
}

//-----------------------------------------------------------------------------

CoinPackedVector::~CoinPackedVector ()
{
   delete [] indices_;
   delete [] origIndices_;
   delete [] elements_;
}

//#############################################################################

void
CoinPackedVector::gutsOfSetVector(int size,
				 const int * inds, const double * elems,
				 bool testForDuplicateIndex,
				 const char * method)
{
   if ( size != 0 ) {
      reserve(size);
      nElements_ = size;
      CoinDisjointCopyN(inds, size, indices_);
      CoinDisjointCopyN(elems, size, elements_);
      CoinIotaN(origIndices_, size, 0);
   }
   if (testForDuplicateIndex) {
     try {
       CoinPackedVectorBase::setTestForDuplicateIndex(testForDuplicateIndex);
     }
     catch (CoinError& e) {
       throw CoinError("duplicate index", method, "CoinPackedVector");
     }
   } else {
     setTestsOff();
   }
}

//-----------------------------------------------------------------------------

void
CoinPackedVector::gutsOfSetConstant(int size,
				   const int * inds, double value,
				   bool testForDuplicateIndex,
				   const char * method)
{
   if ( size != 0 ) {
      reserve(size);
      nElements_ = size;
      CoinDisjointCopyN(inds, size, indices_);
      CoinFillN(elements_, size, value);
      CoinIotaN(origIndices_, size, 0);
   }
   try {
      CoinPackedVectorBase::setTestForDuplicateIndex(testForDuplicateIndex);
   }
   catch (CoinError& e) {
      throw CoinError("duplicate index", method, "CoinPackedVector");
   }
}

//#############################################################################
