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

#ifndef IMAGEPARAMSSERIALIZATION_H
#define IMAGEPARAMSSERIALIZATION_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include "ImageParams.h"

#include "Global/GlobalDefines.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
// clang-format off
GCC_DIAG_OFF(unused-parameter)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
GCC_DIAG_OFF(sign-compare)
// /opt/local/include/boost/serialization/smart_cast.hpp:254:25: warning: unused parameter 'u' [-Wunused-parameter]
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/version.hpp>
GCC_DIAG_ON(sign-compare)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
GCC_DIAG_ON(unused-parameter)
// clang-format on
#endif

#include "Engine/EngineFwd.h"



// Note: these classes are used for cache serialization and do not have to maintain backward compatibility

namespace boost {
namespace serialization {
template<class Archive>
void
serialize(Archive & ar,
          OfxRangeD & r,
          const unsigned int /*version*/)
{
    ar &  boost::serialization::make_nvp("Min", r.min);
    ar &  boost::serialization::make_nvp("Max", r.max);
}
}
}

NATRON_NAMESPACE_ENTER

template<class Archive>
void
ImagePlaneDesc::save(Archive & ar,
                           const unsigned int /*version*/) const
{
    ar &  boost::serialization::make_nvp("PlaneID", _planeID);
    ar &  boost::serialization::make_nvp("PlaneLabel", _planeLabel);
    ar &  boost::serialization::make_nvp("ChannelsLabel", _channelsLabel);
    ar &  boost::serialization::make_nvp("Channels", _channels);
}

template<class Archive>
void
ImagePlaneDesc::load(Archive & ar,
                     const unsigned int version)
{
    if (version < IMAGEPLANEDESC_SERIALIZATION_INTRODUCES_ID) {
        ar &  boost::serialization::make_nvp("Layer", _planeID);
        _planeLabel = _planeID;
        ar &  boost::serialization::make_nvp("Components", _channels);
        ar &  boost::serialization::make_nvp("CompName", _channelsLabel);
    } else {
        ar &  boost::serialization::make_nvp("PlaneID", _planeID);
        ar &  boost::serialization::make_nvp("PlaneLabel", _planeLabel);
        ar &  boost::serialization::make_nvp("ChannelsLabel", _channelsLabel);
        ar &  boost::serialization::make_nvp("Channels", _channels);
    }
}

template<class Archive>
void
ImageParams::serialize(Archive & ar,
                       const unsigned int /*version*/)
{
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(NonKeyParams);
    ar & ::boost::serialization::make_nvp("RoD", _rod);
    ar & ::boost::serialization::make_nvp("IsProjectFormat", _isRoDProjectFormat);
    // backward compatibility is not necessary
    //if (version < IMAGE_SERIALIZATION_REMOVE_FRAMESNEEDED) {
    //    std::map<int, std::map<int,std::vector<RangeD> > > f;
    //    ar & ::boost::serialization::make_nvp("FramesNeeded",f);
    //}
    ar & ::boost::serialization::make_nvp("Components", _components);
    ar & ::boost::serialization::make_nvp("MMLevel", _mipmapLevel);
    ar & ::boost::serialization::make_nvp("Premult", _premult);
    ar & ::boost::serialization::make_nvp("Fielding", _fielding);
}

NATRON_NAMESPACE_EXIT

#endif // IMAGEPARAMSSERIALIZATION_H
