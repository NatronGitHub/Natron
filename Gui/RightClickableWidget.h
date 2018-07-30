/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://natrongithub.github.io/>,
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

#ifndef Gui_RightClickableWidget_h
#define Gui_RightClickableWidget_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <map>

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QFrame>
#include <QTabWidget>
#include <QDialog>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Global/GlobalDefines.h"

#include "Engine/DockablePanelI.h"

#include "Gui/GuiFwd.h"

NATRON_NAMESPACE_ENTER

class RightClickableWidget
    : public QWidget
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    // coverity[self_assign]
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

    const DockablePanel* panel;

public:


    RightClickableWidget(const DockablePanel* panel,
                         QWidget* parent)
        : QWidget(parent)
        , panel(panel)
    {
        setObjectName( QString::fromUtf8("SettingsPanel") );
    }

    virtual ~RightClickableWidget() {}

    const DockablePanel* getPanel() const { return panel; }

    static RightClickableWidget* isParentSettingsPanelRecursive(QWidget* w);

Q_SIGNALS:

    void clicked(const QPoint& p);
    void rightClicked(const QPoint& p);
    void escapePressed();

private:

    virtual void enterEvent(QEvent* e) OVERRIDE FINAL;
    virtual void keyPressEvent(QKeyEvent* e) OVERRIDE FINAL;
    virtual void mousePressEvent(QMouseEvent* e) OVERRIDE FINAL;
};

NATRON_NAMESPACE_EXIT

#endif // Gui_RightClickableWidget_h
