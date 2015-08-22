//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */


// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

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


