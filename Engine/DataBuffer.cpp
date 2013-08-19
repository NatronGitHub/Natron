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

 

 




#include "Superviser/GlobalDefines.h"
#include "Core/DataBuffer.h"
DataBuffer::DataBuffer(){
	this->_lock = new QMutex();
	_buffer=NULL;
}
DataBuffer::~DataBuffer(){

	{
		QMutexLocker guard(_lock);
		if(_buffer!=NULL)
		free(_buffer);
	}
	delete _lock;

}

void DataBuffer::allocate(size_t nb_bytes){
	QMutexLocker guard(_lock);

	_buffer = (unsigned char*)malloc(nb_bytes);
	_size = nb_bytes;
}
void DataBuffer::resize(size_t nb_bytes){
	QMutexLocker guard(_lock);

	free(_buffer);
	_buffer = (unsigned char*)malloc(nb_bytes);
	_size = nb_bytes;
}
void DataBuffer::clear(){
	QMutexLocker guard(_lock);
	memset(_buffer,0,_size);
}

size_t DataBuffer::fillBuffer(const void* src,size_t nb_bytes,size_t step){
	QMutexLocker guard(_lock);
	const unsigned char* inbuffer = reinterpret_cast<const unsigned char*>(src) ;
	if(!valid())
		return -1;
	size_t extra = 0;
	if(nb_bytes > _size){
		extra = nb_bytes - _size;
		int index=0;
		for(U32 i=0; i < _size;i+=step){
			_buffer[index++] = inbuffer[i];
		}
	}else{
		int index=0;
		for(U32 i=0; i < nb_bytes;i+=step){
			_buffer[index++] = inbuffer[i];
		}
	}
	return extra;
}


DataBuffer::Ptr DataBufferManager::get(const std::string& bufferName){
	QMutexLocker guard(&lock_);
	DataBuffer::Ptr retVal;

	DataBufferMap::iterator it = _bufferMap.find(bufferName);
	if (it == _bufferMap.end()) {
		retVal.allocate();
		_bufferMap.insert(std::make_pair(std::string(bufferName), retVal));
	}
	else {
		retVal = it->second;
	}
	return retVal;


}


void DataBufferManager::release(const std::string& bufferName){
	QMutexLocker guard(&lock_);
	DataBufferMap::iterator it = _bufferMap.find(bufferName);

	if (it != _bufferMap.end()) {
		if (it->second.count() == 1) {
			_bufferMap.erase(it);
		}
	}
}
