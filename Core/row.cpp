//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com
#include "Core/row.h"
#include "Core/node.h"

Row::Row(){
    _cached = false;
    buffers = 0;
}

void Row::turnOn(Channel c){
    if( c & _channels) return;
    _channels += c;
    buffers[c] = (float*)malloc((r-x)*sizeof(float));
}



Row::Row(int x,int y, int range, ChannelSet channels)
{
    _cached = false;
    this->x=x;
	this->_y=y;
    this->r=range;
    _channels = channels;
    buffers = (float**)malloc(MAX_BUFFERS_PER_ROW*sizeof(float*));
    memset(buffers, 0, sizeof(float*)*MAX_BUFFERS_PER_ROW);
   
}
void Row::allocate(){
    foreachChannels(z, _channels){
        buffers[z] = (float*)malloc((r-x)*sizeof(float));
    }    
}

void Row::range(int offset,int range){
    //TODO : change the name of the function to reflect its behaviour
    if((range-offset) < (r-x)) // don't changeSize if the range is smaller
        return;
    r = range;
    x = offset;
    foreachChannels(z, _channels){
        buffers[(int)z] = (float*)realloc(buffers[(int)z],(r-x)*sizeof(float));
    }
   
}

Row::~Row(){
    foreachChannels(z, _channels){
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
    if(buffers[c])
        memset(buffers[c], 0, sizeof(float)*(r-x));
}


void Row::get(Node &input,int y,int x,int range,ChannelSet channels){

    
    cout << " NOT YET IMPLEMENTED " << endl;


}
bool compareRows(const Row &a,const Row &b){
    if (a.y()<=b.y()){
        return true;
    }else{
        return false;
    }
}

