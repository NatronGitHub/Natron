/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "SettingsSerialization.h"

#include <boost/make_shared.hpp>

GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
#include <yaml-cpp/yaml.h>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON

SERIALIZATION_NAMESPACE_ENTER

void
SettingsSerialization::encode(YAML::Emitter& em) const
{
    if (knobs.empty() && pluginsData.empty() && shortcuts.empty()) {
        return;
    }
    em << YAML::BeginMap;

    if (!knobs.empty()) {
        em << YAML::Key << "Settings" << YAML::Value;

        em << YAML::BeginSeq;
        for (KnobSerializationList::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
            (*it)->encode(em);
        }
        em << YAML::EndSeq;
    }

    if (!pluginsData.empty()) {
        em << YAML::Key << "Plugins" << YAML::Value;

        em << YAML::BeginSeq;
        for (PluginDataMap::const_iterator it = pluginsData.begin(); it != pluginsData.end(); ++it) {
            em << YAML::Flow << YAML::BeginMap;
            em << YAML::Key << "ID" << YAML::Value << it->first.identifier;
            em << YAML::Key << "MajorVersion" << YAML::Value << it->first.majorVersion;
            em << YAML::Key << "MinorVersion" << YAML::Value << it->first.minorVersion;
            em << YAML::EndMap;

            em << YAML::Flow << YAML::BeginSeq;
            if (!it->second.enabled) {
                em << "Disabled";
            }
            if (!it->second.renderScaleEnabled) {
                em << "RenderScale_Disabled";
            }

            if (!it->second.multiThreadingEnabled) {
                em << "MultiThread_Disabled";
            }

            if (!it->second.openGLEnabled) {
                em << "OpenGL_Disabled";
            }
            em << YAML::EndSeq;
            
        }
        em << YAML::EndSeq;
    }
    if (!shortcuts.empty()) {

        em << YAML::Key << "Shortcuts" << YAML::Value;

        em << YAML::BeginSeq;
        for (KeybindsShortcutMap::const_iterator it = shortcuts.begin(); it != shortcuts.end(); ++it) {
            em << YAML::Flow << YAML::BeginMap;
            em << YAML::Key << "Group" << YAML::Value << it->first.grouping;
            em << YAML::Key << "Action" << YAML::Value << it->first.actionID;
            em << YAML::Key << "Modifiers" << YAML::Value << YAML::Flow << YAML::BeginSeq;
            for (std::list<std::string>::const_iterator it2 = it->second.modifiers.begin(); it2 != it->second.modifiers.end(); ++it2) {
                em << *it2;
            }
            em << YAML::EndSeq;
            em << YAML::Key << "Key" << YAML::Value << it->second.symbol;
            em << YAML::EndMap;
        }

        em << YAML::EndSeq;
    }

    em << YAML::EndMap;
} // encode

void
SettingsSerialization::decode(const YAML::Node& node)
{

    if (node["Settings"]) {
        const YAML::Node& knobsNode = node["Settings"];
        for (std::size_t i = 0; i < knobsNode.size(); ++i) {
            KnobSerializationPtr n = boost::make_shared<KnobSerialization>();
            n->decode(knobsNode[i]);
            knobs.push_back(n);
        }
    }
    if (node["Plugins"]) {
        const YAML::Node& pluginNode = node["Plugins"];
        for (std::size_t i = 0; i < pluginNode.size(); ++i) {

            if (pluginNode[i].size() != 2) {
                throw YAML::InvalidNode();
            }

            PluginData data;
            PluginID id;

            const YAML::Node& idNode = pluginNode[0];
            id.identifier = idNode["ID"].as<std::string>();
            id.majorVersion = idNode["MajorVersion"].as<int>();
            id.minorVersion = idNode["MinorVersion"].as<int>();

            const YAML::Node& propsNode = pluginNode[1];

            for (std::size_t i = 0; i < propsNode.size(); ++i) {
                std::string prop = propsNode[i].as<std::string>();
                if (prop == "Disabled") {
                    data.enabled = false;
                } else if (prop == "RenderScale_Disabled") {
                    data.renderScaleEnabled = false;
                } else if (prop == "MultiThread_Disabled") {
                    data.multiThreadingEnabled = false;
                } else if (prop == "OpenGL_Disabled") {
                    data.openGLEnabled = false;
                }
            }

            pluginsData[id] = data;
        }
    }

    if (node["Shortcuts"]) {
        const YAML::Node& shortcutsNode = node["Shortcuts"];
        for (std::size_t i = 0; i < shortcutsNode.size(); ++i) {
            const YAML::Node& keybindNode = shortcutsNode[i];
            KeybindShortcutKey shortcutKey;
            KeybindShortcut shortcut;
            shortcutKey.grouping = keybindNode["Group"].as<std::string>();
            shortcutKey.actionID = keybindNode["Action"].as<std::string>();
            shortcut.symbol = keybindNode["Key"].as<std::string>();
            const YAML::Node& modifsNode = keybindNode["Modifiers"];
            for (std::size_t j = 0; j < modifsNode.size(); ++j) {
                shortcut.modifiers.push_back(modifsNode[j].as<std::string>());
            }

            shortcuts[shortcutKey] = shortcut;
        }
    }
} // decode

SERIALIZATION_NAMESPACE_EXIT
