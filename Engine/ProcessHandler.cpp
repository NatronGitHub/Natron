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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "ProcessHandler.h"

#include <stdexcept>

#include <QProcess>
#include <QLocalServer>
#include <QLocalSocket>
#include <QCoreApplication>
#include <QTemporaryFile>
#include <QWaitCondition>
#include <QMutex>
#include <QDir>
#include <QDebug>

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/Node.h"
#include "Engine/OutputEffectInstance.h"

NATRON_NAMESPACE_ENTER;

ProcessHandler::ProcessHandler(AppInstance* app,
                               const QString & projectPath,
                               OutputEffectInstance* writer)
    : _app(app)
      ,_process(new QProcess)
      ,_writer(writer)
      ,_ipcServer(0)
      ,_bgProcessOutputSocket(0)
      ,_bgProcessInputSocket(0)
      ,_earlyCancel(false)
      ,_processLog()
      ,_processArgs()
{
    ///setup the server used to listen the output of the background process
    _ipcServer = new QLocalServer();
    QObject::connect( _ipcServer,SIGNAL( newConnection() ),this,SLOT( onNewConnectionPending() ) );
    QString serverName;
    {
        QTemporaryFile tmpf(NATRON_APPLICATION_NAME "_OUTPUT_PIPE_");
        tmpf.open();
        serverName = tmpf.fileName();
        tmpf.remove();
    }
    _ipcServer->listen(serverName);


    _processArgs << "-b" << "-w" << writer->getScriptName_mt_safe().c_str();
    _processArgs << "--IPCpipe" << QString("\"") + _ipcServer->fullServerName() + QString("\"");
    _processArgs << QString("\"") + projectPath + QString("\"");

    ///connect the useful slots of the process
    QObject::connect( _process,SIGNAL( readyReadStandardOutput() ),this,SLOT( onStandardOutputBytesWritten() ) );
    QObject::connect( _process,SIGNAL( readyReadStandardError() ),this,SLOT( onStandardErrorBytesWritten() ) );
    QObject::connect( _process,SIGNAL( error(QProcess::ProcessError) ),this,SLOT( onProcessError(QProcess::ProcessError) ) );
    QObject::connect( _process,SIGNAL( finished(int,QProcess::ExitStatus) ),this,SLOT( onProcessEnd(int,QProcess::ExitStatus) ) );


    ///start the process
    _processLog.push_back( "Starting background rendering: " + QCoreApplication::applicationFilePath() );
    _processLog.push_back(" ");
    for (int i = 0; i < _processArgs.size(); ++i) {
        _processLog.push_back(_processArgs[i] + " ");
    }
   
}

ProcessHandler::~ProcessHandler()
{
    Q_EMIT deleted();

    if (_ipcServer) {
        _ipcServer->close();
        delete _ipcServer;
    }
    if (_bgProcessInputSocket) {
        _bgProcessInputSocket->close();
        delete _bgProcessInputSocket;
    }
    if (_process) {
        _process->close();
        delete _process;
    }
}

void
ProcessHandler::startProcess()
{
     _process->start(QCoreApplication::applicationFilePath(),_processArgs);
}

const QString &
ProcessHandler::getProcessLog() const
{
    return _processLog;
}

void
ProcessHandler::onNewConnectionPending()
{
    ///accept only 1 connection!
    if (_bgProcessOutputSocket) {
        return;
    }

    _bgProcessOutputSocket = _ipcServer->nextPendingConnection();

    QObject::connect( _bgProcessOutputSocket, SIGNAL( readyRead() ), this, SLOT( onDataWrittenToSocket() ) );
}

void
ProcessHandler::onDataWrittenToSocket()
{
    ///always running in the main thread
    assert( QThread::currentThread() == qApp->thread() );

    QString str = _bgProcessOutputSocket->readLine();
    while ( str.endsWith('\n') ) {
        str.chop(1);
    }
    _processLog.append("Message received: " + str + '\n');
    if ( str.startsWith(kFrameRenderedStringShort) ) {
        str = str.remove(kFrameRenderedStringShort);
        
        if (!str.isEmpty()) {
            if (!str.contains(';')) {
                //The report does not have extended timer infos
                Q_EMIT frameRendered( str.toInt() );
            } else {
                QStringList splits = str.split(';');
                if (splits.size() == 3) {
                    Q_EMIT frameRenderedWithTimer(splits[0].toInt(), splits[1].toDouble(), splits[2].toDouble());
                } else {
                    if (!splits.isEmpty()) {
                        Q_EMIT frameRendered(splits[0].toInt());
                    }
                }
            }
        }
        
    } else if ( str.startsWith(kRenderingFinishedStringShort) ) {
        ///don't do anything
    } else if ( str.startsWith(kProgressChangedStringShort) ) {
        str = str.remove(kProgressChangedStringShort);
        Q_EMIT frameProgress( str.toInt() );
    } else if ( str.startsWith(kBgProcessServerCreatedShort) ) {
        str = str.remove(kBgProcessServerCreatedShort);
        ///the bg process wants us to create the pipe for its input
        if (!_bgProcessInputSocket) {
            _bgProcessInputSocket = new QLocalSocket();
            QObject::connect( _bgProcessInputSocket, SIGNAL( connected() ), this, SLOT( onInputPipeConnectionMade() ) );
            _bgProcessInputSocket->connectToServer(str,QLocalSocket::ReadWrite);
        }
    } else if ( str.startsWith(kRenderingStartedShort) ) {
        ///if the user pressed cancel prior to the pipe being created, wait for it to be created and send the abort
        ///message right away
        if (_earlyCancel) {
            _bgProcessInputSocket->waitForConnected(5000);
            _earlyCancel = false;
            onProcessCanceled();
        }
    } else {
        _processLog.append("Error: Unable to interpret message.\n");
        throw std::runtime_error("ProcessHandler::onDataWrittenToSocket() received erroneous message");
    }
}

void
ProcessHandler::onInputPipeConnectionMade()
{
    ///always running in the main thread
    assert( QThread::currentThread() == qApp->thread() );

    _processLog.append("The input channel (the one the bg process listens to) was successfully created and connected.\n");
}

void
ProcessHandler::onStandardOutputBytesWritten()
{
    QString str( _process->readAllStandardOutput().data() );
#ifdef DEBUG
    qDebug() << "Message(stdout):" << str;
#endif
    _processLog.append("Message(stdout): " + str) + '\n';
}

void
ProcessHandler::onStandardErrorBytesWritten()
{
    QString str( _process->readAllStandardError().data() );
#ifdef DEBUG
    qDebug() << "Message(stderr):" << str;
#endif
    _processLog.append("Error(stderr): " + str) + '\n';
}

void
ProcessHandler::onProcessCanceled()
{
    Q_EMIT processCanceled();

    if (!_bgProcessInputSocket) {
        _earlyCancel = true;
    } else {
        _bgProcessInputSocket->write( (QString(kAbortRenderingStringShort) + '\n').toUtf8() );
        _bgProcessInputSocket->flush();
    }
}

void
ProcessHandler::onProcessError(QProcess::ProcessError err)
{
    if (err == QProcess::FailedToStart) {
        Dialogs::errorDialog( _writer->getScriptName(),QObject::tr("The render process failed to start").toStdString() );
    } else if (err == QProcess::Crashed) {
        //@TODO: find out a way to get the backtrace
    }
}

void
ProcessHandler::onProcessEnd(int exitCode,
                             QProcess::ExitStatus stat)
{
    int returnCode = 0;

    if (stat == QProcess::CrashExit) {
        returnCode = 2;
    } else if (exitCode == 1) {
        returnCode = 1;
    }
    Q_EMIT processFinished(returnCode);
}

ProcessInputChannel::ProcessInputChannel(const QString & mainProcessServerName)
    : QThread()
      , _mainProcessServerName(mainProcessServerName)
      , _backgroundOutputPipeMutex(new QMutex)
      , _backgroundOutputPipe(0)
      , _backgroundIPCServer(0)
      , _backgroundInputPipe(0)
      , _mustQuitMutex()
      , _mustQuitCond()
      , _mustQuit(false)
{
    initialize();
    _backgroundIPCServer->moveToThread(this);
    start();
}

ProcessInputChannel::~ProcessInputChannel()
{
    if ( isRunning() ) {
        _mustQuit = true;
        while (_mustQuit) {
            _mustQuitCond.wait(&_mustQuitMutex);
        }
    }

    delete _backgroundIPCServer;
    delete _backgroundOutputPipeMutex;
    delete _backgroundOutputPipe;
}

void
ProcessInputChannel::writeToOutputChannel(const QString & message)
{
    {
        QMutexLocker l(_backgroundOutputPipeMutex);
        _backgroundOutputPipe->write( (message + '\n').toUtf8() );
        _backgroundOutputPipe->flush();
    }
}

void
ProcessInputChannel::onNewConnectionPending()
{
    if (_backgroundInputPipe) {
        ///listen to only 1 process!
        return;
    }
    _backgroundInputPipe = _backgroundIPCServer->nextPendingConnection();
    QObject::connect( _backgroundInputPipe, SIGNAL( readyRead() ), this, SLOT( onInputChannelMessageReceived() ) );
}

bool
ProcessInputChannel::onInputChannelMessageReceived()
{
    QString str( _backgroundInputPipe->readLine() );

    while ( str.endsWith('\n') ) {
        str.chop(1);
    }
    if ( str.startsWith(kAbortRenderingStringShort) ) {
        qDebug() << "Aborting render!";
        appPTR->abortAnyProcessing();

        return true;
    } else {
        std::cerr << "Error: Unable to interpret message: " << str.toStdString() << std::endl;
        throw std::runtime_error("ProcessInputChannel::onInputChannelMessageReceived() received erroneous message");
    }

    return false;
}

void
ProcessInputChannel::run()
{
    for (;; ) {
        if ( _backgroundInputPipe->waitForReadyRead(100) ) {
            if ( onInputChannelMessageReceived() ) {
                qDebug() << "Background process now closing the input channel...";

                return;
            }
        }

        QMutexLocker l(&_mustQuitMutex);
        if (_mustQuit) {
            _mustQuit = false;
            _mustQuitCond.wakeOne();

            return;
        }
    }
}

void
ProcessInputChannel::initialize()
{
    _backgroundOutputPipe = new QLocalSocket();
    QObject::connect( _backgroundOutputPipe, SIGNAL( connected() ), this, SLOT( onOutputPipeConnectionMade() ) );
    _backgroundOutputPipe->connectToServer(_mainProcessServerName,QLocalSocket::ReadWrite);
    std::cout << "Attempting connection to " << _mainProcessServerName.toStdString() << std::endl;

    _backgroundIPCServer = new QLocalServer();
    QObject::connect( _backgroundIPCServer,SIGNAL( newConnection() ),this,SLOT( onNewConnectionPending() ) );
    QString serverName;
    {
        QTemporaryFile tmpf( QDir::tempPath() + QDir::separator() + NATRON_APPLICATION_NAME "_INPUT_SOCKET"
                             + QString::number( QCoreApplication::applicationPid() ) );
        tmpf.open();
        serverName = tmpf.fileName();
        tmpf.remove();
    }
    _backgroundIPCServer->listen(serverName);

    if ( !_backgroundOutputPipe->waitForConnected(5000) ) { //< blocking, we wait for the server to respond
        std::cout << "WARNING: The GUI application failed to respond, canceling this process will not be possible"
            " unless it finishes or you kill it." << std::endl;
    }
    writeToOutputChannel( QString(kBgProcessServerCreatedShort) + _backgroundIPCServer->fullServerName() );

    ///we wait for the GUI app to connect its socket to this server, we let it 5 sec to reply
    _backgroundIPCServer->waitForNewConnection(5000);

    ///since we're still not returning the event loop, just process them manually in case
    ///Qt didn't caught the new connection pending.
    QCoreApplication::processEvents();
}

void
ProcessInputChannel::onOutputPipeConnectionMade()
{
    qDebug() << "The output channel was successfully created and connected.";
}

NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_ProcessHandler.cpp"
