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

#include <QToolTip>
#include <QHelpEvent>

bool MenuWithToolTips::event (QEvent * e)
{
    const QHelpEvent *helpEvent = static_cast <QHelpEvent *>(e);
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
