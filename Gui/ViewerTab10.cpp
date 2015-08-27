/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier
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

#include <QtCore/QDebug>

#include <QApplication>
GCC_DIAG_UNUSED_PRIVATE_FIELD_OFF
// /opt/local/include/QtGui/qmime.h:119:10: warning: private field 'type' is not used [-Wunused-private-field]
#include <QtGui/QKeyEvent>
GCC_DIAG_UNUSED_PRIVATE_FIELD_ON

#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/OutputSchedulerThread.h" // RenderEngine
#include "Engine/Settings.h"
#include "Engine/TimeLine.h"
#include "Engine/ViewerInstance.h"

#include "Gui/ActionShortcuts.h"
#include "Gui/GuiMacros.h"
#include "Gui/Button.h"
#include "Gui/ChannelsComboBox.h"
#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h" // appPTR
#include "Gui/NodeGraph.h"
#include "Gui/NodeGui.h"
#include "Gui/RenderStatsDialog.h"
#include "Gui/RotoGui.h"
#include "Gui/SpinBox.h"
#include "Gui/TimeLineGui.h"
#include "Gui/TrackerGui.h"
#include "Gui/ViewerGL.h"


using namespace Natron;


void
ViewerTab::onColorSpaceComboBoxChanged(int v)
{
    Natron::ViewerColorSpaceEnum colorspace;

    if (v == 0) {
        colorspace = Natron::eViewerColorSpaceLinear;
    } else if (v == 1) {
        colorspace = Natron::eViewerColorSpaceSRGB;
    } else if (v == 2) {
        colorspace = Natron::eViewerColorSpaceRec709;
    } else {
        assert(false);
    }
    _imp->viewer->setLut( (int)colorspace );
    _imp->viewerNode->onColorSpaceChanged(colorspace);
}

void
ViewerTab::onEnableViewerRoIButtonToggle(bool b)
{
    _imp->enableViewerRoI->setDown(b);
    _imp->viewer->setUserRoIEnabled(b);
}

void
ViewerTab::updateViewsMenu(int count)
{
    int currentIndex = _imp->viewsComboBox->activeIndex();

    _imp->viewsComboBox->clear();
    if (count == 1) {
        _imp->viewsComboBox->hide();
        _imp->viewsComboBox->addItem( tr("Main") );
    } else if (count == 2) {
        _imp->viewsComboBox->show();
        _imp->viewsComboBox->addItem( tr("Left"),QIcon(),QKeySequence(Qt::CTRL + Qt::Key_1) );
        _imp->viewsComboBox->addItem( tr("Right"),QIcon(),QKeySequence(Qt::CTRL + Qt::Key_2) );
    } else {
        _imp->viewsComboBox->show();
        for (int i = 0; i < count; ++i) {
            _imp->viewsComboBox->addItem( QString( tr("View ") ) + QString::number(i + 1),QIcon(),Gui::keySequenceForView(i) );
        }
    }
    if ( ( currentIndex < _imp->viewsComboBox->count() ) && (currentIndex != -1) ) {
        _imp->viewsComboBox->setCurrentIndex(currentIndex);
    } else {
        _imp->viewsComboBox->setCurrentIndex(0);
    }
    _imp->gui->updateViewsActions(count);
}

void
ViewerTab::setCurrentView(int view)
{
    _imp->viewsComboBox->setCurrentIndex(view);
}

int
ViewerTab::getCurrentView() const
{
    QMutexLocker l(&_imp->currentViewMutex);

    return _imp->currentViewIndex;
}

void
ViewerTab::setPlaybackMode(Natron::PlaybackModeEnum mode)
{
    QPixmap pix;
    switch (mode) {
        case Natron::ePlaybackModeLoop:
            appPTR->getIcon(NATRON_PIXMAP_PLAYER_LOOP_MODE, &pix);
            break;
        case Natron::ePlaybackModeBounce:
            appPTR->getIcon(NATRON_PIXMAP_PLAYER_BOUNCE, &pix);
            break;
        case Natron::ePlaybackModeOnce:
            appPTR->getIcon(NATRON_PIXMAP_PLAYER_PLAY_ONCE, &pix);
            break;
        default:
            break;
    }
    {
        QMutexLocker k(&_imp->playbackModeMutex);
        _imp->playbackMode = mode;
    }
    _imp->playbackMode_Button->setIcon(QIcon(pix));
    _imp->viewerNode->getRenderEngine()->setPlaybackMode(mode);

}

Natron::PlaybackModeEnum
ViewerTab::getPlaybackMode() const
{
    QMutexLocker k(&_imp->playbackModeMutex);
    return _imp->playbackMode;
}

void
ViewerTab::togglePlaybackMode()
{
    Natron::PlaybackModeEnum mode = _imp->viewerNode->getRenderEngine()->getPlaybackMode();
    mode = (Natron::PlaybackModeEnum)(((int)mode + 1) % 3);
    QPixmap pix;
    switch (mode) {
        case Natron::ePlaybackModeLoop:
            appPTR->getIcon(NATRON_PIXMAP_PLAYER_LOOP_MODE, &pix);
            break;
        case Natron::ePlaybackModeBounce:
            appPTR->getIcon(NATRON_PIXMAP_PLAYER_BOUNCE, &pix);
            break;
        case Natron::ePlaybackModeOnce:
            appPTR->getIcon(NATRON_PIXMAP_PLAYER_PLAY_ONCE, &pix);
            break;
        default:
            break;
    }
    {
        QMutexLocker k(&_imp->playbackModeMutex);
        _imp->playbackMode = mode;
    }
    _imp->playbackMode_Button->setIcon(QIcon(pix));
    _imp->viewerNode->getRenderEngine()->setPlaybackMode(mode);
}

void
ViewerTab::onClipToProjectButtonToggle(bool b)
{
    _imp->clipToProjectFormatButton->setDown(b);
    _imp->viewer->setClipToDisplayWindow(b);
}


void
ViewerTab::updateZoomComboBox(int value)
{
    assert(value > 0);
    QString str = QString::number(value);
    str.append( QChar('%') );
    str.prepend("  ");
    str.append("  ");
    _imp->zoomCombobox->setCurrentText_no_emit(str);
}

/*In case they're several viewer around, we need to reset the dag and tell it
   explicitly we want to use this viewer and not another one.*/
void
ViewerTab::startPause(bool b)
{
    abortRendering();
    if (b) {
        _imp->gui->getApp()->setLastViewerUsingTimeline(_imp->viewerNode->getNode());
        _imp->viewerNode->getRenderEngine()->renderFromCurrentFrame(getGui()->getApp()->isRenderStatsActionChecked(), OutputSchedulerThread::eRenderDirectionForward);
    }
}

void
ViewerTab::abortRendering()
{
    if (_imp->play_Forward_Button) {
        _imp->play_Forward_Button->setDown(false);
        _imp->play_Forward_Button->setChecked(false);
    }
    if (_imp->play_Backward_Button) {
        _imp->play_Backward_Button->setDown(false);
        _imp->play_Backward_Button->setChecked(false);
    }
    if (_imp->gui && _imp->gui->isGUIFrozen() && appPTR->getCurrentSettings()->isAutoTurboEnabled()) {
        _imp->gui->onFreezeUIButtonClicked(false);
    }
    if (_imp->gui) {
        ///Abort all viewers because they are all synchronised.
        const std::list<ViewerTab*> & activeNodes = _imp->gui->getViewersList();

        for (std::list<ViewerTab*>::const_iterator it = activeNodes.begin(); it != activeNodes.end(); ++it) {
            ViewerInstance* viewer = (*it)->getInternalNode();
            if (viewer) {
                viewer->getRenderEngine()->abortRendering(true);
            }
        }
    }
}

void
ViewerTab::onEngineStarted(bool forward)
{
    if (!_imp->gui) {
        return;
    }
    
    if (_imp->play_Forward_Button) {
        _imp->play_Forward_Button->setDown(forward);
        _imp->play_Forward_Button->setChecked(forward);
    }

    if (_imp->play_Backward_Button) {
        _imp->play_Backward_Button->setDown(!forward);
        _imp->play_Backward_Button->setChecked(!forward);
    }

    if (_imp->gui && !_imp->gui->isGUIFrozen() && appPTR->getCurrentSettings()->isAutoTurboEnabled()) {
        _imp->gui->onFreezeUIButtonClicked(true);
    }
}

void
ViewerTab::onEngineStopped()
{
    if (!_imp->gui) {
        return;
    }
    
    if (_imp->play_Forward_Button && _imp->play_Forward_Button->isDown()) {
        _imp->play_Forward_Button->setDown(false);
        _imp->play_Forward_Button->setChecked(false);
    }
    
    if (_imp->play_Backward_Button && _imp->play_Backward_Button->isDown()) {
        _imp->play_Backward_Button->setDown(false);
        _imp->play_Backward_Button->setChecked(false);
    }
    if (_imp->gui && _imp->gui->isGUIFrozen() && appPTR->getCurrentSettings()->isAutoTurboEnabled()) {
        _imp->gui->onFreezeUIButtonClicked(false);
    }
}

void
ViewerTab::startBackward(bool b)
{
    abortRendering();
    if (b) {
        _imp->gui->getApp()->setLastViewerUsingTimeline(_imp->viewerNode->getNode());
        _imp->viewerNode->getRenderEngine()->renderFromCurrentFrame(getGui()->getApp()->isRenderStatsActionChecked(), OutputSchedulerThread::eRenderDirectionBackward);

    }
}

void
ViewerTab::seek(SequenceTime time)
{
    _imp->currentFrameBox->setValue(time);
    _imp->timeLineGui->seek(time);
    
}

void
ViewerTab::previousFrame()
{
    int prevFrame = _imp->timeLineGui->currentFrame() -1 ;
    if (prevFrame  < _imp->timeLineGui->leftBound()) {
        prevFrame = _imp->timeLineGui->rightBound();
    }
    seek(prevFrame);
}

void
ViewerTab::nextFrame()
{
    int nextFrame = _imp->timeLineGui->currentFrame() + 1;
    if (nextFrame  > _imp->timeLineGui->rightBound()) {
        nextFrame = _imp->timeLineGui->leftBound();
    }
    seek(nextFrame);
}

void
ViewerTab::previousIncrement()
{
    seek( _imp->timeLineGui->currentFrame() - _imp->incrementSpinBox->value() );
}

void
ViewerTab::nextIncrement()
{
    seek( _imp->timeLineGui->currentFrame() + _imp->incrementSpinBox->value() );
}

void
ViewerTab::firstFrame()
{
    seek( _imp->timeLineGui->leftBound() );
}

void
ViewerTab::lastFrame()
{
    seek( _imp->timeLineGui->rightBound() );
}

void
ViewerTab::onTimeLineTimeChanged(SequenceTime time,
                                 int /*reason*/)
{
    if (!_imp->gui) {
        return;
    }
    _imp->currentFrameBox->setValue(time);
    
    if (_imp->timeLineGui->getTimeline() != _imp->gui->getApp()->getTimeLine()) {
        _imp->viewerNode->renderCurrentFrame(true);
    }
}

void
ViewerTab::onCurrentTimeSpinBoxChanged(double time)
{
    _imp->timeLineGui->seek(time);
}

void
ViewerTab::centerViewer()
{
    _imp->viewer->fitImageToFormat();
    if ( _imp->viewer->displayingImage() ) {
        _imp->viewerNode->renderCurrentFrame(false);
    } else {
        _imp->viewer->updateGL();
    }
}

void
ViewerTab::refresh(bool enableRenderStats)
{
    _imp->viewerNode->forceFullComputationOnNextFrame();
    if (!enableRenderStats) {
        _imp->viewerNode->renderCurrentFrame(false);
    } else {
        getGui()->getOrCreateRenderStatsDialog()->show();
        _imp->viewerNode->renderCurrentFrameWithRenderStats(false);
    }
}

void
ViewerTab::refresh()
{
    Qt::KeyboardModifiers m = qApp->keyboardModifiers();
    refresh(m.testFlag(Qt::ShiftModifier) && m.testFlag(Qt::ControlModifier));
}

ViewerTab::~ViewerTab()
{
    if (_imp->gui) {
        NodeGraph* graph = 0;
        if (_imp->viewerNode) {
            boost::shared_ptr<NodeCollection> collection = _imp->viewerNode->getNode()->getGroup();
            if (collection) {
                NodeGroup* isGrp = dynamic_cast<NodeGroup*>(collection.get());
                if (isGrp) {
                    NodeGraphI* graph_i = isGrp->getNodeGraph();
                    if (graph_i) {
                        graph = dynamic_cast<NodeGraph*>(graph_i);
                        assert(graph);
                    }
                } else {
                    graph = _imp->gui->getNodeGraph();
                }
            }
            _imp->viewerNode->invalidateUiContext();
        } else {
            graph = _imp->gui->getNodeGraph();
        }
        assert(graph);
        if ( _imp->app && !_imp->app->isClosing() && (graph->getLastSelectedViewer() == this) ) {
            graph->setLastSelectedViewer(0);
        }
    }
    for (std::map<NodeGui*,RotoGui*>::iterator it = _imp->rotoNodes.begin(); it != _imp->rotoNodes.end(); ++it) {
        delete it->second;
    }
    for (std::map<NodeGui*,TrackerGui*>::iterator it = _imp->trackerNodes.begin(); it != _imp->trackerNodes.end(); ++it) {
        delete it->second;
    }
}

void
ViewerTab::keyPressEvent(QKeyEvent* e)
{
    Qt::KeyboardModifiers modifiers = e->modifiers();
    Qt::Key key = (Qt::Key)e->key();

    if ( isKeybind(kShortcutGroupGlobal, kShortcutIDActionShowPaneFullScreen, modifiers, key) ) { //< this shortcut is global
        if ( parentWidget() ) {
            QKeyEvent* ev = new QKeyEvent(QEvent::KeyPress,key, modifiers);
            QCoreApplication::postEvent(parentWidget(),ev);
        }
    } else if ( isKeybind(kShortcutGroupViewer, kShortcutIDActionLuminance, modifiers, key) ) {
        int currentIndex = _imp->viewerChannels->activeIndex();
        if (currentIndex == 0) {
            _imp->viewerChannels->setCurrentIndex_no_emit(1);
            setDisplayChannels(1, true);
        } else {
            _imp->viewerChannels->setCurrentIndex_no_emit(0);
            setDisplayChannels(0, true);
        }
    } else if ( isKeybind(kShortcutGroupViewer, kShortcutIDActionRed, modifiers, key) ) {
        int currentIndex = _imp->viewerChannels->activeIndex();
        if (currentIndex == 2) {
            _imp->viewerChannels->setCurrentIndex_no_emit(1);
            setDisplayChannels(1, true);
        } else {
            _imp->viewerChannels->setCurrentIndex_no_emit(2);
            setDisplayChannels(2, true);
        }
    } else if ( isKeybind(kShortcutGroupViewer, kShortcutIDActionGreen, modifiers, key) ) {
        int currentIndex = _imp->viewerChannels->activeIndex();
        if (currentIndex == 3) {
            _imp->viewerChannels->setCurrentIndex_no_emit(1);
            setDisplayChannels(1, true);
        } else {
            _imp->viewerChannels->setCurrentIndex_no_emit(3);
            setDisplayChannels(3, true);
        }
    } else if ( isKeybind(kShortcutGroupViewer, kShortcutIDActionBlue, modifiers, key) ) {
        int currentIndex = _imp->viewerChannels->activeIndex();
        if (currentIndex == 4) {
            _imp->viewerChannels->setCurrentIndex_no_emit(1);
            setDisplayChannels(1, true);
        } else {
            _imp->viewerChannels->setCurrentIndex_no_emit(4);
            setDisplayChannels(4, true);
        }
    } else if ( isKeybind(kShortcutGroupViewer, kShortcutIDActionAlpha, modifiers, key) ) {
        int currentIndex = _imp->viewerChannels->activeIndex();
        if (currentIndex == 5) {
            _imp->viewerChannels->setCurrentIndex_no_emit(1);
            setDisplayChannels(1, true);
        } else {
            _imp->viewerChannels->setCurrentIndex_no_emit(5);
            setDisplayChannels(5, true);
        }
    } else if ( isKeybind(kShortcutGroupViewer, kShortcutIDActionLuminanceA, modifiers, key) ) {
        int currentIndex = _imp->viewerChannels->activeIndex();
        if (currentIndex == 0) {
            _imp->viewerChannels->setCurrentIndex_no_emit(1);
            setDisplayChannels(1, false);
        } else {
            _imp->viewerChannels->setCurrentIndex_no_emit(0);
            setDisplayChannels(0, false);
        }
    } else if ( isKeybind(kShortcutGroupViewer, kShortcutIDActionRedA, modifiers, key) ) {
        int currentIndex = _imp->viewerChannels->activeIndex();
        if (currentIndex == 2) {
            _imp->viewerChannels->setCurrentIndex_no_emit(1);
            setDisplayChannels(1, false);
        } else {
            _imp->viewerChannels->setCurrentIndex_no_emit(2);
            setDisplayChannels(2, false);
        }
    } else if ( isKeybind(kShortcutGroupViewer, kShortcutIDActionGreenA, modifiers, key) ) {
        int currentIndex = _imp->viewerChannels->activeIndex();
        if (currentIndex == 3) {
            _imp->viewerChannels->setCurrentIndex_no_emit(1);
            setDisplayChannels(1, false);
        } else {
            _imp->viewerChannels->setCurrentIndex_no_emit(3);
            setDisplayChannels(3, false);
        }
    } else if ( isKeybind(kShortcutGroupViewer, kShortcutIDActionBlueA, modifiers, key) ) {
        int currentIndex = _imp->viewerChannels->activeIndex();
        if (currentIndex == 4) {
            _imp->viewerChannels->setCurrentIndex_no_emit(1);
            setDisplayChannels(1, false);
        } else {
            _imp->viewerChannels->setCurrentIndex_no_emit(4);
            setDisplayChannels(4, false);
        }
    } else if ( isKeybind(kShortcutGroupViewer, kShortcutIDActionAlphaA, modifiers, key) ) {
        int currentIndex = _imp->viewerChannels->activeIndex();
        if (currentIndex == 5) {
            _imp->viewerChannels->setCurrentIndex_no_emit(1);
            setDisplayChannels(1, false);
        } else {
            _imp->viewerChannels->setCurrentIndex_no_emit(5);
            setDisplayChannels(5, false);
        }
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerPrevious, modifiers, key) ) {
        previousFrame();
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerBackward, modifiers, key) ) {
        startBackward( !_imp->play_Backward_Button->isDown() );
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerStop, modifiers, key) ) {
        abortRendering();
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerForward, modifiers, key) ) {
        startPause( !_imp->play_Forward_Button->isDown() );
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerNext, modifiers, key) ) {
        nextFrame();
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerPrevIncr, modifiers, key) ) {
        //prev incr
        previousIncrement();
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerNextIncr, modifiers, key) ) {
        //next incr
        nextIncrement();
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerFirst, modifiers, key) ) {
        //first frame
        firstFrame();
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerLast, modifiers, key) ) {
        //last frame
        lastFrame();
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerPrevKF, modifiers, key) ) {
        //prev key
        _imp->app->getTimeLine()->goToPreviousKeyframe();
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerNextKF, modifiers, key) ) {
        //next key
        _imp->app->getTimeLine()->goToNextKeyframe();
    } else if ( isKeybind(kShortcutGroupViewer, kShortcutIDActionFitViewer, modifiers, key) ) {
        centerViewer();
    } else if ( isKeybind(kShortcutGroupViewer, kShortcutIDActionClipEnabled, modifiers, key) ) {
        onClipToProjectButtonToggle( !_imp->clipToProjectFormatButton->isDown() );
    } else if ( isKeybind(kShortcutGroupViewer, kShortcutIDActionRefresh, modifiers, key) ) {
        refresh(false);
    } else if ( isKeybind(kShortcutGroupViewer, kShortcutIDActionRefreshWithStats, modifiers, key) ) {
        refresh(true);
    } else if ( isKeybind(kShortcutGroupViewer, kShortcutIDActionROIEnabled, modifiers, key) ) {
        onEnableViewerRoIButtonToggle( !_imp->enableViewerRoI->isDown() );
    } else if ( isKeybind(kShortcutGroupViewer, kShortcutIDActionProxyEnabled, modifiers, key) ) {
        onRenderScaleButtonClicked(!_imp->renderScaleActive);
    } else if ( isKeybind(kShortcutGroupViewer, kShortcutIDActionProxyLevel2, modifiers, key) ) {
        _imp->renderScaleCombo->setCurrentIndex(0);
    } else if ( isKeybind(kShortcutGroupViewer, kShortcutIDActionProxyLevel4, modifiers, key) ) {
        _imp->renderScaleCombo->setCurrentIndex(1);
    } else if ( isKeybind(kShortcutGroupViewer, kShortcutIDActionProxyLevel8, modifiers, key) ) {
        _imp->renderScaleCombo->setCurrentIndex(2);
    } else if ( isKeybind(kShortcutGroupViewer, kShortcutIDActionProxyLevel16, modifiers, key) ) {
        _imp->renderScaleCombo->setCurrentIndex(3);
    } else if ( isKeybind(kShortcutGroupViewer, kShortcutIDActionProxyLevel32, modifiers, key) ) {
        _imp->renderScaleCombo->setCurrentIndex(4);
    } else if (isKeybind(kShortcutGroupViewer, kShortcutIDActionZoomLevel100, modifiers, key) ) {
        _imp->viewer->zoomSlot(100);
        _imp->zoomCombobox->setCurrentIndex_no_emit(4);
    } else if (isKeybind(kShortcutGroupViewer, kShortcutIDActionZoomIn, modifiers, key) ) {
        zoomIn();
    } else if (isKeybind(kShortcutGroupViewer, kShortcutIDActionZoomOut, modifiers, key) ) {
        zoomOut();
    } else {
        QWidget::keyPressEvent(e);
    }
} // keyPressEvent

void
ViewerTab::setDisplayChannels(int i, bool setBothInputs)
{
    Natron::DisplayChannelsEnum channels;
    
    switch (i) {
        case 0:
            channels = Natron::eDisplayChannelsY;
            break;
        case 1:
            channels = Natron::eDisplayChannelsRGB;
            break;
        case 2:
            channels = Natron::eDisplayChannelsR;
            break;
        case 3:
            channels = Natron::eDisplayChannelsG;
            break;
        case 4:
            channels = Natron::eDisplayChannelsB;
            break;
        case 5:
            channels = Natron::eDisplayChannelsA;
            break;
        default:
            channels = Natron::eDisplayChannelsRGB;
            break;
    }
    _imp->viewerNode->setDisplayChannels(channels, setBothInputs);
}

void
ViewerTab::onViewerChannelsChanged(int i)
{
    setDisplayChannels(i, false);
}

bool
ViewerTab::eventFilter(QObject *target,
                       QEvent* e)
{
    if (e->type() == QEvent::MouseButtonPress) {
        if (_imp->gui && _imp->app) {
            boost::shared_ptr<NodeGuiI> gui_i = _imp->viewerNode->getNode()->getNodeGui();
            assert(gui_i);
            boost::shared_ptr<NodeGui> gui = boost::dynamic_pointer_cast<NodeGui>(gui_i);
            _imp->gui->selectNode(gui);
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
    if (!_imp->playerButtonsContainer->isVisible()) {
        return QSize(500,200);
    }
    return QWidget::minimumSizeHint();
}

QSize
ViewerTab::sizeHint() const
{
    if (!_imp->playerButtonsContainer->isVisible()) {
        return QSize(500,200);
    }

    return QWidget::sizeHint();
}

void
ViewerTab::showView(int view)
{
    {
        QMutexLocker l(&_imp->currentViewMutex);
        
        _imp->currentViewIndex = view;
    }
    abortRendering();
    _imp->viewerNode->renderCurrentFrame(true);
}
