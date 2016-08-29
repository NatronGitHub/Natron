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

    em << YAML_NAMESPACE::BeginMap;
    em << YAML_NAMESPACE::Key << "Params" << YAML_NAMESPACE::Value;
    em << YAML_NAMESPACE::BeginSeq;
    for (std::list<KnobSerializationPtr>::const_iterator it = _knobs.begin(); it!=_knobs.end(); ++it) {
        (*it)->encode(em);
    }
    em << YAML_NAMESPACE::EndSeq;
    em << YAML_NAMESPACE::Key << "OverlayColor" << YAML_NAMESPACE::Value;
    em << YAML_NAMESPACE::Flow;
    em << YAML_NAMESPACE::BeginSeq;
    em << _overlayColor[0] << _overlayColor[1] << _overlayColor[2] << _overlayColor[3];
    em << YAML_NAMESPACE::EndSeq;
    em << YAML_NAMESPACE::EndMap;

    
}

void
RotoDrawableItemSerialization::decode(const YAML_NAMESPACE::Node& node)
{
    RotoItemSerialization::decode(node);

    for (YAML_NAMESPACE::const_iterator it = node.begin(); it!=node.end(); ++it) {
        std::string key = it->first.as<std::string>();
        if (key == "Params") {
            if (!it->second.IsSequence()) {
                throw YAML_NAMESPACE::InvalidNode();
            }
            for (YAML_NAMESPACE::const_iterator it2 = it->second.begin(); it2!=it->second.end(); ++it2) {
                KnobSerializationPtr s(new KnobSerialization);
                s->decode(it2->second);
                _knobs.push_back(s);
            }
        } else if (key == "OverlayColor") {
            if (!it->second.IsSequence() || it->second.size() != 4) {
                throw YAML_NAMESPACE::InvalidNode();
            }
            int i = 0;
            for (YAML_NAMESPACE::const_iterator it2 = it->second.begin(); it2!=it->second.end(); ++it2, ++i) {
                _overlayColor[i] = it2->as<double>();
            }
        }
    }

}



SERIALIZATION_NAMESPACE_EXIT