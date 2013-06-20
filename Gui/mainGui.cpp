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
#include "Core/viewerNode.h"
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
_centralWidget(0),
_mainLayout(0),
_toolsPane(0),
_viewersPane(0),
_workshopPane(0),
_viewerWorkshopSplitter(0),
_propertiesPane(0),
_middleRightSplitter(0),
_nodeGraphTab(0),
_propertiesScrollArea(0),
_propertiesContainer(0),
_layoutPropertiesBin(0),
_toolBox(0),
menubar(0),
menuFile(0),
menuEdit(0),
menuDisplay(0),
menuOptions(0),
viewersMenu(0),
cacheMenu(0),
_leftRightSplitter(0),
_nextViewerTabPlace(0),
_propertiesBinLocation(0),
_nodeGraphLocation(0)
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
    _nodeGraphTab->_nodeGraphArea->createNodeGUI(_layoutPropertiesBin,type,node,x,y);
}


void Gui::createGui(){
    
    
    setupUi();
    
    QObject::connect(QCoreApplication::instance(), SIGNAL(aboutToQuit()), this, SLOT(exit()));
    //nodeGraphArea->installEventFilter(this);
    //  viewer_tab->installEventFilter(this);
    //rightDock->installEventFilter(this);
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
    

	//WorkShop->setTabText(WorkShop->indexOf(CurveEditor), QApplication::translate("Powiter", "Motion Editor"));
    
	//WorkShop->setTabText(WorkShop->indexOf(GraphEditor), QApplication::translate("Powiter", "Graph Editor"));
	menuFile->setTitle(QApplication::translate("Powiter", "File"));
	menuEdit->setTitle(QApplication::translate("Powiter", "Edit"));
	menuDisplay->setTitle(QApplication::translate("Powiter", "Display"));
	menuOptions->setTitle(QApplication::translate("Powiter", "Options"));
	viewersMenu->setTitle(QApplication::translate("Powiter", "Viewer(s)"));
	cacheMenu->setTitle(QApplication::translate("Powiter", "Cache"));
    
} 
void Gui::setupUi()
{
	
	QDesktopWidget* desktop=QApplication::desktop();
	QRect screen=desktop->screenGeometry(-1);
	resize(screen.width(),screen.height());
	setDockNestingEnabled(false);
	setStyleSheet("QWidget{background-color:rgba(50,50,50,255); }"
                  "QPushButton{background-color:rgba(71,71,71,255);color:rgba(200,200,200,255);}"
                  "QMainWindow{background-color:rgba(50,50,50,255);}"
                  "QCheckBox::indicator:unchecked {"
                  "image: url(Images/checkbox.png);"
                  "}"
                  "QCheckBox::indicator:checked {"
                  "image: url(Images/checkbox_checked.png);"
                  "}"
                  "QCheckBox::indicator:checked:hover {"
                  "image: url(Images/checkbox_checked_hovered.png);"
                  "}"
                  "QCheckBox::indicator:unchecked:hover {"
                  "image: url(Images/checkbox_hovered.png);"
                  "}"
                  "QGraphicsView{background-color:rgba(71,71,71,255);}"
                  "QScrollArea{background-color:rgba(50,50,50,255);}"
                  "QGroupBox{color:rgba(200,200,200,255);background-color:rgba(50,50,50,255);border:0px;}"
                  "QComboBox{color:rgba(200,200,200,255);background-color:rgba(71,71,71,255);"
                  "selection-color:rgba(243,149,0,255);selection-background-color:rgba(71,71,71,255);}"
                  "QComboBox QAbstractItemView{"
                  "border-radius:0px;"
                  "border:0px;selection-background-color:rgba(71,71,71,255);"
                  "background:rgba(71,71,71,255);color:rgb(200,200,200);"
                  "}"
                  "QDockWidget::title{background:rgb(71,71,71);}QDockWidget{color:rgb(200,200,200);}"
                  "QTabWidget::pane{border:0px;}"
                  "QLabel{color: rgba(200,200,200,255);}"
                  "QLineEdit{border:1px solid; border-radius:1; padding:1px; background-color : rgba(71,71,71,255);}"
                  "QLineEdit{color:rgba(200,200,200,255);}"
                  "QLineEdit:focus {"
                      "selection-color: rgb(243, 149, 0);"
                  "border: 2px groove rgb(243, 149, 0);"
                   "   border-radius: 4px;"
                  "padding: 2px 4px;"
                  "}"
                  "QSplitter::handle:horizontal{background-color:"
                  "qlineargradient(spread:pad, x1:0, y1:0, x2:1, y2:0, stop:0 rgba(71, 71, 71, 255), stop:0.55 rgba(50, 50, 50, 255), "
                  "stop:0.98 rgba(0, 0, 0, 255), stop:1 rgba(0, 0, 0, 0));"
                  "border: 0px;}"
                  "QSplitter::handle:vertical{background-color:"
                  "qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1, stop:0 rgba(71, 71, 71, 255), stop:0.55 rgba(50, 50, 50, 255), "
                  "stop:0.98 rgba(0, 0, 0, 255), stop:1 rgba(0, 0, 0, 0));"
                  "border: 0px;}"
                  "QSplitter::handle::horizontal{"
                  "width: 3px;}"
                  "QSplitter::handle::vertical{"
                  "height: 3px;}"
                  "QTabBar::tab {"
                  "color:rgba(200,200,200,255);"
                  "background: rgb(39,39,39);"
                  "border: 1px solid ;"
                  "    border-bottom-color: rgb(50,50,50);"
                  "    min-width: 8ex;"
                  "padding: 0px;"
                  "    border-top-right-radius: 5px;"
                  "    border-top-left-radius: 5px;"
                  "}"
                  "QTabBar::tab:hover {"
                  "background: rgb(243,149,0);"
                  "}"
                  "QTabBar::tab:selected {"
                  "background: rgb(50,50,50);"
                  "    border-bottom-style:none;"
                  "}"
                  "QTabBar::tab:!selected {"
                  "    margin-top: 3px; /* make non-selected tabs look smaller */"
                  "}"
                  "/* make use of negative margins for overlapping tabs */"
                  "QTabBar::tab:selected {"
                  "    /* expand/overlap to the left and right by 4px */"
                  "    margin-left: -3px;"
                  "    margin-right: -3px;"
                  "}"
                  "QTabBar::tab:first:selected {"
                  "    margin-left: 0; /* the first selected tab has nothing to overlap with on the left */"
                  "}"
                  "QTabBar::tab:last:selected {"
                  "    margin-right: 0; /* the last selected tab has nothing to overlap with on the right */"
                  "}"
                  "QTabBar::tab:only-one {"
                  "margin: 0; /* if there is only one tab, we don't want overlapping margins */"
                  "}"
                  "QTabWidget::tab-bar {"
                  "alignment: left;"
                  "}"
                  "QMenu {"
                      "background-color: rgb(50,50,50); /* sets background of the menu */"
                  "border: 0px;"
                  "margin: 0px;"
                  "color : rgb(200,200,200);"
                  "}"
                  "QMenu::item:selected { /* when user selects item using mouse or keyboard */"
                      "background-color: rgb(243,149,0); ;"
                  "}");
	
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
	_centralWidget = new QWidget(this);
    setCentralWidget(_centralWidget);
	_mainLayout = new QHBoxLayout(_centralWidget);
    _mainLayout->setContentsMargins(0, 0, 0, 0);
	_centralWidget->setLayout(_mainLayout);
    
    _toolsPane = new TabWidget(false,_centralWidget);
    _toolBox = new QToolBox(_toolsPane);
    _toolsPane->appendTab("", _toolBox);
    _toolsPane->setMaximumWidth(50);
    
    _leftRightSplitter = new QSplitter(_centralWidget);
    _leftRightSplitter->setOrientation(Qt::Horizontal);
    _leftRightSplitter->setContentsMargins(0, 0, 0, 0);
    _leftRightSplitter->addWidget(_toolsPane);
    
	_viewerWorkshopSplitter = new QSplitter(_centralWidget);
    _viewerWorkshopSplitter->setContentsMargins(0, 0, 0, 0);
	_viewerWorkshopSplitter->setOrientation(Qt::Vertical);
    QSize viewerWorkshopSplitterSize = _viewerWorkshopSplitter->sizeHint();
    QList<int> sizesViewerSplitter; sizesViewerSplitter <<  viewerWorkshopSplitterSize.height()/2;
    sizesViewerSplitter  << viewerWorkshopSplitterSize.height()/2;
    _viewerWorkshopSplitter->setSizes(sizesViewerSplitter);
    
    /*VIEWERS related*/
    _textureCache = new TextureCache(Settings::getPowiterCurrentSettings()->_cacheSettings.maxTextureCache);
	_viewersPane = new TabWidget(true,_viewerWorkshopSplitter);
    _viewersPane->resize(_viewersPane->width(), screen.height()/4);
	_viewerWorkshopSplitter->addWidget(_viewersPane);
    
	/*initializing texture cache*/
	
	// splitter->setCollapsible(0,false);
    
	/*WORKSHOP PANE*/
	//======================
	_workshopPane = new TabWidget(true,_viewerWorkshopSplitter);
    /*creating DAG gui*/
	addNodeGraph();
    _nodeGraphLocation = _workshopPane;
	
	_viewerWorkshopSplitter->addWidget(_workshopPane);
	
    _middleRightSplitter = new QSplitter(_centralWidget);
    _middleRightSplitter->setContentsMargins(0, 0, 0, 0);
	_middleRightSplitter->setOrientation(Qt::Horizontal);
    _middleRightSplitter->addWidget(_viewerWorkshopSplitter);
    
	/*PROPERTIES DOCK*/
	//======================
	_propertiesPane = new TabWidget(true,this);
    _propertiesBinLocation = _propertiesPane;
	_propertiesScrollArea = new QScrollArea(_propertiesPane);
	_propertiesContainer=new QWidget(_propertiesScrollArea);
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
    QList<int> sizes; sizes <<   240;
    sizes  <<horizontalSplitterSize.width()- 240;
    _middleRightSplitter->setSizes(sizes);
	
    _leftRightSplitter->addWidget(_middleRightSplitter);
    
    _mainLayout->addWidget(_leftRightSplitter);
    
	/*TOOL BAR menus*/
	//======================
	menubar = new QMenuBar(this);
	menubar->setGeometry(QRect(0, 0, 1159, 21));
	menuFile = new QMenu(menubar);
	menuEdit = new QMenu(menubar);
	menuDisplay = new QMenu(menubar);
	menuOptions = new QMenu(menubar);
	viewersMenu= new QMenu(menuDisplay);
	cacheMenu= new QMenu(menubar);
    
	setMenuBar(menubar);
    
	
    
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
        
    QObject::connect(actionClearTextureCache, SIGNAL(triggered()),this,SLOT(clearTexCache()));
    Model* model = ctrlPTR->getModel();
    QObject::connect(actionClearDiskCache, SIGNAL(triggered()),model,SLOT(clearDiskCache()));
    QObject::connect(actionClearPlayBackCache, SIGNAL(triggered()),model,SLOT(clearPlaybackCache()));
    QObject::connect(actionClearNodeCache, SIGNAL(triggered()),model,SLOT(clearNodeCache()));
    
	QMetaObject::connectSlotsByName(this);
    
} // setupUi

void Gui::clearTextureCache(){
    std::vector<U32> currentlyUsedTex;
    for (U32 i = 0; i < _viewerTabs.size(); i++) {
        currentlyUsedTex.push_back(_viewerTabs[i]->viewer->getCurrentTexture());
    }
    _textureCache->clearCache(currentlyUsedTex);
}

ViewerTab* Gui::addViewerTab(Viewer* node,TabWidget* where){
    ViewerTab* tab = new ViewerTab(node,_viewersPane);
    _viewerTabs.push_back(tab);
    tab->setTextureCache(_textureCache);
    where->appendTab(node->getName(),tab);
    return tab;
}

void Gui::removeViewerTab(ViewerTab* tab){
    ViewerTab* otherViewer = 0;
    for(U32 i = 0 ; i < _viewerTabs.size() ; i++){
        if (_viewerTabs[i] != tab) {
            otherViewer = _viewerTabs[i];
            break;
        }
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
        _viewersPane->removeTab(index);
        delete tab;
    }
    
}
void Gui::addNodeGraph(){
    NodeGraphTab* tab = new NodeGraphTab(_workshopPane);
    _nodeGraphTab = tab;
    _workshopPane->appendTab("Node graph",tab->_nodeGraphArea);
}

void Gui::moveNodeGraph(TabWidget* where){
    _nodeGraphLocation->removeTab(_nodeGraphTab->_nodeGraphArea);
    _nodeGraphLocation = where;
    _nodeGraphLocation->appendTab("Node graph",_nodeGraphTab->_nodeGraphArea);
}

void Gui::movePropertiesBin(TabWidget* where){
    _propertiesBinLocation->removeTab(_propertiesScrollArea);
    _propertiesBinLocation = where;
    _propertiesBinLocation->appendTab("Properties",_propertiesScrollArea);
}

NodeGraphTab::NodeGraphTab(QWidget* parent){
    _graphScene=new QGraphicsScene(parent);
	_graphScene->setItemIndexMethod(QGraphicsScene::NoIndex);
    _nodeGraphArea = new NodeGraph(_graphScene,parent);
    _nodeGraphArea->grabKeyboard();
    _nodeGraphArea->releaseKeyboard();
    _nodeGraphArea->setFocus();

}


