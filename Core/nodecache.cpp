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

NodeCache::NodeCache() : AbstractMemoryCache() {
    
}
NodeCache::~NodeCache(){
    
}

NodeCache* NodeCache::getNodeCache(){
   return NodeCache::instance();
}


std::pair<U64,Row*> NodeCache::get(U64 nodeKey, std::string filename, int x, int r,int y ,ChannelSet& channels){
    U64 hashKey = Row::computeHashKey(nodeKey, filename, x, r, y );
    CacheIterator it = isCached(hashKey);
    if (it == end()) {// not in memory
        return make_pair(hashKey,(Row*)NULL);
    }else{ // found in memory
        CacheEntry* entry = AbstractCache::getValueFromIterator(it);
        Row* rowEntry = static_cast<Row*>(entry);
        assert(rowEntry);
        return make_pair(it->first,rowEntry);
    }
    return make_pair(hashKey,(Row*)NULL);
}

Row* NodeCache::add(U64 key,int x, int r, int y, ChannelSet &channels,std::string filename){
    Row* out = 0;
    try{
       out = new Row(x,y,r,channels);
    }catch(const char* str){
        cout << "Failed to create row: " << str << endl;
        delete out;
        return NULL;
    }
    if(!out->allocate()){
        cout << "Failed to allocate row..." << endl;
        delete out;
        return NULL;
    }else{
        AbstractMemoryCache::add(key, out);
    }
    return out;
    
}

