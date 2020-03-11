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

#include "ViewerTab.h"
#include "ViewerTabPrivate.h"

#include <cassert>
#include <stdexcept>
#include <QtCore/QDebug>

#include <QApplication>
GCC_DIAG_UNUSED_PRIVATE_FIELD_OFF
// /opt/local/include/QtGui/qmime.h:119:10: warning: private field 'type' is not used [-Wunused-private-field]
#include <QtGui/QKeyEvent>
#include <QtCore/QTimer>
GCC_DIAG_UNUSED_PRIVATE_FIELD_ON

#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/OutputSchedulerThread.h" // RenderEngine
#include "Engine/Settings.h"
#include "Engine/KnobTypes.h"
#include "Engine/TimeLine.h"
#include "Engine/ViewIdx.h"
#include "Engine/RenderEngine.h"
#include "Engine/ViewerInstance.h"
#include "Engine/ViewerNode.h"

#include "Gui/ActionShortcuts.h"
#include "Gui/GuiMacros.h"
#include "Gui/Button.h"
#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h" // appPTR
#include "Gui/NodeGraph.h"
#include "Gui/NodeGui.h"
#include "Gui/NodeViewerContext.h"
#include "Gui/RenderStatsDialog.h"
#include "Gui/SpinBox.h"
#include "Gui/TimeLineGui.h"
#include "Gui/TabWidget.h"
#include "Gui/ViewerGL.h"


NATRON_NAMESPACE_ENTER


void
ViewerTab::updateZoomComboBox(int value)
{
    assert(value > 0);
    QString str = QString::number(value);
    str.append( QLatin1Char('%') );
    ViewerNodePtr internalNode = getInternalNode();
    if (internalNode) {
        internalNode->setZoomComboBoxText(str.toStdString());
    }
}




void
ViewerTab::abortViewersAndRefresh()
{
    Gui* gui = getGui();
    if (!gui) {
        return;
    }
    const std::list<ViewerTab*> & activeNodes = gui->getViewersList();
    for (std::list<ViewerTab*>::const_iterator it = activeNodes.begin(); it != activeNodes.end(); ++it) {
        ViewerNodePtr viewer = (*it)->getInternalNode();

        viewer->getNode()->getRenderEngine()->abortRenderingAutoRestart();
        viewer->getNode()->getRenderEngine()->renderCurrentFrame();

    }
}

void
ViewerTab::seek(SequenceTime time)
{
    _imp->timeLineGui->seek(time);
    Gui* gui = getGui();
    if (gui) {
        gui->refreshAllPreviews();
    }
}

void
ViewerTab::previousFrame()
{
    int prevFrame = _imp->timeLineGui->currentFrame() - 1;

    if ( prevFrame  < _imp->timeLineGui->leftBound() ) {
        prevFrame = _imp->timeLineGui->rightBound();
    }
    seek(prevFrame);
}

void
ViewerTab::nextFrame()
{
    int nextFrame = _imp->timeLineGui->currentFrame() + 1;

    if ( nextFrame  > _imp->timeLineGui->rightBound() ) {
        nextFrame = _imp->timeLineGui->leftBound();
    }
    seek(nextFrame);
}

void
ViewerTab::getTimelineBounds(int* first, int* last) const
{
    return _imp->timeLineGui->getBounds(first, last);
}


void
ViewerTab::onTimeLineTimeChanged(SequenceTime time,
                                 int reason)
{
    Gui* gui = getGui();
    if (!gui) {
        return;
    }
    ViewerNodePtr node = _imp->viewerNode.lock();
    if ((TimelineChangeReasonEnum)reason != eTimelineChangeReasonPlaybackSeek) {
        node->getCurrentFrameKnob()->setValue(time, ViewSetSpec::all(), DimIdx(0), eValueChangedReasonPluginEdited);
    }

    GuiAppInstancePtr app = gui->getApp();
    if ( app &&  _imp->timeLineGui->getTimeline() != app->getTimeLine() ) {
        node->getNode()->getRenderEngine()->renderCurrentFrame();
    }
}


void
ViewerTab::enterEvent(QEvent* e)
{
    enterEventBase();
    QWidget::enterEvent(e);
}

void
ViewerTab::leaveEvent(QEvent* e)
{
    leaveEventBase();
    QWidget::leaveEvent(e);
}

void
ViewerTab::keyPressEvent(QKeyEvent* e)
{
    ViewerNodePtr internalNode = getInternalNode();
    if (!internalNode || !internalNode->getNode()) {
        return;
    }
    //qDebug() << "ViewerTab::keyPressed:" << e->text() << "modifiers:" << e->modifiers();
    Gui* gui = getGui();
    if (gui) {
        gui->setActiveViewer(this);
    }

    bool accept = true;
    Qt::KeyboardModifiers modifiers = e->modifiers();
    Qt::Key key = (Qt::Key)Gui::handleNativeKeys( e->key(), e->nativeScanCode(), e->nativeVirtualKey() );
    double scale = 1. / ( 1 << _imp->viewer->getCurrentRenderScale() );

    if ( e->isAutoRepeat() && notifyOverlaysKeyRepeat(RenderScale(scale), e) ) {
        update();
    } else if ( notifyOverlaysKeyDown(RenderScale(scale), e) ) {
        update();
    } else if ( isKeybind(kShortcutGroupGlobal, kShortcutActionConnectViewerToInput1, modifiers, key) ) {
        connectToAInput(0);
    } else if ( isKeybind(kShortcutGroupGlobal, kShortcutActionConnectViewerToInput2, modifiers, key) ) {
        connectToAInput(1);
    } else if ( isKeybind(kShortcutGroupGlobal, kShortcutActionConnectViewerToInput3, modifiers, key) ) {
        connectToAInput(2);
    } else if ( isKeybind(kShortcutGroupGlobal, kShortcutActionConnectViewerToInput4, modifiers, key) ) {
        connectToAInput(3);
    } else if ( isKeybind(kShortcutGroupGlobal, kShortcutActionConnectViewerToInput5, modifiers, key) ) {
        connectToAInput(4);
    } else if ( isKeybind(kShortcutGroupGlobal, kShortcutActionConnectViewerToInput6, modifiers, key) ) {
        connectToAInput(5);
    } else if ( isKeybind(kShortcutGroupGlobal, kShortcutActionConnectViewerToInput7, modifiers, key) ) {
        connectToAInput(6);
    } else if ( isKeybind(kShortcutGroupGlobal, kShortcutActionConnectViewerToInput8, modifiers, key) ) {
        connectToAInput(7);
    } else if ( isKeybind(kShortcutGroupGlobal, kShortcutActionConnectViewerToInput9, modifiers, key) ) {
        connectToAInput(8);
    } else if ( isKeybind(kShortcutGroupGlobal, kShortcutActionConnectViewerToInput10, modifiers, key) ) {
        connectToAInput(9);
    } else if ( isKeybind(kShortcutGroupGlobal, kShortcutActionConnectViewerBToInput1, modifiers, key) ) {
        connectToBInput(0);
    } else if ( isKeybind(kShortcutGroupGlobal, kShortcutActionConnectViewerBToInput2, modifiers, key) ) {
        connectToBInput(1);
    } else if ( isKeybind(kShortcutGroupGlobal, kShortcutActionConnectViewerBToInput3, modifiers, key) ) {
        connectToBInput(2);
    } else if ( isKeybind(kShortcutGroupGlobal, kShortcutActionConnectViewerBToInput4, modifiers, key) ) {
        connectToBInput(3);
    } else if ( isKeybind(kShortcutGroupGlobal, kShortcutActionConnectViewerBToInput5, modifiers, key) ) {
        connectToBInput(4);
    } else if ( isKeybind(kShortcutGroupGlobal, kShortcutActionConnectViewerBToInput6, modifiers, key) ) {
        connectToBInput(5);
    } else if ( isKeybind(kShortcutGroupGlobal, kShortcutActionConnectViewerBToInput7, modifiers, key) ) {
        connectToBInput(6);
    } else if ( isKeybind(kShortcutGroupGlobal, kShortcutActionConnectViewerBToInput8, modifiers, key) ) {
        connectToBInput(7);
    } else if ( isKeybind(kShortcutGroupGlobal, kShortcutActionConnectViewerBToInput9, modifiers, key) ) {
        connectToBInput(8);
    } else if ( isKeybind(kShortcutGroupGlobal, kShortcutActionConnectViewerBToInput10, modifiers, key) ) {
        connectToBInput(9);
    } else if (key == Qt::Key_Escape) {
        onViewerSelectionCleared();
        update();
    } else {
        accept = false;
    }

    // If pressing modifiers for the picker, update it to reflect the input image if needed
    if (key == Qt::Key_Control || key == Qt::Key_Alt) {
        _imp->viewer->updateColorPicker(0);
        _imp->viewer->updateColorPicker(1);
    }
    if (accept) {
        takeClickFocus();
        e->accept();
    } else {
        handleUnCaughtKeyPressEvent(e);
        QWidget::keyPressEvent(e);
    }
} // keyPressEvent

void
ViewerTab::keyReleaseEvent(QKeyEvent* e)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    Gui* gui = getGui();
    if (!gui) {
        return QWidget::keyPressEvent(e);
    }
    Qt::Key key = (Qt::Key)e->key();
    double scale = 1. / ( 1 << _imp->viewer->getCurrentRenderScale() );
    if ( notifyOverlaysKeyUp(RenderScale(scale), e) ) {
        _imp->viewer->redraw();
    } else {
        // If pressing modifiers for the picker, update it to reflect the input image if needed
        if (key == Qt::Key_Control || key == Qt::Key_Alt) {
            _imp->viewer->updateColorPicker(0);
            _imp->viewer->updateColorPicker(1);
        }
        
        handleUnCaughtKeyUpEvent(e);
        QWidget::keyReleaseEvent(e);
    }
}

bool
ViewerTab::eventFilter(QObject *target,
                       QEvent* e)
{
    if (e->type() == QEvent::MouseButtonPress) {
        Gui* gui = getGui();
        if (gui) {
            GuiAppInstancePtr app = gui->getApp();
            if (app) {
                ViewerNodePtr viewerNode = _imp->viewerNode.lock();
                if (viewerNode) {
                    NodeGuiIPtr nodegui_i = viewerNode->getNode()->getNodeGui();
                    assert(nodegui_i);
                    NodeGuiPtr nodegui = boost::dynamic_pointer_cast<NodeGui>(nodegui_i);
                    gui->selectNode(nodegui);
                }
            }
        }
    }

    return QWidget::eventFilter(target, e);
}

void
ViewerTab::disconnectViewer()
{
    _imp->viewer->disconnectViewer();
}

QSize
ViewerTab::minimumSizeHint() const
{
    /*if ( !_imp->playerButtonsContainer->isVisible() ) {
        return QSize(500, 200);
    }*/

    return QWidget::minimumSizeHint();
}

QSize
ViewerTab::sizeHint() const
{
    /*if ( !_imp->playerButtonsContainer->isVisible() ) {
        return QSize(500, 200);
    }*/

    return QWidget::sizeHint();
}


NATRON_NAMESPACE_EXIT
