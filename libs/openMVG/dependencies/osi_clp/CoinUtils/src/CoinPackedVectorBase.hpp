/* $Id: CoinPackedVectorBase.hpp 1416 2011-04-17 09:57:29Z stefan $ */
// Copyright (C) 2000, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef CoinPackedVectorBase_H
#define CoinPackedVectorBase_H

#include <set>
#include <map>
#include "CoinPragma.hpp"
#include "CoinError.hpp"

class CoinPackedVector;

/** Abstract base class for various sparse vectors.

    Since this class is abstract, no object of this type can be created. The
    sole purpose of this class is to provide access to a <em>constant</em>
    packed vector. All members of this class are const methods, they can't
    change the object. */

class CoinPackedVectorBase  {
  
public:
   /**@name Virtual methods that the derived classes must provide */
   //@{
   /// Get length of indices and elements vectors
   virtual int getNumElements() const = 0;
   /// Get indices of elements
   virtual const int * getIndices() const = 0;
   /// Get element values
   virtual const double * getElements() const = 0;
   //@}

   /**@name Methods related to whether duplicate-index checking is performed.

       If the checking for duplicate indices is turned off, then
       some CoinPackedVector methods may not work correctly if there
       are duplicate indices.
       Turning off the checking for duplicate indices may result in
       better run time performance.
      */
   //@{
   /** \brief Set to the argument value whether to test for duplicate indices
	      in the vector whenever they can occur.
       
       Calling this method with \p test set to true will trigger an immediate
       check for duplicate indices.
   */
   void setTestForDuplicateIndex(bool test) const;
   /** \brief Set to the argument value whether to test for duplicate indices
	      in the vector whenever they can occur BUT we know that right
	      now the vector has no duplicate indices.

       Calling this method with \p test set to true will <em>not</em> trigger
       an immediate check for duplicate indices; instead, it's assumed that
       the result of the test will be true.
   */
   void setTestForDuplicateIndexWhenTrue(bool test) const;
   /** Returns true if the vector should be tested for duplicate indices when
       they can occur. */
   bool testForDuplicateIndex() const { return testForDuplicateIndex_; }
   /// Just sets test stuff false without a try etc
   inline void setTestsOff() const
   { testForDuplicateIndex_=false; testedDuplicateIndex_=false;}
   //@}

   /**@name Methods for getting info on the packed vector as a full vector */
   //@{
   /** Get the vector as a dense vector. The argument specifies how long this
       dense vector is. <br>
       <strong>NOTE</strong>: The user needs to <code>delete[]</code> this
       pointer after it's not needed anymore.
   */
   double * denseVector(int denseSize) const;
   /** Access the i'th element of the full storage vector.
       If the i'th is not stored, then zero is returned. The initial use of
       this method has some computational and storage overhead associated with
       it.<br>
       <strong>NOTE</strong>: This is <em>very</em> expensive. It is probably
       much better to use <code>denseVector()</code>.
   */
   double operator[](int i) const; 
   //@}

   /**@name Index methods */
   //@{
   /// Get value of maximum index
   int getMaxIndex() const;
   /// Get value of minimum index
   int getMinIndex() const;

   /// Throw an exception if there are duplicate indices
   void duplicateIndex(const char* methodName = NULL,
		       const char * className = NULL) const;

   /** Return true if the i'th element of the full storage vector exists in
       the packed storage vector.*/
   bool isExistingIndex(int i) const;
   
   /** Return the position of the i'th element of the full storage vector.
       If index does not exist then -1 is returned  */
   int findIndex(int i) const;
 
   //@}
  
   /**@name Comparison operators on two packed vectors */
   //@{
   /** Equal. Returns true if vectors have same length and corresponding
       element of each vector is equal. */
   bool operator==(const CoinPackedVectorBase & rhs) const;
   /// Not equal
   bool operator!=(const CoinPackedVectorBase & rhs) const;

#if 0
   // LL: This should be implemented eventually. It is useful to have.
   /** Lexicographic comparisons of two packed vectors. Returns
       negative/0/positive depending on whether \c this is
       smaller/equal.greater than \c rhs */
   int lexCompare(const CoinPackedVectorBase& rhs);
#endif
  
   /** This method establishes an ordering on packed vectors. It is complete
       ordering, but not the same as lexicographic ordering. However, it is
       quick and dirty to compute and thus it is useful to keep packed vectors
       in a heap when all we care is to quickly check whether a particular
       vector is already in the heap or not. Returns negative/0/positive
       depending on whether \c this is smaller/equal.greater than \c rhs. */
   int compare(const CoinPackedVectorBase& rhs) const;

   /** equivalent - If shallow packed vector A & B are equivalent, then they
       are still equivalent no matter how they are sorted.
       In this method the FloatEqual function operator can be specified. The
       default equivalence test is that the entries are relatively equal.<br> 
       <strong>NOTE</strong>: This is a relatively expensive method as it
       sorts the two shallow packed vectors.
   */
   template <class FloatEqual> bool
   isEquivalent(const CoinPackedVectorBase& rhs, const FloatEqual& eq) const
   {
      if (getNumElements() != rhs.getNumElements())
	 return false;

      duplicateIndex("equivalent", "CoinPackedVector");
      rhs.duplicateIndex("equivalent", "CoinPackedVector");

      std::map<int,double> mv;
      const int * inds = getIndices();
      const double * elems = getElements();
      int i;
      for ( i = getNumElements() - 1; i >= 0; --i) {
	 mv.insert(std::make_pair(inds[i], elems[i]));
      }

      std::map<int,double> mvRhs;
      inds = rhs.getIndices();
      elems = rhs.getElements();
      for ( i = getNumElements() - 1; i >= 0; --i) {
	 mvRhs.insert(std::make_pair(inds[i], elems[i]));
      }

      std::map<int,double>::const_iterator mvI = mv.begin();
      std::map<int,double>::const_iterator mvIlast = mv.end();
      std::map<int,double>::const_iterator mvIrhs = mvRhs.begin();
      while (mvI != mvIlast) {
	 if (mvI->first != mvIrhs->first || ! eq(mvI->second, mvIrhs->second))
	    return false;
	 ++mvI;
	 ++mvIrhs;
      }
      return true;
   }

   bool isEquivalent(const CoinPackedVectorBase& rhs) const;
   //@}


   /**@name Arithmetic operators. */
   //@{
   /// Create the dot product with a full vector
   double dotProduct(const double* dense) const;

   /// Return the 1-norm of the vector
   double oneNorm() const;

   /// Return the square of the 2-norm of the vector
   double normSquare() const;

   /// Return the 2-norm of the vector
   double twoNorm() const;

   /// Return the infinity-norm of the vector
   double infNorm() const;

   /// Sum elements of vector.
   double sum() const;
   //@}

protected:

   /**@name Constructors, destructor
      <strong>NOTE</strong>: All constructors are protected. There's no need
      to expose them, after all, this is an abstract class. */
   //@{
   /** Default constructor. */
   CoinPackedVectorBase();

public:
   /** Destructor */
   virtual ~CoinPackedVectorBase();
   //@}

private:
   /**@name Disabled methods */
   //@{
   /** The copy constructor. <br>
       This must be at least protected, but we make it private. The reason is
       that when, say, a shallow packed vector is created, first the
       underlying class, it this one is constructed. However, at that point we
       don't know how much of the data members of this class we need to copy
       over. Therefore the copy constructor is not used. */
   CoinPackedVectorBase(const CoinPackedVectorBase&);
   /** This class provides <em>const</em> access to packed vectors, so there's
       no need to provide an assignment operator. */
   CoinPackedVectorBase& operator=(const CoinPackedVectorBase&);
   //@}
   
protected:
    
   /**@name Protected methods */
   //@{      
   /// Find Maximum and Minimum Indices
   void findMaxMinIndices() const;

   /// Return indexSetPtr_ (create it if necessary).
   std::set<int> * indexSet(const char* methodName = NULL,
			    const char * className = NULL) const;

   /// Delete the indexSet
   void clearIndexSet() const;
   void clearBase() const;
   void copyMaxMinIndex(const CoinPackedVectorBase & x) const {
      maxIndex_ = x.maxIndex_;
      minIndex_ = x.minIndex_;
   }
   //@}
    
private:
   /**@name Protected member data */
   //@{
   /// Contains max index value or -infinity
   mutable int maxIndex_;
   /// Contains minimum index value or infinity
   mutable int minIndex_;
   /** Store the indices in a set. This set is only created if it is needed.
       Its primary use is testing for duplicate indices.
    */
   mutable std::set<int> * indexSetPtr_;
   /** True if the vector should be tested for duplicate indices when they can
       occur. */
   mutable bool testForDuplicateIndex_;
   /** True if the vector has already been tested for duplicate indices. Most
       of the operations in CoinPackedVector preserves this flag. */
   mutable bool testedDuplicateIndex_;
   //@}
};

#endif
