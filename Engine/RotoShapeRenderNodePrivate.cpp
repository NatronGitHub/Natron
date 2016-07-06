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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "RotoShapeRenderNodePrivate.h"

#include "Engine/KnobTypes.h"
#include "Engine/RotoShapeRenderNode.h"
#include "Engine/RotoStrokeItem.h"

NATRON_NAMESPACE_ENTER;

RotoShapeRenderNodePrivate::RotoShapeRenderNodePrivate()
{
}





double
RotoShapeRenderNodePrivate::renderStroke_generic(RenderStrokeDataPtr userData,
                                                 PFNRenderStrokeBeginRender beginCallback,
                                                 PFNRenderStrokeRenderDot renderDotCallback,
                                                 PFNRenderStrokeEndRender endCallback,
                                                 const std::list<std::list<std::pair<Point, double> > >& strokes,
                                                 double distToNext,
                                                 const RotoDrawableItem* stroke,
                                                 bool doBuildup,
                                                 double opacity,
                                                 double time,
                                                 unsigned int mipmapLevel)
{
    double brushSize, brushSizePixel, brushSpacing, brushHardness, writeOnStart, writeOnEnd;
    bool pressureAffectsOpacity, pressureAffectsHardness, pressureAffectsSize;
    {
        KnobDoublePtr brushSizeKnob = stroke->getBrushSizeKnob();
        brushSize = brushSizeKnob->getValueAtTime(time);
        KnobDoublePtr brushSpacingKnob = stroke->getBrushSpacingKnob();
        brushSpacing = brushSpacingKnob->getValueAtTime(time);
        if (brushSpacing == 0.) {
            return distToNext;
        }
        brushSpacing = std::max(brushSpacing, 0.05);



        KnobDoublePtr brushHardnessKnob = stroke->getBrushHardnessKnob();
        brushHardness = brushHardnessKnob->getValueAtTime(time);
        KnobDoublePtr visiblePortionKnob = stroke->getBrushVisiblePortionKnob();
        writeOnStart = visiblePortionKnob->getValueAtTime(time, 0);
        writeOnEnd = visiblePortionKnob->getValueAtTime(time, 1);
        if ( (writeOnEnd - writeOnStart) <= 0. ) {
            return distToNext;
        }


        KnobBoolPtr pressureOpacityKnob = stroke->getPressureOpacityKnob();
        KnobBoolPtr pressureSizeKnob = stroke->getPressureSizeKnob();
        KnobBoolPtr pressureHardnessKnob = stroke->getPressureHardnessKnob();
        pressureAffectsOpacity = pressureOpacityKnob->getValueAtTime(time);
        pressureAffectsSize = pressureSizeKnob->getValueAtTime(time);
        pressureAffectsHardness = pressureHardnessKnob->getValueAtTime(time);
        brushSizePixel = brushSize;
        if (mipmapLevel != 0) {
            brushSizePixel = std::max( 1., brushSizePixel / (1 << mipmapLevel) );
        }
    }

    double shapeColor[3];
    stroke->getColor(time, shapeColor);


    beginCallback(userData, brushSizePixel, brushSpacing, brushHardness, pressureAffectsOpacity, pressureAffectsHardness, pressureAffectsSize, doBuildup, shapeColor, opacity);

    for (std::list<std::list<std::pair<Point, double> > >::const_iterator strokeIt = strokes.begin(); strokeIt != strokes.end(); ++strokeIt) {
        int firstPoint = (int)std::floor( (strokeIt->size() * writeOnStart) );
        int endPoint = (int)std::ceil( (strokeIt->size() * writeOnEnd) );
        assert( firstPoint >= 0 && firstPoint < (int)strokeIt->size() && endPoint > firstPoint && endPoint <= (int)strokeIt->size() );


        ///The visible portion of the paint's stroke with points adjusted to pixel coordinates
        std::list<std::pair<Point, double> > visiblePortion;
        std::list<std::pair<Point, double> >::const_iterator startingIt = strokeIt->begin();
        std::list<std::pair<Point, double> >::const_iterator endingIt = strokeIt->begin();
        std::advance(startingIt, firstPoint);
        std::advance(endingIt, endPoint);
        for (std::list<std::pair<Point, double> >::const_iterator it = startingIt; it != endingIt; ++it) {
            visiblePortion.push_back(*it);
        }
        if ( visiblePortion.empty() ) {
            return distToNext;
        }

        std::list<std::pair<Point, double> >::iterator it = visiblePortion.begin();

        if (visiblePortion.size() == 1) {
            double spacing;
            renderDotCallback(userData, it->first, it->second, &spacing);
            continue;
        }

        std::list<std::pair<Point, double> >::iterator next = it;
        ++next;

        while ( next != visiblePortion.end() ) {
            //Render for each point a dot. Spacing is a percentage of brushSize:
            //Spacing at 1 means no dot is overlapping another (so the spacing is in fact brushSize)
            //Spacing at 0 we do not render the stroke

            double dist = std::sqrt( (next->first.x - it->first.x) * (next->first.x - it->first.x) +  (next->first.y - it->first.y) * (next->first.y - it->first.y) );

            // while the next point can be drawn on this segment, draw a point and advance
            while (distToNext <= dist) {
                double a = dist == 0. ? 0. : distToNext / dist;
                Point center = {
                    it->first.x * (1 - a) + next->first.x * a,
                    it->first.y * (1 - a) + next->first.y * a
                };
                double pressure = it->second * (1 - a) + next->second * a;

                // draw the dot
                double spacing;
                renderDotCallback(userData, center, pressure, &spacing);
                distToNext += spacing;
            }

            // go to the next segment
            distToNext -= dist;
            ++next;
            ++it;
        }
    }

    endCallback(userData);
    
    
    return distToNext;

}

NATRON_NAMESPACE_EXIT;
