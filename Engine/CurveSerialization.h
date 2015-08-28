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

#ifndef NATRON_ENGINE_CURVESERIALIZATION_H_
#define NATRON_ENGINE_CURVESERIALIZATION_H_

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Curve.h"

#include "Global/Macros.h"
#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
GCC_DIAG_OFF(unused-parameter)
// /opt/local/include/boost/serialization/smart_cast.hpp:254:25: warning: unused parameter 'u' [-Wunused-parameter]
#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
// /usr/local/include/boost/serialization/shared_ptr.hpp:112:5: warning: unused typedef 'boost_static_assert_typedef_112' [-Wunused-local-typedef]
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/set.hpp>
#include <boost/serialization/scoped_ptr.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
GCC_DIAG_ON(unused-parameter)
#endif

#include "Engine/CurvePrivate.h"

template<class Archive>
void
KeyFrame::serialize(Archive & ar,
                    const unsigned int /*version*/)
{
    ar & boost::serialization::make_nvp("Time",_time);
    ar & boost::serialization::make_nvp("Value",_value);
    ar & boost::serialization::make_nvp("InterpolationMethod",_interpolation);
    ar & boost::serialization::make_nvp("LeftDerivative",_leftDerivative);
    ar & boost::serialization::make_nvp("RightDerivative",_rightDerivative);
}

template<class Archive>
void
Curve::serialize(Archive & ar,
                 const unsigned int /*version*/)
{
    QMutexLocker l(&_imp->_lock);
    ar & boost::serialization::make_nvp("KeyFrameSet",_imp->keyFrames);
}

#endif // NATRON_ENGINE_CURVESERIALIZATION_H_
