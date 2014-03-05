#ifndef IMAGEPARAMSSERIALIZATION_H
#define IMAGEPARAMSSERIALIZATION_H


#include "Engine/ImageParams.h"
#include "Global/GlobalDefines.h"
CLANG_DIAG_OFF(unused-parameter)
// /opt/local/include/boost/serialization/smart_cast.hpp:254:25: warning: unused parameter 'u' [-Wunused-parameter]
#include <boost/archive/binary_iarchive.hpp>
CLANG_DIAG_ON(unused-parameter)
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/vector.hpp>

using namespace Natron;

namespace boost {
    namespace serialization {
        template<class Archive>
        void serialize(Archive & ar,OfxRangeD& r,const unsigned int /*version*/)
        {
            ar &  boost::serialization::make_nvp("Min",r.min);
            ar &  boost::serialization::make_nvp("Max",r.max);
        }

    }
}

template<class Archive>
void ImageParams::serialize(Archive & ar,const unsigned int /*version*/)
{
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Natron::NonKeyParams);
    ar & boost::serialization::make_nvp("Rod",_rod);
    ar & boost::serialization::make_nvp("InputNbIdentity",_inputNbIdentity);
    ar & boost::serialization::make_nvp("InputTimeIdentity",_inputTimeIdentity);
    ar & boost::serialization::make_nvp("FramesNeeded",_framesNeeded);
    ar & boost::serialization::make_nvp("Components",_components);
    
}



#endif // IMAGEPARAMSSERIALIZATION_H
