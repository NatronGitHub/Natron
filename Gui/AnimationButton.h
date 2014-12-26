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

class AnimationButton
    : public Button
{
    Q_OBJECT

public:

    explicit AnimationButton(KnobGui* knob,
                             QWidget* parent = 0)
        : Button(parent)
          , _dragging(false)
          , _knob(knob)
    {
        setMouseTracking(true);
        setAcceptDrops(true);
        setFocusPolicy(Qt::NoFocus);
    }

    explicit AnimationButton(KnobGui* knob,
                             const QString & text,
                             QWidget * parent = 0)
        : Button(text,parent)
          , _dragging(false)
          , _knob(knob)
    {
        setMouseTracking(true);
        setAcceptDrops(true);
        setFocusPolicy(Qt::NoFocus);
    }

    AnimationButton(KnobGui* knob,
                    const QIcon & icon,
                    const QString & text,
                    QWidget * parent = 0)
        : Button(icon,text,parent)
          , _dragging(false)
          , _knob(knob)
    {
        setMouseTracking(true);
        setAcceptDrops(true);
        setFocusPolicy(Qt::NoFocus);
    }

signals:

    void animationMenuRequested();

private:

    virtual void mousePressEvent(QMouseEvent* e);
    virtual void mouseMoveEvent(QMouseEvent* e);
    virtual void mouseReleaseEvent(QMouseEvent* e);
    virtual void dragEnterEvent(QDragEnterEvent* e);
    virtual void dragMoveEvent(QDragMoveEvent* e);
    virtual void dragLeaveEvent(QDragLeaveEvent* e);
    virtual void dropEvent(QDropEvent* e);
    virtual void enterEvent(QEvent* e);
    virtual void leaveEvent(QEvent* e);
    QPoint _dragPos;
    bool _dragging;
    KnobGui* _knob;
};

#endif // ANIMATIONBUTTON_H
