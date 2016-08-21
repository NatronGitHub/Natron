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
#include "Engine/SerializationBase.h"
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
#define NODE_SERIALIZATION_EXTERNALIZE_SERIALIZATION 16
#define NODE_SERIALIZATION_CHANGE_PYTHON_MODULE_TO_ONLY_NAME 17
#define NODE_SERIALIZATION_CURRENT_VERSION NODE_SERIALIZATION_CHANGE_PYTHON_MODULE_TO_ONLY_NAME

NATRON_NAMESPACE_ENTER;

class NodeSerialization : public SerializationObjectBase
{
public:

    typedef std::list<KnobSerializationPtr> KnobValues;

    ///Used to serialize
    explicit NodeSerialization(const NodePtr & n = NodePtr(),
                      bool serializeInputs = true);

    ~NodeSerialization()
    {
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

    const std::string & getGroupFullyQualifiedName() const
    {
        return _groupFullyQualifiedScriptName;
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
        return !_hasBeenSerialized;
    }

    U64 getKnobsAge() const
    {
        return _knobsAge;
    }

    const std::string & getMasterNodeName() const
    {
        return _masterNodeFullyQualifiedScriptName;
    }

    boost::shared_ptr<RotoContextSerialization>  getRotoContext() const
    {
        return _rotoContext;
    }


    boost::shared_ptr<TrackerContextSerialization> getTrackerContext() const
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

    const std::list<boost::shared_ptr<GroupKnobSerialization> >& getUserPages() const
    {
        return _userPages;
    }

    const std::list< NodeSerializationPtr >& getNodesCollection() const
    {
        return _children;
    }

    const std::list<ImageComponentsSerialization>& getUserCreatedComponents() const
    {
        return _userComponents;
    }

    const std::string& getGroupFullScriptName() const
    {
        return _groupFullyQualifiedScriptName;
    }

    void getPosition(double *x, double *y) const
    {
        *x = _nodePositionCoords[0];
        *y = _nodePositionCoords[1];
    }

    void getSize(double *w, double *h) const
    {
        *w = _nodeSize[0];
        *h = _nodeSize[1];
    }

    void getColor(double *r, double *g, double *b) const
    {
        *r = _nodeColor[0];
        *g = _nodeColor[1];
        *b = _nodeColor[2];
    }

    void getOverlayColor(double *r, double *g, double *b) const
    {
        *r = _overlayColor[0];
        *g = _overlayColor[1];
        *b = _overlayColor[2];
    }

    bool getSelected() const
    {
        return _nodeIsSelected;
    }

    unsigned int getVersion() const
    {
        return _version;
    }


    unsigned int _version;

    bool _hasBeenSerialized;

    // Knobs serialization
    KnobValues _knobsValues;

    // The age of the knobs. Used for caching
    U64 _knobsAge;

    // The group full script-name or empty if the node is part of the top-level group
    std::string _groupFullyQualifiedScriptName;

    // The node label as seen in the nodegraph
    std::string _nodeLabel;

    // The node script-name as used in Python
    std::string _nodeScriptName;

    // The cache ID is the first script name of the node and is ensured to be unique for caching. It cannot be changed.
    std::string _cacheID;

    // The ID of the plug-in embedded into the node
    std::string _pluginID;

    // Plugin version used by the node
    int _pluginMajorVersion;
    int _pluginMinorVersion;

    // If this node is a clone of another one, this is the full script-name of the master node
    std::string _masterNodeFullyQualifiedScriptName;

    // Serialization of inputs, this is a map of the input label to the script-name (not full) of the input node
    std::map<std::string, std::string> _inputs;

    // Deprecated: Before Natron 2, inputs were serialized just by their index. This is left here for backward compatibility,
    // new serialization schemes should rather use the _inputs map which is more robust against changes.
    std::vector<std::string> _oldInputs;

    // If this node has a Roto context, this is its serialization
    boost::shared_ptr<RotoContextSerialization> _rotoContext;
    // If this node has a Tracker context, this is its serialization
    boost::shared_ptr<TrackerContextSerialization> _trackerContext;

    // Deprecated: In Natron 1, the tracker was a bundle with sub instances where each track would be 1 node.
    // This is left here for backward compatibility.
    std::string _multiInstanceParentName;

    // The serialization of the pages created by user
    std::list<boost::shared_ptr<GroupKnobSerialization> > _userPages;

    // The pages order in the node by script-name
    std::list<std::string> _pagesIndexes;

    // If this node is a group or a multi-instance, this is the children
    std::list< NodeSerializationPtr > _children;

    // If this node is a PyPlug, this is the Python module name
    std::string _pythonModule;

    // If this node is a PyPlug, this is the PyPlug version
    unsigned int _pythonModuleVersion;

    // This is the user created components on the node
    std::list<ImageComponentsSerialization> _userComponents;

    // UI stuff
    double _nodePositionCoords[2]; // x,y  X=Y=INT_MIN if there is no position info
    double _nodeSize[2]; // width, height, W=H=-1 if there is no size info
    double _nodeColor[3]; // node color (RGB), between 0. and 1. If R=G=B=-1 then no color
    double _overlayColor[3]; // overlay color (RGB), between 0. and 1. If R=G=B=-1 then no color
    bool _nodeIsSelected; // is this node selected by the user ?

    // Ordering of the knobs in the viewer UI for this node
    std::list<std::string> _viewerUIKnobsOrder;

    void initializeForSerialization(const NodePtr& node, bool serializeInputs);

    friend class ::boost::serialization::access;
    template<class Archive>
    void save(Archive & ar,
              const unsigned int /*version*/) const
    {
        ar & ::boost::serialization::make_nvp("Plugin_label", _nodeLabel);
        ar & ::boost::serialization::make_nvp("GroupName", _groupFullyQualifiedScriptName);
        ar & ::boost::serialization::make_nvp("Plugin_script_name", _nodeScriptName);
        ar & ::boost::serialization::make_nvp("Plugin_id", _pluginID);
        ar & ::boost::serialization::make_nvp("PythonModule", _pythonModule);
        ar & ::boost::serialization::make_nvp("PythonModuleVersion", _pythonModuleVersion);
        ar & ::boost::serialization::make_nvp("Plugin_major_version", _pluginMajorVersion);
        ar & ::boost::serialization::make_nvp("Plugin_minor_version", _pluginMinorVersion);
        int nbKnobs = (int)_knobsValues.size();
        ar & ::boost::serialization::make_nvp("KnobsCount", nbKnobs);

        for (KnobValues::const_iterator it = _knobsValues.begin(); it != _knobsValues.end(); ++it) {
            ar & ::boost::serialization::make_nvp( "item", *(*it) );
        }
        ar & ::boost::serialization::make_nvp("Inputs_map", _inputs);
        ar & ::boost::serialization::make_nvp("KnobsAge", _knobsAge);
        ar & ::boost::serialization::make_nvp("MasterNode", _masterNodeFullyQualifiedScriptName);
        bool hasRotoContext = _rotoContext.get() != 0;
        ar & ::boost::serialization::make_nvp("HasRotoContext", hasRotoContext);
        if (hasRotoContext) {
            ar & ::boost::serialization::make_nvp("RotoContext", *_rotoContext);
        }

        bool hasTrackerContext = _trackerContext.get() != 0;
        ar & ::boost::serialization::make_nvp("HasTrackerContext", hasTrackerContext);
        if (hasTrackerContext) {
            ar & ::boost::serialization::make_nvp("TrackerContext", *_trackerContext);
        }

        ar & ::boost::serialization::make_nvp("MultiInstanceParent", _multiInstanceParentName);
        int userPagesCount = (int)_userPages.size();
        ar & ::boost::serialization::make_nvp("UserPagesCount", userPagesCount);
        for (std::list<boost::shared_ptr<GroupKnobSerialization> >::const_iterator it = _userPages.begin(); it != _userPages.end(); ++it) {
            ar & ::boost::serialization::make_nvp("item", **it);
        }

        int pageSizes = (int)_pagesIndexes.size();
        ar & ::boost::serialization::make_nvp("PagesCount", pageSizes);
        for (std::list<std::string>::const_iterator it = _pagesIndexes.begin(); it != _pagesIndexes.end(); ++it) {
            ar & ::boost::serialization::make_nvp("name", *it);
        }

        int nodesCount = (int)_children.size();
        ar & ::boost::serialization::make_nvp("Children", nodesCount);

        for (std::list< NodeSerializationPtr >::const_iterator it = _children.begin();
             it != _children.end();
             ++it) {
            ar & ::boost::serialization::make_nvp("item", **it);
        }

        ar & ::boost::serialization::make_nvp("UserComponents", _userComponents);
        ar & ::boost::serialization::make_nvp("CacheID", _cacheID);

        ar & ::boost::serialization::make_nvp("PosX", _nodePositionCoords[0]);
        ar & ::boost::serialization::make_nvp("PosY", _nodePositionCoords[1]);
        ar & ::boost::serialization::make_nvp("Width", _nodeSize[0]);
        ar & ::boost::serialization::make_nvp("Height", _nodeSize[1]);
        ar & ::boost::serialization::make_nvp("NodeColor_R", _nodeColor[0]);
        ar & ::boost::serialization::make_nvp("NodeColor_G", _nodeColor[1]);
        ar & ::boost::serialization::make_nvp("NodeColor_B", _nodeColor[2]);
        ar & ::boost::serialization::make_nvp("OverlayColor_R", _overlayColor[0]);
        ar & ::boost::serialization::make_nvp("OverlayColor_G", _overlayColor[1]);
        ar & ::boost::serialization::make_nvp("OverlayColor_B", _overlayColor[2]);
        ar & ::boost::serialization::make_nvp("IsSelected", _nodeIsSelected);
        ar & ::boost::serialization::make_nvp("ViewerKnobsOrder", _viewerUIKnobsOrder);

    } // save

    template<class Archive>
    void load(Archive & ar,
              const unsigned int version)
    {
        _version = version;
        
        if (version > NODE_SERIALIZATION_CURRENT_VERSION) {
            throw std::invalid_argument("The project you're trying to load contains data produced by a more recent "
                                        "version of Natron, which makes it unreadable");
        }

        _hasBeenSerialized = true;
        
        ar & ::boost::serialization::make_nvp("Plugin_label", _nodeLabel);
        if (version >= NODE_SERIALIZATION_EXTERNALIZE_SERIALIZATION) {
            ar & ::boost::serialization::make_nvp("GroupName", _groupFullyQualifiedScriptName);
        }
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
        int nbKnobs;
        ar & ::boost::serialization::make_nvp("KnobsCount", nbKnobs);
        for (int i = 0; i < nbKnobs; ++i) {
            KnobSerializationPtr ks(new KnobSerialization);
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
        ar & ::boost::serialization::make_nvp("MasterNode", _masterNodeFullyQualifiedScriptName);
        if (version < NODE_SERIALIZATION_INTRODUCES_SCRIPT_NAME) {
            _masterNodeFullyQualifiedScriptName = NATRON_PYTHON_NAMESPACE::makeNameScriptFriendly(_masterNodeFullyQualifiedScriptName);
        }

        if (version >= NODE_SERIALIZATION_V_INTRODUCES_ROTO) {
            bool hasRotoContext;
            ar & ::boost::serialization::make_nvp("HasRotoContext", hasRotoContext);
            if (hasRotoContext) {
                _rotoContext.reset(new RotoContextSerialization);
                ar & ::boost::serialization::make_nvp("RotoContext", *_rotoContext);
            }

            if (version >= NODE_SERIALIZATION_INTRODUCES_TRACKER_CONTEXT) {
                bool hasTrackerContext;
                ar & boost::serialization::make_nvp("HasTrackerContext", hasTrackerContext);
                if (hasTrackerContext) {
                    _trackerContext.reset(new TrackerContextSerialization);
                    ar & boost::serialization::make_nvp("TrackerContext", *_trackerContext);
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
                boost::shared_ptr<GroupKnobSerialization> s( new GroupKnobSerialization() );
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
            } else {
                _pagesIndexes.push_back(NATRON_USER_MANAGED_KNOBS_PAGE);
            }
        }

        if (version >= NODE_SERIALIZATION_INTRODUCES_GROUPS) {
            int nodesCount;
            ar & ::boost::serialization::make_nvp("Children", nodesCount);

            for (int i = 0; i < nodesCount; ++i) {
                NodeSerializationPtr s(new NodeSerialization);
                ar & ::boost::serialization::make_nvp("item", *s);
                _children.push_back(s);
            }
        }
        if (version >= NODE_SERIALIZATION_INTRODUCES_USER_COMPONENTS) {
            if (version >= NODE_SERIALIZATION_EXTERNALIZE_SERIALIZATION) {
                ar & ::boost::serialization::make_nvp("UserComponents", _userComponents);
            } else {
                std::list<ImageComponents> comps;
                ar & ::boost::serialization::make_nvp("UserComponents", comps);
                for (std::list<ImageComponents>::iterator it = comps.begin(); it!=comps.end(); ++it) {
                    ImageComponentsSerialization s;
                    s.layerName = it->getLayerName();
                    s.globalCompsName = it->getComponentsGlobalName();
                    s.channelNames = it->getComponentsNames();
                    _userComponents.push_back(s);
                }
            }
        }
        if (version >= NODE_SERIALIZATION_INTRODUCES_CACHE_ID) {
            ar & ::boost::serialization::make_nvp("CacheID", _cacheID);
        }

        if (version >= NODE_SERIALIZATION_EXTERNALIZE_SERIALIZATION) {
            ar & ::boost::serialization::make_nvp("PosX", _nodePositionCoords[0]);
            ar & ::boost::serialization::make_nvp("PosY", _nodePositionCoords[1]);
            ar & ::boost::serialization::make_nvp("Width", _nodeSize[0]);
            ar & ::boost::serialization::make_nvp("Height", _nodeSize[1]);
            ar & ::boost::serialization::make_nvp("NodeColor_R", _nodeColor[0]);
            ar & ::boost::serialization::make_nvp("NodeColor_G", _nodeColor[1]);
            ar & ::boost::serialization::make_nvp("NodeColor_B", _nodeColor[2]);
            ar & ::boost::serialization::make_nvp("OverlayColor_R", _overlayColor[0]);
            ar & ::boost::serialization::make_nvp("OverlayColor_G", _overlayColor[1]);
            ar & ::boost::serialization::make_nvp("OverlayColor_B", _overlayColor[2]);
            ar & ::boost::serialization::make_nvp("IsSelected", _nodeIsSelected);
            ar & ::boost::serialization::make_nvp("ViewerKnobsOrder", _viewerUIKnobsOrder);

        } 
    } // load

    BOOST_SERIALIZATION_SPLIT_MEMBER()
};

NATRON_NAMESPACE_EXIT;

BOOST_CLASS_VERSION(NATRON_NAMESPACE::NodeSerialization, NODE_SERIALIZATION_CURRENT_VERSION)


#endif // NODESERIALIZATION_H
