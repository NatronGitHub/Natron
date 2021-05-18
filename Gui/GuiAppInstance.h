/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2021 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#ifndef GUIAPPINSTANCE_H
#define GUIAPPINSTANCE_H

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
#ifndef NATRON_ENABLE_IO_META_NODES
    std::map<std::string, NodePtr> readerNodes;
#else
    NodePtr readerNode;
#endif

    FileDialogPreviewProvider()
        : viewerUI(0)
        , viewerNodeInternal()
        , viewerNode()
#ifndef NATRON_ENABLE_IO_META_NODES
        , readerNodes()
#else
        , readerNode()
#endif
    {}
};


struct GuiAppInstancePrivate;
class GuiAppInstance
    : public AppInstance
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    GuiAppInstance(int appID);

    virtual ~GuiAppInstance();

    void resetPreviewProvider();

    virtual bool isBackground() const OVERRIDE FINAL { return false; }

private:


    void deletePreviewProvider();


    /** @brief Attempts to find an untitled autosave. If found one, prompts the user
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
    virtual void loadProjectGui(bool isAutosave,  boost::archive::xml_iarchive & archive) const OVERRIDE FINAL;
    virtual void saveProjectGui(boost::archive::xml_oarchive & archive) OVERRIDE FINAL;
    virtual void notifyRenderStarted(const QString & sequenceName,
                                     int firstFrame, int lastFrame,
                                     int frameStep, bool canPause,
                                     OutputEffectInstance* writer,
                                     const ProcessHandlerPtr & process) OVERRIDE FINAL;
    virtual void notifyRenderRestarted( OutputEffectInstance* writer,
                                        const ProcessHandlerPtr & process) OVERRIDE FINAL;
    virtual void setupViewersForViews(const std::vector<std::string>& viewNames) OVERRIDE FINAL;

    void setViewersCurrentView(ViewIdx view);

    void setUndoRedoStackLimit(int limit);

    bool isClosing() const;

    virtual bool isGuiFrozen() const OVERRIDE FINAL;
    virtual bool isShowingDialog() const OVERRIDE FINAL;
    virtual void progressStart(const NodePtr& node, const std::string &message, const std::string &messageid, bool canCancel = true) OVERRIDE FINAL;
    virtual void progressEnd(const NodePtr& node) OVERRIDE FINAL;
    virtual bool progressUpdate(const NodePtr& node, double t) OVERRIDE FINAL;
    virtual void onMaxPanelsOpenedChanged(int maxPanels) OVERRIDE FINAL;
    virtual void onRenderQueuingChanged(bool queueingEnabled) OVERRIDE FINAL;
    virtual void connectViewersToViewerCache() OVERRIDE FINAL;
    virtual void disconnectViewersFromViewerCache() OVERRIDE FINAL;
    FileDialogPreviewProviderPtr getPreviewProvider() const;
    virtual std::string openImageFileDialog() OVERRIDE FINAL;
    virtual std::string saveImageFileDialog() OVERRIDE FINAL;
    virtual void clearViewersLastRenderedTexture() OVERRIDE FINAL;
    virtual void appendToScriptEditor(const std::string& str) OVERRIDE FINAL;
    virtual void printAutoDeclaredVariable(const std::string& str) OVERRIDE FINAL;
    virtual void toggleAutoHideGraphInputs() OVERRIDE FINAL;
    virtual void setLastViewerUsingTimeline(const NodePtr& node) OVERRIDE FINAL;
    virtual ViewerInstance* getLastViewerUsingTimeline() const OVERRIDE FINAL;

    void discardLastViewerUsingTimeline();


    virtual void declareCurrentAppVariable_Python() OVERRIDE;
    virtual void createLoadProjectSplashScreen(const QString& projectFile) OVERRIDE FINAL;
    virtual void updateProjectLoadStatus(const QString& str) OVERRIDE FINAL;
    virtual void closeLoadPRojectSplashScreen() OVERRIDE FINAL;
    virtual void renderAllViewers(bool canAbort) OVERRIDE FINAL;
    virtual void refreshAllPreviews() OVERRIDE FINAL;
    virtual void abortAllViewers() OVERRIDE FINAL;
    virtual void queueRedrawForAllViewers() OVERRIDE FINAL;

    int getOverlayRedrawRequestsCount() const;

    void clearOverlayRedrawRequests();

    void setKnobDnDData(QDrag* drag, const KnobIPtr& knob, int dimension);
    void getKnobDnDData(QDrag** drag,  KnobIPtr* knob, int* dimension) const;

    bool checkAllReadersModificationDate(bool errorAndWarn);

public Q_SLOTS:


    void reloadStylesheet();

    virtual void redrawAllViewers() OVERRIDE FINAL;

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

Q_SIGNALS:

    void keyframeIndicatorsChanged();

public:

    virtual void* getOfxHostOSHandle() const OVERRIDE FINAL;


    ///Rotopaint related
    virtual void updateLastPaintStrokeData(int newAge,
                                           const std::list<std::pair<Point, double> >& points,
                                           const RectD& lastPointsBbox,
                                           int strokeIndex) OVERRIDE FINAL;
    virtual void getLastPaintStrokePoints(std::list<std::list<std::pair<Point, double> > >* strokes, int* strokeIndex) const OVERRIDE FINAL;
    virtual void getRenderStrokeData(RectD* lastStrokeMovementBbox, std::list<std::pair<Point, double> >* lastStrokeMovementPoints,
                                     double *distNextIn, ImagePtr* strokeImage) const OVERRIDE FINAL;
    virtual int getStrokeLastIndex() const OVERRIDE FINAL;
    virtual void getStrokeAndMultiStrokeIndex(RotoStrokeItemPtr* stroke, int* strokeIndex) const OVERRIDE FINAL;
    virtual void updateStrokeImage(const ImagePtr& image, double distNextOut, bool setDistNextOut) OVERRIDE FINAL;
    virtual RectD getLastPaintStrokeBbox() const OVERRIDE FINAL;
    virtual RectD getPaintStrokeWholeBbox() const OVERRIDE FINAL;
    virtual void setUserIsPainting(const NodePtr& rotopaintNode,
                                   const RotoStrokeItemPtr& stroke,
                                   bool isPainting) OVERRIDE FINAL;
    virtual void getActiveRotoDrawingStroke(NodePtr* node,
                                            RotoStrokeItemPtr* stroke,
                                            bool* isPainting) const OVERRIDE FINAL;
    virtual void reloadScriptEditorFonts() OVERRIDE FINAL;
    ///////////////// OVERRIDDEN FROM TIMELINEKEYFRAMES
    virtual void removeAllKeyframesIndicators() OVERRIDE FINAL;
    virtual void addKeyframeIndicator(SequenceTime time) OVERRIDE FINAL;
    virtual void addMultipleKeyframeIndicatorsAdded(const std::list<SequenceTime> & keys, bool emitSignal) OVERRIDE FINAL;
    virtual void removeKeyFrameIndicator(SequenceTime time) OVERRIDE FINAL;
    virtual void removeMultipleKeyframeIndicator(const std::list<SequenceTime> & keys, bool emitSignal) OVERRIDE FINAL;
    virtual void addNodesKeyframesToTimeline(const std::list<Node*> & nodes) OVERRIDE FINAL;
    virtual void addNodeKeyframesToTimeline(Node* node) OVERRIDE FINAL;
    virtual void removeNodesKeyframesFromTimeline(const std::list<Node*> & nodes) OVERRIDE FINAL;
    virtual void removeNodeKeyframesFromTimeline(Node* node) OVERRIDE FINAL;
    virtual void getKeyframes(std::list<SequenceTime>* keys) const OVERRIDE FINAL;
    virtual void removeAllUserKeyframesIndicators() OVERRIDE FINAL;
    virtual void addUserKeyframeIndicator(SequenceTime time) OVERRIDE FINAL;
    virtual void addUserMultipleKeyframeIndicatorsAdded(const std::list<SequenceTime> & keys, bool emitSignal) OVERRIDE FINAL;
    virtual void removeUserKeyFrameIndicator(SequenceTime time) OVERRIDE FINAL;
    virtual void removeUserMultipleKeyframeIndicator(const std::list<SequenceTime> & keys, bool emitSignal) OVERRIDE FINAL;
    virtual void getUserKeyframes(std::list<SequenceTime>* keys) const OVERRIDE FINAL;
    ///////////////// END OVERRIDDEN FROM TIMELINEKEYFRAMES

private:

    virtual void onGroupCreationFinished(const NodePtr& node, const NodeSerializationPtr& serialization, bool autoConnect) OVERRIDE FINAL;
    virtual void createNodeGui(const NodePtr &node,
                               const NodePtr&  parentMultiInstance,
                               const CreateNodeArgs& args) OVERRIDE FINAL;
    boost::scoped_ptr<GuiAppInstancePrivate> _imp;
};

NATRON_NAMESPACE_EXIT

#endif // GUIAPPINSTANCE_H
