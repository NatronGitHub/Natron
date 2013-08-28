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

#include "NodeCache.h"

#include <sstream>
#include <cassert>
#include <QtCore/QDir>

#include "Engine/Row.h"
#include "Engine/MemoryFile.h"
#include "Engine/ChannelSet.h"
#include "Engine/Hash.h"

using namespace std;

NodeCache::NodeCache() : AbstractMemoryCache() {
}

NodeCache::~NodeCache() {    
}

NodeCache* NodeCache::getNodeCache(){
   return NodeCache::instance();
}


std::pair<U64,Row*> NodeCache::get(U64 nodeKey, const std::string& filename, int x, int r, int y, const ChannelSet& ){
    U64 hashKey = Row::computeHashKey(nodeKey, filename, x, r, y );
    CacheEntry* entry = getCacheEntry(hashKey);
    if(entry){
        Row* rowEntry = dynamic_cast<Row*>(entry);
        assert(rowEntry);
        return make_pair(hashKey,rowEntry);
    }
    return make_pair(hashKey,(Row*)NULL);
}

Row* NodeCache::addRow(U64 key,int x, int r, int y, const ChannelSet &channels,const std::string&){
    Row* out = 0;
    try{
       out = new Row(x,y,r,channels);
    }catch(const char* str){
        cout << "Failed to create row: " << str << endl;
        delete out;
        return NULL;
    }
    if(!out->allocateRow()){
        cout << "Failed to allocate row..." << endl;
        delete out;
        return NULL;
    }else{
        out->notifyCacheForDeletion();
        out->addReference(); // increase reference counting BEFORE adding it to the cache and exposing it to other threads
        AbstractMemoryCache::add(key, out);
    }
    return out;
    
}

