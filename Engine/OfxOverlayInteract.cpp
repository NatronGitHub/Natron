//  Powiter
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */
#include "OfxOverlayInteract.h"


#include "Engine/OfxImageEffectInstance.h"
#include "Engine/OfxNode.h"

using namespace Powiter;

OfxOverlayInteract::OfxOverlayInteract(OfxImageEffectInstance &v, int bitDepthPerComponent, bool hasAlpha):
OFX::Host::ImageEffect::OverlayInteract(v,bitDepthPerComponent,hasAlpha)
, _node(v.node())
{
}

OfxStatus OfxOverlayInteract::swapBuffers(){
    _node->swapBuffersOfAttachedViewer();
    return kOfxStatOK;
}

OfxStatus OfxOverlayInteract::redraw(){
    _node->redrawInteractOnAttachedViewer();
    return kOfxStatOK;
}

void OfxOverlayInteract::getViewportSize(double &width, double &height) const{
    _node->viewportSizeOfAttachedViewer(width, height);
}

void OfxOverlayInteract::getPixelScale(double& xScale, double& yScale) const{
    _node->pixelScaleOfAttachedViewer(xScale, yScale);
}

void OfxOverlayInteract::getBackgroundColour(double &r, double &g, double &b) const{
    _node->backgroundColorOfAttachedViewer(r, g, b);
}
