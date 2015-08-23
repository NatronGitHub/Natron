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
#ifndef NATRON_ENGINE_OFXOVERLAYINTERACT_H_
#define NATRON_ENGINE_OFXOVERLAYINTERACT_H_

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "Global/Macros.h"
// ofxhPropertySuite.h:565:37: warning: 'this' pointer cannot be null in well-defined C++ code; comparison may be assumed to always evaluate to true [-Wtautological-undefined-compare]
CLANG_DIAG_OFF(unknown-pragmas)
CLANG_DIAG_OFF(tautological-undefined-compare) // appeared in clang 3.5
#include <ofxhImageEffect.h>
CLANG_DIAG_ON(tautological-undefined-compare)
CLANG_DIAG_ON(unknown-pragmas)


class OverlaySupport;
class KnobI;

namespace Natron {
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

    /*Calls updateGL() on the attached viewer*/
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
    virtual OfxStatus drawAction(OfxTime time, const OfxPointD &renderScale) OVERRIDE FINAL;

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
                                      int     key,
                                      char*   keyString) OVERRIDE FINAL;

    // interact action - kOfxInteractActionLoseFocus
    //
    // Params -
    //
    //    time              - the effect time at which changed occured
    //    renderScale       - the render scale
    virtual OfxStatus gainFocusAction(OfxTime time,
                                      const OfxPointD &renderScale) OVERRIDE FINAL;

    // interact action - kOfxInteractActionLoseFocus
    //
    // Params -
    //
    //    time              - the effect time at which changed occured
    //    renderScale       - the render scale
    virtual OfxStatus loseFocusAction(OfxTime  time,
                                      const OfxPointD &renderScale) OVERRIDE FINAL;

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

    /*Calls updateGL() on all viewers*/
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

#endif // NATRON_ENGINE_OFXOVERLAYINTERACT_H_
