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

///From Qt Doc:
/*
 Qt::MouseButton QMouseEvent::button() const
 Returns the button that caused the event.
 Note that the returned value is always Qt::NoButton for mouse move events.
 
 Hence calling e->button() is not suitable for mouseMoveEvents, it's better checking e->buttons() instead
 */
#define buttonIsLeft(e)              ((e)->buttons().testFlag(Qt::LeftButton) && modifierIsNone(e))
#define buttonIsMiddle(e)            (((e)->buttons().testFlag(Qt::MiddleButton) && modifierIsNone(e)) || (e->buttons().testFlag(Qt::LeftButton) && modifierIsAlt(e)))
#define buttonIsRight(e)             (((e)->buttons().testFlag(Qt::RightButton) && modifierIsNone(e)) || (e->buttons().testFlag(Qt::LeftButton) && modifierIsControl(e)))
#define buttonIsRightShift(e)             (((e)->buttons().testFlag(Qt::RightButton) && modifierIsShift(e)) || (e->buttons().testFlag(Qt::LeftButton) && modifierIsControlShift(e)))

#endif
