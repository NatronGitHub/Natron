//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
*contact: immarespond at gmail dot com
*
*/



#ifndef NATRON_ENGINE_CURVESERIALIZATION_H_
#define NATRON_ENGINE_CURVESERIALIZATION_H_

#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/scoped_ptr.hpp>
#include <boost/serialization/split_free.hpp>

#include "Engine/Curve.h"
#include "Engine/CurvePrivate.h"


namespace boost {
namespace serialization {

template<class Archive>
void serialize(Archive & ar, KeyFramePrivate& k,const unsigned int version)
{
    (void)version;
    ar & boost::serialization::make_nvp("Time",k._time);
    ar & boost::serialization::make_nvp("Value",k._value);
    ar & boost::serialization::make_nvp("InterpolationMethod",k._interpolation);
    ar & boost::serialization::make_nvp("LeftTangent",k._leftTangent);
    ar & boost::serialization::make_nvp("RightTangent",k._rightTangent);
}


template<class Archive>
void serialize(Archive & ar,CurvePrivate& c, const unsigned int version)
{
    (void)version;
    ar & boost::serialization::make_nvp("KeyFrames",c._keyFrames);
}


    
}
}


template<class Archive>
void KeyFrame::serialize(Archive & ar, const unsigned int version)
{
    (void)version;
    ar & boost::serialization::make_nvp("PIMPL",_imp);
}


template<class Archive>
void Curve::serialize(Archive & ar, const unsigned int version)
{
    (void)version;
    ar & boost::serialization::make_nvp("PIMPL",_imp);
}




#endif // NATRON_ENGINE_CURVESERIALIZATION_H_
