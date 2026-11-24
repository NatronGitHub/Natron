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

#ifndef NATRON_ENGINE_HASH64_H
#define NATRON_ENGINE_HASH64_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <cstring> // memcpy
#include <type_traits>
#include <vector>

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

    // The bytes of value, copied to the start of a zeroed U64. With memcpy, not a union, which would
    // be undefined behaviour (C.183); the result, and so every hash, is the same.
    template<typename T>
    static U64 toU64(T value)
    {
        static_assert(sizeof(T) <= sizeof(U64), "Type too large");
        static_assert(std::is_trivially_copyable<T>::value, "Type not trivially copyable");
        U64 raw = 0;

        std::memcpy(&raw, &value, sizeof(T));

        return raw;
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
    U64 hash;
    std::vector<U64> node_values;
};

void Hash64_appendQString(Hash64* hash, const QString & str);

NATRON_NAMESPACE_EXIT

#endif // NATRON_ENGINE_Hash64_H

