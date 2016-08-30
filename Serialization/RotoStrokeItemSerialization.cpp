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

#include "RotoLayerSerialization.h"

#include "Serialization/CurveSerialization.h"

SERIALIZATION_NAMESPACE_ENTER

void
RotoStrokeItemSerialization::encode(YAML_NAMESPACE::Emitter& em) const
{
    RotoDrawableItemSerialization::encode(em);
    bool isSolid = _brushType != kRotoStrokeItemSerializationBrushTypeSolid;
    if (isSolid && _subStrokes.empty()) {
        return;
    }
    em << YAML_NAMESPACE::BeginMap;
    if (!isSolid) {
        em << YAML_NAMESPACE::Key << "Type" << YAML_NAMESPACE::Value << _brushType;
    }
    if (!_subStrokes.empty()) {
        em << YAML_NAMESPACE::Key << "SubStrokes" << YAML_NAMESPACE::Value;
        em << YAML_NAMESPACE::BeginSeq;
        for (std::list<PointCurves>::const_iterator it = _subStrokes.begin(); it!= _subStrokes.end(); ++it) {
            em << YAML_NAMESPACE::Flow;
            em << YAML_NAMESPACE::BeginMap;
            em << YAML_NAMESPACE::Key << "x" << YAML_NAMESPACE::Value;
            it->x->encode(em);
            em << YAML_NAMESPACE::Key << "y" << YAML_NAMESPACE::Value;
            it->y->encode(em);
            em << YAML_NAMESPACE::Key << "pressure" << YAML_NAMESPACE::Value;
            it->pressure->encode(em);
            em << YAML_NAMESPACE::EndMap;
        }
        em << YAML_NAMESPACE::EndSeq;
    }
    em << YAML_NAMESPACE::EndMap;
    
}


void
RotoStrokeItemSerialization::decode(const YAML_NAMESPACE::Node& node)
{
    RotoItemSerialization::decode(node);
    if (!node.IsMap()) {
        throw YAML_NAMESPACE::InvalidNode();
    }
    if (node["Type"]) {
        _brushType = node["Type"].as<std::string>();
    } else {
        _brushType = kRotoStrokeItemSerializationBrushTypeSolid;
    }

    if (node["SubStrokes"]) {
        YAML_NAMESPACE::Node strokesNode = node["SubStrokes"];
        for (std::size_t i = 0; i < strokesNode.size(); ++i) {
            YAML_NAMESPACE::Node strokeN = strokesNode[i];
            PointCurves p;
            p.x.reset(new CurveSerialization);
            p.y.reset(new CurveSerialization);
            p.pressure.reset(new CurveSerialization);
            p.x->decode(strokeN["x"]);
            p.y->decode(strokeN["y"]);
            p.pressure->decode(strokeN["pressure"]);
            _subStrokes.push_back(p);
        }
    }


}

SERIALIZATION_NAMESPACE_EXIT
