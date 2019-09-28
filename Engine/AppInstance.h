/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
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

#ifndef Engine_AppInstance_h
#define Engine_AppInstance_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <vector>
#include <list>
#include <map>
#include <string>
#include <utility>
#include <climits>              // for INT_MAX, INT_MIN
#include <cstddef>              // for NULL

#include <QtCore/QtGlobal>      // for Q_UNUSED
#include <QtCore/QObject>  // for Q_OBJECT, Q_SIGNALS, Q_SLOTS
#include <QtCore/QMutex>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/scoped_ptr.hpp>
#endif

#include "Global/GlobalDefines.h"
#include "Engine/RectD.h"
#include "Engine/TimeValue.h"

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

struct AppInstancePrivate;


class FlagSetter
{
    bool* p;
    QMutex* lock;

public:

    FlagSetter(bool initialValue,
               bool* p);

    FlagSetter(bool initialValue,
               bool* p,
               QMutex* mutex);

    ~FlagSetter();
};

class FlagIncrementer
{
    int* p;
    QMutex* lock;

public:

    FlagIncrementer(int* p);

    FlagIncrementer(int* p,
                    QMutex* mutex);

    ~FlagIncrementer();
};

class CreateNodeStackItem;
typedef boost::shared_ptr<CreateNodeStackItem> CreateNodeStackItemPtr;
typedef boost::weak_ptr<CreateNodeStackItem> CreateNodeStackItemWPtr;
typedef std::list<CreateNodeStackItemPtr> CreateNodeStackItemPtrList;



class AppInstance
    : public QObject
    , public boost::noncopyable
    , public boost::enable_shared_from_this<AppInstance>
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

protected: // parent of GuiAppInstance
    // constructors should be privatized in any class that derives from boost::enable_shared_from_this<>

    AppInstance(int appID);

public:
    static AppInstancePtr create(int appID) WARN_UNUSED_RETURN
    {
        return AppInstancePtr( new AppInstance(appID) );
    }

    virtual ~AppInstance();

    virtual void aboutToQuit();
    virtual bool isBackground() const { return true; }




    void load(const CLArgs& cl, bool makeEmptyInstance);

protected:

    virtual void loadInternal(const CLArgs& cl, bool makeEmptyInstance);

    void executeCommandLinePythonCommands(const CLArgs& args);

public:

    int getAppID() const;

    void exportDocs(const QString path);

    /** @brief Create a new node  in the node graph.
    **/
    NodePtr createNode(const CreateNodeArgsPtr& args);
    NodePtr createReader(const std::string& filename,
                         const CreateNodeArgsPtr& args);


    NodePtr createWriter(const std::string& filename,
                         const CreateNodeArgsPtr& args,
                         int firstFrame = INT_MIN, int lastFrame = INT_MAX);

    NodePtr getNodeByFullySpecifiedName(const std::string & name) const;

    ProjectPtr getProject() const;
    TimeLinePtr getTimeLine() const;

    RenderQueuePtr getRenderQueue() const;

    /*true if the user is NOT scrubbing the timeline*/
    virtual bool shouldRefreshPreview() const
    {
        return false;
    }


    virtual void errorDialog(const std::string & title, const std::string & message, bool useHtml) const;
    virtual void errorDialog(const std::string & title, const std::string & message, bool* stopAsking, bool useHtml) const;
    virtual void warningDialog(const std::string & title, const std::string & message, bool useHtml) const;
    virtual void warningDialog(const std::string & title, const std::string & message, bool* stopAsking, bool useHtml) const;
    virtual void informationDialog(const std::string & title, const std::string & message, bool useHtml) const;
    virtual void informationDialog(const std::string & title, const std::string & message, bool* stopAsking, bool useHtml) const;
    virtual StandardButtonEnum questionDialog(const std::string & title,
                                              const std::string & message,
                                              bool useHtml,
                                              StandardButtons buttons =
                                                  StandardButtons(eStandardButtonYes | eStandardButtonNo),
                                              StandardButtonEnum defaultButton = eStandardButtonNoButton) const WARN_UNUSED_RETURN;

    /**
     * @brief Asks a question to the user and returns the reply.
     * @param stopAsking Set to true if the user do not want Natron to ask the question again.
     **/
    virtual StandardButtonEnum questionDialog(const std::string & /*title*/,
                                              const std::string & /*message*/,
                                              bool /*useHtml*/,
                                              StandardButtons /*buttons*/,
                                              StandardButtonEnum /*defaultButton*/,
                                              bool* /*stopAsking*/)
    {
        return eStandardButtonYes;
    }

    virtual void loadProjectGui(bool /*isAutosave*/, const SERIALIZATION_NAMESPACE::ProjectSerializationPtr& /*serialization*/) const
    {
    }

    virtual void setupViewersForViews(const std::vector<std::string>& /*viewNames*/)
    {
    }

    virtual void notifyRenderStarted(const QString & /*sequenceName*/,
                                     TimeValue /*firstFrame*/,
                                     TimeValue /*lastFrame*/,
                                     TimeValue /*frameStep*/,
                                     bool /*canPause*/,
                                     const NodePtr& /*writer*/,
                                     const ProcessHandlerPtr & /*process*/)
    {
    }

    virtual void notifyRenderRestarted( const NodePtr& /*writer*/,
                                        const ProcessHandlerPtr & /*process*/)
    {
    }

    virtual bool isShowingDialog() const
    {
        return false;
    }

    virtual void setGuiFrozen(bool frozen) { Q_UNUSED(frozen); }

    virtual bool isGuiFrozen() const { return false; }

    virtual void refreshAllTimeEvaluationParams(bool /*onlyTimeEvaluationKnobs*/) {}

    virtual void progressStart(const NodePtr& node,
                               const std::string &message,
                               const std::string &messageid,
                               bool canCancel = true)
    {
        Q_UNUSED(node);
        Q_UNUSED(message);
        Q_UNUSED(messageid);
        Q_UNUSED(canCancel);
    }

    virtual void progressEnd(const NodePtr& /*node*/)
    {
    }

    virtual bool progressUpdate(const NodePtr& /*node*/,
                                double /*t*/)
    {
        return true;
    }

    virtual void showRenderStatsWindow() {}

    /**
     * @brief Checks for a new version of Natron
     **/
    void checkForNewVersion() const;
    virtual void onMaxPanelsOpenedChanged(int /*maxPanels*/)
    {
    }

    virtual void onRenderQueuingChanged(bool /*queueingEnabled*/)
    {
    }

    virtual void goToPreviousKeyframe()
    {
    }
    virtual void goToNextKeyframe()
    {
    }

    virtual void setMasterSyncViewer(const NodePtr& viewerNode) { Q_UNUSED(viewerNode); }

    virtual NodePtr getMasterSyncViewer() const { return NodePtr(); }

    ViewerColorSpaceEnum getDefaultColorSpaceForBitDepth(ImageBitDepthEnum bitdepth) const;

    double getProjectFrameRate() const;

    virtual std::string openImageFileDialog() { return std::string(); }

    virtual std::string saveImageFileDialog() { return std::string(); }

    virtual bool checkAllReadersModificationDate(bool /*errorAndWarn*/) { return true; }

    void onOCIOConfigPathChanged(const std::string& path);

public:

    void addInvalidExpressionKnob(const KnobIPtr& knob);
    void removeInvalidExpressionKnob(const KnobIConstPtr& knob);
    void recheckInvalidLinks();

    virtual void toggleAutoHideGraphInputs() {}


    /**
     * @brief Returns true if the project is currently in the process of creating a node
     **/
    bool isCreatingNode() const;


    virtual void appendToScriptEditor(const std::string& str);
    virtual void printAutoDeclaredVariable(const std::string& str);

    virtual void setLastViewerUsingTimeline(const NodePtr& /*node*/) {}

    virtual ViewerNodePtr getLastViewerUsingTimeline() const { return ViewerNodePtr(); }

    bool loadPythonScript(const QFileInfo& file);

    bool loadPythonScriptAndReportToScriptEditor(const QString& script);

    virtual void getAllViewers(std::list<ViewerNodePtr>* viewers) const { Q_UNUSED(viewers); }

    virtual void renderAllViewers() {}

    virtual void abortAllViewers(bool /*autoRestartPlayback*/) {}

    virtual void refreshAllPreviews() {}

    virtual void getViewersOpenGLContextFormat(int* bitdepthPerComponent, bool *hasAlpha) const { *bitdepthPerComponent = 0; *hasAlpha = false;}

    virtual void declareCurrentAppVariable_Python();

    void execOnProjectCreatedCallback();

    virtual void createLoadProjectSplashScreen(const QString& /*projectFile*/) {}

    virtual void updateProjectLoadStatus(const QString& str);

    virtual void closeLoadPRojectSplashScreen() {}

    std::string getAppIDString() const;

    bool isTopLevelNodeBeingCreated(const NodePtr& node) const;

    bool isDuringPythonPyPlugCreation() const;

    bool isDuringPyPlugCreation() const;

    virtual bool isDraftRenderEnabled() const { return false; }

    virtual void setDraftRenderEnabled(bool /*b*/) {}

    virtual void setUserIsPainting(const RotoStrokeItemPtr& /*stroke*/) {}

    virtual RotoStrokeItemPtr getActiveRotoDrawingStroke() const { return RotoStrokeItemPtr(); }

    virtual bool isRenderStatsActionChecked() const { return false; }

    bool saveTemp(const std::string& filename);
    virtual bool save(const std::string& filename);
    virtual bool saveAs(const std::string& filename);
    virtual AppInstancePtr loadProject(const std::string& filename);

    ///Close the current project but keep the window
    virtual bool resetProject();

    ///Reset + close window, quit if last window
    virtual bool closeProject();

    ///Opens a new window
    virtual AppInstancePtr newProject();
    virtual void* getOfxHostOSHandle() const { return NULL; }

    virtual void createGroupGui(const NodePtr & /*group*/, const CreateNodeArgs& /*args*/) {}


    virtual void reloadScriptEditorFonts() {}

    SerializableWindow* getMainWindowSerialization() const;

    std::list<SerializableWindow*> getFloatingWindowsSerialization() const;

    std::list<SplitterI*> getSplittersSerialization() const;

    std::list<TabWidgetI*> getTabWidgetsSerialization() const;

    std::list<PyPanelI*> getPyPanelsSerialization() const;

    std::list<DockablePanelI*> getOpenedSettingsPanels() const;

    void setOpenedSettingsPanelsInternal(const std::list<DockablePanelI*>& panels);

    void registerFloatingWindow(SerializableWindow* window);
    void unregisterFloatingWindow(SerializableWindow* window);
    void clearFloatingWindows();


    void registerSplitter(SplitterI* splitter);
    void unregisterSplitter(SplitterI* splitter);
    void clearSplitters();

    void registerTabWidget(TabWidgetI* tabWidget);
    void unregisterTabWidget(TabWidgetI* tabWidget);
    void clearTabWidgets();

    void registerPyPanel(PyPanelI* panel, const std::string& pythonFunction);
    void unregisterPyPanel(PyPanelI* panel);

    void registerSettingsPanel(DockablePanelI* panel, int index = -1);
    void unregisterSettingsPanel(DockablePanelI* panel);
    void clearSettingsPanels();

    /**
    * @brief If baseName is already used by another pane or it is empty,this function will return a new pane name that is not already
    * used by another pane. Otherwise it will return baseName.
    **/
    QString getAvailablePaneName( const QString & baseName = QString() ) const;

    virtual void getHistogramScriptNames(std::list<std::string>* /*histograms*/) const {}

    virtual void getViewportsProjection(std::map<std::string,SERIALIZATION_NAMESPACE::ViewportData>* /*projections*/) const {}

    void saveApplicationWorkspace(SERIALIZATION_NAMESPACE::WorkspaceSerialization* serialization);


public Q_SLOTS:

    void quit();

    void quitNow();

    virtual void redrawAllViewers() {}

    virtual void redrawAllTimelines() {}

    void triggerAutoSave();

    void clearOpenFXPluginsCaches();

    void clearAllLastRenderedImages();

    void newVersionCheckDownloaded();

    void newVersionCheckError();




Q_SIGNALS:

    void pluginsPopulated();

protected:

    virtual void onTabWidgetRegistered(TabWidgetI* tabWidget) { Q_UNUSED(tabWidget); }

    virtual void onTabWidgetUnregistered(TabWidgetI* tabWidget) { Q_UNUSED(tabWidget); }

    virtual void createMainWindow() { }

    void setMainWindowPointer(SerializableWindow* window);

    virtual void onNodeAboutToBeCreated(const NodePtr& node, const CreateNodeArgsPtr& args);

    virtual void onNodeCreated(const NodePtr& node, const CreateNodeArgsPtr& args);

private:



    bool openFileDialogIfNeeded(const CreateNodeArgsPtr& args, bool openFile);

    NodePtr createNodeInternal(const CreateNodeArgsPtr& args);


    NodePtr createNodeFromPyPlug(const PluginPtr& plugin, const CreateNodeArgsPtr& args);

    friend class AddCreateNode_RAII;
    boost::scoped_ptr<AppInstancePrivate> _imp;
};

class AddCreateNode_RAII
{
    AppInstancePrivate* _imp;
    CreateNodeStackItemPtr _item;

public:


    AddCreateNode_RAII(const AppInstancePtr& app,
                       const NodePtr& node,
                       const CreateNodeArgsPtr& args);

    ~AddCreateNode_RAII();
};

NATRON_NAMESPACE_EXIT

#endif // Engine_AppInstance_h
