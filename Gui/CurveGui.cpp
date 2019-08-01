/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
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
#include <cstdlib>

#include <QtCore/QThread>
#include <QtCore/QObject>
#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>

#include <QTreeWidgetItem>

#include "Engine/Curve.h"
#include "Engine/KnobTypes.h"
#include "Engine/Image.h"
#include "Engine/OSGLFunctions.h"
#include "Engine/Utils.h"
#include "Engine/ViewIdx.h"

#include "Gui/AnimItemBase.h"
#include "Gui/AnimationModuleBase.h"
#include "Gui/AnimationModuleViewPrivate.h"
#include "Gui/AnimationModuleSelectionModel.h"
#include "Gui/AnimationModuleView.h"
#include "Gui/CurveWidgetPrivate.h"
#include "Gui/KnobGui.h"

NATRON_NAMESPACE_ENTER


#define NATRON_NO_PREDEFINED_CURVE_COLORS 6
#define NATRON_NO_PREDEFINED_CURVE_SATURATION 220
#define NATRON_NO_PREDEFINED_CURVE_VALUE 150
static const OfxRGBColourD curveColors[NATRON_NO_PREDEFINED_CURVE_COLORS] =
{
    // h s v
    {0, NATRON_NO_PREDEFINED_CURVE_SATURATION, NATRON_NO_PREDEFINED_CURVE_VALUE},
    {60, NATRON_NO_PREDEFINED_CURVE_SATURATION, NATRON_NO_PREDEFINED_CURVE_VALUE},
    {120, NATRON_NO_PREDEFINED_CURVE_SATURATION, NATRON_NO_PREDEFINED_CURVE_VALUE},
    {180, NATRON_NO_PREDEFINED_CURVE_SATURATION, NATRON_NO_PREDEFINED_CURVE_VALUE},
    {240, NATRON_NO_PREDEFINED_CURVE_SATURATION, NATRON_NO_PREDEFINED_CURVE_VALUE},
    {300, NATRON_NO_PREDEFINED_CURVE_SATURATION, NATRON_NO_PREDEFINED_CURVE_VALUE}
};

struct CurveGuiPrivate
{
    AnimationModuleView* curveWidget;
    AnimItemBaseWPtr item;
    DimIdx dimension;
    ViewIdx view;
    CurveWPtr internalCurve; // ptr to the internal curve

    double color[4]; // the color that must be used to draw the curve
    int lineWidth; // its thickness

    CurveGuiPrivate(AnimationModuleView *curveWidget,
                    const AnimItemBasePtr& item, DimIdx dimension, ViewIdx view)
    : curveWidget(curveWidget)
    , item(item)
    , dimension(dimension)
    , view(view)
    , internalCurve()
    , color()
    , lineWidth(1.)
    {

        QColor tmpColor;
        int colorIdx = rand() % NATRON_NO_PREDEFINED_CURVE_COLORS;
        assert(colorIdx >= 0 && colorIdx < NATRON_NO_PREDEFINED_CURVE_COLORS);
        tmpColor.setHsv(curveColors[colorIdx].r, curveColors[colorIdx].g, curveColors[colorIdx].b);
        color[0] = tmpColor.redF();
        color[1] = tmpColor.greenF();
        color[2] = tmpColor.blueF();
        color[3] = 1.;

        internalCurve = item->getCurve(dimension, view);
        assert(internalCurve.lock());

    }
};

CurveGui::CurveGui(AnimationModuleView *curveWidget,
                   const AnimItemBasePtr& item, DimIdx dimension, ViewIdx view)
: _imp(new CurveGuiPrivate(curveWidget, item, dimension, view))
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
}

CurveGui::~CurveGui()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
}

AnimItemBasePtr
CurveGui::getItem() const
{
    return _imp->item.lock();
}

ViewIdx
CurveGui::getView() const
{
    return _imp->view;
}

DimIdx
CurveGui::getDimension() const
{
    return _imp->dimension;
}

AnimItemDimViewIndexID
CurveGui::getCurveID() const
{
    return AnimItemDimViewIndexID(getItem(), getView(), getDimension());
}

QString
CurveGui::getName() const
{
    QString ret;
    AnimItemBasePtr item = _imp->item.lock();
    if (!item) {
        return ret;
    }
    ret = item->getViewDimensionLabel(_imp->dimension, _imp->view);
    return ret;
}


void
CurveGui::getColor(double* color) const
{
    memcpy(color, _imp->color, sizeof(double) * 4);
}

void
CurveGui::setColor(const double* color)
{
    memcpy(_imp->color, color, sizeof(double) * 4);
}

int
CurveGui::getLineWidth() const
{
    return _imp->lineWidth;
}

void
CurveGui::setLineWidth(int t)
{
    _imp->lineWidth = t;
}

Curve::YRange
CurveGui::getCurveYRange() const
{
    return getInternalCurve()->getCurveYRange();
}

CurvePtr
CurveGui::getInternalCurve() const
{
    return _imp->internalCurve.lock();
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

double
CurveGui::evaluate(bool useExpr, double x) const
{
    AnimItemBasePtr item = _imp->item.lock();
    if (!item) {
        return 0;
    }
    try {
        return item->evaluateCurve(useExpr, x, _imp->dimension, _imp->view);
    } catch (...) {
        return 0;
    }
}

int
CurveGui::getKeyFrameIndex(TimeValue time) const
{
    return getInternalCurve()->keyFrameIndex(time);
}

KeyFrame
CurveGui::setKeyFrameInterpolation(KeyframeTypeEnum interp,
                                       int index)
{
    return getInternalCurve()->setKeyFrameInterpolation(interp, index);
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
        *x2WidgetCoords = keys.begin()->getTime();
        double y;
        _imp->curveWidget->toWidgetCoordinates(x2WidgetCoords, &y);
        *x1Key = *keys.begin();
        *isx1Key = true;
        return;
    } else if (!isPeriodic && x >= keys.rbegin()->getTime()) {
        *x2WidgetCoords = _imp->curveWidget->width() - 1;
        return;
    }



    // We're between 2 keyframes or the curve is periodic, get the upper and lower keyframes widget coordinates

    // Points to the first keyframe with a greater time (in widget coords) than x1
    KeyFrameSet::const_iterator upperIt = keys.end();

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
            // Otherwise start from the beginning
            itKeys = keys.begin();
        }
        *lastUpperIt = keys.end();
        for (; itKeys != keys.end(); ++itKeys) {
            if (itKeys->getTime() > xClamped) {
                upperIt = itKeys;
                *lastUpperIt = upperIt;
                break;
            }
        }
    }

    double tprev, vprev, vprevDerivRight, tnext, vnext, vnextDerivLeft;
    if ( upperIt == keys.end() ) {
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

    } else if ( upperIt == keys.begin() ) {
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
        *x2WidgetCoords = _imp->curveWidget->width() - 1;
        return;
    }
    assert(normalizeTimeRange > 0.);

    double t = ( xClamped - tprev ) / normalizeTimeRange;
    double P3 = vnext;
    double P0 = vprev;
    // Hermite coefficients P0' and P3' are for t normalized in [0,1]
    double P3pl = vnextDerivLeft / normalizeTimeRange; // normalize for t \in [0,1]
    double P0pr = vprevDerivRight / normalizeTimeRange; // normalize for t \in [0,1]
    double secondDer = 6. * (1. - t) * (P3 - P3pl * (1. / 3) - P0 - P0pr * (2. / 3)) +
    6. * t * (P0 - P3 + P3pl * (2. / 3) + P0pr * (1. / 3) );

    double secondDerWidgetCoord = _imp->curveWidget->toWidgetCoordinates(0, secondDer).y();
    double normalizedSecondDerWidgetCoord = std::abs(secondDerWidgetCoord / normalizeTimeRange);

    // compute delta_x so that the y difference between the derivative and the curve is at most
    // 1 pixel (use the second order Taylor expansion of the function)
    double delta_x = std::max(2. / std::max(std::sqrt(normalizedSecondDerWidgetCoord), 0.1), 1.);

    // The x widget coordinate of the next keyframe
    double tNextWidgetCoords = _imp->curveWidget->toWidgetCoordinates(tnext, 0).x();

    // The widget coordinate of the x passed in parameter but clamped to the curve period
    double xClampedWidgetCoords = _imp->curveWidget->toWidgetCoordinates(xClamped, 0).x();

    // The real x passed in parameter in widget coordinates
    double xWidgetCoords = _imp->curveWidget->toWidgetCoordinates(x, 0).x();

    double x2ClampedWidgetCoords = xClampedWidgetCoords + delta_x;

    double deltaXtoNext = (tNextWidgetCoords - xClampedWidgetCoords);
    // If nearby next key, clamp to it
    if (x2ClampedWidgetCoords > tNextWidgetCoords && deltaXtoNext > 1e-6) {
        // x2 is the position of the next keyframe with the period removed
        *x2WidgetCoords = xWidgetCoords + deltaXtoNext;
        x1Key->setValue(vnext);
        x1Key->setTime(TimeValue(x + (tnext - xClamped)));
        *isx1Key = true;
    } else {
        // just add the delta to the x widget coord
        *x2WidgetCoords = xWidgetCoords + delta_x;
    }

} // nextPointForSegment



static void
drawLineStrip(const std::vector<float>& vertices,
              const QPointF& btmLeft,
              const QPointF& topRight)
{
    if (vertices.empty()) {
        return;
    }
    GL_GPU::Begin(GL_LINE_STRIP);

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
                GL_GPU::Vertex2f(vertices[i - 2], vertices[i - 1] + 100000);
            } else if (previousWasTooBelow) {
                GL_GPU::Vertex2f(vertices[i - 2], vertices[i - 1] - 100000);
            }
        }
        GL_GPU::Vertex2f(vertices[i], vertices[i + 1]);
    }

    GL_GPU::End();
}

class KeyFrameWithStringTimePredicate
{
public:
    KeyFrameWithStringTimePredicate(double t)
    : _t(t)
    {
    };

    bool operator()(const KeyFrame & f)
    {
        return f.getTime() == _t;
    }

private:
    double _t;
};


void
CurveGui::drawCurve(int curveIndex,
                    int curvesCount)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    AnimItemBasePtr item = _imp->item.lock();
    if (!item) {
        return;
    }

    std::vector<float> vertices, exprVertices;
    const double widgetWidth = _imp->curveWidget->width();
    KeyFrameSet keyframes;
    bool hasDrawnExpr = false;
    if (item->hasExpression(_imp->dimension, _imp->view)) {

        //we have no choice but to evaluate the expression at each time
        for (int i = 0; i < widgetWidth; ++i) {
            double x = _imp->curveWidget->toZoomCoordinates(i, 0).x();
            double y = evaluate(true /*useExpr*/, x);
            exprVertices.push_back(x);
            exprVertices.push_back(y);
        }
        hasDrawnExpr = true;

    }


    QPointF btmLeft = _imp->curveWidget->toZoomCoordinates(0, _imp->curveWidget->height() - 1);
    QPointF topRight = _imp->curveWidget->toZoomCoordinates(_imp->curveWidget->width() - 1, 0);




    bool isPeriodic = false;
    std::pair<double,double> parametricRange = std::make_pair(-std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity());

    keyframes = getInternalCurve()->getKeyFrames_mt_safe();
    isPeriodic = getInternalCurve()->isCurvePeriodic();
    parametricRange = getInternalCurve()->getXRange();

    if ( keyframes.empty() ) {
        // Add a horizontal line for constant knobs, except string knobs.
        KnobIPtr isKnob = boost::dynamic_pointer_cast<KnobI>(item->getInternalAnimItem());

        if (isKnob) {
            KnobStringBasePtr isString = boost::dynamic_pointer_cast<KnobStringBase>(isKnob);
            if (!isString) {
                double value = evaluate(false, 0);
                vertices.push_back(btmLeft.x() + 1);
                vertices.push_back(value);
                vertices.push_back(topRight.x() - 1);
                vertices.push_back(value);
            }
        }
    } else {
        try {
            double x1 = 0;
            double x2;

            bool isX1AKey = false;
            KeyFrame x1Key;
            KeyFrameSet::const_iterator lastUpperIt = keyframes.end();

            while ( x1 < (widgetWidth - 1) ) {
                double x, y;
                if (!isX1AKey) {
                    x = _imp->curveWidget->toZoomCoordinates(x1, 0).x();
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
                double x = _imp->curveWidget->toZoomCoordinates(x1, 0).x();
                double y = evaluate(false, x);
                vertices.push_back( (float)x );
                vertices.push_back( (float)y );
            }
        } catch (...) {
        }
    }

    // No Expr curve or no vertices for the curve, don't draw anything else
    if (exprVertices.empty() && vertices.empty()) {
        return;
    }

    AnimationModuleSelectionModelPtr selectionModel = item->getModel()->getSelectionModel();
    assert(selectionModel);
    const AnimItemDimViewKeyFramesMap& selectedKeys = selectionModel->getCurrentKeyFramesSelection();


    const KeyFrameSet* foundThisCurveSelectedKeys = 0;
    {
        AnimItemDimViewIndexID k;
        k.item = item;
        k.dim = _imp->dimension;
        k.view = _imp->view;
        AnimItemDimViewKeyFramesMap::const_iterator foundDimView = selectedKeys.find(k);

        if (foundDimView != selectedKeys.end()) {
            foundThisCurveSelectedKeys = &foundDimView->second;
        }
    }


    {
        GLProtectAttrib<GL_GPU> a(GL_HINT_BIT | GL_ENABLE_BIT | GL_LINE_BIT | GL_COLOR_BUFFER_BIT | GL_POINT_BIT | GL_CURRENT_BIT);

        // If this is the only curve selected, draw min/max
        if (foundThisCurveSelectedKeys && selectedKeys.size()) {

            // Draw y min/max axis so the user understands why the value is clamped
            Curve::YRange curveYRange = getCurveYRange();
            if (curveYRange.min != INT_MIN &&
                curveYRange.min != -std::numeric_limits<double>::infinity() &&
                curveYRange.max != INT_MAX &&
                curveYRange.max != std::numeric_limits<double>::infinity() ) {
                QColor minMaxColor;
                minMaxColor.setRgbF(0.398979, 0.398979, 0.398979);
                GL_GPU::Color4d(minMaxColor.redF(), minMaxColor.greenF(), minMaxColor.blueF(), 1.);
                GL_GPU::Begin(GL_LINES);
                GL_GPU::Vertex2d(btmLeft.x(), curveYRange.min);
                GL_GPU::Vertex2d(topRight.x(), curveYRange.min);
                GL_GPU::Vertex2d(btmLeft.x(), curveYRange.max);
                GL_GPU::Vertex2d(topRight.x(), curveYRange.max);
                GL_GPU::End();
                GL_GPU::Color4d(1., 1., 1., 1.);

                double xText = _imp->curveWidget->toZoomCoordinates(10, 0).x();

                _imp->curveWidget->renderText( xText, curveYRange.min, tr("min").toStdString(), minMaxColor.redF(), minMaxColor.greenF(), minMaxColor.blueF(), minMaxColor.alphaF());
                _imp->curveWidget->renderText( xText, curveYRange.max, tr("max").toStdString(), minMaxColor.redF(), minMaxColor.greenF(), minMaxColor.blueF(), minMaxColor.alphaF());
            }
        }


        GL_GPU::Color4f(_imp->color[0], _imp->color[1], _imp->color[2], _imp->color[3]);
        GL_GPU::PointSize(_imp->lineWidth);
        GL_GPU::Enable(GL_BLEND);
        GL_GPU::BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        GL_GPU::Enable(GL_LINE_SMOOTH);
        GL_GPU::Hint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);
        GL_GPU::LineWidth(1.5);
        glCheckError(GL_GPU);
        if (hasDrawnExpr) {
            drawLineStrip(exprVertices, btmLeft, topRight);
            GL_GPU::LineStipple(2, 0xAAAA);
            GL_GPU::Enable(GL_LINE_STIPPLE);
        }
        drawLineStrip(vertices, btmLeft, topRight);
        if (hasDrawnExpr) {
            GL_GPU::Disable(GL_LINE_STIPPLE);
        }

        glCheckErrorIgnoreOSXBug(GL_GPU);

        //render the name of the curve
        GL_GPU::Color4f(1.f, 1.f, 1.f, 1.f);


        double interval = ( topRight.x() - btmLeft.x() ) / (double)curvesCount;
        double textX = _imp->curveWidget->toZoomCoordinates(15, 0).x() + interval * (double)curveIndex;
        double textY;

        CurvePtr internalCurve = _imp->internalCurve.lock();
        QString curveName = getName();
        QColor thisColor;
        thisColor.setRgbF(Image::clamp(_imp->color[0], 0., 1.),
                          Image::clamp(_imp->color[1], 0., 1.),
                          Image::clamp(_imp->color[2], 0., 1.));

        try {
            // Use expression to place the text if the curve is not animated
            textY = evaluate(internalCurve && !internalCurve->isAnimated(), textX);
        } catch (...) {
            // if it fails attempt without expression, this will most likely return a constant value
            textY = evaluate(false /*useExpression*/, textX);
        }

        if ( ( textX >= btmLeft.x() ) && ( textX <= topRight.x() ) && ( textY >= btmLeft.y() ) && ( textY <= topRight.y() ) ) {
            _imp->curveWidget->renderText( textX, textY, curveName.toStdString(), thisColor.redF(), thisColor.greenF(), thisColor.blueF(), thisColor.alphaF());
        }
        GL_GPU::Color4f(_imp->color[0], _imp->color[1], _imp->color[2], _imp->color[3]);

        //draw keyframes
        GL_GPU::PointSize(7.f);
        GL_GPU::Enable(GL_POINT_SMOOTH);


        KeyFrameSet::const_iterator foundSelectedKey;
        if (foundThisCurveSelectedKeys) {
            foundSelectedKey = foundThisCurveSelectedKeys->end();
        }
        for (KeyFrameSet::const_iterator k = keyframes.begin(); k != keyframes.end(); ++k) {
            const KeyFrame & key = (*k);

            // Do not draw keyframes out of range
            if ( ( key.getTime() < btmLeft.x() ) || ( key.getTime() > topRight.x() ) || ( key.getValue() < btmLeft.y() ) || ( key.getValue() > topRight.y() ) ) {
                continue;
            }

            GL_GPU::Color4f(_imp->color[0], _imp->color[1], _imp->color[2], _imp->color[3]);

            bool drawKeySelected = false;
            if (foundThisCurveSelectedKeys) {
                KeyFrameSet::const_iterator start = foundSelectedKey == foundThisCurveSelectedKeys->end() ? foundThisCurveSelectedKeys->begin() : foundSelectedKey;
                foundSelectedKey = std::find_if(start, foundThisCurveSelectedKeys->end(), KeyFrameWithStringTimePredicate(key.getTime()));
                drawKeySelected = foundSelectedKey != foundThisCurveSelectedKeys->end();
                if (!drawKeySelected) {
                    // Also draw the keyframe as selected if it is inside the selection rectangle (but not yet selected)
                    RectD selectionRect = _imp->curveWidget->getSelectionRectangle();
                    drawKeySelected |= _imp->curveWidget->_imp->eventTriggeredFromCurveEditor && (!selectionRect.isNull() && selectionRect.contains(key.getTime(), key.getValue()));
                }
            }
            // If the key is selected change its color
            if (drawKeySelected) {
                GL_GPU::Color4f(0.8f, 0.8f, 0.8f, 1.f);
            }


            RectD keyframeBbox = _imp->curveWidget->_imp->getKeyFrameBoundingRectCanonical(_imp->curveWidget->_imp->curveEditorZoomContext, key.getTime(), key.getValue());

            // draw texture of the keyframe
            {
                AnimationModuleViewPrivate::KeyframeTexture texType = AnimationModuleViewPrivate::kfTextureFromKeyframeType( key.getInterpolation(), drawKeySelected);
                if (texType != AnimationModuleViewPrivate::kfTextureNone) {
                    _imp->curveWidget->_imp->drawTexturedKeyframe(texType, keyframeBbox, false /*drawdimed*/);
                }
            }


            // Draw tangents if not constant
            bool drawTangents = drawKeySelected && internalCurve->isYComponentMovable() && (key.getInterpolation() != eKeyframeTypeConstant);
            if (drawTangents) {
                QFontMetrics m( _imp->curveWidget->font());


                // If interpolation is not free and not broken display with dashes the tangents lines
                if ( (key.getInterpolation() != eKeyframeTypeFree) && (key.getInterpolation() != eKeyframeTypeBroken) ) {
                    GL_GPU::LineStipple(2, 0xAAAA);
                    GL_GPU::Enable(GL_LINE_STIPPLE);
                }

                QPointF leftTanPos, rightTanPos;
                _imp->curveWidget->getKeyTangentPoints(k, keyframes, &leftTanPos, &rightTanPos);

                // Draw the derivatives lines
                GL_GPU::Begin(GL_LINES);
                GL_GPU::Color4f(1., 0.35, 0.35, 1.);
                GL_GPU::Vertex2f( leftTanPos.x(), leftTanPos.y() );
                GL_GPU::Vertex2f(key.getTime(), key.getValue());
                GL_GPU::Vertex2f(key.getTime(), key.getValue());
                GL_GPU::Vertex2f( rightTanPos.x(), rightTanPos.y());
                GL_GPU::End();
                if ( (key.getInterpolation() != eKeyframeTypeFree) && (key.getInterpolation() != eKeyframeTypeBroken) ) {
                    GL_GPU::Disable(GL_LINE_STIPPLE);
                }


                // Draw the tangents handles
                GL_GPU::Begin(GL_POINTS);
                GL_GPU::Vertex2f( leftTanPos.x(), leftTanPos.y() );
                GL_GPU::Vertex2f( rightTanPos.x(), rightTanPos.y());
                GL_GPU::End();

                // If only one keyframe is selected, also draw the coordinates
                if (selectedKeys.size() == 1 && foundThisCurveSelectedKeys && foundThisCurveSelectedKeys->size() == 1) {
                    double rounding = ipow(10, CURVEWIDGET_DERIVATIVE_ROUND_PRECISION);
                    QString leftDerivStr = QString::fromUtf8("l: %1").arg(std::floor( (key.getLeftDerivative() * rounding) + 0.5 ) / rounding);
                    QString rightDerivStr = QString::fromUtf8("r: %1").arg(std::floor( (key.getRightDerivative() * rounding) + 0.5 ) / rounding);
                    double yLeftWidgetCoord = _imp->curveWidget->toWidgetCoordinates(0, leftTanPos.y()).y();
                    yLeftWidgetCoord += (m.height() + 4);

                    double yRightWidgetCoord = _imp->curveWidget->toWidgetCoordinates(0, rightTanPos.y()).y();
                    yRightWidgetCoord += (m.height() + 4);

                    GL_GPU::Color4f(1., 1., 1., 1.);
                    glCheckFramebufferError(GL_GPU);
                    _imp->curveWidget->renderText( leftTanPos.x(), _imp->curveWidget->toZoomCoordinates(0, yLeftWidgetCoord).y(),
                                              leftDerivStr.toStdString(), 0.9, 0.9, 0.9, 1.);
                    _imp->curveWidget->renderText( rightTanPos.x(), _imp->curveWidget->toZoomCoordinates(0, yRightWidgetCoord).y(),
                                                  rightDerivStr.toStdString(), 0.9, 0.9, 0.9, 1.);


                    QString coordStr = QString::fromUtf8("x: %1, y: %2");
                    coordStr = coordStr.arg(key.getTime()).arg(key.getValue());
                    double yWidgetCoord = _imp->curveWidget->toWidgetCoordinates( 0, key.getValue() ).y();
                    yWidgetCoord += (m.height() + 4);
                    GL_GPU::Color4f(1., 1., 1., 1.);
                    glCheckFramebufferError(GL_GPU);
                    _imp->curveWidget->renderText( key.getTime(), _imp->curveWidget->toZoomCoordinates(0, yWidgetCoord).y(),
                                                  coordStr.toStdString(), 0.9, 0.9, 0.9, 1.);

                }

            } // drawTangents

        } // for (KeyFrameSet::const_iterator k = keyframes.begin(); k != keyframes.end(); ++k) {
    } // GLProtectAttrib(GL_HINT_BIT | GL_ENABLE_BIT | GL_LINE_BIT | GL_COLOR_BUFFER_BIT | GL_POINT_BIT | GL_CURRENT_BIT);

    glCheckError(GL_GPU);
} // drawCurve


NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_CurveGui.cpp"
