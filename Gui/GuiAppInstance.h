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

#ifndef Gui_GuiAppInstance_h
#define Gui_GuiAppInstance_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <map>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#endif

#include "Engine/AppInstance.h"
#include "Engine/DimensionIdx.h"
#include "Engine/ViewIdx.h"

#include "Gui/GuiFwd.h"

class QDrag;

NATRON_NAMESPACE_ENTER

/**
 * @brief This little struct contains what enables file dialogs to show previews.
 * It is shared by all dialogs so that we don't have to recreate the nodes everytimes
 **/
class FileDialogPreviewProvider
{
public:
    ViewerTab* viewerUI;
    NodePtr viewerNodeInternal;
    NodeGuiPtr viewerNode;
    NodePtr readerNode;

    FileDialogPreviewProvider()
        : viewerUI(0)
        , viewerNodeInternal()
        , viewerNode()
        , readerNode()
    {}
};


struct GuiAppInstancePrivate;
class GuiAppInstance
    : public AppInstance // derives from boost::enable_shared_from_this<>
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

private:
    // constructors should be privatized in any class that derives from boost::enable_shared_from_this<>

    GuiAppInstance(int appID);

public:
    static AppInstancePtr create(int appID) WARN_UNUSED_RETURN
    {
        return AppInstancePtr( new GuiAppInstance(appID) );
    }

    virtual ~GuiAppInstance();

    void resetPreviewProvider();

    virtual bool isBackground() const OVERRIDE FINAL { return false; }

private:


    void deletePreviewProvider();


    /** @brief Attemps to find an untitled autosave. If found one, prompts the user
     * whether he/she wants to load it. If something was loaded this function
     * returns true,otherwise false.
     **/
    bool findAndTryLoadUntitledAutoSave() WARN_UNUSED_RETURN;

public:

    virtual void aboutToQuit() OVERRIDE FINAL;

private:

    virtual void loadInternal(const CLArgs& cl, bool makeEmptyInstance) OVERRIDE FINAL;

public:


    Gui* getGui() const WARN_UNUSED_RETURN;


    //////////
    virtual bool shouldRefreshPreview() const OVERRIDE FINAL;
    virtual void errorDialog(const std::string & title, const std::string & message, bool useHtml) const OVERRIDE FINAL;
    virtual void errorDialog(const std::string & title, const std::string & message, bool* stopAsking, bool useHtml) const OVERRIDE FINAL;
    virtual void warningDialog(const std::string & title, const std::string & message, bool useHtml) const OVERRIDE FINAL;
    virtual void warningDialog(const std::string & title, const std::string & message, bool* stopAsking, bool useHtml) const OVERRIDE FINAL;
    virtual void informationDialog(const std::string & title, const std::string & message, bool useHtml) const OVERRIDE FINAL;
    virtual void informationDialog(const std::string & title, const std::string & message, bool* stopAsking, bool useHtml) const OVERRIDE FINAL;
    virtual StandardButtonEnum questionDialog(const std::string & title,
                                              const std::string & message,
                                              bool useHtml,
                                              StandardButtons buttons = StandardButtons(eStandardButtonYes | eStandardButtonNo),
                                              StandardButtonEnum defaultButton = eStandardButtonNoButton) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual StandardButtonEnum questionDialog(const std::string & title,
                                              const std::string & message,
                                              bool useHtml,
                                              StandardButtons buttons,
                                              StandardButtonEnum defaultButton,
                                              bool* stopAsking) OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void loadProjectGui(bool isAutosave,  const SERIALIZATION_NAMESPACE::ProjectSerializationPtr& serialization) const OVERRIDE FINAL;
    virtual void notifyRenderStarted(const QString & sequenceName,
                                     TimeValue firstFrame, TimeValue lastFrame,
                                     TimeValue frameStep, bool canPause,
                                     const NodePtr& writer,
                                     const ProcessHandlerPtr & process) OVERRIDE FINAL;
    virtual void notifyRenderRestarted( const NodePtr& writer,
                                        const ProcessHandlerPtr & process) OVERRIDE FINAL;
    virtual void setupViewersForViews(const std::vector<std::string>& viewNames) OVERRIDE FINAL;

    void setViewersCurrentView(ViewIdx view);

    void setUndoRedoStackLimit(int limit);

    bool isClosing() const;

    virtual void setGuiFrozen(bool frozen) OVERRIDE FINAL;
    virtual bool isGuiFrozen() const OVERRIDE FINAL;
    virtual bool isShowingDialog() const OVERRIDE FINAL;
    virtual void refreshAllTimeEvaluationParams(bool onlyTimeEvaluationKnobs) OVERRIDE FINAL;
    virtual void progressStart(const NodePtr& node, const std::string &message, const std::string &messageid, bool canCancel = true) OVERRIDE FINAL;
    virtual void progressEnd(const NodePtr& node) OVERRIDE FINAL;
    virtual bool progressUpdate(const NodePtr& node, double t) OVERRIDE FINAL;
    virtual void onMaxPanelsOpenedChanged(int maxPanels) OVERRIDE FINAL;
    virtual void onRenderQueuingChanged(bool queueingEnabled) OVERRIDE FINAL;
    boost::shared_ptr<FileDialogPreviewProvider> getPreviewProvider() const;
    virtual std::string openImageFileDialog() OVERRIDE FINAL;
    virtual std::string saveImageFileDialog() OVERRIDE FINAL;
    virtual void appendToScriptEditor(const std::string& str) OVERRIDE FINAL;
    virtual void printAutoDeclaredVariable(const std::string& str) OVERRIDE FINAL;
    virtual void toggleAutoHideGraphInputs() OVERRIDE FINAL;
    virtual void setLastViewerUsingTimeline(const NodePtr& node) OVERRIDE FINAL;
    virtual ViewerNodePtr getLastViewerUsingTimeline() const OVERRIDE FINAL;

    void discardLastViewerUsingTimeline();


    virtual void declareCurrentAppVariable_Python() OVERRIDE;
    virtual void createLoadProjectSplashScreen(const QString& projectFile) OVERRIDE FINAL;
    virtual void updateProjectLoadStatus(const QString& str) OVERRIDE FINAL;
    virtual void closeLoadPRojectSplashScreen() OVERRIDE FINAL;
    virtual void getAllViewers(std::list<ViewerNodePtr>* viewers) const OVERRIDE FINAL;
    virtual void renderAllViewers() OVERRIDE FINAL;
    virtual void refreshAllPreviews() OVERRIDE FINAL;
    virtual void getViewersOpenGLContextFormat(int* bitdepthPerComponent, bool *hasAlpha) const OVERRIDE FINAL;
    virtual void abortAllViewers(bool autoRestartPlayback) OVERRIDE FINAL;

    void setKnobDnDData(QDrag* drag, const KnobIPtr& knob, DimSpec dimension, ViewSetSpec view);
    void getKnobDnDData(QDrag** drag,  KnobIPtr* knob, DimSpec* dimension, ViewSetSpec* view) const;

    virtual bool checkAllReadersModificationDate(bool errorAndWarn) OVERRIDE FINAL;

    virtual void setMasterSyncViewer(const NodePtr& viewerNode) OVERRIDE FINAL;

    virtual NodePtr getMasterSyncViewer() const OVERRIDE FINAL WARN_UNUSED_RETURN;

    virtual void showRenderStatsWindow() OVERRIDE FINAL;

    virtual void getHistogramScriptNames(std::list<std::string>* histograms) const OVERRIDE FINAL;

    virtual void getViewportsProjection(std::map<std::string,SERIALIZATION_NAMESPACE::ViewportData>* projections) const OVERRIDE FINAL;

public Q_SLOTS:


    void reloadStylesheet();

    virtual void redrawAllViewers() OVERRIDE FINAL;

    virtual void redrawAllTimelines() OVERRIDE FINAL;

    void projectFormatChanged(const Format& f);

    virtual bool isDraftRenderEnabled() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void setDraftRenderEnabled(bool b) OVERRIDE FINAL;
    virtual bool isRenderStatsActionChecked() const OVERRIDE FINAL;
    virtual bool save(const std::string& filename) OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool saveAs(const std::string& filename) OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual AppInstancePtr loadProject(const std::string& filename) OVERRIDE FINAL WARN_UNUSED_RETURN;

    ///Close the current project but keep the window
    virtual bool resetProject()  OVERRIDE FINAL;

    ///Reset + close window, quit if last window
    virtual bool closeProject()  OVERRIDE FINAL;

    ///Opens a new window
    virtual AppInstancePtr newProject()  OVERRIDE FINAL;

    void handleFileOpenEvent(const std::string& filename);

    virtual void goToPreviousKeyframe() OVERRIDE FINAL;
    virtual void goToNextKeyframe() OVERRIDE FINAL;


public:

    virtual void* getOfxHostOSHandle() const OVERRIDE FINAL;

    virtual void setUserIsPainting(const RotoStrokeItemPtr& stroke) OVERRIDE FINAL;
    virtual RotoStrokeItemPtr getActiveRotoDrawingStroke() const OVERRIDE FINAL;

    virtual void reloadScriptEditorFonts() OVERRIDE FINAL;

    virtual void createGroupGui(const NodePtr & group, const CreateNodeArgs& args) OVERRIDE FINAL;

private:

    virtual void onNodeAboutToBeCreated(const NodePtr& node, const CreateNodeArgsPtr& args) OVERRIDE FINAL;

    virtual void onNodeCreated(const NodePtr& node, const CreateNodeArgsPtr& args) OVERRIDE FINAL;

    virtual void onTabWidgetRegistered(TabWidgetI* tabWidget) OVERRIDE FINAL;

    virtual void onTabWidgetUnregistered(TabWidgetI* tabWidget) OVERRIDE FINAL;

    virtual void createMainWindow() OVERRIDE FINAL;

    boost::scoped_ptr<GuiAppInstancePrivate> _imp;
};


inline GuiAppInstancePtr
toGuiAppInstance(const AppInstancePtr& instance)
{
    return boost::dynamic_pointer_cast<GuiAppInstance>(instance);
}


NATRON_NAMESPACE_EXIT

#endif // Gui_GuiAppInstance_h
