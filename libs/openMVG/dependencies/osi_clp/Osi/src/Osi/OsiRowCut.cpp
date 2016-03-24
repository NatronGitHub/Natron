// Copyright (C) 2000, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include <cfloat>
#include <cstdlib>
#include <cstdio>
#include <iostream>

#include "CoinPragma.hpp"
#include "CoinFinite.hpp"
#include "OsiRowCut.hpp"

#ifndef OSI_INLINE_ROWCUT_METHODS

//  //-------------------------------------------------------------------
//  // Set/Get lower & upper bounds
//  //-------------------------------------------------------------------
double OsiRowCut::lb() const { return lb_; }

void OsiRowCut::setLb(double lb) { lb_ = lb; }

double OsiRowCut::ub() const { return ub_; }

void OsiRowCut::setUb(double ub) { ub_ = ub; }

//-------------------------------------------------------------------
// Set row elements
//------------------------------------------------------------------- 
void OsiRowCut::setRow(int size, 
		       const int* colIndices, const double* elements,
		       bool testForDuplicateIndex)
{
   row_.setVector(size, colIndices, elements, testForDuplicateIndex);
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
// Get the row for changing
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
   if ( row() != rhs.row() )            return false;
   if ( lb() != rhs.lb() )              return false;
   if ( ub() != rhs.ub() )              return false;

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
   const CoinPackedVector& r = row();
   r.duplicateIndex("consistent", "OsiRowCut");
   if ( r.getMinIndex() < 0 ) return false;
   return true;
}

bool OsiRowCut::consistent(const OsiSolverInterface& im) const
{  
   const CoinPackedVector& r = row();
   if ( r.getMaxIndex() >= im.getNumCols() ) return false;

   return true;
}

bool OsiRowCut::infeasible(const OsiSolverInterface &) const
{
   if ( lb() > ub() ) return true;

   return false;
}

#endif
/* Returns infeasibility of the cut with respect to solution 
    passed in i.e. is positive if cuts off that solution.  
    solution is getNumCols() long..
*/
double 
OsiRowCut::violated(const double * solution) const
{
   int i;
   double sum = 0.0;
   const int* column = row_.getIndices();
   int number = row_.getNumElements();
   const double* element = row_.getElements();
   for ( i = 0;  i < number;  i++ ) {
      int colIndx = column[i];
      sum += solution[colIndx] * element[i];
   }
   if ( sum > ub_ )
      return sum-ub_;
   else if ( sum < lb_ )
      return lb_-sum;
   else
      return 0.0;
}

//-------------------------------------------------------------------
// Row sense, rhs, range
//-------------------------------------------------------------------
char OsiRowCut::sense() const
{
   if      ( lb_ == ub_ )                        return 'E';
   else if ( lb_ == -COIN_DBL_MAX && ub_ == COIN_DBL_MAX ) return 'N';
   else if ( lb_ == -COIN_DBL_MAX )                   return 'L';
   else if ( ub_ == COIN_DBL_MAX )                    return 'G';
   else                                          return 'R';
}

double OsiRowCut::rhs() const
{
   if      ( lb_ == ub_ )                                  return ub_;
   else if ( lb_ == -COIN_DBL_MAX && ub_ == COIN_DBL_MAX ) return 0.0;
   else if ( lb_ == -COIN_DBL_MAX )                        return ub_;
   else if ( ub_ == COIN_DBL_MAX )                         return lb_;
   else                                                     return ub_;
}

double OsiRowCut::range() const
{
   if      ( lb_ == ub_ )                                  return 0.0;
   else if ( lb_ == -COIN_DBL_MAX && ub_ == COIN_DBL_MAX ) return 0.0;
   else if ( lb_ == -COIN_DBL_MAX )                        return 0.0;
   else if ( ub_ == COIN_DBL_MAX )                         return 0.0;
   else                                                    return ub_ - lb_;
}

//-------------------------------------------------------------------
// Default Constructor 
//-------------------------------------------------------------------
OsiRowCut::OsiRowCut () : OsiCut(),
			  row_(),
			  lb_(-COIN_DBL_MAX),
			  ub_( COIN_DBL_MAX)
{
   //#ifdef NDEBUG
   //row_.setTestForDuplicateIndex(false);
   //#endif
}

//-------------------------------------------------------------------
// Ownership constructor 
//-------------------------------------------------------------------

OsiRowCut::OsiRowCut(double cutlb, double cutub,
 		     int capacity, int size,
 		     int *&colIndices, double *&elements):
   OsiCut(),
   row_(capacity, size, colIndices, elements),
   lb_(cutlb),
   ub_(cutub)
{}

//-------------------------------------------------------------------
// Copy constructor 
//-------------------------------------------------------------------
OsiRowCut::OsiRowCut (const OsiRowCut & source) :
   OsiCut(source),
   row_(source.row_),
   lb_(source.lb_),
   ub_(source.ub_)
{
   // Nothing to do here
}


//----------------------------------------------------------------
// Clone
//----------------------------------------------------------------
OsiRowCut * OsiRowCut::clone() const
{ return (new OsiRowCut(*this)); }


//-------------------------------------------------------------------
// Destructor 
//-------------------------------------------------------------------
OsiRowCut::~OsiRowCut ()
{
   // Nothing to do here
}

//----------------------------------------------------------------
// Assignment operator 
//-------------------------------------------------------------------
OsiRowCut &
OsiRowCut::operator=(const OsiRowCut& rhs)
{
   if ( this != &rhs ) {
      OsiCut::operator=(rhs);
      row_ = rhs.row_;
      lb_ = rhs.lb_;
      ub_ = rhs.ub_;
   }
   return *this;
}

//----------------------------------------------------------------
// Print
//-------------------------------------------------------------------

void
OsiRowCut::print() const
{
   int i;
   std::cout << "Row cut has " << row_.getNumElements()
	     << " elements";
   if ( lb_ < -1.0e20 && ub_<1.0e20 ) 
      std::cout << " with upper rhs of " << ub_;
   else if ( lb_ > -1.0e20 && ub_ > 1.0e20 ) 
      std::cout << " with lower rhs of " << lb_;
   else
      std::cout << " !!! with lower, upper rhs of " << lb_ << " and " << ub_;
   std::cout << std::endl;
   for ( i = 0;  i < row_.getNumElements();  i++ ) {
      int colIndx = row_.getIndices()[i];
      double element = row_.getElements()[i];
      if ( i > 0 && element > 0 )
	 std::cout << " +";
      std::cout << element << " * x" << colIndx << " ";
   }
   std::cout << std::endl;
}

//-------------------------------------------------------------------
// Default Constructor 
//-------------------------------------------------------------------
OsiRowCut2::OsiRowCut2(int row) :
   OsiRowCut(),
   whichRow_(row)
{
  // nothing to do here
}

//-------------------------------------------------------------------
// Copy constructor 
//-------------------------------------------------------------------
OsiRowCut2::OsiRowCut2(const OsiRowCut2 & source) :
   OsiRowCut(source),
   whichRow_(source.whichRow_)
{  
  // Nothing to do here
}


//----------------------------------------------------------------
// Clone
//----------------------------------------------------------------
OsiRowCut * OsiRowCut2::clone() const
{ return (new OsiRowCut2(*this)); }


//-------------------------------------------------------------------
// Destructor 
//-------------------------------------------------------------------
OsiRowCut2::~OsiRowCut2 ()
{
   // Nothing to do here
}

//----------------------------------------------------------------
// Assignment operator 
//-------------------------------------------------------------------
OsiRowCut2 &
OsiRowCut2::operator=(const OsiRowCut2& rhs)
{
   if ( this != &rhs ) {
      OsiRowCut::operator = (rhs);
      whichRow_ = rhs.whichRow_;
   }
   return *this;
}
