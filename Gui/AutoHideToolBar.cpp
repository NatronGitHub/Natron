//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "AutoHideToolBar.h"

#include "Gui/Gui.h"

AutoHideToolBar::AutoHideToolBar(Gui* gui,
                                 QWidget* parent)
: QToolBar(parent)
, _gui(gui)
{
}

void AutoHideToolBar::leaveEvent(QEvent* e)
{
    if ( _gui->isLeftToolBarDisplayedOnMouseHoverOnly() ) {
        _gui->setLeftToolBarVisible(false);
    }
    QToolBar::leaveEvent(e);
}

