/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2020 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#ifndef Engine_Color_h
#define Engine_Color_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

/**
 * @brief A RGB Color
 **/
template<typename T>
class ColorRgb
{
public:
    T r;
    T g;
    T b;

    /*
    T& operator[](int i)
    {
        assert(0 <= i && i < 3);
        return (i == 0) ? r : ( (i == 1) ? g : ( (i == 2) ? b : T() ) );
    }
     */

    ColorRgb()
    : r(), g(), b()
    {
    }

    ColorRgb(T r_,
             T g_,
             T b_)
    : r(r_), g(g_), b(b_)
    {
    }

    void set(T r_,
             T g_,
             T b_)
    {
        r = r_;
        g = g_;
        b = b_;
    }
};

/**
 * @brief A RGBA Color
 **/
template<typename T>
class ColorRgba
{
public:
    T r;
    T g;
    T b;
    T a;

    /*
    T& operator[](int i)
    {
        assert(0 <= i && i < 4);
        return (i == 0) ? r : ( (i == 1) ? g : ( (i == 2) ? b : ( (i == 3) ? a : T() ) ) );
    }
     */

    ColorRgba()
    : r(), g(), b(), a()
    {
    }

    ColorRgba(T r_,
              T g_,
              T b_,
              T a_)
    : r(r_), g(g_), b(b_), a(a_)
    {
    }

    void set(T r_,
             T g_,
             T b_,
             T a_)
    {
        r = r_;
        g = g_;
        b = b_;
        a = a_;
    }
};

typedef ColorRgb<double> ColorRgbD;
typedef ColorRgba<double> ColorRgbaD;

NATRON_NAMESPACE_EXIT


#endif // Engine_Color_h
