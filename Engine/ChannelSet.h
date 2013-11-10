//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012. 
*contact: immarespond at gmail dot com
*
*/

#ifndef POWITER_ENGINE_CHANNELSET_H_
#define POWITER_ENGINE_CHANNELSET_H_

#include <string>
#include <QtCore/QMetaType>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>

#include "Global/Macros.h"
#include "Global/GlobalDefines.h"

namespace Natron {

class ChannelSet{
    U32 mask; // 1 bit per channel and LSB is for "all" channels
    unsigned int _size;
    
    
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        (void)version;
        ar & mask;
        ar & _size;
    }
public:
    
    // class iterator: used to iterate over channels
    class iterator{
        Natron::Channel _cur;
        U32 _mask; // bit 0 means "all", other bits are for channels 1..31 
    public:
        iterator(const iterator& other):_cur(other._cur),_mask(other._mask){}
        iterator(U32 mask_, Natron::Channel cur) {
            _cur = cur;
            _mask = mask_;
        }
        Natron::Channel operator *() const{return _cur;}
        Natron::Channel operator->() const{return _cur;}
        void operator++(){
            int i = (int)_cur;
            while(i < 32){
                if(_mask & (1 << i)){
                    _cur = (Natron::Channel)i;
                    return;
                }
                ++i;
            }
            _cur = Natron::Channel_black;
        }
        void operator++(int){
            int i = (int)_cur;
            while(i < 32){
                if(_mask & (1 << i)){
                    _cur = (Natron::Channel)i;
                    return;
                }
                ++i;
            }
        }
        void operator--(){
            int i = (int)_cur;
            while(i >= 0){
                if(_mask & (1 << i)){
                    _cur = (Natron::Channel)i;
                    return;
                }
                --i;
            }
            _cur = Natron::Channel_black;
        }
        void operator--(int){
            int i = (int)_cur;
            while(i >= 0){
                if(_mask & (1 << i)){
                    _cur = (Natron::Channel)i;
                    return;
                }
                --i;
            }
            _cur = Natron::Channel_black;
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
    ChannelSet(Natron::ChannelMask v) { *this = v; }// use operator=(ChannelMask v)
    ChannelSet(Natron::Channel v) { *this = v; } // use operator=(Channel z)
    ~ChannelSet() {}
    const ChannelSet& operator=(const ChannelSet& source);
    const ChannelSet& operator=(Natron::ChannelMask source) ;
    const ChannelSet& operator=(Natron::Channel z);
    
    void clear() { mask = 0; _size = 0;}
    operator bool() const { return (bool)(mask >> 1) ; }// allow to do stuff like if(channelsA & channelsB)
    bool empty() const { return !(bool)(mask >> 1); }
    
    bool operator==(const ChannelSet& source) const;
    bool operator!=(const ChannelSet& source) const { return !(*this == source); }
    bool operator<(const ChannelSet& source) const;
    bool operator==(Natron::ChannelMask v) const { return mask == U32(v << 1); }
    bool operator!=(Natron::ChannelMask v) const { return mask != U32(v << 1); }
    bool operator==(Natron::Channel z) const;
    bool operator!=(Natron::Channel z) const { return !(*this == z); }
    
    /*add a channel with +=*/
    void operator+=(const ChannelSet& source);
    void operator+=(Natron::ChannelMask source);
    void operator+=(Natron::Channel z);
    void insert(Natron::Channel z) { *this += z; }
    
    /*turn off a channel with -=*/
    void operator-=(const ChannelSet& source);
    void operator-=(Natron::ChannelMask source);
    void operator-=(Natron::Channel z);
    void erase(Natron::Channel z) { *this -= z; }
    
    void operator&=(const ChannelSet& source);
    void operator&=(Natron::ChannelMask source) ;
    void operator&=(Natron::Channel z);
    
    
    ChannelSet operator&(const ChannelSet& c) const;
    ChannelSet operator&(Natron::ChannelMask c) const;
    ChannelSet operator&(Natron::Channel z) const;
    bool contains(const ChannelSet& source) const;
    bool contains(Natron::ChannelMask source) const { return (mask & 1) || (!(~mask & (source << 1))); }
    bool contains(Natron::Channel z) const;
    unsigned size() const;
    Natron::Channel first() const;
    Natron::Channel next(Natron::Channel k) const;
    Natron::Channel last() const;
    Natron::Channel previous(Natron::Channel k) const;
    
    U32 value() const {return mask;}
    
    iterator begin(){return iterator(mask,first());}
    
    iterator end(){
        Natron::Channel _last = this->last();
        return iterator(mask,(Natron::Channel)(_last+1));
    }
    void printOut() const;
    
#define foreachChannels(CUR, CHANNELS) \
    for (Natron::Channel CUR = CHANNELS.first(); CUR; CUR = CHANNELS.next(CUR))


        };



/*returns the channel of name "name"*/
Natron::Channel getChannelByName(const std::string &name);

/*Return a cstring with the name of the channel c*/
std::string getChannelName(Natron::Channel c);

/*useful function to check whether alpha is on in the mask*/
bool hasAlpha(ChannelSet mask);

} // namespace Natron

Q_DECLARE_METATYPE(Natron::ChannelSet);

#endif // POWITER_ENGINE_CHANNELSET_H_
