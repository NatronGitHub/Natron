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


#ifndef KEYFRAMEINTERPOLATOR_H
#define KEYFRAMEINTERPOLATOR_H


// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****


#include "Engine/Curve.h"

NATRON_NAMESPACE_ENTER

class KeyFrameInterpolator
{
public:

    KeyFrameInterpolator();


    virtual ~KeyFrameInterpolator();

    virtual KeyFrameInterpolatorPtr createCopy() const;

    /**
     * @brief For a periodic curve, ensure t and the iterator point to keyframes in the periodic range
     **/
    static void ensureIteratorInPeriod(TimeValue xmin,
                                       TimeValue xmax,
                                       const KeyFrameSet& keyframes,
                                       TimeValue *t,
                                       KeyFrameSet::const_iterator *itup);

    /**
     * @brief Compute interpolation parameters from keyframes and an iterator
     * to the next keyframe (the first with time > t)
     **/
    static void interParams(const KeyFrameSet &keyFrames,
                            bool isPeriodic,
                            double xMin,
                            double xMax,
                            TimeValue *t,
                            KeyFrameSet::const_iterator itup,
                            KeyFrame* kCur,
                            KeyFrame* kNext);
    
    /**
     * @brief Interpolate the given keyframe 
     * @param t the parametric time at which to interpolate
     * @param itup An iterator pointing to the first element that's considered 
     * to have a parametric time t greater than the given t.
     * If t is greater than any keyframe time it will be pointing to the end of the keyframes set.
     * @param isPeriodic True if the curve is considered periodic, with the given period.
     * If periodic, the parametric t is always in the range [xmin, xmax] given in parameter
     * @param xmin The start of the period of the curve if isPeriodic is true
     * @param xmax The end of the period of the curve if isPeriodic is true
     * Note that the [xmin, xmax] may be greater than the range covered by the keyframes set. 
     * This function can only have a parametric t which is in one of the following 3 configurations (in brackets):
     *
     *   xmin..<1>....k1..<2>...k2..<3>....xmax
     *
     **/
    virtual KeyFrame interpolate(TimeValue t,
                                 KeyFrameSet::const_iterator itup,
                                 const KeyFrameSet& keyframes,
                                 const bool isPeriodic,
                                 TimeValue xmin,
                                 TimeValue xmax);
};


class OfxCustomStringKeyFrameInterpolator : public KeyFrameInterpolator
{

public:

    typedef OfxStatus (*customParamInterpolationV1Entry_t)(const void* handleRaw, OfxPropertySetHandle inArgsRaw, OfxPropertySetHandle outArgsRaw);


    OfxCustomStringKeyFrameInterpolator(customParamInterpolationV1Entry_t f, const void* handleRaw, const std::string& paramName);

    virtual ~OfxCustomStringKeyFrameInterpolator();

    virtual KeyFrameInterpolatorPtr createCopy() const OVERRIDE;

    virtual KeyFrame interpolate(TimeValue t,
                                 KeyFrameSet::const_iterator itup,
                                 const KeyFrameSet& keyframes,
                                 const bool isPeriodic,
                                 TimeValue xmin,
                                 TimeValue xmax) OVERRIDE FINAL;

private:

    const void* _handleRaw;
    std::string _paramName;
    customParamInterpolationV1Entry_t _f;
};

NATRON_NAMESPACE_EXIT

#endif // KEYFRAMEINTERPOLATOR_H
