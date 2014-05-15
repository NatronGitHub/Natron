//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */

#ifndef FROMQTENUMS_H
#define FROMQTENUMS_H

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

class QtEnumConvert {
    
public:

static Natron::Key fromQtKey(Qt::Key k);
    
static Natron::KeyboardModifier fromQtModifier(Qt::KeyboardModifier m);
    
static Natron::KeyboardModifiers fromQtModifiers(Qt::KeyboardModifiers m);

static Natron::StandardButton fromQtStandardButton(QMessageBox::StandardButton b);
    
static QMessageBox::StandardButton toQtStandardButton(Natron::StandardButton b);

static QMessageBox::StandardButtons toQtStandarButtons(Natron::StandardButtons buttons);

}; //namespace Natron

#endif // FROMQTENUMS_H
