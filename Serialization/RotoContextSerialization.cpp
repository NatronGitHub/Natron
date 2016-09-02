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

#include "RotoContextSerialization.h"

SERIALIZATION_NAMESPACE_ENTER

void
RotoContextSerialization::encode(YAML_NAMESPACE::Emitter& em) const
{
    em << YAML_NAMESPACE::BeginMap;
    em << YAML_NAMESPACE::Key << "BaseLayer" << YAML_NAMESPACE::Value;
    _baseLayer.encode(em);
    em << YAML_NAMESPACE::EndMap;
}

void
RotoContextSerialization::decode(const YAML_NAMESPACE::Node& node)
{
    if (node["BaseLayer"]) {
        _baseLayer.decode(node["BaseLayer"]);
    }
}

SERIALIZATION_NAMESPACE_EXIT