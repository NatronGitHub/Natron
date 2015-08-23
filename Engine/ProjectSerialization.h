//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */


#ifndef PROJECTSERIALIZATION_H
#define PROJECTSERIALIZATION_H

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "Global/Macros.h"
#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
CLANG_DIAG_OFF(unused-local-typedef)
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
CLANG_DIAG_ON(unused-local-typedef)
GCC_DIAG_ON(unused-parameter)
#endif

#include "Global/GitVersion.h"
#include "Global/GlobalDefines.h"
#include "Global/MemoryInfo.h"
#include "Engine/ProjectPrivate.h"
#include "Engine/Project.h"
#include "Engine/TimeLine.h"
#include "Engine/Node.h"
#include "Engine/NodeSerialization.h"
#include "Engine/NodeGroupSerialization.h"
#include "Engine/KnobSerialization.h"

#define PROJECT_SERIALIZATION_INTRODUCES_NATRON_VERSION 2
#define PROJECT_SERIALIZATION_REMOVES_NODE_COUNTERS 3
#define PROJECT_SERIALIZATION_REMOVES_TIMELINE_BOUNDS 4
#define PROJECT_SERIALIZATION_INTRODUCES_GROUPS 5
#define PROJECT_SERIALIZATION_VERSION PROJECT_SERIALIZATION_INTRODUCES_GROUPS

class AppInstance;
class ProjectSerialization
{
    NodeCollectionSerialization _nodes;
    std::list<Format> _additionalFormats;
    std::list< boost::shared_ptr<KnobSerialization> > _projectKnobs;
    SequenceTime _timelineCurrent;
    qint64 _creationDate;
    AppInstance* _app;
    unsigned int _version;
    
public:

    ProjectSerialization(AppInstance* app)
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
    
    void initialize(const Natron::Project* project);

    SequenceTime getCurrentTime() const
    {
        return _timelineCurrent;
    }

    const std::list< boost::shared_ptr<KnobSerialization>  > & getProjectKnobsValues() const
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


    friend class boost::serialization::access;
    template<class Archive>
    void save(Archive & ar,
              const unsigned int /*version*/) const
    {
        
        std::string natronVersion(NATRON_APPLICATION_NAME);
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
        ar & boost::serialization::make_nvp("NatronVersion",natronVersion);
        
        ar & boost::serialization::make_nvp("NodesCollection",_nodes);
        
        int knobsCount = _projectKnobs.size();
        ar & boost::serialization::make_nvp("ProjectKnobsCount",knobsCount);
        for (std::list< boost::shared_ptr<KnobSerialization> >::const_iterator it = _projectKnobs.begin();
             it != _projectKnobs.end();
             ++it) {
            ar & boost::serialization::make_nvp( "item",*(*it) );
        }
        ar & boost::serialization::make_nvp("AdditionalFormats", _additionalFormats);
        ar & boost::serialization::make_nvp("Timeline_current_time", _timelineCurrent);
        ar & boost::serialization::make_nvp("CreationDate", _creationDate);
    }

    template<class Archive>
    void load(Archive & ar,
              const unsigned int version)
    {
        _version = version;
        if (version >= PROJECT_SERIALIZATION_INTRODUCES_NATRON_VERSION) {
            std::string natronVersion;
            ar & boost::serialization::make_nvp("NatronVersion",natronVersion);
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
                    QStringList splits = QString(vStr.c_str()).split('.');
                    if (splits.size() == 3) {
                        int major,minor,rev;
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
        assert(_app);
        Q_UNUSED(_app);
        
        if (version < PROJECT_SERIALIZATION_INTRODUCES_GROUPS) {
            int nodesCount;
            ar & boost::serialization::make_nvp("NodesCount",nodesCount);
            for (int i = 0; i < nodesCount; ++i) {
                boost::shared_ptr<NodeSerialization> ns(new NodeSerialization);
                ar & boost::serialization::make_nvp("item",*ns);
                _nodes.addNodeSerialization(ns);
            }
        } else {
            ar & boost::serialization::make_nvp("NodesCollection",_nodes);
        }
        
        int knobsCount;
        ar & boost::serialization::make_nvp("ProjectKnobsCount",knobsCount);
        
        for (int i = 0; i < knobsCount; ++i) {
            boost::shared_ptr<KnobSerialization> ks(new KnobSerialization);
            ar & boost::serialization::make_nvp("item",*ks);
            _projectKnobs.push_back(ks);
        }

        ar & boost::serialization::make_nvp("AdditionalFormats", _additionalFormats);
        ar & boost::serialization::make_nvp("Timeline_current_time", _timelineCurrent);
        if (version < PROJECT_SERIALIZATION_REMOVES_TIMELINE_BOUNDS) {
            SequenceTime left,right;
            ar & boost::serialization::make_nvp("Timeline_left_bound", left);
            ar & boost::serialization::make_nvp("Timeline_right_bound", right);
        }
        if (version < PROJECT_SERIALIZATION_REMOVES_NODE_COUNTERS) {
            std::map<std::string,int> _nodeCounters;
            ar & boost::serialization::make_nvp("NodeCounters", _nodeCounters);
        }
        ar & boost::serialization::make_nvp("CreationDate", _creationDate);
    }

    BOOST_SERIALIZATION_SPLIT_MEMBER()
};

BOOST_CLASS_VERSION(ProjectSerialization,PROJECT_SERIALIZATION_VERSION)

#endif // PROJECTSERIALIZATION_H
