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

#include "Gui.h"

#include <cassert>
#include <stdexcept>

#include "Global/Macros.h"

GCC_DIAG_UNUSED_PRIVATE_FIELD_OFF
// /opt/local/include/QtGui/qmime.h:119:10: warning: private field 'type' is not used [-Wunused-private-field]
#include <QCloseEvent>
GCC_DIAG_UNUSED_PRIVATE_FIELD_ON
#include <QApplication> // qApp
#include <QMenuBar>
#include <QUndoGroup>
#include <QDesktopServices>
#include <QUrl>

#include "Engine/Node.h"
#include "Engine/Project.h"
#include "Engine/ViewerInstance.h"

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

NATRON_NAMESPACE_ENTER;


// Helper function: Get the icon with the given name from the icon theme.
// If unavailable, fall back to the built-in icon. Icon names conform to this specification:
// http://standards.freedesktop.org/icon-naming-spec/icon-naming-spec-latest.html
static QIcon
get_icon(const QString &name)
{
    return QIcon::fromTheme( name, QIcon(QString(":icons/") + name) );
}

Gui::Gui(GuiAppInstance* app,
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
    std::map<std::string,std::string> formats;
    appPTR->getCurrentSettings()->getFileFormatsForReadingAndReader(&formats);
    for (std::map<std::string,std::string>::iterator it = formats.begin(); it!=formats.end(); ++it) {
        registerFileType(QString("image/") + QString(it->first.c_str()), it->first.c_str(), QString(".") + QString(it->first.c_str()), 0, true);
    }
    enableShellOpen();
#endif
    
    QObject::connect( this, SIGNAL( doDialog(int, QString, QString, bool, StandardButtons, int) ), this,
                     SLOT( onDoDialog(int, QString, QString, bool, StandardButtons, int) ) );
    QObject::connect( this, SIGNAL( doDialogWithStopAskingCheckbox(int, QString, QString, bool, StandardButtons, int) ), this,
                      SLOT( onDoDialogWithStopAskingCheckbox(int, QString, QString, bool, StandardButtons, int) ) );
    QObject::connect( app, SIGNAL( pluginsPopulated() ), this, SLOT( addToolButttonsToToolBar() ) );
    
    QObject::connect(qApp, SIGNAL(focusChanged(QWidget*,QWidget*)), this, SLOT(onFocusChanged(QWidget*, QWidget*)));

}

Gui::~Gui()
{
    _imp->_nodeGraphArea->invalidateAllNodesParenting();
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
Gui::closeInstance(bool warnUserIfSaveNeeded)
{
    return abortProject(true, warnUserIfSaveNeeded);
}

void
Gui::closeProject()
{
    closeInstance(true);

    ///When closing a project we can remove the ViewerCache from memory and put it on disk
    ///since we're not sure it will be used right away
    appPTR->clearPlaybackCache();
    //_imp->_appInstance->getProject()->createViewer();
    //_imp->_appInstance->execOnProjectCreatedCallback();
}

void
Gui::reloadProject()
{
    
    boost::shared_ptr<Project> proj = getApp()->getProject();
    if (!proj->hasProjectBeenSavedByUser()) {
        Dialogs::errorDialog(tr("Reload project").toStdString(), tr("This project has not been saved yet").toStdString());
        return;
    }
    QString filename = proj->getProjectFilename();
    QString projectPath = proj->getProjectPath();
    if (!projectPath.endsWith("/")) {
        projectPath.append('/');
    }
    projectPath.append(filename);
    
    if (!abortProject(false, true)) {
        return;
    }
   
    (void)openProjectInternal(projectPath.toStdString(), false);
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
Gui::abortProject(bool quitApp, bool warnUserIfSaveNeeded)
{
    if (getApp()->getProject()->hasNodes() && warnUserIfSaveNeeded) {
        int ret = saveWarning();
        if (ret == 0) {
            if ( !saveProject() ) {
                return false;
            }
        } else if (ret == 2) {
            return false;
        }
    }
    _imp->saveGuiGeometry();
    
    _imp->setUndoRedoActions(0,0);
    if (quitApp) {
        assert(_imp->_appInstance);
        _imp->_appInstance->quit();
    } else {
        setGuiAboutToClose(true);
        _imp->_appInstance->resetPreviewProvider();
        _imp->_appInstance->getProject()->closeProject(false);
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
    if ( _imp->_appInstance->isClosing() ) {
        e->ignore();
    } else {
        if ( !closeInstance(true) ) {
            e->ignore();

            return;
        }
        e->accept();
    }
}

boost::shared_ptr<NodeGui>
Gui::createNodeGUI(boost::shared_ptr<Node> node,
                   const CreateNodeArgs& args)
{
    assert(_imp->_nodeGraphArea);
    
    boost::shared_ptr<NodeCollection> group = node->getGroup();
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
    boost::shared_ptr<NodeGui> nodeGui = graph->createNodeGUI(node, args);
    QObject::connect( node.get(), SIGNAL( labelChanged(QString) ), this, SLOT( onNodeNameChanged(QString) ) );
    assert(nodeGui);

    return nodeGui;
}

void
Gui::addNodeGuiToCurveEditor(const boost::shared_ptr<NodeGui> &node)
{
    _imp->_curveEditor->addNode(node);
}

void
Gui::removeNodeGuiFromCurveEditor(const boost::shared_ptr<NodeGui>& node)
{
    _imp->_curveEditor->removeNode(node.get());
}

void
Gui::addNodeGuiToDopeSheetEditor(const boost::shared_ptr<NodeGui> &node)
{
    _imp->_dopeSheetEditor->addNode(node);
}

void
Gui::removeNodeGuiFromDopeSheetEditor(const boost::shared_ptr<NodeGui> &node)
{
    _imp->_dopeSheetEditor->removeNode(node.get());
}

void
Gui::createViewerGui(boost::shared_ptr<Node> viewer)
{
    TabWidget* where = _imp->_nextViewerTabPlace;

    if (!where) {
        where = getAnchor();
    } else {
        _imp->_nextViewerTabPlace = NULL; // < reseting next viewer anchor to default
    }
    assert(where);

    ViewerInstance* v = dynamic_cast<ViewerInstance*>( viewer->getLiveInstance() );
    assert(v);

    ViewerTab* tab = addNewViewerTab(v, where);
    
    NodeGraph* graph = 0;
    boost::shared_ptr<NodeCollection> collection = viewer->getGroup();
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

const std::list<boost::shared_ptr<NodeGui> > &
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
    assert(_imp->_appInstance);
    if ( dynamic_cast<QInputEvent*>(e) ) {
        /*Make top level instance this instance since it receives all
           user inputs.*/
        appPTR->setAsTopLevelInstance( _imp->_appInstance->getAppID() );
    }

    return QMainWindow::eventFilter(target, e);
}

void
Gui::createMenuActions()
{
    _imp->menubar = new QMenuBar(this);
    setMenuBar(_imp->menubar);

    _imp->menuFile = new Menu(QObject::tr("File"), _imp->menubar);
    _imp->menuRecentFiles = new Menu(QObject::tr("Open Recent"), _imp->menuFile);
    _imp->menuEdit = new Menu(QObject::tr("Edit"), _imp->menubar);
    _imp->menuLayout = new Menu(QObject::tr("Layout"), _imp->menubar);
    _imp->menuDisplay = new Menu(QObject::tr("Display"), _imp->menubar);
    _imp->menuRender = new Menu(QObject::tr("Render"), _imp->menubar);
    _imp->viewersMenu = new Menu(QObject::tr("Viewer(s)"), _imp->menuDisplay);
    _imp->viewerInputsMenu = new Menu(QObject::tr("Connect Current Viewer"), _imp->viewersMenu);
    _imp->viewersViewMenu = new Menu(QObject::tr("Display View Number"), _imp->viewersMenu);
    _imp->cacheMenu = new Menu(QObject::tr("Cache"), _imp->menubar);
    _imp->menuHelp = new Menu(QObject::tr("Help"), _imp->menubar);


    _imp->actionNew_project = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionNewProject, kShortcutDescActionNewProject, this);
    _imp->actionNew_project->setIcon( get_icon("document-new") );
    QObject::connect( _imp->actionNew_project, SIGNAL( triggered() ), this, SLOT( newProject() ) );

    _imp->actionOpen_project = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionOpenProject, kShortcutDescActionOpenProject, this);
    _imp->actionOpen_project->setIcon( get_icon("document-open") );
    QObject::connect( _imp->actionOpen_project, SIGNAL( triggered() ), this, SLOT( openProject() ) );

    _imp->actionClose_project = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionCloseProject, kShortcutDescActionCloseProject, this);
    _imp->actionClose_project->setIcon( get_icon("document-close") );
    QObject::connect( _imp->actionClose_project, SIGNAL( triggered() ), this, SLOT( closeProject() ) );

    _imp->actionReload_project = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionReloadProject, kShortcutDescActionReloadProject, this);
    _imp->actionReload_project->setIcon( get_icon("document-open") );
    QObject::connect( _imp->actionReload_project, SIGNAL( triggered() ), this, SLOT( reloadProject() ) );
    
    _imp->actionSave_project = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionSaveProject, kShortcutDescActionSaveProject, this);
    _imp->actionSave_project->setIcon( get_icon("document-save") );
    QObject::connect( _imp->actionSave_project, SIGNAL( triggered() ), this, SLOT( saveProject() ) );

    _imp->actionSaveAs_project = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionSaveAsProject, kShortcutDescActionSaveAsProject, this);
    _imp->actionSaveAs_project->setIcon( get_icon("document-save-as") );
    QObject::connect( _imp->actionSaveAs_project, SIGNAL( triggered() ), this, SLOT( saveProjectAs() ) );

    _imp->actionExportAsGroup = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionExportProject, kShortcutDescActionExportProject, this);
    _imp->actionExportAsGroup->setIcon( get_icon("document-save-as") );
    QObject::connect( _imp->actionExportAsGroup, SIGNAL( triggered() ), this, SLOT( exportProjectAsGroup() ) );

    _imp->actionSaveAndIncrementVersion = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionSaveAndIncrVersion, kShortcutDescActionSaveAndIncrVersion, this);
    QObject::connect( _imp->actionSaveAndIncrementVersion, SIGNAL( triggered() ), this, SLOT( saveAndIncrVersion() ) );

    _imp->actionPreferences = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionPreferences, kShortcutDescActionPreferences, this);
    _imp->actionPreferences->setMenuRole(QAction::PreferencesRole);
    QObject::connect( _imp->actionPreferences, SIGNAL( triggered() ), this, SLOT( showSettings() ) );

    _imp->actionExit = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionQuit, kShortcutDescActionQuit, this);
    _imp->actionExit->setMenuRole(QAction::QuitRole);
    _imp->actionExit->setIcon( get_icon("application-exit") );
    QObject::connect( _imp->actionExit, SIGNAL( triggered() ), appPTR, SLOT( exitAppWithSaveWarning() ) );

    _imp->actionProject_settings = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionProjectSettings, kShortcutDescActionProjectSettings, this);
    _imp->actionProject_settings->setIcon( get_icon("document-properties") );
    QObject::connect( _imp->actionProject_settings, SIGNAL( triggered() ), this, SLOT( setVisibleProjectSettingsPanel() ) );

    _imp->actionShowOfxLog = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionShowOFXLog, kShortcutDescActionShowOFXLog, this);
    QObject::connect( _imp->actionShowOfxLog, SIGNAL( triggered() ), this, SLOT( showOfxLog() ) );

    _imp->actionShortcutEditor = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionShowShortcutEditor, kShortcutDescActionShowShortcutEditor, this);
    QObject::connect( _imp->actionShortcutEditor, SIGNAL( triggered() ), this, SLOT( showShortcutEditor() ) );

    _imp->actionNewViewer = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionNewViewer, kShortcutDescActionNewViewer, this);
    QObject::connect( _imp->actionNewViewer, SIGNAL( triggered() ), this, SLOT( createNewViewer() ) );


#ifdef __NATRON_WIN32__
    _imp->actionShowWindowsConsole = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionShowWindowsConsole, kShortcutDescActionShowWindowsConsole, this);
    _imp->actionShowWindowsConsole->setShortcutContext(Qt::ApplicationShortcut);
    QObject::connect( _imp->actionShowWindowsConsole, SIGNAL( triggered() ), this, SLOT( onShowApplicationConsoleActionTriggered() ) );
#endif
    
    _imp->actionFullScreen = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionFullscreen, kShortcutDescActionFullscreen, this);
    _imp->actionFullScreen->setIcon( get_icon("view-fullscreen") );
    _imp->actionFullScreen->setShortcutContext(Qt::ApplicationShortcut);
    QObject::connect( _imp->actionFullScreen, SIGNAL( triggered() ), this, SLOT( toggleFullScreen() ) );

    _imp->actionClearDiskCache = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionClearDiskCache, kShortcutDescActionClearDiskCache, this);
    QObject::connect( _imp->actionClearDiskCache, SIGNAL( triggered() ), appPTR, SLOT( clearDiskCache() ) );

    _imp->actionClearPlayBackCache = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionClearPlaybackCache, kShortcutDescActionClearPlaybackCache, this);
    QObject::connect( _imp->actionClearPlayBackCache, SIGNAL( triggered() ), appPTR, SLOT( clearPlaybackCache() ) );

    _imp->actionClearNodeCache = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionClearNodeCache, kShortcutDescActionClearNodeCache, this);
    QObject::connect( _imp->actionClearNodeCache, SIGNAL( triggered() ), appPTR, SLOT( clearNodeCache() ) );
    QObject::connect( _imp->actionClearNodeCache, SIGNAL( triggered() ), _imp->_appInstance, SLOT( clearOpenFXPluginsCaches() ) );

    _imp->actionClearPluginsLoadingCache = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionClearPluginsLoadCache, kShortcutDescActionClearPluginsLoadCache, this);
    QObject::connect( _imp->actionClearPluginsLoadingCache, SIGNAL( triggered() ), appPTR, SLOT( clearPluginsLoadedCache() ) );

    _imp->actionClearAllCaches = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionClearAllCaches, kShortcutDescActionClearAllCaches, this);
    QObject::connect( _imp->actionClearAllCaches, SIGNAL( triggered() ), appPTR, SLOT( clearAllCaches() ) );

    _imp->actionShowAboutWindow = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionShowAbout, kShortcutDescActionShowAbout, this);
    _imp->actionShowAboutWindow->setMenuRole(QAction::AboutRole);
    QObject::connect( _imp->actionShowAboutWindow, SIGNAL( triggered() ), this, SLOT( showAbout() ) );

    _imp->renderAllWriters = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionRenderAll, kShortcutDescActionRenderAll, this);
    QObject::connect( _imp->renderAllWriters, SIGNAL( triggered() ), this, SLOT( renderAllWriters() ) );

    _imp->renderSelectedNode = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionRenderSelected, kShortcutDescActionRenderSelected, this);
    QObject::connect( _imp->renderSelectedNode, SIGNAL( triggered() ), this, SLOT( renderSelectedNode() ) );

    _imp->enableRenderStats = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionEnableRenderStats, kShortcutDescActionEnableRenderStats,this);
    _imp->enableRenderStats->setCheckable(true);
    _imp->enableRenderStats->setChecked(false);
    QObject::connect( _imp->enableRenderStats, SIGNAL( triggered() ), this, SLOT(onEnableRenderStatsActionTriggered() ) );

    for (int c = 0; c < NATRON_MAX_RECENT_FILES; ++c) {
        _imp->actionsOpenRecentFile[c] = new QAction(this);
        _imp->actionsOpenRecentFile[c]->setVisible(false);
        connect( _imp->actionsOpenRecentFile[c], SIGNAL( triggered() ), this, SLOT( openRecentFile() ) );
    }

    const char* descs[10] = {
        kShortcutDescActionConnectViewerToInput1,
        kShortcutDescActionConnectViewerToInput2,
        kShortcutDescActionConnectViewerToInput3,
        kShortcutDescActionConnectViewerToInput4,
        kShortcutDescActionConnectViewerToInput5,
        kShortcutDescActionConnectViewerToInput6,
        kShortcutDescActionConnectViewerToInput7,
        kShortcutDescActionConnectViewerToInput8,
        kShortcutDescActionConnectViewerToInput9,
        kShortcutDescActionConnectViewerToInput10
    };
    
    const char* ids[10] = {
        kShortcutIDActionConnectViewerToInput1,
        kShortcutIDActionConnectViewerToInput2,
        kShortcutIDActionConnectViewerToInput3,
        kShortcutIDActionConnectViewerToInput4,
        kShortcutIDActionConnectViewerToInput5,
        kShortcutIDActionConnectViewerToInput6,
        kShortcutIDActionConnectViewerToInput7,
        kShortcutIDActionConnectViewerToInput8,
        kShortcutIDActionConnectViewerToInput9,
        kShortcutIDActionConnectViewerToInput10
    };
    
    
    for (int i = 0; i < 10; ++i) {
        _imp->actionConnectInput[i] = new ActionWithShortcut(kShortcutGroupGlobal, ids[i], descs[i], this);
        _imp->actionConnectInput[i]->setData(i);
        _imp->actionConnectInput[i]->setShortcutContext(Qt::WidgetShortcut);
        QObject::connect( _imp->actionConnectInput[i], SIGNAL( triggered() ), this, SLOT( connectInput() ) );
    }

    _imp->actionImportLayout = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionImportLayout, kShortcutDescActionImportLayout, this);
    QObject::connect( _imp->actionImportLayout, SIGNAL( triggered() ), this, SLOT( importLayout() ) );

    _imp->actionExportLayout = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionExportLayout, kShortcutDescActionExportLayout, this);
    QObject::connect( _imp->actionExportLayout, SIGNAL( triggered() ), this, SLOT( exportLayout() ) );

    _imp->actionRestoreDefaultLayout = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionDefaultLayout, kShortcutDescActionDefaultLayout, this);
    QObject::connect( _imp->actionRestoreDefaultLayout, SIGNAL( triggered() ), this, SLOT( restoreDefaultLayout() ) );

    _imp->actionPrevTab = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionPrevTab, kShortcutDescActionPrevTab, this);
    QObject::connect( _imp->actionPrevTab, SIGNAL( triggered() ), this, SLOT( onPrevTabTriggered() ) );
    _imp->actionNextTab = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionNextTab, kShortcutDescActionNextTab, this);
    QObject::connect( _imp->actionNextTab, SIGNAL( triggered() ), this, SLOT( onNextTabTriggered() ) );
    _imp->actionCloseTab = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionCloseTab, kShortcutDescActionCloseTab, this);
    QObject::connect( _imp->actionCloseTab, SIGNAL( triggered() ), this, SLOT( onCloseTabTriggered() ) );

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
    _imp->viewersMenu->addAction( _imp->viewersViewMenu->menuAction() );
    for (int i = 0; i < 10; ++i) {
        _imp->viewerInputsMenu->addAction(_imp->actionConnectInput[i]);
    }
    
    _imp->menuDisplay->addAction(_imp->actionProject_settings);
    _imp->menuDisplay->addAction(_imp->actionShowOfxLog);
    _imp->menuDisplay->addAction(_imp->actionShortcutEditor);
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
    _imp->actionHelpWebsite = new QAction(this);
    _imp->actionHelpWebsite->setText(QObject::tr("Website"));
    _imp->menuHelp->addAction(_imp->actionHelpWebsite);
    QObject::connect( _imp->actionHelpWebsite, SIGNAL( triggered() ), this, SLOT( openHelpWebsite() ) );

    _imp->actionHelpForum = new QAction(this);
    _imp->actionHelpForum->setText(QObject::tr("Forum"));
    _imp->menuHelp->addAction(_imp->actionHelpForum);
    QObject::connect( _imp->actionHelpForum, SIGNAL( triggered() ), this, SLOT( openHelpForum() ) );

    _imp->actionHelpIssues = new QAction(this);
    _imp->actionHelpIssues->setText(QObject::tr("Issues"));
    _imp->menuHelp->addAction(_imp->actionHelpIssues);
    QObject::connect( _imp->actionHelpIssues, SIGNAL( triggered() ), this, SLOT( openHelpIssues() ) );

    _imp->actionHelpWiki = new QAction(this);
    _imp->actionHelpWiki->setText(QObject::tr("Wiki"));
    _imp->menuHelp->addAction(_imp->actionHelpWiki);
    QObject::connect( _imp->actionHelpWiki, SIGNAL( triggered() ), this, SLOT( openHelpWiki() ) );

    _imp->actionHelpPython = new QAction(this);
    _imp->actionHelpPython->setText(QObject::tr("Python API"));
    _imp->menuHelp->addAction(_imp->actionHelpPython);
    QObject::connect( _imp->actionHelpPython, SIGNAL( triggered() ), this, SLOT( openHelpPython() ) );

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
    QDesktopServices::openUrl(QUrl(NATRON_WEBSITE_URL));
}

void
Gui::openHelpForum()
{
    QDesktopServices::openUrl(QUrl(NATRON_FORUM_URL));
}

void
Gui::openHelpIssues()
{
    QDesktopServices::openUrl(QUrl(NATRON_ISSUE_TRACKER_URL));
}

void
Gui::openHelpWiki()
{
    QDesktopServices::openUrl(QUrl(NATRON_WIKI_URL));
}

void
Gui::openHelpPython()
{
    QDesktopServices::openUrl(QUrl(NATRON_PYTHON_URL));
}

NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_Gui.cpp"
