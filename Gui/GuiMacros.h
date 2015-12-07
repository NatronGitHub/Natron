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

#ifndef Gui_GuiMacros_h
#define Gui_GuiMacros_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

// Mac keyboard:
// ctrl = MetaModifier
// alt = AltModifier
// cmd = CtrlModifier

// the following macro only tests the Control, Alt, and Shift modifiers, and discards the others
#define modCAS(e) ( (e)->modifiers() & (Qt::ControlModifier | Qt::AltModifier | Qt::ShiftModifier) )

#define modCASIsNone(e)            ( modCAS(e) == (Qt::NoModifier) )
#define modCASIsControl(e)         ( modCAS(e) == (Qt::ControlModifier) )
#define modCASIsControlShift(e)    ( modCAS(e) == (Qt::ControlModifier | Qt::ShiftModifier) )
#define modCASIsControlAlt(e)      ( modCAS(e) == (Qt::ControlModifier | Qt::AltModifier) )
#define modCASIsControlAltShift(e) ( modCAS(e) == (Qt::ControlModifier | Qt::AltModifier | Qt::ShiftModifier) )
#define modCASIsAlt(e)             ( modCAS(e) == (Qt::AltModifier) )
#define modCASIsAltShift(e)        ( modCAS(e) == (Qt::AltModifier | Qt::ShiftModifier) )
#define modCASIsShift(e)           ( modCAS(e) == (Qt::ShiftModifier) )

// macros to test if the modifier is present (but there may be other modifiers too)
#define modifierHasControl(e)         ( (e)->modifiers().testFlag(Qt::ControlModifier) )
#define modifierHasAlt(e)             ( (e)->modifiers().testFlag(Qt::AltModifier) )
#define modifierHasShift(e)           ( (e)->modifiers().testFlag(Qt::ShiftModifier) )

// macros to test if a button is held down (pressed), or a single-button compatibility combination
// Right click emulated with Left + MetaModifier (ctrl key on the keyboard), which is the way its done everywhere else on the mac
// Middle click emulated with Left + AltModifier (alt key on the keyboard)
#define buttonMetaAlt(e)          ( (e)->modifiers() & (Qt::MetaModifier | Qt::AltModifier) )

/// THE GOOD VERSION
// PLEASE DON'T BREAK THIS.
// THANK YOU VERY VERY VERY MUCH.
#define buttonDownIsLeft(e)              ( ( (e)->buttons() == Qt::LeftButton   && buttonMetaAlt(e) == Qt::NoModifier ) )

//Right click emulated with Left + MetaModifier, which is the way its done everywhere else on the mac
#define buttonDownIsRight(e)             ( ( (e)->buttons() == Qt::RightButton  && buttonMetaAlt(e) == Qt::NoModifier ) || \
                                           ( (e)->buttons() == Qt::LeftButton   && buttonMetaAlt(e) == Qt::MetaModifier ) )

#define triggerButtonIsLeft(e) ( (e)->button() == Qt::LeftButton && buttonMetaAlt(e) == Qt::NoModifier )
#define triggerButtonIsRight(e) ( (e)->button() == Qt::RightButton || \
                                  ( (e)->button() == Qt::LeftButton   && buttonMetaAlt(e) == Qt::MetaModifier ) )


#define buttonDownIsMiddle(e)            ( ( (e)->buttons() == Qt::MiddleButton && buttonMetaAlt(e) == Qt::NoModifier ) || \
                                           ( (e)->buttons() == Qt::LeftButton   && buttonMetaAlt(e) == Qt::AltModifier ) )


// macros to test the button that triggered the event
#define triggerButtonIsMiddle(e) ( (e)->button() == Qt::MiddleButton || \
                                   ( (e)->button() == Qt::LeftButton   && buttonMetaAlt(e) == Qt::AltModifier ) )


// macros to test
#define buttonModifier(e)            ( (e)->modifiers() & ( Qt::KeyboardModifierMask & ~(Qt::MetaModifier | Qt::AltModifier) ) )
#define buttonModifierIsNone(e)      (buttonModifier(e) == Qt::NoModifier)
#define buttonModifierIsShift(e)     (buttonModifier(e) == Qt::ShiftModifier)
#define buttonModifierIsControl(e)   (buttonModifier(e) == Qt::ControlModifier)

#endif // ifndef Gui_GuiMacros_h
