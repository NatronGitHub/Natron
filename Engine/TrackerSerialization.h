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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****


#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/version.hpp>
#endif

#include "Engine/KnobSerialization.h"
#include "Engine/TrackerContext.h"


#define TRACK_SERIALIZATION_VERSION 1
#define TRACKER_CONTEXT_SERIALIZATION_VERSION 1

NATRON_NAMESPACE_ENTER;

class TrackSerialization
{
    friend class boost::serialization::access;
    friend class TrackMarker;

public:

    TrackSerialization()
        : _enabled(true)
        , _label()
        , _scriptName()
        , _knobs()
        , _userKeys()
    {
    }

private:

    template<class Archive>
    void save(Archive & ar,
              const unsigned int version) const
    {
        (void)version;
        ar & boost::serialization::make_nvp("Enabled", _enabled);
        ar & boost::serialization::make_nvp("ScriptName", _scriptName);
        ar & boost::serialization::make_nvp("Label", _label);
        int nbItems = (int)_knobs.size();
        ar & boost::serialization::make_nvp("NbItems", nbItems);
        for (std::list<boost::shared_ptr<KnobSerialization> >::const_iterator it = _knobs.begin(); it != _knobs.end(); ++it) {
            ar & boost::serialization::make_nvp("Item", **it);
        }

        int nbUserKeys = (int)_userKeys.size();
        ar & boost::serialization::make_nvp("NbUserKeys", nbUserKeys);
        for (std::list<int>::const_iterator it = _userKeys.begin(); it != _userKeys.end(); ++it) {
            ar & boost::serialization::make_nvp("UserKeys", *it);
        }
    }

    template<class Archive>
    void load(Archive & ar,
              const unsigned int version)
    {
        (void)version;
        ar & boost::serialization::make_nvp("Enabled", _enabled);
        ar & boost::serialization::make_nvp("ScriptName", _scriptName);
        ar & boost::serialization::make_nvp("Label", _label);
        int nbItems;
        ar & boost::serialization::make_nvp("NbItems", nbItems);
        for (int i = 0; i < nbItems; ++i) {
            boost::shared_ptr<KnobSerialization> s( new KnobSerialization() );
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


    bool _enabled;
    std::string _label, _scriptName;
    std::list<boost::shared_ptr<KnobSerialization> > _knobs;
    std::list<int> _userKeys;
};


class TrackerContextSerialization
{
    friend class boost::serialization::access;
    friend class TrackerContext;

public:

    TrackerContextSerialization()
        : _tracks()
    {
    }

private:

    template<class Archive>
    void save(Archive & ar,
              const unsigned int version) const
    {
        (void)version;
        int nbItems = _tracks.size();
        ar & boost::serialization::make_nvp("NbItems", nbItems);
        for (std::list<TrackSerialization>::const_iterator it = _tracks.begin(); it != _tracks.end(); ++it) {
            ar & boost::serialization::make_nvp("Item", *it);
        }
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

    std::list<TrackSerialization> _tracks;
};

NATRON_NAMESPACE_EXIT;

BOOST_CLASS_VERSION(NATRON_NAMESPACE::TrackSerialization, TRACK_SERIALIZATION_VERSION)
BOOST_CLASS_VERSION(NATRON_NAMESPACE::TrackerContextSerialization, TRACKER_CONTEXT_SERIALIZATION_VERSION)


#endif // TRACKERSERIALIZATION_H
