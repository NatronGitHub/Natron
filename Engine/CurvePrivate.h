/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2023 The Natron developers
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

#ifndef NATRON_ENGINE_CURVEPRIVATE_H
#define NATRON_ENGINE_CURVEPRIVATE_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <QtCore/QMutex>
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
#include <QtCore/QRecursiveMutex>
#endif

#include "Engine/Variant.h"
#include "Engine/Knob.h"
#include "Engine/KnobTypes.h"
#include "Engine/KnobFile.h"
#include "Engine/EngineFwd.h"

//#define NATRON_CURVE_USE_CACHE

NATRON_NAMESPACE_ENTER

struct CurvePrivate
{
    enum CurveTypeEnum
    {
        eCurveTypeDouble = 0, //< the values held by the keyframes can be any real
        eCurveTypeInt, //< the values held by the keyframes can only be integers
        eCurveTypeIntConstantInterp, //< same as eCurveTypeInt but interpolation is restricted to eKeyframeTypeConstant
        eCurveTypeBool, //< the values held by the keyframes can be either 0 or 1
        eCurveTypeString //< the values held by the keyframes can only be integers and keyframes are ordered by increasing values
        // and times
    };

    KeyFrameSet keyFrames;

#ifdef NATRON_CURVE_USE_CACHE
    std::map<double, double> resultCache; //< a cache for interpolations
#endif

    KnobI* owner;
    int dimensionInOwner;
    CurveTypeEnum type;
    double xMin, xMax;
    double yMin, yMax;
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    mutable QRecursiveMutex _lock;
#else
    mutable QMutex _lock; //< the plug-ins can call getValueAt at any moment and we must make sure the user is not playing around
#endif
    bool isParametric;
    bool isPeriodic;

    CurvePrivate()
        : keyFrames()
#ifdef NATRON_CURVE_USE_CACHE
        , resultCache()
#endif
        , owner(NULL)
        , dimensionInOwner(-1)
        , type(eCurveTypeDouble)
        , xMin(-std::numeric_limits<double>::infinity())
        , xMax(std::numeric_limits<double>::infinity())
        , yMin(-std::numeric_limits<double>::infinity())
        , yMax(std::numeric_limits<double>::infinity())
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
        , _lock()
#else
        , _lock(QMutex::Recursive)
#endif
        , isParametric(false)
        , isPeriodic(false)
    {
    }

    CurvePrivate(const CurvePrivate & other)
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
        : _lock()
#else
        : _lock(QMutex::Recursive)
#endif
    {
        *this = other;
    }

    void operator=(const CurvePrivate & other)
    {
        keyFrames = other.keyFrames;
        owner = other.owner;
        dimensionInOwner = other.dimensionInOwner;
        isParametric = other.isParametric;
        type = other.type;
        xMin = other.xMin;
        xMax = other.xMax;
        yMin = other.yMin;
        yMax = other.yMax;
        isPeriodic = other.isPeriodic;
    }

    
};

NATRON_NAMESPACE_EXIT

#endif // NATRON_ENGINE_CURVEPRIVATE_H
