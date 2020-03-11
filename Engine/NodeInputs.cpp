/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
 * Copyright (C) 2018-2020 The Natron developers
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

#include "NodePrivate.h"

#include <QDebug>

#include "Engine/GroupOutput.h"
#include "Engine/InputDescription.h"
#include "Engine/PrecompNode.h"
#include "Engine/Settings.h"

NATRON_NAMESPACE_ENTER


/**
 * @brief Resolves links of the graph in the case of containers (that do not do any rendering but only contain nodes inside)
 * so that algorithms that cycle the tree from bottom to top
 * properly visit all nodes in the correct order
 **/
static NodePtr
applyNodeRedirectionsUpstream(const NodePtr& node)
{
    if (!node) {
        return node;
    }
    NodeGroupPtr isGrp = node->isEffectNodeGroup();
    if (isGrp) {
        //The node is a group, instead jump directly to the output node input of the  group
        return applyNodeRedirectionsUpstream(isGrp->getOutputNodeInput());
    }


    GroupInputPtr isInput = node->isEffectGroupInput();
    if (isInput) {
        //The node is a group input,  jump to the corresponding input of the group
        NodeCollectionPtr collection = node->getGroup();
        assert(collection);
        isGrp = toNodeGroup(collection);
        if (isGrp) {
            return applyNodeRedirectionsUpstream(isGrp->getRealInputForInput(node));
        }
    }

    return node;
}

/**
 * @brief Resolves links of the graph in the case of containers (that do not do any rendering but only contain nodes inside)
 * so that algorithms that cycle the tree from top to bottom
 * properly visit all nodes in the correct order. Note that one node may translate to several nodes since multiple nodes
 * may be connected to the same node.
 **/
static void
applyNodeRedirectionsDownstream(int recurseCounter,
                                const NodePtr& node,
                                NodesList& translated)
{
    NodeGroupPtr isGrp = node->isEffectNodeGroup();

    if (isGrp) {
        //The node is a group, meaning it should not be taken into account, instead jump directly to the input nodes output of the group
        NodesList inputNodes;
        isGrp->getInputsOutputs(&inputNodes);
        for (NodesList::iterator it2 = inputNodes.begin(); it2 != inputNodes.end(); ++it2) {
            //Call recursively on them
            applyNodeRedirectionsDownstream(recurseCounter + 1, *it2, translated);
        }

        return;
    }

    GroupOutputPtr isOutput = node->isEffectGroupOutput();
    if (isOutput) {
        //The node is the output of a group, its outputs are the outputs of the group
        NodeCollectionPtr collection = isOutput->getNode()->getGroup();
        if (!collection) {
            return;
        }
        isGrp = toNodeGroup(collection);
        if (isGrp) {
            OutputNodesMap groupOutputs;

            NodePtr grpNode = isGrp->getNode();
            if (grpNode) {
                grpNode->getOutputs(groupOutputs);
            }

            for (OutputNodesMap::iterator it2 = groupOutputs.begin(); it2 != groupOutputs.end(); ++it2) {
                //Call recursively on them
                applyNodeRedirectionsDownstream(recurseCounter + 1, it2->first, translated);
            }
        }

        return;
    }

    //Base case: return this node
    if (recurseCounter > 0) {
        translated.push_back(node);
    }
} // applyNodeRedirectionsDownstream

void
Node::getOutputsWithGroupRedirection(NodesList& outputs) const
{
#pragma message WARN("This function is confusing, we should probably re-think it")
    NodesList redirections;
    NodePtr thisShared = boost::const_pointer_cast<Node>( shared_from_this() );

    applyNodeRedirectionsDownstream(0, thisShared, redirections);
    if ( !redirections.empty() ) {
        outputs.insert( outputs.begin(), redirections.begin(), redirections.end() );
    } else {
        QMutexLocker l(&_imp->outputsMutex);
        for (InternalOutputNodesMap::const_iterator it = _imp->outputs.begin(); it != _imp->outputs.end(); ++it) {
            NodePtr output = it->first.lock();
            if (output) {
                outputs.push_back(output);
            }
        }
    }
}


NodePtr
Node::getInput(int index) const
{
    if (!_imp->effect) {
        return NodePtr();
    }

    return getInputInternal(true, index);
}

NodePtr
Node::getInputInternal(bool useGroupRedirections,
                       int index) const
{
    QMutexLocker l(&_imp->inputsMutex);
    if ( ( index >= (int)_imp->inputs.size() ) || (index < 0) ) {
        return NodePtr();
    }

    NodePtr ret =  _imp->inputs[index].lock();
    if (ret && useGroupRedirections) {
        ret = applyNodeRedirectionsUpstream(ret);
    }

    return ret;
}

NodePtr
Node::getRealInput(int index) const
{
    return getInputInternal(false, index);
}

std::list<int>
Node::getInputIndicesConnectedToThisNode(const NodeConstPtr& outputNode) const
{
    std::list<int> ret;
    QMutexLocker l(&_imp->outputsMutex);
    NodePtr tmp = boost::const_pointer_cast<Node>(outputNode);
    InternalOutputNodesMap::const_iterator found = _imp->outputs.find(tmp);
    if (found != _imp->outputs.end()) {
        ret = found->second;
        return ret;
    }
    return ret;
}

const std::vector<NodeWPtr> &
Node::getInputs() const
{
    ////Only called by the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    return _imp->inputs;
}


std::vector<NodeWPtr>
Node::getInputs_copy() const
{
    QMutexLocker l(&_imp->inputsMutex);

    return _imp->inputs;
}

std::string
Node::getInputLabel(int inputNb) const
{
    QMutexLocker l(&_imp->inputsMutex);
    if ( (inputNb < 0) || ( inputNb >= (int)_imp->inputDescriptions.size() ) ) {
        throw std::invalid_argument("Index out of range");
    }

    return _imp->inputDescriptions[inputNb]->getPropertyUnsafe<std::string>(kInputDescPropLabel);
}

std::string
Node::getInputHint(int inputNb) const
{

    QMutexLocker l(&_imp->inputsMutex);
    if ( (inputNb < 0) || ( inputNb >= (int)_imp->inputDescriptions.size() ) ) {
        throw std::invalid_argument("Index out of range");
    }

    return _imp->inputDescriptions[inputNb]->getPropertyUnsafe<std::string>(kInputDescPropHint);
}

void
Node::setInputLabel(int inputNb, const std::string& label)
{
    {
        QMutexLocker l(&_imp->inputsMutex);
        if ( (inputNb < 0) || ( inputNb >= (int)_imp->inputDescriptions.size() ) ) {
            throw std::invalid_argument("Index out of range");
        }
        _imp->inputDescriptions[inputNb]->setProperty(kInputDescPropLabel, label);
    }
    _imp->effect->onInputLabelChanged(inputNb, label);

    Q_EMIT inputEdgeLabelChanged(inputNb, QString::fromUtf8(label.c_str()));
}

void
Node::setInputHint(int inputNb, const std::string& hint)
{
    {
        QMutexLocker l(&_imp->inputsMutex);
        if ( (inputNb < 0) || ( inputNb >= (int)_imp->inputDescriptions.size() ) ) {
            throw std::invalid_argument("Index out of range");
        }
        _imp->inputDescriptions[inputNb]->setProperty(kInputDescPropHint, hint);
    }
}

bool
Node::isInputVisible(int inputNb) const
{
    QMutexLocker l(&_imp->inputsMutex);
    if ( (inputNb < 0) || ( inputNb >= (int)_imp->inputDescriptions.size() ) ) {
        return false;
    }

    return _imp->inputDescriptions[inputNb]->getPropertyUnsafe<bool>(kInputDescPropIsVisible);
}

void
Node::setInputVisible(int inputNb, bool visible)
{
    {
        QMutexLocker l(&_imp->inputsMutex);
        if ( (inputNb < 0) || ( inputNb >= (int)_imp->inputDescriptions.size() ) ) {
            throw std::invalid_argument("Index out of range");
        }
        _imp->inputDescriptions[inputNb]->setProperty(kInputDescPropIsVisible, visible);
    }
    Q_EMIT inputVisibilityChanged(inputNb);
}

ImageFieldExtractionEnum
Node::getInputFieldExtraction(int inputNb) const
{
    QMutexLocker l(&_imp->inputsMutex);
    if ( (inputNb < 0) || ( inputNb >= (int)_imp->inputDescriptions.size() ) ) {
        return eImageFieldExtractionDouble;
    }

    return _imp->inputDescriptions[inputNb]->getPropertyUnsafe<ImageFieldExtractionEnum>(kInputDescPropFieldingSupport);
}

std::bitset<4>
Node::getSupportedComponents(int inputNb) const
{

    // For a Read or Write node, actually return the bits from the embedded plug-in
    ReadNodePtr isRead = toReadNode(_imp->effect);
    WriteNodePtr isWrite = toWriteNode(_imp->effect);
    NodePtr embeddedNode;
    if (isRead) {
        embeddedNode = isRead->getEmbeddedReader();
    } else if (isWrite) {
        embeddedNode = isWrite->getEmbeddedWriter();
    }

    if (embeddedNode) {
        return embeddedNode->getSupportedComponents(inputNb);
    }

    if (inputNb == -1) {
        PluginPtr plugin = getPlugin();
        return plugin->getPropertyUnsafe<std::bitset<4> >(kNatronPluginPropOutputSupportedComponents);
    } else {
        QMutexLocker l(&_imp->inputsMutex);
        if ( (inputNb < 0) || ( inputNb >= (int)_imp->inputDescriptions.size() ) ) {
            return std::bitset<4>();
        }

        return _imp->inputDescriptions[inputNb]->getPropertyUnsafe<std::bitset<4> >(kInputDescPropSupportedComponents);
    }
}

bool
Node::isInputHostDescribed(int inputNb) const
{
    QMutexLocker l(&_imp->inputsMutex);
    if ( (inputNb < 0) || ( inputNb >= (int)_imp->inputDescriptions.size() ) ) {
        return false;
    }

    return _imp->inputDescriptions[inputNb]->getPropertyUnsafe<bool>(kInputDescPropIsHostInput);
}

bool
Node::isInputMask(int inputNb) const
{
    QMutexLocker l(&_imp->inputsMutex);
    if ( (inputNb < 0) || ( inputNb >= (int)_imp->inputDescriptions.size() ) ) {
        return false;
    }

    return _imp->inputDescriptions[inputNb]->getPropertyUnsafe<bool>(kInputDescPropIsMask);
}

bool
Node::isInputOptional(int inputNb) const
{
    QMutexLocker l(&_imp->inputsMutex);
    if ( (inputNb < 0) || ( inputNb >= (int)_imp->inputDescriptions.size() ) ) {
        return false;
    }

    return _imp->inputDescriptions[inputNb]->getPropertyUnsafe<bool>(kInputDescPropIsOptional);
}

bool
Node::isTilesSupportedByInput(int inputNb) const
{
    QMutexLocker l(&_imp->inputsMutex);
    if ( (inputNb < 0) || ( inputNb >= (int)_imp->inputDescriptions.size() ) ) {
        return true;
    }

    return _imp->inputDescriptions[inputNb]->getPropertyUnsafe<bool>(kInputDescPropSupportsTiles);
}

bool
Node::isTemporalAccessSupportedByInput(int inputNb) const
{
    QMutexLocker l(&_imp->inputsMutex);
    if ( (inputNb < 0) || ( inputNb >= (int)_imp->inputDescriptions.size() ) ) {
        return false;
    }

    return _imp->inputDescriptions[inputNb]->getPropertyUnsafe<bool>(kInputDescPropSupportsTemporal);
}

bool
Node::canInputReceiveDistortion(int inputNb) const
{
    QMutexLocker l(&_imp->inputsMutex);
    if ( (inputNb < 0) || ( inputNb >= (int)_imp->inputDescriptions.size() ) ) {
        return false;
    }

    return _imp->inputDescriptions[inputNb]->getPropertyUnsafe<bool>(kInputDescPropCanReceiveDistortion);
}

bool
Node::canInputReceiveTransform3x3(int inputNb) const
{
    QMutexLocker l(&_imp->inputsMutex);
    if ( (inputNb < 0) || ( inputNb >= (int)_imp->inputDescriptions.size() ) ) {
        return false;
    }

    return _imp->inputDescriptions[inputNb]->getPropertyUnsafe<bool>(kInputDescPropCanReceiveTransform3x3);
}

bool
Node::getInputDescription(int inputNb, InputDescription* desc) const
{
    QMutexLocker l(&_imp->inputsMutex);
    if ( (inputNb < 0) || ( inputNb >= (int)_imp->inputDescriptions.size() ) ) {
        return false;
    }
    desc->cloneProperties(*_imp->inputDescriptions[inputNb]);
    return true;
}

int
Node::getNInputs() const
{
    QMutexLocker l(&_imp->inputsMutex);
    return _imp->inputs.size();
}


int
Node::getInputNumberFromLabel(const std::string& inputLabel) const
{
    if (inputLabel.empty()) {
        return -1;
    }
    QMutexLocker l(&_imp->inputsMutex);
    for (U32 i = 0; i < _imp->inputDescriptions.size(); ++i) {
        if (_imp->inputDescriptions[i]->getPropertyUnsafe<std::string>(kInputDescPropLabel) == inputLabel) {
            return i;
        }
    }

    return -1;
}

void
Node::addInput(const InputDescriptionPtr& description)
{
    insertInput(-1, description);
}

void
Node::insertInput(int index, const InputDescriptionPtr& description)
{
    beginInputEdition();
    {
        QMutexLocker k(&_imp->inputsMutex);
        _imp->inputIsRenderingCounter.resize(_imp->inputIsRenderingCounter.size() + 1);
        if (index >= (int)_imp->inputs.size() || index == -1) {
            _imp->inputs.push_back(NodePtr());
            _imp->inputDescriptions.push_back(description);
        } else {
            {
                InputsV::iterator it = _imp->inputs.begin();
                std::advance(it, index);
                _imp->inputs.insert(it, NodePtr());
            }
            {
                std::vector<InputDescriptionPtr>::iterator it = _imp->inputDescriptions.begin();
                std::advance(it, index);
                _imp->inputDescriptions.insert(it, description);
            }
        }
    }
    ++_imp->hasModifiedInputsDescription;
    endInputEdition(true);


}

void
Node::changeInputDescription(int inputNb, const InputDescriptionPtr& description)
{
    beginInputEdition();
    {
        QMutexLocker k(&_imp->inputsMutex);
        if (inputNb < 0 || inputNb >= (int)_imp->inputs.size()) {
            return;
        }
        _imp->inputDescriptions[inputNb] = description;
    }
    _imp->inputsModified.insert(inputNb);
    ++_imp->hasModifiedInputsDescription;
    endInputEdition(true);

}

void
Node::removeInput(int inputNb)
{
    beginInputEdition();
    {
        QMutexLocker k(&_imp->inputsMutex);
        if (inputNb < 0 || inputNb >= (int)_imp->inputs.size()) {
            return;
        }
        _imp->inputIsRenderingCounter.resize(_imp->inputIsRenderingCounter.size() - 1);
        {
            InputsV::iterator it = _imp->inputs.begin();
            std::advance(it, inputNb);
            _imp->inputs.erase(it);
        }
        {
            std::vector<InputDescriptionPtr>::iterator it = _imp->inputDescriptions.begin();
            std::advance(it, inputNb);
            _imp->inputDescriptions.erase(it);
        }
    }
    ++_imp->hasModifiedInputsDescription;
    endInputEdition(true);
}

void
Node::removeAllInputs()
{
    beginInputEdition();
    {
        QMutexLocker k(&_imp->inputsMutex);
        _imp->inputs.clear();
        _imp->inputDescriptions.clear();
        _imp->inputIsRenderingCounter.clear();
    }
    ++_imp->hasModifiedInputsDescription;
    endInputEdition(true);
}

bool
Node::isInputConnected(int inputNb) const
{
    return getInput(inputNb) != NULL;
}

bool
Node::hasInputConnected() const
{
    QMutexLocker l(&_imp->inputsMutex);
    for (U32 i = 0; i < _imp->inputs.size(); ++i) {
        if ( _imp->inputs[i].lock() ) {
            return true;
        }
    }


    return false;
}

bool
Node::hasMandatoryInputDisconnected() const
{
    QMutexLocker l(&_imp->inputsMutex);

    assert(_imp->inputs.size() == _imp->inputDescriptions.size());
    for (U32 i = 0; i < _imp->inputs.size(); ++i) {
        if ( !_imp->inputs[i].lock() && !_imp->inputDescriptions[i]->getPropertyUnsafe<bool>(kInputDescPropIsOptional) ) {
            return true;
        }
    }

    return false;
}

bool
Node::hasAllInputsConnected() const
{
    QMutexLocker l(&_imp->inputsMutex);

    for (U32 i = 0; i < _imp->inputs.size(); ++i) {
        if ( !_imp->inputs[i].lock() ) {
            return false;
        }
    }

    return true;
}

bool
Node::hasOutputConnected() const
{
    ////Only called by the main-thread

    if ( isOutputNode() ) {
        return true;
    }
    if ( QThread::currentThread() == qApp->thread() ) {
        if (_imp->outputs.size() > 0) {
            return true;
        }
    } else {
        QMutexLocker l(&_imp->outputsMutex);
        if (_imp->outputs.size() > 0) {
            return true;
        }
    }

    return false;
}

bool
Node::checkIfConnectingInputIsOk(const NodePtr& input) const
{
    ////Only called by the main-thread
    if (!input || input.get() == this) {
        return false;
    }
    bool found = input->isNodeUpstream(shared_from_this());
    return !found;
}

bool
Node::isNodeUpstreamInternal(const NodeConstPtr& input, std::set<NodeConstPtr>& markedNodes) const
{
    if (!input) {
        return false;
    }

    NodeConstPtr thisShared = shared_from_this();
    std::pair<std::set<NodeConstPtr>::iterator, bool> insertOk = markedNodes.insert(thisShared);
    if (!insertOk.second) {
        return false;
    }


    std::vector<NodeWPtr> inputs = getInputs_copy();
    for (std::size_t i = 0; i  < inputs.size(); ++i) {
        if (inputs[i].lock() == input) {
            return true;
        }
    }

    for (std::size_t i = 0; i  < inputs.size(); ++i) {
        NodePtr in = inputs[i].lock();
        if (in) {
            if (in->isNodeUpstreamInternal(input, markedNodes)) {
                return true;
            }
        }
    }
    return false;
}

bool
Node::isNodeUpstream(const NodeConstPtr& input) const
{
    std::set<NodeConstPtr> markedNodes;
    return isNodeUpstreamInternal(input, markedNodes);
}



Node::CanConnectInputReturnValue
Node::canConnectInput(const NodePtr& input,
                      int inputNumber) const
{
    ///No-one is allowed to connect to the other node
    if ( !input || !input->canOthersConnectToThisNode() ) {
        return eCanConnectInput_givenNodeNotConnectable;
    }

    ///Check for invalid index
    {
        QMutexLocker l(&_imp->inputsMutex);
        if ( (inputNumber < 0) || ( inputNumber >= (int)_imp->inputs.size() ) ) {
            return eCanConnectInput_indexOutOfRange;
        }
        if ( _imp->inputs[inputNumber].lock() ) {
            return eCanConnectInput_inputAlreadyConnected;
        }
    }

    NodeGroupPtr isGrp = input->isEffectNodeGroup();
    if ( isGrp && !isGrp->getOutputNode() ) {
        return eCanConnectInput_groupHasNoOutput;
    }


    ///Applying this connection would create cycles in the graph
    if ( !checkIfConnectingInputIsOk( input ) ) {
        return eCanConnectInput_graphCycles;
    }

    {
        ///Check for invalid pixel aspect ratio if the node doesn't support multiple clip PARs

        double inputPAR = input->getEffectInstance()->getAspectRatio(-1);
        double inputFPS = input->getEffectInstance()->getFrameRate();
        QMutexLocker l(&_imp->inputsMutex);

        for (InputsV::const_iterator it = _imp->inputs.begin(); it != _imp->inputs.end(); ++it) {
            NodePtr node = it->lock();
            if (node) {
                if ( !_imp->effect->isMultipleInputsWithDifferentPARSupported() ) {
                    if (node->getEffectInstance()->getAspectRatio(-1) != inputPAR) {
                        return eCanConnectInput_differentPars;
                    }
                }

                if (std::abs(node->getEffectInstance()->getFrameRate() - inputFPS) > 0.01) {
                    return eCanConnectInput_differentFPS;
                }
            }
        }
    }

    return eCanConnectInput_ok;
} // Node::canConnectInput

bool
Node::canOthersConnectToThisNode() const
{
    if (isBeingDestroyed()) {
        return false;
    }
    if ( isEffectBackdrop() ) {
        return false;
    } else if ( isEffectGroupOutput() ) {
        return false;
    } else if ( _imp->effect->isWriter() && (_imp->effect->getSequentialRenderSupport() == eSequentialPreferenceOnlySequential) ) {
        return false;
    }
    ///In debug mode only allow connections to Writer nodes
# ifdef DEBUG
    return !isEffectViewerInstance();
# else // !DEBUG
    return !isEffectViewerInstance() /* && !_imp->effect->isWriter()*/;
# endif // !DEBUG
}


bool
Node::connectInput(const NodePtr & input, int inputNumber)
{

    if (!input) {
        return false;
    }


    return replaceInputInternal(input, inputNumber, true);
} // Node::connectInput


bool
Node::replaceInputInternal(const NodePtr& input, int inputNumber, bool failIfExisting)
{
    // Check for cycles: they are forbidden in the graph
    if ( input && !checkIfConnectingInputIsOk( input ) ) {
        return false;
    }

    if (isBeingDestroyed()) {
        return false;
    }

    {
        ///Check for invalid index
        QMutexLocker l(&_imp->inputsMutex);
        if ( (inputNumber < 0) || ( inputNumber >= (int)_imp->inputs.size() ) ) {
            return false;
        }
    }


    bool destroyed;
    {
        QMutexLocker k(&_imp->isBeingDestroyedMutex);
        destroyed = _imp->isBeingDestroyed;
    }

    NodePtr curIn;
    {
        QMutexLocker l(&_imp->inputsMutex);
        ///Set the input

        curIn = _imp->inputs[inputNumber].lock();

        if (failIfExisting && curIn) {
            return false;
        }

        if (curIn == input) {
            // Nothing changed
            return true;
        }

        NodePtr thisShared = shared_from_this();
        if (curIn) {
            QObject::connect( curIn.get(), SIGNAL(labelChanged(QString,QString)), this, SLOT(onInputLabelChanged(QString,QString)), Qt::UniqueConnection );
            curIn->disconnectOutput(thisShared, inputNumber);
        }
        if (input) {
            _imp->inputs[inputNumber] = input;
            input->connectOutput(thisShared, inputNumber);
        } else {
            _imp->inputs[inputNumber].reset();
        }
    }

    if (destroyed) {
        // Don't do more if the node is destroyed because we would run code that is not needed on the node.
        return true;
    }

    // Make the application recheck expressions, they may now be valid again.
    getApp()->recheckInvalidLinks();

    ///Get notified when the input name has changed
    if (input) {
        QObject::connect( input.get(), SIGNAL(labelChanged(QString,QString)), this, SLOT(onInputLabelChanged(QString, QString)),Qt::UniqueConnection );
    }

    // Notify the GUI
    Q_EMIT inputChanged(inputNumber);

    beginInputEdition();

    // Call the instance changed action with a reason clip changed
    onInputChanged(inputNumber);

    // Notify cache
    _imp->effect->invalidateHashCache();

    // Run Python callback
    std::string inputChangedCB = _imp->effect->getInputChangedCallback();
    if ( !inputChangedCB.empty() ) {
        _imp->runInputChangedCallback(inputNumber, inputChangedCB);
    }

    endInputEdition(true);

    return true;
} // replaceInputInternal

bool
Node::swapInput(const NodePtr& input,
                int inputNumber)
{

    return replaceInputInternal(input, inputNumber, false);
} // Node::replaceInput

void
Node::connectOutputsToMainInput()
{
    NodePtr mainInputNode;
    int prefInput_i = getPreferredInput();
    if (prefInput_i != -1) {
        mainInputNode = getRealInput(prefInput_i);
    }


    OutputNodesMap outputs;
    getOutputs(outputs);
    for (OutputNodesMap::const_iterator it = outputs.begin(); it != outputs.end(); ++it) {
        for (std::list<int>::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
            it->first->swapInput(mainInputNode, *it2);
        }
    }

} // connectOutputsToMainInput

void
Node::switchInput0And1()
{
    int maxInputs = getNInputs();
    if (maxInputs < 2) {
        return;
    }
    ///get the first input number to switch
    int inputAIndex = -1;
    for (int i = 0; i < maxInputs; ++i) {
        if ( !isInputMask(i) ) {
            inputAIndex = i;
            break;
        }
    }

    ///There's only a mask ??
    if (inputAIndex == -1) {
        return;
    }

    ///get the second input number to switch
    int inputBIndex = -1;
    int firstMaskInput = -1;
    for (int j = 0; j < maxInputs; ++j) {
        if (j == inputAIndex) {
            continue;
        }
        if ( !isInputMask(j) ) {
            inputBIndex = j;
            break;
        } else {
            firstMaskInput = j;
        }
    }
    if (inputBIndex == -1) {
        ///if there's a mask use it as input B for the switch
        if (firstMaskInput != -1) {
            inputBIndex = firstMaskInput;
        }
    }


    NodePtr input0, input1;

    {
        QMutexLocker l(&_imp->inputsMutex);
        assert( inputAIndex < (int)_imp->inputs.size() && inputBIndex < (int)_imp->inputs.size() );

        input0 = _imp->inputs[inputAIndex].lock();
        input1 = _imp->inputs[inputBIndex].lock();
        _imp->inputs[inputAIndex] = _imp->inputs[inputBIndex];
        _imp->inputs[inputBIndex] = input0;


    }
    Q_EMIT inputChanged(inputAIndex);
    Q_EMIT inputChanged(inputBIndex);

    beginInputEdition();
    onInputChanged(inputAIndex);
    onInputChanged(inputBIndex);


    // Notify cache
    _imp->effect->invalidateHashCache();



    std::string inputChangedCB = _imp->effect->getInputChangedCallback();
    if ( !inputChangedCB.empty() ) {
        _imp->runInputChangedCallback(inputAIndex, inputChangedCB);
        _imp->runInputChangedCallback(inputBIndex, inputChangedCB);
    }


    endInputEdition(true);
} // switchInput0And1

void
Node::onInputLabelChanged(const QString& /*oldName*/, const QString & newName)
{
    assert( QThread::currentThread() == qApp->thread() );
    Node* inp = dynamic_cast<Node*>( sender() );
    assert(inp);
    if (!inp) {
        // coverity[dead_error_line]
        return;
    }
    int inputNb = -1;
    ///No need to lock, inputs is only written to by the mainthread

    for (U32 i = 0; i < _imp->inputs.size(); ++i) {
        if (_imp->inputs[i].lock().get() == inp) {
            inputNb = i;
            break;
        }
    }

    if (inputNb != -1) {
        Q_EMIT inputLabelChanged(inputNb, newName);
    }
}

void
Node::connectOutput(const NodePtr& output, int outputInputIndex)
{
    assert(output);

    {
        QMutexLocker l(&_imp->outputsMutex);
        std::list<int>& indices = _imp->outputs[output];
        std::list<int>::const_iterator foundIndex = std::find(indices.begin(), indices.end(), outputInputIndex);
        if (foundIndex != indices.end()) {
            return;
        }
        indices.push_back(outputInputIndex);
    }
    Q_EMIT outputsChanged();
}

bool
Node::disconnectInput(int inputNumber)
{
    return replaceInputInternal(NodePtr(), inputNumber, false);
} // Node::disconnectInput


bool
Node::disconnectInput(const NodePtr& input)
{
    std::list<int> indices = input->getInputIndicesConnectedToThisNode(shared_from_this());

    if (!indices.empty()) {
        for (std::list<int>::const_iterator it = indices.begin(); it != indices.end(); ++it) {
            swapInput(NodePtr(), *it);
        }
        return true;
    }
    return false;

} // Node::disconnectInput

bool
Node::disconnectOutput(const NodePtr& output, int outputInputIndex)
{
    assert(output);
    bool ret = false;
    {
        QMutexLocker l(&_imp->outputsMutex);
        InternalOutputNodesMap::iterator found = _imp->outputs.find(output);
        if (found != _imp->outputs.end()) {
            std::list<int>::iterator foundIndex = std::find(found->second.begin(), found->second.end(), outputInputIndex);
            if (foundIndex != found->second.end()) {
                found->second.erase(foundIndex);
                ret = true;
            }
            if (found->second.empty()) {
                _imp->outputs.erase(found);
            }
        }
    }

    //Will just refresh the gui
    Q_EMIT outputsChanged();

    return ret;
}


void
Node::getOutputs(OutputNodesMap& outputs) const
{
    QMutexLocker l(&_imp->outputsMutex);
    for (InternalOutputNodesMap::const_iterator it = _imp->outputs.begin(); it != _imp->outputs.end(); ++it) {
        NodePtr output = it->first.lock();
        if (!output) {
            continue;
        }
        outputs[output] = it->second;
    }
}

void
Node::getInputNames(std::map<std::string, std::string> & inputNames,
                    std::map<std::string, std::string> & maskNames) const
{
    // This is called by the serialization thread.
    // We use the inputs because we want to serialize exactly how the tree was to the user

    {
        QMutexLocker l(&_imp->inputsMutex);
        assert(_imp->inputs.size() == _imp->inputDescriptions.size());
        for (std::size_t i = 0; i < _imp->inputs.size(); ++i) {
            NodePtr input = _imp->inputs[i].lock();
            std::map<std::string, std::string>* container = 0;

            if (_imp->inputDescriptions[i]->getPropertyUnsafe<bool>(kInputDescPropIsMask)) {
                container = &maskNames;
            } else {
                container = &inputNames;
            }

            std::string inputLabel = _imp->inputDescriptions[i]->getPropertyUnsafe<std::string>(kInputDescPropLabel);
            if (input) {
                (*container)[inputLabel] = input->getScriptName_mt_safe();
            } else {
                (*container)[inputLabel] = std::string();
            }

        }

    }
}

int
Node::getPreferredInputInternal(bool connected) const
{
    if (isBeingDestroyed()) {
        return -1;
    }
    {
        int prefInput;
        if (_imp->effect->getDefaultInput(connected, &prefInput)) {
            return prefInput;
        }
    }
    int nInputs = getNInputs();

    if (nInputs == 0) {
        return -1;
    }
    std::vector<NodePtr> inputs(nInputs);
    std::vector<std::string> inputLabels(nInputs);
    int inputA = -1;
    int inputB = -1;
    {
        // fill input labels, and if one is called "Source", return it
        // if it's "A" or "B", keep the index.
        for (int i = 0; i < nInputs; ++i) {
            std::string inputLabel = getInputLabel(i);
            //printf("%d->%s\n", i, inputLabel.c_str());
            if (inputLabel == kOfxImageEffectSimpleSourceClipName) {
                inputs[i] = getInput(i);
                if ( (connected && inputs[i]) || (!connected && !inputs[i]) ) {
                    return i;
                }
            } else if (inputLabel == "A") {
                inputA = i;
            } else if (inputLabel == "B") {
                inputB = i;
            }
        }
    }
    bool useInputA = false;
    if (!connected) {
        // For the merge node, use the preference (only when not connected)
        useInputA = appPTR->getCurrentSettings()->useInputAForMergeAutoConnect();
    }

    ///Find an input named A
    int inputToFind = -1, foundOther = -1;
    if ( useInputA || (getPluginID() == PLUGINID_OFX_SHUFFLE && getMajorVersion() < 3) ) {
        inputToFind = inputA;
        foundOther = inputB;
    } else {
        inputToFind = inputB;
        foundOther = inputA;
    }
    if (inputToFind != -1) {
        inputs[inputToFind] = getInput(inputToFind);
        if ( (connected && inputs[inputToFind]) || (!connected && !inputs[inputToFind]) ) {
            return inputToFind;
        }
    }
    if (foundOther != -1) {
        inputs[foundOther] = getInput(foundOther);
        if ( (connected && inputs[foundOther]) || (!connected && !inputs[foundOther]) ) {
            return foundOther;
        }
    }


    for (int i = 0; i < nInputs; ++i) {
        if (!inputs[i]) {
            inputs[i] = getInput(i);
        }
    }


    ///we return the first non-optional empty input
    int firstNonOptionalEmptyInput = -1;
    std::list<int> optionalEmptyInputs;
    std::list<int> optionalEmptyMasks;

    for (int i = 0; i < nInputs; ++i) {
        if ( (connected && inputs[i]) || (!connected && !inputs[i]) ) {
            if ( !isInputOptional(i) ) {
                if (firstNonOptionalEmptyInput == -1) {
                    firstNonOptionalEmptyInput = i;
                    break;
                }
            } else {
                if ( isInputMask(i) ) {
                    optionalEmptyMasks.push_back(i);
                } else {
                    optionalEmptyInputs.push_back(i);
                }
            }
        }
    }


    ///Default to the first non optional empty input
    if (firstNonOptionalEmptyInput != -1) {
        return firstNonOptionalEmptyInput;
    }  else {
        if ( !optionalEmptyInputs.empty() ) {
            //We return the first optional empty input
            std::list<int>::iterator first = optionalEmptyInputs.begin();

            if ( first == optionalEmptyInputs.end() ) {
                return -1;
            } else {
                return *first;
            }
        } else if ( !optionalEmptyMasks.empty() ) {
            return optionalEmptyMasks.front();
        } else {
            return -1;
        }
    }
} // Node::getPreferredInputInternal

int
Node::getPreferredInput() const
{
    return getPreferredInputInternal(true);
}

int
Node::getPreferredInputForConnection() const
{
    return getPreferredInputInternal(false);
}

NodePtr
Node::getPreferredInputNode() const
{
    GroupInputPtr isInput = isEffectGroupInput();

    if (isInput) {
        NodeGroupPtr isGroup = toNodeGroup(getGroup());
        assert(isGroup);
        if (!isGroup) {
            return NodePtr();
        }
        int inputNb = -1;
        std::vector<NodePtr> groupInputs;
        isGroup->getInputs(&groupInputs);
        for (std::size_t i = 0; i < groupInputs.size(); ++i) {
            if (groupInputs[i].get() == this) {
                inputNb = i;
                break;
            }
        }
        if (inputNb != -1) {
            NodePtr input = isGroup->getNode()->getInput(inputNb);

            return input;
        }
    } else {
        int idx = getPreferredInput();
        if (idx != -1) {
            return getInput(idx);
        }
    }

    return NodePtr();
}


void
Node::beginInputEdition()
{
    assert( QThread::currentThread() == qApp->thread() );
    ++_imp->inputModifiedRecursion;
}

void
Node::endInputEdition(bool triggerRender)
{
    if (isBeingDestroyed()) {
        return;
    }
    assert( QThread::currentThread() == qApp->thread() );
    if (_imp->inputModifiedRecursion > 0) {
        --_imp->inputModifiedRecursion;
    }

    if (!_imp->inputModifiedRecursion) {

        if (_imp->hasModifiedInputsDescription > 0) {
            _imp->hasModifiedInputsDescription = 0;
            Q_EMIT inputsDescriptionChanged();
        }

        if (!getApp()->getProject()->isLoadingProject() && !getApp()->isCreatingNode()) {
            bool hasChanged = !_imp->inputsModified.empty();
            _imp->inputsModified.clear();

            if (hasChanged) {

                // Force a refresh of the meta-datas

                _imp->effect->onMetadataChanged_recursive_public();
            }

            triggerRender = triggerRender && hasChanged;

            if (triggerRender) {
                getApp()->renderAllViewers();
            }
        }
    }
}

void
Node::onInputChanged(int inputNb)
{
    if ( getApp()->getProject()->isProjectClosing() || isBeingDestroyed() ) {
        return;
    }
    assert( QThread::currentThread() == qApp->thread() );

    bool mustCallEndInputEdition = _imp->inputModifiedRecursion == 0;
    if (mustCallEndInputEdition) {
        beginInputEdition();
    }


    //if (!getApp()->isCreatingNode()) {
        _imp->effect->onInputChanged_public(inputNb);
    //}
    _imp->inputsModified.insert(inputNb);


    /*
     If this is a group, also notify the output nodes of the GroupInput node inside the Group corresponding to
     the this inputNb
     */
    NodeGroupPtr isGroup = isEffectNodeGroup();
    if (isGroup) {
        std::vector<NodePtr> groupInputs;
        isGroup->getInputs(&groupInputs);
        if ( (inputNb >= 0) && ( inputNb < (int)groupInputs.size() ) && groupInputs[inputNb] ) {
            OutputNodesMap inputOutputs;
            groupInputs[inputNb]->getOutputs(inputOutputs);
            for (OutputNodesMap::iterator it = inputOutputs.begin(); it != inputOutputs.end(); ++it) {
                for (std::list<int>::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
                    it->first->onInputChanged(*it2);
                }
            }
        }
    }

    /*
     If this is an output node, notify the Group output nodes that their input have changed.
     */
    GroupOutputPtr isGroupOutput = isEffectGroupOutput();
    if (isGroupOutput) {
        NodeGroupPtr containerGroup = toNodeGroup( isGroupOutput->getNode()->getGroup() );
        if (containerGroup) {
            OutputNodesMap groupOutputs;
            containerGroup->getNode()->getOutputs(groupOutputs);
            for (OutputNodesMap::iterator it = groupOutputs.begin(); it != groupOutputs.end(); ++it) {
                for (std::list<int>::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
                    it->first->onInputChanged(*it2);
                }
            }
        }
    }

    if (mustCallEndInputEdition) {
        endInputEdition(true);
    }
} // Node::onInputChanged


bool
Node::duringInputChangedAction() const
{
    assert( QThread::currentThread() == qApp->thread() );

    return _imp->inputModifiedRecursion > 0;
}

static std::string removeTrailingDigits(const std::string& str)
{
    if (str.empty()) {
        return std::string();
    }
    std::size_t i = str.size() - 1;
    while (i > 0 && std::isdigit(str[i])) {
        --i;
    }

    if (i == 0) {
        // Name only consists of digits
        return std::string();
    }

    return str.substr(0, i + 1);
}


bool
Node::isEntitledForInspectorInputsStyle() const
{

    // Find the number of inputs that share the same basename

    int nInputsWithSameBasename = 0;

    std::string baseInputName;

    // We need a boolean here because the baseInputName may be empty in the case input names only contain numbers
    bool baseInputNameSet = false;

    int maxInputs = getNInputs();
    for (int i = 0; i < maxInputs; ++i) {
        if (!baseInputNameSet) {
            baseInputName = removeTrailingDigits(getInputLabel(i));
            baseInputNameSet = true;
            nInputsWithSameBasename = 1;
        } else {
            std::string thisBaseName = removeTrailingDigits(getInputLabel(i));
            if (thisBaseName == baseInputName) {
                ++nInputsWithSameBasename;
            }
        }

    }
    return maxInputs > 4 && nInputsWithSameBasename >= 4;
}

bool
Node::hasSequentialOnlyNodeUpstream(std::string & nodeName) const
{
    if (isBeingDestroyed()) {
        return false;
    }
    ///Just take into account sequentiallity for writers
    if ( (_imp->effect->getSequentialRenderSupport() == eSequentialPreferenceOnlySequential) && _imp->effect->isWriter() ) {
        nodeName = getScriptName_mt_safe();

        return true;
    } else {
        QMutexLocker l(&_imp->inputsMutex);

        for (InputsV::iterator it = _imp->inputs.begin(); it != _imp->inputs.end(); ++it) {
            NodePtr input = it->lock();
            if ( input && input->hasSequentialOnlyNodeUpstream(nodeName) && input->getEffectInstance()->isWriter() ) {
                nodeName = input->getScriptName_mt_safe();

                return true;
            }
        }

        return false;
    }
}

void
Node::moveBelowPositionRecursively(const RectD & r)
{

    RectD boundingRect = getNodeBoundingRect();
    if (!r.intersects(boundingRect)) {
        return;
    }
    movePosition(0, r.height() + boundingRect.height() / 2.);

    boundingRect = getNodeBoundingRect();

    OutputNodesMap outputs;
    getOutputs(outputs);
    for (OutputNodesMap::const_iterator it = outputs.begin(); it != outputs.end(); ++it) {
        it->first->moveBelowPositionRecursively(boundingRect);
    }

}

void
Node::moveAbovePositionRecursively(const RectD & r)
{
    RectD boundingRect = getNodeBoundingRect();
    if (!r.intersects(boundingRect)) {
        return;
    }

    movePosition(0, -r.height() - boundingRect.height() / 2.);

    boundingRect = getNodeBoundingRect();

    const std::vector<NodeWPtr>& inputs = getInputs();
    for (std::size_t i = 0; i < inputs.size(); ++i) {
        NodePtr input = inputs[i].lock();
        if (!input) {
            continue;
        }
        input->moveAbovePositionRecursively(boundingRect);
    }
}


bool
Node::autoConnect(const NodePtr& selected)
{
    if (!canOthersConnectToThisNode()) {
        return false;
    }

    if (!selected) {
        return false;
    }

    // 2 possible values:
    // 1 = pop the node above the selected node and move the inputs of the selected node a little
    // 2 = pop the node below the selected node and move the outputs of the selected node a little
    enum AutoConnectBehaviorEnum {
        eAutoConnectBehaviorAbove,
        eAutoConnectBehaviorBelow
    };

    AutoConnectBehaviorEnum behavior;

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

    ///1)
    if ( selected->isOutputNode() ) {
        ///case 1-a) just do default we don't know what else to do
        if (isOutputNode()) {
            return false;
        } else {
            ///for either cases 1-b) or 1-c) we just connect the created node as input of the selected node.
            behavior = eAutoConnectBehaviorAbove;
        }
    }
    ///2) and 3) are similar except for case b)
    else {
        ///case 2 or 3- a): connect the created node as output of the selected node.
        if (isOutputNode()) {
            behavior = eAutoConnectBehaviorBelow;
        }
        ///case b)
        else if (isInputNode()) {
            if ( selected->getEffectInstance()->isReader() ) {
                ///case 2-b) just do default we don't know what else to do
                return false;
            } else {
                ///case 3-b): connect the created node as input of the selected node
                behavior = eAutoConnectBehaviorAbove;
            }
        }
        ///case c) connect created as output of the selected node
        else {
            behavior = eAutoConnectBehaviorBelow;
        }
    }


    // If behaviour is 1 , just check that we can effectively connect the node to avoid moving them for nothing
    // otherwise fail
    if (behavior == eAutoConnectBehaviorAbove) {
        const std::vector<NodeWPtr> & inputs = selected->getInputs();
        bool oneInputEmpty = false;
        for (std::size_t i = 0; i < inputs.size(); ++i) {
            if ( !inputs[i].lock() ) {
                oneInputEmpty = true;
                break;
            }
        }
        if (!oneInputEmpty) {
            return false;
        }
    }

    Point selectedNodeSize, selectedNodePos;
    selected->getSize(&selectedNodeSize.x, &selectedNodeSize.y);
    selected->getPosition(&selectedNodePos.x, &selectedNodePos.y);
    Point createdNodeSize, createdNodePos;
    getSize(&createdNodeSize.x, &createdNodeSize.y);
    getPosition(&createdNodePos.x, &createdNodePos.y);

    Point selectedNodeMiddlePos;
    selectedNodeMiddlePos.x = selectedNodePos.x + selectedNodeSize.x / 2.;
    selectedNodeMiddlePos.y = selectedNodePos.y + selectedNodeSize.y / 2.;

    RectD selectedNodeRect( selectedNodePos.x, selectedNodePos.y, selectedNodePos.x + selectedNodeSize.x, selectedNodePos.y + selectedNodeSize.y);

    Point position;
    if (behavior == eAutoConnectBehaviorAbove) {
        ///pop it above the selected node

        ///If this is the first connected input, insert it in a "linear" way so the tree remains vertical
        int nbConnectedInput = 0;
        const std::vector<NodeWPtr> & inputs = selected->getInputs();
        for (std::size_t i = 0; i < inputs.size(); ++i) {
            if ( inputs[i].lock() ) {
                ++nbConnectedInput;
            }
        }

        // Connect it to the first input


        if (nbConnectedInput == 0) {

            position.x = selectedNodeMiddlePos.x - createdNodeSize.x / 2.;
            position.y = selectedNodeMiddlePos.y - selectedNodeSize.y / 2. - createdNodeSize.x / 2. - createdNodeSize.y;

            RectD createdNodeRect( position.x, position.y, position.x + createdNodeSize.x, position.y + createdNodeSize.y);

            // Now that we have the position of the node, move the inputs of the selected node to make some space for this node
            for (std::size_t i = 0; i < inputs.size(); ++i) {
                NodePtr input = inputs[i].lock();
                if (input) {
                    input->moveAbovePositionRecursively(createdNodeRect);
                }
            }

            int selectedInput = selected->getPreferredInputForConnection();
            if (selectedInput != -1) {
                selected->swapInput(shared_from_this(), selectedInput);
            }
        } else {

            Point selectedCenter;
            selectedCenter.x = (selectedNodeRect.x1 + selectedNodeRect.x2) / 2.;
            selectedCenter.y = (selectedNodeRect.y1 + selectedNodeRect.y2) / 2.;

            position.x = selectedCenter.x + nbConnectedInput * 150 - createdNodeSize.x / 2.;
            position.y = selectedCenter.y - selectedNodeSize.y / 2. - createdNodeSize.x / 2. - createdNodeSize.y;


            int index = selected->getPreferredInputForConnection();
            if (index != -1) {
                selected->swapInput(shared_from_this(), index);
            }
            //} // if (isSelectedViewer) {*/
        } // if (nbConnectedInput == 0) {
    } else if (behavior == eAutoConnectBehaviorBelow) {

        // Pop it below the selected node

        OutputNodesMap outputs;
        selected->getOutputs(outputs);
        if ( !isOutputNode() || outputs.empty() ) {

            ///actually move the created node where the selected node is
            position.x = selectedNodeMiddlePos.x - createdNodeSize.x / 2.;
            position.y = selectedNodeMiddlePos.y + selectedNodeSize.y / 2. + selectedNodeSize.y / 2.;

            RectD createdNodeRect( position.x, position.y, position.x + createdNodeSize.x, position.y + createdNodeSize.y);

            ///and move the selected node below recusively
            for (OutputNodesMap::const_iterator it = outputs.begin(); it != outputs.end(); ++it) {
                it->first->moveBelowPositionRecursively(createdNodeRect);
            }

            ///Connect the created node to the selected node
            ///finally we connect the created node to the selected node
            int createdInput = getPreferredInputForConnection();
            if (createdInput != -1) {
                ignore_result( connectInput(selected, createdInput) );
            }

            if ( !isOutputNode() ) {
                ///we find all the nodes that were previously connected to the selected node,
                ///and connect them to the created node instead.
                OutputNodesMap outputsConnectedToSelectedNode;
                selected->getOutputs(outputsConnectedToSelectedNode);

                for (OutputNodesMap::iterator it = outputsConnectedToSelectedNode.begin(); it != outputsConnectedToSelectedNode.end(); ++it) {
                    const NodePtr &output = it->first;
                    for (std::list<int>::iterator it2 = it->second.begin(); it2 !=it->second.end(); ++it2) {
                        output->swapInput(shared_from_this(), *it2);
                    }
                }


            }
        } else {
            ///the created node is an output node and the selected node already has several outputs, create it aside
            position.x = selectedNodeMiddlePos.x + (int)outputs.size() * 150 - createdNodeSize.x / 2.;
            position.y = selectedNodeMiddlePos.y + selectedNodeSize.y / 2. + selectedNodeSize.y / 2.;

            //Don't pop a dot, it will most likely annoy the user, just fallback on behavior 0

            int index = getPreferredInputForConnection();
            if (index != -1) {
                swapInput(selected, index);
            }
        }
    }

    setPosition(position.x, position.y);
    return true;

} // autoConnect

NATRON_NAMESPACE_EXIT
