/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2026 The Natron developers
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

#include "KnobGuiBytes.h"

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QHBoxLayout>
#include <QCoreApplication>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/KnobTypes.h"

#include "Gui/Label.h"

NATRON_NAMESPACE_ENTER

KnobGuiBytes::KnobGuiBytes(KnobIPtr knob,
                           KnobGuiContainerI *container)
    : KnobGui(knob, container)
    , _label(0)
{
    _knob = std::dynamic_pointer_cast<KnobBytes>(knob);
}

KnobGuiBytes::~KnobGuiBytes()
{
}

void
KnobGuiBytes::createWidget(QHBoxLayout* layout)
{
    _label = new Label( QCoreApplication::translate("KnobGuiBytes", "(binary data)"), layout->parentWidget() );
    layout->addWidget(_label);
}

void
KnobGuiBytes::removeSpecificGui()
{
    if (_label) {
        _label->deleteLater();
        _label = 0;
    }
}

void
KnobGuiBytes::_hide()
{
    if (_label) {
        _label->hide();
    }
}

void
KnobGuiBytes::_show()
{
    if (_label) {
        _label->show();
    }
}

KnobIPtr
KnobGuiBytes::getKnob() const
{
    return _knob.lock();
}

NATRON_NAMESPACE_EXIT
