/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2022 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#include <QtCore/QtGlobal> // for Q_OS_*
#include <QtCore/QDebug>
GCC_DIAG_UNUSED_PRIVATE_FIELD_OFF
// /opt/local/include/QtGui/qmime.h:119:10: warning: private field 'type' is not used [-Wunused-private-field]
#include <QCloseEvent>
GCC_DIAG_UNUSED_PRIVATE_FIELD_ON
#include <QApplication> // qApp
#include <QMenuBar>
#include <QUndoGroup>
#include <QDesktopServices>
#include <QtCore/QUrl>

#include "Engine/Node.h"
#include "Engine/Project.h"
#include "Engine/ViewerInstance.h"
#include "Engine/Settings.h"

#include "Global/StrUtils.h"

#include "Gui/ActionShortcuts.h"
#include "Gui/CurveEditor.h"
#include "Gui/DopeSheetEditor.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h" // appPTR
#include "Gui/GuiPrivate.h"
#include "Gui/Menu.h"
#include "Gui/NodeGraph.h"
#include "Gui/ProjectGui.h"
#include "Gui/ToolButton.h"
#include "Gui/RenderStatsDialog.h"
#include "Gui/ViewerTab.h"

NATRON_NAMESPACE_ENTER


// Helper function: Get the icon with the given name from the icon theme.
// If unavailable, fall back to the built-in icon. Icon names conform to this specification:
// http://standards.freedesktop.org/icon-naming-spec/icon-naming-spec-latest.html
static QIcon
get_icon(const std::string &name)
{
    QString str = QString::fromUtf8( name.c_str() );

    return QIcon::fromTheme( str, QIcon(QString::fromUtf8(":icons/") + str) );
}

Gui::Gui(const GuiAppInstancePtr& app,
         QWidget* parent)
#ifndef __NATRON_WIN32__
    : QMainWindow(parent)
#else
    : DocumentWindow(parent)
#endif
    , SerializableWindow()
    , _imp( new GuiPrivate(app, this) )

{
#ifdef __NATRON_WIN32
    //Register file types
    registerFileType(NATRON_PROJECT_FILE_MIME_TYPE, "Natron Project file", "." NATRON_PROJECT_FILE_EXT, 0, true);
    std::map<std::string, std::string> formats;
    appPTR->getCurrentSettings()->getFileFormatsForReadingAndReader(&formats);
    for (std::map<std::string, std::string>::iterator it = formats.begin(); it != formats.end(); ++it) {
        registerFileType(QString("image/") + QString( it->first.c_str() ), it->first.c_str(), QString(".") + QString( it->first.c_str() ), 0, true);
    }
    enableShellOpen();
#endif

    QObject::connect( this, SIGNAL(doDialog(int,QString,QString,bool,StandardButtons,int)), this,
                      SLOT(onDoDialog(int,QString,QString,bool,StandardButtons,int)) );
    QObject::connect( this, SIGNAL(doDialogWithStopAskingCheckbox(int,QString,QString,bool,StandardButtons,int)), this,
                      SLOT(onDoDialogWithStopAskingCheckbox(int,QString,QString,bool,StandardButtons,int)) );
    QObject::connect( app.get(), SIGNAL(pluginsPopulated()), this, SLOT(addToolButttonsToToolBar()) );
    QObject::connect( qApp, SIGNAL(focusChanged(QWidget*,QWidget*)), this, SLOT(onFocusChanged(QWidget*,QWidget*)) );
    QObject::connect (this, SIGNAL(s_showLogOnMainThread()), this, SLOT(onShowLogOnMainThreadReceived()));

    setAcceptDrops(true);

    setAttribute(Qt::WA_DeleteOnClose, true);

#ifdef Q_OS_MAC
     QObject::connect( appPTR, SIGNAL(dockClicked()), this, SLOT(dockClicked()) );
#endif
#ifdef DEBUG
    qDebug() << "Gui::Gui()" << (void*)(this);
#endif
}

Gui::~Gui()
{
#ifdef DEBUG
    qDebug() << "Gui::~Gui()" << (void*)(this);
#endif
    _imp->_nodeGraphArea->invalidateAllNodesParenting();
    delete _imp->_errorLog;
    delete _imp->_projectGui;
    delete _imp->_undoStacksGroup;
    _imp->_viewerTabs.clear();
    for (U32 i = 0; i < _imp->_toolButtons.size(); ++i) {
        delete _imp->_toolButtons[i];
    }
}

bool
Gui::isLeftToolBarDisplayedOnMouseHoverOnly() const
{
    return _imp->leftToolBarDisplayedOnHoverOnly;
}

void
Gui::setLeftToolBarDisplayedOnMouseHoverOnly(bool b)
{
    _imp->leftToolBarDisplayedOnHoverOnly = b;
    QPoint p = QCursor::pos();

    if (b) {
        _imp->refreshLeftToolBarVisibility( mapFromGlobal(p) );
    } else {
        _imp->_toolBox->show();
    }
}

void
Gui::refreshLeftToolBarVisibility(const QPoint & globalPos)
{
    _imp->refreshLeftToolBarVisibility( mapFromGlobal(globalPos) );
}

void
Gui::setLeftToolBarVisible(bool visible)
{
    _imp->_toolBox->setVisible(visible);
}

bool
Gui::closeProject()
{
    bool ret = abortProject(true, true, true);

    ///When closing a project we can remove the ViewerCache from memory and put it on disk
    ///since we're not sure it will be used right away
    appPTR->clearPlaybackCache();

    return ret;
}

void
Gui::reloadProject()
{
    ProjectPtr proj = getApp()->getProject();

    if ( !proj->hasProjectBeenSavedByUser() ) {
        Dialogs::errorDialog( tr("Reload project").toStdString(), tr("This project has not been saved yet").toStdString() );

        return;
    }
    QString filename = proj->getProjectFilename();
    QString projectPath = proj->getProjectPath();
    StrUtils::ensureLastPathSeparator(projectPath);

    projectPath.append(filename);

    if ( !abortProject(false, true, true) ) {
        return;
    }

    AppInstancePtr appInstance = openProjectInternal(projectPath.toStdString(), false);
    Q_UNUSED(appInstance);
}

void
Gui::setGuiAboutToClose(bool about)
{
    ///don't show dialogs when about to close, otherwise we could enter in a deadlock situation
    {
        QMutexLocker l(&_imp->aboutToCloseMutex);
        _imp->_aboutToClose = about;
    }
}

void
Gui::notifyGuiClosing()
{
    //Invalidates the Gui* pointer on most widgets to avoid attempts to dereference it in most events
    _imp->notifyGuiClosing();
}

bool
Gui::abortProject(bool quitApp,
                  bool warnUserIfSaveNeeded,
                  bool blocking)
{
    GuiAppInstancePtr app = getApp();

    // stop viewers before anything else
    std::list<ViewerTab*> viewers = getViewersList();
    for (std::list<ViewerTab*>::iterator it = viewers.begin(); it != viewers.end(); ++it) {
        (*it)->abortRendering();
    }

    if (app && app->getProject()->hasNodes() && warnUserIfSaveNeeded) {
        int ret = saveWarning();
        if (ret == 0) {
            if ( !saveProject() ) {
                return false;
            }
        } else if (ret == 1) {
            app->getProject()->removeLastAutosave();
        } else if (ret == 2) {
            return false;
        }
    }
    _imp->saveGuiGeometry();

    _imp->setUndoRedoActions(0, 0);
    if (quitApp) {
        if (app) {
            app->quit();
        }
    } else {
        {
            // we do not need this list anymore, avoid using it
            QMutexLocker k(&_imp->openedPanelsMutex);
            _imp->openedPanels.clear();
        }

        setGuiAboutToClose(true);
        if (app) {
            app->resetPreviewProvider();
            if (!blocking) {
                app->getProject()->reset(false /*aboutToQuit*/, false /*blocking*/);
            } else {
                app->getProject()->closeProject_blocking(false);
            }
        }
        centerAllNodeGraphsWithTimer();
        restoreDefaultLayout();
        setGuiAboutToClose(false);
    }

    ///Reset current undo/reso actions
    _imp->_currentUndoAction = 0;
    _imp->_currentRedoAction = 0;

    return true;
}

void
Gui::toggleFullScreen()
{
    QWidget* activeWin = qApp->activeWindow();

    if (!activeWin) {
        qApp->setActiveWindow(this);
        activeWin = this;
    }

    if ( activeWin->isFullScreen() ) {
        activeWin->showNormal();
    } else {
        activeWin->showFullScreen();
    }
}

void
Gui::closeEvent(QCloseEvent* e)
{
    assert(e);
    GuiAppInstancePtr app = getApp();
    if ( app && app->isClosing() ) {
        e->ignore();
    } else {
        if ( !abortProject(true, true, true) ) {
            e->ignore();

            return;
        }
        e->accept();
    }
}

NodeGuiPtr
Gui::createNodeGUI(NodePtr node,
                   const CreateNodeArgs& args)
{
    assert(_imp->_nodeGraphArea);

    NodeCollectionPtr group = node->getGroup();
    NodeGraph* graph;
    if (group) {
        NodeGraphI* graph_i = group->getNodeGraph();
        assert(graph_i);
        graph = dynamic_cast<NodeGraph*>(graph_i);
        assert(graph);
    } else {
        graph = _imp->_nodeGraphArea;
    }
    if (!graph) {
        throw std::logic_error("");
    }
    NodeGuiPtr nodeGui = graph->createNodeGUI(node, args);
    QObject::connect( node.get(), SIGNAL(labelChanged(QString)), this, SLOT(onNodeNameChanged(QString)) );
    assert(nodeGui);

    return nodeGui;
}

void
Gui::addNodeGuiToCurveEditor(const NodeGuiPtr &node)
{
    _imp->_curveEditor->addNode(node);
}

void
Gui::removeNodeGuiFromCurveEditor(const NodeGuiPtr& node)
{
    _imp->_curveEditor->removeNode( node.get() );
}

void
Gui::addNodeGuiToDopeSheetEditor(const NodeGuiPtr &node)
{
    _imp->_dopeSheetEditor->addNode(node);
}

void
Gui::removeNodeGuiFromDopeSheetEditor(const NodeGuiPtr &node)
{
    _imp->_dopeSheetEditor->removeNode( node.get() );
}

void
Gui::createViewerGui(NodePtr viewer)
{
    TabWidget* where = _imp->_nextViewerTabPlace;

    if (!where) {
        where = getAnchor();
    } else {
        _imp->_nextViewerTabPlace = NULL; // < resetting next viewer anchor to default
    }
    assert(where);

    ViewerInstance* v = viewer->isEffectViewer();
    assert(v);

    ViewerTab* tab = addNewViewerTab(v, where);
    NodeGraph* graph = 0;
    NodeCollectionPtr collection = viewer->getGroup();
    if (!collection) {
        return;
    }
    NodeGroup* isGrp = dynamic_cast<NodeGroup*>( collection.get() );
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
    graph->setLastSelectedViewer(tab);
}

const NodesGuiList &
Gui::getSelectedNodes() const
{
    assert(_imp->_nodeGraphArea);

    return _imp->_nodeGraphArea->getSelectedNodes();
}

void
Gui::createGui()
{
    setupUi();

    ///post a fake event so the qt handlers are called and the proper widget receives the focus
    QMouseEvent e(QEvent::MouseMove, QCursor::pos(), Qt::NoButton, Qt::NoButton, Qt::NoModifier);
    qApp->sendEvent(this, &e);
}

bool
Gui::eventFilter(QObject *target,
                 QEvent* e)
{
    if (_imp->_aboutToClose) {
        return true;
    }
    if ( dynamic_cast<QInputEvent*>(e) ) {
        /*Make top level instance this instance since it receives all
           user inputs.*/
        GuiAppInstancePtr app = getApp();
        if (app) {
            appPTR->setAsTopLevelInstance( app->getAppID() );
        }
    }

    return QMainWindow::eventFilter(target, e);
}

void
Gui::createMenuActions()
{
    _imp->menubar = new QMenuBar(this);
    setMenuBar(_imp->menubar);

    _imp->menuFile = new Menu(tr("File"), _imp->menubar);
    _imp->menuRecentFiles = new Menu(tr("Open Recent"), _imp->menuFile);
    _imp->menuEdit = new Menu(tr("Edit"), _imp->menubar);
    _imp->menuLayout = new Menu(tr("Layout"), _imp->menubar);
    _imp->menuDisplay = new Menu(tr("Display"), _imp->menubar);
    _imp->menuRender = new Menu(tr("Render"), _imp->menubar);
    _imp->viewersMenu = new Menu(tr("Viewer(s)"), _imp->menuDisplay);
    _imp->viewerInputsMenu = new Menu(tr("Connect to A Side"), _imp->viewersMenu);
    _imp->viewerInputsBMenu = new Menu(tr("Connect to B Side"), _imp->viewersMenu);
    _imp->viewersViewMenu = new Menu(tr("View"), _imp->viewersMenu);
    _imp->cacheMenu = new Menu(tr("Cache"), _imp->menubar);
    _imp->menuHelp = new Menu(tr("Help"), _imp->menubar);


    _imp->actionNew_project = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionNewProject, kShortcutDescActionNewProject, this);
    _imp->actionNew_project->setIcon( get_icon("document-new") );
    QObject::connect( _imp->actionNew_project, SIGNAL(triggered()), this, SLOT(newProject()) );

    _imp->actionOpen_project = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionOpenProject, kShortcutDescActionOpenProject, this);
    _imp->actionOpen_project->setIcon( get_icon("document-open") );
    QObject::connect( _imp->actionOpen_project, SIGNAL(triggered()), this, SLOT(openProject()) );

    _imp->actionClose_project = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionCloseProject, kShortcutDescActionCloseProject, this);
    _imp->actionClose_project->setIcon( get_icon("document-close") );
    QObject::connect( _imp->actionClose_project, SIGNAL(triggered()), this, SLOT(closeProject()) );

    _imp->actionReload_project = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionReloadProject, kShortcutDescActionReloadProject, this);
    _imp->actionReload_project->setIcon( get_icon("document-open") );
    QObject::connect( _imp->actionReload_project, SIGNAL(triggered()), this, SLOT(reloadProject()) );

    _imp->actionSave_project = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionSaveProject, kShortcutDescActionSaveProject, this);
    _imp->actionSave_project->setIcon( get_icon("document-save") );
    QObject::connect( _imp->actionSave_project, SIGNAL(triggered()), this, SLOT(saveProject()) );

    _imp->actionSaveAs_project = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionSaveAsProject, kShortcutDescActionSaveAsProject, this);
    _imp->actionSaveAs_project->setIcon( get_icon("document-save-as") );
    QObject::connect( _imp->actionSaveAs_project, SIGNAL(triggered()), this, SLOT(saveProjectAs()) );

    _imp->actionExportAsGroup = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionExportProject, kShortcutDescActionExportProject, this);
    _imp->actionExportAsGroup->setIcon( get_icon("document-save-as") );
    QObject::connect( _imp->actionExportAsGroup, SIGNAL(triggered()), this, SLOT(exportProjectAsGroup()) );

    _imp->actionSaveAndIncrementVersion = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionSaveAndIncrVersion, kShortcutDescActionSaveAndIncrVersion, this);
    QObject::connect( _imp->actionSaveAndIncrementVersion, SIGNAL(triggered()), this, SLOT(saveAndIncrVersion()) );

    _imp->actionPreferences = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionPreferences, kShortcutDescActionPreferences, this);
    _imp->actionPreferences->setMenuRole(QAction::PreferencesRole);
    QObject::connect( _imp->actionPreferences, SIGNAL(triggered()), this, SLOT(showSettings()) );

    _imp->actionExit = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionQuit, kShortcutDescActionQuit, this);
    _imp->actionExit->setMenuRole(QAction::QuitRole);
    _imp->actionExit->setIcon( get_icon("application-exit") );
    QObject::connect( _imp->actionExit, SIGNAL(triggered()), appPTR, SLOT(exitAppWithSaveWarning()) );

    _imp->actionProject_settings = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionProjectSettings, kShortcutDescActionProjectSettings, this);
    _imp->actionProject_settings->setIcon( get_icon("document-properties") );
    QObject::connect( _imp->actionProject_settings, SIGNAL(triggered()), this, SLOT(setVisibleProjectSettingsPanel()) );

    _imp->actionShowErrorLog = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionShowErrorLog, kShortcutDescActionShowErrorLog, this);
    QObject::connect( _imp->actionShowErrorLog, SIGNAL(triggered()), this, SLOT(showErrorLog()) );

    _imp->actionNewViewer = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionNewViewer, kShortcutDescActionNewViewer, this);
    QObject::connect( _imp->actionNewViewer, SIGNAL(triggered()), this, SLOT(createNewViewer()) );


#ifdef __NATRON_WIN32__
    _imp->actionShowWindowsConsole = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionShowWindowsConsole, kShortcutDescActionShowWindowsConsole, this);
    _imp->actionShowWindowsConsole->setShortcutContext(Qt::ApplicationShortcut);
    QObject::connect( _imp->actionShowWindowsConsole, SIGNAL(triggered()), this, SLOT(onShowApplicationConsoleActionTriggered()) );
#endif

    _imp->actionFullScreen = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionFullscreen, kShortcutDescActionFullscreen, this);
    _imp->actionFullScreen->setIcon( get_icon("view-fullscreen") );
    _imp->actionFullScreen->setShortcutContext(Qt::ApplicationShortcut);
    QObject::connect( _imp->actionFullScreen, SIGNAL(triggered()), this, SLOT(toggleFullScreen()) );

    _imp->actionClearDiskCache = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionClearDiskCache, kShortcutDescActionClearDiskCache, this);
    QObject::connect( _imp->actionClearDiskCache, SIGNAL(triggered()), appPTR, SLOT(clearDiskCache()) );

    _imp->actionClearPlayBackCache = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionClearPlaybackCache, kShortcutDescActionClearPlaybackCache, this);
    QObject::connect( _imp->actionClearPlayBackCache, SIGNAL(triggered()), appPTR, SLOT(clearPlaybackCache()) );

    _imp->actionClearNodeCache = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionClearNodeCache, kShortcutDescActionClearNodeCache, this);
    QObject::connect( _imp->actionClearNodeCache, SIGNAL(triggered()), appPTR, SLOT(clearNodeCache()) );
    GuiAppInstancePtr app = getApp();
    if (app) {
        QObject::connect( _imp->actionClearNodeCache, SIGNAL(triggered()), app.get(), SLOT(clearOpenFXPluginsCaches()) );
    }

    _imp->actionClearPluginsLoadingCache = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionClearPluginsLoadCache, kShortcutDescActionClearPluginsLoadCache, this);
    QObject::connect( _imp->actionClearPluginsLoadingCache, SIGNAL(triggered()), appPTR, SLOT(clearPluginsLoadedCache()) );

    _imp->actionClearAllCaches = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionClearAllCaches, kShortcutDescActionClearAllCaches, this);
    QObject::connect( _imp->actionClearAllCaches, SIGNAL(triggered()), appPTR, SLOT(clearAllCaches()) );

    _imp->actionShowAboutWindow = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionShowAbout, kShortcutDescActionShowAbout, this);
    _imp->actionShowAboutWindow->setMenuRole(QAction::AboutRole);
    QObject::connect( _imp->actionShowAboutWindow, SIGNAL(triggered()), this, SLOT(showAbout()) );

    _imp->renderAllWriters = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionRenderAll, kShortcutDescActionRenderAll, this);
    QObject::connect( _imp->renderAllWriters, SIGNAL(triggered()), this, SLOT(renderAllWriters()) );

    _imp->renderSelectedNode = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionRenderSelected, kShortcutDescActionRenderSelected, this);
    QObject::connect( _imp->renderSelectedNode, SIGNAL(triggered()), this, SLOT(renderSelectedNode()) );

    _imp->enableRenderStats = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionEnableRenderStats, kShortcutDescActionEnableRenderStats, this);
    _imp->enableRenderStats->setCheckable(true);
    _imp->enableRenderStats->setChecked(false);
    QObject::connect( _imp->enableRenderStats, SIGNAL(triggered()), this, SLOT(onEnableRenderStatsActionTriggered()) );

    for (int c = 0; c < NATRON_MAX_RECENT_FILES; ++c) {
        _imp->actionsOpenRecentFile[c] = new QAction(this);
        _imp->actionsOpenRecentFile[c]->setVisible(false);
        connect( _imp->actionsOpenRecentFile[c], SIGNAL(triggered()), this, SLOT(openRecentFile()) );
    }

    const char* descs[NATRON_CONNECT_INPUT_NB] = {
        kShortcutDescActionConnectViewerToInput1,
        kShortcutDescActionConnectViewerToInput2,
        kShortcutDescActionConnectViewerToInput3,
        kShortcutDescActionConnectViewerToInput4,
        kShortcutDescActionConnectViewerToInput5,
        kShortcutDescActionConnectViewerToInput6,
        kShortcutDescActionConnectViewerToInput7,
        kShortcutDescActionConnectViewerToInput8,
        kShortcutDescActionConnectViewerToInput9,
        kShortcutDescActionConnectViewerToInput10,
        kShortcutDescActionConnectViewerBToInput1,
        kShortcutDescActionConnectViewerBToInput2,
        kShortcutDescActionConnectViewerBToInput3,
        kShortcutDescActionConnectViewerBToInput4,
        kShortcutDescActionConnectViewerBToInput5,
        kShortcutDescActionConnectViewerBToInput6,
        kShortcutDescActionConnectViewerBToInput7,
        kShortcutDescActionConnectViewerBToInput8,
        kShortcutDescActionConnectViewerBToInput9,
        kShortcutDescActionConnectViewerBToInput10
    };
    const char* ids[NATRON_CONNECT_INPUT_NB] = {
        kShortcutIDActionConnectViewerToInput1,
        kShortcutIDActionConnectViewerToInput2,
        kShortcutIDActionConnectViewerToInput3,
        kShortcutIDActionConnectViewerToInput4,
        kShortcutIDActionConnectViewerToInput5,
        kShortcutIDActionConnectViewerToInput6,
        kShortcutIDActionConnectViewerToInput7,
        kShortcutIDActionConnectViewerToInput8,
        kShortcutIDActionConnectViewerToInput9,
        kShortcutIDActionConnectViewerToInput10,
        kShortcutIDActionConnectViewerBToInput1,
        kShortcutIDActionConnectViewerBToInput2,
        kShortcutIDActionConnectViewerBToInput3,
        kShortcutIDActionConnectViewerBToInput4,
        kShortcutIDActionConnectViewerBToInput5,
        kShortcutIDActionConnectViewerBToInput6,
        kShortcutIDActionConnectViewerBToInput7,
        kShortcutIDActionConnectViewerBToInput8,
        kShortcutIDActionConnectViewerBToInput9,
        kShortcutIDActionConnectViewerBToInput10
    };


    for (int i = 0; i < NATRON_CONNECT_INPUT_NB; ++i) {
        _imp->actionConnectInput[i] = new ActionWithShortcut(kShortcutGroupGlobal, ids[i], descs[i], this);
        _imp->actionConnectInput[i]->setData( i % (NATRON_CONNECT_INPUT_NB / 2) );
        _imp->actionConnectInput[i]->setShortcutContext(Qt::WidgetShortcut);
        if (i < NATRON_CONNECT_INPUT_NB / 2) {
            QObject::connect( _imp->actionConnectInput[i], SIGNAL(triggered()), this, SLOT(connectAInput()) );
        } else {
            QObject::connect( _imp->actionConnectInput[i], SIGNAL(triggered()), this, SLOT(connectBInput()) );
        }
    }

    _imp->actionImportLayout = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionImportLayout, kShortcutDescActionImportLayout, this);
    QObject::connect( _imp->actionImportLayout, SIGNAL(triggered()), this, SLOT(importLayout()) );

    _imp->actionExportLayout = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionExportLayout, kShortcutDescActionExportLayout, this);
    QObject::connect( _imp->actionExportLayout, SIGNAL(triggered()), this, SLOT(exportLayout()) );

    _imp->actionRestoreDefaultLayout = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionDefaultLayout, kShortcutDescActionDefaultLayout, this);
    QObject::connect( _imp->actionRestoreDefaultLayout, SIGNAL(triggered()), this, SLOT(restoreDefaultLayout()) );

    _imp->actionPrevTab = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionPrevTab, kShortcutDescActionPrevTab, this);
    QObject::connect( _imp->actionPrevTab, SIGNAL(triggered()), this, SLOT(onPrevTabTriggered()) );
    _imp->actionNextTab = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionNextTab, kShortcutDescActionNextTab, this);
    QObject::connect( _imp->actionNextTab, SIGNAL(triggered()), this, SLOT(onNextTabTriggered()) );
    _imp->actionCloseTab = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionCloseTab, kShortcutDescActionCloseTab, this);
    QObject::connect( _imp->actionCloseTab, SIGNAL(triggered()), this, SLOT(onCloseTabTriggered()) );

    _imp->menubar->addAction( _imp->menuFile->menuAction() );
    _imp->menubar->addAction( _imp->menuEdit->menuAction() );
    _imp->menubar->addAction( _imp->menuLayout->menuAction() );
    _imp->menubar->addAction( _imp->menuDisplay->menuAction() );
    _imp->menubar->addAction( _imp->menuRender->menuAction() );
    _imp->menubar->addAction( _imp->cacheMenu->menuAction() );
    _imp->menubar->addAction( _imp->menuHelp->menuAction() );

#ifdef __APPLE__
    _imp->menuFile->addAction(_imp->actionShowAboutWindow);
#endif

    _imp->menuFile->addAction(_imp->actionNew_project);
    _imp->menuFile->addAction(_imp->actionOpen_project);
    _imp->menuFile->addAction( _imp->menuRecentFiles->menuAction() );
    updateRecentFileActions();
    for (int c = 0; c < NATRON_MAX_RECENT_FILES; ++c) {
        _imp->menuRecentFiles->addAction(_imp->actionsOpenRecentFile[c]);
    }

    _imp->menuFile->addAction(_imp->actionReload_project);
    _imp->menuFile->addSeparator();
    _imp->menuFile->addAction(_imp->actionClose_project);
    _imp->menuFile->addAction(_imp->actionSave_project);
    _imp->menuFile->addAction(_imp->actionSaveAs_project);
    _imp->menuFile->addAction(_imp->actionSaveAndIncrementVersion);
    _imp->menuFile->addAction(_imp->actionExportAsGroup);
    _imp->menuFile->addSeparator();
    _imp->menuFile->addAction(_imp->actionExit);

    _imp->menuEdit->addAction(_imp->actionPreferences);

    _imp->menuLayout->addAction(_imp->actionImportLayout);
    _imp->menuLayout->addAction(_imp->actionExportLayout);
    _imp->menuLayout->addAction(_imp->actionRestoreDefaultLayout);
    _imp->menuLayout->addAction(_imp->actionPrevTab);
    _imp->menuLayout->addAction(_imp->actionNextTab);
    _imp->menuLayout->addAction(_imp->actionCloseTab);

    _imp->menuDisplay->addAction(_imp->actionNewViewer);
    _imp->menuDisplay->addAction( _imp->viewersMenu->menuAction() );
    _imp->viewersMenu->addAction( _imp->viewerInputsMenu->menuAction() );
    _imp->viewersMenu->addAction( _imp->viewerInputsBMenu->menuAction() );
    _imp->viewersMenu->addAction( _imp->viewersViewMenu->menuAction() );
    for (int i = 0; i < NATRON_CONNECT_INPUT_NB; ++i) {
        if ( i < (NATRON_CONNECT_INPUT_NB / 2) ) {
            _imp->viewerInputsMenu->addAction(_imp->actionConnectInput[i]);
        } else {
            _imp->viewerInputsBMenu->addAction(_imp->actionConnectInput[i]);
        }
    }

    _imp->menuDisplay->addAction(_imp->actionProject_settings);
    _imp->menuDisplay->addAction(_imp->actionShowErrorLog);
#ifdef __NATRON_WIN32__
    _imp->menuDisplay->addAction(_imp->actionShowWindowsConsole);
#endif

    _imp->menuDisplay->addSeparator();
    _imp->menuDisplay->addAction(_imp->actionFullScreen);

    _imp->menuRender->addAction(_imp->renderAllWriters);
    _imp->menuRender->addAction(_imp->renderSelectedNode);
    _imp->menuRender->addAction(_imp->enableRenderStats);

    _imp->cacheMenu->addAction(_imp->actionClearDiskCache);
    _imp->cacheMenu->addAction(_imp->actionClearPlayBackCache);
    _imp->cacheMenu->addAction(_imp->actionClearNodeCache);
    _imp->cacheMenu->addAction(_imp->actionClearAllCaches);
    _imp->cacheMenu->addSeparator();
    _imp->cacheMenu->addAction(_imp->actionClearPluginsLoadingCache);

    // Help menu
    _imp->actionHelpDocumentation = new QAction(this);
    _imp->actionHelpDocumentation->setText( tr("Documentation") );
    _imp->menuHelp->addAction(_imp->actionHelpDocumentation);
    QObject::connect( _imp->actionHelpDocumentation, SIGNAL(triggered()), this, SLOT(openHelpDocumentation()) );

    _imp->actionHelpWebsite = new QAction(this);
    _imp->actionHelpWebsite->setText( tr("Website") );
    _imp->menuHelp->addAction(_imp->actionHelpWebsite);
    QObject::connect( _imp->actionHelpWebsite, SIGNAL(triggered()), this, SLOT(openHelpWebsite()) );

    _imp->actionHelpForum = new QAction(this);
    _imp->actionHelpForum->setText( tr("Forum") );
    _imp->menuHelp->addAction(_imp->actionHelpForum);
    QObject::connect( _imp->actionHelpForum, SIGNAL(triggered()), this, SLOT(openHelpForum()) );

    _imp->actionHelpIssues = new QAction(this);
    _imp->actionHelpIssues->setText( tr("Issues") );
    _imp->menuHelp->addAction(_imp->actionHelpIssues);
    QObject::connect( _imp->actionHelpIssues, SIGNAL(triggered()), this, SLOT(openHelpIssues()) );

#ifndef __APPLE__
    _imp->menuHelp->addSeparator();
    _imp->menuHelp->addAction(_imp->actionShowAboutWindow);
#endif

    ///Create custom menu
    const std::list<PythonUserCommand> & commands = appPTR->getUserPythonCommands();
    for (std::list<PythonUserCommand>::const_iterator it = commands.begin(); it != commands.end(); ++it) {
        addMenuEntry(it->grouping, it->pythonFunction, it->key, it->modifiers);
    }
} // createMenuActions

void
Gui::openHelpWebsite()
{
    QDesktopServices::openUrl( QUrl( QString::fromUtf8(NATRON_WEBSITE_URL) ) );
}

void
Gui::openHelpForum()
{
    QDesktopServices::openUrl( QUrl( QString::fromUtf8(NATRON_FORUM_URL) ) );
}

void
Gui::openHelpIssues()
{
    QDesktopServices::openUrl( QUrl( QString::fromUtf8(NATRON_ISSUE_TRACKER_URL) ) );
}

void
Gui::openHelpDocumentation()
{
    int serverPort = appPTR->getDocumentationServerPort();

    QString localUrl = QString::fromUtf8("http://localhost:") + QString::number(serverPort);
#ifdef NATRON_DOCUMENTATION_ONLINE
    int docSource = appPTR->getCurrentSettings()->getDocumentationSource();
    QString remoteUrl = QString::fromUtf8(NATRON_DOCUMENTATION_ONLINE);
    if ( (serverPort == 0) && (docSource == 0) ) {
        docSource = 1;
    }

    switch (docSource) {
    case 0:
        QDesktopServices::openUrl( QUrl(localUrl) );
        break;
    case 1:
        QDesktopServices::openUrl( QUrl(remoteUrl) );
        break;
    case 2:
        Dialogs::informationDialog(tr("Missing documentation").toStdString(), tr("Missing documentation, please go to settings and select local or online documentation source.").toStdString(), true);
        break;
    }
#else
    QDesktopServices::openUrl( QUrl(localUrl) );
#endif
}

#ifdef Q_OS_MAC
void
Gui::dockClicked()
{
    raise();
    show();
    setWindowState( (windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
}
#endif

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_Gui.cpp"
