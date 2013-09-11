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
CLANG_DIAG_OFF(unused-private-field);
#include <QtGui/qmime.h>
CLANG_DIAG_ON(unused-private-field);
#include <QtGui/QCloseEvent>
#include <QApplication>
#include <QMenu>
#include <QLayout>
#include <QDesktopWidget>
#include <QSplitter>
#include <QMenuBar>
#include <QToolBar>
#include <QFileDialog>
#include <QMessageBox>
 

#include "Global/AppManager.h"

#include "Engine/Model.h"
#include "Engine/VideoEngine.h"
#include "Engine/Settings.h"
#include "Engine/ViewerNode.h"
#include "Engine/OfxNode.h"

#include "Gui/Texture.h"
#include "Gui/ViewerGL.h"
#include "Gui/TabWidget.h"
#include "Gui/FeedbackSpinBox.h"
#include "Gui/Timeline.h"
#include "Gui/NodeGraph.h"
#include "Gui/ViewerTab.h"
#include "Gui/SequenceFileDialog.h"
#include "Gui/NodeGui.h"

#define PLUGIN_GROUP_DEFAULT "Other"
#define PLUGIN_GROUP_DEFAULT_ICON_PATH POWITER_IMAGES_PATH"openeffects.png"
 
using namespace std;
using namespace Powiter;

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
actionNew_project(0),
actionOpen_project(0),
actionSave_project(0),
actionSaveAs_project(0),
actionPreferences(0),
actionExit(0),
actionProject_settings(0),
actionFullScreen(0),
actionSplitViewersTab(0),
actionClearDiskCache(0),
actionClearPlayBackCache(0),
actionClearNodeCache(0),
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
cacheMenu(0)
{
}

Gui::~Gui(){
    
    
}

bool Gui::exit(){
    int ret = saveWarning();
    if(ret == 0){
        saveProject();
    }else if(ret == 2){
        return false;
    }
    delete _appInstance;
    delete this;

    if(_appInstance->getAppID() != 0){
        return false;
    }else{
        qApp->exit(0);
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

void Gui::closeEvent(QCloseEvent *e){
    if(!exit()){
        e->ignore();
        return;
    }
    e->accept();
}


NodeGui* Gui::createNodeGUI( Node* node){
    if(int ret = node->canMakePreviewImage()){
        if(ret == 2){
            OfxNode* n = dynamic_cast<OfxNode*>(node);
            n->computePreviewImage();
        }
    }
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
    dynamic_cast<ViewerNode*>(viewer)->initializeViewerTab(where);
    _lastSelectedViewer = dynamic_cast<ViewerNode*>(viewer)->getUiContext();
}
void Gui::autoConnect(NodeGui* target,NodeGui* created){
    if(target)
        _nodeGraphTab->_nodeGraphArea->autoConnect(target, created);
    _nodeGraphTab->_nodeGraphArea->selectNode(created);
}
NodeGui* Gui::getSelectedNode() const{
    return _nodeGraphTab->_nodeGraphArea->getSelectedNode();
}


void Gui::createGui(){
    
    
    setupUi();
    
    QObject::connect(QCoreApplication::instance(), SIGNAL(aboutToQuit()), this, SLOT(exit()));
    
}



bool Gui::eventFilter(QObject *target, QEvent *event){
    if(dynamic_cast<QInputEvent*>(event)){
        /*Make top level instance this instance since it receives all
         user inputs.*/
        appPTR->setAsTopLevelInstance(_appInstance->getAppID());
    }
     
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Right) {
            _appInstance->getModel()->getVideoEngine()->nextFrame();
            focusNextChild();
            return true;
        }else if(keyEvent->key() == Qt::Key_Left){
            _appInstance->getModel()->getVideoEngine()->previousFrame();
            focusNextChild();
            return true;
        }
    }
    return QMainWindow::eventFilter(target, event);
}
void Gui::retranslateUi(QMainWindow *MainWindow)
{
    Q_UNUSED(MainWindow);
	setWindowTitle(QApplication::translate("Powiter", "Powiter"));
	actionNew_project->setText(QApplication::translate("Powiter", "New Project"));
	actionOpen_project->setText(QApplication::translate("Powiter", "Open Project..."));
	actionSave_project->setText(QApplication::translate("Powiter", "Save Project"));
    actionSaveAs_project->setText(QApplication::translate("Powiter", "Save Project As..."));
	actionPreferences->setText(QApplication::translate("Powiter", "Preferences..."));
    actionExit->setText(QApplication::translate("Powiter", "Exit"));
	actionProject_settings->setText(QApplication::translate("Powiter", "Project Settings..."));
	actionFullScreen->setText(QApplication::translate("Powiter","Toggle Full Screen"));
	actionSplitViewersTab->setText(QApplication::translate("Powiter","Toggle Multi-View Area"));
	actionClearDiskCache->setText(QApplication::translate("Powiter","Clear Disk Cache"));
	actionClearPlayBackCache->setText(QApplication::translate("Powiter","Clear Playback Cache"));
	actionClearNodeCache ->setText(QApplication::translate("Powiter","Clear Per-Node Cache"));
    
    
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
    
    setMouseTracking(true);
    installEventFilter(this);
    QDesktopWidget* desktop=QApplication::desktop();
    QRect screen=desktop->screenGeometry();
    assert(!isFullScreen());
    resize(0.93*screen.width(),0.93*screen.height()); // leave some space
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
	actionFullScreen = new QAction(this);
	actionFullScreen->setObjectName(QString::fromUtf8("actionFullScreen"));
	actionFullScreen->setShortcut(QKeySequence(Qt::CTRL+Qt::META+Qt::Key_F));
    actionFullScreen->setIcon(get_icon("view-fullscreen"));
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
    /*creating DAG gui*/
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
    QList<int> sizes; sizes <<   240;
    sizes  <<horizontalSplitterSize.width()- 240;
    _middleRightSplitter->setSizes(sizes);
	
    
    _leftRightSplitter->addWidget(_middleRightSplitter);
    
    
    
    
    _mainLayout->addWidget(_leftRightSplitter);
    
    
    
    
    
	
    
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
    menuDisplay->addSeparator();
	menuDisplay->addAction(actionFullScreen);
	viewersMenu->addAction(actionSplitViewersTab);
	cacheMenu->addAction(actionClearDiskCache);
	cacheMenu->addAction(actionClearPlayBackCache);
	cacheMenu->addAction(actionClearNodeCache);
	retranslateUi(this);
    
    
    
    Model* model = _appInstance->getModel();
    QObject::connect(actionFullScreen, SIGNAL(triggered()),this,SLOT(toggleFullScreen()));
    QObject::connect(actionClearDiskCache, SIGNAL(triggered()),model,SLOT(clearDiskCache()));
    QObject::connect(actionClearPlayBackCache, SIGNAL(triggered()),model,SLOT(clearPlaybackCache()));
    QObject::connect(actionClearNodeCache, SIGNAL(triggered()),model,SLOT(clearNodeCache()));
    QObject::connect(actionExit,SIGNAL(triggered()),this,SLOT(exit()));
    
	QMetaObject::connectSlotsByName(this);
    
} // setupUi

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

void Gui::maximize(TabWidget* what){
    for (U32 i =0; i < _panes.size(); ++i) {
        if (_panes[i] != what) {
            _panes[i]->hide();
        }
    }
}

void Gui::minimize(){
    for (U32 i =0; i < _panes.size(); ++i) {
        _panes[i]->show();
    }
}


ViewerTab* Gui::addNewViewerTab(ViewerNode* node,TabWidget* where){
    ViewerTab* tab = new ViewerTab(this,node,_viewersPane);
    _viewerTabs.push_back(tab);
    where->appendTab(node->getName().c_str(),tab);
    return tab;
}
void Gui::addViewerTab(ViewerTab* tab,TabWidget* where){
    bool found = false;
    for (U32 i = 0; i < _viewerTabs.size(); ++i) {
        if (_viewerTabs[i] == tab) {
            found = true;
            break;
        }
    }
    if(!found)
        _viewerTabs.push_back(tab);
    where->appendTab(tab->getInternalNode()->getName().c_str(), tab);
    
}

void Gui::removeViewerTab(ViewerTab* tab,bool initiatedFromNode,bool deleteData){
    assert(tab);
    for (U32 i = 0; i < _viewerTabs.size(); ++i) {
        if (_viewerTabs[i] == tab) {
            _viewerTabs.erase(_viewerTabs.begin()+i);
            break;
        }
    }
    
    if(deleteData){
        if (!initiatedFromNode) {
            assert(_nodeGraphTab);
            assert(_nodeGraphTab->_nodeGraphArea);
            tab->getInternalNode()->deactivate();
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
void Gui::addNodeGraph(){
    NodeGraphTab* tab = new NodeGraphTab(this,_workshopPane);
    _nodeGraphTab = tab;
    assert(_workshopPane);
    _workshopPane->appendTab("Node graph",tab->_nodeGraphArea);
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



NodeGraphTab::NodeGraphTab(Gui* gui,QWidget* parent){
    _graphScene=new QGraphicsScene(parent);
	_graphScene->setItemIndexMethod(QGraphicsScene::NoIndex);
    _nodeGraphArea = new NodeGraph(gui,_graphScene,parent);
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

void Gui::closePane(TabWidget* what){
    QSplitter* container = dynamic_cast<QSplitter*>(what->parentWidget());
    if(!container) return;
    
    /*Removing it from the _panes vector*/
    for (U32 i = 0; i < _panes.size(); ++i) {
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
:QToolButton(parent),
_app(app){
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
    QMenu* _lastMenu = _menu;
    while(index < (int)grouping.size()){
        bool found = false;
        for (U32 i =  0; i < _subMenus.size(); ++i) {
            if (_subMenus[i]->title() == QString(grouping[index])) {
                _lastMenu = _subMenus[i];
                found = true;
                break;
            }
        }
        if(!found)
            break;
        ++index;
    }
    for(int i = index; i < (int)grouping.size() ; ++i) {
        QMenu* menu = _lastMenu->addMenu(grouping[index]);
        _subMenus.push_back(menu);
        _lastMenu = menu;
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
void ActionRef::onTriggered(){
    _app->createNode(_nodeName);
}
void Gui::addUndoRedoActions(QAction* undoAction,QAction* redoAction){
    menuEdit->addAction(undoAction);
	menuEdit->addAction(redoAction);
}
void Gui::newProject(){
//    int ret = saveWarning();
//    if(ret == 0){
//        saveProject();
//    }else if(ret == 2){
//        return;
//    }
    appPTR->newAppInstance();
}
void Gui::openProject(){
    std::vector<std::string> filters;
    filters.push_back("rs");
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
    vector<string> filter;
    filter.push_back("rs");
    SequenceFileDialog dialog(this,filter,false,SequenceFileDialog::SAVE_DIALOG);
    QString outFile;
    if(dialog.exec()){
        outFile = dialog.filesToSave();
    }
    if (outFile.size() > 0) {
        if (outFile.indexOf(".rs") == -1) {
            outFile.append(".rs");
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

void Gui::errorDialog(const QString& title,const QString& text){
    QMessageBox::critical(this, title, text);
}

void Gui::selectNode(NodeGui* node){
    _nodeGraphTab->_nodeGraphArea->selectNode(node);
}
