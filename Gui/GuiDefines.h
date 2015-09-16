/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
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

#ifndef _Gui_GuiDefines_h_
#define _Gui_GuiDefines_h_

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#define NATRON_BUTTON_BORDER 4
#define NATRON_SMALL_BUTTON_SIZE 15
#define NATRON_SMALL_BUTTON_ICON_SIZE (NATRON_SMALL_BUTTON_SIZE - NATRON_BUTTON_BORDER)
#define NATRON_MEDIUM_BUTTON_SIZE 22
#define NATRON_MEDIUM_BUTTON_ICON_SIZE (NATRON_MEDIUM_BUTTON_SIZE - NATRON_BUTTON_BORDER)
#define NATRON_LARGE_BUTTON_SIZE 30
#define NATRON_LARGE_BUTTON_ICON_SIZE (NATRON_LARGE_BUTTON_SIZE - NATRON_BUTTON_BORDER)
#define NATRON_TOOL_BUTTON_SIZE 36
#define NATRON_TOOL_BUTTON_ICON_SIZE (NATRON_TOOL_BUTTON_SIZE - NATRON_BUTTON_BORDER)

#define NATRON_PREVIEW_WIDTH 64
#define NATRON_PREVIEW_HEIGHT 48
#define NATRON_WHEEL_ZOOM_PER_DELTA 1.00152 // 120 wheel deltas (one click on a standard wheel mouse) is x1.2
//#define NATRON_FONT "Helvetica"
//#define NATRON_FONT_ALT "Times"
#define NATRON_FONT "Droid Sans"
#define NATRON_FONT_ALT "Droid Sans"
#define NATRON_FONT_SIZE_6 6
#define NATRON_FONT_SIZE_8 8
#define NATRON_FONT_SIZE_10 10
#define NATRON_FONT_SIZE_11 11
#define NATRON_FONT_SIZE_12 12
#define NATRON_FONT_SIZE_13 13
#define NATRON_MAX_RECENT_FILES 5

#endif // _Gui_GuiDefines_h_
