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

#ifdef NATRON_BOOST_SERIALIZATION_COMPAT
#include "Engine/TrackerContext.h"
#define TRACK_SERIALIZATION_ADD_TRACKER_PM 2
#define TRACK_SERIALIZATION_VERSION TRACK_SERIALIZATION_ADD_TRACKER_PM
#define TRACKER_CONTEXT_SERIALIZATION_VERSION 1
#endif

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


#ifdef NATRON_BOOST_SERIALIZATION_COMPAT
    template<class Archive>
    void save(Archive & ar,
              const unsigned int /*version*/) const
    {
        throw std::runtime_error("Saving with boost is no longer supported");
    }

    template<class Archive>
    void load(Archive & ar,
              const unsigned int version)
    {
        (void)version;
        if (version >= TRACK_SERIALIZATION_ADD_TRACKER_PM) {
            ar & boost::serialization::make_nvp("IsTrackerPM", _isPM);
        }

        ar & boost::serialization::make_nvp("Enabled", _enabled);
        ar & boost::serialization::make_nvp("ScriptName", _scriptName);
        ar & boost::serialization::make_nvp("Label", _label);
        int nbItems;
        ar & boost::serialization::make_nvp("NbItems", nbItems);
        for (int i = 0; i < nbItems; ++i) {
            KnobSerializationPtr s( new KnobSerialization() );
            ar & boost::serialization::make_nvp("Item", *s);
            _knobs.push_back(s);
        }
        int nbUserKeys;
        ar & boost::serialization::make_nvp("NbUserKeys", nbUserKeys);
        for (int i = 0; i < nbUserKeys; ++i) {
            int key;
            ar & boost::serialization::make_nvp("UserKeys", key);
            _userKeys.push_back(key);
        }
    }

    BOOST_SERIALIZATION_SPLIT_MEMBER()
#endif


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


#ifdef NATRON_BOOST_SERIALIZATION_COMPAT
    template<class Archive>
    void save(Archive & ar,
              const unsigned int version) const
    {
        throw std::runtime_error("Saving with boost is no longer supported");
    }

    template<class Archive>
    void load(Archive & ar,
              const unsigned int version)
    {
        (void)version;
        int nbItems;
        ar & boost::serialization::make_nvp("NbItems", nbItems);
        for (int i = 0; i < nbItems; ++i) {
            TrackSerialization s;
            ar & boost::serialization::make_nvp("Item", s);
            _tracks.push_back(s);
        }
    }

    BOOST_SERIALIZATION_SPLIT_MEMBER()
#endif

    virtual void encode(YAML::Emitter& em) const OVERRIDE;

    virtual void decode(const YAML::Node& node) OVERRIDE;

    std::list<TrackSerialization> _tracks;
};

SERIALIZATION_NAMESPACE_EXIT;

#ifdef NATRON_BOOST_SERIALIZATION_COMPAT
BOOST_CLASS_VERSION(SERIALIZATION_NAMESPACE::TrackSerialization, TRACK_SERIALIZATION_VERSION)
BOOST_CLASS_VERSION(SERIALIZATION_NAMESPACE::TrackerContextSerialization, TRACKER_CONTEXT_SERIALIZATION_VERSION)
#endif


#endif // TRACKERSERIALIZATION_H
