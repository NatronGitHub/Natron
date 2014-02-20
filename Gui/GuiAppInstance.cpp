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

#include "Gui/GuiApplicationManager.h"
#include "Gui/Gui.h"
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
    std::map<Natron::Node*,NodeGui*> _nodeMapping; //< a mapping between all nodes and their respective gui. FIXME: it should go away.

    GuiAppInstancePrivate()
    : _gui(NULL)
    , _nodeMapping()
    {
    }
};

GuiAppInstance::GuiAppInstance(int appID)
: AppInstance(appID)
, _imp(new GuiAppInstancePrivate)

{
}

GuiAppInstance::~GuiAppInstance() {
    
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

void GuiAppInstance::createNodeGui(Natron::Node *node,bool loadRequest,bool openImageFileDialog) {
    NodeGui* nodegui = _imp->_gui->createNodeGUI(node);
    assert(nodegui);
    _imp->_nodeMapping.insert(std::make_pair(node,nodegui));
    
    if (node->pluginID() == "Viewer") {
        _imp->_gui->createViewerGui(node);
    }
    
    nodegui->initializeInputs();
    nodegui->initializeKnobs();
    
    ///must be called after initializeKnobs as it populates the node's knobs in the curve editor;
    _imp->_gui->addNodeGuiToCurveEditor(nodegui);

    
    if (!loadRequest) {
        if(_imp->_gui->getSelectedNode()){
            Node* selected = _imp->_gui->getSelectedNode()->getNode();
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
}

Gui* GuiAppInstance::getGui() const { return _imp->_gui; }


bool GuiAppInstance::shouldRefreshPreview() const {
    return !_imp->_gui->isUserScrubbingTimeline();
}


NodeGui* GuiAppInstance::getNodeGui(Node* n) const {
    std::map<Node*,NodeGui*>::const_iterator it = _imp->_nodeMapping.find(n);
    if (it == _imp->_nodeMapping.end()) {
        return NULL;
    } else {
        assert(it->second);
        return it->second;
    }
}

NodeGui* GuiAppInstance::getNodeGui(const std::string& nodeName) const{
    for(std::map<Node*,NodeGui*>::const_iterator it = _imp->_nodeMapping.begin();
        it != _imp->_nodeMapping.end();++it){
        assert(it->first && it->second);
        if(it->first->getName() == nodeName){
            return it->second;
        }
    }
    return (NodeGui*)NULL;
}

Node* GuiAppInstance::getNode(NodeGui* n) const{
    for (std::map<Node*,NodeGui*>::const_iterator it = _imp->_nodeMapping.begin(); it!= _imp->_nodeMapping.end(); ++it) {
        if(it->second == n){
            return it->first;
        }
    }
    return NULL;
    
}

void GuiAppInstance::errorDialog(const std::string& title,const std::string& message) const {
    _imp->_gui->errorDialog(title, message);
}

void GuiAppInstance::warningDialog(const std::string& title,const std::string& message) const {
    _imp->_gui->warningDialog(title, message);
    
}

void GuiAppInstance::informationDialog(const std::string& title,const std::string& message) const {
    _imp->_gui->informationDialog(title, message);
}

Natron::StandardButton GuiAppInstance::questionDialog(const std::string& title,const std::string& message,Natron::StandardButtons buttons,
                                                   Natron::StandardButton defaultButton) const {
    return _imp->_gui->questionDialog(title, message,buttons,defaultButton);
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
    writer->getFrameRange(&firstFrame, &lastFrame);
    if(firstFrame > lastFrame) {
        throw std::invalid_argument("First frame in the sequence is greater than the last frame");
    }
    ///get the output file knob to get the same of the sequence
    QString outputFileSequence;
    const std::vector< boost::shared_ptr<Knob> >& knobs = writer->getKnobs();
    for (U32 i = 0; i < knobs.size(); ++i) {
        if (knobs[i]->typeName() == OutputFile_Knob::typeNameStatic()) {
            boost::shared_ptr<OutputFile_Knob> fk = boost::dynamic_pointer_cast<OutputFile_Knob>(knobs[i]);
            if(fk->isOutputImageFile()){
                outputFileSequence = fk->getValue().toString();
            }
        }
    }

    
    if (appPTR->getCurrentSettings()->isRenderInSeparatedProcessEnabled()) {
        ProcessHandler* newProcess = 0;
        try {
            newProcess = new ProcessHandler(this,getProject()->getLastAutoSaveFilePath(),outputFileSequence,firstFrame,lastFrame ,writer); //< the process will delete itself
        } catch (const std::exception& e) {
            Natron::errorDialog(writer->getName(), std::string("Error while starting rendering") + ": " + e.what());
            delete newProcess;
        } catch (...) {
            Natron::errorDialog(writer->getName(), std::string("Error while starting rendering"));
            delete newProcess;
        }
    } else {
        writer->renderFullSequence(NULL);
        _imp->_gui->onWriterRenderStarted(outputFileSequence, firstFrame, lastFrame, writer);
    }
}

void GuiAppInstance::onProjectNodesCleared() {
    _imp->_nodeMapping.clear();
}

void GuiAppInstance::notifyRenderProcessHandlerStarted(const QString& sequenceName,
                                       int firstFrame,int lastFrame,
                                       ProcessHandler* process) {
    _imp->_gui->onProcessHandlerStarter(sequenceName,firstFrame,lastFrame,process);

}