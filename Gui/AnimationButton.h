//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */


#ifndef ANIMATIONBUTTON_H
#define ANIMATIONBUTTON_H

#include "Gui/Button.h"

class QDragEnterEvent;
class QDragLeaveEvent;
class QMouseEvent;
class QDropEvent;
class QEvent;

class KnobGui;

class AnimationButton : public Button
{
public:
    
    explicit AnimationButton(KnobGui* knob,QWidget* parent = 0)
    : Button(parent)
    , _dragging(false)
    , _knob(knob)
    {
        setMouseTracking(true);
        setAcceptDrops(true);
    }
    explicit AnimationButton(KnobGui* knob,const QString & text, QWidget * parent = 0)
    : Button(text,parent)
    , _dragging(false)
    , _knob(knob)
    {
        setMouseTracking(true);
        setAcceptDrops(true);

    }
    AnimationButton(KnobGui* knob,const QIcon & icon, const QString & text, QWidget * parent = 0)
    : Button(icon,text,parent)
    , _dragging(false)
    , _knob(knob)
    {
        setMouseTracking(true);
        setAcceptDrops(true);

    }
    
private:
    
    virtual void mousePressEvent(QMouseEvent* event);
    
    virtual void mouseMoveEvent(QMouseEvent* event);
    
    virtual void mouseReleaseEvent(QMouseEvent* event);
    
    virtual void dragEnterEvent(QDragEnterEvent* event);
    
    virtual void dragMoveEvent(QDragMoveEvent* event);
    
    virtual void dragLeaveEvent(QDragLeaveEvent* event);
    
    virtual void dropEvent(QDropEvent* event);
 
    virtual void enterEvent(QEvent* event);
    
    virtual void leaveEvent(QEvent* event);
    
    QPoint _dragPos;
    bool _dragging;
    KnobGui* _knob;

};

#endif // ANIMATIONBUTTON_H
