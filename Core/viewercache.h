//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com

#ifndef DISKCACHE_H
#define DISKCACHE_H

#include "Core/abstractCache.h"
#include "Core/singleton.h"

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
	int _actualH,_actualW; // zoomed H&W
	FrameEntry();
	FrameEntry(float zoom,float exp,float lut,U64 treeVers,
		float byteMode,ReaderInfo* info,int actualW,int actualH);
	FrameEntry(const FrameEntry& other);


	virtual std::string printOut();

	static std::pair<U64,FrameEntry*> recoverFromString(QString str);

	/*Returns a key computed from the parameters.*/
	static U64 computeHashKey(std::string filename,
		U64 treeVersion,
		float zoomFactor,
		float exposure,
		float lut ,
		float byteMode,
		int w,
		int h);

	virtual ~FrameEntry();

	bool operator==(const FrameEntry& other);


};

class ReaderInfo;
class MemoryMappedEntry;
class ViewerCache : public AbstractDiskCache , public Singleton<ViewerCache>
{

public:


	ViewerCache();

	~ViewerCache();

	virtual std::string cacheName(){return "ViewerCache";}

	virtual std::string cacheVersion(){return "1.0.0";}

	/*Recover an entry from string*/
	virtual std::pair<U64,MemoryMappedEntry*> recoverEntryFromString(QString str);


	static ViewerCache* getViewerCache();

	/*Construct a frame entry,adds it to the cache and returns a pointer to it.*/
	FrameEntry* add(U64 key,
		std::string filename,
		U64 treeVersion,
		float zoomFactor,
		float exposure,
		float lut ,
		float byteMode,
		int w,
		int h,
		ReaderInfo* info);


	/*Returns a valid frameID if it could find one matching the parameters, otherwise
	returns <0,NULL>.*/
	std::pair<U64,FrameEntry*>  get(std::string filename,
		U64 treeVersion,
		float zoomFactor,
		float exposure,
		float lut ,
		float byteMode,
		int w,
		int h,
		ReaderInfo* info){

	}





};



#endif // DISKCACHE_H
