//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */


#include "NoOp.h"
#include "Engine/Transform.h"

NoOpBase::NoOpBase(boost::shared_ptr<Natron::Node> n)
    : Natron::EffectInstance(n)
{
    setSupportsRenderScaleMaybe(eSupportsYes);
}

void
NoOpBase::addAcceptedComponents(int /*inputNb*/,
                                std::list<Natron::ImageComponentsEnum>* comps)
{
    comps->push_back(Natron::eImageComponentRGB);
    comps->push_back(Natron::eImageComponentRGBA);
    comps->push_back(Natron::eImageComponentAlpha);
}

void
NoOpBase::addSupportedBitDepth(std::list<Natron::ImageBitDepthEnum>* depths) const
{
    depths->push_back(Natron::eImageBitDepthByte);
    depths->push_back(Natron::eImageBitDepthShort);
    depths->push_back(Natron::eImageBitDepthFloat);
}

bool
NoOpBase::isIdentity(SequenceTime time,
                     const RenderScale & /*scale*/,
                     const RectD & /*rod*/,
                     const double /*par*/,
                     int /*view*/,
                     SequenceTime* inputTime,
                     int* inputNb)
{
    *inputTime = time;
    *inputNb = 0;

    return true;
}

std::string
Dot::getDescription() const
{
    return "Doesn't do anything to the input image, this is used in the node graph to make bends in the links.";
}

Natron::StatusEnum
NoOpBase::getTransform(SequenceTime /*time*/,
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