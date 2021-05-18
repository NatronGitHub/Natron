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

#ifndef Gui_GuiDefines_h
#define Gui_GuiDefines_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#define NATRON_BUTTON_BORDER 4
#define NATRON_SMALL_BUTTON_SIZE 15
#define NATRON_SMALL_BUTTON_ICON_SIZE (NATRON_SMALL_BUTTON_SIZE - NATRON_BUTTON_BORDER)
#define NATRON_MEDIUM_BUTTON_SIZE 22
#define NATRON_MEDIUM_BUTTON_ICON_SIZE (NATRON_MEDIUM_BUTTON_SIZE - NATRON_BUTTON_BORDER)
#define NATRON_LARGE_BUTTON_SIZE 30
#define NATRON_LARGE_BUTTON_ICON_SIZE (NATRON_LARGE_BUTTON_SIZE - NATRON_BUTTON_BORDER)
#define NATRON_TOOL_BUTTON_ICON_SIZE 24 // same as Nuke
#define NATRON_TOOL_BUTTON_BORDER 6 // a bit more than NATRON_BUTTON_BORDER
#define NATRON_TOOL_BUTTON_SIZE (NATRON_TOOL_BUTTON_ICON_SIZE + NATRON_TOOL_BUTTON_BORDER)

#define NATRON_PREVIEW_WIDTH 64
#define NATRON_PREVIEW_HEIGHT 38

#define NODE_WIDTH 80
#define NODE_HEIGHT 30

#define NATRON_WHEEL_ZOOM_PER_DELTA 1.00152 // 120 wheel deltas (one click on a standard wheel mouse) is x1.2
//#define NATRON_FONT "Helvetica"
//#define NATRON_FONT_ALT "Times"
#define NATRON_FONT "Droid Sans"
#define NATRON_FONT_ALT "Droid Sans"
#define NATRON_SCRIPT_FONT "Courier New"
#define NATRON_FONT_SIZE_6 6
#define NATRON_FONT_SIZE_8 8
#define NATRON_FONT_SIZE_10 10
#define NATRON_FONT_SIZE_11 11
#define NATRON_FONT_SIZE_12 12
#define NATRON_FONT_SIZE_13 13
#define NATRON_FONT_SIZE_DEFAULT NATRON_FONT_SIZE_11 // the sliders font becomes undreadable below 11 on non-HiDPI mac displays

#define NATRON_MAX_RECENT_FILES 10 // 10 is the default is most apps

#endif // Gui_GuiDefines_h
