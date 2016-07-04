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

#include "RotoShapeRenderNode.h"

NATRON_NAMESPACE_ENTER;

struct RotoShapeRenderNodePrivate
{
    RotoShapeRenderNodePrivate()
    {
        
    }
};

RotoShapeRenderNode::RotoShapeRenderNode(NodePtr n)
: EffectInstance(n)
, _imp(new RotoShapeRenderNodePrivate())
{
    setSupportsRenderScaleMaybe(eSupportsYes);
}

RotoShapeRenderNode::~RotoShapeRenderNode()
{

}


void
RotoShapeRenderNode::addAcceptedComponents(int /*inputNb*/,
                                 std::list<ImageComponents>* comps)
{
    comps->push_back( ImageComponents::getRGBAComponents() );
    comps->push_back( ImageComponents::getRGBComponents() );
    comps->push_back( ImageComponents::getXYComponents() );
    comps->push_back( ImageComponents::getAlphaComponents() );
}

void
RotoShapeRenderNode::addSupportedBitDepth(std::list<ImageBitDepthEnum>* depths) const
{
    depths->push_back(eImageBitDepthFloat);
}

StatusEnum
RotoShapeRenderNode::getRegionOfDefinition(U64 hash, double time, const RenderScale & scale, ViewIdx view, RectD* rod)
{

}

bool
RotoShapeRenderNode::isIdentity(double time,
                const RenderScale & scale,
                const RectI & roi,
                ViewIdx view,
                double* inputTime,
                ViewIdx* inputView,
                int* inputNb)
{

}

StatusEnum
RotoShapeRenderNode::render(const RenderActionArgs& args)
{

} // RotoShapeRenderNode::render

NATRON_NAMESPACE_EXIT;