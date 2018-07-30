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

#ifndef WORKSPACESERIALIZATION_H
#define WORKSPACESERIALIZATION_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#endif

#include "Serialization/SerializationBase.h"
#include "Serialization/SerializationFwd.h"

#define kSplitterOrientationHorizontal "Horizontal"
#define kSplitterOrientationVertical "Vertical"

#define kSplitterChildTypeSplitter "Splitter"
#define kSplitterChildTypeTabWidget "TabWidget"
#define kSplitterChildTypeSettingsPanel "SettingsPanel"

SERIALIZATION_NAMESPACE_ENTER

/**
 * @brief Data used for each viewport to restore the OpenGL orthographic projection correctly
 **/
class ViewportData
: public SerializationObjectBase
{
public:


    double left;
    double bottom;
    double zoomFactor;
    double par;
    bool zoomOrPanSinceLastFit;

    ViewportData()
    : SerializationObjectBase()
    , left(0)
    , bottom(0)
    , zoomFactor(0)
    , par(0)
    , zoomOrPanSinceLastFit(false)
    {

    }

    virtual ~ViewportData()
    {

    }


    virtual void encode(YAML::Emitter& em) const OVERRIDE;

    virtual void decode(const YAML::Node& node) OVERRIDE;



};

/**
 * @brief Serialization for user custom Python panels
 **/
class PythonPanelSerialization
: public SerializationObjectBase
{
public:

    // Serialization of user knobs
    KnobSerializationList knobs;

    // label of the python panel
    std::string name;

    // The python function used to create it
    std::string pythonFunction;

    // Custom user data serialization
    std::string userData;


    PythonPanelSerialization()
    : SerializationObjectBase()
    , knobs()
    , name()
    , pythonFunction()
    , userData()
    {

    }

    virtual ~PythonPanelSerialization()
    {

    }

    virtual void encode(YAML::Emitter& em) const OVERRIDE;

    virtual void decode(const YAML::Node& node) OVERRIDE;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);


};

enum ProjectWorkspaceWidgetTypeEnum
{
    eProjectWorkspaceWidgetTypeTabWidget,
    eProjectWorkspaceWidgetTypeSplitter,
    eProjectWorkspaceWidgetTypeSettingsPanel,
    eProjectWorkspaceWidgetTypeNone
};

class TabWidgetSerialization
: public SerializationObjectBase
{
public:

    // Script-name of the ordered tabs. For GroupNode NodeGraph this is the fully qualified name of the group node
    std::list<std::string> tabs;

    // The current index in the tab widget
    int currentIndex;

    // If true, then this tab widget will be the "main" tab widget so it will receive new user created viewers etc...
    bool isAnchor;

    // Script-name of the tab widget
    std::string scriptName;

    TabWidgetSerialization()
    : SerializationObjectBase()
    , tabs()
    , currentIndex(-1)
    , isAnchor(false)
    , scriptName()
    {

    }

    virtual ~TabWidgetSerialization()
    {

    }

    virtual void encode(YAML::Emitter& em) const OVERRIDE;

    virtual void decode(const YAML::Node& node) OVERRIDE;

};

/**
 * @brief Serialization of splitters used to split the user interface in panels
 **/
class WidgetSplitterSerialization
: public SerializationObjectBase
{
public:
    // Corresponds to enum Qt::OrientationEnum
    std::string orientation;

    // The width or height (depending on orientation) of the left child
    int leftChildSize;

    // The width or height (depending on orientation) of the right child
    int rightChildSize;

    struct Child
    {
        boost::shared_ptr<WidgetSplitterSerialization> childIsSplitter;
        boost::shared_ptr<TabWidgetSerialization> childIsTabWidget;
        std::string type;
    };

    // The 2 children of the splitter
    Child leftChild, rightChild;

    WidgetSplitterSerialization()
    : SerializationObjectBase()
    , orientation()
    , leftChildSize(0)
    , rightChildSize(0)
    , leftChild()
    , rightChild()
    {
    }

    virtual ~WidgetSplitterSerialization()
    {

    }

    virtual void encode(YAML::Emitter& em) const OVERRIDE;

    virtual void decode(const YAML::Node& node) OVERRIDE;

};


class WindowSerialization
: public SerializationObjectBase
{

public:

    // Used to identify what is inside the window
    std::string childType;

    // If the main-child of the window is a splitter, this is its serialization
    boost::shared_ptr<WidgetSplitterSerialization> isChildSplitter;

    // If the main-child of the window is a tabwidget, this is its serialization
    boost::shared_ptr<TabWidgetSerialization> isChildTabWidget;

    // If the main-child of the window is a settings-panel, this is the fully qualified name
    // of the node, or kNatronProjectSettingsPanelSerializationName for the project settings panel
    std::string isChildSettingsPanel;

    // the position (x,y) of the window in pixel coordinates
    int windowPosition[2];

    // the size (width, height) of the window in pixels
    int windowSize[2];

    WindowSerialization()
    : SerializationObjectBase()
    , isChildSplitter()
    , isChildTabWidget()
    , isChildSettingsPanel()
    , windowPosition()
    , windowSize()
    {
    }

    virtual ~WindowSerialization()
    {


    }
    
    virtual void encode(YAML::Emitter& em) const OVERRIDE;

    virtual void decode(const YAML::Node& node) OVERRIDE;

};


/**
 * @brief The workspace used by the user in the project
 **/
class WorkspaceSerialization
: public SerializationObjectBase
{

public:

    // Created histograms
    std::list<std::string> _histograms;

    // List of python panels
    std::list<PythonPanelSerialization> _pythonPanels;

    // The main-window serialization
    boost::shared_ptr<WindowSerialization> _mainWindowSerialization;

    // All indpendent windows in the Project besides the main window
    std::list<boost::shared_ptr<WindowSerialization> > _floatingWindowsSerialization;

    WorkspaceSerialization()
    : SerializationObjectBase()
    , _histograms()
    , _pythonPanels()
    , _mainWindowSerialization()
    , _floatingWindowsSerialization()
    {
    }

    virtual ~WorkspaceSerialization()
    {
        
    }


    virtual void encode(YAML::Emitter& em) const OVERRIDE;

    virtual void decode(const YAML::Node& node) OVERRIDE;

};


SERIALIZATION_NAMESPACE_EXIT


#endif // WORKSPACESERIALIZATION_H
