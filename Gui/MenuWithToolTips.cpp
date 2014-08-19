//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012. 
*contact: immarespond at gmail dot com
*
*/
#include "MenuWithToolTips.h"

#include "Global/Macros.h"

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QToolTip>
CLANG_DIAG_OFF(unused-private-field)
// /opt/local/include/QtGui/qmime.h:119:10: warning: private field 'type' is not used [-Wunused-private-field]
#include <QHelpEvent>
CLANG_DIAG_ON(unused-private-field)
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

MenuWithToolTips::MenuWithToolTips(QWidget* parent)
: QMenu(parent)
{
    setFont(QFont(NATRON_FONT, NATRON_FONT_SIZE_11));
}

bool MenuWithToolTips::event (QEvent* e)
{
    const QHelpEvent* helpEvent = static_cast <QHelpEvent*>(e);
    if (helpEvent->type() == QEvent::ToolTip) {
        QAction* action = activeAction();
        if(!action){
            return false;
        }
        if(action->text() != action->toolTip()){

            QToolTip::showText(helpEvent->globalPos(), action->toolTip());
        }
    } else {
        QToolTip::hideText();
    }
    return QMenu::event(e);
}
