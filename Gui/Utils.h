//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef Natron_Gui_Utils_h_
#define Natron_Gui_Utils_h_

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include <QTextDocument> // for Qt::WhiteSpaceMode

namespace Natron {
QString convertFromPlainText(const QString &plain, Qt::WhiteSpaceMode mode);
}

#endif // Natron_Gui_Utils_h_
