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

#include "NodePrivate.h"

#include <QDebug>

#include "Engine/GroupOutput.h"
#include "Engine/PrecompNode.h"
#include "Engine/Settings.h"

NATRON_NAMESPACE_ENTER;


void
Node::initializeInputs()
{
    ////Only called by the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    const int inputCount = getMaxInputCount();
    InputsV oldInputs;
    {
        QMutexLocker k(&_imp->inputsLabelsMutex);
        _imp->inputLabels.resize(inputCount);
        _imp->inputHints.resize(inputCount);
        for (int i = 0; i < inputCount; ++i) {
            _imp->inputLabels[i] = _imp->effect->getInputLabel(i);
            _imp->inputHints[i] = _imp->effect->getInputHint(i);
        }
    }
    {
        QMutexLocker l(&_imp->lastRenderStartedMutex);
        _imp->inputIsRenderingCounter.resize(inputCount);
    }
    {
        QMutexLocker l(&_imp->inputsMutex);
        oldInputs = _imp->inputs;

        std::vector<bool> oldInputsVisibility = _imp->inputsVisibility;
        _imp->inputIsRenderingCounter.resize(inputCount);
        _imp->inputs.resize(inputCount);
        _imp->inputsVisibility.resize(inputCount);
        ///if we added inputs, just set to NULL the new inputs, and add their label to the labels map
        for (int i = 0; i < inputCount; ++i) {
            if ( i < (int)oldInputs.size() ) {
                _imp->inputs[i] = oldInputs[i];
            } else {
                _imp->inputs[i].reset();
            }
            if (i < (int) oldInputsVisibility.size()) {
                _imp->inputsVisibility[i] = oldInputsVisibility[i];
            } else {
                _imp->inputsVisibility[i] = true;
            }
        }


        ///Set the components the plug-in accepts
        _imp->effect->refreshAcceptedComponents(inputCount);
    }
    _imp->inputsInitialized = true;

    Q_EMIT inputsInitialized();
} // Node::initializeInputs





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
    if (!_imp->inputsInitialized) {
        qDebug() << "Node::getInput(): inputs not initialized";
    }


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

const std::vector<NodeWPtr > &
Node::getInputs() const
{
    ////Only called by the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    assert(_imp->inputsInitialized);
    return _imp->inputs;
}


std::vector<NodeWPtr >
Node::getInputs_copy() const
{
    assert(_imp->inputsInitialized);

    QMutexLocker l(&_imp->inputsMutex);

    return _imp->inputs;
}

std::string
Node::getInputLabel(int inputNb) const
{
    assert(_imp->inputsInitialized);

    QMutexLocker l(&_imp->inputsLabelsMutex);
    if ( (inputNb < 0) || ( inputNb >= (int)_imp->inputLabels.size() ) ) {
        throw std::invalid_argument("Index out of range");
    }

    return _imp->inputLabels[inputNb];
}

std::string
Node::getInputHint(int inputNb) const
{
    assert(_imp->inputsInitialized);

    QMutexLocker l(&_imp->inputsLabelsMutex);
    if ( (inputNb < 0) || ( inputNb >= (int)_imp->inputHints.size() ) ) {
        throw std::invalid_argument("Index out of range");
    }

    return _imp->inputHints[inputNb];
}

void
Node::setInputLabel(int inputNb, const std::string& label)
{
    {
        QMutexLocker l(&_imp->inputsLabelsMutex);
        if ( (inputNb < 0) || ( inputNb >= (int)_imp->inputLabels.size() ) ) {
            throw std::invalid_argument("Index out of range");
        }
        _imp->inputLabels[inputNb] = label;
    }
    _imp->effect->onInputLabelChanged(inputNb, label);

    Q_EMIT inputEdgeLabelChanged(inputNb, QString::fromUtf8(label.c_str()));
}

void
Node::setInputHint(int inputNb, const std::string& hint)
{
    {
        QMutexLocker l(&_imp->inputsLabelsMutex);
        if ( (inputNb < 0) || ( inputNb >= (int)_imp->inputHints.size() ) ) {
            throw std::invalid_argument("Index out of range");
        }
        _imp->inputHints[inputNb] = hint;
    }
}

bool
Node::isInputVisible(int inputNb) const
{
    QMutexLocker k(&_imp->inputsMutex);
    if (inputNb >= 0 && inputNb < (int)_imp->inputsVisibility.size()) {
        return _imp->inputsVisibility[inputNb];
    } else {
        throw std::invalid_argument("Index out of range");
    }
    return false;
}

void
Node::setInputVisible(int inputNb, bool visible)
{
    {
        QMutexLocker k(&_imp->inputsMutex);
        if (inputNb >= 0 && inputNb < (int)_imp->inputsVisibility.size()) {
            _imp->inputsVisibility[inputNb] = visible;
        } else {
            throw std::invalid_argument("Index out of range");
        }
    }
    Q_EMIT inputVisibilityChanged(inputNb);
}


int
Node::getInputNumberFromLabel(const std::string& inputLabel) const
{
    if (inputLabel.empty()) {
        return -1;
    }
    assert(_imp->inputsInitialized);
    QMutexLocker l(&_imp->inputsLabelsMutex);
    for (U32 i = 0; i < _imp->inputLabels.size(); ++i) {
        if (_imp->inputLabels[i] == inputLabel) {
            return i;
        }
    }

    return -1;
}

bool
Node::isInputConnected(int inputNb) const
{
    assert(_imp->inputsInitialized);

    return getInput(inputNb) != NULL;
}

bool
Node::hasInputConnected() const
{
    assert(_imp->inputsInitialized);

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

    for (U32 i = 0; i < _imp->inputs.size(); ++i) {
        if ( !_imp->inputs[i].lock() && !_imp->effect->isInputOptional(i) ) {
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
    assert( QThread::currentThread() == qApp->thread() );
    if (!input || input.get() == this) {
        return false;
    }
    bool found = input->isNodeUpstream(shared_from_this());
    return !found;
}

bool
Node::isNodeUpstreamInternal(const NodeConstPtr& input, std::list<const Node*>& markedNodes) const
{
    if (!input) {
        return false;
    }

    if (std::find(markedNodes.begin(), markedNodes.end(), this) != markedNodes.end()) {
        return false;
    }

    markedNodes.push_back(this);

    // No need to lock inputs is only written to by the main-thread
    for (std::size_t i = 0; i  < _imp->inputs.size(); ++i) {
        if (_imp->inputs[i].lock() == input) {
            return true;
        }
    }

    for (std::size_t i = 0; i  < _imp->inputs.size(); ++i) {
        NodePtr in = _imp->inputs[i].lock();
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
    ////Only called by the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    std::list<const Node*> markedNodes;
    return isNodeUpstreamInternal(input, markedNodes);
}

#if 0
static Node::CanConnectInputReturnValue
checkCanConnectNoMultiRes(const Node* output,
                          const NodePtr& input)
{
    //http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#kOfxImageEffectPropSupportsMultiResolution
    //Check that the input has the same RoD that another input and that its rod is set to 0,0
    RenderScale scale(1.);
    RectD rod;
    {
        GetRegionOfDefinitionResultsPtr actionResults;
        ActionRetCodeEnum stat = input->getEffectInstance()->getRegionOfDefinition_public(TimeValue(output->getApp()->getTimeLine()->currentFrame()),
                                                                                          scale,
                                                                                          ViewIdx(0),
                                                                                          &actionResults);
        if (isFailureRetCode(stat)) {
            return Node::eCanConnectInput_givenNodeNotConnectable;
        }
        rod = actionResults->getRoD();
    }
    if (rod.isNull()) {
        return Node::eCanConnectInput_givenNodeNotConnectable;
    }

    if ( (rod.x1 != 0) || (rod.y1 != 0) ) {
        return Node::eCanConnectInput_multiResNotSupported;
    }

    // Commented-out: Some Furnace plug-ins from The Foundry (e.g F_Steadiness) are not supporting multi-resolution but actually produce an output
    // with a RoD different from the input

    /*RectD outputRod;
     stat = output->getEffectInstance()->getRegionOfDefinition_public(output->getCacheID(), output->getApp()->getTimeLine()->currentFrame(), scale, ViewIdx(0), &outputRod);
     Q_UNUSED(stat);

     if ( !outputRod.isNull() && (rod != outputRod) ) {
     return Node::eCanConnectInput_multiResNotSupported;
     }*/

    for (int i = 0; i < output->getMaxInputCount(); ++i) {
        NodePtr inputNode = output->getInput(i);
        if (inputNode) {

            GetRegionOfDefinitionResultsPtr actionResults;
            ActionRetCodeEnum stat = inputNode->getEffectInstance()->getRegionOfDefinition_public(TimeValue(output->getApp()->getTimeLine()->currentFrame()),
                                                                                                  scale,
                                                                                                  ViewIdx(0),
                                                                                                  &actionResults);
            if ( isFailureRetCode(stat)) {
                return Node::eCanConnectInput_givenNodeNotConnectable;
            }
            RectD inputRod = actionResults->getRoD();
            if (inputRod != rod) {
                return Node::eCanConnectInput_multiResNotSupported;
            }
        }
    }

    return Node::eCanConnectInput_ok;
}
#endif // 0


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

#if 0
    if ( !_imp->effect->supportsMultiResolution() ) {
        CanConnectInputReturnValue ret = checkCanConnectNoMultiRes(this, input);
        if (ret != eCanConnectInput_ok) {
            return ret;
        }
    }
#endif
    {
        ///Check for invalid pixel aspect ratio if the node doesn't support multiple clip PARs

        double inputPAR = input->getEffectInstance()->getAspectRatio(-1);
        double inputFPS = input->getEffectInstance()->getFrameRate();
        QMutexLocker l(&_imp->inputsMutex);

        for (InputsV::const_iterator it = _imp->inputs.begin(); it != _imp->inputs.end(); ++it) {
            NodePtr node = it->lock();
            if (node) {
                if ( !_imp->effect->supportsMultipleClipPARs() ) {
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
    if ( isEffectBackdrop() ) {
        return false;
    } else if ( isEffectGroupOutput() ) {
        return false;
    } else if ( _imp->effect->isWriter() && (_imp->effect->getSequentialPreference() == eSequentialPreferenceOnlySequential) ) {
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

    assert(_imp->inputsInitialized);

    if (!input) {
        return false;
    }


    return replaceInputInternal(input, inputNumber, true);
} // Node::connectInput


bool
Node::replaceInputInternal(const NodePtr& input, int inputNumber, bool failIfExisting)
{
    assert(_imp->inputsInitialized);

    // Check for cycles: they are forbidden in the graph
    if ( input && !checkIfConnectingInputIsOk( input ) ) {
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
Node::switchInput0And1()
{
    assert(_imp->inputsInitialized);
    int maxInputs = getMaxInputCount();
    if (maxInputs < 2) {
        return;
    }
    ///get the first input number to switch
    int inputAIndex = -1;
    for (int i = 0; i < maxInputs; ++i) {
        if ( !_imp->effect->isInputMask(i) ) {
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
        if ( !_imp->effect->isInputMask(j) ) {
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
    assert(_imp->inputsInitialized);
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
    assert(_imp->inputsInitialized);
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

const std::vector<std::string> &
Node::getInputLabels() const
{
    assert(_imp->inputsInitialized);
    ///MT-safe as it never changes.
    ////Only called by the main-thread
    assert( QThread::currentThread() == qApp->thread() );

    return _imp->inputLabels;
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

    std::vector<std::string> inputLabels;
    InputsV inputsVec;
    {
        QMutexLocker l(&_imp->inputsLabelsMutex);
        inputLabels = _imp->inputLabels;
    }
    {
        QMutexLocker l(&_imp->inputsMutex);
        inputsVec = _imp->inputs;
    }
    assert( inputsVec.size() == inputLabels.size() );

    for (std::size_t i = 0; i < inputsVec.size(); ++i) {
        NodePtr input = inputsVec[i].lock();

        std::map<std::string, std::string>* container = 0;
        if (_imp->effect->isInputMask(i)) {
            container = &maskNames;
        } else {
            container = &inputNames;
        }
        if (input) {
            (*container)[inputLabels[i]] = input->getScriptName_mt_safe();
        } else {
            (*container)[inputLabels[i]] = std::string();
        }
    }
}

int
Node::getPreferredInputInternal(bool connected) const
{
    {
        int prefInput;
        if (_imp->effect->getDefaultInput(connected, &prefInput)) {
            return prefInput;
        }
    }
    int nInputs = getMaxInputCount();

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
        useInputA = appPTR->getCurrentSettings()->isMergeAutoConnectingToAInput();
    }

    ///Find an input named A
    int inputToFind = -1, foundOther = -1;
    if ( useInputA || (getPluginID() == PLUGINID_OFX_SHUFFLE) ) {
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
            if ( !_imp->effect->isInputOptional(i) ) {
                if (firstNonOptionalEmptyInput == -1) {
                    firstNonOptionalEmptyInput = i;
                    break;
                }
            } else {
                if ( _imp->effect->isInputMask(i) ) {
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
    assert( QThread::currentThread() == qApp->thread() );
    if (_imp->inputModifiedRecursion > 0) {
        --_imp->inputModifiedRecursion;
    }

    if (!_imp->inputModifiedRecursion) {
        bool hasChanged = !_imp->inputsModified.empty();
        _imp->inputsModified.clear();

        if (hasChanged) {

            // Force a refresh of the meta-datas

            _imp->effect->onMetadataChanged_recursive_public();

            _imp->effect->refreshDynamicProperties();
        }

        triggerRender = triggerRender && hasChanged;

        if (triggerRender) {
            getApp()->renderAllViewers();
        }
    }
}

void
Node::onInputChanged(int inputNb)
{
    if ( getApp()->getProject()->isProjectClosing() ) {
        return;
    }
    assert( QThread::currentThread() == qApp->thread() );

    bool mustCallEndInputEdition = _imp->inputModifiedRecursion == 0;
    if (mustCallEndInputEdition) {
        beginInputEdition();
    }


    if (!getApp()->isDuringPythonPyPlugCreation()) {
        _imp->effect->onInputChanged_public(inputNb);
    }
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

    int maxInputs = getMaxInputCount();
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



int
Node::getMaxInputCount() const
{
    ///MT-safe, never changes
    if (!_imp->effect) {
        return 0;
    }
    return _imp->effect->getMaxInputCount();
}

bool
Node::hasSequentialOnlyNodeUpstream(std::string & nodeName) const
{
    ///Just take into account sequentiallity for writers
    if ( (_imp->effect->getSequentialPreference() == eSequentialPreferenceOnlySequential) && _imp->effect->isWriter() ) {
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


NATRON_NAMESPACE_EXIT;
