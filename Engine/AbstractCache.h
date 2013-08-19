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

#ifndef __PowiterOsX__abstractCache__
#define __PowiterOsX__abstractCache__

#include "Core/LRUcache.h"
#include "Superviser/GlobalDefines.h"
#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <vector>

class MemoryFile;
/*Base class for cache entries. This can be overloaded to fit parameters you'd
 like to track like offset in files, list of elements etc...
 Entries are not copyable, you must create a new object if you want to copy infos.*/
class CacheEntry {
    int _refCount;
public:
    
    CacheEntry():_refCount(0),_size(0){}
    
    virtual ~CacheEntry(){}
    
    int refCount() const {return _refCount;}
    
    void addReference() {++_refCount; assert(_refCount >= 0);}
    
    void removeReference() {--_refCount; assert(_refCount >= 0);}
    
    /*Returns true if the cache can delete this entry*/
    bool isRemovable() const {return !_refCount;}
    
    /*Returns the size of the entry in bytes.*/
    U64 size() const {return _size;}
    
    virtual bool isMemoryMappedEntry () const =0;
    
protected:
    U64 _size; //the size in bytes of the entry

    
    /*Must be implemented to handle the allocation of the entry.
     Must return true on success,false otherwise.*/
    virtual bool allocate(U64 byteCount, const char* path= 0)=0;
    
    /*Must implement the deallocation of the entry*/
    virtual void deallocate()=0;
    
};

/*A memory-mapped cache entry
 */
class MemoryMappedEntry: public CacheEntry{
    
protected:
    std::string _path; // < the path of the file backing the entry
    MemoryFile* _mappedFile; // < the object holding mapped datas
public:
    
    MemoryMappedEntry();
    
    /*Returns a pointer to the memory mapped object*/
    const MemoryFile* getMappedFile() const {return _mappedFile;}
    
    /*Must print a string representing all the data, terminated by a
     new line character.*/
    virtual std::string printOut()=0;
    
    /*Returns the path of the file backing the entry*/
    std::string path() const {return _path;}
    
    virtual bool isMemoryMappedEntry () const {return true;};
    
    /*Allocates the mapped file*/
    virtual bool allocate(U64 byteCount, const char* path =0);
    
    /*Removes the mapped file from RAM, saving it on disk in the file
     indicated by _path*/
    virtual void deallocate();
    
    /*Assumed that allocate has already been called once.
     It is used to re-open the mapping filed. Returns false
     if it couldn't open it.*/
    bool reOpen();
    
    virtual ~MemoryMappedEntry();
};

class InMemoryEntry : public CacheEntry{
protected:
    char* _data; // < the buffer holding data
public:
    InMemoryEntry();
    
    /*allocates the buffer*/
    virtual bool allocate(U64 byteCount,const char* path = 0);
    
    virtual bool isMemoryMappedEntry () const {return false;};
    
    /*deallocate the buffer*/
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
 the most.
 The entry must derive InMemoryEntry for memory caches and MemoryMappedEntry for disk caches.
 Each cache entry for disk-caches will be under the sub-folder whose name is the 1st byte
 of the hash key.
 You have an example of both AbstractMemoryCache and AbstractDiskCache with 
 NodeCache.h and ViewerCache.h .*/
class AbstractCache {
    
public:
    
#ifdef USE_VARIADIC_TEMPLATES
    
#ifdef CACHE_USE_BOOST
#ifdef CACHE_USE_HASH
    typedef BoostLRUCacheContainer<U64, CacheEntry* , boost::bimaps::unordered_set_of> CacheContainer;
#else
    typedef BoostLRUCacheContainer<U64, CacheEntry* , boost::bimaps::set_of> CacheContainer;
#endif
    typedef CacheContainer::container_type::left_iterator CacheIterator;
    typedef CacheContainer::container_type::left_const_iterator ConstCacheIterator;
    static CacheEntry*  getValueFromIterator(CacheIterator it){return it->second;}
    
#else // cache use STL
    
#ifdef CACHE_USE_HASH
    typedef StlLRUCache<U64,CacheEntry*, std::unordered_map> CacheContainer;
#else
    typedef StlLRUCache<U64,CacheEntry*, std::map> CacheContainer;
#endif
    typedef CacheContainer::key_to_value_type::iterator CacheIterator;
    typedef CacheContainer::key_to_value_type::const_iterator ConstCacheIterator;
    static CacheEntry*  getValueFromIterator(CacheIterator it){return it->second;}
#endif // CACHE_USE_BOOST
    
#else // USE_VARIADIC_TEMPLATES
    
#ifdef CACHE_USE_BOOST
#ifdef CACHE_USE_HASH
    typedef BoostLRUHashCache<U64, CacheEntry*> CacheContainer;
#else
    typedef BoostLRUTreeCache<U64, CacheEntry* > CacheContainer;
#endif
    typedef CacheContainer::container_type::left_iterator CacheIterator;
    typedef CacheContainer::container_type::left_const_iterator ConstCacheIterator;
    static CacheEntry*  getValueFromIterator(CacheIterator it){return it->second;}
    
#else // cache use STL and tree (std map)
    
    typedef StlLRUTreeCache<U64,CacheEntry*> CacheContainer;
    typedef CacheContainer::key_to_value_type::iterator CacheIterator;
    typedef CacheContainer::key_to_value_type::const_iterator ConstCacheIterator;
    static CacheEntry*  getValueFromIterator(CacheIterator it){return it->second.first;}
#endif // CACHE_USE_BOOST
    
#endif // USE_VARIADIC_TEMPLATES
    
private:
    
    
    U64 _maximumCacheSize; // maximum size allowed for the cache
    
protected:
    //protected so derived class can modify it
    U64 _size; // current size of the cache in bytes
    
    QMutex _lock;
    CacheContainer _cache; // the actual cache 

public:
    
    AbstractCache();
    
    ~AbstractCache();
    
    /*Adds a new entry to the cache. Returns true if it removed
     another entry before inserting the new one, false otherwise.
     */
    virtual bool add(U64 key,CacheEntry* entry);
    
    
    /*clears the content of the cache (and deletes cache entries).
     Note that all entries that are not removable from the cache
     (because someone explicitly called preventFromDeletion() on them)
     will not get deleted. The size remaining will be 0 if there're no
     unremovable entries, otherwise it will be the size of these entries.*/
    void clear();
    
    
    /*Returns the name  of the cache. It will
     serve as the root folder for this cache, and will
     be located under <CacheRoot>/<cacheName()>*/
    virtual std::string cacheName()=0;
    
    
    /*Set the maximum cache size in bytes. It does not shrink the content of the cache,
     but prevent it to grow. You must clear it if you want to remove the data
     exceeding the size.*/
    void setMaximumCacheSize(U64 size){_maximumCacheSize = size;}
    
    const U64& getMaximumSize() const {return _maximumCacheSize;}
    
    const U64& getCurrentSize() const {return _size;}
    
    /*Returns an iterator to the cache. If found it points
     to a valid cache entry, otherwise it points  to end.
     */
    CacheEntry* getCacheEntry(const U64& key) ;
    

protected:
    
    /*Erase one element from the cache. It is used
     by the implementation of clear(), and shouldn't be
     called elsewhere.*/
    void erase(CacheIterator it);
    
    
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
    
    U64 _inMemorySize; // the size of the in-memory portion of the cache in bytes
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
    
    /*Returns the name of the cache with its path preprended*/
    QString getCachePath();
    
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
    
    /*Clears-out entirely the cache (RAM+disk)*/
    void clearDiskCache();
    
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
    
    /*prints out the content of the cache*/
    void debug();
    
protected:

    
    /*Returns an iterator to the cache. If found it points
     to a valid cache entry, otherwise it points to to end.
     Protected so the derived class must explicitly encapsulate
     this function: the CacheIterator should be opaque to the end user.*/
    CacheEntry* isInMemory(const U64& key);
    
    
private:
    
    /*used by the restore func.*/
    void cleanUpDiskAndReset();
    
};

#endif /* defined(__PowiterOsX__abstractCache__) */
