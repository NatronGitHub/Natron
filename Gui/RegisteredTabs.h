//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _Gui_RegisteredTabs_h_
#define _Gui_RegisteredTabs_h_

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include <utility> // pair
#include <map>
#include <string>

class QWidget;
class ScriptObject;

typedef std::map<std::string,std::pair<QWidget*,ScriptObject*> > RegisteredTabs;

#endif // _Gui_RegisteredTabs_h_
