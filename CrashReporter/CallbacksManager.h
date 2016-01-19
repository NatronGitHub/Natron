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

#ifndef CALLBACKSMANAGER_H
#define CALLBACKSMANAGER_H

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
class NetworkErrorDialog : public QDialog
{
    
    QVBoxLayout* mainLayout;
    QTextEdit* textArea;
    QDialogButtonBox* buttons;
    
public:
    
    NetworkErrorDialog(const QString& errorMessage, QWidget* parent);
    
    virtual ~NetworkErrorDialog();
    

};
#endif

class CallbacksManager : public QObject
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

    bool hasReceivedDump() const;
    
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
    
    /**
     * @brief To be called to start breakpad generation server right away
     * after the constructor
     * This throws a runtime exception upon error
     **/
    void initCrashGenerationServer();

    void createCrashGenerationServer();

    void initQApplication(int& argc, char** argv);
    
    void uploadFileToRepository(const QString& filepath, const QString& comments);

    static CallbacksManager *_instance;

    
#ifndef Q_OS_LINUX
    //On Windows & OSX breakpad expects us to manage the pipe
    QLocalServer* _breakpadPipeServer;
#endif
    
#ifndef NATRON_CRASH_REPORTER_USE_FORK
    //The Natron process has no way to print to stdout/stderr, so connect signals
    QProcess* _natronProcess;
#endif
    
    /*
     Pipe between the 2 applications to check one another if they are still alive
     */
    QLocalServer* _comServer;
    
    //owned by _comServer
    QLocalSocket* _comPipeConnection;

    QNetworkReply* _uploadReply;
    bool _dumpReceived;
#ifndef REPORTER_CLI_ONLY
    CrashDialog* _dialog;
    QProgressDialog* _progressDialog;
#endif
    QString _dumpFilePath;
    QString _dumpDirPath;
    QString _pipePath,_comPipePath;
    google_breakpad::CrashGenerationServer* _crashServer;
    
#ifdef Q_OS_LINUX
    //On Linux this is the pipe FD of crash generation server on the server side
    int _serverFD;
    int _clientFD;
#endif
    
#ifdef REPORTER_CLI_ONLY
    QCoreApplication* _app;
#else
    QApplication* _app;
#endif
    
};

#endif // CALLBACKSMANAGER_H
