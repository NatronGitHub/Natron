//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "Gui.h"

#include <cassert>
#include <fstream>
#include <algorithm> // min, max

#include "Global/Macros.h"

#include <QtCore/QTextStream>
#include <QtCore/QSettings>
#include <QtCore/QDebug>
#include <QtCore/QThread>
#include <QtCore/QTimer>

CLANG_DIAG_OFF(unused-private-field)
// /opt/local/include/QtGui/qmime.h:119:10: warning: private field 'type' is not used [-Wunused-private-field]
#include <QCloseEvent>
CLANG_DIAG_ON(unused-private-field)
#include <QHBoxLayout>
#include <QApplication> // qApp
#include <QCheckBox>
#include <QDesktopWidget>
#include <QScrollBar>
#include <QMenuBar>
#include <QProgressDialog>
#include <QTextEdit>
#include <QUndoGroup>

#include <cairo/cairo.h>

#include <boost/version.hpp>

#include "Engine/GroupOutput.h"
#include "Engine/Image.h"
#include "Engine/Lut.h" // floatToInt
#include "Engine/Node.h"
#include "Engine/Project.h"
#include "Engine/Settings.h"
#include "Engine/ViewerInstance.h"

#include "Gui/AboutWindow.h"
#include "Gui/ActionShortcuts.h"
#include "Gui/AutoHideToolBar.h"
#include "Gui/CurveEditor.h"
#include "Gui/DockablePanel.h"
#include "Gui/DopeSheetEditor.h"
#include "Gui/FloatingWidget.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/GuiPrivate.h"
#include "Gui/Histogram.h"
#include "Gui/Menu.h"
#include "Gui/MessageBox.h"
#include "Gui/NodeGraph.h"
#include "Gui/NodeGui.h"
#include "Gui/NodeSettingsPanel.h"
#include "Gui/PreferencesPanel.h"
#include "Gui/ProjectGui.h"
#include "Gui/ProjectGuiSerialization.h" // PaneLayout
#include "Gui/PropertiesBinWrapper.h"
#include "Gui/RegisteredTabs.h"
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

#define PLUGIN_GROUP_DEFAULT_ICON_PATH NATRON_IMAGES_PATH "GroupingIcons/Set" NATRON_ICON_SET_NUMBER "/other_grouping_" NATRON_ICON_SET_NUMBER ".png"


using namespace Natron;


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
    : QMainWindow(parent)
    , SerializableWindow()
    , _imp( new GuiPrivate(app, this) )

{
    QObject::connect( this, SIGNAL( doDialog(int, QString, QString, bool, Natron::StandardButtons, int) ), this,
                      SLOT( onDoDialog(int, QString, QString, bool, Natron::StandardButtons, int) ) );
    QObject::connect( this, SIGNAL( doDialogWithStopAskingCheckbox(int, QString, QString, bool, Natron::StandardButtons, int) ), this,
                      SLOT( onDoDialogWithStopAskingCheckbox(int, QString, QString, bool, Natron::StandardButtons, int) ) );
    QObject::connect( app, SIGNAL( pluginsPopulated() ), this, SLOT( addToolButttonsToToolBar() ) );
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
Gui::closeInstance()
{
    if ( getApp()->getProject()->hasNodes() ) {
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
    abortProject(true);

    return true;
}

void
Gui::closeProject()
{
    closeInstance();

    ///When closing a project we can remove the ViewerCache from memory and put it on disk
    ///since we're not sure it will be used right away
    appPTR->clearPlaybackCache();
    //_imp->_appInstance->getProject()->createViewer();
    //_imp->_appInstance->execOnProjectCreatedCallback();
}

void
Gui::abortProject(bool quitApp)
{
    _imp->setUndoRedoActions(0,0);
    if (quitApp) {
        ///don't show dialogs when about to close, otherwise we could enter in a deadlock situation
        {
            QMutexLocker l(&_imp->aboutToCloseMutex);
            _imp->_aboutToClose = true;
        }

        assert(_imp->_appInstance);

        _imp->_appInstance->getProject()->closeProject(true);
        _imp->notifyGuiClosing();
        _imp->_appInstance->quit();
    } else {
        {
            QMutexLocker l(&_imp->aboutToCloseMutex);
            _imp->_aboutToClose = true;
        }
        _imp->_appInstance->resetPreviewProvider();
        _imp->_appInstance->getProject()->closeProject(false);
        centerAllNodeGraphsWithTimer();
        restoreDefaultLayout();
        {
            QMutexLocker l(&_imp->aboutToCloseMutex);
            _imp->_aboutToClose = false;
        }
    }

    ///Reset current undo/reso actions
    _imp->_currentUndoAction = 0;
    _imp->_currentRedoAction = 0;
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
        if ( !closeInstance() ) {
            e->ignore();

            return;
        }
        e->accept();
    }
}

boost::shared_ptr<NodeGui>
Gui::createNodeGUI( boost::shared_ptr<Node> node,
                    bool requestedByLoad,
                    double xPosHint,
                    double yPosHint,
                    bool pushUndoRedoCommand,
                    bool autoConnect)
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
    boost::shared_ptr<NodeGui> nodeGui = graph->createNodeGUI(node, requestedByLoad,
                                                              xPosHint, yPosHint, pushUndoRedoCommand, autoConnect);
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
    _imp->menuOptions = new Menu(QObject::tr("Options"), _imp->menubar);
    _imp->menuRender = new Menu(QObject::tr("Render"), _imp->menubar);
    _imp->viewersMenu = new Menu(QObject::tr("Viewer(s)"), _imp->menuDisplay);
    _imp->viewerInputsMenu = new Menu(QObject::tr("Connect Current Viewer"), _imp->viewersMenu);
    _imp->viewersViewMenu = new Menu(QObject::tr("Display View Number"), _imp->viewersMenu);
    _imp->cacheMenu = new Menu(QObject::tr("Cache"), _imp->menubar);


    _imp->actionNew_project = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionNewProject, kShortcutDescActionNewProject, this);
    _imp->actionNew_project->setIcon( get_icon("document-new") );
    QObject::connect( _imp->actionNew_project, SIGNAL( triggered() ), this, SLOT( newProject() ) );

    _imp->actionOpen_project = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionOpenProject, kShortcutDescActionOpenProject, this);
    _imp->actionOpen_project->setIcon( get_icon("document-open") );
    QObject::connect( _imp->actionOpen_project, SIGNAL( triggered() ), this, SLOT( openProject() ) );

    _imp->actionClose_project = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionCloseProject, kShortcutDescActionCloseProject, this);
    _imp->actionClose_project->setIcon( get_icon("document-close") );
    QObject::connect( _imp->actionClose_project, SIGNAL( triggered() ), this, SLOT( closeProject() ) );

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
    QObject::connect( _imp->actionExit, SIGNAL( triggered() ), appPTR, SLOT( exitApp() ) );

    _imp->actionProject_settings = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionProjectSettings, kShortcutDescActionProjectSettings, this);
    _imp->actionProject_settings->setIcon( get_icon("document-properties") );
    QObject::connect( _imp->actionProject_settings, SIGNAL( triggered() ), this, SLOT( setVisibleProjectSettingsPanel() ) );

    _imp->actionShowOfxLog = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionShowOFXLog, kShortcutDescActionShowOFXLog, this);
    QObject::connect( _imp->actionShowOfxLog, SIGNAL( triggered() ), this, SLOT( showOfxLog() ) );

    _imp->actionShortcutEditor = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionShowShortcutEditor, kShortcutDescActionShowShortcutEditor, this);
    QObject::connect( _imp->actionShortcutEditor, SIGNAL( triggered() ), this, SLOT( showShortcutEditor() ) );

    _imp->actionNewViewer = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionNewViewer, kShortcutDescActionNewViewer, this);
    QObject::connect( _imp->actionNewViewer, SIGNAL( triggered() ), this, SLOT( createNewViewer() ) );

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


    for (int c = 0; c < NATRON_MAX_RECENT_FILES; ++c) {
        _imp->actionsOpenRecentFile[c] = new QAction(this);
        _imp->actionsOpenRecentFile[c]->setVisible(false);
        connect( _imp->actionsOpenRecentFile[c], SIGNAL( triggered() ), this, SLOT( openRecentFile() ) );
    }

    _imp->actionConnectInput1 = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput1, kShortcutDescActionConnectViewerToInput1, this);
    QObject::connect( _imp->actionConnectInput1, SIGNAL( triggered() ), this, SLOT( connectInput1() ) );

    _imp->actionConnectInput2 = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput2, kShortcutDescActionConnectViewerToInput2, this);
    QObject::connect( _imp->actionConnectInput2, SIGNAL( triggered() ), this, SLOT( connectInput2() ) );

    _imp->actionConnectInput3 = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput3, kShortcutDescActionConnectViewerToInput3, this);
    QObject::connect( _imp->actionConnectInput3, SIGNAL( triggered() ), this, SLOT( connectInput3() ) );

    _imp->actionConnectInput4 = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput4, kShortcutDescActionConnectViewerToInput4, this);
    QObject::connect( _imp->actionConnectInput4, SIGNAL( triggered() ), this, SLOT( connectInput4() ) );

    _imp->actionConnectInput5 = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput5, kShortcutDescActionConnectViewerToInput5, this);
    QObject::connect( _imp->actionConnectInput5, SIGNAL( triggered() ), this, SLOT( connectInput5() ) );


    _imp->actionConnectInput6 = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput6, kShortcutDescActionConnectViewerToInput6, this);
    QObject::connect( _imp->actionConnectInput6, SIGNAL( triggered() ), this, SLOT( connectInput6() ) );

    _imp->actionConnectInput7 = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput7, kShortcutDescActionConnectViewerToInput7, this);
    QObject::connect( _imp->actionConnectInput7, SIGNAL( triggered() ), this, SLOT( connectInput7() ) );

    _imp->actionConnectInput8 = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput8, kShortcutDescActionConnectViewerToInput8, this);
    QObject::connect( _imp->actionConnectInput8, SIGNAL( triggered() ), this, SLOT( connectInput8() ) );

    _imp->actionConnectInput9 = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput9, kShortcutDescActionConnectViewerToInput9, this);

    _imp->actionConnectInput10 = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput10, kShortcutDescActionConnectViewerToInput10, this);
    QObject::connect( _imp->actionConnectInput9, SIGNAL( triggered() ), this, SLOT( connectInput9() ) );
    QObject::connect( _imp->actionConnectInput10, SIGNAL( triggered() ), this, SLOT( connectInput10() ) );

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
    _imp->menubar->addAction( _imp->menuOptions->menuAction() );
    _imp->menubar->addAction( _imp->menuRender->menuAction() );
    _imp->menubar->addAction( _imp->cacheMenu->menuAction() );
    _imp->menuFile->addAction(_imp->actionShowAboutWindow);
    _imp->menuFile->addAction(_imp->actionNew_project);
    _imp->menuFile->addAction(_imp->actionOpen_project);
    _imp->menuFile->addAction( _imp->menuRecentFiles->menuAction() );
    updateRecentFileActions();
    for (int c = 0; c < NATRON_MAX_RECENT_FILES; ++c) {
        _imp->menuRecentFiles->addAction(_imp->actionsOpenRecentFile[c]);
    }

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

    _imp->menuOptions->addAction(_imp->actionProject_settings);
    _imp->menuOptions->addAction(_imp->actionShowOfxLog);
    _imp->menuOptions->addAction(_imp->actionShortcutEditor);
    _imp->menuDisplay->addAction(_imp->actionNewViewer);
    _imp->menuDisplay->addAction( _imp->viewersMenu->menuAction() );
    _imp->viewersMenu->addAction( _imp->viewerInputsMenu->menuAction() );
    _imp->viewersMenu->addAction( _imp->viewersViewMenu->menuAction() );
    _imp->viewerInputsMenu->addAction(_imp->actionConnectInput1);
    _imp->viewerInputsMenu->addAction(_imp->actionConnectInput2);
    _imp->viewerInputsMenu->addAction(_imp->actionConnectInput3);
    _imp->viewerInputsMenu->addAction(_imp->actionConnectInput4);
    _imp->viewerInputsMenu->addAction(_imp->actionConnectInput5);
    _imp->viewerInputsMenu->addAction(_imp->actionConnectInput6);
    _imp->viewerInputsMenu->addAction(_imp->actionConnectInput7);
    _imp->viewerInputsMenu->addAction(_imp->actionConnectInput8);
    _imp->viewerInputsMenu->addAction(_imp->actionConnectInput9);
    _imp->viewerInputsMenu->addAction(_imp->actionConnectInput10);
    _imp->menuDisplay->addSeparator();
    _imp->menuDisplay->addAction(_imp->actionFullScreen);

    _imp->menuRender->addAction(_imp->renderAllWriters);
    _imp->menuRender->addAction(_imp->renderSelectedNode);

    _imp->cacheMenu->addAction(_imp->actionClearDiskCache);
    _imp->cacheMenu->addAction(_imp->actionClearPlayBackCache);
    _imp->cacheMenu->addAction(_imp->actionClearNodeCache);
    _imp->cacheMenu->addAction(_imp->actionClearAllCaches);
    _imp->cacheMenu->addSeparator();
    _imp->cacheMenu->addAction(_imp->actionClearPluginsLoadingCache);

    ///Create custom menu
    const std::list<PythonUserCommand> & commands = appPTR->getUserPythonCommands();
    for (std::list<PythonUserCommand>::const_iterator it = commands.begin(); it != commands.end(); ++it) {
        addMenuEntry(it->grouping, it->pythonFunction, it->key, it->modifiers);
    }
} // createMenuActions

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
    _imp->_toolBox->setOrientation(Qt::Vertical);
    _imp->_toolBox->setMaximumWidth(40);

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
    QObject::connect( _imp->_appInstance->getProject().get(), SIGNAL( projectNameChanged(QString) ), this, SLOT( onProjectNameChanged(QString) ) );


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
    _imp->_curveEditor->getCurveWidget()->updateGL();

    {
        QMutexLocker k (&_imp->_histogramsMutex);
        for (std::list<Histogram*>::iterator it = _imp->_histograms.begin(); it != _imp->_histograms.end(); ++it) {
            (*it)->updateGL();
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

    for (std::list<TabWidget*>::iterator it = panesCpy.begin(); it != panesCpy.end(); ++it) {
        ///Conserve tabs by removing them from the tab widgets. This way they will not be deleted.
        while ( (*it)->count() > 0 ) {
            (*it)->removeTab(0, false);
        }
        (*it)->setParent(NULL);
        delete *it;
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
            (*it)->setParent(NULL);
            delete *it;
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

void
Gui::createDefaultLayout1()
{
    ///First tab widget must be created this way
    TabWidget* mainPane = new TabWidget(this, _imp->_leftRightSplitter);
    {
        QMutexLocker l(&_imp->_panesMutex);
        _imp->_panes.push_back(mainPane);
    }

    mainPane->setObjectName_mt_safe("pane1");
    mainPane->setAsAnchor(true);

    _imp->_leftRightSplitter->addWidget(mainPane);

    QList<int> sizes;
    sizes << _imp->_toolBox->sizeHint().width() << width();
    _imp->_leftRightSplitter->setSizes_mt_safe(sizes);


    TabWidget* propertiesPane = mainPane->splitHorizontally(false);
    TabWidget* workshopPane = mainPane->splitVertically(false);
    Splitter* propertiesSplitter = dynamic_cast<Splitter*>( propertiesPane->parentWidget() );
    assert(propertiesSplitter);
    sizes.clear();
    sizes << width() * 0.65 << width() * 0.35;
    propertiesSplitter->setSizes_mt_safe(sizes);

    TabWidget::moveTab(_imp->_nodeGraphArea, _imp->_nodeGraphArea, workshopPane);
    TabWidget::moveTab(_imp->_curveEditor, _imp->_curveEditor, workshopPane);
    TabWidget::moveTab(_imp->_dopeSheetEditor, _imp->_dopeSheetEditor, workshopPane);
    TabWidget::moveTab(_imp->_propertiesBin, _imp->_propertiesBin, propertiesPane);

    {
        QMutexLocker l(&_imp->_viewerTabsMutex);
        for (std::list<ViewerTab*>::iterator it2 = _imp->_viewerTabs.begin(); it2 != _imp->_viewerTabs.end(); ++it2) {
            TabWidget::moveTab(*it2, *it2, mainPane);
        }
    }
    {
        QMutexLocker l(&_imp->_histogramsMutex);
        for (std::list<Histogram*>::iterator it2 = _imp->_histograms.begin(); it2 != _imp->_histograms.end(); ++it2) {
            TabWidget::moveTab(*it2, *it2, mainPane);
        }
    }


    ///Default to NodeGraph displayed
    workshopPane->makeCurrentTab(0);
}

static void
restoreTabWidget(TabWidget* pane,
                 const PaneLayout & serialization)
{
    ///Find out if the name is already used
    QString availableName = pane->getGui()->getAvailablePaneName( serialization.name.c_str() );

    pane->setObjectName_mt_safe(availableName);
    pane->setAsAnchor(serialization.isAnchor);
    const RegisteredTabs & tabs = pane->getGui()->getRegisteredTabs();
    for (std::list<std::string>::const_iterator it = serialization.tabs.begin(); it != serialization.tabs.end(); ++it) {
        RegisteredTabs::const_iterator found = tabs.find(*it);

        ///If the tab exists in the current project, move it
        if ( found != tabs.end() ) {
            TabWidget::moveTab(found->second.first, found->second.second, pane);
        }
    }
    pane->makeCurrentTab(serialization.currentIndex);
}

static void
restoreSplitterRecursive(Gui* gui,
                         Splitter* splitter,
                         const SplitterSerialization & serialization)
{
    Qt::Orientation qO;
    Natron::OrientationEnum nO = (Natron::OrientationEnum)serialization.orientation;

    switch (nO) {
    case Natron::eOrientationHorizontal:
        qO = Qt::Horizontal;
        break;
    case Natron::eOrientationVertical:
        qO = Qt::Vertical;
        break;
    default:
        throw std::runtime_error("Unrecognized splitter orientation");
        break;
    }
    splitter->setOrientation(qO);

    if (serialization.children.size() != 2) {
        throw std::runtime_error("Splitter has a child count that is not 2");
    }

    for (std::vector<SplitterSerialization::Child*>::const_iterator it = serialization.children.begin();
         it != serialization.children.end(); ++it) {
        if ( (*it)->child_asSplitter ) {
            Splitter* child = new Splitter(splitter);
            splitter->addWidget_mt_safe(child);
            restoreSplitterRecursive( gui, child, *( (*it)->child_asSplitter ) );
        } else {
            assert( (*it)->child_asPane );
            TabWidget* pane = new TabWidget(gui, splitter);
            gui->registerPane(pane);
            splitter->addWidget_mt_safe(pane);
            restoreTabWidget( pane, *( (*it)->child_asPane ) );
        }
    }

    splitter->restoreNatron( serialization.sizes.c_str() );
}

void
Gui::restoreLayout(bool wipePrevious,
                   bool enableOldProjectCompatibility,
                   const GuiLayoutSerialization & layoutSerialization)
{
    ///Wipe the current layout
    if (wipePrevious) {
        wipeLayout();
    }

    ///For older projects prior to the layout change, just set default layout.
    if (enableOldProjectCompatibility) {
        createDefaultLayout1();
    } else {
        std::list<ApplicationWindowSerialization*> floatingDockablePanels;

        QDesktopWidget* desktop = QApplication::desktop();
        QRect screen = desktop->screenGeometry();
        
        ///now restore the gui layout
        for (std::list<ApplicationWindowSerialization*>::const_iterator it = layoutSerialization._windows.begin();
             it != layoutSerialization._windows.end(); ++it) {
            QWidget* mainWidget = 0;

            ///The window contains only a pane (for the main window it also contains the toolbar)
            if ( (*it)->child_asPane ) {
                TabWidget* centralWidget = new TabWidget(this);
                registerPane(centralWidget);
                restoreTabWidget(centralWidget, *(*it)->child_asPane);
                mainWidget = centralWidget;
            }
            ///The window contains a splitter as central widget
            else if ( (*it)->child_asSplitter ) {
                Splitter* centralWidget = new Splitter(this);
                restoreSplitterRecursive(this, centralWidget, *(*it)->child_asSplitter);
                mainWidget = centralWidget;
            }
            ///The child is a dockable panel, restore it later
            else if ( !(*it)->child_asDockablePanel.empty() ) {
                assert(!(*it)->isMainWindow);
                floatingDockablePanels.push_back(*it);
                continue;
            }

            assert(mainWidget);
            QWidget* window;
            if ( (*it)->isMainWindow ) {
                // mainWidget->setParent(_imp->_leftRightSplitter);
                _imp->_leftRightSplitter->addWidget_mt_safe(mainWidget);
                window = this;
            } else {
                FloatingWidget* floatingWindow = new FloatingWidget(this, this);
                floatingWindow->setWidget(mainWidget);
                registerFloatingWindow(floatingWindow);
                window = floatingWindow;
            }

            ///Restore geometry
            window->resize(std::min((*it)->w,screen.width()), std::min((*it)->h,screen.height()));
            window->move( QPoint( (*it)->x, (*it)->y ) );
        }

        for (std::list<ApplicationWindowSerialization*>::iterator it = floatingDockablePanels.begin();
             it != floatingDockablePanels.end(); ++it) {
            ///Find the node associated to the floating panel if any and float it
            assert( !(*it)->child_asDockablePanel.empty() );
            if ( (*it)->child_asDockablePanel == kNatronProjectSettingsPanelSerializationName ) {
                _imp->_projectGui->getPanel()->floatPanel();
            } else {
                ///Find a node with the dockable panel name
                const std::list<boost::shared_ptr<NodeGui> > & nodes = getNodeGraph()->getAllActiveNodes();
                DockablePanel* panel = 0;
                for (std::list<boost::shared_ptr<NodeGui> >::const_iterator it2 = nodes.begin(); it2 != nodes.end(); ++it2) {
                    if ( (*it2)->getNode()->getScriptName() == (*it)->child_asDockablePanel ) {
                        ( (*it2)->getSettingPanel()->floatPanel() );
                        panel = (*it2)->getSettingPanel();
                        break;
                    }
                }
                if (panel) {
                    FloatingWidget* fWindow = dynamic_cast<FloatingWidget*>( panel->parentWidget() );
                    assert(fWindow);
                    fWindow->move( QPoint( (*it)->x, (*it)->y ) );
                    fWindow->resize(std::min((*it)->w,screen.width()), std::min((*it)->h,screen.height()));
                }
            }
        }
    }
} // restoreLayout

void
Gui::exportLayout()
{
    std::vector<std::string> filters;

    filters.push_back(NATRON_LAYOUT_FILE_EXT);
    SequenceFileDialog dialog( this, filters, false, SequenceFileDialog::eFileDialogModeSave, _imp->_lastSaveProjectOpenedDir.toStdString(), this, false );
    if ( dialog.exec() ) {
        std::string filename = dialog.filesToSave();
        QString filenameCpy( filename.c_str() );
        QString ext = Natron::removeFileExtension(filenameCpy);
        if (ext != NATRON_LAYOUT_FILE_EXT) {
            filename.append("." NATRON_LAYOUT_FILE_EXT);
        }

        std::ofstream ofile;
        try {
            ofile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
            ofile.open(filename.c_str(), std::ofstream::out);
        } catch (const std::ofstream::failure & e) {
            Natron::errorDialog( tr("Error").toStdString()
                                 , tr("Exception occured when opening file").toStdString(), false );

            return;
        }

        if ( !ofile.good() ) {
            Natron::errorDialog( tr("Error").toStdString()
                                 , tr("Failure to open the file").toStdString(), false );

            return;
        }

        try {
            boost::archive::xml_oarchive oArchive(ofile);
            GuiLayoutSerialization s;
            s.initialize(this);
            oArchive << boost::serialization::make_nvp("Layout", s);
        }catch (...) {
            Natron::errorDialog( tr("Error").toStdString()
                                 , tr("Failure when saving the layout").toStdString(), false );
            ofile.close();

            return;
        }

        ofile.close();
    }
}

const QString &
Gui::getLastLoadProjectDirectory() const
{
    return _imp->_lastLoadProjectOpenedDir;
}

const QString &
Gui::getLastSaveProjectDirectory() const
{
    return _imp->_lastSaveProjectOpenedDir;
}

const QString &
Gui::getLastPluginDirectory() const
{
    return _imp->_lastPluginDir;
}

void
Gui::updateLastPluginDirectory(const QString & str)
{
    _imp->_lastPluginDir = str;
}

void
Gui::importLayout()
{
    std::vector<std::string> filters;

    filters.push_back(NATRON_LAYOUT_FILE_EXT);
    SequenceFileDialog dialog( this, filters, false, SequenceFileDialog::eFileDialogModeOpen, _imp->_lastLoadProjectOpenedDir.toStdString(), this, false );
    if ( dialog.exec() ) {
        std::string filename = dialog.selectedFiles();
        std::ifstream ifile;
        try {
            ifile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
            ifile.open(filename.c_str(), std::ifstream::in);
        } catch (const std::ifstream::failure & e) {
            QString err = QString("Exception occured when opening file %1: %2").arg( filename.c_str() ).arg( e.what() );
            Natron::errorDialog( tr("Error").toStdString(), tr( err.toStdString().c_str() ).toStdString(), false );

            return;
        }

        try {
            boost::archive::xml_iarchive iArchive(ifile);
            GuiLayoutSerialization s;
            iArchive >> boost::serialization::make_nvp("Layout", s);
            restoreLayout(true, false, s);
        } catch (const boost::archive::archive_exception & e) {
            ifile.close();
            QString err = QString("Exception occured when opening file %1: %2").arg( filename.c_str() ).arg( e.what() );
            Natron::errorDialog( tr("Error").toStdString(), tr( err.toStdString().c_str() ).toStdString(), false );

            return;
        } catch (const std::exception & e) {
            ifile.close();
            QString err = QString("Exception occured when opening file %1: %2").arg( filename.c_str() ).arg( e.what() );
            Natron::errorDialog( tr("Error").toStdString(), tr( err.toStdString().c_str() ).toStdString(), false );

            return;
        }

        ifile.close();
    }
}

void
Gui::createDefaultLayoutInternal(bool wipePrevious)
{
    if (wipePrevious) {
        wipeLayout();
    }

    std::string fileLayout = appPTR->getCurrentSettings()->getDefaultLayoutFile();
    if ( !fileLayout.empty() ) {
        std::ifstream ifile;
        ifile.open( fileLayout.c_str() );
        if ( !ifile.is_open() ) {
            createDefaultLayout1();
        } else {
            try {
                boost::archive::xml_iarchive iArchive(ifile);
                GuiLayoutSerialization s;
                iArchive >> boost::serialization::make_nvp("Layout", s);
                restoreLayout(false, false, s);
            } catch (const boost::archive::archive_exception & e) {
                ifile.close();
                QString err = QString("Exception occured when opening file %1: %2").arg( fileLayout.c_str() ).arg( e.what() );
                Natron::errorDialog( tr("Error").toStdString(), tr( err.toStdString().c_str() ).toStdString(), false );

                return;
            } catch (const std::exception & e) {
                ifile.close();
                QString err = QString("Exception occured when opening file %1: %2").arg( fileLayout.c_str() ).arg( e.what() );
                Natron::errorDialog( tr("Error").toStdString(), tr( err.toStdString().c_str() ).toStdString(), false );

                return;
            }

            ifile.close();
        }
    } else {
        createDefaultLayout1();
    }
}

void
Gui::restoreDefaultLayout()
{
    createDefaultLayoutInternal(true);
}

void
Gui::initProjectGuiKnobs()
{
    assert(_imp->_projectGui);
    _imp->_projectGui->initializeKnobsGui();
}

QKeySequence
Gui::keySequenceForView(int v)
{
    switch (v) {
    case 0:

        return QKeySequence(Qt::CTRL + Qt::ALT +  Qt::Key_1);
        break;
    case 1:

        return QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_2);
        break;
    case 2:

        return QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_3);
        break;
    case 3:

        return QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_4);
        break;
    case 4:

        return QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_5);
        break;
    case 5:

        return QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_6);
        break;
    case 6:

        return QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_7);
        break;
    case 7:

        return QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_8);
        break;
    case 8:

        return QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_9);
        break;
    case 9:

        return QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_0);
        break;
    default:

        return QKeySequence();
    }
}

static const char*
slotForView(int view)
{
    switch (view) {
    case 0:

        return SLOT( showView0() );
        break;
    case 1:

        return SLOT( showView1() );
        break;
    case 2:

        return SLOT( showView2() );
        break;
    case 3:

        return SLOT( showView3() );
        break;
    case 4:

        return SLOT( showView4() );
        break;
    case 5:

        return SLOT( showView5() );
        break;
    case 6:

        return SLOT( showView6() );
        break;
    case 7:

        return SLOT( showView7() );
        break;
    case 8:

        return SLOT( showView7() );
        break;
    case 9:

        return SLOT( showView8() );
        break;
    default:

        return NULL;
    }
}

void
Gui::updateViewsActions(int viewsCount)
{
    _imp->viewersViewMenu->clear();
    //if viewsCount == 1 we don't add a menu entry
    _imp->viewersMenu->removeAction( _imp->viewersViewMenu->menuAction() );
    if (viewsCount == 2) {
        QAction* left = new QAction(this);
        left->setCheckable(false);
        left->setShortcut( QKeySequence(Qt::CTRL + Qt::Key_1) );
        _imp->viewersViewMenu->addAction(left);
        left->setText( tr("Display Left View") );
        QObject::connect( left, SIGNAL( triggered() ), this, SLOT( showView0() ) );
        QAction* right = new QAction(this);
        right->setCheckable(false);
        right->setShortcut( QKeySequence(Qt::CTRL + Qt::Key_2) );
        _imp->viewersViewMenu->addAction(right);
        right->setText( tr("Display Right View") );
        QObject::connect( right, SIGNAL( triggered() ), this, SLOT( showView1() ) );

        _imp->viewersMenu->addAction( _imp->viewersViewMenu->menuAction() );
    } else if (viewsCount > 2) {
        for (int i = 0; i < viewsCount; ++i) {
            if (i > 9) {
                break;
            }
            QAction* viewI = new QAction(this);
            viewI->setCheckable(false);
            QKeySequence seq = keySequenceForView(i);
            if ( !seq.isEmpty() ) {
                viewI->setShortcut(seq);
            }
            _imp->viewersViewMenu->addAction(viewI);
            const char* slot = slotForView(i);
            viewI->setText( QString( tr("Display View ") ) + QString::number(i + 1) );
            if (slot) {
                QObject::connect(viewI, SIGNAL( triggered() ), this, slot);
            }
        }

        _imp->viewersMenu->addAction( _imp->viewersViewMenu->menuAction() );
    }
}

void
Gui::putSettingsPanelFirst(DockablePanel* panel)
{
    _imp->_layoutPropertiesBin->removeWidget(panel);
    _imp->_layoutPropertiesBin->insertWidget(0, panel);
    _imp->_propertiesScrollArea->verticalScrollBar()->setValue(0);
    buildTabFocusOrderPropertiesBin();
}

void
Gui::buildTabFocusOrderPropertiesBin()
{
    int next = 1;

    for (int i = 0; i < _imp->_layoutPropertiesBin->count(); ++i, ++next) {
        QLayoutItem* item = _imp->_layoutPropertiesBin->itemAt(i);
        QWidget* w = item->widget();
        QWidget* nextWidget = _imp->_layoutPropertiesBin->itemAt(next < _imp->_layoutPropertiesBin->count() ? next : 0)->widget();

        if (w && nextWidget) {
            setTabOrder(w, nextWidget);
        }
    }
}

void
Gui::setVisibleProjectSettingsPanel()
{
    addVisibleDockablePanel( _imp->_projectGui->getPanel() );
    if ( !_imp->_projectGui->isVisible() ) {
        _imp->_projectGui->setVisible(true);
    }
}
