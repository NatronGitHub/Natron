/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2017 INRIA and Alexandre Gauthier-Foichat
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

#include "AnimatingObjectI.h"

#include <stdexcept>

#include <QMutex>

NATRON_NAMESPACE_ENTER;

struct AnimatingObjectIPrivate
{
    mutable QMutex viewsMutex;
    std::list<ViewIdx> views;

    AnimatingObjectIPrivate()
    : viewsMutex()
    , views()
    {
        views.push_back(ViewIdx(0));
    }

    AnimatingObjectIPrivate(const AnimatingObjectIPrivate& other)
    : viewsMutex()
    , views()
    {
        QMutexLocker k(&other.viewsMutex);
        views = other.views;
    }

    ViewIdx findMatchingView(ViewIdx inView) const
    {
        // Private - should not lock
        assert(!viewsMutex.tryLock());

        std::list<ViewIdx>::const_iterator foundView = std::find(views.begin(), views.end(), inView);

        // Not found - fallback on main view
        if (foundView == views.end()) {
            return ViewIdx(0);
        }
        return inView;
    }
};

AnimatingObjectI::AnimatingObjectI()
: _imp(new AnimatingObjectIPrivate())
{

}

AnimatingObjectI::AnimatingObjectI(const boost::shared_ptr<AnimatingObjectI>& other, const FrameViewRenderKey& /*key*/)
: _imp(new AnimatingObjectIPrivate(*other->_imp))
{

}

AnimatingObjectI::~AnimatingObjectI()
{
    
}

std::list<ViewIdx>
AnimatingObjectI::getViewsList() const
{
    QMutexLocker k(&_imp->viewsMutex);
    return _imp->views;
}

ViewIdx
AnimatingObjectI::checkIfViewExistsOrFallbackMainView(ViewIdx view) const
{

    // Find the view. If it is not in the split views, fallback on the main view.
    QMutexLocker k(&_imp->viewsMutex);
    return _imp->findMatchingView(view);

} // checkIfViewExistsOrFallbackMainView

bool
AnimatingObjectI::splitView(ViewIdx view)
{
    if (!canSplitViews()) {
        return false;
    }
    {
        QMutexLocker k(&_imp->viewsMutex);
        for (std::list<ViewIdx>::iterator it = _imp->views.begin(); it!=_imp->views.end(); ++it) {
            if (*it == view) {
                return false;
            }
        }
        _imp->views.push_back(view);
    }
    return true;
}

bool
AnimatingObjectI::unSplitView(ViewIdx view)
{
    // Cannot split the main view
    if (view == 0) {
        return false;
    }

    if (!canSplitViews()) {
        return false;
    }

    bool viewFound = false;
    {
        QMutexLocker k(&_imp->viewsMutex);
        for (std::list<ViewIdx>::iterator it = _imp->views.begin(); it!=_imp->views.end(); ++it) {
            if (*it == view) {
                _imp->views.erase(it);
                viewFound = true;
                break;
            }
        }
    }
    return viewFound;
}

void
AnimatingObjectI::unSplitAllViews()
{
    std::list<ViewIdx> views = getViewsList();
    for (std::list<ViewIdx>::iterator it = views.begin(); it != views.end(); ++it) {
        if (*it == ViewIdx(0)) {
            continue;
        }
        unSplitView(*it);
    }
}


ValueChangedReturnCodeEnum
AnimatingObjectI::setIntValueAtTime(TimeValue /*time*/, int /*value*/, ViewSetSpec /*view*/, DimSpec /*dimension*/, ValueChangedReasonEnum /*reason*/, KeyFrame* /*newKey*/)
{
    if (getKeyFrameDataType() != eKeyframeDataTypeInt) {
        throw std::invalid_argument("Invalid call to setIntValueAtTime on an object that does not support integer");
    }
    return eValueChangedReturnCodeNothingChanged;
}

ValueChangedReturnCodeEnum
AnimatingObjectI::setDoubleValueAtTime(TimeValue /*time*/, double /*value*/, ViewSetSpec /*view*/, DimSpec /*dimension*/, ValueChangedReasonEnum /*reason*/, KeyFrame* /*newKey*/)
{
    if (getKeyFrameDataType() != eKeyframeDataTypeDouble) {
        throw std::invalid_argument("Invalid call to setDoubleValueAtTime on an object that does not support double");
    }
    return eValueChangedReturnCodeNothingChanged;
}

ValueChangedReturnCodeEnum
AnimatingObjectI::setBoolValueAtTime(TimeValue /*time*/, bool /*value*/, ViewSetSpec /*view*/, DimSpec /*dimension*/, ValueChangedReasonEnum /*reason*/, KeyFrame* /*newKey*/)
{
    if (getKeyFrameDataType() != eKeyframeDataTypeBool) {
        throw std::invalid_argument("Invalid call to setBoolValueAtTime on an object that does not support bool");
    }
    return eValueChangedReturnCodeNothingChanged;
}

ValueChangedReturnCodeEnum
AnimatingObjectI::setStringValueAtTime(TimeValue /*time*/, const std::string& /*value*/, ViewSetSpec /*view*/, DimSpec /*dimension*/, ValueChangedReasonEnum /*reason*/, KeyFrame* /*newKey*/)
{
    if (getKeyFrameDataType() != eKeyframeDataTypeString) {
        throw std::invalid_argument("Invalid call to setStringValueAtTime on an object that does not support string");
    }
    return eValueChangedReturnCodeNothingChanged;
}

void
AnimatingObjectI::setMultipleIntValueAtTime(const std::list<IntTimeValuePair>& /*keys*/, ViewSetSpec /*view*/, DimSpec /*dimension*/, ValueChangedReasonEnum /*reason*/, std::vector<KeyFrame>* /*newKey*/)
{
    if (getKeyFrameDataType() != eKeyframeDataTypeInt) {
        throw std::invalid_argument("Invalid call to setMultipleIntValueAtTime on an object that does not support integer");
    }
}

void
AnimatingObjectI::setMultipleDoubleValueAtTime(const std::list<DoubleTimeValuePair>& /*keys*/, ViewSetSpec /*view*/, DimSpec /*dimension*/, ValueChangedReasonEnum /*reason*/, std::vector<KeyFrame>* /*newKey*/)
{
    if (getKeyFrameDataType() != eKeyframeDataTypeDouble) {
        throw std::invalid_argument("Invalid call to setMultipleDoubleValueAtTime on an object that does not support double");
    }
}


void
AnimatingObjectI::setMultipleBoolValueAtTime(const std::list<BoolTimeValuePair>& /*keys*/, ViewSetSpec /*view*/, DimSpec /*dimension*/, ValueChangedReasonEnum /*reason*/, std::vector<KeyFrame>* /*newKey*/)
{
    if (getKeyFrameDataType() != eKeyframeDataTypeBool) {
        throw std::invalid_argument("Invalid call to setMultipleBoolValueAtTime on an object that does not support boolean");
    }
}

void
AnimatingObjectI::setMultipleStringValueAtTime(const std::list<StringTimeValuePair>& /*keys*/, ViewSetSpec /*view*/, DimSpec /*dimension*/, ValueChangedReasonEnum /*reason*/, std::vector<KeyFrame>* /*newKey*/)
{
    if (getKeyFrameDataType() != eKeyframeDataTypeString) {
        throw std::invalid_argument("Invalid call to setMultipleStringValueAtTime on an object that does not support string");
    }
}

void
AnimatingObjectI::setIntValueAtTimeAcrossDimensions(TimeValue /*time*/, const std::vector<int>& /*values*/, DimIdx /*dimensionStartIndex*/, ViewSetSpec /*view*/, ValueChangedReasonEnum /*reason*/, std::vector<ValueChangedReturnCodeEnum>* /*retCodes*/)
{
    if (getKeyFrameDataType() != eKeyframeDataTypeInt) {
        throw std::invalid_argument("Invalid call to setIntValueAtTimeAcrossDimensions on an object that does not support integer");
    }
}

void
AnimatingObjectI::setDoubleValueAtTimeAcrossDimensions(TimeValue /*time*/, const std::vector<double>& /*values*/, DimIdx /*dimensionStartIndex*/, ViewSetSpec /*view*/, ValueChangedReasonEnum /*reason*/, std::vector<ValueChangedReturnCodeEnum>* /*retCodes*/)
{
    if (getKeyFrameDataType() != eKeyframeDataTypeDouble) {
        throw std::invalid_argument("Invalid call to setDoubleValueAtTimeAcrossDimensions on an object that does not support double");
    }

}

void
AnimatingObjectI::setBoolValueAtTimeAcrossDimensions(TimeValue /*time*/, const std::vector<bool>& /*values*/, DimIdx /*dimensionStartIndex*/, ViewSetSpec /*view*/, ValueChangedReasonEnum /*reason*/, std::vector<ValueChangedReturnCodeEnum>* /*retCodes*/)
{
    if (getKeyFrameDataType() != eKeyframeDataTypeBool) {
        throw std::invalid_argument("Invalid call to setBoolValueAtTimeAcrossDimensions on an object that does not support boolean");
    }
}

void
AnimatingObjectI::setStringValueAtTimeAcrossDimensions(TimeValue /*time*/, const std::vector<std::string>& /*values*/, DimIdx /*dimensionStartIndex*/, ViewSetSpec /*view*/, ValueChangedReasonEnum /*reason*/, std::vector<ValueChangedReturnCodeEnum>* /*retCodes*/)
{
    if (getKeyFrameDataType() != eKeyframeDataTypeString) {
        throw std::invalid_argument("Invalid call to setStringValueAtTimeAcrossDimensions on an object that does not support string");
    }
}

void
AnimatingObjectI::setMultipleIntValueAtTimeAcrossDimensions(const PerCurveIntValuesList& /*keysPerDimension*/,  ValueChangedReasonEnum /*reason*/)
{
    if (getKeyFrameDataType() != eKeyframeDataTypeInt) {
        throw std::invalid_argument("Invalid call to setMultipleIntValueAtTimeAcrossDimensions on an object that does not support integer");
    }
}

void
AnimatingObjectI::setMultipleDoubleValueAtTimeAcrossDimensions(const PerCurveDoubleValuesList& /*keysPerDimension*/,  ValueChangedReasonEnum /*reason*/)
{
    if (getKeyFrameDataType() != eKeyframeDataTypeDouble) {
        throw std::invalid_argument("Invalid call to setMultipleDoubleValueAtTimeAcrossDimensions on an object that does not support double");
    }
}


void
AnimatingObjectI::setMultipleBoolValueAtTimeAcrossDimensions(const PerCurveBoolValuesList& /*keysPerDimension*/,  ValueChangedReasonEnum /*reason*/)
{
    if (getKeyFrameDataType() != eKeyframeDataTypeBool) {
        throw std::invalid_argument("Invalid call to setMultipleBoolValueAtTimeAcrossDimensions on an object that does not support boolean");
    }
}


void
AnimatingObjectI::setMultipleStringValueAtTimeAcrossDimensions(const PerCurveStringValuesList& /*keysPerDimension*/,  ValueChangedReasonEnum /*reason*/)
{
    if (getKeyFrameDataType() != eKeyframeDataTypeString) {
        throw std::invalid_argument("Invalid call to setMultipleStringValueAtTimeAcrossDimensions on an object that does not support string");
    }
}


void
AnimatingObjectI::deleteValueAtTime(TimeValue time, ViewSetSpec view, DimSpec dimension, ValueChangedReasonEnum reason)
{
    std::list<double> times;
    times.push_back(time);
    deleteValuesAtTime(times, view, dimension, reason);
}

bool
AnimatingObjectI::moveValueAtTime(TimeValue time, ViewSetSpec view,  DimSpec dimension, double dt, double dv, KeyFrame* newKey)
{
    std::list<double> times;
    times.push_back(time);
    std::vector<KeyFrame> keys;
    bool ret = moveValuesAtTime(times, view, dimension, dt, dv, newKey ? &keys : 0);
    if (newKey && !keys.empty()) {
        *newKey = keys.front();
    }
    return ret;
}

bool
AnimatingObjectI::moveValuesAtTime(const std::list<double>& times, ViewSetSpec view,  DimSpec dimension, double dt, double dv, std::vector<KeyFrame>* outKeys)
{
    return warpValuesAtTime(times, view, dimension, Curve::TranslationKeyFrameWarp(dt, dv), outKeys);
}

bool
AnimatingObjectI::transformValueAtTime(TimeValue time, ViewSetSpec view,  DimSpec dimension, const Transform::Matrix3x3& matrix, KeyFrame* newKey)
{
    std::list<double> times;
    times.push_back(time);
    std::vector<KeyFrame> keys;
    bool ret = transformValuesAtTime(times, view, dimension, matrix, newKey ? &keys : 0);
    if (newKey && !keys.empty()) {
        *newKey = keys.front();
    }
    return ret;
}

bool
AnimatingObjectI::transformValuesAtTime(const std::list<double>& times, ViewSetSpec view,  DimSpec dimension, const Transform::Matrix3x3& matrix, std::vector<KeyFrame>* outKeys)
{
    return warpValuesAtTime(times, view, dimension, Curve::AffineKeyFrameWarp(matrix),  outKeys);
}


void
AnimatingObjectI::setInterpolationAtTime(ViewSetSpec view, DimSpec dimension, TimeValue time, KeyframeTypeEnum interpolation, KeyFrame* newKey)
{
    std::list<double> times;
    times.push_back(time);
    std::vector<KeyFrame> keys;
    setInterpolationAtTimes(view, dimension, times, interpolation, newKey ? &keys : 0);
    if (newKey && !keys.empty()) {
        *newKey = keys.front();
    }
}

enum DeleteKnobAnimationEnum
{
    eDeleteKnobAnimationAll,
    eDeleteKnobAnimationBeforeTime,
    eDeleteKnobAnimationAfterTime
};

static void deleteValuesConditionalInternal(AnimatingObjectI* obj, DeleteKnobAnimationEnum type, const std::set<double>& keyframesToIgnore, const RangeD* range, TimeValue time, ViewSetSpec view, DimSpec dimension, ValueChangedReasonEnum reason)
{
    std::list<ViewIdx> views = obj->getViewsList();
    for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
        if (!view.isAll() && *it != view) {
            continue;
        }
        for (int i = 0; i < obj->getNDimensions(); ++i) {
            if (!dimension.isAll() && i != dimension) {
                continue;
            }
            CurvePtr curve = obj->getAnimationCurve(ViewIdx(0), DimIdx(i));
            assert(curve);
            KeyFrameSet keys = curve->getKeyFrames_mt_safe();
            std::list<double> toRemove;
            switch (type) {
                case eDeleteKnobAnimationAll: {
                    for (KeyFrameSet::iterator it = keys.begin(); it != keys.end(); ++it) {
                        if (range && it->getTime() < range->min) {
                            continue;
                        }
                        if (range && it->getTime() > range->max) {
                            break;
                        }
                        std::set<double>::iterator found = keyframesToIgnore.find( it->getTime() );
                        if ( found == keyframesToIgnore.end() ) {
                            toRemove.push_back( it->getTime() );
                        }
                    }

                    break;
                }
                case eDeleteKnobAnimationBeforeTime: {
                    for (KeyFrameSet::iterator it = keys.begin(); it != keys.end(); ++it) {
                        if (it->getTime() >= time) {
                            break;
                        }
                        std::set<double>::iterator found = keyframesToIgnore.find( it->getTime() );
                        if ( found == keyframesToIgnore.end() ) {
                            toRemove.push_back( it->getTime() );
                        }
                    }
                    break;
                }
                case eDeleteKnobAnimationAfterTime: {
                    for (KeyFrameSet::reverse_iterator it = keys.rbegin(); it != keys.rend(); ++it) {
                        if (it->getTime() <= time) {
                            break;
                        }
                        std::set<double>::iterator found = keyframesToIgnore.find( it->getTime() );
                        if ( found == keyframesToIgnore.end() ) {
                            toRemove.push_back( it->getTime() );
                        }
                    }
                    break;
                }
            }
            obj->deleteValuesAtTime(toRemove, *it, DimIdx(i), reason);
        }
    }
} // deleteValuesConditionalInternal

void
AnimatingObjectI::deleteValuesExceptAtTime(const std::set<double>& keyframesToIgnore, const RangeD* range, ViewSetSpec view, DimSpec dimension, ValueChangedReasonEnum reason)
{
    deleteValuesConditionalInternal(this, eDeleteKnobAnimationAll, keyframesToIgnore, range, TimeValue(0), view, dimension, reason);
}

void
AnimatingObjectI::deleteValuesBeforeTime(const std::set<double>& keyframesToIgnore, TimeValue time, ViewSetSpec view, DimSpec dimension, ValueChangedReasonEnum reason)
{
    deleteValuesConditionalInternal(this, eDeleteKnobAnimationBeforeTime, keyframesToIgnore, 0, time, view, dimension, reason);
}


void
AnimatingObjectI::deleteValuesAfterTime(const std::set<double>& keyframesToIgnore, TimeValue time, ViewSetSpec view, DimSpec dimension, ValueChangedReasonEnum reason)
{
    deleteValuesConditionalInternal(this, eDeleteKnobAnimationAfterTime, keyframesToIgnore, 0, time, view, dimension, reason);
}

bool
AnimatingObjectI::getKeyFrameTime(ViewIdx view,
                            int index,
                            DimIdx dimension,
                            TimeValue* time) const
{
    CurvePtr curve = getAnimationCurve(view, dimension);
    if (!curve) {
        return false;
    }

    KeyFrame kf;
    bool ret = curve->getKeyFrameWithIndex(index, &kf);
    if (ret) {
        *time = kf.getTime();
    }

    return ret;
}

bool
AnimatingObjectI::getLastKeyFrameTime(ViewIdx view,
                                DimIdx dimension,
                                TimeValue* time) const
{
    CurvePtr curve = getAnimationCurve(view, dimension);
    if (!curve) {
        return false;
    }
    if (!curve->isAnimated()) {
        return false;
    }
    *time = TimeValue(curve->getMaximumTimeCovered());
    return true;
}

bool
AnimatingObjectI::getFirstKeyFrameTime(ViewIdx view,
                                 DimIdx dimension,
                                 TimeValue* time) const
{
    return getKeyFrameTime(view, 0, dimension, time);
}

int
AnimatingObjectI::getKeyFramesCount(ViewIdx view,
                              DimIdx dimension) const
{
    CurvePtr curve = getAnimationCurve(view, dimension);
    if (!curve) {
        return false;
    }

    return curve->getKeyFramesCount();
}

bool
AnimatingObjectI::getPreviousKeyFrameTime(ViewIdx view, DimIdx dimension, TimeValue time, TimeValue* keyframeTime) const
{

    CurvePtr curve = getAnimationCurve(view, dimension);
    if (!curve) {
        return false;
    }

    KeyFrame kf;
    if (curve->getPreviousKeyframeTime(time, &kf)) {
        *keyframeTime = kf.getTime();
        return true;
    }

    return false;
}

bool
AnimatingObjectI::getNextKeyFrameTime(ViewIdx view, DimIdx dimension, TimeValue time, TimeValue* keyframeTime) const
{

    CurvePtr curve = getAnimationCurve(view, dimension);
    if (!curve) {
        return false;
    }

    KeyFrame kf;
    if (curve->getNextKeyframeTime(time, &kf)) {
        *keyframeTime = kf.getTime();
        return true;
    }

    return false;
}


int
AnimatingObjectI::getKeyFrameIndex(ViewIdx view,
                             DimIdx dimension,
                             TimeValue time) const
{
    CurvePtr curve = getAnimationCurve(view, dimension);
    if (!curve) {
        return -1;
    }
    return curve->keyFrameIndex(time);
}



NATRON_NAMESPACE_EXIT;
