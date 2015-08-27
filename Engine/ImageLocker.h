/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier
 *
 * Natron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Natron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 * ***** END LICENSE BLOCK ***** */

#ifndef IMAGELOCKER_H
#define IMAGELOCKER_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <cassert>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#endif


#include "Global/Macros.h"


template<typename EntryType>
class LockManagerI
{
public:
    
    LockManagerI() {}
    
    virtual ~LockManagerI() {}
    
    virtual void lock(const boost::shared_ptr<EntryType>& entry) = 0;
    
    virtual bool tryLock(const boost::shared_ptr<EntryType>& entry) = 0;
    
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
        unlock();
    }
    
    void lock(const boost::shared_ptr<EntryType>& entry) {
        assert(!_entry);
        _entry = entry;
        if (_manager) {
            _manager->lock(_entry);
        }
    }
    
    bool tryLock(const boost::shared_ptr<EntryType>& entry)
    {
        assert(!_entry);
        if (_manager) {
            if (_manager->tryLock(entry)) {
                _entry = entry;
                return true;
            }
        }
        return false;
    }
    
    void unlock()
    {
        if (_entry && _manager) {
            _manager->unlock(_entry);
            _entry.reset();
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
