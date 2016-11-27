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

#include "Bezier.h"

#include <algorithm> // min, max
#include <sstream>
#include <locale>
#include <limits>
#include <cmath>
#include <cassert>
#include <stdexcept>

#include <QtCore/QLineF>
#include <QtCore/QDebug>

GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
// /usr/local/include/boost/bind/arg.hpp:37:9: warning: unused typedef 'boost_static_assert_typedef_37' [-Wunused-local-typedef]
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/math/special_functions/fpclassify.hpp>
#include <boost/algorithm/string/predicate.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON



#include "Global/MemoryInfo.h"

#include "Engine/AppInstance.h"
#include "Engine/BezierCP.h"
#include "Engine/FeatherPoint.h"
#include "Engine/Interpolation.h"
#include "Engine/TimeLine.h"
#include "Engine/Image.h"
#include "Engine/ImageParams.h"
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


NATRON_NAMESPACE_ENTER;

struct BezierShape
{
    BezierCPs points; //< the control points of the curve
    BezierCPs featherPoints; //< the feather points, the number of feather points must equal the number of cp.

    //updated whenever the Bezier is edited, this is used to determine if a point lies inside the bezier or not
    //it has a value for each keyframe
    mutable std::map<double, bool> isClockwiseOriented;
    mutable bool isClockwiseOrientedStatic; //< used when the bezier has no keyframes
    bool finished; //< when finished is true, the last point of the list is connected to the first point of the list.

    BezierShape()
    : points()
    , featherPoints()
    , isClockwiseOriented()
    , isClockwiseOrientedStatic(false)
    , finished(false)
    {

    }
};

typedef std::map<ViewIdx, BezierShape> PerViewBezierShapeMap;

struct BezierPrivate
{
    mutable QMutex itemMutex; //< protects points & featherPoits
    PerViewBezierShapeMap viewShapes;

    bool autoRecomputeOrientation; // when true, orientation will be computed automatically on editing
    bool isOpenBezier; // when true the bezier will be rendered even if not closed

    std::string baseName;

    KnobDoubleWPtr feather; //< number of pixels to add to the feather distance (from the feather point), between -100 and 100
    KnobDoubleWPtr featherFallOff; //< the rate of fall-off for the feather, between 0 and 1,  0.5 meaning the
    //alpha value is half the original value when at half distance from the feather distance
    KnobChoiceWPtr fallOffRampType;

    KnobDoubleWPtr motionBlurKnob;
    KnobDoubleWPtr shutterKnob;
    KnobChoiceWPtr shutterTypeKnob;
    KnobDoubleWPtr customOffsetKnob;


    BezierPrivate(const std::string& baseName, bool isOpenBezier)
    : itemMutex()
    , viewShapes()
    , autoRecomputeOrientation(true)
    , isOpenBezier(isOpenBezier)
    , baseName(baseName)
    {
        viewShapes.insert(std::make_pair(ViewIdx(0), BezierShape()));
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
                                                     double time,
                                                     const BezierShape& shape,
                                                     const Transform::Matrix3x3& transform,
                                                     int* index) const;
    
    BezierCPs::const_iterator findFeatherPointNearby(double x,
                                                     double y,
                                                     double acceptance,
                                                     double time,
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

Bezier::Bezier(const Bezier& other)
: RotoDrawableItem(other.getModel())
, _imp( new BezierPrivate(other.getBaseItemName(), other.isOpenBezier()) )
{

}

bool
Bezier::isOpenBezier() const
{
    return _imp->isOpenBezier;
}


Bezier::~Bezier()
{
}

RotoStrokeType
Bezier::getBrushType() const
{
    return eRotoStrokeTypeSolid;
}

bool
Bezier::isAutoKeyingEnabled() const
{
    KnobItemsTablePtr model = getModel();
    if (!model) {
        return false;
    }
    KnobButtonPtr knob = toKnobButton(model->getNode()->getEffectInstance()->getKnobByName(kRotoUIParamAutoKeyingEnabled));
    if (!knob) {
        return false;
    }
    return knob->getValue();
}

bool
Bezier::isFeatherLinkEnabled() const
{
    KnobItemsTablePtr model = getModel();
    if (!model) {
        return false;
    }
    KnobButtonPtr knob = toKnobButton(model->getNode()->getEffectInstance()->getKnobByName(kRotoUIParamFeatherLinkEnabled));
    if (!knob) {
        return false;
    }
    return knob->getValue();
}

bool
Bezier::isRippleEditEnabled() const
{
    KnobItemsTablePtr model = getModel();
    if (!model) {
        return false;
    }
    KnobButtonPtr knob = toKnobButton(model->getNode()->getEffectInstance()->getKnobByName(kRotoUIParamRippleEdit));
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
                                      double time,
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
                                      double time,
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

// updates param bbox with the bbox of this segment
void
Bezier::bezierPointBboxUpdate(const Point & p0,
                              const Point & p1,
                              const Point & p2,
                              const Point & p3,
                              RectD *bbox,
                              bool* bboxSet) ///< input/output
{
    {
        double x1, x2;
        bezierBounds(p0.x, p1.x, p2.x, p3.x, &x1, &x2);
        if (bboxSet && !*bboxSet) {
            bbox->x1 = x1;
            bbox->x2 = x2;
        } else {
            if (x1 < bbox->x1) {
                bbox->x1 = x1;
            }
            if (x2 > bbox->x2) {
                bbox->x2 = x2;
            }
        }
    }
    {
        double y1, y2;
        bezierBounds(p0.y, p1.y, p2.y, p3.y, &y1, &y2);
        if (bboxSet && !*bboxSet) {
            bbox->y1 = y1;
            bbox->y2 = y2;
        } else {
            if (y1 < bbox->y1) {
                bbox->y1 = y1;
            }
            if (y2 > bbox->y2) {
                bbox->y2 = y2;
            }
        }
    }
    if (bboxSet && !*bboxSet) {
        *bboxSet = true;
    }
}

// compute a bounding box for the bezier segment
// algorithm:
// - compute extrema of the cubic, i.e. values of t for
// which the derivative of the x or y coordinate of the
// Bezier is 0. If they are in [0,1] then they take part in
// bbox computation (there can be up to four extrema, 2 for
// x and 2 for y). the Bbox is the Bbox of these points and the
// extremal points (P0,P3)
static void
bezierSegmentBboxUpdate(const BezierCP & first,
                        const BezierCP & last,
                        double time,
                        unsigned int mipMapLevel,
                        const Transform::Matrix3x3& transform,
                        RectD* bbox,
                        bool *bboxSet) ///< input/output
{
    Point p0, p1, p2, p3;
    Transform::Point3D p0M, p1M, p2M, p3M;

    assert(bbox);

    try {
        first.getPositionAtTime(time, &p0M.x, &p0M.y);
        first.getRightBezierPointAtTime(time, &p1M.x, &p1M.y);
        last.getPositionAtTime(time, &p3M.x, &p3M.y);
        last.getLeftBezierPointAtTime(time, &p2M.x, &p2M.y);
    } catch (const std::exception & e) {
        assert(false);
    }

    p0M.z = p1M.z = p2M.z = p3M.z = 1;

    p0M = Transform::matApply(transform, p0M);
    p1M = Transform::matApply(transform, p1M);
    p2M = Transform::matApply(transform, p2M);
    p3M = Transform::matApply(transform, p3M);

    p0.x = p0M.x / p0M.z;
    p1.x = p1M.x / p1M.z;
    p2.x = p2M.x / p2M.z;
    p3.x = p3M.x / p3M.z;

    p0.y = p0M.y / p0M.z;
    p1.y = p1M.y / p1M.z;
    p2.y = p2M.y / p2M.z;
    p3.y = p3M.y / p3M.z;

    if (mipMapLevel > 0) {
        int pot = 1 << mipMapLevel;
        p0.x /= pot;
        p0.y /= pot;

        p1.x /= pot;
        p1.y /= pot;

        p2.x /= pot;
        p2.y /= pot;

        p3.x /= pot;
        p3.y /= pot;
    }
    Bezier::bezierPointBboxUpdate(p0, p1, p2, p3, bbox, bboxSet);
}

void
Bezier::bezierSegmentListBboxUpdate(const BezierCPs & points,
                                    bool finished,
                                    bool isOpenBezier,
                                    double time,
                                    unsigned int mipMapLevel,
                                    const Transform::Matrix3x3& transform,
                                    RectD* bbox) ///< input/output
{
    if ( points.empty() ) {
        return;
    }
    if (points.size() == 1) {
        // only one point
        Transform::Point3D p0;
        const BezierCPPtr& p = points.front();
        p->getPositionAtTime(time, &p0.x, &p0.y);
        p0.z = 1;
        p0 = Transform::matApply(transform, p0);
        bbox->x1 = p0.x;
        bbox->x2 = p0.x;
        bbox->y1 = p0.y;
        bbox->y2 = p0.y;

        return;
    }
    BezierCPs::const_iterator next = points.begin();
    if ( next != points.end() ) {
        ++next;
    }
    bool bboxSet = false;
    for (BezierCPs::const_iterator it = points.begin(); it != points.end(); ++it) {
        if ( next == points.end() ) {
            if (!finished && !isOpenBezier) {
                break;
            }
            next = points.begin();
        }
        bezierSegmentBboxUpdate(*(*it), *(*next), time, mipMapLevel, transform, bbox, &bboxSet);

        // increment for next iteration
        if ( next != points.end() ) {
            ++next;
        }
    } // for()
}

inline double euclDist(double x1, double y1, double x2, double y2)
{
    double dx = x2 - x1;
    double dy = y2 - y1;
    return dx * dx + dy * dy;
}


inline void addPointConditionnally(const Point& p, double t, std::list< ParametricPoint >* points)
{
    if (points->empty()) {
        ParametricPoint x;
        x.x = p.x;
        x.y = p.y;
        x.t = t;
        points->push_back(x);
    } else {
        const ParametricPoint& b = points->back();
        if (b.x != p.x || b.y != p.y) {
            ParametricPoint x;
            x.x = p.x;
            x.y = p.y;
            x.t = t;
            points->push_back(x);
        }
    }
}

#ifndef ROTO_BEZIER_EVAL_ITERATIVE
/**
 * @brief Recursively subdivide the bezier segment p0,p1,p2,p3 until the cubic curve is assumed to be flat. The errorScale is used to determine the stopping criterion.
 * The greater it is, the smoother the curve will be.
 **/
static void
recursiveBezierInternal(const Point& p0, const Point& p1, const Point& p2, const Point& p3,
                        double t_p0, double t_p1, double t_p2, double t_p3,
                        double errorScale, int recursionLevel, int maxRecursion, std::list< ParametricPoint >* points)
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
                    addPointConditionnally(p1, t_p1, points);
                    return;
                }
            } else {
                if (d3 < distanceToleranceSquare) {
                    addPointConditionnally(p2, t_p2, points);
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
                    addPointConditionnally(p, t_p23, points);
                    return;
                }

                // Check Angle
                da1 = std::fabs(std::atan2(p3.y - p2.y, p3.x - p2.x) - std::atan2(p2.y - p1.y, p2.x - p1.x));
                if (da1 >= M_PI) {
                    da1 = 2. * M_PI - da1;
                }

                if (da1 < angleToleranceMax) {
                    addPointConditionnally(p1, t_p1, points);
                    addPointConditionnally(p2, t_p2, points);
                    return;
                }

                if (cuspTolerance != 0.0) {
                    if (da1 > cuspTolerance) {
                        addPointConditionnally(p2, t_p2, points);
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
                    addPointConditionnally(p, t_p23, points);
                    return;
                }
                // Check Angle
                da1 = std::fabs(std::atan2(p2.y - p1.y, p2.x - p1.x) - std::atan2(p1.y - p0.y, p1.x - p0.x));
                if (da1 >= M_PI) {
                    da1 = 2 * M_PI - da1;
                }

                if (da1 < angleToleranceMax) {
                    addPointConditionnally(p1, t_p1, points);
                    addPointConditionnally(p2, t_p2, points);
                    return;
                }

                if (cuspTolerance != 0.0) {
                    if (da1 > cuspTolerance) {
                        addPointConditionnally(p1, t_p1, points);
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
                    addPointConditionnally(p, t_p23, points);
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
                    addPointConditionnally(p, t_p23, points);
                    return;
                }

                if (cuspTolerance != 0.0) {
                    if (da1 > cuspTolerance) {
                        addPointConditionnally(p1, t_p1, points);
                        return;
                    }

                    if (da2 > cuspTolerance) {
                        addPointConditionnally(p2, t_p2, points);
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


    recursiveBezierInternal(p0, p12, p123, p1234, t_p0, t_p12, t_p123, t_p1234, errorScale, recursionLevel + 1, maxRecursion, points);
    recursiveBezierInternal(p1234, p234, p34, p3, t_p1234, t_p234, t_p34, t_p3, errorScale, recursionLevel + 1, maxRecursion, points);
}

static void
recursiveBezier(const Point& p0, const Point& p1, const Point& p2, const Point& p3, double errorScale, int maxRecursion, std::list< ParametricPoint >* points)
{
    ParametricPoint p0x,p3x;
    p0x.x = p0.x;
    p0x.y = p0.y;
    p0x.t = 0.;
    p3x.x = p3.x;
    p3x.y = p3.y;
    p3x.t = 1.;
    points->push_back(p0x);
    recursiveBezierInternal(p0, p1, p2, p3, 0., 1. / 3., 2. / 3., 1., errorScale, 0, maxRecursion, points);
    points->push_back(p3x);
}
#endif // #ifdef ROTO_BEZIER_EVAL_ITERATIVE

// compute nbPointsperSegment points and update the bbox bounding box for the Bezier
// segment from 'first' to 'last' evaluated at 'time'
// If nbPointsPerSegment is -1 then it will be automatically computed
static void
bezierSegmentEval(const BezierCP & first,
                  const BezierCP & last,
                  double time,
                  unsigned int mipMapLevel,
#ifdef ROTO_BEZIER_EVAL_ITERATIVE
                  int nbPointsPerSegment,
#else
                  double errorScale,
#endif
                  const Transform::Matrix3x3& transform,
                  std::vector< ParametricPoint >* points, ///< output
                  RectD* bbox = NULL,
                  bool* bboxSet = NULL) ///< input/output (optional)
{
    Transform::Point3D p0M, p1M, p2M, p3M;
    Point p0, p1, p2, p3;

    try {
        first.getPositionAtTime(time, &p0M.x, &p0M.y);
        first.getRightBezierPointAtTime(time, &p1M.x, &p1M.y);
        last.getPositionAtTime(time, &p3M.x, &p3M.y);
        last.getLeftBezierPointAtTime(time, &p2M.x, &p2M.y);
    } catch (const std::exception & e) {
        assert(false);
    }

    p0M.z = p1M.z = p2M.z = p3M.z = 1;

    p0M = matApply(transform, p0M);
    p1M = matApply(transform, p1M);
    p2M = matApply(transform, p2M);
    p3M = matApply(transform, p3M);

    p0.x = p0M.x / p0M.z; p0.y = p0M.y / p0M.z;
    p1.x = p1M.x / p1M.z; p1.y = p1M.y / p1M.z;
    p2.x = p2M.x / p2M.z; p2.y = p2M.y / p2M.z;
    p3.x = p3M.x / p3M.z; p3.y = p3M.y / p3M.z;


    if (mipMapLevel > 0) {
        int pot = 1 << mipMapLevel;
        p0.x /= pot;
        p0.y /= pot;

        p1.x /= pot;
        p1.y /= pot;

        p2.x /= pot;
        p2.y /= pot;

        p3.x /= pot;
        p3.y /= pot;
    }

#ifdef ROTO_BEZIER_EVAL_ITERATIVE
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
    for (int i = 0; i < nbPointsPerSegment; ++i) {
        ParametricPoint p;
        p.t = incr * i;
        Bezier::bezierPoint(p0, p1, p2, p3, p.t, &cur);
        p.x = cur.x;
        p.y = cur.y;
        points->push_back(p);
    }
#else
    static const int maxRecursion = 32;
    recursiveBezier(p0, p1, p2, p3, errorScale, maxRecursion, points);
#endif
    if (bbox) {
        Bezier::bezierPointBboxUpdate(p0,  p1,  p2,  p3, bbox, bboxSet);
    }
} // bezierSegmentEval

/**
 * @brief Determines if the point (x,y) lies on the bezier curve segment defined by first and last.
 * @returns True if the point is close (according to the acceptance) to the curve, false otherwise.
 * @param param[out] It is set to the parametric value at which the subdivision of the bezier segment
 * yields the closest point to (x,y) on the curve.
 **/
static bool
bezierSegmentMeetsPoint(const BezierCP & first,
                        const BezierCP & last,
                        const Transform::Matrix3x3& transform,
                        double time,
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
isPointCloseTo(double time,
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

static bool
bezierSegmenEqual(double time,
                  const BezierCP & p0,
                  const BezierCP & p1,
                  const BezierCP & s0,
                  const BezierCP & s1)
{
    double prevX, prevY, prevXF, prevYF;
    double nextX, nextY, nextXF, nextYF;

    p0.getPositionAtTime(time, &prevX, &prevY);
    p1.getPositionAtTime(time, &nextX, &nextY);
    s0.getPositionAtTime(time, &prevXF, &prevYF);
    s1.getPositionAtTime(time, &nextXF, &nextYF);
    if ( (prevX != prevXF) || (prevY != prevYF) || (nextX != nextXF) || (nextY != nextYF) ) {
        return true;
    } else {
        ///check derivatives
        double prevRightX, prevRightY, nextLeftX, nextLeftY;
        double prevRightXF, prevRightYF, nextLeftXF, nextLeftYF;
        p0.getRightBezierPointAtTime(time, &prevRightX, &prevRightY);
        p1.getLeftBezierPointAtTime(time, &nextLeftX, &nextLeftY);
        s0.getRightBezierPointAtTime(time, &prevRightXF, &prevRightYF);
        s1.getLeftBezierPointAtTime(time, &nextLeftXF, &nextLeftYF);
        if ( (prevRightX != prevRightXF) || (prevRightY != prevRightYF) || (nextLeftX != nextLeftXF) || (nextLeftY != nextLeftYF) ) {
            return true;
        } else {
            return false;
        }
    }
}


class IsRenderingFlagLocker
{
    NodePtr effectNode;
    EnsureOperationOutOfRender* _locker;
public:

    IsRenderingFlagLocker(const Bezier* bezier)
    : effectNode()
    , _locker(0)
    {
        NodePtr mask = bezier->getMaskNode();
        NodePtr effect = bezier->getEffectNode();
        if (mask) {
            effectNode = mask;
        } else if (effect) {
            effectNode = effect;
        }
        if (effectNode) {
            _locker = new EnsureOperationOutOfRender(effectNode);
        }
    }

    bool isRendering() const
    {
        if (_locker) {
            return _locker->isNodeRendering();
        } else {
            return false;
        }
    }

    ~IsRenderingFlagLocker()
    {
        delete _locker;
    }

};

void
Bezier::clearAllPoints()
{
    removeAnimation(ViewSetSpec::all(), DimSpec::all());
    QMutexLocker k(&_imp->itemMutex);
    for (PerViewBezierShapeMap::iterator it = _imp->viewShapes.begin(); it != _imp->viewShapes.end(); ++it) {
        it->second.points.clear();
        it->second.featherPoints.clear();
        it->second.finished = false;
        it->second.isClockwiseOriented.clear();
        it->second.isClockwiseOrientedStatic = false;
    }
}

void
Bezier::copyItem(const KnobTableItemPtr& other)
{
    BezierPtr this_shared = toBezier( shared_from_this() );
    assert(this_shared);

    BezierPtr otherBezier = toBezier(other);
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

BezierCPPtr
Bezier::addControlPointInternal(double x, double y, double time, ViewIdx view)
{
    if ( isCurveFinished(view) ) {
        return BezierCPPtr();
    }

    double keyframeTime;
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
            if (!getMasterKeyframe(0, view, &k)) {
                keyframeTime = getApp()->getTimeLine()->currentFrame();;
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
                        double time,
                        ViewSetSpec view)
{
    BezierCPPtr ret;
    if (view.isAll()) {
        std::list<ViewIdx> views = getViewsList();
        for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
            ret = addControlPointInternal(x, y, time, *it);
        }
    } else {
        ViewIdx view_i = getViewIdxFromGetSpec(ViewGetSpec(view));
        ret = addControlPointInternal(x, y, time, view_i);
    }
    evaluateCurveModified();

    return ret;
} // Bezier::addControlPoint

void
Bezier::evaluateCurveModified()
{
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
        std::set<double> existingKeyframes;
        getMasterKeyFrameTimes(view, &existingKeyframes);

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


        for (std::set<double>::iterator it = existingKeyframes.begin(); it != existingKeyframes.end(); ++it) {
            Point p0, p1, p2, p3;
            (*prev)->getPositionAtTime(*it, &p0.x, &p0.y);
            (*prev)->getRightBezierPointAtTime( *it, &p1.x, &p1.y);
            (*next)->getPositionAtTime(*it, &p3.x, &p3.y);
            (*next)->getLeftBezierPointAtTime(*it, &p2.x, &p2.y);

            Point p0f;
            Point p1f;
            if (useFeatherPoints() && prevF != shape->featherPoints.end() && *prevF) {
                (*prevF)->getPositionAtTime(*it, &p0f.x, &p0f.y);
                (*prevF)->getRightBezierPointAtTime(*it, &p1f.x, &p1f.y);
            } else {
                p0f = p0;
                p1f = p1;
            }
            Point p2f;
            Point p3f;
            if (useFeatherPoints() && nextF != shape->featherPoints.end() && *nextF) {
                (*nextF)->getPositionAtTime(*it, &p3f.x, &p3f.y);
                (*nextF)->getLeftBezierPointAtTime(*it, &p2f.x, &p2f.y);
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
            (*prev)->setRightBezierPointAtTime(*it, p0p1.x, p0p1.y);
            (*next)->setLeftBezierPointAtTime(*it, p2p3.x, p2p3.y);

            if ( useFeatherPoints() ) {
                if (prevF != shape->featherPoints.end() && *prevF) {
                    (*prevF)->setRightBezierPointAtTime(*it, p0p1f.x, p0p1f.y);
                }
                if (nextF != shape->featherPoints.end() && *nextF) {
                    (*nextF)->setLeftBezierPointAtTime(*it, p2p3f.x, p2p3f.y);
                }
            }


            p->setPositionAtTime(*it, dest.x, dest.y);
            ///The left control point of p is p0p1_p1p2 and the right control point is p1p2_p2p3
            p->setLeftBezierPointAtTime(*it, p0p1_p1p2.x, p0p1_p1p2.y);
            p->setRightBezierPointAtTime(*it, p1p2_p2p3.x, p1p2_p2p3.y);

            if ( useFeatherPoints() ) {
                fp->setPositionAtTime(*it, destf.x, destf.y);
                fp->setLeftBezierPointAtTime(*it, p0p1_p1p2f.x, p0p1_p1p2f.y);
                fp->setRightBezierPointAtTime(*it, p1p2_p2p3f.x, p1p2_p2p3f.y);
            }
        }

        ///if there's no keyframes
        if ( existingKeyframes.empty() ) {
            Point p0, p1, p2, p3;

            (*prev)->getPositionAtTime(0, &p0.x, &p0.y);
            (*prev)->getRightBezierPointAtTime(0, &p1.x, &p1.y);
            (*next)->getPositionAtTime(0, &p3.x, &p3.y);
            (*next)->getLeftBezierPointAtTime(0, &p2.x, &p2.y);


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


        ///If auto-keying is enabled, set a new keyframe
        int currentTime = getApp()->getTimeLine()->currentFrame();
        if ( !hasMasterKeyframeAtTime(currentTime, view) && isAutoKeyingEnabled() ) {
            setKeyFrame(currentTime, view, 0);
        }
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
        ViewIdx view_i = getViewIdxFromGetSpec(ViewGetSpec(view));
        ret = addControlPointAfterIndexInternal(index, t, view_i);
    }
    evaluateCurveModified();

    return ret;

} // addControlPointAfterIndex

int
Bezier::getControlPointsCount(ViewGetSpec view) const
{
    ViewIdx view_i = getViewIdxFromGetSpec(view);
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
                       double time,
                       ViewGetSpec view,
                       double *t,
                       bool* feather) const
{

    ViewIdx view_i = getViewIdxFromGetSpec(view);


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
            ViewIdx view_i = getViewIdxFromGetSpec(ViewGetSpec(view));
            QMutexLocker l(&_imp->itemMutex);
            BezierShape* shape = _imp->getViewShape(view_i);
            if (!shape) {
                return;
            }

            shape->finished = finished;
        }
        
    }
    
    resetTransformCenter();
    refreshPolygonOrientation(false, view);
    evaluateCurveModified();
}

bool
Bezier::isCurveFinished(ViewGetSpec view) const
{
    ViewIdx view_i = getViewIdxFromGetSpec(view);
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
    BezierShape* shape = _imp->getViewShape(view);
    if (!shape) {
        return;
    }
    {
        QMutexLocker l(&_imp->itemMutex);
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
        ViewIdx view_i = getViewIdxFromGetSpec(ViewGetSpec(view));
        removeControlPointByIndexInternal(index, view_i);
    }

    refreshPolygonOrientation(false, view);
    evaluateCurveModified();
}

void
Bezier::movePointByIndexInternalForView(int index, double time, ViewIdx view, double dx, double dy, bool onlyFeather)
{
    bool rippleEdit = isRippleEditEnabled();
    bool autoKeying = isAutoKeyingEnabled();
    bool fLinkEnabled = ( onlyFeather ? true : isFeatherLinkEnabled() );
    bool keySet = false;
    Transform::Matrix3x3 trans, invTrans;
    getTransformAtTime(time, view, &trans);
    invTrans = Transform::matInverse(trans);

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
            if (!isOnKeyframe) {
                keySet = true;
            }
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
            std::set<double> keyframes;
            getMasterKeyFrameTimes(view, &keyframes);
            for (std::set<double>::iterator it2 = keyframes.begin(); it2 != keyframes.end(); ++it2) {
                if (*it2 == time) {
                    continue;
                }
                if (!onlyFeather) {
                    assert(cp);
                    cp->getPositionAtTime(*it2, &p.x, &p.y);
                    cp->getLeftBezierPointAtTime(*it2,  &left.x, &left.y);
                    cp->getRightBezierPointAtTime(*it2,  &right.x, &right.y);

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


                    cp->setPositionAtTime(*it2, p.x, p.y );
                    cp->setLeftBezierPointAtTime(*it2, left.x, left.y);
                    cp->setRightBezierPointAtTime(*it2, right.x, right.y);
                }
                if (moveFeather && useFeather) {
                    assert(fp);
                    fp->getPositionAtTime(*it2, &pF.x, &pF.y);
                    fp->getLeftBezierPointAtTime(*it2,  &leftF.x, &leftF.y);
                    fp->getRightBezierPointAtTime(*it2, &rightF.x, &rightF.y);

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
                    
                    
                    fp->setPositionAtTime(*it2, pF.x, pF.y);
                    fp->setLeftBezierPointAtTime(*it2, leftF.x, leftF.y);
                    fp->setRightBezierPointAtTime(*it2, rightF.x, rightF.y);
                }
            }
        }
    }

} // movePointByIndexInternalForView

void
Bezier::movePointByIndexInternal(int index,
                                 double time,
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
        ViewIdx view_i = getViewIdxFromGetSpec(ViewGetSpec(view));
        movePointByIndexInternalForView(index, time, view_i, dx, dy, onlyFeather);
    }
    refreshPolygonOrientation(time, view);
    if (isAutoKeyingEnabled()) {
        setKeyFrame(time, view, 0);
    }
    evaluateCurveModified();

} // movePointByIndexInternal


void
Bezier::setPointByIndexInternalForView(int index, double time, ViewIdx view, double x, double y)
{
    Transform::Matrix3x3 trans, invTrans;
    getTransformAtTime(time, view, &trans);
    invTrans = Transform::matInverse(trans);


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
Bezier::setPointByIndexInternal(int index, double time, ViewSetSpec view, double dx, double dy)
{
    if (view.isAll()) {
        std::list<ViewIdx> views = getViewsList();
        for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
            setPointByIndexInternalForView(index, time, *it, dx, dy);
        }
    } else {
        ViewIdx view_i = getViewIdxFromGetSpec(ViewGetSpec(view));
        setPointByIndexInternalForView(index, time, view_i, dx, dy);
    }
    refreshPolygonOrientation(time, view);
    if (isAutoKeyingEnabled()) {
        setKeyFrame(time, view, 0);
    }
    evaluateCurveModified();
}


void
Bezier::movePointByIndex(int index,
                         double time,
                         ViewSetSpec view,
                         double dx,
                         double dy)
{
    movePointByIndexInternal(index, time, view, dx, dy, false);
} // movePointByIndex

void
Bezier::setPointByIndex(int index,
                        double time,
                        ViewSetSpec view,
                        double x,
                        double y)
{
    setPointByIndexInternal(index, time, view, x, y);
} // setPointByIndex

void
Bezier::moveFeatherByIndex(int index,
                           double time,
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
                                       double time,
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
    invTrans = Transform::matInverse(trans);


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
            ///this function is called when building a new bezier we must
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
            std::set<double> keyframes;
            getMasterKeyFrameTimes(view, &keyframes);
            for (std::set<double>::iterator it2 = keyframes.begin(); it2 != keyframes.end(); ++it2) {
                if (*it2 == time) {
                    continue;
                }

                if (isLeft || moveBoth) {
                    if (moveControlPoint) {
                        (cp)->getLeftBezierPointAtTime(*it2, &left.x, &left.y);
                        left = Transform::matApply(trans, left);
                        left.x += lx; left.y += ly;
                        left = Transform::matApply(invTrans, left);
                        (cp)->setLeftBezierPointAtTime(*it2, left.x, left.y);
                    }
                    if ( moveFeather && useFeatherPoints() ) {
                        (fp)->getLeftBezierPointAtTime(*it2,  &leftF.x, &leftF.y);
                        leftF = Transform::matApply(trans, leftF);
                        leftF.x += flx; leftF.y += fly;
                        leftF = Transform::matApply(invTrans, leftF);
                        (fp)->setLeftBezierPointAtTime(*it2, leftF.x, leftF.y);
                    }
                } else {
                    if (moveControlPoint) {
                        (cp)->getRightBezierPointAtTime(*it2,  &right.x, &right.y);
                        right = Transform::matApply(trans, right);
                        right.x += rx; right.y += ry;
                        right = Transform::matApply(invTrans, right);
                        (cp)->setRightBezierPointAtTime(*it2, right.x, right.y);
                    }
                    if ( moveFeather && useFeatherPoints() ) {
                        (cp)->getRightBezierPointAtTime(*it2, &rightF.x, &rightF.y);
                        rightF = Transform::matApply(trans, rightF);
                        rightF.x += frx; rightF.y += fry;
                        rightF = Transform::matApply(invTrans, rightF);
                        (cp)->setRightBezierPointAtTime(*it2, rightF.x, rightF.y);
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
                                double time,
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
        ViewIdx view_i = getViewIdxFromGetSpec(ViewGetSpec(view));
        moveBezierPointInternalForView(cpParam, fpParam, index, time, view_i, lx, ly, rx, ry, flx, fly, frx, fry, isLeft, moveBoth, onlyFeather);
    }

    refreshPolygonOrientation(time, view);
    if (isAutoKeyingEnabled()) {
        setKeyFrame(time, view, 0);
    }
    evaluateCurveModified();

} // moveBezierPointInternal

void
Bezier::moveLeftBezierPoint(int index,
                            double time,
                            ViewSetSpec view,
                            double dx,
                            double dy)
{
    moveBezierPointInternal(NULL, NULL, index, time, view, dx, dy, 0, 0, dx, dy, 0, 0, true, false, false);
} // moveLeftBezierPoint

void
Bezier::moveRightBezierPoint(int index,
                             double time,
                             ViewSetSpec view,
                             double dx,
                             double dy)
{
    moveBezierPointInternal(NULL, NULL, index, time, view, 0, 0, dx, dy, 0, 0, dx, dy, false, false, false);
} // moveRightBezierPoint

void
Bezier::movePointLeftAndRightIndex(BezierCP & cp,
                                   BezierCP & fp,
                                   double time,
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
Bezier::setPointAtIndexInternalForView(bool setLeft, bool setRight, bool setPoint, bool feather, bool featherAndCp, int index, double time, ViewIdx view, double x, double y, double lx, double ly, double rx, double ry)
{
    bool autoKeying = isAutoKeyingEnabled();
    bool rippleEdit = isRippleEditEnabled();
    bool keySet = false;


    {
        QMutexLocker l(&_imp->itemMutex);
        BezierShape* shape = _imp->getViewShape(view);
        if (!shape) {
            return;
        }

        bool isOnKeyframe = hasMasterKeyframeAtTime(time, view);

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
            if (!isOnKeyframe) {
                keySet = true;
            }
        }

        if (rippleEdit) {
            std::set<double> keyframes;
            getMasterKeyFrameTimes(view, &keyframes);
            for (std::set<double>::iterator it2 = keyframes.begin(); it2 != keyframes.end(); ++it2) {
                if (setPoint) {
                    (*fp)->setPositionAtTime(*it2, x, y);
                    if (featherAndCp) {
                        (*cp)->setPositionAtTime(*it2, x, y);
                    }
                }
                if (setLeft) {
                    (*fp)->setLeftBezierPointAtTime(*it2, lx, ly);
                    if (featherAndCp) {
                        (*cp)->setLeftBezierPointAtTime(*it2, lx, ly);
                    }
                }
                if (setRight) {
                    (*fp)->setRightBezierPointAtTime(*it2, rx, ry);
                    if (featherAndCp) {
                        (*cp)->setRightBezierPointAtTime(*it2, rx, ry);
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
                                double time,
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
        ViewIdx view_i = getViewIdxFromGetSpec(ViewGetSpec(view));
        setPointAtIndexInternalForView(setLeft, setRight, setPoint, feather, featherAndCp, index, time, view_i, x, y, lx, ly, rx, ry);
    }

    refreshPolygonOrientation(time, view);

    if (isAutoKeyingEnabled()) {
        setKeyFrame(time, view, 0);
    }
    evaluateCurveModified();
} // setPointAtIndexInternal

void
Bezier::setLeftBezierPoint(int index,
                           double time,
                           ViewSetSpec view,
                           double x,
                           double y)
{
    setPointAtIndexInternal(true, false, false, false, true, index, time, view, 0, 0, x, y, 0, 0);
}

void
Bezier::setRightBezierPoint(int index,
                            double time,
                            ViewSetSpec view,
                            double x,
                            double y)
{
    setPointAtIndexInternal(false, true, false, false, true, index, time, view, 0, 0, 0, 0, x, y);
}

void
Bezier::setPointAtIndex(bool feather,
                        int index,
                        double time,
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
Bezier::onTransformSet(double time, ViewSetSpec view)
{
    refreshPolygonOrientation(time, view);
}

void
Bezier::transformPointInternal(const BezierCPPtr & point, double time, ViewIdx view, Transform::Matrix3x3* matrix)
{
    bool autoKeying = isAutoKeyingEnabled();
    bool keySet = false;

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
            if (!isonKeyframe) {
                keySet = true;
            }
        }
    }

}

void
Bezier::transformPoint(const BezierCPPtr & point,
                       double time,
                       ViewSetSpec view,
                       Transform::Matrix3x3* matrix)
{

    if (view.isAll()) {
        std::list<ViewIdx> views = getViewsList();
        for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
            transformPointInternal(point, time, *it, matrix);
        }
    } else {
        ViewIdx view_i = getViewIdxFromGetSpec(ViewGetSpec(view));
        transformPointInternal(point, time, view_i, matrix);
    }

    refreshPolygonOrientation(time, view);
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
        ViewIdx view_i = getViewIdxFromGetSpec(ViewGetSpec(view));
        removeFeatherAtIndexForView(index, view_i);
    }


    evaluateCurveModified();
}


void
Bezier::smoothOrCuspPointAtIndexInternal(bool isSmooth, int index, double time, ViewIdx view, const std::pair<double, double>& pixelScale)
{
    bool autoKeying = isAutoKeyingEnabled();
    bool rippleEdit = isRippleEditEnabled();


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
            (*cp)->smoothPoint(time, view, autoKeying, rippleEdit, pixelScale);
            if (useFeather) {
                (*fp)->smoothPoint(time, view,autoKeying, rippleEdit, pixelScale);
            }
        } else {
            (*cp)->cuspPoint(time, autoKeying, rippleEdit, pixelScale);
            if (useFeather) {
                (*fp)->cuspPoint(time,  autoKeying, rippleEdit, pixelScale);
            }
        }
    }
    

} // smoothOrCuspPointAtIndexInternal

void
Bezier::smoothOrCuspPointAtIndex(bool isSmooth,
                                 int index,
                                 double time,
                                 ViewSetSpec view,
                                 const std::pair<double, double>& pixelScale)
{
    if (view.isAll()) {
        std::list<ViewIdx> views = getViewsList();
        for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
            smoothOrCuspPointAtIndexInternal(isSmooth, index, time, *it, pixelScale);
        }
    } else {
        ViewIdx view_i = getViewIdxFromGetSpec(ViewGetSpec(view));
        smoothOrCuspPointAtIndexInternal(isSmooth, index, time, view_i, pixelScale);
    }


    refreshPolygonOrientation(time, view);
    if (isAutoKeyingEnabled()) {
        setKeyFrame(time, view, 0);
    }
    evaluateCurveModified();

} // Bezier::smoothOrCuspPointAtIndex

void
Bezier::smoothPointAtIndex(int index,
                           double time,
                           ViewSetSpec view,
                           const std::pair<double, double>& pixelScale)
{
    smoothOrCuspPointAtIndex(true, index, time, view, pixelScale);
}

void
Bezier::cuspPointAtIndex(int index,
                         double time,
                         ViewSetSpec view,
                         const std::pair<double, double>& pixelScale)
{
    smoothOrCuspPointAtIndex(false, index, time, view, pixelScale);
}


void
Bezier::onKeyFrameSetForView(double time, ViewIdx view)
{
    {
        QMutexLocker l(&_imp->itemMutex);
        BezierShape* shape = _imp->getViewShape(view);
        if (!shape) {
            return;
        }
        if ( hasMasterKeyframeAtTime(time, view)) {
            return;
        }

        bool useFeather = useFeatherPoints();
        assert(shape->points.size() == shape->featherPoints.size() || !useFeather);

        for (BezierCPs::iterator it = shape->points.begin(); it != shape->points.end(); ++it) {
            double x, y;
            double leftDerivX, rightDerivX, leftDerivY, rightDerivY;

            (*it)->getPositionAtTime(time,  &x, &y);
            (*it)->setPositionAtTime(time, x, y);

            (*it)->getLeftBezierPointAtTime(time,  &leftDerivX, &leftDerivY);
            (*it)->getRightBezierPointAtTime(time,  &rightDerivX, &rightDerivY);
            (*it)->setLeftBezierPointAtTime(time, leftDerivX, leftDerivY);
            (*it)->setRightBezierPointAtTime(time, rightDerivX, rightDerivY);
        }

        if (useFeather) {
            for (BezierCPs::iterator it = shape->featherPoints.begin(); it != shape->featherPoints.end(); ++it) {
                double x, y;
                double leftDerivX, rightDerivX, leftDerivY, rightDerivY;

                (*it)->getPositionAtTime(time, &x, &y);
                (*it)->setPositionAtTime(time, x, y);

                (*it)->getLeftBezierPointAtTime(time,  &leftDerivX, &leftDerivY);
                (*it)->getRightBezierPointAtTime(time,  &rightDerivX, &rightDerivY);
                (*it)->setLeftBezierPointAtTime(time, leftDerivX, leftDerivY);
                (*it)->setRightBezierPointAtTime(time, rightDerivX, rightDerivY);
            }
        }
    }
} // onKeyFrameSetForView

void
Bezier::onKeyFrameRemovedForView(double time, ViewIdx view)
{
    {
        QMutexLocker l(&_imp->itemMutex);
        BezierShape* shape = _imp->getViewShape(view);
        if (!shape) {
            return;
        }
        if ( hasMasterKeyframeAtTime(time, view)) {
            return;
        }
        assert( shape->featherPoints.size() == shape->points.size() || !useFeatherPoints() );

        bool useFeather = useFeatherPoints();
        BezierCPs::iterator fp = shape->featherPoints.begin();
        for (BezierCPs::iterator it = shape->points.begin(); it != shape->points.end(); ++it) {
            (*it)->removeKeyframe(time);
            if (useFeather) {
                (*fp)->removeKeyframe(time);
                ++fp;
            }
        }

        std::map<double, bool>::iterator found = shape->isClockwiseOriented.find(time);
        if ( found != shape->isClockwiseOriented.end() ) {
            shape->isClockwiseOriented.erase(found);
        }
        
    }
} // onKeyFrameRemovedForView

void
Bezier::onKeyFrameSet(double time, ViewSetSpec view)
{

    if (view.isAll()) {
        std::list<ViewIdx> views = getViewsList();
        for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
            onKeyFrameSetForView(time, *it);
        }
    } else {
        ViewIdx view_i = getViewIdxFromGetSpec(ViewGetSpec(view));
        onKeyFrameSetForView(time, view_i);
    }

} // onKeyFrameSet

void
Bezier::onKeyFrameRemoved(double time, ViewSetSpec view)
{

    if (view.isAll()) {
        std::list<ViewIdx> views = getViewsList();
        for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
            onKeyFrameRemovedForView(time, *it);
        }
    } else {
        ViewIdx view_i = getViewIdxFromGetSpec(ViewGetSpec(view));
        onKeyFrameRemovedForView(time, view_i);
    }

    evaluateCurveModified();
} // onKeyFrameRemoved


void
Bezier::deCastelJau(const std::list<BezierCPPtr >& cps,
                    double time,
                    unsigned int mipMapLevel,
                    bool finished,
                    int nBPointsPerSegment,
                    const Transform::Matrix3x3& transform,
                    std::vector<std::vector<ParametricPoint> >* points,
                    std::vector<ParametricPoint >* pointsSingleList,
                    RectD* bbox)
{
    bool bboxSet = false;
    assert((points && !pointsSingleList) || (!points && pointsSingleList));
    BezierCPs::const_iterator next = cps.begin();

    if ( next != cps.end() ) {
        ++next;
    }


    for (BezierCPs::const_iterator it = cps.begin(); it != cps.end(); ++it) {
        if ( next == cps.end() ) {
            if (!finished) {
                break;
            }
            next = cps.begin();
        }

        RectD segbbox;
        bool segbboxSet = false;
        if (points) {
            std::vector<ParametricPoint> segmentPoints;
            bezierSegmentEval(*(*it), *(*next), time,  mipMapLevel, nBPointsPerSegment, transform, &segmentPoints, bbox ? &segbbox : 0, &segbboxSet);
            points->push_back(segmentPoints);
        } else {
            assert(pointsSingleList);
            bezierSegmentEval(*(*it), *(*next), time,  mipMapLevel, nBPointsPerSegment, transform, pointsSingleList, bbox ? &segbbox : 0, &segbboxSet);
        }

        if (bbox) {
            if (!bboxSet) {
                *bbox = segbbox;
                bboxSet = true;
            } else {
                bbox->merge(segbbox);
            }
        }

        // increment for next iteration
        if ( next != cps.end() ) {
            ++next;
        }
    } // for()
} // deCastelJau

void
Bezier::evaluateAtTime_DeCasteljau(double time,
                                   ViewGetSpec view,
                                   unsigned int mipMapLevel,
#ifdef ROTO_BEZIER_EVAL_ITERATIVE
                                   int nbPointsPerSegment,
#else
                                   double errorScale,
#endif
                                   std::vector<std::vector< ParametricPoint> >* points,
                                   RectD* bbox) const
{
    evaluateAtTime_DeCasteljau_internal(time, view, mipMapLevel,
#ifdef ROTO_BEZIER_EVAL_ITERATIVE
                                        nbPointsPerSegment,
#else
                                        errorScale,
#endif
                                        points, 0, bbox);
}

void
Bezier::evaluateAtTime_DeCasteljau(double time,
                                   ViewGetSpec view,
                                   unsigned int mipMapLevel,
#ifdef ROTO_BEZIER_EVAL_ITERATIVE
                                   int nbPointsPerSegment,
#else
                                   double errorScale,
#endif
                                   std::vector<ParametricPoint >* pointsSingleList,
                                   RectD* bbox) const
{
    evaluateAtTime_DeCasteljau_internal(time, view, mipMapLevel,
#ifdef ROTO_BEZIER_EVAL_ITERATIVE
                                        nbPointsPerSegment,
#else
                                        errorScale,
#endif
                                         0, pointsSingleList, bbox);
}

void
Bezier::evaluateAtTime_DeCasteljau_internal(double time,
                                            ViewGetSpec view,
                                            unsigned int mipMapLevel,
#ifdef ROTO_BEZIER_EVAL_ITERATIVE
                                            int nbPointsPerSegment,
#else
                                            double errorScale,
#endif
                                            std::vector<std::vector<ParametricPoint> >* points,
                                            std::vector<ParametricPoint >* pointsSingleList,
                                            RectD* bbox) const
{
    assert((points && !pointsSingleList) || (!points && pointsSingleList));
    Transform::Matrix3x3 transform;

    getTransformAtTime(time, view, &transform);
    QMutexLocker l(&_imp->itemMutex);
    ViewIdx view_i = getViewIdxFromGetSpec(view);
    const BezierShape* shape = _imp->getViewShape(view_i);
    if (!shape) {
        return;
    }
    deCastelJau(shape->points, time, mipMapLevel, shape->finished,
#ifdef ROTO_BEZIER_EVAL_ITERATIVE
                nbPointsPerSegment,
#else
                errorScale,
#endif
                transform, points, pointsSingleList, bbox);
}

void
Bezier::evaluateAtTime_DeCasteljau_autoNbPoints(double time,
                                                ViewGetSpec view,
                                                unsigned int mipMapLevel,
                                                std::vector<std::vector<ParametricPoint> >* points,
                                                RectD* bbox) const
{
    evaluateAtTime_DeCasteljau(time, view, mipMapLevel,
#ifdef ROTO_BEZIER_EVAL_ITERATIVE
                               -1,
#else
                               1,
#endif
                               points, bbox);
}

void
Bezier::evaluateFeatherPointsAtTime_DeCasteljau(double time,
                                                ViewGetSpec view,
                                                unsigned int mipMapLevel,
#ifdef ROTO_BEZIER_EVAL_ITERATIVE
                                                int nbPointsPerSegment,
#else
                                                double errorScale,
#endif
                                                bool evaluateIfEqual,
                                                std::vector<ParametricPoint >* points,
                                                RectD* bbox) const
{
    evaluateFeatherPointsAtTime_DeCasteljau_internal(time, view, mipMapLevel,
#ifdef ROTO_BEZIER_EVAL_ITERATIVE
                                                     nbPointsPerSegment,
#else
                                                     errorScale,
#endif
                                                      evaluateIfEqual, 0, points, bbox);
}

void
Bezier::evaluateFeatherPointsAtTime_DeCasteljau_internal(double time,
                                                         ViewGetSpec view,
                                                         unsigned int mipMapLevel,
#ifdef ROTO_BEZIER_EVAL_ITERATIVE
                                                         int nbPointsPerSegment,
#else
                                                         double errorScale,
#endif
                                                         bool evaluateIfEqual,
                                                         std::vector<std::vector<ParametricPoint>  >* points,
                                                         std::vector<ParametricPoint >* pointsSingleList,
                                                         RectD* bbox) const
{
    assert((points && !pointsSingleList) || (!points && pointsSingleList));
    assert( useFeatherPoints() );
    QMutexLocker l(&_imp->itemMutex);

    ViewIdx view_i = getViewIdxFromGetSpec(view);
    const BezierShape* shape = _imp->getViewShape(view_i);
    if (!shape) {
        return;
    }


    if ( shape->points.empty() ) {
        return;
    }
    BezierCPs::const_iterator itCp = shape->points.begin();
    BezierCPs::const_iterator next = shape->featherPoints.begin();
    if ( next != shape->featherPoints.end() ) {
        ++next;
    }
    BezierCPs::const_iterator nextCp = itCp;
    if ( nextCp != shape->points.end() ) {
        ++nextCp;
    }

    Transform::Matrix3x3 transform;
    getTransformAtTime(time, view_i, &transform);

    for (BezierCPs::const_iterator it = shape->featherPoints.begin(); it != shape->featherPoints.end();
         ++it) {
        if ( next == shape->featherPoints.end() ) {
            next = shape->featherPoints.begin();
        }
        if ( nextCp == shape->points.end() ) {
            if (!shape->finished) {
                break;
            }
            nextCp = shape->points.begin();
        }
        if ( !evaluateIfEqual && bezierSegmenEqual(time,  **itCp, **nextCp, **it, **next) ) {
            continue;
        }
        if (points) {
            std::vector<ParametricPoint> segmentPoints;
            bezierSegmentEval(*(*it), *(*next), time, mipMapLevel,
#ifdef ROTO_BEZIER_EVAL_ITERATIVE
                              nbPointsPerSegment,
#else
                              errorScale,
#endif
                              transform, &segmentPoints, bbox);
            points->push_back(segmentPoints);
        } else {
            assert(pointsSingleList);
            bezierSegmentEval(*(*it), *(*next), time,  mipMapLevel,
#ifdef ROTO_BEZIER_EVAL_ITERATIVE
                              nbPointsPerSegment,
#else
                              errorScale,
#endif
                              transform, pointsSingleList, bbox);
        }

        // increment for next iteration
        if ( itCp != shape->featherPoints.end() ) {
            ++itCp;
        }
        if ( next != shape->featherPoints.end() ) {
            ++next;
        }
        if ( nextCp != shape->featherPoints.end() ) {
            ++nextCp;
        }
    } // for(it)

}

void
Bezier::evaluateFeatherPointsAtTime_DeCasteljau(double time,
                                                ViewGetSpec view,
                                                unsigned int mipMapLevel,
#ifdef ROTO_BEZIER_EVAL_ITERATIVE
                                                int nbPointsPerSegment,
#else
                                                double errorScale,
#endif
                                                bool evaluateIfEqual, ///< evaluate only if feather points are different from control points
                                                std::vector<std::vector<ParametricPoint> >* points, ///< output
                                                RectD* bbox) const ///< output
{
    evaluateFeatherPointsAtTime_DeCasteljau_internal(time, view, mipMapLevel,
#ifdef ROTO_BEZIER_EVAL_ITERATIVE
                                                     nbPointsPerSegment,
#else
                                                     errorScale,
#endif
                                                     evaluateIfEqual, points, 0, bbox);
} // Bezier::evaluateFeatherPointsAtTime_DeCasteljau

void
Bezier::getMotionBlurSettings(const double time,
                              ViewGetSpec view,
                              double* startTime,
                              double* endTime,
                              double* timeStep) const
{
    *startTime = time, *timeStep = 1., *endTime = time;

    double motionBlurAmnt = getMotionBlurAmountKnob()->getValueAtTime(time, DimIdx(0), view);
    if ( isOpenBezier() || (motionBlurAmnt == 0) ) {
        return;
    }
    int nbSamples = std::floor(motionBlurAmnt * 10 + 0.5);
    double shutterInterval = getShutterKnob()->getValueAtTime(time, DimIdx(0), view);
    if (shutterInterval == 0) {
        return;
    }
    int shutterType_i = getShutterTypeKnob()->getValueAtTime(time, DimIdx(0), view);
    if (nbSamples != 0) {
        *timeStep = shutterInterval / nbSamples;
    }
    if (shutterType_i == 0) { // centered
        *startTime = time - shutterInterval / 2.;
        *endTime = time + shutterInterval / 2.;
    } else if (shutterType_i == 1) { // start
        *startTime = time;
        *endTime = time + shutterInterval;
    } else if (shutterType_i == 2) { // end
        *startTime = time - shutterInterval;
        *endTime = time;
    } else if (shutterType_i == 3) { // custom
        *startTime = time + getShutterOffsetKnob()->getValueAtTime(time, DimIdx(0), view);
        *endTime = *startTime + shutterInterval;
    } else {
        assert(false);
    }


}

RectD
Bezier::getBoundingBox(double time, ViewGetSpec view) const
{
    double startTime = time, mbFrameStep = 1., endTime = time;

    RotoPaintPtr rotoPaintNode;
    KnobItemsTablePtr model = getModel();
    if (model) {
        rotoPaintNode = toRotoPaint(model->getNode()->getEffectInstance());
    }


    int mbType_i = rotoPaintNode->getMotionBlurTypeKnob()->getValue();
    bool applyPerShapeMotionBlur = mbType_i == 0;
    if (applyPerShapeMotionBlur) {
        getMotionBlurSettings(time, view, &startTime, &endTime, &mbFrameStep);
    }

    ViewIdx view_i = getViewIdxFromGetSpec(view);
    RectD bbox;
    bool bboxSet = false;
    for (double t = startTime; t <= endTime; t += mbFrameStep) {
        RectD pointsBbox;

        Transform::Matrix3x3 transform;
        getTransformAtTime(t, view, &transform);

        QMutexLocker l(&_imp->itemMutex);
        const BezierShape* shape = _imp->getViewShape(view_i);
        if (!shape) {
            continue;
        }


        bezierSegmentListBboxUpdate(shape->points, shape->finished, _imp->isOpenBezier, t, 0, transform, &pointsBbox);


        if (useFeatherPoints() && !_imp->isOpenBezier) {
            RectD featherPointsBbox;
            bezierSegmentListBboxUpdate( shape->featherPoints, shape->finished, _imp->isOpenBezier, t,  0, transform, &featherPointsBbox);
            pointsBbox.merge(featherPointsBbox);
            if (shape->featherPoints.size() > 1) {
                // EDIT: Partial fix, just pad the BBOX by the feather distance. This might not be accurate but gives at least something
                // enclosing the real bbox and close enough
                double featherDistance = _imp->feather.lock()->getValueAtTime(t, DimIdx(0), view);
                pointsBbox.x1 -= featherDistance;
                pointsBbox.x2 += featherDistance;
                pointsBbox.y1 -= featherDistance;
                pointsBbox.y2 += featherDistance;
            }
        } else if (_imp->isOpenBezier) {
            double brushSize = _imp->feather.lock()->getValueAtTime(t, DimIdx(0), view);
            double halfBrushSize = brushSize / 2. + 1;
            pointsBbox.x1 -= halfBrushSize;
            pointsBbox.x2 += halfBrushSize;
            pointsBbox.y1 -= halfBrushSize;
            pointsBbox.y2 += halfBrushSize;
        }
        if (!bboxSet) {
            bboxSet = true;
            bbox = pointsBbox;
        } else {
            bbox.merge(pointsBbox);
        }
    }

    return bbox;
} // Bezier::getBoundingBox

std::list< BezierCPPtr >
Bezier::getControlPoints(ViewGetSpec view) const
{
    ViewIdx view_i = getViewIdxFromGetSpec(view);
    QMutexLocker l(&_imp->itemMutex);
    const BezierShape* shape = _imp->getViewShape(view_i);
    if (!shape) {
        return std::list< BezierCPPtr >();
    }


    return shape->points;
}




std::list< BezierCPPtr >
Bezier::getFeatherPoints(ViewGetSpec view) const
{
    ViewIdx view_i = getViewIdxFromGetSpec(view);
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
                             double time,
                             ViewGetSpec view,
                             ControlPointSelectionPrefEnum pref,
                             int* index) const
{
    ///only called on the main-thread
    Transform::Matrix3x3 transform;
    getTransformAtTime(time, view, &transform);
    QMutexLocker l(&_imp->itemMutex);
    BezierCPPtr cp, fp;
    ViewIdx view_i = getViewIdxFromGetSpec(view);

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
Bezier::getControlPointIndex(const BezierCPPtr & cp, ViewGetSpec view) const
{
    return getControlPointIndex( cp.get() , view);
}

int
Bezier::getControlPointIndex(const BezierCP* cp, ViewGetSpec view) const
{
    ///only called on the main-thread
    assert(cp);
    QMutexLocker l(&_imp->itemMutex);
    ViewIdx view_i = getViewIdxFromGetSpec(view);

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
Bezier::getFeatherPointIndex(const BezierCPPtr & fp, ViewGetSpec view) const
{
    ///only called on the main-thread
    QMutexLocker l(&_imp->itemMutex);
    int i = 0;
    ViewIdx view_i = getViewIdxFromGetSpec(view);

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
Bezier::getControlPointAtIndex(int index, ViewGetSpec view) const
{
    QMutexLocker l(&_imp->itemMutex);
    ViewIdx view_i = getViewIdxFromGetSpec(view);

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
Bezier::getFeatherPointAtIndex(int index, ViewGetSpec view) const
{
    QMutexLocker l(&_imp->itemMutex);
    ViewIdx view_i = getViewIdxFromGetSpec(view);

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
Bezier::controlPointsWithinRect(double time,
                                ViewGetSpec view,
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
    ViewIdx view_i = getViewIdxFromGetSpec(view);

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
Bezier::getFeatherPointForControlPoint(const BezierCPPtr & cp, ViewGetSpec view) const
{
    assert( !cp->isFeatherPoint() );
    int index = getControlPointIndex(cp, view);
    assert(index != -1);

    return getFeatherPointAtIndex(index, view);
}

BezierCPPtr
Bezier::getControlPointForFeatherPoint(const BezierCPPtr & fp, ViewGetSpec view) const
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
Bezier::leftDerivativeAtPoint(double time,
                              const BezierCP & p,
                              const BezierCP & prev,
                              const Transform::Matrix3x3& transform,
                              double *dx,
                              double *dy)
{
    ///First-off, determine if the segment is a linear/quadratic/cubic bezier segment.
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
Bezier::rightDerivativeAtPoint(double time,
                               const BezierCP & p,
                               const BezierCP & next,
                               const Transform::Matrix3x3& transform,
                               double *dx,
                               double *dy)
{
    ///First-off, determine if the segment is a linear/quadratic/cubic bezier segment.
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
    refreshPolygonOrientation(false, ViewSetSpec::all());
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

bool
Bezier::isFeatherPolygonClockwiseOrientedInternal(double time, ViewGetSpec view) const
{
    ViewIdx view_i = getViewIdxFromGetSpec(view);
    QMutexLocker k(&_imp->itemMutex);
    const BezierShape* shape = _imp->getViewShape(view_i);
    if (!shape) {
        return false;
    }
    std::map<double, bool>::iterator it = shape->isClockwiseOriented.find(time);

    if ( it != shape->isClockwiseOriented.end() ) {
        return it->second;
    } else {
        int kfCount = getMasterKeyframesCount(view_i);
        if ( (kfCount > 0) && shape->finished ) {
            computePolygonOrientation(time, view_i, false);
            it = shape->isClockwiseOriented.find(time);
            if ( it != shape->isClockwiseOriented.end() ) {
                return it->second;
            } else {
                return false;
            }
        } else {
            return shape->isClockwiseOrientedStatic;
        }
    }
}

bool
Bezier::isFeatherPolygonClockwiseOriented(double time, ViewGetSpec view) const
{
    return isFeatherPolygonClockwiseOrientedInternal(time, view);
}

void
Bezier::setAutoOrientationComputation(bool autoCompute)
{
    assert( QThread::currentThread() == qApp->thread() );
    _imp->autoRecomputeOrientation = autoCompute;
}

void
Bezier::refreshPolygonOrientation(double time, ViewSetSpec view)
{
   
    QMutexLocker k(&_imp->itemMutex);
    if (!_imp->autoRecomputeOrientation) {
        return;
    }
    computePolygonOrientation(time, view, false);
}

void
Bezier::refreshPolygonOrientationForView(ViewIdx view)
{
    std::set<double> kfs;
    getMasterKeyFrameTimes(view, &kfs);

    QMutexLocker k(&_imp->itemMutex);
    if ( kfs.empty() ) {
        computePolygonOrientation(0, view, true);
    } else {
        for (std::set<double>::iterator it = kfs.begin(); it != kfs.end(); ++it) {
            computePolygonOrientation(*it, view, false);
        }
    }

}

void
Bezier::refreshPolygonOrientation(ViewSetSpec view)
{
    {
        QMutexLocker k(&_imp->itemMutex);
        if (!_imp->autoRecomputeOrientation) {
            return;
        }
    }
    if (view.isAll()) {
        std::list<ViewIdx> views = getViewsList();
        for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
            refreshPolygonOrientationForView(*it);
        }
    } else {
        ViewIdx view_i = getViewIdxFromGetSpec(ViewGetSpec(view));
        refreshPolygonOrientationForView(view_i);
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
void
Bezier::computePolygonOrientationForView(double time, ViewIdx view, bool isStatic) const
{
    //Private - should already be locked
    assert( !_imp->itemMutex.tryLock() );

    const BezierShape* shape = _imp->getViewShape(view);
    if (!shape) {
        return;
    }
    if (shape->points.size() <= 1) {
        return;
    }

    bool useFeather = useFeatherPoints();
    const BezierCPs& cps = useFeather ? shape->featherPoints : shape->points;
    double polygonSurface = 0.;
    if (shape->points.size() == 2) {
        //It does not matter since there are only 2 points
        polygonSurface = -1;
    } else {
        Point originalPoint;
        BezierCPs::const_iterator it = cps.begin();
        (*it)->getPositionAtTime(time, &originalPoint.x, &originalPoint.y);
        ++it;
        BezierCPs::const_iterator next = it;
        if ( next != cps.end() ) {
            ++next;
        }
        for (; next != cps.end(); ++it, ++next) {
            assert( it != cps.end() );
            double x, y;
            (*it)->getPositionAtTime(time, &x, &y);
            double xN, yN;
            (*next)->getPositionAtTime(time, &xN, &yN);
            Point u;
            u.x = x - originalPoint.x;
            u.y = y - originalPoint.y;

            Point v;
            v.x = xN - originalPoint.x;
            v.y = yN - originalPoint.y;

            //This is the area of the parallelogram defined by the U and V sides
            //Since a triangle is half a parallelogram, just half the cross-product
            double crossProduct = v.y * u.x - v.x * u.y;
            polygonSurface += (crossProduct / 2.);
        }
    } // for()
    if (isStatic) {
        shape->isClockwiseOrientedStatic = polygonSurface < 0;

    } else {
        shape->isClockwiseOriented[time] = polygonSurface < 0;
        
    }

} // computePolygonOrientationForView

void
Bezier::computePolygonOrientation(double time,
                                  ViewSetSpec view,
                                  bool isStatic) const
{
    if (view.isAll()) {
        std::list<ViewIdx> views = getViewsList();
        for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
            computePolygonOrientationForView(time, *it, isStatic);
        }
    } else {
        ViewIdx view_i = getViewIdxFromGetSpec(ViewGetSpec(view));
        computePolygonOrientationForView(time, view_i, isStatic);
    }

} // Bezier::computePolygonOrientation

/**
 * @brief Computes the location of the feather extent relative to the current feather point position and
 * the given feather distance.
 * In the case the control point and the feather point of the bezier are distinct, this function just makes use
 * of Thales theorem.
 * If the feather point and the control point are equal then this function computes the left and right derivative
 * of the bezier at that point to determine the direction in which the extent is.
 * @returns The delta from the given feather point to apply to find out the extent position.
 *
 * Note that the delta will be applied to fp.
 **/
Point
Bezier::expandToFeatherDistance(const Point & cp, //< the point
                                Point* fp, //< the feather point
                                double featherDistance, //< feather distance
                                double time, //< time
                                bool clockWise, //< is the bezier  clockwise oriented or not
                                const Transform::Matrix3x3& transform,
                                BezierCPs::const_iterator prevFp, //< iterator pointing to the feather before curFp
                                BezierCPs::const_iterator curFp, //< iterator pointing to fp
                                BezierCPs::const_iterator nextFp) //< iterator pointing after curFp
{
    Point ret;

    if (featherDistance != 0) {
        ///shortcut when the feather point is different than the control point
        if ( (cp.x != fp->x) && (cp.y != fp->y) ) {
            double dx = (fp->x - cp.x);
            double dy = (fp->y - cp.y);
            double dist = sqrt(dx * dx + dy * dy);
            ret.x = ( dx * (dist + featherDistance) ) / dist;
            ret.y = ( dy * (dist + featherDistance) ) / dist;
            fp->x =  ret.x + cp.x;
            fp->y =  ret.y + cp.y;
        } else {
            //compute derivatives to determine the feather extent
            double leftX, leftY, rightX, rightY, norm;
            Bezier::leftDerivativeAtPoint(time, **curFp, **prevFp, transform, &leftX, &leftY);
            Bezier::rightDerivativeAtPoint(time, **curFp, **nextFp, transform, &rightX, &rightY);
            norm = sqrt( (rightX - leftX) * (rightX - leftX) + (rightY - leftY) * (rightY - leftY) );

            ///normalize derivatives by their norm
            if (norm != 0) {
                ret.x = -( (rightY - leftY) / norm );
                ret.y = ( (rightX - leftX) / norm );
            } else {
                ///both derivatives are the same, use the direction of the left one
                norm = sqrt( (leftX - cp.x) * (leftX - cp.x) + (leftY - cp.y) * (leftY - cp.y) );
                if (norm != 0) {
                    ret.x = -( (leftY - cp.y) / norm );
                    ret.y = ( (leftX - cp.x) / norm );
                } else {
                    ///both derivatives and control point are equal, just use 0
                    ret.x = ret.y = 0;
                }
            }

            if (clockWise) {
                fp->x = cp.x + ret.x * featherDistance;
                fp->y = cp.y + ret.y * featherDistance;
            } else {
                fp->x = cp.x - ret.x * featherDistance;
                fp->y = cp.y - ret.y * featherDistance;
            }
        }
    } else {
        ret.x = ret.y = 0;
    }

    return ret;
} // expandToFeatherDistance

void
Bezier::appendToHash(double time, ViewIdx view, Hash64* hash)
{
    std::list<BezierCPPtr> cps = getControlPoints(view);
    std::list<BezierCPPtr> fps = getFeatherPoints(view);
    assert(cps.size() == fps.size() || fps.empty());

    if (!cps.empty()) {

        if (!_imp->isOpenBezier) {
            hash->append(isCurveFinished(view));
        }

        std::list<BezierCPPtr>::const_iterator fIt = fps.begin();
        for (std::list<BezierCPPtr>::const_iterator it = cps.begin(); it!=cps.end(); ++it, ++fIt) {
            double x, y, lx, ly, rx, ry;
            (*it)->getPositionAtTime(time, &x, &y);
            (*it)->getLeftBezierPointAtTime(time, &lx, &ly);
            (*it)->getRightBezierPointAtTime(time, &rx, &ry);
            double fx, fy, flx, fly, frx, fry;
            (*fIt)->getPositionAtTime(time, &fx, &fy);
            (*fIt)->getLeftBezierPointAtTime(time, &flx, &fly);
            (*fIt)->getRightBezierPointAtTime(time, &frx, &fry);

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
    RotoDrawableItem::appendToHash(time, view, hash);
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
    _imp->motionBlurKnob = createDuplicateOfTableKnob<KnobDouble>(kRotoPerShapeMotionBlurParam);
    _imp->shutterKnob = createDuplicateOfTableKnob<KnobDouble>(kRotoPerShapeShutterParam);
    _imp->shutterTypeKnob = createDuplicateOfTableKnob<KnobChoice>(kRotoPerShapeShutterOffsetTypeParam);
    _imp->customOffsetKnob = createDuplicateOfTableKnob<KnobDouble>(kRotoPerShapeShutterCustomOffsetParam);



} // initializeKnobs

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



KnobDoublePtr
Bezier::getMotionBlurAmountKnob() const
{
    return _imp->motionBlurKnob.lock();
}

KnobDoublePtr
Bezier::getShutterOffsetKnob() const
{
    return _imp->customOffsetKnob.lock();
}

KnobDoublePtr
Bezier::getShutterKnob() const
{
    return _imp->shutterKnob.lock();
}

KnobChoicePtr
Bezier::getShutterTypeKnob() const
{
    return _imp->shutterTypeKnob.lock();
}

NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_Bezier.cpp"
