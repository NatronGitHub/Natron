//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com

#ifndef DISKCACHE_H
#define DISKCACHE_H

//#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <QtGui/qvector4d.h>
#include <QtCore/QFile>

#define CACHE_VERSION "1.0.0"

#include "Core/hash.h"
#include "Core/row.h"
#include "Core/displayFormat.h"

class ReaderInfo;
class VideoEngine;
class ViewerGL;
class MMAPfile;
class ViewerCache
{
    
    friend class VideoEngine;
    
    qint64 MAX_VIEWER_CACHE_SIZE; // total disk cache space allowed
    qint64 MAX_RAM_CACHE; // total ram cache space allowed
    qint64 newCacheBlockIndex; // the number in cache of the new frame
    qint64 cacheSize; // current cache size
   
public:
    
	/*The class associated to a Frame in cache.
     *Two frames with the same FrameID will be considered
     *the same.*/
    class FrameID{
    public:
        
        
        float _exposure;
        float _lut;
        int _rank;
        float _zoom;
        U64 _treeVers;
        ReaderInfo* _frameInfo; //bbox,format,channels,frame name,ydirection
        std::string _cacheIndex;
        float _byteMode;
        int _actualH,_actualW; // zoomed H&W
        FrameID();
        FrameID(float zoom,float exp,float lut,int rank,U64 treeVers,
                std::string cacheIndex,float byteMode,ReaderInfo* info,int actualW,int actualH);
        FrameID(const FrameID& other);
        
        ~FrameID();
        void operator=(const FrameID& other);
        
        bool operator==(const FrameID& other);
        
        
    };
    
	typedef std::multimap<std::string,ViewerCache::FrameID>::iterator FramesIterator;
	typedef std::multimap<std::string,ViewerCache::FrameID>::reverse_iterator ReverseFramesIterator;
    
    // multimap : each filename  may have several match
    // depending on the following variables : current tree version(hash value), current builtin zoom, exposure , viewer LUT
    // the FrameID represents then : < zoomLvl, exp , LUT , rank >   ranking is for LRU removal when cache is full
    //This map is an outpost that prevents Powiter to look into the disk whether the frame is already present
    std::multimap<std::string,ViewerCache::FrameID> _frames;
    
    std::map<int,ViewerCache::FrameID> _rankMap; // storing rank into a map to make freeing algorithm efficient
    
    
    std::vector<std::pair<ViewerCache::FrameID,MMAPfile*> > _playbackCache; // playback cache ( in memory : volatile)
    
    
    qint64 _playbackCacheSize;
    
    qint64 MAX_PLAYBACK_CACHE_SIZE;
    
    ViewerCache::FramesIterator end(){return _frames.end();}
    ViewerCache::FramesIterator begin(){return _frames.begin();}
    
    /* add the frame to the cache if there's enough space, otherwise some free space is made (LRU) to insert it
     It appends the frame with rank 0 (remove last) and increments all the other frame present in cache
     The nbFrameHint is to know how many frame will be actually used in this sequence and how many should we free
     */
    std::pair<char*,ViewerCache::FrameID> mapNewFrame(int frameNB,
                                                      std::string filename,
                                                      int width,
                                                      int height,
                                                      int nbFrameHint,
                                                      U64 treeVers);
    
    /*Close the last file in the queue*/
    void closeMappedFile();
    
    /*empty all the cache held by the RAM*/
    void clearPlayBackCache();
    
    /*checks whether the frame is present or not.
     Returns a FramesIterator pointing to the frame if it found it other wise the boolean returns false.*/
    std::pair<ViewerCache::FramesIterator,bool> isCached(std::string filename,
                                                         U64 treeVersion,
                                                         float zoomFactor,
                                                         float exposure,
                                                         float lut ,
                                                         float byteMode);
    
    /*This is the function called to finilize caching once the frame has been written to the pointer
     returned by mapNew<Frame*/
    void appendFrame(ViewerCache::FrameID _info);
    
    
    const char* retrieveFrame(int frameNb,FramesIterator it);
    
    /*Restores the cache from the disk(from the settings file)*/
    void restoreCache();
    
    /*Saves the cache state to the disk(only the settings)*/
    void saveCache();
    
    /*Clears all the disk cache even the playback cache.*/
    void clearCache();
    
    
    void debugCache(bool verbose);
    
    ViewerCache(qint64 maxDiskSize,qint64 maxRamSize);
    
    ~ViewerCache();
private:
    
    // LRU freeing algorithm for the disk file
    void makeFreeSpace(int nbFrames);
    
};



#endif // DISKCACHE_H
