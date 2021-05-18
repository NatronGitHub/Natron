/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2021 The Natron developers
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

#include "NoOpBase.h"

#include <cassert>
#include <stdexcept>

#include "Engine/Transform.h" // Matrix3x3
#include "Engine/ViewIdx.h"

NATRON_NAMESPACE_ENTER

NoOpBase::NoOpBase(NodePtr n)
    : OutputEffectInstance(n)
{
    setSupportsRenderScaleMaybe(eSupportsYes);
}

void
NoOpBase::addAcceptedComponents(int /*inputNb*/,
                                std::list<ImagePlaneDesc>* comps)
{
    comps->push_back( ImagePlaneDesc::getRGBComponents() );
    comps->push_back( ImagePlaneDesc::getRGBAComponents() );
    comps->push_back( ImagePlaneDesc::getAlphaComponents() );
}

void
NoOpBase::addSupportedBitDepth(std::list<ImageBitDepthEnum>* depths) const
{
    depths->push_back(eImageBitDepthByte);
    depths->push_back(eImageBitDepthShort);
    depths->push_back(eImageBitDepthFloat);
}

bool
NoOpBase::isIdentity(double time,
                     const RenderScale & /*scale*/,
                     const RectI & /*roi*/,
                     ViewIdx view,
                     double* inputTime,
                     ViewIdx* inputView,
                     int* inputNb)
{
    *inputTime = time;
    *inputNb = 0;
    *inputView = view;

    return true;
}

bool
NoOpBase::isHostChannelSelectorSupported(bool* /*defaultR*/,
                                         bool* /*defaultG*/,
                                         bool* /*defaultB*/,
                                         bool* /*defaultA*/) const
{
    return false;
}

StatusEnum
NoOpBase::getTransform(double /*time*/,
                       const RenderScale & /*renderScale*/,
                       bool /*draftRender*/,
                       ViewIdx /*view*/,
                       EffectInstancePtr* inputToTransform,
                       Transform::Matrix3x3* transform)
{
    *inputToTransform = getInput(0);
    if (!*inputToTransform) {
        return eStatusFailed;
    }
    transform->a = 1.; transform->b = 0.; transform->c = 0.;
    transform->d = 0.; transform->e = 1.; transform->f = 0.;
    transform->g = 0.; transform->h = 0.; transform->i = 1.;

    return eStatusOK;
}

bool
NoOpBase::getInputsHoldingTransform(std::list<int>* inputs) const
{
    inputs->push_back(0);

    return true;
}

NATRON_NAMESPACE_EXIT
