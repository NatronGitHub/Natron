#ifndef IMAGEPARAMSSERIALIZATION_H
#define IMAGEPARAMSSERIALIZATION_H

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "ImageParams.h"

#include "Global/GlobalDefines.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
GCC_DIAG_OFF(unused-parameter)
CLANG_DIAG_OFF(unused-local-typedefs) //-Wunused-local-typedefs
GCC_DIAG_OFF(sign-compare)
// /opt/local/include/boost/serialization/smart_cast.hpp:254:25: warning: unused parameter 'u' [-Wunused-parameter]
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/map.hpp>
//vector.hpp:216:18: warning: comparison of integers of different signs: 'int' and 'base_type' (aka 'unsigned long') [-Wsign-compare]
#include <boost/serialization/vector.hpp>
GCC_DIAG_ON(unused-parameter)
CLANG_DIAG_ON(unused-local-typedefs) //-Wunused-local-typedefs
GCC_DIAG_ON(sign-compare)
#endif

using namespace Natron;

namespace boost {
namespace serialization {
template<class Archive>
void
serialize(Archive & ar,
          OfxRangeD & r,
          const unsigned int /*version*/)
{
    ar &  boost::serialization::make_nvp("Min",r.min);
    ar &  boost::serialization::make_nvp("Max",r.max);
}
    

}
}

template<class Archive>
void
ImageComponents::serialize(Archive & ar,
          const unsigned int /*version*/)
{
    ar &  boost::serialization::make_nvp("Layer",_layerName);
    ar &  boost::serialization::make_nvp("Components",_componentNames);
    ar &  boost::serialization::make_nvp("CompName",_globalComponentsName);
}

template<class Archive>
void
ImageParams::serialize(Archive & ar,
                       const unsigned int /*version*/)
{
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Natron::NonKeyParams);
    ar & boost::serialization::make_nvp("RoD",_rod);
    ar & boost::serialization::make_nvp("Bounds",_bounds);
    ar & boost::serialization::make_nvp("IsProjectFormat",_isRoDProjectFormat);
    ar & boost::serialization::make_nvp("FramesNeeded",_framesNeeded);
    ar & boost::serialization::make_nvp("Components",_components);
    ar & boost::serialization::make_nvp("MMLevel",_mipMapLevel);
}

#endif // IMAGEPARAMSSERIALIZATION_H
