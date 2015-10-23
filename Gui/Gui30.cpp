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

#include "Gui.h"

#include <cassert>
#include <sstream> // stringstream
#include <algorithm> // min, max
#include <map>
#include <list>
#include <utility>

#include "Global/Macros.h"

#include <QtCore/QThread>

#include <QAction>
#include <QApplication> // qApp
#include <QGridLayout>
#include <QCheckBox>
#include <QTextEdit>
#include <QUndoGroup>
#include <QUndoStack>

#include "Gui/CurveEditor.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h" // appPTR
#include "Gui/GuiPrivate.h"
#include "Gui/Menu.h"
#include "Gui/MessageBox.h"
#include "Gui/NodeGraph.h"
#include "Gui/PreferencesPanel.h"
#include "Gui/QtEnumConvert.h"
#include "Gui/ResizableMessageBox.h"
#include "Gui/TabWidget.h"
#include "Gui/Utils.h" // convertFromPlainText


using namespace Natron;


void
Gui::errorDialog(const std::string & title,
                 const std::string & text,
                 bool useHtml)
{
    ///don't show dialogs when about to close, otherwise we could enter in a deadlock situation
    {
        QMutexLocker l(&_imp->aboutToCloseMutex);
        if (_imp->_aboutToClose) {
            return;
        }
    }


    Natron::StandardButtons buttons(Natron::eStandardButtonYes | Natron::eStandardButtonNo);

    if ( QThread::currentThread() != QCoreApplication::instance()->thread() ) {
        QMutexLocker locker(&_imp->_uiUsingMainThreadMutex);
        _imp->_uiUsingMainThread = true;
        locker.unlock();
        Q_EMIT doDialog(0, QString( title.c_str() ), QString( text.c_str() ), useHtml, buttons, (int)Natron::eStandardButtonYes);
        locker.relock();
        while (_imp->_uiUsingMainThread) {
            _imp->_uiUsingMainThreadCond.wait(&_imp->_uiUsingMainThreadMutex);
        }
    } else {
        Q_EMIT doDialog(0, QString( title.c_str() ), QString( text.c_str() ), useHtml, buttons, (int)Natron::eStandardButtonYes);
    }
}

void
Gui::errorDialog(const std::string & title,
                 const std::string & text,
                 bool* stopAsking,
                 bool useHtml)
{
    ///don't show dialogs when about to close, otherwise we could enter in a deadlock situation
    {
        QMutexLocker l(&_imp->aboutToCloseMutex);
        if (_imp->_aboutToClose) {
            return;
        }
    }

    Natron::StandardButtons buttons(Natron::eStandardButtonOk);

    if ( QThread::currentThread() != QCoreApplication::instance()->thread() ) {
        QMutexLocker locker(&_imp->_uiUsingMainThreadMutex);
        _imp->_uiUsingMainThread = true;
        locker.unlock();
        Q_EMIT doDialogWithStopAskingCheckbox( (int)MessageBox::eMessageBoxTypeError, QString( title.c_str() ), QString( text.c_str() ), useHtml, buttons, (int)Natron::eStandardButtonOk );
        locker.relock();
        while (_imp->_uiUsingMainThread) {
            _imp->_uiUsingMainThreadCond.wait(&_imp->_uiUsingMainThreadMutex);
        }
    } else {
        Q_EMIT doDialogWithStopAskingCheckbox( (int)MessageBox::eMessageBoxTypeError,
                                               QString( title.c_str() ), QString( text.c_str() ), useHtml, buttons, (int)Natron::eStandardButtonOk );
    }
    *stopAsking = _imp->_lastStopAskingAnswer;
}

void
Gui::warningDialog(const std::string & title,
                   const std::string & text,
                   bool useHtml)
{
    ///don't show dialogs when about to close, otherwise we could enter in a deadlock situation
    {
        QMutexLocker l(&_imp->aboutToCloseMutex);
        if (_imp->_aboutToClose) {
            return;
        }
    }

    Natron::StandardButtons buttons(Natron::eStandardButtonYes | Natron::eStandardButtonNo);

    if ( QThread::currentThread() != QCoreApplication::instance()->thread() ) {
        QMutexLocker locker(&_imp->_uiUsingMainThreadMutex);
        _imp->_uiUsingMainThread = true;
        locker.unlock();
        Q_EMIT doDialog(1, QString( title.c_str() ), QString( text.c_str() ), useHtml, buttons, (int)Natron::eStandardButtonYes);
        locker.relock();
        while (_imp->_uiUsingMainThread) {
            _imp->_uiUsingMainThreadCond.wait(&_imp->_uiUsingMainThreadMutex);
        }
    } else {
        Q_EMIT doDialog(1, QString( title.c_str() ), QString( text.c_str() ), useHtml, buttons, (int)Natron::eStandardButtonYes);
    }
}

void
Gui::warningDialog(const std::string & title,
                   const std::string & text,
                   bool* stopAsking,
                   bool useHtml)
{
    ///don't show dialogs when about to close, otherwise we could enter in a deadlock situation
    {
        QMutexLocker l(&_imp->aboutToCloseMutex);
        if (_imp->_aboutToClose) {
            return;
        }
    }

    Natron::StandardButtons buttons(Natron::eStandardButtonOk);

    if ( QThread::currentThread() != QCoreApplication::instance()->thread() ) {
        QMutexLocker locker(&_imp->_uiUsingMainThreadMutex);
        _imp->_uiUsingMainThread = true;
        locker.unlock();
        Q_EMIT doDialogWithStopAskingCheckbox( (int)MessageBox::eMessageBoxTypeWarning, QString( title.c_str() ), QString( text.c_str() ), useHtml, buttons, (int)Natron::eStandardButtonOk );
        locker.relock();
        while (_imp->_uiUsingMainThread) {
            _imp->_uiUsingMainThreadCond.wait(&_imp->_uiUsingMainThreadMutex);
        }
    } else {
        Q_EMIT doDialogWithStopAskingCheckbox( (int)MessageBox::eMessageBoxTypeWarning,
                                               QString( title.c_str() ), QString( text.c_str() ), useHtml, buttons, (int)Natron::eStandardButtonOk );
    }
    *stopAsking = _imp->_lastStopAskingAnswer;
}

void
Gui::informationDialog(const std::string & title,
                       const std::string & text,
                       bool useHtml)
{
    ///don't show dialogs when about to close, otherwise we could enter in a deadlock situation
    {
        QMutexLocker l(&_imp->aboutToCloseMutex);
        if (_imp->_aboutToClose) {
            return;
        }
    }

    Natron::StandardButtons buttons(Natron::eStandardButtonYes | Natron::eStandardButtonNo);

    if ( QThread::currentThread() != QCoreApplication::instance()->thread() ) {
        QMutexLocker locker(&_imp->_uiUsingMainThreadMutex);
        _imp->_uiUsingMainThread = true;
        locker.unlock();
        Q_EMIT doDialog(2, QString( title.c_str() ), QString( text.c_str() ), useHtml, buttons, (int)Natron::eStandardButtonYes);
        locker.relock();
        while (_imp->_uiUsingMainThread) {
            _imp->_uiUsingMainThreadCond.wait(&_imp->_uiUsingMainThreadMutex);
        }
    } else {
        Q_EMIT doDialog(2, QString( title.c_str() ), QString( text.c_str() ), useHtml, buttons, (int)Natron::eStandardButtonYes);
    }
}

void
Gui::informationDialog(const std::string & title,
                       const std::string & message,
                       bool* stopAsking,
                       bool useHtml)
{
    ///don't show dialogs when about to close, otherwise we could enter in a deadlock situation
    {
        QMutexLocker l(&_imp->aboutToCloseMutex);
        if (_imp->_aboutToClose) {
            return;
        }
    }

    Natron::StandardButtons buttons(Natron::eStandardButtonOk);

    if ( QThread::currentThread() != QCoreApplication::instance()->thread() ) {
        QMutexLocker locker(&_imp->_uiUsingMainThreadMutex);
        _imp->_uiUsingMainThread = true;
        locker.unlock();
        Q_EMIT doDialogWithStopAskingCheckbox( (int)MessageBox::eMessageBoxTypeInformation, QString( title.c_str() ), QString( message.c_str() ), useHtml, buttons, (int)Natron::eStandardButtonOk );
        locker.relock();
        while (_imp->_uiUsingMainThread) {
            _imp->_uiUsingMainThreadCond.wait(&_imp->_uiUsingMainThreadMutex);
        }
    } else {
        Q_EMIT doDialogWithStopAskingCheckbox( (int)MessageBox::eMessageBoxTypeInformation, QString( title.c_str() ), QString( message.c_str() ), useHtml, buttons, (int)Natron::eStandardButtonOk );
    }
    *stopAsking = _imp->_lastStopAskingAnswer;
}

void
Gui::onDoDialog(int type,
                const QString & title,
                const QString & content,
                bool useHtml,
                Natron::StandardButtons buttons,
                int defaultB)
{
    QWidget* currentActiveWindow = qApp->activeWindow();
    
    QString msg = useHtml ? content : Natron::convertFromPlainText(content.trimmed(), Qt::WhiteSpaceNormal);

    if (type == 0) { // error dialog
        QMessageBox critical(QMessageBox::Critical, title, msg, QMessageBox::NoButton, this, Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowStaysOnTopHint);
        critical.setWindowFlags(critical.windowFlags() | Qt::WindowStaysOnTopHint);
        critical.setTextFormat(Qt::RichText);   //this is what makes the links clickable
        ignore_result( critical.exec() );
    } else if (type == 1) { // warning dialog
        QMessageBox warning(QMessageBox::Warning, title, msg, QMessageBox::NoButton, this, Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowStaysOnTopHint);
        warning.setTextFormat(Qt::RichText);
        warning.setWindowFlags(warning.windowFlags() | Qt::WindowStaysOnTopHint);
        ignore_result( warning.exec() );
    } else if (type == 2) { // information dialog
        if (msg.count() < 1000) {
            QMessageBox info(QMessageBox::Information, title, msg, QMessageBox::NoButton, this, Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint| Qt::WindowStaysOnTopHint);
            info.setTextFormat(Qt::RichText);
            info.setWindowFlags(info.windowFlags() | Qt::WindowStaysOnTopHint);
            ignore_result( info.exec() );
        } else {
            // text may be very long: use resizable QMessageBox
            ResizableMessageBox info(QMessageBox::Information, title, msg.left(1000), QMessageBox::NoButton, this, Qt::Dialog | Qt::WindowStaysOnTopHint);
            info.setTextFormat(Qt::RichText);
            info.setWindowFlags(info.windowFlags() | Qt::WindowStaysOnTopHint);
            QGridLayout *layout = qobject_cast<QGridLayout *>( info.layout() );
            if (layout) {
                QTextEdit *edit = new QTextEdit();
                edit->setReadOnly(true);
                edit->setAcceptRichText(true);
                edit->setHtml(msg);
                layout->setRowStretch(1, 0);
                layout->addWidget(edit, 0, 1);
            }
            ignore_result( info.exec() );
        }
    } else { // question dialog
        assert(type == 3);
        QMessageBox ques(QMessageBox::Question, title, msg, QtEnumConvert::toQtStandarButtons(buttons),
                         this, Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowStaysOnTopHint);
        ques.setDefaultButton( QtEnumConvert::toQtStandardButton( (Natron::StandardButtonEnum)defaultB ) );
        ques.setWindowFlags(ques.windowFlags() | Qt::WindowStaysOnTopHint);
        if ( ques.exec() ) {
            _imp->_lastQuestionDialogAnswer = QtEnumConvert::fromQtStandardButton( ques.standardButton( ques.clickedButton() ) );
        }
    }

    {
        QMutexLocker locker(&_imp->_uiUsingMainThreadMutex);
        _imp->_uiUsingMainThread = false;
        _imp->_uiUsingMainThreadCond.wakeOne();
    }
    if (currentActiveWindow) {
        currentActiveWindow->activateWindow();
    }
}

Natron::StandardButtonEnum
Gui::questionDialog(const std::string & title,
                    const std::string & message,
                    bool useHtml,
                    Natron::StandardButtons buttons,
                    Natron::StandardButtonEnum defaultButton)
{
    ///don't show dialogs when about to close, otherwise we could enter in a deadlock situation
    {
        QMutexLocker l(&_imp->aboutToCloseMutex);
        if (_imp->_aboutToClose) {
            return Natron::eStandardButtonNo;
        }
    }

    if ( QThread::currentThread() != QCoreApplication::instance()->thread() ) {
        QMutexLocker locker(&_imp->_uiUsingMainThreadMutex);
        _imp->_uiUsingMainThread = true;
        locker.unlock();
        Q_EMIT doDialog(3, QString( title.c_str() ), QString( message.c_str() ), useHtml, buttons, (int)defaultButton);
        locker.relock();
        while (_imp->_uiUsingMainThread) {
            _imp->_uiUsingMainThreadCond.wait(&_imp->_uiUsingMainThreadMutex);
        }
    } else {
        Q_EMIT doDialog(3, QString( title.c_str() ), QString( message.c_str() ), useHtml, buttons, (int)defaultButton);
    }

    return _imp->_lastQuestionDialogAnswer;
}

Natron::StandardButtonEnum
Gui::questionDialog(const std::string & title,
                    const std::string & message,
                    bool useHtml,
                    Natron::StandardButtons buttons,
                    Natron::StandardButtonEnum defaultButton,
                    bool* stopAsking)
{
    ///don't show dialogs when about to close, otherwise we could enter in a deadlock situation
    {
        QMutexLocker l(&_imp->aboutToCloseMutex);
        if (_imp->_aboutToClose) {
            return Natron::eStandardButtonNo;
        }
    }

    if ( QThread::currentThread() != QCoreApplication::instance()->thread() ) {
        QMutexLocker locker(&_imp->_uiUsingMainThreadMutex);
        _imp->_uiUsingMainThread = true;
        locker.unlock();
        Q_EMIT onDoDialogWithStopAskingCheckbox( (int)Natron::MessageBox::eMessageBoxTypeQuestion,
                                                 QString( title.c_str() ), QString( message.c_str() ), useHtml, buttons, (int)defaultButton );
        locker.relock();
        while (_imp->_uiUsingMainThread) {
            _imp->_uiUsingMainThreadCond.wait(&_imp->_uiUsingMainThreadMutex);
        }
    } else {
        Q_EMIT onDoDialogWithStopAskingCheckbox( (int)Natron::MessageBox::eMessageBoxTypeQuestion,
                                                 QString( title.c_str() ), QString( message.c_str() ), useHtml, buttons, (int)defaultButton );
    }

    *stopAsking = _imp->_lastStopAskingAnswer;

    return _imp->_lastQuestionDialogAnswer;
}

void
Gui::onDoDialogWithStopAskingCheckbox(int type,
                                      const QString & title,
                                      const QString & content,
                                      bool useHtml,
                                      Natron::StandardButtons buttons,
                                      int defaultB)
{
    QString message = useHtml ? content : Natron::convertFromPlainText(content.trimmed(), Qt::WhiteSpaceNormal);
    Natron::MessageBox dialog(title, content, (Natron::MessageBox::MessageBoxTypeEnum)type, buttons, (Natron::StandardButtonEnum)defaultB, this);
    QCheckBox* stopAskingCheckbox = new QCheckBox(tr("Do Not Show This Again"), &dialog);

    dialog.setCheckBox(stopAskingCheckbox);
    dialog.setWindowFlags(dialog.windowFlags() | Qt::WindowStaysOnTopHint);
    if ( dialog.exec() ) {
        _imp->_lastQuestionDialogAnswer = dialog.getReply();
        _imp->_lastStopAskingAnswer = stopAskingCheckbox->isChecked();
    }
}

void
Gui::selectNode(boost::shared_ptr<NodeGui> node)
{
    if (!node) {
        return;
    }
    _imp->_nodeGraphArea->selectNode(node, false); //< wipe current selection
}

void
Gui::connectInput(int inputNb)
{
    NodeGraph* graph = 0;
    
    if (_imp->_lastFocusedGraph) {
        graph = _imp->_lastFocusedGraph;
    } else {
        graph = _imp->_nodeGraphArea;
    }
    graph->connectCurrentViewerToSelection(inputNb);
}

void
Gui::connectInput()
{
    QAction* action = qobject_cast<QAction*>(sender());
    if (!action) {
        return;
    }
    int inputNb = action->data().toInt();
    connectInput(inputNb);
}

void
Gui::showView0()
{
    _imp->_appInstance->setViewersCurrentView(0);
}

void
Gui::showView1()
{
    _imp->_appInstance->setViewersCurrentView(1);
}

void
Gui::showView2()
{
    _imp->_appInstance->setViewersCurrentView(2);
}

void
Gui::showView3()
{
    _imp->_appInstance->setViewersCurrentView(3);
}

void
Gui::showView4()
{
    _imp->_appInstance->setViewersCurrentView(4);
}

void
Gui::showView5()
{
    _imp->_appInstance->setViewersCurrentView(5);
}

void
Gui::showView6()
{
    _imp->_appInstance->setViewersCurrentView(6);
}

void
Gui::showView7()
{
    _imp->_appInstance->setViewersCurrentView(7);
}

void
Gui::showView8()
{
    _imp->_appInstance->setViewersCurrentView(8);
}

void
Gui::showView9()
{
    _imp->_appInstance->setViewersCurrentView(9);
}

void
Gui::setCurveEditorOnTop()
{
    QMutexLocker l(&_imp->_panesMutex);

    for (std::list<TabWidget*>::iterator it = _imp->_panes.begin(); it != _imp->_panes.end(); ++it) {
        TabWidget* cur = (*it);
        assert(cur);
        for (int i = 0; i < cur->count(); ++i) {
            if (cur->tabAt(i) == _imp->_curveEditor) {
                cur->makeCurrentTab(i);
                break;
            }
        }
    }
}

void
Gui::showSettings()
{
    if (!_imp->_settingsGui) {
        _imp->_settingsGui = new PreferencesPanel(appPTR->getCurrentSettings(), this);
    }
    _imp->_settingsGui->show();
    _imp->_settingsGui->raise();
    _imp->_settingsGui->activateWindow();
}

void
Gui::registerNewUndoStack(QUndoStack* stack)
{
    _imp->_undoStacksGroup->addStack(stack);
    QAction* undo = stack->createUndoAction(stack);
    undo->setShortcut(QKeySequence::Undo);
    QAction* redo = stack->createRedoAction(stack);
    redo->setShortcut(QKeySequence::Redo);
    _imp->_undoStacksActions.insert( std::make_pair( stack, std::make_pair(undo, redo) ) );
}

void
Gui::removeUndoStack(QUndoStack* stack)
{
    std::map<QUndoStack*, std::pair<QAction*, QAction*> >::iterator it = _imp->_undoStacksActions.find(stack);
	if (it == _imp->_undoStacksActions.end()) {
		return;
	}
    if (_imp->_currentUndoAction == it->second.first) {
        _imp->menuEdit->removeAction(_imp->_currentUndoAction);
    }
    if (_imp->_currentRedoAction == it->second.second) {
        _imp->menuEdit->removeAction(_imp->_currentRedoAction);
    }
    if ( it != _imp->_undoStacksActions.end() ) {
        _imp->_undoStacksActions.erase(it);
    }
}

void
Gui::onCurrentUndoStackChanged(QUndoStack* stack)
{
    std::map<QUndoStack*, std::pair<QAction*, QAction*> >::iterator it = _imp->_undoStacksActions.find(stack);

    //the stack must have been registered first with registerNewUndoStack()
    if ( it != _imp->_undoStacksActions.end() ) {
        _imp->setUndoRedoActions(it->second.first, it->second.second);
    }
}
