/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
 * Copyright (C) 2018-2020 The Natron developers
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

#include "ChannelsComboBox.h"

#include <cassert>
#include <stdexcept>

#include <QtCore/QDebug>

GCC_DIAG_UNUSED_PRIVATE_FIELD_OFF
// /opt/local/include/QtGui/qmime.h:119:10: warning: private field 'type' is not used [-Wunused-private-field]
#include <QtGui/QPaintEvent>
GCC_DIAG_UNUSED_PRIVATE_FIELD_ON

NATRON_NAMESPACE_ENTER

ChannelsComboBox::ChannelsComboBox(QWidget *parent)
    : ComboBox(parent)
{
    setObjectName(QString::fromUtf8("ChannelsComboBox"));
}

void ChannelsComboBox::paintEvent(QPaintEvent *event)
{
    ComboBox::paintEvent(event);
    int idx = activeIndex();

    setProperty("color", idx);

    if (current_index != idx)
        updateStyle();

    current_index = idx;
}

NATRON_NAMESPACE_EXIT
