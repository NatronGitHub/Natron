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

#include "Row.h"

#include <algorithm>
#include <cassert>
#include <sstream>
#include <QtCore/QFile>

#include "Engine/Node.h"
#include "Engine/Hash64.h"
#include "Engine/MemoryFile.h"

using namespace std;
using namespace Powiter;

Row::Row():_cacheWillDelete(false){
    buffers = 0;
}

void Row::turnOn(Channel c) {
    if(_channels.contains(c)) return;
    _channels += c;
    if(c != Channel_alpha) {
        buffers[c] = (float*)calloc((r-x),sizeof(float));
        assert(buffers[c]);
    } else {
        buffers[c] = (float*)malloc((r-x)*sizeof(float));
        assert(buffers[c]);
        for(int i =0 ;i <  (r-x) ;++i) {
            buffers[c][i] = 1.f;
        }
    }
}



Row::Row(int x_,int y_, int range, ChannelSet channels)
: _cacheWillDelete(false)
, _y(y_)
, _zoomedY(-1)
, _channels(channels)
, x(x_)
, r(range)
, buffers(NULL)
{
    buffers = (float**)malloc(POWITER_MAX_BUFFERS_PER_ROW*sizeof(float*));
    assert(buffers);
    memset(buffers, 0, sizeof(float*)*POWITER_MAX_BUFFERS_PER_ROW);
    
}
bool Row::allocateRow(const char*){
    size_t dataSize = (r-x)*sizeof(float);
    foreachChannels(z, _channels){
        if(z != Channel_alpha) {
            buffers[z] = (float*)calloc((r-x),sizeof(float));
            assert(buffers[z]);
        } else {
            buffers[z] = (float*)malloc(dataSize);
            assert(buffers[z]);
            for(int i =0 ;i <  (r-x) ;++i) {
                buffers[z][i] = 1.f;
            }
        }
    }
    _size += dataSize*_channels.size();
    return true;
}


void Row::range(int offset,int right){
    
    if((right-offset) < (r-x)) // don't changeSize if the range is smaller
        return;
    r = right;
    x = offset;
    foreachChannels(z, _channels){
        if (buffers[(int)z]) {
            buffers[(int)z] = (float*)realloc(buffers[(int)z],(r-x)*sizeof(float));
        } else {
            buffers[(int)z] = (float*)malloc((r-x)*sizeof(float));
        }
    }
}

Row::~Row(){
    foreachChannels(z, _channels){
        if(buffers[(int)z])
            free(buffers[(int)z]);
    }
    free(buffers);
}

const float* Row::operator[](Channel z) const {
    if (_channels & z) {
        assert(buffers[z]);
        return buffers[z] - x;
    } else {
        return NULL;
    }
}

float* Row::writable(Channel c) {
    if (_channels.contains(c)) {
        assert(buffers[c]);
        return buffers[c] - x;
    } else {
        return NULL;
    }
}

void Row::copy(const Row *source,ChannelSet channels,int o,int r_){
    _channels = channels;
    range(o, r_); // does nothing if the range is smaller
    foreachChannels(z, channels){
        if(!buffers[z]){
            turnOn(z);
        }
        const float* sourcePtr = (*source)[z] + o;
        float* to = buffers[z] -x + o;
        float* end = to + r_;
        while (to != end) {
            *to = *sourcePtr;
            ++to;
            ++sourcePtr;
        }
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

U64 Row::computeHashKey(U64 nodeKey, const std::string& filename, int x , int r, int y){
    Hash64 _hash;
    Hash64_appendQString(&_hash,QString(filename.c_str()));
    _hash.append(nodeKey);
    _hash.append(x);
    _hash.append(r);
    _hash.append(y);
    _hash.computeHash();
    return _hash.value();
}

void Row::release(){
    if(!_cacheWillDelete)
        delete this;
    else
        removeReference();
}

