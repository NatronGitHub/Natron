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

#ifndef NATRON_ENGINE_CURVE_H
#define NATRON_ENGINE_CURVE_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <vector>
#include <map>
#include <set>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#endif

#include "Global/GlobalDefines.h"
#include "Engine/EngineFwd.h"
#include "Engine/DimensionIdx.h"
#include "Serialization/SerializationBase.h"
#include "Engine/Transform.h"


#define NATRON_CURVE_X_SPACING_EPSILON 1e-6

NATRON_NAMESPACE_ENTER;

/**
 * @brief A KeyFrame is a lightweight pair <time,value>. These are the values that are used
 * to interpolate a Curve. The _leftDerivative and _rightDerivative can be
 * used by the interpolation method of the curve.
 **/

class KeyFrame
{
public:

    KeyFrame();

    KeyFrame(double time,
             double initialValue,
             double leftDerivative = 0.,
             double rightDerivative = 0.,
             KeyframeTypeEnum interpolation = eKeyframeTypeSmooth);

    KeyFrame(const KeyFrame & other);

    ~KeyFrame();

    void operator=(const KeyFrame & o);

    bool operator==(const KeyFrame & o) const
    {
        return o._time == _time &&
               o._value == _value &&
               o._interpolation == _interpolation &&
               o._leftDerivative == _leftDerivative &&
               o._rightDerivative == _rightDerivative;
    }

    bool operator!=(const KeyFrame & o) const
    {
        return !(*this == o);
    }

    double getValue() const;

    double getTime() const;

    double getLeftDerivative() const;

    double getRightDerivative() const;

    void setLeftDerivative(double v);

    void setRightDerivative(double v);

    void setValue(double v);

    void setTime(double time);

    void setInterpolation(KeyframeTypeEnum interp);

    KeyframeTypeEnum getInterpolation() const;

private:

    double _time;
    double _value;
    double _leftDerivative;
    double _rightDerivative;
    KeyframeTypeEnum _interpolation;

    friend class ::boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
};

struct KeyFrame_compare_time
{
    bool operator() (const KeyFrame & lhs,
                     const KeyFrame & rhs) const
    {
        return lhs.getTime() < rhs.getTime();
    }
};



typedef std::set<KeyFrame, KeyFrame_compare_time> KeyFrameSet;


struct CurvePrivate;

class Curve : public SERIALIZATION_NAMESPACE::SerializableObjectBase
{
    enum CurveChangedReasonEnum
    {
        eCurveChangedReasonDerivativesChanged = 0,
        eCurveChangedReasonKeyframeChanged = 1
    };

public:
    struct YRange {
        YRange(double min_, double max_)
            : min(min_)
            , max(max_)
        {
        }

        double min;
        double max;
    };

    /**
     * @brief An empty curve, held by no one. This constructor is used by the serialization.
     **/
    Curve();

    /**
     * @brief An empty curve, held by owner. This is the "normal" constructor.
     **/
    Curve(const KnobIPtr& owner,
          DimIdx dimensionInOwner,
          ViewIdx viewInOwner);

    Curve(const Curve & other);

    /**
     * @brief Set the curve to be periodic, i.e: the first keyframe is considered to be equal to the last keyframe.
     * Note that this will clear all existing keyframes when called.
     **/
    void setPeriodic(bool periodic);

    bool isCurvePeriodic() const;

    ~Curve();

    virtual void fromSerialization(const SERIALIZATION_NAMESPACE::SerializationObjectBase& serialization) OVERRIDE FINAL;

    virtual void toSerialization(SERIALIZATION_NAMESPACE::SerializationObjectBase* serialization) OVERRIDE FINAL;

    void operator=(const Curve & other);

    bool operator==(const Curve & other) const;

    bool operator!=(const Curve & other) const
    {
        return !(*this == other);
    }

    /**
     * @brief Copies all the keyframes held by other, but does not change the pointer to the owner.
     **/
    void clone(const Curve & other);

    bool cloneAndCheckIfChanged(const Curve& other, double offset, const RangeD* range);

    /**
     * @brief Same as the other version clone except that keyframes will be offset by the given offset
     * and only the keyframes lying in the given range will be copied.
     **/
    void clone(const Curve & other, double offset, const RangeD* range);

    bool isAnimated() const WARN_UNUSED_RETURN;

    /**
     * @brief Returns true if the Curve represents a string animation, in which case the user cannot
     * modify the Y component of the curve.
     **/
    bool isYComponentMovable() const WARN_UNUSED_RETURN;

    /**whether the curve will clamp possible keyframe X values to integers or not.**/
    bool areKeyFramesTimeClampedToIntegers() const WARN_UNUSED_RETURN;

    bool areKeyFramesValuesClampedToIntegers() const WARN_UNUSED_RETURN;

    bool areKeyFramesValuesClampedToBooleans() const WARN_UNUSED_RETURN;

    void setXRange(double a, double b);

    std::pair<double, double> getXRange() const WARN_UNUSED_RETURN;

    ///returns true if a keyframe was successfully added, false if it just replaced an already
    ///existing key at this time.
    bool addKeyFrame(KeyFrame key);

    // Returns true if a keyframe was added, false if it modified an existing one
    ValueChangedReturnCodeEnum setOrAddKeyframe(KeyFrame key);

    void removeKeyFrameWithTime(double time);

    void removeKeyFrameWithIndex(int index);

    void removeKeyFramesBeforeTime(double time, std::list<double>* keyframeRemoved);

    void removeKeyFramesAfterTime(double time, std::list<double>* keyframeRemoved);

    bool getNearestKeyFrameWithTime(double time, KeyFrame* k) const WARN_UNUSED_RETURN;

    /**
     * @brief Returns the previous keyframe to the given time, which doesn't have to be a keyframe time
     **/
    bool getPreviousKeyframeTime(double time, KeyFrame* k) const WARN_UNUSED_RETURN;

    /**
     * @brief Returns the next keyframe to the given time, which doesn't have to be a keyframe time
     **/
    bool getNextKeyframeTime(double time, KeyFrame* k) const WARN_UNUSED_RETURN;

    bool getKeyFrameWithTime(double time, KeyFrame* k) const WARN_UNUSED_RETURN;

    /*
     * @brief Returns the number of keyframes in the range [first,last[
     */
    int getNKeyFramesInRange(double first, double last) const WARN_UNUSED_RETURN;

    bool getKeyFrameWithIndex(int index, KeyFrame* k) const WARN_UNUSED_RETURN;

    int getKeyFramesCount() const WARN_UNUSED_RETURN;

    double getMinimumTimeCovered() const WARN_UNUSED_RETURN;

    double getMaximumTimeCovered() const WARN_UNUSED_RETURN;

    double getValueAt(double t, bool clamp = true) const WARN_UNUSED_RETURN;

    double getDerivativeAt(double t) const WARN_UNUSED_RETURN;

    double getIntegrateFromTo(double t1, double t2) const WARN_UNUSED_RETURN;

    KeyFrameSet getKeyFrames_mt_safe() const WARN_UNUSED_RETURN;

    void clearKeyFrames();

    /**
     * @brief Set the new value and time of the keyframe positioned at index index and returns the new  keyframe.
     * Also the index of the new keyframe is returned in newIndex.
     **/
    KeyFrame setKeyFrameValueAndTime(double time, double value, int index, int* newIndex = NULL);

    class KeyFrameWarp
    {
    protected:
        bool _inverted;
    public:


        KeyFrameWarp() : _inverted(false) {}

        virtual ~KeyFrameWarp() {}

        void setWarpInverted(bool inverted)
        {
            _inverted = inverted;
        }

        virtual bool allowReplacingExistingKeyframes() const = 0;

        virtual KeyFrame applyForwardWarp(const KeyFrame& key) const = 0;

        virtual KeyFrame applyBackwardWarp(const KeyFrame& key) const = 0;

        virtual bool isIdentity() const = 0;

        virtual bool mergeWith(const KeyFrameWarp& other) = 0;

    };

    class TranslationKeyFrameWarp : public KeyFrameWarp
    {
        double _dt, _dv;

    public:

        TranslationKeyFrameWarp(double dt, double dv)
        : KeyFrameWarp()
        , _dt(dt)
        , _dv(dv)
        {

        }

        double getDT() const
        {
            return _dt;
        }

        double getDV() const
        {
            return _dv;
        }

        virtual ~TranslationKeyFrameWarp()
        {

        }

        virtual bool allowReplacingExistingKeyframes() const OVERRIDE FINAL
        {
            return false;
        }

        virtual KeyFrame applyForwardWarp(const KeyFrame& key) const OVERRIDE FINAL
        {
            if (_inverted) {
                return KeyFrame(key.getTime() - _dt, key.getTime() - _dv);
            } else {
                return KeyFrame(key.getTime() + _dt, key.getTime() + _dv);
            }
        }

        virtual KeyFrame applyBackwardWarp(const KeyFrame& key) const OVERRIDE FINAL
        {
            if (_inverted) {
                return KeyFrame(key.getTime() + _dt, key.getTime() + _dv);
            } else {
                return KeyFrame(key.getTime() - _dt, key.getTime() - _dv);
            }
        }

        virtual bool isIdentity() const OVERRIDE FINAL
        {
            return _dt == 0 && _dv == 0;
        }

        virtual bool mergeWith(const KeyFrameWarp& other) OVERRIDE FINAL
        {
            const TranslationKeyFrameWarp* otherWarp = dynamic_cast<const TranslationKeyFrameWarp*>(&other);
            if (otherWarp) {
                _dt += otherWarp->_dt;
                _dv += otherWarp->_dv;
                return true;
            }
            return false;
        }

    };


    class AffineKeyFrameWarp : public KeyFrameWarp
    {
        Transform::Matrix3x3 _transform, _inverseTransform;

    public:

        AffineKeyFrameWarp(const Transform::Matrix3x3& transform)
        : KeyFrameWarp()
        , _transform(transform)
        , _inverseTransform(Transform::matInverse(transform))
        {

        }

        virtual ~AffineKeyFrameWarp()
        {

        }

        virtual bool allowReplacingExistingKeyframes() const OVERRIDE FINAL
        {
            return true;
        }

        virtual KeyFrame applyForwardWarp(const KeyFrame& key) const OVERRIDE FINAL
        {
            if (_inverted) {
                return applyWarp(_inverseTransform, key);
            } else {
                return applyWarp(_transform, key);
            }
        }

        virtual KeyFrame applyBackwardWarp(const KeyFrame& key) const OVERRIDE FINAL
        {
            if (_inverted) {
                return applyWarp(_transform, key);
            } else {
                return applyWarp(_inverseTransform, key);
            }
        }

        virtual bool isIdentity() const OVERRIDE FINAL
        {
            return _transform.isIdentity();
        }

        virtual bool mergeWith(const KeyFrameWarp& other) OVERRIDE FINAL
        {
            const AffineKeyFrameWarp* otherWarp = dynamic_cast<const AffineKeyFrameWarp*>(&other);
            if (otherWarp) {
                _transform = Transform::matMul(_transform, otherWarp->_transform);
                _inverseTransform = Transform::matMul(_inverseTransform, otherWarp->_inverseTransform);
                return true;
            }
            return false;
        }


    private:

        KeyFrame applyWarp(const Transform::Matrix3x3& mat, const KeyFrame& key) const
        {
            Transform::Point3D p;
            p.x = key.getTime();
            p.y = key.getValue();
            p.z = 1;
            p = Transform::matApply(mat, p);
            return KeyFrame(p.x, p.y);

        }

    };


    /**
     * @brief Moves multiple keyframes in the set. 
     * @param times The keyframes time to move. It is expected that the times are sorted by increasing order otherwise this function will fail.
     * @param warp The warp to apply to each keyframe
     * @param allowReplacingExistingKeys When false, if a warped keyframe match an already existing keyframe, the function will fail, otherwise it will just continue blindly
     * @param newKeys If non NULL, the new keyframes corresponding to the input times will be returned
     * @param keysAdded A list of the keyframes that were created
     * @param keysRemoved A list of the keyframes that were removed
     * Note: it is guaranteed that keyframes in the added list are not to be found in the removed list and vice versa.
     **/
    bool transformKeyframesValueAndTime(const std::list<double>& times,
                                        const KeyFrameWarp& warp,
                                        bool allowReplacingExistingKeys,
                                        std::vector<KeyFrame>* newKeys = NULL,
                                        std::list<double>* keysAdded = 0,
                                        std::list<double>* keysRemoved = 0);

private:

    KeyFrame setKeyFrameValueAndTimeInternal(double time, double value, int index, int* newIndex);

public:

    /**
     * @brief Set the left derivative  of the keyframe positioned at index index and returns the new  keyframe.
     * Also the index of the new keyframe is returned in newIndex.
     **/
    KeyFrame setKeyFrameLeftDerivative(double value, int index, int* newIndex = NULL);

private:

    KeyFrame setKeyFrameLeftDerivativeInternal(double value, int index, int* newIndex);

public:

    /**
     * @brief Set the right derivative  of the keyframe positioned at index index and returns the new keyframe.
     * Also the index of the new keyframe is returned in newIndex.
     **/
    KeyFrame setKeyFrameRightDerivative(double value, int index, int* newIndex = NULL);

private:

    KeyFrame setKeyFrameRightDerivativeInternal(double value, int index, int* nextIndex);

public:


    /**
     * @brief Set the right and left derivatives  of the keyframe positioned at index index and returns the new  keyframe.
     * Also the index of the new keyframe is returned in newIndex.
     **/
    KeyFrame setKeyFrameDerivatives(double left, double right, int index, int* newIndex = NULL);

private:

    KeyFrame setKeyFrameDerivativesInternal(double left, double right, int index, int* newIndex);

public:

    /**
     * @brief  Set the interpolation method of the keyframe positioned at index index and returns the new  keyframe.
     * Also the index of the new keyframe is returned in newIndex.
     **/
    KeyFrame setKeyFrameInterpolation(KeyframeTypeEnum interp, int index, int* newIndex = NULL);
    bool setKeyFrameInterpolation(KeyframeTypeEnum interp, double time, KeyFrame* ret = 0);

private:

    KeyFrame setKeyFrameInterpolationInternal(KeyframeTypeEnum interp, int index, int* newIndex);

public:

    void setCurveInterpolation(KeyframeTypeEnum interp);

    YRange getCurveYRange() const WARN_UNUSED_RETURN;

    YRange getCurveDisplayYRange() const WARN_UNUSED_RETURN;

    int keyFrameIndex(double time) const WARN_UNUSED_RETURN;

    /// set the curve Y range (used for testing, when the Curve his not owned by a Knob)
    void setYRange(double yMin, double yMax);

    /**
     * @brief Find a keyframe by time in the given keys set.
     * @param fromIt A hint where to start the search. If fromIt == keys.end() then the search will cycle the whole keys set,
     * otherwise it will start from the provided iterator.
     **/
    static KeyFrameSet::const_iterator findWithTime(const KeyFrameSet& keys, KeyFrameSet::const_iterator fromIt, double time);

    /**
     * @brief Smooth the curve.
     * @param range, if non null the curve will only be smoothed over the given range (intersected to the keyframes range)
     **/
    void smooth(const RangeD* range);

    void setKeyframes(const KeyFrameSet& keys, bool refreshDerivatives);

private:

    friend class ::boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);

    ///////The following functions are not thread-safe
    KeyFrameSet::const_iterator find(double time, KeyFrameSet::const_iterator fromIt) const WARN_UNUSED_RETURN;
    KeyFrameSet::const_iterator atIndex(int index) const WARN_UNUSED_RETURN;
    KeyFrameSet::const_iterator begin() const WARN_UNUSED_RETURN;
    KeyFrameSet::const_iterator end() const WARN_UNUSED_RETURN;
    YRange getCurveYRange_internal() const WARN_UNUSED_RETURN;

    void removeKeyFrame(KeyFrameSet::const_iterator it);

    double clampValueToCurveYRange(double v) const WARN_UNUSED_RETURN;

    void setKeyframesInternal(const KeyFrameSet& keys, bool refreshDerivatives);

    ///returns an iterator to the new keyframe in the keyframe set and
    ///a boolean indicating whether it removed a keyframe already existing at this time or not
    std::pair<KeyFrameSet::iterator, bool> addKeyFrameNoUpdate(const KeyFrame & cp) WARN_UNUSED_RETURN;


    /**
     * @brief Called when a keyframe/derivative is modified, indicating that the curve has changed and we must
     * evaluate any change (i.e: force a new render)
     * WARNING: The iterator "key" is invalid after this call.
     * The value pointed to by key before this call is now pointed to by the iterator returned by this function.
     **/
    KeyFrameSet::iterator evaluateCurveChanged(CurveChangedReasonEnum reason, KeyFrameSet::iterator key) WARN_UNUSED_RETURN;
    KeyFrameSet::iterator refreshDerivatives(CurveChangedReasonEnum reason, KeyFrameSet::iterator key);
    KeyFrameSet::iterator setKeyFrameValueAndTimeNoUpdate(double value, double time, KeyFrameSet::iterator k) WARN_UNUSED_RETURN;

    bool mustClamp() const;

    KeyFrameSet::iterator setKeyframeInterpolation_internal(KeyFrameSet::iterator it, KeyframeTypeEnum type);

    /**
     * @brief Called when the curve has changed to invalidate any cache relying on the curve values.
     **/
    void onCurveChanged();

private:
    boost::scoped_ptr<CurvePrivate> _imp;
};

NATRON_NAMESPACE_EXIT;

#endif // NATRON_ENGINE_CURVE_H
