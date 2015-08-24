//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "LogWindow.h"

#include <cmath>

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QVBoxLayout>
#include <QTextBrowser>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Gui/Button.h"
#include "Gui/GuiApplicationManager.h" // appPTR

LogWindow::LogWindow(const QString & log,
                     QWidget* parent)
    : QDialog(parent)
{
    mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    textBrowser = new QTextBrowser(this);
    textBrowser->setOpenExternalLinks(true);
    textBrowser->setText(log);

    mainLayout->addWidget(textBrowser);

    QWidget* buttonsContainer = new QWidget(this);
    QHBoxLayout* buttonsLayout = new QHBoxLayout(buttonsContainer);
    
    clearButton = new Button(tr("Clear"),buttonsContainer);
    buttonsLayout->addWidget(clearButton);
    QObject::connect(clearButton, SIGNAL(clicked()), this, SLOT(onClearButtonClicked()));
    buttonsLayout->addStretch();
    okButton = new Button(tr("Ok"),buttonsContainer);
    buttonsLayout->addWidget(okButton);
    QObject::connect( okButton, SIGNAL( clicked() ), this, SLOT( accept() ) );
    mainLayout->addWidget(buttonsContainer);
}

void
LogWindow::onClearButtonClicked()
{
    appPTR->clearOfxLog_mt_safe();
    textBrowser->clear();
}
