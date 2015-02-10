//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "NodeBackDropSerialization.h"

NodeBackDropSerialization::NodeBackDropSerialization()
    : posX(0),posY(0),width(0),height(0),name(),label(),r(0),g(0),b(0),selected(false), _isNull(true)
{
}

