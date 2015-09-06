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

#ifndef CRASHDIALOG_H
#define CRASHDIALOG_H

#include <QDialog>
#include <QMutex>

class QLineEdit;
class QVBoxLayout;
class QLabel;
class QTextEdit;
class QGridLayout;
class QPushButton;
class QHBoxLayout;
class QFrame;
class QLocalSocket;
class CrashDialog : public QDialog
{
    
    Q_OBJECT
    
public:

    CrashDialog(const QString& filePath);

    virtual ~CrashDialog();
    
public slots:
    
    void onSendClicked();
    
    void onDontSendClicked();
    
    void onSaveClicked();
    
private:
    
    QString _filePath;
    
    QVBoxLayout* _mainLayout;
    QFrame* _mainFrame;
    QGridLayout* _gridLayout;
    QLabel* _iconLabel;
    QLabel* _infoLabel;
    QLabel* _refLabel;
    QLineEdit* _refContent;
    QLabel* _descLabel;
    QTextEdit* _descEdit;
    QFrame* _buttonsFrame;
    QHBoxLayout* _buttonsLayout;
    QPushButton* _sendButton;
    QPushButton* _dontSendButton;
    QPushButton* _saveReportButton;
    
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
    
#ifdef DEBUG
    QMutex _dFileMutex;
    QFile* _dFile;
#endif
    
    QLocalSocket* _outputPipe;
};

#endif // CRASHDIALOG_H
