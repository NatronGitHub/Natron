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
//We can test equality of eg: e->modifiers() == Qt::ControlModifier because in practise some other modifiers are held
//like the KeyPadModifier. The idea here is to get rid of anything that is not ControlModifier,ShiftModifier and AltModifier.
#define modifierIsControl(e)         ((e)->modifiers().testFlag(Qt::ControlModifier) && \
!(e)->modifiers().testFlag(Qt::ShiftModifier) && \
!(e)->modifiers().testFlag(Qt::AltModifier))

#define modifierIsControlShift(e)    ((e)->modifiers().testFlag(Qt::ControlModifier) && \
(e)->modifiers().testFlag(Qt::ShiftModifier) && \
!(e)->modifiers().testFlag(Qt::AltModifier))

#define modifierIsControlAlt(e)      ((e)->modifiers().testFlag(Qt::ControlModifier) && \
!(e)->modifiers().testFlag(Qt::ShiftModifier) && \
(e)->modifiers().testFlag(Qt::AltModifier))

#define modifierIsControlAltShift(e) ((e)->modifiers().testFlag(Qt::ControlModifier) && \
(e)->modifiers().testFlag(Qt::ShiftModifier) && \
(e)->modifiers().testFlag(Qt::AltModifier))

#define modifierIsAlt(e)             (!(e)->modifiers().testFlag(Qt::ControlModifier) && \
!(e)->modifiers().testFlag(Qt::ShiftModifier) && \
(e)->modifiers().testFlag(Qt::AltModifier))

#define modifierIsAltShift(e)        (!(e)->modifiers().testFlag(Qt::ControlModifier) && \
(e)->modifiers().testFlag(Qt::ShiftModifier) && \
(e)->modifiers().testFlag(Qt::AltModifier))

#define modifierIsShift(e)           (!(e)->modifiers().testFlag(Qt::ControlModifier) && \
(e)->modifiers().testFlag(Qt::ShiftModifier) && \
!(e)->modifiers().testFlag(Qt::AltModifier))

// macros to test if the modifier is present (but there may be other modifiers too)
#define modifierHasControl(e)         ((e)->modifiers().testFlag(Qt::ControlModifier))
#define modifierHasAlt(e)             ((e)->modifiers().testFlag(Qt::AltModifier))
#define modifierHasShift(e)           ((e)->modifiers().testFlag(Qt::ShiftModifier))

//This is the convenient way for checking that effectively no modifier are held, in practise modifierIsNone doesn't work
//because some modifier are always held, like KeyPadModifier
#define modifierIsNone(e) (!modifierHasControl(e) && !modifierHasAlt(e) && !modifierHasShift(e))

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
