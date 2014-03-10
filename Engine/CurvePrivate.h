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
#include <QMutex>

#include "Engine/Rect.h"
#include "Engine/Variant.h"
#include "Engine/Knob.h"
#include "Engine/KnobTypes.h"
#include "Engine/KnobFile.h"

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
    
    double xMin, xMax;
    double yMin, yMax;
    bool hasYRange;
    QMutex _lock; //< the plug-ins can call getValueAt at any moment and we must make sure the user is not playing around
    
    
    CurvePrivate()
    : keyFrames()
    , owner(NULL)
    , xMin(INT_MIN)
    , xMax(INT_MAX)
    , yMin(INT_MIN)
    , yMax(INT_MAX)
    , hasYRange(false)
    , _lock(QMutex::Recursive)
    {}
    
    CurvePrivate::CurveType getCurveType() const {
        assert(owner);
        if (owner->typeName() == Int_Knob::typeNameStatic() ||
             owner->typeName() == Choice_Knob::typeNameStatic()) {
             return CurvePrivate::INT_CURVE;
        } else if (owner->typeName() == String_Knob::typeNameStatic() ||
                   owner->typeName() == File_Knob::typeNameStatic()) {
            return CurvePrivate::STRING_CURVE;
        } else if (owner->typeName() == Bool_Knob::typeNameStatic()) {
            return CurvePrivate::BOOL_CURVE;
        } else {
            return CurvePrivate::DOUBLE_CURVE;
        }
    }
};




#endif // NATRON_ENGINE_CURVEPRIVATE_H_
