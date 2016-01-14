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

#include "CallbacksManager.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QHttpMultiPart>

#define UPLOAD_URL "http://breakpad.natron.fr/submit"
#define FALLBACK_FORM_URL "http://breakpad.natron.fr/form/"

CallbacksManager* CallbacksManager::_instance = 0;

#include <cassert>
#include <iostream>
#include <QLocalSocket>
#include <QLocalServer>

#include <QThread>
#ifndef REPORTER_CLI_ONLY
#include <QApplication>
#include <QProgressDialog>
#include <QMessageBox>
#else
#include <QCoreApplication>
#endif
#ifdef DEBUG
#include <QTextStream>
#endif

#include <QFile>
#include <QFileInfo>


#if defined(Q_OS_MAC)
#include "client/mac/crash_generation/crash_generation_server.h"
#elif defined(Q_OS_LINUX)
#include "client/linux/crash_generation/crash_generation_server.h"
#elif defined(Q_OS_WIN32)
#include "client/windows/crash_generation/crash_generation_server.h"
#endif

#ifndef REPORTER_CLI_ONLY
#include "CrashDialog.h"
#endif

#include "../Global/Macros.h"

using namespace google_breakpad;

CallbacksManager::CallbacksManager(bool autoUpload)
: QObject()
#ifdef TRACE_CRASH_RERPORTER
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
, _didError(false)
, _dumpFilePath()
, _crashServer(0)
{
    _instance = this;
    QObject::connect(this, SIGNAL(doDumpCallBackOnMainThread(QString)), this, SLOT(onDoDumpOnMainThread(QString)));

#ifdef TRACE_CRASH_RERPORTER
    _dFile = new QFile("debugCrashReporter.txt");
    _dFile->open(QIODevice::ReadWrite | QIODevice::Truncate | QIODevice::Text);
#endif
}


CallbacksManager::~CallbacksManager() {
#ifdef TRACE_CRASH_RERPORTER
    delete _dFile;
#endif

    delete _outputPipe;
    _instance = 0;
    delete _crashServer;


}

static void addTextHttpPart(QHttpMultiPart* multiPart, const QString& name, const QString& value)
{
    QHttpPart part;
    part.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("text/plain"));
    part.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"" + name + "\""));
    part.setBody(value.toLatin1());
    multiPart->append(part);
}

static void addFileHttpPart(QHttpMultiPart* multiPart, const QString& name, const QString& filePath)
{
    QFile *file = new QFile(filePath);
    file->setParent(multiPart);
    if (!file->open(QIODevice::ReadOnly)) {
        CallbacksManager::instance()->writeDebugMessage("Failed to open the following file for uploading: " + filePath);
        return;
    }
    
    QHttpPart part;
    part.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("text/dmp"));
    part.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"" + name + "\"; filename=\"" +  file->fileName() + "\""));
    part.setBodyDevice(file);
    
    multiPart->append(part);
}

static QString getVersionString()
{
    QString versionStr(NATRON_VERSION_STRING);
    versionStr.append(" ");
    versionStr.append(NATRON_DEVELOPMENT_STATUS);
    if (QString(NATRON_DEVELOPMENT_STATUS) == QString(NATRON_DEVELOPMENT_RELEASE_CANDIDATE)) {
        versionStr += ' ';
        versionStr += QString::number(NATRON_BUILD_NUMBER);
    }
    return versionStr;
}

void
CallbacksManager::uploadFileToRepository(const QString& filepath, const QString& comments)
{
    assert(!_uploadReply);
    
    const QString productName(NATRON_APPLICATION_NAME);
    QString versionStr = getVersionString();
    

    writeDebugMessage("Attempt to upload crash report...");
#ifndef REPORTER_CLI_ONLY
    assert(_dialog);
    _progressDialog = new QProgressDialog(_dialog);
    _progressDialog->setRange(0, 100);
    _progressDialog->setMinimumDuration(100);
    _progressDialog->setLabelText(tr("Uploading crash report..."));
    QObject::connect(_progressDialog, SIGNAL(canceled()), this, SLOT(onProgressDialogCanceled()));
#endif
    
    QFileInfo finfo(filepath);
    if (!finfo.exists()) {
        writeDebugMessage("Dump file (" + filepath + ") does not exist");
        return;
    }
    
    QString guidStr = finfo.fileName();
    {
        int lastDotPos = guidStr.lastIndexOf('.');
        if (lastDotPos != -1) {
            guidStr = guidStr.mid(0, lastDotPos);
        }
    }
    
    QNetworkAccessManager *networkMnger = new QNetworkAccessManager(this);
    QObject::connect(networkMnger, SIGNAL(finished(QNetworkReply*)),
                     this, SLOT(replyFinished()));
    
    //Corresponds to the "multipart/form-data" subtype, meaning the body parts contain form elements, as described in RFC 2388
    // https://www.ietf.org/rfc/rfc2388.txt
    // http://doc.qt.io/qt-4.8/qhttpmultipart.html#ContentType-enum
    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    
    addTextHttpPart(multiPart, "ProductName", productName);
    addTextHttpPart(multiPart, "Version", versionStr);
    addTextHttpPart(multiPart, "guid", guidStr);
    addTextHttpPart(multiPart, "Comments", comments);
    addFileHttpPart(multiPart, "upload_file_minidump", filepath);
    
    QNetworkRequest request(QUrl(UPLOAD_URL));
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
#ifndef REPORTER_CLI_ONLY
    assert(_progressDialog);
    double percent = (double)bytesSent / bytesTotal;
    _progressDialog->setValue(percent * 100);
#endif
}

void
CallbacksManager::replyFinished() {
    if (!_uploadReply || _didError) {
        return;
    }
    
    QByteArray reply = _uploadReply->readAll();
    QString successStr("File uploaded successfully!\n" + QString(reply));
    writeDebugMessage(successStr);
    
#ifndef REPORTER_CLI_ONLY
    QMessageBox info(QMessageBox::Information, "Dump Uploading", successStr, QMessageBox::NoButton, _dialog, Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint| Qt::WindowStaysOnTopHint);
    info.exec();
    
    if (_dialog) {
        _dialog->deleteLater();
    }
#else
    std::cerr << successStr.toStdString() << std::endl;
#endif
    _uploadReply->deleteLater();
    _uploadReply = 0;
    qApp->exit();
}

static QString getNetworkErrorString(QNetworkReply::NetworkError e)
{
    switch (e) {
        case QNetworkReply::ConnectionRefusedError:
            return "The remote server refused the connection (the server is not accepting requests)";
        case QNetworkReply::RemoteHostClosedError:
            return "The remote server closed the connection prematurely, before the entire reply was received and processed";
        case QNetworkReply::HostNotFoundError:
            return "The remote host name was not found (invalid hostname)";
        case QNetworkReply::TimeoutError:
            return "The connection to the remote server timed out";
        case QNetworkReply::OperationCanceledError:
            return "The operation was canceled";
        case QNetworkReply::SslHandshakeFailedError:
            return "The SSL/TLS handshake failed and the encrypted channel could not be established";
        case QNetworkReply::TemporaryNetworkFailureError:
            return "The connection was broken due to disconnection from the network";
        case QNetworkReply::ProxyConnectionRefusedError:
            return "the connection to the proxy server was refused (the proxy server is not accepting requests)";
        case QNetworkReply::ProxyConnectionClosedError:
            return "The proxy server closed the connection prematurely, before the entire reply was received and processed";
        case QNetworkReply::ProxyNotFoundError:
            return "The proxy host name was not found (invalid proxy hostname)";
        case QNetworkReply::ProxyTimeoutError:
            return "The connection to the proxy timed out or the proxy did not reply in time to the request sent";
        case QNetworkReply::ProxyAuthenticationRequiredError:
            return "The proxy requires authentication in order to honour the request but did not accept any credentials offered (if any)";
        case QNetworkReply::ContentAccessDenied:
            return "The access to the remote content was denied (similar to HTTP error 401)";
        case QNetworkReply::ContentOperationNotPermittedError:
            return "The operation requested on the remote content is not permitted";
        case QNetworkReply::ContentNotFoundError:
            return "The remote content was not found at the server (similar to HTTP error 404)";
        case QNetworkReply::AuthenticationRequiredError:
            return "The remote server requires authentication to serve the content but the credentials provided were not accepted (if any)";
        case QNetworkReply::ContentReSendError:
            return "The request needed to be sent again, but this failed for example because the upload data could not be read a second time.";
        case QNetworkReply::ProtocolUnknownError:
            return "The Network Access API cannot honor the request because the protocol is not known";
        case QNetworkReply::ProtocolInvalidOperationError:
            return "The requested operation is invalid for this protocol";
        case QNetworkReply::UnknownNetworkError:
            return "An unknown network-related error was detected";
        case QNetworkReply::UnknownProxyError:
            return "An unknown proxy-related error was detected";
        case QNetworkReply::UnknownContentError:
            return "An unknown error related to the remote content was detected";
        case QNetworkReply::ProtocolFailure:
            return "a breakdown in protocol was detected (parsing error, invalid or unexpected responses, etc.)";
        default:
            return QString();
    }
}

void
CallbacksManager::replyError(QNetworkReply::NetworkError errCode)
{
    if (!_uploadReply) {
        return;
    }
    
    
    QFileInfo finfo(_dumpFilePath);
    if (!finfo.exists()) {
        writeDebugMessage("Dump file (" + _dumpFilePath + ") does not exist");
    }
    
    QString guidStr = finfo.fileName();
    {
        int lastDotPos = guidStr.lastIndexOf('.');
        if (lastDotPos != -1) {
            guidStr = guidStr.mid(0, lastDotPos);
        }
    }
    
    QString errStr("Network error: " + getNetworkErrorString(errCode) + "\nDump file is located at " +
                   _dumpFilePath + "\nYou can submit it directly to the developers by filling out the form at " + QString(FALLBACK_FORM_URL) +
                   " with the following informations:\nProductName: " + NATRON_APPLICATION_NAME + "\nVersion: " + getVersionString() +
                   "\nguid: " + guidStr + "\nPlease add any comment describing the issue and the state of the application at the moment it crashed.");
    writeDebugMessage(errStr);
    
    
    _didError = true;

#ifndef REPORTER_CLI_ONLY
    QMessageBox info(QMessageBox::Critical, "Dump Uploading", errStr, QMessageBox::NoButton, _dialog, Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint| Qt::WindowStaysOnTopHint);
    info.exec();
    if (_dialog) {
        _dialog->deleteLater();
    }
#endif
    _uploadReply->deleteLater();
    _uploadReply = 0;
    
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
    _dumpFilePath = filePath;
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

        Q_EMIT doDumpCallBackOnMainThread(filePath);

    } else {
        writeDebugMessage("Dump file does not seem to exist...exiting crash reporter now.");
    }
}

#ifdef TRACE_CRASH_RERPORTER
void
CallbacksManager::writeDebugMessage(const QString& str)
{
    QMutexLocker k(&_dFileMutex);
    QTextStream ts(_dFile);
    ts << str << '\n';
}
#endif



/*Converts a std::string to wide string*/
inline std::wstring
s2ws(const std::string & s)
{
    
    
#ifdef Q_OS_WIN32
    int len;
    int slength = (int)s.length() + 1;
    len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, 0, 0);
    wchar_t* buf = new wchar_t[len];
    MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, buf, len);
    std::wstring r(buf);
    delete[] buf;
    return r;
#else
    std::wstring dest;
    
    size_t max = s.size() * 4;
    mbtowc (NULL, NULL, max);  /* reset mbtowc */
    
    const char* cstr = s.c_str();
    
    while (max > 0) {
        wchar_t w;
        size_t length = mbtowc(&w,cstr,max);
        if (length < 1) {
            break;
        }
        dest.push_back(w);
        cstr += length;
        max -= length;
    }
    return dest;
#endif
    
}


void dumpRequest_xPlatform(const QString& filePath)
{
    CallbacksManager::instance()->s_emitDoCallBackOnMainThread(filePath);
}

#if defined(Q_OS_MAC)
void OnClientDumpRequest(void */*context*/,
                         const ClientInfo &/*client_info*/,
                         const std::string &file_path) {
    
    dumpRequest_xPlatform(file_path.c_str());
}
#elif defined(Q_OS_LINUX)
void OnClientDumpRequest(void* /*context*/,
                         const ClientInfo* /*client_info*/,
                         const string* file_path) {
    
    dumpRequest_xPlatform(file_path->c_str());
}
#elif defined(Q_OS_WIN32)
void OnClientDumpRequest(void* /*context*/,
                         const google_breakpad::ClientInfo* /*client_info*/,
                         const std::wstring* file_path) {
    
    QString str = QString::fromWCharArray(file_path->c_str());
    dumpRequest_xPlatform(str);
}
#endif

void
CallbacksManager::startCrashGenerationServer()
{
     writeDebugMessage("Starting crash generation server and notifying Natron...");

    /*
     * We initialize the CrashGenerationServer now that the connection is made between Natron & the Crash reporter
     */
#if defined(Q_OS_MAC)
    _crashServer = new CrashGenerationServer(_pipePath.toStdString().c_str(),
                                             0, // filter cb
                                             0, // filter ctx
                                             OnClientDumpRequest, // dump cb
                                             0, // dump ctx
                                             0, // exit cb
                                             0, // exit ctx
                                             true, // auto-generate dumps
                                             _dumpDirPath.toStdString()); // path to dump to
#elif defined(Q_OS_LINUX)
     std::string stdDumpPath = _dumpDirPath.toStdString();
    _crashServer = new CrashGenerationServer(_serverFD,
                                          OnClientDumpRequest, // dump cb
                                          0, // dump ctx
                                          0, // exit cb
                                          0, // exit ctx
                                          true, // auto-generate dumps
                                          &stdDumpPath); // path to dump to
#elif defined(Q_OS_WIN32)
    _pipePath.replace("/","\\");
    std::string pipeName = _pipePath.toStdString();
    std::wstring wpipeName = s2ws(pipeName);
    std::string stdDumPath = _dumpDirPath.toStdString();
    std::wstring stdWDumpPath = s2ws(stdDumPath);
    _crashServer = new CrashGenerationServer(wpipeName,
                                          0, // SECURITY ATTRS
                                          0, // on client connected cb
                                          0, // on client connected ctx
                                          OnClientDumpRequest, // dump cb
                                          0, // dump ctx
                                          0, // exit cb
                                          0, // exit ctx
                                          0, // upload request cb
                                          0, //  upload request ctx
                                          true, // auto-generate dumps
                                          &stdWDumpPath); // path to dump to
#endif
    if (!_crashServer->Start()) {
        writeDebugMessage("Failure to start breakpad server, crash generation will fail.");
        qApp->exit();
        return;
    } else {
        writeDebugMessage("Crash generation server started successfully.");
    }

    //Notify Natron that the server is started
    QString message("-i");
    writeToOutputPipe(message);
}

void
CallbacksManager::onOutputPipeConnectionMade()
{
    writeDebugMessage("IPC pipe with Natron successfully connected.");

    //At this point we're sure that the server is created, so we notify Natron about it so it can create its ExceptionHandler
    startCrashGenerationServer();



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
CallbacksManager::initOuptutPipe(const QString& comPipeName,
                                 const QString& pipeName,
                                 const QString& dumpPath, int server_fd)
{
    assert(!_outputPipe);
    _dumpDirPath = dumpPath;
    _pipePath = pipeName;
    _serverFD = server_fd;

    //Request a connection to the socket opened by Natron for our own IPC. We use it to notify Natron that our crash generation server has been created
    _outputPipe = new QLocalSocket;
    QObject::connect( _outputPipe, SIGNAL( connected() ), this, SLOT( onOutputPipeConnectionMade() ) );
    _outputPipe->connectToServer(comPipeName,QLocalSocket::ReadWrite);


    
}
