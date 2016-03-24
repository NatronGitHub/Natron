/* $Id: CoinHelperFunctions.hpp 1581 2013-04-06 12:48:50Z stefan $ */
// Copyright (C) 2000, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef CoinHelperFunctions_H
#define CoinHelperFunctions_H

#include "CoinUtilsConfig.h"

#if defined(_MSC_VER)
#  include <direct.h>
#  define getcwd _getcwd
#else
#  include <unistd.h>
#endif
//#define USE_MEMCPY

#include <cstdlib>
#include <cstdio>
#include <algorithm>
#include "CoinTypes.hpp"
#include "CoinError.hpp"

// Compilers can produce better code if they know about __restrict
#ifndef COIN_RESTRICT
#ifdef COIN_USE_RESTRICT
#define COIN_RESTRICT __restrict
#else
#define COIN_RESTRICT
#endif
#endif

//#############################################################################

/** This helper function copies an array to another location using Duff's
    device (for a speedup of ~2). The arrays are given by pointers to their
    first entries and by the size of the source array. Overlapping arrays are
    handled correctly. */

template <class T> inline void
CoinCopyN(const T* from, const int size, T* to)
{
    if (size == 0 || from == to)
	return;

#ifndef NDEBUG
    if (size < 0)
	throw CoinError("trying to copy negative number of entries",
			"CoinCopyN", "");
#endif

    int n = (size + 7) / 8;
    if (to > from) {
	const T* downfrom = from + size;
	T* downto = to + size;
	// Use Duff's device to copy
	switch (size % 8) {
	case 0: do{     *--downto = *--downfrom;
	case 7:         *--downto = *--downfrom;
	case 6:         *--downto = *--downfrom;
	case 5:         *--downto = *--downfrom;
	case 4:         *--downto = *--downfrom;
	case 3:         *--downto = *--downfrom;
	case 2:         *--downto = *--downfrom;
	case 1:         *--downto = *--downfrom;
	}while(--n>0);
	}
    } else {
	// Use Duff's device to copy
	--from;
	--to;
	switch (size % 8) {
	case 0: do{     *++to = *++from;
	case 7:         *++to = *++from;
	case 6:         *++to = *++from;
	case 5:         *++to = *++from;
	case 4:         *++to = *++from;
	case 3:         *++to = *++from;
	case 2:         *++to = *++from;
	case 1:         *++to = *++from;
	}while(--n>0);
	}
    }
}

//-----------------------------------------------------------------------------

/** This helper function copies an array to another location using Duff's
    device (for a speedup of ~2). The source array is given by its first and
    "after last" entry; the target array is given by its first entry.
    Overlapping arrays are handled correctly.

    All of the various CoinCopyN variants use an int for size. On 64-bit
    architectures, the address diff last-first will be a 64-bit quantity.
    Given that everything else uses an int, I'm going to choose to kick
    the difference down to int.  -- lh, 100823 --
*/
template <class T> inline void
CoinCopy(const T* first, const T* last, T* to)
{
    CoinCopyN(first, static_cast<int>(last-first), to);
}

//-----------------------------------------------------------------------------

/** This helper function copies an array to another location. The two arrays
    must not overlap (otherwise an exception is thrown). For speed 8 entries
    are copied at a time. The arrays are given by pointers to their first
    entries and by the size of the source array.

    Note JJF - the speed claim seems to be false on IA32 so I have added
    CoinMemcpyN which can be used for atomic data */
template <class T> inline void
CoinDisjointCopyN(const T* from, const int size, T* to)
{
#ifndef _MSC_VER
    if (size == 0 || from == to)
	return;

#ifndef NDEBUG
    if (size < 0)
	throw CoinError("trying to copy negative number of entries",
			"CoinDisjointCopyN", "");
#endif

#if 0
    /* There is no point to do this test. If to and from are from different
       blocks then dist is undefined, so this can crash correct code. It's
       better to trust the user that the arrays are really disjoint. */
    const long dist = to - from;
    if (-size < dist && dist < size)
	throw CoinError("overlapping arrays", "CoinDisjointCopyN", "");
#endif

    for (int n = size / 8; n > 0; --n, from += 8, to += 8) {
	to[0] = from[0];
	to[1] = from[1];
	to[2] = from[2];
	to[3] = from[3];
	to[4] = from[4];
	to[5] = from[5];
	to[6] = from[6];
	to[7] = from[7];
    }
    switch (size % 8) {
    case 7: to[6] = from[6];
    case 6: to[5] = from[5];
    case 5: to[4] = from[4];
    case 4: to[3] = from[3];
    case 3: to[2] = from[2];
    case 2: to[1] = from[1];
    case 1: to[0] = from[0];
    case 0: break;
    }
#else
    CoinCopyN(from, size, to);
#endif
}

//-----------------------------------------------------------------------------

/** This helper function copies an array to another location. The two arrays
    must not overlap (otherwise an exception is thrown). For speed 8 entries
    are copied at a time. The source array is given by its first and "after
    last" entry; the target array is given by its first entry. */
template <class T> inline void
CoinDisjointCopy(const T* first, const T* last, T* to)
{
    CoinDisjointCopyN(first, static_cast<int>(last - first), to);
}

//-----------------------------------------------------------------------------

/*! \brief Return an array of length \p size filled with input from \p array,
  or null if \p array is null.
*/

template <class T> inline T*
CoinCopyOfArray( const T * array, const int size)
{
    if (array) {
	T * arrayNew = new T[size];
	std::memcpy(arrayNew,array,size*sizeof(T));
	return arrayNew;
    } else {
	return NULL;
    }
}


/*! \brief Return an array of length \p size filled with first copySize from \p array,
  or null if \p array is null.
*/

template <class T> inline T*
CoinCopyOfArrayPartial( const T * array, const int size,const int copySize)
{
    if (array||size) {
	T * arrayNew = new T[size];
	assert (copySize<=size);
	std::memcpy(arrayNew,array,copySize*sizeof(T));
	return arrayNew;
    } else {
	return NULL;
    }
}

/*! \brief Return an array of length \p size filled with input from \p array,
  or filled with (scalar) \p value if \p array is null
*/

template <class T> inline T*
CoinCopyOfArray( const T * array, const int size, T value)
{
    T * arrayNew = new T[size];
    if (array) {
        std::memcpy(arrayNew,array,size*sizeof(T));
    } else {
	int i;
	for (i=0;i<size;i++)
	    arrayNew[i] = value;
    }
    return arrayNew;
}


/*! \brief Return an array of length \p size filled with input from \p array,
  or filled with zero if \p array is null
*/

template <class T> inline T*
CoinCopyOfArrayOrZero( const T * array , const int size)
{
    T * arrayNew = new T[size];
    if (array) {
      std::memcpy(arrayNew,array,size*sizeof(T));
    } else {
      std::memset(arrayNew,0,size*sizeof(T));
    }
    return arrayNew;
}


//-----------------------------------------------------------------------------

/** This helper function copies an array to another location. The two arrays
    must not overlap (otherwise an exception is thrown). For speed 8 entries
    are copied at a time. The arrays are given by pointers to their first
    entries and by the size of the source array.

    Note JJF - the speed claim seems to be false on IA32 so I have added
    alternative coding if USE_MEMCPY defined*/
#ifndef COIN_USE_RESTRICT
template <class T> inline void
CoinMemcpyN(register const T* from, const int size, register T* to)
{
#ifndef _MSC_VER
#ifdef USE_MEMCPY
    // Use memcpy - seems a lot faster on Intel with gcc
#ifndef NDEBUG
    // Some debug so check
    if (size < 0)
	throw CoinError("trying to copy negative number of entries",
			"CoinMemcpyN", "");

#if 0
    /* There is no point to do this test. If to and from are from different
       blocks then dist is undefined, so this can crash correct code. It's
       better to trust the user that the arrays are really disjoint. */
    const long dist = to - from;
    if (-size < dist && dist < size)
	throw CoinError("overlapping arrays", "CoinMemcpyN", "");
#endif
#endif
    std::memcpy(to,from,size*sizeof(T));
#else
    if (size == 0 || from == to)
	return;

#ifndef NDEBUG
    if (size < 0)
	throw CoinError("trying to copy negative number of entries",
			"CoinMemcpyN", "");
#endif

#if 0
    /* There is no point to do this test. If to and from are from different
       blocks then dist is undefined, so this can crash correct code. It's
       better to trust the user that the arrays are really disjoint. */
    const long dist = to - from;
    if (-size < dist && dist < size)
	throw CoinError("overlapping arrays", "CoinMemcpyN", "");
#endif

    for (int n = size / 8; n > 0; --n, from += 8, to += 8) {
	to[0] = from[0];
	to[1] = from[1];
	to[2] = from[2];
	to[3] = from[3];
	to[4] = from[4];
	to[5] = from[5];
	to[6] = from[6];
	to[7] = from[7];
    }
    switch (size % 8) {
    case 7: to[6] = from[6];
    case 6: to[5] = from[5];
    case 5: to[4] = from[4];
    case 4: to[3] = from[3];
    case 3: to[2] = from[2];
    case 2: to[1] = from[1];
    case 1: to[0] = from[0];
    case 0: break;
    }
#endif
#else
    CoinCopyN(from, size, to);
#endif
}
#else
template <class T> inline void
CoinMemcpyN(const T * COIN_RESTRICT from, int size, T* COIN_RESTRICT to)
{
#ifdef USE_MEMCPY
  std::memcpy(to,from,size*sizeof(T));
#else
  T * COIN_RESTRICT put =  to;
  const T * COIN_RESTRICT get = from;
  for ( ; 0<size ; --size)
    *put++ = *get++;
#endif
}
#endif

//-----------------------------------------------------------------------------

/** This helper function copies an array to another location. The two arrays
    must not overlap (otherwise an exception is thrown). For speed 8 entries
    are copied at a time. The source array is given by its first and "after
    last" entry; the target array is given by its first entry. */
template <class T> inline void
CoinMemcpy(const T* first, const T* last, T* to)
{
    CoinMemcpyN(first, static_cast<int>(last - first), to);
}

//#############################################################################

/** This helper function fills an array with a given value. For speed 8 entries
    are filled at a time. The array is given by a pointer to its first entry
    and its size.

    Note JJF - the speed claim seems to be false on IA32 so I have added
    CoinZero to allow for memset. */
template <class T> inline void
CoinFillN(T* to, const int size, const T value)
{
    if (size == 0)
	return;

#ifndef NDEBUG
    if (size < 0)
	throw CoinError("trying to fill negative number of entries",
			"CoinFillN", "");
#endif
#if 1
    for (int n = size / 8; n > 0; --n, to += 8) {
	to[0] = value;
	to[1] = value;
	to[2] = value;
	to[3] = value;
	to[4] = value;
	to[5] = value;
	to[6] = value;
	to[7] = value;
    }
    switch (size % 8) {
    case 7: to[6] = value;
    case 6: to[5] = value;
    case 5: to[4] = value;
    case 4: to[3] = value;
    case 3: to[2] = value;
    case 2: to[1] = value;
    case 1: to[0] = value;
    case 0: break;
    }
#else
    // Use Duff's device to fill
    int n = (size + 7) / 8;
    --to;
    switch (size % 8) {
    case 0: do{     *++to = value;
    case 7:         *++to = value;
    case 6:         *++to = value;
    case 5:         *++to = value;
    case 4:         *++to = value;
    case 3:         *++to = value;
    case 2:         *++to = value;
    case 1:         *++to = value;
    }while(--n>0);
    }
#endif
}

//-----------------------------------------------------------------------------

/** This helper function fills an array with a given value. For speed 8
    entries are filled at a time. The array is given by its first and "after
    last" entry. */
template <class T> inline void
CoinFill(T* first, T* last, const T value)
{
    CoinFillN(first, last - first, value);
}

//#############################################################################

/** This helper function fills an array with zero. For speed 8 entries
    are filled at a time. The array is given by a pointer to its first entry
    and its size.

    Note JJF - the speed claim seems to be false on IA32 so I have allowed
    for memset as an alternative */
template <class T> inline void
CoinZeroN(T* to, const int size)
{
#ifdef USE_MEMCPY
    // Use memset - seems faster on Intel with gcc
#ifndef NDEBUG
    // Some debug so check
    if (size < 0)
	throw CoinError("trying to fill negative number of entries",
			"CoinZeroN", "");
#endif
    memset(to,0,size*sizeof(T));
#else
    if (size == 0)
	return;

#ifndef NDEBUG
    if (size < 0)
	throw CoinError("trying to fill negative number of entries",
			"CoinZeroN", "");
#endif
#if 1
    for (int n = size / 8; n > 0; --n, to += 8) {
	to[0] = 0;
	to[1] = 0;
	to[2] = 0;
	to[3] = 0;
	to[4] = 0;
	to[5] = 0;
	to[6] = 0;
	to[7] = 0;
    }
    switch (size % 8) {
    case 7: to[6] = 0;
    case 6: to[5] = 0;
    case 5: to[4] = 0;
    case 4: to[3] = 0;
    case 3: to[2] = 0;
    case 2: to[1] = 0;
    case 1: to[0] = 0;
    case 0: break;
    }
#else
    // Use Duff's device to fill
    int n = (size + 7) / 8;
    --to;
    switch (size % 8) {
    case 0: do{     *++to = 0;
    case 7:         *++to = 0;
    case 6:         *++to = 0;
    case 5:         *++to = 0;
    case 4:         *++to = 0;
    case 3:         *++to = 0;
    case 2:         *++to = 0;
    case 1:         *++to = 0;
    }while(--n>0);
    }
#endif
#endif
}
/// This Debug helper function checks an array is all zero
inline void
CoinCheckDoubleZero(double * to, const int size)
{
    int n=0;
    for (int j=0;j<size;j++) {
	if (to[j])
	    n++;
    }
    if (n) {
	printf("array of length %d should be zero has %d nonzero\n",size,n);
    }
}
/// This Debug helper function checks an array is all zero
inline void
CoinCheckIntZero(int * to, const int size)
{
    int n=0;
    for (int j=0;j<size;j++) {
	if (to[j])
	    n++;
    }
    if (n) {
	printf("array of length %d should be zero has %d nonzero\n",size,n);
    }
}

//-----------------------------------------------------------------------------

/** This helper function fills an array with a given value. For speed 8
    entries are filled at a time. The array is given by its first and "after
    last" entry. */
template <class T> inline void
CoinZero(T* first, T* last)
{
    CoinZeroN(first, last - first);
}

//#############################################################################

/** Returns strdup or NULL if original NULL */
inline char * CoinStrdup(const char * name)
{
  char* dup = NULL;
  if (name) {
    const int len = static_cast<int>(strlen(name));
    dup = static_cast<char*>(malloc(len+1));
    CoinMemcpyN(name, len, dup);
    dup[len] = 0;
  }
  return dup;
}

//#############################################################################

/** Return the larger (according to <code>operator<()</code> of the arguments.
    This function was introduced because for some reason compiler tend to
    handle the <code>max()</code> function differently. */
template <class T> inline T
CoinMax(const T x1, const T x2)
{
    return (x1 > x2) ? x1 : x2;
}

//-----------------------------------------------------------------------------

/** Return the smaller (according to <code>operator<()</code> of the arguments.
    This function was introduced because for some reason compiler tend to
    handle the min() function differently. */
template <class T> inline T
CoinMin(const T x1, const T x2)
{
    return (x1 < x2) ? x1 : x2;
}

//-----------------------------------------------------------------------------

/** Return the absolute value of the argument. This function was introduced
    because for some reason compiler tend to handle the abs() function
    differently. */
template <class T> inline T
CoinAbs(const T value)
{
    return value<0 ? -value : value;
}

//#############################################################################

/** This helper function tests whether the entries of an array are sorted
    according to operator<. The array is given by a pointer to its first entry
    and by its size. */
template <class T> inline bool
CoinIsSorted(const T* first, const int size)
{
    if (size == 0)
	return true;

#ifndef NDEBUG
    if (size < 0)
	throw CoinError("negative number of entries", "CoinIsSorted", "");
#endif
#if 1
    // size1 is the number of comparisons to be made
    const int size1 = size  - 1;
    for (int n = size1 / 8; n > 0; --n, first += 8) {
	if (first[8] < first[7]) return false;
	if (first[7] < first[6]) return false;
	if (first[6] < first[5]) return false;
	if (first[5] < first[4]) return false;
	if (first[4] < first[3]) return false;
	if (first[3] < first[2]) return false;
	if (first[2] < first[1]) return false;
	if (first[1] < first[0]) return false;
    }

    switch (size1 % 8) {
    case 7: if (first[7] < first[6]) return false;
    case 6: if (first[6] < first[5]) return false;
    case 5: if (first[5] < first[4]) return false;
    case 4: if (first[4] < first[3]) return false;
    case 3: if (first[3] < first[2]) return false;
    case 2: if (first[2] < first[1]) return false;
    case 1: if (first[1] < first[0]) return false;
    case 0: break;
    }
#else
    const T* next = first;
    const T* last = first + size;
    for (++next; next != last; first = next, ++next)
	if (*next < *first)
	    return false;
#endif
    return true;
}

//-----------------------------------------------------------------------------

/** This helper function tests whether the entries of an array are sorted
    according to operator<. The array is given by its first and "after
    last" entry. */
template <class T> inline bool
CoinIsSorted(const T* first, const T* last)
{
    return CoinIsSorted(first, static_cast<int>(last - first));
}

//#############################################################################

/** This helper function fills an array with the values init, init+1, init+2,
    etc. For speed 8 entries are filled at a time. The array is given by a
    pointer to its first entry and its size. */
template <class T> inline void
CoinIotaN(T* first, const int size, T init)
{
    if (size == 0)
	return;

#ifndef NDEBUG
    if (size < 0)
	throw CoinError("negative number of entries", "CoinIotaN", "");
#endif
#if 1
    for (int n = size / 8; n > 0; --n, first += 8, init += 8) {
	first[0] = init;
	first[1] = init + 1;
	first[2] = init + 2;
	first[3] = init + 3;
	first[4] = init + 4;
	first[5] = init + 5;
	first[6] = init + 6;
	first[7] = init + 7;
    }
    switch (size % 8) {
    case 7: first[6] = init + 6;
    case 6: first[5] = init + 5;
    case 5: first[4] = init + 4;
    case 4: first[3] = init + 3;
    case 3: first[2] = init + 2;
    case 2: first[1] = init + 1;
    case 1: first[0] = init;
    case 0: break;
    }
#else
    // Use Duff's device to fill
    int n = (size + 7) / 8;
    --first;
    --init;
    switch (size % 8) {
    case 0: do{     *++first = ++init;
    case 7:         *++first = ++init;
    case 6:         *++first = ++init;
    case 5:         *++first = ++init;
    case 4:         *++first = ++init;
    case 3:         *++first = ++init;
    case 2:         *++first = ++init;
    case 1:         *++first = ++init;
    }while(--n>0);
    }
#endif
}

//-----------------------------------------------------------------------------

/** This helper function fills an array with the values init, init+1, init+2,
    etc. For speed 8 entries are filled at a time. The array is given by its
    first and "after last" entry. */
template <class T> inline void
CoinIota(T* first, const T* last, T init)
{
    CoinIotaN(first, last-first, init);
}

//#############################################################################

/** This helper function deletes certain entries from an array. The array is
    given by pointers to its first and "after last" entry (first two
    arguments). The positions of the entries to be deleted are given in the
    integer array specified by the last two arguments (again, first and "after
    last" entry). */
template <class T> inline T *
CoinDeleteEntriesFromArray(T * arrayFirst, T * arrayLast,
			   const int * firstDelPos, const int * lastDelPos)
{
    int delNum = static_cast<int>(lastDelPos - firstDelPos);
    if (delNum == 0)
	return arrayLast;

    if (delNum < 0)
	throw CoinError("trying to delete negative number of entries",
			"CoinDeleteEntriesFromArray", "");

    int * delSortedPos = NULL;
    if (! (CoinIsSorted(firstDelPos, lastDelPos) &&
	   std::adjacent_find(firstDelPos, lastDelPos) == lastDelPos)) {
	// the positions of the to be deleted is either not sorted or not unique
	delSortedPos = new int[delNum];
	CoinDisjointCopy(firstDelPos, lastDelPos, delSortedPos);
	std::sort(delSortedPos, delSortedPos + delNum);
	delNum = static_cast<int>(std::unique(delSortedPos,
				  delSortedPos+delNum) - delSortedPos);
    }
    const int * delSorted = delSortedPos ? delSortedPos : firstDelPos;

    const int last = delNum - 1;
    int size = delSorted[0];
    for (int i = 0; i < last; ++i) {
	const int copyFirst = delSorted[i] + 1;
	const int copyLast = delSorted[i+1];
	CoinCopy(arrayFirst + copyFirst, arrayFirst + copyLast,
		 arrayFirst + size);
	size += copyLast - copyFirst;
    }
    const int copyFirst = delSorted[last] + 1;
    const int copyLast = static_cast<int>(arrayLast - arrayFirst);
    CoinCopy(arrayFirst + copyFirst, arrayFirst + copyLast,
	     arrayFirst + size);
    size += copyLast - copyFirst;

    if (delSortedPos)
	delete[] delSortedPos;

    return arrayFirst + size;
}

//#############################################################################

#define COIN_OWN_RANDOM_32

#if defined COIN_OWN_RANDOM_32
/* Thanks to Stefano Gliozzi for providing an operating system
   independent random number generator.  */

/*! \brief Return a random number between 0 and 1

  A platform-independent linear congruential generator. For a given seed, the
  generated sequence is always the same regardless of the (32-bit)
  architecture. This allows to build & test in different environments, getting
  in most cases the same optimization path.

  Set \p isSeed to true and supply an integer seed to set the seed
  (vid. #CoinSeedRandom)

  \todo Anyone want to volunteer an upgrade for 64-bit architectures?
*/
inline double CoinDrand48 (bool isSeed = false, unsigned int seed = 1)
{
  static unsigned int last = 123456;
  if (isSeed) {
    last = seed;
  } else {
    last = 1664525*last+1013904223;
    return ((static_cast<double> (last))/4294967296.0);
  }
  return (0.0);
}

/// Set the seed for the random number generator
inline void CoinSeedRandom(int iseed)
{
  CoinDrand48(true, iseed);
}

#else // COIN_OWN_RANDOM_32

#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__CYGWIN32__)

/// Return a random number between 0 and 1
inline double CoinDrand48() { return rand() / (double) RAND_MAX; }
/// Set the seed for the random number generator
inline void CoinSeedRandom(int iseed) { srand(iseed + 69822); }

#else

/// Return a random number between 0 and 1
inline double CoinDrand48() { return drand48(); }
/// Set the seed for the random number generator
inline void CoinSeedRandom(int iseed) { srand48(iseed + 69822); }

#endif

#endif // COIN_OWN_RANDOM_32

//#############################################################################

/** This function figures out whether file names should contain slashes or
    backslashes as directory separator */
inline char CoinFindDirSeparator()
{
    int size = 1000;
    char* buf = 0;
    while (true) {
	buf = new char[size];
	if (getcwd(buf, size))
	    break;
	delete[] buf;
	buf = 0;
	size = 2*size;
    }
    // if first char is '/' then it's unix and the dirsep is '/'. otherwise we
    // assume it's dos and the dirsep is '\'
    char dirsep = buf[0] == '/' ? '/' : '\\';
    delete[] buf;
    return dirsep;
}
//#############################################################################

inline int CoinStrNCaseCmp(const char* s0, const char* s1,
			   const size_t len)
{
    for (size_t i = 0; i < len; ++i) {
	if (s0[i] == 0) {
	    return s1[i] == 0 ? 0 : -1;
	}
	if (s1[i] == 0) {
	    return 1;
	}
	const int c0 = tolower(s0[i]);
	const int c1 = tolower(s1[i]);
	if (c0 < c1)
	    return -1;
	if (c0 > c1)
	    return 1;
    }
    return 0;
}

//#############################################################################

/// Swap the arguments.
template <class T> inline void CoinSwap (T &x, T &y)
{
    T t = x;
    x = y;
    y = t;
}

//#############################################################################

/** This helper function copies an array to file
    Returns 0 if OK, 1 if bad write.
*/

template <class T> inline int
CoinToFile( const T* array, CoinBigIndex size, FILE * fp)
{
    CoinBigIndex numberWritten;
    if (array&&size) {
	numberWritten =
	    static_cast<CoinBigIndex>(fwrite(&size,sizeof(int),1,fp));
	if (numberWritten!=1)
	    return 1;
	numberWritten =
	    static_cast<CoinBigIndex>(fwrite(array,sizeof(T),size_t(size),fp));
	if (numberWritten!=size)
	    return 1;
    } else {
	size = 0;
	numberWritten =
	    static_cast<CoinBigIndex>(fwrite(&size,sizeof(int),1,fp));
	if (numberWritten!=1)
	    return 1;
    }
    return 0;
}

//#############################################################################

/** This helper function copies an array from file and creates with new.
    Passed in array is ignored i.e. not deleted.
    But if NULL and size does not match and newSize 0 then leaves as NULL and 0
    Returns 0 if OK, 1 if bad read, 2 if size did not match.
*/

template <class T> inline int
CoinFromFile( T* &array, CoinBigIndex size, FILE * fp, CoinBigIndex & newSize)
{
    CoinBigIndex numberRead;
    numberRead =
        static_cast<CoinBigIndex>(fread(&newSize,sizeof(int),1,fp));
    if (numberRead!=1)
	return 1;
    int returnCode=0;
    if (size!=newSize&&(newSize||array))
	returnCode=2;
    if (newSize) {
	array = new T [newSize];
	numberRead =
	    static_cast<CoinBigIndex>(fread(array,sizeof(T),newSize,fp));
	if (numberRead!=newSize)
	    returnCode=1;
    } else {
	array = NULL;
    }
    return returnCode;
}

//#############################################################################

/// Cube Root
#if 0
inline double CoinCbrt(double x)
{
#if defined(_MSC_VER)
    return pow(x,(1./3.));
#else
    return cbrt(x);
#endif
}
#endif

//-----------------------------------------------------------------------------

/// This helper returns "sizeof" as an int
#define CoinSizeofAsInt(type) (static_cast<int>(sizeof(type)))
/// This helper returns "strlen" as an int
inline int
CoinStrlenAsInt(const char * string)
{
    return static_cast<int>(strlen(string));
}

/** Class for thread specific random numbers
*/
#if defined COIN_OWN_RANDOM_32
class CoinThreadRandom  {
public:
  /**@name Constructors, destructor */

  //@{
  /** Default constructor. */
  CoinThreadRandom()
  { seed_=12345678;}
  /** Constructor wih seed. */
  CoinThreadRandom(int seed)
  {
    seed_ = seed;
  }
  /** Destructor */
  ~CoinThreadRandom() {}
  // Copy
  CoinThreadRandom(const CoinThreadRandom & rhs)
  { seed_ = rhs.seed_;}
  // Assignment
  CoinThreadRandom& operator=(const CoinThreadRandom & rhs)
  {
    if (this != &rhs) {
      seed_ = rhs.seed_;
    }
    return *this;
  }

  //@}

  /**@name Sets/gets */

  //@{
  /** Set seed. */
  inline void setSeed(int seed)
  {
    seed_ = seed;
  }
  /** Get seed. */
  inline unsigned int getSeed() const
  {
    return seed_;
  }
  /// return a random number
  inline double randomDouble() const
  {
    double retVal;
    seed_ = 1664525*(seed_)+1013904223;
    retVal = ((static_cast<double> (seed_))/4294967296.0);
    return retVal;
  }
  /// make more random (i.e. for startup)
  inline void randomize(int n=0)
  {
    if (!n)
      n=seed_ & 255;
    for (int i=0;i<n;i++)
      randomDouble();
  }
  //@}


protected:
  /**@name Data members
     The data members are protected to allow access for derived classes. */
  //@{
  /// Current seed
  mutable unsigned int seed_;
  //@}
};
#else
class CoinThreadRandom  {
public:
  /**@name Constructors, destructor */

  //@{
  /** Default constructor. */
  CoinThreadRandom()
  { seed_[0]=50000;seed_[1]=40000;seed_[2]=30000;}
  /** Constructor wih seed. */
  CoinThreadRandom(const unsigned short seed[3])
  { memcpy(seed_,seed,3*sizeof(unsigned short));}
  /** Constructor wih seed. */
  CoinThreadRandom(int seed)
  {
    union { int i[2]; unsigned short int s[4];} put;
    put.i[0]=seed;
    put.i[1]=seed;
    memcpy(seed_,put.s,3*sizeof(unsigned short));
  }
  /** Destructor */
  ~CoinThreadRandom() {}
  // Copy
  CoinThreadRandom(const CoinThreadRandom & rhs)
  { memcpy(seed_,rhs.seed_,3*sizeof(unsigned short));}
  // Assignment
  CoinThreadRandom& operator=(const CoinThreadRandom & rhs)
  {
    if (this != &rhs) {
      memcpy(seed_,rhs.seed_,3*sizeof(unsigned short));
    }
    return *this;
  }

  //@}

  /**@name Sets/gets */

  //@{
  /** Set seed. */
  inline void setSeed(const unsigned short seed[3])
  { memcpy(seed_,seed,3*sizeof(unsigned short));}
  /** Set seed. */
  inline void setSeed(int seed)
  {
    union { int i[2]; unsigned short int s[4];} put;
    put.i[0]=seed;
    put.i[1]=seed;
    memcpy(seed_,put.s,3*sizeof(unsigned short));
  }
  /// return a random number
  inline double randomDouble() const
  {
    double retVal;
#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__CYGWIN32__)
    retVal=rand();
    retVal=retVal/(double) RAND_MAX;
#else
    retVal = erand48(seed_);
#endif
    return retVal;
  }
  /// make more random (i.e. for startup)
  inline void randomize(int n=0)
  {
    if (!n) {
      n=seed_[0]+seed_[1]+seed_[2];
      n &= 255;
    }
    for (int i=0;i<n;i++)
      randomDouble();
  }
  //@}


protected:
  /**@name Data members
     The data members are protected to allow access for derived classes. */
  //@{
  /// Current seed
  mutable unsigned short seed_[3];
  //@}
};
#endif
#ifndef COIN_DETAIL
#define COIN_DETAIL_PRINT(s) {}
#else
#define COIN_DETAIL_PRINT(s) s
#endif
#endif
