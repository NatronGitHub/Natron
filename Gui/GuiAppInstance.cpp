//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
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

#include "Engine/Project.h"
#include "Engine/EffectInstance.h"
#include "Engine/Node.h"
#include "Engine/ProcessHandler.h"
#include "Engine/Settings.h"
#include "Engine/KnobFile.h"
using namespace Natron;

struct GuiAppInstancePrivate {
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
    
    GuiAppInstancePrivate()
    : _gui(NULL)
    , _nodeMapping()
    , _activeBgProcesses()
    , _activeBgProcessesMutex()
    , _isClosing(false)
    , _showingDialog(false)
    , _showingDialogMutex()
    {
    }
};

GuiAppInstance::GuiAppInstance(int appID)
: AppInstance(appID)
, _imp(new GuiAppInstancePrivate)

{
}

void GuiAppInstance::aboutToQuit()
{
    _imp->_isClosing = true;
    _imp->_nodeMapping.clear(); //< necessary otherwise Qt parenting system will try to delete the NodeGui instead of automatic shared_ptr
	_imp->_gui->close();
    _imp->_gui->setParent(NULL);
}

GuiAppInstance::~GuiAppInstance() {
    
//#ifndef __NATRON_WIN32__
    _imp->_gui->getNodeGraph()->discardGuiPointer();
    _imp->_gui->deleteLater();
    _imp->_gui = 0;
    _imp.reset();
//#endif
    QCoreApplication::processEvents();

}

bool GuiAppInstance::isClosing() const
{
    return !_imp || _imp->_isClosing;
}

void GuiAppInstance::load(const QString& projectName,const QStringList& /*writers*/) {
    appPTR->setLoadingStatus("Creating user interface...");
    _imp->_gui = new Gui(this);
    _imp->_gui->createGui();
    
    /// clear the nodes mapping (node<-->nodegui) when the project's node are cleared.
    /// This should go away when we remove that mapping.
    QObject::connect(getProject().get(), SIGNAL(nodesCleared()), this, SLOT(onProjectNodesCleared()));
    
    ///if the app is interactive, build the plugins toolbuttons from the groups we extracted off the plugins.
    const std::vector<PluginGroupNode*>& _toolButtons = appPTR->getPluginsToolButtons();
    for (U32 i = 0; i < _toolButtons.size(); ++i) {
        assert(_toolButtons[i]);
        _imp->_gui->findOrCreateToolButton(_toolButtons[i]);
    }
    emit pluginsPopulated();
    
    ///show the gui
    _imp->_gui->show();
    
    
    if (getAppID() == 0 && appPTR->getCurrentSettings()->isCheckForUpdatesEnabled()) {
        appPTR->setLoadingStatus("Checking if updates are available...");
        ///Before loading autosave check for a new version
        checkForNewVersion();
    }
    
    /// Create auto-save dir if it does not exists
    QDir dir = Natron::Project::autoSavesDir();
    dir.mkpath(".");
    
    bool loadSpecifiedProject = true;
    
    /// If this is the first instance of the software, try to load an autosave
    if(getAppID() == 0){
        if(getProject()->findAndTryLoadAutoSave()){
            ///if we successfully loaded an autosave ignore the specified project in the launch args.
            loadSpecifiedProject = false;
        }
    }
    
    if (loadSpecifiedProject) {
        if (projectName.isEmpty()) {
            ///if the user didn't specify a projects name in the launch args just create a viewer node.
            createNode("Viewer");
        } else {
            ///Otherwise just load the project specified.
            QFileInfo infos(projectName);
            QString name = infos.fileName();
            QString path = infos.filePath();
            appPTR->setLoadingStatus("Loading project: " + path + name);
            getProject()->loadProject(path,name);
        }
        
    }

}

void GuiAppInstance::createNodeGui(boost::shared_ptr<Natron::Node> node,bool loadRequest,bool openImageFileDialog) {
    boost::shared_ptr<NodeGui> nodegui = _imp->_gui->createNodeGUI(node,loadRequest);
    assert(nodegui);
    _imp->_nodeMapping.insert(std::make_pair(node,nodegui));
    
    ///It needs to be here because we rely on the _nodeMapping member
    bool isViewer = node->pluginID() == "Viewer";
    if (isViewer) {
        _imp->_gui->createViewerGui(node);
    }
    
    ///must be done after the viewer gui has been created
    if (node->isRotoNode()) {
        _imp->_gui->createNewRotoInterface(nodegui.get());
    }
    
    
    nodegui->initializeInputs();
    nodegui->initializeKnobs();
    
    if (!loadRequest) {
        nodegui->beginEditKnobs();
    }
    
    ///must be called after initializeKnobs as it populates the node's knobs in the curve editor;
    _imp->_gui->addNodeGuiToCurveEditor(nodegui);

    
    if (!loadRequest) {
        if(_imp->_gui->getSelectedNode()){
            boost::shared_ptr<Node> selected = _imp->_gui->getSelectedNode()->getNode();
            getProject()->autoConnectNodes(selected, node);
        }
        _imp->_gui->selectNode(nodegui);
        
        if (openImageFileDialog) {
            node->getLiveInstance()->openImageFileKnob();
            if (node->isPreviewEnabled()) {
                node->computePreviewImage(getTimeLine()->currentFrame());
            }

        }
    }
    if (!loadRequest && !isViewer) {
        triggerAutoSave();
    }

}

Gui* GuiAppInstance::getGui() const { return _imp->_gui; }


bool GuiAppInstance::shouldRefreshPreview() const {
    return !_imp->_gui->isUserScrubbingTimeline();
}


boost::shared_ptr<NodeGui> GuiAppInstance::getNodeGui(boost::shared_ptr<Node> n) const {
    std::map<boost::shared_ptr<Node>,boost::shared_ptr<NodeGui> >::const_iterator it = _imp->_nodeMapping.find(n);
    if (it == _imp->_nodeMapping.end()) {
        return boost::shared_ptr<NodeGui>();
    } else {
        assert(it->second);
        return it->second;
    }
}

boost::shared_ptr<NodeGui> GuiAppInstance::getNodeGui(const std::string& nodeName) const{
    for(std::map<boost::shared_ptr<Node>,boost::shared_ptr<NodeGui> >::const_iterator it = _imp->_nodeMapping.begin();
        it != _imp->_nodeMapping.end();++it){
        assert(it->first && it->second);
        if(it->first->getName() == nodeName){
            return it->second;
        }
    }
    return boost::shared_ptr<NodeGui>();
}

boost::shared_ptr<Node> GuiAppInstance::getNode(boost::shared_ptr<NodeGui> n) const{
    for (std::map<boost::shared_ptr<Node>,boost::shared_ptr<NodeGui> >::const_iterator it = _imp->_nodeMapping.begin(); it!= _imp->_nodeMapping.end(); ++it) {
        if(it->second == n){
            return it->first;
        }
    }
    return boost::shared_ptr<Node>();
    
}

void GuiAppInstance::deleteNode(const boost::shared_ptr<NodeGui>& n)
{

    assert(!n->getNode()->isActivated());
    if (!isClosing()) {
        getProject()->removeNodeFromProject(n->getNode());
    }
    for (std::map<boost::shared_ptr<Node>,boost::shared_ptr<NodeGui> >::iterator it = _imp->_nodeMapping.begin(); it!= _imp->_nodeMapping.end(); ++it) {
        if(it->second == n){
            _imp->_nodeMapping.erase(it);
            break;
        }
    }
    

}

void GuiAppInstance::errorDialog(const std::string& title,const std::string& message) const {
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

void GuiAppInstance::warningDialog(const std::string& title,const std::string& message) const {
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

void GuiAppInstance::informationDialog(const std::string& title,const std::string& message) const {
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

Natron::StandardButton GuiAppInstance::questionDialog(const std::string& title,const std::string& message,Natron::StandardButtons buttons,
                                                   Natron::StandardButton defaultButton) const {
    {
        QMutexLocker l(&_imp->_showingDialogMutex);
        _imp->_showingDialog = true;
    }
    Natron::StandardButton ret =  _imp->_gui->questionDialog(title, message,buttons,defaultButton);
    {
        QMutexLocker l(&_imp->_showingDialogMutex);
        _imp->_showingDialog = false;
    }

    return ret;
}

bool GuiAppInstance::isShowingDialog() const
{
    QMutexLocker l(&_imp->_showingDialogMutex);
    return _imp->_showingDialog;
}

void GuiAppInstance::loadProjectGui(boost::archive::xml_iarchive& archive) const {
    _imp->_gui->loadProjectGui(archive);
}

void GuiAppInstance::saveProjectGui(boost::archive::xml_oarchive& archive) {
    _imp->_gui->saveProjectGui(archive);
}

void GuiAppInstance::setupViewersForViews(int viewsCount) {
    _imp->_gui->updateViewersViewsMenu(viewsCount);
}

void GuiAppInstance::setViewersCurrentView(int view) {
    _imp->_gui->setViewersCurrentView(view);
}

void GuiAppInstance::startRenderingFullSequence(Natron::OutputEffectInstance* writer) {
    
    /*Start the renderer in a background process.*/
    getProject()->autoSave(); //< takes a snapshot of the graph at this time, this will be the version loaded by the process
    
    
    ///validate the frame range to render
    int firstFrame,lastFrame;
    writer->getFrameRange_public(&firstFrame, &lastFrame);
    //if firstframe and lastframe are infinite clamp them to the timeline bounds
    if (firstFrame == INT_MIN) {
        firstFrame = getTimeLine()->firstFrame();
    }
    if (lastFrame == INT_MAX) {
        lastFrame = getTimeLine()->lastFrame();
    }
    if(firstFrame > lastFrame) {
        Natron::errorDialog(writer->getNode()->getName_mt_safe() ,"First frame in the sequence is greater than the last frame");
        return;
    }
    ///get the output file knob to get the same of the sequence
    QString outputFileSequence;
    const std::vector< boost::shared_ptr<KnobI> >& knobs = writer->getKnobs();
    for (U32 i = 0; i < knobs.size(); ++i) {
        if (knobs[i]->typeName() == OutputFile_Knob::typeNameStatic()) {
            boost::shared_ptr<OutputFile_Knob> fk = boost::dynamic_pointer_cast<OutputFile_Knob>(knobs[i]);
            if(fk->isOutputImageFile()){
                outputFileSequence = fk->getValue().c_str();
            }
        }
    }

    
    if (appPTR->getCurrentSettings()->isRenderInSeparatedProcessEnabled()) {
        try {
            boost::shared_ptr<ProcessHandler> process(new ProcessHandler(this,getProject()->getLastAutoSaveFilePath(),writer));
            QObject::connect(process.get(), SIGNAL(processFinished(int)), this, SLOT(onProcessFinished()));
            notifyRenderProcessHandlerStarted(outputFileSequence,firstFrame,lastFrame,process);

            {
                QMutexLocker l(&_imp->_activeBgProcessesMutex);
                _imp->_activeBgProcesses.push_back(process);
            }
        } catch (const std::exception& e) {
            Natron::errorDialog(writer->getName(), std::string("Error while starting rendering") + ": " + e.what());
        } catch (...) {
            Natron::errorDialog(writer->getName(), std::string("Error while starting rendering"));
        }
    } else {
        writer->renderFullSequence(NULL);
        _imp->_gui->onWriterRenderStarted(outputFileSequence, firstFrame, lastFrame, writer);
    }
}

void GuiAppInstance::onProcessFinished() {
    ProcessHandler* proc = qobject_cast<ProcessHandler*>(sender());
    if (proc) {
        QMutexLocker l(&_imp->_activeBgProcessesMutex);
        for (std::list< boost::shared_ptr<ProcessHandler> >::iterator it = _imp->_activeBgProcesses.begin(); it != _imp->_activeBgProcesses.end();++it) {
            if ((*it).get() == proc) {
                _imp->_activeBgProcesses.erase(it);
                return;
            }
        }
    }
}

void GuiAppInstance::onProjectNodesCleared() {
    _imp->_nodeMapping.clear();
}

void GuiAppInstance::notifyRenderProcessHandlerStarted(const QString& sequenceName,
                                       int firstFrame,int lastFrame,
                                       const boost::shared_ptr<ProcessHandler>& process) {
    _imp->_gui->onProcessHandlerStarted(sequenceName,firstFrame,lastFrame,process);

}

void GuiAppInstance::setUndoRedoStackLimit(int limit) {
    _imp->_gui->setUndoRedoStackLimit(limit);
}

void GuiAppInstance::startProgress(Natron::EffectInstance* effect,const std::string& message)
{
    {
        QMutexLocker l(&_imp->_showingDialogMutex);
        _imp->_showingDialog = true;
    }

    _imp->_gui->startProgress(effect, message);

}

void GuiAppInstance::endProgress(Natron::EffectInstance* effect)
{
    _imp->_gui->endProgress(effect);
    {
        QMutexLocker l(&_imp->_showingDialogMutex);
        _imp->_showingDialog = false;
    }

}

bool GuiAppInstance::progressUpdate(Natron::EffectInstance* effect,double t)
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