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

#ifndef KNOBSERIALIZATION_H
#define KNOBSERIALIZATION_H
#include <map>

#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/serialization/map.hpp>

#include "Engine/Variant.h"
#include "Engine/CurveSerialization.h"


class Curve;
class KnobSerialization
{
    std::map<int,Variant> _values;
    int _dimension;
    /* the keys for a specific dimension*/
    std::map<int, boost::shared_ptr<Curve> > _curves;
    std::map<int, std::string > _masters;
    std::multimap<int, std::string > _slaves;

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        (void)version;
        ar & boost::serialization::make_nvp("Dimension",_dimension);
        ar & boost::serialization::make_nvp("Values",_values);
        ar & boost::serialization::make_nvp("Curves",_curves);
        ar & boost::serialization::make_nvp("Masters",_masters);
        ar & boost::serialization::make_nvp("Slaves",_slaves);
    }


public:

    KnobSerialization();

    void initialize(const Knob* knob);

    const std::map<int,Variant>& getValues() const { return _values; }

    int getDimension() const { return _dimension; }

    const std::map<int, boost::shared_ptr<Curve> >& getCurves() const { return _curves; }

    const std::map<int,std::string >& getMasters() const { return _masters; }

    const std::multimap<int, std::string >& getSlaves() const { return _slaves; }

};


#endif // KNOBSERIALIZATION_H
