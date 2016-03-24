// Copyright (C) 2000, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef OsiCut_H
#define OsiCut_H

#include "OsiCollections.hpp"
#include "OsiSolverInterface.hpp"

/** Base Class for cut.

The Base cut class contains:
  <ul>
  <li>a measure of the cut's effectivness
  </ul>
*/

/*
  COIN_NOTEST_DUPLICATE is rooted in CoinUtils. Check there before you
  meddle here.
*/
#ifdef COIN_FAST_CODE
#ifndef COIN_NOTEST_DUPLICATE
#define COIN_NOTEST_DUPLICATE
#endif
#endif

#ifndef COIN_NOTEST_DUPLICATE
#define COIN_DEFAULT_VALUE_FOR_DUPLICATE true
#else
#define COIN_DEFAULT_VALUE_FOR_DUPLICATE false
#endif


class OsiCut  {
  
public:
    
  //-------------------------------------------------------------------
  /**@name Effectiveness */
  //@{
  /// Set effectiveness
  inline void setEffectiveness( double e ); 
  /// Get effectiveness
  inline double effectiveness() const; 
  //@}

  /**@name GloballyValid */
  //@{
  /// Set globallyValid (nonzero true)
  inline void setGloballyValid( bool trueFalse ) 
  { globallyValid_=trueFalse ? 1 : 0;}
  inline void setGloballyValid( ) 
  { globallyValid_=1;}
  inline void setNotGloballyValid( ) 
  { globallyValid_=0;}
  /// Get globallyValid
  inline bool globallyValid() const
  { return globallyValid_!=0;}
  /// Set globallyValid as integer (nonzero true)
  inline void setGloballyValidAsInteger( int trueFalse ) 
  { globallyValid_=trueFalse;}
  /// Get globallyValid
  inline int globallyValidAsInteger() const
  { return globallyValid_;}
  //@}

  /**@name Debug stuff */
  //@{
    /// Print cuts in collection
  virtual void print() const {}
  //@}
   
#if 0
  / **@name Times used */
  / /@{
  / // Set times used
  inline void setTimesUsed( int t );
  / // Increment times used
  inline void incrementTimesUsed();
  / // Get times used
  inline int timesUsed() const;
  / /@}
  
  / **@name Times tested */
  / /@{
  / // Set times tested
  inline void setTimesTested( int t );
  / // Increment times tested
  inline void incrementTimesTested();
  / // Get times tested
  inline int timesTested() const;
  / /@}
#endif

  //----------------------------------------------------------------

  /**@name Comparison operators  */
  //@{
    ///equal. 2 cuts are equal if there effectiveness are equal
    inline virtual bool operator==(const OsiCut& rhs) const; 
    /// not equal
    inline virtual bool operator!=(const OsiCut& rhs) const; 
    /// less than. True if this.effectiveness < rhs.effectiveness
    inline virtual bool operator< (const OsiCut& rhs) const; 
    /// less than. True if this.effectiveness > rhs.effectiveness
    inline virtual bool operator> (const OsiCut& rhs) const; 
  //@}

  //----------------------------------------------------------------
  // consistent() - returns true if the cut is consistent with repect to itself.
  //         This might include checks to ensure that a packed vector
  //         itself does not have a negative index.
  // consistent(const OsiSolverInterface& si) - returns true if cut is consistent with
  //         respect to the solver interface's model. This might include a check to 
  //         make sure a column index is not greater than the number
  //         of columns in the problem.
  // infeasible(const OsiSolverInterface& si) - returns true if the cut is infeasible 
  //         "with respect to itself". This might include a check to ensure 
  //         the lower bound is greater than the upper bound, or if the
  //         cut simply replaces bounds that the new bounds are feasible with 
  //         respect to the old bounds.
  //-----------------------------------------------------------------
  /**@name Sanity checks on cut */
  //@{
  /** Returns true if the cut is consistent with respect to itself,
      without considering any
      data in the model. For example, it might check to ensure
      that a column index is not negative.
  */
  inline virtual bool consistent() const=0; 

  /** Returns true if cut is consistent when considering the solver
      interface's model.  For example, it might check to ensure
      that a column index is not greater than the number of columns
      in the model. Assumes consistent() is true.
  */
  inline virtual bool consistent(const OsiSolverInterface& si) const=0;

  /** Returns true if the cut is infeasible "with respect to itself" and
      cannot be satisfied. This method does NOT check whether adding the
      cut to the solver interface's model will make the -model- infeasble.
      A cut which returns !infeasible(si) may very well make the model
      infeasible. (Of course, adding a cut with returns infeasible(si) 
      will make the model infeasible.)

      The "with respect to itself" is in quotes becaues 
      in the case where the cut
      simply replaces existing bounds, it may make
      sense to test infeasibility with respect to the current bounds
      held in the solver interface's model. For example, if the cut 
      has a single variable in it, it might check that the maximum
      of new and existing lower bounds is greater than the minium of 
      the new and existing upper bounds.

      Assumes that consistent(si) is true.<br>
      Infeasible cuts can be a useful mechanism for a cut generator to
      inform the solver interface that its detected infeasibility of the
      problem.
  */
  inline virtual bool infeasible(const OsiSolverInterface &si) const=0;

  /** Returns infeasibility of the cut with respect to solution 
      passed in i.e. is positive if cuts off that solution.  
      solution is getNumCols() long..
  */
  virtual double violated(const double * solution) const=0;
  //@}

protected:

  /**@name Constructors and destructors */
  //@{
  /// Default Constructor 
  OsiCut ();
  
  /// Copy constructor 
  OsiCut ( const OsiCut &);
   
  /// Assignment operator 
  OsiCut & operator=( const OsiCut& rhs);

  /// Destructor 
  virtual ~OsiCut ();
  //@}
  
private:
  
  /**@name Private member data */
  //@{
  /// Effectiveness
  double effectiveness_;
  /// If cut has global validity i.e. can be used anywhere in tree
  int globallyValid_;
#if 0
  /// Times used
  int timesUsed_;
  /// Times tested
  int timesTested_;
#endif
  //@}
};


//-------------------------------------------------------------------
// Set/Get member data
//-------------------------------------------------------------------
void OsiCut::setEffectiveness(double e)  { effectiveness_=e; }
double OsiCut::effectiveness() const { return effectiveness_; }

#if 0
void OsiCut::setTimesUsed( int t ) { timesUsed_=t; }
void OsiCut::incrementTimesUsed() { timesUsed_++; }
int OsiCut::timesUsed() const { return timesUsed_; }

void OsiCut::setTimesTested( int t ) { timesTested_=t; }
void OsiCut::incrementTimesTested() { timesTested_++; }
int OsiCut::timesTested() const{ return timesTested_; }
#endif

//----------------------------------------------------------------
// == operator 
//-------------------------------------------------------------------
bool
OsiCut::operator==(const OsiCut& rhs) const
{
  return effectiveness()==rhs.effectiveness();
}
bool
OsiCut::operator!=(const OsiCut& rhs) const
{
  return !( (*this)==rhs );
}
bool
OsiCut::operator< (const OsiCut& rhs) const
{
  return effectiveness()<rhs.effectiveness();
}
bool
OsiCut::operator> (const OsiCut& rhs) const
{
  return effectiveness()>rhs.effectiveness();
}
#endif
