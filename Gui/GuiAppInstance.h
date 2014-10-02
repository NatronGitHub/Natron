//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#ifndef GUIAPPINSTANCE_H
#define GUIAPPINSTANCE_H

#include <map>

#include "Engine/AppInstance.h"

#include "Global/Macros.h"

class NodeGui;

class Gui;
class ViewerTab;
class Format;

/**
 * @brief This little struct contains what enables file dialogs to show previews.
 * It is shared by all dialogs so that we don't have to recreate the nodes everytimes
 **/
struct FileDialogPreviewProvider
{
    ViewerTab* viewerUI;
    boost::shared_ptr<NodeGui> viewerNode;
    std::map<std::string,boost::shared_ptr<NodeGui> > readerNodes;
    
    FileDialogPreviewProvider()
    : viewerUI(0)
    , viewerNode()
    , readerNodes()
    {}
};


struct GuiAppInstancePrivate;
class GuiAppInstance
    : public AppInstance
{
    Q_OBJECT

public:

    GuiAppInstance(int appID);

    virtual ~GuiAppInstance();

    virtual void aboutToQuit() OVERRIDE FINAL;
    virtual void load(const QString & projectName = QString(),
                      const std::list<RenderRequest> &writersWork = std::list<AppInstance::RenderRequest>()) OVERRIDE FINAL;
    Gui* getGui() const WARN_UNUSED_RETURN;

    //////////
    boost::shared_ptr<NodeGui> getNodeGui(const boost::shared_ptr<Natron::Node> & n) const WARN_UNUSED_RETURN;
    boost::shared_ptr<NodeGui> getNodeGui(const std::string & nodeName) const WARN_UNUSED_RETURN;
    boost::shared_ptr<Natron::Node> getNode(const boost::shared_ptr<NodeGui> & n) const WARN_UNUSED_RETURN;

    /**
     * @brief Remove the node n from the mapping in GuiAppInstance and from the project so the pointer is no longer
     * referenced anywhere. This function is called on nodes that were already deleted by the user but were kept into
     * the undo/redo stack. That means this node is no longer references by any other node and can be safely deleted.
     * The first thing this function does is to assert that the node n is not active.
     **/
    void deleteNode(const boost::shared_ptr<NodeGui> & n);
    //////////

    virtual bool shouldRefreshPreview() const OVERRIDE FINAL;
    virtual void errorDialog(const std::string & title,const std::string & message) const OVERRIDE FINAL;
    virtual void warningDialog(const std::string & title,const std::string & message) const OVERRIDE FINAL;
    virtual void informationDialog(const std::string & title,const std::string & message) const OVERRIDE FINAL;
    virtual Natron::StandardButton questionDialog(const std::string & title,const std::string & message,Natron::StandardButtons buttons =
                                                      Natron::StandardButtons(Natron::Yes | Natron::No),
                                                  Natron::StandardButton defaultButton = Natron::NoButton) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void loadProjectGui(boost::archive::xml_iarchive & archive) const OVERRIDE FINAL;
    virtual void saveProjectGui(boost::archive::xml_oarchive & archive) OVERRIDE FINAL;
    virtual void notifyRenderProcessHandlerStarted(const QString & sequenceName,
                                                   int firstFrame,int lastFrame,
                                                   const boost::shared_ptr<ProcessHandler> & process) OVERRIDE FINAL;
    virtual void setupViewersForViews(int viewsCount) OVERRIDE FINAL;

    void setViewersCurrentView(int view);

    void setUndoRedoStackLimit(int limit);

    bool isClosing() const;
    
    virtual bool isGuiFrozen() const OVERRIDE FINAL;

    virtual bool isShowingDialog() const OVERRIDE FINAL;
    virtual void startProgress(Natron::EffectInstance* effect,const std::string & message) OVERRIDE FINAL;
    virtual void endProgress(Natron::EffectInstance* effect) OVERRIDE FINAL;
    virtual bool progressUpdate(Natron::EffectInstance* effect,double t) OVERRIDE FINAL;
    virtual void onMaxPanelsOpenedChanged(int maxPanels) OVERRIDE FINAL;
    virtual void connectViewersToViewerCache() OVERRIDE FINAL;
    virtual void disconnectViewersFromViewerCache() OVERRIDE FINAL;
    virtual void clearNodeGuiMapping() OVERRIDE FINAL;
    virtual void aboutToAutoSave() OVERRIDE FINAL;
    virtual void autoSaveFinished() OVERRIDE FINAL;

    boost::shared_ptr<FileDialogPreviewProvider> getPreviewProvider() const;

    virtual std::string openImageFileDialog() OVERRIDE FINAL;
    virtual std::string saveImageFileDialog() OVERRIDE FINAL;

    virtual void startRenderingFullSequence(const AppInstance::RenderWork& w,bool renderInSeparateProcess,const QString& savePath) OVERRIDE FINAL;

    virtual void clearViewersLastRenderedTexture() OVERRIDE FINAL;
    
public slots:

    void onProcessFinished();

    void projectFormatChanged(const Format& f);
private:

    virtual void createBackDrop() OVERRIDE FINAL;
    virtual void createNodeGui(boost::shared_ptr<Natron::Node> node,
                               const std::string & multiInstanceParentName,
                               bool loadRequest,
                               bool autoConnect,
                               double xPosHint,double yPosHint,
                               bool pushUndoRedoCommand) OVERRIDE FINAL;


    boost::scoped_ptr<GuiAppInstancePrivate> _imp;
};

#endif // GUIAPPINSTANCE_H
