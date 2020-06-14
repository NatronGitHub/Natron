/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2020 The Natron developers
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

#include "NodePrivate.h"

#include <sstream> // stringstream

#include <QDebug>

#include "Engine/AppInstance.h"
#include "Engine/KnobTypes.h"
#include "Engine/KnobItemsTable.h"
#include "Engine/ImagePlaneDesc.h"
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
, app(app_)
, isPersistent(true)
, outputsMutex()
, outputs()
, inputsMutex()
, inputs()
, inputDescriptions()
, effect()
, duringInteractAction(false)
, nameMutex()
, scriptName()
, label()
, plugin(plugin_)
, pyPlugHandle()
, isPyPlug(false)
, computingPreview(false)
, previewThreadQuit(false)
, computingPreviewMutex()
, mustQuitPreview(0)
, mustQuitPreviewMutex()
, mustQuitPreviewCond()
, ioContainer()
, lastRenderStartedMutex()
, lastRenderStartedSlotCallTime()
, renderStartedCounter(0)
, inputIsRenderingCounter()
, lastInputNRenderStartedSlotCallTime()
, persistentMessages()
, persistentMessageMutex()
, guiPointer()
, nodeCreated(false)
, wasCreatedSilently(false)
, isBeingDestroyedMutex()
, isBeingDestroyed(false)
, inputModifiedRecursion(0)
, hasModifiedInputsDescription(0)
, inputsModified()
, refreshIdentityStateRequestsCount(0)
, streamWarnings()
, requiresGLFinishBeforeRender(false)
, lastTimeInvariantMetadataHashRefreshed(0)
, nodePositionCoords()
, nodeSize()
, nodeColor()
, overlayColor()
, overlayActionsDraftEnabled(true)
, nodeIsSelected(false)
, restoringDefaults(false)
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
