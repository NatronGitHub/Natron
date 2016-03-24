/* $Id: CoinPresolveMonitor.cpp 1585 2013-04-06 20:42:02Z stefan $ */
// Copyright (C) 2011 Lou Hafer
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include <cassert>
#include <iostream>

#include "CoinHelperFunctions.hpp"
#include "CoinPackedVector.hpp"
#include "CoinPresolveMatrix.hpp"
#include "CoinPresolveMonitor.hpp"

/*! \file

  This file contains methods for CoinPresolveMonitor, used to monitor changes
  to a row or column during presolve and postsolve
*/

/*
  Constructors
*/

/*
  Default constructor
*/
CoinPresolveMonitor::CoinPresolveMonitor ()
{ }

/*
  Constructor to initialise from a CoinPresolveMatrix (not threaded)
*/
CoinPresolveMonitor::CoinPresolveMonitor (const CoinPresolveMatrix *mtx,
					  bool isRow,
					  int k)
{
  ndx_ = k ;
  isRow_ = isRow ;

  if (isRow) {
    origVec_ = extractRow(k,mtx) ;
    const double *blow = mtx->getRowLower() ;
    lb_ = blow[k] ;
    const double *b = mtx->getRowUpper() ;
    ub_ = b[k] ;
  } else {
    origVec_ = extractCol(k,mtx) ;
    const double *lb = mtx->getColLower() ;
    lb_ = lb[k] ;
    const double *ub = mtx->getColUpper() ;
    ub_ = ub[k] ;
  }
  origVec_->sortIncrIndex() ;
}
/*
  Constructor to initialise from a CoinPostsolveMatrix
*/
CoinPresolveMonitor::CoinPresolveMonitor (const CoinPostsolveMatrix *mtx,
					  bool isRow,
					  int k)
{
  ndx_ = k ;
  isRow_ = isRow ;

  if (isRow) {
    origVec_ = extractRow(k,mtx) ;
    const double *blow = mtx->getRowLower() ;
    lb_ = blow[k] ;
    const double *b = mtx->getRowUpper() ;
    ub_ = b[k] ;
  } else {
    origVec_ = extractCol(k,mtx) ;
    const double *lb = mtx->getColLower() ;
    lb_ = lb[k] ;
    const double *ub = mtx->getColUpper() ;
    ub_ = ub[k] ;
  }
  origVec_->sortIncrIndex() ;
}

/*
  Extract a row from a CoinPresolveMatrix. Since a CoinPresolveMatrix contains
  both row-ordered and column-ordered copies, this is relatively efficient.
*/
CoinPackedVector *CoinPresolveMonitor::extractRow (int i,
					const CoinPresolveMatrix *mtx) const
{
  const CoinBigIndex *rowStarts = mtx->getRowStarts() ;
  const int *colIndices = mtx->getColIndicesByRow() ;
  const double *coeffs = mtx->getElementsByRow() ;
  const int rowLen = mtx->hinrow_[i] ;
  const CoinBigIndex &ii = rowStarts[i] ;
  return (new CoinPackedVector(rowLen,&colIndices[ii],&coeffs[ii])) ;
}

/*
  Extract a column from a CoinPresolveMatrix. Since a CoinPresolveMatrix
  contains both row-ordered and column-ordered copies, this is relatively
  efficient.
*/
CoinPackedVector *CoinPresolveMonitor::extractCol (int j,
					const CoinPresolveMatrix *mtx) const
{
  const CoinBigIndex *colStarts = mtx->getColStarts() ;
  const int *colLens = mtx->getColLengths() ;
  const int *rowIndices = mtx->getRowIndicesByCol() ;
  const double *coeffs = mtx->getElementsByCol() ;
  const CoinBigIndex &jj = colStarts[j] ;
  return (new CoinPackedVector(colLens[j],&rowIndices[jj],&coeffs[jj])) ;
}

/*
  Extract a row from a CoinPostsolveMatrix. This is very painful, because
  the matrix is threaded column-ordered only. We have to scan every entry
  in the matrix, looking for entries that match the requested row index.
*/
CoinPackedVector *CoinPresolveMonitor::extractRow (int i,
					const CoinPostsolveMatrix *mtx) const
{
  const CoinBigIndex *colStarts = mtx->getColStarts() ;
  const int *colLens = mtx->getColLengths() ;
  const double *coeffs = mtx->getElementsByCol() ;
  const int *rowIndices = mtx->getRowIndicesByCol() ;
  const CoinBigIndex *colLinks = mtx->link_ ;

  int n = mtx->getNumCols() ;

  CoinPackedVector *pkvec = new CoinPackedVector() ;

  for (int j = 0 ; j < n ; j++) {
    const CoinBigIndex ii =
	presolve_find_row3(i,colStarts[j],colLens[j],rowIndices,colLinks) ;
    if (ii >= 0) pkvec->insert(j,coeffs[ii]) ;
  }

  return (pkvec) ;
}

/*
  Extract a column from a CoinPostsolveMatrix. At least here we only need to
  walk one threaded column.
*/
CoinPackedVector *CoinPresolveMonitor::extractCol (int j,
					const CoinPostsolveMatrix *mtx) const
{
  const CoinBigIndex *colStarts = mtx->getColStarts() ;
  const int *colLens = mtx->getColLengths() ;
  const double *coeffs = mtx->getElementsByCol() ;
  const int *rowIndices = mtx->getRowIndicesByCol() ;
  const CoinBigIndex *colLinks = mtx->link_ ;

  CoinPackedVector *pkvec = new CoinPackedVector() ;

  CoinBigIndex jj = colStarts[j] ;
  const int &lenj = colLens[j] ;
  for (int k = 0 ; k < lenj ; k++) {
    pkvec->insert(rowIndices[jj],coeffs[jj]) ;
    jj = colLinks[jj] ;
  }
  
  return (pkvec) ;
}

/*
  Extract the current version of the row or column from the CoinPresolveMatrix
  into a CoinPackedVector, sort it, and compare it to the stored
  CoinPackedVector. Differences are reported to std::cout.
*/
void CoinPresolveMonitor::checkAndTell (const CoinPresolveMatrix *mtx)
{
  CoinPackedVector *curVec = 0 ;
  const double *lbs = 0 ;
  const double *ubs = 0 ;
  if (isRow_) {
    lbs = mtx->getRowLower() ;
    ubs = mtx->getRowUpper() ;
    curVec = extractRow(ndx_,mtx) ;
  } else {
    curVec = extractCol(ndx_,mtx) ;
    lbs = mtx->getColLower() ;
    ubs = mtx->getColUpper() ;
  }
  checkAndTell(curVec,lbs[ndx_],ubs[ndx_]) ;
}

/*
  Extract the current version of the row or column from the CoinPostsolveMatrix
  into a CoinPackedVector, sort it, and compare it to the stored
  CoinPackedVector. Differences are reported to std::cout.
*/
void CoinPresolveMonitor::checkAndTell (const CoinPostsolveMatrix *mtx)
{
  CoinPackedVector *curVec = 0 ;
  const double *lbs = 0 ;
  const double *ubs = 0 ;
  if (isRow_) {
    lbs = mtx->getRowLower() ;
    ubs = mtx->getRowUpper() ;
    curVec = extractRow(ndx_,mtx) ;
  } else {
    curVec = extractCol(ndx_,mtx) ;
    lbs = mtx->getColLower() ;
    ubs = mtx->getColUpper() ;
  }
  checkAndTell(curVec,lbs[ndx_],ubs[ndx_]) ;
}

/*
  And the worker method, which does the actual diff of the vectors and also
  checks the bounds.

  If the vector fails a quick check with ==, extract the indices, merge them,
  and then walk the indices checking for presence in each vector. Where both
  elements are present, check the coefficient. Report any differences.

  Not the most efficient implementation, but it leverages existing
  capabilities.
*/
void CoinPresolveMonitor::checkAndTell (CoinPackedVector *curVec,
					double lb, double ub)
{
  curVec->sortIncrIndex() ;

  std::cout
    << "checking " << ((isRow_)?"row ":"column ") << ndx_ << " ..." ;

  int diffcnt = 0 ;
  if (lb_ != lb) {
    diffcnt++ ;
    std::cout
      << std::endl << "    "
      << ((isRow_)?"blow":"lb") << " = " << lb_ << " in original, "
      << lb << " in current." ;
  }
  if (ub_ != ub) {
    diffcnt++ ;
    std::cout
      << std::endl << "    "
      << ((isRow_)?"b":"ub") << " = " << ub_ << " in original, "
      << ub << " in current." ;
  }
  bool vecDiff = ((*origVec_) == (*curVec)) ;
/*
  Dispense with the easy outcomes.
*/
  if (diffcnt == 0 && !vecDiff) {
    std::cout << " equal." << std::endl ;
    return ;
  } else if (!vecDiff) {
    std::cout << std::endl << " coefficients equal." << std::endl ;
    return ;
  }
/*
  We have to compare the coefficients. Merge the index sets.
*/
  int origLen = origVec_->getNumElements() ;
  int curLen = curVec->getNumElements() ;
  int mergedLen = origLen+curLen ;
  int *mergedIndices = new int [mergedLen] ;
  CoinCopyN(origVec_->getIndices(),origLen,mergedIndices) ;
  CoinCopyN(curVec->getIndices(),curLen,mergedIndices+origLen) ;
  std::inplace_merge(mergedIndices,mergedIndices+origLen,
  		     mergedIndices+mergedLen) ;
  int *uniqEnd = std::unique(mergedIndices,mergedIndices+mergedLen) ;
  int uniqLen = static_cast<int>(uniqEnd-mergedIndices) ;

  for (int k = 0 ; k < uniqLen ; k++) {
    int j = mergedIndices[k] ;
    double aij_orig = 0.0 ;
    double aij_cur = 0.0 ;
    bool inOrig = false ;
    bool inCur = false ;
    if (origVec_->findIndex(j) >= 0) {
      inOrig = true ;
      aij_orig = (*origVec_)[j] ;
    }
    if (curVec->findIndex(j) >= 0) {
      inCur = true ;
      aij_cur = (*curVec)[j] ;
    }
    if (inOrig == false || inCur == false ||
        aij_orig != aij_cur) {
      diffcnt++ ;
      std::cout << std::endl << "    " ;
      if (isRow_)
        std::cout << "coeff a(" << ndx_ << "," << j << ") " ;
      else
        std::cout << "coeff a(" << j << "," << ndx_ << ") " ;
      if (inOrig == false)
        std::cout << "= " << aij_cur << " not present in original." ;
      else if (inCur == false)
        std::cout << "= " << aij_orig << " not present in current." ;
      else
        std::cout
	  << " = " << aij_orig << " in original, "
	  << aij_cur << " in current." ;
    }
  }
  std::cout << std::endl << "  " << diffcnt << " changes." << std::endl ;
  delete[] mergedIndices ;
}

