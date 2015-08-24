//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

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
        
        Natron::EffectInstance* effect = dynamic_cast<Natron::EffectInstance*>(_knob->getKnob()->getHolder());
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
        expr << effect->getNode()->getFullyQualifiedName() << "." << _knob->getKnob()->getName()
        << ".get()";
        if (_knob->getKnob()->getDimension() > 1) {
            expr << "[dimension]";
        }


        // initiate Drag

        _knob->onCopyAnimationActionTriggered();
        QDrag* drag = new QDrag(this);
        QMimeData* mimeData = new QMimeData;
        mimeData->setData("Animation", QByteArray(expr.str().c_str()));
        drag->setMimeData(mimeData);

        QFontMetrics fmetrics = fontMetrics();
        QString textFirstLine( tr("Linking value from:") );
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
        appPTR->getIcon(Natron::NATRON_PIXMAP_LINK_CURSOR, &p);
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

