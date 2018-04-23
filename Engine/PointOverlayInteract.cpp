/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#include "PointOverlayInteract.h"

#include <cmath>

#include "Engine/AppManager.h"
#include "Engine/OSGLFunctions.h"
#include "Engine/EffectInstance.h"
#include "Engine/OverlaySupport.h"
#include "Engine/Settings.h"
#include "Engine/KnobTypes.h"

NATRON_NAMESPACE_ENTER


enum PositionInteractState
{
    ePositionInteractStateInactive,
    ePositionInteractStatePoised,
    ePositionInteractStatePicked
};


struct PointOverlayInteractPrivate
{
    KnobDoubleWPtr param;
    KnobBoolWPtr interactive;
    QPointF dragPos;
    QPointF lastPenPos;
    bool interactiveDrag;
    PositionInteractState state;

    PointOverlayInteractPrivate()
    : param()
    , interactive()
    , dragPos()
    , lastPenPos()
    , interactiveDrag(false)
    , state(ePositionInteractStateInactive)
    {

    }

    static double pointSize()
    {
        return TO_DPIX(5);
    }

    static double pointTolerance()
    {
        return TO_DPIX(12.);
    }

    bool getInteractive() const
    {
        KnobBoolPtr inter = interactive.lock();
        if (inter) {
            return inter->getValue();
        } else {
            return !appPTR->getCurrentSettings()->getRenderOnEditingFinishedOnly();
        }
    }

};

PointOverlayInteract::PointOverlayInteract()
: OverlayInteractBase()
, _imp(new PointOverlayInteractPrivate())
{
}

PointOverlayInteract::~PointOverlayInteract()
{

}

void
PointOverlayInteract::describeKnobs(OverlayInteractBase::KnobsDescMap* desc) const
{
    {
        OverlayInteractBase::KnobDesc& knob = (*desc)["position"];
        knob.type = KnobDouble::typeNameStatic();
        knob.nDims = 2;
        knob.optional = false;
    }
    {

        OverlayInteractBase::KnobDesc& knob = (*desc)["interactive"];
        knob.type = KnobBool::typeNameStatic();
        knob.nDims = 1;
        knob.optional = true;
    }

}

void
PointOverlayInteract::fetchKnobs(const std::map<std::string, std::string>& knobs)
{
    _imp->param = getEffectKnobByRole<KnobDouble>(knobs, "position", 2, false);
    _imp->interactive = getEffectKnobByRole<KnobBool>(knobs, "interactive", 1, true);
}

std::string
PointOverlayInteract::getName()
{
    KnobDoublePtr knob = _imp->param.lock();

    if (knob) {
        return knob->getName();
    }
    return "";
}

void
PointOverlayInteract::drawOverlay(TimeValue time,
                                  const RenderScale & /*renderScale*/,
                                  ViewIdx /*view*/)
{
    KnobDoublePtr knob = _imp->param.lock();

    // do not show interact if knob is secret or not enabled
    // see https://github.com/MrKepzie/Natron/issues/932
    if ( !knob || !knob->shouldDrawOverlayInteract()) {
        return;
    }

    ColorRgba<double> color;
    if ( !getOverlayColor(color.r, color.g, color.b) ) {
        color.r = color.g = color.b = 0.8;
    }


    RenderScale pscale;
    getPixelScale(pscale.x, pscale.y);

    float pR = 1.f;
    float pG = 1.f;
    float pB = 1.f;
    switch (_imp->state) {
        case ePositionInteractStateInactive:
            pR = (float)color.r; pG = (float)color.g; pB = (float)color.b; break;
        case ePositionInteractStatePoised:
            pR = 0.f; pG = 1.0f; pB = 0.0f; break;
        case ePositionInteractStatePicked:
            pR = 0.f; pG = 1.0f; pB = 0.0f; break;
    }

    QPointF pos;
    if (_imp->state == ePositionInteractStatePicked) {
        pos = _imp->lastPenPos;
    } else {
        double p[2];
        for (int i = 0; i < 2; ++i) {
            p[i] = knob->getValueAtTime(time, DimIdx(i));
            if (knob->getValueIsNormalized(DimIdx(i)) != eValueIsNormalizedNone) {
                p[i] = knob->denormalize(DimIdx(i), time, p[i]);
            }
        }
        pos.setX(p[0]);
        pos.setY(p[1]);
    }
    //glPushAttrib(GL_ALL_ATTRIB_BITS); // caller is responsible for protecting attribs
    GL_GPU::PointSize( (GLfloat)_imp->pointSize() );
    // Draw everything twice
    // l = 0: shadow
    // l = 1: drawing

    double w, h;
    getViewportSize(w, h);

    GLdouble projection[16];
    GL_GPU::GetDoublev( GL_PROJECTION_MATRIX, projection);
    OfxPointD shadow; // how much to translate GL_PROJECTION to get exactly one pixel on screen
    shadow.x = 2. / (projection[0] * w);
    shadow.y = 2. / (projection[5] * h);


    int fmHeight = getLastCallingViewport()->getWidgetFontHeight();
    for (int l = 0; l < 2; ++l) {
        // shadow (uses GL_PROJECTION)
        GL_GPU::MatrixMode(GL_PROJECTION);
        int direction = (l == 0) ? 1 : -1;
        // translate (1,-1) pixels
        GL_GPU::Translated(direction * shadow.x, -direction * shadow.y, 0);
        GL_GPU::MatrixMode(GL_MODELVIEW); // Modelview should be used on Nuke

        GL_GPU::Color3f(pR * l, pG * l, pB * l);
        GL_GPU::Begin(GL_POINTS);
        GL_GPU::Vertex2d( pos.x(), pos.y() );
        GL_GPU::End();
        getLastCallingViewport()->renderText(pos.x(), pos.y() - ( fmHeight + _imp->pointSize() ) * pscale.y, knob->getOriginalName(), pR*l, pG*l, pB*l, 1.);
    }

} // drawOverlay

// round to the closest int, 1/10 int, etc
// this make parameter editing easier
// pscale is args.pixelScale.x / args.renderScale.x;
// pscale10 is the power of 10 below pscale
inline double
fround(double val,
       double pscale)
{
    double pscale10 = std::pow( 10., std::floor( std::log10(pscale) ) );

    return pscale10 * std::floor(val / pscale10 + 0.5);
}

bool
PointOverlayInteract::onOverlayPenDown(TimeValue time,
                                       const RenderScale & renderScale,
                                       ViewIdx view,
                                       const QPointF & viewportPos,
                                       const QPointF & penPos,
                                       double pressure,
                                       TimeValue timestamp,
                                       PenType /*pen*/)
{
    KnobDoublePtr knob = _imp->param.lock();

    // do not show interact if knob is secret or not enabled
    // see https://github.com/MrKepzie/Natron/issues/932
    if ( !knob || !knob->shouldDrawOverlayInteract() ) {
        return false;
    }

    _imp->lastPenPos = penPos;


    bool motion = onOverlayPenMotion(time, renderScale, view, viewportPos, penPos, pressure, timestamp);
    Q_UNUSED(motion);
    if (_imp->state == ePositionInteractStatePoised) {
        _imp->state = ePositionInteractStatePicked;
        _imp->interactiveDrag = _imp->getInteractive();

        return true;
    }

    return false;

}

bool
PointOverlayInteract::onOverlayPenMotion(TimeValue time,
                                         const RenderScale & /*renderScale*/,
                                         ViewIdx view,
                                         const QPointF & /*viewportPos*/,
                                         const QPointF & penPos,
                                         double /*pressure*/,
                                         TimeValue /*timestamp*/)
{
    KnobDoublePtr knob = _imp->param.lock();

    // do not show interact if knob is secret or not enabled
    // see https://github.com/MrKepzie/Natron/issues/932
    if ( !knob || !knob->shouldDrawOverlayInteract() ) {
        return false;
    }

    RenderScale pscale;
    getPixelScale(pscale.x, pscale.y);

    QPointF pos;
    if (_imp->state == ePositionInteractStatePicked) {
        pos = _imp->lastPenPos;
    } else {
        double p[2];
        for (int i = 0; i < 2; ++i) {
            p[i] = knob->getValueAtTime(time, DimIdx(i));
            if (knob->getValueIsNormalized(DimIdx(i)) != eValueIsNormalizedNone) {
                p[i] = knob->denormalize(DimIdx(i), time, p[i]);
            }
        }
        pos.setX(p[0]);
        pos.setY(p[1]);
    }

    bool didSomething = false;
    bool valuesChanged = false;

    switch (_imp->state) {
        case ePositionInteractStateInactive:
        case ePositionInteractStatePoised: {
            // are we in the box, become 'poised'
            PositionInteractState newState;
            if ( ( std::fabs( penPos.x() - pos.x() ) <= _imp->pointTolerance() * pscale.x) &&
                ( std::fabs( penPos.y() - pos.y() ) <= _imp->pointTolerance() * pscale.y) ) {
                newState = ePositionInteractStatePoised;
            } else {
                newState = ePositionInteractStateInactive;
            }

            if (_imp->state != newState) {
                // state changed, must redraw
                redraw();
            }
            _imp->state = newState;
            //}
            break;
        }

        case ePositionInteractStatePicked: {
            valuesChanged = true;
            break;
        }
    }
    didSomething = (_imp->state == ePositionInteractStatePoised) || (_imp->state == ePositionInteractStatePicked);

    if ( (_imp->state != ePositionInteractStateInactive) && _imp->interactiveDrag && valuesChanged ) {
        std::vector<double> p(2);
        p[0] = fround(_imp->lastPenPos.x(), pscale.x);
        p[1] = fround(_imp->lastPenPos.y(), pscale.y);
        for (int i = 0; i < 2; ++i) {
            if (knob->getValueIsNormalized(DimIdx(i)) != eValueIsNormalizedNone) {
                p[i] = knob->normalize(DimIdx(i), time, p[i]);
            }
        }


        knob->setValueAcrossDimensions(p, DimIdx(0), ViewSetSpec(view), eValueChangedReasonUserEdited);
    }

    _imp->lastPenPos = penPos;

    return (didSomething || valuesChanged);
} // onOverlayPenMotion


bool
PointOverlayInteract::onOverlayPenUp(TimeValue time,
                                     const RenderScale & renderScale,
                                     ViewIdx view,
                                     const QPointF & viewportPos,
                                     const QPointF & penPos,
                                     double pressure,
                                     TimeValue timestamp)
{
    KnobDoublePtr knob = _imp->param.lock();

    // do not show interact if knob is secret or not enabled
    // see https://github.com/MrKepzie/Natron/issues/932
    if ( !knob || !knob->shouldDrawOverlayInteract() ) {
        return false;
    }

    RenderScale pscale;
    getPixelScale(pscale.x, pscale.y);
    
    bool didSomething = false;
    if (_imp->state == ePositionInteractStatePicked) {
        if (!_imp->interactiveDrag) {
            std::vector<double> p(2);
            p[0] = fround(_imp->lastPenPos.x(), pscale.x);
            p[1] = fround(_imp->lastPenPos.y(), pscale.y);
            for (int i = 0; i < 2; ++i) {
                if (knob->getValueIsNormalized(DimIdx(i)) != eValueIsNormalizedNone) {
                    p[i] = knob->normalize(DimIdx(i), time, p[i]);
                }
            }

            knob->setValueAcrossDimensions(p, DimIdx(0), view, eValueChangedReasonUserEdited);
        }

        _imp->state = ePositionInteractStateInactive;
        bool motion = onOverlayPenMotion(time, renderScale, view, viewportPos, penPos, pressure, timestamp);
        Q_UNUSED(motion);
        didSomething = true;
    }
    
    return didSomething;
} // onOverlayPenUp


bool
PointOverlayInteract::onOverlayFocusLost(TimeValue /*time*/,
                                         const RenderScale & /*renderScale*/,
                                         ViewIdx /*view*/)
{
    KnobDoublePtr knob = _imp->param.lock();

    // do not show interact if knob is secret or not enabled
    // see https://github.com/MrKepzie/Natron/issues/932
    if ( !knob || !knob->shouldDrawOverlayInteract() ) {
        return false;
    }

    if (_imp->state != ePositionInteractStateInactive) {
        _imp->state = ePositionInteractStateInactive;
        // state changed, must redraw
        redraw();
    }

    return false;
}

NATRON_NAMESPACE_EXIT
