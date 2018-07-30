/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
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

#include "KeyFrameInterpolator.h"

#include "Engine/Interpolation.h"


#include <ofxParam.h>
#include <ofxhPropertySuite.h> // PropSpec

NATRON_NAMESPACE_ENTER

KeyFrameInterpolator::KeyFrameInterpolator()
{

}

KeyFrameInterpolatorPtr
KeyFrameInterpolator::createCopy() const
{
    return KeyFrameInterpolatorPtr(new KeyFrameInterpolator);
}


KeyFrameInterpolator::~KeyFrameInterpolator()
{

}

void
KeyFrameInterpolator::ensureIteratorInPeriod(TimeValue xMin,
                                             TimeValue xMax,
                                             const KeyFrameSet& keyFrames,
                                             TimeValue *t,
                                             KeyFrameSet::const_iterator *itup)
{
    const double period = xMax - xMin;

    double minKeyFrameX = keyFrames.begin()->getTime() + xMin;
    assert(xMin < xMax);
    if (*t < minKeyFrameX || *t > minKeyFrameX + period) {
        // This will bring t either in minTime <= t <= maxTime or t in the range minTime - (maxTime - minTime) < t < minTime
        *t = TimeValue(std::fmod(*t - minKeyFrameX, period ) + minKeyFrameX);
        if (*t < minKeyFrameX) {
            *t = TimeValue(*t + period);
        }
        assert(*t >= minKeyFrameX && *t <= minKeyFrameX + period);
    }
    *itup = keyFrames.upper_bound(KeyFrame(*t, 0.));

}

void
KeyFrameInterpolator::interParams(const KeyFrameSet &keyFrames,
                                  bool isPeriodic,
                                  double xMin,
                                  double xMax,
                                  TimeValue *t,
                                  KeyFrameSet::const_iterator itup,
                                  KeyFrame* kCur,
                                  KeyFrame* kNext)
{

    assert(keyFrames.size() >= 1);
    assert( itup == keyFrames.end() || *t < itup->getTime() );
    double period = xMax - xMin;

    if (isPeriodic) {
        ensureIteratorInPeriod(TimeValue(xMin), TimeValue(xMax), keyFrames, t, &itup);
    }

    if ( itup == keyFrames.begin() ) {
        // We are in the case where all keys have a greater time
        *kNext = *itup;

        // If periodic, we are in between xMin and the first keyframe
        if (isPeriodic) {
            KeyFrameSet::const_reverse_iterator last =  keyFrames.rbegin();
            *kCur = *last;
            kCur->setTime(TimeValue(last->getTime() - period));
        } else {
            kCur->cloneProperties(*kNext);
            kCur->setTime(TimeValue(kNext->getTime() - 1.));
            kCur->setRightDerivative(0.);
            kCur->setInterpolation(eKeyframeTypeNone);
        }

    } else if ( itup == keyFrames.end() ) {

        // We are in the case where no key has a greater time
        // If periodic, we are in-between the last keyframe and xMax
        if (isPeriodic) {
            KeyFrameSet::const_iterator next = keyFrames.begin();
            KeyFrameSet::const_reverse_iterator prev = keyFrames.rbegin();
            *kCur = *prev;
            *kNext = *next;
            kNext->setTime(TimeValue(next->getTime() + period));
        } else {
            KeyFrameSet::const_reverse_iterator itlast = keyFrames.rbegin();
            *kCur = *itlast;
            kNext->cloneProperties(*kCur);
            kNext->setTime(TimeValue(kCur->getTime() + 1.));
            kNext->setLeftDerivative(0.);
            kNext->setInterpolation(eKeyframeTypeNone);
        }

    } else {
        // between two keyframes
        // get the last keyframe with time <= t
        KeyFrameSet::const_iterator itcur = itup;
        --itcur;
        assert(itcur->getTime() <= *t);
        *kCur = *itcur;
        *kNext = *itup;
    }
} // interParams

KeyFrame
KeyFrameInterpolator::interpolate(TimeValue t,
                                  KeyFrameSet::const_iterator itup,
                                  const KeyFrameSet& keyframes,
                                  const bool isPeriodic,
                                  TimeValue xmin,
                                  TimeValue xmax)
{

    KeyFrame keyCur, keyNext;

    interParams(keyframes,
                isPeriodic,
                xmin,
                xmax,
                &t,
                itup,
                &keyCur, &keyNext);

    double v = Interpolation::interpolate(keyCur.getTime(), keyCur.getValue(),
                                          keyCur.getRightDerivative(),
                                          keyNext.getLeftDerivative(),
                                          keyNext.getTime(), keyNext.getValue(),
                                          t,
                                          keyCur.getInterpolation(),
                                          keyNext.getInterpolation());


    // Properties don't follow an interpolation unlike the value of the keyframe, thus copy the properties from P0
    keyCur.setTime(t);
    keyCur.setValue(v);
    return keyCur;

} // interpolate


OfxCustomStringKeyFrameInterpolator::OfxCustomStringKeyFrameInterpolator(customParamInterpolationV1Entry_t f, const void* handleRaw, const std::string& paramName)
: KeyFrameInterpolator()
, _handleRaw(handleRaw)
, _paramName(paramName)
, _f(f)
{

}

OfxCustomStringKeyFrameInterpolator::~OfxCustomStringKeyFrameInterpolator()
{

}

KeyFrameInterpolatorPtr OfxCustomStringKeyFrameInterpolator::createCopy() const
{
    return boost::shared_ptr<OfxCustomStringKeyFrameInterpolator>(new OfxCustomStringKeyFrameInterpolator(_f, _handleRaw, _paramName));
} // createCopy

KeyFrame
OfxCustomStringKeyFrameInterpolator::interpolate(TimeValue t,
                                                 KeyFrameSet::const_iterator itup,
                                                 const KeyFrameSet& keyframes,
                                                 const bool isPeriodic,
                                                 TimeValue xmin,
                                                 TimeValue xmax)
{

    if (isPeriodic) {
        ensureIteratorInPeriod(xmin, xmax, keyframes, &t, &itup);
    }

    if ( itup == keyframes.end() ) {
        // If the time is greater than the time of all keyframes return the last
        --itup;
        return *itup;
    } else if ( itup == keyframes.begin() ) {
        // if the time is lesser than the time of all keyframes, return the first
        return *itup;
    }

    // If we are on a keyframe, return it
    if (itup->getTime() == t) {
        return *itup;
    }

    // general case, we're in-between 2 keyframes
    KeyFrameSet::const_iterator lower = itup;
    --lower;


    OFX::Host::Property::PropSpec inArgsSpec[] = {
        { kOfxPropName,    OFX::Host::Property::eString, 1, true, "" },
        { kOfxPropTime,    OFX::Host::Property::eDouble, 1, true, "" },
        { kOfxParamPropCustomValue,    OFX::Host::Property::eString, 2, true, ""},
        { kOfxParamPropInterpolationTime,    OFX::Host::Property::eDouble, 2, true, "" },
        { kOfxParamPropInterpolationAmount,    OFX::Host::Property::eDouble, 1, true, "" },
        OFX::Host::Property::propSpecEnd
    };
    OFX::Host::Property::Set inArgs(inArgsSpec);
    inArgs.setStringProperty( kOfxPropName, _paramName );
    inArgs.setDoubleProperty(kOfxPropTime, t);

    std::string s1, s2;
    lower->getPropertySafe(kKeyFramePropString, 0, &s1);
    itup->getPropertySafe(kKeyFramePropString, 0, &s2);
    inArgs.setStringProperty(kOfxParamPropCustomValue, s1, 0);
    inArgs.setStringProperty(kOfxParamPropCustomValue, s2, 1);
    inArgs.setDoubleProperty(kOfxParamPropInterpolationTime, lower->getTime(), 0);
    inArgs.setDoubleProperty(kOfxParamPropInterpolationTime, itup->getTime(), 1);
    inArgs.setDoubleProperty( kOfxParamPropInterpolationAmount, (t - lower->getTime()) / (double)(itup->getTime() - lower->getTime()) );


    OFX::Host::Property::PropSpec outArgsSpec[] = {
        { kOfxParamPropCustomValue,    OFX::Host::Property::eString, 1, false, ""},
        OFX::Host::Property::propSpecEnd
    };
    OFX::Host::Property::Set outArgs(outArgsSpec);

    _f( _handleRaw, inArgs.getHandle(), outArgs.getHandle() );

    std::string value = outArgs.getStringProperty(kOfxParamPropCustomValue, 0).c_str();

    KeyFrame ret(t, 0.);
    ret.setProperty(kKeyFramePropString, value);
    return ret;

} // interpolate

NATRON_NAMESPACE_EXIT
