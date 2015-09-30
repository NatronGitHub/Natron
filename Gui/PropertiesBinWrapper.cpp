/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
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

#include "PropertiesBinWrapper.h"

#include <QApplication>

#include "Gui/Gui.h"

PropertiesBinWrapper::PropertiesBinWrapper(Gui* parent)
: QWidget(parent)
, PanelWidget(this,parent)
{
    QObject::connect(qApp, SIGNAL(focusChanged(QWidget*,QWidget*)), this, SLOT(onFocusChanged(QWidget*, QWidget*)));
}

PropertiesBinWrapper::~PropertiesBinWrapper()
{
    
}

void
PropertiesBinWrapper::mousePressEvent(QMouseEvent* e)
{
    takeClickFocus();
    QWidget::mousePressEvent(e);
}

static PropertiesBinWrapper* isPropertiesBinChild(QWidget* w, int recursionLevel)
{
    if (!w) {
        return 0;
    }
    PropertiesBinWrapper* pw = dynamic_cast<PropertiesBinWrapper*>(w);
    if (pw && recursionLevel > 0) {
        /*
         Do not return it if recursion is 0, otherwise the focus stealing of the mouse over will actually take click focus
         */
        return pw;
    }
    return isPropertiesBinChild(w->parentWidget(), recursionLevel + 1);
}

void
PropertiesBinWrapper::onFocusChanged(QWidget* /*old*/, QWidget* newFocus)
{
    PropertiesBinWrapper* pw = isPropertiesBinChild(newFocus,0);
    if (pw) {
        assert(pw == this);
        takeClickFocus();
    }
}

void
PropertiesBinWrapper::enterEvent(QEvent* e)
{
    enterEventBase();
    QWidget::enterEvent(e);
}

void
PropertiesBinWrapper::leaveEvent(QEvent* e)
{
    leaveEventBase();
    QWidget::leaveEvent(e);
}