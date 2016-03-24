/* $Id: CoinPresolveHelperFunctions.cpp 1560 2012-11-24 00:29:01Z lou $ */
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

/*! \file

  This file contains helper functions for CoinPresolve. The declarations needed
  for use are included in CoinPresolveMatrix.hpp.
*/

#include <stdio.h>

#include <cassert>
#include <iostream>

#include "CoinHelperFunctions.hpp"
#include "CoinPresolveMatrix.hpp"


/*! \defgroup PMMDVX Packed Matrix Major Dimension Vector Expansion
    \brief Functions to help with major-dimension vector expansion in a
	   packed matrix structure.

  This next block of functions handles the problems associated with expanding
  a column in a column-major representation or a row in a row-major
  representation.
  
  We need to be able to answer the questions:
    * Is there room to expand a major vector in place?
    * Is there sufficient free space at the end of the element and minor
      index storage areas (bulk storage) to hold the major vector?

  When the answer to the first question is `no', we need to be able to move
  the major vector to the free space at the end of bulk storage.
  
  When the answer to the second question is `no', we need to be able to
  compact the major vectors in the bulk storage area in order to regain a
  block of useable space at the end.

  presolve_make_memlists initialises a linked list that tracks the position of
  major vectors in the bulk storage area. It's used to locate physically
  adjacent vectors.

  presolve_expand deals with adding a coefficient to a major vector, either
  in-place or by moving it to free space at the end of the storage areas.
  There are two inline wrappers, presolve_expand_col and presolve_expand_row,
  defined in CoinPresolveMatrix.hpp.

  compact_rep compacts the major vectors in the storage areas to
  leave a single block of free space at the end.
*/
//@{

/*
  This first function doesn't need to be known outside of this file.
*/
namespace {

/*
  compact_rep

  This routine compacts the major vectors in the bulk storage area,
  leaving a single block of free space at the end. The vectors are not
  reordered, just shifted down to remove gaps.
*/

void compact_rep (double *elems, int *indices,
		  CoinBigIndex *starts, const int *lengths, int n,
		  const presolvehlink *link)
{
# if PRESOLVE_SUMMARY
  printf("****COMPACTING****\n") ;
# endif

  // for now, just look for the first element of the list
  int i = n ;
  while (link[i].pre != NO_LINK)
    i = link[i].pre ;

  CoinBigIndex j = 0 ;
  for (; i != n; i = link[i].suc) {
    CoinBigIndex s = starts[i] ;
    CoinBigIndex e = starts[i] + lengths[i] ;

    // because of the way link is organized, j <= s
    starts[i] = j ;
    for (CoinBigIndex k = s; k < e; k++) {
      elems[j] = elems[k] ;
      indices[j] = indices[k] ;
      j++ ;
   }
  }
}


} /* end unnamed namespace */

/*
  \brief Initialise linked list for major vector order in bulk storage

  Initialise the linked list that will track the order of major vectors in
  the element and row index bulk storage arrays.  When finished, link[j].pre
  contains the index of the previous non-empty vector in the storage arrays
  and link[j].suc contains the index of the next non-empty vector.

  For an empty vector j, link[j].pre = link[j].suc = NO_LINK.

  If n is the number of major-dimension vectors, link[n] is valid;
  link[n].pre is the index of the last non-empty vector, and
  link[n].suc = NO_LINK.

  This routine makes the implicit assumption that the order of vectors in the
  storage areas matches the order in starts. (I.e., there's no check that
  starts[j] > starts[i] for j < i.)
*/

void presolve_make_memlists (/*CoinBigIndex *starts,*/ int *lengths,
			     presolvehlink *link, int n)
{
  int i ;
  int pre = NO_LINK ;
  
  for (i=0; i<n; i++) {
    if (lengths[i]) {
      link[i].pre = pre ;
      if (pre != NO_LINK)
	link[pre].suc = i ;
      pre = i ;
    }
    else {
      link[i].pre = NO_LINK ;
      link[i].suc = NO_LINK ;
    }
  }
  if (pre != NO_LINK)
    link[pre].suc = n ;

  // (1) Arbitrarily place the last non-empty entry in link[n].pre
  link[n].pre = pre ;

  link[n].suc = NO_LINK ;
}



/*
  presolve_expand_major

  The routine looks at the space currently occupied by major-dimension vector
  k and makes sure that there's room to add one coefficient.

  This may require moving the vector to the vacant area at the end of the
  bulk storage array. If there's no room left at the end of the array, an
  attempt is made to compact the existing vectors to make space.

  Returns true for failure, false for success.
*/

bool presolve_expand_major (CoinBigIndex *majstrts, double *els,
			    int *minndxs, int *majlens,
			    presolvehlink *majlinks, int nmaj, int k)

{ const CoinBigIndex bulkCap = majstrts[nmaj] ;

/*
  Get the start and end of column k, and the index of the column which
  follows in the bulk storage.
*/
  CoinBigIndex kcsx = majstrts[k] ;
  CoinBigIndex kcex = kcsx + majlens[k] ;
  int nextcol = majlinks[k].suc ;
/*
  Do we have room to add one coefficient in place?
*/
  if (kcex+1 < majstrts[nextcol])
  { /* no action required */ }
/*
  Is k the last non-empty column? In that case, attempt to compact the
  bulk storage. This will move k, so update the column start and end.
  If we still have no space, it's a fatal error.
*/
  else
  if (nextcol == nmaj)
  { compact_rep(els,minndxs,majstrts,majlens,nmaj,majlinks) ;
    kcsx = majstrts[k] ;
    kcex = kcsx + majlens[k] ;
    if (kcex+1 >= bulkCap)
    { return (true) ; } }
/*
  The most complicated case --- we need to move k from its current location
  to empty space at the end of the bulk storage. And we may need to make that!
  Compaction is identical to the above case.
*/
  else
  { int lastcol = majlinks[nmaj].pre ;
    int newkcsx = majstrts[lastcol]+majlens[lastcol] ;
    int newkcex = newkcsx+majlens[k] ;

    if (newkcex+1 >= bulkCap)
    { compact_rep(els,minndxs,majstrts,majlens,nmaj,majlinks) ;
      kcsx = majstrts[k] ;
      kcex = kcsx + majlens[k] ;
      newkcsx = majstrts[lastcol]+majlens[lastcol] ;
      newkcex = newkcsx+majlens[k] ;
      if (newkcex+1 >= bulkCap)
      { return (true) ; } }
/*
  Moving the vector requires three actions. First we move the data, then
  update the packed matrix vector start, then relink the storage order list,
*/
    memcpy(reinterpret_cast<void *>(&minndxs[newkcsx]),
	   reinterpret_cast<void *>(&minndxs[kcsx]),majlens[k]*sizeof(int)) ;
    memcpy(reinterpret_cast<void *>(&els[newkcsx]),
	   reinterpret_cast<void *>(&els[kcsx]),majlens[k]*sizeof(double)) ;
    majstrts[k] = newkcsx ;
    PRESOLVE_REMOVE_LINK(majlinks,k) ;
    PRESOLVE_INSERT_LINK(majlinks,k,lastcol) ; }
/*
  Success --- the vector has room for one more coefficient.
*/
  return (false) ; }

//@}



/*
  Helper function to duplicate a major-dimension vector.
*/

/*
  A major-dimension vector is composed of paired element and minor index
  arrays.  We want to duplicate length entries from both arrays, starting at
  offset.

  If tgt > 0, we'll run a more complicated copy loop which will elide the
  entry with minor index == tgt. In this case, we want to reduce the size of
  the allocated array by 1.

  Pigs will fly before sizeof(int) > sizeof(double), but if it ever
  happens this code will fail.
*/

double *presolve_dupmajor (const double *elems, const int *indices,
			   int length, CoinBigIndex offset, int tgt)

{ int n ;

  if (tgt >= 0) length-- ;

  if (2*sizeof(int) <= sizeof(double))
    n = (3*length+1)>>1 ;
  else
    n = 2*length ;

  double *dArray = new double [n] ;
  int *iArray = reinterpret_cast<int *>(dArray+length) ;

  if (tgt < 0)
  { memcpy(dArray,elems+offset,length*sizeof(double)) ;
    memcpy(iArray,indices+offset,length*sizeof(int)) ; }
  else
  { int korig ;
    int kcopy = 0 ;
    indices += offset ;
    elems += offset ;
    for (korig = 0 ; korig <= length ; korig++)
    { int i = indices[korig] ;
      if (i != tgt)
      { dArray[kcopy] = elems[korig] ;
	iArray[kcopy++] = indices[korig] ; } } }

  return (dArray) ; }



/*
  Routines to find the position of the entry for a given minor index in a
  major vector. Inline wrappers with column-major and row-major parameter
  names are defined in CoinPresolveMatrix.hpp. The threaded matrix used in
  postsolve exists only as a column-major form, so only one wrapper is
  defined.
*/

/*
  presolve_find_minor

  Find the position (k) of the entry for a given minor index (tgt) within
  the range of entries for a major vector (ks, ke).

  Print a tag and abort (DIE) if there's no entry for tgt.
*/
#if 0
CoinBigIndex presolve_find_minor (int tgt, CoinBigIndex ks,
				  CoinBigIndex ke, const int *minndxs)

{ CoinBigIndex k ;
  for (k = ks ; k < ke ; k++)
  { if (minndxs[k] == tgt)
      return (k) ; }
  DIE("FIND_MINOR") ;

  abort () ; return -1; }
#endif
/*
  As presolve_find_minor, but return a position one past the end of
  the major vector when the entry is not already present.
*/
CoinBigIndex presolve_find_minor1 (int tgt, CoinBigIndex ks,
				   CoinBigIndex ke, const int *minndxs)
{ CoinBigIndex k ;
  for (k = ks ; k < ke ; k++)
  { if (minndxs[k] == tgt)
      return (k) ; }

  return (k) ; }

/*
  In a threaded matrix, the major vector does not occupy a contiguous block
  in the bulk storage area. For example, in a threaded column-major matrix,
  if a<i,p> is in pos'n kp of hrow, the next coefficient a<i,q> will be
  in pos'n kq = link[kp]. Abort if we don't find it.
*/
CoinBigIndex presolve_find_minor2 (int tgt, CoinBigIndex ks,
				   int majlen, const int *minndxs,
				   const CoinBigIndex *majlinks)

{ for (int i = 0 ; i < majlen ; ++i)
  { if (minndxs[ks] == tgt)
      return (ks) ;
    ks = majlinks[ks] ; }
  DIE("FIND_MINOR2") ;

  abort () ; return -1; }

/*
  As presolve_find_minor2, but return -1 if the entry is missing
*/
CoinBigIndex presolve_find_minor3 (int tgt, CoinBigIndex ks,
				   int majlen, const int *minndxs,
				   const CoinBigIndex *majlinks)

{ for (int i = 0 ; i < majlen ; ++i)
  { if (minndxs[ks] == tgt)
      return (ks) ;
    ks = majlinks[ks] ; }

  return (-1) ; }

/*
  Delete the entry for a minor index from a major vector.  The last entry in
  the major vector is moved into the hole left by the deleted entry. This
  leaves some space between the end of this major vector and the start of the
  next in the bulk storage areas (this is termed loosely packed). Inline
  wrappers with column-major and row-major parameter names are defined in
  CoinPresolveMatrix.hpp.  The threaded matrix used in postsolve exists only
  as a column-major form, so only one wrapper is defined.
*/

#if 0
void presolve_delete_from_major (int majndx, int minndx,
				 const CoinBigIndex *majstrts,
				 int *majlens, int *minndxs, double *els)

{ CoinBigIndex ks = majstrts[majndx] ;
  CoinBigIndex ke = ks + majlens[majndx] ;

  CoinBigIndex kmi = presolve_find_minor(minndx,ks,ke,minndxs) ;

  minndxs[kmi] = minndxs[ke-1] ;
  els[kmi] = els[ke-1] ;
  majlens[majndx]-- ;
  
  return ; }
// Delete all marked and zero marked
void presolve_delete_many_from_major (int majndx, char * marked,
				 const CoinBigIndex *majstrts,
				 int *majlens, int *minndxs, double *els)

{ 
  CoinBigIndex ks = majstrts[majndx] ;
  CoinBigIndex ke = ks + majlens[majndx] ;
  CoinBigIndex put=ks;
  for (CoinBigIndex k=ks;k<ke;k++) {
    int iMinor = minndxs[k];
    if (!marked[iMinor]) {
      minndxs[put]=iMinor;
      els[put++]=els[k];
    } else {
      marked[iMinor]=0;
    }
  } 
  majlens[majndx] = put-ks ;
  return ;
}
#endif

/*
  Delete the entry for a minor index from a major vector in a threaded matrix.

  This involves properly relinking the free list.
*/
void presolve_delete_from_major2 (int majndx, int minndx,
				  CoinBigIndex *majstrts, int *majlens,
				  int *minndxs, int *majlinks, 
				  CoinBigIndex *free_listp)

{
  CoinBigIndex k = majstrts[majndx] ;

/*
  Desired entry is the first in its major vector. We need to touch up majstrts
  to point to the next entry and link the deleted entry to the front of the
  free list.
*/
  if (minndxs[k] == minndx) {
    majstrts[majndx] = majlinks[k] ;
    majlinks[k] = *free_listp ;
    *free_listp = k ;
    majlens[majndx]-- ;
  } else {
/*
  Desired entry is somewhere in the major vector. We need to relink around
  it and then link it on the front of the free list.

  The loop runs over elements 1 .. len-1; we've already ruled out element 0.
*/
    int len = majlens[majndx] ;
    CoinBigIndex kpre = k ;
    k = majlinks[k] ;
    for (int i = 1 ; i < len ; ++i) {
      if (minndxs[k] == minndx) {
        majlinks[kpre] = majlinks[k] ;
	majlinks[k] = *free_listp ;
	*free_listp = k ;
	majlens[majndx]-- ;
	return ;
      }
      kpre = k ;
      k = majlinks[k] ;
    }
    DIE("DELETE_FROM_MAJOR2") ;
  }

  assert(*free_listp >= 0) ;

  return ;
}

