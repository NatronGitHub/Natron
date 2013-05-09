//
//  DataBuffer.cpp
//  PowiterOsX
//
//  Created by Alexandre on 1/7/13.
//  Copyright (c) 2013 Alexandre. All rights reserved.
//

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
	const unsigned char* IN = reinterpret_cast<const unsigned char*>(src) ;
	if(!valid())
		return -1;
	size_t extra = 0;
	if(nb_bytes > _size){
		extra = nb_bytes - _size;
		int index=0;
		for(int i=0; i < _size;i+=step){
			_buffer[index++] = IN[i];
		}
	}else{
		int index=0;
		for(int i=0; i < nb_bytes;i+=step){
			_buffer[index++] = IN[i];
		}
	}
	return extra;
}


DataBuffer::Ptr DataBufferManager::get(const char* bufferName){
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


void DataBufferManager::release(const char *bufferName){
	QMutexLocker guard(&lock_);
	DataBufferMap::iterator it = _bufferMap.find(bufferName);

	if (it != _bufferMap.end()) {
		if (it->second.count() == 1) {
			_bufferMap.erase(it);
		}
	}
}
