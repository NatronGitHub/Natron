//  Powiter
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
//  contact: immarespond at gmail dot com

#ifndef CHANNELS_H
#define CHANNELS_H

#include <string>
#include <cstring>
//#include <boost/foreach.hpp>
#include <QtCore/QForeachContainer>
#include <vector>
#include "Superviser/powiterFn.h"
#undef foreach 
#define foreach Q_FOREACH

using namespace Powiter_Enums;
using namespace std;


class ChannelSet{
    U32 mask; // 1 bit per channel and bit 0 is for "all" channels
    unsigned int _size;
public:
    
    // class iterator: used to iterate over channels
    class iterator{
        Channel _cur;
        U32 _mask;
    public:
        iterator(const iterator& other):_cur(other._cur),_mask(other._mask){}
        iterator(U32 mask,Channel cur){
            _cur = cur;
            _mask = mask;
        }
        Channel operator *() const{return _cur;}
        Channel operator->() const{return _cur;}
        void operator++(){
            int i = (int)_cur;
            while(i < 32){
                if(_mask & (1 << i)){
                    _cur = (Channel)i;
                    return;
                }
                i++;
            }
            _cur = Channel_black;
        }
        void operator++(int){
            int i = (int)_cur;
            while(i < 32){
                if(_mask & (1 << i)){
                    _cur = (Channel)i;
                    return;
                }
                i++;
            }
        }
        void operator--(){
            int i = (int)_cur;
            while(i >= 0){
                if(_mask & (1 << i)){
                    _cur = (Channel)i;
                    return;
                }
                i--;
            }
            _cur = Channel_black;
        }
        void operator--(int){
            int i = (int)_cur;
            while(i >= 0){
                if(_mask & (1 << i)){
                    _cur = (Channel)i;
                    return;
                }
                i--;
            }
            _cur = Channel_black;
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
    ChannelSet(ChannelSetInit v);
    ChannelSet(Channel v) : _size(1) {mask = v;}
    ~ChannelSet() {}
    const ChannelSet& operator=(const ChannelSet& source);
    const ChannelSet& operator=(ChannelSetInit source) ;
    const ChannelSet& operator=(Channel z);
    
    void clear() { mask = 0; _size = 0;}
    operator bool() const { return mask ; }// allow to do stuff like if(channelsA & channelsB)
    bool empty() const { return !mask; }
    
    bool operator==(const ChannelSet& source) const;
    bool operator!=(const ChannelSet& source) const { return !(*this == source); }
    bool operator<(const ChannelSet& source) const;
    bool operator==(ChannelSetInit v) const { return mask == U32(v); }
    bool operator!=(ChannelSetInit v) const { return !(*this == v); }
    bool operator==(Channel z) const;
    bool operator!=(Channel z) const { return !(*this == z); }
    
    /*add a channel with +=*/
    void operator+=(const ChannelSet& source);
    void operator+=(ChannelSetInit source);
    void operator+=(Channel z);
    void insert(Channel z) { *this += z; }
    
    /*turn off a channel with -=*/
    void operator-=(const ChannelSet& source);
    void operator-=(ChannelSetInit source);
    void operator-=(Channel z);
    void erase(Channel z) { *this -= z; }
    
    void operator&=(const ChannelSet& source);
    void operator&=(ChannelSetInit source) ;
    void operator&=(Channel z);
    
    
    ChannelSet operator&(const ChannelSet& c) const;
    ChannelSet operator&(ChannelSetInit c) const;
    ChannelSet operator&(Channel z) const;
    bool contains(const ChannelSet& source) const;
    bool contains(ChannelSetInit source) const { return !(~mask & source); }
    bool contains(Channel z) const;
    unsigned size() const;
    Channel first() const;
    Channel next(Channel k) const;
    Channel last() const;
    Channel previous(Channel k) const;
    
    U32 value() const {return mask;}
    
    iterator begin(){return iterator(mask,first());}
    
    iterator end(){
        Channel _last = this->last();
        return iterator(mask,(Channel)(_last+1));
    }
    void printOut();
    
#define foreachChannels(CUR, CHANNELS) \
    for (Channel CUR = CHANNELS.first(); CUR; CUR = CHANNELS.next(CUR))


        };
typedef ChannelSet ChannelMask;


/*returns the channel of name "name"*/
Channel getChannelByName(const char *name);

/*Return a cstring with the name of the channel c*/
std::string getChannelName(Channel c);

/*useful function to check whether alpha is on in the mask*/
bool hasAlpha(ChannelMask mask);


#endif // CHANNELS_H
