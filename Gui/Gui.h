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

#ifndef POWITER_GUI_GUI_H_
#define POWITER_GUI_GUI_H_

#include <map>
#include <string>
#ifndef Q_MOC_RUN
#include <boost/noncopyable.hpp>
#endif
#include <QtCore/QObject>
#include <QToolButton>
#include <QIcon>
#include <QDialog>
#include <QAction>
#include <QMainWindow>

#include "Global/GlobalDefines.h"

#include "Engine/Format.h"

class QString;
class TabWidget;
class AppInstance;
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
class AppInstance;
class NodeGraph;
class ViewerTab;
class Node;
class ViewerNode;
class QToolBar;
class QGraphicsScene;
class AppInstance;
class NodeGui;
class QProgressBar;
class Button;
class QLabel;
class Writer;
class Gui;
class SpinBox;
class LineEdit;
class DockablePanel;
class ProjectSettingsPanel;
/*Holds just a reference to an action*/
class ActionRef : public QObject{
    Q_OBJECT
    
    AppInstance* _app;
public:
    
    ActionRef(AppInstance* app,QAction* action,const QString& nodeName):_app(app),_action(action),_nodeName(nodeName){
        QObject::connect(action, SIGNAL(triggered()), this, SLOT(onTriggered()));
    }
    virtual ~ActionRef(){QObject::disconnect(_action, SIGNAL(triggered()), this, SLOT(onTriggered()));}
    
    QAction* _action;
    QString _nodeName;
    
    public slots:
    void onTriggered();

};

class ToolButton : public QToolButton{
    AppInstance* _app;
    QMenu* _menu;
    std::vector<QMenu*> _subMenus;
    std::vector<ActionRef*> _actions;
public:
    
    ToolButton(AppInstance* app,
               const QString& actionName,
               const QStringList& firstElement,
               const QString& pluginName,
               QIcon pluginIcon = QIcon(),
               QIcon groupIcon = QIcon(),
               QWidget* parent = 0);
    
    void addTool(const QString& actionName,
                 const QStringList& grouping,
                 const QString& pluginName,
                 QIcon pluginIcon = QIcon());
    
    virtual ~ToolButton();

    
};

/*This class represents a floating pane that embeds a widget*/
class FloatingWidget : public QWidget{
    
    QWidget* _embeddedWidget;
    QVBoxLayout* _layout;
    
public:
    
    explicit FloatingWidget(QWidget* parent = 0);
    
    virtual ~FloatingWidget(){}
    
    /*Set the embedded widget. Only 1 widget can be embedded
     by FloatingWidget. Once set, this function does nothing
     for subsequent calls..*/
    void setWidget(const QSize& widgetSize,QWidget* w);
};

class RenderingProgressDialog : public QDialog{
    
    Q_OBJECT
    
    QVBoxLayout* _mainLayout;
    QLabel* _totalLabel;
    QProgressBar* _totalProgress;
    QFrame* _separator;
    QLabel* _perFrameLabel;
    QProgressBar* _perFrameProgress;
    Button* _cancelButton;
    Writer* _writer;
    QString _sequenceName;
    int _firstFrame;
    int _lastFrame;
public:
    
    RenderingProgressDialog(Writer* writer,const QString& sequenceName,int firstFrame,int lastFrame,QWidget* parent = 0);
    
    virtual ~RenderingProgressDialog(){}
    
public slots:
        
    void onFrameRendered(int);
    
    void onCurrentFrameProgress(int);

    void onCancelation();
};

/*This class encapsulate a nodegraph GUI*/
class NodeGraphTab{
public:

    // FIXME: these (public) data members are never deleted
    // cppcheck message:
    // "Class 'NodeGraphTab' is unsafe, 'NodeGraphTab::_graphScene' can leak by wrong usage."
    QGraphicsScene* _graphScene;
    NodeGraph *_nodeGraphArea;
    
    NodeGraphTab(Gui* gui,QWidget* parent);
    virtual ~NodeGraphTab(){}
};

class Gui : public QMainWindow,public boost::noncopyable
{
    Q_OBJECT
    
    ViewerTab* _lastSelectedViewer;
    
public:
    explicit Gui(AppInstance* app,QWidget* parent=0);
    
    virtual ~Gui();
    
    void createGui();
    
    NodeGui* createNodeGUI(Node *node);
    
    void autoConnect(NodeGui* target,NodeGui* created);
    
    NodeGui* getSelectedNode() const;
    
    void setLastSelectedViewer(ViewerTab* tab){_lastSelectedViewer = tab;}
    
    ViewerTab* getLastSelectedViewer() const {return _lastSelectedViewer;}
    
    bool eventFilter(QObject *target, QEvent *event);

    void createViewerGui(Node* viewer);
    
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
    
    void maximize(TabWidget* what,bool isViewerPane);
    
    void minimize();
    
    /*Returns a valid tab if a tab with a matching name has been
     found. Otherwise returns NULL.*/
    QWidget* findExistingTab(const std::string& name) const;
    
    void registerTab(const std::string& name, QWidget* tab);
    
    void loadStyleSheet();
    
    void addPluginToolButton(const QString& actionName,
                             const QStringList& groups,
                             const QString& pluginName,
                             const QString& pluginIconPath,
                             const QString& groupIconPath);
    void addUndoRedoActions(QAction* undoAction,QAction* redoAction);

    
    bool isGraphWorthless() const;
    
    /**
     * @brief An error dialog with title and text customizable
     **/
    void errorDialog(const QString& title,const QString& text);
    
    void warningDialog(const QString& title,const QString& text);
    
    void informationDialog(const QString& title,const QString& text);
    
    Powiter::StandardButton questionDialog(const QString& title,const QString& message,Powiter::StandardButtons buttons =
                                           Powiter::StandardButtons(Powiter::Yes | Powiter::No),
                                           Powiter::StandardButton defaultButton = Powiter::NoButton);
    
    void selectNode(NodeGui* node);
    
    AppInstance* getApp() { return _appInstance; }
    
    void showProgressDialog(Writer* writer,const QString& sequenceName,int firstFrame,int lastFrame);

    const std::map<QString,ToolButton*>& getToolButtons() const {return _toolGroups;}
    
    void updateViewsActions(int viewsCount);
    
    static QKeySequence keySequenceForView(int v);
    
private:

    void addNodeGraph();
    
    void restoreGuiGeometry();
    
    void saveGuiGeometry();


public slots:
    
    bool exit();
    void toggleFullScreen();
    void closeEvent(QCloseEvent *e);
    void newProject();
    void openProject();
    void saveProject();
    void saveProjectAs();
    void autoSave();
    
    void connectInput1();
    void connectInput2();
    void connectInput3();
    void connectInput4();
    void connectInput5();
    void connectInput6();
    void connectInput7();
    void connectInput8();
    void connectInput9();
    void connectInput10();
    
    void showView0();
    void showView1();
    void showView2();
    void showView3();
    void showView4();
    void showView5();
    void showView6();
    void showView7();
    void showView8();
    void showView9();
    
    /*Returns a code from the save dialog:
     * -1  = unrecognized code
     * 0 = Save
     * 1 = Discard
     * 2 = Cancel or Escape
     **/
    int saveWarning();
    
    void setVisibleProjectSettingsPanel();
    
    void putSettingsPanelFirst(DockablePanel* panel);
    
private:
    AppInstance* _appInstance;
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
    QAction *actionClearDiskCache;
    QAction *actionClearPlayBackCache;
    QAction *actionClearNodeCache;
    
    QAction* actionConnectInput1;
    QAction* actionConnectInput2;
    QAction* actionConnectInput3;
    QAction* actionConnectInput4;
    QAction* actionConnectInput5;
    QAction* actionConnectInput6;
    QAction* actionConnectInput7;
    QAction* actionConnectInput8;
    QAction* actionConnectInput9;
    QAction* actionConnectInput10;
    
    
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
    std::list<ViewerTab*> _viewerTabs;
    
    /*GRAPH*/
    //======================
    
    NodeGraphTab* _nodeGraphTab;
    
    /*TOOLBOX*/
    QToolBar* _toolBox;
    std::map<QString,ToolButton*> _toolGroups;
    
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
    QMenu *viewerInputsMenu;
    QMenu *viewersViewMenu;
    QMenu *cacheMenu;
    
    
    /*all TabWidget's : used to know what to hide/show for fullscreen mode*/
    std::list<TabWidget*> _panes;
    
    /*Registered tabs: for drag&drop purpose*/
    std::map<std::string,QWidget*> _registeredTabs;
    
    ProjectSettingsPanel* _projectGui;
    
    void setupUi();
   
	void retranslateUi(QMainWindow *MainWindow);
    
};



class AddFormatDialog : public QDialog {
    
    QVBoxLayout* _mainLayout;
    
    QWidget* _secondLine;
    QHBoxLayout* _secondLineLayout;
    QLabel* _widthLabel;
    SpinBox* _widthSpinBox;
    QLabel* _heightLabel;
    SpinBox* _heightSpinBox;
    QLabel* _pixelAspectLabel;
    SpinBox* _pixelAspectSpinBox;
    
    
    QWidget* _thirdLine;
    QHBoxLayout* _thirdLineLayout;
    QLabel* _nameLabel;
    LineEdit* _nameLineEdit;
    
    QWidget* _fourthLine;
    QHBoxLayout* _fourthLineLayout;
    Button* _cancelButton;
    Button* _okButton;
    
public:
    
    AddFormatDialog(QWidget* parent = 0);
    
    virtual ~AddFormatDialog(){}
    
    Format getFormat() const ;
    
};


#endif // POWITER_GUI_GUI_H_
