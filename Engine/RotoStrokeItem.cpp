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

#include "RotoStrokeItem.h"

#include <algorithm> // min, max
#include <sstream>
#include <locale>
#include <limits>
#include <cassert>
#include <stdexcept>

#include <QtCore/QLineF>
#include <QtCore/QDebug>

GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
// /usr/local/include/boost/bind/arg.hpp:37:9: warning: unused typedef 'boost_static_assert_typedef_37' [-Wunused-local-typedef]
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/math/special_functions/fpclassify.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON

#include "Global/MemoryInfo.h"
#include "Engine/RotoContextPrivate.h"

#include "Engine/AppInstance.h"
#include "Engine/Bezier.h"
#include "Engine/BezierCP.h"
#include "Engine/CoonsRegularization.h"
#include "Engine/FeatherPoint.h"
#include "Engine/Format.h"
#include "Engine/Hash64.h"
#include "Engine/Image.h"
#include "Engine/ImageParams.h"
#include "Engine/Hash64.h"
#include "Engine/Interpolation.h"
#include "Engine/RenderStats.h"
#include "Engine/RotoShapeRenderCairo.h"
#include "Engine/RotoPoint.h"
#include "Engine/Settings.h"
#include "Engine/TimeLine.h"
#include "Engine/Transform.h"
#include "Engine/ViewerInstance.h"

#include "Serialization/CurveSerialization.h"
#include "Serialization/RotoStrokeItemSerialization.h"

#define kMergeOFXParamOperation "operation"
#define kBlurCImgParamSize "size"
#define kTimeOffsetParamOffset "timeOffset"
#define kFrameHoldParamFirstFrame "firstFrame"

#define kTransformParamTranslate "translate"
#define kTransformParamRotate "rotate"
#define kTransformParamScale "scale"
#define kTransformParamUniform "uniform"
#define kTransformParamSkewX "skewX"
#define kTransformParamSkewY "skewY"
#define kTransformParamSkewOrder "skewOrder"
#define kTransformParamCenter "center"
#define kTransformParamFilter "filter"
#define kTransformParamResetCenter "resetCenter"
#define kTransformParamBlackOutside "black_outside"


#ifndef M_PI
#define M_PI        3.14159265358979323846264338327950288   /* pi             */
#endif

NATRON_NAMESPACE_ENTER;

////////////////////////////////////Stroke//////////////////////////////////

RotoStrokeItem::RotoStrokeItem(RotoStrokeType type,
                               const RotoContextPtr& context,
                               const std::string & name,
                               const RotoLayerPtr& parent)

    : RotoDrawableItem(context, name, parent)
    , _imp( new RotoStrokeItemPrivate(type) )
{
}

RotoStrokeItem::~RotoStrokeItem()
{
#ifdef ROTO_SHAPE_RENDER_ENABLE_CAIRO
    RotoShapeRenderCairo::purgeCaches_cairo_internal(_imp->strokeDotPatterns);
#endif
    deactivateNodes();
}

RotoStrokeType
RotoStrokeItem::getBrushType() const
{
    return _imp->type;
}

static void
evaluateStrokeInternal(const KeyFrameSet& xCurve,
                       const KeyFrameSet& yCurve,
                       const KeyFrameSet& pCurve,
                       const Transform::Matrix3x3& transform,
                       unsigned int mipMapLevel,
                       double halfBrushSize,
                       bool pressureAffectsSize,
                       std::list<std::pair<Point, double> >* points,
                       RectD* bbox)
{
    //Increment the half brush size so that the stroke is enclosed in the RoD
    halfBrushSize += 1.;
    halfBrushSize = std::max(0.5, halfBrushSize);

    bool bboxSet = false;
    if (bbox) {
        bbox->clear();
    }

    if ( xCurve.empty() ) {
        return;
    }

    assert( xCurve.size() == yCurve.size() && xCurve.size() == pCurve.size() );

    KeyFrameSet::const_iterator xIt = xCurve.begin();
    KeyFrameSet::const_iterator yIt = yCurve.begin();
    KeyFrameSet::const_iterator pIt = pCurve.begin();
    KeyFrameSet::const_iterator xNext = xIt;
    if ( xNext != xCurve.end() ) {
        ++xNext;
    }
    KeyFrameSet::const_iterator yNext = yIt;
    if ( yNext != yCurve.end() ) {
        ++yNext;
    }
    KeyFrameSet::const_iterator pNext = pIt;
    if ( pNext != pCurve.end() ) {
        ++pNext;
    }


    int pot = 1 << mipMapLevel;

    if ( (xCurve.size() == 1) && ( xIt != xCurve.end() ) && ( yIt != yCurve.end() ) && ( pIt != pCurve.end() ) ) {
        assert( xNext == xCurve.end() && yNext == yCurve.end() && pNext == pCurve.end() );
        Transform::Point3D p;
        p.x = xIt->getValue();
        p.y = yIt->getValue();
        p.z = 1.;

        p = Transform::matApply(transform, p);

        Point pixelPoint;
        pixelPoint.x = p.x / pot;
        pixelPoint.y = p.y / pot;
        points->push_back( std::make_pair( pixelPoint, pIt->getValue() ) );
        if (bbox) {
            bbox->x1 = p.x;
            bbox->x2 = p.x;
            bbox->y1 = p.y;
            bbox->y2 = p.y;
            double pressure = pressureAffectsSize ? pIt->getValue() : 1.;
            double padding = halfBrushSize * pressure;
            bbox->x1 -= padding;
            bbox->x2 += padding;
            bbox->y1 -= padding;
            bbox->y2 += padding;
            bboxSet = true;
        }

    }

    double pressure = 0.;
    for (;
         xNext != xCurve.end() && yNext != yCurve.end() && pNext != pCurve.end();
         ++xIt, ++yIt, ++pIt, ++xNext, ++yNext, ++pNext) {
        assert( xIt != xCurve.end() && yIt != yCurve.end() && pIt != pCurve.end() );

        double dt = xNext->getTime() - xIt->getTime();
        double pressp0 = pIt->getValue();
        double pressp3 = pNext->getValue();
        double pressp1 = pressp0 + dt * pIt->getRightDerivative() / 3.;
        double pressp2 = pressp3 - dt * pNext->getLeftDerivative() / 3.;

        pressure = std::max(pressure, pressureAffectsSize ? std::max(pressp0, pressp3) : 1.);

        Transform::Point3D p0, p1, p2, p3;
        p0.z = p1.z = p2.z = p3.z = 1;
        p0.x = xIt->getValue();
        p0.y = yIt->getValue();
        p1.x = p0.x + dt * xIt->getRightDerivative() / 3.;
        p1.y = p0.y + dt * yIt->getRightDerivative() / 3.;
        p3.x = xNext->getValue();
        p3.y = yNext->getValue();
        p2.x = p3.x - dt * xNext->getLeftDerivative() / 3.;
        p2.y = p3.y - dt * yNext->getLeftDerivative() / 3.;


        p0 = Transform::matApply(transform, p0);
        p1 = Transform::matApply(transform, p1);
        p2 = Transform::matApply(transform, p2);
        p3 = Transform::matApply(transform, p3);



        /*
         * Approximate the necessary number of line segments, using http://antigrain.com/research/adaptive_bezier/
         */
        double dx1, dy1, dx2, dy2, dx3, dy3;
        dx1 = p1.x - p0.x;
        dy1 = p1.y - p0.y;
        dx2 = p2.x - p1.x;
        dy2 = p2.y - p1.y;
        dx3 = p3.x - p2.x;
        dy3 = p3.y - p2.y;
        double length = std::sqrt(dx1 * dx1 + dy1 * dy1) +
        std::sqrt(dx2 * dx2 + dy2 * dy2) +
        std::sqrt(dx3 * dx3 + dy3 * dy3);
        double nbPointsPerSegment = (int)std::max(length * 0.25, 2.);
        double incr = 1. / (double)(nbPointsPerSegment - 1);

        RectD pointBox;
        bool pointBboxSet = false;

        for (int i = 0; i < nbPointsPerSegment; ++i) {
            double t = incr * i;
            Point p;
            p.x = Bezier::bezierEval(p0.x, p1.x, p2.x, p3.x, t);
            p.y = Bezier::bezierEval(p0.y, p1.y, p2.y, p3.y, t);

            if (bbox) {
                if (!pointBboxSet) {
                    pointBox.x1 = p.x;
                    pointBox.x2 = p.x;
                    pointBox.y1 = p.y;
                    pointBox.y2 = p.y;
                    pointBboxSet = true;
                } else {
                    pointBox.x1 = std::min(p.x, pointBox.x1);
                    pointBox.x2 = std::max(p.x, pointBox.x2);
                    pointBox.y1 = std::min(p.y, pointBox.y1);
                    pointBox.y2 = std::max(p.y, pointBox.y2);

                }
            }

            double pi = Bezier::bezierEval(pressp0, pressp1, pressp2, pressp3, t);
            p.x /= pot;
            p.y /= pot;
            points->push_back( std::make_pair(p, pi) );
        }

        double padding = halfBrushSize * pressure;
        pointBox.x1 -= padding;
        pointBox.x2 += padding;
        pointBox.y1 -= padding;
        pointBox.y2 += padding;

        
        if (bbox) {
            if (!bboxSet) {
                bboxSet = true;
                *bbox = pointBox;
            } else {
                bbox->merge(pointBox);
            }
        }


    } // for (; xNext != xCurve.end() ;++xNext, ++yNext, ++pNext) {

} // evaluateStrokeInternal

bool
RotoStrokeItem::isEmpty() const
{
    QMutexLocker k(&itemMutex);

    return _imp->strokes.empty();
}

void
RotoStrokeItem::setStrokeFinished()
{
    {
        QMutexLocker k(&itemMutex);
        _imp->finished = true;

#ifdef ROTO_SHAPE_RENDER_ENABLE_CAIRO
        RotoShapeRenderCairo::purgeCaches_cairo_internal(_imp->strokeDotPatterns);
#endif
    }

    resetTransformCenter();

    NodePtr effectNode = getEffectNode();
    NodePtr maskNode = getMaskNode();
    NodePtr mergeNode = getMergeNode();
    NodePtr timeOffsetNode = getTimeOffsetNode();
    NodePtr frameHoldNode = getFrameHoldNode();

    if (effectNode) {
        effectNode->setWhileCreatingPaintStroke(false);
        effectNode->getEffectInstance()->invalidateHashCache();
    }
    mergeNode->setWhileCreatingPaintStroke(false);
    mergeNode->getEffectInstance()->invalidateHashCache();
    if (timeOffsetNode) {
        timeOffsetNode->setWhileCreatingPaintStroke(false);
        timeOffsetNode->getEffectInstance()->invalidateHashCache();
    }
    if (frameHoldNode) {
        frameHoldNode->setWhileCreatingPaintStroke(false);
        frameHoldNode->getEffectInstance()->invalidateHashCache();
    }
    if (maskNode) {
        maskNode->setWhileCreatingPaintStroke(false);
        maskNode->getEffectInstance()->invalidateHashCache();
    }

    getContext()->setWhileCreatingPaintStrokeOnMergeNodes(false);
    getContext()->clearViewersLastRenderedStrokes();
    //Might have to do this somewhere else if several viewers are active on the rotopaint node
    resetNodesThreadSafety();
}

bool
RotoStrokeItem::appendPoint(bool newStroke,
                            const RotoPoint& p)
{
    assert( QThread::currentThread() == qApp->thread() );


    RotoStrokeItemPtr thisShared = toRotoStrokeItem( shared_from_this() );
    assert(thisShared);
    {
        QMutexLocker k(&itemMutex);
        if (_imp->finished) {
            _imp->finished = false;
        }

        if (newStroke) {
            setNodesThreadSafetyForRotopainting();
        }

        if ( _imp->strokeDotPatterns.empty() ) {
            _imp->strokeDotPatterns.resize(ROTO_PRESSURE_LEVELS);
            for (std::size_t i = 0; i < _imp->strokeDotPatterns.size(); ++i) {
                _imp->strokeDotPatterns[i] = (cairo_pattern_t*)0;
            }
        }

        RotoStrokeItemPrivate::StrokeCurves* stroke = 0;
        if (newStroke) {
            RotoStrokeItemPrivate::StrokeCurves s;
            s.xCurve.reset(new Curve);
            s.yCurve.reset(new Curve);
            s.pressureCurve.reset(new Curve);
            _imp->strokes.push_back(s);
        }
        stroke = &_imp->strokes.back();
        assert(stroke);
        if (!stroke) {
            throw std::logic_error("");
        }

        assert( stroke->xCurve->getKeyFramesCount() == stroke->yCurve->getKeyFramesCount() &&
                stroke->xCurve->getKeyFramesCount() == stroke->pressureCurve->getKeyFramesCount() );
        int nk = stroke->xCurve->getKeyFramesCount();
        double t;
        if (nk == 0) {
            qDebug() << "start stroke!";
            t = 0.;
            // set time origin for this curve
            _imp->curveT0 = p.timestamp();
        } else if (p.timestamp() == 0.) {
            t = nk; // some systems may not have a proper timestamp use a dummy one
        } else {
            t = p.timestamp() - _imp->curveT0;
        }
        if (nk > 0) {
            //Clamp timestamps difference to 1e-3 in case Qt delivers its events all at once
            double dt = t - _imp->lastTimestamp;
            if (dt < 0.01) {
                qDebug() << "dt is lower than 0.01!";
                t = _imp->lastTimestamp + 0.01;
            }
        }
        _imp->lastTimestamp = t;
        qDebug("t[%d]=%g", nk, t);

#if 0   // the following was disabled because it creates oscillations.

        // if it's at least the 3rd point in curve, add intermediate point if
        // the time since last keyframe is larger that the time to the previous one...
        // This avoids overshooting when the pen suddenly stops, and restarts much later
        if (nk >= 2) {
            KeyFrame xp, xpp;
            bool valid;
            valid = _imp->xCurve.getKeyFrameWithIndex(nk - 1, &xp);
            assert(valid);
            valid = _imp->xCurve.getKeyFrameWithIndex(nk - 2, &xpp);
            assert(valid);

            double tp = xp.getTime();
            double tpp = xpp.getTime();
            if ( (t != tp) && (tp != tpp) && ( (t - tp) > (tp - tpp) ) ) {
                //printf("adding extra keyframe, %g > %g\n", t - tp, tp - tpp);
                // add a keyframe to avoid overshoot when the pen stops suddenly and starts again much later
                KeyFrame yp, ypp;
                valid = _imp->yCurve.getKeyFrameWithIndex(nk - 1, &yp);
                assert(valid);
                valid = _imp->yCurve.getKeyFrameWithIndex(nk - 2, &ypp);
                assert(valid);
                KeyFrame pp, ppp;
                valid = _imp->pressureCurve.getKeyFrameWithIndex(nk - 1, &pp);
                assert(valid);
                valid = _imp->pressureCurve.getKeyFrameWithIndex(nk - 2, &ppp);
                assert(valid);
                double tn = tp + (tp - tpp);
                KeyFrame xn, yn, pn;
                double alpha = (tp - tpp) / (t - tp);
                assert(0 < alpha && alpha < 1);
                xn.setTime(tn);
                yn.setTime(tn);
                pn.setTime(tn);
                xn.setValue(xp.getValue() * (1 - alpha) + p.pos.x * alpha);
                yn.setValue(yp.getValue() * (1 - alpha) + p.pos.y * alpha);
                pn.setValue(pp.getValue() * (1 - alpha) + p.pressure * alpha);
                _imp->xCurve.addKeyFrame(xn);
                _imp->xCurve.setKeyFrameInterpolation(eKeyframeTypeCatmullRom, nk);
                _imp->yCurve.addKeyFrame(yn);
                _imp->yCurve.setKeyFrameInterpolation(eKeyframeTypeCatmullRom, nk);
                _imp->pressureCurve.addKeyFrame(pn);
                _imp->pressureCurve.setKeyFrameInterpolation(eKeyframeTypeCatmullRom, nk);
                ++nk;
            }
        }
#endif

        bool addKeyFrameOk; // did we add a new keyframe (normally yes, but just in case)
        int ki; // index of the new keyframe (normally nk, but just in case)
        {
            KeyFrame k;
            k.setTime(t);
            k.setValue(p.pos().x);
            addKeyFrameOk = stroke->xCurve->addKeyFrame(k);
            ki = ( addKeyFrameOk ? nk : (nk - 1) );
        }
        {
            KeyFrame k;
            k.setTime(t);
            k.setValue(p.pos().y);
            bool aok = stroke->yCurve->addKeyFrame(k);
            assert(aok == addKeyFrameOk);
            if (aok != addKeyFrameOk) {
                throw std::logic_error("RotoStrokeItem::appendPoint");
            }
        }

        {
            KeyFrame k;
            k.setTime(t);
            k.setValue( p.pressure() );
            bool aok = stroke->pressureCurve->addKeyFrame(k);
            assert(aok == addKeyFrameOk);
            if (aok != addKeyFrameOk) {
                throw std::logic_error("RotoStrokeItem::appendPoint");
            }
        }
        // Use CatmullRom interpolation, which means that the tangent may be modified by the next point on the curve.
        // In a previous version, the previous keyframe was set to Free so its tangents don't get overwritten, but this caused oscillations.
        KeyframeTypeEnum interpolation = _imp->type == eRotoStrokeTypeSmear ? eKeyframeTypeFree : eKeyframeTypeCatmullRom;
        stroke->xCurve->setKeyFrameInterpolation(interpolation, ki);
        stroke->yCurve->setKeyFrameInterpolation(interpolation, ki);
        stroke->pressureCurve->setKeyFrameInterpolation(interpolation, ki);
    } // QMutexLocker k(&itemMutex);


    return true;
} // RotoStrokeItem::appendPoint

void
RotoStrokeItem::addStroke(const CurvePtr& xCurve,
                          const CurvePtr& yCurve,
                          const CurvePtr& pCurve)
{
    RotoStrokeItemPrivate::StrokeCurves s;

    s.xCurve = xCurve;
    s.yCurve = yCurve;
    s.pressureCurve = pCurve;

    {
        QMutexLocker k(&itemMutex);
        _imp->strokes.push_back(s);
    }
    invalidateHashCache();
}

bool
RotoStrokeItem::removeLastStroke(CurvePtr* xCurve,
                                 CurvePtr* yCurve,
                                 CurvePtr* pCurve)
{
    bool empty;
    {
        QMutexLocker k(&itemMutex);
        if ( _imp->strokes.empty() ) {
            return true;
        }
        RotoStrokeItemPrivate::StrokeCurves& last = _imp->strokes.back();
        *xCurve = last.xCurve;
        *yCurve = last.yCurve;
        *pCurve = last.pressureCurve;
        _imp->strokes.pop_back();
        empty =  _imp->strokes.empty();
    }

    invalidateHashCache();

    return empty;
}

std::vector<cairo_pattern_t*>
RotoStrokeItem::getPatternCache() const
{
    _imp->strokeDotPatternsMutex.lock();
    return _imp->strokeDotPatterns;
}

void
RotoStrokeItem::updatePatternCache(const std::vector<cairo_pattern_t*>& cache)
{
    _imp->strokeDotPatterns = cache;
    _imp->strokeDotPatternsMutex.unlock();
}

void
RotoStrokeItem::setDrawingGLContext(const OSGLContextPtr& gpuContext, const OSGLContextPtr& cpuContext)
{
    QMutexLocker k(&itemMutex);
    _imp->drawingGlGpuContext = gpuContext;
    _imp->drawingGlCpuContext = cpuContext;
}


void
RotoStrokeItem::getDrawingGLContext(OSGLContextPtr* gpuContext, OSGLContextPtr* cpuContext) const
{
    QMutexLocker k(&itemMutex);
    *gpuContext = _imp->drawingGlGpuContext.lock();
    *cpuContext = _imp->drawingGlCpuContext.lock();
}

RectD
RotoStrokeItem::getWholeStrokeRoDWhilePainting() const
{
    QMutexLocker k(&itemMutex);

    return _imp->wholeStrokeBboxWhilePainting;
}

bool
RotoStrokeItem::getMostRecentStrokeChangesSinceAge(double time,
                                                   int lastAge,
                                                   int lastMultiStrokeIndex,
                                                   std::list<std::pair<Point, double> >* points,
                                                   RectD* pointsBbox,
                                                   RectD* wholeStrokeBbox,
                                                   bool *isStrokeFirstTick,
                                                   int* newAge,
                                                   int* strokeIndex)
{
    Transform::Matrix3x3 transform;

    getTransformAtTime(time, &transform);

    if (lastAge == -1) {
        *wholeStrokeBbox = computeBoundingBox(time);
    } else {
        QMutexLocker k(&itemMutex);
        *wholeStrokeBbox = _imp->wholeStrokeBboxWhilePainting;
    }
    *isStrokeFirstTick = false;

    QMutexLocker k(&itemMutex);
    assert( !_imp->strokes.empty() );
    if ( (lastMultiStrokeIndex < 0) || ( lastMultiStrokeIndex >= (int)_imp->strokes.size() ) ) {
        return false;
    }
    RotoStrokeItemPrivate::StrokeCurves* stroke = 0;
    assert(_imp->strokes[lastMultiStrokeIndex].xCurve &&
           _imp->strokes[lastMultiStrokeIndex].yCurve &&
           _imp->strokes[lastMultiStrokeIndex].pressureCurve);
    if (!_imp->strokes[lastMultiStrokeIndex].xCurve ||
        !_imp->strokes[lastMultiStrokeIndex].yCurve ||
        !_imp->strokes[lastMultiStrokeIndex].pressureCurve) {
        return false;
    }
    if (lastAge == _imp->strokes[lastMultiStrokeIndex].xCurve->getKeyFramesCount() - 1) {
        //We rendered completly that stroke so far, pick the next one if there is
        if (lastMultiStrokeIndex == (int)_imp->strokes.size() - 1) {
            //nothing to do
            return false;
        } else {
            assert( lastMultiStrokeIndex + 1 < (int)_imp->strokes.size() );
            stroke = &_imp->strokes[lastMultiStrokeIndex + 1];
            *strokeIndex = lastMultiStrokeIndex + 1;
        }
    } else {
        stroke = &_imp->strokes[lastMultiStrokeIndex];
        *strokeIndex = lastMultiStrokeIndex;
    }
    if (!stroke) {
        return false;
    }
    assert( stroke && stroke->xCurve->getKeyFramesCount() == stroke->yCurve->getKeyFramesCount() && stroke->xCurve->getKeyFramesCount() == stroke->pressureCurve->getKeyFramesCount() );

    //We changed stroke index so far, reset the age
    if ( (*strokeIndex != lastMultiStrokeIndex) && (lastAge != -1) ) {
        lastAge = -1;
        *wholeStrokeBbox = computeBoundingBoxInternal(time);
    }


    if (lastAge == -1) {
        *isStrokeFirstTick = true;
    }

    KeyFrameSet xCurve = stroke->xCurve->getKeyFrames_mt_safe();
    KeyFrameSet yCurve = stroke->yCurve->getKeyFrames_mt_safe();
    KeyFrameSet pCurve = stroke->pressureCurve->getKeyFrames_mt_safe();

    if ( xCurve.empty() ) {
        return false;
    }
    if (lastAge == -1) {
        lastAge = 0;
    }

    if (lastAge >= (int) xCurve.size()) {
        return false;
    }


    KeyFrameSet realX, realY, realP;
    KeyFrameSet::iterator xIt = xCurve.begin();
    KeyFrameSet::iterator yIt = yCurve.begin();
    KeyFrameSet::iterator pIt = pCurve.begin();
    std::advance(xIt, lastAge);
    std::advance(yIt, lastAge);
    std::advance(pIt, lastAge);
    *newAge = (int)xCurve.size() - 1;

    if ( lastAge < (int)xCurve.size() ) {
        for (; xIt != xCurve.end(); ++xIt, ++yIt, ++pIt) {
            realX.insert(*xIt);
            realY.insert(*yIt);
            realP.insert(*pIt);
        }
    }

    if ( realX.empty() ) {
        return false;
    }

    double halfBrushSize = getBrushSizeKnob()->getValue() / 2.;
    bool pressureSize = getPressureSizeKnob()->getValue();
    evaluateStrokeInternal(realX, realY, realP, transform, 0, halfBrushSize, pressureSize, points, pointsBbox);

    if ( !wholeStrokeBbox->isNull() ) {
        wholeStrokeBbox->merge(*pointsBbox);
    } else {
        *wholeStrokeBbox = *pointsBbox;
    }

    _imp->wholeStrokeBboxWhilePainting = *wholeStrokeBbox;

    return true;
} // RotoStrokeItem::getMostRecentStrokeChangesSinceAge

void
RotoStrokeItem::clone(const RotoItem* other)
{
    const RotoStrokeItem* otherStroke = dynamic_cast<const RotoStrokeItem*>(other);

    assert(otherStroke);
    if (!otherStroke) {
        throw std::logic_error("RotoStrokeItem::clone");
    }
    {
        QMutexLocker k(&itemMutex);
        _imp->strokes.clear();
        for (std::vector<RotoStrokeItemPrivate::StrokeCurves>::const_iterator it = otherStroke->_imp->strokes.begin();
             it != otherStroke->_imp->strokes.end(); ++it) {
            RotoStrokeItemPrivate::StrokeCurves s;
            s.xCurve.reset(new Curve);
            s.yCurve.reset(new Curve);
            s.pressureCurve.reset(new Curve);
            s.xCurve->clone( *(it->xCurve) );
            s.yCurve->clone( *(it->yCurve) );
            s.pressureCurve->clone( *(it->pressureCurve) );
            _imp->strokes.push_back(s);
        }
        _imp->type = otherStroke->_imp->type;
        _imp->finished = true;
    }
    RotoDrawableItem::clone(other);
    invalidateHashCache();
    resetNodesThreadSafety();
}

void
RotoStrokeItem::toSerialization(SERIALIZATION_NAMESPACE::SerializationObjectBase* obj)
{
    SERIALIZATION_NAMESPACE::RotoStrokeItemSerialization* s = dynamic_cast<SERIALIZATION_NAMESPACE::RotoStrokeItemSerialization*>(obj);
    if (!s) {
        return;
    }

    RotoDrawableItem::toSerialization(obj);

    {
        QMutexLocker k(&itemMutex);
        switch (_imp->type) {
            case eRotoStrokeTypeBlur:
                s->_brushType = kRotoStrokeItemSerializationBrushTypeBlur;
                break;
            case eRotoStrokeTypeSmear:
                s->_brushType = kRotoStrokeItemSerializationBrushTypeSmear;
                break;
            case eRotoStrokeTypeSolid:
                s->_brushType = kRotoStrokeItemSerializationBrushTypeSolid;
                break;
            case eRotoStrokeTypeClone:
                s->_brushType = kRotoStrokeItemSerializationBrushTypeClone;
                break;
            case eRotoStrokeTypeReveal:
                s->_brushType = kRotoStrokeItemSerializationBrushTypeReveal;
                break;
            case eRotoStrokeTypeDodge:
                s->_brushType = kRotoStrokeItemSerializationBrushTypeDodge;
                break;
            case eRotoStrokeTypeBurn:
                s->_brushType = kRotoStrokeItemSerializationBrushTypeBurn;
                break;
            case eRotoStrokeTypeEraser:
                s->_brushType = kRotoStrokeItemSerializationBrushTypeEraser;
                break;
            default:
                break;
        }
        for (std::vector<RotoStrokeItemPrivate::StrokeCurves>::const_iterator it = _imp->strokes.begin();
             it != _imp->strokes.end(); ++it) {
            SERIALIZATION_NAMESPACE::RotoStrokeItemSerialization::PointCurves p;
            p.x.reset(new SERIALIZATION_NAMESPACE::CurveSerialization);
            p.y.reset(new SERIALIZATION_NAMESPACE::CurveSerialization);
            p.pressure.reset(new SERIALIZATION_NAMESPACE::CurveSerialization);
            it->xCurve->toSerialization(p.x.get());
            it->yCurve->toSerialization(p.y.get());
            it->pressureCurve->toSerialization(p.pressure.get());
            s->_subStrokes.push_back(p);
        }
    }
}


RotoStrokeType
RotoStrokeItem::strokeTypeFromSerializationString(const std::string& s)
{
    if (s == kRotoStrokeItemSerializationBrushTypeBlur) {
        return eRotoStrokeTypeBlur;
    } else if (s == kRotoStrokeItemSerializationBrushTypeSmear) {
        return eRotoStrokeTypeSmear;
    } else if (s == kRotoStrokeItemSerializationBrushTypeSolid) {
        return eRotoStrokeTypeSolid;
    } else if (s == kRotoStrokeItemSerializationBrushTypeClone) {
        return eRotoStrokeTypeClone;
    } else if (s == kRotoStrokeItemSerializationBrushTypeReveal) {
        return eRotoStrokeTypeReveal;
    } else if (s == kRotoStrokeItemSerializationBrushTypeDodge) {
        return eRotoStrokeTypeDodge;
    } else if (s == kRotoStrokeItemSerializationBrushTypeBurn) {
        return eRotoStrokeTypeBurn;
    } else if (s == kRotoStrokeItemSerializationBrushTypeEraser) {
        return eRotoStrokeTypeEraser;
    } else {
        throw std::runtime_error("Unknown brush type: " + s);
    }
}

void
RotoStrokeItem::fromSerialization(const SERIALIZATION_NAMESPACE::SerializationObjectBase & obj)
{
    const SERIALIZATION_NAMESPACE::RotoStrokeItemSerialization* s = dynamic_cast<const SERIALIZATION_NAMESPACE::RotoStrokeItemSerialization*>(&obj);
    if (!s) {
        return;
    }
    RotoDrawableItem::fromSerialization(obj);
    {
        QMutexLocker k(&itemMutex);
        _imp->type = strokeTypeFromSerializationString(s->_brushType);
        for (std::list<SERIALIZATION_NAMESPACE::RotoStrokeItemSerialization::PointCurves>::const_iterator it = s->_subStrokes.begin(); it!=s->_subStrokes.end(); ++it) {
            RotoStrokeItemPrivate::StrokeCurves stroke;
            stroke.xCurve.reset(new Curve);
            stroke.yCurve.reset(new Curve);
            stroke.pressureCurve.reset(new Curve);
            stroke.xCurve->fromSerialization(*it->x);
            stroke.yCurve->fromSerialization(*it->y);
            stroke.pressureCurve->fromSerialization(*it->pressure);
            _imp->strokes.push_back(stroke);
        }
    }


    setStrokeFinished();
}

RectD
RotoStrokeItem::computeBoundingBoxInternal(double time) const
{
    RectD bbox;
    Transform::Matrix3x3 transform;

    getTransformAtTime(time, &transform);
    bool pressureAffectsSize = getPressureSizeKnob()->getValueAtTime(time);
    bool bboxSet = false;
    double halfBrushSize = getBrushSizeKnob()->getValueAtTime(time) / 2. + 1;
    halfBrushSize = std::max(0.5, halfBrushSize);

    for (std::vector<RotoStrokeItemPrivate::StrokeCurves>::const_iterator it = _imp->strokes.begin(); it != _imp->strokes.end(); ++it) {
        KeyFrameSet xCurve = it->xCurve->getKeyFrames_mt_safe();
        KeyFrameSet yCurve = it->yCurve->getKeyFrames_mt_safe();
        KeyFrameSet pCurve = it->pressureCurve->getKeyFrames_mt_safe();

        RectD curveBox;
        bool curveBoxSet = false;

        if ( xCurve.empty() ) {
            continue;
        }

        assert( xCurve.size() == yCurve.size() && xCurve.size() == pCurve.size() );

        KeyFrameSet::const_iterator xIt = xCurve.begin();
        KeyFrameSet::const_iterator yIt = yCurve.begin();
        KeyFrameSet::const_iterator pIt = pCurve.begin();
        KeyFrameSet::const_iterator xNext = xIt;
        KeyFrameSet::const_iterator yNext = yIt;
        KeyFrameSet::const_iterator pNext = pIt;
        ++xNext;
        ++yNext;
        ++pNext;

        if (xCurve.size() == 1) {
            Transform::Point3D p;
            p.x = xIt->getValue();
            p.y = yIt->getValue();
            p.z = 1.;
            p = Transform::matApply(transform, p);
            double pressure = pressureAffectsSize ? pIt->getValue() : 1.;
            curveBox.x1 = p.x;
            curveBox.x2 = p.x;
            curveBox.y1 = p.y;
            curveBox.y2 = p.y;
            curveBox.x1 -= halfBrushSize * pressure;
            curveBox.x2 += halfBrushSize * pressure;
            curveBox.y1 -= halfBrushSize * pressure;
            curveBox.y2 += halfBrushSize * pressure;
            curveBoxSet = true;
        }


        for (; xNext != xCurve.end(); ++xIt, ++yIt, ++pIt, ++xNext, ++yNext, ++pNext) {


            double dt = xNext->getTime() - xIt->getTime();
            double pressure = pressureAffectsSize ? std::max( pIt->getValue(), pNext->getValue() ) : 1.;
            Transform::Point3D p0, p1, p2, p3;
            p0.z = p1.z = p2.z = p3.z = 1;
            p0.x = xIt->getValue();
            p0.y = yIt->getValue();
            p1.x = p0.x + dt * xIt->getRightDerivative() / 3.;
            p1.y = p0.y + dt * yIt->getRightDerivative() / 3.;
            p3.x = xNext->getValue();
            p3.y = yNext->getValue();
            p2.x = p3.x - dt * xNext->getLeftDerivative() / 3.;
            p2.y = p3.y - dt * yNext->getLeftDerivative() / 3.;


            p0 = Transform::matApply(transform, p0);
            p1 = Transform::matApply(transform, p1);
            p2 = Transform::matApply(transform, p2);
            p3 = Transform::matApply(transform, p3);

            Point p0_, p1_, p2_, p3_;
            p0_.x = p0.x; p0_.y = p0.y;
            p1_.x = p1.x; p1_.y = p1.y;
            p2_.x = p2.x; p2_.y = p2.y;
            p3_.x = p3.x; p3_.y = p3.y;

            RectD pointBox;
            bool pointBoxSet = false;
            Bezier::bezierPointBboxUpdate(p0_, p1_, p2_, p3_, &pointBox, &pointBoxSet);
            pointBox.x1 -= halfBrushSize * pressure;
            pointBox.x2 += halfBrushSize * pressure;
            pointBox.y1 -= halfBrushSize * pressure;
            pointBox.y2 += halfBrushSize * pressure;

            if (!curveBoxSet) {
                curveBoxSet = true;
                curveBox = pointBox;
            } else {
                curveBox.merge(pointBox);
            }

        }

        assert(curveBoxSet);
        if (!bboxSet) {
            bboxSet = true;
            bbox = curveBox;
        } else {
            bbox.merge(curveBox);
        }

    }

    return bbox;
} // RotoStrokeItem::computeBoundingBoxInternal

RectD
RotoStrokeItem::computeBoundingBox(double time) const
{
    QMutexLocker k(&itemMutex);

    return computeBoundingBoxInternal(time);
}

RectD
RotoStrokeItem::getBoundingBox(double time) const
{
    bool isActivated = getActivatedKnob()->getValueAtTime(time);

    if (!isActivated) {
        return RectD();
    }

    return computeBoundingBox(time);
}

std::list<CurvePtr >
RotoStrokeItem::getXControlPoints() const
{
    assert( QThread::currentThread() == qApp->thread() );
    std::list<CurvePtr > ret;
    QMutexLocker k(&itemMutex);
    for (std::vector<RotoStrokeItemPrivate::StrokeCurves>::const_iterator it = _imp->strokes.begin(); it != _imp->strokes.end(); ++it) {
        ret.push_back(it->xCurve);
    }

    return ret;
}

std::list<CurvePtr >
RotoStrokeItem::getYControlPoints() const
{
    assert( QThread::currentThread() == qApp->thread() );
    std::list<CurvePtr > ret;
    QMutexLocker k(&itemMutex);
    for (std::vector<RotoStrokeItemPrivate::StrokeCurves>::const_iterator it = _imp->strokes.begin(); it != _imp->strokes.end(); ++it) {
        ret.push_back(it->xCurve);
    }

    return ret;
}

void
RotoStrokeItem::evaluateStroke(unsigned int mipMapLevel,
                               double time,
                               std::list<std::list<std::pair<Point, double> > >* strokes,
                               RectD* bbox) const
{
    double brushSize = getBrushSizeKnob()->getValueAtTime(time) / 2.;
    bool pressureAffectsSize = getPressureSizeKnob()->getValueAtTime(time);
    Transform::Matrix3x3 transform;

    getTransformAtTime(time, &transform);

    bool bboxSet = false;
    for (std::vector<RotoStrokeItemPrivate::StrokeCurves>::const_iterator it = _imp->strokes.begin(); it != _imp->strokes.end(); ++it) {
        KeyFrameSet xSet, ySet, pSet;
        {
            QMutexLocker k(&itemMutex);
            xSet = it->xCurve->getKeyFrames_mt_safe();
            ySet = it->yCurve->getKeyFrames_mt_safe();
            pSet = it->pressureCurve->getKeyFrames_mt_safe();
        }
        assert( xSet.size() == ySet.size() && xSet.size() == pSet.size() );

        std::list<std::pair<Point, double> > points;
        RectD strokeBbox;

        evaluateStrokeInternal(xSet, ySet, pSet, transform, mipMapLevel, brushSize, pressureAffectsSize, &points, &strokeBbox);
        if (bbox) {
            if (bboxSet) {
                bbox->merge(strokeBbox);
            } else {
                *bbox = strokeBbox;
                bboxSet = true;
            }
        }
        if (!points.empty()) {
            strokes->push_back(points);
        }
    }
}

void
RotoStrokeItem::appendToHash(double time, ViewIdx view, Hash64* hash)
{
    {
        // Append the item knobs
        QMutexLocker k(&itemMutex);
        for (std::vector<RotoStrokeItemPrivate::StrokeCurves>::const_iterator it = _imp->strokes.begin(); it != _imp->strokes.end(); ++it) {
            Hash64::appendCurve(it->xCurve, hash);
            Hash64::appendCurve(it->yCurve, hash);

            // We don't add the pressure curve if there is more than 1 point, because it's extremely unlikely that the user draws twice the same curve with different pressure
            if (it->pressureCurve->getKeyFramesCount() == 1) {
                Hash64::appendCurve(it->pressureCurve, hash);}
            
        }
    }
    

    RotoDrawableItem::appendToHash(time, view, hash);
}

void
RotoStrokeItem::invalidateHashCache()
{
    RotoDrawableItem::invalidateHashCache();
}


NATRON_NAMESPACE_EXIT;

