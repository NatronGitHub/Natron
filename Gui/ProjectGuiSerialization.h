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
#include <boost/serialization/list.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/version.hpp>
#include "Engine/Rect.h"
#include "Gui/NodeGuiSerialization.h"
#include "Gui/NodeBackDropSerialization.h"

#define VIEWER_DATA_INTRODUCES_WIPE_COMPOSITING 2
#define VIEWER_DATA_INTRODUCES_FRAME_RANGE 3
#define VIEWER_DATA_SERIALIZATION_VERSION VIEWER_DATA_INTRODUCES_FRAME_RANGE

#define PROJECT_GUI_INTRODUCES_BACKDROPS 2
#define PROJECT_GUI_SERIALIZATION_VERSION PROJECT_GUI_INTRODUCES_BACKDROPS

class ProjectGui;

struct ViewerData {
    double zoomLeft;
    double zoomBottom;
    double zoomFactor;
    double zoomPAR;
    bool userRoIenabled;
    RectI userRoI;
    bool isClippedToProject;
    bool autoContrastEnabled;
    double gain;
    bool renderScaleActivated;
    int mipMapLevel;
    std::string colorSpace;
    std::string channels;
    bool zoomOrPanSinceLastFit;
    int wipeCompositingOp;
    bool frameRangeLocked;
    
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar,const unsigned int version)
    {
        
        ar & boost::serialization::make_nvp("zoomLeft",zoomLeft);
        ar & boost::serialization::make_nvp("zoomBottom",zoomBottom);
        ar & boost::serialization::make_nvp("zoomFactor",zoomFactor);
        ar & boost::serialization::make_nvp("zoomPAR",zoomPAR);
        ar & boost::serialization::make_nvp("UserRoIEnabled",userRoIenabled);
        ar & boost::serialization::make_nvp("UserRoI",userRoI);
        ar & boost::serialization::make_nvp("ClippedToProject",isClippedToProject);
        ar & boost::serialization::make_nvp("AutoContrast",autoContrastEnabled);
        ar & boost::serialization::make_nvp("Gain",gain);
        ar & boost::serialization::make_nvp("ColorSpace",colorSpace);
        ar & boost::serialization::make_nvp("Channels",channels);
        ar & boost::serialization::make_nvp("RenderScaleActivated",renderScaleActivated);
        ar & boost::serialization::make_nvp("MipMapLevel",mipMapLevel);
        if (version >= VIEWER_DATA_INTRODUCES_WIPE_COMPOSITING) {
            ar & boost::serialization::make_nvp("ZoomOrPanSinceFit",zoomOrPanSinceLastFit);
            ar & boost::serialization::make_nvp("CompositingOP",wipeCompositingOp);
        } else {
            zoomOrPanSinceLastFit = false;
            wipeCompositingOp = 0;
        }
        if (version >= VIEWER_DATA_INTRODUCES_FRAME_RANGE) {
            ar & boost::serialization::make_nvp("FrameRangeLocked",frameRangeLocked);
        } else {
            frameRangeLocked = false;
        }
    }
};

BOOST_CLASS_VERSION(ViewerData, VIEWER_DATA_SERIALIZATION_VERSION)

struct PaneLayout{
    
    bool floating;
    int posx,posy;
    bool parentingCreated;
    std::list<bool> splits;
    std::string parentName;
    std::list<std::string> splitsNames;
    std::list<std::string> tabs;
    
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
    
    std::list< NodeGuiSerialization > _serializedNodes;
    
    ///widget name, pane layout
    std::map<std::string,PaneLayout> _layout;
    
    //splitter name, splitter serialization
    std::map<std::string,std::string> _splittersStates;
    
    std::map<std::string, ViewerData > _viewersData;
    
    std::list<std::string> _histograms;
    
    std::list<NodeBackDropSerialization> _backdrops;

    bool _arePreviewTurnedOffGlobally;
    
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar,const unsigned int version)
    {
        (void)version;
        ar & boost::serialization::make_nvp("NodesGui",_serializedNodes);
        ar & boost::serialization::make_nvp("Gui_Layout",_layout);
        ar & boost::serialization::make_nvp("Splitters_states",_splittersStates);
        ar & boost::serialization::make_nvp("ViewersData",_viewersData);
        ar & boost::serialization::make_nvp("PreviewsTurnedOffGlobaly",_arePreviewTurnedOffGlobally);
        ar & boost::serialization::make_nvp("Histograms",_histograms);
        if (version >= PROJECT_GUI_INTRODUCES_BACKDROPS) {
            ar & boost::serialization::make_nvp("Backdrops",_backdrops);
        }
    }
    
public:
    
    ProjectGuiSerialization(){}
    
    ~ProjectGuiSerialization(){ _serializedNodes.clear(); }
    
    void initialize(const ProjectGui* projectGui);
    
    const std::list< NodeGuiSerialization >& getSerializedNodesGui() const { return _serializedNodes; }
    
    const std::map<std::string,PaneLayout>& getGuiLayout() const { return _layout; }
    
    const std::map<std::string,std::string>& getSplittersStates() const { return _splittersStates; }
    
    const std::map<std::string, ViewerData >& getViewersProjections() const { return _viewersData; }
    
    bool arePreviewsTurnedOffGlobally() const { return _arePreviewTurnedOffGlobally; }
    
    const std::list<std::string>& getHistograms() const { return _histograms; }
    
    const std::list<NodeBackDropSerialization>& getBackdrops() const { return _backdrops; }
    
private:
    
    void createParenting(std::map<std::string,PaneLayout>::iterator it);
};
BOOST_CLASS_VERSION(ProjectGuiSerialization, PROJECT_GUI_SERIALIZATION_VERSION)


#endif // PROJECTGUISERIALIZATION_H
