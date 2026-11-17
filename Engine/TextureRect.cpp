/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2023 The Natron developers
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

NATRON_NAMESPACE_ENTER

TextureRect::TextureRect()
: RectI()
, par(1.)
, closestPo2(1)
{
}

TextureRect::TextureRect(int x1_,
            int y1_,
            int x2_,
            int y2_,
            int closestPo2_,
            double par_)
: RectI(x1_, y1_, x2_, y2_)
, par(par_)
, closestPo2(closestPo2_)
{
}

void
TextureRect::reset()
{
    set(0, 0, 0, 0);
    par = 1;
    closestPo2 = 1;
}


NATRON_NAMESPACE_EXIT
