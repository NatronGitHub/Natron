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

#include "RotoContext.h"

#include <algorithm> // min, max
#include <sstream>
#include <locale>
#include <limits>
#include <stdexcept>

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

#include "Engine/Interpolation.h"
#include "Engine/AppInstance.h"
#include "Engine/TimeLine.h"
#include "Engine/Image.h"
#include "Engine/ImageParams.h"
#include "Engine/Hash64.h"
#include "Engine/Settings.h"
#include "Engine/Format.h"
#include "Engine/RotoSerialization.h"
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
static inline double
bezier(double p0,
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
static inline void
bezierFullPoint(const Point & p0,
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
            updateRange(bezier(p0, p1, p2, p3, t), xmin, xmax);
        }
        
        return;
    }
    double disc = b * b - 4 * a * c;
    if (disc < 0) {
        // no real solution
    } else if (disc == 0) {
        double t = -b / (2 * a);
        if ( (0 < t) && (t < 1) ) {
            updateRange(bezier(p0, p1, p2, p3, t), xmin, xmax);
        }
    } else {
        double t;
        t = ( -b - std::sqrt(disc) ) / (2 * a);
        if ( (0 < t) && (t < 1) ) {
            updateRange(bezier(p0, p1, p2, p3, t), xmin, xmax);
        }
        t = ( -b + std::sqrt(disc) ) / (2 * a);
        if ( (0 < t) && (t < 1) ) {
            updateRange(bezier(p0, p1, p2, p3, t), xmin, xmax);
        }
    }
}

// updates param bbox with the bbox of this segment
static void
bezierPointBboxUpdate(const Point & p0,
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
    bezierPointBboxUpdate(p0, p1, p2, p3, bbox);
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
    for (double t = 0.; t <= 1.; t += incr) {
        Bezier::bezierPoint(p0, p1, p2, p3, t, &cur);
        points->push_back(cur);
    }
    if (bbox) {
        bezierPointBboxUpdate(p0,  p1,  p2,  p3, bbox);
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
    for (double t = 0.; t <= 1.; t += incr) {
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

////////////////////////////////////ControlPoint////////////////////////////////////

BezierCP::BezierCP()
    : _imp( new BezierCPPrivate(boost::shared_ptr<Bezier>()) )
{
}

BezierCP::BezierCP(const BezierCP & other)
    : _imp( new BezierCPPrivate(other._imp->holder.lock()) )
{
    clone(other);
}

BezierCP::BezierCP(const boost::shared_ptr<Bezier>& curve)
    : _imp( new BezierCPPrivate(curve) )
{
}

BezierCP::~BezierCP()
{
}

bool
BezierCP::getPositionAtTime(bool useGuiCurves,
                            double time,
                            double* x,
                            double* y,
                            bool skipMasterOrRelative) const
{
    bool ret;
    KeyFrame k;
    Curve* xCurve = useGuiCurves ? _imp->guiCurveX.get() : _imp->curveX.get();
    Curve* yCurve = useGuiCurves ? _imp->guiCurveY.get() : _imp->curveY.get();
    if ( xCurve->getKeyFrameWithTime(time, &k) ) {
        bool ok;
        *x = k.getValue();
        ok = yCurve->getKeyFrameWithTime(time, &k);
        assert(ok);
        *y = k.getValue();
        ret = true;
    } else {
        try {
            *x = xCurve->getValueAt(time);
            *y = yCurve->getValueAt(time);
        } catch (const std::exception & e) {
            QMutexLocker l(&_imp->staticPositionMutex);
            *x = useGuiCurves ? _imp->guiX : _imp->x;
            *y = useGuiCurves ? _imp->guiY : _imp->y;
        }

        ret = false;
    }

    if (!skipMasterOrRelative) {
        SequenceTime offsetTime;
        Double_Knob* masterTrack;
        {
            QReadLocker l(&_imp->masterMutex);
            offsetTime = _imp->offsetTime;
            masterTrack = _imp->masterTrack ? _imp->masterTrack.get() : NULL;
        }
        if (masterTrack) {
            double masterX = masterTrack->getValueAtTime(time,0);
            double masterY = masterTrack->getValueAtTime(time,1);
            double masterOffsetTimeX = masterTrack->getValueAtTime(offsetTime,0);
            double masterOffsetTimeY = masterTrack->getValueAtTime(offsetTime,1);
            *x += (masterX - masterOffsetTimeX);
            *y += (masterY - masterOffsetTimeY);
        }
    }

    return ret;
}

void
BezierCP::setPositionAtTime(bool useGuiCurves,
                            double time,
                            double x,
                            double y)
{
    {
        KeyFrame k(time,x);
        k.setInterpolation(Natron::eKeyframeTypeLinear);
        if (!useGuiCurves) {
            _imp->curveX->addKeyFrame(k);
        } else {
            _imp->guiCurveX->addKeyFrame(k);
        }
    }
    {
        KeyFrame k(time,y);
        k.setInterpolation(Natron::eKeyframeTypeLinear);
        if (!useGuiCurves) {
            _imp->curveY->addKeyFrame(k);
        } else {
            _imp->guiCurveY->addKeyFrame(k);
        }
    }
}

void
BezierCP::setStaticPosition(bool useGuiCurves,
                            double x,
                            double y)
{
    QMutexLocker l(&_imp->staticPositionMutex);
    if (!useGuiCurves) {
        _imp->x = x;
        _imp->y = y;
    } else {
        _imp->guiX = x;
        _imp->guiY = y;
    }
}

void
BezierCP::setLeftBezierStaticPosition(bool useGuiCurves,
                                      double x,
                                      double y)
{
    QMutexLocker l(&_imp->staticPositionMutex);
    if (!useGuiCurves) {
        _imp->leftX = x;
        _imp->leftY = y;
    } else {
        _imp->guiLeftX = x;
        _imp->guiLeftY = y;
    }
}

void
BezierCP::setRightBezierStaticPosition(bool useGuiCurves,
                                       double x,
                                       double y)
{
    QMutexLocker l(&_imp->staticPositionMutex);
    if (!useGuiCurves) {
        _imp->rightX = x;
        _imp->rightY = y;
    } else {
        _imp->guiRightX = x;
        _imp->guiRightY = y;
    }
}

bool
BezierCP::getLeftBezierPointAtTime(bool useGuiCurves,
                                   double time,
                                   double* x,
                                   double* y,
                                   bool skipMasterOrRelative) const
{
    KeyFrame k;
    bool ret;

    Curve* xCurve = useGuiCurves ? _imp->guiCurveLeftBezierX.get() : _imp->curveLeftBezierX.get();
    Curve* yCurve = useGuiCurves ? _imp->guiCurveLeftBezierY.get() : _imp->curveLeftBezierY.get();
    if ( xCurve->getKeyFrameWithTime(time, &k) ) {
        bool ok;
        *x = k.getValue();
        ok = yCurve->getKeyFrameWithTime(time, &k);
        assert(ok);
        *y = k.getValue();
        ret =  true;
    } else {
        try {
            *x = xCurve->getValueAt(time);
            *y = yCurve->getValueAt(time);
        } catch (const std::exception & e) {
            QMutexLocker l(&_imp->staticPositionMutex);
            if (!useGuiCurves) {
                *x = _imp->leftX;
                *y = _imp->leftY;
            } else {
                *x = _imp->guiLeftX;
                *y = _imp->guiLeftY;
            }
        }

        ret =  false;
    }

    if (!skipMasterOrRelative) {
        Double_Knob* masterTrack;
        SequenceTime offsetTime;
        {
            QReadLocker l(&_imp->masterMutex);
            masterTrack = _imp->masterTrack ? _imp->masterTrack.get() : NULL;
            offsetTime = _imp->offsetTime;
        }
        if (masterTrack) {
            double masterX = masterTrack->getValueAtTime(time,0);
            double masterY = masterTrack->getValueAtTime(time,1);
            double masterOffsetTimeX = masterTrack->getValueAtTime(offsetTime,0);
            double masterOffsetTimeY = masterTrack->getValueAtTime(offsetTime,1);
            *x += (masterX - masterOffsetTimeX);
            *y += (masterY - masterOffsetTimeY);
        }
    }

    return ret;
}

bool
BezierCP::getRightBezierPointAtTime(bool useGuiCurves,
                                    double time,
                                    double *x,
                                    double *y,
                                    bool skipMasterOrRelative) const
{
    KeyFrame k;
    bool ret;
    Curve* xCurve = useGuiCurves ? _imp->guiCurveRightBezierX.get() : _imp->curveRightBezierX.get();
    Curve* yCurve = useGuiCurves ? _imp->guiCurveRightBezierY.get() : _imp->curveRightBezierY.get();
    if ( xCurve->getKeyFrameWithTime(time, &k) ) {
        bool ok;
        *x = k.getValue();
        ok = yCurve->getKeyFrameWithTime(time, &k);
        assert(ok);
        *y = k.getValue();
        ret = true;
    } else {
        try {
            *x = xCurve->getValueAt(time);
            *y = yCurve->getValueAt(time);
        } catch (const std::exception & e) {
            QMutexLocker l(&_imp->staticPositionMutex);
            if (!useGuiCurves) {
                *x = _imp->rightX;
                *y = _imp->rightY;
            } else {
                *x = _imp->guiRightX;
                *y = _imp->guiRightY;
            }
        }

        ret =  false;
    }


    if (!skipMasterOrRelative) {
        Double_Knob* masterTrack;
        SequenceTime offsetTime;
        {
            QReadLocker l(&_imp->masterMutex);
            masterTrack = _imp->masterTrack ? _imp->masterTrack.get() : NULL;
            offsetTime = _imp->offsetTime;
        }
        if (masterTrack) {
            double masterX = masterTrack->getValueAtTime(time,0);
            double masterY = masterTrack->getValueAtTime(time,1);
            double masterOffsetTimeX = masterTrack->getValueAtTime(offsetTime,0);
            double masterOffsetTimeY = masterTrack->getValueAtTime(offsetTime,1);
            *x += (masterX - masterOffsetTimeX);
            *y += (masterY - masterOffsetTimeY);
        }
    }

    return ret;
}

void
BezierCP::setLeftBezierPointAtTime(bool useGuiCurves,
                                   double time,
                                   double x,
                                   double y)
{
    {
        KeyFrame k(time,x);
        k.setInterpolation(Natron::eKeyframeTypeLinear);
        if (!useGuiCurves) {
            _imp->curveLeftBezierX->addKeyFrame(k);
        } else {
            _imp->guiCurveLeftBezierX->addKeyFrame(k);
        }
    }
    {
        KeyFrame k(time,y);
        k.setInterpolation(Natron::eKeyframeTypeLinear);
        if (!useGuiCurves) {
            _imp->curveLeftBezierY->addKeyFrame(k);
        } else {
            _imp->guiCurveLeftBezierY->addKeyFrame(k);
        }
    }
}

void
BezierCP::setRightBezierPointAtTime(bool useGuiCurves,
                                    double time,
                                    double x,
                                    double y)
{
    {
        KeyFrame k(time,x);
        k.setInterpolation(Natron::eKeyframeTypeLinear);
        if (!useGuiCurves) {
            _imp->curveRightBezierX->addKeyFrame(k);
        } else {
            _imp->guiCurveRightBezierX->addKeyFrame(k);
        }
    }
    {
        KeyFrame k(time,y);
        k.setInterpolation(Natron::eKeyframeTypeLinear);
        if (!useGuiCurves) {
            _imp->curveRightBezierY->addKeyFrame(k);
        } else {
            _imp->guiCurveRightBezierY->addKeyFrame(k);
        }
    }
}


void
BezierCP::removeAnimation(bool useGuiCurves,int currentTime)
{
    {
        QMutexLocker k(&_imp->staticPositionMutex);
        try {
            if (!useGuiCurves) {
                _imp->x = _imp->curveX->getValueAt(currentTime);
                _imp->y = _imp->curveY->getValueAt(currentTime);
                _imp->leftX = _imp->curveLeftBezierX->getValueAt(currentTime);
                _imp->leftY = _imp->curveLeftBezierY->getValueAt(currentTime);
                _imp->rightX = _imp->curveRightBezierX->getValueAt(currentTime);
                _imp->rightY = _imp->curveRightBezierY->getValueAt(currentTime);
            } else {
                _imp->guiX = _imp->guiCurveX->getValueAt(currentTime);
                _imp->guiY = _imp->guiCurveY->getValueAt(currentTime);
                _imp->guiLeftX = _imp->guiCurveLeftBezierX->getValueAt(currentTime);
                _imp->guiLeftY = _imp->guiCurveLeftBezierY->getValueAt(currentTime);
                _imp->guiRightX = _imp->guiCurveRightBezierX->getValueAt(currentTime);
                _imp->guiRightY = _imp->guiCurveRightBezierY->getValueAt(currentTime);

            }
        } catch (const std::exception & e) {
            //
        }
    }
    if (!useGuiCurves) {
        _imp->curveX->clearKeyFrames();
        _imp->curveY->clearKeyFrames();
        _imp->curveLeftBezierX->clearKeyFrames();
        _imp->curveRightBezierX->clearKeyFrames();
        _imp->curveLeftBezierY->clearKeyFrames();
        _imp->curveRightBezierY->clearKeyFrames();
    } else {
        _imp->guiCurveX->clearKeyFrames();
        _imp->guiCurveY->clearKeyFrames();
        _imp->guiCurveLeftBezierX->clearKeyFrames();
        _imp->guiCurveRightBezierX->clearKeyFrames();
        _imp->guiCurveLeftBezierY->clearKeyFrames();
        _imp->guiCurveRightBezierY->clearKeyFrames();
    }
}

void
BezierCP::removeKeyframe(bool useGuiCurves,double time)
{
    ///only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );

    ///if the keyframe count reaches 0 update the "static" values which may be fetched
    if (_imp->curveX->getKeyFramesCount() == 1) {
        QMutexLocker l(&_imp->staticPositionMutex);
        if (!useGuiCurves) {
            _imp->x = _imp->curveX->getValueAt(time);
            _imp->y = _imp->curveY->getValueAt(time);
            _imp->leftX = _imp->curveLeftBezierX->getValueAt(time);
            _imp->leftY = _imp->curveLeftBezierY->getValueAt(time);
            _imp->rightX = _imp->curveRightBezierX->getValueAt(time);
            _imp->rightY = _imp->curveRightBezierY->getValueAt(time);
        } else {
            _imp->guiX = _imp->guiCurveX->getValueAt(time);
            _imp->guiY = _imp->guiCurveY->getValueAt(time);
            _imp->guiLeftX = _imp->guiCurveLeftBezierX->getValueAt(time);
            _imp->guiLeftY = _imp->guiCurveLeftBezierY->getValueAt(time);
            _imp->guiRightX = _imp->guiCurveRightBezierX->getValueAt(time);
            _imp->guiRightY = _imp->guiCurveRightBezierY->getValueAt(time);
        }
    }

    try {
        if (!useGuiCurves) {
            _imp->curveX->removeKeyFrameWithTime(time);
            _imp->curveY->removeKeyFrameWithTime(time);
            _imp->curveLeftBezierX->removeKeyFrameWithTime(time);
            _imp->curveRightBezierX->removeKeyFrameWithTime(time);
            _imp->curveLeftBezierY->removeKeyFrameWithTime(time);
            _imp->curveRightBezierY->removeKeyFrameWithTime(time);
        } else {
            _imp->guiCurveX->removeKeyFrameWithTime(time);
            _imp->guiCurveY->removeKeyFrameWithTime(time);
            _imp->guiCurveLeftBezierX->removeKeyFrameWithTime(time);
            _imp->guiCurveRightBezierX->removeKeyFrameWithTime(time);
            _imp->guiCurveLeftBezierY->removeKeyFrameWithTime(time);
            _imp->guiCurveRightBezierY->removeKeyFrameWithTime(time);
        }
    } catch (...) {
    }
}



bool
BezierCP::hasKeyFrameAtTime(bool useGuiCurves,double time) const
{
    KeyFrame k;
    if (!useGuiCurves) {
        return _imp->curveX->getKeyFrameWithTime(time, &k);
    } else {
        return _imp->guiCurveX->getKeyFrameWithTime(time, &k);
    }
}

void
BezierCP::getKeyframeTimes(bool useGuiCurves,std::set<int>* times) const
{
    KeyFrameSet set;
    if (!useGuiCurves) {
        set = _imp->curveX->getKeyFrames_mt_safe();
    } else {
        set = _imp->guiCurveX->getKeyFrames_mt_safe();
    }

    for (KeyFrameSet::iterator it = set.begin(); it != set.end(); ++it) {
        times->insert( (int)it->getTime() );
    }
}

void
BezierCP::getKeyFrames(bool useGuiCurves,std::list<std::pair<int,Natron::KeyframeTypeEnum> >* keys) const
{
    KeyFrameSet set;
    if (!useGuiCurves) {
        set = _imp->curveX->getKeyFrames_mt_safe();
    } else {
        set = _imp->guiCurveX->getKeyFrames_mt_safe();
    }
    for (KeyFrameSet::iterator it = set.begin(); it != set.end(); ++it) {
        keys->push_back(std::make_pair(it->getTime(), it->getInterpolation()));
    }
}

int
BezierCP::getKeyFrameIndex(bool useGuiCurves,double time) const
{
    if (!useGuiCurves) {
        return _imp->curveX->keyFrameIndex(time);
    } else {
        return _imp->guiCurveX->keyFrameIndex(time);
    }
}

void
BezierCP::setKeyFrameInterpolation(bool useGuiCurves,Natron::KeyframeTypeEnum interp,int index)
{
    if (!useGuiCurves) {
        _imp->curveX->setKeyFrameInterpolation(interp, index);
        _imp->curveY->setKeyFrameInterpolation(interp, index);
        _imp->curveLeftBezierX->setKeyFrameInterpolation(interp, index);
        _imp->curveLeftBezierY->setKeyFrameInterpolation(interp, index);
        _imp->curveRightBezierX->setKeyFrameInterpolation(interp, index);
        _imp->curveRightBezierY->setKeyFrameInterpolation(interp, index);
    } else {
        _imp->guiCurveX->setKeyFrameInterpolation(interp, index);
        _imp->guiCurveY->setKeyFrameInterpolation(interp, index);
        _imp->guiCurveLeftBezierX->setKeyFrameInterpolation(interp, index);
        _imp->guiCurveLeftBezierY->setKeyFrameInterpolation(interp, index);
        _imp->guiCurveRightBezierX->setKeyFrameInterpolation(interp, index);
        _imp->guiCurveRightBezierY->setKeyFrameInterpolation(interp, index);
    }
}

int
BezierCP::getKeyframeTime(bool useGuiCurves,int index) const
{
    KeyFrame k;
    bool ok ;
    if (!useGuiCurves) {
        ok = _imp->curveX->getKeyFrameWithIndex(index, &k);
    } else {
        ok = _imp->guiCurveX->getKeyFrameWithIndex(index, &k);
    }

    if (ok) {
        return k.getTime();
    } else {
        return INT_MAX;
    }
}

int
BezierCP::getKeyframesCount(bool useGuiCurves) const
{
    return !useGuiCurves ? _imp->curveX->getKeyFramesCount() : _imp->guiCurveX->getKeyFramesCount();
}

int
BezierCP::getControlPointsCount() const
{
    boost::shared_ptr<Bezier> b = _imp->holder.lock();
    assert(b);
    return b->getControlPointsCount();
}

boost::shared_ptr<Bezier>
BezierCP::getBezier() const
{
    boost::shared_ptr<Bezier> b = _imp->holder.lock();
    assert(b);
    return b;
}

int
BezierCP::isNearbyTangent(bool useGuiCurves,
                          double time,
                          double x,
                          double y,
                          double acceptance) const
{

    Transform::Point3D p,left,right;
    p.z = left.z = right.z = 1;
    
    Transform::Matrix3x3 transform;
    getBezier()->getTransformAtTime(time, &transform);
    
    getPositionAtTime(useGuiCurves,time, &p.x, &p.y);
    getLeftBezierPointAtTime(useGuiCurves,time, &left.x, &left.y);
    getRightBezierPointAtTime(useGuiCurves,time, &right.x, &right.y);
    
    p = Transform::matApply(transform, p);
    left = Transform::matApply(transform, left);
    right = Transform::matApply(transform, right);
    
    if ( (p.x != left.x) || (p.y != left.y) ) {
        if ( ( left.x >= (x - acceptance) ) && ( left.x <= (x + acceptance) ) && ( left.y >= (y - acceptance) ) && ( left.y <= (y + acceptance) ) ) {
            return 0;
        }
    }
    if ( (p.x != right.x) || (p.y != right.y) ) {
        if ( ( right.x >= (x - acceptance) ) && ( right.x <= (x + acceptance) ) && ( right.y >= (y - acceptance) ) && ( right.y <= (y + acceptance) ) ) {
            return 1;
        }
    }

    return -1;
}

#define TANGENTS_CUSP_LIMIT 25
namespace {
static void
cuspTangent(double x,
            double y,
            double *tx,
            double *ty,
            const std::pair<double,double>& pixelScale)
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
        double newDx = 0.9 * dx ;
        double newDy = 0.9 * dy;
        *tx = x + newDx;
        *ty = y + newDy;
    }
}

static void
smoothTangent(bool useGuiCurves,
              double time,
              bool left,
              const BezierCP* p,
              const Transform::Matrix3x3& transform,
              double x,
              double y,
              double *tx,
              double *ty,
              const std::pair<double,double>& pixelScale)
{
    if ( (x == *tx) && (y == *ty) ) {
        
        const std::list < boost::shared_ptr<BezierCP> > & cps = ( p->isFeatherPoint() ?
                                                                  p->getBezier()->getFeatherPoints() :
                                                                  p->getBezier()->getControlPoints() );

        if (cps.size() == 1) {
            return;
        }

        std::list < boost::shared_ptr<BezierCP> >::const_iterator prev = cps.end();
        if (prev != cps.begin()) {
            --prev;
        }
        std::list < boost::shared_ptr<BezierCP> >::const_iterator next = cps.begin();
        if (next != cps.end()) {
            ++next;
        }

        int index = 0;
        int cpCount = (int)cps.size();
        for (std::list < boost::shared_ptr<BezierCP> >::const_iterator it = cps.begin();
             it != cps.end();
             ++it) {
            if ( prev == cps.end() ) {
                prev = cps.begin();
            }
            if ( next == cps.end() ) {
                next = cps.begin();
            }
            if (it->get() == p) {
                break;
            }

            // increment for next iteration
            if (prev != cps.end()) {
                ++prev;
            }
            if (next != cps.end()) {
                ++next;
            }
            ++index;
        } // for(it)

        assert(index < cpCount);
        Q_UNUSED(cpCount);

        double leftDx,leftDy,rightDx,rightDy;
        Bezier::leftDerivativeAtPoint(useGuiCurves,time, *p, **prev, transform, &leftDx, &leftDy);
        Bezier::rightDerivativeAtPoint(useGuiCurves,time, *p, **next, transform, &rightDx, &rightDy);
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
        double newDx,newDy;
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
}

bool
BezierCP::cuspPoint(bool useGuiCurves,
                    double time,
                    bool autoKeying,
                    bool rippleEdit,
                    const std::pair<double,double>& pixelScale)
{
    ///only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    {
        ///update the offset time
        QWriteLocker l(&_imp->masterMutex);
        if (_imp->masterTrack) {
            _imp->offsetTime = time;
        }
    }

    double x,y,leftX,leftY,rightX,rightY;
    getPositionAtTime(useGuiCurves,time, &x, &y,true);
    getLeftBezierPointAtTime(useGuiCurves,time, &leftX, &leftY,true);
    bool isOnKeyframe = getRightBezierPointAtTime(useGuiCurves,time, &rightX, &rightY,true);
    double newLeftX = leftX,newLeftY = leftY,newRightX = rightX,newRightY = rightY;
    cuspTangent(x, y, &newLeftX, &newLeftY, pixelScale);
    cuspTangent(x, y, &newRightX, &newRightY, pixelScale);

    bool keyframeSet = false;

    if (autoKeying || isOnKeyframe) {
        setLeftBezierPointAtTime(useGuiCurves,time, newLeftX, newLeftY);
        setRightBezierPointAtTime(useGuiCurves,time, newRightX, newRightY);
        if (!isOnKeyframe) {
            keyframeSet = true;
        }
    }

    if (rippleEdit) {
        std::set<int> times;
        getKeyframeTimes(useGuiCurves,&times);
        for (std::set<int>::iterator it = times.begin(); it != times.end(); ++it) {
            setLeftBezierPointAtTime(useGuiCurves,*it, newLeftX, newLeftY);
            setRightBezierPointAtTime(useGuiCurves,*it, newRightX, newRightY);
        }
    }

    return keyframeSet;
}

bool
BezierCP::smoothPoint(bool useGuiCurves,
                      double time,
                      bool autoKeying,
                      bool rippleEdit,
                      const std::pair<double,double>& pixelScale)
{
    ///only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    {
        ///update the offset time
        QWriteLocker l(&_imp->masterMutex);
        if (_imp->masterTrack) {
            _imp->offsetTime = time;
        }
    }
    Transform::Matrix3x3 transform;
    getBezier()->getTransformAtTime(time, &transform);
    
    Transform::Point3D pos,left,right;
    pos.z = left.z = right.z = 1.;
    
    getPositionAtTime(useGuiCurves,time, &pos.x, &pos.y,true);
    
    getLeftBezierPointAtTime(useGuiCurves, time, &left.x, &left.y,true);
    bool isOnKeyframe = getRightBezierPointAtTime(useGuiCurves, time, &right.x, &right.y,true);

    pos = Transform::matApply(transform, pos);
    left = Transform::matApply(transform, left);
    right = Transform::matApply(transform, right);
    
    smoothTangent(useGuiCurves,time,true,this , transform, pos.x, pos.y, &left.x, &left.y, pixelScale);
    smoothTangent(useGuiCurves,time,false,this, transform, pos.x, pos.y, &right.x, &right.y, pixelScale);

    bool keyframeSet = false;

    transform = Transform::matInverse(transform);
    
    if (autoKeying || isOnKeyframe) {
        setLeftBezierPointAtTime(useGuiCurves, time, left.x, left.y);
        setRightBezierPointAtTime(useGuiCurves, time, right.x, right.y);
        if (!isOnKeyframe) {
            keyframeSet = true;
        }
    }

    if (rippleEdit) {
        std::set<int> times;
        getKeyframeTimes(useGuiCurves, &times);
        for (std::set<int>::iterator it = times.begin(); it != times.end(); ++it) {
            setLeftBezierPointAtTime(useGuiCurves, *it, left.x, left.y);
            setRightBezierPointAtTime(useGuiCurves, *it, right.x, right.y);
        }
    }

    return keyframeSet;
}

boost::shared_ptr<Curve>
BezierCP::getXCurve() const
{
    return _imp->curveX;
}

boost::shared_ptr<Curve>
BezierCP::getYCurve() const
{
    return _imp->curveY;
}

boost::shared_ptr<Curve>
BezierCP::getLeftXCurve() const
{
    return _imp->curveLeftBezierX;
}

boost::shared_ptr<Curve>
BezierCP::getLeftYCurve() const
{
    return _imp->curveLeftBezierY;
}

boost::shared_ptr<Curve>
BezierCP::getRightXCurve() const
{
    return _imp->curveRightBezierX;
}

boost::shared_ptr<Curve>
BezierCP::getRightYCurve() const
{
    return _imp->curveRightBezierY;
}

void
BezierCP::clone(const BezierCP & other)
{
    _imp->curveX->clone(*other._imp->curveX);
    _imp->curveY->clone(*other._imp->curveY);
    _imp->curveLeftBezierX->clone(*other._imp->curveLeftBezierX);
    _imp->curveLeftBezierY->clone(*other._imp->curveLeftBezierY);
    _imp->curveRightBezierX->clone(*other._imp->curveRightBezierX);
    _imp->curveRightBezierY->clone(*other._imp->curveRightBezierY);

    _imp->guiCurveX->clone(*other._imp->curveX);
    _imp->guiCurveY->clone(*other._imp->curveY);
    _imp->guiCurveLeftBezierX->clone(*other._imp->curveLeftBezierX);
    _imp->guiCurveLeftBezierY->clone(*other._imp->curveLeftBezierY);
    _imp->guiCurveRightBezierX->clone(*other._imp->curveRightBezierX);
    _imp->guiCurveRightBezierY->clone(*other._imp->curveRightBezierY);
    
    {
        QMutexLocker l(&_imp->staticPositionMutex);
        _imp->x = other._imp->x;
        _imp->y = other._imp->y;
        _imp->leftX = other._imp->leftX;
        _imp->leftY = other._imp->leftY;
        _imp->rightX = other._imp->rightX;
        _imp->rightY = other._imp->rightY;
        
        _imp->guiX = other._imp->x;
        _imp->guiY = other._imp->y;
        _imp->guiLeftX = other._imp->leftX;
        _imp->guiLeftY = other._imp->leftY;
        _imp->guiRightX = other._imp->rightX;
        _imp->guiRightY = other._imp->rightY;
    }

    {
        QWriteLocker l(&_imp->masterMutex);
        _imp->masterTrack = other._imp->masterTrack;
        _imp->offsetTime = other._imp->offsetTime;
    }
}

bool
BezierCP::equalsAtTime(bool useGuiCurves,
                       double time,
                       const BezierCP & other) const
{
    double x,y,leftX,leftY,rightX,rightY;

    getPositionAtTime(useGuiCurves, time, &x, &y,true);
    getLeftBezierPointAtTime(useGuiCurves, time, &leftX, &leftY,true);
    getRightBezierPointAtTime(useGuiCurves, time, &rightX, &rightY,true);

    double ox,oy,oLeftX,oLeftY,oRightX,oRightY;
    other.getPositionAtTime(useGuiCurves, time, &ox, &oy,true);
    other.getLeftBezierPointAtTime(useGuiCurves, time, &oLeftX, &oLeftY,true);
    other.getRightBezierPointAtTime(useGuiCurves, time, &oRightX, &oRightY,true);

    if ( (x == ox) && (y == oy) && (leftX == oLeftX) && (leftY == oLeftY) && (rightX == oRightX) && (rightY == oRightY) ) {
        return true;
    }

    return false;
}

SequenceTime
BezierCP::getOffsetTime() const
{
    QReadLocker l(&_imp->masterMutex);

    return _imp->offsetTime;
}

void
BezierCP::cloneInternalCurvesToGuiCurves()
{
    _imp->guiCurveX->clone(*_imp->curveX);
    _imp->guiCurveY->clone(*_imp->curveY);
    
    
    _imp->guiCurveLeftBezierX->clone(*_imp->curveLeftBezierX);
    _imp->guiCurveLeftBezierY->clone(*_imp->curveLeftBezierY);
    _imp->guiCurveRightBezierX->clone(*_imp->curveRightBezierX);
    _imp->guiCurveRightBezierY->clone(*_imp->curveRightBezierY);
    
    QMutexLocker k(&_imp->staticPositionMutex);
    _imp->guiX = _imp->x;
    _imp->guiY = _imp->y;
    _imp->guiLeftX = _imp->leftX;
    _imp->guiLeftY = _imp->leftY;
    _imp->guiRightX = _imp->rightX;
    _imp->guiRightY = _imp->rightY;
}

void
BezierCP::cloneGuiCurvesToInternalCurves()
{
    _imp->curveX->clone(*_imp->guiCurveX);
    _imp->curveY->clone(*_imp->guiCurveY);
    
    
    _imp->curveLeftBezierX->clone(*_imp->guiCurveLeftBezierX);
    _imp->curveLeftBezierY->clone(*_imp->guiCurveLeftBezierY);
    _imp->curveRightBezierX->clone(*_imp->guiCurveRightBezierX);
    _imp->curveRightBezierY->clone(*_imp->guiCurveRightBezierY);
    
    QMutexLocker k(&_imp->staticPositionMutex);
    _imp->x = _imp->guiX;
    _imp->y = _imp->guiY;
    _imp->leftX = _imp->guiLeftX;
    _imp->leftY = _imp->guiLeftY;
    _imp->rightX = _imp->guiRightX;
    _imp->rightY = _imp->guiRightY;
}

void
BezierCP::slaveTo(SequenceTime offsetTime,
                  const boost::shared_ptr<Double_Knob> & track)
{
    assert( QThread::currentThread() == qApp->thread() );
    assert(!_imp->masterTrack);
    QWriteLocker l(&_imp->masterMutex);
    _imp->masterTrack = track;
    _imp->offsetTime = offsetTime;
}

void
BezierCP::unslave()
{
    assert( QThread::currentThread() == qApp->thread() );
    assert(_imp->masterTrack);
    QWriteLocker l(&_imp->masterMutex);
    _imp->masterTrack.reset();
}

boost::shared_ptr<Double_Knob>
BezierCP::isSlaved() const
{
    QReadLocker l(&_imp->masterMutex);

    return _imp->masterTrack;
}

////////////////////////////////////RotoItem////////////////////////////////////
namespace {
class RotoMetaTypesRegistration
{
public:
    inline RotoMetaTypesRegistration()
    {
        qRegisterMetaType<RotoItem*>("RotoItem");
    }
};
}

static RotoMetaTypesRegistration registration;
RotoItem::RotoItem(const boost::shared_ptr<RotoContext>& context,
                   const std::string & name,
                   boost::shared_ptr<RotoLayer> parent)
    : itemMutex()
      , _imp( new RotoItemPrivate(context,name,parent) )
{
}

RotoItem::~RotoItem()
{
}

void
RotoItem::clone(const RotoItem*  other)
{
    QMutexLocker l(&itemMutex);

    _imp->parentLayer = other->_imp->parentLayer;
    _imp->scriptName = other->_imp->scriptName;
    _imp->label = other->_imp->label;
    _imp->globallyActivated = other->_imp->globallyActivated;
    _imp->locked = other->_imp->locked;
}

void
RotoItem::setParentLayer(boost::shared_ptr<RotoLayer> layer)
{
    ///called on the main-thread only
    assert( QThread::currentThread() == qApp->thread() );

    RotoStrokeItem* isStroke = dynamic_cast<RotoStrokeItem*>(this);
    if (isStroke) {
        if (!layer) {
            isStroke->deactivateNodes();
        } else {
            isStroke->activateNodes();
        }
    }
    
    QMutexLocker l(&itemMutex);
    _imp->parentLayer = layer;
}

boost::shared_ptr<RotoLayer>
RotoItem::getParentLayer() const
{
    QMutexLocker l(&itemMutex);
    return _imp->parentLayer.lock();
}

void
RotoItem::setGloballyActivated_recursive(bool a)
{
    {
        QMutexLocker l(&itemMutex);
        _imp->globallyActivated = a;
        RotoLayer* layer = dynamic_cast<RotoLayer*>(this);
        if (layer) {
            const RotoItems & children = layer->getItems();
            for (RotoItems::const_iterator it = children.begin(); it != children.end(); ++it) {
                (*it)->setGloballyActivated_recursive(a);
            }
        }
    }
}

void
RotoItem::setGloballyActivated(bool a,
                               bool setChildren)
{
    ///called on the main-thread only
    assert( QThread::currentThread() == qApp->thread() );
    if (setChildren) {
        setGloballyActivated_recursive(a);
    } else {
        QMutexLocker l(&itemMutex);
        _imp->globallyActivated = a;
    }
    boost::shared_ptr<RotoContext> c = _imp->context.lock();
    if (c) {
        RotoDrawableItem* isDrawable = dynamic_cast<RotoDrawableItem*>(this);
        if (isDrawable) {
            isDrawable->incrementNodesAge();
        }
        c->evaluateChange();
    }
}

bool
RotoItem::isGloballyActivated() const
{
    QMutexLocker l(&itemMutex);

    return _imp->globallyActivated;
}

static bool
isDeactivated_imp(const boost::shared_ptr<RotoLayer>& item)
{
    if ( !item->isGloballyActivated() ) {
        return true;
    } else {
        boost::shared_ptr<RotoLayer> parent = item->getParentLayer();
        if (parent) {
            return isDeactivated_imp(parent);
        }
    }
    return false;
}

bool
RotoItem::isDeactivatedRecursive() const
{
    boost::shared_ptr<RotoLayer> parent;
    {
        QMutexLocker l(&itemMutex);
        if (!_imp->globallyActivated) {
            return true;
        }
        parent = _imp->parentLayer.lock();
    }

    if (parent) {
        return isDeactivated_imp(parent);
    }
    return false;
}

void
RotoItem::setLocked_recursive(bool locked,RotoItem::SelectionReasonEnum reason)
{
    {
        {
            QMutexLocker m(&itemMutex);
            _imp->locked = locked;
        }
        getContext()->onItemLockedChanged(shared_from_this(),reason);
        RotoLayer* layer = dynamic_cast<RotoLayer*>(this);
        if (layer) {
            const RotoItems & children = layer->getItems();
            for (RotoItems::const_iterator it = children.begin(); it != children.end(); ++it) {
                (*it)->setLocked_recursive(locked,reason);
            }
        }
    }
}

void
RotoItem::setLocked(bool l,
                    bool lockChildren,
                    RotoItem::SelectionReasonEnum reason)
{
    ///called on the main-thread only
    assert( QThread::currentThread() == qApp->thread() );
    if (!lockChildren) {
        {
            QMutexLocker m(&itemMutex);
            _imp->locked = l;
        }
        getContext()->onItemLockedChanged(shared_from_this(),reason);
    } else {
        setLocked_recursive(l,reason);
    }
}

bool
RotoItem::getLocked() const
{
    QMutexLocker l(&itemMutex);

    return _imp->locked;
}

static
bool
isLocked_imp(const boost::shared_ptr<RotoLayer>& item)
{
    if ( item->getLocked() ) {
        return true;
    } else {
        boost::shared_ptr<RotoLayer> parent = item->getParentLayer();
        if (parent) {
            return isLocked_imp(parent);
        }
    }
    return false;
}

bool
RotoItem::isLockedRecursive() const
{
    boost::shared_ptr<RotoLayer> parent;
    {
        QMutexLocker l(&itemMutex);
        if (_imp->locked) {
            return true;
        }
        parent = _imp->parentLayer.lock();
    }

    if (parent) {
        return isLocked_imp(parent);
    } else {
        return false;
    }
}

int
RotoItem::getHierarchyLevel() const
{
    int ret = 0;
    boost::shared_ptr<RotoLayer> parent;

    {
        QMutexLocker l(&itemMutex);
        parent = _imp->parentLayer.lock();
    }

    while (parent) {
        parent = parent->getParentLayer();
        ++ret;
    }

    return ret;
}

boost::shared_ptr<RotoContext>
RotoItem::getContext() const
{
    return _imp->context.lock();
}

bool
RotoItem::setScriptName(const std::string & name)
{
    ///called on the main-thread only
    assert( QThread::currentThread() == qApp->thread() );
    
    if (name.empty()) {
        return false;
    }
    
    
    std::string cpy = Natron::makeNameScriptFriendly(name);
    
    if (cpy.empty()) {
        return false;
    }
    
    boost::shared_ptr<RotoItem> existingItem = getContext()->getItemByName(name);
    if ( existingItem && (existingItem.get() != this) ) {
        return false;
    }
    
    std::string oldFullName = getFullyQualifiedName();
    bool oldNameEmpty;
    {
        QMutexLocker l(&itemMutex);
        oldNameEmpty = _imp->scriptName.empty();
        _imp->scriptName = cpy;
    }
    std::string newFullName = getFullyQualifiedName();
    
    boost::shared_ptr<RotoContext> c = _imp->context.lock();
    if (c) {
        if (!oldNameEmpty) {
            c->changeItemScriptName(oldFullName, newFullName);
        }
        c->onItemScriptNameChanged(shared_from_this());
    }
    return true;
}

static void getScriptNameRecursive(RotoLayer* item,std::string* scriptName)
{
    scriptName->insert(0, ".");
    scriptName->insert(0, item->getScriptName());
    boost::shared_ptr<RotoLayer> parent = item->getParentLayer();
    if (parent) {
        getScriptNameRecursive(parent.get(), scriptName);
    }
}

std::string
RotoItem::getFullyQualifiedName() const
{
    std::string name = getScriptName();
    boost::shared_ptr<RotoLayer> parent = getParentLayer();
    if (parent) {
        getScriptNameRecursive(parent.get(), &name);
    }
    return name;
}

std::string
RotoItem::getScriptName() const
{
    QMutexLocker l(&itemMutex);

    return _imp->scriptName;
}

std::string
RotoItem::getLabel() const
{
    QMutexLocker l(&itemMutex);
    return _imp->label;
}

void
RotoItem::setLabel(const std::string& label)
{
    {
        QMutexLocker l(&itemMutex);
        _imp->label = label;
    }
    boost::shared_ptr<RotoContext> c = _imp->context.lock();
    if (c) {
        c->onItemLabelChanged(shared_from_this());
    }
}

void
RotoItem::save(RotoItemSerialization *obj) const
{
    boost::shared_ptr<RotoLayer> parent;
    {
        QMutexLocker l(&itemMutex);
        obj->activated = _imp->globallyActivated;
        obj->name = _imp->scriptName;
        obj->label = _imp->label;
        obj->locked = _imp->locked;
        parent = _imp->parentLayer.lock();
    }

    if (parent) {
        obj->parentLayerName = parent->getScriptName();
    }
}

void
RotoItem::load(const RotoItemSerialization &obj)
{
    {
        QMutexLocker l(&itemMutex);
        _imp->globallyActivated = obj.activated;
        _imp->locked = obj.locked;
        _imp->scriptName = obj.name;
        if (!obj.label.empty()) {
            _imp->label = obj.label;
        } else {
            _imp->label = _imp->scriptName;
        }
        std::locale loc;
        std::string cpy;
        for (std::size_t i = 0; i < _imp->scriptName.size(); ++i) {
            
            ///Ignore starting digits
            if (cpy.empty() && std::isdigit(_imp->scriptName[i],loc)) {
                continue;
            }
            
            ///Spaces becomes underscores
            if (std::isspace(_imp->scriptName[i],loc)){
                cpy.push_back('_');
            }
            
            ///Non alpha-numeric characters are not allowed in python
            else if (_imp->scriptName[i] == '_' || std::isalnum(_imp->scriptName[i], loc)) {
                cpy.push_back(_imp->scriptName[i]);
            }
        }
        if (!cpy.empty()) {
            _imp->scriptName = cpy;
        } else {
            l.unlock();
            std::string name = getContext()->generateUniqueName(kRotoBezierBaseName);
            l.relock();
            _imp->scriptName = name;
        }
    }
    boost::shared_ptr<RotoLayer> parent = getContext()->getLayerByName(obj.parentLayerName);

    {
        QMutexLocker l(&itemMutex);
        _imp->parentLayer = parent;
    }
}

std::string
RotoItem::getRotoNodeName() const
{
    return getContext()->getRotoNodeName();
}

static boost::shared_ptr<RotoItem> getPreviousInLayer(const boost::shared_ptr<RotoLayer>& layer, const boost::shared_ptr<const RotoItem>& item)
{
    RotoItems layerItems = layer->getItems_mt_safe();
    if (layerItems.empty()) {
        return boost::shared_ptr<RotoItem>();
    }
    RotoItems::iterator found = layerItems.end();
    if (item) {
        for (RotoItems::iterator it = layerItems.begin(); it != layerItems.end(); ++it) {
            if (*it == item) {
                found = it;
                break;
            }
        }
        assert(found != layerItems.end());
    } else {
        found = layerItems.end();
    }
    
    if (found != layerItems.end()) {
        ++found;
        if (found != layerItems.end()) {
            return *found;
        }
    }
    
    //Item was still not found, find in great parent layer
    boost::shared_ptr<RotoLayer> parentLayer = layer->getParentLayer();
    if (!parentLayer) {
        return boost::shared_ptr<RotoItem>();
    }
    RotoItems greatParentItems = parentLayer->getItems_mt_safe();
    
    found = greatParentItems.end();
    for (RotoItems::iterator it = greatParentItems.begin(); it != greatParentItems.end(); ++it) {
        if (*it == layer) {
            found = it;
            break;
        }
    }
    assert(found != greatParentItems.end());
    boost::shared_ptr<RotoItem> ret = getPreviousInLayer(parentLayer, layer);
    assert(ret != item);
    return ret;
}

boost::shared_ptr<RotoItem>
RotoItem::getPreviousItemInLayer() const
{
    boost::shared_ptr<RotoLayer> layer = getParentLayer();
    if (!layer) {
        return boost::shared_ptr<RotoItem>();
    }
    return getPreviousInLayer(layer, shared_from_this());
}

////////////////////////////////////RotoDrawableItem////////////////////////////////////

RotoDrawableItem::RotoDrawableItem(const boost::shared_ptr<RotoContext>& context,
                                   const std::string & name,
                                   const boost::shared_ptr<RotoLayer>& parent,
                                   bool isStroke)
    : RotoItem(context,name,parent)
      , _imp( new RotoDrawableItemPrivate(isStroke) )
{
#ifdef NATRON_ROTO_INVERTIBLE
    QObject::connect( _imp->inverted->getSignalSlotHandler().get(), SIGNAL( valueChanged(int,int) ), this, SIGNAL( invertedStateChanged() ) );
#endif
    QObject::connect( this, SIGNAL( overlayColorChanged() ), context.get(), SIGNAL( refreshViewerOverlays() ) );
    QObject::connect( _imp->color->getSignalSlotHandler().get(), SIGNAL( valueChanged(int,int) ), this, SIGNAL( shapeColorChanged() ) );
    QObject::connect( _imp->compOperator->getSignalSlotHandler().get(), SIGNAL( valueChanged(int,int) ), this,
                      SIGNAL( compositingOperatorChanged(int,int) ) );
    
    std::vector<std::string> operators;
    std::vector<std::string> tooltips;
    getNatronCompositingOperators(&operators, &tooltips);
    
    _imp->compOperator->populateChoices(operators,tooltips);
    _imp->compOperator->setDefaultValueFromLabel(getNatronOperationString(eMergeCopy));
    
}

RotoDrawableItem::~RotoDrawableItem()
{
}

void
RotoDrawableItem::addKnob(const boost::shared_ptr<KnobI>& knob)
{
    _imp->knobs.push_back(knob);
}

void
RotoDrawableItem::setNodesThreadSafetyForRotopainting()
{
    
    assert(boost::dynamic_pointer_cast<RotoStrokeItem>(boost::dynamic_pointer_cast<RotoDrawableItem>(shared_from_this())));
    
    getContext()->getNode()->setRenderThreadSafety(Natron::eRenderSafetyInstanceSafe);
    getContext()->getNode()->setWhileCreatingPaintStroke(true);
    if (_imp->effectNode) {
        _imp->effectNode->setWhileCreatingPaintStroke(true);
        _imp->effectNode->setRenderThreadSafety(Natron::eRenderSafetyInstanceSafe);
    }
    if (_imp->mergeNode) {
        _imp->mergeNode->setWhileCreatingPaintStroke(true);
        _imp->mergeNode->setRenderThreadSafety(Natron::eRenderSafetyInstanceSafe);
    }
    if (_imp->timeOffsetNode) {
        _imp->timeOffsetNode->setWhileCreatingPaintStroke(true);
        _imp->timeOffsetNode->setRenderThreadSafety(Natron::eRenderSafetyInstanceSafe);
    }
    if (_imp->frameHoldNode) {
        _imp->frameHoldNode->setWhileCreatingPaintStroke(true);
        _imp->frameHoldNode->setRenderThreadSafety(Natron::eRenderSafetyInstanceSafe);
    }
}

void
RotoDrawableItem::createNodes(bool connectNodes)
{
    
    const std::list<boost::shared_ptr<KnobI> >& knobs = getKnobs();
    for (std::list<boost::shared_ptr<KnobI> >::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        QObject::connect((*it)->getSignalSlotHandler().get(), SIGNAL(updateDependencies(int,int)), this, SLOT(onRotoKnobChanged(int,int)));
    }
    
    boost::shared_ptr<RotoContext> context = getContext();
    boost::shared_ptr<KnobI> outputChansKnob = context->getNode()->getKnobByName(kOutputChannelsKnobName);
    assert(outputChansKnob);
    QObject::connect(outputChansKnob->getSignalSlotHandler().get(), SIGNAL(valueChanged(int,int)), this, SLOT(onRotoOutputChannelsChanged()));
    
    
    AppInstance* app = context->getNode()->getApp();
    QString fixedNamePrefix(context->getNode()->getScriptName_mt_safe().c_str());
    fixedNamePrefix.append('_');
    fixedNamePrefix.append(getScriptName().c_str());
    fixedNamePrefix.append('_');
    fixedNamePrefix.append(QString::number(context->getAge()));
    fixedNamePrefix.append('_');
    
    QString pluginId;
    
    RotoStrokeType type;
    boost::shared_ptr<RotoDrawableItem> thisShared = boost::dynamic_pointer_cast<RotoDrawableItem>(shared_from_this());
    assert(thisShared);
    boost::shared_ptr<RotoStrokeItem> isStroke = boost::dynamic_pointer_cast<RotoStrokeItem>(thisShared);

    if (isStroke) {
        type = isStroke->getBrushType();
    } else {
        type = eRotoStrokeTypeSolid;
    }
    switch (type) {
        case Natron::eRotoStrokeTypeBlur:
            pluginId = PLUGINID_OFX_BLURCIMG;
            break;
        case Natron::eRotoStrokeTypeEraser:
            pluginId = PLUGINID_OFX_CONSTANT;
            break;
        case Natron::eRotoStrokeTypeSolid:
            pluginId = PLUGINID_OFX_ROTO;
            break;
        case Natron::eRotoStrokeTypeClone:
        case Natron::eRotoStrokeTypeReveal:
            pluginId = PLUGINID_OFX_TRANSFORM;
            break;
        case Natron::eRotoStrokeTypeBurn:
        case Natron::eRotoStrokeTypeDodge:
            //uses merge
            break;
        case Natron::eRotoStrokeTypeSharpen:
            //todo
            break;
        case Natron::eRotoStrokeTypeSmear:
            pluginId = PLUGINID_NATRON_ROTOSMEAR;
            break;
    }
    
    QString baseFixedName = fixedNamePrefix;
    if (!pluginId.isEmpty()) {
        fixedNamePrefix.append("Effect");
        
        CreateNodeArgs args(pluginId, "",
                            -1,-1,
                            false,
                            INT_MIN,
                            INT_MIN,
                            false,
                            false,
                            false,
                            fixedNamePrefix,
                            CreateNodeArgs::DefaultValuesList(),
                            boost::shared_ptr<NodeCollection>());
        args.createGui = false;
        _imp->effectNode = app->createNode(args);
        assert(_imp->effectNode);
        
        if (type == eRotoStrokeTypeClone || type == eRotoStrokeTypeReveal) {
            {
                fixedNamePrefix = baseFixedName;
                fixedNamePrefix.append("TimeOffset");
                CreateNodeArgs args(PLUGINID_OFX_TIMEOFFSET, "",
                                    -1,-1,
                                    false,
                                    INT_MIN,
                                    INT_MIN,
                                    false,
                                    false,
                                    false,
                                    fixedNamePrefix,
                                    CreateNodeArgs::DefaultValuesList(),
                                    boost::shared_ptr<NodeCollection>());
                args.createGui = false;
                _imp->timeOffsetNode = app->createNode(args);
                assert(_imp->timeOffsetNode);
              
            }
            {
                fixedNamePrefix = baseFixedName;
                fixedNamePrefix.append("FrameHold");
                CreateNodeArgs args(PLUGINID_OFX_FRAMEHOLD, "",
                                    -1,-1,
                                    false,
                                    INT_MIN,
                                    INT_MIN,
                                    false,
                                    false,
                                    false,
                                    fixedNamePrefix,
                                    CreateNodeArgs::DefaultValuesList(),
                                    boost::shared_ptr<NodeCollection>());
                args.createGui = false;
                _imp->frameHoldNode = app->createNode(args);
                assert(_imp->frameHoldNode);
               
            }
        }
    }
    
    fixedNamePrefix = baseFixedName;
    fixedNamePrefix.append("Merge");
    CreateNodeArgs args(PLUGINID_OFX_MERGE, "",
                        -1,-1,
                        false,
                        INT_MIN,
                        INT_MIN,
                        false,
                        false,
                        false,
                        fixedNamePrefix,
                        CreateNodeArgs::DefaultValuesList(),
                        boost::shared_ptr<NodeCollection>());
    args.createGui = false;
    
    bool ok = _imp->mergeNode = app->createNode(args);
    assert(ok);
    
    assert(_imp->mergeNode);
    
    if (type != eRotoStrokeTypeSolid) {
        int maxInp = _imp->mergeNode->getMaxInputCount();
        for (int i = 0; i < maxInp; ++i) {
            if (_imp->mergeNode->getLiveInstance()->isInputMask(i)) {
                
                //Connect this rotopaint node as a mask
                ok = _imp->mergeNode->connectInput(context->getNode(), i);
                assert(ok);
                break;
            }
        }
    }
    
    boost::shared_ptr<KnobI> mergeOperatorKnob = _imp->mergeNode->getKnobByName(kMergeOFXParamOperation);
    assert(mergeOperatorKnob);
    Choice_Knob* mergeOp = dynamic_cast<Choice_Knob*>(mergeOperatorKnob.get());
    assert(mergeOp);
    
    boost::shared_ptr<Choice_Knob> compOp = getOperatorKnob();

    MergingFunctionEnum op;
    if (type == eRotoStrokeTypeDodge || type == eRotoStrokeTypeBurn) {
        op = (type == eRotoStrokeTypeDodge ?eMergeColorDodge : eMergeColorBurn);
    } else if (type == eRotoStrokeTypeSolid) {
        op = eMergeOver;
    } else {
        op = eMergeCopy;
    }
    mergeOp->setValueFromLabel(getNatronOperationString(op), 0);
    compOp->setValueFromLabel(getNatronOperationString(op), 0);

    if (isStroke) {
        if (type == eRotoStrokeTypeBlur) {
            double strength = isStroke->getBrushEffectKnob()->getValue();
            boost::shared_ptr<KnobI> knob = _imp->effectNode->getKnobByName(kBlurCImgParamSize);
            Double_Knob* isDbl = dynamic_cast<Double_Knob*>(knob.get());
            if (isDbl) {
                isDbl->setValues(strength, strength, Natron::eValueChangedReasonNatronInternalEdited);
            }
        } else if (type == eRotoStrokeTypeSharpen) {
            //todo
        } else if (type == eRotoStrokeTypeSmear) {
            boost::shared_ptr<Double_Knob> spacingKnob = isStroke->getBrushSpacingKnob();
            assert(spacingKnob);
            spacingKnob->setValue(0.05, 0);
        }
        
        setNodesThreadSafetyForRotopainting();
    }
    
    ///Attach this stroke to the underlying nodes used
    if (_imp->effectNode) {
        _imp->effectNode->attachRotoItem(thisShared);
    }
    if (_imp->mergeNode) {
        _imp->mergeNode->attachRotoItem(thisShared);
    }
    if (_imp->timeOffsetNode) {
        _imp->timeOffsetNode->attachRotoItem(thisShared);
    }
    if (_imp->frameHoldNode) {
        _imp->frameHoldNode->attachRotoItem(thisShared);
    }
    
    
    onRotoOutputChannelsChanged();
    
    if (connectNodes) {
        refreshNodesConnections();
    }

}



void
RotoDrawableItem::disconnectNodes()
{
    _imp->mergeNode->disconnectInput(0);
    _imp->mergeNode->disconnectInput(1);
    if (_imp->effectNode) {
        _imp->effectNode->disconnectInput(0);
    }
    if (_imp->timeOffsetNode) {
        _imp->timeOffsetNode->disconnectInput(0);
    }
    if (_imp->frameHoldNode) {
        _imp->frameHoldNode->disconnectInput(0);
    }
}

void
RotoDrawableItem::deactivateNodes()
{
    if (_imp->effectNode) {
        _imp->effectNode->deactivate(std::list< Node* >(),true,false,false,false);
    }
    if (_imp->mergeNode) {
        _imp->mergeNode->deactivate(std::list< Node* >(),true,false,false,false);
    }
    if (_imp->timeOffsetNode) {
        _imp->timeOffsetNode->deactivate(std::list< Node* >(),true,false,false,false);
    }
    if (_imp->frameHoldNode) {
        _imp->frameHoldNode->deactivate(std::list< Node* >(),true,false,false,false);
    }
}

void
RotoDrawableItem::activateNodes()
{
    if (_imp->effectNode) {
        _imp->effectNode->activate(std::list< Node* >(),false,false);
    }
    _imp->mergeNode->activate(std::list< Node* >(),false,false);
    if (_imp->timeOffsetNode) {
        _imp->timeOffsetNode->activate(std::list< Node* >(),false,false);
    }
    if (_imp->frameHoldNode) {
        _imp->frameHoldNode->activate(std::list< Node* >(),false,false);
    }
}


void
RotoDrawableItem::onRotoOutputChannelsChanged()
{
    
    boost::shared_ptr<KnobI> outputChansKnob = getContext()->getNode()->getKnobByName(kOutputChannelsKnobName);
    assert(outputChansKnob);
    Choice_Knob* outputChannels = dynamic_cast<Choice_Knob*>(outputChansKnob.get());
    assert(outputChannels);
    
    int outputchans_i = outputChannels->getValue();
    std::string rotopaintOutputChannels;
    std::vector<std::string> rotoPaintchannelEntries = outputChannels->getEntries_mt_safe();
    if (outputchans_i  < (int)rotoPaintchannelEntries.size()) {
        rotopaintOutputChannels = rotoPaintchannelEntries[outputchans_i];
    }
    if (rotopaintOutputChannels.empty()) {
        return;
    }
    
    std::list<Node*> nodes;
    if (_imp->mergeNode) {
        nodes.push_back(_imp->mergeNode.get());
    }
    if (_imp->effectNode) {
        nodes.push_back(_imp->effectNode.get());
    }
    if (_imp->timeOffsetNode) {
        nodes.push_back(_imp->timeOffsetNode.get());
    }
    if (_imp->frameHoldNode) {
        nodes.push_back(_imp->frameHoldNode.get());
    }
    for (std::list<Node*>::iterator it = nodes.begin(); it!=nodes.end(); ++it) {
        
        std::list<KnobI*> knobs;
        boost::shared_ptr<KnobI> channelsKnob = (*it)->getKnobByName(kOutputChannelsKnobName);
        if (channelsKnob) {
            knobs.push_back(channelsKnob.get());
        }
        boost::shared_ptr<KnobI> aChans = (*it)->getKnobByName("A_channels");
        if (aChans) {
            knobs.push_back(aChans.get());
        }
        boost::shared_ptr<KnobI> bChans = (*it)->getKnobByName("B_channels");
        if (bChans) {
            knobs.push_back(bChans.get());
        }
        for (std::list<KnobI*>::iterator it = knobs.begin(); it!=knobs.end();++it) {
            Choice_Knob* nodeChannels = dynamic_cast<Choice_Knob*>(*it);
            if (nodeChannels) {
                std::vector<std::string> entries = nodeChannels->getEntries_mt_safe();
                for (std::size_t i = 0; i < entries.size(); ++i) {
                    if (entries[i] == rotopaintOutputChannels) {
                        nodeChannels->setValue(i, 0);
                        break;
                    }
                }
                
            }
        }
    }
}



static RotoDrawableItem* findPreviousOfItemInLayer(RotoLayer* layer, RotoItem* item)
{
    RotoItems layerItems = layer->getItems_mt_safe();
    if (layerItems.empty()) {
        return 0;
    }
    RotoItems::iterator found = layerItems.end();
    if (item) {
        for (RotoItems::iterator it = layerItems.begin(); it != layerItems.end(); ++it) {
            if (it->get() == item) {
                found = it;
                break;
            }
        }
        assert(found != layerItems.end());
    } else {
        found = layerItems.end();
    }
    
    if (found != layerItems.end()) {
        ++found;
        for (; found != layerItems.end(); ++found) {
            
            //We found another stroke below at the same level
            RotoDrawableItem* isDrawable = dynamic_cast<RotoDrawableItem*>(found->get());
            if (isDrawable) {
                assert(isDrawable != item);
                return isDrawable;
            }
            
            //Cycle through a layer that is at the same level
            RotoLayer* isLayer = dynamic_cast<RotoLayer*>(found->get());
            if (isLayer) {
                RotoDrawableItem* si = findPreviousOfItemInLayer(isLayer, 0);
                if (si) {
                    assert(si != item);
                    return si;
                }
            }
        }
    }
    
    //Item was still not found, find in great parent layer
    boost::shared_ptr<RotoLayer> parentLayer = layer->getParentLayer();
    if (!parentLayer) {
        return 0;
    }
    RotoItems greatParentItems = parentLayer->getItems_mt_safe();
    
    found = greatParentItems.end();
    for (RotoItems::iterator it = greatParentItems.begin(); it != greatParentItems.end(); ++it) {
        if (it->get() == layer) {
            found = it;
            break;
        }
    }
    assert(found != greatParentItems.end());
    RotoDrawableItem* ret = findPreviousOfItemInLayer(parentLayer.get(), layer);
    assert(ret != item);
    return ret;
}

RotoDrawableItem*
RotoDrawableItem::findPreviousInHierarchy()
{
    boost::shared_ptr<RotoLayer> layer = getParentLayer();
    if (!layer) {
        return 0;
    }
    return findPreviousOfItemInLayer(layer.get(), this);
}

void
RotoDrawableItem::onRotoKnobChanged(int /*dimension*/, int reason)
{
    KnobSignalSlotHandler* handler = qobject_cast<KnobSignalSlotHandler*>(sender());
    if (!handler) {
        return;
    }
    
    boost::shared_ptr<KnobI> triggerKnob = handler->getKnob();
    assert(triggerKnob);
    rotoKnobChanged(triggerKnob, (Natron::ValueChangedReasonEnum)reason);
    
    
}

void
RotoDrawableItem::rotoKnobChanged(const boost::shared_ptr<KnobI>& knob, Natron::ValueChangedReasonEnum reason)
{
    boost::shared_ptr<Choice_Knob> compKnob = getOperatorKnob();
    RotoStrokeItem* isStroke = dynamic_cast<RotoStrokeItem*>(this);
    RotoStrokeType type;
    if (isStroke) {
        type = isStroke->getBrushType();
    } else {
        type = eRotoStrokeTypeSolid;
    }

    if (reason == Natron::eValueChangedReasonSlaveRefresh) {
        getContext()->s_breakMultiStroke();
    }
    
    if (knob == compKnob) {
        boost::shared_ptr<KnobI> mergeOperatorKnob = _imp->mergeNode->getKnobByName(kMergeOFXParamOperation);
        Choice_Knob* mergeOp = dynamic_cast<Choice_Knob*>(mergeOperatorKnob.get());
        if (mergeOp) {
            mergeOp->setValueFromLabel(compKnob->getEntry(compKnob->getValue()), 0);
        }
    } else if (knob == _imp->sourceColor) {
        refreshNodesConnections();
    } else if (knob == _imp->effectStrength) {
        
        double strength = _imp->effectStrength->getValue();
        switch (type) {
            case Natron::eRotoStrokeTypeBlur: {
                boost::shared_ptr<KnobI> knob = _imp->effectNode->getKnobByName(kBlurCImgParamSize);
                Double_Knob* isDbl = dynamic_cast<Double_Knob*>(knob.get());
                if (isDbl) {
                    isDbl->setValues(strength, strength, Natron::eValueChangedReasonNatronInternalEdited);
                }
            }   break;
            case Natron::eRotoStrokeTypeSharpen: {
                //todo
                break;
            }
            default:
                //others don't have a control
                break;
        }
    } else if (knob == _imp->timeOffset && _imp->timeOffsetNode) {
        
        int offsetMode_i = _imp->timeOffsetMode->getValue();
        boost::shared_ptr<KnobI> offsetKnob;
        
        if (offsetMode_i == 0) {
            offsetKnob = _imp->timeOffsetNode->getKnobByName(kTimeOffsetParamOffset);
        } else {
            offsetKnob = _imp->frameHoldNode->getKnobByName(kFrameHoldParamFirstFrame);
        }
        Int_Knob* offset = dynamic_cast<Int_Knob*>(offsetKnob.get());
        if (offset) {
            double value = _imp->timeOffset->getValue();
            offset->setValue(value,0);
        }
    } else if (knob == _imp->timeOffsetMode && _imp->timeOffsetNode) {
        refreshNodesConnections();
    }
    
    if (type == eRotoStrokeTypeClone || type == eRotoStrokeTypeReveal) {
        if (knob == _imp->cloneTranslate) {
            boost::shared_ptr<KnobI> translateKnob = _imp->effectNode->getKnobByName(kTransformParamTranslate);
            Double_Knob* translate = dynamic_cast<Double_Knob*>(translateKnob.get());
            if (translate) {
                translate->clone(_imp->cloneTranslate.get());
            }
        } else if (knob == _imp->cloneRotate) {
            boost::shared_ptr<KnobI> rotateKnob = _imp->effectNode->getKnobByName(kTransformParamRotate);
            Double_Knob* rotate = dynamic_cast<Double_Knob*>(rotateKnob.get());
            if (rotate) {
                rotate->clone(_imp->cloneRotate.get());
            }
        } else if (knob == _imp->cloneScale) {
            boost::shared_ptr<KnobI> scaleKnob = _imp->effectNode->getKnobByName(kTransformParamScale);
            Double_Knob* scale = dynamic_cast<Double_Knob*>(scaleKnob.get());
            if (scale) {
                scale->clone(_imp->cloneScale.get());
            }
        } else if (knob == _imp->cloneScaleUniform) {
            boost::shared_ptr<KnobI> uniformKnob = _imp->effectNode->getKnobByName(kTransformParamUniform);
            Bool_Knob* uniform = dynamic_cast<Bool_Knob*>(uniformKnob.get());
            if (uniform) {
                uniform->clone(_imp->cloneScaleUniform.get());
            }
        } else if (knob == _imp->cloneSkewX) {
            boost::shared_ptr<KnobI> skewxKnob = _imp->effectNode->getKnobByName(kTransformParamSkewX);
            Double_Knob* skewX = dynamic_cast<Double_Knob*>(skewxKnob.get());
            if (skewX) {
                skewX->clone(_imp->cloneSkewX.get());
            }
        } else if (knob == _imp->cloneSkewY) {
            boost::shared_ptr<KnobI> skewyKnob = _imp->effectNode->getKnobByName(kTransformParamSkewY);
            Double_Knob* skewY = dynamic_cast<Double_Knob*>(skewyKnob.get());
            if (skewY) {
                skewY->clone(_imp->cloneSkewY.get());
            }
        } else if (knob == _imp->cloneSkewOrder) {
            boost::shared_ptr<KnobI> skewOrderKnob = _imp->effectNode->getKnobByName(kTransformParamSkewOrder);
            Choice_Knob* skewOrder = dynamic_cast<Choice_Knob*>(skewOrderKnob.get());
            if (skewOrder) {
                skewOrder->clone(_imp->cloneSkewOrder.get());
            }
        } else if (knob == _imp->cloneCenter) {
            boost::shared_ptr<KnobI> centerKnob = _imp->effectNode->getKnobByName(kTransformParamCenter);
            Double_Knob* center = dynamic_cast<Double_Knob*>(centerKnob.get());
            if (center) {
                center->clone(_imp->cloneCenter.get());
                
            }
        } else if (knob == _imp->cloneFilter) {
            boost::shared_ptr<KnobI> filterKnob = _imp->effectNode->getKnobByName(kTransformParamFilter);
            Choice_Knob* filter = dynamic_cast<Choice_Knob*>(filterKnob.get());
            if (filter) {
                filter->clone(_imp->cloneFilter.get());
            }
        } else if (knob == _imp->cloneBlackOutside) {
            boost::shared_ptr<KnobI> boKnob = _imp->effectNode->getKnobByName(kTransformParamBlackOutside);
            Bool_Knob* bo = dynamic_cast<Bool_Knob*>(boKnob.get());
            if (bo) {
                bo->clone(_imp->cloneBlackOutside.get());
                
            }
        }
    }

    
    
    incrementNodesAge();

}

void
RotoDrawableItem::incrementNodesAge()
{
    if (_imp->effectNode) {
        _imp->effectNode->incrementKnobsAge();
    }
    if (_imp->mergeNode) {
        _imp->mergeNode->incrementKnobsAge();
    }
    if (_imp->timeOffsetNode) {
        _imp->timeOffsetNode->incrementKnobsAge();
    }
    if (_imp->frameHoldNode) {
        _imp->frameHoldNode->incrementKnobsAge();
    }
}

boost::shared_ptr<Natron::Node>
RotoDrawableItem::getEffectNode() const
{
    return _imp->effectNode;
}


boost::shared_ptr<Natron::Node>
RotoDrawableItem::getMergeNode() const
{
    return _imp->mergeNode;
}

boost::shared_ptr<Natron::Node>
RotoDrawableItem::getTimeOffsetNode() const
{
    
    return _imp->timeOffsetNode;
}

boost::shared_ptr<Natron::Node>
RotoDrawableItem::getFrameHoldNode() const
{
    return _imp->frameHoldNode;
}

void
RotoDrawableItem::refreshNodesConnections()
{
    RotoDrawableItem* previous = findPreviousInHierarchy();
    boost::shared_ptr<Node> rotoPaintInput =  getContext()->getNode()->getInput(0);
    
    boost::shared_ptr<Node> upstreamNode = previous ? previous->getMergeNode() : rotoPaintInput;
    
    bool connectionChanged = false;
    
    RotoStrokeItem* isStroke = dynamic_cast<RotoStrokeItem*>(this);
    RotoStrokeType type;
    if (isStroke) {
        type = isStroke->getBrushType();
    } else {
        type = eRotoStrokeTypeSolid;
    }
    
    if (_imp->effectNode && type != eRotoStrokeTypeEraser) {
        
        
        boost::shared_ptr<Natron::Node> mergeInput;
        if (!_imp->timeOffsetNode) {
            mergeInput = _imp->effectNode;
        } else {
            double timeOffsetMode_i = _imp->timeOffsetMode->getValue();
            if (timeOffsetMode_i == 0) {
                //relative
                mergeInput = _imp->timeOffsetNode;
            } else {
                mergeInput = _imp->frameHoldNode;
            }
            if (_imp->effectNode->getInput(0) != mergeInput) {
                _imp->effectNode->disconnectInput(0);
                _imp->effectNode->connectInputBase(mergeInput, 0);
                connectionChanged = true;
            }
        }
        /*
         * This case handles: Stroke, Blur, Sharpen, Smear, Clone
         */
        if (_imp->mergeNode->getInput(1) != _imp->effectNode) {
            _imp->mergeNode->disconnectInput(1);
            _imp->mergeNode->connectInputBase(_imp->effectNode, 1); // A
            connectionChanged = true;
        }
        
        if (_imp->mergeNode->getInput(0) != upstreamNode) {
            _imp->mergeNode->disconnectInput(0);
            if (upstreamNode) {
                _imp->mergeNode->connectInputBase(upstreamNode, 0); // B
            }
            connectionChanged = true;
        }
        
        
        int reveal_i = _imp->sourceColor->getValue();
        boost::shared_ptr<Node> revealInput;
        bool shouldUseUpstreamForReveal = true;
        if ((type == eRotoStrokeTypeReveal ||
             type == eRotoStrokeTypeClone ||
             type == eRotoStrokeTypeEraser) && reveal_i > 0) {
            shouldUseUpstreamForReveal = false;
            revealInput = getContext()->getNode()->getInput(reveal_i - 1);
        }
        if (!revealInput && shouldUseUpstreamForReveal) {
            if (type != eRotoStrokeTypeSolid) {
                revealInput = upstreamNode;
            }
            
        }
        
        if (revealInput) {
            if (mergeInput->getInput(0) != revealInput) {
                mergeInput->disconnectInput(0);
                mergeInput->connectInputBase(revealInput, 0);
                connectionChanged = true;
            }
        } else {
            if (mergeInput->getInput(0)) {
                mergeInput->disconnectInput(0);
                connectionChanged = true;
            }
            
        }
    } else {
        
        if (type == eRotoStrokeTypeEraser) {
            
            boost::shared_ptr<Node> eraserInput = rotoPaintInput ? rotoPaintInput : _imp->effectNode;
            if (_imp->mergeNode->getInput(1) != eraserInput) {
                _imp->mergeNode->disconnectInput(1);
                if (eraserInput) {
                    _imp->mergeNode->connectInputBase(eraserInput, 1); // A
                }
                connectionChanged = true;
            }
            
            
            if (_imp->mergeNode->getInput(0) != upstreamNode) {
                _imp->mergeNode->disconnectInput(0);
                if (upstreamNode) {
                    _imp->mergeNode->connectInputBase(upstreamNode, 0); // B
                }
                connectionChanged = true;
            }
            
        } else if (type == eRotoStrokeTypeReveal) {
            
            int reveal_i = _imp->sourceColor->getValue();
            
            boost::shared_ptr<Node> revealInput = getContext()->getNode()->getInput(reveal_i - 1);
            
            if (_imp->mergeNode->getInput(1) != revealInput) {
                _imp->mergeNode->disconnectInput(1);
                if (revealInput) {
                    _imp->mergeNode->connectInputBase(revealInput, 1); // A
                }
                connectionChanged = true;
            }
            
            
            if (_imp->mergeNode->getInput(0) != upstreamNode) {
                _imp->mergeNode->disconnectInput(0);
                if (upstreamNode) {
                    _imp->mergeNode->connectInputBase(upstreamNode, 0); // B
                }
                connectionChanged = true;
            }
            
        } else if (type == eRotoStrokeTypeDodge || type == eRotoStrokeTypeBurn) {
            
            if (_imp->mergeNode->getInput(1) != upstreamNode) {
                _imp->mergeNode->disconnectInput(1);
                if (upstreamNode) {
                    _imp->mergeNode->connectInputBase(upstreamNode, 1); // A
                }
                connectionChanged = true;
            }
            
            
            if (_imp->mergeNode->getInput(0) != upstreamNode) {
                _imp->mergeNode->disconnectInput(0);
                if (upstreamNode) {
                    _imp->mergeNode->connectInputBase(upstreamNode, 0); // B
                }
                connectionChanged = true;
            }
            
        } else {
            //unhandled case
            assert(false);
        }
    }
    if (connectionChanged && (type == eRotoStrokeTypeClone || type == eRotoStrokeTypeReveal)) {
        resetCloneTransformCenter();
    }
}

void
RotoDrawableItem::resetNodesThreadSafety()
{
    if (_imp->effectNode) {
        _imp->effectNode->revertToPluginThreadSafety();
    }
    _imp->mergeNode->revertToPluginThreadSafety();
    if (_imp->timeOffsetNode) {
        _imp->timeOffsetNode->revertToPluginThreadSafety();
    }
    if (_imp->frameHoldNode) {
        _imp->frameHoldNode->revertToPluginThreadSafety();
    }
    getContext()->getNode()->revertToPluginThreadSafety();
    
}

void
RotoDrawableItem::resetCloneTransformCenter()
{
    
    RotoStrokeType type;
    RotoStrokeItem* isStroke = dynamic_cast<RotoStrokeItem*>(this);
    if (isStroke) {
        type = isStroke->getBrushType();
    } else {
        type = eRotoStrokeTypeSolid;
    }
    if (type != eRotoStrokeTypeReveal && type != eRotoStrokeTypeClone) {
        return;
    }
    boost::shared_ptr<KnobI> resetCenterKnob = _imp->effectNode->getKnobByName(kTransformParamResetCenter);
    Button_Knob* resetCenter = dynamic_cast<Button_Knob*>(resetCenterKnob.get());
    if (!resetCenter) {
        return;
    }
    boost::shared_ptr<KnobI> centerKnob = _imp->effectNode->getKnobByName(kTransformParamCenter);
    Double_Knob* center = dynamic_cast<Double_Knob*>(centerKnob.get());
    if (!center) {
        return;
    }
    resetCenter->evaluateValueChange(0, Natron::eValueChangedReasonUserEdited);
    double x = center->getValue(0);
    double y = center->getValue(1);
    _imp->cloneCenter->setValues(x, y, Natron::eValueChangedReasonNatronGuiEdited);
}


void
RotoDrawableItem::clone(const RotoItem* other)
{
    const RotoDrawableItem* otherDrawable = dynamic_cast<const RotoDrawableItem*>(other);
    if (!otherDrawable) {
        return;
    }
    const std::list<boost::shared_ptr<KnobI> >& otherKnobs = otherDrawable->getKnobs();
    assert(otherKnobs.size() == _imp->knobs.size());
    if (otherKnobs.size() != _imp->knobs.size()) {
        return;
    }
    std::list<boost::shared_ptr<KnobI> >::iterator it = _imp->knobs.begin();
    std::list<boost::shared_ptr<KnobI> >::const_iterator otherIt = otherKnobs.begin();
    for (; it != _imp->knobs.end(); ++it, ++otherIt) {
        (*it)->clone(*otherIt);
    }
    {
        QMutexLocker l(&itemMutex);
        memcpy(_imp->overlayColor, otherDrawable->_imp->overlayColor, sizeof(double) * 4);
    }
    RotoItem::clone(other);
}

static void
serializeRotoKnob(const boost::shared_ptr<KnobI> & knob,
                  KnobSerialization* serialization)
{
    std::pair<int, boost::shared_ptr<KnobI> > master = knob->getMaster(0);
    bool wasSlaved = false;

    if (master.second) {
        wasSlaved = true;
        knob->unSlave(0,false);
    }

    serialization->initialize(knob);

    if (wasSlaved) {
        knob->slaveTo(0, master.second, master.first);
    }
}

void
RotoDrawableItem::save(RotoItemSerialization *obj) const
{
    RotoDrawableItemSerialization* s = dynamic_cast<RotoDrawableItemSerialization*>(obj);

    assert(s);
    for (std::list<boost::shared_ptr<KnobI> >::const_iterator it = _imp->knobs.begin(); it != _imp->knobs.end(); ++it) {
        boost::shared_ptr<KnobSerialization> k(new KnobSerialization());
        serializeRotoKnob(*it,k.get());
        s->_knobs.push_back(k);
    }
    {
        
        QMutexLocker l(&itemMutex);
        memcpy(s->_overlayColor, _imp->overlayColor, sizeof(double) * 4);
    }
    RotoItem::save(obj);
}

void
RotoDrawableItem::load(const RotoItemSerialization &obj)
{
    RotoItem::load(obj);

    const RotoDrawableItemSerialization & s = dynamic_cast<const RotoDrawableItemSerialization &>(obj);
    for (std::list<boost::shared_ptr<KnobSerialization> >::const_iterator it = s._knobs.begin(); it!=s._knobs.end(); ++it) {
        
        for (std::list<boost::shared_ptr<KnobI> >::const_iterator it2 = _imp->knobs.begin(); it2 != _imp->knobs.end(); ++it2) {
            if ((*it2)->getName() == (*it)->getName()) {
                (*it2)->clone((*it)->getKnob().get());
                break;
            }
        }
    }
    {
        QMutexLocker l(&itemMutex);
        memcpy(_imp->overlayColor, s._overlayColor, sizeof(double) * 4);
    }
    
    RotoStrokeType type;
    RotoStrokeItem* isStroke = dynamic_cast<RotoStrokeItem*>(this);
    if (isStroke) {
        type = isStroke->getBrushType();
    } else {
        type = eRotoStrokeTypeSolid;
    }

    boost::shared_ptr<Choice_Knob> compKnob = getOperatorKnob();
    boost::shared_ptr<KnobI> mergeOperatorKnob = _imp->mergeNode->getKnobByName(kMergeOFXParamOperation);
    Choice_Knob* mergeOp = dynamic_cast<Choice_Knob*>(mergeOperatorKnob.get());
    if (mergeOp) {
        mergeOp->setValueFromLabel(compKnob->getEntry(compKnob->getValue()), 0);
    }

    if (type == eRotoStrokeTypeClone || type == eRotoStrokeTypeReveal) {
        boost::shared_ptr<KnobI> translateKnob = _imp->effectNode->getKnobByName(kTransformParamTranslate);
        Double_Knob* translate = dynamic_cast<Double_Knob*>(translateKnob.get());
        if (translate) {
            translate->clone(_imp->cloneTranslate.get());
        }
        boost::shared_ptr<KnobI> rotateKnob = _imp->effectNode->getKnobByName(kTransformParamRotate);
        Double_Knob* rotate = dynamic_cast<Double_Knob*>(rotateKnob.get());
        if (rotate) {
            rotate->clone(_imp->cloneRotate.get());
        }
        boost::shared_ptr<KnobI> scaleKnob = _imp->effectNode->getKnobByName(kTransformParamScale);
        Double_Knob* scale = dynamic_cast<Double_Knob*>(scaleKnob.get());
        if (scale) {
            scale->clone(_imp->cloneScale.get());
        }
        boost::shared_ptr<KnobI> uniformKnob = _imp->effectNode->getKnobByName(kTransformParamUniform);
        Bool_Knob* uniform = dynamic_cast<Bool_Knob*>(uniformKnob.get());
        if (uniform) {
            uniform->clone(_imp->cloneScaleUniform.get());
        }
        boost::shared_ptr<KnobI> skewxKnob = _imp->effectNode->getKnobByName(kTransformParamSkewX);
        Double_Knob* skewX = dynamic_cast<Double_Knob*>(skewxKnob.get());
        if (skewX) {
            skewX->clone(_imp->cloneSkewX.get());
        }
        boost::shared_ptr<KnobI> skewyKnob = _imp->effectNode->getKnobByName(kTransformParamSkewY);
        Double_Knob* skewY = dynamic_cast<Double_Knob*>(skewyKnob.get());
        if (skewY) {
            skewY->clone(_imp->cloneSkewY.get());
        }
        boost::shared_ptr<KnobI> skewOrderKnob = _imp->effectNode->getKnobByName(kTransformParamSkewOrder);
        Choice_Knob* skewOrder = dynamic_cast<Choice_Knob*>(skewOrderKnob.get());
        if (skewOrder) {
            skewOrder->clone(_imp->cloneSkewOrder.get());
        }
        boost::shared_ptr<KnobI> centerKnob = _imp->effectNode->getKnobByName(kTransformParamCenter);
        Double_Knob* center = dynamic_cast<Double_Knob*>(centerKnob.get());
        if (center) {
            center->clone(_imp->cloneCenter.get());
            
        }
        boost::shared_ptr<KnobI> filterKnob = _imp->effectNode->getKnobByName(kTransformParamFilter);
        Choice_Knob* filter = dynamic_cast<Choice_Knob*>(filterKnob.get());
        if (filter) {
            filter->clone(_imp->cloneFilter.get());
        }
        boost::shared_ptr<KnobI> boKnob = _imp->effectNode->getKnobByName(kTransformParamBlackOutside);
        Bool_Knob* bo = dynamic_cast<Bool_Knob*>(boKnob.get());
        if (bo) {
            bo->clone(_imp->cloneBlackOutside.get());
            
        }
        
        int offsetMode_i = _imp->timeOffsetMode->getValue();
        boost::shared_ptr<KnobI> offsetKnob;
        
        if (offsetMode_i == 0) {
            offsetKnob = _imp->timeOffsetNode->getKnobByName(kTimeOffsetParamOffset);
        } else {
            offsetKnob = _imp->frameHoldNode->getKnobByName(kFrameHoldParamFirstFrame);
        }
        Int_Knob* offset = dynamic_cast<Int_Knob*>(offsetKnob.get());
        if (offset) {
            offset->clone(_imp->timeOffset.get());
        }
        
        
    } else if (type == eRotoStrokeTypeBlur) {
        boost::shared_ptr<KnobI> knob = _imp->effectNode->getKnobByName(kBlurCImgParamSize);
        Double_Knob* isDbl = dynamic_cast<Double_Knob*>(knob.get());
        if (isDbl) {
            isDbl->clone(_imp->effectStrength.get());
        }
    }

}

bool
RotoDrawableItem::isActivated(double time) const
{
    if (!isGloballyActivated()) {
        return false;
    }
    int lifetime_i = _imp->lifeTime->getValue();
    if (lifetime_i == 0) {
        return time == _imp->lifeTimeFrame->getValue();
    } else if (lifetime_i == 1) {
        return time <= _imp->lifeTimeFrame->getValue();
    } else if (lifetime_i == 2) {
        return time >= _imp->lifeTimeFrame->getValue();
    } else {
        return _imp->activated->getValueAtTime(time);
    }
}

void
RotoDrawableItem::setActivated(bool a, double time)
{
    _imp->activated->setValueAtTime(time, a, 0);
    getContext()->onItemKnobChanged();
}

double
RotoDrawableItem::getOpacity(double time) const
{
    ///MT-safe thanks to Knob
    return _imp->opacity->getValueAtTime(time);
}

void
RotoDrawableItem::setOpacity(double o,double time)
{
    _imp->opacity->setValueAtTime(time, o, 0);
    getContext()->onItemKnobChanged();
}

double
RotoDrawableItem::getFeatherDistance(double time) const
{
    ///MT-safe thanks to Knob
    return _imp->feather->getValueAtTime(time);
}

void
RotoDrawableItem::setFeatherDistance(double d,double time)
{
    _imp->feather->setValueAtTime(time, d, 0);
    getContext()->onItemKnobChanged();
}


int
RotoDrawableItem::getNumKeyframesFeatherDistance() const
{
    return _imp->feather->getKeyFramesCount(0);
}

void
RotoDrawableItem::setFeatherFallOff(double f,double time)
{
    _imp->featherFallOff->setValueAtTime(time, f, 0);
    getContext()->onItemKnobChanged();
}

double
RotoDrawableItem::getFeatherFallOff(double time) const
{
    ///MT-safe thanks to Knob
    return _imp->featherFallOff->getValueAtTime(time);
}

#ifdef NATRON_ROTO_INVERTIBLE
bool
RotoDrawableItem::getInverted(double time) const
{
    ///MT-safe thanks to Knob
    return _imp->inverted->getValueAtTime(time);
}

#endif

void
RotoDrawableItem::getColor(double time,
                           double* color) const
{
    color[0] = _imp->color->getValueAtTime(time,0);
    color[1] = _imp->color->getValueAtTime(time,1);
    color[2] = _imp->color->getValueAtTime(time,2);
}

void
RotoDrawableItem::setColor(double time,double r,double g,double b)
{
    _imp->color->setValueAtTime(time, r, 0);
    _imp->color->setValueAtTime(time, g, 1);
    _imp->color->setValueAtTime(time, b, 2);
    getContext()->onItemKnobChanged();
}

int
RotoDrawableItem::getCompositingOperator() const
{
    return _imp->compOperator->getValue();
}

void
RotoDrawableItem::setCompositingOperator(int op)
{
    _imp->compOperator->setValue( op, 0);
}

std::string
RotoDrawableItem::getCompositingOperatorToolTip() const
{
    return _imp->compOperator->getHintToolTipFull();
}

void
RotoDrawableItem::getOverlayColor(double* color) const
{
    QMutexLocker l(&itemMutex);

    memcpy(color, _imp->overlayColor, sizeof(double) * 4);
}

void
RotoDrawableItem::setOverlayColor(const double *color)
{
    ///MT-safe: only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    {
        QMutexLocker l(&itemMutex);
        memcpy(_imp->overlayColor, color, sizeof(double) * 4);
    }
    Q_EMIT overlayColorChanged();
}

boost::shared_ptr<Bool_Knob> RotoDrawableItem::getActivatedKnob() const
{
    return _imp->activated;
}

boost::shared_ptr<Double_Knob> RotoDrawableItem::getFeatherKnob() const
{
    return _imp->feather;
}

boost::shared_ptr<Double_Knob> RotoDrawableItem::getFeatherFallOffKnob() const
{
    return _imp->featherFallOff;
}

boost::shared_ptr<Double_Knob> RotoDrawableItem::getOpacityKnob() const
{
    return _imp->opacity;
}

#ifdef NATRON_ROTO_INVERTIBLE
boost::shared_ptr<Bool_Knob> RotoDrawableItem::getInvertedKnob() const
{
    return _imp->inverted;
}

#endif
boost::shared_ptr<Choice_Knob> RotoDrawableItem::getOperatorKnob() const
{
    return _imp->compOperator;
}

boost::shared_ptr<Color_Knob> RotoDrawableItem::getColorKnob() const
{
    return _imp->color;
}

boost::shared_ptr<Double_Knob>
RotoDrawableItem::getBrushSizeKnob() const
{
    return _imp->brushSize;
}

boost::shared_ptr<Double_Knob>
RotoDrawableItem::getBrushHardnessKnob() const
{
    return _imp->brushHardness;
}

boost::shared_ptr<Double_Knob>
RotoDrawableItem::getBrushSpacingKnob() const
{
    return _imp->brushSpacing;
}

boost::shared_ptr<Double_Knob>
RotoDrawableItem::getBrushEffectKnob() const
{
    return _imp->effectStrength;
}

boost::shared_ptr<Double_Knob>
RotoDrawableItem::getBrushVisiblePortionKnob() const
{
    return _imp->visiblePortion;
}

boost::shared_ptr<Bool_Knob>
RotoDrawableItem::getPressureOpacityKnob() const
{
    return _imp->pressureOpacity;
}

boost::shared_ptr<Bool_Knob>
RotoDrawableItem::getPressureSizeKnob() const
{
    return _imp->pressureSize;
}

boost::shared_ptr<Bool_Knob>
RotoDrawableItem::getPressureHardnessKnob() const
{
    return _imp->pressureHardness;
}

boost::shared_ptr<Bool_Knob>
RotoDrawableItem::getBuildupKnob() const
{
    return _imp->buildUp;
}

boost::shared_ptr<Int_Knob>
RotoDrawableItem::getTimeOffsetKnob() const
{
    return _imp->timeOffset;
}

boost::shared_ptr<Choice_Knob>
RotoDrawableItem::getTimeOffsetModeKnob() const
{
    return _imp->timeOffsetMode;
}

boost::shared_ptr<Choice_Knob>
RotoDrawableItem::getBrushSourceTypeKnob() const
{
    return _imp->sourceColor;
}

boost::shared_ptr<Double_Knob>
RotoDrawableItem::getBrushCloneTranslateKnob() const
{
    return _imp->cloneTranslate;
}

boost::shared_ptr<Double_Knob>
RotoDrawableItem::getCenterKnob() const
{
    return _imp->center;
}

boost::shared_ptr<Int_Knob>
RotoDrawableItem::getLifeTimeFrameKnob() const
{
    return _imp->lifeTimeFrame;
}

void
RotoDrawableItem::setKeyframeOnAllTransformParameters(double time)
{
    _imp->translate->setValueAtTime(time, _imp->translate->getValue(0), 0);
    _imp->translate->setValueAtTime(time, _imp->translate->getValue(1), 1);
    
    _imp->scale->setValueAtTime(time, _imp->scale->getValue(0), 0);
    _imp->scale->setValueAtTime(time, _imp->scale->getValue(1), 1);
    
    _imp->rotate->setValueAtTime(time, _imp->rotate->getValue(0), 0);
    
    _imp->skewX->setValueAtTime(time, _imp->skewX->getValue(0), 0);
    _imp->skewY->setValueAtTime(time, _imp->skewY->getValue(0), 0);
}

const std::list<boost::shared_ptr<KnobI> >&
RotoDrawableItem::getKnobs() const
{
    return _imp->knobs;
}


void
RotoDrawableItem::getTransformAtTime(double time,Transform::Matrix3x3* matrix) const
{
    double tx = _imp->translate->getValueAtTime(time, 0);
    double ty = _imp->translate->getValueAtTime(time, 1);
    double sx = _imp->scale->getValueAtTime(time, 0);
    double sy = _imp->scaleUniform->getValueAtTime(time) ? sx : _imp->scale->getValueAtTime(time, 1);
    double skewX = _imp->skewX->getValueAtTime(time, 0);
    double skewY = _imp->skewY->getValueAtTime(time, 0);
    double rot = _imp->rotate->getValueAtTime(time, 0);
    rot = rot * M_PI / 180.0;
    double centerX = _imp->center->getValueAtTime(time, 0);
    double centerY = _imp->center->getValueAtTime(time, 1);
    bool skewOrderYX = _imp->skewOrder->getValueAtTime(time) == 1;
    *matrix = Transform::matTransformCanonical(tx, ty, sx, sy, skewX, skewY, skewOrderYX, rot, centerX, centerY);
}

/**
 * @brief Set the transform at the given time
 **/
void
RotoDrawableItem::setTransform(double time, double tx, double ty, double sx, double sy, double centerX, double centerY, double rot, double skewX, double skewY)
{
    
    bool autoKeying = getContext()->isAutoKeyingEnabled();
    
    if (autoKeying) {
        _imp->translate->setValueAtTime(time, tx, 0);
        _imp->translate->setValueAtTime(time, ty, 1);
        
        _imp->scale->setValueAtTime(time, sx, 0);
        _imp->scale->setValueAtTime(time, sy, 1);
        
        _imp->center->setValue(centerX, 0);
        _imp->center->setValue(centerY, 1);
        
        _imp->rotate->setValueAtTime(time, rot, 0);
        
        _imp->skewX->setValueAtTime(time, skewX, 0);
        _imp->skewY->setValueAtTime(time, skewY, 0);
    } else {
        _imp->translate->setValue(tx, 0);
        _imp->translate->setValue(ty, 1);
        
        _imp->scale->setValue(sx, 0);
        _imp->scale->setValue(sy, 1);
        
        _imp->center->setValue(centerX, 0);
        _imp->center->setValue(centerY, 1);
        
        _imp->rotate->setValue(rot, 0);
        
        _imp->skewX->setValue(skewX, 0);
        _imp->skewY->setValue(skewY, 0);

    }
    
    onTransformSet(time);
}

////////////////////////////////////Layer////////////////////////////////////

RotoLayer::RotoLayer(const boost::shared_ptr<RotoContext>& context,
                     const std::string & n,
                     const boost::shared_ptr<RotoLayer>& parent)
    : RotoItem(context,n,parent)
      , _imp( new RotoLayerPrivate() )
{
}

RotoLayer::RotoLayer(const RotoLayer & other)
    : RotoItem( other.getContext(),other.getScriptName(),other.getParentLayer() )
      ,_imp( new RotoLayerPrivate() )
{
    clone(&other);
}

RotoLayer::~RotoLayer()
{
}

void
RotoLayer::clone(const RotoItem* other)
{
    RotoItem::clone(other);
    const RotoLayer* isOtherLayer = dynamic_cast<const RotoLayer*>(other);
    if (!isOtherLayer) {
        return;
    }
    boost::shared_ptr<RotoLayer> this_shared = boost::dynamic_pointer_cast<RotoLayer>(shared_from_this());
    assert(this_shared);
    
    QMutexLocker l(&itemMutex);

    _imp->items.clear();
    for (std::list<boost::shared_ptr<RotoItem> >::const_iterator it = isOtherLayer->_imp->items.begin();
         it != isOtherLayer->_imp->items.end(); ++it) {
        boost::shared_ptr<RotoLayer> isLayer = boost::dynamic_pointer_cast<RotoLayer>(*it);
        boost::shared_ptr<Bezier> isBezier = boost::dynamic_pointer_cast<Bezier>(*it);
        boost::shared_ptr<RotoStrokeItem> isStroke = boost::dynamic_pointer_cast<RotoStrokeItem>(*it);
        if (isBezier) {
            boost::shared_ptr<Bezier> copy( new Bezier(*isBezier, this_shared) );
            copy->createNodes();
            _imp->items.push_back(copy);
        } else if (isStroke) {
            boost::shared_ptr<RotoStrokeItem> copy(new RotoStrokeItem(isStroke->getBrushType(),
                                                                      isStroke->getContext(),
                                                                      isStroke->getScriptName() + "copy",
                                                                      boost::shared_ptr<RotoLayer>()));
            copy->createNodes();
            _imp->items.push_back(copy);
            copy->setParentLayer(this_shared);
        } else {
            assert(isLayer);
            if (isLayer) {
                boost::shared_ptr<RotoLayer> copy( new RotoLayer(*isLayer) );
                copy->setParentLayer(this_shared);
                _imp->items.push_back(copy);
                getContext()->addLayer(copy);
            }
        }
    }
}

void
RotoLayer::save(RotoItemSerialization *obj) const
{
    RotoLayerSerialization* s = dynamic_cast<RotoLayerSerialization*>(obj);

    assert(s);
    RotoItems items;
    {
        QMutexLocker l(&itemMutex);
        items = _imp->items;
    }

    for (RotoItems::const_iterator it = items.begin(); it != items.end(); ++it) {
        boost::shared_ptr<Bezier> isBezier = boost::dynamic_pointer_cast<Bezier>(*it);
        boost::shared_ptr<RotoStrokeItem> isStroke = boost::dynamic_pointer_cast<RotoStrokeItem>(*it);
        boost::shared_ptr<RotoLayer> layer = boost::dynamic_pointer_cast<RotoLayer>(*it);
        boost::shared_ptr<RotoItemSerialization> childSerialization;
        if (isBezier && !isStroke) {
            childSerialization.reset(new BezierSerialization);
            isBezier->save( childSerialization.get() );
        } else if (isStroke) {
            childSerialization.reset(new RotoStrokeSerialization());
            isStroke->save(childSerialization.get());
        } else {
            assert(layer);
            if (layer) {
                childSerialization.reset(new RotoLayerSerialization);
                layer->save( childSerialization.get() );
            }
        }
        assert(childSerialization);
        s->children.push_back(childSerialization);
    }


    RotoItem::save(obj);
}

void
RotoLayer::load(const RotoItemSerialization &obj)
{
    const RotoLayerSerialization & s = dynamic_cast<const RotoLayerSerialization &>(obj);
    boost::shared_ptr<RotoLayer> this_layer = boost::dynamic_pointer_cast<RotoLayer>(shared_from_this());
    assert(this_layer);
    RotoItem::load(obj);
    {
        for (std::list<boost::shared_ptr<RotoItemSerialization> >::const_iterator it = s.children.begin(); it != s.children.end(); ++it) {
            boost::shared_ptr<BezierSerialization> b = boost::dynamic_pointer_cast<BezierSerialization>(*it);
            boost::shared_ptr<RotoStrokeSerialization> s = boost::dynamic_pointer_cast<RotoStrokeSerialization>(*it);
            boost::shared_ptr<RotoLayerSerialization> l = boost::dynamic_pointer_cast<RotoLayerSerialization>(*it);
            if (b && !s) {
                boost::shared_ptr<Bezier> bezier( new Bezier(getContext(), kRotoBezierBaseName, boost::shared_ptr<RotoLayer>(), false) );
                bezier->createNodes(false);
                bezier->load(*b);
                if (!bezier->getParentLayer()) {
                    bezier->setParentLayer(this_layer);
                }
                QMutexLocker l(&itemMutex);
                _imp->items.push_back(bezier);
            }
            else if (s) {
                boost::shared_ptr<RotoStrokeItem> stroke(new RotoStrokeItem((Natron::RotoStrokeType)s->getType(),getContext(),kRotoPaintBrushBaseName,boost::shared_ptr<RotoLayer>()));
                stroke->createNodes(false);
                stroke->load(*s);
                if (!stroke->getParentLayer()) {
                    stroke->setParentLayer(this_layer);
                }
                

                QMutexLocker l(&itemMutex);
                _imp->items.push_back(stroke);
            }
            else if (l) {
                boost::shared_ptr<RotoLayer> layer( new RotoLayer(getContext(), kRotoLayerBaseName, this_layer) );
                _imp->items.push_back(layer);
                getContext()->addLayer(layer);
                layer->load(*l);
                if (!layer->getParentLayer()) {
                    layer->setParentLayer(this_layer);
                }

            }
            //Rotopaint tree nodes use the roto context age for their script-name, make sure they have a different one
            getContext()->incrementAge();
        }
    }
}

void
RotoLayer::addItem(const boost::shared_ptr<RotoItem> & item,bool declareToPython )
{
    ///only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    boost::shared_ptr<RotoLayer> parentLayer = item->getParentLayer();
    if (parentLayer) {
        parentLayer->removeItem(item);
    }
    
    item->setParentLayer(boost::dynamic_pointer_cast<RotoLayer>(shared_from_this()));
    {
        QMutexLocker l(&itemMutex);
        _imp->items.push_back(item);
    }
    if (declareToPython) {
        getContext()->declareItemAsPythonField(item);
    }
    getContext()->refreshRotoPaintTree();
}

void
RotoLayer::insertItem(const boost::shared_ptr<RotoItem> & item,
                      int index)
{
    ///only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    assert(index >= 0);
    
    boost::shared_ptr<RotoLayer> parentLayer = item->getParentLayer();
    if (parentLayer && parentLayer.get() != this) {
        parentLayer->removeItem(item);
    }
    
    
    {
        QMutexLocker l(&itemMutex);
        if (parentLayer.get() != this) {
            item->setParentLayer(boost::dynamic_pointer_cast<RotoLayer>(shared_from_this()));
        } else {
            RotoItems::iterator found = std::find(_imp->items.begin(), _imp->items.end(), item);
            if (found != _imp->items.end()) {
                _imp->items.erase(found);
            }
        }
        RotoItems::iterator it = _imp->items.begin();
        if ( index >= (int)_imp->items.size() ) {
            it = _imp->items.end();
        } else {
            std::advance(it, index);
        }
        ///insert before the iterator
        _imp->items.insert(it, item);
    }
    getContext()->declareItemAsPythonField(item);
    getContext()->refreshRotoPaintTree();
}

void
RotoLayer::removeItem(const boost::shared_ptr<RotoItem>& item)
{
    ///only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    {
        QMutexLocker l(&itemMutex);
        for (RotoItems::iterator it = _imp->items.begin(); it != _imp->items.end(); ++it) {
            if (*it == item) {
                l.unlock();
                getContext()->removeItemAsPythonField(item);
                l.relock();
                _imp->items.erase(it);
                break;
            }
        }
    }
    item->setParentLayer(boost::shared_ptr<RotoLayer>());
    RotoStrokeItem* isStroke = dynamic_cast<RotoStrokeItem*>(item.get());
    if (isStroke) {
        isStroke->disconnectNodes();
    }
    getContext()->refreshRotoPaintTree();
}

int
RotoLayer::getChildIndex(const boost::shared_ptr<RotoItem> & item) const
{
    QMutexLocker l(&itemMutex);
    int i = 0;

    for (RotoItems::iterator it = _imp->items.begin(); it != _imp->items.end(); ++it, ++i) {
        if (*it == item) {
            return i;
        }
    }

    return -1;
}

const RotoItems &
RotoLayer::getItems() const
{
    ///only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );

    return _imp->items;
}

RotoItems
RotoLayer::getItems_mt_safe() const
{
    QMutexLocker l(&itemMutex);

    return _imp->items;
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
        boost::shared_ptr<Double_Knob> masterCp = (*itCp)->isSlaved();
        
        if (masterCp) {
            masterCp->removeSlavedTrack(*itCp);
        }
        
        if (useFeather) {
            boost::shared_ptr<Double_Knob> masterFp = (*itFp)->isSlaved();
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
Bezier::resetCenterKnob()
{
    double time = getContext()->getTimelineCurrentTime();
    bool autoKeying = getContext()->isAutoKeyingEnabled();
    Point center;
    std::size_t nPoints = 0;
    
    {
        QMutexLocker l(&itemMutex);
        
        ///Compute the value of the center knob
        center.x = center.y = 0;
        
        for (BezierCPs::iterator it = _imp->points.begin(); it!=_imp->points.end(); ++it) {
            double x,y;
            (*it)->getPositionAtTime(true,time, &x, &y);
            center.x += x;
            center.y += y;
        }
        nPoints = _imp->points.size();
        
    }
    if (nPoints) {
        assert(nPoints > 0);
        center.x /= nPoints;
        center.y /= nPoints;
        boost::shared_ptr<Double_Knob> centerKnob = getCenterKnob();
        if (autoKeying) {
            centerKnob->setValueAtTime(time, center.x, 0);
            centerKnob->setValueAtTime(time, center.y, 1);
            
            //Also set a keyframe on all transform parameters
            setKeyframeOnAllTransformParameters(time);
        } else  {
            centerKnob->setValue(center.x, 0);
            centerKnob->setValue(center.y, 1);
        }
        
    }
}

void
Bezier::setCurveFinished(bool finished)
{

    ///only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    
    if (!_imp->isOpenBezier) {
        QMutexLocker l(&itemMutex);
        _imp->finished = finished;
    }
    
    resetCenterKnob();

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
        
        boost::shared_ptr<Double_Knob> isSlaved = (*it)->isSlaved();
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
        }
        
        bool isOnKeyframe;
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
            moveFeather = true;
            if (isLeft || moveBoth) {
                (fp)->getLeftBezierPointAtTime(useGuiCurve,time, &leftF.x, &leftF.y,true);
                leftF = Transform::matApply(trans, leftF);
                moveFeather = moveFeather && left.x == leftF.x && left.y == leftF.y;
            }
            if (!isLeft || moveBoth) {
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

RectD
Bezier::getBoundingBox(double time) const
{
    
    RectD bbox; // a very empty bbox
    bbox.setupInfinity();
    
    Transform::Matrix3x3 transform;
    getTransformAtTime(time, &transform);
    
    QMutexLocker l(&itemMutex);
    bezierSegmentListBboxUpdate(false,_imp->points, _imp->finished, _imp->isOpenBezier, time, 0, transform , &bbox);
    
    
    if (useFeatherPoints() && !_imp->isOpenBezier) {
        bezierSegmentListBboxUpdate(false,_imp->featherPoints, _imp->finished, _imp->isOpenBezier, time, 0, transform, &bbox);
        // EDIT: Partial fix, just pad the BBOX by the feather distance. This might not be accurate but gives at least something
        // enclosing the real bbox and close enough
        double featherDistance = getFeatherDistance(time);
        bbox.x1 -= featherDistance;
        bbox.x2 += featherDistance;
        bbox.y1 -= featherDistance;
        bbox.y2 += featherDistance;
    } else if (_imp->isOpenBezier) {
        double brushSize = getBrushSizeKnob()->getValueAtTime(time);
        double halfBrushSize = brushSize / 2. + 1;
        bbox.x1 -= halfBrushSize;
        bbox.x2 += halfBrushSize;
        bbox.y1 -= halfBrushSize;
        bbox.y2 += halfBrushSize;
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

static bool
pointInPolygon(const Point & p,
               const std::list<Point> & polygon,
               const RectD & featherPolyBBox,
               Bezier::FillRuleEnum rule)
{
    ///first check if the point lies inside the bounding box
    if ( (p.x < featherPolyBBox.x1) || (p.x >= featherPolyBBox.x2) || (p.y < featherPolyBBox.y1) || (p.y >= featherPolyBBox.y2)
         || polygon.empty() ) {
        return false;
    }

    int winding_number = 0;
    std::list<Point>::const_iterator last_pt = polygon.begin();
    std::list<Point>::const_iterator last_start = last_pt;
    std::list<Point>::const_iterator cur = last_pt;
    ++cur;
    for (; cur != polygon.end(); ++cur, ++last_pt) {
        Bezier::point_line_intersection(*last_pt, *cur, p, &winding_number);
    }

    // implicitly close last subpath
    if (last_pt != last_start) {
        Bezier::point_line_intersection(*last_pt, *last_start, p, &winding_number);
    }

    return rule == Bezier::eFillRuleWinding
           ? (winding_number != 0)
           : ( (winding_number % 2) != 0 );
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


////////////////////////////////////Stroke//////////////////////////////////

RotoStrokeItem::RotoStrokeItem(Natron::RotoStrokeType type,
                               const boost::shared_ptr<RotoContext>& context,
                               const std::string & name,
                               const boost::shared_ptr<RotoLayer>& parent)

: RotoDrawableItem(context,name,parent, true)
, _imp(new RotoStrokeItemPrivate(type))
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



Natron::RotoStrokeType
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
                       std::list<std::pair<Natron::Point,double> >* points,
                       RectD* bbox)
{
    //Increment the half brush size so that the stroke is enclosed in the RoD
    halfBrushSize += 1.;
    if (bbox) {
        bbox->setupInfinity();
    }
    if (xCurve.empty()) {
        return;
    }
    
    assert(xCurve.size() == yCurve.size() && xCurve.size() == pCurve.size());
    
    KeyFrameSet::const_iterator xIt = xCurve.begin();
    KeyFrameSet::const_iterator yIt = yCurve.begin();
    KeyFrameSet::const_iterator pIt = pCurve.begin();
    KeyFrameSet::const_iterator xNext = xIt;
    if (xNext != xCurve.end()) {
        ++xNext;
    }
    KeyFrameSet::const_iterator yNext = yIt;
    if (yNext != yCurve.end()) {
        ++yNext;
    }
    KeyFrameSet::const_iterator pNext = pIt;
    if (pNext != pCurve.end()) {
        ++pNext;
    }


    int pot = 1 << mipMapLevel;
    
    if (xCurve.size() == 1 && xIt != xCurve.end() && yIt != yCurve.end() && pIt != pCurve.end()) {
        assert(xNext == xCurve.end() && yNext == yCurve.end() && pNext == pCurve.end());
        Transform::Point3D p;
        p.x = xIt->getValue();
        p.y = yIt->getValue();
        p.z = 1.;
        
        p = Transform::matApply(transform, p);
        
        Natron::Point pixelPoint;
        pixelPoint.x = p.x / pot;
        pixelPoint.y = p.y / pot;
        points->push_back(std::make_pair(pixelPoint, pIt->getValue()));
        if (bbox) {
            bbox->x1 = p.x;
            bbox->x2 = p.x;
            bbox->y1 = p.y;
            bbox->y2 = p.y;
            double pressure = pressureAffectsSize ? pIt->getValue() : 1.;
            bbox->x1 -= halfBrushSize * pressure;
            bbox->x2 += halfBrushSize * pressure;
            bbox->y1 -= halfBrushSize * pressure;
            bbox->y2 += halfBrushSize * pressure;
        }
        return;
    }
    
    double pressure = 0;
    for ( ;
         xNext != xCurve.end() && yNext != yCurve.end() && pNext != pCurve.end();
         ++xIt, ++yIt, ++pIt, ++xNext, ++yNext, ++pNext) {
        
        assert(xIt != xCurve.end() && yIt != yCurve.end() && pIt != pCurve.end());

        double x1 = xIt->getValue();
        double y1 = yIt->getValue();
        double z1 = 1.;
        double press1 = pIt->getValue();
        double x2 = xNext->getValue();
        double y2 = yNext->getValue();
        double z2 = 1;
        double press2 = pNext->getValue();
        double dt = (xNext->getTime() - xIt->getTime());

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
        double dx1,dy1,dx2,dy2,dx3,dy3;
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
        
        pressure = std::max(pressure,pressureAffectsSize ? std::max(press1, press2) : 1.);
        
        for (double t = 0.; t <= 1.; t += incr) {
            
            Point p;
            p.x = bezier(x1, x1pr, x2pl, x2, t);
            p.y = bezier(y1, y1pr, y2pl, y2, t);
            
            if (bbox) {
                bbox->x1 = std::min(p.x,bbox->x1);
                bbox->x2 = std::max(p.x,bbox->x2);
                bbox->y1 = std::min(p.y,bbox->y1);
                bbox->y2 = std::max(p.y,bbox->y2);
            }
            
            double pi = bezier(press1, press1pr, press2pl, press2, t);
            p.x /= pot;
            p.y /= pot;
            points->push_back(std::make_pair(p, pi));
        }
    } // for (; xNext != xCurve.end() ;++xNext, ++yNext, ++pNext) {
    if (bbox) {
        bbox->x1 -= halfBrushSize * pressure;
        bbox->x2 += halfBrushSize * pressure;
        bbox->y1 -= halfBrushSize * pressure;
        bbox->y2 += halfBrushSize * pressure;
    }
}

void
RotoStrokeItem::setStrokeFinished()
{
    double time = getContext()->getTimelineCurrentTime();
    bool autoKeying = getContext()->isAutoKeyingEnabled();
    Point center;
    bool centerSet = false;
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
        
        ///Compute the value of the center knob
        center.x = center.y = 0;
        
        int nbPoints = 0;
        for (std::list<RotoStrokeItemPrivate::StrokeCurves>::const_iterator it = _imp->strokes.begin();
             it!=_imp->strokes.end(); ++it) {
            KeyFrameSet xCurve = it->xCurve->getKeyFrames_mt_safe();
            KeyFrameSet yCurve = it->yCurve->getKeyFrames_mt_safe();
            assert(xCurve.size() == yCurve.size());
            
            KeyFrameSet::iterator xIt = xCurve.begin();
            KeyFrameSet::iterator yIt = yCurve.begin();
            for (; xIt != xCurve.end(); ++xIt, ++yIt) {
                center.x += xIt->getValue();
                center.y += yIt->getValue();
            }
            nbPoints += xCurve.size();
        }
        
        centerSet = nbPoints > 0;
        if (centerSet) {
            center.x /= (double)nbPoints;
            center.y /= (double)nbPoints;
        }
    }
    if (centerSet) {
        

        boost::shared_ptr<Double_Knob> centerKnob = getCenterKnob();
        if (autoKeying) {
            centerKnob->setValueAtTime(time, center.x, 0);
            centerKnob->setValueAtTime(time, center.y, 1);
            setKeyframeOnAllTransformParameters(time);
        } else  {
            centerKnob->setValue(center.x, 0);
            centerKnob->setValue(center.y, 1);
        }
        
    }
    
    boost::shared_ptr<Node> effectNode = getEffectNode();
    boost::shared_ptr<Node> mergeNode = getMergeNode();
    boost::shared_ptr<Node> timeOffsetNode = getTimeOffsetNode();
    boost::shared_ptr<Node> frameHoldNode = getFrameHoldNode();
    
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
    
    getContext()->setStrokeBeingPainted(boost::shared_ptr<RotoStrokeItem>());
    getContext()->getNode()->setWhileCreatingPaintStroke(false);
    getContext()->clearViewersLastRenderedStrokes();
    //Might have to do this somewhere else if several viewers are active on the rotopaint node
    resetNodesThreadSafety();
    
}





bool
RotoStrokeItem::appendPoint(bool newStroke, const RotoPoint& p)
{
    assert(QThread::currentThread() == qApp->thread());


    boost::shared_ptr<RotoStrokeItem> thisShared = boost::dynamic_pointer_cast<RotoStrokeItem>(shared_from_this());
    assert(thisShared);
    {
        QMutexLocker k(&itemMutex);
        if (_imp->finished) {
            _imp->finished = false;
            if (newStroke) {
                setNodesThreadSafetyForRotopainting();
            }
        }
        
        if (_imp->strokeDotPatterns.empty()) {
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
        
        assert(stroke->xCurve->getKeyFramesCount() == stroke->yCurve->getKeyFramesCount() &&
               stroke->xCurve->getKeyFramesCount() == stroke->pressureCurve->getKeyFramesCount());
        int nk = stroke->xCurve->getKeyFramesCount();

        double t;
        if (nk == 0) {
            qDebug() << "start stroke!";
            t = 0.;
            // set time origin for this curve
            _imp->curveT0 = p.timestamp;
        } else if (p.timestamp == 0.) {
            t = nk; // some systems may not have a proper timestamp use a dummy one
        } else {
            t = p.timestamp - _imp->curveT0;
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
        qDebug("t[%d]=%g",nk,t);

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
            if ( t != tp && tp != tpp && ((t - tp) > (tp - tpp))) {
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
                double alpha = (tp - tpp)/(t - tp);
                assert(0 < alpha && alpha < 1);
                xn.setTime(tn);
                yn.setTime(tn);
                pn.setTime(tn);
                xn.setValue(xp.getValue()*(1-alpha)+p.pos.x*alpha);
                yn.setValue(yp.getValue()*(1-alpha)+p.pos.y*alpha);
                pn.setValue(pp.getValue()*(1-alpha)+p.pressure*alpha);
                _imp->xCurve.addKeyFrame(xn);
                _imp->xCurve.setKeyFrameInterpolation(Natron::eKeyframeTypeCatmullRom, nk);
                _imp->yCurve.addKeyFrame(yn);
                _imp->yCurve.setKeyFrameInterpolation(Natron::eKeyframeTypeCatmullRom, nk);
                _imp->pressureCurve.addKeyFrame(pn);
                _imp->pressureCurve.setKeyFrameInterpolation(Natron::eKeyframeTypeCatmullRom, nk);
                ++nk;
            }
        }
#endif

        bool addKeyFrameOk; // did we add a new keyframe (normally yes, but just in case)
        int ki; // index of the new keyframe (normally nk, but just in case)
        {
            KeyFrame k;
            k.setTime(t);
            k.setValue(p.pos.x);
            addKeyFrameOk = stroke->xCurve->addKeyFrame(k);
            ki = (addKeyFrameOk ? nk : (nk - 1));
        }
        {
            KeyFrame k;
            k.setTime(t);
            k.setValue(p.pos.y);
            bool aok = stroke->yCurve->addKeyFrame(k);
            assert(aok == addKeyFrameOk);
        }
        
        {
            KeyFrame k;
            k.setTime(t);
            k.setValue(p.pressure);
            bool aok = stroke->pressureCurve->addKeyFrame(k);
            assert(aok == addKeyFrameOk);
        }
        // Use CatmullRom interpolation, which means that the tangent may be modified by the next point on the curve.
        // In a previous version, the previous keyframe was set to Free so its tangents don't get overwritten, but this caused oscillations.
        stroke->xCurve->setKeyFrameInterpolation(Natron::eKeyframeTypeCatmullRom, ki);
        stroke->yCurve->setKeyFrameInterpolation(Natron::eKeyframeTypeCatmullRom, ki);
        stroke->pressureCurve->setKeyFrameInterpolation(Natron::eKeyframeTypeCatmullRom, ki);


    } // QMutexLocker k(&itemMutex);
    
    
    
    return true;
}

void
RotoStrokeItem::addStroke(const boost::shared_ptr<Curve>& xCurve,
               const boost::shared_ptr<Curve>& yCurve,
               const boost::shared_ptr<Curve>& pCurve)
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
RotoStrokeItem::removeLastStroke(boost::shared_ptr<Curve>* xCurve,
                      boost::shared_ptr<Curve>* yCurve,
                      boost::shared_ptr<Curve>* pCurve)
{
    
    bool empty;
    {
        QMutexLocker k(&itemMutex);
        if (_imp->strokes.empty()) {
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
    assert(!_imp->strokeDotPatternsMutex.tryLock());
    return _imp->strokeDotPatterns;
}

void
RotoStrokeItem::updatePatternCache(const std::vector<cairo_pattern_t*>& cache)
{
    assert(!_imp->strokeDotPatternsMutex.tryLock());
    _imp->strokeDotPatterns = cache;
}

double
RotoStrokeItem::renderSingleStroke(const boost::shared_ptr<RotoStrokeItem>& stroke,
                          const RectD& rod,
                          const std::list<std::pair<Natron::Point,double> >& points,
                          unsigned int mipmapLevel,
                          double par,
                          const Natron::ImageComponents& components,
                          Natron::ImageBitDepthEnum depth,
                          double distToNext,
                          boost::shared_ptr<Natron::Image> *wholeStrokeImage)
{
    QMutexLocker k(&_imp->strokeDotPatternsMutex);
    return getContext()->renderSingleStroke(stroke, rod, points, mipmapLevel, par, components, depth, distToNext, wholeStrokeImage);
}

RectD
RotoStrokeItem::getWholeStrokeRoDWhilePainting() const
{
    QMutexLocker k(&itemMutex);
    return _imp->wholeStrokeBboxWhilePainting;
}

bool
RotoStrokeItem::getMostRecentStrokeChangesSinceAge(int time,int lastAge,
                                                   std::list<std::pair<Natron::Point,double> >* points,
                                                   RectD* pointsBbox,
                                                   RectD* wholeStrokeBbox,
                                                   int* newAge)
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
    assert(!_imp->strokes.empty());
    RotoStrokeItemPrivate::StrokeCurves& stroke = _imp->strokes.back();
    assert(stroke.xCurve->getKeyFramesCount() == stroke.yCurve->getKeyFramesCount() && stroke.xCurve->getKeyFramesCount() == stroke.pressureCurve->getKeyFramesCount());
    
  
    
    KeyFrameSet xCurve = stroke.xCurve->getKeyFrames_mt_safe();
    KeyFrameSet yCurve = stroke.yCurve->getKeyFrames_mt_safe();
    KeyFrameSet pCurve = stroke.pressureCurve->getKeyFrames_mt_safe();
    
    if (xCurve.empty()) {
        return false;
    }
    if (lastAge == -1) {
        lastAge = 0;
    }
    
    assert(lastAge < (int)xCurve.size());
    
    KeyFrameSet  realX,realY,realP;
    KeyFrameSet::iterator xIt = xCurve.begin();
    KeyFrameSet::iterator yIt = yCurve.begin();
    KeyFrameSet::iterator pIt = pCurve.begin();
    std::advance(xIt, lastAge);
    std::advance(yIt, lastAge);
    std::advance(pIt, lastAge);
    
    *newAge = (int)xCurve.size() - 1;
    
    for (; xIt != xCurve.end(); ++xIt,++yIt,++pIt) {
        realX.insert(*xIt);
        realY.insert(*yIt);
        realP.insert(*pIt);
    }
    
    if (realX.empty()) {
        return false;
    }
    
    double halfBrushSize = getBrushSizeKnob()->getValue() / 2.;
    bool pressureSize = getPressureSizeKnob()->getValue();
    evaluateStrokeInternal(realX, realY, realP, transform, 0, halfBrushSize, pressureSize, points, pointsBbox);
    
    if (!wholeStrokeBbox->isNull()) {
        wholeStrokeBbox->merge(*pointsBbox);
    } else {
        *wholeStrokeBbox = *pointsBbox;
    }

    _imp->wholeStrokeBboxWhilePainting = *wholeStrokeBbox;
    
    return true;
}


void
RotoStrokeItem::clone(const RotoItem* other)
{

    const RotoStrokeItem* otherStroke = dynamic_cast<const RotoStrokeItem*>(other);
    assert(otherStroke);
    {
        QMutexLocker k(&itemMutex);
        _imp->strokes.clear();
        for (std::list<RotoStrokeItemPrivate::StrokeCurves>::const_iterator it = otherStroke->_imp->strokes.begin();
             it!=otherStroke->_imp->strokes.end(); ++it) {
            RotoStrokeItemPrivate::StrokeCurves s;
            s.xCurve.reset(new Curve);
            s.yCurve.reset(new Curve);
            s.pressureCurve.reset(new Curve);
            s.xCurve->clone(*(it->xCurve));
            s.yCurve->clone(*(it->yCurve));
            s.pressureCurve->clone(*(it->pressureCurve));
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
    RotoStrokeSerialization* s = dynamic_cast<RotoStrokeSerialization*>(obj);
    assert(s);
    {
        QMutexLocker k(&itemMutex);
        s->_brushType = (int)_imp->type;
        for (std::list<RotoStrokeItemPrivate::StrokeCurves>::const_iterator it = _imp->strokes.begin();
             it!=_imp->strokes.end(); ++it) {
            boost::shared_ptr<Curve> xCurve(new Curve);
            boost::shared_ptr<Curve> yCurve(new Curve);
            boost::shared_ptr<Curve> pressureCurve(new Curve);
            xCurve->clone(*(it->xCurve));
            yCurve->clone(*(it->yCurve));
            pressureCurve->clone(*(it->pressureCurve));
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
    const RotoStrokeSerialization* s = dynamic_cast<const RotoStrokeSerialization*>(&obj);
    assert(s);
    {
        QMutexLocker k(&itemMutex);
        _imp->type = (Natron::RotoStrokeType)s->_brushType;
        
        assert(s->_xCurves.size() == s->_yCurves.size() && s->_xCurves.size() == s->_pressureCurves.size());
        std::list<boost::shared_ptr<Curve> >::const_iterator itY = s->_yCurves.begin();
        std::list<boost::shared_ptr<Curve> >::const_iterator itP = s->_pressureCurves.begin();
        for (std::list<boost::shared_ptr<Curve> >::const_iterator it = s->_xCurves.begin();
             it!=s->_xCurves.end(); ++it,++itY,++itP) {
            RotoStrokeItemPrivate::StrokeCurves s;
            s.xCurve.reset(new Curve);
            s.yCurve.reset(new Curve);
            s.pressureCurve.reset(new Curve);
            s.xCurve->clone(**(it));
            s.yCurve->clone(**(itY));
            s.pressureCurve->clone(**(itP));
            _imp->strokes.push_back(s);
            
        }
    }
    
    
    setStrokeFinished();

    
}


RectD
RotoStrokeItem::computeBoundingBox(double time) const
{
    RectD bbox;
    
    Transform::Matrix3x3 transform;
    getTransformAtTime(time, &transform);
    bool pressureAffectsSize = getPressureSizeKnob()->getValueAtTime(time);
    
    QMutexLocker k(&itemMutex);
    bool bboxSet = false;
    
    double halfBrushSize = getBrushSizeKnob()->getValueAtTime(time) / 2. + 1;
    
    for (std::list<RotoStrokeItemPrivate::StrokeCurves>::const_iterator it = _imp->strokes.begin(); it != _imp->strokes.end(); ++it) {
        KeyFrameSet xCurve = it->xCurve->getKeyFrames_mt_safe();
        KeyFrameSet yCurve = it->yCurve->getKeyFrames_mt_safe();
        KeyFrameSet pCurve = it->pressureCurve->getKeyFrames_mt_safe();
        
        if (xCurve.empty()) {
            return RectD();
        }
        
        assert(xCurve.size() == yCurve.size() && xCurve.size() == pCurve.size());
        
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
        
        for (;xNext != xCurve.end(); ++xIt,++yIt,++pIt, ++xNext, ++yNext, ++pNext) {
            
            RectD subBox;
            subBox.setupInfinity();
            
            double dt = xNext->getTime() - xIt->getTime();
            
            double pressure = pressureAffectsSize ? std::max(pIt->getValue(), pNext->getValue()) : 1.;
            Transform::Point3D p0,p1,p2,p3;
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
            
            Point p0_,p1_,p2_,p3_;
            p0_.x = p0.x; p0_.y = p0.y;
            p1_.x = p1.x; p1_.y = p1.y;
            p2_.x = p2.x; p2_.y = p2.y;
            p3_.x = p3.x; p3_.y = p3.y;
            
            bezierPointBboxUpdate(p0_,p1_,p2_,p3_,&subBox);
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
}

RectD
RotoStrokeItem::getBoundingBox(double time) const
{

    
    bool isActivated = getActivatedKnob()->getValueAtTime(time);
    if (!isActivated)  {
        return RectD();
    }

    return computeBoundingBox(time);
    
}

std::list<boost::shared_ptr<Curve> >
RotoStrokeItem::getXControlPoints() const
{
    assert(QThread::currentThread() == qApp->thread());
    std::list<boost::shared_ptr<Curve> > ret;
    QMutexLocker k(&itemMutex);
    for (std::list<RotoStrokeItemPrivate::StrokeCurves>::const_iterator it = _imp->strokes.begin() ;it!=_imp->strokes.end(); ++it) {
        ret.push_back(it->xCurve);
    }
    return ret;
}

std::list<boost::shared_ptr<Curve> >
RotoStrokeItem::getYControlPoints() const
{
    assert(QThread::currentThread() == qApp->thread());
    std::list<boost::shared_ptr<Curve> > ret;
    QMutexLocker k(&itemMutex);
    for (std::list<RotoStrokeItemPrivate::StrokeCurves>::const_iterator it = _imp->strokes.begin() ;it!=_imp->strokes.end(); ++it) {
        ret.push_back(it->xCurve);
    }
    return ret;
}

void
RotoStrokeItem::evaluateStroke(unsigned int mipMapLevel, double time, std::list<std::list<std::pair<Natron::Point,double> > >* strokes,
                               RectD* bbox) const
{
    double brushSize = getBrushSizeKnob()->getValueAtTime(time) / 2.;
    bool pressureAffectsSize = getPressureSizeKnob()->getValueAtTime(time);
    
    Transform::Matrix3x3 transform;
    getTransformAtTime(time, &transform);
    
    bool bboxSet = false;
    for (std::list<RotoStrokeItemPrivate::StrokeCurves>::const_iterator it = _imp->strokes.begin() ;it != _imp->strokes.end(); ++it) {
        KeyFrameSet xSet,ySet,pSet;
        {
            QMutexLocker k(&itemMutex);
            xSet = it->xCurve->getKeyFrames_mt_safe();
            ySet = it->yCurve->getKeyFrames_mt_safe();
            pSet = it->pressureCurve->getKeyFrames_mt_safe();
        }
        assert(xSet.size() == ySet.size() && xSet.size() == pSet.size());
        
        std::list<std::pair<Natron::Point,double> > points;
        RectD strokeBbox;
        
        evaluateStrokeInternal(xSet,ySet,pSet, transform, mipMapLevel,brushSize, pressureAffectsSize, &points,&strokeBbox);
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



////////////////////////////////////RotoContext////////////////////////////////////


RotoContext::RotoContext(const boost::shared_ptr<Natron::Node>& node)
    : _imp( new RotoContextPrivate(node) )
{
    QObject::connect(_imp->lifeTime.lock()->getSignalSlotHandler().get(), SIGNAL(valueChanged(int,int)), this, SLOT(onLifeTimeKnobValueChanged(int,int)));
}

bool
RotoContext::isRotoPaint() const
{
    return _imp->isPaintNode;
}

void
RotoContext::setStrokeBeingPainted(const boost::shared_ptr<RotoStrokeItem>& stroke)
{
    QMutexLocker k(&_imp->rotoContextMutex);
    _imp->strokeBeingPainted = stroke;
}

boost::shared_ptr<RotoStrokeItem>
RotoContext::getStrokeBeingPainted() const
{
    QMutexLocker k(&_imp->rotoContextMutex);
    return _imp->strokeBeingPainted;
}

void
RotoContext::getRotoPaintTreeNodes(std::list<boost::shared_ptr<Natron::Node> >* nodes) const
{
    std::list<boost::shared_ptr<RotoDrawableItem> > items = getCurvesByRenderOrder(false);
    for (std::list<boost::shared_ptr<RotoDrawableItem> >::iterator it = items.begin(); it != items.end(); ++it) {
      
        boost::shared_ptr<Natron::Node> effectNode = (*it)->getEffectNode();
        boost::shared_ptr<Natron::Node> mergeNode = (*it)->getMergeNode();
        boost::shared_ptr<Natron::Node> timeOffsetNode = (*it)->getTimeOffsetNode();
        boost::shared_ptr<Natron::Node> frameHoldNode = (*it)->getFrameHoldNode();
        if (effectNode) {
            nodes->push_back(effectNode);
        }
        if (mergeNode) {
            nodes->push_back(mergeNode);
        }
        if (timeOffsetNode) {
            nodes->push_back(timeOffsetNode);
        }
        if (frameHoldNode) {
            nodes->push_back(frameHoldNode);
        }
    }
}

///Must be done here because at the time of the constructor, the shared_ptr doesn't exist yet but
///addLayer() needs it to get a shared ptr to this
void
RotoContext::createBaseLayer()
{
    ////Add the base layer
    boost::shared_ptr<RotoLayer> base = addLayerInternal(false);
    
    deselect(base, RotoItem::eSelectionReasonOther);
}

RotoContext::~RotoContext()
{
}

boost::shared_ptr<RotoLayer>
RotoContext::getOrCreateBaseLayer()
{
    QMutexLocker k(&_imp->rotoContextMutex);
    if (_imp->layers.empty()) {
        k.unlock();
        addLayer();
        k.relock();
    }
    assert(!_imp->layers.empty());
    return _imp->layers.front();
}

boost::shared_ptr<RotoLayer>
RotoContext::addLayerInternal(bool declarePython)
{
    boost::shared_ptr<RotoContext> this_shared = shared_from_this();
    assert(this_shared);
    
    ///MT-safe: only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    boost::shared_ptr<RotoLayer> item;
    
    std::string name = generateUniqueName(kRotoLayerBaseName);
    {
        
        boost::shared_ptr<RotoLayer> deepestLayer;
        boost::shared_ptr<RotoLayer> parentLayer;
        {
            QMutexLocker l(&_imp->rotoContextMutex);
            deepestLayer = _imp->findDeepestSelectedLayer();

            if (!deepestLayer) {
                ///find out if there's a base layer, if so add to the base layer,
                ///otherwise create the base layer
                for (std::list<boost::shared_ptr<RotoLayer> >::iterator it = _imp->layers.begin(); it != _imp->layers.end(); ++it) {
                    int hierarchy = (*it)->getHierarchyLevel();
                    if (hierarchy == 0) {
                        parentLayer = *it;
                        break;
                    }
                }
            } else {
                parentLayer = deepestLayer;
            }
        }
        
        item.reset( new RotoLayer(this_shared, name, boost::shared_ptr<RotoLayer>()) );
        if (parentLayer) {
            parentLayer->addItem(item,declarePython);
        }
        
        QMutexLocker l(&_imp->rotoContextMutex);

        _imp->layers.push_back(item);
        
        _imp->lastInsertedItem = item;
    }
    
    Q_EMIT itemInserted(RotoItem::eSelectionReasonOther);
    
    
    clearSelection(RotoItem::eSelectionReasonOther);
    select(item, RotoItem::eSelectionReasonOther);
    
    return item;

}

boost::shared_ptr<RotoLayer>
RotoContext::addLayer()
{
    return addLayerInternal(true);
} // addLayer

void
RotoContext::addLayer(const boost::shared_ptr<RotoLayer> & layer)
{
    std::list<boost::shared_ptr<RotoLayer> >::iterator it = std::find(_imp->layers.begin(), _imp->layers.end(), layer);

    if ( it == _imp->layers.end() ) {
        _imp->layers.push_back(layer);
    }
}

boost::shared_ptr<RotoItem>
RotoContext::getLastInsertedItem() const
{
    ///MT-safe: only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );

    return _imp->lastInsertedItem;
}

#ifdef NATRON_ROTO_INVERTIBLE
boost::shared_ptr<Bool_Knob>
RotoContext::getInvertedKnob() const
{
    return _imp->inverted.lock();
}

#endif

boost::shared_ptr<Color_Knob>
RotoContext::getColorKnob() const
{
    return _imp->colorKnob.lock();
}

void
RotoContext::setAutoKeyingEnabled(bool enabled)
{
    ///MT-safe: only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );

    QMutexLocker l(&_imp->rotoContextMutex);
    _imp->autoKeying = enabled;
}

bool
RotoContext::isAutoKeyingEnabled() const
{
    ///MT-safe: only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );

    QMutexLocker l(&_imp->rotoContextMutex);

    return _imp->autoKeying;
}

void
RotoContext::setFeatherLinkEnabled(bool enabled)
{
    ///MT-safe: only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );

    QMutexLocker l(&_imp->rotoContextMutex);
    _imp->featherLink = enabled;
}

bool
RotoContext::isFeatherLinkEnabled() const
{
    ///MT-safe: only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );

    QMutexLocker l(&_imp->rotoContextMutex);

    return _imp->featherLink;
}

void
RotoContext::setRippleEditEnabled(bool enabled)
{
    ///MT-safe: only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );

    QMutexLocker l(&_imp->rotoContextMutex);
    _imp->rippleEdit = enabled;
}

bool
RotoContext::isRippleEditEnabled() const
{
    ///MT-safe: only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );

    QMutexLocker l(&_imp->rotoContextMutex);

    return _imp->rippleEdit;
}

boost::shared_ptr<Natron::Node>
RotoContext::getNode() const
{
    return _imp->node.lock();
}

int
RotoContext::getTimelineCurrentTime() const
{
    return getNode()->getApp()->getTimeLine()->currentFrame();
}

std::string
RotoContext::generateUniqueName(const std::string& baseName)
{
    int no = 1;
    
    bool foundItem;
    std::string name;
    do {
        std::stringstream ss;
        ss << baseName;
        ss << no;
        name = ss.str();
        if (getItemByName(name)) {
            foundItem = true;
        } else {
            foundItem = false;
        }
        ++no;
    } while (foundItem);
    return name;
}

boost::shared_ptr<Bezier>
RotoContext::makeBezier(double x,
                        double y,
                        const std::string & baseName,
                        double time,
                        bool isOpenBezier)
{
    ///MT-safe: only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    boost::shared_ptr<RotoLayer> parentLayer;
    boost::shared_ptr<RotoContext> this_shared = boost::dynamic_pointer_cast<RotoContext>(shared_from_this());
    assert(this_shared);
    std::string name = generateUniqueName(baseName);

    {

        QMutexLocker l(&_imp->rotoContextMutex);
        boost::shared_ptr<RotoLayer> deepestLayer = _imp->findDeepestSelectedLayer();


        if (!deepestLayer) {
            ///if there is no base layer, create one
            if ( _imp->layers.empty() ) {
                l.unlock();
                addLayer();
                l.relock();
            }
            parentLayer = _imp->layers.front();
        } else {
            parentLayer = deepestLayer;
        }
    }
    assert(parentLayer);
    boost::shared_ptr<Bezier> curve( new Bezier(this_shared, name, boost::shared_ptr<RotoLayer>(), isOpenBezier) );
    curve->createNodes();
    if (parentLayer) {
        parentLayer->insertItem(curve,0);
    }
    _imp->lastInsertedItem = curve;

    Q_EMIT itemInserted(RotoItem::eSelectionReasonOther);


    clearSelection(RotoItem::eSelectionReasonOther);
    select(curve, RotoItem::eSelectionReasonOther);

    if ( isAutoKeyingEnabled() ) {
        curve->setKeyframe( getTimelineCurrentTime() );
    }
    curve->addControlPoint(x, y, time);
    
    return curve;
} // makeBezier

boost::shared_ptr<RotoStrokeItem>
RotoContext::makeStroke(Natron::RotoStrokeType type,const std::string& baseName,bool clearSel)
{
    ///MT-safe: only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    boost::shared_ptr<RotoLayer> parentLayer;
    boost::shared_ptr<RotoContext> this_shared = boost::dynamic_pointer_cast<RotoContext>(shared_from_this());
    assert(this_shared);
    std::string name = generateUniqueName(baseName);
    
    {
        
        QMutexLocker l(&_imp->rotoContextMutex);
        ++_imp->age; // increase age 
        
        boost::shared_ptr<RotoLayer> deepestLayer = _imp->findDeepestSelectedLayer();
        
        
        if (!deepestLayer) {
            ///if there is no base layer, create one
            if ( _imp->layers.empty() ) {
                l.unlock();
                addLayer();
                l.relock();
            }
            parentLayer = _imp->layers.front();
        } else {
            parentLayer = deepestLayer;
        }
    }
    assert(parentLayer);
    boost::shared_ptr<RotoStrokeItem> curve( new RotoStrokeItem(type,this_shared, name, boost::shared_ptr<RotoLayer>()) );
    if (parentLayer) {
        parentLayer->insertItem(curve,0);
    }
    curve->createNodes();
    _imp->strokeBeingPainted = curve;
    
    _imp->lastInsertedItem = curve;
    
    Q_EMIT itemInserted(RotoItem::eSelectionReasonOther);
    
    if (clearSel) {
        clearSelection(RotoItem::eSelectionReasonOther);
        select(curve, RotoItem::eSelectionReasonOther);
    }
    return curve;
}

boost::shared_ptr<Bezier>
RotoContext::makeEllipse(double x,double y,double diameter,bool fromCenter, double time)
{
    double half = diameter / 2.;
    boost::shared_ptr<Bezier> curve = makeBezier(x , fromCenter ? y - half : y ,kRotoEllipseBaseName, time, false);
    if (fromCenter) {
        curve->addControlPoint(x + half,y, time);
        curve->addControlPoint(x,y + half, time);
        curve->addControlPoint(x - half,y, time);
    } else {
        curve->addControlPoint(x + diameter,y - diameter, time);
        curve->addControlPoint(x,y - diameter, time);
        curve->addControlPoint(x - diameter,y - diameter, time);
    }
    
    boost::shared_ptr<BezierCP> top = curve->getControlPointAtIndex(0);
    boost::shared_ptr<BezierCP> right = curve->getControlPointAtIndex(1);
    boost::shared_ptr<BezierCP> bottom = curve->getControlPointAtIndex(2);
    boost::shared_ptr<BezierCP> left = curve->getControlPointAtIndex(3);

    double topX,topY,rightX,rightY,btmX,btmY,leftX,leftY;
    top->getPositionAtTime(false,time, &topX, &topY);
    right->getPositionAtTime(false,time, &rightX, &rightY);
    bottom->getPositionAtTime(false,time, &btmX, &btmY);
    left->getPositionAtTime(false,time, &leftX, &leftY);
    
    curve->setLeftBezierPoint(0, time,  (leftX + topX) / 2., topY);
    curve->setRightBezierPoint(0, time, (rightX + topX) / 2., topY);
    
    curve->setLeftBezierPoint(1, time,  rightX, (rightY + topY) / 2.);
    curve->setRightBezierPoint(1, time, rightX, (rightY + btmY) / 2.);
    
    curve->setLeftBezierPoint(2, time,  (rightX + btmX) / 2., btmY);
    curve->setRightBezierPoint(2, time, (leftX + btmX) / 2., btmY);
    
    curve->setLeftBezierPoint(3, time,   leftX, (btmY + leftY) / 2.);
    curve->setRightBezierPoint(3, time, leftX, (topY + leftY) / 2.);
    curve->setCurveFinished(true);
    
    return curve;
}

boost::shared_ptr<Bezier>
RotoContext::makeSquare(double x,double y,double initialSize,double time)
{
    boost::shared_ptr<Bezier> curve = makeBezier(x,y,kRotoRectangleBaseName,time, false);
    curve->addControlPoint(x + initialSize,y, time);
    curve->addControlPoint(x + initialSize,y - initialSize, time);
    curve->addControlPoint(x,y - initialSize, time);
    curve->setCurveFinished(true);
    
    return curve;

}

void
RotoContext::removeItemRecursively(const boost::shared_ptr<RotoItem>& item,
                                   RotoItem::SelectionReasonEnum reason)
{
    boost::shared_ptr<RotoLayer> isLayer = boost::dynamic_pointer_cast<RotoLayer>(item);
    boost::shared_ptr<RotoItem> foundSelected;
    
    {
        QMutexLocker l(&_imp->rotoContextMutex);
        for (std::list< boost::shared_ptr<RotoItem> >::iterator it = _imp->selectedItems.begin(); it != _imp->selectedItems.end(); ++it) {
            if (*it == item) {
                foundSelected = *it;
                break;
            }
        }
    }
    if (foundSelected) {
        deselectInternal(foundSelected);
    }
    
    if (isLayer) {
        
        const RotoItems & items = isLayer->getItems();
        for (RotoItems::const_iterator it = items.begin(); it != items.end(); ++it) {
            removeItemRecursively(*it, reason);
        }
        QMutexLocker l(&_imp->rotoContextMutex);

        for (std::list<boost::shared_ptr<RotoLayer> >::iterator it = _imp->layers.begin(); it != _imp->layers.end(); ++it) {
            if (*it == isLayer) {
                _imp->layers.erase(it);
                break;
            }
        }
    }
    Q_EMIT itemRemoved(item,(int)reason);
}

void
RotoContext::removeItem(const boost::shared_ptr<RotoItem>& item,
                        RotoItem::SelectionReasonEnum reason)
{
    ///MT-safe: only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    
    boost::shared_ptr<RotoLayer> layer = item->getParentLayer();
    if (layer) {
        layer->removeItem(item);
    }
    
    removeItemRecursively(item,reason);
    
    Q_EMIT selectionChanged( (int)reason );
}

void
RotoContext::addItem(const boost::shared_ptr<RotoLayer>& layer,
                     int indexInLayer,
                     const boost::shared_ptr<RotoItem> & item,
                     RotoItem::SelectionReasonEnum reason)
{
    ///MT-safe: only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    {
        if (layer) {
            layer->insertItem(item,indexInLayer);
        }
        
        QMutexLocker l(&_imp->rotoContextMutex);

        boost::shared_ptr<RotoLayer> isLayer = boost::dynamic_pointer_cast<RotoLayer>(item);
        if (isLayer) {
            std::list<boost::shared_ptr<RotoLayer> >::iterator foundLayer = std::find(_imp->layers.begin(), _imp->layers.end(), isLayer);
            if ( foundLayer == _imp->layers.end() ) {
                _imp->layers.push_back(isLayer);
            }
        }
        _imp->lastInsertedItem = item;
    }
    Q_EMIT itemInserted(reason);
}


const std::list< boost::shared_ptr<RotoLayer> > &
RotoContext::getLayers() const
{
    ///MT-safe: only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );

    return _imp->layers;
}

boost::shared_ptr<Bezier>
RotoContext::isNearbyBezier(double x,
                            double y,
                            double acceptance,
                            int* index,
                            double* t,
                            bool* feather) const
{
    ///MT-safe: only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );

    QMutexLocker l(&_imp->rotoContextMutex);
    std::list<std::pair<boost::shared_ptr<Bezier>, std::pair<int,double> > > nearbyBeziers;
    for (std::list< boost::shared_ptr<RotoLayer> >::const_iterator it = _imp->layers.begin(); it != _imp->layers.end(); ++it) {
        const RotoItems & items = (*it)->getItems();
        for (RotoItems::const_iterator it2 = items.begin(); it2 != items.end(); ++it2) {
            boost::shared_ptr<Bezier> b = boost::dynamic_pointer_cast<Bezier>(*it2);
            if (b && !b->isLockedRecursive()) {
                double param;
                int i = b->isPointOnCurve(x, y, acceptance, &param,feather);
                if (i != -1) {
                    nearbyBeziers.push_back(std::make_pair(b, std::make_pair(i, param)));
                }
            }
        }
    }
    
    std::list<std::pair<boost::shared_ptr<Bezier>, std::pair<int,double> > >::iterator firstNotSelected = nearbyBeziers.end();
    for (std::list<std::pair<boost::shared_ptr<Bezier>, std::pair<int,double> > >::iterator it = nearbyBeziers.begin(); it!=nearbyBeziers.end(); ++it) {
        bool foundSelected = false;
        for (std::list<boost::shared_ptr<RotoItem> >::iterator it2 = _imp->selectedItems.begin(); it2 != _imp->selectedItems.end(); ++it2) {
            if (it2->get() == it->first.get()) {
                foundSelected = true;
                break;
            }
        }
        if (foundSelected) {
            *index = it->second.first;
            *t = it->second.second;
            return it->first;
        } else if (firstNotSelected == nearbyBeziers.end()) {
            firstNotSelected = it;
        }
    }
    if (firstNotSelected != nearbyBeziers.end()) {
        *index = firstNotSelected->second.first;
        *t = firstNotSelected->second.second;
        return firstNotSelected->first;
    }

    return boost::shared_ptr<Bezier>();
}

void
RotoContext::onLifeTimeKnobValueChanged(int /*dim*/, int reason)
{
    if ((Natron::ValueChangedReasonEnum)reason != eValueChangedReasonUserEdited) {
        return;
    }
    int lifetime_i = _imp->lifeTime.lock()->getValue();
    _imp->activated.lock()->setSecret(lifetime_i != 3);
    boost::shared_ptr<Int_Knob> frame = _imp->lifeTimeFrame.lock();
    frame->setSecret(lifetime_i == 3);
    if (lifetime_i != 3) {
        frame->setValue(getTimelineCurrentTime(), 0);
    }
}

void
RotoContext::onAutoKeyingChanged(bool enabled)
{
    ///MT-safe: only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    QMutexLocker l(&_imp->rotoContextMutex);
    _imp->autoKeying = enabled;
}

void
RotoContext::onFeatherLinkChanged(bool enabled)
{
    ///MT-safe: only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    QMutexLocker l(&_imp->rotoContextMutex);
    _imp->featherLink = enabled;
}

void
RotoContext::onRippleEditChanged(bool enabled)
{
    ///MT-safe: only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    QMutexLocker l(&_imp->rotoContextMutex);
    _imp->rippleEdit = enabled;
}

void
RotoContext::getMaskRegionOfDefinition(double time,
                                       int /*view*/,
                                       RectD* rod) // rod is in canonical coordinates
const
{
    
    QMutexLocker l(&_imp->rotoContextMutex);
    bool first = true;

    for (std::list<boost::shared_ptr<RotoLayer> >::const_iterator it = _imp->layers.begin(); it != _imp->layers.end(); ++it) {
        RotoItems items = (*it)->getItems_mt_safe();
        for (RotoItems::iterator it2 = items.begin(); it2 != items.end(); ++it2) {
            Bezier* isBezier = dynamic_cast<Bezier*>(it2->get());
            RotoStrokeItem* isStroke = dynamic_cast<RotoStrokeItem*>(it2->get());
            if (isBezier && !isStroke) {
                if (isBezier->isActivated(time)  && isBezier->getControlPointsCount() > 1) {
                    RectD splineRoD = isBezier->getBoundingBox(time);
                    if ( splineRoD.isNull() ) {
                        continue;
                    }
                    
                    if (first) {
                        first = false;
                        *rod = splineRoD;
                    } else {
                        rod->merge(splineRoD);
                    }
                }
            } else if (isStroke) {
                RectD strokeRod;
                if (isStroke->isActivated(time)) {
                    if (isStroke == _imp->strokeBeingPainted.get()) {
                        strokeRod = isStroke->getMergeNode()->getPaintStrokeRoD_duringPainting();
                    } else {
                        strokeRod = isStroke->getBoundingBox(time);
                    }
                    if (first) {
                        first = false;
                        *rod = strokeRod;
                    } else {
                        rod->merge(strokeRod);
                    }
                }
            }
        }
    }
}

bool
RotoContext::isEmpty() const
{
    QMutexLocker l(&_imp->rotoContextMutex);

    return _imp->layers.empty();
}

void
RotoContext::save(RotoContextSerialization* obj) const
{
    QMutexLocker l(&_imp->rotoContextMutex);

    obj->_autoKeying = _imp->autoKeying;
    obj->_featherLink = _imp->featherLink;
    obj->_rippleEdit = _imp->rippleEdit;

    ///There must always be the base layer
    assert( !_imp->layers.empty() );

    ///Serializing this layer will recursively serialize everything
    _imp->layers.front()->save( dynamic_cast<RotoItemSerialization*>(&obj->_baseLayer) );

    ///the age of the context is not serialized as the images are wiped from the cache anyway

    ///Serialize the selection
    for (std::list<boost::shared_ptr<RotoItem> >::const_iterator it = _imp->selectedItems.begin(); it != _imp->selectedItems.end(); ++it) {
        obj->_selectedItems.push_back( (*it)->getScriptName() );
    }
}

static void
linkItemsKnobsRecursively(RotoContext* ctx,
                          const boost::shared_ptr<RotoLayer> & layer)
{
    const RotoItems & items = layer->getItems();

    for (RotoItems::const_iterator it = items.begin(); it != items.end(); ++it) {
        boost::shared_ptr<Bezier> isBezier = boost::dynamic_pointer_cast<Bezier>(*it);
        boost::shared_ptr<RotoLayer> isLayer = boost::dynamic_pointer_cast<RotoLayer>(*it);

        if (isBezier) {
            ctx->select(isBezier, RotoItem::eSelectionReasonOther);
        } else if (isLayer) {
            linkItemsKnobsRecursively(ctx, isLayer);
        }
    }
}

void
RotoContext::load(const RotoContextSerialization & obj)
{
    assert( QThread::currentThread() == qApp->thread() );
    ///no need to lock here, when this is called the main-thread is the only active thread

    _imp->isCurrentlyLoading = true;
    _imp->autoKeying = obj._autoKeying;
    _imp->featherLink = obj._featherLink;
    _imp->rippleEdit = obj._rippleEdit;

    for (std::list<boost::weak_ptr<KnobI> >::iterator it = _imp->knobs.begin(); it != _imp->knobs.end(); ++it) {
        it->lock()->setAllDimensionsEnabled(false);
    }

    assert(_imp->layers.size() == 1);

    boost::shared_ptr<RotoLayer> baseLayer = _imp->layers.front();

    baseLayer->load(obj._baseLayer);

    for (std::list<std::string>::const_iterator it = obj._selectedItems.begin(); it != obj._selectedItems.end(); ++it) {
        boost::shared_ptr<RotoItem> item = getItemByName(*it);
        boost::shared_ptr<Bezier> isBezier = boost::dynamic_pointer_cast<Bezier>(item);
        boost::shared_ptr<RotoLayer> isLayer = boost::dynamic_pointer_cast<RotoLayer>(item);
        if (isBezier) {
            select(isBezier,RotoItem::eSelectionReasonOther);
        } else if (isLayer) {
            linkItemsKnobsRecursively(this, isLayer);
        }
    }
    _imp->isCurrentlyLoading = false;
    refreshRotoPaintTree();
}

void
RotoContext::select(const boost::shared_ptr<RotoItem> & b,
                    RotoItem::SelectionReasonEnum reason)
{
    
    selectInternal(b);
    Q_EMIT selectionChanged( (int)reason );
}

void
RotoContext::select(const std::list<boost::shared_ptr<RotoDrawableItem> > & beziers,
                    RotoItem::SelectionReasonEnum reason)
{
    
    for (std::list<boost::shared_ptr<RotoDrawableItem> >::const_iterator it = beziers.begin(); it != beziers.end(); ++it) {
        selectInternal(*it);
    }
    
    Q_EMIT selectionChanged( (int)reason );
}

void
RotoContext::select(const std::list<boost::shared_ptr<RotoItem> > & items,
                    RotoItem::SelectionReasonEnum reason)
{
    
    for (std::list<boost::shared_ptr<RotoItem> >::const_iterator it = items.begin(); it != items.end(); ++it) {
        selectInternal(*it);
    }
    
    Q_EMIT selectionChanged( (int)reason );
}

void
RotoContext::deselect(const boost::shared_ptr<RotoItem> & b,
                      RotoItem::SelectionReasonEnum reason)
{
    
    deselectInternal(b);
    
    Q_EMIT selectionChanged( (int)reason );
}

void
RotoContext::deselect(const std::list<boost::shared_ptr<Bezier> > & beziers,
                      RotoItem::SelectionReasonEnum reason)
{
    
    for (std::list<boost::shared_ptr<Bezier> >::const_iterator it = beziers.begin(); it != beziers.end(); ++it) {
        deselectInternal(*it);
    }
    
    Q_EMIT selectionChanged( (int)reason );
}

void
RotoContext::deselect(const std::list<boost::shared_ptr<RotoItem> > & items,
                      RotoItem::SelectionReasonEnum reason)
{
    
    for (std::list<boost::shared_ptr<RotoItem> >::const_iterator it = items.begin(); it != items.end(); ++it) {
        deselectInternal(*it);
    }
    
    Q_EMIT selectionChanged( (int)reason );
}

void
RotoContext::clearAndSelectPreviousItem(const boost::shared_ptr<RotoItem>& item,RotoItem::SelectionReasonEnum reason)
{
    clearSelection(reason);
    assert(item);
    boost::shared_ptr<RotoItem> prev = item->getPreviousItemInLayer();
    if (prev) {
        select(prev,reason);
    }
}

void
RotoContext::clearSelection(RotoItem::SelectionReasonEnum reason)
{
    bool empty;
    {
        QMutexLocker l(&_imp->rotoContextMutex);
        empty = _imp->selectedItems.empty();
    }
    while (!empty) {
        boost::shared_ptr<RotoItem> item;
        {
            QMutexLocker l(&_imp->rotoContextMutex);
            item = _imp->selectedItems.front();
        }
        deselectInternal(item);
        
        QMutexLocker l(&_imp->rotoContextMutex);
        empty = _imp->selectedItems.empty();
    }
    Q_EMIT selectionChanged( (int)reason );
}

void
RotoContext::selectInternal(const boost::shared_ptr<RotoItem> & item)
{
    ///only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    
    int nbUnlockedBeziers = 0;
    int nbUnlockedStrokes = 0;
    int nbStrokeWithoutStrength = 0;
    int nbStrokeWithoutCloneFunctions = 0;
    bool foundItem = false;
    
    {
        QMutexLocker l(&_imp->rotoContextMutex);
        for (std::list<boost::shared_ptr<RotoItem> >::iterator it = _imp->selectedItems.begin(); it != _imp->selectedItems.end(); ++it) {
            Bezier* isBezier = dynamic_cast<Bezier*>(it->get());
            RotoStrokeItem* isStroke = dynamic_cast<RotoStrokeItem*>(it->get());
            if (!isStroke && isBezier && !isBezier->isLockedRecursive()) {
                if (isBezier->isOpenBezier()) {
                    ++nbUnlockedStrokes;
                } else {
                    ++nbUnlockedBeziers;
                }
            } else if (isStroke) {
                ++nbUnlockedStrokes;
            }
            if (it->get() == item.get()) {
                foundItem = true;
            }
        }
    }
    ///the item is already selected, exit
    if (foundItem) {
        return;
    }
    
    
    boost::shared_ptr<Bezier> isBezier = boost::dynamic_pointer_cast<Bezier>(item);
    boost::shared_ptr<RotoStrokeItem> isStroke = boost::dynamic_pointer_cast<RotoStrokeItem>(item);
    RotoDrawableItem* isDrawable = dynamic_cast<RotoDrawableItem*>(item.get());
    boost::shared_ptr<RotoLayer> isLayer = boost::dynamic_pointer_cast<RotoLayer>(item);

    if (isDrawable) {
        if (!isStroke && isBezier && !isBezier->isLockedRecursive()) {
            if (isBezier->isOpenBezier()) {
                ++nbUnlockedStrokes;
            } else {
                ++nbUnlockedBeziers;
            }
            ++nbStrokeWithoutCloneFunctions;
            ++nbStrokeWithoutStrength;
        } else if (isStroke) {
            ++nbUnlockedStrokes;
            if (isStroke->getBrushType() != eRotoStrokeTypeBlur && isStroke->getBrushType() != eRotoStrokeTypeSharpen) {
                ++nbStrokeWithoutStrength;
            }
            if (isStroke->getBrushType() != eRotoStrokeTypeClone && isStroke->getBrushType() != eRotoStrokeTypeReveal) {
                ++nbStrokeWithoutCloneFunctions;
            }
        }
        
        const std::list<boost::shared_ptr<KnobI> >& drawableKnobs = isDrawable->getKnobs();
        for (std::list<boost::shared_ptr<KnobI> >::const_iterator it = drawableKnobs.begin(); it != drawableKnobs.end(); ++it) {
            
            for (std::list<boost::weak_ptr<KnobI> >::iterator it2 = _imp->knobs.begin(); it2 != _imp->knobs.end(); ++it2) {
                boost::shared_ptr<KnobI> thisKnob = it2->lock();
                if (thisKnob->getName() == (*it)->getName()) {
                    
                    //Clone current state
                    thisKnob->cloneAndUpdateGui(it->get());
                    
                    //Slave internal knobs of the bezier
                    assert((*it)->getDimension() == thisKnob->getDimension());
                    for (int i = 0; i < (*it)->getDimension(); ++i) {
                        (*it)->slaveTo(i, thisKnob, i);
                    }
                    
                    QObject::connect((*it)->getSignalSlotHandler().get(), SIGNAL(keyFrameSet(SequenceTime,int,int,bool)),
                                     this, SLOT(onSelectedKnobCurveChanged()));
                    QObject::connect((*it)->getSignalSlotHandler().get(), SIGNAL(keyFrameRemoved(SequenceTime,int,int)),
                                     this, SLOT(onSelectedKnobCurveChanged()));
                    QObject::connect((*it)->getSignalSlotHandler().get(), SIGNAL(keyFrameMoved(int,int,int)),
                                     this, SLOT(onSelectedKnobCurveChanged()));
                    QObject::connect((*it)->getSignalSlotHandler().get(), SIGNAL(animationRemoved(int)),
                                     this, SLOT(onSelectedKnobCurveChanged()));
                    QObject::connect((*it)->getSignalSlotHandler().get(), SIGNAL(derivativeMoved(SequenceTime,int)),
                                     this, SLOT(onSelectedKnobCurveChanged()));

                    QObject::connect((*it)->getSignalSlotHandler().get(), SIGNAL(keyFrameInterpolationChanged(SequenceTime,int)),
                                     this, SLOT(onSelectedKnobCurveChanged()));

                    break;
                }
            }
            
        }

    } else if (isLayer) {
        const RotoItems & children = isLayer->getItems();
        for (RotoItems::const_iterator it = children.begin(); it != children.end(); ++it) {
            selectInternal(*it);
        }
    }
    
    ///enable the knobs
    if (nbUnlockedBeziers > 0 || nbUnlockedStrokes > 0) {
        
        boost::shared_ptr<KnobI> strengthKnob = _imp->brushEffectKnob.lock();
        boost::shared_ptr<KnobI> sourceTypeKnob = _imp->sourceTypeKnob.lock();
        boost::shared_ptr<KnobI> timeOffsetKnob = _imp->timeOffsetKnob.lock();
        boost::shared_ptr<KnobI> timeOffsetModeKnob = _imp->timeOffsetModeKnob.lock();
        for (std::list<boost::weak_ptr<KnobI> >::iterator it = _imp->knobs.begin(); it != _imp->knobs.end(); ++it) {
            boost::shared_ptr<KnobI> k = it->lock();
            if (!k) {
                continue;
            }
            
            if (k == strengthKnob) {
                if (nbStrokeWithoutStrength) {
                    k->setAllDimensionsEnabled(false);
                } else {
                    k->setAllDimensionsEnabled(true);
                }
            } else {
                
                bool mustDisable = false;
                if (nbStrokeWithoutCloneFunctions) {
                    bool isCloneKnob = false;
                    for (std::list<boost::weak_ptr<KnobI> >::iterator it2 = _imp->cloneKnobs.begin(); it2!=_imp->cloneKnobs.end(); ++it2) {
                        if (it2->lock() == k) {
                            isCloneKnob = true;
                        }
                    }
                    mustDisable |= isCloneKnob;
                }
                if (nbUnlockedBeziers && !mustDisable) {
                    bool isStrokeKnob = false;
                    for (std::list<boost::weak_ptr<KnobI> >::iterator it2 = _imp->strokeKnobs.begin(); it2!=_imp->strokeKnobs.end(); ++it2) {
                        if (it2->lock() == k) {
                            isStrokeKnob = true;
                        }
                    }
                    mustDisable |= isStrokeKnob;
                }
                if (nbUnlockedStrokes && !mustDisable) {
                    bool isBezierKnob = false;
                    for (std::list<boost::weak_ptr<KnobI> >::iterator it2 = _imp->shapeKnobs.begin(); it2!=_imp->shapeKnobs.end(); ++it2) {
                        if (it2->lock() == k) {
                            isBezierKnob = true;
                        }
                    }
                    mustDisable |= isBezierKnob;
                }
                k->setAllDimensionsEnabled(!mustDisable);
                
            }
            if (nbUnlockedBeziers >= 2 || nbUnlockedStrokes >= 2) {
                k->setDirty(true);
            }
        }
        
        //show activated/frame knob according to lifetime
        int lifetime_i = _imp->lifeTime.lock()->getValue();
        _imp->activated.lock()->setSecret(lifetime_i != 3);
        _imp->lifeTimeFrame.lock()->setSecret(lifetime_i == 3);
    }
    
    QMutexLocker l(&_imp->rotoContextMutex);
    _imp->selectedItems.push_back(item);
} // selectInternal

void
RotoContext::onSelectedKnobCurveChanged()
{
    KnobSignalSlotHandler* handler = qobject_cast<KnobSignalSlotHandler*>(sender());
    if (handler) {
        boost::shared_ptr<KnobI> knob = handler->getKnob();
        for (std::list<boost::weak_ptr<KnobI> >::const_iterator it = _imp->knobs.begin(); it != _imp->knobs.end(); ++it) {
            boost::shared_ptr<KnobI> k = it->lock();
            if (k->getName() == knob->getName()) {
                k->clone(knob.get());
                break;
            }
        }
    }
}

void
RotoContext::deselectInternal(boost::shared_ptr<RotoItem> b)
{
    ///only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    int nbBeziersUnLockedBezier = 0;
    int nbStrokesUnlocked = 0;
    {
        QMutexLocker l(&_imp->rotoContextMutex);

        std::list<boost::shared_ptr<RotoItem> >::iterator foundSelected = std::find(_imp->selectedItems.begin(),_imp->selectedItems.end(),b);
        
        ///if the item is not selected, exit
        if ( foundSelected == _imp->selectedItems.end() ) {
            return;
        }
        
        _imp->selectedItems.erase(foundSelected);
        
        for (std::list<boost::shared_ptr<RotoItem> >::iterator it = _imp->selectedItems.begin(); it != _imp->selectedItems.end(); ++it) {
            Bezier* isBezier = dynamic_cast<Bezier*>(it->get());
            RotoStrokeItem* isStroke = dynamic_cast<RotoStrokeItem*>(it->get());
            if (!isStroke && isBezier && !isBezier->isLockedRecursive()) {
                ++nbBeziersUnLockedBezier;
            } else if (isStroke) {
                ++nbStrokesUnlocked;
            }
        }
    }
    

    
    bool bezierDirty = nbBeziersUnLockedBezier > 1;
    bool strokeDirty = nbStrokesUnlocked > 1;
    boost::shared_ptr<Bezier> isBezier = boost::dynamic_pointer_cast<Bezier>(b);
    RotoDrawableItem* isDrawable = dynamic_cast<RotoDrawableItem*>(b.get());
    boost::shared_ptr<RotoStrokeItem> isStroke = boost::dynamic_pointer_cast<RotoStrokeItem>(b);
    boost::shared_ptr<RotoLayer> isLayer = boost::dynamic_pointer_cast<RotoLayer>(b);
    if (isDrawable) {
        ///first-off set the context knobs to the value of this bezier
        
        const std::list<boost::shared_ptr<KnobI> >& drawableKnobs = isDrawable->getKnobs();
        for (std::list<boost::shared_ptr<KnobI> >::const_iterator it = drawableKnobs.begin(); it != drawableKnobs.end(); ++it) {
            
            for (std::list<boost::weak_ptr<KnobI> >::iterator it2 = _imp->knobs.begin(); it2 != _imp->knobs.end(); ++it2) {
                boost::shared_ptr<KnobI> knob = it2->lock();
                if (knob->getName() == (*it)->getName()) {
                    
                    //Clone current state
                    knob->cloneAndUpdateGui(it->get());
                    
                    //Slave internal knobs of the bezier
                    assert((*it)->getDimension() == knob->getDimension());
                    for (int i = 0; i < (*it)->getDimension(); ++i) {
                        (*it)->unSlave(i,isBezier ? !bezierDirty : !strokeDirty);
                    }
                    
                    QObject::disconnect((*it)->getSignalSlotHandler().get(), SIGNAL(keyFrameSet(SequenceTime,int,int,bool)),
                                     this, SLOT(onSelectedKnobCurveChanged()));
                    QObject::disconnect((*it)->getSignalSlotHandler().get(), SIGNAL(keyFrameRemoved(SequenceTime,int,int)),
                                     this, SLOT(onSelectedKnobCurveChanged()));
                    QObject::disconnect((*it)->getSignalSlotHandler().get(), SIGNAL(keyFrameMoved(int,int,int)),
                                     this, SLOT(onSelectedKnobCurveChanged()));
                    QObject::disconnect((*it)->getSignalSlotHandler().get(), SIGNAL(animationRemoved(int)),
                                     this, SLOT(onSelectedKnobCurveChanged()));
                    QObject::disconnect((*it)->getSignalSlotHandler().get(), SIGNAL(derivativeMoved(SequenceTime,int)),
                                     this, SLOT(onSelectedKnobCurveChanged()));
                    
                    QObject::disconnect((*it)->getSignalSlotHandler().get(), SIGNAL(keyFrameInterpolationChanged(SequenceTime,int)),
                                        this, SLOT(onSelectedKnobCurveChanged()));
                    break;
                }
            }
            
        }
        
        
    } else if (isLayer) {
        const RotoItems & children = isLayer->getItems();
        for (RotoItems::const_iterator it = children.begin(); it != children.end(); ++it) {
            deselectInternal(*it);
        }
    }
    
    
    
    ///if the selected beziers count reaches 0 notify the gui knobs so they appear not enabled
    
    if (nbBeziersUnLockedBezier == 0 || nbStrokesUnlocked == 0) {
        for (std::list<boost::weak_ptr<KnobI> >::iterator it = _imp->knobs.begin(); it != _imp->knobs.end(); ++it) {
            boost::shared_ptr<KnobI> k = it->lock();
            if (!k) {
                continue;
            }
            k->setAllDimensionsEnabled(false);
            if (!bezierDirty || !strokeDirty) {
                k->setDirty(false);
            }
        }
    }
    
} // deselectInternal


boost::shared_ptr<RotoItem>
RotoContext::getLastItemLocked() const
{
    QMutexLocker l(&_imp->rotoContextMutex);

    return _imp->lastLockedItem;
}

static void
addOrRemoveKeyRecursively(const boost::shared_ptr<RotoLayer>& isLayer,
                          double time,
                          bool add,
                          bool removeAll)
{
    const RotoItems & items = isLayer->getItems();

    for (RotoItems::const_iterator it2 = items.begin(); it2 != items.end(); ++it2) {
        boost::shared_ptr<RotoLayer> layer = boost::dynamic_pointer_cast<RotoLayer>(*it2);
        boost::shared_ptr<Bezier> isBezier = boost::dynamic_pointer_cast<Bezier>(*it2);
        if (isBezier) {
            if (add) {
                isBezier->setKeyframe(time);
            } else {
                if (!removeAll) {
                    isBezier->removeKeyframe(time);
                } else {
                    isBezier->removeAnimation();
                }
            }
        } else if (layer) {
            addOrRemoveKeyRecursively(layer, time, add,removeAll);
        }
    }
}

void
RotoContext::setKeyframeOnSelectedCurves()
{
    ///only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );

    double time = getTimelineCurrentTime();
    QMutexLocker l(&_imp->rotoContextMutex);
    for (std::list<boost::shared_ptr<RotoItem> >::iterator it = _imp->selectedItems.begin(); it != _imp->selectedItems.end(); ++it) {
        boost::shared_ptr<RotoLayer> isLayer = boost::dynamic_pointer_cast<RotoLayer>(*it);
        boost::shared_ptr<Bezier> isBezier = boost::dynamic_pointer_cast<Bezier>(*it);
        if (isBezier) {
            isBezier->setKeyframe(time);
        } else if (isLayer) {
            addOrRemoveKeyRecursively(isLayer,time, true, false);
        }
    }
}

void
RotoContext::removeAnimationOnSelectedCurves()
{
    ///only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    
    double time = getTimelineCurrentTime();
    for (std::list<boost::shared_ptr<RotoItem> >::iterator it = _imp->selectedItems.begin(); it != _imp->selectedItems.end(); ++it) {
        boost::shared_ptr<RotoLayer> isLayer = boost::dynamic_pointer_cast<RotoLayer>(*it);
        boost::shared_ptr<Bezier> isBezier = boost::dynamic_pointer_cast<Bezier>(*it);
        if (isBezier) {
            isBezier->removeAnimation();
        } else if (isLayer) {
            addOrRemoveKeyRecursively(isLayer,time, false, true);
        }
    }
    if (!_imp->selectedItems.empty()) {
        evaluateChange();
    }
}

void
RotoContext::removeKeyframeOnSelectedCurves()
{
    ///only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );

    double time = getTimelineCurrentTime();
    for (std::list<boost::shared_ptr<RotoItem> >::iterator it = _imp->selectedItems.begin(); it != _imp->selectedItems.end(); ++it) {
        boost::shared_ptr<RotoLayer> isLayer = boost::dynamic_pointer_cast<RotoLayer>(*it);
        boost::shared_ptr<Bezier> isBezier = boost::dynamic_pointer_cast<Bezier>(*it);
        if (isBezier) {
            isBezier->removeKeyframe(time);
        } else if (isLayer) {
            addOrRemoveKeyRecursively(isLayer,time, false, false);
        }
    }
}

static void
findOutNearestKeyframeRecursively(const boost::shared_ptr<RotoLayer>& layer,
                                  bool previous,
                                  double time,
                                  int* nearest)
{
    const RotoItems & items = layer->getItems();

    for (RotoItems::const_iterator it = items.begin(); it != items.end(); ++it) {
        boost::shared_ptr<RotoLayer> layer = boost::dynamic_pointer_cast<RotoLayer>(*it);
        boost::shared_ptr<Bezier> isBezier = boost::dynamic_pointer_cast<Bezier>(*it);
        if (isBezier) {
            if (previous) {
                int t = isBezier->getPreviousKeyframeTime(time);
                if ( (t != INT_MIN) && (t > *nearest) ) {
                    *nearest = t;
                }
            } else if (layer) {
                int t = isBezier->getNextKeyframeTime(time);
                if ( (t != INT_MAX) && (t < *nearest) ) {
                    *nearest = t;
                }
            }
        } else {
            assert(layer);
            if (layer) {
                findOutNearestKeyframeRecursively(layer, previous, time, nearest);
            }
        }
    }
}

void
RotoContext::goToPreviousKeyframe()
{
    ///only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );

    double time = getTimelineCurrentTime();
    int minimum = INT_MIN;

    {
        QMutexLocker l(&_imp->rotoContextMutex);
        for (std::list<boost::shared_ptr<RotoItem> >::iterator it = _imp->selectedItems.begin(); it != _imp->selectedItems.end(); ++it) {
            boost::shared_ptr<RotoLayer> layer = boost::dynamic_pointer_cast<RotoLayer>(*it);
            boost::shared_ptr<Bezier> isBezier = boost::dynamic_pointer_cast<Bezier>(*it);
            if (isBezier) {
                int t = isBezier->getPreviousKeyframeTime(time);
                if ( (t != INT_MIN) && (t > minimum) ) {
                    minimum = t;
                }
            } else {
                assert(layer);
                if (layer) {
                    findOutNearestKeyframeRecursively(layer, true,time,&minimum);
                }
            }
        }
    }

    if (minimum != INT_MIN) {
        getNode()->getApp()->setLastViewerUsingTimeline(boost::shared_ptr<Natron::Node>());
        getNode()->getApp()->getTimeLine()->seekFrame(minimum, false,  NULL, Natron::eTimelineChangeReasonPlaybackSeek);
    }
}

void
RotoContext::goToNextKeyframe()
{
    ///only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );

    double time = getTimelineCurrentTime();
    int maximum = INT_MAX;

    {
        QMutexLocker l(&_imp->rotoContextMutex);
        for (std::list<boost::shared_ptr<RotoItem> >::iterator it = _imp->selectedItems.begin(); it != _imp->selectedItems.end(); ++it) {
            boost::shared_ptr<RotoLayer> isLayer = boost::dynamic_pointer_cast<RotoLayer>(*it);
            boost::shared_ptr<Bezier> isBezier = boost::dynamic_pointer_cast<Bezier>(*it);
            if (isBezier) {
                int t = isBezier->getNextKeyframeTime(time);
                if ( (t != INT_MAX) && (t < maximum) ) {
                    maximum = t;
                }
            } else {
                assert(isLayer);
                if (isLayer) {
                    findOutNearestKeyframeRecursively(isLayer, false, time, &maximum);
                }
            }
        }
    }
    if (maximum != INT_MAX) {
        getNode()->getApp()->setLastViewerUsingTimeline(boost::shared_ptr<Natron::Node>());
        getNode()->getApp()->getTimeLine()->seekFrame(maximum, false, NULL,Natron::eTimelineChangeReasonPlaybackSeek);
        
    }
}

static void
appendToSelectedCurvesRecursively(std::list< boost::shared_ptr<RotoDrawableItem> > * curves,
                                  const boost::shared_ptr<RotoLayer>& isLayer,
                                  double time,
                                  bool onlyActives,
                                  bool addStrokes)
{
    RotoItems items = isLayer->getItems_mt_safe();

    for (RotoItems::const_iterator it = items.begin(); it != items.end(); ++it) {
        boost::shared_ptr<RotoLayer> layer = boost::dynamic_pointer_cast<RotoLayer>(*it);
        boost::shared_ptr<RotoDrawableItem> isDrawable = boost::dynamic_pointer_cast<RotoDrawableItem>(*it);
        RotoStrokeItem* isStroke = dynamic_cast<RotoStrokeItem*>(isDrawable.get());
        if (isStroke && !addStrokes) {
            continue;
        }
        if (isDrawable) {
            if ( !onlyActives || isDrawable->isActivated(time) ) {
                curves->push_front(isDrawable);
            }
        } else if ( layer && layer->isGloballyActivated() ) {
            appendToSelectedCurvesRecursively(curves, layer, time, onlyActives ,addStrokes);
        }
    }
}

const std::list< boost::shared_ptr<RotoItem> > &
RotoContext::getSelectedItems() const
{
    ///only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    QMutexLocker l(&_imp->rotoContextMutex);

    return _imp->selectedItems;
}

std::list< boost::shared_ptr<RotoDrawableItem> >
RotoContext::getSelectedCurves() const
{
    ///only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    std::list< boost::shared_ptr<RotoDrawableItem> > drawables;
    double time = getTimelineCurrentTime();
    {
        QMutexLocker l(&_imp->rotoContextMutex);
        for (std::list<boost::shared_ptr<RotoItem> >::iterator it = _imp->selectedItems.begin(); it != _imp->selectedItems.end(); ++it) {
            assert(*it);
            boost::shared_ptr<RotoLayer> isLayer = boost::dynamic_pointer_cast<RotoLayer>(*it);
            boost::shared_ptr<RotoDrawableItem> isDrawable = boost::dynamic_pointer_cast<RotoDrawableItem>(*it);
            if (isDrawable) {
                drawables.push_back(isDrawable);
            } else {
                assert(isLayer);
                if (isLayer) {
                    appendToSelectedCurvesRecursively(&drawables, isLayer,time,false, true);
                }
            }
        }
    }
    return drawables;
}

std::list< boost::shared_ptr<RotoDrawableItem> >
RotoContext::getCurvesByRenderOrder(bool onlyActivated) const
{
    std::list< boost::shared_ptr<RotoDrawableItem> > ret;
    
    ///Note this might not be the timeline's current frame if this is a render thread.
    double time = getNode()->getLiveInstance()->getThreadLocalRenderTime();
    {
        QMutexLocker l(&_imp->rotoContextMutex);
        if ( !_imp->layers.empty() ) {
            appendToSelectedCurvesRecursively(&ret, _imp->layers.front(), time, onlyActivated, true);
        }
    }

    return ret;
}

int
RotoContext::getNCurves() const
{
    std::list< boost::shared_ptr<RotoDrawableItem> > curves = getCurvesByRenderOrder();
    return (int)curves.size();
}

boost::shared_ptr<RotoLayer>
RotoContext::getLayerByName(const std::string & n) const
{
    QMutexLocker l(&_imp->rotoContextMutex);

    for (std::list<boost::shared_ptr<RotoLayer> >::const_iterator it = _imp->layers.begin(); it != _imp->layers.end(); ++it) {
        if ( (*it)->getScriptName() == n ) {
            return *it;
        }
    }

    return boost::shared_ptr<RotoLayer>();
}

static void
findItemRecursively(const std::string & n,
                    const boost::shared_ptr<RotoLayer> & layer,
                    boost::shared_ptr<RotoItem>* ret)
{
    if (layer->getScriptName() == n) {
        *ret = boost::dynamic_pointer_cast<RotoItem>(layer);
    } else {
        const RotoItems & items = layer->getItems();
        for (RotoItems::const_iterator it2 = items.begin(); it2 != items.end(); ++it2) {
            boost::shared_ptr<RotoLayer> isLayer = boost::dynamic_pointer_cast<RotoLayer>(*it2);
            if ( (*it2)->getScriptName() == n ) {
                *ret = *it2;

                return;
            } else if (isLayer) {
                findItemRecursively(n, isLayer, ret);
            }
        }
    }
}

boost::shared_ptr<RotoItem>
RotoContext::getItemByName(const std::string & n) const
{
    boost::shared_ptr<RotoItem> ret;
    QMutexLocker l(&_imp->rotoContextMutex);

    for (std::list<boost::shared_ptr<RotoLayer> >::const_iterator it = _imp->layers.begin(); it != _imp->layers.end(); ++it) {
        findItemRecursively(n, *it, &ret);
    }

    return ret;
}

boost::shared_ptr<RotoLayer>
RotoContext::getDeepestSelectedLayer() const
{
    QMutexLocker l(&_imp->rotoContextMutex);
    return findDeepestSelectedLayer();
}

boost::shared_ptr<RotoLayer>
RotoContext::findDeepestSelectedLayer() const
{
    QMutexLocker k(&_imp->rotoContextMutex);
    return _imp->findDeepestSelectedLayer();
}

void
RotoContext::incrementAge()
{
    _imp->incrementRotoAge();
}

void
RotoContext::evaluateChange()
{
    _imp->incrementRotoAge();
    _imp->node.lock()->getLiveInstance()->abortAnyEvaluation();
    getNode()->getLiveInstance()->evaluate_public(NULL, true,Natron::eValueChangedReasonUserEdited);
}

void
RotoContext::evaluateChange_noIncrement()
{
    //Used for the rotopaint to optimize the portion to render (the last tick of the mouse move)
    std::list<Node*> outputs;
    for (std::list<Node*>::iterator it = outputs.begin(); it!=outputs.end(); ++it) {
        (*it)->incrementKnobsAge();
    }
    
    std::list<ViewerInstance* > viewers;
    getNode()->hasViewersConnected(&viewers);
    for (std::list<ViewerInstance* >::iterator it = viewers.begin();
         it != viewers.end();
         ++it) {
        (*it)->renderCurrentFrame(true);
    }
}


void
RotoContext::clearViewersLastRenderedStrokes()
{
    std::list<ViewerInstance* > viewers;
    getNode()->hasViewersConnected(&viewers);
    for (std::list<ViewerInstance* >::iterator it = viewers.begin();
         it != viewers.end();
         ++it) {
        (*it)->clearLastRenderedImage();
    }
}

U64
RotoContext::getAge()
{
    QMutexLocker l(&_imp->rotoContextMutex);

    return _imp->age;
}

void
RotoContext::onItemLockedChanged(const boost::shared_ptr<RotoItem>& item, RotoItem::SelectionReasonEnum reason)
{
    assert(item);
    ///refresh knobs
    int nbBeziersUnLockedBezier = 0;

    {
        QMutexLocker l(&_imp->rotoContextMutex);

        for (std::list<boost::shared_ptr<RotoItem> >::iterator it = _imp->selectedItems.begin(); it != _imp->selectedItems.end(); ++it) {
            boost::shared_ptr<Bezier> isBezier = boost::dynamic_pointer_cast<Bezier>(*it);
            if ( isBezier && !isBezier->isLockedRecursive() ) {
                ++nbBeziersUnLockedBezier;
            }
        }
    }
    bool dirty = nbBeziersUnLockedBezier > 1;
    bool enabled = nbBeziersUnLockedBezier > 0;

    for (std::list<boost::weak_ptr<KnobI> >::iterator it = _imp->knobs.begin(); it!=_imp->knobs.end(); ++it) {
        boost::shared_ptr<KnobI> knob = it->lock();
        if (!knob) {
            continue;
        }
        knob->setDirty(dirty);
        knob->setAllDimensionsEnabled(enabled);
    }
    _imp->lastLockedItem = item;
    Q_EMIT itemLockedChanged((int)reason);
}

void
RotoContext::onItemScriptNameChanged(const boost::shared_ptr<RotoItem>& item)
{
    Q_EMIT itemScriptNameChanged(item);
}

void
RotoContext::onItemLabelChanged(const boost::shared_ptr<RotoItem>& item)
{
    Q_EMIT itemLabelChanged(item);
}

void
RotoContext::onItemKnobChanged()
{
    emitRefreshViewerOverlays();
    evaluateChange();
}

std::string
RotoContext::getRotoNodeName() const
{
    return getNode()->getScriptName_mt_safe();
}

void
RotoContext::emitRefreshViewerOverlays()
{
    Q_EMIT refreshViewerOverlays();
}

void
RotoContext::getBeziersKeyframeTimes(std::list<int> *times) const
{
    std::list< boost::shared_ptr<RotoDrawableItem> > splines = getCurvesByRenderOrder();

    for (std::list< boost::shared_ptr<RotoDrawableItem> > ::iterator it = splines.begin(); it != splines.end(); ++it) {
        std::set<int> splineKeys;
        Bezier* isBezier = dynamic_cast<Bezier*>(it->get());
        if (!isBezier) {
            continue;
        }
        isBezier->getKeyframeTimes(&splineKeys);
        for (std::set<int>::iterator it2 = splineKeys.begin(); it2 != splineKeys.end(); ++it2) {
            times->push_back(*it2);
        }
    }
}

static void dequeueActionForLayer(RotoLayer* layer, bool *evaluate)
{
    RotoItems items = layer->getItems_mt_safe();

    for (RotoItems::iterator it = items.begin(); it!=items.end(); ++it) {
        Bezier* isBezier = dynamic_cast<Bezier*>(it->get());
        RotoLayer* isLayer = dynamic_cast<RotoLayer*>(it->get());
        if (isBezier) {
            *evaluate = *evaluate | isBezier->dequeueGuiActions();
        } else if (isLayer) {
            dequeueActionForLayer(isLayer,evaluate);
        }
        
    }
}

void
RotoContext::dequeueGuiActions()
{
    bool evaluate = false;
    {
        QMutexLocker l(&_imp->rotoContextMutex);
        if (!_imp->layers.empty()) {
            dequeueActionForLayer(_imp->layers.front().get(),&evaluate);
        }
    }
    if (evaluate) {
        evaluateChange();
    }
    
}

bool
RotoContext::isDoingNeatRender() const
{
    QMutexLocker k(&_imp->doingNeatRenderMutex);
    return _imp->doingNeatRender;
}

void
RotoContext::notifyRenderFinished()
{
    setIsDoingNeatRender(false);
}

void
RotoContext::setIsDoingNeatRender(bool doing)
{
    
    QMutexLocker k(&_imp->doingNeatRenderMutex);
    _imp->doingNeatRender = doing;
    if (doing && _imp->mustDoNeatRender) {
        _imp->mustDoNeatRender = false;
    }
    if (!doing) {
        _imp->doingNeatRenderCond.wakeAll();
    }
}

void
RotoContext::evaluateNeatStrokeRender()
{
    {
        QMutexLocker k(&_imp->doingNeatRenderMutex);
        _imp->mustDoNeatRender = true;
    }
    evaluateChange();
    
    //EvaluateChange will call setIsDoingNeatRender(true);
    {
        QMutexLocker k(&_imp->doingNeatRenderMutex);
        while (_imp->doingNeatRender) {
            _imp->doingNeatRenderCond.wait(&_imp->doingNeatRenderMutex);
        }
    }
}

static void
adjustToPointToScale(unsigned int mipmapLevel,
                     double &x,
                     double &y)
{
    if (mipmapLevel != 0) {
        int pot = (1 << mipmapLevel);
        x /= pot;
        y /= pot;
    }
}

template <typename PIX,int maxValue, int dstNComps, int srcNComps, bool useOpacity>
static void
convertCairoImageToNatronImageForDstComponents_noColor(cairo_surface_t* cairoImg,
                                                       Natron::Image* image,
                                                       const RectI & pixelRod,
                                                       double shapeColor[3],
                                                       double opacity)
{
    
    unsigned char* cdata = cairo_image_surface_get_data(cairoImg);
    unsigned char* srcPix = cdata;
    int stride = cairo_image_surface_get_stride(cairoImg);
    
    Natron::Image::WriteAccess acc = image->getWriteRights();
    
    double r = useOpacity ? shapeColor[0] * opacity : shapeColor[0];
    double g = useOpacity ? shapeColor[1] * opacity : shapeColor[1];
    double b = useOpacity ? shapeColor[2] * opacity : shapeColor[2];
    
    for (int y = 0; y < pixelRod.height(); ++y, srcPix += stride) {
        PIX* dstPix = (PIX*)acc.pixelAt(pixelRod.x1, pixelRod.y1 + y);
        assert(dstPix);
        
        for (int x = 0; x < pixelRod.width(); ++x) {
            float cairoPixel = (float)srcPix[x * srcNComps] / 255.f;
            switch (dstNComps) {
                case 4:
                    dstPix[x * dstNComps + 0] = PIX(cairoPixel * r * maxValue);
                    assert(!boost::math::isnan(dstPix[x * dstNComps + 0]));
                    dstPix[x * dstNComps + 1] = PIX(cairoPixel * g * maxValue);
                    assert(!boost::math::isnan(dstPix[x * dstNComps + 1]));
                    dstPix[x * dstNComps + 2] = PIX(cairoPixel * b * maxValue);
                    assert(!boost::math::isnan(dstPix[x * dstNComps + 2]));
                    dstPix[x * dstNComps + 3] = useOpacity ? PIX(cairoPixel * opacity * maxValue) : PIX(cairoPixel * maxValue);
                    assert(!boost::math::isnan(dstPix[x * dstNComps + 3]));
                    break;
                case 1:
                    dstPix[x] = useOpacity ? PIX(cairoPixel * opacity * maxValue) : PIX(cairoPixel * maxValue);
                    assert(!boost::math::isnan(dstPix[x]));
                    break;
                case 3:
                    dstPix[x * dstNComps + 0] = PIX(cairoPixel * r * maxValue);
                    dstPix[x * dstNComps + 1] = PIX(cairoPixel * g * maxValue);
                    dstPix[x * dstNComps + 2] = PIX(cairoPixel * b * maxValue);
                    break;
                case 2:
                    dstPix[x * dstNComps + 0] = PIX(cairoPixel * r * maxValue);
                    dstPix[x * dstNComps + 1] = PIX(cairoPixel * g * maxValue);
                    break;

                default:
                    break;
            }
#         ifdef DEBUG
            for (int c = 0; c < dstNComps; ++c) {
                assert(dstPix[x * dstNComps + c] == dstPix[x * dstNComps + c]); // check for NaN
            }
#         endif
        }
    }

}

template <typename PIX,int maxValue, int dstNComps, int srcNComps>
static void
convertCairoImageToNatronImageForOpacity(cairo_surface_t* cairoImg,
                                         Natron::Image* image,
                                         const RectI & pixelRod,
                                         double shapeColor[3],
                                         double opacity,
                                         bool useOpacity)
{
    if (useOpacity) {
        convertCairoImageToNatronImageForDstComponents_noColor<PIX,maxValue,dstNComps, srcNComps, true>(cairoImg, image,pixelRod, shapeColor, opacity);
    } else {
        convertCairoImageToNatronImageForDstComponents_noColor<PIX,maxValue,dstNComps, srcNComps, false>(cairoImg, image,pixelRod, shapeColor, opacity);
    }

}


template <typename PIX,int maxValue, int dstNComps>
static void
convertCairoImageToNatronImageForSrcComponents_noColor(cairo_surface_t* cairoImg,
                                                       int srcNComps,
                                                       Natron::Image* image,
                                                       const RectI & pixelRod,
                                                       double shapeColor[3],
                                                       double opacity,
                                                       bool useOpacity)
{
    if (srcNComps == 1) {
        convertCairoImageToNatronImageForOpacity<PIX,maxValue,dstNComps, 1>(cairoImg, image,pixelRod, shapeColor, opacity, useOpacity);
    } else if (srcNComps == 4) {
        convertCairoImageToNatronImageForOpacity<PIX,maxValue,dstNComps, 4>(cairoImg, image,pixelRod, shapeColor, opacity, useOpacity);
    } else {
        assert(false);
    }
}

template <typename PIX,int maxValue>
static void
convertCairoImageToNatronImage_noColor(cairo_surface_t* cairoImg,
                                       int srcNComps,
                                       Natron::Image* image,
                                       const RectI & pixelRod,
                                       double shapeColor[3],
                                       double opacity,
                                       bool useOpacity)
{
    int comps = (int)image->getComponentsCount();
    switch (comps) {
        case 1:
            convertCairoImageToNatronImageForSrcComponents_noColor<PIX,maxValue,1>(cairoImg, srcNComps, image,pixelRod, shapeColor, opacity, useOpacity);
            break;
        case 2:
            convertCairoImageToNatronImageForSrcComponents_noColor<PIX,maxValue,2>(cairoImg, srcNComps, image,pixelRod, shapeColor, opacity, useOpacity);
            break;
        case 3:
            convertCairoImageToNatronImageForSrcComponents_noColor<PIX,maxValue,3>(cairoImg, srcNComps, image,pixelRod, shapeColor, opacity, useOpacity);
            break;
        case 4:
            convertCairoImageToNatronImageForSrcComponents_noColor<PIX,maxValue,4>(cairoImg, srcNComps, image,pixelRod, shapeColor, opacity, useOpacity);
            break;
        default:
            break;
    }
}

template <typename PIX,int maxValue, int srcNComps, int dstNComps>
static void
convertCairoImageToNatronImageForDstComponents(cairo_surface_t* cairoImg,
                                                       Natron::Image* image,
                                                       const RectI & pixelRod)
{
    
    unsigned char* cdata = cairo_image_surface_get_data(cairoImg);
    unsigned char* srcPix = cdata;
    int stride = cairo_image_surface_get_stride(cairoImg);
    int pixelSize = stride / pixelRod.width();
    
    Natron::Image::WriteAccess acc = image->getWriteRights();
    
    for (int y = 0; y < pixelRod.height(); ++y, srcPix += stride) {
        PIX* dstPix = (PIX*)acc.pixelAt(pixelRod.x1, pixelRod.y1 + y);
        assert(dstPix);
        
        for (int x = 0; x < pixelRod.width(); ++x) {
            switch (dstNComps) {
                case 4:
                    assert(srcNComps == dstNComps);
                    // cairo's format is ARGB (that is BGRA when interpreted as bytes)
                    dstPix[x * dstNComps + 3] = PIX( (float)srcPix[x * pixelSize + 3] / 255.f ) * maxValue;
                    dstPix[x * dstNComps + 0] = PIX( (float)srcPix[x * pixelSize + 2] / 255.f ) * maxValue;
                    dstPix[x * dstNComps + 1] = PIX( (float)srcPix[x * pixelSize + 1] / 255.f ) * maxValue;
                    dstPix[x * dstNComps + 2] = PIX( (float)srcPix[x * pixelSize + 0] / 255.f ) * maxValue;
                    break;
                case 1:
                    assert(srcNComps == dstNComps);
                    dstPix[x] = PIX( (float)srcPix[x] / 255.f ) * maxValue;
                    break;
                case 3:
                    assert(srcNComps == dstNComps);
                    dstPix[x * dstNComps + 0] = PIX( (float)srcPix[x * pixelSize + 2] / 255.f ) * maxValue;
                    dstPix[x * dstNComps + 1] = PIX( (float)srcPix[x * pixelSize + 1] / 255.f ) * maxValue;
                    dstPix[x * dstNComps + 2] = PIX( (float)srcPix[x * pixelSize + 0] / 255.f ) * maxValue;
                    break;
                case 2:
                    assert(srcNComps == 3);
                    dstPix[x * dstNComps + 0] = PIX( (float)srcPix[x * pixelSize + 2] / 255.f ) * maxValue;
                    dstPix[x * dstNComps + 1] = PIX( (float)srcPix[x * pixelSize + 1] / 255.f ) * maxValue;
                    break;
                default:
                    break;
            }
#         ifdef DEBUG
            for (int c = 0; c < dstNComps; ++c) {
                assert(dstPix[x * dstNComps + c] == dstPix[x * dstNComps + c]); // check for NaN
            }
#         endif
        }
    }
    
}

template <typename PIX,int maxValue, int srcNComps>
static void
convertCairoImageToNatronImageForSrcComponents(cairo_surface_t* cairoImg,
                                               Natron::Image* image,
                                               const RectI & pixelRod)
{
    

    int comps = (int)image->getComponentsCount();
    switch (comps) {
        case 1:
            convertCairoImageToNatronImageForDstComponents<PIX,maxValue, srcNComps, 1>(cairoImg,image,pixelRod);
            break;
        case 2:
            convertCairoImageToNatronImageForDstComponents<PIX,maxValue, srcNComps, 2>(cairoImg,image,pixelRod);
            break;
        case 3:
            convertCairoImageToNatronImageForDstComponents<PIX,maxValue, srcNComps, 3>(cairoImg,image,pixelRod);
            break;
        case 4:
            convertCairoImageToNatronImageForDstComponents<PIX,maxValue, srcNComps, 4>(cairoImg,image,pixelRod);
            break;
        default:
            break;
    }
}


template <typename PIX,int maxValue>
static void
convertCairoImageToNatronImage(cairo_surface_t* cairoImg,
                               Natron::Image* image,
                               const RectI & pixelRod,
                               int srcNComps)
{
    switch (srcNComps) {
        case 1:
            convertCairoImageToNatronImageForSrcComponents<PIX,maxValue,1>(cairoImg,image,pixelRod);
            break;
        case 2:
            convertCairoImageToNatronImageForSrcComponents<PIX,maxValue,2>(cairoImg,image,pixelRod);
            break;
        case 3:
            convertCairoImageToNatronImageForSrcComponents<PIX,maxValue,3>(cairoImg,image,pixelRod);
            break;
        case 4:
            convertCairoImageToNatronImageForSrcComponents<PIX,maxValue,4>(cairoImg,image,pixelRod);
            break;
        default:
            break;
    }
}

template <typename PIX,int maxValue, int srcNComps, int dstNComps>
static void
convertNatronImageToCairoImageForComponents(unsigned char* cairoImg,
                                            std::size_t stride,
                                            Natron::Image* image,
                                            const RectI& roi,
                                            const RectI& dstBounds,
                                            double shapeColor[3])
{
    unsigned char* dstPix = cairoImg;
    dstPix += ((roi.y1 - dstBounds.y1) * stride + (roi.x1 - dstBounds.x1));
    
    Natron::Image::ReadAccess acc = image->getReadRights();
    
    for (int y = 0; y < roi.height(); ++y, dstPix += stride) {
        
        const PIX* srcPix = (const PIX*)acc.pixelAt(roi.x1, roi.y1 + y);
        assert(srcPix);
        
        for (int x = 0; x < roi.width(); ++x) {
#         ifdef DEBUG
            for (int c = 0; c < srcNComps; ++c) {
                assert(srcPix[x * srcNComps + c] == srcPix[x * srcNComps + c]); // check for NaN
            }
#         endif
            if (dstNComps == 1) {
                dstPix[x] = (float)srcPix[x * srcNComps] / maxValue * 255.f;
            } else if (dstNComps == 4) {
                if (srcNComps == 4) {
                    //We are in the !buildUp case, do exactly the opposite that is done in convertNatronImageToCairoImageForComponents
                    dstPix[x * dstNComps + 0] = (float)(srcPix[x * srcNComps + 2] / maxValue) / shapeColor[2] * 255.f;
                    dstPix[x * dstNComps + 1] = (float)(srcPix[x * srcNComps + 1] / maxValue) / shapeColor[1] * 255.f;
                    dstPix[x * dstNComps + 2] = (float)(srcPix[x * srcNComps + 0] / maxValue) / shapeColor[0] * 255.f;
                    dstPix[x * dstNComps + 3] = 255;//(float)srcPix[x * srcNComps + 3] / maxValue * 255.f;
                } else {
                    assert(srcNComps == 1);
                    float pix = (float)srcPix[x];
                    dstPix[x * dstNComps + 0] = pix / maxValue * 255.f;
                    dstPix[x * dstNComps + 1] = pix / maxValue * 255.f;
                    dstPix[x * dstNComps + 2] = pix / maxValue * 255.f;
                    dstPix[x * dstNComps + 3] = pix / maxValue * 255.f;
                }
            }
            // no need to check for NaN, dstPix is unsigned char
        }
    }

}

template <typename PIX,int maxValue, int srcComps>
static void
convertNatronImageToCairoImageForSrcComponents(unsigned char* cairoImg,
                                               int dstNComps,
                                               std::size_t stride,
                                               Natron::Image* image,
                                               const RectI& roi,
                                               const RectI& dstBounds,
                                               double shapeColor[3])
{
    if (dstNComps == 1) {
        convertNatronImageToCairoImageForComponents<PIX,maxValue, srcComps, 1>(cairoImg, stride, image, roi, dstBounds,shapeColor);
    } else if (dstNComps == 4) {
        convertNatronImageToCairoImageForComponents<PIX,maxValue, srcComps, 4>(cairoImg, stride, image, roi, dstBounds,shapeColor);
    } else {
        assert(false);
    }
}

template <typename PIX,int maxValue>
static void
convertNatronImageToCairoImage(unsigned char* cairoImg,
                               int dstNComps,
                               std::size_t stride,
                               Natron::Image* image,
                               const RectI& roi,
                               const RectI& dstBounds,
                               double shapeColor[3])
{
    int numComps = (int)image->getComponentsCount();
    switch (numComps) {
        case 1:
            convertNatronImageToCairoImageForSrcComponents<PIX,maxValue, 1>(cairoImg, dstNComps, stride, image, roi, dstBounds,shapeColor);
            break;
        case 2:
            convertNatronImageToCairoImageForSrcComponents<PIX,maxValue, 2>(cairoImg, dstNComps,stride, image, roi, dstBounds,shapeColor);
            break;
        case 3:
            convertNatronImageToCairoImageForSrcComponents<PIX,maxValue, 3>(cairoImg, dstNComps,stride, image, roi, dstBounds,shapeColor);
            break;
        case 4:
            convertNatronImageToCairoImageForSrcComponents<PIX,maxValue, 4>(cairoImg, dstNComps,stride, image, roi, dstBounds,shapeColor);
            break;
        default:
            break;
    }
}

double
RotoContext::renderSingleStroke(const boost::shared_ptr<RotoStrokeItem>& stroke,
                                const RectD& pointsBbox,
                                const std::list<std::pair<Natron::Point,double> >& points,
                                unsigned int mipmapLevel,
                                double par,
                                const Natron::ImageComponents& components,
                                Natron::ImageBitDepthEnum depth,
                                double distToNext,
                                boost::shared_ptr<Natron::Image> *image)
{
    
    double time = getTimelineCurrentTime();

    double shapeColor[3];
    stroke->getColor(time, shapeColor);
    
    boost::shared_ptr<Natron::Image> source = *image;
    RectI pixelPointsBbox;
    pointsBbox.toPixelEnclosing(mipmapLevel, par, &pixelPointsBbox);
    
    bool copyFromImage = false;
    bool mipMapLevelChanged = false;
    if (!source) {
        source.reset(new Natron::Image(components,
                                    pointsBbox,
                                    pixelPointsBbox,
                                    mipmapLevel,
                                    par,
                                    depth, false));
        *image = source;
    } else {
        
        if ((*image)->getMipMapLevel() > mipmapLevel) {
            
            mipMapLevelChanged = true;
            
            RectD otherRoD = (*image)->getRoD();
            RectI oldBounds;
            otherRoD.toPixelEnclosing((*image)->getMipMapLevel(), par, &oldBounds);
            RectD mergeRoD = pointsBbox;
            mergeRoD.merge(otherRoD);
            RectI mergeBounds;
            mergeRoD.toPixelEnclosing(mipmapLevel, par, &mergeBounds);
            
            
            //upscale the original image
            source.reset(new Natron::Image(components,
                                        mergeRoD,
                                        mergeBounds,
                                        mipmapLevel,
                                        par,
                                        depth, false));
            source->fillZero(pixelPointsBbox);
            (*image)->upscaleMipMap(oldBounds, (*image)->getMipMapLevel(), source->getMipMapLevel(), source.get());
            *image = source;
        } else if ((*image)->getMipMapLevel() < mipmapLevel) {
            mipMapLevelChanged = true;
        
            RectD otherRoD = (*image)->getRoD();
            RectI oldBounds;
            otherRoD.toPixelEnclosing((*image)->getMipMapLevel(), par, &oldBounds);
            RectD mergeRoD = pointsBbox;
            mergeRoD.merge(otherRoD);
            RectI mergeBounds;
            mergeRoD.toPixelEnclosing(mipmapLevel, par, &mergeBounds);
            
            //downscale the original image
            source.reset(new Natron::Image(components,
                                        mergeRoD,
                                        mergeBounds,
                                        mipmapLevel,
                                        par,
                                        depth, false));
            source->fillZero(pixelPointsBbox);
            (*image)->downscaleMipMap(pointsBbox, oldBounds, (*image)->getMipMapLevel(), source->getMipMapLevel(), false, source.get());
            *image = source;
        } else {
            RectD otherRoD = (*image)->getRoD();
            RectI oldBounds = (*image)->getBounds();
            RectD mergeRoD = pointsBbox;
            mergeRoD.merge(otherRoD);
            source->setRoD(mergeRoD);
            source->ensureBounds(pixelPointsBbox,true);

        }
        copyFromImage = true;
    }

    bool doBuildUp = stroke->getBuildupKnob()->getValueAtTime(time);

    
    cairo_format_t cairoImgFormat;
    
    int srcNComps;
    //For the non build-up case, we use the LIGHTEN compositing operator, which only works on colors
    if (!doBuildUp || components.getNumComponents() > 1) {
        cairoImgFormat = CAIRO_FORMAT_ARGB32;
        srcNComps = 4;
    } else {
        cairoImgFormat = CAIRO_FORMAT_A8;
        srcNComps = 1;
    }
    
    
    ////Allocate the cairo temporary buffer
    cairo_surface_t* cairoImg;

    std::vector<unsigned char> buf;
    if (copyFromImage) {
        std::size_t stride = cairo_format_stride_for_width(cairoImgFormat, pixelPointsBbox.width());
        std::size_t memSize = stride * pixelPointsBbox.height();
        buf.resize(memSize);
        memset(buf.data(), 0, sizeof(unsigned char) * memSize);
        convertNatronImageToCairoImage<float, 1>(buf.data(), srcNComps, stride, source.get(), pixelPointsBbox, pixelPointsBbox, shapeColor);
        cairoImg = cairo_image_surface_create_for_data(buf.data(), cairoImgFormat, pixelPointsBbox.width(), pixelPointsBbox.height(),
                                                       stride);
       
    } else {
        cairoImg = cairo_image_surface_create(cairoImgFormat, pixelPointsBbox.width(), pixelPointsBbox.height() );
        cairo_surface_set_device_offset(cairoImg, -pixelPointsBbox.x1, -pixelPointsBbox.y1);
    }
    if (cairo_surface_status(cairoImg) != CAIRO_STATUS_SUCCESS) {
        return 0;
    }
    cairo_surface_set_device_offset(cairoImg, -pixelPointsBbox.x1, -pixelPointsBbox.y1);
    cairo_t* cr = cairo_create(cairoImg);
    //cairo_set_fill_rule(cr, CAIRO_FILL_RULE_EVEN_ODD); // creates holes on self-overlapping shapes
    cairo_set_fill_rule(cr, CAIRO_FILL_RULE_WINDING);
    
    // these Roto shapes must be rendered WITHOUT antialias, or the junction between the inner
    // polygon and the feather zone will have artifacts. This is partly due to the fact that cairo
    // meshes are not antialiased.
    // Use a default feather distance of 1 pixel instead!
    // UPDATE: unfortunately, this produces less artifacts, but there are still some remaining (use opacity=0.5 to test)
    // maybe the inner polygon should be made of mesh patterns too?
    cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
    
    std::list<std::list<std::pair<Natron::Point,double> > > strokes;
    std::list<std::pair<Natron::Point,double> > toScalePoints;
    int pot = 1 << mipmapLevel;
    if (mipmapLevel == 0) {
        toScalePoints = points;
    } else {
        for (std::list<std::pair<Natron::Point,double> >::const_iterator it = points.begin(); it!=points.end(); ++it) {
            std::pair<Natron::Point,double> p = *it;
            p.first.x /= pot;
            p.first.y /= pot;
            toScalePoints.push_back(p);
        }
    }
    strokes.push_back(toScalePoints);
    
    std::vector<cairo_pattern_t*> dotPatterns = stroke->getPatternCache();
    if (mipMapLevelChanged) {
        for (std::size_t i = 0; i < dotPatterns.size(); ++i) {
            if (dotPatterns[i]) {
                cairo_pattern_destroy(dotPatterns[i]);
                dotPatterns[i] = 0;
            }
        }
        dotPatterns.clear();
    }
    if (dotPatterns.empty()) {
        dotPatterns.resize(ROTO_PRESSURE_LEVELS);
        for (std::size_t i = 0; i < dotPatterns.size(); ++i) {
            dotPatterns[i] = (cairo_pattern_t*)0;
        }
    }
    
    
    double opacity = stroke->getOpacity(time);
    distToNext = _imp->renderStroke(cr, dotPatterns, strokes, distToNext, stroke, doBuildUp, opacity, time, mipmapLevel);
    
    stroke->updatePatternCache(dotPatterns);
    
    assert(cairo_surface_status(cairoImg) == CAIRO_STATUS_SUCCESS);
    
    ///A call to cairo_surface_flush() is required before accessing the pixel data
    ///to ensure that all pending drawing operations are finished.
    cairo_surface_flush(cairoImg);
    
    
    
    convertCairoImageToNatronImage_noColor<float, 1>(cairoImg, srcNComps, source.get(), pixelPointsBbox, shapeColor, 1., false);

    
    cairo_destroy(cr);
    ////Free the buffer used by Cairo
    cairo_surface_destroy(cairoImg);
    
    return distToNext;
}

boost::shared_ptr<Natron::Image>
RotoContext::renderMaskFromStroke(const boost::shared_ptr<RotoDrawableItem>& stroke,
                                  const RectI& /*roi*/,
                                  U64 rotoAge,
                                  U64 nodeHash,
                                  const Natron::ImageComponents& components,
                                  SequenceTime time,
                                  int view,
                                  Natron::ImageBitDepthEnum depth,
                                  unsigned int mipmapLevel)
{
    boost::shared_ptr<Node> node = getNode();
    
    
    
    ImagePtr image;// = stroke->getStrokeTimePreview();

    ///compute an enhanced hash different from the one of the node in order to differentiate within the cache
    ///the output image of the roto node and the mask image.
    Hash64 hash;
    hash.append(nodeHash);
    hash.append(rotoAge);
    hash.computeHash();
    
    Natron::ImageKey key = Natron::Image::makeKey(hash.value(), true ,time, view, false);
    
    {
        QMutexLocker k(&_imp->cacheAccessMutex);
        node->getLiveInstance()->getImageFromCacheAndConvertIfNeeded(true, false, key, mipmapLevel, NULL, NULL, depth, components, depth, components, EffectInstance::InputImagesMap(), boost::shared_ptr<RenderStats>(), &image);
    }
    if (image) {
        return image;
    }
    
    
    RectD bbox;
    
    std::list<std::list<std::pair<Natron::Point,double> > > strokes;
    
    RotoStrokeItem* isStroke = dynamic_cast<RotoStrokeItem*>(stroke.get());
    Bezier* isBezier = dynamic_cast<Bezier*>(stroke.get());
    if (isStroke) {
        isStroke->evaluateStroke(mipmapLevel, time, &strokes, &bbox);
    } else {
        assert(isBezier);
        bbox = isBezier->getBoundingBox(time);
        if (isBezier->isOpenBezier()) {
            std::list<Point> decastelJauPolygon;
            isBezier->evaluateAtTime_DeCasteljau_autoNbPoints(false, time, mipmapLevel, &decastelJauPolygon, 0);
            std::list<std::pair<Natron::Point,double> > points;
            for (std::list<Point> ::iterator it = decastelJauPolygon.begin(); it!=decastelJauPolygon.end(); ++it) {
                points.push_back(std::make_pair(*it, 1.));
            }
            strokes.push_back(points);
        }
        
    }
    
    RectI pixelRod;
    bbox.toPixelEnclosing(mipmapLevel, 1., &pixelRod);

    
    boost::shared_ptr<Natron::ImageParams> params = Natron::Image::makeParams( 0,
                                                                              bbox,
                                                                              pixelRod,
                                                                              1., // par
                                                                              mipmapLevel,
                                                                              false,
                                                                              components,
                                                                              depth,
                                                                              std::map<int,std::map<int, std::vector<RangeD> > >() );
    /*
     At this point we take the cacheAccessMutex so that no other thread can retrieve this image from the cache while it has not been 
     finished rendering. You might wonder why we do this differently here than in EffectInstance::renderRoI, this is because we do not use
     the trimap and notification system here in the rotocontext, which would be to much just for this small object, rather we just lock
     it once, which is fine.
     */
    QMutexLocker k(&_imp->cacheAccessMutex);
    
    Natron::getImageFromCacheOrCreate(key, params, &image);
    if (!image) {
        std::stringstream ss;
        ss << "Failed to allocate an image of ";
        ss << printAsRAM( params->getElementsCount() * sizeof(Natron::Image::data_t) ).toStdString();
        Natron::errorDialog( QObject::tr("Out of memory").toStdString(),ss.str() );
        
        return image;
    }
    
    ///Does nothing if image is already alloc
    image->allocateMemory();
    

    image = renderMaskInternal(stroke, pixelRod, components, time, depth, mipmapLevel, strokes, image);
    
    return image;
}




boost::shared_ptr<Natron::Image>
RotoContext::renderMaskInternal(const boost::shared_ptr<RotoDrawableItem>& stroke,
                                const RectI & roi,
                                const Natron::ImageComponents& components,
                                SequenceTime time,
                                Natron::ImageBitDepthEnum depth,
                                unsigned int mipmapLevel,
                                const std::list<std::list<std::pair<Natron::Point,double> > >& strokes,
                                const boost::shared_ptr<Natron::Image> &image)
{
 
    
    boost::shared_ptr<Node> node = getNode();
    
    RotoStrokeItem* isStroke = dynamic_cast<RotoStrokeItem*>(stroke.get());
    Bezier* isBezier = dynamic_cast<Bezier*>(stroke.get());
    cairo_format_t cairoImgFormat;
    
    int srcNComps;
    bool doBuildUp = true;
    
    if (isStroke) {
        doBuildUp = stroke->getBuildupKnob()->getValueAtTime(time);
        //For the non build-up case, we use the LIGHTEN compositing operator, which only works on colors
        if (!doBuildUp || components.getNumComponents() > 1) {
            cairoImgFormat = CAIRO_FORMAT_ARGB32;
            srcNComps = 4;
        } else {
            cairoImgFormat = CAIRO_FORMAT_A8;
            srcNComps = 1;
        }
        
    } else {
        cairoImgFormat = CAIRO_FORMAT_A8;
        srcNComps = 1;
    }
    

    ////Allocate the cairo temporary buffer
    cairo_surface_t* cairoImg = cairo_image_surface_create(cairoImgFormat, roi.width(), roi.height() );
    cairo_surface_set_device_offset(cairoImg, -roi.x1, -roi.y1);
    if (cairo_surface_status(cairoImg) != CAIRO_STATUS_SUCCESS) {
        return image;
    }
    cairo_t* cr = cairo_create(cairoImg);
    //cairo_set_fill_rule(cr, CAIRO_FILL_RULE_EVEN_ODD); // creates holes on self-overlapping shapes
    cairo_set_fill_rule(cr, CAIRO_FILL_RULE_WINDING);
    
    // these Roto shapes must be rendered WITHOUT antialias, or the junction between the inner
    // polygon and the feather zone will have artifacts. This is partly due to the fact that cairo
    // meshes are not antialiased.
    // Use a default feather distance of 1 pixel instead!
    // UPDATE: unfortunately, this produces less artifacts, but there are still some remaining (use opacity=0.5 to test)
    // maybe the inner polygon should be made of mesh patterns too?
    cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);

    
    double shapeColor[3];
    stroke->getColor(time, shapeColor);

    double opacity = stroke->getOpacity(time);

    assert(isStroke || isBezier);
    if (isStroke || !isBezier || (isBezier && isBezier->isOpenBezier())) {
        std::vector<cairo_pattern_t*> dotPatterns(ROTO_PRESSURE_LEVELS);
        for (std::size_t i = 0; i < dotPatterns.size(); ++i) {
            dotPatterns[i] = (cairo_pattern_t*)0;
        }
        _imp->renderStroke(cr, dotPatterns, strokes, 0, stroke, doBuildUp, opacity, time, mipmapLevel);
        
        for (std::size_t i = 0; i < dotPatterns.size(); ++i) {
            if (dotPatterns[i]) {
                cairo_pattern_destroy(dotPatterns[i]);
                dotPatterns[i] = 0;
            }
        }
        
        
    } else {
        _imp->renderBezier(cr, isBezier, opacity, time, mipmapLevel);
    }
    
    bool useOpacityToConvert = (isBezier != 0);
    
    switch (depth) {
        case Natron::eImageBitDepthFloat:
            convertCairoImageToNatronImage_noColor<float, 1>(cairoImg, srcNComps, image.get(), roi, shapeColor, opacity, useOpacityToConvert);
            break;
        case Natron::eImageBitDepthByte:
            convertCairoImageToNatronImage_noColor<unsigned char, 255>(cairoImg, srcNComps,  image.get(), roi,shapeColor, opacity, useOpacityToConvert);
            break;
        case Natron::eImageBitDepthShort:
            convertCairoImageToNatronImage_noColor<unsigned short, 65535>(cairoImg, srcNComps, image.get(), roi,shapeColor, opacity, useOpacityToConvert);
            break;
        case Natron::eImageBitDepthHalf:
        case Natron::eImageBitDepthNone:
            assert(false);
            break;
    }

    
    assert(cairo_surface_status(cairoImg) == CAIRO_STATUS_SUCCESS);
    
    ///A call to cairo_surface_flush() is required before accessing the pixel data
    ///to ensure that all pending drawing operations are finished.
    cairo_surface_flush(cairoImg);
    
    
    
    cairo_destroy(cr);
    ////Free the buffer used by Cairo
    cairo_surface_destroy(cairoImg);

    
    return image;
}


static inline
double hardnessGaussLookup(double f)
{
    //2 hyperbolas + 1 parabola to approximate a gauss function
    if (f < -0.5) {
        f = -1. - f;
        return (2. * f * f);
    }
    
    if (f < 0.5) {
        return (1. - 2. * f * f);
    }
    f = 1. - f;
    return (2. * f * f);
}


void
RotoContextPrivate::renderDot(cairo_t* cr,
                              std::vector<cairo_pattern_t*>& dotPatterns,
                              const Point &center,
                              double internalDotRadius,
                              double externalDotRadius,
                              double pressure,
                              bool doBuildUp,
                              const std::vector<std::pair<double, double> >& opacityStops,
                              double opacity)
{
    
    if (!opacityStops.empty()) {
        cairo_pattern_t* pattern;
        // sometimes, Qt gives a pressure level > 1... so we clamp it
        int pressureInt = int(std::max(0., std::min(pressure, 1.)) * (ROTO_PRESSURE_LEVELS-1) + 0.5);
        assert(pressureInt >= 0 && pressureInt < ROTO_PRESSURE_LEVELS);
        if (dotPatterns[pressureInt]) {
            pattern = dotPatterns[pressureInt];
        } else {
            pattern = cairo_pattern_create_radial(0, 0, internalDotRadius, 0, 0, externalDotRadius);
            for (std::size_t i = 0; i < opacityStops.size(); ++i) {
                if (doBuildUp) {
                    cairo_pattern_add_color_stop_rgba(pattern, opacityStops[i].first, 1., 1., 1.,opacityStops[i].second);
                } else {
                    cairo_pattern_add_color_stop_rgba(pattern, opacityStops[i].first, opacityStops[i].second, opacityStops[i].second, opacityStops[i].second,1);
                }
            }
            //dotPatterns[pressureInt] = pattern;
        }
        cairo_translate(cr, center.x, center.y);
        cairo_set_source(cr, pattern);
        cairo_translate(cr, -center.x, -center.y);
    } else {
        if (doBuildUp) {
            cairo_set_source_rgba(cr, 1., 1., 1., opacity);
        } else {
            cairo_set_source_rgba(cr, opacity, opacity, opacity, 1.);
        }
    }
#ifdef DEBUG
    //Make sure the dot we are about to render falls inside the clip region, otherwise the bounds of the image are mis-calculated.
    cairo_surface_t* target = cairo_get_target(cr);
    int w = cairo_image_surface_get_width(target);
    int h = cairo_image_surface_get_height(target);
    double x1,y1;
    cairo_surface_get_device_offset(target, &x1, &y1);
    assert(std::floor(center.x - externalDotRadius) >= -x1 && std::floor(center.x + externalDotRadius) < -x1 + w &&
           std::floor(center.y - externalDotRadius) >= -y1 && std::floor(center.y + externalDotRadius) < -y1 + h);
#endif
    cairo_arc(cr, center.x, center.y, externalDotRadius, 0, M_PI * 2);
    cairo_fill(cr);
}

static void getRenderDotParams(double alpha, double brushSizePixel, double brushHardness, double brushSpacing, double pressure, bool pressureAffectsOpacity, bool pressureAffectsSize, bool pressureAffectsHardness, double* internalDotRadius, double* externalDotRadius, double * spacing, std::vector<std::pair<double,double> >* opacityStops)
{
    if (pressureAffectsSize) {
        brushSizePixel *= pressure;
    }
    if (pressureAffectsHardness) {
        brushHardness *= pressure;
    }
    if (pressureAffectsOpacity) {
        alpha *= pressure;
    }
    
    *internalDotRadius = std::max(brushSizePixel * brushHardness,1.) / 2.;
    *externalDotRadius = std::max(brushSizePixel, 1.) / 2.;
    *spacing = *externalDotRadius * 2. * brushSpacing;
    
    
    opacityStops->clear();
    
    double exp = brushHardness != 1.0 ?  0.4 / (1.0 - brushHardness) : 0.;
    const int maxStops = 8;
    double incr = 1. / maxStops;
    
    if (brushHardness != 1.) {
        for (double d = 0; d <= 1.; d += incr) {
            double o = hardnessGaussLookup(std::pow(d, exp));
            opacityStops->push_back(std::make_pair(d, o * alpha));
        }
    }
    
}

double
RotoContextPrivate::renderStroke(cairo_t* cr,
                                 std::vector<cairo_pattern_t*>& dotPatterns,
                                 const std::list<std::list<std::pair<Natron::Point,double> > >& strokes,
                                 double distToNext,
                                 const boost::shared_ptr<RotoDrawableItem>&  stroke,
                                 bool doBuildup,
                                 double alpha,
                                 double time,
                                 unsigned int mipmapLevel)
{
    if (strokes.empty()) {
        return distToNext;
    }
    
    if (!stroke->isActivated(time)) {
        return distToNext;
    }
    
    assert(dotPatterns.size() == ROTO_PRESSURE_LEVELS);
    
    boost::shared_ptr<Double_Knob> brushSizeKnob = stroke->getBrushSizeKnob();
    double brushSize = brushSizeKnob->getValueAtTime(time);
    boost::shared_ptr<Double_Knob> brushSpacingKnob = stroke->getBrushSpacingKnob();
    double brushSpacing = brushSpacingKnob->getValueAtTime(time);
    if (brushSpacing == 0.) {
        return distToNext;
    }
    
    brushSpacing = std::max(brushSpacing, 0.05);
    
    boost::shared_ptr<Double_Knob> brushHardnessKnob = stroke->getBrushHardnessKnob();
    double brushHardness = brushHardnessKnob->getValueAtTime(time);
    boost::shared_ptr<Double_Knob> visiblePortionKnob = stroke->getBrushVisiblePortionKnob();
    double writeOnStart = visiblePortionKnob->getValueAtTime(time, 0);
    double writeOnEnd = visiblePortionKnob->getValueAtTime(time, 1);
    if ((writeOnEnd - writeOnStart) <= 0.) {
        return distToNext;
    }
    
    boost::shared_ptr<Bool_Knob> pressureOpacityKnob = stroke->getPressureOpacityKnob();
    boost::shared_ptr<Bool_Knob> pressureSizeKnob = stroke->getPressureSizeKnob();
    boost::shared_ptr<Bool_Knob> pressureHardnessKnob = stroke->getPressureHardnessKnob();
    
    bool pressureAffectsOpacity = pressureOpacityKnob->getValueAtTime(time);
    bool pressureAffectsSize = pressureSizeKnob->getValueAtTime(time);
    bool pressureAffectsHardness = pressureHardnessKnob->getValueAtTime(time);
    
    double brushSizePixel = brushSize;
    if (mipmapLevel != 0) {
        brushSizePixel = std::max(1.,brushSizePixel / (1 << mipmapLevel));
    }
    cairo_set_operator(cr,doBuildup ? CAIRO_OPERATOR_OVER : CAIRO_OPERATOR_LIGHTEN);

    
    for (std::list<std::list<std::pair<Natron::Point,double> > >::const_iterator strokeIt = strokes.begin() ;strokeIt != strokes.end() ;++strokeIt) {
        int firstPoint = (int)std::floor((strokeIt->size() * writeOnStart));
        int endPoint = (int)std::ceil((strokeIt->size() * writeOnEnd));
        assert(firstPoint >= 0 && firstPoint < (int)strokeIt->size() && endPoint > firstPoint && endPoint <= (int)strokeIt->size());
        
        
        ///The visible portion of the paint's stroke with points adjusted to pixel coordinates
        std::list<std::pair<Point,double> > visiblePortion;
        std::list<std::pair<Point,double> >::const_iterator startingIt = strokeIt->begin();
        std::list<std::pair<Point,double> >::const_iterator endingIt = strokeIt->begin();
        std::advance(startingIt, firstPoint);
        std::advance(endingIt, endPoint);
        for (std::list<std::pair<Point,double> >::const_iterator it = startingIt; it!=endingIt; ++it) {
            visiblePortion.push_back(*it);
        }
        if (visiblePortion.empty()) {
            return distToNext;
        }
        
        std::list<std::pair<Point,double> >::iterator it = visiblePortion.begin();
        
        if (visiblePortion.size() == 1) {
            double internalDotRadius, externalDotRadius, spacing;
            std::vector<std::pair<double,double> > opacityStops;
            getRenderDotParams(alpha, brushSizePixel, brushHardness, brushSpacing, it->second, pressureAffectsOpacity, pressureAffectsSize, pressureAffectsHardness, &internalDotRadius, &externalDotRadius, &spacing, &opacityStops);
            renderDot(cr, dotPatterns, it->first, internalDotRadius, externalDotRadius, it->second, doBuildup, opacityStops, alpha);
            return 0;
        }
        
        std::list<std::pair<Point,double> >::iterator next = it;
        ++next;
        
        while (next!=visiblePortion.end()) {
            //Render for each point a dot. Spacing is a percentage of brushSize:
            //Spacing at 1 means no dot is overlapping another (so the spacing is in fact brushSize)
            //Spacing at 0 we do not render the stroke
            
            double dist = std::sqrt((next->first.x - it->first.x) * (next->first.x - it->first.x) +  (next->first.y - it->first.y) * (next->first.y - it->first.y));
            
            // while the next point can be drawn on this segment, draw a point and advance
            while (distToNext <= dist) {
                double a = dist == 0. ? 0. : distToNext/dist;
                Point center = {
                    it->first.x * (1 - a) + next->first.x * a,
                    it->first.y * (1 - a) + next->first.y * a
                };
                double pressure = it->second * (1 - a) + next->second * a;
                
                // draw the dot
                double internalDotRadius, externalDotRadius, spacing;
                std::vector<std::pair<double,double> > opacityStops;
                getRenderDotParams(alpha, brushSizePixel, brushHardness, brushSpacing, pressure, pressureAffectsOpacity, pressureAffectsSize, pressureAffectsHardness, &internalDotRadius, &externalDotRadius, &spacing, &opacityStops);
                renderDot(cr, dotPatterns, center, internalDotRadius, externalDotRadius, pressure, doBuildup, opacityStops, alpha);
                
                distToNext += spacing;
            }
            
            // go to the next segment
            distToNext -= dist;
            ++next;
            ++it;
        }
    }
    
    
    return distToNext;
}



void
RotoContextPrivate::renderBezier(cairo_t* cr,const Bezier* bezier,double opacity, double time, unsigned int mipmapLevel)
{
    ///render the bezier only if finished (closed) and activated
    if ( !bezier->isCurveFinished() || !bezier->isActivated(time) || ( bezier->getControlPointsCount() <= 1 ) ) {
        return;
    }
    
    
    double fallOff = bezier->getFeatherFallOff(time);
    double featherDist = bezier->getFeatherDistance(time);
#ifdef NATRON_ROTO_INVERTIBLE
    bool inverted = (*it2)->getInverted(time);
#else
    const bool inverted = false;
#endif
    double shapeColor[3];
    bezier->getColor(time, shapeColor);
    
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
    
    BezierCPs cps = bezier->getControlPoints_mt_safe();
    
    BezierCPs fps = bezier->getFeatherPoints_mt_safe();
    
    assert( cps.size() == fps.size() );
    
    if ( cps.empty() ) {
        return;
    }
    
    cairo_new_path(cr);
    
    ////Define the feather edge pattern
    cairo_pattern_t* mesh = cairo_pattern_create_mesh();
    if (cairo_pattern_status(mesh) != CAIRO_STATUS_SUCCESS) {
        cairo_pattern_destroy(mesh);
        return;
    }
    
    ///Adjust the feather distance so it takes the mipmap level into account
    if (mipmapLevel != 0) {
        featherDist /= (1 << mipmapLevel);
    }
    
    Transform::Matrix3x3 transform;
    bezier->getTransformAtTime(time, &transform);
    
    
    renderFeather(bezier, time, mipmapLevel, inverted, shapeColor, opacity, featherDist, fallOff, mesh);
    
    
    if (!inverted) {
        // strangely, the above-mentioned cairo bug doesn't affect this function
        renderInternalShape(time, mipmapLevel, shapeColor, opacity, transform, cr, mesh, cps);
#ifdef NATRON_ROTO_INVERTIBLE
    } else {
#pragma message WARN("doesn't work! the image should be infinite for this to work!")
        // Doesn't work! the image should be infinite for this to work!
        // Or at least it should contain the Union of the source RoDs.
        // Here, it only contains the boinding box of the Bezier.
        // If there's a transform after the roto node, a black border will appear.
        // The only solution would be to have a color parameter which specifies how on image is outside of its RoD.
        // Unfortunately, the OFX definition is: "it is black and transparent"
        
        ///If inverted, draw an inverted rectangle on all the image first
        // with a hole consisting of the feather polygon
        
        double xOffset, yOffset;
        cairo_surface_get_device_offset(cairoImg, &xOffset, &yOffset);
        int width = cairo_image_surface_get_width(cairoImg);
        int height = cairo_image_surface_get_height(cairoImg);
        
        cairo_move_to(cr, -xOffset, -yOffset);
        cairo_line_to(cr, -xOffset + width, -yOffset);
        cairo_line_to(cr, -xOffset + width, -yOffset + height);
        cairo_line_to(cr, -xOffset, -yOffset + height);
        cairo_line_to(cr, -xOffset, -yOffset);
        // strangely, the above-mentioned cairo bug doesn't affect this function
#pragma message WARN("WRONG! should use the outer feather contour, *displaced* by featherDistance, not fps")
        renderInternalShape(time, mipmapLevel, cr, fps);
#endif
    }
    
    applyAndDestroyMask(cr, mesh);

}

void
RotoContextPrivate::renderFeather(const Bezier* bezier,double time, unsigned int mipmapLevel, bool inverted, double shapeColor[3], double /*opacity*/, double featherDist, double fallOff, cairo_pattern_t* mesh)
{
    
    ///Note that we do not use the opacity when rendering the bezier, it is rendered with correct floating point opacity/color when converting
    ///to the Natron image.
    
    double fallOffInverse = 1. / fallOff;
    /*
     * We descretize the feather control points to obtain a polygon so that the feather distance will be of the same thickness around all the shape.
     * If we were to extend only the end points, the resulting bezier interpolation would create a feather with different thickness around the shape,
     * yielding an unwanted behaviour for the end user.
     */
    ///here is the polygon of the feather bezier
    ///This is used only if the feather distance is different of 0 and the feather points equal
    ///the control points in order to still be able to apply the feather distance.
    std::list<Point> featherPolygon;
    std::list<Point> bezierPolygon;
    RectD featherPolyBBox;
    featherPolyBBox.setupInfinity();
    
    bezier->evaluateFeatherPointsAtTime_DeCasteljau(false, time, mipmapLevel, 50, true, &featherPolygon, &featherPolyBBox);
    bezier->evaluateAtTime_DeCasteljau(false, time, mipmapLevel, 50, &bezierPolygon, NULL);
    
    bool clockWise = bezier->isFeatherPolygonClockwiseOriented(false,time);
    
    assert( !featherPolygon.empty() && !bezierPolygon.empty());


    std::list<Point> featherContour;

    // prepare iterators
    std::list<Point>::iterator next = featherPolygon.begin();
    ++next;  // can only be valid since we assert the list is not empty
    if (next == featherPolygon.end()) {
        next = featherPolygon.begin();
    }
    std::list<Point>::iterator prev = featherPolygon.end();
    --prev; // can only be valid since we assert the list is not empty
    std::list<Point>::iterator bezIT = bezierPolygon.begin();
    std::list<Point>::iterator prevBez = bezierPolygon.end();
    --prevBez; // can only be valid since we assert the list is not empty

    // prepare p1
    double absFeatherDist = std::abs(featherDist);
    Point p1 = *featherPolygon.begin();
    double norm = sqrt( (next->x - prev->x) * (next->x - prev->x) + (next->y - prev->y) * (next->y - prev->y) );
    assert(norm != 0);
    double dx = -( (next->y - prev->y) / norm );
    double dy = ( (next->x - prev->x) / norm );

    if (!clockWise) {
        p1.x -= dx * absFeatherDist;
        p1.y -= dy * absFeatherDist;
    } else {
        p1.x += dx * absFeatherDist;
        p1.y += dy * absFeatherDist;
    }
    
    Point origin = p1;
    featherContour.push_back(p1);


    // increment for first iteration
    std::list<Point>::iterator cur = featherPolygon.begin();
    // ++cur, ++prev, ++next, ++bezIT, ++prevBez
    // all should be valid, actually
    assert(cur != featherPolygon.end() &&
           prev != featherPolygon.end() &&
           next != featherPolygon.end() &&
           bezIT != bezierPolygon.end() &&
           prevBez != bezierPolygon.end());
    if (cur != featherPolygon.end()) {
        ++cur;
    }
    if (prev != featherPolygon.end()) {
        ++prev;
    }
    if (next != featherPolygon.end()) {
        ++next;
    }
    if (bezIT != bezierPolygon.end()) {
        ++bezIT;
    }
    if (prevBez != bezierPolygon.end()) {
        ++prevBez;
    }

    for (;; ++cur) { // for each point in polygon
        if ( next == featherPolygon.end() ) {
            next = featherPolygon.begin();
        }
        if ( prev == featherPolygon.end() ) {
            prev = featherPolygon.begin();
        }
        if ( bezIT == bezierPolygon.end() ) {
            bezIT = bezierPolygon.begin();
        }
        if ( prevBez == bezierPolygon.end() ) {
            prevBez = bezierPolygon.begin();
        }
        bool mustStop = false;
        if ( cur == featherPolygon.end() ) {
            mustStop = true;
            cur = featherPolygon.begin();
        }
        
        ///skip it
        if ( (cur->x == prev->x) && (cur->y == prev->y) ) {
            continue;
        }
        
        Point p0, p0p1, p1p0, p2, p2p3, p3p2, p3;
        p0.x = prevBez->x;
        p0.y = prevBez->y;
        p3.x = bezIT->x;
        p3.y = bezIT->y;
        
        if (!mustStop) {
            norm = sqrt( (next->x - prev->x) * (next->x - prev->x) + (next->y - prev->y) * (next->y - prev->y) );
            assert(norm != 0);
            dx = -( (next->y - prev->y) / norm );
            dy = ( (next->x - prev->x) / norm );
            p2 = *cur;

            if (!clockWise) {
                p2.x -= dx * absFeatherDist;
                p2.y -= dy * absFeatherDist;
            } else {
                p2.x += dx * absFeatherDist;
                p2.y += dy * absFeatherDist;
            }
        } else {
            p2 = origin;
        }
        featherContour.push_back(p2);
        
        ///linear interpolation
        p0p1.x = (p0.x * fallOff * 2. + fallOffInverse * p1.x) / (fallOff * 2. + fallOffInverse);
        p0p1.y = (p0.y * fallOff * 2. + fallOffInverse * p1.y) / (fallOff * 2. + fallOffInverse);
        p1p0.x = (p0.x * fallOff + 2. * fallOffInverse * p1.x) / (fallOff + 2. * fallOffInverse);
        p1p0.y = (p0.y * fallOff + 2. * fallOffInverse * p1.y) / (fallOff + 2. * fallOffInverse);

        
        p2p3.x = (p3.x * fallOff + 2. * fallOffInverse * p2.x) / (fallOff + 2. * fallOffInverse);
        p2p3.y = (p3.y * fallOff + 2. * fallOffInverse * p2.y) / (fallOff + 2. * fallOffInverse);
        p3p2.x = (p3.x * fallOff * 2. + fallOffInverse * p2.x) / (fallOff * 2. + fallOffInverse);
        p3p2.y = (p3.y * fallOff * 2. + fallOffInverse * p2.y) / (fallOff * 2. + fallOffInverse);

        
        ///move to the initial point
        cairo_mesh_pattern_begin_patch(mesh);
        cairo_mesh_pattern_move_to(mesh, p0.x, p0.y);
        cairo_mesh_pattern_curve_to(mesh, p0p1.x, p0p1.y, p1p0.x, p1p0.y, p1.x, p1.y);
        cairo_mesh_pattern_line_to(mesh, p2.x, p2.y);
        cairo_mesh_pattern_curve_to(mesh, p2p3.x, p2p3.y, p3p2.x, p3p2.y, p3.x, p3.y);
        cairo_mesh_pattern_line_to(mesh, p0.x, p0.y);
        ///Set the 4 corners color
        ///inner is full color
        
        // IMPORTANT NOTE:
        // The two sqrt below are due to a probable cairo bug.
        // To check wether the bug is present is a given cairo version,
        // make any shape with a very large feather and set
        // opacity to 0.5. Then, zoom on the polygon border to check if the intensity is continuous
        // and approximately equal to 0.5.
        // If the bug if ixed in cairo, please use #if CAIRO_VERSION>xxx to keep compatibility with
        // older Cairo versions.
        cairo_mesh_pattern_set_corner_color_rgba( mesh, 0, shapeColor[0], shapeColor[1], shapeColor[2],
                                                 std::sqrt(inverted ? 0. : 1.) );
        ///outter is faded
        cairo_mesh_pattern_set_corner_color_rgba(mesh, 1, shapeColor[0], shapeColor[1], shapeColor[2],
                                                 inverted ? 1. : 0.);
        cairo_mesh_pattern_set_corner_color_rgba(mesh, 2, shapeColor[0], shapeColor[1], shapeColor[2],
                                                 inverted ? 1. : 0.);
        ///inner is full color
        cairo_mesh_pattern_set_corner_color_rgba(mesh, 3, shapeColor[0], shapeColor[1], shapeColor[2],
                                                 std::sqrt(inverted ? 0. : 1.));
        assert(cairo_pattern_status(mesh) == CAIRO_STATUS_SUCCESS);
        
        cairo_mesh_pattern_end_patch(mesh);
        
        if (mustStop) {
            break;
        }
        
        p1 = p2;

        // increment for next iteration
        // ++prev, ++next, ++bezIT, ++prevBez
        if (prev != featherPolygon.end()) {
            ++prev;
        }
        if (next != featherPolygon.end()) {
            ++next;
        }
        if (bezIT != bezierPolygon.end()) {
            ++bezIT;
        }
        if (prevBez != bezierPolygon.end()) {
            ++prevBez;
        }

    }  // for each point in polygon

}

void
RotoContextPrivate::renderInternalShape(double time,
                                        unsigned int mipmapLevel,
                                        double /*shapeColor*/[3],
                                        double /*opacity*/,
                                        const Transform::Matrix3x3& transform,
                                        cairo_t* cr,
#ifdef ROTO_USE_MESH_PATTERN_ONLY
                                        cairo_pattern_t* mesh,
#else
                                        cairo_pattern_t* /*mesh*/,
#endif
                                        const BezierCPs & cps)
{
    assert(!cps.empty());
#ifdef ROTO_USE_MESH_PATTERN_ONLY
    std::list<BezierCPs> coonPatches;
    bezulate(time, cps, &coonPatches);
    
    for (std::list<BezierCPs>::iterator it = coonPatches.begin(); it != coonPatches.end(); ++it) {
        
        std::list<BezierCPs> fixedPatch;
        Natron::regularize(*it, time, &fixedPatch);
        for (std::list<BezierCPs>::iterator it2 = fixedPatch.begin(); it2 != fixedPatch.end(); ++it2) {
            
            
            
            std::size_t size = it2->size();
            assert(size <= 4 && size >= 2);
            
            BezierCPs::iterator patchIT = it2->begin();
            boost::shared_ptr<BezierCP> p0ptr,p1ptr,p2ptr,p3ptr;
            p0ptr = *patchIT;
            ++patchIT;
            if (size == 2) {
                p1ptr = p0ptr;
                p2ptr = *patchIT;
                p3ptr = p2ptr;
            } else if (size == 3) {
                p1ptr = *patchIT;
                p2ptr = *patchIT;
                ++patchIT;
                p3ptr = *patchIT;
            } else if (size == 4) {
                p1ptr = *patchIT;
                ++patchIT;
                p2ptr = *patchIT;
                ++patchIT;
                p3ptr = *patchIT;
            }
            assert(p0ptr && p1ptr && p2ptr && p3ptr);
            
            Point p0,p0p1,p1p0,p1,p1p2,p2p1,p2p3,p3p2,p2,p3,p3p0,p0p3;
            
            p0ptr->getLeftBezierPointAtTime(time, &p0p3.x, &p0p3.y);
            p0ptr->getPositionAtTime(time, &p0.x, &p0.y);
            p0ptr->getRightBezierPointAtTime(time, &p0p1.x, &p0p1.y);
            
            p1ptr->getLeftBezierPointAtTime(time, &p1p0.x, &p1p0.y);
            p1ptr->getPositionAtTime(time, &p1.x, &p1.y);
            p1ptr->getRightBezierPointAtTime(time, &p1p2.x, &p1p2.y);
            
            p2ptr->getLeftBezierPointAtTime(time, &p2p1.x, &p2p1.y);
            p2ptr->getPositionAtTime(time, &p2.x, &p2.y);
            p2ptr->getRightBezierPointAtTime(time, &p2p3.x, &p2p3.y);
            
            p3ptr->getLeftBezierPointAtTime(time, &p3p2.x, &p3p2.y);
            p3ptr->getPositionAtTime(time, &p3.x, &p3.y);
            p3ptr->getRightBezierPointAtTime(time, &p3p0.x, &p3p0.y);
            
            
            adjustToPointToScale(mipmapLevel, p0.x, p0.y);
            adjustToPointToScale(mipmapLevel, p0p1.x, p0p1.y);
            adjustToPointToScale(mipmapLevel, p1p0.x, p1p0.y);
            adjustToPointToScale(mipmapLevel, p1.x, p1.y);
            adjustToPointToScale(mipmapLevel, p1p2.x, p1p2.y);
            adjustToPointToScale(mipmapLevel, p2p1.x, p2p1.y);
            adjustToPointToScale(mipmapLevel, p2.x, p2.y);
            adjustToPointToScale(mipmapLevel, p2p3.x, p2p3.y);
            adjustToPointToScale(mipmapLevel, p3p2.x, p3p2.y);
            adjustToPointToScale(mipmapLevel, p3.x, p3.y);
            adjustToPointToScale(mipmapLevel, p3p0.x, p3p0.y);
            adjustToPointToScale(mipmapLevel, p0p3.x, p0p3.y);
            
            
            /*
             Add a Coons patch such as:
             
             C1  Side 1   C2
             +---------------+
             |               |
             |  P1       P2  |
             |               |
             Side 0 |               | Side 2
             |               |
             |               |
             |  P0       P3  |
             |               |
             +---------------+
             C0     Side 3   C3
             
             In the above drawing, C0 is p0, P0 is p0p1, P1 is p1p0, C1 is p1 and so on...
             */
            
            ///move to C0
            cairo_mesh_pattern_begin_patch(mesh);
            cairo_mesh_pattern_move_to(mesh, p0.x, p0.y);
            if (size == 4) {
                cairo_mesh_pattern_curve_to(mesh, p0p1.x, p0p1.y, p1p0.x, p1p0.y, p1.x, p1.y);
                cairo_mesh_pattern_curve_to(mesh, p1p2.x, p1p2.y, p2p1.x, p2p1.y, p2.x, p2.y);
                cairo_mesh_pattern_curve_to(mesh, p2p3.x, p2p3.y, p3p2.x, p3p2.y, p3.x, p3.y);
                cairo_mesh_pattern_curve_to(mesh, p3p0.x, p3p0.y, p0p3.x, p0p3.y, p0.x, p0.y);
            } else if (size == 3) {
                cairo_mesh_pattern_curve_to(mesh, p0p1.x, p0p1.y, p1p0.x, p1p0.y, p1.x, p1.y);
                cairo_mesh_pattern_line_to(mesh, p2.x, p2.y);
                cairo_mesh_pattern_curve_to(mesh, p2p3.x, p2p3.y, p3p2.x, p3p2.y, p3.x, p3.y);
                cairo_mesh_pattern_curve_to(mesh, p3p0.x, p3p0.y, p0p3.x, p0p3.y, p0.x, p0.y);
            } else {
                assert(size == 2);
                cairo_mesh_pattern_line_to(mesh, p1.x, p1.y);
                cairo_mesh_pattern_curve_to(mesh, p1p2.x, p1p2.y, p2p1.x, p2p1.y, p2.x, p2.y);
                cairo_mesh_pattern_line_to(mesh, p3.x, p3.y);
                cairo_mesh_pattern_curve_to(mesh, p3p0.x, p3p0.y, p0p3.x, p0p3.y, p0.x, p0.y);
            }
            ///Set the 4 corners color
            
            // IMPORTANT NOTE:
            // The two sqrt below are due to a probable cairo bug.
            // To check wether the bug is present is a given cairo version,
            // make any shape with a very large feather and set
            // opacity to 0.5. Then, zoom on the polygon border to check if the intensity is continuous
            // and approximately equal to 0.5.
            // If the bug if ixed in cairo, please use #if CAIRO_VERSION>xxx to keep compatibility with
            // older Cairo versions.
            cairo_mesh_pattern_set_corner_color_rgba( mesh, 0, shapeColor[0], shapeColor[1], shapeColor[2],
                                                     std::sqrt(opacity) );
            cairo_mesh_pattern_set_corner_color_rgba(mesh, 1, shapeColor[0], shapeColor[1], shapeColor[2],
                                                     opacity);
            cairo_mesh_pattern_set_corner_color_rgba(mesh, 2, shapeColor[0], shapeColor[1], shapeColor[2],
                                                     opacity);
            cairo_mesh_pattern_set_corner_color_rgba(mesh, 3, shapeColor[0], shapeColor[1], shapeColor[2],
                                                     std::sqrt(opacity));
            assert(cairo_pattern_status(mesh) == CAIRO_STATUS_SUCCESS);
            
            cairo_mesh_pattern_end_patch(mesh);
            
        }
    }
#else
    
    cairo_set_source_rgba(cr, 1, 1, 1, 1);
    
    BezierCPs::const_iterator point = cps.begin();
    assert(point != cps.end());
    BezierCPs::const_iterator nextPoint = point;
    if (nextPoint != cps.end()) {
        ++nextPoint;
    }

    
    Transform::Point3D initCp;
    (*point)->getPositionAtTime(false,time, &initCp.x,&initCp.y);
    initCp.z = 1.;
    initCp = Transform::matApply(transform, initCp);
    
    adjustToPointToScale(mipmapLevel,initCp.x,initCp.y);

    cairo_move_to(cr, initCp.x,initCp.y);

    while ( point != cps.end() ) {
        if ( nextPoint == cps.end() ) {
            nextPoint = cps.begin();
        }
        
        Transform::Point3D right,nextLeft,next;
        (*point)->getRightBezierPointAtTime(false,time, &right.x, &right.y);
        right.z = 1;
        (*nextPoint)->getLeftBezierPointAtTime(false,time, &nextLeft.x, &nextLeft.y);
        nextLeft.z = 1;
        (*nextPoint)->getPositionAtTime(false,time, &next.x, &next.y);
        next.z = 1;

        right = Transform::matApply(transform, right);
        nextLeft = Transform::matApply(transform, nextLeft);
        next = Transform::matApply(transform, next);
        
        adjustToPointToScale(mipmapLevel,right.x,right.y);
        adjustToPointToScale(mipmapLevel,next.x,next.y);
        adjustToPointToScale(mipmapLevel,nextLeft.x,nextLeft.y);
        cairo_curve_to(cr, right.x, right.y, nextLeft.x, nextLeft.y, next.x, next.y);

        // increment for next iteration
        ++point;
        if (nextPoint != cps.end()) {
            ++nextPoint;
        }
    } // while()
//    if (cairo_get_antialias(cr) != CAIRO_ANTIALIAS_NONE ) {
//        cairo_fill_preserve(cr);
//        // These line properties make for a nicer looking polygon mesh
//        cairo_set_line_join(cr, CAIRO_LINE_JOIN_BEVEL);
//        cairo_set_line_cap(cr, CAIRO_LINE_CAP_BUTT);
//        // Comment out the following call to cairo_set_line width
//        // since the hard-coded width value of 1.0 is not appropriate
//        // for fills of small areas. Instead, use the line width that
//        // has already been set by the user via the above call of
//        // poly_line which in turn calls set_current_context which in
//        // turn calls cairo_set_line_width for the user-specified
//        // width.
//        cairo_set_line_width(cr, 1.0);
//        cairo_stroke(cr);
//    } else {
    cairo_fill(cr);
//    }
#endif
}

struct qpointf_compare_less
{
    bool operator() (const QPointF& lhs,const QPointF& rhs) const
    {
        if (std::abs(lhs.x() - rhs.x()) < 1e-6) {
            if (std::abs(lhs.y() - rhs.y()) < 1e-6) {
                return false;
            } else if (lhs.y() < rhs.y()) {
                return true;
            } else {
                return false;
            }
        } else if (lhs.x() < rhs.x()) {
            return true;
        } else {
            return false;
        }
    }
};



//From http://www.math.ualberta.ca/~bowman/publications/cad10.pdf
void
RotoContextPrivate::bezulate(double time, const BezierCPs& cps,std::list<BezierCPs>* patches)
{
    BezierCPs simpleClosedCurve = cps;
    
    while (simpleClosedCurve.size() > 4) {
        
        bool found = false;
        for (int n = 3; n >= 2; --n) {
            
            assert((int)simpleClosedCurve.size() > n);
            
            //next points at point i + n
            BezierCPs::iterator next = simpleClosedCurve.begin();
            std::advance(next, n);
            
            std::list<Point> polygon;
            RectD bbox;
            bbox.setupInfinity();
            for (BezierCPs::iterator it = simpleClosedCurve.begin(); it != simpleClosedCurve.end(); ++it) {
                Point p;
                (*it)->getPositionAtTime(false,time, &p.x, &p.y);
                polygon.push_back(p);
                if (p.x < bbox.x1) {
                    bbox.x1 = p.x;
                }
                if (p.x > bbox.x2) {
                    bbox.x2 = p.x;
                }
                if (p.y < bbox.y1) {
                    bbox.y1 = p.y;
                }
                if (p.y > bbox.y2) {
                    bbox.y2 = p.y;
                }
            }
            
            
            
            for (BezierCPs::iterator it = simpleClosedCurve.begin(); it != simpleClosedCurve.end(); ++it) {
                
                bool nextIsPassedEnd = false;
                if (next == simpleClosedCurve.end()) {
                    next = simpleClosedCurve.begin();
                    nextIsPassedEnd = true;
                }
                
                //mid-point of the line segment between points i and i + n
                Point nextPoint,curPoint;
                (*it)->getPositionAtTime(false,time, &curPoint.x, &curPoint.y);
                (*next)->getPositionAtTime(false,time, &nextPoint.x, &nextPoint.y);
                
                /*
                 * Compute the number of intersections between the current line segment [it,next] and all other line segments
                 * If the number of intersections is different of 2, ignore this segment.
                 */
                QLineF line(QPointF(curPoint.x,curPoint.y),QPointF(nextPoint.x,nextPoint.y));
                std::set<QPointF,qpointf_compare_less> intersections;
                std::list<Point>::const_iterator last_pt = polygon.begin();
                std::list<Point>::const_iterator cur = last_pt;
                ++cur;
                QPointF intersectionPoint;
                for (; cur != polygon.end(); ++cur, ++last_pt) {
                    QLineF polygonSegment(QPointF(last_pt->x,last_pt->y),QPointF(cur->x,cur->y));
                    if (line.intersect(polygonSegment, &intersectionPoint) == QLineF::BoundedIntersection) {
                        intersections.insert(intersectionPoint);
                    }
                    if (intersections.size() > 2) {
                        break;
                    }
                }
                
                if (intersections.size() != 2) {
                    continue;
                }
                
                /*
                 * Check if the midpoint of the line segment [it,next] lies inside the simple closed curve (polygon), otherwise
                 * ignore it.
                 */
                Point midPoint;
                midPoint.x = (nextPoint.x + curPoint.x) / 2.;
                midPoint.y = (nextPoint.y + curPoint.y) / 2.;
                bool isInside = pointInPolygon(midPoint,polygon,bbox,Bezier::eFillRuleWinding);

                if (isInside) {
                    
                    //Make the sub closed curve composed of the path from points i to i + n
                    BezierCPs subCurve;
                    subCurve.push_back(*it);
                    BezierCPs::iterator pointIt = it;
                    for (int i = 0; i < n - 1; ++i) {
                        ++pointIt;
                        if (pointIt == simpleClosedCurve.end()) {
                            pointIt = simpleClosedCurve.begin();
                        }
                        subCurve.push_back(*pointIt);
                    }
                    subCurve.push_back(*next);
                    
                    // Ensure that all interior angles are less than 180 degrees.
                    
                    
                    
                    patches->push_back(subCurve);
                    
                    //Remove i + 1 to i + n
                    BezierCPs::iterator eraseStart = it;
                    ++eraseStart;
                    bool eraseStartIsPassedEnd = false;
                    if (eraseStart == simpleClosedCurve.end()) {
                        eraseStart = simpleClosedCurve.begin();
                        eraseStartIsPassedEnd = true;
                    }
                    //"it" is  invalidated after the next instructions but we leave the loop anyway
                    assert(!simpleClosedCurve.empty());
                    if ((!nextIsPassedEnd && !eraseStartIsPassedEnd) || (nextIsPassedEnd && eraseStartIsPassedEnd)) {
                        simpleClosedCurve.erase(eraseStart,next);
                    } else {
                        simpleClosedCurve.erase(eraseStart,simpleClosedCurve.end());
                        if (!simpleClosedCurve.empty()) {
                            simpleClosedCurve.erase(simpleClosedCurve.begin(),next);
                        }
                    }
                    found = true;
                    break;
                }

                // increment for next iteration
                if (next != simpleClosedCurve.end()) {
                    ++next;
                }
            } // for(it)
            if (found) {
                break;
            }
        } // for(n)
        
        if (!found) {
            BezierCPs subdivisedCurve;
            //Subdivise the curve at the midpoint of each segment
            BezierCPs::iterator next = simpleClosedCurve.begin();
            if (next != simpleClosedCurve.end()) {
                ++next;
            }
            for (BezierCPs::iterator it = simpleClosedCurve.begin(); it != simpleClosedCurve.end(); ++it) {
                
                if (next == simpleClosedCurve.end()) {
                    next = simpleClosedCurve.begin();
                }
                Point p0,p1,p2,p3,p0p1, p1p2, p2p3, p0p1_p1p2, p1p2_p2p3,dest;
                (*it)->getPositionAtTime(false,time, &p0.x, &p0.y);
                (*it)->getRightBezierPointAtTime(false,time, &p1.x, &p1.y);
                (*next)->getLeftBezierPointAtTime(false,time, &p2.x, &p2.y);
                (*next)->getPositionAtTime(false,time, &p3.x, &p3.y);
                bezierFullPoint(p0, p1, p2, p3, 0.5, &p0p1, &p1p2, &p2p3, &p0p1_p1p2, &p1p2_p2p3, &dest);
                boost::shared_ptr<BezierCP> controlPoint(new BezierCP);
                controlPoint->setStaticPosition(false,dest.x, dest.y);
                controlPoint->setLeftBezierStaticPosition(false,p0p1_p1p2.x, p0p1_p1p2.y);
                controlPoint->setRightBezierStaticPosition(false,p1p2_p2p3.x, p1p2_p2p3.y);
                subdivisedCurve.push_back(*it);
                subdivisedCurve.push_back(controlPoint);

                // increment for next iteration
                if (next != simpleClosedCurve.end()) {
                    ++next;
                }
            } // for()
            simpleClosedCurve = subdivisedCurve;
        }
    }
    if (!simpleClosedCurve.empty()) {
        assert(simpleClosedCurve.size() >= 2);
        patches->push_back(simpleClosedCurve);
    }
}

void
RotoContextPrivate::applyAndDestroyMask(cairo_t* cr,
                                        cairo_pattern_t* mesh)
{
    assert(cairo_pattern_status(mesh) == CAIRO_STATUS_SUCCESS);
    cairo_set_source(cr, mesh);

    ///paint with the feather with the pattern as a mask
    cairo_mask(cr, mesh);

    cairo_pattern_destroy(mesh);
}

void
RotoContext::changeItemScriptName(const std::string& oldFullyQualifiedName,const std::string& newFullyQUalifiedName)
{
    std::string appID = getNode()->getApp()->getAppIDString();
    std::string nodeName = appID + "." + getNode()->getFullyQualifiedName();
    std::string err;
    
    std::string declStr = nodeName + ".roto." + newFullyQUalifiedName + " = " + nodeName + ".roto." + oldFullyQualifiedName + "\n";
    std::string delStr = "del " + nodeName + ".roto." + oldFullyQualifiedName + "\n";
    std::string script = declStr + delStr;
    if (!appPTR->isBackground()) {
        getNode()->getApp()->printAutoDeclaredVariable(script);
    }
    if (!Natron::interpretPythonScript(script , &err, 0)) {
        getNode()->getApp()->appendToScriptEditor(err);
    }
}

void
RotoContext::removeItemAsPythonField(const boost::shared_ptr<RotoItem>& item)
{
    std::string appID = getNode()->getApp()->getAppIDString();
    std::string nodeName = appID + "." + getNode()->getFullyQualifiedName();
    std::string err;
    std::string script = "del " + nodeName + ".roto." + item->getFullyQualifiedName() + "\n";
    if (!appPTR->isBackground()) {
        getNode()->getApp()->printAutoDeclaredVariable(script);
    }
    if (!Natron::interpretPythonScript(script , &err, 0)) {
        getNode()->getApp()->appendToScriptEditor(err);
    }
    
}

static void refreshLayerRotoPaintTree(RotoLayer* layer)
{
    const RotoItems& items = layer->getItems();
    for (RotoItems::const_iterator it = items.begin(); it!=items.end(); ++it) {
        RotoLayer* isLayer = dynamic_cast<RotoLayer*>(it->get());
        RotoDrawableItem* isDrawable = dynamic_cast<RotoDrawableItem*>(it->get());
        if (isLayer) {
            refreshLayerRotoPaintTree(isLayer);
        } else if (isDrawable) {
            isDrawable->refreshNodesConnections();
        }
    }
}

void
RotoContext::refreshRotoPaintTree()
{
    if (!_imp->isPaintNode || _imp->isCurrentlyLoading) {
        return;
    }
    boost::shared_ptr<RotoLayer> layer;
    {
        QMutexLocker k(&_imp->rotoContextMutex);
        if (_imp->layers.empty()) {
            return;
        }
        layer = _imp->layers.front();
    }
    refreshLayerRotoPaintTree(layer.get());
}

void
RotoContext::onRotoPaintInputChanged(const boost::shared_ptr<Natron::Node>& node)
{
    assert(_imp->isPaintNode);
    
    if (node) {
        
    }
    refreshRotoPaintTree();
}

void
RotoContext::declareItemAsPythonField(const boost::shared_ptr<RotoItem>& item)
{
    std::string appID = getNode()->getApp()->getAppIDString();
    std::string nodeName = appID + "." + getNode()->getFullyQualifiedName();
    RotoLayer* isLayer = dynamic_cast<RotoLayer*>(item.get());
    
    std::string err;
    std::string script = nodeName + ".roto." + item->getFullyQualifiedName() + " = " +
    nodeName + ".roto.getItemByName(\"" + item->getScriptName() + "\")\n";
    if (!appPTR->isBackground()) {
        getNode()->getApp()->printAutoDeclaredVariable(script);
    }
    if(!Natron::interpretPythonScript(script , &err, 0)) {
        getNode()->getApp()->appendToScriptEditor(err);
    }
    
    if (isLayer) {
        const RotoItems& items = isLayer->getItems();
        for (RotoItems::const_iterator it = items.begin(); it != items.end(); ++it) {
            declareItemAsPythonField(*it);
        }
    }
}

void
RotoContext::declarePythonFields()
{
    for (std::list< boost::shared_ptr<RotoLayer> >::iterator it = _imp->layers.begin(); it != _imp->layers.end(); ++it) {
        declareItemAsPythonField(*it);
    }
}
