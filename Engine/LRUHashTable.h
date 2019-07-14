/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#ifndef Engine_LRUHashTable_h
#define Engine_LRUHashTable_h

#if 0

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"


#include <map>
#include <list>
#include <utility>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
CLANG_DIAG_OFF(unknown-pragmas)
CLANG_DIAG_OFF(redeclared-class-member)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
#include <boost/interprocess/containers/map.hpp>
#include <boost/interprocess/containers/list.hpp>
#include <boost/interprocess/managed_mapped_file.hpp>
CLANG_DIAG_ON(redeclared-class-member)
CLANG_DIAG_ON(unknown-pragmas)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
#endif

#include "Engine/EngineFwd.h"


NATRON_NAMESPACE_ENTER

/**
 * @brief A least-recently used container: it works as a map, execept that there's a evict() function to remove the least recently
 * used key/value pair. A value is set to be the most recently used when it is used by the methods find() or insert()
 *
 * The implementation makes use of boost interprocess compatible containers.
 **/
template <typename K, typename V, typename SEGMENT_MANAGER>
class LRUHashTable
{
public:

    typedef K key_type;
    typedef V value_type;

    typedef boost::interprocess::allocator<key_type, SEGMENT_MANAGER> MM_allocator_key_type;

    // Key access history, most recent at back
    typedef boost::interprocess::list<key_type, MM_allocator_key_type> key_tracker_type;

    struct ValueAge
    {
        value_type value;
        typename key_tracker_type::iterator historyIterator;
    };

    typedef boost::interprocess::allocator<ValueAge, SEGMENT_MANAGER> MM_allocator_ValueAge;

    // Key to value and key history iterator
    typedef boost::interprocess::map<key_type, ValueAge, std::less<key_type>, MM_allocator_ValueAge> key_to_value_type;

    // Iterators on the map
    typedef typename key_to_value_type::iterator iterator;
    typedef typename key_to_value_type::const_iterator const_iterator;

    LRUHashTable()
    {
    }

    /**
     * @brief Obtain value of the cached function for k and updates the LRU record.
     **/
    iterator find(const key_type & key)
    {
        // Attempt to find existing record
        iterator it = _key_to_value.find(key);
        if ( it != _key_to_value.end() ) {
            // Update access record by moving
            // accessed key to back of list
            _key_tracker.splice(_key_tracker.end(), _key_tracker, it->second.historyIterator);
        }

        return it;
    }

    /**
     * @brief Removes the value pointed by the iterator from the container
     **/
    void erase(iterator it)
    {
        _key_tracker.erase(it->second.historyIterator);
        _key_to_value.erase(it);
    }

    /**
     * @brief Returns the end of the container
     **/
    iterator end()
    {
        return _key_to_value.end();
    }

    /**
     * @brief Returns the begining of the container
     **/
    iterator begin()
    {
        return _key_to_value.begin();
    }

    /**
     * @brief Inserts the given key and value in the container
     **/
    void insert(const key_type & key, const value_type& value)
    {
        ValueAge v;
        v.value = value;

        // Insert the key at the end of the record
        v.historyIterator = _key_tracker.insert(_key_tracker.end(), key);

        _key_to_value.insert(std::make_pair(key, v));
    }

    /**
     * @brief Clears the container
     **/
    void clear()
    {
        _key_to_value.clear();
        _key_tracker.clear();
    }

    /**
     * @brief Removes the least recently used element from the container
     **/
    std::pair<key_type, value_type> evict()
    {
        // Assert method is never called when cache is empty
        if (_key_tracker.empty()) {
            return std::make_pair( key_type(), V() );
        }
        // Identify least recently used key
        iterator it  = _key_to_value.find( _key_tracker.front() );

        std::pair<key_type, value_type> ret = std::make_pair(it->first, it->second.value);

        // Erase both elements to completely purge record
        _key_to_value.erase(it);
        _key_tracker.pop_front();

        return ret;
    }

    /**
     * @brief Returns the number of key/value pairs.
     **/
    std::size_t size()
    {
        return _key_to_value.size();
    }

private:
    // Key access history
    key_tracker_type _key_tracker;

    // Key-to-value lookup
    key_to_value_type _key_to_value;
};

NATRON_NAMESPACE_EXIT
#endif // 0
#endif // ifndef Engine_LRUHashTable_h
