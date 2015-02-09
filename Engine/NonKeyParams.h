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

#ifndef NONKEYPARAMS_H
#define NONKEYPARAMS_H

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include <cstddef>

#include "Global/GlobalDefines.h"

namespace boost {
namespace serialization {
class access;
}
}

namespace Natron {
class NonKeyParams
{
public:

    NonKeyParams();

    virtual ~NonKeyParams()
    {
    }

    NonKeyParams(int cost,
                 U64 elementsCount);

    NonKeyParams(const NonKeyParams & other);

    ///the number of elements the associated cache entry should allocate the first time (relative to the datatype of the entry)
    U64 getElementsCount() const;
    
    void setElementsCount(U64 count);

    int getCost() const;

    

    template<class Archive>
    void serialize(Archive & ar,const unsigned int /*version*/);

protected:

    bool operator==(const NonKeyParams & other) const
    {
        return (_cost == other._cost &&
                _elementsCount == other._elementsCount);
    }
    
    bool operator!=(const NonKeyParams & other) const
    {
        return !(*this == other);
    }


private:


    int _cost; //< the cost of the element associated to this key
    std::size_t _elementsCount; //< the number of elements the associated cache entry should allocate (relative to the datatype of the entry)
};
}

#endif // NONKEYPARAMS_H
