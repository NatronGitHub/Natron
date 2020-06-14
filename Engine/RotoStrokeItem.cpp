/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2020 The Natron developers
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
#include <QThread>

GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
// /usr/local/include/boost/bind/arg.hpp:37:9: warning: unused typedef 'boost_static_assert_typedef_37' [-Wunused-local-typedef]
#include <boost/bind.hpp>
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
#include "Engine/Node.h"
#include "Engine/KnobTypes.h"
#include "Engine/Hash64.h"
#include "Engine/Interpolation.h"
#include "Engine/Project.h"
#include "Engine/RenderStats.h"
#include "Engine/RotoShapeRenderCairo.h"
#include "Engine/RotoPaint.h"
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

NATRON_NAMESPACE_ENTER

struct RotoStrokeItemPrivate
{
    RotoStrokeItem* _publicInterface;


    mutable QMutex lock;

    // brush type
    RotoStrokeType type;

    // When true, the render will be optimized to render only new points that were added to the curve
    // and will re-use the same image.
    bool isCurrentlyDrawing;

    // A stroke is composed of 3 interpolation curves
    struct StrokeCurves
    {
        CurvePtr xCurve, yCurve, pressureCurve;
    };

    // A list of all storkes contained in this item. Basically each time penUp() is called it makes a new stroke
    std::vector<StrokeCurves> strokes;


    // timestamp of the first point in curve
    double curveT0;

    // timestamp of the last point added in the curve
    double lastTimestamp;

    // Used when drawing by the algorithm in RotoShapeRenderNodePrivate::renderStroke_generic
    // to know where we stopped along the path at a given draw step.
    mutable double distToNextOut;
    Point lastCenter;

    // For a render clone, the bbox is computed only once per time
    boost::scoped_ptr<std::map<TimeValue,RectD> > renderCachedBbox;

    // Similarly, the hash is computed from the main instance curves since the clone only
    // contains the portion that was not rendered yet
    boost::scoped_ptr<std::map<TimeValue,U64> > renderCachedHash;

    // While drawing the stroke, this is the bounding box of the points
    // used to render. Basically this is the bbox of the points extracted
    // in the copy ctor.
    mutable RectD lastStrokeStepBbox;

    // Used only when drawing: Index in the xCurve, yCurve, pressureCurve of the last point (included)
    // that was drawn by a previous draw step.
    // This is used when creating a render clone for this stroke in the copy ctor to only
    // copy the points that are left to render.
    // Mutable since it is updated from the copy ctor of the render clone
    mutable int lastPointIndexInSubStroke, pickupPointIndexInSubStroke;

    // The index of the sub-stroke (in the strokes vector) for which the lastPointIndexInSubStroke corresponds to.
    // Mutable since it is updated from the copy ctor of the render clone
    mutable int currentSubStroke;

    // For cairo back-end, we cache the cairo_pattern used to render the stroke dots. This cache
    // is only valid throughout a single render or during the drawing action.
    mutable QMutex strokeDotPatternsMutex;
    std::vector<cairo_pattern_t*> strokeDotPatterns;

    // The OpenGL context that is used when drawing. Basically we need to remember it because the temporary paint buffer
    // allocated on the Node might be an OpenGL texture associated with one of these contexts.
    OSGLContextWPtr drawingGlCpuContext,drawingGlGpuContext;

    KnobDoubleWPtr effectStrength;
    KnobBoolWPtr pressureOpacity, pressureSize, pressureHardness, buildUp;
    KnobDoubleWPtr cloneTranslate;
    KnobDoubleWPtr cloneRotate;
    KnobDoubleWPtr cloneScale;
    KnobBoolWPtr cloneScaleUniform;
    KnobDoubleWPtr cloneSkewX;
    KnobDoubleWPtr cloneSkewY;
    KnobChoiceWPtr cloneSkewOrder;
    KnobDoubleWPtr cloneCenter;
    KnobChoiceWPtr cloneFilter;
    KnobBoolWPtr cloneBlackOutside;

    RotoStrokeItemPrivate(RotoStrokeItem* publicInterface, RotoStrokeType type)
    : _publicInterface(publicInterface)
    , lock()
    , type(type)
    , isCurrentlyDrawing(false)
    , strokes()
    , curveT0(0)
    , lastTimestamp(0)
    , distToNextOut(0)
    , lastCenter()
    , renderCachedBbox()
    , renderCachedHash()
    , lastStrokeStepBbox()
    , lastPointIndexInSubStroke(-1)
    , pickupPointIndexInSubStroke(0)
    , currentSubStroke(0)
    , strokeDotPatternsMutex()
    , strokeDotPatterns()
    , drawingGlCpuContext()
    , drawingGlGpuContext()
    {

    }


    /**
     * @brief Copy from the other stroke the points that still need to be rendered in the stroke so far.
     **/
    bool copyStrokeForRendering(const RotoStrokeItemPrivate& other);

    RectD computeBoundingBox(TimeValue time, ViewIdx view) const;

    U64 computeHashFromStrokes() const;

};

bool
RotoStrokeItemPrivate::copyStrokeForRendering(const RotoStrokeItemPrivate& other)
{
    QMutexLocker k1(&lock);

    assert(!other.lock.tryLock());

    type = other.type;
    isCurrentlyDrawing = other.isCurrentlyDrawing;
    curveT0 = other.curveT0;
    lastTimestamp = other.lastTimestamp;
    distToNextOut = other.distToNextOut;
    lastCenter = other.lastCenter;
    lastStrokeStepBbox = other.lastStrokeStepBbox;
    lastPointIndexInSubStroke = other.lastPointIndexInSubStroke;
    currentSubStroke = other.currentSubStroke;
    strokeDotPatterns = other.strokeDotPatterns;
    drawingGlCpuContext = other.drawingGlCpuContext;
    drawingGlGpuContext = other.drawingGlGpuContext;


    assert(currentSubStroke >= 0);

    bool hasDoneSomething = false;

    // If the curve is in a drawing state, pick-up from where
    // the drawing was at.
    // Otherwise render all sub-strokes
    pickupPointIndexInSubStroke = (!isCurrentlyDrawing || lastPointIndexInSubStroke == -1) ? 0 : lastPointIndexInSubStroke + 1;
    int pickupStrokeIndex = isCurrentlyDrawing ? currentSubStroke : 0;

    for (int strokeIndex = pickupStrokeIndex; strokeIndex < (int)other.strokes.size(); ++strokeIndex) {

        const RotoStrokeItemPrivate::StrokeCurves* originalStroke = 0;
        originalStroke = &other.strokes[strokeIndex];
        assert(other.strokes[strokeIndex].xCurve &&
               other.strokes[strokeIndex].yCurve &&
               other.strokes[strokeIndex].pressureCurve);
        if (!originalStroke->xCurve ||
            !originalStroke->yCurve ||
            !originalStroke->pressureCurve) {
            return false;
        }

        // Curves should be consistant for a stroke
        assert( originalStroke->xCurve->getKeyFramesCount() == originalStroke->yCurve->getKeyFramesCount() && originalStroke->xCurve->getKeyFramesCount() == originalStroke->pressureCurve->getKeyFramesCount() );

        // Check if we reached the end of the sub-stroke
        bool hasDataToCopy = true;
        {
            int nKeys = originalStroke->xCurve->getKeyFramesCount();
            if (nKeys == 0 || (isCurrentlyDrawing && lastPointIndexInSubStroke != -1 && lastPointIndexInSubStroke >= nKeys - 1)) {
                hasDataToCopy = false;
            }
        }
        if (hasDataToCopy) {
            StrokeCurves strokeCopy;
            strokeCopy.xCurve.reset(new Curve);
            strokeCopy.yCurve.reset(new Curve);
            strokeCopy.pressureCurve.reset(new Curve);


            // Copy what we need from the sub-stroke.
            // we still need to add to the stroke the last point rendered before so that
            // the interval between the last draw step and the next step is drawn.
            if (pickupPointIndexInSubStroke <= 1) {
                strokeCopy.xCurve->clone(*(originalStroke->xCurve));
                strokeCopy.yCurve->clone(*(originalStroke->yCurve));
                strokeCopy.pressureCurve->clone(*(originalStroke->pressureCurve));
            } else {
                strokeCopy.xCurve->cloneIndexRange(*(originalStroke->xCurve), pickupPointIndexInSubStroke - 1);
                strokeCopy.yCurve->cloneIndexRange(*(originalStroke->yCurve), pickupPointIndexInSubStroke - 1);
                strokeCopy.pressureCurve->cloneIndexRange(*(originalStroke->pressureCurve), pickupPointIndexInSubStroke - 1);
            }
            // Store on the render clone the new index: it will be updated in updateStrokeData() on the "main instance" when the render is finished, so we are sure we rendered this stroke
            // so far
            lastPointIndexInSubStroke = originalStroke->xCurve->getKeyFramesCount() - 1;
            if (lastPointIndexInSubStroke >= pickupPointIndexInSubStroke) {
                hasDoneSomething = true;
            }

            strokes.push_back(strokeCopy);
        }

        // If the user previously drawn a stroke, there may be nothing to render in that stroke but a new stroke was made.
        // In this case, go on to the next stroke
        if (isCurrentlyDrawing && !hasDataToCopy && currentSubStroke < (int)other.strokes.size() - 1) {

            // Increment the stroke index
            other.currentSubStroke += 1;
            currentSubStroke = other.currentSubStroke;

            // Reset the last point index
            pickupPointIndexInSubStroke = 0;
            other.lastPointIndexInSubStroke = -1;
            lastPointIndexInSubStroke = -1;
        }

        // Only draw one sub-stroke by step
        if (isCurrentlyDrawing && hasDataToCopy) {
            break;
        }
    }


    other.pickupPointIndexInSubStroke = pickupPointIndexInSubStroke;

    // During painting, we copy on the clone only the points that were not rendered yet.
    // In order for the getBoundingBox function to return something decent, we still need
    // to know the bounding box of the full stroke
    if (isCurrentlyDrawing) {

#ifdef DEBUG
        if (lastPointIndexInSubStroke >= pickupPointIndexInSubStroke) {
            if (pickupPointIndexInSubStroke == 0) {
                assert(lastPointIndexInSubStroke + 1 == strokes.back().xCurve->getKeyFramesCount());
            } else {
                // +2 because we also add the last point of the previous draw step in the call to cloneIndexRange(), otherwise it would make holes in the drawing
                assert((lastPointIndexInSubStroke + 2 - pickupPointIndexInSubStroke) == strokes.back().xCurve->getKeyFramesCount());
            }
        }
#endif

        // The bounding box computation requires a time and view parameters:
        // we are OK to assume that the time view are the current ones in the UI
        // since we are drawing anyway nothing else is happening.
        TimeValue time = _publicInterface->getCurrentRenderTime();
        ViewIdx view = _publicInterface->getCurrentRenderView();


            // This is the bounding box of just the points we did not draw
        lastStrokeStepBbox = computeBoundingBox(time, view);


        // This is the bounding box of the full stroke. Since we only copied from the curve the portion
        // we did not draw yet, we can only compute it on the main instance
        renderCachedBbox.reset(new std::map<TimeValue,RectD>);
        RectD fullStrokeBbox = other.computeBoundingBox(time, view);
        renderCachedBbox->insert(std::make_pair(time, fullStrokeBbox));

        renderCachedHash.reset(new std::map<TimeValue,U64>);
        U64 strokesHash = other.computeHashFromStrokes();
        renderCachedHash->insert(std::make_pair(time, strokesHash));
    }
    return hasDoneSomething;
} // copyStrokeForRendering

int
RotoStrokeItem::getRenderCloneCurrentStrokeIndex() const
{
    assert(isRenderClone());
    QMutexLocker k(&_imp->lock);
    if (_imp->isCurrentlyDrawing) {
        return _imp->currentSubStroke;
    }
    return 0;
}

int
RotoStrokeItem::getRenderCloneCurrentStrokeEndPointIndex() const
{
    assert(isRenderClone());
    QMutexLocker k(&_imp->lock);
    return _imp->lastPointIndexInSubStroke;

}

int
RotoStrokeItem::getRenderCloneCurrentStrokeStartPointIndex() const
{
    QMutexLocker k(&_imp->lock);
    if (_imp->isCurrentlyDrawing) {
        return _imp->pickupPointIndexInSubStroke;
    }
    return 0;
}


RotoStrokeItem::RotoStrokeItem(RotoStrokeType type,
                               const KnobItemsTablePtr& model)

    : RotoDrawableItem(model)
    , _imp( new RotoStrokeItemPrivate(this, type) )
{
}

RotoStrokeItem::RotoStrokeItem(const RotoStrokeItemPtr& other, const FrameViewRenderKey& key)
: RotoDrawableItem(other, key)
, _imp(new RotoStrokeItemPrivate(this, other->getBrushType()))
{

}

RotoStrokeItem::~RotoStrokeItem()
{
#ifdef ROTO_SHAPE_RENDER_ENABLE_CAIRO
    RotoShapeRenderCairo::purgeCaches_cairo_internal(_imp->strokeDotPatterns);
#endif
    if (!isRenderClone()) {
        const NodesList& nodes = getItemNodes();
        for (NodesList::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
            (*it)->destroyNode();
        }
    }
}

KnobHolderPtr
RotoStrokeItem::createRenderCopy(const FrameViewRenderKey& key) const
{
    RotoStrokeItemPtr mainInstance = toRotoStrokeItem(getMainInstance());
    if (!mainInstance) {
        mainInstance = toRotoStrokeItem(boost::const_pointer_cast<KnobHolder>(shared_from_this()));
    }


    RotoStrokeItemPtr ret(new RotoStrokeItem(mainInstance, key));
    return ret;


}


void
RotoStrokeItem::updateStrokeData(const Point& lastCenter, double distNextOut, int lastSubStrokePointIndex)
{
    assert(!isRenderClone());
    QMutexLocker k(&_imp->lock);
    _imp->distToNextOut = distNextOut;
    _imp->lastCenter = lastCenter;
    _imp->lastPointIndexInSubStroke = lastSubStrokePointIndex;
}

void
RotoStrokeItem::getStrokeState(Point* lastCenterIn, double* distNextIn) const
{
    QMutexLocker k(&_imp->lock);
    if (!_imp->isCurrentlyDrawing) {
        *distNextIn = 0;
        lastCenterIn->x = lastCenterIn->y = INT_MIN;
    } else {
        *distNextIn = _imp->distToNextOut;
        *lastCenterIn = _imp->lastCenter;
    }
}

RotoStrokeType
RotoStrokeItem::getBrushType() const
{
    return _imp->type;
}

std::string
RotoStrokeItem::getBaseItemName() const
{
    std::string itemName;

    switch (_imp->type) {
        case eRotoStrokeTypeSolid:
            itemName = kRotoPaintBrushBaseName;
            break;
        case eRotoStrokeTypeEraser:
            itemName = kRotoPaintEraserBaseName;
            break;
        case eRotoStrokeTypeClone:
            itemName = kRotoPaintCloneBaseName;
            break;
        case eRotoStrokeTypeReveal:
            itemName = kRotoPaintRevealBaseName;
            break;
        case eRotoStrokeTypeBlur:
            itemName = kRotoPaintBlurBaseName;
            break;
        case eRotoStrokeTypeSharpen:
            itemName = kRotoPaintSharpenBaseName;
            break;
        case eRotoStrokeTypeSmear:
            itemName = kRotoPaintSmearBaseName;
            break;
        case eRotoStrokeTypeDodge:
            itemName = kRotoPaintDodgeBaseName;
            break;
        case eRotoStrokeTypeBurn:
            itemName = kRotoPaintBurnBaseName;
            break;
        default:
            break;
    }
    return itemName;

}

std::string
RotoStrokeItem::getSerializationClassName() const
{
    switch (_imp->type) {
        case eRotoStrokeTypeBlur:
            return kSerializationStrokeBrushTypeBlur;
        case eRotoStrokeTypeSmear:
            return kSerializationStrokeBrushTypeSmear;
        case eRotoStrokeTypeSolid:
            return kSerializationStrokeBrushTypeSolid;
        case eRotoStrokeTypeClone:
            return kSerializationStrokeBrushTypeClone;
        case eRotoStrokeTypeReveal:
            return kSerializationStrokeBrushTypeReveal;
        case eRotoStrokeTypeDodge:
            return kSerializationStrokeBrushTypeDodge;
        case eRotoStrokeTypeBurn:
            return kSerializationStrokeBrushTypeBurn;
        case eRotoStrokeTypeEraser:
            return kSerializationStrokeBrushTypeEraser;
        default:
            break;
    }
    return std::string();

}

static void
evaluateStrokeInternal(const KeyFrameSet& xCurve,
                       const KeyFrameSet& yCurve,
                       const KeyFrameSet& pCurve,
                       const Transform::Matrix3x3& transform,
                       const RenderScale& scale,
                       double halfBrushSize,
                       bool pressureAffectsSize,
                       std::list<std::pair<Point, double> >* points,
                       RectD* bbox)
{
    //Increment the half brush size so that the stroke is enclosed in the RoD
    halfBrushSize += 1.;
    halfBrushSize = std::max(0.5, halfBrushSize);

    bool bboxSet = false;


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

    if ( (xCurve.size() == 1) && ( xIt != xCurve.end() ) && ( yIt != yCurve.end() ) && ( pIt != pCurve.end() ) ) {
        assert( xNext == xCurve.end() && yNext == yCurve.end() && pNext == pCurve.end() );
        Transform::Point3D p;
        p.x = xIt->getValue();
        p.y = yIt->getValue();
        p.z = 1.;

        p = Transform::matApply(transform, p);

        Point pixelPoint;
        pixelPoint.x = p.x * scale.x;
        pixelPoint.y = p.y * scale.y;
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
        double pressp1 = pressp0 + dt * pIt->getRightDerivative() * (1. / 3);
        double pressp2 = pressp3 - dt * pNext->getLeftDerivative() * (1. / 3);

        pressure = std::max(pressure, pressureAffectsSize ? std::max(pressp0, pressp3) : 1.);

        Transform::Point3D p0, p1, p2, p3;
        p0.z = p1.z = p2.z = p3.z = 1;
        p0.x = xIt->getValue();
        p0.y = yIt->getValue();
        p1.x = p0.x + dt * xIt->getRightDerivative() * (1. / 3);
        p1.y = p0.y + dt * yIt->getRightDerivative() * (1. / 3);
        p3.x = xNext->getValue();
        p3.y = yNext->getValue();
        p2.x = p3.x - dt * xNext->getLeftDerivative() * (1. / 3);
        p2.y = p3.y - dt * yNext->getLeftDerivative() * (1. / 3);


        p0 = Transform::matApply(transform, p0);
        p1 = Transform::matApply(transform, p1);
        p2 = Transform::matApply(transform, p2);
        p3 = Transform::matApply(transform, p3);

        // If the end-points are linear for the segment, just add a straight line to speed up evaluation.
        double nbPointsPerSegment;
        if (xIt->getInterpolation() == eKeyframeTypeLinear && yIt->getInterpolation() == eKeyframeTypeLinear) {
            nbPointsPerSegment = 2;
        } else {
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
            nbPointsPerSegment = (int)std::max(length * 0.25, 2.);
        }
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
            p.x *= scale.x;
            p.y *= scale.y;
            points->push_back( std::make_pair(p, pi) );
        }

        if (bbox) {

            double padding = halfBrushSize * pressure;
            pointBox.x1 -= padding;
            pointBox.x2 += padding;
            pointBox.y1 -= padding;
            pointBox.y2 += padding;


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
    QMutexLocker k(&_imp->lock);

    return _imp->strokes.empty();
}


void
RotoStrokeItem::beginSubStroke()
{
    // Make-up a new sub-stroke
    {
        QMutexLocker k(&_imp->lock);
        RotoStrokeItemPrivate::StrokeCurves s;
        s.xCurve.reset(new Curve);
        s.yCurve.reset(new Curve);
        s.pressureCurve.reset(new Curve);

        _imp->strokes.push_back(s);

        // set thread-safety so that we ensure only 1 thread is rendering
        if (!_imp->isCurrentlyDrawing) {
            _imp->isCurrentlyDrawing = true;
            setNodesThreadSafetyForRotopainting();
        }

        // For cairo, setup the dot pattern cache
        if ( _imp->strokeDotPatterns.empty() ) {
            _imp->strokeDotPatterns.resize(ROTO_PRESSURE_LEVELS);
            for (std::size_t i = 0; i < _imp->strokeDotPatterns.size(); ++i) {
                _imp->strokeDotPatterns[i] = (cairo_pattern_t*)0;
            }
        }
    }

    // Only 1 stroke can be drawn at a time in the UI: make it global to the application instance.
    // The OutputSchedulerThread will use it to only launch a single thread and OpenGL context during the drawing of this stroke.
    RotoStrokeItemPtr thisShared = toRotoStrokeItem(shared_from_this());
    getApp()->setUserIsPainting(thisShared);

}

void
RotoStrokeItem::endSubStroke()
{
    {
        QMutexLocker k(&_imp->lock);
        if (_imp->strokes.empty()) {
            // This function must match a call to beginSubStroke
            assert(false);
            return;
        }
        {
            RotoStrokeItemPrivate::StrokeCurves* stroke = &_imp->strokes.back();

            assert( stroke->xCurve->getKeyFramesCount() == stroke->yCurve->getKeyFramesCount() &&
                   stroke->xCurve->getKeyFramesCount() == stroke->pressureCurve->getKeyFramesCount() );

            int nk = stroke->xCurve->getKeyFramesCount();

            // When ending the stroke, set the points interpolation to catmullrom the the stroke appears smooth even if the user was
            // too sharp with the pen.
            // For smear we don't do this as it changes too much the render in output.
            if (getBrushType() != eRotoStrokeTypeSmear) {
                for (int i = 0; i < nk; ++i) {
                    stroke->xCurve->setKeyFrameInterpolation(eKeyframeTypeCatmullRom, i);
                    stroke->yCurve->setKeyFrameInterpolation(eKeyframeTypeCatmullRom, i);
                    stroke->pressureCurve->setKeyFrameInterpolation(eKeyframeTypeCatmullRom, i);
                }
            }
            if (nk == 0) {
                // If the stroke did not have any point, remove it
                _imp->strokes.pop_back();
                return;
            }
        }

        // We are no longer in drawing state
        _imp->isCurrentlyDrawing = false;


        // Purge cairo cache
#ifdef ROTO_SHAPE_RENDER_ENABLE_CAIRO
        RotoShapeRenderCairo::purgeCaches_cairo_internal(_imp->strokeDotPatterns);
#endif
    }



    // The stroke is finished, reset the transform center
    resetTransformCenter();

    // Reset nodes thread-safety since we now allow multiple concurrent threads again
    resetNodesThreadSafety();

    // We no longer are drawing this stroke, flag it on the app
    getApp()->setUserIsPainting(RotoStrokeItemPtr());

    // Trigger a new render: Since we set the isCurrentlyDrawing flag to false, it will not use the same
    // buffer as it was using during painting hence a new correct image will be created and cached.
    invalidateCacheHashAndEvaluate(true, false);


} // endSubStroke

void
RotoStrokeItem::appendPoint(const RotoPoint& p)
{
    RotoStrokeItemPtr thisShared = toRotoStrokeItem( shared_from_this() );
    assert(thisShared);
    {
        QMutexLocker k(&_imp->lock);

        if (_imp->strokes.empty()) {
            // beginSubStroke was not called.
            assert(false);
            return;
        }
        RotoStrokeItemPrivate::StrokeCurves* stroke = &_imp->strokes.back();
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


        ValueChangedReturnCodeEnum addKeyFrameOk; // did we add a new keyframe (normally yes, but just in case)
        int ki; // index of the new keyframe (normally nk, but just in case)
        {
            KeyFrame k;
            k.setTime(TimeValue(t));
            k.setValue(p.pos().x);
            addKeyFrameOk = stroke->xCurve->setOrAddKeyframe(k);
            ki = ( addKeyFrameOk == eValueChangedReturnCodeKeyframeAdded ? nk : (nk - 1) );
        }
        {
            KeyFrame k;
            k.setTime(TimeValue(t));
            k.setValue(p.pos().y);
            ValueChangedReturnCodeEnum addedKey = stroke->yCurve->setOrAddKeyframe(k);
            assert(addedKey == eValueChangedReturnCodeKeyframeAdded);

        }

        {
            KeyFrame k;
            k.setTime(TimeValue(t));
            k.setValue( p.pressure() );
            ValueChangedReturnCodeEnum addedKey = stroke->pressureCurve->setOrAddKeyframe(k);
            assert(addedKey == eValueChangedReturnCodeKeyframeAdded);
        }
        // While editing a stroke, use linear tangents because we don't have much informations.
        // When the stroke ends, we set all points interpolation to catmull-rom
        KeyframeTypeEnum interpolation = eKeyframeTypeLinear;
        stroke->xCurve->setKeyFrameInterpolation(interpolation, ki);
        stroke->yCurve->setKeyFrameInterpolation(interpolation, ki);
        stroke->pressureCurve->setKeyFrameInterpolation(interpolation, ki);

    } // QMutexLocker k(&itemMutex);

    invalidateCacheHashAndEvaluate(true, false);
} // RotoStrokeItem::appendPoint

void
RotoStrokeItem::setStrokes(const std::list<std::list<RotoPoint> >& strokes)
{
    QMutexLocker k(&_imp->lock);
    _imp->strokes.clear();

    for (std::list<std::list<RotoPoint> >::const_iterator it = strokes.begin(); it != strokes.end(); ++it) {

        RotoStrokeItemPrivate::StrokeCurves s;
        s.xCurve.reset(new Curve);
        s.yCurve.reset(new Curve);
        s.pressureCurve.reset(new Curve);

        for (std::list<RotoPoint>::const_iterator it2 = it->begin(); it2 != it->end(); ++it2) {

            int nk = s.xCurve->getKeyFramesCount();
            ValueChangedReturnCodeEnum addKeyFrameOk; // did we add a new keyframe (normally yes, but just in case)
            int ki; // index of the new keyframe (normally nk, but just in case)
            {
                KeyFrame k;
                k.setTime(it2->timestamp());
                k.setValue(it2->pos().x);
                addKeyFrameOk = s.xCurve->setOrAddKeyframe(k);
                ki = ( addKeyFrameOk == eValueChangedReturnCodeKeyframeAdded ? nk : (nk - 1) );
            }
            {
                KeyFrame k;
                k.setTime(it2->timestamp());
                k.setValue(it2->pos().y);
                ValueChangedReturnCodeEnum aok = s.yCurve->setOrAddKeyframe(k);
                assert(aok == eValueChangedReturnCodeKeyframeAdded);

            }

            {
                KeyFrame k;
                k.setTime(it2->timestamp());
                k.setValue( it2->pressure());
                ValueChangedReturnCodeEnum aok = s.pressureCurve->setOrAddKeyframe(k);
                assert(aok == eValueChangedReturnCodeKeyframeAdded);

            }
            // Use CatmullRom interpolation, which means that the tangent may be modified by the next point on the curve.
            // In a previous version, the previous keyframe was set to Free so its tangents don't get overwritten, but this caused oscillations.
            KeyframeTypeEnum interpolation = _imp->type == eRotoStrokeTypeSmear ? eKeyframeTypeFree : eKeyframeTypeCatmullRom;
            s.xCurve->setKeyFrameInterpolation(interpolation, ki);
            s.yCurve->setKeyFrameInterpolation(interpolation, ki);
            s.pressureCurve->setKeyFrameInterpolation(interpolation, ki);
        }

        _imp->strokes.push_back(s);

    }

    invalidateCacheHashAndEvaluate(true, false);

} // setStrokes

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
        QMutexLocker k(&_imp->lock);
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
        QMutexLocker k(&_imp->lock);
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
    {
        QMutexLocker k(&_imp->lock);
        _imp->drawingGlGpuContext = gpuContext;
        _imp->drawingGlCpuContext = cpuContext;
    }
}


void
RotoStrokeItem::getDrawingGLContext(OSGLContextPtr* gpuContext, OSGLContextPtr* cpuContext) const
{
    QMutexLocker k(&_imp->lock);
    *gpuContext = _imp->drawingGlGpuContext.lock();
    *cpuContext = _imp->drawingGlCpuContext.lock();
}

RectD
RotoStrokeItem::getLastStrokeMovementBbox() const
{
    assert(isRenderClone());
    QMutexLocker k(&_imp->lock);
    return _imp->lastStrokeStepBbox;
}

bool
RotoStrokeItem::isCurrentlyDrawing() const
{
    QMutexLocker k(&_imp->lock);
    return _imp->isCurrentlyDrawing;
}

void
RotoStrokeItem::copyItem(const KnobTableItem& other)
{
    const RotoStrokeItem* otherStroke = dynamic_cast<const RotoStrokeItem*>(&other);

    assert(otherStroke);
    if (!otherStroke) {
        throw std::logic_error("RotoStrokeItem::clone");
    }
    {
        QMutexLocker k(&_imp->lock);
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
        _imp->isCurrentlyDrawing = false;
    }
    RotoDrawableItem::copyItem(other);
    resetNodesThreadSafety();
    invalidateHashCache();
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
        QMutexLocker k(&_imp->lock);
        for (std::vector<RotoStrokeItemPrivate::StrokeCurves>::const_iterator it = _imp->strokes.begin();
             it != _imp->strokes.end(); ++it) {
            SERIALIZATION_NAMESPACE::RotoStrokeItemSerialization::PointCurves p;
            p.x = boost::make_shared<SERIALIZATION_NAMESPACE::CurveSerialization>();
            p.y = boost::make_shared<SERIALIZATION_NAMESPACE::CurveSerialization>();
            p.pressure = boost::make_shared<SERIALIZATION_NAMESPACE::CurveSerialization>();
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
    if (s == kSerializationStrokeBrushTypeBlur) {
        return eRotoStrokeTypeBlur;
    } else if (s == kSerializationStrokeBrushTypeSmear) {
        return eRotoStrokeTypeSmear;
    } else if (s == kSerializationStrokeBrushTypeSolid) {
        return eRotoStrokeTypeSolid;
    } else if (s == kSerializationStrokeBrushTypeClone) {
        return eRotoStrokeTypeClone;
    } else if (s == kSerializationStrokeBrushTypeReveal) {
        return eRotoStrokeTypeReveal;
    } else if (s == kSerializationStrokeBrushTypeDodge) {
        return eRotoStrokeTypeDodge;
    } else if (s == kSerializationStrokeBrushTypeBurn) {
        return eRotoStrokeTypeBurn;
    } else if (s == kSerializationStrokeBrushTypeEraser) {
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
        QMutexLocker k(&_imp->lock);
        for (std::list<SERIALIZATION_NAMESPACE::RotoStrokeItemSerialization::PointCurves>::const_iterator it = s->_subStrokes.begin(); it!=s->_subStrokes.end(); ++it) {
            RotoStrokeItemPrivate::StrokeCurves stroke;
            stroke.xCurve = boost::make_shared<Curve>();
            stroke.yCurve = boost::make_shared<Curve>();
            stroke.pressureCurve = boost::make_shared<Curve>();
            stroke.xCurve->fromSerialization(*it->x);
            stroke.yCurve->fromSerialization(*it->y);
            stroke.pressureCurve->fromSerialization(*it->pressure);
            _imp->strokes.push_back(stroke);
        }
        _imp->isCurrentlyDrawing = false;
    }

}

RectD
RotoStrokeItemPrivate::computeBoundingBox(TimeValue time, ViewIdx view) const
{
    // Private - should not lock
    assert(!lock.tryLock());

    RectD bbox;
    Transform::Matrix3x3 transform;

    _publicInterface->getTransformAtTime(time, view, &transform);
    bool pressureAffectsSize = pressureSize.lock()->getValueAtTime(time);
    bool bboxSet = false;
    double halfBrushSize = _publicInterface->getBrushSizeKnob()->getValueAtTime(time) / 2. + 1;
    halfBrushSize = std::max(0.5, halfBrushSize);

    for (std::vector<RotoStrokeItemPrivate::StrokeCurves>::const_iterator it = strokes.begin(); it != strokes.end(); ++it) {
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
            p1.x = p0.x + dt * xIt->getRightDerivative() * (1. / 3);
            p1.y = p0.y + dt * yIt->getRightDerivative() * (1. / 3);
            p3.x = xNext->getValue();
            p3.y = yNext->getValue();
            p2.x = p3.x - dt * xNext->getLeftDerivative() * (1. / 3);
            p2.y = p3.y - dt * yNext->getLeftDerivative() * (1. / 3);


            p0 = Transform::matApply(transform, p0);
            p1 = Transform::matApply(transform, p1);
            p2 = Transform::matApply(transform, p2);
            p3 = Transform::matApply(transform, p3);

            Point p0_, p1_, p2_, p3_;
            p0_.x = p0.x; p0_.y = p0.y;
            p1_.x = p1.x; p1_.y = p1.y;
            p2_.x = p2.x; p2_.y = p2.y;
            p3_.x = p3.x; p3_.y = p3.y;

            RectD pointBox = Bezier::getBezierSegmentControlPolygonBbox(p0_, p1_, p2_, p3_);
            pointBox.addPadding(halfBrushSize * pressure + 1, halfBrushSize * pressure + 1);

            if (!curveBoxSet) {
                curveBoxSet = true;
                curveBox = pointBox;
            } else {
                curveBox.merge(pointBox);
            }

        } // for all points in stroke

        assert(curveBoxSet);
        if (!bboxSet) {
            bboxSet = true;
            bbox = curveBox;
        } else {
            bbox.merge(curveBox);
        }

    } // for all sub-strokes

    return bbox;
} // RotoStrokeItem::computeBoundingBox


RectD
RotoStrokeItem::getBoundingBox(TimeValue time, ViewIdx view) const
{
    bool enabled = isActivated(time, view);

    if (!enabled) {
        return RectD();
    }
    bool renderClone = isRenderClone();
    QMutexLocker k(&_imp->lock);
    if (renderClone) {
        if (_imp->renderCachedBbox) {
            std::map<TimeValue,RectD>::iterator foundBbox = _imp->renderCachedBbox->find(time);
            if (foundBbox != _imp->renderCachedBbox->end()) {
                return foundBbox->second;
            }
        }
    }
    RectD bbox;
    bbox = _imp->computeBoundingBox(time, view);

    if (renderClone) {
        if (!_imp->renderCachedBbox) {
            _imp->renderCachedBbox.reset(new std::map<TimeValue,RectD>);
            _imp->renderCachedBbox->insert(std::make_pair(time, bbox));
        }
    }
    return bbox;
}

std::list<CurvePtr>
RotoStrokeItem::getXControlPoints() const
{
    std::list<CurvePtr> ret;
    QMutexLocker k(&_imp->lock);
    for (std::vector<RotoStrokeItemPrivate::StrokeCurves>::const_iterator it = _imp->strokes.begin(); it != _imp->strokes.end(); ++it) {
        ret.push_back(it->xCurve);
    }

    return ret;
}

std::list<CurvePtr>
RotoStrokeItem::getYControlPoints() const
{
    std::list<CurvePtr> ret;
    QMutexLocker k(&_imp->lock);
    for (std::vector<RotoStrokeItemPrivate::StrokeCurves>::const_iterator it = _imp->strokes.begin(); it != _imp->strokes.end(); ++it) {
        ret.push_back(it->xCurve);
    }

    return ret;
}

int
RotoStrokeItem::getNumControlPoints(int strokeIndex) const
{
    QMutexLocker k(&_imp->lock);
    if (strokeIndex >= (int)_imp->strokes.size() || strokeIndex < 0) {
        return 0;
    }
    if (!_imp->strokes[strokeIndex].xCurve) {
        return 0;
    }
    return _imp->strokes[strokeIndex].xCurve->getKeyFramesCount();
}

void
RotoStrokeItem::evaluateStroke(const RenderScale& scale,
                               TimeValue time,
                               ViewIdx view,
                               std::list<std::list<std::pair<Point, double> > >* strokes) const
{
    double brushSize = getBrushSizeKnob()->getValueAtTime(time) / 2.;
    bool pressureAffectsSize = getPressureSizeKnob()->getValueAtTime(time);
    Transform::Matrix3x3 transform;
    getTransformAtTime(time, view, &transform);

    for (std::vector<RotoStrokeItemPrivate::StrokeCurves>::const_iterator it = _imp->strokes.begin(); it != _imp->strokes.end(); ++it) {
        KeyFrameSet xSet, ySet, pSet;
        {
            QMutexLocker k(&_imp->lock);
            xSet = it->xCurve->getKeyFrames_mt_safe();
            ySet = it->yCurve->getKeyFrames_mt_safe();
            pSet = it->pressureCurve->getKeyFrames_mt_safe();
        }
        assert( xSet.size() == ySet.size() && xSet.size() == pSet.size() );

        std::list<std::pair<Point, double> > points;

        evaluateStrokeInternal(xSet, ySet, pSet, transform, scale, brushSize, pressureAffectsSize, &points, 0);

        if (!points.empty()) {
            strokes->push_back(points);
        }
    }
}

U64
RotoStrokeItemPrivate::computeHashFromStrokes() const
{
    // Private- should not lock
    assert(!lock.tryLock());

    Hash64 hash;
    for (std::vector<RotoStrokeItemPrivate::StrokeCurves>::const_iterator it = strokes.begin(); it != strokes.end(); ++it) {
        Hash64::appendCurve(it->xCurve, &hash);
        Hash64::appendCurve(it->yCurve, &hash);

        // We don't add the pressure curve if there is more than 1 point, because it's extremely unlikely that the user draws twice the same curve with different pressure
        if (it->pressureCurve->getKeyFramesCount() == 1) {
            Hash64::appendCurve(it->pressureCurve, &hash);
        }
    }
    hash.computeHash();
    return hash.value();
}

void
RotoStrokeItem::appendToHash(const ComputeHashArgs& args, Hash64* hash)
{
    if (args.hashType != HashableObject::eComputeHashTypeTimeViewVariant) {
        return;
    }
    {
        // Append the item knobs
        QMutexLocker k(&_imp->lock);
        bool gotHash = false;
        U64 strokesHash;
        if (_imp->renderCachedHash) {
            std::map<TimeValue,U64>::const_iterator found = _imp->renderCachedHash->find(args.time);
            if (found != _imp->renderCachedHash->end()) {
                strokesHash = found->second;
                gotHash = true;
            }
        }
        if (!gotHash) {
             strokesHash = _imp->computeHashFromStrokes();
        }
        hash->append(strokesHash);
    }


    RotoDrawableItem::appendToHash(args, hash);
}

void
RotoStrokeItem::fetchRenderCloneKnobs()
{
    RotoDrawableItem::fetchRenderCloneKnobs();

    // Make a strength parameter when relevant
    if (_imp->type == eRotoStrokeTypeBlur ||
        _imp->type == eRotoStrokeTypeSharpen) {
        _imp->effectStrength = getOrCreateKnob<KnobDouble>(kRotoBrushEffectParam);
    }

    // This is global to any stroke
    _imp->pressureSize = getOrCreateKnob<KnobBool>(kRotoBrushPressureSizeParam);
    _imp->pressureOpacity = getOrCreateKnob<KnobBool>(kRotoBrushPressureOpacityParam);
    _imp->pressureHardness = getOrCreateKnob<KnobBool>(kRotoBrushPressureHardnessParam);
    _imp->buildUp = getOrCreateKnob<KnobBool>(kRotoBrushBuildupParam);

    if ( (_imp->type == eRotoStrokeTypeClone) || (_imp->type == eRotoStrokeTypeReveal) ) {
        // Clone transform
        _imp->cloneTranslate = getOrCreateKnob<KnobDouble>(kRotoBrushTranslateParam);
        _imp->cloneRotate = getOrCreateKnob<KnobDouble>(kRotoBrushRotateParam);
        _imp->cloneScale = getOrCreateKnob<KnobDouble>(kRotoBrushScaleParam);
        _imp->cloneScaleUniform = getOrCreateKnob<KnobBool>(kRotoBrushScaleUniformParam);
        _imp->cloneSkewX = getOrCreateKnob<KnobDouble>(kRotoBrushSkewXParam);
        _imp->cloneSkewY = getOrCreateKnob<KnobDouble>(kRotoBrushSkewYParam);
        _imp->cloneSkewOrder = getOrCreateKnob<KnobChoice>(kRotoBrushSkewOrderParam);
        _imp->cloneCenter = getOrCreateKnob<KnobDouble>(kRotoBrushCenterParam);
        _imp->cloneFilter = getOrCreateKnob<KnobChoice>(kRotoBrushFilterParam);
        _imp->cloneBlackOutside = getOrCreateKnob<KnobBool>(kRotoBrushBlackOutsideParam);
    }

    RotoStrokeItemPtr mainInstance =toRotoStrokeItem(getMainInstance());
    assert(mainInstance);
    QMutexLocker k(&mainInstance->_imp->lock);
    _imp->copyStrokeForRendering(*mainInstance->_imp);
}

void
RotoStrokeItem::initializeKnobs()
{


    // Make a strength parameter when relevant
    if (_imp->type == eRotoStrokeTypeBlur ||
        _imp->type == eRotoStrokeTypeSharpen) {
        _imp->effectStrength = createDuplicateOfTableKnob<KnobDouble>(kRotoBrushEffectParam);
    }

    // This is global to any stroke
    _imp->pressureSize = createDuplicateOfTableKnob<KnobBool>(kRotoBrushPressureSizeParam);
    _imp->pressureOpacity = createDuplicateOfTableKnob<KnobBool>(kRotoBrushPressureOpacityParam);
    _imp->pressureHardness = createDuplicateOfTableKnob<KnobBool>(kRotoBrushPressureHardnessParam);
    _imp->buildUp = createDuplicateOfTableKnob<KnobBool>(kRotoBrushBuildupParam);

    if ( (_imp->type == eRotoStrokeTypeClone) || (_imp->type == eRotoStrokeTypeReveal) ) {
        // Clone transform
        _imp->cloneTranslate = createDuplicateOfTableKnob<KnobDouble>(kRotoBrushTranslateParam);
        _imp->cloneRotate = createDuplicateOfTableKnob<KnobDouble>(kRotoBrushRotateParam);
        _imp->cloneScale = createDuplicateOfTableKnob<KnobDouble>(kRotoBrushScaleParam);
        _imp->cloneScaleUniform = createDuplicateOfTableKnob<KnobBool>(kRotoBrushScaleUniformParam);
        _imp->cloneSkewX = createDuplicateOfTableKnob<KnobDouble>(kRotoBrushSkewXParam);
        _imp->cloneSkewY = createDuplicateOfTableKnob<KnobDouble>(kRotoBrushSkewYParam);
        _imp->cloneSkewOrder = createDuplicateOfTableKnob<KnobChoice>(kRotoBrushSkewOrderParam);
        _imp->cloneCenter = createDuplicateOfTableKnob<KnobDouble>(kRotoBrushCenterParam);
        _imp->cloneFilter = createDuplicateOfTableKnob<KnobChoice>(kRotoBrushFilterParam);
        _imp->cloneBlackOutside = createDuplicateOfTableKnob<KnobBool>(kRotoBrushBlackOutsideParam);
    }
    RotoDrawableItem::initializeKnobs();

}


KnobDoublePtr
RotoStrokeItem::getBrushEffectKnob() const
{
    return _imp->effectStrength.lock();
}


KnobBoolPtr
RotoStrokeItem::getPressureOpacityKnob() const
{
    return _imp->pressureOpacity.lock();
}

KnobBoolPtr
RotoStrokeItem::getPressureSizeKnob() const
{
    return _imp->pressureSize.lock();
}

KnobBoolPtr
RotoStrokeItem::getPressureHardnessKnob() const
{
    return _imp->pressureHardness.lock();
}

KnobBoolPtr
RotoStrokeItem::getBuildupKnob() const
{
    return _imp->buildUp.lock();
}

KnobDoublePtr
RotoStrokeItem::getBrushCloneTranslateKnob() const
{
    return _imp->cloneTranslate.lock();
}

KnobDoublePtr
RotoStrokeItem::getBrushCloneRotateKnob() const
{
    return _imp->cloneRotate.lock();
}

KnobDoublePtr
RotoStrokeItem::getBrushCloneScaleKnob() const
{
    return _imp->cloneScale.lock();
}

KnobBoolPtr
RotoStrokeItem::getBrushCloneScaleUniformKnob() const
{
    return _imp->cloneScaleUniform.lock();
}

KnobDoublePtr
RotoStrokeItem::getBrushCloneSkewXKnob() const
{
    return _imp->cloneSkewX.lock();
}

KnobDoublePtr
RotoStrokeItem::getBrushCloneSkewYKnob() const
{
    return _imp->cloneSkewY.lock();
}

KnobChoicePtr
RotoStrokeItem::getBrushCloneSkewOrderKnob() const
{
    return _imp->cloneSkewOrder.lock();
}

KnobDoublePtr
RotoStrokeItem::getBrushCloneCenterKnob() const
{
    return _imp->cloneCenter.lock();
}

KnobChoicePtr
RotoStrokeItem::getBrushCloneFilterKnob() const
{
    return _imp->cloneFilter.lock();
}

KnobBoolPtr
RotoStrokeItem::getBrushCloneBlackOutsideKnob() const
{
    return _imp->cloneBlackOutside.lock();
}

NATRON_NAMESPACE_EXIT
