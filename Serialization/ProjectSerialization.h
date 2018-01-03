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

#ifndef PROJECTSERIALIZATION_H
#define PROJECTSERIALIZATION_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Serialization/FormatSerialization.h"
#include "Serialization/NodeSerialization.h"
#include "Serialization/KnobSerialization.h"
#include "Serialization/WorkspaceSerialization.h"
#include "Serialization/SerializationFwd.h"


#define kOSTypeNameWindows "Windows"
#define kOSTypeNameMacOSX "MacOSX"
#define kOSTypeNameLinux "Linux"

SERIALIZATION_NAMESPACE_ENTER


/**
 * @brief Informations related to the version of Natron on which the project was saved
 **/
class ProjectBeingLoadedInfo
 : public SerializationObjectBase
{
public:

    ProjectBeingLoadedInfo()
    : SerializationObjectBase()
    , vMajor(0)
    , vMinor(0)
    , vRev(0)
    , gitBranch()
    , gitCommit()
    , osStr()
    , bits(64)
    {

    }

    virtual ~ProjectBeingLoadedInfo()
    {

    }


    virtual void encode(YAML::Emitter& em) const OVERRIDE;

    virtual void decode(const YAML::Node& node) OVERRIDE;

    // Software version bits
    int vMajor,vMinor,vRev;

    // Git infos: branch name and commit hash
    std::string gitBranch,gitCommit;

    // Operating system string: "Windows", "MacOSX" or "Linux"
    std::string osStr;

    // Is it 64 or 32 bit version of Natron?
    int bits;
};

class ProjectSerialization : public SerializationObjectBase
{
public:

    
    // The collection of nodes in the project
    NodeSerializationList _nodes;

    // Additional user added formats
    std::list<FormatSerialization> _additionalFormats;

    // The project settings serialization
    KnobSerializationList _projectKnobs;

    // The timeline current frame
    int _timelineCurrent;

    // Infos related to Natron version in the project being loaded
    ProjectBeingLoadedInfo _projectLoadedInfo;

    // Fully qualified name of nodes which have their properties panel opened or kNatronProjectSettingsPanelSerializationName
    std::list<std::string> _openedPanelsOrdered;

    // The project workspace serialization
    boost::shared_ptr<WorkspaceSerialization> _projectWorkspace;

    // For each viewport, its projection. They are identified by their script-name
    std::map<std::string, ViewportData> _viewportsData;

    ProjectSerialization()
    : SerializationObjectBase()
    , _nodes()
    , _additionalFormats()
    , _projectKnobs()
    , _timelineCurrent(0)
    , _projectLoadedInfo()
    , _openedPanelsOrdered()
    , _projectWorkspace()
    , _viewportsData()
    {
        
    }


    virtual ~ProjectSerialization()
    {
    }


    virtual void encode(YAML::Emitter& em) const OVERRIDE;

    virtual void decode(const YAML::Node& node) OVERRIDE;


    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
};

SERIALIZATION_NAMESPACE_EXIT

#endif // PROJECTSERIALIZATION_H
