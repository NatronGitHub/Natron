/* $Id: CoinPresolveMatrix.hpp 1581 2013-04-06 12:48:50Z stefan $ */
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef CoinPresolveMatrix_H
#define CoinPresolveMatrix_H

#include "CoinPragma.hpp"
#include "CoinPackedMatrix.hpp"
#include "CoinMessage.hpp"
#include "CoinTime.hpp"

#include <cmath>
#include <cassert>
#include <cfloat>
#include <cassert>
#include <cstdlib>

#if PRESOLVE_DEBUG > 0
#include "CoinFinite.hpp"
#endif

/*! \file

  Declarations for CoinPresolveMatrix and CoinPostsolveMatrix and their
  common base class CoinPrePostsolveMatrix. Also declarations for
  CoinPresolveAction and a number of non-member utility functions.
*/


#if defined(_MSC_VER)
// Avoid MS Compiler problem in recognizing type to delete
// by casting to type.
// Is this still necessary? -- lh, 111202 --
#define deleteAction(array,type) delete [] ((type) array)
#else
#define deleteAction(array,type) delete [] array
#endif

/*
  Define PRESOLVE_DEBUG and PRESOLVE_CONSISTENCY on the configure command
  line or in a Makefile!  See comments in CoinPresolvePsdebug.hpp.
*/
#if PRESOLVE_DEBUG > 0 || PRESOLVE_CONSISTENCY > 0

#define PRESOLVE_STMT(s) s

#define PRESOLVEASSERT(x) \
  ((x) ? 1 : ((std::cerr << "FAILED ASSERTION at line " \
			 << __LINE__ << ": " #x "\n"), abort(), 0))

inline void DIE(const char *s) { std::cout << s ; abort() ; }

/*! \brief Indicate column or row present at start of postsolve

  This code is used during postsolve in [cr]done to indicate columns and rows
  that are present in the presolved system (i.e., present at the start of
  postsolve processing).

  \todo
  There are a bunch of these code definitions, scattered through presolve
  files. They should be collected in one place.
*/
#define PRESENT_IN_REDUCED	'\377'

#else

#define PRESOLVEASSERT(x) {} 
#define	PRESOLVE_STMT(s) {}

inline void DIE(const char *) {}

#endif

/*
  Unclear why these are separate from standard debug.
*/
#ifndef PRESOLVE_DETAIL
#define PRESOLVE_DETAIL_PRINT(s) {}
#else
#define PRESOLVE_DETAIL_PRINT(s) s
#endif

/*! \brief Zero tolerance

  OSL had a fixed zero tolerance; we still use that here.
*/
const double ZTOLDP = 1e-12 ;
/*! \brief Alternate zero tolerance

  Use a different one if we are doing doubletons, etc.
*/
const double ZTOLDP2 = 1e-10 ;

/// The usual finite infinity
#define PRESOLVE_INF COIN_DBL_MAX
/// And a small infinity
#define PRESOLVE_SMALL_INF 1.0e20
/// Check for infinity using finite infinity
#define	PRESOLVEFINITE(n) (-PRESOLVE_INF < (n) && (n) < PRESOLVE_INF)


class CoinPostsolveMatrix ;

/*! \class CoinPresolveAction
    \brief Abstract base class of all presolve routines.

  The details will make more sense after a quick overview of the grand plan:
  A presolve object is handed a problem object, which it is expected to
  modify in some useful way.  Assuming that it succeeds, the presolve object
  should create a postsolve object, <i>i.e.</i>, an object that contains
  instructions for backing out the presolve transform to recover the original
  problem. These postsolve objects are accumulated in a linked list, with each
  successive presolve action adding its postsolve action to the head of the
  list. The end result of all this is a presolved problem object, and a list
  of postsolve objects. The presolved problem object is then handed to a
  solver for optimization, and the problem object augmented with the
  results.  The list of postsolve objects is then traversed. Each of them
  (un)modifies the problem object, with the end result being the original
  problem, augmented with solution information.

  The problem object representation is CoinPrePostsolveMatrix and subclasses.
  Check there for details. The \c CoinPresolveAction class and subclasses
  represent the presolve and postsolve objects.

  In spite of the name, the only information held in a \c CoinPresolveAction
  object is the information needed to postsolve (<i>i.e.</i>, the information
  needed to back out the presolve transformation). This information is not
  expected to change, so the fields are all \c const.

  A subclass of \c CoinPresolveAction, implementing a specific pre/postsolve
  action, is expected to declare a static function that attempts to perform a
  presolve transformation. This function will be handed a CoinPresolveMatrix
  to transform, and a pointer to the head of the list of postsolve objects.
  If the transform is successful, the function will create a new
  \c CoinPresolveAction object, link it at the head of the list of postsolve
  objects, and return a pointer to the postsolve object it has just created.
  Otherwise, it should return 0. It is expected that these static functions
  will be the only things that can create new \c CoinPresolveAction objects;
  this is expressed by making each subclass' constructor(s) private.

  Every subclass must also define a \c postsolve method.
  This function will be handed a CoinPostsolveMatrix to transform.

  It is the client's responsibility to implement presolve and postsolve driver
  routines. See OsiPresolve for examples.

  \note Since the only fields in a \c CoinPresolveAction are \c const, anything
	one can do with a variable declared \c CoinPresolveAction* can also be
	done with a variable declared \c const \c CoinPresolveAction* It is
	expected that all derived subclasses of \c CoinPresolveAction also have
	this property.
*/
class CoinPresolveAction
{
 public:
  /*! \brief Stub routine to throw exceptions.
  
   Exceptions are inefficient, particularly with g++.  Even with xlC, the
   use of exceptions adds a long prologue to a routine.  Therefore, rather
   than use throw directly in the routine, I use it in a stub routine.
  */
  static void throwCoinError(const char *error, const char *ps_routine)
  { throw CoinError(error, ps_routine, "CoinPresolve"); } 

  /*! \brief The next presolve transformation
  
    Set at object construction.
  */
  const CoinPresolveAction *next;
  
  /*! \brief Construct a postsolve object and add it to the transformation list.
  
    This is an `add to head' operation. This object will point to the
    one passed as the parameter.
  */
  CoinPresolveAction(const CoinPresolveAction *next) : next(next) {}
  /// modify next (when building rather than passing)
  inline void setNext(const CoinPresolveAction *nextAction)
  { next = nextAction;}

  /*! \brief A name for debug printing.

    It is expected that the name is not stored in the transform itself.
  */
  virtual const char *name() const = 0;

  /*! \brief Apply the postsolve transformation for this particular
	     presolve action.
  */
  virtual void postsolve(CoinPostsolveMatrix *prob) const = 0;

  /*! \brief Virtual destructor. */
  virtual ~CoinPresolveAction() {}
};

/*
  These are needed for OSI-aware constructors associated with
  CoinPrePostsolveMatrix, CoinPresolveMatrix, and CoinPostsolveMatrix.
*/
class ClpSimplex;
class OsiSolverInterface;

/*
  CoinWarmStartBasis is required for methods in CoinPrePostsolveMatrix
  that accept/return a CoinWarmStartBasis object.
*/
class CoinWarmStartBasis ;

/*! \class CoinPrePostsolveMatrix
    \brief Collects all the information about the problem that is needed
	   in both presolve and postsolve.
    
    In a bit more detail, a column-major representation of the constraint
    matrix and upper and lower bounds on variables and constraints, plus row
    and column solutions, reduced costs, and status. There's also a set of
    arrays holding the original row and column numbers.

    As presolve and postsolve transform the matrix, it will occasionally be
    necessary to expand the number of entries in a column. There are two
    aspects:
    <ul>
      <li> During postsolve, the constraint system is expected to grow as
	   the smaller presolved system is transformed back to the original
	   system.
      <li> During both pre- and postsolve, transforms can increase the number
	   of coefficients in a row or column. (See the 
	   variable substitution, doubleton, and tripleton transforms.)
    </ul>

    The first is addressed by the members #ncols0_, #nrows0_, and #nelems0_.
    These should be set (via constructor parameters) to values large enough
    for the largest size taken on by the constraint system. Typically, this
    will be the size of the original constraint system.

    The second is addressed by a generous allocation of extra (empty) space
    for the arrays used to hold coefficients and row indices. When columns
    must be expanded, they are moved into the empty space. When it is used up,
    the arrays are compacted. When compaction fails to produce sufficient
    space, presolve/postsolve will fail.

    CoinPrePostsolveMatrix isn't really intended to be used `bare' --- the
    expectation is that it'll be used through CoinPresolveMatrix or
    CoinPostsolveMatrix. Some of the functions needed to load a problem are
    defined in the derived classes.

  When CoinPresolve is applied when reoptimising, we need to be prepared to
  accept a basis and modify it in step with the presolve actions (otherwise
  we throw away all the advantages of warm start for reoptimization). But
  other solution components (#acts_, #rowduals_, #sol_, and #rcosts_) are
  needed only for postsolve, where they're used in places to determine the
  proper action(s) when restoring rows or columns.  If presolve is provided
  with a solution, it will modify it in step with the presolve actions.
  Moving the solution components from CoinPrePostsolveMatrix to
  CoinPostsolveMatrix would break a lot of code.  It's not clear that it's
  worth it, and it would preclude upgrades to the presolve side that might
  make use of any of these.  -- lh, 080501 --

  The constructors that take an OSI or ClpSimplex as a parameter really should
  not be here, but for historical reasons they will likely remain for the
  forseeable future.  -- lh, 111202 --
*/

class CoinPrePostsolveMatrix
{
 public:

  /*! \name Constructors & Destructors */

  //@{
  /*! \brief `Native' constructor

    This constructor creates an empty object which must then be loaded. On
    the other hand, it doesn't assume that the client is an
    OsiSolverInterface.
  */
  CoinPrePostsolveMatrix(int ncols_alloc, int nrows_alloc,
			 CoinBigIndex nelems_alloc) ;

  /*! \brief Generic OSI constructor

    See OSI code for the definition.
  */
  CoinPrePostsolveMatrix(const OsiSolverInterface * si,
			int ncols_,
			int nrows_,
			CoinBigIndex nelems_);

  /*! ClpOsi constructor

    See Clp code for the definition.
  */
  CoinPrePostsolveMatrix(const ClpSimplex * si,
			int ncols_,
			int nrows_,
			CoinBigIndex nelems_,
                         double bulkRatio);

  /// Destructor
  ~CoinPrePostsolveMatrix();
  //@}

  /*! \brief Enum for status of various sorts
  
    Matches CoinWarmStartBasis::Status and adds superBasic. Most code that
    converts between CoinPrePostsolveMatrix::Status and
    CoinWarmStartBasis::Status will break if this correspondence is broken.

    superBasic is an unresolved problem: there's no analogue in
    CoinWarmStartBasis::Status.
  */
  enum Status {
    isFree = 0x00,
    basic = 0x01,
    atUpperBound = 0x02,
    atLowerBound = 0x03,
    superBasic = 0x04
  };

  /*! \name Functions to work with variable status

    Functions to work with the CoinPrePostsolveMatrix::Status enum and
    related vectors.

    \todo
    Why are we futzing around with three bit status? A holdover from the
    packed arrays of CoinWarmStartBasis? Big swaths of the presolve code
    manipulates colstat_ and rowstat_ as unsigned char arrays using simple
    assignment to set values.
  */
  //@{
  
  /// Set row status (<i>i.e.</i>, status of artificial for this row)
  inline void setRowStatus(int sequence, Status status)
  {
    unsigned char & st_byte = rowstat_[sequence];
    st_byte = static_cast<unsigned char>(st_byte & (~7)) ;
    st_byte = static_cast<unsigned char>(st_byte | status) ;
  }
  /// Get row status
  inline Status getRowStatus(int sequence) const
  {return static_cast<Status> (rowstat_[sequence]&7);}
  /// Check if artificial for this row is basic
  inline bool rowIsBasic(int sequence) const
  {return (static_cast<Status> (rowstat_[sequence]&7)==basic);}
  /// Set column status (<i>i.e.</i>, status of primal variable)
  inline void setColumnStatus(int sequence, Status status)
  {
    unsigned char & st_byte = colstat_[sequence];
    st_byte = static_cast<unsigned char>(st_byte & (~7)) ;
    st_byte = static_cast<unsigned char>(st_byte | status) ;

#   ifdef PRESOLVE_DEBUG
    switch (status)
    { case isFree:
      { if (clo_[sequence] > -PRESOLVE_INF || cup_[sequence] < PRESOLVE_INF)
	{ std::cout << "Bad status: Var " << sequence
		    << " isFree, lb = " << clo_[sequence]
		    << ", ub = " << cup_[sequence] << std::endl ; }
	break ; }
      case basic:
      { break ; }
      case atUpperBound:
      { if (cup_[sequence] >= PRESOLVE_INF)
	{ std::cout << "Bad status: Var " << sequence
	            << " atUpperBound, lb = " << clo_[sequence]
	            << ", ub = " << cup_[sequence] << std::endl ; }
	break ; }
      case atLowerBound:
      { if (clo_[sequence] <= -PRESOLVE_INF)
	{ std::cout << "Bad status: Var " << sequence
	            << " atLowerBound, lb = " << clo_[sequence]
	            << ", ub = " << cup_[sequence] << std::endl ; }
	break ; }
      case superBasic:
      { if (clo_[sequence] <= -PRESOLVE_INF && cup_[sequence] >= PRESOLVE_INF)
	{ std::cout << "Bad status: Var " << sequence
	            << " superBasic, lb = " << clo_[sequence]
	            << ", ub = " << cup_[sequence] << std::endl ; }
	break ; }
      default:
      { assert(false) ;
	break ; } }
#   endif
  }
  /// Get column (structural variable) status
  inline Status getColumnStatus(int sequence) const
  {return static_cast<Status> (colstat_[sequence]&7);}
  /// Check if column (structural variable) is basic
  inline bool columnIsBasic(int sequence) const
  {return (static_cast<Status> (colstat_[sequence]&7)==basic);}
  /*! \brief Set status of row (artificial variable) to the correct nonbasic
	     status given bounds and current value
  */
  void setRowStatusUsingValue(int iRow);
  /*! \brief Set status of column (structural variable) to the correct
	     nonbasic status given bounds and current value
  */
  void setColumnStatusUsingValue(int iColumn);
  /*! \brief Set column (structural variable) status vector */
  void setStructuralStatus(const char *strucStatus, int lenParam) ;
  /*! \brief Set row (artificial variable) status vector */
  void setArtificialStatus(const char *artifStatus, int lenParam) ;
  /*! \brief Set the status of all variables from a basis */
  void setStatus(const CoinWarmStartBasis *basis) ;
  /*! \brief Get status in the form of a CoinWarmStartBasis */
  CoinWarmStartBasis *getStatus() ;
  /*! \brief Return a print string for status of a column (structural
	     variable)
  */
  const char *columnStatusString(int j) const ;
  /*! \brief Return a print string for status of a row (artificial
	     variable)
  */
  const char *rowStatusString(int i) const ;
  //@}

  /*! \name Functions to load problem and solution information

    These functions can be used to load portions of the problem definition
    and solution. See also the CoinPresolveMatrix and CoinPostsolveMatrix
    classes.
  */
  //@{
  /// Set the objective function offset for the original system.
  void setObjOffset(double offset) ;
  /*! \brief Set the objective sense (max/min)

    Coded as 1.0 for min, -1.0 for max.
    Yes, there's a method, and a matching attribute. No, you really
    don't want to set this to maximise.
  */
  void setObjSense(double objSense) ;
  /// Set the primal feasibility tolerance
  void setPrimalTolerance(double primTol) ;
  /// Set the dual feasibility tolerance
  void setDualTolerance(double dualTol) ;
  /// Set column lower bounds
  void setColLower(const double *colLower, int lenParam) ;
  /// Set column upper bounds
  void setColUpper(const double *colUpper, int lenParam) ;
  /// Set column solution
  void setColSolution(const double *colSol, int lenParam) ;
  /// Set objective coefficients
  void setCost(const double *cost, int lenParam) ;
  /// Set reduced costs
  void setReducedCost(const double *redCost, int lenParam) ;
  /// Set row lower bounds
  void setRowLower(const double *rowLower, int lenParam) ;
  /// Set row upper bounds
  void setRowUpper(const double *rowUpper, int lenParam) ;
  /// Set row solution
  void setRowPrice(const double *rowSol, int lenParam) ;
  /// Set row activity
  void setRowActivity(const double *rowAct, int lenParam) ;
  //@}

  /*! \name Functions to retrieve problem and solution information */
  //@{
  /// Get current number of columns
  inline int getNumCols() const
  { return (ncols_) ; } 
  /// Get current number of rows
  inline int getNumRows() const
  { return (nrows_) ; }
  /// Get current number of non-zero coefficients
  inline int getNumElems() const
  { return (nelems_) ; }
  /// Get column start vector for column-major packed matrix
  inline const CoinBigIndex *getColStarts() const
  { return (mcstrt_) ; } 
  /// Get column length vector for column-major packed matrix
  inline const int *getColLengths() const
  { return (hincol_) ; } 
  /// Get vector of row indices for column-major packed matrix
  inline const int *getRowIndicesByCol() const
  { return (hrow_) ; } 
  /// Get vector of elements for column-major packed matrix
  inline const double *getElementsByCol() const
  { return (colels_) ; } 
  /// Get column lower bounds
  inline const double *getColLower() const
  { return (clo_) ; } 
  /// Get column upper bounds
  inline const double *getColUpper() const
  { return (cup_) ; } 
  /// Get objective coefficients
  inline const double *getCost() const
  { return (cost_) ; } 
  /// Get row lower bounds
  inline const double *getRowLower() const
  { return (rlo_) ; } 
  /// Get row upper bounds
  inline const double *getRowUpper() const
  { return (rup_) ; } 
  /// Get column solution (primal variable values)
  inline const double *getColSolution() const
  { return (sol_) ; }
  /// Get row activity (constraint lhs values)
  inline const double *getRowActivity() const
  { return (acts_) ; }
  /// Get row solution (dual variables)
  inline const double *getRowPrice() const
  { return (rowduals_) ; }
  /// Get reduced costs
  inline const double *getReducedCost() const
  { return (rcosts_) ; }
  /// Count empty columns
  inline int countEmptyCols()
  { int empty = 0 ;
    for (int i = 0 ; i < ncols_ ; i++) if (hincol_[i] == 0) empty++ ;
    return (empty) ; }
  //@}


  /*! \name Message handling */
  //@{
  /// Return message handler
  inline CoinMessageHandler *messageHandler() const 
  { return handler_; }
  /*! \brief Set message handler

    The client retains responsibility for the handler --- it will not be
    destroyed with the \c CoinPrePostsolveMatrix object.
  */
  inline void setMessageHandler(CoinMessageHandler *handler)
  { if (defaultHandler_ == true)
    { delete handler_ ;
      defaultHandler_ = false ; }
    handler_ = handler ; }
  /// Return messages
  inline CoinMessages messages() const 
  { return messages_; }
  //@}

  /*! \name Current and Allocated Size

    During pre- and postsolve, the matrix will change in size. During presolve
    it will shrink; during postsolve it will grow. Hence there are two sets of
    size variables, one for the current size and one for the allocated size.
    (See the general comments for the CoinPrePostsolveMatrix class for more
    information.)
  */
  //@{

  /// current number of columns
  int ncols_;
  /// current number of rows
  int nrows_;
  /// current number of coefficients
  CoinBigIndex nelems_;

  /// Allocated number of columns
  int ncols0_;
  /// Allocated number of rows
  int nrows0_ ;
  /// Allocated number of coefficients
  CoinBigIndex nelems0_ ;
  /*! \brief Allocated size of bulk storage for row indices and coefficients

    This is the space allocated for hrow_ and colels_.  This must be large
    enough to allow columns to be copied into empty space when they need to
    be expanded.  For efficiency (to minimize the number of times the
    representation must be compressed) it's recommended that this be at least
    2*nelems0_.
  */
  CoinBigIndex bulk0_ ;
  /// Ratio of bulk0_ to nelems0_; default is 2.
  double bulkRatio_;
  //@}

  /*! \name Problem representation

    The matrix is the common column-major format: A pair of vectors with
    positional correspondence to hold coefficients and row indices, and a
    second pair of vectors giving the starting position and length of each
    column in the first pair.
  */
  //@{
  /// Vector of column start positions in #hrow_, #colels_
  CoinBigIndex *mcstrt_;
  /// Vector of column lengths
  int *hincol_;
  /// Row indices (positional correspondence with #colels_)
  int *hrow_;
  /// Coefficients (positional correspondence with #hrow_)
  double *colels_;

  /// Objective coefficients
  double *cost_;
  /// Original objective offset
  double originalOffset_;

  /// Column (primal variable) lower bounds
  double *clo_;
  /// Column (primal variable) upper bounds
  double *cup_;

  /// Row (constraint) lower bounds
  double *rlo_;
  /// Row (constraint) upper bounds
  double *rup_;

  /*! \brief Original column numbers

    Over the current range of column numbers in the presolved problem,
    the entry for column j will contain the index of the corresponding
    column in the original problem.
  */
  int * originalColumn_;
  /*! \brief Original row numbers

    Over the current range of row numbers in the presolved problem, the
    entry for row i will contain the index of the corresponding row in
    the original problem.
  */
  int * originalRow_;

  /// Primal feasibility tolerance
  double ztolzb_;
  /// Dual feasibility tolerance
  double ztoldj_;

  /*! \brief Maximization/minimization

    Yes, there's a variable here. No, you really don't want to set this to
    maximise. See the main notes for CoinPresolveMatrix.
  */
  double maxmin_;
  //@}

  /*! \name Problem solution information
   
    The presolve phase will work without any solution information
    (appropriate for initial optimisation) or with solution information
    (appropriate for reoptimisation).  When solution information is supplied,
    presolve will maintain it to the best of its ability.  #colstat_ is
    checked to determine the presence/absence of status information. #sol_ is
    checked for primal solution information, and #rowduals_ for dual solution
    information.

    The postsolve phase requires the complete solution information from the
    presolved problem (status, primal and dual solutions). It will be
    transformed into a correct solution for the original problem.
  */
  //@{
  /*! \brief Vector of primal variable values

    If #sol_ exists, it is assumed that primal solution information should be
    updated and that #acts_ also exists.
  */
  double *sol_;
  /*! \brief Vector of dual variable values

    If #rowduals_ exists, it is assumed that dual solution information should
    be updated and that #rcosts_ also exists.
  */
  double *rowduals_;
  /*! \brief Vector of constraint left-hand-side values (row activity)
  
    Produced by evaluating constraints according to #sol_. Updated iff
    #sol_ exists.
  */
  double *acts_;
  /*! \brief Vector of reduced costs
  
    Produced by evaluating dual constraints according to #rowduals_. Updated
    iff #rowduals_ exists.
  */
  double *rcosts_;

  /*! \brief Status of primal variables

    Coded with CoinPrePostSolveMatrix::Status, one code per char. colstat_ and
    #rowstat_ <b>MUST</b> be allocated as a single vector. This is to maintain
    compatibility with ClpPresolve and OsiPresolve, which do it this way.
  */
  unsigned char *colstat_;

  /*! \brief Status of constraints

    More accurately, the status of the logical variable associated with the
    constraint. Coded with CoinPrePostSolveMatrix::Status, one code per char.
    Note that this must be allocated as a single vector with #colstat_.
  */
  unsigned char *rowstat_;
  //@}

  /*! \name Message handling

    Uses the standard COIN approach: a default handler is installed, and the
    CoinPrePostsolveMatrix object takes responsibility for it. If the client
    replaces the handler with one of their own, it becomes their
    responsibility.
  */
  //@{
  /// Message handler
  CoinMessageHandler *handler_; 
  /// Indicates if the current #handler_ is default (true) or not (false).
  bool defaultHandler_;
  /// Standard COIN messages
  CoinMessage messages_; 
  //@}

};

/*! \relates CoinPrePostsolveMatrix
    \brief Generate a print string for a status code.
*/
const char *statusName (CoinPrePostsolveMatrix::Status status) ;


/*! \class presolvehlink
    \brief Links to aid in packed matrix modification

   Currently, the matrices held by the CoinPrePostsolveMatrix and
   CoinPresolveMatrix objects are represented in the same way as a
   CoinPackedMatrix. In the course of presolve and postsolve transforms, it
   will happen that a major-dimension vector needs to increase in size. In
   order to check whether there is enough room to add another coefficient in
   place, it helps to know the next vector (in memory order) in the bulk
   storage area. To do that, a linked list of major-dimension vectors is
   maintained; the "pre" and "suc" fields give the previous and next vector,
   in memory order (that is, the vector whose mcstrt_ or mrstrt_ entry is
   next smaller or larger).

   Consider a column-major matrix with ncols columns. By definition,
   presolvehlink[ncols].pre points to the column in the last occupied
   position of the bulk storage arrays. There is no easy way to find the
   column which occupies the first position (there is no presolvehlink[-1] to
   consult). If the column that initially occupies the first position is
   moved for expansion, there is no way to reclaim the space until the bulk
   storage is compacted.  The same holds for the last and first rows of a
   row-major matrix, of course.
*/

class presolvehlink
{ public:
  int pre, suc;
} ;

#define NO_LINK -66666666

/*! \relates presolvehlink
    \brief unlink vector i

  Remove vector i from the ordering.
*/
inline void PRESOLVE_REMOVE_LINK(presolvehlink *link, int i)
{ 
  int ipre = link[i].pre;
  int isuc = link[i].suc;
  if (ipre >= 0) {
    link[ipre].suc = isuc;
  }
  if (isuc >= 0) {
    link[isuc].pre = ipre;
  }
  link[i].pre = NO_LINK, link[i].suc = NO_LINK;
}

/*! \relates presolvehlink
    \brief insert vector i after vector j

  Insert vector i between j and j.suc.
*/
inline void PRESOLVE_INSERT_LINK(presolvehlink *link, int i, int j)
{
  int isuc = link[j].suc;
  link[j].suc = i;
  link[i].pre = j;
  if (isuc >= 0) {
    link[isuc].pre = i;
  }
  link[i].suc = isuc;
}

/*! \relates presolvehlink
    \brief relink vector j in place of vector i

   Replace vector i in the ordering with vector j. This is equivalent to
   <pre>
     int pre = link[i].pre;
     PRESOLVE_REMOVE_LINK(link,i);
     PRESOLVE_INSERT_LINK(link,j,pre);
   </pre>
   But, this routine will work even if i happens to be first in the order.
*/
inline void PRESOLVE_MOVE_LINK(presolvehlink *link, int i, int j)
{ 
  int ipre = link[i].pre;
  int isuc = link[i].suc;
  if (ipre >= 0) {
    link[ipre].suc = j;
  }
  if (isuc >= 0) {
    link[isuc].pre = j;
  }
  link[i].pre = NO_LINK, link[i].suc = NO_LINK;
}


/*! \class CoinPresolveMatrix
    \brief Augments CoinPrePostsolveMatrix with information about the problem
	   that is only needed during presolve.

  For problem manipulation, this class adds a row-major matrix
  representation, linked lists that allow for easy manipulation of the matrix
  when applying presolve transforms, and vectors to track row and column
  processing status (changed, needs further processing, change prohibited)

  For problem representation, this class adds information about variable type
  (integer or continuous), an objective offset, and a feasibility tolerance.

  <b>NOTE</b> that the #anyInteger_ and #anyProhibited_ flags are independent
  of the vectors used to track this information for individual variables
  (#integerType_ and #rowChanged_ and #colChanged_, respectively).

  <b>NOTE</b> also that at the end of presolve the column-major and row-major
  matrix representations are loosely packed (<i>i.e.</i>, there may be gaps
  between columns in the bulk storage arrays).

  <b>NOTE</b> that while you might think that CoinPresolve is prepared to
  handle minimisation or maximisation, it's unlikely that this still works.
  This is a good thing: better to convert objective coefficients and duals
  once, before starting presolve, rather than doing it over and over in
  each transform that considers dual variables.

  The constructors that take an OSI or ClpSimplex as a parameter really should
  not be here, but for historical reasons they will likely remain for the
  forseeable future.  -- lh, 111202 --
*/

class CoinPresolveMatrix : public CoinPrePostsolveMatrix
{
 public:

  /*! \brief `Native' constructor

    This constructor creates an empty object which must then be loaded.
    On the other hand, it doesn't assume that the client is an
    OsiSolverInterface.
  */
  CoinPresolveMatrix(int ncols_alloc, int nrows_alloc,
		     CoinBigIndex nelems_alloc) ;

  /*! \brief Clp OSI constructor

    See Clp code for the definition.
  */
  CoinPresolveMatrix(int ncols0,
		    double maxmin,
		    // end prepost members

		    ClpSimplex * si,

		    // rowrep
		    int nrows,
		    CoinBigIndex nelems,
		 bool doStatus,
		 double nonLinearVariable,
                     double bulkRatio);

  /*! \brief Update the model held by a Clp OSI */
  void update_model(ClpSimplex * si,
			    int nrows0,
			    int ncols0,
			    CoinBigIndex nelems0);
  /*! \brief Generic OSI constructor

    See OSI code for the definition.
  */
  CoinPresolveMatrix(int ncols0,
		     double maxmin,
		     // end prepost members
		     OsiSolverInterface * si,
		     // rowrep
		     int nrows,
		     CoinBigIndex nelems,
		     bool doStatus,
		     double nonLinearVariable,
                     const char * prohibited,
		     const char * rowProhibited=NULL);

  /*! \brief Update the model held by a generic OSI */
  void update_model(OsiSolverInterface * si,
			    int nrows0,
			    int ncols0,
			    CoinBigIndex nelems0);

  /// Destructor
  ~CoinPresolveMatrix();

  /*! \brief Initialize a CoinPostsolveMatrix object, destroying the
	     CoinPresolveMatrix object.

    See CoinPostsolveMatrix::assignPresolveToPostsolve.
  */
  friend void assignPresolveToPostsolve (CoinPresolveMatrix *&preObj) ;

  /*! \name Functions to load the problem representation
  */
  //@{
  /*! \brief Load the cofficient matrix.

    Load the coefficient matrix before loading the other vectors (bounds,
    objective, variable type) required to define the problem.
  */
  void setMatrix(const CoinPackedMatrix *mtx) ;

  /// Count number of empty rows
  inline int countEmptyRows()
  { int empty = 0 ;
    for (int i = 0 ; i < nrows_ ; i++) if (hinrow_[i] == 0) empty++ ;
    return (empty) ; }

  /*! \brief Set variable type information for a single variable

    Set \p variableType to 0 for continous, 1 for integer.
    Does not manipulate the #anyInteger_ flag.
  */
  inline void setVariableType(int i, int variableType)
  { if (integerType_ == 0) integerType_ = new unsigned char [ncols0_] ;
    integerType_[i] = static_cast<unsigned char>(variableType) ; }

  /*! \brief Set variable type information for all variables
  
    Set \p variableType[i] to 0 for continuous, 1 for integer.
    Does not manipulate the #anyInteger_ flag.
  */
  void setVariableType(const unsigned char *variableType, int lenParam) ;

  /*! \brief Set the type of all variables

    allIntegers should be true to set the type to integer, false to set the
    type to continuous.
  */
  void setVariableType (bool allIntegers, int lenParam) ;

  /// Set a flag for presence (true) or absence (false) of integer variables
  inline void setAnyInteger (bool anyInteger = true)
  { anyInteger_ = anyInteger ; }
  //@}

  /*! \name Functions to retrieve problem information
  */
  //@{

  /// Get row start vector for row-major packed matrix
  inline const CoinBigIndex *getRowStarts() const
  { return (mrstrt_) ; }
  /// Get vector of column indices for row-major packed matrix
  inline const int *getColIndicesByRow() const
  { return (hcol_) ; }
  /// Get vector of elements for row-major packed matrix
  inline const double *getElementsByRow() const
  { return (rowels_) ; }

  /*! \brief Check for integrality of the specified variable.

    Consults the #integerType_ vector if present; fallback is the
    #anyInteger_ flag.
  */
  inline bool isInteger (int i) const
  { if (integerType_ == 0)
    { return (anyInteger_) ; }
    else
    if (integerType_[i] == 1)
    { return (true) ; }
    else
    { return (false) ; } }

  /*! \brief Check if there are any integer variables

    Consults the #anyInteger_ flag
  */
  inline bool anyInteger () const
  { return (anyInteger_) ; }
  /// Picks up any special options
  inline int presolveOptions() const
  { return presolveOptions_;}
  /// Sets any special options (see #presolveOptions_)
  inline void setPresolveOptions(int value)
  { presolveOptions_=value;}
  //@}

  /*! \name Matrix storage management links
  
    Linked lists, modelled after the linked lists used in OSL
    factorization. They are used for management of the bulk coefficient
    and minor index storage areas.
  */
  //@{
  /// Linked list for the column-major representation.
  presolvehlink *clink_;
  /// Linked list for the row-major representation.
  presolvehlink *rlink_;
  //@}

  /// Objective function offset introduced during presolve
  double dobias_ ;

  /// Adjust objective function constant offset
  inline void change_bias(double change_amount)
  {
    dobias_ += change_amount ;
  # if PRESOLVE_DEBUG > 2
    assert(fabs(change_amount)<1.0e50) ;
    if (change_amount)
      PRESOLVE_STMT(printf("changing bias by %g to %g\n",
			    change_amount, dobias_)) ;
  # endif
  }

  /*! \name Row-major representation

    Common row-major format: A pair of vectors with positional
    correspondence to hold coefficients and column indices, and a second pair
    of vectors giving the starting position and length of each row in
    the first pair.
  */
  //@{
  /// Vector of row start positions in #hcol, #rowels_
  CoinBigIndex *mrstrt_;
  /// Vector of row lengths
  int *hinrow_;
  /// Coefficients (positional correspondence with #hcol_)
  double *rowels_;
  /// Column indices (positional correspondence with #rowels_)
  int *hcol_;
  //@}

  /// Tracks integrality of columns (1 for integer, 0 for continuous)
  unsigned char *integerType_;
  /*! \brief Flag to say if any variables are integer

    Note that this flag is <i>not</i> manipulated by the various
    \c setVariableType routines.
  */
  bool anyInteger_ ;
  /// Print statistics for tuning
  bool tuning_;
  /// Say we want statistics - also set time
  void statistics();
  /// Start time of presolve
  double startTime_;

  /// Bounds can be moved by this to retain feasibility
  double feasibilityTolerance_;
  /// Return feasibility tolerance
  inline double feasibilityTolerance()
  { return (feasibilityTolerance_) ; }
  /// Set feasibility tolerance
  inline void setFeasibilityTolerance (double val)
  { feasibilityTolerance_ = val ; }

  /*! \brief Output status: 0 = feasible, 1 = infeasible, 2 = unbounded

    Actually implemented as single bit flags: 1^0 = infeasible, 1^1 =
    unbounded.
  */
  int status_;
  /// Returns problem status (0 = feasible, 1 = infeasible, 2 = unbounded)
  inline int status()
  { return (status_) ; }
  /// Set problem status
  inline void setStatus(int status)
  { status_ = (status&0x3) ; }

  /*! \brief Presolve pass number

    Should be incremented externally by the method controlling application of
    presolve transforms.
    Used to control the execution of testRedundant (evoked by the
    implied_free transform).
  */
  int pass_;
  /// Set pass number
  inline void setPass (int pass = 0)
  { pass_ = pass ; }

  /*! \brief Maximum substitution level

    Used to control the execution of subst from implied_free
  */
  int maxSubstLevel_;
  /// Set Maximum substitution level (normally 3)
  inline void setMaximumSubstitutionLevel (int level)
  { maxSubstLevel_ = level ; }


  /*! \name Row and column processing status

    Information used to determine if rows or columns can be changed and
    if they require further processing due to changes.

    There are four major lists: the [row,col]ToDo list, and the
    [row,col]NextToDo list.  In general, a transform processes entries from
    the ToDo list and adds entries to the NextToDo list.

    There are two vectors, [row,col]Changed, which track the status of
    individual rows and columns.
  */
  //@{
  /*! \brief Column change status information

    Coded using the following bits:
    <ul>
      <li> 0x01: Column has changed
      <li> 0x02: preprocessing prohibited
      <li> 0x04: Column has been used
      <li> 0x08: Column originally had infinite ub
    </ul>
  */
  unsigned char * colChanged_;
  /// Input list of columns to process
  int * colsToDo_;
  /// Length of #colsToDo_
  int numberColsToDo_;
  /// Output list of columns to process next
  int * nextColsToDo_;
  /// Length of #nextColsToDo_
  int numberNextColsToDo_;

  /*! \brief Row change status information

    Coded using the following bits:
    <ul>
      <li> 0x01: Row has changed
      <li> 0x02: preprocessing prohibited
      <li> 0x04: Row has been used
    </ul>
  */
  unsigned char * rowChanged_;
  /// Input list of rows to process
  int * rowsToDo_;
  /// Length of #rowsToDo_
  int numberRowsToDo_;
  /// Output list of rows to process next
  int * nextRowsToDo_;
  /// Length of #nextRowsToDo_
  int numberNextRowsToDo_;
  /*! \brief Fine control over presolve actions

    Set/clear the following bits to allow or suppress actions:
      - 0x01 allow duplicate column tests for integer variables
      - 0x02 not used
      - 0x04 set to inhibit x+y+z=1 mods
      - 0x08 not used
      - 0x10 set to allow stuff which won't unroll easily (overlapping
          duplicate rows; opportunistic fixing of variables from bound
	  propagation).
      - 0x04000 allow presolve transforms to arbitrarily ignore infeasibility
          and set arbitrary feasible bounds.
      - 0x10000 instructs implied_free_action to be `more lightweight'; will
          return without doing anything after 15 presolve passes.
      - 0x20000 instructs implied_free_action to remove small created elements
      - 0x80000000 set by presolve to say dupcol_action compressed columns
  */
  int presolveOptions_;
  /*! Flag to say if any rows or columns are marked as prohibited

    Note that this flag is <i>not</i> manipulated by any of the
    various \c set*Prohibited routines.
  */
  bool anyProhibited_;
  //@}

  /*! \name Scratch work arrays

    Preallocated work arrays are useful to avoid having to allocate and free
    work arrays in individual presolve methods.

    All are allocated from #setMatrix by #initializeStuff, freed from
    #~CoinPresolveMatrix.  You can use #deleteStuff followed by
    #initializeStuff to remove and recreate them.
  */
  //@{
  /// Preallocated scratch work array, 3*nrows_
  int *usefulRowInt_ ;
  /// Preallocated scratch work array, 2*nrows_
  double *usefulRowDouble_ ;
  /// Preallocated scratch work array, 2*ncols_
  int *usefulColumnInt_ ;
  /// Preallocated scratch work array, ncols_
  double *usefulColumnDouble_ ;
  /// Array of random numbers (max row,column)
  double *randomNumber_ ;

  /// Work array for count of infinite contributions to row lhs upper bound
  int *infiniteUp_ ;
  /// Work array for sum of finite contributions to row lhs upper bound
  double *sumUp_ ;
  /// Work array for count of infinite contributions to row lhs lower bound
  int *infiniteDown_ ;
  /// Work array for sum of finite contributions to row lhs lower bound
  double *sumDown_ ;
  //@}

  /*! \brief Recompute row lhs bounds

    Calculate finite contributions to row lhs upper and lower bounds
    and count infinite contributions. Returns the number of rows found
    to be infeasible.

    If \p whichRow < 0, bounds are recomputed for all rows.

    As of 110611, this seems to be a work in progress in the sense that it's
    barely used by the existing presolve code.
  */
  int recomputeSums(int whichRow) ;

  /// Allocate scratch arrays
  void initializeStuff() ;
  /// Free scratch arrays
  void deleteStuff() ;

  /*! \name Functions to manipulate row and column processing status */
  //@{

  /*! \brief Initialise the column ToDo lists

    Places all columns in the #colsToDo_ list except for columns marked
    as prohibited (<i>viz.</i> #colChanged_).
  */
  void initColsToDo () ;

  /*! \brief Step column ToDo lists

    Moves columns on the #nextColsToDo_ list to the #colsToDo_ list, emptying
    #nextColsToDo_. Returns the number of columns transferred.
  */
  int stepColsToDo () ;

  /// Return the number of columns on the #colsToDo_ list
  inline int numberColsToDo()
  { return (numberColsToDo_) ; }

  /// Has column been changed?
  inline bool colChanged(int i) const {
    return (colChanged_[i]&1)!=0;
  }
  /// Mark column as not changed
  inline void unsetColChanged(int i) {
    colChanged_[i] = static_cast<unsigned char>(colChanged_[i] & (~1)) ;
  }
  /// Mark column as changed.
  inline void setColChanged(int i) {
    colChanged_[i] = static_cast<unsigned char>(colChanged_[i] | (1)) ;
  }
  /// Mark column as changed and add to list of columns to process next
  inline void addCol(int i) {
    if ((colChanged_[i]&1)==0) {
      colChanged_[i] = static_cast<unsigned char>(colChanged_[i] | (1)) ;
      nextColsToDo_[numberNextColsToDo_++] = i;
    }
  }
  /// Test if column is eligible for preprocessing
  inline bool colProhibited(int i) const {
    return (colChanged_[i]&2)!=0;
  }
  /*! \brief Test if column is eligible for preprocessing

    The difference between this method and #colProhibited() is that this
    method first tests #anyProhibited_ before examining the specific entry
    for the specified column.
  */
  inline bool colProhibited2(int i) const {
    if (!anyProhibited_)
      return false;
    else
      return (colChanged_[i]&2)!=0;
  }
  /// Mark column as ineligible for preprocessing
  inline void setColProhibited(int i) {
    colChanged_[i] = static_cast<unsigned char>(colChanged_[i] | (2)) ;
  }
  /*! \brief Test if column is marked as used
  
    This is for doing faster lookups to see where two columns have entries
    in common.
  */
  inline bool colUsed(int i) const {
    return (colChanged_[i]&4)!=0;
  }
  /// Mark column as used
  inline void setColUsed(int i) {
    colChanged_[i] = static_cast<unsigned char>(colChanged_[i] | (4)) ;
  }
  /// Mark column as unused
  inline void unsetColUsed(int i) {
    colChanged_[i] = static_cast<unsigned char>(colChanged_[i] & (~4)) ;
  }
  /// Has column infinite ub (originally)
  inline bool colInfinite(int i) const {
    return (colChanged_[i]&8)!=0;
  }
  /// Mark column as not infinite ub (originally)
  inline void unsetColInfinite(int i) {
    colChanged_[i] = static_cast<unsigned char>(colChanged_[i] & (~8)) ;
  }
  /// Mark column as infinite ub (originally)
  inline void setColInfinite(int i) {
    colChanged_[i] = static_cast<unsigned char>(colChanged_[i] | (8)) ;
  }

  /*! \brief Initialise the row ToDo lists

    Places all rows in the #rowsToDo_ list except for rows marked
    as prohibited (<i>viz.</i> #rowChanged_).
  */
  void initRowsToDo () ;

  /*! \brief Step row ToDo lists

    Moves rows on the #nextRowsToDo_ list to the #rowsToDo_ list, emptying
    #nextRowsToDo_. Returns the number of rows transferred.
  */
  int stepRowsToDo () ;

  /// Return the number of rows on the #rowsToDo_ list
  inline int numberRowsToDo()
  { return (numberRowsToDo_) ; }

  /// Has row been changed?
  inline bool rowChanged(int i) const {
    return (rowChanged_[i]&1)!=0;
  }
  /// Mark row as not changed
  inline void unsetRowChanged(int i) {
    rowChanged_[i] = static_cast<unsigned char>(rowChanged_[i] & (~1)) ;
  }
  /// Mark row as changed
  inline void setRowChanged(int i) {
    rowChanged_[i] = static_cast<unsigned char>(rowChanged_[i] | (1)) ;
  }
  /// Mark row as changed and add to list of rows to process next
  inline void addRow(int i) {
    if ((rowChanged_[i]&1)==0) {
      rowChanged_[i] = static_cast<unsigned char>(rowChanged_[i] | (1)) ;
      nextRowsToDo_[numberNextRowsToDo_++] = i;
    }
  }
  /// Test if row is eligible for preprocessing
  inline bool rowProhibited(int i) const {
    return (rowChanged_[i]&2)!=0;
  }
  /*! \brief Test if row is eligible for preprocessing

    The difference between this method and #rowProhibited() is that this
    method first tests #anyProhibited_ before examining the specific entry
    for the specified row.
  */
  inline bool rowProhibited2(int i) const {
    if (!anyProhibited_)
      return false;
    else
      return (rowChanged_[i]&2)!=0;
  }
  /// Mark row as ineligible for preprocessing
  inline void setRowProhibited(int i) {
    rowChanged_[i] = static_cast<unsigned char>(rowChanged_[i] | (2)) ;
  }
  /*! \brief Test if row is marked as used

     This is for doing faster lookups to see where two rows have entries
     in common.  It can be used anywhere as long as it ends up zeroed out.
  */
  inline bool rowUsed(int i) const {
    return (rowChanged_[i]&4)!=0;
  }
  /// Mark row as used
  inline void setRowUsed(int i) {
    rowChanged_[i] = static_cast<unsigned char>(rowChanged_[i] | (4)) ;
  }
  /// Mark row as unused
  inline void unsetRowUsed(int i) {
    rowChanged_[i] = static_cast<unsigned char>(rowChanged_[i] & (~4)) ;
  }


  /// Check if there are any prohibited rows or columns 
  inline bool anyProhibited() const
  { return anyProhibited_;}
  /// Set a flag for presence of prohibited rows or columns
  inline void setAnyProhibited(bool val = true)
  { anyProhibited_ = val ; }
  //@}

};

/*! \class CoinPostsolveMatrix
    \brief Augments CoinPrePostsolveMatrix with information about the problem
	   that is only needed during postsolve.

  The notable point is that the matrix representation is threaded. The
  representation is column-major and starts with the standard two pairs of
  arrays: one pair to hold the row indices and coefficients, the second pair
  to hold the column starting positions and lengths. But the row indices and
  coefficients for a column do not necessarily occupy a contiguous block in
  their respective arrays. Instead, a link array gives the position of the
  next (row index,coefficient) pair. If the row index and value of a
  coefficient a<p,j> occupy position kp in their arrays, then the position of
  the next coefficient a<q,j> is found as kq = link[kp].

  This threaded representation allows for efficient expansion of columns as
  rows are reintroduced during postsolve transformations. The basic packed
  structures are allocated to the expected size of the postsolved matrix,
  and as new coefficients are added, their location is simply added to the
  thread for the column.

  There is no provision to convert the threaded representation to a packed
  representation. In the context of postsolve, it's not required. (You did
  keep a copy of the original matrix, eh?)

  The constructors that take an OSI or ClpSimplex as a parameter really should
  not be here, but for historical reasons they will likely remain for the
  forseeable future.  -- lh, 111202 --
*/
class CoinPostsolveMatrix : public CoinPrePostsolveMatrix
{
 public:

  /*! \brief `Native' constructor

    This constructor creates an empty object which must then be loaded.
    On the other hand, it doesn't assume that the client is an
    OsiSolverInterface.
  */
  CoinPostsolveMatrix(int ncols_alloc, int nrows_alloc,
		      CoinBigIndex nelems_alloc) ;


  /*! \brief Clp OSI constructor

    See Clp code for the definition.
  */
  CoinPostsolveMatrix(ClpSimplex * si,

		   int ncols0,
		   int nrows0,
		   CoinBigIndex nelems0,
		     
		   double maxmin_,
		   // end prepost members

		   double *sol,
		   double *acts,

		   unsigned char *colstat,
		   unsigned char *rowstat);

  /*! \brief Generic OSI constructor

    See OSI code for the definition.
  */
  CoinPostsolveMatrix(OsiSolverInterface * si,

		   int ncols0,
		   int nrows0,
		   CoinBigIndex nelems0,
		     
		   double maxmin_,
		   // end prepost members

		   double *sol,
		   double *acts,

		   unsigned char *colstat,
		   unsigned char *rowstat);

  /*! \brief Load an empty CoinPostsolveMatrix from a CoinPresolveMatrix

    This routine transfers the contents of the CoinPrePostsolveMatrix
    object from the CoinPresolveMatrix object to the CoinPostsolveMatrix
    object and completes initialisation of the CoinPostsolveMatrix object.
    The empty shell of the CoinPresolveMatrix object is destroyed.

    The routine expects an empty CoinPostsolveMatrix object. If handed a loaded
    object, a lot of memory will leak.
  */
  void assignPresolveToPostsolve (CoinPresolveMatrix *&preObj) ;

  /// Destructor
  ~CoinPostsolveMatrix();

  /*! \name Column thread structures

    As mentioned in the class documentation, the entries for a given column
    do not necessarily occupy a contiguous block of space. The #link_ array
    is used to maintain the threading. There is one thread for each column,
    and a single thread for all free entries in #hrow_ and #colels_.

    The allocated size of #link_ must be at least as large as the allocated
    size of #hrow_ and #colels_.
  */
  //@{

  /*! \brief First entry in free entries thread */
  CoinBigIndex free_list_;
  /// Allocated size of #link_
  int maxlink_;
  /*! \brief Thread array

    Within a thread, link_[k] points to the next entry in the thread.
  */
  CoinBigIndex *link_;

  //@}

  /*! \name Debugging aids

     These arrays are allocated only when CoinPresolve is compiled with
     PRESOLVE_DEBUG defined. They hold codes which track the reason that
     a column or row is added to the problem during postsolve.
  */
  //@{
  char *cdone_;
  char *rdone_;
  //@}

  /// debug
  void check_nbasic();

};


/*! \defgroup MtxManip Presolve Matrix Manipulation Functions

  Functions to work with the loosely packed and threaded packed matrix
  structures used during presolve and postsolve.
*/
//@{

/*! \relates CoinPrePostsolveMatrix
    \brief Initialise linked list for major vector order in bulk storage
*/

void presolve_make_memlists(/*CoinBigIndex *starts,*/ int *lengths,
			    presolvehlink *link, int n);

/*! \relates CoinPrePostsolveMatrix
    \brief Make sure a major-dimension vector k has room for one more
	   coefficient.

    You can use this directly, or use the inline wrappers presolve_expand_col
    and presolve_expand_row
*/
bool presolve_expand_major(CoinBigIndex *majstrts, double *majels,
			   int *minndxs, int *majlens,
			   presolvehlink *majlinks, int nmaj, int k) ;

/*! \relates CoinPrePostsolveMatrix
    \brief Make sure a column (colx) in a column-major matrix has room for
	   one more coefficient
*/

inline bool presolve_expand_col(CoinBigIndex *mcstrt, double *colels,
				int *hrow, int *hincol,
				presolvehlink *clink, int ncols, int colx)
{ return presolve_expand_major(mcstrt,colels,
			       hrow,hincol,clink,ncols,colx) ; }

/*! \relates CoinPrePostsolveMatrix
    \brief Make sure a row (rowx) in a row-major matrix has room for one
	   more coefficient
*/

inline bool presolve_expand_row(CoinBigIndex *mrstrt, double *rowels,
				int *hcol, int *hinrow,
				presolvehlink *rlink, int nrows, int rowx)
{ return presolve_expand_major(mrstrt,rowels,
			       hcol,hinrow,rlink,nrows,rowx) ; }


/*! \relates CoinPrePostsolveMatrix
    \brief Find position of a minor index in a major vector.

    The routine returns the position \c k in \p minndxs for the specified
    minor index \p tgt. It will abort if the entry does not exist. Can be
    used directly or via the inline wrappers presolve_find_row and
    presolve_find_col.
*/
inline CoinBigIndex presolve_find_minor(int tgt,
					CoinBigIndex ks, CoinBigIndex ke,
				        const int *minndxs)
{ CoinBigIndex k ;
  for (k = ks ; k < ke ; k++)
#ifndef NDEBUG
  { if (minndxs[k] == tgt)
      return (k) ; }
  DIE("FIND_MINOR") ;

  abort () ; return -1;
#else
  { if (minndxs[k] == tgt)
      break ; }
  return (k) ;
#endif
}

/*! \relates CoinPrePostsolveMatrix
    \brief Find position of a row in a column in a column-major matrix.

    The routine returns the position \c k in \p hrow for the specified \p row.
    It will abort if the entry does not exist.
*/
inline CoinBigIndex presolve_find_row(int row, CoinBigIndex kcs,
				      CoinBigIndex kce, const int *hrow)
{ return presolve_find_minor(row,kcs,kce,hrow) ; }

/*! \relates CoinPostsolveMatrix
    \brief Find position of a column in a row in a row-major matrix.

    The routine returns the position \c k in \p hcol for the specified \p col.
    It will abort if the entry does not exist.
*/
inline CoinBigIndex presolve_find_col(int col, CoinBigIndex krs,
				      CoinBigIndex kre, const int *hcol)
{ return presolve_find_minor(col,krs,kre,hcol) ; }


/*! \relates CoinPrePostsolveMatrix
    \brief Find position of a minor index in a major vector.

    The routine returns the position \c k in \p minndxs for the specified
    minor index \p tgt.  A return value of \p ke means the entry does not
    exist.  Can be used directly or via the inline wrappers
    presolve_find_row1 and presolve_find_col1.
*/
CoinBigIndex presolve_find_minor1(int tgt, CoinBigIndex ks, CoinBigIndex ke,
				  const int *minndxs);

/*! \relates CoinPrePostsolveMatrix
    \brief Find position of a row in a column in a column-major matrix.

    The routine returns the position \c k in \p hrow for the specified \p row.
    A return value of \p kce means the entry does not exist.
*/
inline CoinBigIndex presolve_find_row1(int row, CoinBigIndex kcs,
				       CoinBigIndex kce, const int *hrow)
{ return presolve_find_minor1(row,kcs,kce,hrow) ; } 

/*! \relates CoinPrePostsolveMatrix
    \brief Find position of a column in a row in a row-major matrix.

    The routine returns the position \c k in \p hcol for the specified \p col.
    A return value of \p kre means the entry does not exist.
*/
inline CoinBigIndex presolve_find_col1(int col, CoinBigIndex krs,
				       CoinBigIndex kre, const int *hcol)
{ return presolve_find_minor1(col,krs,kre,hcol) ; } 

/*! \relates CoinPostsolveMatrix
    \brief Find position of a minor index in a major vector in a threaded
	   matrix.

    The routine returns the position \c k in \p minndxs for the specified
    minor index \p tgt. It will abort if the entry does not exist. Can be
    used directly or via the inline wrapper presolve_find_row2.
*/
CoinBigIndex presolve_find_minor2(int tgt, CoinBigIndex ks, int majlen,
				  const int *minndxs,
				  const CoinBigIndex *majlinks) ;

/*! \relates CoinPostsolveMatrix
    \brief Find position of a row in a column in a column-major threaded
	   matrix.

    The routine returns the position \c k in \p hrow for the specified \p row.
    It will abort if the entry does not exist.
*/
inline CoinBigIndex presolve_find_row2(int row, CoinBigIndex kcs, int collen,
				       const int *hrow,
				       const CoinBigIndex *clinks)
{ return presolve_find_minor2(row,kcs,collen,hrow,clinks) ; }

/*! \relates CoinPostsolveMatrix
    \brief Find position of a minor index in a major vector in a threaded
	   matrix.

    The routine returns the position \c k in \p minndxs for the specified
    minor index \p tgt. It will return -1 if the entry does not exist.
    Can be used directly or via the inline wrappers presolve_find_row3.
*/
CoinBigIndex presolve_find_minor3(int tgt, CoinBigIndex ks, int majlen,
				  const int *minndxs,
				  const CoinBigIndex *majlinks) ;

/*! \relates CoinPostsolveMatrix
    \brief Find position of a row in a column in a column-major threaded
	   matrix.

    The routine returns the position \c k in \p hrow for the specified \p row.
    It will return -1 if the entry does not exist.
*/
inline CoinBigIndex presolve_find_row3(int row, CoinBigIndex kcs, int collen,
				       const int *hrow,
				       const CoinBigIndex *clinks)
{ return presolve_find_minor3(row,kcs,collen,hrow,clinks) ; }

/*! \relates CoinPrePostsolveMatrix
    \brief Delete the entry for a minor index from a major vector.

   Deletes the entry for \p minndx from the major vector \p majndx.
   Specifically, the relevant entries are removed from the minor index
   (\p minndxs) and coefficient (\p els) arrays and the vector length (\p
   majlens) is decremented.  Loose packing is maintained by swapping the last
   entry in the row into the position occupied by the deleted entry.
*/
inline void presolve_delete_from_major(int majndx, int minndx,
				const CoinBigIndex *majstrts,
				int *majlens, int *minndxs, double *els) 
{
  const CoinBigIndex ks = majstrts[majndx] ;
  const CoinBigIndex ke = ks+majlens[majndx] ;

  const CoinBigIndex kmi = presolve_find_minor(minndx,ks,ke,minndxs) ;

  minndxs[kmi] = minndxs[ke-1] ;
  els[kmi] = els[ke-1] ;
  majlens[majndx]-- ;
  
  return ;
}

/*! \relates CoinPrePostsolveMatrix
    \brief Delete marked entries

    Removes the entries specified in \p marked, compressing the major vector
    to maintain loose packing. \p marked is cleared in the process.
*/
inline void presolve_delete_many_from_major(int majndx, char *marked,
				const CoinBigIndex *majstrts,
				int *majlens, int *minndxs, double *els) 
{ 
  const CoinBigIndex ks = majstrts[majndx] ;
  const CoinBigIndex ke = ks+majlens[majndx] ;
  CoinBigIndex put = ks ;
  for (CoinBigIndex k = ks ; k < ke ; k++) {
    int iMinor = minndxs[k] ;
    if (!marked[iMinor]) {
      minndxs[put] = iMinor ;
      els[put++] = els[k] ;
    } else {
      marked[iMinor] = 0 ;
    }
  } 
  majlens[majndx] = put-ks ;
  return ;
}

/*! \relates CoinPrePostsolveMatrix
    \brief Delete the entry for row \p row from column \p col in a
	   column-major matrix

   Deletes the entry for \p row from the major vector for \p col.
   Specifically, the relevant entries are removed from the row index (\p
   hrow) and coefficient (\p colels) arrays and the vector length (\p
   hincol) is decremented.  Loose packing is maintained by swapping the last
   entry in the row into the position occupied by the deleted entry.
*/
inline void presolve_delete_from_col(int row, int col,
				     const CoinBigIndex *mcstrt,
				     int *hincol, int *hrow, double *colels)
{ presolve_delete_from_major(col,row,mcstrt,hincol,hrow,colels) ; }

/*! \relates CoinPrePostsolveMatrix
    \brief Delete the entry for column \p col from row \p row in a
	   row-major matrix

   Deletes the entry for \p col from the major vector for \p row.
   Specifically, the relevant entries are removed from the column index (\p
   hcol) and coefficient (\p rowels) arrays and the vector length (\p
   hinrow) is decremented.  Loose packing is maintained by swapping the last
   entry in the column into the position occupied by the deleted entry.
*/
inline void presolve_delete_from_row(int row, int col,
				     const CoinBigIndex *mrstrt,
				     int *hinrow, int *hcol, double *rowels)
{ presolve_delete_from_major(row,col,mrstrt,hinrow,hcol,rowels) ; }

/*! \relates CoinPostsolveMatrix
    \brief Delete the entry for a minor index from a major vector in a
    threaded matrix.

   Deletes the entry for \p minndx from the major vector \p majndx.
   Specifically, the relevant entries are removed from the minor index (\p
   minndxs) and coefficient (\p els) arrays and the vector length (\p
   majlens) is decremented. The thread for the major vector is relinked
   around the deleted entry and the space is returned to the free list.
*/
void presolve_delete_from_major2 (int majndx, int minndx,
				  CoinBigIndex *majstrts, int *majlens,
				  int *minndxs, int *majlinks,
				   CoinBigIndex *free_listp) ;

/*! \relates CoinPostsolveMatrix
    \brief Delete the entry for row \p row from column \p col in a
	   column-major threaded matrix

   Deletes the entry for \p row from the major vector for \p col.
   Specifically, the relevant entries are removed from the row index (\p
   hrow) and coefficient (\p colels) arrays and the vector length (\p
   hincol) is decremented. The thread for the major vector is relinked
   around the deleted entry and the space is returned to the free list.
*/
inline void presolve_delete_from_col2(int row, int col, CoinBigIndex *mcstrt,
				      int *hincol, int *hrow,
				      int *clinks, CoinBigIndex *free_listp)
{ presolve_delete_from_major2(col,row,mcstrt,hincol,hrow,clinks,free_listp) ; }

//@}

/*! \defgroup PresolveUtilities Presolve Utility Functions

  Utilities used by multiple presolve transform objects.
*/
//@{

/*! \brief Duplicate a major-dimension vector; optionally omit the entry
	   with minor index \p tgt.

    Designed to copy a major-dimension vector from the paired coefficient
    (\p elems) and minor index (\p indices) arrays used in the standard
    packed matrix representation. Copies \p length entries starting at
    \p offset.
    
    If \p tgt is specified, the entry with minor index == \p tgt is
    omitted from the copy.
*/
double *presolve_dupmajor(const double *elems, const int *indices,
			  int length, CoinBigIndex offset, int tgt = -1);

/// Initialize a vector with random numbers
void coin_init_random_vec(double *work, int n);

//@}


#endif
