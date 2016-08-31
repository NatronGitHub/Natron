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

#ifndef PROJECTSERIALIZATION_H
#define PROJECTSERIALIZATION_H

#include "Global/GitVersion.h"
#include "Global/GlobalDefines.h"
#include "Global/MemoryInfo.h"

#include "Serialization/FormatSerialization.h"
#include "Serialization/NodeSerialization.h"
#include "Serialization/KnobSerialization.h"
#include "Serialization/WorkspaceSerialization.h"


#ifdef NATRON_BOOST_SERIALIZATION_COMPAT

#include "Engine/Format.h"
#include "Serialization/NodeGroupSerialization.h"

#define PROJECT_SERIALIZATION_INTRODUCES_NATRON_VERSION 2
#define PROJECT_SERIALIZATION_REMOVES_NODE_COUNTERS 3
#define PROJECT_SERIALIZATION_REMOVES_TIMELINE_BOUNDS 4
#define PROJECT_SERIALIZATION_INTRODUCES_GROUPS 5
#define PROJECT_SERIALIZATION_CHANGE_VERSION_SERIALIZATION 6
#define PROJECT_SERIALIZATION_VERSION PROJECT_SERIALIZATION_CHANGE_VERSION_SERIALIZATION
#endif

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


    virtual void encode(YAML_NAMESPACE::Emitter& em) const OVERRIDE;

    virtual void decode(const YAML_NAMESPACE::Node& node) OVERRIDE;

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

    // The project creation date, obtained with QDateTime::toMSecsSinceEpoch();
    long long _creationDate;

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
    , _creationDate(0)
    , _projectLoadedInfo()
    , _openedPanelsOrdered()
    , _projectWorkspace()
    , _viewportsData()
    {
        
    }


    virtual ~ProjectSerialization()
    {
    }


    virtual void encode(YAML_NAMESPACE::Emitter& em) const OVERRIDE;

    virtual void decode(const YAML_NAMESPACE::Node& node) OVERRIDE;


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

        // Dead serialization code
        if (version >= PROJECT_SERIALIZATION_INTRODUCES_NATRON_VERSION && version < PROJECT_SERIALIZATION_CHANGE_VERSION_SERIALIZATION) {
            std::string natronVersion;
            ar & ::boost::serialization::make_nvp("NatronVersion", natronVersion);
            std::string toFind(NATRON_APPLICATION_NAME " v");
            std::size_t foundV = natronVersion.find(toFind);
            if (foundV != std::string::npos) {
                foundV += toFind.size();
                toFind = " from git branch";
                std::size_t foundFrom = natronVersion.find(toFind);
                if (foundFrom != std::string::npos) {
                    std::string vStr;
                    for (std::size_t i = foundV; i < foundFrom; ++i) {
                        vStr.push_back(natronVersion[i]);
                    }
                    QStringList splits = QString::fromUtf8( vStr.c_str() ).split( QLatin1Char('.') );
                    if (splits.size() == 3) {
                        int major, minor, rev;
                        major = splits[0].toInt();
                        minor = splits[1].toInt();
                        rev = splits[2].toInt();
                        if (NATRON_VERSION_ENCODE(major, minor, rev) > NATRON_VERSION_ENCODED) {
                            throw std::invalid_argument("The given project was produced with a more recent and incompatible version of Natron.");
                        }
                    }
                }
            }
        }

        if (version >= PROJECT_SERIALIZATION_CHANGE_VERSION_SERIALIZATION) {
            ar & ::boost::serialization::make_nvp("VersionMajor", _projectLoadedInfo.vMajor);
            ar & ::boost::serialization::make_nvp("VersionMinor", _projectLoadedInfo.vMinor);
            ar & ::boost::serialization::make_nvp("VersionRev", _projectLoadedInfo.vRev);
            ar & ::boost::serialization::make_nvp("GitBranch", _projectLoadedInfo.gitBranch);
            ar & ::boost::serialization::make_nvp("GitCommit", _projectLoadedInfo.gitCommit);
            ar & ::boost::serialization::make_nvp("OS", _projectLoadedInfo.osStr);
            ar & ::boost::serialization::make_nvp("Bits", _projectLoadedInfo.bits);
       
        }

        if (version < PROJECT_SERIALIZATION_INTRODUCES_GROUPS) {
            int nodesCount;
            ar & ::boost::serialization::make_nvp("NodesCount", nodesCount);
            for (int i = 0; i < nodesCount; ++i) {
                NodeSerializationPtr ns(new NodeSerialization);
                ar & ::boost::serialization::make_nvp("item", *ns);
                _nodes.push_back(ns);
            }
        } else {
            NodeCollectionSerialization nodes;
            ar & ::boost::serialization::make_nvp("NodesCollection", nodes);
            _nodes = nodes.getNodesSerialization();
        } 

        int knobsCount;
        ar & ::boost::serialization::make_nvp("ProjectKnobsCount", knobsCount);

        for (int i = 0; i < knobsCount; ++i) {
            KnobSerializationPtr ks(new KnobSerialization);
            ar & ::boost::serialization::make_nvp("item", *ks);
            _projectKnobs.push_back(ks);
        }

        std::list<NATRON_NAMESPACE::Format> formats;
        ar & ::boost::serialization::make_nvp("AdditionalFormats", formats);
        for (std::list<NATRON_NAMESPACE::Format>::iterator it = formats.begin(); it!=formats.end(); ++it) {
            FormatSerialization s;
            s.x1 = it->x1;
            s.y1 = it->y1;
            s.x2 = it->x2;
            s.y2 = it->y2;
            s.par = it->getPixelAspectRatio();
            s.name = it->getName();
            _additionalFormats.push_back(s);
        }


        ar & ::boost::serialization::make_nvp("Timeline_current_time", _timelineCurrent);
        if (version < PROJECT_SERIALIZATION_REMOVES_TIMELINE_BOUNDS) {
            NATRON_NAMESPACE::SequenceTime left, right;
            ar & ::boost::serialization::make_nvp("Timeline_left_bound", left);
            ar & ::boost::serialization::make_nvp("Timeline_right_bound", right);
        }
        if (version < PROJECT_SERIALIZATION_REMOVES_NODE_COUNTERS) {
            std::map<std::string, int> _nodeCounters;
            ar & ::boost::serialization::make_nvp("NodeCounters", _nodeCounters);
        }
        ar & ::boost::serialization::make_nvp("CreationDate", _creationDate);

    } // load
    
    BOOST_SERIALIZATION_SPLIT_MEMBER()

#endif // #ifdef NATRON_BOOST_SERIALIZATION_COMPAT
};

SERIALIZATION_NAMESPACE_EXIT;

#ifdef NATRON_BOOST_SERIALIZATION_COMPAT
BOOST_CLASS_VERSION(SERIALIZATION_NAMESPACE::ProjectSerialization, PROJECT_SERIALIZATION_VERSION)
#endif

#endif // PROJECTSERIALIZATION_H
