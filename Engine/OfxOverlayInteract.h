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

#include "Engine/OverlayInteractBase.h"


NATRON_NAMESPACE_ENTER


class OfxOverlayInteract
    : public OFX::Host::Interact::Instance
    , public OverlayInteractBase
{
public:

    OfxOverlayInteract(OfxImageEffectInstance* v,
                       int bitDepthPerComponent,
                       bool hasAlpha);

    OfxOverlayInteract(const KnobIPtr& knob,
                        OFX::Host::Interact::Descriptor* desc,
                        OfxImageEffectInstance* v);

    virtual ~OfxOverlayInteract()
    {
    }


    ///////// Overriden from OFX::Host::ImageEffect::OverlayInteract
    virtual OfxStatus swapBuffers() OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        swapOpenGLBuffers();
        return kOfxStatOK;
    }
    virtual OfxStatus redraw() OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        OverlayInteractBase::redraw();
        return kOfxStatOK;
    }

    virtual void getViewportSize(double &width,
                                 double &height) const OVERRIDE FINAL
    {
        OverlayInteractBase::getViewportSize(width, height);
    }

    virtual void getPixelScale(double & xScale,
                               double & yScale) const OVERRIDE FINAL
    {
        OverlayInteractBase::getPixelScale(xScale, yScale);
    }

#ifdef OFX_EXTENSIONS_NATRON
    // hooks to live kOfxInteractPropScreenPixelRatio in the property set
    virtual double getScreenPixelRatio() const OVERRIDE FINAL
    {
        return OverlayInteractBase::getScreenPixelRatio();
    }
#endif

    virtual void getBackgroundColour(double &r,
                                     double &g,
                                     double &b) const OVERRIDE FINAL
    {
        OverlayInteractBase::getBackgroundColour(r, g, b);
    }

    virtual bool getSuggestedColour(double &r,
                                    double &g,
                                    double &b) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    //////////

    ///////// Overriden from OverlayInteractBase
    virtual bool isColorPickerRequired() const OVERRIDE FINAL WARN_UNUSED_RETURN;

    virtual void onViewportSelectionCleared() OVERRIDE FINAL;

    virtual void onViewportSelectionUpdated(const RectD& rectangle, bool onRelease) OVERRIDE FINAL;

    virtual void getMinimumSize(double & minW, double & minH) const OVERRIDE FINAL;

    virtual void getPreferredSize(int & pW, int & pH) const OVERRIDE FINAL;

    virtual void getSize(int &w, int &h) const OVERRIDE FINAL;

    virtual void setSize(int w, int h) OVERRIDE FINAL;

    virtual void getPixelAspectRatio(double & par) const OVERRIDE FINAL;
    ///////////

private:

    ///////// Overriden from OverlayInteractBase
    virtual void drawOverlay(TimeValue time, const RenderScale & renderScale, ViewIdx view) OVERRIDE FINAL;
    virtual bool onOverlayPenDown(TimeValue time, const RenderScale & renderScale, ViewIdx view, const QPointF & viewportPos, const QPointF & pos, double pressure, TimeValue timestamp, PenType pen) OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool onOverlayPenMotion(TimeValue time, const RenderScale & renderScale, ViewIdx view,
                                    const QPointF & viewportPos, const QPointF & pos, double pressure, TimeValue timestamp) OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool onOverlayPenUp(TimeValue time, const RenderScale & renderScale, ViewIdx view, const QPointF & viewportPos, const QPointF & pos, double pressure, TimeValue timestamp) OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool onOverlayKeyDown(TimeValue time, const RenderScale & renderScale, ViewIdx view, Key key, KeyboardModifiers modifiers) OVERRIDE FINAL;
    virtual bool onOverlayKeyUp(TimeValue time, const RenderScale & renderScale, ViewIdx view, Key key, KeyboardModifiers modifiers) OVERRIDE FINAL;
    virtual bool onOverlayKeyRepeat(TimeValue time, const RenderScale & renderScale, ViewIdx view, Key key, KeyboardModifiers modifiers) OVERRIDE FINAL;
    virtual bool onOverlayFocusGained(TimeValue time, const RenderScale & renderScale, ViewIdx view) OVERRIDE FINAL;
    virtual bool onOverlayFocusLost(TimeValue time, const RenderScale & renderScale, ViewIdx view) OVERRIDE FINAL;
};


NATRON_NAMESPACE_EXIT

#endif // NATRON_ENGINE_OFXOVERLAYINTERACT_H
