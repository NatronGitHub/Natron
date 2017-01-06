/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2017 INRIA and Alexandre Gauthier-Foichat
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

#ifndef Engine_ViewIdx_h
#define Engine_ViewIdx_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <cassert>

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER;


class ViewIdx;
class ViewGetSpec;

class ViewSetSpec
{
    int i;

public:
    ViewSetSpec()
    : i(0)
    {
    }

    // cast from int must be explicit
    explicit ViewSetSpec(int index)
    : i(index)
    {
        assert(index >= -2);
    }

    ViewSetSpec(const ViewIdx& view_i);
    ViewSetSpec(const ViewGetSpec& view_get);

    // cast to int is implicit
    operator int() const
    {
        return i;
    }

    bool operator<(const ViewSetSpec& b) const {
        return i < b.i;
    }

    int value() const
    {
        return static_cast<int>(*this);
    }

    bool isAll() const { return i == -1; }

    bool isCurrent() const { return i == -2; }

    bool isViewIdx() const { return i >= 0; }

    static ViewSetSpec all() { return ViewSetSpec(-1); };
    static ViewSetSpec current() { return ViewSetSpec(-2); };
};


class ViewGetSpec
{
    int i;

public:
    ViewGetSpec()
    : i(0)
    {
    }

    // cast from int must be explicit
    explicit ViewGetSpec(int index)
    : i(index)
    {
        assert(index == -2 || index >= 0);
    }

    ViewGetSpec(const ViewIdx& view_i);

    // cast to int is implicit
    operator int() const
    {
        return i;
    }

    bool operator<(const ViewGetSpec& b) const {
        return i < b.i;
    }

    int value() const
    {
        return static_cast<int>(*this);
    }

    bool isCurrent() const { return i == -2; }

    bool isViewIdx() const { return i >= 0; }

    static ViewGetSpec current() { return ViewGetSpec(-2); };

private:

    bool isAll() const { return false; }

    static ViewIdx all(); // overload with no implementation

};

class ViewIdx
    : public ViewGetSpec
{
public:
    ViewIdx()
        : ViewGetSpec(0)
    {
    }

    // cast from int must be explicit
    explicit ViewIdx(int index)
        : ViewGetSpec(index)
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

    bool isCurrent() const { return false; }

    static ViewIdx current(); // overload with no implementation
};

NATRON_NAMESPACE_EXIT;

#endif // Engine_ViewIdx_h
