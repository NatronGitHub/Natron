/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
 * Copyright (C) 2018-2020 The Natron developers
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
#include <QSettings>

#include <QtOpenGL/QGLWidget>
#ifdef __APPLE__
#  include <OpenGL/gl.h>
#  include <OpenGL/glext.h>
#else
#  include <GL/gl.h>
#  include <GL/glext.h>
#endif

#define NATRON_FONT "Droid Sans"
#define NATRON_FONT_SIZE_DEFAULT 11

class PlaceHolderTextEdit
    : public QTextEdit
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

    virtual void focusInEvent(QFocusEvent *e)
    {
        if ( !placeHolder.isNull() ) {
            QString t = toPlainText();
            if ( t.isEmpty() || (t == placeHolder) ) {
                clear();
            }
        }
        QTextEdit::focusInEvent(e);
    }

    virtual void focusOutEvent(QFocusEvent *e)
    {
        if ( !placeHolder.isEmpty() ) {
            if ( toPlainText().isEmpty() ) {
                setText(placeHolder);
            }
        }
        QTextEdit::focusOutEvent(e);
    }
};


CrashDialog::CrashDialog(const QString &filePath)
    : QDialog(0, Qt::Dialog | Qt::WindowStaysOnTopHint)
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
    , _featLabel(0)
    , _featMainFrame(0)
    , _featFrame01(0)
    , _featFrame02(0)
    , _featFrame03(0)
    , _featFrame04(0)
    , _featMainLayout(0)
    , _featColLayout01(0)
    , _featColLayout02(0)
    , _featColLayout03(0)
    , _featColLayout04(0)
    , _feat01Box(0)
    , _feat02Box(0)
    , _feat03Box(0)
    , _feat04Box(0)
    , _feat05Box(0)
    , _feat06Box(0)
    , _feat07Box(0)
    , _feat08Box(0)
    , _feat09Box(0)
    , _feat10Box(0)
    , _feat11Box(0)
    , _feat12Box(0)
    , _feat13Box(0)
    , _buttonsFrame(0)
    , _buttonsLayout(0)
    , _sendButton(0)
    , _dontSendButton(0)
    , _saveReportButton(0)
    , _pressedButton(0)
    , _userFrame(0)
    , _userLayout(0)
    , _userEmail(0)
    , _severity(0)
    , _severityLabel(0)
    , _contactMe(0)
{
    QFile qss( QString::fromUtf8(":/Resources/Stylesheets/mainstyle.qss") );

    if ( qss.open(QIODevice::ReadOnly
                  | QIODevice::Text) ) {
        QTextStream in(&qss);
        QString content = QString::fromUtf8("QWidget { font-family: \"%1\"; font-size: %2px; }\n").arg( QString::fromUtf8(NATRON_FONT) ).arg( QString::number(NATRON_FONT_SIZE_DEFAULT) );
        content += in.readAll();
        setStyleSheet( content
                       .arg( QString::fromUtf8("rgb(243,149,0)") )   // %1: selection-color
                       .arg( QString::fromUtf8("rgb(50,50,50)") )   // %2: medium background
                       .arg( QString::fromUtf8("rgb(71,71,71)") )   // %3: soft background
                       .arg( QString::fromUtf8("rgb(38,38,38)") )   // %4: strong background
                       .arg( QString::fromUtf8("rgb(150,150,150)") )   // %5: text colour
                       .arg( QString::fromUtf8("rgb(86,117,156)") )   // %6: interpolated value color
                       .arg( QString::fromUtf8("rgb(21,97,248)") )   // %7: keyframe value color
                       .arg( QString::fromUtf8("rgb(200,200,200)") )    // %8: disabled editable text
                       .arg( QString::fromUtf8("rgb(180,200,100)") )    // %9: expression background color
                       .arg( QString::fromUtf8("rgb(150,150,50)") )   // %10: altered text color
                       .arg( QString::fromUtf8("rgb(255,195,120)") ) );   // %11: mouse over selection color
    }

    QPalette p = qApp->palette();
    QBrush selCol( QColor(243,149,0) );
    p.setBrush( QPalette::Link, selCol );
    p.setBrush( QPalette::LinkVisited, selCol );
    qApp->setPalette( p );

    setWindowTitle( tr("Problem Report for Natron ") + QString::fromUtf8(NATRON_VERSION_STRING) );
    setAttribute(Qt::WA_DeleteOnClose, false);

    _mainLayout = new QVBoxLayout(this);

    _mainFrame = new QFrame(this);
    _mainFrame->setFrameShape(QFrame::Box);

    _gridLayout = new QGridLayout(_mainFrame);

    QPixmap pix( QString::fromUtf8(":Resources/Images/natronIcon256_linux.png") );
    if (std::max( pix.width(), pix.height() ) != 64) {
        pix = pix.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    _iconLabel = new QLabel(_mainFrame);
    _iconLabel->setPixmap(pix);
    _gridLayout->addWidget(_iconLabel, 0, 0, 1, 2, Qt::AlignHCenter | Qt::AlignVCenter);

    QString infoStr = tr("This report is for development purposes only, and you may be contacted only if you authorize it it.");
    _infoLabel = new QLabel(Qt::convertFromPlainText(infoStr, Qt::WhiteSpaceNormal), _mainFrame);

    _gridLayout->addWidget(_infoLabel, 1, 0, 1, 2);

    _refLabel = new QLabel(tr("Reference:"), _mainFrame);
    _gridLayout->addWidget(_refLabel, 2, 0, 1, 1);

    QFileInfo info(filePath);
    QString name = info.fileName();
    int lastDot = name.lastIndexOf( QLatin1Char('.') );
    if (lastDot != -1) {
        name = name.left(lastDot);
    }

    _refContent = new QLabel(name, _mainFrame);
    _refContent->setTextInteractionFlags( Qt::TextInteractionFlags( style()->styleHint(QStyle::SH_MessageBox_TextInteractionFlags, 0, this) ) );
    _gridLayout->addWidget(_refContent, 2, 1, 1, 1);

    _descLabel = new QLabel(tr("Description:"), _mainFrame);
    _gridLayout->addWidget(_descLabel, 4, 0, 1, 1);
    PlaceHolderTextEdit* edit = new PlaceHolderTextEdit(_mainFrame);
    edit->setPlaceHolderText( tr("Describe here useful info for the developers such as how to reproduce the crash and what you were doing "
                                 "prior to the crash.") );
    _descEdit = edit;
    _gridLayout->addWidget(_descEdit, 4, 1, 1, 1);

    _featLabel = new QLabel(tr("Features:"), _mainFrame);
    _featMainFrame = new QFrame(_mainFrame);
    _featFrame01 = new QFrame(_mainFrame);
    _featFrame02 = new QFrame(_mainFrame);
    _featFrame03 = new QFrame(_mainFrame);
    _featFrame04 = new QFrame(_mainFrame);
    _featMainFrame->setFrameShape(QFrame::Box);

    _feat01Box = new QCheckBox(_featMainFrame);
    _feat01Box->setText(tr("Curve Editor"));
    _feat02Box = new QCheckBox(_featFrame01);
    _feat02Box->setText(tr("Dope Sheet"));
    _feat03Box = new QCheckBox(_featFrame01);
    _feat03Box->setText(tr("GPU"));
    _feat04Box = new QCheckBox(_featFrame01);
    _feat04Box->setText(tr("GUI"));

    _feat05Box = new QCheckBox(_featFrame02);
    _feat05Box->setText(tr("Node Graph"));
    _feat06Box = new QCheckBox(_featFrame02);
    _feat06Box->setText(tr("Plugins"));
    _feat07Box = new QCheckBox(_featFrame02);
    _feat07Box->setText(tr("RotoPaint"));
    _feat08Box = new QCheckBox(_featFrame02);
    _feat08Box->setText(tr("Scripting"));

    _feat09Box = new QCheckBox(_featFrame03);
    _feat09Box->setText(tr("Tracking"));
    _feat10Box = new QCheckBox(_featFrame03);
    _feat10Box->setText(tr("Viewer"));
    _feat11Box = new QCheckBox(_featFrame03);
    _feat11Box->setText(tr("Read"));
    _feat12Box = new QCheckBox(_featFrame03);
    _feat12Box->setText(tr("Write"));

    _feat13Box = new QCheckBox(_featFrame04);
    _feat13Box->setText(tr("Other"));

    _gridLayout->addWidget(_featLabel, 3, 0, 1, 1);
    _gridLayout->addWidget(_featMainFrame, 3, 1, 1, 1);
    _featMainLayout = new QHBoxLayout(_featMainFrame);
    _featColLayout01 = new QVBoxLayout(_featFrame01);
    _featColLayout02 = new QVBoxLayout(_featFrame02);
    _featColLayout03 = new QVBoxLayout(_featFrame03);
    _featColLayout04 = new QVBoxLayout(_featFrame04);

    _featMainLayout->addWidget(_featFrame01);
    _featMainLayout->addWidget(_featFrame02);
    _featMainLayout->addWidget(_featFrame03);
    _featMainLayout->addWidget(_featFrame04);

    _featColLayout01->addWidget(_feat01Box);
    _featColLayout01->addWidget(_feat02Box);
    _featColLayout01->addWidget(_feat03Box);
    _featColLayout01->addWidget(_feat04Box);

    _featColLayout02->addWidget(_feat05Box);
    _featColLayout02->addWidget(_feat06Box);
    _featColLayout02->addWidget(_feat07Box);
    _featColLayout02->addWidget(_feat08Box);

    _featColLayout03->addWidget(_feat09Box);
    _featColLayout03->addWidget(_feat10Box);
    _featColLayout03->addWidget(_feat11Box);
    _featColLayout03->addWidget(_feat12Box);

    _featColLayout04->addWidget(_feat13Box);

    _featColLayout01->addStretch();
    _featColLayout02->addStretch();
    _featColLayout03->addStretch();
    _featColLayout04->addStretch();
    _featMainLayout->addStretch();

    _mainLayout->addWidget(_mainFrame);

    _buttonsFrame = new QFrame(this);
    _buttonsFrame->setFrameShape(QFrame::Box);
    _buttonsLayout = new QHBoxLayout(_buttonsFrame);

    _sendButton = new QPushButton(tr("Send report"), _buttonsFrame);
    _sendButton->setFocusPolicy(Qt::TabFocus);
    QObject::connect( _sendButton, SIGNAL(clicked(bool)), this, SLOT(onSendClicked()) );
    _buttonsLayout->addWidget(_sendButton);

    _dontSendButton = new QPushButton(tr("Don't send"), _buttonsFrame);
    _dontSendButton->setFocusPolicy(Qt::TabFocus);
    QObject::connect( _dontSendButton, SIGNAL(clicked(bool)), this, SLOT(onDontSendClicked()) );
    _buttonsLayout->addWidget(_dontSendButton);

    _saveReportButton = new QPushButton(tr("Save report..."), _buttonsFrame);
    _saveReportButton->setFocusPolicy(Qt::TabFocus);
    QObject::connect( _saveReportButton, SIGNAL(clicked(bool)), this, SLOT(onSaveClicked()) );
    _buttonsLayout->addWidget(_saveReportButton);

    _userFrame = new QFrame(this);
    _userFrame->setFrameShape(QFrame::Box);
    _userLayout = new QHBoxLayout(_userFrame);

    _contactMe = new QCheckBox(tr("I authorize to be contacted by email"), _userFrame);
    _contactMe->setToolTip(tr("Allow us to contact you if we have any questions regarding this issue."));
    _contactMe->setChecked(false);
    _userLayout->addWidget(_contactMe);

    _userEmail = new QLineEdit(_userFrame);
    _userEmail->setPlaceholderText(tr("Your email address ..."));
    _userEmail->setMinimumWidth(250);

    _userLayout->addWidget(_userEmail);

    _severity = new QComboBox(_userFrame);
    _severityLabel = new QLabel(tr("Severity:"), _userFrame);
    _severityLabel->setToolTip(tr("Set a severity on this issue."));

    _severity->addItem(QString::fromUtf8("Low"));
    _severity->addItem(QString::fromUtf8("Normal"));
    _severity->addItem(QString::fromUtf8("High"));

    _userLayout->addWidget(_severityLabel);
    _userLayout->addWidget(_severity);

    _mainLayout->addWidget(_userFrame);
    _mainLayout->addWidget(_buttonsFrame);

    _sendButton->setFocus();

    QGLWidget GLwidget;
    if ( GLwidget.isValid() ) {
        GLwidget.makeCurrent();
        _GLrenderer = QString::fromUtf8( (char *) glGetString(GL_RENDERER) );
        _GLversion = QString::fromUtf8( (char *) glGetString(GL_VERSION) );
        _GLvendor = QString::fromUtf8( (char *) glGetString(GL_VENDOR) );
        _GLshader = QString::fromUtf8( (char *) glGetString(GL_SHADING_LANGUAGE_VERSION) );
        _GLext = QString::fromUtf8( (char *) glGetString(GL_EXTENSIONS) );
    }

    restoreSettings();
}

CrashDialog::~CrashDialog()
{
}

QString
CrashDialog::getDescription() const
{
    return _descEdit->getText();
}

CrashDialog::UserChoice
CrashDialog::getUserChoice() const
{
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

QString CrashDialog::getContact() const
{
    QString email;
    if (_contactMe->isChecked()) {
        email = _userEmail->text();
    }
    return email;
}

QString CrashDialog::getSeverity() const
{
    return _severity->currentText();
}

QString CrashDialog::getFeatures() const
{
    QString features;

    if (_feat01Box->isChecked()) {
        if (!features.isEmpty()) {
            features.append(QString::fromUtf8(", "));
        }
        features.append(_feat01Box->text());
    }
    if (_feat02Box->isChecked()) {
        if (!features.isEmpty()) {
            features.append(QString::fromUtf8(", "));
        }
        features.append(_feat02Box->text());
    }
    if (_feat03Box->isChecked()) {
        if (!features.isEmpty()) {
            features.append(QString::fromUtf8(", "));
        }
        features.append(_feat03Box->text());
    }
    if (_feat04Box->isChecked()) {
        if (!features.isEmpty()) {
            features.append(QString::fromUtf8(", "));
        }
        features.append(_feat04Box->text());
    }
    if (_feat05Box->isChecked()) {
        if (!features.isEmpty()) {
            features.append(QString::fromUtf8(", "));
        }
        features.append(_feat05Box->text());
    }
    if (_feat06Box->isChecked()) {
        if (!features.isEmpty()) {
            features.append(QString::fromUtf8(", "));
        }
        features.append(_feat06Box->text());
    }
    if (_feat07Box->isChecked()) {
        if (!features.isEmpty()) {
            features.append(QString::fromUtf8(", "));
        }
        features.append(_feat07Box->text());
    }
    if (_feat08Box->isChecked()) {
        if (!features.isEmpty()) {
            features.append(QString::fromUtf8(", "));
        }
        features.append(_feat08Box->text());
    }
    if (_feat09Box->isChecked()) {
        if (!features.isEmpty()) {
            features.append(QString::fromUtf8(", "));
        }
        features.append(_feat09Box->text());
    }
    if (_feat10Box->isChecked()) {
        if (!features.isEmpty()) {
            features.append(QString::fromUtf8(", "));
        }
        features.append(_feat10Box->text());
    }
    if (_feat11Box->isChecked()) {
        if (!features.isEmpty()) {
            features.append(QString::fromUtf8(", "));
        }
        features.append(_feat11Box->text());
    }
    if (_feat12Box->isChecked()) {
        if (!features.isEmpty()) {
            features.append(QString::fromUtf8(", "));
        }
        features.append(_feat12Box->text());
    }
    if (_feat13Box->isChecked()) {
        if (!features.isEmpty()) {
            features.append(QString::fromUtf8(", "));
        }
        features.append(_feat13Box->text());
    }

    return features;
}

QString CrashDialog::getGLrenderer() const
{
    return _GLrenderer;
}

QString CrashDialog::getGLversion() const
{
    return _GLversion;
}

QString CrashDialog::getGLvendor() const
{
    return _GLvendor;
}

QString CrashDialog::getGLshader() const
{
    return _GLshader;
}

QString CrashDialog::getGLext() const
{
    return _GLext;
}

void
CrashDialog::onSendClicked()
{
    saveSettings();
    QString description = _descEdit->toPlainText();

    if ( description.isEmpty() ) {
        QMessageBox ques(QMessageBox::Question, tr("Empty description"), tr("The issue report doesn't have any description. "
                                                                            "Would you like to send it anyway?"),
                         QMessageBox::Yes | QMessageBox::No,
                         this, Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowStaysOnTopHint);
        ques.setDefaultButton(QMessageBox::No);
        ques.setWindowFlags(ques.windowFlags() | Qt::WindowStaysOnTopHint);
        if ( ques.exec() ) {
            QMessageBox::StandardButton rep = ques.standardButton( ques.clickedButton() );
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
    saveSettings();
    _pressedButton = _dontSendButton;
    reject();
}

void
CrashDialog::onSaveClicked()
{
    saveSettings();
    _pressedButton = _saveReportButton;

    QString fileName = _filePath;
    fileName.replace( QLatin1Char('\\'), QLatin1Char('/') );
    int foundLastSep = fileName.lastIndexOf( QLatin1Char('/') );
    if (foundLastSep != -1) {
        fileName = fileName.mid(foundLastSep + 1);
    }

    fileName = QDir::homePath() + QLatin1Char('/') + fileName;

    bool saveOk = false;
    QString saveFileName = QFileDialog::getSaveFileName(this,
                                                        tr("Save report"),
                                                        fileName,
                                                        QString::fromUtf8("*.dmp"),
                                                        0,
                                                        0);
    if ( !saveFileName.isEmpty() ) {
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

void CrashDialog::restoreSettings()
{
    QSettings settings( QString::fromUtf8(NATRON_ORGANIZATION_NAME), QString::fromUtf8(NATRON_APPLICATION_NAME) );
    settings.beginGroup( QString::fromUtf8("CrashReporter") );

    QString userEmail = settings.value( QString::fromUtf8("email") ).toString();
    if ( !userEmail.isEmpty() ) {
        _userEmail->setText(userEmail);
    }

    bool contactMe = settings.value( QString::fromUtf8("contactme") ).toBool();
    _contactMe->setChecked(contactMe);

    settings.endGroup();
}

void CrashDialog::saveSettings()
{
        QSettings settings( QString::fromUtf8(NATRON_ORGANIZATION_NAME), QString::fromUtf8(NATRON_APPLICATION_NAME) );
        settings.beginGroup( QString::fromUtf8("CrashReporter") );

        if ( !_userEmail->text().isEmpty() ) {
            QString savedEmail = settings.value( QString::fromUtf8("email") ).toString();
            if ( savedEmail != _userEmail->text() ) {
                settings.setValue( QString::fromUtf8("email"), _userEmail->text() );
            }
        }

        bool contactMe = settings.value( QString::fromUtf8("contactme") ).toBool();
        if ( contactMe != _contactMe->isChecked() ) {
            settings.setValue( QString::fromUtf8("contactme"), _contactMe->isChecked() );
        }

        settings.endGroup();
        settings.sync();
}

