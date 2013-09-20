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

#ifndef POWITER_ENGINE_NODECACHE_H_
#define POWITER_ENGINE_NODECACHE_H_

#include "Engine/AbstractCache.h"

class ChannelSet;
class Row;

/*The node-cache stores the results of previously computed Rows by all
 the nodes across the DAG. Some nodes don't cache and will never put a new
 entry in this class (e.g : the viewer node, which has its custom cache).
 Nodes that do small/almost instant computations should avoid caching too much,
 or should propose an option to cache or not since it can lead to heavy memory
 consumption. 
 An entry in the cache is a pair < hash key, Row >, where the hash key is computed
 by the Hash class with the following parameters :
- Nodekey
- filename
- (x,r)
- y

 Last important point : this cache implements the singleton pattern as it is the 
 only object that will represent all the node cache.
 */
#include <QtCore/QString>

class NodeCache : public AbstractMemoryCache {
    
public:
    NodeCache();
    
    virtual ~NodeCache();
   
    virtual std::string cacheName(){return "NodeCache";}
    
    virtual std::string cacheVersion(){return "v1.0.0";}
    
    /*Returns a valid pair<key,row> if the cache was able to find a row represented
     by the nodeKey, the filename and the range (x,r). 
     Returns <key,NULL> if nothing was found, key being the key that was computed to find the entry.*/
    std::pair<U64,Row*> get(U64 nodeKey, const std::string& filename, int x, int r, int y, const ChannelSet& channels);
    
    /*add a new Row(x,y,r,channels) to the cache with the U64 as key and
     returns a pointer to it.*/
    Row* addRow(U64 key, int x, int r, int y, const ChannelSet& channels, const std::string& filename);

    
};

#endif /* defined(POWITER_ENGINE_NODECACHE_H_) */
