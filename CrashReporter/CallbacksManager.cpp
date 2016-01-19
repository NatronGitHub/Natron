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

#include <stdexcept>

#include <QtCore/QDebug>

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QHttpMultiPart>

#define UPLOAD_URL "http://breakpad.natron.fr/submit"
#define FALLBACK_FORM_URL "http://breakpad.natron.fr/form/"

CallbacksManager* CallbacksManager::_instance = 0;

#include <cassert>
#include <iostream>
#include <sstream>
#include <exception>

#ifndef REPORTER_CLI_ONLY
#include <QApplication>
#include <QProgressDialog>
#include <QMessageBox>
#include <QDialogButtonBox>
#include <QDialog>
#include <QVBoxLayout>
#include <QLabel>
#include <QTextEdit>
#else //!REPORTER_CLI_ONLY
#include <QCoreApplication>
#endif

#include <QLocalSocket>
#include <QLocalServer>
#include <QThread>
#include <QFile>
#include <QDir>
#include <QTemporaryFile>
#include <QFileInfo>
#include <QFile>
#include <QProcess>
#include <QStringList>
#include <QString>
#include <QVarLengthArray>
#include <QSettings>

#ifdef DEBUG
#include <QTextStream>
#endif


#if defined(Q_OS_MAC)
#include "client/mac/crash_generation/crash_generation_server.h"
#include <execinfo.h>
#include <cstdio>
#include <sys/errno.h>
#include <sys/signal.h>
#include <sys/wait.h>
#elif defined(Q_OS_LINUX)
#include "client/linux/crash_generation/crash_generation_server.h"

#ifndef __USE_GNU
#define __USE_GNU
#endif
#include <ucontext.h>
#include <execinfo.h>
#include <cstdio>
#include <errno.h>
#include <sys/wait.h>
#elif defined(Q_OS_WIN32)
#include <windows.h>
#include "client/windows/crash_generation/crash_generation_server.h"
#endif

#ifndef REPORTER_CLI_ONLY
#include "CrashDialog.h"
#endif

#include "Global/ProcInfo.h"

#define EXIT_APP(code,exitIfDumpReceived) ( CallbacksManager::instance()->s_emitDoExitCallBackOnMainThread(code,exitIfDumpReceived) )

namespace {
#ifdef NATRON_CRASH_REPORTER_USE_FORK

static char*
qstringToMallocCharArray(const QString& str)
{
    std::string stdStr = str.toStdString();
    return strndup(stdStr.c_str(), stdStr.size() + 1);
}


static void
handleChildDeadSignal( int /*signalId*/ )
{
    /*
         In Practise this handler is only called on OS X when the Natron application exits.
         On Linux this may be called when ptrace dumper suspends the thread of Natron, hence we handle it
         separately
         */
#ifdef Q_OS_LINUX
    int status;
    pid_t pid = waitpid(WAIT_ANY, &status, WUNTRACED | WNOHANG);
     if (pid <= 0 || errno == ECHILD) {
         //The child process is not really finished, this was probably called from breakpad, ignore it
         return;
     }
#endif
    if (qApp) {
        EXIT_APP(0, false);
    } else {
        std::exit(1);
    }
}

static void
setChildDeadSignal()
{
    struct sigaction sa;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = handleChildDeadSignal;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        std::perror("setting up termination signal");
        std::exit(1);
    }
}

#endif // NATRON_CRASH_REPORTER_USE_FORK
} // anon namespace


void dumpRequest_xPlatform(const QString& filePath)
{
    CallbacksManager::instance()->s_emitDoDumpCallBackOnMainThread(filePath);
}

void exitRequest_xPlatform()
{
    CallbacksManager::instance()->s_emitDoExitCallBackOnMainThread(0, false);
}

using namespace google_breakpad;

CallbacksManager::CallbacksManager()
: QObject()
#ifndef Q_OS_LINUX
, _breakpadPipeServer(0)
#endif
#ifndef NATRON_CRASH_REPORTER_USE_FORK
, _natronProcess(0)
#endif
, _comServer(0)
, _comPipeConnection(0)
, _uploadReply(0)
, _dumpReceived(false)
#ifndef REPORTER_CLI_ONLY
, _dialog(0)
, _progressDialog(0)
#endif
, _didError(false)
, _dumpFilePath()
, _pipePath()
, _comPipePath()
, _crashServer(0)
#ifdef Q_OS_LINUX
, _serverFD(-1)
, _clientFD(-1)
#endif
, _app(0)
{
    _instance = this;
}


CallbacksManager::~CallbacksManager() {
#ifdef TRACE_CRASH_RERPORTER
    delete _dFile;
#endif

#ifndef Q_OS_LINUX
    delete _breakpadPipeServer;
#endif
    
    delete _comServer;
    
    delete _crashServer;
    
#ifndef NATRON_CRASH_REPORTER_USE_FORK
    if (_natronProcess) {
        //Wait at most 5sec then exit
        _natronProcess->waitForFinished(5000);
        delete _natronProcess;
    }
#endif
    
    delete _app;
    
    _instance = 0;


}

void
CallbacksManager::initQApplication(int &argc, char** argv)
{
#ifdef REPORTER_CLI_ONLY
    _app = new QCoreApplication(argc,argv);
#else
    _app = new QApplication(argc,argv);
    _app->setQuitOnLastWindowClosed(false);
#endif
    
}

int
CallbacksManager::exec()
{
    assert(_app);
    return _app->exec();
}

static QString getNatronBinaryFilePathFromCrashReporterDirPath(const QString& crashReporterDirPath)
{
    QString ret = crashReporterDirPath;
    if (!ret.endsWith('/')) {
        ret.append('/');
    }
    
#ifndef REPORTER_CLI_ONLY
#ifdef Q_OS_UNIX
    ret.append("Natron-bin");
#elif defined(Q_OS_WIN)
    ret.append("Natron-bin.exe");
#endif
#else //REPORTER_CLI_ONLY
#ifdef Q_OS_UNIX
    ret.append("NatronRenderer-bin");
#elif defined(Q_OS_WIN)
    ret.append("NatronRenderer-bin.exe");
#endif
#endif // REPORTER_CLI_ONLY
    return ret;
}

bool
CallbacksManager::hasReceivedDump() const
{
    assert(QThread::currentThread() == qApp->thread());
    return _dumpReceived;
}

void
CallbacksManager::init(int& argc, char** argv)
{
#ifdef NATRON_USE_BREAKPAD
    bool enableBreakpad = true;
#else
    bool enableBreakpad = false;
#endif
    
    /*
     Check the value of the "Enable crash reports" parameter of the preferences of Natron
     */
    QSettings settings(NATRON_ORGANIZATION_NAME,NATRON_APPLICATION_NAME);
    if (settings.contains("enableCrashReports")) {
        bool userEnableCrashReports = settings.value("enableCrashReports").toBool();
        if (!userEnableCrashReports) {
            enableBreakpad = false;
        }
    }

    
    initQApplication(argc, argv);

    
    if (enableBreakpad) {
        //This will throw an exception if it fails to start the crash generation server
        initCrashGenerationServer();
        assert(_crashServer);
    }

    /*
     At this point the crash generation server is created
     qApp has been defined so far
     */
    assert(_app);
    
    QString crashReporterBinaryFilePath = qApp->applicationFilePath();
    QString natronBinaryPath = qApp->applicationDirPath();
    natronBinaryPath = getNatronBinaryFilePathFromCrashReporterDirPath(natronBinaryPath);
    
    if (!QFile::exists(natronBinaryPath)) {
        std::stringstream ss;
        ss << natronBinaryPath.toStdString() << ": no such file or directory.";
        throw std::runtime_error(ss.str());
        return;
    }
    
#ifndef NATRON_CRASH_REPORTER_USE_FORK
    
    _natronProcess = new QProcess();
    QObject::connect(_natronProcess,SIGNAL(readyReadStandardOutput()), this, SLOT(onNatronProcessStdOutWrittenTo()));
    QObject::connect(_natronProcess,SIGNAL(readyReadStandardError()), this, SLOT(onNatronProcessStdErrWrittenTo()));
    

    QStringList processArgs;
    for (int i = 0; i < argc; ++i) {
        processArgs.push_back(QString(argv[i]));
    }
    if (enableBreakpad) {
        processArgs.push_back(QString("--" NATRON_BREAKPAD_PROCESS_EXEC));
        processArgs.push_back(crashReporterBinaryFilePath);
        processArgs.push_back(QString("--" NATRON_BREAKPAD_PROCESS_PID));
        qint64 crashReporterPid = _app->applicationPid();
        QString pidStr = QString::number(crashReporterPid);
        processArgs.push_back(pidStr);
        processArgs.push_back(QString("--" NATRON_BREAKPAD_CLIENT_FD_ARG));
        processArgs.push_back(QString::number(-1));
        processArgs.push_back(QString("--" NATRON_BREAKPAD_PIPE_ARG));
        processArgs.push_back(_pipePath);
        processArgs.push_back(QString("--" NATRON_BREAKPAD_COM_PIPE_ARG));
        processArgs.push_back(_comPipePath);
    }
    
    _natronProcess->start(natronBinaryPath, processArgs);
    
    //_natronProcess = natronProcess;
#else // NATRON_CRASH_REPORTER_USE_FORK

    
    setChildDeadSignal();
    
    std::vector<char*> argvChild(argc);
    for (int i = 0; i < argc; ++i) {
        argvChild[i] = strdup(argv[i]);
    }
    
    if (enableBreakpad) {
        argvChild.push_back(strdup("--" NATRON_BREAKPAD_PROCESS_EXEC));
        argvChild.push_back(qstringToMallocCharArray(crashReporterBinaryFilePath));
        
        argvChild.push_back(strdup("--" NATRON_BREAKPAD_PROCESS_PID));
        QString pidStr = QString::number((qint64)getpid());
        argvChild.push_back(qstringToMallocCharArray(pidStr));
        
        argvChild.push_back(strdup("--" NATRON_BREAKPAD_CLIENT_FD_ARG));
        {
            char pipe_fd_string[8];
            sprintf(pipe_fd_string, "%d",
#ifdef Q_OS_LINUX
                    _clientFD
#else
                    -1
#endif
                    );
            argvChild.push_back(pipe_fd_string);
        }
        argvChild.push_back(strdup("--" NATRON_BREAKPAD_PIPE_ARG));
        argvChild.push_back(qstringToMallocCharArray(_pipePath));
        argvChild.push_back(strdup("--" NATRON_BREAKPAD_COM_PIPE_ARG));
        argvChild.push_back(qstringToMallocCharArray(_comPipePath));
    }
    argvChild.push_back(NULL);

    char** childargv_p = &argvChild.front();
    std::string natronStdBinPath = natronBinaryPath.toStdString();
    const char* natronBinPath_p = natronStdBinPath.c_str();
    
    /*
     Directly fork this process so that we have no variable yet allocated.
     The child process will exec the actual Natron process. We have to do this in order
     for google-breakpad to correctly work:
     This process will use ptrace to inspect the crashing thread of Natron, but it can only
     do so on some Linux distro if Natron is a child process.
     Also the crash reporter and Natron must share the same file descriptors for the crash generation
     server to work.
     */
    pid_t natronPID = fork();
    
    if (natronPID == -1) {
        std::stringstream ss;
        ss << "Fork failed";
        throw std::runtime_error(ss.str());
        return;
    } else if (natronPID == 0) {
        /*
         We are the child process (i.e: Natron)
         */

        if (execv(natronBinPath_p,childargv_p) < 0) {
            std::stringstream ss;
            ss << "Forked process failed to execute " << natronBinaryPath.toStdString() << ": " << strerror(errno) << " (" << errno << ")";
            throw std::runtime_error(ss.str());
            return;
        }
        
       
    } // child process

#endif // NATRON_CRASH_REPORTER_USE_FORK
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
        std::cerr << "Failed to open the following file for uploading: " + filePath.toStdString() << std::endl;
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
    

#ifndef REPORTER_CLI_ONLY
    assert(_dialog);
    _progressDialog = new QProgressDialog(_dialog);
    _progressDialog->setRange(0, 100);
    _progressDialog->setMinimumDuration(100);
    _progressDialog->setLabelText(tr("Uploading crash report..."));
    QObject::connect(_progressDialog, SIGNAL(canceled()), this, SLOT(onProgressDialogCanceled()));
#else
    std::cerr << tr("Crash report received and located in: ").toStdString() << std::endl;
    std::cerr << filepath.toStdString() << std::endl;
    std::cerr << tr("Uploading crash report...").toStdString() << std::endl;
#endif
    
    QFileInfo finfo(filepath);
    if (!finfo.exists()) {
        std::cerr << tr("Dump File (").toStdString() << filepath.toStdString() << tr(") does not exist").toStdString() << std::endl;
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
#else
    Q_UNUSED(bytesSent);
    Q_UNUSED(bytesTotal);
#endif
}

void
CallbacksManager::replyFinished() {
    if (!_uploadReply || _didError) {
        return;
    }
    
    QByteArray reply = _uploadReply->readAll();
    while (reply.endsWith('\n')) {
        reply.chop(1);
    }
    QString successStr("File uploaded successfully!\n" + QString(reply));
    
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
    EXIT_APP(0, true);
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

#ifndef REPORTER_CLI_ONLY


NetworkErrorDialog::NetworkErrorDialog(const QString& errorMessage, QWidget* parent)
: QDialog(parent)
, mainLayout(0)
, textArea(0)
, buttons(0)
{
    mainLayout = new QVBoxLayout(this);
    textArea = new QTextEdit(this);
    textArea->setTextInteractionFlags(Qt::TextBrowserInteraction);
    textArea->setPlainText(errorMessage);
    
    buttons = new QDialogButtonBox(QDialogButtonBox::Ok, Qt::Horizontal, this);
    QObject::connect(buttons, SIGNAL(accepted()), this, SLOT(accept()));
    QObject::connect(buttons, SIGNAL(rejected()), this, SLOT(reject()));
    
    mainLayout->addWidget(textArea);
    mainLayout->addWidget(buttons);
}

NetworkErrorDialog::~NetworkErrorDialog()
{
    
}
#endif

void
CallbacksManager::replyError(QNetworkReply::NetworkError errCode)
{
    if (!_uploadReply) {
        return;
    }
    if (errCode == QNetworkReply::UnknownContentError) {
        return;
    }
    
    QFileInfo finfo(_dumpFilePath);
    if (!finfo.exists()) {
        std::cerr << "Dump file (" <<  _dumpFilePath.toStdString() << ") does not exist";
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
                   "\nguid: " + guidStr + "\n\nPlease add any comment describing the issue and the state of the application at the moment it crashed.");
    
    _didError = true;

#ifndef REPORTER_CLI_ONLY
    NetworkErrorDialog info(errStr,_dialog);
    info.setWindowTitle("Dump Uploading");
    info.exec();
    
    if (_dialog) {
        _dialog->deleteLater();
    }
#else
    std::cerr << errStr.toStdString() << std::endl;
#endif
    _uploadReply->deleteLater();
    _uploadReply = 0;
    
    EXIT_APP(0, true);
}

void
CallbacksManager::onDoDumpOnMainThread(const QString& filePath)
{

    assert(QThread::currentThread() == qApp->thread());

    _dumpReceived = true;
    
#ifdef REPORTER_CLI_ONLY
    uploadFileToRepository(filePath,"Crash auto-uploaded from NatronRenderer");
    
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
    
    if (doUpload) {
        uploadFileToRepository(_dialog->getOriginalDumpFilePath(),_dialog->getDescription());
    } else {
        _dialog->deleteLater();
        EXIT_APP(0, true);
    }

#endif

}

void
CallbacksManager::s_emitDoDumpCallBackOnMainThread(const QString& filePath)
{
    if (QFile::exists(filePath)) {
        Q_EMIT doDumpCallBackOnMainThread(filePath);
    }
}

void
CallbacksManager::s_emitDoExitCallBackOnMainThread(int exitCode, bool exitEvenIfDumpedReceived)
{
    Q_EMIT doExitCallbackOnMainThread(exitCode,exitEvenIfDumpedReceived);
}

void
CallbacksManager::onDoExitOnMainThread(int exitCode, bool exitEvenIfDumpedReceived)
{
    if (!CallbacksManager::instance()->hasReceivedDump() || exitEvenIfDumpedReceived) {
        qApp->exit(exitCode);
    }
}

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





#if defined(Q_OS_MAC)
void OnClientDumpRequest(void */*context*/,
                         const ClientInfo &/*client_info*/,
                         const std::string &file_path) {
    
    dumpRequest_xPlatform(file_path.c_str());
}
void onClientExitRequest(void* /*context*/,
                          const ClientInfo& /*client_info*/)
{
    exitRequest_xPlatform();
}
#elif defined(Q_OS_LINUX)
void OnClientDumpRequest(void* /*context*/,
                         const ClientInfo* /*client_info*/,
                         const string* file_path) {
    
    dumpRequest_xPlatform(file_path->c_str());
}
void onClientExitRequest(void* /*context*/,
                          const ClientInfo* /*client_info*/)
{
    exitRequest_xPlatform();
}

#elif defined(Q_OS_WIN32)
void OnClientDumpRequest(void* /*context*/,
                         const google_breakpad::ClientInfo* /*client_info*/,
                         const std::wstring* file_path) {
    
    QString str = QString::fromWCharArray(file_path->c_str());
    dumpRequest_xPlatform(str);
}
void onClientExitRequest(void* /*context*/,
                          const ClientInfo* /*client_info*/)
{
    exitRequest_xPlatform();
}
#endif

void
CallbacksManager::createCrashGenerationServer()
{

    /*
     * We initialize the CrashGenerationServer now that the connection is made between Natron & the Crash reporter
     */
#if defined(Q_OS_MAC)
    _crashServer = new CrashGenerationServer(_pipePath.toStdString().c_str(),
                                             0, // filter cb
                                             0, // filter ctx
                                             OnClientDumpRequest, // dump cb
                                             0, // dump ctx
                                             onClientExitRequest, // exit cb
                                             0, // exit ctx
                                             true, // auto-generate dumps
                                             _dumpDirPath.toStdString()); // path to dump to
#elif defined(Q_OS_LINUX)
     std::string stdDumpPath = _dumpDirPath.toStdString();
    _crashServer = new CrashGenerationServer(_serverFD,
                                          OnClientDumpRequest, // dump cb
                                          0, // dump ctx
                                          onClientExitRequest, // exit cb
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
                                          onClientExitRequest, // exit cb
                                          0, // exit ctx
                                          0, // upload request cb
                                          0, //  upload request ctx
                                          true, // auto-generate dumps
                                          &stdWDumpPath); // path to dump to
#endif
    if (!_crashServer->Start()) {
        std::stringstream ss;
        ss << "Failure to start breakpad server, crash generation will fail." << std::endl;
        ss << "Dump path is: " << _dumpDirPath.toStdString() << std::endl;
        ss << "Pipe name is: " << _pipePath.toStdString() << std::endl;
        throw std::runtime_error(ss.str());
    }
}



void
CallbacksManager::onNatronProcessStdOutWrittenTo()
{
#ifndef NATRON_CRASH_REPORTER_USE_FORK
    QString str(_natronProcess->readAllStandardOutput().data());
    while (str.endsWith('\n')) {
        str.chop(1);
    }
    std::cout << str.toStdString() << std::endl;
#endif
}

void
CallbacksManager::onNatronProcessStdErrWrittenTo()
{
#ifndef NATRON_CRASH_REPORTER_USE_FORK
    QString str(_natronProcess->readAllStandardError().data());
    while (str.endsWith('\n')) {
        str.chop(1);
    }
    std::cerr << str.toStdString() << std::endl;
#endif
}


static QString getTempDirPath()
{
#ifdef Q_OS_UNIX

    QString temp = QString(qgetenv("TMPDIR"));
    if (temp.isEmpty()) {
        temp = QLatin1String("/tmp/");
    }
    return QDir::cleanPath(temp);
#else
    QString ret;
    wchar_t tempPath[MAX_PATH];
    DWORD len = GetTempPathW(MAX_PATH, tempPath);
    if (len)
        ret = QString::fromWCharArray(tempPath, len);
    if (!ret.isEmpty()) {
        while (ret.endsWith(QLatin1Char('\\')))
            ret.chop(1);
        ret = QDir::fromNativeSeparators(ret);
    }
    if (ret.isEmpty()) {
#if !defined(Q_OS_WINCE)
        ret = QLatin1String("C:/tmp");
#else
        ret = QLatin1String("/Temp");
#endif
    } else if (ret.length() >= 2 && ret[1] == QLatin1Char(':')) {
        ret[0] = ret.at(0).toUpper(); // Force uppercase drive letters.
    }
    return ret;
#endif
}


void
CallbacksManager::initCrashGenerationServer()
{
    
#ifndef Q_OS_LINUX
    assert(!_breakpadPipeServer);
#endif
    
    QObject::connect(this, SIGNAL(doDumpCallBackOnMainThread(QString)), this, SLOT(onDoDumpOnMainThread(QString)));
    QObject::connect(this, SIGNAL(doExitCallbackOnMainThread(int,bool)), this, SLOT(onDoExitOnMainThread(int,bool)));

#ifdef Q_OS_LINUX
    if (!google_breakpad::CrashGenerationServer::CreateReportChannel(&_serverFD, &_clientFD)) {
        throw std::runtime_error("breakpad: Failure to create named pipe for the CrashGenerationServer (CreateReportChannel).");
        return;
    }
#endif
    
    /*
     Use a temporary file to get a random file name for the pipes.
     We use 2 different pipe: 1 for the CrashReporter to notify to Natron that it has started correctly, and another
     one that is used by the google_breakpad server itself.
     */

    _dumpDirPath = getTempDirPath();
    {
        QString tmpFileName;
#ifdef Q_OS_WIN32
        tmpFileName += "//./pipe/";
#elif defined(Q_OS_UNIX)
        tmpFileName	= _dumpDirPath;
        tmpFileName += '/';
        tmpFileName += NATRON_APPLICATION_NAME;
#endif
        tmpFileName += "_CRASH_PIPE_";
        {
            QString tmpTemplate;
#ifndef Q_OS_WIN32
            tmpTemplate.append(tmpFileName);
#endif
            QTemporaryFile tmpf(tmpTemplate);
            tmpf.open(); // this will append a random unique string  to the passed template (tmpFileName)
            tmpFileName = tmpf.fileName();
            tmpf.remove();
        }
        
#ifndef Q_OS_WIN32
        _pipePath = tmpFileName;
#else
        int foundLastSlash = tmpFileName.lastIndexOf('/');
        if (foundLastSlash !=1) {
            _pipePath = "//./pipe/";
            if (foundLastSlash < tmpFileName.size() - 1) {
                _pipePath.append(tmpFileName.mid(foundLastSlash + 1));
            }
        }
#endif
        
    }
    
    _comPipePath = _pipePath + "IPC_COM";
    
#ifndef Q_OS_LINUX
    //Create the crash generation pipe ourselves
    //qApp has been defined so far
    assert(qApp);
    _breakpadPipeServer = new QLocalServer;
    _breakpadPipeServer->listen(_pipePath);
    
#endif

    _comServer = new QLocalServer;
    _comServer->listen(_comPipePath);
    QObject::connect(_comServer, SIGNAL(newConnection()), this, SLOT(onComPipeConnectionPending()));

    createCrashGenerationServer();
    
}

void
CallbacksManager::onComPipeConnectionPending()
{
    _comPipeConnection = _comServer->nextPendingConnection();
    QObject::connect(_comPipeConnection, SIGNAL(readyRead()), this, SLOT(onComPipeDataWrittenTo()));
}

void
CallbacksManager::onComPipeDataWrittenTo()
{
    QString str = _comPipeConnection->readLine();
    while (str.endsWith('\n')) {
        str.chop(1);
    }
    if (str == NATRON_NATRON_TO_BREAKPAD_EXISTENCE_CHECK) {
        _comPipeConnection->write(NATRON_NATRON_TO_BREAKPAD_EXISTENCE_CHECK_ACK "\n");
        _comPipeConnection->flush();
    }
}
