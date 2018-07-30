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

#ifndef NATRON_ENGINE_HASH64_H
#define NATRON_ENGINE_HASH64_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <vector>
#include <string>
#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/static_assert.hpp>
#endif

#include "Global/GlobalDefines.h"

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

/*The hash of a Node is the checksum of the vector of data containing:
    - the values of the current knob for this node + the name of the node
    - the hash values for the  tree upstream
 */

class Hash64
{
public:
    Hash64()
    : hash(0)
    , node_values()
    , hashValid(false)
    {
    }

    ~Hash64()
    {
    }

    U64 value() const
    {
        return hash;
    }

    bool isEmpty() const
    {
        return node_values.empty();
    }

    void computeHash();

    void reset();

    bool valid() const
    {
        return hashValid;
    }

    template<typename T>
    static U64 toU64(T value)
    {
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
            BOOST_STATIC_ASSERT(sizeof(T) <= 8);
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
        alias_cast_t<T> ac;

        ac.data = value;

        return ac.raw;
    }

    template<typename T>
    static T fromU64(U64 raw)
    {
        GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
        BOOST_STATIC_ASSERT(sizeof(T) <= 8);
        GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
        alias_cast_t<T> ac;

        ac.raw = raw;

        return ac.data;

    }

    void insert(const std::vector<U64>& elements)
    {
        std::size_t curSize = node_values.size();
        node_values.resize(curSize + elements.size());

        int c = curSize;
        for (std::vector<U64>::const_iterator it = elements.begin(); it != elements.end(); ++it, ++c) {
            node_values[c] = *it;
        }
        hashValid = false;

    }

    template<typename T>
    void append(T value)
    {
        node_values.push_back( toU64(value) );
        hashValid = false;
    }


    static void appendQString(const QString & str, Hash64* hash);

    static void appendCurve(const CurvePtr& curve, Hash64* hash);

    bool operator== (const Hash64 & h) const
    {
        return this->hash == h.value();
    }

    bool operator!= (const Hash64 & h) const
    {
        return this->hash != h.value();
    }

private:
    template<typename T>
    struct alias_cast_t
    {
        alias_cast_t()
            : raw(0)
        {
        };                          // initialize to 0 in case sizeof(T) < 8

        union
        {
            U64 raw;
            T data;
        };
    };

    U64 hash;
    std::vector<U64> node_values;
    bool hashValid;
};


NATRON_NAMESPACE_EXIT

#endif // NATRON_ENGINE_Hash64_H

