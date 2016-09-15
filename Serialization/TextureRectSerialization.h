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

#ifndef NATRON_ENGINE_TEXTURERECTSERIALIZATION_H
#define NATRON_ENGINE_TEXTURERECTSERIALIZATION_H

#include "Serialization/RectISerialization.h"
#include "Serialization/SerializationFwd.h"

SERIALIZATION_NAMESPACE_ENTER

class TextureRectSerialization
: public SerializationObjectBase
{
public:

    RectISerialization rect;
    int closestPo2;
    double par;

    TextureRectSerialization()
    : SerializationObjectBase()
    , rect()
    , closestPo2(0)
    , par(0)
    {

    }

    virtual ~TextureRectSerialization()
    {

    }

    virtual void encode(YAML::Emitter& em) const OVERRIDE;

    virtual void decode(const YAML::Node& node) OVERRIDE;

};


SERIALIZATION_NAMESPACE_EXIT

#endif // NATRON_ENGINE_TEXTURERECTSERIALIZATION_H
