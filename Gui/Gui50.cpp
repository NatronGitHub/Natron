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
#include <utility>
#include <stdexcept>

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
#include <QVBoxLayout>
#include <QTreeWidget>
#include <QTabBar>
#include <QTextEdit>
#include <QLineEdit>
#include <QCheckBox>
#include <QComboBox>
#include <QTreeView>

#include "Engine/GroupOutput.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h" // NodesList, NodeCollection
#include "Engine/Project.h"
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
#include "Gui/LogWindow.h"
#include "Gui/NodeGraph.h"
#include "Gui/NodeGui.h"
#include "Gui/NodeSettingsPanel.h"
#include "Gui/RightClickableWidget.h"
#include "Gui/RenderingProgressDialog.h"
#include "Gui/RotoGui.h"
#include "Gui/ScriptEditor.h"
#include "Gui/ShortCutEditor.h"
#include "Gui/SpinBox.h"
#include "Gui/TabWidget.h"
#include "Gui/ViewerGL.h"
#include "Gui/ViewerTab.h"
#include "Gui/PropertiesBinWrapper.h"
#include "Gui/Histogram.h"

NATRON_NAMESPACE_ENTER;


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
Gui::onDoProgressStartOnMainThread(KnobHolder* effect, const QString &message, const QString &/*messageid*/, bool canCancel)
{
    assert(QThread::currentThread() == qApp->thread());
    
    QString progressLabel;
    EffectInstance* isEffect = dynamic_cast<EffectInstance*>(effect);
    if (isEffect) {
        progressLabel.append(isEffect->getNode()->getLabel().c_str());
        progressLabel.append(": ");
    }
    progressLabel.append(message);
    GeneralProgressDialog* dialog = new GeneralProgressDialog(progressLabel, canCancel, this);
    
    {
        QMutexLocker k(&_imp->_progressBarsMutex);
        std::map<KnobHolder*, GeneralProgressDialog*>::iterator found = _imp->_progressBars.find(effect);
        ///If a second dialog was asked for whilst another is still active, the first dialog will not be
        ///able to be canceled.
        if (found != _imp->_progressBars.end()) {
            _imp->_progressBars.erase(found);
        }
        
        _imp->_progressBars.insert( std::make_pair(effect, dialog) );
    }
    
    //Do not show the dialog, it will be shown automatically on progress update
    //dialog->show();
    
}

void
Gui::onDoProgressEndOnMainThread(KnobHolder* effect)
{
    assert(QThread::currentThread() == qApp->thread());

    
    GeneralProgressDialog* dialog = 0;
    {
        QMutexLocker k(&_imp->_progressBarsMutex);
        std::map<KnobHolder*, GeneralProgressDialog*>::iterator found = _imp->_progressBars.find(effect);
        if (found == _imp->_progressBars.end()) {
            return;
        }
        dialog = found->second;
        _imp->_progressBars.erase(found);
    }
    
    if (dialog) {
        dialog->close();
    }
    
}

void
Gui::onDoProgressUpdateOnMainThread(KnobHolder* effect,double t)
{
    assert(QThread::currentThread() == qApp->thread());
    GeneralProgressDialog* dialog = 0;
    {
        QMutexLocker k(&_imp->_progressBarsMutex);
        std::map<KnobHolder*, GeneralProgressDialog*>::iterator found = _imp->_progressBars.find(effect);
        if (found == _imp->_progressBars.end()) {
            return;
        }
        dialog = found->second;
        _imp->_progressBars.erase(found);
    }
    if (dialog) {
        dialog->updateProgress(t);
    }
}

void
Gui::progressStart(KnobHolder* effect,
                   const std::string &message,
                   const std::string &messageid,
                   bool canCancel)
{
    if (!effect) {
        return;
    }
    if (QThread::currentThread() != qApp->thread()) {
        Q_EMIT s_doProgressStartOnMainThread(effect, message.c_str(), messageid.c_str(), canCancel);
    } else {
        onDoProgressStartOnMainThread(effect, message.c_str(), messageid.c_str(), canCancel);
    }

}

void
Gui::progressEnd(KnobHolder* effect)
{
    if (!effect) {
        return;
    }
    if (QThread::currentThread() != qApp->thread()) {
        Q_EMIT s_doProgressEndOnMainThread(effect);
    } else {
        onDoProgressEndOnMainThread(effect);
    }
}

bool
Gui::progressUpdate(KnobHolder* effect,
                    double t)
{
    if (!effect) {
        return true;
    }
    bool isMainThread = QThread::currentThread() == qApp->thread();
    if (!isMainThread) {
        Q_EMIT s_doProgressUpdateOnMainThread(effect, t);
        return true;
    } else {
        onDoProgressUpdateOnMainThread(effect, t);
    }

    QMutexLocker k(&_imp->_progressBarsMutex);
    std::map<KnobHolder*, GeneralProgressDialog*>::iterator found = _imp->_progressBars.find(effect);
    if (found != _imp->_progressBars.end()) {
        if (found->second->wasCanceled()) {
            return false;
        }
    }
    
    if (isMainThread) {
        QCoreApplication::processEvents();
    }

    return true;
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

static RightClickableWidget* isParentSettingsPanelRecursive(QWidget* w)
{
    if (!w) {
        return 0;
    }
    RightClickableWidget* panel = qobject_cast<RightClickableWidget*>(w);
    if (panel) {
        return panel;
    } else {
        return isParentSettingsPanelRecursive(w->parentWidget());
    }
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

void
Gui::keyPressEvent(QKeyEvent* e)
{
    if (_imp->currentPanelFocusEventRecursion > 0) {
        return;
    }
    
    QWidget* w = qApp->widgetAt( QCursor::pos() );

    
    Qt::Key key = (Qt::Key)e->key();
    Qt::KeyboardModifiers modifiers = e->modifiers();

    if (key == Qt::Key_Escape) {
        
        RightClickableWidget* panel = isParentSettingsPanelRecursive(w);
        if (panel) {
            panel->getPanel()->closePanel();
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
    } else if (isKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput1, modifiers, key) ) {
        connectInput(0);
    } else if (isKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput2, modifiers, key) ) {
        connectInput(1);
    } else if (isKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput3, modifiers, key) ) {
        connectInput(2);
    } else if (isKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput4, modifiers, key) ) {
        connectInput(3);
    } else if (isKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput5, modifiers, key) ) {
        connectInput(4);
    } else if (isKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput6, modifiers, key) ) {
        connectInput(5);
    } else if (isKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput7, modifiers, key) ) {
        connectInput(6);
    } else if (isKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput8, modifiers, key) ) {
        connectInput(7);
    } else if (isKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput9, modifiers, key) ) {
        connectInput(8);
    } else if (isKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput10, modifiers, key) ) {
        connectInput(9);
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
                if ((*it)->hasFocus() || (_imp->currentPanelFocus == *it && !_imp->keyPressEventHasVisitedFocusWidget)) {
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
                qApp->notify(viewers.front(),ev);
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
            qApp->notify(curFocusWidget,ev);
            --_imp->currentPanelFocusEventRecursion;
        } else {
            QMainWindow::keyPressEvent(e);
        }
    }
}

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
            if ((*it)->hasFocus() || (_imp->currentPanelFocus == *it && !_imp->keyUpEventHasVisitedFocusWidget)) {
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
            qApp->notify(viewers.front(),ev);
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
        qApp->notify(curFocusWidget,ev);
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
    for (std::list<NodeGraph*>::iterator it = _imp->_groups.begin(); it!= _imp->_groups.end(); ++it) {
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
        for (std::list<NodeGraph*>::iterator it = _imp->_groups.begin(); it!= _imp->_groups.end(); ++it) {
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
    assert(QThread::currentThread() == qApp->thread());
    for (std::list<ViewerTab*>::const_iterator it = _imp->_viewerTabs.begin(); it != _imp->_viewerTabs.end(); ++it) {
        if ( (*it)->isVisible() ) {
            (*it)->getInternalNode()->renderCurrentFrame(canAbort);
        }
    }
}

void
Gui::abortAllViewers()
{
    assert(QThread::currentThread() == qApp->thread());
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

#ifdef __NATRON_WIN32__
void
Gui::ddeOpenFile(const QString& filePath)
{
    _imp->_appInstance->handleFileOpenEvent(filePath.toStdString());
}
#endif


bool
Gui::isFocusStealingPossible()
{
    assert( qApp && qApp->thread() == QThread::currentThread() );
    QWidget* currentFocus = qApp->focusWidget();
    
    bool focusStealingNotPossible =
    dynamic_cast<QLineEdit*>(currentFocus) ||
    dynamic_cast<QTextEdit*>(currentFocus) ||
    dynamic_cast<QCheckBox*>(currentFocus) ||
    dynamic_cast<ComboBox*>(currentFocus) ||
    dynamic_cast<QComboBox*>(currentFocus);

    return !focusStealingNotPossible;

}

void
Gui::setCurrentPanelFocus(PanelWidget* widget)
{
    assert(QThread::currentThread() == qApp->thread());
    _imp->currentPanelFocus = widget;
}

PanelWidget*
Gui::getCurrentPanelFocus() const
{
    assert(QThread::currentThread() == qApp->thread());
    return _imp->currentPanelFocus;
}

static PanelWidget* isPaneChild(QWidget* w, int recursionLevel)
{
    if (!w) {
        return 0;
    }
    PanelWidget* pw = dynamic_cast<PanelWidget*>(w);
    if (pw && recursionLevel > 0) {
        /*
         Do not return it if recursion is 0, otherwise the focus stealing of the mouse over will actually take click focus
         */
        return pw;
    }
    return isPaneChild(w->parentWidget(), recursionLevel + 1);
}

void
Gui::onFocusChanged(QWidget* /*old*/, QWidget* newFocus)
{
    PanelWidget* pw = isPaneChild(newFocus,0);
    if (pw) {
        pw->takeClickFocus();
    }
}

NATRON_NAMESPACE_EXIT;
