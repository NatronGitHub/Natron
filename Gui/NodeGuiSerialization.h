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

#include "Global/Macros.h"
CLANG_DIAG_OFF(unused-parameter)
// /opt/local/include/boost/serialization/smart_cast.hpp:254:25: warning: unused parameter 'u' [-Wunused-parameter]
#include <boost/archive/xml_iarchive.hpp>
CLANG_DIAG_ON(unused-parameter)
#include <boost/archive/xml_oarchive.hpp>
#include <boost/serialization/version.hpp>

#define NODE_GUI_INTRODUCES_COLOR 2
#define NODE_GUI_INTRODUCES_SELECTED 3
#define NODE_GUI_SERIALIZATION_VERSION NODE_GUI_INTRODUCES_SELECTED

class NodeGui;
class NodeGuiSerialization
{
    
public:
    
    NodeGuiSerialization() : _colorWasFound(false), _selected(false) {}
    
    void initialize(const boost::shared_ptr<NodeGui>& n);

    double getX() const {return _posX;}
    
    double getY() const {return _posY;}
    
    bool isPreviewEnabled() const {return _previewEnabled;}
    
    const std::string& getName() const {return _nodeName;}
    
    void getColor(float* r,float *g,float* b) const
    {
        *r = _r;
        *g = _g;
        *b = _b;
    }
    
    bool colorWasFound() const { return _colorWasFound; }
    
    bool isSelected() const { return _selected; }
private:

    std::string _nodeName;
    double _posX,_posY;
    bool _previewEnabled;
    float _r,_g,_b; //< color
    bool _colorWasFound;
    bool _selected;

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {

        ar & boost::serialization::make_nvp("Name",_nodeName);
        ar & boost::serialization::make_nvp("X_position",_posX);
        ar & boost::serialization::make_nvp("Y_position",_posY);
        ar & boost::serialization::make_nvp("Preview_enabled",_previewEnabled);
        if (version >= NODE_GUI_INTRODUCES_COLOR) {
            ar & boost::serialization::make_nvp("r",_r);
            ar & boost::serialization::make_nvp("g",_g);
            ar & boost::serialization::make_nvp("b",_b);
            _colorWasFound = true;
        }
        if (version >= NODE_GUI_INTRODUCES_SELECTED) {
            ar & boost::serialization::make_nvp("Selected",_selected);
        }
    }


};
BOOST_CLASS_VERSION(NodeGuiSerialization, NODE_GUI_SERIALIZATION_VERSION)


#endif // NODEGUISERIALIZATION_H
