//  Powiter
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
#if QT_VERSION < 0x050000
CLANG_DIAG_OFF(unused-private-field);
#include <QtGui/qmime.h>
CLANG_DIAG_ON(unused-private-field);
#endif
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
#include <QDropEvent>

#include "Global/AppManager.h"

#include "Engine/VideoEngine.h"
#include "Engine/Settings.h"
#include "Engine/ViewerInstance.h"
#include "Engine/Project.h"
#include "Engine/OfxEffectInstance.h"
#include "Writers/Writer.h"

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
#include "Gui/DockablePanel.h"

#define PLUGIN_GROUP_DEFAULT "Other"
#define PLUGIN_GROUP_DEFAULT_ICON_PATH POWITER_IMAGES_PATH"openeffects.png"
 
using namespace Powiter;
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
_lastQuestionDialogAnswer(Powiter::No),
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
_viewersPane(0),
_nextViewerTabPlace(0),
_workshopPane(0),
_viewerWorkshopSplitter(0),
_propertiesPane(0),
_middleRightSplitter(0),
_leftRightSplitter(0),
_nodeGraphTab(0),
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
_projectGui(0)
{
    QObject::connect(this,SIGNAL(doDialog(int,QString,QString,Powiter::StandardButtons,Powiter::StandardButton)),this,
                     SLOT(onDoDialog(int,QString,QString,Powiter::StandardButtons,Powiter::StandardButton)));
    
}

Gui::~Gui()
{
    delete _nodeGraphTab;
    delete _appInstance;
    _viewerTabs.clear();
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
    if (_appInstance->getAppID() != 0) {
        delete this;
        return false;
    } else {
        delete this;
        AppManager::quit();
        qApp->quit();
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
    assert(_nodeGraphTab);
    assert(_nodeGraphTab->_nodeGraphArea);
    NodeGui* gui = _nodeGraphTab->_nodeGraphArea->createNodeGUI(_layoutPropertiesBin,node);
    return gui;
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
    assert(_nodeGraphTab);
    assert(_nodeGraphTab->_nodeGraphArea);
    if(target) {
        _nodeGraphTab->_nodeGraphArea->autoConnect(target, created);
    }
    _nodeGraphTab->_nodeGraphArea->selectNode(created);
}

NodeGui* Gui::getSelectedNode() const {
    assert(_nodeGraphTab);
    assert(_nodeGraphTab->_nodeGraphArea);
    return _nodeGraphTab->_nodeGraphArea->getSelectedNode();
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
    actionPreferences->setText(tr("Preferences..."));
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
    
    loadStyleSheet();
    
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
	actionClearDiskCache->setShortcut(QKeySequence(Qt::CTRL+Qt::SHIFT+Qt::Key_K));
	actionClearPlayBackCache = new QAction(this);
	actionClearPlayBackCache->setObjectName(QString::fromUtf8("actionClearPlayBackCache"));
	actionClearPlayBackCache->setCheckable(false);
	actionClearPlayBackCache->setShortcut(QKeySequence(Qt::CTRL+Qt::SHIFT+Qt::Key_P));
	actionClearNodeCache = new QAction(this);
	actionClearNodeCache->setObjectName(QString::fromUtf8("actionClearNodeCache"));
	actionClearNodeCache->setCheckable(false);
	actionClearNodeCache->setShortcut(QKeySequence(Qt::CTRL+Qt::SHIFT+Qt::Key_B));
    
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
    _leftRightSplitter->setChildrenCollapsible(false);
    _leftRightSplitter->setOrientation(Qt::Horizontal);
    _leftRightSplitter->setContentsMargins(0, 0, 0, 0);
    
    
    _toolBox = new QToolBar(_leftRightSplitter);
    _toolBox->setOrientation(Qt::Vertical);
    _toolBox->setMaximumWidth(40);
    
    _leftRightSplitter->addWidget(_toolBox);
    
	_viewerWorkshopSplitter = new QSplitter(_centralWidget);
    _viewerWorkshopSplitter->setContentsMargins(0, 0, 0, 0);
	_viewerWorkshopSplitter->setOrientation(Qt::Vertical);
    _viewerWorkshopSplitter->setChildrenCollapsible(false);
    QSize viewerWorkshopSplitterSize = _viewerWorkshopSplitter->sizeHint();
    QList<int> sizesViewerSplitter; sizesViewerSplitter <<  viewerWorkshopSplitterSize.height()/2;
    sizesViewerSplitter  << viewerWorkshopSplitterSize.height()/2;
    
    /*VIEWERS related*/
    
	_viewersPane = new TabWidget(this,TabWidget::NOT_CLOSABLE,_viewerWorkshopSplitter);
    _panes.push_back(_viewersPane);
    _viewersPane->resize(_viewersPane->width(), this->height()/5);
	_viewerWorkshopSplitter->addWidget(_viewersPane);
    //  _viewerWorkshopSplitter->setSizes(sizesViewerSplitter);
    
    
	/*WORKSHOP PANE*/
	//======================
	_workshopPane = new TabWidget(this,TabWidget::NOT_CLOSABLE,_viewerWorkshopSplitter);
    _panes.push_back(_workshopPane);
    /*creating Tree gui*/
    addNodeGraph();
    
	_viewerWorkshopSplitter->addWidget(_workshopPane);
    
	
    _middleRightSplitter = new QSplitter(_centralWidget);
    _middleRightSplitter->setChildrenCollapsible(false);
    _middleRightSplitter->setContentsMargins(0, 0, 0, 0);
	_middleRightSplitter->setOrientation(Qt::Horizontal);
    _middleRightSplitter->addWidget(_viewerWorkshopSplitter);
    
	/*PROPERTIES DOCK*/
	//======================
	_propertiesPane = new TabWidget(this,TabWidget::NOT_CLOSABLE,this);
    _propertiesPane->setObjectName("Properties_Pane");
    _panes.push_back(_propertiesPane);
	_propertiesScrollArea = new QScrollArea(_propertiesPane);
    _nodeGraphTab->_nodeGraphArea->setPropertyBinPtr(_propertiesScrollArea);
    _propertiesScrollArea->setObjectName("Properties_GUI");
	_propertiesContainer=new QWidget(_propertiesScrollArea);
    _propertiesContainer->setObjectName("_propertiesContainer");
	_layoutPropertiesBin=new QVBoxLayout(_propertiesContainer);
	_layoutPropertiesBin->setSpacing(0);
    _layoutPropertiesBin->setContentsMargins(0, 0, 0, 0);
	_propertiesContainer->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
	_propertiesContainer->setLayout(_layoutPropertiesBin);
    
	_propertiesScrollArea->setWidget(_propertiesContainer);
	_propertiesScrollArea->setWidgetResizable(true);
	_propertiesPane->appendTab("Properties",_propertiesScrollArea);
    
    _middleRightSplitter->addWidget(_propertiesPane);
    QSize horizontalSplitterSize = _middleRightSplitter->sizeHint();
    QList<int> sizes;
    sizes << 195;
    sizes << horizontalSplitterSize.width()- 195;
    _middleRightSplitter->setSizes(sizes);
	
    
    _leftRightSplitter->addWidget(_middleRightSplitter);
    
    _mainLayout->addWidget(_leftRightSplitter);
	
    
    _projectGui = new DockablePanel(_appInstance->getProject().get(),
                                    _layoutPropertiesBin,
                                    true,
                                    "Project Settings",
                                    "The settings of the current project.",
                                    "Rendering",
                                    _propertiesContainer);
    _projectGui->initializeKnobs();
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
	menuFile->addAction(actionPreferences);
	menuFile->addSeparator();
	menuFile->addAction(actionExit);
	
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
	retranslateUi(this);

    QObject::connect(actionFullScreen, SIGNAL(triggered()),this,SLOT(toggleFullScreen()));
    QObject::connect(actionClearDiskCache, SIGNAL(triggered()),appPTR,SLOT(clearDiskCache()));
    QObject::connect(actionClearPlayBackCache, SIGNAL(triggered()),appPTR,SLOT(clearPlaybackCache()));
    QObject::connect(actionClearNodeCache, SIGNAL(triggered()),appPTR,SLOT(clearNodeCache()));
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
    putSettingsPanelFirst(_projectGui);
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
        // cout << content.toStdString() << endl;
        setStyleSheet(content
                      .arg("rgb(243,149,0)")
                      .arg("rgb(50,50,50)")
                      .arg("rgb(71,71,71)")
                      .arg("rgb(38,38,38)")
                      .arg("rgb(200,200,200)"));
    }
}

void Gui::maximize(TabWidget* what,bool isViewerPane) {
    assert(what);
    if(what->isFloating())
        return;
    for (std::list<TabWidget*>::iterator it = _panes.begin(); it != _panes.end(); ++it) {
        if (*it != what && !(*it)->isFloating()) {
            (*it)->hide();
            if(!isViewerPane && (*it)->objectName() == "Properties_Pane")
                (*it)->show();
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
    where->appendTab(viewer->getName().c_str(),tab);
    return tab;
}

void Gui::addViewerTab(ViewerTab* tab, TabWidget* where) {
    assert(tab);
    assert(where);
    std::list<ViewerTab*>::iterator it = std::find(_viewerTabs.begin(), _viewerTabs.end(), tab);
    if (it == _viewerTabs.end()) {
        _viewerTabs.push_back(tab);
    }
    where->appendTab(tab->getInternalNode()->getName().c_str(), tab);
    
}

void Gui::removeViewerTab(ViewerTab* tab,bool initiatedFromNode,bool deleteData){
    assert(tab);
    std::list<ViewerTab*>::iterator it = std::find(_viewerTabs.begin(), _viewerTabs.end(), tab);
    if (it != _viewerTabs.end()) {
        _viewerTabs.erase(it);
    }

    if(deleteData){
        if (!initiatedFromNode) {
            assert(_nodeGraphTab);
            assert(_nodeGraphTab->_nodeGraphArea);
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
void Gui::addNodeGraph() {
    assert(!_nodeGraphTab);
    _nodeGraphTab = new NodeGraphTab(this,_workshopPane);
    assert(_workshopPane);
    _workshopPane->appendTab("Node graph",_nodeGraphTab->_nodeGraphArea);
   
}

void Gui::moveTab(QWidget* what,TabWidget *where){
    TabWidget* from = dynamic_cast<TabWidget*>(what->parentWidget());
    
    if(!from) return;
    QString name;
    if(from == where){
        /*We check that even if it is the same TabWidget, it really exists.*/
        bool found = false;
        for (int i =0; i < from->count(); ++i) {
            if (what == from->tabAt(i)) {
                found = true;
                break;
            }
        }
        if (found) {
            return;
        } else {
            assert(_nodeGraphTab);
            /*if it is not found we have to recover the name*/
            if(what == _nodeGraphTab->_nodeGraphArea)
                name = "Node Graph";
            else if(what == _propertiesScrollArea)
                name = "Properties";
            else return;
        }
    }else{
        if(what == _nodeGraphTab->_nodeGraphArea)
            name = "Node Graph";
        else if(what == _propertiesScrollArea)
            name = "Properties";
        else return;
        
    }    
    from->removeTab(what);
    where->appendTab(name, what);
}



NodeGraphTab::NodeGraphTab(Gui* gui,QWidget* parent)
: _graphScene(0)
, _nodeGraphArea(0)
{
    _graphScene = new QGraphicsScene;
	_graphScene->setItemIndexMethod(QGraphicsScene::NoIndex);
    _nodeGraphArea = new NodeGraph(gui,_graphScene,parent);

}

void Gui::splitPaneHorizontally(TabWidget* what){
    QSplitter* container = dynamic_cast<QSplitter*>(what->parentWidget());
    if(!container) return;
    
    /*We need to know the position in the container layout of the old tab widget*/
    int oldIndex = container->indexOf(what);
    
    QSplitter* newSplitter = new QSplitter(container);
    newSplitter->setContentsMargins(0, 0, 0, 0);
    newSplitter->setOrientation(Qt::Horizontal);
    what->setVisible(false);
    what->setParent(newSplitter);
    newSplitter->addWidget(what);
    what->setVisible(true);
    
    
    /*Adding now a new tab*/
    TabWidget* newTab = new TabWidget(this,TabWidget::CLOSABLE,newSplitter);
    _panes.push_back(newTab);
    newSplitter->addWidget(newTab);
    
    QSize splitterSize = newSplitter->sizeHint();
    QList<int> sizes; sizes <<   splitterSize.width()/2;
    sizes  << splitterSize.width()/2;
    newSplitter->setSizes(sizes);
    
    /*Inserting back the new splitter at the original index*/
    container->insertWidget(oldIndex,newSplitter);
    
}

void Gui::splitPaneVertically(TabWidget* what){
    QSplitter* container = dynamic_cast<QSplitter*>(what->parentWidget());
    if(!container) return;
    
    /*We need to know the position in the container layout of the old tab widget*/
    int oldIndex = container->indexOf(what);
    
    QSplitter* newSplitter = new QSplitter(container);
    newSplitter->setContentsMargins(0, 0, 0, 0);
    newSplitter->setOrientation(Qt::Vertical);
    what->setVisible(false);
    what->setParent(newSplitter);
    newSplitter->addWidget(what);
    what->setVisible(true);
    
    /*Adding now a new tab*/
    TabWidget* newTab = new TabWidget(this,TabWidget::CLOSABLE,newSplitter);
    _panes.push_back(newTab);
    newSplitter->addWidget(newTab);
    
    QSize splitterSize = newSplitter->sizeHint();
    QList<int> sizes; sizes <<   splitterSize.height()/2;
    sizes  << splitterSize.height()/2;
    newSplitter->setSizes(sizes);
    /*Inserting back the new splitter at the original index*/
    container->insertWidget(oldIndex,newSplitter);
}
void Gui::floatWidget(QWidget* what){
    
    FloatingWidget* floatingW = new FloatingWidget(this);
    const QSize& size = what->size();
    what->setVisible(false);
    what->setParent(0);
    floatingW->setWidget(size,what);
    
}

void Gui::closePane(TabWidget* what) {
    assert(what);
    /*If it is floating we do not need to re-arrange the splitters containing the tab*/
    if (what->isFloating()) {
        what->parentWidget()->close();
        what->destroyTabs();
        return;
    }
    
    QSplitter* container = dynamic_cast<QSplitter*>(what->parentWidget());
    if(!container) {
        return;
    }
    /*Removing it from the _panes vector*/
    std::list<TabWidget*>::iterator it = std::find(_panes.begin(), _panes.end(), what);
    if (it != _panes.end()) {
        _panes.erase(it);
    }
    
   
    
    /*Only sub-panes are closable. That means the splitter owning them must also
     have a splitter as parent*/
    QSplitter* mainContainer = dynamic_cast<QSplitter*>(container->parentWidget());
    if(!mainContainer) {
        return;
    }
    
    /*identifying the other tab*/
    TabWidget* other = 0;
    for (int i = 0; i < container->count(); ++i) {
        TabWidget* tab = dynamic_cast<TabWidget*>(container->widget(i));
        if (tab) {
            other = tab;
            break;
        }
    }
    
    /*Removing "what" from the container and delete it*/
    what->setVisible(false);
    what->destroyTabs();
    // delete what;
    
    /*Removing the container from the mainContainer*/
    int subSplitterIndex = 0;
    for (int i = 0; i < mainContainer->count(); ++i) {
        QSplitter* subSplitter = dynamic_cast<QSplitter*>(mainContainer->widget(i));
        if (subSplitter && subSplitter == container) {
            subSplitterIndex = i;
            container->setVisible(false);
            container->setParent(0);
            break;
        }
    }
    /*moving the other to the mainContainer*/
    if(other){
        other->setVisible(true);
        other->setParent(mainContainer);
    }
    mainContainer->insertWidget(subSplitterIndex, other);
    
    /*deleting the subSplitter*/
    delete container;
    
}

FloatingWidget::FloatingWidget(QWidget* parent)
: QWidget(parent)
, _embeddedWidget(0)
, _layout(0)
{
    setWindowFlags(Qt::WindowStaysOnTopHint | Qt::Window);
    _layout = new QVBoxLayout(this);
    _layout->setContentsMargins(0, 0, 0, 0);
    setLayout(_layout);
    
}
void FloatingWidget::setWidget(const QSize& widgetSize,QWidget* w)
{
    assert(w);
    if (_embeddedWidget) {
        return;
    }
    w->setParent(this);
    assert(_layout);
    _layout->addWidget(w);
    w->setVisible(true);
    resize(widgetSize);
    show();
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

void Gui::addPluginToolButton(const QString& actionName,
                              const QStringList& groups,
                              const QString& pluginName,
                              const QString& pluginIconPath,
                              const QString& groupIconPath){
    
    QIcon pluginIcon,groupIcon;
    if(!pluginIconPath.isEmpty() && QFile::exists(pluginIconPath)){
        pluginIcon.addFile(pluginIconPath);
    }
    if(!groupIconPath.isEmpty() && QFile::exists(groupIconPath)){
        groupIcon.addFile(groupIconPath);
    }else{
        groupIcon.addFile(PLUGIN_GROUP_DEFAULT_ICON_PATH);
    }
    QString mainGroup;
    if(groups.size()){
        mainGroup = groups[0];
    }else{
        mainGroup = PLUGIN_GROUP_DEFAULT;
        groupIcon.addFile(PLUGIN_GROUP_DEFAULT_ICON_PATH);
    }
    
    std::map<QString,ToolButton*>::iterator found =  _toolGroups.find(mainGroup);
    if(found != _toolGroups.end()){
        found->second->addTool(actionName,groups,pluginName,pluginIcon);
    }else{
        ToolButton* tb = new ToolButton(_appInstance,actionName,groups,pluginName,pluginIcon,groupIcon,_toolBox);
        _toolBox->addWidget(tb);
        tb->setToolTip(mainGroup);
        _toolGroups.insert(make_pair(mainGroup,tb));
    }
    
}
ToolButton::ToolButton(AppInstance* app,
                       const QString& actionName,
                       const QStringList& firstElement,
                       const QString& pluginName,
                       QIcon pluginIcon , QIcon groupIcon ,
                       QWidget* parent)
: QToolButton(parent)
, _app(app)
, _menu(0)
, _subMenus()
, _actions()
{
    setPopupMode(QToolButton::InstantPopup);
    _menu = new QMenu(this);
    if(!groupIcon.isNull())
        setIcon(groupIcon);
    setMenu(_menu);
    setMaximumSize(35,35);
    
    QMenu* _lastMenu = _menu;
    for (int i = 1; i < firstElement.size(); ++i) {
        _lastMenu = _lastMenu->addMenu(firstElement[i]);
        _subMenus.push_back(_lastMenu);
    }
    QAction* action = 0;
    if(pluginIcon.isNull()){
        action = _lastMenu->addAction(pluginName);
    }else{
        action = _lastMenu->addAction(pluginIcon, pluginName);
    }
    ActionRef* actionRef = new ActionRef(_app,action,actionName);
    _actions.push_back(actionRef);
    
}
ToolButton::~ToolButton(){
    for(U32 i = 0; i < _actions.size() ; ++i) {
        delete _actions[i];
    }
}

void ToolButton::addTool(const QString& actionName,const QStringList& grouping,const QString& pluginName,QIcon pluginIcon){
    std::vector<std::string> subMenuToAdd;
    int index = 1;
    QMenu* lastMenu = _menu;
    while(index < (int)grouping.size()){
        bool found = false;
        for (U32 i =  0; i < _subMenus.size(); ++i) {
            if (_subMenus[i]->title() == QString(grouping[index])) {
                lastMenu = _subMenus[i];
                found = true;
                break;
            }
        }
        if(!found)
            break;
        ++index;
    }
    for(int i = index; i < (int)grouping.size() ; ++i) {
        QMenu* menu = lastMenu->addMenu(grouping[index]);
        _subMenus.push_back(menu);
        lastMenu = menu;
    }
    QAction* action = 0;
    if(pluginIcon.isNull()){
        action = lastMenu->addAction(pluginName);
    }else{
        action = lastMenu->addAction(pluginIcon, pluginName);
    }
    ActionRef* actionRef = new ActionRef(_app,action,actionName);
    _actions.push_back(actionRef);
}
void ActionRef::onTriggered(){
    _app->createNode(_nodeName);
}
void Gui::addUndoRedoActions(QAction* undoAction,QAction* redoAction){
    menuEdit->addAction(undoAction);
	menuEdit->addAction(redoAction);
}
void Gui::newProject(){
    appPTR->newAppInstance(false);
}
void Gui::openProject(){
    std::vector<std::string> filters;
    filters.push_back(POWITER_PROJECT_FILE_EXTENION);
    SequenceFileDialog dialog(this,filters,false,
                              SequenceFileDialog::OPEN_DIALOG);
    if(dialog.exec()){
        QStringList selectedFiles = dialog.selectedFiles();
        if (selectedFiles.size() > 0) {
            //clearing current graph
            _appInstance->clearNodes();
            QString file = selectedFiles.at(0);
            QString name = SequenceFileDialog::removePath(file);
            QString path = file.left(file.indexOf(name));
            
            _appInstance->loadProject(path,name);
        }
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
    filter.push_back(POWITER_PROJECT_FILE_EXTENION);
    SequenceFileDialog dialog(this,filter,false,SequenceFileDialog::SAVE_DIALOG);
    QString outFile;
    if(dialog.exec()){
        outFile = dialog.filesToSave();
    }
    if (outFile.size() > 0) {
        if (outFile.indexOf("." POWITER_PROJECT_FILE_EXTENION) == -1) {
            outFile.append("." POWITER_PROJECT_FILE_EXTENION);
        }
        QString file = SequenceFileDialog::removePath(outFile);
        QString path = outFile.left(outFile.indexOf(file));
        _appInstance->saveProject(path,file,false);
    }
}

void Gui::autoSave(){
    _appInstance->autoSave();
}

bool Gui::isGraphWorthless() const{
    return _nodeGraphTab->_nodeGraphArea->isGraphWorthLess();
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
    QMutexLocker locker(&_uiUsingMainThreadMutex);
    _uiUsingMainThread = true;
    Powiter::StandardButtons buttons(Powiter::Yes | Powiter::No);
    emit doDialog(0,QString(title.c_str()),QString(text.c_str()),buttons,Powiter::Yes);
    while(_uiUsingMainThread){
        _uiUsingMainThreadCond.wait(&_uiUsingMainThreadMutex);
    }
}

void Gui::warningDialog(const std::string& title,const std::string& text){
    QMutexLocker locker(&_uiUsingMainThreadMutex);
    _uiUsingMainThread = true;
    Powiter::StandardButtons buttons(Powiter::Yes | Powiter::No);
    emit doDialog(1,QString(title.c_str()),QString(text.c_str()),buttons,Powiter::Yes);
    while(_uiUsingMainThread){
        _uiUsingMainThreadCond.wait(&_uiUsingMainThreadMutex);
    }
}

void Gui::informationDialog(const std::string& title,const std::string& text){
    QMutexLocker locker(&_uiUsingMainThreadMutex);
    _uiUsingMainThread = true;
    Powiter::StandardButtons buttons(Powiter::Yes | Powiter::No);
    emit doDialog(2,QString(title.c_str()),QString(text.c_str()),buttons,Powiter::Yes);
    while(_uiUsingMainThread){
        _uiUsingMainThreadCond.wait(&_uiUsingMainThreadMutex);
    }
}
void Gui::onDoDialog(int type,const QString& title,const QString& content,Powiter::StandardButtons buttons,Powiter::StandardButton defaultB){
    QMutexLocker locker(&_uiUsingMainThreadMutex);
    if(type == 0){
        QMessageBox::critical(this, title, content);
    }else if(type == 1){
        QMessageBox::warning(this, title, content);
    }else if(type == 2){
        QMessageBox::information(this, title,content);
    }else{
        _lastQuestionDialogAnswer = (Powiter::StandardButton)QMessageBox::question(this,title,content,
                                                                                   (QMessageBox::StandardButtons)buttons,
                                                                                   (QMessageBox::StandardButtons)defaultB);
    }
    _uiUsingMainThread = false;
    _uiUsingMainThreadCond.wakeOne();
}

Powiter::StandardButton Gui::questionDialog(const std::string& title,const std::string& message,Powiter::StandardButtons buttons,
                                       Powiter::StandardButton defaultButton) {
    QMutexLocker locker(&_uiUsingMainThreadMutex);
    _uiUsingMainThread = true;
    emit doDialog(3,QString(title.c_str()),QString(message.c_str()),buttons,defaultButton);
    while(_uiUsingMainThread){
        _uiUsingMainThreadCond.wait(&_uiUsingMainThreadMutex);
    }
    return _lastQuestionDialogAnswer;
}

void Gui::selectNode(NodeGui* node){
    _nodeGraphTab->_nodeGraphArea->selectNode(node);
}

void Gui::connectInput1(){
    _nodeGraphTab->_nodeGraphArea->connectCurrentViewerToSelection(0);
}
void Gui::connectInput2(){
    _nodeGraphTab->_nodeGraphArea->connectCurrentViewerToSelection(1);
}
void Gui::connectInput3(){
    _nodeGraphTab->_nodeGraphArea->connectCurrentViewerToSelection(2);
}
void Gui::connectInput4(){
    _nodeGraphTab->_nodeGraphArea->connectCurrentViewerToSelection(3);
}
void Gui::connectInput5(){
    _nodeGraphTab->_nodeGraphArea->connectCurrentViewerToSelection(4);
}
void Gui::connectInput6(){
    _nodeGraphTab->_nodeGraphArea->connectCurrentViewerToSelection(5);
}
void Gui::connectInput7(){
    _nodeGraphTab->_nodeGraphArea->connectCurrentViewerToSelection(6);
}
void Gui::connectInput8(){
    _nodeGraphTab->_nodeGraphArea->connectCurrentViewerToSelection(7);
}
void Gui::connectInput9(){
    _nodeGraphTab->_nodeGraphArea->connectCurrentViewerToSelection(8);
}
void Gui::connectInput10(){
    _nodeGraphTab->_nodeGraphArea->connectCurrentViewerToSelection(9);
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

void RenderingProgressDialog::onCancelation(){
    /*Maybe we should ask for user permission with another dialog ? */
    _writer->abortRendering();
    hide();
    delete this;
    
}
RenderingProgressDialog::RenderingProgressDialog(Writer* writer,const QString& sequenceName,int firstFrame,int lastFrame,QWidget* parent):
QDialog(parent),
_writer(writer),
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
    VideoEngine* videoEngine = writer->getVideoEngine().get();
    QObject::connect(_cancelButton, SIGNAL(clicked()), this, SLOT(onCancelation()));
    QObject::connect(videoEngine, SIGNAL(frameRendered(int)), this, SLOT(onFrameRendered(int)));
    QObject::connect(videoEngine, SIGNAL(progressChanged(int)),this,SLOT(onCurrentFrameProgress(int)));
    QObject::connect(videoEngine, SIGNAL(engineStopped()), this, SLOT(onCancelation()));
    
}
void Gui::showProgressDialog(Writer* writer,const QString& sequenceName,int firstFrame,int lastFrame){
    RenderingProgressDialog* dialog = new RenderingProgressDialog(writer,sequenceName,firstFrame,lastFrame,this);
    dialog->show();
}

void Gui::restoreGuiGeometry(){
    QSettings settings(POWITER_ORGANIZATION_NAME,POWITER_APPLICATION_NAME);
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
    QSettings settings(POWITER_ORGANIZATION_NAME,POWITER_APPLICATION_NAME);
    
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


AddFormatDialog::AddFormatDialog(QWidget* parent):QDialog(parent)
{
    _mainLayout = new QVBoxLayout(this);
    _mainLayout->setSpacing(0);
    _mainLayout->setContentsMargins(5, 5, 0, 0);
    setLayout(_mainLayout);
    setWindowTitle("New Format");
    
    _secondLine = new QWidget(this);
    _secondLineLayout = new QHBoxLayout(_secondLine);
    _mainLayout->addWidget(_secondLine);
    
    _widthLabel = new QLabel("w:",_secondLine);
    _secondLineLayout->addWidget(_widthLabel);
    _widthSpinBox = new SpinBox(this,SpinBox::INT_SPINBOX);
    _widthSpinBox->setMaximum(99999);
    _widthSpinBox->setMinimum(1);
    _widthSpinBox->setValue(1);
    _secondLineLayout->addWidget(_widthSpinBox);
    
    
    _heightLabel = new QLabel("h:",_secondLine);
    _secondLineLayout->addWidget(_heightLabel);
    _heightSpinBox = new SpinBox(this,SpinBox::INT_SPINBOX);
    _heightSpinBox->setMaximum(99999);
    _heightSpinBox->setMinimum(1);
    _heightSpinBox->setValue(1);
    _secondLineLayout->addWidget(_heightSpinBox);
    
    
    _pixelAspectLabel = new QLabel("pixel aspect:",_secondLine);
    _secondLineLayout->addWidget(_pixelAspectLabel);
    _pixelAspectSpinBox = new SpinBox(this,SpinBox::DOUBLE_SPINBOX);
    _pixelAspectSpinBox->setMinimum(0.);
    _pixelAspectSpinBox->setValue(1.);
    _secondLineLayout->addWidget(_pixelAspectSpinBox);

    
    _thirdLine = new QWidget(this);
    _thirdLineLayout = new QHBoxLayout(_thirdLine);
    _thirdLine->setLayout(_thirdLineLayout);
    _mainLayout->addWidget(_thirdLine);

    
    _nameLabel = new QLabel("Name:",_thirdLine);
    _thirdLineLayout->addWidget(_nameLabel);
    _nameLineEdit = new LineEdit(_thirdLine);
    _thirdLineLayout->addWidget(_nameLineEdit);
    
    _fourthLine = new QWidget(this);
    _fourthLineLayout = new QHBoxLayout(_fourthLine);
    _fourthLine->setLayout(_fourthLineLayout);
    _mainLayout->addWidget(_fourthLine);
    
    
    _cancelButton = new Button("Cancel",_fourthLine);
    QObject::connect(_cancelButton, SIGNAL(clicked()), this, SLOT(reject()));
    _fourthLineLayout->addWidget(_cancelButton);
    
    _okButton = new Button("Ok",_fourthLine);
    QObject::connect(_okButton, SIGNAL(clicked()), this, SLOT(accept()));
    _fourthLineLayout->addWidget(_okButton);
    
}

Format AddFormatDialog::getFormat() const{
    int w = (int)_widthSpinBox->value();
    int h = (int)_heightSpinBox->value();
    double pa = _pixelAspectSpinBox->value();
    QString name = _nameLineEdit->text();
    return Format(0,0,w,h,name.toStdString(),pa);
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

