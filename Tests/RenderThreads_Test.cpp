/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2026 The Natron developers
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

// The render threads of a sequence render (OutputSchedulerThread, #248), with the test hooks of
// OutputSchedulerThread::setTestHooks(): adjustNumberOfThreads() stops and starts a render thread
// alternately, and the render threads and stopRender() wait at the critical points, to widen the windows
// of the races:
// - a thread told to quit took a frame and quit without rendering it: the render never finished;
// - a thread finishing a frame during stopRender() started a new thread, which was never told to quit:
//   stopRender() waited for it forever.
// The output is a DiskCache node, a built-in writer, after net.natron.test.Params (whose render does nothing).

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <memory>
#include <mutex>
#include <set>

#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QThread>

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/EffectInstance.h"
#include "Engine/Node.h"
#include "Engine/OutputEffectInstance.h"
#include "Engine/OutputSchedulerThread.h"
#include "Engine/Project.h"

#include "OfxTestPlugin.h"

NATRON_NAMESPACE_USING

#ifndef NATRON_PLAYBACK_USES_THREAD_POOL
namespace {
// The frames reported by RenderEngine::frameRendered, from several render threads
struct RenderedFrames
{
    std::mutex mutex;
    std::set<int> frames;
};

// Disconnects at the end of the scope, also when an ASSERT returns early
class ConnectionGuard
{
public:
    explicit ConnectionGuard(const QMetaObject::Connection& c)
        : _c(c)
    {
    }

    ~ConnectionGuard()
    {
        QObject::disconnect(_c);
    }

private:
    QMetaObject::Connection _c;
};

class RenderThreadsTest
    : public ::testing::Test
{
protected:
    NodePtr _source, _output;

    virtual void SetUp() OVERRIDE
    {
        if ( !NatronTestPlugin::isAvailable(kNatronTestPluginParams) ) {
            return;
        }
        AppInstancePtr app = appPTR->getTopLevelInstance();
        {
            CreateNodeArgs args( kNatronTestPluginParams, app->getProject() );
            args.setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);
            args.setProperty<bool>(kCreateNodeArgsPropAutoConnect, false);
            _source = app->createNode(args);
        }
        {
            CreateNodeArgs args( PLUGINID_NATRON_DISKCACHE, app->getProject() );
            args.setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);
            args.setProperty<bool>(kCreateNodeArgsPropAutoConnect, false);
            _output = app->createNode(args);
        }
        if (_source && _output) {
            _output->connectInput(_source, 0);
        }
        OutputSchedulerThread::setTestHooks(true, 300);
    }

    virtual void TearDown() OVERRIDE
    {
        OutputSchedulerThread::setTestHooks(false, 0);
        if (_output) {
            _output->destroyNode(true, false);
            _output.reset();
        }
        if (_source) {
            _source->destroyNode(true, false);
            _source.reset();
        }
    }

    bool skipped()
    {
        if (!_source) {
            NatronTestPlugin::reportSkipped(kNatronTestPluginParams);

            return true;
        }

        return false;
    }

    OutputEffectInstance* output() const
    {
        return dynamic_cast<OutputEffectInstance*>( _output->getEffectInstance().get() );
    }

    // Waits until the render is not running anymore; returns false after timeoutMs
    bool waitForRenderEnd(int timeoutMs) const
    {
        QElapsedTimer timer;
        timer.start();
        while ( output()->isDoingSequentialRender() ) {
            if (timer.elapsed() > timeoutMs) {
                return false;
            }
            QCoreApplication::processEvents();
            QThread::msleep(1);
        }

        return true;
    }

    // Starts a non-blocking render, and waits until it has started
    void startRender(int first,
                     int last)
    {
        output()->renderFullSequence(false, false, NULL, first, last, 1);
        QElapsedTimer timer;
        timer.start();
        while ( !output()->isDoingSequentialRender() && timer.elapsed() < 5000 ) {
            QCoreApplication::processEvents();
        }
    }
};
} // anonymous namespace

// Every frame is rendered, and the render finishes
TEST_F(RenderThreadsTest, NoFrameIsDropped)
{
    if ( skipped() ) {
        return;
    }
    ASSERT_TRUE( _output && output() );
    for (int i = 0; i < 10; ++i) {
        const int nFrames = 500;
        // other frames each time, so that they are not in the cache
        RenderEnginePtr engine = output()->getRenderEngine();
        ASSERT_TRUE( bool(engine) );
        // frameRendered is emitted by the render threads (free-for-all scheduling), and a connection
        // without a context object is direct: the slot runs in those threads, at the same time.
        std::shared_ptr<RenderedFrames> rendered = std::make_shared<RenderedFrames>();
        ConnectionGuard c( QObject::connect( engine.get(), &RenderEngine::frameRendered, [rendered](int time, double) {
            std::lock_guard<std::mutex> l(rendered->mutex);
            rendered->frames.insert(time);
        } ) );
        startRender(1 + i * nFrames, (i + 1) * nFrames);
        ASSERT_TRUE( waitForRenderEnd(30000) ) << "the render stalled (iteration " << i << ")";
        std::size_t nRendered;
        {
            std::lock_guard<std::mutex> l(rendered->mutex);
            nRendered = rendered->frames.size();
        }
        EXPECT_EQ( (std::size_t)nFrames, nRendered ) << "iteration " << i;
    }
}

// Aborting the render while the render threads stop and start, many times: the render stops
TEST_F(RenderThreadsTest, AbortStopsTheRender)
{
    if ( skipped() ) {
        return;
    }
    ASSERT_TRUE( _output && output() );
    // longer waits, so that a thread finishing a frame adjusts the number of threads during stopRender()
    OutputSchedulerThread::setTestHooks(true, 2000);
    for (int i = 0; i < 20; ++i) {
        startRender(1 + i * 100000, (i + 1) * 100000);
        // let a few frames render
        QElapsedTimer timer;
        timer.start();
        while ( timer.elapsed() < 50 ) {
            QCoreApplication::processEvents();
        }
        RenderEnginePtr engine = output()->getRenderEngine();
        ASSERT_TRUE( bool(engine) );
        engine->abortRenderingNoRestart();
        ASSERT_TRUE( waitForRenderEnd(30000) ) << "the render did not stop after the abort (iteration " << i << ")";
    }
}
#endif // ifndef NATRON_PLAYBACK_USES_THREAD_POOL
