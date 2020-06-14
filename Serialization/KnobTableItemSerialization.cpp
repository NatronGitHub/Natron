/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2020 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#include "KnobTableItemSerialization.h"

#include <iostream>

#include <boost/make_shared.hpp>

GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
#include <yaml-cpp/yaml.h>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON

#include "Serialization/BezierSerialization.h"
#include "Serialization/RotoStrokeItemSerialization.h"

SERIALIZATION_NAMESPACE_ENTER

static KnobTableItemSerializationPtr createSerializationObjectForItemTag(const std::string& tag)
{
    if (tag == kSerializationRotoGroupTag || tag == kSerializationCompLayerTag || tag == kSerializationRotoPlanarTrackGroupTag) {
        KnobTableItemSerializationPtr ret = boost::make_shared<KnobTableItemSerialization>();
        return ret;
    } else if (tag == kSerializationClosedBezierTag) {
        BezierSerializationPtr ret = boost::make_shared<BezierSerialization>(false);
        return ret;
    } else if (tag == kSerializationOpenedBezierTag) {
        BezierSerializationPtr ret = boost::make_shared<BezierSerialization>(true);
        return ret;
    } else if (tag == kSerializationStrokeBrushTypeSolid ||
               tag == kSerializationStrokeBrushTypeEraser ||
               tag == kSerializationStrokeBrushTypeClone ||
               tag == kSerializationStrokeBrushTypeReveal ||
               tag == kSerializationStrokeBrushTypeBlur ||
               tag == kSerializationStrokeBrushTypeSharpen ||
               tag == kSerializationStrokeBrushTypeSmear ||
               tag == kSerializationStrokeBrushTypeDodge ||
               tag == kSerializationStrokeBrushTypeBurn) {
        RotoStrokeItemSerializationPtr ret = boost::make_shared<RotoStrokeItemSerialization>();
        return ret;
    } else if (tag == kSerializationTrackTag) {
        KnobTableItemSerializationPtr ret = boost::make_shared<KnobTableItemSerialization>();
        return ret;
    } else {
        std::cerr << "Unknown YAML tag " << tag << std::endl;
        throw YAML::InvalidNode();
        assert(false);
        return KnobTableItemSerializationPtr();
    }
}

void
KnobTableItemSerialization::encode(YAML::Emitter& em) const
{
    if (_emitMap) {
        em << YAML::BeginMap;
    }
    em << YAML::Key << "Name" << YAML::Value << scriptName;

    // During D&D operations, this scriptname is in fact the fully qualified script-name of the item in the table
    // so that we can figure out its ancestors.
    // Otherwise when saving the project, this is a regular script-name.
    std::string actualScriptName;
    {
        std::size_t foundDot = scriptName.find_last_of(".");
        if (foundDot != std::string::npos) {
            actualScriptName = scriptName.substr(foundDot + 1);
        } else {
            actualScriptName = scriptName;
        }
    }

    if (label != actualScriptName) {
        em << YAML::Key << "Label" << YAML::Value << label;
    }
    if (!children.empty()) {
        em << YAML::Key << "Children" << YAML::Value;
        em << YAML::BeginSeq;
        for (std::list<KnobTableItemSerializationPtr>::const_iterator it = children.begin(); it != children.end(); ++it) {
            assert(!(*it)->verbatimTag.empty());
            em << YAML::VerbatimTag((*it)->verbatimTag);
            (*it)->encode(em);
        }
        em << YAML::EndSeq;
    }
    if (!knobs.empty()) {
        em << YAML::Key << "Params" << YAML::Value << YAML::BeginSeq;
        for (KnobSerializationList::const_iterator it = knobs.begin(); it!=knobs.end(); ++it) {
            (*it)->encode(em);
        }
        em << YAML::EndSeq;
    }
    if (!animationCurves.empty()) {
        em << YAML::Key << "Animation" << YAML::Value;
        if (animationCurves.size() > 1) {
            em << YAML::BeginMap;
        }
        for (std::map<std::string, CurveSerialization>::const_iterator it = animationCurves.begin(); it != animationCurves.end(); ++it) {
            if (animationCurves.size() > 1) {
                em << YAML::Key << it->first << YAML::Value;
            }
            it->second.encode(em);
        }
        if (animationCurves.size() > 1) {
            em << YAML::EndMap;
        }
    }
    
    if (_emitMap) {
        em << YAML::EndMap;
    }
}

void
KnobTableItemSerialization::decode(const YAML::Node& node)
{
    if (!node.IsMap()) {
        throw YAML::InvalidNode();
    }

    scriptName = node["Name"].as<std::string>();
    if (node["Label"]) {
        label = node["Label"].as<std::string>();
    } else {
        std::string actualScriptName;
        {
            std::size_t foundDot = scriptName.find_last_of(".");
            if (foundDot != std::string::npos) {
                actualScriptName = scriptName.substr(foundDot + 1);
            } else {
                actualScriptName = scriptName;
            }
        }
        label = actualScriptName;
    }
    if (node["Children"]) {
        const YAML::Node& childrenNode = node["Children"];
        for (std::size_t i = 0; i < childrenNode.size(); ++i) {
            const std::string& nodeTag = childrenNode[i].Tag();
            KnobTableItemSerializationPtr child = createSerializationObjectForItemTag(nodeTag);
            if (!child) {
                continue;
            }
            child->verbatimTag = nodeTag;
            child->decode(childrenNode[i]);
            children.push_back(child);
        }
    }
    if (node["Params"]) {
        const YAML::Node& paramsNode = node["Params"];
        for (std::size_t i = 0; i < paramsNode.size(); ++i) {
            KnobSerializationPtr child = boost::make_shared<KnobSerialization>();
            child->decode(paramsNode[i]);
            knobs.push_back(child);
        }
    }
    if (node["Animation"]) {
        const YAML::Node& animNode = node["Animation"];
        if (animNode.IsMap()) {
            // multi-view
            for (YAML::const_iterator it = animNode.begin(); it!=animNode.end(); ++it) {
                std::string viewName = it->first.as<std::string>();
                CurveSerialization c;
                c.decode(it->second);
                animationCurves.insert(std::make_pair(viewName, c));
            }
        } else {
            CurveSerialization c;
            c.decode(animNode);
            animationCurves.insert(std::make_pair("Main", c));
        }

    }
}

void
KnobItemsTableSerialization::encode(YAML::Emitter& em) const
{
    if (nodeScriptName.empty() && items.empty()) {
        return;
    }
    em << YAML::BeginMap;
    if (!nodeScriptName.empty()) {
        em << YAML::Key << "Node" << YAML::Value << nodeScriptName;
    }
    if (!tableIdentifier.empty()) {
        em << YAML::Key << "TableName" << YAML::Value << tableIdentifier;
    }
    if (!items.empty()) {
        em << YAML::Key << "Items" << YAML::Value << YAML::BeginSeq;
        for (std::list<KnobTableItemSerializationPtr>::const_iterator it = items.begin(); it!= items.end(); ++it) {
            assert(!(*it)->verbatimTag.empty());
            em << YAML::VerbatimTag((*it)->verbatimTag);
            (*it)->encode(em);
        }
    }
    em << YAML::EndSeq;
    em << YAML::EndMap;
}

void
KnobItemsTableSerialization::decode(const YAML::Node& node)
{
    if (!node.IsMap()) {
        throw YAML::InvalidNode();
    }
    
    if (node["Node"]) {
        nodeScriptName = node["Node"].as<std::string>();
    }

    if (node["TableName"]) {
        tableIdentifier = node["TableName"].as<std::string>();
    }
    
    if (node["Items"]) {
        const YAML::Node& itemsNode = node["Items"];
        for (std::size_t i = 0; i < itemsNode.size(); ++i) {
            const std::string& nodeTag = itemsNode[i].Tag();
            KnobTableItemSerializationPtr s = createSerializationObjectForItemTag(nodeTag);
            if (!s) {
                continue;
            }
            s->verbatimTag = nodeTag;
            s->decode(itemsNode[i]);
            items.push_back(s);
        }
    }
}



SERIALIZATION_NAMESPACE_EXIT
