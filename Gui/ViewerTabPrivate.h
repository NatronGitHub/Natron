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

#ifndef Gui_ViewerTabPrivate_h
#define Gui_ViewerTabPrivate_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <map>
#include <set>

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QMutex>
#include <QtCore/QTimer>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Global/Enums.h"

#include "Engine/ViewIdx.h"
#include "Engine/EngineFwd.h"
#include "Engine/TimeValue.h"
#include "Gui/ComboBox.h"
#include "Gui/GuiFwd.h"
#include "Gui/NodeViewerContext.h"
#include "Gui/CachedFramesThread.h"


#define NATRON_TRANSFORM_AFFECTS_OVERLAYS

NATRON_NAMESPACE_ENTER

struct ViewerTabPrivate
{

    ViewerTab* publicInterface; // can not be a smart ptr

    ViewerNodeWPtr viewerNode; // < pointer to the internal node

    /*OpenGL viewer*/
    ViewerGL* viewer;
    QWidget* viewerContainer;
    QHBoxLayout* viewerLayout;
    QWidget* viewerSubContainer;
    QVBoxLayout* viewerSubContainerLayout;
    QVBoxLayout* mainLayout;

    InfoViewerWidget* infoWidget[2];


    /*frame seeker*/
    TimeLineGui* timeLineGui;


    boost::scoped_ptr<CachedFramesThread> cachedFramesThread;


    // This is all nodes that have a viewer context
    std::map<NodeGuiWPtr, NodeViewerContextPtr> nodesContext;

    /*
       Tells for a given plug-in ID, which nodes is currently activated in the viewer interface
     */
    struct PluginViewerContext
    {
        PluginWPtr plugin, pyPlug;
        NodeGuiWPtr currentNode;
        NodeViewerContextPtr currentContext;
    };


    // This is the current active context for each plug-in
    // We don't use a map because we need to retain the insertion order for each plug-in
    std::list<PluginViewerContext> currentNodeContext;

    // Weak_ptr because the viewer node  itself controls the lifetime of this widget
    bool isFileDialogViewer;

    //The last node that took the penDown/motion/keyDown/keyRelease etc...
    NodeWPtr lastOverlayNode;
    bool hasPenDown;
    bool canEnableDraftOnPenMotion;
    bool hasCaughtPenMotionWhileDragging;

    ViewerTabPrivate(ViewerTab* publicInterface,
                     const NodeGuiPtr& node);

#ifdef NATRON_TRANSFORM_AFFECTS_OVERLAYS
    // return the tronsform to apply to the overlay as a 3x3 homography in canonical coordinates
    bool getOverlayTransform(TimeValue time,
                             ViewIdx view,
                             const NodePtr& target,
                             const EffectInstancePtr& currentNode,
                             Transform::Matrix3x3* transform) const;

    bool getTimeTransform(TimeValue time,
                          ViewIdx view,
                          const NodePtr& target,
                          const EffectInstancePtr& currentNode,
                          TimeValue *newTime) const;

#endif

    std::list<PluginViewerContext>::iterator findActiveNodeContextForNode(const NodePtr& plugin);
    std::list<PluginViewerContext>::iterator findActiveNodeContextForPlugin(const PluginPtr& plugin);


    // Returns true if this node has a viewer context but it is not active
    bool hasInactiveNodeViewerContext(const NodePtr& node);
};

NATRON_NAMESPACE_EXIT

#endif // Gui_ViewerTabPrivate_h
