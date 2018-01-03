/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
 *
 * Natron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Natron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 * ***** END LICENSE BLOCK ***** */

#ifndef NODEBACKDROPSERIALIZATION_H
#define NODEBACKDROPSERIALIZATION_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#ifdef NATRON_BOOST_SERIALIZATION_COMPAT

#include "Serialization/KnobSerialization.h"
#include "Serialization/SerializationCompat.h"
#include "Serialization/SerializationFwd.h"


#define NODE_BACKDROP_INTRODUCES_SELECTED 2
#define NODE_BACKDROP_SERIALIZATION_VERSION NODE_BACKDROP_INTRODUCES_SELECTED


SERIALIZATION_NAMESPACE_ENTER


/**
   This class is completly deprecated do not use it.
   We keep it for backward compatibility with old projects prior to Natron v1.1
 **/

class NodeBackdropSerialization
{
public:

    NodeBackdropSerialization()
    : posX(0), posY(0), width(0), height(0), name(), label(), r(0), g(0), b(0), selected(false), _isNull(true)
    {
    }

    std::string getFullySpecifiedName() const
    {
        return name;
    }

    std::string getMasterBackdropName() const
    {
        return masterBackdropName;
    }

    void getPos(double & x,
                double &y) const
    {
        x = posX; y = posY;
    }

    void getSize(int & w,
                 int & h) const
    {
        w = width; h = height;
    }


    void getColor(float & red,
                  float &green,
                  float & blue) const
    {
        red = r; green = g; blue = b;
    }

    bool isSelected() const
    {
        return selected;
    }

    bool isNull() const
    {
        return _isNull;
    }

    const KnobSerializationPtr& getLabelSerialization() const
    {
        return label;
    }
private:

    double posX;
    double posY;
    int width, height;
    std::string name;
    KnobSerializationPtr label;
    float r, g, b;
    std::string masterBackdropName;
    bool selected;
    bool _isNull;

    friend class ::boost::serialization::access;
    template<class Archive>
    void save(Archive & ar,
              const unsigned int /*version*/) const
    {
        ar & ::boost::serialization::make_nvp("X_position", posX);
        ar & ::boost::serialization::make_nvp("Y_position", posY);
        ar & ::boost::serialization::make_nvp("Width", width);
        ar & ::boost::serialization::make_nvp("Height", height);
        ar & ::boost::serialization::make_nvp("Name", name);
        ar & ::boost::serialization::make_nvp("MasterName", masterBackdropName);
        ar & ::boost::serialization::make_nvp("Label", *label);
        ar & ::boost::serialization::make_nvp("r", r);
        ar & ::boost::serialization::make_nvp("g", g);
        ar & ::boost::serialization::make_nvp("b", b);
        ar & ::boost::serialization::make_nvp("Selected", selected);
    }

    template<class Archive>
    void load(Archive & ar,
              const unsigned int version)
    {
        ar & ::boost::serialization::make_nvp("X_position", posX);
        ar & ::boost::serialization::make_nvp("Y_position", posY);
        ar & ::boost::serialization::make_nvp("Width", width);
        ar & ::boost::serialization::make_nvp("Height", height);
        ar & ::boost::serialization::make_nvp("Name", name);
        ar & ::boost::serialization::make_nvp("MasterName", masterBackdropName);

        label.reset(new KnobSerialization);
        ar & ::boost::serialization::make_nvp("Label", *label);
        ar & ::boost::serialization::make_nvp("r", r);
        ar & ::boost::serialization::make_nvp("g", g);
        ar & ::boost::serialization::make_nvp("b", b);
        if (version >= NODE_BACKDROP_INTRODUCES_SELECTED) {
            ar & ::boost::serialization::make_nvp("Selected", selected);
        }
        _isNull = false;
    }

    BOOST_SERIALIZATION_SPLIT_MEMBER()
};


SERIALIZATION_NAMESPACE_EXIT


BOOST_CLASS_VERSION(SERIALIZATION_NAMESPACE::NodeBackdropSerialization, NODE_BACKDROP_SERIALIZATION_VERSION)

#endif // #ifdef NATRON_BOOST_SERIALIZATION_COMPAT

#endif // NODEBACKDROPSERIALIZATION_H
