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

#include "Global/Macros.h"
CLANG_DIAG_OFF(unused-parameter)
// /opt/local/include/boost/serialization/smart_cast.hpp:254:25: warning: unused parameter 'u' [-Wunused-parameter]
#include <boost/archive/xml_iarchive.hpp>
CLANG_DIAG_ON(unused-parameter)
#include <boost/archive/xml_oarchive.hpp>
#include <boost/serialization/list.hpp>

#include "Engine/Variant.h"
#include "Engine/CurveSerialization.h"



struct MasterSerialization
{
    int masterDimension;
    std::string masterNodeName;
    std::string masterKnobName;
    
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        (void)version;
        ar & boost::serialization::make_nvp("MasterDimension",masterDimension);
        ar & boost::serialization::make_nvp("MasterNodeName",masterNodeName);
        ar & boost::serialization::make_nvp("MasterKnobName",masterKnobName);
    }

};

struct ValueSerialization
{
    Variant value;
    bool hasAnimation;
    Curve curve;
    bool hasMaster;
    MasterSerialization master;
    
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        (void)version;
        ar & boost::serialization::make_nvp("Value",value);
        ar & boost::serialization::make_nvp("HasAnimation",hasAnimation);
        if (hasAnimation) {
            ar & boost::serialization::make_nvp("Curve",curve);
        }
        ar & boost::serialization::make_nvp("HasMaster",hasMaster);
        if (hasMaster) {
            ar & boost::serialization::make_nvp("Master",master);
        }
    }

    
};

class Curve;
class KnobSerialization
{
    std::string _name;
    std::list<ValueSerialization> _values;
    int _dimension;
    std::string _extraData;

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        (void)version;
        ar & boost::serialization::make_nvp("Name",_name);
        ar & boost::serialization::make_nvp("Dimension",_dimension);
        ar & boost::serialization::make_nvp("Values",_values);
        ar & boost::serialization::make_nvp("Extra_datas",_extraData);
    }


public:

    KnobSerialization();

    void initialize(const Knob* knob);
    
    const std::string& getName() const { return _name; }

    int getDimension() const { return _dimension; }
    
    const std::list<ValueSerialization>& getValuesSerialized() const { return _values; }

    const std::string& getExtraData() const { return _extraData; }
};


#endif // KNOBSERIALIZATION_H
