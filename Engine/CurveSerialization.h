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

#ifndef NATRON_ENGINE_CURVESERIALIZATION_H
#define NATRON_ENGINE_CURVESERIALIZATION_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include "Curve.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
// clang-format off
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
// clang-format on
#endif

#include <locale>
#include <sstream>
#include <string>
#include <type_traits>

#include "Global/MathUtils.h"
#include "Engine/CurvePrivate.h"
#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

namespace CurveSerializationDetail {
// Number of non-finite values read by KeyFrame::serialize() in this thread,
// so that Curve::serialize() knows that it must recompute the derivatives.
inline thread_local int nonFiniteValuesRead = 0;

// Write a finite value: a NaN or an infinity can't be read back by an XML archive
// (projects saved by Natron before the fix for #1267 contain NaN derivatives).
template<class Archive>
void
saveFinite(Archive & ar,
           const char* name,
           double v)
{
    if ( !MathUtils::isFinite(v) ) {
        v = 0.;
    }
    ar & ::boost::serialization::make_nvp(name, v);
}

// Parse a number written by an XML archive. The stream uses the "C" locale, as the
// archive does, so the result doesn't depend on the process locale (strtod() would
// read "556.568" as 556 after a Python script called locale.setlocale(LC_ALL, "") on a
// French or German system). The whole string must be a number: "nan", "inf" or the
// old MSVCRT "1.#QNAN" fail, and are reported as such.
inline bool
parseDouble(const std::string& s,
            double* v)
{
    thread_local std::istringstream iss = [] {
        std::istringstream i;
        i.imbue( std::locale::classic() );
        return i;
    }();
    iss.clear();
    iss.str(s);
    iss >> *v;
    if ( iss.fail() ) {
        return false;
    }
    iss >> std::ws;

    return iss.eof() && MathUtils::isFinite(*v);
}

// Read a value that may be "nan" or "inf" in an XML archive (xml_iarchive fails on
// them with "input stream error"): read the text and parse it. A value that isn't a
// finite number is replaced by 0 and counted in nonFiniteValuesRead.
template<class Archive>
void
loadFinite(Archive & ar,
           const char* name,
           double* v)
{
    if ( std::is_same<Archive, boost::archive::xml_iarchive>::value ) {
        std::string s;
        ar & ::boost::serialization::make_nvp(name, s);
        if ( !parseDouble(s, v) ) {
            *v = 0.;
            ++nonFiniteValuesRead;
        }
    } else {
        ar & ::boost::serialization::make_nvp(name, *v);
        if ( !MathUtils::isFinite(*v) ) {
            *v = 0.;
            ++nonFiniteValuesRead;
        }
    }
}
} // namespace CurveSerializationDetail

template<class Archive>
void
KeyFrame::serialize(Archive & ar,
                    const unsigned int /*version*/)
{
    ar & ::boost::serialization::make_nvp("Time", _time);
    if (Archive::is_saving::value) {
        CurveSerializationDetail::saveFinite(ar, "Value", _value);
    } else {
        CurveSerializationDetail::loadFinite(ar, "Value", &_value);
    }
    ar & ::boost::serialization::make_nvp("InterpolationMethod", _interpolation);
    if (Archive::is_saving::value) {
        CurveSerializationDetail::saveFinite(ar, "LeftDerivative", _leftDerivative);
        CurveSerializationDetail::saveFinite(ar, "RightDerivative", _rightDerivative);
    } else {
        CurveSerializationDetail::loadFinite(ar, "LeftDerivative", &_leftDerivative);
        CurveSerializationDetail::loadFinite(ar, "RightDerivative", &_rightDerivative);
    }
}

template<class Archive>
void
Curve::serialize(Archive & ar,
                 const unsigned int /*version*/)
{
    QMutexLocker l(&_imp->_lock);
    const int nonFiniteBefore = CurveSerializationDetail::nonFiniteValuesRead;
    ar & ::boost::serialization::make_nvp("KeyFrameSet", _imp->keyFrames);
    if ( Archive::is_loading::value && (CurveSerializationDetail::nonFiniteValuesRead != nonFiniteBefore) ) {
        // NaN values (#1267) were replaced by 0: recompute the automatic derivatives.
        // The curve is usually a temporary that gets cloned or copied into the knob's curve,
        // which is periodic or not, so the flag is kept and clone() recomputes them again.
        _imp->mustRefreshDerivatives = true;
        refreshAutomaticDerivatives();
    }
}

NATRON_NAMESPACE_EXIT

#endif // NATRON_ENGINE_CURVESERIALIZATION_H
