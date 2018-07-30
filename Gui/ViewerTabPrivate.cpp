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

#include "ViewerTabPrivate.h"

#include <cassert>
#include <stdexcept>

#include <QtCore/QDebug>

#include "Engine/EffectInstance.h"
#include "Engine/Node.h"
#include "Engine/TimeLine.h"
#include "Engine/Transform.h"
#include "Engine/ViewIdx.h"
#include "Engine/ViewerInstance.h"
#include "Engine/ViewerNode.h"

#include "Gui/ActionShortcuts.h"
#include "Gui/ClickableLabel.h"
#include "Gui/Gui.h"
#include "Gui/NodeGui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/ViewerTab.h"


NATRON_NAMESPACE_ENTER


ViewerTabPrivate::ViewerTabPrivate(ViewerTab* publicInterface,
                                   const NodeGuiPtr& node_ui)
    : publicInterface(publicInterface)
    , viewerNode()
    , viewer(NULL)
    , viewerContainer(NULL)
    , viewerLayout(NULL)
    , viewerSubContainer(NULL)
    , viewerSubContainerLayout(NULL)
    , mainLayout(NULL)
    , infoWidget()
    , timeLineGui(NULL)
    , cachedFramesThread()
    , nodesContext()
    , currentNodeContext()
    , isFileDialogViewer(false)
    , lastOverlayNode()
    , hasPenDown(false)
    , canEnableDraftOnPenMotion(false)
    , hasCaughtPenMotionWhileDragging(false)
{
    viewerNode = node_ui->getNode()->isEffectViewerNode();
    infoWidget[0] = infoWidget[1] = NULL;
}

#ifdef NATRON_TRANSFORM_AFFECTS_OVERLAYS
bool
ViewerTabPrivate::getOverlayTransform(TimeValue time,
                                      ViewIdx view,
                                      const NodePtr& target,
                                      const EffectInstancePtr& currentNode,
                                      Transform::Matrix3x3* transform) const
{
    if ( currentNode == target->getEffectInstance() ) {
        return true;
    }
    if (!currentNode) {
        return false;
    }
    RenderScale s(1.);
    EffectInstancePtr input;
    ActionRetCodeEnum stat = eActionStatusReplyDefault;
    DistortionFunction2DPtr disto;

    // call getTransform even if the effect claims not to support it: it may still return
    // a transform to apply to the overlays (eg for Reformat).
    // If transform is not implemented, it should return eStatusReplyDefault:
    // http://openfx.sourceforge.net/Documentation/1.4/ofxProgrammingReference.html#mainEntryPoint
    // "the value kOfxStatReplyDefault is returned if the plug-in does not trap the action"

    // Internally this will return an identity matrix if the node is identity

    {
        GetDistortionResultsPtr results;
        stat = currentNode->getInverseDistortion_public(time, s, /*draftRender=*/true, view, &results);
        if (isFailureRetCode(stat)) {
            return false;
        }
        if (stat != eActionStatusReplyDefault && results) {
            disto = results->getResults();
        }
        if (disto && disto->inputNbToDistort != -1) {
            input = currentNode->getInputMainInstance(disto->inputNbToDistort);
        }

    }


    if (stat == eActionStatusOK) {
        if (!input) {
            return false;
        }

        if (disto->transformMatrix) {
            Transform::Matrix3x3 mat(*disto->transformMatrix);

            // The matrix must be inversed because this is the inversed transform
            Transform::Matrix3x3 invTransform;
            if (!transform->inverse(&invTransform)) {
                return false;
            }
            *transform = Transform::matMul(invTransform, mat);
        }

        return getOverlayTransform(time, view, target, input, transform);
    } else if (stat == eActionStatusReplyDefault) {
        // No transfo matrix found, pass to the input...

        // Test all inputs recursively, going from last to first, preferring non optional inputs.
        std::list<EffectInstancePtr> nonOptionalInputs;
        std::list<EffectInstancePtr> optionalInputs;
        int maxInp = currentNode->getNInputs();

        // We cycle in reverse by default. It should be a setting of the application.
        // In this case it will return input B instead of input A of a merge for example.
        for (int i = maxInp - 1; i >= 0; --i) {
            EffectInstancePtr inp = currentNode->getInputMainInstance(i);
            bool optional = currentNode->getNode()->isInputOptional(i);
            if (inp) {
                if (optional) {
                    optionalInputs.push_back(inp);
                } else {
                    nonOptionalInputs.push_back(inp);
                }
            }
        }

        if ( nonOptionalInputs.empty() && optionalInputs.empty() ) {
            return false;
        }

        // Cycle through all non optional inputs first
        for (std::list<EffectInstancePtr> ::iterator it = nonOptionalInputs.begin(); it != nonOptionalInputs.end(); ++it) {
            Transform::Matrix3x3 mat;
            mat.setIdentity();
            bool isOk = getOverlayTransform(time, view, target, *it, &mat);
            if (isOk) {
                *transform = Transform::matMul(*transform, mat);

                return true;
            }
        }

        ///Cycle through optional inputs...
        for (std::list<EffectInstancePtr> ::iterator it = optionalInputs.begin(); it != optionalInputs.end(); ++it) {
            Transform::Matrix3x3 mat;
            mat.setIdentity();
            bool isOk = getOverlayTransform(time, view, target, *it, &mat);
            if (isOk) {
                *transform = Transform::matMul(*transform, mat);

                return true;
            }
        }
        
    }

    return false;



} // ViewerTabPrivate::getOverlayTransform

static TimeValue
transformTimeForNode(const EffectInstancePtr& currentNode,
                     int inputNb,
                     TimeValue inTime,
                     ViewIdx view)
{
    FramesNeededMap framesNeeded;
    {
        GetFramesNeededResultsPtr results;
        ActionRetCodeEnum stat = currentNode->getFramesNeeded_public(TimeValue(inTime), view, &results);
        if (isFailureRetCode(stat)) {
            return inTime;
        }
        results->getFramesNeeded(&framesNeeded);
    }
    FramesNeededMap::iterator foundInput0 = framesNeeded.find(inputNb /*input*/);


    if ( foundInput0 == framesNeeded.end() ) {
        return inTime;
    }

    FrameRangesMap::iterator foundView0 = foundInput0->second.find(view);
    if ( foundView0 == foundInput0->second.end() ) {
        return inTime;
    }

    if ( foundView0->second.empty() ) {
        return inTime;
    } else {
        return TimeValue(foundView0->second.front().min);
    }
}

bool
ViewerTabPrivate::getTimeTransform(TimeValue time,
                                   ViewIdx view,
                                   const NodePtr& target,
                                   const EffectInstancePtr& currentNode,
                                   TimeValue *newTime) const
{
    if (!currentNode || !currentNode->getNode()) {
        return false;
    }
    if ( currentNode == target->getEffectInstance() ) {
        *newTime = time;

        return true;
    }



    ///Test all inputs recursively, going from last to first, preferring non optional inputs.
    std::list<int> nonOptionalInputs;
    std::list<int> optionalInputs;
    int maxInp = currentNode->getNInputs();

    ///We cycle in reverse by default. It should be a setting of the application.
    ///In this case it will return input B instead of input A of a merge for example.
    for (int i = maxInp - 1; i >= 0; --i) {
        EffectInstancePtr inp = currentNode->getInputMainInstance(i);
        bool optional = currentNode->getNode()->isInputOptional(i);
        if (inp) {
            if (optional) {
                optionalInputs.push_back(i);
            } else {
                nonOptionalInputs.push_back(i);
            }

        }

    }


    ///Cycle through all non optional inputs first

    for (std::list<int> ::iterator it = nonOptionalInputs.begin(); it != nonOptionalInputs.end(); ++it) {
        TimeValue inputTime;
        bool isOk = false;
        EffectInstancePtr input = currentNode->getInputMainInstance(*it);
        if (input) {

            if ( !currentNode->getNode()->getEffectInstance()->isNodeDisabledForFrame(time, view) ) {
                *newTime = transformTimeForNode(currentNode, *it, time, view);
            } else {
                *newTime = time;
            }
            isOk = getTimeTransform(*newTime, view, target, input, &inputTime);
        }
        if (isOk) {
            *newTime = inputTime;

            return true;
        }
    }

    ///Cycle through optional inputs...
    for (std::list<int> ::iterator it = optionalInputs.begin(); it != optionalInputs.end(); ++it) {
        TimeValue inputTime;
        bool isOk = false;
        EffectInstancePtr input = currentNode->getInputMainInstance(*it);
        if (input) {
            if ( !currentNode->getNode()->getEffectInstance()->isNodeDisabledForFrame(time, view) ) {
                *newTime = transformTimeForNode(currentNode, *it, time, view);
            } else {
                *newTime = time;
            }
            isOk = getTimeTransform(*newTime, view, target, input, &inputTime);
        }
        if (isOk) {
            *newTime = inputTime;

            return true;
        }
    }

    return false;
} // ViewerTabPrivate::getTimeTransform

#endif // ifdef NATRON_TRANSFORM_AFFECTS_OVERLAYS

std::list<ViewerTabPrivate::PluginViewerContext>::iterator
ViewerTabPrivate::findActiveNodeContextForNode(const NodePtr& node)
{
    // Try once with the pyplug if any and once without, because the plug-in may have changed
    PluginPtr plug = node->getPlugin();
    return findActiveNodeContextForPlugin(plug);
}

std::list<ViewerTabPrivate::PluginViewerContext>::iterator
ViewerTabPrivate::findActiveNodeContextForPlugin(const PluginPtr& plugin)
{
    std::string pluginID = plugin->getPluginID();
    // Roto and RotoPaint are 2 different plug-ins but we don't want them at the same time in the viewer
    bool isRotoOrRotoPaint = pluginID == PLUGINID_NATRON_ROTO || pluginID == PLUGINID_NATRON_ROTOPAINT;
    for (std::list<PluginViewerContext>::iterator it = currentNodeContext.begin(); it != currentNodeContext.end(); ++it) {
        if (isRotoOrRotoPaint) {
            std::string otherID = it->plugin.lock()->getPluginID();
            if (otherID == PLUGINID_NATRON_ROTO || otherID == PLUGINID_NATRON_ROTOPAINT) {
                return it;
            }
        } else {
            if (plugin == it->plugin.lock() || plugin == it->pyPlug.lock()) {
                return it;
            }
        }

    }

    return currentNodeContext.end();
}

bool
ViewerTabPrivate::hasInactiveNodeViewerContext(const NodePtr& node)
{
    if (node->isEffectViewerNode()) {
        return false;
    }
    std::list<ViewerTabPrivate::PluginViewerContext>::iterator found = findActiveNodeContextForNode(node);

    if ( found == currentNodeContext.end() ) {
        return false;
    }
    NodeGuiPtr n = found->currentNode.lock();
    if (!n) {
        return false;
    }

    return n->getNode() != node;
}

NATRON_NAMESPACE_EXIT
