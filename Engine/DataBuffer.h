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

#ifndef POWITER_ENGINE_DATABUFFER_H_
#define POWITER_ENGINE_DATABUFFER_H_

#include <map>
#include <string>
#include <QtCore/QMutex>

#include "Engine/ReferenceCountedObject.h"

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
    ssize_t fillBuffer(const void* src,size_t nb_bytes,size_t step=1);

    bool valid(){return _buffer!=NULL;}

    unsigned char* buffer(){return _buffer;}

	bool isZero(){
		bool res=true;
		unsigned int i=0;
		while(i<_size){
			if((int)_buffer[i] != 0) res = false;
			++i;
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
    DataBuffer::Ptr get(const std::string& bufferName);

	/*Reduces the count of bufferName by 1.
	 *If it was the only reference left,
	 *it effectivly delete bufferName.*/
    void release(const std::string& bufferName);
};

static DataBufferManager _dataBufferManager;

#endif /* defined(POWITER_ENGINE_DATABUFFER_H_) */
