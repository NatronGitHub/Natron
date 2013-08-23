//  Powiter
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012. 
*contact: immarespond at gmail dot com
*
*/

 

 



#ifndef GUI_H
#define GUI_H
#include <QtCore/QVariant>
#include <QToolButton>
#include <QIcon>
#include <QAction>
#include "Global/GlobalDefines.h"
#include <QMainWindow>

#ifndef Q_MOC_RUN
#include <boost/noncopyable.hpp>
#endif
#include <QDialog>
#include <map>
class QString;
class QAction;
class TabWidget;
class Controler;
class QDockWidget;
class QScrollArea;
class QWidget;
class QVBoxLayout;
class QSplitter;
class QHBoxLayout;
class QFrame;
class QMenu;
class QMenuBar;
class QProgressBar;
class QStatusBar;
class QTreeView;
class Controler;
class NodeGraph;
class ViewerTab;
class Node;
class ViewerNode;
class QToolBar;
class QGraphicsScene;


/*Holds just a reference to an action*/
class ActionRef : public QObject{
    Q_OBJECT
public:
    
    ActionRef(QAction* action,const std::string& nodeName):_action(action),_nodeName(nodeName){
        QObject::connect(action, SIGNAL(triggered()), this, SLOT(onTriggered()));
    }
    virtual ~ActionRef(){QObject::disconnect(_action, SIGNAL(triggered()), this, SLOT(onTriggered()));}
    
    QAction* _action;
    std::string _nodeName;
    
    public slots:
    void onTriggered();

};

class ToolButton : public QToolButton{
    QMenu* _menu;
    std::vector<QMenu*> _subMenus;
    std::vector<ActionRef*> _actions;
public:
    
    ToolButton(const std::string& actionName,
               const std::vector<std::string>& firstElement,
               const std::string& pluginName,
               QIcon pluginIcon = QIcon(),
               QIcon groupIcon = QIcon(),
               QWidget* parent = 0);
    
    void addTool(const std::string& actionName,
                 const std::vector<std::string>& grouping,
                 const std::string& pluginName,
                 QIcon pluginIcon = QIcon());
    
    virtual ~ToolButton();

    
};

/*This class represents a floating pane that embeds a widget*/
class FloatingWidget : public QWidget{
    QWidget* _embeddedWidget;
    QVBoxLayout* _layout;
public:
    FloatingWidget(QWidget* parent = 0);
    virtual ~FloatingWidget(){}
    
    /*Set the embedded widget. Only 1 widget can be embedded
     by FloatingWidget. Once set, this function does nothing
     for subsequent calls..*/
    void setWidget(const QSize& widgetSize,QWidget* w);
};

/*This class encapsulate a nodegraph GUI*/
class NodeGraphTab{
public:

    QVBoxLayout *_graphEditorLayout;
    QGraphicsScene* _graphScene;
    NodeGraph *_nodeGraphArea;
    
    NodeGraphTab(QWidget* parent);
    virtual ~NodeGraphTab(){}
};

class Gui : public QMainWindow,public boost::noncopyable
{
    Q_OBJECT
public:
    Gui(QWidget* parent=0);
    
    virtual ~Gui();
    
    void createGui();
    
    void createNodeGUI(Node *node);
    
    bool eventFilter(QObject *target, QEvent *event);

    
    /*Called internally by the viewer node. It adds
     a new Viewer tab GUI and returns a pointer to it.*/
    ViewerTab* addNewViewerTab(ViewerNode* node,TabWidget* where);
    
    void addViewerTab(ViewerTab* tab,TabWidget* where);
    
    /*Called internally by the viewer node when
     it gets deleted. This removes the 
     associated GUI. It may also be called from the TabWidget
     that wants to close. The deleteData flag tells whether we actually want
     to destroy the tab/node or just hide them.*/
    void removeViewerTab(ViewerTab* tab,bool initiatedFromNode,bool deleteData = true);
    
    void setNewViewerAnchor(TabWidget* where){_nextViewerTabPlace = where;}
    
    void moveTab(QWidget* what,TabWidget* where);
    
    void splitPaneHorizontally(TabWidget* what);
    
    void splitPaneVertically(TabWidget* what);
    
    void floatWidget(QWidget* what);
    
    void closePane(TabWidget* what);
    
    void maximize(TabWidget* what);
    
    void minimize();
    
    /*Returns a valid tab if a tab with a matching name has been
     found. Otherwise returns NULL.*/
    QWidget* findExistingTab(const std::string& name) const;
    
    void registerTab(const std::string& name, QWidget* tab);
    
    void loadStyleSheet();
    
    void addPluginToolButton(const std::string& actionName,
                             const std::vector<std::string>& groups,
                             const std::string& pluginName,
                             const std::string& pluginIconPath,
                             const std::string& groupIconPath);
    void addUndoRedoActions(QAction* undoAction,QAction* redoAction);

    
    bool isGraphWorthless() const;
    
    void errorDialog(const QString& title,const QString& text);
    
private:

    void addNodeGraph();


public slots:
    void exit();
    void toggleFullScreen();
    void closeEvent(QCloseEvent *e);
    void newProject();
    void openProject();
    void saveProject();
    void saveProjectAs();
    void autoSave();
    void saveWarning();
    
public:
    /*TOOL BAR ACTIONS*/
    //======================
    QAction *actionNew_project;
    QAction *actionOpen_project;
    QAction *actionSave_project;
    QAction *actionSaveAs_project;
    QAction *actionPreferences;
    QAction *actionExit;
    QAction *actionProject_settings;
    QAction *actionFullScreen;
	QAction *actionSplitViewersTab;
    QAction *actionClearDiskCache;
    QAction *actionClearPlayBackCache;
    QAction *actionClearNodeCache;
    
    
    QWidget *_centralWidget;
    QHBoxLayout* _mainLayout;
    
    
    TabWidget* _viewersPane;
    
    // this one is a ptr to others TabWidget.
    //It tells where to put the viewer when making a new one
    // If null it places it on default tab widget
    TabWidget* _nextViewerTabPlace;
    
    TabWidget* _workshopPane;
    QSplitter* _viewerWorkshopSplitter;
    
    TabWidget* _propertiesPane;
    QSplitter* _middleRightSplitter;

    QSplitter* _leftRightSplitter;
    
	/*VIEWERS*/
	//======================
    std::vector<ViewerTab*> _viewerTabs;
    
    /*GRAPH*/
    //======================
    
    NodeGraphTab* _nodeGraphTab;
    
    /*TOOLBOX*/
    QToolBar* _toolBox;
    std::map<std::string,ToolButton*> _toolGroups;
    
    /*PROPERTIES*/
    //======================
    QScrollArea *_propertiesScrollArea;
    QWidget *_propertiesContainer;
    QVBoxLayout *_layoutPropertiesBin;
    
    
 
    /*MENU*/
    //======================
    QMenuBar *menubar;
    QMenu *menuFile;
    QMenu *menuEdit;
    QMenu *menuDisplay;
    QMenu *menuOptions;
	QMenu *viewersMenu;
    QMenu *cacheMenu;
    
    
    /*all TabWidget's : used to know what to hide/show for fullscreen mode*/
    std::vector<TabWidget*> _panes;
    
    /*Registered tabs: for drag&drop purpose*/
    std::map<std::string,QWidget*> _registeredTabs;
    
    void setupUi();
   
	void retranslateUi(QMainWindow *MainWindow);
    
};

#endif // GUI_H
