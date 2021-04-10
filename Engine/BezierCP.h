/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2021 The Natron developers
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
#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

/**
 * @class A Bezier is an animated control point of a Bezier. It is the starting point
 * and/or the ending point of a bezier segment. (It would correspond to P0/P3).
 * The left bezier point/right bezier point we refer to in the functions below
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
{
    ///This is the unique class allowed to call the setters.
    friend class Bezier;

public:

    ///Constructore used by the serialization
    BezierCP();

    BezierCP(const BezierCP & other);

    BezierCP(const BezierPtr& curve);

    virtual ~BezierCP();

    CurvePtr getXCurve() const;
    CurvePtr getYCurve() const;
    CurvePtr getLeftXCurve() const;
    CurvePtr getLeftYCurve() const;
    CurvePtr getRightXCurve() const;
    CurvePtr getRightYCurve() const;

    void clone(const BezierCP & other);

    void setPositionAtTime(bool useGuiCurves, double time, double x, double y);

    void setLeftBezierPointAtTime(bool useGuiCurves, double time, double x, double y);

    void setRightBezierPointAtTime(bool useGuiCurves, double time, double x, double y);

    void setBroken(bool broken);

    void setStaticPosition(bool useGuiCurves, double x, double y);

    void setLeftBezierStaticPosition(bool useGuiCurves, double x, double y);

    void setRightBezierStaticPosition(bool useGuiCurves, double x, double y);

    void removeKeyframe(bool useGuiCurves, double time);

    void removeAnimation(bool useGuiCurves, double currentTime);

    ///returns true if a keyframe was set
    bool cuspPoint(bool useGuiCurves, double time, ViewIdx view, bool autoKeying, bool rippleEdit, const std::pair<double, double>& pixelScale);

    ///returns true if a keyframe was set
    bool smoothPoint(bool useGuiCurves, double time, ViewIdx view, bool autoKeying, bool rippleEdit, const std::pair<double, double>& pixelScale);


    virtual bool isFeatherPoint() const
    {
        return false;
    }

    bool equalsAtTime(bool useGuiCurves, double time, ViewIdx view, const BezierCP & other) const;

    bool getPositionAtTime(bool useGuiCurves, double time, ViewIdx view, double* x, double* y) const;

    bool getLeftBezierPointAtTime(bool useGuiCurves, double time, ViewIdx view, double* x, double* y, bool reAlign = true) const;

    bool getRightBezierPointAtTime(bool useGuiCurves, double time, ViewIdx view, double *x, double *y, bool reAlign = true) const;

    bool getBroken() const;
    
    bool hasKeyFrameAtTime(bool useGuiCurves, double time) const;

    void getKeyframeTimes(bool useGuiCurves, std::set<double>* times) const;

    void getKeyFrames(bool useGuiCurves, std::list<std::pair<double, KeyframeTypeEnum> >* keys) const;

    int getKeyFrameIndex(bool useGuiCurves, double time) const;

    void setKeyFrameInterpolation(bool useGuiCurves, KeyframeTypeEnum interp, int index);

    double getKeyframeTime(bool useGuiCurves, int index) const;

    int getKeyframesCount(bool useGuiCurves) const;

    int getControlPointsCount() const;

    /**
     * @brief Pointer to the bezier holding this control point. This is not protected by a mutex
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
    int isNearbyTangent(bool useGuiCurves, double time, ViewIdx view, double x, double y, double acceptance) const;

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

typedef std::list<BezierCPPtr> BezierCPs;

NATRON_NAMESPACE_EXIT

#endif // Engine_BezierCP_h
