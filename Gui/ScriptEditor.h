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


#ifndef SCRIPTEDITOR_H
#define SCRIPTEDITOR_H

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#endif

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QWidget>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/ScriptObject.h"

class Gui;

struct ScriptEditorPrivate;

class ScriptEditor : public QWidget, public ScriptObject
{
CLANG_DIAG_OFF(inconsistent-missing-override)
    Q_OBJECT
CLANG_DIAG_ON(inconsistent-missing-override)

public:
    
    ScriptEditor(Gui* gui);
    
    virtual ~ScriptEditor();
    
    void setInputScript(const QString& script);
    
    QString getInputScript() const;
    
    QString getAutoSavedScript() const;
    
    void appendToScriptEditor(const QString& str);
    
    void printAutoDeclaredVariable(const QString& str);
    
public Q_SLOTS:

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

private:
    
    virtual void keyPressEvent(QKeyEvent* e) OVERRIDE FINAL;
    
    boost::scoped_ptr<ScriptEditorPrivate> _imp;
};


#endif // SCRIPTEDITOR_H
