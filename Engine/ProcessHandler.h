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

#ifndef PROCESSHANDLER_H
#define PROCESSHANDLER_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
#include <QProcess>
#include <QThread>
#include <QStringList>
#include <QString>
CLANG_DIAG_ON(deprecated)

#include "Global/GlobalDefines.h"

#include "Engine/EngineFwd.h"


/**
 * @brief This class represents a background render process. It starts a render and reports progress via a
 * progress dialog. This class encaspulates an IPC server (a named pipe) where the render process can write to
 * in order to communicate withe the main process (the GUI app).
 * @see ProcessInputChannel represents the "input" pipe of the background process, this is where the background
 * app expect messages from the "main" process to come. It listen to messages from the main app to take decisions.
 * For instance, the main app can ask the background process to terminate via this channel.
 **/


/**
 * Here is a schema resuming how the ProcessHandler and ProcessInputChannel class works together:
 * - ProcessHandler belongs to the main (GUI) process
 * - ProcessInputChannel belongs to the background process
 *
 * 1) The user of the main (GUI) process asks a new render, creating a new ProcessHandler which will
 * start a new process. Before the process is started, the main process has created a local server
 * used to receive the output messages of the background process.
 *
 * 2) Once the background process is started the first thing it does is creating a ProcessInputChannel.
 * This creates the output channel where it will write to and asks a connection to the server hosted by
 * the main process. It also creates a new local server which will serve to host an input channel and
 * listen to messages coming from the main process.
 *
 * 3) The background process waits for the main process to answer the connection request of the output channel.
 * Once it has replied, it will send a message (kBgProcessServerCreatedShort) meaning the main process should
 * open the input channel where it will write to (and the background process will listen to).
 *
 * 4) The main process creates the input channel in ProcessHandler::onDataWrittenToSocket
 *
 * 5) The background process catches the pending connection and accepts it.
 *
 * The IPC is setup, now both processes are listening to each-other on both sides.
 *
 * NB: Message that are exchanged via this channel consists of exactly 1 line, i.e a
 * string terminated with the \n character.
 **/
class ProcessHandler
    : public QObject
{
    Q_OBJECT

    AppInstance* _app; //< pointer to the app executing this process
    QProcess* _process; //< the process executing the render
    Natron::OutputEffectInstance* _writer; //< pointer to the writer that will render in the bg process
    QLocalServer* _ipcServer; //< the server for IPC with the background process
    QLocalSocket* _bgProcessOutputSocket; //< the socket where data is output by the process

    //the socket where data is read by the process
    //note that this socket is initialized only when the background process sends the message
    //kBgProcessServerCreatedShort, meaning it created its server for the input pipe and we can actually open it.
    QLocalSocket* _bgProcessInputSocket;
    bool _earlyCancel; //< true if the user pressed cancel but the _bgProcessInput socket was not created yet
    QString _processLog; //< used to record the log of the process
    QStringList _processArgs;
    
public:

    /**
     * @brief Starts a new process which will load the project specified by "projectPath".
     * The process will render using the effect specified by writer.
     **/
    ProcessHandler(AppInstance* app,
                   const QString & projectPath,
                   Natron::OutputEffectInstance* writer);

    virtual ~ProcessHandler();

    const QString & getProcessLog() const;

public Q_SLOTS:

    /**
     * @brief Called whenever the background process requests a new connection to this server,
     * i.e: this is when the background process wants to create its output pipe where it will write data to.
     **/
    void onNewConnectionPending();

    /**
     * @brief Slot called whenever the background process writes something to the output socket (pipe).
     **/
    void onDataWrittenToSocket();

    /**
     * @brief Called whenever the background process writes something to its standard output, its just for the sake
     * of redirecting and being able to see what it wrote.
     **/
    void onStandardOutputBytesWritten();

    /**
     * @brief Called whenever the background process writes something to its standard error, its just for the sake
     * of redirecting and being able to see what it wrote.
     **/
    void onStandardErrorBytesWritten();

    /**
     * @brief Called whenever the main GUI app clicked the cancel button of the progress dialog.
     * It sends a message to the background process via its input pipe to abort the ongoing render.
     **/
    void onProcessCanceled();

    /**
     * @brief Called on process error.
     **/
    void onProcessError(QProcess::ProcessError err);

    /**
     * @brief Called when the process finishes.
     **/
    void onProcessEnd(int exitCode,QProcess::ExitStatus stat);

    /**
     * @brief Called when the input pipe connection is successfully sealed.
     **/
    void onInputPipeConnectionMade();
    
    /**
     * @brief Start the process execution
     **/
    void startProcess();

Q_SIGNALS:

    void deleted();

    void frameRendered(int);
    
    void frameRenderedWithTimer(int frame, double timeSpentForFrame, double timeRemaining);

    void frameProgress(int);

    void processCanceled();

    /**
     * @brief Emitted when the process terminates. The parameter contains a return code:
     * 0: Everything went OK
     * 1: Underminated error
     * 2: Crash.
     **/
    void processFinished(int);
};

/**
 * @brief This class represents the "input" pipe of the background process, this is where the background
 * app expect messages from the "main" process to come. It listen to messages from the main app to take decisions.
 * For instance, the main app can ask the background process to terminate via this channel.
 * This class works hand in hand with the ProcessHandler class: it takes in parameter the name of the server created
 * by the ProcessHandler, and creates the output channel where it will write data to. Also it creates a local server
 * and notifies the ProcessHandler it has done so, so the ProcessHandler creates the input channel where the main process
 * will write data to.
 **/
class ProcessInputChannel
    : public QThread
{
    Q_OBJECT

public:

    /**
     * @brief Creates a new ProcessInputChannel effectively starting a new thread in order to have our own event loop.
     * This is required in order to listen properly to the incoming messages.
     **/
    ProcessInputChannel(const QString & mainProcessServerName);

    virtual ~ProcessInputChannel();

    /**
     * @brief Call it if you want to write something to the background process output channel.
     **/
    void writeToOutputChannel(const QString & message);

public Q_SLOTS:

    /**
     * @brief Called when the main process wants to create the input channel.
     **/
    void onNewConnectionPending();

    /**
     * @brief Called whenever the main process writes something to the background process' input channel.
     * @returns True if the input channel should close, false otherwise.
     **/
    bool onInputChannelMessageReceived();

    /**
     * @brief Called when the output pipe connection is successfully sealed.
     **/
    void onOutputPipeConnectionMade();

private:

    /**
     * @brief Runs the event loop the signal/slots are caught correctly.
     **/
    virtual void run();

    /**
     * @brief Called once the first time run is started.
     * Post-condition: The output channel is created and you can write to it via the
     * writeToOutputChannel(QString) function. Also the local server as been created
     * and the main process should have replied with a connection request to create the input channel.
     **/
    void initialize();

    QString _mainProcessServerName;
    QMutex* _backgroundOutputPipeMutex;
    QLocalSocket* _backgroundOutputPipe; //< if the process is background but managed by a gui process then this
                                         //pipe is used to output messages
    QLocalServer* _backgroundIPCServer; //< for a background app used to manage input IPC  with the gui app
    QLocalSocket* _backgroundInputPipe; //<if the process is bg but managed by a gui process then the pipe is used
                                        //to read input messages
    QMutex _mustQuitMutex;
    QWaitCondition _mustQuitCond;
    bool _mustQuit;
};

#endif // PROCESSHANDLER_H
