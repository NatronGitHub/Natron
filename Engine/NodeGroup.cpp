//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#include "NodeGroup.h"

#include <set>
#include <locale>
#include <cfloat>
#include <QThreadPool>
#include <QCoreApplication>
#include <QTextStream>

#include "Engine/AppInstance.h"
#include "Engine/Node.h"
#include "Engine/OutputSchedulerThread.h"
#include "Engine/Project.h"
#include "Engine/KnobTypes.h"
#include "Engine/KnobFile.h"
#include "Engine/TimeLine.h"
#include "Engine/NoOp.h"
#include "Engine/ViewerInstance.h"
#include "Engine/Plugin.h"
#include "Engine/NodeGuiI.h"
#include "Engine/Curve.h"
#include "Engine/RotoContext.h"
using namespace Natron;

struct NodeCollectionPrivate
{
    AppInstance* app;
    
    NodeGraphI* graph;
    
    mutable QMutex nodesMutex;
    NodeList nodes;
    
    NodeCollectionPrivate(AppInstance* app)
    : app(app)
    , graph(0)
    , nodesMutex()
    , nodes()
    {
        
    }
    
    NodePtr findNodeInternal(const std::string& name,const std::string& recurseName) const;
};

NodeCollection::NodeCollection(AppInstance* app)
: _imp(new NodeCollectionPrivate(app))
{
    
}

NodeCollection::~NodeCollection()
{
    
}

AppInstance*
NodeCollection::getApplication() const
{
    return _imp->app;
}

void
NodeCollection::setNodeGraphPointer(NodeGraphI* graph)
{
    _imp->graph = graph;
}

void
NodeCollection::discardNodeGraphPointer()
{
    _imp->graph = 0;
}

NodeGraphI*
NodeCollection::getNodeGraph() const
{
    return _imp->graph;
}

NodeList
NodeCollection::getNodes() const
{
    QMutexLocker k(&_imp->nodesMutex);
    return _imp->nodes;
}

void
NodeCollection::addNode(const NodePtr& node)
{
    {
        QMutexLocker k(&_imp->nodesMutex);
        _imp->nodes.push_back(node);
    }
}


void
NodeCollection::removeNode(const NodePtr& node)
{
    QMutexLocker k(&_imp->nodesMutex);
    NodeList::iterator found = std::find(_imp->nodes.begin(), _imp->nodes.end(), node);
    if (found != _imp->nodes.end()) {
        _imp->nodes.erase(found);
    }
}

NodePtr
NodeCollection::getLastNode(const std::string& pluginID) const
{
    QMutexLocker k(&_imp->nodesMutex);
    for (NodeList::const_reverse_iterator it = _imp->nodes.rbegin(); it != _imp->nodes.rend(); ++it ) {
        if ((*it)->getPluginID() == pluginID) {
            return *it;
        }
    }
    return NodePtr();
}

bool
NodeCollection::hasNodes() const
{
    QMutexLocker k(&_imp->nodesMutex);
    return _imp->nodes.size() > 0;
}

void
NodeCollection::getActiveNodes(NodeList* nodes) const
{
    QMutexLocker k(&_imp->nodesMutex);
    for (NodeList::iterator it = _imp->nodes.begin(); it != _imp->nodes.end(); ++it) {
        if ((*it)->isActivated()) {
            nodes->push_back(*it);
        }
    }
}

void
NodeCollection::getActiveNodesExpandGroups(NodeList* nodes) const
{
    QMutexLocker k(&_imp->nodesMutex);
    for (NodeList::iterator it = _imp->nodes.begin(); it != _imp->nodes.end(); ++it) {
        if ((*it)->isActivated()) {
            nodes->push_back(*it);
            NodeGroup* isGrp = dynamic_cast<NodeGroup*>((*it)->getLiveInstance());
            if (isGrp) {
                isGrp->getActiveNodesExpandGroups(nodes);
            }
        }
    }
}

void
NodeCollection::getViewers(std::list<ViewerInstance*>* viewers) const
{
    QMutexLocker k(&_imp->nodesMutex);
    for (NodeList::iterator it = _imp->nodes.begin(); it != _imp->nodes.end(); ++it) {
        ViewerInstance* isViewer = dynamic_cast<ViewerInstance*>((*it)->getLiveInstance());
        if (isViewer) {
            viewers->push_back(isViewer);
        }
        NodeGroup* isGrp = dynamic_cast<NodeGroup*>((*it)->getLiveInstance());
        if (isGrp) {
            isGrp->getViewers(viewers);
        }
    }
}

void
NodeCollection::quitAnyProcessingForAllNodes()
{
    NodeList nodes = getNodes();
    for (NodeList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        (*it)->quitAnyProcessing();
        NodeGroup* isGrp = dynamic_cast<NodeGroup*>((*it)->getLiveInstance());
        if (isGrp) {
            isGrp->quitAnyProcessingForAllNodes();
        }
    }
}

bool
NodeCollection::hasNodeRendering() const
{
    QMutexLocker k(&_imp->nodesMutex);
    for (NodeList::iterator it = _imp->nodes.begin(); it != _imp->nodes.end(); ++it) {
        if ( (*it)->isOutputNode() ) {
            Natron::OutputEffectInstance* effect = dynamic_cast<Natron::OutputEffectInstance*>( (*it)->getLiveInstance() );
            assert(effect);
            NodeGroup* isGrp = dynamic_cast<NodeGroup*>(effect);
            if (isGrp) {
                if (isGrp->hasNodeRendering()) {
                    return true;
                }
            } else {
                if ( effect->getRenderEngine()->hasThreadsWorking() ) {
                    return true;
                    break;
                }
            }
        }
    }
    return false;
}

void
NodeCollection::refreshViewersAndPreviews()
{
    assert(QThread::currentThread() == qApp->thread());
    
    if ( !appPTR->isBackground() ) {
        int time = _imp->app->getTimeLine()->currentFrame();
        
        QMutexLocker k(&_imp->nodesMutex);
        for (NodeList::iterator it = _imp->nodes.begin(); it != _imp->nodes.end(); ++it) {
            assert(*it);
            (*it)->computePreviewImage(time);
            NodeGroup* isGrp = dynamic_cast<NodeGroup*>((*it)->getLiveInstance());
            if (isGrp) {
                isGrp->refreshViewersAndPreviews();
            } else {
                ViewerInstance* n = dynamic_cast<ViewerInstance*>((*it)->getLiveInstance());
                if (n) {
                    n->getRenderEngine()->renderCurrentFrame(true);
                }
            }
        }
    }
}

void
NodeCollection::refreshPreviews()
{
    int time = _imp->app->getTimeLine()->currentFrame();
    NodeList nodes;
    getActiveNodes(&nodes);
    for (NodeList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        if ( (*it)->isPreviewEnabled() ) {
            (*it)->refreshPreviewImage(time);
        }
        NodeGroup* isGrp = dynamic_cast<NodeGroup*>((*it)->getLiveInstance());
        if (isGrp) {
            isGrp->refreshPreviews();
        }
    }
}

void
NodeCollection::forceRefreshPreviews()
{
    int time = _imp->app->getTimeLine()->currentFrame();
    NodeList nodes;
    getActiveNodes(&nodes);
    for (NodeList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        if ( (*it)->isPreviewEnabled() ) {
            (*it)->computePreviewImage(time);
        }
        NodeGroup* isGrp = dynamic_cast<NodeGroup*>((*it)->getLiveInstance());
        if (isGrp) {
            isGrp->forceRefreshPreviews();
        }
    }
}


void
NodeCollection::clearNodes(bool emitSignal)
{
    
    NodeList nodesToDelete;
    {
        QMutexLocker l(&_imp->nodesMutex);
        nodesToDelete = _imp->nodes;
    }
    
    ///First quit any processing
    for (NodeList::iterator it = nodesToDelete.begin(); it != nodesToDelete.end(); ++it) {
        NodeGroup* isGrp = dynamic_cast<NodeGroup*>((*it)->getLiveInstance());
        if (isGrp) {
            isGrp->clearNodes(emitSignal);
        }
        (*it)->quitAnyProcessing();
    }
    
    ///Kill thread pool so threads are killed before killing thread storage
    QThreadPool::globalInstance()->waitForDone();
    
    
    ///Kill effects
    for (NodeList::iterator it = nodesToDelete.begin(); it != nodesToDelete.end(); ++it) {
        (*it)->deactivate(std::list<Natron::Node* >(),false,false,true,false);
    }
    
    for (NodeList::iterator it = nodesToDelete.begin(); it != nodesToDelete.end(); ++it) {
        (*it)->removeReferences();
    }
    
    
    if (emitSignal) {
        Q_EMIT nodesCleared();
    }
    
    {
        QMutexLocker l(&_imp->nodesMutex);
        _imp->nodes.clear();
    }
    
    nodesToDelete.clear();
}

void
NodeCollection::initNodeName(const std::string& pluginLabel,std::string* nodeName)
{
    int no = 1;
    std::string baseName(pluginLabel);
    if (baseName.size() > 3 &&
        baseName[baseName.size() - 1] == 'X' &&
        baseName[baseName.size() - 2] == 'F' &&
        baseName[baseName.size() - 3] == 'O') {
        baseName = baseName.substr(0,baseName.size() - 3);
    }
    bool foundNodeWithName = false;
    
    {
        std::stringstream ss;
        ss << baseName << no;
        *nodeName = ss.str();
    }
    do {
        foundNodeWithName = false;
        QMutexLocker l(&_imp->nodesMutex);
        for (NodeList::iterator it = _imp->nodes.begin(); it != _imp->nodes.end(); ++it) {
            if ((*it)->getName_mt_safe() == *nodeName) {
                foundNodeWithName = true;
                break;
            }
        }
        if (foundNodeWithName) {
            ++no;
            {
                std::stringstream ss;
                ss << baseName << no;
                *nodeName = ss.str();
            }
        }
    } while (foundNodeWithName);
}

bool
NodeCollection::connectNodes(int inputNumber,const NodePtr& input,Natron::Node* output,bool force)
{
    ////Only called by the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    
    NodePtr existingInput = output->getInput(inputNumber);
    if (force && existingInput) {
        bool ok = disconnectNodes(existingInput.get(), output);
        assert(ok);
        if (input->getMaxInputCount() > 0) {
            ok = connectNodes(input->getPreferredInputForConnection(), existingInput, input.get());
            assert(ok);
        }
    }
    
    if (!input) {
        return true;
    }
    
    
    if ( !output->connectInput(input, inputNumber) ) {
        return false;
    }
    
    return true;
}


bool
NodeCollection::connectNodes(int inputNumber,const std::string & inputName,Natron::Node* output)
{
    NodeList nodes = getNodes();
    
    for (NodeList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        assert(*it);
        if ((*it)->getName() == inputName) {
            return connectNodes(inputNumber,*it, output);
        }
    }
    
    return false;
}


bool
NodeCollection::disconnectNodes(Natron::Node* input,Natron::Node* output,bool autoReconnect)
{
    NodePtr inputToReconnectTo;
    int indexOfInput = output->inputIndex( input );
    
    if (indexOfInput == -1) {
        return false;
    }
    
    int inputsCount = input->getMaxInputCount();
    if (inputsCount == 1) {
        inputToReconnectTo = input->getInput(0);
    }
    
    
    if (output->disconnectInput(input) < 0) {
        return false;
    }
    
    if (autoReconnect && inputToReconnectTo) {
        connectNodes(indexOfInput, inputToReconnectTo, output);
    }
    
    return true;
}


bool
NodeCollection::autoConnectNodes(const NodePtr& selected,const NodePtr& created)
{
    ///We follow this rule:
    //        1) selected is output
    //          a) created is output --> fail
    //          b) created is input --> connect input
    //          c) created is regular --> connect input
    //        2) selected is input
    //          a) created is output --> connect output
    //          b) created is input --> fail
    //          c) created is regular --> connect output
    //        3) selected is regular
    //          a) created is output--> connect output
    //          b) created is input --> connect input
    //          c) created is regular --> connect output
    
    ///if true if will connect 'created' as input of 'selected',
    ///otherwise as output.
    bool connectAsInput = false;
    
    ///cannot connect 2 input nodes together: case 2-b)
    if ( (selected->getMaxInputCount() == 0) && (created->getMaxInputCount() == 0) ) {
        return false;
    }
    ///cannot connect 2 output nodes together: case 1-a)
    if ( selected->isOutputNode() && created->isOutputNode() ) {
        return false;
    }
    
    ///1)
    if ( selected->isOutputNode() ) {
        ///assert we're not in 1-a)
        assert( !created->isOutputNode() );
        
        ///for either cases 1-b) or 1-c) we just connect the created node as input of the selected node.
        connectAsInput = true;
    }
    ///2) and 3) are similar exceptfor case b)
    else {
        ///case 2 or 3- a): connect the created node as output of the selected node.
        if ( created->isOutputNode() ) {
            connectAsInput = false;
        }
        ///case b)
        else if (created->getMaxInputCount() == 0) {
            assert(selected->getMaxInputCount() != 0);
            ///case 3-b): connect the created node as input of the selected node
            connectAsInput = true;
        }
        ///case c) connect created as output of the selected node
        else {
            connectAsInput = false;
        }
    }
    
    bool ret = false;
    if (connectAsInput) {
        
        ///connect it to the first input
        int selectedInput = selected->getPreferredInputForConnection();
        if (selectedInput != -1) {
            bool ok = connectNodes(selectedInput, created, selected.get(),true);
            assert(ok);
            ret = true;
        } else {
            ret = false;
        }
        
    } else {
        if ( !created->isOutputNode() ) {
            ///we find all the nodes that were previously connected to the selected node,
            ///and connect them to the created node instead.
            std::map<Node*,int> outputsConnectedToSelectedNode;
            selected->getOutputsConnectedToThisNode(&outputsConnectedToSelectedNode);
            for (std::map<Node*,int>::iterator it = outputsConnectedToSelectedNode.begin();
                 it != outputsConnectedToSelectedNode.end(); ++it) {
                if (it->first->getParentMultiInstanceName().empty()) {
                    bool ok = disconnectNodes(selected.get(), it->first);
                    assert(ok);
                    
                    (void)connectNodes(it->second, created, it->first);
                    //assert(ok); Might not be ok if the disconnectNodes() action above was queued
                }
            }
        }
        ///finally we connect the created node to the selected node
        int createdInput = created->getPreferredInputForConnection();
        if (createdInput != -1) {
            bool ok = connectNodes(createdInput, selected, created.get());
            assert(ok);
            ret = true;
        } else {
            ret = false;
        }
    }
    
    ///update the render trees
    std::list<ViewerInstance* > viewers;
    created->hasViewersConnected(&viewers);
    for (std::list<ViewerInstance* >::iterator it = viewers.begin(); it != viewers.end(); ++it) {
        (*it)->renderCurrentFrame(true);
    }
    
    return ret;
 
} // autoConnectNodes

NodePtr
NodeCollectionPrivate::findNodeInternal(const std::string& name,const std::string& recurseName) const
{
    QMutexLocker k(&nodesMutex);
    for (NodeList::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        if ((*it)->getName_mt_safe() == name) {
            if (!recurseName.empty()) {
                NodeGroup* isGrp = dynamic_cast<NodeGroup*>((*it)->getLiveInstance());
                assert(isGrp);
                return isGrp->getNodeByFullySpecifiedName(recurseName);
            }
            return *it;
        }
    }
    return NodePtr();

}

NodePtr
NodeCollection::getNodeByName(const std::string & name) const
{
    return _imp->findNodeInternal(name,std::string());
}

void
NodeCollection::getNodeNameAndRemainder_LeftToRight(const std::string& fullySpecifiedName,std::string& name,std::string& remainder)
{
    std::size_t foundDot = fullySpecifiedName.find_first_of('.');
    
    if (foundDot != std::string::npos) {
        
        name = fullySpecifiedName.substr(0, foundDot);
        if (foundDot + 1 < fullySpecifiedName.size()) {
            remainder = fullySpecifiedName.substr(foundDot + 1, std::string::npos);
        }
        
    } else {
        name = fullySpecifiedName;
    }
    
}

void
NodeCollection::getNodeNameAndRemainder_RightToLeft(const std::string& fullySpecifiedName,std::string& name,std::string& remainder)
{
    std::size_t foundDot = fullySpecifiedName.find_last_of('.');
    if (foundDot != std::string::npos) {
        
        name = fullySpecifiedName.substr(foundDot + 1, std::string::npos);
        if (foundDot > 0) {
            remainder = fullySpecifiedName.substr(0, foundDot - 1);
        }
        
    } else {
        name = fullySpecifiedName;
    }

}


NodePtr
NodeCollection::getNodeByFullySpecifiedName(const std::string& fullySpecifiedName) const
{
    
    std::string toFind;
    std::string recurseName;
    getNodeNameAndRemainder_LeftToRight(fullySpecifiedName, toFind, recurseName);
   return _imp->findNodeInternal(toFind,recurseName);
   
}


void
NodeCollection::setAllNodesAborted(bool aborted)
{
    QMutexLocker k(&_imp->nodesMutex);
    for (NodeList::iterator it = _imp->nodes.begin(); it != _imp->nodes.end(); ++it) {
        (*it)->setAborted(aborted);
        NodeGroup* isGrp = dynamic_cast<NodeGroup*>((*it)->getLiveInstance());
        if (isGrp) {
            isGrp->setAllNodesAborted(aborted);
        }
    }
}


void
NodeCollection::fixRelativeFilePaths(const std::string& projectPathName,const std::string& newProjectPath,bool blockEval)
{
    NodeList nodes = getNodes();
    
    boost::shared_ptr<Natron::Project> project = _imp->app->getProject();
    for (NodeList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        if ((*it)->isActivated()) {
            if (blockEval) {
                (*it)->getLiveInstance()->blockEvaluation();
            }
            const std::vector<boost::shared_ptr<KnobI> >& knobs = (*it)->getKnobs();
            for (U32 j = 0; j < knobs.size(); ++j) {
                
                Knob<std::string>* isString = dynamic_cast< Knob<std::string>* >(knobs[j].get());
                String_Knob* isStringKnob = dynamic_cast<String_Knob*>(isString);
                if (!isString || isStringKnob || knobs[j] == project->getEnvVarKnob()) {
                    continue;
                }
                
                std::string filepath = isString->getValue();
                
                if (!filepath.empty()) {
                    if (project->fixFilePath(projectPathName, newProjectPath, filepath)) {
                        isString->setValue(filepath, 0);
                    }
                }
            }
            if (blockEval) {
                (*it)->getLiveInstance()->unblockEvaluation();
            }
            
            NodeGroup* isGrp = dynamic_cast<NodeGroup*>((*it)->getLiveInstance());
            if (isGrp) {
                isGrp->fixRelativeFilePaths(projectPathName, newProjectPath, blockEval);
            }
            
        }
    }
    
}

void
NodeCollection::fixPathName(const std::string& oldName,const std::string& newName)
{
    NodeList nodes = getNodes();
    boost::shared_ptr<Natron::Project> project = _imp->app->getProject();
    for (NodeList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        if ((*it)->isActivated()) {
            const std::vector<boost::shared_ptr<KnobI> >& knobs = (*it)->getKnobs();
            for (U32 j = 0; j < knobs.size(); ++j) {
                
                Knob<std::string>* isString = dynamic_cast< Knob<std::string>* >(knobs[j].get());
                String_Knob* isStringKnob = dynamic_cast<String_Knob*>(isString);
                if (!isString || isStringKnob || knobs[j] == project->getEnvVarKnob()) {
                    continue;
                }
                
                std::string filepath = isString->getValue();
                
                if (filepath.size() >= (oldName.size() + 2) &&
                    filepath[0] == '[' &&
                    filepath[oldName.size() + 1] == ']' &&
                    filepath.substr(1,oldName.size()) == oldName) {
                    
                    filepath.replace(1, oldName.size(), newName);
                    isString->setValue(filepath, 0);
                }
            }
            
            NodeGroup* isGrp = dynamic_cast<NodeGroup*>((*it)->getLiveInstance());
            if (isGrp) {
                isGrp->fixPathName(oldName, newName);
            }
            
        }
    }
    
}

bool
NodeCollection::checkIfNodeNameExists(const std::string & n,const Natron::Node* caller) const
{
    QMutexLocker k(&_imp->nodesMutex);
    for (NodeList::const_iterator it = _imp->nodes.begin(); it != _imp->nodes.end(); ++it) {
        if ( (it->get() != caller) && ( (*it)->getName_mt_safe() == n ) ) {
            return true;
        }
    }
    
    return false;
}



struct NodeGroupPrivate
{
    std::vector<boost::weak_ptr<Node> > inputs;
    boost::weak_ptr<Node> output;
    
    boost::shared_ptr<Button_Knob> exportAsTemplate;
    
    NodeGroupPrivate()
    : inputs()
    , output()
    , exportAsTemplate()
    {
        
    }
};

NodeGroup::NodeGroup(const NodePtr &node)
: OutputEffectInstance(node)
, NodeCollection(node ? node->getApp() : 0)
, _imp(new NodeGroupPrivate())
{
    setSupportsRenderScaleMaybe(EffectInstance::eSupportsYes);
}


NodeGroup::~NodeGroup()
{
    
}

std::string
NodeGroup::getDescription() const
{
    return "Use this to nest multiple nodes into a single node. The original nodes will be replaced by the Group node and its "
    "content is available in a separate NodeGraph tab. You can add user parameters to the Group node which can drive "
    "parameters of nodes nested within the Group. To specify the outputs and inputs of the Group node, you may add multiple "
    "Input node within the group and exactly 1 Output node.";
}

void
NodeGroup::addAcceptedComponents(int /*inputNb*/,
                                std::list<Natron::ImageComponentsEnum>* comps)
{
    comps->push_back(Natron::eImageComponentRGB);
    comps->push_back(Natron::eImageComponentRGBA);
    comps->push_back(Natron::eImageComponentAlpha);
}

void
NodeGroup::addSupportedBitDepth(std::list<Natron::ImageBitDepthEnum>* depths) const
{
    depths->push_back(Natron::eImageBitDepthByte);
    depths->push_back(Natron::eImageBitDepthShort);
    depths->push_back(Natron::eImageBitDepthFloat);
}

int
NodeGroup::getMaxInputCount() const
{
    return (int)_imp->inputs.size();
}

std::string
NodeGroup::getInputLabel(int inputNb) const
{
    if (inputNb >= (int)_imp->inputs.size() || inputNb < 0) {
        return std::string();
    }
    
    ///If the input name starts with "input" remove it, otherwise keep the full name
    NodePtr input = _imp->inputs[inputNb].lock();
    if (!input) {
        return std::string();
    }
    QString inputName(input->getName_mt_safe().c_str());
    if (inputName.startsWith("input",Qt::CaseInsensitive)) {
        inputName.remove(0, 5);
    }
    return inputName.toStdString();
}

bool
NodeGroup::isInputOptional(int inputNb) const
{
    if (inputNb >= (int)_imp->inputs.size() || inputNb < 0) {
        return false;
    }
    
    NodePtr n = _imp->inputs[inputNb].lock();
    if (!n) {
        return false;
    }
    GroupInput* input = dynamic_cast<GroupInput*>(n->getLiveInstance());
    assert(input);
    boost::shared_ptr<KnobI> knob = input->getKnobByName("optional");
    assert(knob);
    Bool_Knob* isBool = dynamic_cast<Bool_Knob*>(knob.get());
    assert(isBool);
    return isBool->getValue();
}

bool
NodeGroup::isInputMask(int inputNb) const
{
    
    if (inputNb >= (int)_imp->inputs.size() || inputNb < 0) {
        return false;
    }
    
    NodePtr n = _imp->inputs[inputNb].lock();
    if (!n) {
        return false;
    }
    GroupInput* input = dynamic_cast<GroupInput*>(n->getLiveInstance());
    assert(input);
    boost::shared_ptr<KnobI> knob = input->getKnobByName("isMask");
    assert(knob);
    Bool_Knob* isBool = dynamic_cast<Bool_Knob*>(knob.get());
    assert(isBool);
    return isBool->getValue();
}

void
NodeGroup::initializeKnobs()
{
    boost::shared_ptr<KnobI> nodePage = getKnobByName(NATRON_EXTRA_PARAMETER_PAGE_NAME);
    assert(nodePage);
    Page_Knob* isPage = dynamic_cast<Page_Knob*>(nodePage.get());
    assert(isPage);
    _imp->exportAsTemplate = Natron::createKnob<Button_Knob>(this, "Export as Python plug-in");
    _imp->exportAsTemplate->setName("exportAsGroup");
    _imp->exportAsTemplate->setHintToolTip("Export this group as a Python group script that can be shared and/or later "
                                           "on re-used as a plug-in.");
    isPage->addKnob(_imp->exportAsTemplate);
}

void
NodeGroup::notifyNodeDeactivated(const boost::shared_ptr<Natron::Node>& node)
{
    NodePtr thisNode = getNode();
    GroupInput* isInput = dynamic_cast<GroupInput*>(node->getLiveInstance());
    if (isInput) {
        for (U32 i = 0; i < _imp->inputs.size(); ++i) {
            NodePtr input = _imp->inputs[i].lock();
            if (node == input) {
                
                ///Also disconnect the real input
                thisNode->disconnectInput(i);
                
                _imp->inputs.erase(_imp->inputs.begin() + i);
                thisNode->initializeInputs();
                return;
            }
        }
        ///The input must have been tracked before
        assert(false);
    }
    GroupOutput* isOutput = dynamic_cast<GroupOutput*>(node->getLiveInstance());
    if (isOutput) {
        _imp->output.reset();
    }
    
    ///Notify outputs of the group nodes that their inputs may have changed
    const std::list<Natron::Node*>& outputs = thisNode->getOutputs();
    for (std::list<Natron::Node*>::const_iterator it = outputs.begin(); it != outputs.end(); ++it) {
        int idx = (*it)->getInputIndex(thisNode.get());
        assert(idx != -1);
        (*it)->onInputChanged(idx);
    }

}

void
NodeGroup::notifyNodeActivated(const boost::shared_ptr<Natron::Node>& node)
{
    NodePtr thisNode = getNode();

    GroupInput* isInput = dynamic_cast<GroupInput*>(node->getLiveInstance());
    if (isInput) {
        _imp->inputs.push_back(node);
        thisNode->initializeInputs();
    }
    GroupOutput* isOutput = dynamic_cast<GroupOutput*>(node->getLiveInstance());
    if (isOutput) {
        _imp->output = node;
    }
    
    ///Notify outputs of the group nodes that their inputs may have changed
    const std::list<Natron::Node*>& outputs = thisNode->getOutputs();
    for (std::list<Natron::Node*>::const_iterator it = outputs.begin(); it != outputs.end(); ++it) {
        int idx = (*it)->getInputIndex(thisNode.get());
        assert(idx != -1);
        (*it)->onInputChanged(idx);
    }
}

void
NodeGroup::notifyInputOptionalStateChanged(const boost::shared_ptr<Natron::Node>& /*node*/)
{
    getNode()->initializeInputs();
}

void
NodeGroup::notifyInputMaskStateChanged(const boost::shared_ptr<Natron::Node>& /*node*/)
{
    getNode()->initializeInputs();
}

void
NodeGroup::notifyNodeNameChanged(const boost::shared_ptr<Natron::Node>& node)
{
    GroupInput* isInput = dynamic_cast<GroupInput*>(node->getLiveInstance());
    if (isInput) {
        getNode()->initializeInputs();
    }
}

boost::shared_ptr<Natron::Node>
NodeGroup::getOutputNode() const
{
    return _imp->output.lock();
}

boost::shared_ptr<Natron::Node>
NodeGroup::getOutputNodeInput() const
{
    NodePtr output = _imp->output.lock();
    if (output) {
        return output->getInput(0);
    }
    return NodePtr();
}

boost::shared_ptr<Natron::Node>
NodeGroup::getRealInputForInput(const boost::shared_ptr<Natron::Node>& input) const
{
    for (U32 i = 0; i < _imp->inputs.size(); ++i) {
        if (_imp->inputs[i].lock() == input) {
            return getNode()->getInput(i);
        }
    }
    return boost::shared_ptr<Natron::Node>();
}

void
NodeGroup::getInputsOutputs(std::list<Natron::Node* >* nodes) const
{
    for (U32 i = 0; i < _imp->inputs.size(); ++i) {
        std::list<Natron::Node*> outputs;
        _imp->inputs[i].lock()->getOutputs_mt_safe(outputs);
        nodes->insert(nodes->end(), outputs.begin(),outputs.end());
    }
}

void
NodeGroup::getInputs(std::vector<boost::shared_ptr<Natron::Node> >* inputs) const
{
    for (U32 i = 0; i < _imp->inputs.size(); ++i) {
        inputs->push_back(_imp->inputs[i].lock());
    }
}

void
NodeGroup::knobChanged(KnobI* k,Natron::ValueChangedReasonEnum /*reason*/,
                 int /*view*/,
                 SequenceTime /*time*/,
                 bool /*originatedFromMainThread*/)
{
    if (k == _imp->exportAsTemplate.get()) {
        boost::shared_ptr<NodeGuiI> gui_i = getNode()->getNodeGui();
        if (gui_i) {
            gui_i->exportGroupAsPythonScript();
        }
    }
}

static bool isCharToEscape(const QChar& c)
{
    return c == '\\' || c == '"' || c == '\'';
}

static QString escapeString(const QString& str)
{
    QString ret = str;
    for (int i = 0; i < ret.size(); ++i) {
        if ((i == 0 && isCharToEscape(ret[i])) ||
            (i > 0 && isCharToEscape(ret[i]) && ret[i - 1] != '\\')) {
            ret.insert(i, QChar('\\'));
        }
    }
    ret.prepend('"');
    ret.append('"');
    return ret;
}

static QString escapeString(const std::string& str)
{
    QString s(str.c_str());
    return escapeString(s);
}

#define ESC(s) escapeString(s)

#define WRITE_STATIC_LINE(line) ts << line "\n"
#define WRITE_INDENT(x) \
        for (int _i = 0; _i < x; ++_i) ts << "    ";
#define WRITE_STRING(str) ts << str << "\n"
#define NUM(n) QString::number(n)

static bool exportKnobValues(const boost::shared_ptr<KnobI> knob,
                             const QString& paramFullName,
                             bool mustDefineParam,
                             QTextStream& ts)
{
    
    bool hasExportedValue = false;
    
    Knob<std::string>* isStr = dynamic_cast<Knob<std::string>*>(knob.get());
    AnimatingString_KnobHelper* isAnimatedStr = dynamic_cast<AnimatingString_KnobHelper*>(knob.get());
    Knob<double>* isDouble = dynamic_cast<Knob<double>*>(knob.get());
    Knob<int>* isInt = dynamic_cast<Knob<int>*>(knob.get());
    Knob<bool>* isBool = dynamic_cast<Knob<bool>*>(knob.get());
    Parametric_Knob* isParametric = dynamic_cast<Parametric_Knob*>(knob.get());
    Choice_Knob* isChoice = dynamic_cast<Choice_Knob*>(knob.get());
    Group_Knob* isGrp = dynamic_cast<Group_Knob*>(knob.get());
    String_Knob* isStringKnob = dynamic_cast<String_Knob*>(knob.get());
    
    ///Don't export this kind of parameter. Mainly this is the html label of the node which is 99% of times empty
    if (isStringKnob &&
        isStringKnob->isMultiLine() &&
        isStringKnob->usesRichText() &&
        !isStringKnob->hasContentWithoutHtmlTags() &&
        !isStringKnob->isAnimationEnabled() &&
        isStringKnob->getExpression(0).empty()) {
        return false;
    }
    for (int i = 0; i < knob->getDimension(); ++i) {
        
        if (isParametric) {
            
            if (!hasExportedValue) {
                hasExportedValue = true;
                if (mustDefineParam) {
                    WRITE_INDENT(1); WRITE_STRING("param = " + paramFullName);
                }
            }
            boost::shared_ptr<Curve> curve = isParametric->getParametricCurve(i);
            double r,g,b;
            isParametric->getCurveColor(i, &r, &g, &b);
            WRITE_INDENT(1); WRITE_STRING("param.setCurveColor(" + NUM(i) + ", " +
                                          NUM(r) + ", " + NUM(g) + ", " + NUM(b) + ")");
            if (curve) {
                KeyFrameSet keys = curve->getKeyFrames_mt_safe();
                int c = 0;
                for (KeyFrameSet::iterator it3 = keys.begin(); it3 != keys.end(); ++it3, ++c) {
                    WRITE_INDENT(1); WRITE_STRING("param.setNthControlPoint(" + NUM(i) + ", " +
                                                  NUM(c) + ", " + NUM(it3->getTime()) + ", " +
                                                  NUM(it3->getValue()) + ", " + NUM(it3->getLeftDerivative())
                                                  + ", " + NUM(it3->getRightDerivative()) + ")");
                }
                
            }
        } else {
            boost::shared_ptr<Curve> curve = knob->getCurve(i, true);
            if (curve) {
                KeyFrameSet keys = curve->getKeyFrames_mt_safe();
                
                if (!keys.empty()) {
                    if (!hasExportedValue) {
                        hasExportedValue = true;
                        if (mustDefineParam) {
                            WRITE_INDENT(1); WRITE_STRING("param = " + paramFullName);
                        }
                    }
                }
                
                for (KeyFrameSet::iterator it3 = keys.begin(); it3 != keys.end(); ++it3) {
                    if (isAnimatedStr) {
                        std::string value = isAnimatedStr->getValueAtTime(it3->getTime(),i, true);
                        WRITE_INDENT(1); WRITE_STRING("param.setValueAtTime(" + ESC(value) + ", "
                                                      + NUM(it3->getTime())+ ")");
                        
                    } else if (isBool) {
                        int v = std::min(1., std::max(0.,std::floor(it3->getValue() + 0.5)));
                        QString vStr = v ? "True" : "False";
                        WRITE_INDENT(1); WRITE_STRING("param.setValueAtTime(" + vStr + ", "
                                                      + NUM(it3->getTime())  + ")");
                    } else if (isChoice) {
                        WRITE_INDENT(1); WRITE_STRING("param.setValueAtTime(" + NUM(it3->getValue()) + ", "
                                                      + NUM(it3->getTime()) + ")");
                    } else {
                        WRITE_INDENT(1); WRITE_STRING("param.setValueAtTime(" + NUM(it3->getValue()) + ", "
                                                      + NUM(it3->getTime()) + ", " + NUM(i) + ")");

                    }
                }
            }
            
            if ((!curve || curve->getKeyFramesCount() == 0) && knob->hasModifications(i)) {
                if (!hasExportedValue) {
                    hasExportedValue = true;
                    if (mustDefineParam) {
                        WRITE_INDENT(1); WRITE_STRING("param = " + paramFullName);
                    }
                }
                
                if (isGrp) {
                    int v = std::min(1., std::max(0.,std::floor(isGrp->getValue(i, true) + 0.5)));
                    QString vStr = v ? "True" : "False";
                    WRITE_INDENT(1); WRITE_STRING("param.setOpened(" + vStr + ")");
                } else if (isStr) {
                    std::string v = isStr->getValue(i, true);
                    WRITE_INDENT(1); WRITE_STRING("param.setValue(" + ESC(v)  + ")");
                } else if (isDouble) {
                    double v = isDouble->getValue(i, true);
                    WRITE_INDENT(1); WRITE_STRING("param.setValue(" + NUM(v) + ", " + NUM(i) + ")");
                } else if (isChoice) {
                    int v = isInt->getValue(i, true);
                    WRITE_INDENT(1); WRITE_STRING("param.setValue(" + NUM(v) + ")");
                } else if (isInt) {
                    int v = isInt->getValue(i, true);
                    WRITE_INDENT(1); WRITE_STRING("param.setValue(" + NUM(v) + ", " + NUM(i) + ")");
                } else if (isBool) {
                    int v = std::min(1., std::max(0.,std::floor(isBool->getValue(i, true) + 0.5)));
                    QString vStr = v ? "True" : "False";
                    WRITE_INDENT(1); WRITE_STRING("param.setValue(" + vStr + ")");
                }
            }
        }
        
    } // for (int i = 0; i < (*it2)->getDimension(); ++i)
    if (mustDefineParam && hasExportedValue) {
        WRITE_INDENT(1); WRITE_STRING("del param");
    }
    
    return hasExportedValue;
}

static void exportUserKnob(const boost::shared_ptr<KnobI>& knob,const QString& fullyQualifiedNodeName,Group_Knob* group,Page_Knob* page,QTextStream& ts)
{
    Int_Knob* isInt = dynamic_cast<Int_Knob*>(knob.get());
    Double_Knob* isDouble = dynamic_cast<Double_Knob*>(knob.get());
    Bool_Knob* isBool = dynamic_cast<Bool_Knob*>(knob.get());
    Choice_Knob* isChoice = dynamic_cast<Choice_Knob*>(knob.get());
    Color_Knob* isColor = dynamic_cast<Color_Knob*>(knob.get());
    String_Knob* isStr = dynamic_cast<String_Knob*>(knob.get());
    File_Knob* isFile = dynamic_cast<File_Knob*>(knob.get());
    OutputFile_Knob* isOutFile = dynamic_cast<OutputFile_Knob*>(knob.get());
    Path_Knob* isPath = dynamic_cast<Path_Knob*>(knob.get());
    Group_Knob* isGrp = dynamic_cast<Group_Knob*>(knob.get());
    Button_Knob* isButton = dynamic_cast<Button_Knob*>(knob.get());
    Parametric_Knob* isParametric = dynamic_cast<Parametric_Knob*>(knob.get());
    
    if (isInt) {
        QString createToken;
        switch (isInt->getDimension()) {
            case 1:
                createToken = ".createIntParam(";
                break;
            case 2:
                createToken = ".createInt2DParam(";
                break;
            case 3:
                createToken = ".createInt3DParam(";
                break;
            default:
                assert(false);
                createToken = ".createIntParam(";
                break;
        }
        WRITE_INDENT(1); WRITE_STRING("param = " + fullyQualifiedNodeName + createToken + ESC(isInt->getName()) +
                                      ", " + ESC(isInt->getDescription()) + ")");
        for (int i = 0; i < isInt->getDimension() ; ++i) {
            int min = isInt->getMinimum(i);
            int max = isInt->getMaximum(i);
            int dMin = isInt->getDisplayMinimum(i);
            int dMax = isInt->getDisplayMaximum(i);
            if (min != INT_MIN) {
                WRITE_INDENT(1); WRITE_STRING("param.setMinimum(" + NUM(min) + ", " +
                                              NUM(i) + ")");
            }
            if (max != INT_MAX) {
                WRITE_INDENT(1); WRITE_STRING("param.setMaximum(" + NUM(max) + ", " +
                                              NUM(i) + ")");
            }
            if (dMin != INT_MIN) {
                WRITE_INDENT(1); WRITE_STRING("param.setDisplayMinimum(" + NUM(dMin) + ", " +
                                              NUM(i) + ")");
            }
            if (dMax != INT_MAX) {
                WRITE_INDENT(1); WRITE_STRING("param.setDisplayMaximum(" + NUM(dMax) + ", " +
                                              NUM(i) + ")");
            }
            
        }
    } else if (isDouble) {
        QString createToken;
        switch (isDouble->getDimension()) {
            case 1:
                createToken = ".createDoubleParam(";
                break;
            case 2:
                createToken = ".createDouble2DParam(";
                break;
            case 3:
                createToken = ".createDouble3DParam(";
                break;
            default:
                assert(false);
                createToken = ".createDoubleParam(";
                break;
        }
        WRITE_INDENT(1); WRITE_STRING("param = " + fullyQualifiedNodeName + createToken + ESC(isDouble->getName()) +
                                      ", " + ESC(isDouble->getDescription()) + ")");
        for (int i = 0; i < isDouble->getDimension() ; ++i) {
            double min = isDouble->getMinimum(i);
            double max = isDouble->getMaximum(i);
            double dMin = isDouble->getDisplayMinimum(i);
            double dMax = isDouble->getDisplayMaximum(i);
            if (min != -DBL_MAX) {
                WRITE_INDENT(1); WRITE_STRING("param.setMinimum(" + NUM(min) + ", " +
                                              NUM(i) + ")");
            }
            if (max != DBL_MAX) {
                WRITE_INDENT(1); WRITE_STRING("param.setMaximum(" + NUM(max) + ", " +
                                              NUM(i) + ")");
            }
            if (dMin != -DBL_MAX) {
                WRITE_INDENT(1); WRITE_STRING("param.setDisplayMinimum(" + NUM(dMin) + ", " +
                                              NUM(i) + ")");
            }
            if (dMax != DBL_MAX) {
                WRITE_INDENT(1); WRITE_STRING("param.setDisplayMaximum(" + NUM(dMax) + ", " +
                                              NUM(i) + ")");
            }
            
        }

    } else if (isBool) {
        WRITE_INDENT(1); WRITE_STRING("param = " + fullyQualifiedNodeName + ".createBooleanParam(" + ESC(isBool->getName()) +
                                      ", " + ESC(isBool->getDescription()) + ")");
    } else if (isChoice) {
        WRITE_INDENT(1); WRITE_STRING("param = " + fullyQualifiedNodeName + ".createChoiceParam(" +
                                      ESC(isChoice->getName()) +
                                      ", " + ESC(isChoice->getDescription()) + ")");
        std::vector<std::string> entries = isChoice->getEntries_mt_safe();
        std::vector<std::string> helps = isChoice->getEntriesHelp_mt_safe();
        if (entries.size() > 0) {
            if (helps.empty()) {
                for (U32 i = 0; i < entries.size(); ++i) {
                    helps.push_back("");
                }
            }
            WRITE_INDENT(1); ts << "entries = [ (" << ESC(entries[0]) << ", " << ESC(helps[0]) << "),\n";
            for (U32 i = 1; i < entries.size(); ++i) {
                QString endToken = (i == entries.size() - 1) ? ")]" : "),";
                WRITE_INDENT(1); WRITE_STRING("(" + ESC(entries[0]) + ", " + ESC(helps[0]) + endToken);
            }
            WRITE_INDENT(1); WRITE_STATIC_LINE("param.setOptions(entries)");
            WRITE_INDENT(1); WRITE_STATIC_LINE("del entries");
        }
    } else if (isColor) {
        QString hasAlphaStr = (isColor->getDimension() == 4) ? "True" : "False";
        WRITE_INDENT(1); WRITE_STRING("param = " + fullyQualifiedNodeName + ".createColorParam(" + ESC(isColor->getName()) +
                                      ", " + ESC(isColor->getDescription()) + ", " + hasAlphaStr +  ")");
        for (int i = 0; i < isColor->getDimension() ; ++i) {
            double min = isColor->getMinimum(i);
            double max = isColor->getMaximum(i);
            double dMin = isColor->getDisplayMinimum(i);
            double dMax = isColor->getDisplayMaximum(i);
            if (min != -DBL_MAX) {
                WRITE_INDENT(1); WRITE_STRING("param.setMinimum(" + NUM(min) + ", " +
                                              NUM(i) + ")");
            }
            if (max != DBL_MAX) {
                WRITE_INDENT(1); WRITE_STRING("param.setMaximum(" + NUM(max) + ", " +
                                              NUM(i) + ")");
            }
            if (dMin != -DBL_MAX) {
                WRITE_INDENT(1); WRITE_STRING("param.setDisplayMinimum(" + NUM(dMin) + ", " +
                                              NUM(i) + ")");
            }
            if (dMax != DBL_MAX) {
                WRITE_INDENT(1); WRITE_STRING("param.setDisplayMaximum(" + NUM(dMax) + ", " +
                                              NUM(i) + ")");
            }
            
        }

    } else if (isButton) {
        WRITE_INDENT(1); WRITE_STRING("param = " + fullyQualifiedNodeName + ".createButtonParam(" +
                                      ESC(isButton->getName()) +
                                      ", " + ESC(isButton->getDescription()) + ")");
    } else if (isStr) {
        WRITE_INDENT(1); WRITE_STRING("param = " + fullyQualifiedNodeName + ".createStringParam(" +
                                      ESC(isStr->getName()) +
                                      ", " + ESC(isStr->getDescription()) + ")");
        QString typeStr;
        if (isStr->isLabel()) {
            typeStr = "eStringTypeLabel";
        } else if (isStr->isMultiLine()) {
            if (isStr->usesRichText()) {
                typeStr = "eStringTypeRichTextMultiLine";
            } else {
                typeStr = "eStringTypeMultiLine";
            }
        } else if (isStr->isCustomKnob()) {
            typeStr = "eStringTypeCustom";
        } else {
            typeStr = "eStringTypeDefault";
        }
        WRITE_INDENT(1); WRITE_STRING("param.setType(StringParam." + typeStr + ")");
    } else if (isFile) {
        WRITE_INDENT(1); WRITE_STRING("param = " + fullyQualifiedNodeName + ".createFileParam(" + ESC(isFile->getName()) +
                                      ", " + ESC(isFile->getDescription()) + ")");
        QString seqStr = isFile->isInputImageFile() ? "True" : "False";
        WRITE_INDENT(1); WRITE_STRING("param.setSequenceEnabled("+ seqStr + ")");
    } else if (isOutFile) {
        WRITE_INDENT(1); WRITE_STRING("param = " + fullyQualifiedNodeName + ".createOutputFileParam(" +
                                      ESC(isOutFile->getName()) +
                                      ", " + ESC(isOutFile->getDescription()) + ")");
        QString seqStr = isFile->isInputImageFile() ? "True" : "False";
        WRITE_INDENT(1); WRITE_STRING("param.setSequenceEnabled("+ seqStr + ")");
    } else if (isPath) {
        WRITE_INDENT(1); WRITE_STRING("param = " + fullyQualifiedNodeName + ".createPathParam(" +
                                      ESC(isPath->getName()) +
                                      ", " + ESC(isPath->getDescription()) + ")");
        if (isPath->isMultiPath()) {
            WRITE_INDENT(1); WRITE_STRING("param.setAsMultiPathTable()");
        }
    } else if (isGrp) {
        WRITE_INDENT(1); WRITE_STRING("param = " + fullyQualifiedNodeName + ".createGroupParam(" +
                                      ESC(isGrp->getName()) +
                                      ", " + ESC(isGrp->getDescription()) + ")");
        if (isGrp->isTab()) {
            WRITE_INDENT(1); WRITE_STRING("param.setAsTab()");
        }

    } else if (isParametric) {
        WRITE_INDENT(1); WRITE_STRING("param = " + fullyQualifiedNodeName + ".createParametricParam(" +
                                      ESC(isParametric->getName()) +
                                      ", " + ESC(isParametric->getDescription()) +  ", " +
                                      NUM(isParametric->getDimension()) + ")");
    }
    
    WRITE_STATIC_LINE("");
    if (group) {
        QString grpFullName = fullyQualifiedNodeName + "." + QString(group->getName().c_str());
        WRITE_INDENT(1); WRITE_STATIC_LINE("#Add the param to the group, no need to add it to the page");
        WRITE_INDENT(1); WRITE_STRING(grpFullName + ".addParam(param)");
    } else {
        assert(page);
        QString pageFullName = fullyQualifiedNodeName + "." + QString(page->getName().c_str());
        WRITE_INDENT(1); WRITE_STATIC_LINE("#Add the param to the page");
        WRITE_INDENT(1); WRITE_STRING(pageFullName + ".addParam(param)");
    }
    
    WRITE_STATIC_LINE("");
    WRITE_INDENT(1); WRITE_STATIC_LINE("#Set param properties");

    QString help(knob->getHintToolTip().c_str());
    WRITE_INDENT(1); WRITE_STRING("param.setHelp(\"" + help + "\")");
    if (knob->isNewLineTurnedOff()) {
        WRITE_INDENT(1); WRITE_STRING("param.setAddNewLine(False)");
    }
    
    if (!knob->getIsPersistant()) {
        WRITE_INDENT(1); WRITE_STRING("param.setPersistant(False)");
    }
    
    if (!knob->getEvaluateOnChange()) {
        WRITE_INDENT(1); WRITE_STRING("param.setEvaluateOnChange(False)");
    }
    
    if (knob->canAnimate()) {
        QString animStr = knob->isAnimationEnabled() ? "True" : "False";
        WRITE_INDENT(1); WRITE_STRING("param.setAnimationEnabled(" + animStr + ")");
    }
    
    if (knob->getIsSecret()) {
        WRITE_INDENT(1); WRITE_STRING("param.setVisible(False)");
    }
    
    for (int i = 0; i < knob->getDimension(); ++i) {
        if (!knob->isEnabled(i)) {
            WRITE_INDENT(1); WRITE_STRING("param.setEnabled(False, " +  NUM(i) + ")");
        }
    }
    
    exportKnobValues(knob,"", false, ts);
    WRITE_INDENT(1); WRITE_STRING(fullyQualifiedNodeName + "." + QString(knob->getName().c_str()) + " = param");
    WRITE_INDENT(1); WRITE_STATIC_LINE("del param");
    
    WRITE_STATIC_LINE("");
    
    if (isGrp) {
        const std::vector<boost::shared_ptr<KnobI> >& children =  isGrp->getChildren();
        for (std::vector<boost::shared_ptr<KnobI> >::const_iterator it3 = children.begin(); it3 != children.end(); ++it3) {
            exportUserKnob(*it3, fullyQualifiedNodeName, isGrp, page, ts);
        }
    }
    
}

static void exportBezierPointAtTime(const boost::shared_ptr<BezierCP>& point,
                                    bool isFeather,
                                    int time,
                                    int idx,
                                    QTextStream& ts)
{
    
    QString token = isFeather ? "bezier.setFeatherPointAtIndex(" : "bezier.setPointAtIndex(";
    double x,y,lx,ly,rx,ry;
    point->getPositionAtTime(time, &x, &y);
    point->getLeftBezierPointAtTime(time, &lx, &ly);
    point->getRightBezierPointAtTime(time, &rx, &ry);
    
    WRITE_INDENT(1); WRITE_STATIC_LINE(token + NUM(idx) + ", " +
                                       NUM(time) + ", " + NUM(x) + ", " +
                                       NUM(y) + ", " + NUM(lx) + ", " +
                                       NUM(ly) + ", " + NUM(rx) + ", " +
                                       NUM(ry) + ")");
   
}

static void exportRotoLayer(const std::list<boost::shared_ptr<RotoItem> >& items,
                            const boost::shared_ptr<RotoLayer>& layer,
                            QTextStream& ts)
{
    QString parentLayerName = QString(layer->getName_mt_safe().c_str()) + "_layer";
    for (std::list<boost::shared_ptr<RotoItem> >::const_iterator it = items.begin(); it != items.end(); ++it) {
        
        boost::shared_ptr<RotoLayer> isLayer = boost::dynamic_pointer_cast<RotoLayer>(*it);
        boost::shared_ptr<Bezier> isBezier = boost::dynamic_pointer_cast<Bezier>(*it);
        
        if (isBezier) {
            int time;
            
            const std::list<boost::shared_ptr<BezierCP> >& cps = isBezier->getControlPoints();
            const std::list<boost::shared_ptr<BezierCP> >& fps = isBezier->getFeatherPoints();
            
            if (cps.empty()) {
                continue;
            }
            
            time = cps.front()->getKeyframeTime(0);
            
            WRITE_INDENT(1); WRITE_STATIC_LINE("bezier = roto.createBezier(0,0, " + NUM(time) + ")");
            WRITE_INDENT(1); WRITE_STATIC_LINE("bezier.setName(" + ESC(isBezier->getName_mt_safe()) + ")");
            QString lockedStr = isBezier->getLocked() ? "True" : "False";
            WRITE_INDENT(1); WRITE_STRING("bezier.setLocked(" + lockedStr + ")");
            QString visibleStr = isBezier->isGloballyActivated() ? "True" : "False";
            WRITE_INDENT(1); WRITE_STRING("bezier.setVisible(" + visibleStr + ")");
            
            boost::shared_ptr<Bool_Knob> activatedKnob = isBezier->getActivatedKnob();
            exportKnobValues(activatedKnob, "bezier.getActivatedParam()", true, ts);
            
            boost::shared_ptr<Double_Knob> featherDist = isBezier->getFeatherKnob();
            exportKnobValues(featherDist,"bezier.getFeatherDistanceParam()", true, ts);
            
            boost::shared_ptr<Double_Knob> opacityKnob = isBezier->getOpacityKnob();
            exportKnobValues(opacityKnob,"bezier.getOpacityParam()", true, ts);
            
            boost::shared_ptr<Double_Knob> fallOffKnob = isBezier->getFeatherFallOffKnob();
            exportKnobValues(fallOffKnob,"bezier.getFeatherFallOffParam()", true, ts);
            
            boost::shared_ptr<Color_Knob> colorKnob = isBezier->getColorKnob();
            exportKnobValues(colorKnob, "bezier.getActivatedParam()", true, ts);
            
            boost::shared_ptr<Choice_Knob> compositing = isBezier->getOperatorKnob();
            exportKnobValues(compositing, "bezier.getCompositingOperatorParam()", true, ts);

            
            WRITE_INDENT(1); WRITE_STRING(parentLayerName + ".addItem(bezier)");
            WRITE_INDENT(1); WRITE_STATIC_LINE("");
           
            assert(cps.size() == fps.size());
            
            std::set<int> kf;
            isBezier->getKeyframeTimes(&kf);
            
            //the last python call already registered the first control point
            int nbPts = cps.size() - 1;
            WRITE_INDENT(1); WRITE_STATIC_LINE("for i in range(0, " + NUM(nbPts) +"):");
            WRITE_INDENT(2); WRITE_STATIC_LINE("bezier.addControlPoint(0,0)");
          
            ///Now that all points are created position them
            int idx = 0;
            std::list<boost::shared_ptr<BezierCP> >::const_iterator fpIt = fps.begin();
            for (std::list<boost::shared_ptr<BezierCP> >::const_iterator it2 = cps.begin(); it2 != cps.end(); ++it2,++fpIt,++idx) {
                for (std::set<int>::iterator it3 = kf.begin(); it3 != kf.end(); ++it3) {
                    exportBezierPointAtTime(*it2, false, *it3, idx, ts);
                    exportBezierPointAtTime(*fpIt, true, *it3, idx, ts);
                }
                if (kf.empty()) {
                    exportBezierPointAtTime(*it2, false, time, idx, ts);
                    exportBezierPointAtTime(*fpIt, true, time, idx, ts);
                }
                boost::shared_ptr<Double_Knob> track = (*it2)->isSlaved();
                if (track) {
                    Natron::EffectInstance* effect = dynamic_cast<Natron::EffectInstance*>(track->getHolder());
                    assert(effect && effect->getNode()->isTrackerNode());
                    std::string trackerName = effect->getNode()->getName_mt_safe();
                    int trackTime = (*it2)->getOffsetTime();
                    WRITE_INDENT(1); WRITE_STRING("tracker = group.getNode(\"" + QString(trackerName.c_str()) + "\")");
                    WRITE_INDENT(1); WRITE_STRING("center = tracker.getParam(\"" + QString(track->getName().c_str()) + "\")");
                    WRITE_INDENT(1); WRITE_STRING("bezier.slavePointToTrack(" + NUM(idx) + ", " +
                                                  NUM(trackTime) + ",center)");
                    WRITE_INDENT(1); WRITE_STATIC_LINE("del center");
                    WRITE_INDENT(1); WRITE_STATIC_LINE("del tracker");
                }
                
            }
            if (isBezier->isCurveFinished()) {
                WRITE_INDENT(1); WRITE_STRING("bezier.setCurveFinished(True)");
            }
            
            WRITE_INDENT(1); WRITE_STATIC_LINE("del bezier");
            
            
        } else {
            
            QString name =  QString(isLayer->getName_mt_safe().c_str());
            QString layerName = name + "_layer";
            WRITE_INDENT(1); WRITE_STATIC_LINE(name + " = roto.createLayer()");
            WRITE_INDENT(1); WRITE_STATIC_LINE(layerName +  ".setName(" + ESC(name) + ")");
            QString lockedStr = isLayer->getLocked() ? "True" : "False";
            WRITE_INDENT(1); WRITE_STRING(layerName + ".setLocked(" + lockedStr + ")");
            QString visibleStr = isLayer->isGloballyActivated() ? "True" : "False";
            WRITE_INDENT(1); WRITE_STRING(layerName + ".setVisible(" + visibleStr + ")");
            
            WRITE_INDENT(1); WRITE_STRING(parentLayerName + ".addItem(" + layerName);
            
            const std::list<boost::shared_ptr<RotoItem> >& items = isLayer->getItems();
            exportRotoLayer(items, isLayer, ts);
            WRITE_INDENT(1); WRITE_STRING("del " + layerName);
        }
        WRITE_STATIC_LINE("");
    }
}

static void exportAllNodeKnobs(const boost::shared_ptr<Natron::Node>& node,QTextStream& ts)
{
    
    const std::vector<boost::shared_ptr<KnobI> >& knobs = node->getKnobs();
    std::list<Page_Knob*> userPages;
    for (std::vector<boost::shared_ptr<KnobI> >::const_iterator it2 = knobs.begin(); it2 != knobs.end(); ++it2) {
        if ((*it2)->getIsPersistant() && !(*it2)->isUserKnob()) {
            if (exportKnobValues(*it2,"lastNode.getParam(\"" + QString((*it2)->getName().c_str()) + "\")", true, ts)) {
                WRITE_STATIC_LINE("");
            }
            
        }
        
        if ((*it2)->isUserKnob()) {
            Page_Knob* isPage = dynamic_cast<Page_Knob*>(it2->get());
            if (isPage) {
                userPages.push_back(isPage);
            }
        }
    }// for (std::vector<boost::shared_ptr<KnobI> >::const_iterator it2 = knobs.begin(); it2 != knobs.end(); ++it2)
    if (!userPages.empty()) {
        WRITE_STATIC_LINE("");
        WRITE_INDENT(1); WRITE_STATIC_LINE("#Create the user-parameters");
    }
    for (std::list<Page_Knob*>::iterator it2 = userPages.begin(); it2!= userPages.end(); ++it2) {
        WRITE_INDENT(1); WRITE_STRING("lastNode." + QString((*it2)->getName().c_str()) +
                                      " = lastNode.createPageParam(" + ESC((*it2)->getName()) + ", " +
                                      ESC((*it2)->getDescription()) + ")");
        const std::vector<boost::shared_ptr<KnobI> >& children =  (*it2)->getChildren();
        for (std::vector<boost::shared_ptr<KnobI> >::const_iterator it3 = children.begin(); it3 != children.end(); ++it3) {
            exportUserKnob(*it3, "lastNode", 0, *it2, ts);
        }
    }
    
    if (!userPages.empty()) {
        WRITE_INDENT(1); WRITE_STATIC_LINE("#Refresh the GUI with the newly created parameters");
        WRITE_INDENT(1); WRITE_STATIC_LINE("lastNode.refreshUserParamsGUI()");
    }
    
    boost::shared_ptr<RotoContext> roto = node->getRotoContext();
    if (roto) {
        const std::list<boost::shared_ptr<RotoLayer> >& layers = roto->getLayers();
        
        if (!layers.empty()) {
            WRITE_INDENT(1); WRITE_STATIC_LINE("#For the roto node, create all layers and beziers");
            WRITE_INDENT(1); WRITE_STRING("roto = lastNode.getRotoContext()");
            boost::shared_ptr<RotoLayer> baseLayer = layers.front();
            QString baseLayerName = QString(baseLayer->getName_mt_safe().c_str());
            QString baseLayerToken = baseLayerName +"_layer";
            WRITE_INDENT(1); WRITE_STATIC_LINE(baseLayerToken + " = roto.getBaseLayer()");
            
            WRITE_INDENT(1); WRITE_STRING(baseLayerToken + ".setName(" + ESC(baseLayerName) + ")");
            QString lockedStr = baseLayer->getLocked() ? "True" : "False";
            WRITE_INDENT(1); WRITE_STRING(baseLayerToken + ".setLocked(" + lockedStr + ")");
            QString visibleStr = baseLayer->isGloballyActivated() ? "True" : "False";
            WRITE_INDENT(1); WRITE_STRING(baseLayerToken + ".setVisible(" + visibleStr + ")");
            exportRotoLayer(baseLayer->getItems(),baseLayer,  ts);
            WRITE_INDENT(1); WRITE_STRING("del " + baseLayerToken);
            WRITE_INDENT(1); WRITE_STRING("del roto");
        }
    }
}

static bool exportKnobLinks(const boost::shared_ptr<Natron::Node>& node,
                     const QString& nodeName,
                     QTextStream& ts)
{
    bool hasExportedLink = false;
    const std::vector<boost::shared_ptr<KnobI> >& knobs = node->getKnobs();
    for (std::vector<boost::shared_ptr<KnobI> >::const_iterator it2 = knobs.begin(); it2 != knobs.end(); ++it2) {
        QString paramName = nodeName + ".getParam(\"" + QString((*it2)->getName().c_str()) + "\")";
        
        bool hasDefined = false;
        for (int i = 0; i < (*it2)->getDimension(); ++i) {
            std::string expr = (*it2)->getExpression(i);
            QString hasRetVar = (*it2)->isExpressionUsingRetVariable(i) ? "True" : "False";
            if (!expr.empty()) {
                if (!hasDefined) {
                    WRITE_INDENT(1); WRITE_STRING("param = " + paramName);
                    hasDefined = true;
                }
                hasExportedLink = true;
                WRITE_INDENT(1); WRITE_STRING("param.setExpression(" + ESC(expr) + ", " +
                                              hasRetVar + ", " + NUM(i) + ")");
            }
            
        }
        if (hasDefined) {
            WRITE_INDENT(1); WRITE_STATIC_LINE("del param");
        }
    }
    return hasExportedLink;
}

static void exportGroupInternal(NodeGroup* group,const QString& groupName, QTextStream& ts)
{
    WRITE_INDENT(1); WRITE_STATIC_LINE("#Create all nodes in the group");
    
    NodeList nodes = group->getNodes();
    NodeList exportedNodes;
    
    ///Re-order nodes so we're sure Roto nodes get exported in the end since they may depend on Trackers
    NodeList rotos;
    NodeList newNodes;
    for (NodeList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        if ((*it)->isRotoNode()) {
            rotos.push_back(*it);
        } else {
            newNodes.push_back(*it);
        }
    }
    newNodes.insert(newNodes.end(), rotos.begin(),rotos.end());
    
    for (NodeList::iterator it = newNodes.begin(); it != newNodes.end(); ++it) {
        
        ///Don't create viewer while exporting
        ViewerInstance* isViewer = dynamic_cast<ViewerInstance*>((*it)->getLiveInstance());
        if (isViewer) {
            continue;
        }
        
        if (!(*it)->isActivated()) {
            continue;
        }
        
        exportedNodes.push_back(*it);
        
        
        ///Let the parent of the multi-instance node create the children
        if ((*it)->getParentMultiInstance()) {
            continue;
        }
    
        
        QString nodeName((*it)->getPluginID().c_str());
        
        WRITE_INDENT(1); WRITE_STRING("lastNode = app.createNode(" + ESC(nodeName) + ", " +
                                      NUM((*it)->getPlugin()->getMajorVersion()) + ", " + groupName +
                                      ")");
        WRITE_INDENT(1); WRITE_STRING("lastNode.setName(" + ESC((*it)->getName_mt_safe()) + ")");
        double x,y;
        (*it)->getPosition(&x,&y);
        WRITE_INDENT(1); WRITE_STRING("lastNode.setPosition(" + NUM(x) + ", " + NUM(y) + ")");
        
        QString nodeNameInScript = groupName + QString((*it)->getName_mt_safe().c_str());
        WRITE_INDENT(1); WRITE_STRING(nodeNameInScript + " = lastNode");
        WRITE_STATIC_LINE("");
        exportAllNodeKnobs(*it,ts);
        WRITE_INDENT(1); WRITE_STRING("del lastNode");
        WRITE_STATIC_LINE("");
        
        
        std::list< NodePtr > children;
        (*it)->getChildrenMultiInstance(&children);
        if (!children.empty()) {
            WRITE_INDENT(1); WRITE_STATIC_LINE("#Create children if the node is a multi-instance such as a tracker");
            for (std::list< NodePtr > ::iterator it2 = children.begin(); it2 != children.end(); ++it2) {
                if ((*it2)->isActivated()) {
                    WRITE_INDENT(1); WRITE_STRING("lastNode = " + nodeNameInScript + ".createChild()");
                    WRITE_INDENT(1); WRITE_STRING("lastNode.setName(\"" + QString((*it2)->getName_mt_safe().c_str()) + "\")");
                    exportAllNodeKnobs(*it2,ts);
                    WRITE_INDENT(1); WRITE_STRING(groupName + QString((*it2)->getName_mt_safe().c_str()) + " = lastNode");
                    WRITE_INDENT(1); WRITE_STRING("del lastNode");
                }
            }
        }
        
        
        WRITE_STATIC_LINE("");
        
        NodeGroup* isGrp = dynamic_cast<NodeGroup*>((*it)->getLiveInstance());
        if (isGrp) {
            WRITE_INDENT(1); WRITE_STRING(groupName + "group = " + nodeNameInScript);
            exportGroupInternal(isGrp, groupName + "group", ts);
        }
        WRITE_STATIC_LINE("");
        
    }
    WRITE_STATIC_LINE("");
    WRITE_INDENT(1); WRITE_STATIC_LINE("#Create the parameters of the group node the same way we did for all internal nodes");
    WRITE_INDENT(1); WRITE_STRING("lastNode = " + groupName);
    exportAllNodeKnobs(group->getNode(),ts);
    WRITE_INDENT(1); WRITE_STATIC_LINE("del lastNode");
    WRITE_STATIC_LINE("");
    
    WRITE_INDENT(1); WRITE_STATIC_LINE("#Now that all nodes are created we can connect them together, restore expressions");
    for (NodeList::iterator it = exportedNodes.begin(); it != exportedNodes.end(); ++it) {
        
        QString nodeQualifiedName(groupName + (*it)->getName_mt_safe().c_str());
        bool hasConnected = false;
        if (!(*it)->getParentMultiInstance()) {
            for (int i = 0; i < (*it)->getMaxInputCount(); ++i) {
                NodePtr inputNode = (*it)->getRealInput(i);
                if (inputNode) {
                    hasConnected = true;
                    QString inputQualifiedName(groupName  + inputNode->getName_mt_safe().c_str());
                    WRITE_INDENT(1); WRITE_STRING(nodeQualifiedName + ".connectInput(" + NUM(i) +
                                                  ", " + inputQualifiedName + ")");
                }
            }
        }
        if (exportKnobLinks(*it,nodeQualifiedName, ts) || hasConnected) {
            WRITE_STATIC_LINE("");
        }
    }
    
}

void
NodeGroup::exportGroupToPython(const QString& pluginLabel,
                               const QString& pluginIconPath,
                               const QString& pluginGrouping,
                               QString& output)
{
    QTextStream ts(&output);
    WRITE_STATIC_LINE("#This file was automatically generated by " NATRON_APPLICATION_NAME ".");
    WRITE_STATIC_LINE("#Note that Viewers are never exported");
    WRITE_STATIC_LINE("# -*- coding: utf-8 -*-");
    WRITE_STATIC_LINE("");
    WRITE_STATIC_LINE("from natron import *");
    WRITE_STATIC_LINE("");
   
    WRITE_STATIC_LINE("def getLabel():");
    WRITE_INDENT(1);WRITE_STRING("return \"" + pluginLabel + "\"");
    WRITE_STATIC_LINE("");
  
    if (!pluginIconPath.isEmpty()) {
        WRITE_STATIC_LINE("def getIconPath():");
        WRITE_INDENT(1);WRITE_STRING("return \"" + pluginIconPath + "\"");
        WRITE_STATIC_LINE("");
    }
    
    WRITE_STATIC_LINE("def getGrouping():");
    WRITE_INDENT(1);WRITE_STRING("return \"" + pluginGrouping + "\"");
    WRITE_STATIC_LINE("");

    
    WRITE_STATIC_LINE("def createInstance(app,group):");
    WRITE_STATIC_LINE("");
    exportGroupInternal(this, "group", ts);
}