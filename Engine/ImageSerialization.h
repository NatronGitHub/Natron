#ifndef IMAGESERIALIZATION_H
#define IMAGESERIALIZATION_H


#include "Engine/Image.h"


CLANG_DIAG_OFF(unused-parameter)
// /opt/local/include/boost/serialization/smart_cast.hpp:254:25: warning: unused parameter 'u' [-Wunused-parameter]
#include <boost/archive/binary_iarchive.hpp>
CLANG_DIAG_ON(unused-parameter)
#include <boost/archive/binary_oarchive.hpp>
namespace boost {
namespace serialization {
template<class Archive>
void
serialize(Archive & ar,
          Natron::ImageKey & k,
          const unsigned int /*version*/ )
{
    ar & boost::serialization::make_nvp("NodeHashKey",k._nodeHashKey);
    ar & boost::serialization::make_nvp("Time",k._time);
    ar & boost::serialization::make_nvp("View",k._view);
    ar & boost::serialization::make_nvp("PixelAspect",k._pixelAspect);
}
}
}

#endif // IMAGESERIALIZATION_H
