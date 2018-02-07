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

#ifndef NATRON_ENGINE_RENDERENGINE_H
#define NATRON_ENGINE_RENDERENGINE_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <QObject>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#endif

#include "Engine/EngineFwd.h"
#include "Global/Enums.h"
#include "Engine/TimeValue.h"
#include "Engine/ViewIdx.h"



NATRON_NAMESPACE_ENTER

/**
 * @brief This class manages is a wrapper over OutputThreadScheduler and ViewerCurrentFrameRequestScheduler so that each render request gets processed as soon as possible.
 * This is the class that should be used externally
 **/
struct RenderEnginePrivate;
class RenderEngine
: public QObject
, public boost::enable_shared_from_this<RenderEngine>
{
    Q_OBJECT

    friend class OutputSchedulerThread;
    friend class ViewerDisplayScheduler;
    friend RenderEnginePtr boost::make_shared<RenderEngine>(const NodePtr&);

protected:
    // used by boost::make_shared
    RenderEngine(const NodePtr& output);

public:
    static RenderEnginePtr create(const NodePtr& output)
    {
        return boost::make_shared<RenderEngine>(output);
    }

    virtual ~RenderEngine();

    NodePtr getOutput() const;

    /**
     * @brief Call this to render from firstFrame to lastFrame included.
     **/
    void renderFrameRange(bool isBlocking,
                          bool enableRenderStats,
                          TimeValue firstFrame,
                          TimeValue lastFrame,
                          TimeValue frameStep,
                          const std::vector<ViewIdx>& viewsToRender,
                          RenderDirectionEnum forward);

    /**
     * @brief Same as renderFrameRange except that the frame range will be computed automatically and it will
     * start from the current frame.
     * This is not appropriate to call this function from a writer.
     **/
    void renderFromCurrentFrame(bool enableRenderStats,
                                const std::vector<ViewIdx>& viewsToRender,
                                RenderDirectionEnum forward);


    /**
     * @brief Basically it just renders with the current frame on the timeline.
     **/
    void renderCurrentFrame();

    /**
     * @brief Same as renderCurrentFrame, except that it does not concatenate render requests and starts the render
     * request now. On the main-thread, if a lot of render request are to be made, pref using renderCurrentFrame
     **/
    void renderCurrentFrameNow();

private:

    void renderCurrentFrameInternal(bool enableStats);

    void renderCurrentFrameNowInternal(bool enableStats);
public:


    /**
     * @brief Same as renderCurrentFrame() but also enables detailed render timing for each node
     **/
    void renderCurrentFrameWithRenderStats();

    /**
     * @brief Whether the playback can be automatically restarted by a single render request
     **/
    bool isPlaybackAutoRestartEnabled() const;

    /**
     * @brief Set auto-playback restart enabled
     **/
    void setPlaybackAutoRestartEnabled(bool enabled);

    /**
     * @brief Returns the playback mode of the internal scheduler
     **/
    PlaybackModeEnum getPlaybackMode() const;

    /**
     * @brief Returns the desired user FPS that the internal scheduler should stick to
     **/
    double getDesiredFPS() const;

    /**
     * @brief Quit all processing, making sure all threads are finished, this is not blocking
     **/
    void quitEngine(bool allowRestarts);

    /**
     * @brief Blocks the calling thread until all threads are finished, do not call from the main-thread!
     **/
    void waitForEngineToQuit_not_main_thread();

    /**
     * @brief Same as waitForEngineToQuit_not_main_thread except that this function is not blocking and should be only called
     * from the main-thread. You will get notified with the signal engineQuit when the engine really quits.
     **/
    void waitForEngineToQuit_main_thread(bool allowRestart);

    /**
     * @brief Use this as last resort, this will block the caller thread until all threads have finished
     **/
    void waitForEngineToQuit_enforce_blocking();


    /**
     * @brief Aborts all computations. This is not blocking.
     * @returns False if the no thread was currently running, true if the abort request was taken into account.
     * The AutoRestart version attempts to restart the playback upon a call to renderCurrentFrame
     **/
    bool abortRenderingAutoRestart();
    bool abortRenderingNoRestart();

private:

    bool abortRenderingInternal(bool keepOldestRender);

public:

    /**
     * @brief Blocks the calling thread until all renders are aborted, do not call from the main-thread!
     **/
    void waitForAbortToComplete_not_main_thread();

    /**
     * @brief Same as waitForAbortToComplete_not_main_thread except that this function is not blocking and should be only called
     * from the main-thread. You will get notified with the signal engineAborted when the engine really quits.
     **/
    void waitForAbortToComplete_main_thread();

    /**
     * @brief Use this as last resort, this will block the caller thread until abort is completed
     **/
    void waitForAbortToComplete_enforce_blocking();

    /**
     * @brief Returns true if threads owned by the engine are still alive
     **/
    bool hasThreadsAlive() const;

    /**
     * @brief Returns true if a render is active from this RenderEngine
     **/
    bool hasActiveRender() const;

    /**
     * @brief Returns true if playback is active
     **/
    bool isDoingSequentialRender() const;


    /**
     * The following functions are called by the OutputThreadScheduler to Q_EMIT the corresponding signals
     **/
    void s_fpsChanged(double actual,
                      double desired) { Q_EMIT fpsChanged(actual, desired); }

    void s_frameRendered(int time,
                         double progress) { Q_EMIT frameRendered(time, progress); }

    void s_renderStarted(bool forward) { Q_EMIT renderStarted(forward); }

    void s_renderFinished(int retCode) { Q_EMIT renderFinished(retCode); }

    void s_refreshAllKnobs() { Q_EMIT refreshAllKnobs(); }

    virtual void reportStats(TimeValue time,
                             const RenderStatsPtr& stats);


    public Q_SLOTS:

    void abortRendering_non_blocking()
    {
        abortRenderingNoRestart();
    }

    /**
     * @brief Set the playback mode
     * @param mode Corresponds to the PlaybackModeEnum enum
     **/
    void setPlaybackMode(int mode);


    /**
     *@brief The slot called by the GUI to set the requested fps.
     **/
    void setDesiredFPS(double d);

    void onCurrentFrameRenderRequestPosted();

    void onWatcherEngineAbortedEmitted();

    void onWatcherEngineQuitEmitted();

Q_SIGNALS:

    /**
     * @brief Emitted when the fps has changed
     * This will not be emitted after calling renderCurrentFrame
     **/
    void fpsChanged(double actualFps, double desiredFps);

    /**
     * @brief Emitted after a frame is rendered.
     * This will not be emitted after calling renderCurrentFrame
     **/
    void frameRendered(int time, double progress);


    /**
     * @brief Emitted when the stopRender() function is called
     * @param retCode Will be set to 1 if the render was finished because it was aborted, 0 otherwise.
     * This will not be emitted after calling renderCurrentFrame
     **/
    void renderFinished(int retCode);


    void renderStarted(bool forward);

    // Emitted after calling waitForAbortToComplete_main_thread
    void engineAborted();

    // Emitted after calling waitForEngineToQuit_main_thread
    void engineQuit();

    /**
     * @brief Emitted when gui is frozen and rendering is finished to update all knobs
     **/
    void refreshAllKnobs();

    void currentFrameRenderRequestPosted();


protected:


    /**
     * @brief Must create the main-scheduler that will be used for scheduling playback/writing on disk.
     **/
    virtual OutputSchedulerThreadPtr createScheduler(const NodePtr& effect);

private:



    boost::scoped_ptr<RenderEnginePrivate> _imp;
};

/**
 * @brief An implementation of RenderEngine that uses the ViewerDisplayScheduler instead of the DefaultScheduler.
 * This class is intended to be used by a viewer node only.
 **/
class ViewerRenderEngine
: public RenderEngine
{
    friend boost::shared_ptr<ViewerRenderEngine> boost::make_shared<ViewerRenderEngine,const NodePtr&>(const NodePtr&);

protected:
    // used by boost::make_shared
    ViewerRenderEngine(const NodePtr& output)
    : RenderEngine(output)
    {}
public:

    static RenderEnginePtr create(const NodePtr& output)
    {
        return boost::make_shared<ViewerRenderEngine>(output);
    }

    virtual ~ViewerRenderEngine() {}
    
    virtual void reportStats(TimeValue time,
                             const RenderStatsPtr& stats) OVERRIDE FINAL;
private:
    
    virtual OutputSchedulerThreadPtr createScheduler(const NodePtr& effect) OVERRIDE FINAL WARN_UNUSED_RETURN;
};

NATRON_NAMESPACE_EXIT

#endif // NATRON_ENGINE_RENDERENGINE_H
