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

#include "OfxOverlayInteract.h"

#include "Global/Macros.h"
#include "Engine/OfxImageEffectInstance.h"
#include "Engine/OfxEffectInstance.h"
#include "Engine/Format.h"
#include "Engine/OverlaySupport.h"
#include "Engine/Knob.h"
#include "Engine/Node.h"
#include "Engine/AppInstance.h"


NATRON_NAMESPACE_USING

NatronOverlayInteractSupport::OGLContextSaver::OGLContextSaver(OverlaySupport* viewport)
: _viewport(viewport)
{
    assert(_viewport);
    _viewport->saveOpenGLContext();
}

NatronOverlayInteractSupport::OGLContextSaver::~OGLContextSaver() {
    _viewport->restoreOpenGLContext();
}


OfxOverlayInteract::OfxOverlayInteract(OfxImageEffectInstance &v,
                                       int bitDepthPerComponent,
                                       bool hasAlpha)
    : OFX::Host::ImageEffect::OverlayInteract(v,bitDepthPerComponent,hasAlpha)
      , NatronOverlayInteractSupport()
{

}

////////////////////////////////////////////////////////////////////////////////////////////
// protect all OpenGL attribs from anything wrong that could be done in interact functions
// Should this be done in the GUI?
// Probably not: the fact that interacts are OpenGL
OfxStatus
OfxOverlayInteract::createInstanceAction()
{
    //OGLContextSaver s(_viewport);
    return OFX::Host::ImageEffect::OverlayInteract::createInstanceAction();
}

OfxStatus
OfxOverlayInteract::drawAction(OfxTime time,
                               const OfxPointD &renderScale,
                               int view)
{
    NatronOverlayInteractSupport::OGLContextSaver s(_viewport);
    OfxStatus stat = OFX::Host::ImageEffect::OverlayInteract::drawAction(time, renderScale, view);
    return stat;
}

OfxStatus
OfxOverlayInteract::penMotionAction(OfxTime time,
                                    const OfxPointD &renderScale,
                                    int view,
                                    const OfxPointD &penPos,
                                    const OfxPointI &penPosViewport,
                                    double  pressure)
{
    //OGLContextSaver s(_viewport);
    return OFX::Host::ImageEffect::OverlayInteract::penMotionAction(time, renderScale, view, penPos, penPosViewport, pressure);

}

OfxStatus
OfxOverlayInteract::penUpAction(OfxTime time,
                                const OfxPointD &renderScale,
                                int view,
                                const OfxPointD &penPos,
                                const OfxPointI &penPosViewport,
                                double pressure)
{
    return OFX::Host::ImageEffect::OverlayInteract::penUpAction(time, renderScale, view, penPos, penPosViewport, pressure);

}

OfxStatus
OfxOverlayInteract::penDownAction(OfxTime time,
                                  const OfxPointD &renderScale,
                                  int view,
                                  const OfxPointD &penPos,
                                  const OfxPointI &penPosViewport,
                                  double pressure)
{
    return OFX::Host::ImageEffect::OverlayInteract::penDownAction(time, renderScale, view, penPos, penPosViewport, pressure);

}

OfxStatus
OfxOverlayInteract::keyDownAction(OfxTime time,
                                  const OfxPointD &renderScale,
                                  int view,
                                  int     key,
                                  char*   keyString)
{
    return OFX::Host::ImageEffect::OverlayInteract::keyDownAction(time, renderScale, view, key, keyString);

}

OfxStatus
OfxOverlayInteract::keyUpAction(OfxTime time,
                                const OfxPointD &renderScale,
                                int view,
                                int     key,
                                char*   keyString)
{
    return OFX::Host::ImageEffect::OverlayInteract::keyUpAction(time, renderScale, view, key, keyString);

}

OfxStatus
OfxOverlayInteract::keyRepeatAction(OfxTime time,
                                    const OfxPointD &renderScale,
                                    int view,
                                    int     key,
                                    char*   keyString)
{
    return OFX::Host::ImageEffect::OverlayInteract::keyRepeatAction(time, renderScale, view, key, keyString);
}

OfxStatus
OfxOverlayInteract::gainFocusAction(OfxTime time,
                                    const OfxPointD &renderScale,
                                    int view)
{
    return OFX::Host::ImageEffect::OverlayInteract::gainFocusAction(time, renderScale, view);

}

OfxStatus
OfxOverlayInteract::loseFocusAction(OfxTime  time,
                                    const OfxPointD &renderScale,
                                    int view)
{
    return OFX::Host::ImageEffect::OverlayInteract::loseFocusAction(time, renderScale, view);
}

bool
Natron::OfxOverlayInteract::getSuggestedColour(double &r,
                                               double &g,
                                               double &b) const
{
    OfxImageEffectInstance* effect = dynamic_cast<OfxImageEffectInstance*>(&_instance);
    assert(effect && effect->getOfxEffectInstance());
    return effect->getOfxEffectInstance()->getNode()->getOverlayColor(&r, &g, &b);
}

OfxStatus
OfxOverlayInteract::redraw()
{
    OfxImageEffectInstance* effect = dynamic_cast<OfxImageEffectInstance*>(&_instance);
    assert(effect);
    if (effect && effect->getOfxEffectInstance()->getNode()->shouldDrawOverlay()) {
        AppInstance* app =  effect->getOfxEffectInstance()->getApp();
        assert(app);
        if (effect->getOfxEffectInstance()->isDoingInteractAction()) {
            app->queueRedrawForAllViewers();
        } else {
            app->redrawAllViewers();
        }
    }
    return kOfxStatOK;
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

OverlaySupport*
NatronOverlayInteractSupport::getLastCallingViewport() const
{
    return _viewport;
}

OfxStatus
NatronOverlayInteractSupport::n_swapBuffers()
{
    if (_viewport) {
        _viewport->swapOpenGLBuffers();
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

bool
NatronOverlayInteractSupport::n_getSuggestedColour(double &/*r*/,
                                                   double &/*g*/,
                                                   double &/*b*/) const
{
    return false;
}

void
Natron::OfxParamOverlayInteract::getMinimumSize(double & minW,
                                                double & minH) const
{
    minW = _descriptor.getProperties().getDoubleProperty(kOfxParamPropInteractMinimumSize,0);
    minH = _descriptor.getProperties().getDoubleProperty(kOfxParamPropInteractMinimumSize,1);
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
Natron::OfxParamOverlayInteract::getPixelAspectRatio(double & par) const
{
    par = _descriptor.getProperties().getDoubleProperty(kOfxParamPropInteractSizeAspect);
}


OfxStatus
Natron::OfxParamOverlayInteract::redraw()
{
    if (_viewport) {
        _viewport->redraw();
    }
    
    return kOfxStatOK;
}

