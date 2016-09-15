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

GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
#include <yaml-cpp/yaml.h>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON

#include "Serialization/BezierSerialization.h"
#include "Serialization/RotoStrokeItemSerialization.h"

SERIALIZATION_NAMESPACE_ENTER

void
RotoLayerSerialization::encode(YAML::Emitter& em) const
{
    em << YAML::BeginMap;
    RotoItemSerialization::encode(em);
    if (!children.empty()) {
        em << YAML::Key << "Children" << YAML::Value << YAML::BeginSeq;
        for (std::list<RotoItemSerializationPtr>::const_iterator it = children.begin(); it != children.end(); ++it) {
            em << YAML::BeginMap;
            BezierSerializationPtr isBezier = boost::dynamic_pointer_cast<BezierSerialization>(*it);
            RotoStrokeItemSerializationPtr isStroke = boost::dynamic_pointer_cast<RotoStrokeItemSerialization>(*it);
            RotoLayerSerializationPtr isLayer = boost::dynamic_pointer_cast<RotoLayerSerialization>(*it);
            assert(isBezier || isStroke || isLayer);
            em << YAML::Key << "Type" << YAML::Value;
            if (isBezier) {
                em << "Bezier";
                em << YAML::Key << "Item" << YAML::Value;
                isBezier->encode(em);
            } else if (isStroke) {
                em << "Stroke";
                em << YAML::Key << "Item" << YAML::Value;
                isStroke->encode(em);
            } else if (isLayer) {
                em << "Layer";
                em << YAML::Key << "Item" << YAML::Value;
                isLayer->encode(em);
            }
            em << YAML::EndMap;
        }
        em << YAML::EndSeq;
    }
    if (!knobs.empty()) {
        em << YAML::Key << "Params" << YAML::Value;
        em << YAML::BeginSeq;
        for (KnobSerializationList::const_iterator it = knobs.begin(); it!=knobs.end(); ++it) {
            (*it)->encode(em);
        }
        em << YAML::EndSeq;
    }
    em << YAML::EndMap;
}

void
RotoLayerSerialization::decode(const YAML::Node& node)
{
    RotoItemSerialization::decode(node);
    if (!node.IsMap()) {
        throw YAML::InvalidNode();
    }
    if (node["Children"]) {
        YAML::Node childrenNode = node["Children"];
        for (std::size_t i = 0; i < childrenNode.size(); ++i) {
            YAML::Node childNode = childrenNode[i];
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
    if (node["Params"]) {
        YAML::Node paramsNode = node["Params"];
        for (std::size_t i = 0; i < paramsNode.size(); ++i) {
            KnobSerializationPtr knob(new KnobSerialization);
            knob->decode(paramsNode[i]);
            knobs.push_back(knob);
        }
    }
}

SERIALIZATION_NAMESPACE_EXIT