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
#include "Superviser/powiterFn.h"
#include <QtWidgets/QMainWindow>
#include <boost/noncopyable.hpp>
#include <QtWidgets/QDialog>
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
class TextureCache;
class NodeGraph;
class ViewerTab;
class Node;
class Viewer;
class QToolBox;
class QGraphicsScene;

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
    
    void createNodeGUI(Powiter_Enums::UI_NODE_TYPE type,Node *node,double x,double y);
    
    bool eventFilter(QObject *target, QEvent *event);

    
    /*Called internally by the viewer node. It adds
     a new Viewer tab GUI and returns a pointer to it.*/
    ViewerTab* addViewerTab(Viewer* node,TabWidget* where);
    
    /*Called internally by the viewer node when
     it gets deleted. This removes the 
     associated GUI. It may also be called from the TabWidget
     that wants to close.*/
    void removeViewerTab(ViewerTab* tab,bool initiatedFromNode);
    
    void setNewViewerAnchor(TabWidget* where){_nextViewerTabPlace = where;}
    
    void moveTab(QWidget* what,TabWidget* where);
    
    void splitPaneHorizontally(TabWidget* what);
    
    void splitPaneVertically(TabWidget* what);
    
    void floatWidget(QWidget* what);
    
    void closePane(TabWidget* what);
    
    void setFullScreen(TabWidget* what);
    
    void exitFullScreen();
    
    /*Returns a valid tab if a tab with a matching name has been
     found. Otherwise returns NULL.*/
    QWidget* findExistingTab(const std::string& name) const;
    
    void registerTab(std::string name,QWidget* tab);
    
    void loadStyleSheet();
    
    TextureCache* getTextureCache() const{return _textureCache;}

private:

    TextureCache* _textureCache;
    void clearTextureCache();
    void addNodeGraph();


public slots:
    void exit();
    void closeEvent(QCloseEvent *e);
    void clearTexCache(){clearTextureCache();}
    
public:
    /*TOOL BAR ACTIONS*/
    //======================
    QAction *actionNew_project;
    QAction *actionOpen_project;
    QAction *actionSave_project;
    QAction *actionPreferences;
    QAction *actionExit;
    QAction *actionUndo;
    QAction *actionRedo;
    QAction *actionProject_settings;
	QAction *actionSplitViewersTab;
    QAction *actionClearDiskCache;
    QAction *actionClearPlayBackCache;
    QAction *actionClearNodeCache;
    QAction *actionClearTextureCache;
    
    
    QWidget *_centralWidget;
    QHBoxLayout* _mainLayout;
    
    TabWidget* _toolsPane;
    
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
    QToolBox* _toolBox;
    
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
