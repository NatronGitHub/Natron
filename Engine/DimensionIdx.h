/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
 * Copyright (C) 2018-2020 The Natron developers
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

#ifndef Engine_DimIdx_h
#define Engine_DimIdx_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <cassert>

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

class DimSpec
{
    int i;

public:
    DimSpec()
        : i(0)
    {
    }

    // cast from int must be explicit
    explicit DimSpec(int index)
        : i(index)
    {
        assert(index >= -1);
    }

    // cast to int is implicit
    operator int() const
    {
        return i;
    }

    bool operator<(const DimSpec& b) const {
        return i < b.i;
    }

    int value() const
    {
        return static_cast<int>(*this);
    }

    bool isAll() const { return i == -1; }

    bool isDimIdx() const { return i >= 0; }

    static DimSpec all() { return DimSpec(-1); };
};

class DimIdx
    : public DimSpec
{
public:
    DimIdx()
        : DimSpec(0)
    {
    }

    // cast from int must be explicit
    explicit DimIdx(int index)
        : DimSpec(index)
    {
        assert(index >= 0);
    }

    // cast to int is implicit
    operator int() const
    {
        int i = value();
        assert(i >= 0);

        return i;
    }

private:
    bool isAll() const { return false; }

    static DimIdx all(); // overload with no implementation
};

NATRON_NAMESPACE_EXIT



#endif // DIMENSIONIDX_H

