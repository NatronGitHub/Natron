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

#ifndef NATRON_ENGINE_TEXTURERECT_H
#define NATRON_ENGINE_TEXTURERECT_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <algorithm> // min, max

#include "Global/Macros.h"

#include "Engine/EngineFwd.h"


/** @class This class describes the rectangle (or portion) of an image that is contained
 * into a texture. x1,y1,x2,y2 are respectivly the image coordinates of the left,bottom,right,top
 * edges of the texture. w,h are the width and height of the texture. Note that x2 - x1 != w
 * and likewise y2 - y1 != h , this is because a texture might not contain all the lines/columns
 * of the image in the portion defined.
 **/
class TextureRect
{
public:
    double par; // the par of the associated image
    int x1,y1,x2,y2; // the edges of the texture. These are coordinates in the full size image
    int w,h; // the width and height of the texture. This has nothing to do with x,y,r,t
    int closestPo2; //< the closest power of 2 of the original region of interest of the image

    TextureRect()
        : par(1.)
        , x1(0)
        , y1(0)
        , x2(0)
        , y2(0)
        , w(0)
        , h(0)
        , closestPo2(1)
    {
    }

    TextureRect(int x1_,
                int y1_,
                int x2_,
                int y2_,
                int w_,
                int h_,
                int closestPo2_,
                double par_)
        : par(par_)
        , x1(x1_)
        , y1(y1_)
        , x2(x2_)
        , y2(y2_)
        , w(w_)
        , h(h_)
        , closestPo2(closestPo2_)
    {
    }

    void set(int x1_,
             int y1_,
             int x2_,
             int y2_,
             int w_,
             int h_,
             int closestPo2_,
             double par_)
    {
        x1 = x1_;
        y1 = y1_;
        x2 = x2_;
        y2 = y2_;
        w = w_;
        h = h_;
        closestPo2 = closestPo2_;
        par = par_;
    }

    void reset()
    {
        set(0, 0, 0, 0, 0, 0, 1, 1.);
    }

    bool isNull() const
    {
        return (x2 <= x1) || (y2 <= y1);
    }

    bool intersect(const RectI & r,
                   RectI* intersection) const;
    
    bool contains(const TextureRect& other) const
    {
        return other.x1 >= x1 &&
        other.y1 >= y1 &&
        other.x2 <= x2 &&
        other.y2 <= y2;
    }
};

inline bool
operator==(const TextureRect & first,
           const TextureRect & second)
{
    return first.x1 == second.x1 &&
           first.y1 == second.y1 &&
           first.x2 == second.x2 &&
           first.y2 == second.y2 &&
           first.w == second.w &&
           first.h == second.h &&
           first.closestPo2 == second.closestPo2 &&
           first.par == second.par;
}

inline bool
operator!=(const TextureRect & first,
           const TextureRect & second)
{
    return !(first == second);
}

#endif // NATRON_ENGINE_TEXTURERECT_H
