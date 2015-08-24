//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

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
CLANG_DIAG_ON(unused-private-field)
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

