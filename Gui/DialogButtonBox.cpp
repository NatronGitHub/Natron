/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2020 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#include "DialogButtonBox.h"

#include <QApplication>
#include <QPushButton>

NATRON_NAMESPACE_ENTER

DialogButtonBox::DialogButtonBox(StandardButtons buttons,
                                 Qt::Orientation orientation,
                         QWidget *parent)
: QDialogButtonBox(buttons, orientation, parent)
{
    QList<QAbstractButton *> buttonsList = QDialogButtonBox::buttons();
    Q_FOREACH(QAbstractButton *b, buttonsList) {
        b->setAttribute(Qt::WA_LayoutUsesWidgetRect); // Don't use the layout rect calculated from QMacStyle.
        b->setFont( QApplication::font() ); // necessary, or the buttons will get the default font size
    }
}

void
DialogButtonBox::addButton(QAbstractButton *button, ButtonRole role)
{
    QDialogButtonBox::addButton(button, role);
    button->setAttribute(Qt::WA_LayoutUsesWidgetRect); // Don't use the layout rect calculated from QMacStyle.
    button->setFont( QApplication::font() ); // necessary, or the buttons will get the default font size
}

QPushButton *
DialogButtonBox::addButton(StandardButton button)
{
    QPushButton* b = QDialogButtonBox::addButton(button);
    b->setAttribute(Qt::WA_LayoutUsesWidgetRect); // Don't use the layout rect calculated from QMacStyle.
    b->setFont( QApplication::font() ); // necessary, or the buttons will get the default font size
    return b;
}

QPushButton *
DialogButtonBox::addButton(const QString &text, ButtonRole role)
{
    QPushButton* b = QDialogButtonBox::addButton(text, role);
    b->setAttribute(Qt::WA_LayoutUsesWidgetRect); // Don't use the layout rect calculated from QMacStyle.
    b->setFont( QApplication::font() ); // necessary, or the buttons will get the default font size
    return b;
}

void
DialogButtonBox::setStandardButtons(StandardButtons buttons)
{
    QDialogButtonBox::setStandardButtons(buttons);
    QList<QAbstractButton *> buttonsList = QDialogButtonBox::buttons();
    Q_FOREACH(QAbstractButton *b, buttonsList) {
        b->setAttribute(Qt::WA_LayoutUsesWidgetRect); // Don't use the layout rect calculated from QMacStyle.
        b->setFont( QApplication::font() ); // necessary, or the buttons will get the default font size
    }
}

NATRON_NAMESPACE_EXIT

