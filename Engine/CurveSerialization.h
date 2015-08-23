//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */


#ifndef NATRON_ENGINE_CURVESERIALIZATION_H_
#define NATRON_ENGINE_CURVESERIALIZATION_H_

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "Global/Macros.h"
#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
CLANG_DIAG_OFF(unused-local-typedef)
GCC_DIAG_OFF(unused-parameter)
// /opt/local/include/boost/serialization/smart_cast.hpp:254:25: warning: unused parameter 'u' [-Wunused-parameter]
#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
// /usr/local/include/boost/serialization/shared_ptr.hpp:112:5: warning: unused typedef 'boost_static_assert_typedef_112' [-Wunused-local-typedef]
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/set.hpp>
#include <boost/serialization/scoped_ptr.hpp>
CLANG_DIAG_ON(unused-local-typedef)
GCC_DIAG_ON(unused-parameter)
#endif
#include "Engine/Curve.h"
#include "Engine/CurvePrivate.h"


template<class Archive>
void
Curve::serialize(Archive & ar,
                 const unsigned int /*version*/)
{
    QMutexLocker l(&_imp->_lock);
    ar & boost::serialization::make_nvp("KeyFrameSet",_imp->keyFrames);
}

#endif // NATRON_ENGINE_CURVESERIALIZATION_H_
