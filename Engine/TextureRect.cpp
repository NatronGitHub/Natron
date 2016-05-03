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

#include "TextureRect.h"

#include <cassert>
#include <stdexcept>

#include "Engine/RectI.h"
#include "Engine/RectD.h"

NATRON_NAMESPACE_ENTER;
void
TextureRect::operator=(const RectI& other)
{
    x1 = other.x1;
    x2 = other.x2;
    y1 = other.y1;
    y2 = other.y2;
}

bool
TextureRect::contains(const TextureRect& other) const
{
    return other.x1 >= x1 &&
           other.y1 >= y1 &&
           other.x2 <= x2 &&
           other.y2 <= y2;
}

NATRON_NAMESPACE_EXIT;
