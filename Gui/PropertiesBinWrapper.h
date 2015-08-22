//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _Gui_PropertiesBinWrapper_h_
#define _Gui_PropertiesBinWrapper_h_

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include <QWidget>

#include "Global/Macros.h"

#include "Engine/ScriptObject.h"

class PropertiesBinWrapper : public QWidget, public ScriptObject
{
public:
    PropertiesBinWrapper(QWidget* parent)
    : QWidget(parent)
    , ScriptObject()
    {
    }
};

#endif // _Gui_PropertiesBinWrapper_h_
