//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef NATRON_GUI_MENUWITHTOOLTIPS_H_
#define NATRON_GUI_MENUWITHTOOLTIPS_H_

#include <QMenu>

class MenuWithToolTips : public QMenu {
public:
    MenuWithToolTips(QWidget* parent):QMenu(parent){}
    
    bool event(QEvent * e);
};

#endif // NATRON_GUI_MENUWITHTOOLTIPS_H_
