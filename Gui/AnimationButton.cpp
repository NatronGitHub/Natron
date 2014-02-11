//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */


#include "AnimationButton.h"

#include <QMouseEvent>
#include <QApplication>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDropEvent>
#include <QDrag>
#include <QPainter>
#include <QMimeData>
#include "Gui/KnobGui.h"
#include "Engine/Project.h"
#include "Engine/Knob.h"
#include "Engine/AppManager.h"

void AnimationButton::mousePressEvent(QMouseEvent* event){
    if (event->button() == Qt::LeftButton) {
        _dragPos = event->pos();
        _dragging = true;
    }
    QPushButton::mousePressEvent(event);
}

void AnimationButton::mouseMoveEvent(QMouseEvent* event){
    
    if(_dragging){
        // If the left button isn't pressed anymore then return
        if (!(event->buttons() & Qt::LeftButton))
            return;
        // If the distance is too small then return
        if ((event->pos() - _dragPos).manhattanLength() < QApplication::startDragDistance())
            return;
        
        // initiate Drag
        
        _knob->onCopyAnimationActionTriggered();
        QDrag* drag = new QDrag(this);
        QMimeData* mimeData = new QMimeData;
        mimeData->setData("Animation", "");
        drag->setMimeData(mimeData);
        
        QFontMetrics fmetrics = fontMetrics();
        QString textFirstLine("Copying animation from:");
        QString textSecondLine(_knob->getKnob()->getDescription().c_str());
        QString textThirdLine("Drag it to another animation button.");
        
        int textWidth = std::max(std::max(fmetrics.width(textFirstLine), fmetrics.width(textSecondLine)),fmetrics.width(textThirdLine));
        
        QImage dragImg(textWidth,(fmetrics.height() + 5) * 3,QImage::Format_ARGB32);
        dragImg.fill(QColor(243,137,0));
        QPainter p(&dragImg);
        p.drawText(QPointF(0,dragImg.height() - 2.5), textThirdLine);
        p.drawText(QPointF(0,dragImg.height() - fmetrics.height() - 5), textSecondLine);
        p.drawText(QPointF(0,dragImg.height() - fmetrics.height()*2 - 10), textFirstLine);

        drag->setPixmap(QPixmap::fromImage(dragImg));
        
        drag->exec();
    }else{
        QPushButton::mouseMoveEvent(event);
    }
}

void AnimationButton::mouseReleaseEvent(QMouseEvent* event) {
    _dragging = false;
    QPushButton::mouseReleaseEvent(event);
}

void AnimationButton::dragEnterEvent(QDragEnterEvent* event){
    
    if(event->source() == this){
        return;
    }
    QStringList formats = event->mimeData()->formats();
    if (formats.contains("Animation")) {
        setCursor(Qt::DragCopyCursor);
        event->acceptProposedAction();
    }
}

void AnimationButton::dragLeaveEvent(QDragLeaveEvent* /*event*/){

}

void AnimationButton::dropEvent(QDropEvent* event){
    event->accept();
    QStringList formats = event->mimeData()->formats();
    if (formats.contains("Animation")) {
        _knob->onPasteActionTriggered();
        event->acceptProposedAction();
    }
}

void AnimationButton::dragMoveEvent(QDragMoveEvent* event) {
    
    QStringList formats = event->mimeData()->formats();
    if (formats.contains("Animation")) {
        event->acceptProposedAction();
    }
}

void AnimationButton::enterEvent(QEvent* /*event*/){
    if (cursor().shape() != Qt::OpenHandCursor) {
        setCursor(Qt::OpenHandCursor);
    }
}

void AnimationButton::leaveEvent(QEvent* /*event*/){
    if (cursor().shape() == Qt::OpenHandCursor) {
        setCursor(Qt::ArrowCursor);
    }
}
