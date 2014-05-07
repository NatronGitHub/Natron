//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */

#ifndef ROTOCONTEXTPRIVATE_H
#define ROTOCONTEXTPRIVATE_H

#include <list>
#include <boost/shared_ptr.hpp>

#include <QMutex>
#include <QCoreApplication>
#include <QThread>
#include "Engine/Curve.h"
#include "Engine/Rect.h"
#include "Global/GlobalDefines.h"

class Bezier;

struct BezierCPPrivate
{
    Bezier* holder;
    
    ///the animation curves for the position in the 2D plane
    Curve curveX,curveY;
    double x,y; //< used when there is no keyframe
    
    ///the animation curves for the derivatives
    Curve curveLeftBezierX,curveRightBezierX,curveLeftBezierY,curveRightBezierY;
    double leftX,rightX,leftY,rightY; //< used when there is no keyframe
    
    BezierCPPrivate(Bezier* curve)
    : holder(curve)
    , curveX()
    , curveY()
    , x(0)
    , y(0)
    , curveLeftBezierX()
    , curveRightBezierX()
    , curveLeftBezierY()
    , curveRightBezierY()
    , leftX(0)
    , rightX(0)
    , leftY(0)
    , rightY(0)
    {
        
    }
    
};

class BezierCP;
typedef std::list< boost::shared_ptr<BezierCP> > BezierCPs;


struct BezierPrivate
{
    RotoContext* context;
    
    BezierCPs points; //< the control points of the curve
    BezierCPs featherPoints; //< the feather points, the number of feather points must equal the number of cp.
    bool finished; //< when finished is true, the last point of the list is connected to the first point of the list.
    
    bool activated;//< should the curve be visible/rendered ?
    
    mutable QMutex splineMutex;
    
    RectD boundingBox; //< the bounding box of the bezier
    bool isBoundingBoxValid; //< has the bounding box ever been computed ?
    
    
    BezierPrivate(RotoContext* ctx)
    : context(ctx)
    , points()
    , featherPoints()
    , finished(false)
    , activated(true)
    , splineMutex()
    , boundingBox()
    , isBoundingBoxValid(false)
    {
    }
    
    bool hasKeyframeAtTime(int time) const
    {
        // PRIVATE - should not lock
        assert(!splineMutex.tryLock());
        if (points.empty()) {
            return false;
        } else {
            KeyFrame k;
            return points.front()->hasKeyFrameAtTime(time);
        }
    }
    
    bool hasKeyframeAtCurrentTime() const
    {
        return hasKeyframeAtTime(context->getTimelineCurrentTime());
    }
    
    void getKeyframeTimes(std::set<int>* times) const
    {
        // PRIVATE - should not lock
        assert(!splineMutex.tryLock());
        if (points.empty()) {
            return ;
        }
        points.front()->getKeyframeTimes(times);
    }
    
    BezierCPs::const_iterator atIndex(int index) const
    {
        // PRIVATE - should not lock
        assert(!splineMutex.tryLock());
        if (index >= (int)points.size()) {
            throw std::out_of_range("RotoSpline::atIndex: non-existent control point");
        }
        
        BezierCPs::const_iterator it = points.begin();
        std::advance(it, index);
        return it;
    }
    
    BezierCPs::iterator atIndex(int index)
    {
        // PRIVATE - should not lock
        assert(!splineMutex.tryLock());
        if (index >= (int)points.size()) {
            throw std::out_of_range("RotoSpline::atIndex: non-existent control point");
        }
        
        BezierCPs::iterator it = points.begin();
        std::advance(it, index);
        return it;
    }
    
};

struct RotoContextPrivate
{
    
    mutable QMutex rotoContextMutex;
    std::list< boost::shared_ptr<Bezier> > splines;
    bool autoKeying;
    bool rippleEdit;
    bool featherLink;
    
    Natron::Node* node;
    U64 age;
    
    RotoContextPrivate(Natron::Node* n )
    : rotoContextMutex()
    , splines()
    , autoKeying(true)
    , rippleEdit(false)
    , featherLink(true)
    , node(n)
    , age(0)
    {
        
    }
    
    /**
     * @brief Call this after any change to notify the mask has changed for the cache.
     **/
    void incrementRotoAge()
    {
        ///MT-safe: only called on the main-thread
        assert(QThread::currentThread() == qApp->thread());
        
        QMutexLocker l(&rotoContextMutex);
        ++age;
        
    }
};


#endif // ROTOCONTEXTPRIVATE_H
