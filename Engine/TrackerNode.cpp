/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
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

#include "TrackerNode.h"
#include "Engine/Node.h"
#include "Engine/TrackerContext.h"

NATRON_NAMESPACE_ENTER;

TrackerNode::TrackerNode(boost::shared_ptr<Natron::Node> node)
: Natron::EffectInstance(node)
{
}

TrackerNode::~TrackerNode()
{
    
}

std::string
TrackerNode::getPluginID() const
{
    return PLUGINID_NATRON_TRACKER;
}


std::string
TrackerNode::getPluginLabel() const
{
    return "Tracker";
}

std::string
TrackerNode::getPluginDescription() const
{
    return "Track a 2D point using LibMV from the Blender open-source software.";
}

std::string
TrackerNode::getInputLabel (int inputNb) const
{
    switch (inputNb) {
        case 0:
            return "Source";
        default:
            break;
    }
    return "";
}

bool
TrackerNode::isInputMask(int /*inputNb*/) const
{
    return false;
}

bool
TrackerNode::isInputOptional(int /*inputNb*/) const
{
    return false;
}

void
TrackerNode::addAcceptedComponents(int inputNb,std::list<Natron::ImageComponents>* comps)
{
    
    if (inputNb != 1) {
        comps->push_back(ImageComponents::getRGBAComponents());
        comps->push_back(ImageComponents::getRGBComponents());
        comps->push_back(ImageComponents::getXYComponents());
    }
    comps->push_back(ImageComponents::getAlphaComponents());
}

void
TrackerNode::addSupportedBitDepth(std::list<Natron::ImageBitDepthEnum>* depths) const
{
    depths->push_back(Natron::eImageBitDepthFloat);
}

void
TrackerNode::initializeKnobs()
{
    
}

bool
TrackerNode::isIdentity(double time,
                        const RenderScale & /*scale*/,
                        const RectI & /*roi*/,
                        ViewIdx view,
                        double* inputTime,
                        ViewIdx* inputView,
                        int* inputNb)
{
    // Identity for now, until we can apply a transform
    *inputTime = time;
    *inputNb = 0;
    *inputView = view;
    return true;
}


void
TrackerNode::knobChanged(KnobI* k,
                       ValueChangedReasonEnum reason,
                       ViewSpec view,
                       double time,
                       bool originatedFromMainThread)
{
    boost::shared_ptr<TrackerContext> ctx = getNode()->getTrackerContext();
    if (!ctx) {
        return;
    }
    ctx->knobChanged(k, reason, view, time, originatedFromMainThread);
}

void
TrackerNode::onInputChanged(int inputNb)
{
    boost::shared_ptr<TrackerContext> ctx = getNode()->getTrackerContext();
    ctx->s_onNodeInputChanged(inputNb);
}

NATRON_NAMESPACE_EXIT;
