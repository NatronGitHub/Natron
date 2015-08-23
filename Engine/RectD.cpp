//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "RectD.h"

#include <cmath>

#include "Engine/RectI.h"

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
