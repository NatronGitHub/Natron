#ifndef CHANNELS_H
#define CHANNELS_H

#include <string>
#include <cstring>
#include <boost/foreach.hpp>
#include <vector>
#include "Superviser/powiterFn.h"
#undef foreach
#define foreach BOOST_FOREACH

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
operator bool() const { return mask ; }
bool empty() const { return !mask; }

bool operator==(const ChannelSet& source) const;
bool operator!=(const ChannelSet& source) const { return !(*this == source); }
bool operator<(const ChannelSet& source) const;
bool operator==(ChannelSetInit v) const { return mask == U32(v); }
bool operator!=(ChannelSetInit v) const { return !(*this == v); }
bool operator==(Channel z) const;
bool operator!=(Channel z) const { return !(*this == z); }
void operator+=(const ChannelSet& source);
void operator+=(ChannelSetInit source);
void operator+=(Channel z);
void insert(Channel z) { *this += z; }
void operator-=(const ChannelSet& source);
void operator-=(ChannelSetInit source);
void operator-=(Channel z);
void erase(Channel z) { *this -= z; }

void operator&=(const ChannelSet& source);
void operator&=(ChannelSetInit source) ;
void operator&=(Channel z);

//    template<class Type> ChannelSet operator+(Type z) const {
//        ChannelSet tmp = *this;
//        tmp += z;
//        return tmp;
//    }
//
//    template<class Type> ChannelSet operator-(Type z) const {
//        ChannelSet tmp = *this;
//        tmp -= z;
//        return tmp;
//    }
//    template<class Type> ChannelSet intersection(Type z) const {
//        ChannelSet tmp = *this;
//        tmp &= z;
//        return tmp;
//    }
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


#define foreachChannels(VAR, CHANNELS) \
for (Channel VAR = CHANNELS.first(); VAR; VAR = CHANNELS.next(VAR))

};
typedef ChannelSet ChannelMask;



Channel getChannelByName(const char *name);
const char* getChannelName(Channel c);

bool hasAlpha(ChannelMask mask);


#endif // CHANNELS_H
