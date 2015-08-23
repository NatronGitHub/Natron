//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "Gui.h"

#include <cassert>
#include <sstream> // stringstream
#include <algorithm> // min, max
#include <map>
#include <list>
#include <utility>

#include "Global/Macros.h"

#include <QtCore/QTextStream>
#include <QtCore/QSettings>
#include <QtCore/QDebug>
#include <QtCore/QTimer>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>

#include <QAction>
#include <QApplication> // qApp
#include <QGridLayout>
CLANG_DIAG_OFF(unused-private-field)
// /opt/local/include/QtGui/qmime.h:119:10: warning: private field 'type' is not used [-Wunused-private-field]
#include <QKeyEvent>
CLANG_DIAG_ON(unused-private-field)
#include <QCheckBox>
#include <QMenuBar>
#include <QProgressDialog>
#include <QTextEdit>
#include <QUndoGroup>

#include <cairo/cairo.h>

#include <boost/version.hpp>

#include "Engine/GroupOutput.h"
#include "Engine/Image.h"
#include "Engine/KnobSerialization.h" // createDefaultValueForParam
#include "Engine/Lut.h" // floatToInt
#include "Engine/Node.h"
#include "Engine/NodeGroup.h" // NodeGroup, NodeCollection, NodeList
#include "Engine/Project.h"
#include "Engine/Settings.h"
#include "Engine/ViewerInstance.h"

#include "Gui/AboutWindow.h"
#include "Gui/ActionShortcuts.h"
#include "Gui/CurveEditor.h"
#include "Gui/DockablePanel.h"
#include "Gui/DopeSheetEditor.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h" // appPTR
#include "Gui/GuiPrivate.h"
#include "Gui/Histogram.h"
#include "Gui/Menu.h"
#include "Gui/MessageBox.h"
#include "Gui/NodeGraph.h"
#include "Gui/NodeGui.h"
#include "Gui/NodeSettingsPanel.h"
#include "Gui/PreferencesPanel.h"
#include "Gui/ProjectGui.h"
#include "Gui/RenderingProgressDialog.h"
#include "Gui/ResizableMessageBox.h"
#include "Gui/RightClickableWidget.h"
#include "Gui/RotoGui.h"
#include "Gui/ScriptEditor.h"
#include "Gui/SequenceFileDialog.h"
#include "Gui/ShortCutEditor.h"
#include "Gui/SpinBox.h"
#include "Gui/Splitter.h"
#include "Gui/TabWidget.h"
#include "Gui/ToolButton.h"
#include "Gui/Utils.h" // convertFromPlainText
#include "Gui/ViewerGL.h"
#include "Gui/ViewerTab.h"

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
                     const QString & grouping)
{
    if (grouping == PLUGIN_GROUP_COLOR) {
        appPTR->getIcon(Natron::NATRON_PIXMAP_COLOR_GROUPING, pixmap);
    } else if (grouping == PLUGIN_GROUP_FILTER) {
        appPTR->getIcon(Natron::NATRON_PIXMAP_FILTER_GROUPING, pixmap);
    } else if (grouping == PLUGIN_GROUP_IMAGE) {
        appPTR->getIcon(Natron::NATRON_PIXMAP_IO_GROUPING, pixmap);
    } else if (grouping == PLUGIN_GROUP_TRANSFORM) {
        appPTR->getIcon(Natron::NATRON_PIXMAP_TRANSFORM_GROUPING, pixmap);
    } else if (grouping == PLUGIN_GROUP_DEEP) {
        appPTR->getIcon(Natron::NATRON_PIXMAP_DEEP_GROUPING, pixmap);
    } else if (grouping == PLUGIN_GROUP_MULTIVIEW) {
        appPTR->getIcon(Natron::NATRON_PIXMAP_MULTIVIEW_GROUPING, pixmap);
    } else if (grouping == PLUGIN_GROUP_TIME) {
        appPTR->getIcon(Natron::NATRON_PIXMAP_TIME_GROUPING, pixmap);
    } else if (grouping == PLUGIN_GROUP_PAINT) {
        appPTR->getIcon(Natron::NATRON_PIXMAP_PAINT_GROUPING, pixmap);
    } else if (grouping == PLUGIN_GROUP_OTHER) {
        appPTR->getIcon(Natron::NATRON_PIXMAP_MISC_GROUPING, pixmap);
    } else if (grouping == PLUGIN_GROUP_KEYER) {
        appPTR->getIcon(Natron::NATRON_PIXMAP_KEYER_GROUPING, pixmap);
    } else if (grouping == PLUGIN_GROUP_TOOLSETS) {
        appPTR->getIcon(Natron::NATRON_PIXMAP_TOOLSETS_GROUPING, pixmap);
    } else if (grouping == PLUGIN_GROUP_3D) {
        appPTR->getIcon(Natron::NATRON_PIXMAP_3D_GROUPING, pixmap);
    } else if (grouping == PLUGIN_GROUP_CHANNEL) {
        appPTR->getIcon(Natron::NATRON_PIXMAP_CHANNEL_GROUPING, pixmap);
    } else if (grouping == PLUGIN_GROUP_MERGE) {
        appPTR->getIcon(Natron::NATRON_PIXMAP_MERGE_GROUPING, pixmap);
    } else {
        appPTR->getIcon(Natron::NATRON_PIXMAP_OTHER_PLUGINS, pixmap);
    }
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

    QString selStr, sunkStr, baseStr, raisedStr, txtStr, intStr, kfStr, eStr, altStr;

    //settings->
    {
        double r, g, b;
        settings->getSelectionColor(&r, &g, &b);
        selStr = QString("rgb(%1,%2,%3)").arg(Color::floatToInt<256>(r)).arg(Color::floatToInt<256>(g)).arg(Color::floatToInt<256>(b));
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

    QFile qss(":/Resources/Stylesheets/mainstyle.qss");

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
                       .arg(altStr) ); // %10 = altered text color
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
                QString tabName = (*it)->tabAt(i)->objectName();
                if (tabName == kPropertiesBinName) {
                    hasProperties = true;
                    break;
                }
            }

            bool hasNodeGraphOrCurveEditor = false;
            for (int i = 0; i < what->count(); ++i) {
                QWidget* tab = what->tabAt(i);
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
Gui::registerTab(QWidget* tab,
                 ScriptObject* obj)
{
    std::string name = obj->getScriptName();
    RegisteredTabs::iterator registeredTab = _imp->_registeredTabs.find(name);

    if ( registeredTab == _imp->_registeredTabs.end() ) {
        _imp->_registeredTabs.insert( std::make_pair( name, std::make_pair(tab, obj) ) );
    }
}

void
Gui::unregisterTab(QWidget* tab)
{
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
            tab->notifyAppClosing();
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

QWidget*
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
                     QWidget** w,
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
    if (!plugin->getIsUserCreatable()) {
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

    QIcon icon;
    if ( !plugin->getIconPath().isEmpty() && QFile::exists( plugin->getIconPath() ) ) {
        icon.addFile( plugin->getIconPath() );
    } else {
        //add the default group icon only if it has no parent
        if ( !plugin->hasParent() ) {
            QPixmap pix;
            getPixmapForGrouping( &pix, plugin->getLabel() );
            icon.addPixmap(pix);
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
            boost::shared_ptr<PluginGroupNode> othersToolButton =
                appPTR->findPluginToolButtonOrCreate(grouping,
                                                     PLUGIN_GROUP_DEFAULT,
                                                     PLUGIN_GROUP_DEFAULT_ICON_PATH,
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
                                                   plugin->getLabel(), icon);

    if (isLeaf) {
        QString label = plugin->getNotHighestMajorVersion() ? plugin->getLabelVersionMajorEncoded() : plugin->getLabel();
        assert(parentToolButton);
        QAction* action = new QAction(this);
        action->setText(label);
        action->setIcon( pluginsToolButton->getIcon() );
        QObject::connect( action, SIGNAL( triggered() ), pluginsToolButton, SLOT( onTriggered() ) );
        pluginsToolButton->setAction(action);
    } else {
        Menu* menu = new Menu(this);
        //menu->setFont( QFont(appFont,appFontSize) );
        menu->setTitle( pluginsToolButton->getLabel() );
        pluginsToolButton->setMenu(menu);
        pluginsToolButton->setAction( menu->menuAction() );
    }

    if (pluginsToolButton->getLabel() == PLUGIN_GROUP_IMAGE) {
        ///create 2 special actions to create a reader and a writer so the user doesn't have to guess what
        ///plugin to choose for reading/writing images, let Natron deal with it. THe user can still change
        ///the behavior of Natron via the Preferences Readers/Writers tabs.
        QMenu* imageMenu = pluginsToolButton->getMenu();
        assert(imageMenu);
        QAction* createReaderAction = new QAction(imageMenu);
        QObject::connect( createReaderAction, SIGNAL( triggered() ), this, SLOT( createReader() ) );
        createReaderAction->setText( tr("Read") );
        QPixmap readImagePix;
        appPTR->getIcon(Natron::NATRON_PIXMAP_READ_IMAGE, &readImagePix);
        createReaderAction->setIcon( QIcon(readImagePix) );
        createReaderAction->setShortcutContext(Qt::WidgetShortcut);
        createReaderAction->setShortcut( QKeySequence(Qt::Key_R) );
        imageMenu->addAction(createReaderAction);

        QAction* createWriterAction = new QAction(imageMenu);
        QObject::connect( createWriterAction, SIGNAL( triggered() ), this, SLOT( createWriter() ) );
        createWriterAction->setText( tr("Write") );
        QPixmap writeImagePix;
        appPTR->getIcon(Natron::NATRON_PIXMAP_WRITE_IMAGE, &writeImagePix);
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


void
Gui::newProject()
{
    CLArgs cl;
    AppInstance* app = appPTR->newAppInstance(cl);

    app->execOnProjectCreatedCallback();
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
        
        openProjectInternal(selectedFile);
    }
}

void
Gui::openProject(const std::string & filename)
{
    openProjectInternal(filename);
}

void
Gui::openProjectInternal(const std::string & absoluteFileName)
{
    QFileInfo file(absoluteFileName.c_str());
    if (!file.exists()) {
        return;
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

                return;
            }
        }
    }

    ///if the current graph has no value, just load the project in the same window
    if ( _imp->_appInstance->getProject()->isGraphWorthLess() ) {
        _imp->_appInstance->getProject()->loadProject( path, fileUnPathed);
    } else {
        CLArgs cl;
        AppInstance* newApp = appPTR->newAppInstance(cl);
        newApp->getProject()->loadProject( path, fileUnPathed);
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

        project->saveProject(projectPath, projectName, false);

        ///update the open recents
        if (!projectPath.endsWith('/')) {
            projectPath.append('/');
        }
        QString file = projectPath + projectName;
        updateRecentFiles(file);

        return true;
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
        
        _imp->_appInstance->getProject()->saveProject(path.c_str(), outFile.c_str(), false);

        QString filePath = QString( path.c_str() ) + QString( outFile.c_str() );
        updateRecentFiles(filePath);

        return true;
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

    _imp->_appInstance->getProject()->saveProject(path, name, false);

    QString filename = path = name;
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

void
Gui::errorDialog(const std::string & title,
                 const std::string & text,
                 bool useHtml)
{
    ///don't show dialogs when about to close, otherwise we could enter in a deadlock situation
    {
        QMutexLocker l(&_imp->aboutToCloseMutex);
        if (_imp->_aboutToClose) {
            return;
        }
    }


    Natron::StandardButtons buttons(Natron::eStandardButtonYes | Natron::eStandardButtonNo);

    if ( QThread::currentThread() != QCoreApplication::instance()->thread() ) {
        QMutexLocker locker(&_imp->_uiUsingMainThreadMutex);
        _imp->_uiUsingMainThread = true;
        locker.unlock();
        Q_EMIT doDialog(0, QString( title.c_str() ), QString( text.c_str() ), useHtml, buttons, (int)Natron::eStandardButtonYes);
        locker.relock();
        while (_imp->_uiUsingMainThread) {
            _imp->_uiUsingMainThreadCond.wait(&_imp->_uiUsingMainThreadMutex);
        }
    } else {
        Q_EMIT doDialog(0, QString( title.c_str() ), QString( text.c_str() ), useHtml, buttons, (int)Natron::eStandardButtonYes);
    }
}

void
Gui::errorDialog(const std::string & title,
                 const std::string & text,
                 bool* stopAsking,
                 bool useHtml)
{
    ///don't show dialogs when about to close, otherwise we could enter in a deadlock situation
    {
        QMutexLocker l(&_imp->aboutToCloseMutex);
        if (_imp->_aboutToClose) {
            return;
        }
    }

    Natron::StandardButtons buttons(Natron::eStandardButtonOk);

    if ( QThread::currentThread() != QCoreApplication::instance()->thread() ) {
        QMutexLocker locker(&_imp->_uiUsingMainThreadMutex);
        _imp->_uiUsingMainThread = true;
        locker.unlock();
        Q_EMIT doDialogWithStopAskingCheckbox( (int)MessageBox::eMessageBoxTypeError, QString( title.c_str() ), QString( text.c_str() ), useHtml, buttons, (int)Natron::eStandardButtonOk );
        locker.relock();
        while (_imp->_uiUsingMainThread) {
            _imp->_uiUsingMainThreadCond.wait(&_imp->_uiUsingMainThreadMutex);
        }
    } else {
        Q_EMIT doDialogWithStopAskingCheckbox( (int)MessageBox::eMessageBoxTypeError,
                                               QString( title.c_str() ), QString( text.c_str() ), useHtml, buttons, (int)Natron::eStandardButtonOk );
    }
    *stopAsking = _imp->_lastStopAskingAnswer;
}

void
Gui::warningDialog(const std::string & title,
                   const std::string & text,
                   bool useHtml)
{
    ///don't show dialogs when about to close, otherwise we could enter in a deadlock situation
    {
        QMutexLocker l(&_imp->aboutToCloseMutex);
        if (_imp->_aboutToClose) {
            return;
        }
    }

    Natron::StandardButtons buttons(Natron::eStandardButtonYes | Natron::eStandardButtonNo);

    if ( QThread::currentThread() != QCoreApplication::instance()->thread() ) {
        QMutexLocker locker(&_imp->_uiUsingMainThreadMutex);
        _imp->_uiUsingMainThread = true;
        locker.unlock();
        Q_EMIT doDialog(1, QString( title.c_str() ), QString( text.c_str() ), useHtml, buttons, (int)Natron::eStandardButtonYes);
        locker.relock();
        while (_imp->_uiUsingMainThread) {
            _imp->_uiUsingMainThreadCond.wait(&_imp->_uiUsingMainThreadMutex);
        }
    } else {
        Q_EMIT doDialog(1, QString( title.c_str() ), QString( text.c_str() ), useHtml, buttons, (int)Natron::eStandardButtonYes);
    }
}

void
Gui::warningDialog(const std::string & title,
                   const std::string & text,
                   bool* stopAsking,
                   bool useHtml)
{
    ///don't show dialogs when about to close, otherwise we could enter in a deadlock situation
    {
        QMutexLocker l(&_imp->aboutToCloseMutex);
        if (_imp->_aboutToClose) {
            return;
        }
    }

    Natron::StandardButtons buttons(Natron::eStandardButtonOk);

    if ( QThread::currentThread() != QCoreApplication::instance()->thread() ) {
        QMutexLocker locker(&_imp->_uiUsingMainThreadMutex);
        _imp->_uiUsingMainThread = true;
        locker.unlock();
        Q_EMIT doDialogWithStopAskingCheckbox( (int)MessageBox::eMessageBoxTypeWarning, QString( title.c_str() ), QString( text.c_str() ), useHtml, buttons, (int)Natron::eStandardButtonOk );
        locker.relock();
        while (_imp->_uiUsingMainThread) {
            _imp->_uiUsingMainThreadCond.wait(&_imp->_uiUsingMainThreadMutex);
        }
    } else {
        Q_EMIT doDialogWithStopAskingCheckbox( (int)MessageBox::eMessageBoxTypeWarning,
                                               QString( title.c_str() ), QString( text.c_str() ), useHtml, buttons, (int)Natron::eStandardButtonOk );
    }
    *stopAsking = _imp->_lastStopAskingAnswer;
}

void
Gui::informationDialog(const std::string & title,
                       const std::string & text,
                       bool useHtml)
{
    ///don't show dialogs when about to close, otherwise we could enter in a deadlock situation
    {
        QMutexLocker l(&_imp->aboutToCloseMutex);
        if (_imp->_aboutToClose) {
            return;
        }
    }

    Natron::StandardButtons buttons(Natron::eStandardButtonYes | Natron::eStandardButtonNo);

    if ( QThread::currentThread() != QCoreApplication::instance()->thread() ) {
        QMutexLocker locker(&_imp->_uiUsingMainThreadMutex);
        _imp->_uiUsingMainThread = true;
        locker.unlock();
        Q_EMIT doDialog(2, QString( title.c_str() ), QString( text.c_str() ), useHtml, buttons, (int)Natron::eStandardButtonYes);
        locker.relock();
        while (_imp->_uiUsingMainThread) {
            _imp->_uiUsingMainThreadCond.wait(&_imp->_uiUsingMainThreadMutex);
        }
    } else {
        Q_EMIT doDialog(2, QString( title.c_str() ), QString( text.c_str() ), useHtml, buttons, (int)Natron::eStandardButtonYes);
    }
}

void
Gui::informationDialog(const std::string & title,
                       const std::string & message,
                       bool* stopAsking,
                       bool useHtml)
{
    ///don't show dialogs when about to close, otherwise we could enter in a deadlock situation
    {
        QMutexLocker l(&_imp->aboutToCloseMutex);
        if (_imp->_aboutToClose) {
            return;
        }
    }

    Natron::StandardButtons buttons(Natron::eStandardButtonOk);

    if ( QThread::currentThread() != QCoreApplication::instance()->thread() ) {
        QMutexLocker locker(&_imp->_uiUsingMainThreadMutex);
        _imp->_uiUsingMainThread = true;
        locker.unlock();
        Q_EMIT doDialogWithStopAskingCheckbox( (int)MessageBox::eMessageBoxTypeInformation, QString( title.c_str() ), QString( message.c_str() ), useHtml, buttons, (int)Natron::eStandardButtonOk );
        locker.relock();
        while (_imp->_uiUsingMainThread) {
            _imp->_uiUsingMainThreadCond.wait(&_imp->_uiUsingMainThreadMutex);
        }
    } else {
        Q_EMIT doDialogWithStopAskingCheckbox( (int)MessageBox::eMessageBoxTypeInformation, QString( title.c_str() ), QString( message.c_str() ), useHtml, buttons, (int)Natron::eStandardButtonOk );
    }
    *stopAsking = _imp->_lastStopAskingAnswer;
}

void
Gui::onDoDialog(int type,
                const QString & title,
                const QString & content,
                bool useHtml,
                Natron::StandardButtons buttons,
                int defaultB)
{
    QString msg = useHtml ? content : Natron::convertFromPlainText(content.trimmed(), Qt::WhiteSpaceNormal);


    if (type == 0) { // error dialog
        QMessageBox critical(QMessageBox::Critical, title, msg, QMessageBox::NoButton, this, Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowStaysOnTopHint);
        critical.setWindowFlags(critical.windowFlags() | Qt::WindowStaysOnTopHint);
        critical.setTextFormat(Qt::RichText);   //this is what makes the links clickable
        ignore_result( critical.exec() );
    } else if (type == 1) { // warning dialog
        QMessageBox warning(QMessageBox::Warning, title, msg, QMessageBox::NoButton, this, Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowStaysOnTopHint);
        warning.setTextFormat(Qt::RichText);
        warning.setWindowFlags(warning.windowFlags() | Qt::WindowStaysOnTopHint);
        ignore_result( warning.exec() );
    } else if (type == 2) { // information dialog
        if (msg.count() < 1000) {
            QMessageBox info(QMessageBox::Information, title, msg, QMessageBox::NoButton, this, Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint| Qt::WindowStaysOnTopHint);
            info.setTextFormat(Qt::RichText);
            info.setWindowFlags(info.windowFlags() | Qt::WindowStaysOnTopHint);
            ignore_result( info.exec() );
        } else {
            // text may be very long: use resizable QMessageBox
            ResizableMessageBox info(QMessageBox::Information, title, msg.left(1000), QMessageBox::NoButton, this, Qt::Dialog | Qt::WindowStaysOnTopHint);
            info.setTextFormat(Qt::RichText);
            info.setWindowFlags(info.windowFlags() | Qt::WindowStaysOnTopHint);
            QGridLayout *layout = qobject_cast<QGridLayout *>( info.layout() );
            if (layout) {
                QTextEdit *edit = new QTextEdit();
                edit->setReadOnly(true);
                edit->setAcceptRichText(true);
                edit->setHtml(msg);
                layout->setRowStretch(1, 0);
                layout->addWidget(edit, 0, 1);
            }
            ignore_result( info.exec() );
        }
    } else { // question dialog
        assert(type == 3);
        QMessageBox ques(QMessageBox::Question, title, msg, QtEnumConvert::toQtStandarButtons(buttons),
                         this, Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowStaysOnTopHint);
        ques.setDefaultButton( QtEnumConvert::toQtStandardButton( (Natron::StandardButtonEnum)defaultB ) );
        ques.setWindowFlags(ques.windowFlags() | Qt::WindowStaysOnTopHint);
        if ( ques.exec() ) {
            _imp->_lastQuestionDialogAnswer = QtEnumConvert::fromQtStandardButton( ques.standardButton( ques.clickedButton() ) );
        }
    }

    QMutexLocker locker(&_imp->_uiUsingMainThreadMutex);
    _imp->_uiUsingMainThread = false;
    _imp->_uiUsingMainThreadCond.wakeOne();
}

Natron::StandardButtonEnum
Gui::questionDialog(const std::string & title,
                    const std::string & message,
                    bool useHtml,
                    Natron::StandardButtons buttons,
                    Natron::StandardButtonEnum defaultButton)
{
    ///don't show dialogs when about to close, otherwise we could enter in a deadlock situation
    {
        QMutexLocker l(&_imp->aboutToCloseMutex);
        if (_imp->_aboutToClose) {
            return Natron::eStandardButtonNo;
        }
    }

    if ( QThread::currentThread() != QCoreApplication::instance()->thread() ) {
        QMutexLocker locker(&_imp->_uiUsingMainThreadMutex);
        _imp->_uiUsingMainThread = true;
        locker.unlock();
        Q_EMIT doDialog(3, QString( title.c_str() ), QString( message.c_str() ), useHtml, buttons, (int)defaultButton);
        locker.relock();
        while (_imp->_uiUsingMainThread) {
            _imp->_uiUsingMainThreadCond.wait(&_imp->_uiUsingMainThreadMutex);
        }
    } else {
        Q_EMIT doDialog(3, QString( title.c_str() ), QString( message.c_str() ), useHtml, buttons, (int)defaultButton);
    }

    return _imp->_lastQuestionDialogAnswer;
}

Natron::StandardButtonEnum
Gui::questionDialog(const std::string & title,
                    const std::string & message,
                    bool useHtml,
                    Natron::StandardButtons buttons,
                    Natron::StandardButtonEnum defaultButton,
                    bool* stopAsking)
{
    ///don't show dialogs when about to close, otherwise we could enter in a deadlock situation
    {
        QMutexLocker l(&_imp->aboutToCloseMutex);
        if (_imp->_aboutToClose) {
            return Natron::eStandardButtonNo;
        }
    }

    if ( QThread::currentThread() != QCoreApplication::instance()->thread() ) {
        QMutexLocker locker(&_imp->_uiUsingMainThreadMutex);
        _imp->_uiUsingMainThread = true;
        locker.unlock();
        Q_EMIT onDoDialogWithStopAskingCheckbox( (int)Natron::MessageBox::eMessageBoxTypeQuestion,
                                                 QString( title.c_str() ), QString( message.c_str() ), useHtml, buttons, (int)defaultButton );
        locker.relock();
        while (_imp->_uiUsingMainThread) {
            _imp->_uiUsingMainThreadCond.wait(&_imp->_uiUsingMainThreadMutex);
        }
    } else {
        Q_EMIT onDoDialogWithStopAskingCheckbox( (int)Natron::MessageBox::eMessageBoxTypeQuestion,
                                                 QString( title.c_str() ), QString( message.c_str() ), useHtml, buttons, (int)defaultButton );
    }

    *stopAsking = _imp->_lastStopAskingAnswer;

    return _imp->_lastQuestionDialogAnswer;
}

void
Gui::onDoDialogWithStopAskingCheckbox(int type,
                                      const QString & title,
                                      const QString & content,
                                      bool useHtml,
                                      Natron::StandardButtons buttons,
                                      int defaultB)
{
    QString message = useHtml ? content : Natron::convertFromPlainText(content.trimmed(), Qt::WhiteSpaceNormal);
    Natron::MessageBox dialog(title, content, (Natron::MessageBox::MessageBoxTypeEnum)type, buttons, (Natron::StandardButtonEnum)defaultB, this);
    QCheckBox* stopAskingCheckbox = new QCheckBox(tr("Do Not Show This Again"), &dialog);

    dialog.setCheckBox(stopAskingCheckbox);
    dialog.setWindowFlags(dialog.windowFlags() | Qt::WindowStaysOnTopHint);
    if ( dialog.exec() ) {
        _imp->_lastQuestionDialogAnswer = dialog.getReply();
        _imp->_lastStopAskingAnswer = stopAskingCheckbox->isChecked();
    }
}

void
Gui::selectNode(boost::shared_ptr<NodeGui> node)
{
    if (!node) {
        return;
    }
    _imp->_nodeGraphArea->selectNode(node, false); //< wipe current selection
}

void
Gui::connectInput1()
{
    NodeGraph* graph = 0;

    if (_imp->_lastFocusedGraph) {
        graph = _imp->_lastFocusedGraph;
    } else {
        graph = _imp->_nodeGraphArea;
    }
    graph->connectCurrentViewerToSelection(0);
}

void
Gui::connectInput2()
{
    NodeGraph* graph = 0;

    if (_imp->_lastFocusedGraph) {
        graph = _imp->_lastFocusedGraph;
    } else {
        graph = _imp->_nodeGraphArea;
    }
    graph->connectCurrentViewerToSelection(1);
}

void
Gui::connectInput3()
{
    NodeGraph* graph = 0;

    if (_imp->_lastFocusedGraph) {
        graph = _imp->_lastFocusedGraph;
    } else {
        graph = _imp->_nodeGraphArea;
    }
    graph->connectCurrentViewerToSelection(2);
}

void
Gui::connectInput4()
{
    NodeGraph* graph = 0;

    if (_imp->_lastFocusedGraph) {
        graph = _imp->_lastFocusedGraph;
    } else {
        graph = _imp->_nodeGraphArea;
    }
    graph->connectCurrentViewerToSelection(3);
}

void
Gui::connectInput5()
{
    NodeGraph* graph = 0;

    if (_imp->_lastFocusedGraph) {
        graph = _imp->_lastFocusedGraph;
    } else {
        graph = _imp->_nodeGraphArea;
    }
    graph->connectCurrentViewerToSelection(4);
}

void
Gui::connectInput6()
{
    NodeGraph* graph = 0;

    if (_imp->_lastFocusedGraph) {
        graph = _imp->_lastFocusedGraph;
    } else {
        graph = _imp->_nodeGraphArea;
    }
    graph->connectCurrentViewerToSelection(5);
}

void
Gui::connectInput7()
{
    NodeGraph* graph = 0;

    if (_imp->_lastFocusedGraph) {
        graph = _imp->_lastFocusedGraph;
    } else {
        graph = _imp->_nodeGraphArea;
    }
    graph->connectCurrentViewerToSelection(6);
}

void
Gui::connectInput8()
{
    NodeGraph* graph = 0;

    if (_imp->_lastFocusedGraph) {
        graph = _imp->_lastFocusedGraph;
    } else {
        graph = _imp->_nodeGraphArea;
    }
    graph->connectCurrentViewerToSelection(7);
}

void
Gui::connectInput9()
{
    NodeGraph* graph = 0;

    if (_imp->_lastFocusedGraph) {
        graph = _imp->_lastFocusedGraph;
    } else {
        graph = _imp->_nodeGraphArea;
    }
    graph->connectCurrentViewerToSelection(8);
}

void
Gui::connectInput10()
{
    NodeGraph* graph = 0;

    if (_imp->_lastFocusedGraph) {
        graph = _imp->_lastFocusedGraph;
    } else {
        graph = _imp->_nodeGraphArea;
    }
    graph->connectCurrentViewerToSelection(9);
}

void
Gui::showView0()
{
    _imp->_appInstance->setViewersCurrentView(0);
}

void
Gui::showView1()
{
    _imp->_appInstance->setViewersCurrentView(1);
}

void
Gui::showView2()
{
    _imp->_appInstance->setViewersCurrentView(2);
}

void
Gui::showView3()
{
    _imp->_appInstance->setViewersCurrentView(3);
}

void
Gui::showView4()
{
    _imp->_appInstance->setViewersCurrentView(4);
}

void
Gui::showView5()
{
    _imp->_appInstance->setViewersCurrentView(5);
}

void
Gui::showView6()
{
    _imp->_appInstance->setViewersCurrentView(6);
}

void
Gui::showView7()
{
    _imp->_appInstance->setViewersCurrentView(7);
}

void
Gui::showView8()
{
    _imp->_appInstance->setViewersCurrentView(8);
}

void
Gui::showView9()
{
    _imp->_appInstance->setViewersCurrentView(9);
}

void
Gui::setCurveEditorOnTop()
{
    QMutexLocker l(&_imp->_panesMutex);

    for (std::list<TabWidget*>::iterator it = _imp->_panes.begin(); it != _imp->_panes.end(); ++it) {
        TabWidget* cur = (*it);
        assert(cur);
        for (int i = 0; i < cur->count(); ++i) {
            if (cur->tabAt(i) == _imp->_curveEditor) {
                cur->makeCurrentTab(i);
                break;
            }
        }
    }
}

void
Gui::showSettings()
{
    if (!_imp->_settingsGui) {
        _imp->_settingsGui = new PreferencesPanel(appPTR->getCurrentSettings(), this);
    }
    _imp->_settingsGui->show();
    _imp->_settingsGui->raise();
    _imp->_settingsGui->activateWindow();
}

void
Gui::registerNewUndoStack(QUndoStack* stack)
{
    _imp->_undoStacksGroup->addStack(stack);
    QAction* undo = stack->createUndoAction(stack);
    undo->setShortcut(QKeySequence::Undo);
    QAction* redo = stack->createRedoAction(stack);
    redo->setShortcut(QKeySequence::Redo);
    _imp->_undoStacksActions.insert( std::make_pair( stack, std::make_pair(undo, redo) ) );
}

void
Gui::removeUndoStack(QUndoStack* stack)
{
    std::map<QUndoStack*, std::pair<QAction*, QAction*> >::iterator it = _imp->_undoStacksActions.find(stack);
	if (it == _imp->_undoStacksActions.end()) {
		return;
	}
    if (_imp->_currentUndoAction == it->second.first) {
        _imp->menuEdit->removeAction(_imp->_currentUndoAction);
    }
    if (_imp->_currentRedoAction == it->second.second) {
        _imp->menuEdit->removeAction(_imp->_currentRedoAction);
    }
    if ( it != _imp->_undoStacksActions.end() ) {
        _imp->_undoStacksActions.erase(it);
    }
}

void
Gui::onCurrentUndoStackChanged(QUndoStack* stack)
{
    std::map<QUndoStack*, std::pair<QAction*, QAction*> >::iterator it = _imp->_undoStacksActions.find(stack);

    //the stack must have been registered first with registerNewUndoStack()
    if ( it != _imp->_undoStacksActions.end() ) {
        _imp->setUndoRedoActions(it->second.first, it->second.second);
    }
}

void
Gui::refreshAllPreviews()
{
    _imp->_appInstance->getProject()->refreshPreviews();
}

void
Gui::forceRefreshAllPreviews()
{
    _imp->_appInstance->getProject()->forceRefreshPreviews();
}

void
Gui::startDragPanel(QWidget* panel)
{
    assert(!_imp->_currentlyDraggedPanel);
    _imp->_currentlyDraggedPanel = panel;
}

QWidget*
Gui::stopDragPanel()
{
    assert(_imp->_currentlyDraggedPanel);
    QWidget* ret = _imp->_currentlyDraggedPanel;
    _imp->_currentlyDraggedPanel = 0;

    return ret;
}

void
Gui::showAbout()
{
    _imp->_aboutWindow->show();
    _imp->_aboutWindow->raise();
    _imp->_aboutWindow->activateWindow();
    ignore_result( _imp->_aboutWindow->exec() );
}

void
Gui::showShortcutEditor()
{
    _imp->shortcutEditor->show();
    _imp->shortcutEditor->raise();
    _imp->shortcutEditor->activateWindow();
}

void
Gui::openRecentFile()
{
    QAction *action = qobject_cast<QAction *>( sender() );

    if (action) {
        QFileInfo f( action->data().toString() );
        QString path = f.path() + '/';
        QString filename = path + f.fileName();
        int openedProject = appPTR->isProjectAlreadyOpened( filename.toStdString() );
        if (openedProject != -1) {
            AppInstance* instance = appPTR->getAppInstance(openedProject);
            if (instance) {
                GuiAppInstance* guiApp = dynamic_cast<GuiAppInstance*>(instance);
                assert(guiApp);
                if (guiApp) {
                    guiApp->getGui()->activateWindow();

                    return;
                }
            }
        }

        ///if the current graph has no value, just load the project in the same window
        if ( _imp->_appInstance->getProject()->isGraphWorthLess() ) {
            _imp->_appInstance->getProject()->loadProject( path, f.fileName() );
        } else {
            CLArgs cl;
            AppInstance* newApp = appPTR->newAppInstance(cl);
            newApp->getProject()->loadProject( path, f.fileName() );
        }
    }
}

void
Gui::updateRecentFileActions()
{
    QSettings settings;
    QStringList files = settings.value("recentFileList").toStringList();
    int numRecentFiles = std::min(files.size(), (int)NATRON_MAX_RECENT_FILES);

    for (int i = 0; i < numRecentFiles; ++i) {
        QString text = tr("&%1 %2").arg(i + 1).arg( QFileInfo(files[i]).fileName() );
        _imp->actionsOpenRecentFile[i]->setText(text);
        _imp->actionsOpenRecentFile[i]->setData(files[i]);
        _imp->actionsOpenRecentFile[i]->setVisible(true);
    }
    for (int j = numRecentFiles; j < NATRON_MAX_RECENT_FILES; ++j) {
        _imp->actionsOpenRecentFile[j]->setVisible(false);
    }
}

QPixmap
Gui::screenShot(QWidget* w)
{
#if QT_VERSION < 0x050000
    if (w->objectName() == "CurveEditor") {
        return QPixmap::grabWidget(w);
    }

    return QPixmap::grabWindow( w->winId() );
#else

    return QApplication::primaryScreen()->grabWindow( w->winId() );
#endif
}

void
Gui::onProjectNameChanged(const QString & name)
{
    QString text(QCoreApplication::applicationName() + " - ");

    text.append(name);
    setWindowTitle(text);
}

void
Gui::setColorPickersColor(double r,double g, double b,double a)
{
    assert(_imp->_projectGui);
    _imp->_projectGui->setPickersColor(r,g,b,a);
}

void
Gui::registerNewColorPicker(boost::shared_ptr<Color_Knob> knob)
{
    assert(_imp->_projectGui);
    _imp->_projectGui->registerNewColorPicker(knob);
}

void
Gui::removeColorPicker(boost::shared_ptr<Color_Knob> knob)
{
    assert(_imp->_projectGui);
    _imp->_projectGui->removeColorPicker(knob);
}

bool
Gui::hasPickers() const
{
    assert(_imp->_projectGui);

    return _imp->_projectGui->hasPickers();
}

void
Gui::updateViewersViewsMenu(int viewsCount)
{
    QMutexLocker l(&_imp->_viewerTabsMutex);

    for (std::list<ViewerTab*>::iterator it = _imp->_viewerTabs.begin(); it != _imp->_viewerTabs.end(); ++it) {
        (*it)->updateViewsMenu(viewsCount);
    }
}

void
Gui::setViewersCurrentView(int view)
{
    QMutexLocker l(&_imp->_viewerTabsMutex);

    for (std::list<ViewerTab*>::iterator it = _imp->_viewerTabs.begin(); it != _imp->_viewerTabs.end(); ++it) {
        (*it)->setCurrentView(view);
    }
}

const std::list<ViewerTab*> &
Gui::getViewersList() const
{
    return _imp->_viewerTabs;
}

std::list<ViewerTab*>
Gui::getViewersList_mt_safe() const
{
    QMutexLocker l(&_imp->_viewerTabsMutex);

    return _imp->_viewerTabs;
}

void
Gui::setMasterSyncViewer(ViewerTab* master)
{
    QMutexLocker l(&_imp->_viewerTabsMutex);
    _imp->_masterSyncViewer = master;
}

ViewerTab*
Gui::getMasterSyncViewer() const
{
    QMutexLocker l(&_imp->_viewerTabsMutex);
    return _imp->_masterSyncViewer;
}

void
Gui::activateViewerTab(ViewerInstance* viewer)
{
    OpenGLViewerI* viewport = viewer->getUiContext();

    {
        QMutexLocker l(&_imp->_viewerTabsMutex);
        for (std::list<ViewerTab*>::iterator it = _imp->_viewerTabs.begin(); it != _imp->_viewerTabs.end(); ++it) {
            if ( (*it)->getViewer() == viewport ) {
                TabWidget* viewerAnchor = getAnchor();
                assert(viewerAnchor);
                viewerAnchor->appendTab(*it, *it);
                (*it)->show();
            }
        }
    }
    Q_EMIT viewersChanged();
}

void
Gui::deactivateViewerTab(ViewerInstance* viewer)
{
    OpenGLViewerI* viewport = viewer->getUiContext();
    ViewerTab* v = 0;
    {
        QMutexLocker l(&_imp->_viewerTabsMutex);
        for (std::list<ViewerTab*>::iterator it = _imp->_viewerTabs.begin(); it != _imp->_viewerTabs.end(); ++it) {
            if ( (*it)->getViewer() == viewport ) {
                v = *it;
                break;
            }
        }
        
        if (v && v == _imp->_masterSyncViewer) {
            _imp->_masterSyncViewer = 0;
        }
    }

    if (v) {
        removeViewerTab(v, true, false);
    }
}

ViewerTab*
Gui::getViewerTabForInstance(ViewerInstance* node) const
{
    QMutexLocker l(&_imp->_viewerTabsMutex);

    for (std::list<ViewerTab*>::const_iterator it = _imp->_viewerTabs.begin(); it != _imp->_viewerTabs.end(); ++it) {
        if ( (*it)->getInternalNode() == node ) {
            return *it;
        }
    }

    return NULL;
}

const std::list<boost::shared_ptr<NodeGui> > &
Gui::getVisibleNodes() const
{
    return _imp->_nodeGraphArea->getAllActiveNodes();
}

std::list<boost::shared_ptr<NodeGui> >
Gui::getVisibleNodes_mt_safe() const
{
    return _imp->_nodeGraphArea->getAllActiveNodes_mt_safe();
}

void
Gui::deselectAllNodes() const
{
    _imp->_nodeGraphArea->deselect();
}

void
Gui::onProcessHandlerStarted(const QString & sequenceName,
                             int firstFrame,
                             int lastFrame,
                             const boost::shared_ptr<ProcessHandler> & process)
{
    ///make the dialog which will show the progress
    RenderingProgressDialog *dialog = new RenderingProgressDialog(this, sequenceName, firstFrame, lastFrame, process, this);
    QObject::connect(dialog,SIGNAL(accepted()),this,SLOT(onRenderProgressDialogFinished()));
    QObject::connect(dialog,SIGNAL(rejected()),this,SLOT(onRenderProgressDialogFinished()));
    dialog->show();
}

void
Gui::onRenderProgressDialogFinished()
{
    RenderingProgressDialog* dialog = qobject_cast<RenderingProgressDialog*>(sender());
    if (!dialog) {
        return;
    }
    dialog->close();
    dialog->deleteLater();
}

void
Gui::setNextViewerAnchor(TabWidget* where)
{
    _imp->_nextViewerTabPlace = where;
}

const std::vector<ToolButton*> &
Gui::getToolButtons() const
{
    return _imp->_toolButtons;
}

GuiAppInstance*
Gui::getApp() const
{
    return _imp->_appInstance;
}

const std::list<TabWidget*> &
Gui::getPanes() const
{
    return _imp->_panes;
}

std::list<TabWidget*>
Gui::getPanes_mt_safe() const
{
    QMutexLocker l(&_imp->_panesMutex);

    return _imp->_panes;
}

int
Gui::getPanesCount() const
{
    QMutexLocker l(&_imp->_panesMutex);

    return (int)_imp->_panes.size();
}

QString
Gui::getAvailablePaneName(const QString & baseName) const
{
    QString name = baseName;
    QMutexLocker l(&_imp->_panesMutex);
    int baseNumber = _imp->_panes.size();

    if ( name.isEmpty() ) {
        name.append("pane");
        name.append( QString::number(baseNumber) );
    }

    for (;; ) {
        bool foundName = false;
        for (std::list<TabWidget*>::const_iterator it = _imp->_panes.begin(); it != _imp->_panes.end(); ++it) {
            if ( (*it)->objectName_mt_safe() == name ) {
                foundName = true;
                break;
            }
        }
        if (foundName) {
            ++baseNumber;
            name = QString("pane%1").arg(baseNumber);
        } else {
            break;
        }
    }

    return name;
}

void
Gui::setDraftRenderEnabled(bool b)
{
    QMutexLocker k(&_imp->_isInDraftModeMutex);
    _imp->_isInDraftMode = b;
}

bool
Gui::isDraftRenderEnabled() const
{
    QMutexLocker k(&_imp->_isInDraftModeMutex);
    return _imp->_isInDraftMode;
}

bool
Gui::isDraggingPanel() const
{
    return _imp->_currentlyDraggedPanel != NULL;
}

NodeGraph*
Gui::getNodeGraph() const
{
    return _imp->_nodeGraphArea;
}

CurveEditor*
Gui::getCurveEditor() const
{
    return _imp->_curveEditor;
}

DopeSheetEditor *Gui::getDopeSheetEditor() const
{
    return _imp->_dopeSheetEditor;
}

ScriptEditor*
Gui::getScriptEditor() const
{
    return _imp->_scriptEditor;
}

PropertiesBinWrapper*
Gui::getPropertiesBin() const
{
    return _imp->_propertiesBin;
}

QVBoxLayout*
Gui::getPropertiesLayout() const
{
    return _imp->_layoutPropertiesBin;
}

void
Gui::appendTabToDefaultViewerPane(QWidget* tab,
                                  ScriptObject* obj)
{
    TabWidget* viewerAnchor = getAnchor();

    assert(viewerAnchor);
    viewerAnchor->appendTab(tab, obj);
}

QWidget*
Gui::getCentralWidget() const
{
    std::list<QWidget*> children;

    _imp->_leftRightSplitter->getChildren_mt_safe(children);
    if (children.size() != 2) {
        ///something is wrong
        return NULL;
    }
    for (std::list<QWidget*>::iterator it = children.begin(); it != children.end(); ++it) {
        if (*it == _imp->_toolBox) {
            continue;
        }

        return *it;
    }

    return NULL;
}

const RegisteredTabs &
Gui::getRegisteredTabs() const
{
    return _imp->_registeredTabs;
}

void
Gui::debugImage(const Natron::Image* image,
                const RectI& roi,
                const QString & filename )
{
    if (image->getBitDepth() != Natron::eImageBitDepthFloat) {
        qDebug() << "Debug image only works on float images.";
        return;
    }
    RectI renderWindow;
    RectI bounds = image->getBounds();
    if (roi.isNull()) {
        renderWindow = bounds;
    } else {
        if (!roi.intersect(bounds,&renderWindow)) {
            qDebug() << "The RoI does not interesect the bounds of the image.";
            return;
        }
    }
    QImage output(renderWindow.width(), renderWindow.height(), QImage::Format_ARGB32);
    const Natron::Color::Lut* lut = Natron::Color::LutManager::sRGBLut();
    lut->validate();
    Natron::Image::ReadAccess acc = image->getReadRights();
    const float* from = (const float*)acc.pixelAt( renderWindow.left(), renderWindow.bottom() );
    assert(from);
    int srcNComps = (int)image->getComponentsCount();
    int srcRowElements = srcNComps * bounds.width();
    
    for (int y = renderWindow.height() - 1; y >= 0; --y,
         from += (srcRowElements - srcNComps * renderWindow.width())) {
        
        QRgb* dstPixels = (QRgb*)output.scanLine(y);
        assert(dstPixels);
        
        unsigned error_r = 0x80;
        unsigned error_g = 0x80;
        unsigned error_b = 0x80;
        
        for (int x = 0; x < renderWindow.width(); ++x, from += srcNComps, ++dstPixels) {
            float r,g,b,a;
            switch (srcNComps) {
                case 1:
                    r = g = b = *from;
                    a = 1;
                    break;
                case 2:
                    r = *from;
                    g = *(from + 1);
                    b = 0;
                    a = 1;
                    break;
                case 3:
                    r = *from;
                    g = *(from + 1);
                    b = *(from + 2);
                    a = 1;
                    break;
                case 4:
                    r = *from;
                    g = *(from + 1);
                    b = *(from + 2);
                    a = *(from + 3);
                    break;
                default:
                    assert(false);
                    return;
            }
            error_r = (error_r & 0xff) + lut->toColorSpaceUint8xxFromLinearFloatFast(r);
            error_g = (error_g & 0xff) + lut->toColorSpaceUint8xxFromLinearFloatFast(g);
            error_b = (error_b & 0xff) + lut->toColorSpaceUint8xxFromLinearFloatFast(b);
            assert(error_r < 0x10000 && error_g < 0x10000 && error_b < 0x10000);
            *dstPixels = qRgba(U8(error_r >> 8),
                              U8(error_g >> 8),
                              U8(error_b >> 8),
                              U8(a * 255));
        }
    }
    
    U64 hashKey = image->getHashKey();
    QString hashKeyStr = QString::number(hashKey);
    QString realFileName = filename.isEmpty() ? QString(hashKeyStr + ".png") : filename;
    qDebug() << "Writing image: " << realFileName;
    renderWindow.debug();
    output.save(realFileName);
}

void
Gui::updateLastSequenceOpenedPath(const QString & path)
{
    _imp->_lastLoadSequenceOpenedDir = path;
}

void
Gui::updateLastSequenceSavedPath(const QString & path)
{
    _imp->_lastSaveSequenceOpenedDir = path;
}

void
Gui::updateLastSavedProjectPath(const QString & project)
{
    _imp->_lastSaveProjectOpenedDir = project;
}

void
Gui::updateLastOpenedProjectPath(const QString & project)
{
    _imp->_lastLoadProjectOpenedDir = project;
}

void
Gui::onWriterRenderStarted(const QString & sequenceName,
                           int firstFrame,
                           int lastFrame,
                           Natron::OutputEffectInstance* writer)
{
    assert( QThread::currentThread() == qApp->thread() );

    RenderingProgressDialog *dialog = new RenderingProgressDialog(this, sequenceName, firstFrame, lastFrame,
                                                                  boost::shared_ptr<ProcessHandler>(), this);
    RenderEngine* engine = writer->getRenderEngine();
    QObject::connect( dialog, SIGNAL( canceled() ), engine, SLOT( abortRendering_Blocking() ) );
    QObject::connect( engine, SIGNAL( frameRendered(int) ), dialog, SLOT( onFrameRendered(int) ) );
    QObject::connect( engine, SIGNAL( frameRenderedWithTimer(int,double,double) ), dialog, SLOT( onFrameRenderedWithTimer(int,double,double) ) );
    QObject::connect( engine, SIGNAL( renderFinished(int) ), dialog, SLOT( onVideoEngineStopped(int) ) );
    QObject::connect(dialog,SIGNAL(accepted()),this,SLOT(onRenderProgressDialogFinished()));
    QObject::connect(dialog,SIGNAL(rejected()),this,SLOT(onRenderProgressDialogFinished()));
    dialog->show();
}

void
Gui::setGlewVersion(const QString & version)
{
    _imp->_glewVersion = version;
    _imp->_aboutWindow->updateLibrariesVersions();
}

void
Gui::setOpenGLVersion(const QString & version)
{
    _imp->_openGLVersion = version;
    _imp->_aboutWindow->updateLibrariesVersions();
}

QString
Gui::getGlewVersion() const
{
    return _imp->_glewVersion;
}

QString
Gui::getOpenGLVersion() const
{
    return _imp->_openGLVersion;
}

QString
Gui::getBoostVersion() const
{
    return QString(BOOST_LIB_VERSION);
}

QString
Gui::getQtVersion() const
{
    return QString(QT_VERSION_STR) + " / " + qVersion();
}

QString
Gui::getCairoVersion() const
{
    return QString(CAIRO_VERSION_STRING) + " / " + QString( cairo_version_string() );
}

void
Gui::onNodeNameChanged(const QString & /*name*/)
{
    Natron::Node* node = qobject_cast<Natron::Node*>( sender() );

    if (!node) {
        return;
    }
    ViewerInstance* isViewer = dynamic_cast<ViewerInstance*>( node->getLiveInstance() );
    if (isViewer) {
        Q_EMIT viewersChanged();
    }
}

void
Gui::renderAllWriters()
{
    _imp->_appInstance->startWritersRendering( std::list<AppInstance::RenderRequest>() );
}

void
Gui::renderSelectedNode()
{
    NodeGraph* graph = getLastSelectedGraph();
    if (!graph) {
        return;
    }
    
    const std::list<boost::shared_ptr<NodeGui> > & selectedNodes = graph->getSelectedNodes();

    if ( selectedNodes.empty() ) {
        Natron::warningDialog( tr("Render").toStdString(), tr("You must select a node to render first!").toStdString() );
    } else {
        std::list<AppInstance::RenderWork> workList;

        for (std::list<boost::shared_ptr<NodeGui> >::const_iterator it = selectedNodes.begin();
             it != selectedNodes.end(); ++it) {
            Natron::EffectInstance* effect = (*it)->getNode()->getLiveInstance();
            assert(effect);
            if (effect->isWriter()) {
                if (!effect->areKnobsFrozen()) {
                    //if ((*it)->getNode()->is)
                    ///if the node is a writer, just use it to render!
                    AppInstance::RenderWork w;
                    w.writer = dynamic_cast<Natron::OutputEffectInstance*>(effect);
                    assert(w.writer);
                    w.firstFrame = INT_MIN;
                    w.lastFrame = INT_MAX;
                    workList.push_back(w);
                }
            } else {
                if (selectedNodes.size() == 1) {
                    ///create a node and connect it to the node and use it to render
                    boost::shared_ptr<Natron::Node> writer = createWriter();
                    if (writer) {
                        AppInstance::RenderWork w;
                        w.writer = dynamic_cast<Natron::OutputEffectInstance*>( writer->getLiveInstance() );
                        assert(w.writer);
                        w.firstFrame = INT_MIN;
                        w.lastFrame = INT_MAX;
                        workList.push_back(w);
                    }
                }
            }
        }
        _imp->_appInstance->startWritersRendering(workList);
    }
}

void
Gui::setUndoRedoStackLimit(int limit)
{
    _imp->_nodeGraphArea->setUndoRedoStackLimit(limit);
}

void
Gui::showOfxLog()
{
    QString log = appPTR->getOfxLog_mt_safe();
    LogWindow lw(log, this);

    lw.setWindowTitle( tr("Error Log") );
    ignore_result( lw.exec() );
}

void
Gui::createNewTrackerInterface(NodeGui* n)
{
    QMutexLocker l(&_imp->_viewerTabsMutex);

    for (std::list<ViewerTab*>::iterator it = _imp->_viewerTabs.begin(); it != _imp->_viewerTabs.end(); ++it) {
        (*it)->createTrackerInterface(n);
    }
}

void
Gui::removeTrackerInterface(NodeGui* n,
                            bool permanently)
{
    QMutexLocker l(&_imp->_viewerTabsMutex);

    for (std::list<ViewerTab*>::iterator it = _imp->_viewerTabs.begin(); it != _imp->_viewerTabs.end(); ++it) {
        (*it)->removeTrackerInterface(n, permanently, false);
    }
}

void
Gui::onRotoSelectedToolChanged(int tool)
{
    RotoGui* roto = qobject_cast<RotoGui*>( sender() );

    if (!roto) {
        return;
    }
    QMutexLocker l(&_imp->_viewerTabsMutex);
    for (std::list<ViewerTab*>::iterator it = _imp->_viewerTabs.begin(); it != _imp->_viewerTabs.end(); ++it) {
        (*it)->updateRotoSelectedTool(tool, roto);
    }
}

void
Gui::createNewRotoInterface(NodeGui* n)
{
    QMutexLocker l(&_imp->_viewerTabsMutex);

    for (std::list<ViewerTab*>::iterator it = _imp->_viewerTabs.begin(); it != _imp->_viewerTabs.end(); ++it) {
        (*it)->createRotoInterface(n);
    }
}

void
Gui::removeRotoInterface(NodeGui* n,
                         bool permanently)
{
    QMutexLocker l(&_imp->_viewerTabsMutex);

    for (std::list<ViewerTab*>::iterator it = _imp->_viewerTabs.begin(); it != _imp->_viewerTabs.end(); ++it) {
        (*it)->removeRotoInterface(n, permanently, false);
    }
}

void
Gui::setRotoInterface(NodeGui* n)
{
    QMutexLocker l(&_imp->_viewerTabsMutex);

    for (std::list<ViewerTab*>::iterator it = _imp->_viewerTabs.begin(); it != _imp->_viewerTabs.end(); ++it) {
        (*it)->setRotoInterface(n);
    }
}

void
Gui::onViewerRotoEvaluated(ViewerTab* viewer)
{
    QMutexLocker l(&_imp->_viewerTabsMutex);

    for (std::list<ViewerTab*>::iterator it = _imp->_viewerTabs.begin(); it != _imp->_viewerTabs.end(); ++it) {
        if (*it != viewer) {
            (*it)->getViewer()->redraw();
        }
    }
}

void
Gui::startProgress(KnobHolder* effect,
                   const std::string & message,
                   bool canCancel)
{
    if (!effect) {
        return;
    }
    if ( QThread::currentThread() != qApp->thread() ) {
        qDebug() << "Progress bars called from a thread different than the main-thread is not supported at the moment.";

        return;
    }

    QProgressDialog* dialog = new QProgressDialog(message.c_str(), tr("Cancel"), 0, 100, this);
    if (!canCancel) {
        dialog->setCancelButton(0);
    }
    dialog->setModal(false);
    dialog->setRange(0, 100);
    dialog->setMinimumWidth(250);
    NamedKnobHolder* isNamed = dynamic_cast<NamedKnobHolder*>(effect);
    if (isNamed) {
        dialog->setWindowTitle( isNamed->getScriptName_mt_safe().c_str() );
    }
    std::map<KnobHolder*, QProgressDialog*>::iterator found = _imp->_progressBars.find(effect);

    ///If a second dialog was asked for whilst another is still active, the first dialog will not be
    ///able to be canceled.
    if ( found != _imp->_progressBars.end() ) {
        _imp->_progressBars.erase(found);
    }

    _imp->_progressBars.insert( std::make_pair(effect, dialog) );
    dialog->show();
    //dialog->exec();
}

void
Gui::endProgress(KnobHolder* effect)
{
    if ( QThread::currentThread() != qApp->thread() ) {
        qDebug() << "Progress bars called from a thread different than the main-thread is not supported at the moment.";

        return;
    }

    std::map<KnobHolder*, QProgressDialog*>::iterator found = _imp->_progressBars.find(effect);
    if ( found == _imp->_progressBars.end() ) {
        return;
    }


    found->second->close();
    _imp->_progressBars.erase(found);
}

bool
Gui::progressUpdate(KnobHolder* effect,
                    double t)
{
    if ( QThread::currentThread() != qApp->thread() ) {
        qDebug() << "Progress bars called from a thread different than the main-thread is not supported at the moment.";

        return true;
    }

    std::map<KnobHolder*, QProgressDialog*>::iterator found = _imp->_progressBars.find(effect);
    if ( found == _imp->_progressBars.end() ) {
        NamedKnobHolder* isNamed = dynamic_cast<NamedKnobHolder*>(effect);
        if (isNamed) {
            qDebug() << isNamed->getScriptName_mt_safe().c_str() <<  " called progressUpdate but didn't called startProgress first.";
        }
    } else {
        if ( found->second->wasCanceled() ) {
            return false;
        }
        found->second->setValue(t * 100);
    }
    //QCoreApplication::processEvents();

    return true;
}

void
Gui::addVisibleDockablePanel(DockablePanel* panel)
{
    
    std::list<DockablePanel*>::iterator it = std::find(_imp->openedPanels.begin(), _imp->openedPanels.end(), panel);
    putSettingsPanelFirst(panel);
    if ( it == _imp->openedPanels.end() ) {
        assert(panel);
        int maxPanels = appPTR->getCurrentSettings()->getMaxPanelsOpened();
        if ( ( (int)_imp->openedPanels.size() == maxPanels ) && (maxPanels != 0) ) {
            std::list<DockablePanel*>::iterator it = _imp->openedPanels.begin();
            (*it)->closePanel();
        }
        _imp->openedPanels.push_back(panel);
    }
}

void
Gui::removeVisibleDockablePanel(DockablePanel* panel)
{
    std::list<DockablePanel*>::iterator it = std::find(_imp->openedPanels.begin(), _imp->openedPanels.end(), panel);

    if ( it != _imp->openedPanels.end() ) {
        _imp->openedPanels.erase(it);
    }
}

const std::list<DockablePanel*>&
Gui::getVisiblePanels() const
{
    return _imp->openedPanels;
}

void
Gui::onMaxVisibleDockablePanelChanged(int maxPanels)
{
    assert(maxPanels >= 0);
    if (maxPanels == 0) {
        return;
    }
    while ( (int)_imp->openedPanels.size() > maxPanels ) {
        std::list<DockablePanel*>::iterator it = _imp->openedPanels.begin();
        (*it)->closePanel();
    }
    _imp->_maxPanelsOpenedSpinBox->setValue(maxPanels);
}

void
Gui::onMaxPanelsSpinBoxValueChanged(double val)
{
    appPTR->getCurrentSettings()->setMaxPanelsOpened( (int)val );
}

void
Gui::clearAllVisiblePanels()
{
    while ( !_imp->openedPanels.empty() ) {
        std::list<DockablePanel*>::iterator it = _imp->openedPanels.begin();
        if ( !(*it)->isFloating() ) {
            (*it)->setClosed(true);
        }

        bool foundNonFloating = false;
        for (std::list<DockablePanel*>::iterator it2 = _imp->openedPanels.begin(); it2 != _imp->openedPanels.end(); ++it2) {
            if ( !(*it2)->isFloating() ) {
                foundNonFloating = true;
                break;
            }
        }
        ///only floating windows left
        if (!foundNonFloating) {
            break;
        }
    }
    getApp()->redrawAllViewers();
}

void
Gui::minimizeMaximizeAllPanels(bool clicked)
{
    for (std::list<DockablePanel*>::iterator it = _imp->openedPanels.begin(); it != _imp->openedPanels.end(); ++it) {
        if (clicked) {
            if ( !(*it)->isMinimized() ) {
                (*it)->minimizeOrMaximize(true);
            }
        } else {
            if ( (*it)->isMinimized() ) {
                (*it)->minimizeOrMaximize(false);
            }
        }
    }
    getApp()->redrawAllViewers();
}

void
Gui::connectViewersToViewerCache()
{
    QMutexLocker l(&_imp->_viewerTabsMutex);

    for (std::list<ViewerTab*>::iterator it = _imp->_viewerTabs.begin(); it != _imp->_viewerTabs.end(); ++it) {
        (*it)->connectToViewerCache();
    }
}

void
Gui::disconnectViewersFromViewerCache()
{
    QMutexLocker l(&_imp->_viewerTabsMutex);

    for (std::list<ViewerTab*>::iterator it = _imp->_viewerTabs.begin(); it != _imp->_viewerTabs.end(); ++it) {
        (*it)->disconnectFromViewerCache();
    }
}

void
Gui::moveEvent(QMoveEvent* e)
{
    QMainWindow::moveEvent(e);
    QPoint p = pos();

    setMtSafePosition( p.x(), p.y() );
}


#if 0
bool
Gui::event(QEvent* e)
{
    switch (e->type()) {
        case QEvent::TabletEnterProximity:
        case QEvent::TabletLeaveProximity:
        case QEvent::TabletMove:
        case QEvent::TabletPress:
        case QEvent::TabletRelease:
        {
            QTabletEvent *tEvent = dynamic_cast<QTabletEvent *>(e);
            const std::list<ViewerTab*>& viewers = getViewersList();
            for (std::list<ViewerTab*>::const_iterator it = viewers.begin(); it!=viewers.end(); ++it) {
                QPoint widgetPos = (*it)->mapToGlobal((*it)->mapFromParent((*it)->pos()));
                QRect r(widgetPos.x(),widgetPos.y(),(*it)->width(),(*it)->height());
                if (r.contains(tEvent->globalPos())) {
                    QTabletEvent te(tEvent->type()
                                    , mapFromGlobal(tEvent->pos())
                                    , tEvent->globalPos()
                                    , tEvent->hiResGlobalPos()
                                    , tEvent->device()
                                    , tEvent->pointerType()
                                    , tEvent->pressure()
                                    , tEvent->xTilt()
                                    , tEvent->yTilt()
                                    , tEvent->tangentialPressure()
                                    , tEvent->rotation()
                                    , tEvent->z()
                                    , tEvent->modifiers()
                                    , tEvent->uniqueId());
                    qApp->sendEvent((*it)->getViewer(), &te);
                    return true;
                }
            }
            break;
        }
        default:
            break;
    }
    return QMainWindow::event(e);
}
#endif
void
Gui::resizeEvent(QResizeEvent* e)
{
    QMainWindow::resizeEvent(e);

    setMtSafeWindowSize( width(), height() );
}

void
Gui::keyPressEvent(QKeyEvent* e)
{
    QWidget* w = qApp->widgetAt( QCursor::pos() );

    if ( w && ( w->objectName() == QString("SettingsPanel") ) && (e->key() == Qt::Key_Escape) ) {
        RightClickableWidget* panel = dynamic_cast<RightClickableWidget*>(w);
        assert(panel);
        panel->getPanel()->closePanel();
    }

    Qt::Key key = (Qt::Key)e->key();
    Qt::KeyboardModifiers modifiers = e->modifiers();

    if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerPrevious, modifiers, key) ) {
        if ( getNodeGraph()->getLastSelectedViewer() ) {
            getNodeGraph()->getLastSelectedViewer()->previousFrame();
        }
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerNext, modifiers, key) ) {
        if ( getNodeGraph()->getLastSelectedViewer() ) {
            getNodeGraph()->getLastSelectedViewer()->nextFrame();
        }
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerFirst, modifiers, key) ) {
        if ( getNodeGraph()->getLastSelectedViewer() ) {
            getNodeGraph()->getLastSelectedViewer()->firstFrame();
        }
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerLast, modifiers, key) ) {
        if ( getNodeGraph()->getLastSelectedViewer() ) {
            getNodeGraph()->getLastSelectedViewer()->lastFrame();
        }
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerPrevIncr, modifiers, key) ) {
        if ( getNodeGraph()->getLastSelectedViewer() ) {
            getNodeGraph()->getLastSelectedViewer()->previousIncrement();
        }
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerNextIncr, modifiers, key) ) {
        if ( getNodeGraph()->getLastSelectedViewer() ) {
            getNodeGraph()->getLastSelectedViewer()->nextIncrement();
        }
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerPrevKF, modifiers, key) ) {
        getApp()->getTimeLine()->goToPreviousKeyframe();
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerNextKF, modifiers, key) ) {
        getApp()->getTimeLine()->goToNextKeyframe();
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphDisableNodes, modifiers, key) ) {
        _imp->_nodeGraphArea->toggleSelectedNodesEnabled();
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphFindNode, modifiers, key) ) {
        _imp->_nodeGraphArea->popFindDialog();
    } else {
        QMainWindow::keyPressEvent(e);
    }
}

TabWidget*
Gui::getAnchor() const
{
    QMutexLocker l(&_imp->_panesMutex);

    for (std::list<TabWidget*>::const_iterator it = _imp->_panes.begin(); it != _imp->_panes.end(); ++it) {
        if ( (*it)->isAnchor() ) {
            return *it;
        }
    }

    return NULL;
}

bool
Gui::isGUIFrozen() const
{
    QMutexLocker k(&_imp->_isGUIFrozenMutex);

    return _imp->_isGUIFrozen;
}

void
Gui::onFreezeUIButtonClicked(bool clicked)
{
    {
        QMutexLocker k(&_imp->_isGUIFrozenMutex);
        if (_imp->_isGUIFrozen == clicked) {
            return;
        }
        _imp->_isGUIFrozen = clicked;
    }
    {
        QMutexLocker k(&_imp->_viewerTabsMutex);
        for (std::list<ViewerTab*>::iterator it = _imp->_viewerTabs.begin(); it != _imp->_viewerTabs.end(); ++it) {
            (*it)->setTurboButtonDown(clicked);
            if (!clicked) {
                (*it)->getViewer()->redraw(); //< overlays were disabled while frozen, redraw to make them re-appear
            }
        }
    }
    _imp->_nodeGraphArea->onGuiFrozenChanged(clicked);
    for (std::list<NodeGraph*>::iterator it = _imp->_groups.begin(); it != _imp->_groups.end(); ++it) {
        (*it)->onGuiFrozenChanged(clicked);
    }
}

bool
Gui::hasShortcutEditorAlreadyBeenBuilt() const
{
    return _imp->shortcutEditor != NULL;
}

void
Gui::addShortcut(BoundAction* action)
{
    assert(_imp->shortcutEditor);
    _imp->shortcutEditor->addShortcut(action);
}

void
Gui::getNodesEntitledForOverlays(std::list<boost::shared_ptr<Natron::Node> > & nodes) const
{
    int layoutItemsCount = _imp->_layoutPropertiesBin->count();

    for (int i = 0; i < layoutItemsCount; ++i) {
        QLayoutItem* item = _imp->_layoutPropertiesBin->itemAt(i);
        NodeSettingsPanel* panel = dynamic_cast<NodeSettingsPanel*>( item->widget() );
        if (panel) {
            boost::shared_ptr<NodeGui> node = panel->getNode();
            boost::shared_ptr<Natron::Node> internalNode = node->getNode();
            if (node && internalNode) {
                boost::shared_ptr<MultiInstancePanel> multiInstance = node->getMultiInstancePanel();
                if (multiInstance) {
//                    const std::list< std::pair<boost::weak_ptr<Natron::Node>,bool > >& instances = multiInstance->getInstances();
//                    for (std::list< std::pair<boost::weak_ptr<Natron::Node>,bool > >::const_iterator it = instances.begin(); it != instances.end(); ++it) {
//                        NodePtr instance = it->first.lock();
//                        if (node->isSettingsPanelVisible() &&
//                            !node->isSettingsPanelMinimized() &&
//                            instance->isActivated() &&
//                            instance->hasOverlay() &&
//                            it->second &&
//                            !instance->isNodeDisabled()) {
//                            nodes.push_back(instance);
//                        }
//                    }
                    if ( internalNode->hasOverlay() &&
                         !internalNode->isNodeDisabled() &&
                         node->isSettingsPanelVisible() &&
                         !node->isSettingsPanelMinimized() ) {
                        nodes.push_back( node->getNode() );
                    }
                } else {
                    if ( ( internalNode->hasOverlay() || internalNode->getRotoContext() ) &&
                         !internalNode->isNodeDisabled() &&
                        !internalNode->getParentMultiInstance() && 
                         internalNode->isActivated() &&
                         node->isSettingsPanelVisible() &&
                         !node->isSettingsPanelMinimized() ) {
                        nodes.push_back( node->getNode() );
                    }
                }
            }
        }
    }
}

void
Gui::redrawAllViewers()
{
    QMutexLocker k(&_imp->_viewerTabsMutex);

    for (std::list<ViewerTab*>::const_iterator it = _imp->_viewerTabs.begin(); it != _imp->_viewerTabs.end(); ++it) {
        if ( (*it)->isVisible() ) {
            (*it)->getViewer()->redraw();
        }
    }
}

void
Gui::renderAllViewers()
{
    assert(QThread::currentThread() == qApp->thread());
    for (std::list<ViewerTab*>::const_iterator it = _imp->_viewerTabs.begin(); it != _imp->_viewerTabs.end(); ++it) {
        if ( (*it)->isVisible() ) {
            (*it)->getInternalNode()->renderCurrentFrame(true);
        }
    }
}

void
Gui::toggleAutoHideGraphInputs()
{
    _imp->_nodeGraphArea->toggleAutoHideInputs(false);
}

void
Gui::centerAllNodeGraphsWithTimer()
{
    QTimer::singleShot( 25, _imp->_nodeGraphArea, SLOT( centerOnAllNodes() ) );

    for (std::list<NodeGraph*>::iterator it = _imp->_groups.begin(); it != _imp->_groups.end(); ++it) {
        QTimer::singleShot( 25, *it, SLOT( centerOnAllNodes() ) );
    }
}

void
Gui::setLastEnteredTabWidget(TabWidget* tab)
{
    _imp->_lastEnteredTabWidget = tab;
}

TabWidget*
Gui::getLastEnteredTabWidget() const
{
    return _imp->_lastEnteredTabWidget;
}

void
Gui::onPrevTabTriggered()
{
    TabWidget* t = getLastEnteredTabWidget();
    
    if (t) {
        t->moveToPreviousTab();
    }

}

void
Gui::onNextTabTriggered()
{
    TabWidget* t = getLastEnteredTabWidget();

    if (t) {
        t->moveToNextTab();
    }
}

void
Gui::onCloseTabTriggered()
{
    TabWidget* t = getLastEnteredTabWidget();

    if (t) {
        t->closeCurrentWidget();
    }
}

void
Gui::appendToScriptEditor(const std::string & str)
{
    _imp->_scriptEditor->appendToScriptEditor( str.c_str() );
}

void
Gui::printAutoDeclaredVariable(const std::string & str)
{
    _imp->_scriptEditor->printAutoDeclaredVariable( str.c_str() );
}

void
Gui::exportGroupAsPythonScript(NodeCollection* collection)
{
    assert(collection);
    NodeList nodes = collection->getNodes();
    bool hasOutput = false;
    for (NodeList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        if ( (*it)->isActivated() && dynamic_cast<GroupOutput*>( (*it)->getLiveInstance() ) ) {
            hasOutput = true;
            break;
        }
    }

    if (!hasOutput) {
        Natron::errorDialog( tr("Export").toStdString(), tr("To export as group, at least one Ouptut node must exist.").toStdString() );

        return;
    }
    ExportGroupTemplateDialog dialog(collection, this, this);
    ignore_result( dialog.exec() );
}

void
Gui::exportProjectAsGroup()
{
    exportGroupAsPythonScript( getApp()->getProject().get() );
}


void
Gui::onUserCommandTriggered()
{
    QAction* action = qobject_cast<QAction*>( sender() );

    if (!action) {
        return;
    }
    ActionWithShortcut* aws = dynamic_cast<ActionWithShortcut*>(action);
    if (!aws) {
        return;
    }
    std::map<ActionWithShortcut*, std::string>::iterator found = _imp->pythonCommands.find(aws);
    if ( found != _imp->pythonCommands.end() ) {
        std::string err;
        std::string output;
        if ( !Natron::interpretPythonScript(found->second, &err, &output) ) {
            getApp()->appendToScriptEditor(err);
        } else {
            getApp()->appendToScriptEditor(output);
        }
    }
}

void
Gui::addMenuEntry(const QString & menuGrouping,
                  const std::string & pythonFunction,
                  Qt::Key key,
                  const Qt::KeyboardModifiers & modifiers)
{
    QStringList grouping = menuGrouping.split('/');

    if ( grouping.isEmpty() ) {
        getApp()->appendToScriptEditor( tr("Failed to add menu entry for ").toStdString() +
                                       menuGrouping.toStdString() +
                                       tr(": incorrect menu grouping").toStdString() );

        return;
    }

    std::string appID = getApp()->getAppIDString();
    std::string script = "app = " + appID + "\n" + pythonFunction + "()\n";
    QAction* action = _imp->findActionRecursive(0, _imp->menubar, grouping);
    ActionWithShortcut* aws = dynamic_cast<ActionWithShortcut*>(action);
    if (aws) {
        aws->setShortcut( makeKeySequence(modifiers, key) );
        std::map<ActionWithShortcut*, std::string>::iterator found = _imp->pythonCommands.find(aws);
        if ( found != _imp->pythonCommands.end() ) {
            found->second = pythonFunction;
        } else {
            _imp->pythonCommands.insert( std::make_pair(aws, script) );
        }
    }
}

void
Gui::setDopeSheetTreeWidth(int width)
{
    _imp->_dopeSheetEditor->setTreeWidgetWidth(width);
}

void
Gui::setCurveEditorTreeWidth(int width)
{
    _imp->_curveEditor->setTreeWidgetWidth(width);
}

void Gui::setTripleSyncEnabled(bool enabled)
{
    if (_imp->_isTripleSyncEnabled != enabled) {
        _imp->_isTripleSyncEnabled = enabled;
    }
}

bool Gui::isTripleSyncEnabled() const
{
    return _imp->_isTripleSyncEnabled;
}

void Gui::centerOpenedViewersOn(SequenceTime left, SequenceTime right)
{
    const std::list<ViewerTab *> &viewers = getViewersList();

    for (std::list<ViewerTab *>::const_iterator it = viewers.begin(); it != viewers.end(); ++it) {
        ViewerTab *v = (*it);

        v->centerOn_tripleSync(left, right);
    }
}
