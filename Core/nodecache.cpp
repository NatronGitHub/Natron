//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com
#include "Core/nodecache.h"
#include "Core/row.h"
#include "Core/mappedfile.h"
#include "Core/channels.h"
#include "Core/hash.h"
#include <sstream>
#include <cassert>
#include <QtCore/QDir>
using namespace std;

NodeCache::NodeCache():AbstractDiskCache(0){
    
}
NodeCache::~NodeCache(){
    
}


QString NodeCache::getCachePath(){
    QString str(CACHE_ROOT_PATH);
    str.append(cacheName().c_str());
    str.append("/");
    return str;
}

NodeCache* NodeCache::getNodeCache(){
   return NodeCache::instance();
}


std::pair<U64,Row*> NodeCache::get(U64 nodeKey, std::string filename, int x, int r,int y ,ChannelSet& channels){
    U64 hashKey = Row::computeHashKey(nodeKey, filename, x, r, y );
    CacheIterator it = isInMemory(hashKey);
    if (it == endMemoryCache()) {// not in memory
        it = isCached(hashKey);
        if(it == end()){ //neither on disk
            return make_pair(0,(Row*)NULL);
        }else{ // found on disk
            CacheEntry* entry = AbstractCache::getValueFromIterator(it);
            Row* rowEntry = dynamic_cast<Row*>(entry);
            assert(rowEntry);
            rowEntry->restoreFromBackingFile();
            return make_pair(it->first,rowEntry);
        }
    }else{ // found in memory
        CacheEntry* entry = AbstractCache::getValueFromIterator(it);
        Row* rowEntry = dynamic_cast<Row*>(entry);
        assert(rowEntry);
        return make_pair(it->first,rowEntry);
    }
    return make_pair(0,(Row*)NULL);
}

Row* NodeCache::add(U64 key,int x, int r, int y, ChannelSet &channels,std::string filename){
    Row* out = new Row(x,y,r,channels,Powiter_Enums::BACKED_ON_DISK);
    string name(getCachePath().toStdString());
    {
        QMutexLocker guard(&_mutex);
        ostringstream oss1;
        oss1 << hex << (key >> 56);
        name.append(oss1.str());
        ostringstream oss2;
        oss2 << hex << ((key << 8) >> 8);
        QDir subfolder(name.c_str());
        if(!subfolder.exists()){
            cout << "Something is wrong in cache... couldn't find : " << name << endl;
        }else{
            name.append("/");
            name.append(oss2.str());
            name.append(".powc");
        }
    }
    out->allocate(name.c_str());
    AbstractDiskCache::add(key, out);
    return out;
    
}

std::pair<U64,MemoryMappedEntry*> NodeCache::recoverEntryFromString(QString str) {
    return Row::recoverFromString(str);
}
