//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */
#include "OfxOverlayInteract.h"


#include "Engine/OfxImageEffectInstance.h"
#include "Engine/OfxEffectInstance.h"
#include "Engine/Format.h"
#include "Engine/OverlaySupport.h"
#include "Engine/Knob.h"

using namespace Natron;

OfxOverlayInteract::OfxOverlayInteract(OfxImageEffectInstance &v,
                                       int bitDepthPerComponent,
                                       bool hasAlpha)
    : OFX::Host::ImageEffect::OverlayInteract(v,bitDepthPerComponent,hasAlpha)
      , NatronOverlayInteractSupport()
{
}

Natron::OfxParamOverlayInteract::OfxParamOverlayInteract(KnobI* knob,
                                                         OFX::Host::Interact::Descriptor &desc,
                                                         void *effectInstance)
    : OFX::Host::Interact::Instance(desc,effectInstance)
      , NatronOverlayInteractSupport()
{
    setCallingViewport(knob);
}

NatronOverlayInteractSupport::NatronOverlayInteractSupport()
    : _viewport(NULL)
{
}

NatronOverlayInteractSupport::~NatronOverlayInteractSupport()
{
}

void
NatronOverlayInteractSupport::setCallingViewport(OverlaySupport* viewport)
{
    _viewport = viewport;
}

OfxStatus
NatronOverlayInteractSupport::n_swapBuffers()
{
    if (_viewport) {
        _viewport->swapOpenGLBuffers();
    }

    return kOfxStatOK;
}

OfxStatus
NatronOverlayInteractSupport::n_redraw()
{
    if (_viewport) {
        _viewport->redraw();
    }

    return kOfxStatOK;
}

void
NatronOverlayInteractSupport::n_getViewportSize(double &width,
                                                double &height) const
{
    if (_viewport) {
        _viewport->getViewportSize(width,height);
    }
}

void
NatronOverlayInteractSupport::n_getPixelScale(double & xScale,
                                              double & yScale) const
{
    if (_viewport) {
        _viewport->getPixelScale(xScale,yScale);
    }
}

void
NatronOverlayInteractSupport::n_getBackgroundColour(double &r,
                                                    double &g,
                                                    double &b) const
{
    if (_viewport) {
        _viewport->getBackgroundColour(r,g,b);
    }
}

void
NatronOverlayInteractSupport::n_getOverlayColour(double &r,
                                                 double &g,
                                                 double &b) const
{
    r = g = b = 1.;
}

void
Natron::OfxParamOverlayInteract::getMinimumSize(int & minW,
                                                int & minH) const
{
    minW = _descriptor.getProperties().getIntProperty(kOfxParamPropInteractMinimumSize,0);
    minH = _descriptor.getProperties().getIntProperty(kOfxParamPropInteractMinimumSize,1);
}

void
Natron::OfxParamOverlayInteract::getPreferredSize(int & pW,
                                                  int & pH) const
{
    pW = _descriptor.getProperties().getIntProperty(kOfxParamPropInteractPreferedSize,0);
    pH = _descriptor.getProperties().getIntProperty(kOfxParamPropInteractPreferedSize,1);
}

void
Natron::OfxParamOverlayInteract::getSize(int &w,
                                         int &h) const
{
    w = _descriptor.getProperties().getIntProperty(kOfxParamPropInteractSize,0);
    h = _descriptor.getProperties().getIntProperty(kOfxParamPropInteractSize,1);
}

void
Natron::OfxParamOverlayInteract::setSize(int w,
                                         int h)
{
    _descriptor.getProperties().setIntProperty(kOfxParamPropInteractSize,w,0);
    _descriptor.getProperties().setIntProperty(kOfxParamPropInteractSize,h,1);
}

void
Natron::OfxParamOverlayInteract::getPixelAspect(double & par) const
{
    par = _descriptor.getProperties().getDoubleProperty(kOfxParamPropInteractSizeAspect);
}
