/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2020 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#include "Global/Macros.h"

#include <stdexcept>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/version.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
GCC_DIAG_OFF(unused-parameter)
// /opt/local/include/boost/serialization/smart_cast.hpp:254:25: warning: unused parameter 'u' [-Wunused-parameter]
#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/map.hpp>
// /usr/local/include/boost/serialization/shared_ptr.hpp:112:5: warning: unused typedef 'boost_static_assert_typedef_112' [-Wunused-local-typedef]
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/scoped_ptr.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/version.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
GCC_DIAG_ON(unused-parameter)
#endif

#include "Global/GitVersion.h"
#include "Global/GlobalDefines.h"

#include "Engine/AppInstance.h"
#include "Engine/KnobSerialization.h"
#include "Engine/MemoryInfo.h" // isApplication32Bits
#include "Engine/Node.h"
#include "Engine/NodeSerialization.h"
#include "Engine/NodeGroupSerialization.h"
#include "Engine/ProjectPrivate.h"
#include "Engine/Project.h"
#include "Engine/TimeLine.h"

#include "Engine/EngineFwd.h"

#define PROJECT_SERIALIZATION_INTRODUCES_NATRON_VERSION 2
#define PROJECT_SERIALIZATION_REMOVES_NODE_COUNTERS 3
#define PROJECT_SERIALIZATION_REMOVES_TIMELINE_BOUNDS 4
#define PROJECT_SERIALIZATION_INTRODUCES_GROUPS 5
#define PROJECT_SERIALIZATION_CHANGE_VERSION_SERIALIZATION 6
#define PROJECT_SERIALIZATION_VERSION PROJECT_SERIALIZATION_CHANGE_VERSION_SERIALIZATION

NATRON_NAMESPACE_ENTER

class ProjectBeingLoadedInfo
{
public:
    int vMajor,vMinor,vRev;
    std::string gitBranch,gitCommit;
    std::string osStr;
    int bits;

    ProjectBeingLoadedInfo()
    : vMajor(-1)
    , vMinor(-1)
    , vRev(-1)
    , gitBranch()
    , gitCommit()
    , osStr()
    , bits(-1)
    {

    }
};

class ProjectSerialization
{
    NodeCollectionSerialization _nodes;
    std::list<Format> _additionalFormats;
    std::list<KnobSerializationPtr> _projectKnobs;
    SequenceTime _timelineCurrent;
    qint64 _creationDate;
    AppInstanceWPtr _app;
    unsigned int _version;

    ProjectBeingLoadedInfo _projectLoadedInfo;

public:

    ProjectSerialization(const AppInstancePtr& app)
        : _timelineCurrent(0)
        , _creationDate(0)
        , _app(app)
        , _version(0)
    {
    }

    ~ProjectSerialization()
    {
    }

    unsigned int getVersion() const
    {
        return _version;
    }

    const ProjectBeingLoadedInfo& getProjectBeingLoadedInfo() const
    {
        return _projectLoadedInfo;
    }

    void initialize(const Project* project);

    SequenceTime getCurrentTime() const
    {
        return _timelineCurrent;
    }

    const std::list<KnobSerializationPtr  > & getProjectKnobsValues() const
    {
        return _projectKnobs;
    }

    const std::list<Format> & getAdditionalFormats() const
    {
        return _additionalFormats;
    }

    const NodeCollectionSerialization & getNodesSerialization() const
    {
        return _nodes;
    }

    qint64 getCreationDate() const
    {
        return _creationDate;
    }

    friend class ::boost::serialization::access;
    template<class Archive>
    void save(Archive & ar,
              const unsigned int /*version*/) const
    {
        /*std::string natronVersion(NATRON_APPLICATION_NAME);

        natronVersion.append(" v" NATRON_VERSION_STRING);
        natronVersion.append(" from git branch " GIT_BRANCH);
        natronVersion.append(" commit " GIT_COMMIT);
        natronVersion.append("  for ");
#ifdef __NATRON_WIN32__
        natronVersion.append("  Windows ");
#elif defined(__NATRON_OSX__)
        natronVersion.append("  MacOSX ");
#elif defined(__NATRON_LINUX__)
        natronVersion.append("  Linux ");
#endif
        natronVersion.append(isApplication32Bits() ? "32bit" : "64bit");
        ar & ::boost::serialization::make_nvp("NatronVersion", natronVersion);*/
        int vMajor = NATRON_VERSION_MAJOR;
        int vMinor = NATRON_VERSION_MINOR;
        int vRev = NATRON_VERSION_REVISION;
        ar & ::boost::serialization::make_nvp("VersionMajor", vMajor);
        ar & ::boost::serialization::make_nvp("VersionMinor", vMinor);
        ar & ::boost::serialization::make_nvp("VersionRev", vRev);
        std::string gitBranchStr(GIT_BRANCH);
        ar & ::boost::serialization::make_nvp("GitBranch", gitBranchStr);

        std::string gitCommitStr(GIT_COMMIT);
        ar & ::boost::serialization::make_nvp("GitCommit", gitCommitStr);

        std::string osString;
#ifdef __NATRON_WIN32__
        osString.append("Windows");
#elif defined(__NATRON_OSX__)
        osString.append("MacOSX");
#elif defined(__NATRON_LINUX__)
        osString.append("Linux");
#endif
        ar & ::boost::serialization::make_nvp("OS", osString);

        int bits = isApplication32Bits() ? 32 : 64;
        ar & ::boost::serialization::make_nvp("Bits", bits);

        ar & ::boost::serialization::make_nvp("NodesCollection", _nodes);
        int knobsCount = _projectKnobs.size();
        ar & ::boost::serialization::make_nvp("ProjectKnobsCount", knobsCount);
        for (std::list<KnobSerializationPtr>::const_iterator it = _projectKnobs.begin();
             it != _projectKnobs.end();
             ++it) {
            ar & ::boost::serialization::make_nvp( "item", *(*it) );
        }
        ar & ::boost::serialization::make_nvp("AdditionalFormats", _additionalFormats);
        ar & ::boost::serialization::make_nvp("Timeline_current_time", _timelineCurrent);
        ar & ::boost::serialization::make_nvp("CreationDate", _creationDate);
    }

    template<class Archive>
    void load(Archive & ar,
              const unsigned int version)
    {
        _version = version;
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
                        _projectLoadedInfo.vMajor = splits[0].toInt();
                        _projectLoadedInfo.vMinor = splits[1].toInt();
                        _projectLoadedInfo.vRev = splits[2].toInt();
                        AppInstancePtr app = _app.lock();
                        assert(app);
                        app->setProjectBeingLoadedInfo(_projectLoadedInfo);
                        if (NATRON_VERSION_ENCODE(_projectLoadedInfo.vMajor, _projectLoadedInfo.vMinor, _projectLoadedInfo.vRev) > NATRON_VERSION_ENCODED) {
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
            AppInstancePtr app = _app.lock();
            assert(app);
            app->setProjectBeingLoadedInfo(_projectLoadedInfo);

        }

        if (version < PROJECT_SERIALIZATION_INTRODUCES_GROUPS) {
            int nodesCount;
            ar & ::boost::serialization::make_nvp("NodesCount", nodesCount);
            for (int i = 0; i < nodesCount; ++i) {
                NodeSerializationPtr ns = boost::make_shared<NodeSerialization>();
                ar & ::boost::serialization::make_nvp("item", *ns);
                _nodes.addNodeSerialization(ns);
            }
        } else {
            ar & ::boost::serialization::make_nvp("NodesCollection", _nodes);
        }

        int knobsCount;
        ar & ::boost::serialization::make_nvp("ProjectKnobsCount", knobsCount);

        for (int i = 0; i < knobsCount; ++i) {
            KnobSerializationPtr ks = boost::make_shared<KnobSerialization>();
            ar & ::boost::serialization::make_nvp("item", *ks);
            _projectKnobs.push_back(ks);
        }

        ar & ::boost::serialization::make_nvp("AdditionalFormats", _additionalFormats);
        ar & ::boost::serialization::make_nvp("Timeline_current_time", _timelineCurrent);
        if (version < PROJECT_SERIALIZATION_REMOVES_TIMELINE_BOUNDS) {
            SequenceTime left, right;
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
};

NATRON_NAMESPACE_EXIT

BOOST_CLASS_VERSION(NATRON_NAMESPACE::ProjectSerialization, PROJECT_SERIALIZATION_VERSION)

#endif // PROJECTSERIALIZATION_H
