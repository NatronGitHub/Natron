/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2022 The Natron developers
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

#ifndef Engine_CacheSerialization_h
#define Engine_CacheSerialization_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <list>
#include <set>
#include <cstddef>
#include <stdexcept>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
GCC_DIAG_OFF(unused-parameter)
// /opt/local/include/boost/serialization/smart_cast.hpp:254:25: warning: unused parameter 'u' [-Wunused-parameter]
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/list.hpp>
// /usr/local/include/boost/serialization/shared_ptr.hpp:112:5: warning: unused typedef 'boost_static_assert_typedef_112' [-Wunused-local-typedef]
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/split_member.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
GCC_DIAG_ON(unused-parameter)
#endif

#include "Engine/Cache.h"
#include "Engine/ImageSerialization.h"
#include "Engine/ImageParamsSerialization.h"
#include "Engine/FrameEntrySerialization.h"
#include "Engine/FrameParamsSerialization.h"
#include "Engine/EngineFwd.h"

// Note: these classes are used for cache serialization and do not have to maintain backward compatibility
#define SERIALIZED_ENTRY_INTRODUCES_SIZE 2
#define SERIALIZED_ENTRY_VERSION SERIALIZED_ENTRY_INTRODUCES_SIZE

//Beyond that percentage of occupation, the cache will start evicting LRU entries
#define NATRON_CACHE_LIMIT_PERCENT 0.9

///When defined, number of opened files, memory size and disk size of the cache are printed whenever there's activity.
//#define NATRON_DEBUG_CACHE

NATRON_NAMESPACE_ENTER

/*Saves cache to disk as a settings file.
 */
template<typename EntryType>
void
Cache<EntryType>::save(CacheTOC* tableOfContents)
{
    clearInMemoryPortion(false);
    {
        QMutexLocker l(&_lock);     // must be locked

        for (ConstCacheIterator it = _diskCache.begin(); it != _diskCache.end(); ++it) {
            const std::list<EntryTypePtr> & listOfValues  = getValueFromIterator(it);
            for (typename std::list<EntryTypePtr>::const_iterator it2 = listOfValues.begin(); it2 != listOfValues.end(); ++it2) {
                if ( (*it2)->isStoredOnDisk() ) {
                    SerializedEntry serialization;
                    serialization.hash = (*it2)->getHashKey();
                    serialization.params = (*it2)->getParams();
                    serialization.key = (*it2)->getKey();
                    serialization.size = (*it2)->dataSize();
                    serialization.filePath = (*it2)->getFilePath();
                    serialization.dataOffsetInFile = (*it2)->getOffsetInFile();

                    (*it2)->syncBackingFile();
                    
                    tableOfContents->push_back(serialization);
#ifdef DEBUG
                    if ( !_isTiled && !CacheAPI::checkFileNameMatchesHash(serialization.filePath, serialization.hash) ) {
                        qDebug() << "WARNING: Cache entry filename is not the same as the serialized hash key";
                    }
#endif
                }
            }
        }
    }
}

/*Restores the cache from disk.*/
template<typename EntryType>
void
Cache<EntryType>::restore(const CacheTOC & tableOfContents)
{
    ///Make sure the shared_ptrs live in this list and are destroyed not while under the lock
    ///so that the memory freeing (which might be expensive for large images) doesn't happen while under the lock
    std::list<EntryTypePtr> entriesToBeDeleted;

    std::set<QString> usedFilePaths;
    for (typename CacheTOC::const_iterator it =
             tableOfContents.begin(); it != tableOfContents.end(); ++it) {
        if ( it->hash != it->key.getHash() ) {
            /*
             * If this warning is printed this means that the value computed by it->key()
             * is different than the value stored prior to serialiazing this entry. In other words there're
             * 2 possibilities:
             * 1) The key has changed since it has been added to the cache: maybe you forgot to serialize some
             * members of the key or you didn't save them correctly.
             * 2) The hash key computation is unreliable and is depending upon changing or non-deterministic
             * parameters which is wrong.
             */
            qDebug() << "WARNING: serialized hash key different than the restored one";
        }

#ifdef DEBUG
        if ( !_isTiled && !checkFileNameMatchesHash(it->filePath, it->hash) ) {
            qDebug() << "WARNING: Cache entry filename is not the same as the serialized hash key";
        }
#endif

        EntryType* value = NULL;

        try {
            value = new EntryType(it->key, it->params, this);
            if (it->size != getTileSizeBytes()) {
                delete value;
                continue;
            }
            ///This will not put the entry back into RAM, instead we just insert back the entry into the disk cache
            value->restoreMetadataFromFile(it->size, it->filePath, it->dataOffsetInFile);
        } catch (const std::exception & e) {
            qDebug() << e.what();
            delete value;
            continue;
        }
        const std::string& filePath = value->getFilePath();
        usedFilePaths.insert(QString::fromUtf8(filePath.c_str()));
        {
            QMutexLocker locker(&_lock);
            sealEntry(EntryTypePtr(value), false /*inMemory*/);
        }
    }

    // Remove from the cache all files that are not referenced by the table of contents
    QString cachePath = getCachePath();
    if (isTileCache()) {
        QDir cacheFolder(cachePath);
        QString absolutePath = cacheFolder.absolutePath();
        QStringList etr = cacheFolder.entryList(QDir::NoDotAndDotDot);
        for (QStringList::iterator it = etr.begin(); it!=etr.end(); ++it) {
            QString entryFilePath = absolutePath + QLatin1Char('/') + *it;

            std::set<QString>::iterator foundUsed = usedFilePaths.find(entryFilePath);
            if (foundUsed == usedFilePaths.end()) {
                cacheFolder.remove(*it);
            }
        }
    } else {
        for (U32 i = 0x00; i <= 0xF; ++i) {
            for (U32 j = 0x00; j <= 0xF; ++j) {
                char str[3] = { '0', '0', 0 };
                str[0] += i;
                str[1] += j;

                QDir cacheFolder(cachePath + QLatin1Char('/') + QString::fromUtf8(str));
                QString absolutePath = cacheFolder.absolutePath();
                QStringList etr = cacheFolder.entryList(QDir::NoDotAndDotDot);
                for (QStringList::iterator it = etr.begin(); it!=etr.end(); ++it) {
                    QString entryFilePath = absolutePath + QLatin1Char('/') + *it;

                    std::set<QString>::iterator foundUsed = usedFilePaths.find(entryFilePath);
                    if (foundUsed == usedFilePaths.end()) {
                        cacheFolder.remove(*it);
                    }
                }
            }
        }

    }
}

template<typename EntryType>
struct Cache<EntryType>::SerializedEntry
{
    hash_type hash;
    typename EntryType::key_type key;
    ParamsTypePtr params;
    std::size_t size; //< the data size in bytes
    std::string filePath; //< we need to serialize it as several entries can have the same hash, hence we index them
    std::size_t dataOffsetInFile;

    SerializedEntry()
        : hash(0)
          , key()
          , params()
          , size(0)
          , filePath()
          , dataOffsetInFile(0)
    {
    }

    template<class Archive>
    void serialize(Archive & ar,
                   const unsigned int /*version*/)
    {
        ar & ::boost::serialization::make_nvp("Hash", hash);
        ar & ::boost::serialization::make_nvp("Key", key);
        ar & ::boost::serialization::make_nvp("Params", params);
        ar & ::boost::serialization::make_nvp("Size", size);
        ar & ::boost::serialization::make_nvp("Filename", filePath);
        ar & ::boost::serialization::make_nvp("Offset", dataOffsetInFile);
    }
};

NATRON_NAMESPACE_EXIT


#endif // Engine_CacheSerialization_h
