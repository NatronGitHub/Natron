//
//  DataBuffer.h
//  PowiterOsX
//
//  Created by Alexandre on 1/7/13.
//  Copyright (c) 2013 Alexandre. All rights reserved.
//

#ifndef __PowiterOsX__DataBuffer__
#define __PowiterOsX__DataBuffer__

#include <iostream>
#include <QtCore/QMutex>
#include <map>
#include "Core/referenceCountedObj.h"
#include <string>

class DataBuffer : public ReferenceCountedObject{
    
    
public:
    typedef RefCPtr<DataBuffer> Ptr;
    
    DataBuffer();
    virtual ~DataBuffer();
    
    void allocate(size_t nb_bytes);
    void resize(size_t nb_bytes);
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

class DataBufferManager {
    typedef std::map<std::string,DataBuffer::Ptr> DataBufferMap;
    DataBufferMap _bufferMap;
    QMutex lock_;
public:
    DataBuffer::Ptr get(const char* bufferName);
    void release(const char* bufferName);
};

static DataBufferManager _dataBufferManager;

#endif /* defined(__PowiterOsX__DataBuffer__) */
