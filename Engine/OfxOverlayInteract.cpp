//  Natron
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
#include "Engine/OfxEffectInstance.h"
#include "Engine/Format.h"
#include "Engine/OverlaySupport.h"

#include "Gui/CurveWidget.h"
#include "Gui/ViewerGL.h"

using namespace Natron;

OfxOverlayInteract::OfxOverlayInteract(OfxImageEffectInstance &v, int bitDepthPerComponent, bool hasAlpha):
OFX::Host::ImageEffect::OverlayInteract(v,bitDepthPerComponent,hasAlpha)
, _viewport(NULL)
{
}

void OfxOverlayInteract::setCallingViewport(OverlaySupport* viewport) {
    _viewport = viewport;
}

OfxStatus OfxOverlayInteract::swapBuffers(){
    if (_viewport) {
        _viewport->swapOpenGLBuffers();
    }
    return kOfxStatOK;
}

OfxStatus OfxOverlayInteract::redraw(){
    if (_viewport) {
        _viewport->redraw();
    }
    return kOfxStatOK;
}

void OfxOverlayInteract::getViewportSize(double &width, double &height) const {

    if (_viewport) {
        _viewport->getViewportSize(width,height);
    }
}

void OfxOverlayInteract::getPixelScale(double& xScale, double& yScale) const{
    if (_viewport) {
        _viewport->getPixelScale(xScale,yScale);
    }
}

void OfxOverlayInteract::getBackgroundColour(double &r, double &g, double &b) const{
    if (_viewport) {
        _viewport->getBackgroundColour(r,g,b);
    }
}

#ifdef OFX_EXTENSIONS_NUKE
void OfxOverlayInteract::getOverlayColour(double &r, double &g, double &b) const{
    r = g = b = 1.;
}
#endif
