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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****


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

    KnobTableItemSerialization::encode(em);
    if (_isOpenBezier) {
        em << YAML::Key << "OpenBezier" << YAML::Value << true;
    }

    if (!_shapes.empty()) {
        em << YAML::Key << "Shape" << YAML::Value;

        if (_shapes.size() > 1) {
            // Multi-view
            em << YAML::BeginMap;
        }
        for (PerViewShapeMap::const_iterator it = _shapes.begin(); it != _shapes.end(); ++it) {
            if (_shapes.size() > 1) {
                em << YAML::Key << it->first << YAML::Value;
            }
            em << YAML::BeginMap;
            if (!_isOpenBezier) {
                em << YAML::Key << "Finished" << YAML::Value << it->second.closed;
            }
            em << YAML::Key << "ControlPoints" << YAML::Value;
            em << YAML::BeginSeq;
            for (std::list< ControlPoint >::const_iterator it2 = it->second.controlPoints.begin(); it2 != it->second.controlPoints.end(); ++it2) {
                em << YAML::BeginMap;
                em << YAML::Key << "Inner" << YAML::Value;
                it2->innerPoint.encode(em);
                if (it2->featherPoint) {
                    em << YAML::Key << "Feather" << YAML::Value;
                    it2->featherPoint->encode(em);
                }
                em << YAML::EndMap;
            }
            em << YAML::EndSeq;
            em << YAML::EndMap;

        }
        if (_shapes.size() > 1) {
            em << YAML::EndMap;
        }

    }

    em << YAML::EndMap;
}

static void decodeShape(const YAML::Node& shapeNode, BezierSerialization::Shape* shape)
{
    if (shapeNode["Finished"]) {
        shape->closed = true;
    } else {
        shape->closed = false;
    }
    if (shapeNode["ControlPoints"]) {
        const YAML::Node& cpListNode = shapeNode["ControlPoints"];
        for (std::size_t i = 0; i < cpListNode.size(); ++i) {
            BezierSerialization::ControlPoint cp;
            const YAML::Node& cpNode = cpListNode[i];
            if (cpNode["Inner"]) {
                cp.innerPoint.decode(cpNode["Inner"]);
            }
            if (cpNode["Feather"]) {
                cp.featherPoint.reset(new BezierCPSerialization);
                cp.featherPoint->decode(cpNode["Feather"]);
            }
            shape->controlPoints.push_back(cp);
        }
    }
}

void
BezierSerialization::decode(const YAML::Node& node)
{
    KnobTableItemSerialization::decode(node);
    if (node["OpenBezier"]) {
        _isOpenBezier = true;
    } else {
        _isOpenBezier = false;
    }
    if (node["Shape"]) {
        const YAML::Node& shapeNode = node["Shape"];
        if (!shapeNode["ControlPoints"]) {
            // Multi-view
            for (YAML::const_iterator it = shapeNode.begin(); it != shapeNode.end(); ++it) {
                std::string view = it->first.as<std::string>();
                Shape& shape = _shapes[view];
                decodeShape(it->second, &shape);
            }
        } else {
            Shape& shape = _shapes["Main"];
            decodeShape(shapeNode, &shape);
        }

    }
}

SERIALIZATION_NAMESPACE_EXIT
