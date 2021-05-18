/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
 * Copyright (C) 2018-2021 The Natron developers
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

#ifndef Engine_ViewIdxSerialization_h_H
#define Engine_ViewIdxSerialization_h_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Engine/ViewIdx.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
GCC_DIAG_OFF(unused-parameter)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
// /opt/local/include/boost/serialization/smart_cast.hpp:254:25: warning: unused parameter 'u' [-Wunused-parameter]
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/version.hpp>
GCC_DIAG_ON(unused-parameter)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
#endif

#define VIEWIDX_SERIALIZATION_VERSION 1

NATRON_NAMESPACE_ENTER;

template<class Archive>
void
ImageKey::serialize(Archive & ar,
                    const unsigned int version)
{
    if (version >= IMAGE_KEY_SERIALIZATION_INTRODUCES_CACHE_HOLDER_ID) {
        ar & ::boost::serialization::make_nvp("HolderID", _holderID);
    }
    ar & ::boost::serialization::make_nvp("NodeHashKey", _nodeHashKey);
    ar & ::boost::serialization::make_nvp("FrameVarying", _frameVaryingOrAnimated);
    ar & ::boost::serialization::make_nvp("Time", _time);
    ar & ::boost::serialization::make_nvp("View", _view);
    ar & ::boost::serialization::make_nvp("PixelAspect", _pixelAspect);
    ar & ::boost::serialization::make_nvp("Draft", _draftMode);
}

NATRON_NAMESPACE_EXIT;

BOOST_CLASS_VERSION(NATRON_NAMESPACE::ImageKey, IMAGE_KEY_SERIALIZATION_VERSION)


#endif // Engine_ViewIdxSerialization_h_H
