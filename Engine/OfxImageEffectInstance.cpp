//  Powiter
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
//
//  Created by Frédéric Devernay on 03/09/13.
//
//

#include "OfxImageEffectInstance.h"

#include "Engine/OfxNode.h"

using namespace Powiter;

#if 0
OfxImageEffectInstance::OfxImageEffectInstance(OfxNode* parent,
                                               OFX::Host::ImageEffect::ImageEffectPlugin* plugin,
                                               OFX::Host::ImageEffect::Descriptor& desc,
                                               const std::string& context,
                                               bool interactive)
: OFX::Host::ImageEffect::Instance(plugin, desc, context, interactive)
, node(parent)
{
}

// The size of the current project in canonical coordinates.
// The size of a project is a sub set of the kOfxImageEffectPropProjectExtent. For example a
// project may be a PAL SD project, but only be a letter-box within that. The project size is
// the size of this sub window.
void OfxImageEffectInstance::getProjectSize(double& xSize, double& ySize) const {
    assert(node);
    xSize = node->width();
    ySize = node->height();
}

// The offset of the current project in canonical coordinates.
// The offset is related to the kOfxImageEffectPropProjectSize and is the offset from the origin
// of the project 'subwindow'. For example for a PAL SD project that is in letterbox form, the
// project offset is the offset to the bottom left hand corner of the letter box. The project
// offset is in canonical coordinates.
void OfxImageEffectInstance::getProjectOffset(double& xOffset, double& yOffset) const {
    assert(node);
    xOffset = _info->x();
    yOffset = _info->y();
}

// The extent of the current project in canonical coordinates.
// The extent is the size of the 'output' for the current project. See ProjectCoordinateSystems
// for more infomation on the project extent. The extent is in canonical coordinates and only
// returns the top right position, as the extent is always rooted at 0,0. For example a PAL SD
// project would have an extent of 768, 576.
void OfxImageEffectInstance::getProjectExtent(double& xSize, double& ySize) const {
    assert(node);
    xSize = _info->w();
    ySize = _info->h();
}

// The pixel aspect ratio of the current project
double OfxImageEffectInstance::getProjectPixelAspectRatio() const {
    assert(node);
    return info().displayWindow().pixel_aspect();
}

// The duration of the effect
// This contains the duration of the plug-in effect, in frames.
double OfxImageEffectInstance::getEffectDuration() const {
    assert(node);
    return 1.0;
}

// For an instance, this is the frame rate of the project the effect is in.
double OfxImageEffectInstance::getFrameRate() const {
    assert(node);
    return 25.0;
}

/// This is called whenever a param is changed by the plugin so that
/// the recursive instanceChangedAction will be fed the correct frame
double OfxImageEffectInstance::getFrameRecursive() const {
    assert(node);
    return 0.0;
}

/// This is called whenever a param is changed by the plugin so that
/// the recursive instanceChangedAction will be fed the correct
/// renderScale
void OfxImageEffectInstance::getRenderScaleRecursive(double &x, double &y) const {
    assert(node);
    x = y = 1.0;
}
#endif
