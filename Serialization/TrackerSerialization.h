/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
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

#ifndef TRACKERSERIALIZATION_H
#define TRACKERSERIALIZATION_H

#include "Serialization/KnobSerialization.h"
#include "Serialization/SerializationFwd.h"

SERIALIZATION_NAMESPACE_ENTER;

class TrackSerialization
: public SerializationObjectBase
{

public:


    bool _enabled;
    bool _isPM;
    std::string _label, _scriptName;
    std::list<KnobSerializationPtr> _knobs;
    std::list<int> _userKeys;

    TrackSerialization()
        : SerializationObjectBase()
        , _enabled(true)
        , _isPM(false)
        , _label()
        , _scriptName()
        , _knobs()
        , _userKeys()
    {
    }

    virtual void encode(YAML::Emitter& em) const OVERRIDE;

    virtual void decode(const YAML::Node& node) OVERRIDE;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);


};


class TrackerContextSerialization
: public SerializationObjectBase
{
public:

    TrackerContextSerialization()
        : SerializationObjectBase()
        , _tracks()
    {
    }

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);


    virtual void encode(YAML::Emitter& em) const OVERRIDE;

    virtual void decode(const YAML::Node& node) OVERRIDE;

    std::list<TrackSerialization> _tracks;
};

SERIALIZATION_NAMESPACE_EXIT;

#endif // TRACKERSERIALIZATION_H
