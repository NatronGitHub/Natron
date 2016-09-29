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


NATRON_NAMESPACE_ENTER;



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
    if ( !getGui() ) {
        return;
    }
    const std::list<ViewerTab*> & activeNodes = getGui()->getViewersList();
    for (std::list<ViewerTab*>::const_iterator it = activeNodes.begin(); it != activeNodes.end(); ++it) {
        ViewerNodePtr viewer = (*it)->getInternalNode();
        if (viewer) {
            ViewerInstancePtr instance = viewer->getInternalViewerNode();
            if (instance) {
                RenderEnginePtr engine = instance->getRenderEngine();
                if ( engine ) {
                    engine->abortRenderingAutoRestart();
                    engine->renderCurrentFrame(false, true);
                }
            }
        }
    }
}

void
ViewerTab::seek(SequenceTime time)
{
    _imp->timeLineGui->seek(time);
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
    if ( !getGui() ) {
        return;
    }

    ViewerNodePtr node = _imp->viewerNode.lock();
    ViewerInstancePtr viewerNode = node->getInternalViewerNode();
    if ((TimelineChangeReasonEnum)reason != eTimelineChangeReasonPlaybackSeek) {
        node->getCurrentFrameKnob()->setValueFromPlugin(time, ViewSpec::current(), 0);
    }

    if ( _imp->timeLineGui->getTimeline() != getGui()->getApp()->getTimeLine() ) {
        viewerNode->renderCurrentFrame(true);
    }
}


ViewerTab::~ViewerTab()
{
    if ( getGui() ) {
        NodeGraph* graph = 0;
        ViewerNodePtr internalNode = getInternalNode();

        ViewerInstancePtr viewerNode = internalNode ? internalNode->getInternalViewerNode() : ViewerInstancePtr();
        if (viewerNode) {
            NodeCollectionPtr collection = viewerNode->getNode()->getGroup();
            if (collection) {
                NodeGroupPtr isGrp = toNodeGroup(collection);
                if (isGrp) {
                    NodeGraphI* graph_i = isGrp->getNodeGraph();
                    if (graph_i) {
                        graph = dynamic_cast<NodeGraph*>(graph_i);
                        assert(graph);
                    }
                } else {
                    graph = getGui()->getNodeGraph();
                }
            }
            internalNode->invalidateUiContext();
        } else {
            graph = getGui()->getNodeGraph();
        }
        assert(graph);
        if ( getGui()->getApp() && !getGui()->getApp()->isClosing() && graph && (graph->getLastSelectedViewer() == this) ) {
            graph->setLastSelectedViewer(0);
        }
    }
    _imp->nodesContext.clear();
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
    if ( getGui() ) {
        getGui()->setActiveViewer(this);
    }

    bool accept = true;
    Qt::KeyboardModifiers modifiers = e->modifiers();
    Qt::Key key = (Qt::Key)Gui::handleNativeKeys( e->key(), e->nativeScanCode(), e->nativeVirtualKey() );
    double scale = 1. / ( 1 << _imp->viewer->getCurrentRenderScale() );

    if ( e->isAutoRepeat() && notifyOverlaysKeyRepeat(RenderScale(scale), e) ) {
        update();
    } else if ( notifyOverlaysKeyDown(RenderScale(scale), e) ) {
        update();
    } else if ( isKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput1, modifiers, key) ) {
        connectToAInput(0);
    } else if ( isKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput2, modifiers, key) ) {
        connectToAInput(1);
    } else if ( isKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput3, modifiers, key) ) {
        connectToAInput(2);
    } else if ( isKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput4, modifiers, key) ) {
        connectToAInput(3);
    } else if ( isKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput5, modifiers, key) ) {
        connectToAInput(4);
    } else if ( isKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput6, modifiers, key) ) {
        connectToAInput(5);
    } else if ( isKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput7, modifiers, key) ) {
        connectToAInput(6);
    } else if ( isKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput8, modifiers, key) ) {
        connectToAInput(7);
    } else if ( isKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput9, modifiers, key) ) {
        connectToAInput(8);
    } else if ( isKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput10, modifiers, key) ) {
        connectToAInput(9);
    } else if ( isKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerBToInput1, modifiers, key) ) {
        connectToBInput(0);
    } else if ( isKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerBToInput2, modifiers, key) ) {
        connectToBInput(1);
    } else if ( isKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerBToInput3, modifiers, key) ) {
        connectToBInput(2);
    } else if ( isKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerBToInput4, modifiers, key) ) {
        connectToBInput(3);
    } else if ( isKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerBToInput5, modifiers, key) ) {
        connectToBInput(4);
    } else if ( isKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerBToInput6, modifiers, key) ) {
        connectToBInput(5);
    } else if ( isKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerBToInput7, modifiers, key) ) {
        connectToBInput(6);
    } else if ( isKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerBToInput8, modifiers, key) ) {
        connectToBInput(7);
    } else if ( isKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerBToInput9, modifiers, key) ) {
        connectToBInput(8);
    } else if ( isKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerBToInput10, modifiers, key) ) {
        connectToBInput(9);
    } else if (key == Qt::Key_Escape) {
        _imp->viewer->s_selectionCleared();
        update();
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
} // keyPressEvent

void
ViewerTab::keyReleaseEvent(QKeyEvent* e)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    if ( !getGui() ) {
        return QWidget::keyPressEvent(e);
    }
    double scale = 1. / ( 1 << _imp->viewer->getCurrentRenderScale() );
    if ( notifyOverlaysKeyUp(RenderScale(scale), e) ) {
        _imp->viewer->redraw();
    } else {
        handleUnCaughtKeyUpEvent(e);
        QWidget::keyReleaseEvent(e);
    }
}

bool
ViewerTab::eventFilter(QObject *target,
                       QEvent* e)
{
    if (e->type() == QEvent::MouseButtonPress) {
        if ( getGui() && getGui()->getApp() ) {
            ViewerNodePtr viewerNode = _imp->viewerNode.lock();
            if (viewerNode) {
                NodeGuiIPtr gui_i = viewerNode->getNode()->getNodeGui();
                assert(gui_i);
                NodeGuiPtr gui = boost::dynamic_pointer_cast<NodeGui>(gui_i);
                getGui()->selectNode(gui);
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


NATRON_NAMESPACE_EXIT;
