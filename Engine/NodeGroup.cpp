/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2017 INRIA and Alexandre Gauthier-Foichat
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
#include <sstream> // stringstream

#include <QtCore/QCoreApplication>
#include <QtCore/QTextStream>

#include "Engine/AppInstance.h"
#include "Engine/Bezier.h"
#include "Engine/BezierCP.h"
#include "Engine/Curve.h"
#include "Engine/CreateNodeArgs.h"
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
#include "Engine/RotoLayer.h"
#include "Engine/Settings.h"
#include "Engine/TimeLine.h"
#include "Engine/ViewIdx.h"
#include "Engine/ViewerInstance.h"

#include "Serialization/NodeSerialization.h"

NATRON_NAMESPACE_ENTER;

struct NodeCollectionPrivate
{
    AppInstanceWPtr app;
    NodeGraphI* graph;
    mutable QMutex nodesMutex;
    NodesList nodes;

    mutable QMutex graphEditedMutex;

    // If false the user cannot ever edit this graph from the UI, except if from Python the setSubGraphEditable function is called
    bool isEditable;

    // If true, the user did edit the subgraph
    bool wasGroupEditedByUser;


    NodeCollectionPrivate(const AppInstancePtr& app)
        : app(app)
        , graph(0)
        , nodesMutex()
        , nodes()
        , graphEditedMutex()
        , isEditable(true)
        , wasGroupEditedByUser(false)
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
    std::list<NodeGroupPtr> groupToRecurse;

    {
        QMutexLocker k(&_imp->nodesMutex);
        for (NodesList::const_iterator it = _imp->nodes.begin(); it != _imp->nodes.end(); ++it) {
            if ( onlyActive && !(*it)->isActivated() ) {
                continue;
            }
            nodes.push_back(*it);
            NodeGroupPtr isGrp = (*it)->isEffectNodeGroup();
            if (isGrp) {
                groupToRecurse.push_back(isGrp);
            }
        }
    }

    for (std::list<NodeGroupPtr>::const_iterator it = groupToRecurse.begin(); it != groupToRecurse.end(); ++it) {
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
    if (!node) {
        return;
    }
    QMutexLocker k(&_imp->nodesMutex);
    for (NodesList::iterator it =_imp->nodes.begin(); it != _imp->nodes.end();++it) {
        if ( it->get() == node ) {
            _imp->nodes.erase(it);
            break;
        }
    }
    onNodeRemoved(node);
}

void
NodeCollection::removeNode(const NodePtr& node)
{
    removeNode(node.get());
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
            NodeGroupPtr isGrp = (*it)->isEffectNodeGroup();
            if (isGrp) {
                isGrp->getActiveNodesExpandGroups(nodes);
            }
        }
    }
}

void
NodeCollection::getViewers(std::list<ViewerInstancePtr>* viewers) const
{
    QMutexLocker k(&_imp->nodesMutex);

    for (NodesList::iterator it = _imp->nodes.begin(); it != _imp->nodes.end(); ++it) {
        ViewerInstancePtr isViewer = (*it)->isEffectViewerInstance();
        if (isViewer) {
            viewers->push_back(isViewer);
        }
        NodeGroupPtr isGrp = (*it)->isEffectNodeGroup();
        if (isGrp) {
            isGrp->getViewers(viewers);
        }
    }
}

void
NodeCollection::getWriters(std::list<EffectInstancePtr>* writers) const
{
    QMutexLocker k(&_imp->nodesMutex);

    for (NodesList::iterator it = _imp->nodes.begin(); it != _imp->nodes.end(); ++it) {
        if ( (*it)->getGroup() && (*it)->isActivated() && (*it)->getEffectInstance()->isWriter() && (*it)->isPersistent() ) {
            writers->push_back((*it)->getEffectInstance());
        }
        NodeGroupPtr isGrp = (*it)->isEffectNodeGroup();
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
        NodeGroupPtr isGrp = (*it)->isEffectNodeGroup();

        if (isGrp) {
            isGrp->quitAnyProcessingInternal(blocking);
        }
        PrecompNodePtr isPrecomp = (*it)->isEffectPrecompNode();
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


void
NodeCollection::refreshViewersAndPreviews()
{
    assert( QThread::currentThread() == qApp->thread() );

    AppInstancePtr appInst = getApplication();
    if (!appInst || appInst->isBackground()) {
        return;
    }



    NodesList nodes = getNodes();
    for (NodesList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        assert(*it);
        (*it)->refreshPreviewsAfterProjectLoad();

        if ((*it)->isEffectViewerNode()) {
            (*it)->getRenderEngine()->renderCurrentFrame();
        }
        NodeGroupPtr isGrp = (*it)->isEffectNodeGroup();
        if (isGrp) {
            isGrp->refreshViewersAndPreviews();
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
    NodesList nodes;
    getActiveNodes(&nodes);
    for (NodesList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        if ( (*it)->isPreviewEnabled() ) {
            (*it)->refreshPreviewImage();
        }
        NodeGroupPtr isGrp = (*it)->isEffectNodeGroup();
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
    NodesList nodes;
    getActiveNodes(&nodes);
    for (NodesList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        if ( (*it)->isPreviewEnabled() ) {
            (*it)->computePreviewImage();
        }
        NodeGroupPtr isGrp = (*it)->isEffectNodeGroup();
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

        NodeGroupPtr isGrp = (*it)->isEffectNodeGroup();
        if (isGrp) {
            isGrp->clearNodesInternal(blocking);
        }
        PrecompNodePtr isPrecomp = (*it)->isEffectPrecompNode();
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
    }

    nodesToDelete.clear();
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
NodeCollection::checkNodeName(const NodeConstPtr& node,
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
            if ( (*it != node) && ( (*it)->getScriptName_mt_safe() == *nodeName ) ) {
                foundNodeWithName = true;
                break;
            }
        }
        if (foundNodeWithName) {
            if (errorIfExists || !appendDigit) {
                throw std::runtime_error( tr("A node with the script-name %1 already exists.").arg( QString::fromUtf8( nodeName->c_str() ) ).toStdString() );

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
} // NodeCollection::checkNodeName

void
NodeCollection::initNodeName(const std::string& pluginID,
                             const std::string& pluginLabel,
                             std::string* nodeName)
{
    std::string baseName(pluginLabel);

    if ( (baseName.size() > 3) &&
         ( baseName[baseName.size() - 1] == 'X') &&
         ( baseName[baseName.size() - 2] == 'F') &&
         ( baseName[baseName.size() - 3] == 'O') ) {
        baseName = baseName.substr(0, baseName.size() - 3);
    }

    bool isOutputNode = pluginID == PLUGINID_NATRON_OUTPUT;
    if (!isOutputNode) {
        // For non GroupOutput nodes, always append a digit
        checkNodeName(NodeConstPtr(), baseName, true, false, nodeName);
    } else {
        // For output node, don't append a digit as it is expect there's a single node
        try {
            checkNodeName(NodeConstPtr(), baseName, false, false, nodeName);
        } catch (...) {
            checkNodeName(NodeConstPtr(), baseName, true, false, nodeName);
        }
    }
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
        if ( input && (input->getMaxInputCount() > 0) ) {
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

    int inputsCount = input->getMaxInputCount();
    if (inputsCount == 1) {
        inputToReconnectTo = input->getInput(0);
    }


    if (!output->disconnectInput( input )) {
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
            std::map<NodePtr, int> outputsConnectedToSelectedNode;
            selected->getOutputsConnectedToThisNode(&outputsConnectedToSelectedNode);
            for (std::map<NodePtr, int>::iterator it = outputsConnectedToSelectedNode.begin();
                 it != outputsConnectedToSelectedNode.end(); ++it) {
                bool ok = disconnectNodes(selected, it->first);
                assert(ok);
                ok = connectNodes(it->second, created, it->first);
                Q_UNUSED(ok);
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
    return ret;
} // autoConnectNodes

NodePtr
NodeCollectionPrivate::findNodeInternal(const std::string& name,
                                        const std::string& recurseName) const
{
    QMutexLocker k(&nodesMutex);

    for (NodesList::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        if ( (*it)->getScriptName_mt_safe() == name ) {
            if ( !recurseName.empty() ) {
                NodeGroupPtr isGrp = (*it)->isEffectNodeGroup();
                if (isGrp) {
                    return isGrp->getNodeByFullySpecifiedName(recurseName);
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
            {
                ScopedChanges_RAII changes((*it)->getEffectInstance().get());


                const KnobsVec& knobs = (*it)->getKnobs();
                for (U32 j = 0; j < knobs.size(); ++j) {
                    KnobStringBasePtr isString = toKnobStringBase(knobs[j]);
                    KnobStringPtr isStringKnob = toKnobString(isString);
                    if ( !isString || isStringKnob || ( knobs[j] == project->getEnvVarKnob() ) ) {
                        continue;
                    }

                    std::string filepath = isString->getValue();

                    if ( !filepath.empty() ) {
                        if ( project->fixFilePath(projectPathName, newProjectPath, filepath) ) {
                            isString->setValue(filepath);
                        }
                    }
                }
            }
            
            NodeGroupPtr isGrp = (*it)->isEffectNodeGroup();
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
                KnobStringBasePtr isString = toKnobStringBase(knobs[j]);
                KnobStringPtr isStringKnob = toKnobString(isString);
                if ( !isString || isStringKnob || ( knobs[j] == project->getEnvVarKnob() ) ) {
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

            NodeGroupPtr isGrp = (*it)->isEffectNodeGroup();
            if (isGrp) {
                isGrp->fixPathName(oldName, newName);
            }
        }
    }
}

bool
NodeCollection::checkIfNodeLabelExists(const std::string & n,
                                       const NodeConstPtr& caller) const
{
    QMutexLocker k(&_imp->nodesMutex);

    for (NodesList::const_iterator it = _imp->nodes.begin(); it != _imp->nodes.end(); ++it) {
        if ( (*it != caller) && ( (*it)->getLabel_mt_safe() == n ) ) {
            return true;
        }
    }

    return false;
}

bool
NodeCollection::checkIfNodeNameExists(const std::string & n,
                                      const NodeConstPtr& caller) const
{
    QMutexLocker k(&_imp->nodesMutex);

    for (NodesList::const_iterator it = _imp->nodes.begin(); it != _imp->nodes.end(); ++it) {
        if ( (*it != caller) && ( (*it)->getScriptName_mt_safe() == n ) ) {
            return true;
        }
    }

    return false;
}

void
NodeCollection::recomputeFrameRangeForAllReadersInternal(int* firstFrame,
                                                         int* lastFrame,
                                                         bool setFrameRange)
{
    NodesList nodes = getNodes();

    for (NodesList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        if ( (*it)->isActivated() ) {
            if ( (*it)->getEffectInstance()->isReader() ) {


                GetFrameRangeResultsPtr results;
                ActionRetCodeEnum stat = (*it)->getEffectInstance()->getFrameRange_public(&results );
                if (!isFailureRetCode(stat)) {
                    RangeD thisRange;

                    results->getFrameRangeResults(&thisRange);
                    if (thisRange.min != INT_MIN) {
                        *firstFrame = setFrameRange ? thisRange.min : std::min(*firstFrame, (int)thisRange.min);
                    }
                    if (thisRange.max != INT_MAX) {
                        *lastFrame = setFrameRange ? thisRange.max : std::max(*lastFrame, (int)thisRange.max);
                    }
                }

            } else {
                NodeGroupPtr isGrp = (*it)->isEffectNodeGroup();
                if (isGrp) {
                    isGrp->recomputeFrameRangeForAllReadersInternal(firstFrame, lastFrame, false);
                }
            }
        }
    }
}

void
NodeCollection::recomputeFrameRangeForAllReaders(int* firstFrame,
                                                 int* lastFrame)
{
    recomputeFrameRangeForAllReadersInternal(firstFrame, lastFrame, true);
}


void
NodeCollection::setSubGraphEditedByUser(bool edited)
{
    {
        QMutexLocker k(&_imp->graphEditedMutex);
        _imp->wasGroupEditedByUser = edited;
    }

    // When set edited make sure all knobs have the appropriate "declared by plug-in" flag
    NodeGroup* isGrp = dynamic_cast<NodeGroup*>(this);
    if (isGrp) {

        if (isGrp->isSubGraphPersistent()) {
            KnobIPtr pyPlugPage = isGrp->getNode()->getKnobByName(kPyPlugPageParamName);
            if (pyPlugPage) {
                pyPlugPage->setSecret(!edited);
            }

            KnobIPtr convertToGroupKnob = isGrp->getNode()->getKnobByName(kNatronNodeKnobConvertToGroupButton);
            if (convertToGroupKnob) {
                convertToGroupKnob->setSecret(edited || isSubGraphEditable());
            }
        }

        const KnobsVec& knobs = isGrp->getKnobs();
        for (KnobsVec::const_iterator it = knobs.begin(); it!=knobs.end(); ++it) {
            if ((*it)->isUserKnob()) {
                (*it)->setDeclaredByPlugin(!edited);
            }
        }

    }
}

bool
NodeCollection::isSubGraphEditedByUser() const
{
    QMutexLocker k(&_imp->graphEditedMutex);
    return _imp->wasGroupEditedByUser;
}

void
NodeCollection::setSubGraphEditable(bool editable)
{
    {
        QMutexLocker k(&_imp->graphEditedMutex);
        _imp->isEditable = editable;
    }
    onGraphEditableChanged(editable);
}

bool
NodeCollection::isSubGraphEditable() const
{
    QMutexLocker k(&_imp->graphEditedMutex);
    return _imp->isEditable;
}

static void refreshTimeInvariantMetadataOnAllNodes_recursiveInternal(const NodePtr& caller, std::set<NodePtr>* markedNodes)
{
    if (markedNodes->find(caller) != markedNodes->end()) {
        return;
    }

    markedNodes->insert(caller);

    NodeGroupPtr isGroup = caller->isEffectNodeGroup();
    if (isGroup) {
        NodesList nodes = isGroup->getNodes();
        for (NodesList::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
            refreshTimeInvariantMetadataOnAllNodes_recursiveInternal(*it, markedNodes);
        }
    } else {

        caller->getEffectInstance()->onMetadataChanged_nonRecursive_public();
    }

}

void
NodeCollection::refreshTimeInvariantMetadataOnAllNodes_recursive()
{
    std::set<NodePtr> markedNodes;
    NodesList nodes = getNodes();
    for (NodesList::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        refreshTimeInvariantMetadataOnAllNodes_recursiveInternal(*it, &markedNodes);
    }
}

struct NodeGroupPrivate
{
    mutable QMutex nodesLock; // protects inputs & outputs
    std::vector<NodeWPtr > inputs;
    NodesWList outputs;
    bool isDeactivatingGroup;
    bool isActivatingGroup;
    NodeGroupPrivate()
        : nodesLock(QMutex::Recursive)
        , inputs()
        , outputs()
        , isDeactivatingGroup(false)
        , isActivatingGroup(false)

    {
    }
};

void
NodeGroup::onNodeRemoved(const Node* node)
{
    for (std::vector<NodeWPtr >::iterator it = _imp->inputs.begin(); it != _imp->inputs.end(); ++it) {
        if (it->lock().get() == node) {
            _imp->inputs.erase(it);
            break;
        }
    }
    for (NodesWList::iterator it = _imp->outputs.begin(); it != _imp->outputs.end(); ++it) {
        if (it->lock().get() == node) {
            _imp->outputs.erase(it);
            break;
        }
    }
}

PluginPtr
NodeGroup::createPlugin()
{
    std::vector<std::string> grouping;
    grouping.push_back(PLUGIN_GROUP_OTHER);
    PluginPtr ret = Plugin::create((void*)NodeGroup::create, (void*)NodeGroup::createRenderClone, PLUGINID_NATRON_GROUP, "Group", 1, 0, grouping);

    QString desc =  tr("Use this to nest multiple nodes into a single node. The original nodes will be replaced by the Group node and its "
                       "content is available in a separate NodeGraph tab. You can add user parameters to the Group node which can drive "
                       "parameters of nodes nested within the Group. To specify the outputs and inputs of the Group node, "
                       "you may add multiple Input node within the group and exactly 1 Output node.");
    ret->setProperty<std::string>(kNatronPluginPropDescription, desc.toStdString());
    ret->setProperty<int>(kNatronPluginPropRenderSafety, (int)eRenderSafetyFullySafe);
    ret->setProperty<std::string>(kNatronPluginPropIconFilePath, "Images/group_icon.png");

    return ret;
}


NodeGroup::NodeGroup(const NodePtr &node)
    : EffectInstance(node)
    , NodeCollection( node ? node->getApp() : AppInstancePtr() )
    , _imp( new NodeGroupPrivate() )
{
}

NodeGroup::NodeGroup(const EffectInstancePtr& mainInstance, const FrameViewRenderKey& key)
: EffectInstance(mainInstance, key)
, NodeCollection(mainInstance->getApp())
, _imp(new NodeGroupPrivate())
{
    // Groups are never part of a render tree!
    assert(false);
    throw std::logic_error("A group cannot have a render clone");
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


void
NodeGroup::addAcceptedComponents(int /*inputNb*/,                                 std::bitset<4>* supported)
{
    (*supported)[0] = (*supported)[1] = (*supported)[2] = (*supported)[3] = 1;
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
        if ( ( inputNb >= (int)_imp->inputs.size() ) || (inputNb < 0) ) {
            return std::string();
        }

        ///If the input name starts with "input" remove it, otherwise keep the full name

        input = _imp->inputs[inputNb].lock();
        if (!input) {
            return std::string();
        }
    }
    QString inputName = QString::fromUtf8( input->getLabel_mt_safe().c_str() );

    if ( inputName.startsWith(QString::fromUtf8("input"), Qt::CaseInsensitive) ) {
        inputName.remove(0, 5);
    }

    return inputName.toStdString();
}

TimeValue
NodeGroup::getCurrentRenderTime() const
{
    NodePtr node = getOutputNodeInput();

    if (node) {
        EffectInstancePtr effect = node->getEffectInstance();
        if (!effect) {
            return TimeValue(0);
        }
        return effect->getCurrentRenderTime();
    }

    return EffectInstance::getCurrentRenderTime();
}

ViewIdx
NodeGroup::getCurrentRenderView() const
{
    NodePtr node = getOutputNodeInput();

    if (node) {
        EffectInstancePtr effect = node->getEffectInstance();
        if (!effect) {
            return ViewIdx(0);
        }
        return effect->getCurrentRenderView();
    }

    return EffectInstance::getCurrentRenderView();
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
    GroupInputPtr input = n->isEffectGroupInput();

    if (!input) {
        return false;
    }
    KnobIPtr knob = input->getKnobByName(kNatronGroupInputIsOptionalParamName);
    assert(knob);
    if (!knob) {
        return false;
    }
    KnobBoolPtr isBool = toKnobBool(knob);
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
    GroupInputPtr input = n->isEffectGroupInput();

    if (!input) {
        return false;
    }
    KnobIPtr knob = input->getKnobByName(kNatronGroupInputIsMaskParamName);
    assert(knob);
    if (!knob) {
        return false;
    }
    KnobBoolPtr isBool = toKnobBool(knob);
    assert(isBool);

    return isBool ? isBool->getValue() : false;
}


void
NodeGroup::notifyNodeDeactivated(const NodePtr& node)
{
    if ( getIsDeactivatingGroup() ) {
        return;
    }
    NodePtr thisNode = getNode();

    {
        QMutexLocker k(&_imp->nodesLock);
        GroupInputPtr isInput = node->isEffectGroupInput();
        if (isInput) {
            int i = 0;
            for (std::vector<NodeWPtr >::iterator it = _imp->inputs.begin(); it != _imp->inputs.end(); ++it, ++i) {
                NodePtr input = it->lock();
                if (node == input) {
                    ///Also disconnect the real input

                    thisNode->disconnectInput(i);

                    _imp->inputs.erase(it);
                    thisNode->initializeInputs();

                    break;
                }
            }
        
        }
        GroupOutputPtr isOutput = toGroupOutput( node->getEffectInstance() );
        if (isOutput) {
            for (NodesWList::iterator it = _imp->outputs.begin(); it != _imp->outputs.end(); ++it) {
                if (it->lock()->getEffectInstance() == isOutput) {
                    _imp->outputs.erase(it);
                    break;
                }
            }
        }

    }

    ///Notify outputs of the group nodes that their inputs may have changed
    const NodesWList& outputs = thisNode->getOutputs();
    for (NodesWList::const_iterator it = outputs.begin(); it != outputs.end(); ++it) {
        NodePtr output = it->lock();
        if (!output) {
            continue;
        }
        int idx = output->getInputIndex(thisNode);
        assert(idx != -1);
        output->onInputChanged(idx);
    }
} // NodeGroup::notifyNodeDeactivated

void
NodeGroup::notifyNodeActivated(const NodePtr& node)
{
    if ( getIsActivatingGroup() ) {
        return;
    }

    NodePtr thisNode = getNode();

    {
        QMutexLocker k(&_imp->nodesLock);
        GroupInputPtr isInput = node->isEffectGroupInput();
        if (isInput) {
            _imp->inputs.push_back(node);
            thisNode->initializeInputs();
        }
        GroupOutputPtr isOutput = toGroupOutput( node->getEffectInstance() );
        if (isOutput) {
            _imp->outputs.push_back(node);
        }
    }
    ///Notify outputs of the group nodes that their inputs may have changed
    const NodesWList& outputs = thisNode->getOutputs();
    for (NodesWList::const_iterator it = outputs.begin(); it != outputs.end(); ++it) {
        NodePtr output = it->lock();
        if (!output) {
            continue;
        }
        int idx = output->getInputIndex(thisNode);
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
NodeGroup::notifyNodeLabelChanged(const NodePtr& node)
{
    GroupInputPtr isInput = node->isEffectGroupInput();

    if (isInput) {
        getNode()->initializeInputs();
    }
}

NodePtr
NodeGroup::getOutputNode() const
{
    QMutexLocker k(&_imp->nodesLock);

    ///A group can only have a single output.
    if (_imp->outputs.empty()) {
        return NodePtr();
    }

    return _imp->outputs.front().lock();
}

NodePtr
NodeGroup::getOutputNodeInput() const
{
    NodePtr output = getOutputNode();

    if (output) {
        return output->getInput(0);
    }

    return NodePtr();
}

NodePtr
NodeGroup::getRealInputForInput(const NodePtr& input) const
{
    {
        QMutexLocker k(&_imp->nodesLock);
        for (U32 i = 0; i < _imp->inputs.size(); ++i) {
            if (_imp->inputs[i].lock() == input) {
                NodePtr node = getNode();
                if (!node) {
                    return NodePtr();
                }
                return node->getInput(i);
            }
        }
    }

    return NodePtr();
}

void
NodeGroup::getInputsOutputs(NodesList* nodes) const
{
    QMutexLocker k(&_imp->nodesLock);

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
}

void
NodeGroup::getInputs(std::vector<NodePtr >* inputs) const
{
    QMutexLocker k(&_imp->nodesLock);

    for (U32 i = 0; i < _imp->inputs.size(); ++i) {
        NodePtr input = _imp->inputs[i].lock();
        if (!input) {
            continue;
        }
        inputs->push_back(input);
    }

}

void
NodeGroup::purgeCaches()
{
    NodesList nodes = getNodes();

    for (NodesList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        (*it)->getEffectInstance()->purgeCaches_public();
    }
}

void
NodeGroup::clearLastRenderedImage()
{
    EffectInstance::clearLastRenderedImage();
    NodesList nodes = getNodes();

    for (NodesList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        (*it)->getEffectInstance()->purgeCaches_public();
    }
}

void
NodeGroup::setupInitialSubGraphState()
{
    if (!isSubGraphEditable() || !isSubGraphPersistent()) {
        return;
    }

    setSubGraphEditedByUser(true);

    NodePtr input, output;

    NodeGroupPtr thisShared = toNodeGroup(shared_from_this());
    {
        CreateNodeArgsPtr args(CreateNodeArgs::create(PLUGINID_NATRON_OUTPUT, thisShared));
        args->setProperty(kCreateNodeArgsPropAutoConnect, false);
        args->setProperty(kCreateNodeArgsPropAddUndoRedoCommand, false);
        args->setProperty(kCreateNodeArgsPropSettingsOpened, false);
        output = getApp()->createNode(args);
        assert(output);
        if (!output) {
            throw std::runtime_error( tr("NodeGroup cannot create node %1").arg( QLatin1String(PLUGINID_NATRON_OUTPUT) ).toStdString() );
        }
    }
    {
        CreateNodeArgsPtr args(CreateNodeArgs::create(PLUGINID_NATRON_INPUT, thisShared));
        args->setProperty(kCreateNodeArgsPropAutoConnect, false);
        args->setProperty(kCreateNodeArgsPropAddUndoRedoCommand, false);
        args->setProperty(kCreateNodeArgsPropSettingsOpened, false);
        input = getApp()->createNode(args);
        assert(input);
        if (!input) {
            throw std::runtime_error( tr("NodeGroup cannot create node %1").arg( QLatin1String(PLUGINID_NATRON_INPUT) ).toStdString() );
        }
    }
    if ( input && output && !output->getInput(0) ) {
        output->connectInput(input, 0);

        double x, y;
        output->getPosition(&x, &y);
        y -= 100;
        input->setPosition(x, y);
    }
}

void
NodeGroup::loadSubGraph(const SERIALIZATION_NAMESPACE::NodeSerialization* projectSerialization,
                        const SERIALIZATION_NAMESPACE::NodeSerialization* pyPlugSerialization)
{
    if (getNode()->isPyPlug()) {

        assert(pyPlugSerialization);
        // This will create internal nodes and restore their links.
        Project::restoreGroupFromSerialization(pyPlugSerialization->_children, toNodeGroup(shared_from_this()), 0);

        // For PyPlugs, the graph s not editable anyway
        setSubGraphEditedByUser(false);


    } else if (projectSerialization && isSubGraphPersistent()) {

        // Ok if we are here that means we are loading a group that was edited.
        // Clear any initial nodes created and load the sub-graph

        //Clear any node created already in setupInitialSubGraphState()
        clearNodesBlocking();
        
        // This will create internal nodes.
        Project::restoreGroupFromSerialization(projectSerialization->_children, toNodeGroup(shared_from_this()),  0);

        // A group always appear edited
        setSubGraphEditedByUser(true);
    } else {
        // A group always appear edited
        setSubGraphEditedByUser(true);
    }
}


NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_NodeGroup.cpp"
