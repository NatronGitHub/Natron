
//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef NODEBACKDROPSERIALIZATION_H
#define NODEBACKDROPSERIALIZATION_H

#include <string>

#include "Global/Macros.h"

CLANG_DIAG_OFF(unused-parameter)
// /opt/local/include/boost/serialization/smart_cast.hpp:254:25: warning: unused parameter 'u' [-Wunused-parameter]
#include <boost/archive/xml_iarchive.hpp>
CLANG_DIAG_ON(unused-parameter)
#include <boost/archive/xml_oarchive.hpp>
#include <boost/serialization/version.hpp>
#include <boost/serialization/split_member.hpp>



#include "Engine/KnobSerialization.h"

#define NODE_BACKDROP_SERIALIZATION_VERSION 1

class NodeBackDrop;

class NodeBackDropSerialization
{
public:
    NodeBackDropSerialization();
    
    void initialize(const NodeBackDrop* n);
    
    std::string getName() const { return name; }
    
    std::string getMasterBackdropName() const { return masterBackdropName; }
    
    void getPos(double& x,double &y) const { x = posX; y = posY; }
    
    void getSize(int& w,int& h) const { w = width; h = height; }
    
    boost::shared_ptr<KnobI> getLabelSerialization() const { return label->getKnob(); }

    void getColor(float& red,float &green,float& blue) const { red = r; green = g; blue = b; }
private:
    
    double posX;
    double posY;
    int width,height;
    std::string name;
    boost::shared_ptr<KnobSerialization> label;
    float r,g,b;
    std::string masterBackdropName;
    
    friend class boost::serialization::access;
    template<class Archive>
    void save(Archive & ar, const unsigned int /*version*/) const
    {
        
        ar & boost::serialization::make_nvp("X_position",posX);
        ar & boost::serialization::make_nvp("Y_position",posY);
        ar & boost::serialization::make_nvp("Width",width);
        ar & boost::serialization::make_nvp("Height",height);
        ar & boost::serialization::make_nvp("Name",name);
        ar & boost::serialization::make_nvp("MasterName",masterBackdropName);
        ar & boost::serialization::make_nvp("Label",*label);
        ar & boost::serialization::make_nvp("r",r);
        ar & boost::serialization::make_nvp("g",g);
        ar & boost::serialization::make_nvp("b",b);
    }
    
    template<class Archive>
    void load(Archive & ar, const unsigned int /*version*/)
    {
        
        ar & boost::serialization::make_nvp("X_position",posX);
        ar & boost::serialization::make_nvp("Y_position",posY);
        ar & boost::serialization::make_nvp("Width",width);
        ar & boost::serialization::make_nvp("Height",height);
        ar & boost::serialization::make_nvp("Name",name);
        ar & boost::serialization::make_nvp("MasterName",masterBackdropName);
        label.reset(new KnobSerialization);
        ar & boost::serialization::make_nvp("Label",*label);
        ar & boost::serialization::make_nvp("r",r);
        ar & boost::serialization::make_nvp("g",g);
        ar & boost::serialization::make_nvp("b",b);
    }
    
    BOOST_SERIALIZATION_SPLIT_MEMBER()


};

BOOST_CLASS_VERSION(NodeBackDropSerialization, NODE_BACKDROP_SERIALIZATION_VERSION)

#endif // NODEBACKDROPSERIALIZATION_H
