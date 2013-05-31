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
using namespace std;

Gui::Gui(QWidget* parent):QMainWindow(parent)
{

}
Gui::~Gui(){
    

}
void Gui::exit(){
  //  delete crossPlatform;
   // crossPlatform->exit();
    crossPlatform->getModel()->getVideoEngine()->abort();
    qApp->exit(0);
    
}
void Gui::closeEvent(QCloseEvent *e){
    // save project ...
    exit();
    e->accept();
}


void Gui::addNode_ui(qreal x,qreal y,UI_NODE_TYPE type, Node* node){
    nodeGraphArea->addNode_ui(layout_settings, x,y,type,node);

}

void Gui::setControler(Controler* crossPlatform){
    this->crossPlatform=crossPlatform;
}
void Gui::createGui(){

   
    setupUi(crossPlatform);
    
    QObject::connect(QCoreApplication::instance(), SIGNAL(aboutToQuit()), this, SLOT(exit()));
    nodeGraphArea->setControler(crossPlatform);
    
    nodeGraphArea->installEventFilter(this);
    viewer_tab->installEventFilter(this);
    rightDock->installEventFilter(this);
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
            crossPlatform->getModel()->getVideoEngine()->nextFrame();
            focusNextChild();
            return true;
        }else if(keyEvent->key() == Qt::Key_Left){
            crossPlatform->getModel()->getVideoEngine()->previousFrame();
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
	actionClearBufferCache ->setText(QApplication::translate("Powiter","Clear buffer cache"));

#ifdef __POWITER_WIN32__
	viewersTabContainer->setTabText(viewersTabContainer->indexOf(viewer_tab), QApplication::translate("Powiter", "Viewer 1"));
#endif
	WorkShop->setTabText(WorkShop->indexOf(CurveEditor), QApplication::translate("Powiter", "Motion Editor"));

	WorkShop->setTabText(WorkShop->indexOf(GraphEditor), QApplication::translate("Powiter", "Graph Editor"));
	menuFile->setTitle(QApplication::translate("Powiter", "File"));
	menuEdit->setTitle(QApplication::translate("Powiter", "Edit"));
	menuDisplay->setTitle(QApplication::translate("Powiter", "Display"));
	menuOptions->setTitle(QApplication::translate("Powiter", "Options"));
	viewersMenu->setTitle(QApplication::translate("Powiter", "Viewer(s)"));
	cacheMenu->setTitle(QApplication::translate("Powiter", "Cache"));

} // retranslateUi
void Gui::setupUi(Controler* ctrl)
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
	actionClearDiskCache->setShortcut(QKeySequence(Qt::CTRL+Qt::ALT+Qt::Key_K));
	actionClearPlayBackCache = new QAction(this);
	actionClearPlayBackCache->setObjectName(QString::fromUtf8("actionClearPlayBackCache"));
	actionClearPlayBackCache->setCheckable(false);
	actionClearPlayBackCache->setShortcut(QKeySequence(Qt::CTRL+Qt::ALT+Qt::Key_P));
	actionClearBufferCache = new QAction(this);
	actionClearBufferCache->setObjectName(QString::fromUtf8("actionClearBufferCache"));
	actionClearBufferCache->setCheckable(false);
	actionClearBufferCache->setShortcut(QKeySequence(Qt::CTRL+Qt::ALT+Qt::Key_B));

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
#ifdef __POWITER_WIN32__
	viewersTabContainer = new QTabWidget(splitter);
	viewersTabContainer->setObjectName(QString::fromUtf8("viewersTabContainer"));
	/*Viewer(s)*/
	QVBoxLayout* viewer_tabLayout=viewer_tabLayout=new QVBoxLayout(viewersTabContainer);
	viewer_tabLayout->setObjectName(QString::fromUtf8("viewer_tabLayout"));;
	viewer_tab = new ViewerTab(ctrl,viewer_tabLayout,viewersTabContainer);
	viewer_tab->setLayout(viewer_tabLayout);
	viewer_tab->setObjectName(QString::fromUtf8("Viewer1"));
	viewersTabContainer->addTab(viewer_tab, QString());
	splitter->addWidget(viewersTabContainer);
#else
	/*Viewer(s)*/  //< CONTAINER NOT WORKING WITH QT/COCOA, FIND A WORKAROUND ON OSX
	QVBoxLayout* viewer_tabLayout=viewer_tabLayout=new QVBoxLayout(splitter);
	viewer_tabLayout->setObjectName(QString::fromUtf8("viewer_tabLayout"));;
	viewer_tab = new ViewerTab(ctrl,viewer_tabLayout,splitter);
	viewer_tab->setLayout(viewer_tabLayout);
	viewer_tab->setObjectName(QString::fromUtf8("Viewer1"));
	splitter->addWidget(viewer_tab);
    
    /*initializing texture cache*/
    _textureCache = new TextureCache(crossPlatform->getModel()->getCurrentPowiterSettings()->maxTextureCache);
    viewer_tab->setTextureCache(_textureCache);
#endif
	// splitter->setCollapsible(0,false);

	/*GRAPH EDITOR*/
	//======================
	WorkShop = new QTabWidget(splitter);
	//WorkShop->setStyleSheet("background-color: rgba(50,50,50,255);");

	WorkShop->setObjectName(QString::fromUtf8("WorkShop"));

	GraphEditor = new QWidget();
	GraphEditor->setObjectName(QString::fromUtf8("GraphEditor"));
	verticalLayout_3 = new QVBoxLayout(GraphEditor);
	verticalLayout_3->setObjectName(QString::fromUtf8("verticalLayout_3"));
	graph_scene=new QGraphicsScene(GraphEditor);
	graph_scene->setItemIndexMethod(QGraphicsScene::NoIndex);
	QWidget* viewport=new QWidget();
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
	CurveEditor = new QWidget();
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

	PropertiesDockContent = new QScrollArea();
	//PropertiesDockContent->setStyleSheet("background-color:rgba(50,50,50,255);");

	propertiesContainer=new QWidget();
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
	cacheMenu->addAction(actionClearBufferCache);
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
	ToolDockContent = new QWidget();
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
	QMetaObject::connectSlotsByName(this);
} // setupUi