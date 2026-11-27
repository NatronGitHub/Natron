/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2023 The Natron developers
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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "NodeGroup.h"

#include <iostream>
#include <set>
#include <locale>
#include <limits>
#include <algorithm> // min, max
#include <cassert>
#include <stdexcept>
#include <sstream> // stringstream
#include <limits>

#include <QCoreApplication>
#include <QTextStream>

#include <ofxImageEffect.h> // kOfxImageEffectFileParamName

#include "Engine/AppInstance.h"
#include "Engine/Bezier.h"
#include "Engine/CreateNodeArgs.h"
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
#include "Engine/NodeGuiSerialization.h"
#include "Engine/NodeSerialization.h"
#include "Engine/OutputSchedulerThread.h"
#include "Engine/Plugin.h"
#include "Engine/Project.h"
#include "Engine/PrecompNode.h"
#include "Engine/ReadNode.h"
#include "Engine/RotoContext.h"
#include "Engine/RotoLayer.h"
#include "Engine/RotoPoint.h"
#include "Engine/RotoStrokeItem.h"
#include "Engine/Settings.h"
#include "Engine/TimeLine.h"
#include "Engine/TrackMarker.h"
#include "Engine/TrackerContext.h"
#include "Engine/ViewIdx.h"
#include "Engine/ViewerInstance.h"
#include "Engine/WriteNode.h"

#define NATRON_PYPLUG_EXPORTER_VERSION 11
// The version of the Python project files written by Project::exportToPython()
#define NATRON_PROJECT_PYTHON_EXPORTER_VERSION 1

NATRON_NAMESPACE_ENTER

struct NodeCollectionPrivate
{
    AppInstanceWPtr app;
    NodeGraphI* graph;
    mutable QMutex nodesMutex;
    NodesList nodes;
    // see NodeCollection::addViewerWithoutGui(), protected by nodesMutex
    struct ViewerWithoutGui
    {
        NodeSerializationPtr serialization;
        // by input name: the input node, null if it wasn't found (its name is kept), set by connectViewersWithoutGui()
        std::map<std::string, NodeWPtr> inputs;
        bool connected = false;
    };
    std::list<ViewerWithoutGui> viewersWithoutGui;
    std::map<std::string, NodeGuiSerializationPtr> viewersWithoutGuiData; // by script name

    NodeCollectionPrivate(const AppInstancePtr& app)
        : app(app)
        , graph(0)
        , nodesMutex()
        , nodes()
    {
    }

    NodePtr findNodeInternal(const std::string& name, const std::string& recurseName) const;
};

NodeCollection::NodeCollection(const AppInstancePtr& app)
    : _imp( new NodeCollectionPrivate(app) )
{
}

NodeCollection::~NodeCollection()
{
}

AppInstancePtr
NodeCollection::getApplication() const
{
    return _imp->app.lock();
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
NodeCollection::getNodes_recursive(NodesList& nodes,
                                   bool onlyActive) const
{
    std::list<NodeGroup*> groupToRecurse;

    {
        QMutexLocker k(&_imp->nodesMutex);
        for (NodesList::const_iterator it = _imp->nodes.begin(); it != _imp->nodes.end(); ++it) {
            if ( onlyActive && !(*it)->isActivated() ) {
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
        (*it)->getNodes_recursive(nodes, onlyActive);
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
NodeCollection::removeNode(const Node* node)
{
    QMutexLocker k(&_imp->nodesMutex);
    for (NodesList::iterator it =_imp->nodes.begin(); it != _imp->nodes.end();++it) {
        if ( it->get() == node ) {
            _imp->nodes.erase(it);
            break;
        }
    }
}

NodePtr
NodeCollection::getLastNode(const std::string& pluginID) const
{
    QMutexLocker k(&_imp->nodesMutex);

    for (NodesList::reverse_iterator it = _imp->nodes.rbegin(); it != _imp->nodes.rend(); ++it) {
        if ( (*it)->getPluginID() == pluginID ) {
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
        if ( (*it)->isActivated() ) {
            nodes->push_back(*it);
        }
    }
}

void
NodeCollection::getActiveNodesExpandGroups(NodesList* nodes) const
{
    QMutexLocker k(&_imp->nodesMutex);

    for (NodesList::iterator it = _imp->nodes.begin(); it != _imp->nodes.end(); ++it) {
        if ( (*it)->isActivated() ) {
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
        if ( (*it)->getGroup() && (*it)->isActivated() && (*it)->getEffectInstance()->isWriter() && (*it)->isPartOfProject() ) {
            OutputEffectInstance* out = dynamic_cast<OutputEffectInstance*>( (*it)->getEffectInstance().get() );
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
NodeCollection::quitAnyProcessingInternal(bool blocking)
{
    NodesList nodes = getNodes();

    for (NodesList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        if (blocking) {
            (*it)->quitAnyProcessing_blocking(true);
        } else {
            (*it)->quitAnyProcessing_non_blocking();
        }
        NodeGroup* isGrp = (*it)->isEffectGroup();
        if (isGrp) {
            isGrp->quitAnyProcessingInternal(blocking);
        }
        PrecompNode* isPrecomp = dynamic_cast<PrecompNode*>( (*it)->getEffectInstance().get() );
        if (isPrecomp) {
            isPrecomp->getPrecompApp()->getProject()->quitAnyProcessingInternal(blocking);
        }
    }
}

void
NodeCollection::quitAnyProcessingForAllNodes_blocking()
{
    quitAnyProcessingInternal(true);
}

void
NodeCollection::quitAnyProcessingForAllNodes_non_blocking()
{
    quitAnyProcessingInternal(false);
}

bool
NodeCollection::isCacheIDAlreadyTaken(const std::string& name) const
{
    {
        QMutexLocker k(&_imp->nodesMutex);

        for (NodesList::iterator it = _imp->nodes.begin(); it != _imp->nodes.end(); ++it) {
            if ( (*it)->getCacheID() == name ) {
                return true;
            }
        }
    }

    // nodesMutex is unlocked: AppManager::isCacheIDRetired() locks its own mutex
    return appPTR->isCacheIDRetired(name);
}

bool
NodeCollection::hasNodeRendering() const
{
    QMutexLocker k(&_imp->nodesMutex);

    for (NodesList::iterator it = _imp->nodes.begin(); it != _imp->nodes.end(); ++it) {
        if ( (*it)->isOutputNode() ) {
            NodeGroup* isGrp = (*it)->isEffectGroup();
            PrecompNode* isPrecomp = dynamic_cast<PrecompNode*>( (*it)->getEffectInstance().get() );
            if (isGrp) {
                if ( isGrp->hasNodeRendering() ) {
                    return true;
                }
            } else if (isPrecomp) {
                if ( isPrecomp->getPrecompApp()->getProject()->hasNodeRendering() ) {
                    return true;
                }
            } else {
                OutputEffectInstance* effect = dynamic_cast<OutputEffectInstance*>( (*it)->getEffectInstance().get() );
                if ( effect && effect->getRenderEngine()->hasThreadsWorking() ) {
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
    assert( QThread::currentThread() == qApp->thread() );

    AppInstancePtr appInst = getApplication();
    if (!appInst) {
        return;
    }
    if ( !appInst->isBackground() ) {
        NodesList nodes = getNodes();
        for (NodesList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
            assert(*it);
            (*it)->refreshPreviewsAfterProjectLoad();
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
    AppInstancePtr appInst = getApplication();
    if (!appInst) {
        return;
    }
    if ( appInst->isBackground() ) {
        return;
    }
    double time = appInst->getTimeLine()->currentFrame();
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
    AppInstancePtr appInst = getApplication();
    if (!appInst) {
        return;
    }
    if ( appInst->isBackground() ) {
        return;
    }
    double time = appInst->getTimeLine()->currentFrame();
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
NodeCollection::clearNodesInternal(bool blocking)
{
    NodesList nodesToDelete;
    {
        QMutexLocker l(&_imp->nodesMutex);
        nodesToDelete = _imp->nodes;
    }

    ///Clear recursively containers inside this group
    for (NodesList::iterator it = nodesToDelete.begin(); it != nodesToDelete.end(); ++it) {
        // You should have called quitAnyProcessing before!
        assert( !(*it)->isNodeRendering() );

        NodeGroup* isGrp = (*it)->isEffectGroup();
        if (isGrp) {
            isGrp->clearNodesInternal(blocking);
        }
        PrecompNode* isPrecomp = dynamic_cast<PrecompNode*>( (*it)->getEffectInstance().get() );
        if (isPrecomp) {
            isPrecomp->getPrecompApp()->getProject()->clearNodesInternal(blocking);
        }
    }

    ///Kill effects

    for (NodesList::iterator it = nodesToDelete.begin(); it != nodesToDelete.end(); ++it) {
        (*it)->destroyNode(blocking, false);
    }


    if (_imp->graph) {
        _imp->graph->onNodesCleared();
    }


    {
        QMutexLocker l(&_imp->nodesMutex);
        _imp->nodes.clear();
        _imp->viewersWithoutGui.clear();
        _imp->viewersWithoutGuiData.clear();
    }

    nodesToDelete.clear();
}

void
NodeCollection::addViewerWithoutGui(const NodeSerializationPtr& viewer)
{
    NodeCollectionPrivate::ViewerWithoutGui v;

    v.serialization = viewer;
    QMutexLocker l(&_imp->nodesMutex);
    _imp->viewersWithoutGui.push_back(v);
}

void
NodeCollection::connectViewersWithoutGui()
{
    {
        // with a GUI, there are none
        QMutexLocker l(&_imp->nodesMutex);
        if ( _imp->viewersWithoutGui.empty() ) {
            return;
        }
    }
    // the names of the nodes are read without the nodesMutex (Node::setScriptName() reads the nodes of the group)
    const NodesList nodes = getNodes();
    std::map<std::string, NodePtr> nodesByName;

    for (NodesList::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        nodesByName[(*it)->getScriptName_mt_safe()] = *it;
    }
    QMutexLocker l(&_imp->nodesMutex);
    for (std::list<NodeCollectionPrivate::ViewerWithoutGui>::iterator it = _imp->viewersWithoutGui.begin(); it != _imp->viewersWithoutGui.end(); ++it) {
        if (it->connected) {
            continue;
        }
        const std::map<std::string, std::string>& inputs = it->serialization->getInputs();
        for (std::map<std::string, std::string>::const_iterator input = inputs.begin(); input != inputs.end(); ++input) {
            std::map<std::string, NodePtr>::const_iterator found = nodesByName.find(input->second);
            if ( found != nodesByName.end() ) {
                it->inputs[input->first] = found->second;
            }
        }
        it->connected = true;
    }
}

std::list<NodeSerializationPtr>
NodeCollection::getViewersWithoutGui() const
{
    std::list<NodeCollectionPrivate::ViewerWithoutGui> viewers;
    {
        QMutexLocker l(&_imp->nodesMutex);
        viewers = _imp->viewersWithoutGui;
    }
    std::list<NodeSerializationPtr> ret;
    for (std::list<NodeCollectionPrivate::ViewerWithoutGui>::const_iterator it = viewers.begin(); it != viewers.end(); ++it) {
        std::map<std::string, std::string> inputs;
        const std::map<std::string, std::string>& savedInputs = it->serialization->getInputs();
        for (std::map<std::string, std::string>::const_iterator input = savedInputs.begin(); input != savedInputs.end(); ++input) {
            std::map<std::string, NodeWPtr>::const_iterator found = it->inputs.find(input->first);
            if ( found == it->inputs.end() ) {
                // not found when the project was loaded: as in the file
                inputs.insert(*input);
            } else if ( NodePtr node = found->second.lock() ) {
                // renamed, or deleted
                if ( node->isActivated() ) {
                    inputs[input->first] = node->getScriptName_mt_safe();
                }
            }
        }
        NodeSerializationPtr copy = std::make_shared<NodeSerialization>(*it->serialization);
        copy->setInputs(inputs);
        ret.push_back(copy);
    }

    return ret;
}

bool
NodeCollection::setViewerWithoutGuiData(const std::string& scriptName,
                                        const NodeGuiSerialization& data)
{
    QMutexLocker l(&_imp->nodesMutex);

    for (std::list<NodeCollectionPrivate::ViewerWithoutGui>::const_iterator it = _imp->viewersWithoutGui.begin(); it != _imp->viewersWithoutGui.end(); ++it) {
        if ( it->serialization->getNodeScriptName() == scriptName ) {
            _imp->viewersWithoutGuiData[scriptName] = std::make_shared<NodeGuiSerialization>(data);

            return true;
        }
    }

    return false;
}

std::list<NodeGuiSerializationPtr>
NodeCollection::getViewersWithoutGuiData() const
{
    // the prefix of the fully qualified names of the nodes of this collection
    std::string prefix;
    if ( const NodeGroup* isGroup = dynamic_cast<const NodeGroup*>(this) ) {
        prefix = isGroup->getNode()->getFullyQualifiedName() + '.';
    }
    std::list<NodeGuiSerializationPtr> ret;
    QMutexLocker l(&_imp->nodesMutex);
    for (std::list<NodeCollectionPrivate::ViewerWithoutGui>::const_iterator it = _imp->viewersWithoutGui.begin(); it != _imp->viewersWithoutGui.end(); ++it) {
        const std::string& scriptName = it->serialization->getNodeScriptName();
        std::map<std::string, NodeGuiSerializationPtr>::const_iterator found = _imp->viewersWithoutGuiData.find(scriptName);
        if ( found != _imp->viewersWithoutGuiData.end() ) {
            NodeGuiSerializationPtr data = std::make_shared<NodeGuiSerialization>(*found->second);
            data->setFullySpecifiedName(prefix + scriptName);
            ret.push_back(data);
        }
    }

    return ret;
}

void
NodeCollection::clearNodesBlocking()
{
    quitAnyProcessingForAllNodes_blocking();
    clearNodesInternal(true);
}

void
NodeCollection::clearNodesNonBlocking()
{
    clearNodesInternal(false);
}

void
NodeCollection::checkNodeName(const Node* node,
                              const std::string& baseName,
                              bool appendDigit,
                              bool errorIfExists,
                              std::string* nodeName)
{
    if ( baseName.empty() ) {
        throw std::runtime_error( tr("Invalid script-name.").toStdString() );

        return;
    }
    ///Remove any non alpha-numeric characters from the baseName
    std::string cpy = NATRON_PYTHON_NAMESPACE::makeNameScriptFriendly(baseName);
    if ( cpy.empty() ) {
        throw std::runtime_error( tr("Invalid script-name.").toStdString() );

        return;
    }

    ///If this is a group and one of its parameter has the same script-name as the script-name of one of the node inside
    ///the python attribute will be overwritten. Try to prevent this situation.
    NodeGroup* isGroup = dynamic_cast<NodeGroup*>(this);
    if (isGroup) {
        const KnobsVec&  knobs = isGroup->getKnobs();
        for (KnobsVec::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
            if ( (*it)->getName() == cpy ) {
                throw std::runtime_error( tr("A node within a group cannot have the same script-name (%1) as a parameter on the group for scripting purposes.").arg( QString::fromUtf8( cpy.c_str() ) ).toStdString() );

                return;
            }
        }
    }
    int no = 1;
    {
        std::stringstream ss;
        ss << cpy;
        if (appendDigit) {
            ss << no;
        }
        *nodeName = ss.str();
    }
    if (errorIfExists || !appendDigit) {
        // only this name is checked
        QMutexLocker l(&_imp->nodesMutex);
        for (NodesList::iterator it = _imp->nodes.begin(); it != _imp->nodes.end(); ++it) {
            if ( (it->get() != node) && (*it)->isActivated() && ( (*it)->getScriptName_mt_safe() == *nodeName ) ) {
                throw std::runtime_error( tr("A node with the script-name %1 already exists.").arg( QString::fromUtf8( nodeName->c_str() ) ).toStdString() );
            }
        }

        return;
    }

    // The script-names of the other active nodes, read once: looking each candidate name up in the list of
    // nodes made the default naming quadratic per node, cubic for a script creating thousands of nodes (#1565)
    std::set<std::string> usedNames;
    {
        QMutexLocker l(&_imp->nodesMutex);
        for (NodesList::iterator it = _imp->nodes.begin(); it != _imp->nodes.end(); ++it) {
            if ( (it->get() != node) && (*it)->isActivated() ) {
                usedNames.insert( (*it)->getScriptName_mt_safe() );
            }
        }
    }
    while ( usedNames.count(*nodeName) ) {
        ++no;
        std::stringstream ss;
        ss << cpy << no;
        *nodeName = ss.str();
    }
} // NodeCollection::checkNodeName

void
NodeCollection::initNodeName(const std::string& pluginLabel,
                             std::string* nodeName)
{
    std::string baseName(pluginLabel);

    if ( (baseName.size() > 3) &&
         ( baseName[baseName.size() - 1] == 'X') &&
         ( baseName[baseName.size() - 2] == 'F') &&
         ( baseName[baseName.size() - 3] == 'O') ) {
        baseName = baseName.substr(0, baseName.size() - 3);
    }

    checkNodeName(0, baseName, true, false, nodeName);
}

bool
NodeCollection::connectNodes(int inputNumber,
                             const NodePtr& input,
                             const NodePtr& output,
                             bool force)
{
    ////Only called by the main-thread
    assert( QThread::currentThread() == qApp->thread() );

    NodePtr existingInput = output->getRealInput(inputNumber);
    if (force && existingInput) {
        bool ok = disconnectNodes(existingInput, output);
        if (!ok) {
            return false;
        }
        if ( input && (input->getNInputs() > 0) ) {
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
                        ret == Node::eCanConnectInput_differentPars ||
                        ret == Node::eCanConnectInput_multiResNotSupported;

    if (ret == Node::eCanConnectInput_multiResNotSupported) {
        LogEntry::LogEntryColor c;
        if (output->getColor(&c.r, &c.g, &c.b)) {
            c.colorSet = true;
        }

        QString err = tr("Warning: %1 does not support inputs of different sizes but its inputs produce different output size. Please check this.").arg( QString::fromUtf8( output->getScriptName().c_str() ) );
        appPTR->writeToErrorLog_mt_safe(QString::fromUtf8( output->getScriptName().c_str() ) , QDateTime::currentDateTime(), err, false, c);

    }

    if ( !connectionOk || !output->connectInput(input, inputNumber) ) {
        return false;
    }

    return true;
}

bool
NodeCollection::connectNodes(int inputNumber,
                             const std::string & inputName,
                             const NodePtr& output)
{
    NodesList nodes = getNodes();

    for (NodesList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        assert(*it);
        if ( (*it)->getScriptName() == inputName ) {
            return connectNodes(inputNumber, *it, output);
        }
    }

    return false;
}

bool
NodeCollection::disconnectNodes(const NodePtr& input,
                                const NodePtr& output,
                                bool autoReconnect)
{
    NodePtr inputToReconnectTo;
    int indexOfInput = output->inputIndex( input );

    if (indexOfInput == -1) {
        return false;
    }

    int inputsCount = input->getNInputs();
    if (inputsCount == 1) {
        inputToReconnectTo = input->getInput(0);
    }


    if (output->disconnectInput( input.get() ) < 0) {
        return false;
    }

    if (autoReconnect && inputToReconnectTo) {
        connectNodes(indexOfInput, inputToReconnectTo, output);
    }

    return true;
}

bool
NodeCollection::autoConnectNodes(const NodePtr& selected,
                                 const NodePtr& created)
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
    if ( (selected->getNInputs() == 0) && (created->getNInputs() == 0) ) {
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
        else if (created->getNInputs() == 0) {
            assert(selected->getNInputs() != 0);
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
            bool ok = connectNodes(selectedInput, created, selected, true);
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
            std::list<std::pair<NodePtr, int> > outputsConnectedToSelectedNode;
            selected->getOutputsConnectedToThisNode(&outputsConnectedToSelectedNode);
            for (std::list<std::pair<NodePtr, int> >::iterator it = outputsConnectedToSelectedNode.begin();
                 it != outputsConnectedToSelectedNode.end(); ++it) {
                if ( it->first->getParentMultiInstanceName().empty() ) {
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
NodeCollectionPrivate::findNodeInternal(const std::string& name,
                                        const std::string& recurseName) const
{
    QMutexLocker k(&nodesMutex);

    for (NodesList::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        if ( (*it)->isActivated() && (*it)->getScriptName_mt_safe() == name ) {
            if ( !recurseName.empty() ) {
                NodeGroup* isGrp = (*it)->isEffectGroup();
                if (isGrp) {
                    return isGrp->getNodeByFullySpecifiedName(recurseName);
                } else {
                    NodesList children;
                    (*it)->getChildrenMultiInstance(&children);
                    for (NodesList::iterator it2 = children.begin(); it2 != children.end(); ++it2) {
                        if ( (*it2)->isActivated() && (*it2)->getScriptName_mt_safe() == recurseName ) {
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
    return _imp->findNodeInternal( name, std::string() );
}

void
NodeCollection::getNodeNameAndRemainder_LeftToRight(const std::string& fullySpecifiedName,
                                                    std::string& name,
                                                    std::string& remainder)
{
    std::size_t foundDot = fullySpecifiedName.find_first_of('.');

    if (foundDot != std::string::npos) {
        name = fullySpecifiedName.substr(0, foundDot);
        if ( foundDot + 1 < fullySpecifiedName.size() ) {
            remainder = fullySpecifiedName.substr(foundDot + 1, std::string::npos);
        }
    } else {
        name = fullySpecifiedName;
    }
}

void
NodeCollection::getNodeNameAndRemainder_RightToLeft(const std::string& fullySpecifiedName,
                                                    std::string& name,
                                                    std::string& remainder)
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

    return _imp->findNodeInternal(toFind, recurseName);
}

void
NodeCollection::fixRelativeFilePaths(const std::string& projectPathName,
                                     const std::string& newProjectPath,
                                     bool blockEval)
{
    NodesList nodes = getNodes();
    AppInstancePtr appInst = getApplication();
    if (!appInst) {
        return;
    }
    ProjectPtr project = appInst->getProject();

    for (NodesList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        if ( (*it)->isActivated() ) {
            (*it)->getEffectInstance()->beginChanges();

            const KnobsVec& knobs = (*it)->getKnobs();
            for (U32 j = 0; j < knobs.size(); ++j) {
                KnobStringBase* isString = dynamic_cast< KnobStringBase* >( knobs[j].get() );
                KnobString* isStringKnob = dynamic_cast<KnobString*>(isString);
                // (a KnobBytes holds binary data, not a file path)
                if ( !isString || isStringKnob || dynamic_cast<KnobBytes*>(isString) || ( knobs[j] == project->getEnvVarKnob() ) ) {
                    continue;
                }

                std::string filepath = isString->getValue();

                if ( !filepath.empty() ) {
                    if ( project->fixFilePath(projectPathName, newProjectPath, filepath) ) {
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
NodeCollection::fixPathName(const std::string& oldName,
                            const std::string& newName)
{
    NodesList nodes = getNodes();
    AppInstancePtr appInst = getApplication();
    if (!appInst) {
        return;
    }
    ProjectPtr project = appInst->getProject();

    for (NodesList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        if ( (*it)->isActivated() ) {
            const KnobsVec& knobs = (*it)->getKnobs();
            for (U32 j = 0; j < knobs.size(); ++j) {
                KnobStringBase* isString = dynamic_cast< KnobStringBase* >( knobs[j].get() );
                KnobString* isStringKnob = dynamic_cast<KnobString*>(isString);
                // (a KnobBytes holds binary data, not a file path)
                if ( !isString || isStringKnob || dynamic_cast<KnobBytes*>(isString) || ( knobs[j] == project->getEnvVarKnob() ) ) {
                    continue;
                }

                std::string filepath = isString->getValue();

                if ( ( filepath.size() >= (oldName.size() + 2) ) &&
                     ( filepath[0] == '[') &&
                     ( filepath[oldName.size() + 1] == ']') &&
                     ( filepath.substr( 1, oldName.size() ) == oldName) ) {
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
NodeCollection::checkIfNodeLabelExists(const std::string & n,
                                       const Node* caller) const
{
    QMutexLocker k(&_imp->nodesMutex);

    for (NodesList::const_iterator it = _imp->nodes.begin(); it != _imp->nodes.end(); ++it) {
        if ( (it->get() != caller) && (*it)->isActivated() && ( (*it)->getLabel_mt_safe() == n ) ) {
            return true;
        }
    }

    return false;
}

bool
NodeCollection::checkIfNodeNameExists(const std::string & n,
                                      const Node* caller) const
{
    QMutexLocker k(&_imp->nodesMutex);

    for (NodesList::const_iterator it = _imp->nodes.begin(); it != _imp->nodes.end(); ++it) {
        if ( (it->get() != caller) && (*it)->isActivated() && ( (*it)->getScriptName_mt_safe() == n ) ) {
            return true;
        }
    }

    return false;
}

static void
recomputeFrameRangeForAllReadersInternal(NodeCollection* group,
                                         int* firstFrame,
                                         int* lastFrame,
                                         bool setFrameRange)
{
    NodesList nodes = group->getNodes();

    for (NodesList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        if ( (*it)->isActivated() ) {
            if ( (*it)->getEffectInstance()->isReader() ) {
                double thisFirst, thislast;
                (*it)->getEffectInstance()->getFrameRange_public( (*it)->getHashValue(), &thisFirst, &thislast );
                if (thisFirst != std::numeric_limits<int>::min()) {
                    *firstFrame = setFrameRange ? thisFirst : std::min(*firstFrame, (int)thisFirst);
                }
                if (thislast != std::numeric_limits<int>::max()) {
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
NodeCollection::recomputeFrameRangeForAllReaders(int* firstFrame,
                                                 int* lastFrame)
{
    recomputeFrameRangeForAllReadersInternal(this, firstFrame, lastFrame, true);
}

void
NodeCollection::forceComputeInputDependentDataOnAllTrees()
{
    NodesList nodes;

    getNodes_recursive(nodes, true);
    std::list<Project::NodesTree> trees;
    Project::extractTreesFromNodes(nodes, trees);

    for (NodesList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        (*it)->markAllInputRelatedDataDirty();
    }

    std::list<Node*> markedNodes;
    for (std::list<Project::NodesTree>::iterator it = trees.begin(); it != trees.end(); ++it) {
        it->output.node->forceRefreshAllInputRelatedData();
    }
}

struct NodeGroupPrivate
{
    mutable QRecursiveMutex nodesLock;

    std::vector<NodeWPtr> inputs, guiInputs;
    NodesWList outputs, guiOutputs;
    bool isDeactivatingGroup;
    bool isActivatingGroup;
    bool isEditable;
    KnobButtonPtr exportAsTemplate, convertToGroup;

    NodeGroupPrivate()
    : nodesLock()
    , inputs()
    , guiInputs()
    , outputs()
    , guiOutputs()
    , isDeactivatingGroup(false)
    , isActivatingGroup(false)
    , isEditable(true)
    , exportAsTemplate()
    , convertToGroup()
    {
    }
};

NodeGroup::NodeGroup(const NodePtr &node)
    : OutputEffectInstance(node)
    , NodeCollection( node ? node->getApp() : AppInstancePtr() )
    , _imp( new NodeGroupPrivate() )
{
    setSupportsRenderScaleMaybe(EffectInstance::eSupportsYes);
}

bool
NodeGroup::getIsDeactivatingGroup() const
{
    assert( QThread::currentThread() == qApp->thread() );

    return _imp->isDeactivatingGroup;
}

void
NodeGroup::setIsDeactivatingGroup(bool b)
{
    assert( QThread::currentThread() == qApp->thread() );
    _imp->isDeactivatingGroup = b;
}

bool
NodeGroup::getIsActivatingGroup() const
{
    assert( QThread::currentThread() == qApp->thread() );

    return _imp->isActivatingGroup;
}

void
NodeGroup::setIsActivatingGroup(bool b)
{
    assert( QThread::currentThread() == qApp->thread() );
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
                                 std::list<ImagePlaneDesc>* comps)
{
    comps->push_back( ImagePlaneDesc::getRGBAComponents() );
    comps->push_back( ImagePlaneDesc::getRGBComponents() );
    comps->push_back( ImagePlaneDesc::getAlphaComponents() );
}

void
NodeGroup::addSupportedBitDepth(std::list<ImageBitDepthEnum>* depths) const
{
    depths->push_back(eImageBitDepthByte);
    depths->push_back(eImageBitDepthShort);
    depths->push_back(eImageBitDepthFloat);
}

int
NodeGroup::getNInputs() const
{
    return (int)_imp->inputs.size();
}

std::string
NodeGroup::getInputLabel(int inputNb) const
{
    NodePtr input;
    {
        QMutexLocker k(&_imp->nodesLock);
        if ( ( inputNb >= (int)_imp->inputs.size() ) || (inputNb < 0) ) {
            return std::string();
        }

        ///If the input name starts with "input" remove it, otherwise keep the full name

        input = _imp->inputs[inputNb].lock();
        if (!input || !input->isActivated()) {
            return std::string();
        }
    }
    QString inputName = QString::fromUtf8( input->getLabel_mt_safe().c_str() );

    if ( inputName.startsWith(QString::fromUtf8("input"), Qt::CaseInsensitive) ) {
        inputName.remove(0, 5);
    }

    return inputName.toStdString();
}

KnobButtonPtr
NodeGroup::getExportAsPyPlugButton() const
{
    return _imp->exportAsTemplate;
}

KnobButtonPtr
NodeGroup::getConvertToGroupButton() const
{
    return _imp->convertToGroup;
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

ViewIdx
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

        if ( ( inputNb >= (int)_imp->inputs.size() ) || (inputNb < 0) ) {
            return false;
        }


        n = _imp->inputs[inputNb].lock();
        if (!n) {
            return false;
        }
    }
    EffectInstancePtr effect = n->getEffectInstance();
    if (!effect) {
        return false;
    }
    GroupInput* input = dynamic_cast<GroupInput*>( effect.get() );

    assert(input);
    if (!input) {
        return false;
    }
    KnobIPtr knob = input->getKnobByName(kNatronGroupInputIsOptionalParamName);
    assert(knob);
    if (!knob) {
        return false;
    }
    KnobBool* isBool = dynamic_cast<KnobBool*>( knob.get() );
    assert(isBool);

    return isBool ? isBool->getValue() : false;
}

bool
NodeGroup::isHostChannelSelectorSupported(bool* /*defaultR*/,
                                          bool* /*defaultG*/,
                                          bool* /*defaultB*/,
                                          bool* /*defaultA*/) const
{
    return false;
}

bool
NodeGroup::isInputMask(int inputNb) const
{
    NodePtr n;

    {
        QMutexLocker k(&_imp->nodesLock);

        if ( ( inputNb >= (int)_imp->inputs.size() ) || (inputNb < 0) ) {
            return false;
        }


        n = _imp->inputs[inputNb].lock();
        if (!n) {
            return false;
        }
    }
    GroupInput* input = dynamic_cast<GroupInput*>( n->getEffectInstance().get() );

    if (!input) {
        return false;
    }
    KnobIPtr knob = input->getKnobByName(kNatronGroupInputIsMaskParamName);
    assert(knob);
    if (!knob) {
        return false;
    }
    KnobBool* isBool = dynamic_cast<KnobBool*>( knob.get() );
    assert(isBool);

    return isBool ? isBool->getValue() : false;
}

void
NodeGroup::initializeKnobs()
{
    KnobIPtr nodePage = getKnobByName(NATRON_PARAMETER_PAGE_NAME_EXTRA);

    assert(nodePage);
    KnobPage* isPage = dynamic_cast<KnobPage*>( nodePage.get() );
    assert(isPage);
    _imp->exportAsTemplate = AppManager::createKnob<KnobButton>( this, tr("Export as PyPlug") );
    _imp->exportAsTemplate->setName("exportAsPyPlug");
    _imp->exportAsTemplate->setHintToolTip( tr("Export this group as a Python group script (PyPlug) that can be shared and/or later "
                                               "on re-used as a plug-in.") );
    if (isPage) {
        isPage->addKnob(_imp->exportAsTemplate);
    }

    _imp->convertToGroup = AppManager::createKnob<KnobButton>( this, tr("Convert to Group") );
    _imp->convertToGroup->setName("convertToGroup");
    _imp->convertToGroup->setHintToolTip( tr("Converts this node to a Group: the internal node-graph and the user parameters will become editable") );
    if (isPage) {
        isPage->addKnob(_imp->convertToGroup);
    }
    _imp->convertToGroup->setSecret(true);

}

// Call onInputChanged() on every input of every output node that is connected to groupNode.
// An output node appears once per link in getOutputs(), and may be connected to groupNode
// through several of its inputs.
static void
notifyGroupOutputsInputChanged(const NodePtr& groupNode)
{
    std::set<NodePtr> outputs;
    {
        const NodesWList& outputsList = groupNode->getOutputs();
        for (NodesWList::const_iterator it = outputsList.begin(); it != outputsList.end(); ++it) {
            NodePtr output = it->lock();
            if (output) {
                outputs.insert(output);
            }
        }
    }
    for (std::set<NodePtr>::const_iterator it = outputs.begin(); it != outputs.end(); ++it) {
        int nInputs = (*it)->getNInputs();
        for (int idx = 0; idx < nInputs; ++idx) {
            if ( (*it)->getRealInput(idx) == groupNode ) {
                (*it)->onInputChanged(idx);
            }
        }
    }
}

void
NodeGroup::notifyNodeDeactivated(const NodePtr& node)
{
    if ( getIsDeactivatingGroup() ) {
        return;
    }
    NodePtr thisNode = getNode();

    GroupInput* isInput = dynamic_cast<GroupInput*>( node->getEffectInstance().get() );
    if (isInput) {
        // Disconnect the real input and re-initialize the inputs of the group without holding nodesLock:
        // they take the inputsMutex of the group node and of the nodes inside the group, and render threads
        // take nodesLock while holding these (see #814). _imp->inputs is only modified by the main thread.
        int inputNb = -1;
        {
            QMutexLocker k(&_imp->nodesLock);
            for (U32 i = 0; i < _imp->inputs.size(); ++i) {
                if (_imp->inputs[i].lock() == node) {
                    inputNb = (int)i;
                    break;
                }
            }
        }
        ///The input must have been tracked before
        assert(inputNb != -1);
        if (inputNb != -1) {
            ///Also disconnect the real input
            thisNode->disconnectInput(inputNb);

            // disconnectInput() runs the Python inputChanged callback and emits signals: look for the
            // GroupInput again instead of reusing inputNb.
            {
                QMutexLocker k(&_imp->nodesLock);
                for (std::vector<NodeWPtr>::iterator it = _imp->inputs.begin(); it != _imp->inputs.end(); ++it) {
                    if (it->lock() == node) {
                        _imp->inputs.erase(it);
                        break;
                    }
                }
                ///Sync gui inputs
                _imp->guiInputs = _imp->inputs;
            }
            thisNode->initializeInputs();
        }
        // Fall through: sync the gui inputs/outputs and notify the outputs of the group.
    }

    {
        QMutexLocker k(&_imp->nodesLock);
        GroupOutput* isOutput = dynamic_cast<GroupOutput*>( node->getEffectInstance().get() );
        if (isOutput) {
            for (NodesWList::iterator it = _imp->outputs.begin(); it != _imp->outputs.end(); ++it) {
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
    notifyGroupOutputsInputChanged(thisNode);
} // NodeGroup::notifyNodeDeactivated

void
NodeGroup::notifyNodeActivated(const NodePtr& node)
{
    if ( getIsActivatingGroup() ) {
        return;
    }

    NodePtr thisNode = getNode();

    GroupInput* isInput = dynamic_cast<GroupInput*>( node->getEffectInstance().get() );
    {
        QMutexLocker k(&_imp->nodesLock);
        if (isInput) {
            _imp->inputs.push_back(node);
            _imp->guiInputs.push_back(node);
        }
        GroupOutput* isOutput = dynamic_cast<GroupOutput*>( node->getEffectInstance().get() );
        if (isOutput) {
            _imp->outputs.push_back(node);
            _imp->guiOutputs.push_back(node);
        }
    }
    if (isInput) {
        // Without holding nodesLock (see notifyNodeDeactivated() and #814)
        thisNode->initializeInputs();
    }
    ///Notify outputs of the group nodes that their inputs may have changed
    notifyGroupOutputsInputChanged(thisNode);
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
    GroupInput* isInput = dynamic_cast<GroupInput*>( node->getEffectInstance().get() );

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
    if ( ( !useGuiConnexions && _imp->outputs.empty() ) || ( useGuiConnexions && _imp->guiOutputs.empty() ) ) {
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
NodeGroup::getRealInputForInput(bool useGuiConnexions,
                                const NodePtr& input) const
{
    int inputNb = -1;
    {
        QMutexLocker k(&_imp->nodesLock);
        const std::vector<NodeWPtr>& inputs = useGuiConnexions ? _imp->guiInputs : _imp->inputs;
        for (U32 i = 0; i < inputs.size(); ++i) {
            if (inputs[i].lock() == input) {
                inputNb = (int)i;
                break;
            }
        }
    }
    if (inputNb == -1) {
        return NodePtr();
    }

    // Call getInput() without holding nodesLock: it takes the group node's inputsMutex, and the main thread
    // takes nodesLock while holding it (see #814).
    return useGuiConnexions ? getNode()->getGuiInput(inputNb) : getNode()->getInput(inputNb);
}

void
NodeGroup::getInputsOutputs(NodesList* nodes,
                            bool useGuiConnexions) const
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
            for (NodesWList::iterator it = outputs.begin(); it != outputs.end(); ++it) {
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
            for (NodesWList::iterator it = outputs.begin(); it != outputs.end(); ++it) {
                NodePtr node = it->lock();
                if (node) {
                    nodes->push_back(node);
                }
            }
        }
    }
}

void
NodeGroup::getInputs(std::vector<NodePtr>* inputs,
                     bool useGuiConnexions) const
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
NodeGroup::purgeCaches()
{
    NodesList nodes = getNodes();

    for (NodesList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        (*it)->getEffectInstance()->purgeCaches();
    }
}

bool
NodeGroup::knobChanged(KnobI* k,
                       ValueChangedReasonEnum /*reason*/,
                       ViewSpec /*view*/,
                       double /*time*/,
                       bool /*originatedFromMainThread*/)
{
    bool ret = true;

    if ( k == _imp->exportAsTemplate.get() ) {
        NodeGuiIPtr gui_i = getNode()->getNodeGui();
        if (gui_i) {
            gui_i->exportGroupAsPythonScript();
        }
    } else if (k == _imp->convertToGroup.get() ) {
        getNode()->setPyPlugEdited(true);
    } else {
        ret = false;
    }

    return ret;
}

void
NodeGroup::setSubGraphEditable(bool editable)
{
    assert( QThread::currentThread() == qApp->thread() );
    _imp->isEditable = editable;
    Q_EMIT graphEditableChanged(editable);
}

bool
NodeGroup::isSubGraphEditable() const
{
    assert( QThread::currentThread() == qApp->thread() );

    return _imp->isEditable;
}

static QString
escapeString(const QString& str)
{
    QString ret;

    for (int i = 0; i < str.size(); ++i) {
        //if ( (i == 0) || ( str[i - 1] != QLatin1Char('\\') ) ) { // does not work eg if a line ends with '\'
        if ( str[i] == QLatin1Char('\a') ) {
            ret.append( QLatin1Char('\\') );
            ret.append( QLatin1Char('a') );
        } else if ( str[i] == QLatin1Char('\b') ) {
            ret.append( QLatin1Char('\\') );
            ret.append( QLatin1Char('b') );
        } else if ( str[i] == QLatin1Char('\f') ) {
            ret.append( QLatin1Char('\\') );
            ret.append( QLatin1Char('f') );
        } else if ( str[i] == QLatin1Char('\n') ) {
            ret.append( QLatin1Char('\\') );
            ret.append( QLatin1Char('n') );
        } else if ( str[i] == QLatin1Char('\r') ) {
            ret.append( QLatin1Char('\\') );
            ret.append( QLatin1Char('r') );
        } else if ( str[i] == QLatin1Char('\t') ) {
            ret.append( QLatin1Char('\\') );
            ret.append( QLatin1Char('t') );
        } else if ( str[i] == QLatin1Char('\v') ) {
            ret.append( QLatin1Char('\\') );
            ret.append( QLatin1Char('v') );
        } else if ( str[i] == QLatin1Char('\\') ) {
            ret.append( QLatin1Char('\\') );
            ret.append( QLatin1Char('\\') );
        } else if ( str[i] == QLatin1Char('\'') ) {
            ret.append( QLatin1Char('\\') );
            ret.append( QLatin1Char('\'') );
        } else if ( str[i] == QLatin1Char('"') ) {
            ret.append( QLatin1Char('\\') );
            ret.append( QLatin1Char('\"') );
            //} else if ( str[i] == QLatin1Char('?') ) { // no trigraphs in Python
            //    ret.append( QLatin1Char('\\') );
            //    ret.append( QLatin1Char('\?') );
        } else if (str[i].isPrint()) {
            ret.append(str[i]);
        } else {
#if PY_MAJOR_VERSION >= 3
            // Python 3 strings are unicode
            // QChar::unicode() returns char16_t on Qt6, which arg() has no
            // overload for, and ushort on Qt5, where the cast does nothing.
            ret.append(QString::fromUtf8("\\u%1").arg(ushort(str[i].unicode()), 4, 16, QLatin1Char('0')));
#else
            // Python 2: convert to Utf8
            QByteArray utf8 = QString(str[i]).toUtf8();
            for (int j = 0; j < utf8.size(); ++i) {
                ret.append(QString::fromUtf8("\\x%1").arg(utf8[j], 2, 16, QLatin1Char('0')));
            }
#endif
        }
        //}
    }
    ret.prepend( QLatin1Char('"') );
    ret.append( QLatin1Char('"') );

    return ret;
}

static QString
escapeString(const std::string& str)
{
    QString s = QString::fromUtf8( str.c_str() );

    return escapeString(s);
}

#define ESC(s) escapeString(s)


/* *INDENT-OFF* */

#define WRITE_STATIC_LINE(line) ts << line "\n"
#define WRITE_INDENT(x)                                 \
    for (int _i = 0; _i < x; ++_i) { ts << "    "; }
#define WRITE_STRING(str) ts << str << "\n"
//#define NUM(n) QString::number(n)
#define NUM_INT(n) QString::number(n, 10)
#define NUM_COLOR(n) QString::number(n, 'g', 4)
#define NUM_PIXEL(n) QString::number(n, 'f', 0)
#define NUM_VALUE(n) doubleToPython(n)
#define NUM_TIME(n) doubleToPython(n)

/* *INDENT-ON* */

// The shortest of value with 15, 16 or 17 significant digits that Python reads as value: 16 digits, which the
// exporter wrote before, change some values in their last bit (e.g. 0.1 + 0.2), and 17 digits always give the
// same value (std::numeric_limits<double>::max_digits10), as the .ntp files have it.
static QString
doubleToPython(double value)
{
    for (int precision = std::numeric_limits<double>::digits10; precision < std::numeric_limits<double>::max_digits10; ++precision) {
        const QString text = QString::number(value, 'g', precision);
        if (text.toDouble() == value) {
            return text;
        }
    }

    return QString::number(value, 'g', std::numeric_limits<double>::max_digits10);
}

// The Python name of an interpolation, e.g. "NatronEngine.Natron.KeyframeTypeEnum.eKeyframeTypeSmooth"
static QString
keyframeTypeToPython(KeyframeTypeEnum type)
{
    const char* name = "eKeyframeTypeNone";
    switch (type) {
    case eKeyframeTypeNone:
        break;
    case eKeyframeTypeSmooth:
        name = "eKeyframeTypeSmooth";
        break;
    case eKeyframeTypeBroken:
        name = "eKeyframeTypeBroken";
        break;
    case eKeyframeTypeCatmullRom:
        name = "eKeyframeTypeCatmullRom";
        break;
    case eKeyframeTypeConstant:
        name = "eKeyframeTypeConstant";
        break;
    case eKeyframeTypeCubic:
        name = "eKeyframeTypeCubic";
        break;
    case eKeyframeTypeFree:
        name = "eKeyframeTypeFree";
        break;
    case eKeyframeTypeHorizontal:
        name = "eKeyframeTypeHorizontal";
        break;
    case eKeyframeTypeLinear:
        name = "eKeyframeTypeLinear";
        break;
    }

    return QString::fromUtf8(NATRON_ENGINE_PYTHON_MODULE_NAME ".Natron.KeyframeTypeEnum.") + QString::fromUtf8(name);
}

// A free or broken keyframe keeps its derivatives (Curve::evaluateCurveChanged() doesn't compute them)
static bool
keepsOwnDerivatives(KeyframeTypeEnum type)
{
    return (type == eKeyframeTypeFree) || (type == eKeyframeTypeBroken);
}

// The keyframes of curve with the derivatives that Natron computes for them (Curve::evaluateCurveChanged()),
// which a script that only sets the keyframes gets: a keyframe whose derivatives differ (free or broken, or computed
// by an older Natron) must get its own.
static KeyFrameSet
keyFramesWithComputedDerivatives(const Curve& curve)
{
    Curve computed(curve);
    const KeyFrameSet keys = curve.getKeyFrames_mt_safe();

    computed.clearKeyFrames();
    for (KeyFrameSet::const_iterator k = keys.begin(); k != keys.end(); ++k) {
        // its neighbours' derivatives depend on them
        const bool keepsDerivatives = keepsOwnDerivatives( k->getInterpolation() );
        computed.addKeyFrame( KeyFrame( k->getTime(), k->getValue(), keepsDerivatives ? k->getLeftDerivative() : 0.,
                                        keepsDerivatives ? k->getRightDerivative() : 0., k->getInterpolation() ) );
    }

    return computed.getKeyFrames_mt_safe();
}

static bool
hasOwnDerivatives(const KeyFrame& key,
                  const KeyFrame& computed)
{
    return keepsOwnDerivatives( key.getInterpolation() ) ||
           ( key.getLeftDerivative() != computed.getLeftDerivative() ) || ( key.getRightDerivative() != computed.getRightDerivative() );
}

// The bytes of the KnobBytes values written by the current exportGroupToPython() (main thread): their total must
// stay below KnobBytes::maxExportLength(), for the script to fit in a QString.
static std::size_t gExportedBytes = 0;

// Which values exportKnobValues() writes when a dimension is not animated
enum ExportedValuesEnum
{
    eExportedValuesModified, // only the ones that differ from the default value
    eExportedValuesAll, // also the others (the parameter of a PyPlug instance: its PyPlug may have another default)
};

static bool
exportKnobValues(int indentLevel,
                 const KnobIPtr knob,
                 const QString& paramFullName,
                 bool mustDefineParam,
                 QTextStream& ts,
                 ExportedValuesEnum exportedValues = eExportedValuesModified)
{
    bool hasExportedValue = false;

    KnobStringBase* isStr = dynamic_cast<KnobStringBase*>( knob.get() );
    AnimatingKnobStringHelper* isAnimatedStr = dynamic_cast<AnimatingKnobStringHelper*>( knob.get() );
    KnobDoubleBase* isDouble = dynamic_cast<KnobDoubleBase*>( knob.get() );
    KnobIntBase* isInt = dynamic_cast<KnobIntBase*>( knob.get() );
    KnobBoolBase* isBool = dynamic_cast<KnobBoolBase*>( knob.get() );
    KnobParametric* isParametric = dynamic_cast<KnobParametric*>( knob.get() );
    KnobChoice* isChoice = dynamic_cast<KnobChoice*>( knob.get() );
    KnobGroup* isGrp = dynamic_cast<KnobGroup*>( knob.get() );
    KnobString* isStringKnob = dynamic_cast<KnobString*>( knob.get() );

    ///Don't export this kind of parameter. Mainly this is the html label of the node which is 99% of times empty
    if ( isStringKnob &&
         isStringKnob->isMultiLine() &&
         isStringKnob->usesRichText() &&
         !isStringKnob->hasContentWithoutHtmlTags() &&
         !isStringKnob->isAnimationEnabled() &&
         isStringKnob->getExpression(0).empty() ) {
        return false;
    }

    EffectInstance* holderIsEffect = dynamic_cast<EffectInstance*>( knob->getHolder() );

    if (isChoice && holderIsEffect) {
        //Do not serialize mask channel selector if the mask is not enabled
        int maskInputNb = holderIsEffect->getNode()->isMaskChannelKnob(isChoice);
        if (maskInputNb != -1) {
            if ( !holderIsEffect->getNode()->isMaskEnabled(maskInputNb) ) {
                return false;
            }
        }
    }

    // No value to restore (Python's ButtonParam and SeparatorParam have no setValue()): only their visibility and enabled state
    const bool hasValue = !dynamic_cast<KnobButton*>( knob.get() ) && !dynamic_cast<KnobSeparator*>( knob.get() );
    // (all the other callers pass indentLevel 1)
    int innerIdent = mustDefineParam ? indentLevel + 1 : indentLevel;
    const KnobHelper* knobHelper = dynamic_cast<const KnobHelper*>( knob.get() );

    for (int i = 0; i < knob->getDimension(); ++i) {
        if (isParametric) {
            if (!hasExportedValue) {
                hasExportedValue = true;
                if (mustDefineParam) {
                    WRITE_INDENT(indentLevel); WRITE_STRING(QString::fromUtf8("param = ") + paramFullName);
                    WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("if param is not None:") );
                }
            }
            CurvePtr curve = isParametric->getParametricCurve(i);
            double r, g, b;
            isParametric->getCurveColor(i, &r, &g, &b);
            WRITE_INDENT(innerIdent); WRITE_STRING( QString::fromUtf8("param.setCurveColor(") + NUM_INT(i) + QString::fromUtf8(", ") +
                                                    NUM_COLOR(r) + QString::fromUtf8(", ") + NUM_COLOR(g) + QString::fromUtf8(", ") + NUM_COLOR(b) + QString::fromUtf8(")") );
            if (curve) {
                KeyFrameSet keys = curve->getKeyFrames_mt_safe();
                if ( !keys.empty() ) {
                    WRITE_INDENT(innerIdent); WRITE_STRING( QString::fromUtf8("param.deleteAllControlPoints(") + NUM_INT(i) + QString::fromUtf8(")") );
                }
                // a control point added with its derivatives gets the computed ones, unless it is free or broken
                const KeyFrameSet computedKeys = keyFramesWithComputedDerivatives(*curve);
                KeyFrameSet::const_iterator computedKey = computedKeys.begin();
                for (KeyFrameSet::iterator it3 = keys.begin(); it3 != keys.end(); ++it3, ++computedKey) {
                    const bool ownDerivatives = ( computedKey != computedKeys.end() ) && hasOwnDerivatives(*it3, *computedKey) &&
                                                (it3->getInterpolation() != eKeyframeTypeBroken);
                    const QString interpStr = keyframeTypeToPython(ownDerivatives ? eKeyframeTypeFree : it3->getInterpolation() );

                    WRITE_INDENT(innerIdent); WRITE_STRING( QString::fromUtf8("param.addControlPoint(") + NUM_INT(i) + QString::fromUtf8(", ") +
                                                            NUM_TIME( it3->getTime() ) + QString::fromUtf8(", ") +
                                                            NUM_VALUE( it3->getValue() ) + QString::fromUtf8(", ") + NUM_VALUE( it3->getLeftDerivative() )
                                                            + QString::fromUtf8(", ") + NUM_VALUE( it3->getRightDerivative() ) + QString::fromUtf8(", ") + interpStr + QString::fromUtf8(")") );
                }
            }
        } else { // !isParametric
            CurvePtr curve = knob->getCurve(ViewIdx(0), i, true);
            if (curve) {
                KeyFrameSet keys = curve->getKeyFrames_mt_safe();

                if ( !keys.empty() ) {
                    if (!hasExportedValue) {
                        hasExportedValue = true;
                        if (mustDefineParam) {
                            WRITE_INDENT(indentLevel); WRITE_STRING(QString::fromUtf8("param = ") + paramFullName);
                            WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("if param is not None:") );
                        }
                    }
                }

                // the keyframes whose derivatives differ from the computed ones get theirs explicitly
                KeyFrameSet computedKeys;
                if (isDouble) {
                    computedKeys = keyFramesWithComputedDerivatives(*curve);
                }
                KeyFrameSet::const_iterator computedKey = computedKeys.begin();
                std::list<KeyFrame> derivatives;

                for (KeyFrameSet::iterator it3 = keys.begin(); it3 != keys.end(); ++it3) {
                    if (isAnimatedStr) {
                        std::string value = isAnimatedStr->getValueAtTime(it3->getTime(), i, ViewIdx(0), true);
                        WRITE_INDENT(innerIdent); WRITE_STRING( QString::fromUtf8("param.setValueAtTime(") + ESC(value) + QString::fromUtf8(", ")
                                                                + NUM_TIME( it3->getTime() ) + QChar::fromLatin1(')') );
                    } else if (isBool) {
                        int v = std::min( 1., std::max( 0., std::floor(it3->getValue() + 0.5) ) );
                        QString vStr = v ? QString::fromUtf8("True") : QString::fromUtf8("False");
                        WRITE_INDENT(innerIdent); WRITE_STRING( QString::fromUtf8("param.setValueAtTime(") + vStr + QString::fromUtf8(", ")
                                                                + NUM_TIME( it3->getTime() )  + QLatin1Char(')') );
                    } else if (isChoice) {
                        WRITE_INDENT(innerIdent); WRITE_STRING( QString::fromUtf8("param.setValueAtTime(") + NUM_INT( (int)it3->getValue() ) + QString::fromUtf8(", ")
                                                                + NUM_TIME( it3->getTime() ) + QLatin1Char(')') );
                    } else {
                        WRITE_INDENT(innerIdent); WRITE_STRING( QString::fromUtf8("param.setValueAtTime(") + NUM_VALUE( it3->getValue() ) + QString::fromUtf8(", ")
                                                                + NUM_TIME( it3->getTime() ) + QString::fromUtf8(", ") + NUM_INT(i) + QLatin1Char(')') );
                        bool ownDerivatives = false;
                        if ( computedKey != computedKeys.end() ) {
                            ownDerivatives = hasOwnDerivatives(*it3, *computedKey);
                            ++computedKey;
                        }
                        if (ownDerivatives) {
                            // after all the keyframes are set, which change the derivatives of their neighbours
                            derivatives.push_back(*it3);
                        } else if ( isDouble && (it3->getInterpolation() != eKeyframeTypeSmooth) ) {
                            // a new keyframe is smooth (KeyFrame::KeyFrame())
                            WRITE_INDENT(innerIdent); WRITE_STRING( QString::fromUtf8("param.setInterpolationAtTime(") + NUM_TIME( it3->getTime() ) + QString::fromUtf8(", ") +
                                                                    keyframeTypeToPython( it3->getInterpolation() ) + QString::fromUtf8(", ") + NUM_INT(i) + QLatin1Char(')') );
                        }
                    }
                }
                // The keyframe becomes free: a smooth one whose derivatives an older Natron computed stays free. A
                // broken one is broken again, which keeps its derivatives.
                for (std::list<KeyFrame>::const_iterator k = derivatives.begin(); k != derivatives.end(); ++k) {
                    WRITE_INDENT(innerIdent); WRITE_STRING( QString::fromUtf8("param.setDerivativesAtTime(") + NUM_TIME( k->getTime() ) + QString::fromUtf8(", ") +
                                                            NUM_VALUE( k->getLeftDerivative() ) + QString::fromUtf8(", ") + NUM_VALUE( k->getRightDerivative() ) +
                                                            QString::fromUtf8(", ") + NUM_INT(i) + QLatin1Char(')') );
                    if (k->getInterpolation() == eKeyframeTypeBroken) {
                        WRITE_INDENT(innerIdent); WRITE_STRING( QString::fromUtf8("param.setInterpolationAtTime(") + NUM_TIME( k->getTime() ) + QString::fromUtf8(", ") +
                                                                keyframeTypeToPython(eKeyframeTypeBroken) + QString::fromUtf8(", ") + NUM_INT(i) + QLatin1Char(')') );
                    }
                }
            }

            // Also a value equal to a default that an .ntp file restored (KnobSerialization), which a new node
            // doesn't have (e.g. the center of a Roto node, set from the project format when it was created)
            if ( hasValue && ( !curve || (curve->getKeyFramesCount() == 0) ) && ( (exportedValues == eExportedValuesAll) || knob->hasModifications(i) || ( knobHelper && knobHelper->hasDefaultValueChanged(i) ) ) ) {
                if (!hasExportedValue) {
                    hasExportedValue = true;
                    if (mustDefineParam) {
                        WRITE_INDENT(indentLevel); WRITE_STRING(QString::fromUtf8("param = ") + paramFullName);
                        WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("if param is not None:") );
                    }
                }

                if (isGrp) {
                    int v = std::min( 1., std::max( 0., std::floor(isGrp->getValue(i, ViewIdx(0), true) + 0.5) ) );
                    QString vStr = v ? QString::fromUtf8("True") : QString::fromUtf8("False");
                    WRITE_INDENT(innerIdent); WRITE_STRING( QString::fromUtf8("param.setOpened(") + vStr + QString::fromUtf8(")") );
                } else if ( dynamic_cast<KnobBytes*>(isStr) ) {
                    // binary data (Python bytes), which a string literal can't hold: hexadecimal
                    std::string v = isStr->getValue(i, ViewIdx(0), true);
                    if ( v.size() > KnobBytes::maxExportLength() - std::min(gExportedBytes, KnobBytes::maxExportLength()) ) {
                        // the script is a QString: 2 characters per byte, 2 bytes per character
                        std::cerr << knob->getName() << ": Warning: the value (" << v.size() << " bytes) is too large for the Python script (all the Bytes values of a script: "
                                  << KnobBytes::maxExportLength() << " bytes at most), it is not exported" << std::endl;
                        WRITE_INDENT(innerIdent); WRITE_STRING( QString::fromUtf8("# the value of this Bytes parameter is too large to be exported (%1 bytes)").arg( (qulonglong)v.size() ) );
                    } else {
                        gExportedBytes += v.size();
                        const QByteArray hex = QByteArray::fromRawData( v.data(), (int)v.size() ).toHex();
                        WRITE_INDENT(innerIdent); WRITE_STRING( QString::fromUtf8("param.setValue(bytes.fromhex(\"") + QString::fromLatin1(hex) + QString::fromUtf8("\"))") );
                    }
                } else if (isStr) {
                    std::string v = isStr->getValue(i, ViewIdx(0), true);
                    WRITE_INDENT(innerIdent); WRITE_STRING( QString::fromUtf8("param.setValue(") + ESC(v)  + QString::fromUtf8(")") );
                } else if (isDouble) {
                    double v = isDouble->getValue(i, ViewIdx(0), true);
                    WRITE_INDENT(innerIdent); WRITE_STRING( QString::fromUtf8("param.setValue(") + NUM_VALUE(v) + QString::fromUtf8(", ") + NUM_INT(i) + QString::fromUtf8(")") );
                } else if (isChoice) {
                    WRITE_INDENT(innerIdent); WRITE_STRING( QString::fromUtf8("param.set(") + ESC( isChoice->getActiveEntry().id ) + QString::fromUtf8(")") );
                } else if (isInt) {
                    int v = isInt->getValue(i, ViewIdx(0), true);
                    WRITE_INDENT(innerIdent); WRITE_STRING( QString::fromUtf8("param.setValue(") + NUM_INT(v) + QString::fromUtf8(", ") + NUM_INT(i) + QString::fromUtf8(")") );
                } else if (isBool) {
                    int v = std::min( 1., std::max( 0., std::floor(isBool->getValue(i, ViewIdx(0), true) + 0.5) ) );
                    QString vStr = v ? QString::fromUtf8("True") : QString::fromUtf8("False");
                    WRITE_INDENT(innerIdent); WRITE_STRING( QString::fromUtf8("param.setValue(") + vStr + QString::fromUtf8(")") );
                }
            } // if ((!curve || curve->getKeyFramesCount() == 0) && knob->hasModifications(i)) {
        } // if (isParametric) {
    } // for (int i = 0; i < (*it2)->getDimension(); ++i)

    bool isSecretByDefault = knob->getDefaultIsSecret();
    if (knob->isUserKnob() && isSecretByDefault) {
        if (!hasExportedValue) {
            hasExportedValue = true;
            if (mustDefineParam) {
                WRITE_INDENT(indentLevel); WRITE_STRING(QString::fromUtf8("param = ") + paramFullName);
                WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("if param is not None:") );
            }
        }

        WRITE_INDENT(innerIdent); WRITE_STRING( QString::fromUtf8("param.setVisibleByDefault(False)") );
    }

    if ( knob->isUserKnob() ) {
        bool isSecret = knob->getIsSecret();
        if (isSecret != isSecretByDefault) {
            if (!hasExportedValue) {
                hasExportedValue = true;
                if (mustDefineParam) {
                    WRITE_INDENT(indentLevel); WRITE_STRING(QString::fromUtf8("param = ") + paramFullName);
                    WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("if param is not None:") );
                }
            }

            QString str = QString::fromUtf8("param.setVisible(");
            if (isSecret) {
                str += QString::fromUtf8("False");
            } else {
                str += QString::fromUtf8("True");
            }
            str += QString::fromUtf8(")");
            WRITE_INDENT(innerIdent); WRITE_STRING(str);
        }

        bool enabledByDefault = knob->isDefaultEnabled(0);
        if (!enabledByDefault) {
            if (!hasExportedValue) {
                hasExportedValue = true;
                if (mustDefineParam) {
                    WRITE_INDENT(indentLevel); WRITE_STRING(QString::fromUtf8("param = ") + paramFullName);
                    WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("if param is not None:") );
                }
            }

            WRITE_INDENT(innerIdent); WRITE_STRING( QString::fromUtf8("param.setEnabledByDefault(False)") );
        }

        for (int i = 0; i < knob->getDimension(); ++i) {
            bool isEnabled = knob->isEnabled(i);
            if (isEnabled != enabledByDefault) {
                if (!hasExportedValue) {
                    hasExportedValue = true;
                    if (mustDefineParam) {
                        WRITE_INDENT(indentLevel); WRITE_STRING(QString::fromUtf8("param = ") + paramFullName);
                        WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("if param is not None:") );
                    }
                }

                QString str = QString::fromUtf8("param.setEnabled(");
                if (isEnabled) {
                    str += QString::fromUtf8("True");
                } else {
                    str += QString::fromUtf8("False");
                }
                str += QString::fromUtf8(", ");
                str += NUM_INT(i);
                str += QLatin1Char(')');
                WRITE_INDENT(innerIdent); WRITE_STRING(str);
            }
        }
    } // isuserknob

    if (mustDefineParam && hasExportedValue) {
        WRITE_INDENT(innerIdent); WRITE_STRING("del param");
    }

    return hasExportedValue;
} // exportKnobValues

static void
exportUserKnob(int indentLevel,
               const KnobIPtr& knob,
               const QString& fullyQualifiedNodeName,
               KnobGroup* group,
               KnobPage* page,
               QTextStream& ts)
{
    KnobInt* isInt = dynamic_cast<KnobInt*>( knob.get() );
    KnobDouble* isDouble = dynamic_cast<KnobDouble*>( knob.get() );
    KnobBool* isBool = dynamic_cast<KnobBool*>( knob.get() );
    KnobChoice* isChoice = dynamic_cast<KnobChoice*>( knob.get() );
    KnobColor* isColor = dynamic_cast<KnobColor*>( knob.get() );
    KnobString* isStr = dynamic_cast<KnobString*>( knob.get() );
    KnobFile* isFile = dynamic_cast<KnobFile*>( knob.get() );
    KnobOutputFile* isOutFile = dynamic_cast<KnobOutputFile*>( knob.get() );
    KnobPath* isPath = dynamic_cast<KnobPath*>( knob.get() );
    KnobGroup* isGrp = dynamic_cast<KnobGroup*>( knob.get() );
    KnobButton* isButton = dynamic_cast<KnobButton*>( knob.get() );
    KnobSeparator* isSep = dynamic_cast<KnobSeparator*>( knob.get() );
    KnobParametric* isParametric = dynamic_cast<KnobParametric*>( knob.get() );
    KnobIPtr aliasedParam;
    {
        KnobI::ListenerDimsMap listeners;
        knob->getListeners(listeners);
        if ( !listeners.empty() ) {
            KnobIPtr listener = listeners.begin()->first.lock();
            if ( listener && (listener->getAliasMaster() == knob) ) {
                aliasedParam = listener;
            }
        }
    }

    if (isInt) {
        QString createToken;
        switch ( isInt->getDimension() ) {
        case 1:
            createToken = QString::fromUtf8(".createIntParam(");
            break;
        case 2:
            createToken = QString::fromUtf8(".createInt2DParam(");
            break;
        case 3:
            createToken = QString::fromUtf8(".createInt3DParam(");
            break;
        default:
            assert(false);
            createToken = QString::fromUtf8(".createIntParam(");
            break;
        }
        WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("param = ") + fullyQualifiedNodeName + createToken + ESC( isInt->getName() ) +
                                                 QString::fromUtf8(", ") + ESC( isInt->getLabel() ) + QString::fromUtf8(")") );


        std::vector<int> defaultValues = isInt->getDefaultValues_mt_safe();


        assert( (int)defaultValues.size() == isInt->getDimension() );
        for (int i = 0; i < isInt->getDimension(); ++i) {
            int min = isInt->getMinimum(i);
            int max = isInt->getMaximum(i);
            int dMin = isInt->getDisplayMinimum(i);
            int dMax = isInt->getDisplayMaximum(i);
            if (min != std::numeric_limits<int>::min()) {
                WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("param.setMinimum(") + NUM_INT(min) + QString::fromUtf8(", ") +
                                                         NUM_INT(i) + QString::fromUtf8(")") );
            }
            if (max != std::numeric_limits<int>::max()) {
                WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("param.setMaximum(") + NUM_INT(max) + QString::fromUtf8(", ") +
                                                         NUM_INT(i) + QString::fromUtf8(")") );
            }
            if (dMin != std::numeric_limits<int>::min()) {
                WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("param.setDisplayMinimum(") + NUM_INT(dMin) + QString::fromUtf8(", ") +
                                                         NUM_INT(i) + QString::fromUtf8(")") );
            }
            if (dMax != std::numeric_limits<int>::max()) {
                WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("param.setDisplayMaximum(") + NUM_INT(dMax) + QString::fromUtf8(", ") +
                                                         NUM_INT(i) + QString::fromUtf8(")") );
            }
            WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("param.setDefaultValue(") + NUM_INT(defaultValues[i]) + QString::fromUtf8(", ") + NUM_INT(i) + QString::fromUtf8(")") );
            WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("param.restoreDefaultValue(") + NUM_INT(i) + QString::fromUtf8(")") );
        }
    } else if (isDouble) {
        QString createToken;
        switch ( isDouble->getDimension() ) {
        case 1:
            createToken = QString::fromUtf8(".createDoubleParam(");
            break;
        case 2:
            createToken = QString::fromUtf8(".createDouble2DParam(");
            break;
        case 3:
            createToken = QString::fromUtf8(".createDouble3DParam(");
            break;
        default:
            assert(false);
            createToken = QString::fromUtf8(".createDoubleParam(");
            break;
        }
        WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("param = ") + fullyQualifiedNodeName + createToken + ESC( isDouble->getName() ) +
                                                 QString::fromUtf8(", ") + ESC( isDouble->getLabel() ) + QString::fromUtf8(")") );
        // "Use Point Interact" of a 2D parameter (#1526)
        if ( (isDouble->getDimension() == 2) && isDouble->getHasHostOverlayHandle() ) {
            WRITE_INDENT(indentLevel); WRITE_STATIC_LINE("param.setUsePointInteract(True)");
        }

        std::vector<double> defaultValues = isDouble->getDefaultValues_mt_safe();
        assert( (int)defaultValues.size() == isDouble->getDimension() );
        for (int i = 0; i < isDouble->getDimension(); ++i) {
            double min = isDouble->getMinimum(i);
            double max = isDouble->getMaximum(i);
            double dMin = isDouble->getDisplayMinimum(i);
            double dMax = isDouble->getDisplayMaximum(i);
            if (min != std::numeric_limits<double>::lowest()) {
                WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("param.setMinimum(") + NUM_VALUE(min) + QString::fromUtf8(", ") +
                                                         NUM_INT(i) + QString::fromUtf8(")") );
            }
            if (max != std::numeric_limits<double>::max()) {
                WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("param.setMaximum(") + NUM_VALUE(max) + QString::fromUtf8(", ") +
                                                         NUM_INT(i) + QString::fromUtf8(")") );
            }
            if (dMin != std::numeric_limits<double>::lowest()) {
                WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("param.setDisplayMinimum(") + NUM_VALUE(dMin) + QString::fromUtf8(", ") +
                                                         NUM_INT(i) + QString::fromUtf8(")") );
            }
            if (dMax != std::numeric_limits<double>::max()) {
                WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("param.setDisplayMaximum(") + NUM_VALUE(dMax) + QString::fromUtf8(", ") +
                                                         NUM_INT(i) + QString::fromUtf8(")") );
            }
            if (defaultValues[i] != 0.) {
                WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("param.setDefaultValue(") + NUM_VALUE(defaultValues[i]) + QString::fromUtf8(", ") + NUM_INT(i) + QString::fromUtf8(")") );
                WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("param.restoreDefaultValue(") + NUM_INT(i) + QString::fromUtf8(")") );
            }
        }
    } else if (isBool) {
        WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("param = ") + fullyQualifiedNodeName + QString::fromUtf8(".createBooleanParam(") + ESC( isBool->getName() ) +
                                                 QString::fromUtf8(", ") + ESC( isBool->getLabel() ) + QString::fromUtf8(")") );

        std::vector<bool> defaultValues = isBool->getDefaultValues_mt_safe();
        assert( (int)defaultValues.size() == isBool->getDimension() );

        if (defaultValues[0]) {
            WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("param.setDefaultValue(True)") );
            WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("param.restoreDefaultValue()") );
        }
    } else if (isChoice) {
        WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("param = ") + fullyQualifiedNodeName + QString::fromUtf8(".createChoiceParam(") +
                                                 ESC( isChoice->getName() ) +
                                                 QString::fromUtf8(", ") + ESC( isChoice->getLabel() ) + QString::fromUtf8(")") );

        KnobChoice* aliasedIsChoice = dynamic_cast<KnobChoice*>( aliasedParam.get() );

        if (!aliasedIsChoice) {
            std::vector<ChoiceOption> entries = isChoice->getEntries_mt_safe();

            if (entries.size() > 0) {

                WRITE_INDENT(indentLevel); ts << "entries = [ (" << ESC(entries[0].id) << ", " << ESC(entries[0].tooltip) << "),\n";
                for (U32 i = 1; i < entries.size(); ++i) {
                    QString endToken = (i == entries.size() - 1) ? QString::fromUtf8(")]") : QString::fromUtf8("),");
                    WRITE_INDENT(indentLevel); WRITE_STRING(QString::fromUtf8("(") + ESC(entries[i].id) + QString::fromUtf8(", ") + ESC(entries[i].tooltip) + endToken);
                }
                WRITE_INDENT(indentLevel); WRITE_STATIC_LINE("param.setOptions(entries)");
                WRITE_INDENT(indentLevel); WRITE_STATIC_LINE("del entries");
            }
            std::vector<int> defaultValues = isChoice->getDefaultValues_mt_safe();
            assert( (int)defaultValues.size() == isChoice->getDimension() );
            if (defaultValues[0] != 0) {
                std::string entryStr = isChoice->getEntry(defaultValues[0]).id;
                WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("param.setDefaultValue(") + ESC(entryStr) + QString::fromUtf8(")") );
                WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("param.restoreDefaultValue()") );
            }
        } else {
            std::vector<int> defaultValues = isChoice->getDefaultValues_mt_safe();
            assert( (int)defaultValues.size() == isChoice->getDimension() );
            if (defaultValues[0] != 0) {
                WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("param.setDefaultValue(") + NUM_INT(defaultValues[0]) + QString::fromUtf8(")") );
                WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("param.restoreDefaultValue()"));
            }
        }
    } else if (isColor) {
        QString hasAlphaStr = (isColor->getDimension() == 4) ? QString::fromUtf8("True") : QString::fromUtf8("False");
        WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("param = ") + fullyQualifiedNodeName + QString::fromUtf8(".createColorParam(") + ESC( isColor->getName() ) +
                                                 QString::fromUtf8(", ") + ESC( isColor->getLabel() ) + QString::fromUtf8(", ") + hasAlphaStr +  QString::fromUtf8(")") );


        std::vector<double> defaultValues = isColor->getDefaultValues_mt_safe();
        assert( (int)defaultValues.size() == isColor->getDimension() );

        for (int i = 0; i < isColor->getDimension(); ++i) {
            double min = isColor->getMinimum(i);
            double max = isColor->getMaximum(i);
            double dMin = isColor->getDisplayMinimum(i);
            double dMax = isColor->getDisplayMaximum(i);
            if (min != std::numeric_limits<double>::lowest()) {
                WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("param.setMinimum(") + NUM_VALUE(min) + QString::fromUtf8(", ") +
                                                         NUM_INT(i) + QString::fromUtf8(")") );
            }
            if (max != std::numeric_limits<double>::max()) {
                WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("param.setMaximum(") + NUM_VALUE(max) + QString::fromUtf8(", ") +
                                                         NUM_INT(i) + QString::fromUtf8(")") );
            }
            if (dMin != std::numeric_limits<double>::lowest()) {
                WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("param.setDisplayMinimum(") + NUM_VALUE(dMin) + QString::fromUtf8(", ") +
                                                         NUM_INT(i) + QString::fromUtf8(")") );
            }
            if (dMax != std::numeric_limits<double>::max()) {
                WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("param.setDisplayMaximum(") + NUM_VALUE(dMax) + QString::fromUtf8(", ") +
                                                         NUM_INT(i) + QString::fromUtf8(")") );
            }
            if (defaultValues[i] != 0.) {
                WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("param.setDefaultValue(") + NUM_VALUE(defaultValues[i]) + QString::fromUtf8(", ") + NUM_INT(i) + QString::fromUtf8(")") );
                WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("param.restoreDefaultValue(") + NUM_INT(i) + QString::fromUtf8(")") );
            }
        }
    } else if (isButton) {
        WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("param = ") + fullyQualifiedNodeName + QString::fromUtf8(".createButtonParam(") +
                                                 ESC( isButton->getName() ) +
                                                 QString::fromUtf8(", ") + ESC( isButton->getLabel() ) + QString::fromUtf8(")") );
    } else if (isSep) {
        WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("param = ") + fullyQualifiedNodeName + QString::fromUtf8(".createSeparatorParam(") +
                                                 ESC( isSep->getName() ) +
                                                 QString::fromUtf8(", ") + ESC( isSep->getLabel() ) + QString::fromUtf8(")") );
    } else if (isStr) {
        WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("param = ") + fullyQualifiedNodeName + QString::fromUtf8(".createStringParam(") +
                                                 ESC( isStr->getName() ) +
                                                 QString::fromUtf8(", ") + ESC( isStr->getLabel() ) + QString::fromUtf8(")") );
        QString typeStr;
        if ( isStr->isLabel() ) {
            typeStr = QString::fromUtf8("eStringTypeLabel");
        } else if ( isStr->isMultiLine() ) {
            if ( isStr->usesRichText() ) {
                typeStr = QString::fromUtf8("eStringTypeRichTextMultiLine");
            } else {
                typeStr = QString::fromUtf8("eStringTypeMultiLine");
            }
        } else if ( isStr->isCustomKnob() ) {
            typeStr = QString::fromUtf8("eStringTypeCustom");
        } else {
            typeStr = QString::fromUtf8("eStringTypeDefault");
        }
        WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("param.setType(NatronEngine.StringParam.TypeEnum.") + typeStr + QString::fromUtf8(")") );

        std::vector<std::string> defaultValues = isStr->getDefaultValues_mt_safe();
        assert( (int)defaultValues.size() == isStr->getDimension() );
        QString def = QString::fromUtf8( defaultValues[0].c_str() );
        if ( !def.isEmpty() ) {
            WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("param.setDefaultValue(") + ESC(def) + QString::fromUtf8(")") );
            WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("param.restoreDefaultValue()") );
        }
    } else if (isFile) {
        WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("param = ") + fullyQualifiedNodeName + QString::fromUtf8(".createFileParam(") + ESC( isFile->getName() ) +
                                                 QString::fromUtf8(", ") + ESC( isFile->getLabel() ) + QString::fromUtf8(")") );
        QString seqStr = isFile->isInputImageFile() ? QString::fromUtf8("True") : QString::fromUtf8("False");
        WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("param.setSequenceEnabled(") + seqStr + QString::fromUtf8(")") );

        std::vector<std::string> defaultValues = isFile->getDefaultValues_mt_safe();
        assert( (int)defaultValues.size() == isFile->getDimension() );
        QString def = QString::fromUtf8( defaultValues[0].c_str() );
        if ( !def.isEmpty() ) {
            WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("param.setDefaultValue()") + def + QString::fromUtf8(")") );
            WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("param.restoreDefaultValue()") );
        }
    } else if (isOutFile) {
        WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("param = ") + fullyQualifiedNodeName + QString::fromUtf8(".createOutputFileParam(") +
                                                 ESC( isOutFile->getName() ) +
                                                 QString::fromUtf8(", ") + ESC( isOutFile->getLabel() ) + QString::fromUtf8(")") );
        assert(isOutFile);
        QString seqStr = isOutFile->isOutputImageFile() ? QString::fromUtf8("True") : QString::fromUtf8("False");
        WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("param.setSequenceEnabled(") + seqStr + QString::fromUtf8(")") );

        std::vector<std::string> defaultValues = isOutFile->getDefaultValues_mt_safe();
        assert( (int)defaultValues.size() == isOutFile->getDimension() );
        QString def = QString::fromUtf8( defaultValues[0].c_str() );
        if ( !def.isEmpty() ) {
            WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("param.setDefaultValue(") + ESC(def) + QString::fromUtf8(")") );
            WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("param.restoreDefaultValue()") );
        }
    } else if (isPath) {
        WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("param = ") + fullyQualifiedNodeName + QString::fromUtf8(".createPathParam(") +
                                                 ESC( isPath->getName() ) +
                                                 QString::fromUtf8(", ") + ESC( isPath->getLabel() ) + QString::fromUtf8(")") );
        if ( isPath->isMultiPath() ) {
            WRITE_INDENT(indentLevel); WRITE_STRING("param.setAsMultiPathTable()");
        }

        std::vector<std::string> defaultValues = isPath->getDefaultValues_mt_safe();
        assert( (int)defaultValues.size() == isPath->getDimension() );
        QString def = QString::fromUtf8( defaultValues[0].c_str() );
        if ( !def.isEmpty() ) {
            WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("param.setDefaultValue(") + ESC(def) + QString::fromUtf8(")") );
            WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("param.restoreDefaultValue()") );
        }
    } else if (isGrp) {
        WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("param = ") + fullyQualifiedNodeName + QString::fromUtf8(".createGroupParam(") +
                                                 ESC( isGrp->getName() ) +
                                                 QString::fromUtf8(", ") + ESC( isGrp->getLabel() ) + QString::fromUtf8(")") );
        if ( isGrp->isTab() ) {
            WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("param.setAsTab()") );
        }
    } else if (isParametric) {
        WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("param = ") + fullyQualifiedNodeName + QString::fromUtf8(".createParametricParam(") +
                                                 ESC( isParametric->getName() ) +
                                                 QString::fromUtf8(", ") + ESC( isParametric->getLabel() ) +  QString::fromUtf8(", ") +
                                                 NUM_INT( isParametric->getDimension() ) + QString::fromUtf8(")") );
    }

    WRITE_STATIC_LINE("");

    if (group) {
        QString grpFullName = fullyQualifiedNodeName + QString::fromUtf8(".") + QString::fromUtf8( group->getName().c_str() );
        WRITE_INDENT(indentLevel); WRITE_STATIC_LINE("# Add the param to the group, no need to add it to the page");
        WRITE_INDENT(indentLevel); WRITE_STRING( grpFullName + QString::fromUtf8(".addParam(param)") );
    } else {
        assert(page);
        QString pageFullName = fullyQualifiedNodeName + QString::fromUtf8(".") + QString::fromUtf8( page->getName().c_str() );
        WRITE_INDENT(indentLevel); WRITE_STATIC_LINE("# Add the param to the page");
        WRITE_INDENT(indentLevel); WRITE_STRING( pageFullName + QString::fromUtf8(".addParam(param)") );
    }

    WRITE_STATIC_LINE("");
    WRITE_INDENT(indentLevel); WRITE_STATIC_LINE("# Set param properties");

    QString help = QString::fromUtf8( knob->getHintToolTip().c_str() );
    if ( !aliasedParam || ( aliasedParam->getHintToolTip() != knob->getHintToolTip() ) ) {
        WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("param.setHelp(") + ESC(help) + QString::fromUtf8(")") );
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

    if ( !knob->getIsPersistent() ) {
        WRITE_INDENT(indentLevel); WRITE_STRING("param.setPersistent(False)");
    }

    if ( !knob->getEvaluateOnChange() ) {
        WRITE_INDENT(indentLevel); WRITE_STRING("param.setEvaluateOnChange(False)");
    }

    if ( knob->canAnimate() ) {
        QString animStr = knob->isAnimationEnabled() ? QString::fromUtf8("True") : QString::fromUtf8("False");
        WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("param.setAnimationEnabled(") + animStr + QString::fromUtf8(")") );
    }

    exportKnobValues(indentLevel, knob, QString(), false, ts);
    WRITE_INDENT(indentLevel); WRITE_STRING( fullyQualifiedNodeName + QString::fromUtf8(".") + QString::fromUtf8( knob->getName().c_str() ) + QString::fromUtf8(" = param") );
    WRITE_INDENT(indentLevel); WRITE_STATIC_LINE("del param");

    WRITE_STATIC_LINE("");

    if (isGrp) {
        KnobsVec children =  isGrp->getChildren();
        for (KnobsVec::const_iterator it3 = children.begin(); it3 != children.end(); ++it3) {
            exportUserKnob(indentLevel, *it3, fullyQualifiedNodeName, isGrp, page, ts);
        }
    }
} // exportUserKnob

static void
exportBezierPointAtTime(int indentLevel,
                        const BezierCPPtr& point,
                        bool isFeather,
                        double time,
                        int idx,
                        QTextStream& ts)
{
    QString token = isFeather ? QString::fromUtf8("bezier.setFeatherPointAtIndex(") : QString::fromUtf8("bezier.setPointAtIndex(");
    double x, y, lx, ly, rx, ry;

    point->getPositionAtTime(false, time, ViewIdx(0), &x, &y);
    point->getLeftBezierPointAtTime(false, time, ViewIdx(0), &lx, &ly);
    point->getRightBezierPointAtTime(false, time, ViewIdx(0), &rx, &ry);

    WRITE_INDENT(indentLevel); WRITE_STRING( token + NUM_INT(idx) + QString::fromUtf8(", ") +
                                             NUM_TIME(time) + QString::fromUtf8(", ") + NUM_VALUE(x) + QString::fromUtf8(", ") +
                                             NUM_VALUE(y) + QString::fromUtf8(", ") + NUM_VALUE(lx) + QString::fromUtf8(", ") +
                                             NUM_VALUE(ly) + QString::fromUtf8(", ") + NUM_VALUE(rx) + QString::fromUtf8(", ") +
                                             NUM_VALUE(ry) + QString::fromUtf8(")") );
}

// All the parameters of a shape or a stroke (also its lifetime, its transform, its motion blur...), after its points.
// Finishing it (setCurveFinished(), StrokeItem.addStroke()) set its center to its center: always write the center
// that it has.
static void
exportDrawableItemKnobs(int indentLevel,
                        const RotoDrawableItemPtr& item,
                        const QString& itemName,
                        QTextStream& ts)
{
    const std::list<KnobIPtr>& knobs = item->getKnobs();
    const KnobDoublePtr center = item->getCenterKnob();
    const QString getParam = itemName + QString::fromUtf8(".getParam(");

    for (std::list<KnobIPtr>::const_iterator k = knobs.begin(); k != knobs.end(); ++k) {
        if ( !(*k)->getIsPersistent() ) {
            continue;
        }
        if ( (*k == center) && !center->isAnimated(0) && !center->isAnimated(1) ) {
            for (int i = 0; i < 2; ++i) {
                WRITE_INDENT(indentLevel); WRITE_STRING( getParam + ESC( center->getName() ) + QString::fromUtf8(").setValue(") + NUM_VALUE( center->getValue(i) ) +
                                                         QString::fromUtf8(", ") + NUM_INT(i) + QString::fromUtf8(")") );
            }
            continue;
        }
        exportKnobValues(indentLevel, *k, getParam + ESC( (*k)->getName() ) + QString::fromUtf8(")"), true, ts);
    }
}

// The Python name of a brush type, e.g. "NatronEngine.Natron.RotoStrokeType.eRotoStrokeTypeSolid"
static QString
strokeTypeToPython(RotoStrokeType type)
{
    const char* name = "eRotoStrokeTypeSolid";
    switch (type) {
    case eRotoStrokeTypeSolid:
        break;
    case eRotoStrokeTypeEraser:
        name = "eRotoStrokeTypeEraser";
        break;
    case eRotoStrokeTypeClone:
        name = "eRotoStrokeTypeClone";
        break;
    case eRotoStrokeTypeReveal:
        name = "eRotoStrokeTypeReveal";
        break;
    case eRotoStrokeTypeBlur:
        name = "eRotoStrokeTypeBlur";
        break;
    case eRotoStrokeTypeSharpen:
        name = "eRotoStrokeTypeSharpen";
        break;
    case eRotoStrokeTypeSmear:
        name = "eRotoStrokeTypeSmear";
        break;
    case eRotoStrokeTypeDodge:
        name = "eRotoStrokeTypeDodge";
        break;
    case eRotoStrokeTypeBurn:
        name = "eRotoStrokeTypeBurn";
        break;
    }

    return QString::fromUtf8(NATRON_ENGINE_PYTHON_MODULE_NAME ".Natron.RotoStrokeType.") + QString::fromUtf8(name);
}

// A paint stroke: its lines (the points of each), then its parameters
static void
exportStroke(int indentLevel,
             const RotoStrokeItemPtr& stroke,
             const QString& parentLayerName,
             QTextStream& ts)
{
    WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("stroke = roto.createStroke(") + strokeTypeToPython( stroke->getBrushType() ) + QString::fromUtf8(")") );
    WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("stroke.setScriptName(") + ESC( stroke->getScriptName() ) + QString::fromUtf8(")") );
    WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("stroke.setLabel(") + ESC( stroke->getLabel() ) + QString::fromUtf8(")") );
    QString visibleStr = stroke->isGloballyActivated() ? QString::fromUtf8("True") : QString::fromUtf8("False");
    WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("stroke.setVisible(") + visibleStr + QString::fromUtf8(")") );
    WRITE_INDENT(indentLevel); WRITE_STRING( parentLayerName + QString::fromUtf8(".addItem(stroke)") );

    // the points (x, y, pressure, timestamp) of each line
    const std::vector<std::vector<RotoPoint> > lines = stroke->getStrokesPoints();
    for (std::vector<std::vector<RotoPoint> >::const_iterator line = lines.begin(); line != lines.end(); ++line) {
        WRITE_INDENT(indentLevel); WRITE_STATIC_LINE("stroke.addStroke([");
        for (std::vector<RotoPoint>::const_iterator p = line->begin(); p != line->end(); ++p) {
            WRITE_INDENT(indentLevel + 1); WRITE_STRING( QString::fromUtf8("(") + NUM_VALUE( p->pos().x ) + QString::fromUtf8(", ") + NUM_VALUE( p->pos().y ) +
                                                         QString::fromUtf8(", ") + NUM_VALUE( p->pressure() ) + QString::fromUtf8(", ") +
                                                         NUM_TIME( p->timestamp() ) + QString::fromUtf8("),") );
        }
        WRITE_INDENT(indentLevel); WRITE_STATIC_LINE("])");
    }
    exportDrawableItemKnobs(indentLevel, stroke, QString::fromUtf8("stroke"), ts);
    // Last, as for the shapes (see exportRotoLayer())
    if ( stroke->getLocked() ) {
        WRITE_INDENT(indentLevel); WRITE_STRING("stroke.setLocked(True)");
    }
    WRITE_INDENT(indentLevel); WRITE_STATIC_LINE("del stroke");
}

static void
exportRotoLayer(int indentLevel,
                const std::list<RotoItemPtr>& items,
                const RotoLayerPtr& layer,
                QTextStream& ts)
{
    QString parentLayerName = QString::fromUtf8( layer->getScriptName().c_str() ) + QString::fromUtf8("_layer");

    for (std::list<RotoItemPtr>::const_iterator it = items.begin(); it != items.end(); ++it) {
        RotoLayerPtr isLayer = std::dynamic_pointer_cast<RotoLayer>(*it);
        BezierPtr isBezier = std::dynamic_pointer_cast<Bezier>(*it);
        RotoStrokeItemPtr isStroke = std::dynamic_pointer_cast<RotoStrokeItem>(*it);

        if (isStroke) {
            exportStroke(indentLevel, isStroke, parentLayerName, ts);
        } else if (isBezier) {
            double time;
            const std::list<BezierCPPtr>& cps = isBezier->getControlPoints();
            const std::list<BezierCPPtr>& fps = isBezier->getFeatherPoints();

            if ( cps.empty() ) {
                continue;
            }

            time = cps.front()->getKeyframeTime(false, 0);

            WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("bezier = roto.createBezier(0, 0, ") + NUM_TIME(time) + QString::fromUtf8(")") );
            WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("bezier.setScriptName(") + ESC( isBezier->getScriptName() ) + QString::fromUtf8(")") );
            WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("bezier.setLabel(") + ESC( isBezier->getLabel() ) + QString::fromUtf8(")") );
            QString visibleStr = isBezier->isGloballyActivated() ? QString::fromUtf8("True") : QString::fromUtf8("False");
            WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("bezier.setVisible(") + visibleStr + QString::fromUtf8(")") );

            WRITE_INDENT(indentLevel); WRITE_STRING( parentLayerName + QString::fromUtf8(".addItem(bezier)") );
            WRITE_INDENT(indentLevel); WRITE_STATIC_LINE("");

            assert( cps.size() == fps.size() );

            std::set<double> kf;
            isBezier->getKeyframeTimes(&kf);

            //the last python call already registered the first control point
            int nbPts = cps.size() - 1;
            WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("for i in range(0, ") + NUM_INT(nbPts) + QString::fromUtf8("):") );
            WRITE_INDENT(2); WRITE_STATIC_LINE("bezier.addControlPoint(0,0)");

            ///Now that all points are created position them
            int idx = 0;
            std::list<BezierCPPtr>::const_iterator fpIt = fps.begin();
            for (std::list<BezierCPPtr>::const_iterator it2 = cps.begin(); it2 != cps.end(); ++it2, ++fpIt, ++idx) {
                for (std::set<double>::iterator it3 = kf.begin(); it3 != kf.end(); ++it3) {
                    exportBezierPointAtTime(indentLevel, *it2, false, *it3, idx, ts);
                    exportBezierPointAtTime(indentLevel, *fpIt, true, *it3, idx, ts);
                }
                if ( kf.empty() ) {
                    exportBezierPointAtTime(indentLevel, *it2, false, time, idx, ts);
                    exportBezierPointAtTime(indentLevel, *fpIt, true, time, idx, ts);
                }
            }
            // the keyframes that are not smooth, as a new shape's (e.g. linear, as Natron 2.3.15 and earlier made them)
            {
                std::list<std::pair<double, KeyframeTypeEnum> > keyInterpolations;
                cps.front()->getKeyFrames(false, &keyInterpolations);
                for (std::list<std::pair<double, KeyframeTypeEnum> >::const_iterator k = keyInterpolations.begin(); k != keyInterpolations.end(); ++k) {
                    if (k->second != eKeyframeTypeSmooth) {
                        WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("bezier.setKeyframeInterpolation(") + NUM_TIME(k->first) + QString::fromUtf8(", ") +
                                                                 keyframeTypeToPython(k->second) + QString::fromUtf8(")") );
                    }
                }
            }
            if ( isBezier->isCurveFinished() ) {
                WRITE_INDENT(indentLevel); WRITE_STRING("bezier.setCurveFinished(True)");
            }
            exportDrawableItemKnobs(indentLevel, isBezier, QString::fromUtf8("bezier"), ts);
            // Last: locking (or unlocking) a shape stops the changes of its parameters (RotoContext::onItemLockedChanged())
            if ( isBezier->getLocked() ) {
                WRITE_INDENT(indentLevel); WRITE_STRING("bezier.setLocked(True)");
            }

            WRITE_INDENT(indentLevel); WRITE_STATIC_LINE("del bezier");
        } else if (isLayer) {
            QString name =  QString::fromUtf8( isLayer->getScriptName().c_str() );
            QString layerName = name + QString::fromUtf8("_layer");
            WRITE_INDENT(indentLevel); WRITE_STRING( layerName + QString::fromUtf8(" = roto.createLayer()") );
            WRITE_INDENT(indentLevel); WRITE_STRING( layerName +  QString::fromUtf8(".setScriptName(") + ESC(name) + QString::fromUtf8(")") );
            WRITE_INDENT(indentLevel); WRITE_STRING( layerName + QString::fromUtf8(".setLabel(") + ESC( isLayer->getLabel() ) + QString::fromUtf8(")") );
            QString visibleStr = isLayer->isGloballyActivated() ? QString::fromUtf8("True") : QString::fromUtf8("False");
            WRITE_INDENT(indentLevel); WRITE_STRING( layerName + QString::fromUtf8(".setVisible(") + visibleStr + QString::fromUtf8(")") );

            WRITE_INDENT(indentLevel); WRITE_STRING( parentLayerName + QString::fromUtf8(".addItem(") + layerName + QString::fromUtf8(")") );

            const std::list<RotoItemPtr>& items = isLayer->getItems();
            exportRotoLayer(indentLevel, items, isLayer, ts);
            // after its items (see the shapes above)
            if ( isLayer->getLocked() ) {
                WRITE_INDENT(indentLevel); WRITE_STRING( layerName + QString::fromUtf8(".setLocked(True)") );
            }
            WRITE_INDENT(indentLevel); WRITE_STRING(QString::fromUtf8("del ") + layerName);
        }
        WRITE_STATIC_LINE("");
    }
} // exportRotoLayer

static void
exportAllNodeKnobs(int indentLevel,
                   const NodePtr& node,
                   QTextStream& ts)
{
    const KnobsVec& knobs = node->getKnobs();
    std::list<KnobPage*> userPages;

    for (KnobsVec::const_iterator it2 = knobs.begin(); it2 != knobs.end(); ++it2) {
        if ( (*it2)->getIsPersistent() && !(*it2)->isUserKnob() ) {
            QString getParamStr  = QString::fromUtf8("lastNode.getParam(\"");
            const std::string& paramName =  (*it2)->getName();
            if ( paramName.empty() ) {
                continue;
            }
            getParamStr += QString::fromUtf8( paramName.c_str() );
            getParamStr += QString::fromUtf8("\")");
            if ( exportKnobValues(indentLevel, *it2, getParamStr, true, ts) ) {
                WRITE_STATIC_LINE("");
            }
        }

        if ( (*it2)->isUserKnob() ) {
            KnobPage* isPage = dynamic_cast<KnobPage*>( it2->get() );
            if (isPage) {
                userPages.push_back(isPage);
            }
        }
    } // for (KnobsVec::const_iterator it2 = knobs.begin(); it2 != knobs.end(); ++it2)
    if ( !userPages.empty() ) {
        WRITE_STATIC_LINE("");
        WRITE_INDENT(indentLevel); WRITE_STATIC_LINE("# Create the user parameters");
    }
    for (std::list<KnobPage*>::iterator it2 = userPages.begin(); it2 != userPages.end(); ++it2) {
        WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("lastNode.") + QString::fromUtf8( (*it2)->getName().c_str() ) +
                                                 QString::fromUtf8(" = lastNode.createPageParam(") + ESC( (*it2)->getName() ) + QString::fromUtf8(", ") +
                                                 ESC( (*it2)->getLabel() ) + QString::fromUtf8(")") );
        KnobsVec children =  (*it2)->getChildren();
        for (KnobsVec::const_iterator it3 = children.begin(); it3 != children.end(); ++it3) {
            exportUserKnob(indentLevel, *it3, QString::fromUtf8("lastNode"), 0, *it2, ts);
        }
    }

    if ( !userPages.empty() ) {
        WRITE_INDENT(indentLevel); WRITE_STATIC_LINE("# Refresh the GUI with the newly created parameters");
        std::list<std::string> pagesOrdering = node->getPagesOrder();
        if ( !pagesOrdering.empty() ) {
            QString line = QString::fromUtf8("lastNode.setPagesOrder([");
            std::list<std::string>::iterator next = pagesOrdering.begin();
            ++next;
            for (std::list<std::string>::iterator it = pagesOrdering.begin(); it != pagesOrdering.end(); ++it) {
                line += QLatin1Char('\'');
                line += QString::fromUtf8( it->c_str() );
                line += QLatin1Char('\'');
                if ( next != pagesOrdering.end() ) {
                    line += QString::fromUtf8(", ");
                    ++next;
                }
            }
            line += QString::fromUtf8("])");
            WRITE_INDENT(indentLevel); WRITE_STRING(line);
        }
        WRITE_INDENT(indentLevel); WRITE_STATIC_LINE("lastNode.refreshUserParamsGUI()");
    }

    RotoContextPtr roto = node->getRotoContext();
    if (roto) {
        const std::list<RotoLayerPtr>& layers = roto->getLayers();

        if ( !layers.empty() ) {
            WRITE_INDENT(indentLevel); WRITE_STATIC_LINE("# For the roto node, create all layers and beziers");
            WRITE_INDENT(indentLevel); WRITE_STRING("roto = lastNode.getRotoContext()");
            RotoLayerPtr baseLayer = layers.front();
            QString baseLayerName = QString::fromUtf8( baseLayer->getScriptName().c_str() );
            QString baseLayerToken = baseLayerName + QString::fromUtf8("_layer");
            WRITE_INDENT(indentLevel); WRITE_STRING( baseLayerToken + QString::fromUtf8(" = roto.getBaseLayer()") );

            WRITE_INDENT(indentLevel); WRITE_STRING( baseLayerToken + QString::fromUtf8(".setScriptName(") + ESC(baseLayerName) + QString::fromUtf8(")") );
            WRITE_INDENT(indentLevel); WRITE_STRING( baseLayerToken + QString::fromUtf8(".setLabel(") + ESC( baseLayer->getLabel() ) + QString::fromUtf8(")") );
            QString visibleStr = baseLayer->isGloballyActivated() ? QString::fromUtf8("True") : QString::fromUtf8("False");
            WRITE_INDENT(indentLevel); WRITE_STRING( baseLayerToken + QString::fromUtf8(".setVisible(") + visibleStr + QString::fromUtf8(")") );
            exportRotoLayer(indentLevel, baseLayer->getItems(), baseLayer,  ts);
            // after its items (see exportRotoLayer())
            if ( baseLayer->getLocked() ) {
                WRITE_INDENT(indentLevel); WRITE_STRING( baseLayerToken + QString::fromUtf8(".setLocked(True)") );
            }
            WRITE_INDENT(indentLevel); WRITE_STRING(QString::fromUtf8("del ") + baseLayerToken);
            WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("del roto") );
        }
    }

    // The tracks, after the parameters of the node: with no track, they don't compute the transform
    // (TrackerContext::solveTransformParams())
    TrackerContextPtr tracker = node->getTrackerContext();
    std::vector<TrackMarkerPtr> markers;
    if (tracker) {
        tracker->getAllMarkers(&markers);
    }
    if ( !markers.empty() ) {
        WRITE_INDENT(indentLevel); WRITE_STATIC_LINE("# For the tracker node, create all tracks");
        WRITE_INDENT(indentLevel); WRITE_STRING("tracker = lastNode.getTrackerContext()");
        for (std::vector<TrackMarkerPtr>::const_iterator it = markers.begin(); it != markers.end(); ++it) {
            // a track uses pattern matching or not, whatever the node's parameter now says (TrackerContext::load())
            const bool isPM = dynamic_cast<const TrackMarkerPM*>( it->get() ) != 0;
            WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("track = tracker.createTrack(") + QString::fromUtf8(isPM ? "True" : "False") + QString::fromUtf8(")") );
            WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("track.setScriptName(") + ESC( (*it)->getScriptName_mt_safe() ) + QString::fromUtf8(")") );
            WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("track.setLabel(") + ESC( (*it)->getLabel() ) + QString::fromUtf8(")") );
            // all its parameters, as TrackMarker::save()
            const KnobsVec trackKnobs = (*it)->getKnobs_mt_safe();
            for (KnobsVec::const_iterator k = trackKnobs.begin(); k != trackKnobs.end(); ++k) {
                exportKnobValues(indentLevel, *k, QString::fromUtf8("track.getParam(") + ESC( (*k)->getName() ) + QString::fromUtf8(")"), true, ts);
            }
            std::set<int> userKeyframes;
            (*it)->getUserKeyframes(&userKeyframes);
            for (std::set<int>::const_iterator key = userKeyframes.begin(); key != userKeyframes.end(); ++key) {
                WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("track.setUserKeyframe(") + NUM_INT(*key) + QString::fromUtf8(")") );
            }
            WRITE_INDENT(indentLevel); WRITE_STATIC_LINE("del track");
        }
        WRITE_INDENT(indentLevel); WRITE_STATIC_LINE("del tracker");
    }
} // exportAllNodeKnobs

// The parameter values of a PyPlug instance. Its PyPlug creates its user parameters and its nodes (see exportGroupInternal()).
static void
exportPyPlugKnobValues(int indentLevel,
                       const NodePtr& node,
                       QTextStream& ts)
{
    const KnobsVec& knobs = node->getKnobs();

    for (KnobsVec::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        if ( !(*it)->getIsPersistent() || (*it)->getName().empty() ) {
            continue;
        }
        // its user parameters (the ones its PyPlug creates) keep the value that the instance had when it was
        // saved: the PyPlug may have changed its default value since (e.g. Glow's iterations, in a Natron 2.0 project)
        const ExportedValuesEnum exportedValues = (*it)->isUserKnob() ? eExportedValuesAll : eExportedValuesModified;
        if ( exportKnobValues(indentLevel, *it, QString::fromUtf8("lastNode.getParam(") + ESC( (*it)->getName() ) + QString::fromUtf8(")"), true, ts, exportedValues) ) {
            WRITE_STATIC_LINE("");
        }
    }
}

// The parameters of the nodes inside a PyPlug instance, which the PyPlug creates: the .ntp loader restores the saved
// ones over them (NodeCollectionSerialization::restoreFromSerialization()). A node that the PyPlug no longer has is
// skipped; the nodes of a Group inside the PyPlug are not.
static void
exportPyPlugNodesKnobValues(int indentLevel,
                            const NodeCollection* pyPlug,
                            const QString& pyPlugName,
                            QTextStream& ts)
{
    assert(pyPlug);
    const NodesList nodes = pyPlug->getNodes();

    for (NodesList::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        if ( !(*it)->isActivated() || !(*it)->isPartOfProject() || (*it)->isEffectViewer() ) {
            continue;
        }
        QString values;
        {
            QTextStream valuesStream(&values);
            const KnobsVec& knobs = (*it)->getKnobs();
            for (KnobsVec::const_iterator k = knobs.begin(); k != knobs.end(); ++k) {
                if ( (*k)->getIsPersistent() && !(*k)->getName().empty() ) {
                    exportKnobValues(indentLevel + 1, *k, QString::fromUtf8("pyPlugNode.getParam(") + ESC( (*k)->getName() ) + QString::fromUtf8(")"), true, valuesStream);
                }
            }
        }
        if ( values.isEmpty() ) {
            continue;
        }
        WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("pyPlugNode = ") + pyPlugName + QString::fromUtf8(".getNode(") + ESC( (*it)->getScriptName_mt_safe() ) + QString::fromUtf8(")") );
        WRITE_INDENT(indentLevel); WRITE_STATIC_LINE("if pyPlugNode is not None:");
        ts << values;
        WRITE_INDENT(indentLevel); WRITE_STATIC_LINE("del pyPlugNode");
    }
}

static bool
exportKnobLinks(int indentLevel,
                const NodePtr& groupNode,
                const NodePtr& node,
                const QString& groupName,
                const QString& nodeName,
                QTextStream& ts)
{
    bool hasExportedLink = false;
    const KnobsVec& knobs = node->getKnobs();

    for (KnobsVec::const_iterator it2 = knobs.begin(); it2 != knobs.end(); ++it2) {
        QString paramName = nodeName + QString::fromUtf8(".getParam(\"") + QString::fromUtf8( (*it2)->getName().c_str() ) + QString::fromUtf8("\")");
        bool hasDefined = false;

        //Check for alias link
        KnobIPtr alias = (*it2)->getAliasMaster();
        if (alias) {
            if (!hasDefined) {
                WRITE_INDENT(indentLevel); WRITE_STRING(QString::fromUtf8("param = ") + paramName);
                hasDefined = true;
            }
            hasExportedLink = true;

            EffectInstance* aliasHolder = dynamic_cast<EffectInstance*>( alias->getHolder() );
            assert(aliasHolder);
            if (!aliasHolder) {
                throw std::logic_error("exportKnobLinks");
            }
            QString aliasName;
            if ( groupNode && aliasHolder == groupNode->getEffectInstance().get() ) {
                aliasName = groupName;
            } else {
                aliasName = groupName + QString::fromUtf8( aliasHolder->getNode()->getScriptName_mt_safe().c_str() );
            }
            aliasName += QString::fromUtf8(".getParam(");
            aliasName += ESC(QString::fromUtf8( alias->getName().c_str() ));
            aliasName += QString::fromUtf8(")");

            WRITE_INDENT(indentLevel); WRITE_STRING( aliasName + QString::fromUtf8(".setAsAlias(param)") );
        } else {
            for (int i = 0; i < (*it2)->getDimension(); ++i) {
                std::string expr = (*it2)->getExpression(i);
                QString hasRetVar = (*it2)->isExpressionUsingRetVariable(i) ? QString::fromUtf8("True") : QString::fromUtf8("False");
                if ( !expr.empty() ) {
                    if (!hasDefined) {
                        WRITE_INDENT(indentLevel); WRITE_STRING(QString::fromUtf8("param = ") + paramName);
                        hasDefined = true;
                    }
                    hasExportedLink = true;
                    WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("param.setExpression(") + ESC(expr) + QString::fromUtf8(", ") +
                                                             hasRetVar + QString::fromUtf8(", ") + NUM_INT(i) + QString::fromUtf8(")") );
                }

                std::pair<int, KnobIPtr> master = (*it2)->getMaster(i);
                if (master.second) {
                    if (!hasDefined) {
                        WRITE_INDENT(indentLevel); WRITE_STRING(QString::fromUtf8("param = ") + paramName);
                        hasDefined = true;
                    }
                    hasExportedLink = true;

                    EffectInstance* masterHolder = dynamic_cast<EffectInstance*>( master.second->getHolder() );
                    assert(masterHolder);
                    if (!masterHolder) {
                        throw std::logic_error("exportKnobLinks");
                    }
                    QString masterName;
                    if ( groupNode && masterHolder == groupNode->getEffectInstance().get() ) {
                        masterName = groupName;
                    } else {
                        masterName = groupName + QString::fromUtf8( masterHolder->getNode()->getScriptName_mt_safe().c_str() );
                    }
                    masterName += QLatin1String(".getParam(");
                    masterName += ESC(QString::fromUtf8( master.second->getName().c_str() ));
                    masterName += QLatin1String(")");


                    WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("param.slaveTo(") +  masterName + QString::fromUtf8(", ") +
                                                             NUM_INT(i) + QString::fromUtf8(", ") + NUM_INT(master.first) + QString::fromUtf8(")") );
                }
            }
        }
        if (hasDefined) {
            WRITE_INDENT(indentLevel); WRITE_STATIC_LINE("del param");
        }
    }

    return hasExportedLink;
} // exportKnobLinks

// exportViewers: false for a PyPlug, true for a project (see Project::exportToPython())
static void
exportGroupInternal(int indentLevel,
                    const NodeCollection* collection,
                    const NodePtr& upperLevelGroupNode,
                    const QString& upperLevelGroupName,
                    bool exportViewers,
                    QTextStream& ts)
{
    assert(collection);
    if (!collection) {
        throw std::logic_error(__func__);
    }
    WRITE_INDENT(indentLevel); WRITE_STATIC_LINE("# Create all nodes in the group");
    WRITE_STATIC_LINE("");

    const NodeGroup* isGroup = dynamic_cast<const NodeGroup*>(collection);
    NodePtr groupNode;
    if (isGroup) {
        groupNode = isGroup->getNode();
    }

    QString groupName = upperLevelGroupName + QString::fromUtf8("group");

    // The group of a PyPlug (the top level, which the script doesn't create). A Group inside it, or in a project,
    // had its parameters exported with the nodes of its parent group: exporting them again created its user
    // parameters and the tracks of a Tracker (a NodeGroup) twice.
    if ( isGroup && upperLevelGroupName.isEmpty() ) {
        WRITE_INDENT(indentLevel); WRITE_STATIC_LINE("# Create the parameters of the group node the same way we did for all internal nodes");
        WRITE_INDENT(indentLevel); WRITE_STRING(QString::fromUtf8("lastNode = ") + groupName);
        double r, g, b;
        bool hasColor = groupNode->getColor(&r, &g, &b);
        if (hasColor) {
            // TODO: we could check if the color was actually changed from the default (NodeGui::getColorFromGrouping())
            
            // a precision of 3 digits is enough for the node color
            WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("lastNode.setColor(") + NUM_COLOR(r) + QString::fromUtf8(", ") + NUM_COLOR(g) + QString::fromUtf8(", ") + NUM_COLOR(b) +  QString::fromUtf8(")") );
        }
        exportAllNodeKnobs(indentLevel, groupNode, ts);
        WRITE_INDENT(indentLevel); WRITE_STATIC_LINE("del lastNode");
        WRITE_STATIC_LINE("");
    }


    NodesList nodes = collection->getNodes();
    NodesList exportedNodes;

    ///Re-order nodes so we're sure Roto nodes get exported in the end since they may depend on Trackers
    NodesList rotos;
    NodesList newNodes;
    for (NodesList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        if ( (*it)->isRotoPaintingNode() || (*it)->isRotoNode() ) {
            rotos.push_back(*it);
        } else {
            newNodes.push_back(*it);
        }
    }
    newNodes.insert( newNodes.end(), rotos.begin(), rotos.end() );

    for (NodesList::iterator it = newNodes.begin(); it != newNodes.end(); ++it) {
        ///Don't create viewer while exporting a PyPlug
        ViewerInstance* isViewer = (*it)->isEffectViewer();
        if (isViewer && !exportViewers) {
            continue;
        }
        // the internal nodes of other nodes (e.g. the decoder of a Read) are created by them, as for an .ntp file
        // (NodeCollectionSerialization::initialize())
        if ( !(*it)->isActivated() || !(*it)->isPartOfProject() ) {
            continue;
        }

        exportedNodes.push_back(*it);

        ///Let the parent of the multi-instance node create the children
        if ( (*it)->getParentMultiInstance() ) {
            continue;
        }

        if (isViewer) {
            // NatronRenderer has no Viewers (app.createNode() returns None): only their name, position and inputs
            // (see below), not their settings, which are GUI data
            const QString viewerName = groupName + QString::fromUtf8( (*it)->getScriptName_mt_safe().c_str() );
            WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("# Viewer ") + ESC( (*it)->getScriptName_mt_safe() ) );
            WRITE_INDENT(indentLevel); WRITE_STRING( viewerName + QString::fromUtf8(" = app.createNode(") + ESC( (*it)->getPluginID() ) + QString::fromUtf8(", ") +
                                                     NUM_INT( (*it)->getMajorVersion() ) + QString::fromUtf8(", ") + groupName + QString::fromUtf8(")") );
            WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("if ") + viewerName + QString::fromUtf8(" is not None:") );
            WRITE_INDENT(indentLevel + 1); WRITE_STRING( viewerName + QString::fromUtf8(".setScriptName(") + ESC( (*it)->getScriptName_mt_safe() ) + QString::fromUtf8(")") );
            double r, g, b;
            if ( (*it)->getColor(&r, &g, &b) ) {
                double x, y;
                (*it)->getPosition(&x, &y);
                WRITE_INDENT(indentLevel + 1); WRITE_STRING( viewerName + QString::fromUtf8(".setPosition(") + NUM_PIXEL(x) + QString::fromUtf8(", ") + NUM_PIXEL(y) + QString::fromUtf8(")") );
            }
            WRITE_STATIC_LINE("");
            continue;
        }

        QString nodeName = QString::fromUtf8( (*it)->getPluginID().c_str() );

        // A Read (Write) creates its decoder (encoder) when it is created: give it its file and its decoder
        // (encoder), as the .ntp loader does (ReadNode::onKnobsAboutToBeLoaded()). Set later, while the script
        // runs, they would not create it (no knob changes are handled then, see EffectInstance::onKnobValueChanged()),
        // and the node would render nothing.
        QString createProperties;
        {
            const char* pluginIDKnobName = 0;
            if ( (*it)->getPluginID() == PLUGINID_NATRON_READ ) {
                pluginIDKnobName = kNatronReadNodeParamDecodingPluginID;
            } else if ( (*it)->getPluginID() == PLUGINID_NATRON_WRITE ) {
                pluginIDKnobName = kNatronWriteNodeParamEncodingPluginID;
            }
            KnobStringBase* pluginIDKnob = pluginIDKnobName ? dynamic_cast<KnobStringBase*>( (*it)->getKnobByName(pluginIDKnobName).get() ) : 0;
            KnobStringBase* fileKnob = pluginIDKnobName ? dynamic_cast<KnobStringBase*>( (*it)->getKnobByName(kOfxImageEffectFileParamName).get() ) : 0;
            if (pluginIDKnob && fileKnob) {
                const QString prop = QString::fromUtf8(NATRON_ENGINE_PYTHON_MODULE_NAME ".StringNodeCreationProperty(");
                createProperties = QString::fromUtf8(", {\"" kCreateNodeArgsPropNodeInitialParamValues "\": ") + prop + QString::fromUtf8("[") +
                                   ESC( std::string(pluginIDKnobName) ) + QString::fromUtf8(", ") + ESC( std::string(kOfxImageEffectFileParamName) ) + QString::fromUtf8("]), ") +
                                   ESC( QString::fromUtf8(kCreateNodeArgsPropParamValue "_") + QString::fromUtf8(pluginIDKnobName) ) + QString::fromUtf8(": ") +
                                   prop + ESC( pluginIDKnob->getValue() ) + QString::fromUtf8("), ") +
                                   ESC( QString::fromUtf8(kCreateNodeArgsPropParamValue "_" kOfxImageEffectFileParamName) ) + QString::fromUtf8(": ") +
                                   prop + ESC( fileKnob->getValue() ) + QString::fromUtf8(")}");
            }
        }

        WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("# Start of node ") + ESC( (*it)->getScriptName_mt_safe() ) );
        WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("lastNode = app.createNode(") + ESC(nodeName) + QString::fromUtf8(", ") +
                                                 NUM_INT( (*it)->getMajorVersion() ) + QString::fromUtf8(", ") + groupName +
                                                 createProperties + QString::fromUtf8(")") );
        if ( !dynamic_cast<GroupOutput*>( (*it)->getEffectInstance().get() ) ) {
            // Do not set script name of Output nodes:
            // it triggers an exception in Node::setScriptName()
            // see https://github.com/MrKepzie/Natron/issues/1452
            WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("lastNode.setScriptName(") + ESC( (*it)->getScriptName_mt_safe() ) + QString::fromUtf8(")") );
        }
        WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("lastNode.setLabel(") + ESC( (*it)->getLabel_mt_safe() ) + QString::fromUtf8(")") );
        // Without a GUI (NatronRenderer), a node has a position, size or colour only if its project file or a
        // script gave it one (see Node::getGuiDataWithoutGui()): don't write zeros (black nodes)
        if ( (*it)->hasGuiData() ) {
            double x, y;
            (*it)->getPosition(&x, &y);
            double w, h;
            (*it)->getSize(&w, &h);
            // a precision of 1 pixel is enough for the position on the nodegraph
            WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("lastNode.setPosition(") + NUM_PIXEL(x) + QString::fromUtf8(", ") + NUM_PIXEL(y) + QString::fromUtf8(")") );
            // without a GUI, a node that was only given a position has no size
            if ( (w > 0) && (h > 0) ) {
                WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("lastNode.setSize(") + NUM_PIXEL(w) + QString::fromUtf8(", ") + NUM_PIXEL(h) + QString::fromUtf8(")") );
            }
        }
        double r, g, b;
        if ( (*it)->getColor(&r, &g, &b) ) {
            // a precision of 3 digits is enough for the node color
            WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("lastNode.setColor(") + NUM_COLOR(r) + QString::fromUtf8(", ") + NUM_COLOR(g) + QString::fromUtf8(", ") + NUM_COLOR(b) +  QString::fromUtf8(")") );
        }

        std::list<ImagePlaneDesc> userComps;
        (*it)->getUserCreatedComponents(&userComps);
        for (std::list<ImagePlaneDesc>::iterator it2 = userComps.begin(); it2 != userComps.end(); ++it2) {
            const std::vector<std::string>& channels = it2->getChannels();
            QString compStr = QString::fromUtf8("[");
            for (std::size_t i = 0; i < channels.size(); ++i) {
                compStr.append( ESC(channels[i]) );
                if ( i < (channels.size() - 1) ) {
                    compStr.push_back( QLatin1Char(',') );
                }
            }
            compStr.push_back( QLatin1Char(']') );
            WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("lastNode.addUserPlane(") + ESC( it2->getPlaneLabel() ) + QString::fromUtf8(", ") + compStr +  QString::fromUtf8(")") );
        }

        QString nodeNameInScript = groupName + QString::fromUtf8( (*it)->getScriptName_mt_safe().c_str() );
        WRITE_INDENT(indentLevel); WRITE_STRING( nodeNameInScript + QString::fromUtf8(" = lastNode") );
        WRITE_STATIC_LINE("");
        // A PyPlug instance is created by its PyPlug, with its user parameters and its nodes: only set the
        // parameters, its own and its nodes'. Nodes added or removed inside it are not exported (Convert to
        // Group exports them).
        const bool isPyPlug = (*it)->isPyPlug();
        if (isPyPlug) {
            exportPyPlugKnobValues(indentLevel, *it, ts);
            if ( const NodeGroup* pyPlug = (*it)->isEffectGroup() ) {
                exportPyPlugNodesKnobValues(indentLevel, pyPlug, nodeNameInScript, ts);
            }
        } else {
            exportAllNodeKnobs(indentLevel, *it, ts);
        }
        WRITE_INDENT(indentLevel); WRITE_STRING("del lastNode");
        WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("# End of node ") + ESC( (*it)->getScriptName_mt_safe() ) );
        WRITE_STATIC_LINE("");

        std::list<NodePtr> children;
        (*it)->getChildrenMultiInstance(&children);
        if ( !children.empty() ) {
            WRITE_INDENT(indentLevel); WRITE_STATIC_LINE("# Create children if the node is a multi-instance such as a tracker");
            for (std::list<NodePtr> ::iterator it2 = children.begin(); it2 != children.end(); ++it2) {
                if ( (*it2)->isActivated() ) {
                    // as NodeCollectionSerialization::restoreFromSerialization() (there is no Effect.createChild())
                    WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("lastNode = app.createNode(") + ESC( (*it2)->getPluginID() ) + QString::fromUtf8(", ") +
                                                             NUM_INT( (*it2)->getMajorVersion() ) + QString::fromUtf8(", ") + groupName +
                                                             QString::fromUtf8(", {\"" kCreateNodeArgsPropMultiInstanceParentName "\": " NATRON_ENGINE_PYTHON_MODULE_NAME ".StringNodeCreationProperty(") +
                                                             ESC( (*it)->getScriptName_mt_safe() ) + QString::fromUtf8(")})") );
                    WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("lastNode.setScriptName(\"") + QString::fromUtf8( (*it2)->getScriptName_mt_safe().c_str() ) + QString::fromUtf8("\")") );
                    WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("lastNode.setLabel(\"") + QString::fromUtf8( (*it2)->getLabel_mt_safe().c_str() ) + QString::fromUtf8("\")") );
                    exportAllNodeKnobs(indentLevel, *it2, ts);
                    // the name that the links below use (exportKnobLinks())
                    WRITE_INDENT(indentLevel); WRITE_STRING( groupName + QString::fromUtf8( (*it2)->getScriptName_mt_safe().c_str() ) + QString::fromUtf8(" = lastNode") );
                    WRITE_INDENT(indentLevel); WRITE_STRING("del lastNode");
                }
            }
            WRITE_STATIC_LINE("");
        }

        NodeGroup* isGrp = (*it)->isEffectGroup();
        if (isGrp && !isPyPlug) {
            WRITE_INDENT(indentLevel); WRITE_STRING(groupName + QString::fromUtf8("group = ") + nodeNameInScript);
            exportGroupInternal(indentLevel, isGrp, groupNode, groupName, exportViewers, ts);
            WRITE_STATIC_LINE("");
        }
    }


    WRITE_INDENT(indentLevel); WRITE_STATIC_LINE("# Now that all nodes are created we can connect them together, restore expressions");
    bool hasConnected = false;
    for (NodesList::iterator it = exportedNodes.begin(); it != exportedNodes.end(); ++it) {
        QString nodeQualifiedName( groupName + QString::fromUtf8( (*it)->getScriptName_mt_safe().c_str() ) );

        if ( !(*it)->getParentMultiInstance() ) {
            // a Viewer is None in NatronRenderer
            const bool isViewer = (*it)->isEffectViewer() != 0;
            bool hasInput = false;
            for (int i = 0; i < (*it)->getNInputs(); ++i) {
                NodePtr inputNode = (*it)->getRealInput(i);
                if (inputNode) {
                    if (isViewer && !hasInput) {
                        WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("if ") + nodeQualifiedName + QString::fromUtf8(" is not None:") );
                    }
                    hasInput = true;
                    hasConnected = true;
                    QString inputQualifiedName( groupName  + QString::fromUtf8( inputNode->getScriptName_mt_safe().c_str() ) );
                    const int connectIndent = isViewer ? indentLevel + 1 : indentLevel;
                    WRITE_INDENT(connectIndent); WRITE_STRING( nodeQualifiedName + QString::fromUtf8(".connectInput(") + NUM_INT(i) +
                                                                 QString::fromUtf8(", ") + inputQualifiedName + QString::fromUtf8(")") );
                }
            }
        }
    }
    if (hasConnected) {
        WRITE_STATIC_LINE("");
    }

    bool hasExported = false;

    for (NodesList::iterator it = exportedNodes.begin(); it != exportedNodes.end(); ++it) {
        if ( (*it)->isEffectViewer() ) {
            continue; // no settings (see above)
        }
        QString nodeQualifiedName( groupName + QString::fromUtf8( (*it)->getScriptName_mt_safe().c_str() ) );
        if ( exportKnobLinks(indentLevel, groupNode, *it, groupName, nodeQualifiedName, ts) ) {
            hasExported = true;
        }
    }
    if (hasExported) {
        WRITE_STATIC_LINE("");
    }
    if (isGroup) {
        exportKnobLinks(indentLevel, upperLevelGroupNode ? upperLevelGroupNode : groupNode, groupNode,
                        upperLevelGroupNode ? upperLevelGroupName : groupName, groupName, ts);
    }
} // exportGroupInternal

void
NodeCollection::exportGroupToPython(const QString& pluginID,
                                    const QString& pluginLabel,
                                    const QString& pluginDescription,
                                    const QString& pluginIconPath,
                                    const QString& pluginGrouping,
                                    int version,
                                    QString& output)
{
    QString extModule(pluginLabel);

    extModule.append( QString::fromUtf8("Ext") );

    QTextStream ts(&output);
    gExportedBytes = 0;
    // coding must be set in first or second line, see https://www.python.org/dev/peps/pep-0263/
    WRITE_STATIC_LINE("# -*- coding: utf-8 -*-");
    WRITE_STATIC_LINE("# DO NOT EDIT THIS FILE");
    WRITE_STATIC_LINE(NATRON_PYPLUG_MAGIC);
    QString descline = QString( QString::fromUtf8(NATRON_PYPLUG_GENERATED "%1 PyPlug exporter version %2.") ).arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ).arg(NATRON_PYPLUG_EXPORTER_VERSION);
    WRITE_STRING(descline);
    WRITE_STATIC_LINE("");
    QString handWrittenStr = QString::fromUtf8("# Hand-written code should be added in a separate file named %1.py").arg(extModule);
    WRITE_STRING(handWrittenStr);
    // The documentation of this Natron version (#1272)
    QString docLine = QString::fromUtf8("# See https://natron.readthedocs.io/en/rb-%1.%2/devel/pyplug-writing.html#pyplugwriting-ext").arg(NATRON_VERSION_MAJOR).arg(NATRON_VERSION_MINOR);
    WRITE_STRING(docLine);
    WRITE_STATIC_LINE("# Note that Viewers are never exported");
    WRITE_STATIC_LINE("");
    WRITE_STATIC_LINE("import " NATRON_ENGINE_PYTHON_MODULE_NAME);
    WRITE_STATIC_LINE("import sys");
    WRITE_STATIC_LINE("");
    WRITE_STATIC_LINE("# Try to import the extensions file where callbacks and hand-written code should be located.");
    WRITE_STATIC_LINE("try:");


    WRITE_INDENT(1); WRITE_STRING( QString::fromUtf8("from ") + extModule + QString::fromUtf8(" import *") );
    WRITE_STRING("except ImportError:");
    WRITE_INDENT(1); WRITE_STRING("pass");
    WRITE_STATIC_LINE("");

    WRITE_STATIC_LINE("def getPluginID():");
    WRITE_INDENT(1); WRITE_STRING( QString::fromUtf8("return \"") + pluginID + QString::fromUtf8("\"") );
    WRITE_STATIC_LINE("");

    WRITE_STATIC_LINE("def getLabel():");
    WRITE_INDENT(1); WRITE_STRING( QString::fromUtf8("return ") + ESC(pluginLabel) );
    WRITE_STATIC_LINE("");

    WRITE_STATIC_LINE("def getVersion():");
    WRITE_INDENT(1); WRITE_STRING(QString::fromUtf8("return ") + NUM_INT(version));
    WRITE_STATIC_LINE("");

    if ( !pluginIconPath.isEmpty() ) {
        WRITE_STATIC_LINE("def getIconPath():");
        WRITE_INDENT(1); WRITE_STRING( QString::fromUtf8("return ") + ESC(pluginIconPath) );
        WRITE_STATIC_LINE("");
    }

    WRITE_STATIC_LINE("def getGrouping():");
    WRITE_INDENT(1); WRITE_STRING( QString::fromUtf8("return \"") + pluginGrouping + QString::fromUtf8("\"") );
    WRITE_STATIC_LINE("");

    if ( !pluginDescription.isEmpty() ) {
        WRITE_STATIC_LINE("def getPluginDescription():");
        WRITE_INDENT(1); WRITE_STRING( QString::fromUtf8("return ") + ESC(pluginDescription) );
        WRITE_STATIC_LINE("");
    }


    WRITE_STATIC_LINE("def createInstance(app,group):");

    exportGroupInternal(1, this, NodePtr(), QString(), false, ts);

    ///Import user hand-written code
    WRITE_INDENT(1); WRITE_STATIC_LINE("try:");
    WRITE_INDENT(2); WRITE_STRING( QString::fromUtf8("extModule = sys.modules[") + ESC(extModule) + QString::fromUtf8("]") );
    WRITE_INDENT(1); WRITE_STATIC_LINE("except KeyError:");
    WRITE_INDENT(2); WRITE_STATIC_LINE("extModule = None");

    QString testAttr = QString::fromUtf8("if extModule is not None and hasattr(extModule ,\"createInstanceExt\") and hasattr(extModule.createInstanceExt,\"__call__\"):");
    WRITE_INDENT(1); WRITE_STRING(testAttr);
    WRITE_INDENT(2); WRITE_STRING("extModule.createInstanceExt(app,group)");
} // NodeCollection::exportGroupToPython

// The project settings, as the .ntp loader restores them (see ProjectPrivate::restoreFromSerialization())
static void
exportProjectSettings(int indentLevel,
                      Project* project,
                      QTextStream& ts)
{
    WRITE_INDENT(indentLevel); WRITE_STATIC_LINE("# Project settings");

    // the formats that the project format may be, before it is set
    std::list<Format> formats;
    project->getAdditionalFormats(&formats);
    for (std::list<Format>::const_iterator it = formats.begin(); it != formats.end(); ++it) {
        QString spec;
        if ( !it->getName().empty() ) {
            spec = QString::fromUtf8( it->getName().c_str() ) + QLatin1Char(' ');
        }
        spec += NUM_INT( it->width() ) + QLatin1Char('x') + NUM_INT( it->height() ) + QLatin1Char(' ') + NUM_VALUE( it->getPixelAspectRatio() );
        WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("app.addFormat(") + ESC(spec) + QString::fromUtf8(")") );
    }

    const KnobPathPtr envVars = project->getEnvVarKnob();
    const KnobsVec& knobs = project->getKnobs();
    for (KnobsVec::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        // What each save stamps (the loader sets the version, as for an .ntp file): no difference between two saves
        // of the same project
        const std::string& name = (*it)->getName();
        if ( !(*it)->getIsPersistent() || (name == kProjectParamSoftwareVersion) || (name == kProjectParamLastSaveDate) ||
             (name == kProjectParamLastAuthor) || (name == kProjectParamFrameRange) ) { // the frame range: after the nodes (see exportToPython())
            continue;
        }
        if (*it == envVars) {
            // The loader sets [Project] (the folder of the file) and [OCIO] (the OCIO configuration of this
            // computer) before running the script, as the .ntp loader does: add the other project paths to them.
            std::list<std::vector<std::string> > table;
            envVars->decodeFromKnobTableFormat(envVars->getValue(), &table);
            for (std::list<std::vector<std::string> >::iterator row = table.begin(); row != table.end(); ) {
                if ( !row->empty() && ( ( (*row)[0] == NATRON_PROJECT_ENV_VAR_NAME ) || ( (*row)[0] == NATRON_OCIO_ENV_VAR_NAME ) ) ) {
                    row = table.erase(row);
                } else {
                    ++row;
                }
            }
            if ( !table.empty() ) {
                WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("param = app.getProjectParam(") + ESC( envVars->getName() ) + QString::fromUtf8(")") );
                WRITE_INDENT(indentLevel); WRITE_STRING( QString::fromUtf8("param.setValue(param.getValue() + ") + ESC( envVars->encodeToKnobTableFormat(table) ) + QString::fromUtf8(")") );
                WRITE_INDENT(indentLevel); WRITE_STATIC_LINE("del param");
            }
            continue;
        }
        exportKnobValues(indentLevel, *it, QString::fromUtf8("app.getProjectParam(") + ESC( (*it)->getName() ) + QString::fromUtf8(")"), true, ts);
    }
    WRITE_STATIC_LINE("");
} // exportProjectSettings

// Project's member, defined here to use the PyPlug exporter above. Main thread only (gExportedBytes).
QString
Project::exportToPython()
{
    QString output;
    QTextStream ts(&output);

    gExportedBytes = 0;
    // coding must be set in first or second line, see https://www.python.org/dev/peps/pep-0263/
    WRITE_STATIC_LINE("# -*- coding: utf-8 -*-");
    WRITE_STATIC_LINE(NATRON_PROJECT_PYTHON_MAGIC);
    WRITE_STRING( QString::fromUtf8("# Saved by %1 %2 (Python project format version %3).").arg( QString::fromUtf8(NATRON_APPLICATION_NAME), QString::fromUtf8(NATRON_VERSION_STRING) ).arg(NATRON_PROJECT_PYTHON_EXPORTER_VERSION) );
    WRITE_STRING( QString::fromUtf8("# Open it with File > Open, or with %1Renderer or app.loadProject(), as a .%2 project.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME), QString::fromUtf8(NATRON_PROJECT_FILE_EXT) ) );
    WRITE_STATIC_LINE("");
    WRITE_STATIC_LINE("import " NATRON_ENGINE_PYTHON_MODULE_NAME);
    WRITE_STATIC_LINE("");
    WRITE_STATIC_LINE("");
    WRITE_STATIC_LINE("def createProject(app):");
    WRITE_INDENT(1); WRITE_STATIC_LINE("# the nodes of the project are in app, its top-level group");
    WRITE_INDENT(1); WRITE_STATIC_LINE("group = app");
    WRITE_STATIC_LINE("");
    exportProjectSettings(1, this, ts);
    exportGroupInternal(1, this, NodePtr(), QString(), true, ts);
    // After the nodes, which extend it when they read their file (Node::onFileNameParameterChanged()); the .ntp
    // loader restores it as saved
    const KnobIPtr frameRange = getKnobByName(kProjectParamFrameRange);
    if (frameRange) {
        WRITE_INDENT(1); WRITE_STATIC_LINE("# Project frame range");
        exportKnobValues(1, frameRange, QString::fromUtf8("app.getProjectParam(") + ESC( frameRange->getName() ) + QString::fromUtf8(")"), true, ts, eExportedValuesAll);
    }
    WRITE_STATIC_LINE("");
    WRITE_STATIC_LINE("");
    // the loader runs the file with "app" set to the project's app (see AppInstance::runPythonProjectScript())
    WRITE_STATIC_LINE("if __name__ == \"__main__\":");
    WRITE_INDENT(1); WRITE_STATIC_LINE("createProject(app)");
    ts.flush();

    return output;
} // Project::exportToPython

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_NodeGroup.cpp"
