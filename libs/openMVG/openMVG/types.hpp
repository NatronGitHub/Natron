// Copyright (c) 2015 Pierre MOULON.

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENMVG_TYPES_H_
#define OPENMVG_TYPES_H_

#include <Eigen/Core>

#include <limits>
#include <map>
#include <set>
#include <vector>
#include <memory>

#if defined OPENMVG_STD_UNORDERED_MAP
#include <unordered_map>
#endif

#if defined(OPENMVG_BOOST_UNORDERED_MAP)
#include <boost/unordered_map.hpp>
#endif

#if defined(OPENMVG_BOOST_SHARED_PTR)
#include <boost/shared_ptr.hpp>
#endif

#if defined(OPENMVG_BOOST_UNIQUE_PTR)
#include <boost/move/unique_ptr.hpp>
#endif

namespace openMVG{

typedef unsigned long IndexT;
static const IndexT UndefinedIndexT = std::numeric_limits<IndexT>::max();

typedef std::pair<IndexT,IndexT> Pair;
typedef std::set<Pair> Pair_Set;
typedef std::vector<Pair> Pair_Vec;

#define OPENMVG_NO_UNORDERED_MAP 1

#if defined OPENMVG_NO_UNORDERED_MAP
template<typename K, typename V>
struct Hash_Map : std::map<K, V, std::less<K>,
 Eigen::aligned_allocator<std::pair<K,V> > > {};
#elif defined OPENMVG_STD_UNORDERED_MAP
template<typename Key, typename Value>
struct Hash_Map : std::unordered_map<Key, Value> {};
#elif defined OPENMVG_BOOST_UNORDERED_MAP
template<typename Key, typename Value>
struct Hash_Map : boost::unordered_map<Key, Value> {};
#endif

#if defined(OPENMVG_TR1_SHARED_PTR)
using std::tr1::shared_ptr;
#elif defined(OPENMVG_BOOST_SHARED_PTR)
using boost::shared_ptr;
#else
using std::shared_ptr;
#endif

#if defined(OPENMVG_BOOST_UNIQUE_PTR)
using boost::movelib::unique_ptr;
#else
using std::unique_ptr;
#endif

} // namespace openMVG

#endif  // OPENMVG_TYPES_H_
