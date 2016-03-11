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

#ifndef SCRIPTEDITOR_H
#define SCRIPTEDITOR_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#endif

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QWidget>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Gui/PanelWidget.h"
#include "Gui/GuiFwd.h"

NATRON_NAMESPACE_ENTER;

struct ScriptEditorPrivate;

class ScriptEditor : public QWidget, public PanelWidget
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:
    
    ScriptEditor(Gui* gui);
    
    virtual ~ScriptEditor();
    
    void setInputScript(const QString& script);
    
    QString getInputScript() const;
    
    QString getAutoSavedScript() const;
    
    void appendToScriptEditor(const QString& str);
    
    void printAutoDeclaredVariable(const QString& str);
    
    void reloadHighlighter();
    
public Q_SLOTS:
    
    void doAppendToScriptEditorOnMainThread(const QString& str);
    
    void onUserScrollChanged(bool atBottom);

    void onShowHideOutputClicked(bool clicked);
    
    void onClearOutputClicked();
    
    void onClearHistoryClicked();
    
    void onUndoClicked();
    
    void onRedoClicked();
    
    void onSourceScriptClicked();
    
    void onLoadScriptClicked();
    
    void onSaveScriptClicked();
    
    void onExecScriptClicked();
    
    void onHistoryCanUndoChanged(bool canUndo);
    void onHistoryCanRedoChanged(bool canRedo);
    
    void onInputScriptTextChanged();
    
    void onAutoSaveTimerTimedOut();
    
    void onShowAutoDeclVarsClicked(bool clicked);

Q_SIGNALS:
    
    void appendToScriptEditorOnMainThread(QString);
    
private:
    
    virtual void mousePressEvent(QMouseEvent* e) OVERRIDE FINAL;
    virtual void enterEvent(QEvent *e) OVERRIDE FINAL;
    virtual void leaveEvent(QEvent *e) OVERRIDE FINAL;
    virtual void keyPressEvent(QKeyEvent* e) OVERRIDE FINAL;
    virtual void keyReleaseEvent(QKeyEvent* e) OVERRIDE FINAL;
    
    boost::scoped_ptr<ScriptEditorPrivate> _imp;
};

NATRON_NAMESPACE_EXIT;

#endif // SCRIPTEDITOR_H
