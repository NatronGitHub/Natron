/* $Id: CoinPackedVector.hpp 1509 2011-12-05 13:50:48Z forrest $ */
// Copyright (C) 2000, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef CoinPackedVector_H
#define CoinPackedVector_H

#include <map>

#include "CoinPragma.hpp"
#include "CoinPackedVectorBase.hpp"
#include "CoinSort.hpp"

#ifdef COIN_FAST_CODE
#ifndef COIN_NOTEST_DUPLICATE
#define COIN_NOTEST_DUPLICATE
#endif
#endif

#ifndef COIN_NOTEST_DUPLICATE
#define COIN_DEFAULT_VALUE_FOR_DUPLICATE true
#else
#define COIN_DEFAULT_VALUE_FOR_DUPLICATE false
#endif
/** Sparse Vector

Stores vector of indices and associated element values.
Supports sorting of vector while maintaining the original indices.

Here is a sample usage:
@verbatim
    const int ne = 4;
    int inx[ne] =   {  1,   4,  0,   2 }
    double el[ne] = { 10., 40., 1., 50. }

    // Create vector and set its value
    CoinPackedVector r(ne,inx,el);

    // access each index and element
    assert( r.indices ()[0]== 1  );
    assert( r.elements()[0]==10. );
    assert( r.indices ()[1]== 4  );
    assert( r.elements()[1]==40. );
    assert( r.indices ()[2]== 0  );
    assert( r.elements()[2]== 1. );
    assert( r.indices ()[3]== 2  );
    assert( r.elements()[3]==50. );

    // access original position of index
    assert( r.originalPosition()[0]==0 );
    assert( r.originalPosition()[1]==1 );
    assert( r.originalPosition()[2]==2 );
    assert( r.originalPosition()[3]==3 );

    // access as a full storage vector
    assert( r[ 0]==1. );
    assert( r[ 1]==10.);
    assert( r[ 2]==50.);
    assert( r[ 3]==0. );
    assert( r[ 4]==40.);

    // sort Elements in increasing order
    r.sortIncrElement();

    // access each index and element
    assert( r.indices ()[0]== 0  );
    assert( r.elements()[0]== 1. );
    assert( r.indices ()[1]== 1  );
    assert( r.elements()[1]==10. );
    assert( r.indices ()[2]== 4  );
    assert( r.elements()[2]==40. );
    assert( r.indices ()[3]== 2  );
    assert( r.elements()[3]==50. );    

    // access original position of index    
    assert( r.originalPosition()[0]==2 );
    assert( r.originalPosition()[1]==0 );
    assert( r.originalPosition()[2]==1 );
    assert( r.originalPosition()[3]==3 );

    // access as a full storage vector
    assert( r[ 0]==1. );
    assert( r[ 1]==10.);
    assert( r[ 2]==50.);
    assert( r[ 3]==0. );
    assert( r[ 4]==40.);

    // Restore orignal sort order
    r.sortOriginalOrder();
    
    assert( r.indices ()[0]== 1  );
    assert( r.elements()[0]==10. );
    assert( r.indices ()[1]== 4  );
    assert( r.elements()[1]==40. );
    assert( r.indices ()[2]== 0  );
    assert( r.elements()[2]== 1. );
    assert( r.indices ()[3]== 2  );
    assert( r.elements()[3]==50. );

    // Tests for equality and equivalence
    CoinPackedVector r1;
    r1=r;
    assert( r==r1 );
    assert( r.equivalent(r1) );
    r.sortIncrElement();
    assert( r!=r1 );
    assert( r.equivalent(r1) );

    // Add packed vectors.
    // Similarly for subtraction, multiplication,
    // and division.
    CoinPackedVector add = r + r1;
    assert( add[0] ==  1.+ 1. );
    assert( add[1] == 10.+10. );
    assert( add[2] == 50.+50. );
    assert( add[3] ==  0.+ 0. );
    assert( add[4] == 40.+40. );

    assert( r.sum() == 10.+40.+1.+50. );
@endverbatim
*/
class CoinPackedVector : public CoinPackedVectorBase {
   friend void CoinPackedVectorUnitTest();
  
public:
   /**@name Get methods. */
   //@{
   /// Get the size
   virtual int getNumElements() const { return nElements_; }
   /// Get indices of elements
   virtual const int * getIndices() const { return indices_; }
   /// Get element values
   virtual const double * getElements() const { return elements_; }
   /// Get indices of elements
   int * getIndices() { return indices_; }
   /// Get the size
   inline int getVectorNumElements() const { return nElements_; }
   /// Get indices of elements
   inline const int * getVectorIndices() const { return indices_; }
   /// Get element values
   inline const double * getVectorElements() const { return elements_; }
   /// Get element values
   double * getElements() { return elements_; }
   /** Get pointer to int * vector of original postions.
       If the packed vector has not been sorted then this
       function returns the vector: 0, 1, 2, ..., size()-1. */
   const int * getOriginalPosition() const { return origIndices_; }
   //@}
 
   //-------------------------------------------------------------------
   // Set indices and elements
   //------------------------------------------------------------------- 
   /**@name Set methods */
   //@{
   /// Reset the vector (as if were just created an empty vector)
   void clear();
   /** Assignment operator. <br>
       <strong>NOTE</strong>: This operator keeps the current
       <code>testForDuplicateIndex</code> setting, and affter copying the data
       it acts accordingly. */
   CoinPackedVector & operator=(const CoinPackedVector &);
   /** Assignment operator from a CoinPackedVectorBase. <br>
       <strong>NOTE</strong>: This operator keeps the current
       <code>testForDuplicateIndex</code> setting, and affter copying the data
       it acts accordingly. */
   CoinPackedVector & operator=(const CoinPackedVectorBase & rhs);

   /** Assign the ownership of the arguments to this vector.
       Size is the length of both the indices and elements vectors.
       The indices and elements vectors are copied into this class instance's
       member data. The last argument indicates whether this vector will have
       to be tested for duplicate indices.
   */
   void assignVector(int size, int*& inds, double*& elems,
		     bool testForDuplicateIndex = COIN_DEFAULT_VALUE_FOR_DUPLICATE);

   /** Set vector size, indices, and elements.
       Size is the length of both the indices and elements vectors.
       The indices and elements vectors are copied into this class instance's
       member data. The last argument specifies whether this vector will have
       to be checked for duplicate indices whenever that can happen. */
   void setVector(int size, const int * inds, const double * elems,
		  bool testForDuplicateIndex = COIN_DEFAULT_VALUE_FOR_DUPLICATE);
  
   /** Elements set to have the same scalar value */
   void setConstant(int size, const int * inds, double elems,
		    bool testForDuplicateIndex = COIN_DEFAULT_VALUE_FOR_DUPLICATE);
  
   /** Indices are not specified and are taken to be 0,1,...,size-1 */
   void setFull(int size, const double * elems,
		bool testForDuplicateIndex = COIN_DEFAULT_VALUE_FOR_DUPLICATE);

   /** Indices are not specified and are taken to be 0,1,...,size-1,
    but only where non zero*/
   void setFullNonZero(int size, const double * elems,
		bool testForDuplicateIndex = COIN_DEFAULT_VALUE_FOR_DUPLICATE);

   /** Set an existing element in the packed vector
       The first argument is the "index" into the elements() array
   */
   void setElement(int index, double element);

   /// Insert an element into the vector
   void insert(int index, double element);
   /// Append a CoinPackedVector to the end
   void append(const CoinPackedVectorBase & caboose);

   /// Swap values in positions i and j of indices and elements
   void swap(int i, int j); 

   /** Resize the packed vector to be the first newSize elements.
       Problem with truncate: what happens with origIndices_ ??? */
   void truncate(int newSize); 
   //@}

   /**@name Arithmetic operators. */
   //@{
   /// add <code>value</code> to every entry
   void operator+=(double value);
   /// subtract <code>value</code> from every entry
   void operator-=(double value);
   /// multiply every entry by <code>value</code>
   void operator*=(double value);
   /// divide every entry by <code>value</code>
   void operator/=(double value);
   //@}

   /**@name Sorting */
   //@{ 
   /** Sort the packed storage vector.
       Typcical usages:
       <pre> 
       packedVector.sort(CoinIncrIndexOrdered());   //increasing indices
       packedVector.sort(CoinIncrElementOrdered()); // increasing elements
       </pre>
   */ 
   template <class CoinCompare3>
   void sort(const CoinCompare3 & tc)
   { CoinSort_3(indices_, indices_ + nElements_, origIndices_, elements_,
		tc); }

   void sortIncrIndex()
   { CoinSort_3(indices_, indices_ + nElements_, origIndices_, elements_,
		CoinFirstLess_3<int, int, double>()); }

   void sortDecrIndex()
   { CoinSort_3(indices_, indices_ + nElements_, origIndices_, elements_,
		CoinFirstGreater_3<int, int, double>()); }
  
   void sortIncrElement()
   { CoinSort_3(elements_, elements_ + nElements_, origIndices_, indices_,
		CoinFirstLess_3<double, int, int>()); }

   void sortDecrElement()
   { CoinSort_3(elements_, elements_ + nElements_, origIndices_, indices_,
		CoinFirstGreater_3<double, int, int>()); }
  

   /** Sort in original order.
       If the vector has been sorted, then this method restores
       to its orignal sort order.
   */
   void sortOriginalOrder();
   //@}

   /**@name Memory usage */
   //@{
   /** Reserve space.
       If one knows the eventual size of the packed vector,
       then it may be more efficient to reserve the space.
   */
   void reserve(int n);
   /** capacity returns the size which could be accomodated without
       having to reallocate storage.
   */
   int capacity() const { return capacity_; }
   //@}
   /**@name Constructors and destructors */
   //@{
   /** Default constructor */
   CoinPackedVector(bool testForDuplicateIndex = COIN_DEFAULT_VALUE_FOR_DUPLICATE);
   /** \brief Alternate Constructors - set elements to vector of doubles
   
     This constructor copies the vectors provided as parameters.
   */
   CoinPackedVector(int size, const int * inds, const double * elems,
		   bool testForDuplicateIndex = COIN_DEFAULT_VALUE_FOR_DUPLICATE);
   /** \brief Alternate Constructors - set elements to vector of doubles

     This constructor takes ownership of the vectors passed as parameters.
     \p inds and \p elems will be NULL on return.
   */
   CoinPackedVector(int capacity, int size, int *&inds, double *&elems,
		    bool testForDuplicateIndex = COIN_DEFAULT_VALUE_FOR_DUPLICATE);
   /** Alternate Constructors - set elements to same scalar value */
   CoinPackedVector(int size, const int * inds, double element,
		   bool testForDuplicateIndex = COIN_DEFAULT_VALUE_FOR_DUPLICATE);
   /** Alternate Constructors - construct full storage with indices 0 through
       size-1. */
   CoinPackedVector(int size, const double * elements,
		   bool testForDuplicateIndex = COIN_DEFAULT_VALUE_FOR_DUPLICATE);
   /** Copy constructor. */
   CoinPackedVector(const CoinPackedVector &);
   /** Copy constructor <em>from a PackedVectorBase</em>. */
   CoinPackedVector(const CoinPackedVectorBase & rhs);
   /** Destructor */
   virtual ~CoinPackedVector ();
   //@}
    
private:
   /**@name Private methods */
   //@{  
   /// Copy internal date
   void gutsOfSetVector(int size,
			const int * inds, const double * elems,
			bool testForDuplicateIndex,
			const char * method);
   ///
   void gutsOfSetConstant(int size,
			  const int * inds, double value,
			  bool testForDuplicateIndex,
			  const char * method);
   //@}

private:
   /**@name Private member data */
   //@{
   /// Vector indices
   int * indices_;
   ///Vector elements
   double * elements_;
   /// Size of indices and elements vectors
   int nElements_;
   /// original unsorted indices
   int * origIndices_;
   /// Amount of memory allocated for indices_, origIndices_, and elements_.
   int capacity_;
   //@}
};

//#############################################################################

/**@name Arithmetic operators on packed vectors.

   <strong>NOTE</strong>: These methods operate on those positions where at
   least one of the arguments has a value listed. At those positions the
   appropriate operation is executed, Otherwise the result of the operation is
   considered 0.<br>
   <strong>NOTE 2</strong>: There are two kind of operators here. One is used
   like "c = binaryOp(a, b)", the other is used like "binaryOp(c, a, b)", but
   they are really the same. The first is much more natural to use, but it
   involves the creation of a temporary object (the function *must* return an
   object), while the second form puts the result directly into the argument
   "c". Therefore, depending on the circumstances, the second form can be
   significantly faster.
 */
//@{
template <class BinaryFunction> void
binaryOp(CoinPackedVector& retVal,
	 const CoinPackedVectorBase& op1, double value,
	 BinaryFunction bf)
{
   retVal.clear();
   const int s = op1.getNumElements();
   if (s > 0) {
      retVal.reserve(s);
      const int * inds = op1.getIndices();
      const double * elems = op1.getElements();
      for (int i=0; i<s; ++i ) {
	 retVal.insert(inds[i], bf(value, elems[i]));
      }
   }
}

template <class BinaryFunction> inline void
binaryOp(CoinPackedVector& retVal,
	 double value, const CoinPackedVectorBase& op2,
	 BinaryFunction bf)
{
   binaryOp(retVal, op2, value, bf);
}

template <class BinaryFunction> void
binaryOp(CoinPackedVector& retVal,
	 const CoinPackedVectorBase& op1, const CoinPackedVectorBase& op2,
	 BinaryFunction bf)
{
   retVal.clear();
   const int s1 = op1.getNumElements();
   const int s2 = op2.getNumElements();
/*
  Replaced || with &&, in response to complaint from Sven deVries, who
  rightly points out || is not appropriate for additive operations. &&
  should be ok as long as binaryOp is understood not to create something
  from nothing.		-- lh, 04.06.11
*/
   if (s1 == 0 && s2 == 0)
      return;

   retVal.reserve(s1+s2);

   const int * inds1 = op1.getIndices();
   const double * elems1 = op1.getElements();
   const int * inds2 = op2.getIndices();
   const double * elems2 = op2.getElements();

   int i;
   // loop once for each element in op1
   for ( i=0; i<s1; ++i ) {
      const int index = inds1[i];
      const int pos2 = op2.findIndex(index);
      const double val = bf(elems1[i], pos2 == -1 ? 0.0 : elems2[pos2]);
      // if (val != 0.0) // *THINK* : should we put in only nonzeros?
      retVal.insert(index, val);
   }
   // loop once for each element in operand2  
   for ( i=0; i<s2; ++i ) {
      const int index = inds2[i];
      // if index exists in op1, then element was processed in prior loop
      if ( op1.isExistingIndex(index) )
	 continue;
      // Index does not exist in op1, so the element value must be zero
      const double val = bf(0.0, elems2[i]);
      // if (val != 0.0) // *THINK* : should we put in only nonzeros?
      retVal.insert(index, val);
   }
}

//-----------------------------------------------------------------------------

template <class BinaryFunction> CoinPackedVector
binaryOp(const CoinPackedVectorBase& op1, double value,
	 BinaryFunction bf)
{
   CoinPackedVector retVal;
   retVal.setTestForDuplicateIndex(true);
   binaryOp(retVal, op1, value, bf);
   return retVal;
}

template <class BinaryFunction> CoinPackedVector
binaryOp(double value, const CoinPackedVectorBase& op2,
	 BinaryFunction bf)
{
   CoinPackedVector retVal;
   retVal.setTestForDuplicateIndex(true);
   binaryOp(retVal, op2, value, bf);
   return retVal;
}

template <class BinaryFunction> CoinPackedVector
binaryOp(const CoinPackedVectorBase& op1, const CoinPackedVectorBase& op2,
	 BinaryFunction bf)
{
   CoinPackedVector retVal;
   retVal.setTestForDuplicateIndex(true);
   binaryOp(retVal, op1, op2, bf);
   return retVal;
}

//-----------------------------------------------------------------------------
/// Return the sum of two packed vectors
inline CoinPackedVector operator+(const CoinPackedVectorBase& op1,
				  const CoinPackedVectorBase& op2)
{
   CoinPackedVector retVal;
   retVal.setTestForDuplicateIndex(true);
   binaryOp(retVal, op1, op2, std::plus<double>());
   return retVal;
}

/// Return the difference of two packed vectors
inline CoinPackedVector operator-(const CoinPackedVectorBase& op1,
				 const CoinPackedVectorBase& op2)
{
   CoinPackedVector retVal;
   retVal.setTestForDuplicateIndex(true);
   binaryOp(retVal, op1, op2, std::minus<double>());
   return retVal;
}

/// Return the element-wise product of two packed vectors
inline CoinPackedVector operator*(const CoinPackedVectorBase& op1,
				  const CoinPackedVectorBase& op2)
{
   CoinPackedVector retVal;
   retVal.setTestForDuplicateIndex(true);
   binaryOp(retVal, op1, op2, std::multiplies<double>());
   return retVal;
}

/// Return the element-wise ratio of two packed vectors
inline CoinPackedVector operator/(const CoinPackedVectorBase& op1,
				  const CoinPackedVectorBase& op2)
{
   CoinPackedVector retVal;
   retVal.setTestForDuplicateIndex(true);
   binaryOp(retVal, op1, op2, std::divides<double>());
   return retVal;
}
//@}

/// Returns the dot product of two CoinPackedVector objects whose elements are
/// doubles.  Use this version if the vectors are *not* guaranteed to be sorted.
inline double sparseDotProduct(const CoinPackedVectorBase& op1,
                        const CoinPackedVectorBase& op2){
  int len, i;
  double acc = 0.0;
  CoinPackedVector retVal;

  CoinPackedVector retval = op1*op2;
  len = retval.getNumElements();
  double * CParray = retval.getElements();

  for(i = 0; i < len; i++){
    acc += CParray[i];
  }
return acc;
}


/// Returns the dot product of two sorted CoinPackedVector objects.
///  The vectors should be sorted in ascending order of indices.
inline double sortedSparseDotProduct(const CoinPackedVectorBase& op1,
                        const CoinPackedVectorBase& op2){
  int i, j, len1, len2;
  double acc = 0.0;

  const double* v1val = op1.getElements();
  const double* v2val = op2.getElements();
  const int* v1ind = op1.getIndices();
  const int* v2ind = op2.getIndices();

  len1 = op1.getNumElements();
  len2 = op2.getNumElements();

  i = 0;
  j = 0;

  while(i < len1 && j < len2){
    if(v1ind[i] == v2ind[j]){
      acc += v1val[i] * v2val[j];
      i++;
      j++;
   }
    else if(v2ind[j] < v1ind[i]){
      j++;
    }
    else{
      i++;
    } // end if-else-elseif
  } // end while
  return acc;
 }


//-----------------------------------------------------------------------------

/**@name Arithmetic operators on packed vector and a constant. <br>
   These functions create a packed vector as a result. That packed vector will
   have the same indices as <code>op1</code> and the specified operation is
   done entry-wise with the given value. */
//@{
/// Return the sum of a packed vector and a constant
inline CoinPackedVector
operator+(const CoinPackedVectorBase& op1, double value)
{
   CoinPackedVector retVal(op1);
   retVal += value;
   return retVal;
}

/// Return the difference of a packed vector and a constant
inline CoinPackedVector
operator-(const CoinPackedVectorBase& op1, double value)
{
   CoinPackedVector retVal(op1);
   retVal -= value;
   return retVal;
}

/// Return the element-wise product of a packed vector and a constant
inline CoinPackedVector
operator*(const CoinPackedVectorBase& op1, double value)
{
   CoinPackedVector retVal(op1);
   retVal *= value;
   return retVal;
}

/// Return the element-wise ratio of a packed vector and a constant
inline CoinPackedVector
operator/(const CoinPackedVectorBase& op1, double value)
{
   CoinPackedVector retVal(op1);
   retVal /= value;
   return retVal;
}

//-----------------------------------------------------------------------------

/// Return the sum of a constant and a packed vector
inline CoinPackedVector
operator+(double value, const CoinPackedVectorBase& op1)
{
   CoinPackedVector retVal(op1);
   retVal += value;
   return retVal;
}

/// Return the difference of a constant and a packed vector
inline CoinPackedVector
operator-(double value, const CoinPackedVectorBase& op1)
{
   CoinPackedVector retVal(op1);
   const int size = retVal.getNumElements();
   double* elems = retVal.getElements();
   for (int i = 0; i < size; ++i) {
      elems[i] = value - elems[i];
   }
   return retVal;
}

/// Return the element-wise product of a constant and a packed vector
inline CoinPackedVector
operator*(double value, const CoinPackedVectorBase& op1)
{
   CoinPackedVector retVal(op1);
   retVal *= value;
   return retVal;
}

/// Return the element-wise ratio of a a constant and packed vector
inline CoinPackedVector
operator/(double value, const CoinPackedVectorBase& op1)
{
   CoinPackedVector retVal(op1);
   const int size = retVal.getNumElements();
   double* elems = retVal.getElements();
   for (int i = 0; i < size; ++i) {
      elems[i] = value / elems[i];
   }
   return retVal;
}
//@}

//#############################################################################
/** A function that tests the methods in the CoinPackedVector class. The
    only reason for it not to be a member method is that this way it doesn't
    have to be compiled into the library. And that's a gain, because the
    library should be compiled with optimization on, but this method should be
    compiled with debugging. */
void
CoinPackedVectorUnitTest();

#endif
