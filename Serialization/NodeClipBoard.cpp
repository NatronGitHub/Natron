/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://natrongithub.github.io/>,
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

#include "NodeClipBoard.h"

#include <boost/make_shared.hpp>

GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
#include <yaml-cpp/yaml.h>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON

#include "Serialization/NodeSerialization.h"


SERIALIZATION_NAMESPACE_ENTER


void
NodeClipBoard::encode(YAML::Emitter& em) const
{
    assert(!nodes.empty());
    if (nodes.size() == 1) {
        nodes.front()->encode(em);
    } else {
        em << YAML::BeginSeq;
        for (NodeSerializationList::const_iterator it = nodes.begin(); it!=nodes.end(); ++it) {
            (*it)->encode(em);
        }
        em << YAML::EndSeq;

    }
}


void
NodeClipBoard::decode(const YAML::Node& node)
{

    nodes.clear();
    if (node.IsSequence()) {
        for (std::size_t i = 0; i < node.size(); ++i) {
            NodeSerializationPtr n = boost::make_shared<NodeSerialization>();
            n->decode(node[i]);
            nodes.push_back(n);
        }
    } else {
        NodeSerializationPtr n = boost::make_shared<NodeSerialization>();
        n->decode(node);
        nodes.push_back(n);

    }
}


SERIALIZATION_NAMESPACE_EXIT
