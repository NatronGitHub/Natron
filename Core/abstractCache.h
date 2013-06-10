//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com
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

#ifndef __PowiterOsX__abstractCache__
#define __PowiterOsX__abstractCache__

#include <QtCore/QReadWriteLock>
#include <QtCore/QReadLocker>
#include <QtCore/QWriteLocker>
#include <unordered_map>
#include <list>
#include <iostream>
#include "Core/singleton.h"
#include <boost/bimap.hpp>
#include <boost/bimap/list_of.hpp>
#include <boost/bimap/unordered_set_of.hpp>
#include "Superviser/powiterFn.h"


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
    StlLRUCache(
                value_type (*f)(const key_type&),
                size_t c
                )
    {
    }
    
    // Obtain value of the cached function for k
    typename key_to_value_type::iterator operator()(const key_type& k)  {
        
        // Attempt to find existing record
        typename key_to_value_type::iterator it
        =_key_to_value.find(k);
        
        if (it!=_key_to_value.end()) {
            
            // We do have it:
            
            // Update access record by moving
            // accessed key to back of list
            _key_tracker.splice(
                                _key_tracker.end(),
                                _key_tracker,
                                (*it).second.second
                                );
            
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
        
        // Record k as most-recently-used key
        typename key_tracker_type::iterator it
        =_key_tracker.insert(_key_tracker.end(),k);
        
        // Create the key-value entry,
        // linked to the usage record.
        _key_to_value.insert(std::make_pair(k,std::make_pair(v,it)));
        
        
        if(mustEvict){
            return ret;
        }else{
            return std::make_pair(-1,(value_type)NULL);
        }
    }
    
    void clear(){
        _key_to_value.clear();
        _key_tracker.clear();
    }
    
private:
    
    
    // Purge the least-recently-used element in the cache
    std::pair<key_type,value_type> evict() {
        
        // Assert method is never called when cache is empty
        assert(!_key_tracker.empty());
        
        // Identify least recently used key
        const typename key_to_value_type::iterator it
        =_key_to_value.find(_key_tracker.front());
        assert(it!=_key_to_value.end());
        
        std::pair<key_type,value_type> ret = std::make_pair(it->first,it->second.first);
        // Erase both elements to completely purge record
        _key_to_value.erase(it);
        _key_tracker.pop_front();
        return ret;
    }
    
    // Key access history
    key_tracker_type _key_tracker;
    
    // Key-to-value lookup
    key_to_value_type _key_to_value;
    
};


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
    
    typedef boost::bimaps::bimap<
    SET<key_type>,
    boost::bimaps::list_of<value_type>
    > container_type;
    
    
    BoostLRUCacheContainer()
    {
    }
    
    // Obtain value of the cached function for k
    typename container_type::left_iterator operator()(const key_type& k)  {
        
        // Attempt to find existing record
        typename container_type::left_iterator it =_container.left.find(k);
        
        if (it!=_container.left.end()) {
            
            // We do have it:
            
            // Update the access record view.
            _container.right.relocate(
                                      _container.right.end(),
                                      _container.project_right(it)
                                      );
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
        _container.insert(typename container_type::value_type(k,v));
        
        if(mustEvict){
            return ret;
        }else{
            return std::make_pair(-1,(value_type)NULL);
        }
        
        
        
    }
    void clear(){
        _container.clear();
    }
    
    private:
        
        
        std::pair<key_type,value_type> evict(){
            typename container_type::right_iterator it = _container.right.begin();
            std::pair<key_type,value_type> ret = std::make_pair(it->second,it->first);
            _container.right.erase(it);
            return ret;
            
        }
        
        container_type _container;
        
    };
    
    class MemoryFile;
    
    /*Base class for cache entries. This can be overloaded to fit parameters you'd
     like to track like offset in files, list of elements etc...
     Entries are not copyable, you must create a new object if you want to copy infos.*/
    class CacheEntry {
        
        U64 _size; //the size in bytes of the entry
        
    public:
        CacheEntry():_size(0){}
        virtual ~CacheEntry(){}
        
        const U64 size() const {return _size;}
        
    protected:
        
        virtual void allocate(U64 byteCount, const char* path= 0)=0;
        virtual void deallocate()=0;
        
        
        
        
    };
    
    /*A memory-mapped cache entry
     */
    class MemoryMappedEntry: public CacheEntry{
        
    protected:
        MemoryFile* _mappedFile;
    public:
        MemoryMappedEntry();
        
        const MemoryFile* getMappedFile() const {return _mappedFile;}
        
        /*Must print a string representing all the data, terminated by a
         new line character.*/
        virtual std::string printOut()=0;
        
        virtual void allocate(U64 byteCount, const char* path =0);
        virtual void deallocate();
        virtual ~MemoryMappedEntry();
    };
    
    class InMemoryEntry : public CacheEntry{
    protected:
        char* _data;
    public:
        InMemoryEntry();
        virtual void allocate(U64 byteCount,const char* path = 0);
        
        /*deriving class must implement this operator*/
        virtual bool operator==(const CacheEntry& other)=0;
        
        virtual void deallocate();
        virtual ~InMemoryEntry();
    };
    
    /*Aims to provide an abstract base class for caches. 2 cache modes are availables:
     
     - Disk-caches : these are using MMAP and have an in-memory portion (the currently mapped-files)
     and an "on-disk" portion (backed with files). They have the advantage to store physically data
     on disk so they can survive several runs. These caches can handle a bigger amount of data than
     in-memory caches, but are a bit slower.
     
     - In-memory caches:  these are solely using malloc/free and reside only in-memory. Beware when
     using these caches that they do not consume all the memory as it would make the OS run very slow.
     
     Both these caches implements a LRU freeing rules. They are thread-safe.
     
     You can derive either AbstractMemoryCache or AbstractDiskCache to implement your own cache.
     You must code yourself the function returning a U64 hash
     key for the elements you want to cache. Be sure to have a key that aims to scatter data
     the most. Check nodecache.h to see an example of how to use it.
     The entry must derive InMemoryEntry for memory caches and MemoryMappedEntry for disk caches.
     Each cache entry for disk-caches will be under the sub-folder whose name is the 1st byte
     of the hash key.*/
    class AbstractCache {
        
    public:
        
#ifdef CACHE_USE_BOOST
        typedef BoostLRUCacheContainer<U64, CacheEntry* , boost::bimaps::unordered_set_of> CacheContainer;
        typedef CacheContainer::container_type::left_iterator CacheIterator;
        typedef CacheContainer::container_type::left_const_iterator ConstCacheIterator;
        static CacheEntry*  getValueFromIterator(CacheIterator it){return it->second;}
#else
        typedef StlLRUCache<U64,CacheEntry*, std::unordered_map> CacheContainer;
        typedef CacheContainer::key_to_value_type::iterator CacheIterator;
        typedef CacheContainer::key_to_value_type::const_iterator ConstCacheIterator;
        static boost::shared_ptr<CacheEntry>  getValueFromIterator(CacheIterator it){return it->second;}
#endif
        
    private:
        
        
        U64 _maximumCacheSize; // maximum size allowed for the cache
        CacheContainer _cache; // the actual cache
        
    protected:
        //protected so derived class can modify it
        U64 _size; // current size of the cache
    public:
        
        AbstractCache();
        ~AbstractCache();
        
        /*Adds a new entry to the cache. Returns true if it removed
         another entry before inserting the new one, false otherwise.*/
        virtual bool add(U64 key,CacheEntry* entry);
        
        
        /*clears the content of the cache (and deletes cache entries).
         This effectively resets the size of the cache.*/
        void clear();
        
        
        /*Returns the name  of the cache. It will
         serve as the root folder for this cache, and will
         be located under Cache/<cacheName()>*/
        virtual std::string cacheName()=0;
        
        
        /*Set the maximum cache size. It does not shrink the content of the cache,
         but prevent it to grow. You must clear it if you want to remove the data
         exceeding the size.*/
        void setMaximumCacheSize(U64 size){_maximumCacheSize = size;}
        
        const U64& getMaximumSize() const {return _maximumCacheSize;}
        
        const U64& getCurrentSize() const {return _size;}
        
        /*Returns an iterator to the cache. If found it points
         to a valid cache entry, otherwise it points to to end.
         */
        CacheIterator isCached(const U64& key) ;
        
        
        CacheIterator begin(){return _cache.begin();}
        
        /*So you can compare the result of isCached by the result of this
         function. Again, protected since it should be opaque for the end user.*/
        CacheIterator end() {return _cache.end();}
        
    protected:
        
        
        QReadWriteLock _lock;
        
    };
    
    /*AbstractMemoryCache represents an abstract cache living solely in-memory.
     These are solely using malloc/free and reside only in-memory. Beware when
     using these caches that they do not consume all the memory as it would make the OS run very slow.*/
    class AbstractMemoryCache : public AbstractCache{
    public:
        
        AbstractMemoryCache(){}
        virtual ~AbstractMemoryCache(){}
        
        /*type-check for the entry and call the super-class version
         of add.*/
        virtual bool add(U64 key,CacheEntry* entry);
        
        virtual std::string cacheName()=0;
        
        
    };
    
    /*AbstractDiskCache represents an abstract cache living both in-memory with entries backed on disk (with files).
     These are using MMAP and have an in-memory portion (the currently mapped-files)
     and an "on-disk" portion (backed with files). They have the advantage to store physically data
     on disk so they can survive several runs. These caches can handle a bigger amount of data than
     in-memory caches, but are a bit slower.*/
    class AbstractDiskCache : public AbstractCache {
        
        U64 _inMemorySize; // the size of the in-memory portion of the cache
        double _maximumInMemorySize; // the maximum size of the in-memory portion of the cache.(in % of the maximum cache size)
        
        /*Keeps track of the cache entries that reside in the virtual adress space of the process.
         These cache entries must be of type MemoryMappedEntry, typecheck is done by the class internally.*/
        CacheContainer _inMemoryPortion;
        
    public:
        /*Constructs an empty disk cache of size 0. You must explicitly call
         setMaximumCacheSize to allow the cache to grow. The inMemoryUsage indicates
         the percentage of the maximumCacheSize that you allow to be in memory.*/
        AbstractDiskCache(double inMemoryUsage);
        virtual ~AbstractDiskCache();
        
        /*Creates empty sub-directories for the cache, counting them
         from Ox00 to OxFF.
         Must be called explicitely after the constructor
         */
        void initializeSubDirectories();
        
        virtual std::string cacheName()=0;
        
        /*Returns the cache version as a string. This is
         used to know whether a cache is still valid when
         restoring*/
        virtual std::string cacheVersion()=0;
        
        /*Recover an entry from string*/
        virtual std::pair<U64,MemoryMappedEntry*> recoverEntryFromString(QString str)=0;
        
        /*type-check for the entry, add the entry to the in-memory cache and
         call the super-class version of add.*/
        virtual bool add(U64 key,CacheEntry* entry);
        
        void clearDiskCache(){clearInMemoryCache(); clear();}
        
        /*Removes all in-memory entries and put them in the on-disk portion.
         Some entries might get removed from the on-disk portion when calling this.*/
        void clearInMemoryCache();
        
        /*Set the maximum memory portion usage of the cache (in percentage of the
         maximum cache size).*/
        void setMaximumInMemorySize(double percentage){ _maximumInMemorySize = percentage;}
        
        const double& getMaximumInMemorySize() const {return _maximumInMemorySize;}
        
        const U64& getCurrentInMemoryPortionSize() const {return _inMemorySize;}
        
        /*Saves cache to disk as a settings file.
         */
        void save();
        
        /*Restores the cache from disk.*/
        void restore();

        
    protected:
        /*protected since it should be opaque for the end user.*/
        CacheIterator beginMemoryCache(){return _inMemoryPortion.begin();}
        
        /*protected since it should be opaque for the end user.*/
        CacheIterator endMemoryCache(){ return _inMemoryPortion.end();}
        
        /*Returns an iterator to the cache. If found it points
         to a valid cache entry, otherwise it points to to end.
         Protected so the derived class must explicitly encapsulate
         this function: the CacheIterator should be opaque to the end user.*/
        CacheIterator isInMemory(const U64& key);
        
                
    private:
        
        

    };
    
#endif /* defined(__PowiterOsX__abstractCache__) */
