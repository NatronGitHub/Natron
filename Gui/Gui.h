/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
 *
 * Natron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Natron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 * ***** END LICENSE BLOCK ***** */

#ifndef Gui_Gui_h
#define Gui_Gui_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <set>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#endif

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QtGlobal> // for Q_OS_*
#include <QMainWindow>
#include <QtCore/QUrl>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Global/GlobalDefines.h"

#include "Engine/ScriptObject.h"
#include "Engine/ViewIdx.h"
#include "Engine/SerializableWindow.h"
#include "Engine/TimeLineKeys.h"
#include "Engine/TimeValue.h"

#ifdef __NATRON_WIN32__
#include "Gui/FileTypeMainWindow_win.h"
#endif
#include "Gui/RegisteredTabs.h"

#include "Gui/GuiFwd.h"


NATRON_NAMESPACE_ENTER


#define kMainSplitterObjectName "ToolbarSplitter"


struct GuiPrivate;

class Gui
#ifndef __NATRON_WIN32__
    : public QMainWindow
#else
    : public DocumentWindow
#endif
      , public SerializableWindow
      , public boost::noncopyable
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    friend class PanelWidget;

    explicit Gui(const GuiAppInstancePtr& app,
                 QWidget* parent = 0);

    virtual ~Gui() OVERRIDE;

    /**
     * @brief Creates the whole gui. Must be called only once after the Gui object has been created.
     **/
    void createGui();


    void addNodeGuiToAnimationModuleEditor(const NodeGuiPtr &node);
    void removeNodeGuiFromAnimationModuleEditor(const NodeGuiPtr& node);

    void createViewerGui(const NodeGuiPtr& viewer);

    void createGroupGui(const NodePtr& group, const CreateNodeArgs& args);

    void addGroupGui(NodeGraph* tab, TabWidget* where);

    void removeGroupGui(NodeGraph* tab, bool deleteData);


    void setLastSelectedGraph(NodeGraph* graph);

    NodeGraph* getLastSelectedGraph() const;

    void setActiveViewer(ViewerTab* viewer);
    ViewerTab* getActiveViewer() const;
    NodeCollectionPtr getLastSelectedNodeCollection() const;

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
    ViewerTab* addNewViewerTab(const NodeGuiPtr& node, TabWidget* where);

    void addViewerTab(ViewerTab* tab, TabWidget* where);

    /*Called internally by the viewer node when
       it gets deleted. This removes the
       associated GUI. It may also be called from the TabWidget
       that wants to close. The deleteData flag tells whether we actually want
       to destroy the tab/node or just hide them.*/
    void removeViewerTab(ViewerTab* tab, bool initiatedFromNode, bool deleteData);


    Histogram* addNewHistogram();

    void removeHistogram(Histogram* h);

    const std::list<Histogram*> & getHistograms() const;
    std::list<Histogram*> getHistograms_mt_safe() const;

    void maximize(TabWidget* what);

    void minimize();

    static void loadStyleSheet();
    ToolButton* findExistingToolButton(const QString & name) const;
    ToolButton* findOrCreateToolButton(const PluginGroupNodePtr& treeNode);

    void sortAllPluginsToolButtons();

    const std::vector<ToolButton*> & getToolButtons() const;

    void registerNewUndoStack(const boost::shared_ptr<QUndoStack>& stack);

    void removeUndoStack(const boost::shared_ptr<QUndoStack>& stack);

    /**
     * @brief An error dialog with title and text customizable
     **/
    void errorDialog(const std::string & title, const std::string & text, bool useHtml);

    void errorDialog(const std::string & title,
                     const std::string & text,
                     bool* stopAsking,
                     bool useHtml);


    void warningDialog(const std::string & title, const std::string & text, bool useHtml);

    void warningDialog(const std::string & title,
                       const std::string & text,
                       bool* stopAsking,
                       bool useHtml);

    void informationDialog(const std::string & title, const std::string & text, bool useHtml);

    void informationDialog(const std::string & title,
                           const std::string & message,
                           bool* stopAsking,
                           bool useHtml);

    StandardButtonEnum questionDialog(const std::string & title,
                                      const std::string & message,
                                      bool useHtml,
                                      StandardButtons buttons = StandardButtons(eStandardButtonYes | eStandardButtonNo),
                                      StandardButtonEnum defaultButton = eStandardButtonNoButton);

    StandardButtonEnum questionDialog(const std::string & title,
                                      const std::string & message,
                                      bool useHtml,
                                      StandardButtons buttons,
                                      StandardButtonEnum defaultButton,
                                      bool* stopAsking);


    /**
     * @brief Selects the given node on the node graph, wiping any previous selection.
     **/
    void selectNode(const NodeGuiPtr& node);

    GuiAppInstancePtr getApp() const;

    void updateViewsActions(int viewsCount);

    static QKeySequence keySequenceForView(ViewIdx v);

    /*set the curve editor as the active widget of its pane*/
    void setAnimationEditorOnTop();

    ///Make the layout of the application as it is the first time Natron is opened
    void createDefaultLayout1();

    ///Make the layout according to the serialization.
    ///@param enableOldProjectCompatibility When true, the default Gui layout will be created
    ///prior to restoring. This is because older projects didn't have as much info to recreate the entire layout.
    void restoreLayout(bool wipePrevious, bool enableOldProjectCompatibility, const SERIALIZATION_NAMESPACE::WorkspaceSerialization& layoutSerialization);


    void onPaneRegistered(TabWidgetI* pane);
    void onPaneUnRegistered(TabWidgetI* pane);

    void registerTab(PanelWidget* tab, ScriptObject* obj);
    void unregisterTab(PanelWidget* tab);


    /*Returns a valid tab if a tab with a matching name has been
       found. Otherwise returns NULL.*/
    PanelWidget* findExistingTab(const std::string & name) const;
    void findExistingTab(const std::string & name, PanelWidget** w, ScriptObject** o) const;

    void appendTabToDefaultViewerPane(PanelWidget* tab, ScriptObject* obj);

    /**
     * @brief Get the central of the application, it is either 1 TabWidget or a Splitter.
     * We don't take into account the splitter separating the left toolbar and the central widget.
     * That left splitter is called kMainSplitterObjectName for that reason.
     *
     * Mt-safe
     **/
    QWidget* getCentralWidget() const;
    std::string popOpenFileDialog(bool sequenceDialog,
                                  const std::vector<std::string> & initialfilters, const std::string & initialDir,
                                  bool allowRelativePaths);
    std::string popSaveFileDialog(bool sequenceDialog, const std::vector<std::string> & initialfilters, const std::string & initialDir,
                                  bool allowRelativePaths);
    std::string openImageSequenceDialog();
    std::string saveImageSequenceDialog();

    void setDraftRenderEnabled(bool b);

    bool isDraftRenderEnabled() const;

    /*Refresh all previews if the project's preview mode is auto*/
    void refreshAllPreviews();

    /*force a refresh on all previews no matter what*/
    void forceRefreshAllPreviews();

    void startDragPanel(PanelWidget* panel);

    PanelWidget* stopDragPanel(QSize* initialSize);

    bool isDraggingPanel() const;

    void updateRecentFileActions();

    static QPixmap screenShot(QWidget* w);

    ProjectGui* getProjectGui() const;

    void loadProjectGui(bool isAutosave, const SERIALIZATION_NAMESPACE::ProjectSerializationPtr& serialization) const;

    void saveProjectGui(boost::archive::xml_oarchive & archive);

    void setColorPickersColor(ViewIdx view, double r, double g, double b, double a);

    void registerNewColorPicker(KnobColorPtr knob, ViewIdx view);

    void removeColorPicker(KnobColorPtr knob, ViewIdx view);

    void clearColorPickers();

    bool hasPickers() const;

    void initProjectGuiKnobs();

    void setViewersCurrentView(ViewIdx view);

    const std::list<ViewerTab*> & getViewersList() const;
    std::list<ViewerTab*> getViewersList_mt_safe() const;

    void activateViewerTab(const ViewerNodePtr& viewer);

    void deactivateViewerTab(const ViewerNodePtr& viewer);

    ViewerTab* getViewerTabForInstance(const ViewerNodePtr& node) const;
    const NodesGuiList & getVisibleNodes() const;
    NodesGuiList getVisibleNodes_mt_safe() const;

    void deselectAllNodes() const;

    void onRenderStarted(const QString & sequenceName,
                         TimeValue firstFrame, TimeValue lastFrame, TimeValue frameStep,
                         bool canPause,
                         const NodePtr& writer,
                         const ProcessHandlerPtr & process);

    void onRenderRestarted(const NodePtr& writer,
                           const ProcessHandlerPtr & process);

    NodeGraph* getNodeGraph() const;
    AnimationModuleEditor * getAnimationModuleEditor() const;
    ScriptEditor* getScriptEditor() const;
    ProgressPanel* getProgressPanel() const;
    QVBoxLayout* getPropertiesLayout() const;
    PropertiesBinWrapper* getPropertiesBin() const;
    const RegisteredTabs & getRegisteredTabs() const;

    void updateLastSequenceOpenedPath(const QString & path);

    void updateLastSequenceSavedPath(const QString & path);

    void updateLastSavedProjectPath(const QString& project);

    void updateLastOpenedProjectPath(const QString& project);

    void setUndoRedoStackLimit(int limit);

    NodeGuiPtr getCurrentNodeViewerInterface(const PluginPtr& plugin) const;

    /**
     * @brief Make a new viewer interface for the given node.
     * This will create new widgets and enrich the interface of the viewer tab.
     **/
    void createNodeViewerInterface(const NodeGuiPtr& n);

    /**
     * @brief Set the viewer interface for a plug-in to be the one of the node n to be active on all viewers.
     **/
    void setNodeViewerInterface(const NodeGuiPtr& n);

    /**
     * @brief Removes the interface for this node of any viewer
     * @bool permanently If true, the interface will be destroyed instead of hidden
     **/
    void removeNodeViewerInterface(const NodeGuiPtr& n, bool permanently);

    /**
     * @brief Same as removeNodeViewerInterface but for the Viewer node UI only
     **/
    void removeViewerInterface(const NodeGuiPtr& n,
                               bool permanently);

    void progressStart(const NodePtr& node, const std::string &message, const std::string &messageid, bool canCancel = true);

    void progressEnd(const NodePtr& node);

    bool progressUpdate(const NodePtr& node, double t);

    PanelWidget* ensureProgressPanelVisible();
    void ensureScriptEditorVisible();

    /*Useful function that saves on disk the image in png format.
       The name of the image will be the hash key of the image.*/
    static void debugImage( const ImagePtr& image, const RectI& roi, const QString & filename = QString() );

    void addVisibleDockablePanel(DockablePanel* panel);
    void removeVisibleDockablePanel(DockablePanel* panel);

    std::list<ToolButton*> getToolButtonsOrdered() const;

    void setToolButtonMenuOpened(QToolButton* button);
    QToolButton* getToolButtonMenuOpened() const;

    static bool getPresetIcon(const QString& presetFilePath, const QString& presetIconFile, int pixSize, QPixmap* pixmap);

    void checkNumberOfNonFloatingPanes();

    AppInstancePtr openProject(const std::string& filename) WARN_UNUSED_RETURN;

    bool isGUIFrozen() const;

    const QString& getLastLoadProjectDirectory() const;
    const QString& getLastSaveProjectDirectory() const;
    const QString& getLastPluginDirectory() const;

    void updateLastPluginDirectory(const QString& str);



    bool isLeftToolBarDisplayedOnMouseHoverOnly() const;

    void setLeftToolBarDisplayedOnMouseHoverOnly(bool b);

    void refreshLeftToolBarVisibility(const QPoint& globalPos);

    void setLeftToolBarVisible(bool visible);

    void redrawAllViewers();

    void redrawAllTimelines();

    void renderAllViewers();

    void abortAllViewers(bool autoRestartPlayback);

    void toggleAutoHideGraphInputs();

    void centerAllNodeGraphsWithTimer();

    void setLastEnteredTabWidget(TabWidget* tab);

    TabWidget* getLastEnteredTabWidget() const;

    void appendToScriptEditor(const std::string& str);

    void printAutoDeclaredVariable(const std::string& str);

    void addMenuEntry(const QString& menuGrouping, const std::string& pythonFunction, Qt::Key key, const Qt::KeyboardModifiers& modifiers);

    void setTripleSyncEnabled(bool enabled);
    bool isTripleSyncEnabled() const;

    void centerOpenedViewersOn(SequenceTime left, SequenceTime right);

    bool isAboutToClose() const;

    void setRenderStatsEnabled(bool enabled);
    bool areRenderStatsEnabled() const;

    RenderStatsDialog* getRenderStatsDialog() const;
    RenderStatsDialog* getOrCreateRenderStatsDialog();

#ifdef __NATRON_WIN32__
    /**
     * @param filePath file that was selected in the explorer
     */
    virtual void ddeOpenFile(const QString& filePath) OVERRIDE FINAL;
#endif

    /**
     * @brief Returns true on OS X if on a High DPI (Retina) Display.
     **/
#ifndef Q_OS_MAC
    double getHighDPIScaleFactor() const { return 1.; }

#else
    double getHighDPIScaleFactor() const { return QtMac::getHighDPIScaleFactorInternal(this); }
#endif

    /**
     * @brief Fix a bug where icons are wrongly scaled on Qt 4 in QTabBar:
     * https://bugreports.qt.io/browse/QTBUG-23870
     **/
    void scalePixmapToAdjustDPI(QPixmap* pix);
    void scaleImageToAdjustDPI(QImage* pix);


    AppInstancePtr createNewProject();

    /**
     * @brief Close project right away, without any user interaction.
     * @param quitApp If true, the application will exit, otherwise the main window will stay active.
     **/
    bool abortProject(bool quitApp, bool warnUserIfSaveNeeded, bool blocking);

    void setGuiAboutToClose(bool about);

    void notifyGuiClosing();

    /*
     * @brief To be called by "main widgets" such as NodeGraph, Viewer etc... to determine if focus stealing is possible to have
     * mouse position dependent shortcuts without the user actually clicking.
     */
    static bool isFocusStealingPossible();
    PanelWidget* getCurrentPanelFocus() const;

    void setLastKeyPressVisitedClickFocus(bool visited);
    void setLastKeyUpVisitedClickFocus(bool visited);

    /// Handle the viewer keys separately: use the nativeVirtualKey so that they work
    /// on any keyboard, including French AZERTY (where numbers are shifted)
    static int handleNativeKeys(int key, quint32 nativeScanCode, quint32 nativeVirtualKey) WARN_UNUSED_RETURN;

    void setApplicationConsoleActionVisible(bool visible);

    bool saveProjectAs(const std::string& filename);

    static void fileSequencesFromUrls(const QList<QUrl>& urls, std::vector<SequenceParsing::SequenceFromFilesPtr>* sequences);

    void handleOpenFilesFromUrls(const QList<QUrl>& urls, const QPoint& globalPos);

    static void setQMessageBoxAppropriateFont(QMessageBox* widget);

    void updateAboutWindowLibrariesVersion();

    virtual TabWidgetI* isMainWidgetTab() const OVERRIDE FINAL;

    virtual SplitterI* isMainWidgetSplitter() const OVERRIDE FINAL;

    virtual DockablePanelI* isMainWidgetPanel() const OVERRIDE FINAL;

    void refreshTimelineGuiKeyframesLater();

    void refreshTimelineGuiKeyframesNow();

    const TimeLineKeysSet& getTimelineGuiKeyframes() const;

    void setEditExpressionDialogLanguage(ExpressionLanguageEnum lang);

    void setEditExpressionDialogLanguageValid(bool valid);

    ExpressionLanguageEnum getEditExpressionDialogLanguage() const;

protected:

    virtual void restoreChildFromSerialization(const SERIALIZATION_NAMESPACE::WindowSerialization& serialization) OVERRIDE FINAL;
    
    // Reimplemented Protected Functions

    //bool event(QEvent* event) OVERRIDE;

    bool eventFilter(QObject *target, QEvent* e) OVERRIDE;

Q_SIGNALS:

    void mustRefreshTimelineGuiKeyframesLater();

    void doDialog(int type, const QString & title, const QString & content, bool useHtml, StandardButtons buttons, int defaultB);

    void doDialogWithStopAskingCheckbox(int type, const QString & title, const QString & content, bool useHtml, StandardButtons buttons, int defaultB);

    ///emitted when a viewer changes its name or is deleted/added
    void viewersChanged();

    void s_doProgressStartOnMainThread(const KnobHolderPtr& effect, const QString &message, const QString &messageid, bool canCancel);

    void s_doProgressEndOnMainThread(const KnobHolderPtr& effect);

    void s_doProgressUpdateOnMainThread(const KnobHolderPtr& effect, double t);

    void s_showLogOnMainThread();

    void mustRefreshViewersAndKnobsLater();

public Q_SLOTS:

    void onMustRefreshViewersAndKnobsLaterReceived();

    void onMustRefreshTimelineGuiKeyframesLaterReceived();

    void onShowLogOnMainThreadReceived();

    ///Called whenever the time changes on the timeline
    void onTimelineTimeChanged(SequenceTime time, int reason);

    void onTimelineTimeAboutToChange();

    void reloadStylesheet();

    ///Close the project instance, asking questions to the user and leaving the main window intact
    bool closeProject();

    //Same as close + open same project to discard unsaved changes
    void reloadProject();
    void toggleFullScreen();
    void closeEvent(QCloseEvent* e) OVERRIDE;
    void newProject();
    void openProject();
    bool saveProject();
    bool saveProjectAs();
    void saveAndIncrVersion();

    void autoSave();

    void createNewViewer();

    void connectAInput();
    void connectBInput();

    void connectInput(int inputNb, bool isASide);
    void connectAInput(int inputNb);
    void connectBInput(int inputNb);


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

    void onDoDialog(int type, const QString & title, const QString & content, bool useHtml, StandardButtons buttons, int defaultB);

    void onDoDialogWithStopAskingCheckbox(int type, const QString & title, const QString & content, bool useHtml, StandardButtons buttons, int defaultB);
    /*Returns a code from the save dialog:
     * -1  = unrecognized code
     * 0 = Save
     * 1 = Discard
     * 2 = Cancel or Escape
     **/
    int saveWarning();

    void setVisibleProjectSettingsPanel();

    void putSettingsPanelFirst(DockablePanel* panel);

    void buildTabFocusOrderPropertiesBin();

    void addToolButttonsToToolBar();

    void showSettings();

    void showAbout();

    void showErrorLog();

    void openRecentFile();

    void onProjectNameChanged(const QString & filePath, bool modified);

    void onNodeNameChanged(const QString& oldLabel, const QString & newLabel);

    void onViewerImageChanged(int texIndex);

    NodePtr createReader();
    NodePtr createWriter();

    void renderAllWriters();

    void renderSelectedNode();

    void onEnableRenderStatsActionTriggered();

    void onMaxVisibleDockablePanelChanged(int maxPanels);

    void clearAllVisiblePanels();

    void minimizeMaximizeAllPanels(bool clicked);

    void onMaxPanelsSpinBoxValueChanged(double val);

    void exportLayout();

    void importLayout();

    void restoreDefaultLayout();

    void onFreezeUIButtonClicked(bool clicked);

    void refreshAllTimeEvaluationParams(bool onlyTimeEvaluationKnobs);

    void onPropertiesScrolled();

    void onNextTabTriggered();

    void onPrevTabTriggered();

    void onCloseTabTriggered();

    void onUserCommandTriggered();

    void onFocusChanged(QWidget* old, QWidget*);

    void onShowApplicationConsoleActionTriggered();

    void openHelpWebsite();
    void openHelpForum();
    void openHelpIssues();
    void openHelpDocumentation();

#ifdef Q_OS_MAC
    void dockClicked();
#endif
    
private:

    void importLayoutInternal(const std::string& filename);

    void setCurrentPanelFocus(PanelWidget* widget);

    AppInstancePtr openProjectInternal(const std::string & absoluteFileName, bool attemptToLoadAutosave) WARN_UNUSED_RETURN;

    void setupUi();

    void wipeLayout();

    void createDefaultLayoutInternal(bool wipePrevious);

    void createMenuActions();

    virtual void moveEvent(QMoveEvent* e) OVERRIDE FINAL;
    //virtual bool event(QEvent* e) OVERRIDE FINAL;
    virtual void resizeEvent(QResizeEvent* e) OVERRIDE FINAL;
    virtual void mouseMoveEvent(QMouseEvent* e) OVERRIDE FINAL;
    virtual void keyPressEvent(QKeyEvent* e) OVERRIDE FINAL;
    virtual void keyReleaseEvent(QKeyEvent* e) OVERRIDE FINAL;
    virtual void dragEnterEvent(QDragEnterEvent* e) OVERRIDE FINAL;
    virtual void dragMoveEvent(QDragMoveEvent* e) OVERRIDE FINAL;
    virtual void dragLeaveEvent(QDragLeaveEvent* e) OVERRIDE FINAL;
    virtual void dropEvent(QDropEvent* e) OVERRIDE FINAL;
    boost::scoped_ptr<GuiPrivate> _imp;
};

NATRON_NAMESPACE_EXIT

#endif // Gui_Gui_h
