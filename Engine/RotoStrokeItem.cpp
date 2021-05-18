/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2021 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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
#include <boost/shared_ptr.hpp>
#include <boost/math/special_functions/fpclassify.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON


#include "Engine/AppInstance.h"
#include "Engine/Bezier.h"
#include "Engine/BezierCP.h"
#include "Engine/CoonsRegularization.h"
#include "Engine/FeatherPoint.h"
#include "Engine/Format.h"
#include "Engine/Hash64.h"
#include "Engine/Image.h"
#include "Engine/ImageParams.h"
#include "Engine/Interpolation.h"
#include "Engine/RenderStats.h"
#include "Engine/RotoContextPrivate.h"
#include "Engine/RotoDrawableItemSerialization.h"
#include "Engine/RotoItemSerialization.h"
#include "Engine/RotoPoint.h"
#include "Engine/RotoStrokeItemSerialization.h"
#include "Engine/Settings.h"
#include "Engine/TimeLine.h"
#include "Engine/Transform.h"
#include "Engine/ViewerInstance.h"

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

//This will enable correct evaluation of beziers
//#define ROTO_USE_MESH_PATTERN_ONLY

// The number of pressure levels is 256 on an old Wacom Graphire 4, and 512 on an entry-level Wacom Bamboo
// 512 should be OK, see:
// http://www.davidrevoy.com/article182/calibrating-wacom-stylus-pressure-on-krita
#define ROTO_PRESSURE_LEVELS 512

#ifndef M_PI
#define M_PI        3.14159265358979323846264338327950288   /* pi             */
#endif

NATRON_NAMESPACE_ENTER

////////////////////////////////////Stroke//////////////////////////////////

RotoStrokeItem::RotoStrokeItem(RotoStrokeType type,
                               const RotoContextPtr& context,
                               const std::string & name,
                               const RotoLayerPtr& parent)

    : RotoDrawableItem(context, name, parent, true)
    , _imp( new RotoStrokeItemPrivate(type) )
{
}

RotoStrokeItem::~RotoStrokeItem()
{
    for (std::size_t i = 0; i < _imp->strokeDotPatterns.size(); ++i) {
        if (_imp->strokeDotPatterns[i]) {
            cairo_pattern_destroy(_imp->strokeDotPatterns[i]);
            _imp->strokeDotPatterns[i] = 0;
        }
    }
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
    if (bbox) {
        bbox->setupInfinity();
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
            double padding = std::max(0.5, halfBrushSize) * pressure;
            bbox->x1 -= padding;
            bbox->x2 += padding;
            bbox->y1 -= padding;
            bbox->y2 += padding;
        }

        return;
    }

    double pressure = 0;
    for (;
         xNext != xCurve.end() && yNext != yCurve.end() && pNext != pCurve.end();
         ++xIt, ++yIt, ++pIt, ++xNext, ++yNext, ++pNext) {
        assert( xIt != xCurve.end() && yIt != yCurve.end() && pIt != pCurve.end() );

        double x1 = xIt->getValue();
        double y1 = yIt->getValue();
        double z1 = 1.;
        double press1 = pIt->getValue();
        double x2 = xNext->getValue();
        double y2 = yNext->getValue();
        double z2 = 1;
        double press2 = pNext->getValue();
        double dt = ( xNext->getTime() - xIt->getTime() );
        double x1pr = x1 + dt * xIt->getRightDerivative() / 3.;
        double y1pr = y1 + dt * yIt->getRightDerivative() / 3.;
        double z1pr = 1.;
        double press1pr = press1 + dt * pIt->getRightDerivative() / 3.;
        double x2pl = x2 - dt * xNext->getLeftDerivative() / 3.;
        double y2pl = y2 - dt * yNext->getLeftDerivative() / 3.;
        double z2pl = 1;
        double press2pl = press2 - dt * pNext->getLeftDerivative() / 3.;
        Transform::matApply(transform, &x1, &y1, &z1);
        Transform::matApply(transform, &x1pr, &y1pr, &z1pr);
        Transform::matApply(transform, &x2pl, &y2pl, &z2pl);
        Transform::matApply(transform, &x2, &y2, &z2);

        /*
         * Approximate the necessary number of line segments, using http://antigrain.com/research/adaptive_bezier/
         */
        double dx1, dy1, dx2, dy2, dx3, dy3;
        dx1 = x1pr - x1;
        dy1 = y1pr - y1;
        dx2 = x2pl - x1pr;
        dy2 = y2pl - y1pr;
        dx3 = x2 - x2pl;
        dy3 = y2 - y2pl;
        double length = std::sqrt(dx1 * dx1 + dy1 * dy1) +
                        std::sqrt(dx2 * dx2 + dy2 * dy2) +
                        std::sqrt(dx3 * dx3 + dy3 * dy3);
        double nbPointsPerSegment = (int)std::max(length * 0.25, 2.);
        double incr = 1. / (double)(nbPointsPerSegment - 1);

        pressure = std::max(pressure, pressureAffectsSize ? std::max(press1, press2) : 1.);


        for (int i = 0; i < nbPointsPerSegment; ++i) {
            double t = incr * i;
            Point p;
            p.x = Bezier::bezierEval(x1, x1pr, x2pl, x2, t);
            p.y = Bezier::bezierEval(y1, y1pr, y2pl, y2, t);

            if (bbox) {
                bbox->x1 = std::min(p.x, bbox->x1);
                bbox->x2 = std::max(p.x, bbox->x2);
                bbox->y1 = std::min(p.y, bbox->y1);
                bbox->y2 = std::max(p.y, bbox->y2);
            }

            double pi = Bezier::bezierEval(press1, press1pr, press2pl, press2, t);
            p.x /= pot;
            p.y /= pot;
            points->push_back( std::make_pair(p, pi) );
        }
    } // for (; xNext != xCurve.end() ;++xNext, ++yNext, ++pNext) {
    if (bbox) {
        double padding = std::max(0.5, halfBrushSize) * pressure;
        bbox->x1 -= padding;
        bbox->x2 += padding;
        bbox->y1 -= padding;
        bbox->y2 += padding;
    }
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

        for (std::size_t i = 0; i < _imp->strokeDotPatterns.size(); ++i) {
            if (_imp->strokeDotPatterns[i]) {
                cairo_pattern_destroy(_imp->strokeDotPatterns[i]);
                _imp->strokeDotPatterns[i] = 0;
            }
        }
        _imp->strokeDotPatterns.clear();
    }

    resetTransformCenter();

    NodePtr effectNode = getEffectNode();
    NodePtr mergeNode = getMergeNode();
    NodePtr timeOffsetNode = getTimeOffsetNode();
    NodePtr frameHoldNode = getFrameHoldNode();

    if (effectNode) {
        effectNode->setWhileCreatingPaintStroke(false);
        effectNode->incrementKnobsAge();
    }
    mergeNode->setWhileCreatingPaintStroke(false);
    mergeNode->incrementKnobsAge();
    if (timeOffsetNode) {
        timeOffsetNode->setWhileCreatingPaintStroke(false);
        timeOffsetNode->incrementKnobsAge();
    }
    if (frameHoldNode) {
        frameHoldNode->setWhileCreatingPaintStroke(false);
        frameHoldNode->incrementKnobsAge();
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


    RotoStrokeItemPtr thisShared = boost::dynamic_pointer_cast<RotoStrokeItem>( shared_from_this() );
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
            s.xCurve = boost::make_shared<Curve>();
            s.yCurve = boost::make_shared<Curve>();
            s.pressureCurve = boost::make_shared<Curve>();
            _imp->strokes.push_back(s);
        }
        stroke = &_imp->strokes.back();
        assert(stroke);
        if (!stroke || !stroke->xCurve || !stroke->yCurve) {
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
        stroke->xCurve->setKeyFrameInterpolation(eKeyframeTypeCatmullRom, ki);
        stroke->yCurve->setKeyFrameInterpolation(eKeyframeTypeCatmullRom, ki);
        stroke->pressureCurve->setKeyFrameInterpolation(eKeyframeTypeCatmullRom, ki);
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
    incrementNodesAge();
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

    incrementNodesAge();

    return empty;
}

std::vector<cairo_pattern_t*>
RotoStrokeItem::getPatternCache() const
{
    assert( !_imp->strokeDotPatternsMutex.tryLock() );

    return _imp->strokeDotPatterns;
}

void
RotoStrokeItem::updatePatternCache(const std::vector<cairo_pattern_t*>& cache)
{
    assert( !_imp->strokeDotPatternsMutex.tryLock() );
    _imp->strokeDotPatterns = cache;
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
        //We rendered completely that stroke so far, pick the next one if there is
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
    if (!stroke || !stroke->xCurve || !stroke->yCurve) {
        return false;
    }
    assert( stroke && stroke->xCurve->getKeyFramesCount() == stroke->yCurve->getKeyFramesCount() && stroke->xCurve->getKeyFramesCount() == stroke->pressureCurve->getKeyFramesCount() );

    //We changed stroke index so far, reset the age
    if ( (*strokeIndex != lastMultiStrokeIndex) && (lastAge != -1) ) {
        lastAge = -1;
        *wholeStrokeBbox = computeBoundingBoxInternal(time);
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

    if ( lastAge != (int)(xCurve.size() - 1) ) {
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
            s.xCurve = boost::make_shared<Curve>();
            s.yCurve = boost::make_shared<Curve>();
            s.pressureCurve = boost::make_shared<Curve>();
            s.xCurve->clone( *(it->xCurve) );
            s.yCurve->clone( *(it->yCurve) );
            s.pressureCurve->clone( *(it->pressureCurve) );
            _imp->strokes.push_back(s);
        }
        _imp->type = otherStroke->_imp->type;
        _imp->finished = true;
    }
    RotoDrawableItem::clone(other);
    incrementNodesAge();
    resetNodesThreadSafety();
}

void
RotoStrokeItem::save(RotoItemSerialization* obj) const
{
    RotoDrawableItem::save(obj);
    RotoStrokeItemSerialization* s = dynamic_cast<RotoStrokeItemSerialization*>(obj);

    assert(s);
    if (!s) {
        throw std::logic_error("RotoStrokeItem::save");
    }
    {
        QMutexLocker k(&itemMutex);
        s->_brushType = (int)_imp->type;
        for (std::vector<RotoStrokeItemPrivate::StrokeCurves>::const_iterator it = _imp->strokes.begin();
             it != _imp->strokes.end(); ++it) {
            CurvePtr xCurve = boost::make_shared<Curve>();
            CurvePtr yCurve = boost::make_shared<Curve>();
            CurvePtr pressureCurve = boost::make_shared<Curve>();
            xCurve->clone( *(it->xCurve) );
            yCurve->clone( *(it->yCurve) );
            pressureCurve->clone( *(it->pressureCurve) );
            s->_xCurves.push_back(xCurve);
            s->_yCurves.push_back(yCurve);
            s->_pressureCurves.push_back(pressureCurve);
        }
    }
}

void
RotoStrokeItem::load(const RotoItemSerialization & obj)
{
    RotoDrawableItem::load(obj);
    const RotoStrokeItemSerialization* s = dynamic_cast<const RotoStrokeItemSerialization*>(&obj);

    assert(s);
    if (!s) {
        throw std::logic_error("RotoStrokeItem::load");
    }
    {
        QMutexLocker k(&itemMutex);
        _imp->type = (RotoStrokeType)s->_brushType;

        assert( s->_xCurves.size() == s->_yCurves.size() && s->_xCurves.size() == s->_pressureCurves.size() );
        std::list<CurvePtr>::const_iterator itY = s->_yCurves.begin();
        std::list<CurvePtr>::const_iterator itP = s->_pressureCurves.begin();
        for (std::list<CurvePtr>::const_iterator it = s->_xCurves.begin();
             it != s->_xCurves.end(); ++it, ++itY, ++itP) {
            RotoStrokeItemPrivate::StrokeCurves s;
            s.xCurve = boost::make_shared<Curve>();
            s.yCurve = boost::make_shared<Curve>();
            s.pressureCurve = boost::make_shared<Curve>();
            s.xCurve->clone( **(it) );
            s.yCurve->clone( **(itY) );
            s.pressureCurve->clone( **(itP) );
            _imp->strokes.push_back(s);
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

    for (std::vector<RotoStrokeItemPrivate::StrokeCurves>::const_iterator it = _imp->strokes.begin(); it != _imp->strokes.end(); ++it) {
        KeyFrameSet xCurve = it->xCurve->getKeyFrames_mt_safe();
        KeyFrameSet yCurve = it->yCurve->getKeyFrames_mt_safe();
        KeyFrameSet pCurve = it->pressureCurve->getKeyFrames_mt_safe();

        if ( xCurve.empty() ) {
            return RectD();
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
            RectD subBox;
            subBox.x1 = p.x;
            subBox.x2 = p.x;
            subBox.y1 = p.y;
            subBox.y2 = p.y;
            subBox.x1 -= halfBrushSize * pressure;
            subBox.x2 += halfBrushSize * pressure;
            subBox.y1 -= halfBrushSize * pressure;
            subBox.y2 += halfBrushSize * pressure;
            if (!bboxSet) {
                bboxSet = true;
                bbox = subBox;
            } else {
                bbox.merge(subBox);
            }
        }

        for (; xNext != xCurve.end(); ++xIt, ++yIt, ++pIt, ++xNext, ++yNext, ++pNext) {
            RectD subBox;
            subBox.setupInfinity();

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

            Bezier::bezierPointBboxUpdate(p0_, p1_, p2_, p3_, &subBox);
            subBox.x1 -= halfBrushSize * pressure;
            subBox.x2 += halfBrushSize * pressure;
            subBox.y1 -= halfBrushSize * pressure;
            subBox.y2 += halfBrushSize * pressure;

            if (!bboxSet) {
                bboxSet = true;
                bbox = subBox;
            } else {
                bbox.merge(subBox);
            }
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

std::list<CurvePtr>
RotoStrokeItem::getXControlPoints() const
{
    assert( QThread::currentThread() == qApp->thread() );
    std::list<CurvePtr> ret;
    QMutexLocker k(&itemMutex);
    for (std::vector<RotoStrokeItemPrivate::StrokeCurves>::const_iterator it = _imp->strokes.begin(); it != _imp->strokes.end(); ++it) {
        ret.push_back(it->xCurve);
    }

    return ret;
}

std::list<CurvePtr>
RotoStrokeItem::getYControlPoints() const
{
    assert( QThread::currentThread() == qApp->thread() );
    std::list<CurvePtr> ret;
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
        strokes->push_back(points);
    }
}

NATRON_NAMESPACE_EXIT

