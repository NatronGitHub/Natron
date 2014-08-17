//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */



#include "NoOp.h"


NoOpBase::NoOpBase(boost::shared_ptr<Natron::Node> n)
: Natron::EffectInstance(n)
{
    setSupportsRenderScaleMaybe(eSupportsYes);
}

void
NoOpBase::addAcceptedComponents(int /*inputNb*/,
                                std::list<Natron::ImageComponents>* comps)
{
    comps->push_back(Natron::ImageComponentRGB);
    comps->push_back(Natron::ImageComponentRGBA);
    comps->push_back(Natron::ImageComponentAlpha);
}

void
NoOpBase::addSupportedBitDepth(std::list<Natron::ImageBitDepth>* depths) const
{
    depths->push_back(Natron::IMAGE_BYTE);
    depths->push_back(Natron::IMAGE_SHORT);
    depths->push_back(Natron::IMAGE_FLOAT);
}

bool
NoOpBase::isIdentity(SequenceTime time,
                     const RenderScale& /*scale*/,
                     const RectD& /*rod*/,
                     int /*view*/,
                     SequenceTime* inputTime,
                     int* inputNb)
{
    *inputTime = time;
    *inputNb = 0;
    return true;
}

std::string Dot::getDescription() const
{
    return "Doesn't do anything to the input image, this is used in the node graph to make bends in the links.";
}
