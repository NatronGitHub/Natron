#ifndef IMAGESERIALIZATION_H
#define IMAGESERIALIZATION_H

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "Engine/Image.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
GCC_DIAG_OFF(unused-parameter)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
// /opt/local/include/boost/serialization/smart_cast.hpp:254:25: warning: unused parameter 'u' [-Wunused-parameter]
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
GCC_DIAG_ON(unused-parameter)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
#endif
namespace boost {
namespace serialization {
template<class Archive>
void
serialize(Archive & ar,
          Natron::ImageKey & k,
          const unsigned int /*version*/ )
{
    ar & boost::serialization::make_nvp("NodeHashKey",k._nodeHashKey);
    ar & boost::serialization::make_nvp("FrameVarying",k._frameVaryingOrAnimated);
    ar & boost::serialization::make_nvp("Time",k._time);
    ar & boost::serialization::make_nvp("View",k._view);
    ar & boost::serialization::make_nvp("PixelAspect",k._pixelAspect);
    ar & boost::serialization::make_nvp("Draft",k._draftMode);
}
}
}

#endif // IMAGESERIALIZATION_H
