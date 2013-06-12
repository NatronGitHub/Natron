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

NodeCache* NodeCache::getNodeCache(){
   return NodeCache::instance();
}


std::pair<U64,Row*> NodeCache::get(U64 nodeKey, std::string filename, int x, int r,int y ,ChannelSet& channels){
    U64 hashKey = Row::computeHashKey(nodeKey, filename, x, r, y );
    CacheIterator it = isInMemory(hashKey);
    if (it == endMemoryCache()) {// not in memory
        it = isCached(hashKey);
        if(it == end()){ //neither on disk
            return make_pair(hashKey,(Row*)NULL);
        }else{ // found on disk
            CacheEntry* entry = AbstractCache::getValueFromIterator(it);
            Row* rowEntry = dynamic_cast<Row*>(entry);
            assert(rowEntry);
            if(!rowEntry->restoreMapping()) return make_pair(0,(Row*)NULL);
            return make_pair(it->first,rowEntry);
        }
    }else{ // found in memory
        CacheEntry* entry = AbstractCache::getValueFromIterator(it);
        Row* rowEntry = dynamic_cast<Row*>(entry);
        assert(rowEntry);
        return make_pair(it->first,rowEntry);
    }
    return make_pair(hashKey,(Row*)NULL);
}

Row* NodeCache::add(U64 key,int x, int r, int y, ChannelSet &channels,std::string filename){
    Row* out = 0;
    try{
       out = new Row(x,y,r,channels,Powiter_Enums::BACKED_ON_DISK);
    }catch(const char* str){
        cout << "Failed to create row: " << str << endl;
        delete out;
        return NULL;
    }
    string name(getCachePath().toStdString());
    {
        
        QWriteLocker guard(&_cache._rwLock);
        ostringstream oss1;
        oss1 << hex << (key >> 60);
        oss1 << hex << ((key << 4) >> 60);
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
    if(!out->allocate(name.c_str())){
        cout << "Failed to allocate row..." << endl;
        delete out;
        return NULL;
    }else{
        AbstractDiskCache::add(key, out);
    }
    return out;
    
}

std::pair<U64,MemoryMappedEntry*> NodeCache::recoverEntryFromString(QString str) {
    return Row::recoverFromString(str);
}
