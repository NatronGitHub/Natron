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

#ifndef ROTOSHAPERENDERNODEPRIVATE_H
#define ROTOSHAPERENDERNODEPRIVATE_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****
#include <map>
#include <list>
#include "Engine/EngineFwd.h"
#include "Global/GlobalDefines.h"
#include "Engine/OSGLContext.h"


NATRON_NAMESPACE_ENTER;


class RotoShapeRenderNodePrivate
{
public:

    boost::weak_ptr<KnobChoice> outputComponents, renderType;


    RotoShapeRenderNodePrivate();


    typedef void* RenderStrokeDataPtr;

    typedef void (*PFNRenderStrokeBeginRender) (
    RenderStrokeDataPtr userData,
    double brushSizePixel,
    double brushSpacing,
    double brushHardness,
    bool pressureAffectsOpacity,
    bool pressureAffectsHardness,
    bool pressureAffectsSize,
    bool buildUp,
    double shapeColor[3],
    double opacity);

    typedef void (*PFNRenderStrokeEndRender) (RenderStrokeDataPtr userData);

    typedef void (*PFNRenderStrokeRenderDot) (RenderStrokeDataPtr userData, const Point &prevCenter, const Point &center, double pressure, double *spacing);

    static void renderStroke_generic(RenderStrokeDataPtr userData,
                                     PFNRenderStrokeBeginRender beginCallback,
                                     PFNRenderStrokeRenderDot renderDotCallback,
                                     PFNRenderStrokeEndRender endCallback,
                                     const std::list<std::list<std::pair<Point, double> > >& strokes,
                                     const double distToNextIn,
                                     const Point& lastCenterPointIn,
                                     const RotoDrawableItem* stroke,
                                     bool doBuildup,
                                     double opacity,
                                     double time,
                                     unsigned int mipmapLevel,
                                     double* distToNextOut,
                                     Point* lastCenterPoint);

    // If we were to copy exactly the portion in prevCenter, the smear would leave traces
    // too long. To dampen the effect of the smear, we clamp the spacing
    static Point dampenSmearEffect(const Point& prevCenter, const Point& center, const double spacing);
};

NATRON_NAMESPACE_EXIT;

#endif // ROTOSHAPERENDERNODEPRIVATE_H
