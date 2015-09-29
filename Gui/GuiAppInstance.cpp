/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "GuiAppInstance.h"

#include <stdexcept>

#include <QDir>
#include <QSettings>
#include <QMutex>
#include <QCoreApplication>

#include "Engine/CLArgs.h"
#include "Engine/Project.h"
#include "Engine/EffectInstance.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/Plugin.h"
#include "Engine/ProcessHandler.h"
#include "Engine/Settings.h"
#include "Engine/DiskCacheNode.h"
#include "Engine/KnobFile.h"
#include "Engine/ViewerInstance.h"

#include "Gui/GuiApplicationManager.h"
#include "Gui/Gui.h"
#include "Gui/BackDropGui.h"
#include "Gui/NodeGraph.h"
#include "Gui/NodeGui.h"
#include "Gui/MultiInstancePanel.h"
#include "Gui/ViewerTab.h"
#include "Gui/SplashScreen.h"
#include "Gui/ViewerGL.h"

using namespace Natron;

struct GuiAppInstancePrivate
{
    Gui* _gui; //< ptr to the Gui interface

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

    mutable QMutex lastTimelineViewerMutex;
    boost::shared_ptr<Natron::Node> lastTimelineViewer;

    LoadProjectSplashScreen* loadProjectSplash;

    std::string declareAppAndParamsString;
    int overlayRedrawRequests;
    
    mutable QMutex userIsPaintingMutex;
    boost::shared_ptr<Natron::Node> userIsPainting;
    bool turboAlreadyActiveBeforePainting;
    
    GuiAppInstancePrivate()
    : _gui(NULL)
    , _activeBgProcesses()
    , _activeBgProcessesMutex()
    , _isClosing(false)
    , _showingDialog(false)
    , _showingDialogMutex()
    , _previewProvider(new FileDialogPreviewProvider)
    , lastTimelineViewerMutex()
    , lastTimelineViewer()
    , loadProjectSplash(0)
    , declareAppAndParamsString()
    , overlayRedrawRequests(0)
    , userIsPaintingMutex()
    , userIsPainting()
    , turboAlreadyActiveBeforePainting(false)
    {
    }

    void findOrCreateToolButtonRecursive(const boost::shared_ptr<PluginGroupNode>& n);
};

GuiAppInstance::GuiAppInstance(int appID)
    : AppInstance(appID)
      , _imp(new GuiAppInstancePrivate)

{
}

void
GuiAppInstance::resetPreviewProvider()
{
    deletePreviewProvider();
    _imp->_previewProvider.reset(new FileDialogPreviewProvider);
}

void
GuiAppInstance::deletePreviewProvider()
{
    /**
     Kill the nodes used to make the previews in the file dialogs
     **/
    if (_imp->_previewProvider) {
        if (_imp->_previewProvider->viewerNode) {
            _imp->_gui->removeViewerTab(_imp->_previewProvider->viewerUI, true, true);
            boost::shared_ptr<Natron::Node> node = _imp->_previewProvider->viewerNodeInternal;
            if (node) {
                ViewerInstance* liveInstance = dynamic_cast<ViewerInstance*>(node->getLiveInstance());
                if (liveInstance) {
                    node->deactivate(std::list< Natron::Node* > (),false,false,true,false);
                    liveInstance->invalidateUiContext();
                    node->removeReferences(false);
                    _imp->_previewProvider->viewerNode->deleteReferences();
                    _imp->_previewProvider->viewerNodeInternal.reset();
                }
            }

        }

        for (std::map<std::string,std::pair< boost::shared_ptr<Natron::Node>, boost::shared_ptr<NodeGui> > >::iterator it =
             _imp->_previewProvider->readerNodes.begin();
             it != _imp->_previewProvider->readerNodes.end(); ++it) {
            it->second.second->getNode()->removeReferences(false);
            it->second.second->deleteReferences();
        }
        _imp->_previewProvider->readerNodes.clear();

        _imp->_previewProvider.reset();
    }
}

void
GuiAppInstance::aboutToQuit()
{

    deletePreviewProvider();
    _imp->_isClosing = true;
    _imp->_gui->close();
    _imp->_gui->setParent(NULL);
}

GuiAppInstance::~GuiAppInstance()
{
    ///process events before closing gui
    QCoreApplication::processEvents();

    ///clear nodes prematurely so that any thread running is stopped
    getProject()->clearNodes(false);

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
GuiAppInstancePrivate::findOrCreateToolButtonRecursive(const boost::shared_ptr<PluginGroupNode>& n)
{
    _gui->findOrCreateToolButton(n);
    const std::list<boost::shared_ptr<PluginGroupNode> >& children = n->getChildren();
    for (std::list<boost::shared_ptr<PluginGroupNode> >::const_iterator it = children.begin(); it != children.end(); ++it) {
        findOrCreateToolButtonRecursive(*it);
    }
}

void
GuiAppInstance::load(const CLArgs& cl)
{

    if (getAppID() == 0) {
        appPTR->setLoadingStatus( QObject::tr("Creating user interface...") );
    }
    
    declareCurrentAppVariable_Python();
    
    _imp->_gui = new Gui(this);
    _imp->_gui->createGui();

    printAutoDeclaredVariable(_imp->declareAppAndParamsString);

    ///if the app is interactive, build the plugins toolbuttons from the groups we extracted off the plugins.
    const std::list<boost::shared_ptr<PluginGroupNode> > & _toolButtons = appPTR->getTopLevelPluginsToolButtons();
    for (std::list<boost::shared_ptr<PluginGroupNode>  >::const_iterator it = _toolButtons.begin(); it != _toolButtons.end(); ++it) {
        _imp->findOrCreateToolButtonRecursive(*it);
    }
    _imp->_gui->sortAllPluginsToolButtons();
    
    Q_EMIT pluginsPopulated();

    ///show the gui
    _imp->_gui->show();

    
    boost::shared_ptr<Settings> nSettings = appPTR->getCurrentSettings();

    QObject::connect(getProject().get(), SIGNAL(formatChanged(Format)), this, SLOT(projectFormatChanged(Format)));

    {
        QSettings settings(NATRON_ORGANIZATION_NAME,NATRON_APPLICATION_NAME);
        if ( !settings.contains("checkForUpdates") ) {
            Natron::StandardButtonEnum reply = Natron::questionDialog(tr("Updates").toStdString(),
                                                                      tr("Do you want " NATRON_APPLICATION_NAME " to check for updates "
                                                                      "on launch of the application ?").toStdString(), false);
            bool checkForUpdates = reply == Natron::eStandardButtonYes;
            nSettings->setCheckUpdatesEnabled(checkForUpdates);
        }

        if (nSettings->isCheckForUpdatesEnabled()) {
            appPTR->setLoadingStatus( tr("Checking if updates are available...") );
            checkForNewVersion();
        }
    }
    
    if (nSettings->isDefaultAppearanceOutdated()) {
        Natron::StandardButtonEnum reply = Natron::questionDialog(tr("Appearance").toStdString(),
                                                                  tr(NATRON_APPLICATION_NAME " default appearance changed since last version.\n"
                                                                     "Would you like to set the new default appearance?").toStdString(), false);
        if (reply == Natron::eStandardButtonYes) {
            nSettings->restoreDefaultAppearance();
        }
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
                                                              false,
                                                              Natron::StandardButtons(Natron::eStandardButtonYes | Natron::eStandardButtonNo),
                                                              Natron::eStandardButtonNo);
            if (reply == Natron::eStandardButtonYes) {
                appPTR->restoreDefaultShortcuts();
            }
        }
    }

    /// If this is the first instance of the software, try to load an autosave
    if ( (getAppID() == 0) && cl.getFilename().isEmpty() ) {
        if (findAndTryLoadUntitledAutoSave()) {
            ///if we successfully loaded an autosave ignore the specified project in the launch args.
            return;
        }
    }


    QFileInfo info(cl.getFilename());

    if (cl.getFilename().isEmpty() || !info.exists()) {

        getProject()->createViewer();
        execOnProjectCreatedCallback();

    } else {


        if (info.suffix() == "py") {

            appPTR->setLoadingStatus(tr("Loading script: ") + cl.getFilename());

            ///If this is a Python script, execute it
            loadPythonScript(info);
            execOnProjectCreatedCallback();

        } else if (info.suffix() == NATRON_PROJECT_FILE_EXT) {

            ///Otherwise just load the project specified.
            QString name = info.fileName();
            QString path = info.path();
            path += QDir::separator();
            appPTR->setLoadingStatus(tr("Loading project: ") + path + name);
            getProject()->loadProject(path,name);
            ///remove any file open event that might have occured
            appPTR->setFileToOpen("");
        } else {
            Natron::errorDialog(tr("Invalid file").toStdString(),
                                tr(NATRON_APPLICATION_NAME " only accepts python scripts or .ntp project files").toStdString());
            execOnProjectCreatedCallback();
        }
    }
    
    const QString& extraOnProjectCreatedScript = cl.getDefaultOnProjectLoadedScript();
    if (!extraOnProjectCreatedScript.isEmpty()) {
        QFileInfo cbInfo(extraOnProjectCreatedScript);
        if (cbInfo.exists()) {
            loadPythonScript(cbInfo);
        }
    }

    
} // load


bool
GuiAppInstance::findAndTryLoadUntitledAutoSave()
{
    QDir savesDir(Project::autoSavesDir());
    QStringList entries = savesDir.entryList(QDir::Files | QDir::NoDotAndDotDot);
    
    QStringList foundAutosaves;
    for (int i = 0; i < entries.size(); ++i) {
        const QString & entry = entries.at(i);
        QString searchStr('.');
        searchStr.append(NATRON_PROJECT_FILE_EXT);
        searchStr.append(".autosave");
        int suffixPos = entry.indexOf(searchStr);
        if (suffixPos == -1 || entry.contains("RENDER_SAVE")) {
            continue;
        }
        
        foundAutosaves << entry;
    }
    if (foundAutosaves.empty()) {
        return false;
    }
    
    QString text = tr("An auto-save was restored successfully. It didn't belong to any project\n"
                      "Would you like to restore it ? Clicking No will remove this auto-save.");
    
    
    appPTR->hideSplashScreen();
    
    Natron::StandardButtonEnum ret = Natron::questionDialog(tr("Auto-save").toStdString(),
                                                            text.toStdString(),false, Natron::StandardButtons(Natron::eStandardButtonYes | Natron::eStandardButtonNo),
                                                            Natron::eStandardButtonYes);
    if ( (ret == Natron::eStandardButtonNo) || (ret == Natron::eStandardButtonEscape) ) {
        Project::clearAutoSavesDir();
        return false;
    }
    
    for (int i = 0; i < foundAutosaves.size(); ++i) {
        const QString& autoSaveFileName = foundAutosaves[i];
        if (i == 0) {
            //Load the first one into the current instance of Natron, then open-up new instances
            if (!getProject()->loadProject(savesDir.path() + '/', autoSaveFileName, true)) {
                return false;
            }
        } else {
            CLArgs cl;
            AppInstance* newApp = appPTR->newAppInstance(cl);
            if (!newApp->getProject()->loadProject(savesDir.path() + '/', autoSaveFileName, true)) {
                return false;
            }
        }
    }
    
    return true;
  
} // findAndTryLoadAutoSave


void
GuiAppInstance::createNodeGui(const boost::shared_ptr<Natron::Node> &node,
                              const boost::shared_ptr<Natron::Node>& parentMultiInstance,
                              bool loadRequest,
                              bool autoConnect,
                              double xPosHint,
                              double yPosHint,
                              bool pushUndoRedoCommand)
{

    boost::shared_ptr<NodeCollection> group = node->getGroup();

    NodeGraph* graph;

    if (group) {
        NodeGraphI* graph_i = group->getNodeGraph();
        assert(graph_i);
        graph = dynamic_cast<NodeGraph*>(graph_i);
        assert(graph);
    } else {
        graph = _imp->_gui->getNodeGraph();
    }

    std::list<boost::shared_ptr<NodeGui> >  selectedNodes = graph->getSelectedNodes();

    boost::shared_ptr<NodeGui> nodegui = _imp->_gui->createNodeGUI(node,loadRequest,pushUndoRedoCommand);

    assert(nodegui);
    if ( parentMultiInstance && nodegui) {
        nodegui->hideGui();


        boost::shared_ptr<NodeGuiI> parentNodeGui_i = parentMultiInstance->getNodeGui();
        assert(parentNodeGui_i);
        nodegui->setParentMultiInstance(boost::dynamic_pointer_cast<NodeGui>(parentNodeGui_i));
    }

    ///It needs to be here because we rely on the _nodeMapping member
    bool isViewer = dynamic_cast<ViewerInstance*>(node->getLiveInstance());
    if (isViewer) {
        _imp->_gui->createViewerGui(node);
    }

    ///must be done after the viewer gui has been created
    if (node->isRotoPaintingNode()) {
        _imp->_gui->createNewRotoInterface( nodegui.get() );
    }

    if ( node->isPointTrackerNode() && !parentMultiInstance ) {
        _imp->_gui->createNewTrackerInterface( nodegui.get() );
    }

    NodeGroup* isGroup = dynamic_cast<NodeGroup*>(node->getLiveInstance());
    if (isGroup) {
        _imp->_gui->createGroupGui(node, loadRequest);
    }

    ///Don't initialize inputs if it is a multi-instance child since it is not part of  the graph
    if ( !parentMultiInstance) {
        nodegui->initializeInputs();
    }

    if (!loadRequest && !isViewer) {
        ///we make sure we can have a clean preview.
        node->computePreviewImage( getTimeLine()->currentFrame() );

        triggerAutoSave();
    }
    
    
    ///only move main instances
    if (node->getParentMultiInstanceName().empty()) {
        if (selectedNodes.empty()) {
            autoConnect = false;
        }
        if ( (xPosHint != INT_MIN) && (yPosHint != INT_MIN) && !autoConnect ) {
            QPointF pos = nodegui->mapToParent( nodegui->mapFromScene( QPointF(xPosHint,yPosHint) ) );
            nodegui->refreshPosition( pos.x(),pos.y(), true );
        } else {
            BackDropGui* isBd = dynamic_cast<BackDropGui*>(nodegui.get());
            if (!isBd && !isGroup) {
                boost::shared_ptr<NodeGui> selectedNode;
                if (selectedNodes.size() == 1) {
                    selectedNode = selectedNodes.front();
                    BackDropGui* isBackdropGui = dynamic_cast<BackDropGui*>(selectedNode.get());
                    if (isBackdropGui) {
                        selectedNode.reset();
                    }
                }
                nodegui->getDagGui()->moveNodesForIdealPosition(nodegui,selectedNode,autoConnect);
                if (autoConnect) {
                    std::list<ViewerInstance* > viewers;
                    node->hasViewersConnected(&viewers);
                    for (std::list<ViewerInstance* >::iterator it2 = viewers.begin(); it2 != viewers.end(); ++it2) {
                        (*it2)->renderCurrentFrame(true);
                    }
                }
            }
        }
    }
    
} // createNodeGui

std::string
GuiAppInstance::openImageFileDialog()
{
    {
        QMutexLocker l(&_imp->_showingDialogMutex);
        _imp->_showingDialog = true;
    }
    std::string ret = _imp->_gui->openImageSequenceDialog();
    {
        QMutexLocker l(&_imp->_showingDialogMutex);
        _imp->_showingDialog = false;
    }
    return ret;
}

std::string
GuiAppInstance::saveImageFileDialog()
{
    {
        QMutexLocker l(&_imp->_showingDialogMutex);
        _imp->_showingDialog = true;
    }
    std::string ret =  _imp->_gui->saveImageSequenceDialog();
    {
        QMutexLocker l(&_imp->_showingDialogMutex);
        _imp->_showingDialog = false;
    }
    return ret;
}

Gui*
GuiAppInstance::getGui() const
{
    return _imp->_gui;
}

bool
GuiAppInstance::shouldRefreshPreview() const
{
    return !_imp->_gui->isDraftRenderEnabled();
}


void
GuiAppInstance::deleteNode(const boost::shared_ptr<NodeGui> & n)
{
    if ( !isClosing() ) {
        boost::shared_ptr<Natron::Node> internalNode = n->getNode();
        if (internalNode) {
            getProject()->removeNode(internalNode);
            internalNode->removeReferences(true);
        }
    }
}

void
GuiAppInstance::errorDialog(const std::string & title,
                            const std::string & message,
                            bool useHtml) const
{
    if (appPTR->isSplashcreenVisible()) {
        appPTR->hideSplashScreen();
    }
    {
        QMutexLocker l(&_imp->_showingDialogMutex);
        _imp->_showingDialog = true;
    }
    _imp->_gui->errorDialog(title, message, useHtml);
    {
        QMutexLocker l(&_imp->_showingDialogMutex);
        _imp->_showingDialog = false;
    }
}

void
GuiAppInstance::errorDialog(const std::string & title,const std::string & message,bool* stopAsking, bool useHtml) const
{
    if (appPTR->isSplashcreenVisible()) {
        appPTR->hideSplashScreen();
    }
    {
        QMutexLocker l(&_imp->_showingDialogMutex);
        _imp->_showingDialog = true;
    }
    _imp->_gui->errorDialog(title, message, stopAsking, useHtml);
    {
        QMutexLocker l(&_imp->_showingDialogMutex);
        _imp->_showingDialog = false;
    }
}

void
GuiAppInstance::warningDialog(const std::string & title,
                              const std::string & message,
                              bool useHtml) const
{
    if (appPTR->isSplashcreenVisible()) {
        appPTR->hideSplashScreen();
    }
    {
        QMutexLocker l(&_imp->_showingDialogMutex);
        _imp->_showingDialog = true;
    }
    _imp->_gui->warningDialog(title, message, useHtml);
    {
        QMutexLocker l(&_imp->_showingDialogMutex);
        _imp->_showingDialog = false;
    }
}

void
GuiAppInstance::warningDialog(const std::string & title,
                              const std::string & message,
                              bool* stopAsking,
                              bool useHtml) const
{
    if (appPTR->isSplashcreenVisible()) {
        appPTR->hideSplashScreen();
    }
    {
        QMutexLocker l(&_imp->_showingDialogMutex);
        _imp->_showingDialog = true;
    }
    _imp->_gui->warningDialog(title, message, stopAsking, useHtml);
    {
        QMutexLocker l(&_imp->_showingDialogMutex);
        _imp->_showingDialog = false;
    }
}

void
GuiAppInstance::informationDialog(const std::string & title,
                                  const std::string & message,
                                  bool useHtml) const
{
    if (appPTR->isSplashcreenVisible()) {
        appPTR->hideSplashScreen();
    }
    {
        QMutexLocker l(&_imp->_showingDialogMutex);
        _imp->_showingDialog = true;
    }
    _imp->_gui->informationDialog(title, message, useHtml);
    {
        QMutexLocker l(&_imp->_showingDialogMutex);
        _imp->_showingDialog = false;
    }
}

void
GuiAppInstance::informationDialog(const std::string & title,
                                  const std::string & message,
                                  bool* stopAsking,
                                  bool useHtml) const
{
    if (appPTR->isSplashcreenVisible()) {
        appPTR->hideSplashScreen();
    }
    {
        QMutexLocker l(&_imp->_showingDialogMutex);
        _imp->_showingDialog = true;
    }
    _imp->_gui->informationDialog(title, message, stopAsking, useHtml);
    {
        QMutexLocker l(&_imp->_showingDialogMutex);
        _imp->_showingDialog = false;
    }
}

Natron::StandardButtonEnum
GuiAppInstance::questionDialog(const std::string & title,
                               const std::string & message,
                               bool useHtml,
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
    Natron::StandardButtonEnum ret =  _imp->_gui->questionDialog(title, message,useHtml, buttons,defaultButton);
    {
        QMutexLocker l(&_imp->_showingDialogMutex);
        _imp->_showingDialog = false;
    }

    return ret;
}

Natron::StandardButtonEnum
GuiAppInstance::questionDialog(const std::string & title,
                               const std::string & message,
                               bool useHtml,
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
    Natron::StandardButtonEnum ret =  _imp->_gui->questionDialog(title, message,useHtml, buttons,defaultButton,stopAsking);
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
GuiAppInstance::startRenderingFullSequence(bool enableRenderStats,const AppInstance::RenderWork& w,bool renderInSeparateProcess,const QString& savePath)
{



    ///validate the frame range to render
    double firstFrame,lastFrame;
    if (w.firstFrame == INT_MIN || w.lastFrame == INT_MAX) {
        w.writer->getFrameRange_public(w.writer->getHash(),&firstFrame, &lastFrame, true);
        //if firstframe and lastframe are infinite clamp them to the timeline bounds
        double projectFirst,projectLast;
        getFrameRange(&projectFirst, &projectLast);
        if (firstFrame == INT_MIN) {
            firstFrame = projectFirst;
        }
        if (lastFrame == INT_MAX) {
            lastFrame = projectLast;
        }
        if (firstFrame > lastFrame) {
            Natron::errorDialog( w.writer->getNode()->getLabel_mt_safe(),
                                tr("First frame in the sequence is greater than the last frame").toStdString(), false );

            return;
        }
    } else {
        firstFrame = w.firstFrame;
        lastFrame = w.lastFrame;
    }

    ///get the output file knob to get the name of the sequence
    QString outputFileSequence;

    DiskCacheNode* isDiskCache = dynamic_cast<DiskCacheNode*>(w.writer);
    if (isDiskCache) {
        outputFileSequence = isDiskCache->getNode()->getLabel_mt_safe().c_str();
    } else {
        boost::shared_ptr<KnobI> fileKnob = w.writer->getKnobByName(kOfxImageEffectFileParamName);
        if (fileKnob) {
            Knob<std::string>* isString = dynamic_cast<Knob<std::string>*>(fileKnob.get());
            assert(isString);
            outputFileSequence = isString->getValue().c_str();
        }
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
            Natron::errorDialog( w.writer->getNode()->getLabel(),
                                tr("Error while starting rendering").toStdString() + ": " + e.what(), false );
        } catch (...) {
            Natron::errorDialog( w.writer->getNode()->getLabel(),
                                tr("Error while starting rendering").toStdString(),false  );
        }
    } else {
        _imp->_gui->onWriterRenderStarted(outputFileSequence, firstFrame, lastFrame, w.writer);
        w.writer->renderFullSequence(enableRenderStats,NULL,firstFrame,lastFrame);
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
GuiAppInstance::progressStart(KnobHolder* effect,
                              const std::string &message,
                              const std::string &messageid,
                              bool canCancel)
{
    {
        QMutexLocker l(&_imp->_showingDialogMutex);
        _imp->_showingDialog = true;
    }

    _imp->_gui->progressStart(effect, message, messageid, canCancel);
}

void
GuiAppInstance::progressEnd(KnobHolder* effect)
{
    _imp->_gui->progressEnd(effect);
    {
        QMutexLocker l(&_imp->_showingDialogMutex);
        _imp->_showingDialog = false;
    }
}

bool
GuiAppInstance::progressUpdate(KnobHolder* effect,
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
GuiAppInstance::connectViewersToViewerCache()
{
    _imp->_gui->connectViewersToViewerCache();
}

void
GuiAppInstance::disconnectViewersFromViewerCache()
{
    _imp->_gui->disconnectViewersFromViewerCache();
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
    for (std::list<ViewerTab*>::const_iterator it = tabs.begin(); it != tabs.end(); ++it) {
        (*it)->getViewer()->clearLastRenderedTexture();
    }
}

void
GuiAppInstance::redrawAllViewers()
{
    _imp->_gui->redrawAllViewers();
}

void
GuiAppInstance::toggleAutoHideGraphInputs()
{
    _imp->_gui->toggleAutoHideGraphInputs();
}

void
GuiAppInstance::appendToScriptEditor(const std::string& str)
{
    _imp->_gui->appendToScriptEditor(str);
}

void
GuiAppInstance::printAutoDeclaredVariable(const std::string& str)
{
    _imp->_gui->printAutoDeclaredVariable(str);
}

void
GuiAppInstance::setLastViewerUsingTimeline(const boost::shared_ptr<Natron::Node>& node)
{
    if (!node) {
        QMutexLocker k(&_imp->lastTimelineViewerMutex);
        _imp->lastTimelineViewer.reset();
        return;
    }
    if (dynamic_cast<ViewerInstance*>(node->getLiveInstance())) {
        QMutexLocker k(&_imp->lastTimelineViewerMutex);
        _imp->lastTimelineViewer = node;
    }
}

ViewerInstance*
GuiAppInstance::getLastViewerUsingTimeline() const
{
    QMutexLocker k(&_imp->lastTimelineViewerMutex);
    if (!_imp->lastTimelineViewer) {
        return 0;
    }
    return dynamic_cast<ViewerInstance*>(_imp->lastTimelineViewer->getLiveInstance());
}

void
GuiAppInstance::discardLastViewerUsingTimeline()
{

    QMutexLocker k(&_imp->lastTimelineViewerMutex);
    _imp->lastTimelineViewer.reset();
}

void
GuiAppInstance::declareCurrentAppVariable_Python()
{
    std::string appIDStr = getAppIDString();
    /// define the app variable
    std::stringstream ss;
    ss << appIDStr << " = " << NATRON_GUI_PYTHON_MODULE_NAME << ".natron.getGuiInstance(" << getAppID() << ") \n";
    const std::vector<boost::shared_ptr<KnobI> >& knobs = getProject()->getKnobs();
    for (std::vector<boost::shared_ptr<KnobI> >::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        ss << appIDStr << "." << (*it)->getName() << " = "  << appIDStr  << ".getProjectParam('" <<
        (*it)->getName() << "')\n";
    }

    std::string script = ss.str();
    std::string err;
    _imp->declareAppAndParamsString = script;
    bool ok = Natron::interpretPythonScript(script, &err, 0);
    assert(ok);
    if (!ok) {
        throw std::runtime_error("GuiAppInstance::declareCurrentAppVariable_Python() failed!");
    }
}

void
GuiAppInstance::createLoadProjectSplashScreen(const QString& projectFile)
{
    if (_imp->loadProjectSplash) {
        return;
    }
    _imp->loadProjectSplash = new LoadProjectSplashScreen(projectFile);
}

void
GuiAppInstance::updateProjectLoadStatus(const QString& str)
{
    if (!_imp->loadProjectSplash) {
        return;
    }
    _imp->loadProjectSplash->updateText(str);
}

void
GuiAppInstance::closeLoadPRojectSplashScreen()
{
    _imp->loadProjectSplash->deleteLater();
    _imp->loadProjectSplash = 0;
}

void
GuiAppInstance::renderAllViewers()
{
    _imp->_gui->renderAllViewers();
}

void
GuiAppInstance::reloadStylesheet()
{
    if (_imp->_gui) {
        _imp->_gui->reloadStylesheet();
    }
}

void
GuiAppInstance::queueRedrawForAllViewers()
{
    assert(QThread::currentThread() == qApp->thread());
    ++_imp->overlayRedrawRequests;
}

int
GuiAppInstance::getOverlayRedrawRequestsCount() const
{
    assert(QThread::currentThread() == qApp->thread());
    return _imp->overlayRedrawRequests;
}

void
GuiAppInstance::clearOverlayRedrawRequests()
{
    assert(QThread::currentThread() == qApp->thread());
    _imp->overlayRedrawRequests = 0;
}

void
GuiAppInstance::onGroupCreationFinished(const boost::shared_ptr<Natron::Node>& node)
{
    NodeGraph* graph = 0;
    boost::shared_ptr<NodeCollection> collection = node->getGroup();
    assert(collection);
    NodeGroup* isGrp = dynamic_cast<NodeGroup*>(collection.get());
    if (isGrp) {
        NodeGraphI* graph_i = isGrp->getNodeGraph();
        assert(graph_i);
        graph = dynamic_cast<NodeGraph*>(graph_i);
    } else {
        graph = _imp->_gui->getNodeGraph();
    }
    assert(graph);
    std::list<boost::shared_ptr<NodeGui> > selectedNodes = graph->getSelectedNodes();
    boost::shared_ptr<NodeGui> selectedNode;
    if (!selectedNodes.empty()) {
        selectedNode = selectedNodes.front();
        if (dynamic_cast<BackDropGui*>(selectedNode.get())) {
            selectedNode.reset();
        }
    }
    boost::shared_ptr<NodeGuiI> node_gui_i = node->getNodeGui();
    assert(node_gui_i);
    boost::shared_ptr<NodeGui> nodeGui = boost::dynamic_pointer_cast<NodeGui>(node_gui_i);
    graph->moveNodesForIdealPosition(nodeGui, selectedNode, true);

    std::list<ViewerInstance* > viewers;
    node->hasViewersConnected(&viewers);
    for (std::list<ViewerInstance* >::iterator it2 = viewers.begin(); it2 != viewers.end(); ++it2) {
        (*it2)->renderCurrentFrame(false);
    }
    AppInstance::onGroupCreationFinished(node);
}

bool
GuiAppInstance::isDraftRenderEnabled() const
{
    return _imp->_gui ? _imp->_gui->isDraftRenderEnabled() : false;
}

void
GuiAppInstance::setUserIsPainting(const boost::shared_ptr<Natron::Node>& rotopaintNode)
{
    bool wasTurboActive;
    {
        QMutexLocker k(&_imp->userIsPaintingMutex);
        _imp->userIsPainting = rotopaintNode;
        if (rotopaintNode) {
            _imp->turboAlreadyActiveBeforePainting = _imp->_gui->isGUIFrozen();
        }
        wasTurboActive = _imp->turboAlreadyActiveBeforePainting;
    }
    bool isPainting = rotopaintNode.get() != 0;
    _imp->_gui->onFreezeUIButtonClicked(isPainting || wasTurboActive);
}

boost::shared_ptr<Natron::Node>
GuiAppInstance::getIsUserPainting() const
{
    QMutexLocker k(&_imp->userIsPaintingMutex);
    return _imp->userIsPainting;
}

bool
GuiAppInstance::isRenderStatsActionChecked() const
{
    return _imp->_gui->areRenderStatsEnabled();
}


bool
GuiAppInstance::save(const std::string& /*filename*/)
{
    return _imp->_gui->saveProject();
}

bool
GuiAppInstance::saveAs(const std::string& /*filename*/)
{
    return _imp->_gui->saveProjectAs();
}

AppInstance*
GuiAppInstance::loadProject(const std::string& filename)
{
    return _imp->_gui->openProject(filename);
}

///Close the current project but keep the window
bool
GuiAppInstance::resetProject()
{
    return _imp->_gui->abortProject(false);
}

///Reset + close window, quit if last window
bool
GuiAppInstance::closeProject()
{
    return _imp->_gui->abortProject(true);
}

///Opens a new window
AppInstance*
GuiAppInstance::newProject()
{
    return _imp->_gui->createNewProject();
}