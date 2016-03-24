// Copyright (C) 2000, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef OsiCuts_H
#define OsiCuts_H

#include "CoinPragma.hpp"

#include <cmath>
#include <cfloat>
#include "OsiCollections.hpp"
#include "OsiRowCut.hpp"
#include "OsiColCut.hpp"
#include "CoinFloatEqual.hpp"

/** Collections of row cuts and column cuts
*/
class OsiCuts {
   friend void OsiCutsUnitTest();

public:
  /**@name Iterator classes
   */
  //@{
    /** Iterator

      This is a class for iterating over the collection of cuts.
    */
    class iterator {
      friend class OsiCuts;
    public:
      iterator(OsiCuts& cuts); 
      iterator(const iterator & src);
      iterator & operator=( const iterator& rhs);    
      ~iterator ();
      OsiCut* operator*() const { return cutP_; }    
      iterator operator++();

      iterator operator++(int)
      {
        iterator temp = *this;
        ++*this;
        return temp;
      }
    
      bool operator==(const iterator& it) const {
        return (colCutIndex_+rowCutIndex_)==(it.colCutIndex_+it.rowCutIndex_);
      }
    
      bool operator!=(const iterator& it) const {
        return !((*this)==it);
      }
    
      bool operator<(const iterator& it) const {
        return (colCutIndex_+rowCutIndex_)<(it.colCutIndex_+it.rowCutIndex_);
      }
    
    private: 
      iterator();
      // *THINK* : how to inline these without sticking the code here (ugly...)
      iterator begin();
      iterator end();
      OsiCuts& cuts_;
      int rowCutIndex_;
      int colCutIndex_;
      OsiCut * cutP_;
    };
   
    /** Const Iterator

      This is a class for iterating over the collection of cuts.
    */ 
    class const_iterator {
      friend class OsiCuts;
	public:
	  typedef std::forward_iterator_tag iterator_category;
	  typedef OsiCut* value_type;
	  typedef size_t difference_type;
	  typedef OsiCut ** pointer;
	  typedef OsiCut *& reference;

    public:
      const_iterator(const OsiCuts& cuts);
      const_iterator(const const_iterator & src);
      const_iterator & operator=( const const_iterator& rhs);     
      ~const_iterator ();
      const OsiCut* operator*() const { return cutP_; } 
       
      const_iterator operator++();
    
      const_iterator operator++(int)
      {
        const_iterator temp = *this;
        ++*this;
        return temp;
      }
    
      bool operator==(const const_iterator& it) const {
        return (colCutIndex_+rowCutIndex_)==(it.colCutIndex_+it.rowCutIndex_);
      }
    
      bool operator!=(const const_iterator& it) const {
        return !((*this)==it);
      }
    
      bool operator<(const const_iterator& it) const {
        return (colCutIndex_+rowCutIndex_)<(it.colCutIndex_+it.rowCutIndex_);
      }
    private:  
      inline const_iterator();  
      // *THINK* : how to inline these without sticking the code here (ugly...)
      const_iterator begin();
      const_iterator end();
      const OsiCuts * cutsPtr_;
      int rowCutIndex_;
      int colCutIndex_;
      const OsiCut * cutP_;
    };
  //@}

  //-------------------------------------------------------------------
  // 
  // Cuts class definition begins here:
  //
  //------------------------------------------------------------------- 
    
  /** \name Inserting a cut into collection */
  //@{
    /** \brief Insert a row cut */
    inline void insert( const OsiRowCut & rc );
    /** \brief Insert a row cut unless it is a duplicate - cut may get sorted.
       Duplicate is defined as CoinAbsFltEq says same*/
    void insertIfNotDuplicate( OsiRowCut & rc , CoinAbsFltEq treatAsSame=CoinAbsFltEq(1.0e-12) );
    /** \brief Insert a row cut unless it is a duplicate - cut may get sorted.
       Duplicate is defined as CoinRelFltEq says same*/
    void insertIfNotDuplicate( OsiRowCut & rc , CoinRelFltEq treatAsSame );
    /** \brief Insert a column cut */
    inline void insert( const OsiColCut & cc );

    /** \brief Insert a row cut.
    
      The OsiCuts object takes control of the cut object.
      On return, \c rcPtr is NULL.
    */
    inline void insert( OsiRowCut * & rcPtr );
    /** \brief Insert a column cut.
    
      The OsiCuts object takes control of the cut object.
      On return \c ccPtr is NULL.
    */
    inline void insert( OsiColCut * & ccPtr );
#if 0
    inline void insert( OsiCut    * & cPtr  );
#endif
    
   /** \brief Insert a set of cuts */
   inline void insert(const OsiCuts & cs);

  //@}

  /**@name Number of cuts in collection */
  //@{
    /// Number of row cuts in collection
    inline int sizeRowCuts() const;
    /// Number of column cuts in collection
    inline int sizeColCuts() const;
    /// Number of cuts in collection
    inline int sizeCuts() const;
  //@}
   
  /**@name Debug stuff */
  //@{
    /// Print cuts in collection
    inline void printCuts() const;
  //@}
   
  /**@name Get a cut from collection */
  //@{
    /// Get pointer to i'th row cut
    inline OsiRowCut * rowCutPtr(int i);
    /// Get const pointer to i'th row cut
    inline const OsiRowCut * rowCutPtr(int i) const;
    /// Get pointer to i'th column cut
    inline OsiColCut * colCutPtr(int i);
    /// Get const pointer to i'th column cut
    inline const OsiColCut * colCutPtr(int i) const;

    /// Get reference to i'th row cut
    inline OsiRowCut & rowCut(int i);
    /// Get const reference to i'th row cut
    inline const OsiRowCut & rowCut(int i) const;
    /// Get reference to i'th column cut
    inline OsiColCut & colCut(int i);
    /// Get const reference to i'th column cut
    inline const OsiColCut & colCut(int i) const;

    /// Get const pointer to the most effective cut
    inline const OsiCut * mostEffectiveCutPtr() const;
    /// Get pointer to the most effective cut
    inline OsiCut * mostEffectiveCutPtr();
  //@}

  /**@name Deleting cut from collection */
  //@{ 
    /// Remove i'th row cut from collection
    inline void eraseRowCut(int i);
    /// Remove i'th column cut from collection
    inline void eraseColCut(int i); 
    /// Get pointer to i'th row cut and remove ptr from collection
    inline OsiRowCut * rowCutPtrAndZap(int i);
    /*! \brief Clear all row cuts without deleting them
    
      Handy in case one wants to use CGL without managing cuts in one of
      the OSI containers. Client is ultimately responsible for deleting the
      data structures holding the row cuts.
    */
    inline void dumpCuts() ;
    /*! \brief Selective delete and clear for row cuts.

      Deletes the cuts specified in \p to_erase then clears remaining cuts
      without deleting them. A hybrid of eraseRowCut(int) and dumpCuts().
      Client is ultimately responsible for deleting the data structures
      for row cuts not specified in \p to_erase.
    */
    inline void eraseAndDumpCuts(const std::vector<int> to_erase) ;
  //@}
 
  /**@name Sorting collection */
  //@{ 
    /// Cuts with greatest effectiveness are first.
    inline void sort();
  //@}
  
   
  /**@name Iterators 
     Example of using an iterator to sum effectiveness
     of all cuts in the collection.
     <pre>
     double sumEff=0.0;
     for ( OsiCuts::iterator it=cuts.begin(); it!=cuts.end(); ++it )
           sumEff+= (*it)->effectiveness();
     </pre>
  */
  //@{ 
    /// Get iterator to beginning of collection
    inline iterator begin() { iterator it(*this); it.begin(); return it; }  
    /// Get const iterator to beginning of collection
    inline const_iterator begin() const { const_iterator it(*this); it.begin(); return it; }  
    /// Get iterator to end of collection 
    inline iterator end() { iterator it(*this); it.end(); return it; }
    /// Get const iterator to end of collection 
    inline const_iterator end() const { const_iterator it(*this); it.end(); return it; }  
  //@}
 
   
  /**@name Constructors and destructors */
  //@{
    /// Default constructor
    OsiCuts (); 

    /// Copy constructor 
    OsiCuts ( const OsiCuts &);
  
    /// Assignment operator 
    OsiCuts & operator=( const OsiCuts& rhs);
  
    /// Destructor 
    virtual ~OsiCuts ();
  //@}

private:
  //*@name Function operator for sorting cuts by efectiveness */
  //@{
    class OsiCutCompare
    { 
    public:
      /// Function for sorting cuts by effectiveness
      inline bool operator()(const OsiCut * c1P,const OsiCut * c2P) 
      { return c1P->effectiveness() > c2P->effectiveness(); }
    };
  //@}

  /**@name Private methods */
  //@{     
    /// Copy internal data
    void gutsOfCopy( const OsiCuts & source );
    /// Delete internal data
    void gutsOfDestructor();
  //@}
    
  /**@name Private member data */
  //@{   
    /// Vector of row cuts pointers
    OsiVectorRowCutPtr rowCutPtrs_;
    /// Vector of column cuts pointers
    OsiVectorColCutPtr colCutPtrs_;
  //@}

};


//-------------------------------------------------------------------
// insert cuts into collection
//-------------------------------------------------------------------
void OsiCuts::insert( const OsiRowCut & rc )
{
  OsiRowCut * newCutPtr = rc.clone();
  //assert(dynamic_cast<OsiRowCut*>(newCutPtr) != NULL );
  rowCutPtrs_.push_back(static_cast<OsiRowCut*>(newCutPtr));
}
void OsiCuts::insert( const OsiColCut & cc )
{
  OsiColCut * newCutPtr = cc.clone();
  //assert(dynamic_cast<OsiColCut*>(newCutPtr) != NULL );
  colCutPtrs_.push_back(static_cast<OsiColCut*>(newCutPtr));
}

void OsiCuts::insert( OsiRowCut* & rcPtr )
{
  rowCutPtrs_.push_back(rcPtr);
  rcPtr = NULL;
}
void OsiCuts::insert( OsiColCut* &ccPtr )
{
  colCutPtrs_.push_back(ccPtr);
  ccPtr = NULL;
}
#if 0
void OsiCuts::insert( OsiCut* & cPtr )
{
  OsiRowCut * rcPtr = dynamic_cast<OsiRowCut*>(cPtr);
  if ( rcPtr != NULL ) {
    insert( rcPtr );
    cPtr = rcPtr;
  }
  else {
    OsiColCut * ccPtr = dynamic_cast<OsiColCut*>(cPtr);
    assert( ccPtr != NULL );
    insert( ccPtr );
    cPtr = ccPtr;
  }
}
#endif
    
// LANNEZ SEBASTIEN added Thu May 25 01:22:51 EDT 2006
void OsiCuts::insert(const OsiCuts & cs)
{
  for (OsiCuts::const_iterator it = cs.begin (); it != cs.end (); it++)
  {
    const OsiRowCut * rCut = dynamic_cast <const OsiRowCut * >(*it);
    const OsiColCut * cCut = dynamic_cast <const OsiColCut * >(*it);
    assert (rCut || cCut);
    if (rCut)
      insert (*rCut);
    else
      insert (*cCut);
  }
}

//-------------------------------------------------------------------
// sort
//------------------------------------------------------------------- 
void OsiCuts::sort() 
{
  std::sort(colCutPtrs_.begin(),colCutPtrs_.end(),OsiCutCompare()); 
  std::sort(rowCutPtrs_.begin(),rowCutPtrs_.end(),OsiCutCompare()); 
}


//-------------------------------------------------------------------
// Get number of in collections
//------------------------------------------------------------------- 
int OsiCuts::sizeRowCuts() const {
  return static_cast<int>(rowCutPtrs_.size()); }
int OsiCuts::sizeColCuts() const {
  return static_cast<int>(colCutPtrs_.size()); }
int OsiCuts::sizeCuts()    const {
  return static_cast<int>(sizeRowCuts()+sizeColCuts()); }

//----------------------------------------------------------------
// Get i'th cut from the collection
//----------------------------------------------------------------
const OsiRowCut * OsiCuts::rowCutPtr(int i) const { return rowCutPtrs_[i]; }
const OsiColCut * OsiCuts::colCutPtr(int i) const { return colCutPtrs_[i]; }
OsiRowCut * OsiCuts::rowCutPtr(int i) { return rowCutPtrs_[i]; }
OsiColCut * OsiCuts::colCutPtr(int i) { return colCutPtrs_[i]; }

const OsiRowCut & OsiCuts::rowCut(int i) const { return *rowCutPtr(i); }
const OsiColCut & OsiCuts::colCut(int i) const { return *colCutPtr(i); }
OsiRowCut & OsiCuts::rowCut(int i) { return *rowCutPtr(i); }
OsiColCut & OsiCuts::colCut(int i) { return *colCutPtr(i); }

//----------------------------------------------------------------
// Get most effective cut from collection
//----------------------------------------------------------------
const OsiCut * OsiCuts::mostEffectiveCutPtr() const 
{ 
  const_iterator b=begin();
  const_iterator e=end();
  return *(std::min_element(b,e,OsiCutCompare()));
}
OsiCut * OsiCuts::mostEffectiveCutPtr()  
{ 
  iterator b=begin();
  iterator e=end();
  //return *(std::min_element(b,e,OsiCutCompare()));
  OsiCut * retVal = NULL;
  double maxEff = COIN_DBL_MIN;
  for ( OsiCuts::iterator it=b; it!=e; ++it ) {
    if (maxEff < (*it)->effectiveness() ) {
      maxEff = (*it)->effectiveness();
      retVal = *it;
    }
  }
  return retVal;
}

//----------------------------------------------------------------
// Print all cuts
//----------------------------------------------------------------
void
OsiCuts::printCuts() const  
{ 
  // do all column cuts first
  int i;
  int numberColCuts=sizeColCuts();
  for (i=0;i<numberColCuts;i++) {
    const OsiColCut * cut = colCutPtr(i);
    cut->print();
  }
  int numberRowCuts=sizeRowCuts();
  for (i=0;i<numberRowCuts;i++) {
    const OsiRowCut * cut = rowCutPtr(i);
    cut->print();
  }
}

//----------------------------------------------------------------
// Erase i'th cut from the collection
//----------------------------------------------------------------
void OsiCuts::eraseRowCut(int i) 
{
  delete rowCutPtrs_[i];
  rowCutPtrs_.erase( rowCutPtrs_.begin()+i ); 
}
void OsiCuts::eraseColCut(int i) 
{   
  delete colCutPtrs_[i];
  colCutPtrs_.erase( colCutPtrs_.begin()+i );
}
/// Get pointer to i'th row cut and remove ptr from collection
OsiRowCut * 
OsiCuts::rowCutPtrAndZap(int i)
{
  OsiRowCut * cut = rowCutPtrs_[i];
  rowCutPtrs_[i]=NULL;
  rowCutPtrs_.erase( rowCutPtrs_.begin()+i ); 
  return cut;
}
void OsiCuts::dumpCuts()
{
  rowCutPtrs_.clear() ;
}
void OsiCuts::eraseAndDumpCuts(const std::vector<int> to_erase)
{
  for (unsigned i=0; i<to_erase.size(); i++) {
    delete rowCutPtrs_[to_erase[i]];
  }
  rowCutPtrs_.clear();
}


#endif
