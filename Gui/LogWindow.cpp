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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "LogWindow.h"

#include <cmath>
#include <stdexcept>

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QDateTime>
#include <QtCore/QLocale>
#include <QVBoxLayout>
#include <QScrollBar>
#include <QTextBrowser>
#include <QKeyEvent>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/Image.h"
#include "Engine/Lut.h" // floatToInt
#include "Gui/Button.h"
#include "Gui/DialogButtonBox.h"
#include "Gui/GuiApplicationManager.h" // appPTR

NATRON_NAMESPACE_ENTER;

LogWindow::LogWindow(QWidget* parent)
    : QWidget(parent, Qt::Dialog | Qt::WindowStaysOnTopHint)
{
    setWindowTitle( tr("Error Log") );

    mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    textBrowser = new QTextBrowser(this);
    textBrowser->setOpenExternalLinks(true);
    mainLayout->addWidget(textBrowser);

    DialogButtonBox* buttonBox = new DialogButtonBox(QDialogButtonBox::StandardButtons(QDialogButtonBox::Cancel), Qt::Horizontal, this);

    clearButton = new Button( tr("&Clear") );
    clearButton->setFocusPolicy(Qt::TabFocus);
    buttonBox->addButton(clearButton, QDialogButtonBox::ResetRole);
    QObject::connect( clearButton, SIGNAL(clicked()), this, SLOT(onClearButtonClicked()) );
    QObject::connect(buttonBox, SIGNAL(rejected()), this, SLOT(onOkButtonClicked()));
    mainLayout->addWidget(buttonBox);
}
static void
replaceLineBreaksWithHtmlParagraph(QString &txt)
{
    txt.replace( QString::fromUtf8("\n"), QString::fromUtf8("<br />") );
}

void
LogWindow::displayLog(const std::list<LogEntry>& log)
{
    if (log.empty()) {
        textBrowser->clear();
        return;
    }
    QString content;
    std::list<LogEntry>::const_iterator next = log.begin();
    ++next;

    for (std::list<LogEntry>::const_iterator it = log.begin(); it!=log.end(); ++it) {
        int r = Color::floatToInt<256>(it->color.r);
        int g = Color::floatToInt<256>(it->color.g);
        int b = Color::floatToInt<256>(it->color.b);
        QColor c(r,g,b);
        // only print time - QTime.toString() uses the system locale, that's not what we want
        content.append( QString::fromUtf8("[%3] <font color=%1><b>%2</b></font>: ")
                        .arg( c.name() )
                        .arg(it->context)
                       .arg( QLocale().toString( it->date.time(), QString::fromUtf8("HH:mm:ss.zzz") ) ) );
        if (it->isHtml) {
            content.append(it->message);
        } else {
            QString m(it->message);
            replaceLineBreaksWithHtmlParagraph(m);
            content.append(m);
        }
        if (next != log.end()) {
            content.append(QString::fromUtf8("<br /><br />"));
            ++next;
        }
    }
    textBrowser->setHtml(content);
    okButton->setFocus();
    textBrowser->verticalScrollBar()->setValue( textBrowser->verticalScrollBar()->maximum() );
}


void
LogWindow::onClearButtonClicked()
{
    appPTR->clearErrorLog_mt_safe();
    textBrowser->clear();
}

void
LogWindow::onOkButtonClicked()
{
    hide();
}

void
LogWindow::keyPressEvent(QKeyEvent* e)
{
    Qt::Key k = (Qt::Key)e->key();
    if (k == Qt::Key_Return || k == Qt::Key_Enter || k == Qt::Key_Escape) {
        hide();
    } else {
        QWidget::keyPressEvent(e);
    }
}

LogWindowModal::LogWindowModal(const QString& log, QWidget* parent)
: QDialog(parent)
{

    mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    textBrowser = new QTextBrowser(this);
    textBrowser->setOpenExternalLinks(true);
    textBrowser->setPlainText(log);
    mainLayout->addWidget(textBrowser);

    QWidget* buttonsContainer = new QWidget(this);
    QHBoxLayout* buttonsLayout = new QHBoxLayout(buttonsContainer);
    buttonsLayout->addStretch();
    okButton = new Button(tr("Ok"), buttonsContainer);
    okButton->setFocusPolicy(Qt::TabFocus);
    buttonsLayout->addWidget(okButton);
    buttonsLayout->addStretch();
    QObject::connect( okButton, SIGNAL(clicked()), this, SLOT(onOkButtonClicked()) );
    mainLayout->addWidget(buttonsContainer);
}

void
LogWindowModal::onOkButtonClicked()
{
    hide();
}

void
LogWindowModal::keyPressEvent(QKeyEvent* e)
{
    Qt::Key k = (Qt::Key)e->key();
    if (k == Qt::Key_Return || k == Qt::Key_Enter || k == Qt::Key_Escape) {
        hide();
    } else {
        QDialog::keyPressEvent(e);
    }
}
NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_LogWindow.cpp"
