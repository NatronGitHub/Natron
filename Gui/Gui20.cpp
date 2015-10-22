/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
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

#include "Global/Macros.h"

#include <QtCore/QTextStream>
#include <QtCore/QSettings>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>

#include <QAction>

#include "Engine/CLArgs.h"
#include "Engine/KnobSerialization.h" // createDefaultValueForParam
#include "Engine/Lut.h" // Color, floatToInt
#include "Engine/Node.h"
#include "Engine/NodeGroup.h" // NodeGroup, NodeCollection, NodeList
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
#include "Gui/ViewerGL.h"
#include "Gui/ViewerTab.h"

#include "Global/QtCompat.h" // removeFileExtension

#include "SequenceParsing.h"

#define NAMED_PLUGIN_GROUP_NO 15

static std::string namedGroupsOrdered[NAMED_PLUGIN_GROUP_NO] = {
    PLUGIN_GROUP_IMAGE,
    PLUGIN_GROUP_COLOR,
    PLUGIN_GROUP_CHANNEL,
    PLUGIN_GROUP_MERGE,
    PLUGIN_GROUP_FILTER,
    PLUGIN_GROUP_TRANSFORM,
    PLUGIN_GROUP_TIME,
    PLUGIN_GROUP_PAINT,
    PLUGIN_GROUP_KEYER,
    PLUGIN_GROUP_MULTIVIEW,
    PLUGIN_GROUP_DEEP,
    PLUGIN_GROUP_3D,
    PLUGIN_GROUP_TOOLSETS,
    PLUGIN_GROUP_OTHER,
    PLUGIN_GROUP_DEFAULT
};

#define PLUGIN_GROUP_DEFAULT_ICON_PATH NATRON_IMAGES_PATH "GroupingIcons/Set" NATRON_ICON_SET_NUMBER "/other_grouping_" NATRON_ICON_SET_NUMBER ".png"


using namespace Natron;


namespace {
static void
getPixmapForGrouping(QPixmap* pixmap,
                     int size,
                     const QString & grouping)
{
    Natron::PixmapEnum e = Natron::NATRON_PIXMAP_OTHER_PLUGINS;
    if (grouping == PLUGIN_GROUP_COLOR) {
        e = Natron::NATRON_PIXMAP_COLOR_GROUPING;
    } else if (grouping == PLUGIN_GROUP_FILTER) {
        e = Natron::NATRON_PIXMAP_FILTER_GROUPING;
    } else if (grouping == PLUGIN_GROUP_IMAGE) {
        e = Natron::NATRON_PIXMAP_IO_GROUPING;
    } else if (grouping == PLUGIN_GROUP_TRANSFORM) {
        e = Natron::NATRON_PIXMAP_TRANSFORM_GROUPING;
    } else if (grouping == PLUGIN_GROUP_DEEP) {
        e = Natron::NATRON_PIXMAP_DEEP_GROUPING;
    } else if (grouping == PLUGIN_GROUP_MULTIVIEW) {
        e = Natron::NATRON_PIXMAP_MULTIVIEW_GROUPING;
    } else if (grouping == PLUGIN_GROUP_TIME) {
        e = Natron::NATRON_PIXMAP_TIME_GROUPING;
    } else if (grouping == PLUGIN_GROUP_PAINT) {
        e = Natron::NATRON_PIXMAP_PAINT_GROUPING;
    } else if (grouping == PLUGIN_GROUP_OTHER) {
        e = Natron::NATRON_PIXMAP_MISC_GROUPING;
    } else if (grouping == PLUGIN_GROUP_KEYER) {
        e = Natron::NATRON_PIXMAP_KEYER_GROUPING;
    } else if (grouping == PLUGIN_GROUP_TOOLSETS) {
        e = Natron::NATRON_PIXMAP_TOOLSETS_GROUPING;
    } else if (grouping == PLUGIN_GROUP_3D) {
        e = Natron::NATRON_PIXMAP_3D_GROUPING;
    } else if (grouping == PLUGIN_GROUP_CHANNEL) {
        e = Natron::NATRON_PIXMAP_CHANNEL_GROUPING;
    } else if (grouping == PLUGIN_GROUP_MERGE) {
        e = Natron::NATRON_PIXMAP_MERGE_GROUPING;
    }
    appPTR->getIcon(e, size, pixmap);
}
}



void
Gui::reloadStylesheet()
{
    loadStyleSheet();
}

void
Gui::loadStyleSheet()
{
    boost::shared_ptr<Settings> settings = appPTR->getCurrentSettings();

    QString selStr, sunkStr, baseStr, raisedStr, txtStr, intStr, kfStr, eStr, altStr, lightSelStr;

    //settings->
    {
        double r, g, b;
        settings->getSelectionColor(&r, &g, &b);
        double lr,lg,lb;
        lr = 1;
        lg = 0.75;
        lb = 0.47;
        selStr = QString("rgb(%1,%2,%3)").arg(Color::floatToInt<256>(r)).arg(Color::floatToInt<256>(g)).arg(Color::floatToInt<256>(b));
        lightSelStr = QString("rgb(%1,%2,%3)").arg(Color::floatToInt<256>(lr)).arg(Color::floatToInt<256>(lg)).arg(Color::floatToInt<256>(lb));
    }
    {
        double r, g, b;
        settings->getBaseColor(&r, &g, &b);
        baseStr = QString("rgb(%1,%2,%3)").arg(Color::floatToInt<256>(r)).arg(Color::floatToInt<256>(g)).arg(Color::floatToInt<256>(b));
    }
    {
        double r, g, b;
        settings->getRaisedColor(&r, &g, &b);
        raisedStr = QString("rgb(%1,%2,%3)").arg(Color::floatToInt<256>(r)).arg(Color::floatToInt<256>(g)).arg(Color::floatToInt<256>(b));
    }
    {
        double r, g, b;
        settings->getSunkenColor(&r, &g, &b);
        sunkStr = QString("rgb(%1,%2,%3)").arg(Color::floatToInt<256>(r)).arg(Color::floatToInt<256>(g)).arg(Color::floatToInt<256>(b));
    }
    {
        double r, g, b;
        settings->getTextColor(&r, &g, &b);
        txtStr = QString("rgb(%1,%2,%3)").arg(Color::floatToInt<256>(r)).arg(Color::floatToInt<256>(g)).arg(Color::floatToInt<256>(b));
    }
    {
        double r, g, b;
        settings->getInterpolatedColor(&r, &g, &b);
        intStr = QString("rgb(%1,%2,%3)").arg(Color::floatToInt<256>(r)).arg(Color::floatToInt<256>(g)).arg(Color::floatToInt<256>(b));
    }
    {
        double r, g, b;
        settings->getKeyframeColor(&r, &g, &b);
        kfStr = QString("rgb(%1,%2,%3)").arg(Color::floatToInt<256>(r)).arg(Color::floatToInt<256>(g)).arg(Color::floatToInt<256>(b));
    }
    {
        double r, g, b;
        settings->getExprColor(&r, &g, &b);
        eStr = QString("rgb(%1,%2,%3)").arg(Color::floatToInt<256>(r)).arg(Color::floatToInt<256>(g)).arg(Color::floatToInt<256>(b));
    }
    {
        double r, g, b;
        settings->getAltTextColor(&r, &g, &b);
        altStr = QString("rgb(%1,%2,%3)").arg(Color::floatToInt<256>(r)).arg(Color::floatToInt<256>(g)).arg(Color::floatToInt<256>(b));
    }

    QFile qss;
    std::string userQss = settings->getUserStyleSheetFilePath();
    if (!userQss.empty()) {
        qss.setFileName(userQss.c_str());
    } else {
        qss.setFileName(":/Resources/Stylesheets/mainstyle.qss");
    }

    if ( qss.open(QIODevice::ReadOnly
                  | QIODevice::Text) ) {
        QTextStream in(&qss);
        QString content( in.readAll() );
        setStyleSheet( content
                       .arg(selStr) // %1: selection-color
                       .arg(baseStr) // %2: medium background
                       .arg(raisedStr) // %3: soft background
                       .arg(sunkStr) // %4: strong background
                       .arg(txtStr) // %5: text colour
                       .arg(intStr) // %6: interpolated value color
                       .arg(kfStr) // %7: keyframe value color
                       .arg("rgb(0,0,0)") // %8: disabled editable text
                       .arg(eStr) // %9: expression background color
                       .arg(altStr)  // %10 = altered text color
                       .arg(lightSelStr)); // %11 = mouse over selection color
    } else {
        Natron::errorDialog(tr("Stylesheet").toStdString(), tr("Failure to load stylesheet file ").toStdString() + qss.fileName().toStdString());
    }
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
                if (tabName == kPropertiesBinName) {
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
    std::map<NodeGui*, RotoGui*> rotoNodes;
    std::list<NodeGui*> rotoNodesList;
    std::pair<NodeGui*, RotoGui*> currentRoto;
    std::map<NodeGui*, TrackerGui*> trackerNodes;
    std::list<NodeGui*> trackerNodesList;
    std::pair<NodeGui*, TrackerGui*> currentTracker;
    
    if (!viewer) {
        return 0;
    }
    
    //Don't create tracker & roto interface for file dialog preview viewer
    if (viewer->getNode()->getScriptName() != NATRON_FILE_DIALOG_PREVIEW_VIEWER_NAME) {
        if ( !_imp->_viewerTabs.empty() ) {
            ( *_imp->_viewerTabs.begin() )->getRotoContext(&rotoNodes, &currentRoto);
            ( *_imp->_viewerTabs.begin() )->getTrackerContext(&trackerNodes, &currentTracker);
        } else {
            const std::list<boost::shared_ptr<NodeGui> > & allNodes = _imp->_nodeGraphArea->getAllActiveNodes();
            for (std::list<boost::shared_ptr<NodeGui> >::const_iterator it = allNodes.begin(); it != allNodes.end(); ++it) {
                if ( (*it)->getNode()->getRotoContext() ) {
                    rotoNodesList.push_back( it->get() );
                    if (!currentRoto.first) {
                        currentRoto.first = it->get();
                    }
                } else if ( (*it)->getNode()->isPointTrackerNode() ) {
                    trackerNodesList.push_back( it->get() );
                    if (!currentTracker.first) {
                        currentTracker.first = it->get();
                    }
                }
            }
        }
    }
    for (std::map<NodeGui*, RotoGui*>::iterator it = rotoNodes.begin(); it != rotoNodes.end(); ++it) {
        rotoNodesList.push_back(it->first);
    }

    for (std::map<NodeGui*, TrackerGui*>::iterator it = trackerNodes.begin(); it != trackerNodes.end(); ++it) {
        trackerNodesList.push_back(it->first);
    }

    ViewerTab* tab = new ViewerTab(rotoNodesList, currentRoto.first, trackerNodesList, currentTracker.first, this, viewer, where);
    QObject::connect( tab->getViewer(), SIGNAL( imageChanged(int, bool) ), this, SLOT( onViewerImageChanged(int, bool) ) );
    {
        QMutexLocker l(&_imp->_viewerTabsMutex);
        _imp->_viewerTabs.push_back(tab);
    }
    where->appendTab(tab, tab);
    Q_EMIT viewersChanged();

    return tab;
}

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
    unregisterTab(tab);

    NodeGraph* graph = 0;
    NodeGroup* isGrp = 0;
    boost::shared_ptr<NodeCollection> collection;
    if ( tab->getInternalNode() && tab->getInternalNode()->getNode() ) {
        boost::shared_ptr<NodeCollection> collection = tab->getInternalNode()->getNode()->getGroup();
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

    ViewerTab* lastSelectedViewer = graph->getLastSelectedViewer();

    if (lastSelectedViewer == tab) {
        bool foundOne = false;
        NodeList nodes;
        if (collection) {
            nodes = collection->getNodes();
        }
        for (NodeList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
            ViewerInstance* isViewer = dynamic_cast<ViewerInstance*>( (*it)->getLiveInstance() );
            if ( !isViewer || ( isViewer == tab->getInternalNode() ) || !(*it)->isActivated() ) {
                continue;
            }
            OpenGLViewerI* viewerI = isViewer->getUiContext();
            assert(viewerI);
            ViewerGL* glViewer = dynamic_cast<ViewerGL*>(viewerI);
            assert(glViewer);
            graph->setLastSelectedViewer( glViewer->getViewerTab() );
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
        boost::shared_ptr<NodeGuiI> guiI = internalNode->getNodeGui();
        boost::shared_ptr<NodeGui> gui = boost::dynamic_pointer_cast<NodeGui>(guiI);
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
Gui::registerPyPanel(PyPanel* panel,
                     const std::string & pythonFunction)
{
    QMutexLocker l(&_imp->_pyPanelsMutex);
    std::map<PyPanel*, std::string>::iterator found = _imp->_userPanels.find(panel);

    if ( found == _imp->_userPanels.end() ) {
        _imp->_userPanels.insert( std::make_pair(panel, pythonFunction) );
    }
}

void
Gui::unregisterPyPanel(PyPanel* panel)
{
    QMutexLocker l(&_imp->_pyPanelsMutex);
    std::map<PyPanel*, std::string>::iterator found = _imp->_userPanels.find(panel);

    if ( found != _imp->_userPanels.end() ) {
        _imp->_userPanels.erase(found);
    }
}

std::map<PyPanel*, std::string>
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
Gui::findOrCreateToolButton(const boost::shared_ptr<PluginGroupNode> & plugin)
{
    if (!plugin->getIsUserCreatable() && plugin->getChildren().empty()) {
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

    QIcon toolButtonIcon,menuIcon;
    if ( !plugin->getIconPath().isEmpty() && QFile::exists( plugin->getIconPath() ) ) {
        QPixmap pix(plugin->getIconPath());
        int menuSize = NATRON_MEDIUM_BUTTON_ICON_SIZE;
        int toolButtonSize = !plugin->hasParent() ? NATRON_TOOL_BUTTON_ICON_SIZE : NATRON_MEDIUM_BUTTON_ICON_SIZE;
        
        QPixmap menuPix = pix,toolbuttonPix = pix;
        if (std::max(menuPix.width(), menuPix.height()) != menuSize) {
            menuPix = menuPix.scaled(menuSize, menuSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }
        if (std::max(toolbuttonPix.width(), toolbuttonPix.height()) != toolButtonSize) {
            toolbuttonPix = toolbuttonPix.scaled(toolButtonSize, toolButtonSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }
        menuIcon.addPixmap(menuPix);
        toolButtonIcon.addPixmap(toolbuttonPix);
    } else {
        //add the default group icon only if it has no parent
        if ( !plugin->hasParent() ) {
            QPixmap toolbuttonPix,menuPix;
            getPixmapForGrouping( &toolbuttonPix, NATRON_TOOL_BUTTON_ICON_SIZE, plugin->getLabel() );
            toolButtonIcon.addPixmap(toolbuttonPix);
            getPixmapForGrouping( &menuPix, NATRON_TOOL_BUTTON_ICON_SIZE, plugin->getLabel() );
            menuIcon.addPixmap(menuPix);

        }
    }
    //if the tool-button has no children, this is a leaf, we must create an action
    bool isLeaf = false;
    if ( plugin->getChildren().empty() ) {
        isLeaf = true;
        //if the plugin has no children and no parent, put it in the "others" group
        if ( !plugin->hasParent() ) {
            ToolButton* othersGroup = findExistingToolButton(PLUGIN_GROUP_DEFAULT);
            QStringList grouping(PLUGIN_GROUP_DEFAULT);
            QStringList iconGrouping(PLUGIN_GROUP_DEFAULT_ICON_PATH);
            boost::shared_ptr<PluginGroupNode> othersToolButton =
                appPTR->findPluginToolButtonOrCreate(grouping,
                                                     PLUGIN_GROUP_DEFAULT,
                                                     iconGrouping,
                                                     PLUGIN_GROUP_DEFAULT_ICON_PATH,
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
    ToolButton* pluginsToolButton = new ToolButton(_imp->_appInstance, plugin, plugin->getID(), plugin->getMajorVersion(),
                                                   plugin->getMinorVersion(),
                                                   plugin->getLabel(), toolButtonIcon, menuIcon);

    if (isLeaf) {
        QString label = plugin->getNotHighestMajorVersion() ? plugin->getLabelVersionMajorEncoded() : plugin->getLabel();
        assert(parentToolButton);
        QAction* action = new QAction(this);
        action->setText(label);
        action->setIcon( pluginsToolButton->getMenuIcon() );
        QObject::connect( action, SIGNAL( triggered() ), pluginsToolButton, SLOT( onTriggered() ) );
        pluginsToolButton->setAction(action);
    } else {
        Menu* menu = new Menu(this);
        //menu->setFont( QFont(appFont,appFontSize) );
        menu->setTitle( pluginsToolButton->getLabel() );
        menu->setIcon(menuIcon);
        pluginsToolButton->setMenu(menu);
        pluginsToolButton->setAction( menu->menuAction() );
    }

    if (!plugin->getParent() && pluginsToolButton->getLabel() == PLUGIN_GROUP_IMAGE) {
        ///create 2 special actions to create a reader and a writer so the user doesn't have to guess what
        ///plugin to choose for reading/writing images, let Natron deal with it. THe user can still change
        ///the behavior of Natron via the Preferences Readers/Writers tabs.
        QMenu* imageMenu = pluginsToolButton->getMenu();
        assert(imageMenu);
        QAction* createReaderAction = new QAction(this);
        QObject::connect( createReaderAction, SIGNAL( triggered() ), this, SLOT( createReader() ) );
        createReaderAction->setText( tr("Read") );
        QPixmap readImagePix;
        appPTR->getIcon(Natron::NATRON_PIXMAP_READ_IMAGE, NATRON_MEDIUM_BUTTON_ICON_SIZE, &readImagePix);
        createReaderAction->setIcon( QIcon(readImagePix) );
        createReaderAction->setShortcutContext(Qt::WidgetShortcut);
        createReaderAction->setShortcut( QKeySequence(Qt::Key_R) );
        imageMenu->addAction(createReaderAction);

        QAction* createWriterAction = new QAction(this);
        QObject::connect( createWriterAction, SIGNAL( triggered() ), this, SLOT( createWriter() ) );
        createWriterAction->setText( tr("Write") );
        QPixmap writeImagePix;
        appPTR->getIcon(Natron::NATRON_PIXMAP_WRITE_IMAGE, NATRON_MEDIUM_BUTTON_ICON_SIZE, &writeImagePix);
        createWriterAction->setIcon( QIcon(writeImagePix) );
        createWriterAction->setShortcutContext(Qt::WidgetShortcut);
        createWriterAction->setShortcut( QKeySequence(Qt::Key_W) );
        imageMenu->addAction(createWriterAction);
    }


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

AppInstance*
Gui::createNewProject()
{
    CLArgs cl;
    AppInstance* app = appPTR->newAppInstance(cl);
    
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
        _imp->_lastLoadProjectOpenedDir = path.c_str();
        AppInstance* app = openProjectInternal(selectedFile);
        if (!app) {
            throw std::runtime_error(tr("Failed to open project").toStdString() + ' ' + selectedFile);
        }
    }
}

AppInstance*
Gui::openProject(const std::string & filename)
{
    return openProjectInternal(filename);
}

AppInstance*
Gui::openProjectInternal(const std::string & absoluteFileName)
{
    QFileInfo file(absoluteFileName.c_str());
    if (!file.exists()) {
        return 0;
    }
    QString fileUnPathed = file.fileName();
    QString path = file.path() + "/";
    int openedProject = appPTR->isProjectAlreadyOpened(absoluteFileName);

    if (openedProject != -1) {
        AppInstance* instance = appPTR->getAppInstance(openedProject);
        if (instance) {
            GuiAppInstance* guiApp = dynamic_cast<GuiAppInstance*>(instance);
            assert(guiApp);
            if (guiApp) {
                guiApp->getGui()->activateWindow();
                return instance;
            }
        }
    }

    AppInstance* ret = 0;
    ///if the current graph has no value, just load the project in the same window
    if ( _imp->_appInstance->getProject()->isGraphWorthLess() ) {
      bool ok = _imp->_appInstance->getProject()->loadProject( path, fileUnPathed);
        if (ok) {
            ret = _imp->_appInstance;
        }
    } else {
        CLArgs cl;
        AppInstance* newApp = appPTR->newAppInstance(cl);
        bool ok  = newApp->getProject()->loadProject( path, fileUnPathed);
        if (ok) {
            ret = newApp;
        }
    }

    QSettings settings;
    QStringList recentFiles = settings.value("recentFileList").toStringList();
    recentFiles.removeAll( absoluteFileName.c_str() );
    recentFiles.prepend( absoluteFileName.c_str() );
    while (recentFiles.size() > NATRON_MAX_RECENT_FILES) {
        recentFiles.removeLast();
    }

    settings.setValue("recentFileList", recentFiles);
    appPTR->updateAllRecentFileMenus();
    return ret;
}

static void
updateRecentFiles(const QString & filename)
{
    QSettings settings;
    QStringList recentFiles = settings.value("recentFileList").toStringList();

    recentFiles.removeAll(filename);
    recentFiles.prepend(filename);
    while (recentFiles.size() > NATRON_MAX_RECENT_FILES) {
        recentFiles.removeLast();
    }

    settings.setValue("recentFileList", recentFiles);
    appPTR->updateAllRecentFileMenus();
}

bool
Gui::saveProject()
{
    boost::shared_ptr<Natron::Project> project= _imp->_appInstance->getProject();
    if (project->hasProjectBeenSavedByUser()) {
        
        
        QString projectName = project->getProjectName();
        QString projectPath = project->getProjectPath();
        
        if (!_imp->checkProjectLockAndWarn(projectPath,projectName)) {
            return false;
        }

        bool ret = project->saveProject(projectPath, projectName, 0);

        ///update the open recents
        if (!projectPath.endsWith('/')) {
            projectPath.append('/');
        }
        if (ret) {
            QString file = projectPath + projectName;
            updateRecentFiles(file);
        }
        return ret;
        
    } else {
        return saveProjectAs();
    }
}

bool
Gui::saveProjectAs()
{
    std::vector<std::string> filter;

    filter.push_back(NATRON_PROJECT_FILE_EXT);
    std::string outFile = popSaveFileDialog( false, filter, _imp->_lastSaveProjectOpenedDir.toStdString(), false );
    if (outFile.size() > 0) {
        if (outFile.find("." NATRON_PROJECT_FILE_EXT) == std::string::npos) {
            outFile.append("." NATRON_PROJECT_FILE_EXT);
        }
        std::string path = SequenceParsing::removePath(outFile);
        
        if (!_imp->checkProjectLockAndWarn(path.c_str(),outFile.c_str())) {
            return false;
        }
        _imp->_lastSaveProjectOpenedDir = path.c_str();
        
        bool ret = _imp->_appInstance->getProject()->saveProject(path.c_str(), outFile.c_str(), 0);

        if (ret) {
            QString filePath = QString( path.c_str() ) + QString( outFile.c_str() );
            updateRecentFiles(filePath);
        }

        return ret;
    }

    return false;
}

void
Gui::saveAndIncrVersion()
{
    QString path = _imp->_appInstance->getProject()->getProjectPath();
    QString name = _imp->_appInstance->getProject()->getProjectName();
    int currentVersion = 0;
    int positionToInsertVersion;
    bool mustAppendFileExtension = false;

    // extension is everything after the last '.'
    int lastDotPos = name.lastIndexOf('.');

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
            if ( (i >= 0) && ( name.at(i) == QChar('_') ) ) {
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

    QString toInsert("_");
    for (int c = 0; c < nb0s; ++c) {
        toInsert.append('0');
    }
    toInsert.append(newVersionStr);
    if (mustAppendFileExtension) {
        toInsert.append("." NATRON_PROJECT_FILE_EXT);
    }

    if ( positionToInsertVersion >= name.size() ) {
        name.append(toInsert);
    } else {
        name.insert(positionToInsertVersion, toInsert);
    }

    _imp->_appInstance->getProject()->saveProject(path, name, 0);

    QString filename = path + "/" + name;
    updateRecentFiles(filename);
} // Gui::saveAndIncrVersion

void
Gui::createNewViewer()
{
    NodeGraph* graph = _imp->_lastFocusedGraph ? _imp->_lastFocusedGraph : _imp->_nodeGraphArea;

    assert(graph);
    ignore_result( _imp->_appInstance->createNode( CreateNodeArgs( PLUGINID_NATRON_VIEWER,
                                                                   "",
                                                                   -1, -1,
                                                                   true,
                                                                   INT_MIN, INT_MIN,
                                                                   true,
                                                                   true,
                                                                   true,
                                                                   QString(),
                                                                   CreateNodeArgs::DefaultValuesList(),
                                                                   graph->getGroup() ) ) );
}

boost::shared_ptr<Natron::Node>
Gui::createReader()
{
    boost::shared_ptr<Natron::Node> ret;
    std::map<std::string, std::string> readersForFormat;

    appPTR->getCurrentSettings()->getFileFormatsForReadingAndReader(&readersForFormat);
    std::vector<std::string> filters;
    for (std::map<std::string, std::string>::const_iterator it = readersForFormat.begin(); it != readersForFormat.end(); ++it) {
        filters.push_back(it->first);
    }
    std::string pattern = popOpenFileDialog( true, filters, _imp->_lastLoadSequenceOpenedDir.toStdString(), true );
    if ( !pattern.empty() ) {
        
        QString qpattern( pattern.c_str() );
        
        std::string patternCpy = pattern;
        std::string path = SequenceParsing::removePath(patternCpy);
        _imp->_lastLoadSequenceOpenedDir = path.c_str();
        
        std::string ext = Natron::removeFileExtension(qpattern).toLower().toStdString();
        std::map<std::string, std::string>::iterator found = readersForFormat.find(ext);
        if ( found == readersForFormat.end() ) {
            errorDialog( tr("Reader").toStdString(), tr("No plugin capable of decoding ").toStdString() + ext + tr(" was found.").toStdString(), false);
        } else {
            NodeGraph* graph = 0;
            if (_imp->_lastFocusedGraph) {
                graph = _imp->_lastFocusedGraph;
            } else {
                graph = _imp->_nodeGraphArea;
            }
            boost::shared_ptr<NodeCollection> group = graph->getGroup();
            assert(group);

            CreateNodeArgs::DefaultValuesList defaultValues;
            defaultValues.push_back( createDefaultValueForParam<std::string>(kOfxImageEffectFileParamName, pattern) );
            CreateNodeArgs args(found->second.c_str(),
                                "",
                                -1, -1,
                                true,
                                INT_MIN, INT_MIN,
                                true,
                                true,
                                true,
                                QString(),
                                defaultValues,
                                group);
            ret = _imp->_appInstance->createNode(args);

            if (!ret) {
                return ret;
            }
        }
    }

    return ret;
}

boost::shared_ptr<Natron::Node>
Gui::createWriter()
{
    boost::shared_ptr<Natron::Node> ret;
    std::map<std::string, std::string> writersForFormat;

    appPTR->getCurrentSettings()->getFileFormatsForWritingAndWriter(&writersForFormat);
    std::vector<std::string> filters;
    for (std::map<std::string, std::string>::const_iterator it = writersForFormat.begin(); it != writersForFormat.end(); ++it) {
        filters.push_back(it->first);
    }
    std::string file = popSaveFileDialog( true, filters, _imp->_lastSaveSequenceOpenedDir.toStdString(), true );
    if ( !file.empty() ) {
        
        std::string patternCpy = file;
        std::string path = SequenceParsing::removePath(patternCpy);
        _imp->_lastSaveSequenceOpenedDir = path.c_str();
        
        NodeGraph* graph = 0;
        if (_imp->_lastFocusedGraph) {
            graph = _imp->_lastFocusedGraph;
        } else {
            graph = _imp->_nodeGraphArea;
        }
        boost::shared_ptr<NodeCollection> group = graph->getGroup();
        assert(group);

        ret =  getApp()->createWriter(file, group, true);
    }

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
        return dialog.selectedFiles();
    } else {
        return std::string();
    }
}

std::string
Gui::openImageSequenceDialog()
{
    std::map<std::string, std::string> readersForFormat;

    appPTR->getCurrentSettings()->getFileFormatsForReadingAndReader(&readersForFormat);
    std::vector<std::string> filters;
    for (std::map<std::string, std::string>::const_iterator it = readersForFormat.begin(); it != readersForFormat.end(); ++it) {
        filters.push_back(it->first);
    }

    return popOpenFileDialog(true, filters, _imp->_lastLoadSequenceOpenedDir.toStdString(), true);
}

std::string
Gui::saveImageSequenceDialog()
{
    std::map<std::string, std::string> writersForFormat;

    appPTR->getCurrentSettings()->getFileFormatsForWritingAndWriter(&writersForFormat);
    std::vector<std::string> filters;
    for (std::map<std::string, std::string>::const_iterator it = writersForFormat.begin(); it != writersForFormat.end(); ++it) {
        filters.push_back(it->first);
    }

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
        return dialog.filesToSave();
    } else {
        return "";
    }
}

void
Gui::autoSave()
{
    _imp->_appInstance->getProject()->autoSave();
}

int
Gui::saveWarning()
{
    if ( !_imp->_appInstance->getProject()->isSaveUpToDate() ) {
        Natron::StandardButtonEnum ret =  Natron::questionDialog(NATRON_APPLICATION_NAME, tr("Save changes to ").toStdString() +
                                                                 _imp->_appInstance->getProject()->getProjectName().toStdString() + " ?",
                                                                 false,
                                                                 Natron::StandardButtons(Natron::eStandardButtonSave | Natron::eStandardButtonDiscard | Natron::eStandardButtonCancel), Natron::eStandardButtonSave);
        if ( (ret == Natron::eStandardButtonEscape) || (ret == Natron::eStandardButtonCancel) ) {
            return 2;
        } else if (ret == Natron::eStandardButtonDiscard) {
            return 1;
        } else {
            return 0;
        }
    }

    return -1;
}

void
Gui::loadProjectGui(boost::archive::xml_iarchive & obj) const
{
    assert(_imp->_projectGui);
    _imp->_projectGui->load(obj/*, version*/);
}

void
Gui::saveProjectGui(boost::archive::xml_oarchive & archive)
{
    assert(_imp->_projectGui);
    _imp->_projectGui->save(archive/*, version*/);
}

bool
Gui::isAboutToClose() const
{
    QMutexLocker l(&_imp->aboutToCloseMutex);
    return _imp->_aboutToClose;
}

