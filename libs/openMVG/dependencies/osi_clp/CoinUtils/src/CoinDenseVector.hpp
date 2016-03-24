/* $Id: CoinDenseVector.hpp 1372 2011-01-03 23:31:00Z lou $ */
// Copyright (C) 2003, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef CoinDenseVector_H
#define CoinDenseVector_H

#if defined(_MSC_VER)
// Turn off compiler warning about long names
#  pragma warning(disable:4786)
#endif

#include <cassert>
#include <cstdlib>
#include <cmath>
#include "CoinHelperFunctions.hpp"

//#############################################################################
/** A function that tests the methods in the CoinDenseVector class. The
    only reason for it not to be a member method is that this way it doesn't
    have to be compiled into the library. And that's a gain, because the
    library should be compiled with optimization on, but this method should be
    compiled with debugging. */
    template <typename T> void
    CoinDenseVectorUnitTest(T dummy);

//#############################################################################
/** Dense Vector

Stores a dense (or expanded) vector of floating point values.
Type of vector elements is controlled by templating.
(Some working quantities such as accumulated sums
are explicitly declared of type double). This allows the 
components of the vector integer, single or double precision.

Here is a sample usage:
@verbatim
    const int ne = 4;
    double el[ne] = { 10., 40., 1., 50. }

    // Create vector and set its value
    CoinDenseVector<double> r(ne,el);

    // access each element
    assert( r.getElements()[0]==10. );
    assert( r.getElements()[1]==40. );
    assert( r.getElements()[2]== 1. );
    assert( r.getElements()[3]==50. );

    // Test for equality
    CoinDenseVector<double> r1;
    r1=r;

    // Add dense vectors.
    // Similarly for subtraction, multiplication,
    // and division.
    CoinDenseVector<double> add = r + r1;
    assert( add[0] == 10.+10. );
    assert( add[1] == 40.+40. );
    assert( add[2] ==  1.+ 1. );
    assert( add[3] == 50.+50. );

    assert( r.sum() == 10.+40.+1.+50. );
@endverbatim
*/
template <typename T> class CoinDenseVector {
private:
   /**@name Private member data */
   //@{
   /// Size of element vector
   int nElements_;
   ///Vector elements
   T * elements_;
   //@}
  
public:
   /**@name Get methods. */
   //@{
   /// Get the size
   inline int getNumElements() const { return nElements_; }
   inline int size() const { return nElements_; }
   /// Get element values
   inline const T * getElements() const { return elements_; }
   /// Get element values
   inline T * getElements() { return elements_; }
   //@}
 
   //-------------------------------------------------------------------
   // Set indices and elements
   //------------------------------------------------------------------- 
   /**@name Set methods */
   //@{
   /// Reset the vector (i.e. set all elemenets to zero)
   void clear();
   /** Assignment operator */
   CoinDenseVector & operator=(const CoinDenseVector &);
   /** Member of array operator */
   T & operator[](int index) const;

   /** Set vector size, and elements.
       Size is the length of the elements vector.
       The element vector is copied into this class instance's
       member data. */ 
   void setVector(int size, const T * elems);

  
   /** Elements set to have the same scalar value */
   void setConstant(int size, T elems);
  

   /** Set an existing element in the dense vector
       The first argument is the "index" into the elements() array
   */
   void setElement(int index, T element);
   /** Resize the dense vector to be the first newSize elements.
       If length is decreased, vector is truncated. If increased
       new entries, set to new default element */
   void resize(int newSize, T fill=T()); 

   /** Append a dense vector to this dense vector */
   void append(const CoinDenseVector &);
   //@}

   /**@name norms, sum and scale */
   //@{
   /// 1-norm of vector
   inline T oneNorm() const {
     T norm = 0;
     for (int i=0; i<nElements_; i++)
       norm += CoinAbs(elements_[i]);
     return norm;
   }
   /// 2-norm of vector
   inline double twoNorm() const {
     double norm = 0.;
     for (int i=0; i<nElements_; i++)
       norm += elements_[i] * elements_[i];
     // std namespace removed because it was causing a compile
     // problem with Microsoft Visual C++
     return /*std::*/sqrt(norm);
   }
   /// infinity-norm of vector
   inline T infNorm() const {
     T norm = 0;
     for (int i=0; i<nElements_; i++)
       norm = CoinMax(norm, CoinAbs(elements_[i]));
     return norm;
   }
   /// sum of vector elements
   inline T sum() const {
     T sume = 0;
     for (int i=0; i<nElements_; i++)
       sume += elements_[i];
     return sume;
   }
   /// scale vector elements
   inline void scale(T factor) {
     for (int i=0; i<nElements_; i++)
       elements_[i] *= factor;
     return;
   }
   //@}

   /**@name Arithmetic operators. */
   //@{
   /// add <code>value</code> to every entry
   void operator+=(T value);
   /// subtract <code>value</code> from every entry
   void operator-=(T value);
   /// multiply every entry by <code>value</code>
   void operator*=(T value);
   /// divide every entry by <code>value</code>
   void operator/=(T value);
   //@}

   /**@name Constructors and destructors */
   //@{
   /** Default constructor */
   CoinDenseVector();
   /** Alternate Constructors - set elements to vector of Ts */
   CoinDenseVector(int size, const T * elems);
   /** Alternate Constructors - set elements to same scalar value */
   CoinDenseVector(int size, T element=T());
   /** Copy constructors */
   CoinDenseVector(const CoinDenseVector &);

    /** Destructor */
   ~CoinDenseVector ();
   //@}
    
private:
   /**@name Private methods */
   //@{  
   /// Copy internal data
   void gutsOfSetVector(int size, const T * elems);
   /// Set all elements to a given value
   void gutsOfSetConstant(int size, T value);
   //@}
};

//#############################################################################

/**@name Arithmetic operators on dense vectors.

   <strong>NOTE</strong>: Because these methods return an object (they can't
   return a reference, though they could return a pointer...) they are
   <em>very</em> inefficient...
 */
//@{
/// Return the sum of two dense vectors
template <typename T> inline
CoinDenseVector<T> operator+(const CoinDenseVector<T>& op1,
			     const CoinDenseVector<T>& op2){
  assert(op1.size() == op2.size());
  int size = op1.size();
  CoinDenseVector<T> op3(size);
  const T *elements1 = op1.getElements();
  const T *elements2 = op2.getElements();
  T *elements3 = op3.getElements();
  for(int i=0; i<size; i++)
    elements3[i] = elements1[i] + elements2[i];
  return op3;
}

/// Return the difference of two dense vectors
template <typename T> inline
CoinDenseVector<T> operator-(const CoinDenseVector<T>& op1,
			     const CoinDenseVector<T>& op2){
  assert(op1.size() == op2.size());
  int size = op1.size();
  CoinDenseVector<T> op3(size);
  const T *elements1 = op1.getElements();
  const T *elements2 = op2.getElements();
  T *elements3 = op3.getElements();
  for(int i=0; i<size; i++)
    elements3[i] = elements1[i] - elements2[i];
  return op3;
}


/// Return the element-wise product of two dense vectors
template <typename T> inline
CoinDenseVector<T> operator*(const CoinDenseVector<T>& op1,
			  const CoinDenseVector<T>& op2){
  assert(op1.size() == op2.size());
  int size = op1.size();
  CoinDenseVector<T> op3(size);
  const T *elements1 = op1.getElements();
  const T *elements2 = op2.getElements();
  T *elements3 = op3.getElements();
  for(int i=0; i<size; i++)
    elements3[i] = elements1[i] * elements2[i];
  return op3;
}

/// Return the element-wise ratio of two dense vectors
template <typename T> inline
CoinDenseVector<T> operator/(const CoinDenseVector<T>& op1,
			  const CoinDenseVector<T>& op2){
  assert(op1.size() == op2.size());
  int size = op1.size();
  CoinDenseVector<T> op3(size);
  const T *elements1 = op1.getElements();
  const T *elements2 = op2.getElements();
  T *elements3 = op3.getElements();
  for(int i=0; i<size; i++)
    elements3[i] = elements1[i] / elements2[i];
  return op3;
}
//@}

/**@name Arithmetic operators on dense vector and a constant. 
   These functions create a dense vector as a result. That dense vector will
   have the same indices as <code>op1</code> and the specified operation is
   done entry-wise with the given value. */
//@{
/// Return the sum of a dense vector and a constant
template <typename T> inline
CoinDenseVector<T> operator+(const CoinDenseVector<T>& op1, T value){
  int size = op1.size();
  CoinDenseVector<T> op3(size);
  const T *elements1 = op1.getElements();
  T *elements3 = op3.getElements();
  double dvalue = value;
  for(int i=0; i<size; i++)
    elements3[i] = elements1[i] + dvalue;
  return op3;
}

/// Return the difference of a dense vector and a constant
template <typename T> inline
CoinDenseVector<T> operator-(const CoinDenseVector<T>& op1, T value){
  int size = op1.size();
  CoinDenseVector<T> op3(size);
  const T *elements1 = op1.getElements();
  T *elements3 = op3.getElements();
  double dvalue = value;
  for(int i=0; i<size; i++)
    elements3[i] = elements1[i] - dvalue;
  return op3;
}

/// Return the element-wise product of a dense vector and a constant
template <typename T> inline
CoinDenseVector<T> operator*(const CoinDenseVector<T>& op1, T value){
  int size = op1.size();
  CoinDenseVector<T> op3(size);
  const T *elements1 = op1.getElements();
  T *elements3 = op3.getElements();
  double dvalue = value;
  for(int i=0; i<size; i++)
    elements3[i] = elements1[i] * dvalue;
  return op3;
}

/// Return the element-wise ratio of a dense vector and a constant
template <typename T> inline
CoinDenseVector<T> operator/(const CoinDenseVector<T>& op1, T value){
  int size = op1.size();
  CoinDenseVector<T> op3(size);
  const T *elements1 = op1.getElements();
  T *elements3 = op3.getElements();
  double dvalue = value;
  for(int i=0; i<size; i++)
    elements3[i] = elements1[i] / dvalue;
  return op3;
}

/// Return the sum of a constant and a dense vector
template <typename T> inline
CoinDenseVector<T> operator+(T value, const CoinDenseVector<T>& op1){
  int size = op1.size();
  CoinDenseVector<T> op3(size);
  const T *elements1 = op1.getElements();
  T *elements3 = op3.getElements();
  double dvalue = value;
  for(int i=0; i<size; i++)
    elements3[i] = elements1[i] + dvalue;
  return op3;
}

/// Return the difference of a constant and a dense vector
template <typename T> inline
CoinDenseVector<T> operator-(T value, const CoinDenseVector<T>& op1){
  int size = op1.size();
  CoinDenseVector<T> op3(size);
  const T *elements1 = op1.getElements();
  T *elements3 = op3.getElements();
  double dvalue = value;
  for(int i=0; i<size; i++)
    elements3[i] = dvalue - elements1[i];
  return op3;
}

/// Return the element-wise product of a constant and a dense vector
template <typename T> inline
CoinDenseVector<T> operator*(T value, const CoinDenseVector<T>& op1){
  int size = op1.size();
  CoinDenseVector<T> op3(size);
  const T *elements1 = op1.getElements();
  T *elements3 = op3.getElements();
  double dvalue = value;
  for(int i=0; i<size; i++)
    elements3[i] = elements1[i] * dvalue;
  return op3;
}

/// Return the element-wise ratio of a a constant and dense vector
template <typename T> inline
CoinDenseVector<T> operator/(T value, const CoinDenseVector<T>& op1){
  int size = op1.size();
  CoinDenseVector<T> op3(size);
  const T *elements1 = op1.getElements();
  T *elements3 = op3.getElements();
  double dvalue = value;
  for(int i=0; i<size; i++)
    elements3[i] = dvalue / elements1[i];
  return op3;
}
//@}

#endif
