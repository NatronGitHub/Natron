/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2021 The Natron developers
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

#ifndef NODESERIALIZATION_H
#define NODESERIALIZATION_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <string>
#include <stdexcept>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
GCC_DIAG_OFF(unused-parameter)
GCC_DIAG_OFF(sign-compare)
// /opt/local/include/boost/serialization/smart_cast.hpp:254:25: warning: unused parameter 'u' [-Wunused-parameter]
#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/version.hpp>
GCC_DIAG_ON(sign-compare)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
GCC_DIAG_ON(unused-parameter)
#endif

#include "Engine/KnobSerialization.h"
#include "Engine/TrackerSerialization.h"
#include "Engine/RotoContextSerialization.h"
#include "Engine/ImageParamsSerialization.h"
#include "Engine/AppManager.h"
#include "Engine/EngineFwd.h"


#define NODE_SERIALIZATION_V_INTRODUCES_ROTO 2
#define NODE_SERIALIZATION_INTRODUCES_MULTI_INSTANCE 3
#define NODE_SERIALIZATION_INTRODUCES_USER_KNOBS 4
#define NODE_SERIALIZATION_INTRODUCES_GROUPS 5
#define NODE_SERIALIZATION_EMBEDS_MULTI_INSTANCE_CHILDREN 6
#define NODE_SERIALIZATION_INTRODUCES_SCRIPT_NAME 7
#define NODE_SERIALIZATION_INTRODUCES_PYTHON_MODULE 8
#define NODE_SERIALIZATION_CHANGE_INPUTS_SERIALIZATION 9
#define NODE_SERIALIZATION_INTRODUCES_USER_COMPONENTS 10
#define NODE_SERIALIZATION_INTRODUCES_PYTHON_MODULE_VERSION 11
#define NODE_SERIALIZATION_INTRODUCES_CACHE_ID 12
#define NODE_SERIALIZATION_SERIALIZE_PYTHON_MODULE_ALWAYS 13
#define NODE_SERIALIZATION_SERIALIZE_PAGE_INDEX 14
#define NODE_SERIALIZATION_INTRODUCES_TRACKER_CONTEXT 15
#define NODE_SERIALIZATION_CURRENT_VERSION NODE_SERIALIZATION_INTRODUCES_TRACKER_CONTEXT

NATRON_NAMESPACE_ENTER

class NodeSerialization
{
public:

    typedef std::list<KnobSerializationPtr> KnobValues;

    ///Used to serialize
    explicit NodeSerialization(const NodePtr & n,
                      bool serializeInputs = true);

    ////Used to deserialize
    NodeSerialization()
        : _isNull(true)
        , _nbKnobs(0)
        , _knobsValues()
        , _knobsAge(0)
        , _nodeLabel()
        , _nodeScriptName()
        , _cacheID()
        , _pluginID()
        , _pluginMajorVersion(-1)
        , _pluginMinorVersion(-1)
        , _hasRotoContext(false)
        , _hasTrackerContext(false)
        , _node()
        , _pythonModuleVersion(0)
    {
    }

    ~NodeSerialization()
    {
        _knobsValues.clear(); _inputs.clear();
    }

    const KnobValues & getKnobsValues() const
    {
        return _knobsValues;
    }

    const std::string & getNodeLabel() const
    {
        return _nodeLabel;
    }

    void setNodeLabel(const std::string& s)
    {
        _nodeLabel = s;
    }

    const std::string & getNodeScriptName() const
    {
        return _nodeScriptName;
    }

    void setNodeScriptName(const std::string &s)
    {
        _nodeScriptName = s;
    }

    const std::string & getCacheID() const
    {
        return _cacheID;
    }

    const std::string & getPluginID() const
    {
        return _pluginID;
    }

    const std::string& getPythonModule() const
    {
        return _pythonModule;
    }

    unsigned int getPythonModuleVersion() const
    {
        return _pythonModuleVersion;
    }

    const std::vector<std::string> & getOldInputs() const
    {
        return _oldInputs;
    }

    const std::map<std::string, std::string>& getInputs() const
    {
        return _inputs;
    }

    int getPluginMajorVersion() const
    {
        return _pluginMajorVersion;
    }

    int getPluginMinorVersion() const
    {
        return _pluginMinorVersion;
    }

    bool isNull() const
    {
        return _isNull;
    }

    U64 getKnobsAge() const
    {
        return _knobsAge;
    }

    const std::string & getMasterNodeName() const
    {
        return _masterNodeName;
    }

    NodePtr getNode() const
    {
        return _node;
    }

    bool hasRotoContext() const
    {
        return _hasRotoContext;
    }

    const RotoContextSerialization & getRotoContext() const
    {
        return _rotoContext;
    }

    bool hasTrackerContext() const
    {
        return _hasTrackerContext;
    }

    const TrackerContextSerialization& getTrackerContext() const
    {
        return _trackerContext;
    }

    const std::string & getMultiInstanceParentName() const
    {
        return _multiInstanceParentName;
    }

    const std::list<std::string>& getPagesOrdered() const
    {
        return _pagesIndexes;
    }

    const std::list<GroupKnobSerializationPtr>& getUserPages() const
    {
        return _userPages;
    }

    const std::list<NodeSerializationPtr>& getNodesCollection() const
    {
        return _children;
    }

    const std::list<ImagePlaneDesc>& getUserCreatedComponents() const
    {
        return _userComponents;
    }

private:

    bool _isNull;
    int _nbKnobs;
    KnobValues _knobsValues;
    U64 _knobsAge;
    std::string _nodeLabel, _nodeScriptName, _cacheID;
    std::string _pluginID;
    int _pluginMajorVersion;
    int _pluginMinorVersion;
    std::string _masterNodeName;
    std::map<std::string, std::string> _inputs;
    std::vector<std::string> _oldInputs;
    bool _hasRotoContext;
    RotoContextSerialization _rotoContext;
    bool _hasTrackerContext;
    TrackerContextSerialization _trackerContext;
    NodePtr _node;
    std::string _multiInstanceParentName;
    std::list<GroupKnobSerializationPtr> _userPages;
    std::list<std::string> _pagesIndexes;

    ///If this node is a group or a multi-instance, this is the children
    std::list<NodeSerializationPtr> _children;
    std::string _pythonModule;
    unsigned int _pythonModuleVersion;
    std::list<ImagePlaneDesc> _userComponents;

    friend class ::boost::serialization::access;
    template<class Archive>
    void save(Archive & ar,
              const unsigned int /*version*/) const
    {
        ar & ::boost::serialization::make_nvp("Plugin_label", _nodeLabel);
        ar & ::boost::serialization::make_nvp("Plugin_script_name", _nodeScriptName);
        ar & ::boost::serialization::make_nvp("Plugin_id", _pluginID);
        ar & ::boost::serialization::make_nvp("PythonModule", _pythonModule);
        ar & ::boost::serialization::make_nvp("PythonModuleVersion", _pythonModuleVersion);
        ar & ::boost::serialization::make_nvp("Plugin_major_version", _pluginMajorVersion);
        ar & ::boost::serialization::make_nvp("Plugin_minor_version", _pluginMinorVersion);
        ar & ::boost::serialization::make_nvp("KnobsCount", _nbKnobs);

        for (KnobValues::const_iterator it = _knobsValues.begin(); it != _knobsValues.end(); ++it) {
            ar & ::boost::serialization::make_nvp( "item", *(*it) );
        }
        ar & ::boost::serialization::make_nvp("Inputs_map", _inputs);
        ar & ::boost::serialization::make_nvp("KnobsAge", _knobsAge);
        ar & ::boost::serialization::make_nvp("MasterNode", _masterNodeName);
        ar & ::boost::serialization::make_nvp("HasRotoContext", _hasRotoContext);
        if (_hasRotoContext) {
            ar & ::boost::serialization::make_nvp("RotoContext", _rotoContext);
        }

        ar & ::boost::serialization::make_nvp("HasTrackerContext", _hasTrackerContext);
        if (_hasTrackerContext) {
            ar & ::boost::serialization::make_nvp("TrackerContext", _trackerContext);
        }

        ar & ::boost::serialization::make_nvp("MultiInstanceParent", _multiInstanceParentName);
        int userPagesCount = (int)_userPages.size();
        ar & ::boost::serialization::make_nvp("UserPagesCount", userPagesCount);
        for (std::list<GroupKnobSerializationPtr>::const_iterator it = _userPages.begin(); it != _userPages.end(); ++it) {
            ar & ::boost::serialization::make_nvp("item", **it);
        }

        int pageSizes = (int)_pagesIndexes.size();
        ar & ::boost::serialization::make_nvp("PagesCount", pageSizes);
        for (std::list<std::string>::const_iterator it = _pagesIndexes.begin(); it != _pagesIndexes.end(); ++it) {
            ar & ::boost::serialization::make_nvp("name", *it);
        }

        int nodesCount = (int)_children.size();
        ar & ::boost::serialization::make_nvp("Children", nodesCount);

        for (std::list<NodeSerializationPtr>::const_iterator it = _children.begin();
             it != _children.end();
             ++it) {
            ar & ::boost::serialization::make_nvp("item", **it);
        }

        ar & ::boost::serialization::make_nvp("UserComponents", _userComponents);
        ar & ::boost::serialization::make_nvp("CacheID", _cacheID);
    } // save

    template<class Archive>
    void load(Archive & ar,
              const unsigned int version)
    {
        if (version > NODE_SERIALIZATION_CURRENT_VERSION) {
            throw std::invalid_argument("The project you're trying to load contains data produced by a more recent "
                                        "version of Natron, which makes it unreadable");
        }

        ar & ::boost::serialization::make_nvp("Plugin_label", _nodeLabel);
        if (version >= NODE_SERIALIZATION_INTRODUCES_SCRIPT_NAME) {
            ar & ::boost::serialization::make_nvp("Plugin_script_name", _nodeScriptName);
        } else {
            _nodeScriptName = NATRON_PYTHON_NAMESPACE::makeNameScriptFriendly(_nodeLabel);
        }
        ar & ::boost::serialization::make_nvp("Plugin_id", _pluginID);

        if (version >= NODE_SERIALIZATION_INTRODUCES_PYTHON_MODULE) {
            if ( (version >= NODE_SERIALIZATION_SERIALIZE_PYTHON_MODULE_ALWAYS) || (_pluginID == PLUGINID_NATRON_GROUP) ) {
                ar & ::boost::serialization::make_nvp("PythonModule", _pythonModule);
                if (version >= NODE_SERIALIZATION_INTRODUCES_PYTHON_MODULE_VERSION) {
                    ar & ::boost::serialization::make_nvp("PythonModuleVersion", _pythonModuleVersion);
                } else {
                    _pythonModuleVersion = 1;
                }
            }
        }

        ar & ::boost::serialization::make_nvp("Plugin_major_version", _pluginMajorVersion);
        ar & ::boost::serialization::make_nvp("Plugin_minor_version", _pluginMinorVersion);
        ar & ::boost::serialization::make_nvp("KnobsCount", _nbKnobs);
        for (int i = 0; i < _nbKnobs; ++i) {
            KnobSerializationPtr ks = boost::make_shared<KnobSerialization>();
            ar & ::boost::serialization::make_nvp("item", *ks);
            _knobsValues.push_back(ks);
        }

        if (version < NODE_SERIALIZATION_CHANGE_INPUTS_SERIALIZATION) {
            ar & ::boost::serialization::make_nvp("Inputs_map", _oldInputs);
            if (version < NODE_SERIALIZATION_INTRODUCES_SCRIPT_NAME) {
                for (U32 i = 0; i < _oldInputs.size(); ++i) {
                    _oldInputs[i] = NATRON_PYTHON_NAMESPACE::makeNameScriptFriendly(_oldInputs[i]);
                }
            }
        } else {
            ar & ::boost::serialization::make_nvp("Inputs_map", _inputs);
        }

        ar & ::boost::serialization::make_nvp("KnobsAge", _knobsAge);
        ar & ::boost::serialization::make_nvp("MasterNode", _masterNodeName);
        if (version < NODE_SERIALIZATION_INTRODUCES_SCRIPT_NAME) {
            _masterNodeName = NATRON_PYTHON_NAMESPACE::makeNameScriptFriendly(_masterNodeName);
        }
        _isNull = false;

        if (version >= NODE_SERIALIZATION_V_INTRODUCES_ROTO) {
            ar & ::boost::serialization::make_nvp("HasRotoContext", _hasRotoContext);
            if (_hasRotoContext) {
                ar & ::boost::serialization::make_nvp("RotoContext", _rotoContext);
            }

            if (version >= NODE_SERIALIZATION_INTRODUCES_TRACKER_CONTEXT) {
                ar & boost::serialization::make_nvp("HasTrackerContext", _hasTrackerContext);
                if (_hasTrackerContext) {
                    ar & boost::serialization::make_nvp("TrackerContext", _trackerContext);
                }
            }
        }
        if (version >= NODE_SERIALIZATION_INTRODUCES_MULTI_INSTANCE) {
            ar & ::boost::serialization::make_nvp("MultiInstanceParent", _multiInstanceParentName);
            if (version < NODE_SERIALIZATION_INTRODUCES_SCRIPT_NAME) {
                _multiInstanceParentName = NATRON_PYTHON_NAMESPACE::makeNameScriptFriendly(_multiInstanceParentName);
            }
        }

        if (version >= NODE_SERIALIZATION_INTRODUCES_USER_KNOBS) {
            int userPagesCount;
            ar & ::boost::serialization::make_nvp("UserPagesCount", userPagesCount);
            for (int i = 0; i < userPagesCount; ++i) {
                GroupKnobSerializationPtr s = boost::make_shared<GroupKnobSerialization>();
                ar & ::boost::serialization::make_nvp("item", *s);
                _userPages.push_back(s);
            }
            if (version >= NODE_SERIALIZATION_SERIALIZE_PAGE_INDEX) {
                int count;
                ar & ::boost::serialization::make_nvp("PagesCount", count);
                for (int i = 0; i < count; ++i) {
                    std::string name;
                    ar & ::boost::serialization::make_nvp("name", name);
                    _pagesIndexes.push_back(name);
                }
            } 
        }

        if (version >= NODE_SERIALIZATION_INTRODUCES_GROUPS) {
            int nodesCount;
            ar & ::boost::serialization::make_nvp("Children", nodesCount);

            for (int i = 0; i < nodesCount; ++i) {
                NodeSerializationPtr s = boost::make_shared<NodeSerialization>();
                ar & ::boost::serialization::make_nvp("item", *s);
                _children.push_back(s);
            }
        }
        if (version >= NODE_SERIALIZATION_INTRODUCES_USER_COMPONENTS) {
            ar & ::boost::serialization::make_nvp("UserComponents", _userComponents);
        }
        if (version >= NODE_SERIALIZATION_INTRODUCES_CACHE_ID) {
            ar & ::boost::serialization::make_nvp("CacheID", _cacheID);
        }
    } // load

    BOOST_SERIALIZATION_SPLIT_MEMBER()
};

NATRON_NAMESPACE_EXIT

BOOST_CLASS_VERSION(NATRON_NAMESPACE::NodeSerialization, NODE_SERIALIZATION_CURRENT_VERSION)


#endif // NODESERIALIZATION_H
