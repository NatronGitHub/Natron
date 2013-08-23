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

 

 




#ifndef CHANNELS_H
#define CHANNELS_H

#include <string>
#include <cstring>
#include <vector>
#include "Global/GlobalDefines.h"




class ChannelSet{
    U32 mask; // 1 bit per channel and LSB is for "all" channels
    unsigned int _size;
public:
    
    // class iterator: used to iterate over channels
    class iterator{
        Powiter::Channel _cur;
        U32 _mask;
    public:
        iterator(const iterator& other):_cur(other._cur),_mask(other._mask){}
        iterator(U32 mask,Powiter::Channel cur){
            _cur = cur;
            _mask = mask;
        }
        Powiter::Channel operator *() const{return _cur;}
        Powiter::Channel operator->() const{return _cur;}
        void operator++(){
            int i = (int)_cur;
            while(i < 32){
                if(_mask & (1 << i)){
                    _cur = (Powiter::Channel)i;
                    return;
                }
                ++i;
            }
            _cur = Powiter::Channel_black;
        }
        void operator++(int){
            int i = (int)_cur;
            while(i < 32){
                if(_mask & (1 << i)){
                    _cur = (Powiter::Channel)i;
                    return;
                }
                ++i;
            }
        }
        void operator--(){
            int i = (int)_cur;
            while(i >= 0){
                if(_mask & (1 << i)){
                    _cur = (Powiter::Channel)i;
                    return;
                }
                --i;
            }
            _cur = Powiter::Channel_black;
        }
        void operator--(int){
            int i = (int)_cur;
            while(i >= 0){
                if(_mask & (1 << i)){
                    _cur = (Powiter::Channel)i;
                    return;
                }
                --i;
            }
            _cur = Powiter::Channel_black;
        }
        bool operator==(const iterator& other){
            return _mask==other._mask && _cur==other._cur;
        }
        bool operator!=(const iterator& other){
            return _mask!=other._mask || _cur!=other._cur;
        }
    };
    typedef iterator const_iterator;
    
    
    ChannelSet() : mask(0),_size(0) {}
    ChannelSet(const ChannelSet &source);
    ChannelSet(Powiter::ChannelMask v);
    ChannelSet(Powiter::Channel v) : _size(1) {mask = v;}
    ~ChannelSet() {}
    const ChannelSet& operator=(const ChannelSet& source);
    const ChannelSet& operator=(Powiter::ChannelMask source) ;
    const ChannelSet& operator=(Powiter::Channel z);
    
    void clear() { mask = 0; _size = 0;}
    operator bool() const { return mask ; }// allow to do stuff like if(channelsA & channelsB)
    bool empty() const { return !mask; }
    
    bool operator==(const ChannelSet& source) const;
    bool operator!=(const ChannelSet& source) const { return !(*this == source); }
    bool operator<(const ChannelSet& source) const;
    bool operator==(Powiter::ChannelMask v) const { return mask == U32(v << 1); }
    bool operator!=(Powiter::ChannelMask v) const { return !(*this == (v << 1)); }
    bool operator==(Powiter::Channel z) const;
    bool operator!=(Powiter::Channel z) const { return !(*this == z); }
    
    /*add a channel with +=*/
    void operator+=(const ChannelSet& source);
    void operator+=(Powiter::ChannelMask source);
    void operator+=(Powiter::Channel z);
    void insert(Powiter::Channel z) { *this += z; }
    
    /*turn off a channel with -=*/
    void operator-=(const ChannelSet& source);
    void operator-=(Powiter::ChannelMask source);
    void operator-=(Powiter::Channel z);
    void erase(Powiter::Channel z) { *this -= z; }
    
    void operator&=(const ChannelSet& source);
    void operator&=(Powiter::ChannelMask source) ;
    void operator&=(Powiter::Channel z);
    
    
    ChannelSet operator&(const ChannelSet& c) const;
    ChannelSet operator&(Powiter::ChannelMask c) const;
    ChannelSet operator&(Powiter::Channel z) const;
    bool contains(const ChannelSet& source) const;
    bool contains(Powiter::ChannelMask source) const { return !(~mask & source); }
    bool contains(Powiter::Channel z) const;
    unsigned size() const;
    Powiter::Channel first() const;
    Powiter::Channel next(Powiter::Channel k) const;
    Powiter::Channel last() const;
    Powiter::Channel previous(Powiter::Channel k) const;
    
    U32 value() const {return mask;}
    
    iterator begin(){return iterator(mask,first());}
    
    iterator end(){
        Powiter::Channel _last = this->last();
        return iterator(mask,(Powiter::Channel)(_last+1));
    }
    void printOut() const;
    
#define foreachChannels(CUR, CHANNELS) \
    for (Powiter::Channel CUR = CHANNELS.first(); CUR; CUR = CHANNELS.next(CUR))


        };



/*returns the channel of name "name"*/
Powiter::Channel getChannelByName(const char *name);

/*Return a cstring with the name of the channel c*/
std::string getChannelName(Powiter::Channel c);

/*useful function to check whether alpha is on in the mask*/
bool hasAlpha(ChannelSet mask);


#endif // CHANNELS_H
