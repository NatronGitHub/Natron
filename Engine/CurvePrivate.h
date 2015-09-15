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

#ifndef NATRON_ENGINE_CURVEPRIVATE_H_
#define NATRON_ENGINE_CURVEPRIVATE_H_

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#endif
#include <QMutex>

#include "Engine/Variant.h"
#include "Engine/Knob.h"
#include "Engine/KnobTypes.h"
#include "Engine/KnobFile.h"

class Curve;
class KeyFrame;
class KnobI;

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
    std::map<double,double> resultCache; //< a cache for interpolations
    KnobI* owner;
    int dimensionInOwner;
    bool isParametric;
    CurveTypeEnum type;
    double xMin, xMax;
    double yMin, yMax;
    bool hasYRange;
    mutable QMutex _lock; //< the plug-ins can call getValueAt at any moment and we must make sure the user is not playing around


    CurvePrivate()
    : keyFrames()
    , resultCache()
    , owner(NULL)
    , dimensionInOwner(-1)
    , isParametric(false)
    , type(eCurveTypeDouble)
    , xMin(INT_MIN)
    , xMax(INT_MAX)
    , yMin(INT_MIN)
    , yMax(INT_MAX)
    , hasYRange(false)
    , _lock(QMutex::Recursive)
    {
    }

    CurvePrivate(const CurvePrivate & other)
        : _lock(QMutex::Recursive)
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
        hasYRange = other.hasYRange;
    }
    
};


#endif // NATRON_ENGINE_CURVEPRIVATE_H_
