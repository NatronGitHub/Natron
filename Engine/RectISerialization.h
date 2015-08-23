//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _Engine_RectISerialization_h_
#define _Engine_RectISerialization_h_

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "RectI.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#pragma message WARN("move serialization to a separate header")
CLANG_DIAG_OFF(unused-local-typedef)
GCC_DIAG_OFF(unused-parameter)
// /opt/local/include/boost/serialization/smart_cast.hpp:254:25: warning: unused parameter 'u' [-Wunused-parameter]
#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
CLANG_DIAG_ON(unused-local-typedef)
GCC_DIAG_ON(unused-parameter)
#endif

template<class Archive>
void RectI::serialize(Archive & ar,
                      const unsigned int version)
{
    Q_UNUSED(version);
    ar & boost::serialization::make_nvp("Left",x1);
    ar & boost::serialization::make_nvp("Bottom",y1);
    ar & boost::serialization::make_nvp("Right",x2);
    ar & boost::serialization::make_nvp("Top",y2);
}

BOOST_SERIALIZATION_ASSUME_ABSTRACT(RectI);

#endif // _Engine_RectISerialization_h_
