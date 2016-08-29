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

GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
#include <yaml-cpp/yaml.h>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON

#include "Serialization/KnobSerialization.h"

SERIALIZATION_NAMESPACE_ENTER

void
RotoDrawableItemSerialization::encode(YAML::Emitter& em) const
{
    RotoItemSerialization::encode(em);

    em << YAML::BeginMap;
    em << YAML::Key << "Params" << YAML::Value;
    em << YAML::BeginSeq;
    for (std::list<KnobSerializationPtr>::const_iterator it = _knobs.begin(); it!=_knobs.end(); ++it) {
        (*it)->encode(em);
    }
    em << YAML::EndSeq;
    em << YAML::Key << "OverlayColor" << YAML::Value;
    em << YAML::Flow;
    em << YAML::BeginSeq;
    em << _overlayColor[0] << _overlayColor[1] << _overlayColor[2] << _overlayColor[3];
    em << YAML::EndSeq;
    em << YAML::EndMap;

    
}

void
RotoDrawableItemSerialization::decode(const YAML::Node& node)
{
    RotoItemSerialization::decode(node);

    for (YAML::const_iterator it = node.begin(); it!=node.end(); ++it) {
        std::string key = it->first.as<std::string>();
        if (key == "Params") {
            if (!it->second.IsSequence()) {
                throw YAML::InvalidNode();
            }
            for (YAML::const_iterator it2 = it->second.begin(); it2!=it->second.end(); ++it2) {
                KnobSerializationPtr s(new KnobSerialization);
                s->decode(it2->second);
                _knobs.push_back(s);
            }
        } else if (key == "OverlayColor") {
            if (!it->second.IsSequence() || it->second.size() != 4) {
                throw YAML::InvalidNode();
            }
            int i = 0;
            for (YAML::const_iterator it2 = it->second.begin(); it2!=it->second.end(); ++it2, ++i) {
                _overlayColor[i] = it2->as<double>();
            }
        }
    }

}



SERIALIZATION_NAMESPACE_EXIT