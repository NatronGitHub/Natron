//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

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
    repaint();
}

void
AnimatedCheckBox::setReadOnly(bool readOnly)
{
    this->readOnly = readOnly;
    repaint();
}

void
AnimatedCheckBox::setDirty(bool b)
{
    dirty = b;
    repaint();
}

void
AnimatedCheckBox::setChecked(bool c) {
    checked = c;
    Q_EMIT toggled(checked);
    repaint();
}

void
AnimatedCheckBox::keyPressEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Space) {
        checked = !checked;
        Q_EMIT toggled(checked);
        Q_EMIT clicked(checked);
        repaint();
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
        repaint();
        Q_EMIT clicked(checked);
        Q_EMIT toggled(checked);
        
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
        if (readOnly) {
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

