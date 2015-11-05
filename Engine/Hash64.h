/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
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

#include <vector>
#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/static_assert.hpp>
#endif
#include "Global/Macros.h"
#include "Global/GlobalDefines.h"
#include "Engine/EngineFwd.h"


class QString;

/*The hash of a Node is the checksum of the vector of data containing:
    - the values of the current knob for this node + the name of the node
    - the hash values for the  tree upstream
 */

class Hash64
{
public:
    Hash64()
    {
        hash = 0;
    }

    ~Hash64()
    {
        node_values.clear();
    }

    U64 value() const
    {
        return hash;
    }

    void computeHash();

    void reset();

    bool valid() const
    {
        return hash != 0;
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
    void append(T value)
    {
        node_values.push_back( toU64(value) );
    }

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
};

void Hash64_appendQString(Hash64* hash, const QString & str);

#endif // NATRON_ENGINE_Hash64_H

