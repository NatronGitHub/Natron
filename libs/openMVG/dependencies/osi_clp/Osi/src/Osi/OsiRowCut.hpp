// Copyright (C) 2000, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef OsiRowCut_H
#define OsiRowCut_H

#include "CoinPackedVector.hpp"

#include "OsiCollections.hpp"
#include "OsiCut.hpp"

//#define OSI_INLINE_ROWCUT_METHODS
#ifdef OSI_INLINE_ROWCUT_METHODS
#define OsiRowCut_inline inline
#else
#define OsiRowCut_inline
#endif

/** Row Cut Class

A row cut has:
  <ul>
  <li>a lower bound<br>
  <li>an upper bound<br>
  <li>a vector of row elements
  </ul>
*/
class OsiRowCut : public OsiCut {
   friend void OsiRowCutUnitTest(const OsiSolverInterface * baseSiP,    
				 const std::string & mpsDir);

public:
  
  /**@name Row bounds */
  //@{
    /// Get lower bound
    OsiRowCut_inline double lb() const;
    /// Set lower bound
    OsiRowCut_inline void setLb(double lb);
    /// Get upper bound
    OsiRowCut_inline double ub() const;
    /// Set upper bound
    OsiRowCut_inline void setUb(double ub);
  //@}

  /**@name Row rhs, sense, range */
  //@{
    /// Get sense ('E', 'G', 'L', 'N', 'R')
    char sense() const;
    /// Get right-hand side
    double rhs() const;
    /// Get range (ub - lb for 'R' rows, 0 otherwise)
    double range() const;
  //@}

  //-------------------------------------------------------------------
  /**@name Row elements  */
  //@{
    /// Set row elements
    OsiRowCut_inline void setRow( 
      int size, 
      const int * colIndices, 
      const double * elements,
      bool testForDuplicateIndex = COIN_DEFAULT_VALUE_FOR_DUPLICATE);
    /// Set row elements from a packed vector
    OsiRowCut_inline void setRow( const CoinPackedVector & v );
    /// Get row elements
    OsiRowCut_inline const CoinPackedVector & row() const;
    /// Get row elements for changing
    OsiRowCut_inline CoinPackedVector & mutableRow() ;
  //@}

  /**@name Comparison operators  */
  //@{
#if __GNUC__ != 2 
    using OsiCut::operator== ;
#endif
    /** equal - true if lower bound, upper bound, row elements,
        and OsiCut are equal.
    */
    OsiRowCut_inline bool operator==(const OsiRowCut& rhs) const; 

#if __GNUC__ != 2 
    using OsiCut::operator!= ;
#endif
    /// not equal
    OsiRowCut_inline bool operator!=(const OsiRowCut& rhs) const; 
  //@}
  
    
  //----------------------------------------------------------------
  /**@name Sanity checks on cut */
  //@{
    /** Returns true if the cut is consistent.
        This checks to ensure that:
        <ul>
        <li>The row element vector does not have duplicate indices
        <li>The row element vector indices are >= 0
        </ul>
    */
    OsiRowCut_inline bool consistent() const; 

    /** Returns true if cut is consistent with respect to the solver
        interface's model.
        This checks to ensure that
        <ul>
        <li>The row element vector indices are < the number of columns
            in the model
        </ul>
    */
    OsiRowCut_inline bool consistent(const OsiSolverInterface& im) const;

    /** Returns true if the row cut itself is infeasible and cannot be satisfied.       
        This checks whether
        <ul>
        <li>the lower bound is strictly greater than the
            upper bound.
        </ul>
    */
    OsiRowCut_inline bool infeasible(const OsiSolverInterface &im) const;
    /** Returns infeasibility of the cut with respect to solution 
	passed in i.e. is positive if cuts off that solution.  
	solution is getNumCols() long..
    */
    virtual double violated(const double * solution) const;
  //@}

  /**@name Arithmetic operators. Apply CoinPackedVector methods to the vector */
  //@{
    /// add <code>value</code> to every vector entry
    void operator+=(double value)
	{ row_ += value; }

    /// subtract <code>value</code> from every vector entry
    void operator-=(double value)
	{ row_ -= value; }

    /// multiply every vector entry by <code>value</code>
    void operator*=(double value)
	{ row_ *= value; }

    /// divide every vector entry by <code>value</code>
    void operator/=(double value)
	{ row_ /= value; }
  //@}

  /// Allow access row sorting function
  void sortIncrIndex()
	{row_.sortIncrIndex();}

  /**@name Constructors and destructors */
  //@{
    /// Assignment operator
    OsiRowCut & operator=( const OsiRowCut& rhs);
  
    /// Copy constructor 
    OsiRowCut ( const OsiRowCut &);  

    /// Clone
    virtual OsiRowCut * clone() const;
  
    /// Default Constructor 
    OsiRowCut ();

    /** \brief Ownership Constructor

      This constructor assumes ownership of the vectors passed as parameters
      for indices and elements. \p colIndices and \p elements will be NULL
      on return.
    */
    OsiRowCut(double cutlb, double cutub,
 		     int capacity, int size,
 		     int *&colIndices, double *&elements);
  
    /// Destructor 
    virtual ~OsiRowCut ();
  //@}

  /**@name Debug stuff */
  //@{
    /// Print cuts in collection
  virtual void print() const ;
  //@}
   
private:
  
 
  /**@name Private member data */
  //@{
    /// Row elements
    CoinPackedVector row_;
    /// Row lower bound
    double lb_;
    /// Row upper bound
    double ub_;
  //@}
};

#ifdef OSI_INLINE_ROWCUT_METHODS

//-------------------------------------------------------------------
// Set/Get lower & upper bounds
//-------------------------------------------------------------------
double OsiRowCut::lb() const { return lb_; }
void OsiRowCut::setLb(double lb) { lb_ = lb; }
double OsiRowCut::ub() const { return ub_; }
void OsiRowCut::setUb(double ub) { ub_ = ub; }

//-------------------------------------------------------------------
// Set row elements
//------------------------------------------------------------------- 
void OsiRowCut::setRow(int size, 
		       const int * colIndices, const double * elements)
{
  row_.setVector(size,colIndices,elements);
}
void OsiRowCut::setRow( const CoinPackedVector & v )
{
  row_ = v;
}

//-------------------------------------------------------------------
// Get the row
//-------------------------------------------------------------------
const CoinPackedVector & OsiRowCut::row() const 
{ 
  return row_; 
}

//-------------------------------------------------------------------
// Get the row so we can change
//-------------------------------------------------------------------
CoinPackedVector & OsiRowCut::mutableRow() 
{ 
  return row_; 
}

//----------------------------------------------------------------
// == operator 
//-------------------------------------------------------------------
bool
OsiRowCut::operator==(const OsiRowCut& rhs) const
{
  if ( this->OsiCut::operator!=(rhs) ) return false;
  if ( row() != rhs.row() ) return false;
  if ( lb() != rhs.lb() ) return false;
  if ( ub() != rhs.ub() ) return false;
  return true;
}
bool
OsiRowCut::operator!=(const OsiRowCut& rhs) const
{
  return !( (*this)==rhs );
}


//----------------------------------------------------------------
// consistent & infeasible 
//-------------------------------------------------------------------
bool OsiRowCut::consistent() const
{
  const CoinPackedVector & r=row();
  r.duplicateIndex("consistent", "OsiRowCut");
  if ( r.getMinIndex() < 0 ) return false;
  return true;
}
bool OsiRowCut::consistent(const OsiSolverInterface& im) const
{  
  const CoinPackedVector & r=row();
  if ( r.getMaxIndex() >= im.getNumCols() ) return false;

  return true;
}
bool OsiRowCut::infeasible(const OsiSolverInterface &im) const
{
  if ( lb() > ub() ) return true;

  return false;
}

#endif

/** Row Cut Class which refers back to row which created it.
    It may be useful to strengthen a row rather than add a cut.  To do this
    we need to know which row is strengthened.  This trivial extension
    to OsiRowCut does that.

*/
class OsiRowCut2 : public OsiRowCut {

public:
  
  /**@name Which row */
  //@{
  /// Get row
  inline int whichRow() const
  { return whichRow_;}
  /// Set row
  inline void setWhichRow(int row)
  { whichRow_=row;}
  //@}
  
  /**@name Constructors and destructors */
  //@{
  /// Assignment operator
  OsiRowCut2 & operator=( const OsiRowCut2& rhs);
  
  /// Copy constructor 
  OsiRowCut2 ( const OsiRowCut2 &);  
  
  /// Clone
  virtual OsiRowCut * clone() const;
  
  /// Default Constructor 
  OsiRowCut2 (int row=-1);
  
  /// Destructor 
  virtual ~OsiRowCut2 ();
  //@}

private:
  
 
  /**@name Private member data */
  //@{
  /// Which row
  int whichRow_;
  //@}
};
#endif
