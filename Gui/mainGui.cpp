//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com
#include "Gui/mainGui.h"
#include "Gui/texturecache.h"
#include "Superviser/controler.h"
#include "Gui/GLViewer.h"
#include "Core/model.h"
#include "Core/VideoEngine.h"
#include <QtWidgets/QtWidgets>
#include "Core/settings.h"
#include <cassert>
#include "Gui/tabwidget.h"
#include "Gui/FeedbackSpinBox.h"
#include "Gui/timeline.h"
#include "Gui/DAG.h"
#include "Gui/viewerTab.h"

using namespace std;

Gui::Gui(QWidget* parent):QMainWindow(parent),_textureCache(0),
actionNew_project(0),
actionOpen_project(0),
actionSave_project(0),
actionPreferences(0),
actionExit(0),
actionUndo(0),
actionRedo(0),
actionProject_settings(0),
actionSplitViewersTab(0),
actionClearDiskCache(0),
actionClearPlayBackCache(0),
actionClearNodeCache(0),
actionClearTextureCache(0),
centralwidget(0),
verticalLayout(0),
centralFrame(0),
verticalLayout_2(0),
splitter(0),
viewersTabContainer(0),
WorkShop(0),
GraphEditor(0),
verticalLayout_3(0),
graph_scene(0),
nodeGraphArea(0),
scrollAreaWidgetContents(0),
CurveEditor(0),
progressBar(0),
menubar(0),
menuFile(0),
menuEdit(0),
menuDisplay(0),
menuOptions(0),
viewersMenu(0),
cacheMenu(0),
ToolDock(0),
ToolDockContent(0),
verticalLayout_5(0),
ToolChooser(0),
rightDock(0),
PropertiesDock(0),
PropertiesDockContent(0),
propertiesContainer(0),
layout_settings(0),
statusbar(0),
_currentViewerTab(0)
{
    
}
Gui::~Gui(){
    
    delete _textureCache;
    
}
void Gui::exit(){
    ctrlPTR->getModel()->getVideoEngine()->abort();
	ctrlPTR->Destroy();
    delete this;
    qApp->exit(0);
    
}
void Gui::closeEvent(QCloseEvent *e){
    // save project ...
    exit();
    e->accept();
}


void Gui::createNodeGUI(UI_NODE_TYPE type, Node* node,double x,double y){
    nodeGraphArea->createNodeGUI(layout_settings,type,node,x,y);
    
}


void Gui::createGui(){
    
    
    setupUi();
    
    QObject::connect(QCoreApplication::instance(), SIGNAL(aboutToQuit()), this, SLOT(exit()));
    //nodeGraphArea->installEventFilter(this);
    //  viewer_tab->installEventFilter(this);
    //rightDock->installEventFilter(this);
    nodeGraphArea->grabKeyboard();
    nodeGraphArea->releaseKeyboard();
    nodeGraphArea->setFocus();
}

void Gui::keyPressEvent(QKeyEvent *e){
    
    
    if(e->modifiers().testFlag(Qt::AltModifier) &&
       e->modifiers().testFlag(Qt::ControlModifier) &&
       e->key() == Qt::Key_K){
        
    }
    QMainWindow::keyPressEvent(e);
}

bool Gui::eventFilter(QObject *target, QEvent *event){
    
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Right) {
            ctrlPTR->getModel()->getVideoEngine()->nextFrame();
            focusNextChild();
            return true;
        }else if(keyEvent->key() == Qt::Key_Left){
            ctrlPTR->getModel()->getVideoEngine()->previousFrame();
            focusNextChild();
            return true;
        }
    }
    return QMainWindow::eventFilter(target, event);;
}
void Gui::retranslateUi(QMainWindow *MainWindow)
{
	setWindowTitle(QApplication::translate("Powiter", "Powiter"));
	actionNew_project->setText(QApplication::translate("Powiter", "New project"));
	actionOpen_project->setText(QApplication::translate("Powiter", "Open project"));
	actionSave_project->setText(QApplication::translate("Powiter", "Save project"));
	actionPreferences->setText(QApplication::translate("Powiter", "Preferences"));
	actionExit->setText(QApplication::translate("Powiter", "Exit"));
	actionUndo->setText(QApplication::translate("Powiter", "Undo"));
	actionRedo->setText(QApplication::translate("Powiter", "Redo"));
	actionProject_settings->setText(QApplication::translate("Powiter", "Project settings"));
	actionSplitViewersTab->setText(QApplication::translate("Powiter","Toggle multi-view area"));
	actionClearDiskCache->setText(QApplication::translate("Powiter","Clear disk cache"));
	actionClearPlayBackCache->setText(QApplication::translate("Powiter","Clear playback cache"));
	actionClearNodeCache ->setText(QApplication::translate("Powiter","Clear per-node cache"));
    actionClearTextureCache ->setText(QApplication::translate("Powiter","Clear texture cache"));
    

	WorkShop->setTabText(WorkShop->indexOf(CurveEditor), QApplication::translate("Powiter", "Motion Editor"));
    
	WorkShop->setTabText(WorkShop->indexOf(GraphEditor), QApplication::translate("Powiter", "Graph Editor"));
	menuFile->setTitle(QApplication::translate("Powiter", "File"));
	menuEdit->setTitle(QApplication::translate("Powiter", "Edit"));
	menuDisplay->setTitle(QApplication::translate("Powiter", "Display"));
	menuOptions->setTitle(QApplication::translate("Powiter", "Options"));
	viewersMenu->setTitle(QApplication::translate("Powiter", "Viewer(s)"));
	cacheMenu->setTitle(QApplication::translate("Powiter", "Cache"));
    
} // retranslateUi
void Gui::setupUi()
{
	if (objectName().isEmpty())
		setObjectName(QString::fromUtf8("MainWindow"));
	QDesktopWidget* desktop=QApplication::desktop();
	QRect screen=desktop->screenGeometry(-1);
	resize(screen.width(),screen.height());
	setDockNestingEnabled(false);
	setStyleSheet("QPushButton{background-color:rgba(71,71,71,255);}"
                  "QMainWindow{background-color:rgba(50,50,50,255);}"
                  "QGraphicsView{background-color:rgba(71,71,71,255);}"
                  "QWidget#viewport{background-color:rgba(71,71,71,255);}"
                  "QGroupBox{color:rgba(200,200,200,255);background-color:rgba(50,50,50,255);border:0px;}"
                  "QComboBox{color:rgba(200,200,200,255);background-color:rgba(71,71,71,255);"
                  "selection-color:rgba(243,149,0,255);selection-background-color:rgba(71,71,71,255);}"
                  "QComboBox QAbstractItemView{"
                  "border-radius:0px;"
                  "border:0px;selection-background-color:rgba(71,71,71,255);"
                  "background:rgba(71,71,71,255);color:rgb(200,200,200);"
                  "}"
                  "QDockWidget::title{background:rgb(71,71,71);}QDockWidget{color:rgb(200,200,200);}"
                  "QTabBar::tab{background:rgb(71,71,71);}QTabBar{color:rgb(200,200,200);}"
                  "QTabBar::tab:selected{background:rgb(31,31,31);}"
                  "QTabWidget::pane{border:0px;}"
                  "QLineEdit{border:0px}"
                  "");
	/*TOOL BAR ACTIONS*/
	//======================
	actionNew_project = new QAction(this);
	actionNew_project->setObjectName(QString::fromUtf8("actionNew_project"));
	actionOpen_project = new QAction(this);
	actionOpen_project->setObjectName(QString::fromUtf8("actionOpen_project"));
	actionSave_project = new QAction(this);
	actionSave_project->setObjectName(QString::fromUtf8("actionSave_project"));
	actionPreferences = new QAction(this);
	actionPreferences->setObjectName(QString::fromUtf8("actionPreferences"));
	actionExit = new QAction(this);
	actionExit->setObjectName(QString::fromUtf8("actionExit"));
	actionUndo = new QAction(this);
	actionUndo->setObjectName(QString::fromUtf8("actionUndo"));
	actionRedo = new QAction(this);
	actionRedo->setObjectName(QString::fromUtf8("actionRedo"));
	actionProject_settings = new QAction(this);
	actionProject_settings->setObjectName(QString::fromUtf8("actionProject_settings"));
	actionSplitViewersTab=new QAction(this);
	actionSplitViewersTab->setObjectName(QString::fromUtf8("actionSplitViewersTab"));
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
    actionClearTextureCache = new QAction(this);
	actionClearTextureCache->setObjectName(QString::fromUtf8("actionClearTextureCache"));
	actionClearTextureCache->setCheckable(false);
	actionClearTextureCache->setShortcut(QKeySequence(Qt::CTRL+Qt::SHIFT+Qt::Key_T));
    
    
	/*CENTRAL AREA*/
	//======================
	centralwidget = new QWidget(this);
	centralwidget->setObjectName(QString::fromUtf8("centralwidget"));
	verticalLayout = new QVBoxLayout(centralwidget);
	verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
	centralFrame = new QFrame(centralwidget);
	centralFrame->setObjectName(QString::fromUtf8("centralFrame"));
	centralFrame->setFrameShape(QFrame::StyledPanel);
	centralFrame->setFrameShadow(QFrame::Raised);
	verticalLayout_2 = new QVBoxLayout(centralFrame);
	verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
	splitter = new QSplitter(centralFrame);
	splitter->setObjectName(QString::fromUtf8("splitter"));
	splitter->setOrientation(Qt::Vertical);
    
    /*VIEWERS related*/
    _textureCache = new TextureCache(Settings::getPowiterCurrentSettings()->_cacheSettings.maxTextureCache);
	viewersTabContainer = new TabWidget(splitter);
    viewersTabContainer->resize(viewersTabContainer->width(), screen.height()/4);
    QObject::connect(viewersTabContainer, SIGNAL(currentChanged(int)), this, SLOT(makeCurrentViewerTab(int)));
    QObject::connect(viewersTabContainer, SIGNAL(tabCloseRequested(int)), this, SLOT(removeViewerTab(int)));
	viewersTabContainer->setObjectName(QString::fromUtf8("viewersTabContainer"));
	splitter->addWidget(viewersTabContainer);
    
	/*initializing texture cache*/
	
	// splitter->setCollapsible(0,false);
    
	/*GRAPH EDITOR*/
	//======================
	WorkShop = new TabWidget(splitter);
	//WorkShop->setStyleSheet("background-color: rgba(50,50,50,255);");
    
	WorkShop->setObjectName(QString::fromUtf8("WorkShop"));
    
	GraphEditor = new QWidget(WorkShop);
	GraphEditor->setObjectName(QString::fromUtf8("GraphEditor"));
	verticalLayout_3 = new QVBoxLayout(GraphEditor);
	verticalLayout_3->setObjectName(QString::fromUtf8("verticalLayout_3"));
	graph_scene=new QGraphicsScene(GraphEditor);
	graph_scene->setItemIndexMethod(QGraphicsScene::NoIndex);
	QWidget* viewport=new QWidget(WorkShop);
	viewport->setObjectName(QString::fromUtf8("viewport"));
	// viewport->setStyleSheet(QString("QWidget#viewport{background-color:silver}"));
	//  viewport->setStyleSheet(QString("QWidget#viewport{background-color: rgba(50,50,50,255);}"));
    
	verticalLayout_3->addWidget(viewport);
	nodeGraphArea = new NodeGraph(graph_scene,GraphEditor);
	nodeGraphArea->setObjectName(QString::fromUtf8("nodeGraphArea"));
	nodeGraphArea->setViewport(viewport);
	verticalLayout_3->addWidget(nodeGraphArea);
    
	verticalLayout_3->setContentsMargins(0, 0, 0, 0);
	WorkShop->addTab(GraphEditor, QString());
    
	/*CURVE EDITOR*/
	//======================
	CurveEditor = new QWidget(WorkShop);
	CurveEditor->setObjectName(QString::fromUtf8("CurveEditor"));
	WorkShop->addTab(CurveEditor, QString());
	WorkShop->setContentsMargins(0, 0, 0, 0);
	splitter->addWidget(WorkShop);
	splitter->setContentsMargins(0, 0, 0, 0);
	// splitter->setCollapsible(1,false);
	verticalLayout_2->addWidget(splitter);
	/*PROGRESS BAR*/
	//======================
    
	progressBar = new QProgressBar(centralFrame);
	progressBar->setObjectName(QString::fromUtf8("progressBar"));
	progressBar->setValue(0);
	progressBar->setVisible(false);
    
	verticalLayout_2->addWidget(progressBar);
	verticalLayout_2->setContentsMargins(0, 0, 0, 0);
	verticalLayout->addWidget(centralFrame);
	verticalLayout->setContentsMargins(0, 0, 0, 0);
    
	setCentralWidget(centralwidget);
	/*TOOL BAR menus*/
	//======================
	menubar = new QMenuBar(this);
	menubar->setObjectName(QString::fromUtf8("menubar"));
	menubar->setGeometry(QRect(0, 0, 1159, 21));
	menuFile = new QMenu(menubar);
	menuFile->setObjectName(QString::fromUtf8("menuFile"));
	menuEdit = new QMenu(menubar);
	menuEdit->setObjectName(QString::fromUtf8("menuEdit"));
	menuDisplay = new QMenu(menubar);
	menuDisplay->setObjectName(QString::fromUtf8("menuDisplay"));
	menuOptions = new QMenu(menubar);
	menuOptions->setObjectName(QString::fromUtf8("menuOptions"));
	viewersMenu= new QMenu(menuDisplay);
	viewersMenu->setObjectName(QString::fromUtf8("viewersMenu"));
	cacheMenu= new QMenu(menubar);
	cacheMenu->setObjectName(QString::fromUtf8("cacheMenu"));
    
	setMenuBar(menubar);
    
	/*RIGHT DOCK*/
	//======================
	rightDock = new QDockWidget(this);
	rightDock->setFeatures(QDockWidget::DockWidgetMovable);
	// rightDock->setStyleSheet("background-color: rgba(50,50,50,255);");
	QWidget* rightContent = new QWidget(rightDock);
	QSplitter* rightSplitter = new QSplitter(Qt::Vertical,rightDock);
	QVBoxLayout* rightContentLayout = new QVBoxLayout(rightContent);
	rightContent->setStyleSheet("background-color:rgba(50,50,50,255);");
    
	/*PROPERTIES DOCK*/
	//======================
	PropertiesDock = new QDockWidget(this);
	PropertiesDock->setStyleSheet("background-color:rgba(50,50,50,255);");
    
	PropertiesDock->setObjectName(QString::fromUtf8("PropertiesDock"));
	PropertiesDock->setWindowTitle("Properties");
	PropertiesDock->setFeatures(QDockWidget::NoDockWidgetFeatures);
    
	PropertiesDockContent = new QScrollArea(PropertiesDock);
	//PropertiesDockContent->setStyleSheet("background-color:rgba(50,50,50,255);");
    
	propertiesContainer=new QWidget(PropertiesDockContent);
	// propertiesContainer->setStyleSheet("background-color:rgba(50,50,50,255);");
	propertiesContainer->setObjectName("propertiesContainer");
	layout_settings=new QVBoxLayout(propertiesContainer);
	layout_settings->setSpacing(0);
	propertiesContainer->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
	propertiesContainer->setLayout(layout_settings);
	// propertiesContainer->setStyleSheet(QString("QWidget#propertiesContainer{background-color:silver}"));
	PropertiesDockContent->setWidget(propertiesContainer);
	PropertiesDockContent->setWidgetResizable(true);
	PropertiesDockContent->setObjectName(QString::fromUtf8("PropertiesDockContent"));
	// PropertiesDockContent->setStyleSheet(QString("QScrollArea#PropertiesDockContent{background-color:silver}"));
	PropertiesDock->setWidget(PropertiesDockContent);
	statusbar = new QStatusBar(this);
	statusbar->setObjectName(QString::fromUtf8("statusbar"));
	setStatusBar(statusbar);
    
	menubar->addAction(menuFile->menuAction());
	menubar->addAction(menuEdit->menuAction());
	menubar->addAction(menuDisplay->menuAction());
	menubar->addAction(menuOptions->menuAction());
	menubar->addAction(cacheMenu->menuAction());
	menuFile->addAction(actionNew_project);
	menuFile->addAction(actionOpen_project);
	menuFile->addAction(actionSave_project);
	menuFile->addSeparator();
	menuFile->addAction(actionPreferences);
	menuFile->addSeparator();
	menuFile->addAction(actionExit);
	menuEdit->addAction(actionUndo);
	menuEdit->addAction(actionRedo);
	menuOptions->addAction(actionProject_settings);
	menuDisplay->addAction(viewersMenu->menuAction());
	viewersMenu->addAction(actionSplitViewersTab);
	cacheMenu->addAction(actionClearDiskCache);
	cacheMenu->addAction(actionClearPlayBackCache);
	cacheMenu->addAction(actionClearNodeCache);
    cacheMenu->addAction(actionClearTextureCache);
	retranslateUi(this);
    
	WorkShop->setCurrentIndex(0);
    
    
	/*TOOL dock*/
	//======================
	ToolDock = new QDockWidget(this);
	// ToolDock->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
	ToolDock->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Fixed);
	ToolDock->setMaximumHeight(100);
	// ToolDock->setMaximumSize(RIGHTDOCK_WIDTH,100);
	ToolDock->setObjectName(QString::fromUtf8("ToolDock"));
	ToolDock->setWindowTitle("Tools");
	ToolDock->setFeatures(QDockWidget::DockWidgetMovable);
	ToolDockContent = new QWidget(ToolDock);
	ToolDockContent->setObjectName(QString::fromUtf8("ToolDockContent"));
	verticalLayout_5 = new QVBoxLayout(ToolDockContent);
	verticalLayout_5->setObjectName(QString::fromUtf8("verticalLayout_5"));
	ToolChooser = new QTreeView(ToolDockContent);
	ToolChooser->setObjectName(QString::fromUtf8("ToolChooser"));
	verticalLayout_5->addWidget(ToolChooser);
    
	ToolDock->setWidget(ToolDockContent);
	//  addDockWidget(Qt::RightDockWidgetArea, ToolDock);    // < ADDING TOOL DOCK
    
	/*ADDING RIGHT DOCK TO MAIN WINDOW*/
	rightSplitter->addWidget(PropertiesDock);
	rightSplitter->addWidget(ToolDock);
	rightContentLayout->addWidget(rightSplitter);
	rightContent->setLayout(rightContentLayout);
	rightDock->setWidget(rightContent);
	rightDock->setAllowedAreas(Qt::RightDockWidgetArea | Qt::LeftDockWidgetArea);
	addDockWidget(Qt::RightDockWidgetArea, rightDock);
	rightDock->setMinimumWidth(RIGHTDOCK_WIDTH);
    QObject::connect(actionClearTextureCache, SIGNAL(triggered()),this,SLOT(clearTexCache()));
    VideoEngine* vengine = ctrlPTR->getModel()->getVideoEngine();
    QObject::connect(actionClearDiskCache, SIGNAL(triggered()),vengine,SLOT(clearDiskCache()));
    QObject::connect(actionClearPlayBackCache, SIGNAL(triggered()),vengine,SLOT(clearPlayBackCache()));
    QObject::connect(actionClearNodeCache, SIGNAL(triggered()),vengine,SLOT(clearRowCache()));
    
	QMetaObject::connectSlotsByName(this);
    
} // setupUi

void Gui::clearTextureCache(){
    U32 currentTexture = currentViewer->viewer->getCurrentTexture();
    _textureCache->clearCache(currentTexture);
}

void Gui::addViewerTab(){
    ViewerTab* tab = new ViewerTab(viewersTabContainer);
    _viewerTabs.push_back(tab);
    tab->setTextureCache(_textureCache);
    int index = _viewerTabs.size();
    QString name("Viewer ");
    char tmp[50];
    sprintf(tmp, "%i",index);
    name.append(tmp);
    viewersTabContainer->addTab(tab, name);
    viewersTabContainer->setCurrentIndex(index);
    makeCurrentViewerTab(tab);
}

void Gui::removeViewerTab(ViewerTab* tab){
    /*making current another viewer*/
    if (tab == currentViewer) {
        ViewerTab* otherViewer = 0;
        for(U32 i = 0 ; i < _viewerTabs.size() ; i++){
            if (_viewerTabs[i] != tab) {
                otherViewer = _viewerTabs[i];
                break;
            }
        }
        if(otherViewer)
            makeCurrentViewerTab(otherViewer);
        else
            makeNoCurrentViewerTab();
    }
    int index = -1;
    for(U32 i = 0 ; i < _viewerTabs.size() ; i++){
        if (_viewerTabs[i] == tab) {
            _viewerTabs.erase(_viewerTabs.begin()+i);
            index = i;
            break;
        }
    }
    if(index != -1){
        viewersTabContainer->removeTab(index);
        delete tab;
    }
    
}
void Gui::makeNoCurrentViewerTab(){
    ViewerTab* currentTab  = currentViewer;
    VideoEngine* vengine = ctrlPTR->getModel()->getVideoEngine();
    if(vengine && currentTab){
        QObject::disconnect(vengine, SIGNAL(fpsChanged(double)), currentTab->fpsBox, SLOT(setValue(double)));
        QObject::disconnect(currentTab->fpsBox, SIGNAL(valueChanged(double)), vengine, SLOT(setDesiredFPS(double)));
        QObject::disconnect(currentTab->play_Forward_Button,SIGNAL(toggled(bool)),vengine,SLOT(startPause(bool)));
        QObject::disconnect(currentTab->stop_Button,SIGNAL(clicked()),vengine,SLOT(abort()));
        QObject::disconnect(currentTab->play_Backward_Button,SIGNAL(toggled(bool)),vengine,SLOT(startBackward(bool)));
        QObject::disconnect(currentTab->previousFrame_Button,SIGNAL(clicked()),vengine,SLOT(previousFrame()));
        QObject::disconnect(currentTab->nextFrame_Button,SIGNAL(clicked()),vengine,SLOT(nextFrame()));
        QObject::disconnect(currentTab->previousIncrement_Button,SIGNAL(clicked()),vengine,SLOT(previousIncrement()));
        QObject::disconnect(currentTab->nextIncrement_Button,SIGNAL(clicked()),vengine,SLOT(nextIncrement()));
        QObject::disconnect(currentTab->firstFrame_Button,SIGNAL(clicked()),vengine,SLOT(firstFrame()));
        QObject::disconnect(currentTab->lastFrame_Button,SIGNAL(clicked()),vengine,SLOT(lastFrame()));
        QObject::disconnect(currentTab->frameNumberBox,SIGNAL(valueChanged(double)),vengine,SLOT(seekRandomFrame(double)));
        QObject::disconnect(currentTab->frameSeeker,SIGNAL(positionChanged(int)), vengine, SLOT(seekRandomFrameWithStart(int)));}
    _currentViewerTab = NULL;
}

void Gui::makeCurrentViewerTab(ViewerTab *tab){
    ViewerTab* currentTab  = currentViewer;
    if(tab == currentTab) return;
    VideoEngine* vengine = ctrlPTR->getModel()->getVideoEngine();
    if(currentTab){
        QObject::disconnect(vengine, SIGNAL(fpsChanged(double)), currentTab->fpsBox, SLOT(setValue(double)));
        QObject::disconnect(currentTab->fpsBox, SIGNAL(valueChanged(double)), vengine, SLOT(setDesiredFPS(double)));
        QObject::disconnect(currentTab->play_Forward_Button,SIGNAL(toggled(bool)),vengine,SLOT(startPause(bool)));
        QObject::disconnect(currentTab->stop_Button,SIGNAL(clicked()),vengine,SLOT(abort()));
        QObject::disconnect(currentTab->play_Backward_Button,SIGNAL(toggled(bool)),vengine,SLOT(startBackward(bool)));
        QObject::disconnect(currentTab->previousFrame_Button,SIGNAL(clicked()),vengine,SLOT(previousFrame()));
        QObject::disconnect(currentTab->nextFrame_Button,SIGNAL(clicked()),vengine,SLOT(nextFrame()));
        QObject::disconnect(currentTab->previousIncrement_Button,SIGNAL(clicked()),vengine,SLOT(previousIncrement()));
        QObject::disconnect(currentTab->nextIncrement_Button,SIGNAL(clicked()),vengine,SLOT(nextIncrement()));
        QObject::disconnect(currentTab->firstFrame_Button,SIGNAL(clicked()),vengine,SLOT(firstFrame()));
        QObject::disconnect(currentTab->lastFrame_Button,SIGNAL(clicked()),vengine,SLOT(lastFrame()));
        QObject::disconnect(currentTab->frameNumberBox,SIGNAL(valueChanged(double)),vengine,SLOT(seekRandomFrame(double)));
        QObject::disconnect(currentTab->frameSeeker,SIGNAL(positionChanged(int)), vengine, SLOT(seekRandomFrameWithStart(int)));
    }
    QObject::connect(vengine, SIGNAL(fpsChanged(double)), tab->fpsBox, SLOT(setValue(double)));
    QObject::connect(tab->fpsBox, SIGNAL(valueChanged(double)),vengine, SLOT(setDesiredFPS(double)));
    QObject::connect(tab->play_Forward_Button,SIGNAL(toggled(bool)),vengine,SLOT(startPause(bool)));
    QObject::connect(tab->stop_Button,SIGNAL(clicked()),vengine,SLOT(abort()));
    QObject::connect(tab->play_Backward_Button,SIGNAL(toggled(bool)),vengine,SLOT(startBackward(bool)));
    QObject::connect(tab->previousFrame_Button,SIGNAL(clicked()),vengine,SLOT(previousFrame()));
    QObject::connect(tab->nextFrame_Button,SIGNAL(clicked()),vengine,SLOT(nextFrame()));
    QObject::connect(tab->previousIncrement_Button,SIGNAL(clicked()),vengine,SLOT(previousIncrement()));
    QObject::connect(tab->nextIncrement_Button,SIGNAL(clicked()),vengine,SLOT(nextIncrement()));
    QObject::connect(tab->firstFrame_Button,SIGNAL(clicked()),vengine,SLOT(firstFrame()));
    QObject::connect(tab->lastFrame_Button,SIGNAL(clicked()),vengine,SLOT(lastFrame()));
    QObject::connect(tab->frameNumberBox,SIGNAL(valueChanged(double)),vengine,SLOT(seekRandomFrame(double)));
    QObject::connect(tab->frameSeeker,SIGNAL(positionChanged(int)), vengine, SLOT(seekRandomFrame(int)));
    
    _currentViewerTab = tab;
    int index = 0;
    for (U32 i = 0; i < _viewerTabs.size(); i++) {
        if(_viewerTabs[i] == tab){
            index = i;
        }
    }
    viewersTabContainer->setCurrentIndex(index);
}