/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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
#include <algorithm> // min, max
#include <map>
#include <list>
#include <utility>
#include <stdexcept>

#include <QtCore/QThread>
#include <QtCore/QDebug>

#include <QAction>
#include <QApplication> // qApp
#include <QGridLayout>
#include <QCheckBox>
#include <QLabel>
#include <QTextEdit>
#include <QUndoGroup>
#include <QUndoStack>

#include "Engine/Utils.h" // convertFromPlainText
#include "Engine/ViewIdx.h"

#include "Gui/AnimationModuleEditor.h"
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


NATRON_NAMESPACE_ENTER


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


    StandardButtons buttons(eStandardButtonYes | eStandardButtonNo);

    if ( QThread::currentThread() != QCoreApplication::instance()->thread() ) {
        QMutexLocker locker(&_imp->_uiUsingMainThreadMutex);
        while (_imp->_uiUsingMainThread) {
            _imp->_uiUsingMainThreadCond.wait(&_imp->_uiUsingMainThreadMutex);
        }
        ++_imp->_uiUsingMainThread;
        locker.unlock();
        Q_EMIT doDialog(0, QString::fromUtf8( title.c_str() ), QString::fromUtf8( text.c_str() ), useHtml, buttons, (int)eStandardButtonYes);
        locker.relock();
        while (_imp->_uiUsingMainThread) {
            _imp->_uiUsingMainThreadCond.wait(&_imp->_uiUsingMainThreadMutex);
        }
    } else {
        {
            QMutexLocker locker(&_imp->_uiUsingMainThreadMutex);
            ++_imp->_uiUsingMainThread;
        }
        Q_EMIT doDialog(0, QString::fromUtf8( title.c_str() ), QString::fromUtf8( text.c_str() ), useHtml, buttons, (int)eStandardButtonYes);
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

    StandardButtons buttons(eStandardButtonOk);

    if ( QThread::currentThread() != QCoreApplication::instance()->thread() ) {
        QMutexLocker locker(&_imp->_uiUsingMainThreadMutex);
        while (_imp->_uiUsingMainThread) {
            _imp->_uiUsingMainThreadCond.wait(&_imp->_uiUsingMainThreadMutex);
        }
        ++_imp->_uiUsingMainThread;
        locker.unlock();
        Q_EMIT doDialogWithStopAskingCheckbox( (int)MessageBox::eMessageBoxTypeError, QString::fromUtf8( title.c_str() ), QString::fromUtf8( text.c_str() ), useHtml, buttons, (int)eStandardButtonOk );
        locker.relock();
        while (_imp->_uiUsingMainThread) {
            _imp->_uiUsingMainThreadCond.wait(&_imp->_uiUsingMainThreadMutex);
        }
    } else {
        {
            QMutexLocker locker(&_imp->_uiUsingMainThreadMutex);
            ++_imp->_uiUsingMainThread;
        }
        Q_EMIT doDialogWithStopAskingCheckbox( (int)MessageBox::eMessageBoxTypeError,
                                               QString::fromUtf8( title.c_str() ), QString::fromUtf8( text.c_str() ), useHtml, buttons, (int)eStandardButtonOk );
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

    StandardButtons buttons(eStandardButtonYes | eStandardButtonNo);

    if ( QThread::currentThread() != QCoreApplication::instance()->thread() ) {
        QMutexLocker locker(&_imp->_uiUsingMainThreadMutex);
        while (_imp->_uiUsingMainThread) {
            _imp->_uiUsingMainThreadCond.wait(&_imp->_uiUsingMainThreadMutex);
        }
        ++_imp->_uiUsingMainThread;
        locker.unlock();
        Q_EMIT doDialog(1, QString::fromUtf8( title.c_str() ), QString::fromUtf8( text.c_str() ), useHtml, buttons, (int)eStandardButtonYes);
        locker.relock();
        while (_imp->_uiUsingMainThread) {
            _imp->_uiUsingMainThreadCond.wait(&_imp->_uiUsingMainThreadMutex);
        }
    } else {
        {
            QMutexLocker locker(&_imp->_uiUsingMainThreadMutex);
            ++_imp->_uiUsingMainThread;
        }
        Q_EMIT doDialog(1, QString::fromUtf8( title.c_str() ), QString::fromUtf8( text.c_str() ), useHtml, buttons, (int)eStandardButtonYes);
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

    StandardButtons buttons(eStandardButtonOk);

    if ( QThread::currentThread() != QCoreApplication::instance()->thread() ) {
        QMutexLocker locker(&_imp->_uiUsingMainThreadMutex);
        while (_imp->_uiUsingMainThread) {
            _imp->_uiUsingMainThreadCond.wait(&_imp->_uiUsingMainThreadMutex);
        }
        ++_imp->_uiUsingMainThread;
        locker.unlock();
        Q_EMIT doDialogWithStopAskingCheckbox( (int)MessageBox::eMessageBoxTypeWarning, QString::fromUtf8( title.c_str() ), QString::fromUtf8( text.c_str() ), useHtml, buttons, (int)eStandardButtonOk );
        locker.relock();
        while (_imp->_uiUsingMainThread) {
            _imp->_uiUsingMainThreadCond.wait(&_imp->_uiUsingMainThreadMutex);
        }
    } else {
        {
            QMutexLocker locker(&_imp->_uiUsingMainThreadMutex);
            ++_imp->_uiUsingMainThread;
        }
        Q_EMIT doDialogWithStopAskingCheckbox( (int)MessageBox::eMessageBoxTypeWarning,
                                               QString::fromUtf8( title.c_str() ), QString::fromUtf8( text.c_str() ), useHtml, buttons, (int)eStandardButtonOk );
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

    StandardButtons buttons(eStandardButtonYes | eStandardButtonNo);

    if ( QThread::currentThread() != QCoreApplication::instance()->thread() ) {
        QMutexLocker locker(&_imp->_uiUsingMainThreadMutex);
        while (_imp->_uiUsingMainThread) {
            _imp->_uiUsingMainThreadCond.wait(&_imp->_uiUsingMainThreadMutex);
        }
        assert(!_imp->_uiUsingMainThread);
        ++_imp->_uiUsingMainThread;
        locker.unlock();
        Q_EMIT doDialog(2, QString::fromUtf8( title.c_str() ), QString::fromUtf8( text.c_str() ), useHtml, buttons, (int)eStandardButtonYes);
        locker.relock();
        while (_imp->_uiUsingMainThread) {
            _imp->_uiUsingMainThreadCond.wait(&_imp->_uiUsingMainThreadMutex);
        }
    } else {
        {
            QMutexLocker locker(&_imp->_uiUsingMainThreadMutex);
            ++_imp->_uiUsingMainThread;
        }
        Q_EMIT doDialog(2, QString::fromUtf8( title.c_str() ), QString::fromUtf8( text.c_str() ), useHtml, buttons, (int)eStandardButtonYes);
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

    StandardButtons buttons(eStandardButtonOk);

    if ( QThread::currentThread() != QCoreApplication::instance()->thread() ) {
        QMutexLocker locker(&_imp->_uiUsingMainThreadMutex);
        while (_imp->_uiUsingMainThread) {
            _imp->_uiUsingMainThreadCond.wait(&_imp->_uiUsingMainThreadMutex);
        }
        assert(!_imp->_uiUsingMainThread);
        ++_imp->_uiUsingMainThread;
        locker.unlock();
        Q_EMIT doDialogWithStopAskingCheckbox( (int)MessageBox::eMessageBoxTypeInformation, QString::fromUtf8( title.c_str() ), QString::fromUtf8( message.c_str() ), useHtml, buttons, (int)eStandardButtonOk );
        locker.relock();
        while (_imp->_uiUsingMainThread) {
            _imp->_uiUsingMainThreadCond.wait(&_imp->_uiUsingMainThreadMutex);
        }
    } else {
        {
            QMutexLocker locker(&_imp->_uiUsingMainThreadMutex);
            ++_imp->_uiUsingMainThread;
        }
        Q_EMIT doDialogWithStopAskingCheckbox( (int)MessageBox::eMessageBoxTypeInformation, QString::fromUtf8( title.c_str() ), QString::fromUtf8( message.c_str() ), useHtml, buttons, (int)eStandardButtonOk );
    }
    *stopAsking = _imp->_lastStopAskingAnswer;
}

void
Gui::setQMessageBoxAppropriateFont(QMessageBox* widget)
{
    QGridLayout* grid = dynamic_cast<QGridLayout*>( widget->layout() );

    assert(grid);
    if (!grid) {
        return;
    }
    int count = grid->count();
    for (int i = 0; i < count; ++i) {
        QLayoutItem* item = grid->itemAt(i);
        if (item) {
            QWidget* w = item->widget();
            if (w) {
                QLabel* isLabel = dynamic_cast<QLabel*>(w);
                if (isLabel) {
                    isLabel->setFont( QApplication::font() ); // necessary, or the labels will get the default font size
                }
            }
        }
    }

    QList<QAbstractButton *> buttonsList = widget->buttons();
    Q_FOREACH(QAbstractButton* b, buttonsList) {
        b->setAttribute(Qt::WA_LayoutUsesWidgetRect); // Don't use the layout rect calculated from QMacStyle.
        b->setFont( QApplication::font() ); // necessary, or the labels will get the default font size
    }

}

void
Gui::onDoDialog(int type,
                const QString & title,
                const QString & content,
                bool useHtml,
                StandardButtons buttons,
                int defaultB)
{
    QWidget* currentActiveWindow = qApp->activeWindow();
    QDialog* isActiveWindowADialog = 0;

    if (currentActiveWindow) {
        isActiveWindowADialog = qobject_cast<QDialog*>(currentActiveWindow);
    }

    QString msg = useHtml ? content : NATRON_NAMESPACE::convertFromPlainText(content.trimmed(), NATRON_NAMESPACE::WhiteSpaceNormal);
    QMessageBox::Icon msgBoxType;
    QMessageBox::StandardButtons msgBoxButtons = QMessageBox::NoButton;
    QMessageBox::StandardButton defMsgBoxButton = QMessageBox::NoButton;
    switch (type) {
        case 0:
            msgBoxType = QMessageBox::Critical;
            break;
        case 1:
            msgBoxType = QMessageBox::Warning;
            break;
        case 2:
            msgBoxType = QMessageBox::Information;
            break;
        case 3:
            msgBoxType = QMessageBox::Question;
            msgBoxButtons = QtEnumConvert::toQtStandarButtons(buttons);
            defMsgBoxButton = QtEnumConvert::toQtStandardButton( (StandardButtonEnum)defaultB );
            break;
        default:
            msgBoxType = QMessageBox::Information;
            break;
    }

    if (msg.count() < 1000) {
        QMessageBox dialog(msgBoxType, title, msg, msgBoxButtons, this, Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowStaysOnTopHint);
        setQMessageBoxAppropriateFont(&dialog);
        dialog.setTextFormat(Qt::RichText);
        dialog.setWindowFlags(dialog.windowFlags() | Qt::WindowStaysOnTopHint);
        dialog.setDefaultButton(defMsgBoxButton);
        if ( dialog.exec() ) {
            if (msgBoxType == QMessageBox::Question) {
                _imp->_lastQuestionDialogAnswer = QtEnumConvert::fromQtStandardButton( dialog.standardButton( dialog.clickedButton() ) );
            }
        }
    } else {
        // text may be very long: use resizable QMessageBox
        ResizableMessageBox dialog(msgBoxType, title, msg.left(1000), msgBoxButtons, this, Qt::Dialog | Qt::WindowStaysOnTopHint);
        dialog.setTextFormat(Qt::RichText);
        dialog.setWindowFlags(dialog.windowFlags() | Qt::WindowStaysOnTopHint);
        dialog.setDefaultButton(defMsgBoxButton);
        QGridLayout *layout = qobject_cast<QGridLayout *>( dialog.layout() );
        if (layout) {
            QTextEdit *edit = new QTextEdit;
            edit->setFocusPolicy(Qt::NoFocus);
            edit->setReadOnly(true);
            edit->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
            edit->setAcceptRichText(true);
            edit->setHtml(msg);
            layout->setRowStretch(1, 0);
            layout->addWidget(edit, 0, 1);
        }
        if ( dialog.exec() ) {
            if (msgBoxType == QMessageBox::Question) {
                _imp->_lastQuestionDialogAnswer = QtEnumConvert::fromQtStandardButton( dialog.standardButton( dialog.clickedButton() ) );
            }
        }
    }

    {
        QMutexLocker locker(&_imp->_uiUsingMainThreadMutex);
        assert(_imp->_uiUsingMainThread);
        --_imp->_uiUsingMainThread;
        _imp->_uiUsingMainThreadCond.wakeAll();
    }
    if (currentActiveWindow && !isActiveWindowADialog) {
        currentActiveWindow->activateWindow();
    }
} // Gui::onDoDialog

StandardButtonEnum
Gui::questionDialog(const std::string & title,
                    const std::string & message,
                    bool useHtml,
                    StandardButtons buttons,
                    StandardButtonEnum defaultButton)
{
    ///don't show dialogs when about to close, otherwise we could enter in a deadlock situation
    {
        QMutexLocker l(&_imp->aboutToCloseMutex);
        if (_imp->_aboutToClose) {
            return eStandardButtonNo;
        }
    }

    if ( QThread::currentThread() != QCoreApplication::instance()->thread() ) {
        QMutexLocker locker(&_imp->_uiUsingMainThreadMutex);
        while (_imp->_uiUsingMainThread) {
            _imp->_uiUsingMainThreadCond.wait(&_imp->_uiUsingMainThreadMutex);
        }
        assert(!_imp->_uiUsingMainThread);
        ++_imp->_uiUsingMainThread;
        locker.unlock();
        Q_EMIT doDialog(3, QString::fromUtf8( title.c_str() ), QString::fromUtf8( message.c_str() ), useHtml, buttons, (int)defaultButton);
        locker.relock();
        while (_imp->_uiUsingMainThread) {
            _imp->_uiUsingMainThreadCond.wait(&_imp->_uiUsingMainThreadMutex);
        }
    } else {
        {
            QMutexLocker locker(&_imp->_uiUsingMainThreadMutex);
            ++_imp->_uiUsingMainThread;
        }
        Q_EMIT doDialog(3, QString::fromUtf8( title.c_str() ), QString::fromUtf8( message.c_str() ), useHtml, buttons, (int)defaultButton);
    }

    return _imp->_lastQuestionDialogAnswer;
}

StandardButtonEnum
Gui::questionDialog(const std::string & title,
                    const std::string & message,
                    bool useHtml,
                    StandardButtons buttons,
                    StandardButtonEnum defaultButton,
                    bool* stopAsking)
{
    ///don't show dialogs when about to close, otherwise we could enter in a deadlock situation
    {
        QMutexLocker l(&_imp->aboutToCloseMutex);
        if (_imp->_aboutToClose) {
            return eStandardButtonNo;
        }
    }

    if ( QThread::currentThread() != QCoreApplication::instance()->thread() ) {
        QMutexLocker locker(&_imp->_uiUsingMainThreadMutex);
        while (_imp->_uiUsingMainThread) {
            _imp->_uiUsingMainThreadCond.wait(&_imp->_uiUsingMainThreadMutex);
        }
        assert(!_imp->_uiUsingMainThread);
        ++_imp->_uiUsingMainThread;
        locker.unlock();
        Q_EMIT onDoDialogWithStopAskingCheckbox( (int)MessageBox::eMessageBoxTypeQuestion,
                                                 QString::fromUtf8( title.c_str() ), QString::fromUtf8( message.c_str() ), useHtml, buttons, (int)defaultButton );
        locker.relock();
        while (_imp->_uiUsingMainThread) {
            _imp->_uiUsingMainThreadCond.wait(&_imp->_uiUsingMainThreadMutex);
        }
    } else {
        {
            QMutexLocker locker(&_imp->_uiUsingMainThreadMutex);
            ++_imp->_uiUsingMainThread;
        }
        Q_EMIT onDoDialogWithStopAskingCheckbox( (int)MessageBox::eMessageBoxTypeQuestion,
                                                 QString::fromUtf8( title.c_str() ), QString::fromUtf8( message.c_str() ), useHtml, buttons, (int)defaultButton );
    }

    *stopAsking = _imp->_lastStopAskingAnswer;

    return _imp->_lastQuestionDialogAnswer;
}

void
Gui::onDoDialogWithStopAskingCheckbox(int type,
                                      const QString & title,
                                      const QString & content,
                                      bool useHtml,
                                      StandardButtons buttons,
                                      int defaultB)
{
    QString message = useHtml ? content : NATRON_NAMESPACE::convertFromPlainText(content.trimmed(), NATRON_NAMESPACE::WhiteSpaceNormal);
    MessageBox dialog(title, content, (MessageBox::MessageBoxTypeEnum)type, buttons, (StandardButtonEnum)defaultB, this);
    QCheckBox* stopAskingCheckbox = new QCheckBox(tr("Do Not Show This Again"), &dialog);

    dialog.setCheckBox(stopAskingCheckbox);
    dialog.setWindowFlags(dialog.windowFlags() | Qt::WindowStaysOnTopHint);
    if ( dialog.exec() ) {
        _imp->_lastQuestionDialogAnswer = dialog.getReply();
        _imp->_lastStopAskingAnswer = stopAskingCheckbox->isChecked();
    }

    {
        QMutexLocker locker(&_imp->_uiUsingMainThreadMutex);
        assert(_imp->_uiUsingMainThread);
        --_imp->_uiUsingMainThread;
        _imp->_uiUsingMainThreadCond.wakeAll();
    }
}

void
Gui::selectNode(const NodeGuiPtr& node)
{
    if (!node) {
        return;
    }
    _imp->_nodeGraphArea->selectNode(node, false); //< wipe current selection
}

void
Gui::connectInput(int inputNb,
                  bool isASide)
{
    NodeGraph* graph = 0;

    if (_imp->_lastFocusedGraph) {
        graph = _imp->_lastFocusedGraph;
    } else {
        graph = _imp->_nodeGraphArea;
    }
    graph->connectCurrentViewerToSelection(inputNb, isASide);
}

void
Gui::connectAInput()
{
    QAction* action = qobject_cast<QAction*>( sender() );

    if (!action) {
        return;
    }
    int inputNb = action->data().toInt();
    connectAInput(inputNb);
}

void
Gui::connectAInput(int inputNb)
{
    connectInput(inputNb, true);
}

void
Gui::connectBInput(int inputNb)
{
    connectInput(inputNb, false);
}

void
Gui::connectBInput()
{
    QAction* action = qobject_cast<QAction*>( sender() );

    if (!action) {
        return;
    }
    int inputNb = action->data().toInt();
    connectBInput(inputNb);
}

void
Gui::showView0()
{
    getApp()->setViewersCurrentView( ViewIdx(0) );
}

void
Gui::showView1()
{
    getApp()->setViewersCurrentView( ViewIdx(1) );
}

void
Gui::showView2()
{
    getApp()->setViewersCurrentView( ViewIdx(2) );
}

void
Gui::showView3()
{
    getApp()->setViewersCurrentView( ViewIdx(3) );
}

void
Gui::showView4()
{
    getApp()->setViewersCurrentView( ViewIdx(4) );
}

void
Gui::showView5()
{
    getApp()->setViewersCurrentView( ViewIdx(5) );
}

void
Gui::showView6()
{
    getApp()->setViewersCurrentView( ViewIdx(6) );
}

void
Gui::showView7()
{
    getApp()->setViewersCurrentView( ViewIdx(7) );
}

void
Gui::showView8()
{
    getApp()->setViewersCurrentView( ViewIdx(8) );
}

void
Gui::showView9()
{
    getApp()->setViewersCurrentView( ViewIdx(9) );
}

void
Gui::setAnimationEditorOnTop()
{
    std::list<TabWidgetI*> tabs = getApp()->getTabWidgetsSerialization();
    for (std::list<TabWidgetI*>::iterator it = tabs.begin(); it != tabs.end(); ++it) {
        TabWidget* cur = dynamic_cast<TabWidget*>(*it);
        assert(cur);
        if (!cur) {
            continue;
        }
        for (int i = 0; i < cur->count(); ++i) {
            if (cur->tabAt(i) == _imp->_animationModule) {
                cur->setCurrentIndex(i);
                break;
            }
        }
    }
}

void
Gui::showSettings()
{
    if (!_imp->_settingsGui) {
        _imp->_settingsGui = new PreferencesPanel(this);
        _imp->_settingsGui->createGui();
    }
    _imp->_settingsGui->show();
    _imp->_settingsGui->raise();
    _imp->_settingsGui->activateWindow();
}

void
Gui::registerNewUndoStack(const boost::shared_ptr<QUndoStack>& stack)
{
    _imp->_undoStacksGroup->addStack(stack.get());
}

void
Gui::removeUndoStack(const boost::shared_ptr<QUndoStack>& stack)
{
    _imp->_undoStacksGroup->removeStack(stack.get());
}


NATRON_NAMESPACE_EXIT
