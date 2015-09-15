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

#ifndef NODESERIALIZATION_H
#define NODESERIALIZATION_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <string>
#include <stdexcept>

#include "Global/Macros.h"
#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
GCC_DIAG_OFF(unused-parameter)
// /opt/local/include/boost/serialization/smart_cast.hpp:254:25: warning: unused parameter 'u' [-Wunused-parameter]
#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
GCC_DIAG_OFF(sign-compare)
#include <boost/serialization/vector.hpp>
GCC_DIAG_ON(sign-compare)
#include <boost/serialization/list.hpp>
#include <boost/serialization/version.hpp>
#endif
#include "Engine/KnobSerialization.h"
#include "Engine/RotoContextSerialization.h"
#include "Engine/ImageParamsSerialization.h"
#include "Engine/AppManager.h"


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
#define NODE_SERIALIZATION_CURRENT_VERSION NODE_SERIALIZATION_INTRODUCES_PYTHON_MODULE_VERSION

namespace Natron {
class Node;
}
class AppInstance;


class NodeSerialization
{
public:

    typedef std::list< boost::shared_ptr<KnobSerialization> > KnobValues;

    ///Used to serialize
    NodeSerialization(const boost::shared_ptr<Natron::Node> & n,bool serializeInputs = true);
    
    ////Used to deserialize
    NodeSerialization()
    : _isNull(true)
    , _nbKnobs(0)
    , _knobsValues()
    , _knobsAge(0)
    , _nodeLabel()
    , _nodeScriptName()
    , _pluginID()
    , _pluginMajorVersion(-1)
    , _pluginMinorVersion(-1)
    , _hasRotoContext(false)
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
    
    const std::string & getNodeScriptName() const
    {
        return _nodeScriptName;
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
    
    const std::map<std::string,std::string>& getInputs() const
    {
        return _inputs;
    }

    void switchInput(const std::string& oldInputName,const std::string& newInputName) {
        for (std::map<std::string,std::string>::iterator it = _inputs.begin(); it != _inputs.end(); ++it) {
            if (it->second == oldInputName) {
                it->second = newInputName;
            }
        }
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

    boost::shared_ptr<Natron::Node> getNode() const
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

    const std::string & getMultiInstanceParentName() const
    {
        return _multiInstanceParentName;
    }

    const std::list<boost::shared_ptr<GroupKnobSerialization> >& getUserPages() const
    {
        return _userPages;
    }
    
    const std::list< boost::shared_ptr<NodeSerialization> >& getNodesCollection() const
    {
        return _children;
    }
    
    const std::list<Natron::ImageComponents>& getUserComponents() const
    {
        return _userComponents;
    }
    
private:

    bool _isNull;
    int _nbKnobs;
    KnobValues _knobsValues;
    U64 _knobsAge;
    std::string _nodeLabel,_nodeScriptName;
    std::string _pluginID;
    int _pluginMajorVersion;
    int _pluginMinorVersion;
    std::string _masterNodeName;
    std::map<std::string,std::string> _inputs;
    std::vector<std::string> _oldInputs;
    bool _hasRotoContext;
    RotoContextSerialization _rotoContext;
    boost::shared_ptr<Natron::Node> _node;
    std::string _multiInstanceParentName;
    std::list<boost::shared_ptr<GroupKnobSerialization> > _userPages;
    
    ///If this node is a group or a multi-instance, this is the children
    std::list< boost::shared_ptr<NodeSerialization> > _children;
    
    std::string _pythonModule;
    unsigned int _pythonModuleVersion;
    
    std::list<Natron::ImageComponents> _userComponents;
    
    friend class boost::serialization::access;
    template<class Archive>
    void save(Archive & ar,
              const unsigned int /*version*/) const
    {
        ar & boost::serialization::make_nvp("Plugin_label",_nodeLabel);
        ar & boost::serialization::make_nvp("Plugin_script_name",_nodeScriptName);
        ar & boost::serialization::make_nvp("Plugin_id",_pluginID);
        if (_pluginID == PLUGINID_NATRON_GROUP) {
            ar & boost::serialization::make_nvp("PythonModule",_pythonModule);
            ar & boost::serialization::make_nvp("PythonModuleVersion",_pythonModuleVersion);
        }
        ar & boost::serialization::make_nvp("Plugin_major_version",_pluginMajorVersion);
        ar & boost::serialization::make_nvp("Plugin_minor_version",_pluginMinorVersion);
        ar & boost::serialization::make_nvp("KnobsCount", _nbKnobs);
        
        for (KnobValues::const_iterator it = _knobsValues.begin(); it != _knobsValues.end(); ++it) {
            ar & boost::serialization::make_nvp( "item",*(*it) );
        }
        ar & boost::serialization::make_nvp("Inputs_map",_inputs);
        ar & boost::serialization::make_nvp("KnobsAge",_knobsAge);
        ar & boost::serialization::make_nvp("MasterNode",_masterNodeName);
        ar & boost::serialization::make_nvp("HasRotoContext",_hasRotoContext);
        if (_hasRotoContext) {
            ar & boost::serialization::make_nvp("RotoContext",_rotoContext);
        }
        
        ar & boost::serialization::make_nvp("MultiInstanceParent",_multiInstanceParentName);
        
        int userPagesCount = (int)_userPages.size();
        ar & boost::serialization::make_nvp("UserPagesCount",userPagesCount);
        for (std::list<boost::shared_ptr<GroupKnobSerialization> >::const_iterator it = _userPages.begin(); it != _userPages.end() ; ++it ) {
            ar & boost::serialization::make_nvp("item",**it);
        }
       
        int nodesCount = (int)_children.size();
        ar & boost::serialization::make_nvp("Children",nodesCount);
        
        for (std::list< boost::shared_ptr<NodeSerialization> >::const_iterator it = _children.begin();
             it != _children.end();
             ++it) {
            ar & boost::serialization::make_nvp("item",**it);
        }
        
        ar & boost::serialization::make_nvp("UserComponents",_userComponents);
        
    }
    
    template<class Archive>
    void load(Archive & ar,
              const unsigned int version)
    {
        if (version > NODE_SERIALIZATION_CURRENT_VERSION) {
            throw std::invalid_argument("The project you're trying to load contains data produced by a more recent "
                                        "version of Natron, which makes it unreadable");
        }
        
        ar & boost::serialization::make_nvp("Plugin_label",_nodeLabel);
        if (version >= NODE_SERIALIZATION_INTRODUCES_SCRIPT_NAME) {
            ar & boost::serialization::make_nvp("Plugin_script_name",_nodeScriptName);
        } else {
            _nodeScriptName = Natron::makeNameScriptFriendly(_nodeLabel);
        }
        ar & boost::serialization::make_nvp("Plugin_id",_pluginID);
        
        if (version >= NODE_SERIALIZATION_INTRODUCES_PYTHON_MODULE) {
            if (_pluginID == PLUGINID_NATRON_GROUP) {
                ar & boost::serialization::make_nvp("PythonModule",_pythonModule);
                if (version >= NODE_SERIALIZATION_INTRODUCES_PYTHON_MODULE_VERSION) {
                    ar & boost::serialization::make_nvp("PythonModuleVersion",_pythonModuleVersion);
                } else {
                    _pythonModuleVersion = 1;
                }
            }
        }
        
        ar & boost::serialization::make_nvp("Plugin_major_version",_pluginMajorVersion);
        ar & boost::serialization::make_nvp("Plugin_minor_version",_pluginMinorVersion);
        ar & boost::serialization::make_nvp("KnobsCount", _nbKnobs);
        for (int i = 0; i < _nbKnobs; ++i) {
            boost::shared_ptr<KnobSerialization> ks(new KnobSerialization);
            ar & boost::serialization::make_nvp("item",*ks);
            _knobsValues.push_back(ks);
        }
        
        if (version < NODE_SERIALIZATION_CHANGE_INPUTS_SERIALIZATION) {
            ar & boost::serialization::make_nvp("Inputs_map",_oldInputs);
            if (version < NODE_SERIALIZATION_INTRODUCES_SCRIPT_NAME) {
                for (U32 i = 0; i < _oldInputs.size(); ++i) {
                    _oldInputs[i] = Natron::makeNameScriptFriendly(_oldInputs[i]);
                }
            }
        } else {
            ar & boost::serialization::make_nvp("Inputs_map",_inputs);
        }
        
        ar & boost::serialization::make_nvp("KnobsAge",_knobsAge);
        ar & boost::serialization::make_nvp("MasterNode",_masterNodeName);
        if (version < NODE_SERIALIZATION_INTRODUCES_SCRIPT_NAME) {
            _masterNodeName = Natron::makeNameScriptFriendly(_masterNodeName);
        }
        _isNull = false;
        
        if (version >= NODE_SERIALIZATION_V_INTRODUCES_ROTO) {
            ar & boost::serialization::make_nvp("HasRotoContext",_hasRotoContext);
            if (_hasRotoContext) {
                ar & boost::serialization::make_nvp("RotoContext",_rotoContext);
            }
        }
        if (version >= NODE_SERIALIZATION_INTRODUCES_MULTI_INSTANCE) {
            ar & boost::serialization::make_nvp("MultiInstanceParent",_multiInstanceParentName);
            if (version < NODE_SERIALIZATION_INTRODUCES_SCRIPT_NAME) {
                 _multiInstanceParentName = Natron::makeNameScriptFriendly(_multiInstanceParentName);
            }
        }
        
        if (version >= NODE_SERIALIZATION_INTRODUCES_USER_KNOBS) {
            int userPagesCount;
            ar & boost::serialization::make_nvp("UserPagesCount",userPagesCount);
            for (int i = 0; i < userPagesCount; ++i) {
                boost::shared_ptr<GroupKnobSerialization> s(new GroupKnobSerialization());
                ar & boost::serialization::make_nvp("item",*s);
                _userPages.push_back(s);
            }
        }
        
        if (version >= NODE_SERIALIZATION_INTRODUCES_GROUPS) {
            int nodesCount ;
            ar & boost::serialization::make_nvp("Children",nodesCount);
            
            for (int i = 0; i < nodesCount; ++i) {
                boost::shared_ptr<NodeSerialization> s(new NodeSerialization);
                ar & boost::serialization::make_nvp("item",*s);
                _children.push_back(s);
            }
        }
        if (version >= NODE_SERIALIZATION_INTRODUCES_USER_COMPONENTS)  {
            ar & boost::serialization::make_nvp("UserComponents",_userComponents);
        }

    }
    
    BOOST_SERIALIZATION_SPLIT_MEMBER()
};


BOOST_CLASS_VERSION(NodeSerialization, NODE_SERIALIZATION_CURRENT_VERSION)


#endif // NODESERIALIZATION_H
