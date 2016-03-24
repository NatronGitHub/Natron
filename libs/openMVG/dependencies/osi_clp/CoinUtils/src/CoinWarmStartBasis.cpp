/* $Id: CoinWarmStartBasis.cpp 1515 2011-12-10 23:38:04Z lou $ */
// Copyright (C) 2000, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#if defined(_MSC_VER)
// Turn off compiler warning about long names
#  pragma warning(disable:4786)
#endif

#include "CoinUtilsConfig.h"
#include <cassert>

#include "CoinWarmStartBasis.hpp"
#include "CoinHelperFunctions.hpp"
#include <cmath>
#include <iostream>

//#############################################################################

void 
CoinWarmStartBasis::setSize(int ns, int na) {
  // Round all so arrays multiple of 4
  int nintS = (ns+15) >> 4;
  int nintA = (na+15) >> 4;
  int size = nintS+nintA;
  if (size) {
    if (size>maxSize_) {
      delete[] structuralStatus_;
      maxSize_ = size+10;
      structuralStatus_ = new char[4*maxSize_];
    }
    memset (structuralStatus_, 0, (4*nintS) * sizeof(char));
    artificialStatus_ = structuralStatus_+4*nintS;
    memset (artificialStatus_, 0, (4*nintA) * sizeof(char));
  } else {
    artificialStatus_ = NULL;
  }
  numArtificial_ = na;
  numStructural_ = ns;
}

void 
CoinWarmStartBasis::assignBasisStatus(int ns, int na, char*& sStat, 
				     char*& aStat) {
  // Round all so arrays multiple of 4
  int nintS = (ns+15) >> 4;
  int nintA = (na+15) >> 4;
  int size = nintS+nintA;
  if (size) {
    if (size>maxSize_) {
      delete[] structuralStatus_;
      maxSize_ = size+10;
      structuralStatus_ = new char[4*maxSize_];
    }
    CoinMemcpyN( sStat,(4*nintS), structuralStatus_);
    artificialStatus_ = structuralStatus_+4*nintS;
    CoinMemcpyN( aStat,(4*nintA), artificialStatus_);
  } else {
    artificialStatus_ = NULL;
  }
  numStructural_ = ns;
  numArtificial_ = na;
  delete [] sStat;
  delete [] aStat;
  sStat = NULL;
  aStat = NULL;
}
CoinWarmStartBasis::CoinWarmStartBasis(int ns, int na, 
				     const char* sStat, const char* aStat) :
  numStructural_(ns), numArtificial_(na),
  structuralStatus_(NULL), artificialStatus_(NULL) {
  // Round all so arrays multiple of 4
  int nintS = ((ns+15) >> 4);
  int nintA = ((na+15) >> 4);
  maxSize_ = nintS+nintA;
  if (maxSize_ > 0) {
    structuralStatus_ = new char[4*maxSize_];
    if (nintS>0) {
      structuralStatus_[4*nintS-3]=0;
      structuralStatus_[4*nintS-2]=0;
      structuralStatus_[4*nintS-1]=0;
      CoinMemcpyN( sStat,((ns+3)/4), structuralStatus_);
    }
    artificialStatus_ = structuralStatus_+4*nintS;
    if (nintA > 0) {
      artificialStatus_[4*nintA-3]=0;
      artificialStatus_[4*nintA-2]=0;
      artificialStatus_[4*nintA-1]=0;
      CoinMemcpyN( aStat,((na+3)/4), artificialStatus_);
    }
  }
}

CoinWarmStartBasis::CoinWarmStartBasis(const CoinWarmStartBasis& ws) :
  numStructural_(ws.numStructural_), numArtificial_(ws.numArtificial_),
  structuralStatus_(NULL), artificialStatus_(NULL) {
  // Round all so arrays multiple of 4
  int nintS = (numStructural_+15) >> 4;
  int nintA = (numArtificial_+15) >> 4;
  maxSize_ = nintS+nintA;
  if (maxSize_ > 0) {
    structuralStatus_ = new char[4*maxSize_];
    CoinMemcpyN( ws.structuralStatus_,	(4*nintS), structuralStatus_);
    artificialStatus_ = structuralStatus_+4*nintS;
    CoinMemcpyN( ws.artificialStatus_,	(4*nintA), artificialStatus_);
  }
}

CoinWarmStartBasis& 
CoinWarmStartBasis::operator=(const CoinWarmStartBasis& rhs)
{
  if (this != &rhs) {
    numStructural_=rhs.numStructural_;
    numArtificial_=rhs.numArtificial_;
    // Round all so arrays multiple of 4
    int nintS = (numStructural_+15) >> 4;
    int nintA = (numArtificial_+15) >> 4;
    int size = nintS+nintA;
    if (size>maxSize_) {
      delete[] structuralStatus_;
      maxSize_ = size+10;
      structuralStatus_ = new char[4*maxSize_];
    }
    if (size > 0) {
      CoinMemcpyN( rhs.structuralStatus_,	(4*nintS), structuralStatus_);
      artificialStatus_ = structuralStatus_+4*nintS;
      CoinMemcpyN( rhs.artificialStatus_,	(4*nintA), artificialStatus_);
    } else {
      artificialStatus_ = NULL;
    }
  }
  return *this;
}

// Resizes 
void 
CoinWarmStartBasis::resize (int newNumberRows, int newNumberColumns)
{
  int i , nCharNewS, nCharOldS, nCharNewA, nCharOldA;
  if (newNumberRows!=numArtificial_||newNumberColumns!=numStructural_) {
    nCharOldS  = 4*((numStructural_+15)>>4);
    int nIntS  = (newNumberColumns+15)>>4;
    nCharNewS  = 4*nIntS;
    nCharOldA  = 4*((numArtificial_+15)>>4);
    int nIntA  = (newNumberRows+15)>>4;
    nCharNewA  = 4*nIntA;
    int size = nIntS+nIntA;
    // Do slowly if number of columns increases or need new array
    if (newNumberColumns>numStructural_||
	size>maxSize_) {
      if (size>maxSize_)
	maxSize_ = size+10;
      char * array = new char[4*maxSize_];
      // zap all for clarity and zerofault etc
      memset(array,0,4*maxSize_*sizeof(char));
      CoinMemcpyN(structuralStatus_,(nCharOldS>nCharNewS)?nCharNewS:nCharOldS,array);
      CoinMemcpyN(artificialStatus_,	     (nCharOldA>nCharNewA)?nCharNewA:nCharOldA,
                   array+nCharNewS);
      delete [] structuralStatus_;
      structuralStatus_ = array;
      artificialStatus_ = array+nCharNewS;
      for (i=numStructural_;i<newNumberColumns;i++) 
	setStructStatus(i, atLowerBound);
      for (i=numArtificial_;i<newNumberRows;i++) 
	setArtifStatus(i, basic);
    } else {
      // can do faster
      if (newNumberColumns!=numStructural_) {
	memmove(structuralStatus_+nCharNewS,artificialStatus_,
		(nCharOldA>nCharNewA)?nCharNewA:nCharOldA);
	artificialStatus_ = structuralStatus_+4*nIntS;
      }
      for (i=numArtificial_;i<newNumberRows;i++) 
	setArtifStatus(i, basic);
    }
    numStructural_ = newNumberColumns;
    numArtificial_ = newNumberRows;
  }
}

/*
  compressRows takes an ascending list of target indices without duplicates
  and removes them, compressing the artificialStatus_ array in place. It will
  fail spectacularly if the indices are not sorted. Use deleteRows if you
  need to preprocess the target indices to satisfy the conditions.
*/
void CoinWarmStartBasis::compressRows (int tgtCnt, const int *tgts)
{ 
  int i,keep,t,blkStart,blkEnd ;
/*
  Depending on circumstances, constraint indices may be larger than the size
  of the basis. Check for that now. Scan from top, betting that in most cases
  the majority of indices will be valid.
*/
  for (t = tgtCnt-1 ; t >= 0 && tgts[t] >= numArtificial_ ; t--) ;
  // temporary trap to make sure that scan from top is correct choice.
  // if ((t+1) < tgtCnt/2)
  // { printf("CWSB: tgtCnt %d, t %d; BAD CASE",tgtCnt,t+1) ; }
  if (t < 0) return ;
  tgtCnt = t+1 ;
  Status stati ;

# ifdef COIN_DEBUG
/*
  If we're debugging, scan to see if we're deleting nonbasic artificials.
  (In other words, are we deleting tight constraints?) Easiest to just do this
  up front as opposed to integrating it with the loops below.
*/
  int nbCnt = 0 ;
  for (t = 0 ; t < tgtCnt ; t++)
  { i = tgts[t] ;
    stati = getStatus(artificialStatus_,i) ;
    if (stati != CoinWarmStartBasis::basic)
    { nbCnt++ ; } }
  if (nbCnt > 0)
  { std::cout << nbCnt << " nonbasic artificials deleted." << std::endl ; }
# endif

/*
  Preserve all entries before the first target. Skip across consecutive
  target indices to establish the start of the first block to be retained.
*/
  keep = tgts[0] ;
  for (t = 0 ; t < tgtCnt-1 && tgts[t]+1 == tgts[t+1] ; t++) ;
  blkStart = tgts[t]+1 ;
/*
  Outer loop works through the indices to be deleted. Inner loop copies runs
  of indices to keep.
*/
  while (t < tgtCnt-1)
  { blkEnd = tgts[t+1]-1 ;
    for (i = blkStart ; i <= blkEnd ; i++)
    { stati = getStatus(artificialStatus_,i) ;
      setStatus(artificialStatus_,keep++,stati) ; }
    for (t++ ; t < tgtCnt-1 && tgts[t]+1 == tgts[t+1] ; t++) ;
    blkStart = tgts[t]+1 ; }
/*
  Finish off by copying from last deleted index to end of status array.
*/
  for (i = blkStart ; i < numArtificial_ ; i++)
  { stati = getStatus(artificialStatus_,i) ;
    setStatus(artificialStatus_,keep++,stati) ; }

  numArtificial_ -= tgtCnt ;

  return ; }

/*
  deleteRows takes an unordered list of target indices with duplicates and
  removes them from the basis. The strategy is to preprocesses the list into
  an ascending list without duplicates, suitable for compressRows.
*/
void 
CoinWarmStartBasis::deleteRows (int rawTgtCnt, const int *rawTgts)
{ if (rawTgtCnt <= 0) return ;

  int i;
  int last=-1;
  bool ordered=true;
  for (i=0;i<rawTgtCnt;i++) {
    int iRow = rawTgts[i];
    if (iRow>last) {
      last=iRow;
    } else {
      ordered=false;
      break;
    }
  }
  if (ordered) {
    compressRows(rawTgtCnt,rawTgts) ;
  } else {
    int * tgts = new int[rawTgtCnt] ;
    CoinMemcpyN(rawTgts,rawTgtCnt,tgts);
    int *first = &tgts[0] ;
    int *last = &tgts[rawTgtCnt] ;
    int *endUnique ;
    std::sort(first,last) ;
    endUnique = std::unique(first,last) ;
    int tgtCnt = static_cast<int>(endUnique-first) ;
    compressRows(tgtCnt,tgts) ;
    delete [] tgts ;
  }
  return  ; }

// Deletes columns
void 
CoinWarmStartBasis::deleteColumns(int number, const int * which)
{
  int i ;
  char * deleted = new char[numStructural_];
  int numberDeleted=0;
  memset(deleted,0,numStructural_*sizeof(char));
  for (i=0;i<number;i++) {
    int j = which[i];
    if (j>=0&&j<numStructural_&&!deleted[j]) {
      numberDeleted++;
      deleted[j]=1;
    }
  }
  int nCharNewS  = 4*((numStructural_-numberDeleted+15)>>4);
  int nCharNewA  = 4*((numArtificial_+15)>>4);
  char * array = new char[4*maxSize_];
# ifdef ZEROFAULT
  memset(array,0,(4*maxSize_*sizeof(char))) ;
# endif
  CoinMemcpyN(artificialStatus_,nCharNewA,array+nCharNewS);
  int put=0;
# ifdef COIN_DEBUG
  int numberBasic=0;
# endif
  for (i=0;i<numStructural_;i++) {
    Status status = getStructStatus(i);
    if (!deleted[i]) {
      setStatus(array,put,status) ;
      put++; }
#   ifdef COIN_DEBUG
    else
    if (status==CoinWarmStartBasis::basic)
    { numberBasic++ ; }
#   endif
  }
  delete [] structuralStatus_;
  structuralStatus_ = array;
  artificialStatus_ = structuralStatus_ + nCharNewS;
  delete [] deleted;
  numStructural_ -= numberDeleted;
#ifdef COIN_DEBUG
  if (numberBasic)
    std::cout<<numberBasic<<" basic structurals deleted"<<std::endl;
#endif
}

/*
  Merge the specified entries from the source basis (src) into the target
  basis (this). For each entry in xferCols, xferRows, first is the source index,
  second is the target index, and third is the run length.
  
  This routine was originally created to solve the problem of correctly
  expanding an existing basis but can be used in a general context to merge
  two bases.

  If the xferRows (xferCols) vector is missing, no row (column) information
  will be transferred from src to tgt.
*/

void CoinWarmStartBasis::mergeBasis (const CoinWarmStartBasis *src,
				     const XferVec *xferRows,
				     const XferVec *xferCols)

{ assert(src) ;
  int srcCols = src->getNumStructural() ;
  int srcRows = src->getNumArtificial() ;
/*
  Merge the structural variable status.
*/
  if (srcCols > 0 && xferCols != NULL)
  { XferVec::const_iterator xferSpec = xferCols->begin() ;
    XferVec::const_iterator xferEnd = xferCols->end() ;
    for ( ; xferSpec != xferEnd ; xferSpec++)
    { int srcNdx = (*xferSpec).first ;
      int tgtNdx = (*xferSpec).second ;
      int runLen = (*xferSpec).third ;
      assert(srcNdx >= 0 && srcNdx+runLen <= srcCols) ;
      assert(tgtNdx >= 0 && tgtNdx+runLen <= getNumStructural()) ;
      for (int i = 0 ; i < runLen ; i++)
      { CoinWarmStartBasis::Status stat = src->getStructStatus(srcNdx+i) ;
	setStructStatus(tgtNdx+i,stat) ; } } }
/*
  Merge the row (artificial variable) status.
*/
  if (srcRows > 0 && xferRows != NULL)
  { XferVec::const_iterator xferSpec = xferRows->begin() ;
    XferVec::const_iterator xferEnd = xferRows->end() ;
    for ( ; xferSpec != xferEnd ; xferSpec++)
    { int srcNdx = (*xferSpec).first ;
      int tgtNdx = (*xferSpec).second ;
      int runLen = (*xferSpec).third ;
      assert(srcNdx >= 0 && srcNdx+runLen <= srcRows) ;
      assert(tgtNdx >= 0 && tgtNdx+runLen <= getNumArtificial()) ;
      for (int i = 0 ; i < runLen ; i++)
      { CoinWarmStartBasis::Status stat = src->getArtifStatus(srcNdx+i) ;
	setArtifStatus(tgtNdx+i,stat) ; } } }

  return ; }

// Prints in readable format (for debug)
void 
CoinWarmStartBasis::print() const
{
  int i ;
  int numberBasic=0;
  for (i=0;i<numStructural_;i++) {
    Status status = getStructStatus(i);
    if (status==CoinWarmStartBasis::basic) 
      numberBasic++;
  }
  int numberStructBasic = numberBasic;
  for (i=0;i<numArtificial_;i++) {
    Status status = getArtifStatus(i);
    if (status==CoinWarmStartBasis::basic) 
      numberBasic++;
  }
  std::cout<<"Basis "<<this<<" has "<<numArtificial_<<" rows and "
	   <<numStructural_<<" columns, "
	   <<numberBasic<<" basic, of which "<<numberStructBasic
	   <<" were columns"<<std::endl;
  std::cout<<"Rows:"<<std::endl;
  char type[]={'F','B','U','L'};

  for (i=0;i<numArtificial_;i++) 
    std::cout<<type[getArtifStatus(i)];
  std::cout<<std::endl;
  std::cout<<"Columns:"<<std::endl;

  for (i=0;i<numStructural_;i++) 
    std::cout<<type[getStructStatus(i)];
  std::cout<<std::endl;
}
CoinWarmStartBasis::CoinWarmStartBasis()
{
  
  numStructural_ = 0;
  numArtificial_ = 0;
  maxSize_ = 0;
  structuralStatus_ = NULL;
  artificialStatus_ = NULL;
}
CoinWarmStartBasis::~CoinWarmStartBasis()
{
  delete[] structuralStatus_;
}
// Returns number of basic structurals
int
CoinWarmStartBasis::numberBasicStructurals() const
{
  int i ;
  int numberBasic=0;
  for (i=0;i<numStructural_;i++) {
    Status status = getStructStatus(i);
    if (status==CoinWarmStartBasis::basic) 
      numberBasic++;
  }
  return numberBasic;
}
// Returns true if full basis (for debug)
bool 
CoinWarmStartBasis::fullBasis() const
{
  int i ;
  int numberBasic=0;
  for (i=0;i<numStructural_;i++) {
    Status status = getStructStatus(i);
    if (status==CoinWarmStartBasis::basic) 
      numberBasic++;
  }
  for (i=0;i<numArtificial_;i++) {
    Status status = getArtifStatus(i);
    if (status==CoinWarmStartBasis::basic) 
      numberBasic++;
  }
#ifdef COIN_DEVELOP
  if (numberBasic!=numArtificial_)
    printf("mismatch - basis has %d rows, %d basic\n",
	   numArtificial_,numberBasic);
#endif
  return numberBasic==numArtificial_;
}
// Returns true if full basis and fixes up (for debug)
bool 
CoinWarmStartBasis::fixFullBasis() 
{
  int i ;
  int numberBasic=0;
  for (i=0;i<numStructural_;i++) {
    Status status = getStructStatus(i);
    if (status==CoinWarmStartBasis::basic) 
      numberBasic++;
  }
  for (i=0;i<numArtificial_;i++) {
    Status status = getArtifStatus(i);
    if (status==CoinWarmStartBasis::basic) 
      numberBasic++;
  }
#ifdef COIN_DEVELOP
  if (numberBasic!=numArtificial_)
    printf("mismatch - basis has %d rows, %d basic\n",
	   numArtificial_,numberBasic);
#endif
  bool returnCode = (numberBasic==numArtificial_);
  if (numberBasic>numArtificial_) {
    for (i=0;i<numStructural_;i++) {
      Status status = getStructStatus(i);
      if (status==CoinWarmStartBasis::basic) 
	setStructStatus(i,atLowerBound);
	numberBasic--;
	if (numberBasic==numArtificial_)
	  break;
    }
  } else if (numberBasic<numArtificial_) {
    for (i=0;i<numArtificial_;i++) {
      Status status = getArtifStatus(i);
      if (status!=CoinWarmStartBasis::basic) {
	setArtifStatus(i,basic);
	numberBasic++;
	if (numberBasic==numArtificial_)
	  break;
      }
    }
  }
  return returnCode;
}
/*
  Generate a diff that'll convert oldCWS into the basis pointed to by this.

  This routine is a bit of a hack, for efficiency's sake. Rather than work
  with individual status vector entries, we're going to treat the vectors as
  int's --- in effect, we create one diff entry for each block of 16 status
  entries. Diffs for logicals are tagged with 0x80000000.
*/

CoinWarmStartDiff*
CoinWarmStartBasis::generateDiff (const CoinWarmStart *const oldCWS) const
{
/*
  Make sure the parameter is CoinWarmStartBasis or derived class.
*/
  const CoinWarmStartBasis *oldBasis =
      dynamic_cast<const CoinWarmStartBasis *>(oldCWS) ;
#ifndef NDEBUG 
  if (!oldBasis)
  { throw CoinError("Old basis not derived from CoinWarmStartBasis.",
		    "generateDiff","CoinWarmStartBasis") ; }
#endif
  const CoinWarmStartBasis *newBasis = this ;
/*
  Make sure newBasis is equal or bigger than oldBasis. Calculate the worst case
  number of diffs and allocate vectors to hold them.
*/
  const int oldArtifCnt = oldBasis->getNumArtificial() ;
  const int oldStructCnt = oldBasis->getNumStructural() ;
  const int newArtifCnt = newBasis->getNumArtificial() ;
  const int newStructCnt = newBasis->getNumStructural() ;

  assert(newArtifCnt >= oldArtifCnt) ;
  assert(newStructCnt >= oldStructCnt) ;

  int sizeOldArtif = (oldArtifCnt+15)>>4 ;
  int sizeNewArtif = (newArtifCnt+15)>>4 ;
  int sizeOldStruct = (oldStructCnt+15)>>4 ;
  int sizeNewStruct = (newStructCnt+15)>>4 ;
  int maxBasisLength = sizeNewArtif+sizeNewStruct ;

  unsigned int *diffNdx = new unsigned int [2*maxBasisLength]; 
  unsigned int *diffVal = diffNdx + maxBasisLength; 
/*
  Ok, setup's over. Now scan the logicals (aka artificials, standing in for
  constraints). For the portion of the status arrays which overlap, create
  diffs. Then add any additional status from newBasis.

  I removed the following bit of code & comment:

    if (sizeNew == sizeOld) sizeOld--; // make sure all taken

  I assume this is meant to trap cases where oldBasis does not occupy all of
  the final int, but I can't see where it's necessary.
*/
  const unsigned int *oldStatus =
      reinterpret_cast<const unsigned int *>(oldBasis->getArtificialStatus()) ;
  const unsigned int *newStatus = 
      reinterpret_cast<const unsigned int *>(newBasis->getArtificialStatus()) ;
  int numberChanged = 0 ;
  int i ;
  for (i = 0 ; i < sizeOldArtif ; i++)
  { if (oldStatus[i] != newStatus[i])
    { diffNdx[numberChanged] = i|0x80000000 ;
      diffVal[numberChanged++] = newStatus[i] ; } }
  for ( ; i < sizeNewArtif ; i++)
  { diffNdx[numberChanged] = i|0x80000000 ;
    diffVal[numberChanged++] = newStatus[i] ; }
/*
  Repeat for structural variables.
*/
  oldStatus =
      reinterpret_cast<const unsigned int *>(oldBasis->getStructuralStatus()) ;
  newStatus =
      reinterpret_cast<const unsigned int *>(newBasis->getStructuralStatus()) ;
  for (i = 0 ; i < sizeOldStruct ; i++)
  { if (oldStatus[i] != newStatus[i])
    { diffNdx[numberChanged] = i ;
      diffVal[numberChanged++] = newStatus[i] ; } }
  for ( ; i < sizeNewStruct ; i++)
  { diffNdx[numberChanged] = i ;
    diffVal[numberChanged++] = newStatus[i] ; }
/*
  Create the object of our desire.
*/
  CoinWarmStartBasisDiff *diff;
  if ((numberChanged*2<maxBasisLength+1||!newStructCnt)&&true)
    diff = new CoinWarmStartBasisDiff(numberChanged,diffNdx,diffVal) ;
  else
    diff = new CoinWarmStartBasisDiff(newBasis) ;
/*
  Clean up and return.
*/
  delete[] diffNdx ;

  return (static_cast<CoinWarmStartDiff *>(diff)) ; }


/*
  Apply a diff to the basis pointed to by this.  It's assumed that the
  allocated capacity of the basis is sufficiently large.
*/
void CoinWarmStartBasis::applyDiff (const CoinWarmStartDiff *const cwsdDiff)
{
/*
  Make sure we have a CoinWarmStartBasisDiff
*/
  const CoinWarmStartBasisDiff *diff =
    dynamic_cast<const CoinWarmStartBasisDiff *>(cwsdDiff) ;
#ifndef NDEBUG 
  if (!diff)
  { throw CoinError("Diff not derived from CoinWarmStartBasisDiff.",
		    "applyDiff","CoinWarmStartBasis") ; }
#endif
/*
  Application is by straighforward replacement of words in the status arrays.
  Index entries for logicals (aka artificials) are tagged with 0x80000000.
*/
  const int numberChanges = diff->sze_ ;
  unsigned int *structStatus =
    reinterpret_cast<unsigned int *>(this->getStructuralStatus()) ;
  unsigned int *artifStatus =
    reinterpret_cast<unsigned int *>(this->getArtificialStatus()) ;
  if (numberChanges>=0) {
    const unsigned int *diffNdxs = diff->difference_ ;
    const unsigned int *diffVals = diffNdxs+numberChanges ;
    
    for (int i = 0 ; i < numberChanges ; i++)
      { unsigned int diffNdx = diffNdxs[i] ;
      unsigned int diffVal = diffVals[i] ;
      if ((diffNdx&0x80000000) == 0)
	{ structStatus[diffNdx] = diffVal ; }
      else
	{ artifStatus[diffNdx&0x7fffffff] = diffVal ; } }
  } else {
    // just replace
    const unsigned int * diffA = diff->difference_ -1;
    const int artifCnt = static_cast<int> (diffA[0]);
    const int structCnt = -numberChanges;
    int sizeArtif = (artifCnt+15)>>4 ;
    int sizeStruct = (structCnt+15)>>4 ;
    CoinMemcpyN(diffA+1,sizeStruct,structStatus);
    CoinMemcpyN(diffA+1+sizeStruct,sizeArtif,artifStatus);
  }
  return ; }

const char *statusName (CoinWarmStartBasis::Status status) {
  switch (status) {
    case CoinWarmStartBasis::isFree: { return ("NBFR") ; }
    case CoinWarmStartBasis::basic: { return ("B") ; }
    case CoinWarmStartBasis::atUpperBound: { return ("NBUB") ; }
    case CoinWarmStartBasis::atLowerBound: { return ("NBLB") ; }
    default: { return ("INVALID!") ; }
  }
}

/* Routines for CoinWarmStartBasisDiff */

/*
  Constructor given diff data.
*/
CoinWarmStartBasisDiff::CoinWarmStartBasisDiff (int sze,
  const unsigned int *const diffNdxs, const unsigned int *const diffVals)
  : sze_(sze),
    difference_(NULL)

{ if (sze > 0)
  { difference_ = new unsigned int[2*sze] ;
    CoinMemcpyN(diffNdxs,sze,difference_);
    CoinMemcpyN(diffVals,sze,difference_+sze_); }
  
  return ; }
/*
  Constructor when full is smaller than diff!
*/
CoinWarmStartBasisDiff::CoinWarmStartBasisDiff (const CoinWarmStartBasis * rhs)
  : sze_(0),
    difference_(0)
{
  const int artifCnt = rhs->getNumArtificial() ;
  const int structCnt = rhs->getNumStructural() ;
  int sizeArtif = (artifCnt+15)>>4 ;
  int sizeStruct = (structCnt+15)>>4 ;
  int maxBasisLength = sizeArtif+sizeStruct ;
  assert (maxBasisLength&&structCnt);
  sze_ = - structCnt;
  difference_ = new unsigned int [maxBasisLength+1];
  difference_[0]=artifCnt;
  difference_++;
  CoinMemcpyN(reinterpret_cast<const unsigned int *> (rhs->getStructuralStatus()),sizeStruct,
	      difference_);
  CoinMemcpyN(reinterpret_cast<const unsigned int *> (rhs->getArtificialStatus()),sizeArtif,
	      difference_+sizeStruct);
}

/*
  Copy constructor.
*/

CoinWarmStartBasisDiff::CoinWarmStartBasisDiff
  (const CoinWarmStartBasisDiff &rhs)
  : sze_(rhs.sze_),
    difference_(0)
{ if (sze_ >0)
    { difference_ = CoinCopyOfArray(rhs.difference_,2*sze_); }
  else if (sze_<0) {
    const unsigned int * diff = rhs.difference_ -1;
    const int artifCnt = static_cast<int> (diff[0]);
    const int structCnt = -sze_;
    int sizeArtif = (artifCnt+15)>>4 ;
    int sizeStruct = (structCnt+15)>>4 ;
    int maxBasisLength = sizeArtif+sizeStruct ;
    difference_ = CoinCopyOfArray(diff,maxBasisLength+1);
    difference_++;
  }

  return ; }

/*
  Assignment --- for convenience when assigning objects containing
  CoinWarmStartBasisDiff objects.
*/
CoinWarmStartBasisDiff&
CoinWarmStartBasisDiff::operator= (const CoinWarmStartBasisDiff &rhs)

{ if (this != &rhs)
  { if (sze_>0 )
      { delete[] difference_ ; }
      else if (sze_<0) {
	unsigned int * diff = difference_ -1;
	delete [] diff;
      }
    sze_ = rhs.sze_ ;
    if (sze_ > 0)
      { difference_ = CoinCopyOfArray(rhs.difference_,2*sze_); }
    else if (sze_<0) {
      const unsigned int * diff = rhs.difference_ -1;
      const int artifCnt = static_cast<int> (diff[0]);
      const int structCnt = -sze_;
      int sizeArtif = (artifCnt+15)>>4 ;
      int sizeStruct = (structCnt+15)>>4 ;
      int maxBasisLength = sizeArtif+sizeStruct ;
      difference_ = CoinCopyOfArray(diff,maxBasisLength+1);
      difference_++;
    }
    else
    { difference_ = 0 ; } }
  
  return (*this) ; }
/*brief Destructor */
CoinWarmStartBasisDiff::~CoinWarmStartBasisDiff()
{
  if (sze_>0 ) {
    delete[] difference_ ;
  } else if (sze_<0) {
    unsigned int * diff = difference_ -1;
    delete [] diff;
  }
}
