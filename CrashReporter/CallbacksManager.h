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

class QLocalSocket;
class QLocalServer;
class QNetworkReply;

//#define TRACE_CRASH_RERPORTER
#ifdef TRACE_CRASH_RERPORTER
class QTextStream;
class QFile;
#endif

#ifndef REPORTER_CLI_ONLY
class CrashDialog;
class QProgressDialog;
#endif

namespace google_breakpad {
class CrashGenerationServer;
}


class CallbacksManager : public QObject
{
    Q_OBJECT

public:

    CallbacksManager(bool autoUpload);
    ~CallbacksManager();

    void s_emitDoCallBackOnMainThread(const QString& filePath);

    static CallbacksManager* instance()
    {
        return _instance;
    }

#ifdef TRACE_CRASH_RERPORTER
    void writeDebugMessage(const QString& str);
#else
    void writeDebugMessage(const QString& /*str*/) {}
#endif

    void initOuptutPipe(const QString& comPipeName,
                        const QString& pipeName,
                        const QString& dumpPath,
                        int server_fd);

    void writeToOutputPipe(const QString& str);

public slots:

    void replyFinished();

    void onDoDumpOnMainThread(const QString& filePath);

    void onOutputPipeConnectionMade();

    void onCrashDialogFinished();
    
    void replyError(QNetworkReply::NetworkError);
    
    void onUploadProgress(qint64 bytesSent, qint64 bytesTotal);
    
    void onProgressDialogCanceled();
    
signals:

    void doDumpCallBackOnMainThread(QString);

private:

    void startCrashGenerationServer();

    void uploadFileToRepository(const QString& filepath, const QString& comments);

    static CallbacksManager *_instance;

#ifdef TRACE_CRASH_RERPORTER
    QMutex _dFileMutex;
    QFile* _dFile;
#endif

    //This is the pipe used for our own IPC between Natron & this program, mainly for the handshake on startup
    QLocalSocket* _outputPipe;

    QNetworkReply* _uploadReply;
    bool _autoUpload;
#ifndef REPORTER_CLI_ONLY
    CrashDialog* _dialog;
    QProgressDialog* _progressDialog;
#endif
    bool _didError;
    QString _dumpFilePath;
    QString _dumpDirPath;
    QString _pipePath;
    google_breakpad::CrashGenerationServer* _crashServer;
    int _serverFD;
    
};

#endif // CALLBACKSMANAGER_H
