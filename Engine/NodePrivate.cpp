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

#include <sstream> // stringstream

#include <QDebug>

#include "Engine/AppInstance.h"
#include "Engine/KnobTypes.h"
#include "Engine/KnobItemsTable.h"
#include "Engine/ImageComponents.h"
#include "Engine/Image.h"
#include "Engine/Project.h"
#include "Engine/Timer.h"
#include "Engine/NodeGuiI.h"

NATRON_NAMESPACE_ENTER

NodePrivate::NodePrivate(Node* publicInterface,
                         const AppInstancePtr& app_,
                         const NodeCollectionPtr& collection,
                         const PluginPtr& plugin_)
: _publicInterface(publicInterface)
, group(collection)
, precomp()
, app(app_)
, isPersistent(true)
, inputsInitialized(false)
, outputsMutex()
, outputs()
, inputsMutex()
, inputs()
, effect()
, duringInteractAction(false)
, overlaysViewport(0)
, inputsComponents()
, outputComponents()
, nameMutex()
, scriptName()
, label()
, inputsLabelsMutex()
, inputLabels()
, deactivatedState()
, activatedMutex()
, activated(true)
, plugin(plugin_)
, pyPlugHandle()
, isPyPlug(false)
, computingPreview(false)
, previewThreadQuit(false)
, computingPreviewMutex()
, mustQuitPreview(0)
, mustQuitPreviewMutex()
, mustQuitPreviewCond()
, renderInstancesSharedMutex(QMutex::Recursive)
, ioContainer()
, frameIncrKnob()
, nodeLabelKnob()
, previewEnabledKnob()
, disableNodeKnob()
, infoPage()
, nodeInfos()
, refreshInfoButton()
, forceCaching()
, hideInputs()
, beforeFrameRender()
, beforeRender()
, afterFrameRender()
, afterRender()
, enabledChan()
, channelsSelectors()
, maskSelectors()
, supportedDepths()
, lastRenderStartedMutex()
, lastRenderStartedSlotCallTime()
, renderStartedCounter(0)
, inputIsRenderingCounter(0)
, lastInputNRenderStartedSlotCallTime()
, persistentMessage()
, persistentMessageType(0)
, persistentMessageMutex()
, guiPointer()
, nativeOverlays()
, nodeCreated(false)
, wasCreatedSilently(false)
, createdComponentsMutex()
, createdComponents()
, paintStroke()
, pluginsPropMutex()
, pluginSafety(eRenderSafetyInstanceSafe)
, currentThreadSafety(eRenderSafetyInstanceSafe)
, currentSupportTiles(false)
, currentSupportOpenGLRender(ePluginOpenGLRenderSupportNone)
, currentSupportSequentialRender(eSequentialPreferenceNotSequential)
, currentCanDistort(false)
, lastRenderedImageMutex()
, lastRenderedImage()
, isBeingDestroyedMutex()
, isBeingDestroyed(false)
, inputModifiedRecursion(0)
, inputsModified()
, refreshIdentityStateRequestsCount(0)
, streamWarnings()
, requiresGLFinishBeforeRender(false)
, nodePositionCoords()
, nodeSize()
, nodeColor()
, overlayColor()
, nodeIsSelected(false)
, restoringDefaults(false)
, hostChannelSelectorEnabled(false)
{
    nodePositionCoords[0] = nodePositionCoords[1] = INT_MIN;
    nodeSize[0] = nodeSize[1] = -1;
    nodeColor[0] = nodeColor[1] = nodeColor[2] = -1;
    overlayColor[0] = overlayColor[1] = overlayColor[2] = -1;


    ///Initialize timers
    gettimeofday(&lastRenderStartedSlotCallTime, 0);
    gettimeofday(&lastInputNRenderStartedSlotCallTime, 0);
}

void
Node::setDuringInteractAction(bool b)
{
    assert(QThread::currentThread() == qApp->thread());
    _imp->duringInteractAction = b;
}


void
Node::choiceParamAddLayerCallback(const KnobChoicePtr& knob)
{
    if (!knob) {
        return ;
    }
    EffectInstancePtr effect = toEffectInstance(knob->getHolder());
    if (!effect) {
        return;
    }
    NodeGuiIPtr gui = effect->getNode()->getNodeGui();
    if (!gui) {
        return;
    }
    gui->addComponentsWithDialog(knob);
}


void
NodePrivate::refreshMetadaWarnings(const NodeMetadata &metadata)
{
    assert(QThread::currentThread() == qApp->thread());

    int nInputs = effect->getMaxInputCount();

    QString bitDepthWarning = tr("This nodes converts higher bit depths images from its inputs to a lower bitdepth image. As "
                                 "a result of this process, the quality of the images is degraded. The following conversions are done:\n");
    bool setBitDepthWarning = false;
    const bool supportsMultipleClipDepths = effect->supportsMultipleClipDepths();
    const bool supportsMultipleClipPARs = effect->supportsMultipleClipPARs();
    const bool supportsMultipleClipFPSs = effect->supportsMultipleClipFPSs();
    std::vector<EffectInstancePtr> inputs(nInputs);
    for (int i = 0; i < nInputs; ++i) {
        inputs[i] = _publicInterface->getInput(i);
    }


    ImageBitDepthEnum outputDepth = metadata.getBitDepth(-1);
    double outputPAR = metadata.getPixelAspectRatio(-1);
    bool outputFrameRateSet = false;
    double outputFrameRate = metadata.getOutputFrameRate();
    bool mustWarnFPS = false;
    bool mustWarnPAR = false;

    int nbConnectedInputs = 0;
    for (int i = 0; i < nInputs; ++i) {
        //Check that the bitdepths are all the same if the plug-in doesn't support multiple depths
        if ( !supportsMultipleClipDepths && (metadata.getBitDepth(i) != outputDepth) ) {
        }

        const double pixelAspect = metadata.getPixelAspectRatio(i);

        if (!supportsMultipleClipPARs) {
            if (pixelAspect != outputPAR) {
                mustWarnPAR = true;
            }
        }

        if (!inputs[i]) {
            continue;
        }

        ++nbConnectedInputs;

        const double fps = inputs[i]->getFrameRate(TreeRenderNodeArgsPtr());



        if (!supportsMultipleClipFPSs) {
            if (!outputFrameRateSet) {
                outputFrameRate = fps;
                outputFrameRateSet = true;
            } else if (std::abs(outputFrameRate - fps) > 0.01) {
                // We have several inputs with different frame rates
                mustWarnFPS = true;
            }
        }


        ImageBitDepthEnum inputOutputDepth = inputs[i]->getBitDepth(TreeRenderNodeArgsPtr(), -1);

        //If the bit-depth conversion will be lossy, warn the user
        if ( Image::isBitDepthConversionLossy( inputOutputDepth, metadata.getBitDepth(i) ) ) {
            bitDepthWarning.append( QString::fromUtf8( inputs[i]->getNode()->getLabel_mt_safe().c_str() ) );
            bitDepthWarning.append( QString::fromUtf8(" (") + QString::fromUtf8( Image::getDepthString(inputOutputDepth).c_str() ) + QChar::fromLatin1(')') );
            bitDepthWarning.append( QString::fromUtf8(" ----> ") );
            bitDepthWarning.append( QString::fromUtf8( _publicInterface->getLabel_mt_safe().c_str() ) );
            bitDepthWarning.append( QString::fromUtf8(" (") + QString::fromUtf8( Image::getDepthString( metadata.getBitDepth(i) ).c_str() ) + QChar::fromLatin1(')') );
            bitDepthWarning.append( QChar::fromLatin1('\n') );
            setBitDepthWarning = true;
        }


        if ( !supportsMultipleClipPARs && (pixelAspect != outputPAR) ) {
            qDebug() << _publicInterface->getScriptName_mt_safe().c_str() << ": The input " << inputs[i]->getNode()->getScriptName_mt_safe().c_str()
            << ") has a pixel aspect ratio (" << metadata.getPixelAspectRatio(i)
            << ") different than the output clip (" << outputPAR << ") but it doesn't support multiple clips PAR. "
            << "This should have been handled earlier before connecting the nodes, @see Node::canConnectInput.";
        }
    }

    std::map<Node::StreamWarningEnum, QString> warnings;
    if (setBitDepthWarning) {
        warnings[Node::eStreamWarningBitdepth] = bitDepthWarning;
    } else {
        warnings[Node::eStreamWarningBitdepth] = QString();
    }

    if (mustWarnFPS && nbConnectedInputs > 1) {
        QString fpsWarning = tr("One or multiple inputs have a frame rate different of the output. "
                                "It is not handled correctly by this node. To remove this warning make sure all inputs have "
                                "the same frame-rate, either by adjusting project settings or the upstream Read node.");
        warnings[Node::eStreamWarningFrameRate] = fpsWarning;
    } else {
        warnings[Node::eStreamWarningFrameRate] = QString();
    }

    if (mustWarnPAR && nbConnectedInputs > 1) {
        QString parWarnings = tr("One or multiple input have a pixel aspect ratio different of the output. It is not "
                                 "handled correctly by this node and may yield unwanted results. Please adjust the "
                                 "pixel aspect ratios of the inputs so that they match by using a Reformat node.");
        warnings[Node::eStreamWarningPixelAspectRatio] = parWarnings;
    } else {
        warnings[Node::eStreamWarningPixelAspectRatio] = QString();
    }
    

    _publicInterface->setStreamWarnings(warnings);
} // refreshMetadaWarnings





void
NodePrivate::refreshDefaultPagesOrder()
{
    const KnobsVec& knobs = effect->getKnobs();
    defaultPagesOrder.clear();
    for (KnobsVec::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        KnobPagePtr ispage = toKnobPage(*it);
        if (ispage && !ispage->getChildren().empty() && !ispage->getIsSecret()) {
            defaultPagesOrder.push_back( ispage->getName() );
        }
    }

}



void
NodePrivate::refreshDefaultViewerKnobsOrder()
{
    KnobsVec knobs = effect->getViewerUIKnobs();
    defaultViewerKnobsOrder.clear();
    for (KnobsVec::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        defaultViewerKnobsOrder.push_back( (*it)->getName() );
    }
}


void
NodePrivate::abortPreview_non_blocking()
{
    bool computing;
    {
        QMutexLocker locker(&computingPreviewMutex);
        computing = computingPreview;
    }

    if (computing) {
        QMutexLocker l(&mustQuitPreviewMutex);
        ++mustQuitPreview;
    }
}

void
NodePrivate::abortPreview_blocking(bool allowPreviewRenders)
{
    bool computing;
    {
        QMutexLocker locker(&computingPreviewMutex);
        computing = computingPreview;
        previewThreadQuit = !allowPreviewRenders;
    }

    if (computing) {
        QMutexLocker l(&mustQuitPreviewMutex);
        assert(!mustQuitPreview);
        ++mustQuitPreview;
        while (mustQuitPreview) {
            mustQuitPreviewCond.wait(&mustQuitPreviewMutex);
        }
    }
}

bool
NodePrivate::checkForExitPreview()
{
    {
        QMutexLocker locker(&mustQuitPreviewMutex);
        if (mustQuitPreview || previewThreadQuit) {
            mustQuitPreview = 0;
            mustQuitPreviewCond.wakeOne();

            return true;
        }

        return false;
    }
}


NATRON_NAMESPACE_EXIT
