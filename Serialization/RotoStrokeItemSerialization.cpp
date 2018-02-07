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

#include "RotoStrokeItemSerialization.h"

#include <boost/make_shared.hpp>

GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
#include <yaml-cpp/yaml.h>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON

#include "Serialization/CurveSerialization.h"

SERIALIZATION_NAMESPACE_ENTER

void
RotoStrokeItemSerialization::encode(YAML::Emitter& em) const
{
    em << YAML::BeginMap;
    KnobTableItemSerialization::encode(em);
   
    if (!_subStrokes.empty()) {
        em << YAML::Key << "SubStrokes" << YAML::Value;
        em << YAML::BeginSeq;
        for (std::list<PointCurves>::const_iterator it = _subStrokes.begin(); it!= _subStrokes.end(); ++it) {
            em << YAML::Flow;
            em << YAML::BeginMap;
            em << YAML::Key << "x" << YAML::Value;
            it->x->encode(em);
            em << YAML::Key << "y" << YAML::Value;
            it->y->encode(em);
            em << YAML::Key << "pressure" << YAML::Value;
            it->pressure->encode(em);
            em << YAML::EndMap;
        }
        em << YAML::EndSeq;
    }
    em << YAML::EndMap;
    
}


void
RotoStrokeItemSerialization::decode(const YAML::Node& node)
{

    if (!node.IsMap()) {
        throw YAML::InvalidNode();
    }
    KnobTableItemSerialization::decode(node);

    if (node["SubStrokes"]) {
        const YAML::Node& strokesNode = node["SubStrokes"];
        for (std::size_t i = 0; i < strokesNode.size(); ++i) {
            const YAML::Node& strokeN = strokesNode[i];
            PointCurves p;
            p.x = boost::make_shared<CurveSerialization>();
            p.y = boost::make_shared<CurveSerialization>();
            p.pressure = boost::make_shared<CurveSerialization>();
            p.x->decode(strokeN["x"]);
            p.y->decode(strokeN["y"]);
            p.pressure->decode(strokeN["pressure"]);
            _subStrokes.push_back(p);
        }
    }


}

SERIALIZATION_NAMESPACE_EXIT
