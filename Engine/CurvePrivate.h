/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
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

#ifndef Engine_CurvePrivate_h
#define Engine_CurvePrivate_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#endif

#include <QtCore/QMutex>

#include "Engine/Variant.h"
#include "Engine/Curve.h"
#include "Engine/Knob.h"
#include "Engine/KnobTypes.h"
#include "Engine/KnobFile.h"
#include "Engine/KeyFrameInterpolator.h"


#include "Engine/EngineFwd.h"


NATRON_NAMESPACE_ENTER




struct CurvePrivate
{
    KeyFrameSet keyFrames;


    KeyFrameInterpolatorPtr interpolator;
    std::list<CurveChangesListenerWPtr> listeners;
    CurveTypeEnum type;
    double xMin, xMax;
    double yMin, yMax;
    double displayMin, displayMax;
    mutable QMutex _lock; //< the plug-ins can call getValueAt at any moment and we must make sure the user is not playing around
    bool isPeriodic;
    bool clampKeyFramesTimeToIntegers;

    CurvePrivate()
    : keyFrames()
    , interpolator(new KeyFrameInterpolator)
    , listeners()
    , type(eCurveTypeDouble)
    , xMin(-std::numeric_limits<double>::infinity())
    , xMax(std::numeric_limits<double>::infinity())
    , yMin(-std::numeric_limits<double>::infinity())
    , yMax(std::numeric_limits<double>::infinity())
    , displayMin(-std::numeric_limits<double>::infinity())
    , displayMax(std::numeric_limits<double>::infinity())
    , _lock(QMutex::Recursive)
    , isPeriodic(false)
    , clampKeyFramesTimeToIntegers(true)
    {
    }

    CurvePrivate(const CurvePrivate & other)
        : _lock(QMutex::Recursive)
    {
        *this = other;
    }

    void operator=(const CurvePrivate & other)
    {
        interpolator = other.interpolator->createCopy();
        listeners = other.listeners;
        keyFrames = other.keyFrames;
        type = other.type;
        xMin = other.xMin;
        xMax = other.xMax;
        yMin = other.yMin;
        yMax = other.yMax;
        displayMin = other.displayMin;
        displayMax = other.displayMax;
        isPeriodic = other.isPeriodic;
        clampKeyFramesTimeToIntegers = other.clampKeyFramesTimeToIntegers;
    }


    
};

NATRON_NAMESPACE_EXIT

#endif // Engine_CurvePrivate_h
