// Copyright (C) 2000, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef OsiColCut_H
#define OsiColCut_H

#include <string>

#include "CoinPackedVector.hpp"

#include "OsiCollections.hpp"
#include "OsiCut.hpp"

/** Column Cut Class

Column Cut Class has:
  <ul>
  <li>a sparse vector of column lower bounds
  <li>a sparse vector of column upper bounds
  </ul>
*/
class OsiColCut : public OsiCut {
   friend void OsiColCutUnitTest(const OsiSolverInterface * baseSiP, 
				 const std::string & mpsDir);
  
public:
  
  //----------------------------------------------------------------
  
  /**@name Setting column bounds */
  //@{
  /// Set column lower bounds 
  inline void setLbs( 
    int nElements, 
    const int * colIndices, 
    const double * lbElements );   
  
  /// Set column lower bounds from a packed vector
  inline void setLbs( const CoinPackedVector & lbs );
  
  /// Set column upper bounds 
  inline void setUbs( 
    int nElements, 
    const int * colIndices, 
    const double * ubElements );
  
  /// Set column upper bounds from a packed vector
  inline void setUbs( const CoinPackedVector & ubs );
  //@}
  
  //----------------------------------------------------------------
  
  /**@name Getting column bounds */
  //@{
  /// Get column lower bounds
  inline const CoinPackedVector & lbs() const;
  /// Get column upper bounds
  inline const CoinPackedVector & ubs() const;
  //@}
  
  /**@name Comparison operators  */
  //@{
#if __GNUC__ != 2 
  using OsiCut::operator== ;
#endif
  /** equal - true if lower bounds, upper bounds, 
  and OsiCut are equal.
  */
  inline virtual bool operator==(const OsiColCut& rhs) const; 

#if __GNUC__ != 2 
  using OsiCut::operator!= ;
#endif
  /// not equal
  inline virtual bool operator!=(const OsiColCut& rhs) const; 
  //@}
  
  
  //----------------------------------------------------------------
  
  /**@name Sanity checks on cut */
  //@{
  /** Returns true if the cut is consistent with respect to itself.
  This checks to ensure that:
  <ul>
  <li>The bound vectors do not have duplicate indices,
  <li>The bound vectors indices are >=0
  </ul>
  */
  inline virtual bool consistent() const; 
  
  /** Returns true if cut is consistent with respect to the solver  
  interface's model. This checks to ensure that
  the lower & upperbound packed vectors:
  <ul>
  <li>do not have an index >= the number of column is the model.
  </ul>
  */
  inline virtual bool consistent(const OsiSolverInterface& im) const;
  
  /** Returns true if the cut is infeasible with respect to its bounds and the 
  column bounds in the solver interface's models.
  This checks whether:
  <ul>
  <li>the maximum of the new and existing lower bounds is strictly 
  greater than the minimum of the new and existing upper bounds.
</ul>
  */
  inline virtual bool infeasible(const OsiSolverInterface &im) const;
  /** Returns infeasibility of the cut with respect to solution 
      passed in i.e. is positive if cuts off that solution.  
      solution is getNumCols() long..
  */
  virtual double violated(const double * solution) const;
  //@}
  
  //----------------------------------------------------------------
  
  /**@name Constructors and destructors */
  //@{
  /// Assignment operator 
  OsiColCut & operator=( const OsiColCut& rhs);
  
  /// Copy constructor 
  OsiColCut ( const OsiColCut &);
  
  /// Default Constructor 
  OsiColCut ();
  
  /// Clone
  virtual OsiColCut * clone() const;
  
  /// Destructor 
  virtual ~OsiColCut ();
  //@}
  
  /**@name Debug stuff */
  //@{
    /// Print cuts in collection
  virtual void print() const;
  //@}
   
private:
  
  /**@name Private member data */
  //@{
  /// Lower bounds
  CoinPackedVector lbs_;
  /// Upper bounds
  CoinPackedVector ubs_;
  //@}
  
};



//-------------------------------------------------------------------
// Set lower & upper bound vectors
//------------------------------------------------------------------- 
void OsiColCut::setLbs( 
                       int size, 
                       const int * colIndices, 
                       const double * lbElements )
{
  lbs_.setVector(size,colIndices,lbElements);
}
//
void OsiColCut::setUbs( 
                       int size, 
                       const int * colIndices, 
                       const double * ubElements )
{
  ubs_.setVector(size,colIndices,ubElements);
}
//
void OsiColCut::setLbs( const CoinPackedVector & lbs )
{
  lbs_ = lbs;
}
//
void OsiColCut::setUbs( const CoinPackedVector & ubs )
{
  ubs_ = ubs;
}

//-------------------------------------------------------------------
// Get Column Lower Bounds and Column Upper Bounds
//-------------------------------------------------------------------
const CoinPackedVector & OsiColCut::lbs() const 
{ 
  return lbs_; 
}
//
const CoinPackedVector & OsiColCut::ubs() const 
{ 
  return ubs_; 
}

//----------------------------------------------------------------
// == operator 
//-------------------------------------------------------------------
bool
OsiColCut::operator==(
                      const OsiColCut& rhs) const
{
  if ( this->OsiCut::operator!=(rhs) ) 
    return false;
  if ( lbs() != rhs.lbs() ) 
    return false;
  if ( ubs() != rhs.ubs() ) 
    return false;
  return true;
}
//
bool
OsiColCut::operator!=(
                      const OsiColCut& rhs) const
{
  return !( (*this)==rhs );
}

//----------------------------------------------------------------
// consistent & infeasible 
//-------------------------------------------------------------------
bool OsiColCut::consistent() const
{
  const CoinPackedVector & lb = lbs();
  const CoinPackedVector & ub = ubs();
  // Test for consistent cut.
  // Are packed vectors consistent?
  lb.duplicateIndex("consistent", "OsiColCut");
  ub.duplicateIndex("consistent", "OsiColCut");
  if ( lb.getMinIndex() < 0 ) return false;
  if ( ub.getMinIndex() < 0 ) return false;
  return true;
}
//
bool OsiColCut::consistent(const OsiSolverInterface& im) const
{  
  const CoinPackedVector & lb = lbs();
  const CoinPackedVector & ub = ubs();
  
  // Test for consistent cut.
  if ( lb.getMaxIndex() >= im.getNumCols() ) return false;
  if ( ub.getMaxIndex() >= im.getNumCols() ) return false;
  
  return true;
}

#if 0
bool OsiColCut::feasible(const OsiSolverInterface &im) const
{
  const double * oldColLb = im.getColLower();
  const double * oldColUb = im.getColUpper();
  const CoinPackedVector & cutLbs = lbs();
  const CoinPackedVector & cutUbs = ubs();
  int i;
  
  for ( i=0; i<cutLbs.size(); i++ ) {
    int colIndx = cutLbs.indices()[i];
    double newLb;
    if ( cutLbs.elements()[i] > oldColLb[colIndx] )
      newLb = cutLbs.elements()[i];
    else
      newLb = oldColLb[colIndx];

    double newUb = oldColUb[colIndx];
    if ( cutUbs.indexExists(colIndx) )
      if ( cutUbs[colIndx] < newUb ) newUb = cutUbs[colIndx];
      if ( newLb > newUb ) 
        return false;
  }
  
  for ( i=0; i<cutUbs.size(); i++ ) {
    int colIndx = cutUbs.indices()[i];
    double newUb = cutUbs.elements()[i] < oldColUb[colIndx] ? cutUbs.elements()[i] : oldColUb[colIndx];
    double newLb = oldColLb[colIndx];
    if ( cutLbs.indexExists(colIndx) )
      if ( cutLbs[colIndx] > newLb ) newLb = cutLbs[colIndx];
      if ( newUb < newLb ) 
        return false;
  }
  
  return true;
}
#endif 


bool OsiColCut::infeasible(const OsiSolverInterface &im) const
{
  const double * oldColLb = im.getColLower();
  const double * oldColUb = im.getColUpper();
  const CoinPackedVector & cutLbs = lbs();
  const CoinPackedVector & cutUbs = ubs();
  int i;
  
  for ( i=0; i<cutLbs.getNumElements(); i++ ) {
    int colIndx = cutLbs.getIndices()[i];
    double newLb= cutLbs.getElements()[i] > oldColLb[colIndx] ?
       cutLbs.getElements()[i] : oldColLb[colIndx];

    double newUb = oldColUb[colIndx];
    if ( cutUbs.isExistingIndex(colIndx) )
      if ( cutUbs[colIndx] < newUb ) newUb = cutUbs[colIndx];
      if ( newLb > newUb ) 
        return true;
  }
  
  for ( i=0; i<cutUbs.getNumElements(); i++ ) {
    int colIndx = cutUbs.getIndices()[i];
    double newUb = cutUbs.getElements()[i] < oldColUb[colIndx] ?
       cutUbs.getElements()[i] : oldColUb[colIndx];
    double newLb = oldColLb[colIndx];
    if ( cutLbs.isExistingIndex(colIndx) )
      if ( cutLbs[colIndx] > newLb ) newLb = cutLbs[colIndx];
      if ( newUb < newLb ) 
        return true;
  }
  
  return false;
}

#endif
