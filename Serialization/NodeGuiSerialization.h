/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
 * Copyright (C) 2018-2020 The Natron developers
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

#ifndef NODEGUISERIALIZATION_H
#define NODEGUISERIALIZATION_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#ifdef NATRON_BOOST_SERIALIZATION_COMPAT

#include "Engine/AppManager.h"
#include "Serialization/SerializationCompat.h"
#include "Serialization/SerializationFwd.h"

#include "Gui/GuiFwd.h"


#define NODE_GUI_INTRODUCES_COLOR 2
#define NODE_GUI_INTRODUCES_SELECTED 3
#define NODE_GUI_MERGE_BACKDROP 4
#define NODE_GUI_INTRODUCES_OVERLAY_COLOR 5
#define NODE_GUI_INTRODUCES_CHILDREN 6
#define NODE_GUI_SERIALIZATION_VERSION NODE_GUI_INTRODUCES_CHILDREN


SERIALIZATION_NAMESPACE_ENTER

class NodeGuiSerialization;

typedef boost::shared_ptr<NodeGuiSerialization> NodeGuiSerializationPtr;

/**
 * @brief Deprecated, just used for backward compatibility
 **/
class NodeGuiSerialization
{
public:

    NodeGuiSerialization()
        : _posX(0.)
        , _posY(0.)
        , _width(0)
        , _height(0)
        , _previewEnabled(false)
        , _r(0.)
        , _g(0.)
        , _b(0.)
        , _colorWasFound(false)
        , _selected(false)
        , _overlayR(1.)
        , _overlayG(1.)
        , _overlayB(1.)
        , _hasOverlayColor(false)
    {
    }


    double getX() const
    {
        return _posX;
    }

    double getY() const
    {
        return _posY;
    }

    void getSize(double* w,
                 double *h) const
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

    bool getOverlayColor(double* r,
                         double* g,
                         double* b) const
    {
        if (!_hasOverlayColor) {
            return false;
        }
        *r = _overlayR;
        *g = _overlayG;
        *b = _overlayB;

        return true;
    }

    const std::list<boost::shared_ptr<NodeGuiSerialization> >& getChildren() const
    {
        return _children;
    }


    std::string _nodeName;
    double _posX, _posY;
    double _width, _height;
    bool _previewEnabled;
    float _r, _g, _b; //< color
    bool _colorWasFound;
    bool _selected;
    double _overlayR, _overlayG, _overlayB;
    bool _hasOverlayColor;

    ///If this node is a group, this is the children
    std::list<boost::shared_ptr<NodeGuiSerialization> > _children;

    friend class ::boost::serialization::access;
    template<class Archive>
    void save(Archive & ar,
              const unsigned int /*version*/) const
    {
        ar & ::boost::serialization::make_nvp("Name", _nodeName);
        ar & ::boost::serialization::make_nvp("X_position", _posX);
        ar & ::boost::serialization::make_nvp("Y_position", _posY);
        ar & ::boost::serialization::make_nvp("Preview_enabled", _previewEnabled);
        ar & ::boost::serialization::make_nvp("r", _r);
        ar & ::boost::serialization::make_nvp("g", _g);
        ar & ::boost::serialization::make_nvp("b", _b);
        ar & ::boost::serialization::make_nvp("Selected", _selected);
        ar & ::boost::serialization::make_nvp("Width", _width);
        ar & ::boost::serialization::make_nvp("Height", _height);
        ar & ::boost::serialization::make_nvp("HasOverlayColor", _hasOverlayColor);

        if (_hasOverlayColor) {
            ar & ::boost::serialization::make_nvp("oR", _overlayR);
            ar & ::boost::serialization::make_nvp("oG", _overlayG);
            ar & ::boost::serialization::make_nvp("oB", _overlayB);
        }

        int nodesCount = (int)_children.size();
        ar & ::boost::serialization::make_nvp("Children", nodesCount);

        for (std::list<boost::shared_ptr<NodeGuiSerialization> >::const_iterator it = _children.begin();
             it != _children.end();
             ++it) {
            ar & ::boost::serialization::make_nvp("item", **it);
        }
    }

    template<class Archive>
    void load(Archive & ar,
              const unsigned int version)
    {
        ar & ::boost::serialization::make_nvp("Name", _nodeName);
        ar & ::boost::serialization::make_nvp("X_position", _posX);
        ar & ::boost::serialization::make_nvp("Y_position", _posY);
        ar & ::boost::serialization::make_nvp("Preview_enabled", _previewEnabled);

        if (version >= NODE_GUI_INTRODUCES_COLOR) {
            ar & ::boost::serialization::make_nvp("r", _r);
            ar & ::boost::serialization::make_nvp("g", _g);
            ar & ::boost::serialization::make_nvp("b", _b);
            _colorWasFound = true;
        }
        if (version >= NODE_GUI_INTRODUCES_SELECTED) {
            ar & ::boost::serialization::make_nvp("Selected", _selected);
        }

        if (version >= NODE_GUI_MERGE_BACKDROP) {
            ar & ::boost::serialization::make_nvp("Width", _width);
            ar & ::boost::serialization::make_nvp("Height", _height);
        } else {
            _nodeName = NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::makeNameScriptFriendly(_nodeName);
        }

        if (version >= NODE_GUI_INTRODUCES_OVERLAY_COLOR) {
            ar & ::boost::serialization::make_nvp("HasOverlayColor", _hasOverlayColor);
            if (_hasOverlayColor) {
                ar & ::boost::serialization::make_nvp("oR", _overlayR);
                ar & ::boost::serialization::make_nvp("oG", _overlayG);
                ar & ::boost::serialization::make_nvp("oB", _overlayB);
            }
        } else {
            _hasOverlayColor = false;
        }

        if (version >= NODE_GUI_INTRODUCES_CHILDREN) {
            int nodesCount;
            ar & ::boost::serialization::make_nvp("Children", nodesCount);

            for (int i = 0; i < nodesCount; ++i) {
                NodeGuiSerializationPtr s = boost::make_shared<NodeGuiSerialization>();
                ar & ::boost::serialization::make_nvp("item", *s);
                _children.push_back(s);
            }
        }
    }

    BOOST_SERIALIZATION_SPLIT_MEMBER()
};


SERIALIZATION_NAMESPACE_EXIT


BOOST_CLASS_VERSION(SERIALIZATION_NAMESPACE::NodeGuiSerialization, NODE_GUI_SERIALIZATION_VERSION)

#endif // #ifdef NATRON_BOOST_SERIALIZATION_COMPAT

#endif // NODEGUISERIALIZATION_H
