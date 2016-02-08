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

#ifndef Engine_ViewIdx_h
#define Engine_ViewIdx_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER;

struct ViewIdx
{
    int i;
    
    explicit ViewIdx()
    : i(0)
    {
        
    }
    
    explicit ViewIdx(int index)
    : i (index)
    {
        assert(index >= -2);
    }

    bool operator==(ViewIdx other) const
    {
        return other.i == i;
    }
    
    bool operator!=(ViewIdx other) const
    {
        return other.i != i;
    }

    bool isAll() const { return i == -1; }
    bool isCurrent() const { return i == -2; }

    static ViewIdx all() { return ViewIdx(-1); };
    static ViewIdx current() { return ViewIdx(-2); };
};


NATRON_NAMESPACE_EXIT;

#endif // Engine_ViewIdx_h
