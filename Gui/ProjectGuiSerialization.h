//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */

#ifndef PROJECTGUISERIALIZATION_H
#define PROJECTGUISERIALIZATION_H

#include "Global/Macros.h"
CLANG_DIAG_OFF(unused-parameter)
// /opt/local/include/boost/serialization/smart_cast.hpp:254:25: warning: unused parameter 'u' [-Wunused-parameter]
#include <boost/archive/xml_iarchive.hpp>
CLANG_DIAG_ON(unused-parameter)
#include <boost/archive/xml_oarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/shared_ptr.hpp>

#include "Engine/Rect.h"
#include "Gui/NodeGuiSerialization.h"

class ProjectGui;

struct ViewerData {
    double left,bottom,zoomFactor,aspectRatio;
    bool userRoIenabled;
    RectI userRoI;
    bool isClippedToProject;
    double exposure;
    std::string colorSpace;
    std::string channels;
    
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar,const unsigned int version)
    {
        (void)version;
        ar & boost::serialization::make_nvp("Left",left);
        ar & boost::serialization::make_nvp("Bottom",bottom);
        ar & boost::serialization::make_nvp("ZoomFactor",zoomFactor);
        ar & boost::serialization::make_nvp("AspectRatio",aspectRatio);
        ar & boost::serialization::make_nvp("UserRoIEnabled",userRoIenabled);
        ar & boost::serialization::make_nvp("UserRoI",userRoI);
        ar & boost::serialization::make_nvp("ClippedToProject",isClippedToProject);
        ar & boost::serialization::make_nvp("Exposure",exposure);
        ar & boost::serialization::make_nvp("ColorSpace",colorSpace);
        ar & boost::serialization::make_nvp("Channels",channels);
    }
};



struct PaneLayout{
    
    bool floating;
    int posx,posy;
    bool parentingCreated;
    std::vector<bool> splits;
    std::string parentName;
    std::vector<std::string> splitsNames;
    std::vector<std::string> tabs;
    
    
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar,const unsigned int version)
    {
        (void)version;
        ar & boost::serialization::make_nvp("Floating",floating);
        ar & boost::serialization::make_nvp("Splits",splits);
        ar & boost::serialization::make_nvp("ParentName",parentName);
        ar & boost::serialization::make_nvp("SplitsNames",splitsNames);
        ar & boost::serialization::make_nvp("Tabs",tabs);
    }
};

class ProjectGuiSerialization {
    
    std::vector< boost::shared_ptr<NodeGuiSerialization> > _serializedNodes;
    
    
    std::map<std::string,PaneLayout> _layout;
    
    //splitter name, splitter serialization
    std::map<std::string,std::string> _splittersStates;
    
    std::map<std::string, ViewerData > _viewersData;
    
    
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar,const unsigned int version)
    {
        (void)version;
        ar & boost::serialization::make_nvp("NodesGui",_serializedNodes);
        ar & boost::serialization::make_nvp("Gui_Layout",_layout);
        ar & boost::serialization::make_nvp("Splitters_states",_splittersStates);
        ar & boost::serialization::make_nvp("ViewersData",_viewersData);

    }
    
public:
    
    ProjectGuiSerialization(){}
    
    ~ProjectGuiSerialization(){ _serializedNodes.clear(); }
    
    void initialize(const ProjectGui* projectGui);
    
    const std::vector< boost::shared_ptr<NodeGuiSerialization> >& getSerializedNodesGui() const { return _serializedNodes; }
    
    const std::map<std::string,PaneLayout>& getGuiLayout() const { return _layout; }
    
    const std::map<std::string,std::string>& getSplittersStates() const { return _splittersStates; }
    
    const std::map<std::string, ViewerData >& getViewersProjections() const { return _viewersData; }
    
private:
    
    void createParenting(std::map<std::string,PaneLayout>::iterator it);
};


#endif // PROJECTGUISERIALIZATION_H
