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

#ifndef TRACKSCHEDULER_H
#define TRACKSCHEDULER_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#endif

#include "Global/Macros.h"
#include "Engine/GenericSchedulerThread.h"

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

/**
 * @brief An implementation of the GenericSchedulerThread class to enable multi-threaded tracking.
 **/
class IsTrackingFlagSetter_RAII;
struct TrackSchedulerPrivate;
class TrackScheduler
: public GenericSchedulerThread
{
    GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
    GCC_DIAG_SUGGEST_OVERRIDE_ON

public:


    TrackScheduler(const TrackerParamsProviderBasePtr& paramsProvider);

    virtual ~TrackScheduler();

    /**
     * @brief The main entry point used to launch track tasks.
     **/
    void track(const TrackArgsBasePtr& args);


private Q_SLOTS:

    void doRenderCurrentFrameForViewer(const ViewerNodePtr& viewer);


Q_SIGNALS:

    void trackingStarted(int step);

    void trackingFinished();

    void trackingProgress(double progress);

    void renderCurrentFrameForViewer(const ViewerNodePtr& viewer);

private:

    void emit_trackingStarted(int step)
    {
        Q_EMIT trackingStarted(step);
    }

    void emit_trackingFinished( )
    {
        Q_EMIT trackingFinished();
    }

    virtual TaskQueueBehaviorEnum tasksQueueBehaviour() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return eTaskQueueBehaviorSkipToMostRecent;
    }

    virtual ThreadStateEnum threadLoopOnce(const GenericThreadStartArgsPtr& inArgs) OVERRIDE FINAL WARN_UNUSED_RETURN;

    friend class IsTrackingFlagSetter_RAII;
    boost::scoped_ptr<TrackSchedulerPrivate> _imp;
};



NATRON_NAMESPACE_EXIT

#endif // TRACKSCHEDULER_H
