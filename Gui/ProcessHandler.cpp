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

#include <QDateTime>
#include <QProcess>
#include <QLocalServer>
#include <QLocalSocket>
#include <QCoreApplication>
#include <QTemporaryFile>

#include "Global/AppManager.h"
#include "Engine/KnobFile.h"
#include "Engine/EffectInstance.h"
#include "Gui/Gui.h"

ProcessHandler::ProcessHandler(AppInstance* app,
                               const QString& projectPath,
                               Natron::OutputEffectInstance* writer)
    : _app(app)
    ,_process(new QProcess)
    ,_writer(writer)
    ,_dialog(NULL)
    ,_ipcServer(0)
    ,_bgProcessOutputSocket(0)
    ,_bgProcessInputSocket(0)
    ,_earlyCancel(false)
{

    ///setup the server used to listen the output of the background process
    QDateTime now = QDateTime::currentDateTime();
    _ipcServer = new QLocalServer();
    QObject::connect(_ipcServer,SIGNAL(newConnection()),this,SLOT(onNewConnectionPending()));
    QString serverName(NATRON_APPLICATION_NAME "_OUTPUT_PIPE_" + now.toString());
    _ipcServer->listen(serverName);


    ///the process args
    QStringList appArgs = QCoreApplication::arguments();
    QStringList processArgs;
    processArgs << projectPath  << "-b" << "--writer" << writer->getName().c_str();
    processArgs << "--IPCpipe" << (_ipcServer->fullServerName());

    ///connect the useful slots of the process
    QObject::connect(_process,SIGNAL(readyReadStandardOutput()),this,SLOT(onStandardOutputBytesWritten()));
    QObject::connect(_process,SIGNAL(readyReadStandardError()),this,SLOT(onStandardErrorBytesWritten()));
    QObject::connect(_process,SIGNAL(error(QProcess::ProcessError)),this,SLOT(onProcessError(QProcess::ProcessError)));
    QObject::connect(_process,SIGNAL(finished(int,QProcess::ExitStatus)),this,SLOT(onProcessEnd(int,QProcess::ExitStatus)));

    ///validate the frame range to render
    int firstFrame,lastFrame;
    writer->getFrameRange(&firstFrame, &lastFrame);
    if(firstFrame > lastFrame)
        throw std::invalid_argument("First frame in the sequence is greater than the last frame");

    ///start the process
    _process->start(appArgs.at(0),processArgs);

    ///get the output file knob to get the same of the sequence
    std::string outputFileSequence;
    const std::vector< boost::shared_ptr<Knob> >& knobs = writer->getKnobs();
    for (U32 i = 0; i < knobs.size(); ++i) {
        if (knobs[i]->typeName() == OutputFile_Knob::typeNameStatic()) {
            boost::shared_ptr<OutputFile_Knob> fk = boost::dynamic_pointer_cast<OutputFile_Knob>(knobs[i]);
            if(fk->isOutputImageFile()){
                outputFileSequence = fk->getValue().toString().toStdString();
            }
        }
    }
    assert(app->getGui());

    ///make the dialog which will show the progress
    _dialog = new RenderingProgressDialog(outputFileSequence.c_str(),firstFrame,lastFrame,app->getGui());
    QObject::connect(_dialog,SIGNAL(canceled()),this,SLOT(onProcessCanceled()));
    _dialog->show();

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
        _dialog->onFrameRendered(str.toInt());
    } else if (str.startsWith(kRenderingFinishedStringShort)) {
        if(_process->state() == QProcess::Running) {
            _process->waitForFinished();
        }
    } else if (str.startsWith(kProgressChangedStringShort)) {
        str = str.remove(kProgressChangedStringShort);
        _dialog->onCurrentFrameProgress(str.toInt());
        
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
    _dialog->hide();
    if(!_bgProcessInputSocket) {
        _earlyCancel = true;
    } else {
        _bgProcessInputSocket->write((QString(kAbortRenderingStringShort) + '\n').toUtf8());
        _bgProcessInputSocket->flush();
        if(_process->state() == QProcess::Running) {
            _process->waitForFinished();
        }
    }
}

void ProcessHandler::onProcessError(QProcess::ProcessError err){
    if(err == QProcess::FailedToStart){
        Natron::errorDialog(_writer->getName(),"The render process failed to start");
    }else if(err == QProcess::Crashed){
        //@TODO: find out a way to get the backtrace
    }
}

void ProcessHandler::onProcessEnd(int exitCode,QProcess::ExitStatus stat){
    if(stat == QProcess::CrashExit){
        Natron::errorDialog(_writer->getName(),"The render process exited after a crash");
        // _hasProcessBeenDeleted = true;

    }else if(exitCode == 1){
        Natron::errorDialog(_writer->getName(), "The process ended with a return code of 1, this indicates an undetermined problem occured.");
    }else{
        Natron::informationDialog(_writer->getName(),"Render finished!");
    }
    _dialog->hide();
    delete this;
}


ProcessInputChannel::ProcessInputChannel(const QString& mainProcessServerName)
: QThread()
, _mainProcessServerName(mainProcessServerName)
, _backgroundOutputPipe(0)
, _backgroundIPCServer(0)
, _backgroundInputPipe(0)
{
    initialize();
    _backgroundIPCServer->moveToThread(this);
    start();
    
}

ProcessInputChannel::~ProcessInputChannel() {
    delete _backgroundIPCServer;
    delete _backgroundOutputPipe;
}

void ProcessInputChannel::writeToOutputChannel(const QString& message){
    _backgroundOutputPipe->write((message+'\n').toUtf8());
    _backgroundOutputPipe->flush();
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
    }
}

void ProcessInputChannel::initialize() {
    _backgroundOutputPipe = new QLocalSocket();
    QObject::connect(_backgroundOutputPipe, SIGNAL(connected()), this, SLOT(onOutputPipeConnectionMade()));
    _backgroundOutputPipe->connectToServer(_mainProcessServerName,QLocalSocket::ReadWrite);
    
    QDateTime now = QDateTime::currentDateTime();
    _backgroundIPCServer = new QLocalServer();
    QObject::connect(_backgroundIPCServer,SIGNAL(newConnection()),this,SLOT(onNewConnectionPending()));

    QString serverName;
    {
        QTemporaryFile tmpf(QDir::tempPath() + QDir::separator() + NATRON_APPLICATION_NAME "_INPUT_SOCKET");
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
