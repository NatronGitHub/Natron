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
#ifndef SCRIPTTEXTEDIT_H
#define SCRIPTTEXTEDIT_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QTextEdit>
#include <QPlainTextEdit>
#include <QSyntaxHighlighter>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)
#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#endif



struct PySyntaxHighlighterPrivate;
class PySyntaxHighlighter : public QSyntaxHighlighter
{

public:
    
    PySyntaxHighlighter(QTextDocument *parent = 0);
    
    virtual ~PySyntaxHighlighter();
    
    void reload();
    
private:
    
    virtual void highlightBlock(const QString &text) OVERRIDE FINAL;
    bool matchMultiline(const QString &text, const QRegExp &delimiter, const int inState, const QTextCharFormat &style);

    boost::scoped_ptr<PySyntaxHighlighterPrivate> _imp;
    
};

class LineNumberWidget;
class InputScriptTextEdit : public QPlainTextEdit
{
    
    GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
    GCC_DIAG_SUGGEST_OVERRIDE_ON
    
    
    friend class LineNumberWidget;
    
public:
    
    InputScriptTextEdit(QWidget* parent);
    
    virtual ~InputScriptTextEdit();

    int getLineNumberAreaWidth();
    
    void reloadHighlighter();
    
private Q_SLOTS:
    
    void updateLineNumberAreaWidth(int newBlockCount);
    void highlightCurrentLine();
    void updateLineNumberArea(const QRect &, int);
    
private:
    
    void lineNumberAreaPaintEvent(QPaintEvent *event);
        
    virtual void dragEnterEvent(QDragEnterEvent* e) OVERRIDE FINAL;
    
    virtual void dropEvent(QDropEvent* e) OVERRIDE FINAL;
    
    virtual void resizeEvent(QResizeEvent *event) OVERRIDE FINAL;
    
    virtual void dragMoveEvent(QDragMoveEvent* e) OVERRIDE FINAL;
    
    virtual void enterEvent(QEvent* e) OVERRIDE FINAL;
    
    virtual void leaveEvent(QEvent* e) OVERRIDE FINAL;
  
    
private:
    
    LineNumberWidget* _lineNumber;
    PySyntaxHighlighter* _highlighter;
};

class LineNumberWidget : public QWidget
{
public:
    
    LineNumberWidget(InputScriptTextEdit *editor)
    : QWidget(editor)
    , codeEditor(editor)
    {
        codeEditor = editor;
    }
    
    virtual QSize sizeHint() const OVERRIDE FINAL {
        return QSize(codeEditor->getLineNumberAreaWidth(), 0);
    }
    
protected:
    
    virtual void paintEvent(QPaintEvent *event) OVERRIDE FINAL {
        codeEditor->lineNumberAreaPaintEvent(event);
    }
    
private:
    InputScriptTextEdit *codeEditor;
};

class OutputScriptTextEdit : public QTextEdit
{
    GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
    GCC_DIAG_SUGGEST_OVERRIDE_ON
    
public:
    
    OutputScriptTextEdit(QWidget* parent);
    
    virtual ~OutputScriptTextEdit();
    
Q_SIGNALS:
    
    void userScrollChanged(bool atBottom);
    
private:
    
    virtual void showEvent(QShowEvent* e) OVERRIDE FINAL;
    
    virtual void scrollContentsBy(int dx, int dy) OVERRIDE FINAL;
};

#endif // SCRIPTTEXTEDIT_H
