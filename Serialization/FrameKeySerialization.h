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

#ifndef FRAMEENTRYSERIALIZATION_H
#define FRAMEENTRYSERIALIZATION_H

#include "Serialization/SerializationBase.h"
#include "Serialization/ImageParamsSerialization.h"
#include "Serialization/TextureRectSerialization.h"

SERIALIZATION_NAMESPACE_ENTER;

class FrameKeySerialization
: public SerializationObjectBase
{
public:

    int frame;
    int view;
    unsigned long long treeHash;
    double gain, gamma;
    int lut;
    int bitdepth;
    int channels;
    TextureRectSerialization textureRect;
    unsigned int mipMapLevel;
    std::string inputName;
    ImageComponentsSerialization layer;
    std::string alphaChannelFullName;
    bool draftMode;

    FrameKeySerialization()
    : SerializationObjectBase()
    {

    }

    virtual ~FrameKeySerialization()
    {

    }

    virtual void encode(YAML::Emitter& em) const OVERRIDE;

    virtual void decode(const YAML::Node& node) OVERRIDE;

};


SERIALIZATION_NAMESPACE_EXIT;

#endif // FRAMEENTRYSERIALIZATION_H
