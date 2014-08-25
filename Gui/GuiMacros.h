//
//  GuiMacros.h
//  Gui
//
//  Created by Frédéric Devernay on 19/08/2014.
//
//

#ifndef Gui_GuiMacros_h
#define Gui_GuiMacros_h

// the following macro only tests the Control, Alt, and Shift modifiers, and discards the others
#define modCAS(e) ((e)->modifiers()&(Qt::ControlModifier|Qt::AltModifier|Qt::ShiftModifier))

#define modCASIsNone(e)            (modCAS(e) == (Qt::NoModifier))
#define modCASIsControl(e)         (modCAS(e) == (Qt::ControlModifier))
#define modCASIsControlShift(e)    (modCAS(e) == (Qt::ControlModifier|Qt::ShiftModifier))
#define modCASIsControlAlt(e)      (modCAS(e) == (Qt::ControlModifier|Qt::AltModifier))
#define modCASIsControlAltShift(e) (modCAS(e) == (Qt::ControlModifier|Qt::AltModifier|Qt::ShiftModifier))
#define modCASIsAlt(e)             (modCAS(e) == (Qt::AltModifier))
#define modCASIsAltShift(e)        (modCAS(e) == (Qt::AltModifier|Qt::ShiftModifier))
#define modCASIsShift(e)           (modCAS(e) == (Qt::ShiftModifier))

// macros to test if the modifier is present (but there may be other modifiers too)
#define modifierHasControl(e)         ((e)->modifiers().testFlag(Qt::ControlModifier))
#define modifierHasAlt(e)             ((e)->modifiers().testFlag(Qt::AltModifier))
#define modifierHasShift(e)           ((e)->modifiers().testFlag(Qt::ShiftModifier))

// macros to test if a button is held down (pressed), or a single-button compatibility combination
#define buttonControlAlt(e)          ((e)->modifiers() & (Qt::ControlModifier|Qt::AltModifier))



#if 0



/// THE GOOD VERSION
// PLEASE DON'T BREAK THIS.
// THANK YOU VERY VERY VERY MUCH.
#define buttonDownIsLeft(e)              (((e)->buttons() == Qt::LeftButton   && buttonControlAlt(e) == Qt::NoModifier))
#define buttonDownIsRight(e)             (((e)->buttons() == Qt::RightButton  && buttonControlAlt(e) == Qt::NoModifier) || \
((e)->buttons() == Qt::LeftButton   && buttonControlAlt(e) == Qt::ControlModifier))

#define triggerButtonisLeft(e) ((e)->button() == Qt::LeftButton && buttonControlAlt(e) == Qt::NoModifier)
#define triggerButtonisRight(e) ((e)->button() == Qt::RightButton || \
    ((e)->button() == Qt::LeftButton   && buttonControlAlt(e) == Qt::ControlModifier))

#else




#pragma message WARN("left button + CRTL is right click. https://github.com/MrKepzie/Natron/commit/c58083ed00d1898d0c7efc14bc79ce9c83efaa43 is wrong.")
// PLEASE PLEASE PLEASE FIX THIS.
// IF YOU NEED LEFT+CTRL MODIFIER, IT IS JUST CALLED THE RIGHT BUTTON.
// WHERE DO YOU NEED LEFT+CTRL????
// TELL ME WHERE YOU NEED IT, I WILL PROPOSE A NICER SOLUTION. THERE MUST BE A BETTER SOLUTION.
// THE ONLY MODIFIER FOR BUTTONS SHOULD BE SHIFT.
// SEE THE MACRO buttonModifierIsShift BELOW.
// THIS IS TE REASON WHY THERE IS THE buttonModifier MACRO BELOW.....
// DON'T BREAK IT, PLEASE.
// THANK YOU.

// THE BAD VERSION
#define buttonDownIsLeft(e)              ((e)->buttons() == Qt::LeftButton && !modifierHasAlt(e))
#define buttonDownIsRight(e)             (((e)->buttons() == Qt::RightButton  && buttonControlAlt(e) == Qt::NoModifier))

#define triggerButtonisLeft(e) ((e)->button() == Qt::LeftButton)
#define triggerButtonisRight(e) ((e)->button() == Qt::RightButton)


#endif



#define buttonDownIsMiddle(e)            (((e)->buttons() == Qt::MiddleButton && buttonControlAlt(e) == Qt::NoModifier) || \
    ((e)->buttons() == Qt::LeftButton   && buttonControlAlt(e) == Qt::AltModifier))




// macros to test the button that triggered the event
#define triggerButtonisMiddle(e) ((e)->button() == Qt::MiddleButton || \
((e)->button() == Qt::LeftButton   && buttonControlAlt(e) == Qt::AltModifier))


// macros to test
#define buttonModifier(e)            ((e)->modifiers() & (Qt::KeyboardModifierMask & ~(Qt::ControlModifier|Qt::AltModifier)))
#define buttonModifierIsNone(e)      (buttonModifier(e) == Qt::NoModifier)
#define buttonModifierIsShift(e)     (buttonModifier(e) == Qt::ShiftModifier)

#endif
