//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */

#include "ProcessHandler.h"

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
#include "Engine/EffectInstance.h"

ProcessHandler::ProcessHandler(AppInstance* app,
                               const QString& projectPath,
                               const QString& outputFileSequence,
                               int firstFrame,int lastFrame,
                               Natron::OutputEffectInstance* writer)
    : _app(app)
    ,_process(new QProcess)
    ,_writer(writer)
    ,_ipcServer(0)
    ,_bgProcessOutputSocket(0)
    ,_bgProcessInputSocket(0)
    ,_earlyCancel(false)
{

    ///setup the server used to listen the output of the background process
    _ipcServer = new QLocalServer();
    QObject::connect(_ipcServer,SIGNAL(newConnection()),this,SLOT(onNewConnectionPending()));
    QString serverName;
    {
        QTemporaryFile tmpf(NATRON_APPLICATION_NAME "_OUTPUT_PIPE_" + QString::number(_process->pid()));
        tmpf.open();
        serverName = tmpf.fileName();
    }
    _ipcServer->listen(serverName);
    
    
    
    QStringList processArgs;
    processArgs << projectPath << "-b" << "-w" << writer->getName().c_str();
    processArgs << "--IPCpipe" << (_ipcServer->fullServerName());
    
    ///connect the useful slots of the process
    QObject::connect(_process,SIGNAL(readyReadStandardOutput()),this,SLOT(onStandardOutputBytesWritten()));
    QObject::connect(_process,SIGNAL(readyReadStandardError()),this,SLOT(onStandardErrorBytesWritten()));
    QObject::connect(_process,SIGNAL(error(QProcess::ProcessError)),this,SLOT(onProcessError(QProcess::ProcessError)));
    QObject::connect(_process,SIGNAL(finished(int,QProcess::ExitStatus)),this,SLOT(onProcessEnd(int,QProcess::ExitStatus)));

    
    ///start the process
    qDebug() << "Starting background rendering: " << QCoreApplication::applicationFilePath() << processArgs;
    _process->start(QCoreApplication::applicationFilePath(),processArgs);

    app->notifyRenderProcessHandlerStarted(outputFileSequence,firstFrame,lastFrame,this);

}


ProcessHandler::~ProcessHandler(){
    _process->close();
    delete _process;
    delete _ipcServer;
    delete _bgProcessInputSocket;
}

void ProcessHandler::onNewConnectionPending() {
    ///accept only 1 connection!
    if (_bgProcessOutputSocket) {
        return;
    }

    _bgProcessOutputSocket = _ipcServer->nextPendingConnection();

    QObject::connect(_bgProcessOutputSocket, SIGNAL(readyRead()), this, SLOT(onDataWrittenToSocket()));
}


void ProcessHandler::onDataWrittenToSocket() {
    QString str = _bgProcessOutputSocket->readLine();
    while(str.endsWith('\n')) {
        str.chop(1);
    }
    qDebug() << "ProcessHandler::onDataWrittenToSocket() received " << str;
    if (str.startsWith(kFrameRenderedStringShort)) {
        str = str.remove(kFrameRenderedStringShort);
        emit frameRendered(str.toInt());
    } else if (str.startsWith(kRenderingFinishedStringShort)) {
        ///don't do anything
    } else if (str.startsWith(kProgressChangedStringShort)) {
        str = str.remove(kProgressChangedStringShort);
        emit frameProgress(str.toInt());
        
    } else if (str.startsWith(kBgProcessServerCreatedShort)) {
        str = str.remove(kBgProcessServerCreatedShort);
        ///the bg process wants us to create the pipe for its input
        if (!_bgProcessInputSocket) {
            _bgProcessInputSocket = new QLocalSocket();
            QObject::connect(_bgProcessInputSocket, SIGNAL(connected()), this, SLOT(onInputPipeConnectionMade()));
            _bgProcessInputSocket->connectToServer(str,QLocalSocket::ReadWrite);
        }
    } else if(str.startsWith(kRenderingStartedShort)) {
        ///if the user pressed cancel prior to the pipe being created, wait for it to be created and send the abort
        ///message right away
        if (_earlyCancel) {
            _bgProcessInputSocket->waitForConnected(5000);
            _earlyCancel = false;
            onProcessCanceled();
        }
    } else {
        qDebug() << "Error: Unable to interpret message: " << str;
        throw std::runtime_error("ProcessHandler::onDataWrittenToSocket() received erroneous message");
    }

}

void ProcessHandler::onInputPipeConnectionMade() {
    qDebug() << "The input channel was successfully created and connected.";
}

void ProcessHandler::onStandardOutputBytesWritten(){

    QString str(_process->readAllStandardOutput().data());
    qDebug() << str ;
}

void ProcessHandler::onStandardErrorBytesWritten() {
    QString str(_process->readAllStandardError().data());
    qDebug() << str ;
}

void ProcessHandler::onProcessCanceled(){
    emit processCanceled();
    if(!_bgProcessInputSocket) {
        _earlyCancel = true;
    } else {
        _bgProcessInputSocket->write((QString(kAbortRenderingStringShort) + '\n').toUtf8());
        _bgProcessInputSocket->flush();
    }
}

void ProcessHandler::onProcessError(QProcess::ProcessError err){
    if(err == QProcess::FailedToStart){
        Natron::errorDialog(_writer->getName(),"The render process failed to start");
    }else if(err == QProcess::Crashed){
        //@TODO: find out a way to get the backtrace
    }
}

void ProcessHandler::onProcessEnd(int exitCode,QProcess::ExitStatus stat) {
    int returnCode = 0;
    if (stat == QProcess::CrashExit) {
        returnCode = 2;
    } else if(exitCode == 1) {
        returnCode = 1;
        
    }
    emit processFinished(returnCode);

}


ProcessInputChannel::ProcessInputChannel(const QString& mainProcessServerName)
: QThread()
, _mainProcessServerName(mainProcessServerName)
, _backgroundOutputPipeMutex(new QMutex)
, _backgroundOutputPipe(0)
, _backgroundIPCServer(0)
, _backgroundInputPipe(0)
, _mustQuit(false)
, _mustQuitCond(new QWaitCondition)
, _mustQuitMutex(new QMutex)
{
    initialize();
    _backgroundIPCServer->moveToThread(this);
    start();
    
}

ProcessInputChannel::~ProcessInputChannel() {
    
    if (isRunning()) {
        _mustQuit = true;
        while (_mustQuit) {
            _mustQuitCond->wait(_mustQuitMutex);
        }
    }
    
    delete _backgroundIPCServer;
    delete _backgroundOutputPipeMutex;
    delete _backgroundOutputPipe;
    delete _mustQuitCond;
    delete _mustQuitMutex;
    
}

void ProcessInputChannel::writeToOutputChannel(const QString& message){
    {
        QMutexLocker l(_backgroundOutputPipeMutex);
        _backgroundOutputPipe->write((message+'\n').toUtf8());
        _backgroundOutputPipe->flush();
    }
}

void ProcessInputChannel::onNewConnectionPending() {
    if (_backgroundInputPipe) {
        ///listen to only 1 process!
        return;
    }
    _backgroundInputPipe = _backgroundIPCServer->nextPendingConnection();
    QObject::connect(_backgroundInputPipe, SIGNAL(readyRead()), this, SLOT(onInputChannelMessageReceived()));
}

bool ProcessInputChannel::onInputChannelMessageReceived() {
    QString str(_backgroundInputPipe->readLine());
    while(str.endsWith('\n')) {
        str.chop(1);
    }
    if (str.startsWith(kAbortRenderingStringShort)) {
        qDebug() << "Aborting render!";
        appPTR->abortAnyProcessing();
        return true;
    } else {
        qDebug() << "Error: Unable to interpret message: " << str;
        throw std::runtime_error("ProcessInputChannel::onInputChannelMessageReceived() received erroneous message");
    }

    return false;
}

void ProcessInputChannel::run() {
    
    for(;;) {

        if (_backgroundInputPipe->waitForReadyRead(100)) {
            if(onInputChannelMessageReceived()) {
                qDebug() << "Background process now closing the input channel...";
                return;
            }
        }
        
        QMutexLocker l(_mustQuitMutex);
        if (_mustQuit) {
            _mustQuit = false;
            _mustQuitCond->wakeOne();
            return;
        }
    }
}

void ProcessInputChannel::initialize() {
    _backgroundOutputPipe = new QLocalSocket();
    QObject::connect(_backgroundOutputPipe, SIGNAL(connected()), this, SLOT(onOutputPipeConnectionMade()));
    _backgroundOutputPipe->connectToServer(_mainProcessServerName,QLocalSocket::ReadWrite);
    
    _backgroundIPCServer = new QLocalServer();
    QObject::connect(_backgroundIPCServer,SIGNAL(newConnection()),this,SLOT(onNewConnectionPending()));

    QString serverName;
    {
        QTemporaryFile tmpf(QDir::tempPath() + QDir::separator() + NATRON_APPLICATION_NAME "_INPUT_SOCKET"
                            + QString::number(QCoreApplication::applicationPid()));
        tmpf.open();
        serverName = tmpf.fileName();
    }
    _backgroundIPCServer->listen(serverName);
    
    if(!_backgroundOutputPipe->waitForConnected(5000)){ //< blocking, we wait for the server to respond
        std::cout << "WARNING: The GUI application failed to respond, canceling this process will not be possible"
        " unless it finishes or you kill it." << std::endl;
    }
    writeToOutputChannel(QString(kBgProcessServerCreatedShort) + _backgroundIPCServer->fullServerName());
    
    ///we wait for the GUI app to connect its socket to this server, we let it 5 sec to reply
    _backgroundIPCServer->waitForNewConnection(5000);
    
    ///since we're still not returning the event loop, just process them manually in case
    ///Qt didn't caught the new connection pending.
    QCoreApplication::processEvents();

}

void ProcessInputChannel::onOutputPipeConnectionMade() {
    qDebug() << "The output channel was successfully created and connected.";
}
