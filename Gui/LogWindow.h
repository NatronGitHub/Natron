/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#ifndef Gui_LogWindow_h
#define Gui_LogWindow_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QDialog>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Gui/GuiFwd.h"

NATRON_NAMESPACE_ENTER

class LogWindow
    : public QWidget
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

    QVBoxLayout* _mainLayout;
    QTextBrowser* _textBrowser;
    Button* _clearButton;
    DialogButtonBox* _buttonBox;

public:

    LogWindow(QWidget* parent = 0);

    void displayLog(const std::list<LogEntry>& log);

public Q_SLOTS:

    void onClearButtonClicked();

    void onCloseButtonClicked();

private:

    virtual void keyPressEvent(QKeyEvent* e) OVERRIDE FINAL;
};

class LogWindowModal
: public QDialog
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

    QVBoxLayout* _mainLayout;
    QTextBrowser* _textBrowser;
    DialogButtonBox* _buttonBox;

public:

    LogWindowModal(const QString& log, QWidget* parent = 0);

public Q_SLOTS:

    void onCloseButtonClicked();

private:

    virtual void keyPressEvent(QKeyEvent* e) OVERRIDE FINAL;
};

NATRON_NAMESPACE_EXIT

#endif // Gui_LogWindow_h
