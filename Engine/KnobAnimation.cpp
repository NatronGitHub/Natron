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


// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Knob.h"
#include "KnobPrivate.h"

NATRON_NAMESPACE_ENTER


CurvePtr KnobHelper::getAnimationCurve(ViewIdx view,
                              DimIdx dimension) const
{
    if ( (dimension < 0) || ( dimension >= _imp->dimension) ) {
        throw std::invalid_argument("KnobHelper::getCurve: dimension out of range");
    }

    ViewIdx view_i = getViewIdxFromGetSpec(view);
    KnobDimViewBasePtr dimViewData = getDataForDimView(dimension, view_i);
    return dimViewData->animationCurve;
} // getCurve


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
    CurvePtr curve = data->animationCurve;
    if (!curve) {
        throw std::runtime_error("KnobHelper::deleteValuesAtTimeInternal: curve is null");
    }

    int nKeys = curve->getKeyFramesCount();

    // We are about to remove the last keyframe, ensure that the internal value of the knob is the one of the animation
    if (nKeys - times.size() == 0) {
        copyValuesFromCurve(dimension, view);
    }

    // Update internal curve
    try {
        for (std::list<double>::const_iterator it = times.begin(); it != times.end(); ++it) {
            // This may throw exceptions if keyframes do not exist
            curve->removeKeyFrameWithTime(*it);
        }
    } catch (const std::exception & /*e*/) {
    }


    data->notifyCurveChanged();
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
    if (dimension.isAll()) {
        for (int i = 0; i < _imp->dimension; ++i) {
            if (view.isAll()) {
                for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
                    deleteValuesAtTimeInternal(times, *it, DimIdx(i));
                }
            } else {
                ViewIdx view_i = getViewIdxFromGetSpec(ViewIdx(view.value()));
                deleteValuesAtTimeInternal(times, view_i, DimIdx(i));
            }
        }
    } else {
        if ( ( dimension >= _imp->dimension ) || (dimension < 0) ) {
            throw std::invalid_argument("KnobHelper::deleteValuesAtTime(): Dimension out of range");
        }
        if (view.isAll()) {
            for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
                deleteValuesAtTimeInternal(times, *it, DimIdx(dimension));
            }
        } else {
            ViewIdx view_i = getViewIdxFromGetSpec(ViewIdx(view.value()));
            deleteValuesAtTimeInternal(times, view_i, DimIdx(dimension));
        }
    }

    // virtual portion
    onKeyframesRemoved(times, view, dimension);


    // Evaluate the change
    evaluateValueChange(dimension, *times.begin(), view, reason);



} // deleteValuesAtTime

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
    CurvePtr curve = data->animationCurve;
    if (!curve) {
        throw std::runtime_error("KnobHelper::deleteAnimationConditionalInternal: curve is null");
    }
    if (before) {
        curve->removeKeyFramesBeforeTime(time, 0);
    } else {
        curve->removeKeyFramesAfterTime(time, 0);
    }



    data->notifyCurveChanged();

} // deleteAnimationConditionalInternal

void
KnobHelper::deleteAnimationConditional(TimeValue time,
                                       ViewSetSpec view,
                                       DimSpec dimension,
                                       bool before)
{
    std::list<ViewIdx> views = getViewsList();
    if (dimension.isAll()) {
        for (int i = 0; i < _imp->dimension; ++i) {
            if (view.isAll()) {
                for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
                    deleteAnimationConditionalInternal(time, *it, DimIdx(i), before);
                }
            } else {
                ViewIdx view_i = getViewIdxFromGetSpec(ViewIdx(view.value()));
                deleteAnimationConditionalInternal(time, view_i, DimIdx(i), before);
            }
        }
    } else {
        if ( ( dimension >= _imp->dimension ) || (dimension < 0) ) {
            throw std::invalid_argument("KnobHelper::deleteAnimationConditional(): Dimension out of range");
        }
        if (view.isAll()) {
            for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
                deleteAnimationConditionalInternal(time, *it, DimIdx(dimension.value()), before);
            }
        } else {
            ViewIdx view_i = getViewIdxFromGetSpec(ViewIdx(view.value()));
            deleteAnimationConditionalInternal(time, view_i, DimIdx(dimension.value()), before);
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
KnobHelper::warpValuesAtTimeInternal(const std::list<double>& times, ViewIdx view,  DimIdx dimension, const Curve::KeyFrameWarp& warp, std::vector<KeyFrame>* outKeys)
{
    if (!isAnimated(dimension, view)) {
        return false;
    }
    KnobDimViewBasePtr data = getDataForDimView(dimension, view);
    if (!data) {
        return false;
    }
    CurvePtr curve = data->animationCurve;
    if (!curve) {
        throw std::runtime_error("KnobHelper::warpValuesAtTimeInternal: curve is null");
    }

    // Make sure string animation follows up the warp
    std::vector<std::string> oldStringValues;
    AnimatingKnobStringHelper* isString = dynamic_cast<AnimatingKnobStringHelper*>(this);
    if (isString) {
        oldStringValues.resize(times.size());
        int i = 0;
        for (std::list<double>::const_iterator it = times.begin(); it!=times.end(); ++it, ++i) {
            isString->stringFromInterpolatedValue(*it, view, &oldStringValues[i]);
        }
    }


    if (outKeys) {
        outKeys->clear();
    }

    // This may fail if we cannot find a keyframe at the given time
    std::vector<KeyFrame> newKeys;
    if ( !curve->transformKeyframesValueAndTime(times, warp, &newKeys, 0, 0) ) {
        return false;
    }
    if (outKeys) {
        *outKeys = newKeys;
    }


    // update string animation
    if (isString) {
        onKeyframesRemoved(times, ViewSetSpec(view), dimension);
        assert(newKeys.size() == oldStringValues.size());
        for (std::size_t i = 0; i < newKeys.size(); ++i) {
            double ret;
            isString->stringToKeyFrameValue(newKeys[i].getTime(), ViewIdx(view.value()), oldStringValues[i], &ret);
        }
    }

    data->notifyCurveChanged();
    return true;
} // warpValuesAtTimeInternal

bool
KnobHelper::warpValuesAtTime(const std::list<double>& times, ViewSetSpec view,  DimSpec dimension, const Curve::KeyFrameWarp& warp, std::vector<KeyFrame>* outKeys)
{
    if ( times.empty() || !canAnimate() ) {
        return true;
    }

    bool ret = true;
    std::list<ViewIdx> views = getViewsList();
    if (dimension.isAll()) {
        for (int i = 0; i < _imp->dimension; ++i) {
            if (view.isAll()) {
                for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
                    ret &= warpValuesAtTimeInternal(times, *it, DimIdx(i), warp, outKeys);
                }
            } else {
                ViewIdx view_i = getViewIdxFromGetSpec(ViewIdx(view.value()));
                ret &= warpValuesAtTimeInternal(times, view_i, DimIdx(i), warp, outKeys);
            }
        }
    } else {
        if ( ( dimension >= _imp->dimension ) || (dimension < 0) ) {
            throw std::invalid_argument("KnobHelper::warpValuesAtTime(): Dimension out of range");
        }
        if (view.isAll()) {
            for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
                ret &= warpValuesAtTimeInternal(times, *it, DimIdx(dimension), warp, outKeys);
            }
        } else {
            ViewIdx view_i = getViewIdxFromGetSpec(ViewIdx(view.value()));
            ret &= warpValuesAtTimeInternal(times, view_i, DimIdx(dimension), warp, outKeys);
        }
    }

    evaluateValueChange(dimension, TimeValue(times.front()), view, eValueChangedReasonUserEdited);

    return ret;

} // warpValuesAtTime


bool
KnobHelper::cloneCurve(ViewIdx view,
                       DimIdx dimension,
                       const Curve& curve,
                       double offset,
                       const RangeD* range,
                       const StringAnimationManager* stringAnimation)
{
    if (dimension < 0 || dimension >= _imp->dimension) {
        throw std::invalid_argument("KnobHelper::cloneCurve: Dimension out of range");
    }

    KnobDimViewBasePtr data = getDataForDimView(dimension, view);
    if (!data) {
        return false;
    }
    CurvePtr thisCurve = data->animationCurve;
    if (!thisCurve) {
        return false;
    }

    KeyFrameSet oldKeys = thisCurve->getKeyFrames_mt_safe();

    // Check for changes, slower but may avoid computing diff of keyframes
    bool hasChanged = thisCurve->cloneAndCheckIfChanged(curve, offset, range);

    // Clone string animation if necesssary
    if (stringAnimation) {
        StringAnimationManagerPtr thisStringAnimation = getStringAnimation(view);
        if (thisStringAnimation) {
            thisStringAnimation->clone(*stringAnimation, offset, range);
        }
    }

    if (hasChanged) {
        evaluateValueChange(dimension, getHolder()->getTimelineCurrentTime(), view,  eValueChangedReasonUserEdited);
    }


    if (hasChanged) {
        data->notifyCurveChanged();

    }
    return hasChanged;
} // cloneCurve


void
KnobHelper::setInterpolationAtTimesInternal(ViewIdx view, DimIdx dimension, const std::list<double>& times, KeyframeTypeEnum interpolation, std::vector<KeyFrame>* newKeys)
{
    if (!isAnimated(dimension, view)) {
        return;
    }
    if (times.empty()) {
        return;
    }
    CurvePtr curve = getAnimationCurve(view, dimension);
    if (!curve) {
        throw std::runtime_error("KnobHelper::setInterpolationAtTimesInternal: curve is null");
    }

    for (std::list<double>::const_iterator it = times.begin(); it != times.end(); ++it) {
        KeyFrame k;
        if (curve->setKeyFrameInterpolation(interpolation, *it, &k)) {
            if (newKeys) {
                newKeys->push_back(k);
            }
        }
    }
    KnobDimViewBasePtr data = getDataForDimView(dimension, view);
    assert(data);
    data->notifyCurveChanged();

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
    if (dimension.isAll()) {
        for (int i = 0; i < _imp->dimension; ++i) {
            if (view.isAll()) {
                for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
                    setInterpolationAtTimesInternal(*it, DimIdx(i), times, interpolation, newKeys);
                }
            } else {
                ViewIdx view_i = getViewIdxFromGetSpec(ViewIdx(view.value()));
                setInterpolationAtTimesInternal(view_i, DimIdx(i), times, interpolation, newKeys);
            }
        }
    } else {
        if ( ( dimension >= _imp->dimension ) || (dimension < 0) ) {
            throw std::invalid_argument("KnobHelper::setInterpolationAtTimes(): Dimension out of range");
        }
        if (view.isAll()) {
            for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
                setInterpolationAtTimesInternal(*it, DimIdx(dimension), times, interpolation, newKeys);
            }
        } else {
            ViewIdx view_i = getViewIdxFromGetSpec(ViewIdx(view.value()));
            setInterpolationAtTimesInternal(view_i, DimIdx(dimension), times, interpolation, newKeys);
        }
    }

    evaluateValueChange(dimension, times.front(), view, eValueChangedReasonUserEdited);


} // setInterpolationAtTimes

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
    CurvePtr curve = data->animationCurve;
    if (!curve) {
        throw std::runtime_error("KnobHelper::setLeftAndRightDerivativesAtTimeInternal: curve is null");
    }

    int keyIndex = curve->keyFrameIndex(time);
    if (keyIndex == -1) {
        return false;
    }

    curve->setKeyFrameInterpolation(eKeyframeTypeFree, keyIndex);
    curve->setKeyFrameDerivatives(left, right, keyIndex);

    data->notifyCurveChanged();
    return true;
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
    if (dimension.isAll()) {
        for (int i = 0; i < _imp->dimension; ++i) {
            if (view.isAll()) {
                for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
                    ok |= setLeftAndRightDerivativesAtTimeInternal(*it, DimIdx(i), time, left, right);
                }
            } else {
                ViewIdx view_i = getViewIdxFromGetSpec(ViewIdx(view.value()));
                ok |= setLeftAndRightDerivativesAtTimeInternal(view_i, DimIdx(i), time, left, right);
            }
        }
    } else {
        if ( ( dimension >= _imp->dimension ) || (dimension < 0) ) {
            throw std::invalid_argument("KnobHelper::setLeftAndRightDerivativesAtTime(): Dimension out of range");
        }
        if (view.isAll()) {
            for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
                ok |= setLeftAndRightDerivativesAtTimeInternal(*it, DimIdx(dimension), time, left, right);
            }
        } else {
            ViewIdx view_i = getViewIdxFromGetSpec(ViewIdx(view.value()));
            ok |= setLeftAndRightDerivativesAtTimeInternal(view_i, DimIdx(dimension), time, left, right);
        }
    }

    evaluateValueChange(dimension, time, view, eValueChangedReasonUserEdited);

    return ok;
} // KnobHelper::moveDerivativesAtTime

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
    CurvePtr curve = data->animationCurve;
    if (!curve) {
        throw std::runtime_error("KnobHelper::setDerivativeAtTimeInternal: curve is null");
    }

    int keyIndex = curve->keyFrameIndex(time);
    if (keyIndex == -1) {
        return false;
    }

    curve->setKeyFrameInterpolation(eKeyframeTypeBroken, keyIndex);
    if (isLeft) {
        curve->setKeyFrameLeftDerivative(derivative, keyIndex);
    } else {
        curve->setKeyFrameRightDerivative(derivative, keyIndex);
    }

    data->notifyCurveChanged();

    return true;
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
    if (dimension.isAll()) {
        for (int i = 0; i < _imp->dimension; ++i) {
            if (view.isAll()) {
                for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
                    ok |= setDerivativeAtTimeInternal(*it, DimIdx(i), time, derivative, isLeft);
                }
            } else {
                ViewIdx view_i = getViewIdxFromGetSpec(ViewIdx(view.value()));
                ok |= setDerivativeAtTimeInternal(view_i, DimIdx(i), time, derivative, isLeft);
            }
        }
    } else {
        if ( ( dimension >= _imp->dimension ) || (dimension < 0) ) {
            throw std::invalid_argument("KnobHelper::setDerivativeAtTime(): Dimension out of range");
        }
        if (view.isAll()) {
            for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
                ok |= setDerivativeAtTimeInternal(*it, DimIdx(dimension), time, derivative, isLeft);
            }
        } else {
            ViewIdx view_i = getViewIdxFromGetSpec(ViewIdx(view.value()));
            ok |= setDerivativeAtTimeInternal(view_i, DimIdx(dimension), time, derivative, isLeft);
        }
    }

    evaluateValueChange(dimension, time, view, eValueChangedReasonUserEdited);

    return true;
} // KnobHelper::moveDerivativeAtTime

void
KnobHelper::removeAnimationInternal(ViewIdx view, DimIdx dimension)
{
    if (!isAnimated(dimension, view)) {
        return;
    }

    KnobDimViewBasePtr data = getDataForDimView(dimension, view);
    if (!data) {
        return;
    }
    CurvePtr curve = data->animationCurve;
    if (!curve) {
        throw std::runtime_error("KnobHelper::removeAnimationInternal: curve is null");
    }


    KeyFrameSet keys = curve->getKeyFrames_mt_safe();
    if (keys.empty()) {
        return;
    }
    
    // Ensure the underlying values are the values of the current curve at the current time
    copyValuesFromCurve(dimension, view);

    // Notify the user of the keyframes that were removed
    std::list<double> timesRemoved;

    for (KeyFrameSet::const_iterator it = keys.begin(); it!=keys.end(); ++it) {
        timesRemoved.push_back(it->getTime());
    }
    curve->clearKeyFrames();


    if (timesRemoved.empty()) {
        // Nothing changed
        return;
    }


    // Virtual portion
    onKeyframesRemoved(timesRemoved, view, dimension);


    data->notifyCurveChanged();
} // removeAnimationInternal

void
KnobHelper::removeAnimation(ViewSetSpec view, DimSpec dimension, ValueChangedReasonEnum reason)
{

    if (!canAnimate()) {
        return;
    }

    std::list<ViewIdx> views = getViewsList();
    if (dimension.isAll()) {
        for (int i = 0; i < _imp->dimension; ++i) {
            if (view.isAll()) {
                for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
                    removeAnimationInternal(*it, DimIdx(i));
                }
            } else {
                ViewIdx view_i = getViewIdxFromGetSpec(ViewIdx(view.value()));
                removeAnimationInternal(view_i, DimIdx(i));
            }
        }
    } else {
        if ( ( dimension >= _imp->dimension ) || (dimension < 0) ) {
            throw std::invalid_argument("KnobHelper::removeAnimation(): Dimension out of range");
        }
        if (view.isAll()) {
            for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
                removeAnimationInternal(*it, DimIdx(dimension));
            }
        } else {
            ViewIdx view_i = getViewIdxFromGetSpec(ViewIdx(view.value()));
            removeAnimationInternal(view_i, DimIdx(dimension));
        }
    }
    
    
    evaluateValueChange(dimension, getCurrentTime_TLS(), view, reason);
} // removeAnimation

NATRON_NAMESPACE_EXIT
