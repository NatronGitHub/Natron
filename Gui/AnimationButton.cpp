//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */


#include "AnimationButton.h"

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(unused-private-field)
// /opt/local/include/QtGui/qmime.h:119:10: warning: private field 'type' is not used [-Wunused-private-field]
#include <QMouseEvent>
CLANG_DIAG_ON(unused-private-field)
#include <QApplication>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDropEvent>
#include <QDrag>
#include <QPainter>
#include <QMimeData>
CLANG_DIAG_ON(deprecated)

#include "Engine/Project.h"
#include "Engine/Knob.h"
#include "Engine/AppManager.h"

#include "Gui/KnobGui.h"
#include "Gui/GuiMacros.h"

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

        // initiate Drag

        _knob->onCopyAnimationActionTriggered();
        QDrag* drag = new QDrag(this);
        QMimeData* mimeData = new QMimeData;
        mimeData->setData("Animation", "");
        drag->setMimeData(mimeData);

        QFontMetrics fmetrics = fontMetrics();
        QString textFirstLine( tr("Copying animation from:") );
        QString textSecondLine( _knob->getKnob()->getDescription().c_str() );
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
    } else  {
        QPushButton::mouseMoveEvent(e);
    }
}

void
AnimationButton::mouseReleaseEvent(QMouseEvent* e)
{
    _dragging = false;
    QPushButton::mouseReleaseEvent(e);
    emit animationMenuRequested();
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
        _knob->onPasteAnimationActionTriggered();
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
        setCursor(Qt::OpenHandCursor);
    }
}

void
AnimationButton::leaveEvent(QEvent* /*e*/)
{
    if (cursor().shape() == Qt::OpenHandCursor) {
        setCursor(Qt::ArrowCursor);
    }
}

