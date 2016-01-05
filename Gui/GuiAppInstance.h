/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
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

#include "Engine/AppInstance.h"

#include "Gui/GuiFwd.h"


/**
 * @brief This little struct contains what enables file dialogs to show previews.
 * It is shared by all dialogs so that we don't have to recreate the nodes everytimes
 **/
class FileDialogPreviewProvider
{
public:
    ViewerTab* viewerUI;
    boost::shared_ptr<Natron::Node> viewerNodeInternal;
    boost::shared_ptr<NodeGui> viewerNode;
    std::map<std::string,std::pair< boost::shared_ptr<Natron::Node>, boost::shared_ptr<NodeGui> > > readerNodes;
    
    FileDialogPreviewProvider()
    : viewerUI(0)
    , viewerNodeInternal()
    , viewerNode()
    , readerNodes()
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
    
    
    /** @brief Attemps to find an untitled autosave. If found one, prompts the user
     * whether he/she wants to load it. If something was loaded this function
     * returns true,otherwise false.
     **/
    bool findAndTryLoadUntitledAutoSave() WARN_UNUSED_RETURN;
    
public:
    
    virtual void aboutToQuit() OVERRIDE FINAL;
    virtual void load(const CLArgs& cl,bool makeEmptyInstance) OVERRIDE FINAL;
    
    Gui* getGui() const WARN_UNUSED_RETURN;

    /**
     * @brief Remove the node n from the mapping in GuiAppInstance and from the project so the pointer is no longer
     * referenced anywhere. This function is called on nodes that were already deleted by the user but were kept into
     * the undo/redo stack. That means this node is no longer references by any other node and can be safely deleted.
     * The first thing this function does is to assert that the node n is not active.
     **/
    void deleteNode(const boost::shared_ptr<NodeGui> & n);
    //////////

    virtual bool shouldRefreshPreview() const OVERRIDE FINAL;
    virtual void errorDialog(const std::string & title,const std::string & message, bool useHtml) const OVERRIDE FINAL;
    virtual void errorDialog(const std::string & title,const std::string & message,bool* stopAsking,bool useHtml) const OVERRIDE FINAL;
    virtual void warningDialog(const std::string & title,const std::string & message,bool useHtml) const OVERRIDE FINAL;
    virtual void warningDialog(const std::string & title,const std::string & message,bool* stopAsking,bool useHtml) const OVERRIDE FINAL;
    virtual void informationDialog(const std::string & title,const std::string & message,bool useHtml) const OVERRIDE FINAL;
    virtual void informationDialog(const std::string & title,const std::string & message,bool* stopAsking,bool useHtml) const OVERRIDE FINAL;
    virtual Natron::StandardButtonEnum questionDialog(const std::string & title,
                                                      const std::string & message,
                                                      bool useHtml,
                                                      Natron::StandardButtons buttons = Natron::StandardButtons(Natron::eStandardButtonYes | Natron::eStandardButtonNo),
                                                      Natron::StandardButtonEnum defaultButton = Natron::eStandardButtonNoButton) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    
    virtual Natron::StandardButtonEnum questionDialog(const std::string & title,
                                                      const std::string & message,
                                                      bool useHtml,
                                                      Natron::StandardButtons buttons,
                                                      Natron::StandardButtonEnum defaultButton,
                                                      bool* stopAsking) OVERRIDE FINAL WARN_UNUSED_RETURN;
    
    virtual void loadProjectGui(boost::archive::xml_iarchive & archive) const OVERRIDE FINAL;
    virtual void saveProjectGui(boost::archive::xml_oarchive & archive) OVERRIDE FINAL;
    virtual void notifyRenderProcessHandlerStarted(const QString & sequenceName,
                                                   int firstFrame,int lastFrame,
                                                   int frameStep,
                                                   const boost::shared_ptr<ProcessHandler> & process) OVERRIDE FINAL;
    virtual void setupViewersForViews(const std::vector<std::string>& viewNames) OVERRIDE FINAL;

    void setViewersCurrentView(int view);

    void setUndoRedoStackLimit(int limit);

    bool isClosing() const;
    
    virtual bool isGuiFrozen() const OVERRIDE FINAL;

    virtual bool isShowingDialog() const OVERRIDE FINAL;
    virtual void progressStart(KnobHolder* effect, const std::string &message, const std::string &messageid, bool canCancel = true) OVERRIDE FINAL;
    virtual void progressEnd(KnobHolder* effect) OVERRIDE FINAL;
    virtual bool progressUpdate(KnobHolder* effect,double t) OVERRIDE FINAL;
    virtual void onMaxPanelsOpenedChanged(int maxPanels) OVERRIDE FINAL;
    virtual void connectViewersToViewerCache() OVERRIDE FINAL;
    virtual void disconnectViewersFromViewerCache() OVERRIDE FINAL;


    boost::shared_ptr<FileDialogPreviewProvider> getPreviewProvider() const;

    virtual std::string openImageFileDialog() OVERRIDE FINAL;
    virtual std::string saveImageFileDialog() OVERRIDE FINAL;

    virtual void startRenderingFullSequence(bool enableRenderStats,const AppInstance::RenderWork& w,bool renderInSeparateProcess,const QString& savePath) OVERRIDE FINAL;

    virtual void clearViewersLastRenderedTexture() OVERRIDE FINAL;
    
    virtual void appendToScriptEditor(const std::string& str) OVERRIDE FINAL;
    
    virtual void printAutoDeclaredVariable(const std::string& str) OVERRIDE FINAL;
    
    virtual void toggleAutoHideGraphInputs() OVERRIDE FINAL;
    virtual void setLastViewerUsingTimeline(const boost::shared_ptr<Natron::Node>& node) OVERRIDE FINAL;
    
    virtual ViewerInstance* getLastViewerUsingTimeline() const OVERRIDE FINAL;
    
    void discardLastViewerUsingTimeline();
    

    virtual void declareCurrentAppVariable_Python() OVERRIDE;

    virtual void createLoadProjectSplashScreen(const QString& projectFile) OVERRIDE FINAL;
    
    virtual void updateProjectLoadStatus(const QString& str) OVERRIDE FINAL;
    
    virtual void closeLoadPRojectSplashScreen() OVERRIDE FINAL;
    

    virtual void renderAllViewers(bool canAbort) OVERRIDE FINAL;
    
    
    virtual void queueRedrawForAllViewers() OVERRIDE FINAL;
    
    int getOverlayRedrawRequestsCount() const;
    
    void clearOverlayRedrawRequests();
    
public Q_SLOTS:
    

    void reloadStylesheet();

    virtual void redrawAllViewers() OVERRIDE FINAL;

    void onProcessFinished();

    void projectFormatChanged(const Format& f);
    
    virtual bool isDraftRenderEnabled() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    
    
    virtual bool isRenderStatsActionChecked() const OVERRIDE FINAL;
    
    virtual bool save(const std::string& filename) OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool saveAs(const std::string& filename) OVERRIDE FINAL WARN_UNUSED_RETURN;
    
    virtual AppInstance* loadProject(const std::string& filename) OVERRIDE FINAL WARN_UNUSED_RETURN;
    
    ///Close the current project but keep the window
    virtual bool resetProject()  OVERRIDE FINAL;
    
    ///Reset + close window, quit if last window
    virtual bool closeProject()  OVERRIDE FINAL;
    
    ///Opens a new window
    virtual AppInstance* newProject()  OVERRIDE FINAL;
    
    void handleFileOpenEvent(const std::string& filename);
    
    virtual void goToPreviousKeyframe() OVERRIDE FINAL;
    
    virtual void goToNextKeyframe() OVERRIDE FINAL;
    
Q_SIGNALS:
    
    void keyframeIndicatorsChanged();
    
public:

    virtual void* getOfxHostOSHandle() const OVERRIDE FINAL;
    
    
    ///Rotopaint related
    virtual void updateLastPaintStrokeData(int newAge,
                                           const std::list<std::pair<Natron::Point,double> >& points,
                                           const RectD& lastPointsBbox,
                                           int strokeIndex) OVERRIDE FINAL;
    
    virtual void getLastPaintStrokePoints(std::list<std::list<std::pair<Natron::Point,double> > >* strokes, int* strokeIndex) const OVERRIDE FINAL;
    
    virtual void getRenderStrokeData(RectD* lastStrokeMovementBbox, std::list<std::pair<Natron::Point,double> >* lastStrokeMovementPoints,
                                     double *distNextIn, boost::shared_ptr<Natron::Image>* strokeImage) const OVERRIDE FINAL;
    
    virtual int getStrokeLastIndex() const OVERRIDE FINAL;
    
    virtual void updateStrokeImage(const boost::shared_ptr<Natron::Image>& image, double distNextOut, bool setDistNextOut) OVERRIDE FINAL;

    virtual RectD getLastPaintStrokeBbox() const OVERRIDE FINAL;
    
    virtual RectD getPaintStrokeWholeBbox() const OVERRIDE FINAL;
    
    virtual void setUserIsPainting(const boost::shared_ptr<Natron::Node>& rotopaintNode,
                                   const boost::shared_ptr<RotoStrokeItem>& stroke,
                                   bool isPainting) OVERRIDE FINAL;
    virtual void getActiveRotoDrawingStroke(boost::shared_ptr<Natron::Node>* node,
                                            boost::shared_ptr<RotoStrokeItem>* stroke,
                                            bool* isPainting) const OVERRIDE FINAL;
    
    
    ///////////////// OVERRIDEN FROM TIMELINEKEYFRAMES
    virtual void removeAllKeyframesIndicators() OVERRIDE FINAL;
    
    virtual void addKeyframeIndicator(SequenceTime time) OVERRIDE FINAL;
    
    virtual void addMultipleKeyframeIndicatorsAdded(const std::list<SequenceTime> & keys,bool emitSignal) OVERRIDE FINAL;
    
    virtual void removeKeyFrameIndicator(SequenceTime time) OVERRIDE FINAL;
    
    virtual void removeMultipleKeyframeIndicator(const std::list<SequenceTime> & keys,bool emitSignal) OVERRIDE FINAL;
    
    virtual void addNodesKeyframesToTimeline(const std::list<Natron::Node*> & nodes) OVERRIDE FINAL;
    
    virtual void addNodeKeyframesToTimeline(Natron::Node* node) OVERRIDE FINAL;
    
    virtual void removeNodesKeyframesFromTimeline(const std::list<Natron::Node*> & nodes) OVERRIDE FINAL;
    
    virtual void removeNodeKeyframesFromTimeline(Natron::Node* node) OVERRIDE FINAL;

    virtual void getKeyframes(std::list<SequenceTime>* keys) const OVERRIDE FINAL;
    
   
    ///////////////// END OVERRIDEN FROM TIMELINEKEYFRAMES
    
private:
    
    virtual void onGroupCreationFinished(const boost::shared_ptr<Natron::Node>& node,bool requestedByLoad,bool userEdited) OVERRIDE FINAL;
    
    virtual void createNodeGui(const boost::shared_ptr<Natron::Node> &node,
                               const boost::shared_ptr<Natron::Node>&  parentMultiInstance,
                               bool loadRequest,
                               bool autoConnect,
                               bool userEdited,
                               double xPosHint,double yPosHint,
                               bool pushUndoRedoCommand) OVERRIDE FINAL;
    

    boost::scoped_ptr<GuiAppInstancePrivate> _imp;
};

#endif // GUIAPPINSTANCE_H
