//-----------------------------------------------------------------------------
// name:     OSI Interface for Gurobi
// template: OSI Cplex Interface written by T. Achterberg
// author:   Stefan Vigerske
//           Humboldt University Berlin
// license:  this file may be freely distributed under the terms of EPL
// comments: please scan this file for '???' and 'TODO' and read the comments
//-----------------------------------------------------------------------------
// Copyright (C) 2009 Humboldt University Berlin and others.
// All Rights Reserved.

// $Id$

#include <iostream>
#include <cassert>
#include <string>
#include <numeric>

#include "CoinPragma.hpp"
#include "CoinError.hpp"

#include "OsiConfig.h"
#include "OsiGrbSolverInterface.hpp"
#include "OsiCuts.hpp"
#include "OsiRowCut.hpp"
#include "OsiColCut.hpp"
#include "CoinPackedMatrix.hpp"
#include "CoinWarmStartBasis.hpp"

extern "C" {
#include "gurobi_c.h"
}

/* A utility definition which allows for easy suppression of unused variable warnings from GCC.
*/
#ifndef UNUSED
# if defined(__GNUC__)
#   define UNUSED __attribute__((unused))
# else
#   define UNUSED
# endif
#endif

//#define DEBUG 1

#ifdef DEBUG
#define debugMessage printf("line %4d: ", __LINE__) & printf
#else
#define debugMessage if( false ) printf
#endif

#define GUROBI_CALL(m, x) do \
{ \
  int _retcode; \
  if( (_retcode = (x)) != 0 ) \
  { \
    char s[1001]; \
    if (OsiGrbSolverInterface::globalenv_) \
    { \
      sprintf( s, "Error <%d> from GUROBI function call: ", _retcode ); \
      strncat(s, GRBgeterrormsg(OsiGrbSolverInterface::globalenv_), 1000); \
    } \
    else \
      sprintf( s, "Error <%d> from GUROBI function call (no license?).", _retcode ); \
    debugMessage("%s:%d: %s", __FILE__, __LINE__, s); \
    throw CoinError( s, m, "OsiGrbSolverInterface", __FILE__, __LINE__ ); \
  } \
} \
while( false )

template <bool> struct CompileTimeAssert;
template<> struct CompileTimeAssert<true> {};

#if GRB_VERSION_MAJOR < 4
#define GRB_METHOD_DUAL    GRB_LPMETHOD_DUAL
#define GRB_METHOD_PRIMAL  GRB_LPMETHOD_PRIMAL
#define GRB_INT_PAR_METHOD GRB_INT_PAR_LPMETHOD
#endif

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

void
OsiGrbSolverInterface::switchToLP( void )
{
  debugMessage("OsiGrbSolverInterface::switchToLP()\n");

  if( probtypemip_ )
  {
     GRBmodel* lp = getMutableLpPtr();
     int nc = getNumCols();
     
     char* contattr = new char[nc];
     CoinFillN(contattr, nc, 'C');
     
     GUROBI_CALL( "switchToLP", GRBsetcharattrarray(lp, GRB_CHAR_ATTR_VTYPE, 0, nc, contattr) );
     
     delete[] contattr;

     probtypemip_ = false;
  }
}

void
OsiGrbSolverInterface::switchToMIP( void )
{
  debugMessage("OsiGrbSolverInterface::switchToMIP()\n");

  if( !probtypemip_ )
  {
     GRBmodel* lp = getMutableLpPtr();
     int nc = getNumCols();

     assert(coltype_ != NULL);

     GUROBI_CALL( "switchToMIP", GRBsetcharattrarray(lp, GRB_CHAR_ATTR_VTYPE, 0, nc, coltype_) );

     probtypemip_ = true;
  }
}

void
OsiGrbSolverInterface::resizeColSpace( int minsize )
{
  debugMessage("OsiGrbSolverInterface::resizeColSpace()\n");

  if( minsize > colspace_ )
  {
     int newcolspace = 2*colspace_;
     if( minsize > newcolspace )
        newcolspace = minsize;
     char* newcoltype = new char[newcolspace];

     if( coltype_ != NULL )
     {
       CoinDisjointCopyN( coltype_, colspace_, newcoltype );
       delete[] coltype_;
     }
     coltype_ = newcoltype;

     if( colmap_O2G != NULL )
     {
       int* newcolmap_O2G = new int[newcolspace];
       CoinDisjointCopyN( colmap_O2G, colspace_, newcolmap_O2G );
       delete[] colmap_O2G;
       colmap_O2G = newcolmap_O2G;
     }

     if( colmap_G2O != NULL )
     {
       int* newcolmap_G2O = new int[newcolspace + auxcolspace];
       CoinDisjointCopyN( colmap_G2O, colspace_ + auxcolspace, newcolmap_G2O );
       delete[] colmap_G2O;
       colmap_G2O = newcolmap_G2O;
     }

     colspace_ = newcolspace;
  }
  assert(minsize == 0 || coltype_ != NULL);
  assert(colspace_ >= minsize);
}

void
OsiGrbSolverInterface::freeColSpace()
{
  debugMessage("OsiGrbSolverInterface::freeColSpace()\n");

   if( colspace_ > 0 )
   {
      delete[] coltype_;
      delete[] colmap_O2G;
      delete[] colmap_G2O;
      coltype_ = NULL;
      colmap_O2G = NULL;
      colmap_G2O = NULL;
      colspace_ = 0;
      auxcolspace = 0;
   }
   assert(coltype_ == NULL);
   assert(colmap_O2G == NULL);
   assert(colmap_G2O == NULL);
}

void OsiGrbSolverInterface::resizeAuxColSpace(int minsize)
{
  debugMessage("OsiGrbSolverInterface::resizeAuxColSpace()\n");

  if( minsize > auxcolspace )
  {
     int newauxcolspace = 2*auxcolspace;
     if( minsize > newauxcolspace )
        newauxcolspace = minsize;

     if( colmap_G2O != NULL )
     {
       int* newcolmap_G2O = new int[colspace_ + newauxcolspace];
       CoinDisjointCopyN( colmap_G2O, colspace_ + auxcolspace, newcolmap_G2O );
       delete[] colmap_G2O;
       colmap_G2O = newcolmap_G2O;
     }

     auxcolspace = newauxcolspace;
  }
}

/// resizes auxcolind vector to current number of rows and inits values to -1
void OsiGrbSolverInterface::resizeAuxColIndSpace()
{
  debugMessage("OsiGrbSolverInterface::resizeAuxColIndSpace()\n");

  int numrows = getNumRows();
  if( auxcolindspace < numrows )
  {
    int newspace = 2*auxcolindspace;
    if( numrows > newspace )
       newspace = numrows;

    int* newauxcolind = new int[newspace];

    if( auxcolindspace > 0 )
      CoinDisjointCopyN( auxcolind, auxcolindspace, newauxcolind );
    for( int i = auxcolindspace; i < newspace; ++i )
      newauxcolind[i] = -1;

    delete[] auxcolind;
    auxcolind = newauxcolind;
    auxcolindspace = newspace;
  }
}

//#############################################################################
// Solve methods
//#############################################################################

void OsiGrbSolverInterface::initialSolve()
{
  debugMessage("OsiGrbSolverInterface::initialSolve()\n");
  bool takeHint;
  OsiHintStrength strength;
  int prevalgorithm = -1;

  switchToLP();
  
  GRBmodel* lp = getLpPtr( OsiGrbSolverInterface::FREECACHED_RESULTS );

  /* set whether dual or primal, if hint has been given */
  getHintParam(OsiDoDualInInitial,takeHint,strength);
  if (strength != OsiHintIgnore) {
  	GUROBI_CALL( "initialSolve", GRBgetintparam(GRBgetenv(lp), GRB_INT_PAR_METHOD, &prevalgorithm) );
  	GUROBI_CALL( "initialSolve", GRBsetintparam(GRBgetenv(lp), GRB_INT_PAR_METHOD, takeHint ? GRB_METHOD_DUAL : GRB_METHOD_PRIMAL) );
  }

	/* set whether presolve or not */
  int presolve = GRB_PRESOLVE_AUTO;
  getHintParam(OsiDoPresolveInInitial,takeHint,strength);
  if (strength != OsiHintIgnore)
  	presolve = takeHint ? GRB_PRESOLVE_AUTO : GRB_PRESOLVE_OFF;

  GUROBI_CALL( "initialSolve", GRBsetintparam(GRBgetenv(lp), GRB_INT_PAR_PRESOLVE, presolve) );

	/* set whether output or not */
  GUROBI_CALL( "initialSolve", GRBsetintparam(GRBgetenv(lp), GRB_INT_PAR_OUTPUTFLAG, (messageHandler()->logLevel() > 0)) );

	/* optimize */
  GUROBI_CALL( "initialSolve", GRBoptimize(lp) );
  
  /* reoptimize without presolve if status unclear */
  int stat;
  GUROBI_CALL( "initialSolve", GRBgetintattr(lp, GRB_INT_ATTR_STATUS, &stat) );

  if (stat == GRB_INF_OR_UNBD && presolve != GRB_PRESOLVE_OFF) {
    GUROBI_CALL( "initialSolve", GRBsetintparam(GRBgetenv(lp), GRB_INT_PAR_PRESOLVE, GRB_PRESOLVE_OFF) );
    GUROBI_CALL( "initialSolve", GRBoptimize(lp) );
    GUROBI_CALL( "initialSolve", GRBsetintparam(GRBgetenv(lp), GRB_INT_PAR_PRESOLVE, presolve) );
  }

  /* reset method parameter, if changed */
  if( prevalgorithm != -1 )
  	GUROBI_CALL( "initialSolve", GRBsetintparam(GRBgetenv(lp), GRB_INT_PAR_METHOD, prevalgorithm) );
}

//-----------------------------------------------------------------------------
void OsiGrbSolverInterface::resolve()
{
  debugMessage("OsiGrbSolverInterface::resolve()\n");
  bool takeHint;
  OsiHintStrength strength;
  int prevalgorithm = -1;

  switchToLP();
  
  GRBmodel* lp = getLpPtr( OsiGrbSolverInterface::FREECACHED_RESULTS );

  /* set whether primal or dual */
  getHintParam(OsiDoDualInResolve,takeHint,strength);
  if (strength != OsiHintIgnore) {
  	GUROBI_CALL( "resolve", GRBgetintparam(GRBgetenv(lp), GRB_INT_PAR_METHOD, &prevalgorithm) );
  	GUROBI_CALL( "resolve", GRBsetintparam(GRBgetenv(lp), GRB_INT_PAR_METHOD, takeHint ? GRB_METHOD_DUAL : GRB_METHOD_PRIMAL) );
  }

	/* set whether presolve or not */
  int presolve = GRB_PRESOLVE_OFF;
  getHintParam(OsiDoPresolveInResolve,takeHint,strength);
  if (strength != OsiHintIgnore)
  	presolve = takeHint ? GRB_PRESOLVE_AUTO : GRB_PRESOLVE_OFF;

  GUROBI_CALL( "resolve", GRBsetintparam(GRBgetenv(lp), GRB_INT_PAR_PRESOLVE, presolve) );

	/* set whether output or not */
  GUROBI_CALL( "resolve", GRBsetintparam(GRBgetenv(lp), GRB_INT_PAR_OUTPUTFLAG, (messageHandler()->logLevel() > 0)) );

	/* optimize */
  GUROBI_CALL( "resolve", GRBoptimize(lp) );

  /* reoptimize if status unclear */
  int stat;
  GUROBI_CALL( "resolve", GRBgetintattr(lp, GRB_INT_ATTR_STATUS, &stat) );

  if (stat == GRB_INF_OR_UNBD && presolve != GRB_PRESOLVE_OFF) {
    GUROBI_CALL( "resolve", GRBsetintparam(GRBgetenv(lp), GRB_INT_PAR_PRESOLVE, GRB_PRESOLVE_OFF) );
    GUROBI_CALL( "resolve", GRBoptimize(lp) );
    GUROBI_CALL( "resolve", GRBsetintparam(GRBgetenv(lp), GRB_INT_PAR_PRESOLVE, presolve) );
  }

  /* reset method parameter, if changed */
  if( prevalgorithm != -1 )
  	GUROBI_CALL( "initialSolve", GRBsetintparam(GRBgetenv(lp), GRB_INT_PAR_METHOD, prevalgorithm) );
}

//-----------------------------------------------------------------------------
void OsiGrbSolverInterface::branchAndBound()
{
  debugMessage("OsiGrbSolverInterface::branchAndBound()\n");

  switchToMIP();

  if( colsol_ != NULL && domipstart )
  {
    int* discridx = new int[getNumIntegers()];
    double* discrval  = new double[getNumIntegers()];
 
    int j = 0;
    for( int i = 0; i < getNumCols() && j < getNumIntegers(); ++i )
    {
       if( !isInteger(i) && !isBinary(i) )
          continue;
       discridx[j] = i;
       discrval[j] = colsol_[i];
       ++j;
    }
    assert(j == getNumIntegers());

    GUROBI_CALL( "branchAndBound", GRBsetdblattrlist(getMutableLpPtr(), GRB_DBL_ATTR_START, getNumIntegers(), discridx, discrval) );

    delete[] discridx;
    delete[] discrval;
  }

  GRBmodel* lp = getLpPtr( OsiGrbSolverInterface::FREECACHED_RESULTS );

  GUROBI_CALL( "branchAndBound", GRBsetintparam(GRBgetenv(lp), GRB_INT_PAR_OUTPUTFLAG, (messageHandler()->logLevel() > 0)) );

  GUROBI_CALL( "branchAndBound", GRBoptimize( lp ) );
}

//#############################################################################
// Parameter related methods
//#############################################################################

bool
OsiGrbSolverInterface::setIntParam(OsiIntParam key, int value)
{
  debugMessage("OsiGrbSolverInterface::setIntParam(%d, %d)\n", key, value);

  switch (key)
  {
    case OsiMaxNumIteration:
      if( GRBsetdblparam(GRBgetenv(getMutableLpPtr()), GRB_DBL_PAR_ITERATIONLIMIT, (double)value) != 0 )
      {
        *messageHandler() << "Warning: Setting GUROBI iteration limit to" << value << "failed." << CoinMessageEol;
        if (OsiGrbSolverInterface::globalenv_)
          *messageHandler() << "\t GUROBI error message: " << GRBgeterrormsg(OsiGrbSolverInterface::globalenv_) << CoinMessageEol;
        return false;
      }
      return true;

    case OsiMaxNumIterationHotStart:
    	if( value >= 0 )
    	{
    		hotStartMaxIteration_ = value;
    		return true;
    	}
    	return false;

    case OsiNameDiscipline:
      if( value < 0 || value > 3 )
        return false;
      nameDisc_ = value;
      return true;

    case OsiLastIntParam:
    default:
      return false;
  }
}

//-----------------------------------------------------------------------------

bool
OsiGrbSolverInterface::setDblParam(OsiDblParam key, double value)
{
  debugMessage("OsiGrbSolverInterface::setDblParam(%d, %g)\n", key, value);

  switch (key)
  {
// Gurobi does not have primal or dual objective limits
//  	case OsiDualObjectiveLimit:
//  		break;
//  	case OsiPrimalObjectiveLimit:
//  	  break;
  	case OsiDualTolerance:
  	  if( GRBsetdblparam(GRBgetenv(getMutableLpPtr()), GRB_DBL_PAR_OPTIMALITYTOL, value) != 0 )
      {
        *messageHandler() << "Warning: Setting GUROBI dual (i.e., optimality) tolerance to" << value << "failed." << CoinMessageEol;
        if (OsiGrbSolverInterface::globalenv_)
          *messageHandler() << "\t GUROBI error message: " << GRBgeterrormsg(OsiGrbSolverInterface::globalenv_) << CoinMessageEol;
        return false;
      }
  	  return true;
  	case OsiPrimalTolerance:
  	  if( GRBsetdblparam(GRBgetenv(getMutableLpPtr()), GRB_DBL_PAR_FEASIBILITYTOL, value) != 0 )
      {
        *messageHandler() << "Warning: Setting GUROBI primal (i.e., feasiblity) tolerance to" << value << "failed." << CoinMessageEol;
        if (OsiGrbSolverInterface::globalenv_)
          *messageHandler() << "\t GUROBI error message: " << GRBgeterrormsg(OsiGrbSolverInterface::globalenv_) << CoinMessageEol;
        return false;
      }
  	  return true;
  	case OsiObjOffset:
  		return OsiSolverInterface::setDblParam(key,value);
  	case OsiLastDblParam:
  	default:
  		return false;
  }
}

//-----------------------------------------------------------------------------

bool
OsiGrbSolverInterface::setStrParam(OsiStrParam key, const std::string & value)
{
  debugMessage("OsiGrbSolverInterface::setStrParam(%d, %s)\n", key, value.c_str());

  switch (key) {
    case OsiProbName:
      GUROBI_CALL( "setStrParam", GRBsetstrattr(getLpPtr(), GRB_STR_ATTR_MODELNAME, const_cast<char*>(value.c_str())) );
      return true;

    case OsiSolverName:
    case OsiLastStrParam:
    default:
      return false;
  }
}

// Set a hint parameter
bool
OsiGrbSolverInterface::setHintParam(OsiHintParam key, bool yesNo, OsiHintStrength strength, void* otherInfo)
{
  debugMessage("OsiGrbSolverInterface::setHintParam(%d, %d)\n", key, yesNo);

  if( key == OsiDoScale )
  {
    GUROBI_CALL( "setHintParam", GRBsetintparam(GRBgetenv(getMutableLpPtr()), GRB_INT_PAR_SCALEFLAG, yesNo) );
    OsiSolverInterface::setHintParam(key, yesNo, strength, otherInfo);
    return true;
  }

  return OsiSolverInterface::setHintParam(key, yesNo, strength, otherInfo);
}

//-----------------------------------------------------------------------------

bool
OsiGrbSolverInterface::getIntParam(OsiIntParam key, int& value) const
{
  debugMessage("OsiGrbSolverInterface::getIntParam(%d)\n", key);

  switch (key)
  {
    case OsiMaxNumIteration:
    	double dblval;
      GUROBI_CALL( "getIntParam", GRBupdatemodel(getMutableLpPtr()) );
      GUROBI_CALL( "getIntParam", GRBgetdblparam(GRBgetenv(getMutableLpPtr()), GRB_DBL_PAR_ITERATIONLIMIT, &dblval) );
    	value = (int) dblval;
      return true;

    case OsiMaxNumIterationHotStart:
      value = hotStartMaxIteration_;
      return true;

    case OsiNameDiscipline:
      value = nameDisc_;
      return true;

    case OsiLastIntParam:
    default:
      return false;
  }
}

//-----------------------------------------------------------------------------

bool
OsiGrbSolverInterface::getDblParam(OsiDblParam key, double& value) const
{
  debugMessage("OsiGrbSolverInterface::getDblParam(%d)\n", key);

  GUROBI_CALL( "getDblParam", GRBupdatemodel(getMutableLpPtr()) );

  switch (key) 
  {
// Gurobi does not have primal or dual objective limits
//    case OsiDualObjectiveLimit:
//      break;
//   	case OsiPrimalObjectiveLimit:
//   	  break;
  	case OsiDualTolerance:
  	  GUROBI_CALL( "getDblParam", GRBgetdblparam(GRBgetenv(getMutableLpPtr()), GRB_DBL_PAR_OPTIMALITYTOL, &value) );
  		return true;
  	case OsiPrimalTolerance:
  	  GUROBI_CALL( "getDblParam", GRBgetdblparam(GRBgetenv(getMutableLpPtr()), GRB_DBL_PAR_FEASIBILITYTOL, &value) );
  	  return true;
    case OsiObjOffset:
      return OsiSolverInterface::getDblParam(key, value);
    case OsiLastDblParam:
    default:
      return false;
  }
}

//-----------------------------------------------------------------------------

bool
OsiGrbSolverInterface::getStrParam(OsiStrParam key, std::string & value) const
{
  debugMessage("OsiGrbSolverInterface::getStrParam(%d)\n", key);

  switch (key) {
  	case OsiProbName:
  	{
  	  char* name = NULL;
      GUROBI_CALL( "getStrParam", GRBgetstrattr(getMutableLpPtr(), GRB_STR_ATTR_MODELNAME, &name) );
      assert(name != NULL);
      value = name;
  		return true;
  	}

  	case OsiSolverName:
  		value = "gurobi";
  		return true;

  	case OsiLastStrParam:
  	default:
  		return false;
  }
}

// Get a hint parameter
bool
OsiGrbSolverInterface::getHintParam(OsiHintParam key, bool& yesNo, OsiHintStrength& strength, void*& otherInformation) const
{
  debugMessage("OsiGrbSolverInterface::getHintParam[1](%d)\n", key);
  if( key == OsiDoScale )
  {
    OsiSolverInterface::getHintParam(key, yesNo, strength, otherInformation);
    GUROBI_CALL( "getHintParam", GRBupdatemodel(getMutableLpPtr()) );
    int value;
    GUROBI_CALL( "getHintParam", GRBgetintparam(GRBgetenv(getMutableLpPtr()), GRB_INT_PAR_SCALEFLAG, &value) );
    yesNo = value;
    return true;
  }

  return OsiSolverInterface::getHintParam(key, yesNo, strength, otherInformation);
}

// Get a hint parameter
bool
OsiGrbSolverInterface::getHintParam(OsiHintParam key, bool& yesNo, OsiHintStrength& strength) const
{
  debugMessage("OsiGrbSolverInterface::getHintParam[2](%d)\n", key);
  if( key == OsiDoScale )
  {
    OsiSolverInterface::getHintParam(key, yesNo, strength);
    GUROBI_CALL( "getHintParam", GRBupdatemodel(getMutableLpPtr()) );
    int value;
    GUROBI_CALL( "getHintParam", GRBgetintparam(GRBgetenv(getMutableLpPtr()), GRB_INT_PAR_SCALEFLAG, &value) );
    yesNo = value;
    return true;
  }

  return OsiSolverInterface::getHintParam(key, yesNo, strength);
}

// Get a hint parameter
bool
OsiGrbSolverInterface::getHintParam(OsiHintParam key, bool& yesNo) const
{
  debugMessage("OsiGrbSolverInterface::getHintParam[3](%d)\n", key);
  if( key == OsiDoScale )
  {
    OsiSolverInterface::getHintParam(key, yesNo);
    GUROBI_CALL( "getHintParam", GRBupdatemodel(getMutableLpPtr()) );
    int value;
    GUROBI_CALL( "getHintParam", GRBgetintparam(GRBgetenv(getMutableLpPtr()), GRB_INT_PAR_SCALEFLAG, &value) );
    yesNo = value;
    return true;
  }

  return OsiSolverInterface::getHintParam(key, yesNo);
}

//#############################################################################
// Methods returning info on how the solution process terminated
//#############################################################################

bool OsiGrbSolverInterface::isAbandoned() const
{
  debugMessage("OsiGrbSolverInterface::isAbandoned()\n");

  GUROBI_CALL( "isAbandoned", GRBupdatemodel(getMutableLpPtr()) );

  int stat;
  GUROBI_CALL( "isAbandoned", GRBgetintattr(getMutableLpPtr(), GRB_INT_ATTR_STATUS, &stat) );
	
	return (
			stat == GRB_LOADED ||
			stat == GRB_NUMERIC || 
			stat == GRB_INTERRUPTED
		);
}

bool OsiGrbSolverInterface::isProvenOptimal() const
{
  debugMessage("OsiGrbSolverInterface::isProvenOptimal()\n");

  GUROBI_CALL( "isProvenOptimal", GRBupdatemodel(getMutableLpPtr()) );

  int stat;
  GUROBI_CALL( "isProvenOptimal", GRBgetintattr(getMutableLpPtr(), GRB_INT_ATTR_STATUS, &stat) );

	return (stat == GRB_OPTIMAL);
}

bool OsiGrbSolverInterface::isProvenPrimalInfeasible() const
{
  debugMessage("OsiGrbSolverInterface::isProvenPrimalInfeasible()\n");

  GUROBI_CALL( "isProvenPrimalInfeasible", GRBupdatemodel(getMutableLpPtr()) );

  int stat;
  GUROBI_CALL( "isProvenPrimalInfeasible", GRBgetintattr(getMutableLpPtr(), GRB_INT_ATTR_STATUS, &stat) );

	return (stat == GRB_INFEASIBLE);
}

bool OsiGrbSolverInterface::isProvenDualInfeasible() const
{
  debugMessage("OsiGrbSolverInterface::isProvenDualInfeasible()\n");

  GUROBI_CALL( "isProvenDualInfeasible", GRBupdatemodel(getMutableLpPtr()) );

  int stat;
  GUROBI_CALL( "isProvenDualInfeasible", GRBgetintattr(getMutableLpPtr(), GRB_INT_ATTR_STATUS, &stat) );

	return (stat == GRB_UNBOUNDED);
}

bool OsiGrbSolverInterface::isPrimalObjectiveLimitReached() const
{
  debugMessage("OsiGrbSolverInterface::isPrimalObjectiveLimitReached()\n");

  return false;
}

bool OsiGrbSolverInterface::isDualObjectiveLimitReached() const
{
  debugMessage("OsiGrbSolverInterface::isDualObjectiveLimitReached()\n");

  return false;
}

bool OsiGrbSolverInterface::isIterationLimitReached() const
{
  debugMessage("OsiGrbSolverInterface::isIterationLimitReached()\n");

  GUROBI_CALL( "isIterationLimitReached", GRBupdatemodel(getMutableLpPtr()) );

  int stat;
  GUROBI_CALL( "isIterationLimitReached", GRBgetintattr(getMutableLpPtr(), GRB_INT_ATTR_STATUS, &stat) );

	return (stat == GRB_ITERATION_LIMIT);
}

//#############################################################################
// WarmStart related methods
//#############################################################################

CoinWarmStart* OsiGrbSolverInterface::getEmptyWarmStart () const
{ return (dynamic_cast<CoinWarmStart *>(new CoinWarmStartBasis())) ; }

// below we assume that getBasisStatus uses the same values as the warm start enum, i.e. 0: free, 1: basic, 2: upper, 3: lower
static CompileTimeAssert<CoinWarmStartBasis::isFree       == 0> UNUSED change_In_Enum_CoinWarmStartBasis_Status_free_breaks_this_function;
static CompileTimeAssert<CoinWarmStartBasis::basic        == 1> UNUSED change_In_Enum_CoinWarmStartBasis_Status_basic_breaks_this_function;
static CompileTimeAssert<CoinWarmStartBasis::atUpperBound == 2> UNUSED change_In_Enum_CoinWarmStartBasis_Status_upper_breaks_this_function;
static CompileTimeAssert<CoinWarmStartBasis::atLowerBound == 3> UNUSED change_In_Enum_CoinWarmStartBasis_Status_lower_breaks_this_function;

CoinWarmStart* OsiGrbSolverInterface::getWarmStart() const
{
  debugMessage("OsiGrbSolverInterface::getWarmStart()\n");

  assert(!probtypemip_);

  if( !basisIsAvailable() )
    return NULL;

  CoinWarmStartBasis* ws = NULL;
  int numcols = getNumCols();
  int numrows = getNumRows();
  int *cstat = new int[numcols];
  int *rstat = new int[numrows];
  int i;
  
#if 1
  getBasisStatus(cstat, rstat);

  ws = new CoinWarmStartBasis;
  ws->setSize( numcols, numrows );

  for( i = 0; i < numrows; ++i )
    ws->setArtifStatus( i, CoinWarmStartBasis::Status(rstat[i]) );
  for( i = 0; i < numcols; ++i )
    ws->setStructStatus( i, CoinWarmStartBasis::Status(cstat[i]) );

#else
  GUROBI_CALL( "getWarmStart", GRBupdatemodel(getMutableLpPtr()) );

  GUROBI_CALL( "getWarmStart", GRBgetintattrarray(getMutableLpPtr(), GRB_INT_ATTR_VBASIS, 0, numcols, cstat) );
  GUROBI_CALL( "getWarmStart", GRBgetintattrarray(getMutableLpPtr(), GRB_INT_ATTR_CBASIS, 0, numrows, rstat) );
  
	ws = new CoinWarmStartBasis;
	ws->setSize( numcols, numrows );

	char sense;
	for( i = 0; i < numrows; ++i )
	{
	  switch( rstat[i] )
	  {
	  	case GRB_BASIC:
	  		ws->setArtifStatus( i, CoinWarmStartBasis::basic );
	  		break;
	  	case GRB_NONBASIC_LOWER:
	  	case GRB_NONBASIC_UPPER:
	  	  GUROBI_CALL( "getWarmStart", GRBgetcharattrelement(getMutableLpPtr(), GRB_CHAR_ATTR_SENSE, i, &sense) );
	  		ws->setArtifStatus( i, (sense == '>' ? CoinWarmStartBasis::atUpperBound : CoinWarmStartBasis::atLowerBound) );
	  		break;
	  	default:  // unknown row status
	  		delete ws;
	  		delete[] rstat;
	  		delete[] cstat;
	  		return NULL;
	  }
	}
	
	for( i = 0; i < numcols; ++i )
	{
		switch( cstat[i] )
		{
			case GRB_BASIC:
				ws->setStructStatus( i, CoinWarmStartBasis::basic );
				break;
			case GRB_NONBASIC_LOWER:
				ws->setStructStatus( i, CoinWarmStartBasis::atLowerBound );
				break;
			case GRB_NONBASIC_UPPER:
				ws->setStructStatus( i, CoinWarmStartBasis::atUpperBound );
				break;
			case GRB_SUPERBASIC:
				ws->setStructStatus( i, CoinWarmStartBasis::isFree );
				break;
			default:  // unknown column status
				delete[] rstat;
				delete[] cstat;
				delete ws;
				return NULL;
		}
	}
#endif

	delete[] cstat;
  delete[] rstat;

  return ws;
}

//-----------------------------------------------------------------------------

bool OsiGrbSolverInterface::setWarmStart(const CoinWarmStart* warmstart)
{
  debugMessage("OsiGrbSolverInterface::setWarmStart(%p)\n", (void*)warmstart);

  const CoinWarmStartBasis* ws = dynamic_cast<const CoinWarmStartBasis*>(warmstart);
  int numcols, numrows, i, oi;
  int *stat;

  if( !ws )
    return false;

  numcols = ws->getNumStructural();
  numrows = ws->getNumArtificial();
  
  if( numcols != getNumCols() || numrows != getNumRows() )
    return false;

  switchToLP();

  stat = new int[numcols + nauxcols > numrows ? numcols + nauxcols : numrows];
  for( i = 0; i < numrows; ++i )
  {
  	switch( ws->getArtifStatus( i ) )
  	{
  		case CoinWarmStartBasis::basic:
  			stat[i] = GRB_BASIC;
  			break;
  		case CoinWarmStartBasis::atLowerBound:
  		case CoinWarmStartBasis::atUpperBound:
  			stat[i] = GRB_NONBASIC_LOWER;
  			break;
      case CoinWarmStartBasis::isFree:
         stat[i] = GRB_SUPERBASIC;
         break;
  		default:  // unknown row status
  			delete[] stat;
  			return false;
  	}
  }
  
  GUROBI_CALL( "setWarmStart", GRBsetintattrarray(getLpPtr(OsiGrbSolverInterface::FREECACHED_RESULTS), GRB_INT_ATTR_CBASIS, 0, numrows, stat) );

  for( i = 0; i < numcols + nauxcols; ++i )
  {
    oi = colmap_G2O ? colmap_G2O[i] : i;
    if( oi >= 0 )
    { /* normal variable */
      switch( ws->getStructStatus( oi ) )
      {
        case CoinWarmStartBasis::basic:
          stat[i] = GRB_BASIC;
          break;
        case CoinWarmStartBasis::atLowerBound:
          stat[i] = GRB_NONBASIC_LOWER;
          break;
        case CoinWarmStartBasis::atUpperBound:
          stat[i] = GRB_NONBASIC_UPPER;
          break;
        case CoinWarmStartBasis::isFree:
          stat[i] = GRB_SUPERBASIC;
          break;
        default:  // unknown col status
          delete[] stat;
          return false;
      }
    }
    else
    { /* auxiliary variable, derive status from status of corresponding ranged row */
      switch( ws->getArtifStatus( -oi-1 ) )
      {
        case CoinWarmStartBasis::basic:
          stat[i] = GRB_BASIC;
          break;
        case CoinWarmStartBasis::atLowerBound:
          stat[i] = GRB_NONBASIC_LOWER;
          break;
        case CoinWarmStartBasis::atUpperBound:
          stat[i] = GRB_NONBASIC_UPPER;
          break;
        case CoinWarmStartBasis::isFree:
           stat[i] = GRB_SUPERBASIC;
           break;
        default:  // unknown col status
          delete[] stat;
          return false;
      }
    }
  }
  
  GUROBI_CALL( "setWarmStart", GRBsetintattrarray(getLpPtr(OsiGrbSolverInterface::FREECACHED_RESULTS), GRB_INT_ATTR_VBASIS, 0, numcols+nauxcols, stat) );

  delete[] stat;
  return true;
}

//#############################################################################
// Hotstart related methods (primarily used in strong branching)
//#############################################################################

void OsiGrbSolverInterface::markHotStart()
{
  debugMessage("OsiGrbSolverInterface::markHotStart()\n");

  int numcols, numrows;

  assert(!probtypemip_);

  numcols = getNumCols() + nauxcols;
  numrows = getNumRows();
  if( numcols > hotStartCStatSize_ )
  {
  	delete[] hotStartCStat_;
  	hotStartCStatSize_ = static_cast<int>( 1.2 * static_cast<double>( numcols + nauxcols ) ); // get some extra space for future hot starts
  	hotStartCStat_ = new int[hotStartCStatSize_];
  }
  if( numrows > hotStartRStatSize_ )
  {
  	delete[] hotStartRStat_;
  	hotStartRStatSize_ = static_cast<int>( 1.2 * static_cast<double>( numrows ) ); // get some extra space for future hot starts
  	hotStartRStat_ = new int[hotStartRStatSize_];
  }

  GUROBI_CALL( "markHotStart", GRBupdatemodel(getMutableLpPtr()) );

  GUROBI_CALL( "markHotStart", GRBgetintattrarray(getMutableLpPtr(), GRB_INT_ATTR_VBASIS, 0, numcols + nauxcols, hotStartCStat_) );
  GUROBI_CALL( "markHotStart", GRBgetintattrarray(getMutableLpPtr(), GRB_INT_ATTR_CBASIS, 0, numrows, hotStartRStat_) );
}

void OsiGrbSolverInterface::solveFromHotStart()
{
  debugMessage("OsiGrbSolverInterface::solveFromHotStart()\n");

  double maxiter;

  switchToLP();

  assert( getNumCols() <= hotStartCStatSize_ );
  assert( getNumRows() <= hotStartRStatSize_ );

  GUROBI_CALL( "solveFromHotStart", GRBupdatemodel(getMutableLpPtr()) );

  GUROBI_CALL( "solveFromHotStart", GRBsetintattrarray(getLpPtr(OsiGrbSolverInterface::FREECACHED_RESULTS), GRB_INT_ATTR_CBASIS, 0, getNumRows(), hotStartRStat_ ) );
  GUROBI_CALL( "solveFromHotStart", GRBsetintattrarray(getLpPtr(OsiGrbSolverInterface::FREECACHED_RESULTS), GRB_INT_ATTR_VBASIS, 0, getNumCols() + nauxcols, hotStartCStat_ ) );

  GUROBI_CALL( "solveFromHotStart", GRBgetdblparam(GRBgetenv(getMutableLpPtr()), GRB_DBL_PAR_ITERATIONLIMIT, &maxiter) );
  GUROBI_CALL( "solveFromHotStart", GRBsetdblparam(GRBgetenv(getMutableLpPtr()), GRB_DBL_PAR_ITERATIONLIMIT, (double)hotStartMaxIteration_) );
  
  resolve();

  GUROBI_CALL( "solveFromHotStart", GRBsetdblparam(GRBgetenv(getMutableLpPtr()), GRB_DBL_PAR_ITERATIONLIMIT, maxiter) );
}

void OsiGrbSolverInterface::unmarkHotStart()
{
  debugMessage("OsiGrbSolverInterface::unmarkHotStart()\n");

  // ??? be lazy with deallocating memory and do nothing here, deallocate memory in the destructor
}

//#############################################################################
// Problem information methods (original data)
//#############################################################################

//------------------------------------------------------------------
// Get number of rows, columns, elements, ...
//------------------------------------------------------------------
int OsiGrbSolverInterface::getNumCols() const
{
  debugMessage("OsiGrbSolverInterface::getNumCols()\n");
  
  int numcols;

  GUROBI_CALL( "getNumCols", GRBupdatemodel(getMutableLpPtr()) );

  GUROBI_CALL( "getNumCols", GRBgetintattr(getMutableLpPtr(), GRB_INT_ATTR_NUMVARS, &numcols) );

  numcols -= nauxcols;

  return numcols;
}

int OsiGrbSolverInterface::getNumRows() const
{
  debugMessage("OsiGrbSolverInterface::getNumRows()\n");

  int numrows;

  GUROBI_CALL( "getNumRows", GRBupdatemodel(getMutableLpPtr()) );

  GUROBI_CALL( "getNumRows", GRBgetintattr(getMutableLpPtr(), GRB_INT_ATTR_NUMCONSTRS, &numrows) );

  return numrows;
}

int OsiGrbSolverInterface::getNumElements() const
{
  debugMessage("OsiGrbSolverInterface::getNumElements()\n");

  int numnz;

  GUROBI_CALL( "getNumElements", GRBupdatemodel(getMutableLpPtr()) );

  GUROBI_CALL( "getNumElements", GRBgetintattr(getMutableLpPtr(), GRB_INT_ATTR_NUMNZS, &numnz) );

  /* each auxiliary column contributes one nonzero element to exactly one row */
  numnz -= nauxcols;

  return numnz;
}

//------------------------------------------------------------------
// Get pointer to rim vectors
//------------------------------------------------------------------  

const double * OsiGrbSolverInterface::getColLower() const
{
  debugMessage("OsiGrbSolverInterface::getColLower()\n");

  if( collower_ == NULL )
  {
  	int ncols = getNumCols();
  	if( ncols > 0 )
  	{
  		collower_ = new double[ncols];
  		GUROBI_CALL( "getColLower", GRBupdatemodel(getMutableLpPtr()) );

  		if( nauxcols )
        GUROBI_CALL( "getColLower", GRBgetdblattrlist(getMutableLpPtr(), GRB_DBL_ATTR_LB, ncols, colmap_O2G, collower_) );
  		else
  		  GUROBI_CALL( "getColLower", GRBgetdblattrarray(getMutableLpPtr(), GRB_DBL_ATTR_LB, 0, ncols, collower_) );
  	}
  }
  
  return collower_;
}

//------------------------------------------------------------------
const double * OsiGrbSolverInterface::getColUpper() const
{
  debugMessage("OsiGrbSolverInterface::getColUpper()\n");

  if( colupper_ == NULL )
  {
  	int ncols = getNumCols();
  	if( ncols > 0 )
  	{
  		colupper_ = new double[ncols];
  		GUROBI_CALL( "getColUpper", GRBupdatemodel(getMutableLpPtr()) );

  		if( nauxcols )
        GUROBI_CALL( "getColUpper", GRBgetdblattrlist(getMutableLpPtr(), GRB_DBL_ATTR_UB, ncols, colmap_O2G, colupper_) );
  		else
  		  GUROBI_CALL( "getColUpper", GRBgetdblattrarray(getMutableLpPtr(), GRB_DBL_ATTR_UB, 0, ncols, colupper_) );
  	}
  }
  
  return colupper_;
}

//------------------------------------------------------------------
const char * OsiGrbSolverInterface::getRowSense() const
{
  debugMessage("OsiGrbSolverInterface::getRowSense()\n");

  if ( rowsense_==NULL )
  {
  	int nrows = getNumRows();
  	if( nrows > 0 )
  	{
  		rowsense_ = new char[nrows];
  		GUROBI_CALL( "getRowSense", GRBupdatemodel(getMutableLpPtr()) );

  		GUROBI_CALL( "getRowSense", GRBgetcharattrarray(getMutableLpPtr(), GRB_CHAR_ATTR_SENSE, 0, nrows, rowsense_) );
  	  
  	  for (int i = 0; i < nrows; ++i)
  	  {
  	    if( auxcolind && auxcolind[i] >= 0 )
  	    {
  	      assert(rowsense_[i] == GRB_EQUAL);
          rowsense_[i] = 'R';
          continue;
  	    }
  	  	switch (rowsense_[i])
  	  	{
  	  		case GRB_LESS_EQUAL:
  	  			rowsense_[i] = 'L';
  	  			break;
  	  		case GRB_GREATER_EQUAL:
  	  			rowsense_[i] = 'G';
  	  			break;
  	  		case GRB_EQUAL:
  	  			rowsense_[i] = 'E';
  	  			break;
  	  	}
  	  }
  	}
  }
  
  return rowsense_;
}

//------------------------------------------------------------------
const double * OsiGrbSolverInterface::getRightHandSide() const
{
  debugMessage("OsiGrbSolverInterface::getRightHandSide()\n");

  if ( rhs_ == NULL )
  {
  	int nrows = getNumRows();
  	if( nrows > 0 )
  	{
  		rhs_ = new double[nrows];
  		GUROBI_CALL( "getRightHandSide", GRBupdatemodel(getMutableLpPtr()) );

  		GUROBI_CALL( "getRightHandSide", GRBgetdblattrarray(getMutableLpPtr(), GRB_DBL_ATTR_RHS, 0, nrows, rhs_) );

  		/* for ranged rows, we give the rhs of the aux. variable used to represent the range of this row as row rhs */
  		if( nauxcols )
  		  for( int i = 0; i < nrows; ++i )
  		    if( auxcolind[i] >= 0 )
  		      GUROBI_CALL( "getRightHandSide", GRBgetdblattrelement(getMutableLpPtr(), GRB_DBL_ATTR_UB, auxcolind[i], &rhs_[i]) );
  	}
  }
  
  return rhs_;
}

//------------------------------------------------------------------
const double * OsiGrbSolverInterface::getRowRange() const
{
  debugMessage("OsiGrbSolverInterface::getRowRange()\n");

  if ( rowrange_==NULL ) 
  {
  	int nrows = getNumRows();
  	if (nrows > 0)
    	rowrange_ = CoinCopyOfArrayOrZero((double*)NULL, nrows);

    if( nauxcols )
    {
      double auxlb, auxub;
      for( int i = 0; i < nrows; ++i )
        if( auxcolind[i] >= 0 )
        {
          GUROBI_CALL( "getRowRange", GRBgetdblattrelement(getMutableLpPtr(), GRB_DBL_ATTR_LB, auxcolind[i], &auxlb) );
          GUROBI_CALL( "getRowRange", GRBgetdblattrelement(getMutableLpPtr(), GRB_DBL_ATTR_UB, auxcolind[i], &auxub) );
          rowrange_[i] = auxub - auxlb;
        }
    }
  }
  
  return rowrange_;
}

//------------------------------------------------------------------
const double * OsiGrbSolverInterface::getRowLower() const
{
  debugMessage("OsiGrbSolverInterface::getRowLower()\n");

  if ( rowlower_ == NULL )
  {
  	int nrows = getNumRows();
  	if ( nrows > 0 )
  	{
    	const char* rowsense = getRowSense();
    	
  		rowlower_ = new double[nrows];

      double rhs;
  		double dum1;
  		for ( int i = 0; i < nrows; ++i )
  		{
  		  if( auxcolind && auxcolind[i] >= 0 )
  		  {
  		    assert(rowsense[i] == 'R');
          GUROBI_CALL( "getRowLower", GRBgetdblattrelement(getMutableLpPtr(), GRB_DBL_ATTR_LB, auxcolind[i], &rowlower_[i]) );
  		  }
  		  else
  		  {
          GUROBI_CALL( "getRowLower", GRBgetdblattrelement(getMutableLpPtr(), GRB_DBL_ATTR_RHS, i, &rhs) );
          convertSenseToBound( rowsense[i], rhs, 0.0, rowlower_[i], dum1 );
  		  }
  		}
  	}
  }
  
  return rowlower_;
}

//------------------------------------------------------------------
const double * OsiGrbSolverInterface::getRowUpper() const
{  
  debugMessage("OsiGrbSolverInterface::getRowUpper()\n");

  if ( rowupper_ == NULL )
  {
  	int nrows = getNumRows();
  	if ( nrows > 0 ) 
  	{
  	  const char* rowsense = getRowSense();

      rowupper_ = new double[nrows];

      double rhs;
      double dum1;
      for ( int i = 0; i < nrows; ++i )
      {
        if( auxcolind && auxcolind[i] >= 0 )
        {
          assert(rowsense[i] == 'R');
          GUROBI_CALL( "getRowUpper", GRBgetdblattrelement(getMutableLpPtr(), GRB_DBL_ATTR_UB, auxcolind[i], &rowupper_[i]) );
        }
        else
        {
          GUROBI_CALL( "getRowUpper", GRBgetdblattrelement(getMutableLpPtr(), GRB_DBL_ATTR_RHS, i, &rhs) );
          convertSenseToBound( rowsense[i], rhs, 0.0, dum1, rowupper_[i] );
        }
      }
  	}
  }
  
  return rowupper_;
}

//------------------------------------------------------------------
const double * OsiGrbSolverInterface::getObjCoefficients() const
{
  debugMessage("OsiGrbSolverInterface::getObjCoefficients()\n");

  if ( obj_==NULL )
  {
  	int ncols = getNumCols();
  	if( ncols > 0 )
  	{
  		obj_ = new double[ncols];
  		GUROBI_CALL( "getObjCoefficients", GRBupdatemodel(getMutableLpPtr()) );

    if( nauxcols )
      GUROBI_CALL( "getObjCoefficients", GRBgetdblattrlist(getMutableLpPtr(), GRB_DBL_ATTR_OBJ, ncols, colmap_O2G, obj_) );
    else
      GUROBI_CALL( "getObjCoefficients", GRBgetdblattrarray(getMutableLpPtr(), GRB_DBL_ATTR_OBJ, 0, ncols, obj_) );
  	}
  }
  
  return obj_;
}

//------------------------------------------------------------------
double OsiGrbSolverInterface::getObjSense() const
{
  debugMessage("OsiGrbSolverInterface::getObjSense()\n");

  int sense;
  GUROBI_CALL( "getObjSense", GRBupdatemodel(getMutableLpPtr()) );

  GUROBI_CALL( "getObjSense", GRBgetintattr(getMutableLpPtr(), GRB_INT_ATTR_MODELSENSE, &sense) );
 
  return (double)sense;
}

//------------------------------------------------------------------
// Return information on integrality
//------------------------------------------------------------------

bool OsiGrbSolverInterface::isContinuous( int colNumber ) const
{
  debugMessage("OsiGrbSolverInterface::isContinuous(%d)\n", colNumber);

  return getCtype()[colNumber] == 'C';
}

//------------------------------------------------------------------
// Row and column copies of the matrix ...
//------------------------------------------------------------------

const CoinPackedMatrix * OsiGrbSolverInterface::getMatrixByRow() const
{
  debugMessage("OsiGrbSolverInterface::getMatrixByRow()\n");

  if ( matrixByRow_ == NULL ) 
  {
  	int nrows = getNumRows();
  	int ncols = getNumCols();
  	
    if ( nrows == 0 ) {
    	matrixByRow_ = new CoinPackedMatrix();
    	matrixByRow_->setDimensions(0, ncols);
    	return matrixByRow_;
    }

  	int nelems;
  	int *starts   = new int   [nrows + 1];
  	int *len      = new int   [nrows];

  	GUROBI_CALL( "getMatrixByRow", GRBupdatemodel(getMutableLpPtr()) );

  	GUROBI_CALL( "getMatrixByRow", GRBgetconstrs(getMutableLpPtr(), &nelems, NULL, NULL, NULL, 0, nrows) );

  	assert( nelems == getNumElements() + nauxcols );
  	int     *indices  = new int   [nelems];
  	double  *elements = new double[nelems]; 

  	GUROBI_CALL( "getMatrixByRow", GRBgetconstrs(getMutableLpPtr(), &nelems, starts, indices, elements, 0, nrows) );

  	matrixByRow_ = new CoinPackedMatrix();

  	// Should be able to pass null for length of packed matrix,
  	// assignMatrix does not seem to allow (even though documentation say it is possible to do this).
  	// For now compute the length.
  	starts[nrows] = nelems;
  	for ( int i = 0; i < nrows; ++i )
  		len[i] = starts[i+1] - starts[i];

  	matrixByRow_->assignMatrix( false /* not column ordered */,
  			ncols + nauxcols, nrows, nelems,
  			elements, indices, starts, len);

  	if( nauxcols )
  	{
  	  // delete auxiliary columns from matrix
  	  int* auxcols = new int[nauxcols];
  	  int j = 0;
  	  for( int i = 0; i < ncols + nauxcols; ++i )
  	    if( colmap_G2O[i] < 0 )
  	      auxcols[j++] = i;
  	  assert(j == nauxcols);

  	  matrixByRow_->deleteCols(nauxcols, auxcols);

  	  delete[] auxcols;

  	  assert(matrixByRow_->getNumElements() == getNumElements());
  	  assert(matrixByRow_->getMinorDim() <= ncols);
  	}
  }
  
  return matrixByRow_;
}

//------------------------------------------------------------------

const CoinPackedMatrix * OsiGrbSolverInterface::getMatrixByCol() const
{
	debugMessage("OsiGrbSolverInterface::getMatrixByCol()\n");

	if ( matrixByCol_ == NULL )
	{
		int nrows = getNumRows();
		int ncols = getNumCols();

    matrixByCol_ = new CoinPackedMatrix();

		if ( ncols > 0 )
		{
		  int nelems;
		  int *starts = new int   [ncols + nauxcols + 1];
		  int *len    = new int   [ncols + nauxcols];

		  GUROBI_CALL( "getMatrixByCol", GRBupdatemodel(getMutableLpPtr()) );

		  GUROBI_CALL( "getMatrixByCol", GRBgetvars(getMutableLpPtr(), &nelems, NULL, NULL, NULL, 0, ncols + nauxcols) );

		  int     *indices  = new int   [nelems];
		  double  *elements = new double[nelems];

		  GUROBI_CALL( "getMatrixByCol", GRBgetvars(getMutableLpPtr(), &nelems, starts, indices, elements, 0, ncols + nauxcols) );

		  // Should be able to pass null for length of packed matrix,
		  // assignMatrix does not seem to allow (even though documentation say it is possible to do this).
		  // For now compute the length.
		  starts[ncols + nauxcols] = nelems;
		  for ( int i = 0; i < ncols + nauxcols; i++ )
		    len[i] = starts[i+1] - starts[i];

		  matrixByCol_->assignMatrix( true /* column ordered */,
		      nrows, ncols + nauxcols, nelems,
		      elements, indices, starts, len);

		  assert( matrixByCol_->getNumCols() == ncols + nauxcols );
		  assert( matrixByCol_->getNumRows() == nrows );

	    if( nauxcols )
	    {
	      // delete auxiliary columns from matrix
	      int* auxcols = new int[nauxcols];
	      int j = 0;
	      for( int i = 0; i < ncols + nauxcols; ++i )
	        if( colmap_G2O[i] < 0 )
	          auxcols[j++] = i;
	      assert(j == nauxcols);

	      matrixByCol_->deleteCols(nauxcols, auxcols);

	      delete[] auxcols;

	      assert(matrixByCol_->getNumElements() == getNumElements());
        assert(matrixByCol_->getMajorDim() <= ncols);
	      assert(matrixByCol_->getMinorDim() <= nrows);
	    }
		}
		else
		  matrixByCol_->setDimensions(nrows, ncols);
	}
	
	return matrixByCol_;
} 

//------------------------------------------------------------------
// Get solver's value for infinity
//------------------------------------------------------------------
double OsiGrbSolverInterface::getInfinity() const
{
  debugMessage("OsiGrbSolverInterface::getInfinity()\n");

  return GRB_INFINITY;
}

//#############################################################################
// Problem information methods (results)
//#############################################################################

// *FIXME*: what should be done if a certain vector doesn't exist???

const double * OsiGrbSolverInterface::getColSolution() const
{
	debugMessage("OsiGrbSolverInterface::getColSolution()\n");

	if( colsol_ == NULL )
	{
		int ncols = getNumCols();
		if( ncols > 0 )
		{
			colsol_ = new double[ncols];

			GUROBI_CALL( "getColSolution", GRBupdatemodel(getMutableLpPtr()) );

      if ( GRBgetdblattrelement(getMutableLpPtr(), GRB_DBL_ATTR_X, 0, colsol_) == 0 )
      { // if a solution is available, get it
        if( nauxcols )
          GUROBI_CALL( "getColSolution", GRBgetdblattrlist(getMutableLpPtr(), GRB_DBL_ATTR_X, ncols, colmap_O2G, colsol_) );
        else
          GUROBI_CALL( "getColSolution", GRBgetdblattrarray(getMutableLpPtr(), GRB_DBL_ATTR_X, 0, ncols, colsol_) );
      }
      else
      {
        *messageHandler() << "Warning: OsiGrbSolverInterface::getColSolution() called, but no solution available! Returning lower bounds, but they may be at -infinity. Be aware!" << CoinMessageEol;
        if (OsiGrbSolverInterface::globalenv_)
          *messageHandler() << "\t GUROBI error message: " << GRBgeterrormsg(OsiGrbSolverInterface::globalenv_) << CoinMessageEol;
        if( nauxcols )
          GUROBI_CALL( "getColSolution", GRBgetdblattrlist(getMutableLpPtr(), GRB_DBL_ATTR_LB, ncols, colmap_O2G, colsol_) );
        else
          GUROBI_CALL( "getColSolution", GRBgetdblattrarray(getMutableLpPtr(), GRB_DBL_ATTR_LB, 0, ncols, colsol_) );
      }
		}
	}
	
	return colsol_;
}

//------------------------------------------------------------------
const double * OsiGrbSolverInterface::getRowPrice() const
{
  debugMessage("OsiGrbSolverInterface::getRowPrice()\n");

  if( rowsol_==NULL )
  {
  	int nrows = getNumRows();
  	if( nrows > 0 )
  	{
  		rowsol_ = new double[nrows];
  		
  		GUROBI_CALL( "getRowPrice", GRBupdatemodel(getMutableLpPtr()) );
  	  
      if ( GRBgetdblattrelement(getMutableLpPtr(), GRB_DBL_ATTR_PI, 0, rowsol_) == 0 )
      {
    		GUROBI_CALL( "getRowPrice", GRBgetdblattrarray(getMutableLpPtr(), GRB_DBL_ATTR_PI, 0, nrows, rowsol_) );
      }
      else
      {
        *messageHandler() << "Warning: OsiGrbSolverInterface::getRowPrice() called, but no solution available! Returning 0.0. Be aware!" << CoinMessageEol;
        if (OsiGrbSolverInterface::globalenv_)
          *messageHandler() << "\t GUROBI error message: " << GRBgeterrormsg(OsiGrbSolverInterface::globalenv_) << CoinMessageEol;
        CoinZeroN(rowsol_, nrows);
      }

  		//??? what is the dual value for a ranged row?
  	}
  }
  
  return rowsol_;
}

//------------------------------------------------------------------
const double * OsiGrbSolverInterface::getReducedCost() const
{
  debugMessage("OsiGrbSolverInterface::getReducedCost()\n");

  if( redcost_==NULL )
  {
  	int ncols = getNumCols();
  	if( ncols > 0 )
  	{
  		redcost_ = new double[ncols];

  		GUROBI_CALL( "getReducedCost", GRBupdatemodel(getMutableLpPtr()) );

      if ( GRBgetdblattrelement(getMutableLpPtr(), GRB_DBL_ATTR_RC, 0, redcost_) == 0 )
      { // if reduced costs are available, get them
    		if( nauxcols )
    		  GUROBI_CALL( "getReducedCost", GRBgetdblattrlist(getMutableLpPtr(), GRB_DBL_ATTR_RC, ncols, colmap_O2G, redcost_) );
    		else
          GUROBI_CALL( "getReducedCost", GRBgetdblattrarray(getMutableLpPtr(), GRB_DBL_ATTR_RC, 0, ncols, redcost_) );
      }
      else
      {
        *messageHandler() << "Warning: OsiGrbSolverInterface::getReducedCost() called, but no solution available! Returning 0.0. Be aware!" << CoinMessageEol;
        if (OsiGrbSolverInterface::globalenv_)
          *messageHandler() << "\t GUROBI error message: " << GRBgeterrormsg(OsiGrbSolverInterface::globalenv_) << CoinMessageEol;
        CoinZeroN(redcost_, ncols);
      }
  	}
  }
  
  return redcost_;
}

//------------------------------------------------------------------
const double * OsiGrbSolverInterface::getRowActivity() const
{
  debugMessage("OsiGrbSolverInterface::getRowActivity()\n");

  if( rowact_==NULL )
  {
  	int nrows = getNumRows();
  	if( nrows > 0 )
  	{
  		rowact_ = new double[nrows];

  		GUROBI_CALL( "getRowActivity", GRBupdatemodel(getMutableLpPtr()) );

  		if ( GRBgetdblattrelement(getMutableLpPtr(), GRB_DBL_ATTR_SLACK, 0, rowact_) == 0 )
  		{
  			GUROBI_CALL( "getRowActivity", GRBgetdblattrarray(getMutableLpPtr(), GRB_DBL_ATTR_SLACK, 0, nrows, rowact_) );

  			for( int r = 0; r < nrows; ++r )
  			{
  				if( nauxcols && auxcolind[r] >= 0 )
  				{
  					GUROBI_CALL( "getRowActivity", GRBgetdblattrelement(getMutableLpPtr(), GRB_DBL_ATTR_X, auxcolind[r], &rowact_[r]) );
  				}
  				else
  					rowact_[r] = getRightHandSide()[r] - rowact_[r];
  			}
  		}
      else
      {
        *messageHandler() << "Warning: OsiGrbSolverInterface::getRowActivity() called, but no solution available! Returning 0.0. Be aware!" << CoinMessageEol;
        if (OsiGrbSolverInterface::globalenv_)
          *messageHandler() << "\t GUROBI error message: " << GRBgeterrormsg(OsiGrbSolverInterface::globalenv_) << CoinMessageEol;
        CoinZeroN(rowact_, nrows);
      }
  	}
  }
  return rowact_;
}

//------------------------------------------------------------------
double OsiGrbSolverInterface::getObjValue() const
{
  debugMessage("OsiGrbSolverInterface::getObjValue()\n");

  double objval = 0.0;
 
  GUROBI_CALL( "getObjValue", GRBupdatemodel(getMutableLpPtr()) );

  if( GRBgetdblattr(getMutableLpPtr(), GRB_DBL_ATTR_OBJVAL, &objval) == 0 )
  {
    // Adjust objective function value by constant term in objective function
    double objOffset;
    getDblParam(OsiObjOffset,objOffset);
    objval = objval - objOffset;
  }
  else
  {
    *messageHandler() << "Warning: OsiGrbSolverInterface::getObjValue() called, but probably no solution available! Returning 0.0. Be aware!" << CoinMessageEol;
    if (OsiGrbSolverInterface::globalenv_)
      *messageHandler() << "\t GUROBI error message: " << GRBgeterrormsg(OsiGrbSolverInterface::globalenv_) << CoinMessageEol;
  }

  return objval;
}

//------------------------------------------------------------------
int OsiGrbSolverInterface::getIterationCount() const
{
  debugMessage("OsiGrbSolverInterface::getIterationCount()\n");

  double itercnt;
  
  GUROBI_CALL( "getIterationCount", GRBupdatemodel(getMutableLpPtr()) );

  GUROBI_CALL( "getIterationCount", GRBgetdblattr(getMutableLpPtr(), GRB_DBL_ATTR_ITERCOUNT, &itercnt) );

  return (int)itercnt;
}

//------------------------------------------------------------------
std::vector<double*> OsiGrbSolverInterface::getDualRays(int maxNumRays,
							bool fullRay) const
{
  debugMessage("OsiGrbSolverInterface::getDualRays(%d,%s)\n", maxNumRays,
	       fullRay?"true":"false");
  
  if (fullRay == true) {
    throw CoinError("Full dual rays not yet implemented.","getDualRays",
		    "OsiGrbSolverInterface");
  }

  OsiGrbSolverInterface solver(*this);

  const int numcols = getNumCols();
  const int numrows = getNumRows();
  int* index = new int[CoinMax(numcols,numrows)];
  int i;
  for ( i = CoinMax(numcols,numrows)-1; i >= 0; --i)
  {
  	index[i] = i;
  }
  double* obj = new double[CoinMax(numcols,2*numrows)];
  CoinFillN(obj, numcols, 0.0);
  solver.setObjCoeffSet(index, index+numcols, obj);

  double* clb = new double[2*numrows];
  double* cub = new double[2*numrows];

  const double plusone = 1.0;
  const double minusone = -1.0;
  const char* sense = getRowSense();

  const CoinPackedVectorBase** cols = new const CoinPackedVectorBase*[numrows];
  int newcols = 0;
  for (i = 0; i < numrows; ++i) {
  	switch (sense[i]) {
  		case 'L':
  			cols[newcols++] = new CoinShallowPackedVector(1, &index[i], &minusone, false);
  			break;
  		case 'G':
  			cols[newcols++] = new CoinShallowPackedVector(1, &index[i], &plusone, false);
  			break;
  		case 'R':
  		  cols[newcols++] = new CoinShallowPackedVector(1, &index[i], &minusone, false);
  		  cols[newcols++] = new CoinShallowPackedVector(1, &index[i], &plusone, false);
  		  break;
  		case 'N':
  		case 'E':
  			break;
  	}
  }

  CoinFillN(obj, newcols, 1.0);
  CoinFillN(clb, newcols, 0.0);
  CoinFillN(cub, newcols, getInfinity());

  solver.addCols(newcols, cols, clb, cub, obj);
  delete[] index;
  delete[] cols;
  delete[] clb;
  delete[] cub;
  delete[] obj;

  solver.setObjSense(1.0); // minimize
  solver.initialSolve();

  const double* solverpi = solver.getRowPrice();
  double* pi = new double[numrows];
  for ( i = numrows - 1; i >= 0; --i) {
  	pi[i] = -solverpi[i];
  }
  return std::vector<double*>(1, pi);
}

//------------------------------------------------------------------
std::vector<double*> OsiGrbSolverInterface::getPrimalRays(int maxNumRays) const
{
  debugMessage("OsiGrbSolverInterface::getPrimalRays(%d)\n", maxNumRays);

  return std::vector<double*>();
}

//#############################################################################
// Problem modifying methods (rim vectors)
//#############################################################################

void OsiGrbSolverInterface::setObjCoeff( int elementIndex, double elementValue )
{
  debugMessage("OsiGrbSolverInterface::setObjCoeff(%d, %g)\n", elementIndex, elementValue);
  
  GUROBI_CALL( "setObjCoeff", GRBsetdblattrelement(getLpPtr( OsiGrbSolverInterface::KEEPCACHED_PROBLEM ), GRB_DBL_ATTR_OBJ, nauxcols ? colmap_O2G[elementIndex] : elementIndex, elementValue) );

  if(obj_ != NULL)
    obj_[elementIndex] = elementValue;
}

//-----------------------------------------------------------------------------

void OsiGrbSolverInterface::setObjCoeffSet(const int* indexFirst,
					   const int* indexLast,
					   const double* coeffList)
{
  debugMessage("OsiGrbSolverInterface::setObjCoeffSet(%p, %p, %p)\n", (void*)indexFirst, (void*)indexLast, (void*)coeffList);

  const int cnt = (int)(indexLast - indexFirst);
  
  if( cnt == 0 )
    return;
  if( cnt == 1 )
  {
    setObjCoeff(*indexFirst, *coeffList);
    return;
  }
  assert(cnt > 1);

  if( nauxcols )
  {
    int* indices = new int[cnt];
    for( int i = 0; i < cnt; ++i )
      indices[i] = colmap_O2G[indexFirst[i]];
    GUROBI_CALL( "setObjCoeffSet", GRBsetdblattrlist(getLpPtr( OsiGrbSolverInterface::KEEPCACHED_PROBLEM ), GRB_DBL_ATTR_OBJ, cnt, indices, const_cast<double*>(coeffList)) );
    delete[] indices;
  }
  else
  {
    GUROBI_CALL( "setObjCoeffSet", GRBsetdblattrlist(getLpPtr( OsiGrbSolverInterface::KEEPCACHED_PROBLEM ), GRB_DBL_ATTR_OBJ, cnt, const_cast<int*>(indexFirst), const_cast<double*>(coeffList)) );
  }

	if (obj_ != NULL)
		for (int i = 0; i < cnt; ++i)
			obj_[indexFirst[i]] = coeffList[i];
	
}

//-----------------------------------------------------------------------------
void OsiGrbSolverInterface::setColLower(int elementIndex, double elementValue)
{
  debugMessage("OsiGrbSolverInterface::setColLower(%d, %g)\n", elementIndex, elementValue);

  GUROBI_CALL( "setColLower", GRBsetdblattrelement(getLpPtr( OsiGrbSolverInterface::KEEPCACHED_PROBLEM ), GRB_DBL_ATTR_LB, nauxcols ? colmap_O2G[elementIndex] : elementIndex, elementValue) );

  if(collower_ != NULL)
    collower_[elementIndex] = elementValue;
}

//-----------------------------------------------------------------------------
void OsiGrbSolverInterface::setColUpper(int elementIndex, double elementValue)
{  
  debugMessage("OsiGrbSolverInterface::setColUpper(%d, %g)\n", elementIndex, elementValue);

  GUROBI_CALL( "setColUpper", GRBsetdblattrelement(getLpPtr( OsiGrbSolverInterface::KEEPCACHED_PROBLEM ), GRB_DBL_ATTR_UB, nauxcols ? colmap_O2G[elementIndex] : elementIndex, elementValue) );
	
  if(colupper_ != NULL)
    colupper_[elementIndex] = elementValue;
}

//-----------------------------------------------------------------------------
void OsiGrbSolverInterface::setColBounds( int elementIndex, double lower, double upper )
{
  debugMessage("OsiGrbSolverInterface::setColBounds(%d, %g, %g)\n", elementIndex, lower, upper);

  setColLower(elementIndex, lower);
  setColUpper(elementIndex, upper);
}

//-----------------------------------------------------------------------------
void OsiGrbSolverInterface::setColSetBounds(const int* indexFirst,
					    const int* indexLast,
					    const double* boundList)
{
  debugMessage("OsiGrbSolverInterface::setColSetBounds(%p, %p, %p)\n", (void*)indexFirst, (void*)indexLast, (void*)boundList);

  const int cnt = (int)(indexLast - indexFirst);
  if (cnt == 0)
  	return;
  if (cnt == 1)
  {
    setColLower(*indexFirst, boundList[0]);
    setColUpper(*indexFirst, boundList[1]);
    return;
  }
  assert(cnt > 1);
  
  double* lbList = new double[cnt];
  double* ubList = new double[cnt];
  int* indices = nauxcols ? new int[cnt] : NULL;
  
  for (int i = 0; i < cnt; ++i)
  {
  	lbList[i] = boundList[2*i];
  	ubList[i] = boundList[2*i+1];
  	if( indices )
  	  indices[i] = colmap_O2G[indexFirst[i]];

    if(collower_ != NULL)
    	collower_[indexFirst[i]] = boundList[2*i];
    if(colupper_ != NULL)
    	colupper_[indexFirst[i]] = boundList[2*i+1];
  }
  
  GUROBI_CALL( "setColSetBounds", GRBsetdblattrlist(getLpPtr( OsiGrbSolverInterface::KEEPCACHED_PROBLEM ), GRB_DBL_ATTR_LB, cnt, indices ? indices : const_cast<int*>(indexFirst), lbList) );
  GUROBI_CALL( "setColSetBounds", GRBsetdblattrlist(getLpPtr( OsiGrbSolverInterface::KEEPCACHED_PROBLEM ), GRB_DBL_ATTR_UB, cnt, indices ? indices : const_cast<int*>(indexFirst), ubList) );

	delete[] lbList;
	delete[] ubList;
	delete[] indices;
}

//-----------------------------------------------------------------------------
void
OsiGrbSolverInterface::setRowLower( int i, double elementValue )
{
  debugMessage("OsiGrbSolverInterface::setRowLower(%d, %g)\n", i, elementValue);

  double rhs   = getRightHandSide()[i];
  double range = getRowRange()[i];
  char   sense = getRowSense()[i];
  double lower = 0, upper = 0;

  convertSenseToBound( sense, rhs, range, lower, upper );
  if( lower != elementValue )
  {
  	convertBoundToSense( elementValue, upper, sense, rhs, range );
  	setRowType( i, sense, rhs, range );
  }
}

//-----------------------------------------------------------------------------
void
OsiGrbSolverInterface::setRowUpper( int i, double elementValue )
{
  debugMessage("OsiGrbSolverInterface::setRowUpper(%d, %g)\n", i, elementValue);

  double rhs   = getRightHandSide()[i];
  double range = getRowRange()[i];
  char   sense = getRowSense()[i];
  double lower = 0, upper = 0;

  convertSenseToBound( sense, rhs, range, lower, upper );
  if( upper != elementValue ) {
  	convertBoundToSense( lower, elementValue, sense, rhs, range );
  	setRowType( i, sense, rhs, range );
  }
}

//-----------------------------------------------------------------------------
void
OsiGrbSolverInterface::setRowBounds( int elementIndex, double lower, double upper )
{
  debugMessage("OsiGrbSolverInterface::setRowBounds(%d, %g, %g)\n", elementIndex, lower, upper);

  double rhs, range;
  char sense;
  
  convertBoundToSense( lower, upper, sense, rhs, range );
  
  setRowType( elementIndex, sense, rhs, range );
}

//-----------------------------------------------------------------------------
void
OsiGrbSolverInterface::setRowType(int i, char sense, double rightHandSide, double range)
{
  debugMessage("OsiGrbSolverInterface::setRowType(%d, %c, %g, %g)\n", i, sense, rightHandSide, range);
  
  GUROBI_CALL( "setRowType", GRBupdatemodel(getMutableLpPtr()) );

  if( nauxcols && auxcolind[i] >= 0 )
  { // so far, row i is a ranged row
    switch (sense)
    {
      case 'R':
        // row i was ranged row and remains a ranged row
        assert(range > 0);
        GUROBI_CALL( "setRowType", GRBsetdblattrelement(getLpPtr( OsiGrbSolverInterface::KEEPCACHED_PROBLEM ), GRB_DBL_ATTR_LB, auxcolind[i], rightHandSide - range) );
        GUROBI_CALL( "setRowType", GRBsetdblattrelement(getLpPtr( OsiGrbSolverInterface::KEEPCACHED_PROBLEM ), GRB_DBL_ATTR_UB, auxcolind[i], rightHandSide ) );
        break;
      case 'N':
      case 'L':
      case 'G':
      case 'E':
        convertToNormalRow(i, sense, rightHandSide);
        break;
      default:
        std::cerr << "Unknown row sense: " << sense << std::endl;
        exit(-1);
    }
  }
  else if (sense == 'R')
  {
    convertToRangedRow(i, rightHandSide, range);
  }
  else
  {
    char grbsense;
    switch (sense)
    {
      case 'N':
        grbsense = GRB_LESS_EQUAL;
        rightHandSide = getInfinity();
        break;

      case 'L':
        grbsense = GRB_LESS_EQUAL;
        break;

      case 'G':
        grbsense = GRB_GREATER_EQUAL;
        break;

      case 'E':
        grbsense = GRB_EQUAL;
        break;

      default:
        std::cerr << "Unknown row sense: " << sense << std::endl;
        exit(-1);
    }
    GUROBI_CALL( "setRowType", GRBsetcharattrelement(getLpPtr( OsiGrbSolverInterface::KEEPCACHED_PROBLEM ), GRB_CHAR_ATTR_SENSE, i, grbsense ) );
    GUROBI_CALL( "setRowType", GRBsetdblattrelement(getLpPtr( OsiGrbSolverInterface::KEEPCACHED_PROBLEM ), GRB_DBL_ATTR_RHS, i, rightHandSide) );
  }

  if(rowsense_ != NULL)
    rowsense_[i] = sense;

  if(rhs_ != NULL)
    rhs_[i] = rightHandSide;
  
  if (rowlower_ != NULL || rowupper_ != NULL)
  {
  	double dummy;
  	convertSenseToBound(sense, rightHandSide, range,
  			rowlower_ ? rowlower_[i] : dummy,
  			rowupper_ ? rowupper_[i] : dummy);
  }
}

//-----------------------------------------------------------------------------
void OsiGrbSolverInterface::setRowSetBounds(const int* indexFirst,
					    const int* indexLast,
					    const double* boundList)
{
  debugMessage("OsiGrbSolverInterface::setRowSetBounds(%p, %p, %p)\n", (void*)indexFirst, (void*)indexLast, (void*)boundList);

  const long int cnt = indexLast - indexFirst;
  if (cnt <= 0)
    return;

  char* sense = new char[cnt];
  double* rhs = new double[cnt];
  double* range = new double[cnt];
  for (int i = 0; i < cnt; ++i)
    convertBoundToSense(boundList[2*i], boundList[2*i+1], sense[i], rhs[i], range[i]);
  setRowSetTypes(indexFirst, indexLast, sense, rhs, range);

  delete[] range;
  delete[] rhs;
  delete[] sense;
}

//-----------------------------------------------------------------------------
void
OsiGrbSolverInterface::setRowSetTypes(const int* indexFirst,
				      const int* indexLast,
				      const char* senseList,
				      const double* rhsList,
				      const double* rangeList)
{
  debugMessage("OsiGrbSolverInterface::setRowSetTypes(%p, %p, %p, %p, %p)\n", 
  		(void*)indexFirst, (void*)indexLast, (void*)senseList, (void*)rhsList, (void*)rangeList);

   const int cnt = (int)(indexLast - indexFirst);
   if (cnt == 0)
  	 return;
   if (cnt == 1)
   {
     setRowType(*indexFirst, *senseList, *rhsList, *rangeList);
     return;
   }
   assert(cnt > 0);

   GUROBI_CALL( "setRowSetTypes", GRBupdatemodel(getMutableLpPtr()) );

   char* grbsense = new char[cnt];
   double* rhs = new double[cnt];
   for (int i = 0; i < cnt; ++i)
   {
     rhs[i] = rhsList[i];
     if( nauxcols && auxcolind[indexFirst[i]] >= 0 )
     { // so far, row is a ranged row
       switch (senseList[i])
       {
         case 'R':
           // row i was ranged row and remains a ranged row
           assert(rangeList[i] > 0);
           GUROBI_CALL( "setRowSetTypes", GRBsetdblattrelement(getLpPtr( OsiGrbSolverInterface::KEEPCACHED_PROBLEM ), GRB_DBL_ATTR_LB, auxcolind[indexFirst[i]], rhsList[i] - rangeList[i]) );
           GUROBI_CALL( "setRowSetTypes", GRBsetdblattrelement(getLpPtr( OsiGrbSolverInterface::KEEPCACHED_PROBLEM ), GRB_DBL_ATTR_UB, auxcolind[indexFirst[i]], rhsList[i] ) );
           grbsense[i] = GRB_EQUAL;
           rhs[i] = 0.0;
           break;
         case 'N':
           convertToNormalRow(indexFirst[i], senseList[i], rhsList[i]);
           grbsense[i] = GRB_LESS_EQUAL;
           rhs[i] = getInfinity();
           break;
         case 'L':
           convertToNormalRow(indexFirst[i], senseList[i], rhsList[i]);
           grbsense[i] = GRB_LESS_EQUAL;
           break;
         case 'G':
           convertToNormalRow(indexFirst[i], senseList[i], rhsList[i]);
           grbsense[i] = GRB_GREATER_EQUAL;
           break;
         case 'E':
           convertToNormalRow(indexFirst[i], senseList[i], rhsList[i]);
           grbsense[i] = GRB_EQUAL;
           break;
         default:
           std::cerr << "Unknown row sense: " << senseList[i] << std::endl;
           exit(-1);
       }
     }
     else if (senseList[i] == 'R')
     {
       convertToRangedRow(indexFirst[i], rhsList[i], rangeList[i]);
       grbsense[i] = GRB_EQUAL;
       rhs[i] = 0.0;
     }
     else
     {
       switch (senseList[i])
       {
         case 'N':
           grbsense[i] = GRB_LESS_EQUAL;
           rhs[i] = getInfinity();
           break;

         case 'L':
           grbsense[i] = GRB_LESS_EQUAL;
           break;

         case 'G':
           grbsense[i] = GRB_GREATER_EQUAL;
           break;

         case 'E':
           grbsense[i] = GRB_EQUAL;
           break;

         default:
           std::cerr << "Unknown row sense: " << senseList[i] << std::endl;
           exit(-1);
       }
     }
  	 
     if(rowsense_ != NULL)
  		 rowsense_[indexFirst[i]] = senseList[i];

     if(rhs_ != NULL)
    	 rhs_[indexFirst[i]] = senseList[i] == 'N' ? getInfinity() : rhsList[i];
   }
   
   GUROBI_CALL( "setRowSetTypes", GRBsetcharattrlist(getLpPtr(OsiGrbSolverInterface::KEEPCACHED_ROW), GRB_CHAR_ATTR_SENSE, cnt, const_cast<int*>(indexFirst), grbsense) );
   GUROBI_CALL( "setRowSetTypes", GRBsetdblattrlist(getLpPtr(OsiGrbSolverInterface::KEEPCACHED_ROW), GRB_DBL_ATTR_RHS, cnt, const_cast<int*>(indexFirst), rhs) );

   delete[] rhs;
   delete[] grbsense;
}

//#############################################################################
void
OsiGrbSolverInterface::setContinuous(int index)
{
  debugMessage("OsiGrbSolverInterface::setContinuous(%d)\n", index);

  assert(coltype_ != NULL);
  assert(colspace_ >= getNumCols());

  coltype_[index] = GRB_CONTINUOUS;

  if ( probtypemip_ )
  {
    GUROBI_CALL( "setContinuous", GRBupdatemodel(getMutableLpPtr()) );
    GUROBI_CALL( "setContinuous", GRBsetcharattrelement(getMutableLpPtr(), GRB_CHAR_ATTR_VTYPE, nauxcols ? colmap_O2G[index] : index, GRB_CONTINUOUS) );
  }
}

//-----------------------------------------------------------------------------
void
OsiGrbSolverInterface::setInteger(int index)
{
  debugMessage("OsiGrbSolverInterface::setInteger(%d)\n", index);

  assert(coltype_ != NULL);
  assert(colspace_ >= getNumCols());

  if( getColLower()[index] == 0.0 && getColUpper()[index] == 1.0 )
     coltype_[index] = GRB_BINARY;
  else
     coltype_[index] = GRB_INTEGER;

  if ( probtypemip_ )
  {
    GUROBI_CALL( "setInteger", GRBupdatemodel(getMutableLpPtr()) );
    GUROBI_CALL( "setInteger", GRBsetcharattrelement(getMutableLpPtr(), GRB_CHAR_ATTR_VTYPE, nauxcols ? colmap_O2G[index] : index, coltype_[index]) );
  }
}

//-----------------------------------------------------------------------------
void
OsiGrbSolverInterface::setContinuous(const int* indices, int len)
{
  debugMessage("OsiGrbSolverInterface::setContinuous(%p, %d)\n", (void*)indices, len);

  for( int i = 0; i < len; ++i )
     setContinuous(indices[i]);
}

//-----------------------------------------------------------------------------
void
OsiGrbSolverInterface::setInteger(const int* indices, int len)
{
  debugMessage("OsiGrbSolverInterface::setInteger(%p, %d)\n", (void*)indices, len);

  for( int i = 0; i < len; ++i )
     setInteger(indices[i]);
}

//#############################################################################

void
OsiGrbSolverInterface::setRowName(int ndx, std::string name)
{
  debugMessage("OsiGrbSolverInterface::setRowName\n");

  if (ndx < 0 || ndx >= getNumRows())
    return;

  int nameDiscipline ;
  getIntParam(OsiNameDiscipline, nameDiscipline);
  if (nameDiscipline == 0)
    return;

  OsiSolverInterface::setRowName(ndx, name);

  GUROBI_CALL( "setRowName", GRBsetstrattrelement(getLpPtr(), GRB_STR_ATTR_CONSTRNAME, ndx, const_cast<char*>(name.c_str())) );
}

void
OsiGrbSolverInterface::setColName(int ndx, std::string name)
{
  debugMessage("OsiGrbSolverInterface::setColName\n");

  if (ndx < 0 || ndx >= getNumCols())
    return;

  int nameDiscipline ;
  getIntParam(OsiNameDiscipline, nameDiscipline);
  if (nameDiscipline == 0)
    return;

  OsiSolverInterface::setColName(ndx, name);

  GUROBI_CALL( "setColName", GRBsetstrattrelement(getLpPtr(), GRB_STR_ATTR_VARNAME, nauxcols ? colmap_O2G[ndx] : ndx, const_cast<char*>(name.c_str())) );
}

//#############################################################################

void OsiGrbSolverInterface::setObjSense(double s) 
{
  debugMessage("OsiGrbSolverInterface::setObjSense(%g)\n", s);
  
  GUROBI_CALL( "setObjSense", GRBsetintattr(getLpPtr( OsiGrbSolverInterface::FREECACHED_RESULTS ), GRB_INT_ATTR_MODELSENSE, (int)s) );
}

//-----------------------------------------------------------------------------

void OsiGrbSolverInterface::setColSolution(const double * cs) 
{
  debugMessage("OsiGrbSolverInterface::setColSolution(%p)\n", (void*)cs);

  int nc = getNumCols();

  if( cs == NULL )
  	freeCachedResults();
  else if( nc > 0 )
  {
  	// If colsol isn't allocated, then allocate it
  	if ( colsol_ == NULL )
  		colsol_ = new double[nc];

  	// Copy in new col solution.
  	CoinDisjointCopyN( cs, nc, colsol_ );

  	//*messageHandler() << "OsiGrb::setColSolution: Gurobi does not allow setting the column solution. Command is ignored." << CoinMessageEol;
  }
}

//-----------------------------------------------------------------------------

void OsiGrbSolverInterface::setRowPrice(const double * rs) 
{
  debugMessage("OsiGrbSolverInterface::setRowPrice(%p)\n", (void*)rs);

  int nr = getNumRows();

  if( rs == NULL )
    freeCachedResults();
  else if( nr > 0 )
  {
  	// If rowsol isn't allocated, then allocate it
  	if ( rowsol_ == NULL )
  		rowsol_ = new double[nr];

  	// Copy in new row solution.
  	CoinDisjointCopyN( rs, nr, rowsol_ );

  	*messageHandler() << "OsiGrb::setRowPrice: Gurobi does not allow setting the row price. Command is ignored." << CoinMessageEol;
  }
}

//#############################################################################
// Problem modifying methods (matrix)
//#############################################################################
void 
OsiGrbSolverInterface::addCol(const CoinPackedVectorBase& vec,
			      const double collb, const double colub,   
			      const double obj)
{
  debugMessage("OsiGrbSolverInterface::addCol(%p, %g, %g, %g)\n", (void*)&vec, collb, colub, obj);

  int nc = getNumCols();
  assert(colspace_ >= nc);

  resizeColSpace(nc + 1);
  coltype_[nc] = GRB_CONTINUOUS;

  GUROBI_CALL( "addCol", GRBupdatemodel(getMutableLpPtr()) );

  GUROBI_CALL( "addCol", GRBaddvar(getLpPtr( OsiGrbSolverInterface::KEEPCACHED_ROW ),
  		vec.getNumElements(),
  		const_cast<int*>(vec.getIndices()),
  		const_cast<double*>(vec.getElements()),
  		obj, collb, colub, coltype_[nc], NULL) );

  if( nauxcols )
  {
    colmap_O2G[nc] = nc + nauxcols;
    colmap_G2O[nc + nauxcols] = nc;
  }
}

//-----------------------------------------------------------------------------
void 
OsiGrbSolverInterface::addCols(const int numcols,
			       const CoinPackedVectorBase * const * cols,
			       const double* collb, const double* colub,   
			       const double* obj)
{
  debugMessage("OsiGrbSolverInterface::addCols(%d, %p, %p, %p, %p)\n", numcols, (void*)cols, (void*)collb, (void*)colub, (void*)obj);

  int nc = getNumCols();
  assert(colspace_ >= nc);

  resizeColSpace(nc + numcols);
  CoinFillN(&coltype_[nc], numcols, GRB_CONTINUOUS);

  int i;
  int nz = 0;
  for (i = 0; i < numcols; ++i)
    nz += cols[i]->getNumElements();

  int* index = new int[nz];
  double* elem = new double[nz];
  int* start = new int[numcols+1];

  nz = 0;
  start[0] = 0;
  for (i = 0; i < numcols; ++i)
  {
    const CoinPackedVectorBase* col = cols[i];
    const int len = col->getNumElements();
    CoinDisjointCopyN(col->getIndices(), len, index+nz);
    CoinDisjointCopyN(col->getElements(), len, elem+nz);
    nz += len;
    start[i+1] = nz;
  }

  GUROBI_CALL( "addCols", GRBupdatemodel(getMutableLpPtr()) );
  
  GUROBI_CALL( "addCols", GRBaddvars(getLpPtr(OsiGrbSolverInterface::KEEPCACHED_ROW),
  		numcols, nz,
  		start, index, elem,
  		const_cast<double*>(obj),
  		const_cast<double*>(collb), const_cast<double*>(colub),
  		NULL, NULL) );

  delete[] start;
  delete[] elem;
  delete[] index;

  if( nauxcols )
    for( i = 0; i < numcols; ++i )
    {
      colmap_O2G[nc+i] = nc+i + nauxcols;
      colmap_G2O[nc+i + nauxcols] = nc+i;
    }
}

static int intcompare(const void* p1, const void* p2)
{
  return (*(const int*)p1) - (*(const int*)p2);
}

//-----------------------------------------------------------------------------
void
OsiGrbSolverInterface::deleteCols(const int num, const int * columnIndices)
{
  debugMessage("OsiGrbSolverInterface::deleteCols(%d, %p)\n", num, (void*)columnIndices);

  if( num == 0 )
    return;

  GUROBI_CALL( "deleteCols", GRBupdatemodel(getMutableLpPtr()) );
  
  int* ind = NULL;

  if( nauxcols )
  {
    int nc = getNumCols();

    ind = new int[num];

    // translate into gurobi indices and sort
    for( int i = 0; i < num; ++i )
      ind[i] = colmap_O2G[columnIndices[i]];
    qsort((void*)ind, num, sizeof(int), intcompare);

    // delete indices in gurobi
    GUROBI_CALL( "deleteCols", GRBdelvars(getLpPtr( OsiGrbSolverInterface::KEEPCACHED_ROW ), num, ind) );

    nc -= num;

    // update colmap_G2O and auxcolind
    int offset = 0;
    int j = 0;
    for( int i = ind[0]; i < nc + nauxcols; ++i )
    {
      if( j < num && i == ind[j] )
      { // variable i has been deleted, increase offset by 1
        assert(colmap_G2O[i] >= 0);
        ++offset;
        while( j < num && i == ind[j] )
          ++j;
      }
      // variable from position i+offset has moved to position i
      if( colmap_G2O[i+offset] >= 0 )
      { // if variable at (hithero) position i+offset was a regular variable, then it is now stored at position i
        // substract offset since variable indices also changed in Osi
        colmap_G2O[i] = colmap_G2O[i+offset] - offset;
        assert(colmap_G2O[i] >= 0);
        // update also colmap_O2G
        colmap_O2G[colmap_G2O[i]] = i;
      }
      else
      { // if variable at (hithero) position i+offset was an auxiliary variable, then the corresponding row index is now stored at position i
        int rngrowidx = -colmap_G2O[i+offset]-1;
        assert(auxcolind[rngrowidx] == i+offset);
        colmap_G2O[i] = colmap_G2O[i+offset];
        // update also auxcolind to point to position i now
        auxcolind[rngrowidx] = i;
      }
    }
  }
  else
  {
    GUROBI_CALL( "deleteCols", GRBdelvars(getLpPtr( OsiGrbSolverInterface::KEEPCACHED_ROW ), num, const_cast<int*>(columnIndices)) );
  }

#ifndef NDEBUG
  if( nauxcols )
  {
    int nc = getNumCols();
    int nr = getNumRows();
    for( int i = 0; i < nc + nauxcols; ++i )
    {
      assert(i >= nc || colmap_G2O[colmap_O2G[i]] == i);
      assert(colmap_G2O[i] <  0 || colmap_O2G[colmap_G2O[i]] == i);
      assert(colmap_G2O[i] >= 0 || -colmap_G2O[i]-1 < nr);
      assert(colmap_G2O[i] >= 0 || auxcolind[-colmap_G2O[i]-1] == i);
    }
  }
#endif

  if( !getColNames().empty() || coltype_ != NULL )
  {
    if( ind == NULL )
      ind = new int[num];
      
    memcpy(ind, columnIndices, num * sizeof(int));
    qsort((void*)ind, num, sizeof(int), intcompare);
    
    if( !getColNames().empty() )
       for( int i = num-1; i >= 0; --i )
          deleteColNames(ind[i], 1);

    if( coltype_ != NULL )
    {
      int offset = 0;
      for( int i = 0; i <= getNumCols(); ++i )
      {
        // variable i+offset was deleted
        if( offset < num && ind[offset] == i+offset )
          ++offset;

        // move column type from position i+offset to i
        coltype_[i] = coltype_[i+offset];
      }
    }
  }
    
  delete[] ind;
}

//-----------------------------------------------------------------------------
void 
OsiGrbSolverInterface::addRow(const CoinPackedVectorBase& vec,
			      const double rowlb, const double rowub)
{
  debugMessage("OsiGrbSolverInterface::addRow(%p, %g, %g)\n", (void*)&vec, rowlb, rowub);

  char sense;
  double rhs, range;

  convertBoundToSense( rowlb, rowub, sense, rhs, range );
  
  addRow( vec, sense, rhs, range );
}

//-----------------------------------------------------------------------------
void 
OsiGrbSolverInterface::addRow(const CoinPackedVectorBase& vec,
			      const char rowsen, const double rowrhs,   
			      const double rowrng)
{
  debugMessage("OsiGrbSolverInterface::addRow(%p, %c, %g, %g)\n", (void*)&vec, rowsen, rowrhs, rowrng);

  char grbsense;
  double rhs = rowrhs;

  switch( rowsen )
  {
    case 'R':
      /* ranged rows are added as equality rows first, and then an auxiliary variable is added */
      assert(rowrng > 0.0);
      grbsense = GRB_EQUAL;
      rhs = 0.0;
      break;

  	case 'N':
  		grbsense = GRB_LESS_EQUAL;
  		rhs   = getInfinity();
  		break;
  		
  	case 'L':
  		grbsense = GRB_LESS_EQUAL;
  		break;

  	case 'G':
  		grbsense = GRB_GREATER_EQUAL;
  		break;
  		
  	case 'E':
  		grbsense = GRB_EQUAL;
  		break;
	
  	default:
  	  std::cerr << "Unknown row sense: " << rowsen << std::endl;
  	  exit(-1);
  }

  GUROBI_CALL( "addRow", GRBupdatemodel(getMutableLpPtr()) );

  GUROBI_CALL( "addRow", GRBaddconstr(getLpPtr( OsiGrbSolverInterface::KEEPCACHED_COLUMN ),
  		vec.getNumElements(),
  		const_cast<int*>(vec.getIndices()),
  		const_cast<double*>(vec.getElements()),
  		grbsense, rhs, NULL) );

  if( rowsen == 'R' )
    convertToRangedRow(getNumRows()-1, rowrhs, rowrng);
  else if( nauxcols )
    resizeAuxColIndSpace();
}

//-----------------------------------------------------------------------------
void 
OsiGrbSolverInterface::addRows(const int numrows,
			       const CoinPackedVectorBase * const * rows,
			       const double* rowlb, const double* rowub)
{
  debugMessage("OsiGrbSolverInterface::addRows(%d, %p, %p, %p)\n", numrows, (void*)rows, (void*)rowlb, (void*)rowub);

  int i;
  int nz = 0;
  for (i = 0; i < numrows; ++i)
    nz += rows[i]->getNumElements();

  int* index = new int[nz];
  double* elem = new double[nz];
  int* start = new int[numrows+1];
  char* grbsense = new char[numrows];
  double* rhs = new double[numrows];
  double range;

  bool haverangedrows = false;

  nz = 0;
  start[0] = 0;
  for (i = 0; i < numrows; ++i)
  {
    const CoinPackedVectorBase* row = rows[i];
    const int len = row->getNumElements();
    CoinDisjointCopyN(row->getIndices(), len, index+nz);
    CoinDisjointCopyN(row->getElements(), len, elem+nz);
    nz += len;
    start[i+1] = nz;
    
    convertBoundToSense( rowlb[i], rowub[i], grbsense[i], rhs[i], range );
    if (range || grbsense[i] == 'R')
    {
      grbsense[i] = 'E';
      rhs[i] = 0.0;
      haverangedrows = true;
    }
    
    switch (grbsense[i])
    {
    	case 'N':
    		grbsense[i] = GRB_LESS_EQUAL;
    		rhs[i] = getInfinity();
    		break;
    	case 'L':
    		grbsense[i] = GRB_LESS_EQUAL;
    		break;
    	case 'G':
    		grbsense[i] = GRB_GREATER_EQUAL;
    		break;
    	case 'E':
    		grbsense[i] = GRB_EQUAL;
    		break;
    }
  }
  
  GUROBI_CALL( "addRows", GRBupdatemodel(getMutableLpPtr()) );

  GUROBI_CALL( "addRows", GRBaddconstrs(getLpPtr(OsiGrbSolverInterface::KEEPCACHED_ROW),
  		numrows, nz,
  		start, index, elem,
  		grbsense, rhs, NULL) );

  delete[] start;
  delete[] elem;
  delete[] index;
  delete[] grbsense;
  delete[] rhs;

  if( haverangedrows )
  {
    int nr = getNumRows() - numrows;
    for( i = 0; i < numrows; ++i )
      if( rowlb[i] > getInfinity() && rowub[i] < getInfinity() && rowub[i] - rowlb[i] > 0.0 )
        convertToRangedRow(nr + i, rowub[i], rowub[i] - rowlb[i]);
  }
  else if( nauxcols )
    resizeAuxColIndSpace();
}

//-----------------------------------------------------------------------------
void 
OsiGrbSolverInterface::addRows(const int numrows,
			       const CoinPackedVectorBase * const * rows,
			       const char* rowsen, const double* rowrhs,
			       const double* rowrng)
{
  debugMessage("OsiGrbSolverInterface::addRows(%d, %p, %p, %p, %p)\n", numrows, (void*)rows, (void*)rowsen, (void*)rowrhs, (void*)rowrng);

  int i;
  int nz = 0;
  for (i = 0; i < numrows; ++i)
    nz += rows[i]->getNumElements();

  int* index = new int[nz];
  double* elem = new double[nz];
  int* start = new int[numrows+1];
  char* grbsense = new char[numrows];
  double* rhs = new double[numrows];
  bool haverangedrows = false;

  nz = 0;
  start[0] = 0;
  for (i = 0; i < numrows; ++i)
  {
    const CoinPackedVectorBase* row = rows[i];
    const int len = row->getNumElements();
    CoinDisjointCopyN(row->getIndices(), len, index+nz);
    CoinDisjointCopyN(row->getElements(), len, elem+nz);
    nz += len;
    start[i+1] = nz;
    
    rhs[i] = rowrhs[i];
    
    switch (rowsen[i])
    {
      case 'R':
        assert(rowrng[i] > 0.0);
        grbsense[i] = GRB_EQUAL;
        rhs[i] = 0.0;
        haverangedrows = true;
        break;
    	case 'N':
    		grbsense[i] = GRB_LESS_EQUAL;
    		rhs[i] = getInfinity();
    		break;
    	case 'L':
    		grbsense[i] = GRB_LESS_EQUAL;
    		break;
    	case 'G':
    		grbsense[i] = GRB_GREATER_EQUAL;
    		break;
    	case 'E':
    		grbsense[i] = GRB_EQUAL;
    		break;
    }
  }
  
  GUROBI_CALL( "addRows", GRBupdatemodel(getMutableLpPtr()) );

  GUROBI_CALL( "addRows", GRBaddconstrs(getLpPtr(OsiGrbSolverInterface::KEEPCACHED_ROW),
  		numrows, nz,
  		start, index, elem,
  		grbsense, rhs, NULL) );

  delete[] start;
  delete[] elem;
  delete[] index;
  delete[] grbsense;
  delete[] rhs;

  if( haverangedrows )
  {
    int nr = getNumRows() - numrows;
    for( i = 0; i < numrows; ++i )
      if( rowsen[i] == 'R' )
        convertToRangedRow(nr + i, rowrhs[i], rowrng[i]);
  }
  else if( nauxcols )
    resizeAuxColIndSpace();
}

//-----------------------------------------------------------------------------
void 
OsiGrbSolverInterface::deleteRows(const int num, const int * rowIndices)
{
  debugMessage("OsiGrbSolverInterface::deleteRows(%d, %p)\n", num, (void*)rowIndices);

  if( nauxcols )
  { // check if a ranged row should be deleted; if so, then convert it into a normal row first
    for( int i = 0; i < num; ++i )
    {
      if( auxcolind[rowIndices[i]] >= 0 )
        convertToNormalRow(rowIndices[i], 'E', 0.0);
    }
  }

  GUROBI_CALL( "deleteRows", GRBdelconstrs(getLpPtr( OsiGrbSolverInterface::KEEPCACHED_COLUMN ), num, const_cast<int*>(rowIndices)) );

  if( nauxcols == 0 && getRowNames().empty() )
    return;

  int* ind = CoinCopyOfArray(rowIndices, num);
  qsort((void*)ind, num, sizeof(int), intcompare);

  if( nauxcols )
  {
    int nr = getNumRows();

    int offset = 0;
    int j = 0;
    for( int i = 0; i < nr; ++i )
    {
      if( j < num && i == ind[j] )
      {
        ++offset;
        while( j < num && i == ind[j] )
          ++j;
      }
      auxcolind[i] = auxcolind[i + offset];
      if( auxcolind[i] >= 0 )
        colmap_G2O[auxcolind[i]] = - i - 1;
    }
  }

#ifndef NDEBUG
  if( nauxcols )
  {
    int nc = getNumCols();
    int nr = getNumRows();
    for( int i = 0; i < nc + nauxcols; ++i )
    {
      assert(i >= nc || colmap_G2O[colmap_O2G[i]] == i);
      assert(colmap_G2O[i] <  0 || colmap_O2G[colmap_G2O[i]] == i);
      assert(colmap_G2O[i] >= 0 || -colmap_G2O[i]-1 < nr);
      assert(colmap_G2O[i] >= 0 || auxcolind[-colmap_G2O[i]-1] == i);
    }
  }
#endif

  if( !getRowNames().empty() )
    for( int i = num-1; i >=0; --i )
      deleteRowNames(ind[i], 1);
    
  delete[] ind;
}

//#############################################################################
// Methods to input a problem
//#############################################################################

void
OsiGrbSolverInterface::loadProblem( const CoinPackedMatrix& matrix,
				    const double* collb, const double* colub,
				    const double* obj,
				    const double* rowlb, const double* rowub )
{
  debugMessage("OsiGrbSolverInterface::loadProblem(1)(%p, %p, %p, %p, %p, %p)\n", (void*)&matrix, (void*)collb, (void*)colub, (void*)obj, (void*)rowlb, (void*)rowub);

  const double inf = getInfinity();

  int nrows = matrix.getNumRows();
  char   * rowSense = new char  [nrows];
  double * rowRhs   = new double[nrows];
  double * rowRange = new double[nrows];

  int i;
  for ( i = nrows - 1; i >= 0; --i )
  {
  	const double lower = rowlb ? rowlb[i] : -inf;
  	const double upper = rowub ? rowub[i] : inf;
  	convertBoundToSense( lower, upper, rowSense[i], rowRhs[i], rowRange[i] );
  }

  loadProblem( matrix, collb, colub, obj, rowSense, rowRhs, rowRange ); 
  delete [] rowSense;
  delete [] rowRhs;
  delete [] rowRange;
}
			    
//-----------------------------------------------------------------------------

void
OsiGrbSolverInterface::assignProblem( CoinPackedMatrix*& matrix,
				      double*& collb, double*& colub,
				      double*& obj,
				      double*& rowlb, double*& rowub )
{
  debugMessage("OsiGrbSolverInterface::assignProblem()\n");

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
OsiGrbSolverInterface::loadProblem( const CoinPackedMatrix& matrix,
				    const double* collb, const double* colub,
				    const double* obj,
				    const char* rowsen, const double* rowrhs,
				    const double* rowrng )
{
	debugMessage("OsiGrbSolverInterface::loadProblem(2)(%p, %p, %p, %p, %p, %p, %p)\n",
			(void*)&matrix, (void*)collb, (void*)colub, (void*)obj, (void*)rowsen, (void*)rowrhs, (void*)rowrng);

	int nc = matrix.getNumCols();
	int nr = matrix.getNumRows();
	int i;

	char* myrowsen = new char[nr];
	double* myrowrhs = new double[nr];
	bool haverangedrows = false;
	
	for ( i = 0; i < nr; ++i )
	{
		if (rowrhs)
			myrowrhs[i] = rowrhs[i];
		else
			myrowrhs[i] = 0.0;

		if (rowsen)
			switch (rowsen[i])
			{
				case 'R':
				  assert( rowrng && rowrng[i] > 0.0 );
				  myrowsen[i] = GRB_EQUAL;
				  myrowrhs[i] = 0.0;
				  haverangedrows = true;
				  break;

				case 'N':
					myrowsen[i] = GRB_LESS_EQUAL;
					myrowrhs[i] = getInfinity();
					break;

				case 'L':
					myrowsen[i] = GRB_LESS_EQUAL;
					break;

				case 'G':
					myrowsen[i] = GRB_GREATER_EQUAL;
					break;

				case 'E':
					myrowsen[i] = GRB_EQUAL;
					break;
			}
		else
			myrowsen[i] = 'G';
	}

	// Set column values to defaults if NULL pointer passed
	double * clb;  
	double * cub;
	double * ob;
	if ( collb != NULL )
		clb = const_cast<double*>(collb);
	else
	{
		clb = new double[nc];
		CoinFillN(clb, nc, 0.0);
	}

	if ( colub!=NULL )
		cub = const_cast<double*>(colub);
	else
	{
		cub = new double[nc];
		CoinFillN(cub, nc, getInfinity());
	}

	if ( obj!=NULL )
		ob = const_cast<double*>(obj);
	else
	{
		ob = new double[nc];
		CoinFillN(ob, nc, 0.0);
	}

	bool freeMatrixRequired = false;
	CoinPackedMatrix * m = NULL;

  if( !matrix.isColOrdered() )
  {
    m = new CoinPackedMatrix();
    m->reverseOrderedCopyOf(matrix);
    freeMatrixRequired = true;
  }
  else
    m = const_cast<CoinPackedMatrix *>(&matrix);

  // up to GUROBI 2.0.1, GUROBI may give an "Index is out of range" error if the constraint matrix has uninitalized "gaps"
#if (GRB_VERSION_MAJOR < 2) || (GRB_VERSION_MAJOR == 2 && GRB_VERSION_MINOR == 0 && GRB_VERSION_TECHNICAL <= 1)
	if ( m->hasGaps() )
	{
	  if( freeMatrixRequired )
	  {
	    m->removeGaps();
	  }
	  else
	  {
      m = new CoinPackedMatrix(matrix);
      if( m->hasGaps() )
        m->removeGaps();
      freeMatrixRequired = true;
	  }
	}
#endif

	assert( nc == m->getNumCols() );
	assert( nr == m->getNumRows() );
	assert( m->isColOrdered() ); 
	
	int modelsense;
	GUROBI_CALL( "loadProblem", GRBupdatemodel(getMutableLpPtr()) );
  
	GUROBI_CALL( "loadProblem", GRBgetintattr(getMutableLpPtr(), GRB_INT_ATTR_MODELSENSE, &modelsense) );

	std::string pn;
	getStrParam(OsiProbName, pn);

	gutsOfDestructor(); // kill old LP, if any

	GUROBI_CALL( "loadProblem", GRBloadmodel(getEnvironmentPtr(), &lp_, const_cast<char*>(pn.c_str()),
			nc, nr,
			modelsense,
			0.0,
			ob, 
			const_cast<char *>(myrowsen),
			myrowrhs,
			const_cast<int *>(m->getVectorStarts()),
			const_cast<int *>(m->getVectorLengths()),
			const_cast<int *>(m->getIndices()),
			const_cast<double *>(m->getElements()),
			const_cast<double *>(clb), 
			const_cast<double *>(cub), 
			NULL, NULL, NULL) );

  // GUROBI up to version 2.0.1 may return a scaled LP after GRBoptimize when requesting it via GRBgetvars
#if (GRB_VERSION_MAJOR < 2) || (GRB_VERSION_MAJOR == 2 && GRB_VERSION_MINOR == 0 && GRB_VERSION_TECHNICAL <= 1)
  setHintParam(OsiDoScale, false);
#endif

	delete[] myrowsen;
	delete[] myrowrhs;
	if ( collb == NULL )
		delete[] clb;
	if ( colub == NULL ) 
		delete[] cub;
	if ( obj   == NULL )
		delete[] ob;
	if ( freeMatrixRequired ) 
		delete m;

	resizeColSpace(nc);
	CoinFillN(coltype_, nc, GRB_CONTINUOUS);

	if( haverangedrows )
	{
	  assert(rowrhs);
	  assert(rowrng);
	  assert(rowsen);
    for ( i = 0; i < nr; ++i )
      if( rowsen[i] == 'R' )
        convertToRangedRow(i, rowrhs[i], rowrng[i]);
	}
}
   
//-----------------------------------------------------------------------------

void
OsiGrbSolverInterface::assignProblem( CoinPackedMatrix*& matrix,
				      double*& collb, double*& colub,
				      double*& obj,
				      char*& rowsen, double*& rowrhs,
				      double*& rowrng )
{
  debugMessage("OsiGrbSolverInterface::assignProblem()\n");

  loadProblem( *matrix, collb, colub, obj, rowsen, rowrhs, rowrng );
  delete matrix;   matrix = NULL;
  delete[] collb;  collb = NULL;
  delete[] colub;  colub = NULL;
  delete[] obj;    obj = NULL;
  delete[] rowsen; rowsen = NULL;
  delete[] rowrhs; rowrhs = NULL;
  delete[] rowrng; rowrng = NULL;
}

//-----------------------------------------------------------------------------

void
OsiGrbSolverInterface::loadProblem(const int numcols, const int numrows,
				   const int* start, const int* index,
				   const double* value,
				   const double* collb, const double* colub,   
				   const double* obj,
				   const double* rowlb, const double* rowub )
{
  debugMessage("OsiGrbSolverInterface::loadProblem(3)()\n");

  const double inf = getInfinity();
  
  char   * rowSense = new char  [numrows];
  double * rowRhs   = new double[numrows];
  double * rowRange = new double[numrows];
  
  for ( int i = numrows - 1; i >= 0; --i ) {
    const double lower = rowlb ? rowlb[i] : -inf;
    const double upper = rowub ? rowub[i] : inf;
    convertBoundToSense( lower, upper, rowSense[i], rowRhs[i], rowRange[i] );
  }

  loadProblem(numcols, numrows, start, index, value, collb, colub, obj, rowSense, rowRhs, rowRange);
  
  delete [] rowSense;
  delete [] rowRhs;
  delete [] rowRange;
}

//-----------------------------------------------------------------------------

void
OsiGrbSolverInterface::loadProblem(const int numcols, const int numrows,
				   const int* start, const int* index,
				   const double* value,
				   const double* collb, const double* colub,   
				   const double* obj,
				   const char* rowsen, const double* rowrhs,
				   const double* rowrng )
{
  debugMessage("OsiGrbSolverInterface::loadProblem(4)(%d, %d, %p, %p, %p, %p, %p, %p, %p, %p, %p)\n",
  		numcols, numrows, (void*)start, (void*)index, (void*)value, (void*)collb, (void*)colub, (void*)obj, (void*)rowsen, (void*)rowrhs, (void*)rowrng);

	int nc = numcols;
	int nr = numrows;
	int i;

	char* myrowsen = new char[nr];
	double* myrowrhs = new double[nr];
	bool haverangedrows = false;
	
	for ( i=0; i<nr; i++ )
	{
		if (rowrhs)
			myrowrhs[i] = rowrhs[i];
		else
			myrowrhs[i] = 0.0;

		if (rowsen)
			switch (rowsen[i])
			{
				case 'R':
				  assert(rowrng && rowrng[i] > 0.0);
				  myrowsen[i] = GRB_EQUAL;
				  myrowrhs[i] = 0.0;
				  haverangedrows = true;
				  break;

				case 'N':
					myrowsen[i] = GRB_LESS_EQUAL;
					myrowrhs[i] = getInfinity();
					break;

				case 'L':
					myrowsen[i] = GRB_LESS_EQUAL;
					break;

				case 'G':
					myrowsen[i] = GRB_GREATER_EQUAL;
					break;

				case 'E':
					myrowsen[i] = GRB_EQUAL;
					break;
			}
		else
			myrowsen[i] = 'G';
	}

	// Set column values to defaults if NULL pointer passed
	double * clb;  
	double * cub;
	double * ob;
	if ( collb != NULL )
		clb = const_cast<double*>(collb);
	else
	{
		clb = new double[nc];
		CoinFillN(clb, nc, 0.0);
	}

	if ( colub!=NULL )
		cub = const_cast<double*>(colub);
	else
	{
		cub = new double[nc];
		CoinFillN(cub, nc, getInfinity());
	}

	if ( obj!=NULL )
		ob = const_cast<double*>(obj);
	else
	{
		ob = new double[nc];
		CoinFillN(ob, nc, 0.0);
	}
	
	int* len = new int[nc];
	for (i = 0; i < nc; ++i)
		len[i] = start[i+1] - start[i];

	GUROBI_CALL( "loadProblem", GRBupdatemodel(getMutableLpPtr()) );

	int modelsense;
	GUROBI_CALL( "loadProblem", GRBgetintattr(getMutableLpPtr(), GRB_INT_ATTR_MODELSENSE, &modelsense) );

	std::string pn;
	getStrParam(OsiProbName, pn);

	gutsOfDestructor(); // kill old LP, if any

	GUROBI_CALL( "loadProblem", GRBloadmodel(getEnvironmentPtr(), &lp_, const_cast<char*>(pn.c_str()),
			nc, nr,
			modelsense,
			0.0,
			ob, 
			myrowsen,
			myrowrhs,
			const_cast<int*>(start), len,
			const_cast<int*>(index),
			const_cast<double*>(value),
			const_cast<double *>(clb), 
			const_cast<double *>(cub), 
			NULL, NULL, NULL) );

  // GUROBI up to version 2.0.1 may return a scaled LP after GRBoptimize when requesting it via GRBgetvars
#if (GRB_VERSION_MAJOR < 2) || (GRB_VERSION_MAJOR == 2 && GRB_VERSION_MINOR == 0 && GRB_VERSION_TECHNICAL <= 1)
  setHintParam(OsiDoScale, false);
#endif

	delete[] myrowsen;
	delete[] myrowrhs;
	if ( collb == NULL )
		delete[] clb;
	if ( colub == NULL ) 
		delete[] cub;
	if ( obj   == NULL )
		delete[] ob;
	delete[] len;

	resizeColSpace(nc);
	CoinFillN(coltype_, nc, GRB_CONTINUOUS);

	if( haverangedrows )
	{
	  assert(rowrhs);
	  assert(rowrng);
	  assert(rowsen);
	  for ( i = 0; i < nr; ++i )
	    if( rowsen[i] == 'R' )
	      convertToRangedRow(i, rowrhs[i], rowrng[i]);
	}
}
 
//-----------------------------------------------------------------------------
// Read mps files
//-----------------------------------------------------------------------------
int OsiGrbSolverInterface::readMps( const char * filename,
				     const char * extension )
{
  debugMessage("OsiGrbSolverInterface::readMps(%s, %s)\n", filename, extension);

  // just call base class method
  return OsiSolverInterface::readMps(filename,extension);
}


//-----------------------------------------------------------------------------
// Write mps files
//-----------------------------------------------------------------------------
void OsiGrbSolverInterface::writeMps( const char * filename,
				      const char * extension,
				      double objSense ) const
{
  debugMessage("OsiGrbSolverInterface::writeMps(%s, %s, %g)\n", filename, extension, objSense);

  std::string f(filename);
  std::string e(extension);
  std::string fullname = f + "." + e;

  //TODO give row and column names?
  writeMpsNative(fullname.c_str(), NULL, NULL);

  // do not call Gurobi's MPS writer, because
  // 1. it only writes mps if the extension is mps
  // 2. the instance in Gurobi may have extra columns due to reformulated ranged rows, that may be confusing
//  GUROBI_CALL( "writeMps", GRBwrite(getMutableLpPtr(), const_cast<char*>(fullname.c_str())) );
}

//#############################################################################
// CPX specific public interfaces
//#############################################################################

GRBenv* OsiGrbSolverInterface::getEnvironmentPtr() const
{
  assert( localenv_ != NULL || globalenv_ != NULL );
  return localenv_ ? localenv_ : globalenv_;
}

GRBmodel* OsiGrbSolverInterface::getLpPtr( int keepCached )
{
  freeCachedData( keepCached );
  return getMutableLpPtr();
}

/// Return whether the current Gurobi environment runs in demo mode.
bool OsiGrbSolverInterface::isDemoLicense() const
{
  debugMessage("OsiGrbSolverInterface::isDemoLicense()\n");

  GRBenv* env = getEnvironmentPtr();

  GRBmodel* testlp;

  // a Gurobi demo license allows to solve only models with up to 500 variables
  // thus, let's try to create and solve a model with 1000 variables
  GUROBI_CALL( "isDemoLicense", GRBnewmodel(env, &testlp, "licensetest", 1000, NULL, NULL, NULL, NULL, NULL) );
//  GUROBI_CALL( "resolve", GRBsetintparam(GRBgetenv(testlp), GRB_INT_PAR_PRESOLVE, presolve) );

  int rc = GRBoptimize(testlp);

  if(rc == GRB_ERROR_SIZE_LIMIT_EXCEEDED)
    return true;

  GUROBI_CALL( "isDemoLicense", rc );

  GRBfreemodel(testlp);

  return false;
}

//-----------------------------------------------------------------------------

const char * OsiGrbSolverInterface::getCtype() const
{
  debugMessage("OsiGrbSolverInterface::getCtype()\n");

  return coltype_;
}

//#############################################################################
// Static instance counter methods
//#############################################################################

void OsiGrbSolverInterface::incrementInstanceCounter()
{
	if ( numInstances_ == 0 && !globalenv_)
	{
	  GUROBI_CALL( "incrementInstanceCounter", GRBloadenv( &globalenv_, NULL ) );
		assert( globalenv_ != NULL );
		globalenv_is_ours = true;
	}
  numInstances_++;
}

//-----------------------------------------------------------------------------

void OsiGrbSolverInterface::decrementInstanceCounter()
{
	assert( numInstances_ != 0 );
	assert( globalenv_ != NULL );
	numInstances_--;
	if ( numInstances_ == 0 && globalenv_is_ours)
	{
		GRBfreeenv( globalenv_ );
		globalenv_ = NULL;
	}
}

//-----------------------------------------------------------------------------

unsigned int OsiGrbSolverInterface::getNumInstances()
{
  return numInstances_;
}

void OsiGrbSolverInterface::setEnvironment(GRBenv* globalenv)
{
	if (numInstances_)
	{
		assert(globalenv_);
		throw CoinError("Cannot set global GUROBI environment, since some OsiGrb instance is still using it.", "setEnvironment", "OsiGrbSolverInterface", __FILE__, __LINE__);
	}
	
	assert(!globalenv_ || !globalenv_is_ours);
	
	globalenv_ = globalenv;
	globalenv_is_ours = false;
}


//#############################################################################
// Constructors, destructors clone and assignment
//#############################################################################

//-------------------------------------------------------------------
// Default Constructor 
//-------------------------------------------------------------------
OsiGrbSolverInterface::OsiGrbSolverInterface(bool use_local_env)
  : OsiSolverInterface(),
    localenv_(NULL),
    lp_(NULL),
    hotStartCStat_(NULL),
    hotStartCStatSize_(0),
    hotStartRStat_(NULL),
    hotStartRStatSize_(0),
    hotStartMaxIteration_(1000000), // ??? default iteration limit for strong branching is large
    nameDisc_(0),
    obj_(NULL),
    collower_(NULL),
    colupper_(NULL),
    rowsense_(NULL),
    rhs_(NULL),
    rowrange_(NULL),
    rowlower_(NULL),
    rowupper_(NULL),
    colsol_(NULL),
    rowsol_(NULL),
    redcost_(NULL),
    rowact_(NULL),
    matrixByRow_(NULL),
    matrixByCol_(NULL),
    probtypemip_(false),
    domipstart(false),
    colspace_(0),
    coltype_(NULL),
    nauxcols(0),
    auxcolspace(0),
    colmap_O2G(NULL),
    colmap_G2O(NULL),
    auxcolindspace(0),
    auxcolind(NULL)
{
  debugMessage("OsiGrbSolverInterface::OsiGrbSolverInterface()\n");
  
  if (use_local_env)
  {
    GUROBI_CALL( "OsiGrbSolverInterface", GRBloadenv( &localenv_, NULL ) );
		assert( localenv_ != NULL );
  }
  else
  	incrementInstanceCounter();
    
  gutsOfConstructor();
  
  // change Osi default to Gurobi default
  setHintParam(OsiDoDualInInitial,true,OsiHintTry);
}

OsiGrbSolverInterface::OsiGrbSolverInterface(GRBenv* localgrbenv)
  : OsiSolverInterface(),
    localenv_(localgrbenv),
    lp_(NULL),
    hotStartCStat_(NULL),
    hotStartCStatSize_(0),
    hotStartRStat_(NULL),
    hotStartRStatSize_(0),
    hotStartMaxIteration_(1000000), // ??? default iteration limit for strong branching is large
    nameDisc_(0),
    obj_(NULL),
    collower_(NULL),
    colupper_(NULL),
    rowsense_(NULL),
    rhs_(NULL),
    rowrange_(NULL),
    rowlower_(NULL),
    rowupper_(NULL),
    colsol_(NULL),
    rowsol_(NULL),
    redcost_(NULL),
    rowact_(NULL),
    matrixByRow_(NULL),
    matrixByCol_(NULL),
    probtypemip_(false),
    domipstart(false),
    colspace_(0),
    coltype_(NULL),
    nauxcols(0),
    auxcolspace(0),
    colmap_O2G(NULL),
    colmap_G2O(NULL),
    auxcolindspace(0),
    auxcolind(NULL)
{
  debugMessage("OsiGrbSolverInterface::OsiGrbSolverInterface()\n");

  if (localenv_ == NULL)
  { // if user called this constructor with NULL pointer, we assume that he meant that a local environment should be created
    GUROBI_CALL( "OsiGrbSolverInterface", GRBloadenv( &localenv_, NULL ) );
		assert( localenv_ != NULL );
  }
    
  gutsOfConstructor();
  
  // change Osi default to Gurobi default
  setHintParam(OsiDoDualInInitial,true,OsiHintTry);
}


//----------------------------------------------------------------
// Clone
//----------------------------------------------------------------
OsiSolverInterface * OsiGrbSolverInterface::clone(bool copyData) const
{
  debugMessage("OsiGrbSolverInterface::clone(%d)\n", copyData);

  return( new OsiGrbSolverInterface( *this ) );
}

//-------------------------------------------------------------------
// Copy constructor 
//-------------------------------------------------------------------
OsiGrbSolverInterface::OsiGrbSolverInterface( const OsiGrbSolverInterface & source )
  : OsiSolverInterface(source),
    localenv_(NULL),
    lp_(NULL),
    hotStartCStat_(NULL),
    hotStartCStatSize_(0),
    hotStartRStat_(NULL),
    hotStartRStatSize_(0),
    hotStartMaxIteration_(source.hotStartMaxIteration_),
    nameDisc_(source.nameDisc_),
    obj_(NULL),
    collower_(NULL),
    colupper_(NULL),
    rowsense_(NULL),
    rhs_(NULL),
    rowrange_(NULL),
    rowlower_(NULL),
    rowupper_(NULL),
    colsol_(NULL),
    rowsol_(NULL),
    redcost_(NULL),
    rowact_(NULL),
    matrixByRow_(NULL),
    matrixByCol_(NULL),
    probtypemip_(false),
    domipstart(false),
    colspace_(0),
    coltype_(NULL),
    nauxcols(0),
    auxcolspace(0),
    colmap_O2G(NULL),
    colmap_G2O(NULL),
    auxcolindspace(0),
    auxcolind(NULL)
{
  debugMessage("OsiGrbSolverInterface::OsiGrbSolverInterface(%p)\n", (void*)&source);

  if (source.localenv_) {
    GUROBI_CALL( "OsiGrbSolverInterface", GRBloadenv( &localenv_, NULL ) );
		assert( localenv_ != NULL );
  } else
  	incrementInstanceCounter();
  
  gutsOfConstructor();
  gutsOfCopy( source );
}


//-------------------------------------------------------------------
// Destructor 
//-------------------------------------------------------------------
OsiGrbSolverInterface::~OsiGrbSolverInterface()
{
  debugMessage("OsiGrbSolverInterface::~OsiGrbSolverInterface()\n");

  gutsOfDestructor();
  if (localenv_) {
		GRBfreeenv( localenv_ );
  } else
  	decrementInstanceCounter();
}

//----------------------------------------------------------------
// Assignment operator 
//-------------------------------------------------------------------
OsiGrbSolverInterface& OsiGrbSolverInterface::operator=( const OsiGrbSolverInterface& rhs )
{
  debugMessage("OsiGrbSolverInterface::operator=(%p)\n", (void*)&rhs);

  if (this != &rhs)
  {    
  	OsiSolverInterface::operator=( rhs );
  	gutsOfDestructor();
  	gutsOfConstructor();
  	if ( rhs.lp_ !=NULL )
  		gutsOfCopy( rhs );
  }
  
  return *this;
}

//#############################################################################
// Applying cuts
//#############################################################################

OsiSolverInterface::ApplyCutsReturnCode OsiGrbSolverInterface::applyCuts(const OsiCuts & cs,
        double effectivenessLb)
{
    debugMessage("OsiGrbSolverInterface::applyCuts(%p)\n", (void*)&cs);
  
    OsiSolverInterface::ApplyCutsReturnCode retVal;
    int i;

    // Loop once for each column cut
    for ( i = 0; i < cs.sizeColCuts(); i ++ ) {
        if ( cs.colCut(i).effectiveness() < effectivenessLb ) {
            retVal.incrementIneffective();
            continue;
        }
        if ( !cs.colCut(i).consistent() ) {
            retVal.incrementInternallyInconsistent();
            continue;
        }
        if ( !cs.colCut(i).consistent(*this) ) {
            retVal.incrementExternallyInconsistent();
            continue;
        }
        if ( cs.colCut(i).infeasible(*this) ) {
            retVal.incrementInfeasible();
            continue;
        }
        applyColCut( cs.colCut(i) );
        retVal.incrementApplied();
    }

    // Loop once for each row cut
    int nToApply = 0;
    size_t space = 0;

    for ( i = 0; i < cs.sizeRowCuts(); i ++ ) {
        if ( cs.rowCut(i).effectiveness() < effectivenessLb ) {
            retVal.incrementIneffective();
            continue;
        }
        if ( !cs.rowCut(i).consistent() ) {
            retVal.incrementInternallyInconsistent();
            continue;
        }
        if ( !cs.rowCut(i).consistent(*this) ) {
            retVal.incrementExternallyInconsistent();
            continue;
        }
        if ( cs.rowCut(i).infeasible(*this) ) {
            retVal.incrementInfeasible();
            continue;
        }
        nToApply++;
        space += cs.rowCut(i).row().getNumElements();

        retVal.incrementApplied();
    }

    int base_row = getNumRows();
    int base_col = getNumCols() + nauxcols;

    //now apply row cuts jointly
    int* start = new int[nToApply + 1];
    int* indices = new int[space];
    double* values = new double[space];
    double* lower = new double[nToApply];
    double* upper = new double[nToApply];

    int cur = 0;
    int r = 0;

    int nTrulyRanged = nToApply;
    for ( i = 0; i < cs.sizeRowCuts(); i ++ ) {

        if ( cs.rowCut(i).effectiveness() < effectivenessLb ) {
            continue;
        }
        if ( !cs.rowCut(i).consistent() ) {
            continue;
        }
        if ( !cs.rowCut(i).consistent(*this) ) {
            continue;
        }
        if ( cs.rowCut(i).infeasible(*this) ) {
            continue;
        }

        start[r] = cur;

        const OsiRowCut & rowCut = cs.rowCut(i);

        lower[r] = rowCut.lb();
        upper[r] = rowCut.ub();

        std::cerr << "range " << lower[r] << " - " << upper[r] << std::endl;

        if( lower[r] <= -getInfinity() ) {
            lower[r] = -GRB_INFINITY;
            nTrulyRanged--;
        } else if (upper[r] >= getInfinity() ) {
            upper[r] = GRB_INFINITY;
            nTrulyRanged--;
        }

        for (int k = 0; k < rowCut.row().getNumElements(); k++) {
            values[cur + k] = rowCut.row().getElements()[k];
            indices[cur + k] = rowCut.row().getIndices()[k];
        }

        r++;
        cur += rowCut.row().getNumElements();
    }
    start[nToApply] = cur;

    int nPrevVars = getNumCols();

    GUROBI_CALL( "applyRowCuts", GRBaddrangeconstrs( getLpPtr( OsiGrbSolverInterface::KEEPCACHED_COLUMN ),
                 nToApply, static_cast<int>(space), start, indices, values, lower, upper, NULL) );

    GUROBI_CALL( "applyRowCuts", GRBupdatemodel(getMutableLpPtr()) );

    // store correspondence between ranged rows and new variables added by Gurobi
    int nNewVars = getNumCols() - nPrevVars;

    if (nNewVars != nTrulyRanged) {

        std::cerr << "ERROR in applying cuts for Gurobi: " << __FILE__ << " : " << "line "
                  << __LINE__ << " . Exiting" << std::endl;
        exit(-1);
    }

    if (nTrulyRanged > 0) {
        if( nauxcols == 0 ) {
            // this is the first ranged row
            assert(colmap_O2G == NULL);
            assert(colmap_G2O == NULL);

            assert(colspace_ >= getNumCols());

            auxcolspace = nTrulyRanged;
            colmap_G2O = new int[colspace_ + auxcolspace];
            CoinIotaN(colmap_G2O, colspace_ + auxcolspace, 0);

            colmap_O2G = CoinCopyOfArray(colmap_G2O, colspace_);
        } else {
            assert(colmap_O2G != NULL);
            assert(colmap_G2O != NULL);
            resizeAuxColSpace( nauxcols + nTrulyRanged);
        }

        nauxcols += nTrulyRanged;

        resizeAuxColIndSpace();

        int next = 0;
        for (int k = 0; k < nToApply; k++) {

            if (lower[k] > -GRB_INFINITY && upper[k] < GRB_INFINITY) {
                auxcolind[base_row + next] = base_col + next;
                colmap_G2O[auxcolind[base_row + next]] = - (base_row + next) - 1;
                next++;
            }
        }
    }

    delete[] start;
    delete[] indices;
    delete[] values;
    delete[] lower;
    delete[] upper;

    return retVal;
}

void OsiGrbSolverInterface::applyColCut( const OsiColCut & cc )
{
	debugMessage("OsiGrbSolverInterface::applyColCut(%p)\n", (void*)&cc);

	const double * gurobiColLB = getColLower();
	const double * gurobiColUB = getColUpper();
	const CoinPackedVector & lbs = cc.lbs();
	const CoinPackedVector & ubs = cc.ubs();
	int i;

	for( i = 0; i < lbs.getNumElements(); ++i )
		if ( lbs.getElements()[i] > gurobiColLB[lbs.getIndices()[i]] )
			setColLower( lbs.getIndices()[i], lbs.getElements()[i] );
	for( i = 0; i < ubs.getNumElements(); ++i )
		if ( ubs.getElements()[i] < gurobiColUB[ubs.getIndices()[i]] )
			setColUpper( ubs.getIndices()[i], ubs.getElements()[i] );
}

//-----------------------------------------------------------------------------

void OsiGrbSolverInterface::applyRowCut( const OsiRowCut & rowCut )
{
  debugMessage("OsiGrbSolverInterface::applyRowCut(%p)\n", (void*)&rowCut);

  double rhs = 0.0;
  char sns;
  double lb = rowCut.lb();
  double ub = rowCut.ub();
  bool isrange = false;
  if( lb <= -getInfinity() && ub >= getInfinity() )   // free constraint
  {
  	rhs = getInfinity();
  	sns = GRB_LESS_EQUAL;
  }
  else if( lb <= -getInfinity() )  // <= constraint
  {
  	rhs = ub;
  	sns = GRB_LESS_EQUAL;
  }
  else if( ub >= getInfinity() )  // >= constraint
  {
  	rhs = lb;
  	sns = GRB_GREATER_EQUAL;
  }
  else if( ub == lb )  // = constraint
  {
  	rhs = ub;
  	sns = GRB_EQUAL;
  }
  else  // range constraint
  {
    rhs = 0.0;
    sns = GRB_EQUAL;
    isrange = true;
  }
  
  GUROBI_CALL( "applyRowCut", GRBaddconstr( getLpPtr( OsiGrbSolverInterface::KEEPCACHED_COLUMN ),
  	rowCut.row().getNumElements(),
		const_cast<int*>( rowCut.row().getIndices() ), 
		const_cast<double*>( rowCut.row().getElements() ),
		sns, rhs, NULL) );

  if( isrange )
  {
    convertToRangedRow(getNumRows()-1, ub, ub - lb);
  }
  else if( nauxcols )
    resizeAuxColIndSpace();
}

//#############################################################################
// Private methods (non-static and static) and static data
//#############################################################################

//------------------------------------------------------------------
// Static data
//------------------------------------------------------------------      
GRBenv* OsiGrbSolverInterface::globalenv_ = NULL;
bool OsiGrbSolverInterface::globalenv_is_ours = true;
unsigned int OsiGrbSolverInterface::numInstances_ = 0;
 
//-------------------------------------------------------------------
// Get pointer to GRBmodel*.
// const methods should use getMutableLpPtr().
// non-const methods should use getLpPtr().
//------------------------------------------------------------------- 
GRBmodel* OsiGrbSolverInterface::getMutableLpPtr() const
{
  if ( lp_ == NULL )
  {
    assert(getEnvironmentPtr() != NULL);

    GUROBI_CALL( "getMutableLpPtr", GRBnewmodel(getEnvironmentPtr(), &lp_, "OsiGrb_problem", 0, NULL, NULL, NULL, NULL, NULL) );
    assert( lp_ != NULL );

    // GUROBI up to version 2.0.1 may return a scaled LP after GRBoptimize when requesting it via GRBgetvars
#if (GRB_VERSION_MAJOR < 2) || (GRB_VERSION_MAJOR == 2 && GRB_VERSION_MINOR == 0 && GRB_VERSION_TECHNICAL <= 1)
    GUROBI_CALL( "getMutableLpPtr", GRBsetintparam(GRBgetenv(lp_), GRB_INT_PAR_SCALEFLAG, 0) );
#endif
  }
  return lp_;
}

//-------------------------------------------------------------------

void OsiGrbSolverInterface::gutsOfCopy( const OsiGrbSolverInterface & source )
{
  // Set Rim and constraints
  const double* obj = source.getObjCoefficients();
  const double* rhs = source.getRightHandSide();
  const char* sense = source.getRowSense();
  const CoinPackedMatrix * cols = source.getMatrixByCol();
  const double* lb = source.getColLower();
  const double* ub = source.getColUpper();
  loadProblem(*cols,lb,ub,obj,sense,rhs,source.getRowRange());

  // Set Objective Sense
  setObjSense(source.getObjSense());

  // Set Objective Constant
  double dblParam;
  source.getDblParam(OsiObjOffset,dblParam) ;
  setDblParam(OsiObjOffset,dblParam) ;

  // Set MIP information
  resizeColSpace(source.colspace_);
  CoinDisjointCopyN( source.coltype_, source.colspace_, coltype_ );
  
  // Set Problem name
  std::string strParam;
  source.getStrParam(OsiProbName,strParam) ;
  setStrParam(OsiProbName,strParam) ;

  // Set Solution - not supported by Gurobi
//  setColSolution(source.getColSolution());
//  setRowPrice(source.getRowPrice());
}

//-------------------------------------------------------------------
void OsiGrbSolverInterface::gutsOfConstructor()
{ }

//-------------------------------------------------------------------
void OsiGrbSolverInterface::gutsOfDestructor()
{  
  debugMessage("OsiGrbSolverInterface::gutsOfDestructor()\n");

  if ( lp_ != NULL )
  {
    GUROBI_CALL( "gutsOfDestructor", GRBfreemodel(lp_) );
  	lp_ = NULL;
  	freeAllMemory();
  }
  assert( lp_==NULL );
  assert( obj_==NULL );
  assert( collower_==NULL );
  assert( colupper_==NULL );
  assert( rowsense_==NULL );
  assert( rhs_==NULL );
  assert( rowrange_==NULL );
  assert( rowlower_==NULL );
  assert( rowupper_==NULL );
  assert( colsol_==NULL );
  assert( rowsol_==NULL );
  assert( redcost_==NULL );
  assert( rowact_==NULL );
  assert( matrixByRow_==NULL );
  assert( matrixByCol_==NULL );
  assert( coltype_==NULL );
  assert( colspace_==0 );
  assert( auxcolspace == 0);
  assert( auxcolindspace == 0 );
}

//-------------------------------------------------------------------
/// free cached vectors

void OsiGrbSolverInterface::freeCachedColRim()
{
  freeCacheDouble( obj_ );  
  freeCacheDouble( collower_ ); 
  freeCacheDouble( colupper_ ); 
  assert( obj_==NULL );
  assert( collower_==NULL );
  assert( colupper_==NULL );
}

void OsiGrbSolverInterface::freeCachedRowRim()
{
  freeCacheChar( rowsense_ );
  freeCacheDouble( rhs_ );
  freeCacheDouble( rowrange_ );
  freeCacheDouble( rowlower_ );
  freeCacheDouble( rowupper_ );
  assert( rowsense_==NULL ); 
  assert( rhs_==NULL ); 
  assert( rowrange_==NULL ); 
  assert( rowlower_==NULL ); 
  assert( rowupper_==NULL );
 }

void OsiGrbSolverInterface::freeCachedMatrix()
{
  freeCacheMatrix( matrixByRow_ );
  freeCacheMatrix( matrixByCol_ );
  assert( matrixByRow_==NULL ); 
  assert( matrixByCol_==NULL ); 
}

void OsiGrbSolverInterface::freeCachedResults()
{
  freeCacheDouble( colsol_ ); 
  freeCacheDouble( rowsol_ );
  freeCacheDouble( redcost_ );
  freeCacheDouble( rowact_ );
  assert( colsol_==NULL );
  assert( rowsol_==NULL );
  assert( redcost_==NULL );
  assert( rowact_==NULL );
}


void OsiGrbSolverInterface::freeCachedData( int keepCached )
{
  if( !(keepCached & OsiGrbSolverInterface::KEEPCACHED_COLUMN) )
    freeCachedColRim();
  if( !(keepCached & OsiGrbSolverInterface::KEEPCACHED_ROW) )
    freeCachedRowRim();
  if( !(keepCached & OsiGrbSolverInterface::KEEPCACHED_MATRIX) )
    freeCachedMatrix();
  if( !(keepCached & OsiGrbSolverInterface::KEEPCACHED_RESULTS) )
    freeCachedResults();
}

void OsiGrbSolverInterface::freeAllMemory()
{
  freeCachedData();
  if( hotStartCStat_ != NULL )
    delete[] hotStartCStat_;
  if( hotStartRStat_ != NULL )
    delete[] hotStartRStat_;
  hotStartCStat_     = NULL;
  hotStartCStatSize_ = 0;
  hotStartRStat_     = NULL;
  hotStartRStatSize_ = 0;
  freeColSpace();
  delete[] auxcolind;
  auxcolind = NULL;
  auxcolindspace = 0;
}

/// converts a normal row into a ranged row by adding an auxiliary variable
void OsiGrbSolverInterface::convertToRangedRow(int rowidx, double rhs, double range)
{
  debugMessage("OsiGrbSolverInterface::convertToRangedRow()\n");

  assert(rowidx >= 0);
  assert(rowidx < getNumRows());
  assert(rhs > -getInfinity());
  assert(rhs <  getInfinity());
  assert(range > 0.0);
  assert(range < getInfinity());

  if( nauxcols == 0 )
  { // row rowidx is the first ranged row
    assert(colmap_O2G == NULL);
    assert(colmap_G2O == NULL);

    assert(colspace_ >= getNumCols());

    auxcolspace = 1;
    colmap_G2O = new int[colspace_ + auxcolspace];
    CoinIotaN(colmap_G2O, colspace_ + auxcolspace, 0);

    colmap_O2G = CoinCopyOfArray(colmap_G2O, colspace_);
  }
  else
  {
    assert(colmap_O2G != NULL);
    assert(colmap_G2O != NULL);
    resizeAuxColSpace( nauxcols + 1);
  }

  resizeAuxColIndSpace();
  assert(auxcolind[rowidx] == -1); /* not a ranged row yet */

  GUROBI_CALL( "convertToRangedRow", GRBupdatemodel(getMutableLpPtr()) );

  double minusone = -1.0;
  GUROBI_CALL( "convertToRangedRow", GRBaddvar(getLpPtr( OsiGrbSolverInterface::KEEPCACHED_PROBLEM ),
      1, &rowidx, &minusone,
      0.0, rhs-range, rhs, GRB_CONTINUOUS, NULL) );

  auxcolind[rowidx] = getNumCols() + nauxcols - 1;
  colmap_G2O[auxcolind[rowidx]] = -rowidx - 1;

  ++nauxcols;

#ifndef NDEBUG
  if( nauxcols )
  {
    int nc = getNumCols();
    int nr = getNumRows();
    for( int i = 0; i < nc + nauxcols; ++i )
    {
      assert(i >= nc || colmap_G2O[colmap_O2G[i]] == i);
      assert(colmap_G2O[i] <  0 || colmap_O2G[colmap_G2O[i]] == i);
      assert(colmap_G2O[i] >= 0 || -colmap_G2O[i]-1 < nr);
      assert(colmap_G2O[i] >= 0 || auxcolind[-colmap_G2O[i]-1] == i);
    }
  }
#endif
}

/// converts a ranged row into a normal row by removing its auxiliary variable
void OsiGrbSolverInterface::convertToNormalRow(int rowidx, char sense, double rhs)
{
  debugMessage("OsiGrbSolverInterface::convertToNormalRow()\n");

  assert(rowidx >= 0);
  assert(rowidx < getNumRows());

  int auxvar = auxcolind[rowidx];

  assert(nauxcols);
  assert(auxvar >= 0);
  assert(colmap_G2O[auxvar] == -rowidx - 1);

  GUROBI_CALL( "convertToNormalRow", GRBupdatemodel(getMutableLpPtr()) );

  /* row rowidx should be an ordinary equality row with rhs == 0 now */
  GUROBI_CALL( "convertToNormalRow", GRBdelvars(getLpPtr( OsiGrbSolverInterface::KEEPCACHED_ROW ), 1, &auxcolind[rowidx]) );

  auxcolind[rowidx] = -1;

  if( nauxcols > 1 )
  {
    int nc = getNumCols();
    for(int i = auxvar; i < nc; ++i)
    {
      if( colmap_O2G[i] > auxvar )
        --colmap_O2G[i];
      colmap_G2O[i] = colmap_G2O[i+1];
    }
    for(int i = nc; i < nc + nauxcols; ++i)
      colmap_G2O[i] = colmap_G2O[i+1];

    int nr = getNumRows();
    for(int i = 0; i < nr; ++i)
      if( auxcolind[i] > auxvar )
        --auxcolind[i];
    --nauxcols;
  }
  else
  { // no ranged rows left
    delete[] colmap_O2G;
    delete[] colmap_G2O;
    delete[] auxcolind;
    auxcolspace = 0;
    auxcolindspace = 0;
    nauxcols = 0;
  }

  setRowType(rowidx, sense, rhs, 0.0);

#ifndef NDEBUG
  if( nauxcols )
  {
    int nc = getNumCols();
    int nr = getNumRows();
    for( int i = 0; i < nc + nauxcols; ++i )
    {
      assert(i >= nc || colmap_G2O[colmap_O2G[i]] == i);
      assert(colmap_G2O[i] <  0 || colmap_O2G[colmap_G2O[i]] == i);
      assert(colmap_G2O[i] >= 0 || -colmap_G2O[i]-1 < nr);
      assert(colmap_G2O[i] >= 0 || auxcolind[-colmap_G2O[i]-1] == i);
    }
  }
#endif
}

//#############################################################################
// Resets as if default constructor
void 
OsiGrbSolverInterface::reset()
{
  setInitialData(); // clear base class
  gutsOfDestructor();
}

/**********************************************************************/
/* Returns 1 if can just do getBInv etc
   2 if has all OsiSimplex methods
   and 0 if it has none */
int OsiGrbSolverInterface::canDoSimplexInterface() const {
  return 0;
}

/**********************************************************************/
bool OsiGrbSolverInterface::basisIsAvailable() const {
  if (getNumCols() == 0)
  	return true;

  GUROBI_CALL( "basisIsAvailable", GRBupdatemodel(getMutableLpPtr()) );

  int status;
  GUROBI_CALL( "basisIsAvailable", GRBgetintattr(getMutableLpPtr(), GRB_INT_ATTR_STATUS, &status) );

	if (status == GRB_LOADED || status == GRB_INFEASIBLE || status == GRB_INF_OR_UNBD || status == GRB_UNBOUNDED)
		return false;
	
  int dum;
  return GRBgetintattrelement(getMutableLpPtr(), GRB_INT_ATTR_VBASIS, 0, &dum) == 0;
}

/* Osi return codes:
0: free  
1: basic  
2: upper 
3: lower
*/
void OsiGrbSolverInterface::getBasisStatus(int* cstat, int* rstat) const {

	int numcols = getNumCols();
	int numrows = getNumRows();

	GUROBI_CALL( "getBasisStatus", GRBupdatemodel(getMutableLpPtr()) );

  if( nauxcols )
    GUROBI_CALL( "getBasisStatus", GRBgetintattrlist(getMutableLpPtr(), GRB_INT_ATTR_VBASIS, numcols, colmap_O2G, cstat) );
  else
    GUROBI_CALL( "getBasisStatus", GRBgetintattrarray(getMutableLpPtr(), GRB_INT_ATTR_VBASIS, 0, numcols, cstat) );
	
	for (int i = 0; i < numcols; ++i)
		switch (cstat[i])
		{
			case GRB_BASIC:
				cstat[i] = 1;
				break;
			case GRB_NONBASIC_LOWER:
				cstat[i] = 3;
				break;
			case GRB_NONBASIC_UPPER:
				cstat[i] = 2;
				break;
			case GRB_SUPERBASIC:
				cstat[i] = 0;
				break;
		}

	GUROBI_CALL( "getBasisStatus", GRBgetintattrarray(getMutableLpPtr(), GRB_INT_ATTR_CBASIS, 0, numrows, rstat) );

	char sense;
	for (int i = 0; i < numrows; ++i)
	{
	  if( nauxcols && auxcolind[i] >= 0 )
	  { /* if the row is a ranged row, then we take the basis status from the corresponding auxiliary column
	       is this correct??? */
	    int auxcolstat;
	    GUROBI_CALL( "getBasisStatus", GRBgetintattrelement(getMutableLpPtr(), GRB_INT_ATTR_VBASIS, auxcolind[i], &auxcolstat) );
	    switch( auxcolstat )
	    {
	      case GRB_BASIC:
	        rstat[i] = 1;
	        break;
	      case GRB_NONBASIC_LOWER:
	        rstat[i] = 3;
	        break;
	      case GRB_NONBASIC_UPPER:
	        rstat[i] = 2;
	        break;
	      case GRB_SUPERBASIC:
	        rstat[i] = 0;
	        break;
	    }
	  }
	  else
	  {
	    switch (rstat[i])
	    {
         case GRB_SUPERBASIC:
	        rstat[i] = 0;
	        break;
	      case GRB_BASIC:
	        rstat[i] = 1;
	        break;
	      case GRB_NONBASIC_LOWER:
	      case GRB_NONBASIC_UPPER:
	        GUROBI_CALL( "getBasisStatus", GRBgetcharattrelement(getMutableLpPtr(), GRB_CHAR_ATTR_SENSE, i, &sense) );
	        rstat[i] = (sense == '>' ? 2 : 3);
	        break;
	    }
	  }
	}
}
