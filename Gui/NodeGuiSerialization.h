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


#ifndef NODEGUISERIALIZATION_H
#define NODEGUISERIALIZATION_H

#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/serialization/version.hpp>

class NodeGui;
class NodeGuiSerialization
{
    
public:
    
    NodeGuiSerialization(){}
    
    void initialize(const NodeGui* n);

    double getX() const {return _posX;}
    
    double getY() const {return _posY;}
    
    bool isPreviewEnabled() const {return _previewEnabled;}
    
    const std::string& getName() const {return _nodeName;}
private:

    std::string _nodeName;
    double _posX,_posY;
    bool _previewEnabled;

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        (void)version;
        ar & boost::serialization::make_nvp("Name",_nodeName);
        ar & boost::serialization::make_nvp("X_position",_posX);
        ar & boost::serialization::make_nvp("Y_position",_posY);
        ar & boost::serialization::make_nvp("Preview_enabled",_previewEnabled);
    }


};
BOOST_CLASS_VERSION(NodeGuiSerialization, 1)


#endif // NODEGUISERIALIZATION_H
