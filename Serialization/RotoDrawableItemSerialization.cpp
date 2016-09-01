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

#include "RotoDrawableItemSerialization.h"

#include "Serialization/KnobSerialization.h"

SERIALIZATION_NAMESPACE_ENTER

void
RotoDrawableItemSerialization::encode(YAML_NAMESPACE::Emitter& em) const
{
    RotoItemSerialization::encode(em);
    bool hasOverlayColor = _overlayColor[0] != -1 || _overlayColor[1] != -1 || _overlayColor[2] != -1 || _overlayColor[3] != -1;
    if (_knobs.empty() && !hasOverlayColor) {
        return;
    }
    em << YAML_NAMESPACE::BeginMap;
    if (!_knobs.empty()) {
        em << YAML_NAMESPACE::Key << "Params" << YAML_NAMESPACE::Value;
        em << YAML_NAMESPACE::BeginSeq;
        for (std::list<KnobSerializationPtr>::const_iterator it = _knobs.begin(); it!=_knobs.end(); ++it) {
            (*it)->encode(em);
        }
        em << YAML_NAMESPACE::EndSeq;
    }

    if (hasOverlayColor) {
        em << YAML_NAMESPACE::Key << "OverlayColor" << YAML_NAMESPACE::Value;
        em << YAML_NAMESPACE::Flow;
        em << YAML_NAMESPACE::BeginSeq;
        em << _overlayColor[0] << _overlayColor[1] << _overlayColor[2] << _overlayColor[3];
        em << YAML_NAMESPACE::EndSeq;
    }
    em << YAML_NAMESPACE::EndMap;

}

void
RotoDrawableItemSerialization::decode(const YAML_NAMESPACE::Node& node)
{
    RotoItemSerialization::decode(node);
    if (node["Params"]) {
        YAML_NAMESPACE::Node paramsNode = node["Params"];
        for (std::size_t i = 0; i < paramsNode.size(); ++i) {
            KnobSerializationPtr s(new KnobSerialization);
            s->decode(paramsNode[i]);
            _knobs.push_back(s);
        }
    }
    if (node["OverlayColor"]) {
        YAML_NAMESPACE::Node colorNode = node["OverlayColor"];
        if (colorNode.size() != 4) {
            throw YAML_NAMESPACE::InvalidNode();
        }
        for (std::size_t i = 0; i < colorNode.size(); ++i) {
            _overlayColor[i] = colorNode[i].as<double>();
        }
    }


}



SERIALIZATION_NAMESPACE_EXIT