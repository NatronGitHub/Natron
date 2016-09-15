/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
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

#include "BezierSerialization.h"

GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
#include <yaml-cpp/yaml.h>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON

#include "Serialization/BezierCPSerialization.h"

SERIALIZATION_NAMESPACE_ENTER

void
BezierSerialization::encode(YAML::Emitter& em) const
{
    em << YAML::BeginMap;
    RotoDrawableItemSerialization::encode(em);
    if (!_controlPoints.empty()) {
        em << YAML::Key << "Shape" << YAML::Value;
        em << YAML::BeginSeq;
        for (std::list< ControlPoint >::const_iterator it = _controlPoints.begin(); it != _controlPoints.end(); ++it) {
            em << YAML::BeginMap;
            em << YAML::Key << "Inner" << YAML::Value;
            it->innerPoint.encode(em);
            if (it->featherPoint) {
                em << YAML::Key << "Feather" << YAML::Value;
                it->featherPoint->encode(em);
            }
            em << YAML::EndMap;
        }
        em << YAML::EndSeq;
    }
    em << YAML::Key << "CanClose" << YAML::Value << !_isOpenBezier;
    if (!_isOpenBezier) {
        em << YAML::Key << "Closed" << YAML::Value << _closed;
    }
    em << YAML::EndMap;
}

void
BezierSerialization::decode(const YAML::Node& node)
{
    RotoDrawableItemSerialization::decode(node);
    if (node["Shape"]) {
        YAML::Node shapeNode = node["Shape"];
        for (std::size_t i = 0; i < shapeNode.size(); ++i) {
            ControlPoint c;
            YAML::Node pointNode = shapeNode[i];
            if (pointNode["Inner"]) {
                c.innerPoint.decode(pointNode["Inner"]);
            }
            if (pointNode["Feather"]) {
                c.featherPoint.reset(new BezierCPSerialization);
                c.featherPoint->decode(pointNode["Feather"]);
            }
            _controlPoints.push_back(c);
        }
    }
    if (node["Closed"]) {
        _closed = node["Closed"].as<bool>();
    }

    if (node["CanClose"]) {
        _isOpenBezier = !node["CanClose"].as<bool>();
    } else {
        _isOpenBezier = false;
    }
}

SERIALIZATION_NAMESPACE_EXIT