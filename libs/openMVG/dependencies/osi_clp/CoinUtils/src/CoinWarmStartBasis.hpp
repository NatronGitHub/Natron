/* $Id: CoinWarmStartBasis.hpp 1515 2011-12-10 23:38:04Z lou $ */
/*! \legal
  Copyright (C) 2000 -- 2003, International Business Machines Corporation
  and others.  All Rights Reserved.
  This code is licensed under the terms of the Eclipse Public License (EPL).
*/

/*! \file CoinWarmStart.hpp
    \brief Declaration of the generic simplex (basis-oriented) warm start
	   class. Also contains a basis diff class.
*/

#ifndef CoinWarmStartBasis_H
#define CoinWarmStartBasis_H

#include <vector>

#include "CoinSort.hpp"
#include "CoinHelperFunctions.hpp"
#include "CoinWarmStart.hpp"

//#############################################################################

/*! \class CoinWarmStartBasis
    \brief The default COIN simplex (basis-oriented) warm start class
    
    CoinWarmStartBasis provides for a warm start object which contains the
    status of each variable (structural and artificial).

    \todo Modify this class so that the number of status entries per byte
	  and bytes per status vector allocation unit are not hardcoded.
	  At the least, collect this into a couple of macros.
    
    \todo Consider separate fields for allocated capacity and actual basis
	  size. We could avoid some reallocation, at the price of retaining
	  more space than we need. Perhaps more important, we could do much
	  better sanity checks.
*/

class CoinWarmStartBasis : public virtual CoinWarmStart {
public:

  /*! \brief Enum for status of variables

    Matches CoinPrePostsolveMatrix::Status, without superBasic. Most code that
    converts between CoinPrePostsolveMatrix::Status and
    CoinWarmStartBasis::Status will break if this correspondence is broken.

    The status vectors are currently packed using two bits per status code,
    four codes per byte. The location of the status information for
    variable \c i is in byte <code>i>>2</code> and occupies bits 0:1
    if <code>i\%4 == 0</code>, bits 2:3 if <code>i\%4 == 1</code>, etc.
    The non-member functions getStatus(const char*,int) and
    setStatus(char*,int,CoinWarmStartBasis::Status) are provided to hide
    details of the packing.
  */
  enum Status {
    isFree = 0x00,		///< Nonbasic free variable
    basic = 0x01,		///< Basic variable
    atUpperBound = 0x02,	///< Nonbasic at upper bound
    atLowerBound = 0x03		///< Nonbasic at lower bound
  };

  /** \brief Transfer vector entry for
	 mergeBasis(const CoinWarmStartBasis*,const XferVec*,const XferVec*)
  */
  typedef CoinTriple<int,int,int> XferEntry ;

  /** \brief Transfer vector for
	 mergeBasis(const CoinWarmStartBasis*,const XferVec*,const XferVec*)
  */
  typedef std::vector<XferEntry> XferVec ;

public:

/*! \name Methods to get and set basis information.

  The status of variables is kept in a pair of arrays, one for structural
  variables, and one for artificials (aka logicals and slacks). The status
  is coded using the values of the Status enum.

  \sa CoinWarmStartBasis::Status for a description of the packing used in
  the status arrays.
*/
//@{
  /// Return the number of structural variables
  inline int getNumStructural() const { return numStructural_; }

  /// Return the number of artificial variables
  inline int getNumArtificial() const { return numArtificial_; }

  /** Return the number of basic structurals
  
    A fast test for an all-slack basis.
  */
  int numberBasicStructurals() const ;

  /// Return the status of the specified structural variable.
  inline Status getStructStatus(int i) const {
    const int st = (structuralStatus_[i>>2] >> ((i&3)<<1)) & 3;
    return static_cast<CoinWarmStartBasis::Status>(st);
  }

  /// Set the status of the specified structural variable.
  inline void setStructStatus(int i, Status st) {
    char& st_byte = structuralStatus_[i>>2];
    st_byte = static_cast<char>(st_byte & ~(3 << ((i&3)<<1))) ;
    st_byte = static_cast<char>(st_byte | (st << ((i&3)<<1))) ;
  }

  /** Return the status array for the structural variables
  
    The status information is stored using the codes defined in the
    Status enum, 2 bits per variable, packed 4 variables per byte.
  */
  inline char * getStructuralStatus() { return structuralStatus_; }

  /** \c const overload for
    \link CoinWarmStartBasis::getStructuralStatus()
    	  getStructuralStatus()
    \endlink
  */
  inline const char * getStructuralStatus() const { return structuralStatus_; }

  /** As for \link getStructuralStatus() getStructuralStatus \endlink,
      but returns the status array for the artificial variables.
  */
  inline char * getArtificialStatus() { return artificialStatus_; }

  /// Return the status of the specified artificial variable.
  inline Status getArtifStatus(int i) const {
    const int st = (artificialStatus_[i>>2] >> ((i&3)<<1)) & 3;
    return static_cast<CoinWarmStartBasis::Status>(st);
  }

  /// Set the status of the specified artificial variable.
  inline void setArtifStatus(int i, Status st) {
    char& st_byte = artificialStatus_[i>>2];
    st_byte = static_cast<char>(st_byte & ~(3 << ((i&3)<<1))) ;
    st_byte = static_cast<char>(st_byte | (st << ((i&3)<<1))) ;
  }

  /** \c const overload for
    \link CoinWarmStartBasis::getArtificialStatus()
    	  getArtificialStatus()
    \endlink
  */
  inline const char * getArtificialStatus() const { return artificialStatus_; }

//@}

/*! \name Basis `diff' methods */
//@{

  /*! \brief Generate a `diff' that can convert the warm start basis passed as
	     a parameter to the warm start basis specified by \c this.

    The capabilities are limited: the basis passed as a parameter can be no
    larger than the basis pointed to by \c this.
  */

  virtual CoinWarmStartDiff*
  generateDiff (const CoinWarmStart *const oldCWS) const ;

  /*! \brief Apply \p diff to this basis

    Update this basis by applying \p diff. It's assumed that the allocated
    capacity of the basis is sufficiently large.
  */

  virtual void
  applyDiff (const CoinWarmStartDiff *const cwsdDiff) ;

//@}


/*! \name Methods to modify the warm start object */
//@{

  /*! \brief Set basis capacity; existing basis is discarded.

    After execution of this routine, the warm start object does not describe
    a valid basis: all structural and artificial variables have status isFree.
  */
  virtual void setSize(int ns, int na) ;

  /*! \brief Set basis capacity; existing basis is maintained.

    After execution of this routine, the warm start object describes a valid
    basis: the status of new structural variables (added columns) is set to
    nonbasic at lower bound, and the status of new artificial variables
    (added rows) is set to basic. (The basis can be invalid if new structural
    variables do not have a finite lower bound.)
  */
  virtual void resize (int newNumberRows, int newNumberColumns);

  /** \brief Delete a set of rows from the basis

    \warning
    This routine assumes that the set of indices to be deleted is sorted in
    ascending order and contains no duplicates. Use deleteRows() if this is
    not the case.

    \warning
    The resulting basis is guaranteed valid only if all deleted
    constraints are slack (hence the associated logicals are basic).

    Removal of a tight constraint with a nonbasic logical implies that
    some basic variable must be made nonbasic. This correction is left to
    the client.
  */

  virtual void compressRows (int tgtCnt, const int *tgts) ;

  /** \brief Delete a set of rows from the basis

    \warning
    The resulting basis is guaranteed valid only if all deleted
    constraints are slack (hence the associated logicals are basic).

    Removal of a tight constraint with a nonbasic logical implies that
    some basic variable must be made nonbasic. This correction is left to
    the client.
  */

  virtual void deleteRows(int rawTgtCnt, const int *rawTgts) ;

  /** \brief Delete a set of columns from the basis

    \warning
    The resulting basis is guaranteed valid only if all deleted variables
    are nonbasic.

    Removal of a basic variable implies that some nonbasic variable must be
    made basic. This correction is left to the client.
 */

  virtual void deleteColumns(int number, const int * which);

  /** \brief Merge entries from a source basis into this basis.

    \warning
    It's the client's responsibility to ensure validity of the merged basis,
    if that's important to the application.

    The vector xferCols (xferRows) specifies runs of entries to be taken from
    the source basis and placed in this basis. Each entry is a CoinTriple,
    with first specifying the starting source index of a run, second
    specifying the starting destination index, and third specifying the run
    length.
  */
  virtual void mergeBasis(const CoinWarmStartBasis *src,
			  const XferVec *xferRows,
			  const XferVec *xferCols) ;

//@}

/*! \name Constructors, destructors, and related functions */

//@{

  /** Default constructor

    Creates a warm start object representing an empty basis
    (0 rows, 0 columns).
  */
  CoinWarmStartBasis();

  /** Constructs a warm start object with the specified status vectors.

    The parameters are copied.
    Consider assignBasisStatus(int,int,char*&,char*&) if the object should
    assume ownership.

    \sa CoinWarmStartBasis::Status for a description of the packing used in
    the status arrays.
  */
  CoinWarmStartBasis(int ns, int na, const char* sStat, const char* aStat) ;

  /** Copy constructor */
  CoinWarmStartBasis(const CoinWarmStartBasis& ws) ;

  /** `Virtual constructor' */
  virtual CoinWarmStart *clone() const
  {
     return new CoinWarmStartBasis(*this);
  }

  /** Destructor */
  virtual ~CoinWarmStartBasis();

  /** Assignment */

  virtual CoinWarmStartBasis& operator=(const CoinWarmStartBasis& rhs) ;

  /** Assign the status vectors to be the warm start information.
  
      In this method the CoinWarmStartBasis object assumes ownership of the
      pointers and upon return the argument pointers will be NULL.
      If copying is desirable, use the
      \link CoinWarmStartBasis(int,int,const char*,const char*)
	    array constructor \endlink
      or the
      \link operator=(const CoinWarmStartBasis&)
	    assignment operator \endlink.

      \note
      The pointers passed to this method will be
      freed using delete[], so they must be created using new[].
  */
  virtual void assignBasisStatus(int ns, int na, char*& sStat, char*& aStat) ;
//@}

/*! \name Miscellaneous methods */
//@{

  /// Prints in readable format (for debug)
  virtual void print() const;
  /// Returns true if full basis (for debug)
  bool fullBasis() const;
  /// Returns true if full basis and fixes up (for debug)
  bool fixFullBasis();

//@}

protected:
  /** \name Protected data members

    \sa CoinWarmStartBasis::Status for a description of the packing used in
    the status arrays.
  */
  //@{
    /// The number of structural variables
    int numStructural_;
    /// The number of artificial variables
    int numArtificial_;
    /// The maximum sise (in ints - actually 4*char) (so resize does not need to do new)
    int maxSize_;
    /** The status of the structural variables. */
    char * structuralStatus_;
    /** The status of the artificial variables. */
    char * artificialStatus_;
  //@}
};


/*! \relates CoinWarmStartBasis
    \brief Get the status of the specified variable in the given status array.
*/

inline CoinWarmStartBasis::Status getStatus(const char *array, int i)  {
  const int st = (array[i>>2] >> ((i&3)<<1)) & 3;
  return static_cast<CoinWarmStartBasis::Status>(st);
}

/*! \relates CoinWarmStartBasis
    \brief Set the status of the specified variable in the given status array.
*/

inline void setStatus(char * array, int i, CoinWarmStartBasis::Status st) {
  char& st_byte = array[i>>2];
  st_byte = static_cast<char>(st_byte & ~(3 << ((i&3)<<1))) ;
  st_byte = static_cast<char>(st_byte | (st << ((i&3)<<1))) ;
}

/*! \relates CoinWarmStartBasis
    \brief Generate a print string for a status code
*/
const char *statusName(CoinWarmStartBasis::Status status) ;


/*! \class CoinWarmStartBasisDiff
    \brief A `diff' between two CoinWarmStartBasis objects

  This class exists in order to hide from the world the details of
  calculating and representing a `diff' between two CoinWarmStartBasis
  objects. For convenience, assignment, cloning, and deletion are visible to
  the world, and default and copy constructors are made available to derived
  classes.  Knowledge of the rest of this structure, and of generating and
  applying diffs, is restricted to the friend functions
  CoinWarmStartBasis::generateDiff() and CoinWarmStartBasis::applyDiff().

  The actual data structure is an unsigned int vector, #difference_ which
  starts with indices of changed and then has values starting after #sze_

  \todo This is a pretty generic structure, and vector diff is a pretty generic
	activity. We should be able to convert this to a template.

  \todo Using unsigned int as the data type for the diff vectors might help
	to contain the damage when this code is inevitably compiled for 64 bit
	architectures. But the notion of int as 4 bytes is hardwired into
	CoinWarmStartBasis, so changes are definitely required.
*/

class CoinWarmStartBasisDiff : public virtual CoinWarmStartDiff
{ public:

  /*! \brief `Virtual constructor' */
  virtual CoinWarmStartDiff *clone() const
  { CoinWarmStartBasisDiff *cwsbd =  new CoinWarmStartBasisDiff(*this) ;
    return (dynamic_cast<CoinWarmStartDiff *>(cwsbd)) ; }

  /*! \brief Assignment */
  virtual
    CoinWarmStartBasisDiff &operator= (const CoinWarmStartBasisDiff &rhs) ;

  /*! \brief Destructor */
  virtual ~CoinWarmStartBasisDiff();

  protected:

  /*! \brief Default constructor
  
    This is protected (rather than private) so that derived classes can
    see it when they make <i>their</i> default constructor protected or
    private.
  */
  CoinWarmStartBasisDiff () : sze_(0), difference_(0) { } 

  /*! \brief Copy constructor
  
    For convenience when copying objects containing CoinWarmStartBasisDiff
    objects. But consider whether you should be using #clone() to retain
    polymorphism.

    This is protected (rather than private) so that derived classes can
    see it when they make <i>their</i> copy constructor protected or
    private.
  */
  CoinWarmStartBasisDiff (const CoinWarmStartBasisDiff &cwsbd) ;

  /*! \brief Standard constructor */
  CoinWarmStartBasisDiff (int sze, const unsigned int *const diffNdxs,
			  const unsigned int *const diffVals) ;

  /*! \brief Constructor when full is smaller than diff!*/
  CoinWarmStartBasisDiff (const CoinWarmStartBasis * rhs);
  
  private:

  friend CoinWarmStartDiff*
    CoinWarmStartBasis::generateDiff(const CoinWarmStart *const oldCWS) const ;
  friend void
    CoinWarmStartBasis::applyDiff(const CoinWarmStartDiff *const diff) ;

  /*! \brief Number of entries (and allocated capacity), in units of \c int. */
  int sze_ ;

  /*! \brief Array of diff indices and diff values */

  unsigned int *difference_ ;

} ;


#endif
