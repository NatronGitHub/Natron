/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
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

#ifndef Engine_BezierCP_h
#define Engine_BezierCP_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <list>
#include <set>
#include <utility>

#include "Global/Macros.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#endif

#include "Global/GlobalDefines.h"

#include "Engine/ViewIdx.h"
#include "Serialization/SerializationBase.h"
#include "Engine/TimeValue.h"

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

/**
 * @class A Bezier is an animated control point of a Bezier. It is the starting point
 * and/or the ending point of a Bezier segment. (It would correspond to P0/P3).
 * The left Bezier point/right Bezier point we refer to in the functions below
 * are respectively the P2 and P1 point.
 *
 * Note on multi-thread:
 * All getters or const functions can be called in any thread, that is:
 * - The GUI thread (main-thread)
 * - The render thread
 * - The serialization thread (when saving)
 *
 * Setters or non-const functions can exclusively be called in the main-thread (Gui thread) to ensure there is no
 * race condition whatsoever.

 * More-over the setters must be called ONLY by the Bezier class which is the class handling the thread safety.
 * That's why non-const functions are private.
 **/
struct BezierCPPrivate;
class BezierCP
: public SERIALIZATION_NAMESPACE::SerializableObjectBase
{
    ///This is the unique class allowed to call the setters.
    friend class Bezier;

public:

    ///Constructore used by the serialization
    BezierCP();

    BezierCP(const BezierCP & other);

    BezierCP(const BezierPtr& curve);

    virtual ~BezierCP();

    /**
     * @brief Must be implemented by the derived class to save the state into
     * the serialization object.
     * Derived implementations must call the parent class implementation.
     **/
    virtual void toSerialization(SERIALIZATION_NAMESPACE::SerializationObjectBase* obj)  OVERRIDE;

    /**
     * @brief Must be implemented by the derived class to load the state from
     * the serialization object.
     * Derived implementations must call the parent class implementation.
     **/
    virtual void fromSerialization(const SERIALIZATION_NAMESPACE::SerializationObjectBase & obj) OVERRIDE;
    


    CurvePtr getXCurve() const;
    CurvePtr getYCurve() const;
    CurvePtr getLeftXCurve() const;
    CurvePtr getLeftYCurve() const;
    CurvePtr getRightXCurve() const;
    CurvePtr getRightYCurve() const;

    void copyControlPoint(const BezierCP & other, const RangeD* range = 0);

    void setPositionAtTime(TimeValue time, double x, double y);

    void setLeftBezierPointAtTime(TimeValue time, double x, double y);

    void setRightBezierPointAtTime(TimeValue time, double x, double y);

    void setStaticPosition(double x, double y);

    void setLeftBezierStaticPosition(double x, double y);

    void setRightBezierStaticPosition(double x, double y);

    virtual bool isFeatherPoint() const
    {
        return false;
    }

    bool equalsAtTime(TimeValue time, const BezierCP & other) const;

    bool operator==(const BezierCP& other) const;

    bool operator!=(const BezierCP& other) const
    {
        return !(*this == other);
    }

    bool getPositionAtTime(TimeValue time, double* x, double* y) const;

    bool getLeftBezierPointAtTime(TimeValue time, double* x, double* y) const;

    bool getRightBezierPointAtTime(TimeValue time, double *x, double *y) const;

    void removeKeyframe(TimeValue time);

    void setKeyFrameInterpolation(KeyframeTypeEnum interp, int index);

    int getControlPointsCount(ViewIdx view) const;

    void getKeyframeTimes(std::set<double>* keys) const;

    /**
     * @brief Pointer to the Bezier holding this control point. This is not protected by a mutex
     * since it never changes.
     **/
    BezierPtr getBezier() const;

    /**
     * @brief Returns whether a tangent handle is nearby the given coordinates.
     * That is, this function a number indicating
     * if the given coordinates are close to the left control point (P2, ret = 0) or the right control point(P1, ret = 1).
     * If it is not close to any tangent, -1 is returned.
     * This function can also return the tangent of a feather point, to find out if the point is a feather point call
     * isFeatherPoint() on the returned control point.
     **/
    int isNearbyTangent(TimeValue time, ViewIdx view, double x, double y, double acceptance) const;

    SequenceTime getOffsetTime() const;

    void cloneInternalCurvesToGuiCurves();

    void cloneGuiCurvesToInternalCurves();

    template<class Archive>
    void save(Archive & ar, const unsigned int version) const;

    template<class Archive>
    void load(Archive & ar, const unsigned int version);

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);

private:

    boost::scoped_ptr<BezierCPPrivate> _imp;
};



NATRON_NAMESPACE_EXIT

#endif // Engine_BezierCP_h
