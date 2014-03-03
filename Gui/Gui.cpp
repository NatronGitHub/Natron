//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */

#include "Gui/Gui.h"

#include <cassert>

#include <QWaitCondition>
#include <QMutex>
#include <QTextDocument> // for Qt::convertFromPlainText
#include <QCoreApplication>
#include <QAction>
#include <QSettings>
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
#include <QSplitter>
#include <QToolBar>
#include <QKeySequence>
#include <QScrollArea>
#include <QScrollBar>
#include <QToolButton>
#include <QMessageBox>
#include <QImage>

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

#include "Engine/ViewerInstance.h"
#include "Engine/Project.h"
#include "Engine/Plugin.h"
#include "Engine/Settings.h"
#include "Engine/KnobFile.h"
#include "Engine/SequenceParsing.h"
#include "Engine/ProcessHandler.h"
#include "Engine/Lut.h"
#include "Engine/Image.h"
#include "Engine/VideoEngine.h"

#define PLUGIN_GROUP_DEFAULT "Other"
#define PLUGIN_GROUP_IMAGE "Image"
#define PLUGIN_GROUP_COLOR "Color"
#define PLUGIN_GROUP_FILTER "Filter"
#define PLUGIN_GROUP_TRANSFORM "Transform"
#define PLUGIN_GROUP_DEEP "Deep"
#define PLUGIN_GROUP_MULTIVIEW "Views"
#define PLUGIN_GROUP_OFX "OFX"
#define PLUGIN_GROUP_TIME "Time"
#define PLUGIN_GROUP_PAINT "Paint"

#define PLUGIN_GROUP_DEFAULT_ICON_PATH NATRON_IMAGES_PATH"misc_low.png"

using namespace Natron;


struct GuiPrivate {
    
    Gui* _gui; //< ptr to the public interface
    
    bool _isUserScrubbingTimeline; //< true if the user is actively moving the cursor on the timeline. False on mouse release.
    ViewerTab* _lastSelectedViewer; //< a ptr to the last selected ViewerTab
    GuiAppInstance* _appInstance; //< ptr to the appInstance
    
    ///Dialogs handling members
    QWaitCondition _uiUsingMainThreadCond; //< used with _uiUsingMainThread
    bool _uiUsingMainThread; //< true when the Gui is showing a dialog in the main thread
    mutable QMutex _uiUsingMainThreadMutex; //< protects _uiUsingMainThread
    Natron::StandardButton _lastQuestionDialogAnswer; //< stores the last question answer
    
    ///ptrs to the undo/redo actions from the active stack.
    QAction* _currentUndoAction;
    QAction* _currentRedoAction;
    
    ///all the undo stacks of Natron are gathered here
    QUndoGroup* _undoStacksGroup;
    std::map<QUndoStack*,std::pair<QAction*,QAction*> > _undoStacksActions;
    
    ///all the splitters used to separate the "panes" of the application
    std::list<QSplitter*> _splitters;
    

    ///all the menu actions
    QAction *actionNew_project;
    QAction *actionOpen_project;
    QAction *actionSave_project;
    QAction *actionSaveAs_project;
    QAction *actionPreferences;
    QAction *actionExit;
    QAction *actionProject_settings;
    QAction *actionFullScreen;
    QAction *actionClearDiskCache;
    QAction *actionClearPlayBackCache;
    QAction *actionClearNodeCache;
    QAction *actionClearPluginsLoadingCache;
    QAction *actionClearAllCaches;
    QAction *actionShowAboutWindow;
    QAction *actionsOpenRecentFile[NATRON_MAX_RECENT_FILES];
    
    QAction* actionConnectInput1;
    QAction* actionConnectInput2;
    QAction* actionConnectInput3;
    QAction* actionConnectInput4;
    QAction* actionConnectInput5;
    QAction* actionConnectInput6;
    QAction* actionConnectInput7;
    QAction* actionConnectInput8;
    QAction* actionConnectInput9;
    QAction* actionConnectInput10;
    
    ///the main "central" widget
    QWidget *_centralWidget;
    QHBoxLayout* _mainLayout; //< its layout
    
    ///strings that remember for project save/load and writers/reader dialog where
    ///the user was the last time.
    QString _lastLoadSequenceOpenedDir;
    QString _lastLoadProjectOpenedDir;
    QString _lastSaveSequenceOpenedDir;
    QString _lastSaveProjectOpenedDir;
    
    ///the initial pane where the first Viewer is by default.
    TabWidget* _viewersPane; 
    
    // this one is a ptr to others TabWidget.
    //It tells where to put the viewer when making a new one
    // If null it places it on default tab widget
    TabWidget* _nextViewerTabPlace;

    ///the initial pane where the Curve Editor, Node graph, Dope sheet (etc...) are located.
    TabWidget* _workshopPane;
    
    ///The splitter separating the workshop pane of the viewers pane.
    QSplitter* _viewerWorkshopSplitter;
    
    ///the initial pane where the properties are layed out.
    TabWidget* _propertiesPane;
    
    ///the splitter separating _viewerWorkshopSplitter and the properties pane.
    QSplitter* _middleRightSplitter;
    
    ///the splitter separating _middleRightSplitter and the left toolbar
    QSplitter* _leftRightSplitter;

    ///a list of ptrs to all the viewer tabs.
    std::list<ViewerTab*> _viewerTabs;
    
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
    
    ///the scrollarea holding the properties dock
    QScrollArea *_propertiesScrollArea;
    
    ///the main container of the properties
    QWidget *_propertiesContainer;
    
    ///the vertical layout for the properties dock container.
    QVBoxLayout *_layoutPropertiesBin;
    
    ///The menu bar and all the menus
    QMenuBar *menubar;
    QMenu *menuFile;
    QMenu *menuRecentFiles;
    QMenu *menuEdit;
    QMenu *menuDisplay;
    QMenu *menuOptions;
	QMenu *viewersMenu;
    QMenu *viewerInputsMenu;
    QMenu *viewersViewMenu;
    QMenu *cacheMenu;
    
    
    ///all TabWidget's : used to know what to hide/show for fullscreen mode
    std::list<TabWidget*> _panes;
    
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
    
    GuiPrivate(GuiAppInstance* app,Gui* gui)
    : _gui(gui)
    , _isUserScrubbingTimeline(false)
    , _lastSelectedViewer(NULL)
    , _appInstance(app)
    , _uiUsingMainThreadCond()
    , _uiUsingMainThread(false)
    , _uiUsingMainThreadMutex()
    , _lastQuestionDialogAnswer(Natron::No)
    , _currentUndoAction(0)
    , _currentRedoAction(0)
    , _undoStacksGroup(0)
    , _undoStacksActions()
    , _splitters()
    , actionNew_project(0)
    , actionOpen_project(0)
    , actionSave_project(0)
    , actionSaveAs_project(0)
    , actionPreferences(0)
    , actionExit(0)
    , actionProject_settings(0)
    , actionFullScreen(0)
    , actionClearDiskCache(0)
    , actionClearPlayBackCache(0)
    , actionClearNodeCache(0)
    , actionClearPluginsLoadingCache(0)
    , actionClearAllCaches(0)
    , actionShowAboutWindow(0)
    , actionsOpenRecentFile()
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
    , _centralWidget(0)
    , _mainLayout(0)
    , _lastLoadSequenceOpenedDir()
    , _lastLoadProjectOpenedDir()
    , _lastSaveSequenceOpenedDir()
    , _lastSaveProjectOpenedDir()
    , _viewersPane(0)
    , _nextViewerTabPlace(0)
    , _workshopPane(0)
    , _viewerWorkshopSplitter(0)
    , _propertiesPane(0)
    , _middleRightSplitter(0)
    , _leftRightSplitter(0)
    , _graphScene(0)
    , _nodeGraphArea(0)
    , _curveEditor(0)
    , _toolBox(0)
    , _propertiesScrollArea(0)
    , _propertiesContainer(0)
    , _layoutPropertiesBin(0)
    , menubar(0)
    , menuFile(0)
    , menuRecentFiles(0)
    , menuEdit(0)
    , menuDisplay(0)
    , menuOptions(0)
    , viewersMenu(0)
    , viewerInputsMenu(0)
    , cacheMenu(0)
    , _settingsGui(0)
    , _projectGui(0)
    , _currentlyDraggedPanel(0)
    , _aboutWindow(0)
    {
        
    }
    
    
    void restoreGuiGeometry();
    
    void saveGuiGeometry();
    
    void setUndoRedoActions(QAction* undoAction,QAction* redoAction);

    void retranslateUi(QMainWindow *MainWindow);

};

// Helper function: Get the icon with the given name from the icon theme.
// If unavailable, fall back to the built-in icon. Icon names conform to this specification:
// http://standards.freedesktop.org/icon-naming-spec/icon-naming-spec-latest.html
static QIcon get_icon(const QString &name)
{
    return QIcon::fromTheme(name, QIcon(QString(":icons/") + name));
}

Gui::Gui(GuiAppInstance* app,QWidget* parent)
: QMainWindow(parent)
, _imp(new GuiPrivate(app,this))

{
    QObject::connect(this,SIGNAL(doDialog(int,QString,QString,Natron::StandardButtons,int)),this,
                     SLOT(onDoDialog(int,QString,QString,Natron::StandardButtons,int)));
    QObject::connect(app,SIGNAL(pluginsPopulated()),this,SLOT(addToolButttonsToToolBar()));
}

Gui::~Gui()
{
    delete _imp->_projectGui;
    delete _imp->_undoStacksGroup;
    _imp->_viewerTabs.clear();
    for(U32 i = 0; i < _imp->_toolButtons.size();++i){
        delete _imp->_toolButtons[i];
    }
}

bool Gui::exitGui()
{
    int ret = saveWarning();
    if (ret == 0) {
        if (!saveProject()) {
            return false;
        }
    } else if (ret == 2) {
        return false;
    }
    _imp->saveGuiGeometry();
    return exit();
}

#pragma message WARN("same thing should be done in the non-Gui app, and should be connected to aboutToQuit() also")
bool Gui::exit()
{
    assert(_imp->_appInstance);
	int appId = _imp->_appInstance->getAppID();
    bool rVal = false;
    if (appId != 0) {
        delete _imp->_appInstance;
        delete this;
    } else {
        delete this;
        delete appPTR;
        rVal = true;
    }
    return rVal;
}

void Gui::toggleFullScreen()
{
    if (isFullScreen()) {
        showNormal();
    } else {
        showFullScreen();
    }
}

void Gui::closeEvent(QCloseEvent *e) {
    assert(e);
    if (!exit()) {
        e->ignore();
        return;
    }
    e->accept();
}


NodeGui* Gui::createNodeGUI( Node* node,bool requestedByLoad){
    assert(_imp->_nodeGraphArea);
    NodeGui* nodeGui = _imp->_nodeGraphArea->createNodeGUI(_imp->_layoutPropertiesBin,node,requestedByLoad);
    assert(nodeGui);
    return nodeGui;
}

void Gui::addNodeGuiToCurveEditor(NodeGui* node){
    _imp->_curveEditor->addNode(node);
}

void Gui::createViewerGui(Node* viewer){
    TabWidget* where = _imp->_nextViewerTabPlace;
    if(!where){
        where = _imp->_viewersPane;
    }else{
        _imp->_nextViewerTabPlace = NULL; // < reseting anchor to default
    }
    ViewerInstance* v = dynamic_cast<ViewerInstance*>(viewer->getLiveInstance());
    assert(v);
    _imp->_lastSelectedViewer = addNewViewerTab(v, where);
    v->setUiContext(_imp->_lastSelectedViewer->getViewer());
}


NodeGui* Gui::getSelectedNode() const {
    assert(_imp->_nodeGraphArea);
    return _imp->_nodeGraphArea->getSelectedNode();
}


void Gui::createGui(){
    
    
    setupUi();
    
    ///post a fake event so the qt handlers are called and the proper widget receives the focus
    QMouseEvent e(QEvent::MouseMove,QCursor::pos(),Qt::NoButton,Qt::NoButton,Qt::NoModifier);
    qApp->sendEvent(this, &e);
    
    QObject::connect(QCoreApplication::instance(), SIGNAL(aboutToQuit()), this, SLOT(exit()));
    
}

bool Gui::eventFilter(QObject *target, QEvent *event) {
    assert(_imp->_appInstance);
    if(dynamic_cast<QInputEvent*>(event)){
        /*Make top level instance this instance since it receives all
         user inputs.*/
        appPTR->setAsTopLevelInstance(_imp->_appInstance->getAppID());
    }
    
    return QMainWindow::eventFilter(target, event);
    
}
void GuiPrivate::retranslateUi(QMainWindow *MainWindow)
{
    Q_UNUSED(MainWindow);
    _gui->setWindowTitle(QCoreApplication::applicationName());
    assert(actionNew_project);
    actionNew_project->setText(_gui->tr("&New Project"));
    assert(actionOpen_project);
    actionOpen_project->setText(_gui->tr("&Open Project..."));
    assert(actionSave_project);
    actionSave_project->setText(_gui->tr("&Save Project"));
    assert(actionSaveAs_project);
    actionSaveAs_project->setText(_gui->tr("Save Project As..."));
    assert(actionPreferences);
    actionPreferences->setText(_gui->tr("Preferences"));
    assert(actionExit);
    actionExit->setText(_gui->tr("E&xit"));
    assert(actionProject_settings);
    actionProject_settings->setText(_gui->tr("Project Settings..."));
    assert(actionFullScreen);
    actionFullScreen->setText(_gui->tr("Toggle Full Screen"));
    assert(actionClearDiskCache);
    actionClearDiskCache->setText(_gui->tr("Clear Disk Cache"));
    assert(actionClearPlayBackCache);
    actionClearPlayBackCache->setText(_gui->tr("Clear Playback Cache"));
    assert(actionClearNodeCache);
    actionClearNodeCache->setText(_gui->tr("Clear Per-Node Cache"));
    assert(actionClearPluginsLoadingCache);
    actionClearPluginsLoadingCache->setText(_gui->tr("Clear plugins loading cache"));
    assert(actionClearAllCaches);
    actionClearAllCaches->setText(_gui->tr("Clear all caches"));
    assert(actionShowAboutWindow);
    actionShowAboutWindow->setText(_gui->tr("About"));
    
    assert(actionConnectInput1);
    actionConnectInput1->setText(_gui->tr("Connect to input 1"));
    assert(actionConnectInput2);
    actionConnectInput2 ->setText(_gui->tr("Connect to input 2"));
    assert(actionConnectInput3);
    actionConnectInput3 ->setText(_gui->tr("Connect to input 3"));
    assert(actionConnectInput4);
    actionConnectInput4 ->setText(_gui->tr("Connect to input 4"));
    assert(actionConnectInput5);
    actionConnectInput5 ->setText(_gui->tr("Connect to input 5"));
    assert(actionConnectInput6);
    actionConnectInput6 ->setText(_gui->tr("Connect to input 6"));
    assert(actionConnectInput7);
    actionConnectInput7 ->setText(_gui->tr("Connect to input 7"));
    assert(actionConnectInput8);
    actionConnectInput8 ->setText(_gui->tr("Connect to input 8"));
    assert(actionConnectInput9);
    actionConnectInput9 ->setText(_gui->tr("Connect to input 9"));
    assert(actionConnectInput10);
    actionConnectInput10 ->setText(_gui->tr("Connect to input 10"));
    
    
    assert(menuFile);
    menuFile->setTitle(_gui->tr("File"));
    assert(menuRecentFiles);
    menuRecentFiles->setTitle(_gui->tr("Open recent"));
    assert(menuEdit);
    menuEdit->setTitle(_gui->tr("Edit"));
    assert(menuDisplay);
    menuDisplay->setTitle(_gui->tr("Display"));
    assert(menuOptions);
    menuOptions->setTitle(_gui->tr("Options"));
    assert(viewersMenu);
    viewersMenu->setTitle(_gui->tr("Viewer(s)"));
    assert(cacheMenu);
    cacheMenu->setTitle(_gui->tr("Cache"));
    assert(viewerInputsMenu);
    viewerInputsMenu->setTitle(_gui->tr("Connect Current Viewer"));
    assert(viewersViewMenu);
    viewersViewMenu->setTitle(_gui->tr("Display view number"));
}
void Gui::setupUi()
{
    
    setMouseTracking(true);
    installEventFilter(this);
    QDesktopWidget* desktop = QApplication::desktop();
    QRect screen = desktop->screenGeometry();
    assert(!isFullScreen());
    resize((int)(0.93*screen.width()),(int)(0.93*screen.height())); // leave some space
    assert(!isDockNestingEnabled()); // should be false by default
    
    
    loadStyleSheet();
    
    
    _imp->_undoStacksGroup = new QUndoGroup;
    QObject::connect(_imp->_undoStacksGroup, SIGNAL(activeStackChanged(QUndoStack*)), this, SLOT(onCurrentUndoStackChanged(QUndoStack*)));
    
    /*TOOL BAR menus*/
    //======================
    _imp->menubar = new QMenuBar(this);
    _imp->menuFile = new QMenu(_imp->menubar);
    _imp->menuRecentFiles = new QMenu(_imp->menuFile);
    _imp->menuEdit = new QMenu(_imp->menubar);
    _imp->menuDisplay = new QMenu(_imp->menubar);
    _imp->menuOptions = new QMenu(_imp->menubar);
    _imp->viewersMenu= new QMenu(_imp->menuDisplay);
    _imp->viewerInputsMenu = new QMenu(_imp->viewersMenu);
    _imp->viewersViewMenu = new QMenu(_imp->viewersMenu);
    _imp->cacheMenu= new QMenu(_imp->menubar);
    
    setMenuBar(_imp->menubar);

    _imp->actionNew_project = new QAction(this);
    _imp->actionNew_project->setObjectName(QString::fromUtf8("actionNew_project"));
    _imp->actionNew_project->setShortcut(QKeySequence::New);
    _imp->actionNew_project->setIcon(get_icon("document-new"));
    QObject::connect(_imp->actionNew_project, SIGNAL(triggered()), this, SLOT(newProject()));
    _imp->actionOpen_project = new QAction(this);
    _imp->actionOpen_project->setObjectName(QString::fromUtf8("actionOpen_project"));
    _imp->actionOpen_project->setShortcut(QKeySequence::Open);
    _imp->actionOpen_project->setIcon(get_icon("document-open"));
    QObject::connect(_imp->actionOpen_project, SIGNAL(triggered()), this, SLOT(openProject()));
    _imp->actionSave_project = new QAction(this);
    _imp->actionSave_project->setObjectName(QString::fromUtf8("actionSave_project"));
    _imp->actionSave_project->setShortcut(QKeySequence::Save);
    _imp->actionSave_project->setIcon(get_icon("document-save"));
    QObject::connect(_imp->actionSave_project, SIGNAL(triggered()), this, SLOT(saveProject()));
    _imp->actionSaveAs_project = new QAction(this);
    _imp->actionSaveAs_project->setObjectName(QString::fromUtf8("actionSaveAs_project"));
    _imp->actionSaveAs_project->setShortcut(QKeySequence::SaveAs);
    _imp->actionSaveAs_project->setIcon(get_icon("document-save-as"));
    QObject::connect(_imp->actionSaveAs_project, SIGNAL(triggered()), this, SLOT(saveProjectAs()));
    _imp->actionPreferences = new QAction(this);
    _imp->actionPreferences->setObjectName(QString::fromUtf8("actionPreferences"));
    _imp->actionPreferences->setMenuRole(QAction::PreferencesRole);
    _imp->actionExit = new QAction(this);
    _imp->actionExit->setObjectName(QString::fromUtf8("actionExit"));
    _imp->actionExit->setMenuRole(QAction::QuitRole);
    _imp->actionExit->setIcon(get_icon("application-exit"));
    _imp->actionProject_settings = new QAction(this);
    _imp->actionProject_settings->setObjectName(QString::fromUtf8("actionProject_settings"));
    _imp->actionProject_settings->setIcon(get_icon("document-properties"));
    _imp->actionProject_settings->setShortcut(QKeySequence(Qt::Key_S));
    _imp->actionFullScreen = new QAction(this);
    _imp->actionFullScreen->setObjectName(QString::fromUtf8("actionFullScreen"));
    _imp->actionFullScreen->setShortcut(QKeySequence(Qt::CTRL+Qt::META+Qt::Key_F));
    _imp->actionFullScreen->setIcon(get_icon("view-fullscreen"));
    _imp->actionClearDiskCache = new QAction(this);
    _imp->actionClearDiskCache->setObjectName(QString::fromUtf8("actionClearDiskCache"));
    _imp->actionClearDiskCache->setCheckable(false);
    _imp->actionClearPlayBackCache = new QAction(this);
    _imp->actionClearPlayBackCache->setObjectName(QString::fromUtf8("actionClearPlayBackCache"));
    _imp->actionClearPlayBackCache->setCheckable(false);
    _imp->actionClearNodeCache = new QAction(this);
    _imp->actionClearNodeCache->setObjectName(QString::fromUtf8("actionClearNodeCache"));
    _imp->actionClearNodeCache->setCheckable(false);
    _imp->actionClearPluginsLoadingCache = new QAction(this);
    _imp->actionClearPluginsLoadingCache->setObjectName(QString::fromUtf8("actionClearPluginsLoadedCache"));
    _imp->actionClearPluginsLoadingCache->setCheckable(false);
    _imp->actionClearAllCaches = new QAction(this);
    _imp->actionClearAllCaches->setObjectName(QString::fromUtf8("actionClearAllCaches"));
    _imp->actionClearAllCaches->setCheckable(false);
    _imp->actionClearAllCaches->setShortcut(QKeySequence(Qt::CTRL+Qt::SHIFT+Qt::Key_K));
    _imp->actionShowAboutWindow = new QAction(this);
    _imp->actionShowAboutWindow->setObjectName(QString::fromUtf8("actionShowAboutWindow"));
    _imp->actionShowAboutWindow->setCheckable(false);
    for (int c = 0; c < NATRON_MAX_RECENT_FILES; ++c) {
        _imp->actionsOpenRecentFile[c] = new QAction(this);
        _imp->actionsOpenRecentFile[c]->setVisible(false);
        connect(_imp->actionsOpenRecentFile[c], SIGNAL(triggered()),this, SLOT(openRecentFile()));
    }
    
    _imp->actionConnectInput1 = new QAction(this);
    _imp->actionConnectInput1->setCheckable(false);
    _imp->actionConnectInput1->setShortcut(QKeySequence(Qt::Key_1));
    
    _imp->actionConnectInput2 = new QAction(this);
    _imp->actionConnectInput2->setCheckable(false);
    _imp->actionConnectInput2->setShortcut(QKeySequence(Qt::Key_2));
    
    _imp->actionConnectInput3 = new QAction(this);
    _imp->actionConnectInput3->setCheckable(false);
    _imp->actionConnectInput3->setShortcut(QKeySequence(Qt::Key_3));
    
    _imp->actionConnectInput4 = new QAction(this);
    _imp->actionConnectInput4->setCheckable(false);
    _imp->actionConnectInput4->setShortcut(QKeySequence(Qt::Key_4));
    
    _imp->actionConnectInput5 = new QAction(this);
    _imp->actionConnectInput5->setCheckable(false);
    _imp->actionConnectInput5->setShortcut(QKeySequence(Qt::Key_5));
    
    _imp->actionConnectInput6 = new QAction(this);
    _imp->actionConnectInput6->setCheckable(false);
    _imp->actionConnectInput6->setShortcut(QKeySequence(Qt::Key_6));
    
    _imp->actionConnectInput7 = new QAction(this);
    _imp->actionConnectInput7->setCheckable(false);
    _imp->actionConnectInput7->setShortcut(QKeySequence(Qt::Key_7));
    
    _imp->actionConnectInput8 = new QAction(this);
    _imp->actionConnectInput8->setCheckable(false);
    _imp->actionConnectInput8->setShortcut(QKeySequence(Qt::Key_8));
    
    _imp->actionConnectInput9 = new QAction(this);
    _imp->actionConnectInput9->setCheckable(false);
    _imp->actionConnectInput9->setShortcut(QKeySequence(Qt::Key_9));
    
    _imp->actionConnectInput10 = new QAction(this);
    _imp->actionConnectInput10->setCheckable(false);
    _imp->actionConnectInput10->setShortcut(QKeySequence(Qt::Key_0));

    
    /*CENTRAL AREA*/
    //======================
    _imp->_centralWidget = new QWidget(this);
    setCentralWidget(_imp->_centralWidget);
    _imp->_mainLayout = new QHBoxLayout(_imp->_centralWidget);
    _imp->_mainLayout->setContentsMargins(0, 0, 0, 0);
    _imp->_centralWidget->setLayout(_imp->_mainLayout);
    
    _imp->_leftRightSplitter = new QSplitter(_imp->_centralWidget);
    _imp->_leftRightSplitter->setObjectName("ToolBar_splitter");
    _imp->_splitters.push_back(_imp->_leftRightSplitter);
    _imp->_leftRightSplitter->setChildrenCollapsible(false);
    _imp->_leftRightSplitter->setOrientation(Qt::Horizontal);
    _imp->_leftRightSplitter->setContentsMargins(0, 0, 0, 0);
    
    
    _imp->_toolBox = new QToolBar(_imp->_leftRightSplitter);
    _imp->_toolBox->setOrientation(Qt::Vertical);
    _imp->_toolBox->setMaximumWidth(40);
    
    _imp->_leftRightSplitter->addWidget(_imp->_toolBox);
    
    _imp->_viewerWorkshopSplitter = new QSplitter(_imp->_centralWidget);
    _imp->_viewerWorkshopSplitter->setObjectName("Viewers_Workshop_splitter");
    _imp->_splitters.push_back(_imp->_viewerWorkshopSplitter);
    _imp->_viewerWorkshopSplitter->setContentsMargins(0, 0, 0, 0);
    _imp->_viewerWorkshopSplitter->setOrientation(Qt::Vertical);
    _imp->_viewerWorkshopSplitter->setChildrenCollapsible(false);;
    
    /*VIEWERS related*/
    
    _imp->_viewersPane = new TabWidget(this,TabWidget::NOT_CLOSABLE,_imp->_viewerWorkshopSplitter);
    _imp->_viewersPane->setObjectName("ViewerPane");
    _imp->_panes.push_back(_imp->_viewersPane);
    _imp->_viewersPane->resize(_imp->_viewersPane->width(), this->height()/5);
    _imp->_viewerWorkshopSplitter->addWidget(_imp->_viewersPane);
    
    /*WORKSHOP PANE*/
    //======================
    _imp->_workshopPane = new TabWidget(this,TabWidget::NOT_CLOSABLE,_imp->_viewerWorkshopSplitter);
    _imp->_workshopPane->setObjectName("WorkshopPane");
    _imp->_panes.push_back(_imp->_workshopPane);
    
    _imp->_graphScene = new QGraphicsScene(this);
    _imp->_graphScene->setItemIndexMethod(QGraphicsScene::NoIndex);
    _imp->_nodeGraphArea = new NodeGraph(this,_imp->_graphScene,_imp->_workshopPane);
    _imp->_nodeGraphArea->setObjectName(kNodeGraphObjectName);
    _imp->_workshopPane->appendTab(_imp->_nodeGraphArea);
    
    _imp->_curveEditor = new CurveEditor(_imp->_appInstance->getTimeLine(),this);

    _imp->_curveEditor->setObjectName(kCurveEditorObjectName);
    _imp->_workshopPane->appendTab(_imp->_curveEditor);
    
    _imp->_workshopPane->makeCurrentTab(0);
    
    _imp->_viewerWorkshopSplitter->addWidget(_imp->_workshopPane);
    
    ///if the preferences are not set, give a "basic" shape to the splitter so it doesn't look collapsed
    QList<int> sizesViewerSplitter;
    sizesViewerSplitter << 500;
    sizesViewerSplitter << 500;
    _imp->_viewerWorkshopSplitter->setSizes(sizesViewerSplitter);
    

    _imp->_middleRightSplitter = new QSplitter(_imp->_centralWidget);
    _imp->_middleRightSplitter->setObjectName("Center_PropertiesBin_splitter");
    _imp->_splitters.push_back(_imp->_middleRightSplitter);
    _imp->_middleRightSplitter->setChildrenCollapsible(false);
    _imp->_middleRightSplitter->setContentsMargins(0, 0, 0, 0);
    _imp->_middleRightSplitter->setOrientation(Qt::Horizontal);
    _imp->_middleRightSplitter->addWidget(_imp->_viewerWorkshopSplitter);
    
    /*PROPERTIES DOCK*/
    //======================
    _imp->_propertiesPane = new TabWidget(this,TabWidget::NOT_CLOSABLE,this);
    _imp->_propertiesPane->setObjectName("PropertiesPane");
    _imp->_panes.push_back(_imp->_propertiesPane);

    _imp->_propertiesScrollArea = new QScrollArea(_imp->_propertiesPane);
    _imp->_nodeGraphArea->setPropertyBinPtr(_imp->_propertiesScrollArea);
    _imp->_propertiesScrollArea->setObjectName("Properties");

    _imp->_propertiesContainer=new QWidget(_imp->_propertiesScrollArea);
    _imp->_propertiesContainer->setObjectName("_propertiesContainer");
    _imp->_layoutPropertiesBin=new QVBoxLayout(_imp->_propertiesContainer);
    _imp->_layoutPropertiesBin->setSpacing(0);
    _imp->_layoutPropertiesBin->setContentsMargins(0, 0, 0, 0);
    _imp->_propertiesContainer->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Fixed);
    _imp->_propertiesContainer->setLayout(_imp->_layoutPropertiesBin);
    _imp->_propertiesScrollArea->setWidget(_imp->_propertiesContainer);
    _imp->_propertiesScrollArea->setWidgetResizable(true);

    _imp->_propertiesPane->appendTab(_imp->_propertiesScrollArea);
    
    _imp->_middleRightSplitter->addWidget(_imp->_propertiesPane);
    QList<int> sizesMiddleRightSplitter;
    sizesMiddleRightSplitter << 800;
    sizesMiddleRightSplitter << 300;
    _imp->_middleRightSplitter->setSizes(sizesMiddleRightSplitter);

    
    _imp->_leftRightSplitter->addWidget(_imp->_middleRightSplitter);
    
    _imp->_mainLayout->addWidget(_imp->_leftRightSplitter);


    _imp->_projectGui = new ProjectGui(this);
    _imp->_projectGui->create(_imp->_appInstance->getProject(),
                        _imp->_layoutPropertiesBin,
                        _imp->_propertiesContainer);
    
    initProjectGuiKnobs(); 
    
    _imp->_settingsGui = new PreferencesPanel(appPTR->getCurrentSettings(),this);
    _imp->_settingsGui->hide();

    setVisibleProjectSettingsPanel();
    
    _imp->_aboutWindow = new AboutWindow(this);
    _imp->_aboutWindow->hide();
    
    _imp->menubar->addAction(_imp->menuFile->menuAction());
    _imp->menubar->addAction(_imp->menuEdit->menuAction());
    _imp->menubar->addAction(_imp->menuDisplay->menuAction());
    _imp->menubar->addAction(_imp->menuOptions->menuAction());
    _imp->menubar->addAction(_imp->cacheMenu->menuAction());
    _imp->menuFile->addAction(_imp->actionShowAboutWindow);
    _imp->menuFile->addAction(_imp->actionNew_project);
    _imp->menuFile->addAction(_imp->actionOpen_project);
    _imp->menuFile->addAction(_imp->menuRecentFiles->menuAction());
    updateRecentFileActions();
    for (int c = 0; c < NATRON_MAX_RECENT_FILES; ++c) {
        _imp->menuRecentFiles->addAction(_imp->actionsOpenRecentFile[c]);
    }

    _imp->menuFile->addAction(_imp->actionSave_project);
    _imp->menuFile->addAction(_imp->actionSaveAs_project);
    _imp->menuFile->addSeparator();
    _imp->menuFile->addAction(_imp->actionExit);
    
    _imp->menuEdit->addAction(_imp->actionPreferences);

    _imp->menuOptions->addAction(_imp->actionProject_settings);
    _imp->menuDisplay->addAction(_imp->viewersMenu->menuAction());
    _imp->viewersMenu->addAction(_imp->viewerInputsMenu->menuAction());
    _imp->viewersMenu->addAction(_imp->viewersViewMenu->menuAction());
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
    
    _imp->cacheMenu->addAction(_imp->actionClearDiskCache);
    _imp->cacheMenu->addAction(_imp->actionClearPlayBackCache);
    _imp->cacheMenu->addAction(_imp->actionClearNodeCache);
    _imp->cacheMenu->addAction(_imp->actionClearPluginsLoadingCache);
    _imp->cacheMenu->addAction(_imp->actionClearAllCaches);
    _imp->retranslateUi(this);
    
    
    QObject::connect(_imp->actionShowAboutWindow,SIGNAL(triggered()),this,SLOT(showAbout()));
    QObject::connect(_imp->actionFullScreen, SIGNAL(triggered()),this,SLOT(toggleFullScreen()));
    QObject::connect(_imp->actionClearDiskCache, SIGNAL(triggered()),appPTR,SLOT(clearDiskCache()));
    QObject::connect(_imp->actionClearPlayBackCache, SIGNAL(triggered()),appPTR,SLOT(clearPlaybackCache()));
    QObject::connect(_imp->actionClearNodeCache, SIGNAL(triggered()),appPTR,SLOT(clearNodeCache()));
    QObject::connect(_imp->actionClearPluginsLoadingCache, SIGNAL(triggered()),appPTR,SLOT(clearPluginsLoadedCache()));
    QObject::connect(_imp->actionClearAllCaches, SIGNAL(triggered()),appPTR,SLOT(clearAllCaches()));

    
    //the same action also clears the ofx plugins caches, they are not the same cache but are used to the same end
    QObject::connect(_imp->actionClearNodeCache, SIGNAL(triggered()),_imp->_appInstance,SLOT(clearOpenFXPluginsCaches()));
    QObject::connect(_imp->actionExit,SIGNAL(triggered()),this,SLOT(exitGui()));
    QObject::connect(_imp->actionProject_settings,SIGNAL(triggered()),this,SLOT(setVisibleProjectSettingsPanel()));
    
    QObject::connect(_imp->actionConnectInput1, SIGNAL(triggered()),this,SLOT(connectInput1()));
    QObject::connect(_imp->actionConnectInput2, SIGNAL(triggered()),this,SLOT(connectInput2()));
    QObject::connect(_imp->actionConnectInput3, SIGNAL(triggered()),this,SLOT(connectInput3()));
    QObject::connect(_imp->actionConnectInput4, SIGNAL(triggered()),this,SLOT(connectInput4()));
    QObject::connect(_imp->actionConnectInput5, SIGNAL(triggered()),this,SLOT(connectInput5()));
    QObject::connect(_imp->actionConnectInput6, SIGNAL(triggered()),this,SLOT(connectInput6()));
    QObject::connect(_imp->actionConnectInput7, SIGNAL(triggered()),this,SLOT(connectInput7()));
    QObject::connect(_imp->actionConnectInput8, SIGNAL(triggered()),this,SLOT(connectInput8()));
    QObject::connect(_imp->actionConnectInput9, SIGNAL(triggered()),this,SLOT(connectInput9()));
    QObject::connect(_imp->actionConnectInput10, SIGNAL(triggered()),this,SLOT(connectInput10()));
    
    QObject::connect(_imp->actionPreferences,SIGNAL(triggered()),this,SLOT(showSettings()));
    QObject::connect(_imp->_appInstance->getProject().get(),SIGNAL(projectNameChanged(QString)),this,SLOT(onProjectNameChanged(QString)));
    QMetaObject::connectSlotsByName(this);
    
    _imp->restoreGuiGeometry();
    
    
} // setupUi

void Gui::initProjectGuiKnobs() {
    assert(_imp->_projectGui);
    _imp->_projectGui->initializeKnobsGui();
}

QKeySequence Gui::keySequenceForView(int v){
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

static const char* slotForView(int view){
    switch (view){
    case 0:
        return SLOT(showView0());
        break;
    case 1:
        return SLOT(showView1());
        break;
    case 2:
        return SLOT(showView2());
        break;
    case 3:
        return SLOT(showView3());
        break;
    case 4:
        return SLOT(showView4());
        break;
    case 5:
        return SLOT(showView5());
        break;
    case 6:
        return SLOT(showView6());
        break;
    case 7:
        return SLOT(showView7());
        break;
    case 8:
        return SLOT(showView7());
        break;
    case 9:
        return SLOT(showView8());
        break;
    default:
        return NULL;
    }
}

void Gui::updateViewsActions(int viewsCount){
    _imp->viewersViewMenu->clear();
    //if viewsCount == 1 we don't add a menu entry
    _imp->viewersMenu->removeAction(_imp->viewersViewMenu->menuAction());
    if (viewsCount == 2) {
        QAction* left = new QAction(this);
        left->setCheckable(false);
        left->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_1));
        _imp->viewersViewMenu->addAction(left);
        left->setText("Display left view");
        QObject::connect(left,SIGNAL(triggered()),this,SLOT(showView0()));
        
        QAction* right = new QAction(this);
        right->setCheckable(false);
        right->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_2));
        _imp->viewersViewMenu->addAction(right);
        right->setText("Display right view");
        QObject::connect(right,SIGNAL(triggered()),this,SLOT(showView1()));
        
        _imp->viewersMenu->addAction(_imp->viewersViewMenu->menuAction());
    } else if (viewsCount > 2) {
        for(int i = 0; i < viewsCount;++i){
            if(i > 9)
                break;
            QAction* viewI = new QAction(this);
            viewI->setCheckable(false);
            QKeySequence seq = keySequenceForView(i);
            if(!seq.isEmpty()){
                viewI->setShortcut(seq);
            }
            _imp->viewersViewMenu->addAction(viewI);
            const char* slot = slotForView(i);
            viewI->setText(QString("Display view ")+QString::number(i+1));
            if(slot){
                QObject::connect(viewI,SIGNAL(triggered()),this,slot);
            }
        }
        
        _imp->viewersMenu->addAction(_imp->viewersViewMenu->menuAction());
    }
}


void Gui::putSettingsPanelFirst(DockablePanel* panel){
    _imp->_layoutPropertiesBin->removeWidget(panel);
    _imp->_layoutPropertiesBin->insertWidget(0, panel);
    _imp->_propertiesScrollArea->verticalScrollBar()->setValue(0);
}

void Gui::setVisibleProjectSettingsPanel() {
    putSettingsPanelFirst(_imp->_projectGui->getPanel());
    if(!_imp->_projectGui->isVisible()){
        _imp->_projectGui->setVisible(true);
    }
}
void Gui::loadStyleSheet(){
    QFile qss(":/Resources/Stylesheets/mainstyle.qss");
    if(qss.open(QIODevice::ReadOnly
                | QIODevice::Text))
    {
        QTextStream in(&qss);
        QString content(in.readAll());
        setStyleSheet(content
                      .arg("rgb(243,149,0)") // selection-color
                      .arg("rgb(50,50,50)") // medium background
                      .arg("rgb(71,71,71)") // soft background
                      .arg("rgb(38,38,38)") // strong background
                      .arg("rgb(200,200,200)") // text colour
                      .arg("rgb(86,117,156)") // interpolated value color
                      .arg("rgb(21,97,248)") // keyframe value color
                      .arg("rgb(0,0,0)")); // disabled editable text
    }
}

void Gui::maximize(TabWidget* what) {
    assert(what);
    if(what->isFloating())
        return;
    for (std::list<TabWidget*>::iterator it = _imp->_panes.begin(); it != _imp->_panes.end(); ++it) {
        
        //if the widget is not what we want to maximize and it is not floating , hide it
        if (*it != what && !(*it)->isFloating()) {
            
            // also if we want to maximize the workshop pane, don't hide the properties pane
            if((*it) == _imp->_propertiesPane && what == _imp->_workshopPane){
                continue;
            }
            (*it)->hide();
        }
    }
}

void Gui::minimize(){
    for (std::list<TabWidget*>::iterator it = _imp->_panes.begin(); it != _imp->_panes.end(); ++it) {
        (*it)->show();
    }
}


ViewerTab* Gui::addNewViewerTab(ViewerInstance* viewer,TabWidget* where){
    ViewerTab* tab = new ViewerTab(this,viewer,_imp->_viewersPane);
    _imp->_viewerTabs.push_back(tab);
    where->appendTab(tab);
    return tab;
}

void Gui::addViewerTab(ViewerTab* tab, TabWidget* where) {
    assert(tab);
    assert(where);
    std::list<ViewerTab*>::iterator it = std::find(_imp->_viewerTabs.begin(), _imp->_viewerTabs.end(), tab);
    if (it == _imp->_viewerTabs.end()) {
        _imp->_viewerTabs.push_back(tab);
    }
    where->appendTab(tab);
    
}

void Gui::registerTab(QWidget* tab) {
    std::map<std::string, QWidget*>::iterator registeredTab = _imp->_registeredTabs.find(tab->objectName().toStdString());
    if (registeredTab == _imp->_registeredTabs.end()) {
        _imp->_registeredTabs.insert(std::make_pair(tab->objectName().toStdString(), tab));
    }
}

void Gui::unregisterTab(QWidget* tab) {
    std::map<std::string, QWidget*>::iterator registeredTab = _imp->_registeredTabs.find(tab->objectName().toStdString());
    if(registeredTab != _imp->_registeredTabs.end()){
        _imp->_registeredTabs.erase(registeredTab);
    }
}

void Gui::removeViewerTab(ViewerTab* tab,bool initiatedFromNode,bool deleteData){
    assert(tab);
    tab->hide();
    
    unregisterTab(tab);
    
    if(deleteData){
        
        std::list<ViewerTab*>::iterator it = std::find(_imp->_viewerTabs.begin(), _imp->_viewerTabs.end(), tab);
        if (it != _imp->_viewerTabs.end()) {
            _imp->_viewerTabs.erase(it);
        }

        
        if (!initiatedFromNode) {
            assert(_imp->_nodeGraphArea);
            tab->getInternalNode()->getNode()->deactivate();
        } else {
            
            TabWidget* container = dynamic_cast<TabWidget*>(tab->parentWidget());
            if(container)
                container->removeTab(tab);
            delete tab;
        }

    } else {
        TabWidget* container = dynamic_cast<TabWidget*>(tab->parentWidget());
        if(container)
            container->removeTab(tab);
    }
    
}

void Gui::removePane(TabWidget* pane){
    std::list<TabWidget*>::iterator found = std::find(_imp->_panes.begin(), _imp->_panes.end(), pane);
    assert(found != _imp->_panes.end());
    _imp->_panes.erase(found);
}

void Gui::registerPane(TabWidget* pane){
    std::list<TabWidget*>::iterator found = std::find(_imp->_panes.begin(), _imp->_panes.end(), pane);
    if(found == _imp->_panes.end()){
        _imp->_panes.push_back(pane);
    }
}

void Gui::registerSplitter(QSplitter* s){
    std::list<QSplitter*>::iterator found = std::find(_imp->_splitters.begin(), _imp->_splitters.end(), s);
    if(found == _imp->_splitters.end()){
        _imp->_splitters.push_back(s);
    }
}

void Gui::removeSplitter(QSplitter* s){
    std::list<QSplitter*>::iterator found = std::find(_imp->_splitters.begin(), _imp->_splitters.end(), s);
    if(found != _imp->_splitters.end()){
        _imp->_splitters.erase(found);
    }
}


QWidget* Gui::findExistingTab(const std::string& name) const{
    std::map<std::string,QWidget*>::const_iterator it = _imp->_registeredTabs.find(name);
    if (it != _imp->_registeredTabs.end()) {
        return it->second;
    }else{
        return NULL;
    }
}
ToolButton* Gui::findExistingToolButton(const QString& label) const{
    for(U32 i = 0; i < _imp->_toolButtons.size();++i){
        if(_imp->_toolButtons[i]->getLabel() == label){
            return _imp->_toolButtons[i];
        }
    }
    return NULL;
}

static void getPixmapForGrouping(QPixmap* pixmap,const QString& grouping) {
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
    } else if (grouping == PLUGIN_GROUP_DEFAULT) {
        appPTR->getIcon(Natron::NATRON_PIXMAP_MISC_GROUPING, pixmap);
    } else {
        appPTR->getIcon(Natron::NATRON_PIXMAP_MISC_GROUPING, pixmap);
    }
}

ToolButton* Gui::findOrCreateToolButton(PluginGroupNode* plugin){
    for(U32 i = 0; i < _imp->_toolButtons.size();++i){
        if(_imp->_toolButtons[i]->getID() == plugin->getID()){
            return _imp->_toolButtons[i];
        }
    }
    
    //first-off create the tool-button's parent, if any
    ToolButton* parentToolButton = NULL;
    if(plugin->hasParent())
        parentToolButton = findOrCreateToolButton(plugin->getParent());
    
    QIcon icon;
    if(!plugin->getIconPath().isEmpty() && QFile::exists(plugin->getIconPath())){
        icon.addFile(plugin->getIconPath());
    }else{
        //add the default group icon only if it has no parent
        if(!plugin->hasParent()){
            QPixmap pix;
            getPixmapForGrouping(&pix, plugin->getLabel());
            icon.addPixmap(pix);
        }
    }
    //if the tool-button has no children, this is a leaf, we must create an action
    bool isLeaf = false;
    if(plugin->getChildren().empty()){
        isLeaf = true;
        //if the plugin has no children and no parent, put it in the "others" group
        if(!plugin->hasParent()){
            ToolButton* othersGroup = findExistingToolButton(PLUGIN_GROUP_DEFAULT);
            PluginGroupNode* othersToolButton = appPTR->findPluginToolButtonOrCreate(PLUGIN_GROUP_DEFAULT,PLUGIN_GROUP_DEFAULT, PLUGIN_GROUP_DEFAULT_ICON_PATH);
            othersToolButton->tryAddChild(plugin);
            
            //if the othersGroup doesn't exist, create it
            if(!othersGroup){
                othersGroup = findOrCreateToolButton(othersToolButton);
            }
            parentToolButton = othersGroup;
        }
    }
    ToolButton* pluginsToolButton = new ToolButton(_imp->_appInstance,plugin,plugin->getID(),plugin->getLabel(),icon);
    if(isLeaf){
        assert(parentToolButton);
        QAction* action = new QAction(this);
        action->setText(pluginsToolButton->getLabel());
        action->setIcon(pluginsToolButton->getIcon());
        QObject::connect(action , SIGNAL(triggered()), pluginsToolButton, SLOT(onTriggered()));
        pluginsToolButton->setAction(action);
    }else{
        QMenu* menu = new QMenu(this);
        menu->setTitle(pluginsToolButton->getLabel());
        pluginsToolButton->setMenu(menu);
        pluginsToolButton->setAction(menu->menuAction());
    }
    //if it has a parent, add the new tool button as a child
    if(parentToolButton){
        parentToolButton->tryAddChild(pluginsToolButton);
    }
    _imp->_toolButtons.push_back(pluginsToolButton);
    return pluginsToolButton;
}

void Gui::addToolButttonsToToolBar(){
    for (U32 i = 0; i < _imp->_toolButtons.size(); ++i) {
        
        //if the toolbutton is a root (no parent), add it in the toolbox
        if(_imp->_toolButtons[i]->hasChildren() && !_imp->_toolButtons[i]->getPluginToolButton()->hasParent()){
            QToolButton* button = new QToolButton(_imp->_toolBox);
            button->setIcon(_imp->_toolButtons[i]->getIcon());
            button->setMenu(_imp->_toolButtons[i]->getMenu());
            button->setPopupMode(QToolButton::InstantPopup);
            button->setToolTip(Qt::convertFromPlainText(_imp->_toolButtons[i]->getLabel(), Qt::WhiteSpaceNormal));
            _imp->_toolBox->addWidget(button);
        }
    }
}

void GuiPrivate::setUndoRedoActions(QAction* undoAction,QAction* redoAction){
    if(_currentUndoAction){
        menuEdit->removeAction(_currentUndoAction);
    }
    if(_currentRedoAction){
        menuEdit->removeAction(_currentRedoAction);
    }
    _currentUndoAction = undoAction;
    _currentRedoAction = redoAction;
    menuEdit->addAction(undoAction);
    menuEdit->addAction(redoAction);
}
void Gui::newProject() {
    appPTR->newAppInstance();
}
void Gui::openProject() {
    std::vector<std::string> filters;
    filters.push_back(NATRON_PROJECT_FILE_EXT);
    QStringList selectedFiles =  popOpenFileDialog(false, filters, _imp->_lastLoadProjectOpenedDir.toStdString());
    
    if (selectedFiles.size() > 0) {
        //clearing current graph
        QString file = selectedFiles.at(0);
        QString path = SequenceParsing::removePath(file);
        
        ///if the current graph has no value, just load the project in the same window
        if (_imp->_appInstance->getProject()->isGraphWorthLess()) {
            _imp->_appInstance->getProject()->loadProject(path, file);
        } else {
            AppInstance* newApp = appPTR->newAppInstance();
            newApp->getProject()->loadProject(path, file);
        }
        
        QSettings settings;
        QStringList recentFiles = settings.value("recentFileList").toStringList();
        recentFiles.removeAll(file);
        recentFiles.prepend(file);
        while (recentFiles.size() > NATRON_MAX_RECENT_FILES)
            recentFiles.removeLast();
        
        settings.setValue("recentFileList", recentFiles);
        appPTR->updateAllRecentFileMenus();
    }
    
}
bool Gui::saveProject(){
    
    if(_imp->_appInstance->getProject()->hasProjectBeenSavedByUser()){
        _imp->_appInstance->getProject()->saveProject(_imp->_appInstance->getProject()->getProjectPath(),
                                                _imp->_appInstance->getProject()->getProjectName(),false);
        ///update the open recents
        QString file = _imp->_appInstance->getProject()->getProjectPath() + _imp->_appInstance->getProject()->getProjectName();
        QSettings settings;
        QStringList recentFiles = settings.value("recentFileList").toStringList();
        recentFiles.removeAll(file);
        recentFiles.prepend(file);
        while (recentFiles.size() > NATRON_MAX_RECENT_FILES)
            recentFiles.removeLast();
        
        settings.setValue("recentFileList", recentFiles);
        appPTR->updateAllRecentFileMenus();
        return true;
    }else{
        return saveProjectAs();
    }
    
}
bool Gui::saveProjectAs(){
    std::vector<std::string> filter;
    filter.push_back(NATRON_PROJECT_FILE_EXT);
    QString outFile = popSaveFileDialog(false, filter,_imp->_lastSaveProjectOpenedDir.toStdString());
    if (outFile.size() > 0) {
        if (outFile.indexOf("." NATRON_PROJECT_FILE_EXT) == -1) {
            outFile.append("." NATRON_PROJECT_FILE_EXT);
        }
        QString path = SequenceParsing::removePath(outFile);
        _imp->_appInstance->getProject()->saveProject(path,outFile,false);
        
        QString filePath = path + outFile;
        QSettings settings;
        QStringList recentFiles = settings.value("recentFileList").toStringList();
        recentFiles.removeAll(filePath);
        recentFiles.prepend(filePath);
        while (recentFiles.size() > NATRON_MAX_RECENT_FILES)
            recentFiles.removeLast();
        
        settings.setValue("recentFileList", recentFiles);
        appPTR->updateAllRecentFileMenus();
        return true;
    }
    return false;
}

void Gui::createReader(){
    std::map<std::string,std::string> readersForFormat;
    appPTR->getCurrentSettings()->getFileFormatsForReadingAndReader(&readersForFormat);
    std::vector<std::string> filters;
    for (std::map<std::string,std::string>::const_iterator it = readersForFormat.begin(); it!=readersForFormat.end(); ++it) {
        filters.push_back(it->first);
    }
    QStringList files = popOpenFileDialog(true, filters, _imp->_lastLoadSequenceOpenedDir.toStdString());
    if(!files.isEmpty()){
        QString first = files.at(0);
        std::string ext = Natron::removeFileExtension(first).toLower().toStdString();

        std::map<std::string,std::string>::iterator found = readersForFormat.find(ext);
        if (found == readersForFormat.end()) {
            errorDialog("Reader", "No plugin capable of decoding " + ext + " was found.");
        } else {
            Node* n = _imp->_appInstance->createNode(found->second.c_str(),-1,-1,false,false);
            
            if (!n) {
                return;
            }
            const std::vector<boost::shared_ptr<Knob> >& knobs = n->getKnobs();
            for (U32 i = 0; i < knobs.size(); ++i) {
                if (knobs[i]->typeName() == File_Knob::typeNameStatic()) {
                    boost::shared_ptr<File_Knob> fk = boost::dynamic_pointer_cast<File_Knob>(knobs[i]);
                    assert(fk);
                    
                    if(!fk->isAnimationEnabled() && files.size() > 1){
                        errorDialog("Reader", "This plug-in doesn't support image sequences, please select only 1 file.");
                        break;
                    } else {
                        fk->setFiles(files);
                        
                        if (n->isPreviewEnabled()) {
                            n->computePreviewImage(_imp->_appInstance->getTimeLine()->currentFrame());
                        }
                        
                        break;
                    }
                
                }
            }
        }
    }
}

void Gui::createWriter(){
    std::map<std::string,std::string> writersForFormat;
    appPTR->getCurrentSettings()->getFileFormatsForWritingAndWriter(&writersForFormat);
    std::vector<std::string> filters;
    for (std::map<std::string,std::string>::const_iterator it = writersForFormat.begin(); it!=writersForFormat.end(); ++it) {
        filters.push_back(it->first);
    }
    QString file = popSaveFileDialog(true, filters, _imp->_lastSaveSequenceOpenedDir.toStdString());
    if(!file.isEmpty()){
        QString fileCpy = file;
        std::string ext = Natron::removeFileExtension(fileCpy).toStdString();
        
        std::map<std::string,std::string>::iterator found = writersForFormat.find(ext);
        if(found != writersForFormat.end()){
            Node* n = _imp->_appInstance->createNode(found->second.c_str(),-1,-1,false,false);
            if (!n) {
                return;
            }

            const std::vector<boost::shared_ptr<Knob> >& knobs = n->getKnobs();
            for (U32 i = 0; i < knobs.size(); ++i) {
                if (knobs[i]->typeName() == OutputFile_Knob::typeNameStatic()) {
                    boost::shared_ptr<OutputFile_Knob> fk = boost::dynamic_pointer_cast<OutputFile_Knob>(knobs[i]);
                    assert(fk);
                    if(fk->isOutputImageFile()){
                        fk->setValue<QString>(file);
                        break;
                    }
                }
            }
        }else{
            errorDialog("Writer", "No plugin capable of encoding " + ext + " was found.");
        }
        
    }
}

QStringList Gui::popOpenFileDialog(bool sequenceDialog,const std::vector<std::string>& initialfilters,const std::string& initialDir) {
    SequenceFileDialog dialog(this, initialfilters, sequenceDialog, SequenceFileDialog::OPEN_DIALOG, initialDir);
    if (dialog.exec()) {
        return dialog.selectedFiles();
    }else{
        return QStringList();
    }
}

QString Gui::popSaveFileDialog(bool sequenceDialog,const std::vector<std::string>& initialfilters,const std::string& initialDir) {
    SequenceFileDialog dialog(this,initialfilters,sequenceDialog,SequenceFileDialog::SAVE_DIALOG,initialDir);
    if(dialog.exec()){
        return dialog.filesToSave();
    }else{
        return "";
    }
}

void Gui::autoSave(){
    _imp->_appInstance->getProject()->autoSave();
}


int Gui::saveWarning(){
    
    if(!_imp->_appInstance->getProject()->isGraphWorthLess() && !_imp->_appInstance->getProject()->isSaveUpToDate()){
        Natron::StandardButton ret =  Natron::questionDialog(NATRON_APPLICATION_NAME,"Save changes to " +
                               _imp->_appInstance->getProject()->getProjectName().toStdString() + " ?",
                               Natron::StandardButtons(Natron::Save | Natron::Discard | Natron::Cancel),Natron::Save);
        if(ret == Natron::Escape || ret == Natron::Cancel){
            return 2;
        }else if(ret == Natron::Discard){
            return 1;
        }else{
            return 0;
        }
    }
    return -1;
    
}

void Gui::loadProjectGui(boost::archive::xml_iarchive& obj) const {
    assert(_imp->_projectGui);
    _imp->_projectGui->load(obj);
}

void Gui::saveProjectGui(boost::archive::xml_oarchive& archive) {
    assert(_imp->_projectGui);
    _imp->_projectGui->save(archive);
}

void Gui::errorDialog(const std::string& title,const std::string& text){
    Natron::StandardButtons buttons(Natron::Yes | Natron::No);
    if(QThread::currentThread() != QCoreApplication::instance()->thread()){
        QMutexLocker locker(&_imp->_uiUsingMainThreadMutex);
        _imp->_uiUsingMainThread = true;
        locker.unlock();
        emit doDialog(0,QString(title.c_str()),QString(text.c_str()),buttons,(int)Natron::Yes);
        locker.relock();
        while(_imp->_uiUsingMainThread){
            _imp->_uiUsingMainThreadCond.wait(&_imp->_uiUsingMainThreadMutex);
        }
    }else{
        emit doDialog(0,QString(title.c_str()),QString(text.c_str()),buttons,(int)Natron::Yes);
    }
}

void Gui::warningDialog(const std::string& title,const std::string& text){
    Natron::StandardButtons buttons(Natron::Yes | Natron::No);
    if(QThread::currentThread() != QCoreApplication::instance()->thread()){
        QMutexLocker locker(&_imp->_uiUsingMainThreadMutex);
        _imp->_uiUsingMainThread = true;
        locker.unlock();
        emit doDialog(1,QString(title.c_str()),QString(text.c_str()),buttons,(int)Natron::Yes);
        locker.relock();
        while(_imp->_uiUsingMainThread){
            _imp->_uiUsingMainThreadCond.wait(&_imp->_uiUsingMainThreadMutex);
        }
    }else{
        emit doDialog(1,QString(title.c_str()),QString(text.c_str()),buttons,(int)Natron::Yes);
    }
}

void Gui::informationDialog(const std::string& title,const std::string& text){
    Natron::StandardButtons buttons(Natron::Yes | Natron::No);
    if(QThread::currentThread() != QCoreApplication::instance()->thread()){
        QMutexLocker locker(&_imp->_uiUsingMainThreadMutex);
        _imp->_uiUsingMainThread = true;
        locker.unlock();
        emit doDialog(2,QString(title.c_str()),QString(text.c_str()),buttons,(int)Natron::Yes);
        locker.relock();
        while(_imp->_uiUsingMainThread){
            _imp->_uiUsingMainThreadCond.wait(&_imp->_uiUsingMainThreadMutex);
        }
    }else{
        emit doDialog(2,QString(title.c_str()),QString(text.c_str()),buttons,(int)Natron::Yes);
    }
}
void Gui::onDoDialog(int type, const QString& title, const QString& content, Natron::StandardButtons buttons, int defaultB)
{
    if(type == 0){
        QMessageBox critical(QMessageBox::Critical,title,content,QMessageBox::NoButton,this,Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowStaysOnTopHint);
        critical.exec();
    }else if(type == 1){
        QMessageBox warning(QMessageBox::Warning,title,content,QMessageBox::NoButton,this,Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowStaysOnTopHint);
        warning.exec();
    }else if(type == 2){
        QMessageBox info(QMessageBox::Information,title,content,QMessageBox::NoButton,this,Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowStaysOnTopHint);
        info.exec();
    }else{
        QMessageBox ques(QMessageBox::Question,title,content,QtEnumConvert::toQtStandarButtons(buttons),
                         this,Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowStaysOnTopHint);
        ques.setDefaultButton(QtEnumConvert::toQtStandardButton((Natron::StandardButton)defaultB));
        if (ques.exec()) {
            _imp->_lastQuestionDialogAnswer = QtEnumConvert::fromQtStandardButton(ques.standardButton(ques.clickedButton()));
        }
    }

    QMutexLocker locker(&_imp->_uiUsingMainThreadMutex);
    _imp->_uiUsingMainThread = false;
    _imp->_uiUsingMainThreadCond.wakeOne();
    
}

Natron::StandardButton Gui::questionDialog(const std::string& title,const std::string& message,Natron::StandardButtons buttons,
                                           Natron::StandardButton defaultButton) {
    if(QThread::currentThread() != QCoreApplication::instance()->thread()){
        QMutexLocker locker(&_imp->_uiUsingMainThreadMutex);
        _imp->_uiUsingMainThread = true;
        locker.unlock();
        emit doDialog(3,QString(title.c_str()),QString(message.c_str()),buttons,(int)defaultButton);
        locker.relock();
        while(_imp->_uiUsingMainThread){
            _imp->_uiUsingMainThreadCond.wait(&_imp->_uiUsingMainThreadMutex);
        }
    }else{
        emit doDialog(3,QString(title.c_str()),QString(message.c_str()),buttons,(int)defaultButton);
    }
    return _imp->_lastQuestionDialogAnswer;
}


void Gui::selectNode(NodeGui* node){
    _imp->_nodeGraphArea->selectNode(node);
}

void Gui::connectInput1(){
    _imp->_nodeGraphArea->connectCurrentViewerToSelection(0);
}
void Gui::connectInput2(){
    _imp->_nodeGraphArea->connectCurrentViewerToSelection(1);
}
void Gui::connectInput3(){
    _imp->_nodeGraphArea->connectCurrentViewerToSelection(2);
}
void Gui::connectInput4(){
    _imp->_nodeGraphArea->connectCurrentViewerToSelection(3);
}
void Gui::connectInput5(){
    _imp->_nodeGraphArea->connectCurrentViewerToSelection(4);
}
void Gui::connectInput6(){
    _imp->_nodeGraphArea->connectCurrentViewerToSelection(5);
}
void Gui::connectInput7(){
    _imp->_nodeGraphArea->connectCurrentViewerToSelection(6);
}
void Gui::connectInput8(){
    _imp->_nodeGraphArea->connectCurrentViewerToSelection(7);
}
void Gui::connectInput9(){
    _imp->_nodeGraphArea->connectCurrentViewerToSelection(8);
}
void Gui::connectInput10(){
    _imp->_nodeGraphArea->connectCurrentViewerToSelection(9);
}


void GuiPrivate::restoreGuiGeometry(){
    QSettings settings(NATRON_ORGANIZATION_NAME,NATRON_APPLICATION_NAME);
    settings.beginGroup("MainWindow");
    
    if(settings.contains("pos")){
        QPoint pos = settings.value("pos").toPoint();
        _gui->move(pos);
    }
    if(settings.contains("size")){
        QSize size = settings.value("size").toSize();
        _gui->resize(size);
    }
    if(settings.contains("fullScreen")){
        bool fs = settings.value("fullScreen").toBool();
        if(fs)
            _gui->toggleFullScreen();
    }
    
    if(settings.contains("splitters")){
        QByteArray splittersData = settings.value("splitters").toByteArray();
        QDataStream splittersStream(&splittersData, QIODevice::ReadOnly);
        if (splittersStream.atEnd())
            return;
        QByteArray leftRightSplitterState;
        QByteArray viewerWorkshopSplitterState;
        QByteArray middleRightSplitterState;
        
        splittersStream >> leftRightSplitterState
                        >> viewerWorkshopSplitterState
                        >> middleRightSplitterState;
        
        if(!_leftRightSplitter->restoreState(leftRightSplitterState))
            return;
        if(!_viewerWorkshopSplitter->restoreState(viewerWorkshopSplitterState))
            return;
        if(!_middleRightSplitter->restoreState(middleRightSplitterState))
            return;
    }
    
    settings.endGroup();
    if (settings.contains("LastOpenProjectDialogPath")) {
        _lastLoadSequenceOpenedDir = settings.value("LastOpenProjectDialogPath").toString();
    }
    if (settings.contains("LastSaveProjectDialogPath")) {
        _lastLoadSequenceOpenedDir = settings.value("LastSaveProjectDialogPath").toString();
    }
    if (settings.contains("LastLoadSequenceDialogPath")) {
        _lastLoadSequenceOpenedDir = settings.value("LastLoadSequenceDialogPath").toString();
    }
    if (settings.contains("LastSaveSequenceDialogPath")) {
        _lastLoadSequenceOpenedDir = settings.value("LastSaveSequenceDialogPath").toString();
    }
}

void GuiPrivate::saveGuiGeometry(){
    QSettings settings(NATRON_ORGANIZATION_NAME,NATRON_APPLICATION_NAME);
    
    settings.beginGroup("MainWindow");
    settings.setValue("pos", _gui->pos());
    settings.setValue("size", _gui->size());
    settings.setValue("fullScreen", _gui->isFullScreen());
    
    QByteArray splittersData;
    QDataStream splittersStream(&splittersData, QIODevice::WriteOnly);
    splittersStream << _leftRightSplitter->saveState();
    splittersStream << _viewerWorkshopSplitter->saveState();
    splittersStream << _middleRightSplitter->saveState();
    settings.setValue("splitters", splittersData);
    
    settings.endGroup();
    settings.setValue("LastOpenProjectDialogPath", _lastLoadProjectOpenedDir);
    settings.setValue("LastSaveProjectDialogPath", _lastSaveProjectOpenedDir);
    settings.setValue("LastLoadSequenceDialogPath", _lastLoadSequenceOpenedDir);
    settings.setValue("LastSaveSequenceDialogPath", _lastSaveSequenceOpenedDir);



}


void Gui::showView0(){
    _imp->_appInstance->setViewersCurrentView(0);
}
void Gui::showView1(){
    _imp->_appInstance->setViewersCurrentView(1);
}
void Gui::showView2(){
    _imp->_appInstance->setViewersCurrentView(2);
}
void Gui::showView3(){
    _imp->_appInstance->setViewersCurrentView(3);
}
void Gui::showView4(){
    _imp->_appInstance->setViewersCurrentView(4);
}
void Gui::showView5(){
    _imp->_appInstance->setViewersCurrentView(5);
}
void Gui::showView6(){
    _imp->_appInstance->setViewersCurrentView(6);
}
void Gui::showView7(){
    _imp->_appInstance->setViewersCurrentView(7);
}
void Gui::showView8(){
    _imp->_appInstance->setViewersCurrentView(8);
}
void Gui::showView9(){
    _imp->_appInstance->setViewersCurrentView(9);
}

void Gui::setCurveEditorOnTop(){
    for(std::list<TabWidget*>::iterator it = _imp->_panes.begin();it!=_imp->_panes.end();++it){
        TabWidget* cur = (*it);
        assert(cur);
        for(int i = 0; i < cur->count();++i){
            if(cur->tabAt(i) == _imp->_curveEditor){
                cur->makeCurrentTab(i);
                break;
            }
        }
    }
}

void Gui::showSettings(){
    _imp->_settingsGui->show();
}

void Gui::registerNewUndoStack(QUndoStack* stack){
    _imp->_undoStacksGroup->addStack(stack);
    QAction* undo = stack->createUndoAction(stack);
    undo->setShortcut(QKeySequence::Undo);
    QAction* redo = stack->createRedoAction(stack);
    redo->setShortcut(QKeySequence::Redo);
    _imp->_undoStacksActions.insert(std::make_pair(stack, std::make_pair(undo, redo)));
}

void Gui::removeUndoStack(QUndoStack* stack){
    std::map<QUndoStack*,std::pair<QAction*,QAction*> >::iterator it = _imp->_undoStacksActions.find(stack);
    if(it != _imp->_undoStacksActions.end()){
        _imp->_undoStacksActions.erase(it);
    }
}

void Gui::onCurrentUndoStackChanged(QUndoStack* stack){
    std::map<QUndoStack*,std::pair<QAction*,QAction*> >::iterator it = _imp->_undoStacksActions.find(stack);
    
    //the stack must have been registered first with registerNewUndoStack()
    if(it != _imp->_undoStacksActions.end()){
        _imp->setUndoRedoActions(it->second.first, it->second.second);
    }
}

void Gui::refreshAllPreviews() {
    int time = _imp->_appInstance->getTimeLine()->currentFrame();
    std::vector<Natron::Node*> nodes;
    _imp->_appInstance->getActiveNodes(&nodes);
    for (U32 i = 0; i < nodes.size(); ++i) {
        if (nodes[i]->isPreviewEnabled()) {
            nodes[i]->refreshPreviewImage(time);
        }
    }
}

void Gui::forceRefreshAllPreviews() {
    int time = _imp->_appInstance->getTimeLine()->currentFrame();
    std::vector<Natron::Node*> nodes;
    _imp->_appInstance->getActiveNodes(&nodes);
    for (U32 i = 0; i < nodes.size(); ++i) {
        if (nodes[i]->isPreviewEnabled()) {
            nodes[i]->computePreviewImage(time);
        }
    }
}

void Gui::startDragPanel(QWidget* panel) {
    assert(!_imp->_currentlyDraggedPanel);
    _imp->_currentlyDraggedPanel = panel;
}

QWidget* Gui::stopDragPanel() {
    assert(_imp->_currentlyDraggedPanel);
    QWidget* ret = _imp->_currentlyDraggedPanel;
    _imp->_currentlyDraggedPanel = 0;
    return ret;
}


void Gui::showAbout() {
    _imp->_aboutWindow->show();
    _imp->_aboutWindow->exec();
}

void Gui::openRecentFile() {
    QAction *action = qobject_cast<QAction *>(sender());
    if (action) {
        QFileInfo f(action->data().toString());
        
        QString path = f.path() + QDir::separator();
        ///if the current graph has no value, just load the project in the same window
        if (_imp->_appInstance->getProject()->isGraphWorthLess()) {
            _imp->_appInstance->getProject()->loadProject(path,f.fileName());
        } else {
            AppInstance* newApp = appPTR->newAppInstance();
            newApp->getProject()->loadProject(path,f.fileName());
        }
        
    }
}

void Gui::updateRecentFileActions() {
    QSettings settings;
    QStringList files = settings.value("recentFileList").toStringList();
    
    int numRecentFiles = std::min(files.size(), (int)NATRON_MAX_RECENT_FILES);
    
    for (int i = 0; i < numRecentFiles; ++i) {
        
        QString text = tr("&%1 %2").arg(i + 1).arg(QFileInfo(files[i]).fileName());
        _imp->actionsOpenRecentFile[i]->setText(text);
        _imp->actionsOpenRecentFile[i]->setData(files[i]);
        _imp->actionsOpenRecentFile[i]->setVisible(true);
    }
    for (int j = numRecentFiles; j < NATRON_MAX_RECENT_FILES; ++j)
        _imp->actionsOpenRecentFile[j]->setVisible(false);
    
}

QPixmap Gui::screenShot(QWidget* w) {
#if QT_VERSION < 0x050000
    if (w->objectName() == "CurveEditor") {
        return QPixmap::grabWidget(w);
    }
    return QPixmap::grabWindow(w->winId());
#else
    return QApplication::primaryScreen()->grabWindow(w->winId());
#endif
}

void Gui::onProjectNameChanged(const QString& name) {
    QString text(QCoreApplication::applicationName() + " - ");
    text.append(name);
    setWindowTitle(text);
}

void Gui::setColorPickersColor(const QColor& c) {
    assert(_imp->_projectGui);
    _imp->_projectGui->setPickersColor(c);
}

void Gui::registerNewColorPicker(boost::shared_ptr<Color_Knob> knob) {
    assert(_imp->_projectGui);
    _imp->_projectGui->registerNewColorPicker(knob);
}

void Gui::removeColorPicker(boost::shared_ptr<Color_Knob> knob) {
    assert(_imp->_projectGui);
    _imp->_projectGui->removeColorPicker(knob);
}

void Gui::updateViewersViewsMenu(int viewsCount) {
    for (std::list<ViewerTab*>::iterator it = _imp->_viewerTabs.begin();it!=_imp->_viewerTabs.end();++it) {
        (*it)->updateViewsMenu(viewsCount);
    }
}

void Gui::setViewersCurrentView(int view) {
    for (std::list<ViewerTab*>::iterator it = _imp->_viewerTabs.begin();it!=_imp->_viewerTabs.end();++it) {
        (*it)->setCurrentView(view);
    }

}

const std::list<ViewerTab*>& Gui::getViewersList() const {
    return _imp->_viewerTabs;
}


void Gui::activateViewerTab(ViewerInstance* viewer) {
    OpenGLViewerI* viewport = viewer->getUiContext();
    for (std::list<ViewerTab*>::iterator it = _imp->_viewerTabs.begin();it!=_imp->_viewerTabs.end();++it) {
        if ((*it)->getViewer() == viewport) {
            addViewerTab(*it, _imp->_viewersPane);
            (*it)->show();
        }
    }
}

void Gui::deactivateViewerTab(ViewerInstance* viewer) {
    OpenGLViewerI* viewport = viewer->getUiContext();
    for (std::list<ViewerTab*>::iterator it = _imp->_viewerTabs.begin();it!=_imp->_viewerTabs.end();++it) {
        if ((*it)->getViewer() == viewport) {
            removeViewerTab(*it, false,false);
            break;
        }
    }
}

ViewerTab* Gui::getViewerTabForInstance(ViewerInstance* node) const {
    for (std::list<ViewerTab*>::const_iterator it = _imp->_viewerTabs.begin();it!=_imp->_viewerTabs.end();++it) {
        if ((*it)->getInternalNode() == node) {
            return *it;
        }
    }
    return NULL;
}

const std::vector<NodeGui*>& Gui::getVisibleNodes() const {
    assert(_imp->_nodeGraphArea);
    return  _imp->_nodeGraphArea->getAllActiveNodes();
    
}


void Gui::deselectAllNodes() const {
    _imp->_nodeGraphArea->deselect();
}

void Gui::onProcessHandlerStarter(const QString& sequenceName,int firstFrame,int lastFrame,ProcessHandler* process) {
    ///make the dialog which will show the progress
    RenderingProgressDialog *dialog = new RenderingProgressDialog(sequenceName,firstFrame,lastFrame,this);
    QObject::connect(dialog,SIGNAL(canceled()),process,SLOT(onProcessCanceled()));
    QObject::connect(process,SIGNAL(processCanceled()),dialog,SLOT(onProcessCanceled()));
    QObject::connect(process,SIGNAL(frameRendered(int)),dialog,SLOT(onFrameRendered(int)));
    QObject::connect(process,SIGNAL(frameProgress(int)),dialog,SLOT(onCurrentFrameProgress(int)));

    dialog->show();
}

void Gui::setLastSelectedViewer(ViewerTab* tab){ _imp->_lastSelectedViewer = tab; }

ViewerTab* Gui::getLastSelectedViewer() const { return _imp->_lastSelectedViewer; }

void Gui::setNewViewerAnchor(TabWidget* where){ _imp->_nextViewerTabPlace = where; }

const std::vector<ToolButton*>& Gui::getToolButtons() const { return _imp->_toolButtons; }

GuiAppInstance* Gui::getApp() const { return _imp->_appInstance; }

const std::list<TabWidget*>& Gui::getPanes() const { return _imp->_panes; }

const std::list<QSplitter*>& Gui::getSplitters() const { return _imp->_splitters; }


void Gui::setUserScrubbingTimeline(bool b) { _imp->_isUserScrubbingTimeline = b; }

bool Gui::isUserScrubbingTimeline() const { return _imp->_isUserScrubbingTimeline; }

bool Gui::isDraggingPanel() const { return _imp->_currentlyDraggedPanel!=NULL; }

NodeGraph* Gui::getNodeGraph() const { return _imp->_nodeGraphArea; }

CurveEditor* Gui::getCurveEditor() const { return _imp->_curveEditor; }

QScrollArea* Gui::getPropertiesScrollArea() const { return _imp->_propertiesScrollArea; }

TabWidget* Gui::getWorkshopPane() const { return _imp->_workshopPane; }

const std::map<std::string,QWidget*>& Gui::getRegisteredTabs() const { return _imp->_registeredTabs; }

void Gui::debugImage(Natron::Image* image,const QString& filename ) {
    const RectI& rod = image->getRoD();
    QImage output(rod.width(),rod.height(),QImage::Format_ARGB32);
    const Natron::Color::Lut* lut = Natron::Color::LutManager::sRGBLut();
    lut->to_byte_packed(output.bits(), image->pixelAt(0, 0), rod, rod, rod,
                        Natron::Color::PACKING_RGBA,Natron::Color::PACKING_BGRA, true,false);
    U64 hashKey = image->getHashKey();
    QString hashKeyStr = QString::number(hashKey);
    QString realFileName = filename.isEmpty() ? QString(hashKeyStr+".png") : filename;
    output.save(realFileName);

}

void Gui::updateLastSequenceOpenedPath(const QString& path) {
    _imp->_lastLoadSequenceOpenedDir = path;
}

void Gui::updateLastSequenceSavedPath(const QString& path) {
    _imp->_lastSaveSequenceOpenedDir = path;
}

void Gui::onWriterRenderStarted(const QString& sequenceName,int firstFrame,int lastFrame,Natron::OutputEffectInstance* writer) {
    RenderingProgressDialog *dialog = new RenderingProgressDialog(sequenceName,firstFrame,lastFrame,this);
    VideoEngine* ve = writer->getVideoEngine().get();
    QObject::connect(dialog,SIGNAL(canceled()),ve,SLOT(abortRenderingNonBlocking()));
    QObject::connect(ve,SIGNAL(frameRendered(int)),dialog,SLOT(onFrameRendered(int)));
    QObject::connect(ve,SIGNAL(progressChanged(int)),dialog,SLOT(onCurrentFrameProgress(int)));
    QObject::connect(ve,SIGNAL(engineStopped()),dialog,SLOT(onProcessCanceled()));
    dialog->show();
}