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

 

 

#include "Core/ChannelSet.h"
#include <iostream>
using namespace std;
using namespace Powiter;
Channel getChannelByName(const char *name){
    if(strcmp(name,"Channel_unused")==0){
        return Channel_unused;
    }else if(strcmp(name,"Channel_red")==0){
        return Channel_red;
    }else if(strcmp(name,"Channel_green")==0){
        return Channel_green;
    }else if(strcmp(name,"Channel_blue")==0){
        return Channel_blue;
    }else if(strcmp(name,"Channel_alpha")==0){
        return Channel_alpha;
    }else if(strcmp(name,"Channel_Z")==0){
        return Channel_Z;
    }else{
        throw std::string("(getChannelByName):Bad channel name");
    }
}
std::string getChannelName(Channel c){
    if(c==Channel_red){
        return "Channel_red";
    }else if(c==Channel_green){
        return "Channel_green";
    }else if(c==Channel_blue){
        return "Channel_blue";
        
    }else if(c==Channel_alpha){
        return "Channel_alpha";
        
    }else if(c==Channel_unused){
        return "Channel_unused";
        
    }else if(c==Channel_Z){
        return "Channel_Z";
        
    }else if(c==Channel_black){
        return "Channel_black";
    }
    return "";
}

ChannelSet::ChannelSet(const ChannelSet &source):mask(source.mask),_size(source.size()){}
ChannelSet::ChannelSet(ChannelMask v) {
    if(v == Mask_All){
        mask = Mask_All;
        _size = MAX_CHANNEL_COUNT;
        return;
    }
    mask = (v << 1);
    _size=0;
    for(unsigned int i = 1; i < 32 ; i++){
        if(mask & (1 << i)) _size++;
    }
}

const ChannelSet& ChannelSet::operator=(const ChannelSet& source){
    
    clear();
    foreachChannels(z, source){
            mask |= (1u << z);
    }
    _size = source.size();
    return *this;
}

const ChannelSet& ChannelSet::operator=(ChannelMask source) {
    if(source == Mask_All){
        mask = Mask_All;
        _size = MAX_CHANNEL_COUNT;
        return *this;
    }
    mask = (source << 1);
    _size = 0;
    for(unsigned int i = 1; i < 32 ; i++) {
        if(mask & (1 << i))
            _size++;
    }
    return *this;
}

const ChannelSet& ChannelSet::operator=(Channel z){
    clear();
    if(z < 31){
        mask |= (1u << z);
    }
    _size = 1;
    return *this;
}

bool ChannelSet::operator==(const ChannelSet& source) const{
    if(size() != source.size()) return false;
    Channel z = source.first();
    Channel thisZ = first();
    if(z != thisZ)
        return false;
    else{
        for(unsigned int i =1 ; i < source.size(); i++){
            z = source.next(z);
            thisZ = next(thisZ);
            if(z != thisZ)
                return false;
        }
        return true;
    }
}
bool ChannelSet::operator<(const ChannelSet& source) const{
    if(size() < source.size()) return true;
    else if(size() > source.size()) return false;
    else{
        int sum = 0;
        int thisSum = 0;
        Channel z= source.first();
        sum+=z;
        Channel thisZ = first();
        thisSum+=thisZ;
        for(unsigned int i =1 ; i < source.size(); i++){
            z = source.next(z);
            thisZ = next(thisZ);
            sum+=z;
            thisSum+=thisZ;
        }
        if(thisSum < sum ) return true;
        else return false;
    }
    return false;
}
bool ChannelSet::operator==(Channel z) const{
    if(size() == 1){
        return first() == z;
    }else
        return false;
}
void ChannelSet::operator+=(const ChannelSet& source){
    if(mask & 1){ // mask all
        return;
    }
    Channel z = source.first();
    *this+=z;
    for(unsigned i = 1; i < source.size();i++){
        z=source.next(z);
        *this+=z;
    }
    
}
void ChannelSet::operator+=(ChannelMask source) {
    if(mask & 1){ // mask all
        return;
    }
    if((source & Mask_All) == Mask_All){// mask all
        _size = MAX_CHANNEL_COUNT;
        mask = 1;
        return;
    }
    _size = 0;
    mask |= (source << 1);//shift from 1 bit on the left because the first bit is reserved for the Mask_all
    for(unsigned int i = 1; i < 32 ; i++){
        if(mask & (1 << i)) _size++;
    }
}


void ChannelSet::operator+=(Channel z){
    if((mask & 1) || z == Channel_black){ // mask all or channel black
        return;
    }
    if(!contains(z)){
        if(z < 31){
            mask |= (1 << z);
        }
        _size++;
    }
}
void ChannelSet::operator-=(const ChannelSet& source){
    if(source.value() & 1){
        clear();
        return;
    }
    Channel z = source.first();
    *this-=z;
    for(unsigned i = 0; i < source.size();i++){
        z=source.next(z);
        *this-=z;
    }
}
void ChannelSet::operator-=(ChannelMask source) {
    if(source & 1){
        clear();
        return;
    }
    mask &= ~source;
    _size = 0;
    for(unsigned int i = 1; i < 32 ; i++){
        if(mask & (1 << i)) _size++;    }
}


void ChannelSet::operator-=(Channel z){
    if(z < 31 && *this&z){// if it is a valid channel and it is effectivly contained
        mask &= ~1; // removing the flag all if it was set
        mask &= ~(1 << z); // setting to 0 the channel z
        _size--; // decrementing channels count
    }
    
}
void ChannelSet::operator&=(const ChannelSet& source){
    mask &= source.value();
    if(source.value() & 1){
        _size = MAX_CHANNEL_COUNT;
    }else{
        _size = 0;
        for(unsigned int i = 1; i < 32 ; i++){
            if(mask & (1 << i)) _size++;
        }
        
    }
}
void ChannelSet::operator&=(ChannelMask source) {
    mask &= (source << 1);
    if(source & 1){
        _size = MAX_CHANNEL_COUNT;
    }else{
        _size = 0;
        for(unsigned int i = 1; i < 32 ; i++){
            if(mask & (1 << i)) _size++;
        }
    }
}


void ChannelSet::operator&=(Channel z){
    mask &= (1 << z);
    _size = 0;
    for(unsigned int i = 1; i < 32 ; i++){
        if(mask & (1 << i)) _size++;
    }
}
ChannelSet ChannelSet::operator&(const ChannelSet& c) const{
    ChannelSet ret;
    ret = *this;
    ret&=c;
    return ret;
}

ChannelSet ChannelSet::operator&(ChannelMask c) const{
    ChannelSet ret;
    ret = *this;
    ret&=c;
    return ret;
}

ChannelSet ChannelSet::operator&(Channel z) const{
    ChannelSet ret;
    ret = *this;
    ret&=z;
    return ret;
}
bool ChannelSet::contains(const ChannelSet& source) const{
    foreachChannels(z, source){
        if(!contains(z))
            return false;
    }
    return true;
}
bool ChannelSet::contains(Channel z) const{
    return (mask & (1 << z));
}
unsigned ChannelSet::size() const{
    return _size;
}
Channel ChannelSet::first() const{
    int i =1;
    while(i < 32){
        if(mask & (1 << i)){
            return (Channel)i;
        }
        i++;
    }
    return Channel_black;
}
Channel ChannelSet::next(Channel k) const{
    int i = (int)k+1;
    while(i < 32){
        if(mask & (1 << i)){
            return (Channel)i;
        }
        i++;
    }
    return Channel_black;
}
Channel ChannelSet::last() const{
    int i =31;
    while(i >=0){
        if(mask & (1 << i))
            return (Channel)i;
        i--;
    }
    return Channel_black;
}
Channel ChannelSet::previous(Channel k) const{
    int i = (int)k; i--;
    while(i >= 0){
        if(mask & (1 << i)){
            return (Channel)i;
        }
        i--;
    }
    return Channel_black;
}

void ChannelSet::printOut() const{
    std::cout << "ChannelSet is ..." << std::endl;
    for (Channel CUR = first(); CUR; CUR = next(CUR)){
        std::cout << getChannelName(CUR) << endl;
    }
}

bool hasAlpha(ChannelSet mask){
    return (mask & (1 << Channel_alpha));
}

