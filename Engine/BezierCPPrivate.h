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

#ifndef Engine_BezierCPPrivate_h
#define Engine_BezierCPPrivate_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/make_shared.hpp>
#endif

#include <QtCore/QMutex>
#include <QtCore/QReadWriteLock>

#include "Engine/Curve.h"
#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

struct BezierCPPrivate
{
    BezierWPtr holder;

    ///the animation curves for the position in the 2D plane
    CurvePtr curveX, curveY;
    CurvePtr guiCurveX, guiCurveY;
    double x, y; //< used when there is no keyframe
    double guiX, guiY;
    bool broken; //< true if both tangents are independent

    ///the animation curves for the derivatives
    ///They do not need to be protected as Curve is a thread-safe class.
    CurvePtr curveLeftBezierX, curveRightBezierX, curveLeftBezierY, curveRightBezierY;
    CurvePtr guiCurveLeftBezierX, guiCurveRightBezierX, guiCurveLeftBezierY, guiCurveRightBezierY;
    mutable QMutex staticPositionMutex; //< protects the  leftX,rightX,leftY,rightY
    double leftX, rightX, leftY, rightY; //< used when there is no keyframe
    double guiLeftX, guiRightX, guiLeftY, guiRightY; //< used when there is no keyframe

    BezierCPPrivate(const BezierPtr& curve)
        : holder(curve)
        , curveX()
        , curveY()
        , guiCurveX()
        , guiCurveY()
        , x(0)
        , y(0)
        , guiX(0)
        , guiY(0)
        , broken(false)
        , curveLeftBezierX()
        , curveRightBezierX()
        , curveLeftBezierY()
        , curveRightBezierY()
        , guiCurveLeftBezierX()
        , guiCurveRightBezierX()
        , guiCurveLeftBezierY()
        , guiCurveRightBezierY()
        , staticPositionMutex()
        , leftX(0)
        , rightX(0)
        , leftY(0)
        , rightY(0)
        , guiLeftX(0)
        , guiRightX(0)
        , guiLeftY(0)
        , guiRightY(0)
    {
        curveX= boost::make_shared<Curve>();
        curveY= boost::make_shared<Curve>();
        guiCurveX= boost::make_shared<Curve>();
        guiCurveY= boost::make_shared<Curve>();
        curveLeftBezierX= boost::make_shared<Curve>();
        curveRightBezierX= boost::make_shared<Curve>();
        curveLeftBezierY= boost::make_shared<Curve>();
        curveRightBezierY= boost::make_shared<Curve>();
        guiCurveLeftBezierX= boost::make_shared<Curve>();
        guiCurveRightBezierX= boost::make_shared<Curve>();
        guiCurveLeftBezierY= boost::make_shared<Curve>();
        guiCurveRightBezierY= boost::make_shared<Curve>();
    }
};

NATRON_NAMESPACE_EXIT

#endif // Engine_BezierCPPrivate_h_
