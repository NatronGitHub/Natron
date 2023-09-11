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

#ifndef RENDERSCALE_H
#define RENDERSCALE_H

#include "Global/Macros.h"

#include "ofxCore.h"

NATRON_NAMESPACE_ENTER

class RenderScale {
public:
    // Default constructed object has x & y scale set to 1.
    constexpr RenderScale() = default;

    // Named constant where x & y scale are both 1. (i.e the identity scale factor in both directions.)
    static const RenderScale identity;

    bool operator==(const RenderScale& rhs) const { return x == rhs.x && y == rhs.y; }
    bool operator!=(const RenderScale& rhs) const { return !(*this == rhs); }

    OfxPointD toOfxPointD() const;

    static RenderScale fromMipmapLevel(unsigned int mipmapLevel);
    unsigned int toMipmapLevel() const;

private:
    RenderScale(double scale);
    double x = 1.;
    double y = 1.;
};

NATRON_NAMESPACE_EXIT

#endif // RENDERSCALE_H
