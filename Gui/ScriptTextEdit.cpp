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

#include "Engine/Node.h"

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QStyle>
#include <QDropEvent>
#include <QScrollBar>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h"

ScriptTextEdit::ScriptTextEdit(QWidget* parent)
: QTextEdit(parent)
, isOutput(false)
{
    
}

ScriptTextEdit::~ScriptTextEdit()
{
    
}

void
ScriptTextEdit::setOutput(bool o)
{
    isOutput = o;
    style()->unpolish(this);
    style()->polish(this);
    update();
}

bool
ScriptTextEdit::getOutput() const
{
    return isOutput;
}

void
ScriptTextEdit::dragEnterEvent(QDragEnterEvent* e)
{
    
    QStringList formats = e->mimeData()->formats();
    if ( formats.contains("Animation") ) {
        setCursor(Qt::DragCopyCursor);
        e->acceptProposedAction();
    }
}

void
ScriptTextEdit::dragMoveEvent(QDragMoveEvent* e)
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
ScriptTextEdit::dropEvent(QDropEvent* e)
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
        append(toAppend);
        e->acceptProposedAction();
    }
    
}


void
ScriptTextEdit::enterEvent(QEvent* /*e*/)
{
    if (acceptDrops() && cursor().shape() != Qt::OpenHandCursor) {
        setCursor(Qt::OpenHandCursor);
    }
}

void
ScriptTextEdit::leaveEvent(QEvent* /*e*/)
{
    if (acceptDrops() && cursor().shape() == Qt::OpenHandCursor) {
        setCursor(Qt::ArrowCursor);
    }
}

void
ScriptTextEdit::showEvent(QShowEvent* e)
{
    QTextEdit::showEvent(e);
    verticalScrollBar()->setValue(verticalScrollBar()->maximum());
    Q_EMIT userScrollChanged(true);
}

void
ScriptTextEdit::scrollContentsBy(int dx, int dy)
{
    QTextEdit::scrollContentsBy(dx, dy);
    QScrollBar* sb = verticalScrollBar();
    int v = sb->value();
    int max = sb->maximum();
    Q_EMIT userScrollChanged(v == max);
}
