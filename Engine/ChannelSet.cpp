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

#include "ChannelSet.h"

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <cassert>

using namespace Powiter;
using std::endl;

// variable-precision SWAR algorithm
// see http://stackoverflow.com/questions/109023/how-to-count-the-number-of-set-bits-in-a-32-bit-integer
static int NumberOfSetBits(U32 i)
{
    i = i - ((i >> 1) & 0x55555555);
    i = (i & 0x33333333) + ((i >> 2) & 0x33333333);
    return (((i + (i >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
}

Channel getChannelByName(const std::string &name) {
    if (name == "Channel_red") {
        return Channel_red;
    }
    if (name == "Channel_green") {
        return Channel_green;
    }
    if (name == "Channel_blue") {
        return Channel_blue;
    }
    if (name == "Channel_alpha") {
        return Channel_alpha;
    }
    if (name == "Channel_Z") {
        return Channel_Z;
    }
    std::string c("Channel_");
    if (name.compare(0, c.length(), c) == 0) {
        int id;
        std::istringstream iss(name.substr(c.length(), name.length()-c.length()));
        if (iss >> id) {
            return (Channel)id;
        }
    }
    throw std::runtime_error("(getChannelByName):Bad channel name");
}

std::string getChannelName(Channel c) {
    switch (c) {
        case Channel_black:
            return "Channel_black";
        case Channel_red:
            return "Channel_red";
        case Channel_green:
            return "Channel_green";
        case Channel_blue:
            return "Channel_blue";
        case Channel_alpha:
            return "Channel_alpha";
        case Channel_Z:
            return "Channel_Z";
        case Channel_unused:
            ; // do the default action below
    }
    std::ostringstream s;
    s << "Channel_" << (int)c;
    return s.str();
}

ChannelSet::ChannelSet(const ChannelSet &source)
: mask(source.mask)
, _size(source.size())
{
}

const ChannelSet& ChannelSet::operator=(const ChannelSet& source){
    mask = source.value();
    _size = source.size();
    return *this;
}

const ChannelSet& ChannelSet::operator=(ChannelMask source) {
    if (source == Mask_All) {
        mask = 0xFFFFFFFF;
        _size = POWITER_MAX_CHANNEL_COUNT;
        return *this;
    }
    mask = (source << 1);
    _size = NumberOfSetBits(mask);
    assert(_size <= POWITER_MAX_CHANNEL_COUNT);
    return *this;
}

const ChannelSet& ChannelSet::operator=(Channel z) {
    clear();
    assert(1 <= (int)z && (int)z < 32); // a channelset cannot contain channel 0 (black)
    // Nuke has a U32* other field in its ChannelSet to handle more than 32 channels, but we don't
    if ((int)z < 32) {
        mask |= (1u << z);
    }
    _size = 1;
    return *this;
}

bool ChannelSet::operator==(const ChannelSet& source) const{
    if(size() != source.size()) {
        return false;
    }
    Channel z = source.first();
    Channel thisZ = first();
    if (z != thisZ) {
        return false;
    } else {
        for (unsigned int i =1 ; i < source.size(); ++i) {
            z = source.next(z);
            thisZ = next(thisZ);
            if (z != thisZ) {
                return false;
            }
        }
        return true;
    }
}
bool ChannelSet::operator<(const ChannelSet& source) const {
    if (size() < source.size()) {
        return true;
    } else if(size() > source.size()) {
        return false;
    } else {
        int sum = 0;
        int thisSum = 0;
        Channel z= source.first();
        sum+=z;
        Channel thisZ = first();
        thisSum+=thisZ;
        for(unsigned int i =1 ; i < source.size(); ++i) {
            z = source.next(z);
            thisZ = next(thisZ);
            sum+=z;
            thisSum+=thisZ;
        }
        if (thisSum < sum ) {
            return true;
        } else {
            return false;
        }
    }
    return false;
}

bool ChannelSet::operator==(Channel z) const {
    return (size() == 1 && first() == z);
}

void ChannelSet::operator+=(const ChannelSet& source) {
    if(mask & 1) { // mask all
        return;
    }
    Channel z = source.first();
    if (z == Channel_black) {
        return;
    }
    *this += z;
    for(unsigned i = 1; i < source.size();++i) {
        z=source.next(z);
        if (z == Channel_black) {
            return;
        }
        *this+=z;
    }
    
}

void ChannelSet::operator+=(ChannelMask source) {
    if(mask & 1){ // mask all
        return;
    }
    if(source == Mask_All) {// mask all
        _size = POWITER_MAX_CHANNEL_COUNT;
        mask = 0xffffffff;
        return;
    }
    mask |= (source << 1);//shift from 1 bit on the left because the first bit is reserved for the Mask_all
    _size = NumberOfSetBits(mask);
    assert(_size <= POWITER_MAX_CHANNEL_COUNT);
}


void ChannelSet::operator+=(Channel z) {
    assert(1 <= (int)z && (int)z < 32); // a channelset cannot contain channel 0 (black)
    if (mask & 1) { // mask all
        return;
    }
    if (!contains(z)) {
        if (1 <= (int)z && (int)z < 32) {
            mask |= (1 << z);
        }
        ++_size;
    }
    assert(_size <= POWITER_MAX_CHANNEL_COUNT);
}

void ChannelSet::operator-=(const ChannelSet& source) {
    if (source.value() & 1) { // "all" bit
        clear();
        return;
    }
    Channel z = source.first();
    if (z == Channel_black) { // empty source
        return;
    }
    assert(1 <= (int)z && (int)z < 32); // a channelset cannot contain channel 0 (black)
    *this-=z;
    for(unsigned i = 0; i < source.size();++i) {
        z = source.next(z);
        if (z == Channel_black) { // end of channels
            return;
        }
        assert(1 <= (int)z && (int)z < 32); // a channelset cannot contain channel 0 (black)
        *this -= z;
    }
    assert(_size <= POWITER_MAX_CHANNEL_COUNT);
}

void ChannelSet::operator-=(ChannelMask source) {
    if (source == Mask_All) {
        clear();
        return;
    }
    if (source == Mask_None) {
        return;
    }
    if (mask & 1) {
        mask = ~(U32)source << 1;
    } else {
        mask &= ~(U32)source << 1;
    }
    _size = NumberOfSetBits(mask);
    assert(_size <= POWITER_MAX_CHANNEL_COUNT);
}


void ChannelSet::operator-=(Channel z) {
    assert(1 <= (int)z && (int)z < 32); // a channelset cannot contain channel 0 (black)
    if (1 <= (int)z && (int)z < 32 && *this&z) {// if it is a valid channel and it is effectivly contained
        mask &= ~1U; // removing the flag all if it was set
        mask &= ~(1U << z); // setting to 0 the channel z
        --_size; // decrementing channels count
    }
    assert(_size <= POWITER_MAX_CHANNEL_COUNT);
}

void ChannelSet::operator&=(const ChannelSet& source) {
    if (source.value() & 1) { // "all" bit is on
        return;
    }
    mask &= source.value();
    _size = NumberOfSetBits(mask);
    assert(_size <= POWITER_MAX_CHANNEL_COUNT);
}

void ChannelSet::operator&=(ChannelMask source) {
    if (source == Mask_All) {
        return;
    }
    mask &= (source << 1);
    _size = NumberOfSetBits(mask);
    assert(_size <= POWITER_MAX_CHANNEL_COUNT);
}


void ChannelSet::operator&=(Channel z){
    assert(1 <= (int)z && (int)z < 32); // a channelset cannot contain channel 0 (black)
    mask &= (1 << z);
    _size = NumberOfSetBits(mask);
}

ChannelSet ChannelSet::operator&(const ChannelSet& c) const {
    ChannelSet ret(*this);
    ret &= c;
    return ret;
}

ChannelSet ChannelSet::operator&(ChannelMask c) const {
    ChannelSet ret(*this);
    ret &= c;
    return ret;
}

ChannelSet ChannelSet::operator&(Channel z) const {
    assert(1 <= (int)z && (int)z < 32); // a channelset cannot contain channel 0 (black)
    ChannelSet ret(*this);
    ret &= z;
    return ret;
}

bool ChannelSet::contains(const ChannelSet& source) const {
    if (mask & 1) { // "all" bit is set
        return true;
    }
    foreachChannels(z, source){
        if(!contains(z))
            return false;
    }
    return true;
}

bool ChannelSet::contains(Channel z) const {
    assert(1 <= (int)z && (int)z < 32); // a channelset cannot contain channel 0 (black)
    return ((mask & 1) || (mask & (1 << z)));
}

unsigned ChannelSet::size() const {
    return _size;
}

Channel ChannelSet::first() const {
    int i =1;
    while(i < 32){
        if(mask & (1 << i)){
            return (Channel)i;
        }
        ++i;
    }
    return Channel_black;
}

Channel ChannelSet::next(Channel k) const{
    assert(1 <= (int)k && (int)k < 32); // a channelset cannot contain channel 0 (black)
    int i = (int)k+1;
    while(i < 32){
        if(mask & (1 << i)){
            return (Channel)i;
        }
        ++i;
    }
    return Channel_black;
}

Channel ChannelSet::last() const {
    int i = 31;
    while(i >=0){
        if(mask & (1 << i))
            return (Channel)i;
        --i;
    }
    return Channel_black;
}

Channel ChannelSet::previous(Channel k) const {
    assert(1 <= (int)k && (int)k < 32); // a channelset cannot contain channel 0 (black)
    int i = (int)k-1;
    while (i >= 0) {
        if(mask & (1 << i)){
            return (Channel)i;
        }
        --i;
    }
    return Channel_black;
}

void ChannelSet::printOut() const{
    std::cout << "ChannelSet is ..." << std::endl;
    for (Channel CUR = first(); CUR != Channel_black; CUR = next(CUR)){
        std::cout << getChannelName(CUR) << endl;
    }
}

bool hasAlpha(ChannelSet mask) {
    return ((mask & 1) || (mask & (1 << Channel_alpha)));
}

