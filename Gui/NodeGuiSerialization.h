//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */


#ifndef NODEGUISERIALIZATION_H
#define NODEGUISERIALIZATION_H

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "Global/Macros.h"
#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
CLANG_DIAG_OFF(unused-parameter)
// /opt/local/include/boost/serialization/smart_cast.hpp:254:25: warning: unused parameter 'u' [-Wunused-parameter]
#include <boost/archive/xml_iarchive.hpp>
CLANG_DIAG_ON(unused-parameter)
#include <boost/archive/xml_oarchive.hpp>
#include <boost/serialization/version.hpp>
#endif
#define NODE_GUI_INTRODUCES_COLOR 2
#define NODE_GUI_INTRODUCES_SELECTED 3
#define NODE_GUI_MERGE_BACKDROP 4
#define NODE_GUI_SERIALIZATION_VERSION NODE_GUI_MERGE_BACKDROP

class NodeGui;
class NodeGuiSerialization
{
public:

    NodeGuiSerialization()
        : _posX(0.)
        , _posY(0.)
        , _previewEnabled(false)
        , _r(0.)
        , _g(0.)
        , _b(0.)
        , _colorWasFound(false)
        , _selected(false)
    {
    }

    void initialize(const NodeGui* n);

    double getX() const
    {
        return _posX;
    }

    double getY() const
    {
        return _posY;
    }
    
    void getSize(double* w,double *h) const
    {
        *w = _width;
        *h = _height;
    }

    bool isPreviewEnabled() const
    {
        return _previewEnabled;
    }

    const std::string & getFullySpecifiedName() const
    {
        return _nodeName;
    }

    void getColor(float* r,
                  float *g,
                  float* b) const
    {
        *r = _r;
        *g = _g;
        *b = _b;
    }

    bool colorWasFound() const
    {
        return _colorWasFound;
    }

    bool isSelected() const
    {
        return _selected;
    }

private:

    std::string _nodeName;
    double _posX,_posY;
    double _width,_height;
    bool _previewEnabled;
    float _r,_g,_b; //< color
    bool _colorWasFound;
    bool _selected;

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar,
                   const unsigned int version)
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
        
        if (version >= NODE_GUI_MERGE_BACKDROP) {
            ar & boost::serialization::make_nvp("Width",_width);
            ar & boost::serialization::make_nvp("Height",_height);
        }
    }
};

BOOST_CLASS_VERSION(NodeGuiSerialization, NODE_GUI_SERIALIZATION_VERSION)


#endif // NODEGUISERIALIZATION_H
