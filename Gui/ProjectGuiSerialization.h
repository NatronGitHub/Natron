/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier
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

#ifndef PROJECTGUISERIALIZATION_H
#define PROJECTGUISERIALIZATION_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <list>
#include <string>
#include "Global/Macros.h"
#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
GCC_DIAG_OFF(unused-parameter)
// /opt/local/include/boost/serialization/smart_cast.hpp:254:25: warning: unused parameter 'u' [-Wunused-parameter]
#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/map.hpp>
// /usr/local/include/boost/serialization/shared_ptr.hpp:112:5: warning: unused typedef 'boost_static_assert_typedef_112' [-Wunused-local-typedef]
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/version.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
GCC_DIAG_ON(unused-parameter)
#endif

#include "Engine/RectD.h"

#include "Gui/NodeGuiSerialization.h"
#include "Gui/NodeBackDropSerialization.h"

#define PYTHON_PANEL_SERIALIZATION_VERSION 1

#define VIEWER_DATA_INTRODUCES_WIPE_COMPOSITING 2
#define VIEWER_DATA_INTRODUCES_FRAME_RANGE 3
#define VIEWER_DATA_INTRODUCES_TOOLBARS_VISIBLITY 4
#define VIEWER_DATA_INTRODUCES_CHECKERBOARD 5
#define VIEWER_DATA_INTRODUCES_FPS 6
#define VIEWER_DATA_REMOVES_ASPECT_RATIO 7
#define VIEWER_DATA_INTRODUCES_FPS_LOCK 8
#define VIEWER_DATA_REMOVES_FRAME_RANGE_LOCK 9
#define VIEWER_DATA_INTRODUCES_GAMMA 10
#define VIEWER_DATA_SERIALIZATION_VERSION VIEWER_DATA_INTRODUCES_GAMMA

#define PROJECT_GUI_INTRODUCES_BACKDROPS 2
#define PROJECT_GUI_REMOVES_ALL_NODE_PREVIEW_TOGGLED 3
#define PROJECT_GUI_INTRODUCES_PANELS 4
#define PROJECT_GUI_CHANGES_SPLITTERS 5
#define PROJECT_GUI_EXERNALISE_GUI_LAYOUT 6
#define PROJECT_GUI_SERIALIZATION_MAJOR_OVERHAUL 7
#define PROJECT_GUI_SERIALIZATION_NODEGRAPH_ZOOM_TO_POINT 8
#define PROJECT_GUI_SERIALIZATION_SCRIPT_EDITOR 9
#define PROJECT_GUI_SERIALIZATION_MERGE_BACKDROP 10
#define PROJECT_GUI_SERIALIZATION_INTRODUCES_PYTHON_PANELS 11
#define PROJECT_GUI_SERIALIZATION_VERSION PROJECT_GUI_SERIALIZATION_INTRODUCES_PYTHON_PANELS

#define PANE_SERIALIZATION_INTRODUCES_CURRENT_TAB 2
#define PANE_SERIALIZATION_INTRODUCES_SIZE 3
#define PANE_SERIALIZATION_MAJOR_OVERHAUL 4
#define PANE_SERIALIZATION_INTRODUCE_SCRIPT_NAME 5
#define PANE_SERIALIZATION_VERSION PANE_SERIALIZATION_INTRODUCE_SCRIPT_NAME

#define SPLITTER_SERIALIZATION_VERSION 1
#define APPLICATION_WINDOW_SERIALIZATION_VERSION 1

#define GUI_LAYOUT_SERIALIZATION_MAJOR_OVERHAUL 2
#define GUI_LAYOUT_SERIALIZATION_VERSION GUI_LAYOUT_SERIALIZATION_MAJOR_OVERHAUL

#define kNatronProjectSettingsPanelSerializationName "Natron_Project_Settings_Panel"

class ProjectGui;
class Gui;
class Splitter;
class TabWidget;
class PyPanel;

struct ViewerData
{
    double zoomLeft;
    double zoomBottom;
    double zoomFactor;
    bool userRoIenabled;
    RectD userRoI; // in canonical coordinates
    bool isClippedToProject;
    bool autoContrastEnabled;
    double gain;
    double gamma;
    bool renderScaleActivated;
    int mipMapLevel;
    std::string colorSpace;
    std::string channels;
    bool zoomOrPanSinceLastFit;
    int wipeCompositingOp;
    
    bool leftToolbarVisible;
    bool rightToolbarVisible;
    bool topToolbarVisible;
    bool playerVisible;
    bool infobarVisible;
    bool timelineVisible;

    bool checkerboardEnabled;
    
    double fps;
    bool fpsLocked;
    
    int leftBound,rightBound;
    
    unsigned int version;
    
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar,
                   const unsigned int version_)
    {
        version = version_;
        
        ar & boost::serialization::make_nvp("zoomLeft",zoomLeft);
        ar & boost::serialization::make_nvp("zoomBottom",zoomBottom);
        ar & boost::serialization::make_nvp("zoomFactor",zoomFactor);
        if (version <  VIEWER_DATA_REMOVES_ASPECT_RATIO) {
            double zoomPar;
            ar & boost::serialization::make_nvp("zoomPAR",zoomPar);
        }
        ar & boost::serialization::make_nvp("UserRoIEnabled",userRoIenabled);
        ar & boost::serialization::make_nvp("UserRoI",userRoI);
        ar & boost::serialization::make_nvp("ClippedToProject",isClippedToProject);
        ar & boost::serialization::make_nvp("AutoContrast",autoContrastEnabled);
        ar & boost::serialization::make_nvp("Gain",gain);
        if (version >= VIEWER_DATA_INTRODUCES_GAMMA) {
            ar & boost::serialization::make_nvp("Gain",gamma);
        } else {
            gamma = 1.;
        }
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
        if (version >= VIEWER_DATA_INTRODUCES_FRAME_RANGE && version < VIEWER_DATA_REMOVES_FRAME_RANGE_LOCK) {
            bool frameRangeLocked;
            ar & boost::serialization::make_nvp("FrameRangeLocked",frameRangeLocked);
        }
        if (version >=  VIEWER_DATA_REMOVES_FRAME_RANGE_LOCK) {
            ar & boost::serialization::make_nvp("LeftBound",leftBound);
            ar & boost::serialization::make_nvp("RightBound",rightBound);
        } else {
            leftBound = rightBound = 1;
        }
        
        if (version >= VIEWER_DATA_INTRODUCES_TOOLBARS_VISIBLITY) {
            ar & boost::serialization::make_nvp("LeftToolbarVisible",leftToolbarVisible);
            ar & boost::serialization::make_nvp("RightToolbarVisible",rightToolbarVisible);
            ar & boost::serialization::make_nvp("TopToolbarVisible",topToolbarVisible);
            ar & boost::serialization::make_nvp("PlayerVisible",playerVisible);
            ar & boost::serialization::make_nvp("TimelineVisible",timelineVisible);
            ar & boost::serialization::make_nvp("InfobarVisible",infobarVisible);
        } else {
            leftToolbarVisible = true;
            rightToolbarVisible = true;
            topToolbarVisible = true;
            playerVisible = true;
            timelineVisible = true;
            infobarVisible = true;
        }
        
        if (version >= VIEWER_DATA_INTRODUCES_CHECKERBOARD) {
            ar & boost::serialization::make_nvp("CheckerboardEnabled",checkerboardEnabled);
        } else {
            checkerboardEnabled = false;
        }
        
        if (version >= VIEWER_DATA_INTRODUCES_FPS) {
            ar & boost::serialization::make_nvp("Fps",fps);
        } else {
            fps = 24.;
        }
        
        if (version >= VIEWER_DATA_INTRODUCES_FPS_LOCK) {
            ar & boost::serialization::make_nvp("FpsLocked",fpsLocked);
        } else {
            fpsLocked = true;
        }
    }
};

BOOST_CLASS_VERSION(ViewerData, VIEWER_DATA_SERIALIZATION_VERSION)

struct PythonPanelSerialization
{
    
    std::list<boost::shared_ptr<KnobSerialization> > knobs;
    std::string name;
    std::string pythonFunction;
    std::string userData;
    
    void initialize(PyPanel* tab,const std::string& pythonFunction);
    
    template<class Archive>
    void save(Archive & ar,
              const unsigned int /*version*/) const
    {
        ar & boost::serialization::make_nvp("Label",name);
        ar & boost::serialization::make_nvp("PythonFunction",pythonFunction);
        
        int nKnobs = knobs.size();
        ar & boost::serialization::make_nvp("NumParams",nKnobs);
        for (std::list<boost::shared_ptr<KnobSerialization> >::const_iterator it = knobs.begin() ; it != knobs.end() ; ++it) {
            ar & boost::serialization::make_nvp("item",**it);
        }
        
        ar & boost::serialization::make_nvp("UserData",userData);
    }
    
    template<class Archive>
    void load(Archive & ar,
              const unsigned int /*version*/)
    {
        ar & boost::serialization::make_nvp("Label",name);
        ar & boost::serialization::make_nvp("PythonFunction",pythonFunction);
        
        int nKnobs;
        ar & boost::serialization::make_nvp("NumParams",nKnobs);
        for (int i = 0; i < nKnobs; ++i) {
            boost::shared_ptr<KnobSerialization> k(new KnobSerialization);
            ar & boost::serialization::make_nvp("item",*k);
            knobs.push_back(k);
        }
        
        ar & boost::serialization::make_nvp("UserData",userData);
    }
    
    BOOST_SERIALIZATION_SPLIT_MEMBER()
};

BOOST_CLASS_VERSION(PythonPanelSerialization, PYTHON_PANEL_SERIALIZATION_VERSION)

/**
 * @brief This is to keep compatibility until the version PANE_SERIALIZATION_INTRODUCES_SIZE of PaneLayout
 **/
struct PaneLayoutCompat_PANE_SERIALIZATION_INTRODUCES_SIZE
{
    bool floating;
    int posx,posy;
    int width,height;
    bool parentingCreated;
    std::list<bool> splits;
    std::string parentName;
    std::list<std::string> splitsNames;

    PaneLayoutCompat_PANE_SERIALIZATION_INTRODUCES_SIZE()
        : floating(false)
          , posx(0), posy(0), width(0), height(0)
          , parentingCreated(false)
          , splits()
          , parentName()
          , splitsNames()
    {
    }

    ~PaneLayoutCompat_PANE_SERIALIZATION_INTRODUCES_SIZE()
    {
    }
};

struct PaneLayout
{
    ///These fields are prior to version PANE_SERIALIZATION_MAJOR_OVERHAUL
    std::list<std::string> tabs;
    int currentIndex;

    ///Added in PANE_SERIALIZATION_MAJOR_OVERHAUL
    bool isAnchor;
    std::string name;
    
    ///This is only used to restore compatibility with project saved prior to PANE_SERIALIZATION_MAJOR_OVERHAUL

    PaneLayout()
        : tabs()
          , currentIndex(-1)
          , isAnchor(false)
          , name()
    {
    }

    ~PaneLayout()
    {
    }

    void initialize(TabWidget* tab);

    template<class Archive>
    void save(Archive & ar,
              const unsigned int /*version*/) const
    {
        ar & boost::serialization::make_nvp("Tabs",tabs);
        ar & boost::serialization::make_nvp("Index",currentIndex);
        ar & boost::serialization::make_nvp("Name",name);
        ar & boost::serialization::make_nvp("IsAnchor",isAnchor);
    }

    template<class Archive>
    void load(Archive & ar,
              const unsigned int version)
    {
        
        if (version < PANE_SERIALIZATION_MAJOR_OVERHAUL) {
            PaneLayoutCompat_PANE_SERIALIZATION_INTRODUCES_SIZE compat1;
            ar & boost::serialization::make_nvp("Floating",compat1.floating);
            ar & boost::serialization::make_nvp("Splits",compat1.splits);
            ar & boost::serialization::make_nvp("ParentName",compat1.parentName);
            ar & boost::serialization::make_nvp("SplitsNames",compat1.splitsNames);
            ar & boost::serialization::make_nvp("Tabs",tabs);
            if (version >= PANE_SERIALIZATION_INTRODUCES_CURRENT_TAB) {
                ar & boost::serialization::make_nvp("Index",currentIndex);
            }
            if (version >= PANE_SERIALIZATION_INTRODUCES_SIZE) {
                if (compat1.floating) {
                    ar & boost::serialization::make_nvp("x",compat1.posx);
                    ar & boost::serialization::make_nvp("y",compat1.posy);
                    ar & boost::serialization::make_nvp("w",compat1.width);
                    ar & boost::serialization::make_nvp("h",compat1.height);
                }
            }
        } else {
            ar & boost::serialization::make_nvp("Tabs",tabs);
            ar & boost::serialization::make_nvp("Index",currentIndex);
            ar & boost::serialization::make_nvp("Name",name);
            ar & boost::serialization::make_nvp("IsAnchor",isAnchor);
        }
        if (version < PANE_SERIALIZATION_INTRODUCE_SCRIPT_NAME) {
            
            for (std::list<std::string>::iterator it = tabs.begin() ; it != tabs.end() ; ++it) {
                //Try to map the tab name to an old name
                if (*it == "CurveEditor") {
                    *it = kCurveEditorObjectName;
                } else if (*it == "NodeGraph") {
                    *it = kNodeGraphObjectName;
                } else if (*it == "Properties") {
                    *it = "properties";
                }
            }
        }
    }
    
    BOOST_SERIALIZATION_SPLIT_MEMBER()
};

BOOST_CLASS_VERSION(PaneLayout, PANE_SERIALIZATION_VERSION)

struct SplitterSerialization
{
    std::string sizes;
    int orientation; //< corresponds to enum Natron::OrientationEnum
    struct Child
    {
        //One of the 2 ptrs below is NULL. The child can be either one of these.
        SplitterSerialization* child_asSplitter;
        PaneLayout* child_asPane;

        Child()
            : child_asSplitter(0), child_asPane(0)
        {
        }

        ~Child()
        {
            delete child_asSplitter;
            delete child_asPane;
        }
    };

    std::vector<Child*> children;

    SplitterSerialization()
        : sizes()
          , orientation(0)
          , children()
    {
    }

    ~SplitterSerialization()
    {
        for (std::vector<Child*>::iterator it = children.begin(); it != children.end(); ++it) {
            delete *it;
        }
    }

    /**
     * @brief Called prior to save only
     **/
    void initialize(Splitter* splitter);

    template<class Archive>
    void save(Archive & ar,
              const unsigned int /*version*/) const
    {
        ar & boost::serialization::make_nvp("Sizes",sizes);
        ar & boost::serialization::make_nvp("Orientation",orientation);

        assert(children.size() == 2);
        for (int i = 0; i < 2; ++i) {
            bool isChildSplitter = children[i]->child_asSplitter != NULL;
            ar & boost::serialization::make_nvp("ChildIsSplitter",isChildSplitter);
            if (isChildSplitter) {
                ar & boost::serialization::make_nvp("Child",*children[i]->child_asSplitter);
            } else {
                ar & boost::serialization::make_nvp("Child",*children[i]->child_asPane);
            }
        }
    }

    template<class Archive>
    void load(Archive & ar,
              const unsigned int /*version*/)
    {
        ar & boost::serialization::make_nvp("Sizes",sizes);
        ar & boost::serialization::make_nvp("Orientation",orientation);

        for (int i = 0; i < 2; ++i) {
            Child* c = new Child;
            bool isChildSplitter;
            ar & boost::serialization::make_nvp("ChildIsSplitter",isChildSplitter);
            if (isChildSplitter) {
                c->child_asSplitter = new SplitterSerialization;
                ar & boost::serialization::make_nvp("Child",*c->child_asSplitter);
            } else {
                c->child_asPane = new PaneLayout;
                ar & boost::serialization::make_nvp("Child",*c->child_asPane);
            }
            children.push_back(c);
        }
    }

    BOOST_SERIALIZATION_SPLIT_MEMBER()
};

BOOST_CLASS_VERSION(SplitterSerialization, SPLITTER_SERIALIZATION_VERSION)

/**
 * @brief The serialization of 1 window of the application
 * This doesn't apply to floating settings panels
 **/
class SerializableWindow;
struct ApplicationWindowSerialization
{
    //One of the 2 ptrs below is NULL.
    SplitterSerialization* child_asSplitter;
    PaneLayout* child_asPane;
    std::string child_asDockablePanel;

    //If true, then this is the main-window, otherwise it is considered as a floating window.
    bool isMainWindow;
    int x,y; //< the position of the window.
    int w,h; //< the size of the window

    ApplicationWindowSerialization()
        : child_asSplitter(0)
          , child_asPane(0)
          , isMainWindow(false)
          , x(0)
          , y(0)
          , w(0)
          , h(0)
    {
    }

    ~ApplicationWindowSerialization()
    {
        delete child_asSplitter;
        delete child_asPane;
    }

    ///Used prior to save only
    void initialize(bool isMainWindow,SerializableWindow* widget);

    template<class Archive>
    void save(Archive & ar,
              const unsigned int /*version*/) const
    {
        int childrenChoice;

        if (child_asSplitter) {
            childrenChoice = 0;
        } else if (child_asPane) {
            childrenChoice = 1;
        } else {
            childrenChoice = 2;
        }
        ar & boost::serialization::make_nvp("ChildType",childrenChoice);
        if (childrenChoice == 0) {
            ar & boost::serialization::make_nvp("Child",child_asSplitter);
        } else if (childrenChoice == 1) {
            ar & boost::serialization::make_nvp("Child",child_asPane);
        } else if (childrenChoice == 2) {
            ar & boost::serialization::make_nvp("Child",child_asDockablePanel);
        }
        ar & boost::serialization::make_nvp("MainWindow",isMainWindow);
        ar & boost::serialization::make_nvp("x",x);
        ar & boost::serialization::make_nvp("y",y);
        ar & boost::serialization::make_nvp("w",w);
        ar & boost::serialization::make_nvp("h",h);
    }

    template<class Archive>
    void load(Archive & ar,
              const unsigned int /*version*/)
    {
        int childType;
        ar & boost::serialization::make_nvp("ChildType",childType);

        if (childType == 0) {
            child_asSplitter = new SplitterSerialization;
            ar & boost::serialization::make_nvp("Child",child_asSplitter);
        } else if (childType == 1) {
            child_asPane = new PaneLayout;
            ar & boost::serialization::make_nvp("Child",child_asPane);
        }  else if (childType == 2) {
            ar & boost::serialization::make_nvp("Child",child_asDockablePanel);
        }
        ar & boost::serialization::make_nvp("MainWindow",isMainWindow);
        ar & boost::serialization::make_nvp("x",x);
        ar & boost::serialization::make_nvp("y",y);
        ar & boost::serialization::make_nvp("w",w);
        ar & boost::serialization::make_nvp("h",h);
    }

    BOOST_SERIALIZATION_SPLIT_MEMBER()
};

BOOST_CLASS_VERSION(ApplicationWindowSerialization, APPLICATION_WINDOW_SERIALIZATION_VERSION)


/**
 * @brief A class that encapsulates the layout of the application.
 **/
class GuiLayoutSerialization
{
public:

    GuiLayoutSerialization()
    {
    }

    ~GuiLayoutSerialization()
    {
        for (std::list<ApplicationWindowSerialization*>::iterator it = _windows.begin(); it != _windows.end(); ++it) {
            delete *it;
        }
    }

    ///Old members for compatibility up to PROJECT_GUI_EXERNALISE_GUI_LAYOUT
    std::map<std::string,PaneLayout> _layout;
    std::map<std::string,std::string> _splittersStates;

    ///New in GUI_LAYOUT_SERIALIZATION_MAJOR_OVERHAUL
    std::list<ApplicationWindowSerialization*> _windows;


    void initialize(Gui* gui);

    template<class Archive>
    void save(Archive & ar,
              const unsigned int /*version*/) const
    {
        int windowsCount = _windows.size();
        ar & boost::serialization::make_nvp("NbWindows",windowsCount);

        for (std::list<ApplicationWindowSerialization*>::const_iterator it = _windows.begin(); it != _windows.end(); ++it) {
            ar & boost::serialization::make_nvp("Window",**it);
        }
    }

    template<class Archive>
    void load(Archive & ar,
              const unsigned int version)
    {
        if (version < GUI_LAYOUT_SERIALIZATION_MAJOR_OVERHAUL) {
            ar & boost::serialization::make_nvp("Gui_Layout",_layout);
            ar & boost::serialization::make_nvp("Splitters_states",_splittersStates);
        } else {
            int windowsCount = _windows.size();
            ar & boost::serialization::make_nvp("NbWindows",windowsCount);
            for (int i = 0; i < windowsCount; ++i) {
                ApplicationWindowSerialization* newWindow = new ApplicationWindowSerialization;
                ar & boost::serialization::make_nvp("Window",*newWindow);
                _windows.push_back(newWindow);
            }
        }
    }

    BOOST_SERIALIZATION_SPLIT_MEMBER()
};

BOOST_CLASS_VERSION(GuiLayoutSerialization, GUI_LAYOUT_SERIALIZATION_VERSION);

class ProjectGuiSerialization
{
    ///All nodes gui data
    std::list< NodeGuiSerialization > _serializedNodes;

    ///The layout of the application
    GuiLayoutSerialization _layoutSerialization;

    ///Viewers data such as viewport and activated buttons
    std::map<std::string, ViewerData > _viewersData;

    ///Active histograms
    std::list<std::string> _histograms;

    ///Active backdrops (kept here for bw compatibility with Natron < 1.1
    std::list<NodeBackDropSerialization> _backdrops;
    
    ///All properties panels opened
    std::list<std::string> _openedPanelsOrdered;

    std::string _scriptEditorInput;
    
    std::list<boost::shared_ptr<PythonPanelSerialization> > _pythonPanels;
    
    ///The boost version passed to load(), this is not used on save
    unsigned int _version;

    friend class boost::serialization::access;

    template<class Archive>
    void save(Archive & ar,
              const unsigned int version) const
    {
        Q_UNUSED(version);
        ar & boost::serialization::make_nvp("NodesGui",_serializedNodes);
        ar & boost::serialization::make_nvp("Gui_Layout",_layoutSerialization);
        ar & boost::serialization::make_nvp("ViewersData",_viewersData);
        ar & boost::serialization::make_nvp("Histograms",_histograms);
        ar & boost::serialization::make_nvp("OpenedPanels",_openedPanelsOrdered);
        ar & boost::serialization::make_nvp("ScriptEditorInput",_scriptEditorInput);
        
        int numPyPanels = (int)_pythonPanels.size();
        ar & boost::serialization::make_nvp("NumPyPanels",numPyPanels);
        for (std::list<boost::shared_ptr<PythonPanelSerialization> >::const_iterator it = _pythonPanels.begin(); it != _pythonPanels.end(); ++it) {
            ar & boost::serialization::make_nvp("item",**it);
        }
    }

    template<class Archive>
    void load(Archive & ar,
              const unsigned int version)
    {
        Q_UNUSED(version);
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
        if (version >= PROJECT_GUI_INTRODUCES_BACKDROPS && version < PROJECT_GUI_SERIALIZATION_MERGE_BACKDROP) {
            ar & boost::serialization::make_nvp("Backdrops",_backdrops);
        }
        if (version >= PROJECT_GUI_INTRODUCES_PANELS) {
            ar & boost::serialization::make_nvp("OpenedPanels",_openedPanelsOrdered);
        }
        if (version >= PROJECT_GUI_SERIALIZATION_SCRIPT_EDITOR) {
            ar & boost::serialization::make_nvp("ScriptEditorInput",_scriptEditorInput);
        }
        
        if (version >= PROJECT_GUI_SERIALIZATION_INTRODUCES_PYTHON_PANELS) {
            int numPyPanels;
            ar & boost::serialization::make_nvp("NumPyPanels",numPyPanels);
            for (int i = 0; i < numPyPanels; ++i) {
                boost::shared_ptr<PythonPanelSerialization> s(new PythonPanelSerialization);
                ar & boost::serialization::make_nvp("item",*s);
                _pythonPanels.push_back(s);
            }
        }
        _version = version;
    }

public:

    ProjectGuiSerialization()
        : _serializedNodes()
          , _layoutSerialization()
          , _viewersData()
          , _histograms()
          , _backdrops()
          , _openedPanelsOrdered()
          , _version(0)
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
    
    const std::string& getInputScript() const
    {
        return _scriptEditorInput;
    }

    unsigned int getVersion() const
    {
        return _version;
    }
    
    const std::list<boost::shared_ptr<PythonPanelSerialization> >& getPythonPanels() const
    {
        return _pythonPanels;
    }

    BOOST_SERIALIZATION_SPLIT_MEMBER()
};

BOOST_CLASS_VERSION(ProjectGuiSerialization, PROJECT_GUI_SERIALIZATION_VERSION)


#endif // PROJECTGUISERIALIZATION_H
