/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier
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

#include "CrashDialog.h"
#include <iostream>
#include <cassert>
#include <QThread>
#include <QTextStream>
#include <QFile>
#include <QFrame>
#include <QLabel>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QTextEdit>
#include <QFileInfo>
#include <QPainter>
#include <QApplication>
#include <QLocalSocket>
#include <QFileDialog>
#include <QTextDocument>

CallbacksManager* CallbacksManager::_instance = 0;

class PlaceHolderTextEdit: public QTextEdit
{
    QString placeHolder;
public:
    
    
    PlaceHolderTextEdit(QWidget* parent)
    : QTextEdit(parent)
    , placeHolder()
    {
        
    }
    
    void setPlaceHolderText(const QString& ph)
    {
        placeHolder = ph;
    }
    
private:
    
    virtual void focusInEvent(QFocusEvent *e){
        if (!placeHolder.isNull()){
            QString t = toPlainText();
            if (t.isEmpty() || t == placeHolder) {
                clear();
            }
        }
        QTextEdit::focusInEvent(e);
    }
    
    virtual void focusOutEvent(QFocusEvent *e) {
        if (!placeHolder.isEmpty()){
            if (toPlainText().isEmpty()) {
                setText(placeHolder);
            }
        }
        QTextEdit::focusOutEvent(e);
    }
    
};


CrashDialog::CrashDialog(const QString &filePath)
: QDialog(0,Qt::Dialog | Qt::WindowStaysOnTopHint)
, _filePath(filePath)
, _mainLayout(0)
, _mainFrame(0)
, _gridLayout(0)
, _iconLabel(0)
, _infoLabel(0)
, _refLabel(0)
, _refContent(0)
, _descLabel(0)
, _descEdit(0)
, _buttonsFrame(0)
, _buttonsLayout(0)
, _sendButton(0)
, _dontSendButton(0)
, _saveReportButton(0)
{
    QFile qss(":/Resources/Stylesheets/mainstyle.qss");

        if ( qss.open(QIODevice::ReadOnly
                      | QIODevice::Text) ) {
            QTextStream in(&qss);
            QString content( in.readAll() );
            setStyleSheet( content
                           .arg("rgb(243,149,0)") // %1: selection-color
                           .arg("rgb(50,50,50)") // %2: medium background
                           .arg("rgb(71,71,71)") // %3: soft background
                           .arg("rgb(38,38,38)") // %4: strong background
                           .arg("rgb(200,200,200)") // %5: text colour
                           .arg("rgb(86,117,156)") // %6: interpolated value color
                           .arg("rgb(21,97,248)") // %7: keyframe value color
                           .arg("rgb(0,0,0)")  // %8: disabled editable text
                           .arg("rgb(180, 200, 100)") ); // %9: expression background color
        }
    
    setWindowTitle(tr("Natron Issue Reporter"));
    
    _mainLayout = new QVBoxLayout(this);
    
    _mainFrame = new QFrame(this);
    _mainFrame->setFrameShape(QFrame::Box);
    
    _gridLayout = new QGridLayout(_mainFrame);
    
    QPixmap pix(":Resources/Images/natronIcon256_linux.png");
    pix = pix.scaled(64, 64);

    _iconLabel = new QLabel(_mainFrame);
    _iconLabel->setPixmap(pix);
    _gridLayout->addWidget(_iconLabel, 0, 0, 1, 2,Qt::AlignHCenter | Qt::AlignVCenter);
    
    QString infoStr = tr("Unfortunately Natron crashed.\n This report is for development purposes only and can be sent to the developers team "
                         "to help track down the bug.\n You will not be contacted nor identified by the process of sending the report.");
    _infoLabel = new QLabel(Qt::convertFromPlainText(infoStr, Qt::WhiteSpaceNormal), _mainFrame);
    
    _gridLayout->addWidget(_infoLabel, 1, 0, 1 , 2);
    
    _refLabel = new QLabel("Reference:",_mainFrame);
    _gridLayout->addWidget(_refLabel, 2, 0 , 1, 1);
    
    QFileInfo info(filePath);
    QString name = info.fileName();
    int lastDot = name.lastIndexOf('.');
    if (lastDot != -1) {
        name = name.left(lastDot);
    }
    
    _refContent = new QLabel(name,_mainFrame);
    _gridLayout->addWidget(_refContent,2 , 1, 1, 1);
    
    _descLabel = new QLabel("Description:",_mainFrame);
    _gridLayout->addWidget(_descLabel, 3, 0 , 1, 1);
    PlaceHolderTextEdit* edit = new PlaceHolderTextEdit(_mainFrame);
    edit->setPlaceHolderText(tr("Describe here useful info for the developers such as how to reproduce the crash and what you were doing "
                                "prior to the crash."));
    _descEdit = edit;
    _gridLayout->addWidget(_descEdit, 3, 1 ,1, 1);
    
    _mainLayout->addWidget(_mainFrame);
    
    _buttonsFrame = new QFrame(this);
    _buttonsFrame->setFrameShape(QFrame::Box);
    _buttonsLayout = new QHBoxLayout(_buttonsFrame);
    
    _sendButton = new QPushButton(tr("Send report"),_buttonsFrame);
    QObject::connect(_sendButton, SIGNAL(clicked(bool)), this, SLOT(onSendClicked()));
    _buttonsLayout->addWidget(_sendButton);
    
    _dontSendButton = new QPushButton(tr("Don't send"),_buttonsFrame);
    QObject::connect(_dontSendButton, SIGNAL(clicked(bool)), this, SLOT(onDontSendClicked()));
    _buttonsLayout->addWidget(_dontSendButton);
    
    _saveReportButton = new QPushButton(tr("Save report..."),_buttonsFrame);
    QObject::connect(_saveReportButton, SIGNAL(clicked(bool)), this, SLOT(onSaveClicked()));
    _buttonsLayout->addWidget(_saveReportButton);
    
    _mainLayout->addWidget(_buttonsFrame);
    
}

CrashDialog::~CrashDialog()
{

}

void
CrashDialog::onSendClicked()
{
    
}

void
CrashDialog::onDontSendClicked()
{
    reject();
}

void
CrashDialog::onSaveClicked()
{
    QString filename = QFileDialog::getSaveFileName(this,
                                                    tr("Save report"),
                                                    QString(),
                                                    QString(),
                                                    0,
                                                    0);
    if (!filename.isEmpty()) {
        QFile::copy(_filePath, filename);
        accept();
    }
}

CallbacksManager::CallbacksManager()
    : QObject()
#ifdef DEBUG
    , _dFileMutex()
    , _dFile(0)
#endif
    , _outputPipe(0)
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


void
CallbacksManager::onDoDumpOnMainThread(const QString& filePath)
{
    assert(QThread::currentThread() == qApp->thread());
    CrashDialog d(filePath);
    if (d.exec()) {

    } else {

    }

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
