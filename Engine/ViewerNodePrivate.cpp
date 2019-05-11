/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
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



#include "ViewerNodePrivate.h"

#include "Engine/RenderEngine.h"

NATRON_NAMESPACE_ENTER


/**
 * @brief Cycles recursively upstream thtough the main input of each node until we reach an Input node, or nothing in the
 * sub-graph of the Viewer ndoe
 **/
NodePtr
ViewerNodePrivate::getMainInputRecursiveInternal(const NodePtr& inputParam) const
{

    int prefIndex = inputParam->getPreferredInput();
    if (prefIndex == -1) {
        return NodePtr();
    }
    NodePtr input = inputParam->getRealInput(prefIndex);
    if (!input) {
        return inputParam;
    }
    GroupInput* isInput = dynamic_cast<GroupInput*>(input->getEffectInstance().get());
    if (isInput) {
        return inputParam;
    }
    NodePtr inputRet = getMainInputRecursiveInternal(input);
    if (!inputRet) {
        return input;
    } else {
        return inputRet;
    }

}


/**
 * @brief Returns the last node connected to a GroupInput node following the main-input branch of the graph
 **/
NodePtr
ViewerNodePrivate::getInputRecursive(int inputIndex) const
{
    NodePtr viewerProcess = internalViewerProcessNode[inputIndex].lock();
    if (!viewerProcess) {
        return NodePtr();
    }
    NodePtr input = viewerProcess->getRealInput(inputIndex);
    if (!input) {
        return viewerProcess;
    }
    GroupInput* isInput = dynamic_cast<GroupInput*>(input->getEffectInstance().get());
    if (isInput) {
        return viewerProcess;
    }
    NodePtr inputRet = getMainInputRecursiveInternal(input);
    if (!inputRet) {
        return input;
    } else {
        return inputRet;
    }
}


void
ViewerNodePrivate::refreshInputChoices(bool resetChoiceIfNotFound)
{
    // Refresh the A and B input choices
    KnobChoicePtr aInputKnob = aInputNodeChoiceKnob.lock();
    KnobChoicePtr bInputKnob = bInputNodeChoiceKnob.lock();

    ViewerCompositingOperatorEnum operation = (ViewerCompositingOperatorEnum)blendingModeChoiceKnob.lock()->getValue();
    bInputKnob->setEnabled(operation != eViewerCompositingOperatorNone);

    std::vector<ChoiceOption> entries;
    entries.push_back(ChoiceOption("-", "", ""));

    int nInputs = _publicInterface->getNInputs();
    for (int i = 0; i < nInputs; ++i) {
        NodePtr inputNode = _publicInterface->getNode()->getRealInput(i);
        if (!inputNode) {
            continue;
        }

        std::string optionID;
        {
            std::stringstream ss;
            ss << i;
            optionID = ss.str();
        }

        entries.push_back(ChoiceOption(optionID, inputNode->getLabel(), ""));
    }

    ChoiceOption currentAChoice = aInputKnob->getCurrentEntry();
    ChoiceOption currentBChoice = bInputKnob->getCurrentEntry();

    aInputKnob->populateChoices(entries);
    bInputKnob->populateChoices(entries);

    if (resetChoiceIfNotFound) {
        if (currentAChoice.id == "-" || !aInputKnob->isActiveEntryPresentInEntries(ViewIdx(0))) {
            aInputKnob->setValue(entries.size() > 1 ? 1 : 0);
        }
        if (currentBChoice.id == "-" || !bInputKnob->isActiveEntryPresentInEntries(ViewIdx(0))) {
            bInputKnob->setValue(entries.size() > 1 ? 1 : 0);
        }
    }

    if (uiContext) {
        if ( (operation == eViewerCompositingOperatorNone) || !bInputKnob->isEnabled()  || bInputKnob->getCurrentEntry().id.empty() ) {
            uiContext->setInfoBarVisible(1, false);
        } else if (operation != eViewerCompositingOperatorNone) {
            uiContext->setInfoBarVisible(1, true);
        }
    }
    
} // refreshInputChoices


void
ViewerNodePrivate::refreshInputChoiceMenu(int internalIndex, int groupInputIndex)
{
    KnobChoicePtr inputChoiceKnob = internalIndex == 0 ? aInputNodeChoiceKnob.lock() : bInputNodeChoiceKnob.lock();

    assert(groupInputIndex >= 0 && groupInputIndex < _publicInterface->getNInputs());

    std::string groupInputID;
    {
        std::stringstream ss;
        ss << groupInputIndex;
        groupInputID = ss.str();
    }
    std::vector<ChoiceOption> entries = inputChoiceKnob->getEntries();
    for (std::size_t i = 0; i < entries.size(); ++i) {
        if (entries[i].id == groupInputID) {
            inputChoiceKnob->setValue(i);
            return;
        }
    }

    // Input is no longer connected fallback on first input in the list if any, otherwise on None ("-")

    inputChoiceKnob->setValue(entries.size() > 1 ? 1 : 0, ViewSetSpec::all(), DimIdx(0), eValueChangedReasonPluginEdited);


}


void
ViewerNodePrivate::toggleDownscaleLevel(int index)
{
    assert(index > 0);
    KnobChoicePtr downscaleChoice = downscaleChoiceKnob.lock();
    int curChoice_i = downscaleChoice->getValue();
    if (curChoice_i != index) {
        downscaleChoice->setValue(index);
    } else {
        // Reset back to auto
        downscaleChoice->setValue(0);
    }
}

void
ViewerNodePrivate::getAllViewerNodes(bool inGroupOnly, std::list<ViewerNodePtr>& viewerNodes) const
{
    NodeCollectionPtr collection;
    if (inGroupOnly) {
        collection = _publicInterface->getNode()->getGroup();
    } else {
        collection = _publicInterface->getApp()->getProject();
    }
    NodesList nodes;
    if (inGroupOnly) {
        nodes = collection->getNodes();
    } else {
        collection->getNodes_recursive(nodes);
    }
    for (NodesList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        ViewerNodePtr isViewer = (*it)->isEffectViewerNode();
        if (isViewer) {
            viewerNodes.push_back(isViewer);
        }
    }
}

void
ViewerNodePrivate::abortAllViewersRendering()
{
    std::list<ViewerNodePtr> viewers;
    getAllViewerNodes(false, viewers);

    playForwardButtonKnob.lock()->setValue(false, ViewSetSpec::all(), DimIdx(0), eValueChangedReasonPluginEdited);
    playBackwardButtonKnob.lock()->setValue(false, ViewSetSpec::all(), DimIdx(0), eValueChangedReasonPluginEdited);


    if ( _publicInterface->getApp()->isGuiFrozen() && appPTR->getCurrentSettings()->isAutoTurboEnabled() ) {
        _publicInterface->getApp()->setGuiFrozen(false);
    }

    // Abort all viewers because they are all synchronised.

    for (std::list<ViewerNodePtr>::const_iterator it = viewers.begin(); it != viewers.end(); ++it) {
        (*it)->getNode()->getRenderEngine()->abortRenderingNoRestart();
    }
}

void
ViewerNodePrivate::timelineGoTo(TimeValue time)
{
    _publicInterface->getTimeline()->seekFrame(time, true, _publicInterface->shared_from_this(), eTimelineChangeReasonOtherSeek);
}


void
ViewerNodePrivate::setAlphaChannelFromLayerIfRGBA()
{

    ImagePlaneDesc selectedLayer, selectedAlphaLayer, selectedDisplayLayer;
    int alphaChannelIndex;
    ViewerInstancePtr internalViewer = internalViewerProcessNode[0].lock()->isEffectViewerInstance();
    internalViewer->getChannelOptions(_publicInterface->getTimelineCurrentTime(), &selectedLayer, &selectedAlphaLayer, &alphaChannelIndex, &selectedDisplayLayer);

    // Set the alpha channel to the selected layer's alpha channel if it is not pointing to anything
    if (selectedLayer.getNumComponents() == 4 && selectedAlphaLayer.getNumComponents() == 0) {

        ChoiceOption newOption = selectedLayer.getChannelOption(3);
        KnobChoicePtr alphaChoice = alphaChannelKnob.lock();
        std::vector<ChoiceOption> options = alphaChoice->getEntries();
        for (std::size_t i = 0; i < options.size(); ++i) {
            if (options[i].id == newOption.id) {
                alphaChoice->setValue(i);
                break;
            }
        }
    }

}

void
ViewerNodePrivate::swapViewerProcessInputs()
{
    NodePtr viewerProcessNodesInputs[2];
    for (int i = 0; i < 2; ++i) {
        viewerProcessNodesInputs[i] = getInputRecursive(i);
    }

    NodePtr input0 = viewerProcessNodesInputs[0]->getInput(0);
    NodePtr input1 = viewerProcessNodesInputs[1]->getInput(0);
    viewerProcessNodesInputs[0]->swapInput(input1, 0);
    viewerProcessNodesInputs[1]->swapInput(input0, 0);

    try {
        KnobChoicePtr aChoice = aInputNodeChoiceKnob.lock();
        KnobChoicePtr bChoice = bInputNodeChoiceKnob.lock();
        ChoiceOption aCurChoice = aChoice->getCurrentEntry();
        ChoiceOption bCurChoice = bChoice->getCurrentEntry();
        aChoice->blockValueChanges();
        aChoice->setValueFromID(bCurChoice.id);
        aChoice->unblockValueChanges();
        bChoice->blockValueChanges();
        bChoice->setValueFromID(aCurChoice.id);
        bChoice->unblockValueChanges();
    } catch (...) {

    }
}

void
ViewerNodePrivate::setDisplayChannels(int index, bool setBoth)
{

    for (int i = 0; i < 2; ++i) {
        if (i == 1 && !setBoth) {
            break;
        }
        KnobChoicePtr displayChoice = displayChannelsKnob[i].lock();
        displayChoice->setValue(index);
    }
}

NATRON_NAMESPACE_EXIT
