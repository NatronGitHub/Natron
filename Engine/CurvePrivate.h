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
#ifndef NATRON_ENGINE_CURVEPRIVATE_H_
#define NATRON_ENGINE_CURVEPRIVATE_H_

#include <boost/shared_ptr.hpp>

#include "Engine/Rect.h"
#include "Engine/Variant.h"
#include "Engine/Knob.h"

class Curve;
struct KeyFramePrivate{

    Variant _value; /// the value held by the key
    double _time; /// a value ranging between 0 and 1

    Variant _leftTangent,_rightTangent;
    Natron::KeyframeType _interpolation;


    KeyFramePrivate()
    : _value()
    , _time(0)
    , _interpolation(Natron::KEYFRAME_LINEAR)
    {}


    KeyFramePrivate(double time, const Variant& initialValue)
        : _value(initialValue)
        , _time(time)
        , _leftTangent()
        , _rightTangent()
        , _interpolation(Natron::KEYFRAME_SMOOTH)
    {
        _leftTangent = initialValue;
        _rightTangent = initialValue;
    }

    KeyFramePrivate(const KeyFramePrivate& other)
    {
        _value = other._value;
        _leftTangent = other._leftTangent;
        _rightTangent = other._rightTangent;
        _interpolation = other._interpolation;
        _time = other._time;
    }


};


class KeyFrame;
class Knob;
struct CurvePrivate{

   

    std::list< boost::shared_ptr<KeyFrame> >  _keyFrames;
    Knob* _owner;

    
    CurvePrivate()
    : _keyFrames()
    , _owner(NULL)
    {}

};




#endif // NATRON_ENGINE_CURVEPRIVATE_H_
