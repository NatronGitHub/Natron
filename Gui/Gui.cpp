//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#include "Gui/Gui.h"

#include <cassert>
#include <fstream>

#include <QtCore/QTextStream>
#include <QWaitCondition>
#include <QMutex>
#include <QTextDocument> // for Qt::convertFromPlainText
#include <QCoreApplication>
#include <QAction>
#include <QSettings>
#include <QDebug>
#include <QThread>

#if QT_VERSION >= 0x050000
#include <QScreen>
#endif
#include <QUndoGroup>
CLANG_DIAG_OFF(unused-private-field)
// /opt/local/include/QtGui/qmime.h:119:10: warning: private field 'type' is not used [-Wunused-private-field]
#include <QCloseEvent>
CLANG_DIAG_ON(unused-private-field)
#include <QHBoxLayout>
#include <QGraphicsScene>
#include <QMenu>
#include <QApplication>
#include <QMenuBar>
#include <QDesktopWidget>
#include <QToolBar>
#include <QKeySequence>
#include <QScrollArea>
#include <QScrollBar>
#include <QToolButton>
#include <QMessageBox>
#include <QImage>
#include <QProgressDialog>

#include <cairo/cairo.h>

#include <boost/version.hpp>
CLANG_DIAG_OFF(unused-parameter)
// /opt/local/include/boost/serialization/smart_cast.hpp:254:25: warning: unused parameter 'u' [-Wunused-parameter]
#include <boost/archive/xml_iarchive.hpp>
CLANG_DIAG_ON(unused-parameter)
#include <boost/archive/xml_oarchive.hpp>

#include "Engine/ViewerInstance.h"
#include "Engine/Project.h"
#include "Engine/Plugin.h"
#include "Engine/Settings.h"
#include "Engine/KnobFile.h"
#include "SequenceParsing.h"
#include "Engine/ProcessHandler.h"
#include "Engine/Lut.h"
#include "Engine/Image.h"
#include "Engine/Node.h"
#include "Engine/KnobSerialization.h"
#include "Engine/OutputSchedulerThread.h"

#include "Gui/GuiApplicationManager.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/NodeGraph.h"
#include "Gui/CurveEditor.h"
#include "Gui/PreferencesPanel.h"
#include "Gui/AboutWindow.h"
#include "Gui/ProjectGui.h"
#include "Gui/ToolButton.h"
#include "Gui/ViewerTab.h"
#include "Gui/ViewerGL.h"
#include "Gui/TabWidget.h"
#include "Gui/DockablePanel.h"
#include "Gui/SequenceFileDialog.h"
#include "Gui/FromQtEnums.h"
#include "Gui/RenderingProgressDialog.h"
#include "Gui/NodeGui.h"
#include "Gui/Histogram.h"
#include "Gui/Splitter.h"
#include "Gui/SpinBox.h"
#include "Gui/Button.h"
#include "Gui/RotoGui.h"
#include "Gui/ProjectGuiSerialization.h"
#include "Gui/ActionShortcuts.h"
#include "Gui/ShortCutEditor.h"
#include "Gui/NodeBackDrop.h"

#define kViewerPaneName "ViewerPane"
#define kPropertiesBinName "Properties"

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


struct GuiPrivate
{
    Gui* _gui; //< ptr to the public interface
    bool _isUserScrubbingTimeline; //< true if the user is actively moving the cursor on the timeline. False on mouse release.
    ViewerTab* _lastSelectedViewer; //< a ptr to the last selected ViewerTab
    GuiAppInstance* _appInstance; //< ptr to the appInstance

    ///Dialogs handling members
    QWaitCondition _uiUsingMainThreadCond; //< used with _uiUsingMainThread
    bool _uiUsingMainThread; //< true when the Gui is showing a dialog in the main thread
    mutable QMutex _uiUsingMainThreadMutex; //< protects _uiUsingMainThread
    Natron::StandardButtonEnum _lastQuestionDialogAnswer; //< stores the last question answer

    ///ptrs to the undo/redo actions from the active stack.
    QAction* _currentUndoAction;
    QAction* _currentRedoAction;

    ///all the undo stacks of Natron are gathered here
    QUndoGroup* _undoStacksGroup;
    std::map<QUndoStack*,std::pair<QAction*,QAction*> > _undoStacksActions;

    ///all the splitters used to separate the "panes" of the application
    mutable QMutex _splittersMutex;
    std::list<Splitter*> _splitters;


    ///all the menu actions
    ActionWithShortcut *actionNew_project;
    ActionWithShortcut *actionOpen_project;
    ActionWithShortcut *actionClose_project;
    ActionWithShortcut *actionSave_project;
    ActionWithShortcut *actionSaveAs_project;
    ActionWithShortcut *actionPreferences;
    ActionWithShortcut *actionExit;
    ActionWithShortcut *actionProject_settings;
    ActionWithShortcut *actionShowOfxLog;
    ActionWithShortcut *actionShortcutEditor;
    ActionWithShortcut *actionNewViewer;
    ActionWithShortcut *actionFullScreen;
    ActionWithShortcut *actionClearDiskCache;
    ActionWithShortcut *actionClearPlayBackCache;
    ActionWithShortcut *actionClearNodeCache;
    ActionWithShortcut *actionClearPluginsLoadingCache;
    ActionWithShortcut *actionClearAllCaches;
    ActionWithShortcut *actionShowAboutWindow;
    QAction *actionsOpenRecentFile[NATRON_MAX_RECENT_FILES];
    ActionWithShortcut *renderAllWriters;
    ActionWithShortcut *renderSelectedNode;
    ActionWithShortcut* actionConnectInput1;
    ActionWithShortcut* actionConnectInput2;
    ActionWithShortcut* actionConnectInput3;
    ActionWithShortcut* actionConnectInput4;
    ActionWithShortcut* actionConnectInput5;
    ActionWithShortcut* actionConnectInput6;
    ActionWithShortcut* actionConnectInput7;
    ActionWithShortcut* actionConnectInput8;
    ActionWithShortcut* actionConnectInput9;
    ActionWithShortcut* actionConnectInput10;
    ActionWithShortcut* actionImportLayout;
    ActionWithShortcut* actionExportLayout;
    ActionWithShortcut* actionRestoreDefaultLayout;

    ///the main "central" widget
    QWidget *_centralWidget;
    QHBoxLayout* _mainLayout; //< its layout

    ///strings that remember for project save/load and writers/reader dialog where
    ///the user was the last time.
    QString _lastLoadSequenceOpenedDir;
    QString _lastLoadProjectOpenedDir;
    QString _lastSaveSequenceOpenedDir;
    QString _lastSaveProjectOpenedDir;

    // this one is a ptr to others TabWidget.
    //It tells where to put the viewer when making a new one
    // If null it places it on default tab widget
    TabWidget* _nextViewerTabPlace;

    ///the splitter separating the gui and the left toolbar
    Splitter* _leftRightSplitter;

    ///a list of ptrs to all the viewer tabs.
    mutable QMutex _viewerTabsMutex;
    std::list<ViewerTab*> _viewerTabs;

    ///a list of ptrs to all histograms
    mutable QMutex _histogramsMutex;
    std::list<Histogram*> _histograms;
    int _nextHistogramIndex; //< for giving a unique name to histogram tabs

    ///The scene managing elements of the node graph.
    QGraphicsScene* _graphScene;

    ///The node graph (i.e: the view of the _graphScene)
    NodeGraph *_nodeGraphArea;

    ///The curve editor.
    CurveEditor *_curveEditor;

    ///the left toolbar
    QToolBar* _toolBox;

    ///a vector of all the toolbuttons
    std::vector<ToolButton*> _toolButtons;

    ///holds the properties dock
    QWidget *_propertiesBin;
    
    QScrollArea* _propertiesScrollArea;


    ///the vertical layout for the properties dock container.
    QVBoxLayout *_layoutPropertiesBin;
    Button* _clearAllPanelsButton;
    SpinBox* _maxPanelsOpenedSpinBox;
    Button* _freezeUIButton;
    
    QMutex _isGUIFrozenMutex;
    bool _isGUIFrozen;
    
    ///The menu bar and all the menus
    QMenuBar *menubar;
    QMenu *menuFile;
    QMenu *menuRecentFiles;
    QMenu *menuEdit;
    QMenu *menuLayout;
    QMenu *menuDisplay;
    QMenu *menuOptions;
    QMenu *menuRender;
    QMenu *viewersMenu;
    QMenu *viewerInputsMenu;
    QMenu *viewersViewMenu;
    QMenu *cacheMenu;


    ///all TabWidget's : used to know what to hide/show for fullscreen mode
    mutable QMutex _panesMutex;
    std::list<TabWidget*> _panes;
    mutable QMutex _floatingWindowMutex;
    std::list<FloatingWidget*> _floatingWindows;

    ///All the tabs used in the TabWidgets (used for d&d purpose)
    std::map<std::string,QWidget*> _registeredTabs;

    ///The user preferences window
    PreferencesPanel* _settingsGui;

    ///The project Gui (stored in the properties pane)
    ProjectGui* _projectGui;

    ///ptr to the currently dragged tab for d&d purpose.
    QWidget* _currentlyDraggedPanel;

    ///The "About" window.
    AboutWindow* _aboutWindow;
    std::map<Natron::EffectInstance*,QProgressDialog*> _progressBars;

    ///list of the currently opened property panels
    std::list<DockablePanel*> openedPanels;
    QString _openGLVersion;
    QString _glewVersion;
    QToolButton* _toolButtonMenuOpened;
    QMutex aboutToCloseMutex;
    bool _aboutToClose;
    TabWidget* fullScreenWidgetDuringSave;
    ShortCutEditor* shortcutEditor;

    GuiPrivate(GuiAppInstance* app,
               Gui* gui)
        : _gui(gui)
          , _isUserScrubbingTimeline(false)
          , _lastSelectedViewer(NULL)
          , _appInstance(app)
          , _uiUsingMainThreadCond()
          , _uiUsingMainThread(false)
          , _uiUsingMainThreadMutex()
          , _lastQuestionDialogAnswer(Natron::eStandardButtonNo)
          , _currentUndoAction(0)
          , _currentRedoAction(0)
          , _undoStacksGroup(0)
          , _undoStacksActions()
          , _splitters()
          , actionNew_project(0)
          , actionOpen_project(0)
          , actionClose_project(0)
          , actionSave_project(0)
          , actionSaveAs_project(0)
          , actionPreferences(0)
          , actionExit(0)
          , actionProject_settings(0)
          , actionShowOfxLog(0)
          , actionShortcutEditor(0)
          , actionNewViewer(0)
          , actionFullScreen(0)
          , actionClearDiskCache(0)
          , actionClearPlayBackCache(0)
          , actionClearNodeCache(0)
          , actionClearPluginsLoadingCache(0)
          , actionClearAllCaches(0)
          , actionShowAboutWindow(0)
          , actionsOpenRecentFile()
          , renderAllWriters(0)
          , renderSelectedNode(0)
          , actionConnectInput1(0)
          , actionConnectInput2(0)
          , actionConnectInput3(0)
          , actionConnectInput4(0)
          , actionConnectInput5(0)
          , actionConnectInput6(0)
          , actionConnectInput7(0)
          , actionConnectInput8(0)
          , actionConnectInput9(0)
          , actionConnectInput10(0)
          , actionImportLayout(0)
          , actionExportLayout(0)
          , actionRestoreDefaultLayout(0)
          , _centralWidget(0)
          , _mainLayout(0)
          , _lastLoadSequenceOpenedDir()
          , _lastLoadProjectOpenedDir()
          , _lastSaveSequenceOpenedDir()
          , _lastSaveProjectOpenedDir()
          , _nextViewerTabPlace(0)
          , _leftRightSplitter(0)
          , _viewerTabsMutex()
          , _viewerTabs()
          , _histogramsMutex()
          , _histograms()
          , _nextHistogramIndex(1)
          , _graphScene(0)
          , _nodeGraphArea(0)
          , _curveEditor(0)
          , _toolBox(0)
          , _propertiesBin(0)
          , _propertiesScrollArea(0)
          , _layoutPropertiesBin(0)
          , _clearAllPanelsButton(0)
          , _maxPanelsOpenedSpinBox(0)
          , _freezeUIButton(0)
          , _isGUIFrozenMutex()
          , _isGUIFrozen(false)
          , menubar(0)
          , menuFile(0)
          , menuRecentFiles(0)
          , menuEdit(0)
          , menuLayout(0)
          , menuDisplay(0)
          , menuOptions(0)
          , menuRender(0)
          , viewersMenu(0)
          , viewerInputsMenu(0)
          , cacheMenu(0)
          , _panesMutex()
          , _panes()
          , _floatingWindowMutex()
          , _floatingWindows()
          , _settingsGui(0)
          , _projectGui(0)
          , _currentlyDraggedPanel(0)
          , _aboutWindow(0)
          , _progressBars()
          , openedPanels()
          , _openGLVersion()
          , _glewVersion()
          , _toolButtonMenuOpened(NULL)
          , aboutToCloseMutex()
          , _aboutToClose(false)
          , fullScreenWidgetDuringSave(0)
          , shortcutEditor(0)
    {
    }

    void restoreGuiGeometry();

    void saveGuiGeometry();

    void setUndoRedoActions(QAction* undoAction,QAction* redoAction);

    void addToolButton(ToolButton* tool);

    ///Creates the properties bin and appends it as a tab to the propertiesPane TabWidget
    void createPropertiesBinGui();

    void notifyGuiClosing();

    ///Must be called absolutely before createPropertiesBinGui
    void createNodeGraphGui();

    void createCurveEditorGui();
    
    ///If there's only 1 non-floating pane in the main window, return it, otherwise returns NULL
    TabWidget* getOnly1NonFloatingPane(int & count) const;
};

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
      , _imp( new GuiPrivate(app,this) )

{
    QObject::connect( this,SIGNAL( doDialog(int,QString,QString,Natron::StandardButtons,int) ),this,
                      SLOT( onDoDialog(int,QString,QString,Natron::StandardButtons,int) ) );
    QObject::connect( app,SIGNAL( pluginsPopulated() ),this,SLOT( addToolButttonsToToolBar() ) );
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

void
GuiPrivate::notifyGuiClosing()
{
    ///This is to workaround an issue that when destroying a widget it calls the focusOut() handler hence can
    ///cause bad pointer dereference to the Gui object since we're destroying it.
    {
        QMutexLocker k(&_viewerTabsMutex);
        for (std::list<ViewerTab*>::iterator it = _viewerTabs.begin(); it != _viewerTabs.end(); ++it) {
            (*it)->notifyAppClosing();
        }
    }

    const std::list<boost::shared_ptr<NodeGui> > allNodes = _nodeGraphArea->getAllActiveNodes();

    for (std::list<boost::shared_ptr<NodeGui> >::const_iterator it = allNodes.begin(); it != allNodes.end(); ++it) {
        DockablePanel* panel = (*it)->getSettingPanel();
        if (panel) {
            panel->onGuiClosing();
        }
    }
    _nodeGraphArea->discardGuiPointer();

    {
        QMutexLocker k(&_panesMutex);
        for (std::list<TabWidget*>::iterator it = _panes.begin(); it != _panes.end(); ++it) {
            (*it)->discardGuiPointer();
        }
    }
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
    if ( getApp()->getProject()->hasNodes() ) {
        int ret = saveWarning();
        if (ret == 0) {
            if ( !saveProject() ) {
                return;
            }
        } else if (ret == 2) {
            return;
        }
    }
    ///When closing a project we can remove the ViewerCache from memory and put it on disk
    ///since we're not sure it will be used right away
    appPTR->clearPlaybackCache();
    abortProject(false);
}

void
Gui::abortProject(bool quitApp)
{
    if (quitApp) {
        ///don't show dialogs when about to close, otherwise we could enter in a deadlock situation
        {
            QMutexLocker l(&_imp->aboutToCloseMutex);
            _imp->_aboutToClose = true;
        }

        assert(_imp->_appInstance);

        _imp->notifyGuiClosing();
        _imp->_appInstance->getProject()->closeProject();
        _imp->_appInstance->quit();
    } else {
        _imp->_appInstance->getProject()->closeProject();
        restoreDefaultLayout();
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
        Natron::errorDialog("FullScreen", tr("Please select a window first").toStdString());
        return;
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
                    bool pushUndoRedoCommand)
{
    assert(_imp->_nodeGraphArea);
    boost::shared_ptr<NodeGui> nodeGui = _imp->_nodeGraphArea->createNodeGUI(_imp->_layoutPropertiesBin,node,requestedByLoad,
                                                                             xPosHint,yPosHint,pushUndoRedoCommand);
    QObject::connect( nodeGui.get(),SIGNAL( nameChanged(QString) ),this,SLOT( onNodeNameChanged(QString) ) );
    assert(nodeGui);

    return nodeGui;
}

void
Gui::addNodeGuiToCurveEditor(boost::shared_ptr<NodeGui> node)
{
    _imp->_curveEditor->addNode(node);
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
    _imp->_lastSelectedViewer = addNewViewerTab(v, where);
    v->setUiContext( _imp->_lastSelectedViewer->getViewer() );
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

    _imp->menuFile = new QMenu(QObject::tr("File"),_imp->menubar);
    _imp->menuRecentFiles = new QMenu(QObject::tr("Open recent"),_imp->menuFile);
    _imp->menuEdit = new QMenu(QObject::tr("Edit"),_imp->menubar);
    _imp->menuLayout = new QMenu(QObject::tr("Layout"),_imp->menubar);
    _imp->menuDisplay = new QMenu(QObject::tr("Display"),_imp->menubar);
    _imp->menuOptions = new QMenu(QObject::tr("Options"),_imp->menubar);
    _imp->menuRender = new QMenu(QObject::tr("Render"),_imp->menubar);
    _imp->viewersMenu = new QMenu(QObject::tr("Viewer(s)"),_imp->menuDisplay);
    _imp->viewerInputsMenu = new QMenu(QObject::tr("Connect Current Viewer"),_imp->viewersMenu);
    _imp->viewersViewMenu = new QMenu(QObject::tr("Display view number"),_imp->viewersMenu);
    _imp->cacheMenu = new QMenu(QObject::tr("Cache"),_imp->menubar);


    _imp->actionNew_project = new ActionWithShortcut(kShortcutGroupGlobal,kShortcutIDActionNewProject,kShortcutDescActionNewProject,this);
    _imp->actionNew_project->setIcon( get_icon("document-new") );
    QObject::connect( _imp->actionNew_project, SIGNAL( triggered() ), this, SLOT( newProject() ) );

    _imp->actionOpen_project = new ActionWithShortcut(kShortcutGroupGlobal,kShortcutIDActionOpenProject,kShortcutDescActionOpenProject,this);
    _imp->actionOpen_project->setIcon( get_icon("document-open") );
    QObject::connect( _imp->actionOpen_project, SIGNAL( triggered() ), this, SLOT( openProject() ) );

    _imp->actionClose_project = new ActionWithShortcut(kShortcutGroupGlobal,kShortcutIDActionCloseProject,kShortcutDescActionCloseProject,this);
    _imp->actionClose_project->setIcon( get_icon("document-close") );
    QObject::connect( _imp->actionClose_project, SIGNAL( triggered() ), this, SLOT( closeProject() ) );

    _imp->actionSave_project = new ActionWithShortcut(kShortcutGroupGlobal,kShortcutIDActionSaveProject,kShortcutDescActionSaveProject,this);
    _imp->actionSave_project->setIcon( get_icon("document-save") );
    QObject::connect( _imp->actionSave_project, SIGNAL( triggered() ), this, SLOT( saveProject() ) );

    _imp->actionSaveAs_project = new ActionWithShortcut(kShortcutGroupGlobal,kShortcutIDActionSaveAsProject,kShortcutDescActionSaveAsProject,this);
    _imp->actionSaveAs_project->setIcon( get_icon("document-save-as") );
    QObject::connect( _imp->actionSaveAs_project, SIGNAL( triggered() ), this, SLOT( saveProjectAs() ) );

    _imp->actionPreferences = new ActionWithShortcut(kShortcutGroupGlobal,kShortcutIDActionPreferences,kShortcutDescActionPreferences,this);
    _imp->actionPreferences->setMenuRole(QAction::PreferencesRole);
    QObject::connect( _imp->actionPreferences,SIGNAL( triggered() ),this,SLOT( showSettings() ) );

    _imp->actionExit = new ActionWithShortcut(kShortcutGroupGlobal,kShortcutIDActionQuit,kShortcutDescActionQuit,this);
    _imp->actionExit->setMenuRole(QAction::QuitRole);
    _imp->actionExit->setIcon( get_icon("application-exit") );
    QObject::connect( _imp->actionExit,SIGNAL( triggered() ),appPTR,SLOT( exitApp() ) );

    _imp->actionProject_settings = new ActionWithShortcut(kShortcutGroupGlobal,kShortcutIDActionProjectSettings,kShortcutDescActionProjectSettings,this);
    _imp->actionProject_settings->setIcon( get_icon("document-properties") );
    QObject::connect( _imp->actionProject_settings,SIGNAL( triggered() ),this,SLOT( setVisibleProjectSettingsPanel() ) );

    _imp->actionShowOfxLog = new ActionWithShortcut(kShortcutGroupGlobal,kShortcutIDActionShowOFXLog,kShortcutDescActionShowOFXLog,this);
    QObject::connect( _imp->actionShowOfxLog,SIGNAL( triggered() ),this,SLOT( showOfxLog() ) );

    _imp->actionShortcutEditor = new ActionWithShortcut(kShortcutGroupGlobal,kShortcutIDActionShowShortcutEditor,kShortcutDescActionShowShortcutEditor,this);
    QObject::connect( _imp->actionShortcutEditor,SIGNAL( triggered() ),this,SLOT( showShortcutEditor() ) );

    _imp->actionNewViewer = new ActionWithShortcut(kShortcutGroupGlobal,kShortcutIDActionNewViewer,kShortcutDescActionNewViewer,this);
    QObject::connect( _imp->actionNewViewer,SIGNAL( triggered() ),this,SLOT( createNewViewer() ) );

    _imp->actionFullScreen = new ActionWithShortcut(kShortcutGroupGlobal,kShortcutIDActionFullscreen,kShortcutDescActionFullscreen,this);
    _imp->actionFullScreen->setIcon( get_icon("view-fullscreen") );
    _imp->actionFullScreen->setShortcutContext(Qt::ApplicationShortcut);
    QObject::connect( _imp->actionFullScreen, SIGNAL( triggered() ),this,SLOT( toggleFullScreen() ) );

    _imp->actionClearDiskCache = new ActionWithShortcut(kShortcutGroupGlobal,kShortcutIDActionClearDiskCache,kShortcutDescActionClearDiskCache,this);
    QObject::connect( _imp->actionClearDiskCache, SIGNAL( triggered() ),appPTR,SLOT( clearDiskCache() ) );

    _imp->actionClearPlayBackCache = new ActionWithShortcut(kShortcutGroupGlobal,kShortcutIDActionClearPlaybackCache,kShortcutDescActionClearPlaybackCache,this);
    QObject::connect( _imp->actionClearPlayBackCache, SIGNAL( triggered() ),appPTR,SLOT( clearPlaybackCache() ) );

    _imp->actionClearNodeCache = new ActionWithShortcut(kShortcutGroupGlobal,kShortcutIDActionClearNodeCache,kShortcutDescActionClearNodeCache,this);
    QObject::connect( _imp->actionClearNodeCache, SIGNAL( triggered() ),appPTR,SLOT( clearNodeCache() ) );
    QObject::connect( _imp->actionClearNodeCache, SIGNAL( triggered() ),_imp->_appInstance,SLOT( clearOpenFXPluginsCaches() ) );

    _imp->actionClearPluginsLoadingCache = new ActionWithShortcut(kShortcutGroupGlobal,kShortcutIDActionClearPluginsLoadCache,kShortcutDescActionClearPluginsLoadCache,this);
    QObject::connect( _imp->actionClearPluginsLoadingCache, SIGNAL( triggered() ),appPTR,SLOT( clearPluginsLoadedCache() ) );

    _imp->actionClearAllCaches = new ActionWithShortcut(kShortcutGroupGlobal,kShortcutIDActionClearAllCaches,kShortcutDescActionClearAllCaches,this);
    QObject::connect( _imp->actionClearAllCaches, SIGNAL( triggered() ),appPTR,SLOT( clearAllCaches() ) );

    _imp->actionShowAboutWindow = new ActionWithShortcut(kShortcutGroupGlobal,kShortcutIDActionShowAbout,kShortcutDescActionShowAbout,this);
    _imp->actionShowAboutWindow->setMenuRole(QAction::AboutRole);
    QObject::connect( _imp->actionShowAboutWindow,SIGNAL( triggered() ),this,SLOT( showAbout() ) );

    _imp->renderAllWriters = new ActionWithShortcut(kShortcutGroupGlobal,kShortcutIDActionRenderAll,kShortcutDescActionRenderAll,this);
    QObject::connect( _imp->renderAllWriters,SIGNAL( triggered() ),this,SLOT( renderAllWriters() ) );

    _imp->renderSelectedNode = new ActionWithShortcut(kShortcutGroupGlobal,kShortcutIDActionRenderSelected,kShortcutDescActionRenderSelected,this);
    QObject::connect( _imp->renderSelectedNode,SIGNAL( triggered() ),this,SLOT( renderSelectedNode() ) );


    for (int c = 0; c < NATRON_MAX_RECENT_FILES; ++c) {
        _imp->actionsOpenRecentFile[c] = new QAction(this);
        _imp->actionsOpenRecentFile[c]->setVisible(false);
        connect( _imp->actionsOpenRecentFile[c], SIGNAL( triggered() ),this, SLOT( openRecentFile() ) );
    }

    _imp->actionConnectInput1 = new ActionWithShortcut(kShortcutGroupGlobal,kShortcutIDActionConnectViewerToInput1,kShortcutDescActionConnectViewerToInput1,this);
    QObject::connect( _imp->actionConnectInput1, SIGNAL( triggered() ),this,SLOT( connectInput1() ) );

    _imp->actionConnectInput2 = new ActionWithShortcut(kShortcutGroupGlobal,kShortcutIDActionConnectViewerToInput2,kShortcutDescActionConnectViewerToInput2,this);
    QObject::connect( _imp->actionConnectInput2, SIGNAL( triggered() ),this,SLOT( connectInput2() ) );

    _imp->actionConnectInput3 = new ActionWithShortcut(kShortcutGroupGlobal,kShortcutIDActionConnectViewerToInput3,kShortcutDescActionConnectViewerToInput3,this);
    QObject::connect( _imp->actionConnectInput3, SIGNAL( triggered() ),this,SLOT( connectInput3() ) );

    _imp->actionConnectInput4 = new ActionWithShortcut(kShortcutGroupGlobal,kShortcutIDActionConnectViewerToInput4,kShortcutDescActionConnectViewerToInput4,this);
    QObject::connect( _imp->actionConnectInput4, SIGNAL( triggered() ),this,SLOT( connectInput4() ) );

    _imp->actionConnectInput5 = new ActionWithShortcut(kShortcutGroupGlobal,kShortcutIDActionConnectViewerToInput5,kShortcutDescActionConnectViewerToInput5,this);
    QObject::connect( _imp->actionConnectInput5, SIGNAL( triggered() ),this,SLOT( connectInput5() ) );


    _imp->actionConnectInput6 = new ActionWithShortcut(kShortcutGroupGlobal,kShortcutIDActionConnectViewerToInput6,kShortcutDescActionConnectViewerToInput6,this);
    QObject::connect( _imp->actionConnectInput6, SIGNAL( triggered() ),this,SLOT( connectInput6() ) );

    _imp->actionConnectInput7 = new ActionWithShortcut(kShortcutGroupGlobal,kShortcutIDActionConnectViewerToInput7,kShortcutDescActionConnectViewerToInput7,this);
    QObject::connect( _imp->actionConnectInput7, SIGNAL( triggered() ),this,SLOT( connectInput7() ) );

    _imp->actionConnectInput8 = new ActionWithShortcut(kShortcutGroupGlobal,kShortcutIDActionConnectViewerToInput8,kShortcutDescActionConnectViewerToInput8,this);
    QObject::connect( _imp->actionConnectInput8, SIGNAL( triggered() ),this,SLOT( connectInput8() ) );

    _imp->actionConnectInput9 = new ActionWithShortcut(kShortcutGroupGlobal,kShortcutIDActionConnectViewerToInput9,kShortcutDescActionConnectViewerToInput9,this);

    _imp->actionConnectInput10 = new ActionWithShortcut(kShortcutGroupGlobal,kShortcutIDActionConnectViewerToInput10,kShortcutDescActionConnectViewerToInput10,this);
    QObject::connect( _imp->actionConnectInput9, SIGNAL( triggered() ),this,SLOT( connectInput9() ) );
    QObject::connect( _imp->actionConnectInput10, SIGNAL( triggered() ),this,SLOT( connectInput10() ) );

    _imp->actionImportLayout = new ActionWithShortcut(kShortcutGroupGlobal,kShortcutIDActionImportLayout,kShortcutDescActionImportLayout,this);
    QObject::connect( _imp->actionImportLayout, SIGNAL( triggered() ),this,SLOT( importLayout() ) );

    _imp->actionExportLayout = new ActionWithShortcut(kShortcutGroupGlobal,kShortcutIDActionExportLayout,kShortcutDescActionExportLayout,this);
    QObject::connect( _imp->actionExportLayout, SIGNAL( triggered() ),this,SLOT( exportLayout() ) );

    _imp->actionRestoreDefaultLayout = new ActionWithShortcut(kShortcutGroupGlobal,kShortcutIDActionDefaultLayout,kShortcutDescActionDefaultLayout,this);
    QObject::connect( _imp->actionRestoreDefaultLayout, SIGNAL( triggered() ),this,SLOT( restoreDefaultLayout() ) );

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
    _imp->menuFile->addSeparator();
    _imp->menuFile->addAction(_imp->actionExit);

    _imp->menuEdit->addAction(_imp->actionPreferences);

    _imp->menuLayout->addAction(_imp->actionImportLayout);
    _imp->menuLayout->addAction(_imp->actionExportLayout);
    _imp->menuLayout->addAction(_imp->actionRestoreDefaultLayout);

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
    _imp->_leftRightSplitter->setObjectName(kMainSplitterObjectName);
    _imp->_splitters.push_back(_imp->_leftRightSplitter);
    _imp->_leftRightSplitter->setOrientation(Qt::Horizontal);
    _imp->_leftRightSplitter->setContentsMargins(0, 0, 0, 0);


    _imp->_toolBox = new QToolBar(_imp->_leftRightSplitter);
    _imp->_toolBox->setOrientation(Qt::Vertical);
    _imp->_toolBox->setMaximumWidth(40);

    _imp->_leftRightSplitter->addWidget(_imp->_toolBox);

    _imp->_mainLayout->addWidget(_imp->_leftRightSplitter);

    _imp->createNodeGraphGui();
    _imp->createCurveEditorGui();
    ///Must be absolutely called once _nodeGraphArea has been initialized.
    _imp->createPropertiesBinGui();

    createDefaultLayoutInternal(false);

    _imp->_projectGui = new ProjectGui(this);
    _imp->_projectGui->create(_imp->_appInstance->getProject(),
                              _imp->_layoutPropertiesBin,
                              this);

    initProjectGuiKnobs();

    _imp->_settingsGui = new PreferencesPanel(appPTR->getCurrentSettings(),this);
    _imp->_settingsGui->hide();

    setVisibleProjectSettingsPanel();

    _imp->_aboutWindow = new AboutWindow(this,this);
    _imp->_aboutWindow->hide();

    _imp->shortcutEditor = new ShortCutEditor(this);
    _imp->shortcutEditor->hide();


    //the same action also clears the ofx plugins caches, they are not the same cache but are used to the same end
    QObject::connect( _imp->_appInstance->getProject().get(),SIGNAL( projectNameChanged(QString) ),this,SLOT( onProjectNameChanged(QString) ) );


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
GuiPrivate::createPropertiesBinGui()
{
    _propertiesBin = new QWidget(_gui);
    _propertiesBin->setObjectName(kPropertiesBinName);

    QVBoxLayout* mainPropertiesLayout = new QVBoxLayout(_propertiesBin);
    mainPropertiesLayout->setContentsMargins(0, 0, 0, 0);
    mainPropertiesLayout->setSpacing(0);
    
    _propertiesScrollArea = new QScrollArea(_propertiesBin);
    _propertiesScrollArea->setObjectName("PropertiesBinScrollArea");
    assert(_nodeGraphArea);

    QWidget* propertiesContainer = new QWidget(_propertiesScrollArea);
    propertiesContainer->setObjectName("_propertiesContainer");
    _layoutPropertiesBin = new QVBoxLayout(propertiesContainer);
    _layoutPropertiesBin->setSpacing(0);
    _layoutPropertiesBin->setContentsMargins(0, 0, 0, 0);
    propertiesContainer->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Fixed);
    _propertiesScrollArea->setWidget(propertiesContainer);
    _propertiesScrollArea->setWidgetResizable(true);

    QWidget* propertiesAreaButtonsContainer = new QWidget(propertiesContainer);
    QHBoxLayout* propertiesAreaButtonsLayout = new QHBoxLayout(propertiesAreaButtonsContainer);
    propertiesAreaButtonsLayout->setContentsMargins(0, 0, 0, 0);
    propertiesAreaButtonsLayout->setSpacing(5);
    QPixmap closePanelPix;
    appPTR->getIcon(NATRON_PIXMAP_CLOSE_PANEL, &closePanelPix);
    _clearAllPanelsButton = new Button(QIcon(closePanelPix),"",propertiesAreaButtonsContainer);
    _clearAllPanelsButton->setFixedSize(NATRON_SMALL_BUTTON_SIZE,NATRON_SMALL_BUTTON_SIZE);
    _clearAllPanelsButton->setToolTip( Qt::convertFromPlainText(_gui->tr("Clears all the panels in the properties bin pane."),
                                                                Qt::WhiteSpaceNormal) );
    QObject::connect( _clearAllPanelsButton,SIGNAL( clicked(bool) ),_gui,SLOT( clearAllVisiblePanels() ) );


    _maxPanelsOpenedSpinBox = new SpinBox(propertiesAreaButtonsContainer);
    _maxPanelsOpenedSpinBox->setMaximumSize(NATRON_SMALL_BUTTON_SIZE,NATRON_SMALL_BUTTON_SIZE);
    _maxPanelsOpenedSpinBox->setMinimum(0);
    _maxPanelsOpenedSpinBox->setMaximum(100);
    _maxPanelsOpenedSpinBox->setToolTip( Qt::convertFromPlainText(_gui->tr("Set the maximum of panels that can be opened at the same time "
                                                                           "in the properties bin pane. The special value of 0 indicates "
                                                                           "that an unlimited number of panels can be opened."),
                                                                  Qt::WhiteSpaceNormal) );
    _maxPanelsOpenedSpinBox->setValue( appPTR->getCurrentSettings()->getMaxPanelsOpened() );
    QObject::connect( _maxPanelsOpenedSpinBox,SIGNAL( valueChanged(double) ),_gui,SLOT( onMaxPanelsSpinBoxValueChanged(double) ) );

    QPixmap pixFreezeEnabled,pixFreezeDisabled;
    appPTR->getIcon(Natron::NATRON_PIXMAP_FREEZE_ENABLED,&pixFreezeEnabled);
    appPTR->getIcon(Natron::NATRON_PIXMAP_FREEZE_DISABLED,&pixFreezeDisabled);
    QIcon icFreeze;
    icFreeze.addPixmap(pixFreezeEnabled,QIcon::Normal,QIcon::On);
    icFreeze.addPixmap(pixFreezeDisabled,QIcon::Normal,QIcon::Off);
    _freezeUIButton = new Button(icFreeze,"",propertiesAreaButtonsContainer);
    _freezeUIButton->setCheckable(true);
    _freezeUIButton->setChecked(false);
    _freezeUIButton->setDown(false);
    _freezeUIButton->setFixedSize(NATRON_SMALL_BUTTON_SIZE,NATRON_SMALL_BUTTON_SIZE);
    _freezeUIButton->setToolTip("<p><b>" + _gui->tr("Turbo mode:") + "</p></b><p>" + _gui->tr("When checked, everything besides the viewer will not be refreshed in the user interface "
                                         "for maximum efficiency during playback.") + "</p>");
    QObject::connect( _freezeUIButton, SIGNAL (clicked(bool)), _gui, SLOT(onFreezeUIButtonClicked(bool) ) );
    
    propertiesAreaButtonsLayout->addWidget(_maxPanelsOpenedSpinBox);
    propertiesAreaButtonsLayout->addWidget(_clearAllPanelsButton);
    propertiesAreaButtonsLayout->addWidget(_freezeUIButton);
    propertiesAreaButtonsLayout->addStretch();
    
    mainPropertiesLayout->addWidget(propertiesAreaButtonsContainer);
    mainPropertiesLayout->addWidget(_propertiesScrollArea);

    _gui->registerTab(_propertiesBin);
} // createPropertiesBinGui

void
GuiPrivate::createNodeGraphGui()
{
    _graphScene = new QGraphicsScene(_gui);
    _graphScene->setItemIndexMethod(QGraphicsScene::NoIndex);
    _nodeGraphArea = new NodeGraph(_gui,_graphScene,_gui);
    _nodeGraphArea->setObjectName(kNodeGraphObjectName);
    _gui->registerTab(_nodeGraphArea);
}

void
GuiPrivate::createCurveEditorGui()
{
    _curveEditor = new CurveEditor(_gui,_appInstance->getTimeLine(),_gui);
    _curveEditor->setObjectName(kCurveEditorObjectName);
    _gui->registerTab(_curveEditor);
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
            (*it)->removeTab(0);
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
    TabWidget* mainPane = new TabWidget(this,_imp->_leftRightSplitter);

    mainPane->setObjectName(kViewerPaneName);
    mainPane->setAsAnchor(true);
    {
        QMutexLocker l(&_imp->_panesMutex);
        _imp->_panes.push_back(mainPane);
    }
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

    TabWidget::moveTab(_imp->_nodeGraphArea, workshopPane);
    TabWidget::moveTab(_imp->_curveEditor,workshopPane);
    TabWidget::moveTab(_imp->_propertiesBin,propertiesPane);

    {
        QMutexLocker l(&_imp->_viewerTabsMutex);
        for (std::list<ViewerTab*>::iterator it2 = _imp->_viewerTabs.begin(); it2 != _imp->_viewerTabs.end(); ++it2) {
            TabWidget::moveTab(*it2,mainPane);
        }
    }
    {
        QMutexLocker l(&_imp->_histogramsMutex);
        for (std::list<Histogram*>::iterator it2 = _imp->_histograms.begin(); it2 != _imp->_histograms.end(); ++it2) {
            TabWidget::moveTab(*it2,mainPane);
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
    const std::map<std::string,QWidget*> & tabs = pane->getGui()->getRegisteredTabs();
    for (std::list<std::string>::const_iterator it = serialization.tabs.begin(); it != serialization.tabs.end(); ++it) {
        std::map<std::string,QWidget*>::const_iterator found = tabs.find(*it);

        ///If the tab exists in the current project, move it
        if ( found != tabs.end() ) {
            TabWidget::moveTab(found->second, pane);
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
            restoreSplitterRecursive( gui,child, *( (*it)->child_asSplitter ) );
        } else {
            assert( (*it)->child_asPane );
            TabWidget* pane = new TabWidget(gui,splitter);
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
                restoreSplitterRecursive(this,centralWidget, *(*it)->child_asSplitter);
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
                mainWidget->setParent(_imp->_leftRightSplitter);
                _imp->_leftRightSplitter->addWidget_mt_safe(mainWidget);
                window = this;
            } else {
                FloatingWidget* floatingWindow = new FloatingWidget(this,this);
                floatingWindow->setWidget(mainWidget);
                registerFloatingWindow(floatingWindow);
                window = floatingWindow;
            }

            ///Restore geometry
            window->resize( (*it)->w, (*it)->h );
            window->move( QPoint( (*it)->x,(*it)->y ) );
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
                    if ( (*it2)->getNode()->getName_mt_safe() == (*it)->child_asDockablePanel ) {
                        ( (*it2)->getSettingPanel()->floatPanel() );
                        panel = (*it2)->getSettingPanel();
                        break;
                    }
                }

                if (!panel) {
                    ///try with backdrops setting panels
                    const std::list<NodeBackDrop*> backdrops = getNodeGraph()->getActiveBackDrops();
                    for (std::list<NodeBackDrop*>::const_iterator it2 = backdrops.begin(); it2 != backdrops.end(); ++it2) {
                        if ( (*it2)->getName_mt_safe() == (*it)->child_asDockablePanel ) {
                            ( (*it2)->getSettingsPanel()->floatPanel() );
                            panel = (*it2)->getSettingsPanel();
                            break;
                        }
                    }
                }
                if (panel) {
                    FloatingWidget* fWindow = dynamic_cast<FloatingWidget*>( panel->parentWidget() );
                    assert(fWindow);
                    fWindow->move( QPoint( (*it)->x,(*it)->y ) );
                    fWindow->resize( (*it)->w,(*it)->h );
                }
            }
        }
    }
} // restoreLayout

void
Gui::exportLayout()
{
    std::vector<std::string> filters;

    filters.push_back("." NATRON_LAYOUT_FILE_EXT);
    SequenceFileDialog dialog( this,filters,false,SequenceFileDialog::SAVE_DIALOG,_imp->_lastSaveProjectOpenedDir.toStdString(),this,false );
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
            ofile.open(filename.c_str(),std::ofstream::out);
        } catch (const std::ofstream::failure & e) {
            Natron::errorDialog( tr("Error").toStdString()
                                 , tr("Exception occured when opening file").toStdString() );

            return;
        }

        if ( !ofile.good() ) {
            Natron::errorDialog( tr("Error").toStdString()
                                 , tr("Failure to open the file").toStdString() );

            return;
        }

        try {
            boost::archive::xml_oarchive oArchive(ofile);
            GuiLayoutSerialization s;
            s.initialize(this);
            oArchive << boost::serialization::make_nvp("Layout",s);
        }catch (...) {
            Natron::errorDialog( tr("Error").toStdString()
                                 , tr("Failure when saving the layout").toStdString() );
            ofile.close();

            return;
        }

        ofile.close();
    }
}

const QString&
Gui::getLastLoadProjectDirectory() const
{
    return _imp->_lastLoadProjectOpenedDir;
}

const QString&
Gui::getLastSaveProjectDirectory() const
{
    return _imp->_lastSaveProjectOpenedDir;
}

void
Gui::importLayout()
{
    std::vector<std::string> filters;

    filters.push_back("." NATRON_LAYOUT_FILE_EXT);
    SequenceFileDialog dialog( this,filters,false,SequenceFileDialog::OPEN_DIALOG,_imp->_lastLoadProjectOpenedDir.toStdString(),this,false );
    if ( dialog.exec() ) {
        std::string filename = dialog.selectedFiles();
        std::ifstream ifile;
        try {
            ifile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
            ifile.open(filename.c_str(),std::ifstream::in);
        } catch (const std::ifstream::failure & e) {
            QString err = QString("Exception occured when opening file %1: %2").arg( filename.c_str() ).arg( e.what() );
            Natron::errorDialog( tr("Error").toStdString(),tr( err.toStdString().c_str() ).toStdString() );

            return;
        }

        try {
            boost::archive::xml_iarchive iArchive(ifile);
            GuiLayoutSerialization s;
            iArchive >> boost::serialization::make_nvp("Layout", s);
            restoreLayout(true,false, s);
        } catch (const boost::archive::archive_exception & e) {
            ifile.close();
            QString err = QString("Exception occured when opening file %1: %2").arg( filename.c_str() ).arg( e.what() );
            Natron::errorDialog( tr("Error").toStdString(),tr( err.toStdString().c_str() ).toStdString() );

            return;
        } catch (const std::exception & e) {
            ifile.close();
            QString err = QString("Exception occured when opening file %1: %2").arg( filename.c_str() ).arg( e.what() );
            Natron::errorDialog( tr("Error").toStdString(),tr( err.toStdString().c_str() ).toStdString() );

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
                restoreLayout(false,false, s);
            } catch (const boost::archive::archive_exception & e) {
                ifile.close();
                QString err = QString("Exception occured when opening file %1: %2").arg( fileLayout.c_str() ).arg( e.what() );
                Natron::errorDialog( tr("Error").toStdString(),tr( err.toStdString().c_str() ).toStdString() );

                return;
            } catch (const std::exception & e) {
                ifile.close();
                QString err = QString("Exception occured when opening file %1: %2").arg( fileLayout.c_str() ).arg( e.what() );
                Natron::errorDialog( tr("Error").toStdString(),tr( err.toStdString().c_str() ).toStdString() );

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
        left->setText( tr("Display left view") );
        QObject::connect( left,SIGNAL( triggered() ),this,SLOT( showView0() ) );
        QAction* right = new QAction(this);
        right->setCheckable(false);
        right->setShortcut( QKeySequence(Qt::CTRL + Qt::Key_2) );
        _imp->viewersViewMenu->addAction(right);
        right->setText( tr("Display right view") );
        QObject::connect( right,SIGNAL( triggered() ),this,SLOT( showView1() ) );

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
            viewI->setText( QString( tr("Display view ") ) + QString::number(i + 1) );
            if (slot) {
                QObject::connect(viewI,SIGNAL( triggered() ),this,slot);
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
}

void
Gui::setVisibleProjectSettingsPanel()
{
    addVisibleDockablePanel( _imp->_projectGui->getPanel() );
    if ( !_imp->_projectGui->isVisible() ) {
        _imp->_projectGui->setVisible(true);
    }
}

void
Gui::loadStyleSheet()
{
    QFile qss(":/Resources/Stylesheets/mainstyle.qss");

    if ( qss.open(QIODevice::ReadOnly
                  | QIODevice::Text) ) {
        QTextStream in(&qss);
        QString content( in.readAll() );
        setStyleSheet( content
                       .arg("rgb(243,149,0)") // %1: selection-color
                       .arg("rgb(50,50,50)") // %2: medium background
                       .arg("rgb(71,71,71)") // %3: soft background
                       .arg("rgb(38,38,38)") // %4: strong background
                       .arg("rgb(200,200,200)") // %5: text colour
                       .arg("rgb(86,117,156)") // %6: interpolated value color
                       .arg("rgb(21,97,248)") // %7: keyframe value color
                       .arg("rgb(0,0,0)") ); // %8: disabled editable text
    }
}

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
                QString tabName = what->tabAt(i)->objectName();
                if ( (tabName == kNodeGraphObjectName) || (tabName == kCurveEditorObjectName) ) {
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
    std::map<NodeGui*,RotoGui*> rotoNodes;
    std::list<NodeGui*> rotoNodesList;
    std::pair<NodeGui*,RotoGui*> currentRoto;
    std::map<NodeGui*,TrackerGui*> trackerNodes;
    std::list<NodeGui*> trackerNodesList;
    std::pair<NodeGui*,TrackerGui*> currentTracker;

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
            } else if ( (*it)->getNode()->isTrackerNode() ) {
                trackerNodesList.push_back( it->get() );
                if (!currentTracker.first) {
                    currentTracker.first = it->get();
                }
            }
        }
    }
    for (std::map<NodeGui*,RotoGui*>::iterator it = rotoNodes.begin(); it != rotoNodes.end(); ++it) {
        rotoNodesList.push_back(it->first);
    }

    for (std::map<NodeGui*,TrackerGui*>::iterator it = trackerNodes.begin(); it != trackerNodes.end(); ++it) {
        trackerNodesList.push_back(it->first);
    }

    ViewerTab* tab = new ViewerTab(rotoNodesList,currentRoto.first,trackerNodesList,currentTracker.first,this,viewer,where);
    QObject::connect( tab->getViewer(),SIGNAL( imageChanged(int) ),this,SLOT( onViewerImageChanged(int) ) );
    {
        QMutexLocker l(&_imp->_viewerTabsMutex);
        _imp->_viewerTabs.push_back(tab);
    }
    where->appendTab(tab);
    emit viewersChanged();

    return tab;
}

void
Gui::onViewerImageChanged(int texIndex)
{
    ///notify all histograms a viewer image changed
    ViewerGL* viewer = qobject_cast<ViewerGL*>( sender() );

    if (viewer) {
        QMutexLocker l(&_imp->_histogramsMutex);
        for (std::list<Histogram*>::iterator it = _imp->_histograms.begin(); it != _imp->_histograms.end(); ++it) {
            (*it)->onViewerImageChanged(viewer,texIndex);
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
    where->appendTab(tab);
    emit viewersChanged();
}

void
Gui::registerTab(QWidget* tab)
{
    std::map<std::string, QWidget*>::iterator registeredTab = _imp->_registeredTabs.find( tab->objectName().toStdString() );

    if ( registeredTab == _imp->_registeredTabs.end() ) {
        _imp->_registeredTabs.insert( std::make_pair(tab->objectName().toStdString(), tab) );
    }
}

void
Gui::unregisterTab(QWidget* tab)
{
    std::map<std::string, QWidget*>::iterator registeredTab = _imp->_registeredTabs.find( tab->objectName().toStdString() );

    if ( registeredTab != _imp->_registeredTabs.end() ) {
        _imp->_registeredTabs.erase(registeredTab);
    }
}

void
Gui::registerFloatingWindow(FloatingWidget* window)
{
    QMutexLocker k(&_imp->_floatingWindowMutex);
    std::list<FloatingWidget*>::iterator found = std::find(_imp->_floatingWindows.begin(),_imp->_floatingWindows.end(),window);

    if ( found == _imp->_floatingWindows.end() ) {
        _imp->_floatingWindows.push_back(window);
    }
}

void
Gui::unregisterFloatingWindow(FloatingWidget* window)
{
    QMutexLocker k(&_imp->_floatingWindowMutex);
    std::list<FloatingWidget*>::iterator found = std::find(_imp->_floatingWindows.begin(),_imp->_floatingWindows.end(),window);

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

    if (!initiatedFromNode) {
        assert(_imp->_nodeGraphArea);
        ///call the deleteNode which will call this function again when the node will be deactivated.
        _imp->_nodeGraphArea->removeNode( _imp->_appInstance->getNodeGui( tab->getInternalNode()->getNode() ) );
    } else {
        tab->hide();


        TabWidget* container = dynamic_cast<TabWidget*>( tab->parentWidget() );
        if (container) {
            container->removeTab(tab);
        }

        if (deleteData) {
            QMutexLocker l(&_imp->_viewerTabsMutex);
            std::list<ViewerTab*>::iterator it = std::find(_imp->_viewerTabs.begin(), _imp->_viewerTabs.end(), tab);
            if ( it != _imp->_viewerTabs.end() ) {
                _imp->_viewerTabs.erase(it);
            }
            delete tab;
        }
    }
    emit viewersChanged();
}

Histogram*
Gui::addNewHistogram()
{
    Histogram* h = new Histogram(this);
    QMutexLocker l(&_imp->_histogramsMutex);

    h->setObjectName( "Histogram " + QString::number(_imp->_nextHistogramIndex) );
    ++_imp->_nextHistogramIndex;
    _imp->_histograms.push_back(h);

    return h;
}

void
Gui::removeHistogram(Histogram* h)
{
    unregisterTab(h);
    QMutexLocker l(&_imp->_histogramsMutex);
    std::list<Histogram*>::iterator it = std::find(_imp->_histograms.begin(),_imp->_histograms.end(),h);

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

TabWidget*
GuiPrivate::getOnly1NonFloatingPane(int & count) const
{
    assert(!_panesMutex.tryLock());
    count = 0;
    if (_panes.empty()) {
        return NULL;
    }
    TabWidget* firstNonFloating = 0;
    for (std::list<TabWidget*>::const_iterator it = _panes.begin() ;it!= _panes.end(); ++it) {
        if (!(*it)->isFloatingWindowChild()) {
            if (!firstNonFloating) {
                firstNonFloating = *it;
            }
            ++count;
        }
    }
    ///there should always be at least 1 non floating window
    assert(firstNonFloating);
    return firstNonFloating;
}

void
Gui::unregisterPane(TabWidget* pane)
{
    {
        QMutexLocker l(&_imp->_panesMutex);
        std::list<TabWidget*>::iterator found = std::find(_imp->_panes.begin(), _imp->_panes.end(), pane);
        
        if ( found != _imp->_panes.end() ) {
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

QWidget*
Gui::findExistingTab(const std::string & name) const
{
    std::map<std::string,QWidget*>::const_iterator it = _imp->_registeredTabs.find(name);

    if ( it != _imp->_registeredTabs.end() ) {
        return it->second;
    } else {
        return NULL;
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

ToolButton*
Gui::findOrCreateToolButton(PluginGroupNode* plugin)
{
    for (U32 i = 0; i < _imp->_toolButtons.size(); ++i) {
        if ( _imp->_toolButtons[i]->getID() == plugin->getID() ) {
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
            PluginGroupNode* othersToolButton = appPTR->findPluginToolButtonOrCreate(PLUGIN_GROUP_DEFAULT,PLUGIN_GROUP_DEFAULT, PLUGIN_GROUP_DEFAULT_ICON_PATH);
            othersToolButton->tryAddChild(plugin);

            //if the othersGroup doesn't exist, create it
            if (!othersGroup) {
                othersGroup = findOrCreateToolButton(othersToolButton);
            }
            parentToolButton = othersGroup;
        }
    }
    ToolButton* pluginsToolButton = new ToolButton(_imp->_appInstance,plugin,plugin->getID(),plugin->getLabel(),icon);

    if (isLeaf) {
        QString label = pluginsToolButton->getLabel();
        int foundOFX = label.lastIndexOf("OFX");
        if (foundOFX != -1) {
            label = label.remove(foundOFX, 3);
        }
        assert(parentToolButton);
        QAction* action = new QAction(this);
        action->setText(label);
        action->setIcon( pluginsToolButton->getIcon() );
        QObject::connect( action, SIGNAL( triggered() ), pluginsToolButton, SLOT( onTriggered() ) );
        pluginsToolButton->setAction(action);
    } else {
        QMenu* menu = new QMenu(this);
        menu->setFont( QFont(NATRON_FONT,NATRON_FONT_SIZE_11) );
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
        QObject::connect( createReaderAction,SIGNAL( triggered() ),this,SLOT( createReader() ) );
        createReaderAction->setText( tr("Read") );
        QPixmap readImagePix;
        appPTR->getIcon(Natron::NATRON_PIXMAP_READ_IMAGE, &readImagePix);
        createReaderAction->setIcon( QIcon(readImagePix) );
        createReaderAction->setShortcutContext(Qt::WidgetShortcut);
        createReaderAction->setShortcut( QKeySequence(Qt::Key_R) );
        imageMenu->addAction(createReaderAction);

        QAction* createWriterAction = new QAction(imageMenu);
        QObject::connect( createWriterAction,SIGNAL( triggered() ),this,SLOT( createWriter() ) );
        createWriterAction->setText( tr("Write") );
        QPixmap writeImagePix;
        appPTR->getIcon(Natron::NATRON_PIXMAP_WRITE_IMAGE, &writeImagePix);
        createWriterAction->setIcon( QIcon(writeImagePix) );
        createReaderAction->setShortcutContext(Qt::WidgetShortcut);
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
    namedToolButtons.insert( namedToolButtons.end(), otherToolButtons.begin(),otherToolButtons.end() );

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

namespace {
class AutoRaiseToolButton
    : public QToolButton
{
    Gui* _gui;
    bool _menuOpened;

public:

    AutoRaiseToolButton(Gui* gui,
                        QWidget* parent)
        : QToolButton(parent)
          , _gui(gui)
          , _menuOpened(false)
    {
        setMouseTracking(true);
    }

private:

    virtual void mousePressEvent(QMouseEvent* e) OVERRIDE FINAL
    {
        _menuOpened = !_menuOpened;
        if (_menuOpened) {
            _gui->setToolButtonMenuOpened(this);
        } else {
            _gui->setToolButtonMenuOpened(NULL);
        }
        QToolButton::mousePressEvent(e);
    }

    virtual void mouseReleaseEvent(QMouseEvent* e) OVERRIDE FINAL
    {
        _gui->setToolButtonMenuOpened(NULL);
        QToolButton::mouseReleaseEvent(e);
    }

    virtual void enterEvent(QEvent* e) OVERRIDE FINAL
    {
        AutoRaiseToolButton* btn = dynamic_cast<AutoRaiseToolButton*>( _gui->getToolButtonMenuOpened() );

        if ( btn && (btn != this) && btn->menu()->isActiveWindow() ) {
            btn->menu()->close();
            btn->_menuOpened = false;
            _gui->setToolButtonMenuOpened(this);
            _menuOpened = true;
            showMenu();
        }
        QToolButton::enterEvent(e);
    }
};
} // anonymous namespace

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
GuiPrivate::addToolButton(ToolButton* tool)
{
    QToolButton* button = new AutoRaiseToolButton(_gui,_toolBox);

    button->setIcon( tool->getIcon() );
    button->setMenu( tool->getMenu() );
    button->setPopupMode(QToolButton::InstantPopup);
    button->setToolTip( Qt::convertFromPlainText(tool->getLabel(), Qt::WhiteSpaceNormal) );
    _toolBox->addWidget(button);
}

void
GuiPrivate::setUndoRedoActions(QAction* undoAction,
                               QAction* redoAction)
{
    if (_currentUndoAction) {
        menuEdit->removeAction(_currentUndoAction);
    }
    if (_currentRedoAction) {
        menuEdit->removeAction(_currentRedoAction);
    }
    _currentUndoAction = undoAction;
    _currentRedoAction = redoAction;
    menuEdit->addAction(undoAction);
    menuEdit->addAction(redoAction);
}

void
Gui::newProject()
{
    appPTR->newAppInstance(QString(),QStringList(),std::list<std::pair<int,int> >());
}

void
Gui::openProject()
{
    std::vector<std::string> filters;

    filters.push_back(NATRON_PROJECT_FILE_EXT);
    std::string selectedFile =  popOpenFileDialog( false, filters, _imp->_lastLoadProjectOpenedDir.toStdString(),false );

    if ( !selectedFile.empty() ) {
        openProjectInternal(selectedFile);
    }
}

void
Gui::openProject(const std::string& filename)
{
    openProjectInternal(filename);
}

void
Gui::openProjectInternal(const std::string & absoluteFileName)
{
    std::string fileUnPathed = absoluteFileName;
    std::string path = SequenceParsing::removePath(fileUnPathed);

    ///if the current graph has no value, just load the project in the same window
    if ( _imp->_appInstance->getProject()->isGraphWorthLess() ) {
        _imp->_appInstance->getProject()->loadProject( path.c_str(), fileUnPathed.c_str() );
    } else {
        ///remove autosaves otherwise the new instance might try to load an autosave
        Project::removeAutoSaves();
        AppInstance* newApp = appPTR->newAppInstance(QString(),QStringList(),std::list<std::pair<int,int> >());
        newApp->getProject()->loadProject( path.c_str(), fileUnPathed.c_str() );
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

void
Gui::aboutToSave()
{
    assert( QThread::currentThread() == qApp->thread() );

    ///If a tab is fullscreen, minimize it otherwise the splitter's state wouldn't be saved correctly
    std::list<TabWidget*> panesCpy;
    {
        QMutexLocker l(&_imp->_panesMutex);
        panesCpy = _imp->_panes;
    }
    for (std::list<TabWidget*>::iterator it = panesCpy.begin(); it != panesCpy.end(); ++it) {
        if ( (*it)->isFullScreen() ) {
            _imp->fullScreenWidgetDuringSave = *it;
            minimize();
            QCoreApplication::processEvents();
            break;
        }
    }
}

void
Gui::saveFinished()
{
    assert( QThread::currentThread() == qApp->thread() );
    ///fullscreen again the tab
    if (_imp->fullScreenWidgetDuringSave) {
        maximize(_imp->fullScreenWidgetDuringSave);
        _imp->fullScreenWidgetDuringSave = 0;
    }
}

bool
Gui::saveProject()
{
    if ( _imp->_appInstance->getProject()->hasProjectBeenSavedByUser() ) {
        aboutToSave();
        _imp->_appInstance->getProject()->saveProject(_imp->_appInstance->getProject()->getProjectPath(),
                                                      _imp->_appInstance->getProject()->getProjectName(),false);
        saveFinished();


        ///update the open recents
        QString file = _imp->_appInstance->getProject()->getProjectPath() + _imp->_appInstance->getProject()->getProjectName();
        QSettings settings;
        QStringList recentFiles = settings.value("recentFileList").toStringList();
        recentFiles.removeAll(file);
        recentFiles.prepend(file);
        while (recentFiles.size() > NATRON_MAX_RECENT_FILES) {
            recentFiles.removeLast();
        }

        settings.setValue("recentFileList", recentFiles);
        appPTR->updateAllRecentFileMenus();

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
    std::string outFile = popSaveFileDialog( false, filter,_imp->_lastSaveProjectOpenedDir.toStdString(),false );
    if (outFile.size() > 0) {
        if (outFile.find("." NATRON_PROJECT_FILE_EXT) == std::string::npos) {
            outFile.append("." NATRON_PROJECT_FILE_EXT);
        }
        std::string path = SequenceParsing::removePath(outFile);
        aboutToSave();
        _imp->_appInstance->getProject()->saveProject(path.c_str(),outFile.c_str(),false);
        saveFinished();

        std::string filePath = path + outFile;
        QSettings settings;
        QStringList recentFiles = settings.value("recentFileList").toStringList();
        recentFiles.removeAll( filePath.c_str() );
        recentFiles.prepend( filePath.c_str() );
        while (recentFiles.size() > NATRON_MAX_RECENT_FILES) {
            recentFiles.removeLast();
        }

        settings.setValue("recentFileList", recentFiles);
        appPTR->updateAllRecentFileMenus();

        return true;
    }

    return false;
}

void
Gui::createNewViewer()
{
    (void)_imp->_appInstance->createNode( CreateNodeArgs("Viewer",
                                                         "",
                                                         -1,-1,
                                                         -1,
                                                         true,
                                                         INT_MIN,INT_MIN,
                                                         true,
                                                         true,
                                                         QString(),
                                                         CreateNodeArgs::DefaultValuesList()) );
}

boost::shared_ptr<Natron::Node>
Gui::createReader()
{
    boost::shared_ptr<Natron::Node> ret;
    std::map<std::string,std::string> readersForFormat;

    appPTR->getCurrentSettings()->getFileFormatsForReadingAndReader(&readersForFormat);
    std::vector<std::string> filters;
    for (std::map<std::string,std::string>::const_iterator it = readersForFormat.begin(); it != readersForFormat.end(); ++it) {
        filters.push_back(it->first);
    }
    std::string pattern = popOpenFileDialog( true, filters, _imp->_lastLoadSequenceOpenedDir.toStdString(),true );
    if ( !pattern.empty() ) {
        QString qpattern( pattern.c_str() );
        std::string ext = Natron::removeFileExtension(qpattern).toLower().toStdString();
        std::map<std::string,std::string>::iterator found = readersForFormat.find(ext);
        if ( found == readersForFormat.end() ) {
            errorDialog( tr("Reader").toStdString(), tr("No plugin capable of decoding ").toStdString() + ext + tr(" was found.").toStdString() );
        } else {
            CreateNodeArgs::DefaultValuesList defaultValues;
            defaultValues.push_back(createDefaultValueForParam<std::string>(kOfxImageEffectFileParamName, pattern));
            CreateNodeArgs args(found->second.c_str(),
                                "",
                                -1,-1,
                                -1,
                                true,
                                INT_MIN,INT_MIN,
                                true,
                                true,
                                QString(),
                                defaultValues);
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
    std::map<std::string,std::string> writersForFormat;

    appPTR->getCurrentSettings()->getFileFormatsForWritingAndWriter(&writersForFormat);
    std::vector<std::string> filters;
    for (std::map<std::string,std::string>::const_iterator it = writersForFormat.begin(); it != writersForFormat.end(); ++it) {
        filters.push_back(it->first);
    }
    std::string file = popSaveFileDialog( true, filters, _imp->_lastSaveSequenceOpenedDir.toStdString(),true );
    if ( !file.empty() ) {
        QString fileCpy = file.c_str();
        std::string ext = Natron::removeFileExtension(fileCpy).toStdString();
        std::map<std::string,std::string>::iterator found = writersForFormat.find(ext);
        if ( found != writersForFormat.end() ) {
            
            CreateNodeArgs::DefaultValuesList defaultValues;
            defaultValues.push_back(createDefaultValueForParam<std::string>(kOfxImageEffectFileParamName, file));
            CreateNodeArgs args(found->second.c_str(),"",-1,-1,-1,true,INT_MIN,INT_MIN,true,true,QString(),defaultValues);
            ret = _imp->_appInstance->createNode(args);
            if (!ret) {
                return ret;
            }
        } else {
            errorDialog( tr("Writer").toStdString(), tr("No plugin capable of encoding ").toStdString() + ext + tr(" was found.").toStdString() );
        }
    }

    return ret;
}

std::string
Gui::popOpenFileDialog(bool sequenceDialog,
                       const std::vector<std::string> & initialfilters,
                       const std::string & initialDir,
                       bool allowRelativePaths)
{
    SequenceFileDialog dialog(this, initialfilters, sequenceDialog, SequenceFileDialog::OPEN_DIALOG, initialDir,this,allowRelativePaths);

    if ( dialog.exec() ) {
        return dialog.selectedFiles();
    } else {
        return std::string();
    }
}

std::string
Gui::openImageSequenceDialog()
{
    std::map<std::string,std::string> readersForFormat;
    
    appPTR->getCurrentSettings()->getFileFormatsForReadingAndReader(&readersForFormat);
    std::vector<std::string> filters;
    for (std::map<std::string,std::string>::const_iterator it = readersForFormat.begin(); it != readersForFormat.end(); ++it) {
        filters.push_back(it->first);
    }

    return popOpenFileDialog(true, filters, _imp->_lastLoadSequenceOpenedDir.toStdString(), true);
}

std::string
Gui::saveImageSequenceDialog()
{
    std::map<std::string,std::string> writersForFormat;
    
    appPTR->getCurrentSettings()->getFileFormatsForWritingAndWriter(&writersForFormat);
    std::vector<std::string> filters;
    for (std::map<std::string,std::string>::const_iterator it = writersForFormat.begin(); it != writersForFormat.end(); ++it) {
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
    SequenceFileDialog dialog(this,initialfilters,sequenceDialog,SequenceFileDialog::SAVE_DIALOG,initialDir,this,allowRelativePaths);

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
        Natron::StandardButtonEnum ret =  Natron::questionDialog(NATRON_APPLICATION_NAME,tr("Save changes to ").toStdString() +
                                                             _imp->_appInstance->getProject()->getProjectName().toStdString() + " ?",
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
    _imp->_projectGui->load(obj);
}

void
Gui::saveProjectGui(boost::archive::xml_oarchive & archive)
{
    assert(_imp->_projectGui);
    _imp->_projectGui->save(archive);
}

void
Gui::errorDialog(const std::string & title,
                 const std::string & text)
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
        emit doDialog(0,QString( title.c_str() ),QString( text.c_str() ),buttons,(int)Natron::eStandardButtonYes);
        locker.relock();
        while (_imp->_uiUsingMainThread) {
            _imp->_uiUsingMainThreadCond.wait(&_imp->_uiUsingMainThreadMutex);
        }
    } else {
        emit doDialog(0,QString( title.c_str() ),QString( text.c_str() ),buttons,(int)Natron::eStandardButtonYes);
    }
}

void
Gui::warningDialog(const std::string & title,
                   const std::string & text)
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
        emit doDialog(1,QString( title.c_str() ),QString( text.c_str() ),buttons,(int)Natron::eStandardButtonYes);
        locker.relock();
        while (_imp->_uiUsingMainThread) {
            _imp->_uiUsingMainThreadCond.wait(&_imp->_uiUsingMainThreadMutex);
        }
    } else {
        emit doDialog(1,QString( title.c_str() ),QString( text.c_str() ),buttons,(int)Natron::eStandardButtonYes);
    }
}

void
Gui::informationDialog(const std::string & title,
                       const std::string & text)
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
        emit doDialog(2,QString( title.c_str() ),QString( text.c_str() ),buttons,(int)Natron::eStandardButtonYes);
        locker.relock();
        while (_imp->_uiUsingMainThread) {
            _imp->_uiUsingMainThreadCond.wait(&_imp->_uiUsingMainThreadMutex);
        }
    } else {
        emit doDialog(2,QString( title.c_str() ),QString( text.c_str() ),buttons,(int)Natron::eStandardButtonYes);
    }
}

void
Gui::onDoDialog(int type,
                const QString & title,
                const QString & content,
                Natron::StandardButtons buttons,
                int defaultB)
{
    QString msg = Qt::convertFromPlainText(content, Qt::WhiteSpaceNormal);

    if (type == 0) {
        QMessageBox critical(QMessageBox::Critical, title, msg, QMessageBox::NoButton, this, Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowStaysOnTopHint);
        critical.setTextFormat(Qt::RichText);   //this is what makes the links clickable
        critical.exec();
    } else if (type == 1) {
        QMessageBox warning(QMessageBox::Warning, title, msg, QMessageBox::NoButton, this, Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowStaysOnTopHint);
        warning.setTextFormat(Qt::RichText);
        warning.exec();
    } else if (type == 2) {
        QMessageBox info(QMessageBox::Information, title, msg, QMessageBox::NoButton, this, Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowStaysOnTopHint);
        info.setTextFormat(Qt::RichText);
        info.exec();
    } else {
        QMessageBox ques(QMessageBox::Question, title, msg, QtEnumConvert::toQtStandarButtons(buttons),
                         this, Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowStaysOnTopHint);
        ques.setDefaultButton( QtEnumConvert::toQtStandardButton( (Natron::StandardButtonEnum)defaultB ) );
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
        emit doDialog(3,QString( title.c_str() ),QString( message.c_str() ),buttons,(int)defaultButton);
        locker.relock();
        while (_imp->_uiUsingMainThread) {
            _imp->_uiUsingMainThreadCond.wait(&_imp->_uiUsingMainThreadMutex);
        }
    } else {
        emit doDialog(3,QString( title.c_str() ),QString( message.c_str() ),buttons,(int)defaultButton);
    }

    return _imp->_lastQuestionDialogAnswer;
}

void
Gui::selectNode(boost::shared_ptr<NodeGui> node)
{
    if (!node) {
        return;
    }
    _imp->_nodeGraphArea->selectNode(node,false); //< wipe current selection
}

void
Gui::connectInput1()
{
    _imp->_nodeGraphArea->connectCurrentViewerToSelection(0);
}

void
Gui::connectInput2()
{
    _imp->_nodeGraphArea->connectCurrentViewerToSelection(1);
}

void
Gui::connectInput3()
{
    _imp->_nodeGraphArea->connectCurrentViewerToSelection(2);
}

void
Gui::connectInput4()
{
    _imp->_nodeGraphArea->connectCurrentViewerToSelection(3);
}

void
Gui::connectInput5()
{
    _imp->_nodeGraphArea->connectCurrentViewerToSelection(4);
}

void
Gui::connectInput6()
{
    _imp->_nodeGraphArea->connectCurrentViewerToSelection(5);
}

void
Gui::connectInput7()
{
    _imp->_nodeGraphArea->connectCurrentViewerToSelection(6);
}

void
Gui::connectInput8()
{
    _imp->_nodeGraphArea->connectCurrentViewerToSelection(7);
}

void
Gui::connectInput9()
{
    _imp->_nodeGraphArea->connectCurrentViewerToSelection(8);
}

void
Gui::connectInput10()
{
    _imp->_nodeGraphArea->connectCurrentViewerToSelection(9);
}

void
GuiPrivate::restoreGuiGeometry()
{
    QSettings settings(NATRON_ORGANIZATION_NAME,NATRON_APPLICATION_NAME);

    settings.beginGroup("MainWindow");

    if ( settings.contains("pos") ) {
        QPoint pos = settings.value("pos").toPoint();
        _gui->move(pos);
    }
    if ( settings.contains("size") ) {
        QSize size = settings.value("size").toSize();
        _gui->resize(size);
    } else {
        ///No window size serialized, give some appriopriate default value according to the screen size
        QDesktopWidget* desktop = QApplication::desktop();
        QRect screen = desktop->screenGeometry();
        _gui->resize( (int)( 0.93 * screen.width() ),(int)( 0.93 * screen.height() ) ); // leave some space
    }
    if ( settings.contains("fullScreen") ) {
        bool fs = settings.value("fullScreen").toBool();
        if (fs) {
            _gui->toggleFullScreen();
        }
    }

    settings.endGroup();

    if ( settings.contains("LastOpenProjectDialogPath") ) {
        _lastLoadSequenceOpenedDir = settings.value("LastOpenProjectDialogPath").toString();
    }
    if ( settings.contains("LastSaveProjectDialogPath") ) {
        _lastLoadSequenceOpenedDir = settings.value("LastSaveProjectDialogPath").toString();
    }
    if ( settings.contains("LastLoadSequenceDialogPath") ) {
        _lastLoadSequenceOpenedDir = settings.value("LastLoadSequenceDialogPath").toString();
    }
    if ( settings.contains("LastSaveSequenceDialogPath") ) {
        _lastLoadSequenceOpenedDir = settings.value("LastSaveSequenceDialogPath").toString();
    }
}

void
GuiPrivate::saveGuiGeometry()
{
    QSettings settings(NATRON_ORGANIZATION_NAME,NATRON_APPLICATION_NAME);

    settings.beginGroup("MainWindow");
    settings.setValue( "pos", _gui->pos() );
    settings.setValue( "size", _gui->size() );
    settings.setValue( "fullScreen", _gui->isFullScreen() );

    settings.endGroup();

    settings.setValue("LastOpenProjectDialogPath", _lastLoadProjectOpenedDir);
    settings.setValue("LastSaveProjectDialogPath", _lastSaveProjectOpenedDir);
    settings.setValue("LastLoadSequenceDialogPath", _lastLoadSequenceOpenedDir);
    settings.setValue("LastSaveSequenceDialogPath", _lastSaveSequenceOpenedDir);
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
    _imp->_settingsGui->show();
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
    std::map<QUndoStack*,std::pair<QAction*,QAction*> >::iterator it = _imp->_undoStacksActions.find(stack);

    if ( it != _imp->_undoStacksActions.end() ) {
        _imp->_undoStacksActions.erase(it);
    }
}

void
Gui::onCurrentUndoStackChanged(QUndoStack* stack)
{
    std::map<QUndoStack*,std::pair<QAction*,QAction*> >::iterator it = _imp->_undoStacksActions.find(stack);

    //the stack must have been registered first with registerNewUndoStack()
    if ( it != _imp->_undoStacksActions.end() ) {
        _imp->setUndoRedoActions(it->second.first, it->second.second);
    }
}

void
Gui::refreshAllPreviews()
{
    int time = _imp->_appInstance->getTimeLine()->currentFrame();
    std::vector<boost::shared_ptr<Natron::Node> > nodes;

    _imp->_appInstance->getActiveNodes(&nodes);
    for (U32 i = 0; i < nodes.size(); ++i) {
        if ( nodes[i]->isPreviewEnabled() ) {
            nodes[i]->refreshPreviewImage(time);
        }
    }
}

void
Gui::forceRefreshAllPreviews()
{
    int time = _imp->_appInstance->getTimeLine()->currentFrame();
    std::vector<boost::shared_ptr<Natron::Node> > nodes;

    _imp->_appInstance->getActiveNodes(&nodes);
    for (U32 i = 0; i < nodes.size(); ++i) {
        if ( nodes[i]->isPreviewEnabled() ) {
            nodes[i]->computePreviewImage(time);
        }
    }
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
    _imp->_aboutWindow->exec();
}

void
Gui::showShortcutEditor()
{
    _imp->shortcutEditor->show();
}

void
Gui::openRecentFile()
{
    QAction *action = qobject_cast<QAction *>( sender() );

    if (action) {
        QFileInfo f( action->data().toString() );
        QString path = f.path() + QDir::separator();
        ///if the current graph has no value, just load the project in the same window
        if ( _imp->_appInstance->getProject()->isGraphWorthLess() ) {
            _imp->_appInstance->getProject()->loadProject( path,f.fileName() );
        } else {
            ///remove autosaves otherwise the new instance might try to load an autosave
            Project::removeAutoSaves();
            AppInstance* newApp = appPTR->newAppInstance(QString(),QStringList(),std::list<std::pair<int,int> >());
            newApp->getProject()->loadProject( path,f.fileName() );
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
Gui::setColorPickersColor(const QColor & c)
{
    assert(_imp->_projectGui);
    _imp->_projectGui->setPickersColor(c);
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
Gui::activateViewerTab(ViewerInstance* viewer)
{
    OpenGLViewerI* viewport = viewer->getUiContext();

    {
        QMutexLocker l(&_imp->_viewerTabsMutex);
        for (std::list<ViewerTab*>::iterator it = _imp->_viewerTabs.begin(); it != _imp->_viewerTabs.end(); ++it) {
            if ( (*it)->getViewer() == viewport ) {
                TabWidget* viewerAnchor = getAnchor();
                assert(viewerAnchor);
                viewerAnchor->appendTab(*it);
                (*it)->show();
            }
        }
    }
    emit viewersChanged();
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
    }

    if (v) {
        removeViewerTab(v, true,false);
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
    RenderingProgressDialog *dialog = new RenderingProgressDialog(this,sequenceName,firstFrame,lastFrame,process,this);

    dialog->show();
}

void
Gui::setLastSelectedViewer(ViewerTab* tab)
{
    _imp->_lastSelectedViewer = tab;
}

ViewerTab*
Gui::getLastSelectedViewer() const
{
    return _imp->_lastSelectedViewer;
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
        name.append("Pane");
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
            name = QString("Pane%1").arg(baseNumber);
        } else {
            break;
        }
    }

    return name;
}

void
Gui::setUserScrubbingTimeline(bool b)
{
    _imp->_isUserScrubbingTimeline = b;
}

bool
Gui::isUserScrubbingTimeline() const
{
    return _imp->_isUserScrubbingTimeline;
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

QWidget*
Gui::getPropertiesBin() const
{
    return _imp->_propertiesScrollArea;
}

QVBoxLayout*
Gui::getPropertiesLayout() const
{
    return _imp->_layoutPropertiesBin;
}

void
Gui::appendTabToDefaultViewerPane(QWidget* tab)
{
    TabWidget* viewerAnchor = getAnchor();

    assert(viewerAnchor);
    viewerAnchor->appendTab(tab);
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

const std::map<std::string,QWidget*> &
Gui::getRegisteredTabs() const
{
    return _imp->_registeredTabs;
}

void
Gui::debugImage(const Natron::Image* image,
                const QString & filename )
{
    if (image->getBitDepth() != Natron::eImageBitDepthFloat) {
        qDebug() << "Debug image only works on float images.";

        return;
    }
    const RectI & rod = image->getBounds();
    QImage output(rod.width(),rod.height(),QImage::Format_ARGB32);
    const Natron::Color::Lut* lut = Natron::Color::LutManager::sRGBLut();
    const float* from = (const float*)image->pixelAt( rod.left(), rod.bottom() );

    ///offset the pointer to 0,0
    from -= ( ( rod.bottom() * image->getRowElements() ) + rod.left() * image->getComponentsCount() );
    lut->to_byte_packed(output.bits(), from, rod, rod, rod,
                        Natron::Color::PACKING_RGBA,Natron::Color::PACKING_BGRA, true,false);
    U64 hashKey = image->getHashKey();
    QString hashKeyStr = QString::number(hashKey);
    QString realFileName = filename.isEmpty() ? QString(hashKeyStr + ".png") : filename;
    std::cout << "DEBUG: writing image: " << realFileName.toStdString() << std::endl;
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
Gui::onWriterRenderStarted(const QString & sequenceName,
                           int firstFrame,
                           int lastFrame,
                           Natron::OutputEffectInstance* writer)
{
    assert( QThread::currentThread() == qApp->thread() );

    RenderingProgressDialog *dialog = new RenderingProgressDialog(this,sequenceName,firstFrame,lastFrame,
                                                                  boost::shared_ptr<ProcessHandler>(),this);
    RenderEngine* engine = writer->getRenderEngine();


    QObject::connect( dialog,SIGNAL( canceled() ),engine,SLOT( abortRendering_Blocking() ) );
    QObject::connect( engine,SIGNAL( frameRendered(int) ),dialog,SLOT( onFrameRendered(int) ) );
    QObject::connect( engine,SIGNAL( renderFinished(int) ),dialog,SLOT( onVideoEngineStopped(int) ) );
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
    NodeGui* node = qobject_cast<NodeGui*>( sender() );

    if ( node && (node->getNode()->getPluginID() == "Viewer") ) {
        emit viewersChanged();
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
    const std::list<boost::shared_ptr<NodeGui> > & selectedNodes = _imp->_nodeGraphArea->getSelectedNodes();

    if (selectedNodes.size() > 1) {
        Natron::warningDialog( tr("Render").toStdString(), tr("Please select only a single node").toStdString() );
    } else if ( selectedNodes.empty() ) {
        Natron::warningDialog( tr("Render").toStdString(), tr("You must select a node to render first!").toStdString() );
    } else {
        const boost::shared_ptr<NodeGui> & selectedNode = selectedNodes.front();
        std::list<AppInstance::RenderWork> workList;
        if ( selectedNode->getNode()->getLiveInstance()->isWriter() ) {
            ///if the node is a writer, just use it to render!
            AppInstance::RenderWork w;
            w.writer = dynamic_cast<Natron::OutputEffectInstance*>(selectedNode->getNode()->getLiveInstance());
            assert(w.writer);
            w.firstFrame = INT_MIN;
            w.lastFrame = INT_MAX;
            workList.push_back(w);
            _imp->_appInstance->startWritersRendering(workList);
        } else {
            ///create a node and connect it to the node and use it to render
            boost::shared_ptr<Natron::Node> writer = createWriter();
            if (writer) {
                AppInstance::RenderWork w;
                w.writer = dynamic_cast<Natron::OutputEffectInstance*>(writer->getLiveInstance());
                assert(w.writer);
                w.firstFrame = INT_MIN;
                w.lastFrame = INT_MAX;
                workList.push_back(w);
                _imp->_appInstance->startWritersRendering(workList);
            }
        }
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
    LogWindow lw(log,this);

    lw.setWindowTitle( tr("Errors log") );
    lw.exec();
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
        (*it)->removeTrackerInterface(n, permanently,false);
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
        (*it)->updateRotoSelectedTool(tool,roto);
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
        (*it)->removeRotoInterface(n, permanently,false);
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
Gui::startProgress(Natron::EffectInstance* effect,
                   const std::string & message)
{
    if (!effect) {
        return;
    }
    if ( QThread::currentThread() != qApp->thread() ) {
        qDebug() << "Progress bars called from a thread different than the main-thread is not supported at the moment.";

        return;
    }

    QProgressDialog* dialog = new QProgressDialog(message.c_str(),tr("Cancel"),0,100,this);
    dialog->setModal(false);
    dialog->setRange(0, 100);
    dialog->setMinimumWidth(250);
    dialog->setWindowTitle( effect->getNode()->getName_mt_safe().c_str() );
    std::map<Natron::EffectInstance*,QProgressDialog*>::iterator found = _imp->_progressBars.find(effect);

    ///If a second dialog was asked for whilst another is still active, the first dialog will not be
    ///able to be canceled.
    if ( found != _imp->_progressBars.end() ) {
        _imp->_progressBars.erase(found);
    }

    _imp->_progressBars.insert( std::make_pair(effect,dialog) );
    dialog->show();
    //dialog->exec();
}

void
Gui::endProgress(Natron::EffectInstance* effect)
{
    if ( QThread::currentThread() != qApp->thread() ) {
        qDebug() << "Progress bars called from a thread different than the main-thread is not supported at the moment.";

        return;
    }

    std::map<Natron::EffectInstance*,QProgressDialog*>::iterator found = _imp->_progressBars.find(effect);
    if ( found == _imp->_progressBars.end() ) {
        return;
    }


    found->second->close();
    _imp->_progressBars.erase(found);
}

bool
Gui::progressUpdate(Natron::EffectInstance* effect,
                    double t)
{
    if ( QThread::currentThread() != qApp->thread() ) {
        qDebug() << "Progress bars called from a thread different than the main-thread is not supported at the moment.";

        return true;
    }

    std::map<Natron::EffectInstance*,QProgressDialog*>::iterator found = _imp->_progressBars.find(effect);
    if ( found == _imp->_progressBars.end() ) {
        qDebug() << effect->getNode()->getName_mt_safe().c_str() <<  " called progressUpdate but didn't called startProgress first.";
    }
    if ( found->second->wasCanceled() ) {
        return false;
    }
    found->second->setValue(t * 100);
    QCoreApplication::processEvents();

    return true;
}

void
Gui::addVisibleDockablePanel(DockablePanel* panel)
{
    putSettingsPanelFirst(panel);
    assert(panel);
    int maxPanels = appPTR->getCurrentSettings()->getMaxPanelsOpened();
    if ( ( (int)_imp->openedPanels.size() == maxPanels ) && (maxPanels != 0) ) {
        std::list<DockablePanel*>::iterator it = _imp->openedPanels.begin();
        (*it)->closePanel();
    }
    _imp->openedPanels.push_back(panel);
}

void
Gui::removeVisibleDockablePanel(DockablePanel* panel)
{
    std::list<DockablePanel*>::iterator it = std::find(_imp->openedPanels.begin(),_imp->openedPanels.end(),panel);

    if ( it != _imp->openedPanels.end() ) {
        _imp->openedPanels.erase(it);
    }
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

NodeBackDrop*
Gui::createBackDrop(bool requestedByLoad,
                    const NodeBackDropSerialization & serialization)
{
    return _imp->_nodeGraphArea->createBackDrop(_imp->_layoutPropertiesBin,requestedByLoad,serialization);
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

void
Gui::resizeEvent(QResizeEvent* e)
{
    QMainWindow::resizeEvent(e);

    setMtSafeWindowSize( width(), height() );
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
        _imp->_isGUIFrozen = clicked;
    }
    _imp->_freezeUIButton->setDown(clicked);
    _imp->_nodeGraphArea->onGuiFrozenChanged(clicked);
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

FloatingWidget::FloatingWidget(Gui* gui,
                               QWidget* parent)
    : QWidget(parent)
      , SerializableWindow()
      , _embeddedWidget(0)
      , _layout(0)
      , _gui(gui)
{
    setWindowFlags(Qt::WindowStaysOnTopHint | Qt::Window);
    setAttribute(Qt::WA_DeleteOnClose,true);
    _layout = new QVBoxLayout(this);
    _layout->setContentsMargins(0, 0, 0, 0);
    setLayout(_layout);
}

static void
closeWidgetRecursively(QWidget* w)
{
    Splitter* isSplitter = dynamic_cast<Splitter*>(w);
    TabWidget* isTab = dynamic_cast<TabWidget*>(w);

    if (!isSplitter && !isTab) {
        return;
    }

    if (isTab) {
        isTab->closePane();
    } else {
        assert(isSplitter);
        for (int i = 0; i < isSplitter->count(); ++i) {
            closeWidgetRecursively( isSplitter->widget(i) );
        }
    }
}

FloatingWidget::~FloatingWidget()
{
    if (_embeddedWidget) {
        closeWidgetRecursively(_embeddedWidget);
    }
}

void
FloatingWidget::setWidget(QWidget* w)
{
    QSize widgetSize = w->size();

    assert(w);
    if (_embeddedWidget) {
        return;
    }
    _embeddedWidget = w;
    w->setParent(this);
    assert(_layout);
    _layout->addWidget(w);
    w->setVisible(true);
    w->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    resize(widgetSize);
    show();
}

void
FloatingWidget::removeEmbeddedWidget()
{
    if (!_embeddedWidget) {
        return;
    }
    _layout->removeWidget(_embeddedWidget);
    _embeddedWidget->setParent(NULL);
    _embeddedWidget = 0;
    // _embeddedWidget->setVisible(false);
    hide();
}

void
FloatingWidget::moveEvent(QMoveEvent* e)
{
    QWidget::moveEvent(e);
    QPoint p = pos();

    setMtSafePosition( p.x(), p.y() );
}

void
FloatingWidget::resizeEvent(QResizeEvent* e)
{
    QWidget::resizeEvent(e);

    setMtSafeWindowSize( width(), height() );
}

void
FloatingWidget::closeEvent(QCloseEvent* e)
{
    emit closed();

    closeWidgetRecursively(_embeddedWidget);
    removeEmbeddedWidget();
    _gui->unregisterFloatingWindow(this);
    QWidget::closeEvent(e);
}


void
Gui::getNodesEntitledForOverlays(std::list<boost::shared_ptr<NodeGui> >& nodes) const
{
    int layoutItemsCount = _imp->_layoutPropertiesBin->count();
    for (int i = 0; i < layoutItemsCount; ++i) {
        QLayoutItem* item = _imp->_layoutPropertiesBin->itemAt(i);
        NodeSettingsPanel* panel = dynamic_cast<NodeSettingsPanel*>(item->widget());
        if (panel && panel->getNode() && panel->getNode()->shouldDrawOverlay()) {
            nodes.push_back(panel->getNode());
        }
    }
}

