/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#include "Global/Macros.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
GCC_DIAG_OFF(unused-parameter)
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/version.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
GCC_DIAG_ON(unused-parameter)
#endif

#include "Engine/KnobSerialization.h"
#include "Engine/TrackerContext.h"


#define TRACK_SERIALIZATION_ADD_TRACKER_PM 2
#define TRACK_SERIALIZATION_VERSION TRACK_SERIALIZATION_ADD_TRACKER_PM
#define TRACKER_CONTEXT_SERIALIZATION_VERSION 1

NATRON_NAMESPACE_ENTER

class TrackSerialization
{
    friend class boost::serialization::access;
    friend class TrackMarker;

public:

    TrackSerialization()
        : _enabled(true)
        , _isPM(false)
        , _label()
        , _scriptName()
        , _knobs()
        , _userKeys()
    {
    }

    bool usePatternMatching() const
    {
        return _isPM;
    }

private:

    template<class Archive>
    void save(Archive & ar,
              const unsigned int version) const
    {
        (void)version;

        ar & boost::serialization::make_nvp("IsTrackerPM", _isPM);
        ar & boost::serialization::make_nvp("Enabled", _enabled);
        ar & boost::serialization::make_nvp("ScriptName", _scriptName);
        ar & boost::serialization::make_nvp("Label", _label);
        int nbItems = (int)_knobs.size();
        ar & boost::serialization::make_nvp("NbItems", nbItems);
        for (std::list<KnobSerializationPtr>::const_iterator it = _knobs.begin(); it != _knobs.end(); ++it) {
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
        if (version >= TRACK_SERIALIZATION_ADD_TRACKER_PM) {
            ar & boost::serialization::make_nvp("IsTrackerPM", _isPM);
        }

        ar & boost::serialization::make_nvp("Enabled", _enabled);
        ar & boost::serialization::make_nvp("ScriptName", _scriptName);
        ar & boost::serialization::make_nvp("Label", _label);
        int nbItems;
        ar & boost::serialization::make_nvp("NbItems", nbItems);
        for (int i = 0; i < nbItems; ++i) {
            KnobSerializationPtr s = boost::make_shared<KnobSerialization>();
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
    bool _isPM;
    std::string _label, _scriptName;
    std::list<KnobSerializationPtr> _knobs;
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

NATRON_NAMESPACE_EXIT

BOOST_CLASS_VERSION(NATRON_NAMESPACE::TrackSerialization, TRACK_SERIALIZATION_VERSION)
BOOST_CLASS_VERSION(NATRON_NAMESPACE::TrackerContextSerialization, TRACKER_CONTEXT_SERIALIZATION_VERSION)


#endif // TRACKERSERIALIZATION_H
