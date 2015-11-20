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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "ScriptTextEdit.h"
#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QDropEvent>
#include <QScrollBar>
#include <QTextBlock>
#include <QPainter>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/Node.h"
#include "Engine/Image.h"
#include "Engine/Settings.h"

#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h"

InputScriptTextEdit::InputScriptTextEdit(QWidget* parent)
: QPlainTextEdit(parent)
, _lineNumber(new LineNumberWidget(this))
{
    QObject::connect(this, SIGNAL(blockCountChanged(int)), this, SLOT(updateLineNumberAreaWidth(int)));
    QObject::connect(this, SIGNAL(updateRequest(QRect,int)), this, SLOT(updateLineNumberArea(QRect,int)));
    QObject::connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(highlightCurrentLine()));
}

InputScriptTextEdit::~InputScriptTextEdit()
{
    
}

int
InputScriptTextEdit::getLineNumberAreaWidth()
{
    int digits = 1;
    int max = std::max(1, blockCount());
    //Get the max number of digits
    while (max >= 10) {
        max /= 10;
        ++digits;
    }
    
    return  3 + fontMetrics().width(QLatin1Char('9')) * digits;
}

void
InputScriptTextEdit::resizeEvent(QResizeEvent *e)
{
    QPlainTextEdit::resizeEvent(e);
    
    QRect cr = contentsRect();
    _lineNumber->setGeometry(QRect(cr.left(), cr.top(), getLineNumberAreaWidth(), cr.height()));
}



void
InputScriptTextEdit::updateLineNumberAreaWidth(int /* newBlockCount */)
{
    setViewportMargins(getLineNumberAreaWidth(), 0, 0, 0);
}


void
InputScriptTextEdit::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy) {
        _lineNumber->scroll(0, dy);
    } else {
        _lineNumber->update(0, rect.y(), _lineNumber->width(), rect.height());
    }
    if (rect.contains(viewport()->rect())) {
        updateLineNumberAreaWidth(0);
    }
}

void
InputScriptTextEdit::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> extraSelections;
    
    if (!isReadOnly()) {
        QTextEdit::ExtraSelection selection;
        
        QColor lineColor = QColor(100,100,100);
        
        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }
    
    setExtraSelections(extraSelections);
}

void
InputScriptTextEdit::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    QPainter painter(_lineNumber);
    QColor fillColor;
    QColor txtColor;
    QColor selColor;
    {
        double r,g,b;
        appPTR->getCurrentSettings()->getRaisedColor(&r, &g, &b);
        fillColor.setRgbF(Natron::clamp(r, 0., 1.), Natron::clamp(g, 0., 1.), Natron::clamp(b, 0., 1.));
        appPTR->getCurrentSettings()->getTextColor(&r, &g, &b);
        txtColor.setRgbF(Natron::clamp(r, 0., 1.), Natron::clamp(g, 0., 1.), Natron::clamp(b, 0., 1.));
        appPTR->getCurrentSettings()->getSelectionColor(&r, &g, &b);
        selColor.setRgbF(Natron::clamp(r, 0., 1.), Natron::clamp(g, 0., 1.), Natron::clamp(b, 0., 1.));

    }
    
    painter.fillRect(event->rect(), fillColor);

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = (int) blockBoundingGeometry(block).translated(contentOffset()).top();
    int bottom = top + (int) blockBoundingRect(block).height();

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QString number = QString::number(blockNumber + 1);
            if (blockNumber == textCursor().block().blockNumber()) {
                painter.setPen(selColor);
            } else {
                painter.setPen(txtColor);
            }
            
            painter.drawText(0, top, _lineNumber->width(), fontMetrics().height(),
                             Qt::AlignRight, number);
        }
        
        block = block.next();
        top = bottom;
        bottom = top + (int) blockBoundingRect(block).height();
        ++blockNumber;
    }
}

void
InputScriptTextEdit::dragEnterEvent(QDragEnterEvent* e)
{
    
    QStringList formats = e->mimeData()->formats();
    if ( formats.contains("Animation") ) {
        setCursor(Qt::DragCopyCursor);
        e->acceptProposedAction();
    }
}

void
InputScriptTextEdit::dragMoveEvent(QDragMoveEvent* e)
{
    if (!acceptDrops()) {
        return;
    }
    QStringList formats = e->mimeData()->formats();
    
    if ( formats.contains("Animation") ) {
        e->acceptProposedAction();
    }
}


void
InputScriptTextEdit::dropEvent(QDropEvent* e)
{
    if (!acceptDrops()) {
        return;
    }
    QStringList formats = e->mimeData()->formats();
    if ( formats.contains("Animation") ) {
        std::list<Variant> values;
        std::list<boost::shared_ptr<Curve> > curves;
        std::list<boost::shared_ptr<Curve> > parametricCurves;
        std::map<int,std::string> stringAnimation;
        bool copyAnimation;
        
        std::string appID;
        std::string nodeFullyQualifiedName;
        std::string paramName;
        
        appPTR->getKnobClipBoard(&copyAnimation,&values,&curves,&stringAnimation,&parametricCurves,&appID,&nodeFullyQualifiedName,&paramName);
        
        QString toAppend;
        toAppend.append(nodeFullyQualifiedName.c_str());
        toAppend.append('.');
        toAppend.append(paramName.c_str());
        toAppend.append(".get()");
        if (values.size() > 1) {
            toAppend.append("[dimension]");
        }
        appendPlainText(toAppend);
        e->acceptProposedAction();
    }
    
}


void
InputScriptTextEdit::enterEvent(QEvent* /*e*/)
{
    if (acceptDrops() && cursor().shape() != Qt::OpenHandCursor) {
        setCursor(Qt::OpenHandCursor);
    }
}

void
InputScriptTextEdit::leaveEvent(QEvent* /*e*/)
{
    if (acceptDrops() && cursor().shape() == Qt::OpenHandCursor) {
        setCursor(Qt::ArrowCursor);
    }
}

OutputScriptTextEdit::OutputScriptTextEdit(QWidget* parent)
: QTextEdit(parent)
{
    
}

OutputScriptTextEdit::~OutputScriptTextEdit()
{
    
}

void
OutputScriptTextEdit::showEvent(QShowEvent* e)
{
    QTextEdit::showEvent(e);
    verticalScrollBar()->setValue(verticalScrollBar()->maximum());
    Q_EMIT userScrollChanged(true);
}

void
OutputScriptTextEdit::scrollContentsBy(int dx, int dy)
{
    QTextEdit::scrollContentsBy(dx, dy);
    QScrollBar* sb = verticalScrollBar();
    int v = sb->value();
    int max = sb->maximum();
    Q_EMIT userScrollChanged(v == max);
}
