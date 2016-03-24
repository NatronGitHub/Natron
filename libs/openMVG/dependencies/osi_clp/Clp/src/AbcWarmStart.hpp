/* $Id$ */
/*! \legal
  Copyright (C) 2002, International Business Machines
  Corporation and others, Copyright (C) 2012, FasterCoin.  All Rights Reserved.
  This code is licensed under the terms of the Eclipse Public License (EPL).
*/


#ifndef AbcWarmStart_H
#define AbcWarmStart_H
#include "AbcCommon.hpp"
#include "CoinWarmStartBasis.hpp"
// could test using ClpSimplex (or for fans)
#define CLP_WARMSTART
#ifdef CLP_WARMSTART
#include "ClpSimplex.hpp"
#define AbcSimplex ClpSimplex
#else
#include "AbcSimplex.hpp"
#endif
class AbcWarmStart;
//#############################################################################
class AbcWarmStartOrganizer {
public:
  /// Create Basis type 0
  void createBasis0();
  /// Create Basis type 1,2
  void createBasis12();
  /// Create Basis type 3,4
  void createBasis34();
  /// delete basis
  void deleteBasis(AbcWarmStart * basis);
/*! \name Constructors, destructors, and related functions */

//@{

  /** Default constructor

    Creates a warm start object organizer
  */
  AbcWarmStartOrganizer(AbcSimplex * model=NULL);

  /** Copy constructor */
  AbcWarmStartOrganizer(const AbcWarmStartOrganizer& ws) ;

  /** Destructor */
  virtual ~AbcWarmStartOrganizer();

  /** Assignment */

  virtual AbcWarmStartOrganizer& operator=(const AbcWarmStartOrganizer& rhs) ;
//@}

protected:
  /** \name Protected data members */
  //@{
  /// Pointer to AbcSimplex (can only be applied to that)
  AbcSimplex * model_;
  /// Pointer to first basis
  AbcWarmStart * firstBasis_;
  /// Pointer to last basis
  AbcWarmStart * lastBasis_;
  /// Number of bases
  int numberBases_;
  /// Size of bases (extra)
  int sizeBases_;
  //@}
};

/*! \class AbcWarmStart
  As CoinWarmStartBasis but with alternatives
  (Also uses Clp status meaning for slacks)
*/

class AbcWarmStart : public virtual CoinWarmStartBasis {
public:
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
  /// Set model
  inline void setModel(AbcSimplex * model)
  { model_=model;}
  /// Get model
  inline AbcSimplex * model() const
  { return model_;}
  /// Create Basis type 0
  void createBasis0(const AbcSimplex * model);
  /// Create Basis type 12
  void createBasis12(const AbcSimplex * model);
  /// Create Basis type 34
  void createBasis34(const AbcSimplex * model);
/*! \name Constructors, destructors, and related functions */

//@{

  /** Default constructor

    Creates a warm start object representing an empty basis
    (0 rows, 0 columns).
  */
  AbcWarmStart();

  /** Constructs a warm start object with the specified status vectors.

    The parameters are copied.
    Consider assignBasisStatus(int,int,char*&,char*&) if the object should
    assume ownership.

    \sa AbcWarmStart::Status for a description of the packing used in
    the status arrays.
  */
  AbcWarmStart(AbcSimplex * model,int type) ;

  /** Copy constructor */
  AbcWarmStart(const AbcWarmStart& ws) ;

  /** `Virtual constructor' */
  virtual CoinWarmStart *clone() const
  {
     return new AbcWarmStart(*this);
  }

  /** Destructor */
  virtual ~AbcWarmStart();

  /** Assignment */

  virtual AbcWarmStart& operator=(const AbcWarmStart& rhs) ;

  /** Assign the status vectors to be the warm start information.
  
      In this method the AbcWarmStart object assumes ownership of the
      pointers and upon return the argument pointers will be NULL.
      If copying is desirable, use the
      \link AbcWarmStart(int,int,const char*,const char*)
	    array constructor \endlink
      or the
      \link operator=(const AbcWarmStart&)
	    assignment operator \endlink.

      \note
      The pointers passed to this method will be
      freed using delete[], so they must be created using new[].
  */
  virtual void assignBasisStatus(int ns, int na, char*& sStat, char*& aStat) ;
//@}

protected:
  /** \name Protected data members */
  //@{
  /** Type of basis (always status arrays)
  0 - as CoinWarmStartBasis
  1,2 - plus factor order as shorts or ints (top bit set means column)
  3,4 - plus compact saved factorization
  add 8 to say steepest edge weights stored (as floats)
  may want to change next,previous to tree info
  so can use a different basis for weights
  */
  int typeExtraInformation_;
  /// Length of extra information in bytes
  int lengthExtraInformation_;
  /// The extra information. 
  char * extraInformation_;
  /// Pointer back to AbcSimplex (can only be applied to that)
  AbcSimplex * model_;
  /// Pointer back to AbcWarmStartOrganizer for organization
  AbcWarmStartOrganizer * organizer_;
  /// Pointer to previous basis
  AbcWarmStart * previousBasis_;
  /// Pointer to next basis
  AbcWarmStart * nextBasis_;
  /// Sequence stamp for deletion
  int stamp_;
  /** Number of valid rows (rest should have slacks)
      Check to see if weights are OK for these rows
      and then just btran new ones for weights
   */
  int numberValidRows_;
  //@}
};
#endif
