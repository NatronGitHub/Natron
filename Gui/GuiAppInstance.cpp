//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#include "GuiAppInstance.h"

#include <QDir>
#include <QMutex>
#include <QCoreApplication>

#include "Gui/GuiApplicationManager.h"
#include "Gui/Gui.h"
#include "Gui/NodeGraph.h"
#include "Gui/NodeGui.h"
#include "Gui/MultiInstancePanel.h"
#include "Gui/NodeBackDropSerialization.h"
#include "Gui/ViewerTab.h"
#include "Gui/ViewerGL.h"

#include "Engine/Project.h"
#include "Engine/EffectInstance.h"
#include "Engine/Node.h"
#include "Engine/ProcessHandler.h"
#include "Engine/Settings.h"
#include "Engine/KnobFile.h"
#include "Engine/ViewerInstance.h"
using namespace Natron;

struct GuiAppInstancePrivate
{
    Gui* _gui; //< ptr to the Gui interface
    std::map<boost::shared_ptr<Natron::Node>,boost::shared_ptr<NodeGui> > _nodeMapping; //< a mapping between all nodes and their respective gui. FIXME: it should go away.
    std::list< boost::shared_ptr<ProcessHandler> > _activeBgProcesses;
    QMutex _activeBgProcessesMutex;
    bool _isClosing;

    //////////////
    ////This one is a little tricky:
    ////If the GUI is showing a dialog, then when the exec loop of the dialog returns, it will
    ////immediately process events that were triggered meanwhile. That means that if the dialog
    ////is popped after a request made inside a plugin action, then it may trigger another action
    ////like, say, overlay focus gain on the viewer. If this boolean is true we then don't do any
    ////check on the recursion level of actions. This is a limitation of using the main "qt" thread
    ////as the main "action" thread.
    bool _showingDialog;
    mutable QMutex _showingDialogMutex;

    boost::shared_ptr<FileDialogPreviewProvider> _previewProvider;

    GuiAppInstancePrivate()
        : _gui(NULL)
          , _nodeMapping()
          , _activeBgProcesses()
          , _activeBgProcessesMutex()
          , _isClosing(false)
          , _showingDialog(false)
          , _showingDialogMutex()
          , _previewProvider(new FileDialogPreviewProvider)
    {
    }
};

GuiAppInstance::GuiAppInstance(int appID)
    : AppInstance(appID)
      , _imp(new GuiAppInstancePrivate)

{
}

void
GuiAppInstance::aboutToQuit()
{
    /**
     Kill the nodes used to make the previews in the file dialogs
     **/
    if (_imp->_previewProvider->viewerNode) {
        _imp->_gui->removeViewerTab(_imp->_previewProvider->viewerUI, true, true);
		boost::shared_ptr<Natron::Node> node = _imp->_previewProvider->viewerNode->getNode();
		ViewerInstance* liveInstance = dynamic_cast<ViewerInstance*>(node->getLiveInstance());
		assert(liveInstance);
        node->deactivate(std::list< Natron::Node* > (),false,false,true,false);
		liveInstance->invalidateUiContext();
        node->removeReferences();
        _imp->_previewProvider->viewerNode->deleteReferences();
    }
    
    for (std::map<std::string,boost::shared_ptr<NodeGui> >::iterator it = _imp->_previewProvider->readerNodes.begin();
         it != _imp->_previewProvider->readerNodes.end(); ++it) {
        it->second->getNode()->removeReferences();
        it->second->deleteReferences();
    }
    _imp->_previewProvider->readerNodes.clear();
    
    _imp->_previewProvider.reset();

    
    _imp->_isClosing = true;
    _imp->_nodeMapping.clear(); //< necessary otherwise Qt parenting system will try to delete the NodeGui instead of automatic shared_ptr
    _imp->_gui->close();
    _imp->_gui->setParent(NULL);
}

GuiAppInstance::~GuiAppInstance()
{
    ///process events before closing gui
    QCoreApplication::processEvents();

    ///clear nodes prematurely so that any thread running is stopped
    getProject()->clearNodes(false);

    _imp->_nodeMapping.clear();
    QCoreApplication::processEvents();
//#ifndef __NATRON_WIN32__
    _imp->_gui->getNodeGraph()->discardGuiPointer();
    _imp->_gui->deleteLater();
    _imp->_gui = 0;
    _imp.reset();
//#endif
    QCoreApplication::processEvents();
}

bool
GuiAppInstance::isClosing() const
{
    return !_imp || _imp->_isClosing;
}

void
GuiAppInstance::load(const QString & projectName,
                     const std::list<AppInstance::RenderRequest>& /*writersWork*/)
{
    appPTR->setLoadingStatus( tr("Creating user interface...") );
    _imp->_gui = new Gui(this);
    _imp->_gui->createGui();


    ///if the app is interactive, build the plugins toolbuttons from the groups we extracted off the plugins.
    const std::vector<PluginGroupNode*> & _toolButtons = appPTR->getPluginsToolButtons();
    for (U32 i = 0; i < _toolButtons.size(); ++i) {
        assert(_toolButtons[i]);
        _imp->_gui->findOrCreateToolButton(_toolButtons[i]);
    }
    emit pluginsPopulated();

    ///show the gui
    _imp->_gui->show();


    QObject::connect(getProject().get(), SIGNAL(formatChanged(Format)), this, SLOT(projectFormatChanged(Format)));
    
    if ( (getAppID() == 0) && appPTR->getCurrentSettings()->isCheckForUpdatesEnabled() ) {
        appPTR->setLoadingStatus( tr("Checking if updates are available...") );
        ///Before loading autosave check for a new version
        checkForNewVersion();
    }

    /// Create auto-save dir if it does not exists
    QDir dir = Natron::Project::autoSavesDir();
    dir.mkpath(".");


    if (getAppID() == 0) {
        appPTR->getCurrentSettings()->doOCIOStartupCheckIfNeeded();
        
        if (!appPTR->isShorcutVersionUpToDate()) {
            Natron::StandardButtonEnum reply = questionDialog(tr("Shortcuts").toStdString(),
                                                              tr("Default shortcuts for " NATRON_APPLICATION_NAME " have changed, "
                                                                 "would you like to set them to their defaults ? "
                                                                 "Clicking no will keep the old shortcuts hence if a new shortcut has been "
                                                                 "set to something else than an empty shortcut you won't benefit of it.").toStdString(),
                                                              Natron::StandardButtons(Natron::eStandardButtonYes | Natron::eStandardButtonNo),
                                                              Natron::eStandardButtonNo);
            if (reply == Natron::eStandardButtonYes) {
                appPTR->restoreDefaultShortcuts();
            }
        }
    }
    
    /// If this is the first instance of the software, try to load an autosave
    if ( (getAppID() == 0) && projectName.isEmpty() ) {
        if ( getProject()->findAndTryLoadAutoSave() ) {
            ///if we successfully loaded an autosave ignore the specified project in the launch args.
            return;
        }
    }
    
   

    if ( projectName.isEmpty() ) {
        ///if the user didn't specify a projects name in the launch args just create a viewer node.
        createNode( CreateNodeArgs("Viewer",
                                   "",
                                   -1,-1,
                                   -1,
                                   true,
                                   INT_MIN,INT_MIN,
                                   true,
                                   true,
                                   QString(),
                                   CreateNodeArgs::DefaultValuesList()) );
    } else {
        ///Otherwise just load the project specified.
        QFileInfo info(projectName);
        QString name = info.fileName();
        QString path = info.path();
        path += QDir::separator();
        appPTR->setLoadingStatus(tr("Loading project: ") + path + name);
        getProject()->loadProject(path,name);
        ///remove any file open event that might have occured
        appPTR->setFileToOpen("");
    }
} // load

void
GuiAppInstance::createNodeGui(boost::shared_ptr<Natron::Node> node,
                              const std::string & multiInstanceParentName,
                              bool loadRequest,
                              bool autoConnect,
                              double xPosHint,
                              double yPosHint,
                              bool pushUndoRedoCommand)
{
    boost::shared_ptr<NodeGui> nodegui = _imp->_gui->createNodeGUI(node,loadRequest,xPosHint,yPosHint,pushUndoRedoCommand);

    assert(nodegui);
    if ( !multiInstanceParentName.empty() ) {
        nodegui->hideGui();


        boost::shared_ptr<NodeGui> parentNodeGui = getNodeGui(multiInstanceParentName);
        nodegui->setParentMultiInstance(parentNodeGui);
    }
    _imp->_nodeMapping.insert( std::make_pair(node,nodegui) );

    ///It needs to be here because we rely on the _nodeMapping member
    bool isViewer = node->getPluginID() == "Viewer";
    if (isViewer) {
        _imp->_gui->createViewerGui(node);
    }

    ///must be done after the viewer gui has been created
    if ( node->isRotoNode() ) {
        _imp->_gui->createNewRotoInterface( nodegui.get() );
    }

    if ( node->isTrackerNode() && multiInstanceParentName.empty() ) {
        _imp->_gui->createNewTrackerInterface( nodegui.get() );
    }

    ///Don't initialize inputs if it is a multi-instance child since it is not part of  the graph
    if ( multiInstanceParentName.empty() ) {
        nodegui->initializeInputs();
    }

    nodegui->initializeKnobs();

    if (!loadRequest) {
        nodegui->beginEditKnobs();
    }

    ///must be called after initializeKnobs as it populates the node's knobs in the curve editor;
    _imp->_gui->addNodeGuiToCurveEditor(nodegui);


    if ( !loadRequest && multiInstanceParentName.empty() ) {
        const std::list<boost::shared_ptr<NodeGui> > & selectedNodes = _imp->_gui->getSelectedNodes();
        if ( (selectedNodes.size() == 1) && autoConnect ) {
            const boost::shared_ptr<Node> & selected = selectedNodes.front()->getNode();
            getProject()->autoConnectNodes(selected, node);
        }
        _imp->_gui->selectNode(nodegui);
        
        ///we make sure we can have a clean preview.
        node->computePreviewImage( getTimeLine()->currentFrame() );

    }
    if (!loadRequest && !isViewer) {
        triggerAutoSave();
    }
} // createNodeGui

std::string
GuiAppInstance::openImageFileDialog()
{
    return _imp->_gui->openImageSequenceDialog();
}

std::string
GuiAppInstance::saveImageFileDialog()
{
    return _imp->_gui->saveImageSequenceDialog();
}

Gui*
GuiAppInstance::getGui() const
{
    return _imp->_gui;
}

bool
GuiAppInstance::shouldRefreshPreview() const
{
    return !_imp->_gui->isUserScrubbingTimeline();
}

boost::shared_ptr<NodeGui> GuiAppInstance::getNodeGui(const boost::shared_ptr<Node> & n) const
{
    return getNodeGui(n.get());
}

boost::shared_ptr<NodeGui>
GuiAppInstance::getNodeGui(Natron::Node* n) const 
{
    for (std::map<boost::shared_ptr<Node>,boost::shared_ptr<NodeGui> >::const_iterator it = _imp->_nodeMapping.begin();
         it != _imp->_nodeMapping.end(); ++it) {
        if (it->first.get() == n) {
            return it->second;
        }
    }
    return boost::shared_ptr<NodeGui>();
}

boost::shared_ptr<NodeGui> GuiAppInstance::getNodeGui(const std::string & nodeName) const
{
    for (std::map<boost::shared_ptr<Node>,boost::shared_ptr<NodeGui> >::const_iterator it = _imp->_nodeMapping.begin();
         it != _imp->_nodeMapping.end(); ++it) {
        assert(it->first && it->second);
        if (it->first->getName() == nodeName) {
            return it->second;
        }
    }

    return boost::shared_ptr<NodeGui>();
}

boost::shared_ptr<Node> GuiAppInstance::getNode(const boost::shared_ptr<NodeGui> & n) const
{
    for (std::map<boost::shared_ptr<Node>,boost::shared_ptr<NodeGui> >::const_iterator it = _imp->_nodeMapping.begin(); it != _imp->_nodeMapping.end(); ++it) {
        if (it->second == n) {
            return it->first;
        }
    }

    return boost::shared_ptr<Node>();
}

void
GuiAppInstance::deleteNode(const boost::shared_ptr<NodeGui> & n)
{
    if ( !isClosing() ) {
        getProject()->removeNodeFromProject( n->getNode() );
    }
    for (std::map<boost::shared_ptr<Node>,boost::shared_ptr<NodeGui> >::iterator it = _imp->_nodeMapping.begin(); it != _imp->_nodeMapping.end(); ++it) {
        if (it->second == n) {
            _imp->_nodeMapping.erase(it);
            break;
        }
    }
}

void
GuiAppInstance::errorDialog(const std::string & title,
                            const std::string & message) const
{
    if (appPTR->isSplashcreenVisible()) {
        appPTR->hideSplashScreen();
    }
    {
        QMutexLocker l(&_imp->_showingDialogMutex);
        _imp->_showingDialog = true;
    }
    _imp->_gui->errorDialog(title, message);
    {
        QMutexLocker l(&_imp->_showingDialogMutex);
        _imp->_showingDialog = false;
    }
}

void
GuiAppInstance::errorDialog(const std::string & title,const std::string & message,bool* stopAsking) const
{
    if (appPTR->isSplashcreenVisible()) {
        appPTR->hideSplashScreen();
    }
    {
        QMutexLocker l(&_imp->_showingDialogMutex);
        _imp->_showingDialog = true;
    }
    _imp->_gui->errorDialog(title, message, stopAsking);
    {
        QMutexLocker l(&_imp->_showingDialogMutex);
        _imp->_showingDialog = false;
    }
}

void
GuiAppInstance::warningDialog(const std::string & title,
                              const std::string & message) const
{
    if (appPTR->isSplashcreenVisible()) {
        appPTR->hideSplashScreen();
    }
    {
        QMutexLocker l(&_imp->_showingDialogMutex);
        _imp->_showingDialog = true;
    }
    _imp->_gui->warningDialog(title, message);
    {
        QMutexLocker l(&_imp->_showingDialogMutex);
        _imp->_showingDialog = false;
    }
}

void
GuiAppInstance::warningDialog(const std::string & title,
                              const std::string & message,
                              bool* stopAsking) const
{
    if (appPTR->isSplashcreenVisible()) {
        appPTR->hideSplashScreen();
    }
    {
        QMutexLocker l(&_imp->_showingDialogMutex);
        _imp->_showingDialog = true;
    }
    _imp->_gui->warningDialog(title, message, stopAsking);
    {
        QMutexLocker l(&_imp->_showingDialogMutex);
        _imp->_showingDialog = false;
    }
}

void
GuiAppInstance::informationDialog(const std::string & title,
                                  const std::string & message) const
{
    if (appPTR->isSplashcreenVisible()) {
        appPTR->hideSplashScreen();
    }
    {
        QMutexLocker l(&_imp->_showingDialogMutex);
        _imp->_showingDialog = true;
    }
    _imp->_gui->informationDialog(title, message);
    {
        QMutexLocker l(&_imp->_showingDialogMutex);
        _imp->_showingDialog = false;
    }
}

void
GuiAppInstance::informationDialog(const std::string & title,
                                  const std::string & message,
                                  bool* stopAsking) const
{
    if (appPTR->isSplashcreenVisible()) {
        appPTR->hideSplashScreen();
    }
    {
        QMutexLocker l(&_imp->_showingDialogMutex);
        _imp->_showingDialog = true;
    }
    _imp->_gui->informationDialog(title, message, stopAsking);
    {
        QMutexLocker l(&_imp->_showingDialogMutex);
        _imp->_showingDialog = false;
    }
}

Natron::StandardButtonEnum
GuiAppInstance::questionDialog(const std::string & title,
                               const std::string & message,
                               Natron::StandardButtons buttons,
                               Natron::StandardButtonEnum defaultButton) const
{
    if (appPTR->isSplashcreenVisible()) {
        appPTR->hideSplashScreen();
    }
    {
        QMutexLocker l(&_imp->_showingDialogMutex);
        _imp->_showingDialog = true;
    }
    Natron::StandardButtonEnum ret =  _imp->_gui->questionDialog(title, message,buttons,defaultButton);
    {
        QMutexLocker l(&_imp->_showingDialogMutex);
        _imp->_showingDialog = false;
    }

    return ret;
}

Natron::StandardButtonEnum
GuiAppInstance::questionDialog(const std::string & title,
                               const std::string & message,
                               Natron::StandardButtons buttons,
                               Natron::StandardButtonEnum defaultButton,
                               bool* stopAsking)
{
    if (appPTR->isSplashcreenVisible()) {
        appPTR->hideSplashScreen();
    }
    {
        QMutexLocker l(&_imp->_showingDialogMutex);
        _imp->_showingDialog = true;
    }
    Natron::StandardButtonEnum ret =  _imp->_gui->questionDialog(title, message,buttons,defaultButton,stopAsking);
    {
        QMutexLocker l(&_imp->_showingDialogMutex);
        _imp->_showingDialog = false;
    }
    
    return ret;
}

bool
GuiAppInstance::isShowingDialog() const
{
    QMutexLocker l(&_imp->_showingDialogMutex);

    return _imp->_showingDialog;
}

void
GuiAppInstance::loadProjectGui(boost::archive::xml_iarchive & archive) const
{
    _imp->_gui->loadProjectGui(archive);
}

void
GuiAppInstance::saveProjectGui(boost::archive::xml_oarchive & archive)
{
    _imp->_gui->saveProjectGui(archive);
}

void
GuiAppInstance::setupViewersForViews(int viewsCount)
{
    _imp->_gui->updateViewersViewsMenu(viewsCount);
}

void
GuiAppInstance::setViewersCurrentView(int view)
{
    _imp->_gui->setViewersCurrentView(view);
}

void
GuiAppInstance::startRenderingFullSequence(const AppInstance::RenderWork& w,bool renderInSeparateProcess,const QString& savePath)
{
   


    ///validate the frame range to render
    int firstFrame,lastFrame;
    if (w.firstFrame == INT_MIN || w.lastFrame == INT_MAX) {
        w.writer->getFrameRange_public(w.writer->getHash(),&firstFrame, &lastFrame);
        //if firstframe and lastframe are infinite clamp them to the timeline bounds
        if (firstFrame == INT_MIN) {
            firstFrame = getTimeLine()->firstFrame();
        }
        if (lastFrame == INT_MAX) {
            lastFrame = getTimeLine()->lastFrame();
        }
        if (firstFrame > lastFrame) {
            Natron::errorDialog( w.writer->getNode()->getName_mt_safe(),
                                tr("First frame in the sequence is greater than the last frame").toStdString() );
            
            return;
        }
    } else {
        firstFrame = w.firstFrame;
        lastFrame = w.lastFrame;
    }
    
    ///get the output file knob to get the name of the sequence
    QString outputFileSequence;
    boost::shared_ptr<KnobI> fileKnob = w.writer->getKnobByName(kOfxImageEffectFileParamName);
    if (fileKnob) {
        Knob<std::string>* isString = dynamic_cast<Knob<std::string>*>(fileKnob.get());
        assert(isString);
        outputFileSequence = isString->getValue().c_str();
    }


    if ( renderInSeparateProcess ) {
        try {
            boost::shared_ptr<ProcessHandler> process( new ProcessHandler(this,savePath,w.writer) );
            QObject::connect( process.get(), SIGNAL( processFinished(int) ), this, SLOT( onProcessFinished() ) );
            notifyRenderProcessHandlerStarted(outputFileSequence,firstFrame,lastFrame,process);
            process->startProcess();

            {
                QMutexLocker l(&_imp->_activeBgProcessesMutex);
                _imp->_activeBgProcesses.push_back(process);
            }
        } catch (const std::exception & e) {
            Natron::errorDialog( w.writer->getName(), tr("Error while starting rendering").toStdString() + ": " + e.what() );
        } catch (...) {
            Natron::errorDialog( w.writer->getName(), tr("Error while starting rendering").toStdString() );
        }
    } else {
        _imp->_gui->onWriterRenderStarted(outputFileSequence, firstFrame, lastFrame, w.writer);
        w.writer->renderFullSequence(NULL,firstFrame,lastFrame);
    }
} // startRenderingFullSequence

void
GuiAppInstance::onProcessFinished()
{
    ProcessHandler* proc = qobject_cast<ProcessHandler*>( sender() );

    if (proc) {
        QMutexLocker l(&_imp->_activeBgProcessesMutex);
        for (std::list< boost::shared_ptr<ProcessHandler> >::iterator it = _imp->_activeBgProcesses.begin(); it != _imp->_activeBgProcesses.end(); ++it) {
            if ( (*it).get() == proc ) {
                _imp->_activeBgProcesses.erase(it);

                return;
            }
        }
    }
}

void
GuiAppInstance::clearNodeGuiMapping()
{
    _imp->_nodeMapping.clear();
}

void
GuiAppInstance::notifyRenderProcessHandlerStarted(const QString & sequenceName,
                                                  int firstFrame,
                                                  int lastFrame,
                                                  const boost::shared_ptr<ProcessHandler> & process)
{
    _imp->_gui->onProcessHandlerStarted(sequenceName,firstFrame,lastFrame,process);
}

void
GuiAppInstance::setUndoRedoStackLimit(int limit)
{
    _imp->_gui->setUndoRedoStackLimit(limit);
}

void
GuiAppInstance::startProgress(Natron::EffectInstance* effect,
                              const std::string & message)
{
    {
        QMutexLocker l(&_imp->_showingDialogMutex);
        _imp->_showingDialog = true;
    }

    _imp->_gui->startProgress(effect, message);
}

void
GuiAppInstance::endProgress(Natron::EffectInstance* effect)
{
    _imp->_gui->endProgress(effect);
    {
        QMutexLocker l(&_imp->_showingDialogMutex);
        _imp->_showingDialog = false;
    }
}

bool
GuiAppInstance::progressUpdate(Natron::EffectInstance* effect,
                               double t)
{
    bool ret =  _imp->_gui->progressUpdate(effect, t);

    if (!ret) {
        {
            QMutexLocker l(&_imp->_showingDialogMutex);
            _imp->_showingDialog = false;
        }
    }

    return ret;
}

void
GuiAppInstance::onMaxPanelsOpenedChanged(int maxPanels)
{
    _imp->_gui->onMaxVisibleDockablePanelChanged(maxPanels);
}

void
GuiAppInstance::createBackDrop()
{
    ///This function is not used when loading a project, rather we use the one directly in Gui (@see ProjectGui::load)
    _imp->_gui->createBackDrop( false,NodeBackDropSerialization() );
}



void
GuiAppInstance::connectViewersToViewerCache()
{
    _imp->_gui->connectViewersToViewerCache();
}

void
GuiAppInstance::disconnectViewersFromViewerCache()
{
    _imp->_gui->disconnectViewersFromViewerCache();
}

void
GuiAppInstance::aboutToAutoSave()
{
    _imp->_gui->aboutToSave();
}

void
GuiAppInstance::autoSaveFinished()
{
    _imp->_gui->saveFinished();
}


boost::shared_ptr<FileDialogPreviewProvider>
GuiAppInstance::getPreviewProvider() const
{
    return _imp->_previewProvider;
}

void
GuiAppInstance::projectFormatChanged(const Format& /*f*/)
{
    if (_imp->_previewProvider && _imp->_previewProvider->viewerNode && _imp->_previewProvider->viewerUI) {
        _imp->_previewProvider->viewerUI->getInternalNode()->renderCurrentFrame(true);
    }
}

bool
GuiAppInstance::isGuiFrozen() const
{
    return _imp->_gui->isGUIFrozen();
}


void
GuiAppInstance::clearViewersLastRenderedTexture()
{
    std::list<ViewerTab*> tabs = _imp->_gui->getViewersList_mt_safe();
    for (std::list<ViewerTab*>::const_iterator it = tabs.begin(); it!=tabs.end(); ++it) {
        (*it)->getViewer()->clearLastRenderedTexture();
    }
}