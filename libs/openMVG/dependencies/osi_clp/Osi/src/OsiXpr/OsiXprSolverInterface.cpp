// copyright (C) 2000, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).


/* OPEN: have a look at the OPEN tags */
/* OPEN: read/set more controls ... */

#include <cassert>
#include <numeric>
#include <sstream>

#include "CoinPragma.hpp"

#define __ANSIC_
#include <xprs.h>
#undef  __ANSIC_

#include "OsiXprSolverInterface.hpp"

#include "CoinHelperFunctions.hpp"
#include "OsiCuts.hpp"
#include "OsiColCut.hpp"
#include "CoinPackedMatrix.hpp"
#include "OsiRowCut.hpp"
#include "CoinWarmStartBasis.hpp"
#include "CoinMessage.hpp"

//#define DEBUG

static
void XPRS_CC OsiXprMessageCallback(XPRSprob prob, void *vUserDat, const char *msg, int msgLen, int msgType)
{ 
	if (vUserDat) {
		if (msgType < 0 ) {
			((CoinMessageHandler*)vUserDat)->finish();
		} else {
			/* CoinMessageHandler does not recognize that a "\0" string means that a newline should be printed.
			 * So we let it print a space (followed by a newline).
			 */
			if (((CoinMessageHandler*)vUserDat)->logLevel() > 0)
			  ((CoinMessageHandler*)vUserDat)->message(0, "XPRS", *msg ? msg : " ", ' ') << CoinMessageEol;
		}
	} else {
		if (msgType < 0) {
			fflush(stdout); 
		} else {
			printf("%.*s\n", msgLen, msg); 
			fflush(stdout); 
		}
	}
}

static
void reporterror(const char *fname, int iline, int ierr)
{
  fprintf( stdout, "ERROR: %s in line %d error %d occured\n",
	   fname, iline, ierr );
}

#define XPRS_CHECKED(function, args) do { 		\
      int _nReturn;					\
      if( (_nReturn = function args ) !=0 ) {		\
	 reporterror (#function,__LINE__,_nReturn);	\
      }							\
   } while (0)




//#############################################################################
// Solve methods
//#############################################################################

void
OsiXprSolverInterface::initialSolve() {

  freeSolution();

#if XPVERSION <= 20
   if ( objsense_ == 1.0 ) {
       XPRS_CHECKED( XPRSminim, (prob_,"l") );
   }
   else if ( objsense_ == -1.0 ) {
     XPRS_CHECKED( XPRSmaxim, (prob_,"l"));
   }
#else

   XPRS_CHECKED( XPRSlpoptimize, (prob_, "l") );
#endif

   lastsolvewasmip = false;
}

//-----------------------------------------------------------------------------

void
OsiXprSolverInterface::resolve() {

   freeSolution();

#if XPVERSION <= 20
   if ( objsense_ == 1.0 ) {
       XPRS_CHECKED( XPRSminim, (prob_,"dl") );
   }
   else if ( objsense_ == -1.0 ) {
       XPRS_CHECKED( XPRSmaxim, (prob_,"dl") ) ;
   }
#else

   XPRS_CHECKED( XPRSlpoptimize, (prob_, "dl") );
#endif

   lastsolvewasmip = false;
}

//-----------------------------------------------------------------------------

void
OsiXprSolverInterface::branchAndBound(){
  int status;

  if( colsol_ != NULL && domipstart )
  {
  	XPRS_CHECKED( XPRSloadmipsol, (prob_, colsol_, &status) );
  	/* status = 0 .. accepted
  	 *        = 1 .. infeasible
  	 *        = 2 .. cutoff
  	 *        = 3 .. LP reoptimization interrupted
  	 */
  }

  freeSolution();

#if XPVERSION <= 20
  /* XPRSglobal cannot be called if there is no LP relaxation available yet
   * -> solve LP relaxation first, if no LP solution available */
  XPRS_CHECKED( XPRSgetintattrib, (prob_,XPRS_LPSTATUS, &status) );
  if (status != XPRS_LP_OPTIMAL)
  	initialSolve();

  XPRS_CHECKED( XPRSgetintattrib, (prob_,XPRS_LPSTATUS, &status) );
  if (status != XPRS_LP_OPTIMAL) {
  	messageHandler()->message(0, "XPRS", "XPRESS failed to solve LP relaxation; cannot call XPRSglobal", ' ') << CoinMessageEol;
  	return;
  }
#else

  XPRS_CHECKED( XPRSmipoptimize, (prob_, "") );
#endif

  lastsolvewasmip = true;
}

//#############################################################################
// Parameter related methods
//#############################################################################
bool
OsiXprSolverInterface::setIntParam(OsiIntParam key, int value)
{
  bool retval = false;
  
  switch (key) {
      case OsiMaxNumIteration:
	  retval = XPRSsetintcontrol(prob_,XPRS_LPITERLIMIT, value) == 0;
	  break;
      case OsiMaxNumIterationHotStart:
	  retval = false;
	  break;
      case OsiLastIntParam:
	 retval = false;
	 break;
      case OsiNameDiscipline:
         retval = false;
	 break;
  }
  return retval;
}

//-----------------------------------------------------------------------------

/* OPEN: more dbl parameters ... */
bool
OsiXprSolverInterface::setDblParam(OsiDblParam key, double value)
{
  bool retval = false;
  switch (key) {
  case OsiDualObjectiveLimit:
    retval = XPRSsetdblcontrol(prob_,XPRS_MIPABSCUTOFF, value) == 0;
    break;
  case OsiPrimalObjectiveLimit:
    retval = false;	
    break;
  case OsiDualTolerance:
    retval = XPRSsetdblcontrol(prob_, XPRS_FEASTOL, value) == 0;	
    break;
  case OsiPrimalTolerance:
    retval = XPRSsetdblcontrol(prob_, XPRS_FEASTOL, value) == 0;	
    break;
  case OsiObjOffset: 
    return OsiSolverInterface::setDblParam(key, value);
  case OsiLastDblParam:
    retval = false;
    break;
  }
  return retval;
}
//-----------------------------------------------------------------------------

bool
OsiXprSolverInterface::setStrParam(OsiStrParam key, const std::string & value)
{
    bool retval=false;
    switch (key) {
	
	case OsiProbName:
/* OPEN: what does this mean */
	    OsiSolverInterface::setStrParam(key,value);
	    return retval = true;
	    
	case OsiLastStrParam:
	    return false;
	case OsiSolverName:
	    return false;
    }
    return false;
}
//-----------------------------------------------------------------------------

bool
OsiXprSolverInterface::getIntParam(OsiIntParam key, int& value) const
{
    bool retval = false;
    
    switch (key) {
	case OsiMaxNumIteration:
	    /* OPEN: the return value was logically wrong */
	    retval = XPRSgetintcontrol(prob_,XPRS_LPITERLIMIT, &value) == 0;
	    break;
	case OsiMaxNumIterationHotStart:
	    retval = false;
	    break;
	case OsiLastIntParam:
	    retval = false;
	    break;
        case OsiNameDiscipline:
	    retval = false;
	    break;
    }
    return retval;
}

//-----------------------------------------------------------------------------

bool
OsiXprSolverInterface::getDblParam(OsiDblParam key, double& value) const
{
  bool retval = false;

  switch (key) {
      case OsiDualObjectiveLimit:
	  retval = XPRSgetdblcontrol(prob_,XPRS_MIPABSCUTOFF, &value) == 0;
	  break;
      case OsiPrimalObjectiveLimit:
	  retval = false;
	  break;
      case OsiDualTolerance:
	  retval = XPRSgetdblcontrol(prob_, XPRS_FEASTOL, &value) == 0;
	  break;
      case OsiPrimalTolerance:
	  retval = XPRSgetdblcontrol(prob_, XPRS_FEASTOL, &value) == 0;
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

//-----------------------------------------------------------------------------

bool
OsiXprSolverInterface::getStrParam(OsiStrParam key, std::string & value) const
{
  bool retval = false;
  switch (key) {
  case OsiProbName:
    OsiSolverInterface::getStrParam(key, value);
    retval = true;
    break;
  case OsiSolverName:
    value = "xpress";
    retval = true;
    break;
  case OsiLastStrParam:
    retval = false;
  }
  return retval;
}

//#############################################################################
// Methods returning info on how the solution process terminated
//#############################################################################

bool OsiXprSolverInterface::isAbandoned() const
{
  int  status, glstat;

  XPRS_CHECKED( XPRSgetintattrib, (prob_,XPRS_LPSTATUS, &status) );
  XPRS_CHECKED( XPRSgetintattrib, (prob_,XPRS_MIPSTATUS, &glstat) );

  return status == XPRS_LP_UNFINISHED	  // LP unfinished
      || glstat == XPRS_MIP_NO_SOL_FOUND  // global search incomplete -- no int sol
      || glstat == XPRS_MIP_SOLUTION;	  // global search incomplete -- int sol found
}

bool OsiXprSolverInterface::isProvenOptimal() const
{
  int  status, glstat;

  XPRS_CHECKED( XPRSgetintattrib, (prob_,XPRS_LPSTATUS, &status) );
  XPRS_CHECKED( XPRSgetintattrib, (prob_,XPRS_MIPSTATUS, &glstat) );

  return status == XPRS_LP_OPTIMAL   // LP optimal
      || status == XPRS_LP_CUTOFF    // LP obj worse than cutoff
      || glstat == XPRS_MIP_OPTIMAL; // global search complete -- int found
}

bool OsiXprSolverInterface::isProvenPrimalInfeasible() const
{
  int status;

  XPRS_CHECKED( XPRSgetintattrib, (prob_,XPRS_LPSTATUS, &status) );

  return status == XPRS_LP_INFEAS; // LP infeasible
}

bool OsiXprSolverInterface::isProvenDualInfeasible() const
{
  int status;

   XPRS_CHECKED( XPRSgetintattrib, (prob_,XPRS_LPSTATUS, &status) );

  return status == XPRS_LP_UNBOUNDED; // LP Unbounded
}

bool OsiXprSolverInterface::isPrimalObjectiveLimitReached() const
{
  /* OPEN: what does that mean	*/
  return false;			// N/A in XOSL
}

bool OsiXprSolverInterface::isDualObjectiveLimitReached() const
{
  int status;

   XPRS_CHECKED( XPRSgetintattrib, (prob_,XPRS_LPSTATUS, &status) );

  return status == XPRS_LP_CUTOFF_IN_DUAL; // LP cut off in dual
}

bool OsiXprSolverInterface::isIterationLimitReached() const
{
    int itrlim, itcnt;
    
    XPRS_CHECKED( XPRSgetintcontrol, (prob_,XPRS_LPITERLIMIT, &itrlim) );
    XPRS_CHECKED( XPRSgetintattrib, (prob_,XPRS_SIMPLEXITER, &itcnt) );
    
    return itcnt >= itrlim;
}

//#############################################################################
// WarmStart related methods
//#############################################################################

CoinWarmStart* OsiXprSolverInterface::getEmptyWarmStart () const
{
	return (dynamic_cast<CoinWarmStart *>(new CoinWarmStartBasis()));
}

CoinWarmStart* OsiXprSolverInterface::getWarmStart() const
{
  int pstat, retstat;

  
  /* OPEN: what does this mean */
  XPRS_CHECKED( XPRSgetintattrib, (prob_,XPRS_PRESOLVESTATE, &pstat) );
  if ( (pstat & 128) == 0 ) return NULL;

  CoinWarmStartBasis *ws = NULL;
  int numcols = getNumCols();
  int numrows = getNumRows();
  const double *lb = getColLower();
  double infty = getInfinity();
  int *rstatus = new int[numrows];
  int *cstatus = new int[numcols];

  retstat = XPRSgetbasis(prob_,rstatus, cstatus);

  if ( retstat == 0 ) {
    int i;

    ws = new CoinWarmStartBasis;
    ws->setSize( numcols, numrows );
      
    for( i = 0;	 i < numrows;  i++ ) {
      switch( rstatus[i] ) {
      case 0:
	ws->setArtifStatus(i, CoinWarmStartBasis::atLowerBound);
	break;
      case 1:
	ws->setArtifStatus(i, CoinWarmStartBasis::basic);
	break;
      case 2:
	ws->setArtifStatus(i, CoinWarmStartBasis::atUpperBound);
	break;
      case 3:
 	ws->setArtifStatus(i, CoinWarmStartBasis::isFree);
 	break;
      default:	// unknown row status
	delete ws;
	ws = NULL;
	goto TERMINATE;
      }
    }

    for( i = 0;	 i < numcols;  i++ ) {
      switch( cstatus[i] ) {
      case 0:
	if ( lb[i] <= -infty )
	  ws->setStructStatus(i, CoinWarmStartBasis::isFree);
	else
	  ws->setStructStatus(i, CoinWarmStartBasis::atLowerBound);
	break;
      case 1:
	ws->setStructStatus( i, CoinWarmStartBasis::basic );
	break;
      case 2:
	ws->setStructStatus( i, CoinWarmStartBasis::atUpperBound );
	break;
      case 3:
  ws->setStructStatus(i, CoinWarmStartBasis::isFree);
  break;
      default:	// unknown column status
	delete ws;
	ws = NULL;
	goto TERMINATE;
      }
    }
  }

 TERMINATE:
  delete[] cstatus;
  delete[] rstatus;

  return ws;
}

//-----------------------------------------------------------------------------

bool OsiXprSolverInterface::setWarmStart(const CoinWarmStart* warmstart)
{
  const CoinWarmStartBasis* ws = dynamic_cast<const CoinWarmStartBasis*>(warmstart);

  if ( !ws ) return false;

  int numcols = ws->getNumStructural();
  int numrows = ws->getNumArtificial();

  if ( numcols != getNumCols() || numrows != getNumRows() )
    return false;

  bool retval;
  int retstat;
  int *cstatus = new int[numcols];
  int *rstatus = new int[numrows];
  int i;

  for ( i = 0;	i < numrows;  i++ ) {
    switch( ws->getArtifStatus(i) ) {
    case CoinWarmStartBasis::atLowerBound:
      rstatus[i] = 0;
      break;
    case CoinWarmStartBasis::basic:
      rstatus[i] = 1;
      break;
    case CoinWarmStartBasis::atUpperBound:
      rstatus[i] = 2;
      break;
    default:  // unknown row status
      retval = false;
      goto TERMINATE;
    }
  }

  for( i = 0;  i < numcols;  i++ ) {
    switch( ws->getStructStatus(i) ) {
    case CoinWarmStartBasis::atLowerBound: 
    case CoinWarmStartBasis::isFree:
      cstatus[i] = 0;
      break;
    case CoinWarmStartBasis::basic:
      cstatus[i] = 1;
      break;
    case CoinWarmStartBasis::atUpperBound:
      cstatus[i] = 2;
      break;
    default:  // unknown row status
      retval = false;
      goto TERMINATE;
    }
  }

  retstat = XPRSloadbasis(prob_,rstatus, cstatus);
  retval = (retstat == 0);

 TERMINATE:
  delete[] cstatus;
  delete[] rstatus;
  return retval;
  
}

//#############################################################################
// Hotstart related methods (primarily used in strong branching)
//#############################################################################

void OsiXprSolverInterface::markHotStart()
{
  // *FIXME* : do better... -LL
  OsiSolverInterface::markHotStart();
}

void OsiXprSolverInterface::solveFromHotStart()
{
  // *FIXME* : do better... -LL
  OsiSolverInterface::solveFromHotStart();
}

void OsiXprSolverInterface::unmarkHotStart()
{
  // *FIXME* : do better... -LL
  OsiSolverInterface::unmarkHotStart();
}

//#############################################################################
// Problem information methods (original data)
//#############################################################################

//------------------------------------------------------------------
// Get number of rows and columns
//------------------------------------------------------------------
int
OsiXprSolverInterface::getNumCols() const
{
  if ( !isDataLoaded() ) return 0;

  int	  ncols;
  XPRS_CHECKED( XPRSgetintattrib, (prob_,XPRS_ORIGINALCOLS, &ncols) );

  return ncols;
}
//-----------------------------------------------------------------------------
int
OsiXprSolverInterface::getNumRows() const
{
   if ( !isDataLoaded() ) return 0;

   int	   nrows;

   XPRS_CHECKED( XPRSgetintattrib, (prob_,XPRS_ORIGINALROWS, &nrows) );

   return nrows;
}
//-----------------------------------------------------------------------------
int
OsiXprSolverInterface::getNumElements() const
{
   if ( !isDataLoaded() ) return 0;

   int	   retVal;

   XPRS_CHECKED( XPRSgetintattrib, (prob_,XPRS_ELEMS, &retVal) );

   return retVal;
}

//------------------------------------------------------------------
// Get pointer to rim vectors
//------------------------------------------------------------------  
const double *
OsiXprSolverInterface::getColLower() const
{
    if ( collower_ == NULL ) {
	if ( isDataLoaded() ) {
	    int	    ncols = getNumCols();
	    
	    if ( ncols > 0 ) {
	       collower_ = new double[ncols];
	       XPRS_CHECKED( XPRSgetlb, (prob_,collower_, 0, ncols - 1) );
	    }
	}
    }
    
    return collower_;
}

//-----------------------------------------------------------------------------
const double *
OsiXprSolverInterface::getColUpper() const
{
   if ( colupper_ == NULL ) {
      if ( isDataLoaded() ) {
	 int	 ncols = getNumCols();	 
	 
	 if ( ncols > 0 ) {
	    colupper_ = new double[ncols];
	    XPRS_CHECKED( XPRSgetub, (prob_,colupper_, 0, ncols - 1) );
	 }
      }
   }

   return colupper_;
}
//-----------------------------------------------------------------------------
const char *
OsiXprSolverInterface::getRowSense() const
{
    if ( rowsense_ == NULL ) {
	if ( isDataLoaded() ) {
	    int nrows = getNumRows();

	    if ( nrows > 0 ) {
	       rowsense_ = new char[nrows];
	       XPRS_CHECKED( XPRSgetrowtype, (prob_,rowsense_, 0, nrows - 1) );
	    }
	}
    }

   return rowsense_;
}
//-----------------------------------------------------------------------------
const double *
OsiXprSolverInterface::getRightHandSide() const
{
   if ( rhs_ == NULL ) {
      if ( isDataLoaded() ) {
	 int	 nrows = getNumRows();	

	 if ( nrows > 0 ) {
	    rhs_ = new double[nrows];
	    XPRS_CHECKED( XPRSgetrhs, (prob_,rhs_, 0, nrows - 1) );

	    // Make sure free rows have rhs of zero
	    const char * rs = getRowSense();
	    int nr = getNumRows();
	    int i;
	    for ( i = 0;  i < nr;	i++ ) {
	       if ( rs[i] == 'N' ) rhs_[i]=0.0;
	    }
	 }
      }
   }

   return rhs_;
}
//-----------------------------------------------------------------------------
const double *
OsiXprSolverInterface::getRowRange() const
{
   if ( rowrange_ == NULL ) {
      if ( isDataLoaded() ) {
	 int	 nrows = getNumRows();

	 if ( nrows > 0 ) {
	    rowrange_ = new double[nrows];
	    XPRS_CHECKED( XPRSgetrhsrange, (prob_,rowrange_, 0, nrows - 1) );

	    // Make sure non-R rows have range of 0.0
	    // XPRESS seems to set N and L rows to a range of Infinity
	    const char * rs = getRowSense();
	    int nr = getNumRows();
	    int i;
	    for ( i = 0;  i < nr;	i++ ) {
	       if ( rs[i] != 'R' ) rowrange_[i] = 0.0;
	    }
	 }
      }
   }

   return rowrange_;
}
//-----------------------------------------------------------------------------
const double *
OsiXprSolverInterface::getRowLower() const
{
   if ( rowlower_ == NULL ) {
      int     nrows = getNumRows();
      const   char    *rowsense = getRowSense();
      const   double  *rhs	= getRightHandSide();
      const   double  *rowrange = getRowRange();

      if ( nrows > 0 ) {
	 rowlower_ = new double[nrows];

	 double dum1;
	 for ( int i = 0;  i < nrows;  i++ ) {
	   convertSenseToBound(rowsense[i], rhs[i], rowrange[i],
			       rowlower_[i], dum1);
	 }
      }
   }

   return rowlower_;
}
//-----------------------------------------------------------------------------
const double *
OsiXprSolverInterface::getRowUpper() const
{
   if ( rowupper_ == NULL ) {
      int     nrows = getNumRows();
      const   char    *rowsense = getRowSense();
      const   double  *rhs	= getRightHandSide();
      const   double  *rowrange = getRowRange();

      if ( nrows > 0 ) {
	 rowupper_ = new double[nrows];

	 double dum1;
	 for ( int i = 0;  i < nrows;  i++ ) {
	   convertSenseToBound(rowsense[i], rhs[i], rowrange[i],
			       dum1, rowupper_[i]);
	 }
      }
   }

   return rowupper_;
}
//-----------------------------------------------------------------------------
const double *
OsiXprSolverInterface::getObjCoefficients() const
{
   if ( objcoeffs_ == NULL ) {
      if ( isDataLoaded() ) {
	 int	 ncols = getNumCols();

	 if ( ncols > 0 ) {
	    objcoeffs_ = new double[ncols];
	    XPRS_CHECKED( XPRSgetobj, (prob_,objcoeffs_, 0, ncols - 1) );
	 }
      }
   }

   return objcoeffs_;
}
//-----------------------------------------------------------------------------
double
OsiXprSolverInterface::getObjSense() const
{
   return objsense_;
}

//-----------------------------------------------------------------------------
// Return information on integrality
//-----------------------------------------------------------------------------

bool
OsiXprSolverInterface::isContinuous(int colNumber) const
{
   getVarTypes();

   //std::cerr <<"OsiXprSolverInterface::isContinuous " <<vartype_[colNumber] <<std::endl;
   if ( vartype_ == NULL ) return true;
   if ( vartype_[colNumber] == 'C' ) return true;
   return false;
}
//-----------------------------------------------------------------------------
#if 0
bool
OsiXprSolverInterface::isInteger( int colNumber ) const
{
   return !(isContinuous(colNumber));
}
//-----------------------------------------------------------------------------
bool
OsiXprSolverInterface::isBinary( int colNumber ) const
{
   const double *cu = colupper();
   const double *cl = collower();
  
   getVarTypes();

   if ( vartype_ == NULL ) return false;
   return (vartype_[colNumber] == 'I' || vartype_[colNumber] == 'B') && 
      (cu[colNumber] == 0.0 || cu[colNumber] == 1.0) && 
      (cl[colNumber] == 0.0 || cl[colNumber] == 1.0);
}
//-----------------------------------------------------------------------------
bool
OsiXprSolverInterface::isIntegerNonBinary( int colNumber ) const
{
   getVarTypes();

   if ( vartype_ == NULL ) return false;
   return (vartype_[colNumber] == 'I' || vartype_[colNumber] == 'B') &&
      !isBinary(colNumber);  
}
//-----------------------------------------------------------------------------
bool
OsiXprSolverInterface::isFreeBinary( int colNumber ) const
{
   const   double  *colupper = this->colupper();
   const   double  *collower = this->collower();

   getVarTypes();

   return isBinary(colNumber) && colupper[colNumber] != collower[colNumber];
}
#endif

//------------------------------------------------------------------
// Row and column copies of the matrix ...
//------------------------------------------------------------------

const CoinPackedMatrix *
OsiXprSolverInterface::getMatrixByRow() const
{
  if ( matrixByRow_ == NULL ) {
    if ( isDataLoaded() ) {

      int     nrows = getNumRows();
      int     ncols = getNumCols();
      int     nelems;

      XPRS_CHECKED( XPRSgetrows, (prob_,NULL, NULL, NULL, 0, &nelems, 0, nrows - 1) );
	 
      int     *start   = new int   [nrows + 1];
      int     *length  = new int   [nrows];
      int     *index   = new int   [nelems];
      double  *element = new double[nelems];

      XPRS_CHECKED( XPRSgetrows, (prob_,start, index, element, nelems, &nelems, 0, nrows - 1) );

      std::adjacent_difference(start + 1, start + (nrows+1), length);
      
      matrixByRow_ = new CoinPackedMatrix(true,0,0);	// (mjs) No gaps!
							// Gaps =><= presolve.
      matrixByRow_->assignMatrix(false /* not column ordered */,
				 ncols, nrows, nelems,
				 element, index, start, length);
    } else {
       matrixByRow_ = new CoinPackedMatrix(true,0,0);	// (mjs) No gaps!
      matrixByRow_->reverseOrdering();
    }
  }

  return matrixByRow_;
} 

//-----------------------------------------------------------------------------
const CoinPackedMatrix *
OsiXprSolverInterface::getMatrixByCol() const
{
   if ( matrixByCol_ == NULL ) {
      matrixByCol_ = new CoinPackedMatrix(*getMatrixByRow());
      matrixByCol_->reverseOrdering();
   }

   return matrixByCol_;
}

//------------------------------------------------------------------
// Get solver's value for infinity
//------------------------------------------------------------------
double
OsiXprSolverInterface::getInfinity() const
{
   return XPRS_PLUSINFINITY;
}

//#############################################################################
// Problem information methods (results)
//#############################################################################

const double *
OsiXprSolverInterface::getColSolution() const
{
	if ( colsol_ == NULL ) {
		if ( isDataLoaded() ) {
			int status;
			int nc = getNumCols();

			if ( nc > 0 ) {
				colsol_ = new double[nc];
				if( lastsolvewasmip ) {

				  XPRS_CHECKED( XPRSgetintattrib, (prob_,XPRS_MIPSTATUS, &status) );
				  if( status == XPRS_MIP_SOLUTION || status == XPRS_MIP_OPTIMAL )
				  	XPRS_CHECKED( XPRSgetmipsol, (prob_,colsol_, NULL) );
				  else
				  	XPRS_CHECKED( XPRSgetlb, (prob_,colsol_, 0, nc-1) );

				} else {

				  XPRS_CHECKED( XPRSgetintattrib, (prob_,XPRS_LPSTATUS, &status) );
				  if( status == XPRS_LP_OPTIMAL )
				  	XPRS_CHECKED( XPRSgetlpsol, (prob_,colsol_, NULL, NULL, NULL) );
				  else
				  	XPRS_CHECKED( XPRSgetlb, (prob_,colsol_, 0, nc-1) );
				}
			}
		}
	}

   return colsol_;
}

//-----------------------------------------------------------------------------

const double *
OsiXprSolverInterface::getRowPrice() const
{
	if ( rowprice_ == NULL ) {
		if ( isDataLoaded() ) {
			int nr = getNumRows();

			if ( nr > 0 ) {
	    	int status;

				rowprice_ = new double[nr];

			  XPRS_CHECKED( XPRSgetintattrib, (prob_,XPRS_LPSTATUS, &status) );
			  if( status == XPRS_LP_OPTIMAL )
			  	XPRS_CHECKED( XPRSgetlpsol, (prob_,NULL, NULL, rowprice_, NULL) );
			  else
			  	memset(rowprice_, 0, nr * sizeof(double));
			}
		}
	}

	return rowprice_;
}

//-----------------------------------------------------------------------------

const double * OsiXprSolverInterface::getReducedCost() const
{
  if ( colprice_ == NULL ) {
    if ( isDataLoaded() ) {
    	int status;
      int nc = getNumCols();

      colprice_ = new double[nc];

		  XPRS_CHECKED( XPRSgetintattrib, (prob_,XPRS_LPSTATUS, &status) );
		  if( status == XPRS_LP_OPTIMAL )
		  	XPRS_CHECKED( XPRSgetlpsol, (prob_,NULL, NULL,NULL, colprice_) );
		  else
		  	memset(colprice_, 0, nc * sizeof(double));
    }
  }
  return colprice_;
}

//-----------------------------------------------------------------------------

const double * OsiXprSolverInterface::getRowActivity() const
{
	if( rowact_ == NULL ) {
		if ( isDataLoaded() ) {
			int nrows = getNumRows();
			if( nrows > 0 ) {
				int status;
				int i;
				const double* rhs = getRightHandSide();

				rowact_ = new double[nrows];

				if( lastsolvewasmip ) {

					XPRS_CHECKED( XPRSgetintattrib, (prob_,XPRS_MIPSTATUS, &status) );
					if( status == XPRS_MIP_SOLUTION || status == XPRS_MIP_OPTIMAL ) {
						XPRS_CHECKED( XPRSgetmipsol, (prob_,NULL,rowact_) );
						for ( i = 0;	i < nrows;  i++ )
							rowact_[i] = rhs[i] - rowact_[i];
					} else
						memset(rowact_, 0, nrows * sizeof(double));

				} else {

					XPRS_CHECKED( XPRSgetintattrib, (prob_,XPRS_LPSTATUS, &status) );
					if( status == XPRS_LP_OPTIMAL ) {
						XPRS_CHECKED( XPRSgetlpsol, (prob_,NULL, rowact_, NULL, NULL) );
						for ( i = 0;	i < nrows;  i++ )
							rowact_[i] = rhs[i] - rowact_[i];
					} else
						memset(rowact_, 0, nrows * sizeof(double));
				}

#if 0
				/* OPEN: why do we need the presolve state here ??? */
				XPRS_CHECKED( XPRSgetintattrib, (prob_,XPRS_PRESOLVESTATE, &status) );

				if ( status == 7 ) {
					int i;

					XPRS_CHECKED( XPRSgetsol, (prob_,NULL, rowact_, NULL, NULL) );

					for ( i = 0;	i < nrows;  i++ )
						rowact_[i] = rhs[i] - rowact_[i];
				} else {
					CoinFillN(rowact_, nrows, 0.0);
				}
#endif
			}
		}
	}

	return rowact_;
}

//-----------------------------------------------------------------------------

double
OsiXprSolverInterface::getObjValue() const
{
	double  objvalue = 0;
	double  objconstant = 0;

	if ( isDataLoaded() ) {
		int status;

		if( lastsolvewasmip ) {

			XPRS_CHECKED( XPRSgetintattrib, (prob_,XPRS_MIPSTATUS, &status) );
			if( status == XPRS_MIP_SOLUTION || status == XPRS_MIP_OPTIMAL )
				XPRS_CHECKED( XPRSgetdblattrib, (prob_, XPRS_MIPOBJVAL, &objvalue) );
			else
				objvalue = CoinPackedVector(getNumCols(), getObjCoefficients()).dotProduct(getColSolution());

		} else {

			XPRS_CHECKED( XPRSgetintattrib, (prob_,XPRS_LPSTATUS, &status) );
			if( status == XPRS_LP_OPTIMAL )
				XPRS_CHECKED( XPRSgetdblattrib, (prob_, XPRS_LPOBJVAL, &objvalue) );
			else
				objvalue = CoinPackedVector(getNumCols(), getObjCoefficients()).dotProduct(getColSolution());

		}

		OsiSolverInterface::getDblParam(OsiObjOffset, objconstant);
		// Constant offset is not saved with the xpress representation,
		// but it has to be returned from here anyway.
	}

	return objvalue - objconstant;
}

//-----------------------------------------------------------------------------

int OsiXprSolverInterface::getIterationCount() const
{
  int itcnt;

  XPRS_CHECKED( XPRSgetintattrib, (prob_,XPRS_SIMPLEXITER, &itcnt) );

  return itcnt;
}

//-----------------------------------------------------------------------------

std::vector<double*> OsiXprSolverInterface::getDualRays(int maxNumRays,
							bool fullRay) const
{
  // *FIXME* : must write the method -LL
  throw CoinError("method is not yet written", "getDualRays",
		 "OsiXprSolverInterface");
  return std::vector<double*>();
}

//-----------------------------------------------------------------------------

std::vector<double*> OsiXprSolverInterface::getPrimalRays(int maxNumRays) const
{
#if 0
  // *FIXME* : Still need to expand column into full ncols-length vector

  const int nrows = getNumRows();
  int nrspar;

  XPRS_CHECKED( XPRSgetintattrib, (prob_,XPRS_SPAREROWS, &nrspar) );
  int junb;
  int retcode;

  retcode = XPRSgetunbvec(prob_,&junb);

  if ( retcode != 0 ) 
      return std::vector<double *>(0, (double *) NULL);;
  
  double *ray = new double[nrows];
  

  if ( junb < nrows ) {		// it's a slack
    int i;

    for ( i = 0;  i < nrows;  i++ ) ray[i] = 0.0; 
    ray[junb] = 1.0; 

    XPRS_CHECKED( XPRSftran, (prob_,ray) );
  } else if ( junb >= nrows + nrspar && 
	      junb < nrows + nrspar + getNumCols() ){			
				// it's a structural variable
    int *mstart = new int[nrows];
    int *mrowind = new int[nrows];
    double *dmatval = new double[nrows];
    int nelt;
    int jcol = junb - nrows - nrspar;

    XPRS_CHECKED( XPRSgetcols, (prob_,mstart, mrowind, dmatval, nrows, &nelt, 
				jcol, jcol) ); 
    /* Unpack into the zeroed array y */ 
    int i, ielt;

    for ( i = 0;  i < nrows;  i++ ) ray[i] = 0.0; 
    for ( ielt = 0;  ielt < nelt;  ielt++ ) 
      ray[mrowind[ielt]] = dmatval[ielt]; 

    XPRS_CHECKED( XPRSftran, (prob_,ray) );

    delete [] mstart;
    delete [] mrowind;
    delete [] dmatval;
  } else {			// it's an error
    retcode = 1;
  }

  if ( retcode == 0 ) return std::vector<double *>(1, ray);
  else {
    delete ray;
    return std::vector<double *>(0, (double *) NULL); 
  }
#endif

  // *FIXME* : must write the method -LL
  throw CoinError("method is not yet written", "getPrimalRays",
		 "OsiXprSolverInterface");
  return std::vector<double*>();
}

//-----------------------------------------------------------------------------

#if 0
OsiVectorInt
OsiXprSolverInterface::getFractionalIndices(const double etol) const
{
   OsiVectorInt retVal;
   int	   numInts = numintvars();
   const   double  *sol = colsol();

   getVarTypes();
  
   OsiRelFltEq eq(etol);

   for ( int i = 0;  i < numInts;  i++ ) {
      double colSolElem = sol[ivarind_[i]];
      double distanceFromInteger = colSolElem - floor(colSolElem + 0.5);

      if ( !eq( distanceFromInteger, 0.0 ) )
	 retVal.push_back(ivarind_[i]);
   }

   return retVal;
}
#endif

//#############################################################################
// Problem modifying methods (rim vectors)
//#############################################################################

void
OsiXprSolverInterface::setObjCoeff( int elementIndex, double elementValue )
{
   if ( isDataLoaded() ) {
       XPRS_CHECKED( XPRSchgobj, (prob_,1, &elementIndex, &elementValue) );
      freeCachedResults();
   }
}

//-----------------------------------------------------------------------------

void
OsiXprSolverInterface::setColLower( int elementIndex, double elementValue )
{
   if ( isDataLoaded() ) {
      char boundType = 'L';

      getVarTypes();
      
      if ( vartype_ &&
	   vartype_[elementIndex] == 'B' && 
	   (elementValue != 0.0 && elementValue != 1.0) ) {
	char elementType = 'I';
	
	XPRS_CHECKED( XPRSchgcoltype, (prob_,1, &elementIndex, &elementType) );
      }
      XPRS_CHECKED( XPRSchgbounds, (prob_,1, &elementIndex, &boundType, &elementValue) );

      freeCachedResults();
      //    delete [] collower_;
      //    collower_ = NULL;
   }
}

//-----------------------------------------------------------------------------

void
OsiXprSolverInterface::setColUpper( int elementIndex, double elementValue )
{
   if ( isDataLoaded() ) {
      char boundType = 'U';

      getVarTypes();	  
      
      XPRS_CHECKED( XPRSchgbounds, (prob_,1, &elementIndex, &boundType, &elementValue) );
      if ( vartype_ &&
	   vartype_[elementIndex] == 'B' && 
	   (elementValue != 0.0 && elementValue != 1.0) ) {
	 char elementType = 'I';  
	 
	 XPRS_CHECKED( XPRSchgcoltype, (prob_,1, &elementIndex, &elementType) );
      }
      freeCachedResults();
      //    delete [] colupper_;
      //    colupper_ = NULL;
   } 
}

//-----------------------------------------------------------------------------

void OsiXprSolverInterface::setColBounds(const int elementIndex, double lower, double upper )
{
   if ( isDataLoaded() ) {
     char qbtype[2] = { 'L', 'U' };
     int mindex[2];
     double bnd[2];

     getVarTypes();
     mindex[0] = elementIndex;
     mindex[1] = elementIndex;
     bnd[0] = lower;
     bnd[1] = upper;

     XPRS_CHECKED( XPRSchgbounds, (prob_,2, mindex, qbtype, bnd) );
       
     if ( vartype_ && 
	  vartype_[mindex[0]] == 'B' && 
	  !((lower == 0.0 && upper == 0.0) ||
	    (lower == 1.0 && upper == 1.0) ||
	    (lower == 0.0 && upper == 1.0)) ) {
       char elementType = 'I';	
	 
       XPRS_CHECKED( XPRSchgcoltype, (prob_,1, &mindex[0], &elementType) );
     }
     freeCachedResults();
     //	   delete [] colupper_;
     //	   colupper_ = NULL;
   }
}

//-----------------------------------------------------------------------------

void OsiXprSolverInterface::setColSetBounds(const int* indexFirst,
					    const int* indexLast,
					    const double* boundList)
{
  OsiSolverInterface::setColSetBounds(indexFirst, indexLast, boundList);
}

//-----------------------------------------------------------------------------

void
OsiXprSolverInterface::setRowLower( int elementIndex, double elementValue )
{
  double rhs   = getRightHandSide()[elementIndex];
  double range = getRowRange()[elementIndex];
  char	 sense = getRowSense()[elementIndex];
  double lower = 0, upper = 0;

  convertSenseToBound(sense, rhs, range, lower, upper);
  if( lower != elementValue ) {
    convertBoundToSense(elementValue, upper, sense, rhs, range);
    setRowType(elementIndex, sense, rhs, range);
    // freeCachedResults(); --- invoked in setRowType()
  }
}

//-----------------------------------------------------------------------------

void
OsiXprSolverInterface::setRowUpper( int elementIndex, double elementValue )
{
  double rhs   = getRightHandSide()[elementIndex];
  double range = getRowRange()[elementIndex];
  char	 sense = getRowSense()[elementIndex];
  double lower = 0, upper = 0;

  convertSenseToBound( sense, rhs, range, lower, upper );
  if( upper != elementValue ) {
    convertBoundToSense(lower, elementValue, sense, rhs, range);
    setRowType(elementIndex, sense, rhs, range);
    // freeCachedResults(); --- invoked in setRowType()
  }
}

//-----------------------------------------------------------------------------

void
OsiXprSolverInterface::setRowBounds( int elementIndex, double lower, double upper )
{
  double rhs, range;
  char sense;
  
  convertBoundToSense( lower, upper, sense, rhs, range );
  setRowType( elementIndex, sense, rhs, range );
  // freeCachedRowRim(); --- invoked in setRowType()
}

//-----------------------------------------------------------------------------

void
OsiXprSolverInterface::setRowType(int index, char sense, double rightHandSide,
				  double range)
{
  if ( isDataLoaded() ) {
    int mindex[1] = {index};
    char qrtype[1] = {sense}; 
    double rhs[1] = {rightHandSide};
    double rng[1] = {range};

    XPRS_CHECKED( XPRSchgrowtype, (prob_,1, mindex, qrtype) );
    XPRS_CHECKED( XPRSchgrhs, (prob_,1, mindex, rhs) );
  	// range is properly defined only for range-type rows, so we call XPRSchgrhsrange only for ranged rows
    if (sense == 'R')
    	XPRS_CHECKED( XPRSchgrhsrange, (prob_,1, mindex, rng) );

    freeCachedResults();
  }
}

//-----------------------------------------------------------------------------

void OsiXprSolverInterface::setRowSetBounds(const int* indexFirst,
					    const int* indexLast,
					    const double* boundList)
{
  OsiSolverInterface::setRowSetBounds(indexFirst, indexLast, boundList);
}

//-----------------------------------------------------------------------------

void
OsiXprSolverInterface::setRowSetTypes(const int* indexFirst,
				      const int* indexLast,
				      const char* senseList,
				      const double* rhsList,
				      const double* rangeList)
{
  OsiSolverInterface::setRowSetTypes(indexFirst, indexLast, senseList, rhsList, rangeList);
}

//#############################################################################
void 
OsiXprSolverInterface::setContinuous(int index) 
{
  if ( isDataLoaded() ) {
    int pstat;

    XPRS_CHECKED( XPRSgetintattrib, (prob_,XPRS_PRESOLVESTATE, &pstat) );

    if ( (pstat & 6) == 0 ) {		// not presolved
      char qctype = 'C';

      XPRS_CHECKED( XPRSchgcoltype, (prob_,1, &index, &qctype) );
      freeCachedResults();
    }
  }
}

void 
OsiXprSolverInterface::setInteger(int index) 
{
  if ( isDataLoaded() ) {
    int pstat;

    XPRS_CHECKED( XPRSgetintattrib, (prob_,XPRS_PRESOLVESTATE, &pstat) );

    if ( (pstat & 6) == 0 ) {		// not presolved
      char qctype;

      if ( getColLower()[index] == 0.0 && 
	   getColUpper()[index] == 1.0 ) 
	qctype = 'B';
      else
	qctype = 'I';
      XPRS_CHECKED( XPRSchgcoltype, (prob_,1, &index, &qctype) );
      freeCachedResults();
    }
  }
}

void 
OsiXprSolverInterface::setContinuous(const int* indices, int len) 
{
  if ( isDataLoaded() ) {
    int pstat;

    XPRS_CHECKED( XPRSgetintattrib, (prob_,XPRS_PRESOLVESTATE, &pstat) );

    if ( (pstat & 6) == 0 ) {		// not presolved
      char *qctype = new char[len];

      CoinFillN(qctype, len, 'C');
      
      XPRS_CHECKED( XPRSchgcoltype, (prob_,len, const_cast<int *>(indices), qctype) );
      freeCachedResults();
      delete[] qctype;
    }
  }
}

void 
OsiXprSolverInterface::setInteger(const int* indices, int len) 
{
  if ( isDataLoaded() ) {
    int pstat;

    XPRS_CHECKED( XPRSgetintattrib, (prob_,XPRS_PRESOLVESTATE, &pstat) );

    if ( (pstat & 6) == 0 ) {		// not presolved
      char *qctype = new char[len];
      const double* clb = getColLower();
      const double* cub = getColUpper();

      for ( int i = 0;	i < len;  i++ ) {
	if ( clb[indices[i]] == 0.0 && cub[indices[i]] == 1.0 )
	  qctype[i] = 'B';
	else 
	  qctype[i] = 'I';
      }

      XPRS_CHECKED( XPRSchgcoltype, (prob_,len, const_cast<int *>(indices), qctype) );
      freeCachedResults();
      delete[] qctype;
    }
  }
}

//#############################################################################

void
OsiXprSolverInterface::setObjSense(double s) 
{
   assert(s == 1.0 || s == -1.0);

   objsense_ = s;

   XPRS_CHECKED( XPRSchgobjsense, (prob_, (int)s) );
}

//-----------------------------------------------------------------------------

void
OsiXprSolverInterface::setColSolution(const double *colsol)
{
   freeSolution();

   colsol_ = new double[getNumCols()];

   for ( int i = 0;  i < getNumCols();	i++ )
      colsol_[i] = colsol[i];
}

//-----------------------------------------------------------------------------

void
OsiXprSolverInterface::setRowPrice(const double *rowprice)
{
   freeSolution();

   rowprice_ = new double[getNumRows()];

   for ( int i = 0;  i < getNumRows();	i++ )
      rowprice_[i] = rowprice[i];
}

//#############################################################################
// Problem modifying methods (matrix)
//#############################################################################

void 
OsiXprSolverInterface::addCol(const CoinPackedVectorBase& vec,
			      const double collb, const double colub,	
			      const double obj)
{
  if ( isDataLoaded() ) {
    freeCachedResults();

    int mstart = 0;

    XPRS_CHECKED( XPRSaddcols, (prob_,1, vec.getNumElements(), const_cast<double*>(&obj),
	    &mstart,
	    const_cast<int*>(vec.getIndices()),
	    const_cast<double*>(vec.getElements()),
	    const_cast<double*>(&collb),
	    const_cast<double*>(&colub)) );
  }
}
//-----------------------------------------------------------------------------
void 
OsiXprSolverInterface::addCols(const int numcols,
			       const CoinPackedVectorBase * const * cols,
			       const double* collb, const double* colub,   
			       const double* obj)
{
  // freeCachedResults();

  for( int i = 0;  i < numcols;	 i++ )
      addCol( *(cols[i]), collb[i], colub[i], obj[i] );
}
//-----------------------------------------------------------------------------
void 
OsiXprSolverInterface::deleteCols(const int num, const int *columnIndices)
{
  freeCachedResults();
  XPRS_CHECKED( XPRSdelcols, (prob_,num, const_cast<int *>(columnIndices)) );
}
//-----------------------------------------------------------------------------
void 
OsiXprSolverInterface::addRow(const CoinPackedVectorBase& vec,
			      const double rowlb, const double rowub)
{
  // freeCachedResults(); -- will be invoked

  char sense;
  double rhs, range;

  convertBoundToSense(rowlb, rowub, sense, rhs, range);
  addRow(vec, sense, rhs, range);
}
//-----------------------------------------------------------------------------
void 
OsiXprSolverInterface::addRow(const CoinPackedVectorBase& vec,
			      const char rowsen, const double rowrhs,	
			      const double rowrng)
{
  freeCachedResults();

   int mstart[2] = {0, vec.getNumElements()};

  XPRS_CHECKED( XPRSaddrows, (prob_,1, vec.getNumElements(), 
	  const_cast<char *>(&rowsen), const_cast<double *>(&rowrhs), 
	  const_cast<double *>(&rowrng), mstart,
	  const_cast<int *>(vec.getIndices()),
	  const_cast<double *>(vec.getElements())) );
}
//-----------------------------------------------------------------------------
void 
OsiXprSolverInterface::addRows(const int numrows,
			       const CoinPackedVectorBase * const * rows,
			       const double* rowlb, const double* rowub)
{
  // *FIXME* : must write the method -LL
  throw CoinError("method is not yet written", "addRows",
		 "OsiXprSolverInterface");
}
//-----------------------------------------------------------------------------
void 
OsiXprSolverInterface::addRows(const int numrows,
			       const CoinPackedVectorBase * const * rows,
			       const char* rowsen, const double* rowrhs,   
			       const double* rowrng)
{
  // *FIXME* : must write the method -LL
  throw CoinError("method is not yet written", "addRows",
		 "OsiXprSolverInterface");
}
//-----------------------------------------------------------------------------
void 
OsiXprSolverInterface::deleteRows(const int num, const int * rowIndices)
{
  freeCachedResults();

  XPRS_CHECKED( XPRSdelrows, (prob_,num, const_cast<int *>(rowIndices)) );
}

//#############################################################################
// Methods to input a problem
//#############################################################################

void
OsiXprSolverInterface::loadProblem(const CoinPackedMatrix& matrix,
				   const double* collb, const double* colub,   
				   const double* obj,
				   const double* rowlb, const double* rowub)
{
  const double inf = getInfinity();
  
  char	 * rowSense = new char	[matrix.getNumRows()];
  double * rowRhs   = new double[matrix.getNumRows()];
  double * rowRange = new double[matrix.getNumRows()];
  
  int i;
  for ( i = matrix.getNumRows() - 1; i >= 0; --i) {
    
    double rlb;
    if ( rowlb!=NULL )
      rlb = rowlb[i];
    else
      rlb = -inf;
    
     double rub;
     if ( rowub!=NULL )
       rub = rowub[i];
     else
       rub = inf;
     
     convertBoundToSense(rlb,rub,rowSense[i],rowRhs[i],rowRange[i]);
#if 0
     if ( rlb==rub ) {
       rowSense[i]='E';
       rowRhs[i]  =rlb;
       rowRange[i]=0.0;
       continue;
     }
     if ( rlb<=-inf && rub>=inf ) {
       rowSense[i]='N';
       rowRhs[i]  =inf;
       rowRange[i]=0.0;
       continue;
     }
     if ( rlb<=-inf && !(rub>=inf) ) {
       rowSense[i]='L';
       rowRhs[i]  =rub;
       rowRange[i]=0.0;
       continue;
     }
     if ( !(rlb<=-inf) && rub>=inf ) {
       rowSense[i]='G';
       rowRhs[i]  =rlb;
       rowRange[i]=0.0;
       continue;
     }
     if ( !(rlb<=-inf) && !(rub>=inf) ) {
       rowSense[i]='R';
       rowRhs[i]  =rub;
       rowRange[i]=rub-rlb;
       continue;
     }
#endif
   }
  
   loadProblem(matrix, collb, colub, obj, rowSense, rowRhs, rowRange ); 
   
   delete [] rowSense;
   delete [] rowRhs;
   delete [] rowRange;
}

//-----------------------------------------------------------------------------

void
OsiXprSolverInterface::assignProblem(CoinPackedMatrix*& matrix,
				     double*& collb, double*& colub,
				     double*& obj,
				     double*& rowlb, double*& rowub)
{
   loadProblem(*matrix, collb, colub, obj, rowlb, rowub);
   delete matrix;   matrix = 0;
   delete[] collb;  collb = 0;
   delete[] colub;  colub = 0;
   delete[] obj;    obj = 0;
   delete[] rowlb;  rowlb = 0;
   delete[] rowub;  rowub = 0;
}

//-----------------------------------------------------------------------------

// #define OSIXPR_ADD_OBJ_ROW

void
OsiXprSolverInterface::loadProblem(const CoinPackedMatrix& matrix,
				   const double* collb, const double* colub,
				   const double* obj,
				   const char* rowsen, const double* rowrhs,   
				   const double* rowrng)
{
  freeCachedResults();
  int i;

  char*   rsen;
  double* rrhs;
 
  // Set column values to defaults if NULL pointer passed
  int nc=matrix.getNumCols();
  int nr=matrix.getNumRows();
  double * clb;	 
  double * cub;
  double * ob;
  if ( collb!=NULL ) {
    clb=const_cast<double*>(collb);
  }
  else {
    clb = new double[nc];
    for( i=0; i<nc; i++ ) clb[i]=0.0;
  }
  if ( colub!=NULL ) 
    cub=const_cast<double*>(colub);
  else {
    cub = new double[nc];
    for( i=0; i<nc; i++ ) cub[i]=XPRS_PLUSINFINITY;
  }
  if ( obj!=NULL ) 
    ob=const_cast<double*>(obj);
  else {
    ob = new double[nc];
    for( i=0; i<nc; i++ ) ob[i]=0.0;
  }
  if ( rowsen != NULL )
  	rsen = const_cast<char*>(rowsen);
  else {
  	rsen = new char[nr];
  	for( i = 0; i < nr; ++i ) rsen[i] = 'G';
  }
  if ( rowrhs != NULL )
  	rrhs = const_cast<double*>(rowrhs);
  else {
  	rrhs = new double[nr];
  	for( i = 0; i < nr; ++i ) rrhs[i] = 0.0;
  }

  bool freeMatrixRequired = false;
  CoinPackedMatrix * m = NULL;
  if ( !matrix.isColOrdered() ) {
     m = new CoinPackedMatrix(true,0,0);	// (mjs) No gaps!
    m->reverseOrderedCopyOf(matrix);
    freeMatrixRequired = true;
  } else {
    m = const_cast<CoinPackedMatrix *>(&matrix);
  }
  
  // Generate a problem name
  char probName[256];
  sprintf(probName, "Prob%i", osiSerial_);
  
  nc = m->getNumCols();
  nr = m->getNumRows();
  
  if ( getLogFilePtr()!=NULL ) {   
    fprintf(getLogFilePtr(),"{\n"); 

    fprintf(getLogFilePtr(),"  char rowsen[%d];\n",nr);
    for ( i=0; i<nr; i++ )
      fprintf(getLogFilePtr(),"  rowsen[%d]='%c';\n",i,rsen[i]);

    fprintf(getLogFilePtr(),"  double rowrhs[%d];\n",nr);
    for ( i=0; i<nr; i++ )
      fprintf(getLogFilePtr(),"  rowrhs[%d]=%f;\n",i,rrhs[i]);
    
    fprintf(getLogFilePtr(),"  double rowrng[%d];\n",nr);
    for ( i=0; i<nr; i++ )
      fprintf(getLogFilePtr(),"  rowrng[%d]=%f;\n",i,rowrng ? rowrng[i] : 0.0);

    fprintf(getLogFilePtr(),"  double ob[%d];\n",nc);
    for ( i=0; i<nc; i++ )
      fprintf(getLogFilePtr(),"  ob[%d]=%f;\n",i,ob[i]);

    fprintf(getLogFilePtr(),"  double clb[%d];\n",nc);
    for ( i=0; i<nc; i++ )
      fprintf(getLogFilePtr(),"  clb[%d]=%f;\n",i,clb[i]);

    fprintf(getLogFilePtr(),"  double cub[%d];\n",nc);
    for ( i=0; i<nc; i++ )
      fprintf(getLogFilePtr(),"  cub[%d]=%f;\n",i,cub[i]);

    fprintf(getLogFilePtr(),"  int vectorStarts[%d];\n",nc+1);
    for ( i=0; i<=nc; i++ )
      fprintf(getLogFilePtr(),"  vectorStarts[%d]=%d;\n",i,m->getVectorStarts()[i]);

    fprintf(getLogFilePtr(),"  int vectorLengths[%d];\n",nc);
    for ( i=0; i<nc; i++ )
      fprintf(getLogFilePtr(),"  vectorLengths[%d]=%d;\n",i,m->getVectorLengths()[i]);
    
    fprintf(getLogFilePtr(),"  int indices[%d];\n",m->getVectorStarts()[nc]);
    for ( i=0; i<m->getVectorStarts()[nc]; i++ )
      fprintf(getLogFilePtr(),"  indices[%d]=%d;\n",i,m->getIndices()[i]);

    fprintf(getLogFilePtr(),"  double elements[%d];\n",m->getVectorStarts()[nc]);
    for ( i=0; i<m->getVectorStarts()[nc]; i++ )
      fprintf(getLogFilePtr(),"  elements[%d]=%f;\n",i,m->getElements()[i]);

    fprintf(getLogFilePtr(),"}\n");
  }
  int iret = XPRSloadlp( prob_,
			 probName,
			 nc,
			 nr,
			 const_cast<char*>(rsen),
			 const_cast<double*>(rrhs),
			 const_cast<double*>(rowrng),
			 ob,
			 const_cast<int*>(m->getVectorStarts()),
			 const_cast<int*>(m->getVectorLengths()),
			 const_cast<int*>(m->getIndices()),
			 const_cast<double*>(m->getElements()),
			 clb,
			 cub );
  setStrParam(OsiProbName,probName);
  
  if ( iret != 0 )
      XPRSgetintattrib(prob_,XPRS_ERRORCODE, &iret);
  assert( iret == 0 );
  
   
  char pname[256];      // Problem names can be 200 chars in XPRESS 12
  XPRS_CHECKED( XPRSgetprobname,(prob_,pname) );
  xprProbname_ = pname;
  
  if ( collb==NULL ) delete[] clb;
  if ( colub==NULL ) delete[] cub;
  if ( obj  ==NULL ) delete[] ob;
  if ( rowsen==NULL ) delete[] rsen;
  if ( rowrhs==NULL ) delete[] rrhs;
  
  if (freeMatrixRequired) {
    delete m;
  }
}

//-----------------------------------------------------------------------------

void
OsiXprSolverInterface::assignProblem(CoinPackedMatrix*& matrix,
				     double*& collb, double*& colub,
				     double*& obj,
				     char*& rowsen, double*& rowrhs,
				     double*& rowrng)
{
   loadProblem(*matrix, collb, colub, obj, rowsen, rowrhs, rowrng);
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
OsiXprSolverInterface::loadProblem(const int numcols, const int numrows,
				   const int* start, const int* index,
				   const double* value,
				   const double* collb, const double* colub,   
				   const double* obj,
				   const double* rowlb, const double* rowub )
{
  const double inf = getInfinity();
  
  char   * rowSense = new char  [numrows];
  double * rowRhs   = new double[numrows];
  double * rowRange = new double[numrows];
  
  for ( int i = numrows - 1; i >= 0; --i ) {
    const double lower = rowlb ? rowlb[i] : -inf;
    const double upper = rowub ? rowub[i] : inf;
    convertBoundToSense( lower, upper, rowSense[i], rowRhs[i], rowRange[i] );
  }

  loadProblem(numcols, numrows, start, index, value, collb, colub, obj,
	      rowSense, rowRhs, rowRange);
  delete [] rowSense;
  delete [] rowRhs;
  delete [] rowRange;

}

//-----------------------------------------------------------------------------

void
OsiXprSolverInterface::loadProblem(const int numcols, const int numrows,
				   const int* start, const int* index,
				   const double* value,
				   const double* collb, const double* colub,   
				   const double* obj,
				   const char* rowsen, const double* rowrhs,
				   const double* rowrng )
{
  freeCachedResults();
  int i;
 
  // Set column values to defaults if NULL pointer passed
  int nc = numcols;
  int nr = numrows;
  int * len = new int[nc+1];
  double * clb;  
  double * cub;
  double * ob;
  char*   rsen;
  double* rrhs;

  std::adjacent_difference(start, start + (nc+1), len);
  
  if ( collb!=NULL ) {
    clb=const_cast<double*>(collb);
  }
  else {
    clb = new double[nc];
    for( i=0; i<nc; i++ ) clb[i]=0.0;
  }
  if ( colub!=NULL ) 
    cub=const_cast<double*>(colub);
  else {
    cub = new double[nc];
    for( i=0; i<nc; i++ ) cub[i]=XPRS_PLUSINFINITY;
  }
  if ( obj!=NULL ) 
    ob=const_cast<double*>(obj);
  else {
    ob = new double[nc];
    for( i=0; i<nc; i++ ) ob[i]=0.0;
  }
  if ( rowsen != NULL )
  	rsen = const_cast<char*>(rowsen);
  else {
  	rsen = new char[nr];
  	for( i = 0; i < nr; ++i ) rsen[i] = 'G';
  }
  if ( rowrhs != NULL )
  	rrhs = const_cast<double*>(rowrhs);
  else {
  	rrhs = new double[nr];
  	for( i = 0; i < nr; ++i ) rrhs[i] = 0.0;
  }

  // Generate a problem name
  char probName[256];
  sprintf(probName, "Prob%i", osiSerial_);

  if ( getLogFilePtr()!=NULL ) {   
    fprintf(getLogFilePtr(),"{\n"); 

    fprintf(getLogFilePtr(),"  char rowsen[%d];\n",nr);
    for ( i=0; i<nr; i++ )
      fprintf(getLogFilePtr(),"  rowsen[%d]='%c';\n",i,rsen[i]);

    fprintf(getLogFilePtr(),"  double rowrhs[%d];\n",nr);
    for ( i=0; i<nr; i++ )
      fprintf(getLogFilePtr(),"  rowrhs[%d]=%f;\n",i,rrhs[i]);
    
    fprintf(getLogFilePtr(),"  double rowrng[%d];\n",nr);
    for ( i=0; i<nr; i++ )
      fprintf(getLogFilePtr(),"  rowrng[%d]=%f;\n",i,rowrng ? rowrng[i] : 0.0);

    fprintf(getLogFilePtr(),"  double ob[%d];\n",nc);
    for ( i=0; i<nc; i++ )
      fprintf(getLogFilePtr(),"  ob[%d]=%f;\n",i,ob[i]);

    fprintf(getLogFilePtr(),"  double clb[%d];\n",nc);
    for ( i=0; i<nc; i++ )
      fprintf(getLogFilePtr(),"  clb[%d]=%f;\n",i,clb[i]);

    fprintf(getLogFilePtr(),"  double cub[%d];\n",nc);
    for ( i=0; i<nc; i++ )
      fprintf(getLogFilePtr(),"  cub[%d]=%f;\n",i,cub[i]);

    fprintf(getLogFilePtr(),"  int vectorStarts[%d];\n",nc+1);
    for ( i=0; i<=nc; i++ )
      fprintf(getLogFilePtr(),"  vectorStarts[%d]=%d;\n",i,start[i]);

    fprintf(getLogFilePtr(),"  int vectorLengths[%d];\n",nc);
    for ( i=0; i<nc; i++ )
      fprintf(getLogFilePtr(),"  vectorLengths[%d]=%d;\n",i,len[i+1]);
    
    fprintf(getLogFilePtr(),"  int indices[%d];\n",start[nc]);
    for ( i=0; i<start[nc]; i++ )
      fprintf(getLogFilePtr(),"  indices[%d]=%d;\n",i,index[i]);

    fprintf(getLogFilePtr(),"  double elements[%d];\n",start[nc]);
    for ( i=0; i<start[nc]; i++ )
      fprintf(getLogFilePtr(),"  elements[%d]=%f;\n",i,value[i]);

    fprintf(getLogFilePtr(),"}\n");
  }
  int iret = XPRSloadlp(prob_,probName,
			nc,
			nr,
			const_cast<char*>(rsen),
			const_cast<double*>(rrhs),
			const_cast<double*>(rowrng),
			ob,
			const_cast<int*>(start),
			const_cast<int*>(len+1),
			const_cast<int*>(index),
			const_cast<double*>(value),
			clb,
			cub );
  setStrParam(OsiProbName,probName);
  
  if ( iret != 0 )
    XPRSgetintattrib(prob_,XPRS_ERRORCODE, &iret);
  assert( iret == 0 );
  
  char pname[256];      // Problem names can be 200 chars in XPRESS 12
  XPRS_CHECKED( XPRSgetprobname, (prob_,pname) );
  xprProbname_ = pname;
  
  if ( collb == NULL ) delete[] clb;
  if ( colub == NULL ) delete[] cub;
  if ( obj   == NULL ) delete[] ob;
  if ( rowsen== NULL ) delete[] rsen;
  if ( rowrhs== NULL ) delete[] rrhs;
  delete[] len;
}



//-----------------------------------------------------------------------------
// Read mps files
//-----------------------------------------------------------------------------

int
OsiXprSolverInterface::readMps(const char *filename, const char *extension) 
{
#if 0
  if (getLogFilePtr()!=NULL) {
    fprintf(getLogFilePtr(),"{\n");
    fprintf(getLogFilePtr(),"  XPRSreadprob(prob_,\"%s\");\n",filename);
    fprintf(getLogFilePtr(),"  int namlen;\n");
    fprintf(getLogFilePtr(),"  XPRSgetintcontrol(prob_,XPRS_NAMELENGTH,&namlen);\n");
    fprintf(getLogFilePtr(),"  namlen *= 8;\n");
  }
 
  // Read Mps file.
  // XPRESS generates its own file extensions, so we ignore any supplied.
  int iret = XPRSreadprob(prob_,filename);
  if ( iret != 0 )  
    XPRSgetintattrib(prob_,XPRS_ERRORCODE, &iret);
  assert( iret == 0 );

  // Get length of Mps names
  int namlen;
  XPRSgetintattrib(prob_,XPRS_NAMELENGTH, &namlen);
  namlen *= 8;

  if (getLogFilePtr()!=NULL) {
    fprintf(getLogFilePtr(),"  char objRowName[%d],rowName[%d];\n",namlen,namlen);
    fprintf(getLogFilePtr(),"  XPRSgetstrcontrol(prob_,XPRS_MPSOBJNAME, objRowName);\n");
    fprintf(getLogFilePtr(),"  int nr;\n");
    fprintf(getLogFilePtr(),"  XPRSsetintcontrol(prob_,XPRS_ROWS, &nr);");
  }

  // Allocate space to hold row names.
  char * objRowName = new char[namlen+1];
  char * rowName    = new char[namlen+1];

  // Get name of objective row.
  // If "" returned, then first N row is objective row
  XPRSgetstrcontrol(prob_,XPRS_MPSOBJNAME,objRowName);

  // Get number of rows
  int nr;
  XPRSgetintattrib(prob_,XPRS_ROWS, &nr);
    
  if (getLogFilePtr()!=NULL) {
    fprintf(getLogFilePtr(),"  char rs[%d];\n",nr);
    fprintf(getLogFilePtr(),"  XPRSgetrowtype(prob_,rs, 0, %d);\n",nr-1);
  }

  // Get row sense.
  char * rs = new char[nr];
  XPRSgetrowtype(prob_,rs, 0, nr - 1);

  // Loop once for each row looking for objective row
  int i;
  for ( i=0; i<nr; i++ ) {
    
    // Objective row must be an N row
    if ( rs[i]=='N' ) {      
      
      if (getLogFilePtr()!=NULL) {
        fprintf(getLogFilePtr(),"  XPRSgetnames(prob_,1,rowName,%d,%d);\n",i,i);
      }
      
      // Get name of this row
      XPRSgetnames(prob_,1,rowName,i,i);
      
      // Is this the objective row?
      if( strcmp(rowName,objRowName)==0 ||
        strcmp("",objRowName) == 0 ) {
        
        if (getLogFilePtr()!=NULL) {
          fprintf(getLogFilePtr(),"  int rowToDelete[1];\n");
          fprintf(getLogFilePtr(),"  rowToDelete[0]=%d;\n",i);
          fprintf(getLogFilePtr(),"  XPRSdelrows(prob_,1,rowToDelete);\n");
        }
        
        // found objective row. now delete it
        int rowToDelete[1];
        rowToDelete[0] = i;
        XPRSdelrows(prob_,1,rowToDelete);
        break;
      }
    }
  }

  delete [] rs;
  delete [] rowName;
  delete [] objRowName;
  
  if (getLogFilePtr()!=NULL) {
    fprintf(getLogFilePtr(),"  char pname[256];\n");
    fprintf(getLogFilePtr(),"  XPRSgetprobname(prob_,pname);\n");
    fprintf(getLogFilePtr(),"}\n");
  }
  
  char pname[256];      // Problem names can be 200 chars in XPRESS 12
  XPRSgetprobname(prob_,pname);
  xprProbname_ = pname;
  return 0;
#else
  int retVal = OsiSolverInterface::readMps(filename,extension);
  getStrParam(OsiProbName,xprProbname_);
  return retVal;
#endif
}


//-----------------------------------------------------------------------------
// Write mps files
//-----------------------------------------------------------------------------
void OsiXprSolverInterface::writeMps(const char *filename,
				     const char *extension,
				     double objSense) const
{
	// Note: XPRESS insists on ignoring the extension and
  // adding ".mat" instead.  Forewarned is forearmed.  NOTE: that does not happen for me (Xpress 21).
  // Note: Passing an empty filename produces a file named after
  // the problem.

	assert(filename != NULL);
	assert(extension != NULL);

	char* fullname = new char[strlen(filename) + strlen(extension) + 2];
	sprintf(fullname, "%s.%s", filename, extension);

  XPRS_CHECKED( XPRSwriteprob, (prob_,fullname, "") );

  delete[] fullname;
}

//-----------------------------------------------------------------------------
// Message Handling
//-----------------------------------------------------------------------------
void OsiXprSolverInterface::passInMessageHandler(CoinMessageHandler * handler)
{
	OsiSolverInterface::passInMessageHandler(handler);
	
  XPRS_CHECKED( XPRSsetcbmessage, (prob_, OsiXprMessageCallback, messageHandler()) );
}


//#############################################################################
// Static data and methods
//#############################################################################

void
OsiXprSolverInterface::incrementInstanceCounter()
{
#ifdef DEBUG
    fprintf( stdout, "incrementInstanceCounter:: numInstances %d\n", 
	     numInstances_);
#endif
    if ( numInstances_ == 0 ) {          

       if( XPRSinit(NULL) != 0 ) {
         throw CoinError("failed to init XPRESS, maybe no license", "incrementInstanceCounter", "OsiXprSolverInterface");
       }
    }

    numInstances_++;
    osiSerial_++;
    
    //  cout << "Instances = " << numInstances_ << "; Serial = " << osiSerial_ << endl;
}

//-----------------------------------------------------------------------------

void
OsiXprSolverInterface::decrementInstanceCounter()
{
#ifdef DEBUG
    fprintf( stdout, "decrementInstanceCounter:: numInstances %d\n", 
	     numInstances_);
#endif
  assert( numInstances_ != 0 );

  numInstances_--;
 
  //  cout << "Instances = " << numInstances_ << endl;
  
  if ( numInstances_ == 0 ) {
      XPRSfree();
  }
}

//-----------------------------------------------------------------------------

unsigned int
OsiXprSolverInterface::getNumInstances()
{
#ifdef DEBUG
    fprintf( stdout, "getNumInstances:: numInstances %d\n", 
	     numInstances_);
#endif
   return numInstances_;
}

//-----------------------------------------------------------------------------

// Return XPRESS-MP Version number
int OsiXprSolverInterface::version()
{
/* OPEN: handle version properly !!! */

/*
    int retVal;
    XPRSprob p;
    incrementInstanceCounter();
    XPRS_CHECKED( XPRScreateprob, ( &p ) );
    XPRS_CHECKED( XPRSgetintcontrol, ( p, XPRS_VERSION, &retVal ) );
    XPRS_CHECKED( XPRSdestroyprob, ( p ) );
    decrementInstanceCounter();
*/

    return XPVERSION;

}

//-----------------------------------------------------------------------------

FILE * OsiXprSolverInterface::getLogFilePtr()
{
  if (logFilePtr_!=NULL){fclose(logFilePtr_);logFilePtr_=NULL;}
  if (logFileName_!=NULL) logFilePtr_ = fopen(logFileName_,"a+");
  return logFilePtr_;
}

//-----------------------------------------------------------------------------

void OsiXprSolverInterface::setLogFileName( const char * filename )
{
  logFileName_ = filename;
}

//-----------------------------------------------------------------------------

int OsiXprSolverInterface::iXprCallCount_ = 1;

unsigned int OsiXprSolverInterface::numInstances_ = 0;

unsigned int OsiXprSolverInterface::osiSerial_ = 0;  

FILE * OsiXprSolverInterface::logFilePtr_ = NULL;  

const char * OsiXprSolverInterface::logFileName_ = NULL;

//#############################################################################
// Constructors, destructors clone and assignment
//#############################################################################

//-------------------------------------------------------------------
// Default Constructor 
//-------------------------------------------------------------------
OsiXprSolverInterface::OsiXprSolverInterface (int newrows, int newnz) :
OsiSolverInterface(),
prob_(NULL),
matrixByRow_(NULL),
matrixByCol_(NULL),
colupper_(NULL),
collower_(NULL),
rowupper_(NULL),
rowlower_(NULL),
rowsense_(NULL),
rhs_(NULL),
rowrange_(NULL),
objcoeffs_(NULL),
objsense_(1),
colsol_(NULL),
rowsol_(NULL),
rowact_(NULL),
rowprice_(NULL),
colprice_(NULL),
ivarind_(NULL),
ivartype_(NULL),
vartype_(NULL),
domipstart(false)
{
    incrementInstanceCounter();
    
    xprProbname_ = "";

    gutsOfConstructor();
  
  // newrows and newnz specify room to leave in the solver's matrix
  // structure for cuts.  Note that this is a *global* parameter.
  // The value in effect when the problem is loaded pertains.
  
  if ( newrows > 0 && newnz > 0 ) {         
      XPRS_CHECKED( XPRSsetintcontrol,(prob_,XPRS_EXTRAROWS, newrows) );
      XPRS_CHECKED( XPRSsetintcontrol, (prob_,XPRS_EXTRAELEMS, newnz) );
  }
}

//----------------------------------------------------------------
// Clone
//----------------------------------------------------------------
OsiSolverInterface *
OsiXprSolverInterface::clone(bool copyData) const
{  
   return (new OsiXprSolverInterface(*this));
}

//-------------------------------------------------------------------
// Copy constructor 
//-------------------------------------------------------------------
OsiXprSolverInterface::
OsiXprSolverInterface (const OsiXprSolverInterface & source) :
    OsiSolverInterface(source),
    prob_(NULL),
   matrixByRow_(NULL),
   matrixByCol_(NULL),
   colupper_(NULL),
   collower_(NULL),
   rowupper_(NULL),
   rowlower_(NULL),
   rowsense_(NULL),
   rhs_(NULL),
   rowrange_(NULL),
   objcoeffs_(NULL),
   objsense_(source.objsense_),
   colsol_(NULL),
   rowsol_(NULL),
   rowact_(NULL),
   rowprice_(NULL),
   colprice_(NULL),
   ivarind_(NULL),
   ivartype_(NULL),
   vartype_(NULL),
   domipstart(false)
{
    incrementInstanceCounter();
    xprProbname_ = "";
    gutsOfConstructor();
    
    gutsOfCopy(source);
    
    // Other values remain NULL until requested
}

//-------------------------------------------------------------------
// Destructor 
//-------------------------------------------------------------------
OsiXprSolverInterface::~OsiXprSolverInterface ()
{
   gutsOfDestructor();
   XPRSdestroyprob(prob_);
   decrementInstanceCounter();
}

//-------------------------------------------------------------------
// Assignment operator 
//-------------------------------------------------------------------
OsiXprSolverInterface &
OsiXprSolverInterface::operator=(const OsiXprSolverInterface& rhs)
{
   if ( this != &rhs ) {    
      OsiSolverInterface::operator=(rhs);
      osiSerial_++;       // even though numInstances_ doesn't change
      gutsOfDestructor();
      gutsOfCopy(rhs);
   }
   return *this;
}

//#############################################################################
// Applying cuts
//#############################################################################

void
OsiXprSolverInterface::applyColCut( const OsiColCut & cc )
{
   const double  *collower = getColLower();
   const double  *colupper = getColUpper();
   const CoinPackedVector & lbs = cc.lbs();
   const CoinPackedVector & ubs = cc.ubs();

   int     *index = new int   [lbs.getNumElements() + ubs.getNumElements()];
   char    *btype = new char  [lbs.getNumElements() + ubs.getNumElements()];
   double  *value = new double[lbs.getNumElements() + ubs.getNumElements()];

   int     i, nbds;

   for ( i = nbds = 0;  i < lbs.getNumElements();  i++, nbds++ ) {
      index[nbds] = lbs.getIndices()[i];
      btype[nbds] = 'L';
      value[nbds] = (collower[index[nbds]] > lbs.getElements()[i]) ?
	 collower[index[nbds]] : lbs.getElements()[i];
   }

   for ( i = 0;  i < ubs.getNumElements();  i++, nbds++ ) {
      index[nbds] = ubs.getIndices()[i];
      btype[nbds] = 'U';
      value[nbds] = (colupper[index[nbds]] < ubs.getElements()[i]) ?
	 colupper[index[nbds]] : ubs.getElements()[i];
   }
   
    if (getLogFilePtr()!=NULL) {
      fprintf(getLogFilePtr(),"{\n");
      fprintf(getLogFilePtr(),"   int index[%d];\n",nbds);
      fprintf(getLogFilePtr(),"   char btype[%d];\n",nbds);
      fprintf(getLogFilePtr(),"   double value[%d];\n",nbds);
      for ( i=0; i<nbds; i++ ) {
        fprintf(getLogFilePtr(),"   index[%d]=%d;\n",i,index[i]);
        fprintf(getLogFilePtr(),"   btype[%d]='%c';\n",i,btype[i]);
        fprintf(getLogFilePtr(),"   value[%d]=%f;\n",i,value[i]);
      }
      fprintf(getLogFilePtr(),"}\n");
    }

    XPRS_CHECKED( XPRSchgbounds, (prob_,nbds, index, btype, value) );

   delete [] index;
   delete [] btype;
   delete [] value;

   freeCachedResults();
   //  delete [] colupper_;      colupper_ = NULL;
   //  delete [] collower_;      collower_ = NULL;
}

//-----------------------------------------------------------------------------

void
OsiXprSolverInterface::applyRowCut( const OsiRowCut & rowCut )
{
   const   CoinPackedVector & row=rowCut.row();
   int     start[2] = {0, row.getNumElements()};
   char    sense = rowCut.sense();
   double  rhs   = rowCut.rhs();

   double  r = rowCut.range();

   if (getLogFilePtr()!=NULL) {
       int i;
       fprintf(getLogFilePtr(),"{\n");
       fprintf(getLogFilePtr(),"  char sense = '%c';\n",sense);
       fprintf(getLogFilePtr(),"  double rhs = '%f';\n",rhs);
       fprintf(getLogFilePtr(),"  double r = '%f';\n",r);
       fprintf(getLogFilePtr(),"  int start[2] = {0,%d};\n",start[1]);
       fprintf(getLogFilePtr(),"  int indices[%d];\n",row.getNumElements());
       fprintf(getLogFilePtr(),"  double elements[%d];\n",row.getNumElements());
       for ( i=0; i<row.getNumElements(); i++ ) {
	   fprintf(getLogFilePtr(),"  indices[%d]=%d;\n",i,row.getIndices()[i]);
	   fprintf(getLogFilePtr(),"  elements[%d]=%f;\n",i,row.getElements()[i]);
       }
       fprintf(getLogFilePtr(),"\n");
   }

   // In XPRESS XPRSaddrows(prob_) prototype, indices and elements should be const, but
   // they're not. 
   int rc;

   rc = XPRSaddrows(prob_,1, row.getNumElements(), &sense, &rhs, &r,
			start, const_cast<int *>(row.getIndices()),
			const_cast<double *>(row.getElements())); 
   assert( rc == 0 );
   
   freeCachedResults();
}

//#############################################################################
// Private methods
//#############################################################################

void
OsiXprSolverInterface::gutsOfCopy( const OsiXprSolverInterface & source )
{
   if ( source.xprProbname_ != "" ) {    // source has data
     std::ostringstream pname;
     pname << xprProbname_ << "#" << osiSerial_ <<'\0';
     xprProbname_ = pname.str();
     //     sprintf(xprProbname_, "%s#%d", source.xprProbname_, osiSerial_);
     
     //     std::cout << "Problem " << xprProbname_ << " copied to matrix ";
     

     XPRS_CHECKED( XPRScopyprob, ( prob_, source.prob_, xprProbname_.c_str() ) );
     XPRS_CHECKED( XPRScopycontrols, ( prob_, source.prob_ ) );
     /* if the callbacks are used, a XPRScopycallbacks needs to be added */
   }
}


//-------------------------------------------------------------------
void
OsiXprSolverInterface::gutsOfConstructor()
{
    
    XPRS_CHECKED( XPRScreateprob, (&prob_) );  

    XPRS_CHECKED( XPRSsetcbmessage, (prob_, OsiXprMessageCallback, messageHandler()) );

    /* always create an empty problem to initialize all data structures
     * since the user had no chance to pass in his own message handler, we turn off the output for the following operation
     */

    int outputlog;
    XPRS_CHECKED( XPRSgetintcontrol, (prob_, XPRS_OUTPUTLOG, &outputlog) );
    XPRS_CHECKED( XPRSsetintcontrol, (prob_, XPRS_OUTPUTLOG, 0) );
    
    int istart = 0;
    XPRS_CHECKED( XPRSloadlp, ( prob_, "empty", 0, 0, NULL, NULL, NULL, NULL,
				&istart,
				NULL, NULL, NULL, NULL, NULL) );
    XPRS_CHECKED( XPRSchgobjsense, (prob_, (int)objsense_) );

    XPRS_CHECKED( XPRSsetintcontrol, (prob_, XPRS_OUTPUTLOG, outputlog) );

    //OPEN: only switched off for debugging
    //XPRS_CHECKED( XPRSsetlogfile, (prob_,"xpress.log") );
    // **FIXME** (mjs) Integer problems require solution file or callback to save 
    // optimal solution.  The quick fix is to leave the default solutionfile setting,
    // but implementing a callback would be better.
    //    XPRS_CHECKED( XPRSsetintcontrol, (prob_, XPRS_SOLUTIONFILE,0) );
    
    //    XPRS_CHECKED( XPRSsetintcontrol, ( prob_, XPRS_PRESOLVE, 0) );

    lastsolvewasmip = false;

}
//-------------------------------------------------------------------
void
OsiXprSolverInterface::gutsOfDestructor()
{
   freeCachedResults();

   assert(matrixByRow_ == NULL);
   assert(matrixByCol_ == NULL);
   assert(colupper_    == NULL);
   assert(collower_    == NULL);
   assert(rowupper_    == NULL);
   assert(rowlower_    == NULL);
                    
   assert(rowsense_    == NULL);
   assert(rhs_         == NULL);
   assert(rowrange_    == NULL);
                    
   assert(objcoeffs_   == NULL);
                    
   assert(colsol_      == NULL);
   assert(rowsol_      == NULL);
   assert(rowprice_    == NULL);
   assert(colprice_    == NULL);
   assert(rowact_      == NULL);
   assert(vartype_     == NULL);

}

//-------------------------------------------------------------------

void
OsiXprSolverInterface::freeSolution()
{
   delete [] colsol_;	    colsol_      = NULL;
   delete [] rowsol_;       rowsol_      = NULL;
   delete [] rowact_;       rowact_      = NULL;
   delete [] rowprice_;	    rowprice_    = NULL;
   delete [] colprice_;	    colprice_    = NULL;
}

//-------------------------------------------------------------------

void
OsiXprSolverInterface::freeCachedResults()
{
   delete matrixByRow_;     matrixByRow_ = NULL;
   delete matrixByCol_;     matrixByCol_ = NULL;
   delete [] colupper_;     colupper_    = NULL;
   delete [] collower_;	    collower_    = NULL;
   delete [] rowupper_;	    rowupper_    = NULL;
   delete [] rowlower_;	    rowlower_    = NULL;

   delete [] rowsense_;	    rowsense_    = NULL;
   delete [] rhs_;	    rhs_         = NULL;
   delete [] rowrange_;	    rowrange_    = NULL;

   delete [] objcoeffs_;    objcoeffs_   = NULL;

   freeSolution();

   delete [] ivarind_;      ivarind_     = NULL;
   delete [] ivartype_;     ivartype_    = NULL;
   delete [] vartype_;      vartype_     = NULL;
}

//-------------------------------------------------------------------
// Set up lists of integer variables
//-------------------------------------------------------------------
int
OsiXprSolverInterface::getNumIntVars() const
{
  int     nintvars = 0, nsets = 0;
  
  if ( isDataLoaded() ) {  
    
      XPRS_CHECKED( XPRSgetglobal, (prob_,&nintvars, &nsets, 
      NULL, NULL, NULL, NULL, NULL, NULL, NULL) );
  }
  
  return nintvars;
}

void
OsiXprSolverInterface::getVarTypes() const
{
   int     nintvars = getNumIntVars();
   int     nsets;
   int     ncols = getNumCols();

   if ( vartype_ == NULL && nintvars > 0 ) {
   
    if (getLogFilePtr()!=NULL) {
      fprintf(getLogFilePtr(),"{\n");
      fprintf(getLogFilePtr(),"   int nintvars = %d;\n",nintvars);
      fprintf(getLogFilePtr(),"   int nsets;\n");
      fprintf(getLogFilePtr(),"   char ivartype[%d];\n",nintvars);
      fprintf(getLogFilePtr(),"   char ivarind[%d];\n",nintvars);
      fprintf(getLogFilePtr(),"}\n");
    }

      ivartype_ = new char[nintvars];
      ivarind_  = new int[nintvars];

      XPRS_CHECKED( XPRSgetglobal, (prob_,&nintvars, &nsets, 
		ivartype_, ivarind_, NULL, NULL, NULL, NULL, NULL) );
      // Currently, only binary and integer vars are supported.

      vartype_  = new char[ncols];

      int     i, j;
      for ( i = j = 0;  j < ncols;  j++ ) {
	 if ( i < nintvars && j == ivarind_[i] ) {
	    vartype_[j] = ivartype_[i];
	    i++;
	 } else 
	    vartype_[j] = 'C';
      }
   }
}


bool
OsiXprSolverInterface::isDataLoaded() const
{
  int istatus;

  XPRS_CHECKED( XPRSgetintattrib, ( prob_, XPRS_MIPSTATUS, &istatus ) );
  istatus &= XPRS_MIP_NOT_LOADED;

  return istatus == 0;

}

//#############################################################################
