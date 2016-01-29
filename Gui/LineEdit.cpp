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

#include "LineEdit.h"

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QLineEdit>
GCC_DIAG_UNUSED_PRIVATE_FIELD_OFF
// /opt/local/include/QtGui/qmime.h:119:10: warning: private field 'type' is not used [-Wunused-private-field]
#include <QDragEnterEvent>
GCC_DIAG_UNUSED_PRIVATE_FIELD_ON
#include <QDragLeaveEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QKeySequence>
#include <QUrl>
#include <QMimeData>
#include <QStyle>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Global/QtCompat.h"

NATRON_NAMESPACE_ENTER;

LineEdit::LineEdit(QWidget* parent)
    : QLineEdit(parent)
      , animation(0)
      , dirty(false)
      , altered(false)
{
    setAttribute(Qt::WA_MacShowFocusRect,0);
    connect( this, SIGNAL( editingFinished() ), this, SLOT( onEditingFinished() ) );
}

LineEdit::~LineEdit()
{
}

void
LineEdit::paintEvent(QPaintEvent* e)
{
    QPalette p = this->palette();
    QColor c(200,200,200,255);

    p.setColor( QPalette::Highlight, c );
    p.setColor( QPalette::HighlightedText, c );
    this->setPalette( p );
    QLineEdit::paintEvent(e);
}

void
LineEdit::dropEvent(QDropEvent* e)
{
    if ( !e->mimeData()->hasUrls() ) {
        return;
    }

    QList<QUrl> urls = e->mimeData()->urls();
    QString path;
    if (urls.size() > 0) {
        path = QtCompat::toLocalFileUrlFixed(urls.at(0)).path();
    }
    if ( !path.isEmpty() ) {
        setText(path);
        Q_EMIT textDropped();
    }
}

void
LineEdit::onEditingFinished()
{
    //clearFocus();
}

void
LineEdit::dragEnterEvent(QDragEnterEvent* e)
{
    e->accept();
}

void
LineEdit::dragMoveEvent(QDragMoveEvent* e)
{
    e->accept();
}

void
LineEdit::dragLeaveEvent(QDragLeaveEvent* e)
{
    e->accept();
}

void
LineEdit::setAnimation(int v)
{
    animation = v;
    style()->unpolish(this);
    style()->polish(this);
    update();
}

void
LineEdit::setDirty(bool b)
{
    dirty = b;
    style()->unpolish(this);
    style()->polish(this);
    update();
}

void
LineEdit::setAltered(bool b)
{
    altered = b;
    style()->unpolish(this);
    style()->polish(this);
    update();
}

void
LineEdit::keyPressEvent(QKeyEvent* e)
{
    QLineEdit::keyPressEvent(e);
    if (e->matches(QKeySequence::Paste)) {
        Q_EMIT textPasted();
    }
}

NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_LineEdit.cpp"
