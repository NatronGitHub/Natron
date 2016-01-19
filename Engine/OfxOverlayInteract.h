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

#ifndef NATRON_ENGINE_OFXOVERLAYINTERACT_H
#define NATRON_ENGINE_OFXOVERLAYINTERACT_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"
// ofxhPropertySuite.h:565:37: warning: 'this' pointer cannot be null in well-defined C++ code; comparison may be assumed to always evaluate to true [-Wtautological-undefined-compare]
CLANG_DIAG_OFF(unknown-pragmas)
CLANG_DIAG_OFF(tautological-undefined-compare) // appeared in clang 3.5
#include <ofxhImageEffect.h>
CLANG_DIAG_ON(tautological-undefined-compare)
CLANG_DIAG_ON(unknown-pragmas)
#include "Engine/EngineFwd.h"


NATRON_NAMESPACE_ENTER;
class NatronOverlayInteractSupport
{
    
protected:
    
    OverlaySupport* _viewport;

public:
    NatronOverlayInteractSupport();

    virtual ~NatronOverlayInteractSupport();

    void setCallingViewport(OverlaySupport* viewport);
    
    OverlaySupport* getLastCallingViewport() const;

    /*Swaps the buffer of the attached viewer*/
    OfxStatus n_swapBuffers();


    /// hooks to kOfxInteractPropViewportSize in the property set
    /// this is actually redundant and is to be deprecated
    void n_getViewportSize(double &width, double &height) const;

    // hooks to live kOfxInteractPropPixelScale in the property set
    void n_getPixelScale(double & xScale, double & yScale) const;

    // hooks to kOfxInteractPropBackgroundColour in the property set
    void n_getBackgroundColour(double &r, double &g, double &b) const;

    // hooks to kOfxInteractPropSuggestedColour and kOfxPropOverlayColour in the property set
    bool n_getSuggestedColour(double &r, double &g, double &b) const;
    
    // an RAII class to save OpenGL context
    class OGLContextSaver {
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
public:

    OfxOverlayInteract(OfxImageEffectInstance &v,
                       int bitDepthPerComponent = 8,
                       bool hasAlpha = false);

    virtual ~OfxOverlayInteract()
    {
    }

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

    // interact action - kOfxInteractActionDraw
    //
    // Params -
    //
    //    time              - the effect time at which changed occured
    //    renderScale       - the render scale
    virtual OfxStatus drawAction(OfxTime time,
                                 const OfxPointD &renderScale,
                                 int view) OVERRIDE FINAL;

    // interact action - kOfxInteractActionPenMotion
    //
    // Params  -
    //
    //    time              - the effect time at which changed occured
    //    renderScale       - the render scale
    //    penX              - the X position
    //    penY              - the Y position
    //    pressure          - the pen pressue 0 to 1
    virtual OfxStatus penMotionAction(OfxTime time,
                                      const OfxPointD &renderScale,
                                      int view,
                                      const OfxPointD &penPos,
                                      const OfxPointI &penPosViewport,
                                      double pressure) OVERRIDE FINAL;

    // interact action - kOfxInteractActionPenUp
    //
    // Params  -
    //
    //    time              - the effect time at which changed occured
    //    renderScale       - the render scale
    //    penX              - the X position
    //    penY              - the Y position
    //    pressure          - the pen pressue 0 to 1
    virtual OfxStatus penUpAction(OfxTime time,
                                  const OfxPointD &renderScale,
                                  int view,
                                  const OfxPointD &penPos,
                                  const OfxPointI &penPosViewport,
                                  double  pressure) OVERRIDE FINAL;

    // interact action - kOfxInteractActionPenDown
    //
    // Params  -
    //
    //    time              - the effect time at which changed occured
    //    renderScale       - the render scale
    //    penX              - the X position
    //    penY              - the Y position
    //    pressure          - the pen pressue 0 to 1
    virtual OfxStatus penDownAction(OfxTime time,
                                    const OfxPointD &renderScale,
                                    int view,
                                    const OfxPointD &penPos,
                                    const OfxPointI &penPosViewport,
                                    double  pressure) OVERRIDE FINAL;

    // interact action - kOfxInteractActionkeyDown
    //
    // Params  -
    //
    //    time              - the effect time at which changed occured
    //    renderScale       - the render scale
    //    key               - the pressed key
    //    keyString         - the pressed key string
    virtual OfxStatus keyDownAction(OfxTime time,
                                    const OfxPointD &renderScale,
                                    int view,
                                    int     key,
                                    char*   keyString) OVERRIDE FINAL;

    // interact action - kOfxInteractActionkeyUp
    //
    // Params  -
    //
    //    time              - the effect time at which changed occured
    //    renderScale       - the render scale
    //    key               - the pressed key
    //    keyString         - the pressed key string
    virtual OfxStatus keyUpAction(OfxTime time,
                                  const OfxPointD &renderScale,
                                  int view,
                                  int     key,
                                  char*   keyString) OVERRIDE FINAL;

    // interact action - kOfxInteractActionkeyRepeat
    //
    // Params  -
    //
    //    time              - the effect time at which changed occured
    //    renderScale       - the render scale
    //    key               - the pressed key
    //    keyString         - the pressed key string
    virtual OfxStatus keyRepeatAction(OfxTime time,
                                      const OfxPointD &renderScale,
                                      int view,
                                      int     key,
                                      char*   keyString) OVERRIDE FINAL;

    // interact action - kOfxInteractActionLoseFocus
    //
    // Params -
    //
    //    time              - the effect time at which changed occured
    //    renderScale       - the render scale
    virtual OfxStatus gainFocusAction(OfxTime time,
                                      const OfxPointD &renderScale,
                                      int view) OVERRIDE FINAL;

    // interact action - kOfxInteractActionLoseFocus
    //
    // Params -
    //
    //    time              - the effect time at which changed occured
    //    renderScale       - the render scale
    virtual OfxStatus loseFocusAction(OfxTime  time,
                                      const OfxPointD &renderScale,
                                      int view) OVERRIDE FINAL;

};

class OfxParamOverlayInteract
    : public OFX::Host::Interact::Instance, public NatronOverlayInteractSupport
{
public:

    OfxParamOverlayInteract(KnobI* knob,
                            OFX::Host::Interact::Descriptor &desc,
                            void *effectInstance);


    virtual ~OfxParamOverlayInteract()
    {
    }

    /*Swaps the buffer of the attached viewer*/
    virtual OfxStatus swapBuffers()
    {
        return n_swapBuffers();
    }

    /*Calls update() on all viewers*/
    virtual OfxStatus redraw();


    /// hooks to kOfxInteractPropViewportSize in the property set
    /// this is actually redundant and is to be deprecated
    virtual void getViewportSize(double &width,
                                 double &height) const
    {
        n_getViewportSize(width, height);
    }

    // hooks to live kOfxInteractPropPixelScale in the property set
    virtual void getPixelScale(double & xScale,
                               double & yScale) const
    {
        n_getPixelScale(xScale, yScale);
    }

    // hooks to kOfxInteractPropBackgroundColour in the property set
    virtual void getBackgroundColour(double &r,
                                     double &g,
                                     double &b) const
    {
        n_getBackgroundColour(r, g, b);
    }

    // hooks to kOfxInteractPropSuggestedColour and kOfxPropOverlayColour in the property set
    virtual bool getSuggestedColour(double &r,
                                    double &g,
                                    double &b) const
    {
        return n_getSuggestedColour(r, g, b);
    }

    void getMinimumSize(double & minW, double & minH) const;

    void getPreferredSize(int & pW, int & pH) const;

    void getSize(int &w,int &h) const;

    void setSize(int w,int h);

    void getPixelAspectRatio(double & par) const;
};
}

#endif // NATRON_ENGINE_OFXOVERLAYINTERACT_H
