/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "AnimatedCheckBox.h"

#include <QStyle>
#include <QPainter>
#include <QStyleOption>
#include "Gui/GuiMacros.h"
CLANG_DIAG_OFF(deprecated-register) //'register' storage class specifier is deprecated
GCC_DIAG_UNUSED_PRIVATE_FIELD_OFF
// /opt/local/include/QtGui/qmime.h:119:10: warning: private field 'type' is not used [-Wunused-private-field]
#include <QMouseEvent>
GCC_DIAG_UNUSED_PRIVATE_FIELD_ON
CLANG_DIAG_ON(deprecated-register)

#include "Gui/GuiApplicationManager.h"
#include "Engine/Settings.h"

AnimatedCheckBox::AnimatedCheckBox(QWidget *parent)
: QFrame(parent), animation(0), readOnly(false), dirty(false), altered(false) ,checked(false)
{
    setFocusPolicy(Qt::StrongFocus);
    setSizePolicy(QSizePolicy(QSizePolicy::Preferred,QSizePolicy::Fixed,QSizePolicy::Label));
    setMouseTracking(true);
    setFrameShape(QFrame::Box);
}

void
AnimatedCheckBox::setAnimation(int i)
{
    animation = i;
    update();
}

void
AnimatedCheckBox::setReadOnly(bool readOnly)
{
    this->readOnly = readOnly;
    update();
}

void
AnimatedCheckBox::setDirty(bool b)
{
    dirty = b;
    update();
}

void
AnimatedCheckBox::setChecked(bool c) {
    checked = c;
    Q_EMIT toggled(checked);
    update();
}

void
AnimatedCheckBox::keyPressEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Space) {
        checked = !checked;
        Q_EMIT toggled(checked);
        Q_EMIT clicked(checked);
        update();
    } else {
        QFrame::keyPressEvent(e);
    }
}

void
AnimatedCheckBox::mousePressEvent(QMouseEvent* e)
{
    if (buttonDownIsLeft(e)) {
        if (readOnly) {
            return;
        }
        checked = !checked;
        update();
        Q_EMIT clicked(checked);
        Q_EMIT toggled(checked);
        
    } else {
        QFrame::mousePressEvent(e);
    }
}

void
AnimatedCheckBox::getBackgroundColor(double *r,double *g,double *b) const
{
    appPTR->getCurrentSettings()->getRaisedColor(r,g,b);
}

void
AnimatedCheckBox::paintEvent(QPaintEvent* e)
{
    QFrame::paintEvent(e);
    
    QStyleOption opt;
    opt.initFrom(this);
    QPainter p(this);
    
    QRectF bRect = rect();
    
    double fw = frameWidth();
    
    
    QPen pen = p.pen();
    QColor activeColor;

    ///Draw bg
    QRectF bgRect = bRect.adjusted(fw / 2., fw / 2., -fw, -fw);
    //bRect.adjust(fw, fw, -fw, -fw);
    
    double bgR = 0.,bgG = 0.,bgB = 0.;
    if (animation == 0) {
        getBackgroundColor(&bgR,&bgG,&bgB);
    } else if (animation == 1) {
        appPTR->getCurrentSettings()->getInterpolatedColor(&bgR, &bgG, &bgB);
    } else if (animation == 2) {
        appPTR->getCurrentSettings()->getKeyframeColor(&bgR, &bgG, &bgB);
    } else if (animation == 3) {
        appPTR->getCurrentSettings()->getExprColor(&bgR, &bgG, &bgB);
    }
    activeColor.setRgbF(bgR, bgG, bgB);
    pen.setColor(activeColor);
    p.setPen(pen);
    
    p.fillRect(bgRect, activeColor);
    
    
    ///Draw tick
    if (checked) {
        if (animation == 3) {
            activeColor = Qt::black;
        } else if (readOnly) {
            activeColor.setRgbF(0.5, 0.5, 0.5);
        } else if (altered) {
            double r,g,b;
            appPTR->getCurrentSettings()->getAltTextColor(&r, &g, &b);
            activeColor.setRgbF(r, g, b);
        } else if (dirty) {
            activeColor = Qt::black;
        } else {
            double r,g,b;
            appPTR->getCurrentSettings()->getTextColor(&r, &g, &b);
            activeColor.setRgbF(r, g, b);
        }
        
        pen.setColor(activeColor);
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(pen);
        p.drawLine(bRect.topLeft(), bRect.bottomRight());
        p.drawLine(bRect.bottomLeft(), bRect.topRight());
        
    }
    
    
    ///Draw frame
    pen.setColor(Qt::black);
    p.setPen(pen);
    p.drawRect(bRect);
    
    ///Draw focus highlight
    if (hasFocus()) {
        double selR,selG,selB;
        appPTR->getCurrentSettings()->getSelectionColor(&selR, &selG, &selB);
        QRectF focusRect = bRect.adjusted(fw, fw, -fw, -fw);
        activeColor.setRgbF(selR, selG, selB);
        pen.setColor(activeColor);
        p.setPen(pen);
        p.drawRect(focusRect);
    }
}

