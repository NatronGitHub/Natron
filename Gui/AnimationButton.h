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

#ifndef ANIMATIONBUTTON_H
#define ANIMATIONBUTTON_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Gui/Button.h"
#include "Gui/GuiFwd.h"


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

Q_SIGNALS:

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
