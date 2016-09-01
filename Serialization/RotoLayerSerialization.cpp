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

#include "Serialization/BezierSerialization.h"
#include "Serialization/RotoStrokeItemSerialization.h"

SERIALIZATION_NAMESPACE_ENTER

void
RotoLayerSerialization::encode(YAML_NAMESPACE::Emitter& em) const
{
    RotoItemSerialization::encode(em);
    if (!children.empty()) {
        em << YAML_NAMESPACE::BeginMap;
        em << YAML_NAMESPACE::Key << "Children" << YAML_NAMESPACE::Value << YAML_NAMESPACE::BeginSeq;
        for (std::list<RotoItemSerializationPtr>::const_iterator it = children.begin(); it != children.end(); ++it) {
            em << YAML_NAMESPACE::BeginMap;
            BezierSerializationPtr isBezier = boost::dynamic_pointer_cast<BezierSerialization>(*it);
            RotoStrokeItemSerializationPtr isStroke = boost::dynamic_pointer_cast<RotoStrokeItemSerialization>(*it);
            RotoLayerSerializationPtr isLayer = boost::dynamic_pointer_cast<RotoLayerSerialization>(*it);
            assert(isBezier || isStroke || isLayer);
            em << YAML_NAMESPACE::Key << "Type" << YAML_NAMESPACE::Value;
            if (isBezier) {
                em << "Bezier";
                em << YAML_NAMESPACE::Key << "Item" << YAML_NAMESPACE::Value;
                isBezier->encode(em);
            } else if (isStroke) {
                em << "Stroke";
                em << YAML_NAMESPACE::Key << "Item" << YAML_NAMESPACE::Value;
                isStroke->encode(em);
            } else if (isLayer) {
                em << "Layer";
                em << YAML_NAMESPACE::Key << "Item" << YAML_NAMESPACE::Value;
                isLayer->encode(em);
            }
            em << YAML_NAMESPACE::EndMap;
        }
        em << YAML_NAMESPACE::EndSeq;
        em << YAML_NAMESPACE::EndMap;
    }
}

void
RotoLayerSerialization::decode(const YAML_NAMESPACE::Node& node)
{
    RotoItemSerialization::decode(node);
    if (!node.IsMap()) {
        throw YAML_NAMESPACE::InvalidNode();
    }
    if (node["Children"]) {
        YAML_NAMESPACE::Node childrenNode = node["Children"];
        for (std::size_t i = 0; i < childrenNode.size(); ++i) {
            YAML_NAMESPACE::Node childNode = childrenNode[i];
            std::string type = childNode["Type"].as<std::string>();
            if (type == "Bezier") {
                BezierSerializationPtr s(new BezierSerialization);
                s->decode(childNode["Item"]);
                children.push_back(s);
            } else if (type == "Stroke") {
                RotoStrokeItemSerializationPtr s(new RotoStrokeItemSerialization);
                s->decode(childNode["Item"]);
                children.push_back(s);

            } else if (type == "Layer") {
                RotoLayerSerializationPtr s(new RotoLayerSerialization);
                s->decode(childNode["Item"]);
                children.push_back(s);

            }
        }
    }
}

SERIALIZATION_NAMESPACE_EXIT