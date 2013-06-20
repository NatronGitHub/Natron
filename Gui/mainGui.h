//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com
#ifndef GUI_H
#define GUI_H
#include <QtCore/QVariant>
#include "Superviser/powiterFn.h"
#include <QtWidgets/QMainWindow>
#include <boost/noncopyable.hpp>



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
class QToolBar;
class QGraphicsScene;

/*This object encapsulate a nodegraph GUI*/
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
    ViewerTab* addViewerTab(Viewer* node);
    
    /*Called internally by the viewer node when
     it gets deleted. This removes the 
     associated GUI.*/
    void removeViewerTab(ViewerTab* tab);

    
    
protected:
    virtual void keyPressEvent(QKeyEvent* e);
    
private:

    TextureCache* _textureCache;
    void clearTextureCache();

public slots:
    void exit();
    void closeEvent(QCloseEvent *e);
    void clearTexCache(){clearTextureCache();}
    void addNodeGraph();
    void removeNodeGraph(NodeGraphTab* tab);
    
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
    TabWidget* _workshopPane;
    QSplitter* _viewerWorkshopSplitter;
    
    TabWidget* _propertiesPane;
    QSplitter* _middleRightSplitter;

    QSplitter* _leftRightSplitter;
    
	/*VIEWERS*/
	//======================
    std::vector<ViewerTab*> _viewerTabs;
    
    /*GRAPHS*/
    //======================
    
    NodeGraphTab* _nodeGraphTab;
    
    
    /*TOOLBAR*/
    QToolBar* _toolBar;
    
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
    
    
    void setupUi();
   
	void retranslateUi(QMainWindow *MainWindow);
    
};

#endif // GUI_H
