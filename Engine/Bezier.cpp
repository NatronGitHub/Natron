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

#include "Bezier.h"

#include <algorithm> // min, max
#include <sstream>
#include <locale>
#include <limits>
#include <cmath>
#include <cassert>
#include <stdexcept>

#include <QtCore/QThread>
#include <QCoreApplication>
#include <QtCore/QLineF>
#include <QtCore/QDebug>

GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
// /usr/local/include/boost/bind/arg.hpp:37:9: warning: unused typedef 'boost_static_assert_typedef_37' [-Wunused-local-typedef]
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/math/special_functions/fpclassify.hpp>
#include <boost/algorithm/string/predicate.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON

#include "Engine/AppInstance.h"
#include "Engine/BezierCP.h"
#include "Engine/FeatherPoint.h"
#include "Engine/Interpolation.h"
#include "Engine/TimeLine.h"
#include "Engine/Image.h"
#include "Engine/Hash64.h"
#include "Engine/Settings.h"
#include "Engine/KnobTypes.h"
#include "Engine/Node.h"
#include "Engine/Format.h"
#include "Engine/RotoLayer.h"
#include "Engine/RotoPaint.h"
#include "Engine/RotoPaintPrivate.h"
#include "Engine/RenderStats.h"
#include "Engine/Transform.h"
#include "Engine/Project.h"
#include "Engine/CoonsRegularization.h"
#include "Engine/ViewIdx.h"
#include "Engine/ViewerInstance.h"

#include "Serialization/BezierCPSerialization.h"
#include "Serialization/BezierSerialization.h"

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

struct BezierShape
{
    BezierCPs points; //< the control points of the curve
    BezierCPs featherPoints; //< the feather points, the number of feather points must equal the number of cp.

    bool finished; //< when finished is true, the last point of the list is connected to the first point of the list.

    BezierShape()
    : points()
    , featherPoints()
    , finished(false)
    {

    }
};

typedef std::map<ViewIdx, BezierShape> PerViewBezierShapeMap;

struct BezierPrivate
{
    mutable QMutex itemMutex; //< protects points & featherPoits
    PerViewBezierShapeMap viewShapes;

    bool isOpenBezier; // when true the Bezier will be rendered even if not closed

    std::string baseName;

    KnobDoubleWPtr feather; //< number of pixels to add to the feather distance (from the feather point), between -100 and 100
    KnobDoubleWPtr featherFallOff; //< the rate of fall-off for the feather, between 0 and 1,  0.5 meaning the
    //alpha value is half the original value when at half distance from the feather distance
    KnobChoiceWPtr fallOffRampType;
    KnobBoolWPtr fillShapeKnob;

    KnobKeyFrameMarkersWPtr shapeKeys;

    // When it is a render clone, we cache the result of getBoundingBox()
    boost::scoped_ptr<std::map<TimeValue,RectD> > renderCloneBBoxCache;

    BezierPrivate(const std::string& baseName, bool isOpenBezier)
    : itemMutex()
    , viewShapes()
    , isOpenBezier(isOpenBezier)
    , baseName(baseName)
    , renderCloneBBoxCache()
    {
        viewShapes.insert(std::make_pair(ViewIdx(0), BezierShape()));
    }

    BezierPrivate(const BezierPrivate& other)
    : itemMutex()
    , viewShapes()
    , renderCloneBBoxCache()
    {
        isOpenBezier = other.isOpenBezier;
        baseName = other.baseName;
    }
    
    const BezierShape* getViewShape(ViewIdx view) const
    {
        assert(!itemMutex.tryLock());
        PerViewBezierShapeMap::const_iterator found = viewShapes.find(view);
        if (found == viewShapes.end()) {
            return 0;
        }
        return &found->second;
    }

    BezierShape* getViewShape(ViewIdx view)
    {
        assert(!itemMutex.tryLock());
        PerViewBezierShapeMap::iterator found = viewShapes.find(view);
        if (found == viewShapes.end()) {
            return 0;
        }
        return &found->second;
    }

    BezierCPs::const_iterator atIndex(int index, const BezierShape& shape) const;
    
    BezierCPs::iterator atIndex(int index, BezierShape& shape);
    
    BezierCPs::const_iterator findControlPointNearby(double x,
                                                     double y,
                                                     double acceptance,
                                                     TimeValue time,
                                                     const BezierShape& shape,
                                                     const Transform::Matrix3x3& transform,
                                                     int* index) const;
    
    BezierCPs::const_iterator findFeatherPointNearby(double x,
                                                     double y,
                                                     double acceptance,
                                                     TimeValue time,
                                                     const BezierShape& shape,
                                                     const Transform::Matrix3x3& transform,
                                                     int* index) const;
};


Bezier::Bezier(const KnobItemsTablePtr& model,
               const std::string& baseName,
               bool isOpenBezier)
: RotoDrawableItem(model)
, _imp( new BezierPrivate(baseName, isOpenBezier) )
{
}

Bezier::Bezier(const BezierPtr& other, const FrameViewRenderKey& key)
: RotoDrawableItem(other, key)
, _imp( new BezierPrivate(*other->_imp) )
{

}

bool
Bezier::isOpenBezier() const
{
    return _imp->isOpenBezier;
}

bool
Bezier::isFillEnabled() const
{
    if (_imp->isOpenBezier) {
        return false;
    }
    KnobBoolPtr knob = _imp->fillShapeKnob.lock();
    if (!knob) {
        return true;
    }

    return knob->getValue();
}


Bezier::~Bezier()
{
 
}

KnobHolderPtr
Bezier::createRenderCopy(const FrameViewRenderKey& render) const
{
    BezierPtr mainInstance = toBezier(getMainInstance());
    if (!mainInstance) {
        mainInstance = toBezier(boost::const_pointer_cast<KnobHolder>(shared_from_this()));
    }
    BezierPtr ret(new Bezier(mainInstance, render));
    return ret;
}

RotoStrokeType
Bezier::getBrushType() const
{
    return eRotoStrokeTypeSolid;
}

bool
Bezier::isAutoKeyingEnabled() const
{
    EffectInstancePtr effect = getHolderEffect();
    if (!effect) {
        return false;
    }
    KnobButtonPtr knob = toKnobButton(effect->getKnobByName(kRotoUIParamAutoKeyingEnabled));
    if (!knob) {
        return false;
    }
    return knob->getValue();
}

bool
Bezier::isFeatherLinkEnabled() const
{
    EffectInstancePtr effect = getHolderEffect();
    if (!effect) {
        return false;
    }
    KnobButtonPtr knob = toKnobButton(effect->getKnobByName(kRotoUIParamFeatherLinkEnabled));
    if (!knob) {
        return false;
    }
    return knob->getValue();
}

bool
Bezier::isRippleEditEnabled() const
{
    EffectInstancePtr effect = getHolderEffect();
    if (!effect) {
        return false;
    }
    KnobButtonPtr knob = toKnobButton(effect->getKnobByName(kRotoUIParamRippleEdit));
    if (!knob) {
        return false;
    }
    return knob->getValue();

}


BezierCPs::const_iterator
BezierPrivate::atIndex(int index, const BezierShape& shape) const
{
    // PRIVATE - should not lock
    
    if ( ( index >= (int)shape.points.size() ) || (index < 0) ) {
        throw std::out_of_range("RotoSpline::atIndex: non-existent control point");
    }
    
    BezierCPs::const_iterator it = shape.points.begin();
    std::advance(it, index);
    
    return it;
}

BezierCPs::iterator
BezierPrivate::atIndex(int index, BezierShape& shape)
{
    // PRIVATE - should not lock
    
    if ( ( index >= (int)shape.points.size() ) || (index < 0) ) {
        throw std::out_of_range("RotoSpline::atIndex: non-existent control point");
    }
    
    BezierCPs::iterator it = shape.points.begin();
    std::advance(it, index);
    
    return it;
}


BezierCPs::const_iterator
BezierPrivate::findControlPointNearby(double x,
                                      double y,
                                      double acceptance,
                                      TimeValue time,
                                      const BezierShape& shape,
                                      const Transform::Matrix3x3& transform,
                                      int* index) const
{
    // PRIVATE - should not lock
    int i = 0;
    
    for (BezierCPs::const_iterator it = shape.points.begin(); it != shape.points.end(); ++it, ++i) {
        Transform::Point3D p;
        p.z = 1;
        (*it)->getPositionAtTime(time, &p.x, &p.y);
        p = Transform::matApply(transform, p);
        if ( ( p.x >= (x - acceptance) ) && ( p.x <= (x + acceptance) ) && ( p.y >= (y - acceptance) ) && ( p.y <= (y + acceptance) ) ) {
            *index = i;
            
            return it;
        }
    }
    
    return shape.points.end();
}

BezierCPs::const_iterator
BezierPrivate::findFeatherPointNearby(double x,
                                      double y,
                                      double acceptance,
                                      TimeValue time,
                                      const BezierShape& shape,
                                      const Transform::Matrix3x3& transform,
                                      int* index) const
{
    // PRIVATE - should not lock
    int i = 0;
    
    for (BezierCPs::const_iterator it = shape.featherPoints.begin(); it != shape.featherPoints.end(); ++it, ++i) {
        Transform::Point3D p;
        p.z = 1;
        (*it)->getPositionAtTime(time, &p.x, &p.y);
        p = Transform::matApply(transform, p);
        if ( ( p.x >= (x - acceptance) ) && ( p.x <= (x + acceptance) ) && ( p.y >= (y - acceptance) ) && ( p.y <= (y + acceptance) ) ) {
            *index = i;
            
            return it;
        }
    }
    
    return shape.featherPoints.end();
}



static inline double
lerp(double a,
     double b,
     double t)
{
    return a + (b - a) * t;
}

static inline void
lerpPoint(const Point & a,
          const Point & b,
          double t,
          Point *dest)
{
    dest->x = lerp(a.x, b.x, t);
    dest->y = lerp(a.y, b.y, t);
}

// compute value using the de Casteljau recursive formula
double
Bezier::bezierEval(double p0,
                   double p1,
                   double p2,
                   double p3,
                   double t)
{
    double p0p1, p1p2, p2p3, p0p1_p1p2, p1p2_p2p3;

    p0p1 = lerp(p0, p1, t);
    p1p2 = lerp(p1, p2, t);
    p2p3 = lerp(p2, p3, t);
    p0p1_p1p2 = lerp(p0p1, p1p2, t);
    p1p2_p2p3 = lerp(p1p2, p2p3, t);

    return lerp(p0p1_p1p2, p1p2_p2p3, t);
}

// compute point using the de Casteljau recursive formula
void
Bezier::bezierFullPoint(const Point & p0,
                        const Point & p1,
                        const Point & p2,
                        const Point & p3,
                        double t,
                        Point *p0p1,
                        Point *p1p2,
                        Point *p2p3,
                        Point *p0p1_p1p2,
                        Point *p1p2_p2p3,
                        Point *dest)
{
    lerpPoint(p0, p1, t, p0p1);
    lerpPoint(p1, p2, t, p1p2);
    lerpPoint(p2, p3, t, p2p3);
    lerpPoint(*p0p1, *p1p2, t, p0p1_p1p2);
    lerpPoint(*p1p2, *p2p3, t, p1p2_p2p3);
    lerpPoint(*p0p1_p1p2, *p1p2_p2p3, t, dest);
}

void
Bezier::bezierPoint(const Point & p0,
                    const Point & p1,
                    const Point & p2,
                    const Point & p3,
                    double t,
                    Point *dest)
{
    Point p0p1, p1p2, p2p3, p0p1_p1p2, p1p2_p2p3;

    bezierFullPoint(p0, p1, p2, p3, t, &p0p1, &p1p2, &p2p3, &p0p1_p1p2, &p1p2_p2p3, dest);
}

#if 0 //UNUSED CODE
// compute polynomial coefficients so that
// P(t) = A*t^3 + B*t^2 + C*t + D
static inline void
bezierPolyCoeffs(double p0,
                 double p1,
                 double p2,
                 double p3,
                 double *a,
                 double *b,
                 double *c,
                 double *d)
{
    /*
     These coefficients are obtained from the Bezier formula above (bezierEval).
     Maple code:
     p0p1 := p0 + (p1-p0)*t:
     p1p2 := p1 + (p2-p1)*t:
     p2p3 := p2 + (p3-p2)*t:
     p0p1p1p2 := p0p1 +(p1p2-p0p1)*t:
     p1p2p2p3 := p1p2 +(p2p3-p1p2)*t:
     p := p0p1p1p2 + (p1p2p2p3-p0p1p1p2)*t:
     collect(p,t);
     */
    // d = P0
    *d = p0;
    // c = 3*P1-3*P0
    *c = 3 * p1 - 3 * p0;
    // b = 3*P2-6*P1+3*P0
    *b = 3 * p2 - 6 * p1 + 3 * p0;
    // a = P3-3*P2+3*P1-P0
    *a = p3 - 3 * p2 + 3 * p1 - p0;
}

#endif

// compute polynomial coefficients so that
// P'(t) = A*t^2 + B*t + C
static inline void
bezierPolyDerivativeCoeffs(double p0,
                           double p1,
                           double p2,
                           double p3,
                           double *a,
                           double *b,
                           double *c)
{
    /*
     These coefficients are obtained from the Bezier formula above (bezierEval).
     Maple code:
     p0p1 := p0 + (p1-p0)*t:
     p1p2 := p1 + (p2-p1)*t:
     p2p3 := p2 + (p3-p2)*t:
     p0p1p1p2 := p0p1 +(p1p2-p0p1)*t:
     p1p2p2p3 := p1p2 +(p2p3-p1p2)*t:
     p := p0p1p1p2 + (p1p2p2p3-p0p1p1p2)*t:
     collect(p,t);
     diff(collect(p, t), t);
     */
    // c = 3*P1-3*P0
    *c = 3 * p1 - 3 * p0;
    // b = 2*(3*P2-6*P1+3*P0)
    *b = 2 * (3 * p2 - 6 * p1 + 3 * p0);
    // a = 3*(P3-3*P2+3*P1-P0)
    *a = 3 * (p3 - 3 * p2 + 3 * p1 - p0);
}

static inline void
updateRange(double x,
            double *xmin,
            double *xmax)
{
    if (x < *xmin) {
        *xmin = x;
    }
    if (x > *xmax) {
        *xmax = x;
    }
}

// compute the bounds of the Bezier for t \in [0,1]
// algorithm:
// - compute extrema of the cubic, i.e. values of t for
// which the derivative of the x coordinate of the
// Bezier is 0. If they are in [0,1] then they take part in
// range computation (there can be up to two extrema). the
// Bbox is the Bbox of these points and the
// extremal points (P0,P3)
static inline void
bezierBounds(double p0,
             double p1,
             double p2,
             double p3,
             double *xmin,
             double *xmax)
{
    // initialize with the range of the endpoints
    *xmin = std::min(p0, p3);
    *xmax = std::max(p0, p3);
    double a, b, c;
    bezierPolyDerivativeCoeffs(p0, p1, p2, p3, &a, &b, &c);
    if (a == 0) {
        //aX^2 + bX + c well then then this is a simple line
        //x= -c / b
        double t = -c / b;
        if ( (0 < t) && (t < 1) ) {
            updateRange(Bezier::bezierEval(p0, p1, p2, p3, t), xmin, xmax);
        }

        return;
    }
    double disc = b * b - 4 * a * c;
    if (disc < 0) {
        // no real solution
    } else if (disc == 0) {
        double t = -b / (2 * a);
        if ( (0 < t) && (t < 1) ) {
            updateRange(Bezier::bezierEval(p0, p1, p2, p3, t), xmin, xmax);
        }
    } else {
        double t;
        t = ( -b - std::sqrt(disc) ) / (2 * a);
        if ( (0 < t) && (t < 1) ) {
            updateRange(Bezier::bezierEval(p0, p1, p2, p3, t), xmin, xmax);
        }
        t = ( -b + std::sqrt(disc) ) / (2 * a);
        if ( (0 < t) && (t < 1) ) {
            updateRange(Bezier::bezierEval(p0, p1, p2, p3, t), xmin, xmax);
        }
    }
}


// compute a bounding box for the Bezier segment
// algorithm:
// - compute extrema of the cubic, i.e. values of t for
// which the derivative of the x or y coordinate of the
// Bezier is 0. If they are in [0,1] then they take part in
// bbox computation (there can be up to four extrema, 2 for
// x and 2 for y). the Bbox is the Bbox of these points and the
// extremal points (P0,P3)
RectD
Bezier::getBezierSegmentControlPolygonBbox(const Point & p0,
                                           const Point & p1,
                                           const Point & p2,
                                           const Point & p3)
{
    RectD ret;
    bezierBounds(p0.x, p1.x, p2.x, p3.x, &ret.x1, &ret.x2);
    bezierBounds(p0.y, p1.y, p2.y, p3.y, &ret.y1, &ret.y2);
    return ret;
} // getBezierSegmentControlPolygonBbox

struct BezierPoint
{
    Point p, left, right;
};

static void getBezierPoints(const std::list<BezierCPPtr > & points, TimeValue time, const Transform::Matrix3x3& transform, std::list<BezierPoint> &evaluatedPoints)
{
    assert(!points.empty());
    BezierCPs::const_iterator it = points.begin();
    BezierCPs::const_iterator next = it;
    ++next;
    BezierCPs::const_iterator prev = points.end();
    --prev;
    for (; it != points.end(); ++it, ++prev, ++next) {
        if (prev == points.end()) {
            prev = points.begin();
        }
        if ( next == points.end() ) {
            next = points.begin();
        }

        BezierPoint pt;

        {
            Transform::Point3D p, r, l;
            p.z = r.z = l.z = 1.;
            (*it)->getPositionAtTime(time, &p.x, &p.y);
            (*it)->getRightBezierPointAtTime(time, &r.x, &r.y);
            (*it)->getLeftBezierPointAtTime(time, &l.x, &l.y);
            p = Transform::matApply(transform, p);
            l = Transform::matApply(transform, l);
            r = Transform::matApply(transform, r);

            pt.p.x = p.x / p.z;
            pt.left.x = l.x / p.z;
            pt.right.x = r.x / p.z;

            pt.p.y = p.y / p.z;
            pt.left.y = l.y / l.z;
            pt.right.y = r.y / r.z;

        }


        evaluatedPoints.push_back(pt);


    } // for each control point
} // getBezierExpandedPoints

RectD
Bezier::getBezierSegmentListBbox(const std::list<BezierCPPtr > & points,
                                 double featherDistance,
                                 TimeValue time,
                                 const Transform::Matrix3x3& transform) ///< input/output
{
    RectD bbox;
    if ( points.empty() ) {
        return bbox;
    }
    if (points.size() == 1) {
        // only one point
        Transform::Point3D p0;
        const BezierCPPtr& p = points.front();
        p->getPositionAtTime(time, &p0.x, &p0.y);
        p0.z = 1;
        p0 = Transform::matApply(transform, p0);
        bbox.x1 = p0.x;
        bbox.x2 = p0.x;
        bbox.y1 = p0.y;
        bbox.y2 = p0.y;

        return bbox;
    }

    std::list<BezierPoint> evaluatedPoints;
    getBezierPoints(points, time, transform, evaluatedPoints);

    bool bboxSet = false;

    std::list<BezierPoint>::iterator it = evaluatedPoints.begin();
    std::list<BezierPoint>::iterator next = it;
    ++next;
    for (; it != evaluatedPoints.end(); ++it, ++next) {
        if (next == evaluatedPoints.end()) {
            next = evaluatedPoints.begin();
        }

        Point p0, p1, p2, p3;
        p0 = it->p;
        p1 = it->right;
        p2 = next->left;
        p3 = next->p;

        RectD segmentBbox = Bezier::getBezierSegmentControlPolygonBbox(p0, p1, p2, p3);


        if (!bboxSet) {
            bboxSet = true;
            bbox = segmentBbox;
        } else {
            bbox.merge(segmentBbox);
        }

    }

    // To account for the feather distance, we cannot just offset the control polygon points by the feather distance, we want the bounding box of all points generated on the polygon
    // offset to the feather distance.
    // This results to just padding the bbox by the featherDistance.
    // Since a negative feather distance is allowed, we always grow the bbox to be sure that we do not shrink it.
    double absFeather = std::abs(featherDistance);
    bbox.x1 -= absFeather;
    bbox.y1 -= absFeather;
    bbox.x2 += absFeather;
    bbox.y2 += absFeather;

    return bbox;
} // bezierSegmentListBboxUpdate

inline double euclDist(double x1, double y1, double x2, double y2)
{
    double dx = x2 - x1;
    double dy = y2 - y1;
    return dx * dx + dy * dy;
}


inline void addPointConditionnally(const Point& p, double t, int segmentIndex, std::vector< ParametricPoint >* points)
{
    if (points->empty()) {
        ParametricPoint x;
        x.x = p.x;
        x.y = p.y;
        x.t = t + segmentIndex;
        points->push_back(x);
    } else {
        const ParametricPoint& b = points->back();
        if (b.x != p.x || b.y != p.y) {
            ParametricPoint x;
            x.x = p.x;
            x.y = p.y;
            x.t = t + segmentIndex;
            points->push_back(x);
        }
    }
}

/**
 * @brief Recursively subdivide the Bezier segment p0,p1,p2,p3 until the cubic curve is assumed to be flat. The errorScale is used to determine the stopping criterion.
 * The greater it is, the smoother the curve will be.
 **/
static void
recursiveBezierInternal(int segmentIndex, const Point& p0, const Point& p1, const Point& p2, const Point& p3,
                        double t_p0, double t_p1, double t_p2, double t_p3,
                        double errorScale, int recursionLevel, int maxRecursion, std::vector< ParametricPoint >* points)
{

    if (recursionLevel > maxRecursion) {
        return;
    }
    
    double x12   = (p0.x + p1.x) / 2;
    double y12   = (p0.y + p1.y) / 2;
    double x23   = (p1.x + p2.x) / 2;
    double y23   = (p1.y + p2.y) / 2;
    double x34   = (p2.x + p3.x) / 2;
    double y34   = (p2.y + p3.y) / 2;
    double x123  = (x12 + x23) / 2;
    double y123  = (y12 + y23) / 2;
    double x234  = (x23 + x34) / 2;
    double y234  = (y23 + y34) / 2;
    double x1234 = (x123 + x234) / 2;
    double y1234 = (y123 + y234) / 2;

    double t_p12 = (t_p0 + t_p1) / 2.;
    double t_p23 = (t_p1 + t_p2) / 2.;
    double t_p34 = (t_p2 + t_p3) / 2;
    double t_p123 = (t_p12 + t_p23) / 2.;
    double t_p234 = (t_p23 + t_p34) / 2.;
    double t_p1234 = (t_p123 + t_p234) / 2.;

    static const double angleToleranceMax = 0.;
    static const double cuspTolerance = 0.;
    static const double collinearityEps = 1e-30;
    static const double angleToleranceEps = 0.01;

    double distanceToleranceSquare = 0.5 / errorScale;
    distanceToleranceSquare *= distanceToleranceSquare;


    // approximate the cubic curve by a straight line
    // See http://algorithmist.net/docs/subdivision.pdf for stopping criterion
    double dx = p3.x - p0.x;
    double dy = p3.y - p0.y;

    double d2 = std::fabs(((p1.x - p3.x) * dy - (p1.y - p3.y) * dx));
    double d3 = std::fabs(((p2.x - p3.x) * dy - (p2.y - p3.y) * dx));

    double da1, da2;

    double segmentDistanceSq = dx * dx + dy * dy;

    int possibleCases = ((int)(d2 > collinearityEps) << 1) + (int)(d3 > collinearityEps);
    switch (possibleCases) {
        case 0: {
            // collinear OR p0 is p4
            if (segmentDistanceSq == 0) {
                d2 = (p1.x - p0.x) * (p1.x - p0.x) + (p1.y - p0.y) * (p1.y - p0.y);
                d3 = (p3.x - p2.x) * (p3.x - p2.x) + (p3.y - p2.y) * (p3.y - p2.y);
            } else {
                segmentDistanceSq  = 1 / segmentDistanceSq;
                da1 = p1.x - p0.x;
                da2 = p1.y - p0.y;
                d2  = segmentDistanceSq * (da1 * dx + da2 * dy);
                da1 = p2.x - p0.x;
                da2 = p2.y - p0.y;
                d3  = segmentDistanceSq * (da1 * dx + da2 * dy);
                if (d2 > 0 && d2 < 1 && d3 > 0 && d3 < 1) {
                    // Simple collinear case, 1---2---3---4
                    return;
                }
                if (d2 <= 0) {
                    d2 = euclDist(p1.x, p1.y, p0.x, p0.y);
                } else if (d2 >= 1) {
                    d2 = euclDist(p1.x, p3.x, p1.y, p3.y);
                } else  {
                    d2 = euclDist(p1.x, p1.y, p0.x + d2 * dx, p0.y + d2 * dy);
                }

                if (d3 <= 0) {
                    d3 = euclDist(p2.x, p0.y, p2.x, p0.y);
                } else if (d3 >= 1) {
                    d3 = euclDist(p2.x, p2.y, p3.x, p3.y);
                } else {
                    d3 = euclDist(p2.x, p2.y, p0.x + d3 * dx, p0.y + d3 + dy);
                }
            }
            if (d2 > d3) {
                if (d2 < distanceToleranceSquare) {
                    addPointConditionnally(p1, t_p1, segmentIndex, points);
                    return;
                }
            } else {
                if (d3 < distanceToleranceSquare) {
                    addPointConditionnally(p2, t_p2, segmentIndex, points);
                    return;
                }
            }
        }   break;
        case 1: {
            // p1,p2,p4 are collinear, p3 is significant
            if (d3 * d3 <= distanceToleranceSquare * segmentDistanceSq) {
                if (angleToleranceMax < angleToleranceEps) {
                    Point p;
                    p.x = x23;
                    p.y = y23;
                    addPointConditionnally(p, t_p23, segmentIndex, points);
                    return;
                }

                // Check Angle
                da1 = std::fabs(std::atan2(p3.y - p2.y, p3.x - p2.x) - std::atan2(p2.y - p1.y, p2.x - p1.x));
                if (da1 >= M_PI) {
                    da1 = 2. * M_PI - da1;
                }

                if (da1 < angleToleranceMax) {
                    addPointConditionnally(p1, t_p1, segmentIndex, points);
                    addPointConditionnally(p2, t_p2, segmentIndex, points);
                    return;
                }

                if (cuspTolerance != 0.0) {
                    if (da1 > cuspTolerance) {
                        addPointConditionnally(p2, t_p2, segmentIndex, points);
                        return;
                    }
                }
            }
        }   break;
        case 2: {
            // p1,p3,p4 are collinear, p2 is significant
            if (d2 * d2 <= distanceToleranceSquare * segmentDistanceSq) {
                if (angleToleranceMax < angleToleranceEps) {
                    Point p;
                    p.x = x23;
                    p.y = y23;
                    addPointConditionnally(p, t_p23, segmentIndex, points);
                    return;
                }
                // Check Angle
                da1 = std::fabs(std::atan2(p2.y - p1.y, p2.x - p1.x) - std::atan2(p1.y - p0.y, p1.x - p0.x));
                if (da1 >= M_PI) {
                    da1 = 2 * M_PI - da1;
                }

                if (da1 < angleToleranceMax) {
                    addPointConditionnally(p1, t_p1, segmentIndex, points);
                    addPointConditionnally(p2, t_p2, segmentIndex, points);
                    return;
                }

                if (cuspTolerance != 0.0) {
                    if (da1 > cuspTolerance) {
                        addPointConditionnally(p1, t_p1, segmentIndex, points);
                        return;
                    }
                }

            }
        }   break;
        case 3: {
            if ((d2 + d3) * (d2 + d3) <= distanceToleranceSquare * segmentDistanceSq) {
                // Check curvature
                if (angleToleranceMax < angleToleranceEps) {
                    Point p;
                    p.x = x23;
                    p.y = y23;
                    addPointConditionnally(p, t_p23, segmentIndex, points);
                    return;
                }

                // Handle  cusps
                double a23 = std::atan2(p2.y - p1.y, p2.x - p1.x);
                da1 = std::fabs(a23 - std::atan2(p1.y - p0.y, p1.x - p0.x));
                da2 = std::fabs(std::atan2(p3.y - p2.y, p3.x - p2.x) - a23);
                if (da1 >= M_PI) {
                    da1 = 2 * M_PI - da1;
                }
                if (da2 >= M_PI) {
                    da2 = 2 * M_PI - da2;
                }

                if (da1 + da2 < angleToleranceMax) {
                    Point p;
                    p.x = x23;
                    p.y = y23;
                    addPointConditionnally(p, t_p23, segmentIndex, points);
                    return;
                }

                if (cuspTolerance != 0.0) {
                    if (da1 > cuspTolerance) {
                        addPointConditionnally(p1, t_p1, segmentIndex, points);
                        return;
                    }

                    if (da2 > cuspTolerance) {
                        addPointConditionnally(p2, t_p2, segmentIndex, points);
                        return;
                    }
                }
            }

        }   break;
        default:
            assert(false);
            break;
    } // possibleCases


    // Subdivide
    Point p12 = {x12, y12};
    Point p123 = {x123, y123};
    Point p1234 = {x1234, y1234};
    Point p234 = {x234, y234};
    Point p34 = {x34, y34};


    recursiveBezierInternal(segmentIndex, p0, p12, p123, p1234, t_p0, t_p12, t_p123, t_p1234, errorScale, recursionLevel + 1, maxRecursion, points);
    recursiveBezierInternal(segmentIndex, p1234, p234, p34, p3, t_p1234, t_p234, t_p34, t_p3, errorScale, recursionLevel + 1, maxRecursion, points);
} // recursiveBezierInternal

static void
recursiveBezier(const Point& p0, const Point& p1, const Point& p2, const Point& p3, int segmentIndex, bool skipFirstPoint, double errorScale, int maxRecursion, std::vector< ParametricPoint >* points)
{
    ParametricPoint p0x,p3x;
    p0x.x = p0.x;
    p0x.y = p0.y;
    p0x.t = segmentIndex + 0.;
    p3x.x = p3.x;
    p3x.y = p3.y;
    p3x.t = segmentIndex + 1.;
    if (!skipFirstPoint) {
        points->push_back(p0x);
    }
    recursiveBezierInternal(segmentIndex, p0, p1, p2, p3, 0., 1. / 3., 2. / 3., 1., errorScale, 0, maxRecursion, points);
    points->push_back(p3x);
}

// compute nbPointsperSegment points and update the bbox bounding box for the Bezier
// segment from 'first' to 'last' evaluated at 'time'
// If nbPointsPerSegment is -1 then it will be automatically computed
static void
bezierSegmentEval(const Point& p0,
                  const Point& p1,
                  const Point& p2,
                  const Point& p3,
                  int segmentIndex,
                  bool skipFirstPoint,
                  Bezier::DeCasteljauAlgorithmEnum algo,
                  int nbPointsPerSegment,
                  double errorScale,
                  std::vector< ParametricPoint >* points, ///< output
                  RectD* bbox = NULL) ///< input/output (optional)
{


    if (bbox) {
        *bbox = Bezier::getBezierSegmentControlPolygonBbox(p0,  p1,  p2,  p3);
    }

    switch (algo) {
        case Bezier::eDeCasteljauAlgorithmIterative: {
            if (nbPointsPerSegment == -1) {
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
            Point cur;
            for (int i = skipFirstPoint ? 1 : 0; i < nbPointsPerSegment; ++i) {
                ParametricPoint p;
                p.t = incr * i;
                Bezier::bezierPoint(p0, p1, p2, p3, p.t, &cur);
                p.t += segmentIndex;
                p.x = cur.x;
                p.y = cur.y;
                assert(!bbox || bbox->contains(p.x, p.y));
                points->push_back(p);
            }
        }   break;

        case Bezier::eDeCasteljauAlgorithmRecursive: {
            static const int maxRecursion = 32;
            recursiveBezier(p0, p1, p2, p3, segmentIndex, skipFirstPoint, errorScale, maxRecursion, points);
        }   break;
    }


} // bezierSegmentEval

/**
 * @brief Determines if the point (x,y) lies on the Bezier curve segment defined by first and last.
 * @returns True if the point is close (according to the acceptance) to the curve, false otherwise.
 * @param param[out] It is set to the parametric value at which the subdivision of the Bezier segment
 * yields the closest point to (x,y) on the curve.
 **/
static bool
bezierSegmentMeetsPoint(const BezierCP & first,
                        const BezierCP & last,
                        const Transform::Matrix3x3& transform,
                        TimeValue time,
                        double x,
                        double y,
                        double distance,
                        double *param) ///< output
{
    Transform::Point3D p0, p1, p2, p3;

    p0.z = p1.z = p2.z = p3.z = 1;

    first.getPositionAtTime(time, &p0.x, &p0.y);
    first.getRightBezierPointAtTime(time, &p1.x, &p1.y);
    last.getPositionAtTime(time, &p3.x, &p3.y);
    last.getLeftBezierPointAtTime(time, &p2.x, &p2.y);

    p0 = Transform::matApply(transform, p0);
    p1 = Transform::matApply(transform, p1);
    p2 = Transform::matApply(transform, p2);
    p3 = Transform::matApply(transform, p3);

    ///Use the control polygon to approximate segment length
    double length = ( std::sqrt( (p1.x - p0.x) * (p1.x - p0.x) + (p1.y - p0.y) * (p1.y - p0.y) ) +
                      std::sqrt( (p2.x - p1.x) * (p2.x - p1.x) + (p2.y - p1.y) * (p2.y - p1.y) ) +
                      std::sqrt( (p3.x - p2.x) * (p3.x - p2.x) + (p3.y - p2.y) * (p3.y - p2.y) ) );
    // increment is the distance divided by the  segment length
    double incr = length == 0. ? 1. : distance / length;
    Point p02d, p12d, p22d, p32d;
    {
        p02d.x = p0.x; p02d.y = p0.y;
        p12d.x = p1.x; p12d.y = p1.y;
        p22d.x = p2.x; p22d.y = p2.y;
        p32d.x = p3.x; p32d.y = p3.y;
    }
    ///the minimum square distance between a decasteljau point an the given (x,y) point
    ///we save a sqrt call
    double sqDistance = distance * distance;
    double minSqDistance = std::numeric_limits<double>::infinity();
    double tForMin = -1.;
    // 1/incr = 0.9 -> 2 points   +                + o
    // 1/incr = 1.0 -> 2 points   +                o
    // 1/incr = 1.1 -> 3 points   +              o +
    // 1/incr = 2.0 -> 3 points   +        o       o
    // 1/incr = 2.1 -> 4 points   +       o       o+
    int nbPoints = std::ceil(1. / incr) + 1;
    for (int i = 0; i < nbPoints; ++i) {
        double t = std::min(i * incr, 1.);
        // the last point should be t == 1;
        assert( t < 1 || (t == 1. && i == nbPoints - 1) );
        Point p;
        Bezier::bezierPoint(p02d, p12d, p22d, p32d, t, &p);
        double sqdist = (p.x - x) * (p.x - x) + (p.y - y) * (p.y - y);
        if ( (sqdist <= sqDistance) && (sqdist < minSqDistance) ) {
            minSqDistance = sqdist;
            tForMin = t;
        }
    }

    if (minSqDistance <= sqDistance) {
        *param = tForMin;

        return true;
    }

    return false;
} // bezierSegmentMeetsPoint

static bool
isPointCloseTo(TimeValue time,
               const BezierCP & p,
               double x,
               double y,
               const Transform::Matrix3x3& transform,
               double acceptance)
{
    Transform::Point3D pos;

    pos.z = 1;
    p.getPositionAtTime(time, &pos.x, &pos.y);
    pos = Transform::matApply(transform, pos);
    if ( ( pos.x >= (x - acceptance) ) && ( pos.x <= (x + acceptance) ) && ( pos.y >= (y - acceptance) ) && ( pos.y <= (y + acceptance) ) ) {
        return true;
    }

    return false;
}


void
Bezier::clearAllPoints()
{
    _imp->shapeKeys.lock()->removeAnimation(ViewSetSpec::all(), DimSpec::all(), eValueChangedReasonUserEdited);

    QMutexLocker k(&_imp->itemMutex);
    for (PerViewBezierShapeMap::iterator it = _imp->viewShapes.begin(); it != _imp->viewShapes.end(); ++it) {
        it->second.points.clear();
        it->second.featherPoints.clear();
        it->second.finished = false;
    }
}

void
Bezier::copyItem(const KnobTableItem& other)
{
    BezierPtr this_shared = toBezier( shared_from_this() );
    assert(this_shared);

    const Bezier* otherBezier = dynamic_cast<const Bezier*>(&other);
    if (!otherBezier) {
        return;
    }

    {
        bool useFeather = otherBezier->useFeatherPoints();
        QMutexLocker l(&_imp->itemMutex);
        for (PerViewBezierShapeMap::const_iterator itViews = otherBezier->_imp->viewShapes.begin(); itViews != otherBezier->_imp->viewShapes.end(); ++itViews) {
            assert(itViews->second.featherPoints.size() == itViews->second.points.size() || !useFeather);

            BezierShape& thisShape = _imp->viewShapes[itViews->first];

            thisShape.featherPoints.clear();
            thisShape.points.clear();

            BezierCPs::const_iterator itF = itViews->second.featherPoints.begin();
            for (BezierCPs::const_iterator it = itViews->second.points.begin(); it != itViews->second.points.end(); ++it) {
                BezierCPPtr cp( new BezierCP(this_shared) );
                cp->copyControlPoint(**it);
                thisShape.points.push_back(cp);
                if (useFeather) {
                    BezierCPPtr fp( new BezierCP(this_shared) );
                    fp->copyControlPoint(**itF);
                    thisShape.featherPoints.push_back(fp);
                    ++itF;
                }
            }
            thisShape.finished = itViews->second.finished && !_imp->isOpenBezier;
        } // for all views

        _imp->isOpenBezier = otherBezier->_imp->isOpenBezier;
    }
    RotoDrawableItem::copyItem(other);
    evaluateCurveModified();

} // copyItem

void
Bezier::setKeyFrame(TimeValue time, ViewSetSpec view)
{
    AnimatingObjectI::SetKeyFrameArgs args;
    args.dimension = DimIdx(0);
    args.view = view;
    KeyFrame k;
    k.setTime(time);
    _imp->shapeKeys.lock()->setKeyFrame(args, k);
}

void
Bezier::removeKeyFrame(TimeValue time, ViewSetSpec view)
{
    _imp->shapeKeys.lock()->deleteValueAtTime(time, view, DimIdx(0), eValueChangedReasonUserEdited);
}

bool
Bezier::hasKeyFrameAtTime(TimeValue time, ViewIdx view) const
{
    KeyFrame k;
    return _imp->shapeKeys.lock()->getAnimationCurve(view, DimIdx(0))->getKeyFrameWithTime(time, &k);
}

KeyFrameSet
Bezier::getKeyFrames(ViewIdx view) const
{
    return _imp->shapeKeys.lock()->getAnimationCurve(view, DimIdx(0))->getKeyFrames_mt_safe();
}

BezierCPPtr
Bezier::addControlPointInternal(double x, double y, TimeValue time, ViewIdx view)
{
    if ( isCurveFinished(view) ) {
        return BezierCPPtr();
    }

    TimeValue keyframeTime;
    ///if the curve is empty make a new keyframe at the current timeline's time
    ///otherwise re-use the time at which the keyframe was set on the first control point
    BezierCPPtr p;
    BezierPtr this_shared = toBezier( shared_from_this() );
    assert(this_shared);
    bool autoKeying = isAutoKeyingEnabled();

    {
        QMutexLocker l(&_imp->itemMutex);

        BezierShape* shape = _imp->getViewShape(view);
        if (!shape) {
            return BezierCPPtr();
        }
        assert(!shape->finished);
        if ( shape->points.empty() ) {
            keyframeTime = time;
        } else {
            KeyFrame k;
            bool hasOneKey = _imp->shapeKeys.lock()->getAnimationCurve(view, DimIdx(0))->getKeyFrameWithIndex(0, &k);
            if (!hasOneKey) {
                AppInstancePtr app = getApp();
                assert(app);
                keyframeTime = TimeValue(app->getTimeLine()->currentFrame());
            } else {
                keyframeTime = k.getTime();
            }
        }

        p.reset( new BezierCP(this_shared) );
        if (autoKeying) {
            p->setPositionAtTime(keyframeTime, x, y);
            p->setLeftBezierPointAtTime(keyframeTime, x, y);
            p->setRightBezierPointAtTime(keyframeTime, x, y);
        } else {
            p->setStaticPosition(x, y);
            p->setLeftBezierStaticPosition(x, y);
            p->setRightBezierStaticPosition(x, y);
        }
        shape->points.insert(shape->points.end(), p);

        if ( useFeatherPoints() ) {
            BezierCPPtr fp( new FeatherPoint(this_shared) );
            if (autoKeying) {
                fp->setPositionAtTime(keyframeTime, x, y);
                fp->setLeftBezierPointAtTime(keyframeTime, x, y);
                fp->setRightBezierPointAtTime(keyframeTime, x, y);
            } else {
                fp->setStaticPosition(x, y);
                fp->setLeftBezierStaticPosition(x, y);
                fp->setRightBezierStaticPosition(x, y);
            }
            shape->featherPoints.insert(shape->featherPoints.end(), fp);
        }
    }


    return p;

} // addControlPointInternal


BezierCPPtr
Bezier::addControlPoint(double x,
                        double y,
                        TimeValue time,
                        ViewSetSpec view)
{
    BezierCPPtr ret;
    if (view.isAll()) {
        std::list<ViewIdx> views = getViewsList();
        for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
            ret = addControlPointInternal(x, y, time, *it);
        }
    } else {
        ViewIdx view_i = checkIfViewExistsOrFallbackMainView(ViewIdx(view));
        ret = addControlPointInternal(x, y, time, view_i);
    }
    evaluateCurveModified();

    return ret;
} // Bezier::addControlPoint

void
Bezier::evaluateCurveModified()
{
    // If the curve is not finished, do not evaluate.
    if (!isOpenBezier()) {
        bool hasCurveFinished = false;
        std::list<ViewIdx> views = getViewsList();
        for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
            if (isCurveFinished(*it)) {
                hasCurveFinished = true;
                break;
            }
        }
        if (!hasCurveFinished) {
            return;
        }
    }
    invalidateCacheHashAndEvaluate(true, false);
    
}

BezierCPPtr
Bezier::addControlPointAfterIndexInternal(int index, double t, ViewIdx view)
{
    BezierPtr this_shared = toBezier( shared_from_this() );
    assert(this_shared);

    BezierCPPtr p( new BezierCP(this_shared) );
    BezierCPPtr fp;

    if ( useFeatherPoints() ) {
        fp.reset( new FeatherPoint(this_shared) );
    }
    {
        QMutexLocker l(&_imp->itemMutex);
        BezierShape* shape = _imp->getViewShape(view);
        if (!shape) {
            return BezierCPPtr();
        }
        if ( ( index >= (int)shape->points.size() ) || (index < -1) ) {
            throw std::invalid_argument("Spline control point index out of range.");
        }


        ///we set the new control point position to be in the exact position the curve would have at each keyframe
        KeyFrameSet existingKeyframes = getKeyFrames(view);

        BezierCPs::const_iterator prev, next, prevF, nextF;
        if (index == -1) {
            prev = shape->points.end();
            if ( prev != shape->points.begin() ) {
                --prev;
            }
            next = shape->points.begin();

            if ( useFeatherPoints() ) {
                prevF = shape->featherPoints.end();
                if ( prevF != shape->featherPoints.begin() ) {
                    --prevF;
                }
                nextF = shape->featherPoints.begin();
            }
        } else {
            prev = _imp->atIndex(index, *shape);
            next = prev;
            if ( next != shape->points.end() ) {
                ++next;
            }
            if ( shape->finished && ( next == shape->points.end() ) ) {
                next = shape->points.begin();
            }
            assert( next != shape->points.end() );

            if ( useFeatherPoints() ) {
                prevF = shape->featherPoints.begin();
                std::advance(prevF, index);
                nextF = prevF;
                if ( nextF != shape->featherPoints.end() ) {
                    ++nextF;
                }
                if ( shape->finished && ( nextF == shape->featherPoints.end() ) ) {
                    nextF = shape->featherPoints.begin();
                }
            }
        }


        for (KeyFrameSet::iterator it = existingKeyframes.begin(); it != existingKeyframes.end(); ++it) {
            Point p0, p1, p2, p3;
            (*prev)->getPositionAtTime(it->getTime(), &p0.x, &p0.y);
            (*prev)->getRightBezierPointAtTime( it->getTime(), &p1.x, &p1.y);
            (*next)->getPositionAtTime(it->getTime(), &p3.x, &p3.y);
            (*next)->getLeftBezierPointAtTime(it->getTime(), &p2.x, &p2.y);

            Point p0f;
            Point p1f;
            if (useFeatherPoints() && prevF != shape->featherPoints.end() && *prevF) {
                (*prevF)->getPositionAtTime(it->getTime(), &p0f.x, &p0f.y);
                (*prevF)->getRightBezierPointAtTime(it->getTime(), &p1f.x, &p1f.y);
            } else {
                p0f = p0;
                p1f = p1;
            }
            Point p2f;
            Point p3f;
            if (useFeatherPoints() && nextF != shape->featherPoints.end() && *nextF) {
                (*nextF)->getPositionAtTime(it->getTime(), &p3f.x, &p3f.y);
                (*nextF)->getLeftBezierPointAtTime(it->getTime(), &p2f.x, &p2f.y);
            } else {
                p2f = p2;
                p3f = p3;
            }

            Point dest;
            Point p0p1, p1p2, p2p3, p0p1_p1p2, p1p2_p2p3;
            bezierFullPoint(p0, p1, p2, p3, t, &p0p1, &p1p2, &p2p3, &p0p1_p1p2, &p1p2_p2p3, &dest);

            Point destf;
            Point p0p1f, p1p2f, p2p3f, p0p1_p1p2f, p1p2_p2p3f;
            bezierFullPoint(p0f, p1f, p2f, p3f, t, &p0p1f, &p1p2f, &p2p3f, &p0p1_p1p2f, &p1p2_p2p3f, &destf);

            //update prev and next inner control points
            (*prev)->setRightBezierPointAtTime(it->getTime(), p0p1.x, p0p1.y);
            (*next)->setLeftBezierPointAtTime(it->getTime(), p2p3.x, p2p3.y);

            if ( useFeatherPoints() ) {
                if (prevF != shape->featherPoints.end() && *prevF) {
                    (*prevF)->setRightBezierPointAtTime(it->getTime(), p0p1f.x, p0p1f.y);
                }
                if (nextF != shape->featherPoints.end() && *nextF) {
                    (*nextF)->setLeftBezierPointAtTime(it->getTime(), p2p3f.x, p2p3f.y);
                }
            }


            p->setPositionAtTime(it->getTime(), dest.x, dest.y);
            ///The left control point of p is p0p1_p1p2 and the right control point is p1p2_p2p3
            p->setLeftBezierPointAtTime(it->getTime(), p0p1_p1p2.x, p0p1_p1p2.y);
            p->setRightBezierPointAtTime(it->getTime(), p1p2_p2p3.x, p1p2_p2p3.y);

            if ( useFeatherPoints() ) {
                fp->setPositionAtTime(it->getTime(), destf.x, destf.y);
                fp->setLeftBezierPointAtTime(it->getTime(), p0p1_p1p2f.x, p0p1_p1p2f.y);
                fp->setRightBezierPointAtTime(it->getTime(), p1p2_p2p3f.x, p1p2_p2p3f.y);
            }
        }

        ///if there's no keyframes
        if ( existingKeyframes.empty() ) {
            Point p0, p1, p2, p3;

            (*prev)->getPositionAtTime(TimeValue(0), &p0.x, &p0.y);
            (*prev)->getRightBezierPointAtTime(TimeValue(0), &p1.x, &p1.y);
            (*next)->getPositionAtTime(TimeValue(0), &p3.x, &p3.y);
            (*next)->getLeftBezierPointAtTime(TimeValue(0), &p2.x, &p2.y);


            Point dest;
            Point p0p1, p1p2, p2p3, p0p1_p1p2, p1p2_p2p3;
            bezierFullPoint(p0, p1, p2, p3, t, &p0p1, &p1p2, &p2p3, &p0p1_p1p2, &p1p2_p2p3, &dest);

            //update prev and next inner control points
            (*prev)->setRightBezierStaticPosition(p0p1.x, p0p1.y);
            (*next)->setLeftBezierStaticPosition(p2p3.x, p2p3.y);

            if ( useFeatherPoints() ) {
                (*prevF)->setRightBezierStaticPosition(p0p1.x, p0p1.y);
                (*nextF)->setLeftBezierStaticPosition(p2p3.x, p2p3.y);
            }

            p->setStaticPosition(dest.x, dest.y);
            ///The left control point of p is p0p1_p1p2 and the right control point is p1p2_p2p3
            p->setLeftBezierStaticPosition(p0p1_p1p2.x, p0p1_p1p2.y);
            p->setRightBezierStaticPosition(p1p2_p2p3.x, p1p2_p2p3.y);

            if ( useFeatherPoints() ) {
                fp->setStaticPosition(dest.x, dest.y);
                fp->setLeftBezierStaticPosition(p0p1_p1p2.x, p0p1_p1p2.y);
                fp->setRightBezierStaticPosition(p1p2_p2p3.x, p1p2_p2p3.y);
            }
        }


        ////Insert the point into the container
        if (index != -1) {
            BezierCPs::iterator it = shape->points.begin();
            ///it will point at the element right after index
            std::advance(it, index + 1);
            shape->points.insert(it, p);

            if ( useFeatherPoints() ) {
                ///insert the feather point
                BezierCPs::iterator itF = shape->featherPoints.begin();
                std::advance(itF, index + 1);
                shape->featherPoints.insert(itF, fp);
            }
        } else {
            shape->points.push_front(p);
            if ( useFeatherPoints() ) {
                shape->featherPoints.push_front(fp);
            }
        }


    }


    ///If auto-keying is enabled, set a new keyframe
    AppInstancePtr app = getApp();
    assert(app);
    TimeValue currentTime = TimeValue(app->getTimeLine()->currentFrame());


    if ( isAutoKeyingEnabled() ) {
        setKeyFrame(currentTime, view);
    }

    evaluateCurveModified();
    
    return p;
} // addControlPointAfterIndexInternal

BezierCPPtr
Bezier::addControlPointAfterIndex(int index,
                                  double t,
                                  ViewSetSpec view)
{
    BezierCPPtr ret;
    if (view.isAll()) {
        std::list<ViewIdx> views = getViewsList();
        for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
            ret = addControlPointAfterIndexInternal(index, t, *it);
        }
    } else {
        ViewIdx view_i = checkIfViewExistsOrFallbackMainView(ViewIdx(view));
        ret = addControlPointAfterIndexInternal(index, t, view_i);
    }
    evaluateCurveModified();

    return ret;

} // addControlPointAfterIndex

int
Bezier::getControlPointsCount(ViewIdx view) const
{
    ViewIdx view_i = checkIfViewExistsOrFallbackMainView(view);
    QMutexLocker k(&_imp->itemMutex);
    const BezierShape* shape = _imp->getViewShape(view_i);
    if (!shape) {
        return 0;
    }
    return (int)shape->points.size();
}

int
Bezier::isPointOnCurve(double x,
                       double y,
                       double distance,
                       TimeValue time,
                       ViewIdx view,
                       double *t,
                       bool* feather) const
{

    ViewIdx view_i = checkIfViewExistsOrFallbackMainView(view);


    Transform::Matrix3x3 transform;
    getTransformAtTime(time, view_i, &transform);

    QMutexLocker l(&_imp->itemMutex);

    const BezierShape* shape = _imp->getViewShape(view_i);
    if (!shape) {
        return -1;
    }


    ///special case: if the curve has only 1 control point, just check if the point
    ///is nearby that sole control point
    if (shape->points.size() == 1) {
        const BezierCPPtr & cp = shape->points.front();
        if ( isPointCloseTo(time, *cp, x, y, transform, distance) ) {
            *feather = false;

            return 0;
        } else {
            if ( useFeatherPoints() ) {
                ///do the same with the feather points
                const BezierCPPtr & fp = shape->featherPoints.front();
                if ( isPointCloseTo(time,  *fp, x, y, transform, distance) ) {
                    *feather = true;

                    return 0;
                }
            }
        }

        return -1;
    }

    ///For each segment find out if the point lies on the bezier
    int index = 0;
    bool useFeather = useFeatherPoints();

    assert( shape->featherPoints.size() == shape->points.size() || !useFeather);

    BezierCPs::const_iterator fp = shape->featherPoints.begin();
    for (BezierCPs::const_iterator it = shape->points.begin(); it != shape->points.end(); ++it, ++index) {
        BezierCPs::const_iterator next = it;
        BezierCPs::const_iterator nextFp = fp;

        if ( useFeather && ( nextFp != shape->featherPoints.end() ) ) {
            ++nextFp;
        }
        if ( next != shape->points.end() ) {
            ++next;
        }
        if ( next == shape->points.end() ) {
            if (!shape->finished) {
                return -1;
            } else {
                next = shape->points.begin();
                if (useFeather) {
                    nextFp = shape->featherPoints.begin();
                }
            }
        }
        if ( bezierSegmentMeetsPoint(*(*it), *(*next), transform, time, x, y, distance, t) ) {
            *feather = false;

            return index;
        }

        if ( useFeather && bezierSegmentMeetsPoint(**fp, **nextFp, transform, time,  x, y, distance, t) ) {
            *feather = true;

            return index;
        }
        if (useFeather) {
            ++fp;
        }
    }


    return -1;
} // isPointOnCurve

void
Bezier::setCurveFinished(bool finished, ViewSetSpec view)
{

    if (!_imp->isOpenBezier) {

        if (view.isAll()) {
            std::list<ViewIdx> views = getViewsList();
            for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
                QMutexLocker l(&_imp->itemMutex);
                BezierShape* shape = _imp->getViewShape(*it);
                if (!shape) {
                    return;
                }

                shape->finished = finished;
            }
        } else {
            ViewIdx view_i = checkIfViewExistsOrFallbackMainView(ViewIdx(view));
            QMutexLocker l(&_imp->itemMutex);
            BezierShape* shape = _imp->getViewShape(view_i);
            if (!shape) {
                return;
            }

            shape->finished = finished;
        }
        
    }
    
    resetTransformCenter();
    evaluateCurveModified();
}

bool
Bezier::isCurveFinished(ViewIdx view) const
{
    ViewIdx view_i = checkIfViewExistsOrFallbackMainView(view);
    QMutexLocker l(&_imp->itemMutex);
    const BezierShape* shape = _imp->getViewShape(view_i);
    if (!shape) {
        return false;
    }
    return shape->finished;
}

void
Bezier::removeControlPointByIndexInternal(int index, ViewIdx view)
{

    {
        QMutexLocker l(&_imp->itemMutex);
        BezierShape* shape = _imp->getViewShape(view);
        if (!shape) {
            return;
        }
        BezierCPs::iterator it;
        try {
            it = _imp->atIndex(index, *shape);
        } catch (...) {
            ///attempt to remove an unexsiting point
            return;
        }

        shape->points.erase(it);

        if ( useFeatherPoints() ) {
            BezierCPs::iterator itF = shape->featherPoints.begin();
            std::advance(itF, index);
            shape->featherPoints.erase(itF);
        }
    }
}

void
Bezier::removeControlPointByIndex(int index, ViewSetSpec view)
{
    if (view.isAll()) {
        std::list<ViewIdx> views = getViewsList();
        for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
            removeControlPointByIndexInternal(index, *it);
        }
    } else {
        ViewIdx view_i = checkIfViewExistsOrFallbackMainView(ViewIdx(view));
        removeControlPointByIndexInternal(index, view_i);
    }

    evaluateCurveModified();
}

void
Bezier::movePointByIndexInternalForView(int index, TimeValue time, ViewIdx view, double dx, double dy, bool onlyFeather)
{
    bool rippleEdit = isRippleEditEnabled();
    bool autoKeying = isAutoKeyingEnabled();
    bool fLinkEnabled = ( onlyFeather ? true : isFeatherLinkEnabled() );
    Transform::Matrix3x3 trans, invTrans;
    getTransformAtTime(time, view, &trans);

    if (!trans.inverse(&invTrans)) {
        trans.setIdentity();
    }

    {
        QMutexLocker l(&_imp->itemMutex);

        BezierShape* shape = _imp->getViewShape(view);
        if (!shape) {
            return;
        }
        Transform::Point3D p, left, right;
        p.z = left.z = right.z = 1;

        BezierCPPtr cp;
        bool isOnKeyframe = false;
        if (!onlyFeather) {
            BezierCPs::iterator it = shape->points.begin();
            if ( (index < 0) || ( index >= (int)shape->points.size() ) ) {
                throw std::runtime_error("invalid index");
            }
            std::advance(it, index);
            assert( it != shape->points.end() );
            cp = *it;
            cp->getPositionAtTime(time, &p.x, &p.y);
            isOnKeyframe |= cp->getLeftBezierPointAtTime(time,  &left.x, &left.y);
            cp->getRightBezierPointAtTime(time, &right.x, &right.y);

            p = Transform::matApply(trans, p);
            left = Transform::matApply(trans, left);
            right = Transform::matApply(trans, right);

            p.x += dx;
            p.y += dy;
            left.x += dx;
            left.y += dy;
            right.x += dx;
            right.y += dy;
        }

        bool useFeather = useFeatherPoints();
        Transform::Point3D pF, leftF, rightF;
        pF.z = leftF.z = rightF.z = 1;
        BezierCPPtr fp;
        if (useFeather) {
            BezierCPs::iterator itF = shape->featherPoints.begin();
            std::advance(itF, index);
            assert( itF != shape->featherPoints.end() );
            fp = *itF;
            fp->getPositionAtTime(time, &pF.x, &pF.y);
            isOnKeyframe |= fp->getLeftBezierPointAtTime(time,  &leftF.x, &leftF.y);
            fp->getRightBezierPointAtTime(time, &rightF.x, &rightF.y);

            pF = Transform::matApply(trans, pF);
            rightF = Transform::matApply(trans, rightF);
            leftF = Transform::matApply(trans, leftF);

            pF.x += dx;
            pF.y += dy;
            leftF.x += dx;
            leftF.y += dy;
            rightF.x += dx;
            rightF.y += dy;
        }

        bool moveFeather = ( fLinkEnabled || ( useFeather && fp && cp->equalsAtTime(time, *fp) ) );


        if ( !onlyFeather && (autoKeying || isOnKeyframe) ) {
            p = Transform::matApply(invTrans, p);
            right = Transform::matApply(invTrans, right);
            left = Transform::matApply(invTrans, left);

            assert(cp);
            cp->setPositionAtTime(time, p.x, p.y );
            cp->setLeftBezierPointAtTime(time, left.x, left.y);
            cp->setRightBezierPointAtTime(time, right.x, right.y);
        }

        if (moveFeather && useFeather) {
            if (autoKeying || isOnKeyframe) {
                assert(fp);

                pF = Transform::matApply(invTrans, pF);
                rightF = Transform::matApply(invTrans, rightF);
                leftF = Transform::matApply(invTrans, leftF);

                fp->setPositionAtTime(time, pF.x, pF.y);
                fp->setLeftBezierPointAtTime(time, leftF.x, leftF.y);
                fp->setRightBezierPointAtTime(time, rightF.x, rightF.y);
            }
        }

        if (rippleEdit) {

            KeyFrameSet keyframes = getKeyFrames(view);
            for (KeyFrameSet::iterator it2 = keyframes.begin(); it2 != keyframes.end(); ++it2) {
                if (it2->getTime() == time) {
                    continue;
                }
                if (!onlyFeather) {
                    assert(cp);
                    cp->getPositionAtTime(it2->getTime(), &p.x, &p.y);
                    cp->getLeftBezierPointAtTime(it2->getTime(),  &left.x, &left.y);
                    cp->getRightBezierPointAtTime(it2->getTime(),  &right.x, &right.y);

                    p = Transform::matApply(trans, p);
                    left = Transform::matApply(trans, left);
                    right = Transform::matApply(trans, right);

                    p.x += dx;
                    p.y += dy;
                    left.x += dx;
                    left.y += dy;
                    right.x += dx;
                    right.y += dy;

                    p = Transform::matApply(invTrans, p);
                    right = Transform::matApply(invTrans, right);
                    left = Transform::matApply(invTrans, left);


                    cp->setPositionAtTime(it2->getTime(), p.x, p.y );
                    cp->setLeftBezierPointAtTime(it2->getTime(), left.x, left.y);
                    cp->setRightBezierPointAtTime(it2->getTime(), right.x, right.y);
                }
                if (moveFeather && useFeather) {
                    assert(fp);
                    fp->getPositionAtTime(it2->getTime(), &pF.x, &pF.y);
                    fp->getLeftBezierPointAtTime(it2->getTime(),  &leftF.x, &leftF.y);
                    fp->getRightBezierPointAtTime(it2->getTime(), &rightF.x, &rightF.y);

                    pF = Transform::matApply(trans, pF);
                    rightF = Transform::matApply(trans, rightF);
                    leftF = Transform::matApply(trans, leftF);

                    pF.x += dx;
                    pF.y += dy;
                    leftF.x += dx;
                    leftF.y += dy;
                    rightF.x += dx;
                    rightF.y += dy;

                    pF = Transform::matApply(invTrans, pF);
                    rightF = Transform::matApply(invTrans, rightF);
                    leftF = Transform::matApply(invTrans, leftF);
                    
                    
                    fp->setPositionAtTime(it2->getTime(), pF.x, pF.y);
                    fp->setLeftBezierPointAtTime(it2->getTime(), leftF.x, leftF.y);
                    fp->setRightBezierPointAtTime(it2->getTime(), rightF.x, rightF.y);
                }
            }
        }
    }

} // movePointByIndexInternalForView

void
Bezier::movePointByIndexInternal(int index,
                                 TimeValue time,
                                 ViewSetSpec view,
                                 double dx,
                                 double dy,
                                 bool onlyFeather)
{
    ///only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );

    if (view.isAll()) {
        std::list<ViewIdx> views = getViewsList();
        for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
            movePointByIndexInternalForView(index, time, *it, dx, dy, onlyFeather);
        }
    } else {
        ViewIdx view_i = checkIfViewExistsOrFallbackMainView(ViewIdx(view));
        movePointByIndexInternalForView(index, time, view_i, dx, dy, onlyFeather);
    }
    if (isAutoKeyingEnabled()) {
        setKeyFrame(time, view);
    }
    evaluateCurveModified();

} // movePointByIndexInternal


void
Bezier::setPointByIndexInternalForView(int index, TimeValue time, ViewIdx view, double x, double y)
{
    Transform::Matrix3x3 trans, invTrans;
    getTransformAtTime(time, view, &trans);
    if (!trans.inverse(&invTrans)) {
        trans.setIdentity();
    }


    double dx, dy;

    {
        QMutexLocker l(&_imp->itemMutex);

        BezierShape* shape = _imp->getViewShape(view);
        if (!shape) {
            return;
        }
        Transform::Point3D p(0., 0., 1.);
        Transform::Point3D left(0., 0., 1.);
        Transform::Point3D right(0., 0., 1.);
        BezierCPPtr cp;
        BezierCPs::const_iterator it = _imp->atIndex(index, *shape);
        assert( it != shape->points.end() );
        cp = *it;
        cp->getPositionAtTime(time, &p.x, &p.y);
        cp->getLeftBezierPointAtTime(time,  &left.x, &left.y);
        cp->getRightBezierPointAtTime(time, &right.x, &right.y);

        p = Transform::matApply(trans, p);
        left = Transform::matApply(trans, left);
        right = Transform::matApply(trans, right);

        dx = x - p.x;
        dy = y - p.y;
    }

    movePointByIndexInternalForView(index, time, view, dx, dy, false);
}

void
Bezier::setPointByIndexInternal(int index, TimeValue time, ViewSetSpec view, double dx, double dy)
{
    if (view.isAll()) {
        std::list<ViewIdx> views = getViewsList();
        for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
            setPointByIndexInternalForView(index, time, *it, dx, dy);
        }
    } else {
        ViewIdx view_i = checkIfViewExistsOrFallbackMainView(ViewIdx(view));
        setPointByIndexInternalForView(index, time, view_i, dx, dy);
    }
    if (isAutoKeyingEnabled()) {
        setKeyFrame(time, view);
    }
    evaluateCurveModified();
}


void
Bezier::movePointByIndex(int index,
                         TimeValue time,
                         ViewSetSpec view,
                         double dx,
                         double dy)
{
    movePointByIndexInternal(index, time, view, dx, dy, false);
} // movePointByIndex

void
Bezier::setPointByIndex(int index,
                        TimeValue time,
                        ViewSetSpec view,
                        double x,
                        double y)
{
    setPointByIndexInternal(index, time, view, x, y);
} // setPointByIndex

void
Bezier::moveFeatherByIndex(int index,
                           TimeValue time,
                           ViewSetSpec view,
                           double dx,
                           double dy)
{
    movePointByIndexInternal(index, time, view, dx, dy, true);
} // moveFeatherByIndex

void
Bezier::moveBezierPointInternalForView(BezierCP* cpParam,
                                       BezierCP* fpParam,
                                       int index,
                                       TimeValue time,
                                       ViewIdx view,
                                       double lx, double ly, double rx, double ry,
                                       double flx, double fly, double frx, double fry,
                                       bool isLeft,
                                       bool moveBoth,
                                       bool onlyFeather)
{
    bool autoKeying = isAutoKeyingEnabled();
    bool featherLink = isFeatherLinkEnabled();
    bool rippleEdit = isRippleEditEnabled();
    Transform::Matrix3x3 trans, invTrans;
    getTransformAtTime(time, view, &trans);
    if (!trans.inverse(&invTrans)) {
        trans.setIdentity();
    }

    {
        QMutexLocker l(&_imp->itemMutex);
        BezierShape* shape = _imp->getViewShape(view);
        if (!shape) {
            return;
        }

        BezierCP* cp = 0;
        BezierCP* fp = 0;
        bool moveControlPoint = !onlyFeather;
        if (cpParam) {
            cp = cpParam;
        } else {
            BezierCPs::iterator cpIt = shape->points.begin();
            std::advance(cpIt, index);
            assert( cpIt != shape->points.end() );
            cp = cpIt->get();
            assert(cp);
        }

        if (fpParam) {
            fp = fpParam;
        } else {
            BezierCPs::iterator fpIt = shape->featherPoints.begin();
            std::advance(fpIt, index);
            assert( fpIt != shape->featherPoints.end() );
            fp = fpIt->get();
            assert(fp);
        }


        bool isOnKeyframe = false;
        Transform::Point3D left, right;
        left.z = right.z = 1;
        if (isLeft || moveBoth) {
            isOnKeyframe = (cp)->getLeftBezierPointAtTime(time, &left.x, &left.y);
            left = Transform::matApply(trans, left);
        }
        if (!isLeft || moveBoth) {
            isOnKeyframe = (cp)->getRightBezierPointAtTime(time, &right.x, &right.y);
            right = Transform::matApply(trans, right);
        }

        // Move the feather point if feather link is enabled
        bool moveFeather = (onlyFeather || featherLink) && fp;
        Transform::Point3D leftF, rightF;
        leftF.z = rightF.z = 1;
        if (useFeatherPoints() && fp) {
            if (isLeft || moveBoth) {
                (fp)->getLeftBezierPointAtTime(time, &leftF.x, &leftF.y);
                leftF = Transform::matApply(trans, leftF);

                // Move the feather point if it is identical to the control point
                if ( (left.x == leftF.x) && (left.y == leftF.y) ) {
                    moveFeather = true;
                }
            }
            if (!isLeft || moveBoth) {
                (fp)->getRightBezierPointAtTime(time, &rightF.x, &rightF.y);
                rightF = Transform::matApply(trans, rightF);

                // Move the feather point if it is identical to the control point
                if ( (right.x == rightF.x) && (right.y == rightF.y) ) {
                    moveFeather = true;
                }
            }
        }

        if (autoKeying || isOnKeyframe) {
            if (moveControlPoint) {
                if (isLeft || moveBoth) {
                    left.x += lx;
                    left.y += ly;
                    left = Transform::matApply(invTrans, left);
                    (cp)->setLeftBezierPointAtTime(time, left.x, left.y);
                }
                if (!isLeft || moveBoth) {
                    right.x += rx;
                    right.y += ry;
                    right = Transform::matApply(invTrans, right);
                    (cp)->setRightBezierPointAtTime(time, right.x, right.y);
                }
            }
            if ( moveFeather && useFeatherPoints() ) {
                if (isLeft || moveBoth) {
                    leftF.x += flx;
                    leftF.y += fly;
                    leftF = Transform::matApply(invTrans, leftF);
                    (fp)->setLeftBezierPointAtTime(time, leftF.x, leftF.y);
                }
                if (!isLeft || moveBoth) {
                    rightF.x += frx;
                    rightF.y += fry;
                    rightF = Transform::matApply(invTrans, rightF);
                    (fp)->setRightBezierPointAtTime(time, rightF.x, rightF.y);
                }
            }
        } else {
            ///this function is called when building a new Bezier we must
            ///move the static position if there is no keyframe, otherwise the
            ///curve would never be built
            if (moveControlPoint) {
                if (isLeft || moveBoth) {
                    left.x += lx;
                    left.y += ly;
                    left = Transform::matApply(invTrans, left);
                    (cp)->setLeftBezierStaticPosition(left.x, left.y);
                }
                if (!isLeft || moveBoth) {
                    right.x += rx;
                    right.y += ry;
                    right = Transform::matApply(invTrans, right);
                    (cp)->setRightBezierStaticPosition(right.x, right.y);
                }
            }
            if ( moveFeather && useFeatherPoints() ) {
                if (isLeft || moveBoth) {
                    leftF.x += flx;
                    leftF.y += fly;
                    leftF = Transform::matApply(invTrans, leftF);
                    (fp)->setLeftBezierStaticPosition(leftF.x, leftF.y);
                }
                if (!isLeft || moveBoth) {
                    rightF.x += frx;
                    rightF.y += fry;
                    rightF = Transform::matApply(invTrans, rightF);
                    (fp)->setRightBezierStaticPosition(rightF.x, rightF.y);
                }
            }
        }

        if (rippleEdit) {
            KeyFrameSet keyframes = getKeyFrames(view);

            for (KeyFrameSet::iterator it2 = keyframes.begin(); it2 != keyframes.end(); ++it2) {
                if (it2->getTime() == time) {
                    continue;
                }

                if (isLeft || moveBoth) {
                    if (moveControlPoint) {
                        (cp)->getLeftBezierPointAtTime(it2->getTime(), &left.x, &left.y);
                        left = Transform::matApply(trans, left);
                        left.x += lx; left.y += ly;
                        left = Transform::matApply(invTrans, left);
                        (cp)->setLeftBezierPointAtTime(it2->getTime(), left.x, left.y);
                    }
                    if ( moveFeather && useFeatherPoints() ) {
                        (fp)->getLeftBezierPointAtTime(it2->getTime(),  &leftF.x, &leftF.y);
                        leftF = Transform::matApply(trans, leftF);
                        leftF.x += flx; leftF.y += fly;
                        leftF = Transform::matApply(invTrans, leftF);
                        (fp)->setLeftBezierPointAtTime(it2->getTime(), leftF.x, leftF.y);
                    }
                } else {
                    if (moveControlPoint) {
                        (cp)->getRightBezierPointAtTime(it2->getTime(),  &right.x, &right.y);
                        right = Transform::matApply(trans, right);
                        right.x += rx; right.y += ry;
                        right = Transform::matApply(invTrans, right);
                        (cp)->setRightBezierPointAtTime(it2->getTime(), right.x, right.y);
                    }
                    if ( moveFeather && useFeatherPoints() ) {
                        (cp)->getRightBezierPointAtTime(it2->getTime(), &rightF.x, &rightF.y);
                        rightF = Transform::matApply(trans, rightF);
                        rightF.x += frx; rightF.y += fry;
                        rightF = Transform::matApply(invTrans, rightF);
                        (cp)->setRightBezierPointAtTime(it2->getTime(), rightF.x, rightF.y);
                    }
                }
            }
        }
    }

} // moveBezierPointInternalForView

void
Bezier::moveBezierPointInternal(BezierCP* cpParam,
                                BezierCP* fpParam,
                                int index,
                                TimeValue time,
                                ViewSetSpec view,
                                double lx,
                                double ly,
                                double rx,
                                double ry,
                                double flx,
                                double fly,
                                double frx,
                                double fry,
                                bool isLeft,
                                bool moveBoth,
                                bool onlyFeather)
{
    

    if (view.isAll()) {
        std::list<ViewIdx> views = getViewsList();
        for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
            moveBezierPointInternalForView(cpParam, fpParam, index, time, *it, lx, ly, rx, ry, flx, fly, frx, fry, isLeft, moveBoth, onlyFeather);
        }
    } else {
        ViewIdx view_i = checkIfViewExistsOrFallbackMainView(ViewIdx(view));
        moveBezierPointInternalForView(cpParam, fpParam, index, time, view_i, lx, ly, rx, ry, flx, fly, frx, fry, isLeft, moveBoth, onlyFeather);
    }

    if (isAutoKeyingEnabled()) {
        setKeyFrame(time, view);
    }
    evaluateCurveModified();

} // moveBezierPointInternal

void
Bezier::moveLeftBezierPoint(int index,
                            TimeValue time,
                            ViewSetSpec view,
                            double dx,
                            double dy)
{
    moveBezierPointInternal(NULL, NULL, index, time, view, dx, dy, 0, 0, dx, dy, 0, 0, true, false, false);
} // moveLeftBezierPoint

void
Bezier::moveRightBezierPoint(int index,
                             TimeValue time,
                             ViewSetSpec view,
                             double dx,
                             double dy)
{
    moveBezierPointInternal(NULL, NULL, index, time, view, 0, 0, dx, dy, 0, 0, dx, dy, false, false, false);
} // moveRightBezierPoint

void
Bezier::movePointLeftAndRightIndex(BezierCP & cp,
                                   BezierCP & fp,
                                   TimeValue time,
                                   ViewSetSpec view,
                                   double lx,
                                   double ly,
                                   double rx,
                                   double ry,
                                   double flx,
                                   double fly,
                                   double frx,
                                   double fry,
                                   bool onlyFeather)
{
    moveBezierPointInternal(&cp, &fp, -1, time, view, lx, ly, rx, ry, flx, fly, frx, fry, false, true, onlyFeather);
}

void
Bezier::setPointAtIndexInternalForView(bool setLeft, bool setRight, bool setPoint, bool feather, bool featherAndCp, int index, TimeValue time, ViewIdx view, double x, double y, double lx, double ly, double rx, double ry)
{
    bool autoKeying = isAutoKeyingEnabled();
    bool rippleEdit = isRippleEditEnabled();


    {
        QMutexLocker l(&_imp->itemMutex);
        BezierShape* shape = _imp->getViewShape(view);
        if (!shape) {
            return;
        }

        bool isOnKeyframe = hasKeyFrameAtTime(time, view);


        if ( index >= (int)shape->points.size() ) {
            throw std::invalid_argument("Bezier::setPointAtIndex: Index out of range.");
        }

        BezierCPs::iterator fp = shape->featherPoints.begin();
        BezierCPs::iterator cp = shape->points.begin();
        if (!feather && !featherAndCp) {
            fp = cp;
        }
        std::advance(fp, index);
        if (featherAndCp) {
            std::advance(cp, index);
        }

        if (autoKeying || isOnKeyframe) {
            if (setPoint) {
                (*fp)->setPositionAtTime(time, x, y);
                if (featherAndCp) {
                    (*cp)->setPositionAtTime(time, x, y);
                }
            }
            if (setLeft) {
                (*fp)->setLeftBezierPointAtTime(time, lx, ly);
                if (featherAndCp) {
                    (*cp)->setLeftBezierPointAtTime(time, lx, ly);
                }
            }
            if (setRight) {
                (*fp)->setRightBezierPointAtTime(time, rx, ry);
                if (featherAndCp) {
                    (*cp)->setRightBezierPointAtTime(time, rx, ry);
                }
            }
        }

        if (rippleEdit) {

            KeyFrameSet keyframes = getKeyFrames(view);
            for (KeyFrameSet::iterator it2 = keyframes.begin(); it2 != keyframes.end(); ++it2) {
                if (setPoint) {
                    (*fp)->setPositionAtTime(it2->getTime(), x, y);
                    if (featherAndCp) {
                        (*cp)->setPositionAtTime(it2->getTime(), x, y);
                    }
                }
                if (setLeft) {
                    (*fp)->setLeftBezierPointAtTime(it2->getTime(), lx, ly);
                    if (featherAndCp) {
                        (*cp)->setLeftBezierPointAtTime(it2->getTime(), lx, ly);
                    }
                }
                if (setRight) {
                    (*fp)->setRightBezierPointAtTime(it2->getTime(), rx, ry);
                    if (featherAndCp) {
                        (*cp)->setRightBezierPointAtTime(it2->getTime(), rx, ry);
                    }
                }
            }
        }
    }

} // setPointAtIndexInternalForView

void
Bezier::setPointAtIndexInternal(bool setLeft,
                                bool setRight,
                                bool setPoint,
                                bool feather,
                                bool featherAndCp,
                                int index,
                                TimeValue time,
                                ViewSetSpec view,
                                double x,
                                double y,
                                double lx,
                                double ly,
                                double rx,
                                double ry)
{

    if (view.isAll()) {
        std::list<ViewIdx> views = getViewsList();
        for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
            setPointAtIndexInternalForView(setLeft, setRight, setPoint, feather, featherAndCp, index, time, *it, x, y, lx, ly, rx, ry);
        }
    } else {
        ViewIdx view_i = checkIfViewExistsOrFallbackMainView(ViewIdx(view));
        setPointAtIndexInternalForView(setLeft, setRight, setPoint, feather, featherAndCp, index, time, view_i, x, y, lx, ly, rx, ry);
    }

    if (isAutoKeyingEnabled()) {
        setKeyFrame(time, view);
    }
    evaluateCurveModified();
} // setPointAtIndexInternal

void
Bezier::setLeftBezierPoint(int index,
                           TimeValue time,
                           ViewSetSpec view,
                           double x,
                           double y)
{
    setPointAtIndexInternal(true, false, false, false, true, index, time, view, 0, 0, x, y, 0, 0);
}

void
Bezier::setRightBezierPoint(int index,
                            TimeValue time,
                            ViewSetSpec view,
                            double x,
                            double y)
{
    setPointAtIndexInternal(false, true, false, false, true, index, time, view, 0, 0, 0, 0, x, y);
}

void
Bezier::setPointAtIndex(bool feather,
                        int index,
                        TimeValue time,
                        ViewSetSpec view,
                        double x,
                        double y,
                        double lx,
                        double ly,
                        double rx,
                        double ry)
{
    setPointAtIndexInternal(true, true, true, feather, false, index, time, view, x, y, lx, ly, rx, ry);
}

void
Bezier::transformPointInternal(const BezierCPPtr & point, TimeValue time, ViewIdx view, Transform::Matrix3x3* matrix)
{
    bool autoKeying = isAutoKeyingEnabled();

    {
        QMutexLocker l(&_imp->itemMutex);
        BezierShape* shape = _imp->getViewShape(view);
        if (!shape) {
            return;
        }

        Transform::Point3D cp, leftCp, rightCp;
        point->getPositionAtTime(time, &cp.x, &cp.y);
        point->getLeftBezierPointAtTime(time, &leftCp.x, &leftCp.y);
        bool isonKeyframe = point->getRightBezierPointAtTime(time,  &rightCp.x, &rightCp.y);


        cp.z = 1.;
        leftCp.z = 1.;
        rightCp.z = 1.;

        cp = matApply(*matrix, cp);
        leftCp = matApply(*matrix, leftCp);
        rightCp = matApply(*matrix, rightCp);

        cp.x /= cp.z; cp.y /= cp.z;
        leftCp.x /= leftCp.z; leftCp.y /= leftCp.z;
        rightCp.x /= rightCp.z; rightCp.y /= rightCp.z;

        if (autoKeying || isonKeyframe) {
            point->setPositionAtTime(time, cp.x, cp.y);
            point->setLeftBezierPointAtTime(time, leftCp.x, leftCp.y);
            point->setRightBezierPointAtTime(time, rightCp.x, rightCp.y);
        }
    }

}

void
Bezier::transformPoint(const BezierCPPtr & point,
                       TimeValue time,
                       ViewSetSpec view,
                       Transform::Matrix3x3* matrix)
{

    if (view.isAll()) {
        std::list<ViewIdx> views = getViewsList();
        for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
            transformPointInternal(point, time, *it, matrix);
        }
    } else {
        ViewIdx view_i = checkIfViewExistsOrFallbackMainView(ViewIdx(view));
        transformPointInternal(point, time, view_i, matrix);
    }

    evaluateCurveModified();
 
} // Bezier::transformPoint

void
Bezier::removeFeatherAtIndexForView(int index, ViewIdx view)
{
    QMutexLocker l(&_imp->itemMutex);

    BezierShape* shape = _imp->getViewShape(view);
    if (!shape) {
        return;
    }
    if ( index >= (int)shape->points.size() ) {
        throw std::invalid_argument("Bezier::removeFeatherAtIndex: Index out of range.");
    }

    BezierCPs::iterator cp = shape->points.begin();
    std::advance(cp, index);
    BezierCPs::iterator fp = shape->featherPoints.begin();
    std::advance(fp, index);

    assert( cp != shape->points.end() && fp != shape->featherPoints.end() );

    (*fp)->copyControlPoint(**cp);

}

void
Bezier::removeFeatherAtIndex(int index, ViewSetSpec view)
{
    assert( useFeatherPoints() );
    if (view.isAll()) {
        std::list<ViewIdx> views = getViewsList();
        for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
            removeFeatherAtIndexForView(index, *it);
        }
    } else {
        ViewIdx view_i = checkIfViewExistsOrFallbackMainView(ViewIdx(view));
        removeFeatherAtIndexForView(index, view_i);
    }


    evaluateCurveModified();
}

#define TANGENTS_CUSP_LIMIT 25

NATRON_NAMESPACE_ANONYMOUS_ENTER

static void
cuspTangent(double x,
            double y,
            double *tx,
            double *ty,
            const std::pair<double, double>& pixelScale)
{
    ///decrease the tangents distance by 1 fourth
    ///if the tangents are equal to the control point, make them 10 pixels long
    double dx = *tx - x;
    double dy = *ty - y;
    double distSquare = dx * dx + dy * dy;

    if (distSquare <= pixelScale.first * pixelScale.second * TANGENTS_CUSP_LIMIT * TANGENTS_CUSP_LIMIT) {
        *tx = x;
        *ty = y;
    } else {
        double newDx = 0.9 * dx;
        double newDy = 0.9 * dy;
        *tx = x + newDx;
        *ty = y + newDy;
    }
}


static void
smoothTangent(TimeValue time,
              bool left,
              const BezierCPPtr& p,
              const std::list < BezierCPPtr > & cps,
              const Transform::Matrix3x3& transform,
              double x,
              double y,
              double *tx,
              double *ty,
              const std::pair<double, double>& pixelScale)
{
    if ( (x == *tx) && (y == *ty) ) {


        if (cps.size() == 1) {
            return;
        }

        std::list < BezierCPPtr >::const_iterator prev = cps.end();
        if ( prev != cps.begin() ) {
            --prev;
        }
        std::list < BezierCPPtr >::const_iterator next = cps.begin();
        if ( next != cps.end() ) {
            ++next;
        }

        int index = 0;
        int cpCount = (int)cps.size();
        for (std::list < BezierCPPtr >::const_iterator it = cps.begin();
             it != cps.end();
             ++it) {
            if ( prev == cps.end() ) {
                prev = cps.begin();
            }
            if ( next == cps.end() ) {
                next = cps.begin();
            }
            if (*it == p) {
                break;
            }

            // increment for next iteration
            if ( prev != cps.end() ) {
                ++prev;
            }
            if ( next != cps.end() ) {
                ++next;
            }
            ++index;
        } // for(it)
        if ( prev == cps.end() ) {
            prev = cps.begin();
        }
        if ( next == cps.end() ) {
            next = cps.begin();
        }

        assert(index < cpCount);
        Q_UNUSED(cpCount);

        double leftDx, leftDy, rightDx, rightDy;
        Bezier::leftDerivativeAtPoint(time, *p, **prev, transform, &leftDx, &leftDy);
        Bezier::rightDerivativeAtPoint(time, *p, **next, transform, &rightDx, &rightDy);
        double norm = sqrt( (rightDx - leftDx) * (rightDx - leftDx) + (rightDy - leftDy) * (rightDy - leftDy) );
        Point delta;
        ///normalize derivatives by their norm
        if (norm != 0) {
            delta.x = ( (rightDx - leftDx) / norm ) * TANGENTS_CUSP_LIMIT * pixelScale.first;
            delta.y = ( (rightDy - leftDy) / norm ) * TANGENTS_CUSP_LIMIT * pixelScale.second;
        } else {
            ///both derivatives are the same, use the direction of the left one
            norm = sqrt( (leftDx - x) * (leftDx - x) + (leftDy - y) * (leftDy - y) );
            if (norm != 0) {
                delta.x = ( (rightDx - x) / norm ) * TANGENTS_CUSP_LIMIT * pixelScale.first;
                delta.y = ( (leftDy - y) / norm ) * TANGENTS_CUSP_LIMIT * pixelScale.second;
            } else {
                ///both derivatives and control point are equal, just use 0
                delta.x = delta.y = 0;
            }
        }

        if (!left) {
            *tx = x + delta.x;
            *ty = y + delta.y;
        } else {
            *tx = x - delta.x;
            *ty = y - delta.y;
        }
    } else {
        ///increase the tangents distance by 1 fourth
        ///if the tangents are equal to the control point, make them 10 pixels long
        double dx = *tx - x;
        double dy = *ty - y;
        double newDx, newDy;
        if ( (dx == 0) && (dy == 0) ) {
            dx = (dx < 0 ? -TANGENTS_CUSP_LIMIT : TANGENTS_CUSP_LIMIT) * pixelScale.first;
            dy = (dy < 0 ? -TANGENTS_CUSP_LIMIT : TANGENTS_CUSP_LIMIT) * pixelScale.second;
        }
        newDx = dx * 1.1;
        newDy = dy * 1.1;

        *tx = x + newDx;
        *ty = y + newDy;
    }
} // smoothTangent


static bool cuspPoint(TimeValue time,
                      const Transform::Matrix3x3& transform,
                      const BezierCPPtr& cp,
                      bool autoKeying,
                      bool rippleEdit,
                      const std::pair<double, double>& pixelScale)
{
    ///only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );

    Transform::Point3D pos, left, right;
    pos.z = left.z = right.z = 1.;

    cp->getPositionAtTime(time,  &pos.x, &pos.y);
    cp->getLeftBezierPointAtTime(time,  &left.x, &left.y);
    bool isOnKeyframe = cp->getRightBezierPointAtTime(time,  &right.x, &right.y);


    pos = Transform::matApply(transform, pos);
    left = Transform::matApply(transform, left);
    right = Transform::matApply(transform, right);

    Point newLeft, newRight;
    newLeft.x = left.x;
    newLeft.y = left.y;
    newRight.x = right.x;
    newRight.y = right.y;
    cuspTangent(pos.x, pos.y, &newLeft.x, &newLeft.y, pixelScale);
    cuspTangent(pos.x, pos.y, &newRight.x, &newRight.y, pixelScale);

    bool keyframeSet = false;

    if (autoKeying || isOnKeyframe) {
        cp->setLeftBezierPointAtTime(time, newLeft.x, newLeft.y);
        cp->setRightBezierPointAtTime(time, newRight.x, newRight.y);
        if (!isOnKeyframe) {
            keyframeSet = true;
        }
    }

    if (rippleEdit) {
        std::set<double> times;
        cp->getKeyframeTimes(&times);
        for (std::set<double>::iterator it = times.begin(); it != times.end(); ++it) {
            cp->setLeftBezierPointAtTime(TimeValue(*it), newLeft.x, newLeft.y);
            cp->setRightBezierPointAtTime(TimeValue(*it), newRight.x, newRight.y);
        }
    }

    return keyframeSet;
}

static bool smoothPoint(TimeValue time,
                        const Transform::Matrix3x3& transform,
                        const BezierCPPtr& cp,
                        const std::list < BezierCPPtr > & cps,
                        bool autoKeying,
                        bool rippleEdit,
                        const std::pair<double, double>& pixelScale)
{
    ///only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );


    Transform::Point3D pos, left, right;
    pos.z = left.z = right.z = 1.;

    cp->getPositionAtTime(time,  &pos.x, &pos.y);
    cp->getLeftBezierPointAtTime(time,  &left.x, &left.y);
    bool isOnKeyframe = cp->getRightBezierPointAtTime(time,  &right.x, &right.y);

    pos = Transform::matApply(transform, pos);
    left = Transform::matApply(transform, left);
    right = Transform::matApply(transform, right);

    smoothTangent(time, true, cp, cps, transform, pos.x, pos.y, &left.x, &left.y, pixelScale);
    smoothTangent(time, false, cp, cps, transform, pos.x, pos.y, &right.x, &right.y, pixelScale);

    bool keyframeSet = false;

    if (autoKeying || isOnKeyframe) {
        cp->setLeftBezierPointAtTime(time, left.x, left.y);
        cp->setRightBezierPointAtTime(time, right.x, right.y);
        if (!isOnKeyframe) {
            keyframeSet = true;
        }
    }

    if (rippleEdit) {
        std::set<double> times;
        cp->getKeyframeTimes(&times);
        for (std::set<double>::iterator it = times.begin(); it != times.end(); ++it) {
            cp->setLeftBezierPointAtTime(TimeValue(*it), left.x, left.y);
            cp->setRightBezierPointAtTime(TimeValue(*it), right.x, right.y);
        }
    }
    
    return keyframeSet;
} // smoothPoint

NATRON_NAMESPACE_ANONYMOUS_EXIT

void
Bezier::smoothOrCuspPointAtIndexInternal(bool isSmooth, int index, TimeValue time, ViewIdx view, const std::pair<double, double>& pixelScale)
{
    bool autoKeying = isAutoKeyingEnabled();
    bool rippleEdit = isRippleEditEnabled();

    Transform::Matrix3x3 transform;
    getTransformAtTime(time, view, &transform);


    {
        QMutexLocker l(&_imp->itemMutex);
        BezierShape* shape = _imp->getViewShape(view);
        if (!shape) {
            return;
        }
        if ( index >= (int)shape->points.size() ) {
            throw std::invalid_argument("Bezier::smoothOrCuspPointAtIndex: Index out of range.");
        }

        BezierCPs::iterator cp = shape->points.begin();
        std::advance(cp, index);
        BezierCPs::iterator fp;
        bool useFeather = useFeatherPoints();

        if (useFeather) {
            fp = shape->featherPoints.begin();
            std::advance(fp, index);
        }
        assert( cp != shape->points.end() && fp != shape->featherPoints.end() );
        if (isSmooth) {
            smoothPoint(time, transform, *cp, shape->points, autoKeying, rippleEdit, pixelScale);
            if (useFeather) {
                smoothPoint(time, transform, *fp, shape->featherPoints,autoKeying, rippleEdit, pixelScale);
            }
        } else {
            cuspPoint(time, transform, *cp, autoKeying, rippleEdit, pixelScale);
            if (useFeather) {
                cuspPoint(time, transform, *fp,  autoKeying, rippleEdit, pixelScale);
            }
        }
    }
    

} // smoothOrCuspPointAtIndexInternal

void
Bezier::smoothOrCuspPointAtIndex(bool isSmooth,
                                 int index,
                                 TimeValue time,
                                 ViewSetSpec view,
                                 const std::pair<double, double>& pixelScale)
{
    if (view.isAll()) {
        std::list<ViewIdx> views = getViewsList();
        for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
            smoothOrCuspPointAtIndexInternal(isSmooth, index, time, *it, pixelScale);
        }
    } else {
        ViewIdx view_i = checkIfViewExistsOrFallbackMainView(ViewIdx(view));
        smoothOrCuspPointAtIndexInternal(isSmooth, index, time, view_i, pixelScale);
    }


    if (isAutoKeyingEnabled()) {
        setKeyFrame(time, view);
    }
    evaluateCurveModified();

} // Bezier::smoothOrCuspPointAtIndex

void
Bezier::smoothPointAtIndex(int index,
                           TimeValue time,
                           ViewSetSpec view,
                           const std::pair<double, double>& pixelScale)
{
    smoothOrCuspPointAtIndex(true, index, time, view, pixelScale);
}

void
Bezier::cuspPointAtIndex(int index,
                         TimeValue time,
                         ViewSetSpec view,
                         const std::pair<double, double>& pixelScale)
{
    smoothOrCuspPointAtIndex(false, index, time, view, pixelScale);
}


bool
Bezier::splitView(ViewIdx view)
{
    bool ret = RotoDrawableItem::splitView(view);
    if (ret) {
        _imp->shapeKeys.lock()->splitView(view);
    }
    return ret;
}

bool
Bezier::unSplitView(ViewIdx view)
{
    bool ret = RotoDrawableItem::unSplitView(view);
    if (ret) {
        _imp->shapeKeys.lock()->unSplitView(view);
    }
    return ret;
}

bool
Bezier::findViewFromShapeKeysCurve(const Curve* curve, ViewIdx* view) const
{
    KnobKeyFrameMarkersPtr shapeKeys = _imp->shapeKeys.lock();
    std::list<ViewIdx> allViews = shapeKeys->getViewsList();


    bool foundView = false;
    for (std::list<ViewIdx>::const_iterator it = allViews.begin(); it!=allViews.end(); ++it) {
        CurvePtr thisCurve = shapeKeys->getAnimationCurve(*it, DimIdx(0));
        if (thisCurve.get() == curve) {
            *view = *it;
            foundView = true;
            break;
        }
    }
    return foundView;
}

void
Bezier::onKeyFrameRemoved(const Curve* curve, const KeyFrame& key)
{

    ViewIdx view;
    bool foundView = findViewFromShapeKeysCurve(curve, &view);
    assert(foundView);
    (void)foundView;


    QMutexLocker l(&_imp->itemMutex);
    BezierShape* shape = _imp->getViewShape(view);
    if (!shape) {
        return;
    }
    bool isOnKeyframe = hasKeyFrameAtTime(key.getTime(), view);

    if ( isOnKeyframe) {
        return;
    }
    assert( shape->featherPoints.size() == shape->points.size() || !useFeatherPoints() );

    bool useFeather = useFeatherPoints();
    BezierCPs::iterator fp = shape->featherPoints.begin();
    for (BezierCPs::iterator it = shape->points.begin(); it != shape->points.end(); ++it) {
        (*it)->removeKeyframe(key.getTime());
        if (useFeather) {
            (*fp)->removeKeyframe(key.getTime());
            ++fp;
        }
    }

} // onKeyFrameRemoved

void
Bezier::onKeyFrameSet(const Curve* curve, const KeyFrame& key, bool added)
{
    if (!added) {
        return;
    }

    // Find out which view this curve is


    ViewIdx view;
    bool foundView = findViewFromShapeKeysCurve(curve, &view);
    assert(foundView);
    (void)foundView;

    QMutexLocker l(&_imp->itemMutex);
    BezierShape* shape = _imp->getViewShape(view);
    if (!shape) {
        return;
    }

    bool isOnKeyframe = hasKeyFrameAtTime(key.getTime(), view);

    if ( isOnKeyframe) {
        return;
    }

    bool useFeather = useFeatherPoints();
    assert(shape->points.size() == shape->featherPoints.size() || !useFeather);

    for (BezierCPs::iterator it = shape->points.begin(); it != shape->points.end(); ++it) {
        double x, y;
        double leftDerivX, rightDerivX, leftDerivY, rightDerivY;

        (*it)->getPositionAtTime(key.getTime(),  &x, &y);
        (*it)->setPositionAtTime(key.getTime(), x, y);

        (*it)->getLeftBezierPointAtTime(key.getTime(),  &leftDerivX, &leftDerivY);
        (*it)->getRightBezierPointAtTime(key.getTime(),  &rightDerivX, &rightDerivY);
        (*it)->setLeftBezierPointAtTime(key.getTime(), leftDerivX, leftDerivY);
        (*it)->setRightBezierPointAtTime(key.getTime(), rightDerivX, rightDerivY);
    }

    if (useFeather) {
        for (BezierCPs::iterator it = shape->featherPoints.begin(); it != shape->featherPoints.end(); ++it) {
            double x, y;
            double leftDerivX, rightDerivX, leftDerivY, rightDerivY;

            (*it)->getPositionAtTime(key.getTime(), &x, &y);
            (*it)->setPositionAtTime(key.getTime(), x, y);

            (*it)->getLeftBezierPointAtTime(key.getTime(),  &leftDerivX, &leftDerivY);
            (*it)->getRightBezierPointAtTime(key.getTime(),  &rightDerivX, &rightDerivY);
            (*it)->setLeftBezierPointAtTime(key.getTime(), leftDerivX, leftDerivY);
            (*it)->setRightBezierPointAtTime(key.getTime(), rightDerivX, rightDerivY);
        }
    }
} // onKeyFrameAdded

void
Bezier::onKeyFrameMoved(const Curve* curve, const KeyFrame& from, const KeyFrame& to)
{
    ViewIdx view;
    bool foundView = findViewFromShapeKeysCurve(curve, &view);
    assert(foundView);
    (void)foundView;


    QMutexLocker l(&_imp->itemMutex);
    BezierShape* shape = _imp->getViewShape(view);
    if (!shape) {
        return;
    }

    assert( shape->featherPoints.size() == shape->points.size() || !useFeatherPoints() );

    bool useFeather = useFeatherPoints();
    for (BezierCPs::iterator it = shape->points.begin(); it != shape->points.end(); ++it) {
        double x, y;
        double leftDerivX, rightDerivX, leftDerivY, rightDerivY;

        (*it)->getPositionAtTime(from.getTime(),  &x, &y);
        (*it)->getLeftBezierPointAtTime(from.getTime(),  &leftDerivX, &leftDerivY);
        (*it)->getRightBezierPointAtTime(from.getTime(),  &rightDerivX, &rightDerivY);

        (*it)->removeKeyframe(from.getTime());

        (*it)->setPositionAtTime(to.getTime(), x, y);
        (*it)->setLeftBezierPointAtTime(to.getTime(), leftDerivX, leftDerivY);
        (*it)->setRightBezierPointAtTime(to.getTime(), rightDerivX, rightDerivY);

    }

    if (useFeather) {
        for (BezierCPs::iterator it = shape->featherPoints.begin(); it != shape->featherPoints.end(); ++it) {
            double x, y;
            double leftDerivX, rightDerivX, leftDerivY, rightDerivY;

            (*it)->getPositionAtTime(from.getTime(), &x, &y);
            (*it)->getLeftBezierPointAtTime(from.getTime(),  &leftDerivX, &leftDerivY);
            (*it)->getRightBezierPointAtTime(from.getTime(),  &rightDerivX, &rightDerivY);

            (*it)->removeKeyframe(from.getTime());

            (*it)->setPositionAtTime(to.getTime(), x, y);
            (*it)->setLeftBezierPointAtTime(to.getTime(), leftDerivX, leftDerivY);
            (*it)->setRightBezierPointAtTime(to.getTime(), rightDerivX, rightDerivY);
        }
    }

} // onKeyFrameMoved


void
Bezier::deCasteljau(bool isOpenBezier,
                    const std::list<BezierCPPtr >& cps,
                    TimeValue time,
                    const RenderScale &scale,
                    double featherDistance,
                    bool finished,
                    bool clockWise,
                    DeCasteljauAlgorithmEnum algo,
                    int nbPointsPerSegment,
                    double errorScale,
                    const Transform::Matrix3x3& transform,
                    std::vector<ParametricPoint>* pointsSingleList,
                    RectD* bbox)
{
    assert(!cps.empty());
    assert(pointsSingleList);



    {
        // Do not expand the control polygon points to the feather distance otherwise the feather will have a Bezier that differs from the original bezier
        // instead, we expand each discretized point.
        std::list<BezierPoint> evaluatedPoints;
        getBezierPoints(cps, time, transform, evaluatedPoints);

        int segmentIndex = 0;
        std::list<BezierPoint>::iterator it = evaluatedPoints.begin();
        std::list<BezierPoint>::iterator next = it;
        ++next;
        for (; it != evaluatedPoints.end(); ++it, ++next, ++segmentIndex) {
            if (next == evaluatedPoints.end()) {
                if (!finished || isOpenBezier) {
                    break;
                }
                next = evaluatedPoints.begin();
            }

            Point p0, p1, p2, p3;
            p0 = it->p;
            p1 = it->right;
            p2 = next->left;
            p3 = next->p;

            p0.x *= scale.x;
            p0.y *= scale.y;

            p1.x *= scale.x;
            p1.y *= scale.y;

            p2.x *= scale.x;
            p2.y *= scale.y;

            p3.x *= scale.x;
            p3.y *= scale.y;

            bezierSegmentEval(p0, p1, p2, p3, segmentIndex, !isOpenBezier /*skipFirstPoint*/, algo, nbPointsPerSegment, errorScale, pointsSingleList, 0);
        } // for each control point
    }

    
    // Expand points to feather distance if needed
    if (featherDistance != 0) {
        double featherX = featherDistance * scale.x;
        double featherY = featherDistance * scale.y;

        std::vector<ParametricPoint > pointsCopy = *pointsSingleList;
        std::vector<ParametricPoint >::iterator it = pointsCopy.begin();
        std::vector<ParametricPoint >::iterator outIt = pointsSingleList->begin();
        std::vector<ParametricPoint >::iterator next = it;
        ++next;
        std::vector<ParametricPoint >::iterator prev = pointsCopy.end();
        --prev;
        for (;it != pointsCopy.end(); ++it, ++prev, ++next, ++outIt) {
            if (prev == pointsCopy.end()) {
                prev = pointsCopy.begin();
            }
            if (next == pointsCopy.end()) {
                next = pointsCopy.begin();
            }
            double diffx = next->x - prev->x;
            double diffy = next->y - prev->y;
            double norm = std::sqrt( diffx * diffx + diffy * diffy );
            double dx = (norm != 0) ? -( diffy / norm ) : 0;
            double dy = (norm != 0) ? ( diffx / norm ) : 1;

            if (!clockWise) {
                outIt->x -= dx * featherX;
                outIt->y -= dy * featherY;
            } else {
                outIt->x += dx * featherX;
                outIt->y += dy * featherY;
            }
        }
    }

    if (bbox) {
        bool bboxSet = false;
        for (std::size_t i = 0; i < pointsSingleList->size(); ++i) {
            if (!bboxSet) {
                bboxSet = true;
                bbox->x1 = (*pointsSingleList)[i].x;
                bbox->x2 = (*pointsSingleList)[i].x;
                bbox->y1 = (*pointsSingleList)[i].y;
                bbox->y2 = (*pointsSingleList)[i].y;
            } else {
                bbox->x1 = std::min(bbox->x1, (*pointsSingleList)[i].x);
                bbox->x2 = std::max(bbox->x2, (*pointsSingleList)[i].x);
                bbox->y1 = std::min(bbox->y1, (*pointsSingleList)[i].y);
                bbox->y2 = std::max(bbox->y2, (*pointsSingleList)[i].y);
            }
        }
    }

} // deCasteljau



void
Bezier::evaluateAtTime(TimeValue time,
                       ViewIdx view,
                       const RenderScale &scale,
                       DeCasteljauAlgorithmEnum algo,
                       int nbPointsPerSegment,
                       double errorScale,
                       std::vector<ParametricPoint >* pointsSingleList,
                       RectD* bbox) const
{
    Transform::Matrix3x3 transform;
    getTransformAtTime(time, view, &transform);

    bool clockWise = isClockwiseOriented(time, view);

    QMutexLocker l(&_imp->itemMutex);
    ViewIdx view_i = checkIfViewExistsOrFallbackMainView(view);
    const BezierShape* shape = _imp->getViewShape(view_i);
    if (!shape) {
        return;
    }
    deCasteljau(isOpenBezier(),
                shape->points,
                time,
                scale,
                0, /*featherDistance*/
                shape->finished,
                clockWise,
                algo,
                nbPointsPerSegment,
                errorScale,
                transform,
                pointsSingleList,
                bbox);
} // evaluateAtTime



void
Bezier::evaluateFeatherPointsAtTime(bool applyFeatherDistance,
                                    TimeValue time,
                                    ViewIdx view,
                                    const RenderScale &scale,
                                    DeCasteljauAlgorithmEnum algo,
                                    int nbPointsPerSegment,
                                    double errorScale,
                                    std::vector<ParametricPoint >* pointsSingleList,
                                    RectD* bbox) const
{
    assert( useFeatherPoints() );

    bool clockWise = isClockwiseOriented(time, view);
    Transform::Matrix3x3 transform;
    getTransformAtTime(time, view, &transform);

    KnobDoublePtr featherKnob = _imp->feather.lock();
    double featherDistance = 0;
    if (applyFeatherDistance && featherKnob) {
        featherDistance = featherKnob->getValueAtTime(time);
    }

    QMutexLocker l(&_imp->itemMutex);
    ViewIdx view_i = checkIfViewExistsOrFallbackMainView(view);
    const BezierShape* shape = _imp->getViewShape(view_i);
    if (!shape) {
        return;
    }

    deCasteljau(isOpenBezier(),
                shape->featherPoints,
                time,
                scale,
                featherDistance,
                shape->finished,
                clockWise,
                algo,
                nbPointsPerSegment,
                errorScale,
                transform,
                pointsSingleList,
                bbox);

} // evaluateFeatherPointsAtTime


RectD
Bezier::getBoundingBox(TimeValue time, ViewIdx view) const
{

    ViewIdx view_i = checkIfViewExistsOrFallbackMainView(view);

    Transform::Matrix3x3 transform;
    getTransformAtTime(time, view, &transform);

    bool renderClone = isRenderClone();
    QMutexLocker l(&_imp->itemMutex);
    if (renderClone) {
        if (_imp->renderCloneBBoxCache) {
            std::map<TimeValue,RectD>::iterator foundTime = _imp->renderCloneBBoxCache->find(time);
            if (foundTime != _imp->renderCloneBBoxCache->end()) {
                return foundTime->second;
            }
        }
    }



    const BezierShape* shape = _imp->getViewShape(view_i);
    if (!shape) {
        return RectD();
    }
    
    RectD pointsBbox = getBezierSegmentListBbox(shape->points, 0 /*featherDistance*/, time,  transform);


    const bool isOpen = isOpenBezier() || !isFillEnabled();
    if (useFeatherPoints() && !isOpen) {

        // EDIT: Partial fix, just pad the BBOX by the feather distance. This might not be accurate but gives at least something
        // enclosing the real bbox and close enough
        KnobDoublePtr featherKnob = _imp->feather.lock();
        double featherDistance = 0;
        if (featherKnob) {
            featherDistance = featherKnob->getValueAtTime(time, DimIdx(0), view);
        }


        RectD featherPointsBbox = getBezierSegmentListBbox(shape->featherPoints, featherDistance, time, transform);
        pointsBbox.merge(featherPointsBbox);

        // Add one px padding so that we are sure the bounding box encloses the bounding box computed of the render points
        pointsBbox.addPadding(1, 1);

    } else if (isOpen) {
        double brushSize = getBrushSizeKnob()->getValueAtTime(time, DimIdx(0), view);
        double halfBrushSize = brushSize / 2. + 1;
        pointsBbox.x1 -= halfBrushSize;
        pointsBbox.x2 += halfBrushSize;
        pointsBbox.y1 -= halfBrushSize;
        pointsBbox.y2 += halfBrushSize;
    }


    if (renderClone) {

        if (!_imp->renderCloneBBoxCache) {
            _imp->renderCloneBBoxCache.reset(new std::map<TimeValue,RectD>);
            _imp->renderCloneBBoxCache->insert(std::make_pair(time, pointsBbox));
        }
    }
    return pointsBbox;
} // Bezier::getBoundingBox

std::list< BezierCPPtr >
Bezier::getControlPoints(ViewIdx view) const
{
    ViewIdx view_i = checkIfViewExistsOrFallbackMainView(view);
    QMutexLocker l(&_imp->itemMutex);
    const BezierShape* shape = _imp->getViewShape(view_i);
    if (!shape) {
        return std::list< BezierCPPtr >();
    }


    return shape->points;
}




std::list< BezierCPPtr >
Bezier::getFeatherPoints(ViewIdx view) const
{
    ViewIdx view_i = checkIfViewExistsOrFallbackMainView(view);
    QMutexLocker l(&_imp->itemMutex);
    const BezierShape* shape = _imp->getViewShape(view_i);
    if (!shape) {
        return std::list< BezierCPPtr >();
    }


    return shape->featherPoints;
}

std::pair<BezierCPPtr, BezierCPPtr >
Bezier::isNearbyControlPoint(double x,
                             double y,
                             double acceptance,
                             TimeValue time,
                             ViewIdx view,
                             ControlPointSelectionPrefEnum pref,
                             int* index) const
{
    ///only called on the main-thread
    Transform::Matrix3x3 transform;
    getTransformAtTime(time, view, &transform);
    QMutexLocker l(&_imp->itemMutex);
    BezierCPPtr cp, fp;
    ViewIdx view_i = checkIfViewExistsOrFallbackMainView(view);

    const BezierShape* shape = _imp->getViewShape(view_i);
    if (!shape) {
        return std::make_pair(cp, fp);
    }
    switch (pref) {
    case eControlPointSelectionPrefFeatherFirst: {
        BezierCPs::const_iterator itF = _imp->findFeatherPointNearby(x, y, acceptance, time, *shape, transform, index);
        if ( itF != shape->featherPoints.end() ) {
            fp = *itF;
            BezierCPs::const_iterator it = shape->points.begin();
            std::advance(it, *index);
            cp = *it;

            return std::make_pair(fp, cp);
        } else {
            BezierCPs::const_iterator it = _imp->findControlPointNearby(x, y, acceptance, time, *shape, transform, index);
            if ( it != shape->points.end() ) {
                cp = *it;
                itF = shape->featherPoints.begin();
                std::advance(itF, *index);
                fp = *itF;

                return std::make_pair(cp, fp);
            }
        }
        break;
    }
    case eControlPointSelectionPrefControlPointFirst:
    case eControlPointSelectionPrefWhateverFirst:
    default: {
        BezierCPs::const_iterator it = _imp->findControlPointNearby(x, y, acceptance, time, *shape, transform, index);
        if ( it != shape->points.end() ) {
            cp = *it;
            BezierCPs::const_iterator itF = shape->featherPoints.begin();
            std::advance(itF, *index);
            fp = *itF;

            return std::make_pair(cp, fp);
        } else {
            BezierCPs::const_iterator itF = _imp->findFeatherPointNearby(x, y, acceptance, time, *shape, transform, index);
            if ( itF != shape->featherPoints.end() ) {
                fp = *itF;
                it = shape->points.begin();
                std::advance(it, *index);
                cp = *it;

                return std::make_pair(fp, cp);
            }
        }
        break;
    }
    }

    ///empty pair
    *index = -1;

    return std::make_pair(cp, fp);
} // isNearbyControlPoint

int
Bezier::getControlPointIndex(const BezierCPPtr & cp, ViewIdx view) const
{
    return getControlPointIndex( cp.get() , view);
}

int
Bezier::getControlPointIndex(const BezierCP* cp, ViewIdx view) const
{
    ///only called on the main-thread
    assert(cp);
    QMutexLocker l(&_imp->itemMutex);
    ViewIdx view_i = checkIfViewExistsOrFallbackMainView(view);

    const BezierShape* shape = _imp->getViewShape(view_i);
    if (!shape) {
        return -1;
    }
    int i = 0;
    for (BezierCPs::const_iterator it = shape->points.begin(); it != shape->points.end(); ++it, ++i) {
        if (it->get() == cp) {
            return i;
        }
    }

    return -1;
}

int
Bezier::getFeatherPointIndex(const BezierCPPtr & fp, ViewIdx view) const
{
    ///only called on the main-thread
    QMutexLocker l(&_imp->itemMutex);
    int i = 0;
    ViewIdx view_i = checkIfViewExistsOrFallbackMainView(view);

    const BezierShape* shape = _imp->getViewShape(view_i);
    if (!shape) {
        return -1;
    }

    for (BezierCPs::const_iterator it = shape->featherPoints.begin(); it != shape->featherPoints.end(); ++it, ++i) {
        if (*it == fp) {
            return i;
        }
    }

    return -1;
}

BezierCPPtr
Bezier::getControlPointAtIndex(int index, ViewIdx view) const
{
    QMutexLocker l(&_imp->itemMutex);
    ViewIdx view_i = checkIfViewExistsOrFallbackMainView(view);

    const BezierShape* shape = _imp->getViewShape(view_i);
    if (!shape) {
        return BezierCPPtr();
    }
    

    if ( (index < 0) || ( index >= (int)shape->points.size() ) ) {
        return BezierCPPtr();
    }

    BezierCPs::const_iterator it = shape->points.begin();
    std::advance(it, index);

    return *it;
}

BezierCPPtr
Bezier::getFeatherPointAtIndex(int index, ViewIdx view) const
{
    QMutexLocker l(&_imp->itemMutex);
    ViewIdx view_i = checkIfViewExistsOrFallbackMainView(view);

    const BezierShape* shape = _imp->getViewShape(view_i);
    if (!shape) {
        return BezierCPPtr();
    }
    if ( (index < 0) || ( index >= (int)shape->featherPoints.size() ) ) {
        return BezierCPPtr();
    }

    BezierCPs::const_iterator it = shape->featherPoints.begin();
    std::advance(it, index);

    return *it;
}

std::list< std::pair<BezierCPPtr, BezierCPPtr > >
Bezier::controlPointsWithinRect(TimeValue time,
                                ViewIdx view,
                                double l,
                                double r,
                                double b,
                                double t,
                                double acceptance,
                                int mode) const
{
    std::list< std::pair<BezierCPPtr, BezierCPPtr > > ret;

    ///only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    QMutexLocker locker(&_imp->itemMutex);
    int i = 0;
    ViewIdx view_i = checkIfViewExistsOrFallbackMainView(view);

    const BezierShape* shape = _imp->getViewShape(view_i);
    if (!shape) {
        return ret;
    }
    if ( (mode == 0) || (mode == 1) ) {
        for (BezierCPs::const_iterator it = shape->points.begin(); it != shape->points.end(); ++it, ++i) {
            double x, y;
            (*it)->getPositionAtTime(time,  &x, &y);
            if ( ( x >= (l - acceptance) ) && ( x <= (r + acceptance) ) && ( y >= (b - acceptance) ) && ( y <= (t - acceptance) ) ) {
                std::pair<BezierCPPtr, BezierCPPtr > p;
                p.first = *it;
                BezierCPs::const_iterator itF = shape->featherPoints.begin();
                std::advance(itF, i);
                p.second = *itF;
                ret.push_back(p);
            }
        }
    }
    i = 0;
    if ( (mode == 0) || (mode == 2) ) {
        for (BezierCPs::const_iterator it = shape->featherPoints.begin(); it != shape->featherPoints.end(); ++it, ++i) {
            double x, y;
            (*it)->getPositionAtTime(time,  &x, &y);
            if ( ( x >= (l - acceptance) ) && ( x <= (r + acceptance) ) && ( y >= (b - acceptance) ) && ( y <= (t - acceptance) ) ) {
                std::pair<BezierCPPtr, BezierCPPtr > p;
                p.first = *it;
                BezierCPs::const_iterator itF = shape->points.begin();
                std::advance(itF, i);
                p.second = *itF;

                ///avoid duplicates
                bool found = false;
                for (std::list< std::pair<BezierCPPtr, BezierCPPtr > >::iterator it2 = ret.begin();
                     it2 != ret.end(); ++it2) {
                    if (it2->first == *itF) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    ret.push_back(p);
                }
            }
        }
    }

    return ret;
} // controlPointsWithinRect

BezierCPPtr
Bezier::getFeatherPointForControlPoint(const BezierCPPtr & cp, ViewIdx view) const
{
    assert( !cp->isFeatherPoint() );
    int index = getControlPointIndex(cp, view);
    assert(index != -1);

    return getFeatherPointAtIndex(index, view);
}

BezierCPPtr
Bezier::getControlPointForFeatherPoint(const BezierCPPtr & fp, ViewIdx view) const
{
    assert( fp->isFeatherPoint() );
    int index = getFeatherPointIndex(fp, view);
    assert(index != -1);
    if (index == -1) {
        return BezierCPPtr();
    }

    return getControlPointAtIndex(index, view);
}

void
Bezier::leftDerivativeAtPoint(TimeValue time,
                              const BezierCP & p,
                              const BezierCP & prev,
                              const Transform::Matrix3x3& transform,
                              double *dx,
                              double *dy)
{
    ///First-off, determine if the segment is a linear/quadratic/cubic Bezier segment.
    assert( !p.equalsAtTime(time,  prev) );
    bool p0equalsP1, p1equalsP2, p2equalsP3;
    Transform::Point3D p0, p1, p2, p3;
    p0.z = p1.z = p2.z = p3.z = 1;
    prev.getPositionAtTime(time,  &p0.x, &p0.y);
    prev.getRightBezierPointAtTime(time, &p1.x, &p1.y);
    p.getLeftBezierPointAtTime(time,  &p2.x, &p2.y);
    p.getPositionAtTime(time,  &p3.x, &p3.y);
    p0equalsP1 = p0.x == p1.x && p0.y == p1.y;
    p1equalsP2 = p1.x == p2.x && p1.y == p2.y;
    p2equalsP3 = p2.x == p3.x && p2.y == p3.y;

    p0 = Transform::matApply(transform, p0);
    p1 = Transform::matApply(transform, p1);
    p2 = Transform::matApply(transform, p2);
    p3 = Transform::matApply(transform, p3);

    int degree = 3;
    if (p0equalsP1) {
        --degree;
    }
    if (p1equalsP2) {
        --degree;
    }
    if (p2equalsP3) {
        --degree;
    }
    assert(degree >= 1 && degree <= 3);

    ///derivatives for t == 1.
    if (degree == 1) {
        *dx = p0.x - p3.x;
        *dy = p0.y - p3.y;
    } else if (degree == 2) {
        if (p0equalsP1) {
            p1 = p2;
        }
        *dx = 2. * (p1.x - p3.x);
        *dy = 2. * (p1.y - p3.y);
    } else {
        *dx = 3. * (p2.x - p3.x);
        *dy = 3. * (p2.y - p3.y);
    }
}

void
Bezier::rightDerivativeAtPoint(TimeValue time,
                               const BezierCP & p,
                               const BezierCP & next,
                               const Transform::Matrix3x3& transform,
                               double *dx,
                               double *dy)
{
    ///First-off, determine if the segment is a linear/quadratic/cubic Bezier segment.
    assert( !p.equalsAtTime(time, next) );
    bool p0equalsP1, p1equalsP2, p2equalsP3;
    Transform::Point3D p0, p1, p2, p3;
    p0.z = p1.z = p2.z = p3.z = 1;
    p.getPositionAtTime(time, &p0.x, &p0.y);
    p.getRightBezierPointAtTime(time, &p1.x, &p1.y);
    next.getLeftBezierPointAtTime(time, &p2.x, &p2.y);
    next.getPositionAtTime(time, &p3.x, &p3.y);
    p0equalsP1 = p0.x == p1.x && p0.y == p1.y;
    p1equalsP2 = p1.x == p2.x && p1.y == p2.y;
    p2equalsP3 = p2.x == p3.x && p2.y == p3.y;

    p0 = Transform::matApply(transform, p0);
    p1 = Transform::matApply(transform, p1);
    p2 = Transform::matApply(transform, p2);
    p3 = Transform::matApply(transform, p3);

    int degree = 3;
    if (p0equalsP1) {
        --degree;
    }
    if (p1equalsP2) {
        --degree;
    }
    if (p2equalsP3) {
        --degree;
    }
    assert(degree >= 1 && degree <= 3);

    ///derivatives for t == 0.
    if (degree == 1) {
        *dx = p3.x - p0.x;
        *dy = p3.y - p0.y;
    } else if (degree == 2) {
        if (p0equalsP1) {
            p1 = p2;
        }
        *dx = 2. * (p1.x - p0.x);
        *dy = 2. * (p1.y - p0.y);
    } else {
        *dx = 3. * (p1.x - p0.x);
        *dy = 3. * (p1.y - p0.y);
    }
}

void
Bezier::toSerialization(SERIALIZATION_NAMESPACE::SerializationObjectBase* obj)
{
    SERIALIZATION_NAMESPACE::BezierSerialization* s = dynamic_cast<SERIALIZATION_NAMESPACE::BezierSerialization*>(obj);
    if (!s) {
        return;
    }
    std::vector<std::string> projectViews = getApp()->getProject()->getProjectViewNames();
    {

        QMutexLocker l(&_imp->itemMutex);
        for (PerViewBezierShapeMap::const_iterator it = _imp->viewShapes.begin(); it != _imp->viewShapes.end(); ++it) {
            std::string view;
            if (it->first >= 0 && it->first < (int)projectViews.size()) {
                view = projectViews[it->first];
            }
            if (view.empty()) {
                view = "Main";
            }

            SERIALIZATION_NAMESPACE::BezierSerialization::Shape& shapeSerialization = s->_shapes[view];
            shapeSerialization.closed = it->second.finished;

            assert( it->second.featherPoints.size() == it->second.points.size() || !useFeatherPoints() );

            bool useFeather = useFeatherPoints();
            BezierCPs::const_iterator fp = it->second.featherPoints.begin();
            for (BezierCPs::const_iterator it2 = it->second.points.begin(); it2 != it->second.points.end(); ++it2) {
                SERIALIZATION_NAMESPACE::BezierSerialization::ControlPoint c;
                (*it2)->toSerialization(&c.innerPoint);
                if (useFeather) {
                    if (**it2 != **fp) {
                        c.featherPoint.reset(new SERIALIZATION_NAMESPACE::BezierCPSerialization);
                        (*fp)->toSerialization(c.featherPoint.get());
                    }
                    ++fp;
                }
                shapeSerialization.controlPoints.push_back(c);
                
            }

        } // for all views

        s->_isOpenBezier = _imp->isOpenBezier;

    }
    RotoDrawableItem::toSerialization(obj);
}

void
Bezier::fromSerialization(const SERIALIZATION_NAMESPACE::SerializationObjectBase & obj)
{
    const SERIALIZATION_NAMESPACE::BezierSerialization* s = dynamic_cast<const SERIALIZATION_NAMESPACE::BezierSerialization*>(&obj);
    if (!s) {
        return;
    }

    std::vector<std::string> projectViews = getApp()->getProject()->getProjectViewNames();


    BezierPtr this_shared = toBezier( shared_from_this() );
    assert(this_shared);
    {
        QMutexLocker l(&_imp->itemMutex);
        _imp->isOpenBezier = s->_isOpenBezier;


        for (SERIALIZATION_NAMESPACE::BezierSerialization::PerViewShapeMap::const_iterator it = s->_shapes.begin(); it != s->_shapes.end(); ++it) {

            // Find the view index corresponding to the view name
            ViewIdx view_i(0);
            Project::getViewIndex(projectViews, it->first, &view_i);

            BezierShape& shape = _imp->viewShapes[view_i];
            shape.finished = it->second.closed && !_imp->isOpenBezier;

            bool useFeather = useFeatherPoints();

            for (std::list<SERIALIZATION_NAMESPACE::BezierSerialization::ControlPoint>::const_iterator it2 = it->second.controlPoints.begin(); it2 != it->second.controlPoints.end(); ++it2) {
                BezierCPPtr cp( new BezierCP(this_shared) );
                cp->fromSerialization(it2->innerPoint);
                shape.points.push_back(cp);

                if (useFeather) {
                    BezierCPPtr fp( new FeatherPoint(this_shared) );
                    if (it2->featherPoint) {
                        fp->fromSerialization(*it2->featherPoint);
                    } else {
                        fp->fromSerialization(it2->innerPoint);
                    }
                    shape.featherPoints.push_back(fp);
                }
            }
        } // for all views

    }
    evaluateCurveModified();
    RotoDrawableItem::fromSerialization(obj);
}


void
Bezier::point_line_intersection(const Point &p1,
                                const Point &p2,
                                const Point &pos,
                                int *winding)
{
    double x1 = p1.x;
    double y1 = p1.y;
    double x2 = p2.x;
    double y2 = p2.y;
    double y = pos.y;
    int dir = 1;

    if ( qFuzzyCompare(y1, y2) ) {
        // ignore horizontal lines according to scan conversion rule
        return;
    } else if (y2 < y1) {
        double x_tmp = x2; x2 = x1; x1 = x_tmp;
        double y_tmp = y2; y2 = y1; y1 = y_tmp;
        dir = -1;
    }

    if ( (y >= y1) && (y < y2) ) {
        double x = x1 + ( (x2 - x1) / (y2 - y1) ) * (y - y1);

        // count up the winding number if we're
        if (x <= pos.x) {
            (*winding) += dir;
        }
    }
}

/*
 The algorithm to know which side is the outside of a polygon consists in computing the global polygon orientation.
 To compute the orientation, compute its surface. If positive the polygon is clockwise, if negative it's counterclockwise.
 to compute the surface, take the starting point of the polygon, and imagine a fan made of all the triangles
 pointing at this point. The surface of a tringle is half the cross-product of two of its sides issued from
 the same point (the starting point of the polygon, in this case.
 The orientation of a polygon has to be computed only once for each modification of the polygon (whenever it's edited), and
 should be stored with the polygon.
 Of course an 8-shaped polygon doesn't have an outside, but it still has an orientation. The feather direction
 should follow this orientation.
 */
bool
Bezier::isClockwiseOriented(TimeValue time, ViewIdx view) const
{
    ViewIdx view_i = checkIfViewExistsOrFallbackMainView(view);
    QMutexLocker k(&_imp->itemMutex);
    const BezierShape* shape = _imp->getViewShape(view_i);
    if (!shape) {
        return false;
    }

    if (shape->points.size() <= 1) {
        return false;
    } 

    const BezierCPs& cps = shape->points;
    double polygonSurface = 0.;
    std::vector<Point> allPoints;



    {
        BezierCPs::const_iterator it = cps.begin();
        BezierCPs::const_iterator next = it;
        if ( next != cps.end() ) {
            ++next;
        }
        

        for (; next != cps.end(); ++it, ++next) {
            assert( it != cps.end() );

            // We skip the first point because this is the center of the fan for the U and V vector to compute the area
            if (it != cps.begin()) {
                Point p0;
                (*it)->getPositionAtTime(time, &p0.x, &p0.y);
                allPoints.push_back(p0);
            }

            Point p1, p2;
            (*it)->getRightBezierPointAtTime(time, &p1.x, &p1.y);
            (*next)->getLeftBezierPointAtTime(time, &p2.x, &p2.y);

            allPoints.push_back(p1);
            allPoints.push_back(p2);

            // If we are a closed Bezier or we are not on the last segment, remove the last point so we don't add duplicates
            if (shape->finished && next == cps.end()) {
                Point p3;
                (*next)->getPositionAtTime(time, &p3.x, &p3.y);
                allPoints.push_back(p3);
            }

        } // for each control point
    }

    Point originalPoint;
    {
        BezierCPs::const_iterator it = cps.begin();
        (*it)->getPositionAtTime(time, &originalPoint.x, &originalPoint.y);
    }
    
    {
        std::vector<Point>::const_iterator it = allPoints.begin();
        std::vector<Point>::const_iterator next = it;
        if ( next != allPoints.end() ) {
            ++next;
        }
        for (; it != allPoints.end(); ++it, ++next) {

            if (next == allPoints.end()) {
                next = allPoints.begin();
            }
            Point u;
            u.x = it->x - originalPoint.x;
            u.y = it->y - originalPoint.y;


            Point v;
            v.x = next->x - originalPoint.x;
            v.y = next->y - originalPoint.y;

            //This is the area of the parallelogram defined by the U and V sides
            //Since a triangle is half a parallelogram, just half the cross-product
            double crossProduct = v.y * u.x - v.x * u.y;
            polygonSurface += (crossProduct / 2.);
            
        }
    }
    return polygonSurface < 0;


} // isFeatherPolygonClockwiseOriented


/**
 * @brief Computes the location of the feather extent relative to the current feather point position and
 * the given feather distance.
 * In the case the control point and the feather point of the Bezier are distinct, this function just makes use
 * of Thales theorem.
 * If the feather point and the control point are equal then this function computes the left and right derivative
 * of the Bezier at that point to determine the direction in which the extent is.
 * @returns The delta from the given feather point to apply to find out the extent position.
 *
 * Note that the delta will be applied to fp.
 **/
void
Bezier::expandToFeatherDistance(const Point & cp, //< the point
                                Point* fp, //< the feather point (input/output)
                                double featherDistance_x,         //< feather distance
                                double featherDistance_y,         //< feather distance
                                TimeValue time, //< time
                                bool clockWise, //< is the Bezier  clockwise oriented or not
                                const Transform::Matrix3x3& transform,
                                BezierCPs::const_iterator prevFp, //< iterator pointing to the feather before curFp
                                BezierCPs::const_iterator curFp, //< iterator pointing to fp
                                BezierCPs::const_iterator nextFp) //< iterator pointing after curFp
{
    if (featherDistance_x == 0 && featherDistance_y == 0) {
        return;
    }
    Point delta;
    ///shortcut when the feather point is different than the control point
    if ( (cp.x != fp->x) && (cp.y != fp->y) ) {
        double dx = (fp->x - cp.x);
        double dy = (fp->y - cp.y);
        double dist = sqrt(dx * dx + dy * dy);
        delta.x = ( dx * (dist + featherDistance_x) ) / dist;
        delta.y = ( dy * (dist + featherDistance_y) ) / dist;
        fp->x =  delta.x + cp.x;
        fp->y =  delta.y + cp.y;
    } else {
        //compute derivatives to determine the feather extent
        double leftX, leftY, rightX, rightY, norm;
        Bezier::leftDerivativeAtPoint(time, **curFp, **prevFp, transform, &leftX, &leftY);
        Bezier::rightDerivativeAtPoint(time, **curFp, **nextFp, transform, &rightX, &rightY);
        norm = sqrt( (rightX - leftX) * (rightX - leftX) + (rightY - leftY) * (rightY - leftY) );

        ///normalize derivatives by their norm
        if (norm != 0) {
            delta.x = -( (rightY - leftY) / norm );
            delta.y = ( (rightX - leftX) / norm );
        } else {
            ///both derivatives are the same, use the direction of the left one
            norm = sqrt( (leftX - cp.x) * (leftX - cp.x) + (leftY - cp.y) * (leftY - cp.y) );
            if (norm != 0) {
                delta.x = -( (leftY - cp.y) / norm );
                delta.y = ( (leftX - cp.x) / norm );
            } else {
                ///both derivatives and control point are equal, just use 0
                delta.x = delta.y = 0;
            }
        }

        delta.x *= featherDistance_x;
        delta.y *= featherDistance_y;

        if (clockWise) {
            fp->x = cp.x + delta.x;
            fp->y = cp.y + delta.y;
        } else {
            fp->x = cp.x - delta.x;
            fp->y = cp.y - delta.y;
        }
    }

} // expandToFeatherDistance


void
Bezier::appendToHash(const ComputeHashArgs& args, Hash64* hash)
{
    if (args.hashType != HashableObject::eComputeHashTypeTimeViewVariant) {
        return;
    }
    std::list<BezierCPPtr> cps = getControlPoints(args.view);
    std::list<BezierCPPtr> fps = getFeatherPoints(args.view);
    assert(cps.size() == fps.size() || fps.empty());

    if (!cps.empty()) {

        if (!_imp->isOpenBezier) {
            hash->append(isCurveFinished(args.view));
        }

        std::list<BezierCPPtr>::const_iterator fIt = fps.begin();
        for (std::list<BezierCPPtr>::const_iterator it = cps.begin(); it!=cps.end(); ++it, ++fIt) {
            double x, y, lx, ly, rx, ry;
            (*it)->getPositionAtTime(args.time, &x, &y);
            (*it)->getLeftBezierPointAtTime(args.time, &lx, &ly);
            (*it)->getRightBezierPointAtTime(args.time, &rx, &ry);
            double fx, fy, flx, fly, frx, fry;
            (*fIt)->getPositionAtTime(args.time, &fx, &fy);
            (*fIt)->getLeftBezierPointAtTime(args.time, &flx, &fly);
            (*fIt)->getRightBezierPointAtTime(args.time, &frx, &fry);

            hash->append(x);
            hash->append(y);
            hash->append(lx);
            hash->append(ly);
            hash->append(rx);
            hash->append(ry);

            // Only add feather if different
            if (x != fx || y != fy || lx != flx || ly != fly || rx != frx || ry != fry) {
                hash->append(fx);
                hash->append(fy);
                hash->append(flx);
                hash->append(fly);
                hash->append(frx);
                hash->append(fry);
            }
        }
    }
    RotoDrawableItem::appendToHash(args, hash);
} // appendToHash

std::string
Bezier::getBaseItemName() const
{
    return _imp->baseName;
}

std::string
Bezier::getSerializationClassName() const
{
    return _imp->isOpenBezier ? kSerializationOpenedBezierTag : kSerializationClosedBezierTag;
}

void
Bezier::initializeKnobs()
{
    RotoDrawableItem::initializeKnobs();

    _imp->feather = createDuplicateOfTableKnob<KnobDouble>(kRotoFeatherParam);
    _imp->featherFallOff = createDuplicateOfTableKnob<KnobDouble>(kRotoFeatherFallOffParam);
    _imp->fallOffRampType = createDuplicateOfTableKnob<KnobChoice>(kRotoFeatherFallOffType);
    if (!_imp->isOpenBezier) {
        _imp->fillShapeKnob = createDuplicateOfTableKnob<KnobBool>(kBezierParamFillShape);
    }
    _imp->shapeKeys = createDuplicateOfTableKnob<KnobKeyFrameMarkers>(kRotoShapeUserKeyframesParam);

    BezierPtr thisShared = toBezier(shared_from_this());
    _imp->shapeKeys.lock()->getAnimationCurve(ViewIdx(0), DimIdx(0))->registerListener(thisShared);

} // initializeKnobs

void
Bezier::fetchRenderCloneKnobs()
{
    // Only copy the range needed by the motion blur samples
    RangeD range;
    int nDivisions;
    getMotionBlurSettings(getCurrentRenderTime(), getCurrentRenderView(), &range, &nDivisions);

    // If motion blur is enabled, only clone
    {
        BezierPtr mainInstance = toBezier(getMainInstance());
        QMutexLocker k(&mainInstance->_imp->itemMutex);
        for (PerViewBezierShapeMap::const_iterator it = mainInstance->_imp->viewShapes.begin(); it != mainInstance->_imp->viewShapes.end(); ++it) {
            BezierShape& thisShape = _imp->viewShapes[it->first];
            thisShape.finished = it->second.finished;
            for (BezierCPs::const_iterator it2 = it->second.points.begin(); it2 != it->second.points.end(); ++it2) {
                BezierCPPtr copy(new BezierCP());
                copy->copyControlPoint(**it2, &range);
                thisShape.points.push_back(copy);
            }
            for (BezierCPs::const_iterator it2 = it->second.featherPoints.begin(); it2 != it->second.featherPoints.end(); ++it2) {
                BezierCPPtr copy(new BezierCP());
                copy->copyControlPoint(**it2, &range);
                thisShape.featherPoints.push_back(copy);
            }
            
        }
    }

    RotoDrawableItem::fetchRenderCloneKnobs();
    _imp->feather = getKnobByNameAndType<KnobDouble>(kRotoFeatherParam);
    _imp->featherFallOff = getKnobByNameAndType<KnobDouble>(kRotoFeatherFallOffParam);
    _imp->fallOffRampType = getKnobByNameAndType<KnobChoice>(kRotoFeatherFallOffType);
    if (!_imp->isOpenBezier) {
        _imp->fillShapeKnob = getKnobByNameAndType<KnobBool>(kBezierParamFillShape);
    }
    _imp->shapeKeys = getKnobByNameAndType<KnobKeyFrameMarkers>(kRotoShapeUserKeyframesParam);

}

KnobDoublePtr
Bezier::getFeatherKnob() const
{
    return _imp->feather.lock();
}

KnobDoublePtr
Bezier::getFeatherFallOffKnob() const
{
    return _imp->featherFallOff.lock();
}

KnobChoicePtr
Bezier::getFallOffRampTypeKnob() const
{
    return _imp->fallOffRampType.lock();
}


NATRON_NAMESPACE_EXIT


NATRON_NAMESPACE_USING
#include "moc_Bezier.cpp"
