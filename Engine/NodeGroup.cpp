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
#include <locale>
#include <QThreadPool>
#include <QCoreApplication>

#include "Engine/AppInstance.h"
#include "Engine/Node.h"
#include "Engine/OutputSchedulerThread.h"
#include "Engine/Project.h"
#include "Engine/KnobTypes.h"
#include "Engine/KnobFile.h"
#include "Engine/TimeLine.h"
#include "Engine/NoOp.h"
#include "Engine/ViewerInstance.h"

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
    
    NodeGroupPrivate()
    : inputs()
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
   
}

void
NodeGroup::notifyNodeDeactivated(const boost::shared_ptr<Natron::Node>& node)
{
    GroupInput* isInput = dynamic_cast<GroupInput*>(node->getLiveInstance());
    if (isInput) {
        for (U32 i = 0; i < _imp->inputs.size(); ++i) {
            NodePtr input = _imp->inputs[i].lock();
            if (node == input) {
                
                ///Also disconnect the real input
                getNode()->disconnectInput(i);
                
                _imp->inputs.erase(_imp->inputs.begin() + i);
                getNode()->initializeInputs();
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

}

void
NodeGroup::notifyNodeActivated(const boost::shared_ptr<Natron::Node>& node)
{
    GroupInput* isInput = dynamic_cast<GroupInput*>(node->getLiveInstance());
    if (isInput) {
        _imp->inputs.push_back(node);
        getNode()->initializeInputs();
    }
    GroupOutput* isOutput = dynamic_cast<GroupOutput*>(node->getLiveInstance());
    if (isOutput) {
        _imp->output = node;
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