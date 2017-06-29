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

#ifndef NATRON_ENGINE_RENDERTHREADTASK_H
#define NATRON_ENGINE_RENDERTHREADTASK_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****


#include <vector>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/weak_ptr.hpp>
#endif

#include <QtCore/QRunnable>
#include <QtCore/QMutex>

#include "Global/GlobalDefines.h"
#include "Engine/EngineFwd.h"
#include "Engine/TimeValue.h"
#include "Engine/ViewIdx.h"

NATRON_NAMESPACE_ENTER

struct RenderThreadTaskPrivate;

/**
 * @brief Runnable task interface executed on the main thread pool that renders a given frame and views for the given node
 **/
class RenderThreadTask : public QRunnable
{
public:


    RenderThreadTask(const NodePtr& output,
                     OutputSchedulerThread* scheduler,
                     const TimeValue time,
                     const bool useRenderStats,
                     const std::vector<ViewIdx>& viewsToRender);

    virtual ~RenderThreadTask();

    OutputSchedulerThread* getScheduler() const;

    virtual void abortRender() = 0;

    virtual void run() OVERRIDE FINAL;

protected:

    /**
     * @brief Must render the frame
     **/
    virtual void renderFrame(TimeValue time, const std::vector<ViewIdx>& viewsToRender, bool enableRenderStats) = 0;
    boost::scoped_ptr<RenderThreadTaskPrivate> _imp;
};

/**
 * @brief Implementation of RenderThreadTask for all render nodes (Writer, DiskCache, etc...) 
 **/
class DefaultRenderFrameRunnable
: public RenderThreadTask
{

protected:

    mutable QMutex renderObjectsMutex;
    std::list<TreeRenderPtr> renderObjects;

public:
    DefaultRenderFrameRunnable(const NodePtr& writer,
                               OutputSchedulerThread* scheduler,
                               const TimeValue time,
                               const bool useRenderStats,
                               const std::vector<ViewIdx>& viewsToRender);


    virtual ~DefaultRenderFrameRunnable()
    {
    }

    virtual void abortRender() OVERRIDE;

private:

    virtual void renderFrame(TimeValue time,
                             const std::vector<ViewIdx>& viewsToRender,
                             bool enableRenderStats) OVERRIDE;

    void runBeforeFrameRenderCallback(TimeValue frame, const NodePtr& outputNode);

protected:

    ActionRetCodeEnum renderFrameInternal(NodePtr outputNode,
                                          TimeValue time,
                                          ViewIdx view,
                                          const RenderStatsPtr& stats,
                                          ImagePtr* imagePlane);

};



NATRON_NAMESPACE_EXIT

#endif // RENDERTHREADTASK_H
