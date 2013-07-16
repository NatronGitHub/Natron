//  Powiter
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012. 
*contact: immarespond at gmail dot com
*
*/

 

 



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

#ifndef PowiterOsX_LRUcache_h
#define PowiterOsX_LRUcache_h

#include <QtCore/QReadWriteLock>
#include <QtCore/QReadLocker>
#include <QtCore/QWriteLocker>
#include <boost/bimap.hpp>
#include <boost/bimap/list_of.hpp>
#include <boost/bimap/set_of.hpp>
#include <boost/bimap/unordered_set_of.hpp>
#include "Superviser/powiterFn.h"
#include <map>
#include <list>
#include <iostream>


/*4 types of LRU caches are defined here:
 
 - STL with hashing : std::unordered_map
 - STL with comparison: std::map
 - BOOST with hashing: boost::bimap with boost::unordered_set_of
 - BOOST with comparison : boost::bimap with boost::set_of
 
 Using the appropriate #define , the software can be tuned to use a specific 
 underlying container version for all caches.
 
 USE_VARIADIC_TEMPLATES : define this if c++11 features like var args are
 supported. It will make use of variadic templates to greatly 
 reduce the line of codes necessary, and it will also make it possible
 to use STL with hashing (std::unordered_map), as it is defined in c++11
 
 CACHE_USE_BOOST : define this to tell the compiler to use boost internally
 for caches. Otherwise it will fallback on a STL version
 
 CACHE_USE_HASH : define this to tell the compiler to use hashing
 (std::unordered_map or boost::unordered_set_of) instead of a
 tree-based version (std::map or boost::set_of).
 
 WARNING:  definining CACHE_USE_HASH and not defining
 CACHE_USE_BOOST will require USE_VARIADIC_TEMPLATES to be
 defined otherwise it will not compile. (no std::unordered_map
 support on c++98)
 
 */

#ifdef USE_VARIADIC_TEMPLATES // c++11 is defined as well as unordered_map

#ifndef CACHE_USE_BOOST

#include <unordered_map>

// Class providing fixed-size (by number of records)
// LRU-replacement cache of a function with signature
// V f(K).
// MAP should be one of std::map or std::unordered_map.
// Variadic template args used to deal with the
// different type argument signatures of those
// containers; the default comparator/hash/allocator
// will be used.


template <typename K,typename V,template<typename...> class MAP>
class StlLRUCache
{
public:
    typedef K key_type;
    typedef V value_type;
    // Key access history, most recent at back
    typedef std::list<key_type> key_tracker_type;
    // Key to value and key history iterator
    typedef MAP<key_type,std::pair<value_type,typename key_tracker_type::iterator> > key_to_value_type;
    
    // Constuctor specifies the cached function and
    // the maximum number of records to be stored
    StlLRUCache(){}
    
    // Obtain value of the cached function for k
    typename key_to_value_type::iterator operator()(const key_type& k)  {
        typename key_to_value_type::iterator it;
        {
            QReadLocker g(&_rwLock);
            // Attempt to find existing record
            it =_key_to_value.find(k);
        }
        if (it!=_key_to_value.end()) {
            // We do have it:
            // Update access record by moving
            // accessed key to back of list
            {
                QWriteLocker g(&_rwLock);
                _key_tracker.splice(_key_tracker.end(),_key_tracker,(*it).second.second);
            }
        }
        return it;
    }
    typename key_to_value_type::iterator end(){return _key_to_value.end();}
    typename key_to_value_type::iterator begin(){return _key_to_value.begin();}
    // Record a fresh key-value pair in the cache
    // Return value is the value evicted from cache space was necessary.
    std::pair<key_type,value_type> insert(const key_type& k,const value_type& v,bool mustEvict) {
        std::pair<key_type,value_type> ret ;
        // Make space if necessary
        if (mustEvict)
            ret =  evict();
        
        QWriteLocker g(&_rwLock);
        // Record k as most-recently-used key
        typename key_tracker_type::iterator it =_key_tracker.insert(_key_tracker.end(),k);
        
        // Create the key-value entry,
        // linked to the usage record.
        _key_to_value.insert(std::make_pair(k,std::make_pair(v,it)));
        if(mustEvict){
            return ret;
        }else{
            return std::make_pair(0,(value_type)NULL);
        }
    }
    void erase(typename key_tracker_type::iterator it){
        _key_tracker.erase(it);
    }
    
    void clear(){
        _key_to_value.clear();
        _key_tracker.clear();
    }
    
    // Purge the least-recently-used element in the cache
    std::pair<key_type,value_type> evict() {
        // Assert method is never called when cache is empty
        assert(!_key_tracker.empty());
        const typename key_to_value_type::iterator it;
        {
            QReadLocker g(&_rwLock);
            // Identify least recently used key
            it  =_key_to_value.find(_key_tracker.front());
            assert(it!=_key_to_value.end());
        }
        
        std::pair<key_type,value_type> ret = std::make_pair(it->first,it->second.first);
        {
            // Erase both elements to completely purge record
            QWriteLocker g(&_rwLock);
            _key_to_value.erase(it);
            _key_tracker.pop_front();
        }
        return ret;
    }
    U32 size(){return _container.size();}

    //public member so your cache implem can use it
    QReadWriteLock _rwLock;

    
private:
        // Key access history
    key_tracker_type _key_tracker;
    
    // Key-to-value lookup
    key_to_value_type _key_to_value;
};

#else // ! CACHE_USE_BOOST
// Class providing fixed-size (by number of records)
// LRU-replacement cache of a function with signature
// V f(K).
// SET is expected to be one of boost::bimaps::set_of
// or boost::bimaps::unordered_set_of
template <typename K,typename V,template <typename...> class SET>
class BoostLRUCacheContainer
{
public:
    typedef K key_type;
    typedef V value_type;
    typedef boost::bimaps::bimap<SET<key_type>,boost::bimaps::list_of<value_type> > container_type;
    
    BoostLRUCacheContainer(){}
    
    // Obtain value of the cached function for k
    typename container_type::left_iterator operator()(const key_type& k)  {
        typename container_type::left_iterator it;
        {
            QReadLocker g(&_rwLock);
            // Attempt to find existing record
            it=_container.left.find(k);
        }
        if (it!=_container.left.end()) {
            // We do have it:
            // Update the access record view.
            QWriteLocker g(&_rwLock);
            _container.right.relocate(_container.right.end(),_container.project_right(it));
        }
        return it;
    }
    // return end of the iterator
    typename container_type::left_iterator end() {
        return _container.left.end();
    }
    
    // return begin of the iterator
    typename container_type::left_iterator begin() {
        return _container.left.begin();
    }
    // Return value is the value evicted from cache space was necessary.
    std::pair<key_type,value_type> insert(const key_type& k,const value_type& v,bool mustEvict) {
        std::pair<key_type,value_type> ret;
        if(mustEvict){
            ret = evict();
        }
        // Create a new record from the key and the value
        // bimap's list_view defaults to inserting this at
        // the list tail (considered most-recently-used).
        QWriteLocker g(&_rwLock);
        _container.insert(typename container_type::value_type(k,v));
        
        if(mustEvict){
            return ret;
        }else{
            return std::make_pair(0,(value_type)NULL);
        }
    }
    void erase(typename container_type::left_iterator it){
        typename container_type::right_iterator itRight = _container.project_right(it);
        _container.right.erase(itRight);
    }
    
    
    void clear(){
        _container.clear();
    }
    
    std::pair<key_type,value_type> evict(){
        QWriteLocker g(&_rwLock);
        typename container_type::right_iterator it = _container.right.begin();
        std::pair<key_type,value_type> ret = std::make_pair(it->second,it->first);
        _container.right.erase(it);
        return ret;
        
    }

    U32 size(){return _container.size();}
    
    //public member so your cache implem can use it
    QReadWriteLock _rwLock;
private:
        container_type _container;
};
#endif // CACHE_USE_BOOST

#else // c++98 no support for stl unordered_map + no variadic templates

#ifndef CACHE_USE_BOOST
template <typename K,typename V>
class StlLRUTreeCache{
public:
    typedef K key_type;
    typedef V value_type;
    // Key access history, most recent at back
    typedef std::list<key_type> key_tracker_type;
    // Key to value and key history iterator
    typedef std::map<key_type,std::pair<value_type,typename key_tracker_type::iterator> > key_to_value_type;
    
    // Constuctor specifies the cached function and
    // the maximum number of records to be stored
    StlLRUTreeCache(){}
    
    // Obtain value of the cached function for k
    typename key_to_value_type::iterator operator()(const key_type& k)  {
        typename key_to_value_type::iterator it;
        {
            QReadLocker g(&_rwLock);
            // Attempt to find existing record
            it =_key_to_value.find(k);
        }
        if (it!=_key_to_value.end()) {
            // We do have it:
            // Update access record by moving
            // accessed key to back of list
            {
                QWriteLocker g(&_rwLock);
                _key_tracker.splice(_key_tracker.end(),_key_tracker,(*it).second.second);
            }
        }
        return it;
    }    typename key_to_value_type::iterator end(){return _key_to_value.end();}
    typename key_to_value_type::iterator begin(){return _key_to_value.begin();}
    // Record a fresh key-value pair in the cache
    // Return value is the value evicted from cache space was necessary.
    std::pair<key_type,value_type> insert(const key_type& k,const value_type& v,bool mustEvict) {
        std::pair<key_type,value_type> ret ;
        // Make space if necessary
        if (mustEvict)
            ret =  evict();
        
        QWriteLocker g(&_rwLock);
        // Record k as most-recently-used key
        typename key_tracker_type::iterator it =_key_tracker.insert(_key_tracker.end(),k);
        
        // Create the key-value entry,
        // linked to the usage record.
        _key_to_value.insert(std::make_pair(k,std::make_pair(v,it)));
        if(mustEvict){
            return ret;
        }else{
            return std::make_pair(0,(value_type)NULL);
        }
    }
    void erase(typename key_tracker_type::iterator it){
        _key_tracker.erase(it);
    }
    void clear(){
        _key_to_value.clear();
        _key_tracker.clear();
    }
    
    // Purge the least-recently-used element in the cache
    std::pair<key_type,value_type> evict() {
        // Assert method is never called when cache is empty
        assert(!_key_tracker.empty());
        typename key_to_value_type::iterator it;
        {
            QReadLocker g(&_rwLock);
            // Identify least recently used key
            it  =_key_to_value.find(_key_tracker.front());
            assert(it!=_key_to_value.end());
        }
        
        std::pair<key_type,value_type> ret = std::make_pair(it->first,it->second.first);
        {
            // Erase both elements to completely purge record
            QWriteLocker g(&_rwLock);
            _key_to_value.erase(it);
            _key_tracker.pop_front();
        }
        return ret;
    }
    U32 size(){return _key_to_value.size();}
    
    //public member so your cache implem can use it
    QReadWriteLock _rwLock;
    
private:
       // Key access history
    key_tracker_type _key_tracker;
    
    // Key-to-value lookup
    key_to_value_type _key_to_value;
    
};
#else // boost

#ifdef CACHE_USE_HASH
template <typename K,typename V>
class BoostLRUHashCache{
public:
    typedef K key_type;
    typedef V value_type;
    typedef boost::bimaps::bimap<boost::bimaps::unordered_set_of<key_type>,boost::bimaps::list_of<value_type> > container_type;
    
    BoostLRUHashCache(){}
    
    // Obtain value of the cached function for k
    typename container_type::left_iterator operator()(const key_type& k)  {
        typename container_type::left_iterator it;
        {
            QReadLocker g(&_rwLock);
            // Attempt to find existing record
            it=_container.left.find(k);
        }
        if (it!=_container.left.end()) {
            // We do have it:
            // Update the access record view.
            QWriteLocker g(&_rwLock);
            _container.right.relocate(_container.right.end(),_container.project_right(it));
        }
        return it;
    }
    // return end of the iterator
    typename container_type::left_iterator end() {
        return _container.left.end();
    }
    
    // return begin of the iterator
    typename container_type::left_iterator begin() {
        return _container.left.begin();
    }
    // Return value is the value evicted from cache space was necessary.
    std::pair<key_type,value_type> insert(const key_type& k,const value_type& v,bool mustEvict) {
        std::pair<key_type,value_type> ret;
        if(mustEvict){
            ret = evict();
        }
        // Create a new record from the key and the value
        // bimap's list_view defaults to inserting this at
        // the list tail (considered most-recently-used).
        QWriteLocker g(&_rwLock);
        _container.insert(typename container_type::value_type(k,v));
        
        if(mustEvict){
            return ret;
        }else{
            return std::make_pair(0,(value_type)NULL);
        }
    }
    void erase(typename container_type::left_iterator it){
        typename container_type::right_iterator itRight = _container.project_right(it);
        _container.right.erase(itRight);
    }
    
    void clear(){
        _container.clear();
    }
    
    std::pair<key_type,value_type> evict(){
        QWriteLocker g(&_rwLock);
        typename container_type::right_iterator it = _container.right.begin();
        std::pair<key_type,value_type> ret = std::make_pair(it->second,it->first);
        _container.right.erase(it);
        //_container.left.erase
        return ret;
        
    }
    
    U32 size(){return _container.size();}
    
    
    //public member so your cache implem can use it
    QReadWriteLock _rwLock;
private:
        container_type _container;
};
#else // CACHE_USE_HASH
template <typename K,typename V>
class BoostLRUTreeCache{
public:
    typedef K key_type;
    typedef V value_type;
    typedef boost::bimaps::bimap<boost::bimaps::set_of<key_type>,boost::bimaps::list_of<value_type> > container_type;
    
    BoostLRUTreeCache(){}
    
    // Obtain value of the cached function for k
    typename container_type::left_iterator operator()(const key_type& k)  {
        typename container_type::left_iterator it;
        {
            QReadLocker g(&_rwLock);
            // Attempt to find existing record
            it=_container.left.find(k);
        }
        if (it!=_container.left.end()) {
            // We do have it:
            // Update the access record view.
            QWriteLocker g(&_rwLock);
            _container.right.relocate(_container.right.end(),_container.project_right(it));
        }
        return it;
    }
    // return end of the iterator
    typename container_type::left_iterator end() {
        return _container.left.end();
    }
    
    // return begin of the iterator
    typename container_type::left_iterator begin() {
        return _container.left.begin();
    }
    // Return value is the value evicted from cache space was necessary.
    std::pair<key_type,value_type> insert(const key_type& k,const value_type& v,bool mustEvict) {
        std::pair<key_type,value_type> ret;
        if(mustEvict){
            ret = evict();
        }
        // Create a new record from the key and the value
        // bimap's list_view defaults to inserting this at
        // the list tail (considered most-recently-used).
        QWriteLocker g(&_rwLock);
        _container.insert(typename container_type::value_type(k,v));
        
        if(mustEvict){
            return ret;
        }else{
            return std::make_pair(0,(value_type)NULL);
        }
    }
    void erase(typename container_type::left_iterator it){
        typename container_type::right_iterator itRight = _container.project_right(it);
        _container.right.erase(itRight);
    }

    
    void clear(){
        _container.clear();
    }
    
    std::pair<key_type,value_type> evict(){
        QWriteLocker g(&_rwLock);
        typename container_type::right_iterator it = _container.right.begin();
        std::pair<key_type,value_type> ret = std::make_pair(it->second,it->first);
        _container.right.erase(it);
        return ret;
        
    }
    
    U32 size(){return _container.size();}
    
    //public member so your cache implem can use it
    QReadWriteLock _rwLock;
private:
    
    container_type _container;
};
#endif // CACHE_USE_HASH
#endif // !CACHE_USE_BOOST

#endif // !USE_VARIADIC_TEMPLATES

#endif
