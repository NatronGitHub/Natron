/* $Id: CoinShallowPackedVector.hpp 1498 2011-11-02 15:25:35Z mjs $ */
// Copyright (C) 2000, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef CoinShallowPackedVector_H
#define CoinShallowPackedVector_H

#if defined(_MSC_VER)
// Turn off compiler warning about long names
#  pragma warning(disable:4786)
#endif

#include "CoinError.hpp"
#include "CoinPackedVectorBase.hpp"

/** Shallow Sparse Vector
 
This class is for sparse vectors where the indices and 
elements are stored elsewhere.  This class only maintains
pointers to the indices and elements.  Since this class
does not own the index and element data it provides
read only access to to the data.  An CoinSparsePackedVector
must be used when the sparse vector's data will be altered.

This class stores pointers to the vectors.
It does not actually contain the vectors.

Here is a sample usage:
@verbatim
   const int ne = 4; 
   int inx[ne] =   {  1,   4,  0,   2 }; 
   double el[ne] = { 10., 40., 1., 50. }; 
 
   // Create vector and set its value 
   CoinShallowPackedVector r(ne,inx,el); 
 
   // access each index and element 
   assert( r.indices ()[0]== 1  ); 
   assert( r.elements()[0]==10. ); 
   assert( r.indices ()[1]== 4  ); 
   assert( r.elements()[1]==40. ); 
   assert( r.indices ()[2]== 0  ); 
   assert( r.elements()[2]== 1. ); 
   assert( r.indices ()[3]== 2  ); 
   assert( r.elements()[3]==50. ); 
 
   // access as a full storage vector 
   assert( r[ 0]==1. ); 
   assert( r[ 1]==10.); 
   assert( r[ 2]==50.); 
   assert( r[ 3]==0. ); 
   assert( r[ 4]==40.); 
 
   // Tests for equality and equivalence
   CoinShallowPackedVector r1; 
   r1=r; 
   assert( r==r1 ); 
   r.sort(CoinIncrElementOrdered()); 
   assert( r!=r1 ); 
 
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
class CoinShallowPackedVector : public CoinPackedVectorBase {
   friend void CoinShallowPackedVectorUnitTest();

public:
  
   /**@name Get methods */
   //@{
   /// Get length of indices and elements vectors
   virtual int getNumElements() const { return nElements_; }
   /// Get indices of elements
   virtual const int * getIndices() const { return indices_; }
   /// Get element values
   virtual const double * getElements() const { return elements_; }
   //@}

   /**@name Set methods */
   //@{
   /// Reset the vector (as if were just created an empty vector)
   void clear();
   /** Assignment operator. */
   CoinShallowPackedVector& operator=(const CoinShallowPackedVector & x);
   /** Assignment operator from a CoinPackedVectorBase. */
   CoinShallowPackedVector& operator=(const CoinPackedVectorBase & x);
   /** just like the explicit constructor */
   void setVector(int size, const int * indices, const double * elements,
		  bool testForDuplicateIndex = true);
   //@}

   /**@name Methods to create, set and destroy */
   //@{
   /** Default constructor. */
   CoinShallowPackedVector(bool testForDuplicateIndex = true);
   /** Explicit Constructor.
       Set vector size, indices, and elements. Size is the length of both the
       indices and elements vectors. The indices and elements vectors are not
       copied into this class instance. The ShallowPackedVector only maintains
       the pointers to the indices and elements vectors. <br>
       The last argument specifies whether the creator of the object knows in
       advance that there are no duplicate indices.
   */
   CoinShallowPackedVector(int size,
			  const int * indices, const double * elements,
			  bool testForDuplicateIndex = true);
   /** Copy constructor from the base class. */
   CoinShallowPackedVector(const CoinPackedVectorBase &);
   /** Copy constructor. */
   CoinShallowPackedVector(const CoinShallowPackedVector &);
   /** Destructor. */
   virtual ~CoinShallowPackedVector() {}
   /// Print vector information.
   void print();
   //@}

private:
   /**@name Private member data */
   //@{
   /// Vector indices
   const int * indices_;
   ///Vector elements
   const double * elements_;
   /// Size of indices and elements vectors
   int nElements_;
   //@}
};

//#############################################################################
/** A function that tests the methods in the CoinShallowPackedVector class. The
    only reason for it not to be a member method is that this way it doesn't
    have to be compiled into the library. And that's a gain, because the
    library should be compiled with optimization on, but this method should be
    compiled with debugging. */
void
CoinShallowPackedVectorUnitTest();

#endif
