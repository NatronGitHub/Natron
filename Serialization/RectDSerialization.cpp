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

#include "RectDSerialization.h"

SERIALIZATION_NAMESPACE_ENTER

void
RectDSerialization::encode(YAML_NAMESPACE::Emitter& em) const
{
    em << YAML_NAMESPACE::Flow << YAML_NAMESPACE::BeginSeq << x1 << y1 << x2 << y2 << YAML_NAMESPACE::EndSeq;
}

void
RectDSerialization::decode(const YAML_NAMESPACE::Node& node)
{
    if (!node.IsSequence() || node.size() != 4) {
        throw YAML_NAMESPACE::InvalidNode();
    }

    x1 = node[0].as<double>();
    y1 = node[1].as<double>();
    x2 = node[2].as<double>();
    y2 = node[3].as<double>();
}

SERIALIZATION_NAMESPACE_EXIT
