/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
 *
 * Natron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Natron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 * ***** END LICENSE BLOCK ***** */
//

//Copyright (c) 2010-2011, Tim Day <timday@timday.com>
//
//Permission to use, copy, modify, and/or distribute this software for any
//purpose with or without fee is hereby granted, provided that the above
//copyright notice and this permission notice appear in all copies.
//
//THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
//WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
//MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
//ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
//WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
//ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
//OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

#ifndef NATRON_ENGINE_LRUCACHE_H
#define NATRON_ENGINE_LRUCACHE_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <map>
#include <list>
#include <utility>
#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
CLANG_DIAG_OFF(unknown-pragmas)
CLANG_DIAG_OFF(redeclared-class-member)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
#include <boost/bimap/list_of.hpp>
#include <boost/bimap/set_of.hpp>
#include <boost/bimap/unordered_set_of.hpp>
#include <boost/bimap.hpp>
CLANG_DIAG_ON(redeclared-class-member)
CLANG_DIAG_ON(unknown-pragmas)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
#endif

#include "Engine/EngineFwd.h"


//#define USE_VARIADIC_TEMPLATES
#define NATRON_CACHE_USE_HASH
#define NATRON_CACHE_USE_BOOST


/**@brief 4 types of LRU caches are defined here:
 *
 *- STL with hashing : std::unordered_map
 *- STL with comparison: std::map
 *- BOOST with hashing: boost::bimap with boost::unordered_set_of
 *- BOOST with comparison : boost::bimap with boost::set_of
 *
 * Using the appropriate #define , the software can be tuned to use a specific
 * underlying container version for all caches.
 *
 * USE_VARIADIC_TEMPLATES : define this if c++11 features like var args are
 * supported. It will make use of variadic templates to greatly
 * reduce the line of codes necessary, and it will also make it possible
 * to use STL with hashing (std::unordered_map), as it is defined in c++11
 *
 * NATRON_CACHE_USE_BOOST : define this to tell the compiler to use boost internally
 * for caches. Otherwise it will fallback on a STL version
 *
 * NATRON_CACHE_USE_HASH : define this to tell the compiler to use hashing
 *(std::unordered_map or boost::unordered_set_of) instead of a
 * tree-based version (std::map or boost::set_of).
 *
 * WARNING:  definining NATRON_CACHE_USE_HASH and not defining
 * NATRON_CACHE_USE_BOOST will require USE_VARIADIC_TEMPLATES to be
 * defined otherwise it will not compile. (no std::unordered_map
 * support on c++98)
 *
 **/

#ifdef USE_VARIADIC_TEMPLATES // c++11 is defined as well as unordered_map

#  ifndef NATRON_CACHE_USE_BOOST

#include <unordered_map>

// Class providing fixed-size (by number of records)
// LRU-replacement cache of a function with signature
// V f(K).
// MAP should be one of std::map or std::unordered_map.
// Variadic template args used to deal with the
// different type argument signatures of those
// containers; the default comparator/hash/allocator
// will be used.

///WARNING: Cached element must have a use_count() method that returns
///the current reference counting of the object. Typically a shared_ptr.


template <typename K,typename V,template<typename ...> class MAP>
class StlLRUHashTable
{
public:
    typedef K key_type;
    typedef std::list<V> value_type;
    // Key access history, most recent at back
    typedef std::list<key_type> key_tracker_type;
    // Key to value and key history iterator
    typedef MAP<key_type,std::pair<value_type,typename key_tracker_type::iterator> > key_to_value_type;

    // Constuctor specifies the cached function and
    // the maximum number of records to be stored
    StlLRUHashTable()
    {
    }

    // Obtain value of the cached function for k
    typename key_to_value_type::iterator operator()(const key_type & k)
    {
        // Attempt to find existing record
        typename key_to_value_type::iterator it = _key_to_value.find(k);
        if ( it != _key_to_value.end() ) {
            // We do have it:
            // Update access record by moving
            // accessed key to back of list
            _key_tracker.splice(_key_tracker.end(),_key_tracker,(*it).second.second);
        }

        return it;
    }

    void erase(typename key_to_value_type::iterator it)
    {
        _key_tracker.erase(it->second.second);
        _key_to_value.erase(it);
    }

    typename key_to_value_type::iterator end()
    {
        return _key_to_value.end();
    }

    typename key_to_value_type::iterator begin()
    {
        return _key_to_value.begin();
    }
    
    void insert(const key_type & k,
                const value_type& list)
    {
        typename key_tracker_type::iterator it = _key_tracker.insert(_key_tracker.end(),k);
        _key_to_value.insert( std::make_pair( k,std::make_pair(list,it) ) );
    }

    // Record a fresh key-value pair in the cache
    void insert(const key_type & k,
                const value_type & v)
    {
        typename key_to_value_type::iterator found =  _key_to_value.find(k);
        if ( found != _key_to_value.end() ) {
            found->second.first.push_back(v);
        } else {
            value_type list;
            list.push_back(v);
            // Create the key-value entry,
            // linked to the usage record.
            typename key_tracker_type::iterator it = _key_tracker.insert(_key_tracker.end(),k);
            _key_to_value.insert( std::make_pair( k,std::make_pair(list,it) ) );
        }
    }

    void clear()
    {
        _key_to_value.clear();
        _key_tracker.clear();
    }

    // Purge the least-recently-used element in the cache
    std::pair<key_type,V> evict()
    {
        // Assert method is never called when cache is empty
        assert( !_key_tracker.empty() );
        // Identify least recently used key
        const typename key_to_value_type::iterator it  = _key_to_value.find( _key_tracker.front() );
        for (typename std::list<V>::iterator it2 = it->second.first.begin();
             it2 != it->second.first.end();
             ++it2) {
            if ( (*it2).use_count() == 1 ) {
                std::pair<key_type,V> ret = std::make_pair(it->first,*it2);
                if (it->second.first.size() == 1) {
                    // Erase both elements to completely purge record
                    _key_to_value.erase(it);
                    _key_tracker.pop_front();
                } else {
                    it->second.first.erase(it2);
                }

                return ret;
            }
        }

        return std::make_pair( key_type(),V() );
    }

    unsigned int size()
    {
        return _container.size();
    }

private:
    // Key access history
    key_tracker_type _key_tracker;

    // Key-to-value lookup
    key_to_value_type _key_to_value;
};

#  else // NATRON_CACHE_USE_BOOST
        // Class providing fixed-size (by number of records)
        // LRU-replacement cache of a function with signature
        // V f(K).
        // SET is expected to be one of boost::bimaps::set_of
        // or boost::bimaps::unordered_set_of
template <typename K,typename V,template <typename ...> class SET>
class BoostLRUHashTable
{
public:
    typedef K key_type;
    typedef V value_type;
    typedef boost::bimaps::bimap<SET<key_type>,boost::bimaps::list_of<value_type> > container_type;

    BoostLRUHashTable()
    {
    }

    // Obtain value of the cached function for k
    typename container_type::left_iterator operator()(const key_type & k)
    {
        // Attempt to find existing record
        typename container_type::left_iterator it = _container.left.find(k);
        if ( it != _container.left.end() ) {
            // We do have it:
            // Update the access record view.
            _container.right.relocate( _container.right.end(),_container.project_right(it) );
        }

        return it;
    }

    void erase(typename container_type::left_iterator it)
    {
        _container.left.erase(it);
    }

    // return end of the iterator
    typename container_type::left_iterator end()
    {
        return _container.left.end();
    }

    // return begin of the iterator
    typename container_type::left_iterator begin()
    {
        return _container.left.begin();
    }

    void insert(const key_type & k,
                const value_type& list)
    {
        _container.insert( typename container_type::value_type(k,list) );
    }
    
    void insert(const key_type & k,
                const V & v)
    {
        // Create a new record from the key and the value
        // bimap's list_view defaults to inserting this at
        // the list tail (considered most-recently-used).
        typename container_type::left_iterator found = this->operator ()(k);
        if ( found != _container.left.end() ) {
            found->second.push_back(v);
        } else {
            value_type list;
            list.push_back(v);
            _container.insert( typename container_type::value_type(k,list) );
        }
    }

    void clear()
    {
        _container.clear();
    }

    std::pair<key_type,V> evict()
    {
        typename container_type::right_iterator it = _container.right.begin();
        while ( it != _container.right.end() ) {
            for (typename std::list<V>::iterator it2 = it->first.begin();
                 it2 != it->first.end();
                 ++it2) {
                if ( (*it2).use_count() == 1 ) {
                    std::pair<key_type,V> ret = std::make_pair(it->second,*it2);
                    if (it->first.size() == 1) {
                        _container.right.erase(it);
                    } else {
                        it->first.erase(it2);
                    }

                    return ret;
                }
            }
            ++it;
        }

        return std::make_pair( key_type(),V() );
    }

    unsigned int size()
    {
        return _container.size();
    }

private:
    container_type _container;
};

#  endif // NATRON_CACHE_USE_BOOST

#else // !USE_VARIADIC_TEMPLATES

// c++98 does not support stl unordered_map + variadic templates

#  ifndef NATRON_CACHE_USE_BOOST
template <typename K,typename V>
class StlLRUHashTable
{
public:
    typedef K key_type;
    typedef V value_type;
    // Key access history, most recent at back
    typedef std::list<key_type> key_tracker_type;
    // Key to value and key history iterator
    typedef std::map<key_type,std::pair<value_type,typename key_tracker_type::iterator> > key_to_value_type;

    // Constuctor specifies the cached function and
    // the maximum number of records to be stored
    StlLRUHashTable()
    {
    }

    // Obtain value of the cached function for k
    typename key_to_value_type::iterator operator()(const key_type & k)
    {
        // Attempt to find existing record
        typename key_to_value_type::iterator it = _key_to_value.find(k);
        if ( it != _key_to_value.end() ) {
            // We do have it:
            // Update access record by moving
            // accessed key to back of list
            _key_tracker.splice(_key_tracker.end(),_key_tracker,(*it).second.second);
        }

        return it;
    }

    void erase(typename key_to_value_type::iterator it)
    {
        _key_tracker.erase(it->second.second);
        _key_to_value.erase(it);
    }

    typename key_to_value_type::iterator end()
    {
        return _key_to_value.end();
    }

    typename key_to_value_type::iterator begin()
    {
        return _key_to_value.begin();
    }
    
    void insert(const key_type & k,
                const value_type& list)
    {
        typename key_tracker_type::iterator it = _key_tracker.insert(_key_tracker.end(),k);
        _key_to_value.insert( std::make_pair( k,std::make_pair(list,it) ) );
    }

    // Record a fresh key-value pair in the cache
    void insert(const key_type & k,
                const value_type & v)
    {
        typename key_to_value_type::iterator found =  _key_to_value.find(k);
        if ( found != _key_to_value.end() ) {
            found->second.first.push_back(v);
        } else {
            value_type list;
            list.push_back(v);
            // Create the key-value entry,
            // linked to the usage record.
            typename key_tracker_type::iterator it = _key_tracker.insert(_key_tracker.end(),k);
            _key_to_value.insert( std::make_pair( k,std::make_pair(list,it) ) );
        }
    }

    void clear()
    {
        _key_to_value.clear();
        _key_tracker.clear();
    }

    // Purge the least-recently-used element in the cache
    std::pair<key_type,V> evict()
    {
        // Assert method is never called when cache is empty
        assert( !_key_tracker.empty() );
        // Identify least recently used key
        const typename key_to_value_type::iterator it  = _key_to_value.find( _key_tracker.front() );
        for (typename std::list<V>::iterator it2 = it->second.first.begin();
             it2 != it->second.first.end();
             ++it2) {
            if ( (*it2).use_count() == 1 ) {
                std::pair<key_type,V> ret = std::make_pair(it->first,*it2);
                if (it->second.first.size() == 1) {
                    // Erase both elements to completely purge record
                    _key_to_value.erase(it);
                    _key_tracker.pop_front();
                } else {
                    it->second.first.erase(it2);
                }

                return ret;
            }
        }

        return std::make_pair( key_type(),V() );
    }

    unsigned int size()
    {
        return _key_to_value.size();
    }

private:
    // Key access history
    key_tracker_type _key_tracker;

    // Key-to-value lookup
    key_to_value_type _key_to_value;
};

#  else // NATRON_CACHE_USE_BOOST

#    ifdef NATRON_CACHE_USE_HASH
template <typename K,typename V>
class BoostLRUHashTable
{
public:
    typedef K key_type;
    typedef std::list<V> value_type;
    typedef boost::bimaps::bimap<boost::bimaps::unordered_set_of<key_type>,boost::bimaps::list_of<value_type> > container_type;

    BoostLRUHashTable()
    {
    }

    typename container_type::left_iterator operator()(const key_type & k)
    {
        // Attempt to find existing record
        typename container_type::left_iterator it = _container.left.find(k);
        if ( it != _container.left.end() ) {
            // We do have it:
            // Update the access record view.
            _container.right.relocate( _container.right.end(),_container.project_right(it) );
        }

        return it;
    }

    void erase(typename container_type::left_iterator it)
    {
        _container.left.erase(it);
    }

    // return end of the iterator
    typename container_type::left_iterator end()
    {
        return _container.left.end();
    }

    // return begin of the iterator
    typename container_type::left_iterator begin()
    {
        return _container.left.begin();
    }

    void insert(const key_type & k,
                const value_type& list)
    {
        _container.insert(typename container_type::value_type(k,list));
    }
    
    void insert(const key_type & k,
                const V & v)
    {
        // Create a new record from the key and the value
        // bimap's list_view defaults to inserting this at
        // the list tail (considered most-recently-used).
        typename container_type::left_iterator found = this->operator ()(k);
        if ( found != _container.left.end() ) {
            found->second.push_back(v);
        } else {
            value_type list;
            list.push_back(v);
            _container.insert( typename container_type::value_type(k,list) );
        }
    }

    void clear()
    {
        _container.clear();
    }

    std::pair<key_type,V> evict()
    {
        typename container_type::right_iterator it = _container.right.begin();
        while ( it != _container.right.end() ) {
            for (typename std::list<V>::iterator it2 = it->first.begin(); it2 != it->first.end(); ++it2) {
                if (it2->use_count() == 1) {
                    std::pair<key_type,V> ret = std::make_pair(it->second,*it2);
                    if (it->first.size() == 1) {
                        _container.right.erase(it);
                    } else {
                        it->first.erase(it2);
                    }

                    return ret;
                }
            }
            ++it;
        }

        return std::make_pair( key_type(),V() );
    }

    unsigned int size()
    {
        return _container.size();
    }

private:
    container_type _container;
};

#    else // !NATRON_CACHE_USE_HASH

template <typename K,typename V>
class BoostLRUHashTable
{
public:
    typedef K key_type;
    typedef V value_type;
    typedef boost::bimaps::bimap<boost::bimaps::set_of<key_type>,boost::bimaps::list_of<value_type> > container_type;

    BoostLRUHashTable()
    {
    }

    typename container_type::left_iterator operator()(const key_type & k)
    {
        // Attempt to find existing record
        typename container_type::left_iterator it = _container.left.find(k);
        if ( it != _container.left.end() ) {
            // We do have it:
            // Update the access record view.
            _container.right.relocate( _container.right.end(),_container.project_right(it) );
        }

        return it;
    }

    void erase(typename container_type::left_iterator it)
    {
        _container.left.erase(it);
    }

    // return end of the iterator
    typename container_type::left_iterator end()
    {
        return _container.left.end();
    }

    // return begin of the iterator
    typename container_type::left_iterator begin()
    {
        return _container.left.begin();
    }
    
    void insert(const key_type & k,
                const value_type& list)
    {
        _container.insert(typename container_type::value_type(k,list));
    }
    

    void insert(const key_type & k,
                const V & v)
    {
        // Create a new record from the key and the value
        // bimap's list_view defaults to inserting this at
        // the list tail (considered most-recently-used).
        typename container_type::left_iterator found = this->operator ()(k);
        if ( found != _container.left.end() ) {
            found->second.push_back(v);
        } else {
            value_type list;
            list.push_back(v);
            _container.insert( typename container_type::value_type(k,list) );
        }
    }

    void clear()
    {
        _container.clear();
    }

    std::pair<key_type,V> evict()
    {
        typename container_type::right_iterator it = _container.right.begin();
        while ( it != _container.right.end() ) {
            for (typename std::list<V>::iterator it2 = it->first.begin(); it2 != it->first.end(); ++it2) {
                if ( (*it2).use_count() == 1 ) {
                    std::pair<key_type,V> ret = std::make_pair(it->second,*it2);
                    if (it->first.size() == 1) {
                        _container.right.erase(it);
                    } else {
                        it->first.erase(it2);
                    }

                    return ret;
                }
            }
            ++it;
        }

        return std::make_pair( key_type(),V() );
    }

    unsigned int size()
    {
        return _container.size();
    }

private:
    container_type _container;
};

#    endif // !NATRON_CACHE_USE_HASH
#  endif // NATRON_CACHE_USE_BOOST

#endif // !USE_VARIADIC_TEMPLATES

#endif // ifndef NATRON_ENGINE_LRUCACHE_H
