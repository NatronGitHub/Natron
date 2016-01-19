/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
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

NATRON_NAMESPACE_ENTER;

class QtEnumConvert
{
public:

    static Natron::Key fromQtKey(Qt::Key k);
    static Natron::KeyboardModifier fromQtModifier(Qt::KeyboardModifier m);
    static Natron::KeyboardModifiers fromQtModifiers(Qt::KeyboardModifiers m);
    static StandardButtonEnum fromQtStandardButton(QMessageBox::StandardButton b);
    static QMessageBox::StandardButton toQtStandardButton(StandardButtonEnum b);
    static QMessageBox::StandardButtons toQtStandarButtons(StandardButtons buttons);
};

NATRON_NAMESPACE_EXIT;

#endif // Gui_QtEnumConvert_h
