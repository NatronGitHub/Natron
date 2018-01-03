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

#include "CornerPinOverlayInteract.h"

#include "Engine/AppManager.h"
#include "Engine/OSGLFunctions.h"
#include "Engine/EffectInstance.h"
#include "Engine/OverlaySupport.h"
#include "Engine/Settings.h"
#include "Engine/KnobTypes.h"

NATRON_NAMESPACE_ENTER

struct CornerPinOverlayInteractPrivate
{
    std::vector<KnobDoubleWPtr > from, to;
    std::vector<KnobBoolWPtr > enable;
    KnobBoolWPtr invert;
    KnobChoiceWPtr overlayPoints;
    KnobBoolWPtr interactive;

    QPointF lastPenPos;
    int dragging; // -1: idle, else dragging point number
    int hovering; // -1: idle, else hovering point number
    Point toDrag[4];
    Point fromDrag[4];
    bool enableDrag[4];
    bool useFromDrag;
    bool interactiveDrag;
    bool toPointsAutoKeyingEnabled;
    bool toPointsEnabledIfNotAnimated;


    CornerPinOverlayInteractPrivate()
    : lastPenPos()
    , dragging(-1)
    , hovering(-1)
    , useFromDrag(false)
    , interactiveDrag(false)
    , toPointsAutoKeyingEnabled(false)
    , toPointsEnabledIfNotAnimated(true)
    {

    }

    static double pointSize() { return TO_DPIX(5.); }

    static double pointTolerance() { return TO_DPIX(12.); }

    static bool isNearby(const QPointF & p,
                         double x,
                         double y,
                         double tolerance,
                         const OfxPointD & pscale)
    {
        return std::fabs(p.x() - x) <= tolerance * pscale.x &&  std::fabs(p.y() - y) <= tolerance * pscale.y;
    }

    void getFrom(TimeValue time,
                 int index,
                 double* tx,
                 double* ty) const
    {
        KnobDoublePtr knob = from[index].lock();

        assert(knob);
        *tx = knob->getValueAtTime(time, DimIdx(0));
        *ty = knob->getValueAtTime(time, DimIdx(1));
    }

    void getTo(TimeValue time,
               int index,
               double* tx,
               double* ty) const
    {
        KnobDoublePtr knob = to[index].lock();

        assert(knob);
        *tx = knob->getValueAtTime(time, DimIdx(0));
        *ty = knob->getValueAtTime(time, DimIdx(1));
    }

    bool getEnabled(TimeValue time,
                    int index) const
    {
        KnobBoolPtr knob = enable[index].lock();

        if (!knob) {
            return true;
        }

        return knob->getValueAtTime(time);
    }

    bool getInverted(TimeValue time) const
    {
        KnobBoolPtr knob = invert.lock();

        if (!knob) {
            return false;
        }

        return knob->getValueAtTime(time);
    }

    bool getUseFromPoints(TimeValue time) const
    {
        KnobChoicePtr knob = overlayPoints.lock();

        assert(knob);

        return knob->getValueAtTime(time) == 1;
    }

    bool getInteractive() const
    {
        if ( interactive.lock() ) {
            return interactive.lock()->getValue();
        } else {
            return !appPTR->getCurrentSettings()->getRenderOnEditingFinishedOnly();
        }
    }

    bool areToPointsAnimated() const
    {
        bool hasAnimation = true;
        for (int i = 0; i < 4; ++i) {
            KnobDoublePtr knob = to[i].lock();
            for (int d = 0; d < 2; ++d) {
                bool dimHasAnim = knob->isAnimated(DimIdx(d), ViewIdx(0));
                if (!dimHasAnim) {
                    hasAnimation = false;
                    break;
                }
            }
            if (!hasAnimation) {
                break;
            }
        }
        return hasAnimation;
    }

};

CornerPinOverlayInteract::CornerPinOverlayInteract()
: OverlayInteractBase()
, _imp(new CornerPinOverlayInteractPrivate())
{

    for (unsigned i = 0; i < 4; ++i) {
        _imp->toDrag[i].x = _imp->toDrag[i].y = _imp->fromDrag[i].x = _imp->fromDrag[i].y = 0.;
    }

}

CornerPinOverlayInteract::~CornerPinOverlayInteract()
{

}

void
CornerPinOverlayInteract::setAutoKeyEnabledOnToPoints(bool enabled)
{
    _imp->toPointsAutoKeyingEnabled = enabled;
}


void
CornerPinOverlayInteract::setToPointsEnabledIfNotAnimated(bool enabled)
{
    _imp->toPointsEnabledIfNotAnimated = enabled;
}

void
CornerPinOverlayInteract::describeKnobs(OverlayInteractBase::KnobsDescMap* desc) const
{
    for (int i = 0; i < 4; ++i) {
        {
            std::string name;
            {
                std::stringstream ss;
                ss << "from" << i + 1;
                name = ss.str();
            }
            OverlayInteractBase::KnobDesc& knob = (*desc)[name];
            knob.type = KnobDouble::typeNameStatic();
            knob.nDims = 2;
            knob.optional = false;
        }
        {
            std::string name;
            {
                std::stringstream ss;
                ss << "to" << i + 1;
                name = ss.str();
            }
            OverlayInteractBase::KnobDesc& knob = (*desc)[name];
            knob.type = KnobDouble::typeNameStatic();
            knob.nDims = 2;
            knob.optional = false;
        }
        {
            std::string name;
            {
                std::stringstream ss;
                ss << "enable" << i + 1;
                name = ss.str();
            }
            OverlayInteractBase::KnobDesc& knob = (*desc)[name];
            knob.type = KnobBool::typeNameStatic();
            knob.nDims = 1;
            knob.optional = false;
        }
    }
    {
        std::string name;
        {
            std::stringstream ss;
            ss << "overlayPoints";
            name = ss.str();
        }
        OverlayInteractBase::KnobDesc& knob = (*desc)[name];
        knob.type = KnobChoice::typeNameStatic();
        knob.nDims = 1;
        knob.optional = false;
    }
    {
        std::string name;
        {
            std::stringstream ss;
            ss << "invert";
            name = ss.str();
        }
        OverlayInteractBase::KnobDesc& knob = (*desc)[name];
        knob.type = KnobBool::typeNameStatic();
        knob.nDims = 1;
        knob.optional = false;
    }
    {
        std::string name;
        {
            std::stringstream ss;
            ss << "interactive";
            name = ss.str();
        }
        OverlayInteractBase::KnobDesc& knob = (*desc)[name];
        knob.type = KnobBool::typeNameStatic();
        knob.nDims = 1;
        knob.optional = true;
    }

}

void
CornerPinOverlayInteract::fetchKnobs(const std::map<std::string, std::string>& knobs)
{
    _imp->from.resize(4);
    _imp->to.resize(4);
    _imp->enable.resize(4);

    for (int i = 0; i < 4; ++i) {
        {
            std::string name;
            {
                std::stringstream ss;
                ss << "from" << i + 1;
                name = ss.str();
            }
            _imp->from[i] = getEffectKnobByRole<KnobDouble>(knobs, name, 2, false);
        }
        {
            std::string name;
            {
                std::stringstream ss;
                ss << "to" << i + 1;
                name = ss.str();
            }
            _imp->to[i] = getEffectKnobByRole<KnobDouble>(knobs, name, 2, false);
        }
        {
            std::string name;
            {
                std::stringstream ss;
                ss << "enable" << i + 1;
                name = ss.str();
            }
            _imp->enable[i] = getEffectKnobByRole<KnobBool>(knobs, name, 1, true);
        }
    }
    _imp->overlayPoints = getEffectKnobByRole<KnobChoice>(knobs, "overlayPoints", 1, false);
    _imp->invert = getEffectKnobByRole<KnobBool>(knobs, "invert", 1, true);
    _imp->interactive = getEffectKnobByRole<KnobBool>(knobs, "interactive", 1, true);
}

void
CornerPinOverlayInteract::drawOverlay(TimeValue time,
                                      const RenderScale & /*renderScale*/,
                                      ViewIdx /*view*/)
{
    // do not show interact if knob is secret or not enabled
    // see https://github.com/MrKepzie/Natron/issues/932
    KnobDoublePtr from1Knob = _imp->from[0].lock();

    if ( !from1Knob || !from1Knob->shouldDrawOverlayInteract() ) {
        return;
    }


    OfxRGBColourD color;
    if ( !getOverlayColor(color.r, color.g, color.b) ) {
        color.r = color.g = color.b = 0.8;
    }

    RenderScale pscale;
    getPixelScale(pscale.x, pscale.y);

    OfxPointD to[4];
    OfxPointD from[4];
    bool enable[4];
    bool useFrom;

    if (_imp->dragging == -1) {
        for (int i = 0; i < 4; ++i) {
            _imp->getFrom(time, i, &from[i].x, &from[i].y);
            _imp->getTo(time, i, &to[i].x, &to[i].y);
            enable[i] = _imp->getEnabled(time, i);
        }
        useFrom = _imp->getUseFromPoints(time);
    } else {
        for (int i = 0; i < 4; ++i) {
            to[i] = _imp->toDrag[i];
            from[i] = _imp->fromDrag[i];
            enable[i] = _imp->enableDrag[i];
        }
        useFrom = _imp->useFromDrag;
    }

    if (!useFrom && !_imp->areToPointsAnimated()) {
        return;
    }

    OfxPointD p[4];
    OfxPointD q[4];
    int enableBegin = 4;
    int enableEnd = 0;
    for (int i = 0; i < 4; ++i) {
        if (enable[i]) {
            if (useFrom) {
                p[i] = from[i];
                q[i] = to[i];
            } else {
                q[i] = from[i];
                p[i] = to[i];
            }
            if (i < enableBegin) {
                enableBegin = i;
            }
            if (i + 1 > enableEnd) {
                enableEnd = i + 1;
            }
        }
    }

    double w, h;
    getViewportSize(w, h);

    GLdouble projection[16];
    GL_GPU::GetDoublev( GL_PROJECTION_MATRIX, projection);
    OfxPointD shadow; // how much to translate GL_PROJECTION to get exactly one pixel on screen
    shadow.x = 2. / (projection[0] * w);
    shadow.y = 2. / (projection[5] * h);


    //glPushAttrib(GL_ALL_ATTRIB_BITS); // caller is responsible for protecting attribs

    //glDisable(GL_LINE_STIPPLE);
    GL_GPU::Enable(GL_LINE_SMOOTH);
    //glEnable(GL_POINT_SMOOTH);
    GL_GPU::Enable(GL_BLEND);
    GL_GPU::Hint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);
    GL_GPU::LineWidth(1.5f);
    GL_GPU::BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    GL_GPU::PointSize( CornerPinOverlayInteractPrivate::pointSize() );
    // Draw everything twice
    // l = 0: shadow
    // l = 1: drawing
    for (int l = 0; l < 2; ++l) {
        // shadow (uses GL_PROJECTION)
        GL_GPU::MatrixMode(GL_PROJECTION);
        int direction = (l == 0) ? 1 : -1;
        // translate (1,-1) pixels
        GL_GPU::Translated(direction * shadow.x, -direction * shadow.y, 0);
        GL_GPU::MatrixMode(GL_MODELVIEW); // Modelview should be used on Nuke

        GL_GPU::Color3f( (float)(color.r / 2) * l, (float)(color.g / 2) * l, (float)(color.b / 2) * l );
        GL_GPU::Begin(GL_LINES);
        for (int i = enableBegin; i < enableEnd; ++i) {
            if (enable[i]) {
                GL_GPU::Vertex2d(p[i].x, p[i].y);
                GL_GPU::Vertex2d(q[i].x, q[i].y);
            }
        }
        GL_GPU::End();
        GL_GPU::Color3f( (float)color.r * l, (float)color.g * l, (float)color.b * l );
        GL_GPU::Begin(GL_LINE_LOOP);
        for (int i = enableBegin; i < enableEnd; ++i) {
            if (enable[i]) {
                GL_GPU::Vertex2d(p[i].x, p[i].y);
            }
        }
        GL_GPU::End();
        GL_GPU::Begin(GL_POINTS);
        for (int i = enableBegin; i < enableEnd; ++i) {
            if (enable[i]) {
                if ( (_imp->hovering == i) || (_imp->dragging == i) ) {
                    GL_GPU::Color3f(0.f * l, 1.f * l, 0.f * l);
                } else {
                    GL_GPU::Color3f( (float)color.r * l, (float)color.g * l, (float)color.b * l );
                }
                GL_GPU::Vertex2d(p[i].x, p[i].y);
            }
        }
        GL_GPU::End();

        for (int i = enableBegin; i < enableEnd; ++i) {
            if (enable[i]) {
                std::string text = useFrom ? _imp->from[i].lock()->getName().c_str() :  _imp->to[i].lock()->getName().c_str() ;
                getLastCallingViewport()->renderText(p[i].x, p[i].y, text, color.r * l , color.g * l, color.b * l, 1);
            }
        }
    }

    //glPopAttrib();

} // drawOverlay

bool
CornerPinOverlayInteract::onOverlayPenDown(TimeValue time,
                                           const RenderScale & /*renderScale*/,
                                           ViewIdx /*view*/,
                                           const QPointF & /*viewportPos*/,
                                           const QPointF & pos,
                                           double /*pressure*/,
                                           TimeValue /*timestamp*/,
                                           PenType /*pen*/)
{
    // do not show interact if knob is secret or not enabled
    // see https://github.com/MrKepzie/Natron/issues/932
    KnobDoublePtr from1Knob = _imp->from[0].lock();

    if ( !from1Knob || !from1Knob->shouldDrawOverlayInteract() ) {
        return false;
    }

    RenderScale pscale;
    getPixelScale(pscale.x, pscale.y);

    OfxPointD to[4];
    OfxPointD from[4];
    bool enable[4];
    bool useFrom;

    if (_imp->dragging == -1) {
        for (int i = 0; i < 4; ++i) {
            _imp->getFrom(time, i, &from[i].x, &from[i].y);
            _imp->getTo(time, i, &to[i].x, &to[i].y);
            enable[i] = _imp->getEnabled(time, i);
        }
        useFrom = _imp->getUseFromPoints(time);
    } else {
        for (int i = 0; i < 4; ++i) {
            to[i] = _imp->toDrag[i];
            from[i] = _imp->fromDrag[i];
            enable[i] = _imp->enableDrag[i];
        }
        useFrom = _imp->useFromDrag;
    }

    if (!useFrom && !_imp->areToPointsAnimated()) {
        return false;
    }

    OfxPointD p[4];
    OfxPointD q[4];
    int enableBegin = 4;
    int enableEnd = 0;
    for (int i = 0; i < 4; ++i) {
        if (enable[i]) {
            if (useFrom) {
                p[i] = from[i];
                q[i] = to[i];
            } else {
                q[i] = from[i];
                p[i] = to[i];
            }
            if (i < enableBegin) {
                enableBegin = i;
            }
            if (i + 1 > enableEnd) {
                enableEnd = i + 1;
            }
        }
    }


    bool didSomething = false;

    for (int i = enableBegin; i < enableEnd; ++i) {
        if (enable[i]) {
            if ( CornerPinOverlayInteractPrivate::isNearby(pos, p[i].x, p[i].y, CornerPinOverlayInteractPrivate::pointTolerance(), pscale) ) {
                _imp->dragging = i;
                didSomething = true;
            }
            _imp->toDrag[i] = to[i];
            _imp->fromDrag[i] = from[i];
            _imp->enableDrag[i] = enable[i];
        }
    }
    _imp->interactiveDrag = _imp->getInteractive();
    _imp->useFromDrag = useFrom;
    _imp->lastPenPos = pos;
    
    
    return didSomething;

} // onOverlayPenDown


bool
CornerPinOverlayInteract::onOverlayPenMotion(TimeValue time,
                                             const RenderScale & /*renderScale*/,
                                             ViewIdx view,
                                             const QPointF & /*viewportPos*/,
                                             const QPointF & penPos,
                                             double /*pressure*/,
                                             TimeValue /*timestamp*/)
{
    // do not show interact if knob is secret or not enabled
    // see https://github.com/MrKepzie/Natron/issues/932
    KnobDoublePtr from1Knob = _imp->from[0].lock();

    if ( !from1Knob || !from1Knob->shouldDrawOverlayInteract() ) {
        return false;
    }

    RenderScale pscale;
    getPixelScale(pscale.x, pscale.y);

    OfxPointD to[4];
    OfxPointD from[4];
    bool enable[4];
    bool useFrom;

    if (_imp->dragging == -1) {
        for (int i = 0; i < 4; ++i) {
            _imp->getFrom(time, i, &from[i].x, &from[i].y);
            _imp->getTo(time, i, &to[i].x, &to[i].y);
            enable[i] = _imp->getEnabled(time, i);
        }
        useFrom = _imp->getUseFromPoints(time);
    } else {
        for (int i = 0; i < 4; ++i) {
            to[i] = _imp->toDrag[i];
            from[i] = _imp->fromDrag[i];
            enable[i] = _imp->enableDrag[i];
        }
        useFrom = _imp->useFromDrag;
    }

    if (!useFrom && !_imp->areToPointsAnimated()) {
        return false;
    }

    OfxPointD p[4];
    OfxPointD q[4];
    int enableBegin = 4;
    int enableEnd = 0;
    for (int i = 0; i < 4; ++i) {
        if (enable[i]) {
            if (useFrom) {
                p[i] = from[i];
                q[i] = to[i];
            } else {
                q[i] = from[i];
                p[i] = to[i];
            }
            if (i < enableBegin) {
                enableBegin = i;
            }
            if (i + 1 > enableEnd) {
                enableEnd = i + 1;
            }
        }
    }
    bool didSomething = false;
    bool valuesChanged = false;
    OfxPointD delta;
    delta.x = penPos.x() - _imp->lastPenPos.x();
    delta.y = penPos.y() - _imp->lastPenPos.y();

    _imp->hovering = -1;

    for (int i = enableBegin; i < enableEnd; ++i) {
        if (enable[i]) {
            if (_imp->dragging == i) {
                if (useFrom) {
                    from[i].x += delta.x;
                    from[i].y += delta.y;
                    _imp->fromDrag[i] = from[i];
                } else {
                    to[i].x += delta.x;
                    to[i].y += delta.y;
                    _imp->toDrag[i] = to[i];
                }
                valuesChanged = true;
            } else if ( CornerPinOverlayInteractPrivate::isNearby(penPos, p[i].x, p[i].y, CornerPinOverlayInteractPrivate::pointTolerance(), pscale) ) {
                _imp->hovering = i;
                didSomething = true;
            }
        }
    }

    if ( (_imp->dragging != -1) && _imp->interactiveDrag && valuesChanged ) {
        // no need to redraw overlay since it is slave to the paramaters

        if (_imp->useFromDrag) {
            KnobDoublePtr knob = _imp->from[_imp->dragging].lock();
            assert(knob);
            std::vector<double> val(2);
            val[0] = from[_imp->dragging].x;
            val[1] = from[_imp->dragging].y;
            knob->setValueAcrossDimensions(val, DimIdx(0), view, eValueChangedReasonUserEdited);
        } else {
            KnobDoublePtr knob = _imp->to[_imp->dragging].lock();
            assert(knob);
            std::vector<double> val(2);
            val[0] = to[_imp->dragging].x;
            val[1] = to[_imp->dragging].y;

            if (_imp->toPointsAutoKeyingEnabled) {
                knob->setValueAtTimeAcrossDimensions(time, val, DimIdx(0), view, eValueChangedReasonUserEdited);

                // Also set a keyframe on other points
                for (int i = 0; i < 4; ++i) {
                    if (i == _imp->dragging) {
                        continue;
                    }
                    std::vector<double> values(2);
                    KnobDoublePtr toPoint = _imp->to[i].lock();
                    values[0] = toPoint->getValueAtTime(time, DimIdx(0));
                    values[1] = toPoint->getValueAtTime(time, DimIdx(1));
                    toPoint->setValueAtTimeAcrossDimensions(time, values, DimIdx(0), view, eValueChangedReasonUserEdited);
                }
            } else {
                knob->setValueAcrossDimensions(val, DimIdx(0), view, eValueChangedReasonUserEdited);
            }
        }
    }

    _imp->lastPenPos = penPos;
    
    return didSomething || valuesChanged;
} // onOverlayPenMotion

bool
CornerPinOverlayInteract::onOverlayPenUp(TimeValue time,
                                         const RenderScale & /*renderScale*/,
                                         ViewIdx view,
                                         const QPointF & /*viewportPos*/,
                                         const QPointF & /*pos*/,
                                         double /*pressure*/,
                                         TimeValue /*timestamp*/)
{
    // do not show interact if knob is secret or not enabled
    // see https://github.com/MrKepzie/Natron/issues/932
    KnobDoublePtr from1Knob = _imp->from[0].lock();

    if ( !from1Knob || !from1Knob->shouldDrawOverlayInteract() ) {
        return false;
    }

    bool didSomething = _imp->dragging != -1;

    if ( !_imp->interactiveDrag && (_imp->dragging != -1) ) {
        // no need to redraw overlay since it is slave to the paramaters
        if (_imp->useFromDrag) {
            KnobDoublePtr knob = _imp->from[_imp->dragging].lock();
            assert(knob);
            std::vector<double> val(2);
            val[0] = _imp->fromDrag[_imp->dragging].x;
            val[1] = _imp->fromDrag[_imp->dragging].y;
            knob->setValueAcrossDimensions(val, DimIdx(0), view, eValueChangedReasonUserEdited);
        } else {
            KnobDoublePtr knob = _imp->to[_imp->dragging].lock();
            assert(knob);
            std::vector<double> val(2);
            val[0] = _imp->toDrag[_imp->dragging].x;
            val[1] = _imp->toDrag[_imp->dragging].y;

            if (_imp->toPointsAutoKeyingEnabled) {
                knob->setValueAtTimeAcrossDimensions(time, val, DimIdx(0), view, eValueChangedReasonUserEdited);

                // Also set a keyframe on other points
                for (int i = 0; i < 4; ++i) {
                    if (i == _imp->dragging) {
                        continue;
                    }
                    std::vector<double> values(2);
                    KnobDoublePtr toPoint = _imp->to[i].lock();
                    values[0] = toPoint->getValueAtTime(time, DimIdx(0));
                    values[1] = toPoint->getValueAtTime(time, DimIdx(1));
                    toPoint->setValueAtTimeAcrossDimensions(time, values, DimIdx(0), view, eValueChangedReasonUserEdited);
                }
            } else {
                knob->setValueAcrossDimensions(val, DimIdx(0), view, eValueChangedReasonUserEdited);
            }
        }
    }
    _imp->dragging = -1;

    return didSomething;
} // onOverlayPenUp

bool
CornerPinOverlayInteract::onOverlayFocusLost(TimeValue /*time*/,
                                             const RenderScale & /*renderScale*/,
                                             ViewIdx /*view*/)
{
    _imp->dragging = -1;
    _imp->hovering = -1;
    _imp->interactiveDrag = false;

    return false;
} // onOverlayFocusLost



NATRON_NAMESPACE_EXIT
