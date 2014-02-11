//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012. 
*contact: immarespond at gmail dot com
*
*/

#include "LineEdit.h"

#include <QLineEdit>
CLANG_DIAG_OFF(unused-private-field)
// /opt/local/include/QtGui/qmime.h:119:10: warning: private field 'type' is not used [-Wunused-private-field]
#include <QDragEnterEvent>
CLANG_DIAG_ON(unused-private-field)
#include <QDragLeaveEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QUrl>
#include <QMimeData>
#include <QStyle>

class QPaintEvent;

LineEdit::LineEdit(QWidget* parent)
: QLineEdit(parent)
{
    setAttribute(Qt::WA_MacShowFocusRect,0);
}

LineEdit::~LineEdit()
{
}
    
void LineEdit::paintEvent(QPaintEvent* e) {
    QPalette p = this->palette();
    QColor c(200,200,200,255);
    p.setColor( QPalette::Highlight, c );
    p.setColor( QPalette::HighlightedText, c );
    this->setPalette( p );
    QLineEdit::paintEvent(e);
}

void LineEdit::dropEvent(QDropEvent* event) {
    if(!event->mimeData()->hasUrls())
        return;

    QList<QUrl> urls = event->mimeData()->urls();
    QString path;
    if (urls.size() > 0) {
        path = urls.at(0).path();
    }
    if (!path.isEmpty()) {
        setText(path);

    }

}

void LineEdit::dragEnterEvent(QDragEnterEvent *ev) {
    ev->accept();
}

void LineEdit::dragMoveEvent(QDragMoveEvent* e) {
    e->accept();
}

void LineEdit::dragLeaveEvent(QDragLeaveEvent* e) {
    e->accept();
}

void LineEdit::setAnimation(int v) {
    animation = v;
    style()->unpolish(this);
    style()->polish(this);
    repaint();
}
