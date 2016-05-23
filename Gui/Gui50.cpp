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
#include <locale>
#include <utility>
#include <stdexcept>

#if !defined(SBK_RUN) && !defined(Q_MOC_RUN)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
#include <boost/algorithm/string/case_conv.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
#endif

#include "Global/Macros.h"

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
#include <QTabBar>
#include <QTextEdit>
#include <QLineEdit>
#include <QCursor>
#include <QCheckBox>
#include <QComboBox>
#include <QTreeView>

#include "Global/QtCompat.h"

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
#include "Gui/GuiMacros.h"
#include "Gui/LogWindow.h"
#include "Gui/NodeGraph.h"
#include "Gui/NodeGui.h"
#include "Gui/NodeSettingsPanel.h"
#include "Gui/ProgressPanel.h"
#include "Gui/RightClickableWidget.h"
#include "Gui/RenderingProgressDialog.h"
#include "Gui/RotoGui.h"
#include "Gui/ScriptEditor.h"
#include "Gui/ShortCutEditor.h"
#include "Gui/SpinBox.h"
#include "Gui/TabWidget.h"
#include "Gui/ViewerGL.h"
#include "Gui/ViewerTab.h"
#include "Gui/SequenceFileDialog.h"
#include "Gui/PropertiesBinWrapper.h"
#include "Gui/Histogram.h"

NATRON_NAMESPACE_ENTER;


void
Gui::setUndoRedoStackLimit(int limit)
{
    _imp->_nodeGraphArea->setUndoRedoStackLimit(limit);
}

void
Gui::showErrorLog()
{
    QString log = appPTR->getErrorLog_mt_safe();
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

    std::list<ViewerTab*> viewers;
    {
        QMutexLocker l(&_imp->_viewerTabsMutex);
        viewers = _imp->_viewerTabs;
    }
    for (std::list<ViewerTab*>::iterator it = viewers.begin(); it != viewers.end(); ++it) {
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
Gui::progressStart(const NodePtr& node,
                   const std::string &message,
                   const std::string & /*messageid*/,
                   bool canCancel)
{
    _imp->_progressPanel->startTask( node, INT_MIN, INT_MAX, 1, false, canCancel, QString::fromUtf8( message.c_str() ) );
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
Gui::handleNativeKeys(int key, quint32 nativeScanCode, quint32 nativeVirtualKey)
{
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
#pragma message WARN("TODO: handle keys 0-9 on AZERTY keyboards")
    // https://msdn.microsoft.com/en-us/library/aa299374%28v=vs.60%29.aspx
    qDebug() << "scancode=" << nativeScanCode << "virtualkey=" << nativeVirtualKey;
#endif
#ifdef Q_WS_X11
#pragma message WARN("TODO: handle keys 0-9 on AZERTY keyboards")
    // probably only possible on Linux
    // https://www.win.tue.nl/~aeb/linux/kbd/scancodes-1.html
    qDebug() << "scancode=" << nativeScanCode << "virtualkey=" << nativeVirtualKey;
#endif
    return key;
}

void
Gui::keyPressEvent(QKeyEvent* e)
{
    if (_imp->currentPanelFocusEventRecursion > 0) {
        return;
    }

    QWidget* w = qApp->widgetAt( QCursor::pos() );
    Qt::Key key = (Qt::Key)Gui::handleNativeKeys( e->key(), e->nativeScanCode(), e->nativeVirtualKey() );
    Qt::KeyboardModifiers modifiers = e->modifiers();

    if (key == Qt::Key_Escape) {
        RightClickableWidget* panel = RightClickableWidget::isParentSettingsPanelRecursive(w);
        if (panel) {
            panel->getPanel()->closePanel();
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
                if ( !Python::interpretPythonScript(str.toStdString(), &error, &output) ) {
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
        connectInput(0);
    } else if ( isKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput2, modifiers, key) ) {
        connectInput(1);
    } else if ( isKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput3, modifiers, key) ) {
        connectInput(2);
    } else if ( isKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput4, modifiers, key) ) {
        connectInput(3);
    } else if ( isKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput5, modifiers, key) ) {
        connectInput(4);
    } else if ( isKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput6, modifiers, key) ) {
        connectInput(5);
    } else if ( isKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput7, modifiers, key) ) {
        connectInput(6);
    } else if ( isKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput8, modifiers, key) ) {
        connectInput(7);
    } else if ( isKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput9, modifiers, key) ) {
        connectInput(8);
    } else if ( isKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput10, modifiers, key) ) {
        connectInput(9);
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
                if ( (*it)->hasFocus() || ( (_imp->currentPanelFocus == *it) && !_imp->keyPressEventHasVisitedFocusWidget ) ) {
                    viewerTabHasFocus = true;
                    break;
                }
            }
            //Plug-ins did not yet receive a keyDown event for this modifier, send it
            if (!viewers.empty() && !viewerTabHasFocus) {
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
Gui::refreshAllTimeEvaluationParams()
{
    int time = getApp()->getProject()->getCurrentTime();

    for (std::list<NodeGraph*>::iterator it = _imp->_groups.begin(); it != _imp->_groups.end(); ++it) {
        (*it)->refreshNodesKnobsAtTime(true, time);
    }
    getNodeGraph()->refreshNodesKnobsAtTime(true, time);
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
Gui::getNodesEntitledForOverlays(NodesList & nodes) const
{
    std::list<DockablePanel*> panels;
    {
        QMutexLocker k(&_imp->openedPanelsMutex);
        panels = _imp->openedPanels;
    }

    for (std::list<DockablePanel*>::const_iterator it = panels.begin();
         it != panels.end(); ++it) {
        NodeSettingsPanel* panel = dynamic_cast<NodeSettingsPanel*>(*it);
        if (!panel) {
            continue;
        }
        NodeGuiPtr node = panel->getNode();
        NodePtr internalNode = node->getNode();
        if (node && internalNode) {
            boost::shared_ptr<MultiInstancePanel> multiInstance = node->getMultiInstancePanel();
            if (multiInstance) {
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
            (*it)->getInternalNode()->getNode()->abortAnyProcessing();
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
        Dialogs::errorDialog( tr("Export").toStdString(), tr("To export as group, at least one Ouptut node must exist.").toStdString() );

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
        if ( !Python::interpretPythonScript(found->second, &err, &output) ) {
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
    _imp->_appInstance->handleFileOpenEvent( filePath.toStdString() );
}

#endif


bool
Gui::isFocusStealingPossible()
{
    assert( qApp && qApp->thread() == QThread::currentThread() );
    QWidget* currentFocus = qApp->focusWidget();
    bool focusStealingNotPossible = ( dynamic_cast<QLineEdit*>(currentFocus) ||
                                      dynamic_cast<QTextEdit*>(currentFocus) ||
                                      dynamic_cast<QCheckBox*>(currentFocus) ||
                                      dynamic_cast<ComboBox*>(currentFocus) ||
                                      dynamic_cast<QComboBox*>(currentFocus) );

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
                           std::vector< boost::shared_ptr<SequenceParsing::SequenceFromFiles> >* sequences)
{
    QStringList filesList;

    for (int i = 0; i < urls.size(); ++i) {
        const QUrl rl = QtCompat::toLocalFileUrlFixed( urls.at(i) );
        QString path = rl.toLocalFile();

#ifdef __NATRON_WIN32__
        if ( !path.isEmpty() && ( ( path.at(0) == QLatin1Char('/') ) || ( path.at(0) == QLatin1Char('\\') ) ) ) {
            path = path.remove(0, 1);
        }
        path = FileSystemModel::mapPathWithDriveLetterToPathWithNetworkShareName(path);

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
    ///get all the decoders
    std::map<std::string, std::string> readersForFormat;
    appPTR->getCurrentSettings()->getFileFormatsForReadingAndReader(&readersForFormat);
    for (std::map<std::string, std::string>::const_iterator it = readersForFormat.begin(); it != readersForFormat.end(); ++it) {
        supportedExtensions.push_back( QString::fromUtf8( it->first.c_str() ) );
    }
    *sequences = SequenceFileDialog::fileSequencesFromFilesList(filesList, supportedExtensions);
}

void
Gui::dragEnterEvent(QDragEnterEvent* e)
{
    if ( !e->mimeData()->hasUrls() ) {
        return;
    }

    QList<QUrl> urls = e->mimeData()->urls();

    std::vector< boost::shared_ptr<SequenceParsing::SequenceFromFiles> > sequences;
    fileSequencesFromUrls(urls, &sequences);

    if ( !sequences.empty() ) {
        e->accept();
    }
}

void
Gui::dragMoveEvent(QDragMoveEvent* e)
{
    if ( !e->mimeData()->hasUrls() ) {
        return;
    }

    QList<QUrl> urls = e->mimeData()->urls();

    std::vector< boost::shared_ptr<SequenceParsing::SequenceFromFiles> > sequences;
    fileSequencesFromUrls(urls, &sequences);

    if ( !sequences.empty() ) {
        e->accept();
    }
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
    std::vector< boost::shared_ptr<SequenceParsing::SequenceFromFiles> > sequences;

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
        boost::shared_ptr<SequenceParsing::SequenceFromFiles> & sequence = sequences[i];
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
            AppInstance* appInstance = openProject( content.begin()->second.absoluteFileName() );
            Q_UNUSED(appInstance);
        } else if (extLower == "py") {
            const std::map<int, SequenceParsing::FileNameContent>& content = sequence->getFrameIndexes();
            assert( !content.empty() );
            _imp->_scriptEditor->sourceScript( QString::fromUtf8( content.begin()->second.absoluteFileName().c_str() ) );
            ensureScriptEditorVisible();
        } else {
            std::map<std::string, std::string> readersForFormat;
            appPTR->getCurrentSettings()->getFileFormatsForReadingAndReader(&readersForFormat);
            std::map<std::string, std::string>::iterator found = readersForFormat.find(extLower);
            if ( found == readersForFormat.end() ) {
                Dialogs::errorDialog("Reader", "No plugin capable of decoding " + extLower + " was found.");
            } else {
                std::string pattern = sequence->generateValidSequencePattern();
                CreateNodeArgs args( QString::fromUtf8( found->second.c_str() ), eCreateNodeReasonUserCreate, graph->getGroup() );
                args.xPosHint = graphScenePos.x();
                args.yPosHint = graphScenePos.y();
                args.paramValues.push_back( createDefaultValueForParam<std::string>(kOfxImageEffectFileParamName, pattern) );

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

NATRON_NAMESPACE_EXIT;
