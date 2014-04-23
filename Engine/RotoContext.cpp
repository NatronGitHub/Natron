//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */

#include "RotoContext.h"

#include <list>
#include "Engine/Curve.h"

////////////////////////////////////ControlPoint////////////////////////////////////

struct ControlPoint::ControlPointPrivate
{
    ///the coordinates of the point in the 2D plane
    double x,y;
    
    ///the derivates of x and y respectively on the left and on the right of the point
	double leftDerivX,leftDerivY,rightDerivX,rightDerivY;
    
    ///the animation curve for all the 6 doubles above
    Curve curveX,curveY;
    Curve curveLeftDerivX,curveLeftDerivY,curveRightDerivX,curveRightDerivY;
    
    ControlPointPrivate()
    : x(0)
    , y(0)
    , leftDerivX(0)
    , leftDerivY(0)
    , rightDerivX(0)
    , rightDerivY(0)
    , curveX()
    , curveY()
    , curveLeftDerivX()
    , curveLeftDerivY()
    , curveRightDerivX()
    , curveRightDerivY()
    {
        
    }
};

ControlPoint::ControlPoint()
: _imp(new ControlPointPrivate())
{
    
}

ControlPoint::~ControlPoint()
{
    
}

////////////////////////////////////RotoSpline////////////////////////////////////
struct RotoSpline::RotoSplinePrivate
{
    std::list<ControlPoint> points; //< the control points of the curve
    std::list<ControlPoint> featherPoints; //< the feather points, the number of feather points must equal the number of cp.
    bool finished; //< when finished is true, the last point of the list is connected to the first point of the list.
    
    RotoSplinePrivate()
    : points()
    , featherPoints()
    , finished(false)
    {
    }
    
};


RotoSpline::RotoSpline()
: _imp(new RotoSplinePrivate())
{
    
}

RotoSpline::~RotoSpline()
{
    
}

////////////////////////////////////RotoContext////////////////////////////////////

struct RotoContext::RotoContextPrivate
{
    std::list<RotoSpline> _splines;
    
    RotoContextPrivate()
    {
        
    }
};

RotoContext::RotoContext()
: _imp(new RotoContextPrivate())
{
}

RotoContext::~RotoContext()
{
    
}
