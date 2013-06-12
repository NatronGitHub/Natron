//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com



#include "abstractCache.h"
#include "Superviser/powiterFn.h"
#include "Core/mappedfile.h"
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <sstream>
using namespace std;


MemoryMappedEntry::MemoryMappedEntry():_mappedFile(0){
    
}
bool MemoryMappedEntry::allocate(U64 byteCount,const char* path){
#ifdef PW_DEBUG
    if(QFile::exists(path)){
        cout << "WARNING: A file with the same name already exist : " << path << endl;
    }
#endif
    try{
        _mappedFile = new MemoryFile(path,Powiter_Enums::if_exists_keep_if_dont_exists_create);
    }catch(const char* str){
        cout << str << endl;
        deallocate();
        if(QFile::exists(path)){
            QFile::remove(path);
        }
        return false;
    }
    _mappedFile->resize(byteCount);
    _size += _mappedFile->size();
    string newPath(path);
    if(_path.empty()) _path.append(path);
    else if(_path != newPath) _path = newPath;
    return true;
}
void MemoryMappedEntry::deallocate(){
    if(_mappedFile){
        delete _mappedFile;
    }
    _mappedFile = 0;
}

bool MemoryMappedEntry::reOpen(){
    if(_path.empty() || !_mappedFile) return false;
    try{
        _mappedFile = new MemoryFile(_path.c_str(),Powiter_Enums::if_exists_keep_if_dont_exists_fail);
    }catch(const char* str){
        deallocate();
        if(QFile::exists(_path.c_str())){
            QFile::remove(_path.c_str());
        }
        cout << str << endl;
        return false;
    }
    return true;
}

MemoryMappedEntry::~MemoryMappedEntry(){
    deallocate();
    
}

InMemoryEntry::InMemoryEntry():_data(0){
    
}
bool InMemoryEntry::allocate(U64 byteCount,const char* path){
    _data = (char*)malloc(byteCount);
    if(!_data) return false;
    return true;
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
    QWriteLocker guard(&_cache._rwLock);
    for(CacheIterator it = _cache.begin() ; it!=_cache.end() ; it++){
        delete it->second;
    }
    _cache.clear();
    _size = 0;
}

/*Returns an iterator to the cache. If found it points
 to a valid cache entry, otherwise it points to to end.*/
AbstractCache::CacheIterator AbstractCache::isCached(const U64& key)  {
    return _cache(key);
}


bool AbstractCache::add(U64 key,CacheEntry* entry){
    bool evict = false;
    {
        QWriteLocker guard(&_cache._rwLock);
        if (_size >= _maximumCacheSize) {
            evict = true;
        }
        _size += entry->size();
    }
    std::pair<U64,CacheEntry*> evicted = _cache.insert(key,entry,evict);
    if(evicted.second){
        {
            QWriteLocker guard(&_cache._rwLock);
            _size -= evicted.second->size();
        }
        /*if it is a memorymapped entry, remove the backing file in the meantime*/
        MemoryMappedEntry* mmEntry = static_cast<MemoryMappedEntry*>(evicted.second);
        if(mmEntry){
            delete evicted.second;
            std::string path = mmEntry->getMappedFile()->path();
            QFile::remove(path.c_str());
        }
    }
    return evict;
}

bool AbstractMemoryCache::add(U64 key,CacheEntry* entry){
    assert(static_cast<InMemoryEntry*>(entry));
    return AbstractCache::add(key, entry);
}

AbstractDiskCache::AbstractDiskCache(double inMemoryUsage):_inMemorySize(0){
    _maximumInMemorySize = inMemoryUsage;
}

bool AbstractDiskCache::add(U64 key,CacheEntry* entry){
    MemoryMappedEntry* mmEntry = static_cast<MemoryMappedEntry*>(entry);
    assert(mmEntry);
    bool mustEvictFromMemory = false;
    {
        QWriteLocker guard(&_cache._rwLock);
        if(_inMemorySize > _maximumInMemorySize*getMaximumSize()){
            mustEvictFromMemory = true;
        }
        _inMemorySize += mmEntry->size();
    }
    std::pair<U64,CacheEntry*> evicted = _inMemoryPortion.insert(key, mmEntry, mustEvictFromMemory);
    if(evicted.second){
        /*switch the evicted entry from memory to the disk.*/
        mmEntry->deallocate();
        {
            QWriteLocker guard(&_cache._rwLock);
            _inMemorySize -= mmEntry->size();
        }
        return AbstractCache::add(evicted.first, evicted.second);
    }
    return true;
}

AbstractCache::CacheIterator AbstractDiskCache::isInMemory(const U64 &key) {
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
    _inMemorySize = 0;
    while(_inMemoryPortion.size() > 0){
        std::pair<U64,CacheEntry*> evicted = _inMemoryPortion.evict();
        MemoryMappedEntry* mmEntry = static_cast<MemoryMappedEntry*>(evicted.second);
        assert(mmEntry);
        mmEntry->deallocate();
        AbstractCache::add(evicted.first, evicted.second);
    }
}

void AbstractDiskCache::clearDiskCache(){
    clearInMemoryCache();
    clear();
}

void AbstractDiskCache::save(){
    QString newCachePath(CACHE_ROOT_PATH);
    newCachePath.append(cacheName().c_str());
    newCachePath.append("/restoreFile.powc");
    QFile _restoreFile(newCachePath);
    _restoreFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append);
    QTextStream out(&_restoreFile);
    /*clearing in-memory cache so only the on-disk portion has entries left.*/
    clearInMemoryCache();
    
    for(CacheIterator it = begin(); it!= end() ; it++){
        MemoryMappedEntry* mmEntry = static_cast<MemoryMappedEntry*>(AbstractCache::getValueFromIterator(it));
        assert(mmEntry);
        out << mmEntry->printOut().c_str() << endl;
    }
    _restoreFile.close();
    
    clear();
}

void AbstractDiskCache::restore(){
    QString newCachePath(CACHE_ROOT_PATH);
    newCachePath.append(cacheName().c_str());
    QString settingsFilePath(newCachePath);
    settingsFilePath.append("/restoreFile.powc");
    if(QFile::exists(settingsFilePath)){
        QDir directory(newCachePath);
        QStringList files = directory.entryList();
        
        
        /*Now counting actual data files in the cache*/
        /*check if there's 256 subfolders, otherwise reset cache.*/
        int count = 0; // -1 because of the restoreFile
        int subFolderCount = 0;
        for(int i =0 ; i< files.size() ;i++){
            QString subFolder(newCachePath);
            subFolder.append("/");
            subFolder.append(files[i]);
            if(subFolder.right(1) == QString(".") || subFolder.right(2) == QString("..")) continue;
            QDir d(subFolder);
            if(d.exists()){
                subFolderCount++;
                QStringList items = d.entryList();
                for(int j = 0 ; j < items.size();j++){
                    if(items[j] != QString(".") && items[j] != QString("..")) count++;
                }
            }
        }
        if(subFolderCount<256){
            cout << cacheName() << " doesn't contain sub-folders indexed from 00 to FF. Reseting." << endl;
            cleanUpDiskAndReset();
        }
        
        QFile _restoreFile(settingsFilePath);
        _restoreFile.open(QIODevice::ReadWrite);
        QTextStream in(&_restoreFile);
        
        
        QString line = in.readLine();
        std::vector<std::pair<U64,MemoryMappedEntry*> > entries;
        if(line.contains(QString(cacheVersion().c_str()))){
            line = in.readLine();
            while(!line.isNull()){
                if (line.isEmpty()) {
                    line = in.readLine();
                    continue;
                }
                std::pair<U64,MemoryMappedEntry*> entry = recoverEntryFromString(line);
                if(entry.second){
                    entries.push_back(entry);
                }else{
                    cout <<"WARNING: " <<  cacheName() << " failed to recover entry, discarding it." << endl;
                }
                line = in.readLine();
            }
            _restoreFile.close();
            if (count == entries.size()) {
                for (U32 i = 0; i < entries.size(); i++) {
                    std::pair<U64,MemoryMappedEntry*>& entry = entries[i];
                    add(entry.first,entry.second);
                }
                
                /*cache is filled , debug*/
                debug();
                
            }else{
                cout << cacheName() << ":The entries count in the restore file does not equal the number of actual data files. Reseting." << endl;
                cleanUpDiskAndReset();
            }
            
            /*now that we're done using it, clear it*/
            QFile::remove(settingsFilePath);
            QFile newFile(settingsFilePath);
            newFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate);
            QTextStream outsetti(&newFile);
            outsetti << cacheVersion().c_str() << endl;
            newFile.close();

        }else{ /*cache version is different*/
            /*re-create the cache*/
            _restoreFile.resize(0);
            cleanUpDiskAndReset();
            /*re-create settings file*/
            QFile _restoreFile(settingsFilePath);
            _restoreFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate);
            QTextStream outsetti(&_restoreFile);
            outsetti << cacheVersion().c_str() << endl;
            _restoreFile.close();
        }
    }else{ // restore file doesn't exist
        /*re-create cache*/
        QDir root(ROOT);
        QStringList rootEntries = root.entryList();
        bool foundCache = false;
        for (int i =0 ; i< rootEntries.size(); i++) {
            if (rootEntries[i] == QString("Cache")) {
                foundCache = true;
                break;
            }
        }
        if(!foundCache){
            root.mkdir("Cache");
        }
        QDir cacheDir(CACHE_ROOT_PATH);
        QStringList cacheRootEntries = root.entryList();
        bool foundCacheName = false;
        for (int i =0 ; i< cacheRootEntries.size(); i++) {
            if (cacheRootEntries[i] == QString(cacheName().c_str())) {
                foundCacheName = true;
                break;
            }
        }
        if(!foundCacheName){
            cacheDir.mkdir(cacheName().c_str());
        }
        
        cleanUpDiskAndReset();
        
        /*re-create settings file*/
        QFile _restoreFile(settingsFilePath);
        _restoreFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate);
        QTextStream outsetti(&_restoreFile);
        outsetti << cacheVersion().c_str() << endl;
        _restoreFile.close();
    }
}

void AbstractDiskCache::cleanUpDiskAndReset(){
    QString cachePath(CACHE_ROOT_PATH);
    cachePath.append(cacheName().c_str());
    QDir dir(cachePath);
    if(dir.exists()){
        dir.removeRecursively();
    }
    initializeSubDirectories();
    
}

QString AbstractDiskCache::getCachePath(){
    QString str(CACHE_ROOT_PATH);
    str.append(cacheName().c_str());
    str.append("/");
    return str;
}

void AbstractDiskCache::debug(){
    cout << "====================DEBUGING: " << cacheName() << " =========" << endl;
    cout << "-------------------IN MEMORY ENTRIES-------------------" << endl;
    for (CacheIterator it = beginMemoryCache(); it!=endMemoryCache(); it++) {
        MemoryMappedEntry* entry = static_cast<MemoryMappedEntry*>(it->second);
        assert(entry);
        cout << "[" << entry->path() << "] = " << entry->size() << " bytes. ";
        if(entry->getMappedFile()->data()){
            cout << "Entry has a valid ptr." << endl;
        }else{
            cout << "Entry has a NULL ptr." << endl;
        }
    }
    cout <<" --------------------ON DISK ENTRIES---------------------" << endl;
    for (CacheIterator it = begin(); it!=end(); it++) {
        MemoryMappedEntry* entry = static_cast<MemoryMappedEntry*>(it->second);
        assert(entry);
        cout << "[" << entry->path() << "] = " << entry->size() << " bytes. ";
        if(entry->getMappedFile()){
            cout << "Entry is still allocated!!" << endl;
        }else{
            cout << "Entry is normally deallocated." << endl;
        }
    }
    cout << "Total entries count : " << _inMemoryPortion.size()+_cache.size() << endl;
    cout << "-===========END DEBUGGING: " << cacheName() << " ===========" << endl;
    
}

