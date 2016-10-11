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


#ifndef ANIMATINGOBJECTI_H
#define ANIMATINGOBJECTI_H


// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <vector>
#include <list>
#include <string>

#include "Global/GlobalDefines.h"
#include "Engine/EngineFwd.h"
#include "Engine/Curve.h"
#include "Engine/ViewIdx.h"
#include "Engine/Variant.h"

NATRON_NAMESPACE_ENTER;

template <typename T>
class TimeValuePair
{
public:

    double time;
    T value;

    TimeValuePair(double t,
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
    double time;
    Variant value;
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
inline TimeValuePair<T> variantTimevaluePairToTemplated(const VariantTimeValuePair& v)
{
     return TimeValuePair<T>(v.time, variantToType<T>(v.value));
}

class AnimatingObjectI
{
public:

    enum KeyframeDataTypeEnum
    {
        // Keyframe is just a time - no value
        eKeyframeDataTypeNone,

        // Keyframe value is an int
        eKeyframeDataTypeInt,

        // Keyframe value is a double
        eKeyframeDataTypeDouble,

        // Keyframe value is a bool
        eKeyframeDataTypeBool,

        // Keyframe value is a string
        eKeyframeDataTypeString
    };

    AnimatingObjectI();

    /**
     * @brief Returns the internal value that is represented by keyframes themselves.
     **/
    virtual KeyframeDataTypeEnum getKeyFrameDataType() const = 0;

    /**
     * @brief Returns the number of dimensions in the object that can animate
     **/
    virtual int getNDimensions() const { return 1; }

    /**
     * @brief Returns a pointer to the underlying animation curve
     **/
    virtual CurvePtr getAnimationCurve(ViewIdx idx, int dimension) const = 0;

    /**
     * @brief For an object that supports animating strings, this is should return a pointer to it
     **/
    virtual StringAnimationManagerPtr getStringAnimation() const { return StringAnimationManagerPtr(); };

    ////////////////////////// Integer based animating objects

    /**
     * @brief Set a keyframe on the curve at the given view and dimension. This is only relevant on curves of type int.
     * @return A status that reports the kind of modification operated on the object
     **/
    virtual ValueChangedReturnCodeEnum setIntValueAtTime(double time, int value, ViewSpec view, int dimension, ValueChangedReasonEnum reason, KeyFrame* newKey = 0) = 0;

    /**
     * @brief Set multiple keyframes on the curve at the given view and dimension. This is only relevant on curves of type int.
     * @param newKey[out] If non null, this will be set to the new keyframe in return
     **/
    virtual void setMultipleIntValueAtTime(const std::list<IntTimeValuePair>& keys, ViewSpec view, int dimension, ValueChangedReasonEnum reason, std::vector<KeyFrame>* newKey = 0) = 0;

    /**
     * @brief Set a keyframe across multiple dimensions at once. This is only relevant on curves of type int.
     * @param dimensionStartIndex The dimension to start from. The caller should ensure that dimensionStartIndex + values.size() <= getNDimensions()
     * @param retCodes[out] If non null, each return code for each dimension will be stored there. It will be of the same size as the values parameter.
     **/
    virtual void setIntValueAtTimeAcrossDimensions(double time, const std::vector<int>& values, int dimensionStartIndex, ViewSpec view, ValueChangedReasonEnum reason, std::vector<ValueChangedReturnCodeEnum>* retCodes = 0);

    /**
     * @brief Set multiple keyframes across multiple dimensions on the curve at the given view and dimension. This is only relevant on curves of type int.
     * @param dimensionStartIndex The dimension to start from. The caller should ensure that dimensionStartIndex + keysPerDimension.size() <= getNDimensions()
     * Note: as multiple keyframes are set across multiple dimensions this makes it hard to return all status codes so if the caller
     * really needs the status code then another function giving that result should be considered.
     **/
    virtual void setMultipleIntValueAtTimeAcrossDimensions(const std::vector<std::list<IntTimeValuePair> >& keysPerDimension, int dimensionStartIndex, ViewSpec view, ValueChangedReasonEnum reason) = 0;

    ////////////////////////// Double based animating objects

    /**
     * @brief Set a keyframe on the curve at the given view and dimension. This is only relevant on curves of type double.
     * @return A status that reports the kind of modification operated on the object
     **/
    virtual ValueChangedReturnCodeEnum setDoubleValueAtTime(double time, double value, ViewSpec view, int dimension, ValueChangedReasonEnum reason, KeyFrame* newKey = 0) = 0;

    /**
     * @brief Set multiple keyframes on the curve at the given view and dimension. This is only relevant on curves of type double.
     * @param newKey[out] If non null, this will be set to the new keyframe in return
     **/
    virtual void setMultipleDoubleValueAtTime(const std::list<DoubleTimeValuePair>& keys, ViewSpec view, int dimension, ValueChangedReasonEnum reason, std::vector<KeyFrame>* newKey = 0) = 0;

    /**
     * @brief Set a keyframe across multiple dimensions at once. This is only relevant on curves of type double.
     * @param dimensionStartIndex The dimension to start from. The caller should ensure that dimensionStartIndex + values.size() <= getNDimensions()
     * @param retCodes[out] If non null, each return code for each dimension will be stored there. It will be of the same size as the values parameter.
     **/
    virtual void setDoubleValueAtTimeAcrossDimensions(double time, const std::vector<double>& values, int dimensionStartIndex, ViewSpec view, ValueChangedReasonEnum reason, std::vector<ValueChangedReturnCodeEnum>* retCodes = 0) = 0;

    /**
     * @brief Set multiple keyframes across multiple dimensions on the curve at the given view and dimension. This is only relevant on curves of type double.
     * @param dimensionStartIndex The dimension to start from. The caller should ensure that dimensionStartIndex + keysPerDimension.size() <= getNDimensions()
     * Note: as multiple keyframes are set across multiple dimensions this makes it hard to return all status codes so if the caller
     * really needs the status code then another function giving that result should be considered.
     **/
    virtual void setMultipleDoubleValueAtTimeAcrossDimensions(const std::vector<std::list<DoubleTimeValuePair> >& keysPerDimension, int dimensionStartIndex, ViewSpec view, ValueChangedReasonEnum reason) = 0;

    ////////////////////////// Bool based animating objects


    /**
     * @brief Set a keyframe on the curve at the given view and dimension. This is only relevant on curves of type bool.
     * @return A status that reports the kind of modification operated on the object
     **/
    virtual ValueChangedReturnCodeEnum setBoolValueAtTime(double time, bool value, ViewSpec view, int dimension, ValueChangedReasonEnum reason, KeyFrame* newKey = 0) = 0;

    /**
     * @brief Set multiple keyframes on the curve at the given view and dimension. This is only relevant on curves of type bool.
     * @param newKey[out] If non null, this will be set to the new keyframe in return
     **/
    virtual void setMultipleBoolValueAtTime(const std::list<BoolTimeValuePair>& keys, ViewSpec view, int dimension, ValueChangedReasonEnum reason, std::vector<KeyFrame>* newKey = 0) = 0;

    /**
     * @brief Set a keyframe across multiple dimensions at once. This is only relevant on curves of type bool.
     * @param dimensionStartIndex The dimension to start from. The caller should ensure that dimensionStartIndex + values.size() <= getNDimensions()
     * @param retCodes[out] If non null, each return code for each dimension will be stored there. It will be of the same size as the values parameter.
     **/
    virtual void setBoolValueAtTimeAcrossDimensions(double time, const std::vector<bool>& values, int dimensionStartIndex, ViewSpec view, ValueChangedReasonEnum reason, std::vector<ValueChangedReturnCodeEnum>* retCodes = 0) = 0;

    /**
     * @brief Set multiple keyframes across multiple dimensions on the curve at the given view and dimension. This is only relevant on curves of type bool.
     * @param dimensionStartIndex The dimension to start from. The caller should ensure that dimensionStartIndex + keysPerDimension.size() <= getNDimensions()
     * Note: as multiple keyframes are set across multiple dimensions this makes it hard to return all status codes so if the caller
     * really needs the status code then another function giving that result should be considered.
     **/
    virtual void setMultipleBoolValueAtTimeAcrossDimensions(const std::vector<std::list<BoolTimeValuePair> >& keysPerDimension, int dimensionStartIndex, ViewSpec view, ValueChangedReasonEnum reason) = 0;


    ////////////////////////// String based animating objects

    /**
     * @brief Set a keyframe on the curve at the given view and dimension. This is only relevant on curves of type string.
     * @return A status that reports the kind of modification operated on the object
     **/
    virtual ValueChangedReturnCodeEnum setStringValueAtTime(double time, const std::string& value, ViewSpec view, int dimension, ValueChangedReasonEnum reason, KeyFrame* newKey = 0) = 0;

    /**
     * @brief Set multiple keyframes on the curve at the given view and dimension. This is only relevant on curves of type string.
     * @param newKey[out] If non null, this will be set to the new keyframe in return
     **/
    virtual void setMultipleStringValueAtTime(const std::list<StringTimeValuePair>& keys, ViewSpec view, int dimension, ValueChangedReasonEnum reason, std::vector<KeyFrame>* newKey = 0);

    /**
     * @brief Set a keyframe across multiple dimensions at once. This is only relevant on curves of type string.
     * @param dimensionStartIndex The dimension to start from. The caller should ensure that dimensionStartIndex + values.size() <= getNDimensions()
     * @param retCodes[out] If non null, each return code for each dimension will be stored there. It will be of the same size as the values parameter.
     **/
    virtual void setStringValueAtTimeAcrossDimensions(double time, const std::vector<std::string>& values, int dimensionStartIndex, ViewSpec view, ValueChangedReasonEnum reason, std::vector<ValueChangedReturnCodeEnum>* retCodes = 0);

    /**
     * @brief Set multiple keyframes across multiple dimensions on the curve at the given view and dimension. This is only relevant on curves of type string.
     * @param dimensionStartIndex The dimension to start from. The caller should ensure that dimensionStartIndex + keysPerDimension.size() <= getNDimensions()
     * Note: as multiple keyframes are set across multiple dimensions this makes it hard to return all status codes so if the caller
     * really needs the status code then another function giving that result should be considered.
     **/
    virtual void setMultipleStringValueAtTimeAcrossDimensions(const std::vector<std::list<StringTimeValuePair> >& keysPerDimension, int dimensionStartIndex, ViewSpec view, ValueChangedReasonEnum reason) = 0;

    ///////////////////////// Curve manipulation

    /**
     * @brief Copies all the animation of *curve* into the animation curve in the given dimension and view.
     * @param offset All keyframes of the original *curve* will be offset by this amount before being copied to this curve
     * @param range If non NULL Only keyframes within the given range will be copied
     * @param stringAnimation If non NULL, the implementation should also clone any string animation with the given parameters
     * @return True if something has changed, false otherwise
     **/
    virtual bool cloneCurve(ViewSpec view, int dimension, const Curve& curve, double offset, const RangeD* range, const StringAnimationManager* stringAnimation, CurveChangeReason reason) = 0;

    /**
     * @brief Removes a keyframe at the given time if it matches any on the curve for the given view and dimension.
     * The default implementation just calls deleteValuesAtTime.
     **/
    virtual void deleteValueAtTime(CurveChangeReason curveChangeReason, double time, ViewSpec view, int dimension);

    /**
     * @brief Removes the keyframes at the given times if they exist on the curve for the given view and dimension.
     **/
    virtual void deleteValuesAtTime(CurveChangeReason curveChangeReason, const std::list<double>& times, ViewSpec view, int dimension) = 0;

    /**
     * @brief If a keyframe at the given time exists in the curve at the given view and dimension then it will be moved by dt in time
     * and dv in value. 
     * @param newKey[out] If non null, the new keyframe in return will be assigned to this parameter.
     * @returns True If the keyframe could be moved, false otherwise
     * The default implementation just calls moveValuesAtTime.
     **/
    virtual bool moveValueAtTime(CurveChangeReason reason, double time, ViewSpec view,  int dimension, double dt, double dv, KeyFrame* newKey = 0);

    /**
     * @brief If keyframes at the given times exist in the curve at the given view and dimension then they will be moved by dt in time
     * and dv in value.
     * @param keyframes[out] If non null, the new keyframes in return will be assigned to this parameter.
     * @returns True If all the keyframe could be moved, false otherwise
     * Note that if this function succeeds, it is guaranteed that all keyframes have moved.
     * The default implementation just calls warpValuesAtTime.
     **/
    virtual bool moveValuesAtTime(CurveChangeReason reason, const std::list<double>& times, ViewSpec view,  int dimension, double dt, double dv, std::vector<KeyFrame>* keyframes = 0) ;

    /**
     * @brief If a keyframe at the given time exists in the curve at the given view and dimension then it will be warped by the given
     * affine transform, assuming the X coordinate is the time and the Y coordinate is the value. Z always equals 1.
     * @param newKey[out] If non null, the new keyframe in return will be assigned to this parameter.
     * @returns True If the keyframe could be warped, false otherwise
     * The default implementation just calls transformValuesAtTime.
     **/
    virtual bool transformValueAtTime(CurveChangeReason curveChangeReason, double time, ViewSpec view,  int dimension, const Transform::Matrix3x3& matrix, KeyFrame* newKey);

    /**
     * @brief If keyframes at the given times exist in the curve at the given view and dimension then they will be warped by the given
     * affine transform, assuming the X coordinate is the time and the Y coordinate is the value. Z always equals 1.
     * @param newKey[out] If non null, the new keyframes in return will be assigned to this parameter.
     * @returns True If the keyframes could be warped, false otherwise
     * Note that if this function succeeds, it is guaranteed that all keyframes have been warped.
     * The default implementation just calls warpValuesAtTime.
     **/
    virtual bool transformValuesAtTime(CurveChangeReason curveChangeReason, const std::list<double>& times, ViewSpec view,  int dimension, const Transform::Matrix3x3& matrix, std::vector<KeyFrame>* keyframes) ;

    /**
     * @brief If keyframes at the given times exist in the curve at the given view and dimension then they will be warped by the given
     * warp, assuming the X coordinate is the time and the Y coordinate is the value. Z always equals 1.
     * @param allowKeysOverlap If false, then if a warped keyframe is on the same time as another existing keyframe, then this function will 
     * return false, otherwise it will just substitute the existing keyframe
     * @param newKey[out] If non null, the new keyframes in return will be assigned to this parameter.
     * @returns True If the keyframes could be warped, false otherwise
     * Note that if this function succeeds, it is guaranteed that all keyframes have been warped.
     **/
    virtual bool warpValuesAtTime(CurveChangeReason curveChangeReason, const std::list<double>& times, ViewSpec view,  int dimension, const Curve::KeyFrameWarp& warp, bool allowKeysOverlap, std::vector<KeyFrame>* keyframes = 0) = 0;

    /**
     * @brief Removes all keyframes on the object for the given view in the given dimension.
     * @param reason Identifies who called the function to optimize redraws of the Gui
     * The default implementation just calls removeAnimationAcrossDimensions.
     **/
    virtual void removeAnimation(ViewSpec view, int dimension, CurveChangeReason reason);

    /**
     * @brief Removes all keyframes on the object for the given view in the given dimensions.
     * @param dimensions A vector of dimensions index which must be 0 <= dimension <
     * @param reason Identifies who called the function to optimize redraws of the Gui
     **/
    virtual void removeAnimationAcrossDimensions(ViewSpec view, const std::vector<int>& dimensions, CurveChangeReason reason) = 0;

    /**
     * @brief Removes animation on the curve at the given view and dimension before the given time. 
     * If a keyframe at the given time exists it is not removed.
     **/
    virtual void deleteAnimationBeforeTime(double time, ViewSpec view, int dimension, CurveChangeReason reason) = 0;

    /**
     * @brief Removes animation on the curve at the given view and dimension after the given time.
     * If a keyframe at the given time exists it is not removed.
     **/
    virtual void deleteAnimationAfterTime(double time, ViewSpec view, int dimension, CurveChangeReason reason) = 0;

    /**
     * @brief Set the interpolation type for the given keyframe on the curve at the given dimension and view
     * @param newKey[out] If non null, the new keyframe in return will be assigned to this parameter
     * The default implementation just calls setInterpolationAtTimes.
     **/
    virtual void setInterpolationAtTime(CurveChangeReason reason, ViewSpec view, int dimension, double time, KeyframeTypeEnum interpolation, KeyFrame* newKey = 0);
    
    /**
     * @brief Set the interpolation type for the given keyframes on the curve at the given dimension and view
     * @param newKey[out] If non null, the new keyframes in return will be assigned to this parameter
     **/
    virtual void setInterpolationAtTimes(CurveChangeReason reason, ViewSpec view, int dimension, const std::list<double>& times, KeyframeTypeEnum interpolation, std::vector<KeyFrame>* newKeys = 0) = 0;

    /**
     * @brief Set the left and right derivatives of the control point at the given time on the curve at the given view and dimension
     * @param left The new value to set for the left derivative of the keyframe at the given time
     * @param right The new value to set for the right derivative of the keyframe at the given time
     **/
    virtual bool setLeftAndRightDerivativesAtTime(CurveChangeReason reason, ViewSpec view, int dimension, double time, double left, double right) = 0;

    /**
     * @brief Set the left or right derivative of the control point at the given time on the curve at the given view and dimension
     * @param derivative The new value to set for the derivative of the keyframe at the given time
     * @param isLeft If true, the left derivative will be set, otherwise the right derivative will be set
     **/
    virtual bool setDerivativeAtTime(CurveChangeReason reason, ViewSpec view, int dimension, double time, double derivative, bool isLeft) = 0;
};

NATRON_NAMESPACE_EXIT;

#endif // ANIMATINGOBJECTI_H
