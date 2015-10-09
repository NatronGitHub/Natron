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

#include "Global/Macros.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QThread>
#include <QtCore/QTimer>

#include <QHBoxLayout>
#include <QGraphicsScene>
#include <QUndoGroup>

#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/Project.h"
#include "Engine/TimeLine.h"

#include "Gui/AboutWindow.h"
#include "Gui/AutoHideToolBar.h"
#include "Gui/CurveEditor.h"
#include "Gui/CurveWidget.h"
#include "Gui/FloatingWidget.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiPrivate.h"
#include "Gui/Histogram.h"
#include "Gui/NodeGraph.h"
#include "Gui/ProjectGui.h"
#include "Gui/ShortCutEditor.h"
#include "Gui/Splitter.h"
#include "Gui/TabWidget.h"
#include "Gui/ViewerTab.h"


using namespace Natron;


void
Gui::setupUi()
{
    setWindowTitle( QCoreApplication::applicationName() );
    setMouseTracking(true);
    installEventFilter(this);
    assert( !isFullScreen() );

    loadStyleSheet();

    ///Restores position, size of the main window as well as whether it was fullscreen or not.
    _imp->restoreGuiGeometry();


    _imp->_undoStacksGroup = new QUndoGroup;
    QObject::connect( _imp->_undoStacksGroup, SIGNAL( activeStackChanged(QUndoStack*) ), this, SLOT( onCurrentUndoStackChanged(QUndoStack*) ) );

    createMenuActions();

    /*CENTRAL AREA*/
    //======================
    _imp->_centralWidget = new QWidget(this);
    setCentralWidget(_imp->_centralWidget);
    _imp->_mainLayout = new QHBoxLayout(_imp->_centralWidget);
    _imp->_mainLayout->setContentsMargins(0, 0, 0, 0);
    _imp->_centralWidget->setLayout(_imp->_mainLayout);

    _imp->_leftRightSplitter = new Splitter(_imp->_centralWidget);
    _imp->_leftRightSplitter->setChildrenCollapsible(false);
    _imp->_leftRightSplitter->setObjectName(kMainSplitterObjectName);
    _imp->_splitters.push_back(_imp->_leftRightSplitter);
    _imp->_leftRightSplitter->setOrientation(Qt::Horizontal);
    _imp->_leftRightSplitter->setContentsMargins(0, 0, 0, 0);


    _imp->_toolBox = new AutoHideToolBar(this, _imp->_leftRightSplitter);
    _imp->_toolBox->setToolButtonStyle(Qt::ToolButtonIconOnly);
    _imp->_toolBox->setOrientation(Qt::Vertical);
    _imp->_toolBox->setMaximumWidth(NATRON_TOOL_BUTTON_SIZE);

    if (_imp->leftToolBarDisplayedOnHoverOnly) {
        _imp->refreshLeftToolBarVisibility( mapFromGlobal( QCursor::pos() ) );
    }

    _imp->_leftRightSplitter->addWidget(_imp->_toolBox);

    _imp->_mainLayout->addWidget(_imp->_leftRightSplitter);

    _imp->createNodeGraphGui();
    _imp->createCurveEditorGui();
    _imp->createDopeSheetGui();
    _imp->createScriptEditorGui();
    ///Must be absolutely called once _nodeGraphArea has been initialized.
    _imp->createPropertiesBinGui();

    createDefaultLayoutInternal(false);

    _imp->_projectGui = new ProjectGui(this);
    _imp->_projectGui->create(_imp->_appInstance->getProject(),
                              _imp->_layoutPropertiesBin,
                              this);

    initProjectGuiKnobs();


    setVisibleProjectSettingsPanel();

    _imp->_aboutWindow = new AboutWindow(this, this);
    _imp->_aboutWindow->hide();

    _imp->shortcutEditor = new ShortCutEditor(this);
    _imp->shortcutEditor->hide();


    //the same action also clears the ofx plugins caches, they are not the same cache but are used to the same end
    
    boost::shared_ptr<Project> project = _imp->_appInstance->getProject();
    QObject::connect( project.get(), SIGNAL( projectNameChanged(QString) ), this, SLOT( onProjectNameChanged(QString) ) );
    
    boost::shared_ptr<TimeLine> timeline = project->getTimeLine();
    QObject::connect( timeline.get(),SIGNAL( frameChanged(SequenceTime,int) ), this,SLOT( onTimeChanged(SequenceTime,int) ) );
    QObject::connect( timeline.get(),SIGNAL( frameAboutToChange()), this, SLOT(onTimelineTimeAboutToChange()));

    /*Searches recursively for all child objects of the given object,
       and connects matching signals from them to slots of object that follow the following form:

        void on_<object name>_<signal name>(<signal parameters>);

       Let's assume our object has a child object of type QPushButton with the object name button1.
       The slot to catch the button's clicked() signal would be:

       void on_button1_clicked();

       If object itself has a properly set object name, its own signals are also connected to its respective slots.
     */
    QMetaObject::connectSlotsByName(this);
} // setupUi


void
Gui::onPropertiesScrolled()
{
#ifdef __NATRON_WIN32__
    //On Windows Qt 4.8.6 has a bug where the viewport of the scrollarea gets scrolled outside the bounding rect of the QScrollArea and overlaps all widgets inheriting QGLWidget.
    //The only thing I could think of was to repaint all GL widgets manually...

    {
        QMutexLocker k(&_imp->_viewerTabsMutex);
        for (std::list<ViewerTab*>::iterator it = _imp->_viewerTabs.begin(); it != _imp->_viewerTabs.end(); ++it) {
            (*it)->redrawGLWidgets();
        }
    }
    _imp->_curveEditor->getCurveWidget()->update();

    {
        QMutexLocker k (&_imp->_histogramsMutex);
        for (std::list<Histogram*>::iterator it = _imp->_histograms.begin(); it != _imp->_histograms.end(); ++it) {
            (*it)->update();
        }
    }
#endif
}

void
Gui::createGroupGui(const boost::shared_ptr<Natron::Node> & group,
                    bool requestedByLoad)
{
    boost::shared_ptr<NodeGroup> isGrp = boost::dynamic_pointer_cast<NodeGroup>( group->getLiveInstance()->shared_from_this() );

    assert(isGrp);
    boost::shared_ptr<NodeCollection> collection = boost::dynamic_pointer_cast<NodeCollection>(isGrp);
    assert(collection);

    TabWidget* where = 0;
    if (_imp->_lastFocusedGraph) {
        TabWidget* isTab = dynamic_cast<TabWidget*>( _imp->_lastFocusedGraph->parentWidget() );
        if (isTab) {
            where = isTab;
        } else {
            QMutexLocker k(&_imp->_panesMutex);
            assert( !_imp->_panes.empty() );
            where = _imp->_panes.front();
        }
    }

    QGraphicsScene* scene = new QGraphicsScene(this);
    scene->setItemIndexMethod(QGraphicsScene::NoIndex);
    NodeGraph* nodeGraph = new NodeGraph(this, collection, scene, this);
    nodeGraph->setObjectName( group->getLabel().c_str() );
    _imp->_groups.push_back(nodeGraph);
    if ( where && !requestedByLoad && !getApp()->isCreatingPythonGroup() ) {
        where->appendTab(nodeGraph, nodeGraph);
        QTimer::singleShot( 25, nodeGraph, SLOT( centerOnAllNodes() ) );
    } else {
        nodeGraph->setVisible(false);
    }
}

void
Gui::addGroupGui(NodeGraph* tab,
                 TabWidget* where)
{
    assert(tab);
    assert(where);
    {
        std::list<NodeGraph*>::iterator it = std::find(_imp->_groups.begin(), _imp->_groups.end(), tab);
        if ( it == _imp->_groups.end() ) {
            _imp->_groups.push_back(tab);
        }
    }
    where->appendTab(tab, tab);
}

void
Gui::removeGroupGui(NodeGraph* tab,
                    bool deleteData)
{
    tab->hide();

    if (_imp->_lastFocusedGraph == tab) {
        _imp->_lastFocusedGraph = 0;
    }
    TabWidget* container = dynamic_cast<TabWidget*>( tab->parentWidget() );
    if (container) {
        container->removeTab(tab, true);
    }

    if (deleteData) {
        std::list<NodeGraph*>::iterator it = std::find(_imp->_groups.begin(), _imp->_groups.end(), tab);
        if ( it != _imp->_groups.end() ) {
            _imp->_groups.erase(it);
        }
        tab->deleteLater();
    }
}

void
Gui::setLastSelectedGraph(NodeGraph* graph)
{
    assert( QThread::currentThread() == qApp->thread() );
    _imp->_lastFocusedGraph = graph;
}

NodeGraph*
Gui::getLastSelectedGraph() const
{
    assert( QThread::currentThread() == qApp->thread() );

    return _imp->_lastFocusedGraph;
}

boost::shared_ptr<NodeCollection>
Gui::getLastSelectedNodeCollection() const
{
    NodeGraph* graph = 0;

    if (_imp->_lastFocusedGraph) {
        graph = _imp->_lastFocusedGraph;
    } else {
        graph = _imp->_nodeGraphArea;
    }
    boost::shared_ptr<NodeCollection> group = graph->getGroup();
    assert(group);

    return group;
}


void
Gui::wipeLayout()
{
    std::list<TabWidget*> panesCpy;
    {
        QMutexLocker l(&_imp->_panesMutex);
        panesCpy = _imp->_panes;
        _imp->_panes.clear();
    }
    
    std::list<FloatingWidget*> floatingWidgets = getFloatingWindows();
    
    for (std::list<FloatingWidget*>::const_iterator it = floatingWidgets.begin(); it!=floatingWidgets.end(); ++it) {
        (*it)->deleteLater();
    }
    {
        QMutexLocker k(&_imp->_floatingWindowMutex);
        _imp->_floatingWindows.clear();
    }
    

    for (std::list<TabWidget*>::iterator it = panesCpy.begin(); it != panesCpy.end(); ++it) {
        ///Conserve tabs by removing them from the tab widgets. This way they will not be deleted.
        while ( (*it)->count() > 0 ) {
            (*it)->removeTab(0, false);
        }
    //(*it)->setParent(NULL);
        (*it)->deleteLater();
    }

    std::list<Splitter*> splittersCpy;
    {
        QMutexLocker l(&_imp->_splittersMutex);
        splittersCpy = _imp->_splitters;
        _imp->_splitters.clear();
    }
    for (std::list<Splitter*>::iterator it = splittersCpy.begin(); it != splittersCpy.end(); ++it) {
        if (_imp->_leftRightSplitter != *it) {
            while ( (*it)->count() > 0 ) {
                (*it)->widget(0)->setParent(NULL);
            }
            //(*it)->setParent(NULL);
            (*it)->deleteLater();
        }
    }
    
  

    Splitter *newSplitter = new Splitter(_imp->_centralWidget);
    newSplitter->addWidget(_imp->_toolBox);
    newSplitter->setObjectName_mt_safe( _imp->_leftRightSplitter->objectName_mt_safe() );
    _imp->_mainLayout->removeWidget(_imp->_leftRightSplitter);
    unregisterSplitter(_imp->_leftRightSplitter);
    _imp->_leftRightSplitter->deleteLater();
    _imp->_leftRightSplitter = newSplitter;
    _imp->_leftRightSplitter->setChildrenCollapsible(false);
    _imp->_mainLayout->addWidget(newSplitter);

    {
        QMutexLocker l(&_imp->_splittersMutex);
        _imp->_splitters.push_back(newSplitter);
    }
}

