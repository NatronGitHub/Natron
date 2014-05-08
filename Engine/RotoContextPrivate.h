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
    
    boost::shared_ptr<Double_Knob> opacity; //< opacity of the rendered shape between 0 and 1
    boost::shared_ptr<Int_Knob> feather;//< number of pixels to add to the feather distance (from the feather point), between -100 and 100
    boost::shared_ptr<Double_Knob> featherFallOff; //< the rate of fall-off for the feather, between 0 and 1,  0.5 meaning the
                                                   //alpha value is half the original value when at half distance from the feather distance
    boost::shared_ptr<Bool_Knob> activated; //< should the curve be visible/rendered ? (animable)
    boost::shared_ptr<Bool_Knob> inverted; //< invert the rendering
    
    BezierPrivate(RotoContext* ctx)
    : context(ctx)
    , points()
    , featherPoints()
    , finished(false)
    , splineMutex()
    , opacity(new Double_Knob(NULL,"Opacity",1))
    , feather(new Int_Knob(NULL,"Feather",1))
    , featherFallOff(new Double_Knob(NULL,"Feather fall-off",1))
    , activated(new Bool_Knob(NULL,"Activated",1))
    , inverted(new Bool_Knob(NULL,"Inverted",1))
    {
        opacity->populate();
        opacity->setDefaultValue(1.);
        feather->populate();
        feather->setDefaultValue(0);
        featherFallOff->populate();
        featherFallOff->setDefaultValue(0.5);
        activated->populate();
        activated->setDefaultValue(true);
        inverted->populate();
        inverted->setDefaultValue(false);
        overlayColor[0] = 0.85164;
        overlayColor[1] = 0.196936;
        overlayColor[2] = 0.196936;
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
    boost::shared_ptr<Bool_Knob> inverted;
    
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
        opacity->setHintToolTip("Controls the opacity of the selected shape(s).");
        opacity->setMinimum(0.);
        opacity->setMaximum(1.);
        opacity->setDefaultValue(1.);
        opacity->setAllDimensionsEnabled(false);
        feather = Natron::createKnob<Int_Knob>(effect,"Feather");
        feather->setHintToolTip("Controls the distance of feather (in pixels) to add around the selected shape(s)");
        feather->setMinimum(-100);
        feather->setMaximum(100);
        feather->setDefaultValue(0);
        feather->setAllDimensionsEnabled(false);
        featherFallOff = Natron::createKnob<Double_Knob>(effect, "Feather fall-off");
        featherFallOff->setHintToolTip("Controls the rate at which the feather is applied on the selected shape(s)."
                                       " 0 means no fading (i.e: 1's only), 1 means sharp fading (i.e: 0's only)");
        featherFallOff->setMinimum(0.);
        featherFallOff->setMaximum(1.);
        featherFallOff->setDefaultValue(0.5);
        featherFallOff->setAllDimensionsEnabled(false);
        activated = Natron::createKnob<Bool_Knob>(effect,"Activated");
        activated->setHintToolTip("Controls whether the selected shape(s) should be visible and rendered or not."
                                  "Note that you can animate this parameter so you can activate/deactive the shape "
                                  "throughout the time.");
        activated->turnOffNewLine();
        activated->setDefaultValue(true);
        activated->setAllDimensionsEnabled(false);
        inverted = Natron::createKnob<Bool_Knob>(effect, "Inverted");
        inverted->setHintToolTip("Controls whether the selected shape(s) should be inverted. When inverted everything "
                                 "outside the shape will be set to 1 and everything inside the shape will be set to 0.");
        inverted->setDefaultValue(false);
        inverted->setAllDimensionsEnabled(false);
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
