/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2023 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#ifndef NATRON_ENGINE_ABSTRACTCACHE_H
#define NATRON_ENGINE_ABSTRACTCACHE_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <vector>
#include <sstream> // stringstream
#include <fstream>
#include <functional>
#include <list>
#include <set>
#include <cstddef>
#include <utility>
#include <algorithm> // min, max
#include <string>
#include <stdexcept>

#include "Global/GlobalDefines.h"
#include "Global/StrUtils.h"

GCC_DIAG_OFF(deprecated)
#include <QtCore/QMutex>
#include <QtCore/QThread>
#include <QtCore/QWaitCondition>
#include <QtCore/QMutexLocker>
#include <QtCore/QObject>
#include <QtCore/QBuffer>
#include <QtCore/QRunnable>
GCC_DIAG_ON(deprecated)

#include "Engine/AppManager.h" //for access to settings
#include "Engine/CacheEntry.h"
#include "Engine/ImageLocker.h"
#include "Engine/LRUHashTable.h"
#include "Engine/MemoryInfo.h" // getSystemTotalRAM
#include "Engine/Settings.h"
#include "Engine/StandardPaths.h"

#include "Engine/EngineFwd.h"

//Beyond that percentage of occupation, the cache will start evicting LRU entries
#define NATRON_CACHE_LIMIT_PERCENT 0.9

#define NATRON_TILE_CACHE_FILE_SIZE_BYTES 2000000000

///When defined, number of opened files, memory size and disk size of the cache are printed whenever there's activity.
//#define NATRON_DEBUG_CACHE

NATRON_NAMESPACE_ENTER

/**
 * @brief The point of this thread is to delete the content of the list in a separate thread so the thread calling
 * get() doesn't wait for all the entries to be deleted (which can be expensive for large images)
 **/
template <typename T>
class DeleterThread
    : public QThread
{
    mutable QMutex _entriesQueueMutex;
    std::list<std::shared_ptr<T> >_entriesQueue;
    QWaitCondition _entriesQueueNotEmptyCond;
    CacheAPI* cache;
    QMutex mustQuitMutex;
    QWaitCondition mustQuitCond;
    bool mustQuit;

public:

    DeleterThread(CacheAPI* cache)
        : QThread()
        , _entriesQueueMutex()
        , _entriesQueue()
        , _entriesQueueNotEmptyCond()
        , cache(cache)
        , mustQuitMutex()
        , mustQuitCond()
        , mustQuit(false)
    {
        setObjectName( QString::fromUtf8("CacheDeleter") );
    }

    virtual ~DeleterThread()
    {
    }

    void appendToQueue(const std::list<std::shared_ptr<T> > & entriesToDelete)
    {
        if ( entriesToDelete.empty() ) {
            return;
        }

        {
            QMutexLocker k(&_entriesQueueMutex);
            _entriesQueue.insert( _entriesQueue.begin(), entriesToDelete.begin(), entriesToDelete.end() );
        }
        if ( !isRunning() ) {
            start();
        } else {
            QMutexLocker k(&_entriesQueueMutex);
            _entriesQueueNotEmptyCond.wakeOne();
        }
    }

    void quitThread()
    {
        if ( !isRunning() ) {
            return;
        }
        QMutexLocker k(&mustQuitMutex);
        assert(!mustQuit);
        mustQuit = true;

        {
            QMutexLocker k2(&_entriesQueueMutex);
            _entriesQueue.push_back( std::shared_ptr<T>() );
            _entriesQueueNotEmptyCond.wakeOne();
        }
        while (mustQuit) {
            mustQuitCond.wait(k.mutex());
        }
    }

    bool isWorking() const
    {
        QMutexLocker k(&_entriesQueueMutex);

        return !_entriesQueue.empty();
    }

private:

    virtual void run() OVERRIDE FINAL
    {
        for (;; ) {
            bool quit;
            {
                QMutexLocker k(&mustQuitMutex);
                quit = mustQuit;
            }

            {
                std::shared_ptr<T> front;
                {
                    QMutexLocker k(&_entriesQueueMutex);
                    if ( quit && _entriesQueue.empty() ) {
                        k.unlock();
                        QMutexLocker k2(&mustQuitMutex);
                        assert(mustQuit);
                        mustQuit = false;
                        mustQuitCond.wakeOne();

                        return;
                    }
                    while ( _entriesQueue.empty() ) {
                        _entriesQueueNotEmptyCond.wait(k.mutex());
                    }

                    assert( !_entriesQueue.empty() );
                    front = _entriesQueue.front();
                    _entriesQueue.pop_front();
                }
                if (front) {
                    front->scheduleForDestruction();
                }
            } // front. After this scope, the image is guaranteed to be freed
            cache->notifyMemoryDeallocated();
        }
    }
};


/**
 * @brief The point of this thread is to remove entries that we are sure are no longer needed
 * e.g: they may have a hash that can no longer be produced
 **/
class CacheCleanerThread
    : public QThread
{
    mutable QMutex _requestQueueMutex;
    struct CleanRequest
    {
        std::string holderID;
        U64 nodeHash;
        bool removeAll;
    };

    std::list<CleanRequest> _requestsQueues;
    QWaitCondition _requestsQueueNotEmptyCond;
    CacheAPI* cache;
    QMutex mustQuitMutex;
    QWaitCondition mustQuitCond;
    bool mustQuit;

public:

    CacheCleanerThread(CacheAPI* cache)
        : QThread()
        , _requestQueueMutex()
        , _requestsQueues()
        , _requestsQueueNotEmptyCond()
        , cache(cache)
        , mustQuitMutex()
        , mustQuitCond()
        , mustQuit(false)
    {
        setObjectName( QString::fromUtf8("CacheCleaner") );
    }

    virtual ~CacheCleanerThread()
    {
    }

    void appendToQueue(const std::string & holderID,
                       U64 nodeHash,
                       bool removeAll)
    {
        {
            QMutexLocker k(&_requestQueueMutex);
            CleanRequest r;
            r.holderID = holderID;
            r.nodeHash = nodeHash;
            r.removeAll = removeAll;
            _requestsQueues.push_back(r);
        }
        if ( !isRunning() ) {
            start();
        } else {
            QMutexLocker k(&_requestQueueMutex);
            _requestsQueueNotEmptyCond.wakeOne();
        }
    }

    void quitThread()
    {
        if ( !isRunning() ) {
            return;
        }
        QMutexLocker k(&mustQuitMutex);
        assert(!mustQuit);
        mustQuit = true;

        {
            QMutexLocker k2(&_requestQueueMutex);
            CleanRequest r;
            _requestsQueues.push_back(r);
            _requestsQueueNotEmptyCond.wakeOne();
        }
        while (mustQuit) {
            mustQuitCond.wait(k.mutex());
        }
    }

    bool isWorking() const
    {
        QMutexLocker k(&_requestQueueMutex);

        return !_requestsQueues.empty();
    }

private:

    virtual void run() OVERRIDE FINAL
    {
        for (;; ) {
            bool quit;
            {
                QMutexLocker k(&mustQuitMutex);
                quit = mustQuit;
            }

            {
                CleanRequest front;
                {
                    QMutexLocker k(&_requestQueueMutex);
                    if ( quit && _requestsQueues.empty() ) {
                        k.unlock();
                        QMutexLocker k2(&mustQuitMutex);
                        assert(mustQuit);
                        mustQuit = false;
                        mustQuitCond.wakeOne();

                        return;
                    }
                    while ( _requestsQueues.empty() ) {
                        _requestsQueueNotEmptyCond.wait(k.mutex());
                    }

                    assert( !_requestsQueues.empty() );
                    front = _requestsQueues.front();
                    _requestsQueues.pop_front();
                }
                cache->removeAllEntriesWithDifferentNodeHashForHolderPrivate(front.holderID, front.nodeHash, front.removeAll);
            }
        }
    }
};


class CacheSignalEmitter
    : public QObject
{
    Q_OBJECT

public:
    CacheSignalEmitter()
    {
    }

    ~CacheSignalEmitter()
    {
    }

    void emitSignalClearedInMemoryPortion()
    {
        Q_EMIT clearedInMemoryPortion();
    }

    void emitClearedDiskPortion()
    {
        Q_EMIT clearedDiskPortion();
    }

    void emitAddedEntry(SequenceTime time)
    {
        Q_EMIT addedEntry(time);
    }

    void emitRemovedEntry(SequenceTime time,
                          int storage)
    {
        Q_EMIT removedEntry(time, storage);
    }

    void emitEntryStorageChanged(SequenceTime time,
                                 int oldStorage,
                                 int newStorage)
    {
        Q_EMIT entryStorageChanged(time, oldStorage, newStorage);
    }

Q_SIGNALS:

    void clearedInMemoryPortion();

    void clearedDiskPortion();

    void addedEntry(SequenceTime);
    void removedEntry(SequenceTime, int);
    void entryStorageChanged(SequenceTime, int, int);
};


/*
 * ValueType must be derived of CacheEntryHelper
 */
template<typename EntryType>
class Cache
    : public CacheAPI
{
    friend class CacheCleanerThread;
public:

    typedef typename EntryType::hash_type hash_type;
    typedef typename EntryType::data_t data_t;
    typedef typename EntryType::key_t key_t;
    typedef typename EntryType::param_t param_t;
    typedef std::shared_ptr<param_t> ParamsTypePtr;
    typedef std::shared_ptr<EntryType> EntryTypePtr;

    struct SerializedEntry;

    typedef std::list<SerializedEntry> CacheTOC;

public:


#ifdef USE_VARIADIC_TEMPLATES

#ifdef NATRON_CACHE_USE_BOOST
#ifdef NATRON_CACHE_USE_HASH
    typedef BoostLRUHashTable<hash_type, EntryTypePtr>, boost::bimaps::unordered_set_of > CacheContainer;
#else
    typedef BoostLRUHashTable<hash_type, EntryTypePtr>, boost::bimaps::set_of > CacheContainer;
#endif
    typedef typename CacheContainer::container_type::left_iterator CacheIterator;
    typedef typename CacheContainer::container_type::left_const_iterator ConstCacheIterator;
    static std::list<CachedValue> &  getValueFromIterator(CacheIterator it)
    {
        return it->second;
    }
    static const std::list<CachedValue> &  getValueFromIterator(ConstCacheIterator it)
    {
        return it->second;
    }

#else // cache use STL

#ifdef NATRON_CACHE_USE_HASH
    typedef StlLRUHashTable<hash_type, EntryTypePtr>, std::unordered_map > CacheContainer;
#else
    typedef StlLRUHashTable<hash_type, EntryTypePtr>, std::map > CacheContainer;
#endif
    typedef typename CacheContainer::key_to_value_type::iterator CacheIterator;
    typedef typename CacheContainer::key_to_value_type::const_iterator ConstCacheIterator;
    static std::list<EntryTypePtr> &  getValueFromIterator(CacheIterator it)
    {
        return it->second;
    }
    static const std::list<EntryTypePtr> &  getValueFromIterator(ConstCacheIterator it)
    {
        return it->second;
    }

#endif // NATRON_CACHE_USE_BOOST

#else // !USE_VARIADIC_TEMPLATES

#ifdef NATRON_CACHE_USE_BOOST
#ifdef NATRON_CACHE_USE_HASH
    typedef BoostLRUHashTable<hash_type, EntryTypePtr> CacheContainer;
#else
    typedef BoostLRUHashTable<hash_type, EntryTypePtr> CacheContainer;
#endif
    typedef typename CacheContainer::container_type::left_iterator CacheIterator;
    typedef typename CacheContainer::container_type::left_const_iterator ConstCacheIterator;
    static std::list<EntryTypePtr> &  getValueFromIterator(CacheIterator it)
    {
        return it->second;
    }
    static const std::list<EntryTypePtr> &  getValueFromIterator(ConstCacheIterator it)
    {
        return it->second;
    }

#else // cache use STL and tree (std map)

    typedef StlLRUHashTable<hash_type, EntryTypePtr> CacheContainer;
    typedef typename CacheContainer::key_to_value_type::iterator CacheIterator;
    typedef typename CacheContainer::key_to_value_type::const_iterator ConstCacheIterator;
    static std::list<EntryTypePtr> &   getValueFromIterator(CacheIterator it)
    {
        return it->second.first;
    }
    static const std::list<EntryTypePtr> &   getValueFromIterator(ConstCacheIterator it)
    {
        return it->second.first;
    }

#endif // NATRON_CACHE_USE_BOOST

#endif // USE_VARIADIC_TEMPLATES

private:


    std::size_t _maximumInMemorySize;     // the maximum size of the in-memory portion of the cache.(in % of the maximum cache size)
    std::size_t _maximumCacheSize;     // maximum size allowed for the cache

    /*mutable because we need to change modify it in the sealEntryInternal function which
         is called by an external object that have a const ref to the cache.
     */
    mutable std::size_t _memoryCacheSize;     // current size of the cache in bytes
    mutable std::size_t _diskCacheSize;
    mutable QMutex _sizeLock; // protects _memoryCacheSize & _diskCacheSize & _maximumInMemorySize & _maximumCacheSize
    mutable QMutex _lock; //protects _memoryCache & _diskCache
    mutable QMutex _getLock;  //prevents get() and getOrCreate() to be called simultaneously


    /*These 2 are mutable because we need to modify the LRU list even
         when we call get() and we want this function to be const.*/
    mutable CacheContainer _memoryCache;
    mutable CacheContainer _diskCache;
    const std::string _cacheName;
    const unsigned int _version;

    /*mutable because it doesn't hold any data, it just emits signals but signals cannot
         be const somehow .*/
    mutable CacheSignalEmitterPtr _signalEmitter;

    ///Store the system physical total RAM in a member
    std::size_t _maxPhysicalRAM;
    bool _tearingDown;
    mutable DeleterThread<EntryType> _deleterThread;
    mutable QWaitCondition _memoryFullCondition; //< protected by _sizeLock
    mutable CacheCleanerThread _cleanerThread;

    // If tiled, the cache will consist only of a few large files that each contain tiles of the same size.
    // This is useful to cache chunks of data that always have the same size.
    mutable QMutex _tileCacheMutex;
    bool _isTiled;
    std::size_t _tileByteSize;

    // True when clearing the cache, protected by _tileCacheMutex
    bool _clearingCache;

    // Used when the cache is tiled
    std::set<TileCacheFilePtr> _cacheFiles;

    // When set these are used for fast search of a free tile
    TileCacheFileWPtr _nextAvailableCacheFile;
    int _nextAvailableCacheFileIndex;
public:


    Cache(const std::string & cacheName,
          unsigned int version,
          U64 maximumCacheSize,      // total size
          double maximumInMemoryPercentage //how much should live in RAM
          )
        : CacheAPI()
        , _maximumInMemorySize(maximumCacheSize * maximumInMemoryPercentage)
        , _maximumCacheSize(maximumCacheSize)
        , _memoryCacheSize(0)
        , _diskCacheSize(0)
        , _sizeLock()
        , _lock()
        , _getLock()
        , _memoryCache()
        , _diskCache()
        , _cacheName(cacheName)
        , _version(version)
        , _signalEmitter()
        , _maxPhysicalRAM( getSystemTotalRAM() )
        , _tearingDown(false)
        , _deleterThread(this)
        , _memoryFullCondition()
        , _cleanerThread(this)
        , _tileCacheMutex()
        , _isTiled(false)
        , _tileByteSize(0)
        , _clearingCache(false)
        , _cacheFiles()
        , _nextAvailableCacheFile()
        , _nextAvailableCacheFileIndex(-1)
    {
        _signalEmitter = std::make_shared<CacheSignalEmitter>();
    }

    virtual ~Cache()
    {
        QMutexLocker locker(&_lock);

        _tearingDown = true;
        _memoryCache.clear();
        _diskCache.clear();
    }

    virtual bool isTileCache() const OVERRIDE FINAL
    {
        QMutexLocker k(&_tileCacheMutex);
        return _isTiled;
    }

    virtual std::size_t getTileSizeBytes() const OVERRIDE FINAL
    {
        QMutexLocker k(&_tileCacheMutex);
        return _tileByteSize;
    }

    /**
     * @brief Set the cache to be in tile mode.
     * If tiled, the cache will consist only of a few large files that each contain tiles of the same size.
     * This is useful to cache chunks of data that always have the same size.
     **/
    void setTiled(bool tiled, std::size_t tileByteSize)
    {
        QMutexLocker k(&_tileCacheMutex);
        _isTiled = tiled;
        _tileByteSize = tileByteSize;
    }


    void waitForDeleterThread()
    {
        _deleterThread.quitThread();
        _cleanerThread.quitThread();
    }

    /**
     * @brief Look-up the cache for an entry whose key matches the params.
     * @param params The key identifying the entry we're looking for.
     * @param [out] params A pointer to the params that go along the returnValue.
     * They do not help to identify they entry, rather
     * this class can be used to cache other parameters along with the value_type.
     * @param [out] returnValue The returnValue, contains the cache entry if the return value
     * of the function is true, otherwise the pointer is left untouched.
     * @returns True if the cache successfully found an entry matching the params.
     * False otherwise.
     **/
    bool get(const typename EntryType::key_type & key,
             std::list<EntryTypePtr>* returnValue) const
    {
        ///Be atomic, so it cannot be created by another thread in the meantime
        QMutexLocker getlocker(&_getLock);

        ///lock the cache before reading it.
        QMutexLocker locker(&_lock);

        return getInternal(key, returnValue);
    } // get

private:



    virtual TileCacheFilePtr getTileCacheFile(const std::string& filepath, std::size_t dataOffset) OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        QMutexLocker k(&_tileCacheMutex);
        assert(_isTiled);
        if (!_isTiled) {
            throw std::logic_error("allocTile() but cache is not tiled!");
        }
        for (std::set<TileCacheFilePtr>::iterator it = _cacheFiles.begin(); it != _cacheFiles.end(); ++it) {
            if ((*it)->file->path() == filepath) {
                int index = dataOffset / _tileByteSize;

                // The dataOffset should be a multiple of the tile size
                assert(_tileByteSize * index == dataOffset);
                assert(!(*it)->usedTiles[index]);
                (*it)->usedTiles[index] = true;
                return *it;
            }
        }
        if (!fileExists(filepath)) {
            return TileCacheFilePtr();
        } else {
            TileCacheFilePtr ret = std::make_shared<TileCacheFile>();
            ret->file = std::make_shared<MemoryFile>(filepath, MemoryFile::eFileOpenModeEnumIfExistsKeepElseFail);
            std::size_t nTilesPerFile = std::floor( ( (double)NATRON_TILE_CACHE_FILE_SIZE_BYTES ) / _tileByteSize );
            ret->usedTiles.resize(nTilesPerFile, false);
            int index = dataOffset / _tileByteSize;

            // The dataOffset should be a multiple of the tile size
            assert(_tileByteSize * index == dataOffset);
            assert(index >= 0 && index < (int)ret->usedTiles.size());
            assert(!ret->usedTiles[index]);
            ret->usedTiles[index] = true;
            _cacheFiles.insert(ret);
            return ret;

        }

    }

    /**
     * @brief Relevant only for tiled caches. This will allocate the memory required for a tile in the cache and lock it.
     * Note that the calling entry should have exactly the size of a tile in the cache.
     * In return, a pointer to a memory file is returned and the output parameter dataOffset will be set to the offset - in bytes - where the
     * contiguous memory block for this tile begin relative to the start of the data of the memory file.
     * This function may throw exceptions in case of failure.
     * To retrieve the exact pointer of the block of memory for this tile use tileFile->file->data() + dataOffset
     **/
    virtual TileCacheFilePtr allocTile(std::size_t *dataOffset) OVERRIDE FINAL
    {

        QMutexLocker k(&_tileCacheMutex);

        assert(_isTiled);
        if (!_isTiled) {
            throw std::logic_error("allocTile() but cache is not tiled!");
        }
        // First, search for a file with available space.
        // If not found create one
        TileCacheFilePtr foundAvailableFile;
        int foundTileIndex = -1;
        {
            foundAvailableFile = _nextAvailableCacheFile.lock();
            if (_nextAvailableCacheFileIndex != -1 && foundAvailableFile) {
                foundTileIndex = _nextAvailableCacheFileIndex;
                *dataOffset = foundTileIndex * _tileByteSize;
                _nextAvailableCacheFileIndex = -1;
                _nextAvailableCacheFile.reset();
            } else {
                foundTileIndex = -1;
                foundAvailableFile.reset();
            }
        }
        if (foundTileIndex == -1) {
            for (std::set<TileCacheFilePtr>::iterator it = _cacheFiles.begin(); it != _cacheFiles.end(); ++it) {
                for (std::size_t i = 0; i < (*it)->usedTiles.size(); ++i) {
                    if (!(*it)->usedTiles[i])  {
                        foundTileIndex = i;
                        *dataOffset = i * _tileByteSize;
                        break;
                    }
                }
                if (foundTileIndex != -1) {
                    foundAvailableFile = *it;
                    break;
                }
            }
        }

        if (!foundAvailableFile) {
            // Create a file if all space is taken
            foundAvailableFile = std::make_shared<TileCacheFile>();
            int nCacheFiles = (int)_cacheFiles.size();
            std::stringstream cacheFilePathSs;
            cacheFilePathSs << getCachePath().toStdString() << "/CachePart" << nCacheFiles;
            std::string cacheFilePath = cacheFilePathSs.str();
            foundAvailableFile->file = std::make_shared<MemoryFile>(cacheFilePath, MemoryFile::eFileOpenModeEnumIfExistsKeepElseCreate);

            std::size_t nTilesPerFile = std::floor(((double)NATRON_TILE_CACHE_FILE_SIZE_BYTES) / _tileByteSize);
            std::size_t cacheFileSize = nTilesPerFile * _tileByteSize;
            foundAvailableFile->file->resize(cacheFileSize);
            foundAvailableFile->usedTiles.resize(nTilesPerFile, false);
            *dataOffset = 0;
            foundTileIndex = 0;
            _cacheFiles.insert(foundAvailableFile);
            _nextAvailableCacheFile = foundAvailableFile;
            _nextAvailableCacheFileIndex = 1;
        }

        // Notify the memory file that this portion of the file is valid
        foundAvailableFile->usedTiles[foundTileIndex] = true;
        return foundAvailableFile;
    }

            /**
             * @brief Free a tile from the cache that was previously allocated with allocTile. It will be made available again for other entries.
             **/
    virtual void freeTile(const TileCacheFilePtr& file, std::size_t dataOffset) OVERRIDE FINAL
    {
        QMutexLocker k(&_tileCacheMutex);

        assert(_isTiled);
        if (!_isTiled) {
            throw std::logic_error("allocTile() but cache is not tiled!");
        }
        std::set<TileCacheFilePtr>::iterator foundTileFile = _cacheFiles.find(file);
        assert(foundTileFile != _cacheFiles.end());
        if (foundTileFile == _cacheFiles.end()) {
            return;
        }
        int index = dataOffset / _tileByteSize;

        // The dataOffset should be a multiple of the tile size
        assert(_tileByteSize * index == dataOffset);
        assert(index >= 0 && index < (int)(*foundTileFile)->usedTiles.size());
        assert((*foundTileFile)->usedTiles[index]);
        (*foundTileFile)->usedTiles[index] = false;

        // If the file does not have any tile associated, remove it
        // A use_count of 2 means that the tile file is only referenced by the cache itself and the entry calling
        // the freeTile() function, hence once its freed, no tile should be using it anymore
        if ((*foundTileFile).use_count() <= 2) {
            // Do not remove the file except if we are clearing the cache
            if (_clearingCache) {
                (*foundTileFile)->file->remove();
                _cacheFiles.erase(foundTileFile);
            } else {
                // Invalidate this portion of the cache
                (*foundTileFile)->file->flush(MemoryFile::eFlushTypeInvalidate, (*foundTileFile)->file->data() + dataOffset, _tileByteSize);
            }
        } else {
            _nextAvailableCacheFile = *foundTileFile;
            _nextAvailableCacheFileIndex = index;
        }
    }


    void createInternal(const typename EntryType::key_type & key,
                        const ParamsTypePtr & params,
                        ImageLockerHelper<EntryType>* entryLocker,
                        EntryTypePtr* returnValue) const
    {
        //_lock must not be taken here

        ///Before allocating the memory check that there's enough space to fit in memory
        appPTR->checkCacheFreeMemoryIsGoodEnough();


        ///Just in case, we don't allow more than X files to be removed at once.
        int safeCounter = 0;
        ///If too many files are opened, fall-back on RAM storage.
        while (!_isTiled && appPTR->isNCacheFilesOpenedCapped() && safeCounter < 1000) {
#ifdef NATRON_DEBUG_CACHE
            qDebug() << "Reached maximum cache files opened limit,clearing last recently used one...";
#endif
            if ( !evictLRUDiskEntry() ) {
                break;
            }
            ++safeCounter;
        }

        U64 memoryCacheSize, maximumInMemorySize;
        {
            QMutexLocker k(&_sizeLock);
            memoryCacheSize = _memoryCacheSize;
            maximumInMemorySize = std::max( (std::size_t)1, _maximumInMemorySize );
        }
        {
            QMutexLocker locker(&_lock);
            std::list<EntryTypePtr> entriesToBeDeleted;
            double occupationPercentage = (double)memoryCacheSize / maximumInMemorySize;
            ///While the current cache size can't fit the new entry, erase the last recently used entries.
            ///Also if the total free RAM is under the limit of the system free RAM to keep free, erase LRU entries.
            while (occupationPercentage > NATRON_CACHE_LIMIT_PERCENT) {
                std::list<EntryTypePtr> deleted;
                if ( !tryEvictInMemoryEntry(deleted) ) {
                    break;
                }

                for (typename std::list<EntryTypePtr>::iterator it = deleted.begin(); it != deleted.end(); ++it) {
                    entriesToBeDeleted.push_back(*it);
                    memoryCacheSize -= (*it)->size();
                }

                occupationPercentage = (double)memoryCacheSize / maximumInMemorySize;
            }

            if ( !entriesToBeDeleted.empty() ) {
                ///Launch a separate thread whose function will be to delete all the entries to be deleted
                _deleterThread.appendToQueue(entriesToBeDeleted);

                ///Clearing the list here will not delete the objects pointing to by the shared_ptr's because we made a copy
                ///that the separate thread will delete
                entriesToBeDeleted.clear();
            }
        }
        {
            //If _maximumcacheSize == 0 we don't return 1 otherwise we would cause a deadlock
            QMutexLocker k(&_sizeLock);
            double occupationPercentage =  _maximumCacheSize == 0 ? 0.99 : (double)_memoryCacheSize / _maximumCacheSize;

            //_memoryCacheSize member will get updated while images are being destroyed by the parallel thread.
            //we wait for cache memory occupation to be < 100% to be sure we don't hit swap here
            while ( occupationPercentage >= 1. && _deleterThread.isWorking() ) {
                _memoryFullCondition.wait(k.mutex());
                occupationPercentage =  _maximumCacheSize == 0 ? 0.99 : (double)_memoryCacheSize / _maximumCacheSize;
            }
        }
        if (_isTiled) {

            QMutexLocker locker(&_lock);
            // For tiled caches, we insert directly into the disk cache, so make sure there is room for it
            std::list<EntryTypePtr> entriesToBeDeleted;
            U64 diskCacheSize, maximumDiskCacheSize;
            {
                QMutexLocker k(&_sizeLock);
                diskCacheSize = _diskCacheSize;
                maximumDiskCacheSize = std::max( (std::size_t)1, _maximumCacheSize - _maximumInMemorySize );
            }
            double diskPercentage = (double)diskCacheSize / maximumDiskCacheSize;
            while (diskPercentage >= NATRON_CACHE_LIMIT_PERCENT) {
                std::list<EntryTypePtr> deleted;
                if ( !tryEvictDiskEntry(deleted) ) {
                    break;
                }

                for (typename std::list<EntryTypePtr>::iterator it = deleted.begin(); it != deleted.end(); ++it) {
                    diskCacheSize -= (*it)->size();
                    entriesToBeDeleted.push_back(*it);
                }
                diskPercentage = (double)diskCacheSize / maximumDiskCacheSize;
            }
            if ( !entriesToBeDeleted.empty() ) {
                ///Launch a separate thread whose function will be to delete all the entries to be deleted
                _deleterThread.appendToQueue(entriesToBeDeleted);

                ///Clearing the list here will not delete the objects pointing to by the shared_ptr's because we made a copy
                ///that the separate thread will delete
                entriesToBeDeleted.clear();
            }

        }
        {
            QMutexLocker locker(&_lock);

            try {
                returnValue->reset( new EntryType(key, params, this ) );

                ///Don't call allocateMemory() here because we're still under the lock and we might force tons of threads to wait unnecesserarily
            } catch (const std::bad_alloc & e) {
                *returnValue = EntryTypePtr();
            }

            // For a tiled cache, all entries must have the same size
            assert(!_isTiled || (*returnValue)->getSizeInBytesFromParams() == _tileByteSize);

            if (*returnValue) {

                // If there is a lock, lock it before exposing the entry to other threads
                if (entryLocker) {
                    entryLocker->lock(*returnValue);
                }
                sealEntry(*returnValue, _isTiled ? false : true);
            }
        }
    } // createInternal

public:

    void swapOrInsert(const EntryTypePtr& entryToBeEvicted,
                      const EntryTypePtr& newEntry)
    {
        QMutexLocker locker(&_lock);

        const typename EntryType::key_type& key = entryToBeEvicted->getKey();
        typename EntryType::hash_type hash = entryToBeEvicted->getHashKey();
        ///find a matching value in the internal memory container
        CacheIterator memoryCached = _memoryCache(hash);
        if ( memoryCached != _memoryCache.end() ) {
            std::list<EntryTypePtr> & ret = getValueFromIterator(memoryCached);
            for (typename std::list<EntryTypePtr>::iterator it = ret.begin(); it != ret.end(); ++it) {
                if ( ( (*it)->getKey() == key ) && ( (*it)->getParams() == entryToBeEvicted->getParams() ) ) {
                    ret.erase(it);
                    break;
                }
            }
            ///Append it
            ret.push_back(newEntry);
        } else {
            ///Look in disk cache
            CacheIterator diskCached = _diskCache(hash);
            if ( diskCached != _diskCache.end() ) {
                ///Remove the old entry
                std::list<EntryTypePtr> & ret = getValueFromIterator(diskCached);
                for (typename std::list<EntryTypePtr>::iterator it = ret.begin(); it != ret.end(); ++it) {
                    if ( ( (*it)->getKey() == key ) && ( (*it)->getParams() == entryToBeEvicted->getParams() ) ) {
                        ret.erase(it);
                        break;
                    }
                }
            }
            ///Insert in mem cache
            _memoryCache.insert(hash, newEntry);
        }
    }

    /**
     * @brief Look-up the cache for an entry whose key matches the 'key' and 'params'.
     * Unlike get(...) this function creates a new entry if it couldn't be found.
     * Note that this function also takes extra parameters which go along the value_type
     * and will be cached. These parameters aren't taken into account for the computation of
     * the hash key. It is a safe place to cache any extra data that is relative to an entry,
     * but doesn't make it an identifier of that entry. The base class just adds the necessary
     * info for the cache to be able to instantiate a new entry (that is the cost and the elements count).
     *
     * @param key The key identifying the entry we're looking for.
     *
     * @param params The non unique parameters. They do not help to identify they entry, rather
     * this class can be used to cache other parameters along with the value_type.
     *
     * @param imageLocker A pointer to an ImageLockerHelperBase which will lock the image if it was freshly
     * created so that you can call allocateMemory() safely without another thread accessing it.
     *
     * @param [out] returnValue The returnValue, contains the cache entry.
     * Internally the allocation of the new entry might fail on the requested device,
     * e.g: if you ask for an entry with a large cost, the cache will try to put the
     * entry on disk to preserve it, but if the allocation failed it will fallback
     * on RAM instead.
     *
     * Either way the returnValue parameter can never be NULL.
     * @returns True if the cache successfully found an entry matching the key.
     * False otherwise.
     **/
    bool getOrCreate(const typename EntryType::key_type & key,
                     const ParamsTypePtr & params,
                     ImageLockerHelper<EntryType>* locker,
                     EntryTypePtr* returnValue) const
    {
        ///Make sure the shared_ptrs live in this list and are destroyed not while under the lock
        ///so that the memory freeing (which might be expensive for large images) doesn't happen while under the lock

        {
            ///Be atomic, so it cannot be created by another thread in the meantime
            QMutexLocker getlocker(&_getLock);
            std::list<EntryTypePtr> entries;
            bool didGetSucceed;
            {
                QMutexLocker locker(&_lock);
                didGetSucceed = getInternal(key, &entries);
            }
            if (didGetSucceed) {
                for (typename std::list<EntryTypePtr>::iterator it = entries.begin(); it != entries.end(); ++it) {
                    if (*(*it)->getParams() == *params) {
                        *returnValue = *it;

                        return true;
                    }
                }
            }

            createInternal(key, params, locker, returnValue);

            return false;
        } // getlocker
    }

    /**
     * @brief Clears entirely the disk portion and memory portion.
     **/
    void clear()
    {
        {
            QMutexLocker k(&_tileCacheMutex);
            _clearingCache = true;
        }
        clearDiskPortion();


        if (_signalEmitter) {
            ///block signals otherwise the we would be spammed of notifications
            _signalEmitter->blockSignals(true);
        }
        QMutexLocker locker(&_lock);
        std::pair<hash_type, EntryTypePtr> evictedFromMemory = _memoryCache.evict();
        while (evictedFromMemory.second) {
            if ( !_isTiled && evictedFromMemory.second->isStoredOnDisk() ) {
                evictedFromMemory.second->removeAnyBackingFile();
            }
            evictedFromMemory = _memoryCache.evict();
        }

        if (_signalEmitter) {
            _signalEmitter->blockSignals(false);
            _signalEmitter->emitSignalClearedInMemoryPortion();
        }

        {
            QMutexLocker k(&_tileCacheMutex);
            _clearingCache = false;
        }
    }

    /**
     * @brief Clears all entries on disk that are not actively being used somewhere else in the application.
     * Any entry being used by the application will be left in the cache.
     **/
    void clearDiskPortion()
    {
        if (_signalEmitter) {
            ///block signals otherwise the we would be spammed of notifications
            _signalEmitter->blockSignals(true);
        }
        QMutexLocker locker(&_lock);

        /// An entry which has a use_count greater than 1 is not removable:
        /// The backing file must not be removed because it might be read/written to
        /// at the same time. The best we can do is just let it here in the cache.
        std::pair<hash_type, EntryTypePtr> evictedFromDisk = _diskCache.evict();
        //if the cache couldn't evict that means all entries are used somewhere and we shall not remove them!
        //we'll let the user of these entries purge the extra entries left in the cache later on
        while (evictedFromDisk.second) {
            if (!_isTiled) {
                evictedFromDisk.second->removeAnyBackingFile();
            }
            evictedFromDisk = _diskCache.evict();
        }


        _signalEmitter->blockSignals(false);
        _signalEmitter->emitClearedDiskPortion();
    }

    /**
     * @brief Clears the memory portion and moves it to the disk portion if possible
     **/
    void clearInMemoryPortion(bool emitSignals = true)
    {
        if (_signalEmitter) {
            ///block signals otherwise the we would be spammed of notifications
            _signalEmitter->blockSignals(true);
        }
        QMutexLocker locker(&_lock);
        std::pair<hash_type, EntryTypePtr> evictedFromMemory = _memoryCache.evict();
        while (evictedFromMemory.second) {
            // Move back the entry on disk if it can be store on disk
            // For tiled caches, the tile is sharing the same file with other entries
            // so we cannot close it, just remove the entry
            if ( evictedFromMemory.second->isStoredOnDisk() && !_isTiled) {
                evictedFromMemory.second->deallocate();
                /*insert it back into the disk portion */

                U64 diskCacheSize, maximumCacheSize;
                {
                    QMutexLocker k(&_sizeLock);
                    diskCacheSize = _diskCacheSize;
                    maximumCacheSize = _maximumCacheSize;
                }

                /*before that we need to clear the disk cache if it exceeds the maximum size allowed*/
                while (diskCacheSize + evictedFromMemory.second->size() >= maximumCacheSize) {
                    {
                        std::pair<hash_type, EntryTypePtr> evictedFromDisk = _diskCache.evict();
                        //if the cache couldn't evict that means all entries are used somewhere and we shall not remove them!
                        //we'll let the user of these entries purge the extra entries left in the cache later on
                        if (!evictedFromDisk.second) {
                            break;
                        }
                        ///Erase the file from the disk if we reach the limit.
                        evictedFromDisk.second->removeAnyBackingFile();
                    }
                    {
                        QMutexLocker k(&_sizeLock);
                        diskCacheSize = _diskCacheSize;
                        maximumCacheSize = _maximumCacheSize;
                    }
                }

                /*update the disk cache size*/
                CacheIterator existingDiskCacheEntry = _diskCache( evictedFromMemory.second->getHashKey() );
                /*if the entry doesn't exist on the disk cache,make a new list and insert it*/
                if ( existingDiskCacheEntry == _diskCache.end() ) {
                    _diskCache.insert(evictedFromMemory.second->getHashKey(), evictedFromMemory.second);
                }
            }

            evictedFromMemory = _memoryCache.evict();
        }

        _signalEmitter->blockSignals(false);
        if (emitSignals) {
            _signalEmitter->emitSignalClearedInMemoryPortion();
        }
    } // clearInMemoryPortion

    void clearExceedingEntries()
    {
        ///Make sure the shared_ptrs live in this list and are destroyed not while under the lock
        ///so that the memory freeing (which might be expensive for large images) doesn't happen while under the lock
        std::list<EntryTypePtr> entriesToBeDeleted;

        {
            QMutexLocker locker(&_lock);
            U64 memoryCacheSize, maximumInMemorySize;
            {
                QMutexLocker k(&_sizeLock);
                memoryCacheSize = _memoryCacheSize;
                maximumInMemorySize = std::max( (std::size_t)1, _maximumInMemorySize );
            }
            double occupationPercentage = (double)memoryCacheSize / maximumInMemorySize;
            while (occupationPercentage >= NATRON_CACHE_LIMIT_PERCENT) {
                std::list<EntryTypePtr> deleted;
                if ( !tryEvictInMemoryEntry(deleted) ) {
                    break;
                }

                for (typename std::list<EntryTypePtr>::iterator it = deleted.begin(); it != deleted.end(); ++it) {
                    if ( !(*it)->isStoredOnDisk() ) {
                        memoryCacheSize -= (*it)->size();
                    }
                    entriesToBeDeleted.push_back(*it);
                }
                occupationPercentage = (double)memoryCacheSize / maximumInMemorySize;
            }

            U64 diskCacheSize, maximumDiskCacheSize;
            {
                QMutexLocker k(&_sizeLock);
                diskCacheSize = _diskCacheSize;
                maximumDiskCacheSize = std::max( (std::size_t)1, _maximumCacheSize - _maximumInMemorySize );
            }
            double diskPercentage = (double)diskCacheSize / maximumDiskCacheSize;
            while (diskPercentage >= NATRON_CACHE_LIMIT_PERCENT) {
                std::list<EntryTypePtr> deleted;
                if ( !tryEvictDiskEntry(deleted) ) {
                    break;
                }

                for (typename std::list<EntryTypePtr>::iterator it = deleted.begin(); it != deleted.end(); ++it) {
                    diskCacheSize -= (*it)->size();
                    entriesToBeDeleted.push_back(*it);
                }
                diskPercentage = (double)diskCacheSize / maximumDiskCacheSize;
            }
            

        }
    }

    /**
     * @brief Get a copy of the cache at the moment it gets the lock for reading.
     * Returning this function, the caller can assume the entries will not be removed
     * from the cache because their use_count is > 1
     **/
    void getCopy(std::list<EntryTypePtr>* copy) const
    {
        QMutexLocker locker(&_lock);

        for (CacheIterator it = _memoryCache.begin(); it != _memoryCache.end(); ++it) {
            const std::list<EntryTypePtr> & entries = getValueFromIterator(it);
            copy->insert( copy->end(), entries.begin(), entries.end() );
        }
        for (CacheIterator it = _diskCache.begin(); it != _diskCache.end(); ++it) {
            const std::list<EntryTypePtr> & entries = getValueFromIterator(it);
            copy->insert( copy->end(), entries.begin(), entries.end() );
        }
    }

    /**
     * @brief Removes the last recently used entry from the in-memory cache.
     * This is expensive since it takes the lock. Returns false
     * if there's nothing left to evict.
     **/
    bool evictLRUInMemoryEntry() const
    {
        ///Make sure the shared_ptrs live in this list and are destroyed not while under the lock
        ///so that the memory freeing (which might be expensive for large images) doesn't happen while under the lock
        std::list<EntryTypePtr> entriesToBeDeleted;
        bool ret;
        {
            QMutexLocker locker(&_lock);
            ret = tryEvictInMemoryEntry(entriesToBeDeleted);
        }

        return ret;
    }

    /**
     * @brief Removes the last recently used entry from the disk cache.
     * This is expensive since it takes the lock. Returns false
     * if there's nothing left to evict.
     **/
    bool evictLRUDiskEntry() const
    {

        QMutexLocker locker(&_lock);
        std::list<EntryTypePtr> entriesToBeDeleted;
        return tryEvictDiskEntry(entriesToBeDeleted);
    }

    /**
     * @brief To be called by a CacheEntry whenever it's size changes.
     * This way the cache can keep track of the real memory footprint.
     **/
    virtual void notifyEntrySizeChanged(std::size_t oldSize,
                                        std::size_t newSize) const OVERRIDE FINAL
    {
        ///The entry has notified it's memory layout has changed, it must have been due to an action from the cache
        QMutexLocker k(&_sizeLock);

        ///This function can only be called for RAM buffers or while a memory mapped file is mapped into the RAM, so
        ///we just have to modify the RAM size.

        ///Avoid overflows, _memoryCacheSize may not always fallback to 0
        qint64 diff = (qint64)newSize - (qint64)oldSize;

        if (diff < 0) {
            _memoryCacheSize = diff > (qint64)_memoryCacheSize ? 0 : _memoryCacheSize + diff;
        } else {
            _memoryCacheSize += diff;
        }
#ifdef NATRON_DEBUG_CACHE
        qDebug() << cacheName().c_str() << " memory size: " << printAsRAM(_memoryCacheSize);
#endif
    }

    /**
     * @brief To be called by a CacheEntry on allocation.
     **/
    virtual void notifyEntryAllocated(double time,
                                      std::size_t size,
                                      StorageModeEnum storage) const OVERRIDE FINAL
    {
        ///The entry has notified it's memory layout has changed, it must have been due to an action from the cache, hence the
        ///lock should already be taken.
        QMutexLocker k(&_sizeLock);

        if (storage == eStorageModeDisk) {
            if (_isTiled) {
                // For tile caches, we do not control which portion of the cache is in memory, so just keep track of the disk portion
                _diskCacheSize += size;
            } else {
                _memoryCacheSize += size;
                appPTR->increaseNCacheFilesOpened();
            }
        } else {
            _memoryCacheSize += size;
        }

        _signalEmitter->emitAddedEntry(time);


#ifdef NATRON_DEBUG_CACHE
        qDebug() << cacheName().c_str() << " memory size: " << printAsRAM(_memoryCacheSize);
#endif
    }

    /**
     * @brief To be called by a CacheEntry on destruction.
     **/
    virtual void notifyEntryDestroyed(double time,
                                      std::size_t size,
                                      StorageModeEnum storage) const OVERRIDE FINAL
    {
        QMutexLocker k(&_sizeLock);

        if (storage == eStorageModeRAM) {
            _memoryCacheSize = size > _memoryCacheSize ? 0 : _memoryCacheSize - size;
#ifdef NATRON_DEBUG_CACHE
            qDebug() << cacheName().c_str() << " memory size: " << printAsRAM(_memoryCacheSize);
#endif
        } else if (storage == eStorageModeDisk) {
            _diskCacheSize = size > _diskCacheSize ? 0 : _diskCacheSize - size;
#ifdef NATRON_DEBUG_CACHE
            qDebug() << cacheName().c_str() << " disk size: " << printAsRAM(_diskCacheSize);
#endif
        }


        _signalEmitter->emitRemovedEntry(time, (int)storage);
    }

    virtual void notifyMemoryDeallocated() const OVERRIDE FINAL
    {
        QMutexLocker k(&_sizeLock);

        _memoryFullCondition.wakeAll();
    }

    /**
     * @brief To be called whenever an entry is deallocated from memory and put back on disk or whenever
     * it is reallocated in the RAM.
     **/
    virtual void notifyEntryStorageChanged(StorageModeEnum oldStorage,
                                           StorageModeEnum newStorage,
                                           double time,
                                           std::size_t size) const OVERRIDE FINAL
    {
        assert(!_isTiled);

        if (_tearingDown) {
            return;
        }
        QMutexLocker k(&_sizeLock);

        assert(oldStorage != newStorage);
        assert(newStorage != eStorageModeNone);
        if (oldStorage == eStorageModeRAM) {
            _memoryCacheSize = size > _memoryCacheSize ? 0 : _memoryCacheSize - size;
            _diskCacheSize += size;
#ifdef NATRON_DEBUG_CACHE
            qDebug() << cacheName().c_str() << " memory size: " << printAsRAM(_memoryCacheSize);
            qDebug() << cacheName().c_str() << " disk size: " << printAsRAM(_diskCacheSize);
#endif
            ///We switched from RAM to DISK that means the MemoryFile object has been destroyed hence the file has been closed.
            appPTR->decreaseNCacheFilesOpened();
        } else if (oldStorage == eStorageModeDisk) {
            _memoryCacheSize += size;
            _diskCacheSize = size > _diskCacheSize ? 0 : _diskCacheSize - size;
#ifdef NATRON_DEBUG_CACHE
            qDebug() << cacheName().c_str() << " memory size: " << printAsRAM(_memoryCacheSize);
            qDebug() << cacheName().c_str() << " disk size: " << printAsRAM(_diskCacheSize);
#endif
            ///We switched from DISK to RAM that means the MemoryFile object has been created and the file opened
            appPTR->increaseNCacheFilesOpened();
        } else {
            if (newStorage == eStorageModeRAM) {
                _memoryCacheSize += size;
            } else if (newStorage == eStorageModeDisk) {
                _diskCacheSize += size;
            }
        }

        _signalEmitter->emitEntryStorageChanged(time, (int)oldStorage, (int)newStorage);
    }

    virtual void backingFileClosed() const OVERRIDE FINAL
    {
        assert(!_isTiled);
        appPTR->decreaseNCacheFilesOpened();
    }

    // const data member: no need to take the lock
    const std::string & cacheName() const
    {
        return _cacheName;
    }

    /*Returns the cache version as a string. This is
         used to know whether a cache is still valid when
         restoring*/
    // const data member: no need to take the lock
    unsigned int cacheVersion() const
    {
        return _version;
    }

    /*Returns the name of the cache with its path preprended*/
    QString getCachePath() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        QString cacheFolderName( appPTR->getDiskCacheLocation() );
        StrUtils::ensureLastPathSeparator(cacheFolderName);

        cacheFolderName.append( QString::fromUtf8( cacheName().c_str() ) );

        return cacheFolderName;
    }

    std::string getRestoreFilePath() const
    {
        QString newCachePath( getCachePath() );
        StrUtils::ensureLastPathSeparator(newCachePath);

        newCachePath.append( QString::fromUtf8("restoreFile." NATRON_CACHE_FILE_EXT) );

        return newCachePath.toStdString();
    }

    void setMaximumCacheSize(U64 newSize)
    {
        QMutexLocker k(&_sizeLock);

        _maximumCacheSize = newSize;
    }

    void setMaximumInMemorySize(double percentage)
    {
        QMutexLocker k(&_sizeLock);

        _maximumInMemorySize = _maximumCacheSize * percentage;
    }

    std::size_t getMaximumSize() const
    {
        QMutexLocker k(&_sizeLock);

        return _maximumCacheSize;
    }

    std::size_t getMaximumMemorySize() const
    {
        QMutexLocker k(&_sizeLock);

        return _maximumInMemorySize;
    }

    std::size_t getMemoryCacheSize() const
    {
        QMutexLocker k(&_sizeLock);

        return _memoryCacheSize;
    }

    std::size_t getDiskCacheSize() const
    {
        QMutexLocker k(&_sizeLock);

        return _diskCacheSize;
    }

    CacheSignalEmitterPtr activateSignalEmitter() const
    {
        return _signalEmitter;
    }

    /** @brief This function can be called to remove a specific entry from the cache. For example a frame
     * that has had its render aborted but already belong to the cache.
     **/
    void removeEntry(EntryTypePtr entry)
    {
        ///early return if entry is NULL
        if (!entry) {
            return;
        }
        std::list<EntryTypePtr> toRemove;

        {
            QMutexLocker l(&_lock);
            CacheIterator existingEntry = _memoryCache( entry->getHashKey() );
            if ( existingEntry != _memoryCache.end() ) {
                std::list<EntryTypePtr> & ret = getValueFromIterator(existingEntry);
                for (typename std::list<EntryTypePtr>::iterator it = ret.begin(); it != ret.end(); ++it) {
                    if ( (*it)->getKey() == entry->getKey() ) {
                        toRemove.push_back(*it);
                        ret.erase(it);
                        break;
                    }
                }
                if ( ret.empty() ) {
                    _memoryCache.erase(existingEntry);
                }
            } else {
                existingEntry = _diskCache( entry->getHashKey() );
                if ( existingEntry != _diskCache.end() ) {
                    std::list<EntryTypePtr> & ret = getValueFromIterator(existingEntry);
                    for (typename std::list<EntryTypePtr>::iterator it = ret.begin(); it != ret.end(); ++it) {
                        if ( (*it)->getKey() == entry->getKey() ) {
                            toRemove.push_back(*it);
                            ret.erase(it);
                            break;
                        }
                    }
                    if ( ret.empty() ) {
                        _diskCache.erase(existingEntry);
                    }
                }
            }
        } // QMutexLocker l(&_lock);
        if ( !toRemove.empty() ) {
            _deleterThread.appendToQueue(toRemove);

            ///Clearing the list here will not delete the objects pointing to by the shared_ptr's because we made a copy
            ///that the separate thread will delete
            toRemove.clear();
        }
    } // removeEntry

    void removeEntry(U64 hash)
    {
        std::list<EntryTypePtr> toRemove;
        {
            QMutexLocker l(&_lock);
            CacheIterator existingEntry = _memoryCache( hash);
            if ( existingEntry != _memoryCache.end() ) {
                std::list<EntryTypePtr> & ret = getValueFromIterator(existingEntry);
                for (typename std::list<EntryTypePtr>::iterator it = ret.begin(); it != ret.end(); ++it) {
                    toRemove.push_back(*it);
                }
                _memoryCache.erase(existingEntry);
            } else {
                existingEntry = _diskCache( hash );
                if ( existingEntry != _diskCache.end() ) {
                    std::list<EntryTypePtr> & ret = getValueFromIterator(existingEntry);
                    for (typename std::list<EntryTypePtr>::iterator it = ret.begin(); it != ret.end(); ++it) {
                        toRemove.push_back(*it);
                    }
                    _diskCache.erase(existingEntry);
                }
            }
        } // QMutexLocker l(&_lock);

        if ( !toRemove.empty() ) {
            _deleterThread.appendToQueue(toRemove);

            ///Clearing the list here will not delete the objects pointing to by the shared_ptr's because we made a copy
            ///that the separate thread will delete
            toRemove.clear();
        }
    }

    /*Saves cache to disk as a settings file.
     */
    void save(CacheTOC* tableOfContents);


    /*Restores the cache from disk.*/
    void restore(const CacheTOC & tableOfContents);


    void removeAllEntriesWithDifferentNodeHashForHolderPublic(const CacheEntryHolder* holder,
                                                              U64 nodeHash)
    {
        _cleanerThread.appendToQueue(holder->getCacheID(), nodeHash, false);
    }

    void removeAllEntriesForHolderPublic(const CacheEntryHolder* holder,
                                         bool blocking)
    {
        if (blocking) {
            removeAllEntriesWithDifferentNodeHashForHolderPrivate(holder->getCacheID(), 0, true);
        } else {
            _cleanerThread.appendToQueue(holder->getCacheID(), 0, true);
        }
    }

    void getMemoryStatsForCacheEntryHolder(const CacheEntryHolder* holder,
                                           std::size_t* ramOccupied,
                                           std::size_t* diskOccupied) const
    {
        *ramOccupied = 0;
        *diskOccupied = 0;

        std::string holderID = holder->getCacheID();
        QMutexLocker locker(&_lock);

        for (ConstCacheIterator memIt = _memoryCache.begin(); memIt != _memoryCache.end(); ++memIt) {
            const std::list<EntryTypePtr> & entries = getValueFromIterator(memIt);
            if ( !entries.empty() ) {
                const EntryTypePtr & front = entries.front();

                if (front->getKey().getCacheHolderID() == holderID) {
                    for (typename std::list<EntryTypePtr>::const_iterator it = entries.begin(); it != entries.end(); ++it) {
                        *ramOccupied += (*it)->size();
                    }
                }
            }
        }

        for (ConstCacheIterator memIt = _diskCache.begin(); memIt != _diskCache.end(); ++memIt) {
            const std::list<EntryTypePtr> & entries = getValueFromIterator(memIt);
            if ( !entries.empty() ) {
                const EntryTypePtr & front = entries.front();

                if (front->getKey().getCacheHolderID() == holderID) {
                    for (typename std::list<EntryTypePtr>::const_iterator it = entries.begin(); it != entries.end(); ++it) {
                        *diskOccupied += (*it)->size();
                    }
                }
            }
        }
    }

private:

    virtual void removeAllEntriesWithDifferentNodeHashForHolderPrivate(const std::string & holderID,
                                                                       U64 nodeHash,
                                                                       bool removeAll) OVERRIDE FINAL
    {
        std::list<EntryTypePtr> toDelete;
        CacheContainer newMemCache, newDiskCache;
        {
            QMutexLocker locker(&_lock);

            for (ConstCacheIterator memIt = _memoryCache.begin(); memIt != _memoryCache.end(); ++memIt) {
                const std::list<EntryTypePtr> & entries = getValueFromIterator(memIt);
                if ( !entries.empty() ) {
                    const EntryTypePtr & front = entries.front();

                    if ( (front->getKey().getCacheHolderID() == holderID) &&
                         ( ( front->getKey().getTreeVersion() != nodeHash) || removeAll ) ) {
                        for (typename std::list<EntryTypePtr>::const_iterator it = entries.begin(); it != entries.end(); ++it) {
                            toDelete.push_back(*it);
                        }
                    } else {
                        typename EntryType::hash_type hash = front->getHashKey();
                        newMemCache.insert(hash, entries);
                    }
                }
            }

            for (ConstCacheIterator dIt = _diskCache.begin(); dIt != _diskCache.end(); ++dIt) {
                const std::list<EntryTypePtr> & entries = getValueFromIterator(dIt);
                if ( !entries.empty() ) {
                    const EntryTypePtr & front = entries.front();

                    if ( (front->getKey().getCacheHolderID() == holderID) &&
                         ( ( front->getKey().getTreeVersion() != nodeHash) || removeAll ) ) {
                        for (typename std::list<EntryTypePtr>::const_iterator it = entries.begin(); it != entries.end(); ++it) {
                            toDelete.push_back(*it);
                        }
                    } else {
                        typename EntryType::hash_type hash = front->getHashKey();
                        newDiskCache.insert(hash, entries);
                    }
                }
            }

            _memoryCache = newMemCache;
            _diskCache = newDiskCache;
        } // QMutexLocker locker(&_lock);

        if ( !toDelete.empty() ) {
            _deleterThread.appendToQueue(toDelete);

            ///Clearing the list here will not delete the objects pointing to by the shared_ptr's because we made a copy
            ///that the separate thread will delete
            toDelete.clear();
        }
    } // removeAllEntriesWithDifferentNodeHashForHolderPrivate

    bool getInternal(const typename EntryType::key_type & key,
                     std::list<EntryTypePtr>* returnValue) const
    {
        ///Private should be locked
        assert( !_lock.tryLock() );

        ///find a matching value in the internal memory container
        CacheIterator memoryCached = _memoryCache( key.getHash() );

        if ( memoryCached != _memoryCache.end() ) {
            ///we found something with a matching hash key. There may be several entries linked to
            ///this key, we need to find one with matching params
            std::list<EntryTypePtr> & ret = getValueFromIterator(memoryCached);
            for (typename std::list<EntryTypePtr>::const_iterator it = ret.begin(); it != ret.end(); ++it) {
                if ( (*it)->getKey() == key ) {
                    returnValue->push_back(*it);

                    ///Q_EMIT the added signal otherwise when first reading something that's already cached
                    ///the timeline wouldn't update
                    if (_signalEmitter) {
                        _signalEmitter->emitAddedEntry( key.getTime() );
                    }
                }
            }

            return returnValue->size() > 0;
        } else {
            ///fallback on the disk cache internal container
            CacheIterator diskCached = _diskCache( key.getHash() );

            if ( diskCached == _diskCache.end() ) {
                /*the entry was neither in memory or disk, just allocate a new one*/
                return false;
            } else {
                /*we found something with a matching hash key. There may be several entries linked to
                   this key, we need to find one with matching values(operator ==)*/
                std::list<EntryTypePtr> & ret = getValueFromIterator(diskCached);

                for (typename std::list<EntryTypePtr>::iterator it = ret.begin();
                     it != ret.end(); ++it) {
                    if ( (*it)->getKey() == key ) {


                        /*If we found 1 entry in the list that has exactly the same key params,
                         we re-open the mapping to the RAM put the entry
                         back into the memoryCache.*/
                        if (!_isTiled) {
                            try {
                                (*it)->reOpenFileMapping();
                            } catch (const std::exception & e) {
                                qDebug() << "Error while reopening cache file: " << e.what();
                                ret.erase(it);

                                return false;
                            } catch (...) {
                                qDebug() << "Error while reopening cache file";
                                ret.erase(it);

                                return false;
                            }

                            //put it back into the RAM
                            _memoryCache.insert( (*it)->getHashKey(), *it );


                            U64 memoryCacheSize, maximumInMemorySize;
                            {
                                QMutexLocker k(&_sizeLock);
                                memoryCacheSize = _memoryCacheSize;
                                maximumInMemorySize = _maximumInMemorySize;
                            }
                            std::list<EntryTypePtr> entriesToBeDeleted;

                            //now clear extra entries from the disk cache so it doesn't exceed the RAM limit.
                            while (memoryCacheSize > maximumInMemorySize) {
                                if ( !tryEvictInMemoryEntry(entriesToBeDeleted) ) {
                                    break;
                                }

                                {
                                    QMutexLocker k(&_sizeLock);
                                    memoryCacheSize = _memoryCacheSize;
                                    maximumInMemorySize = _maximumInMemorySize;
                                }
                            }
                        }
                        
                        returnValue->push_back(*it);
                        ///Q_EMIT the added signal otherwise when first reading something that's already cached
                        ///the timeline wouldn't update
                        if (_signalEmitter) {
                            _signalEmitter->emitAddedEntry( key.getTime() );
                        }

                        if (!_isTiled) {

                            ret.erase(it);

                            ///Remove it from the disk cache
                            _diskCache.erase(diskCached);
                        }

                        return true;
                    }
                }

                /*if we reache here it means no entries linked to the hash key matches the params,then
                   we allocate a new one*/
                return false;
            }
        }
    } // getInternal

    /** @brief Inserts into the cache an entry that was previously allocated by the createInternal()
     * function. This is called directly by createInternal() if the allocation was successful
     **/
    void sealEntry(const EntryTypePtr & entry,
                   bool inMemory) const
    {
        assert( !_lock.tryLock() );   // must be locked
        typename EntryType::hash_type hash = entry->getHashKey();

        if (inMemory) {
            /*if the entry doesn't exist on the memory cache,make a new list and insert it*/
            CacheIterator existingEntry = _memoryCache(hash);
            if ( existingEntry == _memoryCache.end() ) {
                _memoryCache.insert(hash, entry);
            } else {
                /*append to the existing list*/
                getValueFromIterator(existingEntry).push_back(entry);
            }
        } else {
            CacheIterator existingEntry = _diskCache(hash);
            if ( existingEntry == _diskCache.end() ) {
                _diskCache.insert(hash, entry);
            } else {
                /*append to the existing list*/
                getValueFromIterator(existingEntry).push_back(entry);
            }
        }
    }

    bool tryEvictInMemoryEntry(std::list<EntryTypePtr> & entriesToBeDeleted) const
    {
        assert( !_lock.tryLock() );
        std::pair<hash_type, EntryTypePtr> evicted = _memoryCache.evict();
        //if the cache couldn't evict that means all entries are used somewhere and we shall not remove them!
        //we'll let the user of these entries purge the extra entries left in the cache later on
        if (!evicted.second) {
            return false;
        }

        // If it is stored on disk, remove it from memory
        // If the cache is tiled, the entry is sharing the same file with other entries so we cannot close the file.
        // Just deallocate it
        if ( !evicted.second->isStoredOnDisk()) {
            entriesToBeDeleted.push_back(evicted.second);
        } else {

            assert( evicted.second.unique() );

            ///This is EXPENSIVE! it calls msync
            evicted.second->deallocate();

            /*insert it back into the disk portion */

            U64 diskCacheSize, maximumCacheSize, maximumInMemorySize;
            {
                QMutexLocker k(&_sizeLock);
                diskCacheSize = _diskCacheSize;
                maximumInMemorySize = _maximumInMemorySize;
                maximumCacheSize = _maximumCacheSize;
            }

            /*before that we need to clear the disk cache if it exceeds the maximum size allowed*/
            while ( ( diskCacheSize  + evicted.second->size() ) >= (maximumCacheSize - maximumInMemorySize) ) {
                std::pair<hash_type, EntryTypePtr> evictedFromDisk = _diskCache.evict();
                //if the cache couldn't evict that means all entries are used somewhere and we shall not remove them!
                //we'll let the user of these entries purge the extra entries left in the cache later on
                if (!evictedFromDisk.second) {
                    break;
                }

                ///Erase the file from the disk if we reach the limit.
                evictedFromDisk.second->removeAnyBackingFile();

                entriesToBeDeleted.push_back(evictedFromDisk.second);

                {
                    QMutexLocker k(&_sizeLock);
                    maximumInMemorySize = _maximumInMemorySize;
                    maximumCacheSize = _maximumCacheSize;
                }

                //The entry is not yet deleted for real since it's done in a separate thread when this function
                ///size() will return 0 at this point, we have to recompute it
                std::size_t fsize = evictedFromDisk.second->getElementsCountFromParams();
                diskCacheSize -= fsize;
            }

            CacheIterator existingDiskCacheEntry = _diskCache(evicted.first);
            /*if the entry doesn't exist on the disk cache,make a new list and insert it*/
            if ( existingDiskCacheEntry == _diskCache.end() ) {
                _diskCache.insert(evicted.first, evicted.second);
            } else {   /*append to the existing list*/
                getValueFromIterator(existingDiskCacheEntry).push_back(evicted.second);
            }
        } // if (!evicted.second->isStoredOnDisk())

        return true;
    } // tryEvictEntry

    bool tryEvictDiskEntry(std::list<EntryTypePtr> & entriesToBeDeleted) const
    {

        assert( !_lock.tryLock() );
        std::pair<hash_type, EntryTypePtr> evicted = _diskCache.evict();
        //if the cache couldn't evict that means all entries are used somewhere and we shall not remove them!
        //we'll let the user of these entries purge the extra entries left in the cache later on
        if (!evicted.second) {
            return false;
        }
        if (!_isTiled) {
            // Erase the file from the disk if we reach the limit.
            evicted.second->removeAnyBackingFile();
        }
        entriesToBeDeleted.push_back(evicted.second);
        return true;
    }

};

NATRON_NAMESPACE_EXIT


#endif /*NATRON_ENGINE_ABSTRACTCACHE_H_ */
