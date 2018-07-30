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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Gui.h"

#include <cassert>
#include <sstream> // stringstream
#include <algorithm> // min, max
#include <map>
#include <list>
#include <utility>
#include <stdexcept>

#include <QtCore/QTextStream>
#include <QtCore/QSettings>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>

#include <QAction>
#include <QApplication>

#include "Engine/CLArgs.h"
#include "Engine/CreateNodeArgs.h"
#include "Serialization/KnobSerialization.h" 
#include "Engine/Lut.h" // Color, floatToInt
#include "Engine/Node.h"
#include "Engine/NodeGroup.h" // NodeGroup, NodeCollection, NodesList
#include "Engine/Project.h"
#include "Engine/Settings.h"
#include "Engine/ViewerInstance.h"
#include "Engine/ViewerNode.h"

#include "Gui/ActionShortcuts.h"
#include "Gui/AnimationModuleEditor.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h" // appPTR
#include "Gui/GuiPrivate.h"
#include "Gui/Histogram.h"
#include "Gui/Menu.h"
#include "Gui/NodeGraph.h"
#include "Gui/NodeGui.h"
#include "Gui/ProjectGui.h"
#include "Gui/SequenceFileDialog.h"
#include "Gui/Splitter.h"
#include "Gui/TabWidget.h"
#include "Gui/ToolButton.h"
#include "Gui/ScriptEditor.h"
#include "Gui/ViewerGL.h"
#include "Gui/ViewerTab.h"

#include "Global/QtCompat.h" // removeFileExtension
#include "Global/StrUtils.h"
#ifdef DEBUG
#include "Global/FloatingPointExceptions.h"
#endif

#include <SequenceParsing.h> // for SequenceParsing::removePath

#define NAMED_PLUGIN_GROUP_NO 15


NATRON_NAMESPACE_ENTER

// Group ordering is set at every place in the code where GROUP_ORDER appears in the comments
static std::string namedGroupsOrdered[NAMED_PLUGIN_GROUP_NO] = {
    PLUGIN_GROUP_IMAGE,
    PLUGIN_GROUP_PAINT,
    PLUGIN_GROUP_TIME,
    PLUGIN_GROUP_CHANNEL,
    PLUGIN_GROUP_COLOR,
    PLUGIN_GROUP_FILTER,
    PLUGIN_GROUP_KEYER,
    PLUGIN_GROUP_MERGE,
    PLUGIN_GROUP_TRANSFORM,
    PLUGIN_GROUP_3D,
    PLUGIN_GROUP_DEEP,
    PLUGIN_GROUP_MULTIVIEW,
    PLUGIN_GROUP_TOOLSETS,
    PLUGIN_GROUP_OTHER,
    PLUGIN_GROUP_DEFAULT
};
NATRON_NAMESPACE_ANONYMOUS_ENTER

static void
getPixmapForGrouping(QPixmap* pixmap,
                     int size,
                     const QString & grouping)
{
    PixmapEnum e = NATRON_PIXMAP_OTHER_PLUGINS;

    // Group ordering is set at every place in the code where GROUP_ORDER appears in the comments
    if ( grouping == QString::fromUtf8(PLUGIN_GROUP_IMAGE) ) {
        e = NATRON_PIXMAP_IO_GROUPING;
    } else if ( grouping == QString::fromUtf8(PLUGIN_GROUP_PAINT) ) {
        e = NATRON_PIXMAP_PAINT_GROUPING;
    } else if ( grouping == QString::fromUtf8(PLUGIN_GROUP_TIME) ) {
        e = NATRON_PIXMAP_TIME_GROUPING;
    } else if ( grouping == QString::fromUtf8(PLUGIN_GROUP_CHANNEL) ) {
        e = NATRON_PIXMAP_CHANNEL_GROUPING;
    } else if ( grouping == QString::fromUtf8(PLUGIN_GROUP_COLOR) ) {
        e = NATRON_PIXMAP_COLOR_GROUPING;
    } else if ( grouping == QString::fromUtf8(PLUGIN_GROUP_FILTER) ) {
        e = NATRON_PIXMAP_FILTER_GROUPING;
    } else if ( grouping == QString::fromUtf8(PLUGIN_GROUP_KEYER) ) {
        e = NATRON_PIXMAP_KEYER_GROUPING;
    } else if ( grouping == QString::fromUtf8(PLUGIN_GROUP_MERGE) ) {
        e = NATRON_PIXMAP_MERGE_GROUPING;
    } else if ( grouping == QString::fromUtf8(PLUGIN_GROUP_TRANSFORM) ) {
        e = NATRON_PIXMAP_TRANSFORM_GROUPING;
    } else if ( grouping == QString::fromUtf8(PLUGIN_GROUP_3D) ) {
        e = NATRON_PIXMAP_3D_GROUPING;
    } else if ( grouping == QString::fromUtf8(PLUGIN_GROUP_DEEP) ) {
        e = NATRON_PIXMAP_DEEP_GROUPING;
    } else if ( grouping == QString::fromUtf8(PLUGIN_GROUP_MULTIVIEW) ) {
        e = NATRON_PIXMAP_MULTIVIEW_GROUPING;
    } else if ( grouping == QString::fromUtf8(PLUGIN_GROUP_TOOLSETS) ) {
        e = NATRON_PIXMAP_TOOLSETS_GROUPING;
    } else if ( grouping == QString::fromUtf8(PLUGIN_GROUP_OTHER) ) {
        e = NATRON_PIXMAP_MISC_GROUPING;
    }
    appPTR->getIcon(e, size, pixmap);
}

NATRON_NAMESPACE_ANONYMOUS_EXIT

void
Gui::reloadStylesheet()
{
    Gui::loadStyleSheet();

    if (_imp->_scriptEditor) {
        _imp->_scriptEditor->reloadHighlighter();
    }
}

static
QString
qcolor_to_qstring(const QColor& col)
{
    return QString::fromUtf8("rgb(%1,%2,%3)").arg( col.red() ).arg( col.green() ).arg( col.blue() );
}

static inline
QColor
to_qcolor(double r,
          double g,
          double b)
{
    return QColor( Color::floatToInt<256>(r), Color::floatToInt<256>(g), Color::floatToInt<256>(b) );
}

void
Gui::loadStyleSheet()
{
    SettingsPtr settings = appPTR->getCurrentSettings();
    QColor selCol, sunkCol, baseCol, raisedCol, txtCol, intCol, kfCol, disCol, eCol, altCol, lightSelCol;

    //settings->
    {
        double r, g, b;
        settings->getSelectionColor(&r, &g, &b);
        selCol = to_qcolor(r, g, b);
    }
    {
        double r, g, b;
        r = 1;
        g = 0.75;
        b = 0.47;
        lightSelCol = to_qcolor(r, g, b);
    }
    {
        double r, g, b;
        settings->getBaseColor(&r, &g, &b);
        baseCol = to_qcolor(r, g, b);
    }
    {
        double r, g, b;
        settings->getRaisedColor(&r, &g, &b);
        raisedCol = to_qcolor(r, g, b);
    }
    {
        double r, g, b;
        settings->getSunkenColor(&r, &g, &b);
        sunkCol = to_qcolor(r, g, b);
    }
    {
        double r, g, b;
        settings->getTextColor(&r, &g, &b);
        txtCol = to_qcolor(r, g, b);
    }
    {
        double r, g, b;
        settings->getInterpolatedColor(&r, &g, &b);
        intCol = to_qcolor(r, g, b);
    }
    {
        double r, g, b;
        settings->getKeyframeColor(&r, &g, &b);
        kfCol = to_qcolor(r, g, b);
    }
    {
        double r, g, b;
        r = g = b = 0.;
        disCol = to_qcolor(r, g, b);
    }
    {
        double r, g, b;
        settings->getExprColor(&r, &g, &b);
        eCol = to_qcolor(r, g, b);
    }
    {
        double r, g, b;
        settings->getAltTextColor(&r, &g, &b);
        altCol = to_qcolor(r, g, b);
    }

    // First, set the global palette colors
    // Note: the following colors may be wrong, please test and fix these on Linux with an empty mainstyle.css
    QPalette p = qApp->palette();

    p.setBrush( QPalette::Window, sunkCol );
    p.setBrush( QPalette::WindowText, txtCol );
    p.setBrush( QPalette::Base, baseCol );
    //p.setBrush( QPalette::ToolTipBase, baseCol );
    //p.setBrush( QPalette::ToolTipText, txtCol );
    p.setBrush( QPalette::Text, txtCol );
    p.setBrush( QPalette::Button, baseCol );
    p.setBrush( QPalette::ButtonText, txtCol );
    p.setBrush( QPalette::Light, raisedCol );
    p.setBrush( QPalette::Dark, sunkCol );
    p.setBrush( QPalette::Mid, baseCol );
    //p.setBrush( QPalette::Shadow, sunkCol );
    p.setBrush( QPalette::BrightText, txtCol );
    p.setBrush( QPalette::Link, selCol ); // can only be set via palette
    p.setBrush( QPalette::LinkVisited, selCol ); // can only be set via palette
    {
#ifdef DEBUG
        boost_adaptbx::floating_point::exception_trapping trap(0);
#endif
        qApp->setPalette( p );
    }

    QFile qss;
    std::string userQss = settings->getUserStyleSheetFilePath();
    if ( !userQss.empty() ) {
        qss.setFileName( QString::fromUtf8( userQss.c_str() ) );
    } else {
        qss.setFileName( QString::fromUtf8(":/Resources/Stylesheets/mainstyle.qss") );
    }

    if ( qss.open(QIODevice::ReadOnly
                  | QIODevice::Text) ) {
        QTextStream in(&qss);
        QString content = QString::fromUtf8("QWidget { font-family: \"%1\"; font-size: %2pt; }\n"
                                            "QListView { font-family: \"%1\"; font-size: %2pt; }\n" // .. or the items in the QListView get the wrong font
                                            "QComboBox::drop-down { font-family: \"%1\"; font-size: %2pt; }\n" // ... or the drop-down doesn't get the right font
                                            "QInputDialog { font-family: \"%1\"; font-size: %2pt; }\n" // ... or the label doesn't get the right font
                                            ).arg(appFont).arg(appFontSize);
        content += in.readAll();
#ifdef DEBUG
        boost_adaptbx::floating_point::exception_trapping trap(0);
#endif
        qApp->setStyleSheet( content
                             .arg( qcolor_to_qstring(selCol) ) // %1: selection-color
                             .arg( qcolor_to_qstring(baseCol) ) // %2: medium background
                             .arg( qcolor_to_qstring(raisedCol) ) // %3: soft background
                             .arg( qcolor_to_qstring(sunkCol) ) // %4: strong background
                             .arg( qcolor_to_qstring(txtCol) ) // %5: text colour
                             .arg( qcolor_to_qstring(intCol) ) // %6: interpolated value color
                             .arg( qcolor_to_qstring(kfCol) ) // %7: keyframe value color
                             .arg( qcolor_to_qstring(disCol) ) // %8: disabled editable text
                             .arg( qcolor_to_qstring(eCol) ) // %9: expression background color
                             .arg( qcolor_to_qstring(altCol) ) // %10 = altered text color
                             .arg( qcolor_to_qstring(lightSelCol) ) ); // %11 = mouse over selection color
    } else {
        Dialogs::errorDialog( tr("Stylesheet").toStdString(), tr("Failure to load stylesheet file ").toStdString() + qss.fileName().toStdString() );
    }
} // Gui::loadStyleSheet

void
Gui::maximize(TabWidget* what)
{
    assert(what);
    if ( what->isFloatingWindowChild() ) {
        return;
    }

    std::list<TabWidgetI*> panes = getApp()->getTabWidgetsSerialization();
    for (std::list<TabWidgetI*>::iterator it = panes.begin(); it != panes.end(); ++it) {
        TabWidget* pane = dynamic_cast<TabWidget*>(*it);
        if (!pane) {
            continue;
        }
        //if the widget is not what we want to maximize and it is not floating , hide it
        if ( (pane != what) && !pane->isFloatingWindowChild() ) {
            // also if we want to maximize the workshop pane, don't hide the properties pane

            bool hasProperties = false;
            for (int i = 0; i < pane->count(); ++i) {
                QString tabName = pane->tabAt(i)->getWidget()->objectName();
                if ( tabName == QString::fromUtf8(kPropertiesBinName) ) {
                    hasProperties = true;
                    break;
                }
            }

            bool hasNodeGraphOrCurveEditor = false;
            for (int i = 0; i < what->count(); ++i) {
                QWidget* tab = what->tabAt(i)->getWidget();
                assert(tab);
                NodeGraph* isGraph = dynamic_cast<NodeGraph*>(tab);
                AnimationModuleEditor* isEditor = dynamic_cast<AnimationModuleEditor*>(tab);
                if (isGraph || isEditor) {
                    hasNodeGraphOrCurveEditor = true;
                    break;
                }
            }

            if (hasProperties && hasNodeGraphOrCurveEditor) {
                continue;
            }
            pane->hide();
        }
    }
    _imp->_toolBox->hide();
}

void
Gui::minimize()
{
    std::list<TabWidgetI*> panes = getApp()->getTabWidgetsSerialization();
    for (std::list<TabWidgetI*>::iterator it = panes.begin(); it != panes.end(); ++it) {
        TabWidget* pane = dynamic_cast<TabWidget*>(*it);
        if (!pane) {
            continue;
        }
        pane->show();
    }
    if (!_imp->leftToolBarDisplayedOnHoverOnly) {
        _imp->_toolBox->show();
    }
}

ViewerTab*
Gui::addNewViewerTab(const NodeGuiPtr& node,
                     TabWidget* where)
{
    if (!node) {
        return 0;
    }

    ViewerNodePtr viewer = node->getNode()->isEffectViewerNode();

    NodesGuiList activeNodeViewerUi, nodeViewerUi;

    //Don't create tracker & roto interface for file dialog preview viewer
    NodeCollectionPtr group = viewer->getNode()->getGroup();
    if (group) {
        if ( !_imp->_viewerTabs.empty() ) {
            ( *_imp->_viewerTabs.begin() )->getNodesViewerInterface(&nodeViewerUi, &activeNodeViewerUi);
        } else {
            NodeGraph* graph = dynamic_cast<NodeGraph*>( group->getNodeGraph() );
            if (!graph) {
                graph = _imp->_nodeGraphArea;
            }

            if (graph) {
                const NodesGuiList & allNodes = graph->getAllActiveNodes();
                std::set<std::string> activeNodesPluginID;

                for (NodesGuiList::const_iterator it = allNodes.begin(); it != allNodes.end(); ++it) {
                    nodeViewerUi.push_back( *it );
                    std::string pluginID = (*it)->getNode()->getPluginID();
                    std::set<std::string>::iterator found = activeNodesPluginID.find(pluginID);
                    if ( found == activeNodesPluginID.end() ) {
                        activeNodesPluginID.insert(pluginID);
                        activeNodeViewerUi.push_back(*it);
                    }
                }
            }
        }
    }

    std::string nodeName =  node->getNode()->getFullyQualifiedName();
    for (std::size_t i = 0; i < nodeName.size(); ++i) {
        if (nodeName[i] == '.') {
            nodeName[i] = '_';
        }
    }
    std::string label;
    NodeGraph::makeFullyQualifiedLabel(node->getNode(), &label);

    ViewerTab* tab = new ViewerTab(nodeName, nodeViewerUi, activeNodeViewerUi, this, node, where);
    tab->setLabel(label);

    QObject::connect( tab->getViewer(), SIGNAL(imageChanged(int)), this, SLOT(onViewerImageChanged(int)) );
    {
        QMutexLocker l(&_imp->_viewerTabsMutex);
        _imp->_viewerTabs.push_back(tab);
        if (!_imp->_activeViewer) {
            _imp->_activeViewer = tab;
        }
    }
    where->appendTab(tab, tab);
    Q_EMIT viewersChanged();

    return tab;
} // Gui::addNewViewerTab

void
Gui::onViewerImageChanged(int texIndex)
{
    ///notify all histograms a viewer image changed
    ViewerGL* viewer = qobject_cast<ViewerGL*>( sender() );

    if (viewer) {
        QMutexLocker l(&_imp->_histogramsMutex);
        for (std::list<Histogram*>::iterator it = _imp->_histograms.begin(); it != _imp->_histograms.end(); ++it) {
            (*it)->onViewerImageChanged(viewer, texIndex);
        }
    }
}

void
Gui::addViewerTab(ViewerTab* tab,
                  TabWidget* where)
{
    assert(tab);
    assert(where);
    {
        QMutexLocker l(&_imp->_viewerTabsMutex);
        std::list<ViewerTab*>::iterator it = std::find(_imp->_viewerTabs.begin(), _imp->_viewerTabs.end(), tab);
        if ( it == _imp->_viewerTabs.end() ) {
            _imp->_viewerTabs.push_back(tab);
        }
    }
    where->appendTab(tab, tab);
    Q_EMIT viewersChanged();
}

void
Gui::registerTab(PanelWidget* tab,
                 ScriptObject* obj)
{
    std::string name = obj->getScriptName();
    RegisteredTabs::iterator registeredTab = _imp->_registeredTabs.find(name);

    if ( registeredTab == _imp->_registeredTabs.end() ) {
        _imp->_registeredTabs.insert( std::make_pair( name, std::make_pair(tab, obj) ) );
    }
}

void
Gui::unregisterTab(PanelWidget* tab)
{
    if (getCurrentPanelFocus() == tab) {
        tab->removeClickFocus();
    }
    for (RegisteredTabs::iterator it = _imp->_registeredTabs.begin(); it != _imp->_registeredTabs.end(); ++it) {
        if (it->second.first == tab) {
            _imp->_registeredTabs.erase(it);
            break;
        }
    }
}


void
Gui::removeViewerTab(ViewerTab* tab,
                     bool initiatedFromNode,
                     bool deleteData)
{
    assert(tab);
    if (!tab) {
        throw std::logic_error("");
    }
    unregisterTab(tab);

    if (tab == _imp->_activeViewer) {
        _imp->_activeViewer = 0;
    }
    NodeGraph* graph = 0;
    NodeGroupPtr isGrp;
    NodeCollectionPtr collection;
    if ( tab->getInternalNode() && tab->getInternalNode()->getNode() ) {
        NodeCollectionPtr collection = tab->getInternalNode()->getNode()->getGroup();
        isGrp = toNodeGroup(collection);
    }


    if (isGrp) {
        NodeGraphI* graph_i = isGrp->getNodeGraph();
        assert(graph_i);
        graph = dynamic_cast<NodeGraph*>(graph_i);
    } else {
        graph = getNodeGraph();
    }
    assert(graph);
    if (!graph) {
        throw std::logic_error("");
    }

    ViewerTab* lastSelectedViewer = graph->getLastSelectedViewer();

    if (lastSelectedViewer == tab) {
        bool foundOne = false;
        NodesList nodes;
        if (collection) {
            nodes = collection->getNodes();
        }
        for (NodesList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
            ViewerNodePtr isViewer = (*it)->isEffectViewerNode();
            if ( !isViewer || ( isViewer == tab->getInternalNode() ) ) {
                continue;
            }
            OpenGLViewerI* viewerI = isViewer->getUiContext();
            assert(viewerI);
            ViewerGL* glViewer = dynamic_cast<ViewerGL*>(viewerI);
            assert(glViewer);
            if (glViewer) {
                graph->setLastSelectedViewer( glViewer->getViewerTab() );
            }
            foundOne = true;
            break;
        }
        if (!foundOne) {
            graph->setLastSelectedViewer(0);
        }
    }

    ViewerNodePtr viewerNode = tab->getInternalNode();
    if (viewerNode) {
        viewerNode->getNode()->abortAnyProcessing_non_blocking();
        if (getApp()->getLastViewerUsingTimeline() == viewerNode) {
            getApp()->discardLastViewerUsingTimeline();
        }
    }

    if (!initiatedFromNode) {
        assert(_imp->_nodeGraphArea);
        ///call the deleteNode which will call this function again when the node will be deactivated.
        NodePtr internalNode = tab->getInternalNode()->getNode();
        NodeGuiIPtr guiI = internalNode->getNodeGui();
        NodeGuiPtr gui = boost::dynamic_pointer_cast<NodeGui>(guiI);
        assert(gui);
        NodeGraphI* graph_i = internalNode->getGroup()->getNodeGraph();
        assert(graph_i);
        NodeGraph* graph = dynamic_cast<NodeGraph*>(graph_i);
        assert(graph);
        if (graph) {
            graph->removeNode(gui);
        }
    } else {
        tab->hide();


        TabWidget* container = dynamic_cast<TabWidget*>( tab->parentWidget() );
        if (container) {
            container->removeTab(tab, false);
        }

        if (deleteData) {
            QMutexLocker l(&_imp->_viewerTabsMutex);
            std::list<ViewerTab*>::iterator it = std::find(_imp->_viewerTabs.begin(), _imp->_viewerTabs.end(), tab);
            if ( it != _imp->_viewerTabs.end() ) {
                _imp->_viewerTabs.erase(it);
            }
            tab->notifyGuiClosingPublic();
            tab->deleteLater();
        }
    }
    Q_EMIT viewersChanged();
} // Gui::removeViewerTab

Histogram*
Gui::addNewHistogram()
{
    std::string baseName = "Histogram";
    std::string name;
    bool nameExists;
    int i = 1;
    do {
        nameExists = false;
        std::stringstream ss;
        ss << baseName;
        ss << i;
        name = ss.str();

        {
            QMutexLocker l(&_imp->_histogramsMutex);
            for (std::list<Histogram*>::iterator it = _imp->_histograms.begin(); it != _imp->_histograms.end(); ++it) {
                if ((*it)->getScriptName() == name) {
                    nameExists = true;
                    break;
                }
            }
        }
        ++i;
    } while(nameExists);

    std::string scriptName = NATRON_PYTHON_NAMESPACE::makeNameScriptFriendly(name);
    Histogram* h = new Histogram(scriptName, this);
    h->setLabel(name);

    {
        QMutexLocker l(&_imp->_histogramsMutex);
        _imp->_histograms.push_back(h);
    }
    return h;
}

void
Gui::removeHistogram(Histogram* h)
{
    unregisterTab(h);
    QMutexLocker l(&_imp->_histogramsMutex);
    std::list<Histogram*>::iterator it = std::find(_imp->_histograms.begin(), _imp->_histograms.end(), h);

    assert( it != _imp->_histograms.end() );
    delete *it;
    _imp->_histograms.erase(it);
}

const std::list<Histogram*> &
Gui::getHistograms() const
{
    QMutexLocker l(&_imp->_histogramsMutex);

    return _imp->_histograms;
}

std::list<Histogram*>
Gui::getHistograms_mt_safe() const
{
    QMutexLocker l(&_imp->_histogramsMutex);

    return _imp->_histograms;
}

void
Gui::checkNumberOfNonFloatingPanes()
{
    ///If dropping to 1 non floating pane, make it non closable:floatable
    int nbNonFloatingPanes;
    TabWidget* nonFloatingPane = _imp->getOnly1NonFloatingPane(nbNonFloatingPanes);

    ///When there's only 1 tab left make it closable/floatable again
    if (nbNonFloatingPanes == 1) {
        assert(nonFloatingPane);
        nonFloatingPane->setClosable(false);
    } else {
        std::list<TabWidgetI*> tabs = getApp()->getTabWidgetsSerialization();
        for (std::list<TabWidgetI*>::iterator it = tabs.begin(); it != tabs.end(); ++it) {
            (*it)->setClosable(true);
        }
    }
}


void
Gui::onPaneUnRegistered(TabWidgetI* pane)
{
    if (_imp->_lastEnteredTabWidget == pane) {
        _imp->_lastEnteredTabWidget = 0;
    }
    checkNumberOfNonFloatingPanes();
}

void
Gui::onPaneRegistered(TabWidgetI* pane)
{
    std::list<TabWidgetI*> tabs = getApp()->getTabWidgetsSerialization();
    if (tabs.empty()) {
        _imp->_leftRightSplitter->addWidget(dynamic_cast<TabWidget*>(pane));
    }
    checkNumberOfNonFloatingPanes();
}

PanelWidget*
Gui::findExistingTab(const std::string & name) const
{
    RegisteredTabs::const_iterator it = _imp->_registeredTabs.find(name);

    if ( it != _imp->_registeredTabs.end() ) {
        return it->second.first;
    } else {
        return NULL;
    }
}

void
Gui::findExistingTab(const std::string & name,
                     PanelWidget** w,
                     ScriptObject** o) const
{
    RegisteredTabs::const_iterator it = _imp->_registeredTabs.find(name);

    if ( it != _imp->_registeredTabs.end() ) {
        *w = it->second.first;
        *o = it->second.second;
    } else {
        *w = 0;
        *o = 0;
    }
}

ToolButton*
Gui::findExistingToolButton(const QString & label) const
{
    for (U32 i = 0; i < _imp->_toolButtons.size(); ++i) {
        if (_imp->_toolButtons[i]->getLabel() == label) {
            return _imp->_toolButtons[i];
        }
    }

    return NULL;
}

void
Gui::sortAllPluginsToolButtons()
{
    for (U32 i = 0; i < _imp->_toolButtons.size(); ++i) {
        _imp->_toolButtons[i]->sortChildren();
    }
}

ToolButton*
Gui::findOrCreateToolButton(const PluginGroupNodePtr & treeNode)
{

    // Do not create an action for non user creatable plug-ins
    bool isUserCreatable = true;
    PluginPtr internalPlugin = treeNode->getPlugin();
    if (internalPlugin && treeNode->getChildren().empty() && !internalPlugin->getIsUserCreatable()) {
        isUserCreatable = false;
    }
    if (!isUserCreatable) {
        return 0;
    }

    // Check for existing toolbuttons
    for (std::size_t i = 0; i < _imp->_toolButtons.size(); ++i) {
        if (_imp->_toolButtons[i]->getPluginToolButton() == treeNode) {
            return _imp->_toolButtons[i];
        }
    }

    // Check for parent toolbutton
    ToolButton* parentToolButton = NULL;
    if ( treeNode->getParent() ) {
        assert(treeNode->getParent() != treeNode);
        if (treeNode->getParent() != treeNode) {
            parentToolButton = findOrCreateToolButton( treeNode->getParent() );
        }
    }

    QString resourcesPath;
    if (internalPlugin) {
        resourcesPath = QString::fromUtf8(internalPlugin->getPropertyUnsafe<std::string>(kNatronPluginPropResourcesPath).c_str());
    }
    QString iconFilePath = resourcesPath;
    StrUtils::ensureLastPathSeparator(iconFilePath);
    iconFilePath += treeNode->getTreeNodeIconFilePath();

    QIcon toolButtonIcon, menuIcon;
    // Create tool icon
    if ( !iconFilePath.isEmpty() && QFile::exists(iconFilePath) ) {
        QPixmap pix(iconFilePath);
        int menuSize = TO_DPIX(NATRON_MEDIUM_BUTTON_ICON_SIZE);
        int toolButtonSize = !treeNode->getParent() ? TO_DPIX(NATRON_TOOL_BUTTON_ICON_SIZE) : TO_DPIX(NATRON_MEDIUM_BUTTON_ICON_SIZE);
        QPixmap menuPix = pix, toolbuttonPix = pix;
        if ( (std::max( menuPix.width(), menuPix.height() ) != menuSize) && !menuPix.isNull() ) {
            menuPix = menuPix.scaled(menuSize, menuSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }
        if ( (std::max( toolbuttonPix.width(), toolbuttonPix.height() ) != toolButtonSize) && !toolbuttonPix.isNull() ) {
            toolbuttonPix = toolbuttonPix.scaled(toolButtonSize, toolButtonSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }
        menuIcon.addPixmap(menuPix);
        toolButtonIcon.addPixmap(toolbuttonPix);
    } else {
        // Set default icon only if it has no parent, otherwise leave action without an icon
        if ( !treeNode->getParent() ) {
            QPixmap toolbuttonPix, menuPix;
            getPixmapForGrouping( &toolbuttonPix, TO_DPIX(NATRON_TOOL_BUTTON_ICON_SIZE), treeNode->getTreeNodeName() );
            toolButtonIcon.addPixmap(toolbuttonPix);
            getPixmapForGrouping( &menuPix, TO_DPIX(NATRON_TOOL_BUTTON_ICON_SIZE), treeNode->getTreeNodeName() );
            menuIcon.addPixmap(menuPix);
        }
    }

    // If the tool-button has no children, this is a leaf, we must create an action
    // At this point any plug-in MUST be in a toolbutton, so it must have a parent.
    assert(!treeNode->getChildren().empty() || treeNode->getParent());

    int majorVersion = internalPlugin ? internalPlugin->getPropertyUnsafe<unsigned int>(kNatronPluginPropVersion, 0) : 1;
    int minorVersion = internalPlugin ? internalPlugin->getPropertyUnsafe<unsigned int>(kNatronPluginPropVersion, 1) : 0;

    ToolButton* pluginsToolButton = new ToolButton(getApp(),
                                                   treeNode,
                                                   treeNode->getTreeNodeID(),
                                                   majorVersion,
                                                   minorVersion,
                                                   treeNode->getTreeNodeName(),
                                                   toolButtonIcon,
                                                   menuIcon);

    if (!treeNode->getChildren().empty()) {
        // For grouping items, create the menu
        Menu* menu = new Menu(this);
        QString pluginsToolButtonTitle = pluginsToolButton->getLabel();
        // see doc of QMenubar: convert & to &&
        pluginsToolButtonTitle.replace(QString::fromUtf8("&"), QString::fromUtf8("&&"));
        menu->setTitle(pluginsToolButtonTitle);
        menu->setIcon(menuIcon);
        pluginsToolButton->setMenu(menu);
        pluginsToolButton->setAction( menu->menuAction() );
    } else {
        // This is a leaf (plug-in)
        assert(internalPlugin);
        assert(parentToolButton);

        // If this is the highest major version for this plug-in use normal label, otherwise also append the major version
        bool isHighestMajorVersionForPlugin = internalPlugin->getIsHighestMajorVersion();

        QString pluginLabelText;
        {
            std::string pluginLabel = !isHighestMajorVersionForPlugin ? internalPlugin->getLabelVersionMajorEncoded() : internalPlugin->getLabelWithoutSuffix();
            pluginLabelText = QString::fromUtf8(pluginLabel.c_str());
            // see doc of QMenubar: convert & to &&
           pluginLabelText.replace(QString::fromUtf8("&"), QString::fromUtf8("&&"));
        }

        QKeySequence defaultNodeShortcut;
        QString shortcutGroup = QString::fromUtf8(kShortcutGroupNodes);
        std::vector<std::string> groupingSplit = internalPlugin->getPropertyNUnsafe<std::string>(kNatronPluginPropGrouping);
        for (std::size_t j = 0; j < groupingSplit.size(); ++j) {
            shortcutGroup.push_back( QLatin1Char('/') );
            shortcutGroup.push_back(QString::fromUtf8(groupingSplit[j].c_str()));
        }
        {
            // If the plug-in has a shortcut get it
            defaultNodeShortcut = getKeybind(shortcutGroup, QString::fromUtf8(internalPlugin->getPluginID().c_str()));
        }

        QAction* defaultPresetAction = new QAction(this);
        defaultPresetAction->setShortcut(defaultNodeShortcut);
        defaultPresetAction->setShortcutContext(Qt::WidgetShortcut);
        defaultPresetAction->setText(pluginLabelText);
        defaultPresetAction->setIcon( pluginsToolButton->getMenuIcon() );
        QObject::connect( defaultPresetAction, SIGNAL(triggered()), pluginsToolButton, SLOT(onTriggered()) );


        const std::vector<PluginPresetDescriptor>& presets = internalPlugin->getPresetFiles();
        if (presets.empty()) {
            // If the node has no presets, just make an action, otherwise make a menu
            pluginsToolButton->setAction(defaultPresetAction);
        } else {
            Menu* menu = new Menu(this);
            QString pluginsToolButtonTitle = pluginsToolButton->getLabel();
            // see doc of QMenubar: convert & to &&
            pluginsToolButtonTitle.replace(QString::fromUtf8("&"), QString::fromUtf8("&&"));
            menu->setTitle(pluginsToolButtonTitle);
            menu->setIcon(menuIcon);
            pluginsToolButton->setMenu(menu);
            pluginsToolButton->setAction( menu->menuAction() );

            defaultPresetAction->setText(pluginLabelText + tr(" (Default)"));
            menu->addAction(defaultPresetAction);

            for (std::vector<PluginPresetDescriptor>::const_iterator it = presets.begin(); it!=presets.end(); ++it) {

                QKeySequence presetShortcut;
                {
                    // If the preset has a shortcut get it

                    std::string shortcutKey = internalPlugin->getPluginID();
                    shortcutKey += "_preset_";
                    shortcutKey += it->presetLabel.toStdString();

                    presetShortcut = getKeybind(shortcutGroup, QString::fromUtf8(shortcutKey.c_str()));

                }

                QString presetLabel = pluginLabelText;
                presetLabel += QLatin1String(" (");
                presetLabel += it->presetLabel;
                presetLabel += QLatin1String(")");

                QAction* presetAction = new QAction(this);
                QPixmap presetPix;
                if (getPresetIcon(it->presetFilePath, it->presetIconFile, TO_DPIX(NATRON_MEDIUM_BUTTON_ICON_SIZE), &presetPix)) {
                    presetAction->setIcon( presetPix );
                }
                presetAction->setShortcut(presetShortcut);
                presetAction->setShortcutContext(Qt::WidgetShortcut);
                presetAction->setText(presetLabel);
                presetAction->setData(it->presetLabel);
                QObject::connect( presetAction, SIGNAL(triggered()), pluginsToolButton, SLOT(onTriggered()) );

                menu->addAction(presetAction);
            }
        }

    } // if (!treeNode->getChildren().empty())

    // If it has a parent, add the new tool button as a child
    if (parentToolButton) {
        parentToolButton->tryAddChild(pluginsToolButton);
    }
    _imp->_toolButtons.push_back(pluginsToolButton);

    return pluginsToolButton;
} // findOrCreateToolButton

bool
Gui::getPresetIcon(const QString& presetFilePath, const QString& presetIconFile, int pixSize, QPixmap* pixmap)
{
    // Try to search the icon file base name in the directory containing the presets file
    if (!pixmap) {
        return false;
    }
    QString presetIconFilePath = presetIconFile;
    QString path = presetFilePath;
    int foundSlash = path.lastIndexOf(QLatin1Char('/'));
    if (foundSlash != -1) {
        path = path.mid(0, foundSlash + 1);
    }
    presetIconFilePath = path + presetIconFilePath;

    if (QFile::exists(presetIconFilePath)) {
        pixmap->load(presetIconFilePath);
        if (!pixmap->isNull()) {
            if ( (std::max( pixmap->width(), pixmap->height() ) != pixSize) && !pixmap->isNull() ) {
                *pixmap = pixmap->scaled(pixSize, pixSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            }
        }
    }
    return !pixmap->isNull();
}

std::list<ToolButton*>
Gui::getToolButtonsOrdered() const
{
    ///First-off find the tool buttons that should be ordered
    ///and put in another list the rest
    std::list<ToolButton*> namedToolButtons;
    std::list<ToolButton*> otherToolButtons;

    for (int n = 0; n < NAMED_PLUGIN_GROUP_NO; ++n) {
        for (U32 i = 0; i < _imp->_toolButtons.size(); ++i) {
            if ( _imp->_toolButtons[i]->hasChildren() && !_imp->_toolButtons[i]->getPluginToolButton()->getParent() ) {
                std::string toolButtonName = _imp->_toolButtons[i]->getLabel().toStdString();

                if (n == 0) {
                    ///he first time register unnamed buttons
                    bool isNamedToolButton = false;
                    for (int j = 0; j < NAMED_PLUGIN_GROUP_NO; ++j) {
                        if (toolButtonName == namedGroupsOrdered[j]) {
                            isNamedToolButton = true;
                            break;
                        }
                    }
                    if (!isNamedToolButton) {
                        otherToolButtons.push_back(_imp->_toolButtons[i]);
                    }
                }
                if (toolButtonName == namedGroupsOrdered[n]) {
                    namedToolButtons.push_back(_imp->_toolButtons[i]);
                }
            }
        }
    }
    namedToolButtons.insert( namedToolButtons.end(), otherToolButtons.begin(), otherToolButtons.end() );

    return namedToolButtons;
}

void
Gui::addToolButttonsToToolBar()
{
    std::list<ToolButton*> orederedToolButtons = getToolButtonsOrdered();

    for (std::list<ToolButton*>::iterator it = orederedToolButtons.begin(); it != orederedToolButtons.end(); ++it) {
        _imp->addToolButton(*it);
    }
}

void
Gui::setToolButtonMenuOpened(QToolButton* button)
{
    _imp->_toolButtonMenuOpened = button;
}

QToolButton*
Gui::getToolButtonMenuOpened() const
{
    return _imp->_toolButtonMenuOpened;
}

AppInstancePtr
Gui::createNewProject()
{
    CLArgs cl;
    AppInstancePtr app = appPTR->newAppInstance(cl, false);

    app->execOnProjectCreatedCallback();

    return app;
}

void
Gui::newProject()
{
    createNewProject();
}

void
Gui::openProject()
{
    std::vector<std::string> filters;

    filters.push_back(NATRON_PROJECT_FILE_EXT);
    std::string selectedFile =  popOpenFileDialog( false, filters, _imp->_lastLoadProjectOpenedDir.toStdString(), false );

    if ( !selectedFile.empty() ) {
        std::string patternCpy = selectedFile;
        std::string path = SequenceParsing::removePath(patternCpy);
        _imp->_lastLoadProjectOpenedDir = QString::fromUtf8( path.c_str() );
        AppInstancePtr appInstance = openProjectInternal(selectedFile, true);
        Q_UNUSED(appInstance);
    }
}

AppInstancePtr
Gui::openProject(const std::string & filename)
{
    return openProjectInternal(filename, true);
}

AppInstancePtr
Gui::openProjectInternal(const std::string & absoluteFileName,
                         bool attemptToLoadAutosave)
{
    QFileInfo file( QString::fromUtf8( absoluteFileName.c_str() ) );

    if ( !file.exists() ) {
        return AppInstancePtr();
    }
    QString fileUnPathed = file.fileName();
    QString path = file.path() + QLatin1Char('/');


    int openedProject = appPTR->isProjectAlreadyOpened(absoluteFileName);

    if (openedProject != -1) {
        AppInstancePtr instance = appPTR->getAppInstance(openedProject);
        if (instance) {
            GuiAppInstancePtr guiApp = toGuiAppInstance(instance);
            if (guiApp) {
                guiApp->getGui()->activateWindow();

                return instance;
            }
        }
    }

    AppInstancePtr ret;
    ProjectPtr project = getApp()->getProject();
    ///if the current graph has no value, just load the project in the same window
    if ( project->isGraphWorthLess() ) {
        bool ok = project->loadProject( path, fileUnPathed, false, attemptToLoadAutosave);
        if (ok) {
            ret = _imp->_appInstance.lock();
        }
    } else {
        CLArgs cl;
        AppInstancePtr newApp = appPTR->newAppInstance(cl, false);
        bool ok  = newApp->getProject()->loadProject( path, fileUnPathed, false, attemptToLoadAutosave);
        if (ok) {
            ret = newApp;
        }
    }

    QSettings settings;
    QStringList recentFiles = settings.value( QString::fromUtf8("recentFileList") ).toStringList();
    recentFiles.removeAll( QString::fromUtf8( absoluteFileName.c_str() ) );
    recentFiles.prepend( QString::fromUtf8( absoluteFileName.c_str() ) );
    while (recentFiles.size() > NATRON_MAX_RECENT_FILES) {
        recentFiles.removeLast();
    }

    settings.setValue(QString::fromUtf8("recentFileList"), recentFiles);
    appPTR->updateAllRecentFileMenus();

    return ret;
} // Gui::openProjectInternal

static void
updateRecentFiles(const QString & filename)
{
    QSettings settings;
    QStringList recentFiles = settings.value( QString::fromUtf8("recentFileList") ).toStringList();

    recentFiles.removeAll(filename);
    recentFiles.prepend(filename);
    while (recentFiles.size() > NATRON_MAX_RECENT_FILES) {
        recentFiles.removeLast();
    }

    settings.setValue(QString::fromUtf8("recentFileList"), recentFiles);
    appPTR->updateAllRecentFileMenus();
}

bool
Gui::saveProject()
{
    ProjectPtr project = getApp()->getProject();

    if ( project->hasProjectBeenSavedByUser() ) {
        QString projectFilename = project->getProjectFilename();
        QString projectPath = project->getProjectPath();

        if ( !_imp->checkProjectLockAndWarn(projectPath, projectFilename) ) {
            return false;
        }

        bool ret = project->saveProject(projectPath, projectFilename, 0);

        ///update the open recents
        if ( !projectPath.endsWith( QLatin1Char('/') ) ) {
            projectPath.append( QLatin1Char('/') );
        }
        if (ret) {
            QString file = projectPath + projectFilename;
            updateRecentFiles(file);
        }

        return ret;
    } else {
        return saveProjectAs();
    }
}

bool
Gui::saveProjectAs(const std::string& filename)
{
    std::string fileCopy = filename;

    if (fileCopy.find("." NATRON_PROJECT_FILE_EXT) == std::string::npos) {
        fileCopy.append("." NATRON_PROJECT_FILE_EXT);
    }
    std::string path = SequenceParsing::removePath(fileCopy);

    if ( !_imp->checkProjectLockAndWarn( QString::fromUtf8( path.c_str() ), QString::fromUtf8( fileCopy.c_str() ) ) ) {
        return false;
    }
    _imp->_lastSaveProjectOpenedDir = QString::fromUtf8( path.c_str() );

    bool ret = getApp()->getProject()->saveProject(QString::fromUtf8( path.c_str() ), QString::fromUtf8( fileCopy.c_str() ), 0);

    if (ret) {
        QString filePath = QString::fromUtf8( path.c_str() ) + QString::fromUtf8( fileCopy.c_str() );
        updateRecentFiles(filePath);
    }

    return ret;
}

bool
Gui::saveProjectAs()
{
    std::vector<std::string> filter;

    filter.push_back(NATRON_PROJECT_FILE_EXT);
    std::string outFile = popSaveFileDialog( false, filter, _imp->_lastSaveProjectOpenedDir.toStdString(), false );
    if (outFile.size() > 0) {
        return saveProjectAs(outFile);
    }

    return false;
}

void
Gui::saveAndIncrVersion()
{
    ProjectPtr project = getApp()->getProject();
    QString path = project->getProjectPath();
    QString name = project->getProjectFilename();
    int currentVersion = 0;
    int positionToInsertVersion;
    bool mustAppendFileExtension = false;

    // extension is everything after the last '.'
    int lastDotPos = name.lastIndexOf( QLatin1Char('.') );

    if (lastDotPos == -1) {
        positionToInsertVersion = name.size();
        mustAppendFileExtension = true;
    } else {
        //Extract the current version number if any
        QString versionStr;
        int i = lastDotPos - 1;
        while ( i >= 0 && name.at(i).isDigit() ) {
            versionStr.prepend( name.at(i) );
            --i;
        }

        ++i; //move back the head to the first digit

        if ( !versionStr.isEmpty() ) {
            name.remove( i, versionStr.size() );
            --i; //move 1 char backward, if the char is a '_' remove it
            if ( (i >= 0) && ( name.at(i) == QLatin1Char('_') ) ) {
                name.remove(i, 1);
            }
            currentVersion = versionStr.toInt();
        }

        positionToInsertVersion = i;
    }

    //Incr version
    ++currentVersion;

    QString newVersionStr = QString::number(currentVersion);

    //Add enough 0s in the beginning of the version number to have at least 3 digits
    int nb0s = 3 - newVersionStr.size();
    nb0s = std::max(0, nb0s);

    QString toInsert( QLatin1Char('_') );
    for (int c = 0; c < nb0s; ++c) {
        toInsert.append( QLatin1Char('0') );
    }
    toInsert.append(newVersionStr);
    if (mustAppendFileExtension) {
        toInsert.append( QString::fromUtf8("." NATRON_PROJECT_FILE_EXT) );
    }

    if ( positionToInsertVersion >= name.size() ) {
        name.append(toInsert);
    } else {
        name.insert(positionToInsertVersion, toInsert);
    }

    project->saveProject(path, name, 0);

    QString filename = path + QLatin1Char('/') + name;
    updateRecentFiles(filename);
} // Gui::saveAndIncrVersion

void
Gui::createNewViewer()
{
    NodeGraph* graph = _imp->_lastFocusedGraph ? _imp->_lastFocusedGraph : _imp->_nodeGraphArea;

    assert(graph);
    if (!graph) {
        throw std::logic_error("");
    }
    CreateNodeArgsPtr args(CreateNodeArgs::create(PLUGINID_NATRON_VIEWER_GROUP, graph->getGroup() ));
    args->setProperty<bool>(kCreateNodeArgsPropSubGraphOpened, false);
    ignore_result( getApp()->createNode(args) );
}

NodePtr
Gui::createReader()
{
    NodePtr ret;
    std::vector<std::string> filters;

    appPTR->getSupportedReaderFileFormats(&filters);

    std::string pattern = popOpenFileDialog( true, filters, _imp->_lastLoadSequenceOpenedDir.toStdString(), true );
    if ( !pattern.empty() ) {
        NodeGraph* graph = 0;
        if (_imp->_lastFocusedGraph) {
            graph = _imp->_lastFocusedGraph;
        } else {
            graph = _imp->_nodeGraphArea;
        }
        NodeCollectionPtr group = graph->getGroup();
        assert(group);

        CreateNodeArgsPtr args(CreateNodeArgs::create(PLUGINID_NATRON_READ, group));
        ret = getApp()->createReader(pattern, args);
    }

    return ret;
} // Gui::createReader

NodePtr
Gui::createWriter()
{
    NodePtr ret;
    std::vector<std::string> filters;

    appPTR->getSupportedWriterFileFormats(&filters);

    std::string file;
    bool useDialogForWriters = appPTR->getCurrentSettings()->isFileDialogEnabledForNewWriters();
    if (useDialogForWriters) {
        file = popSaveFileDialog( true, filters, _imp->_lastSaveSequenceOpenedDir.toStdString(), true );
        if ( file.empty() ) {
            return NodePtr();
        }
    }


    if ( !file.empty() ) {
        std::string patternCpy = file;
        std::string path = SequenceParsing::removePath(patternCpy);
        _imp->_lastSaveSequenceOpenedDir = QString::fromUtf8( path.c_str() );
    }

    NodeGraph* graph = 0;
    if (_imp->_lastFocusedGraph) {
        graph = _imp->_lastFocusedGraph;
    } else {
        graph = _imp->_nodeGraphArea;
    }
    NodeCollectionPtr group = graph->getGroup();
    assert(group);

    CreateNodeArgsPtr args(CreateNodeArgs::create(PLUGINID_NATRON_WRITE, group));
    ret =  getApp()->createWriter(file, args);


    return ret;
}

std::string
Gui::popOpenFileDialog(bool sequenceDialog,
                       const std::vector<std::string> & initialfilters,
                       const std::string & initialDir,
                       bool allowRelativePaths)
{
    SequenceFileDialog dialog(this, initialfilters, sequenceDialog, SequenceFileDialog::eFileDialogModeOpen, initialDir, this, allowRelativePaths);

    if ( dialog.exec() ) {
        if (sequenceDialog) {
            _imp->_lastLoadSequenceOpenedDir = dialog.currentDirectory().absolutePath();
        } else {
            _imp->_lastLoadProjectOpenedDir = dialog.currentDirectory().absolutePath();
        }
        return dialog.selectedFiles();
    } else {
        return std::string();
    }
}

std::string
Gui::openImageSequenceDialog()
{
    std::vector<std::string> filters;

    appPTR->getSupportedReaderFileFormats(&filters);

    return popOpenFileDialog(true, filters, _imp->_lastLoadSequenceOpenedDir.toStdString(), true);
}

std::string
Gui::saveImageSequenceDialog()
{
    std::vector<std::string> filters;

    appPTR->getSupportedWriterFileFormats(&filters);

    return popSaveFileDialog(true, filters, _imp->_lastSaveSequenceOpenedDir.toStdString(), true);
}

std::string
Gui::popSaveFileDialog(bool sequenceDialog,
                       const std::vector<std::string> & initialfilters,
                       const std::string & initialDir,
                       bool allowRelativePaths)
{
    SequenceFileDialog dialog(this, initialfilters, sequenceDialog, SequenceFileDialog::eFileDialogModeSave, initialDir, this, allowRelativePaths);

    if ( dialog.exec() ) {
        if (sequenceDialog) {
            _imp->_lastSaveSequenceOpenedDir = dialog.currentDirectory().absolutePath();
        } else {
            _imp->_lastSaveProjectOpenedDir = dialog.currentDirectory().absolutePath();
        }

        return dialog.filesToSave();
    } else {
        return "";
    }
}

void
Gui::autoSave()
{
    getApp()->getProject()->autoSave();
}

int
Gui::saveWarning()
{
    if ( !getApp()->getProject()->isSaveUpToDate() ) {
        StandardButtonEnum ret =  Dialogs::questionDialog(NATRON_APPLICATION_NAME, tr("Save changes to %1?").arg( getApp()->getProject()->getProjectFilename() ).toStdString(),
                                                          false,
                                                          StandardButtons(eStandardButtonSave | eStandardButtonDiscard | eStandardButtonCancel), eStandardButtonSave);
        if ( (ret == eStandardButtonEscape) || (ret == eStandardButtonCancel) ) {
            return 2;
        } else if (ret == eStandardButtonDiscard) {
            return 1;
        } else {
            return 0;
        }
    }

    return -1;
}

void
Gui::loadProjectGui(bool isAutosave, const SERIALIZATION_NAMESPACE::ProjectSerializationPtr& serialization) const
{
    if (_imp->_projectGui) {
        _imp->_projectGui->load(isAutosave, serialization);
    }
}


bool
Gui::isAboutToClose() const
{
    QMutexLocker l(&_imp->aboutToCloseMutex);

    return _imp->_aboutToClose;
}

NATRON_NAMESPACE_EXIT

