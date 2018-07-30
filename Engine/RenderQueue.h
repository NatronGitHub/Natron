/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
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

#ifndef NATRON_ENGINE_RENDERQUEUE_H
#define NATRON_ENGINE_RENDERQUEUE_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <cmath>
#include <climits>
#include <QObject>


#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/enable_shared_from_this.hpp>
#include <boost/scoped_ptr.hpp>
#endif

#include "Engine/TimeValue.h"

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

struct RenderQueuePrivate;
class RenderQueue : public QObject
{

    GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
    GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    struct RenderWork
    {
        // The tree root is the node from which we want to pull images from
        NodePtr treeRoot;

        // A label indicating the task (such as writer filename)
        std::string renderLabel;

        // The first frame to render. If set to INT_MIN, will be automatically set from the frame range
        // of the tree root node.
        TimeValue firstFrame;

        // The last frame (included) to render.  If set to INT_MAX, will be automatically set from the frame range
        // of the tree root node.
        TimeValue lastFrame;

        // The timeline step between frames (may be negative but not 0)
        //  If set to INT_MIN, will be automatically set from the frame step knob (kNatronWriteParamFrameStep)
        TimeValue frameStep;

        // True if we want a detailed per-node timing breakdown
        bool useRenderStats;

        // True if this request is a restart of a previous request
        bool isRestart;

        RenderWork()
        : treeRoot()
        , renderLabel()
        , firstFrame(INT_MIN)
        , lastFrame(INT_MAX)
        , frameStep(INT_MIN)
        , useRenderStats(false)
        , isRestart(false)
        {
        }

        RenderWork(const NodePtr& treeRoot,
                   const std::string& label,
                   TimeValue firstFrame,
                   TimeValue lastFrame,
                   TimeValue frameStep,
                   bool useRenderStats)
        : treeRoot(treeRoot)
        , renderLabel(label)
        , firstFrame(firstFrame)
        , lastFrame(lastFrame)
        , frameStep(frameStep)
        , useRenderStats(useRenderStats)
        , isRestart(false)
        {
        }
    };

    RenderQueue(const AppInstancePtr& app);

    virtual ~RenderQueue();




    
    /**
     * @brief Creates render work request from the given command line arguments.
     * Note that this function may create write nodes if asked for from the command line arguments.
     * This function throw exceptions upon failure with a detailed error message.
     * If writers is empty, it will create a render request for all writer nodes in the project
     **/
    void createRenderRequestsFromCommandLineArgs(bool enableRenderStats,
                                                 const std::list<std::string>& writers,
                                                 const std::list<std::pair<int, std::pair<int, int> > >& frameRanges,
                                                 std::list<RenderWork>& requests);

    /**
     * @brief Same as the other function but with a CLArgs object
     * If the writer args is empty, it will create a render request for all writer nodes in the project
     **/
    void createRenderRequestsFromCommandLineArgs(const CLArgs& cl, std::list<RenderWork>& requests);

    /**
     * @brief Queues the given write nodes to render. This function will block until all renders are finished.
     **/
    void renderBlocking(const std::list<RenderWork>& writers);

    /**
     * @brief Queues the given write nodes to render. This function will return immeditalely
     **/
    void renderNonBlocking(const std::list<RenderWork>& writers);

    /**
     * @brief Remove from the render queue a render that was not yet started. This is useful for the GUI
     * if the user wants to cancel a render request.
     **/
    void removeRenderFromQueue(const NodePtr& writer);

public Q_SLOTS:


    /**
     * @brief Called when a render started with renderWritersInternal is finished
     **/
    void onQueuedRenderFinished(int retCode);

    /**
     * @brief Called when a render started with renderWritersInternal is finished from another process
     **/
    void onBackgroundRenderProcessFinished();

private:

    boost::scoped_ptr<RenderQueuePrivate> _imp;
};

NATRON_NAMESPACE_EXIT

#endif // NATRON_ENGINE_RENDERQUEUE_H
