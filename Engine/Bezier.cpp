/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
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
#include <stdexcept>
#include <cmath>

#include <QLineF>
#include <QtDebug>

GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
// /usr/local/include/boost/bind/arg.hpp:37:9: warning: unused typedef 'boost_static_assert_typedef_37' [-Wunused-local-typedef]
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/math/special_functions/fpclassify.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON

#include "Global/MemoryInfo.h"
#include "Engine/RotoContextPrivate.h"

#include "Engine/AppInstance.h"
#include "Engine/BezierCP.h"
#include "Engine/FeatherPoint.h"
#include "Engine/Interpolation.h"
#include "Engine/TimeLine.h"
#include "Engine/Image.h"
#include "Engine/ImageParams.h"
#include "Engine/Hash64.h"
#include "Engine/Settings.h"
#include "Engine/Format.h"
#include "Engine/RotoLayer.h"
#include "Engine/BezierSerialization.h"
#include "Engine/RenderStats.h"
#include "Engine/Transform.h"
#include "Engine/CoonsRegularization.h"
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

using namespace Natron;



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
                              RectD *bbox) ///< input/output
{
    {
        double x1, x2;
        bezierBounds(p0.x, p1.x, p2.x, p3.x, &x1, &x2);
        if (x1 < bbox->x1) {
            bbox->x1 = x1;
        }
        if (x2 > bbox->x2) {
            bbox->x2 = x2;
        }
    }
    {
        double y1, y2;
        bezierBounds(p0.y, p1.y, p2.y, p3.y, &y1, &y2);
        if (y1 < bbox->y1) {
            bbox->y1 = y1;
        }
        if (y2 > bbox->y2) {
            bbox->y2 = y2;
        }
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
bezierSegmentBboxUpdate(bool useGuiCurves,
                        const BezierCP & first,
                        const BezierCP & last,
                        double time,
                        unsigned int mipMapLevel,
                        const Transform::Matrix3x3& transform,
                        RectD* bbox) ///< input/output
{
    Point p0,p1,p2,p3;
    Transform::Point3D p0M,p1M,p2M,p3M;
    assert(bbox);
    
    try {
        first.getPositionAtTime(useGuiCurves,time, &p0M.x, &p0M.y);
        first.getRightBezierPointAtTime(useGuiCurves,time, &p1M.x, &p1M.y);
        last.getPositionAtTime(useGuiCurves,time, &p3M.x, &p3M.y);
        last.getLeftBezierPointAtTime(useGuiCurves,time, &p2M.x, &p2M.y);
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
    Bezier::bezierPointBboxUpdate(p0, p1, p2, p3, bbox);
}

void
Bezier::bezierSegmentListBboxUpdate(bool useGuiCurves,
                                    const BezierCPs & points,
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
        const boost::shared_ptr<BezierCP>& p = points.front();
        p->getPositionAtTime(useGuiCurves,time, &p0.x, &p0.y);
        p0.z = 1;
        p0 = Transform::matApply(transform, p0);
        bbox->x1 = p0.x;
        bbox->x2 = p0.x;
        bbox->y1 = p0.y;
        bbox->y2 = p0.y;
        return;
    }
    BezierCPs::const_iterator next = points.begin();
    if (next != points.end()) {
        ++next;
    }
    for (BezierCPs::const_iterator it = points.begin(); it != points.end(); ++it) {
        if ( next == points.end() ) {
            if (!finished && !isOpenBezier) {
                break;
            }
            next = points.begin();
        }
        bezierSegmentBboxUpdate(useGuiCurves,*(*it), *(*next), time, mipMapLevel, transform, bbox);
        
        // increment for next iteration
        if (next != points.end()) {
            ++next;
        }
    } // for()
}

// compute nbPointsperSegment points and update the bbox bounding box for the Bezier
// segment from 'first' to 'last' evaluated at 'time'
// If nbPointsPerSegment is -1 then it will be automatically computed
static void
bezierSegmentEval(bool useGuiCurves,
                  const BezierCP & first,
                  const BezierCP & last,
                  double time,
                  unsigned int mipMapLevel,
                  int nbPointsPerSegment,
                  const Transform::Matrix3x3& transform,
                  std::list< Point >* points, ///< output
                  RectD* bbox = NULL) ///< input/output (optional)
{
    Transform::Point3D p0M,p1M,p2M,p3M;
    Point p0,p1,p2,p3;
    
    try {
        first.getPositionAtTime(useGuiCurves,time, &p0M.x, &p0M.y);
        first.getRightBezierPointAtTime(useGuiCurves,time, &p1M.x, &p1M.y);
        last.getPositionAtTime(useGuiCurves,time, &p3M.x, &p3M.y);
        last.getLeftBezierPointAtTime(useGuiCurves,time, &p2M.x, &p2M.y);
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
    
    if (nbPointsPerSegment == -1) {
        /*
         * Approximate the necessary number of line segments, using http://antigrain.com/research/adaptive_bezier/
         */
        double dx1,dy1,dx2,dy2,dx3,dy3;
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
        double t = incr * i;

        Bezier::bezierPoint(p0, p1, p2, p3, t, &cur);
        points->push_back(cur);
    }
    if (bbox) {
        Bezier::bezierPointBboxUpdate(p0,  p1,  p2,  p3, bbox);
    }
}

/**
 * @brief Determines if the point (x,y) lies on the bezier curve segment defined by first and last.
 * @returns True if the point is close (according to the acceptance) to the curve, false otherwise.
 * @param param[out] It is set to the parametric value at which the subdivision of the bezier segment
 * yields the closest point to (x,y) on the curve.
 **/
static bool
bezierSegmentMeetsPoint(bool useGuiCurves,
                        const BezierCP & first,
                        const BezierCP & last,
                        const Transform::Matrix3x3& transform,
                        double time,
                        double x,
                        double y,
                        double distance,
                        double *param) ///< output
{
    Transform::Point3D p0,p1,p2,p3;
    p0.z = p1.z = p2.z = p3.z = 1;
    
    first.getPositionAtTime(useGuiCurves,time, &p0.x, &p0.y);
    first.getRightBezierPointAtTime(useGuiCurves,time, &p1.x, &p1.y);
    last.getPositionAtTime(useGuiCurves,time, &p3.x, &p3.y);
    last.getLeftBezierPointAtTime(useGuiCurves,time, &p2.x, &p2.y);
    
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
    
    
    Natron::Point p02d, p12d, p22d, p32d;
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
    int nbPoints = std::ceil(1./incr) + 1;
    for (int i = 0; i < nbPoints; ++i) {
        double t = std::min(i * incr, 1.);
        // the last point should be t == 1;
        assert(t < 1 || (t == 1. && i == nbPoints-1));
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
}

static bool
isPointCloseTo(bool useGuiCurves,
               double time,
               const BezierCP & p,
               double x,
               double y,
               const Transform::Matrix3x3& transform,
               double acceptance)
{
    Transform::Point3D pos;
    pos.z = 1;
    p.getPositionAtTime(useGuiCurves,time, &pos.x, &pos.y);
    pos = Transform::matApply(transform, pos);
    if ( ( pos.x >= (x - acceptance) ) && ( pos.x <= (x + acceptance) ) && ( pos.y >= (y - acceptance) ) && ( pos.y <= (y + acceptance) ) ) {
        return true;
    }
    
    return false;
}

static bool
bezierSegmenEqual(bool useGuiCurves,
                  double time,
                  const BezierCP & p0,
                  const BezierCP & p1,
                  const BezierCP & s0,
                  const BezierCP & s1)
{
    double prevX,prevY,prevXF,prevYF;
    double nextX,nextY,nextXF,nextYF;
    
    p0.getPositionAtTime(useGuiCurves,time, &prevX, &prevY);
    p1.getPositionAtTime(useGuiCurves,time, &nextX, &nextY);
    s0.getPositionAtTime(useGuiCurves,time, &prevXF, &prevYF);
    s1.getPositionAtTime(useGuiCurves,time, &nextXF, &nextYF);
    if ( (prevX != prevXF) || (prevY != prevYF) || (nextX != nextXF) || (nextY != nextYF) ) {
        return true;
    } else {
        ///check derivatives
        double prevRightX,prevRightY,nextLeftX,nextLeftY;
        double prevRightXF,prevRightYF,nextLeftXF,nextLeftYF;
        p0.getRightBezierPointAtTime(useGuiCurves,time, &prevRightX, &prevRightY);
        p1.getLeftBezierPointAtTime(useGuiCurves,time, &nextLeftX, &nextLeftY);
        s0.getRightBezierPointAtTime(useGuiCurves,time,&prevRightXF, &prevRightYF);
        s1.getLeftBezierPointAtTime(useGuiCurves,time, &nextLeftXF, &nextLeftYF);
        if ( (prevRightX != prevRightXF) || (prevRightY != prevRightYF) || (nextLeftX != nextLeftXF) || (nextLeftY != nextLeftYF) ) {
            return true;
        } else {
            return false;
        }
    }
}


////////////////////////////////////Bezier////////////////////////////////////

namespace  {
enum SplineChangedReason
{
    DERIVATIVES_CHANGED = 0,
    CONTROL_POINT_CHANGED = 1
};
}


Bezier::Bezier(const boost::shared_ptr<RotoContext>& ctx,
               const std::string & name,
               const boost::shared_ptr<RotoLayer>& parent,
               bool isOpenBezier)
    : RotoDrawableItem(ctx,name,parent, false)
      , _imp( new BezierPrivate(isOpenBezier) )
{
}


Bezier::Bezier(const Bezier & other,
               const boost::shared_ptr<RotoLayer>& parent)
: RotoDrawableItem( other.getContext(), other.getScriptName(), other.getParentLayer(), false )
, _imp( new BezierPrivate(false) )
{
    clone(&other);
    setParentLayer(parent);
}

bool
Bezier::isOpenBezier() const
{
    return _imp->isOpenBezier;
}

bool
Bezier::dequeueGuiActions()
{
    bool mustCopy;
    {
        QMutexLocker k2(&_imp->guiCopyMutex);
        mustCopy = _imp->mustCopyGui;
        if (mustCopy) {
            _imp->mustCopyGui = false;
        }
    }
    QMutexLocker k(&itemMutex);
    
    if (mustCopy) {
        boost::shared_ptr<Bezier> this_shared = boost::dynamic_pointer_cast<Bezier>(shared_from_this());
        assert(this_shared);
        BezierCPs::iterator fit = _imp->featherPoints.begin();
        for (BezierCPs::iterator it = _imp->points.begin(); it != _imp->points.end(); ++it,++fit) {
            (*it)->cloneGuiCurvesToInternalCurves();
            (*fit)->cloneGuiCurvesToInternalCurves();
        }
        
        _imp->isClockwiseOriented = _imp->guiIsClockwiseOriented;
        _imp->isClockwiseOrientedStatic = _imp->guiIsClockwiseOrientedStatic;
        incrementNodesAge();
    }
    return mustCopy;
}

void
Bezier::copyInternalPointsToGuiPoints()
{
    assert(!itemMutex.tryLock());
    boost::shared_ptr<Bezier> this_shared = boost::dynamic_pointer_cast<Bezier>(shared_from_this());
    assert(this_shared);
    BezierCPs::iterator fit = _imp->featherPoints.begin();
    for (BezierCPs::iterator it = _imp->points.begin(); it != _imp->points.end(); ++it,++fit) {
        (*it)->cloneInternalCurvesToGuiCurves();
        (*fit)->cloneInternalCurvesToGuiCurves();
    }
    _imp->guiIsClockwiseOriented = _imp->isClockwiseOriented;
    _imp->guiIsClockwiseOrientedStatic = _imp->isClockwiseOrientedStatic;
}

bool
Bezier::canSetInternalPoints() const
{
    return !getContext()->getNode()->isNodeRendering();
}

void
Bezier::clearAllPoints()
{
    removeAnimation();
    QMutexLocker k(&itemMutex);
    _imp->points.clear();
    _imp->featherPoints.clear();
    _imp->isClockwiseOriented.clear();
    _imp->finished = false;
}

void
Bezier::clone(const RotoItem* other)
{
    boost::shared_ptr<Bezier> this_shared = boost::dynamic_pointer_cast<Bezier>(shared_from_this());
    assert(this_shared);
    
    const Bezier* otherBezier = dynamic_cast<const Bezier*>(other);
    if (!otherBezier) {
        return;
    }
    
    Q_EMIT aboutToClone();
    {
        bool useFeather = otherBezier->useFeatherPoints();
        QMutexLocker l(&itemMutex);
        assert(otherBezier->_imp->featherPoints.size() == otherBezier->_imp->points.size() || !useFeather);

        bool useGuiCurve = !canSetInternalPoints();
        if (useGuiCurve) {
            _imp->setMustCopyGuiBezier(true);
        }
        
        _imp->featherPoints.clear();
        _imp->points.clear();
        BezierCPs::const_iterator itF = otherBezier->_imp->featherPoints.begin();
        for (BezierCPs::const_iterator it = otherBezier->_imp->points.begin(); it != otherBezier->_imp->points.end(); ++it) {
            boost::shared_ptr<BezierCP> cp( new BezierCP(this_shared) );
            cp->clone(**it);
            _imp->points.push_back(cp);
            if (useFeather) {
                boost::shared_ptr<BezierCP> fp( new BezierCP(this_shared) );
                fp->clone(**itF);
                _imp->featherPoints.push_back(fp);
                ++itF;
            }
        }
        
        if (!useGuiCurve) {
            copyInternalPointsToGuiPoints();
        }
        
        _imp->isOpenBezier = otherBezier->_imp->isOpenBezier;
        _imp->finished = otherBezier->_imp->finished && !_imp->isOpenBezier;
    }
    incrementNodesAge();
    RotoDrawableItem::clone(other);
    Q_EMIT cloned();
}

Bezier::~Bezier()
{
    BezierCPs::iterator itFp = _imp->featherPoints.begin();
    bool useFeather = !_imp->featherPoints.empty();
    for (BezierCPs::iterator itCp = _imp->points.begin(); itCp != _imp->points.end(); ++itCp) {
        boost::shared_ptr<KnobDouble> masterCp = (*itCp)->isSlaved();
        
        if (masterCp) {
            masterCp->removeSlavedTrack(*itCp);
        }
        
        if (useFeather) {
            boost::shared_ptr<KnobDouble> masterFp = (*itFp)->isSlaved();
            if (masterFp) {
                masterFp->removeSlavedTrack(*itFp);
            }
            ++itFp;
        }
    }
}

boost::shared_ptr<BezierCP>
Bezier::addControlPoint(double x,
                        double y,
                        double time)
{
    
    ///only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    
    if (isCurveFinished()) {
        return boost::shared_ptr<BezierCP>();
    }
    
    
    bool useGuiCurve = !canSetInternalPoints();
    if (useGuiCurve) {
        _imp->setMustCopyGuiBezier(true);
    }

    int keyframeTime;
    ///if the curve is empty make a new keyframe at the current timeline's time
    ///otherwise re-use the time at which the keyframe was set on the first control point
    
    
    boost::shared_ptr<BezierCP> p;
    boost::shared_ptr<Bezier> this_shared = boost::dynamic_pointer_cast<Bezier>(shared_from_this());
    assert(this_shared);
    bool autoKeying = getContext()->isAutoKeyingEnabled();
    {
        QMutexLocker l(&itemMutex);
        assert(!_imp->finished);
        if ( _imp->points.empty() ) {
            keyframeTime = time;
        } else {
            keyframeTime = _imp->points.front()->getKeyframeTime(useGuiCurve,0);
            if (keyframeTime == INT_MAX) {
                keyframeTime = getContext()->getTimelineCurrentTime();
            }
        }
    
        p.reset( new BezierCP(this_shared) );
        if (autoKeying) {
            p->setPositionAtTime(useGuiCurve,keyframeTime, x, y);
            p->setLeftBezierPointAtTime(useGuiCurve,keyframeTime, x,y);
            p->setRightBezierPointAtTime(useGuiCurve,keyframeTime, x, y);
        } else {
            p->setStaticPosition(useGuiCurve,x, y);
            p->setLeftBezierStaticPosition(useGuiCurve,x, y);
            p->setRightBezierStaticPosition(useGuiCurve,x, y);
        }
        _imp->points.insert(_imp->points.end(),p);
        
        if (useFeatherPoints()) {
            boost::shared_ptr<BezierCP> fp( new FeatherPoint(this_shared) );
            if (autoKeying) {
                fp->setPositionAtTime(useGuiCurve,keyframeTime, x, y);
                fp->setLeftBezierPointAtTime(useGuiCurve,keyframeTime, x, y);
                fp->setRightBezierPointAtTime(useGuiCurve,keyframeTime, x, y);
            } else {
                fp->setStaticPosition(useGuiCurve,x, y);
                fp->setLeftBezierStaticPosition(useGuiCurve,x, y);
                fp->setRightBezierStaticPosition(useGuiCurve,x, y);
            }
            _imp->featherPoints.insert(_imp->featherPoints.end(),fp);
        }
        
        if (!useGuiCurve) {
            copyInternalPointsToGuiPoints();
        }
    }
    
    incrementNodesAge();
    return p;
}

boost::shared_ptr<BezierCP>
Bezier::addControlPointAfterIndex(int index,
                                  double t)
{
    ///only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    
    boost::shared_ptr<Bezier> this_shared = boost::dynamic_pointer_cast<Bezier>(shared_from_this());
    assert(this_shared);

    boost::shared_ptr<BezierCP> p( new BezierCP(this_shared) );
    boost::shared_ptr<BezierCP> fp;
    
    bool useGuiCurve = !canSetInternalPoints();
    if (useGuiCurve) {
        _imp->setMustCopyGuiBezier(true);
    }
    
    if (useFeatherPoints()) {
        fp.reset( new FeatherPoint(this_shared) );
    }
    {
        QMutexLocker l(&itemMutex);
        
        if ( ( index >= (int)_imp->points.size() ) || (index < -1) ) {
            throw std::invalid_argument("Spline control point index out of range.");
        }
        
        
        ///we set the new control point position to be in the exact position the curve would have at each keyframe
        std::set<int> existingKeyframes;
        _imp->getKeyframeTimes(useGuiCurve,&existingKeyframes);
        
        BezierCPs::const_iterator prev,next,prevF,nextF;
        if (index == -1) {
            prev = _imp->points.end();
            if (prev != _imp->points.begin()) {
                --prev;
            }
            next = _imp->points.begin();
            
            if (useFeatherPoints()) {
                prevF = _imp->featherPoints.end();
                if (prevF != _imp->featherPoints.begin()) {
                    --prevF;
                }
                nextF = _imp->featherPoints.begin();
            }
        } else {
            prev = _imp->atIndex(index);
            next = prev;
            if (next != _imp->points.end()) {
                ++next;
            }
            if ( _imp->finished && ( next == _imp->points.end() ) ) {
                next = _imp->points.begin();
            }
            assert( next != _imp->points.end() );
            
            if (useFeatherPoints()) {
                prevF = _imp->featherPoints.begin();
                std::advance(prevF, index);
                nextF = prevF;
                if (nextF != _imp->featherPoints.end()) {
                    ++nextF;
                }
                if ( _imp->finished && ( nextF == _imp->featherPoints.end() ) ) {
                    nextF = _imp->featherPoints.begin();
                }
            }
        }
        
        
        for (std::set<int>::iterator it = existingKeyframes.begin(); it != existingKeyframes.end(); ++it) {
            Point p0,p1,p2,p3;
            (*prev)->getPositionAtTime(useGuiCurve,*it, &p0.x, &p0.y);
            (*prev)->getRightBezierPointAtTime(useGuiCurve,*it, &p1.x, &p1.y);
            (*next)->getPositionAtTime(useGuiCurve,*it, &p3.x, &p3.y);
            (*next)->getLeftBezierPointAtTime(useGuiCurve,*it, &p2.x, &p2.y);
            
            
            Point dest;
            Point p0p1, p1p2, p2p3, p0p1_p1p2, p1p2_p2p3;
            bezierFullPoint(p0, p1, p2, p3, t, &p0p1, &p1p2, &p2p3, &p0p1_p1p2, &p1p2_p2p3, &dest);
            
            //update prev and next inner control points
            (*prev)->setRightBezierPointAtTime(useGuiCurve,*it, p0p1.x, p0p1.y);
            (*next)->setLeftBezierPointAtTime(useGuiCurve,*it, p2p3.x, p2p3.y);
            
            if (useFeatherPoints()) {
                (*prevF)->setRightBezierPointAtTime(useGuiCurve,*it, p0p1.x, p0p1.y);
                (*nextF)->setLeftBezierPointAtTime(useGuiCurve,*it, p2p3.x, p2p3.y);
            }
            
            
            p->setPositionAtTime(useGuiCurve,*it, dest.x, dest.y);
            ///The left control point of p is p0p1_p1p2 and the right control point is p1p2_p2p3
            p->setLeftBezierPointAtTime(useGuiCurve,*it, p0p1_p1p2.x, p0p1_p1p2.y);
            p->setRightBezierPointAtTime(useGuiCurve,*it, p1p2_p2p3.x, p1p2_p2p3.y);
            
            if (useFeatherPoints()) {
                fp->setPositionAtTime(useGuiCurve,*it, dest.x, dest.y);
                fp->setLeftBezierPointAtTime(useGuiCurve,*it, p0p1_p1p2.x, p0p1_p1p2.y);
                fp->setRightBezierPointAtTime(useGuiCurve,*it, p1p2_p2p3.x, p1p2_p2p3.y);
            }
        }
        
        ///if there's no keyframes
        if ( existingKeyframes.empty() ) {
            Point p0,p1,p2,p3;
            
            (*prev)->getPositionAtTime(useGuiCurve,0, &p0.x, &p0.y);
            (*prev)->getRightBezierPointAtTime(useGuiCurve,0, &p1.x, &p1.y);
            (*next)->getPositionAtTime(useGuiCurve,0, &p3.x, &p3.y);
            (*next)->getLeftBezierPointAtTime(useGuiCurve,0, &p2.x, &p2.y);
            
            
            Point dest;
            Point p0p1, p1p2, p2p3, p0p1_p1p2, p1p2_p2p3;
            bezierFullPoint(p0, p1, p2, p3, t, &p0p1, &p1p2, &p2p3, &p0p1_p1p2, &p1p2_p2p3, &dest);
            
            //update prev and next inner control points
            (*prev)->setRightBezierStaticPosition(useGuiCurve,p0p1.x, p0p1.y);
            (*next)->setLeftBezierStaticPosition(useGuiCurve,p2p3.x, p2p3.y);
            
             if (useFeatherPoints()) {
                 (*prevF)->setRightBezierStaticPosition(useGuiCurve,p0p1.x, p0p1.y);
                 (*nextF)->setLeftBezierStaticPosition(useGuiCurve,p2p3.x, p2p3.y);
             }
            
            p->setStaticPosition(useGuiCurve,dest.x, dest.y);
            ///The left control point of p is p0p1_p1p2 and the right control point is p1p2_p2p3
            p->setLeftBezierStaticPosition(useGuiCurve,p0p1_p1p2.x, p0p1_p1p2.y);
            p->setRightBezierStaticPosition(useGuiCurve,p1p2_p2p3.x, p1p2_p2p3.y);
            
            if (useFeatherPoints()) {
                fp->setStaticPosition(useGuiCurve,dest.x, dest.y);
                fp->setLeftBezierStaticPosition(useGuiCurve,p0p1_p1p2.x, p0p1_p1p2.y);
                fp->setRightBezierStaticPosition(useGuiCurve,p1p2_p2p3.x, p1p2_p2p3.y);
            }
        }
        
        
        ////Insert the point into the container
        if (index != -1) {
            BezierCPs::iterator it = _imp->points.begin();
            ///it will point at the element right after index
            std::advance(it, index + 1);
            _imp->points.insert(it,p);
            
            if (useFeatherPoints()) {
                ///insert the feather point
                BezierCPs::iterator itF = _imp->featherPoints.begin();
                std::advance(itF, index + 1);
                _imp->featherPoints.insert(itF, fp);
            }
        } else {
            _imp->points.push_front(p);
             if (useFeatherPoints()) {
                 _imp->featherPoints.push_front(fp);
             }
        }
        
        
        ///If auto-keying is enabled, set a new keyframe
        int currentTime = getContext()->getTimelineCurrentTime();
        if ( !_imp->hasKeyframeAtTime(useGuiCurve,currentTime) && getContext()->isAutoKeyingEnabled() ) {
            l.unlock();
            setKeyframe(currentTime);
        }
        if (!useGuiCurve) {
            copyInternalPointsToGuiPoints();
        }
    }
    
    incrementNodesAge();
    
    return p;
} // addControlPointAfterIndex

int
Bezier::getControlPointsCount() const
{
    QMutexLocker l(&itemMutex);

    return (int)_imp->points.size();
}

int
Bezier::isPointOnCurve(double x,
                       double y,
                       double distance,
                       double *t,
                       bool* feather) const
{
    ///only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );

    double time = getContext()->getTimelineCurrentTime();
    
    Transform::Matrix3x3 transform;
    getTransformAtTime(time, &transform);
    
    QMutexLocker l(&itemMutex);

    ///special case: if the curve has only 1 control point, just check if the point
    ///is nearby that sole control point
    if (_imp->points.size() == 1) {
        const boost::shared_ptr<BezierCP> & cp = _imp->points.front();
        if ( isPointCloseTo(true, time, *cp, x, y, transform, distance) ) {
            *feather = false;
            
            return 0;
        } else {
            
            if (useFeatherPoints()) {
                ///do the same with the feather points
                const boost::shared_ptr<BezierCP> & fp = _imp->featherPoints.front();
                if ( isPointCloseTo(true,time, *fp, x, y, transform, distance) ) {
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
    
    assert( _imp->featherPoints.size() == _imp->points.size() || !useFeather);

    BezierCPs::const_iterator fp = _imp->featherPoints.begin();
    for (BezierCPs::const_iterator it = _imp->points.begin(); it != _imp->points.end(); ++it, ++index) {
        BezierCPs::const_iterator next = it;
        BezierCPs::const_iterator nextFp = fp;
        
        if (useFeather && nextFp != _imp->featherPoints.end()) {
            ++nextFp;
        }
        if (next != _imp->points.end()) {
            ++next;
        }
        if ( next == _imp->points.end() ) {
            if (!_imp->finished) {
                return -1;
            } else {
                next = _imp->points.begin();
                if (useFeather) {
                    nextFp = _imp->featherPoints.begin();
                }
            }
        }
        if ( bezierSegmentMeetsPoint(true, *(*it), *(*next), transform, time, x, y, distance, t) ) {
            *feather = false;

            return index;
        }
        
        if (useFeather && bezierSegmentMeetsPoint(true, **fp, **nextFp, transform, time, x, y, distance, t) ) {
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
Bezier::setCurveFinished(bool finished)
{

    ///only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    
    if (!_imp->isOpenBezier) {
        QMutexLocker l(&itemMutex);
        _imp->finished = finished;
    }
    
    getContext()->resetTransformCenter();

    incrementNodesAge();
    refreshPolygonOrientation(false);
}

bool
Bezier::isCurveFinished() const
{
    QMutexLocker l(&itemMutex);

    return _imp->finished;
}

void
Bezier::removeControlPointByIndex(int index)
{
    ///only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    
    
    {
        QMutexLocker l(&itemMutex);
        BezierCPs::iterator it;
        try {
            it = _imp->atIndex(index);
        } catch (...) {
            ///attempt to remove an unexsiting point
            return;
        }
        
        boost::shared_ptr<KnobDouble> isSlaved = (*it)->isSlaved();
        if (isSlaved) {
            isSlaved->removeSlavedTrack(*it);
        }
        _imp->points.erase(it);
        
        if (useFeatherPoints()) {
            BezierCPs::iterator itF = _imp->featherPoints.begin();
            std::advance(itF, index);
            _imp->featherPoints.erase(itF);
            
        }
        
    }
    
    incrementNodesAge();
    refreshPolygonOrientation(false);
   
    Q_EMIT controlPointRemoved();
}

void
Bezier::movePointByIndexInternal(bool useGuiCurve,
                                 int index,
                                 double time,
                                 double dx,
                                 double dy,
                                 bool onlyFeather)
{
    ///only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    
    bool rippleEdit = getContext()->isRippleEditEnabled();
    bool autoKeying = getContext()->isAutoKeyingEnabled();
    bool fLinkEnabled = (onlyFeather ? true : getContext()->isFeatherLinkEnabled());
    bool keySet = false;
    
    
    Transform::Matrix3x3 trans,invTrans;
    getTransformAtTime(time, &trans);
    invTrans = Transform::matInverse(trans);
    
    {
        QMutexLocker l(&itemMutex);
        Transform::Point3D p,left,right;
        p.z = left.z = right.z = 1;
        
        boost::shared_ptr<BezierCP> cp;
        bool isOnKeyframe = false;
        if (!onlyFeather) {
            BezierCPs::iterator it = _imp->points.begin();
            if (index < 0 || index >= (int)_imp->points.size()) {
                throw std::runtime_error("invalid index");
            }
            std::advance(it, index);
            assert(it != _imp->points.end());
            cp = *it;
            cp->getPositionAtTime(useGuiCurve,time, &p.x, &p.y,true);
            isOnKeyframe |= cp->getLeftBezierPointAtTime(useGuiCurve,time, &left.x, &left.y,true);
            cp->getRightBezierPointAtTime(useGuiCurve,time, &right.x, &right.y,true);
            
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
        Transform::Point3D pF,leftF,rightF;
        pF.z = leftF.z = rightF.z = 1;
        boost::shared_ptr<BezierCP> fp;
        if (useFeather) {
            BezierCPs::iterator itF = _imp->featherPoints.begin();
            std::advance(itF, index);
            assert(itF != _imp->featherPoints.end());
            fp = *itF;
            fp->getPositionAtTime(useGuiCurve,time, &pF.x, &pF.y,true);
            isOnKeyframe |= fp->getLeftBezierPointAtTime(useGuiCurve,time, &leftF.x, &leftF.y,true);
            fp->getRightBezierPointAtTime(useGuiCurve,time, &rightF.x, &rightF.y,true);
            
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
        
        bool moveFeather = (fLinkEnabled || (useFeather && fp && cp->equalsAtTime(useGuiCurve,time, *fp)));
        
        
        if (!onlyFeather && (autoKeying || isOnKeyframe)) {
            
            p = Transform::matApply(invTrans, p);
            right = Transform::matApply(invTrans, right);
            left = Transform::matApply(invTrans, left);
            
            assert(cp);
            cp->setPositionAtTime(useGuiCurve,time, p.x, p.y );
            cp->setLeftBezierPointAtTime(useGuiCurve,time, left.x, left.y);
            cp->setRightBezierPointAtTime(useGuiCurve,time, right.x, right.y);
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
                
                fp->setPositionAtTime(useGuiCurve,time, pF.x, pF.y);
                fp->setLeftBezierPointAtTime(useGuiCurve,time, leftF.x, leftF.y);
                fp->setRightBezierPointAtTime(useGuiCurve,time, rightF.x, rightF.y);
            }
        }
        
        if (rippleEdit) {
            std::set<int> keyframes;
            _imp->getKeyframeTimes(useGuiCurve,&keyframes);
            for (std::set<int>::iterator it2 = keyframes.begin(); it2 != keyframes.end(); ++it2) {
                if (*it2 == time) {
                    continue;
                }
                if (!onlyFeather) {
                    assert(cp);
                    cp->getPositionAtTime(useGuiCurve,*it2, &p.x, &p.y,true);
                    cp->getLeftBezierPointAtTime(useGuiCurve,*it2, &left.x, &left.y,true);
                    cp->getRightBezierPointAtTime(useGuiCurve,*it2, &right.x, &right.y,true);
                    
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

                    
                    cp->setPositionAtTime(useGuiCurve,*it2, p.x, p.y );
                    cp->setLeftBezierPointAtTime(useGuiCurve,*it2, left.x, left.y);
                    cp->setRightBezierPointAtTime(useGuiCurve,*it2, right.x, right.y);
                }
                if (moveFeather && useFeather) {
                    assert(fp);
                    fp->getPositionAtTime(useGuiCurve,*it2, &pF.x, &pF.y,true);
                    fp->getLeftBezierPointAtTime(useGuiCurve,*it2, &leftF.x, &leftF.y,true);
                    fp->getRightBezierPointAtTime(useGuiCurve,*it2, &rightF.x, &rightF.y,true);
                    
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

                    
                    fp->setPositionAtTime(useGuiCurve,*it2, pF.x, pF.y);
                    fp->setLeftBezierPointAtTime(useGuiCurve,*it2, leftF.x, leftF.y);
                    fp->setRightBezierPointAtTime(useGuiCurve,*it2, rightF.x, rightF.y);

                }
            }
        }
    }
    
    incrementNodesAge();
    refreshPolygonOrientation(useGuiCurve,time);
    if (autoKeying) {
        setKeyframe(time);
    }
    if (!useGuiCurve) {
        QMutexLocker k(&itemMutex);
        copyInternalPointsToGuiPoints();
    }
    if (keySet) {
        Q_EMIT keyframeSet(time);
    }

} // movePointByIndexInternal

void
Bezier::setPointByIndexInternal(bool useGuiCurve,
                                int index,
                                double time,
                                double x,
                                double y)
{
    ///only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );

    Transform::Matrix3x3 trans,invTrans;
    getTransformAtTime(time, &trans);
    invTrans = Transform::matInverse(trans);

    
    
    double dx, dy;

    {
        QMutexLocker l(&itemMutex);
        Transform::Point3D p,left,right;
        p.z = left.z = right.z = 1;

        boost::shared_ptr<BezierCP> cp;
        bool isOnKeyframe = false;

        BezierCPs::const_iterator it = _imp->atIndex(index);
        assert(it != _imp->points.end());
        cp = *it;
        cp->getPositionAtTime(useGuiCurve,time, &p.x, &p.y,true);
        isOnKeyframe |= cp->getLeftBezierPointAtTime(useGuiCurve,time, &left.x, &left.y,true);
        cp->getRightBezierPointAtTime(useGuiCurve,time, &right.x, &right.y,true);

        p = Transform::matApply(trans, p);
        left = Transform::matApply(trans, left);
        right = Transform::matApply(trans, right);

        dx = x - p.x;
        dy = y - p.y;
    }

    movePointByIndexInternal(useGuiCurve,index, time, dx, dy, false);
} // setPointByIndexInternal

void
Bezier::movePointByIndex(int index,
                         double time,
                         double dx,
                         double dy)
{
    bool useGuiCurve = !canSetInternalPoints();
    if (useGuiCurve) {
        _imp->setMustCopyGuiBezier(true);
    }
    movePointByIndexInternal(useGuiCurve,index, time, dx, dy, false);
} // movePointByIndex

void
Bezier::setPointByIndex(int index,
                         double time,
                         double x,
                         double y)
{
    bool useGuiCurve = !canSetInternalPoints();
    if (useGuiCurve) {
        _imp->setMustCopyGuiBezier(true);
    }
    setPointByIndexInternal(useGuiCurve,index, time, x, y);
} // setPointByIndex

void
Bezier::moveFeatherByIndex(int index,
                           double time,
                           double dx,
                           double dy)
{
    bool useGuiCurve = !canSetInternalPoints();
    if (useGuiCurve) {
        _imp->setMustCopyGuiBezier(true);
    }
    movePointByIndexInternal(useGuiCurve,index, time, dx, dy, true);
} // moveFeatherByIndex

void
Bezier::moveBezierPointInternal(BezierCP* cpParam,
                                int index,
                                double time,
                                double lx, double ly,
                                double rx, double ry,
                                bool isLeft,
                                bool moveBoth)
{
    ///only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    
    bool autoKeying = getContext()->isAutoKeyingEnabled();
    bool featherLink = getContext()->isFeatherLinkEnabled();
    bool rippleEdit = getContext()->isRippleEditEnabled();
    
    Transform::Matrix3x3 trans,invTrans;
    getTransformAtTime(time, &trans);
    invTrans = Transform::matInverse(trans);
    
    bool useGuiCurve = !canSetInternalPoints();
    if (useGuiCurve) {
        _imp->setMustCopyGuiBezier(true);
    }

    
    bool keySet = false;
    {
        QMutexLocker l(&itemMutex);
        BezierCP* cp = 0;
        BezierCP* fp = 0;
        
        if (!cpParam) {
            BezierCPs::iterator cpIt = _imp->points.begin();
            std::advance(cpIt, index);
            assert( cpIt != _imp->points.end() );
            cp = cpIt->get();
            assert(cp);
            
            if (useFeatherPoints()) {
                BezierCPs::iterator fpIt = _imp->featherPoints.begin();
                std::advance(fpIt, index);
                assert(fpIt != _imp->featherPoints.end());
                fp = fpIt->get();
                assert(fp);
            }
        } else {
            cp = cpParam;
#pragma message WARN("if !cpParam, fp is not set, even if useFeatherPoints()!")
        }
        
        bool isOnKeyframe = false;
        Transform::Point3D left,right;
        left.z = right.z = 1;
        if (isLeft || moveBoth) {
            isOnKeyframe = (cp)->getLeftBezierPointAtTime(useGuiCurve,time, &left.x, &left.y,true);
            left = Transform::matApply(trans, left);
        }
        if (!isLeft || moveBoth) {
            isOnKeyframe = (cp)->getRightBezierPointAtTime(useGuiCurve,time, &right.x, &right.y,true);
            right = Transform::matApply(trans, right);
        }
        
        bool moveFeather = false;
        Transform::Point3D leftF, rightF;
        leftF.z = rightF.z = 1;
        if (!cpParam && useFeatherPoints()) {
#pragma message WARN("if !cpParam, fp is not set, even if useFeatherPoints()!")
            moveFeather = true;
            if (isLeft || moveBoth) {
#pragma message WARN("BUG: dereferencing NULL pointer!")
                (fp)->getLeftBezierPointAtTime(useGuiCurve,time, &leftF.x, &leftF.y,true);
                leftF = Transform::matApply(trans, leftF);
                moveFeather = moveFeather && left.x == leftF.x && left.y == leftF.y;
            }
            if (!isLeft || moveBoth) {
#pragma message WARN("BUG: dereferencing NULL pointer!")
                (fp)->getRightBezierPointAtTime(useGuiCurve,time, &rightF.x, &rightF.y,true);
                rightF = Transform::matApply(trans, rightF);
                moveFeather = moveFeather && right.x == rightF.x && right.y == rightF.y;
            }
            moveFeather = moveFeather || featherLink;
        }

        if (autoKeying || isOnKeyframe) {
            if (isLeft || moveBoth) {
                left.x += lx;
                left.y += ly;
                left = Transform::matApply(invTrans, left);
                (cp)->setLeftBezierPointAtTime(useGuiCurve,time, left.x, left.y);
            }
            if (!isLeft || moveBoth) {
                right.x += rx;
                right.y += ry;
                right = Transform::matApply(invTrans, right);
                (cp)->setRightBezierPointAtTime(useGuiCurve,time, right.x, right.y);
            }
            if (moveFeather && useFeatherPoints()) {
                if (isLeft || moveBoth) {
                    leftF.x += lx;
                    leftF.y += ly;
                    leftF = Transform::matApply(invTrans, leftF);
                    (fp)->setLeftBezierPointAtTime(useGuiCurve,time, leftF.x, leftF.y);
                }
                if (!isLeft || moveBoth) {
                    rightF.x += rx;
                    rightF.y += ry;
                    rightF = Transform::matApply(invTrans, rightF);
                    (fp)->setRightBezierPointAtTime(useGuiCurve,time, rightF.x, rightF.y);
                }
            }
            if (!isOnKeyframe) {
                keySet = true;
            }
        } else {
            ///this function is called when building a new bezier we must
            ///move the static position if there is no keyframe, otherwise the
            ///curve would never be built
            if (isLeft || moveBoth) {
                left.x += lx;
                left.y += ly;
                left = Transform::matApply(invTrans, left);
                (cp)->setLeftBezierStaticPosition(useGuiCurve,left.x, left.y);
            }
            if (!isLeft || moveBoth) {
                right.x += rx;
                right.y += ry;
                right = Transform::matApply(invTrans, right);
                (cp)->setRightBezierStaticPosition(useGuiCurve,right.x, right.y);
            }
            if (moveFeather && useFeatherPoints()) {
                if (isLeft || moveBoth) {
                    leftF.x += lx;
                    leftF.y += ly;
                    leftF = Transform::matApply(invTrans, leftF);
                    (fp)->setLeftBezierStaticPosition(useGuiCurve,leftF.x, leftF.y);
                }
                if (!isLeft || moveBoth) {
                    rightF.x += rx;
                    rightF.y += ry;
                    rightF = Transform::matApply(invTrans, rightF);
                    (fp)->setRightBezierStaticPosition(useGuiCurve,rightF.x, rightF.y);
                }
                    
            }
        }
        
        if (rippleEdit) {
            std::set<int> keyframes;
            _imp->getKeyframeTimes(useGuiCurve,&keyframes);
            for (std::set<int>::iterator it2 = keyframes.begin(); it2 != keyframes.end(); ++it2) {
                if (*it2 == time) {
                    continue;
                }
                
                if (isLeft || moveBoth) {
                    (cp)->getLeftBezierPointAtTime(useGuiCurve,*it2, &left.x, &left.y,true);
                    left = Transform::matApply(trans, left);
                    left.x += lx; left.y += ly;
                    left = Transform::matApply(invTrans, left);
                    (cp)->setLeftBezierPointAtTime(useGuiCurve,*it2, left.x, left.y);
                    if (moveFeather && useFeatherPoints()) {
                        (fp)->getLeftBezierPointAtTime(useGuiCurve,*it2, &leftF.x, &leftF.y,true);
                        leftF = Transform::matApply(trans, leftF);
                        leftF.x += lx; leftF.y += ly;
                        leftF = Transform::matApply(invTrans, leftF);
                        (fp)->setLeftBezierPointAtTime(useGuiCurve,*it2, leftF.x, leftF.y);
                    }
                } else {
                    (cp)->getRightBezierPointAtTime(useGuiCurve,*it2, &right.x, &right.y,true);
                    right = Transform::matApply(trans, right);
                    right.x += rx; right.y += ry;
                    right = Transform::matApply(invTrans, right);
                    (cp)->setRightBezierPointAtTime(useGuiCurve,*it2, right.x, right.y);

                    if (moveFeather && useFeatherPoints()) {
                        
                        (cp)->getRightBezierPointAtTime(useGuiCurve,*it2, &rightF.x, &rightF.y,true);
                        rightF = Transform::matApply(trans, rightF);
                        rightF.x += rx; rightF.y += ry;
                        rightF = Transform::matApply(invTrans, rightF);
                        (cp)->setRightBezierPointAtTime(useGuiCurve,*it2, rightF.x, rightF.y);
                    }
                }
            }
        }
    }
    
    incrementNodesAge();
    refreshPolygonOrientation(useGuiCurve,time);
    if (!useGuiCurve) {
        QMutexLocker k(&itemMutex);
        copyInternalPointsToGuiPoints();
    }
    if (autoKeying) {
        setKeyframe(time);
    }
    if (keySet) {
        Q_EMIT keyframeSet(time);
    }

} // moveBezierPointInternal

void
Bezier::moveLeftBezierPoint(int index,
                            double time,
                            double dx,
                            double dy)
{
    moveBezierPointInternal(NULL, index, time, dx, dy, 0, 0, true, false);
} // moveLeftBezierPoint

void
Bezier::moveRightBezierPoint(int index,
                             double time,
                             double dx,
                             double dy)
{
    moveBezierPointInternal(NULL, index, time, 0, 0, dx, dy, false, false);
} // moveRightBezierPoint

void
Bezier::movePointLeftAndRightIndex(BezierCP & p,
                                   double time,
                                   double lx,
                                   double ly,
                                   double rx,
                                   double ry)
{
    moveBezierPointInternal(&p, -1, time, lx, ly, rx, ry, false, true);
}


void
Bezier::setPointAtIndexInternal(bool setLeft,bool setRight,bool setPoint,bool feather,bool featherAndCp,int index,double time,double x,double y,double lx,double ly,double rx,double ry)
{
    ///only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    bool autoKeying = getContext()->isAutoKeyingEnabled();
    bool rippleEdit = getContext()->isRippleEditEnabled();
    bool keySet = false;
    
    bool useGuiCurve = !canSetInternalPoints();
    if (useGuiCurve) {
        _imp->setMustCopyGuiBezier(true);
    }

    {
        QMutexLocker l(&itemMutex);
        bool isOnKeyframe = _imp->hasKeyframeAtTime(useGuiCurve,time);
        
        if ( index >= (int)_imp->points.size() ) {
            throw std::invalid_argument("Bezier::setPointAtIndex: Index out of range.");
        }
        
        BezierCPs::iterator fp = _imp->featherPoints.begin();
        BezierCPs::iterator cp = _imp->points.begin();
        if (!feather && !featherAndCp) {
            fp = cp;
        }
        std::advance(fp, index);
        if (featherAndCp) {
            std::advance(cp, index);
        }
        
        if (autoKeying || isOnKeyframe) {
            if (setPoint) {
                (*fp)->setPositionAtTime(useGuiCurve,time, x, y);
                if (featherAndCp) {
                    (*cp)->setPositionAtTime(useGuiCurve,time, x, y);
                }
            }
            if (setLeft) {
                (*fp)->setLeftBezierPointAtTime(useGuiCurve,time, lx, ly);
                if (featherAndCp) {
                    (*cp)->setLeftBezierPointAtTime(useGuiCurve,time, lx, ly);
                }
            }
            if (setRight) {
                (*fp)->setRightBezierPointAtTime(useGuiCurve,time, rx, ry);
                if (featherAndCp) {
                    (*cp)->setRightBezierPointAtTime(useGuiCurve,time, rx, ry);
                }
            }
            if (!isOnKeyframe) {
                keySet = true;
            }
        }
        
        if (rippleEdit) {
            std::set<int> keyframes;
            _imp->getKeyframeTimes(useGuiCurve,&keyframes);
            for (std::set<int>::iterator it2 = keyframes.begin(); it2 != keyframes.end(); ++it2) {
                if (setPoint) {
                    (*fp)->setPositionAtTime(useGuiCurve,*it2, x, y);
                    if (featherAndCp) {
                        (*cp)->setPositionAtTime(useGuiCurve,*it2, x, y);
                    }
                }
                if (setLeft) {
                    (*fp)->setLeftBezierPointAtTime(useGuiCurve,*it2, lx, ly);
                    if (featherAndCp) {
                        (*cp)->setLeftBezierPointAtTime(useGuiCurve,*it2, lx, ly);
                    }
                }
                if (setRight) {
                    (*fp)->setRightBezierPointAtTime(useGuiCurve,*it2, rx, ry);
                    if (featherAndCp) {
                        (*cp)->setRightBezierPointAtTime(useGuiCurve,*it2, rx, ry);
                    }
                }
            }
        }
    }
    
    incrementNodesAge();
    refreshPolygonOrientation(useGuiCurve,time);
    if (!useGuiCurve) {
        QMutexLocker k(&itemMutex);
        copyInternalPointsToGuiPoints();
    }
    if (autoKeying) {
        setKeyframe(time);
    }
    if (keySet) {
        Q_EMIT keyframeSet(time);
    }

} // setPointAtIndexInternal

void
Bezier::setLeftBezierPoint(int index,
                           double time,
                           double x,
                           double y)
{
    setPointAtIndexInternal(true, false, false, false, true, index, time, 0, 0, x, y, 0, 0);
}

void
Bezier::setRightBezierPoint(int index,
                            double time,
                            double x,
                            double y)
{
    setPointAtIndexInternal(false, true, false, false, true, index, time, 0, 0, 0, 0, x, y);
}

void
Bezier::setPointAtIndex(bool feather,
                        int index,
                        double time,
                        double x,
                        double y,
                        double lx,
                        double ly,
                        double rx,
                        double ry)
{
    setPointAtIndexInternal(true, true, true, feather, false, index, time, x, y, lx, ly, rx, ry);
}


void
Bezier::onTransformSet(double time)
{
    refreshPolygonOrientation(true,time);
}

void
Bezier::transformPoint(const boost::shared_ptr<BezierCP> & point,
                       double time,
                       Transform::Matrix3x3* matrix)
{
    bool autoKeying = getContext()->isAutoKeyingEnabled();
    bool keySet = false;
    
    bool useGuiCurve = !canSetInternalPoints();
    if (useGuiCurve) {
        _imp->setMustCopyGuiBezier(true);
    }

    {
        QMutexLocker l(&itemMutex);
        Transform::Point3D cp,leftCp,rightCp;
        point->getPositionAtTime(useGuiCurve,time, &cp.x, &cp.y,true);
        point->getLeftBezierPointAtTime(useGuiCurve,time, &leftCp.x, &leftCp.y,true);
        bool isonKeyframe = point->getRightBezierPointAtTime(useGuiCurve,time, &rightCp.x, &rightCp.y,true);
        
        
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
            point->setPositionAtTime(useGuiCurve,time, cp.x, cp.y);
            point->setLeftBezierPointAtTime(useGuiCurve,time, leftCp.x, leftCp.y);
            point->setRightBezierPointAtTime(useGuiCurve,time, rightCp.x, rightCp.y);
            if (!isonKeyframe) {
                keySet = true;
            }
        }
    }
    
    incrementNodesAge();
    refreshPolygonOrientation(useGuiCurve,time);
    if (!useGuiCurve) {
        QMutexLocker k(&itemMutex);
        copyInternalPointsToGuiPoints();
    }
    if (keySet) {
        Q_EMIT keyframeSet(time);
    }
}





void
Bezier::removeFeatherAtIndex(int index)
{
    ///only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    assert(useFeatherPoints());
  
    
    QMutexLocker l(&itemMutex);

    if ( index >= (int)_imp->points.size() ) {
        throw std::invalid_argument("Bezier::removeFeatherAtIndex: Index out of range.");
    }

    BezierCPs::iterator cp = _imp->points.begin();
    std::advance(cp, index);
    BezierCPs::iterator fp = _imp->featherPoints.begin();
    std::advance(fp, index);

    assert( cp != _imp->points.end() && fp != _imp->featherPoints.end() );

    (*fp)->clone(**cp);
    
    incrementNodesAge();
  
}

void
Bezier::smoothOrCuspPointAtIndex(bool isSmooth,int index,double time,const std::pair<double,double>& pixelScale)
{
    ///only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    bool keySet = false;
    bool autoKeying = getContext()->isAutoKeyingEnabled();
    bool rippleEdit = getContext()->isRippleEditEnabled();
    
    bool useGuiCurve = !canSetInternalPoints();
    if (useGuiCurve) {
        _imp->setMustCopyGuiBezier(true);
    }
    
    {
        QMutexLocker l(&itemMutex);
        if ( index >= (int)_imp->points.size() ) {
            throw std::invalid_argument("Bezier::smoothOrCuspPointAtIndex: Index out of range.");
        }
        
        BezierCPs::iterator cp = _imp->points.begin();
        std::advance(cp, index);
        BezierCPs::iterator fp;
        
        bool useFeather = useFeatherPoints();
        
        if (useFeather) {
            fp = _imp->featherPoints.begin();
            std::advance(fp, index);
        }
        assert( cp != _imp->points.end() && fp != _imp->featherPoints.end() );
        if (isSmooth) {
            keySet = (*cp)->smoothPoint(useGuiCurve,time,autoKeying,rippleEdit,pixelScale);
            if (useFeather) {
                (*fp)->smoothPoint(useGuiCurve,time,autoKeying,rippleEdit,pixelScale);
            }
        } else {
            keySet = (*cp)->cuspPoint(useGuiCurve,time,autoKeying,rippleEdit, pixelScale);
            if (useFeather) {
                (*fp)->cuspPoint(useGuiCurve,time,autoKeying,rippleEdit,pixelScale);
            }
        }
    }
    
    incrementNodesAge();
    refreshPolygonOrientation(useGuiCurve,time);
    if (autoKeying) {
        setKeyframe(time);
    }
    if (!useGuiCurve) {
        QMutexLocker k(&itemMutex);
        copyInternalPointsToGuiPoints();
    }
    if (keySet) {
        Q_EMIT keyframeSet(time);
    }
}

void
Bezier::smoothPointAtIndex(int index,
                           double time,
                           const std::pair<double,double>& pixelScale)
{
    smoothOrCuspPointAtIndex(true, index, time, pixelScale);
}

void
Bezier::cuspPointAtIndex(int index,
                         double time,
                         const std::pair<double,double>& pixelScale)
{
    smoothOrCuspPointAtIndex(false, index, time, pixelScale);
}

void
Bezier::setKeyframe(double time)
{
    ///only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );

    {
        QMutexLocker l(&itemMutex);
        if ( _imp->hasKeyframeAtTime(true,time) ) {
            return;
        }


        bool useFeather = useFeatherPoints();
        assert(_imp->points.size() == _imp->featherPoints.size() || !useFeather);

        for (BezierCPs::iterator it = _imp->points.begin(); it != _imp->points.end(); ++it) {
            double x, y;
            double leftDerivX, rightDerivX, leftDerivY, rightDerivY;

            (*it)->getPositionAtTime(true,time, &x, &y,true);
            (*it)->setPositionAtTime(true,time, x, y);

            (*it)->getLeftBezierPointAtTime(true,time, &leftDerivX, &leftDerivY,true);
            (*it)->getRightBezierPointAtTime(true,time, &rightDerivX, &rightDerivY,true);
            (*it)->setLeftBezierPointAtTime(true,time, leftDerivX, leftDerivY);
            (*it)->setRightBezierPointAtTime(true,time, rightDerivX, rightDerivY);
        }

        if (useFeather) {
            for (BezierCPs::iterator it = _imp->featherPoints.begin(); it != _imp->featherPoints.end(); ++it) {
                double x, y;
                double leftDerivX, rightDerivX, leftDerivY, rightDerivY;

                (*it)->getPositionAtTime(true,time, &x, &y,true);
                (*it)->setPositionAtTime(true,time, x, y);

                (*it)->getLeftBezierPointAtTime(true,time, &leftDerivX, &leftDerivY,true);
                (*it)->getRightBezierPointAtTime(true,time, &rightDerivX, &rightDerivY,true);
                (*it)->setLeftBezierPointAtTime(true,time, leftDerivX, leftDerivY);
                (*it)->setRightBezierPointAtTime(true,time, rightDerivX, rightDerivY);
            }
        }
    }
    _imp->setMustCopyGuiBezier(true);
    Q_EMIT keyframeSet(time);
}

void
Bezier::removeKeyframe(double time)
{
    ///only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    {
        QMutexLocker l(&itemMutex);

        if ( !_imp->hasKeyframeAtTime(false,time) ) {
            return;
        }
        assert( _imp->featherPoints.size() == _imp->points.size() || !useFeatherPoints());

        bool useFeather = useFeatherPoints();
        
        BezierCPs::iterator fp = _imp->featherPoints.begin();
        for (BezierCPs::iterator it = _imp->points.begin(); it != _imp->points.end(); ++it) {
            (*it)->removeKeyframe(false,time);
            if (useFeather) {
                (*fp)->removeKeyframe(false,time);
                ++fp;
            }
        }
        
        std::map<int,bool>::iterator found = _imp->isClockwiseOriented.find(time);
        if (found != _imp->isClockwiseOriented.end()) {
            _imp->isClockwiseOriented.erase(found);
        }
        
        copyInternalPointsToGuiPoints();
    }
    
    incrementNodesAge();
    getContext()->evaluateChange();
    Q_EMIT keyframeRemoved(time);
}

void
Bezier::removeAnimation()
{
    ///only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    
    double time = getContext()->getTimelineCurrentTime();
    {
        QMutexLocker l(&itemMutex);
        
        assert( _imp->featherPoints.size() == _imp->points.size() || !useFeatherPoints() );
        
        bool useFeather = useFeatherPoints();
        BezierCPs::iterator fp = _imp->featherPoints.begin();
        for (BezierCPs::iterator it = _imp->points.begin(); it != _imp->points.end(); ++it) {
            (*it)->removeAnimation(false,time);
            if (useFeather) {
                (*fp)->removeAnimation(false,time);
                ++fp;
            }
        }
     
        _imp->isClockwiseOriented.clear();
        
        copyInternalPointsToGuiPoints();
    }
    
    incrementNodesAge();
    Q_EMIT animationRemoved();
}

void
Bezier::moveKeyframe(int oldTime,int newTime)
{
    assert(QThread::currentThread() == qApp->thread());
    
    bool useFeather = useFeatherPoints();
    BezierCPs::iterator fp = _imp->featherPoints.begin();
    for (BezierCPs::iterator it = _imp->points.begin(); it != _imp->points.end(); ++it) {
        double x,y,lx,ly,rx,ry;
        (*it)->getPositionAtTime(false,oldTime, &x, &y);
        (*it)->getLeftBezierPointAtTime(false,oldTime, &lx, &ly);
        (*it)->getRightBezierPointAtTime(false,oldTime, &rx, &ry);
        
        (*it)->removeKeyframe(false,oldTime);
        
        (*it)->setPositionAtTime(false,newTime, x, y);
        (*it)->setLeftBezierPointAtTime(false,newTime, lx, ly);
        (*it)->setRightBezierPointAtTime(false,newTime, rx, ry);
        
        if (useFeather) {
            (*fp)->getPositionAtTime(false,oldTime, &x, &y);
            (*fp)->getLeftBezierPointAtTime(false,oldTime, &lx, &ly);
            (*fp)->getRightBezierPointAtTime(false,oldTime, &rx, &ry);
            
            (*fp)->removeKeyframe(false,oldTime);
            
            (*fp)->setPositionAtTime(false,newTime, x, y);
            (*fp)->setLeftBezierPointAtTime(false,newTime, lx, ly);
            (*fp)->setRightBezierPointAtTime(false,newTime, rx, ry);
            ++fp;
        }
    }
    
    {
        QMutexLocker k(&itemMutex);
        bool foundOld;
        bool oldValue;
        std::map<int,bool>::iterator foundOldIt = _imp->isClockwiseOriented.find(oldTime);
        foundOld = foundOldIt != _imp->isClockwiseOriented.end();
        if (foundOld) {
            oldValue = foundOldIt->first;
        } else {
            oldValue = 0;
        }
        _imp->isClockwiseOriented[newTime] = oldValue;
        
        copyInternalPointsToGuiPoints();
    }
    
    incrementNodesAge();
    Q_EMIT keyframeRemoved(oldTime);
    Q_EMIT keyframeSet(newTime);
}

int
Bezier::getKeyframesCount() const
{
    QMutexLocker l(&itemMutex);

    if ( _imp->points.empty() ) {
        return 0;
    } else {
        return _imp->points.front()->getKeyframesCount(true);
    }
}

void
Bezier::deCastelJau(bool useGuiCurves,
                    const std::list<boost::shared_ptr<BezierCP> >& cps,
                    double time,
                    unsigned int mipMapLevel,
                    bool finished,
                    int nBPointsPerSegment,
                    const Transform::Matrix3x3& transform,
                    std::list<Natron::Point>* points, RectD* bbox)
{
    BezierCPs::const_iterator next = cps.begin();
    
    if (next != cps.end()) {
        ++next;
    }
    
    
    for (BezierCPs::const_iterator it = cps.begin(); it != cps.end(); ++it) {
        if ( next == cps.end() ) {
            if (!finished) {
                break;
            }
            next = cps.begin();
        }
        bezierSegmentEval(useGuiCurves,*(*it),*(*next), time,mipMapLevel, nBPointsPerSegment, transform, points,bbox);
        
        // increment for next iteration
        if (next != cps.end()) {
            ++next;
        }
    } // for()
}

void
Bezier::evaluateAtTime_DeCasteljau(bool useGuiPoints,
                                   double time,
                                   unsigned int mipMapLevel,
                                   int nbPointsPerSegment,
                                   std::list< Natron::Point >* points,
                                   RectD* bbox) const
{
    Transform::Matrix3x3 transform;
    getTransformAtTime(time, &transform);
    QMutexLocker l(&itemMutex);
    deCastelJau(useGuiPoints,_imp->points, time, mipMapLevel, _imp->finished, nbPointsPerSegment, transform, points, bbox);
}

void
Bezier::evaluateAtTime_DeCasteljau_autoNbPoints(bool useGuiPoints,
                                                double time,
                                             unsigned int mipMapLevel,
                                             std::list<Natron::Point>* points,
                                             RectD* bbox) const
{
    evaluateAtTime_DeCasteljau(useGuiPoints,time, mipMapLevel, -1, points, bbox);
}

void
Bezier::evaluateFeatherPointsAtTime_DeCasteljau(bool useGuiPoints,
                                                double time,
                                                unsigned int mipMapLevel,
                                                int nbPointsPerSegment,
                                                bool evaluateIfEqual, ///< evaluate only if feather points are different from control points
                                                std::list< Natron::Point >* points, ///< output
                                                RectD* bbox) const ///< output
{
    assert(useFeatherPoints());
    QMutexLocker l(&itemMutex);

    
    if ( _imp->points.empty() ) {
        return;
    }
    BezierCPs::const_iterator itCp = _imp->points.begin();
    BezierCPs::const_iterator next = _imp->featherPoints.begin();
    if (next != _imp->featherPoints.end()) {
        ++next;
    }
    BezierCPs::const_iterator nextCp = itCp;
    if (nextCp != _imp->points.end()) {
        ++nextCp;
    }
    
    Transform::Matrix3x3 transform;
    getTransformAtTime(time, &transform);
    
    for (BezierCPs::const_iterator it = _imp->featherPoints.begin(); it != _imp->featherPoints.end();
         ++it) {
        if ( next == _imp->featherPoints.end() ) {
            next = _imp->featherPoints.begin();
        }
        if ( nextCp == _imp->points.end() ) {
            if (!_imp->finished) {
                break;
            }
            nextCp = _imp->points.begin();
        }
        if ( !evaluateIfEqual && bezierSegmenEqual(useGuiPoints,time, **itCp, **nextCp, **it, **next) ) {
            continue;
        }

        bezierSegmentEval(useGuiPoints, *(*it),*(*next), time, mipMapLevel, nbPointsPerSegment, transform, points, bbox);

        // increment for next iteration
        if (itCp != _imp->featherPoints.end()) {
            ++itCp;
        }
        if (next != _imp->featherPoints.end()) {
            ++next;
        }
        if (nextCp != _imp->featherPoints.end()) {
            ++nextCp;
        }
    } // for(it)
}

void
Bezier::getMotionBlurSettings(const double time,
                           double* startTime,
                           double* endTime,
                           double* timeStep) const
{
    *startTime = time, *timeStep = 1., *endTime = time;
#ifdef NATRON_ROTO_ENABLE_MOTION_BLUR
    
    double motionBlurAmnt = getMotionBlurAmountKnob()->getValueAtTime(time);
    if (isOpenBezier() || motionBlurAmnt == 0) {
        return;
    }
    int nbSamples = std::floor(motionBlurAmnt * 10 + 0.5);
    double shutterInterval = getShutterKnob()->getValueAtTime(time);
    if (shutterInterval == 0) {
        return;
    }
    int shutterType_i = getShutterTypeKnob()->getValueAtTime(time);
    if (nbSamples != 0) {
        *timeStep = shutterInterval / nbSamples;
    }
    if (shutterType_i == 0) { // centered
        *startTime = time - shutterInterval / 2.;
        *endTime = time + shutterInterval / 2.;
    } else if (shutterType_i == 1) {// start
        *startTime = time;
        *endTime = time + shutterInterval;
    } else if (shutterType_i == 2) { // end
        *startTime = time - shutterInterval;
        *endTime = time;
    } else if (shutterType_i == 3) { // custom
        *startTime = time + getShutterOffsetKnob()->getValueAtTime(time);
        *endTime = *startTime + shutterInterval;
    } else {
        assert(false);
    }
    
    
#endif
}

RectD
Bezier::getBoundingBox(double time) const
{
    double startTime = time, mbFrameStep = 1., endTime = time;
#ifdef NATRON_ROTO_ENABLE_MOTION_BLUR
    int mbType_i = getContext()->getMotionBlurTypeKnob()->getValue();
    bool applyPerShapeMotionBlur = mbType_i == 0;
    if (applyPerShapeMotionBlur) {
        getMotionBlurSettings(time, &startTime, &endTime, &mbFrameStep);
    }
#endif
    
    RectD bbox;
    bool bboxSet = false;
    for (double t = startTime; t <= endTime; t+= mbFrameStep) {
        RectD subBbox; // a very empty bbox
        subBbox.setupInfinity();
        
        Transform::Matrix3x3 transform;
        getTransformAtTime(t, &transform);
        
        QMutexLocker l(&itemMutex);
        bezierSegmentListBboxUpdate(false,_imp->points, _imp->finished, _imp->isOpenBezier, t, 0, transform , &subBbox);
        
        
        if (useFeatherPoints() && !_imp->isOpenBezier) {
            bezierSegmentListBboxUpdate(false,_imp->featherPoints, _imp->finished, _imp->isOpenBezier, t, 0, transform, &subBbox);
            // EDIT: Partial fix, just pad the BBOX by the feather distance. This might not be accurate but gives at least something
            // enclosing the real bbox and close enough
            double featherDistance = getFeatherDistance(t);
            subBbox.x1 -= featherDistance;
            subBbox.x2 += featherDistance;
            subBbox.y1 -= featherDistance;
            subBbox.y2 += featherDistance;
        } else if (_imp->isOpenBezier) {
            double brushSize = getBrushSizeKnob()->getValueAtTime(t);
            double halfBrushSize = brushSize / 2. + 1;
            subBbox.x1 -= halfBrushSize;
            subBbox.x2 += halfBrushSize;
            subBbox.y1 -= halfBrushSize;
            subBbox.y2 += halfBrushSize;
        }
        if (!bboxSet) {
            bboxSet = true;
            bbox = subBbox;
        } else {
            bbox.merge(subBbox);
        }
    }
    return bbox;
}

const std::list< boost::shared_ptr<BezierCP> > &
Bezier::getControlPoints() const
{
    ///only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );

    return _imp->points;
}

//protected only
std::list< boost::shared_ptr<BezierCP> > &
Bezier::getControlPoints_internal()
{
    return _imp->points;
}

std::list< boost::shared_ptr<BezierCP> >
Bezier::getControlPoints_mt_safe() const
{
    QMutexLocker l(&itemMutex);

    return _imp->points;
}


const std::list< boost::shared_ptr<BezierCP> > &
Bezier::getFeatherPoints() const
{
    ///only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );

    return _imp->featherPoints;
}


std::list< boost::shared_ptr<BezierCP> >
Bezier::getFeatherPoints_mt_safe() const
{
    QMutexLocker l(&itemMutex);
    return _imp->featherPoints;
}


std::pair<boost::shared_ptr<BezierCP>,boost::shared_ptr<BezierCP> >
Bezier::isNearbyControlPoint(double x,
                             double y,
                             double acceptance,
                             ControlPointSelectionPrefEnum pref,
                             int* index) const
{
    ///only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    double time = getContext()->getTimelineCurrentTime();
    
    Transform::Matrix3x3 transform;
    getTransformAtTime(time, &transform);
    QMutexLocker l(&itemMutex);
    boost::shared_ptr<BezierCP> cp,fp;

    switch (pref) {
    case eControlPointSelectionPrefFeatherFirst: {
        BezierCPs::const_iterator itF = _imp->findFeatherPointNearby(x, y, acceptance, time, transform, index);
        if ( itF != _imp->featherPoints.end() ) {
            fp = *itF;
            BezierCPs::const_iterator it = _imp->points.begin();
            std::advance(it, *index);
            cp = *it;

            return std::make_pair(fp, cp);
        } else {
            BezierCPs::const_iterator it = _imp->findControlPointNearby(x, y, acceptance, time, transform, index);
            if ( it != _imp->points.end() ) {
                cp = *it;
                itF = _imp->featherPoints.begin();
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
        BezierCPs::const_iterator it = _imp->findControlPointNearby(x, y, acceptance, time, transform, index);
        if ( it != _imp->points.end() ) {
            cp = *it;
            BezierCPs::const_iterator itF = _imp->featherPoints.begin();
            std::advance(itF, *index);
            fp = *itF;

            return std::make_pair(cp, fp);
        } else {
            BezierCPs::const_iterator itF = _imp->findFeatherPointNearby(x, y, acceptance, time, transform, index);
            if ( itF != _imp->featherPoints.end() ) {
                fp = *itF;
                it = _imp->points.begin();
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

    return std::make_pair(cp,fp);
} // isNearbyControlPoint

int
Bezier::getControlPointIndex(const boost::shared_ptr<BezierCP> & cp) const
{
    return getControlPointIndex( cp.get() );
}


int
Bezier::getControlPointIndex(const BezierCP* cp) const
{
    ///only called on the main-thread
    assert(cp);
    QMutexLocker l(&itemMutex);
    int i = 0;
    for (BezierCPs::const_iterator it = _imp->points.begin(); it != _imp->points.end(); ++it, ++i) {
        if (it->get() == cp) {
            return i;
        }
    }

    return -1;
}

int
Bezier::getFeatherPointIndex(const boost::shared_ptr<BezierCP> & fp) const
{
    ///only called on the main-thread
    QMutexLocker l(&itemMutex);
    int i = 0;

    for (BezierCPs::const_iterator it = _imp->featherPoints.begin(); it != _imp->featherPoints.end(); ++it, ++i) {
        if (*it == fp) {
            return i;
        }
    }

    return -1;
}

boost::shared_ptr<BezierCP>
Bezier::getControlPointAtIndex(int index) const
{
    QMutexLocker l(&itemMutex);

    if ( (index < 0) || ( index >= (int)_imp->points.size() ) ) {
        return boost::shared_ptr<BezierCP>();
    }

    BezierCPs::const_iterator it = _imp->points.begin();
    std::advance(it, index);

    return *it;
}

boost::shared_ptr<BezierCP>
Bezier::getFeatherPointAtIndex(int index) const
{
    QMutexLocker l(&itemMutex);

    if ( (index < 0) || ( index >= (int)_imp->featherPoints.size() ) ) {
        return boost::shared_ptr<BezierCP>();
    }

    BezierCPs::const_iterator it = _imp->featherPoints.begin();
    std::advance(it, index);

    return *it;
}

std::list< std::pair<boost::shared_ptr<BezierCP>,boost::shared_ptr<BezierCP> > >
Bezier::controlPointsWithinRect(double l,
                                double r,
                                double b,
                                double t,
                                double acceptance,
                                int mode) const
{
    std::list< std::pair<boost::shared_ptr<BezierCP>,boost::shared_ptr<BezierCP> > > ret;

    ///only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    QMutexLocker locker(&itemMutex);
    double time = getContext()->getTimelineCurrentTime();
    int i = 0;
    if ( (mode == 0) || (mode == 1) ) {
        for (BezierCPs::const_iterator it = _imp->points.begin(); it != _imp->points.end(); ++it, ++i) {
            double x,y;
            (*it)->getPositionAtTime(true, time, &x, &y);
            if ( ( x >= (l - acceptance) ) && ( x <= (r + acceptance) ) && ( y >= (b - acceptance) ) && ( y <= (t - acceptance) ) ) {
                std::pair<boost::shared_ptr<BezierCP>,boost::shared_ptr<BezierCP> > p;
                p.first = *it;
                BezierCPs::const_iterator itF = _imp->featherPoints.begin();
                std::advance(itF, i);
                p.second = *itF;
                ret.push_back(p);
            }
        }
    }
    i = 0;
    if ( (mode == 0) || (mode == 2) ) {
        for (BezierCPs::const_iterator it = _imp->featherPoints.begin(); it != _imp->featherPoints.end(); ++it, ++i) {
            double x,y;
            (*it)->getPositionAtTime(true, time, &x, &y);
            if ( ( x >= (l - acceptance) ) && ( x <= (r + acceptance) ) && ( y >= (b - acceptance) ) && ( y <= (t - acceptance) ) ) {
                std::pair<boost::shared_ptr<BezierCP>,boost::shared_ptr<BezierCP> > p;
                p.first = *it;
                BezierCPs::const_iterator itF = _imp->points.begin();
                std::advance(itF, i);
                p.second = *itF;

                ///avoid duplicates
                bool found = false;
                for (std::list< std::pair<boost::shared_ptr<BezierCP>,boost::shared_ptr<BezierCP> > >::iterator it2 = ret.begin();
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

boost::shared_ptr<BezierCP>
Bezier::getFeatherPointForControlPoint(const boost::shared_ptr<BezierCP> & cp) const
{
    assert( !cp->isFeatherPoint() );
    int index = getControlPointIndex(cp);
    assert(index != -1);

    return getFeatherPointAtIndex(index);
}

boost::shared_ptr<BezierCP>
Bezier::getControlPointForFeatherPoint(const boost::shared_ptr<BezierCP> & fp) const
{
    assert( fp->isFeatherPoint() );
    int index = getFeatherPointIndex(fp);
    assert(index != -1);

    return getControlPointAtIndex(index);
}

void
Bezier::leftDerivativeAtPoint(bool useGuiCurves,
                              double time,
                              const BezierCP & p,
                              const BezierCP & prev,
                              const Transform::Matrix3x3& transform,
                              double *dx,
                              double *dy)
{
    ///First-off, determine if the segment is a linear/quadratic/cubic bezier segment.
    assert( !p.equalsAtTime(useGuiCurves,time, prev) );
    bool p0equalsP1,p1equalsP2,p2equalsP3;
    
    Transform::Point3D p0,p1,p2,p3;
    p0.z = p1.z = p2.z = p3.z = 1;
    prev.getPositionAtTime(useGuiCurves,time, &p0.x, &p0.y);
    prev.getRightBezierPointAtTime(useGuiCurves,time, &p1.x, &p1.y);
    p.getLeftBezierPointAtTime(useGuiCurves,time, &p2.x, &p2.y);
    p.getPositionAtTime(useGuiCurves,time, &p3.x, &p3.y);
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
Bezier::rightDerivativeAtPoint(bool useGuiCurves,
                               double time,
                               const BezierCP & p,
                               const BezierCP & next,
                               const Transform::Matrix3x3& transform,
                               double *dx,
                               double *dy)
{
    ///First-off, determine if the segment is a linear/quadratic/cubic bezier segment.
    assert( !p.equalsAtTime(useGuiCurves,time, next) );
    bool p0equalsP1,p1equalsP2,p2equalsP3;
    Transform::Point3D p0,p1,p2,p3;
    p0.z = p1.z = p2.z = p3.z = 1;
    p.getPositionAtTime(useGuiCurves,time, &p0.x, &p0.y);
    p.getRightBezierPointAtTime(useGuiCurves,time, &p1.x, &p1.y);
    next.getLeftBezierPointAtTime(useGuiCurves,time, &p2.x, &p2.y);
    next.getPositionAtTime(useGuiCurves,time, &p3.x, &p3.y);
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
Bezier::save(RotoItemSerialization* obj) const
{
    BezierSerialization* s = dynamic_cast<BezierSerialization*>(obj);
    if (s) {
        QMutexLocker l(&itemMutex);

        s->_closed = _imp->finished;
        s->_isOpenBezier = _imp->isOpenBezier;
        assert( _imp->featherPoints.size() == _imp->points.size() || !useFeatherPoints());


        bool useFeather = useFeatherPoints();
        BezierCPs::const_iterator fp = _imp->featherPoints.begin();
        for (BezierCPs::const_iterator it = _imp->points.begin(); it != _imp->points.end(); ++it) {
            BezierCP c;
            c.clone(**it);
            s->_controlPoints.push_back(c);
            if (useFeather) {
                BezierCP f;
                f.clone(**fp);
                s->_featherPoints.push_back(f);
                ++fp;
            }
            
        }
    }
    
    RotoDrawableItem::save(obj);
}

void
Bezier::load(const RotoItemSerialization & obj)
{
    boost::shared_ptr<Bezier> this_shared = boost::dynamic_pointer_cast<Bezier>(shared_from_this());
    assert(this_shared);

    const BezierSerialization & s = dynamic_cast<const BezierSerialization &>(obj);
    {
        QMutexLocker l(&itemMutex);
        _imp->isOpenBezier = s._isOpenBezier;
        _imp->finished = s._closed && !_imp->isOpenBezier;
        
        bool useFeather = useFeatherPoints();
        std::list<BezierCP>::const_iterator itF = s._featherPoints.begin();
        for (std::list<BezierCP>::const_iterator it = s._controlPoints.begin(); it != s._controlPoints.end(); ++it) {
            boost::shared_ptr<BezierCP> cp( new BezierCP(this_shared) );
            cp->clone(*it);
            _imp->points.push_back(cp);
            
            if (useFeather) {
                boost::shared_ptr<BezierCP> fp( new FeatherPoint(this_shared) );
                fp->clone(*itF);
                _imp->featherPoints.push_back(fp);
                ++itF;
            }
        }
        copyInternalPointsToGuiPoints();
    }
    refreshPolygonOrientation(false);
    RotoDrawableItem::load(obj);
}

void
Bezier::getKeyframeTimes(std::set<int> *times) const
{
    QMutexLocker l(&itemMutex);

    _imp->getKeyframeTimes(true,times);
}

void
Bezier::getKeyframeTimesAndInterpolation(std::list<std::pair<int,Natron::KeyframeTypeEnum> > *keys) const
{
    QMutexLocker l(&itemMutex);
    if ( _imp->points.empty() ) {
        return;
    }
    _imp->points.front()->getKeyFrames(true,keys);
}

int
Bezier::getPreviousKeyframeTime(double time) const
{
    std::set<int> times;
    QMutexLocker l(&itemMutex);

    _imp->getKeyframeTimes(true,&times);
    for (std::set<int>::reverse_iterator it = times.rbegin(); it != times.rend(); ++it) {
        if (*it < time) {
            return *it;
        }
    }

    return INT_MIN;
}

int
Bezier::getNextKeyframeTime(double time) const
{
    std::set<int> times;
    QMutexLocker l(&itemMutex);

    _imp->getKeyframeTimes(true,&times);
    for (std::set<int>::iterator it = times.begin(); it != times.end(); ++it) {
        if (*it > time) {
            return *it;
        }
    }

    return INT_MAX;
}

int
Bezier::getKeyFrameIndex(double time) const
{
    QMutexLocker l(&itemMutex);
    if (_imp->points.empty()) {
        return -1;
    }
    return _imp->points.front()->getKeyFrameIndex(true,time);
}

void
Bezier::setKeyFrameInterpolation(Natron::KeyframeTypeEnum interp,int index)
{
    QMutexLocker l(&itemMutex);
    bool useFeather = useFeatherPoints();
    BezierCPs::iterator fp = _imp->featherPoints.begin();
    for (BezierCPs::iterator it = _imp->points.begin(); it != _imp->points.end(); ++it) {
        (*it)->setKeyFrameInterpolation(false,interp, index);
        
        if (useFeather) {
            (*fp)->setKeyFrameInterpolation(false,interp, index);
            ++fp;
        }
    }
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
Bezier::isFeatherPolygonClockwiseOrientedInternal(bool useGuiCurve,double time) const
{
    std::map<int,bool>::iterator it = _imp->isClockwiseOriented.find(time);
    if (it != _imp->isClockwiseOriented.end()) {
        return it->second;
    } else {
        
        int kfCount;
        if ( _imp->points.empty() ) {
            kfCount = 0;
        } else {
            kfCount = _imp->points.front()->getKeyframesCount(useGuiCurve);
        }
        if (kfCount > 0 && _imp->finished) {
            computePolygonOrientation(useGuiCurve,time, false);
            it = _imp->isClockwiseOriented.find(time);
            if (it != _imp->isClockwiseOriented.end()) {
                return it->second;
            } else {
                return false;
            }
        } else {
            return _imp->isClockwiseOrientedStatic;
        }
    }
}

bool
Bezier::isFeatherPolygonClockwiseOriented(bool useGuiCurve,double time) const
{
    QMutexLocker k(&itemMutex);
    return isFeatherPolygonClockwiseOrientedInternal(useGuiCurve,time);
}

void
Bezier::setAutoOrientationComputation(bool autoCompute)
{
    assert(QThread::currentThread() == qApp->thread());
    _imp->autoRecomputeOrientation = autoCompute;
}

void
Bezier::refreshPolygonOrientation(bool useGuiCurve,double time)
{
    if (useGuiCurve) {
        _imp->setMustCopyGuiBezier(true);
    }
    QMutexLocker k(&itemMutex);
    if (!_imp->autoRecomputeOrientation) {
        return;
    }
    computePolygonOrientation(useGuiCurve,time,false);
}


void
Bezier::refreshPolygonOrientation(bool useGuiCurve)
{
    {
        QMutexLocker k(&itemMutex);
        if (!_imp->autoRecomputeOrientation) {
            return;
        }
    }
    std::set<int> kfs;
    getKeyframeTimes(&kfs);
    
    QMutexLocker k(&itemMutex);
    if (kfs.empty()) {
        computePolygonOrientation(useGuiCurve,0,true);
    } else {
        for (std::set<int>::iterator it = kfs.begin(); it != kfs.end(); ++it) {
            computePolygonOrientation(useGuiCurve,*it,false);
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
void
Bezier::computePolygonOrientation(bool useGuiCurves, double time,bool isStatic) const
{
    //Private - should already be locked
    assert(!itemMutex.tryLock());
    
    if (_imp->points.size() <= 1) {
        return;
    }
    
    bool useFeather = useFeatherPoints();
    const BezierCPs& cps = useFeather ? _imp->featherPoints : _imp->points;
    
    
    double polygonSurface = 0.;
    if (_imp->points.size() == 2) {
        //It does not matter since there are only 2 points
        polygonSurface = -1;
    } else {
        Point originalPoint;
        BezierCPs::const_iterator it = cps.begin();
        (*it)->getPositionAtTime(useGuiCurves, time, &originalPoint.x, &originalPoint.y);
        ++it;
        BezierCPs::const_iterator next = it;
        if (next != cps.end()) {
            ++next;
        }
        for (;next != cps.end(); ++it, ++next) {
            assert(it != cps.end());
            double x,y;
            (*it)->getPositionAtTime(useGuiCurves, time, &x, &y);
            double xN,yN;
            (*next)->getPositionAtTime(useGuiCurves, time, &xN, &yN);
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
        if (!useGuiCurves) {
            _imp->isClockwiseOrientedStatic = polygonSurface < 0;
        } else {
            _imp->guiIsClockwiseOrientedStatic = polygonSurface < 0;
        }
    } else {
        if (!useGuiCurves) {
            _imp->isClockwiseOriented[time] = polygonSurface < 0;
        } else {
            _imp->guiIsClockwiseOriented[time] = polygonSurface < 0;
        }
    }
}

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
Bezier::expandToFeatherDistance(bool useGuiCurve,
                                const Point & cp, //< the point
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
            double leftX,leftY,rightX,rightY,norm;
            Bezier::leftDerivativeAtPoint(useGuiCurve,time, **curFp, **prevFp, transform, &leftX, &leftY);
            Bezier::rightDerivativeAtPoint(useGuiCurve,time, **curFp, **nextFp, transform, &rightX, &rightY);
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


