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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "ProjectGuiSerialization.h"

#include <stdexcept>

#include "Global/Macros.h"

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QDebug>
#include <QSplitter>
#include <QVBoxLayout>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/Node.h"
#include "Engine/PyParameter.h"
#include "Engine/Project.h"
#include "Engine/NodeSerialization.h"
#include "Engine/ProjectSerialization.h"
#include "Engine/ViewerInstance.h"
#include "Engine/ViewerNode.h"

#include "Gui/DockablePanel.h"
#include "Gui/FloatingWidget.h"
#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/Histogram.h"
#include "Gui/NodeGraph.h"
#include "Gui/NodeGui.h"
#include "Gui/NodeGuiSerialization.h"
#include "Gui/ProjectGui.h"
#include "Gui/PythonPanels.h"
#include "Gui/ScriptEditor.h"
#include "Gui/Splitter.h"
#include "Gui/TabWidget.h"
#include "Gui/ViewerGL.h"
#include "Gui/ViewerTab.h"

NATRON_NAMESPACE_ENTER;

static void convertNodeGuiSerializationToNodeSerialization(const std::list<NodeGuiSerialization>& nodeGuis, NodeSerialization* serialization)
{
    for (std::list<NodeGuiSerialization>::const_iterator it = nodeGuis.begin(); it != nodeGuis.end(); ++it) {
        const std::string& fullName = (it)->_nodeName;
        std::string groupMasterName;
        std::string nodeName;
        {
            std::size_t foundDot = fullName.find_last_of(".");
            if (foundDot != std::string::npos) {
                groupMasterName = fullName.substr(0, foundDot);
                nodeName = fullName.substr(foundDot + 1);
            } else {
                nodeName = fullName;
            }
        }
        if (groupMasterName == serialization->_groupFullyQualifiedScriptName && nodeName == serialization->_nodeScriptName) {
            serialization->_nodePositionCoords[0] = it->_posX;
            serialization->_nodePositionCoords[1] = it->_posY;
            serialization->_nodeSize[0] = it->_width;
            serialization->_nodeSize[1] = it->_height;
            if (it->_colorWasFound) {
                serialization->_nodeColor[0] = it->_r;
                serialization->_nodeColor[1] = it->_g;
                serialization->_nodeColor[2] = it->_b;
            } else {
                serialization->_nodeColor[0] = serialization->_nodeColor[1] = serialization->_nodeColor[2] = -1;
            }
            if (it->_hasOverlayColor) {
                serialization->_overlayColor[0] = it->_r;
                serialization->_overlayColor[1] = it->_g;
                serialization->_overlayColor[2] = it->_b;
            } else {
                serialization->_overlayColor[0] = serialization->_overlayColor[1] = serialization->_overlayColor[2] = -1;
            }
            serialization->_nodeIsSelected = it->_selected;
            break;
        }
    }

}

static void convertPaneLayoutToTabWidgetSerialization(const PaneLayout& deprecated, ProjectTabWidgetSerialization* serialization)
{
    serialization->isAnchor = deprecated.isAnchor;
    serialization->currentIndex = deprecated.currentIndex;
    serialization->tabs = deprecated.tabs;
    serialization->scriptName = deprecated.name;
}

static void convertSplitterToProjectSplitterSerialization(const SplitterSerialization& deprecated, ProjectWindowSplitterSerialization* serialization)
{
    QStringList list = QString::fromUtf8(deprecated.sizes.c_str()).split( QLatin1Char(' ') );

    assert(list.size() == 2);
    QList<int> s;
    s << list[0].toInt() << list[1].toInt();
    if ( (s[0] == 0) || (s[1] == 0) ) {
        int mean = (s[0] + s[1]) / 2;
        s[0] = s[1] = mean;
    }

    assert(deprecated.children.size() == 2);
    serialization->leftChildSize = s[0];
    serialization->rightChildSize = s[1];
    serialization->orientation = deprecated.orientation;
    serialization->leftChild.reset(new ProjectWindowSplitterSerialization::Child);
    serialization->rightChild.reset(new ProjectWindowSplitterSerialization::Child);

    ProjectWindowSplitterSerialization::Child* children[2] = {serialization->leftChild.get(), serialization->rightChild.get()};
    for (int i = 0; i < 2; ++i) {
        if (deprecated.children[i]->child_asPane) {
            children[i]->childIsTabWidget.reset(new ProjectTabWidgetSerialization);
            children[i]->type = eProjectWorkspaceWidgetTypeTabWidget;
            convertPaneLayoutToTabWidgetSerialization(*deprecated.children[i]->child_asPane, children[i]->childIsTabWidget.get());
        } else if (deprecated.children[i]->child_asSplitter) {
            children[i]->childIsSplitter.reset(new ProjectWindowSplitterSerialization);
            children[i]->type = eProjectWorkspaceWidgetTypeSplitter;
            convertSplitterToProjectSplitterSerialization(*deprecated.children[i]->child_asSplitter, children[i]->childIsSplitter.get());
        }

    }
}

void
ProjectGuiSerialization::convertToProjectSerialization(ProjectSerialization* serialization) const
{

    if (!serialization->_projectWorkspace) {
        serialization->_projectWorkspace.reset(new ProjectWorkspaceSerialization);
    }
    for (std::map<std::string, ViewerData >::const_iterator it = _viewersData.begin(); it != _viewersData.end(); ++it) {
        ViewportData& d = serialization->_viewportsData[it->first];
        d.left = it->second.zoomLeft;
        d.bottom = it->second.zoomBottom;
        d.zoomFactor = it->second.zoomFactor;
        d.par = 1.;
        d.zoomOrPanSinceLastFit = it->second.zoomOrPanSinceLastFit;
    }

    for (std::list<NodeSerializationPtr>::iterator it = serialization->_nodes.begin(); it!=serialization->_nodes.end(); ++it) {
        convertNodeGuiSerializationToNodeSerialization(_serializedNodes, it->get());
        for (std::list<NodeSerializationPtr>::iterator it2 = (*it)->_children.begin(); it2!=(*it)->_children.end(); ++it2) {
            convertNodeGuiSerializationToNodeSerialization(_serializedNodes, it2->get());
        }
    }

    _layoutSerialization.convertToProjectWorkspaceSerialization(serialization->_projectWorkspace.get());
    serialization->_projectWorkspace->_histograms = _histograms;
    serialization->_projectWorkspace->_pythonPanels = _pythonPanels;

    serialization->_openedPanelsOrdered = _openedPanelsOrdered;



}

void
GuiLayoutSerialization::convertToProjectWorkspaceSerialization(ProjectWorkspaceSerialization* serialization) const
{
    for (std::list<ApplicationWindowSerialization*>::const_iterator it = _windows.begin(); it != _windows.end(); ++it) {
        boost::shared_ptr<ProjectWindowSerialization> window(new ProjectWindowSerialization);
        window->windowPosition[0] = (*it)->x;
        window->windowPosition[1] = (*it)->y;
        window->windowSize[0] = (*it)->w;
        window->windowSize[1] = (*it)->h;

        if ((*it)->child_asSplitter) {
            window->isChildSplitter.reset(new ProjectWindowSplitterSerialization);
            window->childType = eProjectWorkspaceWidgetTypeSplitter;
            convertSplitterToProjectSplitterSerialization(*(*it)->child_asSplitter, window->isChildSplitter.get());
        } else if ((*it)->child_asPane) {
            window->isChildTabWidget.reset(new ProjectTabWidgetSerialization);
            window->childType = eProjectWorkspaceWidgetTypeTabWidget;
            convertPaneLayoutToTabWidgetSerialization(*(*it)->child_asPane, window->isChildTabWidget.get());
        } else if (!(*it)->child_asDockablePanel.empty()) {
            window->isChildSettingsPanel = (*it)->child_asDockablePanel;
        }

        if ((*it)->isMainWindow) {
            serialization->_mainWindowSerialization = window;
        } else {
            serialization->_floatingWindowsSerialization.push_back(window);
        }
    }
}

NATRON_NAMESPACE_EXIT;
