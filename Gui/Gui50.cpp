/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
 * Copyright (C) 2018-2020 The Natron developers
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
#include <locale>
#include <utility>
#include <stdexcept>

#if !defined(SBK_RUN) && !defined(Q_MOC_RUN)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
#include <boost/algorithm/string/case_conv.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
#endif

#include <QtCore/QtGlobal> // for Q_OS_*
#include <QtCore/QDebug>
#include <QtCore/QThread>
#include <QtCore/QTimer>

#include <QApplication> // qApp
GCC_DIAG_UNUSED_PRIVATE_FIELD_OFF
// /opt/local/include/QtGui/qmime.h:119:10: warning: private field 'type' is not used [-Wunused-private-field]
#include <QKeyEvent>
GCC_DIAG_UNUSED_PRIVATE_FIELD_ON
#include <QMenuBar>
#include <QToolButton>
#include <QProgressDialog>
#include <QClipboard>
#include <QVBoxLayout>
#include <QTreeWidget>
#include <QThread>
#include <QTabBar>
#include <QTextEdit>
#include <QLineEdit>
#include <QCursor>
#include <QCheckBox>
#include <QTreeView>

#include "Global/QtCompat.h"

#include "Engine/CreateNodeArgs.h"
#include "Engine/GroupOutput.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h" // NodesList, NodeCollection
#include "Engine/Project.h"
#include "Engine/KnobSerialization.h"
#include "Engine/FileSystemModel.h"
#include "Engine/Settings.h"
#include "Engine/TimeLine.h"
#include "Engine/ViewerInstance.h"

#include "Gui/ActionShortcuts.h"
#include "Gui/CurveEditor.h"
#include "Gui/CurveWidget.h"
#include "Gui/ComboBox.h"
#include "Gui/DockablePanel.h"
#include "Gui/DopeSheetEditor.h"
#include "Gui/ExportGroupTemplateDialog.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h" // appPTR
#include "Gui/GuiPrivate.h"
#include "Gui/KnobWidgetDnD.h"
#include "Gui/GuiMacros.h"
#include "Gui/LogWindow.h"
#include "Gui/NodeGraph.h"
#include "Gui/NodeGui.h"
#include "Gui/NodeSettingsPanel.h"
#include "Gui/ProgressPanel.h"
#include "Gui/RightClickableWidget.h"
#include "Gui/ScriptEditor.h"
#include "Gui/SpinBox.h"
#include "Gui/TabWidget.h"
#include "Gui/PreferencesPanel.h"
#include "Gui/ViewerGL.h"
#include "Gui/ViewerTab.h"
#include "Gui/SequenceFileDialog.h"
#include "Gui/PropertiesBinWrapper.h"
#include "Gui/Histogram.h"

NATRON_NAMESPACE_ENTER


void
Gui::setUndoRedoStackLimit(int limit)
{
    _imp->_nodeGraphArea->setUndoRedoStackLimit(limit);
}

void
Gui::onShowLogOnMainThreadReceived()
{
    std::list<LogEntry> log;
    appPTR->getErrorLog_mt_safe(&log);
    assert(_imp->_errorLog);
    _imp->_errorLog->displayLog(log);

    if (!_imp->_errorLog->isVisible()) {
        _imp->_errorLog->show();
        // no need to raise(), because:
        // - the log window is Qt::WindowStaysOnTopHint
        // - raising a window does not work in general:
        // https://forum.qt.io/topic/6032/bring-window-to-front-raise-show-activatewindow-don-t-work-on-windows/12
    }
}

void
Gui::showErrorLog()
{
    if (QThread::currentThread() == qApp->thread()) {
        onShowLogOnMainThreadReceived();
    } else {
        Q_EMIT s_showLogOnMainThread();
    }
}

void
Gui::createNodeViewerInterface(const NodeGuiPtr& n)
{
    QMutexLocker l(&_imp->_viewerTabsMutex);

    for (std::list<ViewerTab*>::iterator it = _imp->_viewerTabs.begin(); it != _imp->_viewerTabs.end(); ++it) {
        (*it)->createNodeViewerInterface(n);
    }
}

void
Gui::removeNodeViewerInterface(const NodeGuiPtr& n,
                               bool permanently)
{
    QMutexLocker l(&_imp->_viewerTabsMutex);

    for (std::list<ViewerTab*>::iterator it = _imp->_viewerTabs.begin(); it != _imp->_viewerTabs.end(); ++it) {
        (*it)->removeNodeViewerInterface(n, permanently, false /*setNewInterface*/);
    }
}

void
Gui::setNodeViewerInterface(const NodeGuiPtr& n)
{
    QMutexLocker l(&_imp->_viewerTabsMutex);

    for (std::list<ViewerTab*>::iterator it = _imp->_viewerTabs.begin(); it != _imp->_viewerTabs.end(); ++it) {
        (*it)->setPluginViewerInterface(n);
    }
}

void
Gui::progressStart(const NodePtr& node,
                   const std::string &message,
                   const std::string &messageid,
                   bool canCancel)
{
    _imp->_progressPanel->progressStart(node, message, messageid, canCancel);
}

void
Gui::progressEnd(const NodePtr& node)
{
    _imp->_progressPanel->endTask(node);
}

bool
Gui::progressUpdate(const NodePtr& node,
                    double t)
{
    return _imp->_progressPanel->updateTask(node, t);
}

void
Gui::onMaxVisibleDockablePanelChanged(int maxPanels)
{
    assert(maxPanels >= 0);
    if (maxPanels == 0) {
        return;
    }
    while ( (int)_imp->openedPanels.size() > maxPanels ) {
        std::list<DockablePanel*>::reverse_iterator it = _imp->openedPanels.rbegin();
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
    // close panels one by one, since closing a panel updates the openedPanels list.
    while ( !_imp->openedPanels.empty() ) {
        bool foundNonFloating = false;

        // close one panel at a time - this changes the openedPanel list, so we must break the loop
        for (std::list<DockablePanel*>::iterator it = _imp->openedPanels.begin(); it != _imp->openedPanels.end(); ++it) {
            if ( !(*it)->isFloating() ) {
                (*it)->setClosed(true);
                foundNonFloating = true;
                break;
            }
        }

        // no panel was closed
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

std::list<DockablePanel*>
Gui::getOpenedPanels(){
    return _imp->openedPanels;
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
    switch ( e->type() ) {
    case QEvent::TabletEnterProximity:
    case QEvent::TabletLeaveProximity:
    case QEvent::TabletMove:
    case QEvent::TabletPress:
    case QEvent::TabletRelease: {
        QTabletEvent *tEvent = dynamic_cast<QTabletEvent *>(e);
        const std::list<ViewerTab*>& viewers = getViewersList();
        for (std::list<ViewerTab*>::const_iterator it = viewers.begin(); it != viewers.end(); ++it) {
            QPoint widgetPos = (*it)->mapToGlobal( (*it)->mapFromParent( (*it)->pos() ) );
            QRect r( widgetPos.x(), widgetPos.y(), (*it)->width(), (*it)->height() );
            if ( r.contains( tEvent->globalPos() ) ) {
                QTabletEvent te( tEvent->type()
                                 , mapFromGlobal( tEvent->pos() )
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
                                 , tEvent->uniqueId() );
                qApp->sendEvent( (*it)->getViewer(), &te );

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
Gui::setLastKeyPressVisitedClickFocus(bool visited)
{
    _imp->keyPressEventHasVisitedFocusWidget = visited;
}

void
Gui::setLastKeyUpVisitedClickFocus(bool visited)
{
    _imp->keyUpEventHasVisitedFocusWidget = visited;
}

/// Handle the viewer keys separately: use the nativeVirtualKey so that they work
/// on any keyboard, including French AZERTY (where numbers are shifted)
int
Gui::handleNativeKeys(int key,
                      quint32 nativeScanCode,
                      quint32 nativeVirtualKey)
{
    //qDebug() << "scancode=" << nativeScanCode << "virtualkey=" << nativeVirtualKey;
    if ( !appPTR->getCurrentSettings()->isViewerKeysEnabled() ) {
        return key;
    }

#ifdef Q_WS_MAC
    // OS X virtual key codes, from
    // MacOSX10.11.sdk/System/Library/Frameworks/Carbon.framework/Frameworks/HIToolbox.framework/Headers/Events.h
    // kVK_ANSI_1                    = 0x12,
    // kVK_ANSI_2                    = 0x13,
    // kVK_ANSI_3                    = 0x14,
    // kVK_ANSI_4                    = 0x15,
    // kVK_ANSI_6                    = 0x16,
    // kVK_ANSI_5                    = 0x17,
    // kVK_ANSI_9                    = 0x19,
    // kVK_ANSI_7                    = 0x1A,
    // kVK_ANSI_8                    = 0x1C,
    // kVK_ANSI_0                    = 0x1D,
    Q_UNUSED(nativeScanCode);
    switch (nativeVirtualKey) {
    case 0x12:

        return Qt::Key_1;
    case 0x13:

        return Qt::Key_2;
    case 0x14:

        return Qt::Key_3;
    case 0x15:

        return Qt::Key_4;
    case 0x16:

        return Qt::Key_6;
    case 0x17:

        return Qt::Key_5;
    case 0x18:

        return Qt::Key_9;
    case 0x1A:

        return Qt::Key_7;
    case 0x1C:

        return Qt::Key_8;
    case 0x1D:

        return Qt::Key_0;
    }
#endif
#ifdef Q_WS_WIN
    // https://msdn.microsoft.com/en-us/library/aa299374%28v=vs.60%29.aspx
    //  48   0x30   (VK_0)              | 0 key
    //  49   0x31   (VK_1)              | 1 key
    //  50   0x32   (VK_2)              | 2 key
    //  51   0x33   (VK_3)              | 3 key
    //  52   0x34   (VK_4)              | 4 key
    //  53   0x35   (VK_5)              | 5 key
    //  54   0x36   (VK_6)              | 6 key
    //  55   0x37   (VK_7)              | 7 key
    //  56   0x38   (VK_8)              | 8 key
    //  57   0x39   (VK_9)              | 9 key
    // Windows seems to always return the same virtual key for digits, whatever the modifi
    Q_UNUSED(nativeScanCode);
    switch (nativeVirtualKey) {
    case 0x30:

        return Qt::Key_0;
    case 0x31:

        return Qt::Key_1;
    case 0x32:

        return Qt::Key_2;
    case 0x33:

        return Qt::Key_3;
    case 0x34:

        return Qt::Key_4;
    case 0x35:

        return Qt::Key_5;
    case 0x36:

        return Qt::Key_6;
    case 0x37:

        return Qt::Key_7;
    case 0x38:

        return Qt::Key_8;
    case 0x39:

        return Qt::Key_9;
    }
#endif
#if defined(Q_WS_X11) && defined(Q_OS_LINUX)
    // probably only possible on Linux, since scancodes are OS-dependent
    // https://www.win.tue.nl/~aeb/linux/kbd/scancodes-1.html
    Q_UNUSED(nativeVirtualKey);
    switch (nativeScanCode) {
    case 10:

        return Qt::Key_1;
    case 11:

        return Qt::Key_2;
    case 12:

        return Qt::Key_3;
    case 13:

        return Qt::Key_4;
    case 14:

        return Qt::Key_5;
    case 15:

        return Qt::Key_6;
    case 16:

        return Qt::Key_7;
    case 17:

        return Qt::Key_8;
    case 18:

        return Qt::Key_9;
    case 19:

        return Qt::Key_0;
    }
#endif

    return key;
} // Gui::handleNativeKeys

void
Gui::keyPressEvent(QKeyEvent* e)
{
    //qDebug() << "Gui::keyPressed:" << e->text() << "modifiers:" << e->modifiers();
    if (_imp->currentPanelFocusEventRecursion > 0) {
        return;
    }

    QWidget* w = qApp->widgetAt( QCursor::pos() );
    Qt::Key key = (Qt::Key)Gui::handleNativeKeys( e->key(), e->nativeScanCode(), e->nativeVirtualKey() );
    Qt::KeyboardModifiers modifiers = e->modifiers();

    if (key == Qt::Key_Escape) {
        RightClickableWidget* panel = RightClickableWidget::isParentSettingsPanelRecursive(w);
        if (panel) {
            const DockablePanel* dock = panel->getPanel();
            if (dock) {
                const_cast<DockablePanel*>(dock)->closePanel();
            }
        }
    } else if ( (key == Qt::Key_V) && modCASIsControl(e) ) {
        // CTRL +V should have been caught by the Nodegraph if it contained a valid Natron graph.
        // We still try to check if it is a readable Python script
        QClipboard* clipboard = QApplication::clipboard();
        const QMimeData* mimedata = clipboard->mimeData();
        if ( mimedata->hasFormat( QLatin1String("text/plain") ) ) {
            QByteArray data = mimedata->data( QLatin1String("text/plain") );
            QString str = QString::fromUtf8(data);
            if ( QFile::exists(str) ) {
                QList<QUrl> urls;
                urls.push_back( QUrl::fromLocalFile(str) );
                handleOpenFilesFromUrls( urls, QCursor::pos() );
            } else {
                std::string error, output;
                if ( !NATRON_PYTHON_NAMESPACE::interpretPythonScript(str.toStdString(), &error, &output) ) {
                    _imp->_scriptEditor->appendToScriptEditor( QString::fromUtf8( error.c_str() ) );
                    ensureScriptEditorVisible();
                } else if ( !output.empty() ) {
                    _imp->_scriptEditor->appendToScriptEditor( QString::fromUtf8( output.c_str() ) );
                }
            }
        } else if ( mimedata->hasUrls() ) {
            QList<QUrl> urls = mimedata->urls();
            handleOpenFilesFromUrls( urls, QCursor::pos() );
        }
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerPrevious, modifiers, key) ) {
        if ( getNodeGraph()->getLastSelectedViewer() ) {
            getNodeGraph()->getLastSelectedViewer()->previousFrame();
        }
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerForward, modifiers, key) ) {
        if ( getNodeGraph()->getLastSelectedViewer() ) {
            getNodeGraph()->getLastSelectedViewer()->toggleStartForward();
        }
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerBackward, modifiers, key) ) {
        if ( getNodeGraph()->getLastSelectedViewer() ) {
            getNodeGraph()->getLastSelectedViewer()->toggleStartBackward();
        }
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerStop, modifiers, key) ) {
        if ( getNodeGraph()->getLastSelectedViewer() ) {
            getNodeGraph()->getLastSelectedViewer()->abortRendering();
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
        getApp()->goToPreviousKeyframe();
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerNextKF, modifiers, key) ) {
        getApp()->goToNextKeyframe();
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphDisableNodes, modifiers, key) ) {
        _imp->_nodeGraphArea->toggleSelectedNodesEnabled();
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphFindNode, modifiers, key) ) {
        _imp->_nodeGraphArea->popFindDialog();
    } else if ( isKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput1, modifiers, key) ) {
        connectAInput(0);
    } else if ( isKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput2, modifiers, key) ) {
        connectAInput(1);
    } else if ( isKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput3, modifiers, key) ) {
        connectAInput(2);
    } else if ( isKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput4, modifiers, key) ) {
        connectAInput(3);
    } else if ( isKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput5, modifiers, key) ) {
        connectAInput(4);
    } else if ( isKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput6, modifiers, key) ) {
        connectAInput(5);
    } else if ( isKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput7, modifiers, key) ) {
        connectAInput(6);
    } else if ( isKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput8, modifiers, key) ) {
        connectAInput(7);
    } else if ( isKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput9, modifiers, key) ) {
        connectAInput(8);
    } else if ( isKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput10, modifiers, key) ) {
        connectAInput(9);
    } else if ( isKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerBToInput1, modifiers, key) ) {
        connectBInput(0);
    } else if ( isKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerBToInput2, modifiers, key) ) {
        connectBInput(1);
    } else if ( isKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerBToInput3, modifiers, key) ) {
        connectBInput(2);
    } else if ( isKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerBToInput4, modifiers, key) ) {
        connectBInput(3);
    } else if ( isKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerBToInput5, modifiers, key) ) {
        connectBInput(4);
    } else if ( isKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerBToInput6, modifiers, key) ) {
        connectBInput(5);
    } else if ( isKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerBToInput7, modifiers, key) ) {
        connectBInput(6);
    } else if ( isKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerBToInput8, modifiers, key) ) {
        connectBInput(7);
    } else if ( isKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerBToInput9, modifiers, key) ) {
        connectBInput(8);
    } else if ( isKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerBToInput10, modifiers, key) ) {
        connectBInput(9);
    } else {
        /*
         * Modifiers are always uncaught by child implementations so that we can forward them to 1 ViewerTab so that
         * plug-ins overlay interacts always get the keyDown/keyUp events to track modifiers state.
         */
        bool isModifier = key == Qt::Key_Control || key == Qt::Key_Shift || key == Qt::Key_Alt || key == Qt::Key_Meta;
        if (isModifier) {
            const std::list<ViewerTab*>& viewers = getViewersList();
            bool viewerTabHasFocus = false;
            for (std::list<ViewerTab*>::const_iterator it = viewers.begin(); it != viewers.end(); ++it) {
                if ( (*it)->hasFocus() || (_imp->currentPanelFocus == *it) ) {
                    viewerTabHasFocus = true;
                    break;
                }
            }
            //Plug-ins did not yet receive a keyDown event for this modifier, send it
            if ((!viewers.empty() && (!viewerTabHasFocus || !_imp->keyPressEventHasVisitedFocusWidget))) {
                //Increment a recursion counter because the handler of the focus widget might toss it back to us
                ++_imp->currentPanelFocusEventRecursion;
                //If a panel as the click focus, try to send the event to it
                QKeyEvent* ev = new QKeyEvent(QEvent::KeyPress, key, modifiers);
                qApp->notify(viewers.front(), ev);
                --_imp->currentPanelFocusEventRecursion;
            }
        }

        if (_imp->currentPanelFocus && !_imp->keyPressEventHasVisitedFocusWidget) {
            //Increment a recursion counter because the handler of the focus widget might toss it back to us
            ++_imp->currentPanelFocusEventRecursion;
            //If a panel as the click focus, try to send the event to it
            QWidget* curFocusWidget = _imp->currentPanelFocus->getWidget();
            assert(curFocusWidget);
            QKeyEvent* ev = new QKeyEvent(QEvent::KeyPress, key, modifiers);
            qApp->notify(curFocusWidget, ev);
            --_imp->currentPanelFocusEventRecursion;
        } else {
            QMainWindow::keyPressEvent(e);
        }
    }
} // Gui::keyPressEvent

void
Gui::keyReleaseEvent(QKeyEvent* e)
{
    if (_imp->currentPanelFocusEventRecursion > 0) {
        return;
    }

    Qt::Key key = (Qt::Key)e->key();
    Qt::KeyboardModifiers modifiers = e->modifiers();

    /*
     * Modifiers are always uncaught by child implementations so that we can forward them to 1 ViewerTab so that
     * plug-ins overlay interacts always get the keyDown/keyUp events to track modifiers state.
     */
    bool isModifier = key == Qt::Key_Control || key == Qt::Key_Shift || key == Qt::Key_Alt || key == Qt::Key_Meta;
    if (isModifier) {
        const std::list<ViewerTab*>& viewers = getViewersList();
        bool viewerTabHasFocus = false;
        for (std::list<ViewerTab*>::const_iterator it = viewers.begin(); it != viewers.end(); ++it) {
            if ( (*it)->hasFocus() || ( (_imp->currentPanelFocus == *it) && !_imp->keyUpEventHasVisitedFocusWidget ) ) {
                viewerTabHasFocus = true;
                break;
            }
        }
        //Plug-ins did not yet receive a keyUp event for this modifier, send it
        if (!viewers.empty() && !viewerTabHasFocus) {
            //Increment a recursion counter because the handler of the focus widget might toss it back to us
            ++_imp->currentPanelFocusEventRecursion;
            //If a panel as the click focus, try to send the event to it
            QKeyEvent* ev = new QKeyEvent(QEvent::KeyRelease, key, modifiers);
            qApp->notify(viewers.front(), ev);
            --_imp->currentPanelFocusEventRecursion;
        }
    }

    if (_imp->currentPanelFocus && !_imp->keyUpEventHasVisitedFocusWidget) {
        //Increment a recursion counter because the handler of the focus widget might toss it back to us
        ++_imp->currentPanelFocusEventRecursion;
        //If a panel as the click focus, try to send the event to it
        QWidget* curFocusWidget = _imp->currentPanelFocus->getWidget();
        assert(curFocusWidget);
        QKeyEvent* ev = new QKeyEvent(QEvent::KeyRelease, key, modifiers);
        qApp->notify(curFocusWidget, ev);
        --_imp->currentPanelFocusEventRecursion;
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
Gui::refreshAllTimeEvaluationParams(bool onlyTimeEvaluationKnobs)
{
    int time = getApp()->getProject()->getCurrentTime();

    for (std::list<NodeGraph*>::iterator it = _imp->_groups.begin(); it != _imp->_groups.end(); ++it) {
        (*it)->refreshNodesKnobsAtTime(true, time);
    }
    getNodeGraph()->refreshNodesKnobsAtTime(onlyTimeEvaluationKnobs, time);
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
    _imp->_propertiesBin->setEnabled(!clicked);

    if (!clicked) {
        int time = getApp()->getProject()->getCurrentTime();
        for (std::list<NodeGraph*>::iterator it = _imp->_groups.begin(); it != _imp->_groups.end(); ++it) {
            (*it)->refreshNodesKnobsAtTime(false, time);
        }
        getNodeGraph()->refreshNodesKnobsAtTime(false, time);
    }
}

void
Gui::addShortcut(BoundAction* action)
{
    if (_imp->_settingsGui) {
        _imp->_settingsGui->addShortcut(action);
    }
}

void
Gui::getNodesEntitledForOverlays(NodesList & nodes) const
{
    std::list<DockablePanel*> panels;
    {
        QMutexLocker k(&_imp->openedPanelsMutex);
        panels = _imp->openedPanels;
    }

    for (std::list<DockablePanel*>::const_iterator it = panels.begin();
        it != panels.end(); ++it) {

        if (!dynamic_cast<NodeSettingsPanel*>(*it))
            continue;

        NodeSettingsPanel* panel = dynamic_cast<NodeSettingsPanel*>(*it);
        if (!panel) {
            continue;
        }
        NodeGuiPtr node = panel->getNode();
        NodePtr internalNode = node->getNode();
        if (node && internalNode) {
            if ( internalNode->shouldDrawOverlay() ) {
                nodes.push_back( node->getNode() );
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
Gui::renderAllViewers(bool canAbort)
{
    assert( QThread::currentThread() == qApp->thread() );
    for (std::list<ViewerTab*>::const_iterator it = _imp->_viewerTabs.begin(); it != _imp->_viewerTabs.end(); ++it) {
        if ( (*it)->isVisible() ) {
            (*it)->getInternalNode()->renderCurrentFrame(canAbort);
        }
    }
}

void
Gui::abortAllViewers()
{
    assert( QThread::currentThread() == qApp->thread() );
    for (std::list<ViewerTab*>::const_iterator it = _imp->_viewerTabs.begin(); it != _imp->_viewerTabs.end(); ++it) {
        if ( (*it)->isVisible() ) {
            (*it)->getInternalNode()->getNode()->abortAnyProcessing_non_blocking();
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
    QTimer::singleShot( 25, _imp->_nodeGraphArea, SLOT(centerOnAllNodes()) );

    for (std::list<NodeGraph*>::iterator it = _imp->_groups.begin(); it != _imp->_groups.end(); ++it) {
        QTimer::singleShot( 25, *it, SLOT(centerOnAllNodes()) );
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
        PanelWidget* pw = t->currentWidget();
        if (pw) {
            pw->takeClickFocus();
        }
    }
}

void
Gui::onNextTabTriggered()
{
    TabWidget* t = getLastEnteredTabWidget();

    if (t) {
        t->moveToNextTab();
        PanelWidget* pw = t->currentWidget();
        if (pw) {
            pw->takeClickFocus();
        }
    }
}

void
Gui::onCloseTabTriggered()
{
    TabWidget* t = getLastEnteredTabWidget();

    if (t) {
        t->closeCurrentWidget();
        PanelWidget* pw = t->currentWidget();
        if (pw) {
            pw->takeClickFocus();
        }
    }
}

void
Gui::appendToScriptEditor(const std::string & str)
{
    _imp->_scriptEditor->appendToScriptEditor( QString::fromUtf8( str.c_str() ) );
}

void
Gui::printAutoDeclaredVariable(const std::string & str)
{
    _imp->_scriptEditor->printAutoDeclaredVariable( QString::fromUtf8( str.c_str() ) );
}

void
Gui::exportGroupAsPythonScript(NodeCollection* collection)
{
    assert(collection);
    NodesList nodes = collection->getNodes();
    bool hasOutput = false;
    for (NodesList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        if ( (*it)->isActivated() && dynamic_cast<GroupOutput*>( (*it)->getEffectInstance().get() ) ) {
            hasOutput = true;
            break;
        }
    }

    if (!hasOutput) {
        Dialogs::errorDialog( tr("Export").toStdString(), tr("To export as group, at least one Output node must exist.").toStdString() );

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
        if ( !NATRON_PYTHON_NAMESPACE::interpretPythonScript(found->second, &err, &output) ) {
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
    QStringList grouping = menuGrouping.split( QLatin1Char('/') );

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

void
Gui::setTripleSyncEnabled(bool enabled)
{
    if (_imp->_isTripleSyncEnabled != enabled) {
        _imp->_isTripleSyncEnabled = enabled;
    }
}

bool
Gui::isTripleSyncEnabled() const
{
    return _imp->_isTripleSyncEnabled;
}

void
Gui::centerOpenedViewersOn(SequenceTime left,
                           SequenceTime right)
{
    const std::list<ViewerTab *> &viewers = getViewersList();

    for (std::list<ViewerTab *>::const_iterator it = viewers.begin(); it != viewers.end(); ++it) {
        ViewerTab *v = (*it);

        v->centerOn_tripleSync(left, right);
    }
}

#ifdef __NATRON_WIN32__
void
Gui::ddeOpenFile(const QString& filePath)
{
    getApp()->handleFileOpenEvent( filePath.toStdString() );
}

#endif


bool
Gui::isFocusStealingPossible()
{
    assert( qApp && qApp->thread() == QThread::currentThread() );
    QWidget* currentFocus = qApp->focusWidget();
    bool focusStealingNotPossible = ( dynamic_cast<QLineEdit*>(currentFocus) ||
                                      dynamic_cast<QTextEdit*>(currentFocus) );

    return !focusStealingNotPossible;
}

void
Gui::setCurrentPanelFocus(PanelWidget* widget)
{
    assert( QThread::currentThread() == qApp->thread() );
    _imp->currentPanelFocus = widget;

}

PanelWidget*
Gui::getCurrentPanelFocus() const
{
    assert( QThread::currentThread() == qApp->thread() );

    return _imp->currentPanelFocus;
}


static PanelWidget*
isPaneChild(QWidget* w,
            int recursionLevel)
{
    if (!w) {
        return 0;
    }
    PanelWidget* pw = dynamic_cast<PanelWidget*>(w);
    if ( pw && (recursionLevel > 0) ) {
        /*
           Do not return it if recursion is 0, otherwise the focus stealing of the mouse over will actually take click focus
         */
        return pw;
    }

    return isPaneChild(w->parentWidget(), recursionLevel + 1);
}

void
Gui::onFocusChanged(QWidget* /*old*/,
                    QWidget* newFocus)
{
    PanelWidget* pw = isPaneChild(newFocus, 0);

    if (pw) {
        pw->takeClickFocus();
    }
}

void
Gui::fileSequencesFromUrls(const QList<QUrl>& urls,
                           std::vector<SequenceParsing::SequenceFromFilesPtr>* sequences)
{

    QStringList filesList;

    for (int i = 0; i < urls.size(); ++i) {
        const QUrl rl = QtCompat::toLocalFileUrlFixed( urls.at(i) );
        QString path = rl.toLocalFile();

#ifdef __NATRON_WIN32__

        if ( !path.isEmpty() && ( ( path.at(0) == QLatin1Char('/') ) || ( path.at(0) == QLatin1Char('\\') ) ) ) {
            path = path.remove(0, 1);
        }
        if (appPTR->getCurrentSettings()->isDriveLetterToUNCPathConversionEnabled()) {
            path = FileSystemModel::mapPathWithDriveLetterToPathWithNetworkShareName(path);
        }
#endif
        QDir dir(path);
        
        //if the path dropped is not a directory append it
        if ( !dir.exists() ) {
            filesList << path;
        } else {
            //otherwise append everything inside the dir recursively
            SequenceFileDialog::appendFilesFromDirRecursively(&dir, &filesList);
        }
    }

    QStringList supportedExtensions;
    supportedExtensions.push_back( QString::fromLatin1(NATRON_PROJECT_FILE_EXT) );
    supportedExtensions.push_back( QString::fromLatin1("py") );

    std::vector<std::string> readersFormat;
    appPTR->getSupportedReaderFileFormats(&readersFormat);
    for (std::vector<std::string>::const_iterator it = readersFormat.begin(); it != readersFormat.end(); ++it) {
        supportedExtensions.push_back( QString::fromUtf8( it->c_str() ) );
    }
    *sequences = SequenceFileDialog::fileSequencesFromFilesList(filesList, supportedExtensions);

}

void
Gui::dragEnterEvent(QDragEnterEvent* e)
{

    if ( !e->mimeData()->hasUrls() ) {
        return;
    }

    e->accept();

}

void
Gui::dragMoveEvent(QDragMoveEvent* e)
{

    if ( !e->mimeData()->hasUrls() ) {
        return;
    }


    e->accept();

}

void
Gui::dragLeaveEvent(QDragLeaveEvent* e)
{
    e->accept();
}

static NodeGraph*
isNodeGraphChild(QWidget* w)
{
    NodeGraph* n = dynamic_cast<NodeGraph*>(w);
    if (!w) {
        return 0;
    }
    if (n) {
        return n;
    } else {
        QWidget* parent = w->parentWidget();
        if (parent) {
            return isNodeGraphChild(parent);
        } else {
            return 0;
        }
    }
}

void
Gui::handleOpenFilesFromUrls(const QList<QUrl>& urls,
                             const QPoint& globalPos)
{
    std::vector<SequenceParsing::SequenceFromFilesPtr> sequences;


    fileSequencesFromUrls(urls, &sequences);
    QWidget* widgetUnderMouse = QApplication::widgetAt(globalPos);
    NodeGraph* graph = isNodeGraphChild(widgetUnderMouse);

    if (!graph) {
        // No grpah under mouse, use top level one
        graph = _imp->_nodeGraphArea;
    }
    assert(graph);
    if (!graph) {
        return;
    }

    QPointF graphScenePos = graph->mapToScene( graph->mapFromGlobal(globalPos) );
    std::locale local;
    for (U32 i = 0; i < sequences.size(); ++i) {
        SequenceParsing::SequenceFromFilesPtr & sequence = sequences[i];
        if (sequence->count() < 1) {
            continue;
        }

        ///find a decoder for this file type
        //std::string ext = sequence->fileExtension();
        std::string extLower = sequence->fileExtension();
        boost::to_lower(extLower);
        if (extLower == NATRON_PROJECT_FILE_EXT) {
            const std::map<int, SequenceParsing::FileNameContent>& content = sequence->getFrameIndexes();
            assert( !content.empty() );
            AppInstancePtr appInstance = openProject( content.begin()->second.absoluteFileName() );
            Q_UNUSED(appInstance);
        } else if (extLower == "py") {
            const std::map<int, SequenceParsing::FileNameContent>& content = sequence->getFrameIndexes();
            assert( !content.empty() );
            _imp->_scriptEditor->sourceScript( QString::fromUtf8( content.begin()->second.absoluteFileName().c_str() ) );
            ensureScriptEditorVisible();
        } else {
            std::string readerPluginID = appPTR->getReaderPluginIDForFileType(extLower);
            if ( readerPluginID.empty() ) {
                Dialogs::errorDialog("Reader", "No plugin capable of decoding " + extLower + " was found.");
            } else {


                std::string pattern = sequence->generateValidSequencePattern();
                CreateNodeArgs args(readerPluginID, graph->getGroup() );
                args.setProperty<double>(kCreateNodeArgsPropNodeInitialPosition, graphScenePos.x(), 0);
                args.setProperty<double>(kCreateNodeArgsPropNodeInitialPosition, graphScenePos.y(), 1);
                args.addParamDefaultValue<std::string>(kOfxImageEffectFileParamName, pattern);


                NodePtr n = getApp()->createNode(args);

                //And offset scenePos by the Width of the previous node created if several nodes are created
                double w, h;
                n->getSize(&w, &h);
                graphScenePos.rx() += (w + 10);

            }
        }
    }
} // Gui::handleOpenFilesFromUrls

void
Gui::dropEvent(QDropEvent* e)
{
    if ( !e->mimeData()->hasUrls() ) {
        return;
    }

    e->accept();

    QList<QUrl> urls = e->mimeData()->urls();

    handleOpenFilesFromUrls( urls, mapToGlobal( e->pos() ) );
} // dropEvent

NATRON_NAMESPACE_EXIT
