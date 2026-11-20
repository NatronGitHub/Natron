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

#ifndef NATRON_ENGINE_OFXOVERLAYINTERACT_H
#define NATRON_ENGINE_OFXOVERLAYINTERACT_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <QtCore/QByteArray>
#include <QtCore/QPointF>
#include <QtCore/QString>

// ofxhPropertySuite.h:565:37: warning: 'this' pointer cannot be null in well-defined C++ code; comparison may be assumed to always evaluate to true [-Wtautological-undefined-compare]
// clang-format off
CLANG_DIAG_OFF(unknown-pragmas)
CLANG_DIAG_OFF(tautological-undefined-compare) // appeared in clang 3.5
#include <ofxhImageEffect.h>
CLANG_DIAG_ON(tautological-undefined-compare)
CLANG_DIAG_ON(unknown-pragmas)
// clang-format on
#include <ofxNatron.h>

#include "Global/KeySymbols.h"
#include "Engine/EngineFwd.h"
#include "Engine/OfxDrawSuiteGL.h"

NATRON_NAMESPACE_ENTER

/**
 * @brief Convert Natron keyboard modifiers to the kNatronOfxInteractPropKeyModifiers bitmask.
 * The bits follow the OFX key symbols that Natron sends (see QtEnumConvert::fromQtKey()):
 * Qt::ShiftModifier/Key_Shift is Shift, Qt::ControlModifier/Key_Control is Control (Cmd on macOS),
 * Qt::AltModifier/Key_Alt is Alt (Option on macOS) and Qt::MetaModifier/Key_Meta is Meta
 * (the Control key on macOS).
 **/
int ofxKeyModifiersFromNatron(const KeyboardModifiers& modifiers);

class NatronOverlayInteractSupport
{

    bool _hasColorPicker;
    OfxRGBAColourD _lastColorPicker;

protected:

    OverlaySupport* _viewport;

    // the OFX 1.5 DrawSuite of this interact, drawn in _viewport (set at the start of each draw)
    OfxDrawSuiteGL _drawSuite;


public:
    NatronOverlayInteractSupport();

    virtual ~NatronOverlayInteractSupport();

    virtual bool isColorPickerRequired() const = 0;

    /**
     * @brief The kOfxPropKeyString of a key action, from the text the key generates (QKeyEvent::text()):
     * "UTF8 string representing a character key that was pressed, some keys have no UTF8 encoding, in
     * which case this is \"\"". Keys that generate control characters (Return, Tab, Backspace, Escape,
     * Delete, or a key with Ctrl on some systems) have no character: "".
     **/
    static QByteArray keyStringFromText(const QString& text);

    /**
     * @brief The kOfxInteractPropPenViewportPosition of a pen action: "This is in the interact's openGL
     * viewport coordinates, with 0,0 being at the bottom left" (ofxInteract.h). widgetPos is in the
     * coordinates of the calling viewport widget, with 0,0 at the top left, in the units of
     * kOfxInteractPropViewportSize.
     **/
    OfxPointI toPenViewportPosition(const QPointF& widgetPos) const;

    void setHasColorPicker(bool hasPicker)
    {
        _hasColorPicker = hasPicker;
    }

    bool hasColorPicker() const
    {
        return _hasColorPicker;
    }

    void setLastColorPickerColor(const OfxRGBAColourD& color)
    {
        _lastColorPicker = color;
    }

    const OfxRGBAColourD& getLastColorPickerColor() const
    {
        return _lastColorPicker;
    }

    void setCallingViewport(OverlaySupport* viewport);

    OverlaySupport* getLastCallingViewport() const;

    /*Swaps the buffer of the attached viewer*/
    OfxStatus n_swapBuffers();


    /// hooks to kOfxInteractPropViewportSize in the property set
    /// this is actually redundant and is to be deprecated
    void n_getViewportSize(double &width, double &height) const;

    // hooks to live kOfxInteractPropPixelScale in the property set
    void n_getPixelScale(double & xScale, double & yScale) const;

#ifdef OFX_EXTENSIONS_NATRON
    // hooks to live kOfxInteractPropScreenPixelRatio in the property set
    double n_getScreenPixelRatio() const;
#endif
   // hooks to kOfxInteractPropBackgroundColour in the property set
    void n_getBackgroundColour(double &r, double &g, double &b) const;

    // hooks to kOfxInteractPropSuggestedColour and kOfxPropOverlayColour in the property set
    bool n_getSuggestedColour(double &r, double &g, double &b) const;

    // an RAII class to save OpenGL context
    class OGLContextSaver
    {
public:
        OGLContextSaver(OverlaySupport* viewport);

        ~OGLContextSaver();

private:
        OverlaySupport* const _viewport;
    };
};

class OfxImageEffectInstance;
class OfxOverlayInteract
    :  public OFX::Host::ImageEffect::OverlayInteract, public NatronOverlayInteractSupport
{
    int _keyModifiers; // kNatronOfxKeyModifier* bitmask
    OfxImageEffectInstance* _effect;

public:

    OfxOverlayInteract(OfxImageEffectInstance &v,
                       int bitDepthPerComponent,
                       bool hasAlpha);

    virtual ~OfxOverlayInteract()
    {
    }

    // the interact actions are actions of the effect instance (OfxImageEffectInstance::MainThreadAction_RAII)
    virtual OfxStatus callEntry(const char *action, OFX::Host::Property::Set *inArgs) OVERRIDE;

    /**
     * @brief Set the keyboard modifiers of the next interact actions: HostSupport passes them in
     * kNatronOfxInteractPropKeyModifiers (ofxNatron.h extension 13), see getKeyModifiers().
     **/
    void setKeyModifiers(const KeyboardModifiers& modifiers);

    /**
     * @brief Set the keyboard modifiers of the next interact actions to the current state of the
     * keyboard, as given by the calling viewport (for draw and focus actions).
     **/
    void setKeyModifiersFromViewport();

    // HostSupport passes kNatronOfxInteractPropKeyModifiers to the image effect overlays
    // (ofxNatron.h extension 13)
    virtual bool supportsKeyModifiers() const OVERRIDE FINAL
    {
        return true;
    }

    // the value of kNatronOfxInteractPropKeyModifiers in the inArgs of the interact actions
    virtual int getKeyModifiers() const OVERRIDE FINAL
    {
        return _keyModifiers;
    }

    virtual bool isColorPickerRequired() const OVERRIDE FINAL WARN_UNUSED_RETURN;

    /*Swaps the buffer of the attached viewer*/
    virtual OfxStatus swapBuffers() OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return n_swapBuffers();
    }

    /*Calls update() on the attached viewer*/
    virtual OfxStatus redraw() OVERRIDE FINAL WARN_UNUSED_RETURN;

    /// hooks to kOfxInteractPropViewportSize in the property set
    /// this is actually redundant and is to be deprecated
    virtual void getViewportSize(double &width,
                                 double &height) const OVERRIDE FINAL
    {
        n_getViewportSize(width, height);
    }

    // hooks to live kOfxInteractPropPixelScale in the property set
    virtual void getPixelScale(double & xScale,
                               double & yScale) const OVERRIDE FINAL
    {
        n_getPixelScale(xScale, yScale);
    }

#ifdef OFX_EXTENSIONS_NATRON
    // hooks to live kOfxInteractPropScreenPixelRatio in the property set
    virtual double getScreenPixelRatio() const OVERRIDE FINAL
    {
        return n_getScreenPixelRatio();
    }
#endif

   // hooks to kOfxInteractPropBackgroundColour in the property set
    virtual void getBackgroundColour(double &r,
                                     double &g,
                                     double &b) const OVERRIDE FINAL
    {
        n_getBackgroundColour(r, g, b);
    }

    // hooks to kOfxInteractPropSuggestedColour and kOfxPropOverlayColour in the property set
    virtual bool getSuggestedColour(double &r,
                                    double &g,
                                    double &b) const OVERRIDE FINAL WARN_UNUSED_RETURN;

    /// call create instance
    virtual OfxStatus createInstanceAction() OVERRIDE FINAL;

    // OFX 1.5 DrawSuite (OfxDrawSuiteV1): drawn with OpenGL in the calling viewport, like the OpenGL
    // overlays (see OfxDrawSuiteGL). HostSupport has checked the arguments and that
    // kOfxInteractActionDraw is running.
    virtual OfxStatus drawGetColour(OfxStandardColour stdColour,
                                    OfxRGBAColourF *colour) OVERRIDE FINAL
    {
        return _drawSuite.getColour(stdColour, colour);
    }

    virtual OfxStatus drawSetColour(const OfxRGBAColourF *colour) OVERRIDE FINAL
    {
        return _drawSuite.setColour(colour);
    }

    virtual OfxStatus drawSetLineWidth(float width) OVERRIDE FINAL
    {
        return _drawSuite.setLineWidth(width);
    }

    virtual OfxStatus drawSetLineStipple(OfxDrawLineStipplePattern pattern) OVERRIDE FINAL
    {
        return _drawSuite.setLineStipple(pattern);
    }

    virtual OfxStatus drawPrimitive(OfxDrawPrimitive primitive,
                                    const OfxPointD *points,
                                    int pointCount) OVERRIDE FINAL
    {
        return _drawSuite.drawPrimitive(primitive, points, pointCount);
    }

    virtual OfxStatus drawText(const char *text,
                               const OfxPointD *pos,
                               int alignment) OVERRIDE FINAL
    {
        return _drawSuite.drawText(text, pos, alignment);
    }

    // Natron extension 14 (kNatronOfxInteractPropDrawSuiteShadows): the drawSuiteShadows
    // preference. drawAction() passes the same value to the DrawSuite.
    virtual bool drawSuiteShadows() const OVERRIDE FINAL
    {
        return OfxDrawSuiteGL::shadowsPreference();
    }

    // interact action - kOfxInteractActionDraw
    //
    // Params -
    //
    //    time              - the effect time at which changed occurred
    //    renderScale       - the render scale
    virtual OfxStatus drawAction(OfxTime time,
                                 const OfxPointD &renderScale,
                                 int view,
                                 const OfxRGBAColourD* pickerColour) OVERRIDE FINAL;

    // interact action - kOfxInteractActionPenMotion
    //
    // Params  -
    //
    //    time              - the effect time at which changed occurred
    //    renderScale       - the render scale
    //    penX              - the X position
    //    penY              - the Y position
    //    pressure          - the pen pressue 0 to 1
    virtual OfxStatus penMotionAction(OfxTime time,
                                      const OfxPointD &renderScale,
                                      int view,
                                      const OfxRGBAColourD* pickerColour,
                                      const OfxPointD &penPos,
                                      const OfxPointI &penPosViewport,
                                      double pressure) OVERRIDE FINAL;

    // interact action - kOfxInteractActionPenUp
    //
    // Params  -
    //
    //    time              - the effect time at which changed occurred
    //    renderScale       - the render scale
    //    penX              - the X position
    //    penY              - the Y position
    //    pressure          - the pen pressue 0 to 1
    virtual OfxStatus penUpAction(OfxTime time,
                                  const OfxPointD &renderScale,
                                  int view,
                                  const OfxRGBAColourD* pickerColour,
                                  const OfxPointD &penPos,
                                  const OfxPointI &penPosViewport,
                                  double pressure) OVERRIDE FINAL;

    // interact action - kOfxInteractActionPenDown
    //
    // Params  -
    //
    //    time              - the effect time at which changed occurred
    //    renderScale       - the render scale
    //    penX              - the X position
    //    penY              - the Y position
    //    pressure          - the pen pressue 0 to 1
    virtual OfxStatus penDownAction(OfxTime time,
                                    const OfxPointD &renderScale,
                                    int view,
                                    const OfxRGBAColourD* pickerColour,
                                    const OfxPointD &penPos,
                                    const OfxPointI &penPosViewport,
                                    double pressure) OVERRIDE FINAL;

    // interact action - kOfxInteractActionkeyDown
    //
    // Params  -
    //
    //    time              - the effect time at which changed occurred
    //    renderScale       - the render scale
    //    key               - the pressed key
    //    keyString         - the pressed key string
    virtual OfxStatus keyDownAction(OfxTime time,
                                    const OfxPointD &renderScale,
                                    int view,
                                    const OfxRGBAColourD* pickerColour,
                                    int key,
                                    char*   keyString) OVERRIDE FINAL;

    // interact action - kOfxInteractActionkeyUp
    //
    // Params  -
    //
    //    time              - the effect time at which changed occurred
    //    renderScale       - the render scale
    //    key               - the pressed key
    //    keyString         - the pressed key string
    virtual OfxStatus keyUpAction(OfxTime time,
                                  const OfxPointD &renderScale,
                                  int view,
                                  const OfxRGBAColourD* pickerColour,
                                  int key,
                                  char*   keyString) OVERRIDE FINAL;

    // interact action - kOfxInteractActionkeyRepeat
    //
    // Params  -
    //
    //    time              - the effect time at which changed occurred
    //    renderScale       - the render scale
    //    key               - the pressed key
    //    keyString         - the pressed key string
    virtual OfxStatus keyRepeatAction(OfxTime time,
                                      const OfxPointD &renderScale,
                                      int view,
                                      const OfxRGBAColourD* pickerColour,
                                      int key,
                                      char*   keyString) OVERRIDE FINAL;

    // interact action - kOfxInteractActionLoseFocus
    //
    // Params -
    //
    //    time              - the effect time at which changed occurred
    //    renderScale       - the render scale
    virtual OfxStatus gainFocusAction(OfxTime time,
                                      const OfxPointD &renderScale,
                                      int view,
                                      const OfxRGBAColourD* pickerColour) OVERRIDE FINAL;

    // interact action - kOfxInteractActionLoseFocus
    //
    // Params -
    //
    //    time              - the effect time at which changed occurred
    //    renderScale       - the render scale
    virtual OfxStatus loseFocusAction(OfxTime time,
                                      const OfxPointD &renderScale,
                                      int view,
                                      const OfxRGBAColourD* pickerColour) OVERRIDE FINAL;
};

class OfxParamOverlayInteract
    : public OFX::Host::Interact::Instance, public NatronOverlayInteractSupport
{
    OfxImageEffectInstance* _effect; // NULL after destroyInstanceAction()



public:

    OfxParamOverlayInteract(KnobI* knob,
                            OFX::Host::Interact::Descriptor &desc,
                            OfxImageEffectInstance* effect);


    virtual ~OfxParamOverlayInteract()
    {
    }

    // the interact actions are actions of the effect instance (OfxImageEffectInstance::MainThreadAction_RAII)
    virtual OfxStatus callEntry(const char *action, OFX::Host::Property::Set *inArgs) OVERRIDE;

    // the effect instance of the interact, NULL after destroyInstanceAction()
    OfxImageEffectInstance* getEffectInstance() const
    {
        return _effect;
    }

    /**
     * @brief Call kOfxActionDestroyInstance now, while the effect instance is still alive: "An
     * interact instance must always be associated with an effect instance, so it gets created after
     * the effect and destroyed before the effect" (ofxInteracts.rst). The GUI may still hold this
     * object after the effect is gone, so all the later actions fail without calling the plug-in, and
     * the destructor doesn't call kOfxActionDestroyInstance again.
     **/
    void destroyInstanceAction();

    // TODO: keyboard modifiers in parameter interacts (ofxNatron.h extension 13): override
    // supportsKeyModifiers() and getKeyModifiers(), and set the state from CustomParamInteract and
    // CurveWidget before each action (NatronGitHub/Natron#1283). Until then, HostSupport doesn't
    // pass kNatronOfxInteractPropKeyModifiers here, and the plug-ins track the modifier keys.

    virtual bool isColorPickerRequired() const OVERRIDE FINAL WARN_UNUSED_RETURN;

    /*Swaps the buffer of the attached viewer*/
    virtual OfxStatus swapBuffers() OVERRIDE FINAL
    {
        return n_swapBuffers();
    }

    /*Calls update() on all viewers*/
    virtual OfxStatus redraw() OVERRIDE FINAL;

    // interact action - kOfxInteractActionDraw: resets the DrawSuite state
    virtual OfxStatus drawAction(OfxTime time,
                                 const OfxPointD &renderScale,
                                 int view,
                                 const OfxRGBAColourD* pickerColour) OVERRIDE FINAL;

    // OFX 1.5 DrawSuite (OfxDrawSuiteV1): drawn with OpenGL in the calling viewport, like the OpenGL
    // overlays (see OfxDrawSuiteGL). HostSupport has checked the arguments and that
    // kOfxInteractActionDraw is running.
    virtual OfxStatus drawGetColour(OfxStandardColour stdColour,
                                    OfxRGBAColourF *colour) OVERRIDE FINAL
    {
        return _drawSuite.getColour(stdColour, colour);
    }

    virtual OfxStatus drawSetColour(const OfxRGBAColourF *colour) OVERRIDE FINAL
    {
        return _drawSuite.setColour(colour);
    }

    virtual OfxStatus drawSetLineWidth(float width) OVERRIDE FINAL
    {
        return _drawSuite.setLineWidth(width);
    }

    virtual OfxStatus drawSetLineStipple(OfxDrawLineStipplePattern pattern) OVERRIDE FINAL
    {
        return _drawSuite.setLineStipple(pattern);
    }

    virtual OfxStatus drawPrimitive(OfxDrawPrimitive primitive,
                                    const OfxPointD *points,
                                    int pointCount) OVERRIDE FINAL
    {
        return _drawSuite.drawPrimitive(primitive, points, pointCount);
    }

    virtual OfxStatus drawText(const char *text,
                               const OfxPointD *pos,
                               int alignment) OVERRIDE FINAL
    {
        return _drawSuite.drawText(text, pos, alignment);
    }

    // Natron extension 14 (kNatronOfxInteractPropDrawSuiteShadows): the drawSuiteShadows
    // preference. drawAction() passes the same value to the DrawSuite.
    virtual bool drawSuiteShadows() const OVERRIDE FINAL
    {
        return OfxDrawSuiteGL::shadowsPreference();
    }


    /// hooks to kOfxInteractPropViewportSize in the property set
    /// this is actually redundant and is to be deprecated
    virtual void getViewportSize(double &width,
                                 double &height) const OVERRIDE FINAL
    {
        n_getViewportSize(width, height);
    }

    // hooks to live kOfxInteractPropPixelScale in the property set
    virtual void getPixelScale(double & xScale,
                               double & yScale) const OVERRIDE FINAL
    {
        n_getPixelScale(xScale, yScale);
    }

#ifdef OFX_EXTENSIONS_NATRON
    // hooks to live kOfxInteractPropScreenPixelRatio in the property set
    virtual double getScreenPixelRatio() const OVERRIDE FINAL
    {
        return n_getScreenPixelRatio();
    }
#endif

    // hooks to kOfxInteractPropBackgroundColour in the property set
    virtual void getBackgroundColour(double &r,
                                     double &g,
                                     double &b) const OVERRIDE FINAL
    {
        n_getBackgroundColour(r, g, b);
    }

    // hooks to kOfxInteractPropSuggestedColour and kOfxPropOverlayColour in the property set
    virtual bool getSuggestedColour(double &r,
                                    double &g,
                                    double &b) const OVERRIDE FINAL
    {
        return n_getSuggestedColour(r, g, b);
    }

    void getMinimumSize(double & minW, double & minH) const;

    void getPreferredSize(int & pW, int & pH) const;

    void getSize(int &w, int &h) const;

    void setSize(int w, int h);

    void getPixelAspectRatio(double & par) const;
};


NATRON_NAMESPACE_EXIT

#endif // NATRON_ENGINE_OFXOVERLAYINTERACT_H
