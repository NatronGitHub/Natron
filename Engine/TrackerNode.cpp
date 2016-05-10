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

struct TrackerNodePrivate
{
    TrackerNodePrivate()
    {
    }
};

TrackerNode::TrackerNode(boost::shared_ptr<Natron::Node> node)
    : NodeGroup(node)
    , _imp( new TrackerNodePrivate() )
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
    return "Track one or more 2D point(s) using LibMV from the Blender open-source software.\n"
           "\n"
           "Goal:\n"
           "- Track one or more 2D point and use them to either make another object/image match-move their motion or to stabilize the input image."
           "\n"
           "Tracking:\n"
           "- Connect a Tracker node to the image containing the item you need to track\n"
           "- Place tracking markers with CTRL+ALT+Click on the Viewer or by clicking the \"+\" button below the track table in the settings panel"
           "- Setup the motion model to match the motion type of the item you need to track. By default the tracker will only assume the item is underoing a translation. Other motion models can be used for complex tracks but may be slower.\n"
           "- Select in the settings panel or on the Viewer the markers you want to track and then start tracking with the player buttons on the top of the Viewer.\n"
           "- If a track is getting lost or fails at some point, you may refine it by moving the marker at its correct position, this will force a new keyframe on the pattern which will be visible in the Viewer and on the timeline.\n"
           "\n"
           "Using the tracks data:\n"
           "You can either use the Tracker node itself to use the track data or you may export it to another node.\n"
           "Using the Transform within the Tracker node:\n"
           "Go to the Transform tab in the settings panel, and set the Transform Type to the operation you want to achieve. During tracking, the Transform Type should always been set to None if you want to correctly see the tracks on the Viewer."
           "You will notice that the transform parameters will be set automatically when the tracking is finished. Depending on the Transform Type, the values will be computed either to match-move the motion of the tracked points or to stabilize the image.\n"
           "Exporting the tracking data:\n"
           "You may export the tracking data either to a CornerPin node or to a Transform node. The CornerPin node performs a warp that may be more stable than a Transform node when using 4 or more tracks: it retains more information than the Transform node.";
}

void
TrackerNode::initializeKnobs()
{
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
TrackerNode::onKnobsLoaded()
{
    boost::shared_ptr<TrackerContext> ctx = getNode()->getTrackerContext();

    if (!ctx) {
        return;
    }
    ctx->onKnobsLoaded();
}

void
TrackerNode::onInputChanged(int inputNb)
{
    boost::shared_ptr<TrackerContext> ctx = getNode()->getTrackerContext();

    ctx->inputChanged(inputNb);
}

NATRON_NAMESPACE_EXIT;
