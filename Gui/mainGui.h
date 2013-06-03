//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com
#ifndef GUI_H
#define GUI_H
#include <QtCore/QVariant>
#include "Gui/DAG.h"
#include "Gui/viewerTab.h"
#include "Superviser/powiterFn.h"
#include <QtWidgets/QMainWindow>
#define RIGHTDOCK_WIDTH 380

using namespace Powiter_Enums;
class QAction;
class QTabWidget;
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
class Gui:public QMainWindow
{
    Q_OBJECT
public:
    Gui(QWidget* parent=0);
    virtual ~Gui();
	

    void setControler(Controler* crossPlatform);
    void createGui();
    void addNode_ui(qreal x,qreal y,UI_NODE_TYPE type,Node *node);
    bool eventFilter(QObject *target, QEvent *event);

protected:
    virtual void keyPressEvent(QKeyEvent* e);
    
private:

    TextureCache* _textureCache;
    Controler* crossPlatform; // Pointer to the controler
    
    void clearTextureCache();

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
    QAction *actionClearBufferCache;
    QAction *actionClearTextureCache;
    /*CENTRAL ZONE SPLITING*/
    //======================
    QWidget *centralwidget;
    QVBoxLayout *verticalLayout;
    QFrame *centralFrame;
    QVBoxLayout *verticalLayout_2;
    QSplitter *splitter;
    
    
	/*VIEWERS*/
	//======================
    
    QTabWidget *viewersTabContainer;
    ViewerTab *viewer_tab;
    
    /*GRAPH*/
    //======================
    QTabWidget *WorkShop;
    QWidget *GraphEditor;
    QVBoxLayout *verticalLayout_3;
    QGraphicsScene* graph_scene;
    NodeGraph *nodeGraphArea;
    QWidget *scrollAreaWidgetContents;
    
    /*CURVE*/
    //======================
    QWidget *CurveEditor;
    
    /*PROGRESS BAR*/
    //======================
    QProgressBar *progressBar;
    
    /*MENU*/
    //======================
    QMenuBar *menubar;
    QMenu *menuFile;
    QMenu *menuEdit;
    QMenu *menuDisplay;
    QMenu *menuOptions;
	QMenu *viewersMenu;
    QMenu *cacheMenu;
    /*LEFT DOCK*/
    //======================
    QDockWidget *ToolDock;
    QWidget *ToolDockContent;
    QVBoxLayout *verticalLayout_5;
    QTreeView *ToolChooser;
    /*RIGHT DOCK*/
    //======================
    QDockWidget* rightDock;
    QDockWidget *PropertiesDock;
    QScrollArea *PropertiesDockContent;
    QWidget *propertiesContainer;
    QVBoxLayout *layout_settings;
    /*STATUS BAR*/
    //======================
    QStatusBar *statusbar;
    
    void setupUi(Controler* ctrl);
   
	void retranslateUi(QMainWindow *MainWindow);
    
};

#endif // GUI_H
