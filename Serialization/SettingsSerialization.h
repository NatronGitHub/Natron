/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
 * Copyright (C) 2018-2020 The Natron developers
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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#ifndef NATRON_SERIALIZATION_SETTINGSSERIALIZATION_H
#define NATRON_SERIALIZATION_SETTINGSSERIALIZATION_H

#include <map>
#include "Serialization/KnobSerialization.h"
#include "Serialization/SerializationFwd.h"


SERIALIZATION_NAMESPACE_ENTER

class SettingsSerialization
: public SerializationObjectBase

{
public:

    KnobSerializationList knobs;

    struct PluginData
    {
        // If false, the user will not be able to create an instance of this plug-in, unless
        // it is part of a loaded project.
        //
        // True by default
        bool enabled;

        // If false, the plug-in will have its action called with a render-scale of 1.
        // This is useful for buggy plug-ins that have spatial parameters but do not
        // take render scale correctly into account.
        //
        // True by default
        bool renderScaleEnabled;

        // If false, the plug-in render action will not be called by multiple concurrent threads.
        // This is the same as indicating eRenderSafetyUnsafe from the plug-in.
        //
        // True by default
        bool multiThreadingEnabled;

        // If false, Natron will never ask a render using OpenGL, even if the plug-in indicates that
        // it supports OpenGL. This is to work-around plug-ins that have a valid CPU implementation but
        // a buggy OpenGL one.
        // Note that for plug-ins that only have an OpenGL implementation, this will disable them completely
        // and is the same as setting the enabled flag to false.
        //
        // True by default
        bool openGLEnabled;

        PluginData()
        : enabled(true)
        , renderScaleEnabled(true)
        , multiThreadingEnabled(true)
        , openGLEnabled(true)
        {

        }
    };

    struct PluginID
    {
        std::string identifier;
        int majorVersion;
        int minorVersion;
    };


    struct PluginID_Compare
    {
        bool operator() (const PluginID& lhs, const PluginID& rhs) const
        {
            int comp = lhs.identifier.compare(rhs.identifier);
            if (comp < 0) {
                return true;
            } else if (comp > 0) {
                return false;
            } else {
                if (lhs.majorVersion < rhs.majorVersion) {
                    return true;
                } else if (lhs.majorVersion > rhs.majorVersion) {
                    return false;
                } else {
                    return lhs.minorVersion < rhs.minorVersion;
                }
            }
        }
    };

    typedef std::map<PluginID, PluginData, PluginID_Compare> PluginDataMap;
    PluginDataMap pluginsData;

    struct KeybindShortcut
    {
        std::list<std::string> modifiers;
        std::string symbol;
    };

    struct KeybindShortcutKey
    {
        std::string grouping;
        std::string actionID;
    };

    struct KeybindShortcutKey_Compare
    {
        bool operator() (const KeybindShortcutKey& lhs, const KeybindShortcutKey& rhs) const
        {

            int comp = lhs.grouping.compare(rhs.grouping);
            if (comp < 0) {
                return true;
            } else if (comp > 0) {
                return false;
            } else {
                return lhs.actionID < rhs.actionID;
            }

        }
    };

    typedef std::map<KeybindShortcutKey, KeybindShortcut, KeybindShortcutKey_Compare> KeybindsShortcutMap;


    KeybindsShortcutMap shortcuts;

    SettingsSerialization()
    : knobs()
    , pluginsData()
    , shortcuts()
    {

    }

    ~SettingsSerialization()
    {

    }

    virtual void encode(YAML::Emitter& em) const OVERRIDE;

    virtual void decode(const YAML::Node& node) OVERRIDE;
};

SERIALIZATION_NAMESPACE_EXIT

#endif // NATRON_SERIALIZATION_SETTINGSSERIALIZATION_H
