// Copyright (c) 2007, 2008 libmv authors.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.

// Copyright (c) 2012, 2013 Pierre MOULON.

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENMVG_ROBUST_ESTIMATION_RAND_SAMPLING_H_
#define OPENMVG_ROBUST_ESTIMATION_RAND_SAMPLING_H_

#include <vector>
#include <stdlib.h>
#include <cmath>
#include <cassert>
#include <cstdlib> //rand

namespace openMVG {
namespace robust{


/**
* Pick a random subset of the integers [0, total), in random order.
* Note that this can behave badly if num_samples is close to total; runtime
* could be unlimited!
*
* This uses a quadratic rejection strategy and should only be used for small
* num_samples.
*
* \param num_samples   The number of samples to produce.
* \param total_samples The number of samples available.
* \param samples       num_samples of numbers in [0, total_samples) is placed
*                      here on return.
*/
inline void UniformSample(
  std::size_t num_samples,
  std::size_t total_samples,
  std::vector<std::size_t> *samples)
{
  samples->resize(0);
  while (samples->size() < num_samples) {
    std::size_t sample = std::size_t(std::rand() % total_samples);
    bool bFound = false;
    for (std::size_t j = 0; j < samples->size(); ++j) {
      bFound = (*samples)[j] == sample;
      if (bFound) { //the picked index already exist
        break;
      }
    }
    if (!bFound) {
      samples->push_back(sample);
    }
  }
}

/// Get a (sorted) random sample of size X in [0:n-1]
/// samples array must be pre-allocated
inline void random_sample(std::size_t X, std::size_t n, std::vector<std::size_t> *samples)
{
  samples->resize(X);
  for(std::size_t i=0; i < X; ++i) {
    std::size_t r = (std::rand()>>3)%(n-i), j;
    for(j=0; j<i && r>=(*samples)[j]; ++j)
      ++r;
    std::size_t j0 = j;
    for(j=i; j > j0; --j)
      (*samples)[j] = (*samples)[j-1];
    (*samples)[j0] = r;
  }
}

//////////////////////////////////////////////////////////////////////////////
/// Random combination picking
///

/* return a random number between 0 and n (0 and n included), with a uniform distribution
   reference: http://www.bourguet.org/v2/clang/random/ */
inline int alea(int n) {
  /* The following version is often used:
     return (n+1) * rand() / (RAND_MAX + 1.0);
     But it has several issues:
     * risk of overflow if RAND_MAX == MAX_INT (but using a floating-point
     addition fixes this)
     * part of the random bits are lost if RAND_MAX is bigger than the
     biggest integer that can be represented in a double (this is not
     really a problem with 64-bits double, but could be one with 32-bits
     float or 16-bits half)
     * but most importantly, the distribution is biased if RAND_MAX+1 is
     not a multiple of n+1 (the favored values are distributed regularly
     within the interval)
  */
  if (n == 0)
    return 0;

  assert (0 < n && n <= RAND_MAX);
  int partSize =
              n == RAND_MAX ? 1 : 1 + (RAND_MAX-n)/(n+1);
  int maxUsefull = partSize * n + (partSize-1);
  int draw;
  do {
    draw = std::rand();
  } while (draw > maxUsefull);
  int retval = draw/partSize;
  assert(retval >= 0);
  assert(retval <= n);
  return retval;
}

inline int aleamax() {
  return RAND_MAX;
}

/*

  deal: Choose a random k-subset of {0,1,...n-1}

  Author: Frederic Devernay, from public domain Fortran code.

  Synopsis:
  deal(int n, int k, int *a)

  Description:
  Chose a random subset of k elements from {0,1,...n-1}, and return these indices in
  increasing order in the k first elements of the a[] array.

  This function uses two different algorithms, depending on the respective values of k and n.

  If k > n/2, use a simple algorithm which picks up each sample with a probability k/n
  (RKS2 from [Nijenhuis & Wilk, p.43] or [Knuth 3.4.2]).
  If k <= n/2, use the ranksb algorithm.

  ranksb is a translation of the Fortran subroutine RANKSB which appears
  on pp 38-9, Chapter 4 of Nijenhuis & Wilk (1975) Combinatorial Algorithms, Academic Press.
  http://www.math.upenn.edu/~wilf/website/CombAlgDownld.html

  The original public domain Fortran source code is available at:
  http://www.cs.sunysb.edu/~algorith/implement/wilf/distrib/processed/
  The Fortran source code was translated using f2c, and then hand-tuned for C.

  Some comments by Gregory P. Jaxon, from
  https://groups.google.com/group/alt.math.recreational/msg/26f549d66c5de8c8

*/
/*  k << n, order(k) space & time */
inline void ranksb(int n, int k, std::vector<std::size_t> &a)
{
  assert(k <= (int)a.size());
  /* Local variables */
  int i, l, m = 0, p, r = 0, s, x, m0 = 0, ds;

  /* Function Body */
  if (k == 0)
    return;
  if (k == 1) {
    a[0] = alea(n-1);
    return;
  }


  /* Partition [0 : n-1] into k intervals:
     Store the least element of the I'th interval in a[i] */
  for (i = 0; i < k; ++i) {
    a[i] = i * n / k;
  }

  /* Using a uniformly distributed random variable in the
     range 0 <= x < n, make k selections of x such that
     x lands in the remaining portion of some interval.
     At each successful selection, reduce the remaining
     portion of the interval by 1. */
  /* If k is close to n (say, bigger than n/2), the
     while loop may take many iterations. For this reason,
     it is better to use another algorithm in these
     situations (see deal_k_near_n() below). */
  for(i = 0; i < k;  ++i) {
    do {
      x = alea(n-1) + 1;
      l = (x * k - 1) / n;
      // l is the choosen index in a.
      // a vector has size k.
      // x <= n ensures that l < k:
      // x == n:
      //   l = (n*k-1)/n < n*k/n (=k)
      // x == n+1:
      //   l = ((n+1) * k - 1 ) /n
      //     = (n*k + (k-1))/n >= n*k/n (=k)
      assert(l<k);
    } while ((std::size_t)x <= a[l]);
    ++a[l];
  }

  /* Collect the least elements of any interval which
     absorbed a selection in the previous step into the
     low-order indices of a. */
  p = -1;
  for (i = 0; i < k; ++i) {
    m = a[i];
    a[i] = 0;
    if (m != i * n / k) {
      /* A non-empty partition */
      ++p;
      a[p] = m;
    }
  }
  /* Allocate space for each non-empty partition starting
     from the high-order indices.  At the last position
     in each partition's segment, store the interval index
     of the partitions's successor. */
  s = k-1;
  for(; p >=0; p--) {
    l = (a[p] * k - 1) / n;
    ds = a[p] - l * n / k;
    a[p] = 0;
    a[s] = l + 1;
    s -= ds;
  }

  for(l = k-1; l >= 0; l--) {
    /* ranksb each of the sub-problems */
    x = a[l];
    if (x != 0) {
      /* Start a new bin */
      r = l;
      m0 = (x - 1) * n / k;
      m = x * n / k - m0;
      /* The order of arithmetic operations is important!
         The same rounding errors must be produced in each
         computation of a boundary. */
    }

    /* m0 is the least element of the current (l'th)
       interval.  m is the count of the number of
       unselected members of the interval. */
    x = m0 + alea(m-1);
    assert(x >= 0 && x < n);

    /* Bubble Merge the (x-base_l)'th unselected member
       of the current interval into the current interval's
       segment (a [l..r]). */

    i = l;
    while (i < r && (std::size_t)x >= a[i+1]) {
      a[i] = a[i+1];
      ++x;
      ++i;
    }
    assert(x >= 0 && x < n);
    a[i] = x;
    --m;
  }
} /* ranksb_ */

/* k near n, o(n) time, o(k) space (first loop is algorithm RKS2, Nijenhuis & Wilk p 43) */
inline void deal_k_near_n(int n, int k, std::vector<std::size_t> &a)
{
  assert(k <= (int)a.size());
  /* Warning: modifies k and n */
  /* Algorithm: go though all candidates from n-1 to 0, and pick each one with probability k/n */
  while((n > k) && (k > n/2)) {
    /* each number has probability k/n of being picked up */
    if (k > alea(n-1)) {
      /* pick this one up */
      k--;
      n--;
      a[k]= n;
    } else {
      /* don't pick this one */
      n--;
    }
  }
  if (n == k) {
    /* we've got k numbers to sample from a set of k, easy... */
    for(n=n-1; n>=0; n--) {
      a[n] = n;
    }
    k = 0;
  }
  if (k > 0) {
    assert(k <= n/2);
    ranksb(n, k, a);                /* reduced to ranksb */
  }
}

/* k near n, o(n) time, o(k) space */
inline void deal(int n, int k, std::vector<std::size_t> &a)
{
  assert(k <= n);
  assert(k <= (int)a.size());
  if (k <= n/2) {
    ranksb(n, k, a);
  } else {
    deal_k_near_n(n, k, a);
  }
}

} // namespace robust
} // namespace openMVG
#endif // OPENMVG_ROBUST_ESTIMATION_RAND_SAMPLING_H_
