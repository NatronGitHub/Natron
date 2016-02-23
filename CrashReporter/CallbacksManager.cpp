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
#include <string>
#include <cstring>
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
#endif

#include <QCoreApplication>
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
#include "Global/GitVersion.h"

#define EXIT_APP(code,exitIfDumpReceived) ( CallbacksManager::instance()->s_emitDoExitCallBackOnMainThread(code,exitIfDumpReceived) )

namespace {
#ifdef NATRON_CRASH_REPORTER_USE_FORK


// strndup doesn't exist on OS X prior to 10.7
static char *
strndup_replacement(const char *str, size_t n)
{
    size_t len;
    char *copy;
    
    for (len = 0; len < n && str[len]; ++len)
        continue;
    
    if ((copy = (char *)malloc(len + 1)) == NULL)
        return (NULL);
    memcpy(copy, str, len);
    copy[len] = '\0';
    return (copy);
}
    
static char*
qstringToMallocCharArray(const QString& str)
{
    std::string stdStr = str.toStdString();
    return strndup_replacement(stdStr.c_str(), stdStr.size() + 1);
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


using namespace google_breakpad;

CallbacksManager::CallbacksManager()
: QObject()
#ifndef NATRON_CRASH_REPORTER_USE_FORK
, _natronProcess(0)
#endif
, _comServer(0)
, _comPipeConnection(0)
, _uploadReply(0)
, _dumpReceived(false)
, _mustInitQAppAfterDump(false)
#ifndef REPORTER_CLI_ONLY
, _dialog(0)
, _progressDialog(0)
#endif
, _dumpFilePath()
, _pipePath()
, _comPipePath()
, _crashServer(0)
#ifdef Q_OS_LINUX
, _serverFD(-1)
, _clientFD(-1)
#endif
, _initErr(false)
{
    _instance = this;
}


CallbacksManager::~CallbacksManager() {

    delete _comServer;
    
    delete _crashServer;
    
#ifndef NATRON_CRASH_REPORTER_USE_FORK
    if (_natronProcess) {
        //Wait at most 5sec then exit
        _natronProcess->waitForFinished(5000);
        delete _natronProcess;
    }
#endif
    
    
    delete qApp;
    
    _instance = 0;


}

void
CallbacksManager::initQApplication(int &argc, char** argv)
{
    _argc = argc;
    _argv = argv;
    new QCoreApplication(argc,argv);
}

int
CallbacksManager::exec()
{
    assert(qApp);
    int retCode =  qApp->exec();
#ifndef REPORTER_CLI_ONLY
    if (_mustInitQAppAfterDump) {
        
        /*
         We are in gui mode, clean-up everything related to Qt, Kill
         the QCoreApplication and create a QApplication to pop-up the crash dialog instead.
         We do this so that the application icon showsup in the taskbar/dock only now and not when
         we started the crash reporter
         */
        _mustInitQAppAfterDump = false;
        
        /*
         Cleanup everything related
         */
        delete _comServer;
        _comServer = 0;
        
        delete _crashServer;
        _crashServer = 0;
        
#ifndef NATRON_CRASH_REPORTER_USE_FORK
        _natronProcess->waitForFinished(2000);
        delete _natronProcess;
        _natronProcess = 0;
#endif
        
        delete qApp;
        
        new QApplication(_argc,_argv);
        assert(qApp);
        qApp->setQuitOnLastWindowClosed(false);
        
        processCrashReport();
        
        retCode = qApp->exec();
    }
#endif // REPORTER_CLI_ONLY
    return retCode;
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

bool
CallbacksManager::hasInit() const {
    return !_initErr;
}

void
CallbacksManager::onSpawnedProcessFinished(int exitCode, QProcess::ExitStatus status)
{
    QString code;
    if (status == QProcess::CrashExit) {
        code =  "crashed";
    } else if (status == QProcess::NormalExit) {
        code =  "finished";
    }
    qDebug() << "Spawned process" << code << "with exit code:" << exitCode;
    
    EXIT_APP(exitCode, false);
}

void
CallbacksManager::onSpawnedProcessError(QProcess::ProcessError error)
{
    
    bool mustExit = false;
    switch (error) {
        case QProcess::FailedToStart:
            std::cerr << "Spawned process executable does not exist or the system lacks necessary resources, exiting." << std::endl;
            mustExit = true;
            break;
        case QProcess::Crashed:
            std::cerr << "Spawned process crashed, exiting." << std::endl;
            mustExit = true;
            break;
        case QProcess::Timedout:
            std::cerr << "Spawned process timedout, exiting." << std::endl;
            mustExit = true;
            break;
        case QProcess::ReadError:
            break;
        case QProcess::WriteError:
            break;
        case QProcess::UnknownError:
            break;
    }
    
    if (mustExit) {
        _initErr = true;
        EXIT_APP(1, false);
    }
    
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
    assert(qApp);
    
    QString crashReporterBinaryFilePath = qApp->applicationFilePath();
    QString natronBinaryPath = qApp->applicationDirPath();
    natronBinaryPath = getNatronBinaryFilePathFromCrashReporterDirPath(natronBinaryPath);
    
    if (!QFile::exists(natronBinaryPath)) {
        _initErr = true;
        std::stringstream ss;
        ss << natronBinaryPath.toStdString() << ": no such file or directory.";
        throw std::runtime_error(ss.str());
        return;
    }
    
#ifndef NATRON_CRASH_REPORTER_USE_FORK
    
    _natronProcess = new QProcess();
    QObject::connect(_natronProcess,SIGNAL(readyReadStandardOutput()), this, SLOT(onNatronProcessStdOutWrittenTo()));
    QObject::connect(_natronProcess,SIGNAL(readyReadStandardError()), this, SLOT(onNatronProcessStdErrWrittenTo()));
    QObject::connect(_natronProcess,SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(onSpawnedProcessFinished(int,QProcess::ExitStatus)));
    QObject::connect(_natronProcess,SIGNAL(error(QProcess::ProcessError)), this, SLOT(onSpawnedProcessError(QProcess::ProcessError)));

    QStringList processArgs;
    for (int i = 0; i < argc; ++i) {
        processArgs.push_back(QString(argv[i]));
    }
    if (enableBreakpad) {
        processArgs.push_back(QString("--" NATRON_BREAKPAD_PROCESS_EXEC));
        processArgs.push_back(crashReporterBinaryFilePath);
        processArgs.push_back(QString("--" NATRON_BREAKPAD_PROCESS_PID));
        qint64 crashReporterPid = qApp->applicationPid();
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
    const QString gitHash(GIT_COMMIT);
    const QString gitBranch(GIT_BRANCH);

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

    
    //Corresponds to the "multipart/form-data" subtype, meaning the body parts contain form elements, as described in RFC 2388
    // https://www.ietf.org/rfc/rfc2388.txt
    // http://doc.qt.io/qt-4.8/qhttpmultipart.html#ContentType-enum
    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    
    addTextHttpPart(multiPart, "ProductName", productName);
    addTextHttpPart(multiPart, "Version", versionStr);
    addTextHttpPart(multiPart, "GitHash", gitHash);
    addTextHttpPart(multiPart, "GitBranch", gitBranch);
    addTextHttpPart(multiPart, "Version", versionStr);
    addTextHttpPart(multiPart, "guid", guidStr);
    addTextHttpPart(multiPart, "Comments", comments);
    addFileHttpPart(multiPart, "upload_file_minidump", filepath);
    
    QUrl url = QUrl::fromEncoded(QByteArray(UPLOAD_URL));
    QNetworkRequest request(url);
    _uploadReply = networkMnger->post(request, multiPart);
    
    QObject::connect(networkMnger, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyFinished(QNetworkReply*)));
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
CallbacksManager::replyFinished(QNetworkReply* replyParam)
{
    Q_UNUSED(replyParam);
    assert(replyParam == _uploadReply);

    if (!_uploadReply) {
        return;
    }
    
    QNetworkReply::NetworkError err = _uploadReply->error();
    if (err == QNetworkReply::NoError) {
        
        QByteArray reply = _uploadReply->readAll();
        while (reply.endsWith('\n')) {
            reply.chop(1);
        }
        QString successStr("File uploaded successfully!\n Crash ID = " + QString(reply));
        
#ifndef REPORTER_CLI_ONLY
        QMessageBox info(QMessageBox::Information, "Dump Uploading", successStr, QMessageBox::NoButton, _dialog, Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint| Qt::WindowStaysOnTopHint);
        info.exec();
        
        if (_dialog) {
            _dialog->deleteLater();
        }
#else
        std::cerr << successStr.toStdString() << std::endl;
#endif
        
    } else {
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
        
        QString errStr("Network error: (" + QString::number(err) + ") " + _uploadReply->errorString() + "\nDump file is located at " +
                       _dumpFilePath + "\nYou can submit it directly to the developers by filling out the form at\n\n" + QString(FALLBACK_FORM_URL) +
                       "?product=" + NATRON_APPLICATION_NAME + "&version=" + getVersionString() +
                       "&id=" + guidStr + "\n\nPlease add any comment describing the issue and the state of the application at the moment it crashed.");
        
        
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

    }
    
    _uploadReply = 0;
    EXIT_APP(0, true);
}

void
CallbacksManager::onDoDumpOnMainThread(const QString& filePath)
{

    assert(QThread::currentThread() == qApp->thread());

    _dumpReceived = true;
    _dumpFilePath = filePath;
#ifndef REPORTER_CLI_ONLY
    /*
     We are in need to popup the dialog but are still governed by the QCoreApplication, quit the event loop and re-create a 
     QApplication, see exec()
     */
    _mustInitQAppAfterDump = true;
    qApp->exit(0);
    return;
#else
    //In CLI mode we don't need a QApplication...
    processCrashReport();
#endif
}

void
CallbacksManager::processCrashReport()
{
#ifdef REPORTER_CLI_ONLY
    uploadFileToRepository(_dumpFilePath,"Crash auto-uploaded from NatronRenderer");
    
    ///@todo We must notify the user the log is available at filePath but we don't have access to the terminal with this process
#else
    assert(!_dialog);
    _dialog = new CrashDialog(_dumpFilePath);
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

#ifdef Q_OS_WIN32
/*Converts a std::string to wide string*/
static inline std::wstring
utf8_to_utf16(const std::string & s)
{
    
    
#ifdef Q_OS_WIN32
    std::wstring native;
    
    
    native.resize(MultiByteToWideChar (CP_UTF8, 0, s.c_str(), -1, NULL, 0) -1);
    MultiByteToWideChar (CP_UTF8, 0, s.c_str(), s.size(), &native[0], (int)native.size());
    
    return native;

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
    
} // utf8_to_utf16
#endif




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
    std::wstring wpipeName = utf8_to_utf16(pipeName);
    std::string stdDumPath = _dumpDirPath.toStdString();
    std::wstring stdWDumpPath = utf8_to_utf16(stdDumPath);
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



void
CallbacksManager::initCrashGenerationServer()
{

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
    assert(qApp);
    _dumpDirPath = QDir::tempPath();
    {
        QString tmpFileName;
#if defined(Q_OS_UNIX)
        tmpFileName	= _dumpDirPath;
        tmpFileName += '/';
        tmpFileName += NATRON_APPLICATION_NAME;
        tmpFileName += "_CRASH_PIPE_";
#endif
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
                QString toAppend("CRASH_PIPE_");
                toAppend.append(tmpFileName.mid(foundLastSlash + 1));
                _pipePath.append(toAppend);
            } else {
                _pipePath.append("CRASH_PIPE_");
                _pipePath.append(tmpFileName);
            }
        }
#endif
        
    }
    
    _comPipePath = _pipePath + "IPC_COM";
    
    _comServer = new QLocalServer;
    QObject::connect(_comServer, SIGNAL(newConnection()), this, SLOT(onComPipeConnectionPending()));
    _comServer->listen(_comPipePath);

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
