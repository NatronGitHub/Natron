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
class KeyFrame;
class Knob;

struct CurvePrivate{
    
    enum CurveType{
        DOUBLE_CURVE = 0, //< the values held by the keyframes can be any real
        INT_CURVE, //< the values held by the keyframes can only be integers
        BOOL_CURVE, //< the values held by the keyframes can be either 0 or 1
        STRING_CURVE //< the values held by the keyframes can only be integers and keyframes are ordered by increasing values
                     // and times
    };

    
    struct KeyFrame_compare_time {
        bool operator() (const boost::shared_ptr<KeyFrame>& lhs, const boost::shared_ptr<KeyFrame>& rhs) const {
            return lhs->getTime() < rhs->getTime();
        }
    };

    KeyFrameSet keyFrames;

    Knob* owner;

    CurveType curveType;
    bool mustSetCurveType; //< if true the first call to addKeyFrame will set the curveType member
    
    double curveMin,curveMax;
        
    CurvePrivate()
    : keyFrames()
    , owner(NULL)
    , curveType(DOUBLE_CURVE)
    , mustSetCurveType(true)
    , curveMin(INT_MIN)
    , curveMax(INT_MAX)
    {}
};




#endif // NATRON_ENGINE_CURVEPRIVATE_H_
