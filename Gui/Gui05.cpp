/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://natrongithub.github.io/>,
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
#include <stdexcept>

#include <QtCore/QCoreApplication>
#include <QtCore/QThread>
#include <QtCore/QTimer>

#include <QHBoxLayout>
#include <QMenuBar>
#include <QGraphicsScene>
#include <QUndoGroup>

#ifdef DEBUG
#include "Global/FloatingPointExceptions.h"
#endif
#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/Project.h"
#include "Engine/TimeLine.h"
#include "Engine/CreateNodeArgs.h"

#include "Gui/AboutWindow.h"
#include "Gui/AnimationModuleEditor.h"
#include "Gui/AnimationModuleView.h"
#include "Gui/AutoHideToolBar.h"
#include "Gui/DockablePanel.h"
#include "Gui/FloatingWidget.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiPrivate.h"
#include "Gui/Histogram.h"
#include "Gui/NodeGraph.h"
#include "Gui/ProgressPanel.h"
#include "Gui/ProjectGui.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/Splitter.h"
#include "Gui/TabWidget.h"
#include "Gui/ViewerTab.h"


NATRON_NAMESPACE_ENTER


void
Gui::setupUi()
{
    onProjectNameChanged(QString(), false);

    setMouseTracking(true);
    installEventFilter(this);
    assert( !isFullScreen() );

    //Gui::loadStyleSheet();

    ///Restores position, size of the main window as well as whether it was fullscreen or not.
    _imp->restoreGuiGeometry();


    _imp->_undoStacksGroup = new QUndoGroup;

    createMenuActions();

    /*CENTRAL AREA*/
    //======================
    _imp->_centralWidget = new QWidget(this);
    setCentralWidget(_imp->_centralWidget);
    _imp->_mainLayout = new QHBoxLayout(_imp->_centralWidget);
    _imp->_mainLayout->setContentsMargins(0, 0, 0, 0);
    _imp->_centralWidget->setLayout(_imp->_mainLayout);

    _imp->_leftRightSplitter = new Splitter(Qt::Horizontal, this, _imp->_centralWidget);
    _imp->_leftRightSplitter->setChildrenCollapsible(false);
    _imp->_leftRightSplitter->setObjectName( QString::fromUtf8(kMainSplitterObjectName) );
    getApp()->registerSplitter(_imp->_leftRightSplitter);
    _imp->_leftRightSplitter->setOrientation(Qt::Horizontal);
    _imp->_leftRightSplitter->setContentsMargins(0, 0, 0, 0);


    _imp->_toolBox = new AutoHideToolBar(this, _imp->_leftRightSplitter);
    _imp->_toolBox->setToolButtonStyle(Qt::ToolButtonIconOnly);
    _imp->_toolBox->setOrientation(Qt::Vertical);
    _imp->_toolBox->setMaximumWidth( TO_DPIX(NATRON_TOOL_BUTTON_SIZE) );

    if (_imp->leftToolBarDisplayedOnHoverOnly) {
        _imp->refreshLeftToolBarVisibility( mapFromGlobal( QCursor::pos() ) );
    }

    _imp->_leftRightSplitter->addWidget(_imp->_toolBox);

    _imp->_mainLayout->addWidget(_imp->_leftRightSplitter);

    _imp->createNodeGraphGui();
    _imp->createAnimationModuleGui();
    _imp->createScriptEditorGui();
    _imp->createProgressPanelGui();
    ///Must be absolutely called once _nodeGraphArea has been initialized.
    _imp->createPropertiesBinGui();

    ProjectPtr project = getApp()->getProject();

    _imp->_projectGui = new ProjectGui(this);
    _imp->_projectGui->create(project,
                              _imp->_layoutPropertiesBin,
                              this);

    _imp->_errorLog = new LogWindow(0);
    _imp->_errorLog->hide();

    createDefaultLayoutInternal(false);


    initProjectGuiKnobs();


    setVisibleProjectSettingsPanel();

    _imp->_aboutWindow = new AboutWindow(this, this);
    _imp->_aboutWindow->hide();


    //the same action also clears the ofx plugins caches, they are not the same cache but are used to the same end

    QObject::connect( project.get(), SIGNAL(projectNameChanged(QString,bool)), this, SLOT(onProjectNameChanged(QString,bool)) );
    TimeLinePtr timeline = project->getTimeLine();
    QObject::connect( timeline.get(), SIGNAL(frameChanged(SequenceTime,int)), this, SLOT(onTimelineTimeChanged(SequenceTime,int)) );
    QObject::connect( timeline.get(), SIGNAL(frameAboutToChange()), this, SLOT(onTimelineTimeAboutToChange()) );

    /*Searches recursively for all child objects of the given object,
       and connects matching signals from them to slots of object that follow the following form:

        void on_<object name>_<signal name>(<signal parameters>);

       Let's assume our object has a child object of type QPushButton with the object name button1.
       The slot to catch the button's clicked() signal would be:

       void on_button1_clicked();

       If object itself has a properly set object name, its own signals are also connected to its respective slots.
     */
    QMetaObject::connectSlotsByName(this);

    {
#ifdef DEBUG
        boost_adaptbx::floating_point::exception_trapping trap(0);
#endif
        appPTR->setOFXHostHandle( getApp()->getOfxHostOSHandle() );
    }
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
    _imp->_animationModule->getView()->redraw();

    {
        QMutexLocker k (&_imp->_histogramsMutex);
        for (std::list<Histogram*>::iterator it = _imp->_histograms.begin(); it != _imp->_histograms.end(); ++it) {
            (*it)->update();
        }
    }
#endif
}

void
Gui::updateAboutWindowLibrariesVersion()
{
    if (_imp->_aboutWindow) {
        _imp->_aboutWindow->updateLibrariesVersions();
    }
}

void
Gui::createGroupGui(const NodePtr & group,
                    const CreateNodeArgs& args)
{
    NodeGroupPtr isGrp = toNodeGroup( group->getEffectInstance()->shared_from_this() );

    assert(isGrp);
    NodeCollectionPtr collection = boost::dynamic_pointer_cast<NodeCollection>(isGrp);
    assert(collection);

    TabWidget* where = 0;
    if (_imp->_lastFocusedGraph) {
        TabWidget* isTab = dynamic_cast<TabWidget*>( _imp->_lastFocusedGraph->parentWidget() );
        if (isTab) {
            where = isTab;
        } else {
            std::list<TabWidgetI*> panes = getApp()->getTabWidgetsSerialization();
            assert( !panes.empty() );
            where = dynamic_cast<TabWidget*>(panes.front());
        }
    }

    QGraphicsScene* scene = new QGraphicsScene(this);
    scene->setItemIndexMethod(QGraphicsScene::NoIndex);

    
    std::string newName = isGrp->getNode()->getFullyQualifiedName();
    for (std::size_t i = 0; i < newName.size(); ++i) {
        if (newName[i] == '.') {
            newName[i] = '_';
        }
    }
    newName += "_NodeGraph";
    std::string label = tr(" Node Graph").toStdString();
    NodeGraph::makeFullyQualifiedLabel(group, &label);


    NodeGraph* nodeGraph = new NodeGraph(this, collection, newName, scene, this);
    nodeGraph->setLabel(label);
    nodeGraph->setObjectName( QString::fromUtf8( group->getLabel().c_str() ) );
    _imp->_groups.push_back(nodeGraph);
    
    SERIALIZATION_NAMESPACE::NodeSerializationPtr serialization = args.getPropertyUnsafe<SERIALIZATION_NAMESPACE::NodeSerializationPtr>(kCreateNodeArgsPropNodeSerialization);
    bool showSubGraph = args.getPropertyUnsafe<bool>(kCreateNodeArgsPropSubGraphOpened);
    if ( showSubGraph && where && !serialization ) {
        where->appendTab(nodeGraph, nodeGraph);
        QTimer::singleShot( 25, nodeGraph, SLOT(centerOnAllNodes()) );
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

        unregisterTab(tab);
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

void
Gui::setActiveViewer(ViewerTab* viewer)
{
    assert( QThread::currentThread() == qApp->thread() );
    _imp->_activeViewer = viewer;
}

ViewerTab*
Gui::getActiveViewer() const
{
    assert( QThread::currentThread() == qApp->thread() );

    return _imp->_activeViewer;
}

NodeCollectionPtr
Gui::getLastSelectedNodeCollection() const
{
    NodeGraph* graph = 0;

    if (_imp->_lastFocusedGraph) {
        graph = _imp->_lastFocusedGraph;
    } else {
        graph = _imp->_nodeGraphArea;
    }
    NodeCollectionPtr group = graph->getGroup();
    assert(group);

    return group;
}

void
Gui::wipeLayout()
{

    std::list<TabWidgetI*> panesCpy = getApp()->getTabWidgetsSerialization();
    getApp()->clearTabWidgets();
    std::list<SerializableWindow*> floatingWidgets = getApp()->getFloatingWindowsSerialization();

    FloatingWidget* projectFW = _imp->_projectGui->getPanel()->getFloatingWindow();
    for (std::list<SerializableWindow*>::const_iterator it = floatingWidgets.begin(); it != floatingWidgets.end(); ++it) {
        if (!projectFW || (*it) != projectFW) {
            FloatingWidget* isFloating = dynamic_cast<FloatingWidget*>(*it);
            if (isFloating) {
                isFloating->deleteLater();
            }
        }
    }
    getApp()->clearFloatingWindows();

    // Re-add the project window
    if (projectFW) {
        getApp()->registerFloatingWindow(projectFW);
    }



    for (std::list<TabWidgetI*>::iterator it = panesCpy.begin(); it != panesCpy.end(); ++it) {
        TabWidget* pane = dynamic_cast<TabWidget*>(*it);
        if (!pane) {
            continue;
        }
        ///Conserve tabs by removing them from the tab widgets. This way they will not be deleted.
        while ( pane->count() > 0 ) {
            pane->removeTab(0, false);
        }
        //(*it)->setParent(NULL);
        pane->deleteLater();
    }

    std::list<SplitterI*> splittersCpy = getApp()->getSplittersSerialization();
    getApp()->clearSplitters();
    for (std::list<SplitterI*>::iterator it = splittersCpy.begin(); it != splittersCpy.end(); ++it) {
        if (_imp->_leftRightSplitter != *it) {
            Splitter* isSplitter = dynamic_cast<Splitter*>(*it);
            if (!isSplitter) {
                continue;
            }
            while ( isSplitter->count() > 0 ) {
                isSplitter->widget(0)->setParent(NULL);
            }
            //(*it)->setParent(NULL);
            isSplitter->deleteLater();
        }
    }


    Splitter *newSplitter = new Splitter(Qt::Horizontal,this, _imp->_centralWidget);
    newSplitter->addWidget(_imp->_toolBox);
    newSplitter->setObjectName_mt_safe( _imp->_leftRightSplitter->objectName_mt_safe() );
    _imp->_mainLayout->removeWidget(_imp->_leftRightSplitter);
    getApp()->unregisterSplitter(_imp->_leftRightSplitter);
    _imp->_leftRightSplitter->deleteLater();
    _imp->_leftRightSplitter = newSplitter;
    _imp->_leftRightSplitter->setChildrenCollapsible(false);
    _imp->_mainLayout->addWidget(newSplitter);

    getApp()->registerSplitter(newSplitter);

    // Re-create new menu actions
    createMenuActions();
 
} // Gui::wipeLayout

NATRON_NAMESPACE_EXIT
