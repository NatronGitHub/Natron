//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com
#include "Core/row.h"
#include "Core/node.h"
#include "Core/hash.h"
#include "Core/mappedfile.h"
#include <algorithm>
#include <cassert>
#include <sstream>
#include <QtCore/QFile>

using namespace std;
ChannelsToFile::ChannelsToFile(ChannelSet& mapping,int x, int r,size_t totalSize):_totalSize(totalSize){
    assert(mapping.size()*(r-x)*sizeof(float) == totalSize);
    size_t offset = 0;
    size_t channelSize = (r-x)*sizeof(float);
    foreachChannels(z, mapping){
        _mapping.insert(make_pair(z,make_pair(offset,channelSize)));
        offset += channelSize;
    }
}

std::string ChannelsToFile::printMapping(){
    ostringstream oss;
    for(MappingIterator it = begin();it!=_mapping.end();it++){
        oss << getChannelName(it->first);
        oss << " ";
    }
    return  oss.str();
}

Row::Row(Powiter_Enums::RowStorageMode mode ){
    buffers = 0;
    _channelsToFileMapping = 0;
    _storageMode = mode;
}

void Row::turnOn(Channel c){
    if(_storageMode == IN_MEMORY){
        if( c & _channels) return;
        _channels += c;
        buffers[c] = (float*)malloc((r-x)*sizeof(float));
    }else{
        cout << "Row::turnOn currently not supported when backed by a file" << endl;
    }
}



Row::Row(int x,int y, int range, ChannelSet channels,Powiter_Enums::RowStorageMode mode)
{
    this->x=x;
	this->_y=y;
    this->r=range;
    _channels = channels;
    _storageMode = mode;
    _channelsToFileMapping = 0;
    buffers = (float**)malloc(MAX_BUFFERS_PER_ROW*sizeof(float*));
    memset(buffers, 0, sizeof(float*)*MAX_BUFFERS_PER_ROW);
    
}
bool Row::allocate(const char* path){
    if(_storageMode == IN_MEMORY){
        foreachChannels(z, _channels){
            buffers[z] = (float*)calloc((r-x),sizeof(float));
        }
        return true;
    }else{
        size_t size = _channels.size() * (r-x) * sizeof(float);
        if(!MemoryMappedEntry::allocate(size,path)) return false;
        _channelsToFileMapping = new ChannelsToFile(_channels,x,r,_mappedFile->size());
        ChannelsToFile::MappingIterator it = _channelsToFileMapping->begin();
        char* data = _mappedFile->data();
        for(;it!=_channelsToFileMapping->end();it++){
            buffers[it->first] = reinterpret_cast<float*>(data+it->second.first);
        }
        return true;
    }
}

bool Row::restoreMapping(){
    if(_mappedFile) return true;
    
    /*re-open the mapping*/
    if(!MemoryMappedEntry::reOpen()) return false;
    
    ChannelsToFile::MappingIterator it = _channelsToFileMapping->begin();
    char* data = _mappedFile->data();
    for(;it!=_channelsToFileMapping->end();it++){
        buffers[it->first] = reinterpret_cast<float*>(data+it->second.first);
    }
    return true;
}

void Row::range(int offset,int right){
    if(_storageMode == IN_MEMORY){
        if((right-offset) < (r-x)) // don't changeSize if the range is smaller
            return;
        r = right;
        x = offset;
        foreachChannels(z, _channels){
            if(buffers[(int)z])
                buffers[(int)z] = (float*)realloc(buffers[(int)z],(r-x)*sizeof(float));
            else
                buffers[(int)z] = (float*)malloc((r-x)*sizeof(float));
        }
    }else{
        cout << "Row::range currently not supported when backed by a file" << endl;
    }
}

Row::~Row(){
    if(_storageMode == IN_MEMORY){
        foreachChannels(z, _channels){
            if(buffers[(int)z])
                free(buffers[(int)z]);
        }
    }else{
        delete _channelsToFileMapping;
        
    }
    free(buffers);
}
const float* Row::operator[](Channel z) const{
    return _channels & z ?  buffers[z] - x : NULL;
}

float* Row::writable(Channel c){
    return _channels & c ?  buffers[c] - x : NULL;
}
void Row::copy(const Row *source,ChannelSet channels,int o,int r){
    _channels = channels;
    range(o, r); // does nothing if the range is smaller
    foreachChannels(z, channels){
        if(!buffers[z]){
            turnOn(z);
        }
        const float* sourcePtr = (*source)[z] + o;
        float* to = buffers[z] -x + o;
        float* end = to + r;
        while(to!=end) *to++ = *sourcePtr++;
    }
}
void Row::erase(Channel c){
    if(buffers[c]){
        memset(buffers[c], 0, sizeof(float)*(r-x));
    }
}


bool compareRows(const Row &a,const Row &b){
    if (a.y()<=b.y()){
        return true;
    }else{
        return false;
    }
}

U64 Row::computeHashKey(U64 nodeKey,std::string filename, int x , int r, int y){
    Hash _hash;
    _hash.appendQStringToHash(QString(filename.c_str()));
    _hash.appendNodeHashToHash(nodeKey);
    _hash.appendNodeHashToHash(r-x);
    _hash.appendNodeHashToHash(y);
    _hash.computeHash();
    return _hash.getHashValue();
}

void Row::release(){
    if(_storageMode == IN_MEMORY){
        delete this;
    }
}

std::string Row::printOut(){
    ostringstream oss;
    oss << _path;
    oss << "<";
    char tmp[50];
    sprintf(tmp,"%i",x);
    oss << tmp;
    oss << ".";
    sprintf(tmp, "%i",r);
    oss << tmp;
    oss << ".";
    sprintf(tmp,"%i",y());
    oss << tmp;
    oss << ".";
    oss << _channelsToFileMapping->printMapping();
    oss << endl;
    return oss.str();
}

std::pair<U64,Row*> Row::recoverFromString(QString str){
    if(str.isEmpty()) return make_pair(0,(Row*)NULL);
    QString path,xStr,yStr,rStr,channelsMapping;
    int i =0;
    while(str.at(i) != QChar('<')){path.append(str.at(i)); i++;}
    i++;
    while(str.at(i) != QChar('.')){xStr.append(str.at(i)); i++;}
    i++;
    while(str.at(i) != QChar('.')){rStr.append(str.at(i)); i++;}
    i++;
    while(str.at(i) != QChar('.')){yStr.append(str.at(i)); i++;}
    i++;
    while(i < str.size()){channelsMapping.append(str.at(i)); i++;}
    ChannelSet channels;
    int c = 0;
    while(c < channelsMapping.size()){
        QString chanStr;
        while(channelsMapping.at(c) != QChar(' ')){chanStr.append(channelsMapping.at(c)); c++;}
        c++;
        string chanStdStr = chanStr.toStdString();
        channels += getChannelByName(chanStdStr.c_str());
    }
    if (!QFile::exists(path)) {
        cout << "Invalid entry : " << qPrintable(path) << endl;
        return make_pair(0,(Row*)NULL);
    }
    
    Row* out = new Row(xStr.toInt(),yStr.toInt(),rStr.toInt(),channels,Powiter_Enums::BACKED_ON_DISK);
    string pathStr = path.toStdString();
    if(!out->allocate(pathStr.c_str())){
        QFile::remove(path);
        cout << "Invalid entry : " << qPrintable(path) << endl;
        return make_pair(0,(Row*)NULL);
    }
    QString hashKey;
    int j = path.size() - 1;
    while(path.at(j) != QChar('.')) j--;
    j--;
    while(path.at(j) != QChar('/')){hashKey.append(path.at(j));j--;}
    U64 key = hashKey.toULongLong();
    
    return make_pair(key,out);
}