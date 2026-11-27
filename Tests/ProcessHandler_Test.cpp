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

// A render in a separate process ends (ProcessHandler::processFinished()) when its process fails to start:
// QProcess emits finished() only for a process that started, so the render stayed in the render queue
// forever (app.waitForRenders(), hasRenderInProgress(), the renders queued behind it, #1596).
// The output pipe of a background render process (ProcessInputChannel) is written by the render threads: the
// messages must all arrive, without Qt warnings about sockets used from another thread (#1887).

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <atomic>
#include <cstdio>
#include <cstring>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QElapsedTimer>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QThread>
#include <QtCore/QtGlobal> // qInstallMessageHandler
#include <QtNetwork/QLocalServer>
#include <QtNetwork/QLocalSocket>

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/Node.h"
#include "Engine/OutputEffectInstance.h"
#include "Engine/ProcessHandler.h"
#include "Engine/Project.h"

#include "TestUtils.h"

NATRON_NAMESPACE_USING
using NatronTest::JoiningThread;
using NatronTest::Watchdog;

namespace {
// Records the Qt messages (qDebug(), qWarning()...) printed on any thread while it exists, and prints them as
// before. The threads that may print must have finished before it is destroyed.
class QtMessageRecorder
{
public:
    QtMessageRecorder()
    {
        s_instance = this;
        _previous = qInstallMessageHandler(&QtMessageRecorder::handler);
    }

    ~QtMessageRecorder()
    {
        qInstallMessageHandler(_previous);
        s_instance = nullptr;
    }

    QtMessageRecorder(const QtMessageRecorder&) = delete;
    QtMessageRecorder& operator=(const QtMessageRecorder&) = delete;

    std::vector<std::string> messages() const
    {
        std::lock_guard<std::mutex> l(_mutex);

        return _messages;
    }

private:
    static void handler(QtMsgType type,
                        const QMessageLogContext& context,
                        const QString& message)
    {
        QtMessageRecorder* recorder = s_instance;

        if (!recorder) {
            return;
        }
        {
            std::lock_guard<std::mutex> l(recorder->_mutex);
            recorder->_messages.push_back( message.toStdString() );
        }
        if (recorder->_previous) {
            recorder->_previous(type, context, message);
        } else {
            std::fprintf( stderr, "%s\n", message.toLocal8Bit().constData() );
        }
    }

    static std::atomic<QtMessageRecorder*> s_instance;
    QtMessageHandler _previous = nullptr;
    mutable std::mutex _mutex; // guards _messages
    std::vector<std::string> _messages;
};

std::atomic<QtMessageRecorder*> QtMessageRecorder::s_instance(nullptr);

// A QThread (the socket notifiers of a QLocalSocket need one: "QSocketNotifier: Can only be used with threads
// started with QThread") that runs f, and is waited for when destroyed, also when the test exits early (CP.25).
class JoiningQThread
{
    std::unique_ptr<QThread> _thread;

public:
    explicit JoiningQThread(const std::function<void()>& f)
        : _thread( QThread::create(f) )
    {
        _thread->start();
    }

    ~JoiningQThread()
    {
        _thread->wait();
    }

    JoiningQThread(const JoiningQThread&) = delete;
    JoiningQThread& operator=(const JoiningQThread&) = delete;
};
} // anonymous namespace

TEST(ProcessHandler, FailedToStartFinishesTheRender)
{
    AppInstancePtr app = appPTR->getTopLevelInstance();
    ASSERT_TRUE( bool(app) );
    CreateNodeArgs args( PLUGINID_NATRON_DISKCACHE, app->getProject() );
    args.setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);
    args.setProperty<bool>(kCreateNodeArgsPropAutoConnect, false);
    args.setProperty<bool>(kCreateNodeArgsPropSettingsOpened, false);
    NodePtr node = app->createNode(args);
    ASSERT_TRUE( bool(node) );
    OutputEffectInstance* writer = dynamic_cast<OutputEffectInstance*>( node->getEffectInstance().get() );
    ASSERT_TRUE(writer);

    std::vector<int> returnCodes;
    {
        ProcessHandler handler(QString::fromUtf8("unused.ntp"), writer);
        QObject::connect( &handler, &ProcessHandler::processFinished, [&returnCodes](int retCode) {
            returnCodes.push_back(retCode);
        } );

        // what QProcess::errorOccurred() calls when the process can't be started; the render ends from the
        // event loop, after QProcess has returned
        handler.onProcessError(QProcess::FailedToStart);
        EXPECT_TRUE( returnCodes.empty() );
        QCoreApplication::processEvents();
        ASSERT_EQ( returnCodes.size(), 1u );
        EXPECT_EQ(returnCodes[0], 1);

        // the other errors happen while the process runs: finished() follows (or the process keeps running)
        handler.onProcessError(QProcess::Crashed);
        handler.onProcessError(QProcess::ReadError);
        QCoreApplication::processEvents();
        EXPECT_EQ( returnCodes.size(), 1u );
    }

    node->destroyNode(true, false);
}

// What the main process (ProcessHandler) does with the pipes of a background render process, on another thread:
// accept the output pipe of the ProcessInputChannel, connect to its input server when it gives its name, and read
// the messages. The render threads of the background process write them with writeToOutputChannel().
TEST(ProcessInputChannel, MessagesFromRenderThreads)
{
    const int nThreads = 4;
    const int nMessages = 50;
    const QString serverName = QString::fromUtf8("NatronProcessInputChannelTest_%1").arg( QCoreApplication::applicationPid() );
    QtMessageRecorder recorder;

    // written by the main process thread, read after it has been joined
    bool gotServerName = false;
    bool inputConnected = false;
    std::vector<std::string> received;
    std::promise<bool> listening;
    std::future<bool> isListening = listening.get_future();
    {
        // a hang (on Windows, destroying the sockets of the channel waited forever for a read that Qt 5 completes
        // only on the thread that started it) aborts the Tests binary after 60 s, naming this test, instead of
        // running until the ctest timeout
        Watchdog watchdog("ProcessInputChannel.MessagesFromRenderThreads");
        JoiningQThread mainProcess([&]() {
            QLocalServer::removeServer(serverName); // left by a crashed run (Unix)
            QLocalServer server;
            const bool ok = server.listen(serverName);
            listening.set_value(ok);
            if ( !ok || !server.waitForNewConnection(10000) ) {
                return;
            }
            QLocalSocket* output = server.nextPendingConnection(); // a child of server
            QLocalSocket input;
            QElapsedTimer timer;
            timer.start();
            // until all the messages arrived, the channel closed its pipe, or 30 s
            while ( (int)received.size() < nThreads * nMessages && timer.elapsed() < 30000 ) {
                if ( !output->canReadLine() && !output->waitForReadyRead(100) ) {
                    if ( (output->state() != QLocalSocket::ConnectedState) && !output->canReadLine() ) {
                        break;
                    }
                    continue;
                }
                while ( output->canReadLine() ) {
                    std::string line = QString::fromUtf8( output->readLine() ).trimmed().toStdString();
                    if (line.compare(0, std::strlen(kBgProcessServerCreatedShort), kBgProcessServerCreatedShort) == 0) {
                        gotServerName = true;
                        input.connectToServer( QString::fromStdString( line.substr( std::strlen(kBgProcessServerCreatedShort) ) ) );
                        inputConnected = input.waitForConnected(5000);
                    } else {
                        received.push_back(line);
                    }
                }
            }
        });
        ASSERT_TRUE( isListening.get() ) << "QLocalServer::listen(" << serverName.toStdString() << ") failed";

        {
            // connects its output pipe and waits (at most 5 s) for the main process to connect to its input server
            ProcessInputChannel channel(serverName);
            std::vector<std::unique_ptr<JoiningThread> > writers;
            for (int t = 0; t < nThreads; ++t) {
                writers.emplace_back( new JoiningThread([&, t]() {
                    for (int i = 0; i < nMessages; ++i) {
                        channel.writeToOutputChannel( QString::fromUtf8("message %1 %2").arg(t).arg(i) );
                    }
                }) );
            }
            writers.clear(); // join
        } // the destructor sends the pending messages and closes the pipes
    } // join

    EXPECT_TRUE(gotServerName) << "the channel did not send the name of its input server";
    EXPECT_TRUE(inputConnected) << "could not connect to the input server of the channel";
    std::set<std::string> unique( received.begin(), received.end() );
    EXPECT_EQ( (std::size_t)(nThreads * nMessages), received.size() );
    EXPECT_EQ( received.size(), unique.size() ) << "a message arrived twice";
    for (int t = 0; t < nThreads; ++t) {
        for (int i = 0; i < nMessages; ++i) {
            EXPECT_EQ( 1u, unique.count( "message " + std::to_string(t) + ' ' + std::to_string(i) ) ) << "message " << t << ' ' << i;
        }
    }
    for (const std::string& message : recorder.messages()) {
        EXPECT_EQ(std::string::npos, message.find("another thread")) << message;
        EXPECT_EQ(std::string::npos, message.find("Cannot queue arguments")) << message;
        EXPECT_EQ(std::string::npos, message.find("QSocketNotifier")) << message;
    }
}
