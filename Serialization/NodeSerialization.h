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


#include "Serialization/KnobSerialization.h"
#include "Serialization/TrackerSerialization.h"
#include "Serialization/RotoContextSerialization.h"
#include "Serialization/ImageParamsSerialization.h"
#include "Serialization/SerializationBase.h"

#ifdef NATRON_BOOST_SERIALIZATION_COMPAT
#include "Engine/ImageComponents.h"
#include "Engine/EffectInstance.h"
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
#endif

SERIALIZATION_NAMESPACE_ENTER;

class NodeSerialization : public SerializationObjectBase
{
public:

    ///Used to serialize
    explicit NodeSerialization()
    : _knobsValues()
    , _knobsAge(0)
    , _groupFullyQualifiedScriptName()
    , _nodeLabel()
    , _nodeScriptName()
    , _cacheID()
    , _pluginID()
    , _pluginMajorVersion(-1)
    , _pluginMinorVersion(-1)
    , _masterNodeFullyQualifiedScriptName()
    , _inputs()
    , _rotoContext()
    , _trackerContext()
    , _multiInstanceParentName()
    , _userPages()
    , _pagesIndexes()
    , _children()
    , _pythonModule()
    , _pythonModuleVersion(0)
    , _userComponents()
    , _nodePositionCoords()
    , _nodeSize()
    , _nodeColor()
    , _overlayColor()
    , _viewerUIKnobsOrder()
#ifdef NATRON_BOOST_SERIALIZATION_COMPAT
    , _boostSerializationClassVersion(NODE_SERIALIZATION_CURRENT_VERSION)
#endif
    {
        _nodePositionCoords[0] = _nodePositionCoords[1] = INT_MIN;
        _nodeSize[0] = _nodeSize[1] = -1;
        _nodeColor[0] = _nodeColor[1] = _nodeColor[2] = -1;
        _overlayColor[0] = _overlayColor[1] = _overlayColor[2] = -1;

    }


    virtual ~NodeSerialization()
    {
    }



    // Knobs serialization
    KnobSerializationList _knobsValues;

    // The age of the knobs. Used for caching
    unsigned long long _knobsAge;

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

    // If this node has a Roto context, this is its serialization
    boost::shared_ptr<RotoContextSerialization> _rotoContext;
    // If this node has a Tracker context, this is its serialization
    boost::shared_ptr<TrackerContextSerialization> _trackerContext;

    // Deprecated: In Natron 1, the tracker was a bundle with sub instances where each track would be 1 node.
    // This is left here for backward compatibility.
    std::string _multiInstanceParentName;

    // The serialization of the pages created by the user
    std::list<boost::shared_ptr<GroupKnobSerialization> > _userPages;

    // The pages order in the node by script-name
    std::list<std::string> _pagesIndexes;

    // If this node is a group or a multi-instance, this is the children
    NodeSerializationList _children;

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

    // Ordering of the knobs in the viewer UI for this node
    std::list<std::string> _viewerUIKnobsOrder;

#ifdef NATRON_BOOST_SERIALIZATION_COMPAT
    unsigned int _boostSerializationClassVersion;
#endif

    virtual void encode(YAML::Emitter& em) const OVERRIDE;

    virtual void decode(const YAML::Node& node) OVERRIDE;

#ifdef NATRON_BOOST_SERIALIZATION_COMPAT
    template<class Archive>
    void save(Archive & ar,
              const unsigned int /*version*/) const
    {
         throw std::runtime_error("Saving with boost is no longer supported");

    } // save

    template<class Archive>
    void load(Archive & ar,
              const unsigned int version)
    {
        _boostSerializationClassVersion = version;
        
        if (version > NODE_SERIALIZATION_CURRENT_VERSION) {
            throw std::invalid_argument("The project you're trying to load contains data produced by a more recent "
                                        "version of Natron, which makes it unreadable");
        }

        ar & ::boost::serialization::make_nvp("Plugin_label", _nodeLabel);
        if (version >= NODE_SERIALIZATION_EXTERNALIZE_SERIALIZATION) {
            ar & ::boost::serialization::make_nvp("GroupName", _groupFullyQualifiedScriptName);
        }
        if (version >= NODE_SERIALIZATION_INTRODUCES_SCRIPT_NAME) {
            ar & ::boost::serialization::make_nvp("Plugin_script_name", _nodeScriptName);
        } else {
            _nodeScriptName = NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::makeNameScriptFriendly(_nodeLabel);
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
            std::vector<std::string> oldInputs;
            ar & ::boost::serialization::make_nvp("Inputs_map", oldInputs);
            if (version < NODE_SERIALIZATION_INTRODUCES_SCRIPT_NAME) {
                for (std::size_t i = 0; i < oldInputs.size(); ++i) {
                    oldInputs[i] = NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::makeNameScriptFriendly(oldInputs[i]);
                    std::stringstream ss;
                    ss << i;
                    _inputs[ss.str()] = oldInputs[i];
                }
            }
        } else {
            ar & ::boost::serialization::make_nvp("Inputs_map", _inputs);
        }

        ar & ::boost::serialization::make_nvp("KnobsAge", _knobsAge);
        ar & ::boost::serialization::make_nvp("MasterNode", _masterNodeFullyQualifiedScriptName);
        if (version < NODE_SERIALIZATION_INTRODUCES_SCRIPT_NAME) {
            _masterNodeFullyQualifiedScriptName = NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::makeNameScriptFriendly(_masterNodeFullyQualifiedScriptName);
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
                _multiInstanceParentName = NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::makeNameScriptFriendly(_multiInstanceParentName);
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
                std::list<NATRON_NAMESPACE::ImageComponents> comps;
                ar & ::boost::serialization::make_nvp("UserComponents", comps);
                for (std::list<NATRON_NAMESPACE::ImageComponents>::iterator it = comps.begin(); it!=comps.end(); ++it) {
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
            bool selected;
            ar & ::boost::serialization::make_nvp("IsSelected", selected);
            ar & ::boost::serialization::make_nvp("ViewerKnobsOrder", _viewerUIKnobsOrder);

        } 
    } // load

    BOOST_SERIALIZATION_SPLIT_MEMBER()
#endif
};


class NodePresetSerialization : public SerializationObjectBase
{
public:
    
    int version;
    std::string pluginID;
    std::string presetLabel;
    std::string presetIcon;
    int presetSymbol;
    int presetModifiers;
    NodeSerialization node;

    // This flag is used to speed-up preset parsing by Natron: it will only read relevant meta-data from this class
    // to show to the user in the interface instead of the whole node serialization object.
    bool decodeMetaDataOnly;

    NodePresetSerialization()
    : SerializationObjectBase()
    , version(0)
    , pluginID()
    , presetLabel()
    , presetIcon()
    , presetSymbol(0)
    , presetModifiers(0)
    , node()
    , decodeMetaDataOnly(false)
    {

    }

    virtual ~NodePresetSerialization()
    {

    }

    virtual void encode(YAML::Emitter& em) const OVERRIDE;

    virtual void decode(const YAML::Node& node) OVERRIDE;
};

SERIALIZATION_NAMESPACE_EXIT;

#ifdef NATRON_BOOST_SERIALIZATION_COMPAT
BOOST_CLASS_VERSION(SERIALIZATION_NAMESPACE::NodeSerialization, NODE_SERIALIZATION_CURRENT_VERSION)
#endif


#endif // NODESERIALIZATION_H
