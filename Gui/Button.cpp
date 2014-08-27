//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "Button.h"

Button::Button(QWidget* parent)
    : QPushButton(parent)
{
    setAttribute(Qt::WA_LayoutUsesWidgetRect);
    initInternal();
}

Button::Button(const QString & text,
               QWidget * parent)
    : QPushButton(text,parent)
{
    setAttribute(Qt::WA_LayoutUsesWidgetRect);
    initInternal();
}

Button::Button(const QIcon & icon,
               const QString & text,
               QWidget * parent)
    : QPushButton(icon,text,parent)
{
    setAttribute(Qt::WA_LayoutUsesWidgetRect);
    initInternal();
}

void
Button::initInternal()
{
    setFont( QFont(NATRON_FONT,NATRON_FONT_SIZE_11) );
}