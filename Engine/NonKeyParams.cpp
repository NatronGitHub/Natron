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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "NonKeyParams.h"

using namespace Natron;


NonKeyParams::NonKeyParams()
    : _cost(0)
      , _elementsCount(0)
{
}

NonKeyParams::NonKeyParams(int cost,
                           U64 elementsCount)
    : _cost(cost)
      , _elementsCount(elementsCount)
{
}

NonKeyParams::NonKeyParams(const NonKeyParams & other)
    : _cost(other._cost)
      , _elementsCount(other._elementsCount)
{
}

///the number of elements the associated cache entry should allocate (relative to the datatype of the entry)
U64
NonKeyParams::getElementsCount() const
{
    return _elementsCount;
}

void
NonKeyParams::setElementsCount(U64 count)
{
    _elementsCount = count;
}

int
NonKeyParams::getCost() const
{
    return _cost;
}



