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

#ifndef PROJECTSERIALIZATION_H
#define PROJECTSERIALIZATION_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <stdexcept>

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
#include <boost/serialization/scoped_ptr.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/version.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
GCC_DIAG_ON(unused-parameter)
#endif

#include "Global/GitVersion.h"
#include "Global/GlobalDefines.h"
#include "Global/MemoryInfo.h"
#include "Engine/AppInstance.h"
#include "Engine/FormatSerialization.h"
#include "Engine/ProjectPrivate.h"
#include "Engine/Project.h"
#include "Engine/TimeLine.h"
#include "Engine/Node.h"
#include "Engine/NodeSerialization.h"
#include "Engine/NodeGroupSerialization.h"
#include "Engine/KnobSerialization.h"
#include "Engine/EngineFwd.h"



#define PYTHON_PANEL_SERIALIZATION_VERSION 1


#define PROJECT_SERIALIZATION_INTRODUCES_NATRON_VERSION 2
#define PROJECT_SERIALIZATION_REMOVES_NODE_COUNTERS 3
#define PROJECT_SERIALIZATION_REMOVES_TIMELINE_BOUNDS 4
#define PROJECT_SERIALIZATION_INTRODUCES_GROUPS 5
#define PROJECT_SERIALIZATION_CHANGE_VERSION_SERIALIZATION 6
#define PROJECT_SERIALIZATION_CHANGE_FORMAT_SERIALIZATION 7
#define PROJECT_SERIALIZATION_CHANGE_NODE_COLLECTION_SERIALIZATION 8
#define PROJECT_SERIALIZATION_DEPRECATE_PROJECT_GUI 9
#define PROJECT_SERIALIZATION_VERSION PROJECT_SERIALIZATION_DEPRECATE_PROJECT_GUI

NATRON_NAMESPACE_ENTER;

/**
 * @brief Data used for each viewport to restore the OpenGL orthographic projection correctly
 **/
struct ViewportData
{
    double left;
    double bottom;
    double zoomFactor;
    double par;
    bool zoomOrPanSinceLastFit;

    template<class Archive>
    void serialize(Archive & ar,
                   const unsigned int /*version*/)
    {
        ar & ::boost::serialization::make_nvp("left", left);
        ar & ::boost::serialization::make_nvp("bottom", bottom);
        ar & ::boost::serialization::make_nvp("zoomFactor", zoomFactor);
        ar & ::boost::serialization::make_nvp("Par", par);
        ar & ::boost::serialization::make_nvp("UserEdited", zoomOrPanSinceLastFit);

    }
};

/**
 * @brief Serialization for user custom Python panels
 **/
struct PythonPanelSerialization : public SerializationObjectBase
{
    // Serialization of user knobs
    std::list<KnobSerializationPtr> knobs;

    // label of the python panel
    std::string name;

    // The python function used to create it
    std::string pythonFunction;

    // Custom user data serialization
    std::string userData;

    template<class Archive>
    void save(Archive & ar,
              const unsigned int /*version*/) const
    {
        ar & ::boost::serialization::make_nvp("Label", name);
        ar & ::boost::serialization::make_nvp("PythonFunction", pythonFunction);
        int nKnobs = knobs.size();
        ar & ::boost::serialization::make_nvp("NumParams", nKnobs);

        for (std::list<KnobSerializationPtr>::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
            ar & ::boost::serialization::make_nvp("item", **it);
        }

        ar & ::boost::serialization::make_nvp("UserData", userData);
    }

    template<class Archive>
    void load(Archive & ar,
              const unsigned int /*version*/)
    {
        ar & ::boost::serialization::make_nvp("Label", name);
        ar & ::boost::serialization::make_nvp("PythonFunction", pythonFunction);
        int nKnobs;
        ar & ::boost::serialization::make_nvp("NumParams", nKnobs);

        for (int i = 0; i < nKnobs; ++i) {
            KnobSerializationPtr k(new KnobSerialization);
            ar & ::boost::serialization::make_nvp("item", *k);
            knobs.push_back(k);
        }

        ar & ::boost::serialization::make_nvp("UserData", userData);
    }
    
    BOOST_SERIALIZATION_SPLIT_MEMBER()
};

enum ProjectWorkspaceWidgetTypeEnum
{
    eProjectWorkspaceWidgetTypeTabWidget,
    eProjectWorkspaceWidgetTypeSplitter,
    eProjectWorkspaceWidgetTypeSettingsPanel
};

struct ProjectTabWidgetSerialization  : public SerializationObjectBase
{
    // Script-name of the ordered tabs. For GroupNode NodeGraph this is the fully qualified name of the group node
    std::list<std::string> tabs;

    // The current index in the tab widget
    int currentIndex;

    // If true, then this tab widget will be the "main" tab widget so it will receive new user created viewers etc...
    bool isAnchor;

    // Script-name of the tab widget
    std::string scriptName;

    template<class Archive>
    void save(Archive & ar,
              const unsigned int /*version*/) const
    {
        ar & ::boost::serialization::make_nvp("Tabs", tabs);
        ar & ::boost::serialization::make_nvp("Index", currentIndex);
        ar & ::boost::serialization::make_nvp("ScriptName", scriptName);
        ar & ::boost::serialization::make_nvp("IsAnchor", isAnchor);
    }

    template<class Archive>
    void load(Archive & ar,
              const unsigned int /*version*/)
    {
        ar & ::boost::serialization::make_nvp("Tabs", tabs);
        ar & ::boost::serialization::make_nvp("Index", currentIndex);
        ar & ::boost::serialization::make_nvp("ScriptName", scriptName);
        ar & ::boost::serialization::make_nvp("IsAnchor", isAnchor);
    }

    BOOST_SERIALIZATION_SPLIT_MEMBER()
};

/**
 * @brief Serialization of splitters used to split the user interface in panels
 **/
struct ProjectWindowSplitterSerialization : public SerializationObjectBase
{

    // Corresponds to enum Qt::OrientationEnum
    int orientation;

    // The width or height (depending on orientation) of the left child
    int leftChildSize;

    // The width or height (depending on orientation) of the right child
    int rightChildSize;

    struct Child
    {
        boost::shared_ptr<ProjectWindowSplitterSerialization> childIsSplitter;
        boost::shared_ptr<ProjectTabWidgetSerialization> childIsTabWidget;
        ProjectWorkspaceWidgetTypeEnum type;
    };

    // The 2 children of the splitter
    boost::shared_ptr<Child> leftChild, rightChild;

    ProjectWindowSplitterSerialization()
    : orientation(0)
    , leftChildSize(0)
    , rightChildSize(0)
    , leftChild()
    , rightChild()
    {
    }

    

    template<class Archive>
    void save(Archive & ar,
              const unsigned int /*version*/) const
    {

        ar & ::boost::serialization::make_nvp("Orientation", orientation);
        ar & ::boost::serialization::make_nvp("LeftSize", leftChildSize);
        ar & ::boost::serialization::make_nvp("RightSize", rightChildSize);
        ar & ::boost::serialization::make_nvp("LeftChildType", leftChild->type);
        switch (leftChild->type) {
            case eProjectWorkspaceWidgetTypeSplitter:
                ar & ::boost::serialization::make_nvp("LeftChild", *leftChild->childIsSplitter);
                break;
            case eProjectWorkspaceWidgetTypeTabWidget:
                ar & ::boost::serialization::make_nvp("LeftChild", *leftChild->childIsTabWidget);
                break;
            default:
                assert(false);
                break;
        }
        ar & ::boost::serialization::make_nvp("RightChildType", rightChild->type);
        switch (leftChild->type) {
            case eProjectWorkspaceWidgetTypeSplitter:
                ar & ::boost::serialization::make_nvp("RightChild", *rightChild->childIsSplitter);
                break;
            case eProjectWorkspaceWidgetTypeTabWidget:
                ar & ::boost::serialization::make_nvp("RightChild", *rightChild->childIsTabWidget);
                break;
            default:
                assert(false);
                break;
        }

    }

    template<class Archive>
    void load(Archive & ar,
              const unsigned int /*version*/)
    {
        ar & ::boost::serialization::make_nvp("Orientation", orientation);
        ar & ::boost::serialization::make_nvp("LeftSize", leftChildSize);
        ar & ::boost::serialization::make_nvp("RightSize", rightChildSize);
        leftChild.reset(new Child);
        ar & ::boost::serialization::make_nvp("LeftChildType", leftChild->type);
        switch (leftChild->type) {
            case eProjectWorkspaceWidgetTypeSplitter:
                leftChild->childIsSplitter.reset(new ProjectWindowSplitterSerialization);
                ar & ::boost::serialization::make_nvp("LeftChild", *leftChild->childIsSplitter);
                break;
            case eProjectWorkspaceWidgetTypeTabWidget:
                leftChild->childIsTabWidget.reset(new ProjectTabWidgetSerialization);
                ar & ::boost::serialization::make_nvp("LeftChild", *leftChild->childIsTabWidget);
                break;
            default:
                assert(false);
                break;
        }
        rightChild.reset(new Child);
        ar & ::boost::serialization::make_nvp("RightChildType", rightChild->type);
        switch (leftChild->type) {
            case eProjectWorkspaceWidgetTypeSplitter:
                rightChild->childIsSplitter.reset(new ProjectWindowSplitterSerialization);
                ar & ::boost::serialization::make_nvp("RightChild", *rightChild->childIsSplitter);
                break;
            case eProjectWorkspaceWidgetTypeTabWidget:
                rightChild->childIsTabWidget.reset(new ProjectTabWidgetSerialization);
                ar & ::boost::serialization::make_nvp("RightChild", *rightChild->childIsTabWidget);
                break;
            default:
                assert(false);
                break;
        }

    }

    BOOST_SERIALIZATION_SPLIT_MEMBER()
};


struct ProjectWindowSerialization : public SerializationObjectBase
{
    // Used to identify what is inside the window
    ProjectWorkspaceWidgetTypeEnum childType;

    // If the main-child of the window is a splitter, this is its serialization
    boost::shared_ptr<ProjectWindowSplitterSerialization> isChildSplitter;

    // If the main-child of the window is a tabwidget, this is its serialization
    boost::shared_ptr<ProjectTabWidgetSerialization> isChildTabWidget;

    // If the main-child of the window is a settings-panel, this is the fully qualified name
    // of the node, or kNatronProjectSettingsPanelSerializationName for the project settings panel
    std::string isChildSettingsPanel;

    // the position (x,y) of the window in pixel coordinates
    int windowPosition[2];

    // the size (width, height) of the window in pixels
    int windowSize[2];

    ProjectWindowSerialization()
    : isChildSplitter()
    , isChildTabWidget()
    , isChildSettingsPanel()
    , windowPosition()
    , windowSize()
    {
    }


    template<class Archive>
    void save(Archive & ar,
              const unsigned int /*version*/) const
    {

        ar & ::boost::serialization::make_nvp("ChildType", childType);
        switch (childType) {
            case eProjectWorkspaceWidgetTypeSplitter:
                assert(isChildSplitter);
                ar & ::boost::serialization::make_nvp("Child", *isChildSplitter);
                break;
            case eProjectWorkspaceWidgetTypeTabWidget:
                assert(isChildTabWidget);
                ar & ::boost::serialization::make_nvp("Child", *isChildTabWidget);
                break;
            case eProjectWorkspaceWidgetTypeSettingsPanel:
                assert(!isChildSettingsPanel.empty());
                ar & ::boost::serialization::make_nvp("Child", isChildSettingsPanel);
                break;
        }

        ar & ::boost::serialization::make_nvp("x", windowPosition[0]);
        ar & ::boost::serialization::make_nvp("y", windowPosition[1]);
        ar & ::boost::serialization::make_nvp("w", windowSize[0]);
        ar & ::boost::serialization::make_nvp("h", windowSize[1]);
    }

    template<class Archive>
    void load(Archive & ar,
              const unsigned int /*version*/)
    {
        ar & ::boost::serialization::make_nvp("ChildType", childType);
        switch (childType) {
            case eProjectWorkspaceWidgetTypeSplitter:
                isChildSplitter.reset(new ProjectWindowSplitterSerialization);
                ar & ::boost::serialization::make_nvp("Child", *isChildSplitter);
                break;
            case eProjectWorkspaceWidgetTypeTabWidget:
                isChildTabWidget.reset(new ProjectTabWidgetSerialization);
                ar & ::boost::serialization::make_nvp("Child", *isChildTabWidget);
                break;
            case eProjectWorkspaceWidgetTypeSettingsPanel:
                ar & ::boost::serialization::make_nvp("Child", isChildSettingsPanel);
                break;
        }
        ar & ::boost::serialization::make_nvp("x", windowPosition[0]);
        ar & ::boost::serialization::make_nvp("y", windowPosition[1]);
        ar & ::boost::serialization::make_nvp("w", windowSize[0]);
        ar & ::boost::serialization::make_nvp("h", windowSize[1]);

    }

    BOOST_SERIALIZATION_SPLIT_MEMBER()
};


/**
 * @brief The workspace used by the user in the project
 **/
struct ProjectWorkspaceSerialization
{


    // Created histograms
    std::list<std::string> _histograms;

    // List of python panels
    std::list<boost::shared_ptr<PythonPanelSerialization> > _pythonPanels;

    // The main-window serialization
    boost::shared_ptr<ProjectWindowSerialization> _mainWindowSerialization;

    // All indpendent windows in the Project besides the main window
    std::list<boost::shared_ptr<ProjectWindowSerialization> > _floatingWindowsSerialization;

    template<class Archive>
    void save(Archive & ar,
              const unsigned int /*version*/) const
    {

        ar & ::boost::serialization::make_nvp("Histograms", _histograms);

        ar & ::boost::serialization::make_nvp("MainWindow", *_mainWindowSerialization);

        int windowsCount = _floatingWindowsSerialization.size();
        ar & ::boost::serialization::make_nvp("nWindows", windowsCount);
        for (std::list<boost::shared_ptr<ProjectWindowSerialization> >::const_iterator it = _floatingWindowsSerialization.begin(); it != _floatingWindowsSerialization.end(); ++it) {
            ar & ::boost::serialization::make_nvp("Window", **it);
        }

        int nPythonPanels = (int)_pythonPanels.size();
        ar & ::boost::serialization::make_nvp("nPythonPanels", nPythonPanels);
        for (std::list<boost::shared_ptr<PythonPanelSerialization> >::const_iterator it = _pythonPanels.begin(); it!=_pythonPanels.end(); ++it) {
            ar & ::boost::serialization::make_nvp("Panel", **it);
        }
    }

    template<class Archive>
    void load(Archive & ar,
              const unsigned int /*version*/)
    {



        ar & ::boost::serialization::make_nvp("Histograms", _histograms);


        _mainWindowSerialization.reset(new ProjectWindowSerialization);
        ar & ::boost::serialization::make_nvp("MainWindow", *_mainWindowSerialization);

        int windowsCount;
        ar & ::boost::serialization::make_nvp("nWindows", windowsCount);
        for (int i = 0; i < windowsCount; ++i) {
            boost::shared_ptr<ProjectWindowSerialization> window(new ProjectWindowSerialization);
            ar & ::boost::serialization::make_nvp("Window", *window);
            _floatingWindowsSerialization.push_back(window);
        }

        int nPythonPanels;
        ar & ::boost::serialization::make_nvp("nPythonPanels", nPythonPanels);
        for (int i = 0; i < nPythonPanels; ++i) {
            boost::shared_ptr<PythonPanelSerialization> panel(new PythonPanelSerialization);
            ar & ::boost::serialization::make_nvp("Panel", *panel);
            _pythonPanels.push_back(panel);
        }
    }

    BOOST_SERIALIZATION_SPLIT_MEMBER()
};

/**
 * @brief Informations related to the version of Natron on which the project was saved
 **/
class ProjectBeingLoadedInfo
{
public:

    // Software version bits
    int vMajor,vMinor,vRev;

    // Git infos: branch name and commit hash
    std::string gitBranch,gitCommit;

    // Operating system string: "Windows", "MacOSX" or "Linux"
    std::string osStr;

    // Is it 64 or 32 bit version of Natron?
    int bits;
};

class ProjectSerialization : public SerializationObjectBase
{
public:

    
    // The collection of nodes in the project
    std::list<NodeSerializationPtr>  _nodes;

    // Additional user added formats
    std::list<FormatSerialization> _additionalFormats;

    // The project settings serialization
    std::list<KnobSerializationPtr> _projectKnobs;

    // The timeline current frame
    int _timelineCurrent;

    // The project creation date, obtained with QDateTime::toMSecsSinceEpoch();
    long long _creationDate;

    // The serialization version of the project
    unsigned int _version;

    // Infos related to Natron version in the project being loaded
    ProjectBeingLoadedInfo _projectLoadedInfo;

    // Fully qualified name of nodes which have their properties panel opened or kNatronProjectSettingsPanelSerializationName
    std::list<std::string> _openedPanelsOrdered;

    // The project workspace serialization
    boost::shared_ptr<ProjectWorkspaceSerialization> _projectWorkspace;

    // For each viewport, its projection. They are identified by their script-name
    std::map<std::string, ViewportData> _viewportsData;

    ProjectSerialization(const ProjectPtr& app = ProjectPtr());

    ~ProjectSerialization()
    {
    }

    unsigned int getVersion() const
    {
        return _version;
    }

    const ProjectBeingLoadedInfo& getProjectBeingLoadedInfo() const
    {
        return _projectLoadedInfo;
    }


    int getCurrentTime() const
    {
        return _timelineCurrent;
    }

    const std::list< KnobSerializationPtr  > & getProjectKnobsValues() const
    {
        return _projectKnobs;
    }

    const std::list<FormatSerialization> & getAdditionalFormats() const
    {
        return _additionalFormats;
    }

    const std::list<NodeSerializationPtr> & getNodesSerialization() const
    {
        return _nodes;
    }

    long long getCreationDate() const
    {
        return _creationDate;
    }

private:


    friend class ::boost::serialization::access;
    template<class Archive>
    void save(Archive & ar,
              const unsigned int /*version*/) const
    {
        int vMajor = NATRON_VERSION_MAJOR;
        int vMinor = NATRON_VERSION_MINOR;
        int vRev = NATRON_VERSION_REVISION;
        ar & ::boost::serialization::make_nvp("VersionMajor", vMajor);
        ar & ::boost::serialization::make_nvp("VersionMinor", vMinor);
        ar & ::boost::serialization::make_nvp("VersionRev", vRev);
        std::string gitBranchStr(GIT_BRANCH);
        ar & ::boost::serialization::make_nvp("GitBranch", gitBranchStr);

        std::string gitCommitStr(GIT_COMMIT);
        ar & ::boost::serialization::make_nvp("GitCommit", gitCommitStr);

        std::string osString;
#ifdef __NATRON_WIN32__
        osString.append("Windows");
#elif defined(__NATRON_OSX__)
        osString.append("MacOSX");
#elif defined(__NATRON_LINUX__)
        osString.append("Linux");
#endif
        ar & ::boost::serialization::make_nvp("OS", osString);

        int bits = isApplication32Bits() ? 32 : 64;
        ar & ::boost::serialization::make_nvp("Bits", bits);

        int nodesCount = (int)_nodes.size();
        ar & ::boost::serialization::make_nvp("NodesCount", nodesCount);
        for (std::list<NodeSerializationPtr>::const_iterator it = _nodes.begin(); it!=_nodes.end(); ++it) {
            ar & ::boost::serialization::make_nvp("Node", **it);
        }


        int knobsCount = (int)_projectKnobs.size();
        ar & ::boost::serialization::make_nvp("ProjectKnobsCount", knobsCount);
        for (std::list<KnobSerializationPtr>::const_iterator it = _projectKnobs.begin();
             it != _projectKnobs.end();
             ++it) {
            ar & ::boost::serialization::make_nvp( "item", *(*it) );
        }
        ar & ::boost::serialization::make_nvp("AdditionalFormats", _additionalFormats);
        ar & ::boost::serialization::make_nvp("Timeline_current_time", _timelineCurrent);
        ar & ::boost::serialization::make_nvp("CreationDate", _creationDate);


        ar & ::boost::serialization::make_nvp("Workspace", *_projectWorkspace);

        int nViewports = _viewportsData.size();
        ar & ::boost::serialization::make_nvp("nViewports", nViewports);
        for (std::map<std::string, ViewportData>::const_iterator it = _viewportsData.begin(); it!=_viewportsData.end(); ++it) {
            ar & ::boost::serialization::make_nvp("ScriptName", it->first);
            ar & ::boost::serialization::make_nvp("Data", it->second);
        }

        ar & ::boost::serialization::make_nvp("OpenedPanels", _openedPanelsOrdered);
    }

    template<class Archive>
    void load(Archive & ar,
              const unsigned int version)
    {
        _version = version;

        // Dead serialization code
        if (version >= PROJECT_SERIALIZATION_INTRODUCES_NATRON_VERSION && version < PROJECT_SERIALIZATION_CHANGE_VERSION_SERIALIZATION) {
            std::string natronVersion;
            ar & ::boost::serialization::make_nvp("NatronVersion", natronVersion);
            std::string toFind(NATRON_APPLICATION_NAME " v");
            std::size_t foundV = natronVersion.find(toFind);
            if (foundV != std::string::npos) {
                foundV += toFind.size();
                toFind = " from git branch";
                std::size_t foundFrom = natronVersion.find(toFind);
                if (foundFrom != std::string::npos) {
                    std::string vStr;
                    for (std::size_t i = foundV; i < foundFrom; ++i) {
                        vStr.push_back(natronVersion[i]);
                    }
                    QStringList splits = QString::fromUtf8( vStr.c_str() ).split( QLatin1Char('.') );
                    if (splits.size() == 3) {
                        int major, minor, rev;
                        major = splits[0].toInt();
                        minor = splits[1].toInt();
                        rev = splits[2].toInt();
                        if (NATRON_VERSION_ENCODE(major, minor, rev) > NATRON_VERSION_ENCODED) {
                            throw std::invalid_argument("The given project was produced with a more recent and incompatible version of Natron.");
                        }
                    }
                }
            }
        }

        if (version >= PROJECT_SERIALIZATION_CHANGE_VERSION_SERIALIZATION) {
            ar & ::boost::serialization::make_nvp("VersionMajor", _projectLoadedInfo.vMajor);
            ar & ::boost::serialization::make_nvp("VersionMinor", _projectLoadedInfo.vMinor);
            ar & ::boost::serialization::make_nvp("VersionRev", _projectLoadedInfo.vRev);
            ar & ::boost::serialization::make_nvp("GitBranch", _projectLoadedInfo.gitBranch);
            ar & ::boost::serialization::make_nvp("GitCommit", _projectLoadedInfo.gitCommit);
            ar & ::boost::serialization::make_nvp("OS", _projectLoadedInfo.osStr);
            ar & ::boost::serialization::make_nvp("Bits", _projectLoadedInfo.bits);
       
        }

        if (version < PROJECT_SERIALIZATION_INTRODUCES_GROUPS) {
            int nodesCount;
            ar & ::boost::serialization::make_nvp("NodesCount", nodesCount);
            for (int i = 0; i < nodesCount; ++i) {
                NodeSerializationPtr ns(new NodeSerialization);
                ar & ::boost::serialization::make_nvp("item", *ns);
                _nodes.push_back(ns);
            }
        } else if (version < PROJECT_SERIALIZATION_CHANGE_NODE_COLLECTION_SERIALIZATION) {
            NodeCollectionSerialization nodes;
            ar & ::boost::serialization::make_nvp("NodesCollection", nodes);
            _nodes = nodes.getNodesSerialization();
        } else {
            int nodesCount;
            ar & ::boost::serialization::make_nvp("NodesCount", nodesCount);
            for (int i = 0; i < nodesCount; ++i) {
                NodeSerializationPtr ns(new NodeSerialization);
                ar & ::boost::serialization::make_nvp("Node", *ns);
                _nodes.push_back(ns);
            }
        }

        int knobsCount;
        ar & ::boost::serialization::make_nvp("ProjectKnobsCount", knobsCount);

        for (int i = 0; i < knobsCount; ++i) {
            KnobSerializationPtr ks(new KnobSerialization);
            ar & ::boost::serialization::make_nvp("item", *ks);
            _projectKnobs.push_back(ks);
        }

        if (version < PROJECT_SERIALIZATION_CHANGE_FORMAT_SERIALIZATION) {
            std::list<Format> formats;
            ar & ::boost::serialization::make_nvp("AdditionalFormats", formats);
            for (std::list<Format>::iterator it = formats.begin(); it!=formats.end(); ++it) {
                FormatSerialization s;
                s.x1 = it->x1;
                s.y1 = it->y1;
                s.x2 = it->x2;
                s.y2 = it->y2;
                s.par = it->getPixelAspectRatio();
                s.name = it->getName();
                _additionalFormats.push_back(s);
            }
        } else {
            ar & ::boost::serialization::make_nvp("AdditionalFormats", _additionalFormats);
        }

        ar & ::boost::serialization::make_nvp("Timeline_current_time", _timelineCurrent);
        if (version < PROJECT_SERIALIZATION_REMOVES_TIMELINE_BOUNDS) {
            SequenceTime left, right;
            ar & ::boost::serialization::make_nvp("Timeline_left_bound", left);
            ar & ::boost::serialization::make_nvp("Timeline_right_bound", right);
        }
        if (version < PROJECT_SERIALIZATION_REMOVES_NODE_COUNTERS) {
            std::map<std::string, int> _nodeCounters;
            ar & ::boost::serialization::make_nvp("NodeCounters", _nodeCounters);
        }
        ar & ::boost::serialization::make_nvp("CreationDate", _creationDate);

        if (version >= PROJECT_SERIALIZATION_DEPRECATE_PROJECT_GUI) {
            _projectWorkspace.reset(new ProjectWorkspaceSerialization);
            ar & ::boost::serialization::make_nvp("Workspace", *_projectWorkspace);

            int nViewports;
            ar & ::boost::serialization::make_nvp("nViewports", nViewports);
            for (int i = 0; i < nViewports; ++i) {
                std::string name;
                ViewportData data;
                ar & ::boost::serialization::make_nvp("ScriptName", name);
                ar & ::boost::serialization::make_nvp("Data", data);
                _viewportsData[name] = data;
            }
            ar & ::boost::serialization::make_nvp("OpenedPanels", _openedPanelsOrdered);

        }
    } // load
    
    BOOST_SERIALIZATION_SPLIT_MEMBER()
};

NATRON_NAMESPACE_EXIT;

BOOST_CLASS_VERSION(NATRON_NAMESPACE::ProjectSerialization, PROJECT_SERIALIZATION_VERSION)

#endif // PROJECTSERIALIZATION_H
