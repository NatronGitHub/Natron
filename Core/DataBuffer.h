//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com
#ifndef __PowiterOsX__DataBuffer__
#define __PowiterOsX__DataBuffer__

#include <iostream>
#include <QtCore/QMutex>
#include <map>
#include "Core/referenceCountedObj.h"
#include <string>

/*This class implements a generic Buffer with reference counting.
 *When using it you should not directly use the DataBuffer class
 *but only the DataBufferManager class, that will do the counting.
 *The buffer itself is thread-safe
 **/
class DataBuffer : public ReferenceCountedObject{
    
    
public:
    typedef RefCPtr<DataBuffer> Ptr;
    
    DataBuffer();
    virtual ~DataBuffer();
    
    void allocate(size_t nb_bytes);

	/*effectivly resizes the buffer to nb_bytes.
	 *After this all the data in the buffer are junk*/
    void resize(size_t nb_bytes);

	/*set to the 0 the entire content of the buffer*/
    void clear();
    
    /* fill the buffer from src with nb_bytes of data . 
    >// If nb_bytes > _size then only _size bytes are copied and the difference in bytes is returned.
    >// If everything happens without any problem, 0 is returned.
    >// -1 is returned if the buffer is NULL.
    >// the step parameter specifies the step(in bytes) in the src buffer of each element desired to be copied.
     */
    size_t fillBuffer(const void* src,size_t nb_bytes,size_t step=1);

    bool valid(){return _buffer!=NULL;}

    unsigned char* buffer(){return _buffer;}

	bool isZero(){
		bool res=true;
		int i=0;
		while(i<_size){
			if((int)_buffer[i] != 0) res = false;
			i++;
		}
		return res;
	}
private:
    unsigned char* _buffer;
    size_t _size;
    QMutex* _lock;
};

/*Manages all the buffers currently active. Each buffer is referenced by a name.
 *This class is thread-safe .*/
class DataBufferManager {
    typedef std::map<std::string,DataBuffer::Ptr> DataBufferMap;
    DataBufferMap _bufferMap;
    QMutex lock_;
public:
	/*Return a reference counted pointer to the buffer with
	 *bufferName if it exists. Otherwise creates a new empty buffer*/
    DataBuffer::Ptr get(const char* bufferName);

	/*Reduces the count of bufferName by 1.
	 *If it was the only reference left,
	 *it effectivly delete bufferName.*/
    void release(const char* bufferName);
};

static DataBufferManager _dataBufferManager;

#endif /* defined(__PowiterOsX__DataBuffer__) */
