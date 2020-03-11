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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "RectD.h"

#include <cmath>
#include <cassert>
#include <stdexcept>

#include "Engine/RectI.h"

#include "Serialization/RectDSerialization.h"


NATRON_NAMESPACE_ENTER


void
RectD::toPixelEnclosing(const RenderScale & scale,
                        double par,
                        RectI *rect) const
{
    rect->x1 = std::floor(x1 * scale.x / par);
    rect->y1 = std::floor(y1 * scale.y);
    rect->x2 = std::ceil(x2 * scale.x / par);
    rect->y2 = std::ceil(y2 * scale.y);
}

void
RectD::toPixelEnclosing(unsigned int mipMapLevel,
                        double par,
                        RectI *rect) const
{
    double scale = 1. / (1 << mipMapLevel);

    rect->x1 = std::floor(x1 * scale / par);
    rect->y1 = std::floor(y1 * scale);
    rect->x2 = std::ceil(x2 * scale / par);
    rect->y2 = std::ceil(y2 * scale);
}


void
RectD::toSerialization(SERIALIZATION_NAMESPACE::SerializationObjectBase* obj)
{
    SERIALIZATION_NAMESPACE::RectDSerialization* s = dynamic_cast<SERIALIZATION_NAMESPACE::RectDSerialization*>(obj);
    if (!s) {
        return;
    }
    s->x1 = x1;
    s->y1 = y1;
    s->x2 = x2;
    s->y2 = y2;
}

void
RectD::fromSerialization(const SERIALIZATION_NAMESPACE::SerializationObjectBase & obj)
{
    const SERIALIZATION_NAMESPACE::RectDSerialization* s = dynamic_cast<const SERIALIZATION_NAMESPACE::RectDSerialization*>(&obj);
    if (!s) {
        return;
    }
    x1 = s->x1;
    y1 = s->y1;
    x2 = s->x2;
    y2 = s->y2;
}


NATRON_NAMESPACE_EXIT
