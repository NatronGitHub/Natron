/* $Id: CoinPresolveMatrix.cpp 1510 2011-12-08 23:56:01Z lou $ */
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include <stdio.h>

#include <cassert>
#include <iostream>

#include "CoinHelperFunctions.hpp"
#include "CoinPresolveMatrix.hpp"
#include "CoinTime.hpp"

/*! \file

  This file contains methods for CoinPresolveMatrix, the object used during
  presolve transformations.
*/

/*
  Constructor and destructor for CoinPresolveMatrix.
*/

/*
  CoinPresolveMatrix constructor

  The constructor does very little, for much the same reasons that the
  CoinPrePostsolveMatrix constructor does little. Might as well wait until we
  load a matrix.

  In general, for presolve the allocated size can be equal to the size of the
  constraint matrix before presolve transforms are applied. (Presolve
  transforms are assumed to reduce the size of the constraint system.) But we
  need to keep the *_alloc parameters for compatibility with
  CoinPrePostsolveMatrix.
*/

CoinPresolveMatrix::CoinPresolveMatrix
  (int ncols_alloc, int nrows_alloc, CoinBigIndex nelems_alloc)

  : CoinPrePostsolveMatrix(ncols_alloc,nrows_alloc,nelems_alloc),

    clink_(0),
    rlink_(0),

    dobias_(0.0),
    mrstrt_(0),
    hinrow_(0),
    rowels_(0),
    hcol_(0),

    integerType_(0),
    anyInteger_(false),
    tuning_(false),
    startTime_(0.0),
    feasibilityTolerance_(0.0),
    status_(-1),
    pass_(0),
    maxSubstLevel_(3),
    colChanged_(0),
    colsToDo_(0),
    numberColsToDo_(0),
    nextColsToDo_(0),
    numberNextColsToDo_(0),

    rowChanged_(0),
    rowsToDo_(0),
    numberRowsToDo_(0),
    nextRowsToDo_(0),
    numberNextRowsToDo_(0),
    presolveOptions_(0),
    anyProhibited_(false),
    usefulRowInt_(NULL),
    usefulRowDouble_(NULL),
    usefulColumnInt_(NULL),
    usefulColumnDouble_(NULL),
    randomNumber_(NULL),
    infiniteUp_(NULL),
    sumUp_(NULL),
    infiniteDown_(NULL),
    sumDown_(NULL)

{ /* nothing to do here */ 

  return ; }

/*
  CoinPresolveMatrix destructor.
*/

CoinPresolveMatrix::~CoinPresolveMatrix()

{ delete[] clink_ ;
  delete[] rlink_ ;
  
  delete[] mrstrt_ ;
  delete[] hinrow_ ;
  delete[] rowels_ ;
  delete[] hcol_ ;

  delete[] integerType_ ;
  delete[] rowChanged_ ;
  delete[] rowsToDo_ ;
  delete[] nextRowsToDo_ ;
  delete[] colChanged_ ;
  delete[] colsToDo_ ;
  delete[] nextColsToDo_ ;
  delete[] usefulRowInt_;
  delete[] usefulRowDouble_;
  delete[] usefulColumnInt_;
  delete[] usefulColumnDouble_;
  delete[] randomNumber_;
  delete[] infiniteUp_;
  delete[] sumUp_;
  delete[] infiniteDown_;
  delete[] sumDown_;

  return ; }



/*
  This routine loads a CoinPackedMatrix and proceeds to do the bulk of the
  initialisation for the PrePostsolve and Presolve objects.

  In the CoinPrePostsolveMatrix portion of the object, it initialises the
  column-major packed matrix representation and the arrays that track the
  motion of original columns and rows.

  In the CoinPresolveMatrix portion of the object, it initialises the
  row-major packed matrix representation, the arrays that assist in matrix
  storage management, and the arrays that track the rows and columns to be
  processed.

  Arrays are allocated to the requested size (ncols0_, nrow0_, nelems0_).

  The source matrix must be column ordered; it does not need to be gap-free.
  Bulk storage in the column-major (hrow_, colels_) and row-major (hcol_,
  rowels_) matrices is allocated at twice the required size so that we can
  expand columns and rows as needed. This is almost certainly grossly
  oversize, but (1) it's efficient, and (2) the utility routines which
  compact the bulk storage areas have no provision to reallocate.
*/

void CoinPresolveMatrix::setMatrix (const CoinPackedMatrix *mtx)

{
/*
  Check to make sure the matrix will fit and is column ordered.
*/
  if (mtx->isColOrdered() == false)
  { throw CoinError("source matrix must be column ordered",
		    "setMatrix","CoinPrePostsolveMatrix") ; }

  int numCols = mtx->getNumCols() ;
  if (numCols > ncols0_)
  { throw CoinError("source matrix exceeds allocated capacity",
		    "setMatrix","CoinPrePostsolveMatrix") ; }
/*
  Acquire the actual size, but allocate the matrix storage to the
  requested capacity. The column-major rep is part of the PrePostsolve
  object, the row-major rep belongs to the Presolve object.
*/
  ncols_ = numCols ;
  nrows_ = mtx->getNumRows() ;
  nelems_ = mtx->getNumElements() ;
  bulk0_ = static_cast<CoinBigIndex> (bulkRatio_*nelems0_) ;

  if (mcstrt_ == 0) mcstrt_ = new CoinBigIndex [ncols0_+1] ;
  if (hincol_ == 0) hincol_ = new int [ncols0_+1] ;
  if (hrow_ == 0) hrow_ = new int [bulk0_] ;
  if (colels_ == 0) colels_ = new double [bulk0_] ;

  if (mrstrt_ == 0) mrstrt_ = new CoinBigIndex [nrows0_+1] ;
  if (hinrow_ == 0) hinrow_ = new int [nrows0_+1] ;
  if (hcol_ == 0) hcol_ = new int [bulk0_] ;
  if (rowels_ == 0) rowels_ = new double [bulk0_] ;
/*
  Grab the corresponding vectors from the source matrix.
*/
  const CoinBigIndex *src_mcstrt = mtx->getVectorStarts() ;
  const int *src_hincol = mtx->getVectorLengths() ;
  const double *src_colels = mtx->getElements() ;
  const int *src_hrow = mtx->getIndices() ;
/*
  Bulk copy the column starts and lengths.
*/
  CoinMemcpyN(src_mcstrt,mtx->getSizeVectorStarts(),mcstrt_) ;
  CoinMemcpyN(src_hincol,mtx->getSizeVectorLengths(),hincol_) ;
/*
  Copy the coefficients column by column in case there are gaps between
  the columns in the bulk storage area. The assert is just in case the
  gaps are *really* big.
*/
  assert(src_mcstrt[ncols_] <= bulk0_) ;
  int j;
  for ( j = 0 ; j < numCols ; j++)
  { int lenj = src_hincol[j] ;
    CoinBigIndex offset = mcstrt_[j] ;
    CoinMemcpyN(src_colels+offset,lenj,colels_+offset) ;
    CoinMemcpyN(src_hrow+offset,lenj,hrow_+offset) ; }
/*
  Now make a row-major copy. Start by counting the number of coefficients in
  each row; we can do this directly in hinrow. Given the number of
  coefficients in a row, we know how to lay out the bulk storage area.
*/
  CoinZeroN(hinrow_,nrows0_+1) ;
  for ( j = 0 ; j < ncols_ ; j++)
  { int *rowIndices = hrow_+mcstrt_[j] ;
    int lenj = hincol_[j] ;
    for (int k = 0 ; k < lenj ; k++)
    { int i = rowIndices[k] ;
      hinrow_[i]++ ; } }
/*
  Initialize mrstrt[i] to the start of row i+1. As we drop each coefficient
  and column index into the bulk storage arrays, we'll decrement and store.
  When we're done, mrstrt[i] will point to the start of row i, as it should.
*/
  int totalCoeffs = 0 ;
  int i;
  for ( i = 0 ; i < nrows_ ; i++)
  { totalCoeffs += hinrow_[i] ;
    mrstrt_[i] = totalCoeffs ; }
  mrstrt_[nrows_] = totalCoeffs ;
  for ( j = ncols_-1 ; j >= 0 ; j--)
  { int lenj = hincol_[j] ;
    double *colCoeffs = colels_+mcstrt_[j] ;
    int *rowIndices = hrow_+mcstrt_[j] ;
    for (int k = 0 ; k < lenj ; k++)
    { int ri;
      ri = rowIndices[k] ;
      double aij = colCoeffs[k] ;
      CoinBigIndex l = --mrstrt_[ri] ;
      rowels_[l] = aij ;
      hcol_[l] = j ; } }
/*
  Now the support structures. The entry for original column j should start
  out as j; similarly for row i. originalColumn_ and originalRow_ belong to
  the PrePostsolve object.
*/
  if (originalColumn_ == 0) originalColumn_ = new int [ncols0_] ;
  if (originalRow_ == 0) originalRow_ = new int [nrows0_] ;

  for ( j = 0 ; j < ncols0_ ; j++) 
    originalColumn_[j] = j ;
  for ( i = 0 ; i < nrows0_ ; i++) 
    originalRow_[i] = i ;
/*
  We have help to set up the clink_ and rlink_ vectors (aids for matrix bulk
  storage management). clink_ and rlink_ belong to the Presolve object.  Once
  this is done, it's safe to set mrstrt_[nrows_] and mcstrt_[ncols_] to the
  full size of the bulk storage area.
*/
  if (clink_ == 0) clink_ = new presolvehlink [ncols0_+1] ;
  if (rlink_ == 0) rlink_ = new presolvehlink [nrows0_+1] ;
  presolve_make_memlists(/*mcstrt_,*/hincol_,clink_,ncols_) ;
  presolve_make_memlists(/*mrstrt_,*/hinrow_,rlink_,nrows_) ;
  mcstrt_[ncols_] = bulk0_ ;
  mrstrt_[nrows_] = bulk0_ ;
/*
  No rows or columns have been changed just yet. colChanged_ and rowChanged_
  belong to the Presolve object.
*/
  if (colChanged_ == 0) colChanged_ = new unsigned char [ncols0_] ;
  CoinZeroN(colChanged_,ncols0_) ;
  if (rowChanged_ == 0) rowChanged_ = new unsigned char [nrows0_] ;
  CoinZeroN(rowChanged_,nrows0_) ;
/*
  Finally, allocate the various *ToDo arrays. These are used to track the rows
  and columns which should be processed in a given round of presolve
  transforms. These belong to the Presolve object. Setting number*ToDo to 0
  is all the initialization that's required here.
*/
  rowsToDo_ = new int [nrows0_] ;
  numberRowsToDo_ = 0 ;
  nextRowsToDo_ = new int [nrows0_] ;
  numberNextRowsToDo_ = 0 ;
  colsToDo_ = new int [ncols0_] ;
  numberColsToDo_ = 0 ;
  nextColsToDo_ = new int [ncols0_] ;
  numberNextColsToDo_ = 0 ;
  initializeStuff();
  return ; }

/*
  Recompute ups and downs for a row (nonzero if infeasible).

  If oneRow == -1 then do all rows.
*/
int CoinPresolveMatrix::recomputeSums (int oneRow)
{
  const int &numberRows = nrows_ ;
  const int &numberColumns = ncols_ ;
  
  const double *const columnLower = clo_ ;
  const double *const columnUpper = cup_ ;

  double *const rowLower = rlo_ ;
  double *const rowUpper = rup_ ;

  const double *element = rowels_ ;
  const int *column = hcol_ ;
  const CoinBigIndex *rowStart = mrstrt_ ;
  const int *rowLength = hinrow_ ;

  const double large = PRESOLVE_SMALL_INF ;
  const double &tolerance = feasibilityTolerance_ ;

  const int iFirst = ((oneRow >= 0)?oneRow:0) ;
  const int iLast = ((oneRow >= 0)?oneRow:numberRows) ;
/*
  Open a loop to process rows of interest.
*/
  int infeasible = 0 ;
  for (int iRow = iFirst ; iRow < iLast ; iRow++) {
    infiniteUp_[iRow] = 0 ;
    sumUp_[iRow] = 0.0 ;
    infiniteDown_[iRow] = 0 ;
    sumDown_[iRow] = 0.0 ;
/*
  Compute finite and infinite contributions to row lhs upper and lower bounds
  for nonempty rows with at least one reasonable bound.
*/
    if ((rowLower[iRow] > -large || rowUpper[iRow] < large) &&
        rowLength[iRow] > 0) {
      int infiniteUpper = 0 ;
      int infiniteLower = 0 ;
      double maximumUp = 0.0 ;
      double maximumDown = 0.0 ; 
      const CoinBigIndex &rStart = rowStart[iRow] ;
      const CoinBigIndex rEnd = rStart+rowLength[iRow] ;
      for (CoinBigIndex j = rStart ; j < rEnd ; ++j) {
	const double &value = element[j] ;
	const int &iColumn = column[j] ;
	const double &lj = columnLower[iColumn] ;
	const double &uj = columnUpper[iColumn] ;
	if (value > 0.0) {
	  if (uj < large) 
	    maximumUp += uj*value ;
	  else
	    ++infiniteUpper ;
	  if (lj > -large) 
	    maximumDown += lj*value ;
	  else
	    ++infiniteLower ;
	} else if (value < 0.0) {
	  if (uj < large) 
	    maximumDown += uj*value ;
	  else
	    ++infiniteLower ;
	  if (lj > -large) 
	    maximumUp += lj*value ;
	  else
	    ++infiniteUpper ;
	}
      }
      infiniteUp_[iRow] = infiniteUpper ;
      sumUp_[iRow] = maximumUp ;
      infiniteDown_[iRow] = infiniteLower ;
      sumDown_[iRow] = maximumDown ;
      double maxUp = maximumUp+infiniteUpper*large ;
      double maxDown = maximumDown-infiniteLower*large ;
/*
  Check for redundant or infeasible row.
*/
      if (maxUp <= rowUpper[iRow]+tolerance && 
	  maxDown >= rowLower[iRow]-tolerance) {
	infiniteUp_[iRow] = numberColumns+1 ;
	infiniteDown_[iRow] = numberColumns+1 ;
      } else if (maxUp < rowLower[iRow]-tolerance) {
	infeasible++ ;
      } else if (maxDown > rowUpper[iRow]+tolerance) {
	infeasible++ ;
      }
    } else if (rowLength[iRow] > 0) {
/*
  A row where both rhs bounds are very large. Mark as redundant.
*/
      assert(rowLower[iRow] <= -large && rowUpper[iRow] >= large) ;
      infiniteUp_[iRow] = numberColumns+1 ;
      infiniteDown_[iRow] = numberColumns+1 ;
    } else {
/*
  Row with length zero. Check the the rhs bounds include zero and force
  `near-to-zero' to exactly zero.
*/
      assert(rowLength[iRow] == 0) ;
      if (rowLower[iRow] > 0.0 || rowUpper[iRow] < 0.0) {
	double tolerance2 = 10.0*tolerance ;
	if (rowLower[iRow] > 0.0 && rowLower[iRow] < tolerance2)
	  rowLower[iRow] = 0.0 ;
	else
	  infeasible++ ;
	if (rowUpper[iRow] < 0.0 && rowUpper[iRow] > -tolerance2)
	  rowUpper[iRow] = 0.0 ;
	else
	  infeasible++ ;
      }
    }
  }
  return (infeasible) ;
}
/*
  Preallocate scratch work arrays, arrays to hold row lhs bound information,
  and an array of random numbers.
*/
void CoinPresolveMatrix::initializeStuff ()
{
  usefulRowInt_ = new int [3*nrows_] ;
  usefulRowDouble_ = new double [2*nrows_] ;
  usefulColumnInt_ = new int [2*ncols_] ;
  usefulColumnDouble_ = new double[ncols_] ;
  int k = CoinMax(ncols_+1,nrows_+1) ;
  randomNumber_ = new double [k] ;
  coin_init_random_vec(randomNumber_,k) ;
  infiniteUp_ = new int [nrows_] ;
  sumUp_ = new double [nrows_] ;
  infiniteDown_ = new int [nrows_] ;
  sumDown_ = new double [nrows_] ;
  return ;
}

/*
  Free arrays allocated in initializeStuff.
*/
void CoinPresolveMatrix::deleteStuff()
{
  delete[] usefulRowInt_;
  delete[] usefulRowDouble_;
  delete[] usefulColumnInt_;
  delete[] usefulColumnDouble_;
  delete[] randomNumber_;
  delete[] infiniteUp_;
  delete[] sumUp_;
  delete[] infiniteDown_;
  delete[] sumDown_;
  usefulRowInt_ = NULL;
  usefulRowDouble_ = NULL;
  usefulColumnInt_ = NULL;
  usefulColumnDouble_ = NULL;
  randomNumber_ = NULL;
  infiniteUp_ = NULL;
  sumUp_ = NULL;
  infiniteDown_ = NULL;
  sumDown_ = NULL;
}


/*
  These functions set integer type information. The first expects an array with
  an entry for each variable. The second sets all variables to integer or
  continuous type.
*/

void CoinPresolveMatrix::setVariableType (const unsigned char *variableType,
					 int lenParam)

{ int len ;

  if (lenParam < 0)
  { len = ncols_ ; }
  else
  if (lenParam > ncols0_)
  { throw CoinError("length exceeds allocated size",
		    "setIntegerType","CoinPresolveMatrix") ; }
  else
  { len = lenParam ; }

  if (integerType_ == 0) integerType_ = new unsigned char [ncols0_] ;
  CoinCopyN(variableType,len,integerType_) ;

  return ; }

void CoinPresolveMatrix::setVariableType (bool allIntegers, int lenParam)

{ int len ;

  if (lenParam < 0)
  { len = ncols_ ; }
  else
  if (lenParam > ncols0_)
  { throw CoinError("length exceeds allocated size",
		    "setIntegerType","CoinPresolveMatrix") ; }
  else
  { len = lenParam ; }

  if (integerType_ == 0) integerType_ = new unsigned char [ncols0_] ;

  const unsigned char value = 1 ;

  if (allIntegers == true)
  { CoinFillN(integerType_,len,value) ; }
  else
  { CoinZeroN(integerType_,len) ; }

  return ; }

/*
  The next pair of routines initialises the [row,col]ToDo lists in preparation
  for a major pass. All except rows/columns marked as prohibited are added to
  the lists.
*/

void CoinPresolveMatrix::initColsToDo ()
/*
  Initialize the ToDo lists in preparation for a major iteration of
  preprocessing. First, cut back the ToDo and NextToDo lists to zero entries.
  Then place all columns not marked prohibited on the ToDo list.
*/

{ int j ;

  numberNextColsToDo_ = 0 ;

  if (anyProhibited_ == false)
  { for (j = 0 ; j < ncols_ ; j++) 
    { colsToDo_[j] = j ; }
      numberColsToDo_ = ncols_ ; }
  else
  { numberColsToDo_ = 0 ;
    for (j = 0 ; j < ncols_ ; j++) 
    if (colProhibited(j) == false)
    { colsToDo_[numberColsToDo_++] = j ; } }

  return ; }

void CoinPresolveMatrix::initRowsToDo ()
/*
  Initialize the ToDo lists in preparation for a major iteration of
  preprocessing. First, cut back the ToDo and NextToDo lists to zero entries.
  Then place all rows not marked prohibited on the ToDo list.
*/

{ int i ;

  numberNextRowsToDo_ = 0 ;

  if (anyProhibited_ == false)
  { for (i = 0 ; i < nrows_ ; i++) 
    { rowsToDo_[i] = i ; }
      numberRowsToDo_ = nrows_ ; }
  else
  { numberRowsToDo_ = 0 ;
    for (i = 0 ; i < nrows_ ; i++) 
    if (rowProhibited(i) == false)
    { rowsToDo_[numberRowsToDo_++] = i ; } }

  return ; }

int CoinPresolveMatrix::stepColsToDo ()
/*
  This routine transfers the contents of NextToDo to ToDo, simultaneously
  resetting the Changed indicator. It returns the number of columns
  transfered.
*/
{ int k ;

  for (k = 0 ; k < numberNextColsToDo_ ; k++)
  { int j = nextColsToDo_[k] ;
    unsetColChanged(j) ;
    colsToDo_[k] = j ; }
  numberColsToDo_ = numberNextColsToDo_ ;
  numberNextColsToDo_ = 0 ;

  return (numberColsToDo_) ; }

int CoinPresolveMatrix::stepRowsToDo ()
/*
  This routine transfers the contents of NextToDo to ToDo, simultaneously
  resetting the Changed indicator. It returns the number of columns
  transfered.
*/
{ int k ;

  for (k = 0 ; k < numberNextRowsToDo_ ; k++)
  { int i = nextRowsToDo_[k] ;
    unsetRowChanged(i) ;
    rowsToDo_[k] = i ; }
  numberRowsToDo_ = numberNextRowsToDo_ ;
  numberNextRowsToDo_ = 0 ;

  return (numberRowsToDo_) ; }
// Say we want statistics - also set time
void 
CoinPresolveMatrix::statistics()
{
  tuning_=true;
  startTime_ = CoinCpuTime();
}
#ifdef PRESOLVE_DEBUG
#include "CoinPresolvePsdebug.cpp"
#endif
