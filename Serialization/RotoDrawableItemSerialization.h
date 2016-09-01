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

#ifndef Engine_RotoDrawableItemSerialization_h
#define Engine_RotoDrawableItemSerialization_h

#include "Serialization/RotoItemSerialization.h"


SERIALIZATION_NAMESPACE_ENTER;

class RotoDrawableItemSerialization
    : public RotoItemSerialization
{

public:


    KnobSerializationList _knobs;
    double _overlayColor[4];

    RotoDrawableItemSerialization()
    : RotoItemSerialization()
    , _knobs()
    , _overlayColor()
    {
        _overlayColor[0] = _overlayColor[1] = _overlayColor[2] = _overlayColor[3] = -1;
    }

    virtual ~RotoDrawableItemSerialization()
    {
    }

    virtual void encode(YAML_NAMESPACE::Emitter& em) const OVERRIDE;

    virtual void decode(const YAML_NAMESPACE::Node& node) OVERRIDE;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);

};

SERIALIZATION_NAMESPACE_EXIT;


#endif // Engine_RotoDrawableItemSerialization_h
