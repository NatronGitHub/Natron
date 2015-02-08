//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#ifndef CRASHDIALOG_H
#define CRASHDIALOG_H

#include <QDialog>
#include <QWaitCondition>
#include <QMutex>


class QLocalSocket;
class CrashDialog : public QDialog
{
public:

    CrashDialog(const QString& filePath);

    virtual ~CrashDialog();
};


#ifdef DEBUG
class QTextStream;
class QFile;
#endif

class CallbacksManager : public QObject
{
    Q_OBJECT

public:

    CallbacksManager();
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

    void onDoDumpOnMainThread(const QString& filePath);
    
    void onOutputPipeConnectionMade();

signals:

    void doDumpCallBackOnMainThread(QString);

private:

    static CallbacksManager *_instance;
    
    QMutex _doingDialogMutex;
    QWaitCondition _doingDialogCond;
    bool _doingDialog;
#ifdef DEBUG
    QMutex _dFileMutex;
    QFile* _dFile;
#endif
    
    QLocalSocket* _outputPipe;
};

#endif // CRASHDIALOG_H
