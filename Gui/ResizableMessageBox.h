//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _Gui_ResizableMessageBox_h_
#define _Gui_ResizableMessageBox_h_

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include <QMessageBox>

#include "Global/Macros.h"

// resizable message box,
// see http://www.qtcentre.org/threads/24888-Resizing-a-QMessageBox#post135851
// and http://stackoverflow.com/questions/2655354/how-to-allow-resizing-of-qmessagebox-in-pyqt4
class ResizableMessageBox : public QMessageBox {
public:
    explicit ResizableMessageBox(QWidget *parent = 0);

    ResizableMessageBox(Icon icon, const QString &title, const QString &text,
                 StandardButtons buttons = NoButton, QWidget *parent = 0,
                 Qt::WindowFlags flags = Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);

private:
    bool event(QEvent *e) OVERRIDE FINAL;
};

#endif // _Gui_ResizableMessageBox_h_
