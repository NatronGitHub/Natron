//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com

#ifndef __PowiterOsX__nodecache__
#define __PowiterOsX__nodecache__
#include "Core/abstractCache.h"
#include "Core/singleton.h"

class ChannelSet;
class Row;


/*The node-cache stores the results of previously computed Rows by all
 the nodes across the DAG. Some nodes don't cache and will never put a new
 entry in this class (I.e : the viewer node, which has its custom cache).
 Nodes that do small/almost instant computations should avoid caching too much,
 or should propose an option to cache or not since it can lead to heavy memory
 consumption. 
 Last important point : this cache implements the singleton pattern as it is the 
 only object that will represent all the node cache.
 */
#include <QtCore/QString>

class NodeCache : public AbstractDiskCache, public Singleton<NodeCache> {
    
public:
    NodeCache();
    
    virtual ~NodeCache();
   
    virtual std::string cacheName(){return "NodeCache";}
    
    virtual std::string cacheVersion(){return "1.0.0";}
    
    /*Recover an entry from string*/
    virtual std::pair<U64,MemoryMappedEntry*> recoverEntryFromString(QString str);
    
    static NodeCache* getNodeCache();
        
    /*Returns a valid pair<key,row> if the cache was able to find a row represented
     by the nodeKey, the filename and the range (x,r). 
     Returns <0,NULL> if nothing was found.*/
    std::pair<U64,Row*> get(U64 nodeKey,std::string filename,int x,int r,int y ,ChannelSet& channels);
    
    /*add a new Row(x,y,r,channels) to the cache with the U64 as key and
     returns a pointer to it.*/
    Row* add(U64 key,int x,int r,int y,ChannelSet& channels,std::string filename);
        
private:
        
    /*settings file path*/
    QString getCachePath();
};

/*special macro to get the unique pointer to the node cache*/
#define nodeCache NodeCache::getNodeCache()

#endif /* defined(__PowiterOsX__nodecache__) */
