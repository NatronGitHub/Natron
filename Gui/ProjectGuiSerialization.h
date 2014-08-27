//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
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
#define PROJECT_GUI_REMOVES_ALL_NODE_PREVIEW_TOGGLED 3
#define PROJECT_GUI_INTRODUCES_PANELS 4
#define PROJECT_GUI_CHANGES_SPLITTERS 5
#define PROJECT_GUI_EXERNALISE_GUI_LAYOUT 6
#define PROJECT_GUI_SERIALIZATION_VERSION PROJECT_GUI_EXERNALISE_GUI_LAYOUT

#define PANE_SERIALIZATION_INTRODUCES_CURRENT_TAB 2
#define PANE_SERIALIZATION_INTRODUCES_SIZE 3
#define PANE_SERIALIZATION_VERSION PANE_SERIALIZATION_INTRODUCES_SIZE

class ProjectGui;
class Gui;

struct ViewerData
{
    double zoomLeft;
    double zoomBottom;
    double zoomFactor;
    double zoomPAR;
    bool userRoIenabled;
    RectD userRoI; // in canonical coordinates
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
    void serialize(Archive & ar,
                   const unsigned int version)
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

struct PaneLayout
{
    bool floating;

    ///These are only relevant when floating is true
    int posx,posy;
    int width,height;
    bool parentingCreated;
    std::list<bool> splits;
    std::string parentName;
    std::list<std::string> splitsNames;
    std::list<std::string> tabs;
    int currentIndex;

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar,
                   const unsigned int version)
    {
        (void)version;
        ar & boost::serialization::make_nvp("Floating",floating);
        ar & boost::serialization::make_nvp("Splits",splits);
        ar & boost::serialization::make_nvp("ParentName",parentName);
        ar & boost::serialization::make_nvp("SplitsNames",splitsNames);
        ar & boost::serialization::make_nvp("Tabs",tabs);
        if (version >= PANE_SERIALIZATION_INTRODUCES_CURRENT_TAB) {
            ar & boost::serialization::make_nvp("Index",currentIndex);
        }
        if (version >= PANE_SERIALIZATION_INTRODUCES_SIZE) {
            if (floating) {
                ar & boost::serialization::make_nvp("x",posx);
                ar & boost::serialization::make_nvp("y",posy);
                ar & boost::serialization::make_nvp("w",width);
                ar & boost::serialization::make_nvp("h",height);
            }
        }
    }
};

BOOST_CLASS_VERSION(PaneLayout, PANE_SERIALIZATION_VERSION)


class GuiLayoutSerialization
{
public:

    GuiLayoutSerialization()
    {
    }

    void initialize(Gui* gui);

    ///widget name, pane layout
    std::map<std::string,PaneLayout> _layout;

    //splitter name, splitter serialization
    std::map<std::string,std::string> _splittersStates;

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar,
                   const unsigned int /*version*/)
    {
        ar & boost::serialization::make_nvp("Gui_Layout",_layout);
        ar & boost::serialization::make_nvp("Splitters_states",_splittersStates);
    }
};


class ProjectGuiSerialization
{
    std::list< NodeGuiSerialization > _serializedNodes;
    GuiLayoutSerialization _layoutSerialization;
    std::map<std::string, ViewerData > _viewersData;
    std::list<std::string> _histograms;
    std::list<NodeBackDropSerialization> _backdrops;
    std::list<std::string> _openedPanelsOrdered;
    unsigned int _version;

    friend class boost::serialization::access;

    template<class Archive>
    void save(Archive & ar,
              const unsigned int version) const
    {
        (void)version;
        ar & boost::serialization::make_nvp("NodesGui",_serializedNodes);
        ar & boost::serialization::make_nvp("Gui_Layout",_layoutSerialization);
        ar & boost::serialization::make_nvp("ViewersData",_viewersData);
        ar & boost::serialization::make_nvp("Histograms",_histograms);
        ar & boost::serialization::make_nvp("Backdrops",_backdrops);
        ar & boost::serialization::make_nvp("OpenedPanels",_openedPanelsOrdered);
    }

    template<class Archive>
    void load(Archive & ar,
              const unsigned int version)
    {
        (void)version;
        ar & boost::serialization::make_nvp("NodesGui",_serializedNodes);

        if (version < PROJECT_GUI_EXERNALISE_GUI_LAYOUT) {
            ar & boost::serialization::make_nvp("Gui_Layout",_layoutSerialization._layout);
            ar & boost::serialization::make_nvp("Splitters_states",_layoutSerialization._splittersStates);
            if (version < PROJECT_GUI_CHANGES_SPLITTERS) {
                _layoutSerialization._splittersStates.clear();
            }
        } else {
            ar & boost::serialization::make_nvp("Gui_Layout",_layoutSerialization);
        }
        ar & boost::serialization::make_nvp("ViewersData",_viewersData);
        if (version < PROJECT_GUI_REMOVES_ALL_NODE_PREVIEW_TOGGLED) {
            bool tmp = false;
            ar & boost::serialization::make_nvp("PreviewsTurnedOffGlobaly",tmp);
        }
        ar & boost::serialization::make_nvp("Histograms",_histograms);
        if (version >= PROJECT_GUI_INTRODUCES_BACKDROPS) {
            ar & boost::serialization::make_nvp("Backdrops",_backdrops);
        }
        if (version >= PROJECT_GUI_INTRODUCES_PANELS) {
            ar & boost::serialization::make_nvp("OpenedPanels",_openedPanelsOrdered);
        }
        _version = version;
    }

public:

    ProjectGuiSerialization()
    {
    }

    ~ProjectGuiSerialization()
    {
        _serializedNodes.clear();
    }

    void initialize(const ProjectGui* projectGui);

    const std::list< NodeGuiSerialization > & getSerializedNodesGui() const
    {
        return _serializedNodes;
    }

    const GuiLayoutSerialization & getGuiLayout() const
    {
        return _layoutSerialization;
    }

    const std::map<std::string, ViewerData > & getViewersProjections() const
    {
        return _viewersData;
    }

    const std::list<std::string> & getHistograms() const
    {
        return _histograms;
    }

    const std::list<NodeBackDropSerialization> & getBackdrops() const
    {
        return _backdrops;
    }

    const std::list<std::string> & getOpenedPanels() const
    {
        return _openedPanelsOrdered;
    }

    unsigned int getVersion() const
    {
        return _version;
    }

    BOOST_SERIALIZATION_SPLIT_MEMBER()
};

BOOST_CLASS_VERSION(ProjectGuiSerialization, PROJECT_GUI_SERIALIZATION_VERSION)


#endif // PROJECTGUISERIALIZATION_H
