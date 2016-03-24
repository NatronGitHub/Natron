//-----------------------------------------------------------------------------
// name:     OSI Interface for GLPK
//-----------------------------------------------------------------------------
// Copyright (C) 2001, 2002 Vivian De Smedt
// Copyright (C) 2002, 2003 Braden Hunsaker 
// Copyright (C) 2003, 2004 University of Pittsburgh
// Copyright (C) 2004 Joseph Young
// Copyright (C) 2007 Lou Hafer
//    University of Pittsburgh coding done by Brady Hunsaker
// All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).
//
// Comments:
//   
//    Areas that may need work in the code are marked with '???'.
//
//    As of version 4.7, GLPK problems can be of class LPX_LP or LPX_MIP.
// The difference is that LPX_MIP problems have a separate MIP data block, 
// and certain functions are only allowed for LPX_MIP problems.
//
//    In (much) earlier versions of GLPK, if an LPX_MIP problem was
// changed back into a LPX_LP problem, then the MIP data was lost,
// including which columns are integer.  However, LPX_MIP problems
// still had access to LP information (like lpx_get_status).
//
//    It appears that this behavior is no longer true in version 4.7.
// Therefore it may be worthwhile to adjust the interface to change
// the class of the problem as needed.  For the time being, however,
// all OSI problems are set to type LPX_MIP.  The only trick is
// differentiating when the user calls status routines like
// isProvenOptimal().  For now, we assume that the user is referring
// to the most recent solution call.  We add an extra variable,
// bbWasLast_, to the class to record which solution call was most
// recent (lp or mip).
//
// Possible areas of improvement
// -----------------------------
//
// Methods that are not implemented:
//
//  getPrimalRays, getDualRays
//
// Methods that are implemented, but do not do what you probably expect:
//
//  setColSolution, setRowPrice  
//
// Many methods have not been attempted to be improved (for speed) 
// to take advantage of how GLPK works.  The emphasis has been on
// making things functional and not too messy.  There's plenty of room
// for more efficient implementations.
//

/*
  Various bits and pieces of knowledge that are handy when working on
  OsiGlpk.

  Glpk uses 1-based indexing. Be careful.

  Primal and dual solutions may be set by the user. This means that
  getColSolution and getRowPrice are obliged to return the cached solution
  vector whenever it's available. Various other routines which calculate
  values based on primal and dual solutions must also use the cached
  vectors.  On the other hand, we don't want to be rebuilding the cache with
  every call to the solver. The approach is that a call to the solver removes
  the cached values. Subsequent calls to retrieve solution values will
  repopulate the cache by interrogating glpk.
*/

#include <cassert>
#include <cstdio>
#include <cmath>
#include <string>
#include <iostream>

extern "C" {
#include "glpk.h"
}
#ifndef GLP_PROB_DEFINED
#define GLP_PROB_DEFINED
#endif

#include "CoinError.hpp"
#include "CoinPragma.hpp"

#include "OsiConfig.h"

#include "OsiGlpkSolverInterface.hpp"
#include "OsiRowCut.hpp"
#include "OsiColCut.hpp"
#include "CoinPackedMatrix.hpp"
#include "CoinWarmStartBasis.hpp"

/*
  Grab the COIN standard finite infinity from CoinFinite.hpp. Also set a
  zero tolerance, so we can set clean zeros in computed reduced costs and
  row activities. The value 1.0e-9 is lifted from glpk --- this is the value
  that it uses when the LPX_K_ROUND parameter is active.
*/
#include "CoinFinite.hpp"
namespace {
  const double CoinInfinity = COIN_DBL_MAX ;
  const double GlpkZeroTol = 1.0e-9 ;
}

/* Cut names down to a reasonable size. */

#define OGSI OsiGlpkSolverInterface

/*
  A few defines to control execution tracing.

  OGSI_TRACK_SOLVERS	track creation, copy, deletion of OGSI objects
  OGSI_TRACK_FRESH	track how a solution is made stale/fresh
  OGSI_VERBOSITY	enables warnings when > 0; probably should be converted
			to honest messages for message handler
*/

#define OGSI_TRACK_SOLVERS 0
#define OGSI_TRACK_FRESH 0
#define OGSI_VERBOSITY 0

//#############################################################################
// File-local methods
//#############################################################################

namespace {

/*
  Convenience routine for generating error messages. Who knows, maybe it'll get
  used eventually  :-).
*/
inline void checkGLPKerror (int err, std::string glpkfuncname,
			    std::string osimethod)
{ if (err != 0)
  { char s[100] ;
    sprintf(s,"%s returned error %d",glpkfuncname.c_str(),err) ;
    std::cout
      << "ERROR: " << s
      << " (" << osimethod << " in OsiGlpkSolverInterface)" << std::endl ;
    throw CoinError(s,osimethod.c_str(),"OsiGlpkSolverInterface") ; }

  return ; }

} // end file-local namespace

//#############################################################################
// Solve methods
//#############################################################################

/*
  Solve an LP from scratch.
*/
void OGSI::initialSolve()
{ 
# if OGSI_TRACK_FRESH > 0
  std::cout
    << "OGSI(" << std::hex << this << std::dec
    << ")::initialSolve." << std::endl ;
# endif

  LPX *model = getMutableModelPtr() ;
/*
  Prep. Toss any cached solution information.
*/
  freeCachedData(OGSI::FREECACHED_RESULTS) ;
/*
  Solve the lp.
*/
  int err = lpx_simplex(model) ;

  // for Glpk, a solve fails if the initial basis is invalid or singular
  // thus, we construct a (advanced) basis first and try again
#ifdef LPX_E_BADB
  if (err == LPX_E_BADB) {
    lpx_adv_basis(model);
    err = lpx_simplex(model) ;
  } else
#endif
  if (err == LPX_E_SING || err == LPX_E_FAULT) {
    lpx_adv_basis(model);
    err = lpx_simplex(model) ;
  }

  iter_used_ = lpx_get_int_parm(model, LPX_K_ITCNT) ;
/*
  Sort out the various state indications.

  When the presolver is turned on, lpx_simplex() will not be able to tell
  whether the objective function has hit it's upper or lower limit, and does
  not return OBJLL or OBJUL. The code for these cases should be beefed up to
  check the objective against the limit.

  The codes NOPFS and NODFS are returned only when the presolver is used.

  If we ever reach the default case, we're deeply confused.
*/
  isIterationLimitReached_ = false ;
  isTimeLimitReached_ = false;
  isAbandoned_ = false ;
  isPrimInfeasible_ = false ;
  isDualInfeasible_ = false ;
  isFeasible_ = false ;
  isObjLowerLimitReached_ = false ;
  isObjUpperLimitReached_ = false ;

  switch (err)
  { case LPX_E_OK:
    { break ; }
    case LPX_E_ITLIM:
    { isIterationLimitReached_ = true ;
      break ; }
    case LPX_E_OBJLL:
    { isObjLowerLimitReached_ = true ;
      break ; }
    case LPX_E_OBJUL:
    { isObjUpperLimitReached_ = true ;
      break ; }
    case LPX_E_TMLIM:
    { isTimeLimitReached_ = true ;
    } // no break here, so we still report abandoned
    case LPX_E_FAULT:
    case LPX_E_SING:
#ifdef LPX_E_BADB
    case LPX_E_BADB:
#endif
    { isAbandoned_ = true ;
      break ; }
    case LPX_E_NOPFS:
    { isPrimInfeasible_ = true ;
      break ; }
    case LPX_E_NODFS:
    { isDualInfeasible_ = true ;
      break ; }
    default:
    { assert(false) ; } }
    
  switch (lpx_get_status(model))
  { case LPX_OPT:
  	case LPX_FEAS:
  	{ isFeasible_ = true ;
  		break ; }
  	default:
  	{ }
  }
	  
  // Record that simplex was most recent
  bbWasLast_ = 0 ;

  return ; }

//-----------------------------------------------------------------------------

void OGSI::resolve()
{
# if OGSI_TRACK_FRESH > 0
  std::cout
    << "OGSI(" << std::hex << this << std::dec
    << ")::resolve." << std::endl ;
# endif

  LPX *model = getMutableModelPtr() ;
  freeCachedData(OGSI::FREECACHED_RESULTS) ;

  // lpx_simplex will use the current basis if possible
  int err = lpx_simplex(model) ;

  // for Glpk, a solve fails if the initial basis is invalid or singular
  // thus, we construct a (advanced) basis first and try again
#ifdef LPX_E_BADB
  if (err == LPX_E_BADB) {
    lpx_adv_basis(model);
    err = lpx_simplex(model) ;
  } else
#endif
  if (err == LPX_E_SING || err == LPX_E_FAULT) {
    lpx_adv_basis(model);
    err = lpx_simplex(model) ;
  }
  
  iter_used_ = lpx_get_int_parm(model,LPX_K_ITCNT) ;

  isIterationLimitReached_ = false ;
  isTimeLimitReached_ = false ;
  isAbandoned_ = false ;
  isObjLowerLimitReached_ = false ;
  isObjUpperLimitReached_ = false ;
  isPrimInfeasible_ = false ;
  isDualInfeasible_ = false ;
  isFeasible_ = false ;

  switch (err)
  { case LPX_E_OK:
    { break ; }
    case LPX_E_ITLIM:
    { isIterationLimitReached_ = true ;
      break ; }
    case LPX_E_OBJLL:
    { isObjLowerLimitReached_ = true ;
      break ; }
    case LPX_E_OBJUL:
    { isObjUpperLimitReached_ = true ;
      break ; }
    case LPX_E_TMLIM:
    { isTimeLimitReached_ = true ;
    } // no break here, so we still report abandoned
    case LPX_E_FAULT:
    case LPX_E_SING:
#ifdef LPX_E_BADB
    case LPX_E_BADB:
#endif
    { isAbandoned_ = true ;
      break ; }
    case LPX_E_NOPFS:
    { isPrimInfeasible_ = true ;
      break ; }
    case LPX_E_NODFS:
    { isDualInfeasible_ = true ;
      break ; }
    default:
    { assert(false) ; } }

  switch (lpx_get_status(model))
  { case LPX_OPT:
  	case LPX_FEAS:
  	{ isFeasible_ = true ;
  		break ; }
  	default:
  	{ }
  }

  // Record that simplex was most recent
  bbWasLast_ = 0 ;

  return ; }

//-----------------------------------------------------------------------------

/*
  Call glpk's built-in MIP solver. Any halfway recent version (from glpk 4.4,
  at least) will have lpx_intopt, a branch-and-cut solver. The presence of cut
  generators is more recent.
*/
void OGSI::branchAndBound ()

{ LPX *model = getMutableModelPtr() ;
/*
  Destroy cached data.
*/
  freeCachedData(OGSI::FREECACHED_RESULTS) ;
/*
  Assuming we have integer variables in the model, call the best MIP solver
  we can manage.

  lpx_intopt does not require an optimal LP solution as a starting point, so
  we can call it directly.

  lpx_integer needs an initial optimal solution to the relaxation.
*/
  if (lpx_get_num_int(model))
  { int err = LPX_E_FAULT ;

#   ifdef GLPK_HAS_INTOPT
    err = lpx_intopt(model) ;
#   else
    if (lpx_get_status(model) != LPX_OPT)
    { initialSolve() ; }
    err = lpx_integer(model) ;
#   endif
/*
  We have a result. What is it? Start with a positive attitude and revise as
  needed. The various LPX_E_* and LPX_I_* defines are stable back as far as
  glpk-4.4.

  When we get LPX_E_OK (MIP terminated normally), we need to look more
  closely.  LPX_I_OPT indicates a proven optimal integer solution.
  LPX_I_NOFEAS indicates that there is no integer feasible solution.
  LPX_I_UNDEF says the MIP solution is undefined. LPX_I_FEAS says that in
  integer feasible solution was found but not proven optimal (termination of
  search due to some limit is the common cause). It's not clear what to do
  with LPX_I_UNDEF; currently, it is not really reflected in
  termination status. LPX_I_FEAS is reflected by the OsiGlpk specific method
  isFeasible().

  Various other codes are returned by lpx_intopt (lpx_integer returns a
  subset of these).  LPX_E_NOPFS (LPX_E_NODFS) indicate no primal (dual)
  feasible solution; detected at the root, either by presolve or when
  attempting the root relaxation. LPX_E_ITLIM (LPX_E_TMLIM) indicate
  iteration (time) limit reached. Osi doesn't provide for time limit, so lump
  it in with iteration limit (an arguable choice, but seems better than the
  alternatives) and have extra method isTimeLimitReached().
  LPX_E_SING (lp solver failed due to singular basis) is
  legimately characterised as abandoned.  LPX_E_FAULT indicates a structural
  problem (problem not of class MIP, or an integer variable has a non-integer
  bound), which really translates into internal confusion in OsiGlpk.

  Previous comments expressed uncertainty about the iteration count. This
  should be checked at some point. -- lh, 070709 --
*/
    iter_used_ = lpx_get_int_parm(model,LPX_K_ITCNT) ;
    isIterationLimitReached_ = false ;
		isTimeLimitReached_ = false ;
    isAbandoned_ = false ;
    isPrimInfeasible_ = false ;
    isDualInfeasible_ = false ;
    isFeasible_ = false ;
    isObjLowerLimitReached_ = false ;
    isObjUpperLimitReached_ = false ;

    switch (err)
    { case LPX_E_OK:
#ifdef LPX_E_MIPGAP
    case LPX_E_MIPGAP:
#endif
      { 
	break ; }
      case LPX_E_NOPFS:
      { isPrimInfeasible_ = true ;
	break ; }
      case LPX_E_NODFS:
      { isDualInfeasible_ = true ;
	break ; }
      case LPX_E_TMLIM:
      { isTimeLimitReached_ = true ; } // no break
      case LPX_E_ITLIM:
      { isIterationLimitReached_ = true ;
	break ; }
      case LPX_E_SING:
      { isAbandoned_ = true ;
	break ; }
      case LPX_E_FAULT:
      { assert(false) ;
	break ; }
      default:
      { assert(false) ;
	break ; } }
	
	//check this also if err!=LPX_E_OPT, so we know about feasibility in case time/resource limit is reached  
	int mip_status = lpx_mip_status(model) ;
	switch (mip_status)
	{ case LPX_I_OPT:
	  { isFeasible_ = true ;
	  	break ; }
	  case LPX_I_NOFEAS:
	  { isPrimInfeasible_ = true ;
	    break ; }
	  case LPX_I_UNDEF:
	  { break ; }
	  case LPX_I_FEAS:
	  { isFeasible_ = true ;
	  	break ; }
	  default:
	  { assert(false) ;
	    break ; } }
/*
  The final action is to note that our last call to glpk was the MIP solver.
*/
    bbWasLast_ = 1 ; }
/*
  Not a MIP (no integer variables). Call the LP solver. Since we can call
  branchAndBound with no initial LP solution, initialSolve is appropriate here.
  (But for glpk, it actually makes no difference --- lpx_simplex makes the
  decision on how to proceed.)
*/
  else
  { initialSolve() ; }

  return ; }

//#############################################################################
// Parameter related methods
//#############################################################################

/*
  When we set parameters, we have to stash them locally as well as pushing
  them into the LPX object. The trouble comes when loading a new problem. If
  the LPX object is already loaded with a problem, the easiest approach is to
  simply delete it and start fresh. But then we lose all the parameters that
  are held in the LPX object. By keeping a copy, we have a way to recover.

  Really, we should be keeping these values in the OSI base class arrays, but
  the amount of storage involved is too small to provide motivation to change
  from the present setup.
*/

bool OGSI::setIntParam (OsiIntParam key, int value)

{ bool retval = false ;

  switch (key)
  { case OsiMaxNumIteration:
    { if (value >= 0)
      { maxIteration_ = value ;
	lpx_set_int_parm(lp_,LPX_K_ITLIM,value) ;
	retval = true ; }
      else
      { retval = false ; }
      break ; }
    case OsiMaxNumIterationHotStart:
    { if (value >= 0)
      { hotStartMaxIteration_ = value ;
	retval = true ; }
      else
      {	retval = false ; }
      break ; }
    case OsiNameDiscipline:
    { if (value < 0 || value > 3)
      { retval = false ; }
      else
      { nameDisc_ = value ;
	retval = true ; }
      break ; }
    case OsiLastIntParam:
    { retval = false ;
      break ; } }

  return (retval) ; }

//-----------------------------------------------------------------------------

/*
  Self-protection is required here --- glpk will fault if handed a value
  it doesn't like.
*/

bool OGSI::setDblParam (OsiDblParam key, double value)

{ bool retval = false ;

  switch (key)
  { case OsiDualObjectiveLimit:
    // as of 4.7, GLPK only uses this if it does dual simplex
    { dualObjectiveLimit_ = value ;
      if (getObjSense() == 1)				// minimization
      { lpx_set_real_parm(lp_,LPX_K_OBJUL,value) ; }
      else              				// maximization
      { lpx_set_real_parm(lp_,LPX_K_OBJLL,value) ; }
      retval = true ; 
      break ; }
    case OsiPrimalObjectiveLimit:
    // as of 4.7, GLPK only uses this if it does dual simplex
    { primalObjectiveLimit_ = value ;
      if (getObjSense() == 1)
      { lpx_set_real_parm(lp_,LPX_K_OBJLL,value) ; }
      else
      { lpx_set_real_parm(lp_,LPX_K_OBJUL,value) ; }
      retval = true ; 
      break ; }
    case OsiDualTolerance:
    { if (value >= 0 && value <= .001)
      { dualTolerance_ = value;
	lpx_set_real_parm(lp_,LPX_K_TOLDJ,value) ;
	retval = true ; }
      else
      { retval = false ; }
      break ; }
    case OsiPrimalTolerance:
    { if (value >= 0 && value <= .001)
      { primalTolerance_ = value ;
	lpx_set_real_parm(lp_,LPX_K_TOLBND,value) ;
	retval = true ; }
      else
      { retval = false ; }
      break ; }
    case OsiObjOffset:
    { objOffset_ = value ;
      lpx_set_obj_coef(lp_,0,value) ;
      retval = true ;
      break ; }
    case OsiLastDblParam:
    { retval = false ;
      break ; } }

  return (retval) ; }

//-----------------------------------------------------------------------------

bool OGSI::setStrParam (OsiStrParam key,
					  const std::string &value)
{ bool retval = false ;

  switch (key)
  { case OsiProbName:
    { probName_ = value ;
      if (probName_.length() == 0)
        probName_ = "Pb";
      lpx_set_prob_name(lp_,const_cast<char *>(value.c_str())) ;
      retval = true ;
      break ; }
    case OsiSolverName:
    { retval = true ;
      break ; }
    case OsiLastStrParam:
    { break ; } }

  return (retval) ; }

//-----------------------------------------------------------------------------

namespace {
/*!
  A helper routine to deal with hints where glpk lacks any flexibility ---
  the facility is unimplemented, or always on. Failure is defined as
  OsiForceDo in the wrong direction; in this case an error is thrown. If the
  routine returns, then either the hint is compatible with glpk's capabilities,
  or it can be ignored.
*/

void unimp_hint (CoinMessageHandler *hdl, bool glpkSense, bool hintSense,
		 OsiHintStrength hintStrength, const char *msgString)

{ if (glpkSense != hintSense)
  { std::string message = "glpk " ;
    if (glpkSense == true)
    { message += "cannot disable " ; }
    else
    { message += "does not support " ; }
    message += msgString ;
    *hdl << message << CoinMessageEol ;
    if (hintStrength == OsiForceDo)
    { throw CoinError(message,"setHintParam","OsiDylpSolverInterface") ; } }
  
  return ; }

} // end file-local namespace

bool OGSI::setHintParam (OsiHintParam key, bool sense,
			 OsiHintStrength strength, void *info)
/*
  OSI provides arrays for the sense and strength. OGSI provides the array for
  info.
*/

{ bool retval = false ;
  CoinMessageHandler *msgHdl = messageHandler() ;
/*
  Check for out of range.
*/
  if (key >= OsiLastHintParam) return (false) ;
/*
  Set the hint in the OSI structures. Unlike the other set*Param routines,
  setHintParam will return false for key == OsiLastHintParam. Unfortunately,
  it'll also throw for strength = OsiForceDo, without setting a return value.
  We need to catch that throw.
*/
  try
  { retval = OsiSolverInterface::setHintParam(key,sense,strength) ; }
  catch (CoinError&)
  { retval = (strength == OsiForceDo) ; }
    
  if (retval == false) return (false) ;
  info_[key] = info ;
/*
  Did the client say `ignore this'? Who are we to argue.
*/
  if (strength == OsiHintIgnore) return (true) ;
/*
  We have a valid hint which would be impolite to simply ignore. Deal with
  it as best we can. But say something if we're ignoring the hint.

  This is under construction. For the present, we don't distinguish between
  initial solve and resolve.
*/
  switch (key)
  {
    case OsiDoPresolveInInitial:
    case OsiDoPresolveInResolve:
    { if (sense == false)
      { if (strength >= OsiHintTry) lpx_set_int_parm(lp_,LPX_K_PRESOL,0) ; }
      else
      { lpx_set_int_parm(lp_,LPX_K_PRESOL,1) ; }
      retval = true ;
      break ; }
    case OsiDoDualInInitial:
    case OsiDoDualInResolve:
    { unimp_hint(msgHdl,false,sense,strength,"exclusive use of dual simplex") ;
      if (sense == false)
      { if (strength >= OsiHintDo) lpx_set_int_parm(lp_,LPX_K_DUAL,0) ; }
      else
      { lpx_set_int_parm(lp_,LPX_K_DUAL,1) ; }
      retval = true ;
      break ; }
    case OsiDoCrash:
    { unimp_hint(msgHdl,false,sense,strength,"basis crash") ;
      retval = true ;
      break ; }
    case OsiDoInBranchAndCut:
    { unimp_hint(msgHdl,false,sense,strength,"do in branch and cut") ;
      retval = true ;
      break ; }
/*
  0 is no scaling, 3 is geometric mean followed by equilibration.
*/
    case OsiDoScale:
    { if (sense == false)
      { if (strength >= OsiHintTry) lpx_set_int_parm(lp_,LPX_K_SCALE,0) ; }
      else
      { lpx_set_int_parm(lp_,LPX_K_SCALE,3) ; }
      retval = true ;
      break ; }
/*
  Glpk supports four levels, 0 (no output), 1 (errors only), 2 (normal), and
  3 (normal plus informational).
*/
    case OsiDoReducePrint:
    { if (sense == true)
      { if (strength <= OsiHintTry)
	{ lpx_set_int_parm(lp_,LPX_K_MSGLEV,1) ; }
	else
	{ lpx_set_int_parm(lp_,LPX_K_MSGLEV,0) ; } }
      else
      { if (strength <= OsiHintTry)
	{ lpx_set_int_parm(lp_,LPX_K_MSGLEV,2) ; }
	else
	{ lpx_set_int_parm(lp_,LPX_K_MSGLEV,3) ; } }
      int logLevel = lpx_get_int_parm(lp_,LPX_K_MSGLEV) ;
      messageHandler()->setLogLevel(logLevel) ;
      retval = true ;
      break ; }
/*
  The OSI spec says that unimplemented options (and, by implication, hints)
  should return false. In the case of a hint, however, we can ignore anything
  except OsiForceDo, so usability says we should anticipate new hints and set
  this up so the solver doesn't break. So return true.
*/
    default:
    { unimp_hint(msgHdl,!sense,sense,strength,"unrecognized hint") ;
      retval = true ;
      break ; } }

  return (retval) ; }
//-----------------------------------------------------------------------------

bool
OGSI::getIntParam( OsiIntParam key, int& value ) const
{
	bool retval = false;
	switch( key )
    {
    case OsiMaxNumIteration:
		value = maxIteration_;
		retval = true;
		break;

    case OsiMaxNumIterationHotStart:
		value = hotStartMaxIteration_;
		retval = true;
		break;

    case OsiNameDiscipline:
		value = nameDisc_;
		retval = true;
		break;

    case OsiLastIntParam:
		retval = false;
		break;
    }
	return retval;
}

//-----------------------------------------------------------------------------

bool
OGSI::getDblParam( OsiDblParam key, double& value ) const
{
	bool retval = false;
	switch( key )
    {
    case OsiDualObjectiveLimit:
		value = dualObjectiveLimit_;
		retval = true;
		break;

    case OsiPrimalObjectiveLimit:
		value = primalObjectiveLimit_;
		retval = true;
		break;

    case OsiDualTolerance:
		value = dualTolerance_;
		retval = true;
		break;

    case OsiPrimalTolerance:
		value = primalTolerance_;
		retval = true;
		break;

    case OsiObjOffset:
                value = lpx_get_obj_coef(getMutableModelPtr(),0);
		retval = true;
		break;

    case OsiLastDblParam:
		retval = false;
		break;
    }
	return retval;
}


bool
OGSI::getStrParam(OsiStrParam key, std::string & value) const
{
  //	bool retval = false;
  switch (key) {
  case OsiProbName:
    value = lpx_get_prob_name( getMutableModelPtr() );
    break;
  case OsiSolverName:
    value = "glpk";
    break;
  case OsiLastStrParam:
    return false;
  }
  return true;
}
//#############################################################################
// Methods returning info on how the solution process terminated
//#############################################################################

bool OGSI::isAbandoned() const
{
	return isAbandoned_;
}

bool OGSI::isProvenOptimal() const
{
        LPX *model = getMutableModelPtr();

	if( bbWasLast_ == 0 )
	  {
	    int stat = lpx_get_status( model );
	    return stat == LPX_OPT;
	  }
	else
	  {
	    int stat = lpx_mip_status( model );
	    return stat == LPX_I_OPT;
	  }
}

bool OGSI::isProvenPrimalInfeasible() const
{
        LPX *model = getMutableModelPtr();

	if(isPrimInfeasible_==true)
		return true;

	if( bbWasLast_ == 0 )
		return lpx_get_prim_stat( model ) == LPX_P_NOFEAS;
	else
		return lpx_mip_status( model ) == LPX_I_NOFEAS;
}

bool OGSI::isProvenDualInfeasible() const
{
        LPX *model = getMutableModelPtr();
	
	if(isDualInfeasible_==true)
		return true;

	if( bbWasLast_ == 0 )
		return lpx_get_dual_stat( model ) == LPX_D_NOFEAS;
	else
	  // Not sure what to do for MIPs;  does it just mean unbounded?
	  // ??? for now, return false
	        return false;
}

/*
  This should return true if the solver stopped on the primal limit, or if the
  current objective is better than the primal limit. getObjSense == 1 is
  minimisation, -1 is maximisation.
*/
bool OGSI::isPrimalObjectiveLimitReached() const
{ bool retval = false ;
  double obj = getObjValue() ;
  double objlim ;

  getDblParam(OsiPrimalObjectiveLimit,objlim) ;

  if (getObjSense() == 1)
  { if (isObjLowerLimitReached_ || obj < objlim)
    { retval = true ; } }
  else
  { if (isObjUpperLimitReached_ || obj > objlim)
    { retval = true ; } }
  
  return (retval) ; }

/*
  This should return true if the solver stopped on the dual limit, or if the
  current objective is worse than the dual limit.
*/
bool OGSI::isDualObjectiveLimitReached() const
{ bool retval = false ;
  double obj = getObjValue() ;
  double objlim ;

  getDblParam(OsiDualObjectiveLimit,objlim) ;

  if (getObjSense() == 1)
  { if (isObjUpperLimitReached_ || obj > objlim)
    { retval = true ; } }
  else
  { if (isObjLowerLimitReached_ || obj < objlim)
    { retval = true ; } }
  
  return (retval) ; }


bool OGSI::isIterationLimitReached() const
{
	return isIterationLimitReached_;
}


bool OGSI::isTimeLimitReached() const
{
	return isTimeLimitReached_;
}

bool OGSI::isFeasible() const
{
	return isFeasible_;
}

//#############################################################################
// WarmStart related methods
//#############################################################################

/*
  Return a warm start object matching the current state of the solver.

  Nonbasic fixed variables (LPX_NS) are translated to CWSB::atLowerBound.
*/
CoinWarmStart *OGSI::getWarmStart() const

{
/*
  Create an empty basis and size it to the correct dimensions.
*/
  CoinWarmStartBasis *ws = new CoinWarmStartBasis() ;
  int numcols = getNumCols() ;
  int numrows = getNumRows() ;
  ws->setSize(numcols,numrows) ;
/*
  Walk the rows. Retrieve the status information from the glpk model and
  store it in the CWSB object.
  The convention in Osi regarding the status of a row when on bounds is different from Glpk.
  Thus, Glpk's at-lower-bound will be mapped to Osi's at-upper-bound
   and  Glpk's at-upper-bound will be mapped to Osi's at-lower-bound. 
*/
  for (int i = 0 ; i < numrows ; i++)
  { int stati = lpx_get_row_stat(lp_,i+1) ;
    switch (stati)
    { case LPX_BS:
      { ws->setArtifStatus(i,CoinWarmStartBasis::basic) ;
	break ; }
      case LPX_NS:
      case LPX_NL:
      { ws->setArtifStatus(i,CoinWarmStartBasis::atUpperBound) ;
	break ; }
      case LPX_NU:
      { ws->setArtifStatus(i,CoinWarmStartBasis::atLowerBound) ;
	break ; }
      case LPX_NF:
      { ws->setArtifStatus(i,CoinWarmStartBasis::isFree) ;
	break ; }
      default:
      { assert(false) ;
	break ; } } }
/*
  And repeat for the columns.
*/
  for (int j = 0 ; j < numcols ; j++)
  { int statj = lpx_get_col_stat(lp_,j+1) ;
    switch (statj)
    { case LPX_BS:
      { ws->setStructStatus(j,CoinWarmStartBasis::basic) ;
	break ; }
      case LPX_NS:
      case LPX_NL:
      { ws->setStructStatus(j,CoinWarmStartBasis::atLowerBound) ;
	break ; }
      case LPX_NU:
      { ws->setStructStatus(j,CoinWarmStartBasis::atUpperBound) ;
	break ; }
      case LPX_NF:
      { ws->setStructStatus(j,CoinWarmStartBasis::isFree) ;
	break ; }
      default:
      { assert(false) ;
	break ; } } }

  return (ws) ; }

//-----------------------------------------------------------------------------

/*
  Set the given warm start information in the solver.
  
  By definition, a null warmstart parameter is interpreted as `refresh warm
  start information from the solver.' Since OGSI does not cache warm start
  information, no action is required.
  The convention in Osi regarding the status of a row when on bounds is different from Glpk.
  Thus, Osi's at-upper-bound will be mapped to Glpk's at-lower-bound
   and  Osi's at-lower-bound will be mapped to Glpk's at-upper-bound. 
*/

bool OGSI::setWarmStart (const CoinWarmStart* warmstart)

{
/*
  If this is a simple refresh request, we're done.
*/
  if (warmstart == 0)
  { return (true) ; }
/*
  Check that the parameter is a CWSB of the appropriate size.
*/
  const CoinWarmStartBasis *ws =
      dynamic_cast<const CoinWarmStartBasis *>(warmstart) ;
  if (ws == 0)
  { return false ; }
	
  int numcols = ws->getNumStructural() ;
  int numrows = ws->getNumArtificial() ;
	
  if (numcols != getNumCols() || numrows != getNumRows())
  { return (false) ; }
/*
  Looks like a basis. Translate to the appropriate codes for glpk and install.
  Row status first (logical/artificial variables), then column status
  (architectural variables).
*/
  for (int i = 0 ; i < numrows ; i++)
  { int stati ;

    switch (ws->getArtifStatus(i))
    { case CoinWarmStartBasis::basic:
      { stati = LPX_BS ;
	break ; }
      case CoinWarmStartBasis::atLowerBound:
      { stati = LPX_NU ;
	break ; }
      case CoinWarmStartBasis::atUpperBound:
      { stati = LPX_NL ;
	break ; }
      case CoinWarmStartBasis::isFree:
      { stati = LPX_NF ;
	break ; }
      default:
      { assert(false) ;
	return (false) ; } }		

    lpx_set_row_stat(lp_,i+1,stati) ; }
  
  for (int j = 0 ; j < numcols ; j++)
  { int statj ;

    switch (ws->getStructStatus(j))
    { case CoinWarmStartBasis::basic:
      { statj = LPX_BS ;
	break ; }
      case CoinWarmStartBasis::atLowerBound:
      { statj = LPX_NL ;
	break ; }
      case CoinWarmStartBasis::atUpperBound:
      { statj = LPX_NU ;
	break ; }
      case CoinWarmStartBasis::isFree:
      { statj = LPX_NF ;
	break ; }
      default:
      { assert(false) ;
	return (false) ; } }

    lpx_set_col_stat(lp_,j+1,statj) ; }

  return (true) ; }


//#############################################################################
// Hotstart related methods (primarily used in strong branching)
//#############################################################################

void OGSI::markHotStart()
{
        LPX *model = getMutableModelPtr();
	int numcols, numrows;

	numcols = getNumCols();
	numrows = getNumRows();
	if( numcols > hotStartCStatSize_ )
    {
		delete[] hotStartCStat_;
		delete [] hotStartCVal_;
		delete [] hotStartCDualVal_;
		hotStartCStatSize_ = static_cast<int>( 1.2 * static_cast<double>( numcols ) ); // get some extra space for future hot starts
		hotStartCStat_ = new int[hotStartCStatSize_];
		hotStartCVal_ = new double[hotStartCStatSize_];
		hotStartCDualVal_ = new double[hotStartCStatSize_];
    }
	int j;
	for( j = 0; j < numcols; j++ ) {
		int stat;
		double val;
		double dualVal;
		stat=lpx_get_col_stat(model,j);
		val=lpx_get_col_prim(model,j);
		dualVal=lpx_get_col_dual(model,j);
		hotStartCStat_[j] = stat;
		hotStartCVal_[j] = val;
		hotStartCDualVal_[j] = dualVal;
	}

	if( numrows > hotStartRStatSize_ )
    {
		delete [] hotStartRStat_;
		delete [] hotStartRVal_;
		delete [] hotStartRDualVal_;
		hotStartRStatSize_ = static_cast<int>( 1.2 * static_cast<double>( numrows ) ); // get some extra space for future hot starts
		hotStartRStat_ = new int[hotStartRStatSize_];
		hotStartRVal_ = new double[hotStartRStatSize_];
		hotStartRDualVal_ = new double[hotStartRStatSize_];
    }
	int i;
	for( i = 0; i < numrows; i++ ) {
		int stat;
		double val;
		double dualVal;
		stat=lpx_get_row_stat(model,i+1);
		val=lpx_get_row_prim(model,i+1);
		dualVal=lpx_get_row_dual(model,i+1);
		hotStartRStat_[i] = stat;
		hotStartRVal_[i] = val;
		hotStartRDualVal_[i] = dualVal;
	}
}

//-----------------------------------------------------------------------------

void OGSI::solveFromHotStart()
{
#     if OGSI_TRACK_FRESH > 0
      std::cout
	<< "OGSI(" << std::hex << this << std::dec
	<< ")::solveFromHotStart." << std::endl ;
#     endif
        LPX *model = getMutableModelPtr();
	int numcols, numrows;

	numcols = getNumCols();
	numrows = getNumRows();

	assert( numcols <= hotStartCStatSize_ );
	assert( numrows <= hotStartRStatSize_ );

	int j;
	for( j = 0; j < numcols; j++ ) {
	  lpx_set_col_stat( model, j+1, hotStartCStat_[j]);
	}
	int i;
	for( i = 0; i < numrows; i++ ) {
	  lpx_set_row_stat( model, i+1, hotStartRStat_[i]);
	}

	freeCachedData( OGSI::FREECACHED_RESULTS );

	int maxIteration = maxIteration_;
	maxIteration_ = hotStartMaxIteration_;
	resolve();
	maxIteration_ = maxIteration;
}

//-----------------------------------------------------------------------------

void OGSI::unmarkHotStart()
{
	// ??? be lazy with deallocating memory and do nothing here, deallocate memory in the destructor.
}

//#############################################################################
// Problem information methods (original data)
//#############################################################################

//-----------------------------------------------------------------------------
// Get number of rows, columns, elements, ...
//-----------------------------------------------------------------------------
int OGSI::getNumCols() const
{
	return lpx_get_num_cols( getMutableModelPtr() );
}

int OGSI::getNumRows() const
{
	return lpx_get_num_rows( getMutableModelPtr() );
}

int OGSI::getNumElements() const
{
	return lpx_get_num_nz( getMutableModelPtr() );
}

//-----------------------------------------------------------------------------
// Get pointer to rim vectors
//------------------------------------------------------------------

const double * OGSI::getColLower() const
{
        LPX *model = getMutableModelPtr();

	if( collower_ == NULL )
	{
		assert( colupper_ == NULL );
		int numcols = getNumCols();
		if( numcols > 0 )
		{
			collower_ = new double[numcols];
			colupper_ = new double[numcols];
		}

		double inf = getInfinity();

		int i;
		for( i = 0; i < numcols; i++)
		{
			int type;
			double lb;
			double ub;
			type=lpx_get_col_type(model,i+1);
			lb=lpx_get_col_lb(model,i+1);
			ub=lpx_get_col_ub(model,i+1);
			switch ( type )
			{
			case LPX_FR:
				lb = -inf;
				ub = inf;
				break;

			case LPX_LO:
				ub = inf;
				break;

			case LPX_UP:
				lb = -inf;
				break;

			case LPX_FX:
			case LPX_DB:
				break;

			default:
				assert( false );
				break;
			}
			collower_[i] = lb;
			colupper_[i] = ub;
		}
	}
	return collower_;
}

//-----------------------------------------------------------------------------

const double * OGSI::getColUpper() const
{
	if( colupper_ == NULL )
	{
		getColLower();
		//assert( colupper_ != NULL ); // Could be null if no columns.
	}
	return colupper_;
}

//-----------------------------------------------------------------------------

const char * OGSI::getRowSense() const
{
	// Could be in OsiSolverInterfaceImpl.
	if( rowsense_ == NULL )
	{
		assert( rhs_ == NULL && rowrange_ == NULL );

		int numrows = getNumRows();
		if( numrows > 0 )
		{
			rowsense_ = new char[numrows];
			rhs_ = new double[numrows];
			rowrange_ = new double[numrows];
		}

		const double *rowlower = getRowLower();
		const double *rowupper = getRowUpper();
		int i;
		for( i = 0; i < numrows; i++ )
		{
			char sense;
			double right;
			double range;
			convertBoundToSense( rowlower[i], rowupper[i], sense, right, range );
			rowsense_[i] = sense;
			rhs_[i] = right;
			rowrange_[i] = range;
		}
	}
	return rowsense_;
}

//-----------------------------------------------------------------------------

const double * OGSI::getRightHandSide() const
{
	if( rhs_ == NULL )
	{
		getRowSense();
		//assert( rhs_ != NULL ); // Could be null if no rows.
	}
	return rhs_;
}

//-----------------------------------------------------------------------------

const double * OGSI::getRowRange() const
{
	if( rowrange_ == NULL )
	{
		getRowSense();
		//assert( rowrange_ != NULL ); // Could be null if no rows.
	}
	return rowrange_;
}

//-----------------------------------------------------------------------------

const double * OGSI::getRowLower() const
{
        LPX *model = getMutableModelPtr();

	if( rowlower_ == NULL )
	{
		assert( rowupper_ == NULL );
		int numrows = getNumRows();
		if( numrows > 0 )
		{
			rowlower_ = new double[numrows];
			rowupper_ = new double[numrows];
		}
		int i;
		for( i = 0; i < numrows; i++ )
		{
			double inf = getInfinity();
			int type;
			double lb;
			double ub;
			type=lpx_get_row_type(model,i+1);
			lb=lpx_get_row_lb(model,i+1);
			ub=lpx_get_row_ub(model,i+1);
			switch( type )
			{
			case LPX_FR:
				lb = -inf;
				ub = inf;
				break;

			case LPX_LO:
				ub = inf;
				break;

			case LPX_UP:
				lb = -inf;
				break;

			case LPX_DB:
			case LPX_FX:
				break;

			default:
				assert( false );
				break;
			}
			rowlower_[i] = lb;
			rowupper_[i] = ub;
		}
	}
	return rowlower_;
}

//-----------------------------------------------------------------------------

const double * OGSI::getRowUpper() const
{
	if( rowupper_ == NULL )
	{
		getRowLower();
		//assert( rowupper_ != NULL ); // Could be null if no rows.
	}
	return rowupper_;
}

//-----------------------------------------------------------------------------

const double * OGSI::getObjCoefficients() const
{
	if( obj_ == NULL )
	{
	        LPX *model = getMutableModelPtr();

		int numcols = getNumCols();
		if( numcols > 0 )
		{
			obj_ = new double[numcols];
		}
		int i;
		for( i = 0; i < numcols; i++ )
		{
			obj_[i] = lpx_get_obj_coef( model, i + 1);
		}
	}
	return obj_;
}

//-----------------------------------------------------------------------------

double OGSI::getObjSense() const

{ if (lpx_get_obj_dir(lp_) == LPX_MIN)
  { return (+1.0) ; }
  else
  if (lpx_get_obj_dir(lp_) == LPX_MAX)
  { return (-1.0) ; }
  else	// internal confusion
  { assert(false) ;
    return (0) ; } }

//-----------------------------------------------------------------------------
// Return information on integrality
//-----------------------------------------------------------------------------

bool OGSI::isContinuous( int colNumber ) const
{
  return lpx_get_col_kind( getMutableModelPtr(), colNumber+1 ) == LPX_CV;
}

//-----------------------------------------------------------------------------
// Row and column copies of the matrix ...
//-----------------------------------------------------------------------------

const CoinPackedMatrix * OGSI::getMatrixByRow() const
{
	if( matrixByRow_ == NULL )
	{
	        LPX *model = getMutableModelPtr();

		matrixByRow_ = new CoinPackedMatrix();
		matrixByRow_->transpose();  // converts to row-order
		matrixByRow_->setDimensions( 0, getNumCols() );

		int numcols = getNumCols();
		int *colind = new int[numcols+1];
		double *colelem = new double[numcols+1];
		int i;
		for( i = 0; i < getNumRows(); i++ )
		{
			int colsize = lpx_get_mat_row( model, i+1, colind, colelem);
			int j;
			for( j = 1; j <= colsize; j++ )
			{
				--colind[j];
			}

			// Note:  lpx_get_mat_row apparently may return the
			// elements in decreasing order.  This differs from
			// people's standard expectations but is not an error.

			matrixByRow_->appendRow( colsize, colind+1, colelem+1 );
		}
		delete [] colind;
		delete [] colelem;
		if( numcols )
			matrixByRow_->removeGaps();
	}
	return matrixByRow_;
}

//-----------------------------------------------------------------------------

const CoinPackedMatrix * OGSI::getMatrixByCol() const
{
	if( matrixByCol_ == NULL )
	{
   	        LPX *model = getMutableModelPtr();

		matrixByCol_ = new CoinPackedMatrix();
		matrixByCol_->setDimensions( getNumRows(), 0 );

		int numrows = getNumRows();
		int *rowind = new int[numrows+1];
		double *rowelem = new double[numrows+1];
		int j;
		for( j = 0; j < getNumCols(); j++ )
		{
			int rowsize = lpx_get_mat_col( model, j+1, rowind, rowelem);
			int i;
			for( i = 1; i <= rowsize; i++ )
			{
				--rowind[i];
			}
			matrixByCol_->appendCol( rowsize, rowind+1, rowelem+1 );
		}
		delete [] rowind;
		delete [] rowelem;
		if( numrows )
			matrixByCol_->removeGaps();
	}
	return matrixByCol_;
}

//-----------------------------------------------------------------------------
// Get solver's value for infinity
//-----------------------------------------------------------------------------
double OGSI::getInfinity() const
{
  	return (CoinInfinity) ;
}

//#############################################################################
// Problem information methods (results)
//#############################################################################

/*
  Retrieve column solution, querying solver if we don't have a cached solution.
  Refresh reduced costs while we're here.

  Reduced costs may or may not already exist (user can create a reduced cost
  vector by supplying duals (row price) and asking for reduced costs).

  Appropriate calls depend on whether last call to solver was simplex or
  branch-and-bound.
*/

const double *OGSI::getColSolution() const

{ 
/*
  Use the cached solution vector, if present. If we have no constraint system,
  return 0.
*/
  if (colsol_ != 0)
  { return (colsol_) ; }

  int numcols = getNumCols() ;
  if (numcols == 0)
  { return (0) ; }

  colsol_ = new double[numcols] ;
  if (redcost_ != 0) delete [] redcost_ ;
  { redcost_ = new double[numcols] ; }
/*
  Grab the problem status.
*/
  int probStatus ;
  if (bbWasLast_)
  { probStatus = lpx_mip_status(lp_) ; }
  else
  { probStatus = lpx_get_status(lp_) ; }
/*
  If the problem hasn't been solved, glpk returns zeros, but OSI requires that
  the solution be within bound. getColLower() will ensure both upper and lower
  bounds are present. There's an implicit assumption that we're feasible (i.e.,
  collower[j] < colupper[j]). Solution values will be 0.0 unless that's outside
  the bounds.
*/
  if (probStatus == LPX_UNDEF || probStatus == LPX_I_UNDEF)
  { getColLower() ;
    int j ;
    for (j = 0 ; j < numcols ; j++)
    { colsol_[j] = 0.0 ;
      if (collower_[j] > 0.0)
      { colsol_[j] = collower_[j] ; }
      else
      if (colupper_[j] < 0.0) 
      { colsol_[j] = colupper_[j] ; } } }
/*
  We should have a defined solution. For simplex, refresh the reduced costs
  as well as the primal solution. Remove infinitesimals from the results.
*/
  else
  if (bbWasLast_ == 0)
  { int j ;
    for (j = 0 ; j < numcols ; j++)
    { colsol_[j] = lpx_get_col_prim(lp_,j+1) ;
      if (fabs(colsol_[j]) < GlpkZeroTol)
      { colsol_[j] = 0.0 ; }
      redcost_[j] = lpx_get_col_dual(lp_,j+1) ;
      if (fabs(redcost_[j]) < GlpkZeroTol)
      { redcost_[j] = 0.0 ; } } }
/*
  Last solve was branch-and-bound. Set the reduced costs to 0. Remove
  infinitesimals from the results.
*/
  else
  { int j ;
    for (j = 0 ; j < numcols ; j++)
    { colsol_[j] = lpx_mip_col_val(lp_,j+1) ;
      if (fabs(colsol_[j]) < GlpkZeroTol)
      { colsol_[j] = 0.0 ; }
      redcost_[j] = 0.0 ; } }

  return (colsol_) ; }

//-----------------------------------------------------------------------------

/*
  Acquire the row solution (dual variables).

  The user can set this independently, so use the cached copy if it's present.
*/

const double *OGSI::getRowPrice() const

{ 
/*
  If we have a cached solution, use it. If the constraint system is empty,
  return 0. Otherwise, allocate a new vector.
*/
  if (rowsol_ != 0)
  { return (rowsol_) ; }

  int numrows = getNumRows() ;
  if (numrows == 0)
  { return (0) ; }

  rowsol_ = new double[numrows] ;
/*
  For simplex, we have dual variables. For MIP, just set them to zero. Remove
  infinitesimals from the results.
*/
  if (bbWasLast_ == 0)
  { int i ;
    for (i = 0 ; i < numrows; i++)
    { rowsol_[i] = lpx_get_row_dual(lp_,i+1) ;
      if (fabs(rowsol_[i]) < GlpkZeroTol)
      { rowsol_[i] = 0.0 ; } } }
  else
  { int i ;
    for (i = 0 ; i < numrows ; i++)
    { rowsol_[i] = 0 ; } }
  
  return (rowsol_) ; }

//-----------------------------------------------------------------------------

/*
  It's tempting to dive into glpk for this, but ... the client may have used
  setRowPrice(), and it'd be nice to return reduced costs that agree with the
  duals.

  To use glpk's routine (lpx_get_col_dual), the interface needs to track the
  origin of the dual (row price) values.
*/
const double * OGSI::getReducedCost() const
{
/*
  Return the cached copy, if it exists.
*/
  if (redcost_ != 0)
  { return (redcost_) ; }
/*
  We need to calculate. Make sure we have a constraint system, then allocate
  a vector and initialise it with the objective coefficients.
*/
  int n = getNumCols() ;
  if (n == 0)
  { return (0) ; }

  redcost_ = new double[n] ;
  CoinDisjointCopyN(getObjCoefficients(),n,redcost_) ;
/*
  Try and acquire dual variables. 
*/
  const double *y = getRowPrice() ;
  if (!y)
  { return (redcost_) ; }
/*
  Acquire the constraint matrix and calculate y*A.
*/
  const CoinPackedMatrix *mtx = getMatrixByCol() ;
  double *yA = new double[n] ;
  mtx->transposeTimes(y,yA) ;
/*
  Calculate c-yA and remove infinitesimals from the result.
*/
  for (int j = 0 ; j < n ; j++)
  { redcost_[j] -= yA[j] ;
    if (fabs(redcost_[j]) < GlpkZeroTol)
    { redcost_[j] = 0 ; } }
  
  delete[] yA ;

  return (redcost_) ; }

//-----------------------------------------------------------------------------

/*
  As with getReducedCost, we need to be mindful that the primal solution may
  have been set by the client. As it happens, however, glpk has no equivalent
  routine, so there's no temptation to resist.
*/

const double *OGSI::getRowActivity() const

{
/*
  Return the cached copy, if it exists.
*/
  if (rowact_)
  { return (rowact_) ; }
/*
  We need to calculate. Make sure we have a constraint system, then allocate
  a vector and initialise it with the objective coefficients.
*/
  int m = getNumRows() ;
  if (m == 0)
  { return (0) ; }

  rowact_ = new double[m] ;
/*
  Acquire primal variables.
*/
  const double *x = getColSolution() ;
  if (!x)
  { CoinZeroN(rowact_,m) ;
    return (rowact_) ; }
/*
  Acquire the constraint matrix and calculate A*x.
*/
  const CoinPackedMatrix *mtx = getMatrixByRow() ;
  mtx->times(x,rowact_) ;
/*
  Remove infinitesimals from the result.
*/
  for (int i = 0 ; i < m ; i++)
  { if (fabs(rowact_[i]) < GlpkZeroTol)
    { rowact_[i] = 0 ; } }
  
  return (rowact_) ; }

//-----------------------------------------------------------------------------

double OGSI::getObjValue() const
{
  return OsiSolverInterface::getObjValue();
}

//-----------------------------------------------------------------------------

int OGSI::getIterationCount() const
{
	return iter_used_;
}

//-----------------------------------------------------------------------------

std::vector<double*> OGSI::getDualRays(int /*maxNumRays*/, bool /*fullRay*/) const
{
	// ??? not yet implemented.
	throw CoinError("method is not yet implemented", "getDualRays", "OsiGlpkSolverInterface");
	return std::vector<double*>();
}

//-----------------------------------------------------------------------------

std::vector<double*> OGSI::getPrimalRays(int /*maxNumRays*/) const
{
	// ??? not yet implemented.
	throw CoinError("method is not yet implemented", "getPrimalRays", "OsiGlpkSolverInterface");
	return std::vector<double*>();
}

//#############################################################################
// Problem modifying methods (rim vectors)
//#############################################################################

/*
  Yes, I know the OSI spec says that pointers returned by get*() routines are
  valid only as long as the data is unchanged. But in practice, client code
  (in particular, portions of Cgl and Cbc) expects that simple changes to rim
  vector values will not invalidate pointers returned by the various get*()
  routines. So, for example, modifying an objective coefficient should not
  free the cached copies of the objective and column bounds. They should be
  updated on-the-fly. This is exactly the opposite of the original OsiGlpk
  design, hence this warning.

  Beyond that, it's helpful for an OSI to hold on to the current solution as
  long as possible. So, for example, a bounds change that does not make the
  current solution infeasible should not trash the cached solution.

  -- lh, 070326
*/

void OGSI::setObjCoeff (int j, double cj)

{ assert(j >= 0 && j < getNumCols()) ;

/*
  Remove the solution. Arguably the only thing that we should remove here is
  the cached reduced cost.
*/
  freeCachedData(OGSI::KEEPCACHED_PROBLEM) ;
/*
  Push the changed objective down to glpk.
*/
  lpx_set_obj_coef(lp_,j+1,cj) ;

  if (obj_)
  { obj_[j] = cj ; }

  return ; }

//-----------------------------------------------------------------------------

void OGSI::setColLower (int j, double lbj)

{
/*
  Get the upper bound, so we can call setColBounds.  glpk reports 0 for an
  infinite bound, so we need to check the status and possibly correct.
*/
  double inf = getInfinity() ;
  int type = lpx_get_col_type(lp_,j+1) ;
  double ubj = lpx_get_col_ub(lp_,j+1) ;
  switch (type)
  { case LPX_UP:
    case LPX_DB:
    case LPX_FX:
    { break ; }
    case LPX_FR:
    case LPX_LO:
    { ubj = inf ;
      break ; }
    default:
    { assert (false) ; } }

# if OGSI_TRACK_FRESH > 0
/*
  Check if this bound really invalidates the current solution. This check will
  give false positives because OsiGlpk needs a notion of `valid solution from
  solver'. getColSolution will always return something, even if it has to make
  it up on the spot.
*/
  { const double *x = getColSolution() ;
    if (x)
    { if (x[j] < lbj-GlpkZeroTol)
      { std::cout
	  << "OGSI(" << std::hex << this << std::dec
	  << ")::setColLower: new bound "
	  << lbj << " exceeds current value " << x[j]
	  << " by " << (lbj-x[j]) << "." << std::endl ; } } }
# endif

  setColBounds(j,lbj,ubj) ;

  return ; }

//-----------------------------------------------------------------------------

void OGSI::setColUpper (int j, double ubj)

{
/*
  Get the lower bound, so we can call setColBounds.  glpk reports 0 for an
  infinite bound, so we need to check the status and possibly correct.
*/
  double inf = getInfinity() ;
  int type = lpx_get_col_type(lp_,j+1) ;
  double lbj = lpx_get_col_lb(lp_,j+1) ;
  switch (type)
  { case LPX_LO:
    case LPX_DB:
    case LPX_FX:
    { break ; }
    case LPX_FR:
    case LPX_UP:
    { lbj = inf ;
      break ; }
    default:
    { assert (false) ; } }

# if OGSI_TRACK_FRESH > 0
/*
  Check if this bound really invalidates the current solution. This check will
  give false positives because OsiGlpk needs a notion of `valid solution from
  solver'. getColSolution will always return something, even if it has to make
  it up on the spot.
*/
  { const double *x = getColSolution() ;
    if (x)
    { if (x[j] > ubj+GlpkZeroTol)
      { std::cout
	  << "OGSI(" << std::hex << this << std::dec
	  << ")::setColUpper: new bound "
	  << ubj << " exceeds current value " << x[j]
	  << " by " << (x[j]-ubj) << "." << std::endl ; } } }
# endif

  setColBounds(j,lbj,ubj) ;

  return ; }

//-----------------------------------------------------------------------------

/*
  Yes, in theory, we shouldn't need to worry about maintaining those cached
  column bounds over changes to the problem. See the note at the top of this
  section.
*/

void OGSI::setColBounds (int j, double lower, double upper)

{ assert(j >= 0 && j < getNumCols()) ;
/*
  Free only the cached solution. Keep the cached structural variables.
*/
  freeCachedData(OGSI::KEEPCACHED_PROBLEM) ;
/*
  Figure out what type we should use for glpk.
*/
  double inf = getInfinity() ;
  int type ;

  if (lower == upper)
  { type = LPX_FX ; }
  else
  if (lower > -inf && upper < inf)
  { type = LPX_DB ; }
  else
  if (lower > -inf)
  { type = LPX_LO ; }
  else
  if (upper < inf)
  { type = LPX_UP ; }
  else
  { type = LPX_FR ; }
/*
  Push the bound change down into the solver. 1-based addressing.
*/
  int statj = lpx_get_col_stat(lp_,j+1) ;
  lpx_set_col_bnds(lp_,j+1,type,lower,upper) ;
  lpx_set_col_stat(lp_,j+1,statj) ;
  statj = lpx_get_col_stat(lp_,j+1) ;
/*
  Correct the cached upper and lower bound vectors, if present.
*/
  if (collower_)
  { collower_[j] = lower ; }
  if (colupper_)
  { colupper_[j] = upper ; }

  return ; }

//-----------------------------------------------------------------------------

void OGSI::setColSetBounds(const int* indexFirst,
					     const int* indexLast,
					     const double* boundList)
{
	OsiSolverInterface::setColSetBounds( indexFirst, indexLast, boundList );
}

//-----------------------------------------------------------------------------

void
OGSI::setRowLower( int elementIndex, double elementValue )
{
	// Could be in OsiSolverInterfaceImpl.
	double inf = getInfinity();

	int type;
	double lb;
	double ub;

	type=lpx_get_row_type(getMutableModelPtr(),elementIndex+1);
	ub=lpx_get_row_ub(getMutableModelPtr(),elementIndex+1);
	lb = elementValue;
	switch( type )
	{
	case LPX_UP:
	case LPX_DB:
	case LPX_FX:
		break;

	case LPX_FR:
	case LPX_LO:
		ub = inf;
		break;

	default:
		assert( false );
	}
	setRowBounds( elementIndex, lb, ub );
}

//-----------------------------------------------------------------------------
void
OGSI::setRowUpper( int elementIndex, double elementValue )
{
	// Could be in OsiSolverInterfaceImpl.
	double inf = getInfinity();

	int type;
	double lb;
	double ub;

	type=lpx_get_row_type(getMutableModelPtr(),elementIndex+1);
	lb=lpx_get_row_lb(getMutableModelPtr(),elementIndex+1);
	ub = elementValue;
	switch( type )
	{
	case LPX_LO:
	case LPX_DB:
	case LPX_FX:
		break;

	case LPX_FR:
	case LPX_UP:
		lb = -inf;
		break;

	default:
		assert( false );
	}
	setRowBounds( elementIndex, lb, ub );
}

//-----------------------------------------------------------------------------

/*
  As with setColBounds, just changing the bounds should not invalidate the
  current rim vectors.
*/

void OGSI::setRowBounds (int i, double lower, double upper)
{
/*
  Free only the row and column solution, keep the cached structural vectors.
*/
  freeCachedData(OGSI::KEEPCACHED_PROBLEM) ;
/*
  Figure out the correct row type for glpk and push the change down into the
  solver. 1-based addressing.
*/
  double inf = getInfinity() ;
  int type ;

  if( lower == upper)
  { type = LPX_FX ; }
  else
  if (lower > -inf && upper < inf)
  { type = LPX_DB ; }
  else
  if (lower > -inf)
  { type = LPX_LO ; }
  else
  if (upper < inf)
  { type = LPX_UP ; }
  else
  { type = LPX_FR ; }

  lpx_set_row_bnds(lp_,i+1,type,lower,upper) ;
/*
  Update cached vectors, if they exist.
*/
  if (rowlower_)
  { rowlower_[i] = lower ; }
  if (rowupper_)
  { rowupper_[i] = upper ; }

  return ; }

//-----------------------------------------------------------------------------

void
OGSI::setRowType(int elementIndex, char sense, double rightHandSide,
								   double range)
{
	// Could be in OsiSolverInterfaceImpl.
	double lower = 0.0 ;
	double upper = 0.0 ;
    convertSenseToBound( sense, rightHandSide, range, lower, upper );
	setRowBounds( elementIndex, lower, upper );
}

//-----------------------------------------------------------------------------

void OGSI::setRowSetBounds(const int* indexFirst,
					     const int* indexLast,
					     const double* boundList)
{
	// Could be in OsiSolverInterface (should'nt be implemeted at here).
	OsiSolverInterface::setRowSetBounds( indexFirst, indexLast, boundList );
}

//-----------------------------------------------------------------------------

void
OGSI::setRowSetTypes(const int* indexFirst,
				       const int* indexLast,
				       const char* senseList,
				       const double* rhsList,
				       const double* rangeList)
{
	// Could be in OsiSolverInterface (should'nt be implemeted at here).
	OsiSolverInterface::setRowSetTypes( indexFirst, indexLast, senseList, rhsList, rangeList );
}

//#############################################################################

void
OGSI::setContinuous(int index)
{
        LPX *model = getMutableModelPtr();
	freeCachedData( OGSI::FREECACHED_COLUMN );
	lpx_set_col_kind( model, index+1, LPX_CV );
}

//-----------------------------------------------------------------------------

void OGSI::setInteger (int index)

{ LPX *model = getMutableModelPtr() ;
  freeCachedData(OGSI::FREECACHED_COLUMN) ;
  lpx_set_col_kind(model,index+1,LPX_IV) ;
/*
  Temporary hack to correct upper bounds on general integer variables.
  CoinMpsIO insists on forcing a bound of 1e30 for general integer variables
  with no upper bound. This causes several cut generators (MIR, MIR2) to fail.
  Put the bound back to infinity.

  -- lh, 070530 --

  double uj = getColUpper()[index] ;
  if (uj >= 1e30)
  { setColUpper(index,getInfinity()) ; }
*/
  return ; }

//-----------------------------------------------------------------------------

void
OGSI::setContinuous(const int* indices, int len)
{
	// Could be in OsiSolverInterfaceImpl.
	int i;
	for( i = 0; i < len; i++ )
	{
		setContinuous( indices[i] );
	}
}

//-----------------------------------------------------------------------------

void
OGSI::setInteger(const int* indices, int len)
{
	// Could be in OsiSolverInterfaceImpl.
	int i;
	for( i = 0; i < len; i++ )
	{
		setInteger( indices[i] );
	}
}

//#############################################################################

/*
  Opt for the natural sense (minimisation) unless the user is clear that
  maximisation is what's desired.
*/
void OGSI::setObjSense(double s)

{ freeCachedData(OGSI::FREECACHED_RESULTS) ;

  if (s <= -1.0)
  { lpx_set_obj_dir(lp_,LPX_MAX) ; }
  else
  { lpx_set_obj_dir(lp_,LPX_MIN) ; }

  return ; }

//-----------------------------------------------------------------------------

void OGSI::setColSolution(const double * cs)
{
  // You probably don't want to use this function.  You probably want
  // setWarmStart instead.
  // This implementation changes the cached information, 
  // BUT DOES NOT TELL GLPK about the changes.  In that sense, it's not
  // really useful.  It is added to conform to current OSI expectations.

  // Other results (such as row prices) might not make sense with this 
  // new solution, but we can't free all the results we have, since the 
  // row prices may have already been set with setRowPrice.
  if (cs == 0)
    delete [] colsol_;
  else
    {
      int nc = getNumCols();

      if (colsol_ == 0)
	colsol_ = new double[nc];

      // Copy in new col solution.
      CoinDisjointCopyN( cs, nc, colsol_ );
    }
}

//-----------------------------------------------------------------------------

void OGSI::setRowPrice(const double * rs)
{
  // You probably don't want to use this function.  You probably want
  // setWarmStart instead.
  // This implementation changes the cached information, 
  // BUT DOES NOT TELL GLPK about the changes.  In that sense, it's not
  // really useful.  It is added to conform to current OSI expectations.

  // Other results (such as column solutions) might not make sense with this 
  // new solution, but we can't free all the results we have, since the 
  // column solutions may have already been set with setColSolution.
  if (rs == 0)
    delete [] rowsol_;
  else
    {
      int nr = getNumRows();

      if (rowsol_ == 0)
	rowsol_ = new double[nr];

      // Copy in new col solution.
      CoinDisjointCopyN( rs, nr, rowsol_ );
    }
}

//#############################################################################
// Problem modifying methods (matrix)
//#############################################################################

void OGSI::addCol (const CoinPackedVectorBase& vec,
		   const double collb, const double colub, const double obj)
{
  // Note: GLPK expects only non-zero coefficients will be given in 
  //   lpx_set_mat_col and will abort if there are any zeros.  So any
  //   zeros must be removed prior to calling lpx_set_mat_col.
        LPX *model = getMutableModelPtr();
	freeCachedData(OGSI::KEEPCACHED_ROW);

	lpx_add_cols( model, 1 );
	int numcols = getNumCols();
	setColBounds( numcols-1, collb, colub );
	setObjCoeff( numcols-1, obj );
	int i;
	// For GLPK, we don't want the arrays to start at 0
	// We also need to weed out any 0.0 coefficients
	const int *indices = vec.getIndices();
	const double *elements = vec.getElements();
	int numrows = getNumRows();

	int *indices_adj = new int[1+vec.getNumElements()];
	double *elements_adj = new double[1+vec.getNumElements()];

	int count = 0;
	for( i = 0; i < vec.getNumElements(); i++ ) {
	  if (elements[i] != 0.0)
	    {
		if ( indices[i]+1 > numrows ) {
			lpx_add_rows( model, indices[i]+1 - numrows );
			numrows = indices[i]+1;
			// ??? could do this more efficiently with a single call based on the max
		}
		count++;
		// GLPK arrays start at 1
		indices_adj[count] = indices[i] + 1;
		elements_adj[count] = elements[i];
	    }
	}
	lpx_set_mat_col( model, numcols, count, indices_adj, elements_adj );
	delete [] indices_adj;
	delete [] elements_adj;

# if OGSI_TRACK_FRESH > 2
  std::cout
    << "OGSI(" << std::hex << this << std::dec
    << ")::addCol: new column." << std::endl ;
# endif

}

//-----------------------------------------------------------------------------

void OGSI::addCols (const int numcols,
		    const CoinPackedVectorBase *const *cols,
		    const double *collb, const double *colub,
		    const double *obj)
{
  // ??? We could do this more efficiently now 
	// Could be in OsiSolverInterfaceImpl.
	int i;
	for( i = 0; i < numcols; ++i )
		addCol( *(cols[i]), collb[i], colub[i], obj[i] );
}

//-----------------------------------------------------------------------------

void
OGSI::deleteCols(const int num, const int * columnIndices)
{
	int *columnIndicesPlus1 = new int[num+1];
        LPX *model = getMutableModelPtr();
	freeCachedData( OGSI::KEEPCACHED_ROW );

	for( int i = 0; i < num; i++ )
	{
		columnIndicesPlus1[i+1]=columnIndices[i]+1;
		deleteColNames(columnIndices[i],1);
	}
	lpx_del_cols(model,num,columnIndicesPlus1);
	delete [] columnIndicesPlus1;

# if OGSI_TRACK_FRESH > 0
  std::cout
    << "OGSI(" << std::hex << this << std::dec
    << ")::deleteCols: deleted " << num << "columns." << std::endl ;
# endif

}

//-----------------------------------------------------------------------------

void
OGSI::addRow (const CoinPackedVectorBase& vec,
	      const double rowlb, const double rowub)
{
  // Note: GLPK expects only non-zero coefficients will be given in 
  //   lpx_set_mat_row and will abort if there are any zeros.  So any
  //   zeros must be removed prior to calling lpx_set_mat_row.

        LPX *model = getMutableModelPtr();
	freeCachedData( OGSI::KEEPCACHED_COLUMN );

	lpx_add_rows( model, 1 );
	int numrows = getNumRows();
	setRowBounds( numrows-1, rowlb, rowub );
	int i;
	const int *indices = vec.getIndices();
	const double *elements = vec.getElements();
	int numcols = getNumCols();

	// For GLPK, we don't want the arrays to start at 0
	// Also, we need to weed out any 0.0 elements
	int *indices_adj = new int[1+vec.getNumElements()];
	double *elements_adj = new double[1+vec.getNumElements()];

	int count = 0;
	for( i = 0; i < vec.getNumElements(); i++ ) {
	  if ( elements[i] != 0.0 )
	    {
		if ( indices[i]+1 > numcols ) {
		  // ??? Could do this more efficiently with a single call
			lpx_add_cols( model, indices[i]+1 - numcols );
			numcols = indices[i]+1;
		}
		count++;
		elements_adj[count] = elements[i];
		indices_adj[count] = indices[i] + 1;
	    }
	}
	lpx_set_mat_row( model, numrows, count, indices_adj, elements_adj );
	delete [] indices_adj;
	delete [] elements_adj;

# if OGSI_TRACK_FRESH > 0
  std::cout
    << "OGSI(" << std::hex << this << std::dec
    << ")::addRow: new row." << std::endl ;
# endif

}

//-----------------------------------------------------------------------------

void
OGSI::addRow(const CoinPackedVectorBase& vec,
			       const char rowsen, const double rowrhs,
			       const double rowrng)
{
	// Could be in OsiSolverInterfaceImpl.
	double lb = 0.0 ;
	double ub = 0.0 ;
	convertSenseToBound( rowsen, rowrhs, rowrng, lb, ub);
	addRow( vec, lb, ub );
}

//-----------------------------------------------------------------------------

void
OGSI::addRows(const int numrows, 
				const CoinPackedVectorBase * const * rows,
				const double* rowlb, const double* rowub)
{
  // ??? Could do this more efficiently now
	// Could be in OsiSolverInterfaceImpl.
	int i;
	for( i = 0; i < numrows; ++i )
		addRow( *(rows[i]), rowlb[i], rowub[i] );
}

//-----------------------------------------------------------------------------

void
OGSI::addRows(const int numrows,
				const CoinPackedVectorBase * const * rows,
				const char* rowsen, const double* rowrhs,
				const double* rowrng)
{
	// Could be in OsiSolverInterfaceImpl.
	int i;
	for( i = 0; i < numrows; ++i )
		addRow( *(rows[i]), rowsen[i], rowrhs[i], rowrng[i] );
}

//-----------------------------------------------------------------------------

/*
  There's an expectation that a valid basis will be maintained across row
  deletions. Fortunately, glpk will do this automagically as long as we play
  by the rules and delete only slack constraints. If we delete a constraint
  with a nonbasic slack, we're in trouble.
*/
void OGSI::deleteRows (const int num, const int *osiIndices)

{ int *glpkIndices = new int[num+1] ;
  int i,ndx ;
/*
  Arguably, column results remain valid across row deletion.
*/
  freeCachedData(OGSI::KEEPCACHED_COLUMN) ;
/*
  Glpk uses 1-based indexing, so convert the array of indices. While we're
  doing that, delete the row names.
*/
  for (ndx = 0 ; ndx < num ; ndx++)
  { glpkIndices[ndx+1] = osiIndices[ndx]+1 ;
    deleteRowNames(osiIndices[ndx],1) ; }
/*
  See if we're about to do damage. If we delete a row with a nonbasic slack,
  we'll have an excess of basic variables.
*/
  int notBasic = 0 ;
  for (ndx = 1 ; ndx <= num ; ndx++)
  { i = glpkIndices[ndx] ;
    int stati = lpx_get_row_stat(lp_,i) ;
    if (stati != LPX_BS)
    { notBasic++ ; } }
  if (notBasic)
  {
#  if OGSI_VERBOSITY > 1
    std::cout
      << "OGSI(" << std::hex << this << std::dec
      << ")::deleteRows: deleting " << notBasic << " tight constraints; "
      << "basis is no longer valid." << std::endl ;
#  endif
  }
/*
  Tell glpk to delete the rows.
*/
  lpx_del_rows(lp_,num,glpkIndices) ;

  delete [] glpkIndices ;

# if OGSI_TRACK_FRESH > 0
  std::cout
    << "OGSI(" << std::hex << this << std::dec
    << ")::deleteRows: deleted " << num << " rows." << std::endl ;
# endif

  return ; }

//#############################################################################
// Methods to input a problem
//#############################################################################

void OGSI::loadProblem (const CoinPackedMatrix &matrix,
			const double *collb_parm, const double *colub_parm,
			const double *obj_parm,
			const double *rowlb_parm, const double *rowub_parm)

{ 
#   if OGSI_TRACK_FRESH > 0
    std::cout
      << "OGSI(" << std::hex << this << std::dec << ")::loadProblem."
      << std::endl ;
#   endif
/*
  There's always an existing LPX object. If it's empty, we can simply load in
  the new data. If it's non-empty, easiest to delete it and start afresh. When
  we do this, we need to reload parameters. For hints, it's easiest to grab
  from the existing LPX structure.
  
  In any event, get rid of cached data in the OsiGlpk object.
*/
  if (lpx_get_num_cols(lp_) != 0 || lpx_get_num_rows(lp_) != 0)
  { 
    int presolVal = lpx_get_int_parm(lp_,LPX_K_PRESOL) ;
    int usedualVal = lpx_get_int_parm(lp_,LPX_K_DUAL) ;
    int scaleVal = lpx_get_int_parm(lp_,LPX_K_SCALE) ;
    int logVal = lpx_get_int_parm(lp_,LPX_K_MSGLEV) ;
#   if OGSI_TRACK_FRESH > 0
    std::cout
      << "    emptying LPX(" << std::hex << lp_ << std::dec << "), "
#   endif
    lpx_delete_prob(lp_) ;
    lp_ = lpx_create_prob() ;
    assert(lp_) ;
#   if OGSI_TRACK_FRESH > 0
    std::cout
      << "loading LPX(" << std::hex << lp_ << std::dec << ")."
      << std::endl ;
#   endif
    lpx_set_class(lp_,LPX_MIP) ;
    lpx_set_int_parm(lp_,LPX_K_ITLIM,maxIteration_) ; 
    if (getObjSense() == 1)				// minimization
    { lpx_set_real_parm(lp_,LPX_K_OBJUL,dualObjectiveLimit_) ;
      lpx_set_real_parm(lp_,LPX_K_OBJLL,primalObjectiveLimit_) ; }
    else						// maximization
    { lpx_set_real_parm(lp_,LPX_K_OBJLL,dualObjectiveLimit_) ;
      lpx_set_real_parm(lp_,LPX_K_OBJUL,primalObjectiveLimit_) ; }
    lpx_set_real_parm(lp_,LPX_K_TOLDJ,dualTolerance_) ;
    lpx_set_real_parm(lp_,LPX_K_TOLBND,primalTolerance_) ;
    lpx_set_obj_coef(lp_,0,objOffset_) ;
    lpx_set_prob_name(lp_,const_cast<char *>(probName_.c_str())) ;
    lpx_set_int_parm(lp_,LPX_K_PRESOL,presolVal) ;
    lpx_set_int_parm(lp_,LPX_K_DUAL,usedualVal) ;
    lpx_set_int_parm(lp_,LPX_K_SCALE,scaleVal) ;
    lpx_set_int_parm(lp_,LPX_K_MSGLEV,logVal) ;
    messageHandler()->setLogLevel(logVal) ; }

  freeCachedData(OGSI::KEEPCACHED_NONE) ;

  double inf = getInfinity() ;
  int m = matrix.getNumRows() ;
  int n = matrix.getNumCols() ;
  int i,j ;
  double *zeroVec,*infVec,*negInfVec ;
  const double *collb,*colub,*obj,*rowlb,*rowub ;
/*
  Check if we need default values for any of the vectors, and set up
  accordingly.
*/
  if (collb_parm == 0 || obj_parm == 0)
  { zeroVec = new double [n] ;
    CoinZeroN(zeroVec,n) ; }
  else
  { zeroVec = 0 ; }

  if (colub_parm == 0 || rowub_parm == 0)
  { if (colub_parm == 0 && rowub_parm == 0)
    { j = CoinMax(m,n) ; }
    else
    if (colub_parm == 0)
    { j = n ; }
    else
    { j = m ; }
    infVec = new double [j] ;
    CoinFillN(infVec,j,inf) ; }
  else
  { infVec = 0 ; }

  if (rowlb_parm == 0)
  { negInfVec = new double [m] ;
    CoinFillN(negInfVec,m,-inf) ; }
  else
  { negInfVec = 0 ; }

  if (collb_parm == 0)
  { collb = zeroVec ; }
  else
  { collb = collb_parm ; }

  if (colub_parm == 0)
  { colub = infVec ; }
  else
  { colub = colub_parm ; }

  if (obj_parm == 0)
  { obj = zeroVec ; }
  else
  { obj = obj_parm ; }

  if (rowlb_parm == 0)
  { rowlb = negInfVec ; }
  else
  { rowlb = rowlb_parm ; }

  if (rowub_parm == 0)
  { rowub = infVec ; }
  else
  { rowub = rowub_parm ; }
/*
  The actual load.
*/
  if (matrix.isColOrdered())
  { for (j = 0 ; j < n ; j++)
    { const CoinShallowPackedVector reqdBySunCC = matrix.getVector(j) ;
      addCol(reqdBySunCC,collb[j],colub[j],obj[j]) ; }
    // Make sure there are enough rows 
    if (m > getNumRows())
    { lpx_add_rows(lp_,m-getNumRows()) ; }
    for (i = 0 ; i < m ; i++)
    { setRowBounds(i,rowlb[i],rowub[i]) ; } }
  else
  { for (i = 0 ; i < m ; i++)
    { const CoinShallowPackedVector reqdBySunCC = matrix.getVector(i) ;
      addRow(reqdBySunCC,rowlb[i],rowub[i]) ; }
    // Make sure there are enough columns
    if (n > getNumCols())
    { lpx_add_cols(lp_,n-getNumCols()) ; }
    for (j = 0 ; j < n ; j++)
    { setColBounds(j,collb[j],colub[j]) ;
      setObjCoeff(j,obj[j]) ; } }
/*
  Cleanup.
*/
  if (zeroVec) delete[] zeroVec ;
  if (infVec) delete[] infVec ;
  if (negInfVec) delete[] negInfVec ;

  return ; }

//-----------------------------------------------------------------------------

void
OGSI::assignProblem( CoinPackedMatrix*& matrix,
				       double*& collb, double*& colub,
				       double*& obj,
				       double*& rowlb, double*& rowub )
{
	// Could be in OsiSolverInterfaceImpl.
	loadProblem( *matrix, collb, colub, obj, rowlb, rowub );
	delete matrix;
	matrix = NULL;
	delete[] collb;
	collb = NULL;
	delete[] colub;
	colub = NULL;
	delete[] obj;
	obj = NULL;
	delete[] rowlb;
	rowlb = NULL;
	delete[] rowub;
	rowub = NULL;
}

//-----------------------------------------------------------------------------

void
OGSI::loadProblem( const CoinPackedMatrix& matrix,
				     const double* collb, const double* colub,
				     const double* obj,
				     const char* rowsen, const double* rowrhs,
				     const double* rowrng )
{
	// Could be in OsiSolverInterfaceImpl.
	int numrows = matrix.getNumRows();
	double * rowlb = new double[numrows];
	double * rowub = new double[numrows];

	int i;
	for( i = numrows - 1; i >= 0; --i )
	{
		convertSenseToBound( rowsen ? rowsen[i]:'G', 
				     rowrhs ? rowrhs[i]:0.0, 
				     rowrng ? rowrng[i]:0.0, 
				     rowlb[i], rowub[i] );
	}

	loadProblem( matrix, collb, colub, obj, rowlb, rowub );
	delete [] rowlb;
	delete [] rowub;
}

//-----------------------------------------------------------------------------

void
OGSI::assignProblem( CoinPackedMatrix*& matrix,
				       double*& collb, double*& colub,
				       double*& obj,
				       char*& rowsen, double*& rowrhs,
				       double*& rowrng )
{
	// Could be in OsiSolverInterfaceImpl.
	loadProblem( *matrix, collb, colub, obj, rowsen, rowrhs, rowrng );
	delete matrix;
	matrix = NULL;
	delete[] collb;
	collb = NULL;
	delete[] colub;
	colub = NULL;
	delete[] obj;
	obj = NULL;
	delete[] rowsen;
	rowsen = NULL;
	delete[] rowrhs;
	rowrhs = NULL;
	delete[] rowrng;
	rowrng = NULL;
}
//-----------------------------------------------------------------------------

void
OGSI::loadProblem(const int numcols, const int numrows,
				   const int* start, const int* index,
				   const double* value,
				   const double* collb, const double* colub,
				   const double* obj,
				   const double* rowlb, const double* rowub)
{
  freeCachedData( OGSI::KEEPCACHED_NONE );
  LPX *model = getMutableModelPtr();
  double inf = getInfinity();

  // Can't send 0 to lpx_add_xxx
  if (numcols > 0)
    lpx_add_cols( model, numcols );
  if (numrows > 0)
    lpx_add_rows( model, numrows );

  // How many elements?  Column-major, so indices of start are columns
  int numelem = start[ numcols ];
  //  int numelem = 0;
  //  while ( index[numelem] != 0 )
  //    numelem++;
  int *index_adj = new int[1+numelem];
  double *value_adj = new double[1+numelem];

  int i;
  for ( i=1; i <= numelem; i++)
    {
      index_adj[i] = index[i-1] + 1;
      value_adj[i] = value[i-1];
    }

  for( i = 0; i < numcols; i++ )
  {
	setColBounds( i, collb ? collb[i]:0.0, 
		    colub ? colub[i]:inf );
	lpx_set_mat_col( model, i+1, start[i+1]-start[i], &(index_adj[start[i]]), &(value_adj[start[i]]) );
    setObjCoeff( i, obj ? obj[i]:0.0 );
  }
  int j;
  for( j = 0; j < numrows; j++ )
  {
      setRowBounds( j, rowlb ? rowlb[j]:-inf, rowub ? rowub[j]:inf );
  }

  delete [] index_adj;
  delete [] value_adj;
  
}
//-----------------------------------------------------------------------------

void
OGSI::loadProblem(const int numcols, const int numrows,
				   const int* start, const int* index,
				   const double* value,
				   const double* collb, const double* colub,
				   const double* obj,
				   const char* rowsen, const double* rowrhs,   
				   const double* rowrng)
{
   double * rowlb = new double[numrows];
   double * rowub = new double[numrows];
   for (int i = numrows-1; i >= 0; --i) {   
      convertSenseToBound(rowsen != NULL ? rowsen[i] : 'G', rowrhs != NULL ? rowrhs[i] : 0.0, rowrng != NULL ? rowrng[i] : 0.0, rowlb[i], rowub[i]);
   }

   loadProblem( numcols, numrows, start, index, value, collb, colub, obj,
		rowlb, rowub );

   delete[] rowlb;
   delete[] rowub;
}

//-----------------------------------------------------------------------------
// Read mps files
//-----------------------------------------------------------------------------

/*
  Call OSI::readMps, which will parse the mps file and call
  OsiGlpk::loadProblem.
*/
int OGSI::readMps (const char *filename,
				     const char *extension)
{

  int retval = OsiSolverInterface::readMps(filename,extension);

  return (retval) ;
}

//-----------------------------------------------------------------------------
// Write mps files
//-----------------------------------------------------------------------------

void OGSI::writeMps( const char * filename,
				       const char * extension,
		     double /*objSense*/ ) const
{
	// Could be in OsiSolverInterfaceImpl.
#if 1
	std::string f( filename );
	std::string e( extension );
	std::string fullname = f + "." + e;
	lpx_write_mps( getMutableModelPtr(), const_cast<char*>( fullname.c_str() ));
#else
	// Fall back on native MPS writer.  
	// These few lines of code haven't been tested. 2004/10/15
	std::string f( filename );
	std::string e( extension );
	std::string fullname = f + "." + e;

	OsiSolverInterface::writeMpsNative(fullname.c_str(), 
					   NULL, NULL, 0, 2, objSense); 
#endif
}

//############################################################################
// GLPK-specific methods
//############################################################################

// Get a pointer to the instance
LPX * OGSI::getModelPtr ()
{
  freeCachedResults();
  return lp_;
}

//#############################################################################
// Constructors, destructors clone and assignment
//#############################################################################

//-----------------------------------------------------------------------------
// Default Constructor
//-----------------------------------------------------------------------------

OGSI::OsiGlpkSolverInterface ()
: 
OsiSolverInterface()
{
	gutsOfConstructor();
  incrementInstanceCounter();

# if OGSI_TRACK_SOLVERS > 0
  std::cout
    << "OGSI(" << std::hex << this << std::dec
    << "): default constructor." << std::endl ;
# endif
}

//-----------------------------------------------------------------------------
// Clone
//-----------------------------------------------------------------------------

OsiSolverInterface * OGSI::clone(bool copyData) const
{
# if OGSI_TRACK_SOLVERS > 0
  std::cout
    << "OGSI(" << std::hex << this << std::dec << "): cloning." << std::endl ;
# endif
  if (copyData)
    return( new OsiGlpkSolverInterface( *this ) );
  else
    return( new OsiGlpkSolverInterface() );
}

//-----------------------------------------------------------------------------
// Copy constructor
//-----------------------------------------------------------------------------

OGSI::OsiGlpkSolverInterface( const OsiGlpkSolverInterface & source )
: 
OsiSolverInterface(source)
{
	gutsOfConstructor();
	gutsOfCopy( source );
  incrementInstanceCounter();

# if OGSI_TRACK_SOLVERS > 0
  std::cout
    << "OGSI(" << std::hex << this << "): copy from "
    << &source << std::dec << "." << std::endl ;
# endif
}

//-----------------------------------------------------------------------------
// Destructor
//-----------------------------------------------------------------------------

OGSI::~OsiGlpkSolverInterface ()
{
	gutsOfDestructor();
  decrementInstanceCounter();

# if OGSI_TRACK_SOLVERS > 0
  std::cout
    << "OGSI(" << std::hex << this << std::dec
    << "): destructor." << std::endl ;
# endif
}

// Resets 
// ??? look over this carefully to be sure it is correct
void OGSI::reset()
{
  setInitialData();  // this is from the base class OsiSolverInterface
  gutsOfDestructor();
  gutsOfConstructor() ;

# if OGSI_TRACK_SOLVERS > 0
  std::cout
    << "OGSI(" << std::hex << this << std::dec
    << "): reset." << std::endl ;
# endif

  return ;

}


//-----------------------------------------------------------------------------
// Assignment operator
//-----------------------------------------------------------------------------

OsiGlpkSolverInterface& OGSI::operator=( const OsiGlpkSolverInterface& rhs )
{
	if( this != &rhs )
	{
		OsiSolverInterface::operator=( rhs );
		gutsOfDestructor();
		gutsOfConstructor();
		if( rhs.lp_ != NULL )
			gutsOfCopy( rhs );
	}

# if OGSI_TRACK_SOLVERS > 0
  std::cout
    << "OGSI(" << std::hex << this << "): assign from "
    << &rhs << std::dec << "." << std::endl ;
# endif

	return *this;
}

//#############################################################################
// Applying cuts
//#############################################################################

void OGSI::applyColCut( const OsiColCut & cc )
{
	// Could be in OsiSolverInterfaceImpl.
	const double * colLb = getColLower();
	const double * colUb = getColUpper();
	const CoinPackedVector & lbs = cc.lbs();
	const CoinPackedVector & ubs = cc.ubs();
	int i;
#if 0
        // replaced (JJF) because colLb and colUb are invalidated by sets
	for( i = 0; i < lbs.getNumElements(); ++i )
		if( lbs.getElements()[i] > colLb[lbs.getIndices()[i]] )
			setColLower( lbs.getIndices()[i], lbs.getElements()[i] );
		for( i = 0; i < ubs.getNumElements(); ++i )
			if( ubs.getElements()[i] < colUb[ubs.getIndices()[i]] )
				setColUpper( ubs.getIndices()[i], ubs.getElements()[i] );
#else
	double inf = getInfinity();

	int type;
        // lower bounds
	for( i = 0; i < lbs.getNumElements(); ++i ) {
          int column = lbs.getIndices()[i];
          double lower = lbs.getElements()[i];
          double upper = colUb[column];
          if( lower > colLb[column] ) {
            // update cached version as well
            collower_[column] = lower;
            if( lower == upper )
              type = LPX_FX;
            else if( lower > -inf && upper < inf )
              type = LPX_DB;
            else if( lower > -inf )
              type = LPX_LO;
            else if( upper < inf)
              type = LPX_UP;
            else
              type = LPX_FR;
            
            lpx_set_col_bnds( getMutableModelPtr(), column+1, type, lower, upper );
          }
        }
        // lower bounds
	for( i = 0; i < ubs.getNumElements(); ++i ) {
          int column = ubs.getIndices()[i];
          double upper = ubs.getElements()[i];
          double lower = colLb[column];
          if( upper < colUb[column] ) {
            // update cached version as well
            colupper_[column] = upper;
            if( lower == upper )
              type = LPX_FX;
            else if( lower > -inf && upper < inf )
              type = LPX_DB;
            else if( lower > -inf )
              type = LPX_LO;
            else if( upper < inf)
              type = LPX_UP;
            else
              type = LPX_FR;
            
            lpx_set_col_bnds( getMutableModelPtr(), column+1, type, lower, upper );
          }
        }
#endif
}

//-----------------------------------------------------------------------------

void OGSI::applyRowCut( const OsiRowCut & rowCut )
{
	// Could be in OsiSolverInterfaceImpl.
	addRow( rowCut.row(), rowCut.lb(), rowCut.ub() );
}

//#############################################################################
// Private methods (non-static and static) and static data
//#############################################################################

LPX * OGSI::getMutableModelPtr( void ) const
{
  return lp_;
}

/*
  This routine must work even if there's no problem loaded in the solver.
*/

void OGSI::gutsOfCopy (const OsiGlpkSolverInterface &source)

{ LPX *srclpx = source.lp_ ;
  LPX *lpx = lp_ ;
  double dblParam ;
  int intParam ;
  std::string strParam ;
/*
  Copy information from the source OSI that a user might change before
  loading a problem: objective sense and offset, other OSI parameters.  Use
  the get/set parameter calls here to hide pushing information into the LPX
  object.
*/
  setObjSense(source.getObjSense()) ;
  source.getDblParam(OsiObjOffset,dblParam) ;
  setDblParam(OsiObjOffset,dblParam) ;

  source.getIntParam(OsiNameDiscipline,intParam) ;
  setIntParam(OsiNameDiscipline,intParam) ;

  source.getIntParam(OsiMaxNumIteration,intParam) ;
  setIntParam(OsiMaxNumIteration,intParam) ;
  source.getIntParam(OsiMaxNumIterationHotStart,intParam) ;
  setIntParam(OsiMaxNumIterationHotStart,intParam) ;
  
  source.getDblParam(OsiPrimalObjectiveLimit,dblParam) ;
  setDblParam(OsiPrimalObjectiveLimit,dblParam) ;
  source.getDblParam(OsiDualObjectiveLimit,dblParam) ;
  setDblParam(OsiDualObjectiveLimit,dblParam) ;
  source.getDblParam(OsiPrimalTolerance,dblParam) ;
  setDblParam(OsiPrimalTolerance,dblParam) ;
  source.getDblParam(OsiDualTolerance,dblParam) ;
  setDblParam(OsiDualTolerance,dblParam) ;
/*
  For hints, we need to be a little more circumspect, so as not to pump out a
  bunch of warnings. Pull parameters from the source LPX object and load into
  the copy. The actual values of the hint parameters (sense & strength) are
  held up on the parent OSI object, so we don't need to worry about copying
  them.
*/
  intParam = lpx_get_int_parm(srclpx,LPX_K_PRESOL) ;
  lpx_set_int_parm(lpx,LPX_K_PRESOL,intParam) ;
  intParam = lpx_get_int_parm(srclpx,LPX_K_DUAL) ;
  lpx_set_int_parm(lpx,LPX_K_DUAL,intParam) ;
  intParam = lpx_get_int_parm(srclpx,LPX_K_SCALE) ;
  lpx_set_int_parm(lpx,LPX_K_SCALE,intParam) ;
/*
  Printing is a bit more complicated. Pull the parameter and set the log
  level in the message handler and set the print parameter in glpk.
*/
  intParam = lpx_get_int_parm(srclpx,LPX_K_MSGLEV) ;
  lpx_set_int_parm(lpx,LPX_K_MSGLEV,intParam) ;
  messageHandler()->setLogLevel(intParam) ;

# ifdef LPX_K_USECUTS
  intParam=lpx_get_int_parm(lp_,LPX_K_USECUTS);
  lpx_set_int_parm(lp_,LPX_K_USECUTS,intParam);
# endif
  
/*
  Now --- do we have a problem loaded? If not, we're done.
*/
  int n = source.getNumCols() ;
  int m = source.getNumRows() ;
  assert(m >= 0 && n >= 0) ;
  if (m == 0 && n == 0)
  { 
#   if OGSI_TRACK_SOLVERS > 0
    std::cout
      << "    no problem loaded." << std::endl ;
#   endif
    return ; }
/*
  We have rows and/or columns, so we need to transfer the problem. Do a few
  parameters and status fields that may have changed if a problem is loaded.
  Then pull the problem information and load it into the lpx object for this
  OSI. Information on integrality must be copied over separately.
*/
  source.getStrParam(OsiProbName,strParam) ;
  setStrParam(OsiProbName,strParam) ;
  bbWasLast_ = source.bbWasLast_ ;
  iter_used_ = source.iter_used_ ;

  const double *obj = source.getObjCoefficients() ;
  const CoinPackedMatrix *cols = source.getMatrixByCol() ;
  const double *lb = source.getColLower() ;
  const double *ub = source.getColUpper() ;
  const double *rlb = source.getRowLower() ;
  const double *rub = source.getRowUpper() ;
  loadProblem(*cols,lb,ub,obj,rlb,rub) ;

  int i,j ;
  for (j = 0 ; j < n ; j++)
  { if (source.isInteger(j))
    { setInteger(j) ; } }
/*
  Copy the solution information. We need to be a bit careful here. Even though
  the source has something loaded as a problem, there's no guarantee that we've
  even called glpk, so we can't consult glpk directly for solution values.
*/
  setColSolution(source.getColSolution()) ;
  setRowPrice(source.getRowPrice()) ;
/* 
  We can, however, consult it directly for variable status: glpk initialises
  this information as soon as columns and/or rows are created. Of course, that
  applies to the problem we've just created, too. If we're just copying
  nonsense, then don't bother. Once we've copied the status into the new lpx
  object, do the warm-up.
*/
  if (lpx_get_status(srclpx) != LPX_UNDEF)
  { for (j = 1 ; j <= n ; j++)
    { int statj = lpx_get_col_stat(srclpx,j) ;
      lpx_set_col_stat(lpx,j,statj) ; }
    for (i = 1 ; i <= m ; i++)
    { int stati = lpx_get_row_stat(srclpx,i) ;
      lpx_set_row_stat(lpx,i,stati) ; }

#ifndef NDEBUG
    int retval = lpx_warm_up(lpx) ;
#endif
#   if OGSI_TRACK_SOLVERS > 1
    std::cout
      << "    lpx_warm_up returns " << retval << "." << std::endl ;
#   endif
    assert(retval == LPX_E_OK) ; }

  return ; }


//-----------------------------------------------------------------------------

void OGSI::gutsOfConstructor()
{
  bbWasLast_ = 0;
  iter_used_ = 0;
  obj_ = NULL;
  collower_ = NULL;
  colupper_ = NULL;
  ctype_ = NULL;
  rowsense_ = NULL;
  rhs_ = NULL;
  rowrange_ = NULL;
  rowlower_ = NULL;
  rowupper_ = NULL;
  colsol_ = NULL;
  rowsol_ = NULL;
  redcost_ = NULL;
  rowact_ = NULL;
  matrixByRow_ = NULL;
  matrixByCol_ = NULL;

  maxIteration_ = COIN_INT_MAX;
  hotStartMaxIteration_ = 0;
  nameDisc_ = 0;

  dualObjectiveLimit_ = getInfinity() ;
  primalObjectiveLimit_ = -getInfinity() ;
  dualTolerance_ = 1.0e-6;
  primalTolerance_ = 1.0e-6;
  objOffset_ = 0.0 ;

  probName_ = "<none loaded>" ;

  hotStartCStat_ = NULL;
  hotStartCStatSize_ = 0;
  hotStartRStat_ = NULL;
  hotStartRStatSize_ = 0;

  isIterationLimitReached_ = false;
  isTimeLimitReached_ = false;
  isAbandoned_ = false;
  isPrimInfeasible_ = false;
  isDualInfeasible_ = false;
  isObjLowerLimitReached_ = false ;
  isObjUpperLimitReached_ = false ;
  isFeasible_ = false;

  lp_ = lpx_create_prob();
  assert( lp_ != NULL );
  // Make all problems MIPs.  See note at top of file.
  lpx_set_class( lp_, LPX_MIP );

  // Push OSI parameters down into LPX object.
  lpx_set_int_parm(lp_,LPX_K_ITLIM,maxIteration_) ; 

  if (getObjSense() == 1.0)				// minimization
  { lpx_set_real_parm(lp_,LPX_K_OBJUL,dualObjectiveLimit_) ;
    lpx_set_real_parm(lp_,LPX_K_OBJLL,primalObjectiveLimit_) ; }
  else						// maximization
  { lpx_set_real_parm(lp_,LPX_K_OBJLL,dualObjectiveLimit_) ;
    lpx_set_real_parm(lp_,LPX_K_OBJUL,-primalObjectiveLimit_) ; }
  lpx_set_real_parm(lp_,LPX_K_TOLDJ,dualTolerance_) ;
  lpx_set_real_parm(lp_,LPX_K_TOLBND,primalTolerance_) ;

  lpx_set_obj_coef(lp_,0,objOffset_) ;

  lpx_set_prob_name(lp_,const_cast<char *>(probName_.c_str())) ;

/* Stefan: with the new simplex algorithm in Glpk 4.31, some netlib instances (e.g., dfl001)
   take very long or get into a cycle.
   Thus, let's try to leave parameters onto their defaults.
*/
//  lpx_set_int_parm(lp_,LPX_K_PRESOL,0) ;
//  lpx_set_int_parm(lp_,LPX_K_DUAL,1) ;
//  lpx_set_int_parm(lp_,LPX_K_SCALE,3) ;
/*
  Printing is a bit more complicated. Set the log level in the handler and set
  the print parameter in glpk.
*/
  lpx_set_int_parm(lp_,LPX_K_MSGLEV,1) ;
  messageHandler()->setLogLevel(1) ;
  
/*
  Enable cuts if they're available. This is a bit of a pain,
  as the interface has changed since it was first introduced in 4.9.
  LPX_K_USECUTS appears in 4.9; the parameter value
  appears to be unused in this version. LPX_C_ALL appears in 4.10.
*/
# ifdef LPX_K_USECUTS
# ifdef LPX_C_ALL
  lpx_set_int_parm(lp_,LPX_K_USECUTS,LPX_C_ALL) ;
# else
  lpx_set_int_parm(lp_,LPX_K_USECUTS,0) ;
# endif
# endif
}

//-----------------------------------------------------------------------------

void OGSI::gutsOfDestructor()
{
	if( lp_ != NULL )
	{
		lpx_delete_prob( lp_ );
		lp_=NULL;
		freeAllMemory();
	}
	assert( lp_ == NULL );
	assert( obj_ == NULL );
	assert( collower_ == NULL );
	assert( colupper_ == NULL );
	assert( ctype_ == NULL );
	assert( rowsense_ == NULL );
	assert( rhs_ == NULL );
	assert( rowrange_ == NULL );
	assert( rowlower_ == NULL );
	assert( rowupper_ == NULL );
	assert( colsol_ == NULL );
	assert( rowsol_ == NULL );
	assert( redcost_ == NULL );
	assert( rowact_ == NULL );
	assert( matrixByRow_ == NULL );
	assert( matrixByCol_ == NULL );
}

//-----------------------------------------------------------------------------
// free cached vectors
//-----------------------------------------------------------------------------

void OGSI::freeCachedColRim()
{
	delete [] ctype_;
	delete [] obj_;
	delete [] collower_;
	delete [] colupper_;
	ctype_ = NULL;
	obj_ = NULL;
	collower_ = NULL;
	colupper_ = NULL;
}

//-----------------------------------------------------------------------------

void OGSI::freeCachedRowRim()
{
	delete [] rowsense_;
	delete [] rhs_;
	delete [] rowrange_;
	delete [] rowlower_;
	delete [] rowupper_;
	rowsense_ = NULL;
	rhs_ = NULL;
	rowrange_ = NULL;
	rowlower_ = NULL;
	rowupper_ = NULL;
}

//-----------------------------------------------------------------------------

void OGSI::freeCachedMatrix()
{
	delete matrixByRow_;
	delete matrixByCol_;
	matrixByRow_ = NULL;
	matrixByCol_ = NULL;
}

//-----------------------------------------------------------------------------

void OGSI::freeCachedResults()
{
        iter_used_ = 0;
	isAbandoned_ = false;
	isIterationLimitReached_ = false;
	isTimeLimitReached_ = false;
	isPrimInfeasible_ = false;
	isDualInfeasible_ = false;
	isFeasible_ = false;
	delete [] colsol_;
	delete [] rowsol_;
	delete [] redcost_;
	delete [] rowact_;
	colsol_ = NULL;
	rowsol_ = NULL;
	redcost_ = NULL;
	rowact_ = NULL;
}

//-----------------------------------------------------------------------------

void OGSI::freeCachedData( int keepCached )
{
	if( !(keepCached & OGSI::KEEPCACHED_COLUMN) )
		freeCachedColRim();
	if( !(keepCached & OGSI::KEEPCACHED_ROW) )
		freeCachedRowRim();
	if( !(keepCached & OGSI::KEEPCACHED_MATRIX) )
		freeCachedMatrix();
	if( !(keepCached & OGSI::KEEPCACHED_RESULTS) )
		freeCachedResults();
}

//-----------------------------------------------------------------------------

void OGSI::freeAllMemory()
{
	freeCachedData(OGSI::KEEPCACHED_NONE);
	delete[] hotStartCStat_;
	delete[] hotStartRStat_;
	hotStartCStat_ = NULL;
	hotStartCStatSize_ = 0;
	hotStartRStat_ = NULL;
	hotStartRStatSize_ = 0;
}

//-----------------------------------------------------------------------------

/*!
  Set the objective function name.
*/
void OGSI::setObjName (std::string name)

{ OsiSolverInterface::setObjName(name) ;
  lpx_set_obj_name(lp_,const_cast<char *>(name.c_str())) ; }

/*!
  Set a row name. Make sure both glpk and OSI see the same name.
*/
void OGSI::setRowName (int ndx, std::string name)

{ int nameDiscipline ;
/*
  Quietly do nothing if the index is out of bounds.
*/
  if (ndx < 0 || ndx >= getNumRows())
  { return ; }
/*
  Get the name discipline. Quietly do nothing if it's auto.
*/
  (void) getIntParam(OsiNameDiscipline,nameDiscipline) ;
  if (nameDiscipline == 0)
  { return ; }
/*
  Set the name in the OSI base, then in the consys structure.
*/
  OsiSolverInterface::setRowName(ndx,name) ;
  lpx_set_row_name(lp_,ndx+1,const_cast<char *>(name.c_str())) ;

  return ; }

/*!
  Set a column name. Make sure both glpk and OSI see the same name.
*/
void OGSI::setColName (int ndx, std::string name)

{ int nameDiscipline ;
/*
  Quietly do nothing if the index is out of bounds.
*/
  if (ndx < 0 || ndx >= getNumCols())
  { return ; }
/*
  Get the name discipline. Quietly do nothing if it's auto.
*/
  (void) getIntParam(OsiNameDiscipline,nameDiscipline) ;
  if (nameDiscipline == 0)
  { return ; }
/*
  Set the name in the OSI base, then in the consys structure.
*/
  OsiSolverInterface::setColName(ndx,name) ;
  lpx_set_col_name(lp_,ndx+1,const_cast<char *>(name.c_str())) ;

  return ; }


//-----------------------------------------------------------------------------

unsigned int OsiGlpkSolverInterface::numInstances_ = 0;

void OsiGlpkSolverInterface::decrementInstanceCounter()
{
  assert( numInstances_ != 0 );
  if ( --numInstances_ == 0 )
  	glp_free_env();
}
