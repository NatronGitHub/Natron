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
#include "Engine/KnobTypes.h"
#include "Engine/Node.h"
#include "Engine/EffectInstance.h"
#include "Engine/AppManager.h"

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
 
    double overlayColor[4]; //< the color the shape overlay should be drawn with, defaults to smooth red
    
    mutable QMutex splineMutex;
    
    RectD boundingBox; //< the bounding box of the bezier
    bool isBoundingBoxValid; //< has the bounding box ever been computed ?
    
    boost::shared_ptr<Double_Knob> opacity; //< opacity of the rendered shape between 0 and 1
    boost::shared_ptr<Int_Knob> feather;//< number of pixels to add to the feather distance (from the feather point), between -100 and 100
    boost::shared_ptr<Double_Knob> featherFallOff; //< the rate of fall-off for the feather, between 0 and 1,  0.5 meaning the
                                                   //alpha value is half the original value when at half distance from the feather distance
    boost::shared_ptr<Bool_Knob> activated; //< should the curve be visible/rendered ? (animable)
    
    BezierPrivate(RotoContext* ctx)
    : context(ctx)
    , points()
    , featherPoints()
    , finished(false)
    , splineMutex()
    , boundingBox()
    , isBoundingBoxValid(false)
    , opacity(new Double_Knob(NULL,"Opacity",1))
    , feather(new Int_Knob(NULL,"Feather",1))
    , featherFallOff(new Double_Knob(NULL,"Feather fall-off",1))
    , activated(new Bool_Knob(NULL,"Activated",1))
    {
        opacity->populate();
        opacity->setDefaultValue(1.);
        feather->populate();
        feather->setDefaultValue(0);
        featherFallOff->populate();
        featherFallOff->setDefaultValue(0.5);
        activated->populate();
        activated->setDefaultValue(true);
        overlayColor[0] = 0.85;
        overlayColor[1] = 0.67;
        overlayColor[2] = 0.;
        overlayColor[3] = 1.;
        
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
    
    ///These are knobs that take the value of the selected splines infos.
    ///Their value changes when selection changes.
    boost::shared_ptr<Double_Knob> opacity;
    boost::shared_ptr<Int_Knob> feather;
    boost::shared_ptr<Double_Knob> featherFallOff;
    boost::shared_ptr<Bool_Knob> activated; //<allows to disable a shape on a specific frame range
    
    ///This keeps track  of the bezier linked to the context knobs
    std::list<boost::shared_ptr<Bezier> > selectedBeziers;
    
    RotoContextPrivate(Natron::Node* n )
    : rotoContextMutex()
    , splines()
    , autoKeying(true)
    , rippleEdit(false)
    , featherLink(true)
    , node(n)
    , age(0)
    {
        assert(n && n->getLiveInstance());
        Natron::EffectInstance* effect = n->getLiveInstance();
        opacity = Natron::createKnob<Double_Knob>(effect, "Opacity");
        opacity->setMinimum(0.);
        opacity->setMaximum(1.);
        opacity->setDefaultValue(1.);
        feather = Natron::createKnob<Int_Knob>(effect,"Feather");
        feather->setMinimum(-100);
        feather->setMaximum(100);
        feather->setDefaultValue(0);
        featherFallOff = Natron::createKnob<Double_Knob>(effect, "Feather fall-off");
        featherFallOff->setMinimum(0.);
        featherFallOff->setMaximum(1.);
        featherFallOff->setDefaultValue(0.5);
        activated = Natron::createKnob<Bool_Knob>(effect,"Activated");
        activated->setDefaultValue(true);
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
