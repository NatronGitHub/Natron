//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
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




class ChannelSet{
    U32 mask; // 1 bit per channel and bit 0 is for "all" channels
    unsigned int _size;
public:
    
    // class iterator: used to iterate over channels
    class iterator{
        Powiter_Enums::Channel _cur;
        U32 _mask;
    public:
        iterator(const iterator& other):_cur(other._cur),_mask(other._mask){}
        iterator(U32 mask,Powiter_Enums::Channel cur){
            _cur = cur;
            _mask = mask;
        }
        Powiter_Enums::Channel operator *() const{return _cur;}
        Powiter_Enums::Channel operator->() const{return _cur;}
        void operator++(){
            int i = (int)_cur;
            while(i < 32){
                if(_mask & (1 << i)){
                    _cur = (Powiter_Enums::Channel)i;
                    return;
                }
                i++;
            }
            _cur = Powiter_Enums::Channel_black;
        }
        void operator++(int){
            int i = (int)_cur;
            while(i < 32){
                if(_mask & (1 << i)){
                    _cur = (Powiter_Enums::Channel)i;
                    return;
                }
                i++;
            }
        }
        void operator--(){
            int i = (int)_cur;
            while(i >= 0){
                if(_mask & (1 << i)){
                    _cur = (Powiter_Enums::Channel)i;
                    return;
                }
                i--;
            }
            _cur = Powiter_Enums::Channel_black;
        }
        void operator--(int){
            int i = (int)_cur;
            while(i >= 0){
                if(_mask & (1 << i)){
                    _cur = (Powiter_Enums::Channel)i;
                    return;
                }
                i--;
            }
            _cur = Powiter_Enums::Channel_black;
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
    ChannelSet(Powiter_Enums::ChannelSetInit v);
    ChannelSet(Powiter_Enums::Channel v) : _size(1) {mask = v;}
    ~ChannelSet() {}
    const ChannelSet& operator=(const ChannelSet& source);
    const ChannelSet& operator=(Powiter_Enums::ChannelSetInit source) ;
    const ChannelSet& operator=(Powiter_Enums::Channel z);
    
    void clear() { mask = 0; _size = 0;}
    operator bool() const { return mask ; }// allow to do stuff like if(channelsA & channelsB)
    bool empty() const { return !mask; }
    
    bool operator==(const ChannelSet& source) const;
    bool operator!=(const ChannelSet& source) const { return !(*this == source); }
    bool operator<(const ChannelSet& source) const;
    bool operator==(Powiter_Enums::ChannelSetInit v) const { return mask == U32(v); }
    bool operator!=(Powiter_Enums::ChannelSetInit v) const { return !(*this == v); }
    bool operator==(Powiter_Enums::Channel z) const;
    bool operator!=(Powiter_Enums::Channel z) const { return !(*this == z); }
    
    /*add a channel with +=*/
    void operator+=(const ChannelSet& source);
    void operator+=(Powiter_Enums::ChannelSetInit source);
    void operator+=(Powiter_Enums::Channel z);
    void insert(Powiter_Enums::Channel z) { *this += z; }
    
    /*turn off a channel with -=*/
    void operator-=(const ChannelSet& source);
    void operator-=(Powiter_Enums::ChannelSetInit source);
    void operator-=(Powiter_Enums::Channel z);
    void erase(Powiter_Enums::Channel z) { *this -= z; }
    
    void operator&=(const ChannelSet& source);
    void operator&=(Powiter_Enums::ChannelSetInit source) ;
    void operator&=(Powiter_Enums::Channel z);
    
    
    ChannelSet operator&(const ChannelSet& c) const;
    ChannelSet operator&(Powiter_Enums::ChannelSetInit c) const;
    ChannelSet operator&(Powiter_Enums::Channel z) const;
    bool contains(const ChannelSet& source) const;
    bool contains(Powiter_Enums::ChannelSetInit source) const { return !(~mask & source); }
    bool contains(Powiter_Enums::Channel z) const;
    unsigned size() const;
    Powiter_Enums::Channel first() const;
    Powiter_Enums::Channel next(Powiter_Enums::Channel k) const;
    Powiter_Enums::Channel last() const;
    Powiter_Enums::Channel previous(Powiter_Enums::Channel k) const;
    
    U32 value() const {return mask;}
    
    iterator begin(){return iterator(mask,first());}
    
    iterator end(){
        Powiter_Enums::Channel _last = this->last();
        return iterator(mask,(Powiter_Enums::Channel)(_last+1));
    }
    void printOut();
    
#define foreachChannels(CUR, CHANNELS) \
    for (Powiter_Enums::Channel CUR = CHANNELS.first(); CUR; CUR = CHANNELS.next(CUR))


        };
typedef ChannelSet ChannelMask;


/*returns the channel of name "name"*/
Powiter_Enums::Channel getChannelByName(const char *name);

/*Return a cstring with the name of the channel c*/
std::string getChannelName(Powiter_Enums::Channel c);

/*useful function to check whether alpha is on in the mask*/
bool hasAlpha(ChannelMask mask);


#endif // CHANNELS_H
