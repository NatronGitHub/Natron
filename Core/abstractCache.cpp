//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com



#include "abstractCache.h"
#include "Superviser/powiterFn.h"
#include "Core/mappedfile.h"
#include <QtCore/QDir>
#include <QtCore/QTextStream>
#include <sstream>
using namespace std;


MemoryMappedEntry::MemoryMappedEntry():_mappedFile(0){
    
}
void MemoryMappedEntry::allocate(U64 byteCount,const char* path){
    _mappedFile = new MemoryFile(path,Powiter_Enums::if_exists_truncate_if_not_exists_create);
    _mappedFile->resize(byteCount);
}
void MemoryMappedEntry::deallocate(){
    if(_mappedFile){
        delete _mappedFile;
    }
    _mappedFile = 0;
}
MemoryMappedEntry::~MemoryMappedEntry(){
    deallocate();
}

InMemoryEntry::InMemoryEntry():_data(0){
    
}
void InMemoryEntry::allocate(U64 byteCount,const char* path){
    _data = (char*)malloc(byteCount);
}
void InMemoryEntry::deallocate(){
    free(_data);
    _data = 0;
}
InMemoryEntry::~InMemoryEntry(){
    
}



AbstractCache::AbstractCache():_size(0),_maximumCacheSize(0){
    
}
AbstractCache::~AbstractCache(){
    clear();
}

void AbstractCache::clear(){
    QReadLocker guard(&_lock);
    _cache.clear();
    _size = 0;
}

/*Returns an iterator to the cache. If found it points
 to a valid cache entry, otherwise it points to to end.*/
AbstractCache::CacheIterator AbstractCache::isCached(const U64& key)  {
    QReadLocker guard(&_lock);
    return _cache(key);
}


bool AbstractCache::add(U64 key,CacheEntry* entry){
    QReadLocker guard(&_lock);
    bool evict = false;
    if (_size >= _maximumCacheSize) {
        evict = true;
    }
    _size += entry->size();
    std::pair<U64,CacheEntry*> evicted = _cache.insert(key,entry,evict);
    if(evicted.second){
        _size -= evicted.second->size();
        
        /*if it is a memorymapped entry, remove the backing file in the meantime*/
        MemoryMappedEntry* mmEntry = dynamic_cast<MemoryMappedEntry*>(evicted.second);
        if(mmEntry){
            std::string path = mmEntry->getMappedFile()->path();
            QFile::remove(path.c_str());
        }
        delete evicted.second;
    }
    return evict;
}

bool AbstractMemoryCache::add(U64 key,CacheEntry* entry){
    assert(dynamic_cast<InMemoryEntry*>(entry));
    return AbstractCache::add(key, entry);
}

AbstractDiskCache::AbstractDiskCache(double inMemoryUsage):_inMemorySize(0){
    _maximumInMemorySize = inMemoryUsage;
}

bool AbstractDiskCache::add(U64 key,CacheEntry* entry){
    MemoryMappedEntry* mmEntry = dynamic_cast<MemoryMappedEntry*>(entry);
    assert(mmEntry);
    bool mustEvictFromMemory = false;
    QReadLocker guard(&_lock);
    if(_inMemorySize > _maximumInMemorySize*getMaximumSize()){
        mustEvictFromMemory = true;
    }
    _inMemorySize += mmEntry->size();
    std::pair<U64,CacheEntry*> evicted = _inMemoryPortion.insert(key, mmEntry, mustEvictFromMemory);
    if(evicted.second){
        /*switch the evicted entry from memory to the disk.*/
        mmEntry->deallocate();
        _inMemorySize -= mmEntry->size();
        return AbstractCache::add(evicted.first, evicted.second);
    }else{
        return AbstractCache::add(key, mmEntry);
    }
}

AbstractCache::CacheIterator AbstractDiskCache::isInMemory(const U64 &key) {
    QReadLocker guard(&_lock);
    return _inMemoryPortion(key);
}

void AbstractDiskCache::initializeSubDirectories(){
    QDir root(ROOT);
    if(root.exists()){
        QStringList entries = root.entryList();
        bool foundDir = false;
        for(int i =0 ; i < entries.size();i++){
            if(entries[i].toStdString() == "Cache"){
                foundDir=true;
                break;
            }
        }
        if(!foundDir){
            root.mkdir("Cache");
            root.setCurrent(ROOT"Cache");
        }
    }
    QDir cache(CACHE_ROOT_PATH);
    QStringList entries = cache.entryList();
    bool foundDir = false;
    for(int i =0 ; i < entries.size();i++){
        if(entries[i].toStdString() == cacheName()){
            foundDir=true;
            break;
        }
    }
    if(!foundDir){
        cache.mkdir(cacheName().c_str());
        QString newCachePath(CACHE_ROOT_PATH);
        newCachePath.append(cacheName().c_str());
        QDir newCache(newCachePath);
        for(U32 i = 0x00 ; i <= 0xF; i++){
            for(U32 j = 0x00 ; j <= 0xF ; j++){
                ostringstream oss;
                oss << hex <<  i;
                oss << hex << j ;
                string str = oss.str();
                newCache.mkdir(str.c_str());                
            }
        }
    }else{
        QString newCachePath(CACHE_ROOT_PATH);
        newCachePath.append(cacheName().c_str());
        QDir newCache(newCachePath);
        QStringList etr = newCache.entryList();
        // if not 256 subdirs, we re-create the cache
        if (etr.size() < 256) {
            foreach(QString e, etr){
                newCache.rmdir(e);
            }
        }
        for(U32 i = 0x00 ; i <= 0xF; i++){
            for(U32 j = 0x00 ; j <= 0xF ; j++){
                ostringstream oss;
                oss << hex <<  i;
                oss << hex << j ;
                string str = oss.str();
                newCache.mkdir(str.c_str());
            }
        }
    }
}
AbstractDiskCache::~AbstractDiskCache(){
}

void AbstractDiskCache::clearInMemoryCache(){
    QReadLocker guard(&_lock);
    _inMemorySize = 0;
    for (CacheIterator it = beginMemoryCache(); it!=endMemoryCache(); it++) {
        MemoryMappedEntry* mmEntry = dynamic_cast<MemoryMappedEntry*>(AbstractCache::getValueFromIterator(it));
        assert(mmEntry);
        mmEntry->deallocate();
        AbstractCache::add(it->first, it->second);
    }
}

void AbstractDiskCache::save(){
    QString newCachePath(CACHE_ROOT_PATH);
    newCachePath.append(cacheName().c_str());
    newCachePath.append("/restoreFile.powc");
    QFile _restoreFile(newCachePath);
    _restoreFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate);
    QTextStream out(&_restoreFile);
    /*clearing in-memory cache so only the on-disk portion has entries left.*/
    clearInMemoryCache();
    
    for(CacheIterator it = begin(); it!= end() ; it++){
        MemoryMappedEntry* mmEntry = dynamic_cast<MemoryMappedEntry*>(AbstractCache::getValueFromIterator(it));
        assert(mmEntry);
        out << mmEntry->printOut().c_str() << endl;
    }
    _restoreFile.close();
}

void AbstractDiskCache::restore(){
    QString newCachePath(CACHE_ROOT_PATH);
    newCachePath.append(cacheName().c_str());
    QString settingsFilePath(newCachePath);
    settingsFilePath.append("/restoreFile.powc");
    if(QFile::exists(settingsFilePath)){
        QFile _restoreFile(settingsFilePath);
        _restoreFile.open(QIODevice::ReadWrite);
        QTextStream in(&_restoreFile);
        QString line = in.readLine();
        if(line.contains(QString(cacheVersion().c_str()))){
            line = in.readLine();
            while(!line.isNull()){
                
                std::pair<U64,MemoryMappedEntry*> entry = recoverEntryFromString(line);
                if(entry.second){
                    add(entry.first,entry.second);
                }
                line = in.readLine();
            }
            
            /*now that we're done using it, clear it*/
            _restoreFile.close();
            QFile::remove(settingsFilePath);
            QFile newFile(settingsFilePath);
            newFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate);
            QTextStream outsetti(&newFile);
            outsetti << cacheVersion().c_str() << endl;
            newFile.close();

        }else{
            /*re-create the cache*/
            _restoreFile.resize(0);
            QDir directory(newCachePath);
            QStringList files = directory.entryList();
            for(int i =0 ; i< files.size() ;i++){
                directory.remove(files[i]);
            }
            initializeSubDirectories();
            /*re-create settings file*/
            QFile _restoreFile(settingsFilePath);
            _restoreFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate);
            QTextStream outsetti(&_restoreFile);
            outsetti << cacheVersion().c_str() << endl;
            _restoreFile.close();
        }
    }else{
        /*re-create cache*/
        QDir directory(newCachePath);
        QStringList files = directory.entryList();
        for(int i =0 ; i< files.size() ;i++){
            directory.remove(files[i]);
        }
        initializeSubDirectories();
        /*re-create settings file*/
        QFile _restoreFile(settingsFilePath);
        _restoreFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate);
        QTextStream outsetti(&_restoreFile);
        outsetti << cacheVersion().c_str() << endl;
        _restoreFile.close();
    }
}

