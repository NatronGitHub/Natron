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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "ResizableMessageBox.h"

#include <QEvent>
#include <QTextEdit>

// resizable message box,
// see http://www.qtcentre.org/threads/24888-Resizing-a-QMessageBox#post135851
// and http://stackoverflow.com/questions/2655354/how-to-allow-resizing-of-qmessagebox-in-pyqt4
ResizableMessageBox::ResizableMessageBox(QWidget *parent)
: QMessageBox(parent)
{
    setSizeGripEnabled(true);
}

ResizableMessageBox::ResizableMessageBox(Icon icon, const QString &title, const QString &text,
                           StandardButtons buttons, QWidget *parent,
                           Qt::WindowFlags flags)
: QMessageBox(icon, title, text, buttons, parent, flags)
{
    setSizeGripEnabled(true);
}

bool
ResizableMessageBox::event(QEvent *e)
{
    bool result = QMessageBox::event(e);

    //QMessageBox::event in this case will call setFixedSize on the dialog frame, making it not resizable by the user
    if (e->type() == QEvent::LayoutRequest || e->type() == QEvent::Resize) {
        setMinimumHeight(0);
        setMaximumHeight(QWIDGETSIZE_MAX);
        setMinimumWidth(0);
        setMaximumWidth(QWIDGETSIZE_MAX);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        // make the detailed text expanding
        QTextEdit *textEdit = findChild<QTextEdit *>();

        if (textEdit) {
            textEdit->setMinimumHeight(0);
            textEdit->setMaximumHeight(QWIDGETSIZE_MAX);
            textEdit->setMinimumWidth(0);
            textEdit->setMaximumWidth(QWIDGETSIZE_MAX);
            textEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        }


    }
    return result;
}


