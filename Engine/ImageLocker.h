//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#ifndef IMAGELOCKER_H
#define IMAGELOCKER_H

#include <cassert>
#include <boost/shared_ptr.hpp>

#include "Global/Macros.h"


template<typename EntryType>
class LockManagerI
{
public:
    
    LockManagerI() {}
    
    virtual ~LockManagerI() {}
    
    virtual void lock(const boost::shared_ptr<EntryType>& entry) = 0;
    
    
    virtual void unlock(const boost::shared_ptr<EntryType>& entry) =0 ;
    
};

/**
 * @brief Small interface describing an image locker.
 * The idea is that the cache will create a new entry
 * and call lock(entry) on it because its memory has not
 * yet been initialized via CacheEntry::allocateMemory()
 **/
template<typename EntryType>
class ImageLockerHelper
{
    
    LockManagerI<EntryType>* _manager;
    boost::shared_ptr<EntryType> _entry;
    
public:
    
    
    ImageLockerHelper(LockManagerI<EntryType>* manager) : _manager(manager), _entry() {}
    
    ImageLockerHelper(LockManagerI<EntryType>* manager,
                      const boost::shared_ptr<EntryType>& entry) : _manager(manager), _entry(entry) {
    
        if (_entry && _manager) {
            _manager->lock(_entry);
        }
    }
    
    ~ImageLockerHelper()
    {
        if (_entry && _manager) {
            _manager->unlock(_entry);
        }
    }
    
    void lock(const boost::shared_ptr<EntryType>& entry) {
        assert(!_entry);
        _entry = entry;
        if (_manager) {
            _manager->lock(_entry);
        }
    }

};

namespace Natron {
    class Image;
    class FrameEntry;
}


typedef ImageLockerHelper<Natron::Image> ImageLocker;
typedef ImageLockerHelper<Natron::FrameEntry> FrameEntryLocker;

#endif // IMAGELOCKER_H
