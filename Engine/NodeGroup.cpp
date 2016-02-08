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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "NodeGroup.h"

#include <set>
#include <locale>
#include <cfloat>
#include <algorithm> // min, max
#include <cassert>
#include <stdexcept>

#include <QtCore/QThreadPool>
#include <QtCore/QCoreApplication>
#include <QtCore/QTextStream>

#include "Engine/AppInstance.h"
#include "Engine/Bezier.h"
#include "Engine/BezierCP.h"
#include "Engine/Curve.h"
#include "Engine/GroupInput.h"
#include "Engine/GroupOutput.h"
#include "Engine/Image.h"
#include "Engine/KnobFile.h"
#include "Engine/KnobTypes.h"
#include "Engine/Node.h"
#include "Engine/NodeGraphI.h"
#include "Engine/NodeGuiI.h"
#include "Engine/OutputSchedulerThread.h"
#include "Engine/Plugin.h"
#include "Engine/Project.h"
#include "Engine/PrecompNode.h"
#include "Engine/RotoContext.h"
#include "Engine/RotoLayer.h"
#include "Engine/Settings.h"
#include "Engine/TimeLine.h"
#include "Engine/ViewerInstance.h"

#define NATRON_PYPLUG_EXPORTER_VERSION 6

NATRON_NAMESPACE_ENTER;

struct NodeCollectionPrivate
{
    AppInstance* app;
    
    NodeGraphI* graph;
    
    mutable QMutex nodesMutex;
    NodesList nodes;
    
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

NodesList
NodeCollection::getNodes() const
{
    QMutexLocker k(&_imp->nodesMutex);
    return _imp->nodes;
}

void
NodeCollection::getNodes_recursive(NodesList& nodes,bool onlyActive) const
{
    std::list<NodeGroup*> groupToRecurse;
    
    {
        QMutexLocker k(&_imp->nodesMutex);
        for (NodesList::const_iterator it = _imp->nodes.begin(); it != _imp->nodes.end(); ++it) {
            if (onlyActive && !(*it)->isActivated()) {
                continue;
            }
            nodes.push_back(*it);
            NodeGroup* isGrp = (*it)->isEffectGroup();
            if (isGrp) {
                groupToRecurse.push_back(isGrp);
            }
        }
        
    }
    
    for (std::list<NodeGroup*>::const_iterator it = groupToRecurse.begin(); it != groupToRecurse.end(); ++it) {
        (*it)->getNodes_recursive(nodes,onlyActive);
    }
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
    NodesList::iterator found = std::find(_imp->nodes.begin(), _imp->nodes.end(), node);
    if (found != _imp->nodes.end()) {
        _imp->nodes.erase(found);
    }
}

NodePtr
NodeCollection::getLastNode(const std::string& pluginID) const
{
    QMutexLocker k(&_imp->nodesMutex);
    for (NodesList::reverse_iterator it = _imp->nodes.rbegin(); it != _imp->nodes.rend(); ++it ) {
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
NodeCollection::getActiveNodes(NodesList* nodes) const
{
    QMutexLocker k(&_imp->nodesMutex);
    for (NodesList::iterator it = _imp->nodes.begin(); it != _imp->nodes.end(); ++it) {
        if ((*it)->isActivated()) {
            nodes->push_back(*it);
        }
    }
}

void
NodeCollection::getActiveNodesExpandGroups(NodesList* nodes) const
{
    QMutexLocker k(&_imp->nodesMutex);
    for (NodesList::iterator it = _imp->nodes.begin(); it != _imp->nodes.end(); ++it) {
        if ((*it)->isActivated()) {
            nodes->push_back(*it);
            NodeGroup* isGrp = (*it)->isEffectGroup();
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
    for (NodesList::iterator it = _imp->nodes.begin(); it != _imp->nodes.end(); ++it) {
        ViewerInstance* isViewer = (*it)->isEffectViewer();
        if (isViewer) {
            viewers->push_back(isViewer);
        }
        NodeGroup* isGrp = (*it)->isEffectGroup();
        if (isGrp) {
            isGrp->getViewers(viewers);
        }
    }
}

void
NodeCollection::getWriters(std::list<OutputEffectInstance*>* writers) const
{
    QMutexLocker k(&_imp->nodesMutex);
    for (NodesList::iterator it = _imp->nodes.begin(); it != _imp->nodes.end(); ++it) {
        if ((*it)->getEffectInstance()->isWriter()) {
            OutputEffectInstance* out = dynamic_cast<OutputEffectInstance*>((*it)->getEffectInstance().get());
            assert(out);
            writers->push_back(out);
        }
        NodeGroup* isGrp = (*it)->isEffectGroup();
        if (isGrp) {
            isGrp->getWriters(writers);
        }
    }

}

void
NodeCollection::notifyRenderBeingAborted()
{
    QMutexLocker k(&_imp->nodesMutex);
    for (NodesList::iterator it = _imp->nodes.begin(); it != _imp->nodes.end(); ++it) {
        (*it)->notifyRenderBeingAborted();
        NodeGroup* isGrp = (*it)->isEffectGroup();
        if (isGrp) {
            isGrp->notifyRenderBeingAborted();
        }
        PrecompNode* isPrecomp = dynamic_cast<PrecompNode*>((*it)->getEffectInstance().get());
        if (isPrecomp) {
            isPrecomp->getPrecompApp()->getProject()->notifyRenderBeingAborted();
        }
        
    }
}

static void setMustQuitProcessingRecursive(bool mustQuit, NodeCollection* grp)
{
    NodesList nodes = grp->getNodes();
    for (NodesList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        (*it)->setMustQuitProcessing(mustQuit);
        NodeGroup* isGrp = (*it)->isEffectGroup();
        if (isGrp) {
            setMustQuitProcessingRecursive(mustQuit,isGrp);
        }
        PrecompNode* isPrecomp = dynamic_cast<PrecompNode*>((*it)->getEffectInstance().get());
        if (isPrecomp) {
            setMustQuitProcessingRecursive(mustQuit,isPrecomp->getPrecompApp()->getProject().get());
        }

    }
}

static void quitAnyProcessingInternal(NodeCollection* grp) {
    NodesList nodes = grp->getNodes();
    for (NodesList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        (*it)->quitAnyProcessing();
        NodeGroup* isGrp = (*it)->isEffectGroup();
        if (isGrp) {
            quitAnyProcessingInternal(isGrp);
        }
        PrecompNode* isPrecomp = dynamic_cast<PrecompNode*>((*it)->getEffectInstance().get());
        if (isPrecomp) {
            quitAnyProcessingInternal(isPrecomp->getPrecompApp()->getProject().get());
        }
    }
}

void
NodeCollection::quitAnyProcessingForAllNodes()
{
    setMustQuitProcessingRecursive(true, this);
    quitAnyProcessingInternal(this);
    setMustQuitProcessingRecursive(false, this);
}

void
NodeCollection::resetTotalTimeSpentRenderingForAllNodes()
{
    QMutexLocker k(&_imp->nodesMutex);
    for (NodesList::iterator it = _imp->nodes.begin(); it != _imp->nodes.end(); ++it) {
        EffectInstPtr effect = (*it)->getEffectInstance();
        effect->resetTotalTimeSpentRendering();
        NodeGroup* isGroup = dynamic_cast<NodeGroup*>(effect.get());
        if (isGroup) {
            isGroup->resetTotalTimeSpentRenderingForAllNodes();
        }
    }
}

bool
NodeCollection::isCacheIDAlreadyTaken(const std::string& name) const
{
    QMutexLocker k(&_imp->nodesMutex);
    for (NodesList::iterator it = _imp->nodes.begin(); it!=_imp->nodes.end(); ++it) {
        if ((*it)->getCacheID() == name) {
            return true;
        }
    }
    return false;
}

bool
NodeCollection::hasNodeRendering() const
{
    QMutexLocker k(&_imp->nodesMutex);
    for (NodesList::iterator it = _imp->nodes.begin(); it != _imp->nodes.end(); ++it) {
        if ( (*it)->isOutputNode() ) {
            NodeGroup* isGrp = (*it)->isEffectGroup();
            PrecompNode* isPrecomp = dynamic_cast<PrecompNode*>((*it)->getEffectInstance().get());
            if (isGrp) {
                if (isGrp->hasNodeRendering()) {
                    return true;
                }
            } else if (isPrecomp) {
                if (isPrecomp->getPrecompApp()->getProject()->hasNodeRendering()) {
                    return true;
                }
            } else {
                OutputEffectInstance* effect = dynamic_cast<OutputEffectInstance*>((*it)->getEffectInstance().get());
                if (effect && effect->getRenderEngine()->hasThreadsWorking()) {
                    return true;
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
    
    if ( !getApplication()->isBackground() ) {
        double time = _imp->app->getTimeLine()->currentFrame();
        
        NodesList nodes = getNodes();
        for (NodesList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
            assert(*it);
            (*it)->computePreviewImage(time);
            NodeGroup* isGrp = (*it)->isEffectGroup();
            if (isGrp) {
                isGrp->refreshViewersAndPreviews();
            } else {
                ViewerInstance* n = (*it)->isEffectViewer();
                if (n) {
                    n->renderCurrentFrame(true);
                }
            }
        }
    }
}

void
NodeCollection::refreshPreviews()
{
    if (getApplication()->isBackground()) {
        return;
    }
    double time = _imp->app->getTimeLine()->currentFrame();
    NodesList nodes;
    getActiveNodes(&nodes);
    for (NodesList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        if ( (*it)->isPreviewEnabled() ) {
            (*it)->refreshPreviewImage(time);
        }
        NodeGroup* isGrp = (*it)->isEffectGroup();
        if (isGrp) {
            isGrp->refreshPreviews();
        }
    }
}

void
NodeCollection::forceRefreshPreviews()
{
    if (getApplication()->isBackground()) {
        return;
    }
    double time = _imp->app->getTimeLine()->currentFrame();
    NodesList nodes;
    getActiveNodes(&nodes);
    for (NodesList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        if ( (*it)->isPreviewEnabled() ) {
            (*it)->computePreviewImage(time);
        }
        NodeGroup* isGrp = (*it)->isEffectGroup();
        if (isGrp) {
            isGrp->forceRefreshPreviews();
        }
    }
}


void
NodeCollection::clearNodes(bool emitSignal)
{
    
    NodesList nodesToDelete;
    {
        QMutexLocker l(&_imp->nodesMutex);
        nodesToDelete = _imp->nodes;
    }
    
    ///First quit any processing
    for (NodesList::iterator it = nodesToDelete.begin(); it != nodesToDelete.end(); ++it) {
        (*it)->quitAnyProcessing();
        NodeGroup* isGrp = (*it)->isEffectGroup();
        if (isGrp) {
            isGrp->clearNodes(emitSignal);
        }
        PrecompNode* isPrecomp = dynamic_cast<PrecompNode*>((*it)->getEffectInstance().get());
        if (isPrecomp) {
            isPrecomp->getPrecompApp()->getProject()->clearNodes(emitSignal);
        }
    }
    
    ///Kill effects
    
    for (NodesList::iterator it = nodesToDelete.begin(); it != nodesToDelete.end(); ++it) {
        (*it)->destroyNode(false);
    }
    
    
    if (emitSignal) {
        if (_imp->graph) {
            _imp->graph->onNodesCleared();
        }
    }
    
    {
        QMutexLocker l(&_imp->nodesMutex);
        _imp->nodes.clear();
    }
    
    nodesToDelete.clear();
}


void
NodeCollection::checkNodeName(const Node* node, const std::string& baseName,bool appendDigit,bool errorIfExists,std::string* nodeName)
{
    if (baseName.empty()) {
        throw std::runtime_error(QObject::tr("Invalid script-name").toStdString());
        return;
    }
    ///Remove any non alpha-numeric characters from the baseName
    std::string cpy = Python::makeNameScriptFriendly(baseName);
    if (cpy.empty()) {
        throw std::runtime_error(QObject::tr("Invalid script-name").toStdString());
        return;
    }
    
    ///If this is a group and one of its parameter has the same script-name as the script-name of one of the node inside
    ///the python attribute will be overwritten. Try to prevent this situation.
    NodeGroup* isGroup = dynamic_cast<NodeGroup*>(this);
    if (isGroup) {
        const KnobsVec&  knobs = isGroup->getKnobs();
        for (KnobsVec::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
            if ((*it)->getName() == cpy) {
                std::stringstream ss;
                ss << QObject::tr("A node within a group cannot have the same script-name (").toStdString();
                ss << cpy;
                ss << QObject::tr(") as a parameter on the group for scripting purposes.").toStdString();
                throw std::runtime_error(ss.str());
                return;
            }
        }
    }
    bool foundNodeWithName = false;
    int no = 1;

    {
        std::stringstream ss;
        ss << cpy;
        if (appendDigit) {
            ss << no;
        }
        *nodeName = ss.str();
    }
    do {
        foundNodeWithName = false;
        QMutexLocker l(&_imp->nodesMutex);
        for (NodesList::iterator it = _imp->nodes.begin(); it != _imp->nodes.end(); ++it) {
            if (it->get() != node && (*it)->getScriptName_mt_safe() == *nodeName) {
                foundNodeWithName = true;
                break;
            }
        }
        if (foundNodeWithName) {
            if (errorIfExists || !appendDigit) {
                std::stringstream ss;
                ss << QObject::tr("A node with the script-name ").toStdString();
                ss << *nodeName;
                ss << QObject::tr(" already exists").toStdString();
                throw std::runtime_error(ss.str());
                return;
            }
            ++no;
            {
                std::stringstream ss;
                ss << cpy << no;
                *nodeName = ss.str();
            }
        }
    } while (foundNodeWithName);
}

void
NodeCollection::initNodeName(const std::string& pluginLabel,std::string* nodeName)
{
    std::string baseName(pluginLabel);
    if (baseName.size() > 3 &&
        baseName[baseName.size() - 1] == 'X' &&
        baseName[baseName.size() - 2] == 'F' &&
        baseName[baseName.size() - 3] == 'O') {
        baseName = baseName.substr(0,baseName.size() - 3);
    }

    checkNodeName(0, baseName,true, false, nodeName);
    
}

bool
NodeCollection::connectNodes(int inputNumber,const NodePtr& input,const NodePtr& output,bool force)
{
    ////Only called by the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    
    NodePtr existingInput = output->getRealInput(inputNumber);
    if (force && existingInput) {
        bool ok = disconnectNodes(existingInput, output);
        if (!ok) {
            return false;
        }
        if (input && input->getMaxInputCount() > 0) {
            ok = connectNodes(input->getPreferredInputForConnection(), existingInput, input);
            if (!ok) {
                return false;
            }
        }
    }
    
    if (!input) {
        return true;
    }
    
    Node::CanConnectInputReturnValue ret = output->canConnectInput(input, inputNumber);
    
    bool connectionOk = ret == Node::eCanConnectInput_ok ||
    ret == Node::eCanConnectInput_differentFPS ||
    ret == Node::eCanConnectInput_differentPars;
    
    if ( !connectionOk || !output->connectInput(input, inputNumber) ) {
        return false;
    }
    
    return true;
}


bool
NodeCollection::connectNodes(int inputNumber,const std::string & inputName,const NodePtr& output)
{
    NodesList nodes = getNodes();
    
    for (NodesList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        assert(*it);
        if ((*it)->getScriptName() == inputName) {
            return connectNodes(inputNumber,*it, output);
        }
    }
    
    return false;
}


bool
NodeCollection::disconnectNodes(const NodePtr& input,const NodePtr& output,bool autoReconnect)
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
    
    
    if (output->disconnectInput(input.get()) < 0) {
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
            bool ok = connectNodes(selectedInput, created, selected,true);
            assert(ok);
            Q_UNUSED(ok);
            ret = true;
        } else {
            ret = false;
        }
        
    } else {
        if ( !created->isOutputNode() ) {
            ///we find all the nodes that were previously connected to the selected node,
            ///and connect them to the created node instead.
            std::map<NodePtr,int> outputsConnectedToSelectedNode;
            selected->getOutputsConnectedToThisNode(&outputsConnectedToSelectedNode);
            for (std::map<NodePtr,int>::iterator it = outputsConnectedToSelectedNode.begin();
                 it != outputsConnectedToSelectedNode.end(); ++it) {
                if (it->first->getParentMultiInstanceName().empty()) {
                    bool ok = disconnectNodes(selected, it->first);
                    assert(ok);
                    ok = connectNodes(it->second, created, it->first);
                    Q_UNUSED(ok);
                    //assert(ok); Might not be ok if the disconnectNodes() action above was queued
                }
            }
        }
        ///finally we connect the created node to the selected node
        int createdInput = created->getPreferredInputForConnection();
        if (createdInput != -1) {
            bool ok = connectNodes(createdInput, selected, created);
            assert(ok);
            Q_UNUSED(ok);
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
    for (NodesList::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        if ((*it)->getScriptName_mt_safe() == name) {
            if (!recurseName.empty()) {
                NodeGroup* isGrp = (*it)->isEffectGroup();
                if (isGrp) {
                    return isGrp->getNodeByFullySpecifiedName(recurseName);
                } else {
                    NodesList children;
                    (*it)->getChildrenMultiInstance(&children);
                    for (NodesList::iterator it2 = children.begin(); it2 != children.end(); ++it2) {
                        if ((*it2)->getScriptName_mt_safe() == recurseName) {
                            return *it2;
                        }
                    }
                }
            } else {
                return *it;
            }
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
NodeCollection::fixRelativeFilePaths(const std::string& projectPathName,const std::string& newProjectPath,bool blockEval)
{
    NodesList nodes = getNodes();
    
    boost::shared_ptr<Project> project = _imp->app->getProject();
    for (NodesList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        if ((*it)->isActivated()) {
            (*it)->getEffectInstance()->beginChanges();
            
            const KnobsVec& knobs = (*it)->getKnobs();
            for (U32 j = 0; j < knobs.size(); ++j) {
                
                Knob<std::string>* isString = dynamic_cast< Knob<std::string>* >(knobs[j].get());
                KnobString* isStringKnob = dynamic_cast<KnobString*>(isString);
                if (!isString || isStringKnob || knobs[j] == project->getEnvVarKnob()) {
                    continue;
                }
                
                std::string filepath = isString->getValue();
                
                if (!filepath.empty()) {
                    if (project->fixFilePath(projectPathName, newProjectPath, filepath)) {
                        isString->setValue(filepath);
                    }
                }
            }
            (*it)->getEffectInstance()->endChanges(blockEval);
            
        
            NodeGroup* isGrp = (*it)->isEffectGroup();
            if (isGrp) {
                isGrp->fixRelativeFilePaths(projectPathName, newProjectPath, blockEval);
            }
            
        }
    }
    
}

void
NodeCollection::fixPathName(const std::string& oldName,const std::string& newName)
{
    NodesList nodes = getNodes();
    boost::shared_ptr<Project> project = _imp->app->getProject();
    for (NodesList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        if ((*it)->isActivated()) {
            const KnobsVec& knobs = (*it)->getKnobs();
            for (U32 j = 0; j < knobs.size(); ++j) {
                
                Knob<std::string>* isString = dynamic_cast< Knob<std::string>* >(knobs[j].get());
                KnobString* isStringKnob = dynamic_cast<KnobString*>(isString);
                if (!isString || isStringKnob || knobs[j] == project->getEnvVarKnob()) {
                    continue;
                }
                
                std::string filepath = isString->getValue();
                
                if (filepath.size() >= (oldName.size() + 2) &&
                    filepath[0] == '[' &&
                    filepath[oldName.size() + 1] == ']' &&
                    filepath.substr(1,oldName.size()) == oldName) {
                    
                    filepath.replace(1, oldName.size(), newName);
                    isString->setValue(filepath);
                }
            }
            
            NodeGroup* isGrp = (*it)->isEffectGroup();
            if (isGrp) {
                isGrp->fixPathName(oldName, newName);
            }
            
        }
    }
    
}

bool
NodeCollection::checkIfNodeLabelExists(const std::string & n,const Node* caller) const
{
    QMutexLocker k(&_imp->nodesMutex);
    for (NodesList::const_iterator it = _imp->nodes.begin(); it != _imp->nodes.end(); ++it) {
        if ( (it->get() != caller) && ( (*it)->getLabel_mt_safe() == n ) ) {
            return true;
        }
    }
    
    return false;
}

bool
NodeCollection::checkIfNodeNameExists(const std::string & n,const Node* caller) const
{
    QMutexLocker k(&_imp->nodesMutex);
    for (NodesList::const_iterator it = _imp->nodes.begin(); it != _imp->nodes.end(); ++it) {
        if ( (it->get() != caller) && ( (*it)->getScriptName_mt_safe() == n ) ) {
            return true;
        }
    }
    
    return false;
}

static void recomputeFrameRangeForAllReadersInternal(NodeCollection* group, int* firstFrame,int* lastFrame, bool setFrameRange)
{
    NodesList nodes = group->getNodes();
    for (NodesList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        if ((*it)->isActivated()) {
            
            if ((*it)->getEffectInstance()->isReader()) {
                double thisFirst,thislast;
                (*it)->getEffectInstance()->getFrameRange_public((*it)->getHashValue(), &thisFirst, &thislast);
                if (thisFirst != INT_MIN) {
                    *firstFrame = setFrameRange ? thisFirst : std::min(*firstFrame, (int)thisFirst);
                }
                if (thislast != INT_MAX) {
                    *lastFrame = setFrameRange ? thislast : std::max(*lastFrame, (int)thislast);
                }
            } else {
                NodeGroup* isGrp = (*it)->isEffectGroup();
                if (isGrp) {
                    recomputeFrameRangeForAllReadersInternal(isGrp, firstFrame, lastFrame, false);
                }
            }
        }
    }
}

void
NodeCollection::recomputeFrameRangeForAllReaders(int* firstFrame,int* lastFrame)
{
    recomputeFrameRangeForAllReadersInternal(this,firstFrame,lastFrame,true);
}

void
NodeCollection::forceComputeInputDependentDataOnAllTrees()
{
    NodesList nodes;
    getNodes_recursive(nodes,true);
    std::list<Project::NodesTree> trees;
    Project::extractTreesFromNodes(nodes, trees);
    
    for (NodesList::iterator it = nodes.begin(); it!=nodes.end(); ++it) {
        (*it)->markAllInputRelatedDataDirty();
    }
    
    std::list<Node*> markedNodes;
    for (std::list<Project::NodesTree>::iterator it = trees.begin(); it != trees.end(); ++it) {
        it->output.node->forceRefreshAllInputRelatedData();
    }
}


void
NodeCollection::getParallelRenderArgs(std::map<NodePtr,boost::shared_ptr<ParallelRenderArgs> >& argsMap) const
{
    NodesList nodes = getNodes();
    for (NodesList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        
        if (!(*it)->isActivated()) {
            continue;
        }
        boost::shared_ptr<ParallelRenderArgs> args = (*it)->getEffectInstance()->getParallelRenderArgsTLS();
        if (args) {
            argsMap.insert(std::make_pair(*it, args));
        }
        
        if ((*it)->isMultiInstance()) {
            
            ///If the node has children, set the thread-local storage on them too, even if they do not render, it can be useful for expressions
            ///on parameters.
            NodesList children;
            (*it)->getChildrenMultiInstance(&children);
            for (NodesList::iterator it2 = children.begin(); it2!=children.end(); ++it2) {
                boost::shared_ptr<ParallelRenderArgs> childArgs = (*it2)->getEffectInstance()->getParallelRenderArgsTLS();
                if (childArgs) {
                    argsMap.insert(std::make_pair(*it2, childArgs));
                }
            }
        }
        
        //If the node has an attached stroke, that means it belongs to the roto paint tree, hence it is not in the project.
        boost::shared_ptr<RotoContext> rotoContext = (*it)->getRotoContext();
        if (args && rotoContext) {
            for (NodesList::const_iterator it2 = args->rotoPaintNodes.begin(); it2 != args->rotoPaintNodes.end(); ++it2) {
                boost::shared_ptr<ParallelRenderArgs> args2 = (*it2)->getEffectInstance()->getParallelRenderArgsTLS();
                if (args2) {
                    argsMap.insert(std::make_pair(*it2, args2));
                }
            }
        }

        
        const NodeGroup* isGrp = (*it)->isEffectGroup();
        if (isGrp) {
            isGrp->getParallelRenderArgs(argsMap);
        }
        
        const PrecompNode* isPrecomp = dynamic_cast<const PrecompNode*>((*it)->getEffectInstance().get());
        if (isPrecomp) {
            isPrecomp->getPrecompApp()->getProject()->getParallelRenderArgs(argsMap);
        }
    }
}


struct NodeGroupPrivate
{
    mutable QMutex nodesLock; // protects inputs & outputs
    std::vector<NodeWPtr > inputs,guiInputs;
    NodesWList outputs,guiOutputs;
    bool isDeactivatingGroup;
    bool isActivatingGroup;
    bool isEditable;
    
    boost::shared_ptr<KnobButton> exportAsTemplate;
    
    NodeGroupPrivate()
    : nodesLock(QMutex::Recursive)
    , inputs()
    , guiInputs()
    , outputs()
    , guiOutputs()
    , isDeactivatingGroup(false)
    , isActivatingGroup(false)
    , isEditable(true)
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

bool
NodeGroup::getIsDeactivatingGroup() const
{
    assert(QThread::currentThread() == qApp->thread());
    return _imp->isDeactivatingGroup;
}

void
NodeGroup::setIsDeactivatingGroup(bool b)
{
    assert(QThread::currentThread() == qApp->thread());
    _imp->isDeactivatingGroup = b;
}

bool
NodeGroup::getIsActivatingGroup() const
{
    assert(QThread::currentThread() == qApp->thread());
    return _imp->isActivatingGroup;
}

void
NodeGroup::setIsActivatingGroup(bool b)
{
    assert(QThread::currentThread() == qApp->thread());
    _imp->isActivatingGroup = b;
}

NodeGroup::~NodeGroup()
{
    
}

std::string
NodeGroup::getPluginDescription() const
{
    return "Use this to nest multiple nodes into a single node. The original nodes will be replaced by the Group node and its "
    "content is available in a separate NodeGraph tab. You can add user parameters to the Group node which can drive "
    "parameters of nodes nested within the Group. To specify the outputs and inputs of the Group node, you may add multiple "
    "Input node within the group and exactly 1 Output node.";
}

void
NodeGroup::addAcceptedComponents(int /*inputNb*/,
                                std::list<ImageComponents>* comps)
{
    comps->push_back(ImageComponents::getRGBAComponents());
    comps->push_back(ImageComponents::getRGBComponents());
    comps->push_back(ImageComponents::getAlphaComponents());
}

void
NodeGroup::addSupportedBitDepth(std::list<ImageBitDepthEnum>* depths) const
{
    depths->push_back(eImageBitDepthByte);
    depths->push_back(eImageBitDepthShort);
    depths->push_back(eImageBitDepthFloat);
}

int
NodeGroup::getMaxInputCount() const
{
    return (int)_imp->inputs.size();
}

std::string
NodeGroup::getInputLabel(int inputNb) const
{
    NodePtr input;
    {
        QMutexLocker k(&_imp->nodesLock);
        if (inputNb >= (int)_imp->inputs.size() || inputNb < 0) {
            return std::string();
        }
        
        ///If the input name starts with "input" remove it, otherwise keep the full name
        
        input = _imp->inputs[inputNb].lock();
        if (!input) {
            return std::string();
        }
    }
    QString inputName(input->getLabel_mt_safe().c_str());
    if (inputName.startsWith("input",Qt::CaseInsensitive)) {
        inputName.remove(0, 5);
    }
    return inputName.toStdString();
}

double
NodeGroup::getCurrentTime() const
{
    NodePtr node = getOutputNodeInput(false);
    if (node) {
        return node->getEffectInstance()->getCurrentTime();
    }
    return EffectInstance::getCurrentTime();
}

int
NodeGroup::getCurrentView() const
{
    NodePtr node = getOutputNodeInput(false);
    if (node) {
        return node->getEffectInstance()->getCurrentView();
    }
    return EffectInstance::getCurrentView();

}

bool
NodeGroup::isInputOptional(int inputNb) const
{
    NodePtr n;
    
    {
        QMutexLocker k(&_imp->nodesLock);
        
        if (inputNb >= (int)_imp->inputs.size() || inputNb < 0) {
            return false;
        }
        
        
        n = _imp->inputs[inputNb].lock();
        if (!n) {
            return false;
        }
    }
    GroupInput* input = dynamic_cast<GroupInput*>(n->getEffectInstance().get());
    assert(input);
    KnobPtr knob = input->getKnobByName(kNatronGroupInputIsOptionalParamName);
    assert(knob);
    KnobBool* isBool = dynamic_cast<KnobBool*>(knob.get());
    assert(isBool);
    return isBool->getValue();
}

bool
NodeGroup::isInputMask(int inputNb) const
{
    
    NodePtr n;
    
    {
        QMutexLocker k(&_imp->nodesLock);
        
        if (inputNb >= (int)_imp->inputs.size() || inputNb < 0) {
            return false;
        }
        
        
        n = _imp->inputs[inputNb].lock();
        if (!n) {
            return false;
        }
    }
    GroupInput* input = dynamic_cast<GroupInput*>(n->getEffectInstance().get());
    assert(input);
    KnobPtr knob = input->getKnobByName(kNatronGroupInputIsMaskParamName);
    assert(knob);
    KnobBool* isBool = dynamic_cast<KnobBool*>(knob.get());
    assert(isBool);
    return isBool->getValue();
}

void
NodeGroup::initializeKnobs()
{
    KnobPtr nodePage = getKnobByName(NATRON_PARAMETER_PAGE_NAME_EXTRA);
    assert(nodePage);
    KnobPage* isPage = dynamic_cast<KnobPage*>(nodePage.get());
    assert(isPage);
    _imp->exportAsTemplate = AppManager::createKnob<KnobButton>(this, "Export as PyPlug");
    _imp->exportAsTemplate->setName("exportAsPyPlug");
    _imp->exportAsTemplate->setHintToolTip("Export this group as a Python group script (PyPlug) that can be shared and/or later "
                                           "on re-used as a plug-in.");
    isPage->addKnob(_imp->exportAsTemplate);
}

void
NodeGroup::notifyNodeDeactivated(const NodePtr& node)
{
    
    if (getIsDeactivatingGroup()) {
        return;
    }
    NodePtr thisNode = getNode();
    
    {
        QMutexLocker k(&_imp->nodesLock);
        GroupInput* isInput = dynamic_cast<GroupInput*>(node->getEffectInstance().get());
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
        GroupOutput* isOutput = dynamic_cast<GroupOutput*>(node->getEffectInstance().get());
        if (isOutput) {
            for (NodesWList::iterator it = _imp->outputs.begin(); it !=_imp->outputs.end(); ++it) {
                if (it->lock()->getEffectInstance().get() == isOutput) {
                    _imp->outputs.erase(it);
                    break;
                }
            }
        }
        
        ///Sync gui inputs/outputs
        _imp->guiInputs = _imp->inputs;
        _imp->guiOutputs = _imp->outputs;
    }
    
    ///Notify outputs of the group nodes that their inputs may have changed
    const NodesWList& outputs = thisNode->getOutputs();
    for (NodesWList::const_iterator it = outputs.begin(); it != outputs.end(); ++it) {
        NodePtr output = it->lock();
        if (!output) {
            continue;
        }
        int idx = output->getInputIndex(thisNode.get());
        assert(idx != -1);
        output->onInputChanged(idx);
    }

}

void
NodeGroup::notifyNodeActivated(const NodePtr& node)
{
    
    if (getIsActivatingGroup()) {
        return;
    }
    
    NodePtr thisNode = getNode();
    
    {
        QMutexLocker k(&_imp->nodesLock);
        GroupInput* isInput = dynamic_cast<GroupInput*>(node->getEffectInstance().get());
        if (isInput) {
            _imp->inputs.push_back(node);
            _imp->guiInputs.push_back(node);
            thisNode->initializeInputs();
        }
        GroupOutput* isOutput = dynamic_cast<GroupOutput*>(node->getEffectInstance().get());
        if (isOutput) {
            _imp->outputs.push_back(node);
            _imp->guiOutputs.push_back(node);
        }
    }
    ///Notify outputs of the group nodes that their inputs may have changed
    const NodesWList& outputs = thisNode->getOutputs();
    for (NodesWList::const_iterator it = outputs.begin(); it != outputs.end(); ++it) {
        NodePtr output = it->lock();
        if (!output) {
            continue;
        }
        int idx = output->getInputIndex(thisNode.get());
        assert(idx != -1);
        output->onInputChanged(idx);
    }
}

void
NodeGroup::notifyInputOptionalStateChanged(const NodePtr& /*node*/)
{
    getNode()->initializeInputs();
}

void
NodeGroup::notifyInputMaskStateChanged(const NodePtr& /*node*/)
{
    getNode()->initializeInputs();
}

void
NodeGroup::notifyNodeNameChanged(const NodePtr& node)
{
    GroupInput* isInput = dynamic_cast<GroupInput*>(node->getEffectInstance().get());
    if (isInput) {
        getNode()->initializeInputs();
    }
}

void
NodeGroup::dequeueConnexions()
{
    QMutexLocker k(&_imp->nodesLock);
    _imp->inputs = _imp->guiInputs;
    _imp->outputs = _imp->guiOutputs;
}

NodePtr
NodeGroup::getOutputNode(bool useGuiConnexions) const
{
    QMutexLocker k(&_imp->nodesLock);
    ///A group can only have a single output.
    if ((!useGuiConnexions && _imp->outputs.empty()) || (useGuiConnexions && _imp->guiOutputs.empty())) {
        return NodePtr();
    }
    return useGuiConnexions ? _imp->guiOutputs.front().lock() : _imp->outputs.front().lock();
}



NodePtr
NodeGroup::getOutputNodeInput(bool useGuiConnexions) const
{
    NodePtr output = getOutputNode(useGuiConnexions);
    if (output) {
        return useGuiConnexions ? output->getGuiInput(0) : output->getInput(0);
    }
    return NodePtr();
}

NodePtr
NodeGroup::getRealInputForInput(bool useGuiConnexions,const NodePtr& input) const
{
    
    {
        QMutexLocker k(&_imp->nodesLock);
        if (!useGuiConnexions) {
            for (U32 i = 0; i < _imp->inputs.size(); ++i) {
                if (_imp->inputs[i].lock() == input) {
                    return getNode()->getInput(i);
                }
            }
        } else {
            for (U32 i = 0; i < _imp->guiInputs.size(); ++i) {
                if (_imp->guiInputs[i].lock() == input) {
                    return getNode()->getGuiInput(i);
                }
            }

        }
    }
    return NodePtr();
}

void
NodeGroup::getInputsOutputs(NodesList* nodes, bool useGuiConnexions) const
{
    QMutexLocker k(&_imp->nodesLock);
    if (!useGuiConnexions) {
        for (U32 i = 0; i < _imp->inputs.size(); ++i) {
            NodesWList outputs;
            NodePtr input = _imp->inputs[i].lock();
            if (!input) {
                continue;
            }
            input->getOutputs_mt_safe(outputs);
            for (NodesWList::iterator it = outputs.begin(); it!=outputs.end(); ++it) {
                NodePtr node = it->lock();
                if (node) {
                    nodes->push_back(node);
                }
            }
        }
    } else {
        for (U32 i = 0; i < _imp->guiInputs.size(); ++i) {
            NodesWList outputs;
            NodePtr input = _imp->guiInputs[i].lock();
            if (!input) {
                continue;
            }
            input->getOutputs_mt_safe(outputs);
            for (NodesWList::iterator it = outputs.begin(); it!=outputs.end(); ++it) {
                NodePtr node = it->lock();
                if (node) {
                    nodes->push_back(node);
                }
            }
        }
    }
}

void
NodeGroup::getInputs(std::vector<NodePtr >* inputs,bool useGuiConnexions) const
{
    QMutexLocker k(&_imp->nodesLock);
    if (!useGuiConnexions) {
        for (U32 i = 0; i < _imp->inputs.size(); ++i) {
            NodePtr input = _imp->inputs[i].lock();
            if (!input) {
                continue;
            }
            inputs->push_back(input);
        }
    } else {
        for (U32 i = 0; i < _imp->guiInputs.size(); ++i) {
            NodePtr input = _imp->guiInputs[i].lock();
            if (!input) {
                continue;
            }
            inputs->push_back(input);
        }
    }
}

void
NodeGroup::knobChanged(KnobI* k,ValueChangedReasonEnum /*reason*/,
                 int /*view*/,
                 double /*time*/,
                 bool /*originatedFromMainThread*/)
{
    if (k == _imp->exportAsTemplate.get()) {
        boost::shared_ptr<NodeGuiI> gui_i = getNode()->getNodeGui();
        if (gui_i) {
            gui_i->exportGroupAsPythonScript();
        }
    } 
}

void
NodeGroup::setSubGraphEditable(bool editable)
{
    assert(QThread::currentThread() == qApp->thread());
    _imp->isEditable = editable;
    Q_EMIT graphEditableChanged(editable);
}

bool
NodeGroup::isSubGraphEditable() const
{
    assert(QThread::currentThread() == qApp->thread());
    return _imp->isEditable;
}


static QString escapeString(const QString& str)
{
    QString ret;
    for (int i = 0; i < str.size(); ++i) {
        
        if (i == 0 || str[i -1] != QChar('\\')) {
            if (str[i] == '\\') {
                ret.append('\\');
                ret.append('\\');
            } else if (str[i] == '"') {
                ret.append('\\');
                ret.append('\"');
            } else if (str[i] == '\'') {
                ret.append('\\');
                ret.append('\'');
            } else if (str[i] == '\n') {
                ret.append('\\');
                ret.append('n');
            } else if (str[i] == '\t') {
                ret.append('\\');
                ret.append('t');
            } else if (str[i] == '\r') {
                ret.append('\\');
                ret.append('r');
            } else {
                ret.append(str[i]);
            }
        } else {
            ret.append(str[i]);
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
//#define NUM(n) QString::number(n)
#define NUM_INT(n) QString::number(n, 10)
#define NUM_COLOR(n) QString::number(n, 'g', 4)
#define NUM_PIXEL(n) QString::number(n, 'f', 0)
#define NUM_VALUE(n) QString::number(n, 'g', 16)
#define NUM_TIME(n) QString::number(n, 'g', 16)

static bool exportKnobValues(int indentLevel,
                             const KnobPtr knob,
                             const QString& paramFullName,
                             bool mustDefineParam,
                             QTextStream& ts)
{
    
    bool hasExportedValue = false;
    
    Knob<std::string>* isStr = dynamic_cast<Knob<std::string>*>(knob.get());
    AnimatingKnobStringHelper* isAnimatedStr = dynamic_cast<AnimatingKnobStringHelper*>(knob.get());
    Knob<double>* isDouble = dynamic_cast<Knob<double>*>(knob.get());
    Knob<int>* isInt = dynamic_cast<Knob<int>*>(knob.get());
    Knob<bool>* isBool = dynamic_cast<Knob<bool>*>(knob.get());
    KnobParametric* isParametric = dynamic_cast<KnobParametric*>(knob.get());
    KnobChoice* isChoice = dynamic_cast<KnobChoice*>(knob.get());
    KnobGroup* isGrp = dynamic_cast<KnobGroup*>(knob.get());
    KnobString* isStringKnob = dynamic_cast<KnobString*>(knob.get());
    
    ///Don't export this kind of parameter. Mainly this is the html label of the node which is 99% of times empty
    if (isStringKnob &&
        isStringKnob->isMultiLine() &&
        isStringKnob->usesRichText() &&
        !isStringKnob->hasContentWithoutHtmlTags() &&
        !isStringKnob->isAnimationEnabled() &&
        isStringKnob->getExpression(0).empty()) {
        return false;
    }
    
    int innerIdent = mustDefineParam ? 2 : 1;
    
    for (int i = 0; i < knob->getDimension(); ++i) {
        
        if (isParametric) {
            
            if (!hasExportedValue) {
                hasExportedValue = true;
                if (mustDefineParam) {
                    WRITE_INDENT(indentLevel); WRITE_STRING("param = " + paramFullName);
                    WRITE_INDENT(indentLevel); WRITE_STRING("if param is not None:");
                }
            }
            boost::shared_ptr<Curve> curve = isParametric->getParametricCurve(i);
            double r,g,b;
            isParametric->getCurveColor(i, &r, &g, &b);
            WRITE_INDENT(innerIdent); WRITE_STRING("param.setCurveColor(" + NUM_INT(i) + ", " +
                                          NUM_COLOR(r) + ", " + NUM_COLOR(g) + ", " + NUM_COLOR(b) + ")");
            if (curve) {
                KeyFrameSet keys = curve->getKeyFrames_mt_safe();
                int c = 0;
                for (KeyFrameSet::iterator it3 = keys.begin(); it3 != keys.end(); ++it3, ++c) {
                    WRITE_INDENT(innerIdent); WRITE_STRING("param.setNthControlPoint(" + NUM_INT(i) + ", " +
                                                  NUM_INT(c) + ", " + NUM_TIME(it3->getTime()) + ", " +
                                                  NUM_VALUE(it3->getValue()) + ", " + NUM_VALUE(it3->getLeftDerivative())
                                                  + ", " + NUM_VALUE(it3->getRightDerivative()) + ")");
                }
                
            }
        } else { // !isParametric
            boost::shared_ptr<Curve> curve = knob->getCurve(ViewIdx(0), i, true);
            if (curve) {
                KeyFrameSet keys = curve->getKeyFrames_mt_safe();
                
                if (!keys.empty()) {
                    if (!hasExportedValue) {
                        hasExportedValue = true;
                        if (mustDefineParam) {
                            WRITE_INDENT(indentLevel); WRITE_STRING("param = " + paramFullName);
                            WRITE_INDENT(indentLevel); WRITE_STRING("if param is not None:");
                        }
                    }
                }
                
                for (KeyFrameSet::iterator it3 = keys.begin(); it3 != keys.end(); ++it3) {
                    if (isAnimatedStr) {
                        std::string value = isAnimatedStr->getValueAtTime(it3->getTime(),i, ViewIdx(0), true);
                        WRITE_INDENT(innerIdent); WRITE_STRING("param.setValueAtTime(" + ESC(value) + ", "
                                                      + NUM_TIME(it3->getTime())+ ")");
                        
                    } else if (isBool) {
                        int v = std::min(1., std::max(0., std::floor(it3->getValue() + 0.5)));
                        QString vStr = v ? "True" : "False";
                        WRITE_INDENT(innerIdent); WRITE_STRING("param.setValueAtTime(" + vStr + ", "
                                                      + NUM_TIME(it3->getTime())  + ")");
                    } else if (isChoice) {
                        WRITE_INDENT(innerIdent); WRITE_STRING("param.setValueAtTime(" + NUM_INT((int)it3->getValue()) + ", "
                                                      + NUM_TIME(it3->getTime()) + ")");
                    } else {
                        WRITE_INDENT(innerIdent); WRITE_STRING("param.setValueAtTime(" + NUM_VALUE(it3->getValue()) + ", "
                                                      + NUM_TIME(it3->getTime()) + ", " + NUM_INT(i) + ")");

                    }
                }
            }
            
            if ((!curve || curve->getKeyFramesCount() == 0) && knob->hasModifications(i)) {
                if (!hasExportedValue) {
                    hasExportedValue = true;
                    if (mustDefineParam) {
                        WRITE_INDENT(indentLevel); WRITE_STRING("param = " + paramFullName);
                        WRITE_INDENT(indentLevel); WRITE_STRING("if param is not None:");
                    }
                }
                
                if (isGrp) {
                    int v = std::min(1., std::max(0.,std::floor(isGrp->getValue(i, ViewIdx(0), true) + 0.5)));
                    QString vStr = v ? "True" : "False";
                    WRITE_INDENT(innerIdent); WRITE_STRING("param.setOpened(" + vStr + ")");
                } else if (isStr) {
                    std::string v = isStr->getValue(i, ViewIdx(0), true);
                    WRITE_INDENT(innerIdent); WRITE_STRING("param.setValue(" + ESC(v)  + ")");
                } else if (isDouble) {
                    double v = isDouble->getValue(i, ViewIdx(0), true);
                    WRITE_INDENT(innerIdent); WRITE_STRING("param.setValue(" + NUM_VALUE(v) + ", " + NUM_INT(i) + ")");
                } else if (isChoice) {
                    WRITE_INDENT(innerIdent); WRITE_STATIC_LINE("options = param.getOptions()");
                    WRITE_INDENT(innerIdent); WRITE_STATIC_LINE("foundOption = False");
                    WRITE_INDENT(innerIdent); WRITE_STATIC_LINE("for i in range(len(options)):");
                    WRITE_INDENT(innerIdent + 1); WRITE_STRING("if options[i] == " + ESC(isChoice->getActiveEntryText_mt_safe()) + ":");
                    WRITE_INDENT(innerIdent + 2); WRITE_STATIC_LINE("param.setValue(i)");
                    WRITE_INDENT(innerIdent + 2); WRITE_STATIC_LINE("foundOption = True");
                    WRITE_INDENT(innerIdent + 2); WRITE_STATIC_LINE("break");
                    WRITE_INDENT(innerIdent); WRITE_STATIC_LINE("if not foundOption:");
                    std::stringstream error;
                    error << "Could not set option for parameter " << isChoice->getName() ;
                    KnobHolder* holder = isChoice->getHolder();
                    EffectInstance* instance = dynamic_cast<EffectInstance*>(holder);
                    if (instance) {
                        error << " of node " << instance->getNode()->getFullyQualifiedName();
                    }
                    WRITE_INDENT(innerIdent + 1); WRITE_STRING("app.writeToScriptEditor(" + ESC(error.str()) + ")");
                } else if (isInt) {
                    int v = isInt->getValue(i, ViewIdx(0), true);
                    WRITE_INDENT(innerIdent); WRITE_STRING("param.setValue(" + NUM_INT(v) + ", " + NUM_INT(i) + ")");
                } else if (isBool) {
                    int v = std::min(1., std::max(0.,std::floor(isBool->getValue(i, ViewIdx(0), true) + 0.5)));
                    QString vStr = v ? "True" : "False";
                    WRITE_INDENT(innerIdent); WRITE_STRING("param.setValue(" + vStr + ")");
                }
            } // if ((!curve || curve->getKeyFramesCount() == 0) && knob->hasModifications(i)) {
            
        } // if (isParametric) {
        
    } // for (int i = 0; i < (*it2)->getDimension(); ++i)
    
    bool isSecretByDefault = knob->getDefaultIsSecret();
    if (knob->isUserKnob() && isSecretByDefault) {
        if (!hasExportedValue) {
            hasExportedValue = true;
            if (mustDefineParam) {
                WRITE_INDENT(indentLevel); WRITE_STRING("param = " + paramFullName);
                WRITE_INDENT(indentLevel); WRITE_STRING("if param is not None:");
            }
        }
        
        WRITE_INDENT(innerIdent); WRITE_STRING("param.setVisibleByDefault(False)");

    }
    
    if (knob->isUserKnob()) {
        bool isSecret = knob->getIsSecret();
        if (isSecret != isSecretByDefault) {
            if (!hasExportedValue) {
                hasExportedValue = true;
                if (mustDefineParam) {
                    WRITE_INDENT(indentLevel); WRITE_STRING("param = " + paramFullName);
                    WRITE_INDENT(indentLevel); WRITE_STRING("if param is not None:");
                }
            }
            
            QString str("param.setVisible(");
            if (isSecret) {
                str += "False";
            } else {
                str += "True";
            }
            str += ")";
            WRITE_INDENT(innerIdent); WRITE_STRING(str);
        }
        
        bool enabledByDefault = knob->isDefaultEnabled(0);
        if (!enabledByDefault) {
            if (!hasExportedValue) {
                hasExportedValue = true;
                if (mustDefineParam) {
                    WRITE_INDENT(indentLevel); WRITE_STRING("param = " + paramFullName);
                    WRITE_INDENT(indentLevel); WRITE_STRING("if param is not None:");
                }
            }
            
            WRITE_INDENT(innerIdent); WRITE_STRING("param.setEnabledByDefault(False)");
        }
        
        for (int i = 0; i < knob->getDimension(); ++i) {
            
            bool isEnabled = knob->isEnabled(i);
            if (isEnabled != enabledByDefault) {
                if (!hasExportedValue) {
                    hasExportedValue = true;
                    if (mustDefineParam) {
                        WRITE_INDENT(indentLevel); WRITE_STRING("param = " + paramFullName);
                        WRITE_INDENT(indentLevel); WRITE_STRING("if param is not None:");
                    }
                }
                
                QString str("param.setEnabled(");
                if (isEnabled) {
                    str += "True";
                } else {
                    str += "False";
                }
                str += ", ";
                str += NUM_INT(i);
                str += ")";
                WRITE_INDENT(innerIdent); WRITE_STRING(str);
            }
        }
    } // isuserknob
    
    if (mustDefineParam && hasExportedValue) {
        WRITE_INDENT(innerIdent); WRITE_STRING("del param");
    }
    
    return hasExportedValue;
}

static void exportUserKnob(int indentLevel,const KnobPtr& knob,const QString& fullyQualifiedNodeName,KnobGroup* group,KnobPage* page,QTextStream& ts)
{
    KnobInt* isInt = dynamic_cast<KnobInt*>(knob.get());
    KnobDouble* isDouble = dynamic_cast<KnobDouble*>(knob.get());
    KnobBool* isBool = dynamic_cast<KnobBool*>(knob.get());
    KnobChoice* isChoice = dynamic_cast<KnobChoice*>(knob.get());
    KnobColor* isColor = dynamic_cast<KnobColor*>(knob.get());
    KnobString* isStr = dynamic_cast<KnobString*>(knob.get());
    KnobFile* isFile = dynamic_cast<KnobFile*>(knob.get());
    KnobOutputFile* isOutFile = dynamic_cast<KnobOutputFile*>(knob.get());
    KnobPath* isPath = dynamic_cast<KnobPath*>(knob.get());
    KnobGroup* isGrp = dynamic_cast<KnobGroup*>(knob.get());
    KnobButton* isButton = dynamic_cast<KnobButton*>(knob.get());
    KnobSeparator* isSep = dynamic_cast<KnobSeparator*>(knob.get());
    KnobParametric* isParametric = dynamic_cast<KnobParametric*>(knob.get());
    
    
    boost::shared_ptr<KnobI > aliasedParam;
    {
        KnobI::ListenerDimsMap listeners;
        knob->getListeners(listeners);
        if (!listeners.empty()) {
            KnobPtr listener = listeners.begin()->first.lock();
            if (listener && listener->getAliasMaster() == knob) {
                aliasedParam = listener;
            }
        }
    }
    
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
        WRITE_INDENT(indentLevel); WRITE_STRING("param = " + fullyQualifiedNodeName + createToken + ESC(isInt->getName()) +
                                      ", " + ESC(isInt->getLabel()) + ")");
        
        
        std::vector<int> defaultValues = isInt->getDefaultValues_mt_safe();
      
        
        assert((int)defaultValues.size() == isInt->getDimension());
        for (int i = 0; i < isInt->getDimension() ; ++i) {
            int min = isInt->getMinimum(i);
            int max = isInt->getMaximum(i);
            int dMin = isInt->getDisplayMinimum(i);
            int dMax = isInt->getDisplayMaximum(i);
            if (min != INT_MIN) {
                    WRITE_INDENT(indentLevel); WRITE_STRING("param.setMinimum(" + NUM_INT(min) + ", " +
                                              NUM_INT(i) + ")");
            }
            if (max != INT_MAX) {
                WRITE_INDENT(indentLevel); WRITE_STRING("param.setMaximum(" + NUM_INT(max) + ", " +
                                              NUM_INT(i) + ")");
            }
            if (dMin != INT_MIN) {
                    WRITE_INDENT(indentLevel); WRITE_STRING("param.setDisplayMinimum(" + NUM_INT(dMin) + ", " +
                                              NUM_INT(i) + ")");
            }
            if (dMax != INT_MAX) {
                    WRITE_INDENT(indentLevel); WRITE_STRING("param.setDisplayMaximum(" + NUM_INT(dMax) + ", " +
                                              NUM_INT(i) + ")");
            }
            WRITE_INDENT(indentLevel); WRITE_STRING("param.setDefaultValue(" + NUM_INT(defaultValues[i]) + ", " + NUM_INT(i) + ")");
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
        WRITE_INDENT(indentLevel); WRITE_STRING("param = " + fullyQualifiedNodeName + createToken + ESC(isDouble->getName()) +
                                      ", " + ESC(isDouble->getLabel()) + ")");

        std::vector<double> defaultValues = isDouble->getDefaultValues_mt_safe();
        assert((int)defaultValues.size() == isDouble->getDimension());
        for (int i = 0; i < isDouble->getDimension() ; ++i) {
            double min = isDouble->getMinimum(i);
            double max = isDouble->getMaximum(i);
            double dMin = isDouble->getDisplayMinimum(i);
            double dMax = isDouble->getDisplayMaximum(i);
            if (min != -DBL_MAX) {
                    WRITE_INDENT(indentLevel); WRITE_STRING("param.setMinimum(" + NUM_VALUE(min) + ", " +
                                              NUM_INT(i) + ")");
            }
            if (max != DBL_MAX) {
                    WRITE_INDENT(indentLevel); WRITE_STRING("param.setMaximum(" + NUM_VALUE(max) + ", " +
                                              NUM_INT(i) + ")");
            }
            if (dMin != -DBL_MAX) {
                    WRITE_INDENT(indentLevel); WRITE_STRING("param.setDisplayMinimum(" + NUM_VALUE(dMin) + ", " +
                                              NUM_INT(i) + ")");
            }
            if (dMax != DBL_MAX) {
                    WRITE_INDENT(indentLevel); WRITE_STRING("param.setDisplayMaximum(" + NUM_VALUE(dMax) + ", " +
                                              NUM_INT(i) + ")");
            }
            if (defaultValues[i] != 0.) {
                WRITE_INDENT(indentLevel); WRITE_STRING("param.setDefaultValue(" + NUM_VALUE(defaultValues[i]) + ", " + NUM_INT(i) + ")");
            }
        }

    } else if (isBool) {
        WRITE_INDENT(indentLevel); WRITE_STRING("param = " + fullyQualifiedNodeName + ".createBooleanParam(" + ESC(isBool->getName()) +
                                      ", " + ESC(isBool->getLabel()) + ")");

        std::vector<bool> defaultValues = isBool->getDefaultValues_mt_safe();
        assert((int)defaultValues.size() == isBool->getDimension());
        
        if (defaultValues[0]) {
            WRITE_INDENT(indentLevel); WRITE_STRING("param.setDefaultValue(True)");
        }

    } else if (isChoice) {
        WRITE_INDENT(indentLevel); WRITE_STRING("param = " + fullyQualifiedNodeName + ".createChoiceParam(" +
                                      ESC(isChoice->getName()) +
                                      ", " + ESC(isChoice->getLabel()) + ")");
        
        KnobChoice* aliasedIsChoice = dynamic_cast<KnobChoice*>(aliasedParam.get());
        
        if (!aliasedIsChoice) {
            std::vector<std::string> entries = isChoice->getEntries_mt_safe();
            std::vector<std::string> helps = isChoice->getEntriesHelp_mt_safe();
            if (entries.size() > 0) {
                if (helps.empty()) {
                    for (U32 i = 0; i < entries.size(); ++i) {
                        helps.push_back("");
                    }
                }
                WRITE_INDENT(indentLevel); ts << "entries = [ (" << ESC(entries[0]) << ", " << ESC(helps[0]) << "),\n";
                for (U32 i = 1; i < entries.size(); ++i) {
                    QString endToken = (i == entries.size() - 1) ? ")]" : "),";
                    WRITE_INDENT(indentLevel); WRITE_STRING("(" + ESC(entries[i]) + ", " + ESC(helps[i]) + endToken);
                }
                WRITE_INDENT(indentLevel); WRITE_STATIC_LINE("param.setOptions(entries)");
                WRITE_INDENT(indentLevel); WRITE_STATIC_LINE("del entries");
            }
            std::vector<int> defaultValues = isChoice->getDefaultValues_mt_safe();
            assert((int)defaultValues.size() == isChoice->getDimension());
            if (defaultValues[0] != 0) {
                std::string entryStr = isChoice->getEntry(defaultValues[0]);
                WRITE_INDENT(indentLevel); WRITE_STRING("param.setDefaultValue(" + ESC(entryStr) + ")");
                
            }
        } else {
            std::vector<int> defaultValues = isChoice->getDefaultValues_mt_safe();
            assert((int)defaultValues.size() == isChoice->getDimension());
            if (defaultValues[0] != 0) {
                WRITE_INDENT(indentLevel); WRITE_STRING("param.setDefaultValue(" + NUM_INT(defaultValues[0]) + ")");
                
            }
        }
        
        
        
        
    } else if (isColor) {
        QString hasAlphaStr = (isColor->getDimension() == 4) ? "True" : "False";
        WRITE_INDENT(indentLevel); WRITE_STRING("param = " + fullyQualifiedNodeName + ".createColorParam(" + ESC(isColor->getName()) +
                                      ", " + ESC(isColor->getLabel()) + ", " + hasAlphaStr +  ")");
        
        
        std::vector<double> defaultValues = isColor->getDefaultValues_mt_safe();
        assert((int)defaultValues.size() == isColor->getDimension());
        
        for (int i = 0; i < isColor->getDimension() ; ++i) {
            double min = isColor->getMinimum(i);
            double max = isColor->getMaximum(i);
            double dMin = isColor->getDisplayMinimum(i);
            double dMax = isColor->getDisplayMaximum(i);
            if (min != -DBL_MAX) {
                    WRITE_INDENT(indentLevel); WRITE_STRING("param.setMinimum(" + NUM_VALUE(min) + ", " +
                                              NUM_INT(i) + ")");
            }
            if (max != DBL_MAX) {
                    WRITE_INDENT(indentLevel); WRITE_STRING("param.setMaximum(" + NUM_VALUE(max) + ", " +
                                              NUM_INT(i) + ")");
            }
            if (dMin != -DBL_MAX) {
                    WRITE_INDENT(indentLevel); WRITE_STRING("param.setDisplayMinimum(" + NUM_VALUE(dMin) + ", " +
                                              NUM_INT(i) + ")");
            }
            if (dMax != DBL_MAX) {
                    WRITE_INDENT(indentLevel); WRITE_STRING("param.setDisplayMaximum(" + NUM_VALUE(dMax) + ", " +
                                              NUM_INT(i) + ")");
            }
            if (defaultValues[i] != 0.) {
                    WRITE_INDENT(indentLevel); WRITE_STRING("param.setDefaultValue(" + NUM_VALUE(defaultValues[i]) + ", " + NUM_INT(i) + ")");
            }
        }

    } else if (isButton) {
        WRITE_INDENT(indentLevel); WRITE_STRING("param = " + fullyQualifiedNodeName + ".createButtonParam(" +
                                      ESC(isButton->getName()) +
                                      ", " + ESC(isButton->getLabel()) + ")");

    } else if (isSep) {
        WRITE_INDENT(indentLevel); WRITE_STRING("param = " + fullyQualifiedNodeName + ".createSeparatorParam(" +
                                                ESC(isSep->getName()) +
                                                ", " + ESC(isSep->getLabel()) + ")");
        
    } else if (isStr) {
        WRITE_INDENT(indentLevel); WRITE_STRING("param = " + fullyQualifiedNodeName + ".createStringParam(" +
                                      ESC(isStr->getName()) +
                                      ", " + ESC(isStr->getLabel()) + ")");
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
        WRITE_INDENT(indentLevel); WRITE_STRING("param.setType(NatronEngine.StringParam.TypeEnum." + typeStr + ")");
        
        std::vector<std::string> defaultValues = isStr->getDefaultValues_mt_safe();
        assert((int)defaultValues.size() == isStr->getDimension());
        QString def(defaultValues[0].c_str());
        if (!def.isEmpty()) {
                WRITE_INDENT(indentLevel); WRITE_STRING("param.setDefaultValue(" + ESC(def) + ")");
            
        }
        
    } else if (isFile) {
        WRITE_INDENT(indentLevel); WRITE_STRING("param = " + fullyQualifiedNodeName + ".createFileParam(" + ESC(isFile->getName()) +
                                      ", " + ESC(isFile->getLabel()) + ")");
        QString seqStr = isFile->isInputImageFile() ? "True" : "False";
        WRITE_INDENT(indentLevel); WRITE_STRING("param.setSequenceEnabled("+ seqStr + ")");
        
        std::vector<std::string> defaultValues = isFile->getDefaultValues_mt_safe();
        assert((int)defaultValues.size() == isFile->getDimension());
        QString def(defaultValues[0].c_str());
        if (!def.isEmpty()) {
                WRITE_INDENT(indentLevel); WRITE_STRING("param.setDefaultValue(" + def + ")");
        }
        
    } else if (isOutFile) {
        WRITE_INDENT(indentLevel); WRITE_STRING("param = " + fullyQualifiedNodeName + ".createOutputFileParam(" +
                                      ESC(isOutFile->getName()) +
                                      ", " + ESC(isOutFile->getLabel()) + ")");
        assert(isOutFile); 
        QString seqStr = isOutFile->isOutputImageFile() ? "True" : "False";
        WRITE_INDENT(indentLevel); WRITE_STRING("param.setSequenceEnabled("+ seqStr + ")");

        std::vector<std::string> defaultValues = isOutFile->getDefaultValues_mt_safe();
        assert((int)defaultValues.size() == isOutFile->getDimension());
        QString def(defaultValues[0].c_str());
        if (!def.isEmpty()) {
                WRITE_INDENT(indentLevel); WRITE_STRING("param.setDefaultValue(" + ESC(def) + ")");
        }

    } else if (isPath) {
        WRITE_INDENT(indentLevel); WRITE_STRING("param = " + fullyQualifiedNodeName + ".createPathParam(" +
                                      ESC(isPath->getName()) +
                                      ", " + ESC(isPath->getLabel()) + ")");
        if (isPath->isMultiPath()) {
            WRITE_INDENT(indentLevel); WRITE_STRING("param.setAsMultiPathTable()");
        }

        std::vector<std::string> defaultValues = isPath->getDefaultValues_mt_safe();
        assert((int)defaultValues.size() == isPath->getDimension());
        QString def(defaultValues[0].c_str());
        if (!def.isEmpty()) {
                WRITE_INDENT(indentLevel); WRITE_STRING("param.setDefaultValue(" + ESC(def) + ")");
        }

    } else if (isGrp) {
        WRITE_INDENT(indentLevel); WRITE_STRING("param = " + fullyQualifiedNodeName + ".createGroupParam(" +
                                      ESC(isGrp->getName()) +
                                      ", " + ESC(isGrp->getLabel()) + ")");
        if (isGrp->isTab()) {
            WRITE_INDENT(indentLevel); WRITE_STRING("param.setAsTab()");
        }

    } else if (isParametric) {
        WRITE_INDENT(indentLevel); WRITE_STRING("param = " + fullyQualifiedNodeName + ".createParametricParam(" +
                                      ESC(isParametric->getName()) +
                                      ", " + ESC(isParametric->getLabel()) +  ", " +
                                      NUM_INT(isParametric->getDimension()) + ")");
    }
    
    WRITE_STATIC_LINE("");

    if (group) {
        QString grpFullName = fullyQualifiedNodeName + "." + QString(group->getName().c_str());
        WRITE_INDENT(indentLevel); WRITE_STATIC_LINE("# Add the param to the group, no need to add it to the page");
        WRITE_INDENT(indentLevel); WRITE_STRING(grpFullName + ".addParam(param)");
    } else {
        assert(page);
        QString pageFullName = fullyQualifiedNodeName + "." + QString(page->getName().c_str());
        WRITE_INDENT(indentLevel); WRITE_STATIC_LINE("# Add the param to the page");
        WRITE_INDENT(indentLevel); WRITE_STRING(pageFullName + ".addParam(param)");
    }
    
    WRITE_STATIC_LINE("");
    WRITE_INDENT(indentLevel); WRITE_STATIC_LINE("# Set param properties");

    QString help(knob->getHintToolTip().c_str());
    if (!aliasedParam || aliasedParam->getHintToolTip() != knob->getHintToolTip()) {
        WRITE_INDENT(indentLevel); WRITE_STRING("param.setHelp(" + ESC(help) + ")");
    }
    
    
    bool previousHasNewLineActivated = true;
    KnobsVec children;
    if (group) {
        children = group->getChildren();
    } else if (page) {
        children = page->getChildren();
    }
    for (U32 i = 0; i < children.size(); ++i) {
        if (children[i] == knob) {
            if (i > 0) {
                previousHasNewLineActivated = children[i - 1]->isNewLineActivated();
            }
            break;
        }
    }
    
    if (previousHasNewLineActivated) {
        WRITE_INDENT(indentLevel); WRITE_STRING("param.setAddNewLine(True)");
    } else {
        WRITE_INDENT(indentLevel); WRITE_STRING("param.setAddNewLine(False)");
    }
    
    if (!knob->getIsPersistant()) {
        WRITE_INDENT(indentLevel); WRITE_STRING("param.setPersistant(False)");
    }
    
    if (!knob->getEvaluateOnChange()) {
        WRITE_INDENT(indentLevel); WRITE_STRING("param.setEvaluateOnChange(False)");
    }
    
    if (knob->canAnimate()) {
        QString animStr = knob->isAnimationEnabled() ? "True" : "False";
        WRITE_INDENT(indentLevel); WRITE_STRING("param.setAnimationEnabled(" + animStr + ")");
    }

    exportKnobValues(indentLevel,knob,"", false, ts);
    WRITE_INDENT(indentLevel); WRITE_STRING(fullyQualifiedNodeName + "." + QString(knob->getName().c_str()) + " = param");
    WRITE_INDENT(indentLevel); WRITE_STATIC_LINE("del param");
    
    WRITE_STATIC_LINE("");
    
    if (isGrp) {
        KnobsVec children =  isGrp->getChildren();
        for (KnobsVec::const_iterator it3 = children.begin(); it3 != children.end(); ++it3) {
            exportUserKnob(indentLevel,*it3, fullyQualifiedNodeName, isGrp, page, ts);
        }
    }
    
}

static void exportBezierPointAtTime(int indentLevel,
                                    const boost::shared_ptr<BezierCP>& point,
                                    bool isFeather,
                                    double time,
                                    int idx,
                                    QTextStream& ts)
{
    
    QString token = isFeather ? "bezier.setFeatherPointAtIndex(" : "bezier.setPointAtIndex(";
    double x,y,lx,ly,rx,ry;
    point->getPositionAtTime(false ,time,/*view*/0, &x, &y);
    point->getLeftBezierPointAtTime(false ,time,/*view*/0, &lx, &ly);
    point->getRightBezierPointAtTime(false ,time,/*view*/0, &rx, &ry);
    
    WRITE_INDENT(indentLevel); WRITE_STATIC_LINE(token + NUM_INT(idx) + ", " +
                                       NUM_TIME(time) + ", " + NUM_VALUE(x) + ", " +
                                       NUM_VALUE(y) + ", " + NUM_VALUE(lx) + ", " +
                                       NUM_VALUE(ly) + ", " + NUM_VALUE(rx) + ", " +
                                       NUM_VALUE(ry) + ")");
   
}

static void exportRotoLayer(int indentLevel,
                            const std::list<boost::shared_ptr<RotoItem> >& items,
                            const boost::shared_ptr<RotoLayer>& layer,
                            QTextStream& ts)
{
    QString parentLayerName = QString(layer->getScriptName().c_str()) + "_layer";
    for (std::list<boost::shared_ptr<RotoItem> >::const_iterator it = items.begin(); it != items.end(); ++it) {
        
        boost::shared_ptr<RotoLayer> isLayer = boost::dynamic_pointer_cast<RotoLayer>(*it);
        boost::shared_ptr<Bezier> isBezier = boost::dynamic_pointer_cast<Bezier>(*it);
        
        if (isBezier) {
            double time;
            
            const std::list<boost::shared_ptr<BezierCP> >& cps = isBezier->getControlPoints();
            const std::list<boost::shared_ptr<BezierCP> >& fps = isBezier->getFeatherPoints();
            
            if (cps.empty()) {
                continue;
            }
            
            time = cps.front()->getKeyframeTime(false ,0);
            
            WRITE_INDENT(indentLevel); WRITE_STATIC_LINE("bezier = roto.createBezier(0, 0, " + NUM_TIME(time) + ")");
            WRITE_INDENT(indentLevel); WRITE_STATIC_LINE("bezier.setScriptName(" + ESC(isBezier->getScriptName()) + ")");
            WRITE_INDENT(indentLevel); WRITE_STATIC_LINE("bezier.setLabel(" + ESC(isBezier->getLabel()) + ")");
            QString lockedStr = isBezier->getLocked() ? "True" : "False";
            WRITE_INDENT(indentLevel); WRITE_STRING("bezier.setLocked(" + lockedStr + ")");
            QString visibleStr = isBezier->isGloballyActivated() ? "True" : "False";
            WRITE_INDENT(indentLevel); WRITE_STRING("bezier.setVisible(" + visibleStr + ")");
            
            boost::shared_ptr<KnobBool> activatedKnob = isBezier->getActivatedKnob();
            exportKnobValues(indentLevel,activatedKnob, "bezier.getActivatedParam()", true, ts);
            
            boost::shared_ptr<KnobDouble> featherDist = isBezier->getFeatherKnob();
            exportKnobValues(indentLevel,featherDist,"bezier.getFeatherDistanceParam()", true, ts);
            
            boost::shared_ptr<KnobDouble> opacityKnob = isBezier->getOpacityKnob();
            exportKnobValues(indentLevel,opacityKnob,"bezier.getOpacityParam()", true, ts);
            
            boost::shared_ptr<KnobDouble> fallOffKnob = isBezier->getFeatherFallOffKnob();
            exportKnobValues(indentLevel,fallOffKnob,"bezier.getFeatherFallOffParam()", true, ts);
            
            boost::shared_ptr<KnobColor> colorKnob = isBezier->getColorKnob();
            exportKnobValues(indentLevel,colorKnob, "bezier.getColorParam()", true, ts);
            
            boost::shared_ptr<KnobChoice> compositing = isBezier->getOperatorKnob();
            exportKnobValues(indentLevel,compositing, "bezier.getCompositingOperatorParam()", true, ts);

            
            WRITE_INDENT(indentLevel); WRITE_STRING(parentLayerName + ".addItem(bezier)");
            WRITE_INDENT(indentLevel); WRITE_STATIC_LINE("");
           
            assert(cps.size() == fps.size());
            
            std::set<double> kf;
            isBezier->getKeyframeTimes(&kf);
            
            //the last python call already registered the first control point
            int nbPts = cps.size() - 1;
            WRITE_INDENT(indentLevel); WRITE_STATIC_LINE("for i in range(0, " + NUM_INT(nbPts) +"):");
            WRITE_INDENT(2); WRITE_STATIC_LINE("bezier.addControlPoint(0,0)");
          
            ///Now that all points are created position them
            int idx = 0;
            std::list<boost::shared_ptr<BezierCP> >::const_iterator fpIt = fps.begin();
            for (std::list<boost::shared_ptr<BezierCP> >::const_iterator it2 = cps.begin(); it2 != cps.end(); ++it2, ++fpIt, ++idx) {
                for (std::set<double>::iterator it3 = kf.begin(); it3 != kf.end(); ++it3) {
                    exportBezierPointAtTime(indentLevel,*it2, false, *it3, idx, ts);
                    exportBezierPointAtTime(indentLevel,*fpIt, true, *it3, idx, ts);
                }
                if (kf.empty()) {
                    exportBezierPointAtTime(indentLevel,*it2, false, time, idx, ts);
                    exportBezierPointAtTime(indentLevel,*fpIt, true, time, idx, ts);
                }
                boost::shared_ptr<KnobDouble> track = (*it2)->isSlaved();
                if (track) {
                    EffectInstance* effect = dynamic_cast<EffectInstance*>(track->getHolder());
                    assert(effect);
                    if (!effect) {
                        throw std::logic_error("exportRotoLayer");
                    }
                    assert(effect->getNode() && effect->getNode()->isPointTrackerNode());
                    std::string trackerName = effect->getNode()->getScriptName_mt_safe();
                    int trackTime = (*it2)->getOffsetTime();
                    WRITE_INDENT(indentLevel); WRITE_STRING("tracker = group.getNode(\"" + QString(trackerName.c_str()) + "\")");
                    WRITE_INDENT(indentLevel); WRITE_STRING("center = tracker.getParam(\"" + QString(track->getName().c_str()) + "\")");
                    WRITE_INDENT(indentLevel); WRITE_STRING("bezier.slavePointToTrack(" + NUM_INT(idx) + ", " +
                                                  NUM_TIME(trackTime) + ",center)");
                    WRITE_INDENT(indentLevel); WRITE_STATIC_LINE("del center");
                    WRITE_INDENT(indentLevel); WRITE_STATIC_LINE("del tracker");
                }
                
            }
            if (isBezier->isCurveFinished()) {
                WRITE_INDENT(indentLevel); WRITE_STRING("bezier.setCurveFinished(True)");
            }
            
            WRITE_INDENT(indentLevel); WRITE_STATIC_LINE("del bezier");
            
            
        } else {
            
            QString name =  QString(isLayer->getScriptName().c_str());
            QString layerName = name + "_layer";
            WRITE_INDENT(indentLevel); WRITE_STATIC_LINE(name + " = roto.createLayer()");
            WRITE_INDENT(indentLevel); WRITE_STATIC_LINE(layerName +  ".setScriptName(" + ESC(name) + ")");
            WRITE_INDENT(indentLevel); WRITE_STATIC_LINE(layerName +  ".setLabel(" + ESC(isLayer->getLabel()) + ")");
            QString lockedStr = isLayer->getLocked() ? "True" : "False";
            WRITE_INDENT(indentLevel); WRITE_STRING(layerName + ".setLocked(" + lockedStr + ")");
            QString visibleStr = isLayer->isGloballyActivated() ? "True" : "False";
            WRITE_INDENT(indentLevel); WRITE_STRING(layerName + ".setVisible(" + visibleStr + ")");
            
            WRITE_INDENT(indentLevel); WRITE_STRING(parentLayerName + ".addItem(" + layerName);
            
            const std::list<boost::shared_ptr<RotoItem> >& items = isLayer->getItems();
            exportRotoLayer(indentLevel,items, isLayer, ts);
            WRITE_INDENT(indentLevel); WRITE_STRING("del " + layerName);
        }
        WRITE_STATIC_LINE("");
    }
}

static void exportAllNodeKnobs(int indentLevel,const NodePtr& node,QTextStream& ts)
{
    
    const KnobsVec& knobs = node->getKnobs();
    std::list<KnobPage*> userPages;
    for (KnobsVec::const_iterator it2 = knobs.begin(); it2 != knobs.end(); ++it2) {
        if ((*it2)->getIsPersistant() && !(*it2)->isUserKnob()) {
            QString getParamStr("lastNode.getParam(\"");
            const std::string& paramName =  (*it2)->getName();
            if (paramName.empty()) {
                continue;
            }
            getParamStr += paramName.c_str();
            getParamStr += "\")";
            if (exportKnobValues(indentLevel,*it2,getParamStr, true, ts)) {
                WRITE_STATIC_LINE("");
            }
            
        }
        
        if ((*it2)->isUserKnob()) {
            KnobPage* isPage = dynamic_cast<KnobPage*>(it2->get());
            if (isPage) {
                userPages.push_back(isPage);
            }
        }
    }// for (KnobsVec::const_iterator it2 = knobs.begin(); it2 != knobs.end(); ++it2)
    if (!userPages.empty()) {
        WRITE_STATIC_LINE("");
        WRITE_INDENT(indentLevel); WRITE_STATIC_LINE("# Create the user parameters");
    }
    for (std::list<KnobPage*>::iterator it2 = userPages.begin(); it2!= userPages.end(); ++it2) {
        WRITE_INDENT(indentLevel); WRITE_STRING("lastNode." + QString((*it2)->getName().c_str()) +
                                      " = lastNode.createPageParam(" + ESC((*it2)->getName()) + ", " +
                                      ESC((*it2)->getLabel()) + ")");
        KnobsVec children =  (*it2)->getChildren();
        for (KnobsVec::const_iterator it3 = children.begin(); it3 != children.end(); ++it3) {
            exportUserKnob(indentLevel,*it3, "lastNode", 0, *it2, ts);
        }
    }
    
    if (!userPages.empty()) {
        WRITE_INDENT(indentLevel); WRITE_STATIC_LINE("# Refresh the GUI with the newly created parameters");
        std::list<std::string> pagesOrdering = node->getPagesOrder();
        if (!pagesOrdering.empty()) {
            QString line("lastNode.setPagesOrder([");
            std::list<std::string>::iterator next = pagesOrdering.begin();
            ++next;
            for (std::list<std::string>::iterator it = pagesOrdering.begin(); it!=pagesOrdering.end(); ++it) {
                line += '\'';
                line += it->c_str();
                line += '\'';
                if (next != pagesOrdering.end()) {
                    line += ", ";
                    ++next;
                }
            }
            line += "])";
            WRITE_INDENT(indentLevel); WRITE_STRING(line);
        }
        WRITE_INDENT(indentLevel); WRITE_STATIC_LINE("lastNode.refreshUserParamsGUI()");
    }
    
    boost::shared_ptr<RotoContext> roto = node->getRotoContext();
    if (roto) {
        const std::list<boost::shared_ptr<RotoLayer> >& layers = roto->getLayers();
        
        if (!layers.empty()) {
            WRITE_INDENT(indentLevel); WRITE_STATIC_LINE("# For the roto node, create all layers and beziers");
            WRITE_INDENT(indentLevel); WRITE_STRING("roto = lastNode.getRotoContext()");
            boost::shared_ptr<RotoLayer> baseLayer = layers.front();
            QString baseLayerName = QString(baseLayer->getScriptName().c_str());
            QString baseLayerToken = baseLayerName +"_layer";
            WRITE_INDENT(indentLevel); WRITE_STATIC_LINE(baseLayerToken + " = roto.getBaseLayer()");
            
            WRITE_INDENT(indentLevel); WRITE_STRING(baseLayerToken + ".setScriptName(" + ESC(baseLayerName) + ")");
            WRITE_INDENT(indentLevel); WRITE_STRING(baseLayerToken + ".setLabel(" + ESC(baseLayer->getLabel()) + ")");
            QString lockedStr = baseLayer->getLocked() ? "True" : "False";
            WRITE_INDENT(indentLevel); WRITE_STRING(baseLayerToken + ".setLocked(" + lockedStr + ")");
            QString visibleStr = baseLayer->isGloballyActivated() ? "True" : "False";
            WRITE_INDENT(indentLevel); WRITE_STRING(baseLayerToken + ".setVisible(" + visibleStr + ")");
            exportRotoLayer(indentLevel,baseLayer->getItems(),baseLayer,  ts);
            WRITE_INDENT(indentLevel); WRITE_STRING("del " + baseLayerToken);
            WRITE_INDENT(indentLevel); WRITE_STRING("del roto");
        }
    }
}

static bool exportKnobLinks(int indentLevel,
                            const NodePtr& groupNode,
                            const NodePtr& node,
                            const QString& groupName,
                            const QString& nodeName,
                            QTextStream& ts)
{
    bool hasExportedLink = false;
    const KnobsVec& knobs = node->getKnobs();
    for (KnobsVec::const_iterator it2 = knobs.begin(); it2 != knobs.end(); ++it2) {
        QString paramName = nodeName + ".getParam(\"" + QString((*it2)->getName().c_str()) + "\")";
        bool hasDefined = false;
        
        //Check for alias link
        KnobPtr alias = (*it2)->getAliasMaster();
        if (alias) {
            if (!hasDefined) {
                WRITE_INDENT(indentLevel); WRITE_STRING("param = " + paramName);
                hasDefined = true;
            }
            hasExportedLink = true;
            
            EffectInstance* aliasHolder = dynamic_cast<EffectInstance*>(alias->getHolder());
            assert(aliasHolder);
            if (!aliasHolder) {
                throw std::logic_error("exportKnobLinks");
            }
            QString aliasName;
            if (aliasHolder == groupNode->getEffectInstance().get()) {
                aliasName = groupName;
            } else {
                aliasName = groupName + aliasHolder->getNode()->getScriptName_mt_safe().c_str();
            }
            aliasName += '.';
            aliasName += alias->getName().c_str();
            
            WRITE_INDENT(indentLevel); WRITE_STRING(aliasName + ".setAsAlias(" + paramName + ")");
        } else {
            
            for (int i = 0; i < (*it2)->getDimension(); ++i) {
                std::string expr = (*it2)->getExpression(i);
                QString hasRetVar = (*it2)->isExpressionUsingRetVariable(i) ? "True" : "False";
                if (!expr.empty()) {
                    if (!hasDefined) {
                        WRITE_INDENT(indentLevel); WRITE_STRING("param = " + paramName);
                        hasDefined = true;
                    }
                    hasExportedLink = true;
                    WRITE_INDENT(indentLevel); WRITE_STRING("param.setExpression(" + ESC(expr) + ", " +
                                                            hasRetVar + ", " + NUM_INT(i) + ")");
                }
                
                std::pair<int,KnobPtr > master = (*it2)->getMaster(i);
                if (master.second) {
                    if (!hasDefined) {
                        WRITE_INDENT(indentLevel); WRITE_STRING("param = " + paramName);
                        hasDefined = true;
                    }
                    hasExportedLink = true;
                    
                    EffectInstance* masterHolder = dynamic_cast<EffectInstance*>(master.second->getHolder());
                    assert(masterHolder);
                    if (!masterHolder) {
                        throw std::logic_error("exportKnobLinks");
                    }
                    QString masterName;
                    if (masterHolder == groupNode->getEffectInstance().get()) {
                        masterName = groupName;
                    } else {
                        masterName = groupName + masterHolder->getNode()->getScriptName_mt_safe().c_str();
                    }
                    masterName += '.';
                    masterName += master.second->getName().c_str();

                    
                    WRITE_INDENT(indentLevel); WRITE_STRING("param.slaveTo(" +  masterName + ", " +
                                                            NUM_INT(i) + ", " + NUM_INT(master.first) + ")");
                }
                
            }
        }
        if (hasDefined) {
            WRITE_INDENT(indentLevel); WRITE_STATIC_LINE("del param");
        }
    }
    return hasExportedLink;
}

static void exportGroupInternal(int indentLevel,const NodeCollection* collection,const QString& groupName, QTextStream& ts)
{
    WRITE_INDENT(indentLevel); WRITE_STATIC_LINE("# Create all nodes in the group");
    WRITE_STATIC_LINE("");

    NodesList nodes = collection->getNodes();
    NodesList exportedNodes;
    
    ///Re-order nodes so we're sure Roto nodes get exported in the end since they may depend on Trackers
    NodesList rotos;
    NodesList newNodes;
    for (NodesList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        if ((*it)->isRotoPaintingNode() || (*it)->isRotoNode()) {
            rotos.push_back(*it);
        } else {
            newNodes.push_back(*it);
        }
    }
    newNodes.insert(newNodes.end(), rotos.begin(),rotos.end());
    
    for (NodesList::iterator it = newNodes.begin(); it != newNodes.end(); ++it) {
        ///Don't create viewer while exporting
        ViewerInstance* isViewer = (*it)->isEffectViewer();
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
        
        WRITE_INDENT(indentLevel); WRITE_STRING("# Start of node " + ESC((*it)->getScriptName_mt_safe()));
        WRITE_INDENT(indentLevel); WRITE_STRING("lastNode = app.createNode(" + ESC(nodeName) + ", " +
                                      NUM_INT((*it)->getPlugin()->getMajorVersion()) + ", " + groupName +
                                      ")");
        WRITE_INDENT(indentLevel); WRITE_STRING("lastNode.setScriptName(" + ESC((*it)->getScriptName_mt_safe()) + ")");
        WRITE_INDENT(indentLevel); WRITE_STRING("lastNode.setLabel(" + ESC((*it)->getLabel_mt_safe()) + ")");
        double x,y;
        (*it)->getPosition(&x,&y);
        double w,h;
        (*it)->getSize(&w, &h);
        // a precision of 1 pixel is enough for the position on the nodegraph
        WRITE_INDENT(indentLevel); WRITE_STRING("lastNode.setPosition(" + NUM_PIXEL(x) + ", " + NUM_PIXEL(y) + ")");
        WRITE_INDENT(indentLevel); WRITE_STRING("lastNode.setSize(" + NUM_PIXEL(w) + ", " + NUM_PIXEL(h) + ")");
        
        double r,g,b;
        (*it)->getColor(&r,&g,&b);
        // a precision of 3 digits is enough for the node coloe
        WRITE_INDENT(indentLevel); WRITE_STRING("lastNode.setColor(" + NUM_COLOR(r) + ", " + NUM_COLOR(g) + ", " + NUM_COLOR(b) +  ")");
        
        std::list<ImageComponents> userComps;
        (*it)->getUserCreatedComponents(&userComps);
        for (std::list<ImageComponents>::iterator it2 = userComps.begin(); it2 != userComps.end(); ++it2) {
            
            const std::vector<std::string>& channels = it2->getComponentsNames();
            QString compStr("[");
            for (std::size_t i = 0; i < channels.size(); ++i) {
                compStr.append(ESC(channels[i]));
                if (i < (channels.size() - 1)) {
                    compStr.push_back(',');
                }
            }
            compStr.push_back(']');
            WRITE_INDENT(indentLevel); WRITE_STRING("lastNode.addUserPlane(" + ESC(it2->getLayerName()) + ", " + compStr +  ")");
        }
        
        QString nodeNameInScript = groupName + QString((*it)->getScriptName_mt_safe().c_str());
        WRITE_INDENT(indentLevel); WRITE_STRING(nodeNameInScript + " = lastNode");
        WRITE_STATIC_LINE("");
        exportAllNodeKnobs(indentLevel,*it,ts);
        WRITE_INDENT(indentLevel); WRITE_STRING("del lastNode");
        WRITE_INDENT(indentLevel); WRITE_STRING("# End of node " + ESC((*it)->getScriptName_mt_safe()));
        WRITE_STATIC_LINE("");

        std::list< NodePtr > children;
        (*it)->getChildrenMultiInstance(&children);
        if (!children.empty()) {
            WRITE_INDENT(indentLevel); WRITE_STATIC_LINE("# Create children if the node is a multi-instance such as a tracker");
            for (std::list< NodePtr > ::iterator it2 = children.begin(); it2 != children.end(); ++it2) {
                if ((*it2)->isActivated()) {
                    WRITE_INDENT(indentLevel); WRITE_STRING("lastNode = " + nodeNameInScript + ".createChild()");
                    WRITE_INDENT(indentLevel); WRITE_STRING("lastNode.setScriptName(\"" + QString((*it2)->getScriptName_mt_safe().c_str()) + "\")");
                    WRITE_INDENT(indentLevel); WRITE_STRING("lastNode.setLabel(\"" + QString((*it2)->getLabel_mt_safe().c_str()) + "\")");
                    exportAllNodeKnobs(indentLevel,*it2,ts);
                    WRITE_INDENT(indentLevel); WRITE_STRING(nodeNameInScript + "." + QString((*it2)->getScriptName_mt_safe().c_str()) + " = lastNode");
                    WRITE_INDENT(indentLevel); WRITE_STRING("del lastNode");
                }
            }
            WRITE_STATIC_LINE("");
        }

        NodeGroup* isGrp = (*it)->isEffectGroup();
        if (isGrp) {
            WRITE_INDENT(indentLevel); WRITE_STRING(groupName + "group = " + nodeNameInScript);
            exportGroupInternal(indentLevel, isGrp, groupName + "group", ts);
            WRITE_STATIC_LINE("");
        }
    }

    const NodeGroup* isGroup = dynamic_cast<const NodeGroup*>(collection);
    NodePtr groupNode;
    if (isGroup) {
        groupNode = isGroup->getNode();
    }
    if (isGroup) {
        WRITE_INDENT(indentLevel); WRITE_STATIC_LINE("# Create the parameters of the group node the same way we did for all internal nodes");
        WRITE_INDENT(indentLevel); WRITE_STRING("lastNode = " + groupName);
        exportAllNodeKnobs(indentLevel,isGroup->getNode(),ts);
        WRITE_INDENT(indentLevel); WRITE_STATIC_LINE("del lastNode");
        WRITE_STATIC_LINE("");
    }
    
    WRITE_INDENT(indentLevel); WRITE_STATIC_LINE("# Now that all nodes are created we can connect them together, restore expressions");
    bool hasConnected = false;
    for (NodesList::iterator it = exportedNodes.begin(); it != exportedNodes.end(); ++it) {
        
        QString nodeQualifiedName(groupName + (*it)->getScriptName_mt_safe().c_str());
        
        if (!(*it)->getParentMultiInstance()) {
            for (int i = 0; i < (*it)->getMaxInputCount(); ++i) {
                NodePtr inputNode = (*it)->getRealInput(i);
                if (inputNode) {
                    hasConnected = true;
                    QString inputQualifiedName(groupName  + inputNode->getScriptName_mt_safe().c_str());
                    WRITE_INDENT(indentLevel); WRITE_STRING(nodeQualifiedName + ".connectInput(" + NUM_INT(i) +
                                                  ", " + inputQualifiedName + ")");
                }
            }
        }
        
    }
    if (hasConnected) {
        WRITE_STATIC_LINE("");
    }
    
    bool hasExported = false;
    
    for (NodesList::iterator it = exportedNodes.begin(); it != exportedNodes.end(); ++it) {
        QString nodeQualifiedName(groupName + (*it)->getScriptName_mt_safe().c_str());
        if (exportKnobLinks(indentLevel,groupNode, *it, groupName, nodeQualifiedName, ts)) {
            hasExported = true;
        }
    }
    if (hasExported) {
        WRITE_STATIC_LINE("");
    }
    if (isGroup) {
        exportKnobLinks(indentLevel, groupNode, groupNode, groupName, groupName, ts);
    }
}

void
NodeCollection::exportGroupToPython(const QString& pluginID,
                                    const QString& pluginLabel,
                                    const QString& pluginDescription,
                                    const QString& pluginIconPath,
                                    const QString& pluginGrouping,
                                    QString& output)
{
    QString extModule(pluginLabel);
    extModule.append("Ext");
    
    QTextStream ts(&output);
    // coding must be set in first or second line, see https://www.python.org/dev/peps/pep-0263/
    WRITE_STATIC_LINE("# -*- coding: utf-8 -*-");
    WRITE_STATIC_LINE("# DO NOT EDIT THIS FILE");
    QString descline = QString("# This file was automatically generated by " NATRON_APPLICATION_NAME " PyPlug exporter version %1.").arg(NATRON_PYPLUG_EXPORTER_VERSION);
    WRITE_STRING(descline);
    WRITE_STATIC_LINE();
    QString handWrittenStr = QString("# Hand-written code should be added in a separate file named %1.py").arg(extModule);
    WRITE_STRING(handWrittenStr);
    WRITE_STATIC_LINE("# See http://natron.readthedocs.org/en/workshop/groups.html#adding-hand-written-code-callbacks-etc");
    WRITE_STATIC_LINE("# Note that Viewers are never exported");
    WRITE_STATIC_LINE();
    WRITE_STATIC_LINE("import " NATRON_ENGINE_PYTHON_MODULE_NAME);
    WRITE_STATIC_LINE("import sys");
    WRITE_STATIC_LINE("");
    WRITE_STATIC_LINE("# Try to import the extensions file where callbacks and hand-written code should be located.");
    WRITE_STATIC_LINE("try:");
    
    
    WRITE_INDENT(1);WRITE_STRING("from " + extModule + " import *");
    WRITE_STRING("except ImportError:");
    WRITE_INDENT(1);WRITE_STRING("pass");
    WRITE_STATIC_LINE("");
    
    WRITE_STATIC_LINE("def getPluginID():");
    WRITE_INDENT(1);WRITE_STRING("return \"" + pluginID + "\"");
    WRITE_STATIC_LINE("");
    
    WRITE_STATIC_LINE("def getLabel():");
    WRITE_INDENT(1);WRITE_STRING("return " + ESC(pluginLabel));
    WRITE_STATIC_LINE("");
    
    WRITE_STATIC_LINE("def getVersion():");
    WRITE_INDENT(1);WRITE_STRING("return 1");
    WRITE_STATIC_LINE("");
  
    if (!pluginIconPath.isEmpty()) {
        WRITE_STATIC_LINE("def getIconPath():");
        WRITE_INDENT(1);WRITE_STRING("return " + ESC(pluginIconPath));
        WRITE_STATIC_LINE("");
    }
    
    WRITE_STATIC_LINE("def getGrouping():");
    WRITE_INDENT(1);WRITE_STRING("return \"" + pluginGrouping + "\"");
    WRITE_STATIC_LINE("");
    
    if (!pluginDescription.isEmpty()) {
        WRITE_STATIC_LINE("def getPluginDescription():");
        WRITE_INDENT(1);WRITE_STRING("return " + ESC(pluginDescription));
        WRITE_STATIC_LINE("");
    }
    
    
    WRITE_STATIC_LINE("def createInstance(app,group):");
    exportGroupInternal(1, this, "group", ts);
    
    ///Import user hand-written code
    WRITE_INDENT(1);WRITE_STATIC_LINE("try:");
    WRITE_INDENT(2);WRITE_STRING("extModule = sys.modules[" + ESC(extModule) + "]");
    WRITE_INDENT(1);WRITE_STATIC_LINE("except KeyError:");
    WRITE_INDENT(2);WRITE_STATIC_LINE("extModule = None");
    
    QString testAttr("if extModule is not None and hasattr(extModule ,\"createInstanceExt\") and hasattr(extModule.createInstanceExt,\"__call__\"):");
    WRITE_INDENT(1);WRITE_STRING(testAttr);
    WRITE_INDENT(2);WRITE_STRING("extModule.createInstanceExt(app,group)");
}

NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_NodeGroup.cpp"
