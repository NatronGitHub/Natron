/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#include "Knob.h"
#include "KnobPrivate.h"

NATRON_NAMESPACE_ENTER

ValueChangedReturnCodeEnum
KnobDimViewBase::setKeyFrame(const KeyFrame& key, SetKeyFrameFlags flags)
{
    if (!animationCurve) {
        return eValueChangedReturnCodeNothingChanged;
    }

    ValueChangedReturnCodeEnum addKeyRet = animationCurve->setOrAddKeyframe(key, flags);
    notifyCurveChanged();

    return addKeyRet;
    
}

CurvePtr KnobHelper::getAnimationCurve(ViewIdx view,
                              DimIdx dimension) const
{
    if ( (dimension < 0) || ( dimension >= _imp->common->dimension) ) {
        throw std::invalid_argument("KnobHelper::getCurve: dimension out of range");
    }

    ViewIdx view_i = checkIfViewExistsOrFallbackMainView(view);
    KnobDimViewBasePtr dimViewData = getDataForDimView(dimension, view_i);
    return dimViewData->animationCurve;
} // getCurve


void
KnobDimViewBase::deleteValuesAtTime(const std::list<double>& times)
{




    // Update internal curve
    try {
        for (std::list<double>::const_iterator it = times.begin(); it != times.end(); ++it) {
            // This may throw exceptions if keyframes do not exist
            animationCurve->removeKeyFrameWithTime(TimeValue(*it));
        }
    } catch (const std::exception & /*e*/) {
    }


    notifyCurveChanged();
} // deleteValuesAtTime

void
KnobHelper::deleteValuesAtTimeInternal(const std::list<double>& times, ViewIdx view, DimIdx dimension)
{
    if (!isAnimated(dimension, view)) {
        return;
    }

    KnobDimViewBasePtr data = getDataForDimView(dimension, view);
    if (!data) {
        return;
    }

    if (!data->animationCurve) {
        return;
    }

    int nKeys = data->animationCurve->getKeyFramesCount();


    // We are about to remove the last keyframe, ensure that the internal value of the knob is the one of the animation
    if (nKeys - times.size() == 0) {
        copyValuesFromCurve(dimension, view);
    }

    data->deleteValuesAtTime(times);

}

void
KnobHelper::deleteValuesAtTime(const std::list<double>& times,
                               ViewSetSpec view,
                               DimSpec dimension,
                               ValueChangedReasonEnum reason)
{

    if ( times.empty() ) {
        return;
    }

    if ( !canAnimate() ) {
        return;
    }

    std::list<ViewIdx> views = getViewsList();
    for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
        if (!view.isAll() && *it != ViewIdx(view)) {
            continue;
        }
        DimSpec thisDimension = dimension;

        // If the item has its dimensions folded and we modify dimension 0, also modify other dimensions
        if (thisDimension == 0 && !getAllDimensionsVisible(*it)) {
            thisDimension = DimSpec::all();
        }

        for (int i = 0; i < _imp->common->dimension; ++i) {
            if (!thisDimension.isAll() && thisDimension != i) {
                continue;
            }

            deleteValuesAtTimeInternal(times, *it, DimIdx(i));
        }
    }


    // Evaluate the change
    evaluateValueChange(dimension, TimeValue(*times.begin()), view, reason);



} // deleteValuesAtTime

void
KnobDimViewBase::deleteAnimationConditional(TimeValue time, bool before)
{
    if (!animationCurve) {
        return;
    }
    if (before) {
        animationCurve->removeKeyFramesBeforeTime(time, 0);
    } else {
        animationCurve->removeKeyFramesAfterTime(time, 0);
    }

    notifyCurveChanged();
} // deleteAnimationConditional

void
KnobHelper::deleteAnimationConditionalInternal(TimeValue time, ViewIdx view, DimIdx dimension, bool before)
{
    if (!isAnimated(dimension, view)) {
        return;
    }
    KnobDimViewBasePtr data = getDataForDimView(dimension, view);
    if (!data) {
        return;
    }
    data->deleteAnimationConditional(time, before);

} // deleteAnimationConditionalInternal

void
KnobHelper::deleteAnimationConditional(TimeValue time,
                                       ViewSetSpec view,
                                       DimSpec dimension,
                                       bool before)
{

    std::list<ViewIdx> views = getViewsList();
    for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
        if (!view.isAll() && *it != ViewIdx(view)) {
            continue;
        }
        DimSpec thisDimension = dimension;

        // If the item has its dimensions folded and we modify dimension 0, also modify other dimensions
        if (thisDimension == 0 && !getAllDimensionsVisible(*it)) {
            thisDimension = DimSpec::all();
        }

        for (int i = 0; i < _imp->common->dimension; ++i) {
            if (!thisDimension.isAll() && thisDimension != i) {
                continue;
            }

            deleteAnimationConditionalInternal(time, *it, DimIdx(i), before);
        }
    }


    evaluateValueChange(dimension, time, view, eValueChangedReasonUserEdited);


} // deleteAnimationConditional

void
KnobHelper::deleteAnimationBeforeTime(TimeValue time,
                                      ViewSetSpec view,
                                      DimSpec dimension)
{
    deleteAnimationConditional(time, view, dimension, true);
}

void
KnobHelper::deleteAnimationAfterTime(TimeValue time,
                                     ViewSetSpec view,
                                     DimSpec dimension)
{
    deleteAnimationConditional(time, view, dimension, false);
}

bool
KnobDimViewBase::warpValuesAtTime(const std::list<double>& times, const Curve::KeyFrameWarp& warp, std::vector<KeyFrame>* outKeys)
{
    if (!animationCurve) {
        return false;
    }


    if (outKeys) {
        outKeys->clear();
    }

    // This may fail if we cannot find a keyframe at the given time
    std::vector<KeyFrame> newKeys;
    if ( !animationCurve->transformKeyframesValueAndTime(times, warp, &newKeys, 0, 0) ) {
        return false;
    }
    if (outKeys) {
        *outKeys = newKeys;
    }


    notifyCurveChanged();

    return true;
} // warpValuesAtTime

bool
KnobHelper::warpValuesAtTimeInternal(const std::list<double>& times, ViewIdx view,  DimIdx dimension, const Curve::KeyFrameWarp& warp, std::vector<KeyFrame>* outKeys)
{
    if (!isAnimated(dimension, view)) {
        return false;
    }
    KnobDimViewBasePtr data = getDataForDimView(dimension, view);
    if (!data) {
        return false;
    }

    return data->warpValuesAtTime(times, warp, outKeys);
} // warpValuesAtTimeInternal

bool
KnobHelper::warpValuesAtTime(const std::list<double>& times, ViewSetSpec view,  DimSpec dimension, const Curve::KeyFrameWarp& warp, std::vector<KeyFrame>* outKeys)
{
    if ( times.empty() || !canAnimate() ) {
        return true;
    }

    bool ret = true;
    std::list<ViewIdx> views = getViewsList();
    for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
        if (!view.isAll() && *it != ViewIdx(view)) {
            continue;
        }
        DimSpec thisDimension = dimension;

        // If the item has its dimensions folded and we modify dimension 0, also modify other dimensions
        if (thisDimension == 0 && !getAllDimensionsVisible(*it)) {
            thisDimension = DimSpec::all();
        }

        for (int i = 0; i < _imp->common->dimension; ++i) {
            if (!thisDimension.isAll() && thisDimension != i) {
                continue;
            }

            ret &= warpValuesAtTimeInternal(times, *it, DimIdx(i), warp, outKeys);
        }
    }

    evaluateValueChange(dimension, TimeValue(times.front()), view, eValueChangedReasonUserEdited);

    return ret;

} // warpValuesAtTime


bool
KnobDimViewBase::copy(const CopyInArgs& inArgs, CopyOutArgs* outArgs)
{
    bool hasChanged = false;

    {

        // Do not copy the shared knobs

        KeyFrameSet oldKeys;
        if (animationCurve) {
            oldKeys = animationCurve->getKeyFrames_mt_safe();
        }

        const Curve* otherCurve = inArgs.other ? inArgs.other->animationCurve.get() : inArgs.otherCurve;
        if (otherCurve) {
            if (!animationCurve) {
                animationCurve.reset(new Curve(otherCurve->getType()));
            }
            hasChanged |= animationCurve->cloneAndCheckIfChanged(*otherCurve, inArgs.keysToCopyOffset, inArgs.keysToCopyRange);
        }


        if (hasChanged && outArgs) {
            // Compute the keyframes diff
            KeyFrameSet keys;
            if (animationCurve) {
                keys = animationCurve->getKeyFrames_mt_safe();
            }
            Curve::computeKeyFramesDiff(oldKeys, keys, outArgs->keysAdded, outArgs->keysRemoved);
        }
    }
    if (hasChanged) {
        // Notify all shared knobs the curve changed
        notifyCurveChanged();
    }
    return hasChanged;
} // copy


bool
KnobHelper::cloneCurve(ViewIdx view,
                       DimIdx dimension,
                       const Curve& curve,
                       double offset,
                       const RangeD* range)
{
    if (dimension < 0 || dimension >= _imp->common->dimension) {
        throw std::invalid_argument("KnobHelper::cloneCurve: Dimension out of range");
    }

    KnobDimViewBasePtr data = getDataForDimView(dimension, view);
    if (!data) {
        return false;
    }


    KnobDimViewBase::CopyInArgs copyArgs(curve);
    copyArgs.keysToCopyRange = range;
    copyArgs.keysToCopyOffset = offset;

    bool hasChanged = data->copy(copyArgs, 0);

    if (hasChanged) {
        evaluateValueChange(dimension, getHolder()->getTimelineCurrentTime(), view,  eValueChangedReasonUserEdited);
    }
    return hasChanged;
} // cloneCurve


void
KnobDimViewBase::setInterpolationAtTimes(const std::list<double>& times, KeyframeTypeEnum interpolation, std::vector<KeyFrame>* newKeys)
{

    if (!animationCurve) {
        return;
    }

    for (std::list<double>::const_iterator it = times.begin(); it != times.end(); ++it) {
        KeyFrame k;
        if (animationCurve->setKeyFrameInterpolation(interpolation, TimeValue(*it), &k)) {
            if (newKeys) {
                newKeys->push_back(k);
            }
        }
    }

    notifyCurveChanged();
} // setInterpolationAtTimes

void
KnobHelper::setInterpolationAtTimesInternal(ViewIdx view, DimIdx dimension, const std::list<double>& times, KeyframeTypeEnum interpolation, std::vector<KeyFrame>* newKeys)
{
    if (!isAnimated(dimension, view)) {
        return;
    }
    if (times.empty()) {
        return;
    }
    KnobDimViewBasePtr data = getDataForDimView(dimension, view);
    assert(data);
    data->setInterpolationAtTimes(times, interpolation, newKeys);
}

void
KnobHelper::setInterpolationAtTimes(ViewSetSpec view, DimSpec dimension, const std::list<double>& times, KeyframeTypeEnum interpolation, std::vector<KeyFrame>* newKeys)
{

    if (times.empty()) {
        return;
    }
    if ( !canAnimate() ) {
        return;
    }

    std::list<ViewIdx> views = getViewsList();
    for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
        if (!view.isAll() && *it != ViewIdx(view)) {
            continue;
        }
        DimSpec thisDimension = dimension;

        // If the item has its dimensions folded and we modify dimension 0, also modify other dimensions
        if (thisDimension == 0 && !getAllDimensionsVisible(*it)) {
            thisDimension = DimSpec::all();
        }

        for (int i = 0; i < _imp->common->dimension; ++i) {
            if (!thisDimension.isAll() && thisDimension != i) {
                continue;
            }

            setInterpolationAtTimesInternal(*it, DimIdx(i), times, interpolation, newKeys);
        }
    }


    evaluateValueChange(dimension, TimeValue(times.front()), view, eValueChangedReasonUserEdited);


} // setInterpolationAtTimes

bool
KnobDimViewBase::setLeftAndRightDerivativesAtTime(TimeValue time, double left, double right)
{

    if (!animationCurve) {
        return false;
    }

    int keyIndex = animationCurve->keyFrameIndex(time);
    if (keyIndex == -1) {
        return false;
    }

    animationCurve->setKeyFrameInterpolation(eKeyframeTypeFree, keyIndex);
    animationCurve->setKeyFrameDerivatives(left, right, keyIndex);

    notifyCurveChanged();
    return true;
} // setLeftAndRightDerivativesAtTime

bool
KnobHelper::setLeftAndRightDerivativesAtTimeInternal(ViewIdx view, DimIdx dimension, TimeValue time, double left, double right)
{

    if (!isAnimated(dimension, view)) {
        return false;
    }
    KnobDimViewBasePtr data = getDataForDimView(dimension, view);
    if (!data) {
        return false;
    }
    return data->setLeftAndRightDerivativesAtTime(time, left, right);

} // setLeftAndRightDerivativesAtTimeInternal

bool
KnobHelper::setLeftAndRightDerivativesAtTime(ViewSetSpec view,
                                             DimSpec dimension,
                                             TimeValue time,
                                             double left,
                                             double right)
{

    if ( !canAnimate() ) {
        return false;
    }

    bool ok = false;

    std::list<ViewIdx> views = getViewsList();
    for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
        if (!view.isAll() && *it != ViewIdx(view)) {
            continue;
        }
        DimSpec thisDimension = dimension;

        // If the item has its dimensions folded and we modify dimension 0, also modify other dimensions
        if (thisDimension == 0 && !getAllDimensionsVisible(*it)) {
            thisDimension = DimSpec::all();
        }

        for (int i = 0; i < _imp->common->dimension; ++i) {
            if (!thisDimension.isAll() && thisDimension != i) {
                continue;
            }

            ok |= setLeftAndRightDerivativesAtTimeInternal(*it, DimIdx(i), time, left, right);
        }
    }


    evaluateValueChange(dimension, time, view, eValueChangedReasonUserEdited);

    return ok;
} // KnobHelper::setLeftAndRightDerivativesAtTime

bool
KnobDimViewBase::setDerivativeAtTime(TimeValue time, double derivative, bool isLeft)
{
    if (!animationCurve) {
        return false;
    }

    int keyIndex = animationCurve->keyFrameIndex(time);
    if (keyIndex == -1) {
        return false;
    }

    animationCurve->setKeyFrameInterpolation(eKeyframeTypeBroken, keyIndex);
    if (isLeft) {
        animationCurve->setKeyFrameLeftDerivative(derivative, keyIndex);
    } else {
        animationCurve->setKeyFrameRightDerivative(derivative, keyIndex);
    }

    notifyCurveChanged();

    return true;
}

bool
KnobHelper::setDerivativeAtTimeInternal(ViewIdx view, DimIdx dimension, TimeValue time, double derivative, bool isLeft)
{
    if (!isAnimated(dimension, view)) {
        return false;
    }
    KnobDimViewBasePtr data = getDataForDimView(dimension, view);
    if (!data) {
        return false;
    }

    return data->setDerivativeAtTime(time, derivative, isLeft);
}

bool
KnobHelper::setDerivativeAtTime(ViewSetSpec view,
                                DimSpec dimension,
                                TimeValue time,
                                double derivative,
                                bool isLeft)
{
    if ( !canAnimate() ) {
        return false;
    }
    bool ok = false;

    std::list<ViewIdx> views = getViewsList();
    for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
        if (!view.isAll() && *it != ViewIdx(view)) {
            continue;
        }
        DimSpec thisDimension = dimension;

        // If the item has its dimensions folded and we modify dimension 0, also modify other dimensions
        if (thisDimension == 0 && !getAllDimensionsVisible(*it)) {
            thisDimension = DimSpec::all();
        }

        for (int i = 0; i < _imp->common->dimension; ++i) {
            if (!thisDimension.isAll() && thisDimension != i) {
                continue;
            }

            ok |= setDerivativeAtTimeInternal(*it, DimIdx(i), time, derivative, isLeft);
        }
    }


    evaluateValueChange(dimension, time, view, eValueChangedReasonUserEdited);

    return true;
} // KnobHelper::moveDerivativeAtTime


void
KnobDimViewBase::removeAnimation()
{
    if (!animationCurve) {
        return;
    }

    KeyFrameSet keys = animationCurve->getKeyFrames_mt_safe();
    if (keys.empty()) {
        return;
    }

    // Notify the user of the keyframes that were removed
    std::list<double> timesRemoved;

    for (KeyFrameSet::const_iterator it = keys.begin(); it!=keys.end(); ++it) {
        timesRemoved.push_back(it->getTime());
    }
    animationCurve->clearKeyFrames();


    if (timesRemoved.empty()) {
        // Nothing changed
        return;
    }

    notifyCurveChanged();

} // removeAnimation

bool
KnobHelper::removeAnimationInternal(ViewIdx view, DimIdx dimension)
{
    if (!isAnimated(dimension, view)) {
        return false;
    }

    KnobDimViewBasePtr data = getDataForDimView(dimension, view);
    if (!data) {
        return false;
    }

    
    // Ensure the underlying values are the values of the current curve at the current time
    copyValuesFromCurve(dimension, view);

    data->removeAnimation();
    return true;

} // removeAnimationInternal

void
KnobHelper::removeAnimation(ViewSetSpec view, DimSpec dimension, ValueChangedReasonEnum reason)
{

    if (!canAnimate()) {
        return;
    }

    bool removedAnimation = false;
    std::list<ViewIdx> views = getViewsList();
    for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
        if (!view.isAll() && *it != ViewIdx(view)) {
            continue;
        }
        DimSpec thisDimension = dimension;

        // If the item has its dimensions folded and we modify dimension 0, also modify other dimensions
        if (thisDimension == 0 && !getAllDimensionsVisible(*it)) {
            thisDimension = DimSpec::all();
        }

        for (int i = 0; i < _imp->common->dimension; ++i) {
            if (!thisDimension.isAll() && thisDimension != i) {
                continue;
            }

            removedAnimation |= removeAnimationInternal(*it, DimIdx(i));
        }
    }
    
    if (removedAnimation) {
        evaluateValueChange(dimension, getCurrentRenderTime(), view, reason);
    }
} // removeAnimation

NATRON_NAMESPACE_EXIT
