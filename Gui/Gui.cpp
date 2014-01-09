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
#include <QtCore/QEvent>
#include <QtGui/QCloseEvent>
#include <QApplication>
#include <QMenu>
#include <QUrl>
#include <QLayout>
#include <QDesktopWidget>
#include <QSplitter>
#include <QMenuBar>
#include <QToolBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QProgressBar>
#include <QSettings>
#include <QScrollBar>
#include <QUndoGroup>
#include <QDropEvent>

#include "Global/AppManager.h"

#include "Engine/VideoEngine.h"
#include "Engine/Settings.h"
#include "Engine/ViewerInstance.h"
#include "Engine/Project.h"
#include "Engine/OfxEffectInstance.h"
#include "Engine/KnobFile.h"

#include "Gui/Texture.h"
#include "Gui/ViewerGL.h"
#include "Gui/TabWidget.h"
#include "Gui/SpinBox.h"
#include "Gui/TimeLineGui.h"
#include "Gui/NodeGraph.h"
#include "Gui/ViewerTab.h"
#include "Gui/SequenceFileDialog.h"
#include "Gui/NodeGui.h"
#include "Gui/Button.h"
#include "Gui/ComboBox.h"
#include "Gui/CurveEditor.h"
#include "Gui/ProjectGui.h"
#include "Gui/DockablePanel.h"
#include "Gui/PreferencesPanel.h"


#define PLUGIN_GROUP_DEFAULT "Other"
#define PLUGIN_GROUP_DEFAULT_ICON_PATH NATRON_IMAGES_PATH"openeffects.png"

using namespace Natron;
using std::make_pair;

// Helper function: Get the icon with the given name from the icon theme.
// If unavailable, fall back to the built-in icon. Icon names conform to this specification:
// http://standards.freedesktop.org/icon-naming-spec/icon-naming-spec-latest.html
static QIcon get_icon(const QString &name)
{
    return QIcon::fromTheme(name, QIcon(QString(":icons/") + name));
}

Gui::Gui(AppInstance* app,QWidget* parent):QMainWindow(parent),
    _lastSelectedViewer(NULL),
    _appInstance(app),
    _uiUsingMainThreadCond(),
    _uiUsingMainThread(false),
    _uiUsingMainThreadMutex(),
    _lastQuestionDialogAnswer(Natron::No),
    _currentUndoAction(0),
    _currentRedoAction(0),
    _undoStacksGroup(0),
    _undoStacksActions(),
    _splitters(),
    actionNew_project(0),
    actionOpen_project(0),
    actionSave_project(0),
    actionSaveAs_project(0),
    actionPreferences(0),
    actionExit(0),
    actionProject_settings(0),
    actionFullScreen(0),
    actionClearDiskCache(0),
    actionClearPlayBackCache(0),
    actionClearNodeCache(0),
    actionClearPluginsLoadingCache(0),
    actionClearAllCaches(0),
    actionConnectInput1(0),
    actionConnectInput2(0),
    actionConnectInput3(0),
    actionConnectInput4(0),
    actionConnectInput5(0),
    actionConnectInput6(0),
    actionConnectInput7(0),
    actionConnectInput8(0),
    actionConnectInput9(0),
    actionConnectInput10(0),
    _centralWidget(0),
    _mainLayout(0),
    _lastLoadSequenceOpenedDir(),
    _lastLoadProjectOpenedDir(),
    _lastSaveSequenceOpenedDir(),
    _lastSaveProjectOpenedDir(),
    _viewersPane(0),
    _nextViewerTabPlace(0),
    _workshopPane(0),
    _viewerWorkshopSplitter(0),
    _propertiesPane(0),
    _middleRightSplitter(0),
    _leftRightSplitter(0),
    _graphScene(0),
    _nodeGraphArea(0),
    _curveEditor(0),
    _toolBox(0),
    _propertiesScrollArea(0),
    _propertiesContainer(0),
    _layoutPropertiesBin(0),
    menubar(0),
    menuFile(0),
    menuEdit(0),
    menuDisplay(0),
    menuOptions(0),
    viewersMenu(0),
    viewerInputsMenu(0),
    cacheMenu(0),
    _settingsGui(0),
    _projectGui(0)
{
    QObject::connect(this,SIGNAL(doDialog(int,QString,QString,Natron::StandardButtons,int)),this,
                     SLOT(onDoDialog(int,QString,QString,Natron::StandardButtons,int)));
    QObject::connect(app,SIGNAL(pluginsPopulated()),this,SLOT(addToolButttonsToToolBar()));
}

Gui::~Gui()
{
    delete _projectGui;
    delete _undoStacksGroup;
    _viewerTabs.clear();
    for(U32 i = 0; i < _toolButtons.size();++i){
        delete _toolButtons[i];
    }
}

bool Gui::exit(){
    int ret = saveWarning();
    if (ret == 0) {
        saveProject();
    } else if (ret == 2) {
        return false;
    }
    saveGuiGeometry();
    assert(_appInstance);
	int appId = _appInstance->getAppID();
    delete _appInstance;
    delete this;
    if (appId != 0) {
        return false;
    } else {
        AppManager::quit();
        QCoreApplication::instance()->quit();
        return true;
    }
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


NodeGui* Gui::createNodeGUI( Node* node){
    assert(_nodeGraphArea);
    NodeGui* nodeGui = _nodeGraphArea->createNodeGUI(_layoutPropertiesBin,node);
    assert(nodeGui);
    return nodeGui;
}

void Gui::addNodeGuiToCurveEditor(NodeGui* node){
    _curveEditor->addNode(node);
}

void Gui::createViewerGui(Node* viewer){
    TabWidget* where = _nextViewerTabPlace;
    if(!where){
        where = _viewersPane;
    }else{
        _nextViewerTabPlace = NULL; // < reseting anchor to default
    }
    ViewerInstance* v = dynamic_cast<ViewerInstance*>(viewer->getLiveInstance());
    assert(v);
    v->initializeViewerTab(where);
    _lastSelectedViewer = v->getUiContext();
}

void Gui::autoConnect(NodeGui* target,NodeGui* created) {
    assert(_nodeGraphArea);
    if(target) {
        _nodeGraphArea->autoConnect(target, created);
    }
    _nodeGraphArea->selectNode(created);
}

NodeGui* Gui::getSelectedNode() const {
    assert(_nodeGraphArea);
    return _nodeGraphArea->getSelectedNode();
}


void Gui::createGui(){
    
    
    setupUi();
    
    QObject::connect(QCoreApplication::instance(), SIGNAL(aboutToQuit()), this, SLOT(exit()));
    
}



bool Gui::eventFilter(QObject *target, QEvent *event) {
    assert(_appInstance);
    if(dynamic_cast<QInputEvent*>(event)){
        /*Make top level instance this instance since it receives all
         user inputs.*/
        appPTR->setAsTopLevelInstance(_appInstance->getAppID());
    }
    return QMainWindow::eventFilter(target, event);
}
void Gui::retranslateUi(QMainWindow *MainWindow)
{
    Q_UNUSED(MainWindow);
    setWindowTitle(QCoreApplication::applicationName());
    assert(actionNew_project);
    actionNew_project->setText(tr("&New Project"));
    assert(actionOpen_project);
    actionOpen_project->setText(tr("&Open Project..."));
    assert(actionSave_project);
    actionSave_project->setText(tr("&Save Project"));
    assert(actionSaveAs_project);
    actionSaveAs_project->setText(tr("Save Project As..."));
    assert(actionPreferences);
    actionPreferences->setText(tr("Preferences"));
    assert(actionExit);
    actionExit->setText(tr("E&xit"));
    assert(actionProject_settings);
    actionProject_settings->setText(tr("Project Settings..."));
    assert(actionFullScreen);
    actionFullScreen->setText(tr("Toggle Full Screen"));
    assert(actionClearDiskCache);
    actionClearDiskCache->setText(tr("Clear Disk Cache"));
    assert(actionClearPlayBackCache);
    actionClearPlayBackCache->setText(tr("Clear Playback Cache"));
    assert(actionClearNodeCache);
    actionClearNodeCache ->setText(tr("Clear Per-Node Cache"));
    assert(actionClearPluginsLoadingCache);
    actionClearPluginsLoadingCache->setText(tr("Clear plugins loading cache"));
    assert(actionClearAllCaches);
    actionClearAllCaches->setText(tr("Clear all caches"));
    
    assert(actionConnectInput1);
    actionConnectInput1 ->setText(tr("Connect to input 1"));
    assert(actionConnectInput2);
    actionConnectInput2 ->setText(tr("Connect to input 2"));
    assert(actionConnectInput3);
    actionConnectInput3 ->setText(tr("Connect to input 3"));
    assert(actionConnectInput4);
    actionConnectInput4 ->setText(tr("Connect to input 4"));
    assert(actionConnectInput5);
    actionConnectInput5 ->setText(tr("Connect to input 5"));
    assert(actionConnectInput6);
    actionConnectInput6 ->setText(tr("Connect to input 6"));
    assert(actionConnectInput7);
    actionConnectInput7 ->setText(tr("Connect to input 7"));
    assert(actionConnectInput8);
    actionConnectInput8 ->setText(tr("Connect to input 8"));
    assert(actionConnectInput9);
    actionConnectInput9 ->setText(tr("Connect to input 9"));
    assert(actionConnectInput10);
    actionConnectInput10 ->setText(tr("Connect to input 10"));
    
    //WorkShop->setTabText(WorkShop->indexOf(CurveEditor), tr("Motion Editor"));
    
    //WorkShop->setTabText(WorkShop->indexOf(GraphEditor), tr("Graph Editor"));
    assert(menuFile);
    menuFile->setTitle(tr("File"));
    assert(menuEdit);
    menuEdit->setTitle(tr("Edit"));
    assert(menuDisplay);
    menuDisplay->setTitle(tr("Display"));
    assert(menuOptions);
    menuOptions->setTitle(tr("Options"));
    assert(viewersMenu);
    viewersMenu->setTitle(tr("Viewer(s)"));
    assert(cacheMenu);
    cacheMenu->setTitle(tr("Cache"));
    assert(viewerInputsMenu);
    viewerInputsMenu->setTitle(tr("Connect Current Viewer"));
    assert(viewersViewMenu);
    viewersViewMenu->setTitle(tr("Display view number"));
}
void Gui::setupUi()
{
    
    setMouseTracking(true);
    installEventFilter(this);
    QDesktopWidget* desktop=QApplication::desktop();
    QRect screen=desktop->screenGeometry();
    assert(!isFullScreen());
    resize((int)(0.93*screen.width()),(int)(0.93*screen.height())); // leave some space
    assert(!isDockNestingEnabled()); // should be false by default
    
    QPixmap appIcPixmap;
    appPTR->getIcon(Natron::NATRON_PIXMAP_APP_ICON, &appIcPixmap);
    QIcon appIc(appIcPixmap);
    setWindowIcon(appIc);
    
    loadStyleSheet();
    
    
    _undoStacksGroup = new QUndoGroup;
    QObject::connect(_undoStacksGroup, SIGNAL(activeStackChanged(QUndoStack*)), this, SLOT(onCurrentUndoStackChanged(QUndoStack*)));
    
    /*TOOL BAR menus*/
    //======================
    menubar = new QMenuBar(this);
    //menubar->setGeometry(QRect(0, 0, 1159, 21)); // why set the geometry of a menubar?
    menuFile = new QMenu(menubar);
    menuEdit = new QMenu(menubar);
    menuDisplay = new QMenu(menubar);
    menuOptions = new QMenu(menubar);
    viewersMenu= new QMenu(menuDisplay);
    viewerInputsMenu = new QMenu(viewersMenu);
    viewersViewMenu = new QMenu(viewersMenu);
    cacheMenu= new QMenu(menubar);
    
    setMenuBar(menubar);

    actionNew_project = new QAction(this);
    actionNew_project->setObjectName(QString::fromUtf8("actionNew_project"));
    actionNew_project->setShortcut(QKeySequence::New);
    actionNew_project->setIcon(get_icon("document-new"));
    QObject::connect(actionNew_project, SIGNAL(triggered()), this, SLOT(newProject()));
    actionOpen_project = new QAction(this);
    actionOpen_project->setObjectName(QString::fromUtf8("actionOpen_project"));
    actionOpen_project->setShortcut(QKeySequence::Open);
    actionOpen_project->setIcon(get_icon("document-open"));
    QObject::connect(actionOpen_project, SIGNAL(triggered()), this, SLOT(openProject()));
    actionSave_project = new QAction(this);
    actionSave_project->setObjectName(QString::fromUtf8("actionSave_project"));
    actionSave_project->setShortcut(QKeySequence::Save);
    actionSave_project->setIcon(get_icon("document-save"));
    QObject::connect(actionSave_project, SIGNAL(triggered()), this, SLOT(saveProject()));
    actionSaveAs_project = new QAction(this);
    actionSaveAs_project->setObjectName(QString::fromUtf8("actionSaveAs_project"));
    actionSaveAs_project->setShortcut(QKeySequence::SaveAs);
    actionSaveAs_project->setIcon(get_icon("document-save-as"));
    QObject::connect(actionSaveAs_project, SIGNAL(triggered()), this, SLOT(saveProjectAs()));
    actionPreferences = new QAction(this);
    actionPreferences->setObjectName(QString::fromUtf8("actionPreferences"));
    actionPreferences->setMenuRole(QAction::PreferencesRole);
    actionExit = new QAction(this);
    actionExit->setObjectName(QString::fromUtf8("actionExit"));
    actionExit->setMenuRole(QAction::QuitRole);
    actionExit->setIcon(get_icon("application-exit"));
    actionProject_settings = new QAction(this);
    actionProject_settings->setObjectName(QString::fromUtf8("actionProject_settings"));
    actionProject_settings->setIcon(get_icon("document-properties"));
    actionProject_settings->setShortcut(QKeySequence(Qt::Key_S));
    actionFullScreen = new QAction(this);
    actionFullScreen->setObjectName(QString::fromUtf8("actionFullScreen"));
    actionFullScreen->setShortcut(QKeySequence(Qt::CTRL+Qt::META+Qt::Key_F));
    actionFullScreen->setIcon(get_icon("view-fullscreen"));
    actionClearDiskCache = new QAction(this);
    actionClearDiskCache->setObjectName(QString::fromUtf8("actionClearDiskCache"));
    actionClearDiskCache->setCheckable(false);
    actionClearPlayBackCache = new QAction(this);
    actionClearPlayBackCache->setObjectName(QString::fromUtf8("actionClearPlayBackCache"));
    actionClearPlayBackCache->setCheckable(false);
    actionClearNodeCache = new QAction(this);
    actionClearNodeCache->setObjectName(QString::fromUtf8("actionClearNodeCache"));
    actionClearNodeCache->setCheckable(false);
    actionClearPluginsLoadingCache = new QAction(this);
    actionClearPluginsLoadingCache->setObjectName(QString::fromUtf8("actionClearPluginsLoadedCache"));
    actionClearPluginsLoadingCache->setCheckable(false);
    actionClearAllCaches = new QAction(this);
    actionClearAllCaches->setObjectName(QString::fromUtf8("actionClearAllCaches"));
    actionClearAllCaches->setCheckable(false);
    actionClearAllCaches->setShortcut(QKeySequence(Qt::CTRL+Qt::SHIFT+Qt::Key_K));
    
    actionConnectInput1 = new QAction(this);
    actionConnectInput1->setCheckable(false);
    actionConnectInput1->setShortcut(QKeySequence(Qt::Key_1));
    
    actionConnectInput2 = new QAction(this);
    actionConnectInput2->setCheckable(false);
    actionConnectInput2->setShortcut(QKeySequence(Qt::Key_2));
    
    actionConnectInput3 = new QAction(this);
    actionConnectInput3->setCheckable(false);
    actionConnectInput3->setShortcut(QKeySequence(Qt::Key_3));
    
    actionConnectInput4 = new QAction(this);
    actionConnectInput4->setCheckable(false);
    actionConnectInput4->setShortcut(QKeySequence(Qt::Key_4));
    
    actionConnectInput5 = new QAction(this);
    actionConnectInput5->setCheckable(false);
    actionConnectInput5->setShortcut(QKeySequence(Qt::Key_5));
    
    actionConnectInput6 = new QAction(this);
    actionConnectInput6->setCheckable(false);
    actionConnectInput6->setShortcut(QKeySequence(Qt::Key_6));
    
    actionConnectInput7 = new QAction(this);
    actionConnectInput7->setCheckable(false);
    actionConnectInput7->setShortcut(QKeySequence(Qt::Key_7));
    
    actionConnectInput8 = new QAction(this);
    actionConnectInput8->setCheckable(false);
    actionConnectInput8->setShortcut(QKeySequence(Qt::Key_8));
    
    actionConnectInput9 = new QAction(this);
    actionConnectInput9->setCheckable(false);
    actionConnectInput9->setShortcut(QKeySequence(Qt::Key_9));
    
    actionConnectInput10 = new QAction(this);
    actionConnectInput10->setCheckable(false);
    actionConnectInput10->setShortcut(QKeySequence(Qt::Key_0));

    
    /*CENTRAL AREA*/
    //======================
    _centralWidget = new QWidget(this);
    setCentralWidget(_centralWidget);
    _mainLayout = new QHBoxLayout(_centralWidget);
    _mainLayout->setContentsMargins(0, 0, 0, 0);
    _centralWidget->setLayout(_mainLayout);
    
    _leftRightSplitter = new QSplitter(_centralWidget);
    _leftRightSplitter->setObjectName("ToolBar_splitter");
    _splitters.push_back(_leftRightSplitter);
    _leftRightSplitter->setChildrenCollapsible(false);
    _leftRightSplitter->setOrientation(Qt::Horizontal);
    _leftRightSplitter->setContentsMargins(0, 0, 0, 0);
    
    
    _toolBox = new QToolBar(_leftRightSplitter);
    _toolBox->setOrientation(Qt::Vertical);
    _toolBox->setMaximumWidth(40);
    
    _leftRightSplitter->addWidget(_toolBox);
    
    _viewerWorkshopSplitter = new QSplitter(_centralWidget);
    _viewerWorkshopSplitter->setObjectName("Viewers_Workshop_splitter");
    _splitters.push_back(_viewerWorkshopSplitter);
    _viewerWorkshopSplitter->setContentsMargins(0, 0, 0, 0);
    _viewerWorkshopSplitter->setOrientation(Qt::Vertical);
    _viewerWorkshopSplitter->setChildrenCollapsible(false);
    QSize viewerWorkshopSplitterSize = _viewerWorkshopSplitter->sizeHint();
    QList<int> sizesViewerSplitter; sizesViewerSplitter <<  viewerWorkshopSplitterSize.height()/2;
    sizesViewerSplitter  << viewerWorkshopSplitterSize.height()/2;
    
    /*VIEWERS related*/
    
    _viewersPane = new TabWidget(this,TabWidget::NOT_CLOSABLE,_viewerWorkshopSplitter);
    _viewersPane->setObjectName("ViewerPane");
    _panes.push_back(_viewersPane);
    _viewersPane->resize(_viewersPane->width(), this->height()/5);
    _viewerWorkshopSplitter->addWidget(_viewersPane);
    //  _viewerWorkshopSplitter->setSizes(sizesViewerSplitter);
    
    /*WORKSHOP PANE*/
    //======================
    _workshopPane = new TabWidget(this,TabWidget::NOT_CLOSABLE,_viewerWorkshopSplitter);
    _workshopPane->setObjectName("WorkshopPane");
    _panes.push_back(_workshopPane);
    
    _graphScene = new QGraphicsScene(this);
    _graphScene->setItemIndexMethod(QGraphicsScene::NoIndex);
    _nodeGraphArea = new NodeGraph(this,_graphScene,_workshopPane);
    _nodeGraphArea->setObjectName(kNodeGraphObjectName);
    _workshopPane->appendTab(_nodeGraphArea);
    
    _curveEditor = new CurveEditor(_appInstance->getTimeLine(),this);

    _curveEditor->setObjectName(kCurveEditorObjectName);
    _workshopPane->appendTab(_curveEditor);
    
    _workshopPane->makeCurrentTab(0);
    
    _viewerWorkshopSplitter->addWidget(_workshopPane);
    

    _middleRightSplitter = new QSplitter(_centralWidget);
    _middleRightSplitter->setObjectName("Center_PropertiesBin_splitter");
    _splitters.push_back(_middleRightSplitter);
    _middleRightSplitter->setChildrenCollapsible(false);
    _middleRightSplitter->setContentsMargins(0, 0, 0, 0);
    _middleRightSplitter->setOrientation(Qt::Horizontal);
    _middleRightSplitter->addWidget(_viewerWorkshopSplitter);
    
    /*PROPERTIES DOCK*/
    //======================
    _propertiesPane = new TabWidget(this,TabWidget::NOT_CLOSABLE,this);
    _propertiesPane->setObjectName("PropertiesPane");
    _panes.push_back(_propertiesPane);

    _propertiesScrollArea = new QScrollArea(_propertiesPane);
    _nodeGraphArea->setPropertyBinPtr(_propertiesScrollArea);
    _propertiesScrollArea->setObjectName("Properties");

    _propertiesContainer=new QWidget(_propertiesScrollArea);
    _propertiesContainer->setObjectName("_propertiesContainer");
    _layoutPropertiesBin=new QVBoxLayout(_propertiesContainer);
    _layoutPropertiesBin->setSpacing(0);
    _layoutPropertiesBin->setContentsMargins(0, 0, 0, 0);
    _propertiesContainer->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
    _propertiesContainer->setLayout(_layoutPropertiesBin);  
    _propertiesScrollArea->setWidget(_propertiesContainer);
    _propertiesScrollArea->setWidgetResizable(true);

    _propertiesPane->appendTab(_propertiesScrollArea);
    
    _middleRightSplitter->addWidget(_propertiesPane);
    QSize horizontalSplitterSize = _middleRightSplitter->sizeHint();
    QList<int> sizes;
    sizes << 195;
    sizes << horizontalSplitterSize.width()- 195;
    _middleRightSplitter->setSizes(sizes);

    
    _leftRightSplitter->addWidget(_middleRightSplitter);
    
    _mainLayout->addWidget(_leftRightSplitter);


    _projectGui = new ProjectGui;
    _projectGui->create(_appInstance->getProject(),
                        _layoutPropertiesBin,
                        _propertiesContainer);
    

    _settingsGui = new PreferencesPanel(appPTR->getCurrentSettings(),this);
    _settingsGui->hide();

    setVisibleProjectSettingsPanel();
    
    menubar->addAction(menuFile->menuAction());
    menubar->addAction(menuEdit->menuAction());
    menubar->addAction(menuDisplay->menuAction());
    menubar->addAction(menuOptions->menuAction());
    menubar->addAction(cacheMenu->menuAction());
    menuFile->addAction(actionNew_project);
    menuFile->addAction(actionOpen_project);
    menuFile->addAction(actionSave_project);
    menuFile->addAction(actionSaveAs_project);
    menuFile->addSeparator();
    menuFile->addSeparator();
    menuFile->addAction(actionExit);

    menuEdit->addAction(actionPreferences);

    menuOptions->addAction(actionProject_settings);
    menuDisplay->addAction(viewersMenu->menuAction());
    viewersMenu->addAction(viewerInputsMenu->menuAction());
    viewersMenu->addAction(viewersViewMenu->menuAction());
    viewerInputsMenu->addAction(actionConnectInput1);
    viewerInputsMenu->addAction(actionConnectInput2);
    viewerInputsMenu->addAction(actionConnectInput3);
    viewerInputsMenu->addAction(actionConnectInput4);
    viewerInputsMenu->addAction(actionConnectInput5);
    viewerInputsMenu->addAction(actionConnectInput6);
    viewerInputsMenu->addAction(actionConnectInput7);
    viewerInputsMenu->addAction(actionConnectInput8);
    viewerInputsMenu->addAction(actionConnectInput9);
    viewerInputsMenu->addAction(actionConnectInput10);
    menuDisplay->addSeparator();
    menuDisplay->addAction(actionFullScreen);
    
    cacheMenu->addAction(actionClearDiskCache);
    cacheMenu->addAction(actionClearPlayBackCache);
    cacheMenu->addAction(actionClearNodeCache);
    cacheMenu->addAction(actionClearPluginsLoadingCache);
    cacheMenu->addAction(actionClearAllCaches);
    retranslateUi(this);
    
    QObject::connect(actionFullScreen, SIGNAL(triggered()),this,SLOT(toggleFullScreen()));
    QObject::connect(actionClearDiskCache, SIGNAL(triggered()),appPTR,SLOT(clearDiskCache()));
    QObject::connect(actionClearPlayBackCache, SIGNAL(triggered()),appPTR,SLOT(clearPlaybackCache()));
    QObject::connect(actionClearNodeCache, SIGNAL(triggered()),appPTR,SLOT(clearNodeCache()));
    QObject::connect(actionClearPluginsLoadingCache, SIGNAL(triggered()),appPTR,SLOT(clearPluginsLoadedCache()));
    QObject::connect(actionClearAllCaches, SIGNAL(triggered()),appPTR,SLOT(clearAllCaches()));

    
    //the same action also clears the ofx plugins caches, they are not the same cache but are used to the same end
    QObject::connect(actionClearNodeCache, SIGNAL(triggered()),_appInstance,SLOT(clearOpenFXPluginsCaches()));
    QObject::connect(actionExit,SIGNAL(triggered()),this,SLOT(exit()));
    QObject::connect(actionProject_settings,SIGNAL(triggered()),this,SLOT(setVisibleProjectSettingsPanel()));
    
    QObject::connect(actionConnectInput1, SIGNAL(triggered()),this,SLOT(connectInput1()));
    QObject::connect(actionConnectInput2, SIGNAL(triggered()),this,SLOT(connectInput2()));
    QObject::connect(actionConnectInput3, SIGNAL(triggered()),this,SLOT(connectInput3()));
    QObject::connect(actionConnectInput4, SIGNAL(triggered()),this,SLOT(connectInput4()));
    QObject::connect(actionConnectInput5, SIGNAL(triggered()),this,SLOT(connectInput5()));
    QObject::connect(actionConnectInput6, SIGNAL(triggered()),this,SLOT(connectInput6()));
    QObject::connect(actionConnectInput7, SIGNAL(triggered()),this,SLOT(connectInput7()));
    QObject::connect(actionConnectInput8, SIGNAL(triggered()),this,SLOT(connectInput8()));
    QObject::connect(actionConnectInput9, SIGNAL(triggered()),this,SLOT(connectInput9()));
    QObject::connect(actionConnectInput10, SIGNAL(triggered()),this,SLOT(connectInput10()));
    
    QObject::connect(actionPreferences,SIGNAL(triggered()),this,SLOT(showSettings()));

    QMetaObject::connectSlotsByName(this);
    
    restoreGuiGeometry();
    
    
} // setupUi


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
    viewersViewMenu->clear();
    //if viewsCount == 1 we don't add a menu entry
    viewersMenu->removeAction(viewersViewMenu->menuAction());
    if(viewsCount == 2){
        QAction* left = new QAction(this);
        left->setCheckable(false);
        left->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_1));
        viewersViewMenu->addAction(left);
        left->setText("Display left view");
        QObject::connect(left,SIGNAL(triggered()),this,SLOT(showView0()));
        
        QAction* right = new QAction(this);
        right->setCheckable(false);
        right->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_2));
        viewersViewMenu->addAction(right);
        right->setText("Display right view");
        QObject::connect(right,SIGNAL(triggered()),this,SLOT(showView1()));
        
        viewersMenu->addAction(viewersViewMenu->menuAction());
    }else if(viewsCount > 2){
        for(int i = 0; i < viewsCount;++i){
            if(i > 9)
                break;
            QAction* viewI = new QAction(this);
            viewI->setCheckable(false);
            QKeySequence seq = keySequenceForView(i);
            if(!seq.isEmpty()){
                viewI->setShortcut(seq);
            }
            viewersViewMenu->addAction(viewI);
            const char* slot = slotForView(i);
            viewI->setText(QString("Display view ")+QString::number(i+1));
            if(slot){
                QObject::connect(viewI,SIGNAL(triggered()),this,slot);
            }
        }
        
        viewersMenu->addAction(viewersViewMenu->menuAction());
    }
}


void Gui::putSettingsPanelFirst(DockablePanel* panel){
    _layoutPropertiesBin->removeWidget(panel);
    _layoutPropertiesBin->insertWidget(0, panel);
    _propertiesScrollArea->verticalScrollBar()->setValue(0);
}

void Gui::setVisibleProjectSettingsPanel() {
    putSettingsPanelFirst(_projectGui->getPanel());
    if(!_projectGui->isVisible()){
        _projectGui->setVisible(true);
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
                      .arg("rgb(243,149,0)")
                      .arg("rgb(50,50,50)")
                      .arg("rgb(71,71,71)")
                      .arg("rgb(38,38,38)")
                      .arg("rgb(200,200,200)")
                      .arg("rgb(86,117,156)")
                      .arg("rgb(21,97,248)"));
    }
}

void Gui::maximize(TabWidget* what) {
    assert(what);
    if(what->isFloating())
        return;
    for (std::list<TabWidget*>::iterator it = _panes.begin(); it != _panes.end(); ++it) {
        
        //if the widget is not what we want to maximize and it is not floating , hide it
        if (*it != what && !(*it)->isFloating()) {
            
            // also if we want to maximize the workshop pane, don't hide the properties pane
            if((*it) == _propertiesPane && what == _workshopPane){
                continue;
            }
            (*it)->hide();
        }
    }
}

void Gui::minimize(){
    for (std::list<TabWidget*>::iterator it = _panes.begin(); it != _panes.end(); ++it) {
        (*it)->show();
    }
}


ViewerTab* Gui::addNewViewerTab(ViewerInstance* viewer,TabWidget* where){
    ViewerTab* tab = new ViewerTab(this,viewer,_viewersPane);
    _viewerTabs.push_back(tab);
    where->appendTab(tab);
    return tab;
}

void Gui::addViewerTab(ViewerTab* tab, TabWidget* where) {
    assert(tab);
    assert(where);
    std::list<ViewerTab*>::iterator it = std::find(_viewerTabs.begin(), _viewerTabs.end(), tab);
    if (it == _viewerTabs.end()) {
        _viewerTabs.push_back(tab);
    }
    where->appendTab(tab);
    
}

void Gui::removeViewerTab(ViewerTab* tab,bool initiatedFromNode,bool deleteData){
    assert(tab);
    std::list<ViewerTab*>::iterator it = std::find(_viewerTabs.begin(), _viewerTabs.end(), tab);
    if (it != _viewerTabs.end()) {
        _viewerTabs.erase(it);
    }
    
    std::map<std::string, QWidget*>::iterator registeredTab = _registeredTabs.find(tab->objectName().toStdString());
    if(registeredTab != _registeredTabs.end()){
        _registeredTabs.erase(registeredTab);
    }
    
    if(deleteData){
        if (!initiatedFromNode) {
            assert(_nodeGraphArea);
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
    std::list<TabWidget*>::iterator found = std::find(_panes.begin(), _panes.end(), pane);
    assert(found != _panes.end());
    _panes.erase(found);
}

void Gui::registerPane(TabWidget* pane){
    std::list<TabWidget*>::iterator found = std::find(_panes.begin(), _panes.end(), pane);
    if(found == _panes.end()){
        _panes.push_back(pane);
    }
}

void Gui::registerSplitter(QSplitter* s){
    std::list<QSplitter*>::iterator found = std::find(_splitters.begin(), _splitters.end(), s);
    if(found == _splitters.end()){
        _splitters.push_back(s);
    }
}

void Gui::removeSplitter(QSplitter* s){
    std::list<QSplitter*>::iterator found = std::find(_splitters.begin(), _splitters.end(), s);
    if(found != _splitters.end()){
        _splitters.erase(found);
    }
}


void Gui::registerTab(const std::string& name, QWidget* tab){
    _registeredTabs.insert(make_pair(name,tab));
}

QWidget* Gui::findExistingTab(const std::string& name) const{
    std::map<std::string,QWidget*>::const_iterator it = _registeredTabs.find(name);
    if (it != _registeredTabs.end()) {
        return it->second;
    }else{
        return NULL;
    }
}
ToolButton* Gui::findExistingToolButton(const QString& label) const{
    for(U32 i = 0; i < _toolButtons.size();++i){
        if(_toolButtons[i]->getLabel() == label){
            return _toolButtons[i];
        }
    }
    return NULL;
}

ToolButton* Gui::findOrCreateToolButton(PluginToolButton* plugin){
    for(U32 i = 0; i < _toolButtons.size();++i){
        if(_toolButtons[i]->getID() == plugin->getID()){
            return _toolButtons[i];
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
            icon.addFile(PLUGIN_GROUP_DEFAULT_ICON_PATH);
        }
    }
    //if the tool-button has no children, this is a leaf, we must create an action
    bool isLeaf = false;
    if(plugin->getChildren().empty()){
        isLeaf = true;
        //if the plugin has no children and no parent, put it in the "others" group
        if(!plugin->hasParent()){
            ToolButton* othersGroup = findExistingToolButton(PLUGIN_GROUP_DEFAULT);
            PluginToolButton* othersToolButton = appPTR->findPluginToolButtonOrCreate(PLUGIN_GROUP_DEFAULT,PLUGIN_GROUP_DEFAULT, PLUGIN_GROUP_DEFAULT_ICON_PATH);
            othersToolButton->tryAddChild(plugin);
            
            //if the othersGroup doesn't exist, create it
            if(!othersGroup){
                othersGroup = findOrCreateToolButton(othersToolButton);
            }
            parentToolButton = othersGroup;
        }
    }
    ToolButton* pluginsToolButton = new ToolButton(_appInstance,plugin,plugin->getID(),plugin->getLabel(),icon);
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
    _toolButtons.push_back(pluginsToolButton);
    return pluginsToolButton;
}

void ToolButton::tryAddChild(ToolButton* child){
    assert(_menu);
    for(unsigned int i = 0; i < _children.size();++i){
        if(_children[i] == child){
            return;
        }
    }
    _children.push_back(child);
    _menu->addAction(child->getAction());
}


void ToolButton::onTriggered(){
    _app->createNode(_id);
}
void Gui::addToolButttonsToToolBar(){
    for (U32 i = 0; i < _toolButtons.size(); ++i) {
        
        //if the toolbutton is a root (no parent), add it in the toolbox
        if(_toolButtons[i]->hasChildren() && !_toolButtons[i]->getPluginToolButton()->hasParent()){
            QToolButton* button = new QToolButton(_toolBox);
            button->setIcon(_toolButtons[i]->getIcon());
            button->setMenu(_toolButtons[i]->getMenu());
            button->setPopupMode(QToolButton::InstantPopup);
            button->setToolTip(_toolButtons[i]->getLabel());
            _toolBox->addWidget(button);
        }
    }
}

void Gui::setUndoRedoActions(QAction* undoAction,QAction* redoAction){
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
void Gui::newProject(){
    appPTR->newAppInstance(false);
}
void Gui::openProject(){
    std::vector<std::string> filters;
    filters.push_back(NATRON_PROJECT_FILE_EXT);
    QStringList selectedFiles =  popOpenFileDialog(false, filters, _lastLoadProjectOpenedDir.toStdString());
    
    if (selectedFiles.size() > 0) {
        //clearing current graph
        _appInstance->clearNodes();
        QString file = selectedFiles.at(0);
        QString name = SequenceFileDialog::removePath(file);
        QString path = file.left(file.indexOf(name));
        
        _appInstance->loadProject(path,name);
    }
    
}
void Gui::saveProject(){
    if(_appInstance->hasProjectBeenSavedByUser()){
        _appInstance->saveProject(_appInstance->getCurrentProjectPath(),_appInstance->getCurrentProjectName(),false);
    }else{
        saveProjectAs();
    }
}
void Gui::saveProjectAs(){
    std::vector<std::string> filter;
    filter.push_back(NATRON_PROJECT_FILE_EXT);
    QString outFile = popSaveFileDialog(false, filter,_lastSaveProjectOpenedDir.toStdString());
    if (outFile.size() > 0) {
        if (outFile.indexOf("." NATRON_PROJECT_FILE_EXT) == -1) {
            outFile.append("." NATRON_PROJECT_FILE_EXT);
        }
        QString file = SequenceFileDialog::removePath(outFile);
        QString path = outFile.left(outFile.indexOf(file));
        _appInstance->saveProject(path,file,false);
    }
}

void Gui::createReader(){
    std::map<std::string,std::string> readersForFormat;
    appPTR->getCurrentSettings()->getFileFormatsForReadingAndReader(&readersForFormat);
    std::vector<std::string> filters;
    for (std::map<std::string,std::string>::const_iterator it = readersForFormat.begin(); it!=readersForFormat.end(); ++it) {
        filters.push_back(it->first);
    }
    QStringList files = popOpenFileDialog(true, filters, _lastLoadSequenceOpenedDir.toStdString());
    if(!files.isEmpty()){
        QString first = files.at(0);
        std::string ext = Natron::removeFileExtension(first).toStdString();
        
        std::map<std::string,std::string>::iterator found = readersForFormat.find(ext);
        if(found != readersForFormat.end()){
            Node* n = _appInstance->createNode(found->second.c_str());
            const std::vector<boost::shared_ptr<Knob> >& knobs = n->getKnobs();
            for (U32 i = 0; i < knobs.size(); ++i) {
                if (knobs[i]->typeName() == File_Knob::typeNameStatic()) {
                    boost::shared_ptr<File_Knob> fk = boost::dynamic_pointer_cast<File_Knob>(knobs[i]);
                    assert(fk);
                    if(fk->isInputImageFile()){
                        fk->setValue<QStringList>(files);
                        break;
                    }
                }
            }
        }else{
            errorDialog("Reader", "No plugin capable of decoding " + ext + " was found.");
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
    QString file = popSaveFileDialog(true, filters, _lastSaveSequenceOpenedDir.toStdString());
    if(!file.isEmpty()){
        QString fileCpy = file;
        std::string ext = Natron::removeFileExtension(fileCpy).toStdString();
        
        std::map<std::string,std::string>::iterator found = writersForFormat.find(ext);
        if(found != writersForFormat.end()){
            Node* n = _appInstance->createNode(found->second.c_str());
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
    _appInstance->autoSave();
}

bool Gui::isGraphWorthless() const{
    return _nodeGraphArea->isGraphWorthLess();
}

int Gui::saveWarning(){
    
    if(!isGraphWorthless() && !_appInstance->isSaveUpToDate()){
        QMessageBox::StandardButton ret =  QMessageBox::question(this, "",
                                                                 QString("Save changes to " + _appInstance->getCurrentProjectName() + " ?"),
                                                                 QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,QMessageBox::Save);
        if(ret == QMessageBox::Escape || ret == QMessageBox::Cancel){
            return 2;
        }else if(ret == QMessageBox::Discard){
            return 1;
        }else{
            return 0;
        }
    }
    return -1;
    
}

void Gui::errorDialog(const std::string& title,const std::string& text){
    Natron::StandardButtons buttons(Natron::Yes | Natron::No);
    if(QThread::currentThread() != QCoreApplication::instance()->thread()){
        QMutexLocker locker(&_uiUsingMainThreadMutex);
        _uiUsingMainThread = true;
        emit doDialog(0,QString(title.c_str()),QString(text.c_str()),buttons,(int)Natron::Yes);
        while(_uiUsingMainThread){
            _uiUsingMainThreadCond.wait(&_uiUsingMainThreadMutex);
        }
    }else{
        emit doDialog(0,QString(title.c_str()),QString(text.c_str()),buttons,(int)Natron::Yes);
    }
}

void Gui::warningDialog(const std::string& title,const std::string& text){
    Natron::StandardButtons buttons(Natron::Yes | Natron::No);
    if(QThread::currentThread() != QCoreApplication::instance()->thread()){
        QMutexLocker locker(&_uiUsingMainThreadMutex);
        _uiUsingMainThread = true;
        emit doDialog(1,QString(title.c_str()),QString(text.c_str()),buttons,(int)Natron::Yes);
        while(_uiUsingMainThread){
            _uiUsingMainThreadCond.wait(&_uiUsingMainThreadMutex);
        }
    }else{
        emit doDialog(1,QString(title.c_str()),QString(text.c_str()),buttons,(int)Natron::Yes);
    }
}

void Gui::informationDialog(const std::string& title,const std::string& text){
    Natron::StandardButtons buttons(Natron::Yes | Natron::No);
    if(QThread::currentThread() != QCoreApplication::instance()->thread()){
        QMutexLocker locker(&_uiUsingMainThreadMutex);
        _uiUsingMainThread = true;
        emit doDialog(2,QString(title.c_str()),QString(text.c_str()),buttons,(int)Natron::Yes);
        while(_uiUsingMainThread){
            _uiUsingMainThreadCond.wait(&_uiUsingMainThreadMutex);
        }
    }else{
        emit doDialog(2,QString(title.c_str()),QString(text.c_str()),buttons,(int)Natron::Yes);
    }
}
void Gui::onDoDialog(int type,const QString& title,const QString& content,Natron::StandardButtons buttons,int defaultB){
    
    _uiUsingMainThreadMutex.lock();

    if(type == 0){
        QMessageBox::critical(this, title, content);
    }else if(type == 1){
        QMessageBox::warning(this, title, content);
    }else if(type == 2){
        QMessageBox::information(this, title,content);
    }else{
        _lastQuestionDialogAnswer = (Natron::StandardButton)QMessageBox::question(this,title,content,
                                                                                  (QMessageBox::StandardButtons)buttons,
                                                                                  (QMessageBox::StandardButtons)defaultB);
    }
    _uiUsingMainThread = false;
    _uiUsingMainThreadCond.wakeOne();
    _uiUsingMainThreadMutex.unlock();
    
}

Natron::StandardButton Gui::questionDialog(const std::string& title,const std::string& message,Natron::StandardButtons buttons,
                                           Natron::StandardButton defaultButton) {
    if(QThread::currentThread() != QCoreApplication::instance()->thread()){
        QMutexLocker locker(&_uiUsingMainThreadMutex);
        _uiUsingMainThread = true;
        emit doDialog(3,QString(title.c_str()),QString(message.c_str()),buttons,(int)defaultButton);
        while(_uiUsingMainThread){
            _uiUsingMainThreadCond.wait(&_uiUsingMainThreadMutex);
        }
    }else{
        emit doDialog(3,QString(title.c_str()),QString(message.c_str()),buttons,(int)defaultButton);
    }
    return _lastQuestionDialogAnswer;
}


void Gui::selectNode(NodeGui* node){
    _nodeGraphArea->selectNode(node);
}

void Gui::connectInput1(){
    _nodeGraphArea->connectCurrentViewerToSelection(0);
}
void Gui::connectInput2(){
    _nodeGraphArea->connectCurrentViewerToSelection(1);
}
void Gui::connectInput3(){
    _nodeGraphArea->connectCurrentViewerToSelection(2);
}
void Gui::connectInput4(){
    _nodeGraphArea->connectCurrentViewerToSelection(3);
}
void Gui::connectInput5(){
    _nodeGraphArea->connectCurrentViewerToSelection(4);
}
void Gui::connectInput6(){
    _nodeGraphArea->connectCurrentViewerToSelection(5);
}
void Gui::connectInput7(){
    _nodeGraphArea->connectCurrentViewerToSelection(6);
}
void Gui::connectInput8(){
    _nodeGraphArea->connectCurrentViewerToSelection(7);
}
void Gui::connectInput9(){
    _nodeGraphArea->connectCurrentViewerToSelection(8);
}
void Gui::connectInput10(){
    _nodeGraphArea->connectCurrentViewerToSelection(9);
}

void RenderingProgressDialog::onFrameRendered(int frame){
    //  cout << "Current: " << frame << " (first: " << _firstFrame << " , last: " << _lastFrame << ") " << endl;
    double percent = (double)(frame - _firstFrame+1)/(double)(_lastFrame - _firstFrame+1);
    int progress = floor(percent*100);
    _totalProgress->setValue(progress);
    _perFrameLabel->setText("Frame " + QString::number(frame) + ":");
    QString title = QString::number(progress) + "% of " + _sequenceName;
    setWindowTitle(title);
    _perFrameLabel->hide();
    _perFrameProgress->hide();
    _separator->hide();
}

void RenderingProgressDialog::onCurrentFrameProgress(int progress){
    if(!_perFrameProgress->isVisible()){
        _separator->show();
        _perFrameProgress->show();
        _perFrameLabel->show();
    }
    _perFrameProgress->setValue(progress);
}

RenderingProgressDialog::RenderingProgressDialog(const QString& sequenceName,int firstFrame,int lastFrame,QWidget* parent):
    QDialog(parent),
    _sequenceName(sequenceName),
    _firstFrame(firstFrame),
    _lastFrame(lastFrame){
    
    QString title = QString::number(0) + "% of " + _sequenceName;
    setMinimumWidth(fontMetrics().width(title)+100);
    setWindowTitle(QString::number(0) + "% of " + _sequenceName);
    //setWindowFlags(Qt::WindowStaysOnTopHint);
    _mainLayout = new QVBoxLayout(this);
    setLayout(_mainLayout);
    _mainLayout->setContentsMargins(5, 5, 0, 0);
    _mainLayout->setSpacing(5);
    
    _totalLabel = new QLabel("Total progress:",this);
    _mainLayout->addWidget(_totalLabel);
    _totalProgress = new QProgressBar(this);
    _totalProgress->setRange(0, 100);
    _totalProgress->setMinimumWidth(150);
    
    _mainLayout->addWidget(_totalProgress);
    
    _separator = new QFrame(this);
    _separator->setFrameShadow(QFrame::Raised);
    _separator->setMinimumWidth(100);
    _separator->setMaximumHeight(2);
    _separator->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
    
    _mainLayout->addWidget(_separator);
    
    QString txt("Frame ");
    txt.append(QString::number(firstFrame));
    txt.append(":");
    _perFrameLabel = new QLabel(txt,this);
    _mainLayout->addWidget(_perFrameLabel);
    
    _perFrameProgress = new QProgressBar(this);
    _perFrameProgress->setMinimumWidth(150);
    _perFrameProgress->setRange(0, 100);
    _mainLayout->addWidget(_perFrameProgress);
    
    _cancelButton = new Button("Cancel",this);
    _cancelButton->setMaximumWidth(50);
    _mainLayout->addWidget(_cancelButton);
    
    QObject::connect(_cancelButton, SIGNAL(clicked()), this, SIGNAL(canceled()));

    
}


void Gui::restoreGuiGeometry(){
    QSettings settings(NATRON_ORGANIZATION_NAME,NATRON_APPLICATION_NAME);
    settings.beginGroup("MainWindow");
    
    if(settings.contains("pos")){
        QPoint pos = settings.value("pos").toPoint();
        move(pos);
    }
    if(settings.contains("size")){
        QSize size = settings.value("size").toSize();
        resize(size);
    }
    if(settings.contains("fullScreen")){
        bool fs = settings.value("fullScreen").toBool();
        if(fs)
            toggleFullScreen();
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
}

void Gui::saveGuiGeometry(){
    QSettings settings(NATRON_ORGANIZATION_NAME,NATRON_APPLICATION_NAME);
    
    settings.beginGroup("MainWindow");
    settings.setValue("pos", pos());
    settings.setValue("size", size());
    settings.setValue("fullScreen", isFullScreen());
    
    QByteArray splittersData;
    QDataStream splittersStream(&splittersData, QIODevice::WriteOnly);
    splittersStream << _leftRightSplitter->saveState();
    splittersStream << _viewerWorkshopSplitter->saveState();
    splittersStream << _middleRightSplitter->saveState();
    settings.setValue("splitters", splittersData);
    
    settings.endGroup();
    
}


void Gui::showView0(){
    _appInstance->setViewersCurrentView(0);
}
void Gui::showView1(){
    _appInstance->setViewersCurrentView(1);
}
void Gui::showView2(){
    _appInstance->setViewersCurrentView(2);
}
void Gui::showView3(){
    _appInstance->setViewersCurrentView(3);
}
void Gui::showView4(){
    _appInstance->setViewersCurrentView(4);
}
void Gui::showView5(){
    _appInstance->setViewersCurrentView(5);
}
void Gui::showView6(){
    _appInstance->setViewersCurrentView(6);
}
void Gui::showView7(){
    _appInstance->setViewersCurrentView(7);
}
void Gui::showView8(){
    _appInstance->setViewersCurrentView(8);
}
void Gui::showView9(){
    _appInstance->setViewersCurrentView(9);
}

void Gui::setCurveEditorOnTop(){
    for(std::list<TabWidget*>::iterator it = _panes.begin();it!=_panes.end();++it){
        TabWidget* cur = (*it);
        assert(cur);
        for(int i = 0; i < cur->count();++i){
            if(cur->tabAt(i) == _curveEditor){
                cur->makeCurrentTab(i);
                break;
            }
        }
    }
}

void Gui::showSettings(){
    _settingsGui->show();
}

void Gui::registerNewUndoStack(QUndoStack* stack){
    _undoStacksGroup->addStack(stack);
    QAction* undo = stack->createUndoAction(stack);
    undo->setShortcut(QKeySequence::Undo);
    QAction* redo = stack->createRedoAction(stack);
    redo->setShortcut(QKeySequence::Redo);
    _undoStacksActions.insert(std::make_pair(stack, std::make_pair(undo, redo)));
}

void Gui::removeUndoStack(QUndoStack* stack){
    std::map<QUndoStack*,std::pair<QAction*,QAction*> >::iterator it = _undoStacksActions.find(stack);
    if(it != _undoStacksActions.end()){
        _undoStacksActions.erase(it);
    }
}

void Gui::onCurrentUndoStackChanged(QUndoStack* stack){
    std::map<QUndoStack*,std::pair<QAction*,QAction*> >::iterator it = _undoStacksActions.find(stack);
    
    //the stack must have been registered first with registerNewUndoStack()
    if(it != _undoStacksActions.end()){
        setUndoRedoActions(it->second.first, it->second.second);
    }
}

