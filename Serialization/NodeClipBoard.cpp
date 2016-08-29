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

#include "NodeClipBoard.h"

GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
#include <yaml-cpp/yaml.h>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON

#include "Serialization/NodeSerialization.h"

SERIALIZATION_NAMESPACE_ENTER

void
NodeClipBoard::encode(YAML::Emitter& em) const
{
    em << YAML::BeginSeq;
    for (NodeSerializationList::const_iterator it = nodes.begin(); it!=nodes.end(); ++it) {
        (*it)->encode(em);
    }
    em << YAML::EndSeq;
}

void
NodeClipBoard::decode(const YAML::Node& node)
{
    if (!node.IsSequence()) {
        throw YAML::InvalidNode();
    }
    nodes.clear();
    for (YAML::const_iterator it = node.begin(); it!=node.end(); ++it) {
        NodeSerializationPtr n(new NodeSerialization);
        n->decode(it->second);
        nodes.push_back(n);
    }
}


SERIALIZATION_NAMESPACE_EXIT