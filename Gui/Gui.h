//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012. 
*contact: immarespond at gmail dot com
*
*/

#ifndef NATRON_GUI_GUI_H_
#define NATRON_GUI_GUI_H_

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
#include <QWaitCondition>
#include <QMutex>

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
class InspectorNode;
class QToolBar;
class QGraphicsScene;
class NodeGui;
class QProgressBar;
class ViewerInstance;
class Button;
class QLabel;
class Gui;
class SpinBox;
class LineEdit;
class DockablePanel;
class PluginToolButton;
class ComboBox;
class CurveEditor;

namespace Natron{
    class Node;
    class OutputEffectInstance;
    class Project;
}


class ToolButton : public QObject {
    Q_OBJECT
    
public:
    
    ToolButton(AppInstance* app,
               PluginToolButton* pluginToolButton,
               const QString& pluginID,
               const QString& label,
               QIcon icon = QIcon())
    : _app(app)
    , _id(pluginID)
    , _label(label)
    , _icon(icon)
    , _menu(NULL)
    , _children()
    , _action(NULL)
    , _pluginToolButton(pluginToolButton)
    {
    }
    
    
    virtual ~ToolButton(){}
    
    const QString& getID() const { return _id; }

    const QString& getLabel() const { return _label; };
    
    const QIcon& getIcon() const { return _icon; };
    
    bool hasChildren() const { return !_children.empty(); }

    QMenu* getMenu() const { assert(_menu); return _menu; }
    
    void setMenu(QMenu* menu ) { _menu = menu; }
    
    void tryAddChild(ToolButton* child);
    
    const std::vector<ToolButton*>& getChildren() const { return _children; }
    
    QAction* getAction() const { return _action; }
    
    void setAction(QAction* action) {_action = action;}
    
    PluginToolButton* getPluginToolButton() const { return _pluginToolButton; }
    
public slots:
    
    void onTriggered();

private:
    AppInstance* _app;
    QString _id;
    QString _label;
    QIcon _icon;
    QMenu* _menu;
    std::vector<ToolButton*> _children;
    QAction* _action;
    PluginToolButton* _pluginToolButton;
};

/*This class represents a floating pane that embeds a widget*/
class FloatingWidget : public QWidget{

public:
    
    explicit FloatingWidget(QWidget* parent = 0);
    
    virtual ~FloatingWidget(){}
    
    /*Set the embedded widget. Only 1 widget can be embedded
     by FloatingWidget. Once set, this function does nothing
     for subsequent calls..*/
    void setWidget(const QSize& widgetSize,QWidget* w);

private:
    QWidget* _embeddedWidget;
    QVBoxLayout* _layout;
};

class RenderingProgressDialog : public QDialog {
    
    Q_OBJECT
    
public:
    
    RenderingProgressDialog(const QString& sequenceName,int firstFrame,int lastFrame,QWidget* parent = 0);
    
    virtual ~RenderingProgressDialog(){}
    
    void onFrameRendered(int);
    
    void onCurrentFrameProgress(int);

signals:
    
    void canceled();
    
private:
    QVBoxLayout* _mainLayout;
    QLabel* _totalLabel;
    QProgressBar* _totalProgress;
    QFrame* _separator;
    QLabel* _perFrameLabel;
    QProgressBar* _perFrameProgress;
    Button* _cancelButton;
    QString _sequenceName;
    int _firstFrame;
    int _lastFrame;
};

class Gui : public QMainWindow,public boost::noncopyable
{
    Q_OBJECT
    
public:
    explicit Gui(AppInstance* app,QWidget* parent=0);
    
    virtual ~Gui();
    
    void createGui();
    
    NodeGui* createNodeGUI(Natron::Node *node);

    void addNodeGuiToCurveEditor(NodeGui *node);
    
    void autoConnect(NodeGui* target,NodeGui* created);
    
    NodeGui* getSelectedNode() const;
    
    void setLastSelectedViewer(ViewerTab* tab){_lastSelectedViewer = tab;}
    
    ViewerTab* getLastSelectedViewer() const {return _lastSelectedViewer;}
    
    bool eventFilter(QObject *target, QEvent *event);

    void createViewerGui(Natron::Node* viewer);
    
    /*Called internally by the viewer node. It adds
     a new Viewer tab GUI and returns a pointer to it.*/
    ViewerTab* addNewViewerTab(ViewerInstance* node,TabWidget* where);
    
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
    
    ToolButton* findExistingToolButton(const QString& name) const;
    
    ToolButton* findOrCreateToolButton(PluginToolButton* plugin);
    
    const std::vector<ToolButton*>& getToolButtons() const {return _toolButtons;}
    
    void addUndoRedoActions(QAction* undoAction,QAction* redoAction);

    
    bool isGraphWorthless() const;
    
    /**
     * @brief An error dialog with title and text customizable
     **/
    void errorDialog(const std::string& title,const std::string& text);
    
    void warningDialog(const std::string& title,const std::string& text);
    
    void informationDialog(const std::string& title,const std::string& text);
    
    Natron::StandardButton questionDialog(const std::string& title,const std::string& message,Natron::StandardButtons buttons =
                                           Natron::StandardButtons(Natron::Yes | Natron::No),
                                           Natron::StandardButton defaultButton = Natron::NoButton);
    
    void selectNode(NodeGui* node);
    
    AppInstance* getApp() { return _appInstance; }
        
    void updateViewsActions(int viewsCount);
    
    static QKeySequence keySequenceForView(int v);
    
    /*set the curve editor as the active widget of its pane*/
    void setCurveEditorOnTop();
        

private:
    
    void restoreGuiGeometry();
    
    void saveGuiGeometry();
    

signals:
    
    void doDialog(int type,const QString& title,const QString& content,Natron::StandardButtons buttons,int defaultB);

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
    
    void onDoDialog(int type,const QString& title,const QString& content,Natron::StandardButtons buttons,int defaultB);
    
    /*Returns a code from the save dialog:
     * -1  = unrecognized code
     * 0 = Save
     * 1 = Discard
     * 2 = Cancel or Escape
     **/
    int saveWarning();
    
    void setVisibleProjectSettingsPanel();
    
    void putSettingsPanelFirst(DockablePanel* panel);
    
    void addToolButttonsToToolBar();

private:

    ViewerTab* _lastSelectedViewer;
    AppInstance* _appInstance;
    QWaitCondition _uiUsingMainThreadCond;
    bool _uiUsingMainThread;
    mutable QMutex _uiUsingMainThreadMutex;
    Natron::StandardButton _lastQuestionDialogAnswer;
public:
    // FIXME: public pointer members are the sign of a serious design flaw!!! should at least be shared_ptr!
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
    
    QGraphicsScene* _graphScene;
    NodeGraph *_nodeGraphArea;

    CurveEditor *_curveEditor;
    
    /*TOOLBOX*/
    QToolBar* _toolBox;
    std::vector<ToolButton*> _toolButtons;
    
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
    
    DockablePanel* _projectGui;
    
    void setupUi();
   
	void retranslateUi(QMainWindow *MainWindow);
    
};



class AddFormatDialog : public QDialog {
    Q_OBJECT

public:
    
    AddFormatDialog(Natron::Project* project,QWidget* parent = 0);
    
    virtual ~AddFormatDialog(){}
    
    Format getFormat() const ;

public slots:

    void onCopyFromViewer();

private:
    Natron::Project* _project;

    QVBoxLayout* _mainLayout;

    QWidget* _fromViewerLine;
    QHBoxLayout* _fromViewerLineLayout;
    Button* _copyFromViewerButton;
    ComboBox* _copyFromViewerCombo;

    QWidget* _parametersLine;
    QHBoxLayout* _parametersLineLayout;
    QLabel* _widthLabel;
    SpinBox* _widthSpinBox;
    QLabel* _heightLabel;
    SpinBox* _heightSpinBox;
    QLabel* _pixelAspectLabel;
    SpinBox* _pixelAspectSpinBox;


    QWidget* _formatNameLine;
    QHBoxLayout* _formatNameLayout;
    QLabel* _nameLabel;
    LineEdit* _nameLineEdit;

    QWidget* _buttonsLine;
    QHBoxLayout* _buttonsLineLayout;
    Button* _cancelButton;
    Button* _okButton;
};


#endif // NATRON_GUI_GUI_H_
