//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */


#ifndef ABOUTWINDOW_H
#define ABOUTWINDOW_H

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QDialog>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

class QTextBrowser;
class QLabel;
class QTabWidget;
class QVBoxLayout;
class QHBoxLayout;
class Button;
class Gui;

class AboutWindow
    : public QDialog
{
    QVBoxLayout* _mainLayout;
    QLabel* _iconLabel;
    QTabWidget* _tabWidget;
    QTextBrowser* _aboutText;
    QTextBrowser* _libsText;
    QTextBrowser* _teamText;
    QTextBrowser* _licenseText;
    QTextBrowser* _changelogText;
    QWidget* _buttonContainer;
    QHBoxLayout* _buttonLayout;
    Button* _closeButton;
    Gui* _gui;

public:

    AboutWindow(Gui* gui,
                QWidget* parent = 0);

    void updateLibrariesVersions();

    virtual ~AboutWindow()
    {
    }
};

#endif // ABOUTWINDOW_H
