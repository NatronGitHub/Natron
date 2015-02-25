//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "Button.h"

#include <QApplication>
#include "Gui/GuiApplicationManager.h"

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
    setFont(QApplication::font()); // necessary, or the buttons will get the default font size
    //setFont( QFont(appFont,appFontSize) );
}

