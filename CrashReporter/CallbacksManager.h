/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
 * Copyright (C) 2018-2021 The Natron developers
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

#ifndef CALLBACKSMANAGER_H
#define CALLBACKSMANAGER_H

#include <vector>
#include <QMutex>
#include <QObject>
#include <QNetworkReply>
#include <QProcess>

#ifndef REPORTER_CLI_ONLY
#include <QDialog>
#endif

#include "Global/Macros.h"

class QLocalSocket;
class QLocalServer;
class QNetworkReply;

class QString;

#ifndef REPORTER_CLI_ONLY
class CrashDialog;
class QProgressDialog;
class QVBoxLayout;
class QTextEdit;
class QDialogButtonBox;
class QApplication;
#else
class QCoreApplication;
#endif

namespace google_breakpad {
class CrashGenerationServer;
}


#ifndef REPORTER_CLI_ONLY
class NetworkErrorDialog
    : public QDialog
{
    QVBoxLayout* mainLayout;
    QTextEdit* textArea;
    QDialogButtonBox* buttons;

public:

    NetworkErrorDialog(const QString& errorMessage, QWidget* parent);

    virtual ~NetworkErrorDialog();
};

#endif

class CallbacksManager
    : public QObject
{
    Q_OBJECT

public:

    CallbacksManager();

    virtual ~CallbacksManager();


    static CallbacksManager* instance()
    {
        return _instance;
    }

    /**
     * @brief Creates the crash generation server and spawns the actual Natron proces.
     * This function throws an exception in case of error.
     **/
    void init(int& argc, char** argv);
    void initW(int& argc, wchar_t** argv);

private:

    void initInternal();

public:


    bool hasReceivedDump() const;

    bool hasInit() const;

    void s_emitDoDumpCallBackOnMainThread(const QString& filePath);
    void s_emitDoExitCallBackOnMainThread(int exitCode, bool exitEvenIfDumpedReceived);

    int exec();

public Q_SLOTS:

    void replyFinished(QNetworkReply* reply);

    void onDoDumpOnMainThread(const QString& filePath);

    void onDoExitOnMainThread(int exitCode, bool exitEvenIfDumpedReceived);

    void onCrashDialogFinished();

    void onUploadProgress(qint64 bytesSent, qint64 bytesTotal);

    void onProgressDialogCanceled();

    void onNatronProcessStdOutWrittenTo();
    void onNatronProcessStdErrWrittenTo();

    void onComPipeConnectionPending();

    void onComPipeDataWrittenTo();

    void onSpawnedProcessFinished(int exitCode, QProcess::ExitStatus status);
    void onSpawnedProcessError(QProcess::ProcessError error);

Q_SIGNALS:

    void doExitCallbackOnMainThread(int exitCode, bool exitEvenIfDumpedReceived);

    void doDumpCallBackOnMainThread(QString);

private:

    void processCrashReport();

    /**
     * @brief To be called to start breakpad generation server right away
     * after the constructor
     * This throws a runtime exception upon error
     **/
    void initCrashGenerationServer();

    void createCrashGenerationServer();

    void uploadFileToRepository(const QString& filepath, const QString& comments, const QString& contact, const QString& severity, const QString& GLrendererInfo, const QString& GLversionInfo, const QString& GLvendorInfo, const QString& GLshaderInfo, const QString& GLextInfo, const QString &features);

    static CallbacksManager *_instance;


#ifndef NATRON_CRASH_REPORTER_USE_FORK
    //The Natron process has no way to print to stdout/stderr, so connect signals
    //owned by us
    QProcess* _natronProcess;
#endif

    /*
       Pipe between the 2 applications to check one another if they are still alive
       Owned by us
     */
    QLocalServer* _comServer;

    //owned by _comServer
    QLocalSocket* _comPipeConnection;

    //Referenc
    QNetworkReply* _uploadReply;
    bool _dumpReceived;
    bool _mustInitQAppAfterDump;
#ifndef REPORTER_CLI_ONLY
    CrashDialog* _dialog;
    QProgressDialog* _progressDialog;
#endif
    QString _dumpFilePath;
    QString _dumpDirPath;
    QString _pipePath, _comPipePath;

    //owned by us
    google_breakpad::CrashGenerationServer* _crashServer;

#ifdef Q_OS_LINUX
    //On Linux this is the pipe FD of crash generation server on the server side
    int _serverFD;
    int _clientFD;
#endif

    bool _initErr;
    int _argc;
    std::vector<char*> _argv;
};

#endif // CALLBACKSMANAGER_H
