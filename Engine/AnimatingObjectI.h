/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2020 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#ifndef Engine_AnimatingObjectI_h
#define Engine_AnimatingObjectI_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <vector>
#include <list>
#include <string>
#include <set>
#include <map>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#endif

#include "Global/GlobalDefines.h"
#include "Engine/DimensionIdx.h"
#include "Engine/Curve.h"
#include "Engine/TimeValue.h"
#include "Engine/ViewIdx.h"
#include "Engine/Variant.h"

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

template <typename T>
class TimeValuePair
{
public:

    TimeValue time;
    T value;

    TimeValuePair(TimeValue t,
                  const T& v)
    : time(t)
    , value(v)
    {

    }
};

typedef TimeValuePair<int> IntTimeValuePair;
typedef TimeValuePair<double> DoubleTimeValuePair;
typedef TimeValuePair<bool> BoolTimeValuePair;
typedef TimeValuePair<std::string> StringTimeValuePair;

// A time value pair that can adapt at runtime
struct VariantTimeValuePair
{
    TimeValue time;
    Variant value;
};

struct VariantTimeValuePair_Compare
{
    bool operator() (const VariantTimeValuePair& lhs, const VariantTimeValuePair& rhs) const
    {
        return lhs.time < rhs.time;
    }
};


template <typename T>
inline T variantToType(const Variant& v);

template <>
inline int variantToType(const Variant& v)
{
    return v.toInt();
}

template <>
inline bool variantToType(const Variant& v)
{
    return v.toBool();
}

template <>
inline double variantToType(const Variant& v)
{
    return v.toDouble();
}

template <>
inline std::string variantToType(const Variant& v)
{
    return v.toString().toStdString();
}

template <typename T>
inline TimeValuePair<T> variantTimeValuePairToTemplated(const VariantTimeValuePair& v)
{
     return TimeValuePair<T>(v.time, variantToType<T>(v.value));
}

// A pair identifying a curve for a given view/dimension
struct DimensionViewPair
{
    DimIdx dimension;
    ViewIdx view;
};

typedef std::vector<std::pair<DimensionViewPair, std::list<DoubleTimeValuePair> > > PerCurveDoubleValuesList;
typedef std::vector<std::pair<DimensionViewPair, std::list<IntTimeValuePair> > > PerCurveIntValuesList;
typedef std::vector<std::pair<DimensionViewPair, std::list<BoolTimeValuePair> > > PerCurveBoolValuesList;
typedef std::vector<std::pair<DimensionViewPair, std::list<StringTimeValuePair> > > PerCurveStringValuesList;


struct DimensionViewPairCompare
{
    bool operator() (const DimensionViewPair& lhs, const DimensionViewPair& rhs) const
    {
        if (lhs.view < rhs.view) {
            return true;
        } else if (lhs.view > rhs.view) {
            return false;
        } else {
            return lhs.dimension < rhs.dimension;
        }
    }
};
typedef std::set<DimensionViewPair, DimensionViewPairCompare> DimensionViewPairSet;

typedef std::map<DimensionViewPair, std::list<KeyFrame>, DimensionViewPairCompare> PerDimViewKeyFramesMap;


class SplittableViewsI
{
    struct Implementation;

public:

    SplittableViewsI();

    SplittableViewsI(const boost::shared_ptr<SplittableViewsI>& other, const FrameViewRenderKey& key);

    virtual ~SplittableViewsI();
    
    /**
     * @brief Returns true if this object can support multi-view animation. When supported, the object may have a different
     * animation for each view.
     **/
    virtual bool canSplitViews() const = 0;

    /**
     * @brief Get list of views that are split off in the animating object.
     * The ViewIdx(0) is always present and represents the first view in the
     * list of the project views.
     * User can split views by calling splitView in which case they can be attributed
     * a new animation different from the main view. To remove the custom animation for a view
     * call unSplitView which will make the view listen to the main view again
     **/
    std::list<ViewIdx> getViewsList() const WARN_UNUSED_RETURN;

    /**
     * @brief Split the given view in the storage off the main view so that the user can give it
     * a different animation.
     * @return True if the view was successfully split, false otherwise.
     * This must be overloaded by sub-classes to split new data structures when called.
     * The base class version should always be called first and if the return value is false it should
     * exit immediately.
     **/
    virtual bool splitView(ViewIdx view);

    /**
     * @brief Unsplit a view that was previously split with splitView. After this call the animation
     * for the view will be the one of the main view.
     * @return true if the view was unsplit, false otherwise.
     **/
    virtual bool unSplitView(ViewIdx view);

    /**
     * @brief Convenience function, same as calling unSplitView for all views returned by
     * getViewsList except the ViewIdx(0)
     **/
    void unSplitAllViews();


    /**
     * @brief Helper function to use in any getter/setter function when the user gives a ViewIdx
     * to figure out which view to address if the view does not exists.
     **/
    ViewIdx checkIfViewExistsOrFallbackMainView(ViewIdx view) const WARN_UNUSED_RETURN;



private:

    boost::scoped_ptr<Implementation> _imp;

};

class AnimatingObjectI : public SplittableViewsI
{
public:

    
    AnimatingObjectI()
    : SplittableViewsI()
    {

    }

    AnimatingObjectI(const boost::shared_ptr<AnimatingObjectI>& other, const FrameViewRenderKey& key)
    : SplittableViewsI(other ,key)
    {

    }

    virtual ~AnimatingObjectI();

    /**
     * @brief Returns the internal value that is represented by keyframes themselves.
     **/
    virtual CurveTypeEnum getKeyFrameDataType() const = 0;

    /**
     * @brief Returns the number of dimensions in the object that can animate
     **/
    virtual int getNDimensions() const { return 1; }


    /**
     * @brief Returns true if this object can support multi-view animation. When supported, the object may have a different
     * animation for each view.
     **/
    virtual bool canSplitViews() const OVERRIDE = 0;


    /**
     * @brief Must return the current view in the object context. If the calling thread
     * is a render thread, this can be the view being rendered by this thread, otherwise this
     * can be the current view visualized by the UI.
     **/
    virtual ViewIdx getCurrentRenderView() const = 0;


    class SetKeyFrameArgs
    {
    public:

        SetKeyFrameArgs();

        void operator=(const SetKeyFrameArgs &o);

        // The view(s) on which to set the keyframe.
        // If set to all, all views on the knob will be modified.
        // If set to current and views are split-off only the "current" view
        // (as in the current on-going render if called from a render thread) will be set, otherwise all views are set.
        // If set to a view index and the view is split-off or it corresponds to the view 0 (main view) then only the view
        // at the given index will receive the change, otherwise no change occurs.
        ViewSetSpec view;

        // The dimension(s) on which to set the keyframe
        DimSpec dimension;

        // Should we set a property or a value on the keyframe
        SetKeyFrameFlags flags;

        // The value change reason
        ValueChangedReasonEnum reason;

        // True if we want implementation to call knobChanged handler even if nothing was performed by the call
        bool callKnobChangedHandlerEvenIfNothingChanged;
    };

    /**
     * @brief Set a keyframe on the curve at the given view and dimension.
     * @return A status that reports the kind of modification operated on the object
     **/
    virtual ValueChangedReturnCodeEnum setKeyFrame(const SetKeyFrameArgs& args, const KeyFrame& key);

    /**
     * @brief Set multiple keyframes on the curve at the given view and dimension.
     * @param newKey[out] If non null, this will be set to the new keyframes in return
     **/
    virtual void setMultipleKeyFrames(const SetKeyFrameArgs& args, const std::list<KeyFrame>& keys);

    /**
     * @brief Set a keyframe across multiple dimensions at once.
     * @param dimensionStartIndex The dimension to start from. The caller should ensure that dimensionStartIndex + values.size() <= getNDimensions()
     * @param retCodes[out] If non null, each return code for each dimension will be stored there. It will be of the same size as the values parameter.
     **/
    virtual void setKeyFramesAcrossDimensions(const SetKeyFrameArgs& args, const std::vector<KeyFrame>& values, DimIdx dimensionStartIndex = DimIdx(0), std::vector<ValueChangedReturnCodeEnum>* retCodes = 0);


    ///////////////////////// Curve access


    /**
     * @brief Returns a pointer to the underlying animation curve for the given view/dimension
     **/
    virtual CurvePtr getAnimationCurve(ViewIdx idx, DimIdx dimension) const = 0;

    /**
     * @brief Places in time the keyframe time at the given index.
     * If it exists the function returns true, false otherwise.
     * @param view The view of the corresponding curve. If view is current, then the current view
     * in the thread-local storage (if while rendering) will be used, otherwise the view must correspond
     * to a valid view index.
     **/
    virtual bool getKeyFrameTime(ViewIdx view, int index, DimIdx dimension, TimeValue* time) const ;

    /**
     * @brief Convenience function, does the same as getKeyFrameWithIndex but returns the last
     * keyframe.
     * @param view The view of the corresponding curve. If view is current, then the current view
     * in the thread-local storage (if while rendering) will be used, otherwise the view must correspond
     * to a valid view index.
     **/
    virtual bool getLastKeyFrameTime(ViewIdx view, DimIdx dimension, TimeValue* time) const ;

    /**
     * @brief Convenience function, does the same as getKeyFrameWithIndex but returns the first
     * keyframe.
     * @param view The view of the corresponding curve. If view is current, then the current view
     * in the thread-local storage (if while rendering) will be used, otherwise the view must correspond
     * to a valid view index.
     **/
    virtual bool getFirstKeyFrameTime(ViewIdx view, DimIdx dimension, TimeValue* time) const ;

    /**
     * @brief Returns the count of keyframes in the given dimension.
     * @param view The view of the corresponding curve. If view is current, then the current view
     * in the thread-local storage (if while rendering) will be used, otherwise the view must correspond
     * to a valid view index.
     **/
    virtual int getKeyFramesCount(ViewIdx view, DimIdx dimension) const ;

    /**
     * @brief Returns the index of the keyframe with a time lesser than the given time that is the closest
     * to the given time.
     * If no keyframe greater or equal to the given time could be found, returns -1
     **/
    virtual bool getPreviousKeyFrameTime(ViewIdx view, DimIdx dimension, TimeValue time, TimeValue* keyframeTime) const ;

    /**
     * @brief Returns the index of the keyframe with a time greater than the given time that is the closest to the given time.
     * If no keyframe greater or equal to the given time could be found, returns -1
     **/
    virtual bool getNextKeyFrameTime(ViewIdx view, DimIdx dimension, TimeValue time, TimeValue* keyframeTime) const ;

    /**
     * @brief Returns the keyframe index if there's any keyframe in the curve
     * at the given dimension and the given time. -1 otherwise.
     * @param view The view of the corresponding curve. If view is current, then the current view
     * in the thread-local storage (if while rendering) will be used, otherwise the view must correspond
     * to a valid view index.
     **/
    virtual int getKeyFrameIndex(ViewIdx view, DimIdx dimension, TimeValue time) const ;


    ///////////////////////// Curve manipulation

    /**
     * @brief Copies all the animation of *curve* into the animation curve in the given dimension and view.
     * @param offset All keyframes of the original *curve* will be offset by this amount before being copied to this curve
     * @param range If non NULL Only keyframes within the given range will be copied
     * @param stringAnimation If non NULL, the implementation should also clone any string animation with the given parameters
     * @return True if something has changed, false otherwise
     **/
    virtual bool cloneCurve(ViewIdx view, DimIdx dimension, const Curve& curve, double offset, const RangeD* range) = 0;

    /**
     * @brief Removes a keyframe at the given time if it matches any on the curve for the given view and dimension.
     * @param view If set to all, all views on the knob will be modified.
     * If set to current and views are split-off only the "current" view
     * (as in the current on-going render if called from a render thread) will be set, otherwise all views are set.
     * If set to a view index and the view is split-off or it corresponds to the view 0 (main view) then only the view
     * at the given index will receive the change, otherwise no change occurs.
     * The default implementation just calls deleteValuesAtTime.
     **/
    virtual void deleteValueAtTime(TimeValue time, ViewSetSpec view, DimSpec dimension, ValueChangedReasonEnum reason);

    /**
     * @brief Removes the keyframes at the given times if they exist on the curve for the given view and dimension.
     * @param view If set to all, all views on the knob will be modified.
     * If set to current and views are split-off only the "current" view
     * (as in the current on-going render if called from a render thread) will be set, otherwise all views are set.
     * If set to a view index and the view is split-off or it corresponds to the view 0 (main view) then only the view
     * at the given index will receive the change, otherwise no change occurs.
     **/
    virtual void deleteValuesAtTime(const std::list<double>& times, ViewSetSpec view, DimSpec dimension, ValueChangedReasonEnum reason) = 0;

    /**
     * @brief Similar to deleteValuesAtTime, except that the given keyframe times are kept whereas all other keyframes are deleted.
     * @param range If non NULL, any keyframes outside this range will not be deleted.
     **/
    void deleteValuesExceptAtTime(const std::set<double>& keyframesToIgnore, const RangeD* range, ViewSetSpec view, DimSpec dimension, ValueChangedReasonEnum reason);

    /**
     * @brief Remove animation on the object before the given time (excluded). 
     * @param keyframesToIgnore All keyframes included in this list will not be removed
     **/
    void deleteValuesBeforeTime(const std::set<double>& keyframesToIgnore, TimeValue time, ViewSetSpec view, DimSpec dimension, ValueChangedReasonEnum reason);

    /**
     * @brief Remove animation on the object after the given time (excluded).
     * @param keyframesToIgnore All keyframes included in this list will not be removed
     **/
    void deleteValuesAfterTime(const std::set<double>& keyframesToIgnore, TimeValue time, ViewSetSpec view, DimSpec dimension, ValueChangedReasonEnum reason);

    /**
     * @brief If a keyframe at the given time exists in the curve at the given view and dimension then it will be moved by dt in time
     * and dv in value. 
     * @param view If set to all, all views on the knob will be modified.
     * If set to current and views are split-off only the "current" view
     * (as in the current on-going render if called from a render thread) will be set, otherwise all views are set.
     * If set to a view index and the view is split-off or it corresponds to the view 0 (main view) then only the view
     * at the given index will receive the change, otherwise no change occurs.
     * @param newKey[out] If non null, the new keyframe in return will be assigned to this parameter.
     * @returns True If the keyframe could be moved, false otherwise
     * The default implementation just calls moveValuesAtTime.
     **/
    virtual bool moveValueAtTime(TimeValue time, ViewSetSpec view,  DimSpec dimension, double dt, double dv, KeyFrame* newKey = 0);

    /**
     * @brief If keyframes at the given times exist in the curve at the given view and dimension then they will be moved by dt in time
     * and dv in value.
     * @param view If set to all, all views on the knob will be modified.
     * If set to current and views are split-off only the "current" view
     * (as in the current on-going render if called from a render thread) will be set, otherwise all views are set.
     * If set to a view index and the view is split-off or it corresponds to the view 0 (main view) then only the view
     * at the given index will receive the change, otherwise no change occurs.
     * @param keyframes[out] If non null, the new keyframes in return will be assigned to this parameter.
     * @returns True If all the keyframe could be moved, false otherwise
     * Note that if this function succeeds, it is guaranteed that all keyframes have moved.
     * The default implementation just calls warpValuesAtTime.
     **/
    virtual bool moveValuesAtTime(const std::list<double>& times, ViewSetSpec view,  DimSpec dimension, double dt, double dv, std::vector<KeyFrame>* keyframes = 0) ;

    /**
     * @brief If a keyframe at the given time exists in the curve at the given view and dimension then it will be warped by the given
     * affine transform, assuming the X coordinate is the time and the Y coordinate is the value. Z always equals 1.
     * @param view If set to all, all views on the knob will be modified.
     * If set to current and views are split-off only the "current" view
     * (as in the current on-going render if called from a render thread) will be set, otherwise all views are set.
     * If set to a view index and the view is split-off or it corresponds to the view 0 (main view) then only the view
     * at the given index will receive the change, otherwise no change occurs.
     * @param newKey[out] If non null, the new keyframe in return will be assigned to this parameter.
     * @returns True If the keyframe could be warped, false otherwise
     * The default implementation just calls transformValuesAtTime.
     **/
    virtual bool transformValueAtTime(TimeValue time, ViewSetSpec view,  DimSpec dimension, const Transform::Matrix3x3& matrix, KeyFrame* newKey);

    /**
     * @brief If keyframes at the given times exist in the curve at the given view and dimension then they will be warped by the given
     * affine transform, assuming the X coordinate is the time and the Y coordinate is the value. Z always equals 1.
     * @param view If set to all, all views on the knob will be modified.
     * If set to current and views are split-off only the "current" view
     * (as in the current on-going render if called from a render thread) will be set, otherwise all views are set.
     * If set to a view index and the view is split-off or it corresponds to the view 0 (main view) then only the view
     * at the given index will receive the change, otherwise no change occurs.
     * @param newKey[out] If non null, the new keyframes in return will be assigned to this parameter.
     * @returns True If the keyframes could be warped, false otherwise
     * Note that if this function succeeds, it is guaranteed that all keyframes have been warped.
     * The default implementation just calls warpValuesAtTime.
     **/
    virtual bool transformValuesAtTime(const std::list<double>& times, ViewSetSpec view,  DimSpec dimension, const Transform::Matrix3x3& matrix, std::vector<KeyFrame>* keyframes) ;

    /**
     * @brief If keyframes at the given times exist in the curve at the given view and dimension then they will be warped by the given
     * warp, assuming the X coordinate is the time and the Y coordinate is the value. Z always equals 1.
     * @param view If set to all, all views on the knob will be modified.
     * If set to current and views are split-off only the "current" view
     * (as in the current on-going render if called from a render thread) will be set, otherwise all views are set.
     * If set to a view index and the view is split-off or it corresponds to the view 0 (main view) then only the view
     * at the given index will receive the change, otherwise no change occurs.
     * return false, otherwise it will just substitute the existing keyframe
     * @param newKey[out] If non null, the new keyframes in return will be assigned to this parameter.
     * @returns True If the keyframes could be warped, false otherwise
     * Note that if this function succeeds, it is guaranteed that all keyframes have been warped.
     **/
    virtual bool warpValuesAtTime(const std::list<double>& times, ViewSetSpec view,  DimSpec dimension, const Curve::KeyFrameWarp& warp, std::vector<KeyFrame>* keyframes = 0) = 0;

    /**
     * @brief Removes all keyframes on the object for the given view in the given dimension.
     * @param view If set to all, all views on the knob will be modified.
     * If set to current and views are split-off only the "current" view
     * (as in the current on-going render if called from a render thread) will be set, otherwise all views are set.
     * If set to a view index and the view is split-off or it corresponds to the view 0 (main view) then only the view
     * at the given index will receive the change, otherwise no change occurs.
     * @param dimension If set to all, all dimensions will have their animation removed, otherwise
     * only the dimension at the given index will have its animation removed.
     * @param reason Identifies who called the function to optimize redraws of the Gui
     * The default implementation just calls removeAnimationAcrossDimensions.
     **/
    virtual void removeAnimation(ViewSetSpec view, DimSpec dimension, ValueChangedReasonEnum reason) = 0;


    /**
     * @brief Removes animation on the curve at the given view and dimension before the given time. 
     * @param view If set to all, all views on the knob will be modified.
     * If set to current and views are split-off only the "current" view
     * (as in the current on-going render if called from a render thread) will be set, otherwise all views are set.
     * If set to a view index and the view is split-off or it corresponds to the view 0 (main view) then only the view
     * at the given index will receive the change, otherwise no change occurs.
     * If a keyframe at the given time exists it is not removed.
     * @param dimension If set to all, all dimensions will have their animation removed, otherwise
     * only the dimension at the given index will have its animation removed.
     **/
    virtual void deleteAnimationBeforeTime(TimeValue time, ViewSetSpec view, DimSpec dimension) = 0;

    /**
     * @brief Removes animation on the curve at the given view and dimension after the given time.
     * @param view If set to all, all views on the knob will be modified.
     * If set to current and views are split-off only the "current" view
     * (as in the current on-going render if called from a render thread) will be set, otherwise all views are set.
     * If set to a view index and the view is split-off or it corresponds to the view 0 (main view) then only the view
     * at the given index will receive the change, otherwise no change occurs.
     * If a keyframe at the given time exists it is not removed.
     * @param dimension If set to all, all dimensions will have their animation removed, otherwise
     * only the dimension at the given index will have its animation removed.
     **/
    virtual void deleteAnimationAfterTime(TimeValue time, ViewSetSpec view, DimSpec dimension) = 0;

    /**
     * @brief Set the interpolation type for the given keyframe on the curve at the given dimension and view
     * @param view If set to all, all views on the knob will be modified.
     * If set to current and views are split-off only the "current" view
     * (as in the current on-going render if called from a render thread) will be set, otherwise all views are set.
     * If set to a view index and the view is split-off or it corresponds to the view 0 (main view) then only the view
     * at the given index will receive the change, otherwise no change occurs.
     * @param newKey[out] If non null, the new keyframe in return will be assigned to this parameter
     * The default implementation just calls setInterpolationAtTimes.
     **/
    virtual void setInterpolationAtTime(ViewSetSpec view, DimSpec dimension, TimeValue time, KeyframeTypeEnum interpolation, KeyFrame* newKey = 0);
    
    /**
     * @brief Set the interpolation type for the given keyframes on the curve at the given dimension and view
     * @param view If set to all, all views on the knob will be modified.
     * If set to current and views are split-off only the "current" view
     * (as in the current on-going render if called from a render thread) will be set, otherwise all views are set.
     * If set to a view index and the view is split-off or it corresponds to the view 0 (main view) then only the view
     * at the given index will receive the change, otherwise no change occurs.
     * @param newKey[out] If non null, the new keyframes in return will be assigned to this parameter
     **/
    virtual void setInterpolationAtTimes(ViewSetSpec view, DimSpec dimension, const std::list<double>& times, KeyframeTypeEnum interpolation, std::vector<KeyFrame>* newKeys = 0) = 0;

    /**
     * @brief Set the left and right derivatives of the control point at the given time on the curve at the given view and dimension
     * @param view If set to all, all views on the knob will be modified.
     * If set to current and views are split-off only the "current" view
     * (as in the current on-going render if called from a render thread) will be set, otherwise all views are set.
     * If set to a view index and the view is split-off or it corresponds to the view 0 (main view) then only the view
     * at the given index will receive the change, otherwise no change occurs.
     * @param left The new value to set for the left derivative of the keyframe at the given time
     * @param right The new value to set for the right derivative of the keyframe at the given time
     **/
    virtual bool setLeftAndRightDerivativesAtTime(ViewSetSpec view, DimSpec dimension, TimeValue time, double left, double right) = 0;

    /**
     * @brief Set the left or right derivative of the control point at the given time on the curve at the given view and dimension
     * @param view If set to all, all views on the knob will be modified.
     * If set to current and views are split-off only the "current" view
     * (as in the current on-going render if called from a render thread) will be set, otherwise all views are set.
     * If set to a view index and the view is split-off or it corresponds to the view 0 (main view) then only the view
     * at the given index will receive the change, otherwise no change occurs.
     * @param derivative The new value to set for the derivative of the keyframe at the given time
     * @param isLeft If true, the left derivative will be set, otherwise the right derivative will be set
     **/
    virtual bool setDerivativeAtTime(ViewSetSpec view, DimSpec dimension, TimeValue time, double derivative, bool isLeft) = 0;



};

NATRON_NAMESPACE_EXIT

#endif // Engine_AnimatingObjectI_h
