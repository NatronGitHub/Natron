/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
 * Copyright (C) 2018-2020 The Natron developers
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

#include "PyGuiApp.h"

#include <stdexcept>

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QColorDialog>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/PyNodeGroup.h"
#include "Engine/PyNode.h"
#include "Engine/PyParameter.h" // ColorTuple
#include "Engine/ScriptObject.h"
#include "Engine/TimeLine.h"
#include "Engine/ViewIdx.h"
#include "Engine/ViewerNode.h"
#include "Engine/ViewerInstance.h"

#include "Gui/Gui.h"
#include "Gui/NodeGraph.h"
#include "Gui/NodeGui.h"
#include "Gui/PythonPanels.h"
#include "Gui/SequenceFileDialog.h"
#include "Gui/TabWidget.h"
#include "Gui/ViewerGL.h"
#include "Gui/ViewerTab.h"

NATRON_NAMESPACE_ENTER
NATRON_PYTHON_NAMESPACE_ENTER

GuiApp::GuiApp(const GuiAppInstancePtr& app)
    : App(app)
    , _app(app)
{
}

GuiApp::~GuiApp()
{
}

Gui*
GuiApp::getGui() const
{
    return getInternalGuiApp()->getGui();
}

PyModalDialog*
GuiApp::createModalDialog()
{
    PyModalDialog* ret = new PyModalDialog( getInternalGuiApp()->getGui() );

    return ret;
}

PyTabWidget*
GuiApp::getTabWidget(const QString& name) const
{
    std::list<TabWidgetI*> tabs = getInternalGuiApp()->getTabWidgetsSerialization();

    for (std::list<TabWidgetI*>::const_iterator it = tabs.begin(); it != tabs.end(); ++it) {
        if ( (*it)->getScriptName() == name.toStdString() ) {
            TabWidget* isTab = dynamic_cast<TabWidget*>(*it);
            if (isTab) {
                return new PyTabWidget(isTab);
            }
        }
    }

    return 0;
}

PyTabWidget*
GuiApp::getActiveTabWidget() const
{
    TabWidget* tab =  getInternalGuiApp()->getGui()->getLastEnteredTabWidget();

    if (!tab) {
        return 0;
    }

    return new PyTabWidget(tab);
}

bool
GuiApp::moveTab(const QString& scriptName,
                PyTabWidget* pane)
{
    PanelWidget* w;
    ScriptObject* o;

    getInternalGuiApp()->getGui()->findExistingTab(scriptName.toStdString(), &w, &o);
    if (!w || !o) {
        return false;
    }

    return pane->getInternalTabWidget()->moveTab( w, o );
}

void
GuiApp::registerPythonPanel(PyPanel* panel,
                            const QString& pythonFunction)
{
    getInternalGuiApp()->registerPyPanel( panel, pythonFunction.toStdString() );
}

void
GuiApp::unregisterPythonPanel(PyPanel* panel)
{
    getInternalGuiApp()->unregisterPyPanel(panel);
}

QString
GuiApp::getFilenameDialog(const QStringList& filters,
                          const QString& location) const
{
    Gui* gui = getInternalGuiApp()->getGui();
    std::vector<std::string> f;

    for (QStringList::const_iterator it = filters.begin(); it != filters.end(); ++it) {
        f.push_back( it->toStdString() );
    }
    SequenceFileDialog dialog(gui,
                              f,
                              false,
                              SequenceFileDialog::eFileDialogModeOpen,
                              location.toStdString(),
                              gui,
                              false);
    if ( dialog.exec() ) {
        QString ret = QString::fromUtf8( dialog.selectedFiles().c_str() );

        return ret;
    }

    return QString();
}

QString
GuiApp::getSequenceDialog(const QStringList& filters,
                          const QString& location) const
{
    Gui* gui = getInternalGuiApp()->getGui();
    std::vector<std::string> f;

    for (QStringList::const_iterator it = filters.begin(); it != filters.end(); ++it) {
        f.push_back( it->toStdString() );
    }
    SequenceFileDialog dialog(gui,
                              f,
                              true,
                              SequenceFileDialog::eFileDialogModeOpen,
                              location.toStdString(),
                              gui,
                              false);
    if ( dialog.exec() ) {
        QString ret = QString::fromUtf8( dialog.selectedFiles().c_str() );

        return ret;
    }

    return QString();
}

QString
GuiApp::getDirectoryDialog(const QString& location) const
{
    Gui* gui = getInternalGuiApp()->getGui();
    std::vector<std::string> f;
    SequenceFileDialog dialog(gui,
                              f,
                              false,
                              SequenceFileDialog::eFileDialogModeDir,
                              location.toStdString(),
                              gui,
                              false);

    if ( dialog.exec() ) {
        QString ret = QString::fromUtf8( dialog.selectedDirectory().c_str() );

        return ret;
    }

    return QString();
}

QString
GuiApp::saveFilenameDialog(const QStringList& filters,
                           const QString& location) const
{
    Gui* gui = getInternalGuiApp()->getGui();
    std::vector<std::string> f;

    for (QStringList::const_iterator it = filters.begin(); it != filters.end(); ++it) {
        f.push_back( it->toStdString() );
    }
    SequenceFileDialog dialog(gui,
                              f,
                              false,
                              SequenceFileDialog::eFileDialogModeSave,
                              location.toStdString(),
                              gui,
                              false);
    if ( dialog.exec() ) {
        QString ret = QString::fromUtf8( dialog.filesToSave().c_str() );

        return ret;
    }

    return QString();
}

QString
GuiApp::saveSequenceDialog(const QStringList& filters,
                           const QString& location) const
{
    Gui* gui = getInternalGuiApp()->getGui();
    std::vector<std::string> f;

    for (QStringList::const_iterator it = filters.begin(); it != filters.end(); ++it) {
        f.push_back( it->toStdString() );
    }
    SequenceFileDialog dialog(gui,
                              f,
                              true,
                              SequenceFileDialog::eFileDialogModeSave,
                              location.toStdString(),
                              gui,
                              false);
    if ( dialog.exec() ) {
        QString ret = QString::fromUtf8( dialog.filesToSave().c_str() );

        return ret;
    }

    return QString();
}

ColorTuple
GuiApp::getRGBColorDialog() const
{
    ColorTuple ret;
    QColorDialog dialog;

    if ( dialog.exec() ) {
        QColor color = dialog.currentColor();

        ret.r = color.redF();
        ret.g = color.greenF();
        ret.b = color.blueF();
        ret.a = 1.;
    } else {
        ret.r = ret.g = ret.b = ret.a = 0.;
    }

    return ret;
}

std::list<Effect*>
GuiApp::getSelectedNodes(Group* group) const
{
    std::list<Effect*> ret;
    NodeGraph* graph = 0;

    if (group) {
        Effect* isEffect = dynamic_cast<Effect*>(group);
        if (isEffect) {
            NodeGroupPtr nodeGrp = isEffect->getInternalNode()->isEffectNodeGroup();
            if (nodeGrp) {
                NodeGraphI* graph_i  = nodeGrp->getNodeGraph();
                if (graph_i) {
                    graph = dynamic_cast<NodeGraph*>(graph_i);
                    assert(graph);
                    if (!graph) {
                        throw std::logic_error("");
                    }
                }
            }
        }
    } else {
        graph = getInternalGuiApp()->getGui()->getLastSelectedGraph();
    }
    if (!graph) {
        graph = getInternalGuiApp()->getGui()->getNodeGraph();
    }
    assert(graph);
    if (!graph) {
        throw std::logic_error("");
    }
    NodesGuiList nodes = graph->getSelectedNodes();
    for (NodesGuiList::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        NodePtr node = (*it)->getNode();
        Effect* eff = App::createEffectFromNodeWrapper(node);
        assert(eff);
        ret.push_back(eff);
    }

    return ret;
}

void
GuiApp::selectNode(Effect* effect,
                   bool clearPreviousSelection)
{
    if ( !effect || appPTR->isBackground() ) {
        return;
    }
    NodeCollectionPtr collection = effect->getInternalNode()->getGroup();
    if (!collection) {
        return;
    }

    NodeGuiPtr nodeUi = boost::dynamic_pointer_cast<NodeGui>( effect->getInternalNode()->getNodeGui() );
    if (!nodeUi) {
        return;
    }
    NodeGroupPtr isGroup = toNodeGroup(collection);
    NodeGraph* graph = 0;
    if (isGroup) {
        graph = dynamic_cast<NodeGraph*>( isGroup->getNodeGraph() );
    } else {
        graph = getInternalGuiApp()->getGui()->getNodeGraph();
    }
    assert(graph);
    if (!graph) {
        throw std::logic_error("");
    }
    graph->selectNode(nodeUi, !clearPreviousSelection);
}

void
GuiApp::setSelection(const std::list<Effect*>& nodes)
{
    if ( appPTR->isBackground() ) {
        return;
    }
    NodesGuiList selection;
    NodeCollectionPtr collection;
    bool printWarn = false;
    for (std::list<Effect*>::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        NodeGuiPtr nodeUi = boost::dynamic_pointer_cast<NodeGui>( (*it)->getInternalNode()->getNodeGui() );
        if (!nodeUi) {
            continue;
        }
        if (!collection) {
            collection = (*it)->getInternalNode()->getGroup();
        } else {
            if ( (*it)->getInternalNode()->getGroup() != collection ) {
                ///Group mismatch
                printWarn = true;
                continue;
            }
        }
        selection.push_back(nodeUi);
    }
    if (printWarn) {
        getInternalGuiApp()->appendToScriptEditor( tr("Python: Invalid selection from setSelection(): Some nodes in the list do not belong to the same group.").toStdString() );
    } else {
        NodeGroupPtr isGroup = toNodeGroup(collection);
        NodeGraph* graph = 0;
        if (isGroup) {
            graph = dynamic_cast<NodeGraph*>( isGroup->getNodeGraph() );
        } else {
            graph = getInternalGuiApp()->getGui()->getNodeGraph();
        }
        assert(graph);
        if (!graph) {
            throw std::logic_error("");
        }
        graph->setSelection(selection);
    }
}

void
GuiApp::selectAllNodes(Group* group)
{
    if ( appPTR->isBackground() ) {
        return;
    }
    NodeGraph* graph = NULL;
    NodeCollectionPtr collection;
    NodeGroupPtr isGroup = NodeGroupPtr();
    if (group) {
        collection = group->getInternalCollection();
        if (collection) {
            isGroup = toNodeGroup(collection);
            if (isGroup) {
                graph = dynamic_cast<NodeGraph*>( isGroup->getNodeGraph() );
            }
        }
    } else {
        graph = getInternalGuiApp()->getGui()->getLastSelectedGraph();
    }
    if (!graph) {
        graph = getInternalGuiApp()->getGui()->getNodeGraph();
    }
    assert(graph);
    if (!graph) {
        throw std::logic_error("");
    }
    graph->selectAllNodes(false);
}

void
GuiApp::deselectNode(Effect* effect)
{
    if ( !effect || appPTR->isBackground() ) {
        return;
    }

    NodeCollectionPtr collection = effect->getInternalNode()->getGroup();
    if (!collection) {
        return;
    }

    NodeGuiPtr nodeUi = boost::dynamic_pointer_cast<NodeGui>( effect->getInternalNode()->getNodeGui() );
    if (!nodeUi) {
        return;
    }
    NodeGroupPtr isGroup = toNodeGroup(collection);
    NodeGraph* graph = 0;
    if (isGroup) {
        graph = dynamic_cast<NodeGraph*>( isGroup->getNodeGraph() );
    } else {
        graph = getInternalGuiApp()->getGui()->getNodeGraph();
    }
    assert(graph);
    if (!graph) {
        throw std::logic_error("");
    }
    graph->deselectNode(nodeUi);
}

void
GuiApp::clearSelection(Group* group)
{
    if ( appPTR->isBackground() ) {
        return;
    }

    NodeGraph* graph = NULL;
    NodeCollectionPtr collection;
    NodeGroupPtr isGroup = NodeGroupPtr();
    if (group) {
        collection = group->getInternalCollection();
        if (collection) {
            isGroup = toNodeGroup(collection);
            if (isGroup) {
                graph = dynamic_cast<NodeGraph*>( isGroup->getNodeGraph() );
            }
        }
    } else {
        graph = getInternalGuiApp()->getGui()->getLastSelectedGraph();
    }
    if (!graph) {
        graph = getInternalGuiApp()->getGui()->getNodeGraph();
    }
    assert(graph);
    if (!graph) {
        throw std::logic_error("");
    }
    graph->clearSelection();
}


Effect*
GuiApp::getActiveViewer() const
{
    ViewerTab* tab = getInternalGuiApp()->getGui()->getActiveViewer();

    if (!tab) {
        return 0;
    }
    ViewerNodePtr instance = tab->getInternalNode();
    if (!instance) {
        return 0;
    }
    NodePtr node = instance->getNode();
    if (!node) {
        return 0;
    }

    return App::createEffectFromNodeWrapper(node);
}

PyPanel*
GuiApp::getUserPanel(const QString& scriptName) const
{
    PanelWidget* w = getInternalGuiApp()->getGui()->findExistingTab( scriptName.toStdString() );

    if (!w) {
        return 0;
    }

    return dynamic_cast<PyPanel*>( w->getWidget() );
}

void
GuiApp::renderBlocking(Effect* writeNode,
                       int firstFrame,
                       int lastFrame,
                       int frameStep)
{
    renderInternal(true, writeNode, firstFrame, lastFrame, frameStep);
}

void
GuiApp::renderBlocking(const std::list<Effect*>& effects,
                       const std::list<int>& firstFrames,
                       const std::list<int>& lastFrames,
                       const std::list<int>& frameSteps)
{
    renderInternal(true, effects, firstFrames, lastFrames, frameSteps);
}


NATRON_PYTHON_NAMESPACE_EXIT
NATRON_NAMESPACE_EXIT
