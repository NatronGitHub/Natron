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
#include "Engine/KnobSerialization.h" // createDefaultValueForParam
#include "Engine/Lut.h" // Color, floatToInt
#include "Engine/Node.h"
#include "Engine/NodeGroup.h" // NodeGroup, NodeCollection, NodesList
#include "Engine/Project.h"
#include "Engine/Settings.h"
#include "Engine/ViewerInstance.h"

#include "Gui/CurveEditor.h"
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
#include "utils/util.hpp"

#include "Global/QtCompat.h" // removeFileExtension
#ifdef DEBUG
#include "Global/FloatingPointExceptions.h"
#endif

#include <SequenceParsing.h> // for removePath

#define NAMED_PLUGIN_GROUP_NO 15

#define PLUGIN_GROUP_DEFAULT_ICON_PATH NATRON_IMAGES_PATH "GroupingIcons/Set" NATRON_ICON_SET_NUMBER "/other_grouping_" NATRON_ICON_SET_NUMBER ".png"

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
    QString style = fread(":/Resources/Stylesheets/mainstyle.qss");
    qApp->setStyleSheet(style);

//     SettingsPtr settings = appPTR->getCurrentSettings();
//     QColor selCol, sunkCol, baseCol, raisedCol, txtCol, intCol, kfCol, disCol, eCol, altCol, lightSelCol;

//     //settings->
//     {
//         double r, g, b;
//         settings->getSelectionColor(&r, &g, &b);
//         selCol = to_qcolor(r, g, b);
//     }
//     {
//         double r, g, b;
//         r = 1;
//         g = 0.75;
//         b = 0.47;
//         lightSelCol = to_qcolor(r, g, b);
//     }
//     {
//         double r, g, b;
//         settings->getBaseColor(&r, &g, &b);
//         baseCol = to_qcolor(r, g, b);
//     }
//     {
//         double r, g, b;
//         settings->getRaisedColor(&r, &g, &b);
//         raisedCol = to_qcolor(r, g, b);
//     }
//     {
//         double r, g, b;
//         settings->getSunkenColor(&r, &g, &b);
//         sunkCol = to_qcolor(r, g, b);
//     }
//     {
//         double r, g, b;
//         settings->getTextColor(&r, &g, &b);
//         txtCol = to_qcolor(r, g, b);
//     }
//     {
//         double r, g, b;
//         settings->getInterpolatedColor(&r, &g, &b);
//         intCol = to_qcolor(r, g, b);
//     }
//     {
//         double r, g, b;
//         settings->getKeyframeColor(&r, &g, &b);
//         kfCol = to_qcolor(r, g, b);
//     }
//     {
//         double r, g, b;
//         r = g = b = 0.;
//         disCol = to_qcolor(r, g, b);
//     }
//     {
//         double r, g, b;
//         settings->getExprColor(&r, &g, &b);
//         eCol = to_qcolor(r, g, b);
//     }
//     {
//         double r, g, b;
//         settings->getAltTextColor(&r, &g, &b);
//         altCol = to_qcolor(r, g, b);
//     }

//     // First, set the global palette colors
//     // Note: the following colors may be wrong, please test and fix these on Linux with an empty mainstyle.css
//     QPalette p = qApp->palette();

//     p.setBrush( QPalette::Window, sunkCol );
//     p.setBrush( QPalette::WindowText, txtCol );
//     p.setBrush( QPalette::Base, baseCol );
//     //p.setBrush( QPalette::ToolTipBase, baseCol );
//     //p.setBrush( QPalette::ToolTipText, txtCol );
//     p.setBrush( QPalette::Text, txtCol );
//     p.setBrush( QPalette::Button, baseCol );
//     p.setBrush( QPalette::ButtonText, txtCol );
//     p.setBrush( QPalette::Light, raisedCol );
//     p.setBrush( QPalette::Dark, sunkCol );
//     p.setBrush( QPalette::Mid, baseCol );
//     //p.setBrush( QPalette::Shadow, sunkCol );
//     p.setBrush( QPalette::BrightText, txtCol );
//     p.setBrush( QPalette::Link, selCol ); // can only be set via palette
//     p.setBrush( QPalette::LinkVisited, selCol ); // can only be set via palette
//     {
// #ifdef DEBUG
//         boost_adaptbx::floating_point::exception_trapping trap(0);
// #endif
//         qApp->setPalette( p );
//     }

//     QFile qss;
//     std::string userQss = settings->getUserStyleSheetFilePath();
//     if ( !userQss.empty() ) {
//         qss.setFileName( QString::fromUtf8( userQss.c_str() ) );
//     } else {
//         qss.setFileName( QString::fromUtf8(":/Resources/Stylesheets/mainstyle.qss") );
//     }

//     if ( qss.open(QIODevice::ReadOnly
//                   | QIODevice::Text) ) {
//         QTextStream in(&qss);
//         QString content = QString::fromUtf8("QWidget { font-family: \"%1\"; font-size: %2pt; }\n"
//                                             "QListView { font-family: \"%1\"; font-size: %2pt; }\n" // .. or the items in the QListView get the wrong font
//                                             "QComboBox::drop-down { font-family: \"%1\"; font-size: %2pt; }\n" // ... or the drop-down doesn't get the right font
//                                             "QInputDialog { font-family: \"%1\"; font-size: %2pt; }\n" // ... or the label doesn't get the right font
//                                             ).arg(appFont).arg(appFontSize);
//         content += in.readAll();
// #ifdef DEBUG
//         boost_adaptbx::floating_point::exception_trapping trap(0);
// #endif
//         qApp->setStyleSheet( content
//                              .arg( qcolor_to_qstring(selCol) ) // %1: selection-color
//                              .arg( qcolor_to_qstring(baseCol) ) // %2: medium background
//                              .arg( qcolor_to_qstring(raisedCol) ) // %3: soft background
//                              .arg( qcolor_to_qstring(sunkCol) ) // %4: strong background
//                              .arg( qcolor_to_qstring(txtCol) ) // %5: text colour
//                              .arg( qcolor_to_qstring(intCol) ) // %6: interpolated value color
//                              .arg( qcolor_to_qstring(kfCol) ) // %7: keyframe value color
//                              .arg( qcolor_to_qstring(disCol) ) // %8: disabled editable text
//                              .arg( qcolor_to_qstring(eCol) ) // %9: expression background color
//                              .arg( qcolor_to_qstring(altCol) ) // %10 = altered text color
//                              .arg( qcolor_to_qstring(lightSelCol) ) ); // %11 = mouse over selection color
//     } else {
//         Dialogs::errorDialog( tr("Stylesheet").toStdString(), tr("Failure to load stylesheet file ").toStdString() + qss.fileName().toStdString() );
//     }
} // Gui::loadStyleSheet

void
Gui::maximize(TabWidget* what)
{
    assert(what);
    if ( what->isFloatingWindowChild() ) {
        return;
    }

    QMutexLocker l(&_imp->_panesMutex);
    for (std::list<TabWidget*>::iterator it = _imp->_panes.begin(); it != _imp->_panes.end(); ++it) {
        //if the widget is not what we want to maximize and it is not floating , hide it
        if ( (*it != what) && !(*it)->isFloatingWindowChild() ) {
            // also if we want to maximize the workshop pane, don't hide the properties pane

            bool hasProperties = false;
            for (int i = 0; i < (*it)->count(); ++i) {
                QString tabName = (*it)->tabAt(i)->getWidget()->objectName();
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
                CurveEditor* isEditor = dynamic_cast<CurveEditor*>(tab);
                if (isGraph || isEditor) {
                    hasNodeGraphOrCurveEditor = true;
                    break;
                }
            }

            if (hasProperties && hasNodeGraphOrCurveEditor) {
                continue;
            }
            (*it)->hide();
        }
    }
}

void
Gui::minimize()
{
    QMutexLocker l(&_imp->_panesMutex);

    for (std::list<TabWidget*>::iterator it = _imp->_panes.begin(); it != _imp->_panes.end(); ++it) {
        (*it)->show();
    }
}

ViewerTab*
Gui::addNewViewerTab(ViewerInstance* viewer,
                     TabWidget* where)
{
    if (!viewer) {
        return 0;
    }

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

    ViewerTab* tab = new ViewerTab(nodeViewerUi, activeNodeViewerUi, this, viewer, where);
    QObject::connect( tab->getViewer(), SIGNAL(imageChanged(int,bool)), this, SLOT(onViewerImageChanged(int,bool)) );
    {
        QMutexLocker l(&_imp->_viewerTabsMutex);
        _imp->_viewerTabs.push_back(tab);
    }
    where->appendTab(tab, tab);
    Q_EMIT viewersChanged();

    return tab;
} // Gui::addNewViewerTab

void
Gui::onViewerImageChanged(int texIndex,
                          bool hasImageBackend)
{
    ///notify all histograms a viewer image changed
    ViewerGL* viewer = qobject_cast<ViewerGL*>( sender() );

    if (viewer) {
        QMutexLocker l(&_imp->_histogramsMutex);
        for (std::list<Histogram*>::iterator it = _imp->_histograms.begin(); it != _imp->_histograms.end(); ++it) {
            (*it)->onViewerImageChanged(viewer, texIndex, hasImageBackend);
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
Gui::registerFloatingWindow(FloatingWidget* window)
{
    QMutexLocker k(&_imp->_floatingWindowMutex);
    std::list<FloatingWidget*>::iterator found = std::find(_imp->_floatingWindows.begin(), _imp->_floatingWindows.end(), window);

    if ( found == _imp->_floatingWindows.end() ) {
        _imp->_floatingWindows.push_back(window);
    }
}

void
Gui::unregisterFloatingWindow(FloatingWidget* window)
{
    QMutexLocker k(&_imp->_floatingWindowMutex);
    std::list<FloatingWidget*>::iterator found = std::find(_imp->_floatingWindows.begin(), _imp->_floatingWindows.end(), window);

    if ( found != _imp->_floatingWindows.end() ) {
        _imp->_floatingWindows.erase(found);
    }
}

std::list<FloatingWidget*>
Gui::getFloatingWindows() const
{
    QMutexLocker l(&_imp->_floatingWindowMutex);

    return _imp->_floatingWindows;
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
    tab->abortRendering();
    NodeGraph* graph = 0;
    NodeGroup* isGrp = 0;
    NodeCollectionPtr collection;
    if ( tab->getInternalNode() && tab->getInternalNode()->getNode() ) {
        NodeCollectionPtr collection = tab->getInternalNode()->getNode()->getGroup();
        isGrp = dynamic_cast<NodeGroup*>( collection.get() );
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
            ViewerInstance* isViewer = (*it)->isEffectViewer();
            if ( !isViewer || ( isViewer == tab->getInternalNode() ) || !(*it)->isActivated() ) {
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

    ViewerInstance* internalViewer = tab->getInternalNode();
    if (internalViewer) {
        if (getApp()->getLastViewerUsingTimeline() == internalViewer) {
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
    Histogram* h = new Histogram(this);
    QMutexLocker l(&_imp->_histogramsMutex);
    std::stringstream ss;

    ss << _imp->_nextHistogramIndex;

    h->setScriptName( "histogram" + ss.str() );
    h->setLabel( "Histogram" + ss.str() );
    ++_imp->_nextHistogramIndex;
    _imp->_histograms.push_back(h);

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
Gui::unregisterPane(TabWidget* pane)
{
    {
        QMutexLocker l(&_imp->_panesMutex);
        std::list<TabWidget*>::iterator found = std::find(_imp->_panes.begin(), _imp->_panes.end(), pane);

        if ( found != _imp->_panes.end() ) {
            if (_imp->_lastEnteredTabWidget == pane) {
                _imp->_lastEnteredTabWidget = 0;
            }
            _imp->_panes.erase(found);
        }

        if ( ( pane->isAnchor() ) && !_imp->_panes.empty() ) {
            _imp->_panes.front()->setAsAnchor(true);
        }
    }
    checkNumberOfNonFloatingPanes();
}

void
Gui::checkNumberOfNonFloatingPanes()
{
    QMutexLocker l(&_imp->_panesMutex);
    ///If dropping to 1 non floating pane, make it non closable:floatable
    int nbNonFloatingPanes;
    TabWidget* nonFloatingPane = _imp->getOnly1NonFloatingPane(nbNonFloatingPanes);

    ///When there's only 1 tab left make it closable/floatable again
    if (nbNonFloatingPanes == 1) {
        assert(nonFloatingPane);
        nonFloatingPane->setClosable(false);
    } else {
        for (std::list<TabWidget*>::iterator it = _imp->_panes.begin(); it != _imp->_panes.end(); ++it) {
            (*it)->setClosable(true);
        }
    }
}

void
Gui::registerPane(TabWidget* pane)
{
    {
        QMutexLocker l(&_imp->_panesMutex);
        bool hasAnchor = false;

        for (std::list<TabWidget*>::iterator it = _imp->_panes.begin(); it != _imp->_panes.end(); ++it) {
            if ( (*it)->isAnchor() ) {
                hasAnchor = true;
                break;
            }
        }
        std::list<TabWidget*>::iterator found = std::find(_imp->_panes.begin(), _imp->_panes.end(), pane);

        if ( found == _imp->_panes.end() ) {
            if ( _imp->_panes.empty() ) {
                _imp->_leftRightSplitter->addWidget(pane);
                pane->setClosable(false);
            }
            _imp->_panes.push_back(pane);

            if (!hasAnchor) {
                pane->setAsAnchor(true);
            }
        }
    }
    checkNumberOfNonFloatingPanes();
}

void
Gui::registerSplitter(Splitter* s)
{
    QMutexLocker l(&_imp->_splittersMutex);
    std::list<Splitter*>::iterator found = std::find(_imp->_splitters.begin(), _imp->_splitters.end(), s);

    if ( found == _imp->_splitters.end() ) {
        _imp->_splitters.push_back(s);
    }
}

void
Gui::unregisterSplitter(Splitter* s)
{
    QMutexLocker l(&_imp->_splittersMutex);
    std::list<Splitter*>::iterator found = std::find(_imp->_splitters.begin(), _imp->_splitters.end(), s);

    if ( found != _imp->_splitters.end() ) {
        _imp->_splitters.erase(found);
    }
}

void
Gui::registerPyPanel(NATRON_PYTHON_NAMESPACE::PyPanel* panel,
                     const std::string & pythonFunction)
{
    QMutexLocker l(&_imp->_pyPanelsMutex);
    std::map<NATRON_PYTHON_NAMESPACE::PyPanel*, std::string>::iterator found = _imp->_userPanels.find(panel);

    if ( found == _imp->_userPanels.end() ) {
        _imp->_userPanels.insert( std::make_pair(panel, pythonFunction) );
    }
}

void
Gui::unregisterPyPanel(NATRON_PYTHON_NAMESPACE::PyPanel* panel)
{
    QMutexLocker l(&_imp->_pyPanelsMutex);
    std::map<NATRON_PYTHON_NAMESPACE::PyPanel*, std::string>::iterator found = _imp->_userPanels.find(panel);

    if ( found != _imp->_userPanels.end() ) {
        _imp->_userPanels.erase(found);
    }
}

std::map<NATRON_PYTHON_NAMESPACE::PyPanel*, std::string>
Gui::getPythonPanels() const
{
    QMutexLocker l(&_imp->_pyPanelsMutex);

    return _imp->_userPanels;
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
Gui::findOrCreateToolButton(const PluginGroupNodePtr & plugin)
{
    if ( !plugin->getIsUserCreatable() && plugin->getChildren().empty() ) {
        return 0;
    }

    for (U32 i = 0; i < _imp->_toolButtons.size(); ++i) {
        if (_imp->_toolButtons[i]->getPluginToolButton() == plugin) {
            return _imp->_toolButtons[i];
        }
    }

    //first-off create the tool-button's parent, if any
    ToolButton* parentToolButton = NULL;
    if ( plugin->hasParent() ) {
        assert(plugin->getParent() != plugin);
        if (plugin->getParent() != plugin) {
            parentToolButton = findOrCreateToolButton( plugin->getParent() );
        }
    }

    QIcon toolButtonIcon, menuIcon;
    if ( !plugin->getIconPath().isEmpty() && QFile::exists( plugin->getIconPath() ) ) {
        QPixmap pix( plugin->getIconPath() );
        int menuSize = TO_DPIX(NATRON_MEDIUM_BUTTON_ICON_SIZE);
        int toolButtonSize = !plugin->hasParent() ? TO_DPIX(NATRON_TOOL_BUTTON_ICON_SIZE) : TO_DPIX(NATRON_MEDIUM_BUTTON_ICON_SIZE);
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
        //add the default group icon only if it has no parent
        if ( !plugin->hasParent() ) {
            QPixmap toolbuttonPix, menuPix;
            getPixmapForGrouping( &toolbuttonPix, TO_DPIX(NATRON_TOOL_BUTTON_ICON_SIZE), plugin->getLabel() );
            toolButtonIcon.addPixmap(toolbuttonPix);
            getPixmapForGrouping( &menuPix, TO_DPIX(NATRON_TOOL_BUTTON_ICON_SIZE), plugin->getLabel() );
            menuIcon.addPixmap(menuPix);
        }
    }
    //if the tool-button has no children, this is a leaf, we must create an action
    bool isLeaf = false;
    if ( plugin->getChildren().empty() ) {
        isLeaf = true;
        //if the plugin has no children and no parent, put it in the "others" group
        if ( !plugin->hasParent() ) {
            ToolButton* othersGroup = findExistingToolButton( QString::fromUtf8(PLUGIN_GROUP_DEFAULT) );
            QStringList grouping( QString::fromUtf8(PLUGIN_GROUP_DEFAULT) );
            QStringList iconGrouping( QString::fromUtf8(PLUGIN_GROUP_DEFAULT_ICON_PATH) );
            PluginGroupNodePtr othersToolButton =
                appPTR->findPluginToolButtonOrCreate(grouping,
                                                     QString::fromUtf8(PLUGIN_GROUP_DEFAULT),
                                                     iconGrouping,
                                                     QString::fromUtf8(PLUGIN_GROUP_DEFAULT_ICON_PATH),
                                                     1,
                                                     0,
                                                     true);
            othersToolButton->tryAddChild(plugin);

            //if the othersGroup doesn't exist, create it
            if (!othersGroup) {
                othersGroup = findOrCreateToolButton(othersToolButton);
            }
            parentToolButton = othersGroup;
        }
    }
    ToolButton* pluginsToolButton = new ToolButton(getApp(), plugin, plugin->getID(), plugin->getMajorVersion(),
                                                   plugin->getMinorVersion(),
                                                   plugin->getLabel(), toolButtonIcon, menuIcon);

    if (isLeaf) {
        QString pluginLabelText = plugin->getNotHighestMajorVersion() ? plugin->getLabelVersionMajorEncoded() : plugin->getLabel();
        // see doc of QMenubar: convert & to &&
        pluginLabelText.replace(QString::fromUtf8("&"), QString::fromUtf8("&&"));
        assert(parentToolButton);
        QAction* action = new QAction(this);
        action->setText(pluginLabelText);
        action->setIcon( pluginsToolButton->getMenuIcon() );
        QObject::connect( action, SIGNAL(triggered()), pluginsToolButton, SLOT(onTriggered()) );
        pluginsToolButton->setAction(action);
    } else {
        Menu* menu = new Menu(this);
        //menu->setFont( QFont(appFont,appFontSize) );
        QString pluginsToolButtonTitle = pluginsToolButton->getLabel();
        // see doc of QMenubar: convert & to &&
        pluginsToolButtonTitle.replace(QString::fromUtf8("&"), QString::fromUtf8("&&"));
        menu->setTitle(pluginsToolButtonTitle);
        menu->setIcon(menuIcon);
        pluginsToolButton->setMenu(menu);
        pluginsToolButton->setAction( menu->menuAction() );
    }

#ifndef NATRON_ENABLE_IO_META_NODES
    if ( !plugin->getParent() && ( pluginsToolButton->getLabel() == QString::fromUtf8(PLUGIN_GROUP_IMAGE) ) ) {
        ///create 2 special actions to create a reader and a writer so the user doesn't have to guess what
        ///plugin to choose for reading/writing images, let Natron deal with it. THe user can still change
        ///the behavior of Natron via the Preferences Readers/Writers tabs.
        QMenu* imageMenu = pluginsToolButton->getMenu();
        assert(imageMenu);
        QAction* createReaderAction = new QAction(this);
        QObject::connect( createReaderAction, SIGNAL(triggered()), this, SLOT(createReader()) );
        createReaderAction->setText( tr("Read") );
        QPixmap readImagePix;
        appPTR->getIcon(NATRON_PIXMAP_READ_IMAGE, TO_DPIX(NATRON_MEDIUM_BUTTON_ICON_SIZE), &readImagePix);
        createReaderAction->setIcon( QIcon(readImagePix) );
        createReaderAction->setShortcutContext(Qt::WidgetShortcut);
        createReaderAction->setShortcut( QKeySequence(Qt::Key_R) );
        imageMenu->addAction(createReaderAction);

        QAction* createWriterAction = new QAction(this);
        QObject::connect( createWriterAction, SIGNAL(triggered()), this, SLOT(createWriter()) );
        createWriterAction->setText( tr("Write") );
        QPixmap writeImagePix;
        appPTR->getIcon(NATRON_PIXMAP_WRITE_IMAGE, TO_DPIX(NATRON_MEDIUM_BUTTON_ICON_SIZE), &writeImagePix);
        createWriterAction->setIcon( QIcon(writeImagePix) );
        createWriterAction->setShortcutContext(Qt::WidgetShortcut);
        createWriterAction->setShortcut( QKeySequence(Qt::Key_W) );
        imageMenu->addAction(createWriterAction);
    }
#endif


    //if it has a parent, add the new tool button as a child
    if (parentToolButton) {
        parentToolButton->tryAddChild(pluginsToolButton);
    }
    _imp->_toolButtons.push_back(pluginsToolButton);

    return pluginsToolButton;
} // findOrCreateToolButton

std::list<ToolButton*>
Gui::getToolButtonsOrdered() const
{
    ///First-off find the tool buttons that should be ordered
    ///and put in another list the rest
    std::list<ToolButton*> namedToolButtons;
    std::list<ToolButton*> otherToolButtons;

    for (int n = 0; n < NAMED_PLUGIN_GROUP_NO; ++n) {
        for (U32 i = 0; i < _imp->_toolButtons.size(); ++i) {
            if ( _imp->_toolButtons[i]->hasChildren() && !_imp->_toolButtons[i]->getPluginToolButton()->hasParent() ) {
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
            GuiAppInstance* guiApp = dynamic_cast<GuiAppInstance*>( instance.get() );
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
    CreateNodeArgs args(PLUGINID_NATRON_VIEWER, graph->getGroup() );
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

#ifdef NATRON_ENABLE_IO_META_NODES
        CreateNodeArgs args(PLUGINID_NATRON_READ, group);
        ret = getApp()->createReader(pattern, args);
#else

        QString qpattern = QString::fromUtf8( pattern.c_str() );
        std::string patternCpy = pattern;
        std::string path = SequenceParsing::removePath(patternCpy);
        _imp->_lastLoadSequenceOpenedDir = QString::fromUtf8( path.c_str() );

        QString ext_qs = QtCompat::removeFileExtension(qpattern).toLower();
        std::string ext = ext_qs.toStdString();
        std::map<std::string, std::string>::iterator found = readersForFormat.find(ext);
        if ( found == readersForFormat.end() ) {
            errorDialog( tr("Reader").toStdString(), tr("No plugin capable of decoding \"%1\" files was found.").arg(ext_qs).toStdString(), false);
        } else {
            CreateNodeArgs args(found->second.c_str(), group);
            args.addParamDefaultValue(kOfxImageEffectFileParamName, pattern);
            std::string canonicalFilename = pattern;
            getApp()->getProject()->canonicalizePath(canonicalFilename);
            int firstFrame, lastFrame;
            Node::getOriginalFrameRangeForReader(found->second, canonicalFilename, &firstFrame, &lastFrame);
            args.paramValues.push_back( createDefaultValueForParam(kReaderParamNameOriginalFrameRange, firstFrame, lastFrame) );


            ret = getApp()->createNode(args);

            if (!ret) {
                return ret;
            }
        }
#endif
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
#ifdef NATRON_ENABLE_IO_META_NODES
    bool useDialogForWriters = appPTR->getCurrentSettings()->isFileDialogEnabledForNewWriters();
#else
    bool useDialogForWriters = true;
#endif
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

    CreateNodeArgs args(PLUGINID_NATRON_WRITE, group);
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
Gui::loadProjectGui(bool isAutosave, boost::archive::xml_iarchive & obj) const
{
    assert(_imp->_projectGui);
    _imp->_projectGui->load(isAutosave, obj);
}

void
Gui::saveProjectGui(boost::archive::xml_oarchive & archive)
{
    assert(_imp->_projectGui);
    _imp->_projectGui->save(archive);
}

bool
Gui::isAboutToClose() const
{
    QMutexLocker l(&_imp->aboutToCloseMutex);

    return _imp->_aboutToClose;
}

NATRON_NAMESPACE_EXIT

