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
#include "Superviser/PwStrings.h"


#define CACHE_VERSION "1.0.0"

#include "Core/hash.h"
#include "Core/row.h"
#include "Reader/readerInfo.h"



/*The class associated to a Frame in cache.
 *Two frames with the same FrameID will be considered
 *the same.*/
class FrameID{
public:
    
    
    float _exposure;
    float _lut;
    int _rank;
    float _zoom;
    U32 _treeVers;
    ReaderInfo* _frameInfo; //bbox,format,channels,frame name,ydirection
    std::string _cacheIndex;
    float _byteMode;
    int _actualH,_actualW; // zoomed H&W
    FrameID():_exposure(0),_lut(0),_zoom(0),_treeVers(0),_byteMode(0),_actualH(0),_actualW(0){
        _frameInfo = new ReaderInfo;
    }
    FrameID(float zoom,float exp,float lut,int rank,U32 treeVers,
            std::string cacheIndex,float byteMode,ReaderInfo* info,int actualW,int actualH);
    FrameID(const FrameID& other);
    
    ~FrameID(){
        delete _frameInfo;
    }
    void operator=(const FrameID& other){
        _exposure=other._exposure;
        _lut = other._lut;
        _rank = other._rank;
        _zoom = other._zoom;
        _treeVers = other._treeVers;
        _frameInfo->copy(other._frameInfo);
        _actualW = other._actualW;
        _actualH = other._actualH;
    _cacheIndex=other._cacheIndex;
    _byteMode=other._byteMode;
}

bool operator==(const FrameID& other){
    return _exposure == other._exposure &&
    _lut == other._lut &&
    _zoom == other._zoom &&
    _treeVers == other._treeVers &&
     _byteMode == other._byteMode &&
    _frameInfo->channels() == other._frameInfo->channels() &&
    _frameInfo->dataWindow() == other._frameInfo->dataWindow() &&
    _frameInfo->displayWindow() == other._frameInfo->displayWindow() &&
    _frameInfo->currentFrameName() == other._frameInfo->currentFrameName() &&
    _actualH == other._actualH &&
    _actualW == other._actualW;
}


};

typedef std::multimap<std::string, FrameID>::iterator FramesIterator;
typedef std::multimap<std::string, FrameID>::reverse_iterator ReverseFramesIterator;

class VideoEngine;
class ViewerGL;
class MMAPfile;
class DiskCache
{
    
    friend class VideoEngine;
    
    qint64 MAX_DISK_CACHE; // total disk cache space allowed
    qint64 MAX_RAM_CACHE; // total ram cache space allowed
    qint64 newCacheBlockIndex; // the number in cache of the new frame
    qint64 cacheSize; // current cache size
    ViewerGL* gl_viewer; // pointer to the viewer
public:
    
    // multimap : each filename  may have several match
    // depending on the following variables : current tree version(hash value), current builtin zoom, exposure , viewer LUT
    // the FrameID represents then : < zoomLvl, exp , LUT , rank >   ranking is for LRU removal when cache is full
    //This map is an outpost that prevents Powiter to look into the disk whether the frame is already present
    std::multimap<std::string,FrameID> _frames;
    
    std::map<int,FrameID> _rankMap; // storing rank into a map to make freeing algorithm efficient
    
    
    std::vector<std::pair<FrameID,MMAPfile*> > _playbackCache; // playback cache ( in memory : volatile)
    
    
    qint64 _playbackCacheSize;
    
    qint64 MAX_PLAYBACK_CACHE_SIZE;
    
    FramesIterator end(){return _frames.end();}
    FramesIterator begin(){return _frames.begin();}
    
    /* add the frame to the cache if there's enough space, otherwise some free space is made (LRU) to insert it
     It appends the frame with rank 0 (remove last) and increments all the other frame present in cache
     The nbFrameHint is to know how many frame will be actually used in this sequence and how many should we free
     */
    std::pair<char*,FrameID> mapNewFrame(int frameNB,std::string filename,int width,int height,int nbFrameHint,U32 treeVers);
    
    /*Close the last file in the queue*/
    void closeMappedFile();
    
    /*empty all the cache held by the RAM*/
    void clearPlayBackCache();
    
    /*checks whether the frame is present or not.
     Returns a FramesIterator pointing to the frame if it found it other wise the boolean returns false.*/
    std::pair<FramesIterator,bool> isCached(std::string filename,
                                            U32 treeVersion,
                                            float builtinZoom,
                                            float exposure,
                                            float lut ,
                                            bool byteMode,
                                            Format format,
                                            Box2D bbox);
    
    /*This is the function called to finilize caching once the frame has been written to the pointer
     returned by mapNew<Frame*/
    void appendFrame(FrameID _info);
    
    
    const char* retrieveFrame(int frameNb,FramesIterator it);
    
    /*Restores the cache from the disk(from the settings file)*/
    void restoreCache();
    
    /*Saves the cache state to the disk(only the settings)*/
    void saveCache();
    
    /*Clears all the disk cache even the playback cache.*/
    void clearCache();
    
    
    void debugCache();
    
    DiskCache(ViewerGL* gl_viewer,qint64 maxDiskSize,qint64 maxRamSize);
    
    ~DiskCache();
private:
    
    // LRU freeing algorithm for the disk file
    void makeFreeSpace(int nbFrames);
    
};



#endif // DISKCACHE_H
