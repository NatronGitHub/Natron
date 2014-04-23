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

#ifndef KNOBROTO_H
#define KNOBROTO_H

#include <boost/scoped_ptr.hpp>



/**
 * @class A ControlPoint is an animated control point of a roto spline
 **/
class ControlPoint
{
  
public:
    
    ControlPoint();
    
    ~ControlPoint();
    
private:
    
    struct ControlPointPrivate;
    boost::scoped_ptr<ControlPointPrivate> _imp;
};


/**
 * @class This class represents a roto spline where the curve passes by each control points.
 * Note that the roto spline also supports feather points.
 **/

class RotoSpline
{
    
public:
    
    RotoSpline();
    
    ~RotoSpline();
    
private:
    
    struct RotoSplinePrivate;
    boost::scoped_ptr<RotoSplinePrivate> _imp;
};



/**
 * @class This class is a member of all effects instantiated in the context "paint". It describes internally
 * all the splines data structures and their state.
 **/
class RotoContext
{
public:

    RotoContext();
    
    virtual ~RotoContext();
private:
    
    struct RotoContextPrivate;
    boost::scoped_ptr<RotoContextPrivate> _imp;
};

#endif // KNOBROTO_H
