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
    // This assumes that a map is already created

    RotoItemSerialization::encode(em);

    if (!_knobs.empty()) {
        em << YAML::Key << "Params" << YAML::Value;
        em << YAML::BeginSeq;
        for (std::list<KnobSerializationPtr>::const_iterator it = _knobs.begin(); it!=_knobs.end(); ++it) {
            (*it)->encode(em);
        }
        em << YAML::EndSeq;
    }

}

void
RotoDrawableItemSerialization::decode(const YAML::Node& node)
{
    RotoItemSerialization::decode(node);
    if (node["Params"]) {
        YAML::Node paramsNode = node["Params"];
        for (std::size_t i = 0; i < paramsNode.size(); ++i) {
            KnobSerializationPtr s(new KnobSerialization);
            s->decode(paramsNode[i]);
            _knobs.push_back(s);
        }
    }

}



SERIALIZATION_NAMESPACE_EXIT