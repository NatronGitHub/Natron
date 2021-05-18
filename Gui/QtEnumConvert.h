/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2021 The Natron developers
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

#ifndef Gui_QtEnumConvert_h
#define Gui_QtEnumConvert_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <vector>

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <Qt>
#include <QMessageBox>
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Global/KeySymbols.h"
#include "Global/Enums.h"

#include "Gui/GuiFwd.h"

NATRON_NAMESPACE_ENTER

class QtEnumConvert
{
public:

    static Key fromQtKey(Qt::Key k);
    static KeyboardModifierEnum fromQtModifier(Qt::KeyboardModifier m);
    static KeyboardModifiers fromQtModifiers(Qt::KeyboardModifiers m);
    static Qt::Key toQtKey(Key k);
    static Qt::KeyboardModifiers fromOfxtoQtModifiers(const std::list<int>& modifiers);
    static bool fromOfxtoQtModifier(int modifier, Qt::KeyboardModifier* mod);
    static Qt::KeyboardModifier  toQtModifier(KeyboardModifierEnum m);
    static Qt::KeyboardModifiers toQtModifiers(const KeyboardModifiers& modifiers);
    static StandardButtonEnum fromQtStandardButton(QMessageBox::StandardButton b);
    static QMessageBox::StandardButton toQtStandardButton(StandardButtonEnum b);
    static QMessageBox::StandardButtons toQtStandarButtons(StandardButtons buttons);
    static bool toQtCursor(CursorEnum c, Qt::CursorShape* ret);
};

NATRON_NAMESPACE_EXIT

#endif // Gui_QtEnumConvert_h
