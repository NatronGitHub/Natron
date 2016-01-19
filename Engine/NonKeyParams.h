/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
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

#ifndef NONKEYPARAMS_H
#define NONKEYPARAMS_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <cstddef>

#include "Global/GlobalDefines.h"
#include "Engine/EngineFwd.h"


NATRON_NAMESPACE_ENTER;
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

    std::size_t _elementsCount; //< the number of elements the associated cache entry should allocate (relative to the datatype of the entry)
    int _cost; //< the cost of the element associated to this key
};
}

#endif // NONKEYPARAMS_H
