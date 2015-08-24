//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "Gui.h"

#include <cassert>
#include <sstream> // stringstream
#include <algorithm> // min, max
#include <map>
#include <list>
#include <utility>

#include "Global/Macros.h"

//#include <QtCore/QTextStream>
#include <QtCore/QSettings>
#include <QtCore/QDebug>
#include <QtCore/QThread>
#include <QtCore/QTimer>
//#include <QtCore/QFile>
#include <QtCore/QFileInfo>

#include <QAction>
#include <QApplication> // qApp
#include <QGridLayout>
GCC_DIAG_UNUSED_PRIVATE_FIELD_OFF
// /opt/local/include/QtGui/qmime.h:119:10: warning: private field 'type' is not used [-Wunused-private-field]
#include <QKeyEvent>
GCC_DIAG_UNUSED_PRIVATE_FIELD_ON
#include <QCheckBox>
#include <QMenuBar>
#include <QProgressDialog>
#include <QTextEdit>
#include <QUndoGroup>
#include <QUndoStack>

#include <cairo/cairo.h>

#include <boost/version.hpp>

#include "Engine/GroupOutput.h"
#include "Engine/Image.h"
#include "Engine/Lut.h" // floatToInt, LutManager
#include "Engine/Node.h"
#include "Engine/Project.h"
#include "Engine/Settings.h"
#include "Engine/ViewerInstance.h"

#include "Gui/AboutWindow.h"
#include "Gui/ActionShortcuts.h"
#include "Gui/CurveEditor.h"
#include "Gui/DockablePanel.h"
#include "Gui/DopeSheetEditor.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h" // appPTR
#include "Gui/GuiPrivate.h"
#include "Gui/Menu.h"
#include "Gui/MessageBox.h"
#include "Gui/NodeGraph.h"
#include "Gui/NodeGui.h"
#include "Gui/NodeSettingsPanel.h"
#include "Gui/PreferencesPanel.h"
#include "Gui/ProjectGui.h"
#include "Gui/RenderingProgressDialog.h"
#include "Gui/ResizableMessageBox.h"
#include "Gui/RightClickableWidget.h"
#include "Gui/RotoGui.h"
#include "Gui/ScriptEditor.h"
#include "Gui/ShortCutEditor.h"
#include "Gui/SpinBox.h"
#include "Gui/Splitter.h"
#include "Gui/TabWidget.h"
#include "Gui/Utils.h" // convertFromPlainText
#include "Gui/ViewerGL.h"
#include "Gui/ViewerTab.h"


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

    QMutexLocker locker(&_imp->_uiUsingMainThreadMutex);
    _imp->_uiUsingMainThread = false;
    _imp->_uiUsingMainThreadCond.wakeOne();
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
Gui::connectInput1()
{
    NodeGraph* graph = 0;

    if (_imp->_lastFocusedGraph) {
        graph = _imp->_lastFocusedGraph;
    } else {
        graph = _imp->_nodeGraphArea;
    }
    graph->connectCurrentViewerToSelection(0);
}

void
Gui::connectInput2()
{
    NodeGraph* graph = 0;

    if (_imp->_lastFocusedGraph) {
        graph = _imp->_lastFocusedGraph;
    } else {
        graph = _imp->_nodeGraphArea;
    }
    graph->connectCurrentViewerToSelection(1);
}

void
Gui::connectInput3()
{
    NodeGraph* graph = 0;

    if (_imp->_lastFocusedGraph) {
        graph = _imp->_lastFocusedGraph;
    } else {
        graph = _imp->_nodeGraphArea;
    }
    graph->connectCurrentViewerToSelection(2);
}

void
Gui::connectInput4()
{
    NodeGraph* graph = 0;

    if (_imp->_lastFocusedGraph) {
        graph = _imp->_lastFocusedGraph;
    } else {
        graph = _imp->_nodeGraphArea;
    }
    graph->connectCurrentViewerToSelection(3);
}

void
Gui::connectInput5()
{
    NodeGraph* graph = 0;

    if (_imp->_lastFocusedGraph) {
        graph = _imp->_lastFocusedGraph;
    } else {
        graph = _imp->_nodeGraphArea;
    }
    graph->connectCurrentViewerToSelection(4);
}

void
Gui::connectInput6()
{
    NodeGraph* graph = 0;

    if (_imp->_lastFocusedGraph) {
        graph = _imp->_lastFocusedGraph;
    } else {
        graph = _imp->_nodeGraphArea;
    }
    graph->connectCurrentViewerToSelection(5);
}

void
Gui::connectInput7()
{
    NodeGraph* graph = 0;

    if (_imp->_lastFocusedGraph) {
        graph = _imp->_lastFocusedGraph;
    } else {
        graph = _imp->_nodeGraphArea;
    }
    graph->connectCurrentViewerToSelection(6);
}

void
Gui::connectInput8()
{
    NodeGraph* graph = 0;

    if (_imp->_lastFocusedGraph) {
        graph = _imp->_lastFocusedGraph;
    } else {
        graph = _imp->_nodeGraphArea;
    }
    graph->connectCurrentViewerToSelection(7);
}

void
Gui::connectInput9()
{
    NodeGraph* graph = 0;

    if (_imp->_lastFocusedGraph) {
        graph = _imp->_lastFocusedGraph;
    } else {
        graph = _imp->_nodeGraphArea;
    }
    graph->connectCurrentViewerToSelection(8);
}

void
Gui::connectInput10()
{
    NodeGraph* graph = 0;

    if (_imp->_lastFocusedGraph) {
        graph = _imp->_lastFocusedGraph;
    } else {
        graph = _imp->_nodeGraphArea;
    }
    graph->connectCurrentViewerToSelection(9);
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

void
Gui::refreshAllPreviews()
{
    _imp->_appInstance->getProject()->refreshPreviews();
}

void
Gui::forceRefreshAllPreviews()
{
    _imp->_appInstance->getProject()->forceRefreshPreviews();
}

void
Gui::startDragPanel(QWidget* panel)
{
    assert(!_imp->_currentlyDraggedPanel);
    _imp->_currentlyDraggedPanel = panel;
}

QWidget*
Gui::stopDragPanel()
{
    assert(_imp->_currentlyDraggedPanel);
    QWidget* ret = _imp->_currentlyDraggedPanel;
    _imp->_currentlyDraggedPanel = 0;

    return ret;
}

void
Gui::showAbout()
{
    _imp->_aboutWindow->show();
    _imp->_aboutWindow->raise();
    _imp->_aboutWindow->activateWindow();
    ignore_result( _imp->_aboutWindow->exec() );
}

void
Gui::showShortcutEditor()
{
    _imp->shortcutEditor->show();
    _imp->shortcutEditor->raise();
    _imp->shortcutEditor->activateWindow();
}

void
Gui::openRecentFile()
{
    QAction *action = qobject_cast<QAction *>( sender() );

    if (action) {
        QFileInfo f( action->data().toString() );
        QString path = f.path() + '/';
        QString filename = path + f.fileName();
        int openedProject = appPTR->isProjectAlreadyOpened( filename.toStdString() );
        if (openedProject != -1) {
            AppInstance* instance = appPTR->getAppInstance(openedProject);
            if (instance) {
                GuiAppInstance* guiApp = dynamic_cast<GuiAppInstance*>(instance);
                assert(guiApp);
                if (guiApp) {
                    guiApp->getGui()->activateWindow();

                    return;
                }
            }
        }

        ///if the current graph has no value, just load the project in the same window
        if ( _imp->_appInstance->getProject()->isGraphWorthLess() ) {
            _imp->_appInstance->getProject()->loadProject( path, f.fileName() );
        } else {
            CLArgs cl;
            AppInstance* newApp = appPTR->newAppInstance(cl);
            newApp->getProject()->loadProject( path, f.fileName() );
        }
    }
}

void
Gui::updateRecentFileActions()
{
    QSettings settings;
    QStringList files = settings.value("recentFileList").toStringList();
    int numRecentFiles = std::min(files.size(), (int)NATRON_MAX_RECENT_FILES);

    for (int i = 0; i < numRecentFiles; ++i) {
        QString text = tr("&%1 %2").arg(i + 1).arg( QFileInfo(files[i]).fileName() );
        _imp->actionsOpenRecentFile[i]->setText(text);
        _imp->actionsOpenRecentFile[i]->setData(files[i]);
        _imp->actionsOpenRecentFile[i]->setVisible(true);
    }
    for (int j = numRecentFiles; j < NATRON_MAX_RECENT_FILES; ++j) {
        _imp->actionsOpenRecentFile[j]->setVisible(false);
    }
}

QPixmap
Gui::screenShot(QWidget* w)
{
#if QT_VERSION < 0x050000
    if (w->objectName() == "CurveEditor") {
        return QPixmap::grabWidget(w);
    }

    return QPixmap::grabWindow( w->winId() );
#else

    return QApplication::primaryScreen()->grabWindow( w->winId() );
#endif
}

void
Gui::onProjectNameChanged(const QString & name)
{
    QString text(QCoreApplication::applicationName() + " - ");

    text.append(name);
    setWindowTitle(text);
}

void
Gui::setColorPickersColor(double r,double g, double b,double a)
{
    assert(_imp->_projectGui);
    _imp->_projectGui->setPickersColor(r,g,b,a);
}

void
Gui::registerNewColorPicker(boost::shared_ptr<Color_Knob> knob)
{
    assert(_imp->_projectGui);
    _imp->_projectGui->registerNewColorPicker(knob);
}

void
Gui::removeColorPicker(boost::shared_ptr<Color_Knob> knob)
{
    assert(_imp->_projectGui);
    _imp->_projectGui->removeColorPicker(knob);
}

bool
Gui::hasPickers() const
{
    assert(_imp->_projectGui);

    return _imp->_projectGui->hasPickers();
}

void
Gui::updateViewersViewsMenu(int viewsCount)
{
    QMutexLocker l(&_imp->_viewerTabsMutex);

    for (std::list<ViewerTab*>::iterator it = _imp->_viewerTabs.begin(); it != _imp->_viewerTabs.end(); ++it) {
        (*it)->updateViewsMenu(viewsCount);
    }
}

void
Gui::setViewersCurrentView(int view)
{
    QMutexLocker l(&_imp->_viewerTabsMutex);

    for (std::list<ViewerTab*>::iterator it = _imp->_viewerTabs.begin(); it != _imp->_viewerTabs.end(); ++it) {
        (*it)->setCurrentView(view);
    }
}

const std::list<ViewerTab*> &
Gui::getViewersList() const
{
    return _imp->_viewerTabs;
}

std::list<ViewerTab*>
Gui::getViewersList_mt_safe() const
{
    QMutexLocker l(&_imp->_viewerTabsMutex);

    return _imp->_viewerTabs;
}

void
Gui::setMasterSyncViewer(ViewerTab* master)
{
    QMutexLocker l(&_imp->_viewerTabsMutex);
    _imp->_masterSyncViewer = master;
}

ViewerTab*
Gui::getMasterSyncViewer() const
{
    QMutexLocker l(&_imp->_viewerTabsMutex);
    return _imp->_masterSyncViewer;
}

void
Gui::activateViewerTab(ViewerInstance* viewer)
{
    OpenGLViewerI* viewport = viewer->getUiContext();

    {
        QMutexLocker l(&_imp->_viewerTabsMutex);
        for (std::list<ViewerTab*>::iterator it = _imp->_viewerTabs.begin(); it != _imp->_viewerTabs.end(); ++it) {
            if ( (*it)->getViewer() == viewport ) {
                TabWidget* viewerAnchor = getAnchor();
                assert(viewerAnchor);
                viewerAnchor->appendTab(*it, *it);
                (*it)->show();
            }
        }
    }
    Q_EMIT viewersChanged();
}

void
Gui::deactivateViewerTab(ViewerInstance* viewer)
{
    OpenGLViewerI* viewport = viewer->getUiContext();
    ViewerTab* v = 0;
    {
        QMutexLocker l(&_imp->_viewerTabsMutex);
        for (std::list<ViewerTab*>::iterator it = _imp->_viewerTabs.begin(); it != _imp->_viewerTabs.end(); ++it) {
            if ( (*it)->getViewer() == viewport ) {
                v = *it;
                break;
            }
        }
        
        if (v && v == _imp->_masterSyncViewer) {
            _imp->_masterSyncViewer = 0;
        }
    }

    if (v) {
        removeViewerTab(v, true, false);
    }
}

ViewerTab*
Gui::getViewerTabForInstance(ViewerInstance* node) const
{
    QMutexLocker l(&_imp->_viewerTabsMutex);

    for (std::list<ViewerTab*>::const_iterator it = _imp->_viewerTabs.begin(); it != _imp->_viewerTabs.end(); ++it) {
        if ( (*it)->getInternalNode() == node ) {
            return *it;
        }
    }

    return NULL;
}

const std::list<boost::shared_ptr<NodeGui> > &
Gui::getVisibleNodes() const
{
    return _imp->_nodeGraphArea->getAllActiveNodes();
}

std::list<boost::shared_ptr<NodeGui> >
Gui::getVisibleNodes_mt_safe() const
{
    return _imp->_nodeGraphArea->getAllActiveNodes_mt_safe();
}

void
Gui::deselectAllNodes() const
{
    _imp->_nodeGraphArea->deselect();
}

void
Gui::onProcessHandlerStarted(const QString & sequenceName,
                             int firstFrame,
                             int lastFrame,
                             const boost::shared_ptr<ProcessHandler> & process)
{
    ///make the dialog which will show the progress
    RenderingProgressDialog *dialog = new RenderingProgressDialog(this, sequenceName, firstFrame, lastFrame, process, this);
    QObject::connect(dialog,SIGNAL(accepted()),this,SLOT(onRenderProgressDialogFinished()));
    QObject::connect(dialog,SIGNAL(rejected()),this,SLOT(onRenderProgressDialogFinished()));
    dialog->show();
}

void
Gui::onRenderProgressDialogFinished()
{
    RenderingProgressDialog* dialog = qobject_cast<RenderingProgressDialog*>(sender());
    if (!dialog) {
        return;
    }
    dialog->close();
    dialog->deleteLater();
}

void
Gui::setNextViewerAnchor(TabWidget* where)
{
    _imp->_nextViewerTabPlace = where;
}

const std::vector<ToolButton*> &
Gui::getToolButtons() const
{
    return _imp->_toolButtons;
}

GuiAppInstance*
Gui::getApp() const
{
    return _imp->_appInstance;
}

const std::list<TabWidget*> &
Gui::getPanes() const
{
    return _imp->_panes;
}

std::list<TabWidget*>
Gui::getPanes_mt_safe() const
{
    QMutexLocker l(&_imp->_panesMutex);

    return _imp->_panes;
}

int
Gui::getPanesCount() const
{
    QMutexLocker l(&_imp->_panesMutex);

    return (int)_imp->_panes.size();
}

QString
Gui::getAvailablePaneName(const QString & baseName) const
{
    QString name = baseName;
    QMutexLocker l(&_imp->_panesMutex);
    int baseNumber = _imp->_panes.size();

    if ( name.isEmpty() ) {
        name.append("pane");
        name.append( QString::number(baseNumber) );
    }

    for (;; ) {
        bool foundName = false;
        for (std::list<TabWidget*>::const_iterator it = _imp->_panes.begin(); it != _imp->_panes.end(); ++it) {
            if ( (*it)->objectName_mt_safe() == name ) {
                foundName = true;
                break;
            }
        }
        if (foundName) {
            ++baseNumber;
            name = QString("pane%1").arg(baseNumber);
        } else {
            break;
        }
    }

    return name;
}

void
Gui::setDraftRenderEnabled(bool b)
{
    QMutexLocker k(&_imp->_isInDraftModeMutex);
    _imp->_isInDraftMode = b;
}

bool
Gui::isDraftRenderEnabled() const
{
    QMutexLocker k(&_imp->_isInDraftModeMutex);
    return _imp->_isInDraftMode;
}

bool
Gui::isDraggingPanel() const
{
    return _imp->_currentlyDraggedPanel != NULL;
}

NodeGraph*
Gui::getNodeGraph() const
{
    return _imp->_nodeGraphArea;
}

CurveEditor*
Gui::getCurveEditor() const
{
    return _imp->_curveEditor;
}

DopeSheetEditor *Gui::getDopeSheetEditor() const
{
    return _imp->_dopeSheetEditor;
}

ScriptEditor*
Gui::getScriptEditor() const
{
    return _imp->_scriptEditor;
}

PropertiesBinWrapper*
Gui::getPropertiesBin() const
{
    return _imp->_propertiesBin;
}

QVBoxLayout*
Gui::getPropertiesLayout() const
{
    return _imp->_layoutPropertiesBin;
}

void
Gui::appendTabToDefaultViewerPane(QWidget* tab,
                                  ScriptObject* obj)
{
    TabWidget* viewerAnchor = getAnchor();

    assert(viewerAnchor);
    viewerAnchor->appendTab(tab, obj);
}

QWidget*
Gui::getCentralWidget() const
{
    std::list<QWidget*> children;

    _imp->_leftRightSplitter->getChildren_mt_safe(children);
    if (children.size() != 2) {
        ///something is wrong
        return NULL;
    }
    for (std::list<QWidget*>::iterator it = children.begin(); it != children.end(); ++it) {
        if (*it == _imp->_toolBox) {
            continue;
        }

        return *it;
    }

    return NULL;
}

const RegisteredTabs &
Gui::getRegisteredTabs() const
{
    return _imp->_registeredTabs;
}

void
Gui::debugImage(const Natron::Image* image,
                const RectI& roi,
                const QString & filename )
{
    if (image->getBitDepth() != Natron::eImageBitDepthFloat) {
        qDebug() << "Debug image only works on float images.";
        return;
    }
    RectI renderWindow;
    RectI bounds = image->getBounds();
    if (roi.isNull()) {
        renderWindow = bounds;
    } else {
        if (!roi.intersect(bounds,&renderWindow)) {
            qDebug() << "The RoI does not interesect the bounds of the image.";
            return;
        }
    }
    QImage output(renderWindow.width(), renderWindow.height(), QImage::Format_ARGB32);
    const Natron::Color::Lut* lut = Natron::Color::LutManager::sRGBLut();
    lut->validate();
    Natron::Image::ReadAccess acc = image->getReadRights();
    const float* from = (const float*)acc.pixelAt( renderWindow.left(), renderWindow.bottom() );
    assert(from);
    int srcNComps = (int)image->getComponentsCount();
    int srcRowElements = srcNComps * bounds.width();
    
    for (int y = renderWindow.height() - 1; y >= 0; --y,
         from += (srcRowElements - srcNComps * renderWindow.width())) {
        
        QRgb* dstPixels = (QRgb*)output.scanLine(y);
        assert(dstPixels);
        
        unsigned error_r = 0x80;
        unsigned error_g = 0x80;
        unsigned error_b = 0x80;
        
        for (int x = 0; x < renderWindow.width(); ++x, from += srcNComps, ++dstPixels) {
            float r,g,b,a;
            switch (srcNComps) {
                case 1:
                    r = g = b = *from;
                    a = 1;
                    break;
                case 2:
                    r = *from;
                    g = *(from + 1);
                    b = 0;
                    a = 1;
                    break;
                case 3:
                    r = *from;
                    g = *(from + 1);
                    b = *(from + 2);
                    a = 1;
                    break;
                case 4:
                    r = *from;
                    g = *(from + 1);
                    b = *(from + 2);
                    a = *(from + 3);
                    break;
                default:
                    assert(false);
                    return;
            }
            error_r = (error_r & 0xff) + lut->toColorSpaceUint8xxFromLinearFloatFast(r);
            error_g = (error_g & 0xff) + lut->toColorSpaceUint8xxFromLinearFloatFast(g);
            error_b = (error_b & 0xff) + lut->toColorSpaceUint8xxFromLinearFloatFast(b);
            assert(error_r < 0x10000 && error_g < 0x10000 && error_b < 0x10000);
            *dstPixels = qRgba(U8(error_r >> 8),
                              U8(error_g >> 8),
                              U8(error_b >> 8),
                              U8(a * 255));
        }
    }
    
    U64 hashKey = image->getHashKey();
    QString hashKeyStr = QString::number(hashKey);
    QString realFileName = filename.isEmpty() ? QString(hashKeyStr + ".png") : filename;
    qDebug() << "Writing image: " << realFileName;
    renderWindow.debug();
    output.save(realFileName);
}

void
Gui::updateLastSequenceOpenedPath(const QString & path)
{
    _imp->_lastLoadSequenceOpenedDir = path;
}

void
Gui::updateLastSequenceSavedPath(const QString & path)
{
    _imp->_lastSaveSequenceOpenedDir = path;
}

void
Gui::updateLastSavedProjectPath(const QString & project)
{
    _imp->_lastSaveProjectOpenedDir = project;
}

void
Gui::updateLastOpenedProjectPath(const QString & project)
{
    _imp->_lastLoadProjectOpenedDir = project;
}

void
Gui::onWriterRenderStarted(const QString & sequenceName,
                           int firstFrame,
                           int lastFrame,
                           Natron::OutputEffectInstance* writer)
{
    assert( QThread::currentThread() == qApp->thread() );

    RenderingProgressDialog *dialog = new RenderingProgressDialog(this, sequenceName, firstFrame, lastFrame,
                                                                  boost::shared_ptr<ProcessHandler>(), this);
    RenderEngine* engine = writer->getRenderEngine();
    QObject::connect( dialog, SIGNAL( canceled() ), engine, SLOT( abortRendering_Blocking() ) );
    QObject::connect( engine, SIGNAL( frameRendered(int) ), dialog, SLOT( onFrameRendered(int) ) );
    QObject::connect( engine, SIGNAL( frameRenderedWithTimer(int,double,double) ), dialog, SLOT( onFrameRenderedWithTimer(int,double,double) ) );
    QObject::connect( engine, SIGNAL( renderFinished(int) ), dialog, SLOT( onVideoEngineStopped(int) ) );
    QObject::connect(dialog,SIGNAL(accepted()),this,SLOT(onRenderProgressDialogFinished()));
    QObject::connect(dialog,SIGNAL(rejected()),this,SLOT(onRenderProgressDialogFinished()));
    dialog->show();
}

void
Gui::setGlewVersion(const QString & version)
{
    _imp->_glewVersion = version;
    _imp->_aboutWindow->updateLibrariesVersions();
}

void
Gui::setOpenGLVersion(const QString & version)
{
    _imp->_openGLVersion = version;
    _imp->_aboutWindow->updateLibrariesVersions();
}

QString
Gui::getGlewVersion() const
{
    return _imp->_glewVersion;
}

QString
Gui::getOpenGLVersion() const
{
    return _imp->_openGLVersion;
}

QString
Gui::getBoostVersion() const
{
    return QString(BOOST_LIB_VERSION);
}

QString
Gui::getQtVersion() const
{
    return QString(QT_VERSION_STR) + " / " + qVersion();
}

QString
Gui::getCairoVersion() const
{
    return QString(CAIRO_VERSION_STRING) + " / " + QString( cairo_version_string() );
}

void
Gui::onNodeNameChanged(const QString & /*name*/)
{
    Natron::Node* node = qobject_cast<Natron::Node*>( sender() );

    if (!node) {
        return;
    }
    ViewerInstance* isViewer = dynamic_cast<ViewerInstance*>( node->getLiveInstance() );
    if (isViewer) {
        Q_EMIT viewersChanged();
    }
}

void
Gui::renderAllWriters()
{
    _imp->_appInstance->startWritersRendering( std::list<AppInstance::RenderRequest>() );
}

void
Gui::renderSelectedNode()
{
    NodeGraph* graph = getLastSelectedGraph();
    if (!graph) {
        return;
    }
    
    const std::list<boost::shared_ptr<NodeGui> > & selectedNodes = graph->getSelectedNodes();

    if ( selectedNodes.empty() ) {
        Natron::warningDialog( tr("Render").toStdString(), tr("You must select a node to render first!").toStdString() );
    } else {
        std::list<AppInstance::RenderWork> workList;

        for (std::list<boost::shared_ptr<NodeGui> >::const_iterator it = selectedNodes.begin();
             it != selectedNodes.end(); ++it) {
            Natron::EffectInstance* effect = (*it)->getNode()->getLiveInstance();
            assert(effect);
            if (effect->isWriter()) {
                if (!effect->areKnobsFrozen()) {
                    //if ((*it)->getNode()->is)
                    ///if the node is a writer, just use it to render!
                    AppInstance::RenderWork w;
                    w.writer = dynamic_cast<Natron::OutputEffectInstance*>(effect);
                    assert(w.writer);
                    w.firstFrame = INT_MIN;
                    w.lastFrame = INT_MAX;
                    workList.push_back(w);
                }
            } else {
                if (selectedNodes.size() == 1) {
                    ///create a node and connect it to the node and use it to render
                    boost::shared_ptr<Natron::Node> writer = createWriter();
                    if (writer) {
                        AppInstance::RenderWork w;
                        w.writer = dynamic_cast<Natron::OutputEffectInstance*>( writer->getLiveInstance() );
                        assert(w.writer);
                        w.firstFrame = INT_MIN;
                        w.lastFrame = INT_MAX;
                        workList.push_back(w);
                    }
                }
            }
        }
        _imp->_appInstance->startWritersRendering(workList);
    }
}

void
Gui::setUndoRedoStackLimit(int limit)
{
    _imp->_nodeGraphArea->setUndoRedoStackLimit(limit);
}

void
Gui::showOfxLog()
{
    QString log = appPTR->getOfxLog_mt_safe();
    LogWindow lw(log, this);

    lw.setWindowTitle( tr("Error Log") );
    ignore_result( lw.exec() );
}

void
Gui::createNewTrackerInterface(NodeGui* n)
{
    QMutexLocker l(&_imp->_viewerTabsMutex);

    for (std::list<ViewerTab*>::iterator it = _imp->_viewerTabs.begin(); it != _imp->_viewerTabs.end(); ++it) {
        (*it)->createTrackerInterface(n);
    }
}

void
Gui::removeTrackerInterface(NodeGui* n,
                            bool permanently)
{
    QMutexLocker l(&_imp->_viewerTabsMutex);

    for (std::list<ViewerTab*>::iterator it = _imp->_viewerTabs.begin(); it != _imp->_viewerTabs.end(); ++it) {
        (*it)->removeTrackerInterface(n, permanently, false);
    }
}

void
Gui::onRotoSelectedToolChanged(int tool)
{
    RotoGui* roto = qobject_cast<RotoGui*>( sender() );

    if (!roto) {
        return;
    }
    QMutexLocker l(&_imp->_viewerTabsMutex);
    for (std::list<ViewerTab*>::iterator it = _imp->_viewerTabs.begin(); it != _imp->_viewerTabs.end(); ++it) {
        (*it)->updateRotoSelectedTool(tool, roto);
    }
}

void
Gui::createNewRotoInterface(NodeGui* n)
{
    QMutexLocker l(&_imp->_viewerTabsMutex);

    for (std::list<ViewerTab*>::iterator it = _imp->_viewerTabs.begin(); it != _imp->_viewerTabs.end(); ++it) {
        (*it)->createRotoInterface(n);
    }
}

void
Gui::removeRotoInterface(NodeGui* n,
                         bool permanently)
{
    QMutexLocker l(&_imp->_viewerTabsMutex);

    for (std::list<ViewerTab*>::iterator it = _imp->_viewerTabs.begin(); it != _imp->_viewerTabs.end(); ++it) {
        (*it)->removeRotoInterface(n, permanently, false);
    }
}

void
Gui::setRotoInterface(NodeGui* n)
{
    QMutexLocker l(&_imp->_viewerTabsMutex);

    for (std::list<ViewerTab*>::iterator it = _imp->_viewerTabs.begin(); it != _imp->_viewerTabs.end(); ++it) {
        (*it)->setRotoInterface(n);
    }
}

void
Gui::onViewerRotoEvaluated(ViewerTab* viewer)
{
    QMutexLocker l(&_imp->_viewerTabsMutex);

    for (std::list<ViewerTab*>::iterator it = _imp->_viewerTabs.begin(); it != _imp->_viewerTabs.end(); ++it) {
        if (*it != viewer) {
            (*it)->getViewer()->redraw();
        }
    }
}

void
Gui::startProgress(KnobHolder* effect,
                   const std::string & message,
                   bool canCancel)
{
    if (!effect) {
        return;
    }
    if ( QThread::currentThread() != qApp->thread() ) {
        qDebug() << "Progress bars called from a thread different than the main-thread is not supported at the moment.";

        return;
    }

    QProgressDialog* dialog = new QProgressDialog(message.c_str(), tr("Cancel"), 0, 100, this);
    if (!canCancel) {
        dialog->setCancelButton(0);
    }
    dialog->setModal(false);
    dialog->setRange(0, 100);
    dialog->setMinimumWidth(250);
    NamedKnobHolder* isNamed = dynamic_cast<NamedKnobHolder*>(effect);
    if (isNamed) {
        dialog->setWindowTitle( isNamed->getScriptName_mt_safe().c_str() );
    }
    std::map<KnobHolder*, QProgressDialog*>::iterator found = _imp->_progressBars.find(effect);

    ///If a second dialog was asked for whilst another is still active, the first dialog will not be
    ///able to be canceled.
    if ( found != _imp->_progressBars.end() ) {
        _imp->_progressBars.erase(found);
    }

    _imp->_progressBars.insert( std::make_pair(effect, dialog) );
    dialog->show();
    //dialog->exec();
}

void
Gui::endProgress(KnobHolder* effect)
{
    if ( QThread::currentThread() != qApp->thread() ) {
        qDebug() << "Progress bars called from a thread different than the main-thread is not supported at the moment.";

        return;
    }

    std::map<KnobHolder*, QProgressDialog*>::iterator found = _imp->_progressBars.find(effect);
    if ( found == _imp->_progressBars.end() ) {
        return;
    }


    found->second->close();
    _imp->_progressBars.erase(found);
}

bool
Gui::progressUpdate(KnobHolder* effect,
                    double t)
{
    if ( QThread::currentThread() != qApp->thread() ) {
        qDebug() << "Progress bars called from a thread different than the main-thread is not supported at the moment.";

        return true;
    }

    std::map<KnobHolder*, QProgressDialog*>::iterator found = _imp->_progressBars.find(effect);
    if ( found == _imp->_progressBars.end() ) {
        NamedKnobHolder* isNamed = dynamic_cast<NamedKnobHolder*>(effect);
        if (isNamed) {
            qDebug() << isNamed->getScriptName_mt_safe().c_str() <<  " called progressUpdate but didn't called startProgress first.";
        }
    } else {
        if ( found->second->wasCanceled() ) {
            return false;
        }
        found->second->setValue(t * 100);
    }
    //QCoreApplication::processEvents();

    return true;
}

void
Gui::addVisibleDockablePanel(DockablePanel* panel)
{
    
    std::list<DockablePanel*>::iterator it = std::find(_imp->openedPanels.begin(), _imp->openedPanels.end(), panel);
    putSettingsPanelFirst(panel);
    if ( it == _imp->openedPanels.end() ) {
        assert(panel);
        int maxPanels = appPTR->getCurrentSettings()->getMaxPanelsOpened();
        if ( ( (int)_imp->openedPanels.size() == maxPanels ) && (maxPanels != 0) ) {
            std::list<DockablePanel*>::iterator it = _imp->openedPanels.begin();
            (*it)->closePanel();
        }
        _imp->openedPanels.push_back(panel);
    }
}

void
Gui::removeVisibleDockablePanel(DockablePanel* panel)
{
    std::list<DockablePanel*>::iterator it = std::find(_imp->openedPanels.begin(), _imp->openedPanels.end(), panel);

    if ( it != _imp->openedPanels.end() ) {
        _imp->openedPanels.erase(it);
    }
}

const std::list<DockablePanel*>&
Gui::getVisiblePanels() const
{
    return _imp->openedPanels;
}

void
Gui::onMaxVisibleDockablePanelChanged(int maxPanels)
{
    assert(maxPanels >= 0);
    if (maxPanels == 0) {
        return;
    }
    while ( (int)_imp->openedPanels.size() > maxPanels ) {
        std::list<DockablePanel*>::iterator it = _imp->openedPanels.begin();
        (*it)->closePanel();
    }
    _imp->_maxPanelsOpenedSpinBox->setValue(maxPanels);
}

void
Gui::onMaxPanelsSpinBoxValueChanged(double val)
{
    appPTR->getCurrentSettings()->setMaxPanelsOpened( (int)val );
}

void
Gui::clearAllVisiblePanels()
{
    while ( !_imp->openedPanels.empty() ) {
        std::list<DockablePanel*>::iterator it = _imp->openedPanels.begin();
        if ( !(*it)->isFloating() ) {
            (*it)->setClosed(true);
        }

        bool foundNonFloating = false;
        for (std::list<DockablePanel*>::iterator it2 = _imp->openedPanels.begin(); it2 != _imp->openedPanels.end(); ++it2) {
            if ( !(*it2)->isFloating() ) {
                foundNonFloating = true;
                break;
            }
        }
        ///only floating windows left
        if (!foundNonFloating) {
            break;
        }
    }
    getApp()->redrawAllViewers();
}

void
Gui::minimizeMaximizeAllPanels(bool clicked)
{
    for (std::list<DockablePanel*>::iterator it = _imp->openedPanels.begin(); it != _imp->openedPanels.end(); ++it) {
        if (clicked) {
            if ( !(*it)->isMinimized() ) {
                (*it)->minimizeOrMaximize(true);
            }
        } else {
            if ( (*it)->isMinimized() ) {
                (*it)->minimizeOrMaximize(false);
            }
        }
    }
    getApp()->redrawAllViewers();
}

void
Gui::connectViewersToViewerCache()
{
    QMutexLocker l(&_imp->_viewerTabsMutex);

    for (std::list<ViewerTab*>::iterator it = _imp->_viewerTabs.begin(); it != _imp->_viewerTabs.end(); ++it) {
        (*it)->connectToViewerCache();
    }
}

void
Gui::disconnectViewersFromViewerCache()
{
    QMutexLocker l(&_imp->_viewerTabsMutex);

    for (std::list<ViewerTab*>::iterator it = _imp->_viewerTabs.begin(); it != _imp->_viewerTabs.end(); ++it) {
        (*it)->disconnectFromViewerCache();
    }
}

void
Gui::moveEvent(QMoveEvent* e)
{
    QMainWindow::moveEvent(e);
    QPoint p = pos();

    setMtSafePosition( p.x(), p.y() );
}


#if 0
bool
Gui::event(QEvent* e)
{
    switch (e->type()) {
        case QEvent::TabletEnterProximity:
        case QEvent::TabletLeaveProximity:
        case QEvent::TabletMove:
        case QEvent::TabletPress:
        case QEvent::TabletRelease:
        {
            QTabletEvent *tEvent = dynamic_cast<QTabletEvent *>(e);
            const std::list<ViewerTab*>& viewers = getViewersList();
            for (std::list<ViewerTab*>::const_iterator it = viewers.begin(); it!=viewers.end(); ++it) {
                QPoint widgetPos = (*it)->mapToGlobal((*it)->mapFromParent((*it)->pos()));
                QRect r(widgetPos.x(),widgetPos.y(),(*it)->width(),(*it)->height());
                if (r.contains(tEvent->globalPos())) {
                    QTabletEvent te(tEvent->type()
                                    , mapFromGlobal(tEvent->pos())
                                    , tEvent->globalPos()
                                    , tEvent->hiResGlobalPos()
                                    , tEvent->device()
                                    , tEvent->pointerType()
                                    , tEvent->pressure()
                                    , tEvent->xTilt()
                                    , tEvent->yTilt()
                                    , tEvent->tangentialPressure()
                                    , tEvent->rotation()
                                    , tEvent->z()
                                    , tEvent->modifiers()
                                    , tEvent->uniqueId());
                    qApp->sendEvent((*it)->getViewer(), &te);
                    return true;
                }
            }
            break;
        }
        default:
            break;
    }
    return QMainWindow::event(e);
}
#endif
void
Gui::resizeEvent(QResizeEvent* e)
{
    QMainWindow::resizeEvent(e);

    setMtSafeWindowSize( width(), height() );
}

void
Gui::keyPressEvent(QKeyEvent* e)
{
    QWidget* w = qApp->widgetAt( QCursor::pos() );

    if ( w && ( w->objectName() == QString("SettingsPanel") ) && (e->key() == Qt::Key_Escape) ) {
        RightClickableWidget* panel = dynamic_cast<RightClickableWidget*>(w);
        assert(panel);
        panel->getPanel()->closePanel();
    }

    Qt::Key key = (Qt::Key)e->key();
    Qt::KeyboardModifiers modifiers = e->modifiers();

    if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerPrevious, modifiers, key) ) {
        if ( getNodeGraph()->getLastSelectedViewer() ) {
            getNodeGraph()->getLastSelectedViewer()->previousFrame();
        }
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerNext, modifiers, key) ) {
        if ( getNodeGraph()->getLastSelectedViewer() ) {
            getNodeGraph()->getLastSelectedViewer()->nextFrame();
        }
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerFirst, modifiers, key) ) {
        if ( getNodeGraph()->getLastSelectedViewer() ) {
            getNodeGraph()->getLastSelectedViewer()->firstFrame();
        }
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerLast, modifiers, key) ) {
        if ( getNodeGraph()->getLastSelectedViewer() ) {
            getNodeGraph()->getLastSelectedViewer()->lastFrame();
        }
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerPrevIncr, modifiers, key) ) {
        if ( getNodeGraph()->getLastSelectedViewer() ) {
            getNodeGraph()->getLastSelectedViewer()->previousIncrement();
        }
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerNextIncr, modifiers, key) ) {
        if ( getNodeGraph()->getLastSelectedViewer() ) {
            getNodeGraph()->getLastSelectedViewer()->nextIncrement();
        }
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerPrevKF, modifiers, key) ) {
        getApp()->getTimeLine()->goToPreviousKeyframe();
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerNextKF, modifiers, key) ) {
        getApp()->getTimeLine()->goToNextKeyframe();
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphDisableNodes, modifiers, key) ) {
        _imp->_nodeGraphArea->toggleSelectedNodesEnabled();
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphFindNode, modifiers, key) ) {
        _imp->_nodeGraphArea->popFindDialog();
    } else {
        QMainWindow::keyPressEvent(e);
    }
}

TabWidget*
Gui::getAnchor() const
{
    QMutexLocker l(&_imp->_panesMutex);

    for (std::list<TabWidget*>::const_iterator it = _imp->_panes.begin(); it != _imp->_panes.end(); ++it) {
        if ( (*it)->isAnchor() ) {
            return *it;
        }
    }

    return NULL;
}

bool
Gui::isGUIFrozen() const
{
    QMutexLocker k(&_imp->_isGUIFrozenMutex);

    return _imp->_isGUIFrozen;
}

void
Gui::onFreezeUIButtonClicked(bool clicked)
{
    {
        QMutexLocker k(&_imp->_isGUIFrozenMutex);
        if (_imp->_isGUIFrozen == clicked) {
            return;
        }
        _imp->_isGUIFrozen = clicked;
    }
    {
        QMutexLocker k(&_imp->_viewerTabsMutex);
        for (std::list<ViewerTab*>::iterator it = _imp->_viewerTabs.begin(); it != _imp->_viewerTabs.end(); ++it) {
            (*it)->setTurboButtonDown(clicked);
            if (!clicked) {
                (*it)->getViewer()->redraw(); //< overlays were disabled while frozen, redraw to make them re-appear
            }
        }
    }
    _imp->_nodeGraphArea->onGuiFrozenChanged(clicked);
    for (std::list<NodeGraph*>::iterator it = _imp->_groups.begin(); it != _imp->_groups.end(); ++it) {
        (*it)->onGuiFrozenChanged(clicked);
    }
}

bool
Gui::hasShortcutEditorAlreadyBeenBuilt() const
{
    return _imp->shortcutEditor != NULL;
}

void
Gui::addShortcut(BoundAction* action)
{
    assert(_imp->shortcutEditor);
    _imp->shortcutEditor->addShortcut(action);
}

void
Gui::getNodesEntitledForOverlays(std::list<boost::shared_ptr<Natron::Node> > & nodes) const
{
    int layoutItemsCount = _imp->_layoutPropertiesBin->count();

    for (int i = 0; i < layoutItemsCount; ++i) {
        QLayoutItem* item = _imp->_layoutPropertiesBin->itemAt(i);
        NodeSettingsPanel* panel = dynamic_cast<NodeSettingsPanel*>( item->widget() );
        if (panel) {
            boost::shared_ptr<NodeGui> node = panel->getNode();
            boost::shared_ptr<Natron::Node> internalNode = node->getNode();
            if (node && internalNode) {
                boost::shared_ptr<MultiInstancePanel> multiInstance = node->getMultiInstancePanel();
                if (multiInstance) {
//                    const std::list< std::pair<boost::weak_ptr<Natron::Node>,bool > >& instances = multiInstance->getInstances();
//                    for (std::list< std::pair<boost::weak_ptr<Natron::Node>,bool > >::const_iterator it = instances.begin(); it != instances.end(); ++it) {
//                        NodePtr instance = it->first.lock();
//                        if (node->isSettingsPanelVisible() &&
//                            !node->isSettingsPanelMinimized() &&
//                            instance->isActivated() &&
//                            instance->hasOverlay() &&
//                            it->second &&
//                            !instance->isNodeDisabled()) {
//                            nodes.push_back(instance);
//                        }
//                    }
                    if ( internalNode->hasOverlay() &&
                         !internalNode->isNodeDisabled() &&
                         node->isSettingsPanelVisible() &&
                         !node->isSettingsPanelMinimized() ) {
                        nodes.push_back( node->getNode() );
                    }
                } else {
                    if ( ( internalNode->hasOverlay() || internalNode->getRotoContext() ) &&
                         !internalNode->isNodeDisabled() &&
                        !internalNode->getParentMultiInstance() && 
                         internalNode->isActivated() &&
                         node->isSettingsPanelVisible() &&
                         !node->isSettingsPanelMinimized() ) {
                        nodes.push_back( node->getNode() );
                    }
                }
            }
        }
    }
}

void
Gui::redrawAllViewers()
{
    QMutexLocker k(&_imp->_viewerTabsMutex);

    for (std::list<ViewerTab*>::const_iterator it = _imp->_viewerTabs.begin(); it != _imp->_viewerTabs.end(); ++it) {
        if ( (*it)->isVisible() ) {
            (*it)->getViewer()->redraw();
        }
    }
}

void
Gui::renderAllViewers()
{
    assert(QThread::currentThread() == qApp->thread());
    for (std::list<ViewerTab*>::const_iterator it = _imp->_viewerTabs.begin(); it != _imp->_viewerTabs.end(); ++it) {
        if ( (*it)->isVisible() ) {
            (*it)->getInternalNode()->renderCurrentFrame(true);
        }
    }
}

void
Gui::toggleAutoHideGraphInputs()
{
    _imp->_nodeGraphArea->toggleAutoHideInputs(false);
}

void
Gui::centerAllNodeGraphsWithTimer()
{
    QTimer::singleShot( 25, _imp->_nodeGraphArea, SLOT( centerOnAllNodes() ) );

    for (std::list<NodeGraph*>::iterator it = _imp->_groups.begin(); it != _imp->_groups.end(); ++it) {
        QTimer::singleShot( 25, *it, SLOT( centerOnAllNodes() ) );
    }
}

void
Gui::setLastEnteredTabWidget(TabWidget* tab)
{
    _imp->_lastEnteredTabWidget = tab;
}

TabWidget*
Gui::getLastEnteredTabWidget() const
{
    return _imp->_lastEnteredTabWidget;
}

void
Gui::onPrevTabTriggered()
{
    TabWidget* t = getLastEnteredTabWidget();
    
    if (t) {
        t->moveToPreviousTab();
    }

}

void
Gui::onNextTabTriggered()
{
    TabWidget* t = getLastEnteredTabWidget();

    if (t) {
        t->moveToNextTab();
    }
}

void
Gui::onCloseTabTriggered()
{
    TabWidget* t = getLastEnteredTabWidget();

    if (t) {
        t->closeCurrentWidget();
    }
}

void
Gui::appendToScriptEditor(const std::string & str)
{
    _imp->_scriptEditor->appendToScriptEditor( str.c_str() );
}

void
Gui::printAutoDeclaredVariable(const std::string & str)
{
    _imp->_scriptEditor->printAutoDeclaredVariable( str.c_str() );
}

void
Gui::exportGroupAsPythonScript(NodeCollection* collection)
{
    assert(collection);
    NodeList nodes = collection->getNodes();
    bool hasOutput = false;
    for (NodeList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        if ( (*it)->isActivated() && dynamic_cast<GroupOutput*>( (*it)->getLiveInstance() ) ) {
            hasOutput = true;
            break;
        }
    }

    if (!hasOutput) {
        Natron::errorDialog( tr("Export").toStdString(), tr("To export as group, at least one Ouptut node must exist.").toStdString() );

        return;
    }
    ExportGroupTemplateDialog dialog(collection, this, this);
    ignore_result( dialog.exec() );
}

void
Gui::exportProjectAsGroup()
{
    exportGroupAsPythonScript( getApp()->getProject().get() );
}


void
Gui::onUserCommandTriggered()
{
    QAction* action = qobject_cast<QAction*>( sender() );

    if (!action) {
        return;
    }
    ActionWithShortcut* aws = dynamic_cast<ActionWithShortcut*>(action);
    if (!aws) {
        return;
    }
    std::map<ActionWithShortcut*, std::string>::iterator found = _imp->pythonCommands.find(aws);
    if ( found != _imp->pythonCommands.end() ) {
        std::string err;
        std::string output;
        if ( !Natron::interpretPythonScript(found->second, &err, &output) ) {
            getApp()->appendToScriptEditor(err);
        } else {
            getApp()->appendToScriptEditor(output);
        }
    }
}

void
Gui::addMenuEntry(const QString & menuGrouping,
                  const std::string & pythonFunction,
                  Qt::Key key,
                  const Qt::KeyboardModifiers & modifiers)
{
    QStringList grouping = menuGrouping.split('/');

    if ( grouping.isEmpty() ) {
        getApp()->appendToScriptEditor( tr("Failed to add menu entry for ").toStdString() +
                                       menuGrouping.toStdString() +
                                       tr(": incorrect menu grouping").toStdString() );

        return;
    }

    std::string appID = getApp()->getAppIDString();
    std::string script = "app = " + appID + "\n" + pythonFunction + "()\n";
    QAction* action = _imp->findActionRecursive(0, _imp->menubar, grouping);
    ActionWithShortcut* aws = dynamic_cast<ActionWithShortcut*>(action);
    if (aws) {
        aws->setShortcut( makeKeySequence(modifiers, key) );
        std::map<ActionWithShortcut*, std::string>::iterator found = _imp->pythonCommands.find(aws);
        if ( found != _imp->pythonCommands.end() ) {
            found->second = pythonFunction;
        } else {
            _imp->pythonCommands.insert( std::make_pair(aws, script) );
        }
    }
}

void
Gui::setDopeSheetTreeWidth(int width)
{
    _imp->_dopeSheetEditor->setTreeWidgetWidth(width);
}

void
Gui::setCurveEditorTreeWidth(int width)
{
    _imp->_curveEditor->setTreeWidgetWidth(width);
}

void Gui::setTripleSyncEnabled(bool enabled)
{
    if (_imp->_isTripleSyncEnabled != enabled) {
        _imp->_isTripleSyncEnabled = enabled;
    }
}

bool Gui::isTripleSyncEnabled() const
{
    return _imp->_isTripleSyncEnabled;
}

void Gui::centerOpenedViewersOn(SequenceTime left, SequenceTime right)
{
    const std::list<ViewerTab *> &viewers = getViewersList();

    for (std::list<ViewerTab *>::const_iterator it = viewers.begin(); it != viewers.end(); ++it) {
        ViewerTab *v = (*it);

        v->centerOn_tripleSync(left, right);
    }
}
