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

#include "FrameKeySerialization.h"

SERIALIZATION_NAMESPACE_ENTER

void
FrameKeySerialization::encode(YAML_NAMESPACE::Emitter& em) const
{
    em << YAML_NAMESPACE::Flow << YAML_NAMESPACE::BeginSeq;
    em << frame << view << treeHash << gain << gamma << lut << bitdepth << channels;
    textureRect.encode(em);
    em << mipMapLevel << inputName;
    layer.encode(em);
    em << alphaChannelFullName;
    em << draftMode;
    em << YAML_NAMESPACE::EndSeq;
}

void
FrameKeySerialization::decode(const YAML_NAMESPACE::Node& node)
{
    if (!node.IsSequence() || node.size() != 14) {
        throw YAML_NAMESPACE::InvalidNode();
    }
    frame = node[0].as<int>();
    view = node[1].as<int>();
    treeHash = node[2].as<unsigned long long>();
    gain = node[3].as<double>();
    gamma = node[4].as<double>();
    lut = node[5].as<int>();
    bitdepth = node[6].as<int>();
    channels = node[7].as<int>();
    textureRect.decode(node[8]);
    mipMapLevel = node[9].as<unsigned int>();
    inputName = node[10].as<std::string>();
    layer.decode(node[11]);
    alphaChannelFullName = node[12].as<std::string>();
    draftMode = node[13].as<bool>();
   
}

SERIALIZATION_NAMESPACE_EXIT


