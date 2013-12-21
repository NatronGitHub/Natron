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

#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/map.hpp>

#include "Gui/ProjectGui.h"
#include "Gui/NodeGuiSerialization.h"
#include "Gui/NodeGui.h"

struct PaneLayout{
    
    bool floating;
    int posx,posy;
    bool parentingCreated;
    std::vector<bool> splits;
    std::string parentName;
    std::vector<std::string> splitsNames;
    std::vector<std::string> tabs;
    std::string splitterSerialization;
    
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
        ar & boost::serialization::make_nvp("Splitter",splitterSerialization);
    }
};

class ProjectGuiSerialization {
    
    std::vector< boost::shared_ptr<NodeGuiSerialization> > _serializedNodes;
    
    
    std::map<std::string,PaneLayout> _layout;
    
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar,const unsigned int version)
    {
        (void)version;
        ar & boost::serialization::make_nvp("NodesGui",_serializedNodes);
        ar & boost::serialization::make_nvp("Gui_Layout",_layout);
    }
    
public:
    
    ProjectGuiSerialization(){}
    
    ~ProjectGuiSerialization(){ _serializedNodes.clear(); }
    
    void initialize(const ProjectGui* projectGui);
    
    const std::vector< boost::shared_ptr<NodeGuiSerialization> >& getSerializedNodesGui() const { return _serializedNodes; }
    
    const std::map<std::string,PaneLayout>& getGuiLayout() const { return _layout; }
    
private:
    
    void createParenting(std::map<std::string,PaneLayout>::iterator it);
};


#endif // PROJECTGUISERIALIZATION_H
