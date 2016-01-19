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

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QVBoxLayout>
#include <QTextBrowser>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Gui/Button.h"
#include "Gui/GuiApplicationManager.h" // appPTR

NATRON_NAMESPACE_USING

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

#include "moc_LogWindow.cpp"
