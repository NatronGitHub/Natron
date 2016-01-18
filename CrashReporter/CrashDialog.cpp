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

#include "CrashDialog.h"

#include <algorithm>
#include <iostream>
#include <cassert>

#include <QDir>
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
#include <QMessageBox>
#include <QStyle>



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
    
    QString getText() const
    {
        QString ret = toPlainText();
        if (ret == placeHolder) {
            return QString();
        }
        return ret;
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
, _pressedButton(0)
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
                           .arg("rgb(150,150,150)") // %5: text colour
                           .arg("rgb(86,117,156)") // %6: interpolated value color
                           .arg("rgb(21,97,248)") // %7: keyframe value color
                           .arg("rgb(200,200,200)")  // %8: disabled editable text
                           .arg("rgb(180,200,100)")  // %9: expression background color
                           .arg("rgb(150,150,50)") // %10: altered text color
                           .arg("rgb(255,195,120)"));  // %11: mouse over selection color
        }
    
    setWindowTitle(tr("Natron Issue Reporter"));
    setAttribute(Qt::WA_DeleteOnClose, false);
    
    _mainLayout = new QVBoxLayout(this);
    
    _mainFrame = new QFrame(this);
    _mainFrame->setFrameShape(QFrame::Box);
    
    _gridLayout = new QGridLayout(_mainFrame);
    
    QPixmap pix(":Resources/Images/natronIcon256_linux.png");
    if (std::max(pix.width(), pix.height()) != 64) {
        pix = pix.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

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
    _refContent->setTextInteractionFlags(Qt::TextInteractionFlags(style()->styleHint(QStyle::SH_MessageBox_TextInteractionFlags, 0, this)));
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
    _sendButton->setFocusPolicy(Qt::TabFocus);
    QObject::connect(_sendButton, SIGNAL(clicked(bool)), this, SLOT(onSendClicked()));
    _buttonsLayout->addWidget(_sendButton);
    
    _dontSendButton = new QPushButton(tr("Don't send"),_buttonsFrame);
    _dontSendButton->setFocusPolicy(Qt::TabFocus);
    QObject::connect(_dontSendButton, SIGNAL(clicked(bool)), this, SLOT(onDontSendClicked()));
    _buttonsLayout->addWidget(_dontSendButton);
    
    _saveReportButton = new QPushButton(tr("Save report..."),_buttonsFrame);
    _saveReportButton->setFocusPolicy(Qt::TabFocus);
    QObject::connect(_saveReportButton, SIGNAL(clicked(bool)), this, SLOT(onSaveClicked()));
    _buttonsLayout->addWidget(_saveReportButton);
    
    _mainLayout->addWidget(_buttonsFrame);
    
    _sendButton->setFocus();
    
}

CrashDialog::~CrashDialog()
{

}

QString CrashDialog::getDescription() const {
    return _descEdit->getText();
}

CrashDialog::UserChoice
CrashDialog::getUserChoice() const {
    if (_pressedButton == _sendButton) {
        return eUserChoiceUpload;
    } else if (_pressedButton == _dontSendButton) {
        return eUserChoiceIgnore;
    } else if (_pressedButton == _saveReportButton) {
        return eUserChoiceSave;
    } else {
        return eUserChoiceIgnore;
    }
}


void
CrashDialog::onSendClicked()
{
    QString description = _descEdit->toPlainText();
    if (description.isEmpty()) {
        QMessageBox ques(QMessageBox::Question, tr("Empty description"), tr("The issue report doesn't have any description. "
                                                                            "Would you like to send it anyway?"),
                         QMessageBox::Yes | QMessageBox::No,
                         this, Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowStaysOnTopHint);
        ques.setDefaultButton(QMessageBox::No);
        ques.setWindowFlags(ques.windowFlags() | Qt::WindowStaysOnTopHint);
        if ( ques.exec() ) {
            QMessageBox::StandardButton rep = ques.standardButton(ques.clickedButton());
            if (rep != QMessageBox::Yes) {
                return;
            }
        }
    }
    _pressedButton = _sendButton;
    accept();
}

void
CrashDialog::onDontSendClicked()
{
    _pressedButton = _dontSendButton;
    reject();
}

void
CrashDialog::onSaveClicked()
{
    _pressedButton = _saveReportButton;

    QString fileName = _filePath;
    fileName.replace('\\', '/');
    int foundLastSep = fileName.lastIndexOf('/');
    if (foundLastSep != -1) {
        fileName = fileName.mid(foundLastSep + 1);
    }
    
    fileName = QDir::homePath() + '/' + fileName;
    
    bool saveOk = false;
    QString saveFileName = QFileDialog::getSaveFileName(this,
                                            tr("Save report"),
                                            fileName,
                                            QString("*.dmp"),
                                            0,
                                            0);
    if (!saveFileName.isEmpty()) {
        saveOk = QFile::copy(_filePath, saveFileName);
        
    } else {
        return;
    }
    
    if (!saveOk) {
        QMessageBox ques(QMessageBox::Critical, tr("Invalid filename"), tr("The issue could not be saved to: ") + saveFileName,
                         QMessageBox::Ok,
                         this, Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowStaysOnTopHint);
        ques.setWindowFlags(ques.windowFlags() | Qt::WindowStaysOnTopHint);
        ques.exec();
        return;
    }
    
    accept();

}
