/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
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

#include "AnimatingObjectI.h"

NATRON_NAMESPACE_ENTER;

AnimatingObjectI::AnimatingObjectI()
{

}

void
AnimatingObjectI::deleteValueAtTime(CurveChangeReason curveChangeReason, double time, ViewSpec view, int dimension)
{
    std::list<double> times;
    times.push_back(time);
    deleteValuesAtTime(curveChangeReason, times, view, dimension);
}

bool
AnimatingObjectI::moveValueAtTime(CurveChangeReason reason, double time, ViewSpec view,  int dimension, double dt, double dv, KeyFrame* newKey)
{
    std::list<double> times;
    times.push_back(time);
    std::vector<KeyFrame> keys;
    bool ret = moveValuesAtTime(reason, times, view, dimension, dt, dv, newKey ? &keys : 0);
    if (newKey && !keys.empty()) {
        *newKey = keys.front();
    }
    return ret;
}

bool
AnimatingObjectI::moveValuesAtTime(CurveChangeReason reason, const std::list<double>& times, ViewSpec view,  int dimension, double dt, double dv, std::vector<KeyFrame>* outKeys)
{
    return warpValuesAtTime(reason, times, view, dimension, Curve::TranslationKeyFrameWarp(dt, dv), false /*allowReplacingKeys*/, outKeys);
}

bool
AnimatingObjectI::transformValueAtTime(CurveChangeReason curveChangeReason, double time, ViewSpec view,  int dimension, const Transform::Matrix3x3& matrix, KeyFrame* newKey)
{
    std::list<double> times;
    times.push_back(time);
    std::vector<KeyFrame> keys;
    bool ret = transformValuesAtTime(curveChangeReason, times, view, dimension, matrix, newKey ? &keys : 0);
    if (newKey && !keys.empty()) {
        *newKey = keys.front();
    }
    return ret;
}

bool
AnimatingObjectI::transformValuesAtTime(CurveChangeReason curveChangeReason, const std::list<double>& times, ViewSpec view,  int dimension, const Transform::Matrix3x3& matrix, std::vector<KeyFrame>* outKeys)
{
    return warpValuesAtTime(curveChangeReason, times, view, dimension, Curve::AffineKeyFrameWarp(matrix), true /*allowReplacingKeys*/, outKeys);
}

void
AnimatingObjectI::removeAnimation(ViewSpec view, int dimension, CurveChangeReason reason)
{
    std::vector<int> dims(1);
    dims[0] = dimension;
    removeAnimationOnDimensions(view, dims, reason);
}

void
AnimatingObjectI::setInterpolationAtTime(CurveChangeReason reason, ViewSpec view, int dimension, double time, KeyframeTypeEnum interpolation, KeyFrame* newKey)
{
    std::list<double> times;
    times.push_back(time);
    std::vector<KeyFrame> keys;
    setInterpolationAtTimes(reason, view, dimension, times, interpolation, newKey ? &keys : 0);
    if (newKey && !keys.empty()) {
        *newKey = keys.front();
    }
}

NATRON_NAMESPACE_EXIT;
