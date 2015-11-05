/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
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

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#endif

#include <QtCore/QMutex>
#include <QtCore/QReadWriteLock>

#include "Engine/Curve.h"
#include "Engine/EngineFwd.h"


struct BezierCPPrivate
{
    boost::weak_ptr<Bezier> holder;

    ///the animation curves for the position in the 2D plane
    boost::shared_ptr<Curve> curveX,curveY;
    boost::shared_ptr<Curve> guiCurveX,guiCurveY;
    double x,y; //< used when there is no keyframe
    double guiX,guiY;
    
    ///the animation curves for the derivatives
    ///They do not need to be protected as Curve is a thread-safe class.
    boost::shared_ptr<Curve> curveLeftBezierX,curveRightBezierX,curveLeftBezierY,curveRightBezierY;
    boost::shared_ptr<Curve> guiCurveLeftBezierX,guiCurveRightBezierX,guiCurveLeftBezierY,guiCurveRightBezierY;
    
    mutable QMutex staticPositionMutex; //< protects the  leftX,rightX,leftY,rightY
    double leftX,rightX,leftY,rightY; //< used when there is no keyframe
    double guiLeftX,guiRightX,guiLeftY,guiRightY; //< used when there is no keyframe
    mutable QReadWriteLock masterMutex; //< protects masterTrack & relativePoint
    boost::shared_ptr<KnobDouble> masterTrack; //< is this point linked to a track ?
    SequenceTime offsetTime; //< the time at which the offset must be computed

    BezierCPPrivate(const boost::shared_ptr<Bezier>& curve)
    : holder(curve)
    , curveX(new Curve)
    , curveY(new Curve)
    , guiCurveX(new Curve)
    , guiCurveY(new Curve)
    , x(0)
    , y(0)
    , guiX(0)
    , guiY(0)
    , curveLeftBezierX(new Curve)
    , curveRightBezierX(new Curve)
    , curveLeftBezierY(new Curve)
    , curveRightBezierY(new Curve)
    , guiCurveLeftBezierX(new Curve)
    , guiCurveRightBezierX(new Curve)
    , guiCurveLeftBezierY(new Curve)
    , guiCurveRightBezierY(new Curve)
    , staticPositionMutex()
    , leftX(0)
    , rightX(0)
    , leftY(0)
    , rightY(0)
    , guiLeftX(0)
    , guiRightX(0)
    , guiLeftY(0)
    , guiRightY(0)
    , masterMutex()
    , masterTrack()
    , offsetTime(0)
    {
    }
};

#endif // Engine_BezierCPPrivate_h_