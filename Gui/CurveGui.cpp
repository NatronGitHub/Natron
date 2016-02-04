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
#include <QDebug>

#include "Engine/Bezier.h"
#include "Engine/Knob.h"
#include "Engine/KnobTypes.h"
#include "Engine/RotoContext.h" // Bezier

#include "Gui/CurveWidget.h"
#include "Gui/CurveWidgetPrivate.h"
#include "Gui/KnobGui.h"

// warning: 'gluErrorString' is deprecated: first deprecated in OS X 10.9 [-Wdeprecated-declarations]
CLANG_DIAG_OFF(deprecated-declarations)
GCC_DIAG_OFF(deprecated-declarations)

NATRON_NAMESPACE_ENTER;

CurveGui::CurveGui(const CurveWidget *curveWidget,
                   boost::shared_ptr<Curve> curve,
                   const QString & name,
                   const QColor & color,
                   int thickness)
    : _internalCurve(curve)
    , _name(name)
    , _color(color)
    , _thickness(thickness)
    , _visible(false)
    , _selected(false)
    , _curveWidget(curveWidget)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );


    QObject::connect( this,SIGNAL( curveChanged() ),curveWidget,SLOT( onCurveChanged() ) );
}

CurveGui::~CurveGui()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
}

std::pair<KeyFrame,bool> CurveGui::nextPointForSegment(const double x1,
                                                       double* x2,
                                                       const KeyFrameSet & keys,
                                                       const std::list<double>& keysWidgetCoords,
                                                       const double xminCurveWidgetCoord,
                                                       const double xmaxCurveWidgetCoord)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    assert( !keys.empty() );
    
    std::pair<double,double> curveYRange = getCurveYRange();

    if (x1 < xminCurveWidgetCoord) {
        if ( ( curveYRange.first <= kOfxFlagInfiniteMin) && ( curveYRange.second >= kOfxFlagInfiniteMax) ) {
            *x2 = xminCurveWidgetCoord;
        } else {
            ///the curve has a min/max, find out the slope of the curve so we know whether the curve intersects
            ///the min axis, the max axis or nothing.
            if (keys.size() == 1) {
                ///if only 1 keyframe, the curve is horizontal
                *x2 = xminCurveWidgetCoord;
            } else {
                ///find out the equation of the straight line going from the first keyframe and intersecting
                ///the min axis, so we can get the coordinates of the point intersecting the min axis.
                KeyFrameSet::const_iterator firstKf = keys.begin();

                if (firstKf->getLeftDerivative() == 0) {
                    *x2 = xminCurveWidgetCoord;
                } else {
                    double b = firstKf->getValue() - firstKf->getLeftDerivative() * firstKf->getTime();
                    *x2 = _curveWidget->toWidgetCoordinates( (curveYRange.first - b) / firstKf->getLeftDerivative(),0 ).x();
                    if ( (x1 >= *x2) || (*x2 > xminCurveWidgetCoord) ) {
                        ///do the same wit hthe max axis
                        *x2 = _curveWidget->toWidgetCoordinates( (curveYRange.second - b) / firstKf->getLeftDerivative(),0 ).x();

                        if ( (x1 >= *x2) || (*x2 > xminCurveWidgetCoord) ) {
                            /// ok the curve doesn't intersect the min/max axis
                            *x2 = xminCurveWidgetCoord;
                        }
                    }
                }
            }
        }
    } else if (x1 >= xmaxCurveWidgetCoord) {
        if ( (curveYRange.first <= kOfxFlagInfiniteMin) && (curveYRange.second >= kOfxFlagInfiniteMax) ) {
            *x2 = _curveWidget->width() - 1;
        } else {
            ///the curve has a min/max, find out the slope of the curve so we know whether the curve intersects
            ///the min axis, the max axis or nothing.
            if (keys.size() == 1) {
                ///if only 1 keyframe, the curve is horizontal
                *x2 = _curveWidget->width() - 1;
            } else {
                ///find out the equation of the straight line going from the last keyframe and intersecting
                ///the min axis, so we can get the coordinates of the point intersecting the min axis.
                KeyFrameSet::const_reverse_iterator lastKf = keys.rbegin();

                if (lastKf->getRightDerivative() == 0) {
                    *x2 = _curveWidget->width() - 1;
                } else {
                    double b = lastKf->getValue() - lastKf->getRightDerivative() * lastKf->getTime();
                    *x2 = _curveWidget->toWidgetCoordinates( (curveYRange.first - b) / lastKf->getRightDerivative(),0 ).x();
                    if ( (x1 >= *x2) || (*x2 < xmaxCurveWidgetCoord) ) {
                        ///do the same wit hthe min axis
                        *x2 = _curveWidget->toWidgetCoordinates( (curveYRange.second - b) / lastKf->getRightDerivative(),0 ).x();

                        if ( (x1 >= *x2) || (*x2 < xmaxCurveWidgetCoord) ) {
                            /// ok the curve doesn't intersect the min/max axis
                            *x2 = _curveWidget->width() - 1;
                        }
                    }
                }
            }
        }
    } else {
        //we're between 2 keyframes,get the upper and lower
        assert(keys.size() == keysWidgetCoords.size());
        KeyFrameSet::const_iterator upper = keys.end();
        KeyFrameSet::const_iterator itKeys = keys.begin();
        double upperWidgetCoord = x1;
        for (std::list<double>::const_iterator it = keysWidgetCoords.begin(); it !=keysWidgetCoords.end(); ++it, ++itKeys) {
            if (*it > x1) {
                upperWidgetCoord = *it;
                upper = itKeys;
                break;
            }
        }
        assert( upper != keys.end() && upper != keys.begin() );
        if (upper == keys.end() || upper == keys.begin()) {
            return std::make_pair(KeyFrame(0.,0.),false);
        }
        KeyFrameSet::const_iterator lower = upper;
        --lower;

        double t = ( x1 - lower->getTime() ) / ( upper->getTime() - lower->getTime() );
        double P3 = upper->getValue();
        double P0 = lower->getValue();
        // Hermite coefficients P0' and P3' are for t normalized in [0,1]
        double P3pl = upper->getLeftDerivative() / ( upper->getTime() - lower->getTime() ); // normalize for t \in [0,1]
        double P0pr = lower->getRightDerivative() / ( upper->getTime() - lower->getTime() ); // normalize for t \in [0,1]
        double secondDer = 6. * (1. - t) * (P3 - P3pl / 3. - P0 - 2. * P0pr / 3.) +
                6. * t * (P0 - P3 + 2 * P3pl / 3. + P0pr / 3. );
        double secondDerWidgetCoord = std::abs( _curveWidget->toWidgetCoordinates(0,secondDer).y()
                                                / ( upper->getTime() - lower->getTime() ) );
        // compute delta_x so that the y difference between the derivative and the curve is at most
        // 1 pixel (use the second order Taylor expansion of the function)
        double delta_x = std::max(2. / std::max(std::sqrt(secondDerWidgetCoord),0.1), 1.);

        if (upperWidgetCoord < x1 + delta_x) {
            *x2 = upperWidgetCoord;

            return std::make_pair(*upper,true);
        } else {
            *x2 = x1 + delta_x;
        }
    }

    return std::make_pair(KeyFrame(0.,0.),false);
} // nextPointForSegment

std::pair<double,double>
CurveGui::getCurveYRange() const
{
    try {
        return getInternalCurve()->getCurveYRange();
    } catch (const std::exception & e) {
        qDebug() << e.what();
        return std::make_pair(INT_MIN,INT_MAX);
    }
}

boost::shared_ptr<Curve>
CurveGui::getInternalCurve() const
{
    return _internalCurve;
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

    std::vector<float> vertices,exprVertices;
    double x1 = 0;
    double x2;
    const double widgetWidth = _curveWidget->width();
    KeyFrameSet keyframes;
    BezierCPCurveGui* isBezier = dynamic_cast<BezierCPCurveGui*>(this);
    KnobCurveGui* isKnobCurve = dynamic_cast<KnobCurveGui*>(this);
    
    bool hasDrawnExpr = false;
    if (isKnobCurve) {
        std::string expr;
        KnobPtr knob = isKnobCurve->getInternalKnob();
        assert(knob);
        expr = knob->getExpression(isKnobCurve->getDimension());
        if (!expr.empty()) {
            //we have no choice but to evaluate the expression at each time
            for (int i = x1; i < widgetWidth; ++i) {
                double x = _curveWidget->toZoomCoordinates(i,0).x();;
                double y = knob->getValueAtWithExpression(x, isKnobCurve->getDimension());
                exprVertices.push_back(x);
                exprVertices.push_back(y);
            }
            hasDrawnExpr = true;
        }
    }
    
    if (isBezier) {
        std::set<double> keys;
        isBezier->getBezier()->getKeyframeTimes(&keys);
        int i = 0;
        for (std::set<double>::iterator it = keys.begin(); it != keys.end(); ++it, ++i) {
            keyframes.insert(KeyFrame(*it,i));
        }
    } else {
        keyframes = getInternalCurve()->getKeyFrames_mt_safe();
    }
    if (!keyframes.empty()) {
        
        try {
            double xminCurveWidgetCoord = _curveWidget->toWidgetCoordinates(keyframes.begin()->getTime(),0).x();
            double xmaxCurveWidgetCoord = _curveWidget->toWidgetCoordinates(keyframes.rbegin()->getTime(),0).x();
            std::pair<KeyFrame,bool> isX1AKey = std::make_pair(KeyFrame(), false);
            
            std::list<double> keysWidgetCoords;
            for (KeyFrameSet::const_iterator it = keyframes.begin(); it != keyframes.end(); ++it) {
                double widgetCoord = _curveWidget->toWidgetCoordinates(it->getTime(),0).x();
                keysWidgetCoords.push_back(widgetCoord);
            }
            
            while (x1 < (widgetWidth - 1)) {
                double x,y;
                if (!isX1AKey.second) {
                    x = _curveWidget->toZoomCoordinates(x1,0).x();
                    y = evaluate(false,x);
                } else {
                    x = isX1AKey.first.getTime();
                    y = isX1AKey.first.getValue();
                }
                vertices.push_back( (float)x );
                vertices.push_back( (float)y );
                isX1AKey = nextPointForSegment(x1,&x2,keyframes, keysWidgetCoords, xminCurveWidgetCoord, xmaxCurveWidgetCoord);
                x1 = x2;
            }
            //also add the last point
            {
                double x = _curveWidget->toZoomCoordinates(x1,0).x();
                double y = evaluate(false,x);
                vertices.push_back( (float)x );
                vertices.push_back( (float)y );
            }
        } catch (...) {
            
        }
    }
    
    QPointF btmLeft = _curveWidget->toZoomCoordinates(0,_curveWidget->height() - 1);
    QPointF topRight = _curveWidget->toZoomCoordinates(_curveWidget->width() - 1, 0);

    const QColor & curveColor = _selected ?  _curveWidget->getSelectedCurveColor() : _color;

    {
        GLProtectAttrib a(GL_HINT_BIT | GL_ENABLE_BIT | GL_LINE_BIT | GL_COLOR_BUFFER_BIT | GL_POINT_BIT | GL_CURRENT_BIT);
        
        if (!isBezier && _selected) {
            ///Draw y min/max axis so the user understands why the curve is clamped
            std::pair<double,double> curveYRange = getCurveYRange();
            if (curveYRange.first != INT_MIN && curveYRange.second != INT_MAX) {
                QColor minMaxColor;
                minMaxColor.setRgbF(0.398979,0.398979,0.398979);
                glColor4d(minMaxColor.redF(),minMaxColor.greenF(),minMaxColor.blueF(),1.);
                glBegin(GL_LINES);
                glVertex2d(btmLeft.x(), curveYRange.first);
                glVertex2d(topRight.x(), curveYRange.first);
                glVertex2d(btmLeft.x(), curveYRange.second);
                glVertex2d(topRight.x(), curveYRange.second);
                glEnd();
                glColor4d(1., 1., 1., 1.);
                
                double xText = _curveWidget->toZoomCoordinates(10, 0).x();
                
                _curveWidget->renderText(xText, curveYRange.first, QString("min"), minMaxColor, _curveWidget->font());
                _curveWidget->renderText(xText, curveYRange.second, QString("max"), minMaxColor, _curveWidget->font());
            }
            
        }
        
        
        glColor4f( curveColor.redF(), curveColor.greenF(), curveColor.blueF(), curveColor.alphaF() );
        glPointSize(_thickness);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_LINE_SMOOTH);
        glHint(GL_LINE_SMOOTH_HINT,GL_DONT_CARE);
        glLineWidth(1.5);
        glCheckError();
        if (hasDrawnExpr) {
            glBegin(GL_LINE_STRIP);
            bool prevVisible = true;
            for (int i = 0; i < (int)exprVertices.size(); i += 2) {
                bool vertexVisible = exprVertices[i] >= btmLeft.x() && exprVertices[i] <= topRight.x() &&
                exprVertices[i+1] >= btmLeft.y() && exprVertices[i+1] <= topRight.y();
                bool previousWasVisible = prevVisible;
                prevVisible = vertexVisible;
                if (!previousWasVisible && !vertexVisible) {
                    continue;
                }
                glVertex2f(exprVertices[i],exprVertices[i + 1]);
            }
            glEnd();
            
            
            glLineStipple(2, 0xAAAA);
            glEnable(GL_LINE_STIPPLE);
        }
        glBegin(GL_LINE_STRIP);
        
        bool prevVisible = true;
        for (int i = 0; i < (int)vertices.size(); i += 2) {
            bool vertexVisible = vertices[i] >= btmLeft.x() && vertices[i] <= topRight.x() &&
            vertices[i+1] >= btmLeft.y() && vertices[i+1] <= topRight.y();
            bool previousWasVisible = prevVisible;
            prevVisible = vertexVisible;
            if (!previousWasVisible && !vertexVisible) {
                continue;
            }
            glVertex2f(vertices[i],vertices[i + 1]);
        }
     
        glEnd();
        if (hasDrawnExpr) {
            glDisable(GL_LINE_STIPPLE);
        }

        glCheckError();
        
        //render the name of the curve
        glColor4f(1.f, 1.f, 1.f, 1.f);
        
        
        
        double interval = ( topRight.x() - btmLeft.x() ) / (double)curvesCount;
        double textX = _curveWidget->toZoomCoordinates(15, 0).x() + interval * (double)curveIndex;
        double textY;
        
        try {
            textY = evaluate(_internalCurve && !_internalCurve->isAnimated(),textX);
        } catch (...) {
            textY = evaluate(true,textX);
        }
        
        if (textX >= btmLeft.x() && textX <= topRight.x() && textY >= btmLeft.y() && textY <= topRight.y()) {
            _curveWidget->renderText( textX,textY,_name,_color,_curveWidget->getFont() );
        }
        glColor4f( curveColor.redF(), curveColor.greenF(), curveColor.blueF(), curveColor.alphaF() );
        
        //draw keyframes
        glPointSize(7.f);
        glEnable(GL_POINT_SMOOTH);
        
        const SelectedKeys & selectedKeyFrames = _curveWidget->getSelectedKeyFrames();
        for (KeyFrameSet::const_iterator k = keyframes.begin(); k != keyframes.end(); ++k) {
            const KeyFrame & key = (*k);
            
            if (key.getTime() < btmLeft.x() || key.getTime() > topRight.x() || key.getValue() < btmLeft.y() || key.getValue() > topRight.y()) {
                continue;
            }
                
            glColor4f( _color.redF(), _color.greenF(), _color.blueF(), _color.alphaF() );

            //if the key is selected change its color to white
            SelectedKeys::const_iterator isSelected = selectedKeyFrames.end();
            for (SelectedKeys::const_iterator it2 = selectedKeyFrames.begin();
                 it2 != selectedKeyFrames.end(); ++it2) {
                if ( ( (*it2)->key.getTime() == key.getTime() ) && ( (*it2)->curve.get() == this ) ) {
                    isSelected = it2;
                    glColor4f(1.f,1.f,1.f,1.f);
                    break;
                }
            }
            double x = key.getTime();
            double y = key.getValue();
            glBegin(GL_POINTS);
            glVertex2f(x,y);
            glEnd();
            
            if ( !isBezier && ( isSelected != selectedKeyFrames.end() ) && (key.getInterpolation() != eKeyframeTypeConstant) ) {
                
                QFontMetrics m( _curveWidget->getFont() );
                
                
                //draw the derivatives lines
                if ( (key.getInterpolation() != eKeyframeTypeFree) && (key.getInterpolation() != eKeyframeTypeBroken) ) {
                    glLineStipple(2, 0xAAAA);
                    glEnable(GL_LINE_STIPPLE);
                }
                glBegin(GL_LINES);
                glColor4f(1., 0.35, 0.35, 1.);
                glVertex2f( (*isSelected)->leftTan.first, (*isSelected)->leftTan.second );
                glVertex2f(x, y);
                glVertex2f(x, y);
                glVertex2f( (*isSelected)->rightTan.first, (*isSelected)->rightTan.second );
                glEnd();
                if ( (key.getInterpolation() != eKeyframeTypeFree) && (key.getInterpolation() != eKeyframeTypeBroken) ) {
                    glDisable(GL_LINE_STIPPLE);
                    
                }
                
                if (selectedKeyFrames.size() == 1) { //if one keyframe, also draw the coordinates
                    
                    double rounding = std::pow(10., CURVEWIDGET_DERIVATIVE_ROUND_PRECISION);
                    
                    QString leftDerivStr = QString("l: %1").arg(std::floor((key.getLeftDerivative() * rounding) + 0.5) / rounding);
                    QString rightDerivStr = QString("r: %1").arg(std::floor((key.getRightDerivative() * rounding) + 0.5) / rounding);
                    
                    double yLeftWidgetCoord = _curveWidget->toWidgetCoordinates(0,(*isSelected)->leftTan.second).y();
                    yLeftWidgetCoord += (m.height() + 4);
                    
                    double yRightWidgetCoord = _curveWidget->toWidgetCoordinates(0,(*isSelected)->rightTan.second).y();
                    yRightWidgetCoord += (m.height() + 4);
                    
                    glColor4f(1., 1., 1., 1.);
                    glCheckFramebufferError();
                    _curveWidget->renderText( (*isSelected)->leftTan.first, _curveWidget->toZoomCoordinates(0, yLeftWidgetCoord).y(),
                                              leftDerivStr, QColor(240,240,240), _curveWidget->getFont() );
                    _curveWidget->renderText( (*isSelected)->rightTan.first, _curveWidget->toZoomCoordinates(0, yRightWidgetCoord).y(),
                                              rightDerivStr, QColor(240,240,240), _curveWidget->getFont() );
                }
                
                
                
                if (selectedKeyFrames.size() == 1) { //if one keyframe, also draw the coordinates
                    QString coordStr("x: %1, y: %2");
                    coordStr = coordStr.arg(x).arg(y);
                    double yWidgetCoord = _curveWidget->toWidgetCoordinates( 0,key.getValue() ).y();
                    yWidgetCoord += (m.height() + 4);
                    glColor4f(1., 1., 1., 1.);
                    glCheckFramebufferError();
                    _curveWidget->renderText( x, _curveWidget->toZoomCoordinates(0, yWidgetCoord).y(),
                                              coordStr, QColor(240,240,240), _curveWidget->getFont() );
                }
                glBegin(GL_POINTS);
                glVertex2f( (*isSelected)->leftTan.first, (*isSelected)->leftTan.second );
                glVertex2f( (*isSelected)->rightTan.first, (*isSelected)->rightTan.second );
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
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    _visible = visible;
    Q_EMIT curveChanged();
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

KnobCurveGui::KnobCurveGui(const CurveWidget *curveWidget,
                           boost::shared_ptr<Curve>  curve,
                           KnobGui* knob,
                           int dimension,
                           const QString & name,
                           const QColor & color,
                           int thickness)
    : CurveGui(curveWidget,curve,name,color,thickness)
    , _internalKnob()
    , _knob(knob)
    , _dimension(dimension)
{
    
    // even when there is only one keyframe, there may be tangents!
    if (curve->getKeyFramesCount() > 0) {
        setVisible(true);
    }
}

KnobCurveGui::KnobCurveGui(const CurveWidget *curveWidget,
                           boost::shared_ptr<Curve>  curve,
                           const KnobPtr& knob,
                           const boost::shared_ptr<RotoContext>& roto,
                           int dimension,
                           const QString & name,
                           const QColor & color,
                           int thickness)
    : CurveGui(curveWidget,curve,name,color,thickness)
    , _roto(roto)
    , _internalKnob(knob)
    , _knob(0)
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

double
KnobCurveGui::evaluate(bool useExpr,double x) const
{
    KnobPtr knob = getInternalKnob();
    if (useExpr) {
        return knob->getValueAtWithExpression(x,_dimension);
    } else {
        KnobParametric* isParametric = dynamic_cast<KnobParametric*>(knob.get());
        if (isParametric) {
            return isParametric->getParametricCurve(_dimension)->getValueAt(x);
        } else {
            assert(_internalCurve);
            return _internalCurve->getValueAt(x,false);
        }
    }
}

boost::shared_ptr<Curve>
KnobCurveGui::getInternalCurve() const
{
    KnobPtr knob = getInternalKnob();
    KnobParametric* isParametric = dynamic_cast<KnobParametric*>(knob.get());
    if (!knob || !isParametric) {
        return CurveGui::getInternalCurve();
    }
    return isParametric->getParametricCurve(_dimension);
}

KnobPtr
KnobCurveGui::getInternalKnob() const
{
    return _knob ? _knob->getKnob() : _internalKnob;
}

int
KnobCurveGui::getKeyFrameIndex(double time) const
{
    return getInternalCurve()->keyFrameIndex(time);
}

KeyFrame
KnobCurveGui::setKeyFrameInterpolation(KeyframeTypeEnum interp,int index)
{
    return getInternalCurve()->setKeyFrameInterpolation(interp, index);
}

BezierCPCurveGui::BezierCPCurveGui(const CurveWidget *curveWidget,
                                   const boost::shared_ptr<Bezier>& bezier,
                                   const boost::shared_ptr<RotoContext>& roto,
                                   const QString & name,
                                   const QColor & color,
                                   int thickness)
    : CurveGui(curveWidget,boost::shared_ptr<Curve>(),name,color,thickness)
    , _bezier(bezier)
    , _rotoContext(roto)
{
    // even when there is only one keyframe, there may be tangents!
    if (bezier->getKeyframesCount() > 0) {
        setVisible(true);
    }
}

boost::shared_ptr<Bezier>
BezierCPCurveGui::getBezier() const
{
    return _bezier;
}

BezierCPCurveGui::~BezierCPCurveGui()
{
    
}

double
BezierCPCurveGui::evaluate(bool /*useExpr*/,double x) const
{
    std::list<std::pair<double,KeyframeTypeEnum> > keys;
    _bezier->getKeyframeTimesAndInterpolation(&keys);
    
    std::list<std::pair<double,KeyframeTypeEnum> >::iterator upb = keys.end();
    int dist = 0;
    for (std::list<std::pair<double,KeyframeTypeEnum> >::iterator it = keys.begin(); it != keys.end(); ++it, ++dist) {
        if (it->first > x) {
            upb = it;
            break;
        }
    }
    
    if (upb == keys.end()) {
        return keys.size() - 1;
    } else if (upb == keys.begin()) {
        return 0;
    } else {
        std::list<std::pair<double,KeyframeTypeEnum> >::iterator prev = upb;
        if (prev != keys.begin()) {
            --prev;
        }
        if (prev->second == eKeyframeTypeConstant) {
            return dist - 1;
        } else {
            ///Always linear for bezier interpolation
            assert((upb->first - prev->first) != 0);
            return ((double)(x - prev->first) / (upb->first - prev->first)) + dist - 1;
        }
    }
}

std::pair<double,double>
BezierCPCurveGui::getCurveYRange() const
{
    int keys = _bezier->getKeyframesCount();
    return std::make_pair(0, keys - 1);
}

KeyFrameSet
BezierCPCurveGui::getKeyFrames() const
{
    KeyFrameSet ret;
    std::set<double> keys;
    _bezier->getKeyframeTimes(&keys);
    int i = 0;
    for (std::set<double>::iterator it = keys.begin(); it != keys.end(); ++it, ++i) {
        ret.insert(KeyFrame(*it,i));
    }
    return ret;
}

int
BezierCPCurveGui::getKeyFrameIndex(double time) const
{
    return _bezier->getKeyFrameIndex(time);
}

KeyFrame
BezierCPCurveGui::setKeyFrameInterpolation(KeyframeTypeEnum interp,int index)
{
    _bezier->setKeyFrameInterpolation(interp, index);
    return KeyFrame();
}

NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_CurveGui.cpp"
