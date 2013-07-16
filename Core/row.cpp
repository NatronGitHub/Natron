//  Powiter
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
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


Row::Row():_cacheWillDelete(false){
    buffers = 0;
}

void Row::turnOn(Channel c){
    if( c & _channels) return;
    _channels += c;
    if(c != Channel_alpha)
        buffers[c] = (float*)calloc((r-x),sizeof(float));
    else{
        buffers[c] = (float*)malloc((r-x)*sizeof(float));
        for(int i =0 ;i <  (r-x) ;i++){
            buffers[c][i] = 1.f;
        }
    }
}



Row::Row(int x,int y, int range, ChannelSet channels)
:_cacheWillDelete(false){
    this->x=x;
	this->_y=y;
    this->r=range;
    _channels = channels;
    buffers = (float**)malloc(MAX_BUFFERS_PER_ROW*sizeof(float*));
    memset(buffers, 0, sizeof(float*)*MAX_BUFFERS_PER_ROW);
    
}
bool Row::allocateRow(const char*){
    foreachChannels(z, _channels){
        if(z != Channel_alpha)
            buffers[z] = (float*)calloc((r-x),sizeof(float));
        else{
            buffers[z] = (float*)malloc((r-x)*sizeof(float));
            for(int i =0 ;i <  (r-x) ;i++){
                buffers[z][i] = 1.f;
            }
        }
    }
    return true;
}


void Row::range(int offset,int right){
    
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
}

Row::~Row(){
    foreachChannels(z, _channels){
        if(buffers[(int)z])
            free(buffers[(int)z]);
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
    if(!_cacheWillDelete)
        delete this;
}

