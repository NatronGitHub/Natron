//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#include "CrashDialog.h"
#include <cassert>
#include <QThread>
#include <QTextStream>
#include <QFile>
#include <QApplication>
#include <QLocalSocket>

CallbacksManager* CallbacksManager::_instance = 0;


CrashDialog::CrashDialog(const QString &filePath)
    : QDialog(0,Qt::Dialog | Qt::WindowStaysOnTopHint)
{
    QFile qss(":/Resources/Stylesheets/mainstyle.qss");

        if ( qss.open(QIODevice::ReadOnly
                      | QIODevice::Text) ) {
            QTextStream in(&qss);
            QString content( in.readAll() );
            setStyleSheet( content
                           .arg("rgb(243,149,0)") // %1: selection-color
                           .arg("rgb(50,50,50)") // %2: medium background
                           .arg("rgb(71,71,71)") // %3: soft background
                           .arg("rgb(38,38,38)") // %4: strong background
                           .arg("rgb(200,200,200)") // %5: text colour
                           .arg("rgb(86,117,156)") // %6: interpolated value color
                           .arg("rgb(21,97,248)") // %7: keyframe value color
                           .arg("rgb(0,0,0)")  // %8: disabled editable text
                           .arg("rgb(180, 200, 100)") ); // %9: expression background color
        }

}

CrashDialog::~CrashDialog()
{

}

CallbacksManager::CallbacksManager()
    : QObject()
    , _doingDialogMutex()
    , _doingDialogCond()
    , _doingDialog(false)
#ifdef DEBUG
    , _dFileMutex()
    , _dFile(0)
#endif
    , _outputPipe(0)
{
    _instance = this;
    QObject::connect(this, SIGNAL(doDumpCallBackOnMainThread(QString)), this, SLOT(onDoDumpOnMainThread(QString)));
    
#ifdef DEBUG
    _dFile = new QFile("debug.txt");
    _dFile->open(QIODevice::ReadWrite | QIODevice::Truncate | QIODevice::Text);
#endif
}


CallbacksManager::~CallbacksManager() {
#ifdef DEBUG
    delete _dFile;
#endif
    
    delete _outputPipe;
    _instance = 0;
    
}


void
CallbacksManager::onDoDumpOnMainThread(const QString& filePath)
{
    assert(QThread::currentThread() == qApp->thread());
    CrashDialog d(filePath);
    if (d.exec()) {

    } else {

    }
    
    QMutexLocker k(&_doingDialogMutex);
    _doingDialog = false;
    _doingDialogCond.wakeAll();
}

void
CallbacksManager::s_emitDoCallBackOnMainThread(const QString& filePath)
{
    writeDebugMessage("Dump request received, file located at: " + filePath);
    
    {
        QMutexLocker k(&_doingDialogMutex);
        _doingDialog = true;
        
        emit doDumpCallBackOnMainThread(filePath);
        
        while (_doingDialog) {
            _doingDialogCond.wait(&_doingDialogMutex);
        }
    }
    qApp->quit();
}

#ifdef DEBUG
void
CallbacksManager::writeDebugMessage(const QString& str)
{
    QMutexLocker k(&_dFileMutex);
    QTextStream ts(_dFile);
    ts << str << '\n';
}
#endif

void
CallbacksManager::onOutputPipeConnectionMade()
{
    writeDebugMessage("Output IPC pipe with Natron successfully connected.");
    
    //At this point we're sure that the server is created, so we notify Natron about it so it can create its ExceptionHandler
    writeToOutputPipe("-i");
}

void
CallbacksManager::writeToOutputPipe(const QString& str)
{
    assert(_outputPipe);
    if (!_outputPipe) {
        return;
    }
    _outputPipe->write( (str + '\n').toUtf8() );
    _outputPipe->flush();
}

void
CallbacksManager::initOuptutPipe(const QString& comPipeName)
{
    assert(!_outputPipe);
    _outputPipe = new QLocalSocket;
    QObject::connect( _outputPipe, SIGNAL( connected() ), this, SLOT( onOutputPipeConnectionMade() ) );
    _outputPipe->connectToServer(comPipeName,QLocalSocket::ReadWrite);
}
