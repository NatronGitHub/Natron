/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2023 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#ifndef NATRON_ENGINE_LRUCACHE_H
#define NATRON_ENGINE_LRUCACHE_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

//
//Copyright (c) 2010-2011, Tim Day <timday@timday.com>
//
//Permission to use, copy, modify, and/or distribute this software for any
//purpose with or without fee is hereby granted, provided that the above
//copyright notice and this permission notice appear in all copies.
//
//THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
//WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
//MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
//ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
//WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
//ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
//OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

#include <map>
#include <list>
#include <utility>
#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
// clang-format off
CLANG_DIAG_OFF(unknown-pragmas)
CLANG_DIAG_OFF(redeclared-class-member)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
#include <boost/bimap/list_of.hpp>
#include <boost/bimap/unordered_set_of.hpp>
#include <boost/bimap.hpp>
CLANG_DIAG_ON(redeclared-class-member)
CLANG_DIAG_ON(unknown-pragmas)
// clang-format on
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
#endif

#include "Engine/EngineFwd.h"


/**@brief The LRU cache used by Natron's caches: a boost::bimap with
 * boost::unordered_set_of (hashing) for the keys and a list_of for the LRU order.
 **/
template <typename K, typename V>
class BoostLRUHashTable
{
public:
    typedef K key_type;
    typedef std::list<V> value_type;
    typedef boost::bimaps::bimap<boost::bimaps::unordered_set_of<key_type>, boost::bimaps::list_of<value_type> > container_type;

    BoostLRUHashTable()
    {
    }

    typename container_type::left_iterator operator()(const key_type & k)
    {
        // Attempt to find existing record
        typename container_type::left_iterator it = _container.left.find(k);
        if ( it != _container.left.end() ) {
            // We do have it:
            // Update the access record view.
            _container.right.relocate( _container.right.end(), _container.project_right(it) );
        }

        return it;
    }

    void erase(typename container_type::left_iterator it)
    {
        _container.left.erase(it);
    }

    // return end of the iterator
    typename container_type::left_iterator end()
    {
        return _container.left.end();
    }

    // return begin of the iterator
    typename container_type::left_iterator begin()
    {
        return _container.left.begin();
    }

    void insert(const key_type & k,
                const value_type& list)
    {
        _container.insert( typename container_type::value_type(k, list) );
    }

    void insert(const key_type & k,
                const V & v)
    {
        // Create a new record from the key and the value
        // bimap's list_view defaults to inserting this at
        // the list tail (considered most-recently-used).
        typename container_type::left_iterator found = this->operator ()(k);
        if ( found != _container.left.end() ) {
            found->second.push_back(v);
        } else {
            value_type list;
            list.push_back(v);
            _container.insert( typename container_type::value_type(k, list) );
        }
    }

    void clear()
    {
        _container.clear();
    }

    std::pair<key_type, V> evict()
    {
        typename container_type::right_iterator it = _container.right.begin();
        while ( it != _container.right.end() ) {
            for (typename std::list<V>::iterator it2 = it->first.begin(); it2 != it->first.end(); ++it2) {
                if (it2->use_count() == 1) {
                    std::pair<key_type, V> ret = std::make_pair(it->second, *it2);
                    if (it->first.size() == 1) {
                        _container.right.erase(it);
                    } else {
                        it->first.erase(it2);
                    }

                    return ret;
                }
            }
            ++it;
        }

        return std::make_pair( key_type(), V() );
    }

    unsigned int size()
    {
        return _container.size();
    }

private:
    container_type _container;
};

#endif // ifndef NATRON_ENGINE_LRUCACHE_H
