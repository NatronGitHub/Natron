//
//  GuiMacros.h
//  Gui
//
//  Created by Frédéric Devernay on 19/08/2014.
//
//

#ifndef Gui_GuiMacros_h
#define Gui_GuiMacros_h

// macros to test if the modifiers are exactly the given combination: we don't allow any other modifier
#define modifierIsNone(e)            ((e)->modifiers() == (Qt::NoModifier))
#define modifierIsControl(e)         ((e)->modifiers() == (Qt::ControlModifier))
#define modifierIsControlShift(e)    ((e)->modifiers() == (Qt::ControlModifier|Qt::ShiftModifier))
#define modifierIsControlAlt(e)      ((e)->modifiers() == (Qt::ControlModifier|Qt::AltModifier))
#define modifierIsControlAltShift(e) ((e)->modifiers() == (Qt::ControlModifier|Qt::AltModifier|Qt::ShiftModifier))
#define modifierIsAlt(e)             ((e)->modifiers() == (Qt::AltModifier))
#define modifierIsAltShift(e)        ((e)->modifiers() == (Qt::AltModifier|Qt::ShiftModifier))
#define modifierIsShift(e)           ((e)->modifiers() == (Qt::ShiftModifier))

// macros to test if the modifier is present (but there may be other modifiers too)
#define modifierHasControl(e)         ((e)->modifiers().testFlag(Qt::ControlModifier))
#define modifierHasAlt(e)             ((e)->modifiers().testFlag(Qt::AltModifier))
#define modifierHasShift(e)           ((e)->modifiers().testFlag(Qt::ShiftModifier))

// macros to test if a button is pressed, or a single-button compatibility combination
#define buttonControlAlt(e)          ((e)->modifiers() & (Qt::ControlModifier|Qt::AltModifier))
#define buttonIsLeft(e)              (((e)->buttons() == Qt::LeftButton   && buttonControlAlt(e) == Qt::NoModifier))
#define buttonIsMiddle(e)            (((e)->buttons() == Qt::MiddleButton && buttonControlAlt(e) == Qt::NoModifier) || \
                                      ((e)->buttons() == Qt::LeftButton   && buttonControlAlt(e) == Qt::AltModifier))
#define buttonIsRight(e)             (((e)->buttons() == Qt::RightButton  && buttonControlAlt(e) == Qt::NoModifier) || \
                                      ((e)->buttons() == Qt::LeftButton   && buttonControlAlt(e) == Qt::ControlModifier))
// macros to test
#define buttonModifier(e)            ((e)->modifiers() & (Qt::KeyboardModifierMask & ~(Qt::ControlModifier|Qt::AltModifier)))
#define buttonModifierIsNone(e)      (buttonModifier(e) == Qt::NoModifier)
#define buttonModifierIsShift(e)     (buttonModifier(e) == Qt::ShiftModifier)

#endif
