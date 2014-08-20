//
//  GuiMacros.h
//  Gui
//
//  Created by Frédéric Devernay on 19/08/2014.
//
//

#ifndef Gui_GuiMacros_h
#define Gui_GuiMacros_h

#define modifierIsNone(e)            (!(e)->modifiers().testFlag(Qt::ShiftModifier) &&  \
!(e)->modifiers().testFlag(Qt::ControlModifier) && \
!(e)->modifiers().testFlag(Qt::AltModifier))

#define modifierIsControl(e)         ((e)->modifiers() == Qt::ControlModifier)
#define modifierIsControlShift(e)    ((e)->modifiers() == (Qt::ControlModifier|Qt::ShiftModifier))
#define modifierIsControlAlt(e)      ((e)->modifiers() == (Qt::ControlModifier|Qt::AltModifier))
#define modifierIsControlAltShift(e) ((e)->modifiers() == (Qt::ControlModifier|Qt::AltModifier|Qt::ShiftModifier))
#define modifierIsAlt(e)             ((e)->modifiers() == Qt::AltModifier)
#define modifierIsAltShift(e)        ((e)->modifiers() == (Qt::AltModifier|Qt::ShiftModifier))
#define modifierIsShift(e)           ((e)->modifiers() == Qt::ShiftModifier)

#define buttonIsLeft(e)              ((e)->button() == Qt::LeftButton && modifierIsNone(e))
#define buttonIsMiddle(e)            (((e)->button() == Qt::MiddleButton && modifierIsNone(e)) || (e->button() == Qt::LeftButton && modifierIsAlt(e)))
#define buttonIsRight(e)             (((e)->button() == Qt::RightButton && modifierIsNone(e)) || (e->button() == Qt::LeftButton && modifierIsControl(e)))
#define buttonIsRightShift(e)             (((e)->button() == Qt::RightButton && modifierIsShift(e)) || (e->button() == Qt::LeftButton && modifierIsControlShift(e)))

#endif
