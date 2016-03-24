/* $Id: CoinIndexedVector.hpp 1554 2012-10-31 16:52:28Z forrest $ */
// Copyright (C) 2000, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef CoinIndexedVector_H
#define CoinIndexedVector_H

#if defined(_MSC_VER)
// Turn off compiler warning about long names
#  pragma warning(disable:4786)
#endif

#include <map>
#include "CoinFinite.hpp"
#ifndef CLP_NO_VECTOR
#include "CoinPackedVectorBase.hpp"
#endif
#include "CoinSort.hpp"
#include "CoinHelperFunctions.hpp"
#include <cassert>

#ifndef COIN_FLOAT
#define COIN_INDEXED_TINY_ELEMENT 1.0e-50
#define COIN_INDEXED_REALLY_TINY_ELEMENT 1.0e-100
#else
#define COIN_INDEXED_TINY_ELEMENT 1.0e-35
#define COIN_INDEXED_REALLY_TINY_ELEMENT 1.0e-39
#endif

/** Indexed Vector

This stores values unpacked but apart from that is a bit like CoinPackedVector.
It is designed to be lightweight in normal use.

It now has a "packed" mode when it is even more like CoinPackedVector

Indices array has capacity_ extra chars which are zeroed and can
be used for any purpose - but must be re-zeroed

Stores vector of indices and associated element values.
Supports sorting of indices.  

Does not support negative indices.

Does NOT support testing for duplicates

*** getElements is no longer supported

Here is a sample usage:
@verbatim
    const int ne = 4;
    int inx[ne] =   {  1,   4,  0,   2 }
    double el[ne] = { 10., 40., 1., 50. }

    // Create vector and set its valuex1
    CoinIndexedVector r(ne,inx,el);

    // access as a full storage vector
    assert( r[ 0]==1. );
    assert( r[ 1]==10.);
    assert( r[ 2]==50.);
    assert( r[ 3]==0. );
    assert( r[ 4]==40.);

    // sort Elements in increasing order
    r.sortIncrElement();

    // access each index and element
    assert( r.getIndices ()[0]== 0  );
    assert( r.getIndices ()[1]== 1  );
    assert( r.getIndices ()[2]== 4  );
    assert( r.getIndices ()[3]== 2  );

    // access as a full storage vector
    assert( r[ 0]==1. );
    assert( r[ 1]==10.);
    assert( r[ 2]==50.);
    assert( r[ 3]==0. );
    assert( r[ 4]==40.);

    // Tests for equality and equivalence
    CoinIndexedVector r1;
    r1=r;
    assert( r==r1 );
    assert( r.equivalent(r1) );
    r.sortIncrElement();
    assert( r!=r1 );
    assert( r.equivalent(r1) );

    // Add indexed vectors.
    // Similarly for subtraction, multiplication,
    // and division.
    CoinIndexedVector add = r + r1;
    assert( add[0] ==  1.+ 1. );
    assert( add[1] == 10.+10. );
    assert( add[2] == 50.+50. );
    assert( add[3] ==  0.+ 0. );
    assert( add[4] == 40.+40. );

    assert( r.sum() == 10.+40.+1.+50. );
@endverbatim
*/
class CoinIndexedVector {
   friend void CoinIndexedVectorUnitTest();
  
public:
   /**@name Get methods. */
   //@{
   /// Get the size
   inline int getNumElements() const { return nElements_; }
   /// Get indices of elements
   inline const int * getIndices() const { return indices_; }
   /// Get element values
   // ** No longer supported virtual const double * getElements() const ;
   /// Get indices of elements
   inline int * getIndices() { return indices_; }
   /** Get the vector as a dense vector. This is normal storage method.
       The user should not not delete [] this.
   */
   inline double * denseVector() const { return elements_; }
   /// For very temporary use when user needs to borrow a dense vector
  inline void setDenseVector(double * array)
  { elements_ = array;}
   /// For very temporary use when user needs to borrow an index vector
  inline void setIndexVector(int * array)
  { indices_ = array;}
   /** Access the i'th element of the full storage vector.
   */
   double & operator[](int i) const; 

   //@}
 
   //-------------------------------------------------------------------
   // Set indices and elements
   //------------------------------------------------------------------- 
   /**@name Set methods */
   //@{
   /// Set the size
   inline void setNumElements(int value) { nElements_ = value;
   if (!nElements_) packedMode_=false;}
   /// Reset the vector (as if were just created an empty vector).  This leaves arrays!
   void clear();
   /// Reset the vector (as if were just created an empty vector)
   void empty();
   /** Assignment operator. */
   CoinIndexedVector & operator=(const CoinIndexedVector &);
#ifndef CLP_NO_VECTOR
   /** Assignment operator from a CoinPackedVectorBase. <br>
   <strong>NOTE</strong>: This assumes no duplicates */
   CoinIndexedVector & operator=(const CoinPackedVectorBase & rhs);
#endif
   /** Copy the contents of one vector into another.  If multiplier is 1
       It is the equivalent of = but if vectors are same size does
       not re-allocate memory just clears and copies */
   void copy(const CoinIndexedVector & rhs, double multiplier=1.0);

   /** Borrow ownership of the arguments to this vector.
       Size is the length of the unpacked elements vector. */
  void borrowVector(int size, int numberIndices, int* inds, double* elems);

   /** Return ownership of the arguments to this vector.
       State after is empty .
   */
   void returnVector();

   /** Set vector numberIndices, indices, and elements.
       NumberIndices is the length of both the indices and elements vectors.
       The indices and elements vectors are copied into this class instance's
       member data. Assumed to have no duplicates */
  void setVector(int numberIndices, const int * inds, const double * elems);
  
   /** Set vector size, indices, and elements.
       Size is the length of the unpacked elements vector.
       The indices and elements vectors are copied into this class instance's
       member data. We do not check for duplicate indices */
   void setVector(int size, int numberIndices, const int * inds, const double * elems);
  
   /** Elements set to have the same scalar value */
  void setConstant(int size, const int * inds, double elems);
  
   /** Indices are not specified and are taken to be 0,1,...,size-1 */
  void setFull(int size, const double * elems);

   /** Set an existing element in the indexed vector
       The first argument is the "index" into the elements() array
   */
   void setElement(int index, double element);

   /// Insert an element into the vector
   void insert(int index, double element);
   /// Insert a nonzero element into the vector
   inline void quickInsert(int index, double element)
               {
		 assert (!elements_[index]);
		 indices_[nElements_++] = index;
		 assert (nElements_<=capacity_);
		 elements_[index] = element;
	       }
   /** Insert or if exists add an element into the vector
       Any resulting zero elements will be made tiny */
   void add(int index, double element);
   /** Insert or if exists add an element into the vector
       Any resulting zero elements will be made tiny.
       This version does no checking */
   inline void quickAdd(int index, double element)
               {
		 if (elements_[index]) {
		   element += elements_[index];
		   if ((element > 0 ? element : -element) >= COIN_INDEXED_TINY_ELEMENT) {
		     elements_[index] = element;
		   } else {
		     elements_[index] = 1.0e-100;
		   }
		 } else if ((element > 0 ? element : -element) >= COIN_INDEXED_TINY_ELEMENT) {
		   indices_[nElements_++] = index;
		   assert (nElements_<=capacity_);
		   elements_[index] = element;
		 }
	       }
   /** Insert or if exists add an element into the vector
       Any resulting zero elements will be made tiny.
       This knows element is nonzero
       This version does no checking */
   inline void quickAddNonZero(int index, double element)
               {
		 assert (element);
		 if (elements_[index]) {
		   element += elements_[index];
		   if ((element > 0 ? element : -element) >= COIN_INDEXED_TINY_ELEMENT) {
		     elements_[index] = element;
		   } else {
		     elements_[index] = COIN_DBL_MIN;
		   }
		 } else {
		   indices_[nElements_++] = index;
		   assert (nElements_<=capacity_);
		   elements_[index] = element;
		 }
	       }
   /** Makes nonzero tiny.
       This version does no checking */
   inline void zero(int index)
               {
		 if (elements_[index]) 
		   elements_[index] = COIN_DBL_MIN;
	       }
   /** set all small values to zero and return number remaining
      - < tolerance => 0.0 */
   int clean(double tolerance);
   /// Same but packs down
   int cleanAndPack(double tolerance);
   /// Same but packs down and is safe (i.e. if order is odd)
   int cleanAndPackSafe(double tolerance);
  /// Mark as packed
  inline void setPacked()
  { packedMode_ = true;}
#ifndef NDEBUG
   /// For debug check vector is clear i.e. no elements
   void checkClear();
   /// For debug check vector is clean i.e. elements match indices
   void checkClean();
#else
  inline void checkClear() {};
  inline void checkClean() {};
#endif
   /// Scan dense region and set up indices (returns number found)
   int scan();
   /** Scan dense region from start to < end and set up indices
       returns number found
   */
   int scan(int start, int end);
  /** Scan dense region and set up indices (returns number found).
      Only ones >= tolerance */
   int scan(double tolerance);
   /** Scan dense region from start to < end and set up indices
       returns number found.  Only >= tolerance
   */
   int scan(int start, int end, double tolerance);
   /// These are same but pack down
   int scanAndPack();
   int scanAndPack(int start, int end);
   int scanAndPack(double tolerance);
   int scanAndPack(int start, int end, double tolerance);
   /// Create packed array
   void createPacked(int number, const int * indices, 
		    const double * elements);
   /// Create unpacked array
   void createUnpacked(int number, const int * indices, 
		    const double * elements);
   /// Create unpacked singleton
   void createOneUnpackedElement(int index, double element);
   /// This is mainly for testing - goes from packed to indexed
   void expand();
#ifndef CLP_NO_VECTOR
   /// Append a CoinPackedVector to the end
   void append(const CoinPackedVectorBase & caboose);
#endif
   /// Append a CoinIndexedVector to the end (with extra space)
   void append(const CoinIndexedVector & caboose);
   /// Append a CoinIndexedVector to the end and modify indices
  void append(CoinIndexedVector & other,int adjustIndex,bool zapElements=false);

   /// Swap values in positions i and j of indices and elements
   void swap(int i, int j); 

   /// Throw away all entries in rows >= newSize
   void truncate(int newSize); 
   ///  Print out
   void print() const;
   //@}
   /**@name Arithmetic operators. */
   //@{
   /// add <code>value</code> to every entry
   void operator+=(double value);
   /// subtract <code>value</code> from every entry
   void operator-=(double value);
   /// multiply every entry by <code>value</code>
   void operator*=(double value);
   /// divide every entry by <code>value</code> (** 0 vanishes)
   void operator/=(double value);
   //@}

   /**@name Comparison operators on two indexed vectors */
   //@{
#ifndef CLP_NO_VECTOR
   /** Equal. Returns true if vectors have same length and corresponding
       element of each vector is equal. */
   bool operator==(const CoinPackedVectorBase & rhs) const;
   /// Not equal
   bool operator!=(const CoinPackedVectorBase & rhs) const;
#endif
   /** Equal. Returns true if vectors have same length and corresponding
       element of each vector is equal. */
   bool operator==(const CoinIndexedVector & rhs) const;
   /// Not equal
   bool operator!=(const CoinIndexedVector & rhs) const;
   /// Equal with a tolerance (returns -1 or position of inequality). 
   int isApproximatelyEqual(const CoinIndexedVector & rhs, double tolerance=1.0e-8) const;
   //@}

   /**@name Index methods */
   //@{
   /// Get value of maximum index
   int getMaxIndex() const;
   /// Get value of minimum index
   int getMinIndex() const;
   //@}


   /**@name Sorting */
   //@{ 
   /** Sort the indexed storage vector (increasing indices). */
   void sort()
   { std::sort(indices_,indices_+nElements_); }

   void sortIncrIndex()
   { std::sort(indices_,indices_+nElements_); }

   void sortDecrIndex();
  
   void sortIncrElement();

   void sortDecrElement();
   void sortPacked();

   //@}

  //#############################################################################

  /**@name Arithmetic operators on packed vectors.

   <strong>NOTE</strong>: These methods operate on those positions where at
   least one of the arguments has a value listed. At those positions the
   appropriate operation is executed, Otherwise the result of the operation is
   considered 0.<br>
   <strong>NOTE 2</strong>: Because these methods return an object (they can't
   return a reference, though they could return a pointer...) they are
   <em>very</em> inefficient...
 */
//@{
/// Return the sum of two indexed vectors
CoinIndexedVector operator+(
			   const CoinIndexedVector& op2);

/// Return the difference of two indexed vectors
CoinIndexedVector operator-(
			   const CoinIndexedVector& op2);

/// Return the element-wise product of two indexed vectors
CoinIndexedVector operator*(
			   const CoinIndexedVector& op2);

/// Return the element-wise ratio of two indexed vectors (0.0/0.0 => 0.0) (0 vanishes)
CoinIndexedVector operator/(
			   const CoinIndexedVector& op2);
/// The sum of two indexed vectors
void operator+=(const CoinIndexedVector& op2);

/// The difference of two indexed vectors
void operator-=( const CoinIndexedVector& op2);

/// The element-wise product of two indexed vectors
void operator*=(const CoinIndexedVector& op2);

/// The element-wise ratio of two indexed vectors (0.0/0.0 => 0.0) (0 vanishes)
void operator/=(const CoinIndexedVector& op2);
//@}

   /**@name Memory usage */
   //@{
   /** Reserve space.
       If one knows the eventual size of the indexed vector,
       then it may be more efficient to reserve the space.
   */
   void reserve(int n);
   /** capacity returns the size which could be accomodated without
       having to reallocate storage.
   */
   int capacity() const { return capacity_; }
   /// Sets packed mode
   inline void setPackedMode(bool yesNo)
   { packedMode_=yesNo;}
   /// Gets packed mode
   inline bool packedMode() const
   { return packedMode_;}
   //@}

   /**@name Constructors and destructors */
   //@{
   /** Default constructor */
   CoinIndexedVector();
   /** Alternate Constructors - set elements to vector of doubles */
  CoinIndexedVector(int size, const int * inds, const double * elems);
   /** Alternate Constructors - set elements to same scalar value */
  CoinIndexedVector(int size, const int * inds, double element);
   /** Alternate Constructors - construct full storage with indices 0 through
       size-1. */
  CoinIndexedVector(int size, const double * elements);
   /** Alternate Constructors - just size */
  CoinIndexedVector(int size);
   /** Copy constructor. */
   CoinIndexedVector(const CoinIndexedVector &);
   /** Copy constructor.2 */
   CoinIndexedVector(const CoinIndexedVector *);
#ifndef CLP_NO_VECTOR
   /** Copy constructor <em>from a PackedVectorBase</em>. */
   CoinIndexedVector(const CoinPackedVectorBase & rhs);
#endif
   /** Destructor */
   ~CoinIndexedVector ();
   //@}
    
private:
   /**@name Private methods */
   //@{  
   /// Copy internal data
   void gutsOfSetVector(int size,
			const int * inds, const double * elems);
   void gutsOfSetVector(int size, int numberIndices,
			const int * inds, const double * elems);
   void gutsOfSetPackedVector(int size, int numberIndices,
			const int * inds, const double * elems);
   ///
   void gutsOfSetConstant(int size,
			  const int * inds, double value);
   //@}

protected:
   /**@name Private member data */
   //@{
   /// Vector indices
   int * indices_;
   ///Vector elements
   double * elements_;
   /// Size of indices and packed elements vectors
   int nElements_;
   /// Amount of memory allocated for indices_, and elements_.
   int capacity_;
   ///  Offset to get where new allocated array
   int offset_;
   /// If true then is operating in packed mode
   bool packedMode_;
   //@}
};

//#############################################################################
/** A function that tests the methods in the CoinIndexedVector class. The
    only reason for it not to be a member method is that this way it doesn't
    have to be compiled into the library. And that's a gain, because the
    library should be compiled with optimization on, but this method should be
    compiled with debugging. */
void
CoinIndexedVectorUnitTest();
/** Pointer with length in bytes
    
    This has a pointer to an array and the number of bytes in array.
    If number of bytes==-1 then
    CoinConditionalNew deletes existing pointer and returns new pointer
    of correct size (and number bytes still -1).
    CoinConditionalDelete deletes existing pointer and NULLs it.
    So behavior is as normal (apart from New deleting pointer which will have
    no effect with good coding practices.
    If number of bytes >=0 then
    CoinConditionalNew just returns existing pointer if array big enough
    otherwise deletes existing pointer, allocates array with spare 1%+64 bytes
    and updates number of bytes
    CoinConditionalDelete sets number of bytes = -size-2 and then array 
    returns NULL
*/
class CoinArrayWithLength {
  
public:
  /**@name Get methods. */
  //@{
  /// Get the size
  inline int getSize() const 
  { return size_; }
  /// Get the size
  inline int rawSize() const 
  { return size_; }
  /// See if persistence already on
  inline bool switchedOn() const 
  { return size_!=-1; }
  /// Get the capacity (just read it)
  inline int capacity() const 
  { return (size_>-2) ? size_ : (-size_)-2; }
  /// Set the capacity to >=0 if <=-2
  inline void setCapacity() 
  { if (size_<=-2) size_ = (-size_)-2; }
  /// Get Array
  inline const char * array() const 
  { return (size_>-2) ? array_ : NULL; }
  //@}
  
  /**@name Set methods */
  //@{
  /// Set the size
  inline void setSize(int value) 
  { size_ = value; }
  /// Set the size to -1
  inline void switchOff() 
  { size_ = -1; }
  /// Set the size to -2 and alignment
  inline void switchOn(int alignment=3) 
  { size_ = -2; alignment_=alignment;}
  /// Does what is needed to set persistence
  void setPersistence(int flag,int currentLength);
  /// Zero out array
  void clear();
  /// Swaps memory between two members
  void swap(CoinArrayWithLength & other);
  /// Extend a persistent array keeping data (size in bytes)
  void extend(int newSize);
  //@}
  
  /**@name Condition methods */
  //@{
  /// Conditionally gets new array
  char * conditionalNew(long sizeWanted); 
  /// Conditionally deletes
  void conditionalDelete();
  //@}
  
  /**@name Constructors and destructors */
  //@{
  /** Default constructor - NULL*/
  inline CoinArrayWithLength()
    : array_(NULL),size_(-1),offset_(0),alignment_(0)
  { }
  /** Alternate Constructor - length in bytes - size_ -1 */
  inline CoinArrayWithLength(int size)
    : size_(-1),offset_(0),alignment_(0)
  { array_=new char [size];}
  /** Alternate Constructor - length in bytes 
      mode -  0 size_ set to size
      mode>0 size_ set to size and zeroed
      if size<=0 just does alignment
      If abs(mode) >2 then align on that as power of 2
  */
  CoinArrayWithLength(int size, int mode);
  /** Copy constructor. */
  CoinArrayWithLength(const CoinArrayWithLength & rhs);
  /** Copy constructor.2 */
  CoinArrayWithLength(const CoinArrayWithLength * rhs);
  /** Assignment operator. */
  CoinArrayWithLength& operator=(const CoinArrayWithLength & rhs);
  /** Assignment with length (if -1 use internal length) */
  void copy(const CoinArrayWithLength & rhs, int numberBytes=-1);
  /** Assignment with length - does not copy */
  void allocate(const CoinArrayWithLength & rhs, int numberBytes);
  /** Destructor */
  ~CoinArrayWithLength ();
  /// Get array with alignment
  void getArray(int size);
  /// Really get rid of array with alignment
  void reallyFreeArray();
  /// Get enough space (if more needed then do at least needed)
  void getCapacity(int numberBytes,int numberIfNeeded=-1);
  //@}
  
protected:
  /**@name Private member data */
  //@{
  /// Array
  char * array_;
  /// Size of array in bytes
  CoinBigIndex size_;
  /// Offset of array
  int offset_;
  /// Alignment wanted (power of 2)
  int alignment_;
  //@}
};
/// double * version

class CoinDoubleArrayWithLength : public CoinArrayWithLength {
  
public:
  /**@name Get methods. */
  //@{
  /// Get the size
  inline int getSize() const 
  { return size_/CoinSizeofAsInt(double); }
  /// Get Array
  inline double * array() const 
  { return reinterpret_cast<double *> ((size_>-2) ? array_ : NULL); }
  //@}
  
  /**@name Set methods */
  //@{
  /// Set the size
  inline void setSize(int value) 
  { size_ = value*CoinSizeofAsInt(double); }
  //@}
  
  /**@name Condition methods */
  //@{
  /// Conditionally gets new array
  inline double * conditionalNew(int sizeWanted)
  { return reinterpret_cast<double *> ( CoinArrayWithLength::conditionalNew(sizeWanted>=0 ? static_cast<long> ((sizeWanted)*CoinSizeofAsInt(double)) : -1)); }
  //@}
  
  /**@name Constructors and destructors */
  //@{
  /** Default constructor - NULL*/
  inline CoinDoubleArrayWithLength()
  { array_=NULL; size_=-1;}
  /** Alternate Constructor - length in bytes - size_ -1 */
  inline CoinDoubleArrayWithLength(int size)
  { array_=new char [size*CoinSizeofAsInt(double)]; size_=-1;}
  /** Alternate Constructor - length in bytes 
      mode -  0 size_ set to size
      1 size_ set to size and zeroed
  */
  inline CoinDoubleArrayWithLength(int size, int mode)
    : CoinArrayWithLength(size*CoinSizeofAsInt(double),mode) {}
  /** Copy constructor. */
  inline CoinDoubleArrayWithLength(const CoinDoubleArrayWithLength & rhs)
    : CoinArrayWithLength(rhs) {}
  /** Copy constructor.2 */
  inline CoinDoubleArrayWithLength(const CoinDoubleArrayWithLength * rhs)
    : CoinArrayWithLength(rhs) {}
  /** Assignment operator. */
  inline CoinDoubleArrayWithLength& operator=(const CoinDoubleArrayWithLength & rhs)
  { CoinArrayWithLength::operator=(rhs);  return *this;}
  //@}
};
/// CoinFactorizationDouble * version

class CoinFactorizationDoubleArrayWithLength : public CoinArrayWithLength {
  
public:
  /**@name Get methods. */
  //@{
  /// Get the size
  inline int getSize() const 
  { return size_/CoinSizeofAsInt(CoinFactorizationDouble); }
  /// Get Array
  inline CoinFactorizationDouble * array() const 
  { return reinterpret_cast<CoinFactorizationDouble *> ((size_>-2) ? array_ : NULL); }
  //@}
  
  /**@name Set methods */
  //@{
  /// Set the size
  inline void setSize(int value) 
  { size_ = value*CoinSizeofAsInt(CoinFactorizationDouble); }
  //@}
  
  /**@name Condition methods */
  //@{
  /// Conditionally gets new array
  inline CoinFactorizationDouble * conditionalNew(int sizeWanted)
  { return reinterpret_cast<CoinFactorizationDouble *> (CoinArrayWithLength::conditionalNew(sizeWanted>=0 ? static_cast<long> (( sizeWanted)*CoinSizeofAsInt(CoinFactorizationDouble)) : -1)); }
  //@}
  
  /**@name Constructors and destructors */
  //@{
  /** Default constructor - NULL*/
  inline CoinFactorizationDoubleArrayWithLength()
  { array_=NULL; size_=-1;}
  /** Alternate Constructor - length in bytes - size_ -1 */
  inline CoinFactorizationDoubleArrayWithLength(int size)
  { array_=new char [size*CoinSizeofAsInt(CoinFactorizationDouble)]; size_=-1;}
  /** Alternate Constructor - length in bytes 
      mode -  0 size_ set to size
      1 size_ set to size and zeroed
  */
  inline CoinFactorizationDoubleArrayWithLength(int size, int mode)
    : CoinArrayWithLength(size*CoinSizeofAsInt(CoinFactorizationDouble),mode) {}
  /** Copy constructor. */
  inline CoinFactorizationDoubleArrayWithLength(const CoinFactorizationDoubleArrayWithLength & rhs)
    : CoinArrayWithLength(rhs) {}
  /** Copy constructor.2 */
  inline CoinFactorizationDoubleArrayWithLength(const CoinFactorizationDoubleArrayWithLength * rhs)
    : CoinArrayWithLength(rhs) {}
  /** Assignment operator. */
  inline CoinFactorizationDoubleArrayWithLength& operator=(const CoinFactorizationDoubleArrayWithLength & rhs)
  { CoinArrayWithLength::operator=(rhs);  return *this;}
  //@}
};
/// CoinFactorizationLongDouble * version

class CoinFactorizationLongDoubleArrayWithLength : public CoinArrayWithLength {
  
public:
  /**@name Get methods. */
  //@{
  /// Get the size
  inline int getSize() const 
  { return size_/CoinSizeofAsInt(long double); }
  /// Get Array
  inline long double * array() const 
  { return reinterpret_cast<long double *> ((size_>-2) ? array_ : NULL); }
  //@}
  
  /**@name Set methods */
  //@{
  /// Set the size
  inline void setSize(int value) 
  { size_ = value*CoinSizeofAsInt(long double); }
  //@}
  
  /**@name Condition methods */
  //@{
  /// Conditionally gets new array
  inline long double * conditionalNew(int sizeWanted)
  { return reinterpret_cast<long double *> (CoinArrayWithLength::conditionalNew(sizeWanted>=0 ? static_cast<long> (( sizeWanted)*CoinSizeofAsInt(long double)) : -1)); }
  //@}
  
  /**@name Constructors and destructors */
  //@{
  /** Default constructor - NULL*/
  inline CoinFactorizationLongDoubleArrayWithLength()
  { array_=NULL; size_=-1;}
  /** Alternate Constructor - length in bytes - size_ -1 */
  inline CoinFactorizationLongDoubleArrayWithLength(int size)
  { array_=new char [size*CoinSizeofAsInt(long double)]; size_=-1;}
  /** Alternate Constructor - length in bytes 
      mode -  0 size_ set to size
      1 size_ set to size and zeroed
  */
  inline CoinFactorizationLongDoubleArrayWithLength(int size, int mode)
    : CoinArrayWithLength(size*CoinSizeofAsInt(long double),mode) {}
  /** Copy constructor. */
  inline CoinFactorizationLongDoubleArrayWithLength(const CoinFactorizationLongDoubleArrayWithLength & rhs)
    : CoinArrayWithLength(rhs) {}
  /** Copy constructor.2 */
  inline CoinFactorizationLongDoubleArrayWithLength(const CoinFactorizationLongDoubleArrayWithLength * rhs)
    : CoinArrayWithLength(rhs) {}
  /** Assignment operator. */
  inline CoinFactorizationLongDoubleArrayWithLength& operator=(const CoinFactorizationLongDoubleArrayWithLength & rhs)
  { CoinArrayWithLength::operator=(rhs);  return *this;}
  //@}
};
/// int * version

class CoinIntArrayWithLength : public CoinArrayWithLength {
  
public:
  /**@name Get methods. */
  //@{
  /// Get the size
  inline int getSize() const 
  { return size_/CoinSizeofAsInt(int); }
  /// Get Array
  inline int * array() const 
  { return reinterpret_cast<int *> ((size_>-2) ? array_ : NULL); }
  //@}
  
  /**@name Set methods */
  //@{
  /// Set the size
  inline void setSize(int value) 
  { size_ = value*CoinSizeofAsInt(int); }
  //@}
  
  /**@name Condition methods */
  //@{
  /// Conditionally gets new array
  inline int * conditionalNew(int sizeWanted)
  { return reinterpret_cast<int *> (CoinArrayWithLength::conditionalNew(sizeWanted>=0 ? static_cast<long> (( sizeWanted)*CoinSizeofAsInt(int)) : -1)); }
  //@}
  
  /**@name Constructors and destructors */
  //@{
  /** Default constructor - NULL*/
  inline CoinIntArrayWithLength()
  { array_=NULL; size_=-1;}
  /** Alternate Constructor - length in bytes - size_ -1 */
  inline CoinIntArrayWithLength(int size)
  { array_=new char [size*CoinSizeofAsInt(int)]; size_=-1;}
  /** Alternate Constructor - length in bytes 
      mode -  0 size_ set to size
      1 size_ set to size and zeroed
  */
  inline CoinIntArrayWithLength(int size, int mode)
    : CoinArrayWithLength(size*CoinSizeofAsInt(int),mode) {}
  /** Copy constructor. */
  inline CoinIntArrayWithLength(const CoinIntArrayWithLength & rhs)
    : CoinArrayWithLength(rhs) {}
  /** Copy constructor.2 */
  inline CoinIntArrayWithLength(const CoinIntArrayWithLength * rhs)
    : CoinArrayWithLength(rhs) {}
  /** Assignment operator. */
  inline CoinIntArrayWithLength& operator=(const CoinIntArrayWithLength & rhs)
  { CoinArrayWithLength::operator=(rhs);  return *this;}
  //@}
};
/// CoinBigIndex * version

class CoinBigIndexArrayWithLength : public CoinArrayWithLength {
  
public:
  /**@name Get methods. */
  //@{
  /// Get the size
  inline int getSize() const 
  { return size_/CoinSizeofAsInt(CoinBigIndex); }
  /// Get Array
  inline CoinBigIndex * array() const 
  { return reinterpret_cast<CoinBigIndex *> ((size_>-2) ? array_ : NULL); }
  //@}
  
  /**@name Set methods */
  //@{
  /// Set the size
  inline void setSize(int value) 
  { size_ = value*CoinSizeofAsInt(CoinBigIndex); }
  //@}
  
  /**@name Condition methods */
  //@{
  /// Conditionally gets new array
  inline CoinBigIndex * conditionalNew(int sizeWanted)
  { return reinterpret_cast<CoinBigIndex *> (CoinArrayWithLength::conditionalNew(sizeWanted>=0 ? static_cast<long> (( sizeWanted)*CoinSizeofAsInt(CoinBigIndex)) : -1)); }
  //@}
  
  /**@name Constructors and destructors */
  //@{
  /** Default constructor - NULL*/
  inline CoinBigIndexArrayWithLength()
  { array_=NULL; size_=-1;}
  /** Alternate Constructor - length in bytes - size_ -1 */
  inline CoinBigIndexArrayWithLength(int size)
  { array_=new char [size*CoinSizeofAsInt(CoinBigIndex)]; size_=-1;}
  /** Alternate Constructor - length in bytes 
      mode -  0 size_ set to size
      1 size_ set to size and zeroed
  */
  inline CoinBigIndexArrayWithLength(int size, int mode)
    : CoinArrayWithLength(size*CoinSizeofAsInt(CoinBigIndex),mode) {}
  /** Copy constructor. */
  inline CoinBigIndexArrayWithLength(const CoinBigIndexArrayWithLength & rhs)
    : CoinArrayWithLength(rhs) {}
  /** Copy constructor.2 */
  inline CoinBigIndexArrayWithLength(const CoinBigIndexArrayWithLength * rhs)
    : CoinArrayWithLength(rhs) {}
  /** Assignment operator. */
  inline CoinBigIndexArrayWithLength& operator=(const CoinBigIndexArrayWithLength & rhs)
  { CoinArrayWithLength::operator=(rhs);  return *this;}
  //@}
};
/// unsigned int * version

class CoinUnsignedIntArrayWithLength : public CoinArrayWithLength {
  
public:
  /**@name Get methods. */
  //@{
  /// Get the size
  inline int getSize() const 
  { return size_/CoinSizeofAsInt(unsigned int); }
  /// Get Array
  inline unsigned int * array() const 
  { return reinterpret_cast<unsigned int *> ((size_>-2) ? array_ : NULL); }
  //@}
  
  /**@name Set methods */
  //@{
  /// Set the size
  inline void setSize(int value) 
  { size_ = value*CoinSizeofAsInt(unsigned int); }
  //@}
  
  /**@name Condition methods */
  //@{
  /// Conditionally gets new array
  inline unsigned int * conditionalNew(int sizeWanted)
  { return reinterpret_cast<unsigned int *> (CoinArrayWithLength::conditionalNew(sizeWanted>=0 ? static_cast<long> (( sizeWanted)*CoinSizeofAsInt(unsigned int)) : -1)); }
  //@}
  
  /**@name Constructors and destructors */
  //@{
  /** Default constructor - NULL*/
  inline CoinUnsignedIntArrayWithLength()
  { array_=NULL; size_=-1;}
  /** Alternate Constructor - length in bytes - size_ -1 */
  inline CoinUnsignedIntArrayWithLength(int size)
  { array_=new char [size*CoinSizeofAsInt(unsigned int)]; size_=-1;}
  /** Alternate Constructor - length in bytes 
      mode -  0 size_ set to size
      1 size_ set to size and zeroed
  */
  inline CoinUnsignedIntArrayWithLength(int size, int mode)
    : CoinArrayWithLength(size*CoinSizeofAsInt(unsigned int),mode) {}
  /** Copy constructor. */
  inline CoinUnsignedIntArrayWithLength(const CoinUnsignedIntArrayWithLength & rhs)
    : CoinArrayWithLength(rhs) {}
  /** Copy constructor.2 */
  inline CoinUnsignedIntArrayWithLength(const CoinUnsignedIntArrayWithLength * rhs)
    : CoinArrayWithLength(rhs) {}
  /** Assignment operator. */
  inline CoinUnsignedIntArrayWithLength& operator=(const CoinUnsignedIntArrayWithLength & rhs)
  { CoinArrayWithLength::operator=(rhs);  return *this;}
  //@}
};
/// void * version

class CoinVoidStarArrayWithLength : public CoinArrayWithLength {
  
public:
  /**@name Get methods. */
  //@{
  /// Get the size
  inline int getSize() const 
  { return size_/CoinSizeofAsInt(void *); }
  /// Get Array
  inline void ** array() const 
  { return reinterpret_cast<void **> ((size_>-2) ? array_ : NULL); }
  //@}
  
  /**@name Set methods */
  //@{
  /// Set the size
  inline void setSize(int value) 
  { size_ = value*CoinSizeofAsInt(void *); }
  //@}
  
  /**@name Condition methods */
  //@{
  /// Conditionally gets new array
  inline void ** conditionalNew(int sizeWanted)
  { return reinterpret_cast<void **> ( CoinArrayWithLength::conditionalNew(sizeWanted>=0 ? static_cast<long> ((sizeWanted)*CoinSizeofAsInt(void *)) : -1)); }
  //@}
  
  /**@name Constructors and destructors */
  //@{
  /** Default constructor - NULL*/
  inline CoinVoidStarArrayWithLength()
  { array_=NULL; size_=-1;}
  /** Alternate Constructor - length in bytes - size_ -1 */
  inline CoinVoidStarArrayWithLength(int size)
  { array_=new char [size*CoinSizeofAsInt(void *)]; size_=-1;}
  /** Alternate Constructor - length in bytes 
      mode -  0 size_ set to size
      1 size_ set to size and zeroed
  */
  inline CoinVoidStarArrayWithLength(int size, int mode)
    : CoinArrayWithLength(size*CoinSizeofAsInt(void *),mode) {}
  /** Copy constructor. */
  inline CoinVoidStarArrayWithLength(const CoinVoidStarArrayWithLength & rhs)
    : CoinArrayWithLength(rhs) {}
  /** Copy constructor.2 */
  inline CoinVoidStarArrayWithLength(const CoinVoidStarArrayWithLength * rhs)
    : CoinArrayWithLength(rhs) {}
  /** Assignment operator. */
  inline CoinVoidStarArrayWithLength& operator=(const CoinVoidStarArrayWithLength & rhs)
  { CoinArrayWithLength::operator=(rhs);  return *this;}
  //@}
};
/// arbitrary version

class CoinArbitraryArrayWithLength : public CoinArrayWithLength {
  
public:
  /**@name Get methods. */
  //@{
  /// Get the size
  inline int getSize() const 
  { return size_/lengthInBytes_; }
  /// Get Array
  inline void ** array() const 
  { return reinterpret_cast<void **> ((size_>-2) ? array_ : NULL); }
  //@}
  
  /**@name Set methods */
  //@{
  /// Set the size
  inline void setSize(int value) 
  { size_ = value*lengthInBytes_; }
  //@}
  
  /**@name Condition methods */
  //@{
  /// Conditionally gets new array
  inline char * conditionalNew(int length, int sizeWanted)
  { lengthInBytes_=length;return reinterpret_cast<char *> ( CoinArrayWithLength::conditionalNew(sizeWanted>=0 ? static_cast<long> 
									  ((sizeWanted)*lengthInBytes_) : -1)); }
  //@}
  
  /**@name Constructors and destructors */
  //@{
  /** Default constructor - NULL*/
  inline CoinArbitraryArrayWithLength(int length=1)
  { array_=NULL; size_=-1;lengthInBytes_=length;}
  /** Alternate Constructor - length in bytes - size_ -1 */
  inline CoinArbitraryArrayWithLength(int length, int size)
  { array_=new char [size*length]; size_=-1; lengthInBytes_=length;}
  /** Alternate Constructor - length in bytes 
      mode -  0 size_ set to size
      1 size_ set to size and zeroed
  */
  inline CoinArbitraryArrayWithLength(int length, int size, int mode)
    : CoinArrayWithLength(size*length,mode) {lengthInBytes_=length;}
  /** Copy constructor. */
  inline CoinArbitraryArrayWithLength(const CoinArbitraryArrayWithLength & rhs)
    : CoinArrayWithLength(rhs) {}
  /** Copy constructor.2 */
  inline CoinArbitraryArrayWithLength(const CoinArbitraryArrayWithLength * rhs)
    : CoinArrayWithLength(rhs) {}
  /** Assignment operator. */
  inline CoinArbitraryArrayWithLength& operator=(const CoinArbitraryArrayWithLength & rhs)
  { CoinArrayWithLength::operator=(rhs);  return *this;}
  //@}

protected:
  /**@name Private member data */
  //@{
  /// Length in bytes
  int lengthInBytes_;
   //@}
};
class CoinPartitionedVector : public CoinIndexedVector {
  
public:
#ifndef COIN_PARTITIONS
#define COIN_PARTITIONS 8
#endif
   /**@name Get methods. */
   //@{
   /// Get the size of a partition
   inline int getNumElements(int partition) const { assert (partition<COIN_PARTITIONS);
     return numberElementsPartition_[partition]; }
   /// Get number of partitions
   inline int getNumPartitions() const
  { return numberPartitions_; }
   /// Get the size
   inline int getNumElements() const { return nElements_; }
   /// Get starts
  inline int startPartition(int partition) const  { assert (partition<=COIN_PARTITIONS);
    return startPartition_[partition]; }
   /// Get starts
  inline const int * startPartitions() const
  { return startPartition_; }
   //@}
 
   //-------------------------------------------------------------------
   // Set indices and elements
   //------------------------------------------------------------------- 
   /**@name Set methods */
   //@{
   /// Set the size of a partition
  inline void setNumElementsPartition(int partition, int value) { assert (partition<COIN_PARTITIONS);
    if (numberPartitions_) numberElementsPartition_[partition]=value; }
   /// Set the size of a partition (just for a tiny while)
  inline void setTempNumElementsPartition(int partition, int value) { assert (partition<COIN_PARTITIONS);
    numberElementsPartition_[partition]=value; }
  /// Add up number of elements in partitions
  void computeNumberElements();
  /// Add up number of elements in partitions and pack and get rid of partitions
  void compact();
   /** Reserve space.
   */
   void reserve(int n);
  /// Setup partitions (needs end as well)
  void setPartitions(int number,const int * starts);
   /// Reset the vector (as if were just created an empty vector). Gets rid of partitions
   void clearAndReset();
   /// Reset the vector (as if were just created an empty vector). Keeps partitions
   void clearAndKeep();
   /// Clear a partition.
   void clearPartition(int partition);
#ifndef NDEBUG
   /// For debug check vector is clear i.e. no elements
   void checkClear();
   /// For debug check vector is clean i.e. elements match indices
   void checkClean();
#else
  inline void checkClear() {};
  inline void checkClean() {};
#endif
   /// Scan dense region and set up indices (returns number found)
  int scan(int partition, double tolerance=0.0);
   /** Scan dense region from start to < end and set up indices
       returns number found
   */
   ///  Print out
   void print() const;
   //@}
 
   /**@name Sorting */
   //@{ 
   /** Sort the indexed storage vector (increasing indices). */
  void sort();
   //@}

   /**@name Constructors and destructors (not all wriiten) */
   //@{
   /** Default constructor */
   CoinPartitionedVector();
   /** Alternate Constructors - set elements to vector of doubles */
  CoinPartitionedVector(int size, const int * inds, const double * elems);
   /** Alternate Constructors - set elements to same scalar value */
  CoinPartitionedVector(int size, const int * inds, double element);
   /** Alternate Constructors - construct full storage with indices 0 through
       size-1. */
  CoinPartitionedVector(int size, const double * elements);
   /** Alternate Constructors - just size */
  CoinPartitionedVector(int size);
   /** Copy constructor. */
   CoinPartitionedVector(const CoinPartitionedVector &);
   /** Copy constructor.2 */
   CoinPartitionedVector(const CoinPartitionedVector *);
   /** Assignment operator. */
   CoinPartitionedVector & operator=(const CoinPartitionedVector &);
   /** Destructor */
   ~CoinPartitionedVector ();
   //@}
protected:
   /**@name Private member data */
   //@{
   /// Starts
   int startPartition_[COIN_PARTITIONS+1];
   /// Size of indices in a partition
   int numberElementsPartition_[COIN_PARTITIONS];
  /// Number of partitions (0 means off)
  int numberPartitions_;
   //@}
};
#endif
