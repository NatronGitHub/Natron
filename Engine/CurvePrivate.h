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

    Variant value; /// the value held by the key
    double time; /// a value ranging between 0 and 1

    Variant leftTangent,rightTangent;
    Natron::KeyframeType interpolation;


    KeyFramePrivate()
    : value()
    , time(0)
    , interpolation(Natron::KEYFRAME_LINEAR)
    {}


    KeyFramePrivate(double time, const Variant& initialValue)
        : value(initialValue)
        , time(time)
        , leftTangent()
        , rightTangent()
        , interpolation(Natron::KEYFRAME_SMOOTH)
    {
        leftTangent = initialValue;
        rightTangent = initialValue;
    }

    KeyFramePrivate(const KeyFramePrivate& other)
    {
        value = other.value;
        leftTangent = other.leftTangent;
        rightTangent = other.rightTangent;
        interpolation = other.interpolation;
        time = other.time;
    }


};


class KeyFrame;
class Knob;

struct CurvePrivate{
    struct KeyFrame_compare_time {
        bool operator() (const boost::shared_ptr<KeyFrame>& lhs, const boost::shared_ptr<KeyFrame>& rhs) const {
            return lhs->getTime() < rhs->getTime();
        }
    };

    KeyFrameSet keyFrames;

    Knob* owner;

    
    CurvePrivate()
    : keyFrames()
    , owner(NULL)
    {}

};




#endif // NATRON_ENGINE_CURVEPRIVATE_H_
