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

#include "CallbacksManager.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>

#define UPLOAD_URL "http://breakpad.natron.fr/submit"

CallbacksManager* CallbacksManager::_instance = 0;


#include <QLocalSocket>

#ifdef DEBUG
#include <QTextStream>
#include <QFile>
#endif


#ifndef REPORTER_CLI_ONLY
#include "CrashDialog.h"
#endif

CallbacksManager::CallbacksManager(bool autoUpload)
    : QObject()
#ifdef DEBUG
    , _dFileMutex()
    , _dFile(0)
#endif
    , _outputPipe(0)
    , _autoUpload(autoUpload)
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
CallbacksManager::uploadFileToRepository(const QString& str)
{
    QNetworkAccessManager *networkMnger = new QNetworkAccessManager(this);
    QObject::connect(networkMnger, SIGNAL(finished(QNetworkReply*)),
            this, SLOT(replyFinished(QNetworkReply*)));

    QNetworkRequest request(QUrl(UPLOAD_URL));

    ///To complete...
}

void
CallbacksManager::replyFinished(QNetworkReply* reply) {
    if (!reply) {
        return;
    }

    ///What to do when the server replied

    reply->deleteLater();
}

void
CallbacksManager::onDoDumpOnMainThread(const QString& filePath)
{

    assert(QThread::currentThread() == qApp->thread());

    bool doUpload = false;
#ifdef REPORTER_CLI_ONLY
    doUpload = _autoUpload;
    ///@todo We must notify the user the log is available at filePath but we don't have access to the terminal with this process
#endif


#ifndef REPORTER_CLI_ONLY
    CrashDialog d(filePath);
    CrashDialog::UserChoice ret = CrashDialog::eUserChoiceIgnore;
    if (d.exec()) {
        ret = d.getUserChoice();
    }
    switch (ret) {
    case CrashDialog::eUserChoiceUpload:
        doUpload = true;
        break;
    case CrashDialog::eUserChoiceSave: // already handled in the dialog
    case CrashDialog::eUserChoiceIgnore:
    }
#endif

    if (doUpload) {
        uploadFileToRepository(filePath);
    }

}

void
CallbacksManager::s_emitDoCallBackOnMainThread(const QString& filePath)
{
    writeDebugMessage("Dump request received, file located at: " + filePath);
    if (QFile::exists(filePath)) {

        emit doDumpCallBackOnMainThread(filePath);

    } else {
        writeDebugMessage("Dump file does not seem to exist...exiting crash reporter now.");
    }
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
