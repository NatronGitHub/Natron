//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _Engine_FormatSerialization_h_
#define _Engine_FormatSerialization_h_

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "Format.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
GCC_DIAG_OFF(unused-parameter)
#pragma message WARN("move serialization to a separate header")
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/version.hpp>
#include <boost/serialization/nvp.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
GCC_DIAG_ON(unused-parameter)
#endif

#define FORMAT_SERIALIZATION_CHANGES_TO_RECTD 2
#define FORMAT_SERIALIZATION_CHANGES_TO_RECTI 3
#define FORMAT_SERIALIZATION_VERSION FORMAT_SERIALIZATION_CHANGES_TO_RECTI

template<class Archive>
void Format::serialize(Archive & ar,
                       const unsigned int version)
{
    if (version < FORMAT_SERIALIZATION_CHANGES_TO_RECTD) {
        RectI r;
        ar & boost::serialization::make_nvp("RectI",r);
        x1 = r.x1;
        x2 = r.x2;
        y1 = r.y1;
        y2 = r.y2;
    } else if (version < FORMAT_SERIALIZATION_CHANGES_TO_RECTI) {

        RectD r;
        ar & boost::serialization::make_nvp("RectD",r);
        x1 = r.x1;
        x2 = r.x2;
        y1 = r.y1;
        y2 = r.y2;

    } else {
        boost::serialization::void_cast_register<Format,RectI>( static_cast<Format *>(NULL),
                                                               static_cast<RectI *>(NULL) );
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(RectI);
    }
    ar & boost::serialization::make_nvp("Pixel_aspect_ratio",_par);
    ar & boost::serialization::make_nvp("Name",_name);
}

BOOST_CLASS_VERSION(Format, FORMAT_SERIALIZATION_VERSION)


#endif // _Engine_FormatSerialization_h_
