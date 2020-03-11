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

#ifndef ROTOSHAPERENDERNODEPRIVATE_H
#define ROTOSHAPERENDERNODEPRIVATE_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <map>
#include <list>

#include "Global/GlobalDefines.h"
#include "Engine/Color.h"
#include "Engine/OSGLContext.h"
#include "Engine/TimeValue.h"

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER


class RotoShapeRenderNodePrivate
{
public:

    KnobChoiceWPtr renderType;

    KnobChoiceWPtr outputRoDTypeKnob;
    KnobChoiceWPtr outputFormatKnob;
    KnobIntWPtr outputFormatSizeKnob;
    KnobDoubleWPtr outputFormatParKnob;
    KnobBoolWPtr clipToFormatKnob;
    
    // When drawing a smear with OSMesa we cannot use the output image directly
    // because the first draw done by OSMesa will clear the framebuffer instead
    // of preserving the actual content.
    ImagePtr osmesaSmearTmpTexture;


    RotoShapeRenderNodePrivate();

    /**
     * @brief A blind pointer passed to all callbacks during the renderStroke_generic algorithm
     * so that the implementation specific renderer can manage its own datas.
     **/
    typedef void* RenderStrokeDataPtr;

    /**
     * @brief Callback used before starting rendering any dot in renderStroke_generic
     * This is used to set renderer specific settings linked to the user parameters.
     **/
    typedef void (*PFNRenderStrokeBeginRender) (
    RenderStrokeDataPtr userData,
    double brushSizePixelX,
    double brushSizePixelY,
    double brushSpacing,
    double brushHardness,
    bool pressureAffectsOpacity,
    bool pressureAffectsHardness,
    bool pressureAffectsSize,
    bool buildUp,
    double opacity);

    /**
     * @brief Called when finished to render all strokes passed to renderStroke_generic
     * If possible, all drawing should happen at once in this function rather than each renderDot call.
     **/
    typedef void (*PFNRenderStrokeEndRender) (RenderStrokeDataPtr userData);

    /**
     * @brief Callback called to draw a dot at the given center location with given pressure. The prevCenter indicates the location
     * of the latest dot previously rendered. This function must return in spacing the distance wished to the next dot.
     * @returns True if successfully rendered a dot, false otherwise.
     **/
    typedef bool (*PFNRenderStrokeRenderDot) (RenderStrokeDataPtr userData, const Point &prevCenter, const Point &center, double pressure, double *spacing);

    /**
     * @brief This is the generic algorithm used to render strokes. 
     * The algorithm will render dots (using the renderDotCallback) at uniformly
     * distributed positions betweens segment along the stroke. 
     * Since the user may be interactively drawing, the algorithm may take
     * in input the last dot center point and last distToNext to actually 
     * continue a stroke.
     * @returns true if at least one dot was rendered, false otherwise
     **/
    static bool renderStroke_generic(RenderStrokeDataPtr userData,
                                     PFNRenderStrokeBeginRender beginCallback,
                                     PFNRenderStrokeRenderDot renderDotCallback,
                                     PFNRenderStrokeEndRender endCallback,
                                     const std::list<std::list<std::pair<Point, double> > >& strokes,
                                     const double distToNextIn,
                                     const Point& lastCenterPointIn,
                                     const RotoDrawableItemPtr& stroke,
                                     bool doBuildup,
                                     double opacity,
                                     TimeValue time,
                                     ViewIdx view,
                                     const RenderScale& scale,
                                     double* distToNextOut,
                                     Point* lastCenterPoint);

    // If we were to copy exactly the portion in prevCenter, the smear would leave traces
    // too long. To dampen the effect of the smear, we clamp the spacing
    static Point dampenSmearEffect(const Point& prevCenter, const Point& center, const double spacing);
};

NATRON_NAMESPACE_EXIT

#endif // ROTOSHAPERENDERNODEPRIVATE_H
