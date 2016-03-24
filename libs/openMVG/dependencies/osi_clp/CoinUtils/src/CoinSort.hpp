/* $Id: CoinSort.hpp 1596 2013-04-25 14:29:25Z stefan $ */
// Copyright (C) 2000, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef CoinSort_H
#define CoinSort_H

#include <functional>
#include <new>
#include <algorithm>
#include "CoinDistance.hpp"

// Uncomment the next three lines to get thorough initialisation of memory.
// #ifndef ZEROFAULT
// #define ZEROFAULT
// #endif

#ifdef COIN_FAST_CODE
#ifndef COIN_USE_EKK_SORT
#define COIN_USE_EKK_SORT
#endif
#endif

//#############################################################################

/** An ordered pair. It's the same as std::pair, just this way it'll have the
    same look as the triple sorting. */
template <class S, class T>
struct CoinPair {
public:
  /// First member of pair
  S first;
  /// Second member of pair
  T second;
public:
  /// Construct from ordered pair
  CoinPair(const S& s, const T& t) : first(s), second(t) {}
};

//#############################################################################

/**@name Comparisons on first element of two ordered pairs */
//@{   
/** Function operator.
    Returns true if t1.first &lt; t2.first (i.e., increasing). */
template < class S, class T>
class CoinFirstLess_2 {
public:
  /// Compare function
  inline bool operator()(const CoinPair<S,T>& t1,
			 const CoinPair<S,T>& t2) const
  { return t1.first < t2.first; }
};
//-----------------------------------------------------------------------------
/** Function operator.
    Returns true if t1.first &gt; t2.first (i.e, decreasing). */
template < class S, class T>
class CoinFirstGreater_2 {
public:
  /// Compare function
  inline bool operator()(const CoinPair<S,T>& t1,
			 const CoinPair<S,T>& t2) const
  { return t1.first > t2.first; }
};
//-----------------------------------------------------------------------------
/** Function operator.
    Returns true if abs(t1.first) &lt; abs(t2.first) (i.e., increasing). */
template < class S, class T>
class CoinFirstAbsLess_2 {
public:
  /// Compare function
  inline bool operator()(const CoinPair<S,T>& t1,
			 const CoinPair<S,T>& t2) const
  { 
    const T t1Abs = t1.first < static_cast<T>(0) ? -t1.first : t1.first;
    const T t2Abs = t2.first < static_cast<T>(0) ? -t2.first : t2.first;
    return t1Abs < t2Abs; 
  }
};
//-----------------------------------------------------------------------------
/** Function operator.
    Returns true if abs(t1.first) &gt; abs(t2.first) (i.e., decreasing). */
template < class S, class T>
class CoinFirstAbsGreater_2 {
public:
  /// Compare function
  inline bool operator()(CoinPair<S,T> t1, CoinPair<S,T> t2) const
  { 
    const T t1Abs = t1.first < static_cast<T>(0) ? -t1.first : t1.first;
    const T t2Abs = t2.first < static_cast<T>(0) ? -t2.first : t2.first;
    return t1Abs > t2Abs; 
  }
};
//-----------------------------------------------------------------------------
/** Function operator.
    Compare based on the entries of an external vector, i.e., returns true if
    vec[t1.first &lt; vec[t2.first] (i.e., increasing wrt. vec). Note that to
    use this comparison operator .first must be a data type automatically
    convertible to int. */
template < class S, class T, class V>
class CoinExternalVectorFirstLess_2 {
private:
  CoinExternalVectorFirstLess_2();
private:
  const V* vec_;
public:
  inline bool operator()(const CoinPair<S,T>& t1,
			 const CoinPair<S,T>& t2) const
  { return vec_[t1.first] < vec_[t2.first]; }
  CoinExternalVectorFirstLess_2(const V* v) : vec_(v) {}
};
//-----------------------------------------------------------------------------
/** Function operator.
    Compare based on the entries of an external vector, i.e., returns true if
    vec[t1.first &gt; vec[t2.first] (i.e., decreasing wrt. vec). Note that to
    use this comparison operator .first must be a data type automatically
    convertible to int. */
template < class S, class T, class V>
class CoinExternalVectorFirstGreater_2 {
private:
  CoinExternalVectorFirstGreater_2();
private:
  const V* vec_;
public:
  inline bool operator()(const CoinPair<S,T>& t1,
			 const CoinPair<S,T>& t2) const
  { return vec_[t1.first] > vec_[t2.first]; }
  CoinExternalVectorFirstGreater_2(const V* v) : vec_(v) {}
};
//@}

//#############################################################################

/** Sort a pair of containers.<br>

    Iter_S - iterator for first container<br>
    Iter_T - iterator for 2nd container<br>
    CoinCompare2 - class comparing CoinPairs<br>
*/

#ifdef COIN_SORT_ARBITRARY_CONTAINERS
template <class Iter_S, class Iter_T, class CoinCompare2> void
CoinSort_2(Iter_S sfirst, Iter_S slast, Iter_T tfirst, const CoinCompare2& pc)
{
  typedef typename std::iterator_traits<Iter_S>::value_type S;
  typedef typename std::iterator_traits<Iter_T>::value_type T;
  const size_t len = coinDistance(sfirst, slast);
  if (len <= 1)
    return;

  typedef CoinPair<S,T> ST_pair;
  ST_pair* x = static_cast<ST_pair*>(::operator new(len * sizeof(ST_pair)));
# ifdef ZEROFAULT
  memset(x,0,(len*sizeof(ST_pair))) ;
# endif

  int i = 0;
  Iter_S scurrent = sfirst;
  Iter_T tcurrent = tfirst;
  while (scurrent != slast) {
    new (x+i++) ST_pair(*scurrent++, *tcurrent++);
  }

  std::sort(x.begin(), x.end(), pc);

  scurrent = sfirst;
  tcurrent = tfirst;
  for (i = 0; i < len; ++i) {
    *scurrent++ = x[i].first;
    *tcurrent++ = x[i].second;
  }

  ::operator delete(x);
}
//-----------------------------------------------------------------------------
template <class Iter_S, class Iter_T> void
CoinSort_2(Iter_S sfirst, Iter_S slast, Iter_T tfirst)
{
  typedef typename std::iterator_traits<Iter_S>::value_type S;
  typedef typename std::iterator_traits<Iter_T>::value_type T;
  CoinSort_2(sfirst, slast, tfirst, CoinFirstLess_2<S,T>());
}

#else //=======================================================================

template <class S, class T, class CoinCompare2> void
CoinSort_2(S* sfirst, S* slast, T* tfirst, const CoinCompare2& pc)
{
  const size_t len = coinDistance(sfirst, slast);
  if (len <= 1)
    return;

  typedef CoinPair<S,T> ST_pair;
  ST_pair* x = static_cast<ST_pair*>(::operator new(len * sizeof(ST_pair)));
# ifdef ZEROFAULT
  // Can show RUI errors on some systems due to copy of ST_pair with gaps.
  // E.g., <int, double> has 4 byte alignment gap on Solaris/SUNWspro.
  memset(x,0,(len*sizeof(ST_pair))) ;
# endif

  size_t i = 0;
  S* scurrent = sfirst;
  T* tcurrent = tfirst;
  while (scurrent != slast) {
    new (x+i++) ST_pair(*scurrent++, *tcurrent++);
  }

  std::sort(x, x + len, pc);

  scurrent = sfirst;
  tcurrent = tfirst;
  for (i = 0; i < len; ++i) {
    *scurrent++ = x[i].first;
    *tcurrent++ = x[i].second;
  }

  ::operator delete(x);
}
template <class S, class T> void
// This Always uses std::sort
CoinSort_2Std(S* sfirst, S* slast, T* tfirst)
{
  CoinSort_2(sfirst, slast, tfirst, CoinFirstLess_2<S,T>());
}
#ifndef COIN_USE_EKK_SORT
//-----------------------------------------------------------------------------
template <class S, class T> void
CoinSort_2(S* sfirst, S* slast, T* tfirst)
{
  CoinSort_2(sfirst, slast, tfirst, CoinFirstLess_2<S,T>());
}
#else
//-----------------------------------------------------------------------------
extern int boundary_sort;
extern int boundary_sort2;
extern int boundary_sort3;
/// Sort without new and delete
template <class S, class T> void
CoinSort_2(S* key, S* lastKey, T* array2)
{
  const size_t number = coinDistance(key, lastKey);
  if (number <= 1) {
    return;
  } else if (number>10000) {
    CoinSort_2Std(key, lastKey, array2);
    return;
  }
#if 0
  if (number==boundary_sort3) {
    printf("before sort %d entries\n",number);
    for (int j=0;j<number;j++) {
      std::cout<<" ( "<<key[j]<<","<<array2[j]<<")";
    }
    std::cout<<std::endl;
  }
#endif
  int minsize=10;
  int n = static_cast<int>(number);
  int sp;
  S *v = key;
  S *m, t;
  S * ls[32] , * rs[32];
  S *l , *r , c;
  T it;
  int j;
  /*check already sorted  */
  S last=key[0];
  for (j=1;j<n;j++) {
    if (key[j]>=last) {
      last=key[j];
    } else {
      break;
    } /* endif */
  } /* endfor */
  if (j==n) {
    return;
  } /* endif */
  sp = 0 ; ls[sp] = v ; rs[sp] = v + (n-1) ;
  while( sp >= 0 )
  {
     if ( rs[sp] - ls[sp] > minsize )
     {
        l = ls[sp] ; r = rs[sp] ; m = l + (r-l)/2 ;
        if ( *l > *m )
        {
           t = *l ; *l = *m ; *m = t ;
           it = array2[l-v] ; array2[l-v] = array2[m-v] ; array2[m-v] = it ;
        }
        if ( *m > *r )
        {
           t = *m ; *m = *r ; *r = t ;
           it = array2[m-v] ; array2[m-v] = array2[r-v] ; array2[r-v] = it ;
           if ( *l > *m )
           {
              t = *l ; *l = *m ; *m = t ;
              it = array2[l-v] ; array2[l-v] = array2[m-v] ; array2[m-v] = it ;
           }
        }
        c = *m ;
        while ( r - l > 1 )
        {
           while ( *(++l) < c ) ;
           while ( *(--r) > c ) ;
           t = *l ; *l = *r ; *r = t ;
           it = array2[l-v] ; array2[l-v] = array2[r-v] ; array2[r-v] = it ;
        }
        l = r - 1 ;
        if ( l < m )
        {  ls[sp+1] = ls[sp] ;
           rs[sp+1] = l      ;
           ls[sp  ] = r      ;
        }
        else
        {  ls[sp+1] = r      ;
           rs[sp+1] = rs[sp] ;
           rs[sp  ] = l      ;
        }
        sp++ ;
     }
     else sp-- ;
  }
  for ( l = v , m = v + (n-1) ; l < m ; l++ )
  {  if ( *l > *(l+1) )
     {
        c = *(l+1) ;
        it = array2[(l-v)+1] ;
        for ( r = l ; r >= v && *r > c ; r-- )
        {
            *(r+1) = *r ;
            array2[(r-v)+1] = array2[(r-v)] ;
        }
        *(r+1) = c ;
        array2[(r-v)+1] = it ;
     }
  }
#if 0
  if (number==boundary_sort3) {
    printf("after sort %d entries\n",number);
    for (int j=0;j<number;j++) {
      std::cout<<" ( "<<key[j]<<","<<array2[j]<<")";
    }
    std::cout<<std::endl;
    CoinSort_2Many(key, lastKey, array2);
    printf("after2 sort %d entries\n",number);
    for (int j=0;j<number;j++) {
      std::cout<<" ( "<<key[j]<<","<<array2[j]<<")";
    }
    std::cout<<std::endl;
  }
#endif
}
#endif
#endif
/// Sort without new and delete
template <class S, class T> void
CoinShortSort_2(S* key, S* lastKey, T* array2)
{
  const size_t number = coinDistance(key, lastKey);
  if (number <= 2) {
    if (number == 2 && key[0] > key[1]) {
      S tempS = key[0];
      T tempT = array2[0];
      key[0] = key[1];
      array2[0] = array2[1];
      key[1] = tempS;
      array2[1] = tempT;
    } 
    return;
  } else if (number>10000) {
    CoinSort_2Std(key, lastKey, array2);
    return;
  }
  int minsize=10;
  size_t n = number;
  int sp;
  S *v = key;
  S *m, t;
  S * ls[32] , * rs[32];
  S *l , *r , c;
  T it;
  size_t j;
  /*check already sorted  */
  S last=key[0];
  for (j=1;j<n;j++) {
    if (key[j]>=last) {
      last=key[j];
    } else {
      break;
    } /* endif */
  } /* endfor */
  if (j==n) {
    return;
  } /* endif */
  sp = 0 ; ls[sp] = v ; rs[sp] = v + (n-1) ;
  while( sp >= 0 )
  {
     if ( rs[sp] - ls[sp] > minsize )
     {
        l = ls[sp] ; r = rs[sp] ; m = l + (r-l)/2 ;
        if ( *l > *m )
        {
           t = *l ; *l = *m ; *m = t ;
           it = array2[l-v] ; array2[l-v] = array2[m-v] ; array2[m-v] = it ;
        }
        if ( *m > *r )
        {
           t = *m ; *m = *r ; *r = t ;
           it = array2[m-v] ; array2[m-v] = array2[r-v] ; array2[r-v] = it ;
           if ( *l > *m )
           {
              t = *l ; *l = *m ; *m = t ;
              it = array2[l-v] ; array2[l-v] = array2[m-v] ; array2[m-v] = it ;
           }
        }
        c = *m ;
        while ( r - l > 1 )
        {
           while ( *(++l) < c ) ;
           while ( *(--r) > c ) ;
           t = *l ; *l = *r ; *r = t ;
           it = array2[l-v] ; array2[l-v] = array2[r-v] ; array2[r-v] = it ;
        }
        l = r - 1 ;
        if ( l < m )
        {  ls[sp+1] = ls[sp] ;
           rs[sp+1] = l      ;
           ls[sp  ] = r      ;
        }
        else
        {  ls[sp+1] = r      ;
           rs[sp+1] = rs[sp] ;
           rs[sp  ] = l      ;
        }
        sp++ ;
     }
     else sp-- ;
  }
  for ( l = v , m = v + (n-1) ; l < m ; l++ )
  {  if ( *l > *(l+1) )
     {
        c = *(l+1) ;
        it = array2[(l-v)+1] ;
        for ( r = l ; r >= v && *r > c ; r-- )
        {
            *(r+1) = *r ;
            array2[(r-v)+1] = array2[(r-v)] ;
        }
        *(r+1) = c ;
        array2[(r-v)+1] = it ;
     }
  }
}
//#############################################################################
//#############################################################################

/**@name Ordered Triple Struct */
template <class S, class T, class U>
class CoinTriple {
public:
  /// First member of triple
  S first;
  /// Second member of triple
  T second;
  /// Third member of triple
  U third;
public:  
  /// Construct from ordered triple
  CoinTriple(const S& s, const T& t, const U& u):first(s),second(t),third(u) {}
};

//#############################################################################
/**@name Comparisons on first element of two ordered triples */
//@{   
/** Function operator.
    Returns true if t1.first &lt; t2.first (i.e., increasing). */
template < class S, class T, class U >
class CoinFirstLess_3 {
public:
  /// Compare function
  inline bool operator()(const CoinTriple<S,T,U>& t1,
			 const CoinTriple<S,T,U>& t2) const
  { return t1.first < t2.first; }
};
//-----------------------------------------------------------------------------
/** Function operator.
    Returns true if t1.first &gt; t2.first (i.e, decreasing). */
template < class S, class T, class U >
class CoinFirstGreater_3 {
public:
  /// Compare function
  inline bool operator()(const CoinTriple<S,T,U>& t1,
			 const CoinTriple<S,T,U>& t2) const
  { return t1.first>t2.first; }
};
//-----------------------------------------------------------------------------
/** Function operator.
    Returns true if abs(t1.first) &lt; abs(t2.first) (i.e., increasing). */
template < class S, class T, class U >
class CoinFirstAbsLess_3 {
public:
  /// Compare function
  inline bool operator()(const CoinTriple<S,T,U>& t1,
			 const CoinTriple<S,T,U>& t2) const
  { 
    const T t1Abs = t1.first < static_cast<T>(0) ? -t1.first : t1.first;
    const T t2Abs = t2.first < static_cast<T>(0) ? -t2.first : t2.first;
    return t1Abs < t2Abs; 
  }
};
//-----------------------------------------------------------------------------
/** Function operator.
    Returns true if abs(t1.first) &gt; abs(t2.first) (i.e., decreasing). */
template < class S, class T, class U >
class CoinFirstAbsGreater_3 {
public:
  /// Compare function
  inline bool operator()(const CoinTriple<S,T,U>& t1,
			 const CoinTriple<S,T,U>& t2) const
  { 
    const T t1Abs = t1.first < static_cast<T>(0) ? -t1.first : t1.first;
    const T t2Abs = t2.first < static_cast<T>(0) ? -t2.first : t2.first;
    return t1Abs > t2Abs; 
  }
};
//-----------------------------------------------------------------------------
/** Function operator.
    Compare based on the entries of an external vector, i.e., returns true if
    vec[t1.first &lt; vec[t2.first] (i.e., increasing wrt. vec). Note that to
    use this comparison operator .first must be a data type automatically
    convertible to int. */
template < class S, class T, class U, class V>
class CoinExternalVectorFirstLess_3 {
private:
  CoinExternalVectorFirstLess_3();
private:
  const V* vec_;
public:
  inline bool operator()(const CoinTriple<S,T,U>& t1,
			 const CoinTriple<S,T,U>& t2) const
  { return vec_[t1.first] < vec_[t2.first]; }
  CoinExternalVectorFirstLess_3(const V* v) : vec_(v) {}
};
//-----------------------------------------------------------------------------
/** Function operator.
    Compare based on the entries of an external vector, i.e., returns true if
    vec[t1.first &gt; vec[t2.first] (i.e., decreasing wrt. vec). Note that to
    use this comparison operator .first must be a data type automatically
    convertible to int. */
template < class S, class T, class U, class V>
class CoinExternalVectorFirstGreater_3 {
private:
  CoinExternalVectorFirstGreater_3();
private:
  const V* vec_;
public:
  inline bool operator()(const CoinTriple<S,T,U>& t1,
			 const CoinTriple<S,T,U>& t2) const
  { return vec_[t1.first] > vec_[t2.first]; }
  CoinExternalVectorFirstGreater_3(const V* v) : vec_(v) {}
};
//@}

//#############################################################################

/**@name Typedefs for sorting the entries of a packed vector based on an
   external vector. */
//@{
/// Sort packed vector in increasing order of the external vector
typedef CoinExternalVectorFirstLess_3<int, int, double, double>
CoinIncrSolutionOrdered;
/// Sort packed vector in decreasing order of the external vector
typedef CoinExternalVectorFirstGreater_3<int, int, double, double>
CoinDecrSolutionOrdered;
//@}

//#############################################################################

/** Sort a triple of containers.<br>

    Iter_S - iterator for first container<br>
    Iter_T - iterator for 2nd container<br>
    Iter_U - iterator for 3rd container<br>
    CoinCompare3 - class comparing CoinTriples<br>
*/
#ifdef COIN_SORT_ARBITRARY_CONTAINERS
template <class Iter_S, class Iter_T, class Iter_U, class CoinCompare3> void
CoinSort_3(Iter_S sfirst, Iter_S slast, Iter_T tfirst, Iter_U, ufirst,
	  const CoinCompare3& tc)
{
  typedef typename std::iterator_traits<Iter_S>::value_type S;
  typedef typename std::iterator_traits<Iter_T>::value_type T;
  typedef typename std::iterator_traits<Iter_U>::value_type U;
  const size_t len = coinDistance(sfirst, slast);
  if (len <= 1)
    return;

  typedef CoinTriple<S,T,U> STU_triple;
  STU_triple* x =
    static_cast<STU_triple*>(::operator new(len * sizeof(STU_triple)));

  int i = 0;
  Iter_S scurrent = sfirst;
  Iter_T tcurrent = tfirst;
  Iter_U ucurrent = ufirst;
  while (scurrent != slast) {
    new (x+i++) STU_triple(*scurrent++, *tcurrent++, *ucurrent++);
  }

  std::sort(x, x+len, tc);

  scurrent = sfirst;
  tcurrent = tfirst;
  ucurrent = ufirst;
  for (i = 0; i < len; ++i) {
    *scurrent++ = x[i].first;
    *tcurrent++ = x[i].second;
    *ucurrent++ = x[i].third;
  }

  ::operator delete(x);
}
//-----------------------------------------------------------------------------
template <class Iter_S, class Iter_T, class Iter_U> void
CoinSort_3(Iter_S sfirst, Iter_S slast, Iter_T tfirst, Iter_U, ufirst)
{
  typedef typename std::iterator_traits<Iter_S>::value_type S;
  typedef typename std::iterator_traits<Iter_T>::value_type T;
  typedef typename std::iterator_traits<Iter_U>::value_type U;
  CoinSort_3(sfirts, slast, tfirst, ufirst, CoinFirstLess_3<S,T,U>());
}

#else //=======================================================================

template <class S, class T, class U, class CoinCompare3> void
CoinSort_3(S* sfirst, S* slast, T* tfirst, U* ufirst, const CoinCompare3& tc)
{
  const size_t len = coinDistance(sfirst,slast);
  if (len <= 1)
    return;

  typedef CoinTriple<S,T,U> STU_triple;
  STU_triple* x =
    static_cast<STU_triple*>(::operator new(len * sizeof(STU_triple)));

  size_t i = 0;
  S* scurrent = sfirst;
  T* tcurrent = tfirst;
  U* ucurrent = ufirst;
  while (scurrent != slast) {
    new (x+i++) STU_triple(*scurrent++, *tcurrent++, *ucurrent++);
  }

  std::sort(x, x+len, tc);

  scurrent = sfirst;
  tcurrent = tfirst;
  ucurrent = ufirst;
  for (i = 0; i < len; ++i) {
    *scurrent++ = x[i].first;
    *tcurrent++ = x[i].second;
    *ucurrent++ = x[i].third;
  }

  ::operator delete(x);
}
//-----------------------------------------------------------------------------
template <class S, class T, class U> void
CoinSort_3(S* sfirst, S* slast, T* tfirst, U* ufirst)
{
  CoinSort_3(sfirst, slast, tfirst, ufirst, CoinFirstLess_3<S,T,U>());
}

#endif

//#############################################################################

#endif
