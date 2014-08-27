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

#include <ofxhImageEffect.h>

class OverlaySupport;
class KnobI;

namespace Natron {
class NatronOverlayInteractSupport
{
    OverlaySupport* _viewport;

public:
    NatronOverlayInteractSupport();

    virtual ~NatronOverlayInteractSupport();

    void setCallingViewport(OverlaySupport* viewport);

    /*Swaps the buffer of the attached viewer*/
    OfxStatus n_swapBuffers();

    /*Calls updateGL() on the attached viewer*/
    OfxStatus n_redraw();

    /// hooks to kOfxInteractPropViewportSize in the property set
    /// this is actually redundant and is to be deprecated
    void n_getViewportSize(double &width, double &height) const;

    // hooks to live kOfxInteractPropPixelScale in the property set
    void n_getPixelScale(double & xScale, double & yScale) const;

    // hooks to kOfxInteractPropBackgroundColour in the property set
    void n_getBackgroundColour(double &r, double &g, double &b) const;

    // hooks to kOfxPropOverlayColour in the property set
    void n_getOverlayColour(double &r, double &g, double &b) const;
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
    virtual OfxStatus swapBuffers()
    {
        return n_swapBuffers();
    }

    /*Calls updateGL() on the attached viewer*/
    virtual OfxStatus redraw()
    {
        return n_redraw();
    }

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

#ifdef OFX_EXTENSIONS_NUKE
    // hooks to kOfxPropOverlayColour in the property set
    virtual void getOverlayColour(double &r,
                                  double &g,
                                  double &b) const
    {
        n_getOverlayColour(r, g, b);
    }

#endif
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

    /*Calls updateGL() on the attached viewer*/
    virtual OfxStatus redraw()
    {
        return n_redraw();
    }

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

#ifdef OFX_EXTENSIONS_NUKE
    // hooks to kOfxPropOverlayColour in the property set
    virtual void getOverlayColour(double &r,
                                  double &g,
                                  double &b) const
    {
        n_getOverlayColour(r, g, b);
    }

#endif

    void getMinimumSize(int & minW,int & minH) const;

    void getPreferredSize(int & pW,int & pH) const;

    void getSize(int &w,int &h) const;

    void setSize(int w,int h);

    void getPixelAspect(double & par) const;
};
}

#endif // NATRON_ENGINE_OFXOVERLAYINTERACT_H_
