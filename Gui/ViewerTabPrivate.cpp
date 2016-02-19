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

#include "ViewerTabPrivate.h"

#include <cassert>
#include <stdexcept>

#include <QDebug>

#include "Engine/EffectInstance.h"
#include "Engine/Node.h"
#include "Engine/TimeLine.h"
#include "Engine/Transform.h"
#include "Engine/ViewIdx.h"
#include "Engine/ViewerInstance.h"

#include "Gui/ActionShortcuts.h"
#include "Gui/ChannelsComboBox.h"
#include "Gui/ClickableLabel.h"
#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/ViewerTab.h"


NATRON_NAMESPACE_ENTER;


ViewerTabPrivate::ViewerTabPrivate(ViewerTab* publicInterface,ViewerInstance* node)
: publicInterface(publicInterface)
, viewer(NULL)
, viewerContainer(NULL)
, viewerLayout(NULL)
, viewerSubContainer(NULL)
, viewerSubContainerLayout(NULL)
, mainLayout(NULL)
, firstSettingsRow(NULL)
, secondSettingsRow(NULL)
, firstRowLayout(NULL)
, secondRowLayout(NULL)
, layerChoice(NULL)
, alphaChannelChoice(NULL)
, viewerChannels(NULL)
, viewerChannelsAutoswitchedToAlpha(false)
, zoomCombobox(NULL)
, syncViewerButton(NULL)
, centerViewerButton(NULL)
, clipToProjectFormatButton(NULL)
, enableViewerRoI(NULL)
, refreshButton(NULL)
, iconRefreshOff()
, iconRefreshOn()
, ongoingRenderCount(0)
, activateRenderScale(NULL)
, renderScaleActive(false)
, renderScaleCombo(NULL)
, firstInputLabel(NULL)
, firstInputImage(NULL)
, compositingOperatorLabel(NULL)
, compositingOperator(NULL)
, secondInputLabel(NULL)
, secondInputImage(NULL)
, toggleGainButton(NULL)
, gainBox(NULL)
, gainSlider(NULL)
, lastFstopValue(0.)
, autoContrast(NULL)
, gammaBox(NULL)
, lastGammaValue(1.)
, toggleGammaButton(NULL)
, gammaSlider(NULL)
, viewerColorSpace(NULL)
, checkerboardButton(NULL)
, pickerButton(NULL)
, viewsComboBox(NULL)
, currentViewIndex(0)
, currentViewMutex()
, infoWidget()
, playerButtonsContainer(0)
, playerLayout(NULL)
, currentFrameBox(NULL)
, firstFrame_Button(NULL)
, previousKeyFrame_Button(NULL)
, play_Backward_Button(NULL)
, previousFrame_Button(NULL)
, nextFrame_Button(NULL)
, play_Forward_Button(NULL)
, nextKeyFrame_Button(NULL)
, lastFrame_Button(NULL)
, previousIncrement_Button(NULL)
, incrementSpinBox(NULL)
, nextIncrement_Button(NULL)
, playbackMode_Button(NULL)
, playbackModeMutex()
, playbackMode(ePlaybackModeLoop)
, frameRangeEdit(NULL)
, canEditFrameRangeLabel(NULL)
, tripleSyncButton(0)
, canEditFpsBox(NULL)
, canEditFpsLabel(NULL)
, fpsLockedMutex()
, fpsLocked(true)
, fpsBox(NULL)
, userFps(24)
, turboButton(NULL)
, timeLineGui(NULL)
, rotoNodes()
, currentRoto()
, trackerNodes()
, currentTracker()
, inputNamesMap()
, compOperatorMutex()
, compOperator(eViewerCompositingOperatorNone)
, viewerNode(node)
, visibleToolbarsMutex()
, infobarVisible(true)
, playerVisible(true)
, timelineVisible(true)
, leftToolbarVisible(true)
, rightToolbarVisible(true)
, topToolbarVisible(true)
, isFileDialogViewer(false)
, checkerboardMutex()
, checkerboardEnabled(false)
, fpsMutex()
, fps(24.)
, lastOverlayNode()
, hasPenDown(false)
, hasCaughtPenMotionWhileDragging(false)
{
    infoWidget[0] = infoWidget[1] = NULL;
    currentRoto.first = NULL;
    currentRoto.second = NULL;
}

#ifdef NATRON_TRANSFORM_AFFECTS_OVERLAYS
bool
ViewerTabPrivate::getOverlayTransform(double time,
                                      ViewIdx view,
                                      const NodePtr& target,
                                      EffectInstance* currentNode,
                                      Transform::Matrix3x3* transform) const
{
    if (currentNode == target->getEffectInstance().get()) {
        return true;
    }
    RenderScale s(1.);
    EffectInstPtr input;
    StatusEnum stat = eStatusReplyDefault;
    Transform::Matrix3x3 mat;
    if (!currentNode->getNode()->isNodeDisabled() && currentNode->getNode()->getCurrentCanTransform()) {
        stat = currentNode->getTransform_public(time, s, view, &input, &mat);
    }
    if (stat == eStatusFailed) {
        return false;
    } else if (stat == eStatusReplyDefault) {
        //No transfo matrix found, pass to the input...
        
        ///Test all inputs recursively, going from last to first, preferring non optional inputs.
        std::list<EffectInstance*> nonOptionalInputs;
        std::list<EffectInstance*> optionalInputs;
        int maxInp = currentNode->getMaxInputCount();
        
        ///We cycle in reverse by default. It should be a setting of the application.
        ///In this case it will return input B instead of input A of a merge for example.
        for (int i = maxInp - 1; i >= 0; --i) {
            EffectInstPtr inp = currentNode->getInput(i);
            bool optional = currentNode->isInputOptional(i);
            if (inp) {
                if (optional) {
                    optionalInputs.push_back(inp.get());
                } else {
                    nonOptionalInputs.push_back(inp.get());
                }
            }
        }
        
        if (nonOptionalInputs.empty() && optionalInputs.empty()) {
            return false;
        }
        
        ///Cycle through all non optional inputs first
        for (std::list<EffectInstance*> ::iterator it = nonOptionalInputs.begin(); it != nonOptionalInputs.end(); ++it) {
            mat = Transform::Matrix3x3(1,0,0,0,1,0,0,0,1);
            bool isOk = getOverlayTransform(time, view, target, *it, &mat);
            if (isOk) {
                *transform = Transform::matMul(*transform, mat);
                return true;
            }
        }
        
        ///Cycle through optional inputs...
        for (std::list<EffectInstance*> ::iterator it = optionalInputs.begin(); it != optionalInputs.end(); ++it) {
            mat = Transform::Matrix3x3(1,0,0,0,1,0,0,0,1);
            bool isOk = getOverlayTransform(time, view, target, *it, &mat);
            if (isOk) {
                *transform = Transform::matMul(*transform, mat);
                return true;
            }
            
        }
        return false;
    } else {
        
        assert(input);
        double par = input->getAspectRatio(-1);
        
        //The mat is in pixel coordinates, though
        mat = Transform::matMul(Transform::matPixelToCanonical(par, 1, 1, false),mat);
        mat = Transform::matMul(mat,Transform::matCanonicalToPixel(par, 1, 1, false));
        *transform = Transform::matMul(*transform, mat);
        bool isOk = getOverlayTransform(time, view, target, input.get(), transform);
        return isOk;
    }
    return false;
    
}

static double transformTimeForNode(EffectInstance* currentNode, double inTime)
{
    U64 nodeHash = currentNode->getHash();
    FramesNeededMap framesNeeded = currentNode->getFramesNeeded_public(nodeHash, inTime, ViewIdx(0), 0);
    FramesNeededMap::iterator foundInput0 = framesNeeded.find(0/*input*/);
    if (foundInput0 == framesNeeded.end()) {
        return inTime;
    }
    
    FrameRangesMap::iterator foundView0 = foundInput0->second.find(ViewIdx(0));
    if (foundView0 == foundInput0->second.end()) {
        return inTime;
    }
    
    if (foundView0->second.empty()) {
        return inTime;
    } else {
        return (foundView0->second.front().min);
    }
}

bool
ViewerTabPrivate::getTimeTransform(double time,
                                   ViewIdx view,
                                   const NodePtr& target,
                                   EffectInstance* currentNode,
                                   double *newTime) const
{
    if (currentNode == target->getEffectInstance().get()) {
        *newTime = time;
        return true;
    }
    
    if (!currentNode->getNode()->isNodeDisabled()) {
        *newTime = transformTimeForNode(currentNode, time);
    } else {
        *newTime = time;
    }
    
    ///Test all inputs recursively, going from last to first, preferring non optional inputs.
    std::list<EffectInstance*> nonOptionalInputs;
    std::list<EffectInstance*> optionalInputs;
    int maxInp = currentNode->getMaxInputCount();
    
    ///We cycle in reverse by default. It should be a setting of the application.
    ///In this case it will return input B instead of input A of a merge for example.
    for (int i = maxInp - 1; i >= 0; --i) {
        EffectInstPtr inp = currentNode->getInput(i);
        bool optional = currentNode->isInputOptional(i);
        if (inp) {
            if (optional) {
                optionalInputs.push_back(inp.get());
            } else {
                nonOptionalInputs.push_back(inp.get());
            }
        }
    }
    
    if (nonOptionalInputs.empty() && optionalInputs.empty()) {
        return false;
    }
    
    ///Cycle through all non optional inputs first
    for (std::list<EffectInstance*> ::iterator it = nonOptionalInputs.begin(); it != nonOptionalInputs.end(); ++it) {
        double inputTime;
        bool isOk = getTimeTransform(*newTime, view, target, *it, &inputTime);
        if (isOk) {
            *newTime = inputTime;
            return true;
        }
    }
    
    ///Cycle through optional inputs...
    for (std::list<EffectInstance*> ::iterator it = optionalInputs.begin(); it != optionalInputs.end(); ++it) {
        double inputTime;
        bool isOk = getTimeTransform(*newTime, view, target, *it, &inputTime);
        if (isOk) {
            *newTime = inputTime;
            return true;
        }
        
    }
    return false;
}

#endif

void
ViewerTabPrivate::getComponentsAvailabel(std::set<ImageComponents>* comps) const
{
    int activeInputIdx[2];
    viewerNode->getActiveInputs(activeInputIdx[0], activeInputIdx[1]);
    EffectInstPtr activeInput[2] = {EffectInstPtr(), EffectInstPtr()};
    for (int i = 0; i < 2; ++i) {
        activeInput[i] = viewerNode->getInput(activeInputIdx[i]);
        if (activeInput[i]) {
            EffectInstance::ComponentsAvailableMap compsAvailable;
            activeInput[i]->getComponentsAvailable(true, true, publicInterface->getGui()->getApp()->getTimeLine()->currentFrame(), &compsAvailable);
            for (EffectInstance::ComponentsAvailableMap::iterator it = compsAvailable.begin(); it != compsAvailable.end(); ++it) {
                if (it->second.lock()) {
                    comps->insert(it->first);
                }
            }
        }
    }

}

NATRON_NAMESPACE_EXIT;
