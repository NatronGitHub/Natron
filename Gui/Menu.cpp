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

#include "Menu.h"

#include "Global/Macros.h"
#include "Gui/GuiApplicationManager.h"
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QToolTip>
#include <QApplication>
GCC_DIAG_UNUSED_PRIVATE_FIELD_OFF
// /opt/local/include/QtGui/qmime.h:119:10: warning: private field 'type' is not used [-Wunused-private-field]
#include <QHelpEvent>
GCC_DIAG_UNUSED_PRIVATE_FIELD_ON
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

using namespace Natron;

Menu::Menu(const QString &title, QWidget *parent)
: QMenu(title,parent)
{
    setFont(QApplication::font()); // necessary, or the labels will get the default font size
}

Menu::Menu(QWidget* parent)
: QMenu(parent)
{
    setFont(QApplication::font()); // necessary, or the labels will get the default font size
}


MenuWithToolTips::MenuWithToolTips(QWidget* parent)
    : Menu(parent)
{
    
}

bool
MenuWithToolTips::event (QEvent* e)
{
    if (e->type() == QEvent::ToolTip) {
        const QHelpEvent* helpEvent = dynamic_cast<QHelpEvent*>(e);
        assert(helpEvent);

        QAction* action = activeAction();
        if (!helpEvent || !action) {
            return false;
        }
        if ( action->text() != action->toolTip() ) {
            QToolTip::showText( helpEvent->globalPos(), action->toolTip() );
        }
    } else {
        QToolTip::hideText();
    }

    return QMenu::event(e);
}

