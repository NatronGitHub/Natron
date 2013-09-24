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

#include "AbstractCache.h"

#include <sstream>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QTextStream>
#if QT_VERSION < 0x050000
#include <QtGui/QDesktopServices>
#else
#include <QStandardPaths>
#endif

#include "Global/Macros.h"
#include "Engine/MemoryFile.h"

using namespace std;
using namespace Powiter;

#if QT_VERSION < 0x050000
static bool removeRecursively(const QString & dirName)
{
    bool result = false;
    QDir dir(dirName);
    
    if (dir.exists(dirName)) {
        Q_FOREACH(QFileInfo info, dir.entryInfoList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden  | QDir::AllDirs | QDir::Files, QDir::DirsFirst)) {
            if (info.isDir()) {
                result = removeRecursively(info.absoluteFilePath());
            }
            else {
                result = QFile::remove(info.absoluteFilePath());
            }
            
            if (!result) {
                return result;
            }
        }
        result = dir.rmdir(dirName);
    }
    return result;
}
#endif

MemoryMappedEntry::MemoryMappedEntry():_mappedFile(0){
    
}
bool MemoryMappedEntry::allocate(U64 byteCount,const char* path){
    assert(path);
#ifdef POWITER_DEBUG
    if(QFile::exists(path)){
        cout << "WARNING: A file with the same name already exists : " << path
        << " (If displayed on startup ignore it, this is a debug message "
        << " , if not,it probably means your hashing function does not scatter"
        << " the entries efficiently enough)."<< endl;
    }
#endif
    try{
        _mappedFile = new MemoryFile(path,Powiter::if_exists_keep_if_dont_exists_create);
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
    if(_path.empty()) {
        _path.append(path);
    } else if(_path != newPath) {
        _path = newPath;
    }
    return true;
}
void MemoryMappedEntry::deallocate(){
    if(_mappedFile){
        delete _mappedFile;
    }
    _mappedFile = 0;
}

bool MemoryMappedEntry::reOpen(){
    if(_path.empty()) return false;
    try{
        _mappedFile = new MemoryFile(_path.c_str(),Powiter::if_exists_keep_if_dont_exists_fail);
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
    Q_UNUSED(path);
    _data = (char*)malloc(byteCount);
    if(!_data) return false;
    return true;
}
void InMemoryEntry::deallocate(){
    free(_data);
    _data = 0;
}
InMemoryEntry::~InMemoryEntry(){
    deallocate();
}



AbstractCacheHelper::AbstractCacheHelper():_maximumCacheSize(0),_size(0){
    
}
AbstractCacheHelper::~AbstractCacheHelper(){
    QMutexLocker guard(&_lock);
    for(CacheIterator it = _cache.begin() ; it!=_cache.end() ; ++it) {
        CacheEntry* entry = getValueFromIterator(it);
        delete entry;
    }
    _cache.clear();
    _size = 0;
    
}

void AbstractCacheHelper::clear(){
    QMutexLocker guard(&_lock);
    std::vector<std::pair<U64,CacheEntry*> > backUp;
    for(CacheIterator it = _cache.begin() ; it!=_cache.end() ; ++it) {
        CacheEntry* entry = getValueFromIterator(it);
        assert(entry);
        if (entry->isMemoryMappedEntry()) {
            MemoryMappedEntry* mmapEntry = dynamic_cast<MemoryMappedEntry*>(entry);
            if (mmapEntry) {
                if(QFile::exists(mmapEntry->path().c_str())){
                    QFile::remove(mmapEntry->path().c_str());
                }
            }
        }
        _size -= entry->size();
        if (entry->isRemovable()) {
            delete entry;
        } else {
            backUp.push_back(make_pair(it->first,it->second));
        }
    }
    _cache.clear();
    for (unsigned int i = 0; i < backUp.size(); ++i) {
        add(backUp[i].first, backUp[i].second);
    }
}
void AbstractCacheHelper::erase(CacheIterator it){
    _cache.erase(it);
}

/*Returns an iterator to the cache. If found it points
 to a valid cache entry, otherwise it points to to end.*/
CacheEntry* AbstractCacheHelper::getCacheEntry(U64 key)  {
    QMutexLocker g(&_lock);
    CacheEntry* ret = _cache(key);
    if(ret) ret->addReference();
    return ret;
}


bool AbstractCacheHelper::add(U64 key,CacheEntry* entry){
    QMutexLocker guard(&_lock);
    bool evict = false;
    {
        if (_size >= _maximumCacheSize) {
            evict = true;
        }
        _size += entry->size();
    }
    std::pair<U64,CacheEntry*> evicted = _cache.insert(key,entry,evict);
    
    if (evicted.second) {
        /*while we removed an entry from the cache that must not be removed, we insert it again.
         If all the entries in the cache cannot be removed (in theory it should never happen), the
         last one will be evicted.*/
        
        while(!evicted.second->isRemovable()) {
            evicted = _cache.insert(key,entry,true);
            assert(evicted.second);
        }
        _size -= evicted.second->size();
        
        /*if it is a memorymapped entry, remove the backing file in the meantime*/
        if (evicted.second->isMemoryMappedEntry()) {
            MemoryMappedEntry* mmEntry = dynamic_cast<MemoryMappedEntry*>(evicted.second);
            if (mmEntry) {
                assert(mmEntry->getMappedFile());
                std::string path = mmEntry->getMappedFile()->path();
                QFile::remove(path.c_str());
            }
        }
        delete evicted.second;
    }
    return evict;
}

bool AbstractMemoryCache::add(U64 key,CacheEntry* entry){
    assert(dynamic_cast<InMemoryEntry*>(entry));
    return AbstractCacheHelper::add(key, entry);
}

AbstractDiskCache::AbstractDiskCache(double inMemoryUsage):_inMemorySize(0){
    _maximumInMemorySize = inMemoryUsage;
}

bool AbstractDiskCache::add(U64 key,CacheEntry* entry){
    MemoryMappedEntry* mmEntry = dynamic_cast<MemoryMappedEntry*>(entry);
    assert(mmEntry);
    bool mustEvictFromMemory = false;
    QMutexLocker guard(&_lock);
    {
        if(_inMemorySize > _maximumInMemorySize*getMaximumSize()){
            mustEvictFromMemory = true;
        }
        _inMemorySize += mmEntry->size();
    }
    std::pair<U64,CacheEntry*> evicted = _inMemoryPortion.insert(key, mmEntry, mustEvictFromMemory);
    if (evicted.second) {
        if (evicted.second->isRemovable()) {
            /*switch the evicted entry from memory to the disk.*/
            MemoryMappedEntry* mmEvictedEntry = dynamic_cast<MemoryMappedEntry*>(evicted.second);
            assert(mmEvictedEntry);
            mmEvictedEntry->deallocate();
            _inMemorySize -= mmEvictedEntry->size();
            return AbstractCacheHelper::add(evicted.first, evicted.second);
        } else {
            /*We risk an infinite loop here. It might happen if all entries in the cache cannot be removed.
             This is bad design if all entries in the cache cannot be removed, this means that they all live
             in memory and that you shouldn't use a disk cache for this purpose.*/
            add(evicted.first,evicted.second);
        }
    }
    
    return false;
}

CacheEntry* AbstractDiskCache::isInMemory(U64 key) {
    QMutexLocker g(&_lock);
    CacheEntry* ret = _inMemoryPortion(key);
    if(ret)ret->addReference();
    return ret;
}

void AbstractDiskCache::initializeSubDirectories(){
    QDir cacheFolder(getCachePath());
    cacheFolder.mkpath(".");

    QStringList etr = cacheFolder.entryList();
    // if not 256 subdirs, we re-create the cache
    if (etr.size() < 256) {
        foreach(QString e, etr){
            cacheFolder.rmdir(e);
        }
    }
    for(U32 i = 0x00 ; i <= 0xF; ++i) {
        for(U32 j = 0x00 ; j <= 0xF ; ++j) {
            ostringstream oss;
            oss << hex <<  i;
            oss << hex << j ;
            string str = oss.str();
            cacheFolder.mkdir(str.c_str());
        }
    }
}

AbstractDiskCache::~AbstractDiskCache(){
}

void AbstractDiskCache::clearInMemoryCache() {
    _inMemorySize = 0;
    U32 evictedNotRemovableCount = 0;
    while (_inMemoryPortion.size() > 0) {
        std::pair<U64,CacheEntry*> evicted = _inMemoryPortion.evict();
        assert(evicted.second);
        if (evicted.second->isRemovable()) { // shouldn't we remove it anyay, even if it's not removable?
            MemoryMappedEntry* mmEntry = dynamic_cast<MemoryMappedEntry*>(evicted.second);
            assert(mmEntry);
            mmEntry->deallocate();
            AbstractCacheHelper::add(evicted.first, evicted.second);
        } else {
            /*I think I fixed it with this counter. If all the frames are not removable, we
             just return. We can't actually remove a frame not removable since it is currently
             used by the engine, and this is probably during a memcpy call.*/
            ++evictedNotRemovableCount;
            if(evictedNotRemovableCount ==  _inMemoryPortion.size())
                return;
            add(evicted.first, evicted.second);
#warning "if evicted.second->isRemovable() is false, we go into an infinite loop here! Just do a simple project with reader+viewer"
        }
    }
}

void AbstractDiskCache::clearDiskCache(){
    clearInMemoryCache();
    clear();
    
    cleanUpDiskAndReset();
}

void AbstractDiskCache::save(){
    QString newCachePath(getCachePath());
    newCachePath.append(QDir::separator());
    newCachePath.append("restoreFile.powc");
    QFile _restoreFile(newCachePath);
    _restoreFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append);
    QTextStream out(&_restoreFile);
    /*clearing in-memory cache so only the on-disk portion has entries left.*/
    clearInMemoryCache();
    for(CacheIterator it = _cache.begin(); it!= _cache.end() ; ++it) {
        MemoryMappedEntry* mmEntry = dynamic_cast<MemoryMappedEntry*>(AbstractCache::getValueFromIterator(it));
        assert(mmEntry);
        out << mmEntry->printOut().c_str() << endl;
    }
    _restoreFile.close();
    
    
}

void AbstractDiskCache::restore(){
    QString newCachePath(getCachePath());
    QString settingsFilePath(newCachePath);
    settingsFilePath.append(QDir::separator());
    settingsFilePath.append("restoreFile.powc");
    if(QFile::exists(settingsFilePath)){
        QDir directory(newCachePath);
        QStringList files = directory.entryList();
        
        
        /*Now counting actual data files in the cache*/
        /*check if there's 256 subfolders, otherwise reset cache.*/
        int count = 0; // -1 because of the restoreFile
        int subFolderCount = 0;
        for(int i =0 ; i< files.size() ;++i) {
            QString subFolder(newCachePath);
            subFolder.append(QDir::separator());
            subFolder.append(files[i]);
            if(subFolder.right(1) == QString(".") || subFolder.right(2) == QString("..")) continue;
            QDir d(subFolder);
            if(d.exists()){
                ++subFolderCount;
                QStringList items = d.entryList();
                for(int j = 0 ; j < items.size();++j) {
                    if(items[j] != QString(".") && items[j] != QString("..")) {
                        ++count;
                    }
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
            if ((U32)count == entries.size()) {
                for (U32 i = 0; i < entries.size(); ++i) {
                    std::pair<U64,MemoryMappedEntry*>& entry = entries[i];
                    add(entry.first,entry.second);
                }
                
                /*cache is filled , debug*/
                // debug();
                
            }else{
                cout << cacheName() << ": The entries count in the restore file does not equal the number of actual data files.Reseting." << endl;
                cleanUpDiskAndReset();
            }
            
            /*now that we're done using it, clear it*/
            //QFile::remove(settingsFilePath);
            //QFile newFile(settingsFilePath);
            //newFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate);
            //QTextStream outsetti(&newFile);
            //outsetti << cacheVersion().c_str() << endl;
            //newFile.close();
            _restoreFile.remove();
            _restoreFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate);
            QTextStream outsetti(&_restoreFile);
            outsetti << cacheVersion().c_str() << endl;
            _restoreFile.close();
        } else { /*cache version is different*/
            /*re-create the cache*/
            _restoreFile.remove();
            cleanUpDiskAndReset();
            /*re-create settings file*/
            //QFile _restoreFile(settingsFilePath); // error?
            _restoreFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate);
            QTextStream outsetti(&_restoreFile);
            outsetti << cacheVersion().c_str() << endl;
            _restoreFile.close();
        }
    }else{ // restore file doesn't exist
        /*re-create cache*/

        QDir(getCachePath()).mkpath(".");

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
    QString cachePath(getCachePath());
#   if QT_VERSION < 0x050000
    removeRecursively(cachePath);
#   else
    QDir dir(cachePath);
    if(dir.exists()){
        dir.removeRecursively();
    }
#endif
    initializeSubDirectories();
    
}

QString AbstractDiskCache::getCachePath(){
#if QT_VERSION < 0x050000
    QString cacheFolderName(QDesktopServices::storageLocation(QDesktopServices::CacheLocation));
#else
    QString cacheFolderName(QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + QDir::separator());
#endif
    cacheFolderName.append(QDir::separator());
    QString str(cacheFolderName);
    str.append(cacheName().c_str());
    return str;
}

void AbstractDiskCache::debug(){
    QMutexLocker g(&_lock);
    cout << "====================DEBUGING: " << cacheName() << " =========" << endl;
    cout << "-------------------IN MEMORY ENTRIES-------------------" << endl;
    for (CacheIterator it = _inMemoryPortion.begin(); it!=_inMemoryPortion.end(); ++it) {
        MemoryMappedEntry* entry = dynamic_cast<MemoryMappedEntry*>(getValueFromIterator(it));
        assert(entry);
        cout << "[" << entry->path() << "] = " << entry->size() << " bytes. ";
        if(entry->getMappedFile()->data()){
            cout << "Entry has a valid ptr." << endl;
        }else{
            cout << "Entry has a NULL ptr." << endl;
        }
        cout << "Key:" << it->first << endl;
    }
    cout <<" --------------------ON DISK ENTRIES---------------------" << endl;
    for (CacheIterator it = _cache.begin(); it!=_cache.end(); ++it) {
        MemoryMappedEntry* entry = dynamic_cast<MemoryMappedEntry*>(getValueFromIterator(it));
        assert(entry);
        cout << "[" << entry->path() << "] = " << entry->size() << " bytes. ";
        if(entry->getMappedFile()){
            cout << "Entry is still allocated!!" << endl;
        }else{
            cout << "Entry is normally deallocated." << endl;
        }
        cout << "Key:" << it->first << endl;
    }
    cout << "Total entries count : " << _inMemoryPortion.size()+_cache.size() << endl;
    cout << "-===========END DEBUGGING: " << cacheName() << " ===========" << endl;
    
}

