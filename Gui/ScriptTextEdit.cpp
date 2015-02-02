//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "ScriptTextEdit.h"

#include "Engine/Node.h"

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QStyle>
#include <QDropEvent>
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
    repaint();
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
        
        QString toAppend("app.");
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

