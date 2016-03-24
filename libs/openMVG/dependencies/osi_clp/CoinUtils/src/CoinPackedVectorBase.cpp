/* $Id: CoinPackedVectorBase.cpp 1416 2011-04-17 09:57:29Z stefan $ */
// Copyright (C) 2000, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include <numeric>

#include "CoinPackedVectorBase.hpp"
#include "CoinTypes.hpp"
#include "CoinHelperFunctions.hpp"
#include "CoinFloatEqual.hpp"

//#############################################################################

double *
CoinPackedVectorBase::denseVector(int denseSize) const
{
   if (getMaxIndex() >= denseSize)
      throw CoinError("Dense vector size is less than max index",
		     "denseVector", "CoinPackedVectorBase");

   double * dv = new double[denseSize];
   CoinFillN(dv, denseSize, 0.0);
   const int s = getNumElements();
   const int * inds = getIndices();
   const double * elems = getElements();
   for (int i = 0; i < s; ++i)
      dv[inds[i]] = elems[i];
   return dv;
}

//-----------------------------------------------------------------------------

double
CoinPackedVectorBase::operator[](int i) const
{
   if (! testedDuplicateIndex_)
      duplicateIndex("operator[]", "CoinPackedVectorBase");

   // Get a reference to a map of full storage indices to
   // packed storage location.
   const std::set<int> & sv = *indexSet("operator[]", "CoinPackedVectorBase");
#if 1
   if (sv.find(i) == sv.end())
      return 0.0;
   return getElements()[findIndex(i)];
#else
   // LL: suggested change, somthing is wrong with this
   const size_t ind = std::distance(sv.begin(), sv.find(i));
   return (ind == sv.size()) ? 0.0 : getElements()[ind];
#endif

}

//#############################################################################

void
CoinPackedVectorBase::setTestForDuplicateIndex(bool test) const
{
   if (test == true) {
      testForDuplicateIndex_ = true;
      duplicateIndex("setTestForDuplicateIndex", "CoinPackedVectorBase");
   } else {
      testForDuplicateIndex_ = false;
      testedDuplicateIndex_ = false;
   }
}

//#############################################################################

void
CoinPackedVectorBase::setTestForDuplicateIndexWhenTrue(bool test) const
{
  // We know everything is okay so let's not test (e.g. full array)
  testForDuplicateIndex_ = test;
  testedDuplicateIndex_ = test;
}

//#############################################################################

int
CoinPackedVectorBase::getMaxIndex() const
{
   findMaxMinIndices();
   return maxIndex_;
}

//-----------------------------------------------------------------------------

int
CoinPackedVectorBase::getMinIndex() const
{
   findMaxMinIndices();
   return minIndex_;
}

//-----------------------------------------------------------------------------

void
CoinPackedVectorBase::duplicateIndex(const char* methodName,
				    const char * className) const
{
   if (testForDuplicateIndex())
      indexSet(methodName, className);
   testedDuplicateIndex_ = true;
}

//-----------------------------------------------------------------------------

bool
CoinPackedVectorBase::isExistingIndex(int i) const
{
   if (! testedDuplicateIndex_)
      duplicateIndex("indexExists", "CoinPackedVectorBase");

   const std::set<int> & sv = *indexSet("indexExists", "CoinPackedVectorBase");
   return sv.find(i) != sv.end();
}


int
CoinPackedVectorBase::findIndex(int i) const
{
   const int * inds = getIndices();
   int retVal = static_cast<int>(std::find(inds, inds + getNumElements(), i) - inds);
   if (retVal == getNumElements() ) retVal = -1;
   return retVal;
}

//#############################################################################

bool
CoinPackedVectorBase::operator==(const CoinPackedVectorBase& rhs) const
{  if (getNumElements() == 0 || rhs.getNumElements() == 0) {
     if (getNumElements() == 0 && rhs.getNumElements() == 0)
       return (true) ;
     else
       return (false) ;
   } else {
     return (getNumElements()==rhs.getNumElements() &&
	     std::equal(getIndices(),getIndices()+getNumElements(),
		        rhs.getIndices()) &&
	     std::equal(getElements(),getElements()+getNumElements(),
		        rhs.getElements())) ;
   }
}

//-----------------------------------------------------------------------------

bool
CoinPackedVectorBase::operator!=(const CoinPackedVectorBase& rhs) const
{
   return !( (*this)==rhs );
}

//-----------------------------------------------------------------------------

int
CoinPackedVectorBase::compare(const CoinPackedVectorBase& rhs) const
{
  const int size = getNumElements();
  int itmp = size - rhs.getNumElements();
  if (itmp != 0) {
    return itmp;
  }
  itmp = memcmp(getIndices(), rhs.getIndices(), size * sizeof(int));
  if (itmp != 0) {
    return itmp;
  }
  return memcmp(getElements(), rhs.getElements(), size * sizeof(double));
}

bool
CoinPackedVectorBase::isEquivalent(const CoinPackedVectorBase& rhs) const
{
   return isEquivalent(rhs,  CoinRelFltEq());
}

//#############################################################################

double
CoinPackedVectorBase::dotProduct(const double* dense) const
{
   const double * elems = getElements();
   const int * inds = getIndices();
   double dp = 0.0;
   for (int i = getNumElements() - 1; i >= 0; --i)
      dp += elems[i] * dense[inds[i]];
   return dp;
}

//-----------------------------------------------------------------------------

double
CoinPackedVectorBase::oneNorm() const
{
   double norm = 0.0;
   const double* elements = getElements();
   for (int i = getNumElements() - 1; i >= 0; --i) {
      norm += fabs(elements[i]);
   }
   return norm;
}

//-----------------------------------------------------------------------------

double
CoinPackedVectorBase::normSquare() const
{
   return std::inner_product(getElements(), getElements() + getNumElements(),
			     getElements(), 0.0);
}

//-----------------------------------------------------------------------------

double
CoinPackedVectorBase::twoNorm() const
{
   return sqrt(normSquare());
}

//-----------------------------------------------------------------------------

double
CoinPackedVectorBase::infNorm() const
{
   double norm = 0.0;
   const double* elements = getElements();
   for (int i = getNumElements() - 1; i >= 0; --i) {
      norm = CoinMax(norm, fabs(elements[i]));
   }
   return norm;
}

//-----------------------------------------------------------------------------

double
CoinPackedVectorBase::sum() const
{
   return std::accumulate(getElements(), getElements() + getNumElements(), 0.0);
}

//#############################################################################

CoinPackedVectorBase::CoinPackedVectorBase() :
   maxIndex_(-COIN_INT_MAX/*0*/),
   minIndex_(COIN_INT_MAX/*0*/),
   indexSetPtr_(NULL),
   testForDuplicateIndex_(true),
   testedDuplicateIndex_(false) {}

//-----------------------------------------------------------------------------

CoinPackedVectorBase::~CoinPackedVectorBase()
{
   delete indexSetPtr_;
}

//#############################################################################
//#############################################################################

void
CoinPackedVectorBase::findMaxMinIndices() const
{
   if ( getNumElements()==0 )
      return;
   // if indexSet exists then grab begin and rend to get min & max indices
   else if ( indexSetPtr_ != NULL ) {
      maxIndex_ = *indexSetPtr_->rbegin();
      minIndex_ = *indexSetPtr_-> begin();
   } else {
      // Have to scan through vector to find min and max.
      maxIndex_ = *(std::max_element(getIndices(),
				     getIndices() + getNumElements()));
      minIndex_ = *(std::min_element(getIndices(),
				     getIndices() + getNumElements()));
   }
}

//-------------------------------------------------------------------

std::set<int> *
CoinPackedVectorBase::indexSet(const char* methodName,
			      const char * className) const
{
   testedDuplicateIndex_ = true;
   if ( indexSetPtr_ == NULL ) {
      // create a set of the indices
      indexSetPtr_ = new std::set<int>;
      const int s = getNumElements();
      const int * inds = getIndices();
      for (int j=0; j < s; ++j) {
	 if (!indexSetPtr_->insert(inds[j]).second) {
	    testedDuplicateIndex_ = false;
	    delete indexSetPtr_;
	    indexSetPtr_ = NULL;
	    if (methodName != NULL) {
	       throw CoinError("Duplicate index found", methodName, className);
	    } else {
	       throw CoinError("Duplicate index found",
			      "indexSet", "CoinPackedVectorBase");
	    }
	 }
      }
   }
   return indexSetPtr_;
}

//-----------------------------------------------------------------------------

void
CoinPackedVectorBase::clearIndexSet() const
{
   delete indexSetPtr_;
   indexSetPtr_ = NULL;
}

//-----------------------------------------------------------------------------

void
CoinPackedVectorBase::clearBase() const
{
   clearIndexSet();
   maxIndex_ = -COIN_INT_MAX/*0*/;
   minIndex_ = COIN_INT_MAX/*0*/;
   testedDuplicateIndex_ = false;
}

//#############################################################################
