/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://natrongithub.github.io/>,
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

#include "CurveGui.h"

#include <cmath>
#include <algorithm> // min, max
#include <stdexcept>

#include <QtCore/QThread>
#include <QtCore/QObject>
#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>

#include "Engine/Bezier.h"
#include "Engine/Knob.h"
#include "Engine/KnobTypes.h"
#include "Engine/RotoContext.h" // Bezier
#include "Engine/ViewIdx.h"

#include "Gui/CurveWidget.h"
#include "Gui/CurveWidgetPrivate.h"
#include "Gui/KnobGui.h"

NATRON_NAMESPACE_ENTER

CurveGui::CurveGui(CurveWidget *curveWidget,
                   CurvePtr curve,
                   const QString & name,
                   const QColor & color,
                   int thickness)
    : _internalCurve(curve)
    , _curveWidget(curveWidget)
    , _name(name)
    , _color(color)
    , _thickness(thickness)
    , _visible(false)
    , _selected(false)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
}

CurveGui::~CurveGui()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
}

void
CurveGui::nextPointForSegment(const double x, // < in curve coordinates
                              const KeyFrameSet & keys,
                              const bool isPeriodic,
                              const double parametricXMin,
                              const double parametricXMax,
                              KeyFrameSet::const_iterator* lastUpperIt,
                              double* x2WidgetCoords,
                              KeyFrame* x1Key,
                              bool* isx1Key)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    assert( !keys.empty() );

    *isx1Key = false;

    // If non periodic and out of curve range, draw straight lines from widget border to
    // the keyframe on the side
    if (!isPeriodic && x < keys.begin()->getTime()) {
        *x2WidgetCoords = _curveWidget->toWidgetCoordinates(keys.begin()->getTime(), 0).x();
        *x1Key = *keys.begin();
        *isx1Key = true;
        return;
    } else if (!isPeriodic && x >= keys.rbegin()->getTime()) {
        *x2WidgetCoords = _curveWidget->width() - 1;
        return;
    }



    // We're between 2 keyframes or the curve is periodic, get the upper and lower keyframes widget coordinates

    // Points to the first keyframe with a greater time (in widget coords) than x1
    KeyFrameSet::const_iterator upperIt = keys.end();;

    // If periodic, bring back x in the period range (in widget coordinates)
    double xClamped = x;
    double period = parametricXMax - parametricXMin;

    {
        //KeyFrameSet::const_iterator start = keys.begin();
        const double xMin = parametricXMin;// + start->getTime();
        const double xMax = parametricXMax;// + start->getTime();
        if ((x < xMin || x > xMax) && isPeriodic) {
            xClamped = std::fmod(x - xMin, period) + parametricXMin;
            if (xClamped < xMin) {
                xClamped += period;
            }
            assert(xClamped >= xMin && xClamped <= xMax);
        }
    }
    {

        KeyFrameSet::const_iterator itKeys = keys.begin();

        if ( *lastUpperIt != keys.end() ) {
            // If we already have called this function before, start from the previously
            // computed iterator to avoid n square complexity
            itKeys = *lastUpperIt;
        } else {
            // Otherwise start from the begining
            itKeys = keys.begin();
        }

        for (; itKeys != keys.end(); ++itKeys) {
            if (itKeys->getTime() > xClamped) {
                upperIt = itKeys;
                *lastUpperIt = upperIt;
                break;
            }
        }
    }

    double tprev, vprev, vprevDerivRight, tnext, vnext, vnextDerivLeft;
    if (upperIt == keys.end()) {
        // We are in a periodic curve: we are in-between the last keyframe and the parametric xMax
        // If the curve is non periodic, it should have been handled in the 2 cases above: we only draw a straightline
        // from the widget border to the first/last keyframe
        assert(isPeriodic);
        KeyFrameSet::const_iterator start = keys.begin();
        KeyFrameSet::const_reverse_iterator last = keys.rbegin();
        tprev = last->getTime();
        vprev = last->getValue();
        vprevDerivRight = last->getRightDerivative();
        tnext = std::fmod(last->getTime() - start->getTime(), period) + tprev;
        //xClamped += period;
        vnext = start->getValue();
        vnextDerivLeft = start->getLeftDerivative();

    } else if (upperIt == keys.begin()) {
        // We are in a periodic curve: we are in-between the parametric xMin and the first keyframe
        // If the curve is non periodic, it should have been handled in the 2 cases above: we only draw a straightline
        // from the widget border to the first/last keyframe
        assert(isPeriodic);
        KeyFrameSet::const_reverse_iterator last = keys.rbegin();
        tprev = last->getTime();
        //xClamped -= period;
        vprev = last->getValue();
        vprevDerivRight = last->getRightDerivative();
        tnext = std::fmod(last->getTime() - upperIt->getTime(), period) + tprev;
        vnext = upperIt->getValue();
        vnextDerivLeft = upperIt->getLeftDerivative();

    } else {
        // in-between 2 keyframes
        KeyFrameSet::const_iterator prev = upperIt;
        --prev;
        tprev = prev->getTime();
        vprev = prev->getValue();
        vprevDerivRight = prev->getRightDerivative();
        tnext = upperIt->getTime();
        vnext = upperIt->getValue();
        vnextDerivLeft = upperIt->getLeftDerivative();
    }
    double normalizeTimeRange = tnext - tprev;
    if (normalizeTimeRange == 0) {
        // Only 1 keyframe, draw a horizontal line
        *x2WidgetCoords = _curveWidget->width() - 1;
        return;
    }
    assert(normalizeTimeRange > 0.);

    double t = ( xClamped - tprev ) / normalizeTimeRange;
    double P3 = vnext;
    double P0 = vprev;
    // Hermite coefficients P0' and P3' are for t normalized in [0,1]
    double P3pl = vnextDerivLeft / normalizeTimeRange; // normalize for t \in [0,1]
    double P0pr = vprevDerivRight / normalizeTimeRange; // normalize for t \in [0,1]
    double secondDer = 6. * (1. - t) * (P3 - P3pl / 3. - P0 - 2. * P0pr / 3.) +
    6. * t * (P0 - P3 + 2 * P3pl / 3. + P0pr / 3. );

    double secondDerWidgetCoord = _curveWidget->toWidgetCoordinates(0, secondDer).y();
    double normalizedSecondDerWidgetCoord = std::abs(secondDerWidgetCoord / normalizeTimeRange);

    // compute delta_x so that the y difference between the derivative and the curve is at most
    // 1 pixel (use the second order Taylor expansion of the function)
    double delta_x = std::max(2. / std::max(std::sqrt(normalizedSecondDerWidgetCoord), 0.1), 1.);

    // The x widget coordinate of the next keyframe
    double tNextWidgetCoords = _curveWidget->toWidgetCoordinates(tnext, 0).x();

    // The widget coordinate of the x passed in parameter but clamped to the curve period
    double xClampedWidgetCoords = _curveWidget->toWidgetCoordinates(xClamped, 0).x();

    // The real x passed in parameter in widget coordinates
    double xWidgetCoords = _curveWidget->toWidgetCoordinates(x, 0).x();

    double x2ClampedWidgetCoords = xClampedWidgetCoords + delta_x;

    double deltaXtoNext = (tNextWidgetCoords - xClampedWidgetCoords);
    // If nearby next key, clamp to it
    if (x2ClampedWidgetCoords > tNextWidgetCoords && deltaXtoNext > 1e-6) {
        // x2 is the position of the next keyframe with the period removed
        *x2WidgetCoords = xWidgetCoords + deltaXtoNext;
        x1Key->setValue(vnext);
        x1Key->setTime(x + (tnext - xClamped));
        *isx1Key = true;
    } else {
        // just add the delta to the x widget coord
        *x2WidgetCoords = xWidgetCoords + delta_x;
    }

} // nextPointForSegment

Curve::YRange
CurveGui::getCurveYRange() const
{
    return getInternalCurve()->getCurveYRange();
}

CurvePtr
CurveGui::getInternalCurve() const
{
    return _internalCurve;
}

static void
drawLineStrip(const std::vector<float>& vertices,
              const QPointF& btmLeft,
              const QPointF& topRight)
{
    glBegin(GL_LINE_STRIP);

    bool prevVisible = true;
    bool prevTooAbove = false;
    bool prevTooBelow = false;
    for (int i = 0; i < (int)vertices.size(); i += 2) {
        const bool isAbove = i > 0 && vertices[i + 1] > topRight.y();
        const bool isBelow = i > 0 && vertices[i + 1] < btmLeft.y();
        const bool vertexVisible = vertices[i] >= btmLeft.x() && vertices[i] <= topRight.x() && !isAbove && !isBelow;
        const bool previousWasVisible = prevVisible;
        const bool previousWasTooAbove = prevTooAbove;
        const bool previousWasTooBelow = prevTooBelow;

        prevVisible = vertexVisible;
        prevTooBelow = isBelow;
        prevTooAbove = isAbove;

        if (!previousWasVisible && !vertexVisible) {
            continue;
        }
        if ( !previousWasVisible && (i >= 2) ) {
            //At least draw the previous point otherwise this will draw a line between the last previous point and this point
            //Draw them 10000 units further so that we're sure we don't see half of a pixel of a line remaining
            if (previousWasTooAbove) {
                glVertex2f(vertices[i - 2], vertices[i - 1] + 100000);
            } else if (previousWasTooBelow) {
                glVertex2f(vertices[i - 2], vertices[i - 1] - 100000);
            }
        }
        glVertex2f(vertices[i], vertices[i + 1]);
    }

    glEnd();
}

void
CurveGui::drawCurve(int curveIndex,
                    int curvesCount)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    if (!_visible) {
        return;
    }

    assert( QGLContext::currentContext() == _curveWidget->context() );

    std::vector<float> vertices, exprVertices;
    double x1 = 0;
    double x2;
    const double widgetWidth = _curveWidget->width();
    KeyFrameSet keyframes;
    BezierCPCurveGui* isBezier = dynamic_cast<BezierCPCurveGui*>(this);
    KnobCurveGui* isKnobCurve = dynamic_cast<KnobCurveGui*>(this);
    bool hasDrawnExpr = false;
    if (isKnobCurve) {
        std::string expr;
        KnobIPtr knob = isKnobCurve->getInternalKnob();
        assert(knob);
        expr = knob->getExpression( isKnobCurve->getDimension() );
        if ( !expr.empty() ) {
            //we have no choice but to evaluate the expression at each time
            for (int i = x1; i < widgetWidth; ++i) {
                double x = _curveWidget->toZoomCoordinates(i, 0).x();;
                double y = knob->getValueAtWithExpression( x, ViewIdx(0), isKnobCurve->getDimension() );
                exprVertices.push_back(x);
                exprVertices.push_back(y);
            }
            hasDrawnExpr = true;
        }
    }
    bool isPeriodic = false;
    std::pair<double,double> parametricRange = std::make_pair(-std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity());
    if (isBezier) {
        std::set<double> keys;
        isBezier->getBezier()->getKeyframeTimes(&keys);
        int i = 0;
        for (std::set<double>::iterator it = keys.begin(); it != keys.end(); ++it, ++i) {
            keyframes.insert( KeyFrame(*it, i) );
        }
    } else {
        keyframes = getInternalCurve()->getKeyFrames_mt_safe();
        isPeriodic = getInternalCurve()->isCurvePeriodic();
        parametricRange = getInternalCurve()->getXRange();
    }
    if ( !keyframes.empty() ) {
        try {
            bool isX1AKey = false;
            KeyFrame x1Key;
            KeyFrameSet::const_iterator lastUpperIt = keyframes.end();

            while ( x1 < (widgetWidth - 1) ) {
                double x, y;
                if (!isX1AKey) {
                    x = _curveWidget->toZoomCoordinates(x1, 0).x();
                    y = evaluate(false, x);
                } else {
                    x = x1Key.getTime();
                    y = x1Key.getValue();
                }

                vertices.push_back( (float)x );
                vertices.push_back( (float)y );
                nextPointForSegment(x, keyframes, isPeriodic, parametricRange.first, parametricRange.second,  &lastUpperIt, &x2, &x1Key, &isX1AKey);
                x1 = x2;
            }
            //also add the last point
            {
                double x = _curveWidget->toZoomCoordinates(x1, 0).x();
                double y = evaluate(false, x);
                vertices.push_back( (float)x );
                vertices.push_back( (float)y );
            }
        } catch (...) {
        }
    }

    QPointF btmLeft = _curveWidget->toZoomCoordinates(0, _curveWidget->height() - 1);
    QPointF topRight = _curveWidget->toZoomCoordinates(_curveWidget->width() - 1, 0);
    const QColor & curveColor = _selected ?  _curveWidget->getSelectedCurveColor() : _color;

    {
        GLProtectAttrib a(GL_HINT_BIT | GL_ENABLE_BIT | GL_LINE_BIT | GL_COLOR_BUFFER_BIT | GL_POINT_BIT | GL_CURRENT_BIT);

        if (!isBezier && _selected) {
            ///Draw y min/max axis so the user understands why the curve is clamped
            Curve::YRange curveYRange = getCurveYRange();
            if (curveYRange.min != INT_MIN &&
                curveYRange.min != -std::numeric_limits<double>::infinity() &&
                curveYRange.max != INT_MAX &&
                curveYRange.max != std::numeric_limits<double>::infinity() ) {
                QColor minMaxColor;
                minMaxColor.setRgbF(0.398979, 0.398979, 0.398979);
                glColor4d(minMaxColor.redF(), minMaxColor.greenF(), minMaxColor.blueF(), 1.);
                glBegin(GL_LINES);
                glVertex2d(btmLeft.x(), curveYRange.min);
                glVertex2d(topRight.x(), curveYRange.min);
                glVertex2d(btmLeft.x(), curveYRange.max);
                glVertex2d(topRight.x(), curveYRange.max);
                glEnd();
                glColor4d(1., 1., 1., 1.);

                double xText = _curveWidget->toZoomCoordinates(10, 0).x();

                _curveWidget->renderText( xText, curveYRange.min, tr("min"), minMaxColor, _curveWidget->font() );
                _curveWidget->renderText( xText, curveYRange.max, tr("max"), minMaxColor, _curveWidget->font() );
            }
        }


        glColor4f( curveColor.redF(), curveColor.greenF(), curveColor.blueF(), curveColor.alphaF() );
        glPointSize(_thickness);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_LINE_SMOOTH);
        glHint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);
        glLineWidth(1.5);
        glCheckError();
        if (hasDrawnExpr) {
            drawLineStrip(exprVertices, btmLeft, topRight);
            glLineStipple(2, 0xAAAA);
            glEnable(GL_LINE_STIPPLE);
        }
        drawLineStrip(vertices, btmLeft, topRight);
        if (hasDrawnExpr) {
            glDisable(GL_LINE_STIPPLE);
        }

        glCheckErrorIgnoreOSXBug();

        //render the name of the curve
        glColor4f(1.f, 1.f, 1.f, 1.f);


        double interval = ( topRight.x() - btmLeft.x() ) / (double)curvesCount;
        double textX = _curveWidget->toZoomCoordinates(15, 0).x() + interval * (double)curveIndex;
        double textY;

        try {
            textY = evaluate(_internalCurve && !_internalCurve->isAnimated(), textX);
        } catch (...) {
            textY = evaluate(true, textX);
        }

        if ( ( textX >= btmLeft.x() ) && ( textX <= topRight.x() ) && ( textY >= btmLeft.y() ) && ( textY <= topRight.y() ) ) {
            _curveWidget->renderText( textX, textY, _name, _color, _curveWidget->getFont() );
        }
        glColor4f( curveColor.redF(), curveColor.greenF(), curveColor.blueF(), curveColor.alphaF() );

        //draw keyframes
        glPointSize(7.f);
        glEnable(GL_POINT_SMOOTH);

        const SelectedKeys & selectedKeyFrames = _curveWidget->getSelectedKeyFrames();
        SelectedKeys::const_iterator selectedKeyFramesIt;
        SelectedKeys::const_iterator foundCurveSelected = selectedKeyFrames.end();
        for (selectedKeyFramesIt = selectedKeyFrames.begin(); selectedKeyFramesIt != selectedKeyFrames.end(); ++selectedKeyFramesIt) {
            if (selectedKeyFramesIt->first.get() == this) {
                foundCurveSelected = selectedKeyFramesIt;
                break;
            }
        }

        //bool isCurveSelected = foundCurveSelected != selectedKeyFrames.end();

        for (KeyFrameSet::const_iterator k = keyframes.begin(); k != keyframes.end(); ++k) {
            const KeyFrame & key = (*k);

            if ( ( key.getTime() < btmLeft.x() ) || ( key.getTime() > topRight.x() ) || ( key.getValue() < btmLeft.y() ) || ( key.getValue() > topRight.y() ) ) {
                continue;
            }

            glColor4f( _color.redF(), _color.greenF(), _color.blueF(), _color.alphaF() );

            //if the key is selected change its color to white
            KeyPtr isSelected;
            if ( foundCurveSelected != selectedKeyFrames.end() ) {
                for (std::list<KeyPtr>::const_iterator it2 = foundCurveSelected->second.begin();
                     it2 != foundCurveSelected->second.end(); ++it2) {
                    if ( ( (*it2)->key.getTime() == key.getTime() ) && ( (*it2)->curve.get() == this ) ) {
                        isSelected = *it2;
                        glColor4f(1.f, 1.f, 1.f, 1.f);
                        break;
                    }
                }
            }

            double x = key.getTime();
            double y = key.getValue();
            glBegin(GL_POINTS);
            glVertex2f(x, y);
            glEnd();
            glCheckErrorIgnoreOSXBug();

            if ( !isBezier && isSelected && (key.getInterpolation() != eKeyframeTypeConstant) ) {
                QFontMetrics m( _curveWidget->getFont() );


                //draw the derivatives lines
                if ( (key.getInterpolation() != eKeyframeTypeFree) && (key.getInterpolation() != eKeyframeTypeBroken) ) {
                    glLineStipple(2, 0xAAAA);
                    glEnable(GL_LINE_STIPPLE);
                }
                glBegin(GL_LINES);
                glColor4f(1., 0.35, 0.35, 1.);
                glVertex2f( isSelected->leftTan.first, isSelected->leftTan.second );
                glVertex2f(x, y);
                glVertex2f(x, y);
                glVertex2f( isSelected->rightTan.first, isSelected->rightTan.second );
                glEnd();
                if ( (key.getInterpolation() != eKeyframeTypeFree) && (key.getInterpolation() != eKeyframeTypeBroken) ) {
                    glDisable(GL_LINE_STIPPLE);
                }

                bool singleKey = selectedKeyFrames.size() == 1 && selectedKeyFrames.begin()->second.size() == 1;
                if (singleKey) { //if one keyframe, also draw the coordinates
                    double rounding = std::pow(10., CURVEWIDGET_DERIVATIVE_ROUND_PRECISION);
                    QString leftDerivStr = QString::fromUtf8("l: %1").arg(std::floor( (key.getLeftDerivative() * rounding) + 0.5 ) / rounding);
                    QString rightDerivStr = QString::fromUtf8("r: %1").arg(std::floor( (key.getRightDerivative() * rounding) + 0.5 ) / rounding);
                    double yLeftWidgetCoord = _curveWidget->toWidgetCoordinates(0, isSelected->leftTan.second).y();
                    yLeftWidgetCoord += (m.height() + 4);

                    double yRightWidgetCoord = _curveWidget->toWidgetCoordinates(0, isSelected->rightTan.second).y();
                    yRightWidgetCoord += (m.height() + 4);

                    glColor4f(1., 1., 1., 1.);
                    glCheckFramebufferError();
                    _curveWidget->renderText( isSelected->leftTan.first, _curveWidget->toZoomCoordinates(0, yLeftWidgetCoord).y(),
                                              leftDerivStr, QColor(240, 240, 240), _curveWidget->getFont() );
                    _curveWidget->renderText( isSelected->rightTan.first, _curveWidget->toZoomCoordinates(0, yRightWidgetCoord).y(),
                                              rightDerivStr, QColor(240, 240, 240), _curveWidget->getFont() );
                }


                if (singleKey) { //if one keyframe, also draw the coordinates
                    QString coordStr = QString::fromUtf8("x: %1, y: %2");
                    coordStr = coordStr.arg(x).arg(y);
                    double yWidgetCoord = _curveWidget->toWidgetCoordinates( 0, key.getValue() ).y();
                    yWidgetCoord += (m.height() + 4);
                    glColor4f(1., 1., 1., 1.);
                    glCheckFramebufferError();
                    _curveWidget->renderText( x, _curveWidget->toZoomCoordinates(0, yWidgetCoord).y(),
                                              coordStr, QColor(240, 240, 240), _curveWidget->getFont() );
                }
                glBegin(GL_POINTS);
                glVertex2f( isSelected->leftTan.first, isSelected->leftTan.second );
                glVertex2f( isSelected->rightTan.first, isSelected->rightTan.second );
                glEnd();
            } // if ( !isBezier && ( isSelected != selectedKeyFrames.end() ) && (key.getInterpolation() != eKeyframeTypeConstant) ) {
        } // for (KeyFrameSet::const_iterator k = keyframes.begin(); k != keyframes.end(); ++k) {
    } // GLProtectAttrib(GL_HINT_BIT | GL_ENABLE_BIT | GL_LINE_BIT | GL_COLOR_BUFFER_BIT | GL_POINT_BIT | GL_CURRENT_BIT);

    glCheckError();
} // drawCurve

void
CurveGui::setVisible(bool visible)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    _visible = visible;
}

void
CurveGui::setVisibleAndRefresh(bool visible)
{
    setVisible(visible);
    _curveWidget->update();
}

bool
CurveGui::areKeyFramesTimeClampedToIntegers() const
{
    return getInternalCurve()->areKeyFramesTimeClampedToIntegers();
}

bool
CurveGui::areKeyFramesValuesClampedToBooleans() const
{
    return getInternalCurve()->areKeyFramesValuesClampedToBooleans();
}

bool
CurveGui::areKeyFramesValuesClampedToIntegers() const
{
    return getInternalCurve()->areKeyFramesValuesClampedToIntegers();
}

bool
CurveGui::isYComponentMovable() const
{
    return getInternalCurve()->isYComponentMovable();
}

KeyFrameSet
CurveGui::getKeyFrames() const
{
    return getInternalCurve()->getKeyFrames_mt_safe();
}

KnobCurveGui::KnobCurveGui(CurveWidget *curveWidget,
                           CurvePtr  curve,
                           const KnobGuiPtr& knob,
                           int dimension,
                           const QString & name,
                           const QColor & color,
                           int thickness)
    : CurveGui(curveWidget, curve, name, color, thickness)
    , _internalKnob()
    , _knob(knob)
    , _dimension(dimension)
{
    // even when there is only one keyframe, there may be tangents!
    if (curve->getKeyFramesCount() > 0) {
        setVisible(true);
    }

    if (knob) {
        QObject::connect( knob.get(), SIGNAL(keyFrameSet()), this, SLOT(onKnobInternalCurveChanged()) );
        QObject::connect( knob.get(), SIGNAL(keyFrameRemoved()), this, SLOT(onKnobInternalCurveChanged()) );
        QObject::connect( knob.get(), SIGNAL(keyInterpolationChanged()), this, SLOT(onKnobInterpolationChanged()) );
        QObject::connect( knob.get(), SIGNAL(keyInterpolationChanged()), this, SLOT(onKnobInterpolationChanged()) );
        QObject::connect( knob.get(), SIGNAL(refreshCurveEditor()), this, SLOT(onKnobInternalCurveChanged()) );
    }
}

KnobCurveGui::KnobCurveGui(CurveWidget *curveWidget,
                           CurvePtr  curve,
                           const KnobIPtr& knob,
                           const RotoContextPtr& roto,
                           int dimension,
                           const QString & name,
                           const QColor & color,
                           int thickness)
    : CurveGui(curveWidget, curve, name, color, thickness)
    , _roto(roto)
    , _internalKnob(knob)
    , _knob()
    , _dimension(dimension)
{
    // even when there is only one keyframe, there may be tangents!
    if (curve->getKeyFramesCount() > 0) {
        setVisible(true);
    }
}

KnobCurveGui::~KnobCurveGui()
{
}

void
KnobCurveGui::onKnobInternalCurveChanged()
{
    _curveWidget->updateSelectionAfterCurveChange(this);
    _curveWidget->update();
}

void
KnobCurveGui::onKnobInterpolationChanged()
{
    _curveWidget->updateSelectionAfterCurveChange(this);
    _curveWidget->update();
}

double
KnobCurveGui::evaluate(bool useExpr,
                       double x) const
{
    KnobIPtr knob = getInternalKnob();

    KnobParametric* isParametric = dynamic_cast<KnobParametric*>( knob.get() );
    if (isParametric) {
        return isParametric->getParametricCurve(_dimension)->getValueAt(x, false);
    } else if (useExpr) {
        return knob->getValueAtWithExpression(x, ViewIdx(0), _dimension);
    } else {
        assert(_internalCurve);

        return _internalCurve->getValueAt(x, false);
    }
}

CurvePtr
KnobCurveGui::getInternalCurve() const
{
    KnobIPtr knob = getInternalKnob();
    KnobParametric* isParametric = dynamic_cast<KnobParametric*>( knob.get() );

    if (!knob || !isParametric) {
        return CurveGui::getInternalCurve();
    }

    return isParametric->getParametricCurve(_dimension);
}

KnobIPtr
KnobCurveGui::getInternalKnob() const
{
    KnobGuiPtr knob = getKnobGui();

    return knob ? knob->getKnob() : _internalKnob;
}

int
KnobCurveGui::getKeyFrameIndex(double time) const
{
    return getInternalCurve()->keyFrameIndex(time);
}

KeyFrame
KnobCurveGui::setKeyFrameInterpolation(KeyframeTypeEnum interp,
                                       int index)
{
    return getInternalCurve()->setKeyFrameInterpolation(interp, index);
}

BezierCPCurveGui::BezierCPCurveGui(CurveWidget *curveWidget,
                                   const BezierPtr& bezier,
                                   const RotoContextPtr& roto,
                                   const QString & name,
                                   const QColor & color,
                                   int thickness)
    : CurveGui(curveWidget, CurvePtr(), name, color, thickness)
    , _bezier(bezier)
    , _rotoContext(roto)
{
    // even when there is only one keyframe, there may be tangents!
    if (bezier->getKeyframesCount() > 0) {
        setVisible(true);
    }
}

BezierPtr
BezierCPCurveGui::getBezier() const
{
    return _bezier;
}

BezierCPCurveGui::~BezierCPCurveGui()
{
}

double
BezierCPCurveGui::evaluate(bool /*useExpr*/,
                           double x) const
{
    std::list<std::pair<double, KeyframeTypeEnum> > keys;

    _bezier->getKeyframeTimesAndInterpolation(&keys);

    std::list<std::pair<double, KeyframeTypeEnum> >::iterator upb = keys.end();
    int dist = 0;
    for (std::list<std::pair<double, KeyframeTypeEnum> >::iterator it = keys.begin(); it != keys.end(); ++it, ++dist) {
        if (it->first > x) {
            upb = it;
            break;
        }
    }

    if ( upb == keys.end() ) {
        return keys.size() - 1;
    } else if ( upb == keys.begin() ) {
        return 0;
    } else {
        std::list<std::pair<double, KeyframeTypeEnum> >::iterator prev = upb;
        if ( prev != keys.begin() ) {
            --prev;
        }
        if (prev->second == eKeyframeTypeConstant) {
            return dist - 1;
        } else {
            ///Always linear for bezier interpolation
            assert( (upb->first - prev->first) != 0 );

            return ( (double)(x - prev->first) / (upb->first - prev->first) ) + dist - 1;
        }
    }
}

Curve::YRange
BezierCPCurveGui::getCurveYRange() const
{
    int keys = _bezier->getKeyframesCount();

    return Curve::YRange(0, keys - 1);
}

KeyFrameSet
BezierCPCurveGui::getKeyFrames() const
{
    KeyFrameSet ret;
    std::set<double> keys;

    _bezier->getKeyframeTimes(&keys);
    int i = 0;
    for (std::set<double>::iterator it = keys.begin(); it != keys.end(); ++it, ++i) {
        ret.insert( KeyFrame(*it, i) );
    }

    return ret;
}

int
BezierCPCurveGui::getKeyFrameIndex(double time) const
{
    return _bezier->getKeyFrameIndex(time);
}

KeyFrame
BezierCPCurveGui::setKeyFrameInterpolation(KeyframeTypeEnum interp,
                                           int index)
{
    _bezier->setKeyFrameInterpolation(interp, index);

    return KeyFrame();
}

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_CurveGui.cpp"
