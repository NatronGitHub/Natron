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



#ifndef CURVESERIALIZATION_H
#define CURVESERIALIZATION_H

#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/map.hpp>
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


template<class Archive>
void save(Archive & ar,const AnimatingParamPrivate& v ,const unsigned int version)
{
    (void)version;
    ar & boost::serialization::make_nvp("dimension",v._dimension);
    ar & boost::serialization::make_nvp("Values",v._value);
    std::map<int,boost::shared_ptr<Curve> > toSerialize;
    for(std::map<int,boost::shared_ptr<Curve> >::const_iterator it = v._curves.begin(); it!=v._curves.end();++it){
        if(it->second->isAnimated()){
            toSerialize.insert(*it);
        }
    }
    ar & boost::serialization::make_nvp("Curves",toSerialize);
}

template<class Archive>
void load(Archive & ar,AnimatingParamPrivate& v ,const unsigned int version)
{
    (void)version;
    ar & boost::serialization::make_nvp("dimension",v._dimension);
    ar & boost::serialization::make_nvp("Values",v._value);
    
    for(int i = 0 ; i < v._dimension;++i){
        v._curves.insert(std::make_pair(i,boost::shared_ptr<Curve>()));
    }
    std::map<int,boost::shared_ptr<Curve> > serializedCurves;
    ar & boost::serialization::make_nvp("Curves",serializedCurves);
    
    for(std::map<int,boost::shared_ptr<Curve> >::iterator it = v._curves.begin(); it!=v._curves.end();++it){
        std::map<int,boost::shared_ptr<Curve> >::const_iterator found = serializedCurves.find(it->first);
        if(found != serializedCurves.end()){
            it->second->clone(*found->second);
        }
    }
}
    
}
}
BOOST_SERIALIZATION_SPLIT_FREE(AnimatingParamPrivate)


template<class Archive>
void KeyFrame::serialize(Archive & ar, const unsigned int version)
{
    (void)version;
    ar & boost::serialization::make_nvp("PIMPL",_imp);
}
template void KeyFrame::serialize<boost::archive::xml_iarchive>(
boost::archive::xml_iarchive & ar,
const unsigned int file_version
);
template void KeyFrame::serialize<boost::archive::xml_oarchive>(
boost::archive::xml_oarchive & ar,
const unsigned int file_version
);


template<class Archive>
void Curve::serialize(Archive & ar, const unsigned int version)
{
    (void)version;
    ar & boost::serialization::make_nvp("PIMPL",_imp);
}
template void Curve::serialize<boost::archive::xml_iarchive>(
boost::archive::xml_iarchive & ar,
const unsigned int file_version
);
template void Curve::serialize<boost::archive::xml_oarchive>(
boost::archive::xml_oarchive & ar,
const unsigned int file_version
);


template<class Archive>
void AnimatingParam::serialize(Archive & ar, const unsigned int version)
{
    (void)version;
    ar & boost::serialization::make_nvp("PIMPL",_imp);
}
template void AnimatingParam::serialize<boost::archive::xml_iarchive>(
boost::archive::xml_iarchive & ar,
const unsigned int file_version
);
template void AnimatingParam::serialize<boost::archive::xml_oarchive>(
boost::archive::xml_oarchive & ar,
const unsigned int file_version
);

#endif // CURVESERIALIZATION_H
