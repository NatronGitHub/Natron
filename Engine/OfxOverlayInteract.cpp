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

#include "Gui/CurveWidget.h"

using namespace Natron;

OfxOverlayInteract::OfxOverlayInteract(OfxImageEffectInstance &v, int bitDepthPerComponent, bool hasAlpha,CurveWidget* curveWidget):
OFX::Host::ImageEffect::OverlayInteract(v,bitDepthPerComponent,hasAlpha)
, _curveWidget(curveWidget)
, _node(v.node())
{
}

OfxStatus OfxOverlayInteract::swapBuffers(){
    if(_curveWidget){
        _curveWidget->swapBuffers();
    }else{
        _node->swapBuffersOfAttachedViewer();
    }
    return kOfxStatOK;
}

OfxStatus OfxOverlayInteract::redraw(){
    if(_curveWidget){
        _curveWidget->update();
    }else{
        _node->redrawInteractOnAttachedViewer();
    }
    return kOfxStatOK;
}

void OfxOverlayInteract::getViewportSize(double &width, double &height) const{
    if(_curveWidget){
        width = _curveWidget->width();
        height = _curveWidget->height();
    }else{
        _node->viewportSizeOfAttachedViewer(width, height);
    }
}

void OfxOverlayInteract::getPixelScale(double& xScale, double& yScale) const{
    if(_curveWidget){
        double ap = _curveWidget->getPixelAspectRatio();
        xScale = 1. / ap;
        yScale = ap;
    }else{
        _node->pixelScaleOfAttachedViewer(xScale, yScale);
    }
}

void OfxOverlayInteract::getBackgroundColour(double &r, double &g, double &b) const{
    if(_curveWidget){
        _curveWidget->getBackgroundColor(&r, &g, &b);
    }else{
        _node->backgroundColorOfAttachedViewer(r, g, b);
    }
}

#ifdef OFX_EXTENSIONS_NUKE
void OfxOverlayInteract::getOverlayColour(double &r, double &g, double &b) const{
    r = g = b = 1.;
}
#endif
