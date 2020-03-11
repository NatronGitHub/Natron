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

#include "Engine/EffectInstanceTLSData.h"
#include "Engine/OfxImageEffectInstance.h"
#include "Engine/OfxEffectInstance.h"
#include "Engine/Format.h"
#include "Engine/OverlaySupport.h"
#include "Engine/KnobTypes.h"
#include "Engine/Node.h"
#include "Engine/AppInstance.h"


NATRON_NAMESPACE_ENTER




OfxOverlayInteract::OfxOverlayInteract(OfxImageEffectInstance* v,
                                       int bitDepthPerComponent,
                                       bool hasAlpha)
    : OFX::Host::Interact::Instance(v->getOverlayDescriptor(bitDepthPerComponent, hasAlpha), (void *)(v->getHandle()))
    , Natron::OverlayInteractBase()
{
}



OfxOverlayInteract::OfxOverlayInteract(const KnobIPtr& knob,
                                       OFX::Host::Interact::Descriptor* desc,
                                       OfxImageEffectInstance* v)
    : OFX::Host::Interact::Instance(*desc, v)
    , Natron::OverlayInteractBase(knob)
{
}


bool
OfxOverlayInteract::isColorPickerRequired() const
{
    return (bool)getProperties().getIntProperty(kNatronOfxInteractColourPicking);
}


// overridden from OFX::Host::Interact::Instance
bool
OfxOverlayInteract::getSuggestedColour(double &r,
                                       double &g,
                                       double &b) const
{
    return OverlayInteractBase::getOverlayColor(r, g, b);
}


static OfxRGBAColourD colorToOfxColor(const ColorRgba<double>& c)
{
    OfxRGBAColourD ret = {c.r, c.g, c.g, c.a};
    return ret;
}

void
OfxOverlayInteract::drawOverlay(TimeValue time,
                                const RenderScale & renderScale,
                                ViewIdx view)
{

    EffectInstancePtr effect = getEffect();
    EffectInstanceTLSDataPtr tls = effect->getOrCreateTLSObject();
    EffectActionArgsSetter_RAII actionArgsTls(tls, kOfxInteractActionDraw,  time, view, renderScale
#ifdef DEBUG
                                              , /*canSetValue*/ true
                                              , /*canBeCalledRecursively*/ false
#endif
                                              );

    ThreadIsActionCaller_RAII actionCaller(toOfxEffectInstance(effect));

    OfxRGBAColourD pickerColor;
    bool hasPicker = hasColorPicker();
    if (hasPicker) {
        pickerColor = colorToOfxColor(getLastColorPickerColor());
    }

    drawAction(time, renderScale, view, hasPicker ? &pickerColor : /*colourPicker=*/0);

}


bool
OfxOverlayInteract::onOverlayPenDown(TimeValue time,
                                     const RenderScale & renderScale,
                                     ViewIdx view,
                                     const QPointF & viewportPos,
                                     const QPointF & pos,
                                     double pressure,
                                     TimeValue /*timestamp*/,
                                     PenType /*pen*/)
{

    OfxPointD penPos;
    penPos.x = pos.x();
    penPos.y = pos.y();
    OfxPointI penPosViewport;
    penPosViewport.x = viewportPos.x();
    penPosViewport.y = viewportPos.y();

    EffectInstancePtr effect = getEffect();
    EffectInstanceTLSDataPtr tls = effect->getOrCreateTLSObject();
    EffectActionArgsSetter_RAII actionArgsTls(tls, kOfxInteractActionPenDown, time, view, renderScale
#ifdef DEBUG
                                              , /*canSetValue*/ true
                                              , /*canBeCalledRecursively*/ true
#endif
                                              );

    ThreadIsActionCaller_RAII actionCaller(toOfxEffectInstance(effect));

    OfxRGBAColourD pickerColor;
    bool hasPicker = hasColorPicker();
    if (hasPicker) {
        pickerColor = colorToOfxColor(getLastColorPickerColor());
    }


    OfxStatus stat = penDownAction(time, renderScale, view, hasPicker ? &pickerColor : /*colourPicker=*/0, penPos, penPosViewport, pressure);

    if (stat == kOfxStatOK) {
        return true;
    }
    
    
    return false;
}

bool
OfxOverlayInteract::onOverlayPenMotion(TimeValue time,
                                      const RenderScale & renderScale,
                                      ViewIdx view,
                                      const QPointF & viewportPos,
                                       const QPointF & pos,
                                       double pressure,
                                       TimeValue /*timestamp*/)
{

    OfxPointD penPos;
    penPos.x = pos.x();
    penPos.y = pos.y();
    OfxPointI penPosViewport;
    penPosViewport.x = viewportPos.x();
    penPosViewport.y = viewportPos.y();
    OfxStatus stat;


    EffectInstancePtr effect = getEffect();
    EffectInstanceTLSDataPtr tls = effect->getOrCreateTLSObject();
    EffectActionArgsSetter_RAII actionArgsTls(tls, kOfxInteractActionPenMotion, time, view, renderScale
#ifdef DEBUG
                                              , /*canSetValue*/ true
                                              , /*canBeCalledRecursively*/ true
#endif
                                              );

    ThreadIsActionCaller_RAII actionCaller(toOfxEffectInstance(effect));

    OfxRGBAColourD pickerColor;
    bool hasPicker = hasColorPicker();
    if (hasPicker) {
        pickerColor = colorToOfxColor(getLastColorPickerColor());
    }

    stat = penMotionAction(time, renderScale, view, hasPicker ? &pickerColor : /*colourPicker=*/0, penPos, penPosViewport, pressure);

    if (stat == kOfxStatOK) {
        return true;
    }


    return false;
}

bool
OfxOverlayInteract::onOverlayPenUp(TimeValue time,
                                   const RenderScale & renderScale,
                                   ViewIdx view,
                                   const QPointF & viewportPos,
                                   const QPointF & pos,
                                   double pressure,
                                   TimeValue /*timestamp*/)
{
    OfxPointD penPos;
    penPos.x = pos.x();
    penPos.y = pos.y();
    OfxPointI penPosViewport;
    penPosViewport.x = viewportPos.x();
    penPosViewport.y = viewportPos.y();

    EffectInstancePtr effect = getEffect();
    EffectInstanceTLSDataPtr tls = effect->getOrCreateTLSObject();
    EffectActionArgsSetter_RAII actionArgsTls(tls, kOfxInteractActionPenUp, time, view, renderScale
#ifdef DEBUG
                                              , /*canSetValue*/ true
                                              , /*canBeCalledRecursively*/ true
#endif
                                              );

    ThreadIsActionCaller_RAII actionCaller(toOfxEffectInstance(effect));

    OfxRGBAColourD pickerColor;
    bool hasPicker = hasColorPicker();
    if (hasPicker) {
        pickerColor = colorToOfxColor(getLastColorPickerColor());
    }


    OfxStatus stat = penUpAction(time, renderScale, view, hasPicker ? &pickerColor : /*colourPicker=*/0, penPos, penPosViewport, pressure);
    if (stat == kOfxStatOK) {
        return true;
    }


    return false;
}

bool
OfxOverlayInteract::onOverlayKeyDown(TimeValue time,
                                     const RenderScale & renderScale,
                                     ViewIdx view,
                                     Key key,
                                     KeyboardModifiers /*modifiers*/)
{

    EffectInstancePtr effect = getEffect();
    EffectInstanceTLSDataPtr tls = effect->getOrCreateTLSObject();
    EffectActionArgsSetter_RAII actionArgsTls(tls, kOfxInteractActionKeyDown, time, view, renderScale
#ifdef DEBUG
                                              , /*canSetValue*/ true
                                              , /*canBeCalledRecursively*/ true
#endif
                                              );

    ThreadIsActionCaller_RAII actionCaller(toOfxEffectInstance(effect));

    OfxRGBAColourD pickerColor;
    bool hasPicker = hasColorPicker();
    if (hasPicker) {
        pickerColor = colorToOfxColor(getLastColorPickerColor());
    }

    QByteArray keyStr;
    OfxStatus stat = keyDownAction( time, renderScale, view,hasPicker ? &pickerColor : /*colourPicker=*/0, (int)key, keyStr.data() );

    if (stat == kOfxStatOK) {
        return true;
    }


    return false;
}

bool
OfxOverlayInteract::onOverlayKeyUp(TimeValue time,
                                   const RenderScale & renderScale,
                                   ViewIdx view,
                                   Key key,
                                   KeyboardModifiers /* modifiers*/)
{

    EffectInstancePtr effect = getEffect();
    EffectInstanceTLSDataPtr tls = effect->getOrCreateTLSObject();
    EffectActionArgsSetter_RAII actionArgsTls(tls, kOfxInteractActionKeyUp, time, view, renderScale
#ifdef DEBUG
                                              , /*canSetValue*/ true
                                              , /*canBeCalledRecursively*/ true
#endif

                                              );

    ThreadIsActionCaller_RAII actionCaller(toOfxEffectInstance(effect));

    OfxRGBAColourD pickerColor;
    bool hasPicker = hasColorPicker();
    if (hasPicker) {
        pickerColor = colorToOfxColor(getLastColorPickerColor());
    }


    QByteArray keyStr;
    OfxStatus stat = keyUpAction( time, renderScale, view, hasPicker ? &pickerColor : /*colourPicker=*/0, (int)key, keyStr.data() );

    if (stat == kOfxStatOK) {
        return true;
    }
    
    

    return false;
}

bool
OfxOverlayInteract::onOverlayKeyRepeat(TimeValue time,
                                       const RenderScale & renderScale,
                                       ViewIdx view,
                                       Key key,
                                       KeyboardModifiers /*modifiers*/)
{

    EffectInstancePtr effect = getEffect();
    EffectInstanceTLSDataPtr tls = effect->getOrCreateTLSObject();
    EffectActionArgsSetter_RAII actionArgsTls(tls, kOfxInteractActionKeyRepeat, time, view, renderScale
#ifdef DEBUG
                                              , /*canSetValue*/ true
                                              , /*canBeCalledRecursively*/ true
#endif

                                              );

    ThreadIsActionCaller_RAII actionCaller(toOfxEffectInstance(effect));

    OfxRGBAColourD pickerColor;
    bool hasPicker = hasColorPicker();
    if (hasPicker) {
        pickerColor = colorToOfxColor(getLastColorPickerColor());
    }


    QByteArray keyStr;
    OfxStatus stat = keyRepeatAction( time, renderScale, view, hasPicker ? &pickerColor : /*colourPicker=*/0, (int)key, keyStr.data() );

    if (stat == kOfxStatOK) {
        return true;
    }

    return false;
}

bool
OfxOverlayInteract::onOverlayFocusGained(TimeValue time,
                                         const RenderScale & renderScale,
                                         ViewIdx view)
{

    EffectInstancePtr effect = getEffect();
    EffectInstanceTLSDataPtr tls = effect->getOrCreateTLSObject();
    EffectActionArgsSetter_RAII actionArgsTls(tls, kOfxInteractActionGainFocus, time, view, renderScale
#ifdef DEBUG
                                              , /*canSetValue*/ true
                                              , /*canBeCalledRecursively*/ true
#endif

                                              );

    OfxRGBAColourD pickerColor;
    bool hasPicker = hasColorPicker();
    if (hasPicker) {
        pickerColor = colorToOfxColor(getLastColorPickerColor());
    }


    ThreadIsActionCaller_RAII actionCaller(toOfxEffectInstance(effect));

    OfxStatus stat;
    stat = gainFocusAction(time, renderScale, view, hasPicker ? &pickerColor : /*colourPicker=*/0);
    if (stat == kOfxStatOK) {
        return true;
    }
    
    
    return false;
}

bool
OfxOverlayInteract::onOverlayFocusLost(TimeValue time,
                                       const RenderScale & renderScale,
                                       ViewIdx view)
{

    EffectInstancePtr effect = getEffect();
    EffectInstanceTLSDataPtr tls = effect->getOrCreateTLSObject();
    EffectActionArgsSetter_RAII actionArgsTls(tls, kOfxInteractActionLoseFocus, time, view, renderScale
#ifdef DEBUG
                                              , /*canSetValue*/ true
                                              , /*canBeCalledRecursively*/ true
#endif

                                              );

    ThreadIsActionCaller_RAII actionCaller(toOfxEffectInstance(effect));

    OfxRGBAColourD pickerColor;
    bool hasPicker = hasColorPicker();
    if (hasPicker) {
        pickerColor = colorToOfxColor(getLastColorPickerColor());
    }


    OfxStatus stat;
    stat = loseFocusAction(time, renderScale, view, hasPicker ? &pickerColor : /*colourPicker=*/0);
    if (stat == kOfxStatOK) {
        return true;
    }


    return false;
}


void
OfxOverlayInteract::onViewportSelectionCleared()
{
    OfxEffectInstancePtr effect = toOfxEffectInstance(getEffect());
    if (!effect) {
        return;
    }
    KnobIPtr foundSelKnob = effect->getKnobByName(kNatronOfxImageEffectSelectionRectangle);
    if (!foundSelKnob) {
        return;
    }

    KnobIntPtr isIntKnob = toKnobInt(foundSelKnob);
    if (!isIntKnob) {
        return;
    }


    double propV[4] = {0, 0, 0, 0};
    effect->effectInstance()->getProps().setDoublePropertyN(kNatronOfxImageEffectSelectionRectangle, propV, 4);
    isIntKnob->setValue(0);
}


void
OfxOverlayInteract::onViewportSelectionUpdated(const RectD& rectangle, bool onRelease)
{
    OfxEffectInstancePtr effect = toOfxEffectInstance(getEffect());
    if (!effect) {
        return;
    }
    KnobIPtr foundSelKnob = effect->getKnobByName(kNatronOfxImageEffectSelectionRectangle);
    if (!foundSelKnob) {
        return;
    }

    KnobIntPtr isIntKnob = toKnobInt(foundSelKnob);
    if (!isIntKnob) {
        return;
    }
    
    double propV[4] = {rectangle.x1, rectangle.y1, rectangle.x2, rectangle.y2};
    effect->effectInstance()->getProps().setDoublePropertyN(kNatronOfxImageEffectSelectionRectangle, propV, 4);
    isIntKnob->setValue(onRelease ? 2 : 1);
}


void
OfxOverlayInteract::getMinimumSize(double & minW,
                                        double & minH) const
{
    minW = _descriptor.getProperties().getDoubleProperty(kOfxParamPropInteractMinimumSize, 0);
    minH = _descriptor.getProperties().getDoubleProperty(kOfxParamPropInteractMinimumSize, 1);
}

void
OfxOverlayInteract::getPreferredSize(int & pW,
                                          int & pH) const
{
    pW = _descriptor.getProperties().getIntProperty(kOfxParamPropInteractPreferedSize, 0);
    pH = _descriptor.getProperties().getIntProperty(kOfxParamPropInteractPreferedSize, 1);
}

void
OfxOverlayInteract::getSize(int &w,
                                 int &h) const
{
    w = _descriptor.getProperties().getIntProperty(kOfxParamPropInteractSize, 0);
    h = _descriptor.getProperties().getIntProperty(kOfxParamPropInteractSize, 1);
}

void
OfxOverlayInteract::setSize(int w,
                                 int h)
{
    _descriptor.getProperties().setIntProperty(kOfxParamPropInteractSize, w, 0);
    _descriptor.getProperties().setIntProperty(kOfxParamPropInteractSize, h, 1);
}

void
OfxOverlayInteract::getPixelAspectRatio(double & par) const
{
    par = _descriptor.getProperties().getDoubleProperty(kOfxParamPropInteractSizeAspect);
}


NATRON_NAMESPACE_EXIT
