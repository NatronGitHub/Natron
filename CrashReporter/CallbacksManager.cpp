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
#include <QHttpMultiPart>

#define UPLOAD_URL "http://breakpad.natron.fr/submit"

CallbacksManager* CallbacksManager::_instance = 0;

#include <cassert>
#include <QLocalSocket>

#include <QThread>
#ifndef REPORTER_CLI_ONLY
#include <QApplication>
#include <QProgressDialog>
#else
#include <QCoreApplication>
#endif
#ifdef DEBUG
#include <QTextStream>
#endif

#include <QFile>

#ifndef REPORTER_CLI_ONLY
#include "CrashDialog.h"
#endif

#include "../Global/Macros.h"

CallbacksManager::CallbacksManager(bool autoUpload)
: QObject()
#ifdef DEBUG
, _dFileMutex()
, _dFile(0)
#endif
, _outputPipe(0)
, _uploadReply(0)
, _autoUpload(autoUpload)
#ifndef REPORTER_CLI_ONLY
, _dialog(0)
, _progressDialog(0)
#endif
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

static void addTextHttpPart(QHttpMultiPart* multiPart, const QString& string)
{
    QHttpPart part;
    part.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("text/plain"));
    part.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"text\""));
    part.setBody(string.toLatin1());
    multiPart->append(part);
}

static void addFileHttpPart(QHttpMultiPart* multiPart, const QString& filePath)
{
    QFile *file = new QFile(filePath);
    file->setParent(multiPart);
    if (!file->open(QIODevice::ReadOnly)) {
        CallbacksManager::instance()->writeDebugMessage("Failed to open the following file for uploading: " + filePath);
        return;
    }
    
    QHttpPart part;
    part.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("binary/dmp"));
    part.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"dmp\"; filename=\"" +  file->fileName() + "\""));
    part.setBodyDevice(file);
    
    multiPart->append(part);
}

void
CallbacksManager::uploadFileToRepository(const QString& filepath, const QString& comments)
{
    assert(!_uploadReply);
    
    const QString productName(NATRON_APPLICATION_NAME);
    QString versionStr(NATRON_VERSION_STRING " " NATRON_DEVELOPMENT_STATUS);
    if (NATRON_DEVELOPMENT_STATUS == NATRON_DEVELOPMENT_RELEASE_CANDIDATE) {
        versionStr += ' ';
        versionStr += QString::number(NATRON_BUILD_NUMBER);
    }
    
    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    addTextHttpPart(multiPart, productName);
    addTextHttpPart(multiPart, versionStr);
    addTextHttpPart(multiPart, comments);
    addFileHttpPart(multiPart, filepath);
    
    QNetworkRequest request(QUrl(UPLOAD_URL));

    writeDebugMessage("Attempt to upload crash report...");
#ifndef REPORTER_CLI_ONLY
    assert(_dialog);
    _progressDialog = new QProgressDialog(_dialog);
    _progressDialog->setRange(0, 100);
    _progressDialog->setMinimumDuration(100);
    _progressDialog->setLabelText(tr("Uploading crash report..."));
    QObject::connect(_progressDialog, SIGNAL(canceled()), this, SLOT(onProgressDialogCanceled()));
#endif
    
    
    
    QNetworkAccessManager *networkMnger = new QNetworkAccessManager(this);
    QObject::connect(networkMnger, SIGNAL(finished(QNetworkReply*)),
                     this, SLOT(replyFinished()));
    _uploadReply = networkMnger->post(request, multiPart);
    QObject::connect(_uploadReply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(replyError(QNetworkReply::NetworkError)));
    QObject::connect(_uploadReply, SIGNAL(finished()), this, SLOT(replyFinished()));
    QObject::connect(_uploadReply, SIGNAL(uploadProgress(qint64,qint64)), this, SLOT(onUploadProgress(qint64,qint64)));
    multiPart->setParent(_uploadReply);
    
    
}

void
CallbacksManager::onProgressDialogCanceled()
{
    if (_uploadReply) {
        _uploadReply->abort();
    }
}

void
CallbacksManager::onUploadProgress(qint64 bytesSent, qint64 bytesTotal)
{
    assert(_progressDialog);
    double percent = (double)bytesSent / bytesTotal;
    _progressDialog->setValue(percent * 100);
}

void
CallbacksManager::replyFinished() {
    if (!_uploadReply) {
        return;
    }
    
    writeDebugMessage("File uploaded successfully! ");

    _uploadReply->deleteLater();
    _uploadReply = 0;
#ifndef REPORTER_CLI_ONLY
    if (_dialog) {
        _dialog->deleteLater();
    }
#endif
    qApp->exit();
}

void
CallbacksManager::replyError(QNetworkReply::NetworkError errCode)
{
    if (!_uploadReply) {
        return;
    }
    
#ifdef DEBUG
    writeDebugMessage("Network error code " + QString::number((int)errCode));
#endif
    
    
    _uploadReply->deleteLater();
    _uploadReply = 0;
#ifndef REPORTER_CLI_ONLY
    if (_dialog) {
        _dialog->deleteLater();
    }
#endif
    qApp->exit();
}

void
CallbacksManager::onDoDumpOnMainThread(const QString& filePath)
{

    assert(QThread::currentThread() == qApp->thread());

#ifdef REPORTER_CLI_ONLY
    if (_autoUpload) {
        uploadFileToRepository(filePath,"");
    }
    ///@todo We must notify the user the log is available at filePath but we don't have access to the terminal with this process
#else
    assert(!_dialog);
    _dialog = new CrashDialog(filePath);
    QObject::connect(_dialog ,SIGNAL(rejected()), this, SLOT(onCrashDialogFinished()));
    QObject::connect(_dialog ,SIGNAL(accepted()), this, SLOT(onCrashDialogFinished()));
    _dialog->raise();
    _dialog->show();
    
#endif
}

void
CallbacksManager::onCrashDialogFinished()
{
#ifndef REPORTER_CLI_ONLY
    assert(_dialog == qobject_cast<CrashDialog*>(sender()));
    if (!_dialog) {
        return;
    }
    
    bool doUpload = false;
    CrashDialog::UserChoice ret = CrashDialog::eUserChoiceIgnore;
    QDialog::DialogCode code =  (QDialog::DialogCode)_dialog->result();
    if (code == QDialog::Accepted) {
        ret = _dialog->getUserChoice();
        
    }
    
    switch (ret) {
        case CrashDialog::eUserChoiceUpload:
            doUpload = true;
            break;
        case CrashDialog::eUserChoiceSave: // already handled in the dialog
        case CrashDialog::eUserChoiceIgnore:
            break;
    }
    
    writeDebugMessage("Dialog finished with ret code "+ QString::number((int)ret));

    if (doUpload) {
        uploadFileToRepository(_dialog->getOriginalDumpFilePath(),_dialog->getDescription());
    } else {
        _dialog->deleteLater();
    }

#endif

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
