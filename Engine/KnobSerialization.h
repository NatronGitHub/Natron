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
#include <boost/serialization/vector.hpp>

#include "Engine/Variant.h"
#include "Engine/CurveSerialization.h"


class Curve;
class KnobSerialization
{
    bool _hasAnimation;
    std::string _label;
    std::vector<Variant> _values;
    int _dimension;
    /* the keys for a specific dimension*/
    std::vector< boost::shared_ptr<Curve> > _curves;
    std::vector< std::pair< int, std::string > > _masters;
    
    std::string _extraData;

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        (void)version;
        ar & boost::serialization::make_nvp("Label",_label);
        ar & boost::serialization::make_nvp("Dimension",_dimension);
        ar & boost::serialization::make_nvp("Values",_values);
        ar & boost::serialization::make_nvp("HasAnimation",_hasAnimation);
        if (_hasAnimation) {
            ar & boost::serialization::make_nvp("Curves",_curves);
        }
        ar & boost::serialization::make_nvp("Masters",_masters);
        ar & boost::serialization::make_nvp("Extra_datas",_extraData);
    }


public:

    KnobSerialization();

    void initialize(const Knob* knob);
    
    const std::string& getLabel() const { return _label; }

    const std::vector<Variant>& getValues() const { return _values; }

    int getDimension() const { return _dimension; }

    const  std::vector< boost::shared_ptr<Curve> >& getCurves() const { return _curves; }

    const std::vector< std::pair<int,std::string> > & getMasters() const { return _masters; }

    const std::string& getExtraData() const { return _extraData; }
};


#endif // KNOBSERIALIZATION_H
