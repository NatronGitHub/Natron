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

#include "AnimationButton.h"

#include <algorithm> // min, max

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
GCC_DIAG_UNUSED_PRIVATE_FIELD_OFF
// /opt/local/include/QtGui/qmime.h:119:10: warning: private field 'type' is not used [-Wunused-private-field]
#include <QMouseEvent>
GCC_DIAG_UNUSED_PRIVATE_FIELD_ON
#include <QApplication>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDropEvent>
#include <QDrag>
#include <QPainter>
#include <QMimeData>
#include <QKeyEvent>
CLANG_DIAG_ON(deprecated)

#include "Engine/Project.h"
#include "Engine/Knob.h"
#include "Engine/AppManager.h"
#include "Engine/EffectInstance.h"
#include "Engine/Node.h"

#include "Gui/KnobGui.h"
#include "Gui/KnobUndoCommand.h"
#include "Gui/GuiMacros.h"
#include "Gui/GuiApplicationManager.h"

NATRON_NAMESPACE_ENTER;

void
AnimationButton::mousePressEvent(QMouseEvent* e)
{
    if ( buttonDownIsLeft(e) ) {
        _dragPos = e->pos();
        _dragging = true;
    }

    QPushButton::mousePressEvent(e);
}

void
AnimationButton::keyPressEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Control) {
        QPixmap p;
        appPTR->getIcon(NATRON_PIXMAP_LINK_MULT_CURSOR, &p);
        QCursor c(p);
        setCursor(c);
    }
    QPushButton::keyPressEvent(e);
}

void
AnimationButton::keyReleaseEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Control) {
        QPixmap p;
        appPTR->getIcon(NATRON_PIXMAP_LINK_CURSOR, &p);
        QCursor c(p);
        setCursor(c);
    }
    QPushButton::keyReleaseEvent(e);
}

void
AnimationButton::mouseMoveEvent(QMouseEvent* e)
{
    if (_dragging) {
        // If the left button isn't pressed anymore then return
        if ( !buttonDownIsLeft(e) ) {
            return;
        }
        // If the distance is too small then return
        if ( (e->pos() - _dragPos).manhattanLength() < QApplication::startDragDistance() ) {
            return;
        }
        
        EffectInstance* effect = dynamic_cast<EffectInstance*>(_knob->getKnob()->getHolder());
        if (!effect) {
            return;
        }
        boost::shared_ptr<NodeCollection> group = effect->getNode()->getGroup();
        NodeGroup* isGroup = dynamic_cast<NodeGroup*>(group.get());
        
        
        std::stringstream expr;
        if (isGroup) {
            expr << "thisGroup.";
        } else {
            expr << effect->getApp()->getAppIDString() << ".";
        }
        expr << effect->getNode()->getScriptName_mt_safe() << "." << _knob->getKnob()->getName()
        << ".getValue(dimension)";

        Qt::KeyboardModifiers modifiers = qApp->keyboardModifiers();
        bool useMult = modifiers.testFlag(Qt::ControlModifier);
        if (useMult) {
            expr << " * curve(frame,dimension)";
        }

        // initiate Drag

        _knob->onCopyAnimationActionTriggered();
        QDrag* drag = new QDrag(this);
        QMimeData* mimeData = new QMimeData;
        mimeData->setData("Animation", QByteArray(expr.str().c_str()));
        drag->setMimeData(mimeData);

        QFontMetrics fmetrics = fontMetrics();
        QString textFirstLine( tr("Linking value from:") );
        QString textSecondLine( _knob->getKnob()->getLabel().c_str() );
        QString textThirdLine( tr("Drag it to another animation button.") );
        int textWidth = std::max( std::max( fmetrics.width(textFirstLine), fmetrics.width(textSecondLine) ),fmetrics.width(textThirdLine) );
        QImage dragImg(textWidth,(fmetrics.height() + 5) * 3,QImage::Format_ARGB32);
        dragImg.fill( QColor(243,137,0) );
        QPainter p(&dragImg);
        p.drawText(QPointF(0,dragImg.height() - 2.5), textThirdLine);
        p.drawText(QPointF(0,dragImg.height() - fmetrics.height() - 5), textSecondLine);
        p.drawText(QPointF(0,dragImg.height() - fmetrics.height() * 2 - 10), textFirstLine);

        drag->setPixmap( QPixmap::fromImage(dragImg) );
        setDown(false);
        drag->exec();
    } else {
        QPushButton::mouseMoveEvent(e);
    }
}

void
AnimationButton::mouseReleaseEvent(QMouseEvent* e)
{
    _dragging = false;
    QPushButton::mouseReleaseEvent(e);
    Q_EMIT animationMenuRequested();
}

void
AnimationButton::dragEnterEvent(QDragEnterEvent* e)
{
    if (e->source() == this) {
        return;
    }
    QStringList formats = e->mimeData()->formats();
    if ( formats.contains("Animation") ) {
        setCursor(Qt::DragCopyCursor);
        e->acceptProposedAction();
    }
}

void
AnimationButton::dragLeaveEvent(QDragLeaveEvent* /*event*/)
{
}

void
AnimationButton::dropEvent(QDropEvent* e)
{
    e->accept();
    QStringList formats = e->mimeData()->formats();
    if ( formats.contains("Animation") ) {
        QByteArray expr = e->mimeData()->data("Animation");
        std::string expression(expr.data());
        boost::shared_ptr<KnobI> knob = _knob->getKnob();
        _knob->pushUndoCommand(new SetExpressionCommand(knob,false,-1,expression));
        e->acceptProposedAction();
    }
}

void
AnimationButton::dragMoveEvent(QDragMoveEvent* e)
{
    QStringList formats = e->mimeData()->formats();

    if ( formats.contains("Animation") ) {
        e->acceptProposedAction();
    }
}

void
AnimationButton::enterEvent(QEvent* /*e*/)
{
    if (cursor().shape() != Qt::OpenHandCursor) {
        QPixmap p;
        Qt::KeyboardModifiers modifiers = qApp->keyboardModifiers();
        if (modifiers.testFlag(Qt::ControlModifier)) {
            appPTR->getIcon(NATRON_PIXMAP_LINK_MULT_CURSOR, &p);
        } else {
            appPTR->getIcon(NATRON_PIXMAP_LINK_CURSOR, &p);
        }
        QCursor c(p);
        setCursor(c);
    }
}

void
AnimationButton::leaveEvent(QEvent* /*e*/)
{
    if (cursor().shape() == Qt::OpenHandCursor) {
        unsetCursor();
    }
}

NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_AnimationButton.cpp"
