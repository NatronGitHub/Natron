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

#ifndef CALLBACKSMANAGER_H
#define CALLBACKSMANAGER_H

#include <QMutex>
#include <QObject>

class QLocalSocket;
class QNetworkReply;

#ifdef DEBUG
class QTextStream;
class QFile;
#endif

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

#ifdef DEBUG
    void writeDebugMessage(const QString& str);
#else
    void writeDebugMessage(const QString& /*str*/) {}
#endif

    void initOuptutPipe(const QString& comPipeName);

    void writeToOutputPipe(const QString& str);

public slots:

    void replyFinished(QNetworkReply* reply);

    void onDoDumpOnMainThread(const QString& filePath);

    void onOutputPipeConnectionMade();

signals:

    void doDumpCallBackOnMainThread(QString);

private:

    void uploadFileToRepository(const QString& str);

    static CallbacksManager *_instance;

#ifdef DEBUG
    QMutex _dFileMutex;
    QFile* _dFile;
#endif

    QLocalSocket* _outputPipe;
    bool _autoUpload;
};

#endif // CALLBACKSMANAGER_H
