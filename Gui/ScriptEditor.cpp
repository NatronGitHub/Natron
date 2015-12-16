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

#include "ScriptEditor.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QUndoStack>
#include <QUndoCommand>
#include <QFile>
#include <QtCore/QTextStream>
#include <QTextCursor>
#include <QThread>
#include <QApplication>
#include <QSplitter>
#include <QTimer>
#include <QMutex>
#include <QScrollBar>
#include <QKeyEvent>

#include "Gui/GuiApplicationManager.h"
#include "Gui/Gui.h"
#include "Gui/Button.h"
#include "Gui/GuiDefines.h"
#include "Gui/GuiMacros.h"
#include "Gui/SequenceFileDialog.h"
#include "Gui/ScriptTextEdit.h"
#include "Gui/Utils.h"
#include "Gui/ActionShortcuts.h"

#include "Engine/Settings.h"

struct ScriptEditorPrivate
{
    
    QVBoxLayout* mainLayout;
    
    QWidget* buttonsContainer;
    QHBoxLayout* buttonsContainerLayout;
    
    Button* undoB;
    Button* redoB;
    Button* clearHistoB;
    Button* sourceScriptB;
    Button* loadScriptB;
    Button* saveScriptB;
    Button* execScriptB;
    Button* showHideOutputB;
    Button* clearOutputB;
    Button* showAutoDeclVarsB;
    
    OutputScriptTextEdit* outputEdit;
    InputScriptTextEdit* inputEdit;
    
    QUndoStack history;
    
    QTimer autoSaveTimer;
    QMutex autoSavedScriptMutex;
    QString autoSavedScript;
    
    ///Indicate whether we should auto-scroll as results are printed or not
    bool outputAtBottom;
    
    ScriptEditorPrivate()
    : mainLayout(0)
    , buttonsContainer(0)
    , buttonsContainerLayout(0)
    , undoB(0)
    , redoB(0)
    , clearHistoB(0)
    , sourceScriptB(0)
    , loadScriptB(0)
    , saveScriptB(0)
    , execScriptB(0)
    , showHideOutputB(0)
    , clearOutputB(0)
    , showAutoDeclVarsB(0)
    , outputEdit(0)
    , inputEdit(0)
    , autoSaveTimer()
    , autoSavedScriptMutex()
    , autoSavedScript()
    , outputAtBottom(true)
    {
        
    }
};

ScriptEditor::ScriptEditor(Gui* gui)
: QWidget(gui)
, PanelWidget(this,gui)
, _imp(new ScriptEditorPrivate())
{
    _imp->mainLayout = new QVBoxLayout(this);
    _imp->buttonsContainer = new QWidget(this);
    _imp->buttonsContainerLayout = new QHBoxLayout(_imp->buttonsContainer);
    _imp->buttonsContainerLayout->setContentsMargins(0, 0, 0, 0);
    _imp->buttonsContainerLayout->setSpacing(2);
    
    QPixmap undoPix,redoPix,clearHistoPix,sourceScriptPix,loadScriptPix,saveScriptPix,execScriptPix,outputVisiblePix,outputHiddenPix,clearOutpoutPix;
    appPTR->getIcon(Natron::NATRON_PIXMAP_SCRIPT_PREVIOUS_SCRIPT, NATRON_MEDIUM_BUTTON_ICON_SIZE, &undoPix);
    appPTR->getIcon(Natron::NATRON_PIXMAP_SCRIPT_NEXT_SCRIPT, NATRON_MEDIUM_BUTTON_ICON_SIZE, &redoPix);
    appPTR->getIcon(Natron::NATRON_PIXMAP_CLOSE_PANEL, NATRON_MEDIUM_BUTTON_ICON_SIZE, &clearHistoPix);
    appPTR->getIcon(Natron::NATRON_PIXMAP_CLEAR_ALL_ANIMATION, NATRON_MEDIUM_BUTTON_ICON_SIZE, &clearHistoPix);
    appPTR->getIcon(Natron::NATRON_PIXMAP_SCRIPT_CLEAR_OUTPUT, NATRON_MEDIUM_BUTTON_ICON_SIZE, &clearOutpoutPix);
    appPTR->getIcon(Natron::NATRON_PIXMAP_SCRIPT_EXEC_SCRIPT, NATRON_MEDIUM_BUTTON_ICON_SIZE, &execScriptPix);
    appPTR->getIcon(Natron::NATRON_PIXMAP_SCRIPT_OUTPUT_PANE_ACTIVATED, NATRON_MEDIUM_BUTTON_ICON_SIZE, &outputVisiblePix);
    appPTR->getIcon(Natron::NATRON_PIXMAP_SCRIPT_OUTPUT_PANE_DEACTIVATED, NATRON_MEDIUM_BUTTON_ICON_SIZE, &outputHiddenPix);
    appPTR->getIcon(Natron::NATRON_PIXMAP_SCRIPT_LOAD_EXEC_SCRIPT, NATRON_MEDIUM_BUTTON_ICON_SIZE, &sourceScriptPix);
    appPTR->getIcon(Natron::NATRON_PIXMAP_SCRIPT_LOAD_SCRIPT, NATRON_MEDIUM_BUTTON_ICON_SIZE, &loadScriptPix);
    appPTR->getIcon(Natron::NATRON_PIXMAP_SCRIPT_SAVE_SCRIPT, NATRON_MEDIUM_BUTTON_ICON_SIZE, &saveScriptPix);
    
    _imp->undoB = new Button(QIcon(undoPix),"",_imp->buttonsContainer);
    QKeySequence undoSeq(Qt::CTRL + Qt::Key_BracketLeft);
    _imp->undoB->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    _imp->undoB->setIconSize(QSize(NATRON_MEDIUM_BUTTON_ICON_SIZE, NATRON_MEDIUM_BUTTON_ICON_SIZE));
    _imp->undoB->setFocusPolicy(Qt::NoFocus);
    setTooltipWithShortcut(kShortcutGroupScriptEditor, kShortcutIDActionScriptEditorPrevScript,"<p>" + tr("Previous Script") + "</p>" +
                           "<p><b>" + tr("Keyboard shortcut") + ": %1</b></p>", _imp->undoB);
    _imp->undoB->setEnabled(false);
    QObject::connect(_imp->undoB, SIGNAL(clicked(bool)), this, SLOT(onUndoClicked()));
    
    _imp->redoB = new Button(QIcon(redoPix),"",_imp->buttonsContainer);
    QKeySequence redoSeq(Qt::CTRL + Qt::Key_BracketRight);
    _imp->redoB->setFocusPolicy(Qt::NoFocus);
    _imp->redoB->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    _imp->redoB->setIconSize(QSize(NATRON_MEDIUM_BUTTON_ICON_SIZE, NATRON_MEDIUM_BUTTON_ICON_SIZE));
    setTooltipWithShortcut(kShortcutGroupScriptEditor, kShortcutIDActionScriptEditorNextScript,"<p>" + tr("Next Script") + "</p>" +
                           "<p><b>" + tr("Keyboard shortcut") + ": %1</b></p>", _imp->redoB);
    _imp->redoB->setEnabled(false);
    QObject::connect(_imp->redoB, SIGNAL(clicked(bool)), this, SLOT(onRedoClicked()));
    
    _imp->clearHistoB = new Button(QIcon(clearHistoPix),"",_imp->buttonsContainer);
    _imp->clearHistoB->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    _imp->clearHistoB->setIconSize(QSize(NATRON_MEDIUM_BUTTON_ICON_SIZE, NATRON_MEDIUM_BUTTON_ICON_SIZE));
    _imp->clearHistoB->setFocusPolicy(Qt::NoFocus);
    setTooltipWithShortcut(kShortcutGroupScriptEditor, kShortcutIDActionScriptEditorClearHistory,"<p>" + tr("Clear History") + "</p>" +
                           "<p><b>" + tr("Keyboard shortcut") + ": %1</b></p>", _imp->clearHistoB);

    QObject::connect(_imp->clearHistoB, SIGNAL(clicked(bool)), this, SLOT(onClearHistoryClicked()));
    
    _imp->sourceScriptB = new Button(QIcon(sourceScriptPix),"",_imp->buttonsContainer);
    _imp->sourceScriptB->setToolTip(Natron::convertFromPlainText(tr("Open and execute a script."), Qt::WhiteSpaceNormal));
    _imp->sourceScriptB->setFocusPolicy(Qt::NoFocus);
    _imp->sourceScriptB->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    _imp->sourceScriptB->setIconSize(QSize(NATRON_MEDIUM_BUTTON_ICON_SIZE, NATRON_MEDIUM_BUTTON_ICON_SIZE));
    QObject::connect(_imp->sourceScriptB, SIGNAL(clicked(bool)), this, SLOT(onSourceScriptClicked()));
    
    _imp->loadScriptB = new Button(QIcon(loadScriptPix),"",_imp->buttonsContainer);
    _imp->loadScriptB->setToolTip(Natron::convertFromPlainText(tr("Open a script without executing it."), Qt::WhiteSpaceNormal));
    _imp->loadScriptB->setFocusPolicy(Qt::NoFocus);
    _imp->loadScriptB->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    _imp->loadScriptB->setIconSize(QSize(NATRON_MEDIUM_BUTTON_ICON_SIZE, NATRON_MEDIUM_BUTTON_ICON_SIZE));
    QObject::connect(_imp->loadScriptB, SIGNAL(clicked(bool)), this, SLOT(onLoadScriptClicked()));
    
    _imp->saveScriptB = new Button(QIcon(saveScriptPix),"",_imp->buttonsContainer);
    _imp->saveScriptB->setToolTip(Natron::convertFromPlainText(tr("Save the current script."), Qt::WhiteSpaceNormal));
    _imp->saveScriptB->setFocusPolicy(Qt::NoFocus);
    _imp->saveScriptB->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    _imp->saveScriptB->setIconSize(QSize(NATRON_MEDIUM_BUTTON_ICON_SIZE, NATRON_MEDIUM_BUTTON_ICON_SIZE));
    QObject::connect(_imp->saveScriptB, SIGNAL(clicked(bool)), this, SLOT(onSaveScriptClicked()));
    
    _imp->execScriptB = new Button(QIcon(execScriptPix),"",_imp->buttonsContainer);
    QKeySequence execSeq(Qt::CTRL + Qt::Key_Return);
    _imp->execScriptB->setFocusPolicy(Qt::NoFocus);
    _imp->execScriptB->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    _imp->execScriptB->setIconSize(QSize(NATRON_MEDIUM_BUTTON_ICON_SIZE, NATRON_MEDIUM_BUTTON_ICON_SIZE));
    setTooltipWithShortcut(kShortcutGroupScriptEditor, kShortcutIDActionScriptExecScript,"<p>" + tr("Execute the current script") + "</p>" +
                           "<p><b>" + tr("Keyboard shortcut") + ": %1</b></p>", _imp->execScriptB);

    QObject::connect(_imp->execScriptB, SIGNAL(clicked(bool)), this, SLOT(onExecScriptClicked()));
    
    QIcon icShowHide;
    icShowHide.addPixmap(outputVisiblePix,QIcon::Normal,QIcon::On);
    icShowHide.addPixmap(outputHiddenPix,QIcon::Normal,QIcon::Off);
    _imp->showHideOutputB = new Button(icShowHide,"",_imp->buttonsContainer);
    setTooltipWithShortcut(kShortcutGroupScriptEditor, kShortcutIDActionScriptShowOutput,"<p>" + tr("Show/Hide the output area") + "</p>" +
                           "<p><b>" + tr("Keyboard shortcut") + ": %1</b></p>", _imp->showHideOutputB);
    _imp->showHideOutputB->setFocusPolicy(Qt::NoFocus);
    _imp->showHideOutputB->setCheckable(true);
    _imp->showHideOutputB->setChecked(true);
    _imp->showHideOutputB->setDown(true);
    _imp->showHideOutputB->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    _imp->showHideOutputB->setIconSize(QSize(NATRON_MEDIUM_BUTTON_ICON_SIZE, NATRON_MEDIUM_BUTTON_ICON_SIZE));
    QObject::connect(_imp->showHideOutputB, SIGNAL(clicked(bool)), this, SLOT(onShowHideOutputClicked(bool)));
    
    _imp->clearOutputB = new Button(QIcon(clearOutpoutPix),"",_imp->buttonsContainer);
    QKeySequence clearSeq(Qt::CTRL + Qt::Key_Backspace);
    _imp->clearOutputB->setFocusPolicy(Qt::NoFocus);
    _imp->clearOutputB->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    _imp->clearOutputB->setIconSize(QSize(NATRON_MEDIUM_BUTTON_ICON_SIZE, NATRON_MEDIUM_BUTTON_ICON_SIZE));
    setTooltipWithShortcut(kShortcutGroupScriptEditor, kShortcutIDActionScriptClearOutput,"<p>" + tr("Clear the output area") + "</p>" +
                           "<p><b>" + tr("Keyboard shortcut") + ": %1</b></p>", _imp->clearOutputB);
    QObject::connect(_imp->clearOutputB, SIGNAL(clicked(bool)), this, SLOT(onClearOutputClicked()));
    
    _imp->showAutoDeclVarsB = new Button(QIcon(),"...",_imp->buttonsContainer);
    _imp->showAutoDeclVarsB->setFocusPolicy(Qt::NoFocus);
    _imp->showAutoDeclVarsB->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    _imp->showAutoDeclVarsB->setIconSize(QSize(NATRON_MEDIUM_BUTTON_ICON_SIZE, NATRON_MEDIUM_BUTTON_ICON_SIZE));
    _imp->showAutoDeclVarsB->setCheckable(true);
    bool isAutoDeclEnabled = appPTR->getCurrentSettings()->isAutoDeclaredVariablePrintActivated();
    _imp->showAutoDeclVarsB->setChecked(isAutoDeclEnabled);
    _imp->showAutoDeclVarsB->setDown(isAutoDeclEnabled);
    _imp->showAutoDeclVarsB->setToolTip(Natron::convertFromPlainText(tr("When checked, auto-declared Python variable will be printed "
                                                                        "in gray in the output window."), Qt::WhiteSpaceNormal));
    QObject::connect(_imp->showAutoDeclVarsB, SIGNAL(clicked(bool)), this, SLOT(onShowAutoDeclVarsClicked(bool)));
    
    _imp->buttonsContainerLayout->addWidget(_imp->undoB);
    _imp->buttonsContainerLayout->addWidget(_imp->redoB);
    _imp->buttonsContainerLayout->addWidget(_imp->clearHistoB);
    _imp->buttonsContainerLayout->addSpacing(10);
    
    _imp->buttonsContainerLayout->addWidget(_imp->sourceScriptB);
    _imp->buttonsContainerLayout->addWidget(_imp->loadScriptB);
    _imp->buttonsContainerLayout->addWidget(_imp->saveScriptB);
    _imp->buttonsContainerLayout->addWidget(_imp->execScriptB);
    _imp->buttonsContainerLayout->addSpacing(10);
    
    _imp->buttonsContainerLayout->addWidget(_imp->showHideOutputB);
    _imp->buttonsContainerLayout->addWidget(_imp->clearOutputB);
    _imp->buttonsContainerLayout->addSpacing(10);
    
    _imp->buttonsContainerLayout->addWidget(_imp->showAutoDeclVarsB);
    _imp->buttonsContainerLayout->addStretch();
    
    QSplitter* splitter = new QSplitter(Qt::Vertical,this);
    
    QFont scriptFont("Courier New", 12);
    if (!scriptFont.exactMatch()) {
        scriptFont = font();
    }
    
    _imp->outputEdit = new OutputScriptTextEdit(this);
    QObject::connect(_imp->outputEdit, SIGNAL(userScrollChanged(bool)), this, SLOT(onUserScrollChanged(bool)));
    _imp->outputEdit->setFocusPolicy(Qt::ClickFocus);
    _imp->outputEdit->setReadOnly(true);
    _imp->outputEdit->setFont(scriptFont);
    
    _imp->inputEdit = new InputScriptTextEdit(this);
    QObject::connect(_imp->inputEdit, SIGNAL(textChanged()), this, SLOT(onInputScriptTextChanged()));
    _imp->inputEdit->setFont(scriptFont);
    QFontMetrics fm = _imp->inputEdit->fontMetrics();
    _imp->inputEdit->setTabStopWidth(fm.width(' ') * 4);
    _imp->outputEdit->setTabStopWidth(fm.width(' ') * 4);
    
    _imp->mainLayout->addWidget(_imp->buttonsContainer);
    splitter->addWidget(_imp->outputEdit);
    splitter->addWidget(_imp->inputEdit);
    _imp->mainLayout->addWidget(splitter);
    
    QObject::connect(&_imp->history, SIGNAL(canUndoChanged(bool)), this, SLOT(onHistoryCanUndoChanged(bool)));
    QObject::connect(&_imp->history, SIGNAL(canRedoChanged(bool)), this, SLOT(onHistoryCanRedoChanged(bool)));
    
    _imp->autoSaveTimer.setSingleShot(true);
}

ScriptEditor::~ScriptEditor()
{
    
}

void
ScriptEditor::reloadHighlighter()
{
    if (_imp->inputEdit) {
        _imp->inputEdit->reloadHighlighter();
    }
}

void
ScriptEditor::onHistoryCanUndoChanged(bool canUndo)
{
    _imp->undoB->setEnabled(canUndo);
}

void
ScriptEditor::onHistoryCanRedoChanged(bool canRedo)
{
    _imp->redoB->setEnabled(canRedo);
}

void
ScriptEditor::onShowHideOutputClicked(bool clicked)
{
    _imp->showHideOutputB->setDown(clicked);
    _imp->outputEdit->setVisible(clicked);
    
}

void
ScriptEditor::onClearOutputClicked()
{
    _imp->outputEdit->clear();
}

void
ScriptEditor::onClearHistoryClicked()
{
    _imp->inputEdit->clear();
    _imp->history.clear();
}

void
ScriptEditor::onUndoClicked()
{
    _imp->history.undo();
}

void
ScriptEditor::onRedoClicked()
{
    _imp->history.redo();
}

void
ScriptEditor::onSourceScriptClicked()
{
    std::vector<std::string> filters;
    filters.push_back("py");
    SequenceFileDialog dialog(this,filters,false,SequenceFileDialog::eFileDialogModeOpen,getGui()->getLastLoadProjectDirectory().toStdString(),
                              getGui(),false);
    
    if (dialog.exec()) {
        
        QDir currentDir = dialog.currentDirectory();
        getGui()->updateLastOpenedProjectPath(currentDir.absolutePath());

        QString fileName(dialog.selectedFiles().c_str());
        QFile file(fileName);
        if (file.open(QIODevice::ReadOnly)) {
            QTextStream ts(&file);
            QString content = ts.readAll();
            _imp->inputEdit->setPlainText(content);
            onExecScriptClicked();
        } else {
            Natron::errorDialog(tr("Operation failed").toStdString(), tr("Failure to open the file").toStdString());
        }
        
    }
}

void
ScriptEditor::onLoadScriptClicked()
{
    std::vector<std::string> filters;
    filters.push_back("py");
    SequenceFileDialog dialog(this,filters,false,SequenceFileDialog::eFileDialogModeOpen,getGui()->getLastLoadProjectDirectory().toStdString(),
                              getGui(),false);
    
    if (dialog.exec()) {
        
        QDir currentDir = dialog.currentDirectory();
        getGui()->updateLastOpenedProjectPath(currentDir.absolutePath());
        QString fileName(dialog.selectedFiles().c_str());
        QFile file(fileName);
        if (file.open(QIODevice::ReadOnly)) {
            QTextStream ts(&file);
            QString content = ts.readAll();
            _imp->inputEdit->setPlainText(content);
        } else {
            Natron::errorDialog(tr("Operation failed").toStdString(), tr("Failure to open the file").toStdString());
        }
        
    }
}

void
ScriptEditor::onSaveScriptClicked()
{
    std::vector<std::string> filters;
    filters.push_back("py");
    SequenceFileDialog dialog(this,filters,false,SequenceFileDialog::eFileDialogModeSave,getGui()->getLastSaveProjectDirectory().toStdString(),
                              getGui(),false);
    
    if (dialog.exec()) {
        
        QDir currentDir = dialog.currentDirectory();
        getGui()->updateLastSavedProjectPath(currentDir.absolutePath());

        QString fileName(dialog.selectedFiles().c_str());
        QFile file(fileName);
        if (file.open(QIODevice::ReadWrite)) {
            QTextStream ts(&file);
            ts << _imp->inputEdit->toPlainText();
        } else {
            Natron::errorDialog(tr("Operation failed").toStdString(), tr("Failure to save the file").toStdString());
        }
        
    }
}


class InputScriptCommand: public QUndoCommand
{
    
    ScriptEditor* _editor;
    QString _script;
    bool _firstRedoCalled;
    
public:
    
    InputScriptCommand(ScriptEditor* editor,  const QString& script);
    
    void redo();
    
    void undo();
};


InputScriptCommand::InputScriptCommand(ScriptEditor* editor, const QString& script)
: QUndoCommand()
, _editor(editor)
, _script(script)
, _firstRedoCalled(false)
{
    
}

void
InputScriptCommand::redo()
{
    if (_firstRedoCalled) {
        _editor->setInputScript(_script);
    }
    _firstRedoCalled = true;
    setText(QObject::tr("Exec script"));
}

void
InputScriptCommand::undo()
{
    _editor->setInputScript(_script);
   setText(QObject::tr("Exec script"));
}

void
ScriptEditor::onExecScriptClicked()
{
    QString allText = _imp->inputEdit->toPlainText();
    QTextCursor curs = _imp->inputEdit->textCursor();
    QString selectedText;
    if (curs.hasSelection()) {
        int selStart = curs.selectionStart();
        int selEnd = curs.selectionEnd();
        assert(selStart >= 0 && selStart < allText.size() &&
               selEnd >= 0 && selEnd <= allText.size());
        for (int i = selStart; i < selEnd; ++i) {
            selectedText.push_back(allText[i]);
        }
    }
    QString script;
    if (!selectedText.isEmpty()) {
        script = selectedText;
    } else {
        script = allText;
    }
    std::string error,output;
    
    if (!Natron::interpretPythonScript(script.toStdString(), &error, &output)) {
        _imp->outputEdit->append(Natron::convertFromPlainText(error.c_str(),Qt::WhiteSpaceNormal));
    } else {
        QString toAppend(script);
        if (!output.empty()) {
            toAppend.append("\n#Result:\n");
            toAppend.append(output.c_str());
        }
        _imp->outputEdit->append(toAppend);
        if (_imp->outputAtBottom) {
            _imp->outputEdit->verticalScrollBar()->setValue(_imp->outputEdit->verticalScrollBar()->maximum());
        }
        _imp->history.push(new InputScriptCommand(this,script));
        //_imp->inputEdit->clear();
    }
    
}

void
ScriptEditor::setInputScript(const QString& script)
{
    _imp->inputEdit->setPlainText(script);
}

QString
ScriptEditor::getInputScript() const
{
    assert(QThread::currentThread() == qApp->thread());
    
    return _imp->inputEdit->toPlainText();
}

void
ScriptEditor::mousePressEvent(QMouseEvent* e)
{
    takeClickFocus();
    QWidget::mousePressEvent(e);
}

void
ScriptEditor::keyPressEvent(QKeyEvent* e)
{

    bool accept = true;
    
    Qt::Key key = (Qt::Key)e->key();
    Qt::KeyboardModifiers modifiers = e->modifiers();
    
    if (isKeybind(kShortcutGroupScriptEditor, kShortcutIDActionScriptEditorPrevScript, modifiers, key)) {
        onUndoClicked();
    } else if (isKeybind(kShortcutGroupScriptEditor, kShortcutIDActionScriptEditorNextScript, modifiers, key)) {
        onRedoClicked();
    } else if (isKeybind(kShortcutGroupScriptEditor, kShortcutIDActionScriptExecScript, modifiers, key)) {
        onExecScriptClicked();
    } else if (isKeybind(kShortcutGroupScriptEditor, kShortcutIDActionScriptClearOutput, modifiers, key)) {
        onClearOutputClicked();
    } else if (isKeybind(kShortcutGroupScriptEditor, kShortcutIDActionScriptEditorClearHistory, modifiers, key)) {
        onClearHistoryClicked();
    } else if (isKeybind(kShortcutGroupScriptEditor, kShortcutIDActionScriptShowOutput, modifiers, key)) {
        onShowHideOutputClicked(!_imp->showHideOutputB->isChecked());
    } else {
        accept = false;
    }
    if (accept) {
        takeClickFocus();
        e->accept();
    } else {
        handleUnCaughtKeyPressEvent(e);
        QWidget::keyPressEvent(e);
    }
}

void
ScriptEditor::keyReleaseEvent(QKeyEvent* e)
{
    handleUnCaughtKeyPressEvent(e);
    QWidget::keyReleaseEvent(e);
}

void
ScriptEditor::onInputScriptTextChanged()
{
    if (!_imp->autoSaveTimer.isActive()) {
        _imp->autoSaveTimer.singleShot(5000, this, SLOT(onAutoSaveTimerTimedOut()));
    }
}

void
ScriptEditor::onAutoSaveTimerTimedOut()
{
    QMutexLocker k(&_imp->autoSavedScriptMutex);
    _imp->autoSavedScript = _imp->inputEdit->toPlainText();
}

QString
ScriptEditor::getAutoSavedScript() const
{
    QMutexLocker k(&_imp->autoSavedScriptMutex);
    return _imp->autoSavedScript;
}

void
ScriptEditor::doAppendToScriptEditorOnMainThread(const QString& str)
{
    assert(QThread::currentThread() == qApp->thread());
    appendToScriptEditor(str);
}

void
ScriptEditor::appendToScriptEditor(const QString& str)
{
    if (QThread::currentThread() != qApp->thread()) {
        appendToScriptEditorOnMainThread(str);
        return;
    }
    
    _imp->outputEdit->append(str + "\n");
    if (_imp->outputAtBottom) {
        _imp->outputEdit->verticalScrollBar()->setValue(_imp->outputEdit->verticalScrollBar()->maximum());
    }
}

void
ScriptEditor::printAutoDeclaredVariable(const QString& str)
{
    if (!appPTR->getCurrentSettings()->isAutoDeclaredVariablePrintActivated()) {
        return;
    }
    QString cpy = str;
    cpy.replace("\n", "<br/>");
    _imp->outputEdit->append("<font color=grey><b>" + cpy + "</font></b>");
    if (_imp->outputAtBottom) {
        _imp->outputEdit->verticalScrollBar()->setValue(_imp->outputEdit->verticalScrollBar()->maximum());
    }
}

void
ScriptEditor::enterEvent(QEvent *e)
{
    enterEventBase();
    QWidget::enterEvent(e);
}

void
ScriptEditor::leaveEvent(QEvent *e)
{
    leaveEventBase();
    QWidget::leaveEvent(e);
}

void
ScriptEditor::onUserScrollChanged(bool atBottom)
{
    _imp->outputAtBottom = atBottom;
}

void
ScriptEditor::onShowAutoDeclVarsClicked(bool clicked)
{
    _imp->showAutoDeclVarsB->setDown(clicked);
    _imp->showAutoDeclVarsB->setChecked(clicked);
    appPTR->getCurrentSettings()->setAutoDeclaredVariablePrintEnabled(clicked);
}
