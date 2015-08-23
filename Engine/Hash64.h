//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#ifndef NATRON_ENGINE_HASH64_H_
#define NATRON_ENGINE_HASH64_H_

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include <vector>
#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/static_assert.hpp>
#endif
#include "Global/Macros.h"
#include "Global/GlobalDefines.h"


class QString;

namespace Natron {
class Node;
}
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
CLANG_DIAG_OFF(unused-local-typedef)
        BOOST_STATIC_ASSERT(sizeof(T) <= 8);
CLANG_DIAG_ON(unused-local-typedef)
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

#endif // NATRON_ENGINE_Hash64_H_

