/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#ifndef NATRON_ENGINE_TEXTURERECT_H
#define NATRON_ENGINE_TEXTURERECT_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <algorithm> // min, max

#include "Engine/RectI.h"
#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

/** @class This class describes the rectangle (or portion) of an image that is contained
 * into a texture. x1,y1,x2,y2 are respectivly the image coordinates of the left,bottom,right,top
 * edges of the texture. w,h are the width and height of the texture. Note that x2 - x1 != w
 * and likewise y2 - y1 != h , this is because a texture might not contain all the lines/columns
 * of the image in the portion defined.
 **/
class TextureRect
    : public RectI
{
public:
    double par; // the par of the associated image
    int closestPo2; //< the closest power of 2 of the original region of interest of the image

    TextureRect();

    TextureRect(int x1_,
                int y1_,
                int x2_,
                int y2_,
                int closestPo2_,
                double par_);

    void reset();

};

inline bool
operator==(const TextureRect & first,
           const TextureRect & second)
{
    return first.x1 == second.x1 &&
           first.y1 == second.y1 &&
           first.x2 == second.x2 &&
           first.y2 == second.y2 &&
           first.closestPo2 == second.closestPo2 &&
           first.par == second.par;
}

inline bool
operator!=(const TextureRect & first,
           const TextureRect & second)
{
    return !(first == second);
}

NATRON_NAMESPACE_EXIT

#endif // NATRON_ENGINE_TEXTURERECT_H
