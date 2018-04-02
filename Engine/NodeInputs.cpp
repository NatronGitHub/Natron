/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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
#include <QtCore/QThread>

#include "Engine/NodeGroup.h"
#include "Engine/PrecompNode.h"
#include "Engine/GroupInput.h"
#include "Engine/GroupOutput.h"
#include "Engine/AppInstance.h"
#include "Engine/KnobTypes.h"
#include "Engine/Settings.h"
#include "Engine/TimeLine.h"
#include "Engine/Project.h"
#include "Engine/ViewerInstance.h"
#include "Engine/AbortableRenderInfo.h"
#include "Engine/ThreadPool.h"
#include "Engine/OpenGLViewerI.h"

NATRON_NAMESPACE_ENTER

/**
 * @brief Resolves links of the graph in the case of containers (that do not do any rendering but only contain nodes inside)
 * so that algorithms that cycle the tree from bottom to top
 * properly visit all nodes in the correct order
 **/
static NodePtr
applyNodeRedirectionsUpstream(const NodePtr& node,
                              bool useGuiInput)
{
    if (!node) {
        return node;
    }
    NodeGroup* isGrp = node->isEffectGroup();
    if (isGrp) {
        //The node is a group, instead jump directly to the output node input of the  group
        return applyNodeRedirectionsUpstream(isGrp->getOutputNodeInput(useGuiInput), useGuiInput);
    }

    PrecompNode* isPrecomp = dynamic_cast<PrecompNode*>( node->getEffectInstance().get() );
    if (isPrecomp) {
        //The node is a precomp, instead jump directly to the output node of the precomp
        return applyNodeRedirectionsUpstream(isPrecomp->getOutputNode(), useGuiInput);
    }

    GroupInput* isInput = dynamic_cast<GroupInput*>( node->getEffectInstance().get() );
    if (isInput) {
        //The node is a group input,  jump to the corresponding input of the group
        NodeCollectionPtr collection = node->getGroup();
        assert(collection);
        isGrp = dynamic_cast<NodeGroup*>( collection.get() );
        if (isGrp) {
            return applyNodeRedirectionsUpstream(isGrp->getRealInputForInput(useGuiInput, node), useGuiInput);
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
                                bool useGuiOutputs,
                                NodesList& translated)
{
    NodeGroup* isGrp = node->isEffectGroup();

    if (isGrp) {
        //The node is a group, meaning it should not be taken into account, instead jump directly to the input nodes output of the group
        NodesList inputNodes;
        isGrp->getInputsOutputs(&inputNodes, useGuiOutputs);
        for (NodesList::iterator it2 = inputNodes.begin(); it2 != inputNodes.end(); ++it2) {
            //Call recursively on them
            applyNodeRedirectionsDownstream(recurseCounter + 1, *it2, useGuiOutputs, translated);
        }

        return;
    }

    GroupOutput* isOutput = dynamic_cast<GroupOutput*>( node->getEffectInstance().get() );
    if (isOutput) {
        //The node is the output of a group, its outputs are the outputs of the group
        NodeCollectionPtr collection = isOutput->getNode()->getGroup();
        if (!collection) {
            return;
        }
        isGrp = dynamic_cast<NodeGroup*>( collection.get() );
        if (isGrp) {
            NodesWList groupOutputs;
            if (useGuiOutputs) {
                groupOutputs = isGrp->getNode()->getGuiOutputs();
            } else {
                NodePtr grpNode = isGrp->getNode();
                if (grpNode) {
                    grpNode->getOutputs_mt_safe(groupOutputs);
                }
            }
            for (NodesWList::iterator it2 = groupOutputs.begin(); it2 != groupOutputs.end(); ++it2) {
                //Call recursively on them
                NodePtr output = it2->lock();
                if (output) {
                    applyNodeRedirectionsDownstream(recurseCounter + 1, output, useGuiOutputs, translated);
                }
            }
        }

        return;
    }

    PrecompNodePtr isInPrecomp = node->isPartOfPrecomp();
    if ( isInPrecomp && (isInPrecomp->getOutputNode() == node) ) {
        //This node is the output of the precomp, its outputs are the outputs of the precomp node
        NodesWList groupOutputs;
        if (useGuiOutputs) {
            groupOutputs = isInPrecomp->getNode()->getGuiOutputs();
        } else {
            isInPrecomp->getNode()->getOutputs_mt_safe(groupOutputs);
        }
        for (NodesWList::iterator it2 = groupOutputs.begin(); it2 != groupOutputs.end(); ++it2) {
            //Call recursively on them
            NodePtr output = it2->lock();
            if (output) {
                applyNodeRedirectionsDownstream(recurseCounter + 1, output, useGuiOutputs, translated);
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
    NodesList redirections;
    NodePtr thisShared = boost::const_pointer_cast<Node>( shared_from_this() );

    applyNodeRedirectionsDownstream(0, thisShared, false, redirections);
    if ( !redirections.empty() ) {
        outputs.insert( outputs.begin(), redirections.begin(), redirections.end() );
    } else {
        QMutexLocker l(&_imp->outputsMutex);
        for (NodesWList::const_iterator it = _imp->outputs.begin(); it != _imp->outputs.end(); ++it) {
            NodePtr output = it->lock();
            if (output) {
                outputs.push_back(output);
            }
        }
    }
}


NodePtr
Node::getInput(int index) const
{
    return getInputInternal(false, true, index);
}

NodePtr
Node::getInputInternal(bool useGuiInput,
                       bool useGroupRedirections,
                       int index) const
{
    NodePtr parent = _imp->multiInstanceParent.lock();

    if (parent) {
        return parent->getInput(index);
    }
    if (!_imp->inputsInitialized) {
        qDebug() << "Node::getInput(): inputs not initialized";
    }
    QMutexLocker l(&_imp->inputsMutex);
    if ( ( index >= (int)_imp->inputs.size() ) || (index < 0) ) {
        return NodePtr();
    }

    NodePtr ret =  useGuiInput ? _imp->guiInputs[index].lock() : _imp->inputs[index].lock();
    if (ret && useGroupRedirections) {
        ret = applyNodeRedirectionsUpstream(ret, useGuiInput);
    }

    return ret;
}

NodePtr
Node::getGuiInput(int index) const
{
    return getInputInternal(true, true, index);
}

NodePtr
Node::getRealInput(int index) const
{
    return getInputInternal(false, false, index);
}

NodePtr
Node::getRealGuiInput(int index) const
{
    return getInputInternal(true, false, index);
}

int
Node::getInputIndex(const Node* node) const
{
    QMutexLocker l(&_imp->inputsMutex);

    for (U32 i = 0; i < _imp->inputs.size(); ++i) {
        if (_imp->inputs[i].lock().get() == node) {
            return i;
        }
    }

    return -1;
}

const std::vector<NodeWPtr> &
Node::getInputs() const
{
    ////Only called by the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    assert(_imp->inputsInitialized);

    NodePtr parent = _imp->multiInstanceParent.lock();
    if (parent) {
        return parent->getInputs();
    }

    return _imp->inputs;
}

const std::vector<NodeWPtr> &
Node::getGuiInputs() const
{
    ////Only called by the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    assert(_imp->inputsInitialized);

    NodePtr parent = _imp->multiInstanceParent.lock();
    if (parent) {
        return parent->getGuiInputs();
    }

    return _imp->guiInputs;
}

std::vector<NodeWPtr>
Node::getInputs_copy() const
{
    assert(_imp->inputsInitialized);

    NodePtr parent = _imp->multiInstanceParent.lock();
    if (parent) {
        return parent->getInputs();
    }

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
    std::map<int, MaskSelector>::iterator foundMask = _imp->maskSelectors.find(inputNb);
    if (foundMask != _imp->maskSelectors.end()) {
        foundMask->second.channel.lock()->setLabel(label);
    }

    std::map<int, ChannelSelector>::iterator foundChannel = _imp->channelsSelectors.find(inputNb);
    if (foundChannel != _imp->channelsSelectors.end()) {
        foundChannel->second.layer.lock()->setLabel(label + std::string(" Layer"));
    }

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
Node::isInputOnlyAlpha(int inputNb) const
{
    assert(_imp->inputsInitialized);

    const std::list<ImagePlaneDesc>& inputSupportedComps = _imp->inputsComponents[inputNb];
    return (inputSupportedComps.size() == 1 && inputSupportedComps.front().getNumComponents() == 1);
}

int
Node::getNInputs() const
{
    ///MT-safe, never changes
    assert(_imp->effect);

    return _imp->effect->getNInputs();
}

void
Node::initializeInputs()
{
    ////Only called by the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    const int inputCount = getNInputs();
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
        _imp->guiInputs.resize(inputCount);
        _imp->inputsVisibility.resize(inputCount);
        ///if we added inputs, just set to NULL the new inputs, and add their label to the labels map
        for (int i = 0; i < inputCount; ++i) {
            if ( i < (int)oldInputs.size() ) {
                _imp->inputs[i] = oldInputs[i];
                _imp->guiInputs[i] = oldInputs[i];
            } else {
                _imp->inputs[i].reset();
                _imp->guiInputs[i].reset();
            }
            if (i < (int) oldInputsVisibility.size()) {
                _imp->inputsVisibility[i] = oldInputsVisibility[i];
            } else {
                _imp->inputsVisibility[i] = true;
            }
        }


        ///Set the components the plug-in accepts
        _imp->inputsComponents.resize(inputCount);
        for (int i = 0; i < inputCount; ++i) {
            _imp->inputsComponents[i].clear();
            if ( _imp->effect->isInputMask(i) ) {
                //Force alpha for masks
                _imp->inputsComponents[i].push_back( ImagePlaneDesc::getAlphaComponents() );
            } else {
                _imp->effect->addAcceptedComponents(i, &_imp->inputsComponents[i]);
            }
        }
        _imp->outputComponents.clear();
        _imp->effect->addAcceptedComponents(-1, &_imp->outputComponents);
    }
    _imp->inputsInitialized = true;

    Q_EMIT inputsInitialized();
} // Node::initializeInputs


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

    NodePtr parent = _imp->multiInstanceParent.lock();
    if (parent) {
        return parent->hasInputConnected();
    }
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
    NodePtr parent = _imp->multiInstanceParent.lock();

    if (parent) {
        return parent->hasOutputConnected();
    }
    if ( isOutputNode() ) {
        return true;
    }
    if ( QThread::currentThread() == qApp->thread() ) {
        if (_imp->outputs.size() == 1) {
            NodePtr output = _imp->outputs.front().lock();

            return !( output->isTrackerNodePlugin() && output->isMultiInstance() );
        } else if (_imp->outputs.size() > 1) {
            return true;
        }
    } else {
        QMutexLocker l(&_imp->outputsMutex);
        if (_imp->outputs.size() == 1) {
            NodePtr output = _imp->outputs.front().lock();

            return !( output->isTrackerNodePlugin() && output->isMultiInstance() );
        } else if (_imp->outputs.size() > 1) {
            return true;
        }
    }

    return false;
}

bool
Node::checkIfConnectingInputIsOk(Node* input) const
{
    ////Only called by the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    if (!input || input == this) {
        return false;
    }
    bool found;
    input->isNodeUpstream(this, &found);

    return !found;
}

void
Node::isNodeUpstream(const Node* input,
                     bool* ok) const
{
    ////Only called by the main-thread
    assert( QThread::currentThread() == qApp->thread() );

    if (!input) {
        *ok = false;

        return;
    }

    ///No need to lock guiInputs is only written to by the main-thread

    for (U32 i = 0; i  < _imp->inputs.size(); ++i) {
        if (_imp->inputs[i].lock().get() == input) {
            *ok = true;

            return;
        }
    }
    *ok = false;
    for (U32 i = 0; i  < _imp->inputs.size(); ++i) {
        NodePtr in = _imp->inputs[i].lock();
        if (in) {
            in->isNodeUpstream(input, ok);
            if (*ok) {
                return;
            }
        }
    }
}



static Node::CanConnectInputReturnValue
checkCanConnectNoMultiRes(const Node* output,
                          const NodePtr& input)
{
    //http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#kOfxImageEffectPropSupportsMultiResolution
    //Check that the input has the same RoD that another input and that its rod is set to 0,0
    RenderScale scale(1.);
    RectD rod;
    bool isProjectFormat;
    StatusEnum stat = input->getEffectInstance()->getRegionOfDefinition_public(input->getHashValue(),
                                                                               output->getApp()->getTimeLine()->currentFrame(),
                                                                               scale,
                                                                               ViewIdx(0),
                                                                               &rod, &isProjectFormat);

    if ( (stat == eStatusFailed) && !rod.isNull() ) {
        return Node::eCanConnectInput_givenNodeNotConnectable;
    }
    if ( (rod.x1 != 0) || (rod.y1 != 0) ) {
        return Node::eCanConnectInput_multiResNotSupported;
    }

    // Commented-out: Some Furnace plug-ins from The Foundry (e.g F_Steadiness) are not supporting multi-resolution but actually produce an output
    // with a RoD different from the input

    /*RectD outputRod;
       stat = output->getEffectInstance()->getRegionOfDefinition_public(output->getHashValue(), output->getApp()->getTimeLine()->currentFrame(), scale, ViewIdx(0), &outputRod, &isProjectFormat);
       Q_UNUSED(stat);

       if ( !outputRod.isNull() && (rod != outputRod) ) {
        return Node::eCanConnectInput_multiResNotSupported;
       }*/

    for (int i = 0; i < output->getNInputs(); ++i) {
        NodePtr inputNode = output->getInput(i);
        if (inputNode) {
            RectD inputRod;
            stat = inputNode->getEffectInstance()->getRegionOfDefinition_public(inputNode->getHashValue(),
                                                                                output->getApp()->getTimeLine()->currentFrame(),
                                                                                scale,
                                                                                ViewIdx(0),
                                                                                &inputRod, &isProjectFormat);
            if ( (stat == eStatusFailed) && !inputRod.isNull() ) {
                return Node::eCanConnectInput_givenNodeNotConnectable;
            }
            if (inputRod != rod) {
                return Node::eCanConnectInput_multiResNotSupported;
            }
        }
    }

    return Node::eCanConnectInput_ok;
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
        if ( (inputNumber < 0) || ( inputNumber >= (int)_imp->guiInputs.size() ) ) {
            return eCanConnectInput_indexOutOfRange;
        }
        if ( _imp->guiInputs[inputNumber].lock() ) {
            return eCanConnectInput_inputAlreadyConnected;
        }
    }

    NodeGroup* isGrp = input->isEffectGroup();
    if ( isGrp && !isGrp->getOutputNode(true) ) {
        return eCanConnectInput_groupHasNoOutput;
    }

    if ( getParentMultiInstance() || input->getParentMultiInstance() ) {
        return eCanConnectInput_inputAlreadyConnected;
    }

    ///Applying this connection would create cycles in the graph
    if ( !checkIfConnectingInputIsOk( input.get() ) ) {
        return eCanConnectInput_graphCycles;
    }

    if ( _imp->effect->isInputRotoBrush(inputNumber) ) {
        qDebug() << "Debug: Attempt to connect " << input->getScriptName_mt_safe().c_str() << " to Roto brush";

        return eCanConnectInput_indexOutOfRange;
    }

    if ( !_imp->effect->supportsMultiResolution() ) {
        CanConnectInputReturnValue ret = checkCanConnectNoMultiRes(this, input);
        if (ret != eCanConnectInput_ok) {
            return ret;
        }
    }

    {
        ///Check for invalid pixel aspect ratio if the node doesn't support multiple clip PARs

        double inputPAR = input->getEffectInstance()->getAspectRatio(-1);
        double inputFPS = input->getEffectInstance()->getFrameRate();
        QMutexLocker l(&_imp->inputsMutex);

        for (InputsV::const_iterator it = _imp->guiInputs.begin(); it != _imp->guiInputs.end(); ++it) {
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
Node::connectInput(const NodePtr & input,
                   int inputNumber)
{
    assert(_imp->inputsInitialized);
    assert(input);

    ///Check for cycles: they are forbidden in the graph
    if ( !checkIfConnectingInputIsOk( input.get() ) ) {
        return false;
    }
    if ( _imp->effect->isInputRotoBrush(inputNumber) ) {
        qDebug() << "Debug: Attempt to connect " << input->getScriptName_mt_safe().c_str() << " to Roto brush";

        return false;
    }

    ///For effects that do not support multi-resolution, make sure the input effect is correct
    ///otherwise the rendering might crash
    if ( !_imp->effect->supportsMultiResolution() && !getApp()->getProject()->isLoadingProject() ) {
        CanConnectInputReturnValue ret = checkCanConnectNoMultiRes(this, input);
        if (ret != eCanConnectInput_ok) {
            return false;
        }
    }

    bool useGuiInputs = isNodeRendering();
    _imp->effect->abortAnyEvaluation();

    {
        ///Check for invalid index
        QMutexLocker l(&_imp->inputsMutex);
        if ( (inputNumber < 0) ||
             ( inputNumber >= (int)_imp->inputs.size() ) ||
             ( !useGuiInputs && _imp->inputs[inputNumber].lock() ) ||
             ( useGuiInputs && _imp->guiInputs[inputNumber].lock() ) ) {
            return false;
        }

        ///Set the input
        if (!useGuiInputs) {
            _imp->inputs[inputNumber] = input;
            _imp->guiInputs[inputNumber] = input;
        } else {
            _imp->guiInputs[inputNumber] = input;
        }
        input->connectOutput( useGuiInputs, shared_from_this() );
    }

    getApp()->recheckInvalidExpressions();

    ///Get notified when the input name has changed
    QObject::connect( input.get(), SIGNAL(labelChanged(QString)), this, SLOT(onInputLabelChanged(QString)) );

    ///Notify the GUI
    Q_EMIT inputChanged(inputNumber);
    bool mustCallEnd = false;

    if (!useGuiInputs) {
        ///Call the instance changed action with a reason clip changed
        beginInputEdition();
        mustCallEnd = true;
        onInputChanged(inputNumber);
    }

    bool creatingNodeTree = getApp()->isCreatingNodeTree();
    if (!creatingNodeTree) {
        ///Recompute the hash
        computeHash();
    }

    _imp->ifGroupForceHashChangeOfInputs();

    std::string inputChangedCB = getInputChangedCallback();
    if ( !inputChangedCB.empty() ) {
        _imp->runInputChangedCallback(inputNumber, inputChangedCB);
    }

    if (mustCallEnd) {
        endInputEdition(true);
    }

    return true;
} // Node::connectInput



void
Node::Implementation::ifGroupForceHashChangeOfInputs()
{
    ///If the node is a group, force a change of the outputs of the GroupInput nodes so the hash of the tree changes downstream
    NodeGroup* isGrp = dynamic_cast<NodeGroup*>( effect.get() );

    if ( isGrp && !isGrp->getApp()->isCreatingNodeTree() ) {
        NodesList inputsOutputs;
        isGrp->getInputsOutputs(&inputsOutputs, false);
        for (NodesList::iterator it = inputsOutputs.begin(); it != inputsOutputs.end(); ++it) {
            (*it)->incrementKnobsAge_internal();
            (*it)->computeHash();
        }
    }
}

bool
Node::replaceInputInternal(const NodePtr& input, int inputNumber, bool useGuiInputs)
{
    assert(_imp->inputsInitialized);
    assert(input);
    ///Check for cycles: they are forbidden in the graph
    if ( !checkIfConnectingInputIsOk( input.get() ) ) {
        return false;
    }
    if ( _imp->effect->isInputRotoBrush(inputNumber) ) {
        qDebug() << "Debug: Attempt to connect " << input->getScriptName_mt_safe().c_str() << " to Roto brush";

        return false;
    }

    ///For effects that do not support multi-resolution, make sure the input effect is correct
    ///otherwise the rendering might crash
    if ( !_imp->effect->supportsMultiResolution() ) {
        CanConnectInputReturnValue ret = checkCanConnectNoMultiRes(this, input);
        if (ret != eCanConnectInput_ok) {
            return false;
        }
    }

    {
        ///Check for invalid index
        QMutexLocker l(&_imp->inputsMutex);
        if ( (inputNumber < 0) || ( inputNumber > (int)_imp->inputs.size() ) ) {
            return false;
        }
    }

    {
        QMutexLocker l(&_imp->inputsMutex);
        ///Set the input

        if (!useGuiInputs) {
            NodePtr curIn = _imp->inputs[inputNumber].lock();
            if (curIn) {
                QObject::connect( curIn.get(), SIGNAL(labelChanged(QString)), this, SLOT(onInputLabelChanged(QString)) );
                curIn->disconnectOutput(useGuiInputs, this);
            }
            _imp->inputs[inputNumber] = input;
            _imp->guiInputs[inputNumber] = input;
        } else {
            NodePtr curIn = _imp->guiInputs[inputNumber].lock();
            if (curIn) {
                QObject::connect( curIn.get(), SIGNAL(labelChanged(QString)), this, SLOT(onInputLabelChanged(QString)) );
                curIn->disconnectOutput(useGuiInputs, this);
            }
            _imp->guiInputs[inputNumber] = input;
        }
        input->connectOutput( useGuiInputs, shared_from_this() );
    }

    ///Get notified when the input name has changed
    QObject::connect( input.get(), SIGNAL(labelChanged(QString)), this, SLOT(onInputLabelChanged(QString)) );

    ///Notify the GUI
    Q_EMIT inputChanged(inputNumber);
    bool mustCallEnd = false;
    if (!useGuiInputs) {
        beginInputEdition();
        mustCallEnd = true;
        ///Call the instance changed action with a reason clip changed
        onInputChanged(inputNumber);
    }

    bool creatingNodeTree = getApp()->isCreatingNodeTree();
    if (!creatingNodeTree) {
        ///Recompute the hash
        computeHash();
    }

    _imp->ifGroupForceHashChangeOfInputs();

    std::string inputChangedCB = getInputChangedCallback();
    if ( !inputChangedCB.empty() ) {
        _imp->runInputChangedCallback(inputNumber, inputChangedCB);
    }

    if (mustCallEnd) {
        endInputEdition(true);
    }

    return true;
}

bool
Node::replaceInput(const NodePtr& input,
                   int inputNumber)
{


    bool useGuiInputs = isNodeRendering();
    _imp->effect->abortAnyEvaluation();
    return replaceInputInternal(input, inputNumber, useGuiInputs);
} // Node::replaceInput

void
Node::switchInput0And1()
{
    assert(_imp->inputsInitialized);
    int maxInputs = getNInputs();
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

    bool useGuiInputs = isNodeRendering();
    _imp->effect->abortAnyEvaluation();

    {
        QMutexLocker l(&_imp->inputsMutex);
        assert( inputAIndex < (int)_imp->inputs.size() && inputBIndex < (int)_imp->inputs.size() );
        NodePtr input0;

        if (!useGuiInputs) {
            input0 = _imp->inputs[inputAIndex].lock();
            _imp->inputs[inputAIndex] = _imp->inputs[inputBIndex];
            _imp->inputs[inputBIndex] = input0;
            _imp->guiInputs[inputAIndex] = _imp->inputs[inputAIndex];
            _imp->guiInputs[inputBIndex] = _imp->inputs[inputBIndex];
        } else {
            input0 = _imp->guiInputs[inputAIndex].lock();
            _imp->guiInputs[inputAIndex] = _imp->guiInputs[inputBIndex];
            _imp->guiInputs[inputBIndex] = input0;
        }
    }
    Q_EMIT inputChanged(inputAIndex);
    Q_EMIT inputChanged(inputBIndex);
    bool mustCallEnd = false;
    if (!useGuiInputs) {
        beginInputEdition();
        mustCallEnd = true;
        onInputChanged(inputAIndex);
        onInputChanged(inputBIndex);
    }
    bool creatingNodeTree = getApp()->isCreatingNodeTree();
    if (!creatingNodeTree) {
        ///Recompute the hash
        computeHash();
    }

    std::string inputChangedCB = getInputChangedCallback();
    if ( !inputChangedCB.empty() ) {
        _imp->runInputChangedCallback(inputAIndex, inputChangedCB);
        _imp->runInputChangedCallback(inputBIndex, inputChangedCB);
    }


    _imp->ifGroupForceHashChangeOfInputs();

    if (mustCallEnd) {
        endInputEdition(true);
    }
} // switchInput0And1



void
Node::onInputLabelChanged(const QString & name)
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

    for (U32 i = 0; i < _imp->guiInputs.size(); ++i) {
        if (_imp->guiInputs[i].lock().get() == inp) {
            inputNb = i;
            break;
        }
    }

    if (inputNb != -1) {
        Q_EMIT inputLabelChanged(inputNb, name);
    }
}

void
Node::connectOutput(bool useGuiValues,
                    const NodePtr& output)
{
    assert(output);

    {
        QMutexLocker l(&_imp->outputsMutex);
        if (!useGuiValues) {
            _imp->outputs.push_back(output);
            _imp->guiOutputs.push_back(output);
        } else {
            _imp->guiOutputs.push_back(output);
        }
    }
    Q_EMIT outputsChanged();
}

int
Node::disconnectInput(int inputNumber)
{
    assert(_imp->inputsInitialized);

    NodePtr inputShared;
    bool useGuiValues = isNodeRendering();
    bool destroyed;
    {
        QMutexLocker k(&_imp->isBeingDestroyedMutex);
        destroyed = _imp->isBeingDestroyed;
    }
    if (!destroyed) {
        _imp->effect->abortAnyEvaluation();
    }

    {
        QMutexLocker l(&_imp->inputsMutex);
        if ( (inputNumber < 0) ||
             ( inputNumber > (int)_imp->inputs.size() ) ||
             ( !useGuiValues && !_imp->inputs[inputNumber].lock() ) ||
             ( useGuiValues && !_imp->guiInputs[inputNumber].lock() ) ) {
            return -1;
        }
        inputShared = useGuiValues ? _imp->guiInputs[inputNumber].lock() : _imp->inputs[inputNumber].lock();
    }


    QObject::disconnect( inputShared.get(), SIGNAL(labelChanged(QString)), this, SLOT(onInputLabelChanged(QString)) );
    inputShared->disconnectOutput(useGuiValues, this);

    {
        QMutexLocker l(&_imp->inputsMutex);
        if (!useGuiValues) {
            _imp->inputs[inputNumber].reset();
            _imp->guiInputs[inputNumber].reset();
        } else {
            _imp->guiInputs[inputNumber].reset();
        }
    }

    {
        QMutexLocker k(&_imp->isBeingDestroyedMutex);
        if (_imp->isBeingDestroyed) {
            return -1;
        }
    }

    Q_EMIT inputChanged(inputNumber);
    bool mustCallEnd = false;
    if (!useGuiValues) {
        beginInputEdition();
        mustCallEnd = true;
        onInputChanged(inputNumber);
    }
    bool creatingNodeTree = getApp()->isCreatingNodeTree();
    if (!creatingNodeTree) {
        ///Recompute the hash
        computeHash();
    }

    _imp->ifGroupForceHashChangeOfInputs();

    std::string inputChangedCB = getInputChangedCallback();
    if ( !inputChangedCB.empty() ) {
        _imp->runInputChangedCallback(inputNumber, inputChangedCB);
    }
    if (mustCallEnd) {
        endInputEdition(true);
    }

    return inputNumber;
} // Node::disconnectInput

int
Node::disconnectInputInternal(Node* input, bool useGuiValues)
{
    assert(_imp->inputsInitialized);
    int found = -1;
    NodePtr inputShared;
    {
        QMutexLocker l(&_imp->inputsMutex);
        if (!useGuiValues) {
            for (std::size_t i = 0; i < _imp->inputs.size(); ++i) {
                NodePtr curInput = _imp->inputs[i].lock();
                if (curInput.get() == input) {
                    inputShared = curInput;
                    found = (int)i;
                    break;
                }
            }
        } else {
            for (std::size_t i = 0; i < _imp->guiInputs.size(); ++i) {
                NodePtr curInput = _imp->guiInputs[i].lock();
                if (curInput.get() == input) {
                    inputShared = curInput;
                    found = (int)i;
                    break;
                }
            }
        }
    }
    if (found != -1) {
        {
            QMutexLocker l(&_imp->inputsMutex);
            if (!useGuiValues) {
                _imp->inputs[found].reset();
                _imp->guiInputs[found].reset();
            } else {
                _imp->guiInputs[found].reset();
            }
        }
        input->disconnectOutput(useGuiValues, this);
        Q_EMIT inputChanged(found);
        bool mustCallEnd = false;
        if (!useGuiValues) {
            beginInputEdition();
            mustCallEnd = true;
            onInputChanged(found);
        }
        bool creatingNodeTree = getApp()->isCreatingNodeTree();
        if (!creatingNodeTree) {
            ///Recompute the hash
            if ( !getApp()->getProject()->isProjectClosing() ) {
                computeHash();
            }
        }


        _imp->ifGroupForceHashChangeOfInputs();

        std::string inputChangedCB = getInputChangedCallback();
        if ( !inputChangedCB.empty() ) {
            _imp->runInputChangedCallback(found, inputChangedCB);
        }

        if (mustCallEnd) {
            endInputEdition(true);
        }

        return found;
    }

    return -1;
}

int
Node::disconnectInput(Node* input)
{

    bool useGuiValues = isNodeRendering();
    _imp->effect->abortAnyEvaluation();
    return disconnectInputInternal(input, useGuiValues);
} // Node::disconnectInput

int
Node::disconnectOutput(bool useGuiValues,
                       const Node* output)
{
    assert(output);
    int ret = -1;
    {
        QMutexLocker l(&_imp->outputsMutex);
        if (!useGuiValues) {
            int ret = 0;
            for (NodesWList::iterator it = _imp->outputs.begin(); it != _imp->outputs.end(); ++it, ++ret) {
                if (it->lock().get() == output) {
                    _imp->outputs.erase(it);
                    break;
                }
            }
        }
        int ret = 0;
        for (NodesWList::iterator it = _imp->guiOutputs.begin(); it != _imp->guiOutputs.end(); ++it, ++ret) {
            if (it->lock().get() == output) {
                _imp->guiOutputs.erase(it);
                break;
            }
        }
    }

    //Will just refresh the gui
    Q_EMIT outputsChanged();

    return ret;
}

int
Node::inputIndex(const NodePtr& n) const
{
    if (!n) {
        return -1;
    }

    ///Only called by the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    assert(_imp->inputsInitialized);

    NodePtr parent = _imp->multiInstanceParent.lock();
    if (parent) {
        return parent->inputIndex(n);
    }

    ///No need to lock this is only called by the main-thread
    for (std::size_t i = 0; i < _imp->inputs.size(); ++i) {
        if (_imp->inputs[i].lock() == n) {
            return i;
        }
    }


    return -1;
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
Node::getOutputsConnectedToThisNode(std::map<NodePtr, int>* outputs)
{
    ////Only called by the main-thread
    assert( QThread::currentThread() == qApp->thread() );

    NodePtr thisSHared = shared_from_this();
    for (NodesWList::iterator it = _imp->outputs.begin(); it != _imp->outputs.end(); ++it) {
        NodePtr output = it->lock();
        if (!output) {
            continue;
        }

        int indexOfThis = output->inputIndex(thisSHared);
        assert(indexOfThis != -1);
        if (indexOfThis >= 0) {
            outputs->insert( std::make_pair(output, indexOfThis) );
        }
    }
}


const NodesWList &
Node::getOutputs() const
{
    ////Only called by the main-thread
    assert( QThread::currentThread() == qApp->thread() );

    return _imp->outputs;
}

const NodesWList &
Node::getGuiOutputs() const
{
    ////Only called by the main-thread
    assert( QThread::currentThread() == qApp->thread() );

    return _imp->guiOutputs;
}

void
Node::getOutputs_mt_safe(NodesWList& outputs) const
{
    QMutexLocker l(&_imp->outputsMutex);

    outputs =  _imp->outputs;
}

void
Node::getInputNames(std::map<std::string, std::string> & inputNames) const
{
    ///This is called by the serialization thread.
    ///We use the guiInputs because we want to serialize exactly how the tree was to the user

    NodePtr parent = _imp->multiInstanceParent.lock();

    if (parent) {
        parent->getInputNames(inputNames);

        return;
    }

    QMutexLocker l(&_imp->inputsLabelsMutex);
    assert( _imp->inputs.size() == _imp->inputLabels.size() );
    for (std::size_t i = 0; i < _imp->inputs.size(); ++i) {
        NodePtr input = _imp->inputs[i].lock();
        if (input) {
            inputNames.insert( std::make_pair( _imp->inputLabels[i], input->getScriptName_mt_safe() ) );
        }
    }
}

int
Node::getPreferredInputInternal(bool connected) const
{
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
        if ( _imp->effect->isInputRotoBrush(i) ) {
            continue;
        }
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
            while ( first != optionalEmptyInputs.end() && _imp->effect->isInputRotoBrush(*first) ) {
                ++first;
            }
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
    GroupInput* isInput = dynamic_cast<GroupInput*>( _imp->effect.get() );
    PrecompNode* isPrecomp = dynamic_cast<PrecompNode*>( _imp->effect.get() );

    if (isInput) {
        NodeGroup* isGroup = dynamic_cast<NodeGroup*>( getGroup().get() );
        assert(isGroup);
        if (!isGroup) {
            return NodePtr();
        }
        int inputNb = -1;
        std::vector<NodePtr> groupInputs;
        isGroup->getInputs(&groupInputs, false);
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
    } else if (isPrecomp) {
        return isPrecomp->getOutputNode();
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
            if ( !getApp()->isCreatingNodeTree() ) {
                forceRefreshAllInputRelatedData();
            }
            refreshDynamicProperties();
        }

        triggerRender = triggerRender && hasChanged;

        if (triggerRender) {
            std::list<ViewerInstance* > viewers;
            hasViewersConnected(&viewers);
            for (std::list<ViewerInstance* >::iterator it2 = viewers.begin(); it2 != viewers.end(); ++it2) {
                (*it2)->renderCurrentFrame(true);
            }
        }
    }
}

void
Node::onInputChanged(int inputNb,
                     bool isInputA)
{
    if ( getApp()->getProject()->isProjectClosing() ) {
        return;
    }
    assert( QThread::currentThread() == qApp->thread() );

    bool mustCallEndInputEdition = _imp->inputModifiedRecursion == 0;
    if (mustCallEndInputEdition) {
        beginInputEdition();
    }

    refreshMaskEnabledNess(inputNb);
    refreshLayersChoiceSecretness(inputNb);

    InspectorNode* isInspector = dynamic_cast<InspectorNode*>(this);
    if (isInspector) {
        isInspector->refreshActiveInputs(inputNb, isInputA);
    }

    bool shouldDoInputChanged = ( !getApp()->getProject()->isProjectClosing() && !getApp()->isCreatingNodeTree() ) ||
                                _imp->effect->isRotoPaintNode();

    if (shouldDoInputChanged) {
        ///When loading a group (or project) just wait until everything is setup to actually compute input
        ///related data such as clip preferences
        ///Exception for the Rotopaint node which needs to setup its own graph internally

        /**
         * The plug-in might call getImage, set a valid thread storage on the tree.
         **/
        double time = getApp()->getTimeLine()->currentFrame();
        AbortableRenderInfoPtr abortInfo = AbortableRenderInfo::create(false, 0);
        const bool isRenderUserInteraction = true;
        const bool isSequentialRender = false;
        AbortableThread* isAbortable = dynamic_cast<AbortableThread*>( QThread::currentThread() );
        if (isAbortable) {
            isAbortable->setAbortInfo( isRenderUserInteraction, abortInfo, getEffectInstance() );
        }
        ParallelRenderArgsSetter frameRenderArgs( time,
                                                  ViewIdx(0),
                                                  isRenderUserInteraction,
                                                  isSequentialRender,
                                                  abortInfo,
                                                  shared_from_this(),
                                                  0, //texture index
                                                  getApp()->getTimeLine().get(),
                                                  NodePtr(),
                                                  false,
                                                  false,
                                                  RenderStatsPtr() );


        ///Don't do clip preferences while loading a project, they will be refreshed globally once the project is loaded.
        _imp->effect->onInputChanged(inputNb);
        _imp->inputsModified.insert(inputNb);

        // If the effect has render clones, kill them as the plug-in might have changed its internal state
        _imp->effect->clearRenderInstances();

        //A knob value might have changed recursively, redraw  any overlay
        if ( !_imp->effect->isDequeueingValuesSet() &&
             ( _imp->effect->getRecursionLevel() == 0) && _imp->effect->checkIfOverlayRedrawNeeded() ) {
            _imp->effect->redrawOverlayInteract();
        }
    }

    /*
       If this is a group, also notify the output nodes of the GroupInput node inside the Group corresponding to
       the this inputNb
     */
    NodeGroup* isGroup = dynamic_cast<NodeGroup*>( _imp->effect.get() );
    if (isGroup) {
        std::vector<NodePtr> groupInputs;
        isGroup->getInputs(&groupInputs, false);
        if ( (inputNb >= 0) && ( inputNb < (int)groupInputs.size() ) && groupInputs[inputNb] ) {
            std::map<NodePtr, int> inputOutputs;
            groupInputs[inputNb]->getOutputsConnectedToThisNode(&inputOutputs);
            for (std::map<NodePtr, int> ::iterator it = inputOutputs.begin(); it != inputOutputs.end(); ++it) {
                it->first->onInputChanged(it->second);
            }
        }
    }

    /*
       If this is an output node, notify the Group output nodes that their input have changed.
     */
    GroupOutput* isOutput = dynamic_cast<GroupOutput*>( _imp->effect.get() );
    if (isOutput) {
        NodeGroup* containerGroup = dynamic_cast<NodeGroup*>( isOutput->getNode()->getGroup().get() );
        if (containerGroup) {
            std::map<NodePtr, int> groupOutputs;
            containerGroup->getNode()->getOutputsConnectedToThisNode(&groupOutputs);
            for (std::map<NodePtr, int> ::iterator it = groupOutputs.begin(); it != groupOutputs.end(); ++it) {
                it->first->onInputChanged(it->second);
            }
        }
    }

    /*
     * If this node is the output of a pre-comp, notify the precomp output nodes that their input have changed
     */
    PrecompNodePtr isInPrecomp = isPartOfPrecomp();
    if ( isInPrecomp && (isInPrecomp->getOutputNode().get() == this) ) {
        std::map<NodePtr, int> inputOutputs;
        isInPrecomp->getNode()->getOutputsConnectedToThisNode(&inputOutputs);
        for (std::map<NodePtr, int> ::iterator it = inputOutputs.begin(); it != inputOutputs.end(); ++it) {
            it->first->onInputChanged(it->second);
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

//////////////////////////////////

InspectorNode::InspectorNode(const AppInstancePtr& app,
                             const NodeCollectionPtr& group,
                             Plugin* plugin)
    : Node(app, group, plugin)
{
    for (int i = 0; i < 2; ++i) {
        _activeInputs[i] = -1;
    }
}

InspectorNode::~InspectorNode()
{
}

bool
InspectorNode::connectInput(const NodePtr& input,
                            int inputNumber)
{
    ///Only called by the main-thread
    assert( QThread::currentThread() == qApp->thread() );

    if ( !isEffectViewer() ) {
        return connectInputBase(input, inputNumber);
    }

    ///cannot connect more than _maxInputs inputs.
    assert( inputNumber <= getNInputs() );

    assert(input);

    if ( !checkIfConnectingInputIsOk( input.get() ) ) {
        return false;
    }

    ///For effects that do not support multi-resolution, make sure the input effect is correct
    ///otherwise the rendering might crash
    if ( !getEffectInstance()->supportsMultiResolution() ) {
        CanConnectInputReturnValue ret = checkCanConnectNoMultiRes(this, input);
        if (ret != eCanConnectInput_ok) {
            return false;
        }
    }

    ///If the node 'input' is already to an input of the inspector, find it.
    ///If it has the same input number as what we want just return, otherwise
    ///disconnect it and continue as usual.
    int inputAlreadyConnected = inputIndex(input);
    if (inputAlreadyConnected != -1) {
        if (inputAlreadyConnected == inputNumber) {
            return false;
        } else {
            disconnectInput(inputAlreadyConnected);
        }
    }

    if ( !Node::connectInput(input, inputNumber) ) {
        bool creatingNodeTree = getApp()->isCreatingNodeTree();
        if (!creatingNodeTree) {
            ///Recompute the hash
            computeHash();
        }
    }

    return true;
}

void
InspectorNode::setActiveInputAndRefresh(int inputNb,
                                        bool isASide)
{
    assert( QThread::currentThread() == qApp->thread() );

    int maxInputs = getNInputs();
    if ( ( inputNb > (maxInputs - 1) ) || (inputNb < 0) || ( !getInput(inputNb) ) ) {
        return;
    }

    bool creatingNodeTree = getApp()->isCreatingNodeTree();
    if (!creatingNodeTree) {
        ///Recompute the hash
        computeHash();
    }

    Q_EMIT inputChanged(inputNb);
    onInputChanged(inputNb, isASide);

    runInputChangedCallback(inputNb);


    if ( isOutputNode() ) {
        OutputEffectInstance* oei = dynamic_cast<OutputEffectInstance*>( getEffectInstance().get() );
        assert(oei);
        if (oei) {
            oei->renderCurrentFrame(true);
        }
    }
}

void
InspectorNode::refreshActiveInputs(int inputNbChanged,
                                   bool isASide)
{
    assert( QThread::currentThread() == qApp->thread() );
    NodePtr inputNode = getRealInput(inputNbChanged);
    {
        QMutexLocker l(&_activeInputsMutex);
        if (!inputNode) {
            ///check if the input was one of the active ones if so set to -1
            if (_activeInputs[0] == inputNbChanged) {
                _activeInputs[0] = -1;
            } else if (_activeInputs[1] == inputNbChanged) {
                _activeInputs[1] = -1;
            }
        } else {
            if ( !isASide && (_activeInputs[0] != -1) ) {
                ViewerInstance* isViewer = isEffectViewer();
                if (isViewer) {
                    OpenGLViewerI* viewerUI = isViewer->getUiContext();
                    if (viewerUI) {
                        ViewerCompositingOperatorEnum op = viewerUI->getCompositingOperator();
                        if (op == eViewerCompositingOperatorNone) {
                            viewerUI->setCompositingOperator(eViewerCompositingOperatorWipeUnder);
                        }
                    }
                }
                _activeInputs[1] = inputNbChanged;
            } else {
                _activeInputs[0] = inputNbChanged;
                if (_activeInputs[1] == -1) {
                    _activeInputs[1] = inputNbChanged;
                }
            }
        }
    }
    Q_EMIT activeInputsChanged();
    Q_EMIT refreshOptionalState();
}

int
InspectorNode::getPreferredInputInternal(bool connected) const
{
    bool useInputA = false;
    if (!connected) {
        // For the merge node, use the preference (only when not connected)
        useInputA = appPTR->getCurrentSettings()->useInputAForMergeAutoConnect();
    }

    ///Find an input named A
    std::string inputNameToFind, otherName;

    if ( useInputA || (getPluginID() == PLUGINID_OFX_SHUFFLE && getMajorVersion() < 3) ) {
        inputNameToFind = "A";
        otherName = "B";
    } else {
        inputNameToFind = "B";
        otherName = "A";
    }
    int foundOther = -1;
    int maxinputs = getNInputs();
    for (int i = 0; i < maxinputs; ++i) {
        std::string inputLabel = getInputLabel(i);
        if (inputLabel == inputNameToFind) {
            NodePtr inp = getInput(i);
            if ( (connected && inp) || (!connected && !inp) ) {
                return i;
            }
        } else if (inputLabel == otherName) {
            foundOther = i;
        }
    }
    if (foundOther != -1) {
        NodePtr inp = getInput(foundOther);
        if ( (connected && inp) || (!connected && !inp) ) {
            return foundOther;
        }
    }

    int maxInputs = getNInputs();
    for (int i = 0; i < maxInputs; ++i) {
        NodePtr inp = getInput(i);
        if ( (!inp && !connected) || (inp && connected) ) {
            return i;
        }
    }

    return -1;
}

int
InspectorNode::getPreferredInput() const
{
    return getPreferredInputInternal(true);
}

int
InspectorNode::getPreferredInputForConnection() const
{
    return getPreferredInputInternal(false);
}

void
InspectorNode::getActiveInputs(int & a,
                               int &b) const
{
    QMutexLocker l(&_activeInputsMutex);

    a = _activeInputs[0];
    b = _activeInputs[1];
}

void
InspectorNode::setInputA(int inputNb)
{
    assert( QThread::currentThread() == qApp->thread() );
    {
        QMutexLocker l(&_activeInputsMutex);
        _activeInputs[0] = inputNb;
    }
    Q_EMIT refreshOptionalState();
}

void
InspectorNode::setInputB(int inputNb)
{
    assert( QThread::currentThread() == qApp->thread() );
    {
        QMutexLocker l(&_activeInputsMutex);
        _activeInputs[1] = inputNb;
    }
    Q_EMIT refreshOptionalState();
}

NATRON_NAMESPACE_EXIT
