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
    int _nbRows;
    char* _filename;
    char* _cacheIndex;
    int _rowWidth;
    float _byteMode;
    FrameID():_exposure(0),_lut(0),_zoom(0),_treeVers(0),_nbRows(0),_cacheIndex(0),
    _rowWidth(0),_filename(0),_byteMode(0) {}
    FrameID(float zoom,float exp,float lut,int rank,U32 treeVers,
            char* cacheIndex,int rowWidth,int nbRows,
            char* fileName,float byteMode);
    FrameID(const FrameID& other);
    
    ~FrameID(){
    }
    
bool operator==(const FrameID& other){
    return _exposure == other._exposure && _lut == other._lut && _zoom == other._zoom && _treeVers == other._treeVers
    && _byteMode == other._byteMode && !strcmp(_filename,other._filename);
}

};



/*A comparison class used by the cache internally*/
class FramesCompare { 
public:
    bool operator()(const char* x,const char* y) const { return strcmp(x,y)<0; }
};

typedef std::multimap<char*, FrameID,FramesCompare>::iterator FramesIterator;
typedef std::multimap<char*, FrameID,FramesCompare>::reverse_iterator ReverseFramesIterator;

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
    std::multimap<char*,FrameID,FramesCompare> _frames;
    
    std::map<int,FrameID> _rankMap; // storing rank into a map to make freeing algorithm efficient
    
    
    std::vector<std::pair<FrameID,MMAPfile*> > _playbackCache; // playback cache ( in memory : volatile)
    
    
    qint64 _playbackCacheSize;
    
    qint64 MAX_PLAYBACK_CACHE_SIZE;
    
    /* add the frame to the cache if there's enough space, otherwise some free space is made (LRU) to insert it
     It appends the frame with rank 0 (remove last) and increments all the other frame present in cache
     The nbFrameHint is to know how many frame will be actually used in this sequence and how many should we free
     */
    std::pair<char*,FrameID> mapNewFrame(int frameNB,char* filename,int width,int height,int nbFrameHint,U32 treeVers);
    
    /*Close the last file in the queue*/
    void closeMappedFile();
    
    /*empty all the cache held by the RAM*/
    void clearPlayBackCache();
    
    /*checks whether the frame is present or not.
     Returns a FramesIterator pointing to the frame if it found it other wise the boolean returns false.*/
    std::pair<FramesIterator,bool> isCached(char* filename,U32 treeVersion,float builtinZoom,float exposure,float lut ,bool byteMode );
    
    /*This is the function called to finilize caching once the frame has been written to the pointer
     returned by mapNew<Frame*/
    void appendFrame(FrameID _info);
    
    /* Bind the PBO referenced by the unit texBuffer and fill it with the frame data 
     pointed to by it. frameNB is here to know how many frame will be retrieved 
     thus to optimize the freeing algorithm.*/
    std::pair<int,int> retrieveFrame(int frameNb,FramesIterator it,int texBuffer);
    
    /*Restores the cache from the disk(from the settings file)*/
    void restoreCache();
    
    /*Saves the cache state to the disk(only the settings)*/
    void saveCache();
    
    /*Clears all the disk cache even the playback cache.*/
    void clearCache();

    
    DiskCache(ViewerGL* gl_viewer,qint64 maxDiskSize,qint64 maxRamSize);
    
    ~DiskCache();
private:
    
    // LRU freeing algorithm for the disk file
    void makeFreeSpace(int nbFrames);
    
};



#endif // DISKCACHE_H
