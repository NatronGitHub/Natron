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

#ifndef _Engine_FormatSerialization_h_
#define _Engine_FormatSerialization_h_

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Format.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
GCC_DIAG_OFF(unused-parameter)
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
