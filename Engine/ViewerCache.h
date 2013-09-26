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

#ifndef POWITER_ENGINE_VIEWERCACHE_H_
#define POWITER_ENGINE_VIEWERCACHE_H_

#include <QtCore/QObject>

#include "Engine/AbstractCache.h" // for MemoryMappedEntry

#include "Gui/Texture.h" // for TextureRect

class Format;
class ChannelSet;
class Box2D;
class Model;
class ReaderInfo;

/*The class associated to a Frame in cache.
 .*/
class FrameEntry : public MemoryMappedEntry {
public:
    
    
	float _exposure;
	float _lut;
	float _zoom;
	U64 _treeVers;
	ReaderInfo* _frameInfo; //bbox,format,channels,frame name,ydirection
	float _byteMode;
	TextureRect _textureRect; // zoomed H&W
    
	FrameEntry();
    
	FrameEntry(QString inputFileNames,float zoom,float exp,float lut,U64 treeVers,
                         float byteMode,const Box2D& bbox,const Format& dispW,
                         const ChannelSet& channels,const TextureRect& textureRect);
    
    FrameEntry(float zoom,float exp,float lut,U64 treeVers,
               float byteMode,ReaderInfo* info,const TextureRect& textureRect);
    
    
	virtual std::string printOut();
    
	static FrameEntry* recoverFromString(QString str);
    
	/*Returns a key computed from the parameters.*/
	static U64 computeHashKey(int frameNb,
                              QString inputFileNames,
                              U64 treeVersion,
                              float zoomFactor,
                              float exposure,
                              float lut ,
                              float byteMode,
                              const Box2D& bbox,
                              const Format& dispW,
                              const TextureRect& frameRect);
    
	virtual ~FrameEntry();
    
	bool operator==(const FrameEntry& other);
    
    
};

class ReaderInfo;
class MemoryMappedEntry;
class ViewerCache :public QObject, public AbstractDiskCache
{
       Q_OBJECT
public:
    
    
	ViewerCache();
        
	virtual ~ViewerCache();
    
	virtual std::string cacheName(){std::string str("ViewerCache");return str;}
    
	virtual std::string cacheVersion(){std::string str("v1.0.0");return str;}
        
    virtual std::pair<U64,MemoryMappedEntry*> recoverEntryFromString(QString str);
    
	/*Construct a frame entry,adds it to the cache and returns a pointer to it.*/
    // on output, the FrameEntry is locked, and must be unlocked using FrameEntry::unlock()
	FrameEntry* addFrame(U64 key,
                         QString inputFileNames,
                         U64 treeVersion,
                         float zoomFactor,
                         float exposure,
                         float lut ,
                         float byteMode,
                         const TextureRect& textureRect,
                         const Box2D& bbox,
                         const Format& dispW);
    
    
	/*Returns a valid frameID if it could find one matching the parameters, otherwise
     returns NULL.*/
    // on output, the FrameEntry is locked, and must be unlocked using FrameEntry::unlock()
	FrameEntry* get(U64 key);
    
    
    void clearInMemoryPortion();
    
signals:
    void addedFrame();
    void removedFrame();
    void clearedInMemoryFrames();
};



#endif // POWITER_ENGINE_VIEWERCACHE_H_
