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
