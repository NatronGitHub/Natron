//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#ifndef NATRON_GUI_GUI_H_
#define NATRON_GUI_GUI_H_

#ifndef Q_MOC_RUN
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#endif

#include "Global/Macros.h"

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QMainWindow>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Global/GlobalDefines.h"
#include "Gui/SerializableWindow.h"

#define kMainSplitterObjectName "ToolbarSplitter"

//boost
namespace boost {
namespace archive {
class xml_iarchive;
class xml_oarchive;
}
}

//QtGui
class Splitter;
class QUndoStack;
class QScrollArea;
class NodeBackDrop;
class QToolButton;
class QVBoxLayout;
class QMutex;

//Natron gui
class GuiLayoutSerialization;
class GuiAppInstance;
class NodeGui;
class TabWidget;
class ToolButton;
class ViewerTab;
class DockablePanel;
class NodeGraph;
class CurveEditor;
class Histogram;
class NodeBackDropSerialization;
class RotoGui;
class FloatingWidget;
class BoundAction;

//Natron engine
class ViewerInstance;
class PluginGroupNode;
class Color_Knob;
class ProcessHandler;
namespace Natron {
class Node;
class Image;
class EffectInstance;
class OutputEffectInstance;
}


struct GuiPrivate;
class Gui
    : public QMainWindow, public SerializableWindow, public boost::noncopyable
{
    Q_OBJECT

public:
    explicit Gui(GuiAppInstance* app,
                 QWidget* parent = 0);

    virtual ~Gui() OVERRIDE;

    /**
     * @brief Creates the whole gui. Must be called only once after the Gui object has been created.
     **/
    void createGui();

    boost::shared_ptr<NodeGui> createNodeGUI(boost::shared_ptr<Natron::Node> node,
                                             bool requestedByLoad,
                                             double xPosHint,
                                             double yPosHint,
                                             bool pushUndoRedoCommand);

    void addNodeGuiToCurveEditor(boost::shared_ptr<NodeGui> node);

    const std::list<boost::shared_ptr<NodeGui> > & getSelectedNodes() const;

    void setLastSelectedViewer(ViewerTab* tab);

    ViewerTab* getLastSelectedViewer() const;

    bool eventFilter(QObject *target, QEvent* e);

    void createViewerGui(boost::shared_ptr<Natron::Node> viewer);

    /**
     * @brief Calling this will force the next viewer to be created in the given pane.
     **/
    void setNextViewerAnchor(TabWidget* where);

    /**
     * @brief Returns the pane where all viewers are created by default is none was specified via
     * setNextViewerAnchor. It is also the pane that receives all tabs that are moved due to a pane being closed.
     **/
    TabWidget* getAnchor() const;

    /*Called internally by the viewer node. It adds
       a new Viewer tab GUI and returns a pointer to it.*/
    ViewerTab* addNewViewerTab(ViewerInstance* node,TabWidget* where);

    void addViewerTab(ViewerTab* tab,TabWidget* where);

    /*Called internally by the viewer node when
       it gets deleted. This removes the
       associated GUI. It may also be called from the TabWidget
       that wants to close. The deleteData flag tells whether we actually want
       to destroy the tab/node or just hide them.*/
    void removeViewerTab(ViewerTab* tab,bool initiatedFromNode,bool deleteData);

    Histogram* addNewHistogram();

    void removeHistogram(Histogram* h);

    const std::list<Histogram*> & getHistograms() const;
    std::list<Histogram*> getHistograms_mt_safe() const;

    void maximize(TabWidget* what);

    void minimize();

    void loadStyleSheet();

    ToolButton* findExistingToolButton(const QString & name) const;
    ToolButton* findOrCreateToolButton(PluginGroupNode* plugin);
    const std::vector<ToolButton*> & getToolButtons() const;

    void registerNewUndoStack(QUndoStack* stack);

    void removeUndoStack(QUndoStack* stack);

    /**
     * @brief An error dialog with title and text customizable
     **/
    void errorDialog(const std::string & title,const std::string & text);

    void warningDialog(const std::string & title,const std::string & text);

    void informationDialog(const std::string & title,const std::string & text);

    Natron::StandardButton questionDialog(const std::string & title,const std::string & message,Natron::StandardButtons buttons =
                                              Natron::StandardButtons(Natron::Yes | Natron::No),
                                          Natron::StandardButton defaultButton = Natron::NoButton);

    /**
     * @brief Selects the given node on the node graph, wiping any previous selection.
     **/
    void selectNode(boost::shared_ptr<NodeGui> node);

    GuiAppInstance* getApp() const;

    void updateViewsActions(int viewsCount);

    static QKeySequence keySequenceForView(int v);

    /*set the curve editor as the active widget of its pane*/
    void setCurveEditorOnTop();

    ///Make the layout of the application as it is the first time Natron is opened
    void createDefaultLayout1();

    ///Make the layout according to the serialization.
    ///@param enableOldProjectCompatibility When true, the default Gui layout will be created
    ///prior to restoring. This is because older projects didn't have as much infos to recreate the entire layout.
    void restoreLayout(bool wipePrevious,bool enableOldProjectCompatibility,
                       const GuiLayoutSerialization & layoutSerialization);

    const std::list<TabWidget*> & getPanes() const;
    std::list<TabWidget*> getPanes_mt_safe() const;

    int getPanesCount() const;

    /**
     * @brief If baseName is already used by another pane or it is empty,this function will return a new pane name that is not already
     * used by another pane. Otherwise it will return baseName.
     **/
    QString getAvailablePaneName( const QString & baseName = QString() ) const;

    void registerPane(TabWidget* pane);
    void unregisterPane(TabWidget* pane);

    void registerTab(QWidget* tab);
    void unregisterTab(QWidget* tab);

    void registerFloatingWindow(FloatingWidget* window);
    void unregisterFloatingWindow(FloatingWidget* window);


    void registerSplitter(Splitter* s);
    void unregisterSplitter(Splitter* s);

    /**
     * @brief MT-Safe
     **/
    std::list<FloatingWidget*> getFloatingWindows() const;
    


    /*Returns a valid tab if a tab with a matching name has been
       found. Otherwise returns NULL.*/
    QWidget* findExistingTab(const std::string & name) const;


    void appendTabToDefaultViewerPane(QWidget* tab);

    /**
     * @brief Get the central of the application, it is either 1 TabWidget or a Splitter.
     * We don't take into account the splitter separating the left toolbar and the central widget.
     * That left splitter is called kMainSplitterObjectName for that reason.
     *
     * Mt-safe
     **/
    QWidget* getCentralWidget() const;
    std::string popOpenFileDialog(bool sequenceDialog,
                                  const std::vector<std::string> & initialfilters,const std::string & initialDir,
                                  bool allowRelativePaths);
    std::string popSaveFileDialog(bool sequenceDialog,const std::vector<std::string> & initialfilters,const std::string & initialDir,
                                  bool allowRelativePaths);

    std::string openImageSequenceDialog();
    std::string saveImageSequenceDialog();
    
    void setUserScrubbingTimeline(bool b);

    bool isUserScrubbingTimeline() const;

    /*Refresh all previews if the project's preview mode is auto*/
    void refreshAllPreviews();

    /*force a refresh on all previews no matter what*/
    void forceRefreshAllPreviews();

    void startDragPanel(QWidget* panel);

    QWidget* stopDragPanel();

    bool isDraggingPanel() const;

    void updateRecentFileActions();

    static QPixmap screenShot(QWidget* w);

    void loadProjectGui(boost::archive::xml_iarchive & obj) const;

    void saveProjectGui(boost::archive::xml_oarchive & archive);

    void setColorPickersColor(const QColor & c);

    void registerNewColorPicker(boost::shared_ptr<Color_Knob> knob);

    void removeColorPicker(boost::shared_ptr<Color_Knob> knob);

    bool hasPickers() const;

    void initProjectGuiKnobs();

    void updateViewersViewsMenu(int viewsCount);

    void setViewersCurrentView(int view);

    const std::list<ViewerTab*> & getViewersList() const;
    std::list<ViewerTab*> getViewersList_mt_safe() const;

    void activateViewerTab(ViewerInstance* viewer);

    void deactivateViewerTab(ViewerInstance* viewer);

    ViewerTab* getViewerTabForInstance(ViewerInstance* node) const;
    const std::list<boost::shared_ptr<NodeGui> > & getVisibleNodes() const;
    std::list<boost::shared_ptr<NodeGui> > getVisibleNodes_mt_safe() const;

    void deselectAllNodes() const;

    void onProcessHandlerStarted(const QString & sequenceName,int firstFrame,int lastFrame,
                                 const boost::shared_ptr<ProcessHandler> & process);

    void onWriterRenderStarted(const QString & sequenceName,int firstFrame,int lastFrame,
                               Natron::OutputEffectInstance* writer);

    NodeGraph* getNodeGraph() const;
    CurveEditor* getCurveEditor() const;
    QVBoxLayout* getPropertiesLayout() const;
    QScrollArea* getPropertiesScrollArea() const;
    const std::map<std::string,QWidget*> & getRegisteredTabs() const;

    void updateLastSequenceOpenedPath(const QString & path);

    void updateLastSequenceSavedPath(const QString & path);

    void setUndoRedoStackLimit(int limit);

    void setGlewVersion(const QString & version);

    void setOpenGLVersion(const QString & version);

    QString getGlewVersion() const;

    QString getOpenGLVersion() const;

    QString getBoostVersion() const;

    QString getQtVersion() const;

    QString getCairoVersion() const;
    
    /**
     * @brief Make a new rotoscoping/painting interface for the given node.
     * This will create new widgets and enrich the interface of the viewer tab.
     **/
    void createNewRotoInterface(NodeGui* n);

    /**
     * @brief Set the RotoGui for the node n to be active on all viewers.
     **/
    void setRotoInterface(NodeGui* n);

    /**
     * @brief Called by Gui::deactivateRotoInterface and by NodeGraph::deleteNodepluginsly
     **/
    void removeRotoInterface(NodeGui* n,bool pluginsly);

    void onViewerRotoEvaluated(ViewerTab* viewer);

    /**
     * @brief Make a new tracker interface for the given node.
     * This will create new widgets and enrich the interface of the viewer tab.
     **/
    void createNewTrackerInterface(NodeGui* n);

    void removeTrackerInterface(NodeGui* n,bool pluginsly);

    void startProgress(Natron::EffectInstance* effect,const std::string & message);

    void endProgress(Natron::EffectInstance* effect);

    bool progressUpdate(Natron::EffectInstance* effect,double t);

    /*Useful function that saves on disk the image in png format.
       The name of the image will be the hash key of the image.*/
    static void debugImage( const Natron::Image* image,const QString & filename = QString() );

    void addVisibleDockablePanel(DockablePanel* panel);
    void removeVisibleDockablePanel(DockablePanel* panel);

    NodeBackDrop* createBackDrop(bool requestedByLoad,const NodeBackDropSerialization & serialization);
    std::list<ToolButton*> getToolButtonsOrdered() const;

    void setToolButtonMenuOpened(QToolButton* button);
    QToolButton* getToolButtonMenuOpened() const;

    void connectViewersToViewerCache();

    void disconnectViewersFromViewerCache();

    ///Close the application instance, asking questions to the user
    bool closeInstance();

    void aboutToSave();
    void saveFinished();
    
    void checkNumberOfNonFloatingPanes();

    void openProject(const std::string& filename);
    
    bool isGUIFrozen() const;
    
    /**
     * @brief If returns true then you must add shorcuts to the shortcut editor using the addShortcut function
     **/
    bool hasShortcutEditorAlreadyBeenBuilt() const;
    
    void addShortcut(BoundAction* action);
    
    const QString& getLastLoadProjectDirectory() const;
    
    const QString& getLastSaveProjectDirectory() const;
signals:

    void doDialog(int type,const QString & title,const QString & content,Natron::StandardButtons buttons,int defaultB);

    ///emitted when a viewer changes its name or is deleted/added
    void viewersChanged();

public slots:

    ///Close the project instance, asking questions to the user and leaving the main window intact
    void closeProject();
    void toggleFullScreen();
    void closeEvent(QCloseEvent* e);
    void newProject();
    void openProject();
    bool saveProject();
    bool saveProjectAs();
    void autoSave();

    void createNewViewer();

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

    void onDoDialog(int type,const QString & title,const QString & content,Natron::StandardButtons buttons,int defaultB);

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

    void onCurrentUndoStackChanged(QUndoStack* stack);

    void showSettings();

    void showAbout();

    void showShortcutEditor();

    void showOfxLog();
    
    void openRecentFile();

    void onProjectNameChanged(const QString & name);

    void onNodeNameChanged(const QString & name);

    void onViewerImageChanged(int texIndex);

    boost::shared_ptr<Natron::Node> createReader();
    boost::shared_ptr<Natron::Node> createWriter();

    void renderAllWriters();

    void renderSelectedNode();

    void onRotoSelectedToolChanged(int tool);

    void onMaxVisibleDockablePanelChanged(int maxPanels);

    void clearAllVisiblePanels();

    void onMaxPanelsSpinBoxValueChanged(double val);

    void exportLayout();

    void importLayout();

    void restoreDefaultLayout();

    void onFreezeUIButtonClicked(bool clicked);
private:

    /**
     * @brief Close project right away, without any user interaction.
     * @param quitApp If true, the application will exit, otherwise the main window will stay active.
     **/
    void abortProject(bool quitApp);

    void openProjectInternal(const std::string & absoluteFileName);

    void setupUi();

    void wipeLayout();

    void createDefaultLayoutInternal(bool wipePrevious);

    void createMenuActions();

    virtual void moveEvent(QMoveEvent* e) OVERRIDE FINAL;
    virtual void resizeEvent(QResizeEvent* e) OVERRIDE FINAL;
    boost::scoped_ptr<GuiPrivate> _imp;
};


/*This class represents a floating pane that embeds a widget*/
class FloatingWidget
    : public QWidget, public SerializableWindow
{
    Q_OBJECT

public:

    explicit FloatingWidget(Gui* gui,
                            QWidget* parent = 0);

    virtual ~FloatingWidget();

    /*Set the embedded widget. Only 1 widget can be embedded
       by FloatingWidget. Once set, this function does nothing
       for subsequent calls..*/
    void setWidget(QWidget* w);

    void removeEmbeddedWidget();

    QWidget* getEmbeddedWidget() const
    {
        return _embeddedWidget;
    }

signals:

    void closed();

private:

    virtual void moveEvent(QMoveEvent* e) OVERRIDE FINAL;
    virtual void resizeEvent(QResizeEvent* e) OVERRIDE FINAL;
    virtual void closeEvent(QCloseEvent* e) OVERRIDE;
    QWidget* _embeddedWidget;
    QVBoxLayout* _layout;
    Gui* _gui;
};

#endif // NATRON_GUI_GUI_H_
