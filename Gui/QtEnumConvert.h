//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _Gui_QtEnumConvert_h_
#define _Gui_QtEnumConvert_h_

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include <vector>

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <Qt>
#include <QMessageBox>
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Global/KeySymbols.h"
#include "Global/Enums.h"

class QtEnumConvert
{
public:

    static Natron::Key fromQtKey(Qt::Key k);
    static Natron::KeyboardModifier fromQtModifier(Qt::KeyboardModifier m);
    static Natron::KeyboardModifiers fromQtModifiers(Qt::KeyboardModifiers m);
    static Natron::StandardButtonEnum fromQtStandardButton(QMessageBox::StandardButton b);
    static QMessageBox::StandardButton toQtStandardButton(Natron::StandardButtonEnum b);
    static QMessageBox::StandardButtons toQtStandarButtons(Natron::StandardButtons buttons);
};

//namespace Natron

#endif // _Gui_QtEnumConvert_h_
