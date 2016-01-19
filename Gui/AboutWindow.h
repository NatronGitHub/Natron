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

#ifndef ABOUTWINDOW_H
#define ABOUTWINDOW_H

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

NATRON_NAMESPACE_ENTER;

class AboutWindow
    : public QDialog
{
    Q_OBJECT
    
    QVBoxLayout* _mainLayout;
    Natron::Label* _iconLabel;
    QTabWidget* _tabWidget;
    QTextBrowser* _aboutText;
    QTextBrowser* _libsText;
    QTextBrowser* _teamText;
    QTextBrowser* _licenseText;
    QTextBrowser* _changelogText;
    QTextBrowser* _thirdPartyBrowser;
    QWidget* _buttonContainer;
    QHBoxLayout* _buttonLayout;
    Button* _closeButton;
    Gui* _gui;
    TableModel* _model;
    TableView* _view;

public:

    AboutWindow(Gui* gui,
                QWidget* parent = 0);

    void updateLibrariesVersions();

    virtual ~AboutWindow()
    {
    }
    
public Q_SLOTS:
    
    void onSelectionChanged(const QItemSelection & newSelection,
                            const QItemSelection & oldSelection);
};

NATRON_NAMESPACE_EXIT;

#endif // ABOUTWINDOW_H
