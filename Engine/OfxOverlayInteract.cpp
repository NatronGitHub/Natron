/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
 * Copyright (C) 2018-2020 The Natron developers
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

#include <cassert>
#include <stdexcept>

#include <ofxNatron.h>

#include "Engine/OfxImageEffectInstance.h"
#include "Engine/OfxEffectInstance.h"
#include "Engine/Format.h"
#include "Engine/OverlaySupport.h"
#include "Engine/Knob.h"
#include "Engine/Node.h"
#include "Engine/AppInstance.h"


NATRON_NAMESPACE_ENTER

NatronOverlayInteractSupport::OGLContextSaver::OGLContextSaver(OverlaySupport* viewport)
    : _viewport(viewport)
{
    assert(_viewport);
    _viewport->saveOpenGLContext();
}

NatronOverlayInteractSupport::OGLContextSaver::~OGLContextSaver()
{
    _viewport->restoreOpenGLContext();
}

OfxOverlayInteract::OfxOverlayInteract(OfxImageEffectInstance &v,
                                       int bitDepthPerComponent,
                                       bool hasAlpha)
    : OFX::Host::ImageEffect::OverlayInteract(v, bitDepthPerComponent, hasAlpha)
    , NatronOverlayInteractSupport()
{
}

bool
OfxOverlayInteract::isColorPickerRequired() const
{
    return (bool)getProperties().getIntProperty(kNatronOfxInteractColourPicking);
}


// overridden from OFX::Host::Interact::Instance
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

// overridden from OFX::Host::Interact::Instance
OfxStatus
OfxOverlayInteract::drawAction(OfxTime time,
                               const OfxPointD &renderScale,
                               int view,
                               const OfxRGBAColourD* pickerColour)
{
    NatronOverlayInteractSupport::OGLContextSaver s(_viewport);
    OfxStatus stat = OFX::Host::ImageEffect::OverlayInteract::drawAction(time, renderScale, view, pickerColour);

    return stat;
}

// overridden from OFX::Host::Interact::Instance
OfxStatus
OfxOverlayInteract::penMotionAction(OfxTime time,
                                    const OfxPointD &renderScale,
                                    int view,
                                    const OfxRGBAColourD* pickerColour,
                                    const OfxPointD &penPos,
                                    const OfxPointI &penPosViewport,
                                    double pressure)
{
    //OGLContextSaver s(_viewport);
    return OFX::Host::ImageEffect::OverlayInteract::penMotionAction(time, renderScale, view, pickerColour, penPos, penPosViewport, pressure);
}

// overridden from OFX::Host::Interact::Instance
OfxStatus
OfxOverlayInteract::penUpAction(OfxTime time,
                                const OfxPointD &renderScale,
                                int view,
                                const OfxRGBAColourD* pickerColour,
                                const OfxPointD &penPos,
                                const OfxPointI &penPosViewport,
                                double pressure)
{
    return OFX::Host::ImageEffect::OverlayInteract::penUpAction(time, renderScale, view, pickerColour, penPos, penPosViewport, pressure);
}

// overridden from OFX::Host::Interact::Instance
OfxStatus
OfxOverlayInteract::penDownAction(OfxTime time,
                                  const OfxPointD &renderScale,
                                  int view,
                                  const OfxRGBAColourD* pickerColour,
                                  const OfxPointD &penPos,
                                  const OfxPointI &penPosViewport,
                                  double pressure)
{
    return OFX::Host::ImageEffect::OverlayInteract::penDownAction(time, renderScale, view, pickerColour, penPos, penPosViewport, pressure);
}

// overridden from OFX::Host::Interact::Instance
OfxStatus
OfxOverlayInteract::keyDownAction(OfxTime time,
                                  const OfxPointD &renderScale,
                                  int view,
                                  const OfxRGBAColourD* pickerColour,
                                  int key,
                                  char*   keyString)
{
    return OFX::Host::ImageEffect::OverlayInteract::keyDownAction(time, renderScale, view, pickerColour, key, keyString);
}

// overridden from OFX::Host::Interact::Instance
OfxStatus
OfxOverlayInteract::keyUpAction(OfxTime time,
                                const OfxPointD &renderScale,
                                int view,
                                const OfxRGBAColourD* pickerColour,
                                int key,
                                char*   keyString)
{
    return OFX::Host::ImageEffect::OverlayInteract::keyUpAction(time, renderScale, view, pickerColour, key, keyString);
}

// overridden from OFX::Host::Interact::Instance
OfxStatus
OfxOverlayInteract::keyRepeatAction(OfxTime time,
                                    const OfxPointD &renderScale,
                                    int view,
                                    const OfxRGBAColourD* pickerColour,
                                    int key,
                                    char*   keyString)
{
    return OFX::Host::ImageEffect::OverlayInteract::keyRepeatAction(time, renderScale, view, pickerColour, key, keyString);
}

// overridden from OFX::Host::Interact::Instance
OfxStatus
OfxOverlayInteract::gainFocusAction(OfxTime time,
                                    const OfxPointD &renderScale,
                                    int view,
                                    const OfxRGBAColourD* pickerColour)
{
    return OFX::Host::ImageEffect::OverlayInteract::gainFocusAction(time, renderScale, view, pickerColour);
}

// overridden from OFX::Host::Interact::Instance
OfxStatus
OfxOverlayInteract::loseFocusAction(OfxTime time,
                                    const OfxPointD &renderScale,
                                    int view,
                                    const OfxRGBAColourD* pickerColour)
{
    return OFX::Host::ImageEffect::OverlayInteract::loseFocusAction(time, renderScale, view, pickerColour);
}

// overridden from OFX::Host::Interact::Instance
bool
OfxOverlayInteract::getSuggestedColour(double &r,
                                       double &g,
                                       double &b) const
{
    OfxImageEffectInstance* effect = dynamic_cast<OfxImageEffectInstance*>(&_instance);

    assert( effect && effect->getOfxEffectInstance() );

    return effect ? effect->getOfxEffectInstance()->getNode()->getOverlayColor(&r, &g, &b) : false;
}

// overridden from OFX::Host::Interact::Instance
OfxStatus
OfxOverlayInteract::redraw()
{
    OfxImageEffectInstance* effect = dynamic_cast<OfxImageEffectInstance*>(&_instance);

    assert(effect);
    if ( effect && effect->getOfxEffectInstance()->getNode()->shouldDrawOverlay() ) {
        AppInstancePtr app =  effect->getOfxEffectInstance()->getApp();
        assert(app);
        if ( effect->getOfxEffectInstance()->isDoingInteractAction() ) {
            app->queueRedrawForAllViewers();
        } else {
            app->redrawAllViewers();
        }
    }

    return kOfxStatOK;
}

OfxParamOverlayInteract::OfxParamOverlayInteract(KnobI* knob,
                                                 OFX::Host::Interact::Descriptor &desc,
                                                 void *effectInstance)
    : OFX::Host::Interact::Instance(desc, effectInstance)
    , NatronOverlayInteractSupport()
{
    setCallingViewport(knob);
}

bool
OfxParamOverlayInteract::isColorPickerRequired() const
{
    return (bool)getProperties().getIntProperty(kNatronOfxInteractColourPicking);
}

NatronOverlayInteractSupport::NatronOverlayInteractSupport()
    : _hasColorPicker(false)
    , _lastColorPicker()
    , _viewport(NULL)
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
        _viewport->getViewportSize(width, height);
    }
}

void
NatronOverlayInteractSupport::n_getPixelScale(double & xScale,
                                              double & yScale) const
{
    if (_viewport) {
        _viewport->getPixelScale(xScale, yScale);
    }
}

#ifdef OFX_EXTENSIONS_NATRON
// hooks to live kOfxInteractPropScreenPixelRatio in the property set
double
NatronOverlayInteractSupport::n_getScreenPixelRatio() const
{
    if (_viewport) {
        return _viewport->getScreenPixelRatio();
    }
    return 1.;
}
#endif

void
NatronOverlayInteractSupport::n_getBackgroundColour(double &r,
                                                    double &g,
                                                    double &b) const
{
    if (_viewport) {
        _viewport->getBackgroundColour(r, g, b);
    }
}

bool
NatronOverlayInteractSupport::n_getSuggestedColour(double & /*r*/,
                                                   double & /*g*/,
                                                   double & /*b*/) const
{
    return false;
}

void
OfxParamOverlayInteract::getMinimumSize(double & minW,
                                        double & minH) const
{
    minW = _descriptor.getProperties().getDoubleProperty(kOfxParamPropInteractMinimumSize, 0);
    minH = _descriptor.getProperties().getDoubleProperty(kOfxParamPropInteractMinimumSize, 1);
}

void
OfxParamOverlayInteract::getPreferredSize(int & pW,
                                          int & pH) const
{
    pW = _descriptor.getProperties().getIntProperty(kOfxParamPropInteractPreferedSize, 0);
    pH = _descriptor.getProperties().getIntProperty(kOfxParamPropInteractPreferedSize, 1);
}

void
OfxParamOverlayInteract::getSize(int &w,
                                 int &h) const
{
    w = _descriptor.getProperties().getIntProperty(kOfxParamPropInteractSize, 0);
    h = _descriptor.getProperties().getIntProperty(kOfxParamPropInteractSize, 1);
}

void
OfxParamOverlayInteract::setSize(int w,
                                 int h)
{
    _descriptor.getProperties().setIntProperty(kOfxParamPropInteractSize, w, 0);
    _descriptor.getProperties().setIntProperty(kOfxParamPropInteractSize, h, 1);
}

void
OfxParamOverlayInteract::getPixelAspectRatio(double & par) const
{
    par = _descriptor.getProperties().getDoubleProperty(kOfxParamPropInteractSizeAspect);
}

OfxStatus
OfxParamOverlayInteract::redraw()
{
    if (_viewport) {
        _viewport->redraw();
    }

    return kOfxStatOK;
}

NATRON_NAMESPACE_EXIT
