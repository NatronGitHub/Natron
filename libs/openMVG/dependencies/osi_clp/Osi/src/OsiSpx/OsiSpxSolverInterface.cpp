//-----------------------------------------------------------------------------
// name:     OSI Interface for SoPlex >= 1.4.2c
// authors:  Tobias Pfender
//           Ambros Gleixner
//           Wei Huang
//           Konrad-Zuse-Zentrum Berlin (Germany)
// date:     01/16/2002
// license:  this file may be freely distributed under the terms of EPL
//-----------------------------------------------------------------------------
// Copyright (C) 2002, Tobias Pfender, International Business Machines
// Corporation and others.  All Rights Reserved.
// Last edit: $Id$

#include "CoinPragma.hpp"

#include <iostream>
#include <fstream>
#include <cassert>
#include <string>
#include <numeric>

#include "CoinError.hpp"

#include "OsiRowCut.hpp"
#include "OsiColCut.hpp"
#include "CoinPackedMatrix.hpp"
#include "CoinWarmStartBasis.hpp"

#ifndef SOPLEX_LEGACY
#define SOPLEX_LEGACY
#endif

#include "soplex.h"

// it's important to include this header after soplex.h
#include "OsiSpxSolverInterface.hpp"

//#############################################################################
// A couple of helper functions
//#############################################################################

inline void freeCacheDouble( double*& ptr )
{
  if( ptr != NULL )
    {
      delete [] ptr;
      ptr = NULL;
    }
}

inline void freeCacheChar( char*& ptr )
{
  if( ptr != NULL )
    {
      delete [] ptr;
      ptr = NULL;
    }
}

inline void freeCacheMatrix( CoinPackedMatrix*& ptr )
{
  if( ptr != NULL )
    {
      delete ptr;
      ptr = NULL;
    }
}

inline void throwSPXerror( std::string error, std::string osimethod )
{
  std::cout << "ERROR: " << error << " (" << osimethod << 
    " in OsiSpxSolverInterface)" << std::endl;
  throw CoinError( error.c_str(), osimethod.c_str(), "OsiSpxSolverInterface" );
}

//#############################################################################
// Solve methods
//#############################################################################

void OsiSpxSolverInterface::initialSolve()
{
  bool takeHint;
  OsiHintStrength strength;

  // by default we use dual simplex
  // unless we get the hint to use primal simplex
  bool dual = true;
  getHintParam(OsiDoDualInInitial,takeHint,strength);
  if (strength!=OsiHintIgnore)
     dual = takeHint;

  // always use column representation
  if( soplex_->rep() != soplex::SPxSolver::COLUMN )
    soplex_->setRep( soplex::SPxSolver::COLUMN );

  // set algorithm type
  if( dual )
  {
    if( soplex_->type() != soplex::SPxSolver::LEAVE )
      soplex_->setType( soplex::SPxSolver::LEAVE );
  }
  else
  {
    if( soplex_->type() != soplex::SPxSolver::ENTER )
      soplex_->setType( soplex::SPxSolver::ENTER );
  }

  // set dual objective limit
  double dualobjlimit;
  OsiSolverInterface::getDblParam(OsiDualObjectiveLimit, dualobjlimit);
  if( fabs(dualobjlimit) < getInfinity() )
  {
  	double objoffset;
    OsiSolverInterface::getDblParam(OsiDualObjectiveLimit, objoffset);
  	dualobjlimit += objoffset;
  }
  soplex_->setTerminationValue( dualobjlimit );

  // solve
  try 
  {
    soplex_->solve();
  }
  catch (soplex::SPxException e)
  {
  	*messageHandler() << "SoPlex initial solve failed with exception " << e.what() << CoinMessageEol;
#if (SOPLEX_VERSION >= 160) || (SOPLEX_SUBVERSION >= 7)
  	try
  	{
  	   *messageHandler() << "Retry with cleared basis" << CoinMessageEol;
  	   soplex_->clearBasis();
  	   soplex_->solve();
  	}
  	catch (soplex::SPxException e)
  	{
  	   *messageHandler() << "SoPlex initial solve with cleared basis failed with exception " << e.what() << CoinMessageEol;
  	}
#endif
  }

  freeCachedResults();
}
//-----------------------------------------------------------------------------
void OsiSpxSolverInterface::resolve()
{
  bool takeHint;
  OsiHintStrength strength;
  
  // by default we use dual simplex
  // unless we get the hint to use primal simplex
  bool dual = true;
  getHintParam(OsiDoDualInResolve,takeHint,strength);
  if (strength!=OsiHintIgnore)
     dual = takeHint;

  // always use column representation
  if( soplex_->rep() != soplex::SPxSolver::COLUMN )
    soplex_->setRep( soplex::SPxSolver::COLUMN );

  // set algorithm type
  if( dual )
  {
    if( soplex_->type() != soplex::SPxSolver::LEAVE )
      soplex_->setType( soplex::SPxSolver::LEAVE );
  }
  else
  {
    if( soplex_->type() != soplex::SPxSolver::ENTER )
      soplex_->setType( soplex::SPxSolver::ENTER );
  }

  // set dual objective limit
  double dualobjlimit;
  OsiSolverInterface::getDblParam(OsiDualObjectiveLimit, dualobjlimit);
  if( fabs(dualobjlimit) < getInfinity() )
  {
  	double objoffset;
    OsiSolverInterface::getDblParam(OsiDualObjectiveLimit, objoffset);
  	dualobjlimit += objoffset;
  }
  soplex_->setTerminationValue( dualobjlimit );

  // solve
  try 
  {
    soplex_->solve();
  }
  catch (soplex::SPxException e)
  {
   *messageHandler() << "SoPlex resolve failed with exception " << e.what() << CoinMessageEol;
#if (SOPLEX_VERSION >= 160) || (SOPLEX_SUBVERSION >= 7)
   try
   {
      *messageHandler() << "Retry with cleared basis" << CoinMessageEol;
      soplex_->clearBasis();
      soplex_->solve();
   }
   catch (soplex::SPxException e)
   {
      *messageHandler() << "SoPlex resolve with cleared basis failed with exception " << e.what() << CoinMessageEol;
   }
#endif
  }

  freeCachedResults();
}
//-----------------------------------------------------------------------------
void OsiSpxSolverInterface::branchAndBound()
{
  throwSPXerror( "SoPlex does not provide an internal branch and bound procedure",
		 "branchAndBound" );
}

//#############################################################################
// Parameter related methods
//#############################################################################

bool
OsiSpxSolverInterface::setIntParam(OsiIntParam key, int value)
{
  bool retval = false;
  try
  {
  switch (key)
    {
    case OsiMaxNumIteration:
      soplex_->setTerminationIter( value );
      retval = true;
      break;
    case OsiMaxNumIterationHotStart:
      if( value >= 0 )
	{
	  hotStartMaxIteration_ = value;
	  retval = true;
	}
      else
	retval = false;
      break;
    case OsiLastIntParam:
      retval = false;
      break;
    case OsiNameDiscipline:
      retval = OsiSolverInterface::setIntParam(key,value);
      break;
    }
  }
  catch (soplex::SPxException e)
  {
     *messageHandler() << "OsiSpx::setDblParam failed with exception " << e.what() << CoinMessageEol;
     retval = false;
  }

  return retval;
}

//-----------------------------------------------------------------------------

bool
OsiSpxSolverInterface::setDblParam(OsiDblParam key, double value)
{
  bool retval = false;
  try
  {
  switch (key)
    {
    case OsiDualObjectiveLimit:
      retval = OsiSolverInterface::setDblParam(key,value);
      break;
    case OsiPrimalObjectiveLimit:
      retval = OsiSolverInterface::setDblParam(key,value);
      break;
    case OsiDualTolerance:
      // SoPlex doesn't support different deltas for primal and dual
      soplex_->setDelta( value );
      retval = true;
      break;
    case OsiPrimalTolerance:
      // SoPlex doesn't support different deltas for primal and dual
      soplex_->setDelta( value );
      retval = true;
      break;
    case OsiObjOffset:
      retval = OsiSolverInterface::setDblParam(key,value);
      break;
    case OsiLastDblParam:
      retval = false;
      break;
    }
  }
  catch (soplex::SPxException e)
  {
     *messageHandler() << "OsiSpx::setDblParam failed with exception " << e.what() << CoinMessageEol;
     retval = false;
  }

  return retval;
}

void OsiSpxSolverInterface::setTimeLimit(double value) {
	soplex_->setTerminationTime(value);
}

//-----------------------------------------------------------------------------

bool
OsiSpxSolverInterface::getIntParam(OsiIntParam key, int& value) const
{
  bool retval = false;
  switch (key)
    {
    case OsiMaxNumIteration:
      value = soplex_->terminationIter();
      retval = true;
      break;
    case OsiMaxNumIterationHotStart:
      value = hotStartMaxIteration_;
      retval = true;
      break;
    case OsiLastIntParam:
      retval = false;
      break;
    case OsiNameDiscipline:
      retval = OsiSolverInterface::getIntParam(key,value);
      break;
    }
  return retval;
}

//-----------------------------------------------------------------------------

bool
OsiSpxSolverInterface::getDblParam(OsiDblParam key, double& value) const
{
  bool retval = false;
  switch (key) 
    {
    case OsiDualObjectiveLimit:
      retval = OsiSolverInterface::getDblParam(key, value);
      break;
    case OsiPrimalObjectiveLimit:
      retval = OsiSolverInterface::getDblParam(key, value);
      break;
    case OsiDualTolerance:
      value = soplex_->delta();
      retval = true;
      break;
    case OsiPrimalTolerance:
      value = soplex_->delta();
      retval = true;
      break;
    case OsiObjOffset:
      retval = OsiSolverInterface::getDblParam(key, value);
      break;
    case OsiLastDblParam:
      retval = false;
      break;
    }
  return retval;
}

bool
OsiSpxSolverInterface::getStrParam(OsiStrParam key, std::string & value) const
{
  switch (key) {
  case OsiSolverName:
    value = "SoPlex";
    return true;
  default: ;
  }
  return false;
}

double OsiSpxSolverInterface::getTimeLimit() const
{
	return soplex_->terminationTime();
}

//#############################################################################
// Methods returning info on how the solution process terminated
//#############################################################################

bool OsiSpxSolverInterface::isAbandoned() const
{
  int stat = soplex_->status();

  return ( stat == soplex::SPxSolver::SINGULAR ||
	   stat == soplex::SPxSolver::ERROR    );
}

bool OsiSpxSolverInterface::isProvenOptimal() const
{
  int stat = soplex_->status();
  return ( stat == soplex::SPxSolver::OPTIMAL );
}

bool OsiSpxSolverInterface::isProvenPrimalInfeasible() const
{
  int stat = soplex_->status();

  return ( stat == soplex::SPxSolver::INFEASIBLE );
}

bool OsiSpxSolverInterface::isProvenDualInfeasible() const
{
  int stat = soplex_->status();

  return ( stat == soplex::SPxSolver::UNBOUNDED );
}

bool OsiSpxSolverInterface::isDualObjectiveLimitReached() const
{
	if( soplex_->status() == soplex::SPxSolver::ABORT_VALUE )
		return true;

	return OsiSolverInterface::isDualObjectiveLimitReached();
}

bool OsiSpxSolverInterface::isIterationLimitReached() const
{
  return ( soplex_->status() == soplex::SPxSolver::ABORT_ITER );
}

bool OsiSpxSolverInterface::isTimeLimitReached() const
{
  return ( soplex_->status() == soplex::SPxSolver::ABORT_TIME );
}

//#############################################################################
// WarmStart related methods
//#############################################################################

CoinWarmStart* OsiSpxSolverInterface::getWarmStart() const
{
  CoinWarmStartBasis* ws = NULL;
  int numcols = getNumCols();
  int numrows = getNumRows();
  int i;

  ws = new CoinWarmStartBasis();
  ws->setSize( numcols, numrows );

  if( soplex_->status() <= soplex::SPxSolver::NO_PROBLEM )
    return ws;

  // The OSI standard assumes the artificial slack variables to have positive coefficients.  SoPlex uses the convention
  // Ax - s = 0, lhs <= s <= rhs, so we have to invert the ON_LOWER and ON_UPPER statuses.
  for( i = 0; i < numrows; ++i )
  {
    switch( soplex_->getBasisRowStatus( i ) )
    {
    case soplex::SPxSolver::BASIC:
      ws->setArtifStatus( i, CoinWarmStartBasis::basic );
      break;	  
    case soplex::SPxSolver::FIXED:
    case soplex::SPxSolver::ON_LOWER:
      ws->setArtifStatus( i, CoinWarmStartBasis::atUpperBound );
      break;
    case soplex::SPxSolver::ON_UPPER:
      ws->setArtifStatus( i, CoinWarmStartBasis::atLowerBound );
      break;
    case soplex::SPxSolver::ZERO:
      ws->setArtifStatus( i, CoinWarmStartBasis::isFree );
      break;
    default:
      throwSPXerror( "invalid row status", "getWarmStart" );
      break;
    }
  }

  for( i = 0; i < numcols; ++i )
  {
    switch( soplex_->getBasisColStatus( i ) )
    {
    case soplex::SPxSolver::BASIC:
      ws->setStructStatus( i, CoinWarmStartBasis::basic );
      break;	  
    case soplex::SPxSolver::FIXED:
    case soplex::SPxSolver::ON_LOWER:
      ws->setStructStatus( i, CoinWarmStartBasis::atLowerBound );
      break;
    case soplex::SPxSolver::ON_UPPER:
      ws->setStructStatus( i, CoinWarmStartBasis::atUpperBound );
      break;
    case soplex::SPxSolver::ZERO:
      ws->setStructStatus( i, CoinWarmStartBasis::isFree );
      break;
    default:
      throwSPXerror( "invalid column status", "getWarmStart" );
      break;
    }
  }

  return ws;
}

//-----------------------------------------------------------------------------

bool OsiSpxSolverInterface::setWarmStart(const CoinWarmStart* warmstart)
{
  const CoinWarmStartBasis* ws = dynamic_cast<const CoinWarmStartBasis*>(warmstart);
  int numcols, numrows, i;
  soplex::SPxSolver::VarStatus *cstat, *rstat;
  bool retval = false;

  if( !ws )
    return false;

  numcols = ws->getNumStructural();
  numrows = ws->getNumArtificial();
  
  if( numcols != getNumCols() || numrows != getNumRows() )
    return false;

  cstat = new soplex::SPxSolver::VarStatus[numcols];
  rstat = new soplex::SPxSolver::VarStatus[numrows];

  // The OSI standard assumes the artificial slack variables to have positive coefficients.  SoPlex uses the convention
  // Ax - s = 0, lhs <= s <= rhs, so we have to invert the atLowerBound and atUpperBound statuses.
  for( i = 0; i < numrows; ++i )
    {
      switch( ws->getArtifStatus( i ) )
	{
	case CoinWarmStartBasis::basic:
	  rstat[i] = soplex::SPxSolver::BASIC;
	  break;
	case CoinWarmStartBasis::atLowerBound:
	  rstat[i] = soplex::SPxSolver::ON_UPPER;
	  break;
	case CoinWarmStartBasis::atUpperBound:
	  rstat[i] = soplex::SPxSolver::ON_LOWER;
	  break;
	case CoinWarmStartBasis::isFree:
	  rstat[i] = soplex::SPxSolver::ZERO;
	  break;
	default:  // unknown row status
	  retval = false;
	  goto TERMINATE;
	}
    }
  for( i = 0; i < numcols; ++i )
    {
      switch( ws->getStructStatus( i ) )
	{
	case CoinWarmStartBasis::basic:
	  cstat[i] = soplex::SPxSolver::BASIC;
	  break;
	case CoinWarmStartBasis::atLowerBound:
	  cstat[i] = soplex::SPxSolver::ON_LOWER;
	  break;
	case CoinWarmStartBasis::atUpperBound:
	  cstat[i] = soplex::SPxSolver::ON_UPPER;
	  break;
	case CoinWarmStartBasis::isFree:
	  cstat[i] = soplex::SPxSolver::ZERO;
	  break;
	default:  // unknown column status
	  retval = false;
	  goto TERMINATE;
	}
    }

  try 
  {
    soplex_->setBasis( rstat, cstat );
  } catch (soplex::SPxException e) {
    std::cerr << "SoPlex setting starting basis failed with exception " << e.what() << std::endl;
    retval = false;
    goto TERMINATE;
  }
  retval = true;

 TERMINATE:
  delete[] cstat;
  delete[] rstat;
  return retval;
}

//#############################################################################
// Hotstart related methods (primarily used in strong branching)
//#############################################################################

void OsiSpxSolverInterface::markHotStart()
{
  int numcols, numrows;

  numcols = getNumCols();
  numrows = getNumRows();
  if( numcols > hotStartCStatSize_ )
    {
      delete[] reinterpret_cast<soplex::SPxSolver::VarStatus*>(hotStartCStat_);
      hotStartCStatSize_ = static_cast<int>( 1.2 * static_cast<double>( numcols ) ); // get some extra space for future hot starts
      hotStartCStat_ = reinterpret_cast<void*>(new soplex::SPxSolver::VarStatus[hotStartCStatSize_]);
    }
  if( numrows > hotStartRStatSize_ )
    {
      delete[] reinterpret_cast<soplex::SPxSolver::VarStatus*>(hotStartRStat_);
      hotStartRStatSize_ = static_cast<int>( 1.2 * static_cast<double>( numrows ) ); // get some extra space for future hot starts
      hotStartRStat_ = reinterpret_cast<void*>(new soplex::SPxSolver::VarStatus[hotStartRStatSize_]);
    }
  soplex_->getBasis( reinterpret_cast<soplex::SPxSolver::VarStatus*>(hotStartRStat_), reinterpret_cast<soplex::SPxSolver::VarStatus*>(hotStartCStat_) );
}

void OsiSpxSolverInterface::solveFromHotStart()
{
  int maxiter;

  assert( getNumCols() <= hotStartCStatSize_ );
  assert( getNumRows() <= hotStartRStatSize_ );

  // @todo why the hotstart basis is not set here ?????
  //soplex_->setBasis(  reinterpret_cast<soplex::SPxSolver::VarStatus*>(hotStartRStat_),  reinterpret_cast<soplex::SPxSolver::VarStatus*>(hotStartCStat_) );

  maxiter = soplex_->terminationIter();
  soplex_->setTerminationIter( hotStartMaxIteration_ );

  resolve();

  soplex_->setTerminationIter( maxiter );
}

void OsiSpxSolverInterface::unmarkHotStart()
{
  // be lazy with deallocating memory and do nothing here, deallocate memory in the destructor
}

//#############################################################################
// Problem information methods (original data)
//#############################################################################

//------------------------------------------------------------------
// Get number of rows, columns, elements, ...
//------------------------------------------------------------------
int OsiSpxSolverInterface::getNumCols() const
{
  return soplex_->nCols();
}
int OsiSpxSolverInterface::getNumRows() const
{
  return soplex_->nRows();
}
int OsiSpxSolverInterface::getNumElements() const
{
#if 0
  return soplex_->nNzos();
#else   
  int retVal = 0;
  int nrows  = getNumRows();
  int row;
  for( row = 0; row < nrows; ++row ) {
    const soplex::SVector& rowvec = soplex_->rowVector( row );
    retVal += rowvec.size();
  }
  return retVal;
#endif
}

//------------------------------------------------------------------
// Get pointer to rim vectors
//------------------------------------------------------------------  

const double * OsiSpxSolverInterface::getColLower() const
{
  const double * retVal = NULL;
  if ( getNumCols()!=0 ) 
   retVal = soplex_->lower().get_const_ptr();
  return retVal;
}
//------------------------------------------------------------------
const double * OsiSpxSolverInterface::getColUpper() const
{
  const double * retVal = NULL;
  if ( getNumCols()!=0 ) 
   retVal = soplex_->upper().get_const_ptr();
  return retVal;
}
//------------------------------------------------------------------
const char * OsiSpxSolverInterface::getRowSense() const
{
  if ( rowsense_ == NULL )
    {
      // rowsense is determined with rhs, so invoke rhs
      getRightHandSide();
      assert( rowsense_ != NULL || getNumRows() == 0 );
    }
  return rowsense_;
}
//------------------------------------------------------------------
const double * OsiSpxSolverInterface::getRightHandSide() const
{
  if ( rhs_ == NULL )
    {
      int nrows = getNumRows();
      if( nrows > 0 ) 
	{
	  int    row;

	  assert( rowrange_ == NULL );
	  assert( rowsense_ == NULL );

	  rhs_      = new double[nrows];
	  rowrange_ = new double[nrows];
	  rowsense_ = new char[nrows];
	  
	  for( row = 0; row < nrows; ++row )
	    convertBoundToSense( soplex_->lhs( row ), soplex_->rhs( row ),
				 rowsense_[row], rhs_[row], rowrange_[row] );
	}
    }
  return rhs_;
}
//------------------------------------------------------------------
const double * OsiSpxSolverInterface::getRowRange() const
{
  if ( rowrange_==NULL ) 
    {
      // rowrange is determined with rhs, so invoke rhs
      getRightHandSide();
      assert( rowrange_ != NULL || getNumRows() == 0 );
    }
  return rowrange_;
}
//------------------------------------------------------------------
const double * OsiSpxSolverInterface::getRowLower() const
{
  const double * retVal = NULL;
  if ( getNumRows() != 0 )
     retVal = soplex_->lhs().get_const_ptr();
  return retVal;
}
//------------------------------------------------------------------
const double * OsiSpxSolverInterface::getRowUpper() const
{  
  const double * retVal = NULL;
  if ( getNumRows() != 0 )
     retVal = soplex_->rhs().get_const_ptr();
  return retVal;
}
//------------------------------------------------------------------
const double * OsiSpxSolverInterface::getObjCoefficients() const
{
  const double * retVal = NULL;
  if( obj_ == NULL ) {
    if ( getNumCols()!=0 ) {
      obj_ = new soplex::DVector( getNumCols() );
      soplex_->getObj( *obj_ );
      retVal = obj_->get_const_ptr();
    }
  }
  else {
    retVal = obj_->get_const_ptr();
  }
  return retVal;
}
//------------------------------------------------------------------
double OsiSpxSolverInterface::getObjSense() const
{
  switch( soplex_->spxSense() )
    {
    case soplex::SPxLP::MINIMIZE:
      return +1.0;
    case soplex::SPxLP::MAXIMIZE:
      return -1.0;
    default:
      throwSPXerror( "invalid optimization sense", "getObjSense" );
      return 0.0;
    }
}

//------------------------------------------------------------------
// Return information on integrality
//------------------------------------------------------------------

bool OsiSpxSolverInterface::isContinuous( int colNumber ) const
{
  return( spxintvars_->number( colNumber ) < 0 );
}

//------------------------------------------------------------------
// Row and column copies of the matrix ...
//------------------------------------------------------------------

const CoinPackedMatrix * OsiSpxSolverInterface::getMatrixByRow() const
{
  if( matrixByRow_ == NULL )
    {
      int    nrows     = getNumRows();
      int    ncols     = getNumCols();
      int    nelems    = getNumElements();
      double *elements = new double [nelems];
      int    *indices  = new int    [nelems];
      int    *starts   = new int    [nrows+1];
      int    *len      = new int    [nrows];
      int    row, i, elem;

      elem = 0;
      for( row = 0; row < nrows; ++row )
	{
	  const soplex::SVector& rowvec = soplex_->rowVector( row );
	  starts[row] = elem;
	  len   [row] = rowvec.size();
	  for( i = 0; i < len[row]; ++i, ++elem )
	    {
	      assert( elem < nelems );
	      elements[elem] = rowvec.value( i );
	      indices [elem] = rowvec.index( i );
	    }
	}
      starts[nrows] = elem;
      assert( elem == nelems );

      matrixByRow_ = new CoinPackedMatrix();
      matrixByRow_->assignMatrix( false /* not column ordered */,
				  ncols, nrows, nelems,
				  elements, indices, starts, len );      
    }
  return matrixByRow_;
} 

//------------------------------------------------------------------

const CoinPackedMatrix * OsiSpxSolverInterface::getMatrixByCol() const
{
  if( matrixByCol_ == NULL )
    {
      int    nrows     = getNumRows();
      int    ncols     = getNumCols();
      int    nelems    = getNumElements();
      double *elements = new double [nelems];
      int    *indices  = new int    [nelems];
      int    *starts   = new int    [ncols+1];
      int    *len      = new int    [ncols];
      int    col, i, elem;

      elem = 0;
      for( col = 0; col < ncols; ++col )
	{
	  const soplex::SVector& colvec = soplex_->colVector( col );
	  starts[col] = elem;
	  len   [col] = colvec.size();
	  for( i = 0; i < len[col]; ++i, ++elem )
	    {
	      assert( elem < nelems );
	      elements[elem] = colvec.value( i );
	      indices [elem] = colvec.index( i );
	    }
	}
      starts[ncols] = elem;
      assert( elem == nelems );

      matrixByCol_ = new CoinPackedMatrix();
      matrixByCol_->assignMatrix( true /* column ordered */,
				  nrows, ncols, nelems,
				  elements, indices, starts, len );      
    }
  return matrixByCol_;
} 

//------------------------------------------------------------------
// Get solver's value for infinity
//------------------------------------------------------------------
double OsiSpxSolverInterface::getInfinity() const
{
  return soplex::infinity;
}

//#############################################################################
// Problem information methods (results)
//#############################################################################

// *FIXME*: what should be done if a certain vector doesn't exist???

const double * OsiSpxSolverInterface::getColSolution() const
{
  if( colsol_ == NULL )
  {
  	int ncols = getNumCols();
  	if( ncols > 0 )
  	{
  		colsol_ = new soplex::DVector( ncols );
  		if( isProvenOptimal() )
  			soplex_->getPrimal( *colsol_ );
  		else
  			*colsol_ = soplex_->lower();
  	}
  	else
  		return NULL;
  }
  return colsol_->get_const_ptr();
}
//------------------------------------------------------------------
const double * OsiSpxSolverInterface::getRowPrice() const
{
  if( rowsol_ == NULL )
    {
      int nrows = getNumRows();
      if( nrows > 0 )
	{
	  rowsol_ = new soplex::DVector( nrows );
	  if( isProvenOptimal() )
	    soplex_->getDual( *rowsol_ );
	  else
	    rowsol_->clear();
	}
      else
	return NULL;
    }
  return rowsol_->get_const_ptr();
}
//------------------------------------------------------------------
const double * OsiSpxSolverInterface::getReducedCost() const
{
  if( redcost_ == NULL )
    {
      int ncols = getNumCols();
      if( ncols > 0 )
	{
	  redcost_ = new soplex::DVector( ncols );
	  if( isProvenOptimal() )
	    soplex_->getRedCost( *redcost_ );
	  else
	    redcost_->clear();
	}
      else
	return NULL;
    }
  return redcost_->get_const_ptr();
}
//------------------------------------------------------------------
const double * OsiSpxSolverInterface::getRowActivity() const
{
  if( rowact_ == NULL )
    {
      int nrows = getNumRows();
      if( nrows > 0 )
	{
	  rowact_ = new soplex::DVector( nrows );
	  if( isProvenOptimal() )
	    soplex_->getSlacks( *rowact_ );
	  else
	    rowact_->clear();
	}
      else
	return NULL;
    }
  return rowact_->get_const_ptr();
}
//------------------------------------------------------------------
double OsiSpxSolverInterface::getObjValue() const
{
  double objval;

  switch( soplex_->status() )
    {
    case soplex::SPxSolver::OPTIMAL:
    case soplex::SPxSolver::UNBOUNDED:
    case soplex::SPxSolver::INFEASIBLE:
      objval = soplex_->objValue();
      break;
    default:
    {
    	const double* colsol = getColSolution();
    	const double* objcoef = getObjCoefficients();
    	int ncols = getNumCols();
      objval = 0.0;
      for( int i = 0; i < ncols; ++i )
      	objval += colsol[i] * objcoef[i];
      break;
    }
    }

  // Adjust objective function value by constant term in objective function
  double objOffset;
  getDblParam(OsiObjOffset,objOffset);
  objval = objval - objOffset;

  return objval;
}
//------------------------------------------------------------------
int OsiSpxSolverInterface::getIterationCount() const
{
  return soplex_->iteration();
}
//------------------------------------------------------------------
std::vector<double*> OsiSpxSolverInterface::getDualRays(int maxNumRays,
							bool fullRay) const
{
  if (fullRay == true) {
    throw CoinError("Full dual rays not yet implemented.","getDualRays",
		    "OsiSpxSolverInterface");
  }

  std::vector<double*> ret = std::vector<double*>();

  if (soplex_->status() == soplex::SPxSolver::INFEASIBLE && maxNumRays > 0)
  {
    ret.push_back(new double[getNumRows()]);
    soplex::Vector proof(getNumRows(), ret[0]);
    soplex_->getDualfarkas(proof);

    for (int i = 0; i < getNumRows(); ++i)
      ret[0][i] *= -1.0;
  }

  return ret;
}
//------------------------------------------------------------------
std::vector<double*> OsiSpxSolverInterface::getPrimalRays(int maxNumRays) const
{
  // *FIXME* : must write the method
  throw CoinError("method is not yet written", "getPrimalRays",
		  "OsiSpxSolverInterface");
  return std::vector<double*>();
}

//#############################################################################
// Problem modifying methods (rim vectors)
//#############################################################################

void OsiSpxSolverInterface::setObjCoeff( int elementIndex, double elementValue )
{
  soplex_->changeObj( elementIndex, elementValue );
  freeCachedData( OsiSpxSolverInterface::FREECACHED_COLUMN );
}

void OsiSpxSolverInterface::setColLower(int elementIndex, double elementValue)
{
  soplex_->changeLower( elementIndex, elementValue );
  freeCachedData( OsiSpxSolverInterface::FREECACHED_COLUMN );
}
//-----------------------------------------------------------------------------
void OsiSpxSolverInterface::setColUpper(int elementIndex, double elementValue)
{  
  soplex_->changeUpper( elementIndex, elementValue );
  freeCachedData( OsiSpxSolverInterface::FREECACHED_COLUMN );
} 
//-----------------------------------------------------------------------------
void OsiSpxSolverInterface::setColBounds( int elementIndex, double lower, double upper )
{
  soplex_->changeBounds( elementIndex, lower, upper );
  freeCachedData( OsiSpxSolverInterface::FREECACHED_COLUMN );
}
//-----------------------------------------------------------------------------
void
OsiSpxSolverInterface::setRowLower( int i, double elementValue )
{
  soplex_->changeLhs( i, elementValue );
  freeCachedData( OsiSpxSolverInterface::FREECACHED_ROW );
}
//-----------------------------------------------------------------------------
void
OsiSpxSolverInterface::setRowUpper( int i, double elementValue )
{
  soplex_->changeRhs( i, elementValue );
  freeCachedData( OsiSpxSolverInterface::FREECACHED_ROW );
}
//-----------------------------------------------------------------------------
void
OsiSpxSolverInterface::setRowBounds( int elementIndex, double lower, double upper )
{
  soplex_->changeRange( elementIndex, lower, upper );
  freeCachedData( OsiSpxSolverInterface::FREECACHED_ROW );
}
//-----------------------------------------------------------------------------
void
OsiSpxSolverInterface::setRowType(int i, char sense, double rightHandSide,
				  double range)
{
  double lower = 0.0;
  double upper = 0.0;

  convertSenseToBound( sense, rightHandSide, range, lower, upper );
  setRowBounds( i, lower, upper );
}
//#############################################################################
void
OsiSpxSolverInterface::setContinuous(int index)
{
  int pos = spxintvars_->number( index );
  if( pos >= 0 )
    {
      spxintvars_->remove( pos );
      freeCachedData( OsiSpxSolverInterface::FREECACHED_COLUMN );
    }
}
//-----------------------------------------------------------------------------
void
OsiSpxSolverInterface::setInteger(int index)
{
  int pos = spxintvars_->number( index );
  if( pos < 0 )
    {
      spxintvars_->addIdx( index );
      freeCachedData( OsiSpxSolverInterface::FREECACHED_COLUMN );
    }
}
//#############################################################################

void OsiSpxSolverInterface::setObjSense(double s) 
{
  if( s != getObjSense() )
    {
      if( s == +1.0 )
	soplex_->changeSense( soplex::SPxLP::MINIMIZE );
      else
	soplex_->changeSense( soplex::SPxLP::MAXIMIZE );
      freeCachedData( OsiSpxSolverInterface::FREECACHED_RESULTS );
    }
}
 
//-----------------------------------------------------------------------------

void OsiSpxSolverInterface::setColSolution(const double * cs) 
{
  int col;
  int ncols = getNumCols();

  if( colsol_ != NULL )
    delete colsol_;

  if( ncols > 0 && cs != NULL )
    {
      colsol_ = new soplex::DVector( ncols );
      for( col = 0; col < ncols; ++col )
	(*colsol_)[col] = cs[col];
    }
  else
    colsol_ = NULL;
}

//-----------------------------------------------------------------------------

void OsiSpxSolverInterface::setRowPrice(const double * rs) 
{
  int row;
  int nrows = getNumRows();

  if( rowsol_ != NULL )
    delete rowsol_;

  if( nrows > 0 && rs != NULL )
    {
      rowsol_ = new soplex::DVector( nrows );
      for( row = 0; row < nrows; ++row )
	(*rowsol_)[row] = rs[row];
    }
  else
    rowsol_ = NULL;
}

//#############################################################################
// Problem modifying methods (matrix)
//#############################################################################
void 
OsiSpxSolverInterface::addCol(const CoinPackedVectorBase& vec,
			      const double collb, const double colub,   
			      const double obj)
{
  soplex::DSVector colvec;

  colvec.add( vec.getNumElements(), vec.getIndices(), vec.getElements() );
  soplex_->addCol( soplex::LPCol( obj, colvec, colub, collb ) );
  freeCachedData( OsiSpxSolverInterface::KEEPCACHED_ROW );
}
//-----------------------------------------------------------------------------
void 
OsiSpxSolverInterface::deleteCols(const int num, const int * columnIndices)
{
  soplex_->removeCols( const_cast<int*>(columnIndices), num );
  freeCachedData( OsiSpxSolverInterface::KEEPCACHED_ROW );

  // took from OsiClp for updating names
  int nameDiscipline;
  getIntParam(OsiNameDiscipline,nameDiscipline) ;
  if (num && nameDiscipline) {
     // Very clumsy (and inefficient) - need to sort and then go backwards in ? chunks
     int * indices = CoinCopyOfArray(columnIndices,num);
     std::sort(indices,indices+num);
     int num2 = num;
     while (num2) {
       int next = indices[num2-1];
       int firstDelete = num2-1;
       int i;
       for (i = num2-2; i>=0; --i) {
          if (indices[i]+1 == next) {
             --next;
             firstDelete = i;
          } else {
             break;
          }
       }
       OsiSolverInterface::deleteColNames(indices[firstDelete],num2-firstDelete);
       num2 = firstDelete;
       assert (num2>=0);
     }
     delete [] indices;
  }
}
//-----------------------------------------------------------------------------
void 
OsiSpxSolverInterface::addRow(const CoinPackedVectorBase& vec,
			      const double rowlb, const double rowub)
{
  soplex::DSVector rowvec;

  rowvec.add( vec.getNumElements(), vec.getIndices(), vec.getElements() );
  soplex_->addRow( soplex::LPRow( rowlb, rowvec, rowub ) );
  freeCachedData( OsiSpxSolverInterface::KEEPCACHED_COLUMN );
}
//-----------------------------------------------------------------------------
void 
OsiSpxSolverInterface::addRow(const CoinPackedVectorBase& vec,
			      const char rowsen, const double rowrhs,   
			      const double rowrng)
{
  double rowlb = 0.0;
  double rowub = 0.0;

  convertSenseToBound( rowsen, rowrhs, rowrng, rowlb, rowub );
  addRow( vec, rowlb, rowub );
}
//-----------------------------------------------------------------------------
void 
OsiSpxSolverInterface::deleteRows(const int num, const int * rowIndices)
{
  soplex_->removeRows( const_cast<int*>(rowIndices), num );
  freeCachedData( OsiSpxSolverInterface::KEEPCACHED_COLUMN );

  // took from OsiClp for updating names
  int nameDiscipline;
  getIntParam(OsiNameDiscipline,nameDiscipline) ;
  if (num && nameDiscipline) {
    // Very clumsy (and inefficient) - need to sort and then go backwards in ? chunks
    int * indices = CoinCopyOfArray(rowIndices,num);
    std::sort(indices,indices+num);
    int num2=num;
    while (num2) {
      int next = indices[num2-1];
      int firstDelete = num2-1;
      int i;
      for (i = num2-2; i>=0; --i) {
        if (indices[i]+1 == next) {
        	--next;
	        firstDelete = i;
        } else {
          break;
        }
      }
      OsiSolverInterface::deleteRowNames(indices[firstDelete],num2-firstDelete);
      num2 = firstDelete;
      assert(num2 >= 0);
    }
    delete [] indices;
  }
}

//#############################################################################
// Methods to input a problem
//#############################################################################

void
OsiSpxSolverInterface::loadProblem( const CoinPackedMatrix& matrix,
				    const double* collb, const double* colub,
				    const double* obj,
				    const double* rowlb, const double* rowub )
{
  int          ncols   = matrix.getNumCols();
  int          nrows   = matrix.getNumRows();
  const int    *length = matrix.getVectorLengths();
  const int    *start  = matrix.getVectorStarts();
  const double *elem   = matrix.getElements();
  const int    *index  = matrix.getIndices();
  double       *thecollb, *thecolub, *theobj, *therowlb, *therowub;
  
  // create defaults if parameter is NULL
  if( collb == NULL )
  {
    thecollb = new double[ncols];
    CoinFillN( thecollb, ncols, 0.0 );
  }
  else
    thecollb = const_cast<double*>(collb);
  if( colub == NULL )
  {
    thecolub = new double[ncols];
    CoinFillN( thecolub, ncols, getInfinity() );
  }
  else
    thecolub = const_cast<double*>(colub);
  if( obj == NULL )
  {
    theobj = new double[ncols];
    CoinFillN( theobj, ncols, 0.0 );
  }
  else
    theobj = const_cast<double*>(obj);
  if( rowlb == NULL )
  {
    therowlb = new double[nrows];
    CoinFillN( therowlb, nrows, -getInfinity() );
  }
  else
    therowlb = const_cast<double*>(rowlb);
  if( rowub == NULL )
  {
    therowub = new double[nrows];
    CoinFillN( therowub, nrows, +getInfinity() );
  }
  else
    therowub = const_cast<double*>(rowub);

  // copy problem into soplex_
  soplex_->clear();
  spxintvars_->clear();
  freeCachedData( OsiSpxSolverInterface::KEEPCACHED_NONE );

  if( matrix.isColOrdered() )
    {
      int row, col, pos;
      soplex::LPRowSet rowset( nrows, 0 );
      soplex::DSVector rowvec;
      soplex::LPColSet colset( ncols, matrix.getNumElements() );
      soplex::DSVector colvec;

      /* insert empty rows */
      rowvec.clear();
      for( row = 0; row < nrows; ++row )
         rowset.add( therowlb[row], rowvec, therowub[row] );
      soplex_->addRows( rowset );

      /* create columns */
      for( col = 0; col < ncols; ++col )
	{
	  pos = start[col];
	  colvec.clear();
	  colvec.add( length[col], &(index[pos]), &(elem[pos]) );
	  colset.add( theobj[col], thecollb[col], colvec, thecolub[col] );
	}
      
      soplex_->addCols( colset );
      // soplex_->changeRange( soplex::Vector( nrows, therowlb ), soplex::Vector( nrows, therowub ) );
    }
  else
    {
      int row, col, pos;
      soplex::LPRowSet rowset( nrows, matrix.getNumElements() );
      soplex::DSVector rowvec;
      soplex::LPColSet colset( ncols, 0 );
      soplex::DSVector colvec;

      /* insert empty columns */
      colvec.clear();
      for( col = 0; col < ncols; ++col )
         colset.add( theobj[col], thecollb[col], colvec, thecolub[col] );
      soplex_->addCols( colset );

      /* create rows */
      for( row = 0; row < nrows; ++row )
	{
	  pos = start[row];
	  rowvec.clear();
	  rowvec.add( length[row], &(index[pos]), &(elem[pos]) );
	  rowset.add( therowlb[row], rowvec, therowub[row] );
	}
      
      soplex_->addRows( rowset );
      // soplex_->changeObj( soplex::Vector( ncols, theobj ) );
      // soplex_->changeBounds( soplex::Vector( ncols, thecollb ), soplex::Vector( ncols, thecolub ) );
    }

  // switch sense to minimization problem
  soplex_->changeSense( soplex::SPxSolver::MINIMIZE );

  // delete default arrays if neccessary
  if( collb == NULL )
    delete[] thecollb;
  if( colub == NULL )
    delete[] thecolub;
  if( obj == NULL )
    delete[] theobj;
  if( rowlb == NULL )
    delete[] therowlb;
  if( rowub == NULL )
    delete[] therowub;
}
			    
//-----------------------------------------------------------------------------

void
OsiSpxSolverInterface::assignProblem( CoinPackedMatrix*& matrix,
				      double*& collb, double*& colub,
				      double*& obj,
				      double*& rowlb, double*& rowub )
{
  loadProblem( *matrix, collb, colub, obj, rowlb, rowub );
  delete matrix;   matrix = 0;
  delete[] collb;  collb = 0;
  delete[] colub;  colub = 0;
  delete[] obj;    obj = 0;
  delete[] rowlb;  rowlb = 0;
  delete[] rowub;  rowub = 0;
}

//-----------------------------------------------------------------------------

void
OsiSpxSolverInterface::loadProblem( const CoinPackedMatrix& matrix,
				    const double* collb, const double* colub,
				    const double* obj,
				    const char* rowsen, const double* rowrhs,
				    const double* rowrng )
{
  int     nrows = matrix.getNumRows();
  double* rowlb = new double[nrows];
  double* rowub = new double[nrows];
  int     row;
  char    *therowsen;
  double  *therowrhs, *therowrng;

  if( rowsen == NULL )
  {
    therowsen = new char[nrows];
    CoinFillN( therowsen, nrows, 'G' );
  }
  else
    therowsen = const_cast<char*>(rowsen);
  if( rowrhs == NULL )
  {
    therowrhs = new double[nrows];
    CoinFillN( therowrhs, nrows, 0.0 );
  }
  else
    therowrhs = const_cast<double*>(rowrhs);
  if( rowrng == NULL )
  {
    therowrng = new double[nrows];
    CoinFillN( therowrng, nrows, 0.0 );
  }
  else
    therowrng = const_cast<double*>(rowrng);

  for( row = 0; row < nrows; ++row )
    convertSenseToBound( therowsen[row], therowrhs[row], therowrng[row], rowlb[row], rowub[row] );
  
  loadProblem( matrix, collb, colub, obj, rowlb, rowub );

  if( rowsen == NULL )
    delete[] therowsen;
  if( rowrhs == NULL )
    delete[] therowrhs;
  if( rowrng == NULL )
    delete[] therowrng;

  delete[] rowlb;
  delete[] rowub;
}
   
//-----------------------------------------------------------------------------

void
OsiSpxSolverInterface::assignProblem( CoinPackedMatrix*& matrix,
				      double*& collb, double*& colub,
				      double*& obj,
				      char*& rowsen, double*& rowrhs,
				      double*& rowrng )
{
   loadProblem( *matrix, collb, colub, obj, rowsen, rowrhs, rowrng );
   delete matrix;   matrix = 0;
   delete[] collb;  collb = 0;
   delete[] colub;  colub = 0;
   delete[] obj;    obj = 0;
   delete[] rowsen; rowsen = 0;
   delete[] rowrhs; rowrhs = 0;
   delete[] rowrng; rowrng = 0;
}

//-----------------------------------------------------------------------------

void
OsiSpxSolverInterface::loadProblem(const int numcols, const int numrows,
				   const int* start, const int* index,
				   const double* value,
				   const double* collb, const double* colub,   
				   const double* obj,
				   const double* rowlb, const double* rowub )
{
  int col, pos;
  soplex::LPColSet colset( numcols, start[numcols] );
  soplex::DSVector colvec;
  
  soplex_->clear();
  spxintvars_->clear();
  freeCachedData( OsiSpxSolverInterface::KEEPCACHED_NONE );

  for( col = 0; col < numcols; ++col )
    {
      pos = start[col];
      colvec.clear();
      colvec.add( start[col+1] - pos, &(index[pos]), &(value[pos]) );
      colset.add( obj[col], collb[col], colvec, colub[col] );
    }
  
  soplex_->addCols( colset );
  soplex_->changeRange( soplex::Vector( numrows, const_cast<double*>(rowlb) ), soplex::Vector( numrows, const_cast<double*>(rowub) ) );
}

//-----------------------------------------------------------------------------

void
OsiSpxSolverInterface::loadProblem(const int numcols, const int numrows,
				   const int* start, const int* index,
				   const double* value,
				   const double* collb, const double* colub,   
				   const double* obj,
				   const char* rowsen, const double* rowrhs,
				   const double* rowrng )
{
  double* rowlb = new double[numrows];
  double* rowub = new double[numrows];
  int     row;

  for( row = 0; row < numrows; ++row )
    convertSenseToBound( rowsen != NULL ? rowsen[row] : 'G', rowrhs != NULL ? rowrhs[row] : 0.0, rowrng != NULL ? rowrng[row] : 0.0, rowlb[row], rowub[row] );
  
  loadProblem( numcols, numrows, start, index, value, collb, colub, obj, rowlb, rowub );

  delete[] rowlb;
  delete[] rowub;
}
 
//-----------------------------------------------------------------------------
// Read mps files
//-----------------------------------------------------------------------------
int OsiSpxSolverInterface::readMps( const char * filename,
				    const char * extension )
{
  #if 0
  std::string f(filename);
  std::string e(extension);
  std::string fullname = f + "." + e;
  std::ifstream file(fullname.c_str());
  if (!file.good()) 
  {
    std::cerr << "Error opening file " << fullname << " for reading!" << std::endl;
    return 1;
  }

  soplex_->clear();
  if( !soplex_->readMPS(file, NULL, NULL, NULL) )
    throwSPXerror( "error reading file <" + fullname + ">", "readMps" );
  #endif

  // we preserve the objective sense independent of the problem which is read
  soplex::SPxLP::SPxSense objsen = soplex_->spxSense();
  int retval = OsiSolverInterface::readMps(filename,extension);
  soplex_->changeSense(objsen);
  return retval;
}

//-----------------------------------------------------------------------------
// Write mps files
//-----------------------------------------------------------------------------
void OsiSpxSolverInterface::writeMps( const char * filename,
				      const char * extension,
				      double objSense) const
{
  std::string f(filename);
  std::string e(extension);
  std::string fullname = f+"."+e;
  std::ofstream file(fullname.c_str());
  if (!file.good()) 
  {
    std::cerr << "Error opening file " << fullname << " for writing!" << std::endl;
    return;
  }
  soplex::DIdxSet integers(getNumIntegers());
  for (int i = 0; i < getNumCols(); ++i)
  	if (isInteger(i))
  		integers.addIdx(i);
  soplex_->writeMPS(file, NULL, NULL, &integers);
}

//#############################################################################
// Constructors, destructors clone and assignment
//#############################################################################

//-------------------------------------------------------------------
// Default Constructor 
//-------------------------------------------------------------------
OsiSpxSolverInterface::OsiSpxSolverInterface ()
  : OsiSolverInterface(),
    soplex_(new soplex::SoPlex(soplex::SPxSolver::ENTER, soplex::SPxSolver::COLUMN)), // default is primal simplex algorithm
    spxintvars_(new soplex::DIdxSet()),
    hotStartCStat_(NULL),
    hotStartCStatSize_(0),
    hotStartRStat_(NULL),
    hotStartRStatSize_(0),
    hotStartMaxIteration_(1000000), // ??? default iteration limit for strong branching is large
    obj_(NULL),
    rowsense_(NULL),
    rhs_(NULL),
    rowrange_(NULL),
    colsol_(NULL),
    rowsol_(NULL),
    redcost_(NULL),
    rowact_(NULL),
    matrixByRow_(NULL),
    matrixByCol_(NULL)
{
#ifndef NDEBUG
  soplex::Param::setVerbose( 3 );
#else
  soplex::Param::setVerbose( 2 );
#endif

  // SoPlex default objective sense is maximization, thus we explicitly set it to minimization
  soplex_->changeSense( soplex::SPxLP::MINIMIZE );
}


//----------------------------------------------------------------
// Clone
//----------------------------------------------------------------
OsiSolverInterface * OsiSpxSolverInterface::clone(bool copyData) const
{
  return( new OsiSpxSolverInterface( *this ) );
}

//-------------------------------------------------------------------
// Copy constructor 
//-------------------------------------------------------------------
OsiSpxSolverInterface::OsiSpxSolverInterface( const OsiSpxSolverInterface & source )
  : OsiSolverInterface(source),
    soplex_(new soplex::SoPlex(*source.soplex_)),
    spxintvars_(new soplex::DIdxSet(*source.spxintvars_)),
    hotStartCStat_(NULL),
    hotStartCStatSize_(0),
    hotStartRStat_(NULL),
    hotStartRStatSize_(0),
    hotStartMaxIteration_(source.hotStartMaxIteration_),
    obj_(NULL),
    rowsense_(NULL),
    rhs_(NULL),
    rowrange_(NULL),
    colsol_(NULL),
    rowsol_(NULL),
    redcost_(NULL),
    rowact_(NULL),
    matrixByRow_(NULL),
    matrixByCol_(NULL)
{
  if (source.colsol_ != NULL) setColSolution(source.getColSolution());
  if (source.rowsol_ != NULL) setRowPrice(source.getRowPrice());
}


//-------------------------------------------------------------------
// Destructor 
//-------------------------------------------------------------------
OsiSpxSolverInterface::~OsiSpxSolverInterface ()
{
  freeAllMemory();
}

//----------------------------------------------------------------
// Assignment operator 
//-------------------------------------------------------------------
OsiSpxSolverInterface& OsiSpxSolverInterface::operator=( const OsiSpxSolverInterface& source )
{
  if (this != &source)
    {    
      freeAllMemory();

      OsiSolverInterface::operator=( source );
      spxintvars_ = new soplex::DIdxSet(*source.spxintvars_);
      soplex_ = new soplex::SoPlex(*source.soplex_);
      if (source.colsol_ != NULL) setColSolution(source.getColSolution());
      if (source.rowsol_ != NULL) setRowPrice(source.getRowPrice());
      hotStartMaxIteration_ = source.hotStartMaxIteration_;
    }
  return *this;
}

//#############################################################################
// Applying cuts
//#############################################################################

void OsiSpxSolverInterface::applyColCut( const OsiColCut & cc )
{
  const double * soplexColLB = getColLower();
  const double * soplexColUB = getColUpper();
  const CoinPackedVector & lbs = cc.lbs();
  const CoinPackedVector & ubs = cc.ubs();
  int i;

  for( i = 0; i < lbs.getNumElements(); ++i ) 
    if ( lbs.getElements()[i] > soplexColLB[lbs.getIndices()[i]] )
      setColLower( lbs.getIndices()[i], lbs.getElements()[i] );
  for( i = 0; i < ubs.getNumElements(); ++i )
    if ( ubs.getElements()[i] < soplexColUB[ubs.getIndices()[i]] )
      setColUpper( ubs.getIndices()[i], ubs.getElements()[i] );
}

//-----------------------------------------------------------------------------

void OsiSpxSolverInterface::applyRowCut( const OsiRowCut & rowCut )
{
  addRow( rowCut.row(), rowCut.lb(), rowCut.ub() );
}

//#############################################################################
// Private methods (non-static and static)
//#############################################################################

//-------------------------------------------------------------------
/// free cached vectors

void OsiSpxSolverInterface::freeCachedColRim()
{
  delete obj_;
  obj_ = NULL;
}

void OsiSpxSolverInterface::freeCachedRowRim()
{
  freeCacheChar  ( rowsense_ );
  freeCacheDouble( rhs_ );
  freeCacheDouble( rowrange_ );
  assert( rowsense_ == NULL ); 
  assert( rhs_      == NULL ); 
  assert( rowrange_ == NULL ); 
 }

void OsiSpxSolverInterface::freeCachedMatrix()
{
  freeCacheMatrix( matrixByRow_ );
  freeCacheMatrix( matrixByCol_ );
  assert( matrixByRow_ == NULL ); 
  assert( matrixByCol_ == NULL ); 
}

void OsiSpxSolverInterface::freeCachedResults()
{
  delete colsol_;
  delete rowsol_;
  delete redcost_;
  delete rowact_;
  colsol_  = NULL;
  rowsol_  = NULL;
  redcost_ = NULL;
  rowact_  = NULL;
}


void OsiSpxSolverInterface::freeCachedData( int keepCached )
{
  if( !(keepCached & OsiSpxSolverInterface::KEEPCACHED_COLUMN) )
    freeCachedColRim();
  if( !(keepCached & OsiSpxSolverInterface::KEEPCACHED_ROW) )
    freeCachedRowRim();
  if( !(keepCached & OsiSpxSolverInterface::KEEPCACHED_MATRIX) )
    freeCachedMatrix();
  if( !(keepCached & OsiSpxSolverInterface::KEEPCACHED_RESULTS) )
    freeCachedResults();
}

void OsiSpxSolverInterface::freeAllMemory()
{
  freeCachedData();
  delete[] reinterpret_cast<soplex::SPxSolver::VarStatus*>(hotStartCStat_);
  delete[] reinterpret_cast<soplex::SPxSolver::VarStatus*>(hotStartRStat_);
  hotStartCStat_     = NULL;
  hotStartCStatSize_ = 0;
  hotStartRStat_     = NULL;
  hotStartRStatSize_ = 0;
  delete soplex_;
  delete spxintvars_;
}
