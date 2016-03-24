/* $Id: CoinPostsolveMatrix.cpp 1373 2011-01-03 23:57:44Z lou $ */
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include <stdio.h>

#include <cassert>
#include <iostream>

#include "CoinHelperFunctions.hpp"
#include "CoinPresolveMatrix.hpp"

#if PRESOLVE_DEBUG || PRESOLVE_CONSISTENCY
#include "CoinPresolvePsdebug.hpp"
#endif

/*! \file

  This file contains methods for CoinPostsolveMatrix, the object used during
  postsolve transformations.
*/

/*
  Constructor and destructor for CoinPostsolveMatrix.
*/

/*
  Default constructor

  Postpone allocation of space until we actually load the object.
*/

CoinPostsolveMatrix::CoinPostsolveMatrix
  (int ncols_alloc, int nrows_alloc, CoinBigIndex nelems_alloc)

  : CoinPrePostsolveMatrix(ncols_alloc,nrows_alloc,nelems_alloc),
    free_list_(0),
    maxlink_(nelems_alloc),
    link_(0),
    cdone_(0),
    rdone_(0)

{ /* nothing to do here */ 

  return ; }

/*
  Destructor
*/

CoinPostsolveMatrix::~CoinPostsolveMatrix()

{ delete[] link_ ;

  delete[] cdone_ ;
  delete[] rdone_ ;

  return ; }

/*
  This routine loads a CoinPostsolveMatrix object from a CoinPresolveMatrix
  object. The CoinPresolveMatrix object will be stripped, its components
  transferred to the CoinPostsolveMatrix object, and the empty shell of the
  CoinPresolveObject will be destroyed.

  The routine expects an empty CoinPostsolveMatrix object, and will leak
  any memory already allocated.
*/

void
CoinPostsolveMatrix::assignPresolveToPostsolve (CoinPresolveMatrix *&preObj)

{
/*
  Start with simple data --- allocated and current size.
*/
  ncols0_ = preObj->ncols0_ ;
  nrows0_ = preObj->nrows0_ ;
  nelems0_ = preObj->nelems0_ ;
  bulk0_ = preObj->bulk0_ ;

  ncols_ = preObj->ncols_ ;
  nrows_ = preObj->nrows_ ;
  nelems_ = preObj->nelems_ ;
/*
  Now bring over the column-major matrix and other problem data.
*/
  mcstrt_ = preObj->mcstrt_ ;
  preObj->mcstrt_ = 0 ;
  hincol_ = preObj->hincol_ ;
  preObj->hincol_ = 0 ;
  hrow_ = preObj->hrow_ ;
  preObj->hrow_ = 0 ;
  colels_ = preObj->colels_ ;
  preObj->colels_ = 0 ;

  cost_ = preObj->cost_ ;
  preObj->cost_ = 0 ;
  originalOffset_ = preObj->originalOffset_ ;
  clo_ = preObj->clo_ ;
  preObj->clo_ = 0 ;
  cup_ = preObj->cup_ ;
  preObj->cup_ = 0 ;
  rlo_ = preObj->rlo_ ;
  preObj->rlo_ = 0 ;
  rup_ = preObj->rup_ ;
  preObj->rup_ = 0 ;

  originalColumn_ = preObj->originalColumn_ ;
  preObj->originalColumn_ = 0 ;
  originalRow_ = preObj->originalRow_ ;
  preObj->originalRow_ = 0 ;

  ztolzb_ = preObj->ztolzb_ ;
  ztoldj_ = preObj->ztoldj_ ;
  maxmin_ = preObj->maxmin_ ;
/*
  Now the problem solution. Often this will be empty, but that's not a problem.
*/
  sol_ = preObj->sol_ ;
  preObj->sol_ = 0 ;
  rowduals_ = preObj->rowduals_ ;
  preObj->rowduals_ = 0 ;
  acts_ = preObj->acts_ ;
  preObj->acts_ = 0 ;
  rcosts_ = preObj->rcosts_ ;
  preObj->rcosts_ = 0 ;
  colstat_ = preObj->colstat_ ;
  preObj->colstat_ = 0 ;
  rowstat_ = preObj->rowstat_ ;
  preObj->rowstat_ = 0 ;
/*
  The CoinPostsolveMatrix comes with messages and a handler, but replace them
  with the versions from the CoinPresolveObject, in case they've been
  customized. Let preObj believe it's no longer responsible for the handler.
*/
  if (defaultHandler_ == true)
    delete handler_ ;
  handler_ = preObj->handler_ ;
  preObj->defaultHandler_ = false ;
  messages_ = preObj->messages_ ;
/*
  Initialise the postsolve portions of this object. Which amounts to setting
  up the thread links to match the column-major matrix representation. This
  would be trivial except that the presolve matrix is loosely packed. We can
  either compress the matrix, or record the existing free space pattern. Bet
  that the latter is more efficient. Remember that mcstrt_[ncols_] actually
  points to the end of the bulk storage area, so when we process the last
  column in the bulk storage area, we'll add the free space block at the end
  of bulk storage to the free list.

  We need to allow for a 0x0 matrix here --- a pathological case, but it slips
  in when (for example) confirming a solution in an ILP code.
*/
  free_list_ = NO_LINK ;
  maxlink_ = bulk0_ ;
  link_ = new CoinBigIndex [maxlink_] ;

  if (ncols_ > 0)
  { CoinBigIndex minkcs = -1 ;
    for (int j = 0 ; j < ncols_ ; j++)
    { CoinBigIndex kcs = mcstrt_[j] ;
      int lenj = hincol_[j] ;
      assert(lenj > 0) ;
      CoinBigIndex kce = kcs+lenj-1 ;
      CoinBigIndex k ;

      for (k = kcs ; k < kce ; k++)
      { link_[k] = k+1 ; }
      link_[k++] = NO_LINK ;

      if (preObj->clink_[j].pre == NO_LINK)
      { minkcs = kcs ; }
      int nxtj = preObj->clink_[j].suc ;
      assert(nxtj >= 0 && nxtj <= ncols_) ;
      CoinBigIndex nxtcs = mcstrt_[nxtj] ;
      for ( ; k < nxtcs ; k++)
      { link_[k] = free_list_ ;
	free_list_ = k ; } }

    assert(minkcs >= 0) ;
    if (minkcs > 0)
    { for (CoinBigIndex k = 0 ; k < minkcs ; k++)
      { link_[k] = free_list_ ;
	free_list_ = k ; } } }
  else
  { for (CoinBigIndex k = 0 ; k < maxlink_ ; k++)
    { link_[k] = free_list_ ;
      free_list_ = k ; } }
/*
  That's it, preObj can die now.
*/
  delete preObj ;
  preObj = 0 ;

# if PRESOLVE_DEBUG || PRESOLVE_CONSISTENCY
/*
  These are used to track the action of postsolve transforms during debugging.
*/
  cdone_ = new char [ncols0_] ;
  CoinFillN(cdone_,ncols_,PRESENT_IN_REDUCED) ;
  CoinZeroN(cdone_+ncols_,ncols0_-ncols_) ;
  rdone_ = new char [nrows0_] ;
  CoinFillN(rdone_,nrows_,PRESENT_IN_REDUCED) ;
  CoinZeroN(rdone_+nrows_,nrows0_-nrows_) ;
# else
  cdone_ = 0 ;
  rdone_ = 0 ;
# endif

# if PRESOLVE_CONSISTENCY
  presolve_check_free_list(this,true) ;
  presolve_check_threads(this) ;
# endif

  return ; }
