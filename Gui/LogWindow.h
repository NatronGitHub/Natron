//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _Gui_LogWindow_h_
#define _Gui_LogWindow_h_

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "Global/Macros.h"

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QDialog>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)
class QVBoxLayout;
class QTextBrowser;
class Button;
class QString;

class LogWindow
    : public QDialog
{
    Q_OBJECT

    QVBoxLayout* mainLayout;
    QTextBrowser* textBrowser;
    Button* okButton;
    Button* clearButton;
public:

    LogWindow(const QString & log,
              QWidget* parent = 0);
    
public Q_SLOTS:
    
    void onClearButtonClicked();
};

#endif // _Gui_LogWindow_h_
