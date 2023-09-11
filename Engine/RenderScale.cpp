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

#include "RenderScale.h"

#include "Global/Macros.h"

#include "Engine/Image.h"

NATRON_NAMESPACE_ENTER

// static
const RenderScale RenderScale::identity;

RenderScale::RenderScale(unsigned int mipmapLevel_)
    : mipmapLevel(mipmapLevel_)
{
}

OfxPointD
RenderScale::toOfxPointD() const
{
    const double scale = Image::getScaleFromMipMapLevel(mipmapLevel);
    return { scale, scale };
}

// static
RenderScale
RenderScale::fromMipmapLevel(unsigned int mipmapLevel)
{
    return RenderScale(mipmapLevel);
}

unsigned int
RenderScale::toMipmapLevel() const
{
    return mipmapLevel;
}

NATRON_NAMESPACE_EXIT
