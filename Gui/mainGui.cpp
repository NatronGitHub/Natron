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
_nextViewerTabPlace(0)
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
	
    loadStyleSheet();
	
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
    
    _leftRightSplitter = new QSplitter(_centralWidget);
    _leftRightSplitter->setOrientation(Qt::Horizontal);
    _leftRightSplitter->setContentsMargins(0, 0, 0, 0);
    
    _toolsPane = new TabWidget(TabWidget::NONE,_leftRightSplitter);
    _panes.push_back(_toolsPane);
    _toolBox = new QToolBox(_toolsPane);
    _toolsPane->appendTab("", _toolBox);
    _toolsPane->setMaximumWidth(50);
    
  
    _leftRightSplitter->addWidget(_toolsPane);
    
	_viewerWorkshopSplitter = new QSplitter(_centralWidget);
    _viewerWorkshopSplitter->setContentsMargins(0, 0, 0, 0);
	_viewerWorkshopSplitter->setOrientation(Qt::Vertical);
    _viewerWorkshopSplitter->setChildrenCollapsible(false);
    QSize viewerWorkshopSplitterSize = _viewerWorkshopSplitter->sizeHint();
    QList<int> sizesViewerSplitter; sizesViewerSplitter <<  viewerWorkshopSplitterSize.height()/2;
    sizesViewerSplitter  << viewerWorkshopSplitterSize.height()/2;
    
    /*VIEWERS related*/
    _textureCache = new TextureCache(Settings::getPowiterCurrentSettings()->_cacheSettings.maxTextureCache);
	_viewersPane = new TabWidget(TabWidget::NOT_CLOSABLE,_viewerWorkshopSplitter);
    _panes.push_back(_viewersPane);
    _viewersPane->resize(_viewersPane->width(), screen.height()/5);
	_viewerWorkshopSplitter->addWidget(_viewersPane);
  //  _viewerWorkshopSplitter->setSizes(sizesViewerSplitter);

	
    
	/*WORKSHOP PANE*/
	//======================
	_workshopPane = new TabWidget(TabWidget::NOT_CLOSABLE,_viewerWorkshopSplitter);
    _panes.push_back(_workshopPane);
    /*creating DAG gui*/
	addNodeGraph();
	
	_viewerWorkshopSplitter->addWidget(_workshopPane);
	
    _middleRightSplitter = new QSplitter(_centralWidget);
    _middleRightSplitter->setContentsMargins(0, 0, 0, 0);
	_middleRightSplitter->setOrientation(Qt::Horizontal);
    _middleRightSplitter->addWidget(_viewerWorkshopSplitter);
    
	/*PROPERTIES DOCK*/
	//======================
	_propertiesPane = new TabWidget(TabWidget::NOT_CLOSABLE,this);
    _panes.push_back(_propertiesPane);
	_propertiesScrollArea = new QScrollArea(_propertiesPane);
    _propertiesScrollArea->setObjectName("Properties_GUI");
	_propertiesContainer=new QWidget(_propertiesScrollArea);
    _propertiesContainer->setObjectName("_propertiesContainer");
	_layoutPropertiesBin=new QVBoxLayout(_propertiesContainer);
	_layoutPropertiesBin->setSpacing(0);
    _layoutPropertiesBin->setContentsMargins(0, 0, 15, 0);
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

void Gui::loadStyleSheet(){
    QFile qss(ROOT"Resources/Stylesheets/mainstyle.qss");
    if(qss.open(QIODevice::ReadOnly
                    | QIODevice::Text))
    {
        QTextStream in(&qss);
        QString content(in.readAll());
        setStyleSheet(content);
    }
}

void Gui::setFullScreen(TabWidget* what){
    for (U32 i =0; i < _panes.size(); i++) {
        if (_panes[i] != what) {
            _panes[i]->hide();
        }
    }
}

void Gui::exitFullScreen(){
    for (U32 i =0; i < _panes.size(); i++) {
        _panes[i]->show();
    }
}

void Gui::clearTextureCache(){
    std::vector<U32> currentlyUsedTex;
    for (U32 i = 0; i < _viewerTabs.size(); i++) {
        currentlyUsedTex.push_back(_viewerTabs[i]->viewer->getCurrentTexture());
    }
    _textureCache->clearCache(currentlyUsedTex);
}

ViewerTab* Gui::addViewerTab(Viewer* node,TabWidget* where){
    ViewerTab* tab = new ViewerTab(node,_viewersPane);
    tab->setTextureCache(_textureCache);
    _viewerTabs.push_back(tab);
    where->appendTab(node->getName(),tab);
    return tab;
}

void Gui::removeViewerTab(ViewerTab* tab,bool initiatedFromNode){
    for (U32 i = 0; i < _viewerTabs.size(); i++) {
        if (_viewerTabs[i] == tab) {
            _viewerTabs.erase(_viewerTabs.begin()+i);
            break;
        }
    }
    
    if (!initiatedFromNode) {
        _nodeGraphTab->_nodeGraphArea->removeNode(tab->getInternalNode()->getNodeUi());
        ctrlPTR->getModel()->removeNode(tab->getInternalNode());
    }else{
        
        TabWidget* container = dynamic_cast<TabWidget*>(tab->parentWidget());
        container->removeTab(tab);
    }
    
}
void Gui::addNodeGraph(){
    NodeGraphTab* tab = new NodeGraphTab(_workshopPane);
    _nodeGraphTab = tab;
    _workshopPane->appendTab("Node graph",tab->_nodeGraphArea);
}

void Gui::moveTab(QWidget* what,TabWidget *where){
    TabWidget* from = dynamic_cast<TabWidget*>(what->parentWidget());
    
    if(!from) return;
    QString name;
    if(from == where){
        /*We check that even if it is the same TabWidget, it really exists.*/
        bool found = false;
        for (int i =0; i < from->count(); i++) {
            if (what == from->tabAt(i)) {
                found = true;
                break;
            }
        }
        if (found) {
            return;
        }else{
            /*if it is not found we have to recover the name*/
            if(what == _nodeGraphTab->_nodeGraphArea)
                name = "Node Graph";
            else if(what == _propertiesScrollArea)
                name = "Properties";
            else return;
        }
    }else{
        name = from->getTabName(what);

    }
    
    
    from->removeTab(what);
    where->appendTab(name, what);
}



NodeGraphTab::NodeGraphTab(QWidget* parent){
    _graphScene=new QGraphicsScene(parent);
	_graphScene->setItemIndexMethod(QGraphicsScene::NoIndex);
    _nodeGraphArea = new NodeGraph(_graphScene,parent);
    _nodeGraphArea->grabKeyboard();
    _nodeGraphArea->releaseKeyboard();
    _nodeGraphArea->setFocus();
    
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
    TabWidget* newTab = new TabWidget(TabWidget::CLOSABLE,newSplitter);
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
    TabWidget* newTab = new TabWidget(TabWidget::CLOSABLE,newSplitter);
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

void Gui::closePane(TabWidget* what){
    QSplitter* container = dynamic_cast<QSplitter*>(what->parentWidget());
    if(!container) return;
    
    /*Removing it from the _panes vector*/
    for (U32 i = 0; i < _panes.size(); i++) {
        if (_panes[i] == what) {
            _panes.erase(_panes.begin()+i);
            break;
        }
    }
    
    /*If it is floating we do not need to re-arrange the splitters containing the tab*/
    if (what->isFloating()) {
        what->destroyTabs();
        delete what;
        return;
    }

    
    /*Only sub-panes are closable. That means the splitter owning them must also
     have a splitter as parent*/
    QSplitter* mainContainer = dynamic_cast<QSplitter*>(container->parentWidget());
    if(!mainContainer) return;
    
    /*identifying the other tab*/
    TabWidget* other = 0;
    for (int i = 0; i < container->count(); i++) {
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
    for (int i = 0; i < mainContainer->count(); i++) {
        QSplitter* subSplitter = dynamic_cast<QSplitter*>(mainContainer->widget(i));
        if (subSplitter && subSplitter == container) {
            subSplitterIndex = i;
            container->setVisible(false);
            container->setParent(0);
            break;
        }
    }
    /*moving the other to the mainContainer*/
    other->setVisible(true);
    other->setParent(mainContainer);
    mainContainer->insertWidget(subSplitterIndex, other);
    
    /*deleting the subSplitter*/
    delete container;
    
}

FloatingWidget::FloatingWidget(QWidget* parent) : QWidget(parent), _embeddedWidget(0){
    
    setWindowFlags(Qt::WindowStaysOnTopHint | Qt::Window);
    _layout = new QVBoxLayout(this);
    _layout->setContentsMargins(0, 0, 0, 0);
    setLayout(_layout);
    
}
void FloatingWidget::setWidget(const QSize& widgetSize,QWidget* w){
    if (_embeddedWidget) {
        return;
    }
    w->setParent(this);
    _layout->addWidget(w);
    w->setVisible(true);
    resize(widgetSize);
    show();
}

void Gui::registerTab(std::string name,QWidget* tab){
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