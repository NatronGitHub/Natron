/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2023 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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
#include <ofxhParam.h>

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
    , _keyModifiers(0)
    , _effect(&v)
{
}

int
ofxKeyModifiersFromNatron(const KeyboardModifiers& modifiers)
{
    int ret = 0;

    if ( modifiers.testFlag(eKeyboardModifierShift) ) {
        ret |= kNatronOfxKeyModifierShift;
    }
    if ( modifiers.testFlag(eKeyboardModifierControl) ) {
        ret |= kNatronOfxKeyModifierControl;
    }
    if ( modifiers.testFlag(eKeyboardModifierAlt) ) {
        ret |= kNatronOfxKeyModifierAlt;
    }
    if ( modifiers.testFlag(eKeyboardModifierMeta) ) {
        ret |= kNatronOfxKeyModifierMeta;
    }

    return ret;
}

void
OfxOverlayInteract::setKeyModifiers(const KeyboardModifiers& modifiers)
{
    _keyModifiers = ofxKeyModifiersFromNatron(modifiers);
}

void
OfxOverlayInteract::setKeyModifiersFromViewport()
{
    OverlaySupport* viewport = getLastCallingViewport();

    setKeyModifiers( viewport ? viewport->getKeyboardModifiers() : KeyboardModifiers() );
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
    // Natron calls kOfxInteractActionDraw from the paintGL() of the viewport (ViewerGL), so its
    // OpenGL context is current, as V1 (OpenGL) overlays need (see OfxHost::preferOverlayInteractV1()).
    NatronOverlayInteractSupport::OGLContextSaver s(_viewport);
    // the same shadows value as the one HostSupport passes to the plug-in
    _drawSuite.beginDraw( _viewport, drawSuiteShadows() );
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
                                                 OfxImageEffectInstance* effect,
                                                 OFX::Host::Param::Instance* param)
    : OFX::Host::Interact::Instance( desc, effect->getHandle() )
    , NatronOverlayInteractSupport()
    , _effect(effect)
    , _param(param)
{
    assert(_param);
    setCallingViewport(knob);
}

OfxStatus
OfxParamOverlayInteract::callEntry(const char *action,
                                   OFX::Host::Property::Set *inArgs)
{
    // _effect is NULL once the interact is destroyed, when the effect may be gone too (see destroyInstanceAction())
    OfxImageEffectInstance::MainThreadAction_RAII mainThreadAction(_effect);

    return OFX::Host::Interact::Instance::callEntry(action, inArgs);
}

OfxStatus
OfxOverlayInteract::callEntry(const char *action,
                              OFX::Host::Property::Set *inArgs)
{
    OfxImageEffectInstance::MainThreadAction_RAII mainThreadAction(_effect);

    return OFX::Host::ImageEffect::OverlayInteract::callEntry(action, inArgs);
}

void
OfxParamOverlayInteract::destroyInstanceAction()
{
    if (_state == OFX::Host::Interact::eCreated) {
        callEntry(kOfxActionDestroyInstance, NULL);
    }
    // Instance::callEntry() doesn't call the plug-in for a failed instance, and ~Instance() only
    // calls kOfxActionDestroyInstance for a created one.
    _state = OFX::Host::Interact::eFailed;
    _effect = NULL;
    // the parameter, and the interact descriptor that it holds (OfxParamToKnob::getInteractDesc()), belong
    // to the effect, which may be gone too: the size functions don't use them any more
    _param = NULL;
}

bool
OfxParamOverlayInteract::isColorPickerRequired() const
{
    return (bool)getProperties().getIntProperty(kNatronOfxInteractColourPicking);
}

QByteArray
NatronOverlayInteractSupport::keyStringFromText(const QString& text)
{
    for (int i = 0; i < text.size(); ++i) {
        if (text.at(i).category() == QChar::Other_Control) {
            return QByteArray();
        }
    }

    return text.toUtf8();
}

OfxPointI
NatronOverlayInteractSupport::toPenViewportPosition(const QPointF& widgetPos) const
{
    double width = 0., height = 0.;

    n_getViewportSize(width, height);
    OfxPointI ret;
    ret.x = (int)widgetPos.x();
    // flip y, as the parameter interacts do (CustomParamInteract)
    ret.y = (height > 0.) ? (int)(height - 1. - widgetPos.y()) : (int)widgetPos.y();

    return ret;
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

// The size properties of a parameter interact. ofxParam.h puts them on the parameter: the plug-in sets
// kOfxParamPropInteractSizeAspect, MinimumSize and PreferedSize on the parameter descriptor (copied by the
// instance), and the host sets kOfxParamPropInteractSize on the parameter instance. The Support library
// (OFX::ParamInteractDescriptor, OFX::ParamInteract::getInteractSize()) uses the interact descriptor and
// instance instead, where Natron declares them too (OfxImageEffectInstance::getOfxParamOverlayInteractDescProps()).
// A value the plug-in set on the parameter (one that is not the default) wins over the interact descriptor's,
// and the current size is set on both. HostSupport does not tell whether the plug-in set a property, so a
// parameter value equal to the default (10x10, an aspect of 1) counts as unset: a plug-in that sets exactly
// that on the parameter, and another value on the interact descriptor, gets the interact descriptor's value.
// After destroyInstanceAction(), _param is NULL, and the interact descriptor, which the parameter holds, may be
// gone too: the getters give the defaults and setSize() does nothing.

void
OfxParamOverlayInteract::getMinimumSize(double & minW,
                                        double & minH) const
{
    // "Any custom interface will not be less than this size"; default 10x10
    minW = minH = 10.;
    if (!_param) {
        return;
    }
    const OFX::Host::Property::Set& paramProps = _param->getProperties();
    minW = paramProps.getDoubleProperty(kOfxParamPropInteractMinimumSize, 0);
    minH = paramProps.getDoubleProperty(kOfxParamPropInteractMinimumSize, 1);
    if ( (minW == 10.) && (minH == 10.) ) {
        minW = _descriptor.getProperties().getDoubleProperty(kOfxParamPropInteractMinimumSize, 0);
        minH = _descriptor.getProperties().getDoubleProperty(kOfxParamPropInteractMinimumSize, 1);
    }
}

void
OfxParamOverlayInteract::getPreferredSize(int & pW,
                                          int & pH) const
{
    // "A host should attempt to set a parameter's custom interface on a parameter to be this size if
    // possible"; default 10x10
    pW = pH = 10;
    if (!_param) {
        return;
    }
    const OFX::Host::Property::Set& paramProps = _param->getProperties();
    pW = paramProps.getIntProperty(kOfxParamPropInteractPreferedSize, 0);
    pH = paramProps.getIntProperty(kOfxParamPropInteractPreferedSize, 1);
    if ( (pW == 10) && (pH == 10) ) {
        pW = _descriptor.getProperties().getIntProperty(kOfxParamPropInteractPreferedSize, 0);
        pH = _descriptor.getProperties().getIntProperty(kOfxParamPropInteractPreferedSize, 1);
    }
}

void
OfxParamOverlayInteract::getSize(int &w,
                                 int &h) const
{
    w = h = 0;
    if (!_param) {
        return;
    }
    w = _descriptor.getProperties().getIntProperty(kOfxParamPropInteractSize, 0);
    h = _descriptor.getProperties().getIntProperty(kOfxParamPropInteractSize, 1);
}

void
OfxParamOverlayInteract::setSize(int w,
                                 int h)
{
    // "set by a host to indicate the current size of a custom interface if the plug-in has one", on the
    // parameter instance, a double x 2 (ofxParam.h), and for the Support library on the interact
    if (!_param) {
        return;
    }
    _param->getProperties().setDoubleProperty(kOfxParamPropInteractSize, w, 0);
    _param->getProperties().setDoubleProperty(kOfxParamPropInteractSize, h, 1);
    _descriptor.getProperties().setIntProperty(kOfxParamPropInteractSize, w, 0);
    _descriptor.getProperties().setIntProperty(kOfxParamPropInteractSize, h, 1);
}

void
OfxParamOverlayInteract::getPixelAspectRatio(double & par) const
{
    // "the custom interface ... will be of a size with this aspect ratio"; default 1
    par = 1.;
    if (!_param) {
        return;
    }
    par = _param->getProperties().getDoubleProperty(kOfxParamPropInteractSizeAspect);
    if (par == 1.) {
        par = _descriptor.getProperties().getDoubleProperty(kOfxParamPropInteractSizeAspect);
    }
}

OfxStatus
OfxParamOverlayInteract::drawAction(OfxTime time,
                                    const OfxPointD &renderScale,
                                    int view,
                                    const OfxRGBAColourD* pickerColour)
{
    // Natron calls kOfxInteractActionDraw from the paintGL() of the parameter's widget
    // (CustomParamInteract, CurveWidget), so its OpenGL context is current, as V1 (OpenGL)
    // interacts need. The same shadows value as the one HostSupport passes to the plug-in.
    _drawSuite.beginDraw( _viewport, drawSuiteShadows() );

    return OFX::Host::Interact::Instance::drawAction(time, renderScale, view, pickerColour);
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
