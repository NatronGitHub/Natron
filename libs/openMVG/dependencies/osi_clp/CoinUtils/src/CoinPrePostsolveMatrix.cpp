/* $Id: CoinPrePostsolveMatrix.cpp 1516 2011-12-10 23:40:39Z lou $ */
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include <cstdio>
#include <cassert>
#include <iostream>

#include "CoinHelperFunctions.hpp"
#include "CoinPresolveMatrix.hpp"
#ifndef SLIM_CLP
#include "CoinWarmStartBasis.hpp"
#endif

/*! \file
  This file contains methods for CoinPrePostsolveMatrix, the foundation class
  for CoinPresolveMatrix and CoinPostsolveMatrix.
*/

/*
  Constructor and destructor for CoinPrePostsolveMatrix.
*/

/*
  CoinPrePostsolveMatrix constructor

  This constructor does next to nothing, because there's no sensible middle
  ground between next to nothing and a constructor with twenty parameters
  that all need to be extracted from the constraint system held by an OSI.
  The alternative, creating a constructor which takes some flavour of OSI as
  a parameter, seems to me (lh) to be wrong. That knowledge does not belong
  in the generic COIN support library.

  The philosophy here is to create an empty CoinPrePostsolveMatrix object and
  then load in the constraint matrix, vectors, and miscellaneous parameters.
  Some of this will be done from CoinPresolveMatrix or CoinPostsolveMatrix
  constructors, but in the end most of it should be pushed back to an
  OSI-specific method. Then the knowledge of how to access the required data
  in an OSI is pushed back to the individual OSI classes where it belongs.

  Thus, all vector allocation is postponed until load time.
*/

CoinPrePostsolveMatrix::CoinPrePostsolveMatrix
  (int ncols_alloc, int nrows_alloc, CoinBigIndex nelems_alloc)

  : ncols_(0),
    nrows_(0),
    nelems_(0),
    ncols0_(ncols_alloc),
    nrows0_(nrows_alloc),
    nelems0_(nelems_alloc),
    bulkRatio_(2.0),

    mcstrt_(0),
    hincol_(0),
    hrow_(0),
    colels_(0),

    cost_(0),
    originalOffset_(0),
    clo_(0),
    cup_(0),
    rlo_(0),
    rup_(0),

    originalColumn_(0),
    originalRow_(0),

    ztolzb_(0.0),
    ztoldj_(0.0),

    maxmin_(0),

    sol_(0),
    rowduals_(0),
    acts_(0),
    rcosts_(0),
    colstat_(0),
    rowstat_(0),

    handler_(0),
    defaultHandler_(false),
    messages_()

{ handler_ = new CoinMessageHandler() ;
  defaultHandler_ = true ;
  bulk0_ = static_cast<CoinBigIndex> (bulkRatio_*nelems_alloc);

  return ; }

/*
  CoinPrePostsolveMatrix destructor
*/

CoinPrePostsolveMatrix::~CoinPrePostsolveMatrix()
{
  delete[] sol_ ;
  delete[] rowduals_ ;
  delete[] acts_ ;
  delete[] rcosts_ ;

/*
  Note that we do NOT delete rowstat_. This is to maintain compatibility with
  ClpPresolve and OsiPresolve, which allocate a single vector and split it
  between column and row status.
*/
  delete[] colstat_ ;

  delete[] cost_ ;
  delete[] clo_ ;
  delete[] cup_ ;
  delete[] rlo_ ;
  delete[] rup_ ;

  delete[] mcstrt_ ;
  delete[] hrow_ ;
  delete[] colels_ ;
  delete[] hincol_ ;

  delete[] originalColumn_ ;
  delete[] originalRow_ ;

  if (defaultHandler_ == true)
    delete handler_ ;
}


#ifndef SLIM_CLP
/*
  Methods to set the miscellaneous parameters: max/min, objective offset, and
  tolerances.
*/

void CoinPrePostsolveMatrix::setObjOffset (double offset)

{ originalOffset_ = offset ; }

void CoinPrePostsolveMatrix::setObjSense (double objSense)

{ maxmin_ = objSense ; }

void CoinPrePostsolveMatrix::setPrimalTolerance (double primTol)

{ ztolzb_ = primTol ; }

void CoinPrePostsolveMatrix::setDualTolerance (double dualTol)

{ ztoldj_ = dualTol ; }



/*
  Methods to set the various vectors. For all methods, lenParam can be
  omitted and will default to -1. In that case, the default action is to copy
  ncols_ or nrows_ entries, as appropriate.

  It is *not* considered an error to specify lenParam = 0! This allows for
  allocation of vectors in an intially empty system.  Note that ncols_ and
  nrows_ will be 0 before a constraint system is loaded. Be careful what you
  ask for.

  The vector allocated in the CoinPrePostsolveMatrix will be of size ncols0_
  or nrows0_, as appropriate.
*/

void CoinPrePostsolveMatrix::setColLower (const double *colLower, int lenParam)

{ int len ;

  if (lenParam < 0)
  { len = ncols_ ; }
  else
  if (lenParam > ncols0_)
  { throw CoinError("length exceeds allocated size",
		    "setColLower","CoinPrePostsolveMatrix") ; }
  else
  { len = lenParam ; }

  if (clo_ == 0) clo_ = new double[ncols0_] ;
  CoinMemcpyN(colLower,len,clo_) ;

  return ; }
  
void CoinPrePostsolveMatrix::setColUpper (const double *colUpper, int lenParam)

{ int len ;

  if (lenParam < 0)
  { len = ncols_ ; }
  else
  if (lenParam > ncols0_)
  { throw CoinError("length exceeds allocated size",
		    "setColUpper","CoinPrePostsolveMatrix") ; }
  else
  { len = lenParam ; }

  if (cup_ == 0) cup_ = new double[ncols0_] ;
  CoinMemcpyN(colUpper,len,cup_) ;

  return ; }
  
void CoinPrePostsolveMatrix::setColSolution (const double *colSol,
					     int lenParam)

{ int len ;

  if (lenParam < 0)
  { len = ncols_ ; }
  else
  if (lenParam > ncols0_)
  { throw CoinError("length exceeds allocated size",
		    "setColSolution","CoinPrePostsolveMatrix") ; }
  else
  { len = lenParam ; }

  if (sol_ == 0) sol_ = new double[ncols0_] ;
  CoinMemcpyN(colSol,len,sol_) ;

  return ; }
  
void CoinPrePostsolveMatrix::setCost (const double *cost, int lenParam)

{ int len ;

  if (lenParam < 0)
  { len = ncols_ ; }
  else
  if (lenParam > ncols0_)
  { throw CoinError("length exceeds allocated size",
		    "setCost","CoinPrePostsolveMatrix") ; }
  else
  { len = lenParam ; }

  if (cost_ == 0) cost_ = new double[ncols0_] ;
  CoinMemcpyN(cost,len,cost_) ;

  return ; }

void CoinPrePostsolveMatrix::setReducedCost (const double *redCost,
					     int lenParam)

{ int len ;

  if (lenParam < 0)
  { len = ncols_ ; }
  else
  if (lenParam > ncols0_)
  { throw CoinError("length exceeds allocated size",
		    "setReducedCost","CoinPrePostsolveMatrix") ; }
  else
  { len = lenParam ; }

  if (rcosts_ == 0) rcosts_ = new double[ncols0_] ;
  CoinMemcpyN(redCost,len,rcosts_) ;

  return ; }


void CoinPrePostsolveMatrix::setRowLower (const double *rowLower, int lenParam)

{ int len ;

  if (lenParam < 0)
  { len = nrows_ ; }
  else
  if (lenParam > nrows0_)
  { throw CoinError("length exceeds allocated size",
		    "setRowLower","CoinPrePostsolveMatrix") ; }
  else
  { len = lenParam ; }

  if (rlo_ == 0) rlo_ = new double[nrows0_] ;
  CoinMemcpyN(rowLower,len,rlo_) ;

  return ; }
  
void CoinPrePostsolveMatrix::setRowUpper (const double *rowUpper, int lenParam)

{ int len ;

  if (lenParam < 0)
  { len = nrows_ ; }
  else
  if (lenParam > nrows0_)
  { throw CoinError("length exceeds allocated size",
		    "setRowUpper","CoinPrePostsolveMatrix") ; }
  else
  { len = lenParam ; }

  if (rup_ == 0) rup_ = new double[nrows0_] ;
  CoinMemcpyN(rowUpper,len,rup_) ;

  return ; }
  
void CoinPrePostsolveMatrix::setRowPrice (const double *rowSol, int lenParam)

{ int len ;

  if (lenParam < 0)
  { len = nrows_ ; }
  else
  if (lenParam > nrows0_)
  { throw CoinError("length exceeds allocated size",
		    "setRowPrice","CoinPrePostsolveMatrix") ; }
  else
  { len = lenParam ; }

  if (rowduals_ == 0) rowduals_ = new double[nrows0_] ;
  CoinMemcpyN(rowSol,len,rowduals_) ;

  return ; }
  
void CoinPrePostsolveMatrix::setRowActivity (const double *rowAct, int lenParam)

{ int len ;

  if (lenParam < 0)
  { len = nrows_ ; }
  else
  if (lenParam > nrows0_)
  { throw CoinError("length exceeds allocated size",
		    "setRowActivity","CoinPrePostsolveMatrix") ; }
  else
  { len = lenParam ; }

  if (acts_ == 0) acts_ = new double[nrows0_] ;
  CoinMemcpyN(rowAct,len,acts_) ;

  return ; }



/*
  Methods to set the status vectors for a basis. Note that we need to allocate
  colstat_ and rowstat_ as a single vector, to maintain compatibility with
  OsiPresolve and ClpPresolve.

  The `using ::getStatus' declaration is required to get the compiler to
  consider the getStatus helper function defined in CoinWarmStartBasis.hpp.
*/

void CoinPrePostsolveMatrix::setStructuralStatus (const char *strucStatus,
						  int lenParam)

{ int len ;
  using ::getStatus ;

  if (lenParam < 0)
  { len = ncols_ ; }
  else
  if (lenParam > ncols0_)
  { throw CoinError("length exceeds allocated size",
		    "setStructuralStatus","CoinPrePostsolveMatrix") ; }
  else
  { len = lenParam ; }

  if (colstat_ == 0)
  { colstat_ = new unsigned char[ncols0_+nrows0_] ;
#   ifdef ZEROFAULT
    CoinZeroN(colstat_,ncols0_+nrows0_) ;
#   endif
    rowstat_ = colstat_+ncols0_ ; }
  for (int j = 0 ; j < len ; j++)
  { Status statj = Status(getStatus(strucStatus,j)) ;
    setColumnStatus(j,statj) ; }

  return ; }


void CoinPrePostsolveMatrix::setArtificialStatus (const char *artifStatus,
						  int lenParam)

{ int len ;
  using ::getStatus ;

  if (lenParam < 0)
  { len = nrows_ ; }
  else
  if (lenParam > nrows0_)
  { throw CoinError("length exceeds allocated size",
		    "setArtificialStatus","CoinPrePostsolveMatrix") ; }
  else
  { len = lenParam ; }

  if (colstat_ == 0)
  { colstat_ = new unsigned char[ncols0_+nrows0_] ;
#   ifdef ZEROFAULT
    CoinZeroN(colstat_,ncols0_+nrows0_) ;
#   endif
    rowstat_ = colstat_+ncols0_ ; }
  for (int i = 0 ; i < len ; i++)
  { Status stati = Status(getStatus(artifStatus,i)) ;
    setRowStatus(i,stati) ; }

  return ; }

/*
  This routine initialises structural and artificial status given a
  CoinWarmStartBasis as the parameter.
*/

void CoinPrePostsolveMatrix::setStatus (const CoinWarmStartBasis *basis)

{ setStructuralStatus(basis->getStructuralStatus(),
		      basis->getNumStructural()) ;
  setArtificialStatus(basis->getArtificialStatus(),
		      basis->getNumArtificial()) ;

  return ; }

/*
  This routine returns structural and artificial status in the form of a
  CoinWarmStartBasis object.

  What to do when CoinPrePostsolveMatrix::Status == superBasic? There's
  no analog in CoinWarmStartBasis::Status.
*/

CoinWarmStartBasis *CoinPrePostsolveMatrix::getStatus ()

{ int n = ncols_ ;
  int m = nrows_ ;
  CoinWarmStartBasis *wsb = new CoinWarmStartBasis() ;
  wsb->setSize(n,m) ;
  for (int j = 0 ; j < n ; j++)
  { CoinWarmStartBasis::Status statj = 
	CoinWarmStartBasis::Status(getColumnStatus(j)) ;
    wsb->setStructStatus(j,statj) ; }
  for (int i = 0 ; i < m ; i++)
  { CoinWarmStartBasis::Status stati =
	CoinWarmStartBasis::Status(getRowStatus(i)) ;
    wsb->setArtifStatus(i,stati) ; }
  
  return (wsb) ; }
#endif
/*
  Set the status of a non-basic artificial variable based on the
  variable's value and bounds.
*/

void CoinPrePostsolveMatrix::setRowStatusUsingValue (int iRow)

{ double value = acts_[iRow] ;
  double lower = rlo_[iRow] ;
  double upper = rup_[iRow] ;
  if (lower < -1.0e20 && upper > 1.0e20) {
    setRowStatus(iRow,isFree) ;
  } else if (fabs(lower-value) <= ztolzb_) {
    setRowStatus(iRow,atUpperBound) ;
  } else if (fabs(upper-value) <= ztolzb_) {
    setRowStatus(iRow,atLowerBound) ;
  } else {
    setRowStatus(iRow,superBasic) ;
  }
}

/*
  Set the status of a non-basic structural variable based on the
  variable's value and bounds.
*/

void CoinPrePostsolveMatrix::setColumnStatusUsingValue(int iColumn)
{
  double value = sol_[iColumn];
  double lower = clo_[iColumn];
  double upper = cup_[iColumn];
  if (lower<-1.0e20&&upper>1.0e20) {
    setColumnStatus(iColumn,isFree);
  } else if (fabs(lower-value)<=ztolzb_) {
    setColumnStatus(iColumn,atLowerBound);
  } else if (fabs(upper-value)<=ztolzb_) {
    setColumnStatus(iColumn,atUpperBound);
  } else {
    setColumnStatus(iColumn,superBasic);
  }
}
#ifndef SLIM_CLP


/*
  Simple routines to return a constant character string for the status value.
  Separate row and column routines for convenience, and one that just takes
  the status code.
*/

const char *CoinPrePostsolveMatrix::columnStatusString (int j) const

{ Status statj = getColumnStatus(j) ;

  switch (statj)
  { case isFree: { return ("NBFR") ; }
    case basic: { return ("B") ; }
    case atUpperBound: { return ("NBUB") ; }
    case atLowerBound: { return ("NBLB") ; }
    case superBasic: { return ("SB") ; }
    default: { return ("INVALID!") ; }
  }
}

const char *CoinPrePostsolveMatrix::rowStatusString (int j) const

{ Status statj = getRowStatus(j) ;

  switch (statj)
  { case isFree: { return ("NBFR") ; }
    case basic: { return ("B") ; }
    case atUpperBound: { return ("NBUB") ; }
    case atLowerBound: { return ("NBLB") ; }
    case superBasic: { return ("SB") ; }
    default: { return ("INVALID!") ; }
  }
}

const char *statusName (CoinPrePostsolveMatrix::Status status)
{
  switch (status) {
    case CoinPrePostsolveMatrix::isFree: { return ("NBFR") ; }
    case CoinPrePostsolveMatrix::basic: { return ("B") ; }
    case CoinPrePostsolveMatrix::atUpperBound: { return ("NBUB") ; }
    case CoinPrePostsolveMatrix::atLowerBound: { return ("NBLB") ; }
    case CoinPrePostsolveMatrix::superBasic: { return ("SB") ; }
    default: { return ("INVALID!") ; }
  }
}

#endif
