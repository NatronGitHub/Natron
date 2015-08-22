//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "NoOpBase.h"

#include "Engine/Transform.h" // Matrix3x3

using namespace Natron;

NoOpBase::NoOpBase(boost::shared_ptr<Natron::Node> n)
    : Natron::OutputEffectInstance(n)
{
    setSupportsRenderScaleMaybe(eSupportsYes);
}


void
NoOpBase::addAcceptedComponents(int /*inputNb*/,
                                std::list<Natron::ImageComponents>* comps)
{
    comps->push_back(ImageComponents::getRGBComponents());
    comps->push_back(ImageComponents::getRGBAComponents());
    comps->push_back(ImageComponents::getAlphaComponents());
}


void
NoOpBase::addSupportedBitDepth(std::list<Natron::ImageBitDepthEnum>* depths) const
{
    depths->push_back(Natron::eImageBitDepthByte);
    depths->push_back(Natron::eImageBitDepthShort);
    depths->push_back(Natron::eImageBitDepthFloat);
}


bool
NoOpBase::isIdentity(double time,
                     const RenderScale & /*scale*/,
                     const RectI & /*roi*/,
                     int /*view*/,
                     double* inputTime,
                     int* inputNb)
{
    *inputTime = time;
    *inputNb = 0;

    return true;
}


bool
NoOpBase::isHostChannelSelectorSupported(bool* /*defaultR*/,bool* /*defaultG*/, bool* /*defaultB*/, bool* /*defaultA*/) const
{
    return false;
}


Natron::StatusEnum
NoOpBase::getTransform(double /*time*/,
                       const RenderScale& /*renderScale*/,
                       int /*view*/,
                       Natron::EffectInstance** inputToTransform,
                       Transform::Matrix3x3* transform)
{
    *inputToTransform = getInput(0);
    if (!*inputToTransform) {
        return Natron::eStatusFailed;
    }
    transform->a = 1.; transform->b = 0.; transform->c = 0.;
    transform->d = 0.; transform->e = 1.; transform->f = 0.;
    transform->g = 0.; transform->h = 0.; transform->i = 1.;
    return Natron::eStatusOK;
}


bool
NoOpBase::getInputsHoldingTransform(std::list<int>* inputs) const
{
    inputs->push_back(0);
    return true;
}
