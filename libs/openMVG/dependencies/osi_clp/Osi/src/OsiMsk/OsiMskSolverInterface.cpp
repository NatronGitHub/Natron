/* Osi interface for Mosek ver. 7.0
   Lower versions are not supported
 -----------------------------------------------------------------------------
  name:     OSI Interface for MOSEK
 -----------------------------------------------------------------------------

  This code is licensed under the terms of the Eclipse Public License (EPL).

  Originally developed by Bo Jensen from MOSEK ApS.  No longer a maintainer.
*/

#include <iostream>
#include <cassert>
#include <string>
#include <numeric>
#include "OsiConfig.h"
#include "CoinPragma.hpp"
#include "CoinError.hpp"
#include "CoinFinite.hpp"
#include "OsiMskSolverInterface.hpp"
#include "OsiRowCut.hpp"
#include "OsiColCut.hpp"
#include "CoinPackedMatrix.hpp"
#include "CoinWarmStartBasis.hpp"

#include "mosek.h"

#define MSK_OSI_DEBUG_LEVEL          0
#ifndef NDEBUG
#define MSK_OSI_ASSERT_LEVEL         0
#else
#define MSK_OSI_ASSERT_LEVEL         4
#endif
#define MSK_DO_MOSEK_LOG             0

#define debugMessage printf
  
//Choose algorithm to be default in initial solve
#define INITIAL_SOLVE MSK_OPTIMIZER_FREE

//Choose algorithm to be default in resolve
#define RESOLVE_SOLVE MSK_OPTIMIZER_FREE_SIMPLEX

// Unset this flag to disable the warnings from interface.

#define MSK_WARNING_ON

#undef getc

//#############################################################################
// A couple of helper functions
//#############################################################################

// Free memory pointet to by a double pointer

inline void freeCacheDouble( double*& ptr )
{
  if( ptr != NULL )
  {
    delete [] ptr;
    ptr = NULL;
  }
}

// Free memory pointet to by a char pointer

inline void freeCacheChar( char*& ptr )
{
  if( ptr != NULL )
  {
    delete [] ptr;
    ptr = NULL;
  }
}

// Free memory pointet to by a CoinPackedMatrix pointer 

inline void freeCacheMatrix( CoinPackedMatrix*& ptr )
{
  if( ptr != NULL )
  {
    delete ptr; 
    ptr = NULL;
  }
}

// Function used to connect MOSEK to stream
 
#if MSK_VERSION_MAJOR >= 7
static void MSKAPI printlog(void *ptr, const char* s)
#else
static void MSKAPI printlog(void *ptr, char* s)
#endif
{
  printf("%s",s);
} 

static 
void MSKAPI OsiMskStreamFuncLog(MSKuserhandle_t handle, MSKCONST char* str) {
	if (handle) {
		if (((CoinMessageHandler*)handle)->logLevel() >= 1)
			((CoinMessageHandler*)handle)->message(0, "MSK", str, ' ') << CoinMessageEol;
	} else {
		printf(str);
		printf("\n");
	}
}

static 
void MSKAPI OsiMskStreamFuncWarning(MSKuserhandle_t handle, MSKCONST char* str) {
	if (handle) {
		if (((CoinMessageHandler*)handle)->logLevel() >= 0)
			((CoinMessageHandler*)handle)->message(0, "MSK", str, ' ') << CoinMessageEol;
	} else {
		printf(str);
		printf("\n");
	}
}

static 
void MSKAPI OsiMskStreamFuncError(MSKuserhandle_t handle, MSKCONST char* str) {
	if (handle) {
		((CoinMessageHandler*)handle)->message(0, "MSK", str, ' ') << CoinMessageEol;
	} else {
		fprintf(stderr, str);
		fprintf(stderr, "\n");
	}
}

// Prints a error message and throws a exception

static inline void
checkMSKerror( int err, std::string mskfuncname, std::string osimethod )
{
  if( err != MSK_RES_OK )
  {
    char s[100];
    sprintf( s, "%s returned error %d", mskfuncname.c_str(), err );
    std::cout << "ERROR: " << s << " (" << osimethod << 
    " in OsiMskSolverInterface)" << std::endl;
    throw CoinError( s, osimethod.c_str(), "OsiMskSolverInterface" );
  }
}

// Prints a error message and throws a exception

static inline void
MSKassert(int assertlevel, int test, std::string assertname, std::string osimethod )
{
  if( assertlevel > MSK_OSI_ASSERT_LEVEL && !test )
  {
    char s[100];
    sprintf( s, "%s", assertname.c_str());
    std::cout << "Assert: " << s << " (" << osimethod << 
    " in OsiMskSolverInterface)" << std::endl;
    throw CoinError( s, osimethod.c_str(), "OsiMskSolverInterface" );
  }
}


// Prints a warning message, can be shut off by undefining MSK_WARNING_ON

static inline void
OsiMSK_warning(std::string osimethod,  std::string warning)
{
   std::cout << "OsiMsk_warning: "<<warning<<" in "<<osimethod<< std::endl;
}

// Converts Range/Sense/Rhs to MSK bound structure

static inline void
MskConvertSenseToBound(const char rowsen, const double rowrng, 
                         const double rowrhs, double &rlb, double &rub, int &rtag)
{
  switch (rowsen) 
  {
    case 'E':
      rlb  = rowrhs;
      rub  = rowrhs;
      rtag = MSK_BK_FX;
    break;

    case 'L':
      rlb  = MSK_INFINITY;
      rub  = rowrhs;
      rtag = MSK_BK_UP;
      break;
    case 'G':
      rlb  = rowrhs;
      rub  = MSK_INFINITY;
      rtag = MSK_BK_LO;
    break;

    case 'R':
      if( rowrng >= 0 )
      {
        rlb = rowrhs - rowrng;
        rub = rowrhs;
      }
      else
      {
        rlb = rowrhs;
        rub = rowrhs + rowrng;
      }

      rtag = MSK_BK_RA;
      break;
    case 'N':
      rlb  = -MSK_INFINITY;
      rub  = MSK_INFINITY;
      rtag = MSK_BK_FR;
      break;
      
    default:
      MSKassert(3,1,"Unknown rowsen","MskConvertSenseToBound");
  } 
}

// Converts a set of bounds to MSK boundkeys
 
static inline void
MskConvertColBoundToTag(const double collb, const double colub, double &clb, double &cub, int &ctag)
{
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin MskConvertColBoundToTag()\n");
  #endif
  
  if(collb > -MSK_INFINITY && colub < MSK_INFINITY)
  {
    ctag = MSK_BK_RA;
    clb = collb;
    cub = colub;
  }
  else if(collb <= - MSK_INFINITY && colub < MSK_INFINITY)
  {
    ctag = MSK_BK_UP;
    clb = -MSK_INFINITY;
    cub = colub;
  }
  else if(collb > - MSK_INFINITY && colub >= MSK_INFINITY)
  {
    ctag = MSK_BK_LO;
    clb = collb;
    cub = MSK_INFINITY;
  }
  else if(collb <= -MSK_INFINITY && colub >= MSK_INFINITY)
  {
    ctag = MSK_BK_FR;
    clb = -MSK_INFINITY;
    cub = MSK_INFINITY;
  } 

  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("End MskConvertColBoundToTag()\n");
  #endif
}

// Returns true if "solution" is defined in MOSEK, where solution can be basic, interior or 
// integer resp. (MSK_SOL_BAS), (MSK_SOL_ITR) or (MSK_SOL_ITG).

bool OsiMskSolverInterface::definedSolution(int solution) const
{
   #if MSK_OSI_DEBUG_LEVEL > 3
   debugMessage("Begin OsiMskSolverInterface::definedSolution() %p\n",(void*)this);
   #endif

   
   int err, res;
   err = MSK_solutiondef(getMutableLpPtr(), (MSKsoltypee) solution, &res);
   checkMSKerror(err,"MSK_solutiondef","definedSolution");

   #if MSK_OSI_DEBUG_LEVEL > 3
   debugMessage("End OsiMskSolverInterface::definedSolution()\n");
   #endif


   return ( res != MSK_RES_OK);
}

// Returns the flag for solver currently switched on in MOSEK resp. (MSK_OPTIMIZER_FREE),
// (MSK_OPTIMIZER_INTPNT), (MSK_OPTIMIZER_PRIMAL_SIMPLEX), (MSK_OPTIMIZER_MIXED_INT), or
// (MSK_OPTIMIZER_MIXED_INT_CONIC).
// MOSEK also has Conic and nonconvex solvers, but these are for obvious reasons not 
// an option in the Osi interface.

int OsiMskSolverInterface::solverUsed() const
{
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::solverUsed()\n");
  #endif

  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("End OsiMskSolverInterface::solverUsed()\n");
  #endif

  return MSKsolverused_;
}

// Switch the MOSEK solver to LP uses default solver specified by 'InitialSolver' 

void
OsiMskSolverInterface::switchToLP( void )
{
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::switchToLP()\n");
  #endif

  int err = MSK_putintparam(getMutableLpPtr(), MSK_IPAR_MIO_MODE, MSK_MIO_MODE_IGNORED);
  checkMSKerror(err,"MSK_putintparam","switchToLp");

  probtypemip_ = false;
  
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("End OsiMskSolverInterface::switchToLP()\n");
  #endif
}

// Switch the MOSEK solver to MIP. 

void
OsiMskSolverInterface::switchToMIP( void )
{
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::switchToMIP()\n");
  #endif

  int err = MSK_putintparam(getMutableLpPtr(), MSK_IPAR_MIO_MODE, MSK_MIO_MODE_SATISFIED);
  checkMSKerror(err,"MSK_putintparam","switchToMIP");

#if MSK_VERSION_MAJOR >= 7
  err = MSK_putintparam(getMutableLpPtr(), MSK_IPAR_OPTIMIZER, MSK_OPTIMIZER_MIXED_INT_CONIC);
#else
  err = MSK_putintparam(getMutableLpPtr(), MSK_IPAR_OPTIMIZER, MSK_OPTIMIZER_MIXED_INT);
#endif
  checkMSKerror(err,"MSK_putintparam","switchToMIP");
  probtypemip_ = true;
  
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("End OsiMskSolverInterface::switchToMIP()\n");
  #endif
}

// Resize the coltype array. 

void
OsiMskSolverInterface::resizeColType( int minsize )
{
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::resizeColType()\n");
  #endif

  if( minsize > coltypesize_ )
  {
     int newcoltypesize = 2*coltypesize_;
     if( minsize > newcoltypesize )
        newcoltypesize = minsize;
     char *newcoltype = new char[newcoltypesize];

     if( coltype_ != NULL )
     {
        CoinDisjointCopyN( coltype_, coltypesize_, newcoltype );
        delete[] coltype_;
     }
     
     coltype_ = newcoltype;
     coltypesize_ = newcoltypesize;
  }
  
  MSKassert(3,minsize == 0 || coltype_ != NULL,"Unknown rowsen","MskConvertSenseToBound");
  MSKassert(3,coltypesize_ >= minsize,"coltypesize_ >= minsize","MskConvertSenseToBound");

  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("End OsiMskSolverInterface::resizeColType()\n");
  #endif
}

// Free coltype array

void
OsiMskSolverInterface::freeColType()
{
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::freeColType()\n");
  #endif

  if( coltypesize_ > 0 )
  {
    delete[] coltype_;
    
    coltype_ = NULL;
    coltypesize_ = 0;
  }

  MSKassert(3,coltype_ == NULL,"coltype_ == NULL","freeColType");

  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("End OsiMskSolverInterface::freeColType()\n");
  #endif
} 

//#############################################################################
// Solve methods
//#############################################################################

//-----------------------------------------------------------------------------
// Free cached results and optimize the LP problem in task 

void OsiMskSolverInterface::initialSolve()
{
  #if MSK_OSI_DEBUG_LEVEL > 1
  debugMessage("Begin OsiMskSolverInterface::initialSolve() %p\n", (void*)this);
  #endif

  if( definedSolution( MSK_SOL_BAS ) == true )
  {
    resolve();
  }
  else
  {
  	
    int err,solver;

    err = MSK_putintparam(getMutableLpPtr(), MSK_IPAR_SIM_HOTSTART, MSK_SIM_HOTSTART_STATUS_KEYS);
    checkMSKerror(err,"MSK_putintparam","initialsolve");

    err = MSK_getintparam(getMutableLpPtr(), MSK_IPAR_OPTIMIZER, &solver);
    checkMSKerror(err,"MSK_getintparam","initialsolve");

    switchToLP();

    err = MSK_putintparam(getMutableLpPtr(), MSK_IPAR_OPTIMIZER, INITIAL_SOLVE);
    checkMSKerror(err,"MSK_putintparam","initialSolve");

    err = MSK_getintparam(getMutableLpPtr(), MSK_IPAR_OPTIMIZER, &MSKsolverused_);
    checkMSKerror(err,"MSK_getintparam","initialsolve");

    #if MSK_DO_MOSEK_LOG > 0
    err = MSK_putintparam( getMutableLpPtr(),MSK_IPAR_LOG_SIM,MSK_DO_MOSEK_LOG);
    checkMSKerror(err,"MSK_putintparam","initialsolve");
    #endif

    Mskerr = MSK_optimize(getLpPtr( OsiMskSolverInterface::FREECACHED_RESULTS ));

    #if MSK_DO_MOSEK_LOG > 0
    err = MSK_solutionsummary( getMutableLpPtr(),MSK_STREAM_LOG);
    checkMSKerror(err,"MSK_solutionsummary","initialsolve");
    #endif

    err = MSK_putintparam(getMutableLpPtr(), MSK_IPAR_OPTIMIZER, solver);
    checkMSKerror(err,"MSK_putintparam","initialsolve");
    
  }

  #if MSK_OSI_DEBUG_LEVEL > 1
  debugMessage("End OsiMskSolverInterface::initialSolve()\n");
  #endif
}

//-----------------------------------------------------------------------------
// Resolves an LP problem. 

void OsiMskSolverInterface::resolve()
{
  #if MSK_OSI_DEBUG_LEVEL > 1
  debugMessage("Begin OsiMskSolverInterface::resolve %p\n", (void*)this);
  #endif

  if( definedSolution( MSK_SOL_BAS ) == true )
  {
    int err,solver;

    err = MSK_getintparam(getMutableLpPtr(), MSK_IPAR_OPTIMIZER, &solver);
    checkMSKerror(err,"MSK_getintparam","resolve");

    switchToLP();

    err = MSK_putintparam(getMutableLpPtr(), MSK_IPAR_SIM_HOTSTART, MSK_SIM_HOTSTART_STATUS_KEYS);
    checkMSKerror(err,"MSK_putintparam","resolve");

    err = MSK_putintparam(getMutableLpPtr(), MSK_IPAR_OPTIMIZER, RESOLVE_SOLVE);
    checkMSKerror(err,"MSK_putintparam","resolve");

    #if 0
    err = MSK_putintparam( getMutableLpPtr(), MSK_IPAR_SIM_MAX_ITERATIONS, 20000000 );
    #endif

    err = MSK_getintparam(getMutableLpPtr(), MSK_IPAR_OPTIMIZER, &MSKsolverused_);
    checkMSKerror(err,"MSK_getintparam","resolve");

    #if MSK_DO_MOSEK_LOG > 0
    err = MSK_putintparam( getMutableLpPtr(),MSK_IPAR_LOG,MSK_DO_MOSEK_LOG);
    checkMSKerror(err,"MSK_putintparam","resolve");

    err = MSK_putintparam( getMutableLpPtr(),MSK_IPAR_LOG_SIM,MSK_DO_MOSEK_LOG);
    checkMSKerror(err,"MSK_putintparam","resolve");
    #endif

    Mskerr = MSK_optimize(getLpPtr( OsiMskSolverInterface::FREECACHED_RESULTS ));

    #if MSK_DO_MOSEK_LOG > 0
    err = MSK_solutionsummary( getMutableLpPtr(),MSK_STREAM_LOG);
    printf("Mskerr : %d\n",Mskerr);
    #endif

    err = MSK_putintparam(getMutableLpPtr(), MSK_IPAR_OPTIMIZER, solver);
    checkMSKerror(err,"MSK_putintparam","resolve");
  }
  else
  {
    initialSolve();
  }

  #if MSK_OSI_DEBUG_LEVEL > 1
  debugMessage("End OsiMskSolverInterface::resolve... press any key to continue\n");
  getchar();
  #endif
}

//-----------------------------------------------------------------------------
// Resolves an MIP problem with MOSEK MIP solver.

void OsiMskSolverInterface::branchAndBound()
{
  #if MSK_OSI_DEBUG_LEVEL > 1
  debugMessage("Begin OsiMskSolverInterface::branchAndBound()\n");
  #endif

  switchToMIP();

  int err = MSK_getintparam(getMutableLpPtr(), MSK_IPAR_OPTIMIZER, &MSKsolverused_);
  checkMSKerror(err,"MSK_getintparam","branchAndBound");

  Mskerr = MSK_optimize(getLpPtr( OsiMskSolverInterface::FREECACHED_RESULTS ));

  #if MSK_OSI_DEBUG_LEVEL > 1
  debugMessage("End OsiMskSolverInterface::branchAndBound()\n");
  #endif
}

//#############################################################################
// Parameter related methods
//#############################################################################

//-----------------------------------------------------------------------------
// Sets a int parameter in MOSEK

bool
OsiMskSolverInterface::setIntParam(OsiIntParam key, int value)
{
  #if MSK_OSI_DEBUG_LEVEL > 3  
  debugMessage("Begin OsiMskSolverInterface::setIntParam(%d, %d)\n", key, value);
  #endif

  bool retval = false;

  switch (key)
  {
    case OsiMaxNumIteration:
      retval = (MSK_putintparam(getMutableLpPtr(),
                                MSK_IPAR_SIM_MAX_ITERATIONS,
                                value ) == MSK_RES_OK);
      break;
      
    case OsiMaxNumIterationHotStart:
      if (value < 0) {
        retval = false;
      } else {
        hotStartMaxIteration_ = value;
        retval = true;
      }
      break;
    case OsiNameDiscipline:
      retval = false;
      break;
    case OsiLastIntParam:
      retval = false;
      break;
  }  

  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("End OsiMskSolverInterface::setIntParam(%d, %d)\n", key, value);
  #endif

  return retval;
}

//-----------------------------------------------------------------------------
// Sets a double parameter in MOSEK.

bool
OsiMskSolverInterface::setDblParam(OsiDblParam key, double value)
{
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::setDblParam(%d, %g)\n", key, value);
  #endif
  
  bool retval = false;
  switch (key) 
  {
    case OsiDualObjectiveLimit:
      if( getObjSense() == +1 ) // Minimize
            retval = ( MSK_putdouparam( getMutableLpPtr(),
                                        MSK_DPAR_UPPER_OBJ_CUT,
                                        value  ) == MSK_RES_OK ); // min
      else
          retval = ( MSK_putdouparam( getMutableLpPtr(),
                                      MSK_DPAR_LOWER_OBJ_CUT,
                                      value ) == MSK_RES_OK ); // max
      break;
    case OsiPrimalObjectiveLimit:
      if( getObjSense() == +1 ) // Minimize
          retval = ( MSK_putdouparam( getMutableLpPtr(),
                                      MSK_DPAR_LOWER_OBJ_CUT,
                                      value  ) == MSK_RES_OK ); // min
      else
          retval = ( MSK_putdouparam( getMutableLpPtr(),
                                      MSK_DPAR_UPPER_OBJ_CUT,
                                      value ) == MSK_RES_OK ); // max
      break;
    case OsiDualTolerance:
      retval = ( MSK_putdouparam( getMutableLpPtr(), 
                                  MSK_DPAR_BASIS_TOL_S, 
                                  value ) == MSK_RES_OK );
      break;
    case OsiPrimalTolerance:
      retval = ( MSK_putdouparam( getMutableLpPtr(), 
                                  MSK_DPAR_BASIS_TOL_X, 
                                  value ) == MSK_RES_OK );
      break;       
    case OsiObjOffset:
      ObjOffset_ = value;
      retval     = true;
      break;
    case OsiLastDblParam:
      retval = false;
      break;
   }

  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("End OsiMskSolverInterface::setDblParam(%d, %g)\n", key, value);
  #endif

  return retval;
}


//-----------------------------------------------------------------------------
// Sets a string parameter in MOSEK.

bool
OsiMskSolverInterface::setStrParam(OsiStrParam key, const std::string & value)
{
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::setStrParam(%d, %s)\n", key, value.c_str());
  #endif

  bool retval=false;
  switch (key) 
  {
    case OsiProbName:
      OsiSolverInterface::setStrParam(key,value);
      return retval = true;
    case OsiSolverName:
      return false;
    case OsiLastStrParam:
      return false;
  }

  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("End OsiMskSolverInterface::setStrParam(%d, %s)\n", key, value.c_str());
  #endif

  return retval;
}

//-----------------------------------------------------------------------------
// Gets a int parameter in MOSEK. 

bool
OsiMskSolverInterface::getIntParam(OsiIntParam key, int& value) const
{
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::getIntParam(%d)\n", key);
  #endif

  bool retval = false;
  switch (key)
  {
    case OsiMaxNumIteration:
      retval = (MSK_getintparam(getMutableLpPtr(), 
                                MSK_IPAR_SIM_MAX_ITERATIONS, 
                                &value ) == MSK_RES_OK);
      break;
    case OsiMaxNumIterationHotStart:
      value = hotStartMaxIteration_;
      retval = true;
      break;
    case OsiNameDiscipline:
      value = 0;
      retval = false;
      break;
    case OsiLastIntParam:
      retval = false;
      break;
  }

  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("End OsiMskSolverInterface::getIntParam(%d)\n", key);
  #endif

  return retval;
}

//-----------------------------------------------------------------------------
// Get a double parameter in MOSEK.

bool
OsiMskSolverInterface::getDblParam(OsiDblParam key, double& value) const
{
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::getDblParam(%d)\n", key);
  #endif
  
  bool retval = false;

  switch (key) 
    {
    case OsiDualObjectiveLimit:
      if( getObjSense() == +1 )
          retval = ( MSK_getdouparam( getMutableLpPtr(), 
                                      MSK_DPAR_UPPER_OBJ_CUT, 
                                      &value  ) == MSK_RES_OK ); // max
      else
          retval = ( MSK_getdouparam( getMutableLpPtr(), 
                                      MSK_DPAR_LOWER_OBJ_CUT, 
                                      &value ) == MSK_RES_OK ); // min
      break;
    case OsiPrimalObjectiveLimit:
      if( getObjSense() == +1 )
          retval = ( MSK_getdouparam( getMutableLpPtr(), 
                                      MSK_DPAR_LOWER_OBJ_CUT, 
                                      &value ) == MSK_RES_OK ); // min
      else
          retval = ( MSK_getdouparam( getMutableLpPtr(), 
                                      MSK_DPAR_UPPER_OBJ_CUT, 
                                      &value ) == MSK_RES_OK ); // max
      break;
    case OsiDualTolerance:
      retval = ( MSK_getdouparam( getMutableLpPtr(), 
                                  MSK_DPAR_BASIS_TOL_S, 
                                  &value ) == MSK_RES_OK );
      break;
    case OsiPrimalTolerance:
      retval = ( MSK_getdouparam( getMutableLpPtr(), 
                                  MSK_DPAR_BASIS_TOL_X, 
                                  &value ) == MSK_RES_OK );
      break;       
    case OsiObjOffset:
      value = ObjOffset_;
      retval = true;
      break;
    case OsiLastDblParam:
      retval = false;
      break;
    }

  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("End OsiMskSolverInterface::getDblParam(%d)\n", key);
  #endif

  return retval;
}

//-----------------------------------------------------------------------------
// Gets a string parameter from MOSEK

bool
OsiMskSolverInterface::getStrParam(OsiStrParam key, std::string & value) const
{
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::getStrParam(%d)\n", key);
  #endif

  switch (key) 
  {
    case OsiProbName:
      OsiSolverInterface::getStrParam(key, value);
      break;
    case OsiSolverName:
      value = "MOSEK";
      break;
    case OsiLastStrParam:
      return false;
  }

  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("End OsiMskSolverInterface::getStrParam(%d)\n", key);
  #endif

  return true;
}

//#############################################################################
// Methods returning info on how the solution process terminated
//#############################################################################

//-----------------------------------------------------------------------------
// Returns true if solver abandoned in last call to solver.
// Mosek does not use this functionality

bool OsiMskSolverInterface::isAbandoned() const
{
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::isAbandoned()\n");
  debugMessage("isAbandoned() %d\n",(Mskerr != MSK_RES_OK));
  debugMessage("End OsiMskSolverInterface::isAbandoned()\n");
  #endif
  
  return (Mskerr != MSK_RES_OK);
}

//-----------------------------------------------------------------------------
// Returns true if "solution" available is proved to be optimal, where "solution" in LP
// could be both interior and basic, checks for both. 

bool OsiMskSolverInterface::isProvenOptimal() const
{
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::isProvenOptimal()\n");
  #endif
  
  int err;
  MSKsolstae status;
  MSKsoltypee solution;

  if( probtypemip_ == false)
  {
    if( definedSolution( MSK_SOL_BAS ) == true )
      solution = MSK_SOL_BAS;
    else
    {
      if( definedSolution( MSK_SOL_ITR ) == true )
        solution = MSK_SOL_ITR;
      else
        return false;
    }
  }
  else
  {
     if( definedSolution( MSK_SOL_ITG ) == true )
      solution = MSK_SOL_ITG;
    else
      return false;     
  }

  err = MSK_getsolution(getMutableLpPtr(),
                        solution,
                        NULL, 
                        &status, 
                        NULL, 
                        NULL, 
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL);

  checkMSKerror(err,"MSK_getsolution","isProvenOptimal");
  
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Solution type , %d  status %d \n ",(status == MSK_SOL_STA_OPTIMAL), status) ;
  debugMessage("End OsiMskSolverInterface::isProvenOptimal()\n");
  #endif

  return ( status == MSK_SOL_STA_OPTIMAL || status == MSK_SOL_STA_INTEGER_OPTIMAL);
}

//-----------------------------------------------------------------------------
// Returns true if a certificate of primal inf. exits

bool OsiMskSolverInterface::isProvenPrimalInfeasible() const
{
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::isProvenPrimalInfeasible()\n");
  #endif
  
  int err;
  MSKsolstae status;
  MSKsoltypee solution;

  if( probtypemip_ == false)
  {
    if( definedSolution( MSK_SOL_BAS ) == true )
      solution = MSK_SOL_BAS;
    else
    {
      if( definedSolution( MSK_SOL_ITR ) == true )
        solution = MSK_SOL_ITR;
      else
        return false;
    }
  }
  else
  {
    if(  definedSolution( MSK_SOL_ITG ) == true)
      solution = MSK_SOL_ITG;
    else
      return false;     
  }

  err = MSK_getsolution(getMutableLpPtr(),
                        solution,
                        NULL, 
                        &status, 
                        NULL, 
                        NULL, 
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL);

  checkMSKerror(err,"MSK_getsolution","isProvenPrimalInfeasible");
  
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("isProvenPrimalInfeasible %d \n",(status == MSK_SOL_STA_PRIM_INFEAS_CER));
  debugMessage("End OsiMskSolverInterface::isProvenPrimalInfeasible()\n");
  #endif

  return ( status == MSK_SOL_STA_PRIM_INFEAS_CER ); 
}

//-----------------------------------------------------------------------------
// Should return true if a certificate of dual inf. exits
// But COIN does not support this feature thus we return false

bool OsiMskSolverInterface::isProvenDualInfeasible() const
{
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::isProvenDualInfeasible()\n");
  #endif

  int err;
  MSKsolstae status;
  MSKsoltypee solution;

  if( probtypemip_ == false )
  {
    if( definedSolution( MSK_SOL_BAS ) == true )
      solution = MSK_SOL_BAS;
    else
    {
      if( definedSolution( MSK_SOL_ITR ) == true )
        solution = MSK_SOL_ITR;
      else
        return false;
    }
  }
  else
  {
    if(  definedSolution( MSK_SOL_ITG ) == true)
      solution = MSK_SOL_ITG;
    else
      return false;     
  }

  err = MSK_getsolution(getMutableLpPtr(),
                        solution,
                        NULL, 
                        &status, 
                        NULL, 
                        NULL, 
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL
                        );

  checkMSKerror(err,"MSK_getsolution","isProvenDualInfeasible");
  
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("isProvenDualInfeasible %d \n",(status == MSK_SOL_STA_DUAL_INFEAS_CER));
  debugMessage("End OsiMskSolverInterface::isProvenDualInfeasible()\n");
  #endif
  
  return ( status == MSK_SOL_STA_DUAL_INFEAS_CER );
}

//-----------------------------------------------------------------------------
// Returns true if primal objective limit is reached. Checks the objective sense
// first. 

bool OsiMskSolverInterface::isPrimalObjectiveLimitReached() const
{
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::isPrimalObjectiveLimitReached()\n");
  #endif

  if( Mskerr == MSK_RES_TRM_OBJECTIVE_RANGE )
  {
    int err;
    double obj = getObjValue(),value=MSK_INFINITY;
    if( getObjSense() == +1 )
    {
      err = MSK_getdouparam( getMutableLpPtr(), 
                             MSK_DPAR_LOWER_OBJ_CUT, 
                             &value);

      checkMSKerror(err,"MSK_getdouparam","isPrimalObjectiveLimitReached");
    }
    else
    {
      err = MSK_getdouparam(getMutableLpPtr(), 
                            MSK_DPAR_UPPER_OBJ_CUT, 
                            &value );

      checkMSKerror(err,"MSK_getdouparam","isPrimalObjectiveLimitReached");

      obj = -obj;
      value = -value;
    }

    #if MSK_OSI_DEBUG_LEVEL > 3
    debugMessage("primal objective value  %-16.10e , lowerbound %-16.10e , reached %d \n",obj,value,(value <= obj));
    debugMessage("End OsiMskSolverInterface::isPrimalObjectiveLimitReached()\n");
    #endif

    return ( !( value == MSK_INFINITY || value == -MSK_INFINITY ) && value > obj );
  }
  else
  {
    if( Mskerr == MSK_RES_OK )
    {
      if( definedSolution( MSK_SOL_BAS ) == true  )
      {
        int err;
        double obj = getObjValue(),value=MSK_INFINITY;

        if( getObjSense() == +1 )
        {
          err = MSK_getdouparam( getMutableLpPtr(),
                                 MSK_DPAR_LOWER_OBJ_CUT,
                                 &value);

          checkMSKerror(err,"MSK_getdouparam","isPrimalObjectiveLimitReached");

          return (obj < value);
        }
        else
        {
          err = MSK_getdouparam(getMutableLpPtr(),
                                MSK_DPAR_UPPER_OBJ_CUT,
                                &value );

          checkMSKerror(err,"MSK_getdouparam","isPrimalObjectiveLimitReached");

          obj = -obj;
          value = -value;

          return (obj < value);
        }
      }
    }

    #if MSK_OSI_DEBUG_LEVEL > 3
    debugMessage("End OsiMskSolverInterface::isPrimalObjectiveLimitReached()\n");
    #endif

    return false;
  }
}

//-----------------------------------------------------------------------------
// Returns true if dual objective limit is reached. Checks the objective sense
// first.  

bool OsiMskSolverInterface::isDualObjectiveLimitReached() const
{
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::isDualObjectiveLimitReached()\n");
  #endif

  if( Mskerr == MSK_RES_TRM_OBJECTIVE_RANGE )
  {
    int err;
    double obj = getObjValue(),value=MSK_INFINITY;
    if( getObjSense() == +1 )
    {
      err = MSK_getdouparam( getMutableLpPtr(),
                             MSK_DPAR_UPPER_OBJ_CUT,
                             &value);

      checkMSKerror(err,"MSK_getdouparam","isDualObjectiveLimitReached");
    }
    else
    {
      err = MSK_getdouparam( getMutableLpPtr(), 
                             MSK_DPAR_LOWER_OBJ_CUT, 
                             &value );

      checkMSKerror(err,"MSK_getdouparam","isDualObjectiveLimitReached");

      obj = -obj;
      value = -value;
    }

    checkMSKerror( err, "MSK_getdouparam", "isPrimalObjectiveLimitReached" );

    #if MSK_OSI_DEBUG_LEVEL > 3
    debugMessage("dual objective value  %f , lowerbound %f , reached %i \n",obj,value,(value <= obj));
    debugMessage("End OsiMskSolverInterface::isDualObjectiveLimitReached()\n");
    #endif

    return ( !( value == MSK_INFINITY || value == -MSK_INFINITY ) && value <= obj );
  }
  else
  {
    if( Mskerr == MSK_RES_OK )
    {
      if( definedSolution( MSK_SOL_BAS ) == true  )
      {
        int err;
        double obj = getObjValue(),value=MSK_INFINITY;

        if( getObjSense() == +1 )
        {
          err = MSK_getdouparam( getMutableLpPtr(),
                                 MSK_DPAR_UPPER_OBJ_CUT,
                                 &value);

          checkMSKerror(err,"MSK_getdouparam","isDualObjectiveLimitReached");

          return (obj > value);
        }
        else
        {
          err = MSK_getdouparam(getMutableLpPtr(),
                                MSK_DPAR_LOWER_OBJ_CUT,
                                &value );

          checkMSKerror(err,"MSK_getdouparam","isDualObjectiveLimitReached");

          obj = -obj;
          value = -value;

          return (obj > value);
        }
      }
    }

    #if MSK_OSI_DEBUG_LEVEL > 3
    debugMessage("End OsiMskSolverInterface::isDualObjectiveLimitReached()\n");
    #endif

    return false;
  }
}

//-----------------------------------------------------------------------------
// Returns true if iteration number used in last call to optimize eq. max number for
// the solver used. 

bool OsiMskSolverInterface::isIterationLimitReached() const
{
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::isIterationLimitReached()\n");
  #endif
  
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("iteration limit reached %d \n",Mskerr);
  debugMessage("End OsiMskSolverInterface::isIterationLimitReached()\n");
  #endif

  return Mskerr == MSK_RES_TRM_MAX_ITERATIONS;
}

//-----------------------------------------------------------------------------
// Returns true if a license problem occured in last call to optimize.

bool OsiMskSolverInterface::isLicenseError() const
{
   #if MSK_OSI_DEBUG_LEVEL > 3
   debugMessage("Begin OsiMskSolverInterface::isLicenseError()\n");
   #endif

   #if MSK_OSI_DEBUG_LEVEL > 3
   debugMessage("license error %d \n",Mskerr);
   debugMessage("End OsiMskSolverInterface::isLicenseError()\n");
   #endif

   return Mskerr >= MSK_RES_ERR_LICENSE && Mskerr <= MSK_RES_ERR_LICENSE_NO_SERVER_SUPPORT;
}


//#############################################################################
// WarmStart related methods
//#############################################################################

// Get an empty warm start object
CoinWarmStart* OsiMskSolverInterface::getEmptyWarmStart () const
{ return (dynamic_cast<CoinWarmStart *>(new CoinWarmStartBasis())) ; }

//-----------------------------------------------------------------------------
// Get warm start, returns NULL pointer if not available

CoinWarmStart* OsiMskSolverInterface::getWarmStart() const
{

  CoinWarmStartBasis* ws = NULL;
  int numbas = 0,numcols = getNumCols(),numrows = getNumRows(),*skx,*skc,
      err, i,*bkc,*bkx;
  double *blc,*buc,*blx,*bux,*xc,*xx;
  
  bool skip = false;

  #if MSK_OSI_DEBUG_LEVEL > 1
  debugMessage("Begin OsiMskSolverInterface::getWarmStart numcols %d numrows %d %p\n",numcols,numrows, (void*)this);
  #endif

  skx = new int[numcols];
  skc = new int[numrows];

  bkc = new int[numrows];
  blc = new double[numrows];
  buc = new double[numrows];
  xc  = new double[numrows];

  bkx = new int[numcols];
  blx = new double[numcols];
  bux = new double[numcols];
  xx  = new double[numcols];

  err = MSK_getboundslice(getMutableLpPtr(),
                          MSK_ACC_CON,
                          0,
                          numrows,
                          (MSKboundkeye*)bkc,
                          (MSKrealt*)blc,
                          (MSKrealt*)buc);
    
  checkMSKerror( err, "MSK_getsolution", "getWarmStart" );

  err = MSK_getboundslice(getMutableLpPtr(),
                          MSK_ACC_VAR,
                          0,
                          numcols,
                          (MSKboundkeye*)bkx,
                          (MSKrealt*)blx,
                          (MSKrealt*)bux);  
 
  checkMSKerror( err, "MSK_getsolution", "getWarmStart" );
  
  MSKassert(3,!probtypemip_,"!probtypemip_","getWarmStart");

  if( definedSolution( MSK_SOL_BAS ) == true )
  {
    err  =   MSK_getsolution(getMutableLpPtr(),
                             MSK_SOL_BAS,
                             NULL,
                             NULL,
                             (MSKstakeye*)skc, 
                             (MSKstakeye*)skx,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL);

      checkMSKerror( err, "MSK_getsolution", "getWarmStart" );
  }
  else
  {
    #if MSK_OSI_DEBUG_LEVEL > 1
    debugMessage("No basic solution in OsiMskSolverInterface::getWarmStart()\n");
    #endif

    /* No basic solution stored choose slack basis */
    /* Otherwise the unittest can not be passed    */
    ws = new CoinWarmStartBasis;
    ws->setSize( numcols, numrows );
        
    for( i = 0; i < numrows; ++i )
      ws->setArtifStatus( i, CoinWarmStartBasis::basic );

    numbas = numrows;
                        
    for( i = 0; i < numcols; ++i )
    {
      switch(bkx[i])
      {
        case MSK_BK_RA:
        case MSK_BK_LO:
           ws->setStructStatus( i, CoinWarmStartBasis::atLowerBound );
        break;
        case MSK_BK_FX:
        case MSK_BK_UP:
           ws->setStructStatus( i, CoinWarmStartBasis::atUpperBound );
        break;
        case MSK_BK_FR:
           ws->setStructStatus( i, CoinWarmStartBasis::isFree );
        break;
        default:
          checkMSKerror( 1, "Wrong bound key", "getWarmStart" );
      }
    }
    
    #if MSK_OSI_DEBUG_LEVEL > 1
    debugMessage("End OsiMskSolverInterface::getWarmStart()\n");
    #endif

    delete[] skc;
    delete[] skx;

    delete[] bkc;
    delete[] blc;
    delete[] buc;
    delete[] xc;
    
    delete[] bkx;
    delete[] blx;
    delete[] bux;
    delete[] xx;

    /* Leave function */
    return ws;
  }
  
  if( err == MSK_RES_OK )
  {
    /* Status keys should be defined  */
    ws = new CoinWarmStartBasis;

    MSKassert(3,ws != NULL,"1) ws != NULL","getWarmStart");

    ws->setSize( numcols, numrows );
    
    for( i = 0; i < numrows && ws != NULL; ++i )
    {
      switch( skc[i] )
      {
        case MSK_SK_UNK:
        case MSK_SK_BAS:
          /* Warning : Slacks in Osi and Mosek er negated */
          ws->setArtifStatus( i, CoinWarmStartBasis::basic );
          ++numbas;
          break;
        case MSK_SK_LOW:
          /* Warning : Slacks in Osi and Mosek er negated */
          ws->setArtifStatus( i, CoinWarmStartBasis::atUpperBound );
          break;
        case MSK_SK_FIX:
        case MSK_SK_UPR:
          /* Warning : Slacks in Osi and Mosek er negated */
          ws->setArtifStatus( i, CoinWarmStartBasis::atLowerBound );
          break;
        case MSK_SK_SUPBAS:
          ws->setArtifStatus( i, CoinWarmStartBasis::isFree );
          break;
        default:  // unknown row status
          delete ws;
          ws   = NULL;
          skip = true;
          checkMSKerror( 1, "Wrong slack status key", "getWarmStart" );
          break;
      }
    }

    if( skip == false )
    {
      for( i = 0; i < numcols && ws != NULL; ++i )
      {
        switch( skx[i] )
        {
          case MSK_SK_UNK:
          case MSK_SK_BAS:
            ++numbas;
            ws->setStructStatus( i, CoinWarmStartBasis::basic );
            break;
          case MSK_SK_FIX:
          case MSK_SK_LOW:
            ws->setStructStatus( i, CoinWarmStartBasis::atLowerBound );
            break;
          case MSK_SK_UPR:
            ws->setStructStatus( i, CoinWarmStartBasis::atUpperBound );
            break;
          case MSK_SK_SUPBAS:
            ws->setStructStatus( i, CoinWarmStartBasis::isFree );
            break;
          default:  // unknown column status
            delete ws;
            ws = NULL;
            checkMSKerror( 1, "Wrong variable status key", "getWarmStart" );
            break;
        }
      }
    }
  }

  delete[] skx;
  delete[] skc;

  delete[] bkc;
  delete[] blc;
  delete[] buc;
  delete[] xc;

  delete[] bkx;
  delete[] blx;
  delete[] bux;
  delete[] xx;

  MSKassert(3,ws!=NULL,"2) ws!=NULL","getWarmStart");

  #if MSK_OSI_DEBUG_LEVEL > 1
  debugMessage("End OsiMskSolverInterface::getWarmStart() (%p) numcols %d numrows %d numbas %d\n",(void*)(ws),numcols,numrows,numbas);
  #endif

  return ws;
}

//-----------------------------------------------------------------------------
// Set warm start

bool OsiMskSolverInterface::setWarmStart(const CoinWarmStart* warmstart)
{

  const CoinWarmStartBasis* ws = dynamic_cast<const CoinWarmStartBasis*>(warmstart);
  int numcols, numrows, i, restat,numbas=0;
  int *skx, *skc, *bkc, *bkx;
  bool retval = false, skip = false;

  if( !ws )
    return false;

  numcols = ws->getNumStructural();
  numrows = ws->getNumArtificial();

  #if MSK_OSI_DEBUG_LEVEL > 1
  debugMessage("Begin OsiMskSolverInterface::setWarmStart(%p) this = %p numcols %d numrows %d\n", (void *)warmstart,(void*)this,numcols,numrows);
  #endif
  
  if( numcols != getNumCols() || numrows != getNumRows() )
    return false;

  switchToLP();

  skx  = new int[numcols];
  skc  = new int[numrows];
  bkc  = new int[numrows];

  restat = MSK_getboundslice(getMutableLpPtr(),
                             MSK_ACC_CON,
                             0,
                             numrows,
                             (MSKboundkeye*) (bkc), 
                             NULL,
                             NULL);

  checkMSKerror( restat, "MSK_getboundslice", "setWarmStart" );

  for( i = 0; i < numrows; ++i )
  {
    switch( ws->getArtifStatus( i ) )
    {
      case CoinWarmStartBasis::basic:
        skc[i] = MSK_SK_BAS;
        ++numbas;
      break;
      /* Warning : Slacks in Osi and Mosek er negated */
      case CoinWarmStartBasis::atUpperBound:
        switch(bkc[i])
        {
          case MSK_BK_LO:
          case MSK_BK_RA:
          skc[i] = MSK_SK_LOW;
          break;
          case MSK_BK_FX:
          skc[i] = MSK_SK_FIX;
          break;
          default:
          skc[i] = MSK_SK_UNK;
          break;
        }
      break;
      /* Warning : Slacks in Osi and Mosek er negated */
      case CoinWarmStartBasis::atLowerBound:
        switch(bkc[i])
        {
          case MSK_BK_UP:
          case MSK_BK_RA:
          skc[i] = MSK_SK_UPR;
          break;
          case MSK_BK_FX:
          skc[i] = MSK_SK_FIX;
          break;
          default:
          skc[i] = MSK_SK_UNK;
          break;
        }
        break;
      case CoinWarmStartBasis::isFree:
       skc[i] = MSK_SK_SUPBAS;
       break;
      default:  // unknown row status
       retval = false;
       skip   = true;
       MSKassert(3,1,"Unkown rowstatus","setWarmStart");
       break;
    }
  }

  delete[] bkc;
  
  if( skip == false )
  {
    bkx = new int[numcols];

    restat = MSK_getboundslice(getMutableLpPtr(),
                               MSK_ACC_VAR,
                               0,
                               numcols,
                               (MSKboundkeye*) (bkx), 
                               NULL,
                               NULL);

   checkMSKerror( restat, "MSK_getboundslice", "setWarmStart" );

   for( i = 0; i < numcols; ++i )
    {
      switch( ws->getStructStatus( i ) )
      {
        case CoinWarmStartBasis::basic:
         skx[i] = MSK_SK_BAS;
         ++numbas;
         break;
        case CoinWarmStartBasis::atLowerBound:
          switch(bkx[i])
          {
            case MSK_BK_LO:
            case MSK_BK_RA:
              skx[i] = MSK_SK_LOW;
            break;
            case MSK_BK_FX:
              skx[i] = MSK_SK_FIX;
            default:
            skx[i] = MSK_SK_UNK;
            break;
          }        
         break;
       case CoinWarmStartBasis::atUpperBound:
          switch(bkx[i])
          {
            case MSK_BK_UP:
            case MSK_BK_RA:
              skx[i] = MSK_SK_UPR;
            break;
            case MSK_BK_FX:
              skx[i] = MSK_SK_FIX;
            default:
            skx[i] = MSK_SK_UNK;
            break;
          }        
         break;
       case CoinWarmStartBasis::isFree:
         skx[i] = MSK_SK_SUPBAS;
         break;
       default:  // unknown col status
         retval   = false;
         skip     = true;
         MSKassert(3,1,"Unkown col status","setWarmStart");
       break;
      }
    }
    
    delete[] bkx;
  }

  #if MSK_OSI_DEBUG_LEVEL > 1
  debugMessage("OsiMskSolverInterface::setWarmStart(%p) numcols %d numrows %d numbas %d\n", (void *)warmstart,numcols,numrows,numbas);
  #endif

  #if 0
  MSKassert(3,numbas == numrows,"Wrong number of basis variables","setWarmStart");
  #endif

  if( skip == false )
  {
    restat = MSK_putsolution( getLpPtr( OsiMskSolverInterface::FREECACHED_RESULTS ),
                              MSK_SOL_BAS,
                              (MSKstakeye*) (skc), 
                              (MSKstakeye*) (skx), 
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              NULL);
    
    delete[] skx;
    delete[] skc;
  }
  else
  {
    #if MSK_OSI_DEBUG_LEVEL > 1
    debugMessage("Skipping setting values in OsiMskSolverInterface::setWarmStart()\n");
    #endif

    #if MSK_OSI_DEBUG_LEVEL > 1
    debugMessage("End OsiMskSolverInterface::setWarmStart(%p)\n", (void *)warmstart);
    #endif

    delete[] skx;
    delete[] skc;

    return false;
  }
  
  retval = (restat == MSK_RES_OK);
 
  #if MSK_OSI_DEBUG_LEVEL > 1
  debugMessage("End OsiMskSolverInterface::setWarmStart(%p)\n", (void *)warmstart);
  #endif

  return retval;
}

//#############################################################################
// Hotstart related methods (primarily used in strong branching)
//#############################################################################

//-----------------------------------------------------------------------------
// Mark hot start

void OsiMskSolverInterface::markHotStart()
{
  #if MSK_OSI_DEBUG_LEVEL > 1
  debugMessage("Begin OsiMskSolverInterface::markHotStart()\n");
  #endif

  int err;
  int numcols, numrows;

  MSKassert(3,!probtypemip_,"probtypemip_","markHotStart");

  numcols = getNumCols();
  numrows = getNumRows();
  
  if( numcols > hotStartCStatSize_ )
  {
    delete[] hotStartCStat_;
    hotStartCStatSize_ = static_cast<int>( 1.0 * static_cast<double>( numcols ) ); 
    // get some extra space for future hot starts
    hotStartCStat_ = new int[hotStartCStatSize_];
  }
    
  if( numrows > hotStartRStatSize_ )
  {
    delete[] hotStartRStat_;
    hotStartRStatSize_ = static_cast<int>( 1.0 * static_cast<double>( numrows ) ); 
    // get some extra space for future hot starts
    hotStartRStat_ = new int[hotStartRStatSize_];
  }

  err = MSK_getsolution(getMutableLpPtr(),
                        MSK_SOL_BAS,
                        NULL,
                        NULL,
                        (MSKstakeye*) (hotStartRStat_), 
                        (MSKstakeye*) (hotStartCStat_), 
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL);
  
  checkMSKerror( err, "MSK_getsolution", "markHotStart" );

  #if MSK_OSI_DEBUG_LEVEL > 1
  debugMessage("End OsiMskSolverInterface::markHotStart()\n");
  #endif
}

//-----------------------------------------------------------------------------
// Solve from a hot start

void OsiMskSolverInterface::solveFromHotStart()
{
  #if MSK_OSI_DEBUG_LEVEL > 1
  debugMessage("Begin OsiMskSolverInterface::solveFromHotStart()\n");
  #endif

  int err;
  
  int maxiter;

  
  switchToLP();

  MSKassert(3,getNumCols() <= hotStartCStatSize_,"getNumCols() <= hotStartCStatSize_","solveFromHotStart");
  MSKassert(3,getNumRows() <= hotStartRStatSize_,"getNumRows() <= hotStartRStatSize_","solveFromHotStart");

  err = MSK_putsolution(getLpPtr( OsiMskSolverInterface::FREECACHED_RESULTS ),
                        MSK_SOL_BAS,
                        (MSKstakeye*) (hotStartRStat_), 
                        (MSKstakeye*) (hotStartCStat_), 
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL);
  
  
  checkMSKerror( err, "MSK_putsolution", "solveFromHotStart" );
    
  err = MSK_getintparam( getMutableLpPtr(), MSK_IPAR_SIM_MAX_ITERATIONS, &maxiter );
  checkMSKerror( err, "MSK_getintparam", "solveFromHotStart" );

  err = MSK_putintparam( getMutableLpPtr(), MSK_IPAR_SIM_MAX_ITERATIONS, hotStartMaxIteration_ );
  checkMSKerror( err, "MSK_putintparam", "solveFromHotStart" );
  
  resolve();

  err = MSK_putintparam( getMutableLpPtr(), MSK_IPAR_SIM_MAX_ITERATIONS, maxiter );

  checkMSKerror( err, "MSK_putintparam", "solveFromHotStart" );

  #if MSK_OSI_DEBUG_LEVEL > 1
  debugMessage("End OsiMskSolverInterface::solveFromHotStart()\n");
  #endif
}

//-----------------------------------------------------------------------------
// Unmark a hot start

void OsiMskSolverInterface::unmarkHotStart()
{
  #if MSK_OSI_DEBUG_LEVEL > 1
  debugMessage("Begin OsiMskSolverInterface::unmarkHotStart()\n");
  #endif

  freeCachedData();

  if( hotStartCStat_ != NULL )
    delete[] hotStartCStat_;

  if( hotStartRStat_ != NULL )
    delete[] hotStartRStat_;

  hotStartCStat_     = NULL;
  hotStartCStatSize_ = 0;
  hotStartRStat_     = NULL;
  hotStartRStatSize_ = 0;

  #if MSK_OSI_DEBUG_LEVEL > 1
  debugMessage("End OsiMskSolverInterface::unmarkHotStart()\n");
  #endif
}

//#############################################################################
// Problem information methods (original data)
//#############################################################################


//-----------------------------------------------------------------------------
// Returns number of columns in MOSEK task

int OsiMskSolverInterface::getNumCols() const
{
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::getNumCols()\n");
  #endif
  
  int numcol, err;
  err = MSK_getnumvar(getMutableLpPtr(),&numcol);
  checkMSKerror( err, "MSK_getnumvar", "getNumCols" );

  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("End OsiMskSolverInterface::getNumCols()\n");
  #endif
  
  return numcol;
}

//-----------------------------------------------------------------------------
// Returns number of rows in MOSEK task

int OsiMskSolverInterface::getNumRows() const
{
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::getNumRows()\n");
  #endif
  
  int numrow, err;
  err = MSK_getnumcon(getMutableLpPtr(),&numrow);
  checkMSKerror( err, "MSK_getnumcon", "getNumRows" );

  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("End OsiMskSolverInterface::getNumRows()\n");
  #endif
  
  return numrow;
}

//-----------------------------------------------------------------------------
// Returns number of non-zeroes (in matrix) in MOSEK task

int OsiMskSolverInterface::getNumElements() const
{
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::getNumElements()\n");
  #endif

  int numnon, err;
  err = MSK_getnumanz(getMutableLpPtr(),&numnon);
  checkMSKerror( err, "MSK_getnumanz", "getNumElements" );

  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("End OsiMskSolverInterface::getNumElements()\n");
  #endif

  return numnon;
}


//-----------------------------------------------------------------------------
// Returns lower bounds on columns in MOSEK task

const double * OsiMskSolverInterface::getColLower() const
{
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::getColLower()\n");
  #endif

  if(collower_ == NULL )
  {
    int ncols = getNumCols();

    MSKassert(3,colupper_ == NULL,"colupper_","getColLower");

    if( ncols > 0 )
    {
      if(colupper_ == NULL)
        colupper_       = new double[ncols];

      if(collower_ == NULL)
        collower_       = new double[ncols];

      int *dummy_tags = new int[ncols];

      int err = MSK_getboundslice(getMutableLpPtr(), 
                                  MSK_ACC_VAR, 
                                  (MSKidxt)0, 
                                  (MSKidxt)ncols, 
                                  (MSKboundkeye*) (dummy_tags), 
                                  collower_, 
                                  colupper_);
                                  
      checkMSKerror(err, "MSK_getboundslice","getColUpper");

      for( int k = 0; k < ncols; ++k )
      {
        if( dummy_tags[k] == MSK_BK_UP ||
            dummy_tags[k] == MSK_BK_FR )
        {
          /* No lower */
          collower_[k] = -getInfinity();
        }

        if( dummy_tags[k] == MSK_BK_LO ||
            dummy_tags[k] == MSK_BK_FR )
        {
          /* No upper */
          colupper_[k] =  getInfinity();
        }
      }

      delete[] dummy_tags;

    } 
    else
    {
      if(colupper_ != NULL)
        delete [] colupper_;

      if(collower_ != NULL)
        delete [] collower_;

      colupper_ = collower_ = NULL;
    }
  }

  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("End OsiMskSolverInterface::getColLower()\n");
  #endif

  return collower_;
}

//-----------------------------------------------------------------------------
// Returns upper bounds on columns in MOSEK task

const double * OsiMskSolverInterface::getColUpper() const
{
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::getColUpper()\n");
  #endif

  if( colupper_ == NULL )
  {
    int ncols = getNumCols();

    MSKassert(3,collower_ == NULL,"collower_ == NULL","getColUpper");

    if( ncols > 0 )
    {
      if(colupper_ == NULL)
        colupper_       = new double[ncols];

      if(collower_ == NULL)
        collower_       = new double[ncols];

      int *dummy_tags = new int[ncols];
      
      int err = MSK_getboundslice( getMutableLpPtr(), 
                                   MSK_ACC_VAR, 
                                   (MSKidxt)0, 
                                   (MSKidxt)ncols, 
                                   (MSKboundkeye*) (dummy_tags), 
                                   collower_, 
                                   colupper_);
                                  
      checkMSKerror(err,"MSK_getboundslice","getColUpper");
      
      delete[] dummy_tags;
    } 
    else
    {
      if(colupper_ != NULL)
        delete [] colupper_;

      if(collower_ != NULL)
        delete [] collower_;

      colupper_ = collower_ = NULL;
    }
  }

  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("End OsiMskSolverInterface::getColUpper()\n");
  #endif

  return colupper_;
}


//-----------------------------------------------------------------------------
// Returns rowsense in MOSEK task, call getRightHandSide to produce triplets.

const char * OsiMskSolverInterface::getRowSense() const
{
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::getRowSense()\n");
  #endif

  if( rowsense_==NULL )
  {      
    getRightHandSide();

    if( getNumRows() != 0 )
      MSKassert(3,rowsense_!=NULL,"rowsense_!=NULL","getRowSense");
  }

  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("End OsiMskSolverInterface::getRowSense()\n");
  #endif

  return rowsense_;
}

//-----------------------------------------------------------------------------
// Returns the RHS in triplet form. MOSEK uses always boundkeys instead of the
// triplet, so we have to convert back to triplet. 

const double * OsiMskSolverInterface::getRightHandSide() const
{
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::getRightHandSide()\n");
  #endif

  if(rowsense_ == NULL) 
  {
    int nr = getNumRows();
    if ( nr != 0 ) 
    {
      MSKassert(3,(rhs_ == NULL) && (rowrange_ == NULL),"(rhs_ == NULL) && (rowrange_ == NULL)","getRightHandSide");
      
      rowsense_         = new char[nr];
      rhs_              = new double[nr];
      rowrange_         = new double[nr]; 
      const double * lb = getRowLower();
      const double * ub = getRowUpper();      
      int i;
      
      for ( i=0; i<nr; i++ )
        convertBoundToSense(lb[i], ub[i], rowsense_[i], rhs_[i], rowrange_[i]);
    }
  }

  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("End OsiMskSolverInterface::getRightHandSide()\n");
  #endif

  return rhs_;
}

//-----------------------------------------------------------------------------
// Returns rowrange in MOSEK task, call getRightHandSide to produce triplets.

const double * OsiMskSolverInterface::getRowRange() const
{
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::getRowRange()\n");
  #endif

  if( rowrange_ == NULL ) 
  {
    getRightHandSide();
    MSKassert(3,rowrange_!=NULL || getNumRows() == 0,"rowrange_!=NULL || getNumRows() == 0","getRowRange");
  }

  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("End OsiMskSolverInterface::getRowRange()\n");
  #endif

  return rowrange_;
}


//-----------------------------------------------------------------------------
// Returns lower bounds on rows in MOSEK task.

const double * OsiMskSolverInterface::getRowLower() const
{
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::getRowLower()\n");
  #endif

  if( rowlower_ == NULL )
  {
    MSKassert(3,rowupper_ == NULL,"rowupper_ == NULL","getRowLower");

    int nrows = getNumRows();

    if( nrows > 0 )
    {
      rowlower_       = new double[nrows];
      rowupper_       = new double[nrows];
      int *dummy_tags = new int[nrows];
        
      int err = MSK_getboundslice(getMutableLpPtr(), 
                                  MSK_ACC_CON, 
                                  (MSKidxt)0, 
                                  (MSKidxt)nrows, 
                                  (MSKboundkeye*) (dummy_tags), 
                                  rowlower_, 
                                  rowupper_);
        
      checkMSKerror(err,"MSK_getboundslice","getRowLower");
      
      delete[] dummy_tags;
    }
  }

  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("End OsiMskSolverInterface::getRowLower()\n");
  #endif

  return rowlower_;
}


//-----------------------------------------------------------------------------
// Returns upper bounds on rows in MOSEK task.

const double * OsiMskSolverInterface::getRowUpper() const
{  
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::getRowUpper()\n");
  #endif

  if( rowupper_ == NULL )
  {
    MSKassert(3,rowlower_ == NULL,"probtypemip_","getRowUpper");

    int nrows = getNumRows();
    
    if( nrows > 0 )
    {
      rowupper_       = new double[nrows];
      rowlower_       = new double[nrows];
      int *dummy_tags = new int[nrows];
        
      int err = MSK_getboundslice(getMutableLpPtr(), 
                                  MSK_ACC_CON, 
                                  (MSKidxt)0,
                                  (MSKidxt)nrows, 
                                  (MSKboundkeye*) (dummy_tags), 
                                  rowlower_, 
                                  rowupper_);
        
      checkMSKerror(err,"MSK_getboundslice","getRowUpper");
      
      delete[] dummy_tags;
    }
  }

  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("End OsiMskSolverInterface::getRowUpper()\n");
  #endif

  return rowupper_;
}


//-----------------------------------------------------------------------------
// Returns objective coefficient in MOSEK task.


const double * OsiMskSolverInterface::getObjCoefficients() const
{
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::getObjCoefficients()\n");
  #endif

  if( obj_ == NULL )
  {
    int ncols = getNumCols();

    if( ncols > 0 )
    {
      obj_    = new double[ncols]; 
      int err = MSK_getc( getMutableLpPtr(), obj_ );
      
      checkMSKerror( err, "MSK_getc", "getObjCoefficients" );
    }
  }

  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("End OsiMskSolverInterface::getObjCoefficients()\n");
  #endif

  return obj_;
}


//-----------------------------------------------------------------------------
// Returns the direction of optimization

double OsiMskSolverInterface::getObjSense() const
{
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::getObjSense()\n");
  #endif

  int err;
  MSKobjsensee objsen;

  err = MSK_getobjsense(getMutableLpPtr(),
                        &objsen);

  checkMSKerror(err,"MSK_getintparam","getObjSense");

  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("End OsiMskSolverInterface::getObjSense()\n");
  #endif

  if( objsen == MSK_OBJECTIVE_SENSE_MAXIMIZE )
    return -1.0;
  else
    return +1.0;
}


//-----------------------------------------------------------------------------
// Returns true if variabel is set to continuous

bool OsiMskSolverInterface::isContinuous( int colNumber ) const
{
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::isContinuous(%d)\n", colNumber);
  debugMessage("End OsiMskSolverInterface::isContinuous(%d)\n", colNumber);
  #endif
  
  return getCtype()[colNumber] == 'C';
}


//-----------------------------------------------------------------------------
// Returns a Coin matrix by row

const CoinPackedMatrix * OsiMskSolverInterface::getMatrixByRow() const
{
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::getMatrixByRow()\n");
  #endif

  if ( matrixByRow_ == NULL ) 
  {
    int nc, nr, nz, *sub, *ptrb, *ptre, surp, *len;
    double *val;
    nc       = getNumCols();
    nr       = getNumRows();
    nz       = surp = getNumElements();
    ptrb     = new int[nr+1];
    ptre     = new int[nr];
    sub      = new int[nz];
    val      = new double[nz];
    len      = new int[nr];
    ptrb[nr] = nz;

    int err = MSK_getaslice(getMutableLpPtr(),
                            MSK_ACC_CON,
                            0,
                            nr,
                            nz,
                            &surp, 
                            ptrb, 
                            ptre, 
                            sub, 
                            val); 

    checkMSKerror(err, "MSK_getaslice", "getMatrixByRow");

    for(int i=0; i<nr; i++)
      len[i] = ptre[i]-ptrb[i];

    matrixByRow_ = new CoinPackedMatrix();
    matrixByRow_->assignMatrix(false , nc, nr, nz, val, sub, ptrb, len);

    MSKassert(3,matrixByRow_->getNumCols()==nc,"matrixByRow_->getNumCols()==nc","getMatrixByRow");
    MSKassert(3,matrixByRow_->getNumRows()==nr,"matrixByRow_->getNumRows()==nr","getMatrixByRow");

    delete[] ptrb;
    delete[] ptre;
    delete[] sub;
    delete[] val;    
    delete[] len;
  }

  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("End OsiMskSolverInterface::getMatrixByRow()\n");
  #endif

  return matrixByRow_; 
} 


//-----------------------------------------------------------------------------
// Returns a Coin matrix by column

const CoinPackedMatrix * OsiMskSolverInterface::getMatrixByCol() const
{
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::getMatrixByCol()\n");
  #endif

  if ( matrixByCol_ == NULL ) 
  {
    int nc, nr, nz, *sub, *ptrb, *ptre, surp, *len;
    double *val;

    nc       = getNumCols();
    nr       = getNumRows();
    nz       = surp = getNumElements();
    ptrb     = new int[nc+1];
    ptre     = new int[nc];
    sub      = new int[nz];
    val      = new double[nz];
    len      = new int[nc];
    ptrb[nc] = nz;

    int err = MSK_getaslice(getMutableLpPtr(),
                            MSK_ACC_VAR,
                            0,
                            nc,
                            nz,
                            &surp, 
                            ptrb, 
                            ptre, 
                            sub, 
                            val); 

    checkMSKerror(err, "MSK_getaslice", "getMatrixByCol");

    for(int i=0; i<nc; i++)
      len[i] = ptre[i]-ptrb[i];

    matrixByCol_ = new CoinPackedMatrix();
    matrixByCol_->assignMatrix(true , nr, nc, nz, val, sub, ptrb, len);

    MSKassert(3,matrixByCol_->getNumCols()==nc,"matrixByCol_->getNumCols()==nc","getMatrixByCol");
    MSKassert(3,matrixByCol_->getNumRows()==nr,"matrixByCol_->getNumRows()==nr","getMatrixByCol");

    delete[] ptrb;
    delete[] ptre;
    delete[] sub;
    delete[] val;    
    delete[] len;
  }

  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("End OsiMskSolverInterface::getMatrixByCol()\n");
  #endif

  return matrixByCol_; 
} 


//-----------------------------------------------------------------------------
// Returns the infinity level used in MOSEK.

double OsiMskSolverInterface::getInfinity() const
{
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::getInfinity()\n");
  debugMessage("End OsiMskSolverInterface::getInfinity()\n");
  #endif

  return MSK_INFINITY;
}

//#############################################################################
// Problem information methods (results)
//#############################################################################

//-----------------------------------------------------------------------------
// Returns the current col solution. 

const double * OsiMskSolverInterface::getColSolution() const
{
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::getColSolution() %p\n",(void*)this);
  #endif

  if( colsol_ != NULL )
  {
    #if MSK_OSI_DEBUG_LEVEL > 3
    debugMessage("colsol_ != NULL\n");
    #endif

    return colsol_;
  }

  int i;
  int nc = getNumCols();
  if( nc > 0 )
  {
    MSKsoltypee solution = MSK_SOL_END;

    if(colsol_ == NULL)
      colsol_ = new double[nc];

    if( probtypemip_ == false)
    {
      if( definedSolution( MSK_SOL_BAS ) == true )
        solution = MSK_SOL_BAS;
      else if( definedSolution( MSK_SOL_ITR) == true )
        solution = MSK_SOL_ITR;
    }
    else if( definedSolution( MSK_SOL_ITG ) == true )
      solution = MSK_SOL_ITG;  

    if ( solution == MSK_SOL_END )
    {
      double const *cl=getColLower(),*cu=getColLower();

      /* this is just plain stupid, but needed to pass unit test */
      for( i = 0; i < nc; ++i )
      {
        if( cl[i] > -getInfinity() )
        {
          colsol_[i] = cl[i];
        }
        else if( cu[i] < getInfinity() )
        {
          colsol_[i] = cu[i];
        }
        else
        {
          colsol_[i] = 0.0;
        }
      }

      #if MSK_OSI_DEBUG_LEVEL > 3
      debugMessage("colsol_ truncated to zero due to no solution\n");
      debugMessage("probtypemip_ %d\n",probtypemip_);
      #endif

      return colsol_;
    }

    int err = MSK_getsolution(getMutableLpPtr(),
                              solution,
                              NULL,
                              NULL,
                              NULL, 
                              NULL,
                              NULL,
                              NULL,
                              colsol_,
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              NULL
                              );

    checkMSKerror(err,"MSK_getsolution","getColSolution");
  }

  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("End OsiMskSolverInterface::getColSolution()\n");
  #endif

  return colsol_;
}

//-----------------------------------------------------------------------------
// Returns the row price / dual variabels in MOSEK task

const double * OsiMskSolverInterface::getRowPrice() const
{
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::getRowPrice()\n");
  #endif

  if( rowsol_ == NULL )
  {
    int i;
    int nr = getNumRows();

    if( nr > 0 )
    {
      MSKsoltypee solution = MSK_SOL_END;
      rowsol_      = new double[nr];

      if( definedSolution( MSK_SOL_BAS ) == true )
        solution = MSK_SOL_BAS;
      else if( definedSolution( MSK_SOL_ITR) == true )
        solution = MSK_SOL_ITR;
     
      if ( solution == MSK_SOL_END )
      {
        for( i = 0; i < nr; ++i )
           rowsol_[i] = 0.0;

        return rowsol_;
      }

      int err = MSK_getsolution(getMutableLpPtr(),
                                solution,
                                NULL,
                                NULL,
                                NULL, 
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                rowsol_,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL);

                                
      checkMSKerror( err, "MSK_getsolution", "getRowPrice" );
    }
  }

  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("End OsiMskSolverInterface::getRowPrice()\n");
  #endif

  return rowsol_;
}

//-----------------------------------------------------------------------------
// Returns the reduced cost in MOSEK task.

const double * OsiMskSolverInterface::getReducedCost() const
{
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::getReducedCost()\n");
  #endif

  if( redcost_ == NULL )
  {
    int ncols = getNumCols();

    if( ncols > 0 )
    {
      MSKsoltypee solution = MSK_SOL_END;
      if( definedSolution( MSK_SOL_BAS ) == true )
        solution = MSK_SOL_BAS;
      else if( definedSolution( MSK_SOL_ITR) == true )
        solution = MSK_SOL_ITR;

      if ( solution == MSK_SOL_END )
        return NULL;

      redcost_    = new double[ncols];
      double *slx = new double[ncols];
      double *sux = new double[ncols];

      int err = MSK_getsolution(getMutableLpPtr(),
                                solution,
                                NULL,
                                NULL,
                                NULL, 
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                slx,
                                sux,
                                NULL);

      // Calculate reduced cost
      for(int i = 0; i < ncols; i++)
        redcost_[i] = slx[i]-sux[i];

      delete[] slx;
      delete[] sux;

      checkMSKerror( err, "MSK_getsolution", "getReducedCost" );
    }
  }

  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("End OsiMskSolverInterface::getReducedCost()\n");
  #endif

  return  redcost_;
}


//-----------------------------------------------------------------------------
// Returns the rowactivity in MOSEK task.

const double * OsiMskSolverInterface::getRowActivity() const
{
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::getRowActivity()\n");
  #endif

  if( rowact_ == NULL )
  {
    int nrows = getNumRows();
    if( nrows > 0 )
    {
      rowact_ = new double[nrows];

      const double *x = getColSolution() ;
      const CoinPackedMatrix *mtx = getMatrixByRow() ;
      mtx->times(x,rowact_) ;
   }
 }

 #if MSK_OSI_DEBUG_LEVEL > 3
 debugMessage("End OsiMskSolverInterface::getRowActivity()\n");
 #endif

 return  rowact_;
}

//-----------------------------------------------------------------------------
// Returns the objective for defined solution in MOSEK task. 

double OsiMskSolverInterface::getObjValue() const
{
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::getObjValue()\n");
  #endif

  double objval = OsiSolverInterface::getObjValue();

  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("End OsiMskSolverInterface::getObjValue()\n");
  #endif

  return objval;
}

//-----------------------------------------------------------------------------
// Returns the iteration used in last call to optimize. Notice that the cross
// over phase in interior methods is not returned, when interior point is
// used, only the interior point iterations.

int OsiMskSolverInterface::getIterationCount() const
{
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::getIterationCount()\n");
  #endif

  int nr = 0, solver, err;
  int nrp=0;
  solver = solverUsed();

  if( solver == MSK_OPTIMIZER_PRIMAL_SIMPLEX )
  {
    {
      err = MSK_getintinf(getMutableLpPtr(), MSK_IINF_SIM_PRIMAL_ITER, &nr);
      checkMSKerror(err,"MSK_getintinf","getIterationsCount");
    }
  }
  else if( solver == MSK_OPTIMIZER_DUAL_SIMPLEX  )
  {
    {
      err = MSK_getintinf(getMutableLpPtr(), MSK_IINF_SIM_DUAL_ITER, &nr);
      checkMSKerror(err,"MSK_getintinf","getIterationsCount");
    }
  }
  else if( solver == MSK_OPTIMIZER_FREE_SIMPLEX  )
  {
    {
      err = MSK_getintinf(getMutableLpPtr(), MSK_IINF_SIM_DUAL_ITER, &nr);
      checkMSKerror(err,"MSK_getintinf","getIterationsCount");
      err = MSK_getintinf(getMutableLpPtr(), MSK_IINF_SIM_PRIMAL_ITER, &nrp);
      checkMSKerror(err,"MSK_getintinf","getIterationsCount");
      nr = nr+nrp;
    }
  }
  else if( solver == MSK_OPTIMIZER_INTPNT )
  {
    {
      err = MSK_getintinf(getMutableLpPtr(), MSK_IINF_INTPNT_ITER, &nr);
      checkMSKerror(err,"MSK_getintinf","getIterationsCount");
    }
  }

  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("End OsiMskSolverInterface::getIterationCount()\n");
  #endif
    
  return nr;
}

//-----------------------------------------------------------------------------
// Returns one dual ray

std::vector<double*> OsiMskSolverInterface::getDualRays(int maxNumRays,
							bool fullRay) const
{
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::getDualRays(%d,%s)\n", maxNumRays,
	       fullRay?"true":"false");
  #endif

  if (fullRay == true) {
    throw CoinError("Full dual rays not yet implemented.","getDualRays",
		    "OsiMskSolverInterface");
  }

  OsiMskSolverInterface solver(*this);

  int numrows = getNumRows(), r;
  MSKsolstae status;
  MSKsoltypee solution;

  if( probtypemip_ == false )
  {
    if( definedSolution( MSK_SOL_BAS ) == true )
      solution = MSK_SOL_BAS;
    else
    {
      if( definedSolution( MSK_SOL_ITR ) == true )
        solution = MSK_SOL_ITR;
      else
        return std::vector<double*>();
    }
  }
  else
  {
    if( definedSolution( MSK_SOL_ITG ) == true )
      solution = MSK_SOL_ITG;
    else
      return std::vector<double*>();     
  }

  double *farkasray = new double[numrows];
  r = MSK_getsolution(getMutableLpPtr(),
                      solution,
                      NULL, 
                      &status, 
                      NULL, 
                      NULL, 
                      NULL,
                      NULL,
                      NULL,
                      farkasray,
                      NULL,
                      NULL,
                      NULL,
                      NULL,
                      NULL);

  checkMSKerror( r, "MSK_getsolution", "getDualRays" );

  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("End OsiMskSolverInterface::getDualRays(%d)\n", maxNumRays);
  #endif

  if( status != MSK_SOL_STA_PRIM_INFEAS_CER )
  { 
    delete[] farkasray;
    
    return std::vector<double*>();   
  }
  else
    return std::vector<double*>(1, farkasray);
}

//-----------------------------------------------------------------------------
// Returns one primal ray

std::vector<double*> OsiMskSolverInterface::getPrimalRays(int maxNumRays) const
{
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::getPrimalRays(%d)\n", maxNumRays);
  #endif

  OsiMskSolverInterface solver(*this);

  int numcols = getNumCols(), r;
  MSKsolstae status;
  MSKsoltypee solution;

  if( probtypemip_ == false )
  {
    if( definedSolution( MSK_SOL_BAS ) == true )
      solution = MSK_SOL_BAS;
    else
    {
      if( definedSolution( MSK_SOL_ITR ) == true )
        solution = MSK_SOL_ITR;
      else
        return std::vector<double*>();	
    }
  }
  else
  {
    if( definedSolution( MSK_SOL_ITG ) == true )
      solution = MSK_SOL_ITG;
    else
      return std::vector<double*>();     
  }

  double *farkasray = new double[numcols];

  r = MSK_getsolution(getMutableLpPtr(),
                      solution,
                      NULL, 
                      &status, 
                      NULL, 
                      NULL, 
                      NULL,
                      NULL,
                      farkasray,
                      NULL,
                      NULL,
                      NULL,
                      NULL,
                      NULL,
                      NULL);

  checkMSKerror( r, "MSK_getsolution", "getPrimalRays" );

  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("End OsiMskSolverInterface::getPrimalRays(%d)\n", maxNumRays);
  #endif

  if( status != MSK_SOL_STA_DUAL_INFEAS_CER )
  { 
    delete[] farkasray;
    
    return std::vector<double*>();   
  }
  else
    return std::vector<double*>(1, farkasray);
}

//#############################################################################
// Problem modifying methods (rim vectors)
//#############################################################################

//-----------------------------------------------------------------------------
// Sets a variabels objective coeff.

void OsiMskSolverInterface::setObjCoeff( int elementIndex, double elementValue )
{
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::setObjCoeff(%d, %g)\n", elementIndex, elementValue);
  #endif

  const double *oldobj;

  if( redcost_  && !obj_ )
    oldobj = getObjCoefficients();
  else
    oldobj = obj_;

  int err = MSK_putclist(getMutableLpPtr(), 
                          1, 
                          &elementIndex, 
                          &elementValue);

  checkMSKerror(err, "MSK_putclist", "setObjCoeff");

  if( obj_  )
  {
    if( redcost_  )
    {
      redcost_[elementIndex] += elementValue-oldobj[elementIndex];
      obj_[elementIndex]      = elementValue;
    }
    else
    {
      obj_[elementIndex] = elementValue;
    }
  }

  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("End OsiMskSolverInterface::setObjCoeff(%d, %g)\n", elementIndex, elementValue);
  #endif
}

//-----------------------------------------------------------------------------
// Sets a list of objective coeff.

void OsiMskSolverInterface::setObjCoeffSet(const int* indexFirst,
                                           const int* indexLast,
                                           const double* coeffList)
{
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::setObjCoeffSet(%p, %p, %p)\n", (void *)indexFirst, (void *)indexLast, (void *)coeffList);
  #endif

  const double *oldobj;
  const long int cnt = indexLast - indexFirst;

  if( redcost_  && !obj_ )
    oldobj = getObjCoefficients();
  else
    oldobj = obj_;

  int err = MSK_putclist(getMutableLpPtr(),
                         static_cast<int>(cnt),
                         const_cast<int*>(indexFirst),
                         const_cast<double*>(coeffList));

  checkMSKerror(err, "MSK_putclist", "setObjCoeffSet");

  if( obj_  )
  {
    if( redcost_  )
    {
      for( int j = 0; j < cnt; ++j)
      {
        redcost_[j] += coeffList[indexFirst[j]]-oldobj[j];
        obj_[j]      = coeffList[indexFirst[j]];
      }
    }
    else
    {
      for( int j = 0; j < cnt; ++j)
      {
        obj_[j] = coeffList[indexFirst[j]];
      }
    }
  }


  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("End OsiMskSolverInterface::setObjCoeffSet(%p, %p, %p)\n", (void *)indexFirst, (void *)indexLast, (void *)coeffList);
  #endif
}

//-----------------------------------------------------------------------------
// Sets lower bound on one specific column

void OsiMskSolverInterface::setColLower(int elementIndex, double elementValue)
{
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::setColLower(%d, %g)\n", elementIndex, elementValue);
  #endif

  int finite = 1;
  
  if( elementValue <= -getInfinity() )
    finite = 0;
  
  int err = MSK_chgbound(getMutableLpPtr(), 
                         MSK_ACC_VAR,
                         elementIndex,
                         1,       /*It is a lower bound*/
                         finite,  /* Is it finite */ 
                         elementValue);

  checkMSKerror( err, "MSK_chgbound", "setColLower" );
   
  if( collower_ != NULL )
    collower_[elementIndex] = elementValue;

  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("End OsiMskSolverInterface::setColLower(%d, %g)\n", elementIndex, elementValue);
  #endif
}

//-----------------------------------------------------------------------------
// Sets upper bound on one specific column. 

void OsiMskSolverInterface::setColUpper(int elementIndex, double elementValue)
{  
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::setColUpper(%d, %g)\n", elementIndex, elementValue);
  #endif

  int finite = 1;

  if( elementValue >= getInfinity() )
    finite = 0;

  int err = MSK_chgbound( getMutableLpPtr(), 
                          MSK_ACC_VAR,
                          elementIndex,
                          0,       /* It is a upper bound */
                          finite,  /* Is it finite */
                          elementValue);

  checkMSKerror( err, "MSK_chgbound", "setColUpper" );
    
  if( colupper_ != NULL )
    colupper_[elementIndex] = elementValue;

  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("End OsiMskSolverInterface::setColUpper(%d, %g)\n", elementIndex, elementValue);
  #endif
} 

//-----------------------------------------------------------------------------
// Sets upper and lower bound on one specific column 

void OsiMskSolverInterface::setColBounds( int elementIndex, double lower, double upper )
{
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::setColBounds(%d, %g, %g)\n", elementIndex, lower, upper);
  #endif

  setColLower(elementIndex, lower);
  setColUpper(elementIndex, upper);

  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("End OsiMskSolverInterface::setColBounds(%d, %g, %g)\n", elementIndex, lower, upper);
  #endif
}

//-----------------------------------------------------------------------------
// Sets upper and lower bounds on a list of columns. Due to the strange storage of
// boundlist, it is not possible to change all the bounds in one call to MOSEK,
// so the standard method is used. 

void OsiMskSolverInterface::setColSetBounds(const int* indexFirst,
                                             const int* indexLast,
                                             const double* boundList)
{
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::setColSetBounds(%p, %p, %p)\n", (void *)indexFirst, (void *)indexLast, (void *)boundList);
  #endif

  OsiSolverInterface::setColSetBounds( indexFirst, indexLast, boundList );

  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("End OsiMskSolverInterface::setColSetBounds(%p, %p, %p)\n", (void *)indexFirst, (void *)indexLast, (void *)boundList);
  #endif
}

//-----------------------------------------------------------------------------
// Sets the lower bound on a row

void
OsiMskSolverInterface::setRowLower( int i, double elementValue )
{
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::setRowLower(%d, %g)\n", i, elementValue);
  #endif

  double rhs   = getRightHandSide()[i];
  double range = getRowRange()[i];
  char   sense = getRowSense()[i];
  double lower=-MSK_INFINITY, upper=MSK_INFINITY;

  convertSenseToBound( sense, rhs, range, lower, upper );
  if( lower != elementValue ) 
  {
    convertBoundToSense( elementValue, upper, sense, rhs, range );
    setRowType( i, sense, rhs, range );
  }

  #if MSK_OSI_DEBUG_LEVEL > 3  
  debugMessage("End OsiMskSolverInterface::setRowLower(%d, %g)\n", i, elementValue);
  #endif
}

//-----------------------------------------------------------------------------
// Sets the upper bound on a row 

void
OsiMskSolverInterface::setRowUpper( int i, double elementValue )
{
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::setRowUpper(%d, %g)\n", i, elementValue);
  #endif

  double rhs   = getRightHandSide()[i];
  double range = getRowRange()[i];
  char   sense = getRowSense()[i];
  double lower=-MSK_INFINITY, upper=MSK_INFINITY;

  convertSenseToBound( sense, rhs, range, lower, upper );
  if( upper != elementValue ) 
  {
    convertBoundToSense( lower, elementValue, sense, rhs, range );
    setRowType( i, sense, rhs, range );
  }

  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("End OsiMskSolverInterface::setRowUpper(%d, %g)\n", i, elementValue);
  #endif
}

//-----------------------------------------------------------------------------
// Sets the upper and lower bound on a row 

void
OsiMskSolverInterface::setRowBounds( int elementIndex, double lower, double upper )
{
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::setRowBounds(%d, %g, %g)\n", elementIndex, lower, upper);
  #endif

  double rhs, range;
  char sense;
  
  convertBoundToSense( lower, upper, sense, rhs, range );
  setRowType( elementIndex, sense, rhs, range );

  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("End OsiMskSolverInterface::setRowBounds(%d, %g, %g)\n", elementIndex, lower, upper);
  #endif
}

//-----------------------------------------------------------------------------
// Sets the triplet on a row  

void
OsiMskSolverInterface::setRowType(int i, char sense, double rightHandSide,double range)
{
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::setRowType(%d, %c, %g, %g)\n", i, sense, rightHandSide, range);
  #endif

  double rub=MSK_INFINITY,rlb=-MSK_INFINITY;
  int rtag=MSK_BK_FR;

  MskConvertSenseToBound(sense, range, rightHandSide, rlb, rub, rtag); 

  int err = MSK_putbound(getMutableLpPtr(),
                         MSK_ACC_CON, 
                         i, 
                         (MSKboundkeye)rtag, 
                         rlb, 
                         rub);

  if( rowsense_ != NULL )
     rowsense_[i] = sense;

  if( rowrange_ != NULL )
     rowrange_[i] = range;

  if( rhs_      != NULL )
     rhs_[i]      = rightHandSide;
                  
  checkMSKerror( err, "MSK_putbound", "setRowType" );

  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("End OsiMskSolverInterface::setRowType(%d, %c, %g, %g)\n", i, sense, rightHandSide, range);
  #endif
}

//-----------------------------------------------------------------------------
// Set upper og lower bounds for a lisit of rows. Due to the strange storage of
// boundlist, it is not possible to change all the bounds in one call to MOSEK,
// so the standard method is used. 

void OsiMskSolverInterface::setRowSetBounds(const int* indexFirst,
                                              const int* indexLast,
                                              const double* boundList)
{
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::setRowSetBounds(%p, %p, %p)\n", (void *)indexFirst, (void *)indexLast, (void *)boundList);
  #endif

  const long int cnt = indexLast - indexFirst;

  if (cnt <= 0)
    return;

  for (int i = 0; i < cnt; ++i)
    setRowBounds(indexFirst[i], boundList[2*i], boundList[2*i+1]);

  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("End OsiMskSolverInterface::setRowSetBounds(%p, %p, %p)\n", (void *)indexFirst, (void *)indexLast, (void *)boundList);
  #endif
}


//-----------------------------------------------------------------------------
// Set triplets for a list of rows 

void
OsiMskSolverInterface::setRowSetTypes(const int* indexFirst,
                                       const int* indexLast,
                                       const char* senseList,
                                       const double* rhsList,
                                       const double* rangeList)
{
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::setRowSetTypes(%p, %p, %p, %p, %p)\n", 
   (void *)indexFirst, (void *)indexLast, (void *)senseList, (void *)rhsList, (void *)rangeList);
  #endif

  const long int cnt = indexLast - indexFirst;
  
  if (cnt <= 0)
    return;

  for (int i = 0; i < cnt; ++i)
    setRowType(indexFirst[i], senseList[i], rhsList[i], rangeList[i]);

  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("End OsiMskSolverInterface::setRowSetTypes(%p, %p, %p, %p, %p)\n", 
                (void *)indexFirst, (void *)indexLast, (void *)senseList, (void *)rhsList, (void *)rangeList);
  #endif
}


//-----------------------------------------------------------------------------
// Sets a variabel to continuous

void
OsiMskSolverInterface::setContinuous(int index)
{
  int numcols = getNumCols();

  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::setContinuous(%d)\n", index);
  #endif

  MSKassert(3,coltype_ != NULL,"coltype_ != NULL","setContinuous");
  MSKassert(3,coltypesize_ >= getNumCols(),"coltypesize_ >= getNumCols()","setContinuous");

  coltype_[index] = 'C';
  
  if( index < numcols )
  {
    int err = MSK_putvartype( getMutableLpPtr(), index, MSK_VAR_TYPE_CONT);
    checkMSKerror( err, "MSK_putvartype", "setContinuous" );    
  }

  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("End OsiMskSolverInterface::setContinuous(%d)\n", index);
  #endif
}

//-----------------------------------------------------------------------------
// Sets a variabel to integer

void
OsiMskSolverInterface::setInteger(int index)
{
  int numcols = getNumCols();

  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::setInteger(%d)\n", index);
  #endif

  MSKassert(3,coltype_ != NULL,"coltype_ != NULL","setInteger");
  MSKassert(3,coltypesize_ >= getNumCols(),"coltypesize_ >= getNumCols()","setInteger");
  
  coltype_[index] = 'I';

  if( index < numcols )
  {
    int err = MSK_putvartype( getMutableLpPtr(), index, MSK_VAR_TYPE_INT);
    
    checkMSKerror( err, "MSK_putvartype", "setInteger" );
  }

  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("End OsiMskSolverInterface::setInteger(%d)\n", index);
  #endif
}

//-----------------------------------------------------------------------------
// Sets a list of variables to continuous

void
OsiMskSolverInterface::setContinuous(const int* indices, int len)
{
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::setContinuous(%p, %d)\n", (void *)indices, len);
  #endif

  for( int i = 0; i < len; ++i )
    setContinuous(indices[i]);

  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("End OsiMskSolverInterface::setContinuous(%p, %d)\n", (void *)indices, len);
  #endif
}

//-----------------------------------------------------------------------------
// Sets a list of variables to integer 

void
OsiMskSolverInterface::setInteger(const int* indices, int len)
{  
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::setInteger(%p, %d)\n", (void *)indices, len);
  #endif

  for( int i = 0; i < len; ++i )
     setInteger(indices[i]);

  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("End OsiMskSolverInterface::setInteger(%p, %d)\n", (void *)indices, len);
  #endif
}

//-----------------------------------------------------------------------------
// Sets the direction of optimization

void OsiMskSolverInterface::setObjSense(double s) 
{
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::setObjSense(%g)\n", s);
  #endif

  int err;
  double pre;

  pre = getObjSense();

  if( s == +1.0 )
  {
     err = MSK_putobjsense(getMutableLpPtr(),
                           MSK_OBJECTIVE_SENSE_MINIMIZE);
  }
  else
  {
    err = MSK_putobjsense(getMutableLpPtr(),
                          MSK_OBJECTIVE_SENSE_MAXIMIZE);
  }

  checkMSKerror(err,"MSK_putintparam","setObjSense");

  if( pre != s )
  {
    /* A hack to pass unit test, ugly as hell */
    /* When objective sense is changed then reset obj cuts */
    if( s > 0 )
    {
      setDblParam(OsiPrimalObjectiveLimit,-COIN_DBL_MAX) ;
      setDblParam(OsiDualObjectiveLimit,COIN_DBL_MAX) ;
    }
    else
    {
      setDblParam(OsiPrimalObjectiveLimit,COIN_DBL_MAX) ;
      setDblParam(OsiDualObjectiveLimit,-COIN_DBL_MAX) ;
    }
  }

  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("End OsiMskSolverInterface::setObjSense(%g)\n", s);
  #endif
}

//-----------------------------------------------------------------------------
// Sets the col solution. This is very fuzzy WE LIKE STATUS KEYS not numerical values i.e superbasics or basic !!!.

void OsiMskSolverInterface::setColSolution(const double * cs) 
{
  #if MSK_OSI_DEBUG_LEVEL > 1
  debugMessage("Begin OsiMskSolverInterface::setColSolution %p\n", (void*)this);
  #endif
 
  int err,nc = getNumCols(),nr = getNumRows(), numbas = 0;
  MSKstakeye *tskc,*tskx;
  MSKboundkeye *tbkx,*tbkc;
  MSKrealt *tblx,*tbux,*tblc,*tbuc;
  double *txc;
  
  if( cs == NULL )
  {
    freeCachedResults();
  }
  else if( nc > 0 )
  {
    if( colsol_ != NULL )
        delete[] colsol_;
    
    colsol_ = new double[nc];
    
    CoinDisjointCopyN( cs, nc, colsol_ );

    tbkx    = new MSKboundkeye[nc];    
    tskx    = new MSKstakeye[nc];        
    tblx    = new MSKrealt[nc];    
    tbux    = new MSKrealt[nc];    

    tskc    = new MSKstakeye[nr]; 
    txc     = new double[nr];    
    tbkc    = new MSKboundkeye[nr];    
    tblc    = new MSKrealt[nr];    
    tbuc    = new MSKrealt[nr];    

    const CoinPackedMatrix *mtx = getMatrixByCol() ;
    assert(mtx->getNumCols() == nc);
    assert(mtx->getNumRows() == nr);
    mtx->times(cs,txc) ;

    /* Negate due to different Osi and Mosek slack representation */
    for( int i = 0; i < nr; ++i )
    {
      txc[i]  = -txc[i];
      tskc[i] = MSK_SK_UNK; 
    }
    
    err = MSK_getboundslice(getMutableLpPtr(),
                            MSK_ACC_CON,
                            0,
                            nr,
                            tbkc, 
                            tblc,
                            tbuc);
    
    checkMSKerror( err, "MSK_getboundslice", "setColsolution" );

    err = MSK_getboundslice(getMutableLpPtr(),
                            MSK_ACC_VAR,
                            0,
                            nc,
                            tbkx, 
                            tblx,
                            tbux);
    
    checkMSKerror( err, "MSK_getboundslice", "setColsolution" );
    
    if( definedSolution( MSK_SOL_BAS ) == true )
    {
      err = MSK_getsolution(getMutableLpPtr(), 
                            MSK_SOL_BAS, 
                            NULL, 
                            NULL,
                            tskc, 
                            tskx,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            NULL);

      checkMSKerror(err,"MSK_getsolution","setColSol");
    }

    for( int i = 0; i < nr; ++i )
    {
      if( tbkc[i] == MSK_BK_FX && tblc[i] == txc[i] )
      {
        tskc[i] = MSK_SK_FIX;        
      }
      else if( ( tbkc[i] == MSK_BK_LO || tbkc[i] == MSK_BK_RA ) && tblc[i] == txc[i]   )
      {
        tskc[i] = MSK_SK_LOW;
      }        
      else if( ( tbkc[i] == MSK_BK_UP || tbkc[i] == MSK_BK_RA ) && tbuc[i] == txc[i]   )
      {
        tskc[i] = MSK_SK_UPR;
      }        
      else if( tbkc[i] == MSK_BK_FR  && txc[i] == 0.0   )
      {
        tskc[i] = MSK_SK_SUPBAS;
      }        
      else 
      {
        #if 0
        printf("Slack : %d bkc : %d blc : %-16.10e buc : %-16.10e xc : %-16.10e\n",
               i,tbkc[i],tblc[i],tbuc[i],txc[i]);
        #endif

        ++numbas;
        
        tskc[i] = MSK_SK_BAS;
      }        
    }

    for( int j = 0; j < nc; ++j )
    {
      if( tbkx[j] == MSK_BK_FX && tblx[j] == cs[j] )
      {
        tskx[j] = MSK_SK_FIX;        
      }
      else if( ( tbkx[j] == MSK_BK_LO || tbkx[j] == MSK_BK_RA ) && tblx[j] == cs[j]   )
      {
        tskx[j] = MSK_SK_LOW;
      }        
      else if( ( tbkx[j] == MSK_BK_UP || tbkx[j] == MSK_BK_RA ) && tbux[j] == cs[j]   )
      {
        tskx[j] = MSK_SK_UPR;
      }        
      else if( ( tbkx[j] == MSK_BK_FR  && cs[j] == 0.0 ) ||  numbas >= nr   )
      {
        tskx[j] = MSK_SK_SUPBAS;
      }        
      else 
      {
        #if 0
        printf("Org %d : bkx : %d blx : %-16.10e bux : %-16.10e xx : %-16.10e\n",
               j,tbkx[j],tblx[j],tbux[j],cs[j]);
        #endif
        
        tskx[j] = MSK_SK_BAS;
      }        
    }  

    err = MSK_putsolution(getMutableLpPtr(),
                           MSK_SOL_BAS,
                           tskc,
                           tskx,
                           NULL,
                           (MSKrealt*) txc,
                           const_cast<MSKrealt*>(cs),
                           NULL,
                           NULL,
                           NULL,
                           NULL,
                           NULL,
                           NULL);
    
    checkMSKerror(err,"MSK_putsolution","setColSol");
    
    
    MSKassert(3,definedSolution( MSK_SOL_BAS ) == true,"definedSolution( MSK_SOL_BAS ) == true","setColSolution");

    delete [] tbkx;
    delete [] tblx;
    delete [] tbux;
    delete [] tskx;
    delete [] tskc;
    delete [] txc;
    delete [] tbkc;
    delete [] tblc;
    delete [] tbuc;
  }

  #if MSK_OSI_DEBUG_LEVEL > 1
  debugMessage("End OsiMskSolverInterface::setColSolution(%p)\n", (void *)cs);
  #endif
}

//-----------------------------------------------------------------------------
// Sets the rowprices. 

void OsiMskSolverInterface::setRowPrice(const double * rs) 
{
  #if MSK_OSI_DEBUG_LEVEL > 1
  debugMessage("Begin OsiMskSolverInterface::setRowPrice(%p)\n", (void *)rs);
  #endif

  int err,nr = getNumRows(),nc = getNumCols();
  MSKstakeye *tskc,*tskx;
  MSKrealt *tslc,*tsuc,*tslx,*tsux,sn,*txc,*txx;
  double *redcost;

  if( rs == NULL )
    freeCachedResults();
  else if( nr > 0 )
  {
    if ( rowsol_ == NULL )
      rowsol_ = new double[nr];

    colsol_ = new double[nc];

    tskc = new MSKstakeye[nr];
    tslc = new MSKrealt[nr];
    tsuc = new MSKrealt[nr];
    txc  = new MSKrealt[nr];

    tskx = new MSKstakeye[nc];
    tslx = new MSKrealt[nc];
    tsux = new MSKrealt[nc];
    txx  = new MSKrealt[nc];

    redcost = new double[nc];
    
    CoinDisjointCopyN( rs, nr, rowsol_ ); 

    /* Calc reduced costs  */
    const CoinPackedMatrix *mtx = getMatrixByCol() ;
    mtx->transposeTimes(rs,redcost) ;

    for( int j = 0; j < nc; ++j )
    {    
      redcost[j] = getObjCoefficients()[j]-redcost[j];
    
      tslx[j] = CoinMax(0.0,redcost[j]);
      tsux[j] = CoinMax(0.0,-redcost[j]);    
    }

    if( definedSolution( MSK_SOL_BAS ) == true )
    {
      for( int i = 0; i < nr; ++i )
      {
        err = MSK_getsolutioni(getMutableLpPtr(),
                               MSK_ACC_CON,
                               i,
                               MSK_SOL_BAS, 
                               &tskc[i], 
                               &txc[i], 
                               &tslc[i],
                               &tsuc[i], 
                               &sn);

        tslc[i] = CoinMax(0.0,rowsol_[i]);
        tsuc[i] = CoinMax(0.0,-rowsol_[i]);

        checkMSKerror(err,"MSK_putsolutioni","setRowPrice");

      }

      err = MSK_getsolution(getMutableLpPtr(),
                            MSK_SOL_BAS,
                            NULL,
                            NULL,
                            NULL,
                            tskx,
                            NULL,
                            NULL,
                            txx,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            NULL);

      checkMSKerror(err,"MSK_getsolution","setRowPrice");


      err = MSK_putsolution(getMutableLpPtr(),
                             MSK_SOL_BAS,
                             tskc,
                             tskx,
                             NULL,
                             txc,
                             txx,
                             rowsol_, 
                             tslc, 
                             tsuc, 
                             tslx,
                             tsux,
                             NULL);

        checkMSKerror(err,"MSK_putsolution","setRowPrice");
    }
    else
    {
      for( int i = 0; i < nr; ++i )
      {    
        tslc[i] = CoinMax(0.0,rowsol_[i]);
        tsuc[i] = CoinMax(0.0,-rowsol_[i]);
        tskc[i] = MSK_SK_UNK;
      }

      for( int i = 0; i < nc; ++i )
      {    
        tskx[i] = MSK_SK_UNK;
        txx[i]  = 0.0;
      }
      
      err = MSK_putsolution(getMutableLpPtr(),
                             MSK_SOL_BAS,
                             tskc,
                             tskx,
                             NULL,
                             NULL,
                             NULL,
                             rowsol_, 
                             tslc, 
                             tsuc, 
                             tslx,
                             tsux,
                             NULL);
    
      checkMSKerror(err,"MSK_putsolution","setRowPrice");
    }

    for( int i = 0; i < nc; ++i )
    {
      colsol_[i]  = txx[i];
    }

    delete [] tskc;
    delete [] tslc;
    delete [] tsuc;
    delete [] txc;

    delete [] tskx;
    delete [] tslx;
    delete [] tsux;
    delete [] txx;
    delete [] redcost;
  }
  
  #if MSK_OSI_DEBUG_LEVEL > 1
  debugMessage("End OsiMskSolverInterface::setRowPrice(%p)\n", (void *)rs);
  #endif
}


//#############################################################################
// Problem modifying methods (matrix)
//#############################################################################

//-----------------------------------------------------------------------------
// Adds a column to the MOSEK task

void 
OsiMskSolverInterface::addCol(const CoinPackedVectorBase& vec,
                              const double collb, const double colub,   
                              const double obj)
{
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::addCol(%p, %g, %g, %g)\n", (void *)&vec, collb, colub, obj);
  #endif

  int nc = getNumCols();

  MSKassert(3,coltypesize_ >= nc,"coltypesize_ >= nc","addCol");

  resizeColType(nc + 1);
  coltype_[nc] = 'C';

  int ends = vec.getNumElements();
  MSKboundkeye tag;
  MSKtask_t task=getLpPtr(); 

  double inf = getInfinity();
  if(collb > -inf && colub >= inf)
    tag = MSK_BK_LO;
  else if(collb <= -inf && colub < inf)
    tag = MSK_BK_UP;
  else if(collb > -inf && colub < inf)
    tag = MSK_BK_RA;
  else if(collb <= -inf && colub >= inf)
    tag = MSK_BK_FR;
  else
    throw CoinError("Bound error", "addCol", "OsiMSKSolverInterface");

#if MSK_VERSION_MAJOR >= 7
  int       err;
  MSKint32t j;

  err = MSK_getnumvar(task,&j);
   
  if ( err==MSK_RES_OK )
    err = MSK_appendvars(task,1);
 
  if ( err==MSK_RES_OK )
    err = MSK_putcj(task,j,obj);

  if ( err==MSK_RES_OK )
    err = MSK_putacol(task,j,ends,const_cast<int*>(vec.getIndices()),const_cast<double*>(vec.getElements()));

  if ( err==MSK_RES_OK )
    err = MSK_putvarbound(task,j,tag,collb,colub);

#else
  int start = 0;
  int err = MSK_appendvars(task,
                           1,
                           const_cast<double*> (&obj),
                           &start,
                           &ends,
                           const_cast<int*>(vec.getIndices()),
                           const_cast<double*>(vec.getElements()),
                           (&tag),
                           const_cast<double*> (&collb),
                           const_cast<double*> (&colub));

#endif

  checkMSKerror( err, "MSK_appendvars", "addCol" );

  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("End OsiMskSolverInterface::addCol(%p, %g, %g, %g)\n", (void *)&vec, collb, colub, obj);
  #endif
}

//-----------------------------------------------------------------------------
// Adds a list of columns to the MOSEK task

void 
OsiMskSolverInterface::addCols(const int numcols,
                               const CoinPackedVectorBase * const * cols,
                               const double* collb, const double* colub,   
                               const double* obj)
{
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::addCols(%d, %p, %p, %p, %p)\n", numcols, (void *)cols, (void *)collb, (void *)colub, (void *)obj);
  #endif

  int i, nz = 0, err = MSK_RES_OK;
  
  // For efficiency we put hints on the total future size
  err = MSK_getmaxnumanz(getLpPtr(),
                         &nz);
                     
  checkMSKerror( err, "MSK_getmaxanz", "addCols" );

  for( i = 0; i < numcols; ++i)
    nz += cols[i]->getNumElements();
  
  err = MSK_putmaxnumanz(getLpPtr(),
                         nz);
                     
  checkMSKerror( err, "MSK_putmaxanz", "addCols" );
          
  err = MSK_putmaxnumvar(getLpPtr(),
                         numcols+getNumCols());
                     
  checkMSKerror( err, "MSK_putmaxnumvar", "addCols" );

  for( i = 0; i < numcols; ++i )
    addCol( *(cols[i]), collb[i], colub[i], obj[i] );

  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("End OsiMskSolverInterface::addCols(%d, %p, %p, %p, %p)\n", numcols, (void *)cols, (void *)collb, (void *)colub, (void *)obj);
  #endif
}

//-----------------------------------------------------------------------------
// Deletes a list of columns from the MOSEK task 

void 
OsiMskSolverInterface::deleteCols(const int num, const int * columnIndices)
{
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::deleteCols(%d, %p)\n", num, (void *)columnIndices);
  #endif

#if MSK_VERSION_MAJOR >= 7
  int err;
  err = MSK_removevars(getLpPtr( OsiMskSolverInterface::KEEPCACHED_ROW ),
                       num,
                       const_cast<int*>(columnIndices));

  checkMSKerror( err, "MSK_removevars", "deleteCols" );

#else
  int err;
  err = MSK_remove(getLpPtr( OsiMskSolverInterface::KEEPCACHED_ROW ),
                   MSK_ACC_VAR,
                   num,
                   const_cast<int*>(columnIndices));

  checkMSKerror( err, "MSK_remove", "deleteCols" );
#endif

  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("End OsiMskSolverInterface::deleteCols(%d, %p)\n", num, (void *)columnIndices);
  #endif
}

//-----------------------------------------------------------------------------
// Adds a row in bound form to the MOSEK task

void 
OsiMskSolverInterface::addRow(const CoinPackedVectorBase& vec,
                                const double rowlb, const double rowub)
{
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::addRow(%p, %g, %g)\n", (void *)&vec, rowlb, rowub);
  #endif

  getNumRows();

  int          ends = vec.getNumElements();
  double       inf = getInfinity();
  MSKboundkeye tag;
  MSKtask_t    task = getLpPtr( OsiMskSolverInterface::KEEPCACHED_COLUMN );
  
  if(rowlb > -inf && rowub >= inf)
    tag = MSK_BK_LO;
  else if(rowlb <= -inf && rowub < inf)
    tag = MSK_BK_UP;
  else if(rowlb > -inf && rowub < inf)
    tag = MSK_BK_RA;
  else if(rowlb <= -inf && rowub >= inf)
    tag = MSK_BK_FR;
  else
    throw CoinError("Bound error", "addRow", "OsiMSKSolverInterface");

#if MSK_VERSION_MAJOR >= 7
  int       err;
  MSKint32t i;

  err = MSK_getnumcon(task,&i);

  if ( err==MSK_RES_OK ) 
    err = MSK_appendcons(task,1);

  if ( err==MSK_RES_OK )
    err = MSK_putconbound(task,i,tag,rowlb,rowub);

  if ( err==MSK_RES_OK ) 
    err = MSK_putarow(task,i,ends,
                      const_cast<int*>(vec.getIndices()),
                      const_cast<double*>(vec.getElements()));

#else
  int start = 0;
  int err = MSK_appendcons(task,
                           1,
                           &start,
                           &ends,
                           const_cast<int*>(vec.getIndices()),
                           const_cast<double*>(vec.getElements()),
                           (&tag),
                           const_cast<double*> (&rowlb),
                           const_cast<double*> (&rowub));
#endif

  checkMSKerror( err, "MSK_appendcons", "addRow" );

  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("End OsiMskSolverInterface::addRow(%p, %g, %g)\n", (void *)&vec, rowlb, rowub);
  #endif
}

//-----------------------------------------------------------------------------
// Adds a row in triplet form to the MOSEK task

void 
OsiMskSolverInterface::addRow(const CoinPackedVectorBase& vec,
                               const char rowsen, const double rowrhs,   
                               const double rowrng)
{
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::addRow(%p, %c, %g, %g)\n", (void *)&vec, rowsen, rowrhs, rowrng);
  #endif

  int rtag=MSK_BK_FR;
  double lb=-MSK_INFINITY,ub=MSK_INFINITY;
  MskConvertSenseToBound( rowsen, rowrng, rowrhs, lb, ub, rtag );
  addRow(vec, lb, ub);

  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("End OsiMskSolverInterface::addRow(%p, %c, %g, %g)\n", (void *)&vec, rowsen, rowrhs, rowrng);
  #endif
}

//-----------------------------------------------------------------------------
// Adds a serie of rows in bound form to the MOSEK task

void 
OsiMskSolverInterface::addRows(const int numrows,
                                 const CoinPackedVectorBase * const * rows,
                                 const double* rowlb, const double* rowub)
{
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::addRows(%d, %p, %p, %p)\n", numrows, (void *)rows, (void *)rowlb, (void *)rowub);
  #endif

  int i,nz = 0, err = MSK_RES_OK;
  
  // For efficiency we put hints on the total future size
  err = MSK_getmaxnumanz(
                     getLpPtr(),
                     &nz);
                     
  checkMSKerror( err, "MSK_getmaxanz", "addRows" );
  
  
  for( i = 0; i < numrows; ++i)
    nz += rows[i]->getNumElements();
  
  err = MSK_putmaxnumanz(getLpPtr(),
                         nz);
                     
  checkMSKerror( err, "MSK_putmaxanz", "addRows" );
          
  err = MSK_putmaxnumcon(getLpPtr(),
                        numrows+getNumRows());
                    
  checkMSKerror( err, "MSK_putmaxnumcon", "addRows" );

  for( i = 0; i < numrows; ++i )
    addRow( *(rows[i]), rowlb[i], rowub[i] );

  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("End OsiMskSolverInterface::addRows(%d, %p, %p, %p)\n", numrows, (void *)rows, (void *)rowlb, (void *)rowub);
  #endif
}

//-----------------------------------------------------------------------------
// Adds a list of rows in triplet form to the MOSEK task

void 
OsiMskSolverInterface::addRows(const int numrows,
                                 const CoinPackedVectorBase * const * rows,
                                 const char* rowsen, const double* rowrhs,   
                                 const double* rowrng)
{
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::addRows(%d, %p, %p, %p, %p)\n", numrows, (void *)rows, (void *)rowsen, (void *)rowrhs, (void *)rowrng);
  #endif

  int i, err = MSK_RES_OK, nz = 0;
  
    // For efficiency we put hints on the total future size
  for( i = 0; i < numrows; ++i)
    nz += rows[i]->getNumElements();
  
  err = MSK_putmaxnumanz(
                     getLpPtr(),
                     nz);
                     
  checkMSKerror( err, "MSK_putmaxanz", "addRows" );
          
  err = MSK_putmaxnumcon(
                     getLpPtr(),
                     numrows);
                     
  checkMSKerror( err, "MSK_putmaxnumcon", "addRows" );

  for( i = 0; i < numrows; ++i )
    addRow( *(rows[i]), rowsen[i], rowrhs[i], rowrng[i] );

  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("End OsiMskSolverInterface::addRows(%d, %p, %p, %p, %p)\n", numrows, (void *)rows, (void *)rowsen, (void *)rowrhs, (void *)rowrng);
  #endif
}

//-----------------------------------------------------------------------------
// Deletes a list of rows the MOSEK task 

void 
OsiMskSolverInterface::deleteRows(const int num, const int * rowIndices)
{
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::deleteRows(%d, %p)\n", num, (void *)rowIndices);
  #endif

  int err;
#if MSK_VERSION_MAJOR >= 7
  err = MSK_removecons(getLpPtr( OsiMskSolverInterface::KEEPCACHED_COLUMN ),
                       num,
                       const_cast<int*>(rowIndices));

  checkMSKerror( err, "MSK_removecons", "deleteRows" );

#else
  err = MSK_remove(getLpPtr( OsiMskSolverInterface::KEEPCACHED_COLUMN ),
                   MSK_ACC_CON,
                   num,
                   const_cast<int*>(rowIndices));

  checkMSKerror( err, "MSK_remove", "deleteRows" );
#endif

  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("End OsiMskSolverInterface::deleteRows(%d, %p)\n", num, (void *)rowIndices);
  #endif
}

//#############################################################################
// Methods to input a problem
//#############################################################################

//-----------------------------------------------------------------------------
// Loads a problem. Should have its "own" implementation so we don't have to convert
// to triplet, since this is convertet back in the load function called. But
// for simplicity, this is not done. 

void
OsiMskSolverInterface::loadProblem(const CoinPackedMatrix& matrix,
                                    const double* collb, 
                                    const double* colub,
                                    const double* obj,
                                    const double* rowlb, 
                                    const double* rowub )
{
  #if MSK_OSI_DEBUG_LEVEL > 1
  debugMessage("Begin OsiMskSolverInterface::loadProblem(%p, %p, %p, %p, %p, %p)\n", (void *)&matrix, (void *)collb, (void *)colub, (void *)obj, (void *)rowlb, (void *)rowub);
  #endif

  const double inf = getInfinity();
  
  int nrows = matrix.getNumRows();

  char   * rowSense;
  double * rowRhs;
  double * rowRange;

  if( nrows )
  {
    rowSense = new char  [nrows];
    rowRhs   = new double[nrows];
    rowRange = new double[nrows];
  }
  else
  {
    rowSense = NULL;
    rowRhs   = NULL;
    rowRange = NULL;
  }

  int i;
  if( rowlb == NULL && rowub == NULL)
      for ( i = nrows - 1; i >= 0; --i )
          convertBoundToSense( -inf, inf, rowSense[i], rowRhs[i], rowRange[i] );
  else if( rowlb == NULL)
      for ( i = nrows - 1; i >= 0; --i )
          convertBoundToSense( -inf, rowub[i], rowSense[i], rowRhs[i], rowRange[i] );
  else if( rowub == NULL)
      for ( i = nrows - 1; i >= 0; --i )
          convertBoundToSense( rowlb[i], inf, rowSense[i], rowRhs[i], rowRange[i] );
  else
      for ( i = nrows - 1; i >= 0; --i )
          convertBoundToSense( rowlb[i], rowub[i], rowSense[i], rowRhs[i], rowRange[i] );

  loadProblem( matrix, collb, colub, obj, rowSense, rowRhs, rowRange );

  if( nrows )
  {
    delete [] rowSense;
    delete [] rowRhs;
    delete [] rowRange;
  }

  #if MSK_OSI_DEBUG_LEVEL > 1
  debugMessage("End OsiMskSolverInterface::loadProblem(%p, %p, %p, %p, %p, %p)\n", (void *)&matrix, (void *)collb, (void *)colub, (void *)obj, (void *)rowlb, (void *)rowub);
  #endif
}

//-----------------------------------------------------------------------------
// Loads a problem 

void
OsiMskSolverInterface::assignProblem( CoinPackedMatrix*& matrix,
                                      double*& collb, 
                                      double*& colub,
                                      double*& obj,
                                      double*& rowlb, 
                                      double*& rowub )
{
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::assignProblem()\n");
  #endif

  loadProblem( *matrix, collb, colub, obj, rowlb, rowub );
  
  delete matrix;   matrix = 0;
  delete[] collb;  collb  = 0;
  delete[] colub;  colub  = 0;
  delete[] obj;    obj    = 0;
  delete[] rowlb;  rowlb  = 0;
  delete[] rowub;  rowub  = 0;

  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("End OsiMskSolverInterface::assignProblem()\n");
  #endif
}

//-----------------------------------------------------------------------------
// Loads a problem 

void
OsiMskSolverInterface::loadProblem( const CoinPackedMatrix& matrix,
                                    const double* collb, 
                                    const double* colub,
                                    const double* obj,
                                    const char* rowsen, 
                                    const double* rowrhs,
                                    const double* rowrng )
{     
  int nc=matrix.getNumCols();
  int nr=matrix.getNumRows();

  #if MSK_OSI_DEBUG_LEVEL > 1
  debugMessage("Begin OsiMskSolverInterface::loadProblem(%p, %p, %p, %p, %p, %p, %p) numcols : %d numrows : %d\n",
     (void *)&matrix, (void *)collb, (void *)colub, (void *)obj, (void *)rowsen, (void *)rowrhs, (void *)rowrng,nc,nr);
  #endif

  if( nr == 0 && nc == 0 )
    gutsOfDestructor();
  else
  {    
    /* Warning input  pointers can be NULL */
    
    int i,j;
      
    double    * ob;
    int       * rtag  = NULL;
    double    * rlb   = NULL;
    double    * rub   = NULL;

    int       * ctag  = NULL;
    int       * cends = NULL;
    const int *len;
    const int *start;
    double    * clb   = NULL;
    double    * cub   = NULL;

    if( obj != NULL )
      ob=const_cast<double*>(obj);
    else
    {
      ob = new double[nc];
      CoinFillN(ob, nc, 0.0);
    }

    if( nr )
    {
      rtag = new int[nr];
      rlb  = new double[nr];
      rub  = new double[nr];
    }

    if( rowsen && rowrng && rowrhs )
    {
      for( i=0; i < nr; i++ )
        MskConvertSenseToBound( rowsen[i], rowrng[i], rowrhs[i], rlb[i], rub[i], rtag[i]);
    }
    else
    {
      for( i=0; i < nr; i++ )
      {
        rlb[i]  = 0.0;
        rub[i]  = MSK_INFINITY;
        rtag[i] = MSK_BK_LO; 
      }
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

    MSKassert(3,nc == m->getNumCols(),"nc == m->getNumCols()","loadProblem");
    MSKassert(3,nr == m->getNumRows(),"nr == m->getNumRows()","loadProblem");
    MSKassert(3,m->isColOrdered(),"m->isColOrdered()","loadProblem");

    double inf =getInfinity();

    if( nc )
    {
      ctag       = new int[nc];
      cends      = new int[nc];
      clb        = new double[nc];
      cub        = new double[nc];
    }

    len        = (m->getVectorLengths());
    start      = (m->getVectorStarts());

    if( collb == NULL && colub == NULL )
    {
      for(j=0; j < nc; j++)
      {
         cends[j] = start[j] + len[j];
         MskConvertColBoundToTag(0, inf, clb[j], cub[j], ctag[j]);
      }
    }
    else if( collb == NULL )
    {
      for(j=0; j < nc; j++)
      {
         cends[j] = start[j] + len[j];
         MskConvertColBoundToTag( 0, colub[j], clb[j], cub[j], ctag[j]);
      }
    }
    else if( colub == NULL )
    {
      for(j=0; j < nc; j++)
      {
         cends[j] = start[j] + len[j];
         MskConvertColBoundToTag(collb[j], inf, clb[j], cub[j], ctag[j]);
      }
    }
    else
    {
      for(j=0; j < nc; j++)
      {
         cends[j] = start[j] + len[j];
         MskConvertColBoundToTag( collb[j], colub[j], clb[j], cub[j], ctag[j]);
      }
    }

    int err=MSK_inputdata(getLpPtr( OsiMskSolverInterface::KEEPCACHED_NONE ),
                          nr,
                          nc,
                          nr,
                          nc,
                          ob, 
                          0.0,
                          const_cast<int *>(m->getVectorStarts()),
                          cends,
                          const_cast<int *>(m->getIndices()),
                          const_cast<double *>(m->getElements()), 
                          (MSKboundkeye*)rtag, 
                          rlb, 
                          rub, 
                          (MSKboundkeye*)ctag, 
                          clb, 
                          cub); 

    checkMSKerror( err, "MSK_inputdata", "loadProblem" );
              
    if( obj   == NULL )
      delete[] ob;

    if( nr )
    {
      delete[] rtag;
      delete[] rlb;
      delete[] rub;
    }

    if( nc )
    {
      delete[] ctag;
      delete[] clb;
      delete[] cub;
      delete[] cends;
    }

    if ( freeMatrixRequired )
      delete m;

    resizeColType(nc);
    CoinFillN(coltype_, nc, 'C');
  }

  #if MSK_OSI_DEBUG_LEVEL > 1
  debugMessage("End OsiMskSolverInterface::loadProblem(%p, %p, %p, %p, %p, %p, %p)\n",
   (void *)&matrix, (void *)collb, (void *)colub, (void *)obj, (void *)rowsen, (void *)rowrhs, (void *)rowrng);
  #endif
}
   
//-----------------------------------------------------------------------------
// Assigns a problem 

void
OsiMskSolverInterface::assignProblem( CoinPackedMatrix*& matrix,
                                       double*& collb, double*& colub,
                                       double*& obj,
                                       char*& rowsen, double*& rowrhs,
                                       double*& rowrng )
{
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::assignProblem()\n");
  #endif

  loadProblem( *matrix, collb, colub, obj, rowsen, rowrhs, rowrng );
  delete matrix;   matrix = 0;
  delete[] collb;  collb = 0;
  delete[] colub;  colub = 0;
  delete[] obj;    obj = 0;
  delete[] rowsen; rowsen = 0;
  delete[] rowrhs; rowrhs = 0;
  delete[] rowrng; rowrng = 0;

  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("End OsiMskSolverInterface::assignProblem()\n");
  #endif
}

//-----------------------------------------------------------------------------
// Loads a problem 

void
OsiMskSolverInterface::loadProblem(const int numcols, 
                                    const int numrows,
                                    const int* start, 
                                    const int* index,
                                    const double* value,
                                    const double* collb, 
                                    const double* colub,   
                                    const double* obj,
                                    const double* rowlb, 
                                    const double* rowub )
{
  #if MSK_OSI_DEBUG_LEVEL > 1
  debugMessage("Begin OsiMskSolverInterface::loadProblem() numcols : %d numrows : %d\n",numcols,numrows);
  #endif

  const double inf = getInfinity();
  
  char   * rowSense = new char  [numrows];
  double * rowRhs   = new double[numrows];
  double * rowRange = new double[numrows];
  
  for ( int i = numrows - 1; i >= 0; --i ) 
  {
    const double lower = rowlb ? rowlb[i] : -inf;
    const double upper = rowub ? rowub[i] : inf;
    convertBoundToSense( lower, upper, rowSense[i], rowRhs[i], rowRange[i] );
  }

  loadProblem(numcols, numrows, start, index, value, collb, colub, obj,
              rowSense, rowRhs, rowRange);

  delete [] rowSense;
  delete [] rowRhs;
  delete [] rowRange;

  #if MSK_OSI_DEBUG_LEVEL > 1
  debugMessage("End OsiMskSolverInterface::loadProblem()\n");
  #endif
}

//-----------------------------------------------------------------------------
// Loads a problem 

void
OsiMskSolverInterface::loadProblem(const int numcols, 
                                    const int numrows,
                                    const int* start, 
                                    const int* index,
                                    const double* value,
                                    const double* collb, 
                                    const double* colub,   
                                    const double* obj,
                                    const char* rowsen, 
                                    const double* rowrhs,
                                    const double* rowrng )
{
  #if MSK_OSI_DEBUG_LEVEL > 1
  debugMessage("Begin OsiMskSolverInterface::loadProblem(%d, %d, %p, %p, %p, %p, %p, %p, %p, %p, %p)\n",
     numcols, numrows, (void *)start, (void *)index, (void *)value, (void *)collb, (void *)colub, (void *)obj, (void *)rowsen,(void *)rowrhs, (void *)rowrng);
  #endif

  const int nc = numcols;
  const int nr = numrows;

  if( nr == 0 && nc == 0 )
    gutsOfDestructor();
  else
  {
    MSKassert(3,rowsen != NULL,"rowsen != NULL","loadProblem");
    MSKassert(3,rowrhs != NULL,"rowrhs != NULL","loadProblem");

    int i,j;
      
    double   * ob;
    int      * rtag =NULL;
    double   * rlb = NULL;
    double   * rub = NULL;
    int      * ctag =NULL;
    int      * cends = NULL;
    double   * clb = NULL;
    double   * cub = NULL;

    if( obj != NULL )
      ob=const_cast<double*>(obj);
    else
    {
      ob = new double[nc];
      CoinFillN(ob, nc, 0.0);
    }

    if( nr )
    {
      rtag = new int[nr];
      rlb  = new double[nr];
      rub  = new double[nr];
    }

    for( i=0; i < nr; i++ )
      MskConvertSenseToBound( rowsen[i], rowrng != NULL ? rowrng[i] : 0.0, rowrhs[i], rlb[i], rub[i], rtag[i]);

    double inf = getInfinity();

    if( nc )
    {
      ctag       = new int[nc];
      cends      = new int[nc];
      clb        = new double[nc];
      cub        = new double[nc];
    }

    if( collb == NULL && colub == NULL )
        for(j=0; j < nc; j++)
        {
           cends[j] = start[j+1];
           MskConvertColBoundToTag(0, inf, clb[j], cub[j], ctag[j]);
        }
    else if( collb == NULL )
        for(j=0; j < nc; j++)
        {
           cends[j] = start[j+1];
           MskConvertColBoundToTag( 0, colub[j], clb[j], cub[j], ctag[j]);
        }
    else if( colub == NULL )
        for(j=0; j < nc; j++)
        {
           cends[j] = start[j+1];
           MskConvertColBoundToTag(collb[j], inf, clb[j], cub[j], ctag[j]);
        }
    else
        for(j=0; j < nc; j++)
        {
           cends[j] = start[j+1];
           MskConvertColBoundToTag( collb[j], colub[j], clb[j], cub[j], ctag[j]);
        }

    int err=MSK_inputdata(getLpPtr( OsiMskSolverInterface::KEEPCACHED_NONE ),
                          nr,
                          nc,
                          nr,
                          nc,
                          ob, 
                          0.0,
                          const_cast<int *>(start),
                          cends,
                          const_cast<int *>(index),
                          const_cast<double *>(value), 
                          (MSKboundkeye*)rtag, 
                          rlb, 
                          rub, 
                          (MSKboundkeye*)ctag, 
                          clb, 
                          cub);

    checkMSKerror( err, "MSK_inputdata", "loadProblem3" );
              
    if( obj   == NULL )
      delete[] ob;

    if( nr )
    {
      delete[] rtag;
      delete[] rlb;
      delete[] rub;
    }

    if( nc )
    {
      delete[] ctag;
      delete[] cends;
      delete[] clb;
      delete[] cub;
    }

    resizeColType(nc);
    CoinFillN(coltype_, nc, 'C');
  }
  
  #if MSK_OSI_DEBUG_LEVEL > 1
  debugMessage("End OsiMskSolverInterface::loadProblem(%d, %d, %p, %p, %p, %p, %p, %p, %p, %p, %p)\n",
     numcols, numrows, (void *)start, (void *)index, (void *)value, (void *)collb, (void *)colub, (void *)obj, (void *)rowsen, (void *)rowrhs, (void *)rowrng);
  #endif
}
 
//-----------------------------------------------------------------------------
// Reads a MPS file with Coin native MPS reader. If marked code is switch on
// then MOSEK file reader is used, and .gz files can be read aswell.

int OsiMskSolverInterface::readMps(const char * filename,
                                    const char * extension )
{
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::readMps(%s, %s) %p\n", filename, extension,(void*)this);
  debugMessage("End OsiMskSolverInterface::readMps(%s, %s)\n", filename, extension);
  #endif

  return OsiSolverInterface::readMps(filename,extension);
}

//-----------------------------------------------------------------------------
// Writes the problem in MPS format, uses MOSEK writer.

void OsiMskSolverInterface::writeMps( const char * filename,
                                       const char * extension,
                                       double objSense ) const
{
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::writeMps(%s, %s, %g)\n", filename, extension, objSense);
  #endif

  std::string f(filename);
  std::string e(extension);  
  std::string fullname = f + "." + e;

  OsiSolverInterface::writeMpsNative(fullname.c_str(),NULL, NULL, 0, 2, objSense); 

  #if 0
  int err = MSK_writedata( getMutableLpPtr(), const_cast<char*>( fullname.c_str() ));
  checkMSKerror( err, "MSK_writedatafile", "writeMps" );
  #endif
  
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("End OsiMskSolverInterface::writeMps(%s, %s, %g)\n", filename, extension, objSense);
  #endif
}

void OsiMskSolverInterface::passInMessageHandler(CoinMessageHandler * handler) {
	OsiSolverInterface::passInMessageHandler(handler);
	
  MSK_linkfunctotaskstream(getMutableLpPtr(), MSK_STREAM_LOG, messageHandler(), OsiMskStreamFuncLog);
  MSK_linkfunctotaskstream(getMutableLpPtr(), MSK_STREAM_ERR, messageHandler(), OsiMskStreamFuncWarning);
  MSK_linkfunctotaskstream(getMutableLpPtr(), MSK_STREAM_WRN, messageHandler(), OsiMskStreamFuncError);
}

//#############################################################################
// MSK specific public interfaces
//#############################################################################

//-----------------------------------------------------------------------------
// Returns MOSEK task in the interface object

MSKenv_t OsiMskSolverInterface::getEnvironmentPtr()
{
  MSKassert(3,env_ != NULL,"probtypemip_","getEnvironmentPtr");

  return env_;
}

//-----------------------------------------------------------------------------
// Returns MOSEK task in the interface object

MSKtask_t OsiMskSolverInterface::getLpPtr( int keepCached )
{
  freeCachedData( keepCached );
  return getMutableLpPtr();
}

//-----------------------------------------------------------------------------
// Returns the coltype_ array

const char * OsiMskSolverInterface::getCtype() const
{
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::getCtype()\n");
  debugMessage("End OsiMskSolverInterface::getCtype()\n");
  #endif
  
  return coltype_;
}

//#############################################################################
// Static instance counter methods
//#############################################################################

//-----------------------------------------------------------------------------
// Increment the instance count, so we know when to close and open MOSEK.

void OsiMskSolverInterface::incrementInstanceCounter()
{
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::incrementInstanceCounter()\n");
  #endif

  if ( numInstances_ == 0 )
  {
    int err=0;

#if MSK_OSI_DEBUG_LEVEL > 1
    debugMessage("creating new Mosek environment\n");
#endif

#if MSK_VERSION_MAJOR >= 7
    err = MSK_makeenv(&env_,NULL);
#else
    err = MSK_makeenv(&env_,NULL,NULL,NULL,NULL);
#endif
    checkMSKerror( err, "MSK_makeenv", "incrementInstanceCounter" );

    err = MSK_linkfunctoenvstream(env_, MSK_STREAM_LOG, NULL, printlog); 

    checkMSKerror( err, "MSK_linkfunctoenvstream", "incrementInstanceCounter" );

    err = MSK_initenv(env_);
    checkMSKerror( err, "MSK_initenv", "incrementInstanceCounter" );
  }

  numInstances_++;
  
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("End OsiMskSolverInterface::incrementInstanceCounter()\n");
  #endif
}

//-----------------------------------------------------------------------------
// Decrement the instance count, so we know when to close and open MOSEK.

void OsiMskSolverInterface::decrementInstanceCounter()
{
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::decrementInstanceCounter()\n");
  #endif

  MSKassert(3,numInstances_ != 0,"numInstances_ != 0","decrementInstanceCounter");

  numInstances_--;
  if ( numInstances_ == 0 )
  {
#if MSK_OSI_DEBUG_LEVEL > 1
     debugMessage("deleting Mosek environment\n");
#endif

     int err = MSK_deleteenv(&env_);
     checkMSKerror( err, "MSK_deleteenv", "decrementInstanceCounter" );
     env_ = NULL;
  }
  
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("End OsiMskSolverInterface::decrementInstanceCounter()\n");
  #endif
}

//-----------------------------------------------------------------------------
// Returns the number of OsiMskSolverInterface objects in play

unsigned int OsiMskSolverInterface::getNumInstances()
{
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::getNumInstances()\n");
  debugMessage("End OsiMskSolverInterface::getNumInstances()\n");
  #endif
  
  return numInstances_;
}

//#############################################################################
// Constructors, destructors clone and assignment
//#############################################################################

//-----------------------------------------------------------------------------
// Constructor

OsiMskSolverInterface::OsiMskSolverInterface(MSKenv_t mskenv)
  : OsiSolverInterface(),
    Mskerr(MSK_RES_OK),
    ObjOffset_(0.0),
    InitialSolver(INITIAL_SOLVE),
    task_(NULL),
    hotStartCStat_(NULL),
    hotStartCStatSize_(0),
    hotStartRStat_(NULL),
    hotStartRStatSize_(0),
    hotStartMaxIteration_(1000000), 
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
    coltype_(NULL),
    coltypesize_(0),
    probtypemip_(false)
{
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::OsiMskSolverinterface()\n");
  #endif

  if (mskenv) {
  	if (env_) {
  		throw CoinError("Already have a global Mosek environment. Cannot use second one.", "OsiMskSolverInterface", "OsiMskSolverInterface");
  	}
  	env_ = mskenv;
  	++numInstances_;
  } else
  	incrementInstanceCounter();
  
  gutsOfConstructor();
  
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("End OsiMskSolverInterface::OsiMskSolverinterface()\n");
  #endif
}



//-----------------------------------------------------------------------------
// Clone from one to another object

OsiSolverInterface * OsiMskSolverInterface::clone(bool copyData) const
{
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::clone(%d)\n", copyData);
  debugMessage("End OsiMskSolverInterface::clone(%d)\n", copyData);
  #endif
  
  return( new OsiMskSolverInterface( *this ) );
}


//-----------------------------------------------------------------------------
// Copy constructor

OsiMskSolverInterface::OsiMskSolverInterface( const OsiMskSolverInterface & source )
  : OsiSolverInterface(source),
    Mskerr(MSK_RES_OK),
    MSKsolverused_(INITIAL_SOLVE),
    ObjOffset_(source.ObjOffset_),
    InitialSolver(INITIAL_SOLVE),
    task_(NULL),
    hotStartCStat_(NULL),
    hotStartCStatSize_(0),
    hotStartRStat_(NULL),
    hotStartRStatSize_(0),
    hotStartMaxIteration_(source.hotStartMaxIteration_),
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
    coltype_(NULL),
    coltypesize_(0),
    probtypemip_(false)
{
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::OsiMskSolverInterface from (%p) to %p\n", (void *)&source,(void*)this);
  #endif

  incrementInstanceCounter();  
  gutsOfConstructor();
  gutsOfCopy( source );
  
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("End OsiMskSolverInterface::OsiMskSolverInterface(%p)\n", (void *)&source);
  #endif
}

//-----------------------------------------------------------------------------
// Destructor

OsiMskSolverInterface::~OsiMskSolverInterface()
{
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::~OsiMskSolverInterface()\n");
  #endif

  gutsOfDestructor();
  decrementInstanceCounter();

  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("End OsiMskSolverInterface::~OsiMskSolverInterface()\n");
  #endif
}

//-----------------------------------------------------------------------------
// Assign operator

OsiMskSolverInterface& OsiMskSolverInterface::operator=( const OsiMskSolverInterface& rhs )
{
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::operator=(%p)\n", (void *)&rhs);
  #endif

  if (this != &rhs)
  {    
    OsiSolverInterface::operator=( rhs );
    gutsOfDestructor();
    gutsOfConstructor();

    if ( rhs.task_ !=NULL )
      gutsOfCopy( rhs );
  }

  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("End OsiMskSolverInterface::operator=(%p)\n", (void *)&rhs);
  #endif

  return *this;
}

//#############################################################################
// Applying cuts
//#############################################################################

//-----------------------------------------------------------------------------
// Apply col cut

void OsiMskSolverInterface::applyColCut( const OsiColCut & cc )
{
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::applyColCut(%p)\n", (void *)&cc);
  #endif

  double * MskColLB = new double[getNumCols()];
  double * MskColUB = new double[getNumCols()];
  const CoinPackedVector & lbs = cc.lbs();
  const CoinPackedVector & ubs = cc.ubs();
  int i;
  
  for( i = 0; i < getNumCols(); ++i )
  {
    MskColLB[i] = getColLower()[i];
    MskColUB[i] = getColUpper()[i];
  }
    
  for( i = 0; i < lbs.getNumElements(); ++i ) 
    if ( lbs.getElements()[i] > MskColLB[lbs.getIndices()[i]] )
      setColLower( lbs.getIndices()[i], lbs.getElements()[i] );
  for( i = 0; i < ubs.getNumElements(); ++i )
    if ( ubs.getElements()[i] < MskColUB[ubs.getIndices()[i]] )
      setColUpper( ubs.getIndices()[i], ubs.getElements()[i] );
  
  delete[] MskColLB;
  delete[] MskColUB;
  
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("End OsiMskSolverInterface::applyColCut(%p)\n", (void *)&cc);
  #endif
}

//-----------------------------------------------------------------------------
// Apply row cut

void OsiMskSolverInterface::applyRowCut( const OsiRowCut & rowCut )
{
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::applyRowCut(%p)\n", (void *)&rowCut);
  #endif

  const CoinPackedVector & row=rowCut.row();
  addRow(row ,  rowCut.lb(),rowCut.ub());

  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("End OsiMskSolverInterface::applyRowCut(%p)\n", (void *)&rowCut);
  #endif
}

//#############################################################################
// Private methods (non-static and static) and static data
//#############################################################################   

unsigned int OsiMskSolverInterface::numInstances_ = 0;
MSKenv_t OsiMskSolverInterface::env_=NULL;
 
//-----------------------------------------------------------------------------
// Returns MOSEK task in object

MSKtask_t OsiMskSolverInterface::getMutableLpPtr() const
{
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::getMutableLpPtr()\n");
  #endif

  if ( task_ == NULL )
  {
    MSKassert(3,env_ != NULL,"env_ == NULL","getMutableLpPtr");

    int err = MSK_makeemptytask(env_,&task_);
    checkMSKerror(err, "MSK_makeemptytask","getMutableLpPtr");

    err = MSK_linkfunctotaskstream(task_, MSK_STREAM_LOG, messageHandler(), OsiMskStreamFuncLog);
    checkMSKerror( err, "MSK_linkfunctotaskstream", "getMutableLpPtr" );

    err = MSK_linkfunctotaskstream(task_, MSK_STREAM_WRN, messageHandler(), OsiMskStreamFuncWarning);
    checkMSKerror( err, "MSK_linkfunctotaskstream", "getMutableLpPtr" );

    err = MSK_linkfunctotaskstream(task_, MSK_STREAM_ERR, messageHandler(), OsiMskStreamFuncError);
    checkMSKerror( err, "MSK_linkfunctotaskstream", "getMutableLpPtr" );

    err = MSK_putintparam(task_, MSK_IPAR_WRITE_GENERIC_NAMES, MSK_ON);
    checkMSKerror(err,"MSK_putintparam","getMutableLpPtr()");  

    err = MSK_putintparam(task_, MSK_IPAR_PRESOLVE_USE, MSK_ON);
    checkMSKerror(err,"MSK_putintparam","getMutableLpPtr()");  
    
    err = MSK_putintparam(task_, MSK_IPAR_WRITE_DATA_FORMAT, MSK_DATA_FORMAT_MPS);
    checkMSKerror(err,"MSK_putintparam","getMutableLpPtr()");  
    
    err = MSK_putintparam(task_, MSK_IPAR_WRITE_GENERIC_NAMES, MSK_ON);
    checkMSKerror(err,"MSK_putintparam","getMutableLpPtr()");  

    #if MSK_DO_MOSEK_LOG > 0
    char file[] = "MOSEK.log";
    err = MSK_linkfiletotaskstream(task_, MSK_STREAM_LOG, file, 0);
    checkMSKerror( err, "MSK_linkfiletotaskstream", "getMutableLpPtr" );

    err = MSK_putintparam(task_, MSK_IPAR_LOG, 100);
    checkMSKerror(err,"MSK_putintparam","getMutableLpPtr()");  

    err = MSK_putintparam(task_, MSK_IPAR_LOG_SIM, 100);
    checkMSKerror(err,"MSK_putintparam","getMutableLpPtr()");  

    err = MSK_putintparam(task_, MSK_IPAR_LOG_INTPNT, 100);
    checkMSKerror(err,"MSK_putintparam","getMutableLpPtr()");  
    #else
    err = MSK_putintparam(task_, MSK_IPAR_LOG, 100);
    checkMSKerror(err,"MSK_putintparam","getMutableLpPtr()");
    #endif

    err =  MSK_putintparam(task_,MSK_IPAR_SIM_SOLVE_FORM,MSK_SOLVE_PRIMAL);
    checkMSKerror(err,"MSK_putintparam","getMutableLpPtr()");  

    err =  MSK_putintparam(task_,MSK_IPAR_SIM_HOTSTART,MSK_SIM_HOTSTART_STATUS_KEYS);
    checkMSKerror(err,"MSK_putintparam","getMutableLpPtr()");  
    
    std::string pn;
    getStrParam(OsiProbName,pn);
    MSK_puttaskname( task_, const_cast<char*>(pn.c_str()) );
    checkMSKerror(err,"MSK_puttaskname","getMutableLpPtr()");   
  }
  
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("End OsiMskSolverInterface::getMutableLpPtr()\n");
  #endif

  return task_;
}

//-----------------------------------------------------------------------------
// Makes a copy

void OsiMskSolverInterface::gutsOfCopy( const OsiMskSolverInterface & source )
{
  #if MSK_OSI_DEBUG_LEVEL > 1
  debugMessage("Begin OsiMskSolverInterface::gutsOfCopy()\n");
  #endif

  int err;

  InitialSolver = source.InitialSolver;

  MSKassert(3, task_ == NULL, "task_ == NULL", "gutsOfCopy");
  err = MSK_clonetask(source.getMutableLpPtr(),&task_);

  checkMSKerror( err, "MSK_clonetask", "gutsOfCopy" );

  // Set MIP information
  resizeColType(source.coltypesize_);
  CoinDisjointCopyN( source.coltype_, source.coltypesize_, coltype_ );

  // Updates task MIP information 
  for( int k = 0; k < source.coltypesize_; ++k )
  {
    switch(coltype_[k])
    {
      case 'I':
      setInteger(k);
      break;
      
      case 'C':
      setContinuous(k);
      break;
    }
  }

  #if MSK_OSI_DEBUG_LEVEL > 1
  debugMessage("End OsiMskSolverInterface::gutsOfCopy()\n");
  #endif
}


//-----------------------------------------------------------------------------
// Empty function

void OsiMskSolverInterface::gutsOfConstructor()
{ 
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::gutsOfConstructor()\n");
  #endif

  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("End OsiMskSolverInterface::gutsOfConstructor()\n");
  #endif
}


//-----------------------------------------------------------------------------
// Function called from destructor

void OsiMskSolverInterface::gutsOfDestructor()
{  
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::gutsOfDestructor()\n");
  #endif
  
  freeCachedData(KEEPCACHED_NONE);

  if ( task_ != NULL )
  {
  	MSK_unlinkfuncfromtaskstream(getMutableLpPtr(), MSK_STREAM_LOG);
  	MSK_unlinkfuncfromtaskstream(getMutableLpPtr(), MSK_STREAM_ERR);
  	MSK_unlinkfuncfromtaskstream(getMutableLpPtr(), MSK_STREAM_WRN);
  	
      int err = MSK_deletetask(&task_);
      checkMSKerror( err, "MSK_deletetask", "gutsOfDestructor" );
      task_ = NULL;
      freeAllMemory();
  }

  MSKassert(3,task_==NULL,"task_==NULL","gutsOfDestructor");
  MSKassert(3,obj_==NULL,"obj_==NULL","gutsOfDestructor");
  MSKassert(3,collower_==NULL,"collower_==NULL","gutsOfDestructor");
  MSKassert(3,colupper_==NULL,"colupper_==NULL","gutsOfDestructor");
  MSKassert(3,rowsense_==NULL,"rowsense_==NULL","gutsOfDestructor");
  MSKassert(3,rhs_==NULL,"rhs_==NULL","gutsOfDestructor");
  MSKassert(3,rowrange_==NULL,"rowrange_==NULL","gutsOfDestructor");
  MSKassert(3,rowlower_==NULL,"rowlower_==NULL","gutsOfDestructor");
  MSKassert(3,rowupper_==NULL,"rowupper_==NULL","gutsOfDestructor");
  MSKassert(3,colsol_==NULL,"colsol_==NULL","gutsOfDestructor");
  MSKassert(3,rowsol_==NULL,"rowsol_==NULL","gutsOfDestructor");
  MSKassert(3,redcost_==NULL,"redcost_==NULL","gutsOfDestructor");
  MSKassert(3,rowact_==NULL,"rowact_==NULL","gutsOfDestructor");
  MSKassert(3,matrixByRow_==NULL,"probtypemip_","gutsOfDestructor");
  MSKassert(3,matrixByCol_==NULL,"matrixByCol_==NULL","gutsOfDestructor");
  MSKassert(3,coltype_==NULL,"coltype_==NULL","gutsOfDestructor");
  MSKassert(3,coltypesize_==0,"coltypesize_==0","gutsOfDestructor");

  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("End OsiMskSolverInterface::gutsOfDestructor()\n");
  #endif
}


//-----------------------------------------------------------------------------
// Free function

void OsiMskSolverInterface::freeCachedColRim()
{
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::freeCachedColRim()\n");
  #endif

  freeCacheDouble( obj_ );  
  freeCacheDouble( collower_ ); 
  freeCacheDouble( colupper_ );
  
  MSKassert(3,obj_==NULL,"obj_==NULL","freeCachedColRim");
  MSKassert(3,collower_==NULL,"collower_==NULL","freeCachedColRim");
  MSKassert(3,colupper_==NULL,"colupper_==NULL","freeCachedColRim");

  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("End OsiMskSolverInterface::freeCachedColRim()\n");
  #endif
}

//-----------------------------------------------------------------------------
// Free function 

void OsiMskSolverInterface::freeCachedRowRim()
{
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::freeCachedRowRim()\n");
  #endif

  freeCacheChar( rowsense_ );
  freeCacheDouble( rhs_ );
  freeCacheDouble( rowrange_ );
  freeCacheDouble( rowlower_ );
  freeCacheDouble( rowupper_ );
  
  MSKassert(3,rowsense_==NULL,"rowsense_==NULL","freeCachedRowRim");
  MSKassert(3,rhs_==NULL,"rhs_==NULL","freeCachedRowRim");
  MSKassert(3,rowrange_==NULL,"rowrange_==NULL","freeCachedRowRim");
  MSKassert(3,rowlower_==NULL,"rowlower_==NULL","freeCachedRowRim");
  MSKassert(3,rowupper_==NULL,"rowupper_==NULL","freeCachedRowRim");

  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("End OsiMskSolverInterface::freeCachedRowRim()\n");
  #endif
}

//-----------------------------------------------------------------------------
// Free function

void OsiMskSolverInterface::freeCachedMatrix()
{
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::freeCachedMatrix()\n");
  #endif

  freeCacheMatrix( matrixByRow_ );
  freeCacheMatrix( matrixByCol_ );
  MSKassert(3,matrixByRow_==NULL,"matrixByRow_==NULL","freeCachedMatrix");
  MSKassert(3,matrixByCol_==NULL,"matrixByCol_==NULL","freeCachedMatrix");

  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("End OsiMskSolverInterface::freeCachedMatrix()\n");
  #endif    
}

//-----------------------------------------------------------------------------
//  Free function

void OsiMskSolverInterface::freeCachedResults()
{
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::freeCachedResults()\n");
  #endif

  freeCacheDouble( colsol_ ); 
  freeCacheDouble( rowsol_ );
  freeCacheDouble( redcost_ );
  freeCacheDouble( rowact_ );
  
  MSKassert(3,colsol_==NULL,"colsol_==NULL","freeCachedResults");
  MSKassert(3,rowsol_==NULL,"rowsol_==NULL","freeCachedResults");
  MSKassert(3,redcost_==NULL,"redcost_==NULL","freeCachedResults");
  MSKassert(3,rowact_==NULL,"rowact_==NULL","freeCachedResults");

  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("End OsiMskSolverInterface::freeCachedResults()\n");
  #endif
}

//-----------------------------------------------------------------------------
//  Free function

void OsiMskSolverInterface::freeCachedData( int keepCached )
{
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::freeCachedResults()\n");
  #endif

  if( !(keepCached & OsiMskSolverInterface::KEEPCACHED_COLUMN) )
    freeCachedColRim();
  if( !(keepCached & OsiMskSolverInterface::KEEPCACHED_ROW) )
    freeCachedRowRim();
  if( !(keepCached & OsiMskSolverInterface::KEEPCACHED_MATRIX) )
    freeCachedMatrix();
  if( !(keepCached & OsiMskSolverInterface::KEEPCACHED_RESULTS) )
    freeCachedResults();

  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("End OsiMskSolverInterface::freeCachedResults()\n");
  #endif
}

//-----------------------------------------------------------------------------
//  Free function

void OsiMskSolverInterface::freeAllMemory()
{
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("Begin OsiMskSolverInterface::freeCachedResults()\n");
  #endif

  freeCachedData();

  if( hotStartCStat_ != NULL )
    delete[] hotStartCStat_;

  if( hotStartRStat_ != NULL )
    delete[] hotStartRStat_;

  hotStartCStat_     = NULL;
  hotStartCStatSize_ = 0;
  hotStartRStat_     = NULL;
  hotStartRStatSize_ = 0;
  freeColType();
  
  #if MSK_OSI_DEBUG_LEVEL > 3
  debugMessage("End OsiMskSolverInterface::freeCachedResults()\n");
  #endif
}
