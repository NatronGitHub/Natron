/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
 * Copyright (C) 2018-2020 The Natron developers
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

#include <cassert>
#include <stdexcept>

#include <QtCore/QtGlobal> // for Q_OS_*
#include <QtCore/QProcess>
#include <QtNetwork/QLocalServer>
#include <QtNetwork/QLocalSocket>
#include <QtCore/QCoreApplication>
#include <QtCore/QTemporaryFile>
#include <QtCore/QWaitCondition>
#include <QtCore/QMutex>
#include <QtCore/QDir>
#include <QtCore/QDebug>

#ifdef DEBUG
#include "Global/FloatingPointExceptions.h"
#endif
#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/Node.h"
#include "Engine/EffectInstance.h"

NATRON_NAMESPACE_ENTER

ProcessHandler::ProcessHandler(const QString & projectPath,
                               const NodePtr& writer)
    : _process(new QProcess)
    , _writer(writer)
    , _ipcServer(0)
    , _bgProcessOutputSocket(0)
    , _bgProcessInputSocket(0)
    , _earlyCancel(false)
    , _processLog()
    , _processArgs()
{
    ///setup the server used to listen the output of the background process
    _ipcServer = new QLocalServer();
    QObject::connect( _ipcServer, SIGNAL(newConnection()), this, SLOT(onNewConnectionPending()) );
    QString tmpFileName;
#if defined(Q_OS_WIN)
    tmpFileName += QString::fromUtf8("//./pipe");
    tmpFileName += QLatin1Char('/');
    tmpFileName += QString::fromUtf8(NATRON_APPLICATION_NAME);
    tmpFileName += QString::fromUtf8("_INPUT_SOCKET");
#endif

    {
#if defined(Q_OS_UNIX)
        QTemporaryFile tmpf(tmpFileName);
        tmpf.open();
        tmpFileName = tmpf.fileName();
        tmpf.remove();
#else
        QTemporaryFile tmpf;
        tmpf.open();
        QString tmpFilePath = tmpf.fileName();
        QString baseName;
        int lastSlash = tmpFilePath.lastIndexOf( QLatin1Char('/') );
        if ( (lastSlash != -1) && (lastSlash < tmpFilePath.size() - 1) ) {
            baseName = tmpFilePath.mid(lastSlash + 1);
        } else {
            baseName = tmpFilePath;
        }
        tmpFileName += baseName;
        tmpf.remove();
#endif
    }
    _ipcServer->listen(tmpFileName);


    _processArgs << QString::fromUtf8("-b") << QString::fromUtf8("-w") << QString::fromUtf8( writer->getScriptName_mt_safe().c_str() );
    _processArgs << QString::fromUtf8("--IPCpipe") <<  tmpFileName;
    _processArgs << projectPath;

    ///connect the useful slots of the process
    QObject::connect( _process, SIGNAL(readyReadStandardOutput()), this, SLOT(onStandardOutputBytesWritten()) );
    QObject::connect( _process, SIGNAL(readyReadStandardError()), this, SLOT(onStandardErrorBytesWritten()) );
    QObject::connect( _process, SIGNAL(error(QProcess::ProcessError)), this, SLOT(onProcessError(QProcess::ProcessError)) );
    QObject::connect( _process, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(onProcessEnd(int,QProcess::ExitStatus)) );


    ///start the process
    _processLog.push_back( tr("Starting background rendering: %1 %2")
                           .arg( QCoreApplication::applicationFilePath() )
                           .arg( _processArgs.join( QString::fromUtf8(" ") ) ) );
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
    _process->start(QCoreApplication::applicationFilePath(), _processArgs);
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

    QObject::connect( _bgProcessOutputSocket, SIGNAL(readyRead()), this, SLOT(onDataWrittenToSocket()) );
}

void
ProcessHandler::onDataWrittenToSocket()
{
    ///always running in the main thread
    assert( QThread::currentThread() == qApp->thread() );

    QString str = QString::fromUtf8( _bgProcessOutputSocket->readLine() );
    while ( str.endsWith( QLatin1Char('\n') ) ) {
        str.chop(1);
    }
    _processLog.append( QString::fromUtf8("Message received: ") + str + QLatin1Char('\n') );
    if ( str.startsWith( QString::fromUtf8(kFrameRenderedStringShort) ) ) {
        str = str.remove( QString::fromUtf8(kFrameRenderedStringShort) );

        double progressPercent = 0.;
        int foundProgress = str.lastIndexOf( QString::fromUtf8(kProgressChangedStringShort) );
        if (foundProgress != -1) {
            QString progressStr = str.mid(foundProgress);
            progressStr.remove( QString::fromUtf8(kProgressChangedStringShort) );
            progressPercent = progressStr.toDouble();
            str = str.mid(0, foundProgress);
        }
        if ( !str.isEmpty() ) {
            //The report does not have extended timer infos
            Q_EMIT frameRendered(str.toInt(), progressPercent);
        }
    } else if ( str.startsWith( QString::fromUtf8(kRenderingFinishedStringShort) ) ) {
        ///don't do anything
    } else if ( str.startsWith( QString::fromUtf8(kBgProcessServerCreatedShort) ) ) {
        str = str.remove( QString::fromUtf8(kBgProcessServerCreatedShort) );
        ///the bg process wants us to create the pipe for its input
        if (!_bgProcessInputSocket) {
            _bgProcessInputSocket = new QLocalSocket();
            QObject::connect( _bgProcessInputSocket, SIGNAL(connected()), this, SLOT(onInputPipeConnectionMade()) );
            _bgProcessInputSocket->connectToServer(str, QLocalSocket::ReadWrite);
        }
    } else if ( str.startsWith( QString::fromUtf8(kRenderingStartedShort) ) ) {
        ///if the user pressed cancel prior to the pipe being created, wait for it to be created and send the abort
        ///message right away
        if (_earlyCancel) {
            _bgProcessInputSocket->waitForConnected(5000);
            _earlyCancel = false;
            onProcessCanceled();
        }
    } else {
        _processLog.append( QString::fromUtf8("Error: Unable to interpret message.\n") );
        throw std::runtime_error("ProcessHandler::onDataWrittenToSocket() received erroneous message");
    }
}

void
ProcessHandler::onInputPipeConnectionMade()
{
    ///always running in the main thread
    assert( QThread::currentThread() == qApp->thread() );

    _processLog.append( QString::fromUtf8("The input channel (the one the bg process listens to) was successfully created and connected.\n") );
}

void
ProcessHandler::onStandardOutputBytesWritten()
{
    QString str = QString::fromUtf8( _process->readAllStandardOutput().data() );

#ifdef DEBUG
    qDebug() << "Message(stdout):" << str;
#endif
    _processLog.append(QString::fromUtf8("Message(stdout): ") + str);
}

void
ProcessHandler::onStandardErrorBytesWritten()
{
    QString str = QString::fromUtf8( _process->readAllStandardError().data() );

#ifdef DEBUG
    qDebug() << "Message(stderr):" << str;
#endif
    _processLog.append(QString::fromUtf8("Error(stderr): ") + str);
}

void
ProcessHandler::onProcessCanceled()
{
    Q_EMIT processCanceled();

    if (!_bgProcessInputSocket) {
        _earlyCancel = true;
    } else {
        _bgProcessInputSocket->write( ( QString::fromUtf8(kAbortRenderingStringShort) + QLatin1Char('\n') ).toUtf8() );
        _bgProcessInputSocket->flush();
    }
}

void
ProcessHandler::onProcessError(QProcess::ProcessError err)
{
    if (err == QProcess::FailedToStart) {
        Dialogs::errorDialog( _writer->getScriptName(), tr("The render process failed to start.").toStdString() );
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
    , _backgroundOutputPipeMutex()
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
        assert(!_mustQuit);
        _mustQuit = true;
        while (_mustQuit) {
            _mustQuitCond.wait(&_mustQuitMutex);
        }
    }

    delete _backgroundIPCServer;
    delete _backgroundOutputPipe;
}

void
ProcessInputChannel::writeToOutputChannel(const QString & message)
{
    {
        QMutexLocker l(&_backgroundOutputPipeMutex);
        _backgroundOutputPipe->write( ( message + QLatin1Char('\n') ).toUtf8() );
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
    QObject::connect( _backgroundInputPipe, SIGNAL(readyRead()), this, SLOT(onInputChannelMessageReceived()) );
}

bool
ProcessInputChannel::onInputChannelMessageReceived()
{
    QString str = QString::fromUtf8( _backgroundInputPipe->readLine() );

    while ( str.endsWith( QChar::fromLatin1('\n') ) ) {
        str.chop(1);
    }
    if ( str.startsWith( QString::fromUtf8(kAbortRenderingStringShort) ) ) {
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
#ifdef DEBUG
    boost_adaptbx::floating_point::exception_trapping trap(boost_adaptbx::floating_point::exception_trapping::division_by_zero |
                                                           boost_adaptbx::floating_point::exception_trapping::invalid |
                                                           boost_adaptbx::floating_point::exception_trapping::overflow);
#endif
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
    QObject::connect( _backgroundOutputPipe, SIGNAL(connected()), this, SLOT(onOutputPipeConnectionMade()) );
    _backgroundOutputPipe->connectToServer(_mainProcessServerName, QLocalSocket::ReadWrite);
    std::cout << "Attempting connection to " << _mainProcessServerName.toStdString() << std::endl;

    _backgroundIPCServer = new QLocalServer();
    QObject::connect( _backgroundIPCServer, SIGNAL(newConnection()), this, SLOT(onNewConnectionPending()) );
    QString tmpFileName;
#if defined(Q_OS_WIN)
    tmpFileName += QString::fromUtf8("//./pipe");
    tmpFileName += QLatin1Char('/');
    tmpFileName += QString::fromUtf8(NATRON_APPLICATION_NAME);
    tmpFileName += QString::fromUtf8("_INPUT_SOCKET");
#endif

    {
#if defined(Q_OS_UNIX)
        QTemporaryFile tmpf(tmpFileName);
        tmpf.open();
        tmpFileName = tmpf.fileName();
        tmpf.remove();
#else
        QTemporaryFile tmpf;
        tmpf.open();
        QString tmpFilePath = tmpf.fileName();
        QString baseName;
        int lastSlash = tmpFilePath.lastIndexOf( QLatin1Char('/') );
        if ( (lastSlash != -1) && (lastSlash < tmpFilePath.size() - 1) ) {
            baseName = tmpFilePath.mid(lastSlash + 1);
        } else {
            baseName = tmpFilePath;
        }
        tmpFileName += baseName;
        tmpf.remove();
#endif
    }
    _backgroundIPCServer->listen(tmpFileName);

    if ( !_backgroundOutputPipe->waitForConnected(5000) ) { //< blocking, we wait for the server to respond
        std::cout << "WARNING: The GUI application failed to respond, canceling this process will not be possible"
            " unless it finishes or you kill it." << std::endl;
    }
    writeToOutputChannel( QString::fromUtf8(kBgProcessServerCreatedShort) + tmpFileName );

    ///we wait for the GUI app to connect its socket to this server, we let it 5 sec to reply
    _backgroundIPCServer->waitForNewConnection(5000);

    ///since we're still not returning the event loop, just process them manually in case
    ///Qt didn't caught the new connection pending.
    {
#ifdef DEBUG
        boost_adaptbx::floating_point::exception_trapping trap(0);
#endif
        QCoreApplication::processEvents();
    }
} // ProcessInputChannel::initialize

void
ProcessInputChannel::onOutputPipeConnectionMade()
{
    qDebug() << "The output channel was successfully created and connected.";
}

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_ProcessHandler.cpp"
