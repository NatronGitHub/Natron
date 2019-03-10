/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#ifdef NATRON_BOOST_SERIALIZATION_COMPAT

#include "Serialization/RectDSerialization.h"
#include "Serialization/ProjectSerialization.h"
#include "Serialization/NodeGuiSerialization.h"
#include "Serialization/NodeBackdropSerialization.h"
#include "Serialization/WorkspaceSerialization.h"
#include "Serialization/SerializationCompat.h"
#include "Serialization/SerializationFwd.h"


SERIALIZATION_NAMESPACE_ENTER


#define VIEWER_DATA_INTRODUCES_WIPE_COMPOSITING 2
#define VIEWER_DATA_INTRODUCES_FRAME_RANGE 3
#define VIEWER_DATA_INTRODUCES_TOOLBARS_VISIBLITY 4
#define VIEWER_DATA_INTRODUCES_CHECKERBOARD 5
#define VIEWER_DATA_INTRODUCES_FPS 6
#define VIEWER_DATA_REMOVES_ASPECT_RATIO 7
#define VIEWER_DATA_INTRODUCES_FPS_LOCK 8
#define VIEWER_DATA_REMOVES_FRAME_RANGE_LOCK 9
#define VIEWER_DATA_INTRODUCES_GAMMA 10
#define VIEWER_DATA_INTRODUCES_ACTIVE_INPUTS 11
#define VIEWER_DATA_INTRODUCES_PAUSE_VIEWER 12
#define VIEWER_DATA_INTRODUCES_LAYER 13
#define VIEWER_DATA_INTRODUCES_FULL_FRAME_PROC 14
#define VIEWER_DATA_MOVE_TO_ENGINE 15
#define VIEWER_DATA_SERIALIZATION_VERSION VIEWER_DATA_MOVE_TO_ENGINE

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
#define PROJECT_GUI_SERIALIZATION_DEPRECATED 12
#define PROJECT_GUI_SERIALIZATION_VERSION PROJECT_GUI_SERIALIZATION_DEPRECATED

#define PANE_SERIALIZATION_INTRODUCES_CURRENT_TAB 2
#define PANE_SERIALIZATION_INTRODUCES_SIZE 3
#define PANE_SERIALIZATION_MAJOR_OVERHAUL 4
#define PANE_SERIALIZATION_INTRODUCE_SCRIPT_NAME 5
#define PANE_SERIALIZATION_VERSION PANE_SERIALIZATION_INTRODUCE_SCRIPT_NAME

#define SPLITTER_SERIALIZATION_VERSION 1
#define APPLICATION_WINDOW_SERIALIZATION_VERSION 1

#define GUI_LAYOUT_SERIALIZATION_MAJOR_OVERHAUL 2
#define GUI_LAYOUT_SERIALIZATION_VERSION GUI_LAYOUT_SERIALIZATION_MAJOR_OVERHAUL


struct ViewerData
{
    double zoomLeft;
    double zoomBottom;
    double zoomFactor;
    bool zoomOrPanSinceLastFit;
    unsigned int version;

    friend class ::boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar,
                   const unsigned int version_)
    {
        version = version_;

        ar & ::boost::serialization::make_nvp("zoomLeft", zoomLeft);
        ar & ::boost::serialization::make_nvp("zoomBottom", zoomBottom);
        ar & ::boost::serialization::make_nvp("zoomFactor", zoomFactor);
        if (version <  VIEWER_DATA_REMOVES_ASPECT_RATIO) {
            double zoomPar;
            ar & ::boost::serialization::make_nvp("zoomPAR", zoomPar);
        }

        if (version < VIEWER_DATA_MOVE_TO_ENGINE) {
            bool userRoIenabled;
            ar & ::boost::serialization::make_nvp("UserRoIEnabled", userRoIenabled);
            NATRON_NAMESPACE::RectD userRoI;
            ar & ::boost::serialization::make_nvp("UserRoI", userRoI);
            bool isClippedToProject;
            ar & ::boost::serialization::make_nvp("ClippedToProject", isClippedToProject);
            bool autoContrastEnabled;
            ar & ::boost::serialization::make_nvp("AutoContrast", autoContrastEnabled);
            double gain;
            ar & ::boost::serialization::make_nvp("Gain", gain);
            if (version >= VIEWER_DATA_INTRODUCES_GAMMA) {
                double gamma;
                ar & ::boost::serialization::make_nvp("Gain", gamma);
            }

            std::string colorSpace;
            ar & ::boost::serialization::make_nvp("ColorSpace", colorSpace);
            if (version >= VIEWER_DATA_INTRODUCES_LAYER) {
                std::string layerName,alphaLayerName;
                ar & ::boost::serialization::make_nvp("Layer", layerName);
                ar & ::boost::serialization::make_nvp("AlphaLayer", alphaLayerName);
            }
            std::string channels;
            ar & ::boost::serialization::make_nvp("Channels", channels);
            bool renderScaleActivated;
            ar & ::boost::serialization::make_nvp("RenderScaleActivated", renderScaleActivated);
            unsigned int mipMapLevel;
            ar & ::boost::serialization::make_nvp("MipMapLevel", mipMapLevel);
        }


        if (version >= VIEWER_DATA_INTRODUCES_WIPE_COMPOSITING) {
            ar & ::boost::serialization::make_nvp("ZoomOrPanSinceFit", zoomOrPanSinceLastFit);
            if (version < VIEWER_DATA_MOVE_TO_ENGINE) {
                int wipeCompositingOp;
                ar & ::boost::serialization::make_nvp("CompositingOP", wipeCompositingOp);
            }
        } else {
            zoomOrPanSinceLastFit = false;
        }
        if ( (version >= VIEWER_DATA_INTRODUCES_FRAME_RANGE) && (version < VIEWER_DATA_REMOVES_FRAME_RANGE_LOCK) ) {
            bool frameRangeLocked;
            ar & ::boost::serialization::make_nvp("FrameRangeLocked", frameRangeLocked);
        }
        if (version < VIEWER_DATA_MOVE_TO_ENGINE) {
            if (version >=  VIEWER_DATA_REMOVES_FRAME_RANGE_LOCK) {
                int leftBound, rightBound;
                ar & ::boost::serialization::make_nvp("LeftBound", leftBound);
                ar & ::boost::serialization::make_nvp("RightBound", rightBound);
            }
        }



        if (version >= VIEWER_DATA_INTRODUCES_TOOLBARS_VISIBLITY) {
            if (version < VIEWER_DATA_MOVE_TO_ENGINE) {
                bool leftToolbarVisible, rightToolbarVisible, topToolbarVisible, playerVisible, timelineVisible, infobarVisible;
                ar & ::boost::serialization::make_nvp("LeftToolbarVisible", leftToolbarVisible);
                ar & ::boost::serialization::make_nvp("RightToolbarVisible", rightToolbarVisible);
                ar & ::boost::serialization::make_nvp("TopToolbarVisible", topToolbarVisible);
                ar & ::boost::serialization::make_nvp("PlayerVisible", playerVisible);
                ar & ::boost::serialization::make_nvp("TimelineVisible", timelineVisible);
                ar & ::boost::serialization::make_nvp("InfobarVisible", infobarVisible);
            }
        }

        if (version >= VIEWER_DATA_INTRODUCES_PAUSE_VIEWER) {
            if (version < VIEWER_DATA_MOVE_TO_ENGINE) {
                bool isPauseEnabled[2];
                ar & ::boost::serialization::make_nvp("isInputAPaused", isPauseEnabled[0]);
                ar & ::boost::serialization::make_nvp("isInputBPaused", isPauseEnabled[1]);
            }
        }


        if (version < VIEWER_DATA_MOVE_TO_ENGINE) {
            if (version >= VIEWER_DATA_INTRODUCES_CHECKERBOARD) {
                bool checkerboardEnabled;
                ar & ::boost::serialization::make_nvp("CheckerboardEnabled", checkerboardEnabled);
            }
        }

        if (version < VIEWER_DATA_MOVE_TO_ENGINE) {
            if (version >= VIEWER_DATA_INTRODUCES_FPS) {
                double fps;
                ar & ::boost::serialization::make_nvp("Fps", fps);
            }

            if (version >= VIEWER_DATA_INTRODUCES_FPS_LOCK) {
                bool fpsLocked;
                ar & ::boost::serialization::make_nvp("FpsLocked", fpsLocked);
            }
        }
        if (version < VIEWER_DATA_MOVE_TO_ENGINE) {
            if (version >= VIEWER_DATA_INTRODUCES_ACTIVE_INPUTS) {
                int aChoice, bChoice;
                ar & ::boost::serialization::make_nvp("aInput", aChoice);
                ar & ::boost::serialization::make_nvp("bInput", bChoice);
            }

            if (version >= VIEWER_DATA_INTRODUCES_FULL_FRAME_PROC) {
                bool isFullFrameProcessEnabled;
                ar & ::boost::serialization::make_nvp("fullFrame", isFullFrameProcessEnabled);
            }
        }
    } // serialize
};



/**
 * @brief This is to keep compatibility until the version PANE_SERIALIZATION_INTRODUCES_SIZE of PaneLayout
 **/
struct PaneLayoutCompat_PANE_SERIALIZATION_INTRODUCES_SIZE
{
    bool floating;
    int posx, posy;
    int width, height;
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


    template<class Archive>
    void save(Archive & ar,
              const unsigned int /*version*/) const
    {
        throw std::runtime_error("Saving with boost is no longer supported");
    }

    template<class Archive>
    void load(Archive & ar,
              const unsigned int version)
    {
        if (version < PANE_SERIALIZATION_MAJOR_OVERHAUL) {
            PaneLayoutCompat_PANE_SERIALIZATION_INTRODUCES_SIZE compat1;
            ar & ::boost::serialization::make_nvp("Floating", compat1.floating);
            ar & ::boost::serialization::make_nvp("Splits", compat1.splits);
            ar & ::boost::serialization::make_nvp("ParentName", compat1.parentName);
            ar & ::boost::serialization::make_nvp("SplitsNames", compat1.splitsNames);
            ar & ::boost::serialization::make_nvp("Tabs", tabs);
            if (version >= PANE_SERIALIZATION_INTRODUCES_CURRENT_TAB) {
                ar & ::boost::serialization::make_nvp("Index", currentIndex);
            }
            if (version >= PANE_SERIALIZATION_INTRODUCES_SIZE) {
                if (compat1.floating) {
                    ar & ::boost::serialization::make_nvp("x", compat1.posx);
                    ar & ::boost::serialization::make_nvp("y", compat1.posy);
                    ar & ::boost::serialization::make_nvp("w", compat1.width);
                    ar & ::boost::serialization::make_nvp("h", compat1.height);
                }
            }
        } else {
            ar & ::boost::serialization::make_nvp("Tabs", tabs);
            ar & ::boost::serialization::make_nvp("Index", currentIndex);
            ar & ::boost::serialization::make_nvp("Name", name);
            ar & ::boost::serialization::make_nvp("IsAnchor", isAnchor);
        }
        if (version < PANE_SERIALIZATION_INTRODUCE_SCRIPT_NAME) {
            for (std::list<std::string>::iterator it = tabs.begin(); it != tabs.end(); ++it) {
                //Try to map the tab name to an old name
                if (*it == "CurveEditor") {
                    *it = kAnimationModuleEditorObjectName;
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

struct SplitterSerialization
{
    std::string sizes;
    int orientation; //< corresponds to enum OrientationEnum
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


    template<class Archive>
    void save(Archive & ar,
              const unsigned int /*version*/) const
    {
        throw std::runtime_error("Saving with boost is no longer supported");
    }

    template<class Archive>
    void load(Archive & ar,
              const unsigned int /*version*/)
    {
        ar & ::boost::serialization::make_nvp("Sizes", sizes);
        ar & ::boost::serialization::make_nvp("Orientation", orientation);

        for (int i = 0; i < 2; ++i) {
            Child* c = new Child;
            bool isChildSplitter;
            ar & ::boost::serialization::make_nvp("ChildIsSplitter", isChildSplitter);
            if (isChildSplitter) {
                c->child_asSplitter = new SplitterSerialization;
                ar & ::boost::serialization::make_nvp("Child", *c->child_asSplitter);
            } else {
                c->child_asPane = new PaneLayout;
                ar & ::boost::serialization::make_nvp("Child", *c->child_asPane);
            }
            children.push_back(c);
        }
    }

    BOOST_SERIALIZATION_SPLIT_MEMBER()
};


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
    int x, y; //< the position of the window.
    int w, h; //< the size of the window

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


    template<class Archive>
    void save(Archive & ar,
              const unsigned int /*version*/) const
    {
        throw std::runtime_error("Saving with boost is no longer supported");
    }

    template<class Archive>
    void load(Archive & ar,
              const unsigned int /*version*/)
    {
        int childType;
        ar & ::boost::serialization::make_nvp("ChildType", childType);

        if (childType == 0) {
            child_asSplitter = new SplitterSerialization;
            ar & ::boost::serialization::make_nvp("Child", child_asSplitter);
        } else if (childType == 1) {
            child_asPane = new PaneLayout;
            ar & ::boost::serialization::make_nvp("Child", child_asPane);
        }  else if (childType == 2) {
            ar & ::boost::serialization::make_nvp("Child", child_asDockablePanel);
        }
        ar & ::boost::serialization::make_nvp("MainWindow", isMainWindow);
        ar & ::boost::serialization::make_nvp("x", x);
        ar & ::boost::serialization::make_nvp("y", y);
        ar & ::boost::serialization::make_nvp("w", w);
        ar & ::boost::serialization::make_nvp("h", h);
    }

    BOOST_SERIALIZATION_SPLIT_MEMBER()
};


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
    std::map<std::string, PaneLayout> _layout;
    std::map<std::string, std::string> _splittersStates;

    ///New in GUI_LAYOUT_SERIALIZATION_MAJOR_OVERHAUL
    std::list<ApplicationWindowSerialization*> _windows;

    template<class Archive>
    void save(Archive & ar,
              const unsigned int /*version*/) const
    {
        throw std::runtime_error("Saving with boost is no longer supported");
    }

    template<class Archive>
    void load(Archive & ar,
              const unsigned int version)
    {
        if (version < GUI_LAYOUT_SERIALIZATION_MAJOR_OVERHAUL) {
            ar & ::boost::serialization::make_nvp("Gui_Layout", _layout);
            ar & ::boost::serialization::make_nvp("Splitters_states", _splittersStates);
        } else {
            int windowsCount = _windows.size();
            ar & ::boost::serialization::make_nvp("NbWindows", windowsCount);
            for (int i = 0; i < windowsCount; ++i) {
                ApplicationWindowSerialization* newWindow = new ApplicationWindowSerialization;
                ar & ::boost::serialization::make_nvp("Window", *newWindow);
                _windows.push_back(newWindow);
            }
        }
    }

    BOOST_SERIALIZATION_SPLIT_MEMBER()
};


class ProjectGuiSerialization
{
public:
    // Deprecated: Nodes UI data prior to Natron 2.2
    std::list<NodeGuiSerialization> _serializedNodes;

    // The layout of the application windows
    GuiLayoutSerialization _layoutSerialization;

    // Viewports data
    std::map<std::string, ViewerData > _viewersData;

    // Created histograms
    std::list<std::string> _histograms;

    // Active backdrops (kept here for bw compatibility with Natron < 1.1
    std::list<NodeBackdropSerialization> _backdrops;

    // All properties panels opened
    std::list<std::string> _openedPanelsOrdered;

    // List of python panels
    std::list<PythonPanelSerialization > _pythonPanels;

    // The boost version passed to load(), this is not used on save
    unsigned int _version;


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
    }



    template<class Archive>
    void save(Archive & ar,
              const unsigned int /*version*/) const
    {
        throw std::runtime_error("Saving with boost is no longer supported");
    }

    template<class Archive>
    void load(Archive & ar,
              const unsigned int version)
    {
        Q_UNUSED(version);
        ar & ::boost::serialization::make_nvp("NodesGui", _serializedNodes);
        if (version < PROJECT_GUI_EXERNALISE_GUI_LAYOUT) {
            ar & ::boost::serialization::make_nvp("Gui_Layout", _layoutSerialization._layout);
            ar & ::boost::serialization::make_nvp("Splitters_states", _layoutSerialization._splittersStates);
            if (version < PROJECT_GUI_CHANGES_SPLITTERS) {
                _layoutSerialization._splittersStates.clear();
            }
        } else {
            ar & ::boost::serialization::make_nvp("Gui_Layout", _layoutSerialization);
        }

        ar & ::boost::serialization::make_nvp("ViewersData", _viewersData);
        if (version < PROJECT_GUI_REMOVES_ALL_NODE_PREVIEW_TOGGLED) {
            bool tmp = false;
            ar & ::boost::serialization::make_nvp("PreviewsTurnedOffGlobaly", tmp);
        }
        ar & ::boost::serialization::make_nvp("Histograms", _histograms);
        if ( (version >= PROJECT_GUI_INTRODUCES_BACKDROPS) && (version < PROJECT_GUI_SERIALIZATION_MERGE_BACKDROP) ) {
            ar & ::boost::serialization::make_nvp("Backdrops", _backdrops);
        }
        if (version >= PROJECT_GUI_INTRODUCES_PANELS) {
            ar & ::boost::serialization::make_nvp("OpenedPanels", _openedPanelsOrdered);
        }
        if (version >= PROJECT_GUI_SERIALIZATION_SCRIPT_EDITOR && version < PROJECT_GUI_SERIALIZATION_DEPRECATED) {
            std::string input;
            ar & ::boost::serialization::make_nvp("ScriptEditorInput", input);
        }

        if (version >= PROJECT_GUI_SERIALIZATION_INTRODUCES_PYTHON_PANELS) {
            int numPyPanels;
            ar & ::boost::serialization::make_nvp("NumPyPanels", numPyPanels);
            for (int i = 0; i < numPyPanels; ++i) {
                PythonPanelSerialization s;
                ar & ::boost::serialization::make_nvp("item", s);
                _pythonPanels.push_back(s);
            }
        }
        _version = version;
    }



    BOOST_SERIALIZATION_SPLIT_MEMBER()
};


SERIALIZATION_NAMESPACE_EXIT


BOOST_CLASS_VERSION(SERIALIZATION_NAMESPACE::GuiLayoutSerialization, GUI_LAYOUT_SERIALIZATION_VERSION);
BOOST_CLASS_VERSION(SERIALIZATION_NAMESPACE::ApplicationWindowSerialization, APPLICATION_WINDOW_SERIALIZATION_VERSION)
BOOST_CLASS_VERSION(SERIALIZATION_NAMESPACE::SplitterSerialization, SPLITTER_SERIALIZATION_VERSION)
BOOST_CLASS_VERSION(SERIALIZATION_NAMESPACE::PaneLayout, PANE_SERIALIZATION_VERSION)
BOOST_CLASS_VERSION(SERIALIZATION_NAMESPACE::PythonPanelSerialization, 1)
BOOST_CLASS_VERSION(SERIALIZATION_NAMESPACE::ViewerData, VIEWER_DATA_SERIALIZATION_VERSION)
BOOST_CLASS_VERSION(SERIALIZATION_NAMESPACE::ProjectGuiSerialization, PROJECT_GUI_SERIALIZATION_VERSION)

#endif // #ifdef NATRON_BOOST_SERIALIZATION_COMPAT

#endif // PROJECTGUISERIALIZATION_H
