// Copyright (C) 2004, 2006 International Business Machines and others.
// All Rights Reserved.
// This code is published under the Eclipse Public License.
//
// $Id: CoinSmartPtr.hpp 1520 2012-01-29 00:43:31Z tkr $
//
// Authors:  Carl Laird, Andreas Waechter     IBM    2004-08-13
// Removed lots of debugging stuff and reformatted: Laszlo Ladanyi, IBM
#ifndef CoinSmartPtr_hpp
#define CoinSmartPtr_hpp

#include <list>
#include <cassert>
#include <cstddef>
#include <cstring>

namespace Coin {

    //#########################################################################

    /** ReferencedObject class.
     * This is part of the implementation of an intrusive smart pointer 
     * design. This class stores the reference count of all the smart
     * pointers that currently reference it. See the documentation for
     * the SmartPtr class for more details.
     * 
     * A SmartPtr behaves much like a raw pointer, but manages the lifetime 
     * of an object, deleting the object automatically. This class implements
     * a reference-counting, intrusive smart pointer design, where all
     * objects pointed to must inherit off of ReferencedObject, which
     * stores the reference count. Although this is intrusive (native types
     * and externally authored classes require wrappers to be referenced
     * by smart pointers), it is a safer design. A more detailed discussion of
     * these issues follows after the usage information.
     * 
     * Usage Example:
     * Note: to use the SmartPtr, all objects to which you point MUST
     * inherit off of ReferencedObject.
     * 
     * \verbatim
     * 
     * In MyClass.hpp...
     * 
     * #include "CoinSmartPtr.hpp"

     * 
     * class MyClass : public Coin::ReferencedObject // must derive from ReferencedObject
     *    {
     *      ...
     *    }
     * 
     * In my_usage.cpp...
     * 
     * #include "CoinSmartPtr.hpp"
     * #include "MyClass.hpp"
     * 
     * void func(AnyObject& obj)
     *  {
     *    Coin::SmartPtr<MyClass> ptr_to_myclass = new MyClass(...);
     *    // ptr_to_myclass now points to a new MyClass,
     *    // and the reference count is 1
     *    
     *    ...
     * 
     *    obj.SetMyClass(ptr_to_myclass);
     *    // Here, let's assume that AnyObject uses a
     *    // SmartPtr<MyClass> internally here.
     *    // Now, both ptr_to_myclass and the internal
     *    // SmartPtr in obj point to the same MyClass object
     *    // and its reference count is 2.
     * 
     *    ...
     * 
     *    // No need to delete ptr_to_myclass, this
     *    // will be done automatically when the
     *    // reference count drops to zero.
     * 
     *  }   
     *  
     * \endverbatim
     * 
     * Other Notes:
     *  The SmartPtr implements both dereference operators -> & *.
     *  The SmartPtr does NOT implement a conversion operator to
     *    the raw pointer. Use the GetRawPtr() method when this
     *    is necessary. Make sure that the raw pointer is NOT
     *    deleted. 
     *  The SmartPtr implements the comparison operators == & !=
     *    for a variety of types. Use these instead of
     *    \verbatim
     *    if (GetRawPtr(smrt_ptr) == ptr) // Don't use this
     *    \endverbatim
     * SmartPtr's, as currently implemented, do NOT handle circular references.
     *    For example: consider a higher level object using SmartPtrs to point
     *    to A and B, but A and B also point to each other (i.e. A has a
     *    SmartPtr to B and B has a SmartPtr to A). In this scenario, when the
     *    higher level object is finished with A and B, their reference counts
     *    will never drop to zero (since they reference each other) and they
     *    will not be deleted. This can be detected by memory leak tools like
     *    valgrind. If the circular reference is necessary, the problem can be
     *    overcome by a number of techniques:
     *    
     *    1) A and B can have a method that "releases" each other, that is
     *        they set their internal SmartPtrs to NULL.
     *        \verbatim
     *        void AClass::ReleaseCircularReferences()
     *          {
     *          smart_ptr_to_B = NULL;
     *          }
     *        \endverbatim
     *        Then, the higher level class can call these methods before
     *        it is done using A & B.
     * 
     *    2) Raw pointers can be used in A and B to reference each other.
     *        Here, an implicit assumption is made that the lifetime is
     *        controlled by the higher level object and that A and B will
     *        both exist in a controlled manner. Although this seems 
     *        dangerous, in many situations, this type of referencing
     *        is very controlled and this is reasonably safe.
     * 
     *    3) This SmartPtr class could be redesigned with the Weak/Strong
     *        design concept. Here, the SmartPtr is identified as being
     *        Strong (controls lifetime of the object) or Weak (merely
     *        referencing the object). The Strong SmartPtr increments 
     *        (and decrements) the reference count in ReferencedObject
     *        but the Weak SmartPtr does not. In the example above,
     *        the higher level object would have Strong SmartPtrs to
     *        A and B, but A and B would have Weak SmartPtrs to each
     *        other. Then, when the higher level object was done with
     *        A and B, they would be deleted. The Weak SmartPtrs in A
     *        and B would not decrement the reference count and would,
     *        of course, not delete the object. This idea is very similar
     *        to item (2), where it is implied that the sequence of events 
     *        is controlled such that A and B will not call anything using
     *        their pointers following the higher level delete (i.e. in
     *        their destructors!). This is somehow safer, however, because
     *        code can be written (however expensive) to perform run-time 
     *        detection of this situation. For example, the ReferencedObject
     *        could store pointers to all Weak SmartPtrs that are referencing
     *        it and, in its destructor, tell these pointers that it is
     *        dying. They could then set themselves to NULL, or set an
     *        internal flag to detect usage past this point.
     * 
     * Comments on Non-Intrusive Design:
     * In a non-intrusive design, the reference count is stored somewhere other
     * than the object being referenced. This means, unless the reference
     * counting pointer is the first referencer, it must get a pointer to the 
     * referenced object from another smart pointer (so it has access to the 
     * reference count location). In this non-intrusive design, if we are 
     * pointing to an object with a smart pointer (or a number of smart
     * pointers), and we then give another smart pointer the address through
     * a RAW pointer, we will have two independent, AND INCORRECT, reference
     * counts. To avoid this pitfall, we use an intrusive reference counting
     * technique where the reference count is stored in the object being
     * referenced. 
     */
    class ReferencedObject {
    public:
	ReferencedObject() : reference_count_(0) {}
	virtual ~ReferencedObject()       { assert(reference_count_ == 0); }
	inline int ReferenceCount() const { return reference_count_; }
	inline void AddRef() const        { ++reference_count_; }
	inline void ReleaseRef() const    { --reference_count_; }

    private:
	mutable int reference_count_;
    };

    //#########################################################################


//#define IP_DEBUG_SMARTPTR
#if COIN_IPOPT_CHECKLEVEL > 2
# define IP_DEBUG_SMARTPTR
#endif
#ifdef IP_DEBUG_SMARTPTR
# include "IpDebug.hpp"
#endif

    /** Template class for Smart Pointers.
     * A SmartPtr behaves much like a raw pointer, but manages the lifetime 
     * of an object, deleting the object automatically. This class implements
     * a reference-counting, intrusive smart pointer design, where all
     * objects pointed to must inherit off of ReferencedObject, which
     * stores the reference count. Although this is intrusive (native types
     * and externally authored classes require wrappers to be referenced
     * by smart pointers), it is a safer design. A more detailed discussion of
     * these issues follows after the usage information.
     * 
     * Usage Example:
     * Note: to use the SmartPtr, all objects to which you point MUST
     * inherit off of ReferencedObject.
     * 
     * \verbatim
     * 
     * In MyClass.hpp...
     * 
     * #include "CoinSmartPtr.hpp"
     * 
     *  class MyClass : public Coin::ReferencedObject // must derive from ReferencedObject
     *    {
     *      ...
     *    }
     * 
     * In my_usage.cpp...
     * 
     * #include "CoinSmartPtr.hpp"
     * #include "MyClass.hpp"
     * 
     * void func(AnyObject& obj)
     *  {
     *    SmartPtr<MyClass> ptr_to_myclass = new MyClass(...);
     *    // ptr_to_myclass now points to a new MyClass,
     *    // and the reference count is 1
     *    
     *    ...
     * 
     *    obj.SetMyClass(ptr_to_myclass);
     *    // Here, let's assume that AnyObject uses a
     *    // SmartPtr<MyClass> internally here.
     *    // Now, both ptr_to_myclass and the internal
     *    // SmartPtr in obj point to the same MyClass object
     *    // and its reference count is 2.
     * 
     *    ...
     * 
     *    // No need to delete ptr_to_myclass, this
     *    // will be done automatically when the
     *    // reference count drops to zero.
     * 
     *  }   
     *  
     * \endverbatim
     *
     * It is not necessary to use SmartPtr's in all cases where an
     * object is used that has been allocated "into" a SmartPtr.  It is
     * possible to just pass objects by reference or regular pointers,
     * even if lower down in the stack a SmartPtr is to be held on to.
     * Everything should work fine as long as a pointer created by "new"
     * is immediately passed into a SmartPtr, and if SmartPtr's are used
     * to hold on to objects.
     *
     * Other Notes:
     *  The SmartPtr implements both dereference operators -> & *.
     *  The SmartPtr does NOT implement a conversion operator to
     *    the raw pointer. Use the GetRawPtr() method when this
     *    is necessary. Make sure that the raw pointer is NOT
     *    deleted. 
     *  The SmartPtr implements the comparison operators == & !=
     *    for a variety of types. Use these instead of
     *    \verbatim
     *    if (GetRawPtr(smrt_ptr) == ptr) // Don't use this
     *    \endverbatim
     * SmartPtr's, as currently implemented, do NOT handle circular references.
     *    For example: consider a higher level object using SmartPtrs to point to 
     *    A and B, but A and B also point to each other (i.e. A has a SmartPtr 
     *    to B and B has a SmartPtr to A). In this scenario, when the higher
     *    level object is finished with A and B, their reference counts will 
     *    never drop to zero (since they reference each other) and they
     *    will not be deleted. This can be detected by memory leak tools like
     *    valgrind. If the circular reference is necessary, the problem can be
     *    overcome by a number of techniques:
     *    
     *    1) A and B can have a method that "releases" each other, that is
     *        they set their internal SmartPtrs to NULL.
     *        \verbatim
     *        void AClass::ReleaseCircularReferences()
     *          {
     *          smart_ptr_to_B = NULL;
     *          }
     *        \endverbatim
     *        Then, the higher level class can call these methods before
     *        it is done using A & B.
     * 
     *    2) Raw pointers can be used in A and B to reference each other.
     *        Here, an implicit assumption is made that the lifetime is
     *        controlled by the higher level object and that A and B will
     *        both exist in a controlled manner. Although this seems 
     *        dangerous, in many situations, this type of referencing
     *        is very controlled and this is reasonably safe.
     * 
     *    3) This SmartPtr class could be redesigned with the Weak/Strong
     *        design concept. Here, the SmartPtr is identified as being
     *        Strong (controls lifetime of the object) or Weak (merely
     *        referencing the object). The Strong SmartPtr increments 
     *        (and decrements) the reference count in ReferencedObject
     *        but the Weak SmartPtr does not. In the example above,
     *        the higher level object would have Strong SmartPtrs to
     *        A and B, but A and B would have Weak SmartPtrs to each
     *        other. Then, when the higher level object was done with
     *        A and B, they would be deleted. The Weak SmartPtrs in A
     *        and B would not decrement the reference count and would,
     *        of course, not delete the object. This idea is very similar
     *        to item (2), where it is implied that the sequence of events 
     *        is controlled such that A and B will not call anything using
     *        their pointers following the higher level delete (i.e. in
     *        their destructors!). This is somehow safer, however, because
     *        code can be written (however expensive) to perform run-time 
     *        detection of this situation. For example, the ReferencedObject
     *        could store pointers to all Weak SmartPtrs that are referencing
     *        it and, in its destructor, tell these pointers that it is
     *        dying. They could then set themselves to NULL, or set an
     *        internal flag to detect usage past this point.
     * 
     * Comments on Non-Intrusive Design:
     * In a non-intrusive design, the reference count is stored somewhere other
     * than the object being referenced. This means, unless the reference
     * counting pointer is the first referencer, it must get a pointer to the 
     * referenced object from another smart pointer (so it has access to the 
     * reference count location). In this non-intrusive design, if we are 
     * pointing to an object with a smart pointer (or a number of smart
     * pointers), and we then give another smart pointer the address through
     * a RAW pointer, we will have two independent, AND INCORRECT, reference
     * counts. To avoid this pitfall, we use an intrusive reference counting
     * technique where the reference count is stored in the object being
     * referenced. 
     */
    template <class T>
    class SmartPtr {
    public:
	/** Returns the raw pointer contained.  Use to get the value of the
	 * raw ptr (i.e. to pass to other methods/functions, etc.)  Note: This
	 * method does NOT copy, therefore, modifications using this value
	 * modify the underlying object contained by the SmartPtr, NEVER
	 * delete this returned value.
	 */
	T* GetRawPtr() const { return ptr_; }

	/** Returns true if the SmartPtr is NOT NULL.
	 * Use this to check if the SmartPtr is not null
	 * This is preferred to if(GetRawPtr(sp) != NULL)
	 */
	bool IsValid() const { return ptr_ != NULL; }

	/** Returns true if the SmartPtr is NULL.
	 * Use this to check if the SmartPtr IsNull.
	 * This is preferred to if(GetRawPtr(sp) == NULL)
	 */
	bool IsNull() const { return ptr_ == NULL; }

    private:
	/**@name Private Data/Methods */
	//@{
	/** Actual raw pointer to the object. */
	T* ptr_;

	/** Release the currently referenced object. */
	void ReleasePointer_() {
	    if (ptr_) {
		ptr_->ReleaseRef();
		if (ptr_->ReferenceCount() == 0) {
		    delete ptr_;
		}
		ptr_ = NULL;
	    }
	}

	/** Set the value of the internal raw pointer from another raw
	 * pointer, releasing the previously referenced object if necessary. */
	SmartPtr<T>& SetFromRawPtr_(T* rhs){
	    ReleasePointer_(); // Release any old pointer
	    if (rhs != NULL) {
		rhs->AddRef();
		ptr_ = rhs;
	    }
	    return *this;
	}

	/** Set the value of the internal raw pointer from a SmartPtr,
	 * releasing the previously referenced object if necessary. */
	inline SmartPtr<T>& SetFromSmartPtr_(const SmartPtr<T>& rhs) {
	    SetFromRawPtr_(rhs.GetRawPtr());
	    return (*this);
	}
	    
	//@}

    public:
#define dbg_smartptr_verbosity 0

	/**@name Constructors/Destructors */
	//@{
	/** Default constructor, initialized to NULL */
	SmartPtr() : ptr_(NULL) {}

	/** Copy constructor, initialized from copy */
	SmartPtr(const SmartPtr<T>& copy) : ptr_(NULL) {
	    (void) SetFromSmartPtr_(copy);
	}

	/** Constructor, initialized from T* ptr */
	SmartPtr(T* ptr) :  ptr_(NULL) {
	    (void) SetFromRawPtr_(ptr);
	}

	/** Destructor, automatically decrements the reference count, deletes
	 * the object if necessary.*/
	~SmartPtr() {
	    ReleasePointer_();
	}
	//@}

	/**@name Overloaded operators. */
	//@{
	/** Overloaded arrow operator, allows the user to call
	 * methods using the contained pointer. */
	T* operator->() const {
#if COIN_COINUTILS_CHECKLEVEL > 0
	    assert(ptr_);
#endif
	    return ptr_;
	}

	/** Overloaded dereference operator, allows the user
	 * to dereference the contained pointer. */
	T& operator*() const {
#if COIN_IPOPT_CHECKLEVEL > 0
	    assert(ptr_);
#endif
	    return *ptr_;
	}

	/** Overloaded equals operator, allows the user to
	 * set the value of the SmartPtr from a raw pointer */
	SmartPtr<T>& operator=(T* rhs) {
	    return SetFromRawPtr_(rhs);
	}

	/** Overloaded equals operator, allows the user to
	 * set the value of the SmartPtr from another 
	 * SmartPtr */
	SmartPtr<T>& operator=(const SmartPtr<T>& rhs) {
	     return SetFromSmartPtr_(rhs);
	}

	/** Overloaded equality comparison operator, allows the
	 * user to compare the value of two SmartPtrs */
	template <class U1, class U2>
	friend
	bool operator==(const SmartPtr<U1>& lhs, const SmartPtr<U2>& rhs);

	/** Overloaded equality comparison operator, allows the
	 * user to compare the value of a SmartPtr with a raw pointer. */
	template <class U1, class U2>
	friend
	bool operator==(const SmartPtr<U1>& lhs, U2* raw_rhs);

	/** Overloaded equality comparison operator, allows the
	 * user to compare the value of a raw pointer with a SmartPtr. */
	template <class U1, class U2>
	friend
	bool operator==(U1* lhs, const SmartPtr<U2>& raw_rhs);

	/** Overloaded in-equality comparison operator, allows the
	 * user to compare the value of two SmartPtrs */
	template <class U1, class U2>
	friend
	bool operator!=(const SmartPtr<U1>& lhs, const SmartPtr<U2>& rhs);

	/** Overloaded in-equality comparison operator, allows the
	 * user to compare the value of a SmartPtr with a raw pointer. */
	template <class U1, class U2>
	friend
	bool operator!=(const SmartPtr<U1>& lhs, U2* raw_rhs);

	/** Overloaded in-equality comparison operator, allows the
	 * user to compare the value of a SmartPtr with a raw pointer. */
	template <class U1, class U2>
	friend
	bool operator!=(U1* lhs, const SmartPtr<U2>& raw_rhs);
	//@}

    };

    template <class U1, class U2>
    bool ComparePointers(const U1* lhs, const U2* rhs) {
	if (lhs == rhs) {
	    return true;
	}
	// If lhs and rhs point to the same object with different interfaces
	// U1 and U2, we cannot guarantee that the value of the pointers will
	// be equivalent. We can guarantee this if we convert to void*.
	return static_cast<const void*>(lhs) == static_cast<const void*>(rhs);
    }

} // namespace Coin

//#############################################################################

/**@name SmartPtr friends that are overloaded operators, so they are not in
   the Coin namespace. */
//@{
template <class U1, class U2>
bool operator==(const Coin::SmartPtr<U1>& lhs, const Coin::SmartPtr<U2>& rhs) {
    return Coin::ComparePointers(lhs.GetRawPtr(), rhs.GetRawPtr());
}

template <class U1, class U2>
bool operator==(const Coin::SmartPtr<U1>& lhs, U2* raw_rhs) {
    return Coin::ComparePointers(lhs.GetRawPtr(), raw_rhs);
}

template <class U1, class U2>
bool operator==(U1* raw_lhs, const Coin::SmartPtr<U2>& rhs) {
    return Coin::ComparePointers(raw_lhs, rhs.GetRawPtr());
}

template <class U1, class U2>
bool operator!=(const Coin::SmartPtr<U1>& lhs, const Coin::SmartPtr<U2>& rhs) {
    return ! operator==(lhs, rhs);
}

template <class U1, class U2>
bool operator!=(const Coin::SmartPtr<U1>& lhs, U2* raw_rhs) {
    return ! operator==(lhs, raw_rhs);
}

template <class U1, class U2>
bool operator!=(U1* raw_lhs, const Coin::SmartPtr<U2>& rhs) {
    return ! operator==(raw_lhs, rhs);
}
//@}

#define CoinReferencedObject Coin::ReferencedObject
#define CoinSmartPtr         Coin::SmartPtr
#define CoinComparePointers  Coin::ComparePointers

#endif
