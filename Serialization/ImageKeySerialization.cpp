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

#include "ImageKeySerialization.h"

SERIALIZATION_NAMESPACE_ENTER

void
ImageKeySerialization::encode(YAML_NAMESPACE::Emitter& em) const
{
    em << YAML_NAMESPACE::Flow << YAML_NAMESPACE::BeginSeq;
    em << nodeHashKey << time << view << draft;
    em << YAML_NAMESPACE::EndSeq;
}

void
ImageKeySerialization::decode(const YAML_NAMESPACE::Node& node)
{
    if (!node.IsSequence() || node.size() != 4) {
        throw YAML_NAMESPACE::InvalidNode();
    }
    nodeHashKey = node[0].as<unsigned long long>();
    time = node[1].as<double>();
    view = node[2].as<int>();
    draft = node[3].as<bool>();
}

SERIALIZATION_NAMESPACE_EXIT


